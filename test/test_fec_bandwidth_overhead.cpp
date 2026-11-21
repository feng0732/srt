/*
 * SRT - Secure, Reliable, Transport
 * Copyright (c) 2024 Haivision Systems Inc.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 */

#include <memory>
#include <string>
#include <vector>

#include "gtest/gtest.h"
#include "core.h"
#include "fec.h"
#include "packetfilter.h"
#include "packetfilter_api.h"

using namespace std;
using namespace srt;

namespace {

// Builds a configured builtin FEC filter with the given config string.
class FecOverheadTest: public ::testing::Test
{
protected:
    static const int SOCKID = 54321;
    static const int ISN = 123456;
    static const size_t PAYLOAD_SIZE = 1316;

    unique_ptr<FECFilterBuiltin> makeFilter(const string& conf)
    {
        vector<SrtPacket> provided;
        SrtFilterInitializer init;
        init.socket_id = SOCKID;
        init.snd_isn = ISN - 1;
        init.rcv_isn = ISN - 1;
        init.payload_size = PAYLOAD_SIZE;
        init.rcvbuf_size = CSrtConfig::DEF_BUFFER_SIZE;

        return unique_ptr<FECFilterBuiltin>(new FECFilterBuiltin(init, provided, conf));
    }
};

// Row-only FEC: one FEC packet per `cols` source packets.
TEST_F(FecOverheadTest, RowOnly)
{
    // 1 FEC per 2 source packets -> 50%
    EXPECT_EQ(makeFilter("fec,rows:1,cols:2")->extraBandwidthOverheadPercent(), 50);

    // 1 FEC per 4 source packets -> 25%
    EXPECT_EQ(makeFilter("fec,rows:1,cols:4")->extraBandwidthOverheadPercent(), 25);

    // 1 FEC per 10 source packets -> 10%
    EXPECT_EQ(makeFilter("fec,rows:1,cols:10")->extraBandwidthOverheadPercent(), 10);
}

// Columns-only FEC (negative rows): one FEC packet per `|rows|` source packets.
TEST_F(FecOverheadTest, ColumnsOnly)
{
    // 1 FEC per 4 source packets -> 25%
    EXPECT_EQ(makeFilter("fec,rows:-4,cols:10")->extraBandwidthOverheadPercent(), 25);

    // 1 FEC per 5 source packets -> 20%
    EXPECT_EQ(makeFilter("fec,rows:-5,cols:8")->extraBandwidthOverheadPercent(), 20);
}

// Full matrix: (cols + rows) FEC packets per (cols * rows) source packets.
TEST_F(FecOverheadTest, FullMatrix)
{
    // 10x5: 15 FEC per 50 source -> 30%
    EXPECT_EQ(makeFilter("fec,rows:5,cols:10")->extraBandwidthOverheadPercent(), 30);

    // 4x4: 8 FEC per 16 source -> 50%
    EXPECT_EQ(makeFilter("fec,rows:4,cols:4")->extraBandwidthOverheadPercent(), 50);

    // 10x10: 20 FEC per 100 source -> 20%
    EXPECT_EQ(makeFilter("fec,rows:10,cols:10")->extraBandwidthOverheadPercent(), 20);
}

// The percentage must be rounded UP, so the reserved budget never falls
// short of the redundancy actually generated.
TEST_F(FecOverheadTest, RoundingIsUpward)
{
    // 1 FEC per 3 source = 33.333...% -> 34%
    EXPECT_EQ(makeFilter("fec,rows:1,cols:3")->extraBandwidthOverheadPercent(), 34);

    // 1 FEC per 7 source = 14.2857...% -> 15%
    EXPECT_EQ(makeFilter("fec,rows:1,cols:7")->extraBandwidthOverheadPercent(), 15);

    // Full matrix 3x3: 6 FEC per 9 source = 66.666...% -> 67%
    EXPECT_EQ(makeFilter("fec,rows:3,cols:3")->extraBandwidthOverheadPercent(), 67);

    // Full matrix 7x7: 14 FEC per 49 source = 28.571...% -> 29%
    EXPECT_EQ(makeFilter("fec,rows:7,cols:7")->extraBandwidthOverheadPercent(), 29);
}

// The generic filter base class reports no overhead by default.
TEST_F(FecOverheadTest, BaseFilterDefaultsToZero)
{
    // A configured FEC filter always reports a positive value.
    EXPECT_GT(makeFilter("fec,rows:1,cols:10")->extraBandwidthOverheadPercent(), 0);
}

} // namespace
