#include <iostream>
#include "gtest/gtest.h"
#include "common.h"
#include "list.h"
#include "sync.h"

using namespace std;
using namespace srt;

class CSndLossListTest
    : public ::testing::Test
{
protected:
    void SetUp() override
    {
        m_lossList = new CSndLossList(CSndLossListTest::SIZE);
    }

    void TearDown() override
    {
        delete m_lossList;
    }

    void CheckEmptyArray()
    {
        EXPECT_EQ(m_lossList->getLossLength(), 0);
        EXPECT_EQ(m_lossList->popLostSeq(), SRT_SEQNO_NONE);
    }

    void CleanUpList()
    {
        while (m_lossList->popLostSeq() != SRT_SEQNO_NONE);
    }

    CSndLossList* m_lossList;

public:
    const int SIZE = 256;
};

/// Check the state of the freshly created list.
/// Capacity, loss length and pop().
TEST_F(CSndLossListTest, Create)
{
    CheckEmptyArray();
}

///////////////////////////////////////////////////////////////////////////////
///
/// The first group of tests checks insert and pop()
///
///////////////////////////////////////////////////////////////////////////////

/// Insert and pop one element from the list.
TEST_F(CSndLossListTest, InsertPopOneElem)
{
    EXPECT_EQ(m_lossList->insert(1, 1), 1);

    EXPECT_EQ(m_lossList->getLossLength(), 1);
    EXPECT_EQ(m_lossList->popLostSeq(), 1);
    CheckEmptyArray();
}

TEST_F(CSndLossListTest, InsertNegativeSeqno)
{
    cerr << "Expecting IPE message:" << endl;
    EXPECT_EQ(m_lossList->insert(1, SRT_SEQNO_NONE), 0);
    EXPECT_EQ(m_lossList->insert(SRT_SEQNO_NONE, SRT_SEQNO_NONE), 0);
    EXPECT_EQ(m_lossList->insert(SRT_SEQNO_NONE, 1), 0);
    
    CheckEmptyArray();
}

/// Insert two elements at once and pop one by one
TEST_F(CSndLossListTest, InsertPopTwoElemsRange)
{
    EXPECT_EQ(m_lossList->insert(1, 2), 2);

    EXPECT_EQ(m_lossList->getLossLength(), 2);
    EXPECT_EQ(m_lossList->popLostSeq(), 1);
    EXPECT_EQ(m_lossList->getLossLength(), 1);
    EXPECT_EQ(m_lossList->popLostSeq(), 2);
    CheckEmptyArray();
}

/// Insert 1 and 4 and pop() one by one
TEST_F(CSndLossListTest, InsertPopTwoElems)
{
    EXPECT_EQ(m_lossList->insert(1, 1), 1);
    EXPECT_EQ(m_lossList->insert(4, 4), 1);

    EXPECT_EQ(m_lossList->getLossLength(), 2);
    EXPECT_EQ(m_lossList->popLostSeq(), 1);
    EXPECT_EQ(m_lossList->getLossLength(), 1);
    EXPECT_EQ(m_lossList->popLostSeq(), 4);
    CheckEmptyArray();
}

/// Insert 1 and 2 and pop() one by one
TEST_F(CSndLossListTest, InsertPopTwoSerialElems)
{
    EXPECT_EQ(m_lossList->insert(1, 1), 1);
    EXPECT_EQ(m_lossList->insert(2, 2), 1);

    EXPECT_EQ(m_lossList->getLossLength(), 2);
    EXPECT_EQ(m_lossList->popLostSeq(), 1);
    EXPECT_EQ(m_lossList->getLossLength(), 1);
    EXPECT_EQ(m_lossList->popLostSeq(), 2);
    CheckEmptyArray();
}

/// Insert (1,2) and 4, then pop one by one
TEST_F(CSndLossListTest, InsertPopRangeAndSingle)
{
    EXPECT_EQ(m_lossList->insert(1, 2), 2);
    EXPECT_EQ(m_lossList->insert(4, 4), 1);

    EXPECT_EQ(m_lossList->getLossLength(), 3);
    EXPECT_EQ(m_lossList->popLostSeq(), 1);
    EXPECT_EQ(m_lossList->getLossLength(), 2);
    EXPECT_EQ(m_lossList->popLostSeq(), 2);
    EXPECT_EQ(m_lossList->getLossLength(), 1);
    EXPECT_EQ(m_lossList->popLostSeq(), 4);
    CheckEmptyArray();
}

/// Insert 1, 4, 2, 0, then pop
TEST_F(CSndLossListTest, InsertPopFourElems)
{
    EXPECT_EQ(m_lossList->insert(1, 1), 1);
    EXPECT_EQ(m_lossList->insert(4, 4), 1);
    EXPECT_EQ(m_lossList->insert(0, 0), 1);
    EXPECT_EQ(m_lossList->insert(2, 2), 1);

    EXPECT_EQ(m_lossList->getLossLength(), 4);
    EXPECT_EQ(m_lossList->popLostSeq(), 0);
    EXPECT_EQ(m_lossList->getLossLength(), 3);
    EXPECT_EQ(m_lossList->popLostSeq(), 1);
    EXPECT_EQ(m_lossList->getLossLength(), 2);
    EXPECT_EQ(m_lossList->popLostSeq(), 2);
    EXPECT_EQ(m_lossList->getLossLength(), 1);
    EXPECT_EQ(m_lossList->popLostSeq(), 4);
    CheckEmptyArray();
}

/// Insert (1,2) and 4, then pop one by one
TEST_F(CSndLossListTest, InsertCoalesce)
{
    EXPECT_EQ(m_lossList->insert(1, 2), 2);
    EXPECT_EQ(m_lossList->insert(4, 4), 1);
    EXPECT_EQ(m_lossList->insert(3, 3), 1);

    EXPECT_EQ(m_lossList->getLossLength(), 4);
    EXPECT_EQ(m_lossList->popLostSeq(), 1);
    EXPECT_EQ(m_lossList->getLossLength(), 3);
    EXPECT_EQ(m_lossList->popLostSeq(), 2);
    EXPECT_EQ(m_lossList->getLossLength(), 2);
    EXPECT_EQ(m_lossList->popLostSeq(), 3);
    EXPECT_EQ(m_lossList->getLossLength(), 1);
    EXPECT_EQ(m_lossList->popLostSeq(), 4);
    CheckEmptyArray();
}

///////////////////////////////////////////////////////////////////////////////
///
/// The group of tests checks remove() from different positions in the list,
///
///////////////////////////////////////////////////////////////////////////////

///
///
///
TEST_F(CSndLossListTest, BasicRemoveInListNodeHead01)
{
    EXPECT_EQ(m_lossList->insert(1, 2), 2);
    EXPECT_EQ(m_lossList->insert(4, 4), 1);
    EXPECT_EQ(m_lossList->getLossLength(), 3);
    // Remove up to element 4
    m_lossList->removeUpTo(4);
    EXPECT_EQ(m_lossList->getLossLength(), 0);
    EXPECT_EQ(m_lossList->popLostSeq(), -1);
    CheckEmptyArray();
}

TEST_F(CSndLossListTest, BasicRemoveInListNodeHead02)
{
    EXPECT_EQ(m_lossList->insert(1, 2), 2);
    EXPECT_EQ(m_lossList->insert(4, 5), 2);
    EXPECT_EQ(m_lossList->getLossLength(), 4);
    m_lossList->removeUpTo(4);
    EXPECT_EQ(m_lossList->getLossLength(), 1);
    EXPECT_EQ(m_lossList->popLostSeq(), 5);
    EXPECT_EQ(m_lossList->getLossLength(), 0);
    CheckEmptyArray();
}

TEST_F(CSndLossListTest, BasicRemoveInListNodeHead03)
{
    EXPECT_EQ(m_lossList->insert(1, 2), 2);
    EXPECT_EQ(m_lossList->insert(4, 4), 1);
    EXPECT_EQ(m_lossList->insert(8, 8), 1);
    EXPECT_EQ(m_lossList->getLossLength(), 4);
    m_lossList->removeUpTo(4);
    EXPECT_EQ(m_lossList->getLossLength(), 1);
    EXPECT_EQ(m_lossList->popLostSeq(), 8);
    CheckEmptyArray();
}

TEST_F(CSndLossListTest, BasicRemoveInListNodeHead04)
{
    EXPECT_EQ(m_lossList->insert(1, 2), 2);
    EXPECT_EQ(m_lossList->insert(4, 6), 3);
    EXPECT_EQ(m_lossList->insert(8, 8), 1);
    EXPECT_EQ(m_lossList->getLossLength(), 6);
    m_lossList->removeUpTo(4);
    EXPECT_EQ(m_lossList->getLossLength(), 3);
    EXPECT_EQ(m_lossList->popLostSeq(), 5);
    EXPECT_EQ(m_lossList->popLostSeq(), 6);
    EXPECT_EQ(m_lossList->popLostSeq(), 8);
    CheckEmptyArray();
}

TEST_F(CSndLossListTest, BasicRemoveInListNotInNodeHead01)
{
    EXPECT_EQ(m_lossList->insert(1, 2), 2);
    EXPECT_EQ(m_lossList->insert(4, 5), 2);
    EXPECT_EQ(m_lossList->getLossLength(), 4);
    m_lossList->removeUpTo(5);
    EXPECT_EQ(m_lossList->getLossLength(), 0);
    EXPECT_EQ(m_lossList->popLostSeq(), -1);
    CheckEmptyArray();
}

TEST_F(CSndLossListTest, BasicRemoveInListNotInNodeHead02)
{
    EXPECT_EQ(m_lossList->insert(1, 2), 2);
    EXPECT_EQ(m_lossList->insert(4, 5), 2);
    EXPECT_EQ(m_lossList->insert(8, 8), 1);
    EXPECT_EQ(m_lossList->getLossLength(), 5);
    m_lossList->removeUpTo(5);
    EXPECT_EQ(m_lossList->getLossLength(), 1);
    EXPECT_EQ(m_lossList->popLostSeq(), 8);
    CheckEmptyArray();
}

TEST_F(CSndLossListTest, BasicRemoveInListNotInNodeHead03)
{
    EXPECT_EQ(m_lossList->insert(1, 2), 2);
    EXPECT_EQ(m_lossList->insert(4, 8), 5);
    EXPECT_EQ(m_lossList->getLossLength(), 7);
    m_lossList->removeUpTo(5);
    EXPECT_EQ(m_lossList->getLossLength(), 3);
    EXPECT_EQ(m_lossList->popLostSeq(), 6);
    EXPECT_EQ(m_lossList->popLostSeq(), 7);
    EXPECT_EQ(m_lossList->popLostSeq(), 8);
    CheckEmptyArray();
}

TEST_F(CSndLossListTest, BasicRemoveInListNotInNodeHead04)
{
    EXPECT_EQ(m_lossList->insert(1, 2), 2);
    EXPECT_EQ(m_lossList->insert(4, 8), 5);
    EXPECT_EQ(m_lossList->insert(10, 12), 3);
    EXPECT_EQ(m_lossList->getLossLength(), 10);
    m_lossList->removeUpTo(5);
    EXPECT_EQ(m_lossList->getLossLength(), 6);
    EXPECT_EQ(m_lossList->popLostSeq(), 6);
    EXPECT_EQ(m_lossList->popLostSeq(), 7);
    EXPECT_EQ(m_lossList->popLostSeq(), 8);
    EXPECT_EQ(m_lossList->popLostSeq(), 10);
    EXPECT_EQ(m_lossList->popLostSeq(), 11);
    EXPECT_EQ(m_lossList->popLostSeq(), 12);
    CheckEmptyArray();
}

TEST_F(CSndLossListTest, BasicRemoveInListNotInNodeHead05)
{
    EXPECT_EQ(m_lossList->insert(1, 2), 2);
    EXPECT_EQ(m_lossList->insert(4, 8), 5);
    EXPECT_EQ(m_lossList->insert(10, 12), 3);
    EXPECT_EQ(m_lossList->getLossLength(), 10);
    m_lossList->removeUpTo(9);
    EXPECT_EQ(m_lossList->getLossLength(), 3);
    EXPECT_EQ(m_lossList->popLostSeq(), 10);
    EXPECT_EQ(m_lossList->popLostSeq(), 11);
    EXPECT_EQ(m_lossList->popLostSeq(), 12);
    CheckEmptyArray();
}

TEST_F(CSndLossListTest, BasicRemoveInListNotInNodeHead06)
{
    EXPECT_EQ(m_lossList->insert(1, 2), 2);
    EXPECT_EQ(m_lossList->insert(4, 8), 5);
    EXPECT_EQ(m_lossList->insert(10, 12), 3);
    EXPECT_EQ(m_lossList->getLossLength(), 10);
    m_lossList->removeUpTo(50);
    EXPECT_EQ(m_lossList->getLossLength(), 0);
    EXPECT_EQ(m_lossList->popLostSeq(), -1);
    CheckEmptyArray();
}

TEST_F(CSndLossListTest, BasicRemoveInListNotInNodeHead07)
{
    EXPECT_EQ(m_lossList->insert(1, 2), 2);
    EXPECT_EQ(m_lossList->insert(4, 8), 5);
    EXPECT_EQ(m_lossList->insert(10, 12), 3);
    EXPECT_EQ(m_lossList->getLossLength(), 10);
    m_lossList->removeUpTo(-50);
    EXPECT_EQ(m_lossList->getLossLength(), 10);
    EXPECT_EQ(m_lossList->popLostSeq(), 1);
    EXPECT_EQ(m_lossList->popLostSeq(), 2);
    EXPECT_EQ(m_lossList->popLostSeq(), 4);
    EXPECT_EQ(m_lossList->popLostSeq(), 5);
    EXPECT_EQ(m_lossList->popLostSeq(), 6);
    EXPECT_EQ(m_lossList->popLostSeq(), 7);
    EXPECT_EQ(m_lossList->popLostSeq(), 8);
    EXPECT_EQ(m_lossList->popLostSeq(), 10);
    EXPECT_EQ(m_lossList->popLostSeq(), 11);
    EXPECT_EQ(m_lossList->popLostSeq(), 12);
    CheckEmptyArray();
}

TEST_F(CSndLossListTest, BasicRemoveInListNotInNodeHead08)
{
    EXPECT_EQ(m_lossList->insert(1, 2), 2);
    EXPECT_EQ(m_lossList->insert(5, 6), 2);
    EXPECT_EQ(m_lossList->getLossLength(), 4);
    m_lossList->removeUpTo(5);
    EXPECT_EQ(m_lossList->getLossLength(), 1);
    m_lossList->removeUpTo(6);
    EXPECT_EQ(m_lossList->getLossLength(), 0);
    EXPECT_EQ(m_lossList->popLostSeq(), -1);
    CheckEmptyArray();
}

TEST_F(CSndLossListTest, BasicRemoveInListNotInNodeHead09)
{
    EXPECT_EQ(m_lossList->insert(1, 2), 2);
    EXPECT_EQ(m_lossList->insert(5, 6), 2);
    EXPECT_EQ(m_lossList->getLossLength(), 4);
    m_lossList->removeUpTo(5);
    EXPECT_EQ(m_lossList->getLossLength(), 1);
    EXPECT_EQ(m_lossList->insert(1, 2), 2);
    m_lossList->removeUpTo(6);
    EXPECT_EQ(m_lossList->getLossLength(), 0);
    EXPECT_EQ(m_lossList->popLostSeq(), -1);
    CheckEmptyArray();
}

TEST_F(CSndLossListTest, BasicRemoveInListNotInNodeHead10)
{
    EXPECT_EQ(m_lossList->insert(1, 2), 2);
    EXPECT_EQ(m_lossList->insert(5, 6), 2);
    EXPECT_EQ(m_lossList->insert(10, 10), 1);
    EXPECT_EQ(m_lossList->getLossLength(), 5);
    m_lossList->removeUpTo(5);
    EXPECT_EQ(m_lossList->getLossLength(), 2);
    EXPECT_EQ(m_lossList->insert(1, 2), 2);
    m_lossList->removeUpTo(7);
    EXPECT_EQ(m_lossList->getLossLength(), 1);
    EXPECT_EQ(m_lossList->popLostSeq(), 10);
    CheckEmptyArray();
}

TEST_F(CSndLossListTest, BasicRemoveInListNotInNodeHead11)
{
    EXPECT_EQ(m_lossList->insert(1, 2), 2);
    EXPECT_EQ(m_lossList->insert(5, 6), 2);
    EXPECT_EQ(m_lossList->getLossLength(), 4);
    m_lossList->removeUpTo(5);
    EXPECT_EQ(m_lossList->getLossLength(), 1);
    EXPECT_EQ(m_lossList->insert(1, 2), 2);
    m_lossList->removeUpTo(7);
    EXPECT_EQ(m_lossList->getLossLength(), 0);
    EXPECT_EQ(m_lossList->popLostSeq(), -1);
    CheckEmptyArray();
}

/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
TEST_F(CSndLossListTest, InsertRemoveInsert01)
{
    EXPECT_EQ(m_lossList->insert(1, 2), 2);
    EXPECT_EQ(m_lossList->insert(5, 6), 2);
    EXPECT_EQ(m_lossList->getLossLength(), 4);
    m_lossList->removeUpTo(5);
    EXPECT_EQ(m_lossList->getLossLength(), 1);
    EXPECT_EQ(m_lossList->insert(1, 2), 2);
    m_lossList->removeUpTo(6);
    EXPECT_EQ(m_lossList->getLossLength(), 0);
    EXPECT_EQ(m_lossList->popLostSeq(), -1);
    CheckEmptyArray();
}

/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
TEST_F(CSndLossListTest, InsertHead01)
{
    EXPECT_EQ(m_lossList->insert(1, 2), 2);
    EXPECT_EQ(m_lossList->getLossLength(), 2);
    EXPECT_EQ(m_lossList->popLostSeq(), 1);
    EXPECT_EQ(m_lossList->getLossLength(), 1);
    EXPECT_EQ(m_lossList->popLostSeq(), 2);
    CheckEmptyArray();
}

TEST_F(CSndLossListTest, InsertHead02)
{
    EXPECT_EQ(m_lossList->insert(1, 1), 1);
    EXPECT_EQ(m_lossList->getLossLength(), 1);
    EXPECT_EQ(m_lossList->popLostSeq(), 1);
    CheckEmptyArray();
}

TEST_F(CSndLossListTest, InsertHeadIncrease01)
{
    EXPECT_EQ(m_lossList->insert(1, 1), 1);
    EXPECT_EQ(m_lossList->getLossLength(), 1);
    EXPECT_EQ(m_lossList->insert(2, 2), 1);
    EXPECT_EQ(m_lossList->getLossLength(), 2);
    EXPECT_EQ(m_lossList->popLostSeq(), 1);
    EXPECT_EQ(m_lossList->getLossLength(), 1);
    EXPECT_EQ(m_lossList->popLostSeq(), 2);
    CheckEmptyArray();
}

TEST_F(CSndLossListTest, InsertHeadOverlap01)
{
    EXPECT_EQ(m_lossList->insert(1, 5), 5);
    EXPECT_EQ(m_lossList->getLossLength(), 5);
    EXPECT_EQ(m_lossList->insert(6, 8), 3);
    EXPECT_EQ(m_lossList->getLossLength(), 8);
    EXPECT_EQ(m_lossList->insert(2, 10), 2);
    EXPECT_EQ(m_lossList->getLossLength(), 10);
    for (int i = 1; i < 11; i++)
    {
        EXPECT_EQ(m_lossList->popLostSeq(), i);
        EXPECT_EQ(m_lossList->getLossLength(), 10 - i);
    }

    CheckEmptyArray();
}

TEST_F(CSndLossListTest, InsertHeadOverlap02)
{
    EXPECT_EQ(m_lossList->insert(1, 5), 5);
    EXPECT_EQ(m_lossList->getLossLength(), 5);
    EXPECT_EQ(m_lossList->insert(6, 8), 3);
    EXPECT_EQ(m_lossList->getLossLength(), 8);
    EXPECT_EQ(m_lossList->insert(2, 7), 0);
    EXPECT_EQ(m_lossList->getLossLength(), 8);
    EXPECT_EQ(m_lossList->insert(5, 5), 0);
    EXPECT_EQ(m_lossList->getLossLength(), 8);

    for (int i = 1; i < 9; i++)
    {
        EXPECT_EQ(m_lossList->popLostSeq(), i);
        EXPECT_EQ(m_lossList->getLossLength(), 8 - i);
    }

    CheckEmptyArray();
}

TEST_F(CSndLossListTest, InsertHeadNegativeOffset01)
{
    EXPECT_EQ(m_lossList->insert(10000000, 10000000), 1);
    EXPECT_EQ(m_lossList->insert(10000001, 10000001), 1);
    EXPECT_EQ(m_lossList->getLossLength(), 2);

    // The offset of the sequence number being added does not fit
    // into the size of the loss list, it must be ignored.
    // Normally this situation should not happen.
    cerr << "Expecting IPE message:" << endl;
    EXPECT_EQ(m_lossList->insert(1, 1), 0);
    EXPECT_EQ(m_lossList->getLossLength(), 2);
    EXPECT_EQ(m_lossList->popLostSeq(), 10000000);
    EXPECT_EQ(m_lossList->getLossLength(), 1);
    EXPECT_EQ(m_lossList->popLostSeq(), 10000001);

    CheckEmptyArray();
}

// Check the part of the loss report the can fit into the list
// goes into the list.
TEST_F(CSndLossListTest, InsertHeadNegativeOffset02)
{
    const int32_t head_seqno = 10000000;
    EXPECT_EQ(m_lossList->insert(head_seqno,     head_seqno), 1);
    EXPECT_EQ(m_lossList->insert(head_seqno + 1, head_seqno + 1), 1);
    EXPECT_EQ(m_lossList->getLossLength(), 2);

    // The offset of the sequence number being added does not fit
    // into the size of the loss list, it must be ignored.
    // Normally this situation should not happen.

    const int32_t outofbound_seqno = head_seqno - CSndLossListTest::SIZE;
    EXPECT_EQ(m_lossList->insert(outofbound_seqno - 1, outofbound_seqno + 1), 3);
    EXPECT_EQ(m_lossList->getLossLength(), 5);
    EXPECT_EQ(m_lossList->popLostSeq(), outofbound_seqno - 1);
    EXPECT_EQ(m_lossList->getLossLength(), 4);
    EXPECT_EQ(m_lossList->popLostSeq(), outofbound_seqno);
    EXPECT_EQ(m_lossList->getLossLength(), 3);
    EXPECT_EQ(m_lossList->popLostSeq(), outofbound_seqno + 1);
    EXPECT_EQ(m_lossList->getLossLength(), 2);
    EXPECT_EQ(m_lossList->popLostSeq(), 10000000);
    EXPECT_EQ(m_lossList->getLossLength(), 1);
    EXPECT_EQ(m_lossList->popLostSeq(), 10000001);

    CheckEmptyArray();
}

/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
TEST_F(CSndLossListTest, InsertFullListCoalesce)
{
    for (int i = 1; i <= CSndLossListTest::SIZE; i++)
        EXPECT_EQ(m_lossList->insert(i, i), 1);
    EXPECT_EQ(m_lossList->getLossLength(), CSndLossListTest::SIZE);
    // Inserting additional element: 1 item more than list size.
    // Given all elements coalesce into one entry, there is a place to insert it,
    // but sequence span now exceeds list size.
    EXPECT_EQ(m_lossList->insert(CSndLossListTest::SIZE + 1, CSndLossListTest::SIZE + 1), 0);
    EXPECT_EQ(m_lossList->getLossLength(), CSndLossListTest::SIZE);
    for (int i = 1; i <= CSndLossListTest::SIZE; i++)
    {
        EXPECT_EQ(m_lossList->popLostSeq(), i);
        EXPECT_EQ(m_lossList->getLossLength(), CSndLossListTest::SIZE - i);
    }
    EXPECT_EQ(m_lossList->popLostSeq(), -1);
    EXPECT_EQ(m_lossList->getLossLength(), 0);

    CheckEmptyArray();
}

TEST_F(CSndLossListTest, InsertFullListNoCoalesce)
{
    // We will insert each element with a gap of one elements.
    // This should lead to having space for only [i; SIZE] sequence numbers.
    for (int i = 1; i <= CSndLossListTest::SIZE / 2; i++)
        EXPECT_EQ(m_lossList->insert(2 * i, 2 * i), 1);

    // At this point the list has every second element empty
    // [0]:taken, [1]: empty, [2]: taken, [3]: empty, ...
    EXPECT_EQ(m_lossList->getLossLength(), CSndLossListTest::SIZE / 2);

    // Inserting additional element out of the list span must fail.
    const int seqno1 = CSndLossListTest::SIZE + 2;
    EXPECT_EQ(m_lossList->insert(seqno1, seqno1), 0);

    // There should however be a place for one element right after the last inserted one.
    const int seqno_last = CSndLossListTest::SIZE + 1;
    EXPECT_EQ(m_lossList->insert(seqno_last, seqno_last), 1);

    const int initial_length = m_lossList->getLossLength();
    EXPECT_EQ(initial_length, CSndLossListTest::SIZE / 2 + 1);
    for (int i = 1; i <= CSndLossListTest::SIZE / 2; i++)
    {
        EXPECT_EQ(m_lossList->popLostSeq(), 2 * i);
        EXPECT_EQ(m_lossList->getLossLength(), initial_length - i);
    }
    EXPECT_EQ(m_lossList->popLostSeq(), seqno_last);
    EXPECT_EQ(m_lossList->popLostSeq(), -1);
    EXPECT_EQ(m_lossList->getLossLength(), 0);

    CheckEmptyArray();
}

TEST_F(CSndLossListTest, InsertFullListNegativeOffset)
{
    for (int i = 10000000; i < 10000000 + CSndLossListTest::SIZE; i++)
        m_lossList->insert(i, i);
    EXPECT_EQ(m_lossList->getLossLength(), CSndLossListTest::SIZE);
    m_lossList->insert(1, CSndLossListTest::SIZE + 1);
    EXPECT_EQ(m_lossList->getLossLength(), CSndLossListTest::SIZE);
    for (int i = 10000000; i < 10000000 + CSndLossListTest::SIZE; i++)
    {
        EXPECT_EQ(m_lossList->popLostSeq(), i);
        EXPECT_EQ(m_lossList->getLossLength(), CSndLossListTest::SIZE - (i - 10000000 + 1));
    }
    EXPECT_EQ(m_lossList->popLostSeq(), -1);
    EXPECT_EQ(m_lossList->getLossLength(), 0);

    CheckEmptyArray();
}

TEST_F(CSndLossListTest, InsertPositiveOffsetTooFar)
{
    const int32_t head_seqno = 1000;
    EXPECT_EQ(m_lossList->insert(head_seqno, head_seqno), 1);
    EXPECT_EQ(m_lossList->getLossLength(), 1);

    // The offset of the sequence number being added does not fit
    // into the size of the loss list, it must be ignored.
    // Normally this situation should not happen.

    const int32_t outofbound_seqno = head_seqno + CSndLossListTest::SIZE;
    m_lossList->insert(outofbound_seqno, outofbound_seqno);

    const int32_t outofbound_seqno2 = head_seqno + 2 * CSndLossListTest::SIZE;
    m_lossList->insert(outofbound_seqno2, outofbound_seqno2);
}

/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
TEST_F(CSndLossListTest, InsertNoUpdateElement01)
{
    EXPECT_EQ(m_lossList->insert(0, 1), 2);
    EXPECT_EQ(m_lossList->insert(3, 5), 3);
    m_lossList->removeUpTo(3); // Remove all to seq no 3
    EXPECT_EQ(m_lossList->insert(4, 5), 0); // Element not updated
    EXPECT_EQ(m_lossList->getLossLength(), 2);
    EXPECT_EQ(m_lossList->popLostSeq(), 4);
    EXPECT_EQ(m_lossList->popLostSeq(), 5);
}

TEST_F(CSndLossListTest, InsertNoUpdateElement03)
{
    EXPECT_EQ(m_lossList->insert(1, 5), 5);
    EXPECT_EQ(m_lossList->getLossLength(), 5);
    EXPECT_EQ(m_lossList->insert(6, 8), 3);
    EXPECT_EQ(m_lossList->getLossLength(), 8);
    EXPECT_EQ(m_lossList->insert(2, 5), 0);
    EXPECT_EQ(m_lossList->getLossLength(), 8);
}

/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
TEST_F(CSndLossListTest, InsertUpdateElement01)
{
    EXPECT_EQ(m_lossList->insert(1, 5), 5);
    EXPECT_EQ(m_lossList->getLossLength(), 5);
    EXPECT_EQ(m_lossList->insert(1, 8), 3);
    EXPECT_EQ(m_lossList->getLossLength(), 8);
    EXPECT_EQ(m_lossList->insert(2, 5), 0);
    EXPECT_EQ(m_lossList->getLossLength(), 8);
}

///////////////////////////////////////////////////////////////////////////////
///
/// Deadline-aware retransmission support
///
///////////////////////////////////////////////////////////////////////////////

using srt::sync::steady_clock;

namespace {
steady_clock::time_point deadline_at(int64_t ms_since_epoch)
{
    return steady_clock::time_point() + srt::sync::milliseconds_from(ms_since_epoch);
}
}

TEST_F(CSndLossListTest, DeadlineInsertAndPopSingle)
{
    const steady_clock::time_point dl = deadline_at(1000);
    EXPECT_EQ(m_lossList->insert(10, 12, dl), 3);

    EXPECT_EQ(m_lossList->getDeadline(10), dl);
    EXPECT_EQ(m_lossList->getDeadline(11), dl);
    EXPECT_EQ(m_lossList->getDeadline(12), dl);

    steady_clock::time_point popped_dl;
    EXPECT_EQ(m_lossList->popLostSeq(popped_dl), 10);
    EXPECT_EQ(popped_dl, dl);
    EXPECT_EQ(m_lossList->popLostSeq(popped_dl), 11);
    EXPECT_EQ(popped_dl, dl);
    EXPECT_EQ(m_lossList->popLostSeq(popped_dl), 12);
    EXPECT_EQ(popped_dl, dl);
    CheckEmptyArray();
}

TEST_F(CSndLossListTest, DeadlineDefaultIsZero)
{
    EXPECT_EQ(m_lossList->insert(1, 3), 3);
    EXPECT_TRUE(srt::sync::is_zero(m_lossList->getDeadline(1)));
    EXPECT_TRUE(srt::sync::is_zero(m_lossList->getDeadline(2)));
    EXPECT_TRUE(srt::sync::is_zero(m_lossList->getDeadline(3)));

    steady_clock::time_point popped_dl = deadline_at(5);
    EXPECT_EQ(m_lossList->popLostSeq(popped_dl), 1);
    EXPECT_TRUE(srt::sync::is_zero(popped_dl));
}

TEST_F(CSndLossListTest, DeadlineReinsertDoesNotOverwrite)
{
    const steady_clock::time_point dl1 = deadline_at(1000);
    const steady_clock::time_point dl2 = deadline_at(2000);

    EXPECT_EQ(m_lossList->insert(5, 7, dl1), 3);
    // Re-report part of the range with a different deadline: existing
    // deadlines must be preserved, only the new sequence gets dl2.
    EXPECT_EQ(m_lossList->insert(6, 8, dl2), 1);

    EXPECT_EQ(m_lossList->getDeadline(5), dl1);
    EXPECT_EQ(m_lossList->getDeadline(6), dl1);
    EXPECT_EQ(m_lossList->getDeadline(7), dl1);
    EXPECT_EQ(m_lossList->getDeadline(8), dl2);
}

TEST_F(CSndLossListTest, DeadlineRemoveInsideRange)
{
    const steady_clock::time_point dl = deadline_at(4242);
    EXPECT_EQ(m_lossList->insert(10, 14, dl), 5);

    EXPECT_TRUE(m_lossList->remove(12));
    EXPECT_EQ(m_lossList->getLossLength(), 4);
    EXPECT_FALSE(m_lossList->find(12));
    EXPECT_TRUE(m_lossList->find(11));
    EXPECT_TRUE(m_lossList->find(13));

    // Remaining sequences keep their deadlines.
    EXPECT_EQ(m_lossList->getDeadline(10), dl);
    EXPECT_EQ(m_lossList->getDeadline(11), dl);
    EXPECT_EQ(m_lossList->getDeadline(13), dl);
    EXPECT_EQ(m_lossList->getDeadline(14), dl);
    // Cleared slot reads as "no deadline".
    EXPECT_TRUE(srt::sync::is_zero(m_lossList->getDeadline(12)));

    // Removing an unknown sequence fails.
    EXPECT_FALSE(m_lossList->remove(12));
    EXPECT_FALSE(m_lossList->remove(9));
}

TEST_F(CSndLossListTest, DeadlineRemoveRangeHeadAndTail)
{
    const steady_clock::time_point dl = deadline_at(777);
    EXPECT_EQ(m_lossList->insert(20, 23, dl), 4);

    EXPECT_TRUE(m_lossList->remove(20)); // range head: node shifts
    EXPECT_FALSE(m_lossList->find(20));
    EXPECT_TRUE(m_lossList->find(21));
    EXPECT_EQ(m_lossList->getDeadline(21), dl);
    EXPECT_EQ(m_lossList->getLossLength(), 3);

    EXPECT_TRUE(m_lossList->remove(23)); // range tail: node shrinks
    EXPECT_FALSE(m_lossList->find(23));
    EXPECT_TRUE(m_lossList->find(22));
    EXPECT_EQ(m_lossList->getDeadline(22), dl);
    EXPECT_EQ(m_lossList->getLossLength(), 2);

    steady_clock::time_point popped_dl;
    EXPECT_EQ(m_lossList->popLostSeq(popped_dl), 21);
    EXPECT_EQ(popped_dl, dl);
    EXPECT_EQ(m_lossList->popLostSeq(popped_dl), 22);
    EXPECT_EQ(popped_dl, dl);
    CheckEmptyArray();
}

TEST_F(CSndLossListTest, DeadlineSnapshot)
{
    const steady_clock::time_point dl1 = deadline_at(100);
    const steady_clock::time_point dl2 = deadline_at(200);

    EXPECT_EQ(m_lossList->insert(1, 2, dl1), 2);
    EXPECT_EQ(m_lossList->insert(5, 5, dl2), 1);

    std::vector<CSndLossList::LossEntry> entries;
    m_lossList->snapshot(entries);
    ASSERT_EQ(entries.size(), 3u);

    // Snapshot order follows list order (sequence order).
    EXPECT_EQ(entries[0].seq, 1);
    EXPECT_EQ(entries[0].deadline, dl1);
    EXPECT_EQ(entries[1].seq, 2);
    EXPECT_EQ(entries[1].deadline, dl1);
    EXPECT_EQ(entries[2].seq, 5);
    EXPECT_EQ(entries[2].deadline, dl2);
}

TEST_F(CSndLossListTest, DeadlineSurvivesSeqnoWrapping)
{
    // Allocate a small list and exercise the circular sequence space.
    CSndLossList wrap_list(16);

    const int32_t base = INT32_MAX - 3;
    const int32_t end  = CSeqNo::incseq(base, 6);
    const steady_clock::time_point dl = deadline_at(3333);

    // Range straddles the INT32_MAX -> INT32_MIN wrap.
    EXPECT_EQ(wrap_list.insert(base, end, dl), 7);
    EXPECT_EQ(wrap_list.getLossLength(), 7);

    std::vector<CSndLossList::LossEntry> entries;
    wrap_list.snapshot(entries);
    ASSERT_EQ(entries.size(), 7u);
    for (size_t i = 0; i < entries.size(); ++i)
    {
        EXPECT_EQ(CSeqNo::seqoff(base, entries[i].seq), static_cast<int>(i));
        EXPECT_EQ(entries[i].deadline, dl);
        EXPECT_EQ(wrap_list.getDeadline(entries[i].seq), dl);
    }

    // Remove a wrapped sequence from the middle of the range.
    const int32_t wrapped_mid = CSeqNo::incseq(base, 4);
    EXPECT_TRUE(wrap_list.remove(wrapped_mid));
    EXPECT_FALSE(wrap_list.find(wrapped_mid));
    EXPECT_EQ(wrap_list.getLossLength(), 6);

    steady_clock::time_point popped_dl;
    int32_t expected = base;
    for (int i = 0; i < 6; ++i)
    {
        if (expected == wrapped_mid)
            expected = CSeqNo::incseq(expected);
        EXPECT_EQ(wrap_list.popLostSeq(popped_dl), expected);
        EXPECT_EQ(popped_dl, dl);
        expected = CSeqNo::incseq(expected);
    }
    EXPECT_EQ(wrap_list.popLostSeq(popped_dl), SRT_SEQNO_NONE);
}

TEST_F(CSndLossListTest, DeadlinePopRangeHeadKeepsRest)
{
    const steady_clock::time_point dl = deadline_at(99);
    EXPECT_EQ(m_lossList->insert(30, 33, dl), 4);

    steady_clock::time_point popped_dl;
    EXPECT_EQ(m_lossList->popLostSeq(popped_dl), 30);
    EXPECT_EQ(popped_dl, dl);

    // The node shifted one slot: remaining deadlines must stay attached.
    EXPECT_EQ(m_lossList->getDeadline(31), dl);
    EXPECT_EQ(m_lossList->getDeadline(32), dl);
    EXPECT_EQ(m_lossList->getDeadline(33), dl);
    EXPECT_EQ(m_lossList->getLossLength(), 3);
}

TEST_F(CSndLossListTest, DeadlineRemoveUpToClearsFreedSlots)
{
    const steady_clock::time_point dl = deadline_at(555);
    EXPECT_EQ(m_lossList->insert(10, 15, dl), 6);
    EXPECT_EQ(m_lossList->getLossLength(), 6);

    // ACK through 13: slots for sequences 10..13 are released and their
    // deadline entries must be cleared so slot reuse starts clean.
    m_lossList->removeUpTo(13);
    EXPECT_EQ(m_lossList->getLossLength(), 2);
    EXPECT_EQ(m_lossList->getDeadline(14), dl);
    EXPECT_EQ(m_lossList->getDeadline(15), dl);

    // Re-insert the old region without a deadline: it must remain deadline
    // free rather than inherit the stale values from the freed slots.
    EXPECT_EQ(m_lossList->insert(10, 12), 3);
    EXPECT_TRUE(srt::sync::is_zero(m_lossList->getDeadline(10)));
    EXPECT_TRUE(srt::sync::is_zero(m_lossList->getDeadline(11)));
    EXPECT_TRUE(srt::sync::is_zero(m_lossList->getDeadline(12)));
    // Previously surviving entries keep their deadline.
    EXPECT_EQ(m_lossList->getDeadline(14), dl);
    EXPECT_EQ(m_lossList->getDeadline(15), dl);
}

TEST_F(CSndLossListTest, DeadlineRemoveAllEmptiesList)
{
    const steady_clock::time_point dl = deadline_at(1);
    EXPECT_EQ(m_lossList->insert(5, 9, dl), 5);
    m_lossList->removeUpTo(9);
    EXPECT_EQ(m_lossList->getLossLength(), 0);
    CheckEmptyArray();

    // The list is usable again after a full removal.
    const steady_clock::time_point dl2 = deadline_at(2);
    EXPECT_EQ(m_lossList->insert(5, 6, dl2), 2);
    EXPECT_EQ(m_lossList->getDeadline(5), dl2);
    EXPECT_EQ(m_lossList->getDeadline(6), dl2);
}
