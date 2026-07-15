# Cosanta Core version v22.1.4

Official release:

  <https://github.com/cosanta/cosanta-core/releases/tag/v22.1.4-cosa>

Cosanta Core v22.1.4 is a backward-compatible Proof-of-Stake and interface
bug-fix release on top of v22.1.3. It does not introduce another upstream Dash
release range.


# Upgrading

Back up the wallet and configuration, shut down the previous node cleanly and
wait for it to stop before replacing the binaries. A reindex is not normally
required.


# Bug fixes

## Postponed Proof-of-Stake headers

Postponed Proof-of-Stake header processing is throttled to avoid repeated work,
and a possible staking deadlock during chain-state transitions is fixed.

## Masternode synchronization state

Wallet code now queries masternode synchronization and peer state through the
chain interface instead of reaching into node globals. This keeps the wallet
boundary consistent and avoids stale state access.


# v22.1.4 Change log

- [Cosanta v22.1.4 release](https://github.com/cosanta/cosanta-core/releases/tag/v22.1.4-cosa)
- [Changes since v22.1.3](https://github.com/cosanta/cosanta-core/compare/v22.1.3-cosa...v22.1.4-cosa)


# Older releases

- [Cosanta Core v22.1.3](release-notes-22.1.3.md)
