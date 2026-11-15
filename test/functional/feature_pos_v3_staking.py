#!/usr/bin/env python3
# Copyright (c) 2026 The Cosanta Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Proof-of-stake fork with the real staking thread on two nodes.

Both nodes stake their own coins at the same time after the activation height.
The kernel target saturates on regtest, so every slot is won by both nodes and
same-slot siblings are normal; each node keeps the sibling it saw first. The
clock is mocked and moved one slot at a time while the threads run.
"""

from decimal import Decimal
import time

from test_framework.messages import CBlockHeader, from_hex
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal, assert_greater_than, assert_greater_than_or_equal, force_finish_mnsync

# A stake needs a whole coin and the block reward is below a hundredth of one, so
# the chain starts with enough proof-of-work blocks to fund the stakes of both nodes.
POW_BLOCKS = 5300
STAKE_OUTPUTS = 16
FIRST_POS_BLOCK = 3000
STAKE_MIN_AGE = 120
# Funding ends at POW_BLOCKS + 12; node 0 stakes the last blocks below the fork with generatestake.
POSV3_HEIGHT = POW_BLOCKS + 12 + 6
SLOT = 10
POS_BIT = 0x10000000
POSV2_BITS = POS_BIT | 0x08000000
# A staked output matures only after 100 blocks, so every block of the test uses a coin of its own.
BLOCKS_BEFORE_RESTART = 8
BLOCKS_AFTER_RESTART = 8
# The staking thread sleeps one second between passes.
STEP_WALL_SECONDS = 1.2
V3_REJECTS = ["bad-pos-version-v3", "bad-nonce-v3", "time-not-slot", "time-not-after-parent", "time-too-new"]


class PosV3StakingTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 2
        common_args = [
            f"-posparams={FIRST_POS_BLOCK}:{STAKE_MIN_AGE}",
            f"-testactivationheight=posv3@{POSV3_HEIGHT}",
            "-stakeautocombine=0",
        ]
        self.extra_args = [common_args + ["-staking=0"]] * self.num_nodes
        self.staking_args = common_args + ["-staking=1"]
        self.rpc_timeout = 240

    def add_options(self, parser):
        # Staking needs a legacy wallet.
        self.add_wallet_options(parser, descriptors=False)

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()
        self.skip_if_no_bdb()

    def log_size(self, node):
        return node.debug_log_path.stat().st_size

    def log_since(self, node, offset):
        with open(node.debug_log_path, "rb") as log:
            log.seek(offset)
            return log.read().decode("utf-8", errors="replace")

    def start_staking(self, i):
        """Restart node i with the staking thread on and get it ready to stake."""
        node = self.nodes[i]
        offset = self.log_size(node)
        node.mocktime = self.mocktime
        # The wallet was created with load_on_startup, so it is loaded when the staking thread starts.
        self.restart_node(i, extra_args=self.staking_args)
        for j in range(self.num_nodes):
            if j != i and self.nodes[j].running:
                self.connect_nodes(i, j)
        # The thread stakes only once the masternode sync has finished and a peer is connected.
        force_finish_mnsync(node)
        self.wait_until(lambda: "PoSMiner started" in self.log_since(node, offset), timeout=10)
        assert "Staking start skipped" not in self.log_since(node, offset)
        return offset

    def minter(self, block_hash):
        """The node whose wallet staked a block: the coinbase pays its first output to the staker."""
        output = self.nodes[0].getblock(block_hash, 2)["tx"][0]["vout"][0]["scriptPubKey"]
        address = output["address"] if "address" in output else output["addresses"][0]
        owners = [i for i, node in enumerate(self.nodes) if node.getaddressinfo(address)["ismine"]]
        assert_equal(len(owners), 1)
        return owners[0]

    def run_until(self, height, done=lambda: True):
        """Move the clock one slot at a time while the threads stake, until the height is reached."""
        start = time.time()
        steps = 0
        while not (min(node.getblockcount() for node in self.nodes) >= height and done()):
            assert time.time() - start < 120, "the staking threads stopped making blocks"
            self.bump_mocktime(SLOT)
            time.sleep(STEP_WALL_SECONDS)
            steps += 1
        self.log.info(f"Height {height} after {steps} slots ({time.time() - start:.1f} s)")

    def check_v3_header_rules(self, node, first, last):
        parent_time = from_hex(CBlockHeader(), node.getblockheader(node.getblockhash(first - 1), False)).nTime
        for height in range(first, last + 1):
            header = from_hex(CBlockHeader(), node.getblockheader(node.getblockhash(height), False))
            assert_equal(header.nVersion & POSV2_BITS, POSV2_BITS)
            assert_equal(header.nTime % SLOT, 0)
            assert_greater_than(header.nTime, parent_time)
            assert_equal(header.nNonce, 0)
            parent_time = header.nTime

    def fund(self, wallets):
        """Proof-of-work blocks to node 0, then separate one-coin outputs for both wallets.

        Node 1 is down meanwhile: a block still in flight over a clock bump times out, and the peers disconnect.
        """
        addresses = [[wallet.getnewaddress() for _ in range(STAKE_OUTPUTS)] for wallet in wallets]
        self.stop_node(1)
        address = wallets[0].getnewaddress()
        remaining = POW_BLOCKS
        while remaining > 0:
            batch = min(remaining, 100)
            # Blocks mined in one second run ahead of the clock, so it is moved along.
            self.bump_mocktime(120, nodes=[self.nodes[0]])
            self.generatetoaddress(self.nodes[0], batch, address, sync_fun=self.no_op)
            remaining -= batch
        # One coin takes over a hundred coinbase outputs, so every stake gets a transaction of its own.
        for pair in zip(*addresses):
            for to in pair:
                wallets[0].sendtoaddress(to, Decimal("1"))
        self.bump_mocktime(120, nodes=[self.nodes[0]])
        self.generatetoaddress(self.nodes[0], 12, address, sync_fun=self.no_op)
        self.nodes[1].mocktime = self.mocktime
        self.start_node(1, extra_args=self.extra_args[1])
        self.connect_nodes(1, 0)
        self.sync_blocks(timeout=240)
        assert_equal(self.nodes[1].getblockcount(), POSV3_HEIGHT - 6)
        wallets[1] = self.nodes[1].get_wallet_rpc(self.default_wallet_name)
        for wallet in wallets:
            stakeable = [u for u in wallet.listunspent() if u["amount"] >= 1]
            assert_greater_than_or_equal(len(stakeable), STAKE_OUTPUTS)
        self.bump_mocktime(STAKE_MIN_AGE * 2)

    def run_test(self):
        self.log.info("Fund both wallets and cross the activation height with generatestake")
        wallets = [node.get_wallet_rpc(self.default_wallet_name) for node in self.nodes]
        self.fund(wallets)
        for _ in range(6):
            self.bump_mocktime(60)
            wallets[0].generatestake(1)
            # A block still in flight over a few clock bumps times out, and the peers disconnect.
            self.sync_blocks()
        assert_equal(self.nodes[1].getblockcount(), POSV3_HEIGHT)
        below = from_hex(CBlockHeader(), self.nodes[1].getblockheader(self.nodes[1].getblockhash(POSV3_HEIGHT - 1), False))
        assert_equal(below.nVersion & POSV2_BITS, POS_BIT)

        self.log.info("Start the staking threads on both nodes")
        log_offsets = [self.start_staking(i) for i in range(self.num_nodes)]

        self.log.info("Both threads stake blocks of the fork")
        self.run_until(POSV3_HEIGHT + BLOCKS_BEFORE_RESTART)

        self.log.info("Restart one node; its thread resumes staking")
        restart_height = self.nodes[1].getblockcount()
        restarted_offset = self.start_staking(1)
        self.run_until(restart_height + BLOCKS_AFTER_RESTART,
                       lambda: "PoSMiner : block is submitted" in self.log_since(self.nodes[1], restarted_offset))

        self.log.info("Pause one staker so that a same-height split resolves, then check convergence")
        # The reserve is per node, not per wallet.
        self.nodes[1].reservebalance(True, 1000000)
        target = max(node.getblockcount() for node in self.nodes) + 1
        self.run_until(target)
        self.sync_blocks(timeout=30)
        self.nodes[1].reservebalance(False)

        tip = self.nodes[0].getblockcount()
        for node in self.nodes:
            self.check_v3_header_rules(node, POSV3_HEIGHT, tip)
        for node in self.nodes:
            siblings = [t for t in node.getchaintips() if t["status"] != "active"]
            self.log.info(f"node {node.index}: {len(siblings)} side branches, longest {max([t['branchlen'] for t in siblings], default=0)}")
            for sibling in siblings:
                header = from_hex(CBlockHeader(), node.getblockheader(sibling["hash"], False))
                assert_equal(header.nTime % SLOT, 0)
                assert_equal(header.nNonce, 0)
        minters = [self.minter(self.nodes[0].getblockhash(h)) for h in range(POSV3_HEIGHT + 1, tip + 1)]
        self.log.info(f"Blocks {POSV3_HEIGHT + 1}..{tip} staked by node 0: {minters.count(0)}, node 1: {minters.count(1)}")
        assert minters.count(0) > 0 and minters.count(1) > 0

        self.log.info("Neither thread had one of its own blocks refused")
        for node, offset in zip(self.nodes, log_offsets):
            log = self.log_since(node, offset)
            assert "PoSMiner : block is rejected" not in log
            assert "proof check failed" not in log
            for reason in V3_REJECTS:
                # Allowed only for a peer's block that arrived just before this node's clock moved.
                count = log.count(reason)
                if count:
                    self.log.info(f"node {node.index}: {count} x {reason}")
                    assert_equal(reason, "time-too-new")


if __name__ == '__main__':
    PosV3StakingTest().main()
