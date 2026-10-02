// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//-----------------------------------------------------------------------------

//
//  Description:
//      Implements a collection of different list classes, each designed
//     for specialized usage.
//

#include "Pch.h"
#include "List.h"

// Meters ----------------------------------------------------------------------
MtDefine(List, Utilities, "List")

//------------------------------------------------------------------------------
__range(0,INT_MAX) int
RawList::GetSize() const
{
    int cItems = 0;
    ListNode * pCur = m_pHead;
    while (pCur != NULL) {
        cItems++;
        pCur = pCur->pNext;
    }

    return cItems;
}


//------------------------------------------------------------------------------
__ecount_opt(1) ListNode *
RawList::GetTail() const
{
    ListNode * pCur = m_pHead;
    while (pCur != NULL) {
        if (pCur->pNext == NULL) {
            return pCur;
        }
        pCur = pCur->pNext;
    }

    return NULL;
}


//------------------------------------------------------------------------------
__ecount_opt(1) ListNode *
RawList::GetAt(
    __range(0,INT_MAX) int idxItem
    ) const
{
    ListNode * pCur = m_pHead;
    while ((pCur != NULL) && (idxItem-- > 0)) {
        pCur = pCur->pNext;
    }

    return pCur;
}


//------------------------------------------------------------------------------
void
RawList::AddHead(
    __inout_ecount(1) ListNode *pNode
    )
{
    pNode->pPrev = NULL;
    pNode->pNext = m_pHead;

    if (m_pHead != NULL) {
        m_pHead->pPrev = pNode;
    }

    m_pHead = pNode;
}


//------------------------------------------------------------------------------
void
RawList::AddTail(
    __inout_ecount(1) ListNode *pNode
    )
{
    ListNode * pTail = GetTail();
    if (pTail != NULL) {
        pNode->pPrev    = pTail;
        pTail->pNext    = pNode;
    } else {
        m_pHead = pNode;
    }
}


//------------------------------------------------------------------------------
void
RawList::InsertAfter(
    __inout_ecount(1)     ListNode *pInsert, 
    __inout_ecount_opt(1) ListNode *pBefore
    )
{
    if ((pBefore == NULL) || IsEmpty()) {
        AddHead(pInsert);
    } else {
        pInsert->pNext = pBefore->pNext;
        if (pInsert->pNext != NULL) {
            pInsert->pNext->pPrev = pInsert;
        }
        pBefore->pNext = pInsert;
    }
}


//------------------------------------------------------------------------------
void
RawList::InsertBefore(
    __inout_ecount(1)     ListNode * pInsert, 
    __inout_ecount_opt(1) ListNode * pAfter
    )
{
    if ((pAfter == m_pHead) || (pAfter == NULL) || IsEmpty()) {
        AddHead(pInsert);
    } else {
        pInsert->pPrev = pAfter->pPrev;
        pInsert->pNext = pAfter;

        AssertMsg(pInsert->pPrev != NULL, "Must have previous or else is head");

        pInsert->pPrev->pNext = pInsert;
        pAfter->pPrev = pInsert;
    }
}


//------------------------------------------------------------------------------
void
RawList::Unlink(
    __inout_ecount(1) ListNode *pNode
    )
{
    AssertMsg(!IsEmpty(), "List must have nodes to unlink");

    ListNode * pPrev = pNode->pPrev;
    ListNode * pNext = pNode->pNext;

    if (pPrev != NULL) {
        pPrev->pNext = pNext;
    }

    if (pNext != NULL) {
        pNext->pPrev = pPrev;
    }

    if (m_pHead == pNode) {
        m_pHead = pNext;
    }

    pNode->pPrev = NULL;
    pNode->pNext = NULL;
}


//------------------------------------------------------------------------------
__ecount(1) ListNode *
RawList::UnlinkHead()
{
    AssertMsg(!IsEmpty(), "List must have nodes to unlink");

    ListNode * pHead = m_pHead;

    m_pHead = pHead->pNext;
    if (m_pHead != NULL) {
        m_pHead->pPrev = NULL;
    }

    pHead->pNext = NULL;
    AssertMsg(pHead->pPrev == NULL, "Check");

    return pHead;
}


//------------------------------------------------------------------------------
__ecount(1) ListNode *
RawList::UnlinkTail()
{
    AssertMsg(!IsEmpty(), "List must have nodes to unlink");

    ListNode * pTail = GetTail();
    if (pTail != NULL) {
        if (m_pHead == pTail) {
            m_pHead = NULL;
        } else {
            AssertMsg(pTail->pPrev != NULL, "If not head, must have prev");            
            pTail->pPrev->pNext = NULL;
        }
        pTail->pPrev = NULL;
        AssertMsg(pTail->pNext == NULL, "Check");
    }

    return pTail;
}


//------------------------------------------------------------------------------

//
// NOTE-2006/02/28-chrisra SAL gets confused by this -1.
//
// This is a bug in SAL and will currently misreport warning 2061.  This should
// be ignored.
// 
__range(-1,INT_MAX) int
RawList::Find(
    __in_ecount_opt(1) const ListNode *pNode
    ) const
{
    int cItems = -1;
    ListNode * pCur = m_pHead;
    while (pCur != NULL) {
        cItems++;
        if (pCur != pNode) {
            pCur = pCur->pNext;
        } else {
            break;
        }
    }

    return cItems;
}


