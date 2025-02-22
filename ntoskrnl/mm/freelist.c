/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/mm/freelist.c
 * PURPOSE:         Handle the list of free physical pages
 *
 * PROGRAMMERS:     David Welch (welch@cwcom.net)
 *                  Robert Bergkvist
 */

/* INCLUDES ****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

#define MODULE_INVOLVED_IN_ARM3
#include "ARM3/miarm.h"

#define ASSERT_IS_ROS_PFN(x) ASSERT(MI_IS_ROS_PFN(x) == TRUE);

/* GLOBALS ****************************************************************/

PMMPFN MmPfnDatabase;

PFN_NUMBER MmAvailablePages;
PFN_NUMBER MmResidentAvailablePages;
PFN_NUMBER MmResidentAvailableAtInit;

SIZE_T MmTotalCommittedPages;
SIZE_T MmSharedCommit;
SIZE_T MmDriverCommit;
SIZE_T MmProcessCommit;
SIZE_T MmPagedPoolCommit;
SIZE_T MmPeakCommitment;
SIZE_T MmtotalCommitLimitMaximum;

PMMPFN FirstUserLRUPfn;
PMMPFN LastUserLRUPfn;

/* FUNCTIONS *************************************************************/

PFN_NUMBER
NTAPI
MmGetLRUFirstUserPage(VOID)
{
    PFN_NUMBER Page;
    KIRQL OldIrql;

    /* Find the first user page */
    OldIrql = MiAcquirePfnLock();

    if (FirstUserLRUPfn == NULL)
    {
        MiReleasePfnLock(OldIrql);
        return 0;
    }

    Page = MiGetPfnEntryIndex(FirstUserLRUPfn);
    MmReferencePage(Page);

    MiReleasePfnLock(OldIrql);

    return Page;
}

static
VOID
MmInsertLRULastUserPage(PFN_NUMBER Page)
{
    MI_ASSERT_PFN_LOCK_HELD();

    PMMPFN Pfn = MiGetPfnEntry(Page);

    if (FirstUserLRUPfn == NULL)
        FirstUserLRUPfn = Pfn;

    Pfn->PreviousLRU = LastUserLRUPfn;

    if (LastUserLRUPfn != NULL)
        LastUserLRUPfn->NextLRU = Pfn;
    LastUserLRUPfn = Pfn;
}

static
VOID
MmRemoveLRUUserPage(PFN_NUMBER Page)
{
    MI_ASSERT_PFN_LOCK_HELD();

    /* Unset the page as a user page */
    ASSERT(Page != 0);

    PMMPFN Pfn = MiGetPfnEntry(Page);

    ASSERT_IS_ROS_PFN(Pfn);

    if (Pfn->PreviousLRU)
    {
        ASSERT(Pfn->PreviousLRU->NextLRU == Pfn);
        Pfn->PreviousLRU->NextLRU = Pfn->NextLRU;
    }
    else
    {
        ASSERT(FirstUserLRUPfn == Pfn);
        FirstUserLRUPfn = Pfn->NextLRU;
    }

    if (Pfn->NextLRU)
    {
        ASSERT(Pfn->NextLRU->PreviousLRU == Pfn);
        Pfn->NextLRU->PreviousLRU = Pfn->PreviousLRU;
    }
    else
    {
        ASSERT(Pfn == LastUserLRUPfn);
        LastUserLRUPfn = Pfn->PreviousLRU;
    }

    Pfn->PreviousLRU = Pfn->NextLRU = NULL;
}

PFN_NUMBER
NTAPI
MmGetLRUNextUserPage(PFN_NUMBER PreviousPage, BOOLEAN MoveToLast)
{
    PFN_NUMBER Page = 0;
    KIRQL OldIrql;

    /* Find the next user page */
    OldIrql = MiAcquirePfnLock();

    PMMPFN PreviousPfn = MiGetPfnEntry(PreviousPage);
    PMMPFN NextPfn = PreviousPfn->NextLRU;

    /*
     * Move this one at the end of the list.
     * It may be freed by MmDereferencePage below.
     * If it's not, then it means it is still hanging in some process address space.
     * This avoids paging-out e.g. ntdll early just because it's mapped first time.
     */
    if ((MoveToLast) && (MmGetReferenceCountPage(PreviousPage) > 1))
    {
        MmRemoveLRUUserPage(PreviousPage);
        MmInsertLRULastUserPage(PreviousPage);
    }

    if (NextPfn)
    {
        Page = MiGetPfnEntryIndex(NextPfn);
        MmReferencePage(Page);
    }

    MmDereferencePage(PreviousPage);

    MiReleasePfnLock(OldIrql);

    return Page;
}

BOOLEAN
NTAPI
MiIsPfnFree(IN PMMPFN Pfn1)
{
    /* Must be a free or zero page, with no references, linked */
    return ((Pfn1->u3.e1.PageLocation <= StandbyPageList) &&
            (Pfn1->u1.Flink) &&
            (Pfn1->u2.Blink) &&
            !(Pfn1->u3.e2.ReferenceCount));
}

BOOLEAN
NTAPI
MiIsPfnInUse(IN PMMPFN Pfn1)
{
    /* Standby list or higher, unlinked, and with references */
    return !MiIsPfnFree(Pfn1);
}

PMDL
NTAPI
MiAllocatePagesForMdl(IN PHYSICAL_ADDRESS LowAddress,
                      IN PHYSICAL_ADDRESS HighAddress,
                      IN PHYSICAL_ADDRESS SkipBytes,
                      IN SIZE_T TotalBytes,
                      IN MI_PFN_CACHE_ATTRIBUTE CacheAttribute,
                      IN ULONG MdlFlags)
{
    PMDL Mdl;
    PFN_NUMBER PageCount, LowPage, HighPage, SkipPages, PagesFound = 0, Page;
    PPFN_NUMBER MdlPage, LastMdlPage;
    KIRQL OldIrql;
    PMMPFN Pfn1;
    INT LookForZeroedPages;

    ASSERT(KeGetCurrentIrql() <= APC_LEVEL);
    DPRINT("ARM3-DEBUG: Being called with %I64x %I64x %I64x %lx %d %lu\n", LowAddress, HighAddress, SkipBytes, TotalBytes, CacheAttribute, MdlFlags);

    //
    // Convert the low address into a PFN
    //
    LowPage = (PFN_NUMBER)(LowAddress.QuadPart >> PAGE_SHIFT);

    //
    // Convert, and normalize, the high address into a PFN
    //
    HighPage = (PFN_NUMBER)(HighAddress.QuadPart >> PAGE_SHIFT);
    if (HighPage > MmHighestPhysicalPage) HighPage = MmHighestPhysicalPage;

    //
    // Validate skipbytes and convert them into pages
    //
    if (BYTE_OFFSET(SkipBytes.LowPart)) return NULL;
    SkipPages = (PFN_NUMBER)(SkipBytes.QuadPart >> PAGE_SHIFT);

    /* This isn't supported at all */
    if (SkipPages) DPRINT1("WARNING: Caller requesting SkipBytes, MDL might be mismatched\n");

    //
    // Now compute the number of pages the MDL will cover
    //
    PageCount = (PFN_NUMBER)ADDRESS_AND_SIZE_TO_SPAN_PAGES(0, TotalBytes);
    do
    {
        //
        // Try creating an MDL for these many pages
        //
        Mdl = MmCreateMdl(NULL, NULL, PageCount << PAGE_SHIFT);
        if (Mdl) break;

        //
        // This function is not required to return the amount of pages requested
        // In fact, it can return as little as 1 page, and callers are supposed
        // to deal with this scenario. So re-attempt the allocation with less
        // pages than before, and see if it worked this time.
        //
        PageCount -= (PageCount >> 4);
    } while (PageCount);

    //
    // Wow, not even a single page was around!
    //
    if (!Mdl) return NULL;

    //
    // This is where the page array starts....
    //
    MdlPage = (PPFN_NUMBER)(Mdl + 1);

    //
    // Lock the PFN database
    //
    OldIrql = MiAcquirePfnLock();

    //
    // Are we looking for any pages, without discriminating?
    //
    if ((LowPage == 0) && (HighPage == MmHighestPhysicalPage))
    {
        //
        // Well then, let's go shopping
        //
        while (PagesFound < PageCount)
        {
            /* Grab a page */
            MI_SET_USAGE(MI_USAGE_MDL);
            MI_SET_PROCESS2("Kernel");

            /* FIXME: This check should be smarter */
            Page = 0;
            if (MmAvailablePages != 0)
                Page = MiRemoveAnyPage(0);

            if (Page == 0)
            {
                /* This is not good... hopefully we have at least SOME pages */
                ASSERT(PagesFound);
                break;
            }

            /* Grab the page entry for it */
            Pfn1 = MiGetPfnEntry(Page);

            //
            // Make sure it's really free
            //
            ASSERT(Pfn1->u3.e2.ReferenceCount == 0);

            /* Now setup the page and mark it */
            Pfn1->u3.e2.ReferenceCount = 1;
            Pfn1->u2.ShareCount = 1;
            MI_SET_PFN_DELETED(Pfn1);
            Pfn1->u4.PteFrame = 0x1FFEDCB;
            Pfn1->u3.e1.StartOfAllocation = 1;
            Pfn1->u3.e1.EndOfAllocation = 1;
            Pfn1->u4.VerifierAllocation = 0;

            //
            // Save it into the MDL
            //
            *MdlPage++ = MiGetPfnEntryIndex(Pfn1);
            PagesFound++;
        }
    }
    else
    {
        //
        // You want specific range of pages. We'll do this in two runs
        //
        for (LookForZeroedPages = 1; LookForZeroedPages >= 0; LookForZeroedPages--)
        {
            //
            // Scan the range you specified
            //
            for (Page = LowPage; Page < HighPage; Page++)
            {
                //
                // Get the PFN entry for this page
                //
                Pfn1 = MiGetPfnEntry(Page);
                ASSERT(Pfn1);

                //
                // Make sure it's free and if this is our first pass, zeroed
                //
                if (MiIsPfnInUse(Pfn1)) continue;
                if ((Pfn1->u3.e1.PageLocation == ZeroedPageList) != LookForZeroedPages) continue;

                /* Remove the page from the free or zero list */
                ASSERT(Pfn1->u3.e1.ReadInProgress == 0);
                MI_SET_USAGE(MI_USAGE_MDL);
                MI_SET_PROCESS2("Kernel");
                MiUnlinkFreeOrZeroedPage(Pfn1);

                //
                // Sanity checks
                //
                ASSERT(Pfn1->u3.e2.ReferenceCount == 0);

                //
                // Now setup the page and mark it
                //
                Pfn1->u3.e2.ReferenceCount = 1;
                Pfn1->u2.ShareCount = 1;
                MI_SET_PFN_DELETED(Pfn1);
                Pfn1->u4.PteFrame = 0x1FFEDCB;
                Pfn1->u3.e1.StartOfAllocation = 1;
                Pfn1->u3.e1.EndOfAllocation = 1;
                Pfn1->u4.VerifierAllocation = 0;

                //
                // Save this page into the MDL
                //
                *MdlPage++ = Page;
                if (++PagesFound == PageCount) break;
            }

            //
            // If the first pass was enough, don't keep going, otherwise, go again
            //
            if (PagesFound == PageCount) break;
        }
    }

    //
    // Now release the PFN count
    //
    MiReleasePfnLock(OldIrql);

    //
    // We might've found less pages, but not more ;-)
    //
    if (PagesFound != PageCount) ASSERT(PagesFound < PageCount);
    if (!PagesFound)
    {
        //
        // If we didn' tfind any pages at all, fail
        //
        DPRINT1("NO MDL PAGES!\n");
        ExFreePoolWithTag(Mdl, TAG_MDL);
        return NULL;
    }

    //
    // Write out how many pages we found
    //
    Mdl->ByteCount = (ULONG)(PagesFound << PAGE_SHIFT);

    //
    // Terminate the MDL array if there's certain missing pages
    //
    if (PagesFound != PageCount) *MdlPage = LIST_HEAD;

    //
    // Now go back and loop over all the MDL pages
    //
    MdlPage = (PPFN_NUMBER)(Mdl + 1);
    LastMdlPage = MdlPage + PagesFound;
    while (MdlPage < LastMdlPage)
    {
        //
        // Check if we've reached the end
        //
        Page = *MdlPage++;
        if (Page == LIST_HEAD) break;

        //
        // Get the PFN entry for the page and check if we should zero it out
        //
        Pfn1 = MiGetPfnEntry(Page);
        ASSERT(Pfn1);
        if (Pfn1->u3.e1.PageLocation != ZeroedPageList) MiZeroPhysicalPage(Page);
        Pfn1->u3.e1.PageLocation = ActiveAndValid;
    }

    //
    // We're done, mark the pages as locked
    //
    Mdl->Process = NULL;
    Mdl->MdlFlags |= MDL_PAGES_LOCKED;
    return Mdl;
}

VOID
NTAPI
MmSetRmapListHeadPage(PFN_NUMBER Pfn, PMM_RMAP_ENTRY ListHead)
{
    PMMPFN Pfn1;

    /* PFN database must be locked */
    MI_ASSERT_PFN_LOCK_HELD();

    Pfn1 = MiGetPfnEntry(Pfn);
    ASSERT(Pfn1);
    ASSERT_IS_ROS_PFN(Pfn1);

    if (ListHead)
    {
        /* Should not be trying to insert an RMAP for a non-active page */
        ASSERT(MiIsPfnInUse(Pfn1) == TRUE);

        /* Set the list head address */
        Pfn1->RmapListHead = ListHead;
    }
    else
    {
        /* ReactOS semantics dictate the page is STILL active right now */
        ASSERT(MiIsPfnInUse(Pfn1) == TRUE);

        /* In this case, the RMAP is actually being removed, so clear field */
        Pfn1->RmapListHead = NULL;

        /* ReactOS semantics will now release the page, which will make it free and enter a colored list */
    }
}

PMM_RMAP_ENTRY
NTAPI
MmGetRmapListHeadPage(PFN_NUMBER Pfn)
{
    PMMPFN Pfn1;

    /* PFN database must be locked */
    MI_ASSERT_PFN_LOCK_HELD();

    /* Get the entry */
    Pfn1 = MiGetPfnEntry(Pfn);
    ASSERT(Pfn1);

    if (!MI_IS_ROS_PFN(Pfn1))
    {
        return NULL;
    }

    /* Should not have an RMAP for a non-active page */
    ASSERT(MiIsPfnInUse(Pfn1) == TRUE);

    /* Get the list head */
    return Pfn1->RmapListHead;
}

VOID
NTAPI
MmSetSavedSwapEntryPage(PFN_NUMBER Pfn,  SWAPENTRY SwapEntry)
{
    KIRQL oldIrql;
    PMMPFN Pfn1;

    Pfn1 = MiGetPfnEntry(Pfn);
    ASSERT(Pfn1);
    ASSERT_IS_ROS_PFN(Pfn1);

    oldIrql = MiAcquirePfnLock();
    Pfn1->u1.SwapEntry = SwapEntry;
    MiReleasePfnLock(oldIrql);
}

SWAPENTRY
NTAPI
MmGetSavedSwapEntryPage(PFN_NUMBER Pfn)
{
    SWAPENTRY SwapEntry;
    KIRQL oldIrql;
    PMMPFN Pfn1;

    Pfn1 = MiGetPfnEntry(Pfn);
    ASSERT(Pfn1);
    ASSERT_IS_ROS_PFN(Pfn1);

    oldIrql = MiAcquirePfnLock();
    SwapEntry = Pfn1->u1.SwapEntry;
    MiReleasePfnLock(oldIrql);

    return(SwapEntry);
}

VOID
NTAPI
MmReferencePage(PFN_NUMBER Pfn)
{
    PMMPFN Pfn1;

    DPRINT("MmReferencePage(PysicalAddress %x)\n", Pfn << PAGE_SHIFT);

    MI_ASSERT_PFN_LOCK_HELD();
    ASSERT(Pfn != 0);
    ASSERT(Pfn <= MmHighestPhysicalPage);

    Pfn1 = MiGetPfnEntry(Pfn);
    ASSERT(Pfn1);
    ASSERT_IS_ROS_PFN(Pfn1);

    ASSERT(Pfn1->u3.e2.ReferenceCount != 0);
    Pfn1->u3.e2.ReferenceCount++;
}

ULONG
NTAPI
MmGetReferenceCountPage(PFN_NUMBER Pfn)
{
    ULONG RCount;
    PMMPFN Pfn1;

    MI_ASSERT_PFN_LOCK_HELD();

    DPRINT("MmGetReferenceCountPage(PhysicalAddress %x)\n", Pfn << PAGE_SHIFT);

    Pfn1 = MiGetPfnEntry(Pfn);
    ASSERT(Pfn1);
    ASSERT_IS_ROS_PFN(Pfn1);

    RCount = Pfn1->u3.e2.ReferenceCount;

    return(RCount);
}

BOOLEAN
NTAPI
MmIsPageInUse(PFN_NUMBER Pfn)
{
    return MiIsPfnInUse(MiGetPfnEntry(Pfn));
}

VOID
NTAPI
MmDereferencePage(PFN_NUMBER Pfn)
{
    PMMPFN Pfn1;
    DPRINT("MmDereferencePage(PhysicalAddress %x)\n", Pfn << PAGE_SHIFT);

    MI_ASSERT_PFN_LOCK_HELD();

    Pfn1 = MiGetPfnEntry(Pfn);
    ASSERT(Pfn1);
    ASSERT_IS_ROS_PFN(Pfn1);

    ASSERT(Pfn1->u3.e2.ReferenceCount != 0);
    Pfn1->u3.e2.ReferenceCount--;
    if (Pfn1->u3.e2.ReferenceCount == 0)
    {
        /* Apply LRU hack */
        if (Pfn1->u4.MustBeCached)
        {
            MmRemoveLRUUserPage(Pfn);
            Pfn1->u4.MustBeCached = 0;
        }

        /* Mark the page temporarily as valid, we're going to make it free soon */
        Pfn1->u3.e1.PageLocation = ActiveAndValid;

        /* It's not a ROS PFN anymore */
        Pfn1->u4.AweAllocation = FALSE;

        /* Bring it back into the free list */
        DPRINT("Legacy free: %lx\n", Pfn);
        MiInsertPageInFreeList(Pfn);
    }
}

PFN_NUMBER
NTAPI
MmAllocPage(ULONG Type)
{
    PFN_NUMBER PfnOffset;
    PMMPFN Pfn1;
    KIRQL OldIrql;

    OldIrql = MiAcquirePfnLock();

#if MI_TRACE_PFNS
    switch(Type)
    {
    case MC_SYSTEM:
        MI_SET_USAGE(MI_USAGE_CACHE);
        break;
    case MC_USER:
        MI_SET_USAGE(MI_USAGE_SECTION);
        break;
    default:
        ASSERT(FALSE);
    }
#endif

    PfnOffset = MiRemoveZeroPage(MI_GET_NEXT_COLOR());
    if (!PfnOffset)
    {
        MiReleasePfnLock(OldIrql);
        return 0;
    }

    DPRINT("Legacy allocate: %lx\n", PfnOffset);
    Pfn1 = MiGetPfnEntry(PfnOffset);
    Pfn1->u3.e2.ReferenceCount = 1;
    Pfn1->u3.e1.PageLocation = ActiveAndValid;

    /* This marks the PFN as a ReactOS PFN */
    Pfn1->u4.AweAllocation = TRUE;

    /* Allocate the extra ReactOS Data and zero it out */
    Pfn1->u1.SwapEntry = 0;
    Pfn1->RmapListHead = NULL;

    Pfn1->NextLRU = NULL;
    Pfn1->PreviousLRU = NULL;

    if (Type == MC_USER)
    {
        Pfn1->u4.MustBeCached = 1; /* HACK again */
        MmInsertLRULastUserPage(PfnOffset);
    }

    MiReleasePfnLock(OldIrql);
    return PfnOffset;
}

/* EOF */
