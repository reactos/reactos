// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+----------------------------------------------------------------------------
//

//
//  Abstract:
//      Templates for a generic lists
//
//  Notes:
//      Only minimal definitions are defined.  As more functionality is needed
//      it can be added.
//-----------------------------------------------------------------------------

#pragma once

//+----------------------------------------------------------------------------
//
//  Template Class:
//      CDoubleLinkedList<EntryType>
//
//  Synopsis:
//      Wrapper around LIST_ENTRY head for a list of EntryType's, which must
//      derive from LIST_ENTRY.
//
//-----------------------------------------------------------------------------

template <typename TEntry>
class CDoubleLinkedList
{
public:
    CDoubleLinkedList()
    {
        InitializeListHead(&m_leHead);
    }

    ~CDoubleLinkedList()
    {
        Assert(IsListEmpty(&m_leHead));
    }

    BOOLEAN IsEmpty()
    {
        return IsListEmpty(&m_leHead);
    }

    void InsertAtHead(
        __inout_ecount(1) TEntry *pResource
        )
    {
        InsertHeadList(&m_leHead, pResource);
        Assert(pResource->Flink != NULL);
        Assert(pResource->Blink != NULL);
    }

    void InsertAtTail(
        __inout_ecount(1) TEntry *pResource
        )
    {
        InsertTailList(&m_leHead, pResource);
        Assert(pResource->Flink != NULL);
        Assert(pResource->Blink != NULL);
    }

    __out_ecount(1) TEntry *PeekAtHead()
    {
        if (!IsEmpty())
        {
            TEntry *pEntry = RemoveHeadEntry();
            InsertAtHead(pEntry);
            Assert(pEntry->Flink != NULL);
            Assert(pEntry->Blink != NULL);
            return pEntry;                
        }
        else
        {
            return NULL;
        }
    }

    __out_ecount(1) TEntry *PeekAtTail()
    {
        if (!IsEmpty())
        {
            TEntry *pEntry = RemoveTailEntry();
            InsertAtTail(pEntry);
            Assert(pEntry->Flink != NULL);
            Assert(pEntry->Blink != NULL);
            return pEntry;                
        }
        else
        {
            return NULL;
        }
    }

    __out_ecount(1) TEntry *PeekNext(TEntry *pEntry)
    {
        Assert(pEntry->Flink != NULL);
        Assert(pEntry->Blink != NULL);
        return (pEntry->Flink != &m_leHead) ? (TEntry *)pEntry->Flink : NULL;
    }

    __out_ecount(1) TEntry *PeekPrevious(TEntry *pEntry)
    {
        Assert(pEntry->Flink != NULL);
        Assert(pEntry->Blink != NULL);
        return (pEntry->Blink != &m_leHead) ? (TEntry *)pEntry->Blink : NULL;
    }

    void RemoveFromList(TEntry *pEntry)
    {
        pEntry->Flink->Blink = pEntry->Blink;
        pEntry->Blink->Flink = pEntry->Flink;
        pEntry->Blink = NULL;
        pEntry->Flink = NULL;
    }

    __out_ecount(1) TEntry *RemoveHeadEntry()
    {
        // RemoveHeadList() does not fail when the list is empty, it returns
        // m_leHead.  To be on the safe side we explicitly check for the
        // empty case and fail.

        if (IsEmpty())
        {
            RIP("CDoubleLinkedList::RemoveHeadEntry -- Attempt to remove from an empty list.");
        }

        TEntry *pEntry =
            static_cast<TEntry *>(RemoveHeadList(&m_leHead));

        //
        // List element may define MarkAsUnlisted if it would like to know when
        // it is removed from the list.
        //

        __if_exists (TEntry::MarkAsUnlisted)
        {
            pEntry->MarkAsUnlisted();
        }

        return pEntry;
    }

    __out_ecount(1) TEntry *RemoveTailEntry()
    {
        // RemoveTailEntry() does not fail when the list is empty, it returns
        // m_leHead.  To be on the safe side we explicitly check for the
        // empty case and fail.

        if (IsEmpty())
        {
            RIP("CDoubleLinkedList::RemoveTailEntry -- Attempt to remove from an empty list.");
        }

        TEntry *pEntry =
            static_cast<TEntry *>(RemoveTailList(&m_leHead));

        //
        // List element may define MarkAsUnlisted if it would like to know when
        // it is removed from the list.
        //

        __if_exists (TEntry::MarkAsUnlisted)
        {
            pEntry->MarkAsUnlisted();
        }

        return pEntry;
    }

    bool ValidateList()
    {
        if (IsEmpty())
        {
            return true;
        }

        _LIST_ENTRY *pCurrent = m_leHead.Flink;
        while (pCurrent->Flink != &m_leHead)
        {
            pCurrent = pCurrent->Flink;
            if (pCurrent == NULL)
            {
                Assert(false);
            }
        }

        pCurrent = m_leHead.Blink;
        while (pCurrent != &m_leHead)
        {
            pCurrent = pCurrent->Blink;
            if (pCurrent == NULL)
            {
                Assert(false);
            }
        }
        
        return true;
    }
    

protected:

    LIST_ENTRY  m_leHead;
};


