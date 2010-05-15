/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            ntoskrnl/mm/ARM3/pfnlist.c
 * PURPOSE:         ARM Memory Manager PFN List Manipulation
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

#line 15 "ARM³::PFNLIST"
#define MODULE_INVOLVED_IN_ARM3
#include "../ARM3/miarm.h"

/* GLOBALS ********************************************************************/

BOOLEAN MmDynamicPfn;
BOOLEAN MmMirroring;

MMPFNLIST MmZeroedPageListHead = {0, ZeroedPageList, LIST_HEAD, LIST_HEAD};
MMPFNLIST MmFreePageListHead = {0, FreePageList, LIST_HEAD, LIST_HEAD};
MMPFNLIST MmStandbyPageListHead = {0, StandbyPageList, LIST_HEAD, LIST_HEAD};
MMPFNLIST MmModifiedPageListHead = {0, ModifiedPageList, LIST_HEAD, LIST_HEAD};
MMPFNLIST MmModifiedNoWritePageListHead = {0, ModifiedNoWritePageList, LIST_HEAD, LIST_HEAD};
MMPFNLIST MmBadPageListHead = {0, BadPageList, LIST_HEAD, LIST_HEAD};
MMPFNLIST MmRomPageListHead = {0, StandbyPageList, LIST_HEAD, LIST_HEAD};

PMMPFNLIST MmPageLocationList[] =
{
    &MmZeroedPageListHead,
    &MmFreePageListHead,
    &MmStandbyPageListHead,
    &MmModifiedPageListHead,
    &MmModifiedNoWritePageListHead,
    &MmBadPageListHead,
    NULL,
    NULL
};
/* FUNCTIONS ******************************************************************/

VOID
NTAPI
MiInsertInListTail(IN PMMPFNLIST ListHead,
                   IN PMMPFN Entry)
{
    PFN_NUMBER OldBlink, EntryIndex = MiGetPfnEntryIndex(Entry);

    /* Get the back link */
    OldBlink = ListHead->Blink;
    if (OldBlink != LIST_HEAD)
    {
        /* Set the back pointer to point to us now */
        MiGetPfnEntry(OldBlink)->u1.Flink = EntryIndex;
    }
    else
    {
        /* Set the list to point to us */
        ListHead->Flink = EntryIndex;
    }
    
    /* Set the entry to point to the list head forwards, and the old page backwards */
    Entry->u1.Flink = LIST_HEAD;
    Entry->u2.Blink = OldBlink;
    
    /* And now the head points back to us, since we are last */
    ListHead->Blink = EntryIndex;
    ListHead->Total++;
}

VOID
NTAPI
MiInsertZeroListAtBack(IN PFN_NUMBER EntryIndex)
{
    PFN_NUMBER OldBlink;
    PMMPFNLIST ListHead;
    PMMPFN Pfn1;
#if 0
    PMMPFN Blink;
    ULONG Color;
    PMMCOLOR_TABLES ColorHead;
#endif

    /* Make sure the PFN lock is held */
    ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);
    
    /* Get the descriptor */
    Pfn1 = MiGetPfnEntry(EntryIndex);
    ASSERT(Pfn1->u3.e2.ReferenceCount == 0);
    ASSERT(Pfn1->u4.MustBeCached == 0);
    ASSERT(Pfn1->u3.e1.Rom == 0);
    ASSERT(Pfn1->u3.e1.RemovalRequested == 0);
    ASSERT(Pfn1->u4.InPageError == 0);
    
    /* Use the zero list */
    ListHead = &MmZeroedPageListHead;
    ListHead->Total++;

    /* Get the back link */
    OldBlink = ListHead->Blink;
    if (OldBlink != LIST_HEAD)
    {
        /* Set the back pointer to point to us now */
        MiGetPfnEntry(OldBlink)->u1.Flink = EntryIndex;
    }
    else
    {
        /* Set the list to point to us */
        ListHead->Flink = EntryIndex;
    }
    
    /* Set the entry to point to the list head forwards, and the old page backwards */
    Pfn1->u1.Flink = LIST_HEAD;
    Pfn1->u2.Blink = OldBlink;
    
    /* And now the head points back to us, since we are last */
    ListHead->Blink = EntryIndex;
    
    /* Update the page location */
    Pfn1->u3.e1.PageLocation = ZeroedPageList;

    /* FIXME: NOT YET Due to caller semantics: Update the available page count */
    //MmAvailablePages++;

    /* Check if we've reached the configured low memory threshold */
    if (MmAvailablePages == MmLowMemoryThreshold)
    {
        /* Clear the event, because now we're ABOVE the threshold */
        KeClearEvent(MiLowMemoryEvent);
    }
    else if (MmAvailablePages == MmHighMemoryThreshold)
    {
        /* Otherwise check if we reached the high threshold and signal the event */
        KeSetEvent(MiHighMemoryEvent, 0, FALSE);
    }
#if 0
    /* Get the page color */
    Color = EntryIndex & MmSecondaryColorMask;

    /* Get the first page on the color list */
    ColorHead = &MmFreePagesByColor[ZeroedPageList][Color];
    if (ColorHead->Flink == LIST_HEAD)
    {
        /* The list is empty, so we are the first page */
        Pfn1->u4.PteFrame = -1;
        ColorHead->Flink = EntryIndex;
    }
    else
    {
        /* Get the previous page */
        Blink = (PMMPFN)ColorHead->Blink;
        
        /* Make it link to us */
        Pfn1->u4.PteFrame = MiGetPfnEntryIndex(Blink);
        Blink->OriginalPte.u.Long = EntryIndex;
    }
    
    /* Now initialize our own list pointers */
    ColorHead->Blink = Pfn1;
    Pfn1->OriginalPte.u.Long = LIST_HEAD;
    
    /* And increase the count in the colored list */
    ColorHead->Count++;
#endif
}

VOID
NTAPI
MiUnlinkFreeOrZeroedPage(IN PMMPFN Entry)
{
    PFN_NUMBER OldFlink, OldBlink;
    PMMPFNLIST ListHead;
    MMLISTS ListName;
    
    /* Make sure the PFN lock is held */
    ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);
    
    /* Make sure the PFN entry isn't in-use */
    ASSERT(Entry->u3.e1.WriteInProgress == 0);
    ASSERT(Entry->u3.e1.ReadInProgress == 0);
    
    /* Find the list for this entry, make sure it's the free or zero list */
    ListHead = MmPageLocationList[Entry->u3.e1.PageLocation];
    ListName = ListHead->ListName;
    ASSERT(ListHead != NULL);
    ASSERT(ListName <= FreePageList);
    
    /* Remove one count */
    ASSERT(ListHead->Total != 0);
    ListHead->Total--;
    
    /* Get the forward and back pointers */
    OldFlink = Entry->u1.Flink;
    OldBlink = Entry->u2.Blink;
    
    /* Check if the next entry is the list head */
    if (OldFlink != LIST_HEAD)
    {
        /* It is not, so set the backlink of the actual entry, to our backlink */
        MiGetPfnEntry(OldFlink)->u2.Blink = OldBlink;
    }
    else
    {
        /* Set the list head's backlink instead */
        ListHead->Blink = OldFlink;
    }
    
    /* Check if the back entry is the list head */
    if (OldBlink != LIST_HEAD)
    {
        /* It is not, so set the backlink of the actual entry, to our backlink */
        MiGetPfnEntry(OldBlink)->u1.Flink = OldFlink;
    }
    else
    {
        /* Set the list head's backlink instead */
        ListHead->Flink = OldFlink;
    }
    
    /* We are not on a list anymore */
    Entry->u1.Flink = Entry->u2.Blink = 0;
    
    /* FIXME: Deal with color list */
    
    /* See if we hit any thresholds */
    if (MmAvailablePages == MmHighMemoryThreshold)
    {
        /* Clear the high memory event */
        KeClearEvent(MiHighMemoryEvent);
    }
    else if (MmAvailablePages == MmLowMemoryThreshold)
    {
        /* Signal the low memory event */
        KeSetEvent(MiLowMemoryEvent, 0, FALSE);
    }
    
    /* One less page */
    if (--MmAvailablePages < MmMinimumFreePages)
    {
        /* FIXME: Should wake up the MPW and working set manager, if we had one */
    }
}

PFN_NUMBER
NTAPI
MiRemovePageByColor(IN PFN_NUMBER PageIndex,
                    IN ULONG Color)
{
    PMMPFN Pfn1;
    PMMPFNLIST ListHead;
    MMLISTS ListName;
    PFN_NUMBER OldFlink, OldBlink;
    ULONG OldColor, OldCache;
#if 0
    PMMCOLOR_TABLES ColorTable;
#endif   
    /* Make sure PFN lock is held */
    ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);
    ASSERT(Color < MmSecondaryColors);

    /* Get the PFN entry */
    Pfn1 = MiGetPfnEntry(PageIndex);
    ASSERT(Pfn1->u3.e1.RemovalRequested == 0);
    ASSERT(Pfn1->u3.e1.Rom == 0);
    
    /* Capture data for later */
    OldColor = Pfn1->u3.e1.PageColor;
    OldCache = Pfn1->u3.e1.CacheAttribute;

    /* Could be either on free or zero list */
    ListHead = MmPageLocationList[Pfn1->u3.e1.PageLocation];
    ListName = ListHead->ListName;
    ASSERT(ListName <= FreePageList);
    
    /* Remove a page */
    ListHead->Total--;

    /* Get the forward and back pointers */
    OldFlink = Pfn1->u1.Flink;
    OldBlink = Pfn1->u2.Blink;
    
    /* Check if the next entry is the list head */
    if (OldFlink != LIST_HEAD)
    {
        /* It is not, so set the backlink of the actual entry, to our backlink */
        MiGetPfnEntry(OldFlink)->u2.Blink = OldBlink;
    }
    else
    {
        /* Set the list head's backlink instead */
        ListHead->Blink = OldFlink;
    }
    
    /* Check if the back entry is the list head */
    if (OldBlink != LIST_HEAD)
    {
        /* It is not, so set the backlink of the actual entry, to our backlink */
        MiGetPfnEntry(OldBlink)->u1.Flink = OldFlink;
    }
    else
    {
        /* Set the list head's backlink instead */
        ListHead->Flink = OldFlink;
    }
    
    /* We are not on a list anymore */
    Pfn1->u1.Flink = Pfn1->u2.Blink = 0;
    
    /* Zero flags but restore color and cache */
    Pfn1->u3.e2.ShortFlags = 0;
    Pfn1->u3.e1.PageColor = OldColor;
    Pfn1->u3.e1.CacheAttribute = OldCache;
#if 0 // When switching to ARM3
    /* Get the first page on the color list */
    ColorTable = &MmFreePagesByColor[ListName][Color];
    ASSERT(ColorTable->Count >= 1);
    
    /* Set the forward link to whoever we were pointing to */
    ColorTable->Flink = Pfn1->OriginalPte.u.Long;
    if (ColorTable->Flink == LIST_HEAD)
    {
        /* This is the beginning of the list, so set the sentinel value */
        ColorTable->Blink = LIST_HEAD;    
    }
    else
    {
        /* The list is empty, so we are the first page */
        MiGetPfnEntry(ColorTable->Flink)->u4.PteFrame = -1;
    }
    
    /* One more page */
    ColorTable->Total++;
#endif
    /* See if we hit any thresholds */
    if (MmAvailablePages == MmHighMemoryThreshold)
    {
        /* Clear the high memory event */
        KeClearEvent(MiHighMemoryEvent);
    }
    else if (MmAvailablePages == MmLowMemoryThreshold)
    {
        /* Signal the low memory event */
        KeSetEvent(MiLowMemoryEvent, 0, FALSE);
    }
    
    /* One less page */
    if (--MmAvailablePages < MmMinimumFreePages)
    {
        /* FIXME: Should wake up the MPW and working set manager, if we had one */
    }

    /* Return the page */
    return PageIndex;
}

PFN_NUMBER
NTAPI
MiRemoveAnyPage(IN ULONG Color)
{
    PFN_NUMBER PageIndex;
    PMMPFN Pfn1;

    /* Make sure PFN lock is held and we have pages */
    ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);
    ASSERT(MmAvailablePages != 0);
    ASSERT(Color < MmSecondaryColors);
    
    /* Check the colored free list */
#if 0 // Enable when using ARM3 database */
    PageIndex = MmFreePagesByColor[FreePageList][Color].Flink;
    if (PageIndex == LIST_HEAD)
    {
        /* Check the colored zero list */
        PageIndex = MmFreePagesByColor[ZeroedPageList][Color].Flink;
        if (PageIndex == LIST_HEAD)
        {
#endif
            /* Check the free list */
            PageIndex = MmFreePageListHead.Flink;
            Color = PageIndex & MmSecondaryColorMask;
            if (PageIndex == LIST_HEAD)
            {
                /* Check the zero list */
                ASSERT(MmFreePageListHead.Total == 0);
                PageIndex = MmZeroedPageListHead.Flink;
                Color = PageIndex & MmSecondaryColorMask;
                ASSERT(PageIndex != LIST_HEAD);
                if (PageIndex == LIST_HEAD)
                {
                    /* FIXME: Should check the standby list */
                    ASSERT(MmZeroedPageListHead.Total == 0);
                }
            }
#if 0 // Enable when using ARM3 database */
        }
    }
#endif

    /* Remove the page from its list */
    PageIndex = MiRemovePageByColor(PageIndex, Color);

    /* Sanity checks */
    Pfn1 = MiGetPfnEntry(PageIndex);
    ASSERT((Pfn1->u3.e1.PageLocation == FreePageList) ||
           (Pfn1->u3.e1.PageLocation == ZeroedPageList));
    ASSERT(Pfn1->u3.e2.ReferenceCount == 0);
    ASSERT(Pfn1->u2.ShareCount == 0);
        
    /* Return the page */
    return PageIndex;
}

PMMPFN
NTAPI
MiRemoveHeadList(IN PMMPFNLIST ListHead)
{
    PFN_NUMBER Entry, Flink;
    PMMPFN Pfn1;
    
    /* Get the entry that's currently first on the list */
    Entry = ListHead->Flink;
    Pfn1 = MiGetPfnEntry(Entry);
    
    /* Make the list point to the entry following the first one */
    Flink = Pfn1->u1.Flink;
    ListHead->Flink = Flink;

    /* Check if the next entry is actually the list head */
    if (ListHead->Flink != LIST_HEAD)
    {
        /* It isn't, so therefore whoever is coming next points back to the head */
        MiGetPfnEntry(Flink)->u2.Blink = LIST_HEAD;
    }
    else
    {
        /* Then the list is empty, so the backlink should point back to us */
        ListHead->Blink = LIST_HEAD;
    }
  
    /* We are not on a list anymore */
    Pfn1->u1.Flink = Pfn1->u2.Blink = 0;
    ListHead->Total--;
    
    /* Return the head element */
    return Pfn1;
}

VOID
NTAPI
MiInsertPageInFreeList(IN PFN_NUMBER PageFrameIndex)
{
    PMMPFNLIST ListHead;
    PFN_NUMBER LastPage;
    PMMPFN Pfn1;
#if 0
    ULONG Color;
    PMMPFN Blink;
    PMMCOLOR_TABLES ColorTable;
#endif
    /* Make sure the page index is valid */
    ASSERT((PageFrameIndex != 0) &&
           (PageFrameIndex <= MmHighestPhysicalPage) &&
           (PageFrameIndex >= MmLowestPhysicalPage));

    /* Get the PFN entry */
    Pfn1 = MiGetPfnEntry(PageFrameIndex);

    /* Sanity checks that a right kind of page is being inserted here */
    ASSERT(Pfn1->u4.MustBeCached == 0);
    ASSERT(Pfn1->u3.e1.Rom != 1);
    ASSERT(Pfn1->u3.e1.RemovalRequested == 0);
    ASSERT(Pfn1->u4.VerifierAllocation == 0);
    ASSERT(Pfn1->u3.e2.ReferenceCount == 0);

    /* Get the free page list and increment its count */
    ListHead = &MmFreePageListHead;
    ListHead->Total++;

    /* Get the last page on the list */
    LastPage = ListHead->Blink;
    if (LastPage != LIST_HEAD)
    {
        /* Link us with the previous page, so we're at the end now */
        MiGetPfnEntry(LastPage)->u1.Flink = PageFrameIndex;
    }
    else
    {
        /* The list is empty, so we are the first page */
        ListHead->Flink = PageFrameIndex;
    }

    /* Now make the list head point back to us (since we go at the end) */
    ListHead->Blink = PageFrameIndex;
    
    /* And initialize our own list pointers */
    Pfn1->u1.Flink = LIST_HEAD;
    Pfn1->u2.Blink = LastPage;

    /* Set the list name and default priority */
    Pfn1->u3.e1.PageLocation = FreePageList;
    Pfn1->u4.Priority = 3;
    
    /* Clear some status fields */
    Pfn1->u4.InPageError = 0;
    Pfn1->u4.AweAllocation = 0;

    /* Increase available pages */
    MmAvailablePages++;

    /* Check if we've reached the configured low memory threshold */
    if (MmAvailablePages == MmLowMemoryThreshold)
    {
        /* Clear the event, because now we're ABOVE the threshold */
        KeClearEvent(MiLowMemoryEvent);
    }
    else if (MmAvailablePages == MmHighMemoryThreshold)
    {
        /* Otherwise check if we reached the high threshold and signal the event */
        KeSetEvent(MiHighMemoryEvent, 0, FALSE);
    }

#if 0 // When using ARM3 PFN
    /* Get the page color */
    Color = PageFrameIndex & MmSecondaryColorMask;

    /* Get the first page on the color list */
    ColorTable = &MmFreePagesByColor[FreePageList][Color];
    if (ColorTable->Flink == LIST_HEAD)
    {
        /* The list is empty, so we are the first page */
        Pfn1->u4.PteFrame = -1;
        ColorTable->Flink = PageFrameIndex;
    }
    else
    {
        /* Get the previous page */
        Blink = (PMMPFN)ColorTable->Blink;
        
        /* Make it link to us */
        Pfn1->u4.PteFrame = MI_PFNENTRY_TO_PFN(Blink);
        Blink->OriginalPte.u.Long = PageFrameIndex;
    }
    
    /* Now initialize our own list pointers */
    ColorTable->Blink = Pfn1;
    Pfn1->OriginalPte.u.Long = LIST_HEAD;
    
    /* And increase the count in the colored list */
    ColorTable->Count++;
#endif
    
    /* Notify zero page thread if enough pages are on the free list now */
    extern KEVENT ZeroPageThreadEvent;
    if ((MmFreePageListHead.Total > 8) && !(KeReadStateEvent(&ZeroPageThreadEvent)))
    {
        /* This is ReactOS-specific */
        KeSetEvent(&ZeroPageThreadEvent, IO_NO_INCREMENT, FALSE);
    }
}

/* EOF */
