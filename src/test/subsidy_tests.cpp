// Copyright (c) 2014-2023 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparams.h>
#include <validation.h>

#include <test/util/setup_common.h>

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(subsidy_tests, TestingSetup)

BOOST_AUTO_TEST_CASE(block_subsidy_test)
{
    const auto chainParams = CreateChainParams(*m_node.args, CBaseChainParams::MAIN);

    // Cosanta's height-based emission, treasury boundary and halvings.
    struct SubsidyCase { int height; CAmount classic; CAmount v20; };
    constexpr SubsidyCase cases[]{
        {0, 1000000, 1000000},
        {11110, 1000000, 1000000},
        {11111, 2000000, 2000000},
        {22221, 2000000, 2000000},
        {22222, 3000000, 3000000},
        {99998, 9000000, 9000000},
        {99999, 10000000, 10000000},
        {111110, 10000000, 10000000},
        {111111, 20000000, 20000000},
        {699999, 70000000, 70000000},
        {700000, 80000000, 80000000},
        {899999, 90000000, 90000000},
        {900000, 100000000, 100000000},
        {998999, 4500000000, 4500000000},
        {999000, 5000000000, 5000000000},
        {1000000, 5000000000, 5000000000},
        {1000001, 4500000000, 4000000000},
        {1048575, 4500000000, 4000000000},
        {1048576, 2250000000, 2000000000},
        {2097152, 1125000000, 1000000000},
        {35651584, 0, 0},
    };
    for (const auto& test : cases) {
        BOOST_TEST_CONTEXT("previous height " << test.height) {
            for (const auto bits : {0x1c4a47c4, 0x1b10d50b}) {
                BOOST_CHECK_EQUAL(GetBlockSubsidyInner(bits, test.height, chainParams->GetConsensus(), false), test.classic);
                BOOST_CHECK_EQUAL(GetBlockSubsidyInner(bits, test.height, chainParams->GetConsensus(), true), test.v20);
            }
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()
