// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


/*++



Module Name:

    slistutil.cpp

Abstract:

  utilities for manipulating SLists

Environment:

    User mode only.

--*/

#include "precomp.hpp"

/*++

Routine Description:

    Reverse an SList pointed to by ppHead. On return ppHead points to the
    reversed list.

--*/

VOID ReverseSList(__deref_inout_ecount_opt(1) SLIST_ENTRY **ppHead)
{
    SLIST_ENTRY *pHead = *ppHead;
    SLIST_ENTRY *pEntry = NULL;

    while (pHead)
    {
        SLIST_ENTRY *pTemp = pHead->Next;
        pHead->Next = pEntry;
        pEntry = pHead;
        pHead = pTemp;
    }

    *ppHead = pEntry;
}


