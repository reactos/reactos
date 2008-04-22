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
#include <internal/debug.h>

#if defined (ALLOC_PRAGMA)
#pragma alloc_text(INIT, MmInitializePageList)
#endif


/* TYPES *******************************************************************/

#define MM_PHYSICAL_PAGE_FREE    (0x1)
#define MM_PHYSICAL_PAGE_USED    (0x2)
#define MM_PHYSICAL_PAGE_BIOS    (0x3)

/* GLOBALS ****************************************************************/

PPHYSICAL_PAGE MmPageArray;
ULONG MmPageArraySize;

static KSPIN_LOCK PageListLock;
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

   KeAcquireSpinLock(&PageListLock, &oldIrql);
   NextListEntry = UserPageListHead.Flink;
   if (NextListEntry == &UserPageListHead)
   {
      KeReleaseSpinLock(&PageListLock, oldIrql);
      return 0;
   }
   PageDescriptor = CONTAINING_RECORD(NextListEntry, PHYSICAL_PAGE, ListEntry);
   ASSERT_PFN(PageDescriptor);
   KeReleaseSpinLock(&PageListLock, oldIrql);
   return PageDescriptor - MmPageArray;
}

VOID
NTAPI
MmInsertLRULastUserPage(PFN_TYPE Pfn)
{
   KIRQL oldIrql;
   PPHYSICAL_PAGE Page;

   KeAcquireSpinLock(&PageListLock, &oldIrql);
   Page = MiGetPfnEntry(Pfn);
   ASSERT(Page->Flags.Type == MM_PHYSICAL_PAGE_USED);
   ASSERT(Page->Flags.Consumer == MC_USER);
   InsertTailList(&UserPageListHead, &Page->ListEntry);
   KeReleaseSpinLock(&PageListLock, oldIrql);
}

PFN_TYPE
NTAPI
MmGetLRUNextUserPage(PFN_TYPE PreviousPfn)
{
   PLIST_ENTRY NextListEntry;
   PHYSICAL_PAGE* PageDescriptor;
   KIRQL oldIrql;
   PPHYSICAL_PAGE Page;

   KeAcquireSpinLock(&PageListLock, &oldIrql);
   Page = MiGetPfnEntry(PreviousPfn);
   ASSERT(Page->Flags.Type == MM_PHYSICAL_PAGE_USED);
   ASSERT(Page->Flags.Consumer == MC_USER);
   NextListEntry = Page->ListEntry.Flink;
   if (NextListEntry == &UserPageListHead)
   {
      KeReleaseSpinLock(&PageListLock, oldIrql);
      return 0;
   }
   PageDescriptor = CONTAINING_RECORD(NextListEntry, PHYSICAL_PAGE, ListEntry);
   KeReleaseSpinLock(&PageListLock, oldIrql);
   return PageDescriptor - MmPageArray;
}

VOID
NTAPI
MmRemoveLRUUserPage(PFN_TYPE Page)
{
   RemoveEntryList(&MiGetPfnEntry(Page)->ListEntry);
}

PFN_TYPE
NTAPI
MmGetContinuousPages(ULONG NumberOfBytes,
                     PHYSICAL_ADDRESS LowestAcceptableAddress,
                     PHYSICAL_ADDRESS HighestAcceptableAddress,
                     PHYSICAL_ADDRESS BoundaryAddressMultiple)
{
   ULONG NrPages;
   ULONG i, j;
   ULONG start;
   ULONG last;
   ULONG length;
   ULONG boundary;
   KIRQL oldIrql;

   NrPages = PAGE_ROUND_UP(NumberOfBytes) / PAGE_SIZE;

   KeAcquireSpinLock(&PageListLock, &oldIrql);

   last = min(HighestAcceptableAddress.LowPart / PAGE_SIZE, MmPageArraySize - 1);
   boundary = BoundaryAddressMultiple.LowPart / PAGE_SIZE;

   for (j = 0; j < 2; j++)
   {
      start = -1;
      length = 0;
      /* First try to allocate the pages above the 16MB area. This may fail
       * because there are not enough continuous pages or we cannot allocate
       * pages above the 16MB area because the caller has specify an upper limit.
       * The second try uses the specified lower limit.
       */
      for (i = j == 0 ? 0x100000 / PAGE_SIZE : LowestAcceptableAddress.LowPart / PAGE_SIZE; i <= last; )
      {
         if (MiGetPfnEntry(i)->Flags.Type == MM_PHYSICAL_PAGE_FREE)
         {
            if (start == (ULONG)-1)
            {
               start = i;
               length = 1;
            }
            else
            {
               length++;
               if (boundary)
               {
                  if (start / boundary != i / boundary)
                  {
                      start = i;
                      length = 1;
                  }
               }
            }
            if (length == NrPages)
            {
               break;
            }
         }
         else
         {
            start = (ULONG)-1;
         }
         i++;
      }

      if (start != (ULONG)-1 && length == NrPages)
      {
         for (i = start; i < (start + length); i++)
         {
            PPHYSICAL_PAGE Page;
            Page = MiGetPfnEntry(i);
            RemoveEntryList(&Page->ListEntry);
            if (MmPageArray[i].Flags.Zero == 0)
            {
               UnzeroedPageCount--;
            }
            MmStats.NrFreePages--;
            MmStats.NrSystemPages++;
            Page->Flags.Type = MM_PHYSICAL_PAGE_USED;
            Page->Flags.Consumer = MC_NPPOOL;
            Page->ReferenceCount = 1;
            Page->LockCount = 0;
            Page->MapCount = 0;
            Page->SavedSwapEntry = 0;
         }
         KeReleaseSpinLock(&PageListLock, oldIrql);
         for (i = start; i < (start + length); i++)
         {
            if (MiGetPfnEntry(i)->Flags.Zero == 0)
            {
	       MiZeroPage(i);
            }
            else
            {
      	       MiGetPfnEntry(i)->Flags.Zero = 0;
            }
         }
         return start;
      }
   }
   KeReleaseSpinLock(&PageListLock, oldIrql);
   return 0;
}

PFN_TYPE
NTAPI
MmAllocEarlyPage(VOID)
{
    PFN_TYPE Pfn;

    /* Use one of our highest usable pages */
    Pfn = MiFreeDescriptor->BasePage + MiFreeDescriptor->PageCount - 1;
    MiFreeDescriptor->PageCount--;

    /* Return it */
    return Pfn;
}

VOID
NTAPI
MmInitializePageList(VOID)
{
    ULONG i;
    ULONG Reserved;
    NTSTATUS Status;
    PFN_TYPE Pfn = 0;
    PHYSICAL_PAGE UsedPage;
    PLIST_ENTRY NextEntry;
    PMEMORY_ALLOCATION_DESCRIPTOR Md;

    /* Initialize the page lists */
    KeInitializeSpinLock(&PageListLock);
    InitializeListHead(&UserPageListHead);
    InitializeListHead(&FreeUnzeroedPageListHead);
    InitializeListHead(&FreeZeroedPageListHead);
 
    /* Set the size and start of the PFN Database */
    MmPageArray = (PHYSICAL_PAGE *)MmPfnDatabase;
    MmPageArraySize = MmHighestPhysicalPage;
    Reserved = PAGE_ROUND_UP((MmPageArraySize * sizeof(PHYSICAL_PAGE))) / PAGE_SIZE;

    /* Loop every page required to hold the PFN database */
    for (i = 0; i < Reserved; i++)
    {
        PVOID Address = (char*)MmPageArray + (i * PAGE_SIZE);

        /* Check if FreeLDR has already allocated it for us */
        if (!MmIsPagePresent(NULL, Address))
        {
            /* Use one of our highest usable pages */
            Pfn = MmAllocEarlyPage();

            /* Set the PFN */
            Status = MmCreateVirtualMappingForKernel(Address,
                                                     PAGE_READWRITE,
                                                     &Pfn,
                                                     1);
            if (!NT_SUCCESS(Status))
            {
                DPRINT1("Unable to create virtual mapping\n");
                KEBUGCHECK(0);
            }
        }
        else
        {
            /* Setting the page protection is necessary to set the global bit */
            MmSetPageProtect(NULL, Address, PAGE_READWRITE);
        }
    }

    /* Clear the PFN database */
    RtlZeroMemory(MmPageArray, (MmPageArraySize + 1) * sizeof(PHYSICAL_PAGE));

    /* This is what a used page looks like */
    RtlZeroMemory(&UsedPage, sizeof(UsedPage));
    UsedPage.Flags.Type = MM_PHYSICAL_PAGE_USED;
    UsedPage.Flags.Consumer = MC_NPPOOL;
    UsedPage.ReferenceCount = 2;
    UsedPage.MapCount = 1;

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
            /* Loop every page part of the block but valid in the database */
            for (i = 0; i < Md->PageCount; i++)
            {
                /* Skip memory we ignore completely */
                if ((Md->BasePage + i) > MmPageArraySize) break;
                
                /* These are pages reserved by the BIOS/ROMs */
                MmPageArray[Md->BasePage + i].Flags.Type = MM_PHYSICAL_PAGE_BIOS;
                MmPageArray[Md->BasePage + i].Flags.Consumer = MC_NPPOOL;
                MmStats.NrSystemPages++;
            }
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
                MmPageArray[Md->BasePage + i].Flags.Type = MM_PHYSICAL_PAGE_FREE;
                InsertTailList(&FreeUnzeroedPageListHead,
                               &MmPageArray[Md->BasePage + i].ListEntry);
                UnzeroedPageCount++;
                MmStats.NrFreePages++;
            }
        }
        else
        {
            /* Loop every page part of the block */
            for (i = 0; i < Md->PageCount; i++)
            {
                /* Everything else is used memory */
                MmPageArray[Md->BasePage + i] = UsedPage;
                MmStats.NrSystemPages++;
            }
        }
    }

    /* Finally handle the pages describing the PFN database themselves */
    for (i = (MiFreeDescriptor->BasePage + MiFreeDescriptor->PageCount);
         i < (MiFreeDescriptorOrg.BasePage + MiFreeDescriptorOrg.PageCount);
         i++)
    {
        /* Ensure this page was not added previously */
        ASSERT(MmPageArray[i].Flags.Type == 0);

        /* Mark it as used kernel memory */
        MmPageArray[i] = UsedPage;
        MmStats.NrSystemPages++;
    }

    KeInitializeEvent(&ZeroPageThreadEvent, NotificationEvent, TRUE);

    DPRINT("Pages: %x %x\n", MmStats.NrFreePages, MmStats.NrSystemPages);
    MmStats.NrTotalPages = MmStats.NrFreePages + MmStats.NrSystemPages + MmStats.NrUserPages;
    MmInitializeBalancer(MmStats.NrFreePages, MmStats.NrSystemPages);
}

VOID
NTAPI
MmSetFlagsPage(PFN_TYPE Pfn, ULONG Flags)
{
   KIRQL oldIrql;

   KeAcquireSpinLock(&PageListLock, &oldIrql);
   MiGetPfnEntry(Pfn)->AllFlags = Flags;
   KeReleaseSpinLock(&PageListLock, oldIrql);
}

VOID
NTAPI
MmSetRmapListHeadPage(PFN_TYPE Pfn, struct _MM_RMAP_ENTRY* ListHead)
{
   KIRQL oldIrql;
    
   KeAcquireSpinLock(&PageListLock, &oldIrql);    
   MiGetPfnEntry(Pfn)->RmapListHead = ListHead;
   KeReleaseSpinLock(&PageListLock, oldIrql);
}

struct _MM_RMAP_ENTRY*
NTAPI
MmGetRmapListHeadPage(PFN_TYPE Pfn)
{
   KIRQL oldIrql;
   struct _MM_RMAP_ENTRY* ListHead;
    
   KeAcquireSpinLock(&PageListLock, &oldIrql);    
   ListHead = MiGetPfnEntry(Pfn)->RmapListHead;
   KeReleaseSpinLock(&PageListLock, oldIrql);
    
   return(ListHead);
}

VOID
NTAPI
MmMarkPageMapped(PFN_TYPE Pfn)
{
   KIRQL oldIrql;
   PPHYSICAL_PAGE Page;

   if (Pfn <= MmPageArraySize)
   {
      KeAcquireSpinLock(&PageListLock, &oldIrql);
      Page = MiGetPfnEntry(Pfn);
      if (Page->Flags.Type == MM_PHYSICAL_PAGE_FREE)
      {
         DPRINT1("Mapping non-used page\n");
         KEBUGCHECK(0);
      }
      Page->MapCount++;
      Page->ReferenceCount++;
      KeReleaseSpinLock(&PageListLock, oldIrql);
   }
}

VOID
NTAPI
MmMarkPageUnmapped(PFN_TYPE Pfn)
{
   KIRQL oldIrql;
   PPHYSICAL_PAGE Page;

   if (Pfn <= MmPageArraySize)
   {
      KeAcquireSpinLock(&PageListLock, &oldIrql);
      Page = MiGetPfnEntry(Pfn);
      if (Page->Flags.Type == MM_PHYSICAL_PAGE_FREE)
      {
         DPRINT1("Unmapping non-used page\n");
         KEBUGCHECK(0);
      }
      if (Page->MapCount == 0)
      {
         DPRINT1("Unmapping not mapped page\n");
         KEBUGCHECK(0);
      }
      Page->MapCount--;
      Page->ReferenceCount--;
      KeReleaseSpinLock(&PageListLock, oldIrql);
   }
}

ULONG
NTAPI
MmGetFlagsPage(PFN_TYPE Pfn)
{
   KIRQL oldIrql;
   ULONG Flags;

   KeAcquireSpinLock(&PageListLock, &oldIrql);
   Flags = MiGetPfnEntry(Pfn)->AllFlags;
   KeReleaseSpinLock(&PageListLock, oldIrql);

   return(Flags);
}


VOID
NTAPI
MmSetSavedSwapEntryPage(PFN_TYPE Pfn,  SWAPENTRY SavedSwapEntry)
{
   KIRQL oldIrql;

   KeAcquireSpinLock(&PageListLock, &oldIrql);
   MiGetPfnEntry(Pfn)->SavedSwapEntry = SavedSwapEntry;
   KeReleaseSpinLock(&PageListLock, oldIrql);
}

SWAPENTRY
NTAPI
MmGetSavedSwapEntryPage(PFN_TYPE Pfn)
{
   SWAPENTRY SavedSwapEntry;
   KIRQL oldIrql;

   KeAcquireSpinLock(&PageListLock, &oldIrql);
   SavedSwapEntry = MiGetPfnEntry(Pfn)->SavedSwapEntry;
   KeReleaseSpinLock(&PageListLock, oldIrql);

   return(SavedSwapEntry);
}

VOID
NTAPI
MmReferencePageUnsafe(PFN_TYPE Pfn)
{
   KIRQL oldIrql;
   PPHYSICAL_PAGE Page;

   DPRINT("MmReferencePageUnsafe(PysicalAddress %x)\n", Pfn << PAGE_SHIFT);

   if (Pfn == 0 || Pfn > MmPageArraySize)
   {
      return;
   }

   KeAcquireSpinLock(&PageListLock, &oldIrql);

   Page = MiGetPfnEntry(Pfn);
   if (Page->Flags.Type != MM_PHYSICAL_PAGE_USED)
   {
      DPRINT1("Referencing non-used page\n");
      KEBUGCHECK(0);
   }

   Page->ReferenceCount++;
   KeReleaseSpinLock(&PageListLock, oldIrql);
}

VOID
NTAPI
MmReferencePage(PFN_TYPE Pfn)
{
   DPRINT("MmReferencePage(PysicalAddress %x)\n", Pfn << PAGE_SHIFT);

   MmReferencePageUnsafe(Pfn);
}

ULONG
NTAPI
MmGetReferenceCountPage(PFN_TYPE Pfn)
{
   KIRQL oldIrql;
   ULONG RCount;
   PPHYSICAL_PAGE Page;

   DPRINT("MmGetReferenceCountPage(PhysicalAddress %x)\n", Pfn << PAGE_SHIFT);

   KeAcquireSpinLock(&PageListLock, &oldIrql);
   Page = MiGetPfnEntry(Pfn);
   if (Page->Flags.Type != MM_PHYSICAL_PAGE_USED)
   {
      DPRINT1("Getting reference count for free page\n");
      KEBUGCHECK(0);
   }

   RCount = Page->ReferenceCount;

   KeReleaseSpinLock(&PageListLock, oldIrql);
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
   KIRQL oldIrql;
   PPHYSICAL_PAGE Page;

   DPRINT("MmDereferencePage(PhysicalAddress %x)\n", Pfn << PAGE_SHIFT);

   KeAcquireSpinLock(&PageListLock, &oldIrql);
    
   Page = MiGetPfnEntry(Pfn);

   if (Page->Flags.Type != MM_PHYSICAL_PAGE_USED)
   {
      DPRINT1("Dereferencing free page\n");
      KEBUGCHECK(0);
   }
   if (Page->ReferenceCount == 0)
   {
      DPRINT1("Derefrencing page with reference count 0\n");
      KEBUGCHECK(0);
   }

   Page->ReferenceCount--;
   if (Page->ReferenceCount == 0)
   {
      MmStats.NrFreePages++;
      MmStats.NrSystemPages--;
      if (Page->Flags.Consumer == MC_USER) RemoveEntryList(&Page->ListEntry);
      if (Page->RmapListHead != NULL)
      {
         DPRINT1("Freeing page with rmap entries.\n");
         KEBUGCHECK(0);
      }
      if (Page->MapCount != 0)
      {
         DPRINT1("Freeing mapped page (0x%x count %d)\n",
                  Pfn << PAGE_SHIFT, Page->MapCount);
         KEBUGCHECK(0);
      }
      if (Page->LockCount > 0)
      {
         DPRINT1("Freeing locked page\n");
         KEBUGCHECK(0);
      }
      if (Page->SavedSwapEntry != 0)
      {
         DPRINT1("Freeing page with swap entry.\n");
         KEBUGCHECK(0);
      }
      if (Page->Flags.Type != MM_PHYSICAL_PAGE_USED)
      {
         DPRINT1("Freeing page with flags %x\n",
                  Page->Flags.Type);
         KEBUGCHECK(0);
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
   KeReleaseSpinLock(&PageListLock, oldIrql);
}

ULONG
NTAPI
MmGetLockCountPage(PFN_TYPE Pfn)
{
   KIRQL oldIrql;
   ULONG LockCount;
   PPHYSICAL_PAGE Page;

   DPRINT("MmGetLockCountPage(PhysicalAddress %x)\n", Pfn << PAGE_SHIFT);

   KeAcquireSpinLock(&PageListLock, &oldIrql);

   Page = MiGetPfnEntry(Pfn);
   if (Page->Flags.Type != MM_PHYSICAL_PAGE_USED)
   {
      DPRINT1("Getting lock count for free page\n");
      KEBUGCHECK(0);
   }

   LockCount = Page->LockCount;
   KeReleaseSpinLock(&PageListLock, oldIrql);

   return(LockCount);
}

VOID
NTAPI
MmLockPageUnsafe(PFN_TYPE Pfn)
{
   KIRQL oldIrql;
   PPHYSICAL_PAGE Page;

   DPRINT("MmLockPageUnsafe(PhysicalAddress %x)\n", Pfn << PAGE_SHIFT);

   KeAcquireSpinLock(&PageListLock, &oldIrql);

   Page = MiGetPfnEntry(Pfn);
   if (Page->Flags.Type != MM_PHYSICAL_PAGE_USED)
   {
      DPRINT1("Locking free page\n");
      KEBUGCHECK(0);
   }

   Page->LockCount++;
   KeReleaseSpinLock(&PageListLock, oldIrql);
}

VOID
NTAPI
MmLockPage(PFN_TYPE Pfn)
{
   DPRINT("MmLockPage(PhysicalAddress %x)\n", Pfn << PAGE_SHIFT);

   MmLockPageUnsafe(Pfn);
}

VOID
NTAPI
MmUnlockPage(PFN_TYPE Pfn)
{
   KIRQL oldIrql;
   PPHYSICAL_PAGE Page;

   DPRINT("MmUnlockPage(PhysicalAddress %x)\n", Pfn << PAGE_SHIFT);

   KeAcquireSpinLock(&PageListLock, &oldIrql);

   Page = MiGetPfnEntry(Pfn);
   if (Page->Flags.Type != MM_PHYSICAL_PAGE_USED)
   {
      DPRINT1("Unlocking free page\n");
      KEBUGCHECK(0);
   }

   Page->LockCount--;
   KeReleaseSpinLock(&PageListLock, oldIrql);
}

PFN_TYPE
NTAPI
MmAllocPage(ULONG Consumer, SWAPENTRY SavedSwapEntry)
{
   PFN_TYPE PfnOffset;
   PLIST_ENTRY ListEntry;
   PPHYSICAL_PAGE PageDescriptor;
   KIRQL oldIrql;
   BOOLEAN NeedClear = FALSE;

   DPRINT("MmAllocPage()\n");

   KeAcquireSpinLock(&PageListLock, &oldIrql);
   if (IsListEmpty(&FreeZeroedPageListHead))
   {
      if (IsListEmpty(&FreeUnzeroedPageListHead))
      {
         /* Check if this allocation is for the PFN DB itself */
         if (MmStats.NrTotalPages == 0) 
         {
             /* Allocate an early page -- we'll account for it later */
             KeReleaseSpinLock(&PageListLock, oldIrql);
             PfnOffset = MmAllocEarlyPage();
             MiZeroPage(PfnOffset);
             return PfnOffset;
         }

         DPRINT1("MmAllocPage(): Out of memory\n");
         KeReleaseSpinLock(&PageListLock, oldIrql);
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
      KEBUGCHECK(0);
   }
   if (PageDescriptor->MapCount != 0)
   {
      DPRINT1("Got mapped page from freelist\n");
      KEBUGCHECK(0);
   }
   if (PageDescriptor->ReferenceCount != 0)
   {
      DPRINT1("%d\n", PageDescriptor->ReferenceCount);
      KEBUGCHECK(0);
   }
   PageDescriptor->Flags.Type = MM_PHYSICAL_PAGE_USED;
   PageDescriptor->Flags.Consumer = Consumer;
   PageDescriptor->ReferenceCount = 1;
   PageDescriptor->LockCount = 0;
   PageDescriptor->MapCount = 0;
   PageDescriptor->SavedSwapEntry = SavedSwapEntry;

   MmStats.NrSystemPages++;
   MmStats.NrFreePages--;

   KeReleaseSpinLock(&PageListLock, oldIrql);

   PfnOffset = PageDescriptor - MmPageArray;
   if (NeedClear)
   {
      MiZeroPage(PfnOffset);
   }
   if (PageDescriptor->MapCount != 0)
   {
      DPRINT1("Returning mapped page.\n");
      KEBUGCHECK(0);
   }
   return PfnOffset;
}

LONG
NTAPI
MmAllocPagesSpecifyRange(ULONG Consumer,
                         PHYSICAL_ADDRESS LowestAddress,
                         PHYSICAL_ADDRESS HighestAddress,
                         ULONG NumberOfPages,
                         PPFN_TYPE Pages)
{
   PPHYSICAL_PAGE PageDescriptor;
   KIRQL oldIrql;
   PFN_TYPE LowestPage, HighestPage;
   PFN_TYPE pfn;
   ULONG NumberOfPagesFound = 0;
   ULONG i;

   DPRINT("MmAllocPagesSpecifyRange()\n"
          "    LowestAddress = 0x%08x%08x\n"
          "    HighestAddress = 0x%08x%08x\n"
          "    NumberOfPages = %d\n",
          LowestAddress.u.HighPart, LowestAddress.u.LowPart,
          HighestAddress.u.HighPart, HighestAddress.u.LowPart,
          NumberOfPages);

   if (NumberOfPages == 0)
      return 0;

   LowestPage = LowestAddress.LowPart / PAGE_SIZE;
   HighestPage = HighestAddress.LowPart / PAGE_SIZE;
   if ((HighestAddress.u.LowPart % PAGE_SIZE) != 0)
      HighestPage++;

   if (LowestPage >= MmPageArraySize)
   {
      DPRINT1("MmAllocPagesSpecifyRange(): Out of memory\n");
      return -1;
   }
   if (HighestPage > MmPageArraySize)
      HighestPage = MmPageArraySize;

   KeAcquireSpinLock(&PageListLock, &oldIrql);
   if (LowestPage == 0 && HighestPage == MmPageArraySize)
   {
      PLIST_ENTRY ListEntry;
      while (NumberOfPagesFound < NumberOfPages)
      {
         if (!IsListEmpty(&FreeZeroedPageListHead))
         {
            ListEntry = RemoveTailList(&FreeZeroedPageListHead);
         }
         else if (!IsListEmpty(&FreeUnzeroedPageListHead))
         {
            ListEntry = RemoveTailList(&FreeUnzeroedPageListHead);
            UnzeroedPageCount--;
         }
         else
         {
            if (NumberOfPagesFound == 0)
            {
               KeReleaseSpinLock(&PageListLock, oldIrql);
               DPRINT1("MmAllocPagesSpecifyRange(): Out of memory\n");
               return -1;
            }
            else
            {
               break;
            }
         }
         PageDescriptor = CONTAINING_RECORD(ListEntry, PHYSICAL_PAGE, ListEntry);

         ASSERT(PageDescriptor->Flags.Type == MM_PHYSICAL_PAGE_FREE);
         ASSERT(PageDescriptor->MapCount == 0);
         ASSERT(PageDescriptor->ReferenceCount == 0);

         /* Allocate the page */
         PageDescriptor->Flags.Type = MM_PHYSICAL_PAGE_USED;
         PageDescriptor->Flags.Consumer = Consumer;
         PageDescriptor->ReferenceCount = 1;
         PageDescriptor->LockCount = 0;
         PageDescriptor->MapCount = 0;
         PageDescriptor->SavedSwapEntry = 0; /* FIXME: Do we need swap entries? */

         MmStats.NrSystemPages++;
         MmStats.NrFreePages--;

         /* Remember the page */
         pfn = PageDescriptor - MmPageArray;
         Pages[NumberOfPagesFound++] = pfn;
         if(Consumer == MC_USER) MmInsertLRULastUserPage(pfn);
      }
   }
   else
   {
      INT LookForZeroedPages;
      for (LookForZeroedPages = 1; LookForZeroedPages >= 0; LookForZeroedPages--)
      {
         for (pfn = LowestPage; pfn < HighestPage; pfn++)
         {
            PageDescriptor = MmPageArray + pfn;

            if (PageDescriptor->Flags.Type != MM_PHYSICAL_PAGE_FREE)
               continue;
            if (PageDescriptor->Flags.Zero != LookForZeroedPages)
               continue;

            ASSERT(PageDescriptor->MapCount == 0);
            ASSERT(PageDescriptor->ReferenceCount == 0);

            /* Allocate the page */
            PageDescriptor->Flags.Type = MM_PHYSICAL_PAGE_USED;
            PageDescriptor->Flags.Consumer = Consumer;
            PageDescriptor->ReferenceCount = 1;
            PageDescriptor->LockCount = 0;
            PageDescriptor->MapCount = 0;
            PageDescriptor->SavedSwapEntry = 0; /* FIXME: Do we need swap entries? */

            if (!PageDescriptor->Flags.Zero)
               UnzeroedPageCount--;
            MmStats.NrSystemPages++;
            MmStats.NrFreePages--;

            /* Remember the page */
            Pages[NumberOfPagesFound++] = pfn;
            if (NumberOfPagesFound == NumberOfPages)
               break;
         }
         if (NumberOfPagesFound == NumberOfPages)
            break;
      }
   }
   KeReleaseSpinLock(&PageListLock, oldIrql);

   /* Zero unzero-ed pages */
   for (i = 0; i < NumberOfPagesFound; i++)
   {
      pfn = Pages[i];
      if (MiGetPfnEntry(pfn)->Flags.Zero == 0)
      {
         MiZeroPage(pfn);
      }
      else
      {
         MiGetPfnEntry(pfn)->Flags.Zero = 0;
      }
   }

   return NumberOfPagesFound;
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
         KEBUGCHECK(0);
      }

      if (ZeroPageThreadShouldTerminate)
      {
         DPRINT1("ZeroPageThread: Terminating\n");
         return STATUS_SUCCESS;
      }
      Count = 0;
      KeAcquireSpinLock(&PageListLock, &oldIrql);
      while (!IsListEmpty(&FreeUnzeroedPageListHead))
      {
         ListEntry = RemoveTailList(&FreeUnzeroedPageListHead);
         UnzeroedPageCount--;
         PageDescriptor = CONTAINING_RECORD(ListEntry, PHYSICAL_PAGE, ListEntry);
         /* We set the page to used, because MmCreateVirtualMapping failed with unused pages */
         PageDescriptor->Flags.Type = MM_PHYSICAL_PAGE_USED;
         KeReleaseSpinLock(&PageListLock, oldIrql);
         Pfn = PageDescriptor - MmPageArray;
         Status = MiZeroPage(Pfn);

         KeAcquireSpinLock(&PageListLock, &oldIrql);
         if (PageDescriptor->MapCount != 0)
         {
            DPRINT1("Mapped page on freelist.\n");
            KEBUGCHECK(0);
         }
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
      KeReleaseSpinLock(&PageListLock, oldIrql);
   }

   return STATUS_SUCCESS;
}

/* EOF */
