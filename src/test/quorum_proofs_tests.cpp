// Copyright (c) 2025-2026 The Dash Core developers
// Distributed under the MIT software license, see the accompanying file COPYING.
#include <boost/test/unit_test.hpp>
#include <algorithm>
#include <chainlock/clsig.h>
#include <chainparamsbase.h>
#include <consensus/merkle.h>
#include <context.h>
#include <evo/cbtx.h>
#include <evo/evodb.h>
#include <evo/specialtx.h>
#include <future>
#include <hash.h>
#include <llmq/blockprocessor.h>
#include <llmq/context.h>
#include <llmq/quorumproofs.h>
#include <llmq/quorumsman.h>
#include <llmq/signhash.h>
#include <node/blockstorage.h>
#include <pow.h>
#include <rpc/server.h>
#include <streams.h>
#include <test/util/setup_common.h>
#include <validation.h>

// Synthetic testnet proof with Cosanta header hashes and deterministic test BLS
// keys, retaining the 0, 1 and 4 ancestor paths of the upstream wire fixture.
static const char* TESTNET_PROOF =
    "444153484e43303201d4a01700b7559b529c5645d6f17ec90261a98496f9f10173f2a1bc1646ac2d9ca90000002180d31e9d"
    "cc7a0b62c266dac8d0019927b34583e07ccc9b6dc1913d8c9cf4c064525403d021085e852f71c734f26d111d86b6610dc4c9"
    "6de3c01abf277db75243010000030001f377b58af19b177c75bfc1c2c7ceb962911a2b68100db7bc4f50f988a700000032ff"
    "ffffffffff0332ffffffffffff0397f1d3a73197d7942695638c4fa9ac0fc3688c4f9774b905a14e3a3f171bac586c55e83f"
    "f97a1aeffb3af00adb22c6bb97f27eb5c29258b07e0f41b44c9cdc7180f241c0b20fb749cd029cfef29e0230a850f28c4261"
    "1d27c6987a461a38b348360a7f40f8ecbedb6c4f3114ed3ee0a074d20d8eac81a126e25bc17d65b2b01f010e97fdcd5a64dc"
    "f72ac039da682641cd4f73ab7c77962ef770e146fe55d632192a8dcebbead81d4b6f0a71b8e010118c01d8a9e1c9ba466615"
    "e6a52fb8508d30a687808a13d05a3019588fca4d5d76a3b54e7776e3b0c7b8fc010375a147d017a6b995e66c6ec77f144bdc"
    "8bdffd41e924bc0071fd940c0a89083ee1841481ada9e0a925f6db0f0103fc277da18885020000006d000000070bbad189bc"
    "db405fc4f337d3155f9a9254788edc95b3b8e85cdde5408655132bd95fff59b78300333e920f14c85318e3a08f151390c9db"
    "07ea90e95a3dcd4a17335881e7ef79427373278c5d75db84b88db1b932a98280f0f63816c49bba7680356cc8d193deaafc50"
    "abb4ed43c15cea4d4e4617884b013897020d2fa7cce9370fbf179099dfa4ff693ca19a4b5969ead9a032a89de35b2cdf296b"
    "21330b6760a8660185d2dcf378464c4f1a41f9a23e8b60a4c2d81c942ec50bb6641b394dd878f4e1d26fbee716ee68ded746"
    "6648d77b2b027cf2a68f14ae1df48686c4f1440300e3a0170000000020ffe8a875515f906d4e01c3afd3eb4539ac9bb72248"
    "6636acce9bec0f51000000875fc4a522ef6b30f063652efd1186e66b3bae14f58a0d875599bf0a9631c88a2b519d6a6fa800"
    "1ef4b30600b617fe2b20f50487903d4536adc5844d289654970fbfbf28bd9e877b20ff7d5a33a2c5745848f390e1829862fa"
    "69c66a08c1775aa11753247728561e55d3966ab60b3f40ea0092d75d6b0c9489a21e9f4e5e555b2cc858ad32049f789a96f4"
    "475601000003000600000000000000fd49010100e3a017000300013febcaedd1db22c960be4055a715f953a636c3ad24497c"
    "adf23b416e1c00000032ffffffffffff0332ffffffffffff03a572cbea904d67468808c8eb50a9450c9721db309128012543"
    "902d0ac358a62ae28f75bb8f1c7c42c39a8c5529bf0f4e06e11160c25112684c0713d7d9347f2568cf000a5c3414e16ca415"
    "16243d59b495434dc61c1c86e60a1277976ba427135a05a3d45ee68dd2cd9de705c843866c1c4537907c72570a976b031849"
    "06f6a71667926727aaae73006d3334d718604782eb3fa5b162bf1c6f4319083ecb7bf5a1940315f41a8b16de92e372771466"
    "1c82523b37f119ccfd2cb08268f3b41d80208116309117f9c7ea3c3c63a24ebe7c02b89a5aad8fea24025805232197889f02"
    "b585abcef3949129f4ec9d0df597ebfcb0fe52a4269cc143a2fb2b60ded5b556ea3307d9edf0ce3cd2d3cb90cb472c010000"
    "00040000000279ca353e0de915e395676997024b602d3856e940a413ed9c6f1d0c5defdd050fa59444f0db90dcfb4abebb6b"
    "2181bc36afc13b8c17a0066810edbc248f9e3ada00001ca2170000000020051c4196384d1681f282786857cd4611e9a32be8"
    "e2a924636e8879ad752a1f0f261154fda3fba0a9490d6772eeeb5ce019d8b1be03f085e8c2be88f71e1a2c3b06f99d6ae4cd"
    "001e20460e00afcf4d6eaa50f6418f1f4b44711c7ebeab3bbb4ed276d1970739a94e4ad656d61deb78f3b518440d750e2c51"
    "82b6c3260385831862d248199d9eb91507d58747321c60ec6cdc52d004e6142a822d040e9f816eeb924623f0e6f3bc624901"
    "01e95601000003000600000000000000fd490101001ba2170003000138e92399565737d10b9a47ce26d3742f1ee77b66d589"
    "4530eb00d37cdb00000032ffffffffffff0332ffffffffffff0389ece308f9d1f0131765212deca99697b112d61f9be9a5f1"
    "f3780a51335b3ff981747a0b2ca2179b96d2c0c9024e52240026d7812596a8806df233b87070dc759ca432a0d5ac8b397bdf"
    "5c81a55dc954a42d9575d5b39d099b7a15fbad36db6cde0ff0bf58a9949530a9403f49150c323e528c0a410e02c076884885"
    "ebcc846e18995c6ca7152824988505542ff6cbc45d32c09fb5c4e2c54acbf7d3d2c3903d641ad75bb0b5e683f33f89fdeb01"
    "33469155424ac7247828917f2670edd70484c8447c7917b143125a01486e304de89218fe5b9780acb733d6d9c8127b688ae6"
    "0233a382bab13dc36e26f8af7f6c80b1dc3fed2a4ea5e0e83192ce69a7d6d199e1104255174f4cbbb8ad8a54283e87490100"
    "0000040000000209916264ab2466c1d9ff4863293b5872582ed9d91c3457db8661cd4536da68c066fecf371fcf1c13a7d810"
    "e200172086a9a469e2a5e3095a40b322964771085c01000000002015d04f67aef05968f56c42835c3ee9ba14ff7e1845bd79"
    "ffa98d5abb2a0000003865a3b223253b2926188c6d56838d194cadfdb6a77d67358b6aee42ff3e43f19ff79d6a3bc9001ecf"
    "c70c0047a4170000000020fd59bb3d59028ce2ce9ee3278070e2eb1c01876d12a222c5e026231660791236962e5f6fae97a0"
    "6781f0a95278511094c1f2fa4df863c426e73cd20b6ee373300e299f6ad938011ec05d0e008a402a304962136ed03527886d"
    "05a10591a9223e6746da266dd97bce569feea27e7e86bfce9bde783f4465a4b60a4d870e06a9697477f629d75b9d912ca66f"
    "84c1e89693a978a8d2078c6b323ab388ee89019394958195db65ecdb0c399e599b5601000003000600000000000000fd4901"
    "010043a41700030001d21f6a8ab554a886922406a9731081e7da5ded14f4ac2f9eb1ec3edd7700000032ffffffffffff0332"
    "ffffffffffff03ac9b60d5afcbd5663a8a44b7c5a02f19e9a77ab0a35bd65809bb5c67ec582c897feb04decc694b13e08587"
    "f3ff9b5b60ff27d1174b74f2c5b68e4fdae4dd193c8bdee18d218e3ce061d3ac75726dbf0eb9395e2e1e07dd4757bde426b8"
    "f6a9254d4f50d1c48eb51c4ff26415c294bf6e2196db7da343a0a85221ae04025fb726127349b23f6e8c9c9cc8c7ccbf81b2"
    "4e1c69b322eddc8bfa3c17f2cc7c7a983568e5ba9e8544e93456a70ee2ddd758feaf51515b5c05084a9e68b167f57d84dc43"
    "ed963ede7e34d879b4205a193b30d35fdceade18b606a1aa9c19e03f9048f00abc96d88309a9f06d427699caee299ba672b9"
    "ecbbba19e72f61171cbe95451a9882240976c652390830c39a03e408f8010000000500000003e981e0af4ced7c8d0ca3e50e"
    "9c438f8ab44dbdbd0b3f6145e95a55cdd262bbd8c2f12457d5fcdda836c8e8b872d3d35f3872551e557c6385016e40b963e5"
    "6cebfa5b9e96f4bc7976d1a57b911054844ae039a821ece72a881a7ae0dc1715d2380400000000203c143d3b0c2a6025fd1f"
    "c1e86536d54b6caddbbd5ccca5530d0e326f90010000229af3df4d8b6fc28b5a096049815350286c53d93f11d1fce3d09747"
    "350071c85a269f6aff5b011e2a7e0100000000203cb2b3550a3b685833a900a382ac3137c5cea53f8a45fc793bfd2a5de95f"
    "14a51738c1b5f920940752bfa7dd408b1e05a2cc413513223eccf70d2ec8d03fbbce24279f6af816011ebcdb0c0000000020"
    "81c6150ba493de049cc8399914d2cbaa5ac8586f410d63c1560cb330a9b69dc56e145b97802c84d157023105d8a8f847e295"
    "2b29c8656f7f1343e9e186f0d51168289f6adb21011eed440a00000000200dac1bb592800dd5cd8eb865dd31b383245a8bd1"
    "c429997ed73aceba8bc49396dfda21c0255e83d9df7f6d31ef5521fa150f6f40caca08d095ff489a14307606c4289f6a1f31"
    "011e071c0900eba4170000000020ca2ba845159ffc0acbfa5b60b8cc2109f8987e66622f489458ed88a26c000000a09092e0"
    "06bd3582faaba706775243c0c468f79cad4490e1cf537cf641cdff3db97e9f6a18a4001e78dd0e0080e8a8ecdba3c82df609"
    "98971e7aec9f6649326cd7ac78077d5b6029100ca4a6015bcbd552bc7659f488d6e4018e852f0da9d655168a2f548ed8b32d"
    "86e8bafcaf1d96e96e50db4c7d042621522512a4fec650c2a9e57c703e91cb99d9a0eb423201000003000500010000000000"
    "000000000000000000000000000000000000000000000000000000ffffffff016affffffff03b84b8c03000000001976a914"
    "b489115851ca07a26a5ad8bac3cec3c7dbebd83188ac2ed5fd0300000000016af80da706000000001976a91464f2b2b84f62"
    "d68a2cd7f7f5fb2b5aa75ef716d788ac00000000af0300eba417002180d31e9dcc7a0b62c266dac8d0019927b34583e07ccc"
    "9b6dc1913d8c9cf4c04f7a628c9c683a9b5d0a865977ff6d0782e2ffc879981ec6e4fb7caa9d76496300b59e80b1894fe733"
    "fad1e8316070314fe024b93d9b7b51baf63b2642d3d89495ee4bc69c68ec225e7eaa191719846fd00c0d5421047df2b42eac"
    "1386d908c3e755f642d3055c5c3a75b85500b358dce7cc5a17730ad9c75ced9c9e98c491e98a9465d733de20000000000000"
    "0400000002b3336e59b1bc4bdccc3397c24a85c882184d2fb8d6f0a563d19100cadc53bdeb4d864dd73362cbcda1ec96a3f3"
    "3ad9678edcd1ff85bb3f2581b8d54d57685615"
;
struct QuorumProofsRegtestSetup : BasicTestingSetup {
    QuorumProofsRegtestSetup() : BasicTestingSetup(CBaseChainParams::REGTEST) {}
};

// Use a single active quorum to deterministically exercise retirement at a
// mining boundary. The production selector, disk reader, builder and RPC run
// unchanged; only PoW difficulty and the active-set size are reduced.
struct QuorumProofGenerationSetup : TestingSetup {
    Consensus::Params saved_consensus;
    CBlockIndex* saved_tip;
    int checkpoint_height;
    int handoff_height{0};
    bool mine_nonstandard_handoff{false};
    std::vector<llmq::CFinalCommitment> commitments;
    std::vector<CBLSSecretKey> keys;

    QuorumProofGenerationSetup() :
        TestingSetup(CBaseChainParams::TESTNET),
        saved_consensus(Params().GetConsensus()),
        saved_tip(WITH_LOCK(cs_main, return m_node.chainman->ActiveChain().Tip()))
    {
        auto& consensus = const_cast<Consensus::Params&>(Params().GetConsensus());
        consensus.powLimit = uint256S("7fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        for (auto& params : consensus.llmqs) {
            if (params.type == consensus.llmqTypeChainLocks) params.signingActiveQuorumCount = 1;
        }
        const auto params = *Params().GetLLMQ(consensus.llmqTypeChainLocks);
        checkpoint_height = ((consensus.V20Height + 2 * params.dkgInterval) / params.dkgInterval) * params.dkgInterval +
                            params.dkgMiningWindowStart;
    }

    ~QuorumProofGenerationSetup()
    {
        WITH_LOCK(cs_main, m_node.chainman->ActiveChain().SetTip(*saved_tip));
        const_cast<Consensus::Params&>(Params().GetConsensus()) = saved_consensus;
    }

    void CreateHistory()
    {
        const auto& consensus = Params().GetConsensus();
        const auto params = *Params().GetLLMQ(consensus.llmqTypeChainLocks);
        auto& chainman = *m_node.chainman;
        auto& chain = *WITH_LOCK(cs_main, return &chainman.ActiveChain());
        int first_height = checkpoint_height;
        for (const auto& quorum_params : consensus.llmqs) {
            first_height = std::min(first_height, checkpoint_height - 2 * quorum_params.dkgInterval);
        }
        handoff_height = checkpoint_height + params.dkgInterval;
        const int last_height = (mine_nonstandard_handoff ? handoff_height : checkpoint_height) +
                                llmq::SIGN_HEIGHT_OFFSET + 1;
        CBlockIndex* previous{nullptr};
        for (int height = first_height; height <= last_height; ++height) {
            CCbTx payload;
            payload.nVersion = CCbTx::Version::CLSIG_AND_BALANCE;
            payload.nHeight = height;
            if (!commitments.empty()) payload.merkleRootQuorums = SerializeHash(commitments.back());
            if (height > checkpoint_height + 1) {
                const auto signer = llmq::SelectCommitmentForSigning(params, *m_node.llmq_ctx->qman,
                                                                     chainlock::GenSigRequestId(height - 1),
                                                                     chain[height - 1 - llmq::SIGN_HEIGHT_OFFSET]);
                BOOST_REQUIRE(signer);
                const auto match = std::find_if(commitments.begin(), commitments.end(), [&](const auto& entry) {
                    return entry.quorumHash == signer->quorumHash;
                });
                BOOST_REQUIRE(match != commitments.end());
                const size_t key = size_t(match - commitments.begin());
                payload.bestCLSignature = keys[key].Sign(llmq::SignHash{params.type, signer->quorumHash,
                                                                        chainlock::GenSigRequestId(height - 1),
                                                                        previous->GetBlockHash()}
                                                             .Get(),
                                                         false);
            }
            const bool mining = height == checkpoint_height - params.dkgInterval || height == checkpoint_height ||
                                (mine_nonstandard_handoff && height == handoff_height);
            if (mining) {
                keys.emplace_back();
                keys.back().MakeNewKey();
                llmq::CFinalCommitment commitment;
                commitment.nVersion = llmq::CFinalCommitment::BASIC_BLS_NON_INDEXED_QUORUM_VERSION;
                commitment.llmqType = params.type;
                commitment.quorumHash = previous->GetAncestor(height - params.dkgMiningWindowStart)->GetBlockHash();
                commitment.quorumPublicKey = keys.back().GetPublicKey();
                commitment.signers.assign(params.size, true);
                commitment.validMembers.assign(params.size, true);
                commitments.push_back(commitment);
                payload.merkleRootQuorums = SerializeHash(commitment);
            }
            CMutableTransaction coinbase;
            coinbase.nVersion = 3;
            coinbase.nType = TRANSACTION_COINBASE;
            coinbase.vin.resize(1);
            coinbase.vin[0].scriptSig = CScript() << height << OP_0;
            coinbase.vout.emplace_back(0, CScript() << OP_TRUE);
            if (mine_nonstandard_handoff && height > checkpoint_height) {
                // Consensus bounds a coinbase by transaction size alone.
                coinbase.vout.resize(4097, CTxOut(0, CScript() << OP_TRUE));
            }
            SetTxPayload(coinbase, payload);
            CBlock block;
            block.nVersion = 1;
            block.hashPrevBlock = previous ? previous->GetBlockHash() : uint256{};
            block.nBits = 0x207fffff;
            block.nTime = Params().GenesisBlock().nTime + height;
            block.vtx = {MakeTransactionRef(coinbase)};
            if (mining) {
                CMutableTransaction tx;
                tx.nVersion = 3;
                tx.nType = TRANSACTION_QUORUM_COMMITMENT;
                if (mine_nonstandard_handoff && height == handoff_height) {
                    // Consensus permits any special version here, allows rather
                    // than requires empty inputs and outputs, and never reads
                    // the lock time of a mined commitment.
                    tx.nVersion = 4;
                    tx.nLockTime = 1;
                    tx.vin.emplace_back(COutPoint(uint256::ONE, 0));
                    tx.vout.emplace_back(0, CScript() << OP_TRUE);
                }
                llmq::CFinalCommitmentTxPayload qc;
                qc.nHeight = height;
                qc.commitment = commitments.back();
                if (mine_nonstandard_handoff && height == handoff_height) {
                    // Decoys ahead of the real commitment: the same quorum hash under
                    // another type, and another quorum hash under the same type. The
                    // prover must still pick exactly the real one.
                    for (int decoy = 0; decoy < 2; ++decoy) {
                        CMutableTransaction other;
                        other.nVersion = 3;
                        other.nType = TRANSACTION_QUORUM_COMMITMENT;
                        auto payload = qc;
                        if (decoy == 0) {
                            payload.commitment.llmqType = Consensus::LLMQType::LLMQ_400_60;
                        } else {
                            payload.commitment.quorumHash = uint256::ONE;
                        }
                        SetTxPayload(other, payload);
                        block.vtx.push_back(MakeTransactionRef(other));
                    }
                }
                SetTxPayload(tx, qc);
                block.vtx.push_back(MakeTransactionRef(tx));
            }
            block.hashMerkleRoot = BlockMerkleRoot(block);
            while (!CheckProofOfWork(block.GetHash(), block.nBits, consensus))
                ++block.nNonce;
            const auto pos = chainman.m_blockman.SaveBlockToDisk(block, height, nullptr);
            BOOST_REQUIRE(!pos.IsNull());
            LOCK(cs_main);
            auto* index = chainman.m_blockman.InsertBlockIndex(block.GetHash());
            index->nVersion = block.nVersion;
            index->hashMerkleRoot = block.hashMerkleRoot;
            index->nTime = block.nTime;
            index->nBits = block.nBits;
            index->nNonce = block.nNonce;
            index->pprev = previous;
            index->nHeight = height;
            index->nFile = pos.nFile;
            index->nDataPos = pos.nPos;
            index->nStatus = BLOCK_HAVE_DATA;
            chain.SetTip(*index);
            previous = index;
            if (mining) {
                const auto& qc = commitments.back();
                m_node.evodb->Write(std::make_pair(std::string{"q_mc"}, std::make_pair(params.type, qc.quorumHash)),
                                    std::make_pair(qc, block.GetHash()));
                m_node.evodb->Write(std::make_tuple(std::string{"q_mcih"}, params.type,
                                                    htobe32_internal(UINT32_MAX - height)),
                                    height - params.dkgMiningWindowStart);
            }
        }
    }

    UniValue Generate(int minimum)
    {
        JSONRPCRequest request;
        request.context = CoreContext{m_node};
        request.strMethod = "getquorumproofchain";
        request.params = UniValue{UniValue::VARR};
        request.params.push_back(
            WITH_LOCK(cs_main, return m_node.chainman->ActiveChain()[checkpoint_height]->GetBlockHash().ToString()));
        request.params.push_back(minimum);
        if (RPCIsInWarmup(nullptr)) SetRPCWarmupFinished();
        return tableRPC.execute(request);
    }
};

BOOST_FIXTURE_TEST_SUITE(quorum_proofs_tests, QuorumProofsRegtestSetup)
BOOST_FIXTURE_TEST_CASE(generation_retries_retired_checkpoint_signer, QuorumProofGenerationSetup)
{
    CreateHistory();
    auto& chain = *WITH_LOCK(cs_main, return &m_node.chainman->ActiveChain());
    const auto* checkpoint = chain[checkpoint_height];
    chainlock::CoinbaseChainLockReader reader(chain.Tip());
    llmq::QuorumProofBuilder builder(*m_node.llmq_ctx->quorum_block_processor, *m_node.llmq_ctx->qman, chain.Tip(),
                                     *m_node.chainman, reader);
    const auto first = reader.Find(checkpoint_height + 1, chain.Height());
    BOOST_REQUIRE(first);
    BOOST_CHECK_EQUAL(first->height, checkpoint_height + 1);
    BOOST_CHECK(!builder.Build(checkpoint, first->Signed()));

    const auto result = Generate(checkpoint_height + 1);
    const auto proof = llmq::QuorumProofChain::Decode(ParseHex(result["proof_hex"].get_str()));
    BOOST_CHECK_EQUAL(proof.Verify(llmq::QuorumProofBuilder::StateAt(checkpoint)).height,
                      checkpoint_height + llmq::SIGN_HEIGHT_OFFSET);
    BOOST_CHECK(proof.links.empty());

    // A later certificate is required; exhaustion must not return the unbound target.
    auto* tip = chain.Tip();
    WITH_LOCK(cs_main, chain.SetTip(*chain[checkpoint_height + 2]));
    BOOST_CHECK_THROW(Generate(checkpoint_height + 1), UniValue);
    BOOST_CHECK_THROW(Generate(0), UniValue);
    WITH_LOCK(cs_main, chain.SetTip(*tip));

    // Missing history is a hard error, not a reason to skip to another target.
    // A state read once is memoized by block hash, so start from a cold cache.
    llmq::ClearProofStateCacheForTesting();
    WITH_LOCK(cs_main, chain[checkpoint_height]->nStatus &= ~BLOCK_HAVE_DATA);
    BOOST_CHECK_EXCEPTION(Generate(checkpoint_height + 1), UniValue, [](const UniValue& error) {
        return error["message"].get_str().find("historical block unavailable") != std::string::npos;
    });
}
BOOST_FIXTURE_TEST_CASE(consensus_valid_envelopes_are_provable, QuorumProofGenerationSetup)
{
    // A handoff must stay provable whatever shape the miner gave the mined
    // commitment, otherwise one block mined by custom software permanently
    // removes that quorum's key from every proof.
    mine_nonstandard_handoff = true;
    CreateHistory();
    auto& chain = *WITH_LOCK(cs_main, return &m_node.chainman->ActiveChain());
    const auto* checkpoint = chain[checkpoint_height];
    chainlock::CoinbaseChainLockReader reader(chain.Tip());
    llmq::QuorumProofBuilder builder(*m_node.llmq_ctx->quorum_block_processor, *m_node.llmq_ctx->qman, chain.Tip(),
                                     *m_node.chainman, reader);
    const auto entry = reader.Find(handoff_height + llmq::SIGN_HEIGHT_OFFSET, chain.Height());
    BOOST_REQUIRE(entry);
    const auto proof = builder.Build(checkpoint, entry->Signed());
    BOOST_REQUIRE(proof);
    BOOST_REQUIRE_EQUAL(proof->links.size(), 1U);

    CDataStream stream(proof->links[0].mining.transaction, SER_NETWORK, PROTOCOL_VERSION);
    CMutableTransaction mining;
    stream >> mining;
    // Two decoy commitments were mined ahead of it in the same block.
    BOOST_CHECK_EQUAL(proof->links[0].mining.path.index, 3U);
    BOOST_CHECK_EQUAL(mining.nVersion, 4);
    BOOST_CHECK_EQUAL(mining.nLockTime, 1U);
    BOOST_CHECK(!mining.vin.empty());
    BOOST_CHECK(!mining.vout.empty());

    const auto state = proof->Verify(llmq::QuorumProofBuilder::StateAt(checkpoint));
    BOOST_CHECK_EQUAL(state.height, uint32_t(entry->height));

    // The RPC reuses the target block's own state instead of re-verifying the
    // proof; Build() only returns a proof whose verified state is exactly that.
    BOOST_CHECK(state == llmq::QuorumProofBuilder::StateAt(chain[state.height]));

    // The memoized active set is the committed set: its leaves are sorted and
    // hash to the block's own quorum root.
    const auto active = builder.ActiveCommitments(checkpoint);
    std::vector<uint256> active_leaves;
    for (const auto& commitment : active)
        active_leaves.push_back(::SerializeHash(commitment));
    BOOST_CHECK(std::is_sorted(active_leaves.begin(), active_leaves.end()));
    BOOST_CHECK(ComputeMerkleRoot(active_leaves) == llmq::QuorumProofBuilder::StateAt(checkpoint).quorumRoot);

    // Encoding with the caller's verified state is byte-identical to verifying again,
    // and a state that is not the proof's own target is refused.
    const auto target_set = builder.ActiveCommitments(chain[state.height]);
    BOOST_REQUIRE(!target_set.empty());
    std::vector<uint256> leaves;
    for (const auto& commitment : target_set)
        leaves.push_back(::SerializeHash(commitment));
    CDataStream raw(SER_NETWORK, PROTOCOL_VERSION);
    raw << target_set.front();
    const llmq::ProofProjection record{0,
                                       {UCharCast(raw.data()), UCharCast(raw.data()) + raw.size()},
                                       llmq::ProofMerklePath::Build(leaves, 0)};
    BOOST_CHECK(llmq::EncodeBootstrap(*proof, {record}) == llmq::EncodeBootstrap(*proof, state, {record}));
    auto wrong = state;
    wrong.quorumRoot = uint256::ONE;
    BOOST_CHECK_THROW(llmq::EncodeBootstrap(*proof, wrong, {record}), std::runtime_error);
    wrong = state;
    wrong.height -= 1;
    BOOST_CHECK_THROW(llmq::EncodeBootstrap(*proof, wrong, {record}), std::runtime_error);
    BOOST_CHECK(state.blockHash == proof->target.header.GetHash());
}
BOOST_AUTO_TEST_CASE(testnet_wire_and_crypto) {
    auto bytes = ParseHex(TESTNET_PROOF);
    auto proof = llmq::QuorumProofChain::Decode(bytes);
    BOOST_CHECK(proof.Encode() == bytes);
    auto target = proof.Verify(proof.anchor);
    BOOST_CHECK_EQUAL(target.height, 1549547U);
    BOOST_CHECK(target.blockHash == proof.target.header.GetHash());
    auto trusted = proof.anchor;
    trusted.blockHash = uint256::ONE;
    BOOST_CHECK_THROW(proof.Verify(trusted), std::exception);
}
BOOST_AUTO_TEST_CASE(all_truncations_and_old_format_rejected) {
    auto bytes = ParseHex(TESTNET_PROOF);
    for (size_t size = 0; size < bytes.size(); ++size) {
        std::vector<unsigned char> truncated(bytes.begin(), bytes.begin() + size);
        BOOST_CHECK_THROW(llmq::QuorumProofChain::Decode(truncated), std::exception);
    }
    auto old = bytes;
    old[7] = '1';
    BOOST_CHECK_THROW(llmq::QuorumProofChain::Decode(old), std::exception);
    bytes.push_back(0);
    BOOST_CHECK_THROW(llmq::QuorumProofChain::Decode(bytes), std::exception);
}
BOOST_AUTO_TEST_CASE(tampering_cannot_authenticate_state) {
    const auto bytes = ParseHex(TESTNET_PROOF);
    const auto trusted = llmq::QuorumProofChain::Decode(bytes).anchor;
    for (size_t pos : {size_t(8), size_t(13), size_t(120), size_t(600), bytes.size() / 2, bytes.size() - 1}) {
        auto bad = bytes;
        bad[pos] ^= 1;
        BOOST_CHECK_THROW(llmq::QuorumProofChain::Decode(bad).Verify(trusted), std::exception);
    }
    auto proof = llmq::QuorumProofChain::Decode(bytes);
    proof.target.signature.Reset();
    BOOST_CHECK_THROW(proof.Verify(trusted), std::exception);
    proof = llmq::QuorumProofChain::Decode(bytes);
    proof.links[1].ancestors.clear();
    BOOST_CHECK_THROW(proof.Verify(trusted), std::exception);
    proof = llmq::QuorumProofChain::Decode(bytes);
    proof.links[2].ancestors[0].nNonce ^= 1;
    BOOST_CHECK_THROW(proof.Verify(trusted), std::exception);
}
BOOST_AUTO_TEST_CASE(warm_certificate_cache_binds_all_verification_inputs)
{
    const auto proof = llmq::QuorumProofChain::Decode(ParseHex(TESTNET_PROOF));
    CDataStream stream(proof.seed, SER_NETWORK, PROTOCOL_VERSION);
    llmq::CFinalCommitment signer;
    stream >> signer;
    const auto cert = proof.links.front().certificate;
    const auto kind = Consensus::LLMQType::LLMQ_50_60;
    BOOST_REQUIRE(cert.Verify(signer, proof.anchor.height, kind));
    BOOST_CHECK(cert.Verify(signer, proof.anchor.height + 1, kind));
    BOOST_CHECK(!cert.Verify(signer, cert.height, kind));
    BOOST_CHECK(!cert.Verify(signer, cert.height + 1, kind));
    BOOST_CHECK(!cert.Verify(signer, 0, Consensus::LLMQType::LLMQ_400_60));

    auto changed = cert;
    changed.height++;
    BOOST_CHECK(!changed.Verify(signer, 0, kind));
    changed = cert;
    changed.header.nNonce ^= 1;
    BOOST_CHECK(!changed.Verify(signer, 0, kind));
    changed = cert;
    changed.signature = proof.target.signature; // Valid BLS point, wrong signature.
    BOOST_CHECK(!changed.Verify(signer, 0, kind));
    changed.signature.Reset();
    BOOST_CHECK(!changed.Verify(signer, 0, kind));

    auto wrong_signer = signer;
    wrong_signer.quorumHash = uint256::ONE;
    BOOST_CHECK(!cert.Verify(wrong_signer, 0, kind));
    CBLSSecretKey secret;
    secret.MakeNewKey();
    wrong_signer = signer;
    wrong_signer.quorumPublicKey = secret.GetPublicKey();
    BOOST_CHECK(!cert.Verify(wrong_signer, 0, kind));
    wrong_signer.quorumPublicKey.Reset();
    BOOST_CHECK(!cert.Verify(wrong_signer, 0, kind));
    wrong_signer = signer;
    wrong_signer.llmqType = Consensus::LLMQType::LLMQ_400_60;
    BOOST_CHECK(!cert.Verify(wrong_signer, 0, wrong_signer.llmqType));
    BOOST_CHECK(cert.Verify(signer, proof.anchor.height, kind));
}
BOOST_AUTO_TEST_CASE(warm_proof_cache_preserves_context_checks)
{
    const auto proof = llmq::QuorumProofChain::Decode(ParseHex(TESTNET_PROOF));
    const auto target = proof.Verify(proof.anchor);
    auto changed = proof;
    changed.seedPath.siblings.front() = uint256::ONE;
    BOOST_CHECK_THROW(changed.Verify(proof.anchor), std::exception);
    changed = proof;
    changed.seed.push_back(0);
    BOOST_CHECK_THROW(changed.Verify(proof.anchor), std::exception);
    changed = proof;
    changed.anchor.height = proof.links.front().certificate.height;
    BOOST_CHECK_THROW(changed.Verify(changed.anchor), std::exception);
    changed = proof;
    changed.links[1].mining.path.siblings.front() = uint256::ONE;
    BOOST_CHECK_THROW(changed.Verify(proof.anchor), std::exception);
    changed = proof;
    changed.links[1].ancestors.clear();
    BOOST_CHECK_THROW(changed.Verify(proof.anchor), std::exception);
    changed = proof;
    changed.coinbase.path.siblings.front() = uint256::ONE;
    BOOST_CHECK_THROW(changed.Verify(proof.anchor), std::exception);
    BOOST_CHECK(proof.Verify(proof.anchor) == target);
}
BOOST_AUTO_TEST_CASE(v20_checkpoint_height_is_supported)
{
    auto proof = llmq::QuorumProofChain::Decode(ParseHex(TESTNET_PROOF));
    const auto params = CreateChainParams(m_args, CBaseChainParams::TESTNET);
    proof.anchor.height = params->GetConsensus().V20Height;
    BOOST_CHECK_NO_THROW(proof.Verify(proof.anchor));
    --proof.anchor.height;
    BOOST_CHECK_THROW(proof.Verify(proof.anchor), std::runtime_error);
}
BOOST_AUTO_TEST_CASE(concurrent_proof_cache_requests)
{
    std::vector<std::future<bool>> requests;
    for (int i = 0; i < 8; ++i) {
        requests.push_back(std::async(std::launch::async, [] {
            // Each request owns its BLS objects; only the caches are shared.
            const auto proof = llmq::QuorumProofChain::Decode(ParseHex(TESTNET_PROOF));
            for (int j = 0; j < 16; ++j) {
                if (proof.Verify(proof.anchor).height != proof.target.height) return false;
                auto changed = proof;
                changed.target.header.nNonce ^= 1;
                try {
                    changed.Verify(proof.anchor);
                    return false;
                } catch (const std::exception&) {
                }
            }
            return true;
        }));
    }
    for (auto& request : requests)
        BOOST_CHECK(request.get());
}
BOOST_AUTO_TEST_CASE(cumulative_header_budget) {
    auto proof = llmq::QuorumProofChain::Decode(ParseHex(TESTNET_PROOF));
    proof.links[0].ancestors.resize(llmq::MAX_PROOF_HEADERS);
    BOOST_CHECK_THROW(proof.Encode(), std::exception);
    BOOST_CHECK_THROW(proof.Verify(proof.anchor), std::exception);
}
BOOST_AUTO_TEST_CASE(positional_merkle_shape_and_mutation) {
    std::vector<uint256> leaves{uint256::ONE, uint256S("02"), uint256S("03")};
    auto root = Hash(Hash(leaves[0], leaves[1]), Hash(leaves[2], leaves[2]));
    auto path = llmq::ProofMerklePath::Build(leaves, 2);
    BOOST_CHECK(path.Verify(leaves[2], root));
    path.count = 4;
    BOOST_CHECK(!path.Verify(leaves[2], root));
    path = llmq::ProofMerklePath::Build({leaves[0]}, 0);
    BOOST_CHECK(path.Verify(leaves[0], leaves[0]));
    path.siblings.push_back(leaves[0]);
    BOOST_CHECK(!path.Verify(leaves[0], Hash(leaves[0], leaves[0])));
    BOOST_CHECK_THROW(llmq::ProofMerklePath::Build(leaves, 3), std::exception);
}
BOOST_AUTO_TEST_SUITE_END()
