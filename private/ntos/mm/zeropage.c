/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    zeropage.c

Abstract:

    This module contains the zero page thread for memory management.

Author:

    Lou Perazzoli (loup) 6-Apr-1991

Revision History:

--*/

#include "mi.h"

#define MM_ZERO_PAGE_OBJECT     0
#define PO_SYS_IDLE_OBJECT      1
#define NUMBER_WAIT_OBJECTS     2


VOID
MmZeroPageThread (
    VOID
    )

/*++

Routine Description:

    Implements the NT zeroing page thread.  This thread runs
    at priority zero and removes a page from the free list,
    zeroes it, and places it on the zeroed page list.

Arguments:

    StartContext - not used.

Return Value:

    None.

Environment:

    Kernel mode.

--*/

{
    PVOID EndVa;
    KIRQL OldIrql;
    PFN_NUMBER PageFrame;
    PMMPFN Pfn1;
    PVOID StartVa;
    PKTHREAD Thread;
    PVOID ZeroBase;
    PFN_NUMBER NewPage;
    PVOID WaitObjects[NUMBER_WAIT_OBJECTS];
    NTSTATUS Status;

    //
    // Before this becomes the zero page thread, free the kernel
    // initialization code.
    //

#if !defined(_IA64_)
    MiFindInitializationCode (&StartVa, &EndVa);
    if (StartVa != NULL) {
        MiFreeInitializationCode (StartVa, EndVa);
    }
#endif

    //
    // The following code sets the current thread's base priority to zero
    // and then sets its current priority to zero. This ensures that the
    // thread always runs at a priority of zero.
    //

    Thread = KeGetCurrentThread();
    Thread->BasePriority = 0;
    KeSetPriorityThread (Thread, 0);

    //
    // Initialize wait object array for multiple wait
    //

    WaitObjects[MM_ZERO_PAGE_OBJECT] = &MmZeroingPageEvent;
    WaitObjects[PO_SYS_IDLE_OBJECT] = &PoSystemIdleTimer;

    //
    // Loop forever zeroing pages.
    //

    do {

        //
        // Wait until there are at least MmZeroPageMinimum pages
        // on the free list.
        //

        Status = KeWaitForMultipleObjects (NUMBER_WAIT_OBJECTS,
                                           WaitObjects,
                                           WaitAny,
                                           WrFreePage,
                                           KernelMode,
                                           FALSE,
                                           (PLARGE_INTEGER) NULL,
                                           (PKWAIT_BLOCK) NULL
                                           );

        if (Status == PO_SYS_IDLE_OBJECT) {
            PoSystemIdleWorker (TRUE);
            continue;
        }

        LOCK_PFN_WITH_TRY (OldIrql);
        do {
            if ((volatile)MmFreePageListHead.Total == 0) {

                //
                // No pages on the free list at this time, wait for
                // some more.
                //

                MmZeroingPageThreadActive = FALSE;
                UNLOCK_PFN (OldIrql);
                break;

            } else {

                PageFrame = MmFreePageListHead.Flink;
                Pfn1 = MI_PFN_ELEMENT(PageFrame);

                ASSERT (PageFrame != MM_EMPTY_LIST);
                Pfn1 = MI_PFN_ELEMENT(PageFrame);

                NewPage = MiRemoveAnyPage (MI_GET_SECONDARY_COLOR (PageFrame, Pfn1));
                if (NewPage != PageFrame) {

                    //
                    // Someone has removed a page from the colored lists chain
                    // without updating the freelist chain.
                    //

                    KeBugCheckEx (PFN_LIST_CORRUPT,
                                  0x8F,
                                  NewPage,
                                  PageFrame,
                                  0);
                }

                //
                // Zero the page using the last color used to map the page.
                //

#if defined(_AXP64_) || defined(_X86_) || defined(_IA64_)

                ZeroBase = MiMapPageToZeroInHyperSpace (PageFrame);
                UNLOCK_PFN (OldIrql);

#if defined(_X86_)

                KeZeroPageFromIdleThread(ZeroBase);

#else  //X86

                RtlZeroMemory (ZeroBase, PAGE_SIZE);

#endif //X86

#else  //AXP64||X86||IA64

                ZeroBase = (PVOID)(Pfn1->u3.e1.PageColor << PAGE_SHIFT);
                UNLOCK_PFN (OldIrql);
                HalZeroPage(ZeroBase, ZeroBase, PageFrame);

#endif //AXP64||X86||IA64

                LOCK_PFN_WITH_TRY (OldIrql);
                MiInsertPageInList (MmPageLocationList[ZeroedPageList],
                                    PageFrame);
            }
        } while(TRUE);
    } while (TRUE);
}
