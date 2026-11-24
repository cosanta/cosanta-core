# Cosanta Core v24.0.1

Cosanta Core v24.0.1 incorporates the changes from Dash Core v24.0.1 and
v24.0.2. It also fixes proof-of-stake header synchronization and introduces
the PosV3 proof-of-stake upgrade.

## Proof-of-stake header synchronization

- During initial synchronization, the node buffers headers in segments below
  the last checkpoint. It adds a segment to the block index only when its last
  header matches the checkpoint hash.
- Past the last checkpoint, the node defers headers whose stake it cannot yet
  verify without penalizing the peer for the deferral.
- The node uses the UTXO set and block undo data to verify recent competing
  headers, including on pruned nodes and nodes running without `-txindex`.

## PosV3

PosV3 introduces 10-second staking slots and a 256-bit stake modifier.
The proof-of-stake rules change at block **1,078,675** on mainnet and block
**888,547** on testnet. Upgrade nodes before the activation height on their
network.

## Dash Core changes

- [Dash Core v24.0.1](../dash/release-notes-24.0.1.md) includes fixes for v24
  activation, quorum commitments, governance, and network handling.
- [Dash Core v24.0.2](../dash/release-notes-24.0.2.md) includes fixes for a
  memory leak, ChainLock shutdown, CoinJoin scheduling, governance sync, and
  a macOS shutdown crash.
