// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//-----------------------------------------------------------------------------

//
//  Description:
//      Implements a collection of different list classes, each designed
//     for specialized usage.
//

#ifndef UTILLIB__List_h__INCLUDED
#define UTILLIB__List_h__INCLUDED
#pragma once

#if AVALON_INCLUDE_SLIST
#include "SList.h"
#endif

// Meters ----------------------------------------------------------------------
MtExtern(List);

//------------------------------------------------------------------------------
// class List provides a high-performance, doublely-linked list.
//------------------------------------------------------------------------------


// ListNode wraps some object so that it can be maintained in a list.
// This class does not derive from the class b/c it wants to always have
// pNext and pPrev as the first members of the data so that they are in the
// same place for all lists

struct ListNode
{
    ListNode *  pNext;
    ListNode *  pPrev;
};


//------------------------------------------------------------------------------
template <class T>
struct ListNodeT : ListNode
{
    inline  __ecount_opt(1) T *         GetNext() const;
    inline  __ecount_opt(1) T *         GetPrev() const;
};


//------------------------------------------------------------------------------
class RawList
{
// Construction/destruction
public:
            RawList();
            ~RawList();

// Operations
public:
            __range(0,INT_MAX) int      GetSize() const;
            __ecount_opt(1) ListNode *  GetHead() const;
            __ecount_opt(1) ListNode *  GetTail() const;
            __ecount_opt(1) ListNode *  GetAt(__range(0,INT_MAX) int idxItem) const;

    inline  BOOL                        IsEmpty() const;
    inline  void                        Extract(__inout_ecount(1) RawList &lstSrc);
    inline  void                        MarkEmpty();

            void                        Add(__inout_ecount(1) ListNode *pNode);
            void                        AddHead(__inout_ecount(1) ListNode *pNode);
            void                        AddTail(__inout_ecount(1) ListNode *pNode);

            void                        InsertAfter(__inout_ecount(1)     ListNode *pInsert, 
                                                    __inout_ecount_opt(1) ListNode *pBefore
                                                    );

            void                        InsertBefore(__inout_ecount(1)     ListNode *pInsert, 
                                                     __inout_ecount_opt(1) ListNode *pAfter
                                                     );

            void                        Unlink(__inout_ecount(1) ListNode *pNode);
            __ecount(1) ListNode *      UnlinkHead();
            __ecount(1) ListNode *      UnlinkTail();

            __range(-1,INT_MAX) int     Find(__in_ecount_opt(1) const ListNode *pNode) const;

// Implementation
protected:

// Data
protected:
            ListNode *  m_pHead;
};


//------------------------------------------------------------------------------
template <class T>
class List : public RawList
{
// Construction/destruction
public:
    DECLARE_METERHEAP_ALLOC(ProcessHeap, Mt(List));
    inline  ~List();

// Operations
public:
    inline  __ecount_opt(1) T *         GetHead() const;
    inline  __ecount_opt(1) T *         GetTail() const;
    inline  __ecount_opt(1) T *         GetAt(__range(0,INT_MAX) int idxItem) const;

    inline  void                        Extract(__inout_ecount(1) List<T> &lstSrc);
    inline  __ecount_opt(1) T *         Extract();

    inline  void                        Add(__inout_ecount(1) T *pNode);
    inline  void                        AddHead(__inout_ecount(1) T *pNode);
    inline  void                        AddTail(__inout_ecount(1) T *pNode);

    inline  void                        InsertAfter(__inout_ecount(1)     T *pInsert,
                                                    __inout_ecount_opt(1) T *pBefore
                                                    );
    inline  void                        InsertBefore(__inout_ecount(1)     T *pInsert, 
                                                     __inout_ecount_opt(1) T *pAfter
                                                     );

    inline  void                        Remove(__inout_ecount(1) T *pNode);
    inline  BOOL                        RemoveAt(__range(0,INT_MAX) int idxItem);
    inline  void                        RemoveAll();

    inline  void                        Unlink(__inout_ecount(1) T *pNode);
    inline  void                        UnlinkAll();
    inline  __ecount(1) T *             UnlinkHead();
    inline  __ecount(1) T *             UnlinkTail();

    inline  __range(-1,INT_MAX) int     Find(__in_ecount_opt(1) const T * pNode) const;
};


//------------------------------------------------------------------------------
// class GSingleList 
//
// Description: Provides a high-performance, non-thread-safe, 
//   single-linked list that is similar to InterlockedList but without 
//   the cross-thread overhead.
//------------------------------------------------------------------------------

template <class T>
class SingleList
{
// Construction
public:
    DECLARE_METERHEAP_ALLOC(ProcessHeap, Mt(List));
    inline  SingleList();
    inline  ~SingleList();

// Operations
public:
    inline  __ecount_opt(1) T *         GetHead() const;

    inline  BOOL                        IsEmpty() const;
    inline  void                        AddHead(__inout_ecount(1) T *pNode);
            void                        Remove(__inout_ecount(1) T *pNode);
    inline  __ecount_opt(1) T *         Extract();

// Data
protected:
            T *         m_pHead;
};


#if AVALON_INCLUDE_SLIST

//------------------------------------------------------------------------------
// class InterlockedList 
// 
// Description: Provides a high-performance, thread-safe stack
//   that doesn't use any locks.  Because of its high-performance, lightweight
//   nature, there are not very many functions that are available.  All of the
//   available functions use InterlockedXXX functions to safely manipulate
//   the list.
//------------------------------------------------------------------------------
template <class T>
class InterlockedList
{
// Construction
public:
    inline  InterlockedList();
    inline  ~InterlockedList();

// Operations
public:
    inline  BOOL                        IsEmptyNL() const;
    inline  void                        AddHeadNL(__inout_ecount(1) T *pNode);
    inline  __ecount_opt(1) T *         RemoveHeadNL();
    inline  __ecount_opt(1) T *         ExtractNL();

// Implementation
protected:
    inline  void        CheckAlignment() const;

// Data
protected:
    SLIST_HEADER    m_head;
};

#endif // AVALON_INCLUDE_SLIST


//------------------------------------------------------------------------------
// Generic List Utilities
//------------------------------------------------------------------------------

template <class T> bool IsLoop(__in_ecount_opt(1) const T *pEntry);
template <class T> void ReverseSingleList(__deref_inout_ecount(1) T * &pEntry);


#include "List.inl"

#endif // UTILLIB__List_h__INCLUDED



