/*
 * COPYRIGHT:    See COPYING in the top level directory
 * PROJECT:      ReactOS kernel
 * FILE:         ntoskrnl/mm/freelist.c
 * PURPOSE:      Handle the list of free physical pages
 * PROGRAMMER:   David Welch (welch@cwcom.net)
 * UPDATE HISTORY:
 *               27/05/98: Created
 *               18/08/98: Added a fix from Robert Bergkvist
 */

/* INCLUDES ****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

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

static HANDLE ZeroPageThreadHandle;
static CLIENT_ID ZeroPageThreadId;
static KEVENT ZeroPageThreadEvent;

static ULONG UnzeroedPageCount = 0;

/* FUNCTIONS *************************************************************/

VOID
MmTransferOwnershipPage(PFN_TYPE Pfn, ULONG NewConsumer)
{
   KIRQL oldIrql;

   KeAcquireSpinLock(&PageListLock, &oldIrql);
   if (MmPageArray[Pfn].MapCount != 0)
   {
      DbgPrint("Transfering mapped page.\n");
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
MmGetContinuousPages(ULONG NumberOfBytes,
                     PHYSICAL_ADDRESS LowestAcceptableAddress,
                     PHYSICAL_ADDRESS HighestAcceptableAddress,
                     ULONG Alignment)
{
   ULONG NrPages;
   ULONG i;
   ULONG start;
   ULONG length;
   KIRQL oldIrql;

   NrPages = PAGE_ROUND_UP(NumberOfBytes) / PAGE_SIZE;

   KeAcquireSpinLock(&PageListLock, &oldIrql);

   start = -1;
   length = 0;
   for (i = (LowestAcceptableAddress.QuadPart / PAGE_SIZE); i < (HighestAcceptableAddress.QuadPart / PAGE_SIZE); )
   {
      if (MmPageArray[i].Flags.Type ==  MM_PHYSICAL_PAGE_FREE)
      {
         if (start == -1)
         {
            start = i;
            length = 1;
         }
         else
         {
            length++;
         }
         i++;
         if (length == NrPages)
         {
            break;
         }
      }
      else
      {
         start = -1;
         /*
          * Fast forward to the base of the next aligned region
          */
         i = ROUND_UP((i + 1), (Alignment / PAGE_SIZE));
      }
   }
   if (start == -1 || length != NrPages)
   {
      KeReleaseSpinLock(&PageListLock, oldIrql);
      return 0;
   }
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


BOOLEAN
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
         

PVOID INIT_FUNCTION
MmInitializePageList(PVOID FirstPhysKernelAddress,
                     PVOID LastPhysKernelAddress,
                     ULONG MemorySizeInPages,
                     ULONG LastKernelAddress,
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
   LastKernelAddress = ((ULONG)LastKernelAddress + (Reserved * PAGE_SIZE));
   LastPhysKernelAddress = (PVOID)PAGE_ROUND_UP(LastPhysKernelAddress);
   LastPhysKernelAddress = (char*)LastPhysKernelAddress + (Reserved * PAGE_SIZE);

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
      PVOID Address = (char*)(ULONG)MmPageArray + (i * PAGE_SIZE);
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
            DbgPrint("Unable to create virtual mapping\n");
            KEBUGCHECK(0);
         }
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
                * Page zero is reserved
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
               MmPageArray[j].ReferenceCount = 1;
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
         MmPageArray[i].ReferenceCount = 1;
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
MmSetFlagsPage(PFN_TYPE Pfn, ULONG Flags)
{
   KIRQL oldIrql;

   ASSERT(Pfn < MmPageArraySize);
   KeAcquireSpinLock(&PageListLock, &oldIrql);
   MmPageArray[Pfn].AllFlags = Flags;
   KeReleaseSpinLock(&PageListLock, oldIrql);
}

VOID
MmSetRmapListHeadPage(PFN_TYPE Pfn, struct _MM_RMAP_ENTRY* ListHead)
{
   MmPageArray[Pfn].RmapListHead = ListHead;
}

struct _MM_RMAP_ENTRY*
MmGetRmapListHeadPage(PFN_TYPE Pfn)
{
   return(MmPageArray[Pfn].RmapListHead);
}

VOID
MmMarkPageMapped(PFN_TYPE Pfn)
{
   KIRQL oldIrql;

   if (Pfn < MmPageArraySize)
   {
      KeAcquireSpinLock(&PageListLock, &oldIrql);
      if (MmPageArray[Pfn].Flags.Type == MM_PHYSICAL_PAGE_FREE)
      {
         DbgPrint("Mapping non-used page\n");
         KEBUGCHECK(0);
      }
      MmPageArray[Pfn].MapCount++;
      KeReleaseSpinLock(&PageListLock, oldIrql);
   }
}

VOID
MmMarkPageUnmapped(PFN_TYPE Pfn)
{
   KIRQL oldIrql;

   if (Pfn < MmPageArraySize)
   {
      KeAcquireSpinLock(&PageListLock, &oldIrql);
      if (MmPageArray[Pfn].Flags.Type == MM_PHYSICAL_PAGE_FREE)
      {
         DbgPrint("Unmapping non-used page\n");
         KEBUGCHECK(0);
      }
      if (MmPageArray[Pfn].MapCount == 0)
      {
         DbgPrint("Unmapping not mapped page\n");
         KEBUGCHECK(0);
      }
      MmPageArray[Pfn].MapCount--;
      KeReleaseSpinLock(&PageListLock, oldIrql);
   }
}

ULONG
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
MmSetSavedSwapEntryPage(PFN_TYPE Pfn,  SWAPENTRY SavedSwapEntry)
{
   KIRQL oldIrql;

   ASSERT(Pfn < MmPageArraySize);
   KeAcquireSpinLock(&PageListLock, &oldIrql);
   MmPageArray[Pfn].SavedSwapEntry = SavedSwapEntry;
   KeReleaseSpinLock(&PageListLock, oldIrql);
}

SWAPENTRY
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
MmReferencePage(PFN_TYPE Pfn)
{
   KIRQL oldIrql;

   DPRINT("MmReferencePage(PysicalAddress %x)\n", Pfn << PAGE_SHIFT);

   if (Pfn == 0 || Pfn >= MmPageArraySize)
   {
      KEBUGCHECK(0);
   }

   KeAcquireSpinLock(&PageListLock, &oldIrql);

   if (MmPageArray[Pfn].Flags.Type != MM_PHYSICAL_PAGE_USED)
   {
      DbgPrint("Referencing non-used page\n");
      KEBUGCHECK(0);
   }

   MmPageArray[Pfn].ReferenceCount++;
   KeReleaseSpinLock(&PageListLock, oldIrql);
}

ULONG
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
      DbgPrint("Getting reference count for free page\n");
      KEBUGCHECK(0);
   }

   RCount = MmPageArray[Pfn].ReferenceCount;

   KeReleaseSpinLock(&PageListLock, oldIrql);
   return(RCount);
}

BOOLEAN
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
      DbgPrint("Dereferencing free page\n");
      KEBUGCHECK(0);
   }
   if (MmPageArray[Pfn].ReferenceCount == 0)
   {
      DbgPrint("Derefrencing page with reference count 0\n");
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
         DbgPrint("Freeing page with rmap entries.\n");
         KEBUGCHECK(0);
      }
      if (MmPageArray[Pfn].MapCount != 0)
      {
         DbgPrint("Freeing mapped page (0x%x count %d)\n",
                  Pfn << PAGE_SHIFT, MmPageArray[Pfn].MapCount);
         KEBUGCHECK(0);
      }
      if (MmPageArray[Pfn].LockCount > 0)
      {
         DbgPrint("Freeing locked page\n");
         KEBUGCHECK(0);
      }
      if (MmPageArray[Pfn].SavedSwapEntry != 0)
      {
         DbgPrint("Freeing page with swap entry.\n");
         KEBUGCHECK(0);
      }
      if (MmPageArray[Pfn].Flags.Type != MM_PHYSICAL_PAGE_USED)
      {
         DbgPrint("Freeing page with flags %x\n",
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
      DbgPrint("Getting lock count for free page\n");
      KEBUGCHECK(0);
   }

   LockCount = MmPageArray[Pfn].LockCount;
   KeReleaseSpinLock(&PageListLock, oldIrql);

   return(LockCount);
}

VOID
MmLockPage(PFN_TYPE Pfn)
{
   KIRQL oldIrql;

   DPRINT("MmLockPage(PhysicalAddress %x)\n", Pfn << PAGE_SHIFT);

   if (Pfn == 0 || Pfn >= MmPageArraySize)
   {
      KEBUGCHECK(0);
   }

   KeAcquireSpinLock(&PageListLock, &oldIrql);

   if (MmPageArray[Pfn].Flags.Type != MM_PHYSICAL_PAGE_USED)
   {
      DbgPrint("Locking free page\n");
      KEBUGCHECK(0);
   }

   MmPageArray[Pfn].LockCount++;
   KeReleaseSpinLock(&PageListLock, oldIrql);
}

VOID
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
      DbgPrint("Unlocking free page\n");
      KEBUGCHECK(0);
   }

   MmPageArray[Pfn].LockCount--;
   KeReleaseSpinLock(&PageListLock, oldIrql);
}

PFN_TYPE
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
      DbgPrint("Got non-free page from freelist\n");
      KEBUGCHECK(0);
   }
   if (PageDescriptor->MapCount != 0)
   {
      DbgPrint("Got mapped page from freelist\n");
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
      DbgPrint("Returning mapped page.\n");
      KEBUGCHECK(0);
   }
   return PfnOffset;
}

LONG
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

   LowestPage = LowestAddress.QuadPart / PAGE_SIZE;
   HighestPage = HighestAddress.QuadPart / PAGE_SIZE;
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

NTSTATUS STDCALL
MmZeroPageThreadMain(PVOID Ignored)
{
   NTSTATUS Status;
   KIRQL oldIrql;
   PLIST_ENTRY ListEntry;
   PPHYSICAL_PAGE PageDescriptor;
   PFN_TYPE Pfn;
   static PVOID Address = NULL;
   ULONG Count;

   while(1)
   {
      Status = KeWaitForSingleObject(&ZeroPageThreadEvent,
                                     0,
                                     KernelMode,
                                     FALSE,
                                     NULL);
      if (!NT_SUCCESS(Status))
      {
         DbgPrint("ZeroPageThread: Wait failed\n");
         KEBUGCHECK(0);
         return(STATUS_UNSUCCESSFUL);
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
         Count++;
         Pfn = PageDescriptor - MmPageArray;
         if (Address == NULL)
         {
            Address = ExAllocatePageWithPhysPage(Pfn);
         }
         else
         {
            Status = MmCreateVirtualMapping(NULL,
                                            Address,
                                            PAGE_READWRITE | PAGE_SYSTEM,
                                            &Pfn,
                                            1);
            if (!NT_SUCCESS(Status))
            {
               DbgPrint("Unable to create virtual mapping\n");
               KEBUGCHECK(0);
            }
         }
         memset(Address, 0, PAGE_SIZE);
         MmDeleteVirtualMapping(NULL, (PVOID)Address, FALSE, NULL, NULL);
         KeAcquireSpinLock(&PageListLock, &oldIrql);
         if (PageDescriptor->MapCount != 0)
         {
            DbgPrint("Mapped page on freelist.\n");
            KEBUGCHECK(0);
         }
	 PageDescriptor->Flags.Zero = 1;
         PageDescriptor->Flags.Type = MM_PHYSICAL_PAGE_FREE;
         InsertHeadList(&FreeZeroedPageListHead, ListEntry);
      }
      DPRINT("Zeroed %d pages.\n", Count);
      KeResetEvent(&ZeroPageThreadEvent);
      KeReleaseSpinLock(&PageListLock, oldIrql);
   }
}

NTSTATUS INIT_FUNCTION
MmInitZeroPageThread(VOID)
{
   KPRIORITY Priority;
   NTSTATUS Status;

   Status = PsCreateSystemThread(&ZeroPageThreadHandle,
                                 THREAD_ALL_ACCESS,
                                 NULL,
                                 NULL,
                                 &ZeroPageThreadId,
                                 (PKSTART_ROUTINE) MmZeroPageThreadMain,
                                 NULL);
   if (!NT_SUCCESS(Status))
   {
      return(Status);
   }

   Priority = 1;
   NtSetInformationThread(ZeroPageThreadHandle,
                          ThreadPriority,
                          &Priority,
                          sizeof(Priority));

   return(STATUS_SUCCESS);
}

/* EOF */
