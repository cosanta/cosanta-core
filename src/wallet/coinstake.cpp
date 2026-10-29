// Copyright (c) 2024-2026 The Cosanta Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <wallet/wallet.h>

#include <chain.h>
#include <chainparams.h>
#include <consensus/amount.h>
#include <evo/dmn_types.h>
#include <interfaces/chain.h>
#include <key.h>
#include <logging.h>
#include <policy/policy.h>
#include <pos_kernel.h>
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <script/script.h>
#include <script/sign.h>
#include <script/signingprovider.h>
#include <script/standard.h>
#include <shutdown.h>
#include <util/error.h>
#include <util/moneystr.h>
#include <util/time.h>
#include <wallet/receive.h>
#include <wallet/spend.h>

#include <algorithm>
#include <cassert>
#include <map>
#include <memory>
#include <vector>

namespace wallet {

namespace {
constexpr size_t COINSTAKE_TX_OVERHEAD = 12;
constexpr size_t COINSTAKE_OUTPUT_SIZE = 36;
} // namespace

// Add auto-combine inputs to the coinstake under construction. Split out of
// CreateCoinStake() so the candidate selection is unit-testable.
void CWallet::AutocombineCoinStake(const COutPoint& kernel_prevout, const CPubKey& kernel_pubkey,
                                   CAmount target_amount, size_t max_tx_size, size_t max_sigops,
                                   CMutableTransaction& stakeTx, std::vector<CScript>& vin_scripts,
                                   CAmount& reward, size_t& est_tx_size, size_t& est_tx_sigops)
{
    if (fAutocombine == AUTOCOMBINE_DISABLE) {
        return;
    }

    const CAmount split_threshold = nStakeSplitThreshold * COIN;
    const CAmount autocombine_target = split_threshold * 2 - 1;
    const CAmount combine_max = nStakeCombineMax * COIN;

    std::vector<COutPoint> ac_candidates;
    // find candidates
    {
        std::vector<CompactTallyItem> vecTallyRet = SelectCoinsGroupedByAddresses();
        if (fAutocombine == AUTOCOMBINE_SAME) {
            // Autocombine same
            CTxDestination reqdest{PKHash(kernel_pubkey)};
            for (auto titer = vecTallyRet.begin(); titer != vecTallyRet.end(); ++titer) {
                if (titer->txdest == reqdest) {
                    ac_candidates.swap(titer->outpoints);
                    break;
                }
            }
        } else if (fAutocombine == AUTOCOMBINE_ANY) {
            for (auto titer = vecTallyRet.begin(); titer != vecTallyRet.end(); ++titer) {
                ac_candidates.insert(ac_candidates.end(), titer->outpoints.begin(), titer->outpoints.end());
            }
        }
    }

    // Automatically combine
    std::map<COutPoint, Coin> ac_coins;
    auto ac_verified_end = ac_candidates.begin();
    auto ac_iter = ac_candidates.begin();
    auto min_age = Params().MinStakeAge();
    auto ac_len = ac_candidates.size();
    int inputs_included = 0;
    for (size_t i = 0; (i < ac_len) && (inputs_included < nStakeMaxSplit); ++i) {
        CTxOut ac_in;
        CAmount ac_amt = 0;
        int64_t ac_time = 0;
        for (; ac_iter != ac_candidates.end(); ++ac_iter) {
            if (*ac_iter == kernel_prevout) {
                continue;
            }
            {
                LOCK(cs_wallet);
                const CWalletTx* wtx = GetWalletTx(ac_iter->hash);
                if (wtx == nullptr || ac_iter->n >= wtx->tx->vout.size()) {
                    continue;
                }
                // The coinstake sits at vtx[1] before mempool transactions,
                // so unconfirmed parents are not available to it; coinbase
                // and coinstake outputs must also be mature
                if (GetTxDepthInMainChain(*wtx) <= 0 || GetTxBlocksToMaturity(*wtx) > 0) {
                    continue;
                }

                ac_in = wtx->tx->vout[ac_iter->n];
                ac_time = wtx->GetTxTime();
            }

            ac_amt = ac_in.nValue;

            // Do not touch collaterals
            if (inputStakeProtect && dmn_types::IsCollateralAmount(ac_amt)) {
                continue;
            }

            int64_t nTimeInput = ac_time + static_cast<int64_t>(min_age);
            int64_t nCurrentTime = GetTime();
            if (nTimeInput > nCurrentTime) {
                continue;
            }
            // Respect -reservebalance
            if ((reward + ac_amt) > target_amount) {
                continue;
            }
            if (ac_amt < MIN_STAKE_AMOUNT) {
                break;
            }
            if ((reward + ac_amt) < autocombine_target) {
                break;
            }
            // Sweep small inputs (up to -stakecombinemax); the split loop normalizes the output
            if (combine_max > 0 && reward >= split_threshold && ac_amt <= combine_max) {
                break;
            }
        }

        if (ac_iter == ac_candidates.end()) {
            break;
        }

        // Verify against the UTXO set in bounded batches ahead of inclusion,
        // like the kernel input; only the current window of results is kept
        if (ac_iter >= ac_verified_end) {
            constexpr std::ptrdiff_t FINDCOINS_BATCH = 64;
            ac_verified_end = (ac_candidates.end() - ac_iter) > FINDCOINS_BATCH
                                  ? ac_iter + FINDCOINS_BATCH
                                  : ac_candidates.end();
            ac_coins.clear();
            for (auto it = ac_iter; it != ac_verified_end; ++it) {
                ac_coins.emplace(*it, Coin{});
            }
            chain().findCoins(ac_coins);
        }
        if (ac_coins[*ac_iter].IsSpent()) {
            ++ac_iter;
            continue;
        }

        // Real signed size of this input; unsolvable inputs are skipped
        const int ac_in_size = CalculateMaximumSignedInputSize(ac_in, this);
        if (ac_in_size < 0) {
            ++ac_iter;
            continue;
        }
        // A P2SH input brings its redeem script sigops (GetP2SHSigOpCount() in ConnectBlock())
        size_t ac_in_sigops = 0;
        {
            std::vector<std::vector<unsigned char>> ac_solutions;
            if (Solver(ac_in.scriptPubKey, ac_solutions) == TxoutType::SCRIPTHASH) {
                CScript redeem_script;
                const std::unique_ptr<SigningProvider> ac_provider = GetSolvingProvider(ac_in.scriptPubKey);
                if (!ac_provider || !ac_provider->GetCScript(CScriptID(uint160(ac_solutions[0])), redeem_script)) {
                    ++ac_iter;
                    continue;
                }
                ac_in_sigops = redeem_script.GetSigOpCount(true);
            }
        }

        // This input does not fit (+ its potential split output), a smaller one still may
        if (est_tx_size + static_cast<size_t>(ac_in_size) + COINSTAKE_OUTPUT_SIZE > max_tx_size ||
            est_tx_sigops + ac_in_sigops + 1 > max_sigops) {
            ++ac_iter;
            continue;
        }

        inputs_included++;
        stakeTx.vin.emplace_back(*ac_iter);
        vin_scripts.emplace_back(ac_in.scriptPubKey);
        est_tx_size += static_cast<size_t>(ac_in_size);
        est_tx_sigops += ac_in_sigops;
        reward += ac_amt;
        LogPrint(BCLog::STAKING, "%s : auto-combining tx=%s n=%u amount=%llu total=%llu\n", __func__,
                 ac_iter->hash.ToString(), ac_iter->n, ac_amt, reward);
        ++ac_iter;
    }
}

// ppcoin: create coin stake transaction
bool CWallet::CreateCoinStake(const CBlockIndex* pindex_prev, CBlock& curr_block, CMutableTransaction& coinbaseTx, size_t max_tx_size, size_t max_sigops)
{
    // Choose coins to use
    const CAmount nBalance = GetBalance(*this).m_mine_trusted;

    if (m_args.IsArgSet("-reservebalance")) {
        const auto parsed = ParseMoney(m_args.GetArg("-reservebalance", ""));
        if (!parsed) return error("CreateCoinStake : invalid reserve balance amount");
        nReserveBalance = *parsed;
    }

    if (nBalance <= nReserveBalance) {
        return error("CreateCoinStake : balance is less than required to reserve");
    }

    // presstab HyperStake - Initialize as static and don't update the set on every run of CreateCoinStake() in order to lighten resource use
    const CAmount nTargetAmount = nBalance - nReserveBalance;

    if (setStakeCoins.empty() || GetTime() - nLastStakeSetUpdate > nStakeSetUpdateTime) {
        setStakeCoins.clear();

        if (!SelectStakeCoins(setStakeCoins, nTargetAmount)) {
            LogPrint(BCLog::STAKING, "%s : no inputs eligible for staking\n", __func__);
            return false;
        }
        std::sort(setStakeCoins.begin(), setStakeCoins.end(), [](const auto& lhs, const auto& rhs) {
            return std::get<0>(lhs) > std::get<0>(rhs);
        });

        nLastStakeSetUpdate = GetTime();
    }

    LogPrint(BCLog::STAKING, "%s : found %u possible stake inputs\n", __func__, static_cast<unsigned int>(setStakeCoins.size()));

    // NOTE: go from smaller amounts to bigger to increase chance, unlike it was before
    for (auto iter = setStakeCoins.begin(); iter != setStakeCoins.end(); ++iter) {
        if (ShutdownRequested()) {
            break;
        }

        const CBlockIndex* pcoin_index = nullptr;
        const auto* pWalletTxIn = std::get<1>(*iter);

        COutPoint prevoutStake = COutPoint(pWalletTxIn->GetHash(), std::get<2>(*iter));
        LogPrint(BCLog::STAKING, "%s : trying tx=%s n=%u\n", __func__,
                 prevoutStake.hash.ToString(), prevoutStake.n);

        std::map<COutPoint, Coin> coins{{prevoutStake, Coin{}}};
        chain().findCoins(coins);
        if (coins[prevoutStake].IsSpent()) {
            LogPrint(BCLog::STAKING, "%s : skipping stale stake input tx=%s n=%u (already spent)\n",
                     __func__, prevoutStake.hash.ToString(), prevoutStake.n);
            continue;
        }

        const auto* confirmed = pWalletTxIn->state<TxStateConfirmed>();
        if (!confirmed) {
            continue;
        }
        if (confirmed->confirmed_block_height <= pindex_prev->nHeight) {
            pcoin_index = pindex_prev->GetAncestor(confirmed->confirmed_block_height);
        }
        if (!pcoin_index || pcoin_index->GetBlockHash() != confirmed->confirmed_block_hash) {
            LogPrintf("%s : failed to find block index for %s \n",
                      __func__, confirmed->confirmed_block_hash.ToString());
            continue;
        }

        uint256 hashProofOfStake;

        // iterates each utxo inside of CheckStakeKernelHash()
        bool fKernelFound = CheckStakeKernelHash(
                curr_block,
                *pindex_prev,
                *pcoin_index,
                *pWalletTxIn->tx,
                prevoutStake,
                nHashDrift,
                false,
                hashProofOfStake,
                true);

        if (fKernelFound) {
            // Double check that this will pass time requirements
            const bool too_far_in_past = curr_block.nTime <= pindex_prev->GetMedianTimePast();
            if (too_far_in_past) {
                LogPrint(BCLog::STAKING, "%s kernel found, but it is too far in the past \n", __func__);
                continue;
            }

            // Found a kernel
            LogPrint(BCLog::STAKING, "%s  : kernel found\n", __func__);

            std::vector<std::vector<unsigned char>> vSolutions;
            CScript scriptPubKeyOut;
            const auto& tx_in = pWalletTxIn->tx->vout[prevoutStake.n];
            const auto& scriptPubKeyKernel = tx_in.scriptPubKey;

            const TxoutType whichType = Solver(scriptPubKeyKernel, vSolutions);

            if (whichType == TxoutType::NONSTANDARD) {
                LogPrint(BCLog::STAKING, "%s : failed to parse kernel\n", __func__);
                continue;
            }

            LogPrint(BCLog::STAKING, "%s  : parsed kernel type=%d\n", __func__, static_cast<int>(whichType));

            if (whichType == TxoutType::PUBKEYHASH) {
                // convert to pay to address type
                std::unique_ptr<SigningProvider> provider = GetSolvingProvider(scriptPubKeyKernel);
                if (!provider || !provider->GetPubKey(CKeyID(uint160(vSolutions[0])), curr_block.posPubKey)) {
                    LogPrint(BCLog::STAKING, "%s : failed to get key for kernel type=%d\n", __func__, static_cast<int>(whichType));
                    continue;
                }
            } else if (whichType == TxoutType::PUBKEY) {
                curr_block.posPubKey = CPubKey(vSolutions[0]);
            } else {
                LogPrint(BCLog::STAKING, "%s : no support for kernel type=%d\n", __func__, static_cast<int>(whichType));
                continue;
            }

            assert(curr_block.posPubKey.IsValid());
            scriptPubKeyOut = GetScriptForDestination(PKHash(curr_block.posPubKey));

            // Require the same miner's output in CoinBase
            coinbaseTx.vout[0].scriptPubKey = scriptPubKeyOut;

            CMutableTransaction stakeTx;
            stakeTx.vin.emplace_back(prevoutStake);
            // Mark coin stake transaction
            CScript scriptEmpty;
            scriptEmpty.clear();
            stakeTx.vout.push_back(CTxOut(0, scriptEmpty));
            stakeTx.vout.emplace_back(tx_in.nValue, scriptPubKeyOut);

            CAmount reward = stakeTx.vout[1].nValue;
            const CAmount split_threshold = nStakeSplitThreshold * COIN;

            // Upper-bound size accounting for the signed coinstake
            const size_t max_coinstake_size = std::min<size_t>(max_tx_size, MAX_STANDARD_TX_SIZE);

            const int kernel_input_size = CalculateMaximumSignedInputSize(tx_in, this);
            if (kernel_input_size < 0) {
                LogPrint(BCLog::STAKING, "%s : kernel input is not solvable\n", __func__);
                continue;
            }
            size_t est_tx_size = COINSTAKE_TX_OVERHEAD
                + static_cast<size_t>(kernel_input_size)
                + stakeTx.vout.size() * COINSTAKE_OUTPUT_SIZE;
            // vout[1] is P2PKH (1 sigop); the P2PK(H) kernel scriptSig adds none
            size_t est_tx_sigops = 1;
            if (est_tx_size > max_coinstake_size || est_tx_sigops > max_sigops) {
                LogPrint(BCLog::STAKING, "%s : no room for coinstake in the block\n", __func__);
                continue;
            }

            std::vector<CScript> vin_scripts;
            vin_scripts.emplace_back(scriptPubKeyKernel);

            AutocombineCoinStake(prevoutStake, curr_block.posPubKey, nTargetAmount,
                                 max_coinstake_size, max_sigops, stakeTx, vin_scripts,
                                 reward, est_tx_size, est_tx_sigops);

            // Automatically split
            for (int i = 0; (i < nStakeMaxSplit) && (reward > split_threshold * 2) &&
                            (est_tx_size + COINSTAKE_OUTPUT_SIZE <= max_coinstake_size) &&
                            (est_tx_sigops + 1 <= max_sigops); ++i) {
                stakeTx.vout.emplace_back(split_threshold, scriptPubKeyOut);
                est_tx_size += COINSTAKE_OUTPUT_SIZE;
                est_tx_sigops += 1;
                reward -= split_threshold;
            }

            stakeTx.vout[1].nValue = reward;

            LogPrint(BCLog::STAKING, "%s : split stake vout into %u pieces\n", __func__,
                     static_cast<unsigned int>(stakeTx.vout.size()));

            {
                // Keep the wallet/key-store lock order consistent with wallet UI
                // paths (cs_wallet -> cs_KeyStore). SignSignature() retrieves the
                // private key and therefore locks cs_KeyStore.
                LOCK(cs_wallet);
                for (size_t i = 0; i < vin_scripts.size(); ++i) {
                    if (!SignSignature(*GetLegacyScriptPubKeyMan(), vin_scripts[i], stakeTx, i, reward, SIGHASH_ALL)) {
                        return error("CreateCoinStake : failed to sign coinstake");
                    }
                }
            }

            curr_block.posStakeHash = prevoutStake.hash;
            curr_block.posStakeN = prevoutStake.n;
            curr_block.Stake() = MakeTransactionRef(std::move(stakeTx));

            LogPrint(BCLog::STAKING, "%s : added kernel type=%d\n", __func__, static_cast<int>(whichType));

            // Force update
            nLastStakeSetUpdate = 0;
            return true;
        }
    }

    LogPrint(BCLog::STAKING, "%s : no stakes found\n", __func__);

    return false;
}

} // namespace wallet
