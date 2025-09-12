/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            ntoskrnl/mm/ARM3/zeropage.c
 * PURPOSE:         ARM Memory Manager Zero Page Thread Support
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

#define MODULE_INVOLVED_IN_ARM3
#include <mm/ARM3/miarm.h>

/* GLOBALS ********************************************************************/

KEVENT MmZeroingPageEvent;

/* PRIVATE FUNCTIONS **********************************************************/

VOID
NTAPI
MiFindInitializationCode(OUT PVOID *StartVa,
OUT PVOID *EndVa);

VOID
NTAPI
MiFreeInitializationCode(IN PVOID StartVa,
IN PVOID EndVa);

VOID
NTAPI
MmZeroPageThread(VOID)
{
    PKTHREAD Thread = KeGetCurrentThread();
    PVOID StartAddress, EndAddress;
    PVOID WaitObjects[2];

    /* Get the discardable sections to free them */
    MiFindInitializationCode(&StartAddress, &EndAddress);
    if (StartAddress) MiFreeInitializationCode(StartAddress, EndAddress);
    DPRINT("Free pages: %lx\n", MmAvailablePages);

    /* Set our priority to 0 */
    Thread->BasePriority = 0;
    KeSetPriorityThread(Thread, 0);

    /* Setup the wait objects */
    WaitObjects[0] = &MmZeroingPageEvent;
//    WaitObjects[1] = &PoSystemIdleTimer; FIXME: Implement idle timer

    while (TRUE)
    {
        KIRQL OldIrql;

        KeWaitForMultipleObjects(1, // 2
                                 WaitObjects,
                                 WaitAny,
                                 WrFreePage,
                                 KernelMode,
                                 FALSE,
                                 NULL,
                                 NULL);
        OldIrql = MiAcquirePfnLock();

        while (TRUE)
        {
            ULONG PageCount = 0;
            PMMPFN Pfn1 = (PMMPFN)LIST_HEAD;
            PVOID ZeroAddress;
            PFN_NUMBER PageIndex, FreePage;

            while (PageCount < MI_ZERO_PTES)
            {
                PMMPFN Pfn2;

                if (!MmFreePageListHead.Total)
                    break;

                PageIndex = MmFreePageListHead.Flink;
                ASSERT(PageIndex != LIST_HEAD);
                MI_SET_USAGE(MI_USAGE_ZERO_LOOP);
                MI_SET_PROCESS2("Kernel 0 Loop");
                FreePage = MiRemoveAnyPage(MI_GET_PAGE_COLOR(PageIndex));

                /* The first global free page should also be the first on its own list */
                if (FreePage != PageIndex)
                {
                    KeBugCheckEx(PFN_LIST_CORRUPT,
                                0x8F,
                                FreePage,
                                PageIndex,
                                0);
                }

                Pfn2 = MiGetPfnEntry(PageIndex);
                Pfn2->u1.Flink = (PFN_NUMBER)Pfn1;
                Pfn1 = Pfn2;
                PageCount++;
            }
            MiReleasePfnLock(OldIrql);

            if (PageCount == 0)
            {
                KeClearEvent(&MmZeroingPageEvent);
                break;
            }

            ZeroAddress = MiMapPagesInZeroSpace(Pfn1, PageCount);
            ASSERT(ZeroAddress);
            KeZeroPages(ZeroAddress, PageCount * PAGE_SIZE);
            MiUnmapPagesInZeroSpace(ZeroAddress, PageCount);

            OldIrql = MiAcquirePfnLock();

            while (Pfn1 != (PMMPFN)LIST_HEAD)
            {
                PageIndex = MiGetPfnEntryIndex(Pfn1);
                Pfn1 = (PMMPFN)Pfn1->u1.Flink;
                MiInsertPageInList(&MmZeroedPageListHead, PageIndex);
            }
        }
    }
}

/* EOF */
