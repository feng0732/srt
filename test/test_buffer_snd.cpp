#include <iostream>
#include <cstring>
#include "gtest/gtest.h"
#include "buffer_snd.h"
#include "srt.h"
#include "sync.h"

using namespace std;
using namespace srt;

class CSndBufferDeadlineTest
    : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // ip family, number of blocks, max payload, auth tag size.
        m_pBuf = new CSndBuffer(AF_INET, 16, 1316, 0);
    }

    void TearDown() override
    {
        delete m_pBuf;
    }

    /// Add @a npackets single-packet messages to the sending buffer.
    void addPackets(int npackets)
    {
        const char payload[16] = "deadline";
        for (int i = 0; i < npackets; ++i)
        {
            SRT_MSGCTRL mc = srt_msgctrl_default;
            mc.pktseq = i;
            mc.msgttl = SRT_MSGTTL_INF;
            m_pBuf->addBuffer(payload, (int) strlen(payload), mc);
        }
    }

    CSndBuffer* m_pBuf;
};

/// Deadline and abandonment marks stick to the right blocks.
TEST_F(CSndBufferDeadlineTest, DeadlineAndAbandonMarks)
{
    addPackets(4);
    ASSERT_EQ(m_pBuf->getCurrBufSize(), 4);

    const sync::steady_clock::time_point dl =
        sync::steady_clock::now() + sync::milliseconds_from(500);

    // Initially no deadline and nothing abandoned.
    EXPECT_TRUE(sync::is_zero(m_pBuf->getPacketRexmitDeadline(2)));
    EXPECT_FALSE(m_pBuf->isRexmitAbandoned(2));

    m_pBuf->setPacketRexmitDeadline(2, dl);
    m_pBuf->markRexmitAbandoned(2);

    EXPECT_EQ(m_pBuf->getPacketRexmitDeadline(2), dl);
    EXPECT_TRUE(m_pBuf->isRexmitAbandoned(2));

    // Other blocks are untouched.
    EXPECT_TRUE(sync::is_zero(m_pBuf->getPacketRexmitDeadline(1)));
    EXPECT_FALSE(m_pBuf->isRexmitAbandoned(1));
    EXPECT_FALSE(m_pBuf->isRexmitAbandoned(3));
}

/// Acknowledging data releases the blocks; when the blocks are reused for new
/// messages the deadline and abandonment state starts clean again.
TEST_F(CSndBufferDeadlineTest, MarksClearedOnAckAndBlockReuse)
{
    addPackets(4);

    const sync::steady_clock::time_point dl =
        sync::steady_clock::now() + sync::milliseconds_from(100);
    m_pBuf->setPacketRexmitDeadline(0, dl);
    m_pBuf->markRexmitAbandoned(0);
    EXPECT_TRUE(m_pBuf->isRexmitAbandoned(0));

    // ACK the first two packets: their blocks are released to the free pool.
    m_pBuf->ackData(2);
    ASSERT_EQ(m_pBuf->getCurrBufSize(), 2);

    // Add two new packets; they reuse the just-released circular blocks.
    addPackets(2);
    ASSERT_EQ(m_pBuf->getCurrBufSize(), 4);

    // Offsets 0 and 1 now map to the reused blocks, which must be clean.
    EXPECT_FALSE(m_pBuf->isRexmitAbandoned(0));
    EXPECT_FALSE(m_pBuf->isRexmitAbandoned(1));
    EXPECT_TRUE(sync::is_zero(m_pBuf->getPacketRexmitDeadline(0)));
    EXPECT_TRUE(sync::is_zero(m_pBuf->getPacketRexmitDeadline(1)));
}

/// Origin timestamps are reachable per offset, which is what the scheduler
/// uses to derive the playback deadline.
TEST_F(CSndBufferDeadlineTest, OriginTimePerOffset)
{
    addPackets(3);

    const sync::steady_clock::time_point o0 = m_pBuf->getPacketOriginTime(0);
    const sync::steady_clock::time_point o1 = m_pBuf->getPacketOriginTime(1);
    const sync::steady_clock::time_point o2 = m_pBuf->getPacketOriginTime(2);

    EXPECT_FALSE(sync::is_zero(o0));
    // All packets of back-to-back submissions share a non-epoch origin time.
    EXPECT_FALSE(sync::is_zero(o1));
    EXPECT_FALSE(sync::is_zero(o2));
    EXPECT_TRUE(o1 >= o0);
    EXPECT_TRUE(o2 >= o1);
}
