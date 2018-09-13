/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

   pfnlist.c

Abstract:

    This module contains the routines to manipulate pages within the
    Page Frame Database.

Author:

    Lou Perazzoli (loup) 4-Apr-1989
    Landy Wang (landyw) 02-June-1997

Revision History:

--*/
#include "mi.h"

#define MM_LOW_LIMIT 2
#define MM_HIGH_LIMIT 19

KEVENT MmAvailablePagesEventHigh;

ULONG MmTransitionPrivatePages;
ULONG MmTransitionSharedPages;

#define MI_TALLY_TRANSITION_PAGE_ADDITION(Pfn) \
    if (Pfn->u3.e1.PrototypePte) { \
        MmTransitionSharedPages += 1; \
    } \
    else { \
        MmTransitionPrivatePages += 1; \
    } \
    ASSERT (MmTransitionPrivatePages + MmTransitionSharedPages == MmStandbyPageListHead.Total + MmModifiedPageListHead.Total + MmModifiedNoWritePageListHead.Total);

#define MI_TALLY_TRANSITION_PAGE_REMOVAL(Pfn) \
    if (Pfn->u3.e1.PrototypePte) { \
        MmTransitionSharedPages -= 1; \
    } \
    else { \
        MmTransitionPrivatePages -= 1; \
    } \
    ASSERT (MmTransitionPrivatePages + MmTransitionSharedPages == MmStandbyPageListHead.Total + MmModifiedPageListHead.Total + MmModifiedNoWritePageListHead.Total);

VOID
MiRemovePageByColor (
    IN PFN_NUMBER Page,
    IN ULONG PageColor
    );


VOID
FASTCALL
MiInsertPageInList (
    IN PMMPFNLIST ListHead,
    IN PFN_NUMBER PageFrameIndex
    )

/*++

Routine Description:

    This procedure inserts a page at the end of the specified list (free,
    standby, bad, zeroed, modified).


Arguments:

    ListHead - Supplies the list of the list in which to insert the
               specified physical page.

    PageFrameIndex - Supplies the physical page number to insert in the
                     list.

Return Value:

    none.

Environment:

    Must be holding the PFN database mutex with APCs disabled.

--*/

{
    PFN_NUMBER last;
    PMMPFN Pfn1;
    PMMPFN Pfn2;
    ULONG Color;

    MM_PFN_LOCK_ASSERT();
    ASSERT ((PageFrameIndex != 0) &&
            (PageFrameIndex <= MmHighestPhysicalPage) &&
            (PageFrameIndex >= MmLowestPhysicalPage));

    //
    // Check to ensure the reference count for the page is zero.
    //

    Pfn1 = MI_PFN_ELEMENT(PageFrameIndex);

    ASSERT (Pfn1->u3.e1.LockCharged == 0);

    PERFINFO_INSERTINLIST(PageFrameIndex, ListHead);

#if DBG
    if (MmDebug & MM_DBG_PAGE_REF_COUNT) {

        PMMPTE PointerPte;
        KIRQL OldIrql = 99;

        if ((ListHead->ListName == StandbyPageList) ||
            (ListHead->ListName == ModifiedPageList)) {

            if ((Pfn1->u3.e1.PrototypePte == 1)  &&
                    (MmIsAddressValid (Pfn1->PteAddress))) {
                PointerPte = Pfn1->PteAddress;
            } else {

                //
                // The page containing the prototype PTE is not valid,
                // map the page into hyperspace and reference it that way.
                //

                PointerPte = MiMapPageInHyperSpace (Pfn1->PteFrame, &OldIrql);
                PointerPte = (PMMPTE)((PCHAR)PointerPte +
                                        MiGetByteOffset(Pfn1->PteAddress));
            }

            ASSERT ((MI_GET_PAGE_FRAME_FROM_TRANSITION_PTE (PointerPte) == PageFrameIndex) ||
                    (MI_GET_PAGE_FRAME_FROM_PTE (PointerPte) == PageFrameIndex));
            ASSERT (PointerPte->u.Soft.Transition == 1);
            ASSERT (PointerPte->u.Soft.Prototype == 0);
            if (OldIrql != 99) {
                MiUnmapPageInHyperSpace (OldIrql)
            }
        }
    }
#endif

#if PFN_CONSISTENCY
    if (ListHead == &MmFreePageListHead) {
        if (Pfn1->u2.ShareCount != 0) {
            KeBugCheckEx (PFN_LIST_CORRUPT,
                          0x91,
                          PageFrameIndex,
                          Pfn1->u2.ShareCount,
                          Pfn1->u3.e2.ReferenceCount);
        }
    }
    else if (ListHead == &MmZeroedPageListHead) {
        if (Pfn1->u2.ShareCount != 0) {
            KeBugCheckEx (PFN_LIST_CORRUPT,
                          0x92,
                          PageFrameIndex,
                          Pfn1->u2.ShareCount,
                          Pfn1->u3.e2.ReferenceCount);
        }
    }
#endif

#if DBG
    if ((ListHead->ListName == StandbyPageList) ||
        (ListHead->ListName == ModifiedPageList)) {
        if ((Pfn1->OriginalPte.u.Soft.Prototype == 0) &&
           (Pfn1->OriginalPte.u.Soft.Transition == 1)) {
            KeBugCheckEx (MEMORY_MANAGEMENT, 0x8888, 0,0,0);
        }
    }
#endif

    ASSERT (Pfn1->u3.e2.ReferenceCount == 0);

    ListHead->Total += 1;  // One more page on the list.

    //
    // On MIPS R4000 modified pages destined for the paging file are
    // kept on separate lists which group pages of the same color
    // together
    //

    if (ListHead == &MmModifiedPageListHead) {

#if PFN_CONSISTENCY
        if (Pfn1->u2.ShareCount != 0) {
            KeBugCheckEx (PFN_LIST_CORRUPT,
                          0x90,
                          PageFrameIndex,
                          Pfn1->u2.ShareCount,
                          Pfn1->u3.e2.ReferenceCount);
        }
#endif

        if (Pfn1->OriginalPte.u.Soft.Prototype == 0) {

            //
            // This page is destined for the paging file (not
            // a mapped file).  Change the list head to the
            // appropriate colored list head.
            //

            ListHead = &MmModifiedPageListByColor [Pfn1->u3.e1.PageColor];
            ListHead->Total += 1;
            MmTotalPagesForPagingFile += 1;
        }
        else {

            //
            // This page is destined for a mapped file (not
            // the paging file).  If there are no other pages currently
            // destined for the mapped file, start our timer so that we can
            // ensure that these pages make it to disk even if we don't pile
            // up enough of them to trigger the modified page writer or need
            // the memory.  If we don't do this here, then for this scenario,
            // only an orderly system shutdown will write them out (days,
            // weeks, months or years later) and any power out in between
            // means we'll have lost the data.
            //

            if (ListHead->Total - MmTotalPagesForPagingFile == 1) {

                //
                // Start the DPC timer because we're the first on the list.
                //

                if (MiTimerPending == FALSE) {
                    MiTimerPending = TRUE;

                    (VOID) KeSetTimerEx( &MiModifiedPageWriterTimer, MiModifiedPageLife, 0, &MiModifiedPageWriterTimerDpc );
                }
            }
        }
    }
    else if ((Pfn1->u3.e1.RemovalRequested == 1) &&
             (ListHead->ListName <= StandbyPageList)) {

        ListHead->Total -= 1;  // Undo previous increment

        if (ListHead->ListName == StandbyPageList) {
            Pfn1->u3.e1.PageLocation = StandbyPageList;
            MiRestoreTransitionPte (PageFrameIndex);
        }

        ListHead = MmPageLocationList[BadPageList];
        ListHead->Total += 1;  // One more page on the list.
    }


    last = ListHead->Blink;
    if (last == MM_EMPTY_LIST) {

        //
        // List is empty add the page to the ListHead.
        //

        ListHead->Flink = PageFrameIndex;
    } else {
        Pfn2 = MI_PFN_ELEMENT (last);
        Pfn2->u1.Flink = PageFrameIndex;
    }

    ListHead->Blink = PageFrameIndex;
    Pfn1->u1.Flink = MM_EMPTY_LIST;
    Pfn1->u2.Blink = last;
    Pfn1->u3.e1.PageLocation = ListHead->ListName;

    //
    // If the page was placed on the free, standby or zeroed list,
    // update the count of usable pages in the system.  If the count
    // transitions from 0 to 1, the event associated with available
    // pages should become true.
    //

    if (ListHead->ListName <= StandbyPageList) {
        MmAvailablePages += 1;

        //
        // A page has just become available, check to see if the
        // page wait events should be signalled.
        //

        if (MmAvailablePages == MM_LOW_LIMIT) {
            KeSetEvent (&MmAvailablePagesEvent, 0, FALSE);
        } else if (MmAvailablePages == MM_HIGH_LIMIT) {
            KeSetEvent (&MmAvailablePagesEventHigh, 0, FALSE);
        }

        if (ListHead->ListName <= FreePageList) {

            ASSERT (Pfn1->u3.e1.InPageError == 0);

            //
            // We are adding a page to the free or zeroed page list.
            // Add the page to the end of the correct colored page list.
            //

            Color = MI_GET_SECONDARY_COLOR (PageFrameIndex, Pfn1);
            ASSERT (Pfn1->u3.e1.PageColor == MI_GET_COLOR_FROM_SECONDARY(Color));

            if (MmFreePagesByColor[ListHead->ListName][Color].Flink ==
                                                            MM_EMPTY_LIST) {

                //
                // This list is empty, add this as the first and last
                // entry.
                //

                MmFreePagesByColor[ListHead->ListName][Color].Flink =
                                                                PageFrameIndex;
                MmFreePagesByColor[ListHead->ListName][Color].Blink =
                                                                (PVOID)Pfn1;
            } else {
                Pfn2 = (PMMPFN)MmFreePagesByColor[ListHead->ListName][Color].Blink;
                Pfn2->OriginalPte.u.Long = PageFrameIndex;
                MmFreePagesByColor[ListHead->ListName][Color].Blink = (PVOID)Pfn1;
            }
            Pfn1->OriginalPte.u.Long = MM_EMPTY_LIST;

            if (ListHead->ListName == ZeroedPageList) {
                MI_BARRIER_STAMP_ZEROED_PAGE (&Pfn1->PteFrame);
            }
        }
        else {

            //
            // Transition page list so tally it appropriately.
            //

            MI_TALLY_TRANSITION_PAGE_ADDITION (Pfn1);
        }

        if ((ListHead->ListName == FreePageList) &&
            (MmFreePageListHead.Total >= MmMinimumFreePagesToZero) &&
            (MmZeroingPageThreadActive == FALSE)) {

            //
            // There are enough pages on the free list, start
            // the zeroing page thread.
            //

            MmZeroingPageThreadActive = TRUE;
            KeSetEvent (&MmZeroingPageEvent, 0, FALSE);
        }
        return;
    }

    //
    // Check to see if there are too many modified pages.
    //

    if (ListHead->ListName == ModifiedPageList) {

        //
        // Transition page list so tally it appropriately.
        //

        MI_TALLY_TRANSITION_PAGE_ADDITION (Pfn1);

        if (Pfn1->OriginalPte.u.Soft.Prototype == 0) {
            ASSERT (Pfn1->OriginalPte.u.Soft.PageFileHigh == 0);
        }

        PsGetCurrentProcess()->ModifiedPageCount += 1;
        if (MmModifiedPageListHead.Total >= MmModifiedPageMaximum ) {

            //
            // Start the modified page writer.
            //

            KeSetEvent (&MmModifiedPageWriterEvent, 0, FALSE);
        }
    }
    else if (ListHead->ListName == ModifiedNoWritePageList) {
        MI_TALLY_TRANSITION_PAGE_ADDITION (Pfn1);
    }

    return;
}


VOID
FASTCALL
MiInsertStandbyListAtFront (
    IN PFN_NUMBER PageFrameIndex
    )

/*++

Routine Description:

    This procedure inserts a page at the front of the standby list.

Arguments:

    PageFrameIndex - Supplies the physical page number to insert in the
                     list.

Return Value:

    none.

Environment:

    Must be holding the PFN database mutex with APCs disabled.

--*/

{
    PFN_NUMBER first;
    IN PMMPFNLIST ListHead;
    PMMPFN Pfn1;
    PMMPFN Pfn2;

    MM_PFN_LOCK_ASSERT();
    ASSERT ((PageFrameIndex != 0) && (PageFrameIndex <= MmHighestPhysicalPage) &&
        (PageFrameIndex >= MmLowestPhysicalPage));

    //
    // Check to ensure the reference count for the page
    // is zero.
    //

    Pfn1 = MI_PFN_ELEMENT(PageFrameIndex);

    PERFINFO_INSERT_FRONT_STANDBY(PageFrameIndex);

#if DBG
    if (MmDebug & MM_DBG_PAGE_REF_COUNT) {

        PMMPTE PointerPte;
        KIRQL OldIrql = 99;

        if ((Pfn1->u3.e1.PrototypePte == 1)  &&
                (MmIsAddressValid (Pfn1->PteAddress))) {
            PointerPte = Pfn1->PteAddress;
        } else {

            //
            // The page containing the prototype PTE is not valid,
            // map the page into hyperspace and reference it that way.
            //

            PointerPte = MiMapPageInHyperSpace (Pfn1->PteFrame, &OldIrql);
            PointerPte = (PMMPTE)((PCHAR)PointerPte +
                                    MiGetByteOffset(Pfn1->PteAddress));
        }

        ASSERT ((MI_GET_PAGE_FRAME_FROM_TRANSITION_PTE (PointerPte) == PageFrameIndex) ||
                (MI_GET_PAGE_FRAME_FROM_PTE (PointerPte) == PageFrameIndex));
        ASSERT (PointerPte->u.Soft.Transition == 1);
        ASSERT (PointerPte->u.Soft.Prototype == 0);
        if (OldIrql != 99) {
            MiUnmapPageInHyperSpace (OldIrql)
        }
    }

    if ((Pfn1->OriginalPte.u.Soft.Prototype == 0) &&
       (Pfn1->OriginalPte.u.Soft.Transition == 1)) {
        KeBugCheckEx (MEMORY_MANAGEMENT, 0x8889, 0,0,0);
    }
#endif

    ASSERT (Pfn1->u3.e2.ReferenceCount == 0);
    ASSERT (Pfn1->u3.e1.PrototypePte == 1);
    MmTransitionSharedPages += 1;

    MmStandbyPageListHead.Total += 1;  // One more page on the list.

    ASSERT (MmTransitionPrivatePages + MmTransitionSharedPages == MmStandbyPageListHead.Total + MmModifiedPageListHead.Total + MmModifiedNoWritePageListHead.Total);

    ListHead = &MmStandbyPageListHead;

    first = ListHead->Flink;
    if (first == MM_EMPTY_LIST) {

        //
        // List is empty add the page to the ListHead.
        //

        ListHead->Blink = PageFrameIndex;
    } else {
        Pfn2 = MI_PFN_ELEMENT (first);
        Pfn2->u2.Blink = PageFrameIndex;
    }

    ListHead->Flink = PageFrameIndex;
    Pfn1->u2.Blink = MM_EMPTY_LIST;
    Pfn1->u1.Flink = first;
    Pfn1->u3.e1.PageLocation = StandbyPageList;

    //
    // If the page was placed on the free, standby or zeroed list,
    // update the count of usable pages in the system.  If the count
    // transitions from 0 to 1, the event associated with available
    // pages should become true.
    //

    MmAvailablePages += 1;

    //
    // A page has just become available, check to see if the
    // page wait events should be signalled.
    //

    if (MmAvailablePages == MM_LOW_LIMIT) {
        KeSetEvent (&MmAvailablePagesEvent, 0, FALSE);
    } else if (MmAvailablePages == MM_HIGH_LIMIT) {
        KeSetEvent (&MmAvailablePagesEventHigh, 0, FALSE);
    }

    return;
}

PFN_NUMBER  //PageFrameIndex
FASTCALL
MiRemovePageFromList (
    IN PMMPFNLIST ListHead
    )

/*++

Routine Description:

    This procedure removes a page from the head of the specified list (free,
    standby, zeroed, modified).

    This routine clears the flags word in the PFN database, hence the
    PFN information for this page must be initialized.

Arguments:

    ListHead - Supplies the list of the list in which to remove the
               specified physical page.

Return Value:

    The physical page number removed from the specified list.

Environment:

    Must be holding the PFN database mutex with APCs disabled.

--*/

{
    PFN_NUMBER PageFrameIndex;
    PMMPFN Pfn1;
    PMMPFN Pfn2;
    ULONG Color;

    MM_PFN_LOCK_ASSERT();

    //
    // If the specified list is empty return MM_EMPTY_LIST.
    //

    if (ListHead->Total == 0) {

        KdPrint(("MM:Attempting to remove page from empty list\n"));
        KeBugCheckEx (PFN_LIST_CORRUPT, 1, (ULONG_PTR)ListHead, MmAvailablePages, 0);
        return 0;
    }

    ASSERT (ListHead->ListName != ModifiedPageList);

    //
    // Decrement the count of pages on the list and remove the first
    // page from the list.
    //

    ListHead->Total -= 1;
    PageFrameIndex = ListHead->Flink;
    Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);

    PERFINFO_REMOVEPAGE(PageFrameIndex, PERFINFO_LOG_TYPE_REMOVEPAGEFROMLIST);

    ListHead->Flink = Pfn1->u1.Flink;

    //
    // Zero the flink and blink in the pfn database element.
    //

    Pfn1->u1.Flink = 0;         // Assumes Flink width is >= WsIndex width
    Pfn1->u2.Blink = 0;

    //
    // If the last page was removed (the ListHead->Flink is now
    // MM_EMPTY_LIST) make the listhead->Blink MM_EMPTY_LIST as well.
    //

    if (ListHead->Flink == MM_EMPTY_LIST) {
        ListHead->Blink = MM_EMPTY_LIST;
    } else {

        //
        // Make the PFN element point to MM_EMPTY_LIST signifying this
        // is the last page in the list.
        //

        Pfn2 = MI_PFN_ELEMENT (ListHead->Flink);
        Pfn2->u2.Blink = MM_EMPTY_LIST;
    }

    //
    // Check to see if we now have one less page available.
    //

    if (ListHead->ListName <= StandbyPageList) {
        MmAvailablePages -= 1;

        if (ListHead->ListName == StandbyPageList) {

            //
            // This page is currently in transition, restore the PTE to
            // its original contents so this page can be reused.
            //

            MI_TALLY_TRANSITION_PAGE_REMOVAL (Pfn1);
            MiRestoreTransitionPte (PageFrameIndex);
        }

        if (MmAvailablePages < MmMinimumFreePages) {

            //
            // Obtain free pages.
            //

            MiObtainFreePages();
        }
    }

    ASSERT ((PageFrameIndex != 0) &&
            (PageFrameIndex <= MmHighestPhysicalPage) &&
            (PageFrameIndex >= MmLowestPhysicalPage));

    //
    // Zero the PFN flags longword.
    //

    Color = Pfn1->u3.e1.PageColor;
    ASSERT (Pfn1->u3.e1.RemovalRequested == 0);
    Pfn1->u3.e2.ShortFlags = 0;
    Pfn1->u3.e1.PageColor = Color;
    Color = MI_GET_SECONDARY_COLOR (PageFrameIndex, Pfn1);

    if (ListHead->ListName <= FreePageList) {

        //
        // Update the color lists.
        //

        ASSERT (MmFreePagesByColor[ListHead->ListName][Color].Flink == PageFrameIndex);
        MmFreePagesByColor[ListHead->ListName][Color].Flink =
                                         (PFN_NUMBER) Pfn1->OriginalPte.u.Long;
    }

    return PageFrameIndex;
}

VOID
FASTCALL
MiUnlinkPageFromList (
    IN PMMPFN Pfn
    )

/*++

Routine Description:

    This procedure removes a page from the middle of a list.  This is
    designed for the faulting of transition pages from the standby and
    modified list and making them active and valid again.

Arguments:

    Pfn - Supplies a pointer to the PFN database element for the physical
          page to remove from the list.

Return Value:

    none.

Environment:

    Must be holding the PFN database mutex with APCs disabled.

--*/

{
    PMMPFNLIST ListHead;
    PFN_NUMBER Previous;
    PFN_NUMBER Next;
    PMMPFN Pfn2;

    MM_PFN_LOCK_ASSERT();

    PERFINFO_UNLINKPAGE((ULONG_PTR)(Pfn - MmPfnDatabase), Pfn->u3.e1.PageLocation);

    //
    // Page not on standby or modified list, check to see if the
    // page is currently being written by the modified page
    // writer, if so, just return this page.  The reference
    // count for the page will be incremented, so when the modified
    // page write completes, the page will not be put back on
    // the list, rather, it will remain active and valid.
    //

    if (Pfn->u3.e2.ReferenceCount > 0) {

        //
        // The page was not on any "transition lists", check to see
        // if this has I/O in progress.
        //

        if (Pfn->u2.ShareCount == 0) {
#if DBG
            if (MmDebug & MM_DBG_PAGE_IN_LIST) {
                DbgPrint("unlinking page not in list...\n");
                MiFormatPfn(Pfn);
            }
#endif
            return;
        }
        KdPrint(("MM:attempt to remove page from wrong page list\n"));
        KeBugCheckEx (PFN_LIST_CORRUPT,
                      2,
                      Pfn - MmPfnDatabase,
                      MmHighestPhysicalPage,
                      Pfn->u3.e2.ReferenceCount);
        return;
    }

    ListHead = MmPageLocationList[Pfn->u3.e1.PageLocation];

    //
    // Must not remove pages from free or zeroed without updating
    // the colored lists.
    //

    ASSERT (ListHead->ListName >= StandbyPageList);

    //
    // On MIPS R4000 modified pages destined for the paging file are
    // kept on separate lists which group pages of the same color
    // together
    //

    if ((ListHead == &MmModifiedPageListHead) &&
        (Pfn->OriginalPte.u.Soft.Prototype == 0)) {

        //
        // This page is destined for the paging file (not
        // a mapped file).  Change the list head to the
        // appropriate colored list head.
        //

        ListHead->Total -= 1;
        MmTotalPagesForPagingFile -= 1;
        ListHead = &MmModifiedPageListByColor [Pfn->u3.e1.PageColor];
    }

    ASSERT (Pfn->u3.e1.WriteInProgress == 0);
    ASSERT (Pfn->u3.e1.ReadInProgress == 0);
    ASSERT (ListHead->Total != 0);

    Next = Pfn->u1.Flink;
    Pfn->u1.Flink = 0;         // Assumes Flink width is >= WsIndex width
    Previous = Pfn->u2.Blink;
    Pfn->u2.Blink = 0;

    if (Next == MM_EMPTY_LIST) {
        ListHead->Blink = Previous;
    } else {
        Pfn2 = MI_PFN_ELEMENT(Next);
        Pfn2->u2.Blink = Previous;
    }

    if (Previous == MM_EMPTY_LIST) {
        ListHead->Flink = Next;
    } else {
        Pfn2 = MI_PFN_ELEMENT(Previous);
        Pfn2->u1.Flink = Next;
    }

    ListHead->Total -= 1;

    //
    // Check to see if we now have one less page available.
    //

    if (ListHead->ListName <= StandbyPageList) {
        MmAvailablePages -= 1;

        if (ListHead->ListName == StandbyPageList) {
            MI_TALLY_TRANSITION_PAGE_REMOVAL (Pfn);
        }

        if (MmAvailablePages < MmMinimumFreePages) {

            //
            // Obtain free pages.
            //

            MiObtainFreePages();

        }
    }
    else if (ListHead->ListName == ModifiedPageList || ListHead->ListName == ModifiedNoWritePageList) {
        MI_TALLY_TRANSITION_PAGE_REMOVAL (Pfn);
    }

    return;
}

VOID
MiUnlinkFreeOrZeroedPage (
    IN PFN_NUMBER Page
    )

/*++

Routine Description:

    This procedure removes a page from the middle of a list.  This is
    designed for the removing of free or zeroed pages from the middle of
    their lists.

Arguments:

    Pfn - Supplies a pointer to the PFN database element for the physical
          page to remove from the list.

Return Value:

    None.

Environment:

    Must be holding the PFN database mutex with APCs disabled.

--*/

{
    PMMPFNLIST ListHead;
    PFN_NUMBER Previous;
    PFN_NUMBER Next;
    PMMPFN Pfn2;
    PMMPFN Pfn;
    ULONG Color;

    Pfn = MI_PFN_ELEMENT (Page);

    MM_PFN_LOCK_ASSERT();

    ListHead = MmPageLocationList[Pfn->u3.e1.PageLocation];
    ASSERT (ListHead->Total != 0);
    ListHead->Total -= 1;

    ASSERT (ListHead->ListName <= FreePageList);
    ASSERT (Pfn->u3.e1.WriteInProgress == 0);
    ASSERT (Pfn->u3.e1.ReadInProgress == 0);

    PERFINFO_UNLINKFREEPAGE((ULONG_PTR)(Pfn - MmPfnDatabase), Pfn->u3.e1.PageLocation);

    Next = Pfn->u1.Flink;
    Pfn->u1.Flink = 0;         // Assumes Flink width is >= WsIndex width
    Previous = Pfn->u2.Blink;
    Pfn->u2.Blink = 0;

    if (Next == MM_EMPTY_LIST) {
        ListHead->Blink = Previous;
    } else {
        Pfn2 = MI_PFN_ELEMENT(Next);
        Pfn2->u2.Blink = Previous;
    }

    if (Previous == MM_EMPTY_LIST) {
        ListHead->Flink = Next;
    } else {
        Pfn2 = MI_PFN_ELEMENT(Previous);
        Pfn2->u1.Flink = Next;
    }

    //
    // We are removing a page from the middle of the free or zeroed page list.
    // The secondary color tables must be updated at this time.
    //

    Color = MI_GET_SECONDARY_COLOR (Page, Pfn);
    ASSERT (Pfn->u3.e1.PageColor == MI_GET_COLOR_FROM_SECONDARY(Color));

    //
    // Walk down the list and remove the page.
    //

    Next = MmFreePagesByColor[ListHead->ListName][Color].Flink;
    if (Next == Page) {
        MmFreePagesByColor[ListHead->ListName][Color].Flink =
                                                (PFN_NUMBER) Pfn->OriginalPte.u.Long;
    } else {

        //
        // Walk the list to find the parent.
        //

        for (; ; ) {
            Pfn2 = MI_PFN_ELEMENT (Next);
            Next = (PFN_NUMBER) Pfn2->OriginalPte.u.Long;
            if (Page == Next) {
                Pfn2->OriginalPte.u.Long = Pfn->OriginalPte.u.Long;
                if ((PFN_NUMBER) Pfn->OriginalPte.u.Long == MM_EMPTY_LIST) {
                    MmFreePagesByColor[ListHead->ListName][Color].Blink = Pfn2;
                }
                break;
            }
        }
    }

    MmAvailablePages -= 1;

    if (MmAvailablePages < MmMinimumFreePages) {

        //
        // Obtain free pages.
        //

        MiObtainFreePages();
    }

    return;
}


ULONG
FASTCALL
MiEnsureAvailablePageOrWait (
    IN PEPROCESS Process,
    IN PVOID VirtualAddress
    )

/*++

Routine Description:

    This procedure ensures that a physical page is available on
    the zeroed, free or standby list such that the next call the remove a
    page absolutely will not block.  This is necessary as blocking would
    require a wait which could cause a deadlock condition.

    If a page is available the function returns immediately with a value
    of FALSE indicating no wait operation was performed.  If no physical
    page is available, the thread enters a wait state and the function
    returns the value TRUE when the wait operation completes.

Arguments:

    Process - Supplies a pointer to the current process if, and only if,
              the working set mutex is held currently held and should
              be released if a wait operation is issued.  Supplies
              the value NULL otherwise.

    VirtualAddress - Supplies the virtual address for the faulting page.
                     If the value is NULL, the page is treated as a
                     user mode address.

Return Value:

    FALSE - if a page was immediately available.
    TRUE - if a wait operation occurred before a page became available.


Environment:

    Must be holding the PFN database mutex with APCs disabled.

--*/

{
    PVOID Event;
    NTSTATUS Status;
    KIRQL OldIrql;
    KIRQL Ignore;
    ULONG Limit;
    ULONG Relock;
    PFN_NUMBER StrandedPages;
    LOGICAL WsHeldSafe;
    PMMPFN Pfn1;
    PMMPFN EndPfn;
    LARGE_INTEGER WaitBegin;
    LARGE_INTEGER WaitEnd;

    MM_PFN_LOCK_ASSERT();

    if (MmAvailablePages >= MM_HIGH_LIMIT) {

        //
        // Pages are available.
        //

        return FALSE;
    }

    //
    // If this fault is for paged pool (or pagable kernel space,
    // including page table pages), let it use the last page.
    //

#if defined(_IA64_)
    if (MI_IS_SYSTEM_ADDRESS(VirtualAddress) ||
        (MI_IS_HYPER_SPACE_ADDRESS(VirtualAddress))) {
#else
    if (((PMMPTE)VirtualAddress > MiGetPteAddress(HYPER_SPACE)) ||
        ((VirtualAddress > MM_HIGHEST_USER_ADDRESS) &&
         (VirtualAddress < (PVOID)PTE_BASE))) {
#endif

        //
        // This fault is in the system, use 1 page as the limit.
        //

        if (MmAvailablePages >= MM_LOW_LIMIT) {

            //
            // Pages are available.
            //

            return FALSE;
        }

        Limit = MM_LOW_LIMIT;
        Event = (PVOID)&MmAvailablePagesEvent;
    } else {
        Limit = MM_HIGH_LIMIT;
        Event = (PVOID)&MmAvailablePagesEventHigh;
    }

    while (MmAvailablePages < Limit) {
        KeClearEvent ((PKEVENT)Event);

        UNLOCK_PFN (APC_LEVEL);

        if (Process == HYDRA_PROCESS) {
            UNLOCK_SESSION_SPACE_WS (APC_LEVEL);
        }
        else if (Process != NULL) {

            //
            // The working set lock may have been acquired safely or unsafely
            // by our caller.  Handle both cases here and below.
            //

            UNLOCK_WS_REGARDLESS (Process, WsHeldSafe);
        }
        else {
            Relock = FALSE;
            if (MmSystemLockOwner == PsGetCurrentThread()) {
                UNLOCK_SYSTEM_WS (APC_LEVEL);
                Relock = TRUE;
            }
        }

        KiQueryInterruptTime(&WaitBegin);

        //
        // Wait 7 minutes for pages to become available.
        //

        Status = KeWaitForSingleObject(Event,
                                       WrFreePage,
                                       KernelMode,
                                       FALSE,
                                       (PLARGE_INTEGER)&MmSevenMinutes);

        if (Status == STATUS_TIMEOUT) {

            KiQueryInterruptTime(&WaitEnd);

            //
            // See how many transition pages have nonzero reference counts as
            // these indicate drivers that aren't unlocking the pages in their
            // MDLs.
            //

            Limit = 0;
            StrandedPages = 0;

            do {
        
                Pfn1 = MI_PFN_ELEMENT (MmPhysicalMemoryBlock->Run[Limit].BasePage);
                EndPfn = Pfn1 + MmPhysicalMemoryBlock->Run[Limit].PageCount;

                while (Pfn1 < EndPfn) {
                    if ((Pfn1->u3.e1.PageLocation == TransitionPage) &&
                        (Pfn1->u3.e2.ReferenceCount != 0)) {
                            StrandedPages += 1;
                    }
                    Pfn1 += 1;
                }
                Limit += 1;
        
            } while (Limit != MmPhysicalMemoryBlock->NumberOfRuns);

            //
            // This bugcheck can occur for the following reasons:
            //
            // A driver has blocked, deadlocking the modified or mapped
            // page writers.  Examples of this include mutex deadlocks or
            // accesses to paged out memory in filesystem drivers, filter
            // drivers, etc.  This indicates a driver bug.
            //
            // The storage driver(s) are not processing requests.  Examples
            // of this are stranded queues, non-responding drives, etc.  This
            // indicates a driver bug.
            //
            // Not enough pool is available for the storage stack to write out
            // modified pages.  This indicates a driver bug.
            //
            // A high priority realtime thread has starved the balance set
            // manager from trimming pages and/or starved the modified writer
            // from writing them out.  This indicates a bug in the component
            // that created this thread.
            //
            // All the processes have been trimmed to their minimums and all
            // modified pages written, but still no memory is available.  The
            // freed memory must be stuck in transition pages with non-zero
            // reference counts - thus they cannot be put on the freelist.
            // A driver is neglecting to unlock the pages preventing the
            // reference counts from going to zero which would free the pages.
            // This may be due to transfers that never finish and the driver
            // never aborts or other driver bugs.
            //

            KeBugCheckEx (NO_PAGES_AVAILABLE,
                          MmModifiedPageListHead.Total,
                          MmTotalPagesForPagingFile,
                          (MmMaximumNonPagedPoolInBytes >> PAGE_SHIFT) - MmAllocatedNonPagedPool,
                          StrandedPages);

            if (!KdDebuggerNotPresent) {
                DbgPrint ("MmEnsureAvailablePageOrWait: 7 min timeout %x %x %x %x\n", WaitEnd.HighPart, WaitEnd.LowPart, WaitBegin.HighPart, WaitBegin.LowPart);
                DbgBreakPoint ();
            }
        }

        if (Process == HYDRA_PROCESS) {
            LOCK_SESSION_SPACE_WS (Ignore);
        }
        else if (Process != NULL) {

            //
            // The working set lock may have been acquired safely or unsafely
            // by our caller.  Reacquire it in the same manner our caller did.
            //

            LOCK_WS_REGARDLESS (Process, WsHeldSafe);
        }
        else {
            if (Relock) {
                LOCK_SYSTEM_WS (Ignore);
            }
        }

        LOCK_PFN (OldIrql);
    }

    return TRUE;
}


PFN_NUMBER  //PageFrameIndex
FASTCALL
MiRemoveZeroPage (
    IN ULONG PageColor
    )

/*++

Routine Description:

    This procedure removes a zero page from either the zeroed, free
    or standby lists (in that order).  If no pages exist on the zeroed
    or free list a transition page is removed from the standby list
    and the PTE (may be a prototype PTE) which refers to this page is
    changed from transition back to its original contents.

    If the page is not obtained from the zeroed list, it is zeroed.

Arguments:

    PageColor - Supplies the page color for which this page is destined.
                This is used for checking virtual address alignments to
                determine if the D cache needs flushing before the page
                can be reused.

Return Value:

    The physical page number removed from the specified list.

Environment:

    Must be holding the PFN database mutex with APCs disabled.

--*/

{
    PFN_NUMBER Page;
    PMMPFN Pfn1;
    ULONG Color;

    MM_PFN_LOCK_ASSERT();
    ASSERT(MmAvailablePages != 0);

    //
    // Attempt to remove a page from the zeroed page list. If a page
    // is available, then remove it and return its page frame index.
    // Otherwise, attempt to remove a page from the free page list or
    // the standby list.
    //
    // N.B. It is not necessary to change page colors even if the old
    //      color is not equal to the new color. The zero page thread
    //      ensures that all zeroed pages are removed from all caches.
    //

    if (MmFreePagesByColor[ZeroedPageList][PageColor].Flink != MM_EMPTY_LIST) {

        //
        // Remove the first entry on the zeroed by color list.
        //

        Page = MmFreePagesByColor[ZeroedPageList][PageColor].Flink;

#if DBG
        Pfn1 = MI_PFN_ELEMENT(Page);
        ASSERT (Pfn1->u3.e1.PageLocation == ZeroedPageList);
        ASSERT (Pfn1->PteFrame != MI_MAGIC_AWE_PTEFRAME);
#endif

        MiRemovePageByColor (Page, PageColor);

#if DBG
        ASSERT (Pfn1->u3.e1.PageColor == (PageColor & MM_COLOR_MASK));
        ASSERT (Pfn1->u3.e2.ReferenceCount == 0);
        ASSERT (Pfn1->u2.ShareCount == 0);
        ASSERT (Pfn1->PteFrame != MI_MAGIC_AWE_PTEFRAME);
#endif
        return Page;

    }

    //
    // No previously zeroed page with the specified secondary color exists.
    // Try a zeroed page of the primary color.
    //

    if  (MmZeroedPageListHead.Flink != MM_EMPTY_LIST) {
        Page = MmZeroedPageListHead.Flink;
#if DBG
        Pfn1 = MI_PFN_ELEMENT(Page);
        ASSERT (Pfn1->u3.e1.PageLocation == ZeroedPageList);
        ASSERT (Pfn1->PteFrame != MI_MAGIC_AWE_PTEFRAME);
#endif
        Color = MI_GET_SECONDARY_COLOR (Page, MI_PFN_ELEMENT(Page));
        MiRemovePageByColor (Page, Color);
#if DBG
        Pfn1 = MI_PFN_ELEMENT(Page);
        ASSERT (Pfn1->u3.e1.PageColor == (PageColor & MM_COLOR_MASK));
        ASSERT (Pfn1->u3.e2.ReferenceCount == 0);
        ASSERT (Pfn1->u2.ShareCount == 0);
        ASSERT (Pfn1->PteFrame != MI_MAGIC_AWE_PTEFRAME);
#endif
        return Page;
    }

    //
    // No zeroed page of the primary color exists, try a free page of the
    // secondary color.
    //

    if (MmFreePagesByColor[FreePageList][PageColor].Flink != MM_EMPTY_LIST) {

        //
        // Remove the first entry on the free list by color.
        //

        Page = MmFreePagesByColor[FreePageList][PageColor].Flink;

#if DBG
        Pfn1 = MI_PFN_ELEMENT(Page);
        ASSERT (Pfn1->u3.e1.PageLocation == FreePageList);
        ASSERT (Pfn1->PteFrame != MI_MAGIC_AWE_PTEFRAME);
#endif

        MiRemovePageByColor (Page, PageColor);
#if DBG
        Pfn1 = MI_PFN_ELEMENT(Page);
        ASSERT (Pfn1->u3.e1.PageColor == (PageColor & MM_COLOR_MASK));
        ASSERT (Pfn1->u3.e2.ReferenceCount == 0);
        ASSERT (Pfn1->u2.ShareCount == 0);
        ASSERT (Pfn1->PteFrame != MI_MAGIC_AWE_PTEFRAME);
#endif
        goto ZeroPage;
    }

    if  (MmFreePageListHead.Flink != MM_EMPTY_LIST) {
        Page = MmFreePageListHead.Flink;

        Color = MI_GET_SECONDARY_COLOR (Page, MI_PFN_ELEMENT(Page));
        MiRemovePageByColor (Page, Color);
#if DBG
        Pfn1 = MI_PFN_ELEMENT(Page);
        ASSERT (Pfn1->u3.e1.PageColor == (PageColor & MM_COLOR_MASK));
        ASSERT (Pfn1->u3.e2.ReferenceCount == 0);
        ASSERT (Pfn1->u2.ShareCount == 0);
        ASSERT (Pfn1->PteFrame != MI_MAGIC_AWE_PTEFRAME);
#endif
        goto ZeroPage;
    }

    ASSERT (MmZeroedPageListHead.Total == 0);
    ASSERT (MmFreePageListHead.Total == 0);

    if (MmZeroedPageListHead.Total != 0) {

        Page = MiRemovePageFromList(&MmZeroedPageListHead);
        MI_CHECK_PAGE_ALIGNMENT(Page, PageColor & MM_COLOR_MASK);

    } else {

        //
        // Attempt to remove a page from the free list. If a page is
        // available, then remove  it. Otherwise, attempt to remove a
        // page from the standby list.
        //

        if (MmFreePageListHead.Total != 0) {
            Page = MiRemovePageFromList(&MmFreePageListHead);
            ASSERT ((MI_PFN_ELEMENT(Page))->PteFrame != MI_MAGIC_AWE_PTEFRAME);
        } else {

            //
            // Remove a page from the standby list and restore the original
            // contents of the PTE to free the last reference to the physical
            // page.
            //

            ASSERT (MmStandbyPageListHead.Total != 0);

            Page = MiRemovePageFromList(&MmStandbyPageListHead);
            ASSERT ((MI_PFN_ELEMENT(Page))->PteFrame != MI_MAGIC_AWE_PTEFRAME);
        }

        //
        // Zero the page removed from the free or standby list.
        //

ZeroPage:

        Pfn1 = MI_PFN_ELEMENT(Page);

#if defined(_ALPHA_)
        HalZeroPage((PVOID)ULongToPtr((PageColor & MM_COLOR_MASK) << PAGE_SHIFT),
                    (PVOID)ULongToPtr((Pfn1->u3.e1.PageColor) << PAGE_SHIFT),
                    Page);
#else
        MiZeroPhysicalPage (Page, 0);
#endif

        //
        // Note the stamping must occur after the page is zeroed.
        //

        MI_BARRIER_STAMP_ZEROED_PAGE (&Pfn1->PteFrame);

        Pfn1->u3.e1.PageColor = PageColor & MM_COLOR_MASK;

    }

#if DBG
    Pfn1 = MI_PFN_ELEMENT (Page);
    ASSERT (Pfn1->u3.e2.ReferenceCount == 0);
    ASSERT (Pfn1->u2.ShareCount == 0);
    ASSERT (Pfn1->PteFrame != MI_MAGIC_AWE_PTEFRAME);
#endif

    return Page;
}

PFN_NUMBER  //PageFrameIndex
FASTCALL
MiRemoveAnyPage (
    IN ULONG PageColor
    )

/*++

Routine Description:

    This procedure removes a page from either the free, zeroed,
    or standby lists (in that order).  If no pages exist on the zeroed
    or free list a transition page is removed from the standby list
    and the PTE (may be a prototype PTE) which refers to this page is
    changed from transition back to its original contents.

    Note pages MUST exist to satisfy this request.  The caller ensures this
    by first calling MiEnsureAvailablePageOrWait.

Arguments:

    PageColor - Supplies the page color for which this page is destined.
                This is used for checking virtual address alignments to
                determine if the D cache needs flushing before the page
                can be reused.

Return Value:

    The physical page number removed from the specified list.

Environment:

    Must be holding the PFN database mutex with APCs disabled.

--*/

{
    PFN_NUMBER Page;
    PMMPFN Pfn1;
    ULONG Color;

    MM_PFN_LOCK_ASSERT();
    ASSERT(MmAvailablePages != 0);

    //
    // Check the free page list, and if a page is available
    // remove it and return its value.
    //

    if (MmFreePagesByColor[FreePageList][PageColor].Flink != MM_EMPTY_LIST) {

        //
        // Remove the first entry on the free by color list.
        //

        Page = MmFreePagesByColor[FreePageList][PageColor].Flink;
#if DBG
        Pfn1 = MI_PFN_ELEMENT(Page);
        ASSERT (Pfn1->u3.e1.PageLocation == FreePageList);
        ASSERT (Pfn1->u3.e1.PageColor == (PageColor & MM_COLOR_MASK));
        ASSERT (Pfn1->PteFrame != MI_MAGIC_AWE_PTEFRAME);
#endif
        MiRemovePageByColor (Page, PageColor);
#if DBG
        ASSERT (Pfn1->u3.e1.PageColor == (PageColor & MM_COLOR_MASK));
        ASSERT (Pfn1->u3.e2.ReferenceCount == 0);
        ASSERT (Pfn1->u2.ShareCount == 0);
        ASSERT (Pfn1->PteFrame != MI_MAGIC_AWE_PTEFRAME);
#endif
        return Page;

    }

    if (MmFreePagesByColor[ZeroedPageList][PageColor].Flink
                                                        != MM_EMPTY_LIST) {

        //
        // Remove the first entry on the zeroed by color list.
        //

        Page = MmFreePagesByColor[ZeroedPageList][PageColor].Flink;
#if DBG
        Pfn1 = MI_PFN_ELEMENT(Page);
        ASSERT (Pfn1->u3.e1.PageColor == (PageColor & MM_COLOR_MASK));
        ASSERT (Pfn1->u3.e1.PageLocation == ZeroedPageList);
        ASSERT (Pfn1->PteFrame != MI_MAGIC_AWE_PTEFRAME);
#endif

        MiRemovePageByColor (Page, PageColor);
        ASSERT (Pfn1->PteFrame != MI_MAGIC_AWE_PTEFRAME);
        return Page;
    }

    //
    // Try the free page list by primary color.
    //

    if  (MmFreePageListHead.Flink != MM_EMPTY_LIST) {
        Page = MmFreePageListHead.Flink;
        Color = MI_GET_SECONDARY_COLOR (Page, MI_PFN_ELEMENT(Page));

#if DBG
        Pfn1 = MI_PFN_ELEMENT(Page);
        ASSERT (Pfn1->u3.e1.PageColor == (PageColor & MM_COLOR_MASK));
        ASSERT (Pfn1->u3.e1.PageLocation == FreePageList);
        ASSERT (Pfn1->PteFrame != MI_MAGIC_AWE_PTEFRAME);
#endif
        MiRemovePageByColor (Page, Color);
#if DBG
        ASSERT (Pfn1->u3.e1.PageColor == (PageColor & MM_COLOR_MASK));
        ASSERT (Pfn1->u3.e2.ReferenceCount == 0);
        ASSERT (Pfn1->u2.ShareCount == 0);
        ASSERT (Pfn1->PteFrame != MI_MAGIC_AWE_PTEFRAME);
#endif
        return Page;

    }

    if (MmZeroedPageListHead.Flink != MM_EMPTY_LIST) {
        Page = MmZeroedPageListHead.Flink;
        Color = MI_GET_SECONDARY_COLOR (Page, MI_PFN_ELEMENT(Page));
        MiRemovePageByColor (Page, Color);
#if DBG
        Pfn1 = MI_PFN_ELEMENT(Page);
        ASSERT (Pfn1->u3.e1.PageColor == (PageColor & MM_COLOR_MASK));
        ASSERT (Pfn1->u3.e2.ReferenceCount == 0);
        ASSERT (Pfn1->u2.ShareCount == 0);
        ASSERT (Pfn1->PteFrame != MI_MAGIC_AWE_PTEFRAME);
#endif
        return Page;
    }

    if (MmFreePageListHead.Total != 0) {

        Page = MiRemovePageFromList(&MmFreePageListHead);
        ASSERT ((MI_PFN_ELEMENT(Page))->PteFrame != MI_MAGIC_AWE_PTEFRAME);

    } else {

        //
        // Check the zeroed page list, and if a page is available
        // remove it and return its value.
        //

        if (MmZeroedPageListHead.Total != 0) {

            Page = MiRemovePageFromList(&MmZeroedPageListHead);
            ASSERT ((MI_PFN_ELEMENT(Page))->PteFrame != MI_MAGIC_AWE_PTEFRAME);

        } else {

            //
            // No pages exist on the free or zeroed list, use the
            // standby list.
            //

            ASSERT(MmStandbyPageListHead.Total != 0);

            Page = MiRemovePageFromList(&MmStandbyPageListHead);
            ASSERT ((MI_PFN_ELEMENT(Page))->PteFrame != MI_MAGIC_AWE_PTEFRAME);
        }
    }

    MI_CHECK_PAGE_ALIGNMENT(Page, PageColor & MM_COLOR_MASK);
#if DBG
    Pfn1 = MI_PFN_ELEMENT (Page);
    ASSERT (Pfn1->u3.e2.ReferenceCount == 0);
    ASSERT (Pfn1->u2.ShareCount == 0);
#endif
    return Page;
}


VOID
MiRemovePageByColor (
    IN PFN_NUMBER Page,
    IN ULONG Color
    )

/*++

Routine Description:

    This procedure removes a page from the middle of the free or
    zeroed page list.

Arguments:

    PageFrameIndex - Supplies the physical page number to unlink from the
                     list.

Return Value:

    none.

Environment:

    Must be holding the PFN database mutex with APCs disabled.

--*/

{
    PMMPFNLIST ListHead;
    PMMPFNLIST PrimaryListHead;
    PFN_NUMBER Previous;
    PFN_NUMBER Next;
    PMMPFN Pfn1;
    PMMPFN Pfn2;
    ULONG PrimaryColor;

    MM_PFN_LOCK_ASSERT();

    Pfn1 = MI_PFN_ELEMENT (Page);
    PrimaryColor = Pfn1->u3.e1.PageColor;

    PERFINFO_REMOVEPAGE(Page, PERFINFO_LOG_TYPE_REMOVEPAGEBYCOLOR);

    ListHead = MmPageLocationList[Pfn1->u3.e1.PageLocation];

    ListHead->Total -= 1;

    PrimaryListHead = ListHead;

#if PFN_CONSISTENCY
    if (MmFreePagesByColor[PrimaryListHead->ListName][Color].Flink != Page) {

        KeBugCheckEx (PFN_LIST_CORRUPT,
                      0x9A,
                      Page,
                      Color,
                      (PFN_NUMBER)&MmFreePagesByColor[PrimaryListHead->ListName][Color].Flink);
    }
#endif

    Next = Pfn1->u1.Flink;
    Pfn1->u1.Flink = 0;         // Assumes Flink width is >= WsIndex width
    Previous = Pfn1->u2.Blink;
    Pfn1->u2.Blink = 0;

    if (Next == MM_EMPTY_LIST) {
        PrimaryListHead->Blink = Previous;
    } else {
        Pfn2 = MI_PFN_ELEMENT(Next);
        Pfn2->u2.Blink = Previous;
    }

    if (Previous == MM_EMPTY_LIST) {
        PrimaryListHead->Flink = Next;
    } else {
        Pfn2 = MI_PFN_ELEMENT(Previous);
        Pfn2->u1.Flink = Next;
    }

    //
    // Zero the flags longword, but keep the color information.
    //

    ASSERT (Pfn1->u3.e1.RemovalRequested == 0);
    Pfn1->u3.e2.ShortFlags = 0;
    Pfn1->u3.e1.PageColor = PrimaryColor;

    //
    // Update the color lists.
    //

    MmFreePagesByColor[ListHead->ListName][Color].Flink =
                                        (PFN_NUMBER) Pfn1->OriginalPte.u.Long;

    //
    // Note that we now have one less page available.
    //

    MmAvailablePages -= 1;

    if (MmAvailablePages < MmMinimumFreePages) {

        //
        // Obtain free pages.
        //

        MiObtainFreePages();

    }

    return;
}


VOID
FASTCALL
MiInsertFrontModifiedNoWrite (
    IN PFN_NUMBER PageFrameIndex
    )

/*++

Routine Description:

    This procedure inserts a page at the FRONT of the modified no
    write list.

Arguments:

    PageFrameIndex - Supplies the physical page number to insert in the
                     list.

Return Value:

    none.

Environment:

    Must be holding the PFN database mutex with APCs disabled.

--*/

{
    PFN_NUMBER first;
    PMMPFN Pfn1;
    PMMPFN Pfn2;

    MM_PFN_LOCK_ASSERT();
    ASSERT ((PageFrameIndex != 0) && (PageFrameIndex <= MmHighestPhysicalPage) &&
        (PageFrameIndex >= MmLowestPhysicalPage));

    //
    // Check to ensure the reference count for the page
    // is zero.
    //

    Pfn1 = MI_PFN_ELEMENT(PageFrameIndex);

    ASSERT (Pfn1->u3.e2.ReferenceCount == 0);

    MmModifiedNoWritePageListHead.Total += 1;  // One more page on the list.

    MI_TALLY_TRANSITION_PAGE_ADDITION (Pfn1);

    first = MmModifiedNoWritePageListHead.Flink;
    if (first == MM_EMPTY_LIST) {

        //
        // List is empty add the page to the ListHead.
        //

        MmModifiedNoWritePageListHead.Blink = PageFrameIndex;
    } else {
        Pfn2 = MI_PFN_ELEMENT (first);
        Pfn2->u2.Blink = PageFrameIndex;
    }

    MmModifiedNoWritePageListHead.Flink = PageFrameIndex;
    Pfn1->u1.Flink = first;
    Pfn1->u2.Blink = MM_EMPTY_LIST;
    Pfn1->u3.e1.PageLocation = ModifiedNoWritePageList;
    return;
}
