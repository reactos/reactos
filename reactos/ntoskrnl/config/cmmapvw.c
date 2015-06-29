/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/config/cmmapvw.c
 * PURPOSE:         Configuration Manager - Map-Viewed Hive Support
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include "ntoskrnl.h"
#define NDEBUG
#include "debug.h"

/* GLOBALS *******************************************************************/

/* FUNCTIONS *****************************************************************/

VOID
NTAPI
CmpInitHiveViewList(IN PCMHIVE Hive)
{
    /* Initialize the list heads */
    InitializeListHead(&Hive->LRUViewListHead);
    InitializeListHead(&Hive->PinViewListHead);

    /* Reset data */
    Hive->MappedViews = 0;
    Hive->PinnedViews = 0;
    Hive->UseCount = 0;
}

VOID
NTAPI
CmpDestroyHiveViewList(IN PCMHIVE Hive)
{
    PCM_VIEW_OF_FILE CmView;
    PLIST_ENTRY EntryList;

    /* Do NOT destroy the views of read-only hives */
    ASSERT(Hive->Hive.ReadOnly == FALSE);

    /* Free all the views inside the Pinned View List */
    EntryList = RemoveHeadList(&Hive->PinViewListHead);
    while (EntryList != &Hive->PinViewListHead)
    {
        CmView = CONTAINING_RECORD(EntryList, CM_VIEW_OF_FILE, PinViewList);

        /* FIXME: Unmap the view if it is mapped */

        ExFreePool(CmView);

        Hive->PinnedViews--;

        EntryList = RemoveHeadList(&Hive->PinViewListHead);
    }

    /* The Pinned View List should be empty */
    ASSERT(IsListEmpty(&Hive->PinViewListHead) == TRUE);
    ASSERT(Hive->PinnedViews == 0);

    /* Now, free all the views inside the LRU View List */
    EntryList = RemoveHeadList(&Hive->LRUViewListHead);
    while (EntryList != &Hive->LRUViewListHead)
    {
        CmView = CONTAINING_RECORD(EntryList, CM_VIEW_OF_FILE, LRUViewList);

        /* FIXME: Unmap the view if it is mapped */

        ExFreePool(CmView);

        Hive->MappedViews--;

        EntryList = RemoveHeadList(&Hive->LRUViewListHead);
    }

    /* The LRU View List should be empty */
    ASSERT(IsListEmpty(&Hive->LRUViewListHead) == TRUE);
    ASSERT(Hive->MappedViews == 0);
}

/* EOF */
