# Cosanta Core version v19.1.0

Official release:

  <https://github.com/cosanta/cosanta-core/releases/tag/v19.1.0-cosa>

Cosanta Core v19.1.0 is a backward-compatible maintenance release on the
Dash Core v19.3-based Cosanta codebase introduced in v19.0.0. It does not add
another upstream Dash release range.


# Upgrading

Back up the wallet and configuration, shut down the previous node cleanly and
wait for it to stop before replacing the binaries. No reindex or on-disk format
change is required for this update.


# Cosanta changes

## Restore upstream UTXO-set RPC behavior

Restored the upstream `gettxoutsetinfo` and `dumptxoutset` behavior, including
cancellable UTXO scans, the complete `hash_type` API and corrected snapshot and
`coins_written` reporting.

## Apply staking command-line options to wallet state

The `-stake*`, `-poshashinterval` and `-inputstakeprotect` options are applied
to wallet state so explicit command-line and configuration overrides take
effect.


# Dash v19 baseline

Cosanta Core v19.0.0 already incorporated the applicable Dash Core v19.0.0
through v19.3.0 changes. Detailed upstream release notes are archived here:

- [Dash Core v19.0.0](../dash/release-notes-19.0.0.md)
- [Dash Core v19.1.0](../dash/release-notes-19.1.0.md)
- [Dash Core v19.2.0](../dash/release-notes-19.2.0.md)
- [Dash Core v19.3.0](../dash/release-notes-19.3.0.md)


# v19.1.0 Change log

- [Cosanta v19.1.0 release](https://github.com/cosanta/cosanta-core/releases/tag/v19.1.0-cosa)
- [Changes since v19.0.1](https://github.com/cosanta/cosanta-core/compare/v19.0.1-cosa...v19.1.0-cosa)


# Older releases

- [Cosanta Core v19.0.1](https://github.com/cosanta/cosanta-core/releases/tag/v19.0.1-cosa)
