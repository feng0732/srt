#include <utility>
#include <vector>

#include "gtest/gtest.h"
#include "test_env.h"
#include "common.h"
#include "list.h"

using namespace std;
using namespace srt;

class CRcvFilterLossListTest
    : public ::testing::Test
{
protected:
    CRcvFilterLossList m_lossList;

    typedef CRcvFilterLossList::SeqRange SeqRange;

    // Expect the exact contents of the list, in order.
    void ExpectRanges(const vector<SeqRange>& expected)
    {
        EXPECT_EQ(m_lossList.ranges(), expected);
        EXPECT_EQ(m_lossList.empty(), expected.empty());
    }
};

/// Check the state of the freshly created list.
TEST_F(CRcvFilterLossListTest, Create)
{
    EXPECT_TRUE(m_lossList.empty());
    EXPECT_EQ(m_lossList.ranges().size(), 0u);
}

/// Insert a single loss and remove it.
TEST_F(CRcvFilterLossListTest, InsertRemoveSingle)
{
    m_lossList.insert(10, 10);
    ExpectRanges({SeqRange(10, 10)});

    EXPECT_TRUE(m_lossList.remove(10));
    EXPECT_TRUE(m_lossList.empty());
    EXPECT_FALSE(m_lossList.remove(10));
}

/// Insert a single range and remove it as a whole.
TEST_F(CRcvFilterLossListTest, InsertRemoveRange)
{
    m_lossList.insert(10, 15);
    ExpectRanges({SeqRange(10, 15)});

    m_lossList.remove(10, 15);
    EXPECT_TRUE(m_lossList.empty());
}

/// Records may come out of order (e.g. an older FEC group is dismissed
/// after a newer one); they must be kept ordered by sequence.
TEST_F(CRcvFilterLossListTest, InsertOutOfOrder)
{
    m_lossList.insert(20, 22);
    m_lossList.insert(10, 12);
    m_lossList.insert(16, 16);
    ExpectRanges({SeqRange(10, 12), SeqRange(16, 16), SeqRange(20, 22)});
}

/// Inserting a record that overlaps or sticks to existing ones must merge
/// them into a single range.
TEST_F(CRcvFilterLossListTest, InsertMergesOverlapping)
{
    m_lossList.insert(10, 12);
    m_lossList.insert(14, 16);
    // Overlaps the first range.
    m_lossList.insert(12, 14);
    ExpectRanges({SeqRange(10, 16)});

    // Sticks to the existing range from both sides at once.
    m_lossList.insert(20, 22);
    m_lossList.insert(17, 19);
    ExpectRanges({SeqRange(10, 22)});
}

/// Inserting a range that covers several existing ranges absorbs them all.
TEST_F(CRcvFilterLossListTest, InsertAbsorbsMultiple)
{
    m_lossList.insert(10, 12);
    m_lossList.insert(20, 22);
    m_lossList.insert(30, 32);
    m_lossList.insert(5, 40);
    ExpectRanges({SeqRange(5, 40)});
}

/// Duplicated records must not create duplicated entries.
TEST_F(CRcvFilterLossListTest, InsertDuplicate)
{
    m_lossList.insert(10, 15);
    m_lossList.insert(10, 15);
    m_lossList.insert(11, 14);
    m_lossList.insert(10, 10);
    ExpectRanges({SeqRange(10, 15)});
}

/// Removing a sequence in the middle of a range splits it into two.
TEST_F(CRcvFilterLossListTest, RemoveSplitsRange)
{
    m_lossList.insert(10, 20);
    EXPECT_TRUE(m_lossList.remove(15));
    ExpectRanges({SeqRange(10, 14), SeqRange(16, 20)});

    // Remove the edges of the split ranges.
    EXPECT_TRUE(m_lossList.remove(10));
    EXPECT_TRUE(m_lossList.remove(20));
    ExpectRanges({SeqRange(11, 14), SeqRange(16, 19)});

    // A sequence that is not recorded must not be found.
    EXPECT_FALSE(m_lossList.remove(15));
    EXPECT_FALSE(m_lossList.remove(100));
}

/// Removing a range may shrink, split and erase multiple records.
TEST_F(CRcvFilterLossListTest, RemoveRangeVariants)
{
    m_lossList.insert(10, 20);
    m_lossList.insert(30, 40);
    m_lossList.insert(50, 60);

    // Shrink the first range from the left, split the second,
    // and leave the third untouched.
    m_lossList.remove(5, 35);
    ExpectRanges({SeqRange(36, 40), SeqRange(50, 60)});

    // Erase the first one completely and shrink the second from the right.
    m_lossList.remove(36, 55);
    ExpectRanges({SeqRange(56, 60)});
}

/// removeUpTo drops all sequences up to the given one, inclusive.
TEST_F(CRcvFilterLossListTest, RemoveUpTo)
{
    m_lossList.insert(10, 20);
    m_lossList.insert(30, 40);

    m_lossList.removeUpTo(15);
    ExpectRanges({SeqRange(16, 20), SeqRange(30, 40)});

    m_lossList.removeUpTo(30);
    ExpectRanges({SeqRange(31, 40)});

    m_lossList.removeUpTo(100);
    EXPECT_TRUE(m_lossList.empty());

    // No crash on an empty list.
    m_lossList.removeUpTo(100);
    EXPECT_TRUE(m_lossList.empty());
}

/// Sequence numbers wrap around at CSeqNo::m_iMaxSeqNo; ranges crossing
/// the wrap point must be handled consistently.
TEST_F(CRcvFilterLossListTest, WrapAround)
{
    const int32_t MAXSEQ = CSeqNo::m_iMaxSeqNo;

    // A range crossing the wrap point.
    m_lossList.insert(MAXSEQ - 2, CSeqNo::incseq(MAXSEQ));
    ExpectRanges({SeqRange(MAXSEQ - 2, 0)});

    // A record inserted past the wrap point sticks to the existing range.
    m_lossList.insert(1, 3);
    ExpectRanges({SeqRange(MAXSEQ - 2, 3)});

    // A record inserted before the wrap point sticks as well.
    m_lossList.insert(MAXSEQ - 5, MAXSEQ - 3);
    ExpectRanges({SeqRange(MAXSEQ - 5, 3)});

    // Remove a sequence right at the wrap point, splitting the range.
    EXPECT_TRUE(m_lossList.remove(MAXSEQ));
    ExpectRanges({SeqRange(MAXSEQ - 5, MAXSEQ - 1), SeqRange(0, 3)});

    // removeUpTo across the wrap point.
    m_lossList.removeUpTo(1);
    ExpectRanges({SeqRange(2, 3)});

    // Removal of a range crossing the wrap point.
    m_lossList.insert(MAXSEQ - 2, 2);
    m_lossList.remove(MAXSEQ - 1, 1);
    ExpectRanges({SeqRange(MAXSEQ - 2, MAXSEQ - 2), SeqRange(2, 3)});
}

/// A loss reported for a whole FEC group, partially recovered afterwards:
/// recovered packets are punched out of the record, the rest stays reported.
TEST_F(CRcvFilterLossListTest, PartialRecovery)
{
    // Two subsequent FEC groups reported irrecoverable losses.
    m_lossList.insert(100, 103);
    m_lossList.insert(104, 105); // sticks to the previous one
    ExpectRanges({SeqRange(100, 105)});

    // Retransmissions recover some of the packets.
    EXPECT_TRUE(m_lossList.remove(101));
    EXPECT_TRUE(m_lossList.remove(104));
    ExpectRanges({SeqRange(100, 100), SeqRange(102, 103), SeqRange(105, 105)});

    // The remaining losses are dropped (e.g. TLPKTDROP).
    m_lossList.removeUpTo(105);
    EXPECT_TRUE(m_lossList.empty());
}
