# Cosanta Core version v21.1.2

Official release:

  <https://github.com/cosanta/cosanta-core/releases/tag/v21.1.2-cosa>

Cosanta Core v21.1.2 is a backward-compatible maintenance release on the v21
codebase. It does not introduce another upstream Dash release range.


# Upgrading

Back up the wallet and configuration, shut down the previous node cleanly and
wait for it to stop before replacing the binaries. No reindex or on-disk format
change is required.


# Changes

## Hide coinstake rows in transaction history

Coinstake transactions are no longer displayed as ordinary wallet transfers in
the Qt transaction history. This avoids misleading entries while preserving
their normal staking and accounting behavior.

## Release infrastructure

The Guix build became an explicitly requested CI job, and the release-branch
fast-forward merge check was corrected.


# v21.1.2 Change log

- [Cosanta v21.1.2 release](https://github.com/cosanta/cosanta-core/releases/tag/v21.1.2-cosa)
- [Changes since v21.1.1](https://github.com/cosanta/cosanta-core/compare/v21.1.1-cosa...v21.1.2-cosa)


# Older releases

- [Cosanta Core v21.1.1](release-notes-21.1.1.md)
