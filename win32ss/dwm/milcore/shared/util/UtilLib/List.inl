// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//------------------------------------------------------------------------------

//
//  Description:
//      Contains Generic List utilities
//


#ifndef UTILLIB__List_inl__INCLUDED
#define UTILLIB__List_inl__INCLUDED
#pragma once


//------------------------------------------------------------------------------
// Generic List Utilities
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
template <class T>
bool
IsLoop(
    __in_ecount_opt(1) const T *pEntry
    )
{
    if (pEntry == NULL) {
        return false;
    }

    const T * p1 = pEntry;
    const T * p2 = pEntry->pNext;

    while (1) {
        if (p2 == NULL) {
            return false;
        } else if (p1 == p2) {
            return true;
        }

        p2 = p2->pNext;
        if (p2 == NULL) {
            return false;
        } else if (p1 == p2) {
            return true;
        }

        p2 = p2->pNext;
        p1 = p1->pNext;
    }
}


//------------------------------------------------------------------------------
template <class T>
void
ReverseSingleList(
    __deref_inout_ecount(1) T * &pEntry
    )
{
    T * pPrev, * pNext;

    pPrev = NULL;
    while (pEntry != NULL) {
        pNext = static_cast<T *> (pEntry->pNext);
        pEntry->pNext = pPrev;
        pPrev = pEntry;
        pEntry = pNext;
    }

    if (pEntry == NULL) {
        pEntry = pPrev;
    }
}


//------------------------------------------------------------------------------
// class ListNodeT<T>
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
template <class T>
__ecount_opt(1) T *
ListNodeT<T>::GetNext() const
{
    return static_cast<T *>(pNext);
}


//------------------------------------------------------------------------------
template <class T>
inline __ecount_opt(1) T *
ListNodeT<T>::GetPrev() const
{
    return static_cast<T *>(pPrev);
}


//------------------------------------------------------------------------------
// class RawList
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
inline
RawList::RawList()
{
    m_pHead = NULL;
}


//------------------------------------------------------------------------------
inline
RawList::~RawList()
{
    AssertConstMsg(m_pHead == NULL, "List data was not cleaned up");
}


//------------------------------------------------------------------------------
inline BOOL
RawList::IsEmpty() const
{
    return m_pHead == NULL;
}


//------------------------------------------------------------------------------
inline __ecount_opt(1) ListNode *
RawList::GetHead() const
{
    return m_pHead;
}


//------------------------------------------------------------------------------
inline void
RawList::Extract(
    __inout_ecount(1) RawList &lstSrc
    )
{
    AssertConstMsg(IsEmpty(), "Destination list must be empty to receive a new list");

    m_pHead         = lstSrc.m_pHead;
    lstSrc.m_pHead  = NULL;
}


//------------------------------------------------------------------------------
inline void
RawList::MarkEmpty()
{
    m_pHead = NULL;
}


//------------------------------------------------------------------------------
inline void
RawList::Add(
    __inout_ecount(1) ListNode *pNode
    )
{
    AddHead(pNode);
}


/***************************************************************************\
*****************************************************************************
*
* class List<T>
*
*****************************************************************************
\***************************************************************************/

//------------------------------------------------------------------------------
template <class T>
inline
List<T>::~List()
{
    //
    // NOTE: We do not call RemoveAll() from the destructor because this causes
    // too many bugs.  Components are not always aware that List<> is deleting
    // its members in the destructor.
    //
    // Instead, we'll warn if the list is not empty and just unlink everything.

    if (!IsEmpty()) {
        RIP("WARNING: List<> is not empty\n");
        UnlinkAll();
    }
}


//------------------------------------------------------------------------------
template <class T>
inline __ecount_opt(1) T *
List<T>::GetHead() const
{
    return static_cast<T *> (RawList::GetHead());
}


//------------------------------------------------------------------------------
template <class T>
inline __ecount_opt(1) T *
List<T>::GetTail() const
{
    return static_cast<T *> (RawList::GetTail());
}


//------------------------------------------------------------------------------
template <class T>
inline __ecount_opt(1) T *
List<T>::GetAt(
    __range(0,INT_MAX) int idxItem
    ) const
{
    return static_cast<T *> (RawList::GetAt(idxItem));
}


//------------------------------------------------------------------------------
template <class T>
inline void
List<T>::Extract(
    __inout_ecount(1) List<T> &lstSrc
    )
{
    RawList::Extract(lstSrc);
}


//------------------------------------------------------------------------------
template <class T>
inline __ecount_opt(1) T *
List<T>::Extract()
{
    T * pHead = static_cast<T *> (m_pHead);
    m_pHead = NULL;
    return pHead;
}


//------------------------------------------------------------------------------
template <class T>
inline void
List<T>::Add(
    __inout_ecount(1) T *pNode
    )
{
    RawList::Add(pNode);
}


//------------------------------------------------------------------------------
template <class T>
inline void
List<T>::AddHead(
    __inout_ecount(1) T *pNode
    )
{
    RawList::AddHead(pNode);
}


//------------------------------------------------------------------------------
template <class T>
inline void
List<T>::AddTail(
    __inout_ecount(1) T *pNode
    )
{
    RawList::AddTail(pNode);
}


//------------------------------------------------------------------------------
template <class T>
inline void
List<T>::InsertAfter(
    __inout_ecount(1)     T *pInsert,
    __inout_ecount_opt(1) T *pBefore
    )
{
    RawList::InsertAfter(
        pInsert, 
        pBefore
        );
}


//------------------------------------------------------------------------------
template <class T>
inline void
List<T>::InsertBefore(
    __inout_ecount(1)     T *pInsert, 
    __inout_ecount_opt(1) T *pAfter
    )
{
    RawList::InsertBefore(
        pInsert, 
        pAfter
        );
}


//------------------------------------------------------------------------------
template <class T>
inline void
List<T>::Remove(
    __inout_ecount(1) T *pNode
    )
{
    Unlink(pNode);
    DoClientDelete<T>(pNode);
}


//------------------------------------------------------------------------------
template <class T>
inline BOOL
List<T>::RemoveAt(
    __range(0,INT_MAX) int idxItem
    )
{
    ListNode * pCur = GetAt(idxItem);
    if (pCur != NULL) {
        Remove(pCur);
        return TRUE;
    } else {
        return FALSE;
    }
}


//------------------------------------------------------------------------------
template <class T>
inline void
List<T>::RemoveAll()
{
    //
    // When removing each item, need to typecase to T so that delete can do the
    // right thing and call the correct destructor.
    //

    while (m_pHead != NULL) {
        ListNode * pNext = m_pHead->pNext;
        m_pHead->pPrev= NULL;
        T * pHead = (T *) m_pHead;
        DoClientDelete<T>(pHead);
        m_pHead = pNext;
    }
}


//------------------------------------------------------------------------------
template <class T>
inline void
List<T>::Unlink(
    __inout_ecount(1) T *pNode
    )
{
    RawList::Unlink(pNode);
}


//------------------------------------------------------------------------------
template <class T>
inline void
List<T>::UnlinkAll()
{
    while (!IsEmpty()) {
        UnlinkHead();
    }
}


//------------------------------------------------------------------------------
template <class T>
inline __ecount(1) T *
List<T>::UnlinkHead()
{
    return static_cast<T *> (RawList::UnlinkHead());
}


//------------------------------------------------------------------------------
template <class T>
inline __ecount(1) T *
List<T>::UnlinkTail()
{
    return static_cast<T *> (RawList::UnlinkTail());
}

//------------------------------------------------------------------------------
//
// NOTE-2006/02/28-chrisra SAL gets confused by this -1.
//
// This is a bug in SAL and will currently misreport warning 2061.  This should
// be ignored.
// 
template <class T>
inline __range(-1,INT_MAX) int
List<T>::Find(
    __in_ecount_opt(1) const T *pNode
    ) const
{
    return RawList::Find(pNode);
}


//------------------------------------------------------------------------------
// class SingleList
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
template <class T>
inline
SingleList<T>::SingleList()
{
    m_pHead = NULL;
}


//------------------------------------------------------------------------------
template <class T>
inline
SingleList<T>::~SingleList()
{
    //
    // The list should be cleaned up before being destroyed.  This is being
    // explicitly ASSERT'd here to help ensure this, since when it is not it is
    // most likely a programming error internal to DirectUser.
    //

    AssertConstMsg(IsEmpty(), "List data was not cleaned up");
}

//------------------------------------------------------------------------------
template <class T>
inline __ecount_opt(1) T *
SingleList<T>::GetHead() const
{
    return m_pHead;
}

//------------------------------------------------------------------------------
template <class T>
inline BOOL
SingleList<T>::IsEmpty() const
{
    return m_pHead == NULL;
}


//------------------------------------------------------------------------------
template <class T>
inline void
SingleList<T>::AddHead(
    __inout_ecount(1) T *pNode
    )
{
    pNode->pNext = m_pHead;
    m_pHead = pNode;
}

//------------------------------------------------------------------------------
template <class T>
void
SingleList<T>::Remove(
    __inout_ecount(1) T *pNode
    )
{
    if (pNode == m_pHead) {
        m_pHead = pNode->pNext;
        pNode->pNext = NULL;
    } else {
        for (T * pTemp = m_pHead; pTemp != NULL; pTemp = pTemp->pNext) {
            if (pTemp->pNext == pNode) {
                pTemp->pNext = pNode->pNext;
                pNode->pNext = NULL;
                break;
            }
        }
        AssertConstMsg(pTemp != NULL, "Ensure that the node was found.");
    }
}


//------------------------------------------------------------------------------
template <class T>
inline __ecount_opt(1) T *
SingleList<T>::Extract()
{
    T * pHead = m_pHead;
    m_pHead = NULL;
    return pHead;
}


#if AVALON_INCLUDE_SLIST

//------------------------------------------------------------------------------
// class InterlockedList
//------------------------------------------------------------------------------
*****************************************************************************
\***************************************************************************/

//------------------------------------------------------------------------------
template <class T>
inline
InterlockedList<T>::InterlockedList()
{
    _RtlInitializeSListHead(&m_head);
}


//------------------------------------------------------------------------------
template <class T>
inline
InterlockedList<T>::~InterlockedList()
{
    //
    // The list should be cleaned up before being destroyed.  This is being
    // explicitly ASSERT'd here to help ensure this, since when it is not it is
    // most likely a programming error internal to DirectUser.
    //

    AssertConstMsg(IsEmptyNL(), "List data was not cleaned up");
}


//------------------------------------------------------------------------------
template <class T>
inline BOOL
InterlockedList<T>::IsEmptyNL() const
{
#if defined(_WIN64)
    return m_head.Next == 0;
#else
    return m_head.Next.Next == 0;
#endif
}


//------------------------------------------------------------------------------
template <class T>
inline void
InterlockedList<T>::CheckAlignment() const
{
    //
    // SList are a special beast because the pNext field MUST be the first
    // member of the structure.  If it is not, then we can't do an 
    // InterlockedCompareExchange64.
    //

    const size_t nOffsetNode    = offsetof(T, pNext);
    const size_t nOffsetEntry   = offsetof(SINGLE_LIST_ENTRY, Next);
    const size_t nDelta         = nOffsetNode - nOffsetEntry;

    AssertConstMsg(nDelta == 0, "pNext MUST be the first member of the structure");
}


//------------------------------------------------------------------------------
template <class T>
inline void
InterlockedList<T>::AddHeadNL(
    __inout_ecount(1) T *pNode
    )
{
    CheckAlignment();
    _RtlInterlockedPushEntrySList(&m_head, (SINGLE_LIST_ENTRY *) pNode);
}


//------------------------------------------------------------------------------

template <class T>
inline __ecount_opt(1) T *
InterlockedList<T>::RemoveHeadNL()
{
    return static_cast<T *>(_RtlInterlockedPopEntrySList(&m_head));
}


//------------------------------------------------------------------------------
template <class T>
inline __ecount_opt(1) T *
InterlockedList<T>::ExtractNL()
{
    return static_cast<T *>(_RtlInterlockedFlushSList(&m_head));
}


#endif // AVALON_INCLUDE_SLIST

#endif // UTILLIB__List_inl__INCLUDED


