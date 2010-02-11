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

#if defined (ALLOC_PRAGMA)
#pragma alloc_text(INIT, MmInitializePageList)
#endif

#define MODULE_INVOLVED_IN_ARM3
#include "ARM3/miarm.h"

/* TYPES *******************************************************************/

#define MM_PHYSICAL_PAGE_FREE    (0x1)
#define MM_PHYSICAL_PAGE_USED    (0x2)

/* GLOBALS ****************************************************************/

//
//
// ReactOS to NT Physical Page Descriptor Entry Legacy Mapping Definitions
//
//        REACTOS                 NT
//
#define Consumer             PageLocation
#define Type                 CacheAttribute
#define Zero                 PrototypePte
#define LockCount            u3.e1.PageColor
#define RmapListHead         AweReferenceCount
#define SavedSwapEntry       u4.EntireFrame
#define Flags                u3.e1
#define ReferenceCount       u3.ReferenceCount
#define RemoveEntryList(x)   RemoveEntryList((PLIST_ENTRY)x)
#define InsertTailList(x, y) InsertTailList(x, (PLIST_ENTRY)y)
#define ListEntry            u1
#define PHYSICAL_PAGE        MMPFN
#define PPHYSICAL_PAGE       PMMPFN

/* The first array contains ReactOS PFNs, the second contains ARM3 PFNs */
PPHYSICAL_PAGE MmPfnDatabase[2];

ULONG MmAvailablePages;
ULONG MmResidentAvailablePages;

SIZE_T MmTotalCommitLimit;
SIZE_T MmTotalCommittedPages;
SIZE_T MmSharedCommit;
SIZE_T MmDriverCommit;
SIZE_T MmProcessCommit;
SIZE_T MmPagedPoolCommit;
SIZE_T MmPeakCommitment; 
SIZE_T MmtotalCommitLimitMaximum;

MMPFNLIST MmZeroedPageListHead;
MMPFNLIST MmFreePageListHead;
MMPFNLIST MmStandbyPageListHead;
MMPFNLIST MmModifiedPageListHead;
MMPFNLIST MmModifiedNoWritePageListHead;

/* List of pages allocated to the MC_USER Consumer */
static LIST_ENTRY UserPageListHead;
/* List of pages zeroed by the ZPW (MmZeroPageThreadMain) */
static LIST_ENTRY FreeZeroedPageListHead;
/* List of free pages, filled by MmGetReferenceCountPage and
 * and MmInitializePageList */
static LIST_ENTRY FreeUnzeroedPageListHead;

static KEVENT ZeroPageThreadEvent;
static BOOLEAN ZeroPageThreadShouldTerminate = FALSE;

static ULONG UnzeroedPageCount = 0;

/* FUNCTIONS *************************************************************/

PFN_TYPE
NTAPI
MmGetLRUFirstUserPage(VOID)
{
   PLIST_ENTRY NextListEntry;
   PHYSICAL_PAGE* PageDescriptor;
   KIRQL oldIrql;

   oldIrql = KeAcquireQueuedSpinLock(LockQueuePfnLock);
   NextListEntry = UserPageListHead.Flink;
   if (NextListEntry == &UserPageListHead)
   {
	  KeReleaseQueuedSpinLock(LockQueuePfnLock, oldIrql);
      return 0;
   }
   PageDescriptor = CONTAINING_RECORD(NextListEntry, PHYSICAL_PAGE, ListEntry);
   ASSERT_PFN(PageDescriptor);
   KeReleaseQueuedSpinLock(LockQueuePfnLock, oldIrql);
   return PageDescriptor - MmPfnDatabase[0];
}

VOID
NTAPI
MmInsertLRULastUserPage(PFN_TYPE Pfn)
{
   KIRQL oldIrql;
   PPHYSICAL_PAGE Page;

   oldIrql = KeAcquireQueuedSpinLock(LockQueuePfnLock);
   Page = MiGetPfnEntry(Pfn);
   ASSERT(Page);
   ASSERT(Page->Flags.Type == MM_PHYSICAL_PAGE_USED);
   ASSERT(Page->Flags.Consumer == MC_USER);
   InsertTailList(&UserPageListHead, &Page->ListEntry);
   KeReleaseQueuedSpinLock(LockQueuePfnLock, oldIrql);
}

PFN_TYPE
NTAPI
MmGetLRUNextUserPage(PFN_TYPE PreviousPfn)
{
   PLIST_ENTRY NextListEntry;
   PHYSICAL_PAGE* PageDescriptor;
   KIRQL oldIrql;
   PPHYSICAL_PAGE Page;

   oldIrql = KeAcquireQueuedSpinLock(LockQueuePfnLock);
   Page = MiGetPfnEntry(PreviousPfn);
   ASSERT(Page);
   ASSERT(Page->Flags.Type == MM_PHYSICAL_PAGE_USED);
   ASSERT(Page->Flags.Consumer == MC_USER);
   NextListEntry = (PLIST_ENTRY)Page->ListEntry.Flink;
   if (NextListEntry == &UserPageListHead)
   {
	  KeReleaseQueuedSpinLock(LockQueuePfnLock, oldIrql);
      return 0;
   }
   PageDescriptor = CONTAINING_RECORD(NextListEntry, PHYSICAL_PAGE, ListEntry);
   KeReleaseQueuedSpinLock(LockQueuePfnLock, oldIrql);
   return PageDescriptor - MmPfnDatabase[0];
}

VOID
NTAPI
MmRemoveLRUUserPage(PFN_TYPE Page)
{
   RemoveEntryList(&MiGetPfnEntry(Page)->ListEntry);
}

PFN_NUMBER
NTAPI
MiFindContiguousPages(IN PFN_NUMBER LowestPfn,
                      IN PFN_NUMBER HighestPfn,
                      IN PFN_NUMBER BoundaryPfn,
                      IN PFN_NUMBER SizeInPages,
                      IN MEMORY_CACHING_TYPE CacheType)
{
    PFN_NUMBER Page, PageCount, LastPage, Length, BoundaryMask;
    ULONG i = 0;
    PMMPFN Pfn1, EndPfn;
    KIRQL OldIrql;
    PAGED_CODE ();
    ASSERT(SizeInPages != 0);
        
    //
    // Convert the boundary PFN into an alignment mask
    //
    BoundaryMask = ~(BoundaryPfn - 1);
    
    //
    // Loop all the physical memory blocks
    //
    do
    {
        //
        // Capture the base page and length of this memory block
        //
        Page = MmPhysicalMemoryBlock->Run[i].BasePage;
        PageCount = MmPhysicalMemoryBlock->Run[i].PageCount;
        
        //
        // Check how far this memory block will go
        //
        LastPage = Page + PageCount;
        
        //
        // Trim it down to only the PFNs we're actually interested in
        //
        if ((LastPage - 1) > HighestPfn) LastPage = HighestPfn + 1;
        if (Page < LowestPfn) Page = LowestPfn;
        
        //
        // Skip this run if it's empty or fails to contain all the pages we need
        //
        if (!(PageCount) || ((Page + SizeInPages) > LastPage)) continue;
        
        //
        // Now scan all the relevant PFNs in this run
        //
        Length = 0;
        for (Pfn1 = MiGetPfnEntry(Page); Page < LastPage; Page++, Pfn1++)
        {
            //
            // If this PFN is in use, ignore it
            //
            if (Pfn1->Flags.Type != MM_PHYSICAL_PAGE_FREE) continue;
            
            //
            // If we haven't chosen a start PFN yet and the caller specified an
            // alignment, make sure the page matches the alignment restriction
            //
            if ((!(Length) && (BoundaryPfn)) &&
                (((Page ^ (Page + SizeInPages - 1)) & BoundaryMask)))
            {
                //
                // It does not, so bail out
                //
                continue;
            }
            
            //
            // Increase the number of valid pages, and check if we have enough
            //
            if (++Length == SizeInPages)
            {
                //
                // It appears we've amassed enough legitimate pages, rollback
                //
                Pfn1 -= (Length - 1);
                Page -= (Length - 1);
                
                //
                // Acquire the PFN lock
                //
                OldIrql = KeAcquireQueuedSpinLock(LockQueuePfnLock);
                do
                {
                    //
                    // Things might've changed for us. Is the page still free?
                    //
                    if (Pfn1->Flags.Type != MM_PHYSICAL_PAGE_FREE) break;
                    
                    //
                    // So far so good. Is this the last confirmed valid page?
                    //
                    if (!--Length)
                    {
                        //
                        // Sanity check that we didn't go out of bounds
                        //
                        ASSERT(i != MmPhysicalMemoryBlock->NumberOfRuns);
                        
                        //
                        // Loop until all PFN entries have been processed
                        //
                        EndPfn = Pfn1 - SizeInPages + 1;
                        do
                        {
                            //
                            // If this was an unzeroed page, there are now less
                            //
                            if (Pfn1->Flags.Zero == 0) UnzeroedPageCount--;
                            
                            //
                            // One less free page
                            //
                            MmAvailablePages--;
                            
                            //
                            // This PFN is now a used page, set it up
                            //
                            RemoveEntryList(&Pfn1->ListEntry);
                            Pfn1->Flags.Type = MM_PHYSICAL_PAGE_USED;
                            Pfn1->Flags.Consumer = MC_NPPOOL;
                            Pfn1->ReferenceCount = 1;
                            Pfn1->LockCount = 0;
                            Pfn1->SavedSwapEntry = 0;
                            
                            //
                            // Check if it was already zeroed
                            //
                            if (Pfn1->Flags.Zero == 0)
                            {
                                //
                                // It wasn't, so zero it
                                //
                                MiZeroPage(MiGetPfnEntryIndex(Pfn1));
                            }

                            //
                            // Check if this is the last PFN, otherwise go on
                            //
                            if (Pfn1 == EndPfn) break;
                            Pfn1--;
                        } while (TRUE);
                        
                        //
                        // Mark the first and last PFN so we can find them later
                        //
                        Pfn1->Flags.StartOfAllocation = 1;
                        (Pfn1 + SizeInPages - 1)->Flags.EndOfAllocation = 1;
                        
                        //
                        // Now it's safe to let go of the PFN lock
                        //
                        KeReleaseQueuedSpinLock(LockQueuePfnLock, OldIrql);
                        
                        //
                        // Quick sanity check that the last PFN is consistent
                        //
                        EndPfn = Pfn1 + SizeInPages;
                        ASSERT(EndPfn == MiGetPfnEntry(Page + 1));
                        
                        //
                        // Compute the first page, and make sure it's consistent
                        //
                        Page -= SizeInPages - 1;
                        ASSERT(Pfn1 == MiGetPfnEntry(Page));
                        ASSERT(Page != 0);
                        return Page;                                
                    }
                    
                    //
                    // Keep going. The purpose of this loop is to reconfirm that
                    // after acquiring the PFN lock these pages are still usable
                    //
                    Pfn1++;
                    Page++;
                } while (TRUE);
                
                //
                // If we got here, something changed while we hadn't acquired
                // the PFN lock yet, so we'll have to restart
                //
                KeReleaseQueuedSpinLock(LockQueuePfnLock, OldIrql);
                Length = 0;
            }
        }
    } while (++i != MmPhysicalMemoryBlock->NumberOfRuns);
    
    //
    // And if we get here, it means no suitable physical memory runs were found
    //
    return 0;    
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
    PLIST_ENTRY ListEntry;
    PPHYSICAL_PAGE Pfn1;
    INT LookForZeroedPages;
    ASSERT (KeGetCurrentIrql() <= APC_LEVEL);
    
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
    OldIrql = KeAcquireQueuedSpinLock(LockQueuePfnLock);
    
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
            //
            // Do we have zeroed pages?
            //
            if (!IsListEmpty(&FreeZeroedPageListHead))
            {
                //
                // Grab a zero page
                //
                ListEntry = RemoveTailList(&FreeZeroedPageListHead);
            }
            else if (!IsListEmpty(&FreeUnzeroedPageListHead))
            {
                //
                // Nope, grab an unzeroed page
                //
                ListEntry = RemoveTailList(&FreeUnzeroedPageListHead);
                UnzeroedPageCount--;
            }
            else
            {
                //
                // This is not good... hopefully we have at least SOME pages
                //
                ASSERT(PagesFound);
                break;
            }
            
            //
            // Get the PFN entry for this page
            //
            Pfn1 = CONTAINING_RECORD(ListEntry, PHYSICAL_PAGE, ListEntry);
            
            //
            // Make sure it's really free
            //
            ASSERT(Pfn1->Flags.Type == MM_PHYSICAL_PAGE_FREE);
            ASSERT(Pfn1->ReferenceCount == 0);
            
            //
            // Allocate it and mark it
            //
            Pfn1->Flags.Type = MM_PHYSICAL_PAGE_USED;
            Pfn1->Flags.Consumer = MC_NPPOOL;
            Pfn1->Flags.StartOfAllocation = 1;
            Pfn1->Flags.EndOfAllocation = 1;
            Pfn1->ReferenceCount = 1;
            Pfn1->LockCount = 0;
            Pfn1->SavedSwapEntry = 0;
            
            //
            // Decrease available pages
            //
            MmAvailablePages--;
            
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
                if (Pfn1->Flags.Type != MM_PHYSICAL_PAGE_FREE) continue;
                if (Pfn1->Flags.Zero != LookForZeroedPages) continue;
                
                //
                // Sanity checks
                //
                ASSERT(Pfn1->ReferenceCount == 0);
                
                //
                // Now setup the page and mark it
                //
                Pfn1->Flags.Type = MM_PHYSICAL_PAGE_USED;
                Pfn1->Flags.Consumer = MC_NPPOOL;
                Pfn1->ReferenceCount = 1;
                Pfn1->Flags.StartOfAllocation = 1;
                Pfn1->Flags.EndOfAllocation = 1;
                Pfn1->LockCount = 0;
                Pfn1->SavedSwapEntry = 0;
                
                //
                // If this page was unzeroed, we've consumed such a page
                //
                if (!Pfn1->Flags.Zero) UnzeroedPageCount--;
                
                //
                // Decrease available pages
                //
                MmAvailablePages--;
                
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
    KeReleaseQueuedSpinLock(LockQueuePfnLock, OldIrql);
    
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
        ExFreePool(Mdl);
        return NULL;
    }
    
    //
    // Write out how many pages we found
    //
    Mdl->ByteCount = (ULONG)(PagesFound << PAGE_SHIFT);
    
    //
    // Terminate the MDL array if there's certain missing pages
    //
    if (PagesFound != PageCount) *MdlPage = -1;
    
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
        if (Page == (PFN_NUMBER)-1) break;
        
        //
        // Get the PFN entry for the page and check if we should zero it out
        //
        Pfn1 = MiGetPfnEntry(Page);
        ASSERT(Pfn1);
        if (Pfn1->Flags.Zero == 0) MiZeroPage(Page);
    }
    
    //
    // We're done, mark the pages as locked (should we lock them, though???)
    //
    Mdl->Process = NULL;
    Mdl->MdlFlags |= MDL_PAGES_LOCKED; 
    return Mdl;
}

VOID
NTAPI
MmDumpPfnDatabase(VOID)
{
    ULONG i;
    PPHYSICAL_PAGE Pfn1;
    PCHAR State = "????", Consumer = "Unknown";
    KIRQL OldIrql;
    ULONG Totals[5] = {0}, FreePages = 0;
    
    KeRaiseIrql(HIGH_LEVEL, &OldIrql);
    
    //
    // Loop the PFN database
    //
    for (i = 0; i <= MmHighestPhysicalPage; i++)
    {
        Pfn1 = MiGetPfnEntry(i);
        if (!Pfn1) continue;
        
        //
        // Get the consumer
        //
        switch (Pfn1->Flags.Consumer)
        {
            case MC_NPPOOL:
                
                Consumer = "Nonpaged Pool";
                break;
                
            case MC_PPOOL:
                
                Consumer = "Paged Pool";
                break;
                
            case MC_CACHE:
                
                Consumer = "File System Cache";
                break;
                
            case MC_USER:
                
                Consumer = "Process Working Set";
                break;
                
            case MC_SYSTEM:
                
                Consumer = "System";
                break;
        }
        
        //
        // Get the type
        //
        switch (Pfn1->Flags.Type)
        {
            case MM_PHYSICAL_PAGE_USED:
                
                State = "Used";
                Totals[Pfn1->Flags.Consumer]++;
                break;
                
            case MM_PHYSICAL_PAGE_FREE:
                
                State = "Free";
                Consumer = "Free";
                FreePages++;
                break;
        }

        //
        // Pretty-print the page
        //
        DbgPrint("0x%08p:\t%04s\t%20s\t(%02d.%02d) [%08p])\n",
                 i << PAGE_SHIFT,
                 State,
                 Consumer,
                 Pfn1->ReferenceCount,
                 Pfn1->LockCount,
                 Pfn1->RmapListHead);
    }
    
    DbgPrint("Nonpaged Pool:       %d pages\t[%d KB]\n", Totals[MC_NPPOOL], (Totals[MC_NPPOOL] << PAGE_SHIFT) / 1024);
    DbgPrint("Paged Pool:          %d pages\t[%d KB]\n", Totals[MC_PPOOL],  (Totals[MC_PPOOL] << PAGE_SHIFT) / 1024);
    DbgPrint("File System Cache:   %d pages\t[%d KB]\n", Totals[MC_CACHE],  (Totals[MC_CACHE]  << PAGE_SHIFT) / 1024);
    DbgPrint("Process Working Set: %d pages\t[%d KB]\n", Totals[MC_USER],   (Totals[MC_USER]   << PAGE_SHIFT) / 1024);
    DbgPrint("System:              %d pages\t[%d KB]\n", Totals[MC_SYSTEM], (Totals[MC_SYSTEM] << PAGE_SHIFT) / 1024);
    DbgPrint("Free:                %d pages\t[%d KB]\n", FreePages,         (FreePages         << PAGE_SHIFT) / 1024);
    
    KeLowerIrql(OldIrql);
}

VOID
NTAPI
MmInitializePageList(VOID)
{
    ULONG i;
    PHYSICAL_PAGE UsedPage;
    PMEMORY_ALLOCATION_DESCRIPTOR Md;
    PLIST_ENTRY NextEntry;
    ULONG NrSystemPages = 0;

    /* Initialize the page lists */
    InitializeListHead(&UserPageListHead);
    InitializeListHead(&FreeUnzeroedPageListHead);
    InitializeListHead(&FreeZeroedPageListHead);

    /* This is what a used page looks like */
    RtlZeroMemory(&UsedPage, sizeof(UsedPage));
    UsedPage.Flags.Type = MM_PHYSICAL_PAGE_USED;
    UsedPage.Flags.Consumer = MC_NPPOOL;
    UsedPage.ReferenceCount = 1;

    /* Loop the memory descriptors */
    for (NextEntry = KeLoaderBlock->MemoryDescriptorListHead.Flink;
         NextEntry != &KeLoaderBlock->MemoryDescriptorListHead;
         NextEntry = NextEntry->Flink)
    {
#undef ListEntry
        /* Get the descriptor */
        Md = CONTAINING_RECORD(NextEntry,
                               MEMORY_ALLOCATION_DESCRIPTOR,
                               ListEntry);
#define ListEntry            u1        

        /* Skip bad memory */
        if ((Md->MemoryType == LoaderFirmwarePermanent) ||
            (Md->MemoryType == LoaderBBTMemory) ||
            (Md->MemoryType == LoaderSpecialMemory) ||
            (Md->MemoryType == LoaderBad))
        {
            //
            // We do not build PFN entries for this
            //
            continue;
        }
        else if ((Md->MemoryType == LoaderFree) ||
                 (Md->MemoryType == LoaderLoadedProgram) ||
                 (Md->MemoryType == LoaderFirmwareTemporary) ||
                 (Md->MemoryType == LoaderOsloaderStack))
        {
            /* Loop every page part of the block */
            for (i = 0; i < Md->PageCount; i++)
            {
                /* Mark it as a free page */
                MmPfnDatabase[0][Md->BasePage + i].Flags.Type = MM_PHYSICAL_PAGE_FREE;
                InsertTailList(&FreeUnzeroedPageListHead,
                               &MmPfnDatabase[0][Md->BasePage + i].ListEntry);
                UnzeroedPageCount++;
                MmAvailablePages++;
            }
        }
        else
        {
            /* Loop every page part of the block */
            for (i = 0; i < Md->PageCount; i++)
            {
                /* Everything else is used memory */
                MmPfnDatabase[0][Md->BasePage + i] = UsedPage;
                NrSystemPages++;
            }
        }
    }
    
    /* Finally handle the pages describing the PFN database themselves */
    for (i = MxOldFreeDescriptor.BasePage; i < MxFreeDescriptor->BasePage; i++)
    {
        /* Ensure this page was not added previously */
        ASSERT(MmPfnDatabase[0][i].Flags.Type == 0);

        /* Mark it as used kernel memory */
        MmPfnDatabase[0][i] = UsedPage;
        NrSystemPages++;
    }
    
    KeInitializeEvent(&ZeroPageThreadEvent, NotificationEvent, TRUE);
    DPRINT("Pages: %x %x\n", MmAvailablePages, NrSystemPages);
    MmInitializeBalancer(MmAvailablePages, NrSystemPages);
}

VOID
NTAPI
MmSetRmapListHeadPage(PFN_TYPE Pfn, struct _MM_RMAP_ENTRY* ListHead)
{
   KIRQL oldIrql;
    
   oldIrql = KeAcquireQueuedSpinLock(LockQueuePfnLock);
   MiGetPfnEntry(Pfn)->RmapListHead = (LONG)ListHead;
   KeReleaseQueuedSpinLock(LockQueuePfnLock, oldIrql);
}

struct _MM_RMAP_ENTRY*
NTAPI
MmGetRmapListHeadPage(PFN_TYPE Pfn)
{
   KIRQL oldIrql;
   struct _MM_RMAP_ENTRY* ListHead;
    
   oldIrql = KeAcquireQueuedSpinLock(LockQueuePfnLock);
   ListHead = (struct _MM_RMAP_ENTRY*)MiGetPfnEntry(Pfn)->RmapListHead;
   KeReleaseQueuedSpinLock(LockQueuePfnLock, oldIrql);
    
   return(ListHead);
}

VOID
NTAPI
MmSetSavedSwapEntryPage(PFN_TYPE Pfn,  SWAPENTRY SwapEntry)
{
   KIRQL oldIrql;

   oldIrql = KeAcquireQueuedSpinLock(LockQueuePfnLock);
   MiGetPfnEntry(Pfn)->SavedSwapEntry = SwapEntry;
   KeReleaseQueuedSpinLock(LockQueuePfnLock, oldIrql);
}

SWAPENTRY
NTAPI
MmGetSavedSwapEntryPage(PFN_TYPE Pfn)
{
   SWAPENTRY SwapEntry;
   KIRQL oldIrql;

   oldIrql = KeAcquireQueuedSpinLock(LockQueuePfnLock);
   SwapEntry = MiGetPfnEntry(Pfn)->SavedSwapEntry;
   KeReleaseQueuedSpinLock(LockQueuePfnLock, oldIrql);

   return(SwapEntry);
}

VOID
NTAPI
MmReferencePage(PFN_TYPE Pfn)
{
   PPHYSICAL_PAGE Page;

   DPRINT("MmReferencePage(PysicalAddress %x)\n", Pfn << PAGE_SHIFT);

   if (Pfn == 0 || Pfn > MmHighestPhysicalPage)
   {
      return;
   }

   Page = MiGetPfnEntry(Pfn);
   ASSERT(Page);
   if (Page->Flags.Type != MM_PHYSICAL_PAGE_USED)
   {
      DPRINT1("Referencing non-used page\n");
      KeBugCheck(MEMORY_MANAGEMENT);
   }

   Page->ReferenceCount++;
}

ULONG
NTAPI
MmGetReferenceCountPage(PFN_TYPE Pfn)
{
   KIRQL oldIrql;
   ULONG RCount;
   PPHYSICAL_PAGE Page;

   DPRINT("MmGetReferenceCountPage(PhysicalAddress %x)\n", Pfn << PAGE_SHIFT);

   oldIrql = KeAcquireQueuedSpinLock(LockQueuePfnLock);
   Page = MiGetPfnEntry(Pfn);
   ASSERT(Page);
   if (Page->Flags.Type != MM_PHYSICAL_PAGE_USED)
   {
      DPRINT1("Getting reference count for free page\n");
      KeBugCheck(MEMORY_MANAGEMENT);
   }

   RCount = Page->ReferenceCount;

   KeReleaseQueuedSpinLock(LockQueuePfnLock, oldIrql);
   return(RCount);
}

BOOLEAN
NTAPI
MmIsPageInUse(PFN_TYPE Pfn)
{

   DPRINT("MmIsPageInUse(PhysicalAddress %x)\n", Pfn << PAGE_SHIFT);

   return (MiGetPfnEntry(Pfn)->Flags.Type == MM_PHYSICAL_PAGE_USED);
}

VOID
NTAPI
MmDereferencePage(PFN_TYPE Pfn)
{
   PPHYSICAL_PAGE Page;

   DPRINT("MmDereferencePage(PhysicalAddress %x)\n", Pfn << PAGE_SHIFT);

   Page = MiGetPfnEntry(Pfn);
   ASSERT(Page);

   if (Page->Flags.Type != MM_PHYSICAL_PAGE_USED)
   {
      DPRINT1("Dereferencing free page\n");
      KeBugCheck(MEMORY_MANAGEMENT);
   }
   if (Page->ReferenceCount == 0)
   {
      DPRINT1("Derefrencing page with reference count 0\n");
      KeBugCheck(MEMORY_MANAGEMENT);
   }

   Page->ReferenceCount--;
   if (Page->ReferenceCount == 0)
   {
      MmAvailablePages++;
      if (Page->Flags.Consumer == MC_USER) RemoveEntryList(&Page->ListEntry);
      if (Page->RmapListHead != (LONG)NULL)
      {
         DPRINT1("Freeing page with rmap entries.\n");
         KeBugCheck(MEMORY_MANAGEMENT);
      }
      if (Page->LockCount > 0)
      {
         DPRINT1("Freeing locked page\n");
         KeBugCheck(MEMORY_MANAGEMENT);
      }
      if (Page->SavedSwapEntry != 0)
      {
         DPRINT1("Freeing page with swap entry.\n");
         KeBugCheck(MEMORY_MANAGEMENT);
      }
      if (Page->Flags.Type != MM_PHYSICAL_PAGE_USED)
      {
         DPRINT1("Freeing page with flags %x\n",
                  Page->Flags.Type);
         KeBugCheck(MEMORY_MANAGEMENT);
      }
      Page->Flags.Type = MM_PHYSICAL_PAGE_FREE;
      Page->Flags.Consumer = MC_MAXIMUM;
      InsertTailList(&FreeUnzeroedPageListHead,
                     &Page->ListEntry);
      UnzeroedPageCount++;
      if (UnzeroedPageCount > 8 && 0 == KeReadStateEvent(&ZeroPageThreadEvent))
      {
         KeSetEvent(&ZeroPageThreadEvent, IO_NO_INCREMENT, FALSE);
      }
   }
}

ULONG
NTAPI
MmGetLockCountPage(PFN_TYPE Pfn)
{
   KIRQL oldIrql;
   ULONG CurrentLockCount;
   PPHYSICAL_PAGE Page;

   DPRINT("MmGetLockCountPage(PhysicalAddress %x)\n", Pfn << PAGE_SHIFT);

   oldIrql = KeAcquireQueuedSpinLock(LockQueuePfnLock);

   Page = MiGetPfnEntry(Pfn);
   ASSERT(Page);
   if (Page->Flags.Type != MM_PHYSICAL_PAGE_USED)
   {
      DPRINT1("Getting lock count for free page\n");
      KeBugCheck(MEMORY_MANAGEMENT);
   }

   CurrentLockCount = Page->LockCount;
   KeReleaseQueuedSpinLock(LockQueuePfnLock, oldIrql);

   return(CurrentLockCount);
}

VOID
NTAPI
MmLockPage(PFN_TYPE Pfn)
{
   PPHYSICAL_PAGE Page;

   DPRINT("MmLockPage(PhysicalAddress %x)\n", Pfn << PAGE_SHIFT);

   Page = MiGetPfnEntry(Pfn);
   ASSERT(Page);
   if (Page->Flags.Type != MM_PHYSICAL_PAGE_USED)
   {
      DPRINT1("Locking free page\n");
      KeBugCheck(MEMORY_MANAGEMENT);
   }

   Page->LockCount++;
}

VOID
NTAPI
MmUnlockPage(PFN_TYPE Pfn)
{
   PPHYSICAL_PAGE Page;

   DPRINT("MmUnlockPage(PhysicalAddress %x)\n", Pfn << PAGE_SHIFT);

   Page = MiGetPfnEntry(Pfn);
   ASSERT(Page);
   if (Page->Flags.Type != MM_PHYSICAL_PAGE_USED)
   {
      DPRINT1("Unlocking free page\n");
      KeBugCheck(MEMORY_MANAGEMENT);
   }

   Page->LockCount--;
}

PFN_TYPE
NTAPI
MmAllocPage(ULONG Consumer, SWAPENTRY SwapEntry)
{
   PFN_TYPE PfnOffset;
   PLIST_ENTRY ListEntry;
   PPHYSICAL_PAGE PageDescriptor;
   BOOLEAN NeedClear = FALSE;

   DPRINT("MmAllocPage()\n");

   if (IsListEmpty(&FreeZeroedPageListHead))
   {
      if (IsListEmpty(&FreeUnzeroedPageListHead))
      {
         /* Check if this allocation is for the PFN DB itself */
         if (MmNumberOfPhysicalPages == 0) 
         {
             ASSERT(FALSE);
         }

         DPRINT1("MmAllocPage(): Out of memory\n");
         return 0;
      }
      ListEntry = RemoveTailList(&FreeUnzeroedPageListHead);
      UnzeroedPageCount--;

      PageDescriptor = CONTAINING_RECORD(ListEntry, PHYSICAL_PAGE, ListEntry);

      NeedClear = TRUE;
   }
   else
   {
      ListEntry = RemoveTailList(&FreeZeroedPageListHead);

      PageDescriptor = CONTAINING_RECORD(ListEntry, PHYSICAL_PAGE, ListEntry);
   }

   if (PageDescriptor->Flags.Type != MM_PHYSICAL_PAGE_FREE)
   {
      DPRINT1("Got non-free page from freelist\n");
      KeBugCheck(MEMORY_MANAGEMENT);
   }
   if (PageDescriptor->ReferenceCount != 0)
   {
      DPRINT1("%d\n", PageDescriptor->ReferenceCount);
      KeBugCheck(MEMORY_MANAGEMENT);
   }
   PageDescriptor->Flags.Type = MM_PHYSICAL_PAGE_USED;
   PageDescriptor->Flags.Consumer = Consumer;
   PageDescriptor->ReferenceCount = 1;
   PageDescriptor->LockCount = 0;
   PageDescriptor->SavedSwapEntry = SwapEntry;

   MmAvailablePages--;

   PfnOffset = PageDescriptor - MmPfnDatabase[0];
   if ((NeedClear) && (Consumer != MC_SYSTEM))
   {
      MiZeroPage(PfnOffset);
   }
   return PfnOffset;
}

NTSTATUS
NTAPI
MiZeroPage(PFN_TYPE Page)
{
    KIRQL Irql;
    PVOID TempAddress;
    
    Irql = KeRaiseIrqlToDpcLevel();
    TempAddress = MiMapPageToZeroInHyperSpace(Page);
    if (TempAddress == NULL)
    {
        return(STATUS_NO_MEMORY);
    }
    memset(TempAddress, 0, PAGE_SIZE);
    MiUnmapPagesInZeroSpace(TempAddress, 1);
    KeLowerIrql(Irql);
    return(STATUS_SUCCESS);
}

NTSTATUS
NTAPI
MmZeroPageThreadMain(PVOID Ignored)
{
   NTSTATUS Status;
   KIRQL oldIrql;
   PLIST_ENTRY ListEntry;
   PPHYSICAL_PAGE PageDescriptor;
   PFN_TYPE Pfn;
   ULONG Count;

   /* Free initial kernel memory */
   //MiFreeInitMemory();

   /* Set our priority to 0 */
   KeGetCurrentThread()->BasePriority = 0;
   KeSetPriorityThread(KeGetCurrentThread(), 0);

   while(1)
   {
      Status = KeWaitForSingleObject(&ZeroPageThreadEvent,
                                     0,
                                     KernelMode,
                                     FALSE,
                                     NULL);
      if (!NT_SUCCESS(Status))
      {
         DPRINT1("ZeroPageThread: Wait failed\n");
         KeBugCheck(MEMORY_MANAGEMENT);
      }

      if (ZeroPageThreadShouldTerminate)
      {
         DPRINT1("ZeroPageThread: Terminating\n");
         return STATUS_SUCCESS;
      }
      Count = 0;
      oldIrql = KeAcquireQueuedSpinLock(LockQueuePfnLock);
      while (!IsListEmpty(&FreeUnzeroedPageListHead))
      {
         ListEntry = RemoveTailList(&FreeUnzeroedPageListHead);
         UnzeroedPageCount--;
         PageDescriptor = CONTAINING_RECORD(ListEntry, PHYSICAL_PAGE, ListEntry);
         /* We set the page to used, because MmCreateVirtualMapping failed with unused pages */
         PageDescriptor->Flags.Type = MM_PHYSICAL_PAGE_USED;
         KeReleaseQueuedSpinLock(LockQueuePfnLock, oldIrql);
         Pfn = PageDescriptor - MmPfnDatabase[0];
         Status = MiZeroPage(Pfn);

         oldIrql = KeAcquireQueuedSpinLock(LockQueuePfnLock);
         PageDescriptor->Flags.Zero = 1;
         PageDescriptor->Flags.Type = MM_PHYSICAL_PAGE_FREE;
         if (NT_SUCCESS(Status))
         {
            InsertHeadList(&FreeZeroedPageListHead, ListEntry);
            Count++;
         }
         else
         {
            InsertHeadList(&FreeUnzeroedPageListHead, ListEntry);
            UnzeroedPageCount++;
         }

      }
      DPRINT("Zeroed %d pages.\n", Count);
      KeResetEvent(&ZeroPageThreadEvent);
      KeReleaseQueuedSpinLock(LockQueuePfnLock, oldIrql);
   }

   return STATUS_SUCCESS;
}

/* EOF */
