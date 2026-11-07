// Copyright (c) 2019-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bench/data.h>

namespace benchmark {
namespace data {

// Original benchmark transactions with a pre-mined Cosanta PoW header:
// nBits=0x1e0fffff, nNonce=2508444,
// hash=0000057decdd0505f069afd4f055217f2fc884ecbd9b2f08e2019c83a883cea7.
#include <bench/data/block813851.raw.h>
const std::vector<uint8_t> block813851{std::begin(raw_bench::block813851_raw), std::end(raw_bench::block813851_raw)};

} // namespace data
} // namespace benchmark
