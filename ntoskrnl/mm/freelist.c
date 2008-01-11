/* $Id$
 *
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

typedef struct _PHYSICAL_PAGE
{
   union
   {
      struct
      {
        ULONG Type: 2;
        ULONG Consumer: 3;
	ULONG Zero: 1;
      }
      Flags;
      ULONG AllFlags;
   };

   LIST_ENTRY ListEntry;
   ULONG ReferenceCount;
   SWAPENTRY SavedSwapEntry;
   ULONG LockCount;
   ULONG MapCount;
   struct _MM_RMAP_ENTRY* RmapListHead;
}
PHYSICAL_PAGE, *PPHYSICAL_PAGE;


/* GLOBALS ****************************************************************/

static PPHYSICAL_PAGE MmPageArray;
ULONG MmPageArraySize;

static KSPIN_LOCK PageListLock;
static LIST_ENTRY UsedPageListHeads[MC_MAXIMUM];
static LIST_ENTRY FreeZeroedPageListHead;
static LIST_ENTRY FreeUnzeroedPageListHead;
static LIST_ENTRY BiosPageListHead;

static KEVENT ZeroPageThreadEvent;
static BOOLEAN ZeroPageThreadShouldTerminate = FALSE;

static ULONG UnzeroedPageCount = 0;

/* FUNCTIONS *************************************************************/

VOID
NTAPI
MmTransferOwnershipPage(PFN_TYPE Pfn, ULONG NewConsumer)
{
   KIRQL oldIrql;

   KeAcquireSpinLock(&PageListLock, &oldIrql);
   if (MmPageArray[Pfn].MapCount != 0)
   {
      DPRINT1("Transfering mapped page.\n");
      KEBUGCHECK(0);
   }
   if (MmPageArray[Pfn].Flags.Type != MM_PHYSICAL_PAGE_USED)
   {
      DPRINT1("Type: %d\n", MmPageArray[Pfn].Flags.Type);
      KEBUGCHECK(0);
   }
   if (MmPageArray[Pfn].ReferenceCount != 1)
   {
      DPRINT1("ReferenceCount: %d\n", MmPageArray[Pfn].ReferenceCount);
      KEBUGCHECK(0);
   }
   RemoveEntryList(&MmPageArray[Pfn].ListEntry);
   InsertTailList(&UsedPageListHeads[NewConsumer],
                  &MmPageArray[Pfn].ListEntry);
   MmPageArray[Pfn].Flags.Consumer = NewConsumer;
   KeReleaseSpinLock(&PageListLock, oldIrql);
   MiZeroPage(Pfn);
}

PFN_TYPE
NTAPI
MmGetLRUFirstUserPage(VOID)
{
   PLIST_ENTRY NextListEntry;
   PHYSICAL_PAGE* PageDescriptor;
   KIRQL oldIrql;

   KeAcquireSpinLock(&PageListLock, &oldIrql);
   NextListEntry = UsedPageListHeads[MC_USER].Flink;
   if (NextListEntry == &UsedPageListHeads[MC_USER])
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
MmSetLRULastPage(PFN_TYPE Pfn)
{
   KIRQL oldIrql;

   ASSERT(Pfn < MmPageArraySize);
   KeAcquireSpinLock(&PageListLock, &oldIrql);
   if (MmPageArray[Pfn].Flags.Type == MM_PHYSICAL_PAGE_USED &&
       MmPageArray[Pfn].Flags.Consumer == MC_USER)
   {
      RemoveEntryList(&MmPageArray[Pfn].ListEntry);
      InsertTailList(&UsedPageListHeads[MC_USER],
                     &MmPageArray[Pfn].ListEntry);
   }
   KeReleaseSpinLock(&PageListLock, oldIrql);
}

PFN_TYPE
NTAPI
MmGetLRUNextUserPage(PFN_TYPE PreviousPfn)
{
   PLIST_ENTRY NextListEntry;
   PHYSICAL_PAGE* PageDescriptor;
   KIRQL oldIrql;

   KeAcquireSpinLock(&PageListLock, &oldIrql);
   if (MmPageArray[PreviousPfn].Flags.Type != MM_PHYSICAL_PAGE_USED ||
       MmPageArray[PreviousPfn].Flags.Consumer != MC_USER)
   {
      NextListEntry = UsedPageListHeads[MC_USER].Flink;
   }
   else
   {
      NextListEntry = MmPageArray[PreviousPfn].ListEntry.Flink;
   }
   if (NextListEntry == &UsedPageListHeads[MC_USER])
   {
      KeReleaseSpinLock(&PageListLock, oldIrql);
      return 0;
   }
   PageDescriptor = CONTAINING_RECORD(NextListEntry, PHYSICAL_PAGE, ListEntry);
   KeReleaseSpinLock(&PageListLock, oldIrql);
   return PageDescriptor - MmPageArray;
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
         if (MmPageArray[i].Flags.Type == MM_PHYSICAL_PAGE_FREE)
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
            RemoveEntryList(&MmPageArray[i].ListEntry);
            if (MmPageArray[i].Flags.Zero == 0)
            {
               UnzeroedPageCount--;
            }
            MmStats.NrFreePages--;
            MmStats.NrSystemPages++;
            MmPageArray[i].Flags.Type = MM_PHYSICAL_PAGE_USED;
            MmPageArray[i].Flags.Consumer = MC_NPPOOL;
            MmPageArray[i].ReferenceCount = 1;
            MmPageArray[i].LockCount = 0;
            MmPageArray[i].MapCount = 0;
            MmPageArray[i].SavedSwapEntry = 0;
            InsertTailList(&UsedPageListHeads[MC_NPPOOL],
                           &MmPageArray[i].ListEntry);
         }
         KeReleaseSpinLock(&PageListLock, oldIrql);
         for (i = start; i < (start + length); i++)
         {
            if (MmPageArray[i].Flags.Zero == 0)
            {
	       MiZeroPage(i);
            }
            else
            {
      	       MmPageArray[i].Flags.Zero = 0;
            }
         }
         return start;
      }
   }
   KeReleaseSpinLock(&PageListLock, oldIrql);
   return 0;
}


BOOLEAN
NTAPI
MiIsPfnRam(PADDRESS_RANGE BIOSMemoryMap,
           ULONG AddressRangeCount,
	   PFN_TYPE Pfn)
{
   BOOLEAN IsUsable;
   LARGE_INTEGER BaseAddress;
   LARGE_INTEGER EndAddress;
   ULONG i;
   if (BIOSMemoryMap != NULL && AddressRangeCount > 0)
   {
      IsUsable = FALSE;
      for (i = 0; i < AddressRangeCount; i++)
      {
	 BaseAddress.u.LowPart = BIOSMemoryMap[i].BaseAddrLow;
	 BaseAddress.u.HighPart = BIOSMemoryMap[i].BaseAddrHigh;
	 EndAddress.u.LowPart = BIOSMemoryMap[i].LengthLow;
	 EndAddress.u.HighPart = BIOSMemoryMap[i].LengthHigh;
	 EndAddress.QuadPart += BaseAddress.QuadPart;
	 BaseAddress.QuadPart = PAGE_ROUND_DOWN(BaseAddress.QuadPart);
         EndAddress.QuadPart = PAGE_ROUND_UP(EndAddress.QuadPart);

	 if ((BaseAddress.QuadPart >> PAGE_SHIFT) <= Pfn &&
	     Pfn < (EndAddress.QuadPart >> PAGE_SHIFT))
	 {
	    if (BIOSMemoryMap[i].Type == 1)
	    {
	       IsUsable = TRUE;
	    }
	    else
	    {
	       return FALSE;
	    }
	 }
      }
      return IsUsable;
   }
   return TRUE;
}


PVOID
INIT_FUNCTION
NTAPI
MmInitializePageList(ULONG_PTR FirstPhysKernelAddress,
                     ULONG_PTR LastPhysKernelAddress,
                     ULONG MemorySizeInPages,
                     ULONG_PTR LastKernelAddress,
                     PADDRESS_RANGE BIOSMemoryMap,
                     ULONG AddressRangeCount)
/*
 * FUNCTION: Initializes the page list with all pages free
 * except those known to be reserved and those used by the kernel
 * ARGUMENTS:
 *         FirstKernelAddress = First physical address used by the kernel
 *         LastKernelAddress = Last physical address used by the kernel
 */
{
   ULONG i;
   ULONG Reserved;
   NTSTATUS Status;
   PFN_TYPE LastPage;
   PFN_TYPE FirstUninitializedPage;
   ULONG PdeStart = PsGetCurrentProcess()->Pcb.DirectoryTableBase.LowPart;

   DPRINT("MmInitializePageList(FirstPhysKernelAddress %x, "
          "LastPhysKernelAddress %x, "
          "MemorySizeInPages %x, LastKernelAddress %x)\n",
          FirstPhysKernelAddress,
          LastPhysKernelAddress,
          MemorySizeInPages,
          LastKernelAddress);

   for (i = 0; i < MC_MAXIMUM; i++)
   {
      InitializeListHead(&UsedPageListHeads[i]);
   }
   KeInitializeSpinLock(&PageListLock);
   InitializeListHead(&FreeUnzeroedPageListHead);
   InitializeListHead(&FreeZeroedPageListHead);
   InitializeListHead(&BiosPageListHead);

   LastKernelAddress = PAGE_ROUND_UP(LastKernelAddress);

   MmPageArraySize = MemorySizeInPages;
   Reserved =
      PAGE_ROUND_UP((MmPageArraySize * sizeof(PHYSICAL_PAGE))) / PAGE_SIZE;
   MmPageArray = (PHYSICAL_PAGE *)LastKernelAddress;

   DPRINT("Reserved %d\n", Reserved);

   LastKernelAddress = PAGE_ROUND_UP(LastKernelAddress);
   LastKernelAddress = ((ULONG_PTR)LastKernelAddress + (Reserved * PAGE_SIZE));
   LastPhysKernelAddress = (ULONG_PTR)PAGE_ROUND_UP(LastPhysKernelAddress);
   LastPhysKernelAddress = (ULONG_PTR)LastPhysKernelAddress + (Reserved * PAGE_SIZE);

   MmStats.NrTotalPages = 0;
   MmStats.NrSystemPages = 0;
   MmStats.NrUserPages = 0;
   MmStats.NrReservedPages = 0;
   MmStats.NrFreePages = 0;
   MmStats.NrLockedPages = 0;

   /* Preinitialize the Balancer because we need some pages for pte's */
   MmInitializeBalancer(MemorySizeInPages, 0);

   FirstUninitializedPage = (ULONG_PTR)LastPhysKernelAddress / PAGE_SIZE;
   LastPage = MmPageArraySize;
   for (i = 0; i < Reserved; i++)
   {
      PVOID Address = (char*)MmPageArray + (i * PAGE_SIZE);
      ULONG j, start, end;
      if (!MmIsPagePresent(NULL, Address))
      {
         PFN_TYPE Pfn;
         Pfn = 0;
	 while (Pfn == 0 && LastPage > FirstUninitializedPage)
	 {
            /* Allocate the page from the upper end of the RAM */
            if (MiIsPfnRam(BIOSMemoryMap, AddressRangeCount, --LastPage))
	    {
	       Pfn = LastPage;
	    }
	 }
	 if (Pfn == 0)
	 {
	    Pfn = MmAllocPage(MC_NPPOOL, 0);
            if (Pfn == 0)
	    {
	       KEBUGCHECK(0);
	    }
	 }
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
         /* Setting the page protection is necessary to set the global bit on IA32 */
         MmSetPageProtect(NULL, Address, PAGE_READWRITE);
      }
      memset(Address, 0, PAGE_SIZE);

      start = ((ULONG_PTR)Address - (ULONG_PTR)MmPageArray) / sizeof(PHYSICAL_PAGE);
      end = ((ULONG_PTR)Address - (ULONG_PTR)MmPageArray + PAGE_SIZE) / sizeof(PHYSICAL_PAGE);

      for (j = start; j < end && j < LastPage; j++)
      {
         if (MiIsPfnRam(BIOSMemoryMap, AddressRangeCount, j))
	 {
	    if (j == 0)
	    {
               /*
                * Page zero is reserved for the IVT
                */
               MmPageArray[0].Flags.Type = MM_PHYSICAL_PAGE_BIOS;
               MmPageArray[0].Flags.Consumer = MC_NPPOOL;
               MmPageArray[0].Flags.Zero = 0;
               MmPageArray[0].ReferenceCount = 0;
               InsertTailList(&BiosPageListHead,
                              &MmPageArray[0].ListEntry);
	       MmStats.NrReservedPages++;
	    }
	    else if (j == 1)
	    {

               /*
                * Page one is reserved for the initial KPCR
                */
               MmPageArray[1].Flags.Type = MM_PHYSICAL_PAGE_BIOS;
               MmPageArray[1].Flags.Consumer = MC_NPPOOL;
               MmPageArray[1].Flags.Zero = 0;
               MmPageArray[1].ReferenceCount = 0;
               InsertTailList(&BiosPageListHead,
                              &MmPageArray[1].ListEntry);
	       MmStats.NrReservedPages++;
	    }
	    else if (j == 2)
	    {
               /*
                * Page two is reserved for the KUSER_SHARED_DATA
                */
               MmPageArray[2].Flags.Type = MM_PHYSICAL_PAGE_BIOS;
               MmPageArray[2].Flags.Consumer = MC_NPPOOL;
               MmPageArray[2].Flags.Zero = 0;
               MmPageArray[2].ReferenceCount = 0;
               InsertTailList(&BiosPageListHead,
                              &MmPageArray[2].ListEntry);
	       MmStats.NrReservedPages++;
	    }
        /* Protect the Page Directory. This will be changed in r3 */
        else if (j >= (PdeStart / PAGE_SIZE) && j < (MmFreeLdrPageDirectoryEnd / PAGE_SIZE))
	    {
               MmPageArray[j].Flags.Type = MM_PHYSICAL_PAGE_BIOS;
               MmPageArray[j].Flags.Zero = 0;
               MmPageArray[j].Flags.Consumer = MC_NPPOOL;
               MmPageArray[j].ReferenceCount = 1;
               InsertTailList(&BiosPageListHead,
                              &MmPageArray[j].ListEntry);
	       MmStats.NrReservedPages++;
	    }
	    else if (j >= 0xa0000 / PAGE_SIZE && j < 0x100000 / PAGE_SIZE)
	    {
               MmPageArray[j].Flags.Type = MM_PHYSICAL_PAGE_BIOS;
               MmPageArray[j].Flags.Zero = 0;
               MmPageArray[j].Flags.Consumer = MC_NPPOOL;
               MmPageArray[j].ReferenceCount = 1;
               InsertTailList(&BiosPageListHead,
                              &MmPageArray[j].ListEntry);
	       MmStats.NrReservedPages++;
	    }
	    else if (j >= (ULONG)FirstPhysKernelAddress/PAGE_SIZE &&
		     j < (ULONG)LastPhysKernelAddress/PAGE_SIZE)
	    {
               MmPageArray[j].Flags.Type = MM_PHYSICAL_PAGE_USED;
               MmPageArray[j].Flags.Zero = 0;
               MmPageArray[j].Flags.Consumer = MC_NPPOOL;
	       /* Reference count 2, because we're having ReferenceCount track
		  MapCount as well. */
               MmPageArray[j].ReferenceCount = 2;
               MmPageArray[j].MapCount = 1;
               InsertTailList(&UsedPageListHeads[MC_NPPOOL],
                              &MmPageArray[j].ListEntry);
	       MmStats.NrSystemPages++;
	    }
	    else
	    {
               MmPageArray[j].Flags.Type = MM_PHYSICAL_PAGE_FREE;
               MmPageArray[j].Flags.Zero = 0;
               MmPageArray[j].ReferenceCount = 0;
               InsertTailList(&FreeUnzeroedPageListHead,
                              &MmPageArray[j].ListEntry);
               UnzeroedPageCount++;
	       MmStats.NrFreePages++;
	    }
	 }
	 else
	 {
            MmPageArray[j].Flags.Type = MM_PHYSICAL_PAGE_BIOS;
            MmPageArray[j].Flags.Consumer = MC_NPPOOL;
            MmPageArray[j].Flags.Zero = 0;
            MmPageArray[j].ReferenceCount = 0;
            InsertTailList(&BiosPageListHead,
                           &MmPageArray[j].ListEntry);
	    MmStats.NrReservedPages++;
	 }
      }
      FirstUninitializedPage = j;

   }

   /* Add the pages from the upper end to the list */
   for (i = LastPage; i < MmPageArraySize; i++)
   {
      if (MiIsPfnRam(BIOSMemoryMap, AddressRangeCount, i))
      {
         MmPageArray[i].Flags.Type = MM_PHYSICAL_PAGE_USED;
         MmPageArray[i].Flags.Zero = 0;
         MmPageArray[i].Flags.Consumer = MC_NPPOOL;
         MmPageArray[i].ReferenceCount = 2;
         MmPageArray[i].MapCount = 1;
         InsertTailList(&UsedPageListHeads[MC_NPPOOL],
                        &MmPageArray[i].ListEntry);
	 MmStats.NrSystemPages++;
      }
      else
      {
         MmPageArray[i].Flags.Type = MM_PHYSICAL_PAGE_BIOS;
         MmPageArray[i].Flags.Consumer = MC_NPPOOL;
         MmPageArray[i].Flags.Zero = 0;
         MmPageArray[i].ReferenceCount = 0;
         InsertTailList(&BiosPageListHead,
                        &MmPageArray[i].ListEntry);
	 MmStats.NrReservedPages++;
      }
   }



   KeInitializeEvent(&ZeroPageThreadEvent, NotificationEvent, TRUE);

   MmStats.NrTotalPages = MmStats.NrFreePages + MmStats.NrSystemPages +
                          MmStats.NrReservedPages + MmStats.NrUserPages;
   MmInitializeBalancer(MmStats.NrFreePages, MmStats.NrSystemPages + MmStats.NrReservedPages);
   return((PVOID)LastKernelAddress);
}

VOID
NTAPI
MmSetFlagsPage(PFN_TYPE Pfn, ULONG Flags)
{
   KIRQL oldIrql;

   ASSERT(Pfn < MmPageArraySize);
   KeAcquireSpinLock(&PageListLock, &oldIrql);
   MmPageArray[Pfn].AllFlags = Flags;
   KeReleaseSpinLock(&PageListLock, oldIrql);
}

VOID
NTAPI
MmSetRmapListHeadPage(PFN_TYPE Pfn, struct _MM_RMAP_ENTRY* ListHead)
{
   MmPageArray[Pfn].RmapListHead = ListHead;
}

struct _MM_RMAP_ENTRY*
NTAPI
MmGetRmapListHeadPage(PFN_TYPE Pfn)
{
   return(MmPageArray[Pfn].RmapListHead);
}

VOID
NTAPI
MmMarkPageMapped(PFN_TYPE Pfn)
{
   KIRQL oldIrql;

   if (Pfn < MmPageArraySize)
   {
      KeAcquireSpinLock(&PageListLock, &oldIrql);
      if (MmPageArray[Pfn].Flags.Type == MM_PHYSICAL_PAGE_FREE)
      {
         DPRINT1("Mapping non-used page\n");
         KEBUGCHECK(0);
      }
      MmPageArray[Pfn].MapCount++;
      MmPageArray[Pfn].ReferenceCount++;
      KeReleaseSpinLock(&PageListLock, oldIrql);
   }
}

VOID
NTAPI
MmMarkPageUnmapped(PFN_TYPE Pfn)
{
   KIRQL oldIrql;

   if (Pfn < MmPageArraySize)
   {
      KeAcquireSpinLock(&PageListLock, &oldIrql);
      if (MmPageArray[Pfn].Flags.Type == MM_PHYSICAL_PAGE_FREE)
      {
         DPRINT1("Unmapping non-used page\n");
         KEBUGCHECK(0);
      }
      if (MmPageArray[Pfn].MapCount == 0)
      {
         DPRINT1("Unmapping not mapped page\n");
         KEBUGCHECK(0);
      }
      MmPageArray[Pfn].MapCount--;
      MmPageArray[Pfn].ReferenceCount--;
      KeReleaseSpinLock(&PageListLock, oldIrql);
   }
}

ULONG
NTAPI
MmGetFlagsPage(PFN_TYPE Pfn)
{
   KIRQL oldIrql;
   ULONG Flags;

   ASSERT(Pfn < MmPageArraySize);
   KeAcquireSpinLock(&PageListLock, &oldIrql);
   Flags = MmPageArray[Pfn].AllFlags;
   KeReleaseSpinLock(&PageListLock, oldIrql);

   return(Flags);
}


VOID
NTAPI
MmSetSavedSwapEntryPage(PFN_TYPE Pfn,  SWAPENTRY SavedSwapEntry)
{
   KIRQL oldIrql;

   ASSERT(Pfn < MmPageArraySize);
   KeAcquireSpinLock(&PageListLock, &oldIrql);
   MmPageArray[Pfn].SavedSwapEntry = SavedSwapEntry;
   KeReleaseSpinLock(&PageListLock, oldIrql);
}

SWAPENTRY
NTAPI
MmGetSavedSwapEntryPage(PFN_TYPE Pfn)
{
   SWAPENTRY SavedSwapEntry;
   KIRQL oldIrql;

   ASSERT(Pfn < MmPageArraySize);
   KeAcquireSpinLock(&PageListLock, &oldIrql);
   SavedSwapEntry = MmPageArray[Pfn].SavedSwapEntry;
   KeReleaseSpinLock(&PageListLock, oldIrql);

   return(SavedSwapEntry);
}

VOID
NTAPI
MmReferencePageUnsafe(PFN_TYPE Pfn)
{
   KIRQL oldIrql;

   DPRINT("MmReferencePageUnsafe(PysicalAddress %x)\n", Pfn << PAGE_SHIFT);

   if (Pfn == 0 || Pfn >= MmPageArraySize)
   {
      return;
   }

   KeAcquireSpinLock(&PageListLock, &oldIrql);

   if (MmPageArray[Pfn].Flags.Type != MM_PHYSICAL_PAGE_USED)
   {
      DPRINT1("Referencing non-used page\n");
      KEBUGCHECK(0);
   }

   MmPageArray[Pfn].ReferenceCount++;
   KeReleaseSpinLock(&PageListLock, oldIrql);
}

VOID
NTAPI
MmReferencePage(PFN_TYPE Pfn)
{
   DPRINT("MmReferencePage(PysicalAddress %x)\n", Pfn << PAGE_SHIFT);

   if (Pfn == 0 || Pfn >= MmPageArraySize)
   {
      KEBUGCHECK(0);
   }

   MmReferencePageUnsafe(Pfn);
}

ULONG
NTAPI
MmGetReferenceCountPage(PFN_TYPE Pfn)
{
   KIRQL oldIrql;
   ULONG RCount;

   DPRINT("MmGetReferenceCountPage(PhysicalAddress %x)\n", Pfn << PAGE_SHIFT);

   if (Pfn == 0 || Pfn >= MmPageArraySize)
   {
      KEBUGCHECK(0);
   }

   KeAcquireSpinLock(&PageListLock, &oldIrql);

   if (MmPageArray[Pfn].Flags.Type != MM_PHYSICAL_PAGE_USED)
   {
      DPRINT1("Getting reference count for free page\n");
      KEBUGCHECK(0);
   }

   RCount = MmPageArray[Pfn].ReferenceCount;

   KeReleaseSpinLock(&PageListLock, oldIrql);
   return(RCount);
}

BOOLEAN
NTAPI
MmIsUsablePage(PFN_TYPE Pfn)
{

   DPRINT("MmIsUsablePage(PhysicalAddress %x)\n", Pfn << PAGE_SHIFT);

   if (Pfn == 0 || Pfn >= MmPageArraySize)
   {
      KEBUGCHECK(0);
   }

   if (MmPageArray[Pfn].Flags.Type != MM_PHYSICAL_PAGE_USED &&
         MmPageArray[Pfn].Flags.Type != MM_PHYSICAL_PAGE_BIOS)
   {
      return(FALSE);
   }

   return(TRUE);
}

VOID
NTAPI
MmDereferencePage(PFN_TYPE Pfn)
{
   KIRQL oldIrql;

   DPRINT("MmDereferencePage(PhysicalAddress %x)\n", Pfn << PAGE_SHIFT);

   if (Pfn == 0 || Pfn >= MmPageArraySize)
   {
      KEBUGCHECK(0);
   }

   KeAcquireSpinLock(&PageListLock, &oldIrql);

   if (MmPageArray[Pfn].Flags.Type != MM_PHYSICAL_PAGE_USED)
   {
      DPRINT1("Dereferencing free page\n");
      KEBUGCHECK(0);
   }
   if (MmPageArray[Pfn].ReferenceCount == 0)
   {
      DPRINT1("Derefrencing page with reference count 0\n");
      KEBUGCHECK(0);
   }

   MmPageArray[Pfn].ReferenceCount--;
   if (MmPageArray[Pfn].ReferenceCount == 0)
   {
      MmStats.NrFreePages++;
      MmStats.NrSystemPages--;
      RemoveEntryList(&MmPageArray[Pfn].ListEntry);
      if (MmPageArray[Pfn].RmapListHead != NULL)
      {
         DPRINT1("Freeing page with rmap entries.\n");
         KEBUGCHECK(0);
      }
      if (MmPageArray[Pfn].MapCount != 0)
      {
         DPRINT1("Freeing mapped page (0x%x count %d)\n",
                  Pfn << PAGE_SHIFT, MmPageArray[Pfn].MapCount);
         KEBUGCHECK(0);
      }
      if (MmPageArray[Pfn].LockCount > 0)
      {
         DPRINT1("Freeing locked page\n");
         KEBUGCHECK(0);
      }
      if (MmPageArray[Pfn].SavedSwapEntry != 0)
      {
         DPRINT1("Freeing page with swap entry.\n");
         KEBUGCHECK(0);
      }
      if (MmPageArray[Pfn].Flags.Type != MM_PHYSICAL_PAGE_USED)
      {
         DPRINT1("Freeing page with flags %x\n",
                  MmPageArray[Pfn].Flags.Type);
         KEBUGCHECK(0);
      }
      MmPageArray[Pfn].Flags.Type = MM_PHYSICAL_PAGE_FREE;
      MmPageArray[Pfn].Flags.Consumer = MC_MAXIMUM;
      InsertTailList(&FreeUnzeroedPageListHead,
                     &MmPageArray[Pfn].ListEntry);
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

   DPRINT("MmGetLockCountPage(PhysicalAddress %x)\n", Pfn << PAGE_SHIFT);

   if (Pfn == 0 || Pfn >= MmPageArraySize)
   {
      KEBUGCHECK(0);
   }

   KeAcquireSpinLock(&PageListLock, &oldIrql);

   if (MmPageArray[Pfn].Flags.Type != MM_PHYSICAL_PAGE_USED)
   {
      DPRINT1("Getting lock count for free page\n");
      KEBUGCHECK(0);
   }

   LockCount = MmPageArray[Pfn].LockCount;
   KeReleaseSpinLock(&PageListLock, oldIrql);

   return(LockCount);
}

VOID
NTAPI
MmLockPageUnsafe(PFN_TYPE Pfn)
{
   KIRQL oldIrql;

   DPRINT("MmLockPageUnsafe(PhysicalAddress %x)\n", Pfn << PAGE_SHIFT);

   if (Pfn == 0 || Pfn >= MmPageArraySize)
   {
      return;
   }

   KeAcquireSpinLock(&PageListLock, &oldIrql);

   if (MmPageArray[Pfn].Flags.Type != MM_PHYSICAL_PAGE_USED)
   {
      DPRINT1("Locking free page\n");
      KEBUGCHECK(0);
   }

   MmPageArray[Pfn].LockCount++;
   KeReleaseSpinLock(&PageListLock, oldIrql);
}

VOID
NTAPI
MmLockPage(PFN_TYPE Pfn)
{
   DPRINT("MmLockPage(PhysicalAddress %x)\n", Pfn << PAGE_SHIFT);

   if (Pfn == 0 || Pfn >= MmPageArraySize)
   {
      KEBUGCHECK(0);
   }

   MmLockPageUnsafe(Pfn);
}

VOID
NTAPI
MmUnlockPage(PFN_TYPE Pfn)
{
   KIRQL oldIrql;

   DPRINT("MmUnlockPage(PhysicalAddress %x)\n", Pfn << PAGE_SHIFT);

   if (Pfn == 0 || Pfn >= MmPageArraySize)
   {
      KEBUGCHECK(0);
   }

   KeAcquireSpinLock(&PageListLock, &oldIrql);

   if (MmPageArray[Pfn].Flags.Type != MM_PHYSICAL_PAGE_USED)
   {
      DPRINT1("Unlocking free page\n");
      KEBUGCHECK(0);
   }

   MmPageArray[Pfn].LockCount--;
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
   InsertTailList(&UsedPageListHeads[Consumer], ListEntry);

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
         InsertTailList(&UsedPageListHeads[Consumer], &PageDescriptor->ListEntry);

         MmStats.NrSystemPages++;
         MmStats.NrFreePages--;

         /* Remember the page */
         pfn = PageDescriptor - MmPageArray;
         Pages[NumberOfPagesFound++] = pfn;
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
            RemoveEntryList(&PageDescriptor->ListEntry);
            InsertTailList(&UsedPageListHeads[Consumer], &PageDescriptor->ListEntry);

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
      if (MmPageArray[pfn].Flags.Zero == 0)
      {
         MiZeroPage(pfn);
      }
      else
      {
         MmPageArray[pfn].Flags.Zero = 0;
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
