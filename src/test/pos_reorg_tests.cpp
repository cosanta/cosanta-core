// Copyright (c) 2018-2026 The PirateCash Core developers
// Copyright (c) 2026 The Cosanta Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chain.h>
#include <chainparams.h>
#include <consensus/merkle.h>
#include <consensus/validation.h>
#include <index/txindex.h>
#include <interfaces/chain.h>
#include <pos_kernel.h>
#include <script/sign.h>
#include <script/signingprovider.h>
#include <script/standard.h>
#include <test/util/index.h>
#include <test/util/logging.h>
#include <test/util/setup_common.h>
#include <txmempool.h>
#include <validation.h>
#include <validationinterface.h>

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(pos_reorg_tests, TestChain100Setup)

BOOST_AUTO_TEST_CASE(disconnected_coinstake_skips_mempool_acceptance)
{
    auto& chainman = *m_node.chainman;
    auto& chainstate = chainman.ActiveChainstate();
    auto& mempool = *m_node.mempool;
    const CScript script{GetScriptForRawPubKey(coinbaseKey.GetPubKey())};
    // Cosanta's early block rewards are below the minimum stake amount.
    mineBlocks(120);

    FillableSigningProvider provider;
    provider.AddKey(coinbaseKey);
    CMutableTransaction funding;
    CAmount funding_value{0};
    size_t funding_inputs{0};
    while (funding_value < MIN_STAKE_AMOUNT + 1000) {
        BOOST_REQUIRE(funding_inputs < 120);
        funding.vin.emplace_back(COutPoint{m_coinbase_txns[funding_inputs]->GetHash(), 0});
        funding_value += m_coinbase_txns[funding_inputs++]->vout[0].nValue;
    }
    funding.vout.emplace_back(funding_value - 1000, script);
    for (size_t i = 0; i < funding_inputs; ++i) {
        BOOST_REQUIRE(SignSignature(provider, *m_coinbase_txns[i], funding, i, SIGHASH_ALL));
    }
    const auto funding_tx = MakeTransactionRef(funding);
    const CBlock funding_block{CreateAndProcessBlock({funding}, script)};
    const CBlockIndex* funding_index{WITH_LOCK(cs_main, return chainstate.m_chain.Tip())};
    BOOST_REQUIRE_EQUAL(funding_index->GetBlockHash(), funding_block.GetHash());

    // Header validation resolves the staking transaction through the index.
    BOOST_REQUIRE(!g_txindex);
    g_txindex = std::make_unique<TxIndex>(interfaces::MakeChain(m_node), 1 << 20, true);
    struct RestoreStakeState {
        uint32_t first_pos{nFirstPoSBlock};
        ~RestoreStakeState() {
            nFirstPoSBlock = first_pos;
            SyncWithValidationInterfaceQueue();
            g_txindex->Stop();
            g_txindex.reset();
        }
    } restore;
    BOOST_REQUIRE(g_txindex->Start());
    IndexWaitSynced(*g_txindex);
    nFirstPoSBlock = funding_index->nHeight + 1;
    SetMockTime(funding_block.GetBlockTime() + Params().MinStakeAge() + 1);

    CMutableTransaction coinstake;
    coinstake.vin.emplace_back(COutPoint{funding_tx->GetHash(), 0});
    coinstake.vout.emplace_back(0, CScript{});
    const CScript stake_script{GetScriptForDestination(PKHash(coinbaseKey.GetPubKey()))};
    coinstake.vout.emplace_back(funding_tx->vout[0].nValue, stake_script);
    BOOST_REQUIRE(SignSignature(provider, *funding_tx, coinstake, 0, SIGHASH_ALL));
    const auto stake_tx = MakeTransactionRef(coinstake);
    BOOST_REQUIRE(stake_tx->IsCoinStake());

    const auto ordinary_tx = CreateValidMempoolTransaction(
        m_coinbase_txns[119], 0, 120, coinbaseKey, script,
        m_coinbase_txns[119]->vout[0].nValue - 1000, /*submit=*/true);

    // A missing reorg guard still rejects coinstake in PreChecks. Observe the
    // redundant admission attempt, and prove the log observation is active.
    const std::string rejection_log{"AcceptToMemoryPool: " + stake_tx->GetHash().ToString() + " coinstake"};
    {
        ASSERT_DEBUG_LOG(rejection_log);
        LOCK(cs_main);
        const auto result = chainman.ProcessTransaction(stake_tx);
        BOOST_REQUIRE(result.m_result_type == MempoolAcceptResult::ResultType::INVALID);
        BOOST_CHECK_EQUAL(result.m_state.GetRejectReason(), "coinstake");
    }

    // Use normal block validation and real, signed spends of mature coins.
    CBlock block{CreateBlock({coinstake, ordinary_tx}, script, chainstate)};
    block.nVersion |= CBlockHeader::POS_BIT;
    block.posStakeHash = funding_tx->GetHash();
    block.posStakeN = 0;
    block.nTime = GetTime();
    CMutableTransaction coinbase{*block.vtx[0]};
    coinbase.vout[0] = CTxOut{0, stake_script};
    block.vtx[0] = MakeTransactionRef(std::move(coinbase));
    block.hashMerkleRoot = BlockMerkleRoot(block);
    uint256 proof;
    BOOST_REQUIRE(CheckStakeKernelHash(block, *funding_index, *funding_index, *funding_tx,
                                      block.StakeInput(), 60, /*fCheck=*/false, proof, false));
    BOOST_REQUIRE(coinbaseKey.SignCompact(block.GetHash(), block.posBlockSig));
    block.fChecked = false;
    block.m_checked_merkle_root = false;
    BOOST_REQUIRE(block.IsProofOfStake());
    BOOST_REQUIRE(!block.IsProofOfStakeV2());
    BOOST_REQUIRE(chainman.ProcessNewBlock(std::make_shared<const CBlock>(block), true, nullptr));
    CBlockIndex* tip;
    {
        LOCK(cs_main);
        tip = chainstate.m_chain.Tip();
        BOOST_REQUIRE_EQUAL(tip->GetBlockHash(), block.GetHash());
        BOOST_CHECK_EQUAL(mempool.size(), 0U);
    }

    bool attempted_stake_admission{false};
    {
        DebugLogHelper reject_log{rejection_log, [&](const std::string* line) {
            if (line) attempted_stake_admission = true;
            return false;
        }};
        BlockValidationState state;
        BOOST_REQUIRE(chainstate.InvalidateBlock(state, tip));
    }
    BOOST_CHECK_MESSAGE(!attempted_stake_admission, "Disconnected coinstake was submitted to mempool acceptance");
    {
        LOCK(cs_main);
        BOOST_CHECK_EQUAL(chainstate.m_chain.Tip()->GetBlockHash(), block.hashPrevBlock);
        BOOST_CHECK(!mempool.exists(stake_tx->GetHash()));
        BOOST_CHECK(mempool.exists(ordinary_tx.GetHash()));
        BOOST_CHECK_EQUAL(mempool.size(), 1U);
    }
}

BOOST_AUTO_TEST_SUITE_END()
