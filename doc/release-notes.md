# Cosanta Core version v23.1.8

This is a new patch version release, fixing four remotely reachable crashes and
bringing further hardening of the peer-to-peer message handlers along with
Proof-of-Stake, wallet, networking, RPC and build fixes.
Upgrading is **strongly recommended** for all nodes, and required for
masternodes.

Please report bugs using the issue tracker at GitHub:

  <https://github.com/cosanta/cosanta-core/issues>

# Upgrading and downgrading

## How to Upgrade

If you are running an older version, shut it down. Wait until it has completely
shut down (which might take a few minutes for older versions), then run the
installer (on Windows) or just copy over /Applications/Cosanta-Qt (on Mac) or
cosantad/cosanta-qt (on Linux).

## Downgrade warning

### Downgrade to a version < v23.0.0

Downgrading to a version older than v23.0.0 is not supported, and will
require a reindex.

# Release Notes

## Critical fixes

This release fixes four crashes that a remote party could trigger. None of
them affect consensus rules or put funds at risk, but each one can take a node
offline, so all operators should upgrade promptly.

- Fixed a crash while removing provider transactions that a masternode's
  operator-key change invalidates. Those transactions are collected before any
  of them are removed, so when one was an in-mempool descendant of another it
  was already erased along with its ancestor, and the stale entry was then
  dereferenced. Such entries are now skipped. This is reachable whenever a block
  carries a provider registrar update or revocation for a masternode that has
  chained service updates pending in the mempool.
- Fixed a crash caused by an unvalidated LLMQ type in a `qsigshare` message. A
  masternode that received a signature share naming an LLMQ type its chain does
  not register would index a per-type quorum cache that is only populated for
  known types, aborting the process. Unregistered types are now rejected before
  the lookup, and the affected cache lookups no longer create missing entries.
- Fixed a crash caused by a quorum commitment naming a block with no parent,
  such as the genesis block. The parentless block index reached a non-null
  precondition and terminated the process instead of failing validation, which
  no exception handler could contain. Commitments with a parentless quorum base
  block are now rejected, and the LLMQ activation check treats a null
  predecessor as "not enabled" rather than a contract violation.
- Fixed a crash during Proof-of-Stake header validation when a crafted header
  referenced a real staking transaction but supplied an out-of-range output
  index. The peer-controlled index is now checked before transaction outputs
  are accessed, and malformed headers are rejected.

## Security

This release continues the hardening of peer-to-peer message handlers against
denial-of-service from remote peers. These issues do not affect consensus and do
not put funds at risk, but they could be used to crash or degrade nodes -
masternodes in particular - so upgrading is recommended.

- LLMQ / signing: the queues of not-yet-verified recovered signatures and
  signature shares are now bounded, and the vectors carried by the QSIGSHARE,
  QSIGSESANN, QSIGSHARESINV, QGETSIGSHARES and QBSIGSHARES messages are bounded
  before any allocation or decoding takes place. The number of signing share
  sessions a single peer may announce is also capped, so a peer can no longer
  grow that per-peer state without limit (dash#7351).
- LLMQ / DKG: the number of encrypted contribution blobs in a DKG contribution
  is now checked against the quorum's lower bound as well as its upper bound.
- LLMQ / quorum data: the verification vector and encrypted contribution
  vectors in QDATA responses are validated against their expected sizes before
  any BLS decoding is performed.
- Transaction relay: an oversized `notfound` message is now penalised rather
  than silently ignored (dash#7348).
- ChainLocks: the cache of seen ChainLock signatures is now bounded.
- Governance: per-object vote sync requests are now throttled per peer, and
  governance object and vote responses are only accepted from a peer if that
  peer announced them or they were requested from it, using the net-layer
  per-peer request tracker. Governance vote signatures are bounded when read
  from the network and must use one of the two legitimate encodings.
- CoinJoin: the vectors carried by CoinJoin mixing messages are bounded before
  allocation, and a non-participant can no longer abort another session's
  signing phase. An invalid `dstx` message now carries a misbehaviour score
  instead of being dropped for free (dash#7347).
- Bloom filters: filterload and filteradd payloads are bounded before
  allocation.
- Sporks: spork signatures are bounded during deserialization, and malformed
  spork messages now attribute misbehaviour to the sending peer.
- Compact block relay: batched hardening backported from upstream Bitcoin Core
  (dash#7398), including detection of mutated blocks as a defence-in-depth
  measure.

## Proof of Stake and Wallet

- Header-only fork validation now advances through every unvalidated
  Proof-of-Stake header when checking for reuse of a stake UTXO. The previous
  frozen walk could miss reuse in intermediate headers; the corrected walk also
  stops safely at genesis.
- Stake input auto-combining now sweeps up to `-stakecombinemax` inputs (100 by
  default) into kernels that have already reached the split threshold, so small
  staking rewards do not accumulate indefinitely. Candidate selection respects
  `-reservebalance` and `-inputstakeprotect`, requires confirmed and mature
  UTXOs, and observes the block's remaining size and signature-operation
  budgets.
- Fixed a lock-order inversion while signing Proof-of-Stake blocks with an
  encrypted wallet that could deadlock staking after prolonged operation.

## RPC

- `protx listdiff` no longer reports an always-zero `platformP2PPort` /
  `platformHTTPPort` for masternodes registered with extended addresses; the
  live Platform ports are reported instead.

## GUI

- The PoSe score column is no longer hidden together with banned masternodes in
  the masternode list.
- Fixed an abort when scaling widgets whose font was set in pixels rather than
  points (for example by a stylesheet's `font-size: Npx`); such fonts are now
  converted to a point size instead of being assumed to have one (dash#7465).
- Introduced a framework for sourcing and applying data with dedicated feeds, used by the Masternode and Proposal list views for improved data flow and separation of concerns (dash#7146).
- Added a new "Proposal Information" widget to the Information tab with an interactive donut chart showing proposal budget allocation (dash#7159).
- Added distinct widgets for Cosanta-specific reporting in the Debug window, including dedicated Information and Network tabs (dash#7118).
- Added support for reporting `OP_RETURN` payloads as Data Transactions in the transaction list (dash#7144).
- Added Tahoe styled icons for macOS with runtime styling for each network type (mainnet, testnet, devnet, regtest), updated bundle icon, and added mask-based tray icon with generation scripts (dash#7180).
- Filter preferences in the masternode list are now persisted across sessions (dash#7148).
- Fixed overview page font double scaling, recalculated minimum width correctly, fixed `SERVICE` and `STATUS` column sorting, and fixed common types filtering in masternode list (dash#7147).
- Fixed `labelError` styling by moving it from `proposalcreate.ui` into `general.css` for consistency (dash#7145).
- Fixed banned masternodes incorrectly returning status=0 instead of their actual ban status (dash#7157).

## Bug Fixes

- Fixed MN update notifications where the old and new masternode lists were swapped, causing incorrect change detection (dash#7154).
- Reject identity elements in BLS deserialization and key generation to prevent invalid keys from being accepted (dash#7193).
- Fixed quorum labels not being correctly reseated when new quorum types are inserted (dash#7191).
- Skip collecting block txids during IBD to prevent unbounded memory growth in `ChainLockSigner` (dash#7208).
- Serialize `TrySignChainTip` to prevent concurrent signing races that could split signing shares across different block hashes (dash#7209).
- Properly skip evodb repair when reindexing to prevent unnecessary repair attempts (dash#7222).
- Parallel block fetching during initial block download now respects each
  peer's advertised starting height and does not request blocks the peer has not
  claimed to possess.

## Build and CI

- Fixed a CMake compatibility error when building the freetype dependency with
  newer CMake (dash#7372).
- Stabilized the `-par` / `-parbls` help text (and the generated man pages) so
  they no longer embed the core count of the build machine.
- Updated GitHub Actions pins for the Node 24 runtime.
- Fixed the circular-dependencies lint script under Python 3.15.

## Tests

- Governance inventory cache coverage moved from a functional test to unit
  tests, and governance vote test fixtures are now wire-valid.

## Miscellaneous

- Renamed `bitcoin-util` manpage and test references to `cosanta-util` (dash#7221).

## Interfaces

- Consolidated masternode counts into a single struct and exposed chainlock, InstantSend, credit pool, and quorum statistics through the node interface (dash#7160).

## Performance Improvements

- Replaced two heavy `HashMap` constructions with linear lookups in hot paths where the maps were rarely used, reducing overhead (dash#7176).

# v23.1.8 Change log

See detailed [set of changes][set-of-changes].

# Credits

Thanks to everyone who directly contributed to this release:

- Dmitriy Korniychuk
- Konstantin Akimov
- PastaClaw
- PastaPastaPasta
- UdjinM6

As well as everyone that submitted issues, reviewed pull requests and helped
debug the release candidates.

# Older releases

These releases are considered obsolete. Old release notes can be found here:

- [v23.1.7](https://github.com/cosanta/cosanta-core/blob/master/doc/release-notes/dash/release-notes-23.1.7.md) released Jun/30/2026
- [v23.1.5](https://github.com/cosanta/cosanta-core/blob/master/doc/release-notes/dash/release-notes-23.1.5.md) released Jun/19/2026
- [v23.1.4](https://github.com/cosanta/cosanta-core/blob/master/doc/release-notes/dash/release-notes-23.1.4.md) released Jun/18/2026
- [v23.1.3](https://github.com/cosanta/cosanta-core/blob/master/doc/release-notes/dash/release-notes-23.1.3.md) released May/28/2026
- [v23.1.2](https://github.com/cosanta/cosanta-core/blob/master/doc/release-notes/dash/release-notes-23.1.2.md) released Mar/12/2026
- [v23.1.0](https://github.com/cosanta/cosanta-core/blob/master/doc/release-notes/dash/release-notes-23.1.0.md) released Feb/15/2026
- [v23.0.2](https://github.com/cosanta/cosanta-core/blob/master/doc/release-notes/dash/release-notes-23.0.2.md) released Dec/4/2025
- [v23.0.0](https://github.com/cosanta/cosanta-core/blob/master/doc/release-notes/dash/release-notes-23.0.0.md) released Nov/10/2025
- [v22.1.3](https://github.com/cosanta/cosanta-core/blob/master/doc/release-notes/dash/release-notes-22.1.3.md) released Jul/15/2025
- [v22.1.2](https://github.com/cosanta/cosanta-core/blob/master/doc/release-notes/dash/release-notes-22.1.2.md) released Apr/15/2025
- [v22.1.1](https://github.com/cosanta/cosanta-core/blob/master/doc/release-notes/dash/release-notes-22.1.1.md) released Feb/17/2025
- [v22.1.0](https://github.com/cosanta/cosanta-core/blob/master/doc/release-notes/dash/release-notes-22.1.0.md) released Feb/10/2025
- [v22.0.0](https://github.com/cosanta/cosanta-core/blob/master/doc/release-notes/dash/release-notes-22.0.0.md) released Dec/12/2024
- [v21.1.1](https://github.com/cosanta/cosanta-core/blob/master/doc/release-notes/dash/release-notes-21.1.1.md) released Oct/22/2024
- [v21.1.0](https://github.com/cosanta/cosanta-core/blob/master/doc/release-notes/dash/release-notes-21.1.0.md) released Aug/8/2024
- [v21.0.2](https://github.com/cosanta/cosanta-core/blob/master/doc/release-notes/dash/release-notes-21.0.2.md) released Aug/1/2024
- [v21.0.0](https://github.com/cosanta/cosanta-core/blob/master/doc/release-notes/dash/release-notes-21.0.0.md) released Jul/25/2024
- [v20.1.1](https://github.com/cosanta/cosanta-core/blob/master/doc/release-notes/dash/release-notes-20.1.1.md) released April/3/2024

[set-of-changes]: https://github.com/cosanta/cosanta-core/compare/v23.1.7...v23.1.8
