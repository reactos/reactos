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

#include <ddk/ntddk.h>
#include <internal/mm.h>
#include <internal/ntoskrnl.h>

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
      ULONG Type:2;
      ULONG Consumer:3;
    }Flags;
    ULONG AllFlags;
  };

  LIST_ENTRY ListEntry;
  ULONG ReferenceCount;
  SWAPENTRY SavedSwapEntry;
  ULONG LockCount;
  ULONG MapCount;
  struct _MM_RMAP_ENTRY* RmapListHead;
} PHYSICAL_PAGE, *PPHYSICAL_PAGE;

/* GLOBALS ****************************************************************/

static PPHYSICAL_PAGE MmPageArray;
static ULONG MmPageArraySize;

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
MmTransferOwnershipPage(PHYSICAL_ADDRESS PhysicalAddress, ULONG NewConsumer)
{
  ULONG Start = PhysicalAddress.u.LowPart / PAGE_SIZE;
  KIRQL oldIrql;
   
  KeAcquireSpinLock(&PageListLock, &oldIrql);
  if (MmPageArray[Start].MapCount != 0)
    {
      DbgPrint("Transfering mapped page.\n");
      KeBugCheck(0);
    }
  RemoveEntryList(&MmPageArray[Start].ListEntry);
  InsertTailList(&UsedPageListHeads[NewConsumer], 
    &MmPageArray[Start].ListEntry);
  MmPageArray[Start].Flags.Consumer = NewConsumer;
  KeReleaseSpinLock(&PageListLock, oldIrql);  
  MiZeroPage(PhysicalAddress);
}

PHYSICAL_ADDRESS
MmGetLRUFirstUserPage(VOID)
{
  PLIST_ENTRY NextListEntry;
  PHYSICAL_ADDRESS Next;
  PHYSICAL_PAGE* PageDescriptor;
  KIRQL oldIrql;

  KeAcquireSpinLock(&PageListLock, &oldIrql);
  NextListEntry = UsedPageListHeads[MC_USER].Flink;
  if (NextListEntry == &UsedPageListHeads[MC_USER])
    {
      KeReleaseSpinLock(&PageListLock, oldIrql);
      return((LARGE_INTEGER)0LL);
    }
  PageDescriptor = CONTAINING_RECORD(NextListEntry, PHYSICAL_PAGE, ListEntry);
  Next.QuadPart = (ULONG)((ULONG)PageDescriptor - (ULONG)MmPageArray);
  Next.QuadPart = (Next.QuadPart / sizeof(PHYSICAL_PAGE)) * PAGE_SIZE;   
  KeReleaseSpinLock(&PageListLock, oldIrql);
  return(Next);
}

VOID
MmSetLRULastPage(PHYSICAL_ADDRESS PhysicalAddress)
{
  ULONG Start = PhysicalAddress.u.LowPart / PAGE_SIZE;
  KIRQL oldIrql;

  KeAcquireSpinLock(&PageListLock, &oldIrql);
  if (MmPageArray[Start].Flags.Type == MM_PHYSICAL_PAGE_USED &&
      MmPageArray[Start].Flags.Consumer == MC_USER)
    {
      RemoveEntryList(&MmPageArray[Start].ListEntry);
      InsertTailList(&UsedPageListHeads[MC_USER], 
		     &MmPageArray[Start].ListEntry);
    }
  KeReleaseSpinLock(&PageListLock, oldIrql);
}

PHYSICAL_ADDRESS
MmGetLRUNextUserPage(PHYSICAL_ADDRESS PreviousPhysicalAddress)
{
  ULONG Start = PreviousPhysicalAddress.u.LowPart / PAGE_SIZE;
  PLIST_ENTRY NextListEntry;
  PHYSICAL_ADDRESS Next;
  PHYSICAL_PAGE* PageDescriptor;
  KIRQL oldIrql;

  KeAcquireSpinLock(&PageListLock, &oldIrql);
  if (MmPageArray[Start].Flags.Type != MM_PHYSICAL_PAGE_USED || 
      MmPageArray[Start].Flags.Consumer != MC_USER)
    {
      NextListEntry = UsedPageListHeads[MC_USER].Flink;
    }
  else
    {
      NextListEntry = MmPageArray[Start].ListEntry.Flink;
    }
  if (NextListEntry == &UsedPageListHeads[MC_USER])
    {
      KeReleaseSpinLock(&PageListLock, oldIrql);
      return((LARGE_INTEGER)0LL);
    }
  PageDescriptor = CONTAINING_RECORD(NextListEntry, PHYSICAL_PAGE, ListEntry);
  Next.QuadPart = (ULONG)((ULONG)PageDescriptor - (ULONG)MmPageArray);
  Next.QuadPart = (Next.QuadPart / sizeof(PHYSICAL_PAGE)) * PAGE_SIZE;   
  KeReleaseSpinLock(&PageListLock, oldIrql);
  return(Next);
}

PHYSICAL_ADDRESS
MmGetContinuousPages(ULONG NumberOfBytes,
		     PHYSICAL_ADDRESS HighestAcceptableAddress,
		     ULONG Alignment)
{
   ULONG NrPages;
   ULONG i;
   LONG start;
   ULONG length;
   KIRQL oldIrql;
   
   NrPages = PAGE_ROUND_UP(NumberOfBytes) / PAGE_SIZE;
   
   KeAcquireSpinLock(&PageListLock, &oldIrql);
   
   start = -1;
   length = 0;
   for (i = 0; i < (HighestAcceptableAddress.QuadPart / PAGE_SIZE); )
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
	return((LARGE_INTEGER)(LONGLONG)0);
     }  
   for (i = start; i < (start + length); i++)
     {
	RemoveEntryList(&MmPageArray[i].ListEntry);
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
   return((LARGE_INTEGER)((LONGLONG)start * PAGE_SIZE));
}

VOID 
MiParseRangeToFreeList(PADDRESS_RANGE Range)
{
  ULONG i, first, last;

  /* FIXME: Not 64-bit ready */
  
  DPRINT("Range going to free list (Base 0x%X, Length 0x%X, Type 0x%X)\n",
	 Range->BaseAddrLow,
	 Range->LengthLow,
	 Range->Type);
  
  first = (Range->BaseAddrLow + PAGE_SIZE - 1) / PAGE_SIZE;
  last = first + ((Range->LengthLow + PAGE_SIZE - 1) / PAGE_SIZE);
  for (i = first; i < last; i++)
    {
      if (MmPageArray[i].Flags.Type == 0)
        {
          MmPageArray[i].Flags.Type = MM_PHYSICAL_PAGE_FREE;
	  MmPageArray[i].ReferenceCount = 0;
	  InsertTailList(&FreeUnzeroedPageListHead,
			 &MmPageArray[i].ListEntry);
	  UnzeroedPageCount++;
        }
    }
}

VOID
MiParseRangeToBiosList(PADDRESS_RANGE Range)
{
  ULONG i, first, last;
  
  /* FIXME: Not 64-bit ready */
  
  DPRINT("Range going to bios list (Base 0x%X, Length 0x%X, Type 0x%X)\n",
	 Range->BaseAddrLow,
	 Range->LengthLow,
	 Range->Type);
  
  first = (Range->BaseAddrLow + PAGE_SIZE - 1) / PAGE_SIZE;
  last = first + ((Range->LengthLow + PAGE_SIZE - 1) / PAGE_SIZE);
  for (i = first; i < last; i++)
    {
      /* Remove the page from the free list if it is there */
      if (MmPageArray[i].Flags.Type == MM_PHYSICAL_PAGE_FREE)
        {
          RemoveEntryList(&MmPageArray[i].ListEntry);
        }
      
      if (MmPageArray[i].Flags.Type != MM_PHYSICAL_PAGE_BIOS)
        {
          MmPageArray[i].Flags.Type = MM_PHYSICAL_PAGE_BIOS;
	  MmPageArray[i].Flags.Consumer = MC_NPPOOL;
	  MmPageArray[i].ReferenceCount = 1;
	  InsertTailList(&BiosPageListHead,
			 &MmPageArray[i].ListEntry);
        }
    }
}

VOID 
MiParseBIOSMemoryMap(ULONG MemorySizeInPages,
		     PADDRESS_RANGE BIOSMemoryMap,
		     ULONG AddressRangeCount)
{
  PADDRESS_RANGE p;
  ULONG i;
  
  p = BIOSMemoryMap;
  for (i = 0; i < AddressRangeCount; i++)
    {
      if (((p->BaseAddrLow + PAGE_SIZE - 1) / PAGE_SIZE) < MemorySizeInPages)
        {
          if (p->Type == 1)
            {
              MiParseRangeToFreeList(p);
            }
          else
            {
              MiParseRangeToBiosList(p);
            }
        }
      p += 1;
    }
}

PVOID 
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
 *         PageBuffer = Page sized buffer
 *         FirstKernelAddress = First physical address used by the kernel
 *         LastKernelAddress = Last physical address used by the kernel
 */
{
   ULONG i;
   ULONG Reserved;
   NTSTATUS Status;
   
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
   LastPhysKernelAddress = LastPhysKernelAddress + (Reserved * PAGE_SIZE);
     
   MmStats.NrTotalPages = 0;
   MmStats.NrSystemPages = 0;
   MmStats.NrUserPages = 0;
   MmStats.NrReservedPages = 0;
   MmStats.NrFreePages = 0;
   MmStats.NrLockedPages = 0;
   
   for (i = 0; i < Reserved; i++)
     {
       PVOID Address = (PVOID)(ULONG)MmPageArray + (i * PAGE_SIZE);
       if (!MmIsPagePresent(NULL, Address))
	 {
	   ULONG PhysicalAddress;
	   PhysicalAddress = (ULONG)LastPhysKernelAddress - 
	     (Reserved * PAGE_SIZE) + (i * PAGE_SIZE);
	   Status = 
	     MmCreateVirtualMappingUnsafe(NULL,
					  Address,
					  PAGE_READWRITE,
					  (PHYSICAL_ADDRESS)(LONGLONG)PhysicalAddress,
					  FALSE);
	   if (!NT_SUCCESS(Status))
	     {
	       DbgPrint("Unable to create virtual mapping\n");
	       KeBugCheck(0);
	     }
	 }
       memset((PVOID)MmPageArray + (i * PAGE_SIZE), 0, PAGE_SIZE);
     }
   

   /*
    * Page zero is reserved
    */
   MmPageArray[0].Flags.Type = MM_PHYSICAL_PAGE_BIOS;
   MmPageArray[0].Flags.Consumer = MC_NPPOOL;
   MmPageArray[0].ReferenceCount = 0;
   InsertTailList(&BiosPageListHead,
		  &MmPageArray[0].ListEntry); 

   /*
    * Page one is reserved for the initial KPCR
    */
   MmPageArray[1].Flags.Type = MM_PHYSICAL_PAGE_BIOS;
   MmPageArray[1].Flags.Consumer = MC_NPPOOL;
   MmPageArray[1].ReferenceCount = 0;
   InsertTailList(&BiosPageListHead,
      &MmPageArray[1].ListEntry); 

   i = 2;
   if ((ULONG)FirstPhysKernelAddress < 0xa0000)
     {
	MmStats.NrFreePages += (((ULONG)FirstPhysKernelAddress/PAGE_SIZE) - 2);
	for (; i<((ULONG)FirstPhysKernelAddress/PAGE_SIZE); i++)
	  {
	     MmPageArray[i].Flags.Type = MM_PHYSICAL_PAGE_FREE;
	     MmPageArray[i].ReferenceCount = 0;
	     InsertTailList(&FreeUnzeroedPageListHead,
			    &MmPageArray[i].ListEntry);
	     UnzeroedPageCount++;
	  }
	MmStats.NrSystemPages += 
	  ((((ULONG)LastPhysKernelAddress) / PAGE_SIZE) - i);
	for (; i<((ULONG)LastPhysKernelAddress / PAGE_SIZE); i++)
	  {
	     MmPageArray[i].Flags.Type = MM_PHYSICAL_PAGE_USED;
	     MmPageArray[i].Flags.Consumer = MC_NPPOOL;
	     MmPageArray[i].ReferenceCount = 1;
	     MmPageArray[i].MapCount = 1;
	     InsertTailList(&UsedPageListHeads[MC_NPPOOL],
			    &MmPageArray[i].ListEntry);
	  }
	MmStats.NrFreePages += ((0xa0000/PAGE_SIZE) - i);
	for (; i<(0xa0000/PAGE_SIZE); i++)
	  {
	     MmPageArray[i].Flags.Type = MM_PHYSICAL_PAGE_FREE;
	     MmPageArray[i].ReferenceCount = 0;
	     InsertTailList(&FreeUnzeroedPageListHead,
			    &MmPageArray[i].ListEntry);
	     UnzeroedPageCount++;
	  }
	MmStats.NrReservedPages += ((0x100000/PAGE_SIZE) - i);
	for (; i<(0x100000 / PAGE_SIZE); i++)
	  {
	     MmPageArray[i].Flags.Type = MM_PHYSICAL_PAGE_BIOS;
	     MmPageArray[i].Flags.Consumer = MC_NPPOOL;
	     MmPageArray[i].ReferenceCount = 1;
	     InsertTailList(&BiosPageListHead,
			    &MmPageArray[i].ListEntry);
	  }
     }
   else
     {
	MmStats.NrFreePages += ((0xa0000 / PAGE_SIZE) - 2);	  
	for (; i<(0xa0000 / PAGE_SIZE); i++)
	  {
	     MmPageArray[i].Flags.Type = MM_PHYSICAL_PAGE_FREE;
	     MmPageArray[i].ReferenceCount = 0;
	     InsertTailList(&FreeUnzeroedPageListHead,
			    &MmPageArray[i].ListEntry);
	     UnzeroedPageCount++;
	  }
	MmStats.NrReservedPages += (0x60000 / PAGE_SIZE);
	for (; i<(0x100000 / PAGE_SIZE); i++)
	  {
	     MmPageArray[i].Flags.Type = MM_PHYSICAL_PAGE_BIOS;
	     MmPageArray[i].Flags.Consumer = MC_NPPOOL;
	     MmPageArray[i].ReferenceCount = 1;
	     InsertTailList(&BiosPageListHead,
			    &MmPageArray[i].ListEntry);
	  }
	MmStats.NrFreePages += (((ULONG)FirstPhysKernelAddress/PAGE_SIZE) - i);
	for (; i<((ULONG)FirstPhysKernelAddress/PAGE_SIZE); i++)
	  {
	     MmPageArray[i].Flags.Type = MM_PHYSICAL_PAGE_FREE;
	     MmPageArray[i].ReferenceCount = 0;
	     InsertTailList(&FreeUnzeroedPageListHead,
			    &MmPageArray[i].ListEntry);
	     UnzeroedPageCount++;
	  }
	MmStats.NrSystemPages += 
	  (((ULONG)LastPhysKernelAddress/PAGE_SIZE) - i);
	for (; i<((ULONG)LastPhysKernelAddress/PAGE_SIZE); i++)
	  {
	     MmPageArray[i].Flags.Type = MM_PHYSICAL_PAGE_USED;
	     MmPageArray[i].Flags.Consumer = MC_NPPOOL;
	     MmPageArray[i].ReferenceCount = 1;
	     MmPageArray[i].MapCount = 1;
	     InsertTailList(&UsedPageListHeads[MC_NPPOOL],
			    &MmPageArray[i].ListEntry);
	  }
     }
   
   MmStats.NrFreePages += (MemorySizeInPages - i);
   for (; i<MemorySizeInPages; i++)
     {
	MmPageArray[i].Flags.Type = MM_PHYSICAL_PAGE_FREE;
	MmPageArray[i].ReferenceCount = 0;
	InsertTailList(&FreeUnzeroedPageListHead,
		       &MmPageArray[i].ListEntry);
	UnzeroedPageCount++;
     }

  if ((BIOSMemoryMap != NULL) && (AddressRangeCount > 0))
    {
      MiParseBIOSMemoryMap(
        MemorySizeInPages,
        BIOSMemoryMap,
        AddressRangeCount);
    }

   KeInitializeEvent(&ZeroPageThreadEvent, NotificationEvent, TRUE);

  
   MmStats.NrTotalPages = MmStats.NrFreePages + MmStats.NrSystemPages +
     MmStats.NrReservedPages + MmStats.NrUserPages;
   MmInitializeBalancer(MmStats.NrFreePages);
   return((PVOID)LastKernelAddress);
}

VOID 
MmSetFlagsPage(PHYSICAL_ADDRESS PhysicalAddress, ULONG Flags)
{
   ULONG Start = PhysicalAddress.u.LowPart / PAGE_SIZE;
   KIRQL oldIrql;
   
   KeAcquireSpinLock(&PageListLock, &oldIrql);
   MmPageArray[Start].AllFlags = Flags;
   KeReleaseSpinLock(&PageListLock, oldIrql);
}

VOID 
MmSetRmapListHeadPage(PHYSICAL_ADDRESS PhysicalAddress, 
		      struct _MM_RMAP_ENTRY* ListHead)
{
   ULONG Start = PhysicalAddress.u.LowPart / PAGE_SIZE;

   MmPageArray[Start].RmapListHead = ListHead;
}

struct _MM_RMAP_ENTRY*
MmGetRmapListHeadPage(PHYSICAL_ADDRESS PhysicalAddress)
{
  ULONG Start = PhysicalAddress.u.LowPart / PAGE_SIZE;

  return(MmPageArray[Start].RmapListHead);
}

VOID 
MmMarkPageMapped(PHYSICAL_ADDRESS PhysicalAddress)
{
  ULONG Start = PhysicalAddress.u.LowPart / PAGE_SIZE;
  KIRQL oldIrql;

  if (Start < MmPageArraySize)
    {   
      KeAcquireSpinLock(&PageListLock, &oldIrql);
      MmPageArray[Start].MapCount++;
      KeReleaseSpinLock(&PageListLock, oldIrql);
    }
}

VOID 
MmMarkPageUnmapped(PHYSICAL_ADDRESS PhysicalAddress)
{
  ULONG Start = PhysicalAddress.u.LowPart / PAGE_SIZE;
  KIRQL oldIrql;
   
  if (Start < MmPageArraySize)
    {   
      KeAcquireSpinLock(&PageListLock, &oldIrql);
      MmPageArray[Start].MapCount--;
      KeReleaseSpinLock(&PageListLock, oldIrql);
    }
}

ULONG 
MmGetFlagsPage(PHYSICAL_ADDRESS PhysicalAddress)
{
   ULONG Start = PhysicalAddress.u.LowPart / PAGE_SIZE;
   KIRQL oldIrql;
   ULONG Flags;
   
   KeAcquireSpinLock(&PageListLock, &oldIrql);
   Flags = MmPageArray[Start].AllFlags;
   KeReleaseSpinLock(&PageListLock, oldIrql);
   
   return(Flags);
}


VOID 
MmSetSavedSwapEntryPage(PHYSICAL_ADDRESS PhysicalAddress,
			SWAPENTRY SavedSwapEntry)
{
   ULONG Start = PhysicalAddress.u.LowPart / PAGE_SIZE;
   KIRQL oldIrql;

   KeAcquireSpinLock(&PageListLock, &oldIrql);
   MmPageArray[Start].SavedSwapEntry = SavedSwapEntry;
   KeReleaseSpinLock(&PageListLock, oldIrql);
}

SWAPENTRY 
MmGetSavedSwapEntryPage(PHYSICAL_ADDRESS PhysicalAddress)
{
   ULONG Start = PhysicalAddress.u.LowPart / PAGE_SIZE;
   SWAPENTRY SavedSwapEntry;
   KIRQL oldIrql;
   
   KeAcquireSpinLock(&PageListLock, &oldIrql);
   SavedSwapEntry = MmPageArray[Start].SavedSwapEntry;
   KeReleaseSpinLock(&PageListLock, oldIrql);

   return(SavedSwapEntry);
}

VOID 
MmReferencePage(PHYSICAL_ADDRESS PhysicalAddress)
{
   ULONG Start = PhysicalAddress.u.LowPart / PAGE_SIZE;
   KIRQL oldIrql;
   
   DPRINT("MmReferencePage(PhysicalAddress %x)\n", PhysicalAddress);
   
   if (PhysicalAddress.u.LowPart == 0)
     {
	KeBugCheck(0);
     }
   
   KeAcquireSpinLock(&PageListLock, &oldIrql);
   
   if (MmPageArray[Start].Flags.Type != MM_PHYSICAL_PAGE_USED)
     {
	DbgPrint("Referencing non-used page\n");
	KeBugCheck(0);
     }
   
   MmPageArray[Start].ReferenceCount++;
   KeReleaseSpinLock(&PageListLock, oldIrql);
}

ULONG
MmGetReferenceCountPage(PHYSICAL_ADDRESS PhysicalAddress)
{
   ULONG Start = PhysicalAddress.u.LowPart / PAGE_SIZE;
   KIRQL oldIrql;
   ULONG RCount;

   DPRINT("MmGetReferenceCountPage(PhysicalAddress %x)\n", PhysicalAddress);

   if (PhysicalAddress.u.LowPart == 0)
     {
	KeBugCheck(0);
     }
   
   KeAcquireSpinLock(&PageListLock, &oldIrql);
   
   if (MmPageArray[Start].Flags.Type != MM_PHYSICAL_PAGE_USED)
     {
	DbgPrint("Getting reference count for free page\n");
	KeBugCheck(0);
     }
   
   RCount = MmPageArray[Start].ReferenceCount;

   KeReleaseSpinLock(&PageListLock, oldIrql);
   return(RCount);
}

BOOLEAN
MmIsUsablePage(PHYSICAL_ADDRESS PhysicalAddress)
{
   ULONG Start = PhysicalAddress.u.LowPart / PAGE_SIZE;

   DPRINT("MmGetReferenceCountPage(PhysicalAddress %x)\n", PhysicalAddress);

   if (PhysicalAddress.u.LowPart == 0)
     {
	KeBugCheck(0);
     }

   if (MmPageArray[Start].Flags.Type != MM_PHYSICAL_PAGE_USED &&
       MmPageArray[Start].Flags.Type != MM_PHYSICAL_PAGE_BIOS)
     {
       return(FALSE);
     }
   
   return(TRUE);
}

VOID 
MmDereferencePage(PHYSICAL_ADDRESS PhysicalAddress)
{
   ULONG Start = PhysicalAddress.u.LowPart / PAGE_SIZE;
   KIRQL oldIrql;
   
   DPRINT("MmDereferencePage(PhysicalAddress %I64x)\n", PhysicalAddress);

   if (PhysicalAddress.u.LowPart == 0)
     {
	KeBugCheck(0);
     }
   
   KeAcquireSpinLock(&PageListLock, &oldIrql);
  

   if (MmPageArray[Start].Flags.Type != MM_PHYSICAL_PAGE_USED)
     {
	DbgPrint("Dereferencing free page\n");
	KeBugCheck(0);
     }
  
   MmPageArray[Start].ReferenceCount--;
   if (MmPageArray[Start].ReferenceCount == 0)
     {
       MmStats.NrFreePages++;
       MmStats.NrSystemPages--;
       RemoveEntryList(&MmPageArray[Start].ListEntry);
       if (MmPageArray[Start].RmapListHead != NULL)
	 {
	   DbgPrint("Freeing page with rmap entries.\n");
	   KeBugCheck(0);
	 }
       if (MmPageArray[Start].MapCount != 0)
	 {
	   DbgPrint("Freeing mapped page (0x%I64x count %d)\n",
		    PhysicalAddress, MmPageArray[Start].MapCount);
	   KeBugCheck(0);
	 }
       if (MmPageArray[Start].LockCount > 0)
	 {
	   DbgPrint("Freeing locked page\n");
	   KeBugCheck(0);
	 }
       if (MmPageArray[Start].SavedSwapEntry != 0)
	 {
	   DbgPrint("Freeing page with swap entry.\n");
	   KeBugCheck(0);
	 }
       if (MmPageArray[Start].Flags.Type != MM_PHYSICAL_PAGE_USED)
	 {
	   DbgPrint("Freeing page with flags %x\n",
		    MmPageArray[Start].Flags.Type);
	   KeBugCheck(0);
	 }
       MmPageArray[Start].Flags.Type = MM_PHYSICAL_PAGE_FREE;
       InsertTailList(&FreeUnzeroedPageListHead, 
		      &MmPageArray[Start].ListEntry);
       UnzeroedPageCount++;
       if (UnzeroedPageCount > 8 && 0 == KeReadStateEvent(&ZeroPageThreadEvent))
         {
           KeSetEvent(&ZeroPageThreadEvent, IO_NO_INCREMENT, FALSE);
         }
     }
   KeReleaseSpinLock(&PageListLock, oldIrql);
}

ULONG 
MmGetLockCountPage(PHYSICAL_ADDRESS PhysicalAddress)
{
   ULONG Start = PhysicalAddress.u.LowPart / PAGE_SIZE;
   KIRQL oldIrql;
   ULONG LockCount;
   
   DPRINT("MmGetLockCountPage(PhysicalAddress %x)\n", PhysicalAddress);
   
   if (PhysicalAddress.u.LowPart == 0)
     {
	KeBugCheck(0);
     }
   
   KeAcquireSpinLock(&PageListLock, &oldIrql);
   
   if (MmPageArray[Start].Flags.Type != MM_PHYSICAL_PAGE_USED)
     {
	DbgPrint("Getting lock count for free page\n");
	KeBugCheck(0);
     }
   
   LockCount = MmPageArray[Start].LockCount;
   KeReleaseSpinLock(&PageListLock, oldIrql);

   return(LockCount);
}

VOID 
MmLockPage(PHYSICAL_ADDRESS PhysicalAddress)
{
   ULONG Start = PhysicalAddress.u.LowPart / PAGE_SIZE;
   KIRQL oldIrql;
   
   DPRINT("MmLockPage(PhysicalAddress %x)\n", PhysicalAddress);
   
   if (PhysicalAddress.u.LowPart == 0)
     {
	KeBugCheck(0);
     }
   
   KeAcquireSpinLock(&PageListLock, &oldIrql);
   
   if (MmPageArray[Start].Flags.Type != MM_PHYSICAL_PAGE_USED)
     {
	DbgPrint("Locking free page\n");
	KeBugCheck(0);
     }
   
   MmPageArray[Start].LockCount++;
   KeReleaseSpinLock(&PageListLock, oldIrql);
}

VOID 
MmUnlockPage(PHYSICAL_ADDRESS PhysicalAddress)
{
   ULONG Start = PhysicalAddress.u.LowPart / PAGE_SIZE;
   KIRQL oldIrql;
   
   DPRINT("MmUnlockPage(PhysicalAddress %llx)\n", PhysicalAddress);
   
   if (PhysicalAddress.u.LowPart == 0)
     {
	KeBugCheck(0);
     }
   
   KeAcquireSpinLock(&PageListLock, &oldIrql);
   
   if (MmPageArray[Start].Flags.Type != MM_PHYSICAL_PAGE_USED)
     {
	DbgPrint("Unlocking free page\n");
	KeBugCheck(0);
     }
   
   MmPageArray[Start].LockCount--;
   KeReleaseSpinLock(&PageListLock, oldIrql);
}

PHYSICAL_ADDRESS
MmAllocPage(ULONG Consumer, SWAPENTRY SavedSwapEntry)
{
  PHYSICAL_ADDRESS PageOffset;
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
	  return((PHYSICAL_ADDRESS)0LL);
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
      KeBugCheck(0);
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

  PageOffset.QuadPart = (ULONG)((ULONG)PageDescriptor - (ULONG)MmPageArray);
  PageOffset.QuadPart = 
    (PageOffset.QuadPart / sizeof(PHYSICAL_PAGE)) * PAGE_SIZE;
  if (NeedClear)
    {
      MiZeroPage(PageOffset);
    }
  return(PageOffset);
}


NTSTATUS STDCALL
MmZeroPageThreadMain(PVOID Ignored)
{
  NTSTATUS Status;
  KIRQL oldIrql;
  PLIST_ENTRY ListEntry;
  PPHYSICAL_PAGE PageDescriptor;
  PHYSICAL_ADDRESS PhysPage;
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
	  KeBugCheck(0);
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
         PhysPage.QuadPart = (ULONG)((ULONG)PageDescriptor - (ULONG)MmPageArray);
         PhysPage.QuadPart = (PhysPage.QuadPart / sizeof(PHYSICAL_PAGE)) * PAGE_SIZE;
	 if (Address == NULL)
	 {
	    Address = ExAllocatePageWithPhysPage(PhysPage);
	 }
	 else
	 {
            Status = MmCreateVirtualMapping(NULL, 
	                                    Address, 
				            PAGE_READWRITE | PAGE_SYSTEM, 
				            PhysPage,
				            FALSE);
            if (!NT_SUCCESS(Status))
            {
               DbgPrint("Unable to create virtual mapping\n");
	       KeBugCheck(0);
            }
	 }
         memset(Address, 0, PAGE_SIZE);
         MmDeleteVirtualMapping(NULL, (PVOID)Address, FALSE, NULL, NULL);
         KeAcquireSpinLock(&PageListLock, &oldIrql);
 	 PageDescriptor->Flags.Type = MM_PHYSICAL_PAGE_FREE;
	 InsertHeadList(&FreeZeroedPageListHead, ListEntry);
      }
      DPRINT("Zeroed %d pages.\n", Count);
      KeResetEvent(&ZeroPageThreadEvent);
      KeReleaseSpinLock(&PageListLock, oldIrql);
    }
}
 
NTSTATUS MmInitZeroPageThread(VOID)
{
  KPRIORITY Priority;
  NTSTATUS Status;
  
  Status = PsCreateSystemThread(&ZeroPageThreadHandle,
				THREAD_ALL_ACCESS,
				NULL,
				NULL,
				&ZeroPageThreadId,
				MmZeroPageThreadMain,
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
