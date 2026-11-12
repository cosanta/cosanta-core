#!/usr/bin/env python3
# Copyright (c) 2026 The Cosanta Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Header sync of a chain with proof-of-stake blocks.

A proof-of-stake header can be checked only once its stake is visible from the
tip. Below the last checkpoint headers are taken in whole segments that hash to
a checkpoint; above it they wait until the tip gets close enough.
"""

from decimal import Decimal

from test_framework.governance import EXPECTED_STDERR_NO_GOV_PRUNE
from test_framework.messages import CBlock, CBlockHeader, HeaderAndShortIDs, from_hex, msg_block, msg_cmpctblock, msg_headers
from test_framework.p2p import P2PInterface
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal, assert_greater_than, p2p_port

# A stake needs a whole coin and the block reward is below a hundredth of one, so
# the chain starts with enough proof-of-work blocks to fund every stake of the test.
POW_BLOCKS = 3700
STAKE_OUTPUTS = 25
FIRST_POS_BLOCK = 3000
STAKE_MIN_AGE = 600
# The first checkpoint lies further than one plain headers message reaches; the
# segments after it hold both proof-of-work and proof-of-stake blocks.
CHECKPOINT_HEIGHTS = [2500, 3720, 3732]
PRESYNC_RETRY_INTERVAL = 10 * 60
MAX_POS_BLOCK_AHEAD_TIME = 180
HEADERS_RESPONSE_TIME = 2 * 60
# A plain headers message holds fewer headers than a compressed one.
MAX_PLAIN_HEADERS = 2000

STAKER, NO_CHECKPOINTS, CHECKPOINTS, PRUNED, WRONG_CHECKPOINT, P2P_TARGET = range(6)


class HeadersPeer(P2PInterface):
    """Answers getheaders from a fixed list of headers, in plain headers messages.

    The headers carry the hashes the node gave them. after_first_batch tells what
    follows the first message with headers:
    None - further batches, "silent" - no answer, "empty" - a message without headers.
    """

    def __init__(self, headers, after_first_batch=None):
        super().__init__()
        self.headers = headers
        self.after_first_batch = after_first_batch
        self.batches_sent = 0
        self.known = {header.sha256: i for i, header in enumerate(headers)}

    def on_getheaders(self, message):
        start = 0
        for block_hash in message.locator.vHave:
            if block_hash in self.known:
                start = self.known[block_hash] + 1
                break
        if self.batches_sent > 0 and self.after_first_batch == "silent":
            return
        if self.batches_sent > 0 and self.after_first_batch == "empty":
            self.send_message(msg_headers([]))
            return
        self.batches_sent += 1
        self.send_message(msg_headers(self.headers[start:start + MAX_PLAIN_HEADERS]))

    # The node asks for compressed headers; plain ones are an equally valid answer.
    on_getheaders2 = on_getheaders


class BlockPeer(P2PInterface):
    """Holds one block; serves its header only once told to."""

    def __init__(self, block):
        super().__init__()
        self.block = block
        self.serve_headers = False

    def on_getheaders(self, message):
        if self.serve_headers:
            self.send_message(msg_headers([CBlockHeader(self.block)]))

    on_getheaders2 = on_getheaders

    def on_getdata(self, message):
        self.send_message(msg_block(self.block))


class PosHeaderSyncTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 6
        self.common_args = [f"-posparams={FIRST_POS_BLOCK}:{STAKE_MIN_AGE}", "-stakeautocombine=0", "-staking=0"]
        self.extra_args = [self.common_args] * self.num_nodes
        self.rpc_timeout = 240

    def add_options(self, parser):
        # Staking needs a legacy wallet.
        self.add_wallet_options(parser, descriptors=False)

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()
        self.skip_if_no_bdb()

    def setup_network(self):
        self.setup_nodes()

    def running_nodes(self):
        return [node for node in self.nodes if node.running]

    def mine(self, count):
        """Proof-of-work blocks. Blocks mined in one second run ahead of the clock, so it is moved along."""
        address = self.staker.getnewaddress()
        while count > 0:
            batch = min(count, 100)
            self.bump_mocktime(120, nodes=self.running_nodes())
            self.generatetoaddress(self.nodes[STAKER], batch, address, sync_fun=self.no_op)
            count -= batch

    def stake(self, count):
        for _ in range(count):
            self.bump_mocktime(60, nodes=self.running_nodes())
            assert_equal(len(self.staker.generatestake(1)), 1)

    def restart_fresh(self, i, extra_args):
        self.nodes[i].mocktime = self.mocktime
        self.restart_node(i, extra_args=self.common_args + extra_args)

    def connect_to_staker(self, i):
        self.nodes[i].addnode(f"127.0.0.1:{p2p_port(STAKER)}", "onetry")

    def wait_for_tip(self, i, block_hash):
        self.wait_until(lambda: self.nodes[i].getbestblockhash() == block_hash, timeout=240)

    def log_count(self, i, text):
        with open(self.nodes[i].debug_log_path, encoding="utf-8", errors="replace") as log:
            return sum(text in line for line in log)

    def checkpoint_args(self, heights, hashes=None):
        hashes = hashes or {height: self.nodes[STAKER].getblockhash(height) for height in heights}
        return [f"-checkpoint={height}:{hashes[height]}" for height in heights]

    def build_chain(self):
        node = self.nodes[STAKER]
        self.staker = node.get_wallet_rpc(self.default_wallet_name)
        self.mine(POW_BLOCKS)
        # One coin takes over a hundred coinbase outputs, so every stake gets a transaction of its own.
        for _ in range(STAKE_OUTPUTS):
            self.staker.sendtoaddress(self.staker.getnewaddress(), Decimal("1"))
        self.mine(12)
        self.bump_mocktime(STAKE_MIN_AGE * 2, nodes=self.running_nodes())
        # Proof-of-work blocks stay valid after the first proof-of-stake one.
        self.stake(12)
        self.mine(5)
        self.stake(4)
        assert_equal(node.getblockcount(), POW_BLOCKS + 12 + 21)
        assert_greater_than(node.getblockcount(), CHECKPOINT_HEIGHTS[-1])

    def test_window_mode_from_genesis(self):
        self.log.info("A node without checkpoints syncs the whole chain waiting for stakes to become visible")
        tip = self.nodes[STAKER].getbestblockhash()
        self.restart_fresh(NO_CHECKPOINTS, [])
        self.connect_to_staker(NO_CHECKPOINTS)
        self.wait_for_tip(NO_CHECKPOINTS, tip)
        assert_greater_than(self.log_count(NO_CHECKPOINTS, "deferred "), 0)
        assert_equal(self.log_count(NO_CHECKPOINTS, "Misbehaving"), 0)

    def test_anchored_segments(self, i, extra_args):
        tip = self.nodes[STAKER].getbestblockhash()
        self.restart_fresh(i, self.checkpoint_args(CHECKPOINT_HEIGHTS) + extra_args)
        self.connect_to_staker(i)
        self.wait_for_tip(i, tip)
        assert_equal(self.log_count(i, "headers up to"), len(CHECKPOINT_HEIGHTS))
        assert_equal(self.log_count(i, "Misbehaving"), 0)

    def test_wrong_checkpoint(self):
        self.log.info("A chain that does not hash to a checkpoint is not accepted at all")
        height = CHECKPOINT_HEIGHTS[0]
        wrong = {height: self.nodes[STAKER].getblockhash(height + 1)}
        self.restart_fresh(WRONG_CHECKPOINT, self.checkpoint_args([height], wrong))
        with self.nodes[WRONG_CHECKPOINT].assert_debug_log(["headers do not match a checkpoint"], timeout=60):
            self.connect_to_staker(WRONG_CHECKPOINT)
        info = self.nodes[WRONG_CHECKPOINT].getblockchaininfo()
        assert_equal(info["headers"], 0)
        assert_equal(info["blocks"], 0)
        # The peer is punished once, not asked again in a loop.
        assert_equal(self.log_count(WRONG_CHECKPOINT, "headers do not match a checkpoint"), 1)
        self.stop_node(WRONG_CHECKPOINT)

    def test_reorg(self):
        self.log.info("Nodes without a transaction index follow a competing branch")
        followers = [NO_CHECKPOINTS, PRUNED]
        self.stake(1)
        old_tip = self.nodes[STAKER].getbestblockhash()
        for i in followers:
            self.wait_for_tip(i, old_tip)
            self.nodes[i].setnetworkactive(False)
        self.nodes[CHECKPOINTS].setnetworkactive(False)

        self.nodes[STAKER].invalidateblock(old_tip)
        self.stake(2)
        new_tip = self.nodes[STAKER].getbestblockhash()
        assert old_tip != new_tip

        for i in followers:
            self.nodes[i].setnetworkactive(True)
            self.connect_to_staker(i)
            self.wait_for_tip(i, new_tip)
            assert_equal(self.log_count(i, "Misbehaving"), 0)

    def test_catch_up(self):
        self.log.info("A node that was offline for a while catches up")
        self.stake(1)
        self.mine(20)
        self.stake(1)
        self.nodes[CHECKPOINTS].setnetworkactive(True)
        self.connect_to_staker(CHECKPOINTS)
        self.wait_for_tip(CHECKPOINTS, self.nodes[STAKER].getbestblockhash())
        assert_equal(self.log_count(CHECKPOINTS, "Misbehaving"), 0)

    def test_block_announced_too_early(self):
        self.log.info("A block whose timestamp is still too new for our clock is fetched once the clock catches up")
        node = self.nodes[CHECKPOINTS]
        old_tip = node.getbestblockhash()
        node.setnetworkactive(False)
        self.stake(1)
        new_tip = self.nodes[STAKER].getbestblockhash()
        block_time = self.nodes[STAKER].getblockheader(new_tip)["time"]

        # The block is announced only once, 20 seconds before this node's clock allows it.
        node.setmocktime(block_time - MAX_POS_BLOCK_AHEAD_TIME - 20)
        node.setnetworkactive(True)
        with node.assert_debug_log(["time-too-new"], timeout=30):
            self.connect_to_staker(CHECKPOINTS)
        assert_equal(node.getbestblockhash(), old_tip)

        with node.assert_debug_log(["again for a header that was too new"], timeout=30):
            node.setmocktime(block_time - MAX_POS_BLOCK_AHEAD_TIME + 5)
        self.wait_for_tip(CHECKPOINTS, new_tip)
        assert_equal(self.log_count(CHECKPOINTS, "Misbehaving"), 0)
        node.setmocktime(self.mocktime)

    def test_compact_block_announced_too_early(self):
        self.log.info("So is a block announced too early as a compact block")
        node = self.nodes[CHECKPOINTS]
        old_tip = node.getbestblockhash()
        node.setnetworkactive(False)
        self.stake(1)
        new_tip = self.nodes[STAKER].getbestblockhash()
        block = from_hex(CBlock(), self.nodes[STAKER].getblock(new_tip, 0))
        block.rehash()
        assert_equal(block.hash, new_tip)

        node.setmocktime(block.nTime - MAX_POS_BLOCK_AHEAD_TIME - 20)
        node.setnetworkactive(True)
        peer = node.add_p2p_connection(BlockPeer(block))
        compact_block = HeaderAndShortIDs()
        compact_block.initialize_from_block(block, prefill_list=list(range(len(block.vtx))))
        with node.assert_debug_log(["time-too-new"], timeout=30):
            peer.send_message(msg_cmpctblock(compact_block.to_p2p()))
        assert_equal(node.getbestblockhash(), old_tip)

        peer.serve_headers = True
        with node.assert_debug_log(["again for a header that was too new"], timeout=30):
            node.setmocktime(block.nTime - MAX_POS_BLOCK_AHEAD_TIME + 5)
        self.wait_for_tip(CHECKPOINTS, new_tip)
        assert_equal(self.log_count(CHECKPOINTS, "Misbehaving"), 0)
        node.setmocktime(self.mocktime)

    def chain_headers(self):
        """Headers of the chain as it was when the checkpoints were taken, each with the hash the node gave it."""
        headers = []
        for block_hash, header_hex in self.chain:
            header = from_hex(CBlockHeader(), header_hex)
            header.set_hash(block_hash)
            headers.append(header)
        return headers

    def test_p2p_headers(self):
        node = self.nodes[P2P_TARGET]
        self.restart_fresh(P2P_TARGET, self.checkpoint_args(CHECKPOINT_HEIGHTS, self.checkpoint_hashes))

        self.log.info("A peer whose chain ends below a checkpoint is asked again only after a pause, and dropped after three failures")
        short_chain = "ends below a checkpoint, trying another peer"
        with node.assert_debug_log([short_chain], timeout=30):
            peer = node.add_p2p_connection(HeadersPeer(self.chain_headers()[:100]), wait_for_verack=False)
        for failures in (2, 3):
            assert_equal(self.log_count(P2P_TARGET, short_chain), failures - 1)
            assert peer.is_connected
            self.bump_mocktime(PRESYNC_RETRY_INTERVAL + 1, nodes=[node])
            self.wait_until(lambda: self.log_count(P2P_TARGET, short_chain) == failures)
        peer.wait_for_disconnect()
        assert_equal(node.getblockchaininfo()["headers"], 0)

        self.log.info("A peer that goes silent in the middle of a segment loses the role after a timeout")
        peer = node.add_p2p_connection(HeadersPeer(self.chain_headers(), after_first_batch="silent"))
        peer.wait_until(lambda: peer.batches_sent == 1)
        peer.sync_with_ping()
        with node.assert_debug_log(["Timeout downloading headers below a checkpoint"], timeout=30):
            self.bump_mocktime(2 * HEADERS_RESPONSE_TIME + 1, nodes=[node])
        assert_equal(node.getblockchaininfo()["headers"], 0)
        peer.peer_disconnect()
        peer.wait_for_disconnect()

        self.log.info("A peer that has no more headers in the middle of a segment loses it at once")
        failures = self.log_count(P2P_TARGET, short_chain)
        peer = node.add_p2p_connection(HeadersPeer(self.chain_headers(), after_first_batch="empty"))
        self.wait_until(lambda: self.log_count(P2P_TARGET, short_chain) == failures + 1)
        assert_equal(node.getblockchaininfo()["headers"], 0)
        peer.peer_disconnect()
        peer.wait_for_disconnect()

        self.log.info("A header changed below a checkpoint breaks the match: the peer is dropped, nothing is kept")
        headers = self.chain_headers()
        headers[CHECKPOINT_HEIGHTS[0] - 1].nNonce += 1
        peer = node.add_p2p_connection(HeadersPeer(headers), wait_for_verack=False)
        peer.wait_for_disconnect()
        assert_equal(node.getblockchaininfo()["headers"], 0)

        self.log.info("A proof-of-stake signature of the wrong size is refused; the segment before it stays")
        headers = self.chain_headers()
        signed = CHECKPOINT_HEIGHTS[1] - 2
        assert headers[signed].is_proof_of_stake()
        headers[signed].posBlockSig = headers[signed].posBlockSig[:-1]
        # The signature is outside the hash, so the peer may be an honest relayer: it is neither punished nor dropped.
        unusable = self.log_count(P2P_TARGET, short_chain)
        with node.assert_debug_log(["are unusable, trying another peer"], timeout=60):
            bad_signature_peer = node.add_p2p_connection(HeadersPeer(headers), wait_for_verack=False)
        assert_equal(node.getblockchaininfo()["headers"], CHECKPOINT_HEIGHTS[0])
        # Its first message was full for plain headers, though the node had asked for compressed
        # ones: that is not taken for the end of the peer's chain.
        assert_equal(self.log_count(P2P_TARGET, short_chain), unusable)

        self.log.info("Untouched headers from another peer are accepted up to the last checkpoint")
        peer = node.add_p2p_connection(HeadersPeer(self.chain_headers()))
        self.wait_until(lambda: node.getblockchaininfo()["headers"] >= CHECKPOINT_HEIGHTS[-1], timeout=120)
        assert peer.is_connected
        assert bad_signature_peer.is_connected
        assert_equal(node.getblockchaininfo()["blocks"], 0)
        # The peer that failed was not asked again in a loop.
        assert_equal(self.log_count(P2P_TARGET, "are unusable, trying another peer"), 1)
        assert_equal(self.log_count(P2P_TARGET, "Misbehaving"), 1)
        self.stop_node(P2P_TARGET)

    def run_test(self):
        for i in range(1, self.num_nodes):
            self.stop_node(i)
        self.build_chain()
        node = self.nodes[STAKER]
        self.chain = []
        for height in range(1, node.getblockcount() + 1):
            block_hash = node.getblockhash(height)
            self.chain.append((block_hash, node.getblockheader(block_hash, False)))
        self.checkpoint_hashes = {height: self.chain[height - 1][0] for height in CHECKPOINT_HEIGHTS}

        self.test_window_mode_from_genesis()
        self.log.info("A node with checkpoints takes the headers below the last one in segments")
        self.test_anchored_segments(CHECKPOINTS, [])
        self.log.info("So does a pruned node")
        self.test_anchored_segments(PRUNED, ["-prune=1000"])
        self.test_wrong_checkpoint()
        self.test_p2p_headers()
        self.test_reorg()
        self.test_catch_up()
        self.test_block_announced_too_early()
        self.test_compact_block_announced_too_early()
        self.stop_node(PRUNED, expected_stderr=EXPECTED_STDERR_NO_GOV_PRUNE)


if __name__ == '__main__':
    PosHeaderSyncTest().main()
