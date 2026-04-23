# Cosanta Core version v19.3.0

Release is now available from:

  <https://cosanta.net/en/download/>

This is a new minor version release, bringing various bugfixes and other
improvements.

This release is optional for all nodes.

Please report bugs using the issue tracker at github:

  <https://github.com/cosanta/cosanta-core/issues>


# Upgrading and downgrading

## How to Upgrade

If you are running an older version, shut it down. Wait until it has completely
shut down (which might take a few minutes for older versions), then run the
installer (on Windows) or just copy over /Applications/Cosanta-Qt (on Mac) or
cosantad/cosanta-qt (on Linux). If you upgrade after DIP0003 activation and you were
using version < 0.13 you will have to reindex (start with -reindex-chainstate
or -reindex) to make sure your wallet has all the new data synced. Upgrading
from version 0.13 should not require any additional actions.

## Downgrade warning

### Downgrade to a version < v19.2.0

Downgrading to a version older than v19.2.0 is not supported due to changes
in the evodb database. If you need to use an older version, you must either
reindex or re-sync the whole chain.

# Notable changes

## CoinJoin improvements

This release fixes a couple of issues with mixing on nodes that start with no
wallet loaded initially.

## Wallet GUI improvements

Wallets with 100k+ txes should now be able to rescan without hanging forever
while processing notifications for every tx. Running `keypoolrefill` with a
large number of keys will no longer lockup the GUI and can be interrupted.
Running `upgradetohd` can also be interrupted now.

## Changes in RPCs, commands and config options

- `wipewallettxes`: New RPC command which removes all wallet transactions
- `wipetxes`: New command for `dash-wallet` that removes all wallet transactions
- `masternodelist`: New mode `hpmn` filters only HPMNs/EvoNodes
- `protx list`: New type `hpmn` filters only HPMNs/EvoNodes
- `-blockversion` config option is allowed on non-mainnet networks now

## Other changes

There were a few other minor changes too, specifically:
- Added Kittywhiskers Van Gogh (kittywhiskers) and Odysseas Gabrielides
(ogabrielides) to contributors list in 19.2.0 release notes
- There should be no false "unknown rules activated" warning in GUI and RPCs now
- Empty `settings.json` file no longer results in node startup failure
- Block processing was slightly optimized
- BLS library was updated to version 1.3.0 to fix a couple tiny issues
- Fixed a couple of small issues in tests

# v19.3.0 Change log

See detailed [set of changes](https://github.com/dashpay/dash/compare/v19.2.0...dashpay:v19.3.0).

# Credits

Thanks to everyone who directly contributed to this release:

- Odysseas Gabrielides (ogabrielides)
- PastaPastaPasta
- UdjinM6

As well as everyone that submitted issues, reviewed pull requests and helped
debug the release candidates.

# Older releases

Cosanta was forked from Dash Core.
