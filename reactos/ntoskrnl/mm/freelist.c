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

#define MM_PTYPE(x)              ((x) & 0x3)

typedef struct _PHYSICAL_PAGE
{
  ULONG Flags;
  LIST_ENTRY ListEntry;
  ULONG ReferenceCount;
  SWAPENTRY SavedSwapEntry;
  ULONG LockCount;
  ULONG MapCount;
  struct _MM_RMAP_ENTRY* RmapListHead;
} PHYSICAL_PAGE, *PPHYSICAL_PAGE;

/* GLOBALS ****************************************************************/

static PPHYSICAL_PAGE MmPageArray;

static KSPIN_LOCK PageListLock;
static LIST_ENTRY UsedPageListHeads[MC_MAXIMUM];
static LIST_ENTRY FreeZeroedPageListHead;
static LIST_ENTRY FreeUnzeroedPageListHead;
static LIST_ENTRY BiosPageListHead;

NTSTATUS 
MmCreateVirtualMappingUnsafe(struct _EPROCESS* Process,
			     PVOID Address, 
			     ULONG flProtect,
			     ULONG PhysicalAddress);

/* FUNCTIONS *************************************************************/

PVOID
MmGetLRUFirstUserPage(VOID)
{
  PLIST_ENTRY NextListEntry;
  ULONG Next;
  PHYSICAL_PAGE* PageDescriptor;
  KIRQL oldIrql;

  KeAcquireSpinLock(&PageListLock, &oldIrql);
  NextListEntry = UsedPageListHeads[MC_USER].Flink;
  if (NextListEntry == &UsedPageListHeads[MC_USER])
    {
      KeReleaseSpinLock(&PageListLock, oldIrql);
      return(NULL);
    }
  PageDescriptor = CONTAINING_RECORD(NextListEntry, PHYSICAL_PAGE, ListEntry);
  Next = (ULONG)((ULONG)PageDescriptor - (ULONG)MmPageArray);
  Next = (Next / sizeof(PHYSICAL_PAGE)) * PAGESIZE;   
  KeReleaseSpinLock(&PageListLock, oldIrql);
  return((PVOID)Next);
}

PVOID
MmGetLRUNextUserPage(PVOID PreviousPhysicalAddress)
{
  ULONG Start = (ULONG)PreviousPhysicalAddress / PAGESIZE;
  PLIST_ENTRY NextListEntry;
  ULONG Next;
  PHYSICAL_PAGE* PageDescriptor;
  KIRQL oldIrql;

  KeAcquireSpinLock(&PageListLock, &oldIrql);
  if (!(MmPageArray[Start].Flags & MM_PHYSICAL_PAGE_USED))
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
      return(NULL);
    }
  PageDescriptor = CONTAINING_RECORD(NextListEntry, PHYSICAL_PAGE, ListEntry);
  Next = (ULONG)((ULONG)PageDescriptor - (ULONG)MmPageArray);
  Next = (Next / sizeof(PHYSICAL_PAGE)) * PAGESIZE;   
  KeReleaseSpinLock(&PageListLock, oldIrql);
  return((PVOID)Next);
}

PVOID
MmGetContinuousPages(ULONG NumberOfBytes,
		     PHYSICAL_ADDRESS HighestAcceptableAddress,
		     ULONG Alignment)
{
   ULONG NrPages;
   ULONG i;
   ULONG start;
   ULONG length;
   KIRQL oldIrql;
   
   NrPages = PAGE_ROUND_UP(NumberOfBytes) / PAGESIZE;
   
   KeAcquireSpinLock(&PageListLock, &oldIrql);
   
   start = -1;
   length = 0;
   for (i = 0; i < (HighestAcceptableAddress.QuadPart / PAGESIZE); )
     {
	if (MM_PTYPE(MmPageArray[i].Flags) ==  MM_PHYSICAL_PAGE_FREE)
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
	     i = ROUND_UP((i + 1), (Alignment / PAGESIZE));
	  }
     }
   if (start == -1 || length != NrPages)
     {
	KeReleaseSpinLock(&PageListLock, oldIrql);
	return(NULL);
     }  
   for (i = start; i < (start + length); i++)
     {
	RemoveEntryList(&MmPageArray[i].ListEntry);
	MmPageArray[i].Flags = MM_PHYSICAL_PAGE_USED;
	MmPageArray[i].ReferenceCount = 1;
	MmPageArray[i].LockCount = 0;
	MmPageArray[i].MapCount = 0;
	MmPageArray[i].SavedSwapEntry = 0;
	InsertTailList(&UsedPageListHeads[MC_NPPOOL], &MmPageArray[i].ListEntry);
     }
   KeReleaseSpinLock(&PageListLock, oldIrql);
   return((PVOID)(start * 4096));
}

VOID MiParseRangeToFreeList(
  PADDRESS_RANGE Range)
{
  ULONG i, first, last;

  /* FIXME: Not 64-bit ready */

  DPRINT("Range going to free list (Base 0x%X, Length 0x%X, Type 0x%X)\n",
    Range->BaseAddrLow,
    Range->LengthLow,
    Range->Type);

  first = (Range->BaseAddrLow + PAGESIZE - 1) / PAGESIZE;
  last = first + ((Range->LengthLow + PAGESIZE - 1) / PAGESIZE);
  for (i = first; i < last; i++)
    {
      if (MmPageArray[i].Flags == 0)
        {
          MmPageArray[i].Flags = MM_PHYSICAL_PAGE_FREE;
	        MmPageArray[i].ReferenceCount = 0;
	        InsertTailList(&FreeUnzeroedPageListHead,
		        &MmPageArray[i].ListEntry);
        }
    }
}

VOID MiParseRangeToBiosList(
  PADDRESS_RANGE Range)
{
  ULONG i, first, last;

  /* FIXME: Not 64-bit ready */

  DPRINT("Range going to bios list (Base 0x%X, Length 0x%X, Type 0x%X)\n",
    Range->BaseAddrLow,
    Range->LengthLow,
    Range->Type);

  first = (Range->BaseAddrLow + PAGESIZE - 1) / PAGESIZE;
  last = first + ((Range->LengthLow + PAGESIZE - 1) / PAGESIZE);
  for (i = first; i < last; i++)
    {
      /* Remove the page from the free list if it is there */
      if (MmPageArray[i].Flags == MM_PHYSICAL_PAGE_FREE)
        {
          RemoveEntryList(&MmPageArray[i].ListEntry);
        }

      if (MmPageArray[i].Flags != MM_PHYSICAL_PAGE_BIOS)
        {
          MmPageArray[i].Flags = MM_PHYSICAL_PAGE_BIOS;
	        MmPageArray[i].ReferenceCount = 1;
	        InsertTailList(&BiosPageListHead,
		        &MmPageArray[i].ListEntry);
        }
    }
}

VOID MiParseBIOSMemoryMap(
  ULONG MemorySizeInPages,
  PADDRESS_RANGE BIOSMemoryMap,
  ULONG AddressRangeCount)
{
  PADDRESS_RANGE p;
  ULONG i;

  p = BIOSMemoryMap;
  for (i = 0; i < AddressRangeCount; i++)
    {
      if (((p->BaseAddrLow + PAGESIZE - 1) / PAGESIZE) < MemorySizeInPages)
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

PVOID MmInitializePageList(PVOID FirstPhysKernelAddress,
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
   
   Reserved = 
     PAGE_ROUND_UP((MemorySizeInPages * sizeof(PHYSICAL_PAGE))) / PAGESIZE;
   MmPageArray = (PHYSICAL_PAGE *)LastKernelAddress;
   
   DPRINT("Reserved %d\n", Reserved);

   LastKernelAddress = PAGE_ROUND_UP(LastKernelAddress);
   LastKernelAddress = ((ULONG)LastKernelAddress + (Reserved * PAGESIZE));
   LastPhysKernelAddress = (PVOID)PAGE_ROUND_UP(LastPhysKernelAddress);
   LastPhysKernelAddress = LastPhysKernelAddress + (Reserved * PAGESIZE);
     
   MmStats.NrTotalPages = 0;
   MmStats.NrSystemPages = 0;
   MmStats.NrUserPages = 0;
   MmStats.NrReservedPages = 0;
   MmStats.NrFreePages = 0;
   MmStats.NrLockedPages = 0;
   
   for (i = 0; i < Reserved; i++)
     {
	if (!MmIsPagePresent(NULL, 
			     (PVOID)((ULONG)MmPageArray + (i * PAGESIZE))))
	  {
	     Status = 
	       MmCreateVirtualMappingUnsafe(NULL,
					    (PVOID)((ULONG)MmPageArray + 
						    (i * PAGESIZE)),
					    PAGE_READWRITE,
					    (ULONG)(LastPhysKernelAddress 
						    - (i * PAGESIZE)));
	     if (!NT_SUCCESS(Status))
	       {
		  DbgPrint("Unable to create virtual mapping\n");
		  KeBugCheck(0);
	       }
	  }
     }

   /*
    * Page zero is reserved
    */
   MmPageArray[0].Flags = MM_PHYSICAL_PAGE_BIOS;
   MmPageArray[0].ReferenceCount = 0;
   InsertTailList(&BiosPageListHead,
		  &MmPageArray[0].ListEntry); 

   /*
    * Page one is reserved for the initial KPCR
    */
   MmPageArray[1].Flags = MM_PHYSICAL_PAGE_BIOS;
   MmPageArray[1].ReferenceCount = 0;
   InsertTailList(&BiosPageListHead,
      &MmPageArray[1].ListEntry); 

   i = 2;
   if ((ULONG)FirstPhysKernelAddress < 0xa0000)
     {
	MmStats.NrFreePages += (((ULONG)FirstPhysKernelAddress/PAGESIZE) - 1);
	for (; i<((ULONG)FirstPhysKernelAddress/PAGESIZE); i++)
	  {
	     MmPageArray[i].Flags = MM_PHYSICAL_PAGE_FREE;
	     MmPageArray[i].ReferenceCount = 0;
	     InsertTailList(&FreeUnzeroedPageListHead,
			    &MmPageArray[i].ListEntry);
	  }
	MmStats.NrSystemPages += 
	  ((((ULONG)LastPhysKernelAddress) / PAGESIZE) - i);
	for (; i<((ULONG)LastPhysKernelAddress / PAGESIZE); i++)
	  {
	     MmPageArray[i].Flags = MM_PHYSICAL_PAGE_USED;
	     MmPageArray[i].ReferenceCount = 1;
	     InsertTailList(&UsedPageListHeads[MC_NPPOOL],
			    &MmPageArray[i].ListEntry);
	  }
	MmStats.NrFreePages += ((0xa0000/PAGESIZE) - i);
	for (; i<(0xa0000/PAGESIZE); i++)
	  {
	     MmPageArray[i].Flags = MM_PHYSICAL_PAGE_FREE;
	     MmPageArray[i].ReferenceCount = 0;
	     InsertTailList(&FreeUnzeroedPageListHead,
			    &MmPageArray[i].ListEntry);
	  }
	MmStats.NrReservedPages += ((0x100000/PAGESIZE) - i);
	for (; i<(0x100000 / PAGESIZE); i++)
	  {
	     MmPageArray[i].Flags = MM_PHYSICAL_PAGE_BIOS;
	     MmPageArray[i].ReferenceCount = 1;
	     InsertTailList(&BiosPageListHead,
			    &MmPageArray[i].ListEntry);
	  }
     }
   else
     {
	MmStats.NrFreePages += ((0xa0000 / PAGESIZE) - 1);	  
	for (; i<(0xa0000 / PAGESIZE); i++)
	  {
	     MmPageArray[i].Flags = MM_PHYSICAL_PAGE_FREE;
	     MmPageArray[i].ReferenceCount = 0;
	     InsertTailList(&FreeUnzeroedPageListHead,
			    &MmPageArray[i].ListEntry);
	  }
	MmStats.NrReservedPages += (0x60000 / PAGESIZE);
	for (; i<(0x100000 / PAGESIZE); i++)
	  {
	     MmPageArray[i].Flags = MM_PHYSICAL_PAGE_BIOS;
	     MmPageArray[i].ReferenceCount = 1;
	     InsertTailList(&BiosPageListHead,
			    &MmPageArray[i].ListEntry);
	  }
	MmStats.NrFreePages += (((ULONG)FirstPhysKernelAddress/PAGESIZE) - i);
	for (; i<((ULONG)FirstPhysKernelAddress/PAGESIZE); i++)
	  {
	     MmPageArray[i].Flags = MM_PHYSICAL_PAGE_FREE;
	     MmPageArray[i].ReferenceCount = 0;
	     InsertTailList(&FreeUnzeroedPageListHead,
			    &MmPageArray[i].ListEntry);
	  }
	MmStats.NrSystemPages += 
	  (((ULONG)LastPhysKernelAddress/PAGESIZE) - i);
	for (; i<((ULONG)LastPhysKernelAddress/PAGESIZE); i++)
	  {
	     MmPageArray[i].Flags = MM_PHYSICAL_PAGE_USED;
	     MmPageArray[i].ReferenceCount = 1;
	     InsertTailList(&UsedPageListHeads[MC_NPPOOL],
			    &MmPageArray[i].ListEntry);
	  }
     }
   
   MmStats.NrFreePages += (MemorySizeInPages - i);
   for (; i<MemorySizeInPages; i++)
     {
	MmPageArray[i].Flags = MM_PHYSICAL_PAGE_FREE;
	MmPageArray[i].ReferenceCount = 0;
	InsertTailList(&FreeUnzeroedPageListHead,
		       &MmPageArray[i].ListEntry);
     }

  if ((BIOSMemoryMap != NULL) && (AddressRangeCount > 0))
    {
      MiParseBIOSMemoryMap(
        MemorySizeInPages,
        BIOSMemoryMap,
        AddressRangeCount);
    }
  
   MmStats.NrTotalPages = MmStats.NrFreePages + MmStats.NrSystemPages +
     MmStats.NrReservedPages + MmStats.NrUserPages;
   MmInitializeBalancer(MmStats.NrFreePages);
   return((PVOID)LastKernelAddress);
}

VOID MmSetFlagsPage(PVOID PhysicalAddress,
		    ULONG Flags)
{
   ULONG Start = (ULONG)PhysicalAddress / PAGESIZE;
   KIRQL oldIrql;
   
   KeAcquireSpinLock(&PageListLock, &oldIrql);
   MmPageArray[Start].Flags = Flags;
   KeReleaseSpinLock(&PageListLock, oldIrql);
}

VOID 
MmSetRmapListHeadPage(PVOID PhysicalAddress, struct _MM_RMAP_ENTRY* ListHead)
{
   ULONG Start = (ULONG)PhysicalAddress / PAGESIZE;

   MmPageArray[Start].RmapListHead = ListHead;
}

struct _MM_RMAP_ENTRY*
MmGetRmapListHeadPage(PVOID PhysicalAddress)
{
  ULONG Start = (ULONG)PhysicalAddress / PAGESIZE;

  return(MmPageArray[Start].RmapListHead);
}

VOID 
MmMarkPageMapped(PVOID PhysicalAddress)
{
   ULONG Start = (ULONG)PhysicalAddress / PAGESIZE;
   KIRQL oldIrql;
   
   KeAcquireSpinLock(&PageListLock, &oldIrql);
   MmPageArray[Start].MapCount++;
   KeReleaseSpinLock(&PageListLock, oldIrql);
}

VOID 
MmMarkPageUnmapped(PVOID PhysicalAddress)
{
   ULONG Start = (ULONG)PhysicalAddress / PAGESIZE;
   KIRQL oldIrql;
   
   KeAcquireSpinLock(&PageListLock, &oldIrql);
   MmPageArray[Start].MapCount--;
   KeReleaseSpinLock(&PageListLock, oldIrql);
}

ULONG MmGetFlagsPage(PVOID PhysicalAddress)
{
   ULONG Start = (ULONG)PhysicalAddress / PAGESIZE;
   KIRQL oldIrql;
   ULONG Flags;
   
   KeAcquireSpinLock(&PageListLock, &oldIrql);
   Flags = MmPageArray[Start].Flags;
   KeReleaseSpinLock(&PageListLock, oldIrql);
   
   return(Flags);
}


VOID MmSetSavedSwapEntryPage(PVOID PhysicalAddress,
			     SWAPENTRY SavedSwapEntry)
{
   ULONG Start = (ULONG)PhysicalAddress / PAGESIZE;
   KIRQL oldIrql;
   
   KeAcquireSpinLock(&PageListLock, &oldIrql);
   MmPageArray[Start].SavedSwapEntry = SavedSwapEntry;
   KeReleaseSpinLock(&PageListLock, oldIrql);
}

SWAPENTRY 
MmGetSavedSwapEntryPage(PVOID PhysicalAddress)
{
   ULONG Start = (ULONG)PhysicalAddress / PAGESIZE;
   SWAPENTRY SavedSwapEntry;
   KIRQL oldIrql;
   
   KeAcquireSpinLock(&PageListLock, &oldIrql);
   SavedSwapEntry = MmPageArray[Start].SavedSwapEntry;
   KeReleaseSpinLock(&PageListLock, oldIrql);
   
   return(SavedSwapEntry);
}

VOID MmReferencePage(PVOID PhysicalAddress)
{
   ULONG Start = (ULONG)PhysicalAddress / PAGESIZE;
   KIRQL oldIrql;
   
   DPRINT("MmReferencePage(PhysicalAddress %x)\n", PhysicalAddress);
   
   if (((ULONG)PhysicalAddress) == 0)
     {
	KeBugCheck(0);
     }
   
   KeAcquireSpinLock(&PageListLock, &oldIrql);
   
   if (MM_PTYPE(MmPageArray[Start].Flags) != MM_PHYSICAL_PAGE_USED)
     {
	DbgPrint("Referencing non-used page\n");
	KeBugCheck(0);
     }
   
   MmPageArray[Start].ReferenceCount++;
   KeReleaseSpinLock(&PageListLock, oldIrql);
}

ULONG
MmGetReferenceCountPage(PVOID PhysicalAddress)
{
   ULONG Start = (ULONG)PhysicalAddress / PAGESIZE;
   KIRQL oldIrql;
   ULONG RCount;

   DPRINT("MmGetReferenceCountPage(PhysicalAddress %x)\n", PhysicalAddress);

   if (((ULONG)PhysicalAddress) == 0)
     {
	KeBugCheck(0);
     }
   
   KeAcquireSpinLock(&PageListLock, &oldIrql);
   
   if (MM_PTYPE(MmPageArray[Start].Flags) != MM_PHYSICAL_PAGE_USED)
     {
	DbgPrint("Getting reference count for free page\n");
	KeBugCheck(0);
     }
   
   RCount = MmPageArray[Start].ReferenceCount;

   KeReleaseSpinLock(&PageListLock, oldIrql);
   return(RCount);
}

BOOLEAN
MmIsUsablePage(PVOID PhysicalAddress)
{
   ULONG Start = (ULONG)PhysicalAddress / PAGESIZE;

   DPRINT("MmGetReferenceCountPage(PhysicalAddress %x)\n", PhysicalAddress);

   if (((ULONG)PhysicalAddress) == 0)
     {
	KeBugCheck(0);
     }

   if (MM_PTYPE(MmPageArray[Start].Flags) != MM_PHYSICAL_PAGE_USED &&
       MM_PTYPE(MmPageArray[Start].Flags) != MM_PHYSICAL_PAGE_BIOS)
     {
       return(FALSE);
     }
   
   return(TRUE);
}


VOID MmDereferencePage(PVOID PhysicalAddress)
{
   ULONG Start = (ULONG)PhysicalAddress / PAGESIZE;
   KIRQL oldIrql;
   
   DPRINT("MmDereferencePage(PhysicalAddress %x)\n", PhysicalAddress);

   if (((ULONG)PhysicalAddress) == 0)
     {
	KeBugCheck(0);
     }
   
   KeAcquireSpinLock(&PageListLock, &oldIrql);
  

   if (MM_PTYPE(MmPageArray[Start].Flags) != MM_PHYSICAL_PAGE_USED)
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
	   DbgPrint("Freeing mapped page (0x%x count %d)\n",
		    PhysicalAddress, MmPageArray[Start].MapCount);
	   KeBugCheck(0);
	 }
       if (MmPageArray[Start].LockCount > 0)
	 {
	   DbgPrint("Freeing locked page\n");
	   KeBugCheck(0);
	 }
       if (MmPageArray[Start].Flags != MM_PHYSICAL_PAGE_USED)
	 {
	   DbgPrint("Freeing page with flags %x\n",
		    MmPageArray[Start].Flags);
	   KeBugCheck(0);
	 }
       MmPageArray[Start].Flags = MM_PHYSICAL_PAGE_FREE;
       InsertTailList(&FreeUnzeroedPageListHead, &MmPageArray[Start].ListEntry);
     }
   KeReleaseSpinLock(&PageListLock, oldIrql);
}

ULONG MmGetLockCountPage(PVOID PhysicalAddress)
{
   ULONG Start = (ULONG)PhysicalAddress / PAGESIZE;
   KIRQL oldIrql;
   ULONG LockCount;
   
   DPRINT("MmGetLockCountPage(PhysicalAddress %x)\n", PhysicalAddress);
   
   if (((ULONG)PhysicalAddress) == 0)
     {
	KeBugCheck(0);
     }
   
   KeAcquireSpinLock(&PageListLock, &oldIrql);
   
   if (MM_PTYPE(MmPageArray[Start].Flags) != MM_PHYSICAL_PAGE_USED)
     {
	DbgPrint("Getting lock count for free page\n");
	KeBugCheck(0);
     }
   
   LockCount = MmPageArray[Start].LockCount;
   KeReleaseSpinLock(&PageListLock, oldIrql);

   return(LockCount);
}

VOID MmLockPage(PVOID PhysicalAddress)
{
   ULONG Start = (ULONG)PhysicalAddress / PAGESIZE;
   KIRQL oldIrql;
   
   DPRINT("MmLockPage(PhysicalAddress %x)\n", PhysicalAddress);
   
   if (((ULONG)PhysicalAddress) == 0)
     {
	KeBugCheck(0);
     }
   
   KeAcquireSpinLock(&PageListLock, &oldIrql);
   
   if (MM_PTYPE(MmPageArray[Start].Flags) != MM_PHYSICAL_PAGE_USED)
     {
	DbgPrint("Locking free page\n");
	KeBugCheck(0);
     }
   
   MmPageArray[Start].LockCount++;
   KeReleaseSpinLock(&PageListLock, oldIrql);
}

VOID MmUnlockPage(PVOID PhysicalAddress)
{
   ULONG Start = (ULONG)PhysicalAddress / PAGESIZE;
   KIRQL oldIrql;
   
   DPRINT("MmUnlockPage(PhysicalAddress %x)\n", PhysicalAddress);
   
   if (((ULONG)PhysicalAddress) == 0)
     {
	KeBugCheck(0);
     }
   
   KeAcquireSpinLock(&PageListLock, &oldIrql);
   
   if (MM_PTYPE(MmPageArray[Start].Flags) != MM_PHYSICAL_PAGE_USED)
     {
	DbgPrint("Unlocking free page\n");
	KeBugCheck(0);
     }
   
   MmPageArray[Start].LockCount--;
   KeReleaseSpinLock(&PageListLock, oldIrql);
}


PVOID 
MmAllocPage(ULONG Consumer, SWAPENTRY SavedSwapEntry)
{
   ULONG offset;
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
	   return(NULL);
	 }
       ListEntry = RemoveTailList(&FreeUnzeroedPageListHead);

       PageDescriptor = CONTAINING_RECORD(ListEntry, PHYSICAL_PAGE, ListEntry);
       KeReleaseSpinLock(&PageListLock, oldIrql);

       NeedClear = TRUE;
     }
   else
     {
       ListEntry = RemoveTailList(&FreeZeroedPageListHead);
       KeReleaseSpinLock(&PageListLock, oldIrql);

       PageDescriptor = CONTAINING_RECORD(ListEntry, PHYSICAL_PAGE, ListEntry);
     }

   if (PageDescriptor->Flags != MM_PHYSICAL_PAGE_FREE)
     {
	DbgPrint("Got non-free page from freelist\n");
	KeBugCheck(0);
     }
   PageDescriptor->Flags = MM_PHYSICAL_PAGE_USED;
   PageDescriptor->ReferenceCount = 1;
   PageDescriptor->LockCount = 0;
   PageDescriptor->MapCount = 0;
   PageDescriptor->SavedSwapEntry = SavedSwapEntry;
   ExInterlockedInsertTailList(&UsedPageListHeads[Consumer], ListEntry, &PageListLock);
   
   MmStats.NrSystemPages++;
   MmStats.NrFreePages--;

   offset = (ULONG)((ULONG)PageDescriptor - (ULONG)MmPageArray);
   offset = (offset / sizeof(PHYSICAL_PAGE)) * PAGESIZE;   
   if (NeedClear)
     {
       MiZeroPage(offset);
     }
   DPRINT("MmAllocPage() = %x\n",offset);
   return((PVOID)offset);
}
