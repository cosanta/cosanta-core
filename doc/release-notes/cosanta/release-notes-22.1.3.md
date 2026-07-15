# Cosanta Core version v22.1.3

Official release:

  <https://github.com/cosanta/cosanta-core/releases/tag/v22.1.3-cosa>

Cosanta Core v22.1.3 is a mandatory network upgrade from the Dash Core v21
line to Dash Core v22.1.3. Applicable changes from Dash Core v22.0.0 through
v22.1.3 are summarized below and adapted to Cosanta consensus and network
parameters.

Cosanta Core v22.1.3 includes the changes introduced across the v22.0.x and v22.1.x release series, together with additional stability, performance, networking, and build improvements.

## Highlights

- Added support for Asset Unlock transactions and updated withdrawal validation rules.
- Enabled encrypted BIP324/v2 P2P connections by default, with automatic fallback to v1 peers.
- Improved network diversity, onion connectivity, peer selection, and resistance to eclipse and partition attacks.
- Improved deterministic masternode list and quorum rotation performance.
- Added persistent UTXO locking in both RPC and the graphical interface.
- Improved responsiveness for large wallets.
- Updated build tooling, platform compatibility, and release packaging.

## Consensus and Platform withdrawals

- Added the `withdrawals` deployment and its updated consensus validation rules.
- Asset Unlock validation can use all 24 active quorums and the most recent inactive quorum.
- Increased the withdrawal limit to 2,000 COSA per 576 blocks.
- The current Cosanta mainnet schedule uses block 975,744 for v19, 991,872
  for v20, 1,013,576 for MN_RR and 1,030,192 for withdrawals.
- Updated protocol requirements for nodes and masternodes.

## P2P and networking

- Enabled BIP324/v2 encrypted P2P transport by default. Connections automatically fall back to v1 when the remote peer does not support v2.
- Fixed v2-to-v1 downgrade handling for mixing and masternode probe connections, reducing unnecessary connections and masternode load.
- Nodes with onion connectivity now maintain and protect at least two outbound onion connections.
- Multi-network nodes attempt to maintain at least one outbound connection on every reachable network.
- DSQ messages are relayed through the inventory system to reduce bandwidth usage.
- Increased the compressed block-header request limit from 2,000 to 8,000.
- Ports below 1024 and other commonly authenticated service ports are avoided as peer service ports.
- Updated the `isdlock` `cycleHash` field to identify the signing quorum's DKG cycle start block.

## Quorums and masternodes

- Optimized `quorum rotationinfo` and `GETQUORUMROTATIONINFO` by building diffs progressively from oldest to newest.
- Corrected `baseBlockHash` handling in quorum rotation responses.
- Improved deterministic masternode list processing and the performance of RPCs such as `protx diff`.

## Wallet and CoinJoin

- Added the `coinjoinsalt` RPC with `get`, `set`, and `generate` operations.
- Persistent locks are now supported by `lockunspent` and survive node restarts.
- UTXOs locked through the graphical interface are stored persistently.
- Improved graphical interface responsiveness for large wallets.
- Fixed a potential CoinJoin test deadlock caused by wallet transaction scans under the wallet lock.
- Fixed CoinJoin balance disclosure in discrete mode.

## RPC changes

- Added `getislocks` to retrieve InstantSend lock data in JSON and binary hexadecimal formats.
- Added a binary `hex` field to `getbestchainlock`.
- Added descriptor-wallet support to `governance votemany` and `governance votealias`.
- `createwallet` now requires `load_on_startup` to be set explicitly for descriptor wallets.
- Renamed the `getblockfrompeer` named argument from `block_hash` to `blockhash`.
- Removed deprecated `protx *_hpmn` aliases in favor of `protx *_evo`.
- Updated `quorum dkgsimerror` to accept an integer rate from 0 to 100.
- `coinjoin stop` now reports an error when no mixing session is active.
- Updated transaction JSON output so `creditOutputs` entries are objects rather than strings.
- Deprecated top-level mempool fee fields in favor of the `fees` object.

## GUI and configuration

- Added an option to enable RPC server functionality from the graphical interface.
- Added dark-mode appearance support on macOS.
- Extended `-walletnotify` with `%h` for block height and `%b` for block hash.
- Added human-readable units to `-maxuploadtarget`.
- Added StatsD batching and queueing controls.

## Build, compatibility, and tests

- The minimum supported glibc version is 2.31. Ubuntu 18.04 and RHEL 8 are no longer supported.
- Improved FreeBSD build support.
- Updated macOS packaging and notarization support.
- Pinned the QEMU version used by container builds to avoid segmentation faults.
- Improved regtest and devnet activation controls through `-testactivationheight`.
- Fixed debug-build assertions for coinbase transactions in simplified masternode list diffs.
- Improved CoinJoin test stability and deterministic coverage.

## Cosanta integration

- Preserved Cosanta Proof-of-Stake, scrypt hashing, staking, reward and
  network behavior on top of the Dash Core v22.1.3 codebase.
- Cosanta masternodes do not require an auxiliary messenger daemon or its RPC
  configuration.
- Updated seeds, build tooling, release assets, documentation, Qt branding and
  functional tests for the Cosanta release.

## Upgrade notes

Shut down the previous version completely before replacing the binaries. Downgrading to a version older than v22.0.0 may require a full reindex.

### Apple Silicon

If macOS marks the freshly downloaded build as quarantined, clear the attribute before launching:

`xattr -dr com.apple.quarantine /Applications/Cosanta-Qt.app`

# v22.1.3 Change log

- [Cosanta v22.1.3 release](https://github.com/cosanta/cosanta-core/releases/tag/v22.1.3-cosa)
- [Changes since v21.1.2](https://github.com/cosanta/cosanta-core/compare/v21.1.2-cosa...v22.1.3-cosa)

Detailed upstream release notes:

- [Dash Core v22.0.0](../dash/release-notes-22.0.0.md)
- [Dash Core v22.1.0](../dash/release-notes-22.1.0.md)
- [Dash Core v22.1.1](../dash/release-notes-22.1.1.md)
- [Dash Core v22.1.2](../dash/release-notes-22.1.2.md)
- [Dash Core v22.1.3](../dash/release-notes-22.1.3.md)

# Credits

Thanks to Cosanta contributors and to the Dash Core and Bitcoin Core
developers whose upstream work was integrated into this release.

# Older releases

- [Cosanta Core v21.1.2](release-notes-21.1.2.md)
