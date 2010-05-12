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

/* GLOBALS ****************************************************************/

//
//
// ReactOS to NT Physical Page Descriptor Entry Legacy Mapping Definitions
//
//        REACTOS                 NT
//
#define RmapListHead         AweReferenceCount
#define PHYSICAL_PAGE        MMPFN
#define PPHYSICAL_PAGE       PMMPFN

/* The first array contains ReactOS PFNs, the second contains ARM3 PFNs */
PPHYSICAL_PAGE MmPfnDatabase[2];

PFN_NUMBER MmAvailablePages;
PFN_NUMBER MmResidentAvailablePages;
PFN_NUMBER MmResidentAvailableAtInit;

SIZE_T MmTotalCommitLimit;
SIZE_T MmTotalCommittedPages;
SIZE_T MmSharedCommit;
SIZE_T MmDriverCommit;
SIZE_T MmProcessCommit;
SIZE_T MmPagedPoolCommit;
SIZE_T MmPeakCommitment; 
SIZE_T MmtotalCommitLimitMaximum;

KEVENT ZeroPageThreadEvent;
static BOOLEAN ZeroPageThreadShouldTerminate = FALSE;
static RTL_BITMAP MiUserPfnBitMap;

/* FUNCTIONS *************************************************************/

VOID
NTAPI
MiInitializeUserPfnBitmap(VOID)
{
    PVOID Bitmap;
    
    /* Allocate enough buffer for the PFN bitmap and align it on 32-bits */
    Bitmap = ExAllocatePoolWithTag(NonPagedPool,
                                   (((MmHighestPhysicalPage + 1) + 31) / 32) * 4,
                                   '  mM');
    ASSERT(Bitmap);

    /* Initialize it and clear all the bits to begin with */
    RtlInitializeBitMap(&MiUserPfnBitMap,
                        Bitmap,
                        MmHighestPhysicalPage + 1);
    RtlClearAllBits(&MiUserPfnBitMap);
}

PFN_TYPE
NTAPI
MmGetLRUFirstUserPage(VOID)
{
    ULONG Position;
    KIRQL OldIrql;
    
    /* Find the first user page */
    OldIrql = KeAcquireQueuedSpinLock(LockQueuePfnLock);
    Position = RtlFindSetBits(&MiUserPfnBitMap, 1, 0);
    KeReleaseQueuedSpinLock(LockQueuePfnLock, OldIrql);
    if (Position == 0xFFFFFFFF) return 0;
    
    /* Return it */
    return Position;
}

VOID
NTAPI
MmInsertLRULastUserPage(PFN_TYPE Pfn)
{
    KIRQL OldIrql;

    /* Set the page as a user page */
    OldIrql = KeAcquireQueuedSpinLock(LockQueuePfnLock);
    RtlSetBit(&MiUserPfnBitMap, Pfn);
    KeReleaseQueuedSpinLock(LockQueuePfnLock, OldIrql);
}

PFN_TYPE
NTAPI
MmGetLRUNextUserPage(PFN_TYPE PreviousPfn)
{
    ULONG Position;
    KIRQL OldIrql;
    
    /* Find the next user page */
    OldIrql = KeAcquireQueuedSpinLock(LockQueuePfnLock);
    Position = RtlFindSetBits(&MiUserPfnBitMap, 1, PreviousPfn + 1);
    KeReleaseQueuedSpinLock(LockQueuePfnLock, OldIrql);
    if (Position == 0xFFFFFFFF) return 0;
    
    /* Return it */
    return Position;
}

VOID
NTAPI
MmRemoveLRUUserPage(PFN_TYPE Page)
{
    /* Unset the page as a user page */
    RtlClearBit(&MiUserPfnBitMap, Page);
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
            if (MmZeroedPageListHead.Total)
            {
                //
                // Grab a zero page
                //
                Pfn1 = MiRemoveHeadList(&MmZeroedPageListHead);
            }
            else if (MmFreePageListHead.Total)
            {
                //
                // Nope, grab an unzeroed page
                //
                Pfn1 = MiRemoveHeadList(&MmFreePageListHead);
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
            // Make sure it's really free
            //
            ASSERT(MiIsPfnInUse(Pfn1) == FALSE);
            ASSERT(Pfn1->u3.e2.ReferenceCount == 0);
            
            //
            // Allocate it and mark it
            //
            Pfn1->u3.e1.StartOfAllocation = 1;
            Pfn1->u3.e1.EndOfAllocation = 1;
            Pfn1->u3.e2.ReferenceCount = 1;
            
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
                if (MiIsPfnInUse(Pfn1)) continue;
                if ((Pfn1->u3.e1.PageLocation == ZeroedPageList) != LookForZeroedPages) continue;
                
                //
                // Sanity checks
                //
                ASSERT(Pfn1->u3.e2.ReferenceCount == 0);
                
                //
                // Now setup the page and mark it
                //
                Pfn1->u3.e2.ReferenceCount = 1;
                Pfn1->u3.e1.StartOfAllocation = 1;
                Pfn1->u3.e1.EndOfAllocation = 1;
                                
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
        if (Pfn1->u3.e1.PageLocation != ZeroedPageList) MiZeroPage(Page);
        Pfn1->u3.e1.PageLocation = ActiveAndValid;
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
    PCHAR State = "????", Type = "Unknown";
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
        // Get the type
        //
        if (MiIsPfnInUse(Pfn1))
        {
            State = "Used";
        }
        else
        {
            State = "Free";
            Type = "Free";
            FreePages++;
            break;
        }
        
        //
        // Pretty-print the page
        //
        DbgPrint("0x%08p:\t%04s\t%20s\t(%02d) [%08p])\n",
                 i << PAGE_SHIFT,
                 State,
                 Type,
                 Pfn1->u3.e2.ReferenceCount,
                 Pfn1->RmapListHead);
    }
    
    DbgPrint("Nonpaged Pool:       %d pages\t[%d KB]\n", Totals[MC_NPPOOL], (Totals[MC_NPPOOL] << PAGE_SHIFT) / 1024);
    DbgPrint("Paged Pool:          %d pages\t[%d KB]\n", Totals[MC_PPOOL],  (Totals[MC_PPOOL]  << PAGE_SHIFT) / 1024);
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

    /* This is what a used page looks like */
    RtlZeroMemory(&UsedPage, sizeof(UsedPage));
    UsedPage.u3.e1.PageLocation = ActiveAndValid;
    UsedPage.u3.e2.ReferenceCount = 1;

    /* Loop the memory descriptors */
    for (NextEntry = KeLoaderBlock->MemoryDescriptorListHead.Flink;
         NextEntry != &KeLoaderBlock->MemoryDescriptorListHead;
         NextEntry = NextEntry->Flink)
    {
        /* Get the descriptor */
        Md = CONTAINING_RECORD(NextEntry,
                               MEMORY_ALLOCATION_DESCRIPTOR,
                               ListEntry);

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
                MmPfnDatabase[0][Md->BasePage + i].u3.e1.PageLocation = FreePageList;
                MiInsertInListTail(&MmFreePageListHead,
                                   &MmPfnDatabase[0][Md->BasePage + i]);
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
   MiGetPfnEntry(Pfn)->u1.WsIndex = SwapEntry;
   KeReleaseQueuedSpinLock(LockQueuePfnLock, oldIrql);
}

SWAPENTRY
NTAPI
MmGetSavedSwapEntryPage(PFN_TYPE Pfn)
{
   SWAPENTRY SwapEntry;
   KIRQL oldIrql;

   oldIrql = KeAcquireQueuedSpinLock(LockQueuePfnLock);
   SwapEntry = MiGetPfnEntry(Pfn)->u1.WsIndex;
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

   Page->u3.e2.ReferenceCount++;
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

   RCount = Page->u3.e2.ReferenceCount;

   KeReleaseQueuedSpinLock(LockQueuePfnLock, oldIrql);
   return(RCount);
}

BOOLEAN
NTAPI
MmIsPageInUse(PFN_TYPE Pfn)
{
    return MiIsPfnInUse(MiGetPfnEntry(Pfn));
}

VOID
NTAPI
MiSetConsumer(IN PFN_TYPE Pfn,
              IN ULONG Type)
{
    MiGetPfnEntry(Pfn)->u3.e1.PageLocation = ActiveAndValid;
}

VOID
NTAPI
MmDereferencePage(PFN_TYPE Pfn)
{
   PPHYSICAL_PAGE Page;

   DPRINT("MmDereferencePage(PhysicalAddress %x)\n", Pfn << PAGE_SHIFT);

   Page = MiGetPfnEntry(Pfn);
   ASSERT(Page);

   Page->u3.e2.ReferenceCount--;
   if (Page->u3.e2.ReferenceCount == 0)
   {
      MmAvailablePages++;
      Page->u3.e1.PageLocation = FreePageList;
      MiInsertInListTail(&MmFreePageListHead, Page);
      if (MmFreePageListHead.Total > 8 && 0 == KeReadStateEvent(&ZeroPageThreadEvent))
      {
         KeSetEvent(&ZeroPageThreadEvent, IO_NO_INCREMENT, FALSE);
      }
   }
}

PFN_TYPE
NTAPI
MmAllocPage(ULONG Type)
{
   PFN_TYPE PfnOffset;
   PPHYSICAL_PAGE PageDescriptor;
   BOOLEAN NeedClear = FALSE;

   DPRINT("MmAllocPage()\n");

   if (MmZeroedPageListHead.Total == 0)
   {
       if (MmFreePageListHead.Total == 0)
       {
         /* Check if this allocation is for the PFN DB itself */
         if (MmNumberOfPhysicalPages == 0) 
         {
             ASSERT(FALSE);
         }

         DPRINT1("MmAllocPage(): Out of memory\n");
         return 0;
      }
      PageDescriptor = MiRemoveHeadList(&MmFreePageListHead);

      NeedClear = TRUE;
   }
   else
   {
      PageDescriptor = MiRemoveHeadList(&MmZeroedPageListHead);
   }

   PageDescriptor->u3.e2.ReferenceCount = 1;

   MmAvailablePages--;

   PfnOffset = PageDescriptor - MmPfnDatabase[0];
   if ((NeedClear) && (Type != MC_SYSTEM))
   {
      MiZeroPage(PfnOffset);
   }
     
   PageDescriptor->u3.e1.PageLocation = ActiveAndValid;
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

      if (ZeroPageThreadShouldTerminate)
      {
         DPRINT1("ZeroPageThread: Terminating\n");
         return STATUS_SUCCESS;
      }
      Count = 0;
      oldIrql = KeAcquireQueuedSpinLock(LockQueuePfnLock);
      while (MmFreePageListHead.Total)
      {
         PageDescriptor = MiRemoveHeadList(&MmFreePageListHead);
         /* We set the page to used, because MmCreateVirtualMapping failed with unused pages */
         KeReleaseQueuedSpinLock(LockQueuePfnLock, oldIrql);
         Pfn = PageDescriptor - MmPfnDatabase[0];
         Status = MiZeroPage(Pfn);

         oldIrql = KeAcquireQueuedSpinLock(LockQueuePfnLock);
         if (NT_SUCCESS(Status))
         {
            MiInsertZeroListAtBack(Pfn);
            Count++;
         }
         else
         {
            MiInsertInListTail(&MmFreePageListHead, PageDescriptor);
            PageDescriptor->u3.e1.PageLocation = FreePageList;
         }

      }
      DPRINT("Zeroed %d pages.\n", Count);
      KeResetEvent(&ZeroPageThreadEvent);
      KeReleaseQueuedSpinLock(LockQueuePfnLock, oldIrql);
   }

   return STATUS_SUCCESS;
}

/* EOF */
