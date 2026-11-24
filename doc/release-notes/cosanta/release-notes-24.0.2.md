# Cosanta Core v24.0.2

Cosanta Core v24.0.2 brings the fixes from Dash Core v24.0.3 to Cosanta and
introduces the black-and-red Cosanta theme as the default for new Qt wallet
profiles. [Dash Core v24.0.3 release notes](https://github.com/dashpay/dash/blob/v24.0.3/doc/release-notes.md)

This release includes the proof-of-stake header synchronization fixes and the
PosV3 upgrade introduced in Cosanta Core v24.0.1. The PosV3 activation heights
are **1,078,675 on mainnet** and **888,547 on testnet**.
[Cosanta Core v24.0.1 release notes](release-notes-24.0.1.md)

Please report issues in the [Cosanta Core issue tracker](https://github.com/cosanta/cosanta-core/issues).

## Upgrading and downgrading

Shut down the old version and wait for it to exit. Run the installer on Windows,
replace `Cosanta-Qt.app` in `/Applications` on macOS, or replace `cosantad` and
`cosanta-qt` on Linux.

Upgrading from Cosanta Core v24.0.1 requires no additional migration. If you are
upgrading from v23.1.x or earlier, review the first-start index migration and
configuration changes in the [Cosanta Core v24.0.0 upgrade notes](release-notes-24.0.0.md#how-to-upgrade).

Cosanta Core v24.0.1 supports PosV3. Use a PosV3-capable version once the upgrade
activates on your network. For the separate `v24` network upgrade and the
datadir implications of downgrading to pre-v24 releases, see the
[Cosanta Core v24.0.0 downgrade warning](release-notes-24.0.0.md#downgrade-warning).

## Cosanta Qt wallet

- The new **Cosanta** theme is the default for new profiles and profiles
  without a valid saved theme. Existing Light, Dark, and Traditional choices
  are preserved. The theme can be selected in Appearance settings.
- The Tools window has distinct tab icons and a visible active tab. RPC console
  warnings, the incoming and outgoing traffic curves and legend, and wallet
  status icons use the active theme's colors.
- On macOS, rounded popup menus retain their shadow and have transparent corners.

[Cosanta Core pull request #6](https://github.com/cosanta/cosanta-core/pull/6)

## Fixes from Dash Core v24.0.3

Pull request numbers in this section refer to the upstream Dash repository.

### Governance

- The Qt proposal editor bases the payment window on the selected superblocks.
  It hides superblocks whose voting deadline has passed, preventing payments
  from being scheduled for an unintended superblock or missed entirely.
  [Dash #8154](https://github.com/dashpay/dash/pull/8154)
- Improved the handling of cached governance votes received before their
  proposal or other governance object.
  [Dash #8122](https://github.com/dashpay/dash/pull/8122)

### Reindex and indexes

- A completed reindex now flushes all databases before reporting completion.
  This prevents a second reindex after an unclean shutdown immediately afterward.
  A node already unable to start for this reason needs one more reindex.
  [Dash #8103](https://github.com/dashpay/dash/pull/8103)
- Fixed startup after an unclean shutdown with `-addressindex`, `-spentindex`,
  or `-timestampindex` enabled.
  [Dash #8047](https://github.com/dashpay/dash/pull/8047)

### Wallet

- BIP39 validation now checks the entire mnemonic checksum. Wallet creation
  and restoration reject phrases with an invalid checksum. Existing wallet
  files remain usable.
  [Dash #8111](https://github.com/dashpay/dash/pull/8111)
- Fixed a race that could cause a rescan with `-blockfilterindex` to miss
  transactions when wallet descriptors changed during the scan.
  [Dash #8139](https://github.com/dashpay/dash/pull/8139)

### CoinJoin

- Masternodes verify participants' signatures against the final mixing
  transaction. [Dash #7815](https://github.com/dashpay/dash/pull/7815)
- When choosing whose collateral to charge, the masternode counts each
  participant with unsigned inputs once.
  [Dash #8116](https://github.com/dashpay/dash/pull/8116)
- Mixing clients reject queue announcements with an invalid denomination.
  [Dash #7955](https://github.com/dashpay/dash/pull/7955)

### Masternodes, quorums, and sporks

- Nodes retain the final quorum commitment with the highest signer count.
  [Dash #8085](https://github.com/dashpay/dash/pull/8085)
- Nodes request a signature share again if the pending queue was full when
  the share arrived. [Dash #7994](https://github.com/dashpay/dash/pull/7994)
- Nodes accept spork updates signed in the same second.
  [Dash #8127](https://github.com/dashpay/dash/pull/8127)
- Re-signing an InstantSend or ChainLock request restarts its signing timeout.
  [Dash #8203](https://github.com/dashpay/dash/pull/8203)
- The cache for ChainLock and InstantSend quorum lists follows the active
  chain. This fixes cases where a node needed a restart to resume accepting
  ChainLocks after a reorganization.
  [Dash #8102](https://github.com/dashpay/dash/pull/8102)

### GUI

- Fixed issues in the address book, Coin Control, options and PSBT dialogs,
  CoinJoin send tab, shared masternode dialogs, wallet migration, proposal
  list, traffic graph, and overview page.
  [Dash #8123](https://github.com/dashpay/dash/pull/8123),
  [Dash #8124](https://github.com/dashpay/dash/pull/8124)

### RPC and REST

- Quorum RPCs reject quorum types outside the range 0–255.
  [Dash #7954](https://github.com/dashpay/dash/pull/7954)
- Fixed argument handling, range checks, or locking in `masternode payments`,
  `masternode winners`, `getspecialtxes`, `verifyislock`, `getaddressdeltas`,
  `evodb verify`, `evodb repair`, and `getblockheaders`.
  [Dash #8118](https://github.com/dashpay/dash/pull/8118),
  [Dash #8125](https://github.com/dashpay/dash/pull/8125)
- `getdeploymentinfo` and REST `/deploymentinfo` return an error when the
  requested block has yet to be fully validated.
  [Dash #8098](https://github.com/dashpay/dash/pull/8098)

### Contrib

- Fixed the OpenRC init script, the ZeroMQ subscriber example on Python 3.14,
  and IPv6 traffic shaping in the QoS script.
  [Dash #8109](https://github.com/dashpay/dash/pull/8109)

## Changelog and credits

See the [Dash Core v24.0.2 to v24.0.3 changelog](https://github.com/dashpay/dash/compare/v24.0.2...dashpay:v24.0.3).
For earlier Cosanta changes, see the [Cosanta Core v24.0.1 release notes](release-notes-24.0.1.md).

Thanks to PastaPastaPasta and the Dash Core contributors, Dmitriy Korniychuk
for the Cosanta Qt theme, and everyone who reported issues, tested builds, or
reviewed changes.
