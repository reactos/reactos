/*
 * COPYRIGHT:    See COPYING in the top level directory
 * PROJECT:      ReactOS kernel
 * FILE:         ntoskrnl/mm/freelist.c
 * PURPOSE:      Handle the list of free physical pages
 * PROGRAMMER:   David Welch (welch@cwcom.net)
 *               Casper S. Hornstrup (chorns@users.sourceforge.net)
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

/* GLOBALS ****************************************************************/

PPHYSICAL_PAGE MmPageArray;

KSPIN_LOCK MiPageListLock;
LIST_ENTRY MiUsedPageListHeads[MC_MAXIMUM];
LIST_ENTRY MiFreeZeroedPageListHead;
LIST_ENTRY MiFreeUnzeroedPageListHead;
LIST_ENTRY MiBiosPageListHead;
LIST_ENTRY MiStandbyPageListHead;
ULONG MiStandbyPageListSize;
LIST_ENTRY MiModifiedPageListHead;
ULONG MiModifiedPageListSize;
LIST_ENTRY MiModifiedNoWritePageListHead;
ULONG MiModifiedNoWritePageListSize;
/* LIST_ENTRY BadPageListHead; */

/* FUNCTIONS *************************************************************/

VOID
MiAcquirePageListLock(IN ULONG  PageList,
  OUT PLIST_ENTRY  * ListHead)
{
  KeAcquireSpinLockAtDpcLevel(&MiPageListLock);
  switch (PageList)
		{
      case PAGE_LIST_FREE_ZEROED:
        *ListHead = &MiFreeZeroedPageListHead;
        break;
      case PAGE_LIST_FREE_UNZEROED:
        *ListHead = &MiFreeUnzeroedPageListHead;
        break;
      case PAGE_LIST_BIOS:
        *ListHead = &MiBiosPageListHead;
        break;
      case PAGE_LIST_STANDBY:
        *ListHead = &MiStandbyPageListHead;
        break;
      case PAGE_LIST_MODIFIED:
        *ListHead = &MiModifiedPageListHead;
        break;
      case PAGE_LIST_MODIFIED_NO_WRITE:
        *ListHead = &MiModifiedNoWritePageListHead;
        break;
      default:
        DPRINT1("Bad page list type 0x%.08x\n", PageList);
        KeBugCheck(0);
        break;
		}
}


VOID
MiReleasePageListLock()
{
  KeReleaseSpinLockFromDpcLevel(&MiPageListLock);
}


VOID
MmTransferOwnershipPage(IN ULONG_PTR  PhysicalAddress,
  IN ULONG  NewConsumer)
{
  ULONG Start = PhysicalAddress / PAGESIZE;
  KIRQL oldIrql;

  VALIDATE_PHYSICAL_ADDRESS(PhysicalAddress);

  KeAcquireSpinLock(&MiPageListLock, &oldIrql);
  RemoveEntryList(&MmPageArray[Start].ListEntry);
  MmPageArray[Start].Flags = MM_PHYSICAL_PAGE_USED;
  InsertTailList(&MiUsedPageListHeads[NewConsumer],
    &MmPageArray[Start].ListEntry);
  KeReleaseSpinLock(&MiPageListLock, oldIrql);  
}

ULONG_PTR
MmGetLRUFirstUserPage(VOID)
{
  PLIST_ENTRY NextListEntry;
  ULONG_PTR Next;
  PHYSICAL_PAGE* PageDescriptor;
  KIRQL oldIrql;

  KeAcquireSpinLock(&MiPageListLock, &oldIrql);
  NextListEntry = MiUsedPageListHeads[MC_USER].Flink;
  if (NextListEntry == &MiUsedPageListHeads[MC_USER])
    {
      KeReleaseSpinLock(&MiPageListLock, oldIrql);
      return(0);
    }
  PageDescriptor = CONTAINING_RECORD(NextListEntry, PHYSICAL_PAGE, ListEntry);
  //Next = ((ULONG_PTR) PageDescriptor - (ULONG_PTR)MmPageArray);
  //Next = (Next / sizeof(PHYSICAL_PAGE)) * PAGESIZE;   
  Next = MiPageFromDescriptor(PageDescriptor);

  assertmsg(PageDescriptor->Flags == MM_PHYSICAL_PAGE_USED,
    ("Page at 0x%.08x on used page list is not used.\n", Next));

  KeReleaseSpinLock(&MiPageListLock, oldIrql);
  return(Next);
}

ULONG_PTR
MmGetLRUNextUserPage(IN ULONG_PTR PreviousPhysicalAddress)
{
  ULONG Start = PreviousPhysicalAddress / PAGESIZE;
  PLIST_ENTRY NextListEntry;
  ULONG_PTR Next;
  PHYSICAL_PAGE* PageDescriptor;
  KIRQL oldIrql;

  VALIDATE_PHYSICAL_ADDRESS(PreviousPhysicalAddress);

  KeAcquireSpinLock(&MiPageListLock, &oldIrql);
  if (!(MmPageArray[Start].Flags & MM_PHYSICAL_PAGE_USED))
    {
      NextListEntry = MiUsedPageListHeads[MC_USER].Flink;
    }
  else
    {
      NextListEntry = MmPageArray[Start].ListEntry.Flink;
    }
  if (NextListEntry == &MiUsedPageListHeads[MC_USER])
    {
      KeReleaseSpinLock(&MiPageListLock, oldIrql);
      return(0);
    }
  PageDescriptor = CONTAINING_RECORD(NextListEntry, PHYSICAL_PAGE, ListEntry);
  Next = MiPageFromDescriptor(PageDescriptor);

  assertmsg(PageDescriptor->Flags == MM_PHYSICAL_PAGE_USED,
    ("Page at 0x%.08x on used page list is not used.\n", Next));

  KeReleaseSpinLock(&MiPageListLock, oldIrql);
  return(Next);
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
   
   KeAcquireSpinLock(&MiPageListLock, &oldIrql);
   
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
	KeReleaseSpinLock(&MiPageListLock, oldIrql);
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
	InsertTailList(&MiUsedPageListHeads[MC_NPPOOL], 
		       &MmPageArray[i].ListEntry);
     }
   KeReleaseSpinLock(&MiPageListLock, oldIrql);
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
	        InsertTailList(&MiFreeUnzeroedPageListHead,
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
	        InsertTailList(&MiBiosPageListHead,
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
       InitializeListHead(&MiUsedPageListHeads[i]);
     }
   KeInitializeSpinLock(&MiPageListLock);
   InitializeListHead(&MiFreeUnzeroedPageListHead);
   InitializeListHead(&MiFreeZeroedPageListHead);
   InitializeListHead(&MiBiosPageListHead);
   InitializeListHead(&MiStandbyPageListHead);
   MiStandbyPageListSize = 0;
   InitializeListHead(&MiModifiedPageListHead);
   MiModifiedPageListSize = 0;
   InitializeListHead(&MiModifiedNoWritePageListHead);
   MiModifiedNoWritePageListSize = 0;

   LastKernelAddress = PAGE_ROUND_UP(LastKernelAddress);
   
   Reserved = 
     PAGE_ROUND_UP((MemorySizeInPages * sizeof(PHYSICAL_PAGE))) / PAGESIZE;
   MmPageArray = (PHYSICAL_PAGE *)LastKernelAddress;
   
   DPRINT("Reserved %d\n", Reserved);

   LastKernelAddress = PAGE_ROUND_UP(LastKernelAddress);
   LastKernelAddress = ((ULONG_PTR) LastKernelAddress + (Reserved * PAGESIZE));
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
	if (!MmIsPagePresent(NULL, (PVOID) ((ULONG_PTR)MmPageArray + (i * PAGESIZE))))
	  {
	     Status =
	       MmCreateVirtualMappingUnsafe(NULL,
					    (PVOID)((ULONG_PTR)MmPageArray + 
						    (i * PAGESIZE)),
					    PAGE_READWRITE,
					    (ULONG)(LastPhysKernelAddress 
						    - (Reserved * PAGESIZE) + (i * PAGESIZE)),
					    FALSE);
	     if (!NT_SUCCESS(Status))
	       {
		  DbgPrint("Unable to create virtual mapping\n");
		  KeBugCheck(0);
	       }
	  }
	memset((PVOID)MmPageArray + (i * PAGESIZE), 0, PAGESIZE);
     }

   /*
    * Page zero is reserved
    */
   MmPageArray[0].Flags = MM_PHYSICAL_PAGE_BIOS;
   MmPageArray[0].ReferenceCount = 0;
   InsertTailList(&MiBiosPageListHead,
		  &MmPageArray[0].ListEntry);

   /*
    * Page one is reserved for the initial KPCR
    */
   MmPageArray[1].Flags = MM_PHYSICAL_PAGE_BIOS;
   MmPageArray[1].ReferenceCount = 0;
   InsertTailList(&MiBiosPageListHead,
      &MmPageArray[1].ListEntry); 

   i = 2;
   if ((ULONG_PTR) FirstPhysKernelAddress < 0xa0000)
     {
	MmStats.NrFreePages += (((ULONG_PTR) FirstPhysKernelAddress/PAGESIZE) - 1);
	for (; i<((ULONG_PTR) FirstPhysKernelAddress/PAGESIZE); i++)
	  {
	     MmPageArray[i].Flags = MM_PHYSICAL_PAGE_FREE;
	     MmPageArray[i].ReferenceCount = 0;
	     InsertTailList(&MiFreeUnzeroedPageListHead,
			    &MmPageArray[i].ListEntry);
	  }
	MmStats.NrSystemPages += 
	  ((((ULONG_PTR) LastPhysKernelAddress) / PAGESIZE) - i);
	for (; i<((ULONG_PTR) LastPhysKernelAddress / PAGESIZE); i++)
	  {
	     MmPageArray[i].Flags = MM_PHYSICAL_PAGE_USED;
	     MmPageArray[i].ReferenceCount = 1;
	     InsertTailList(&MiUsedPageListHeads[MC_NPPOOL],
			    &MmPageArray[i].ListEntry);
	  }
	MmStats.NrFreePages += ((0xa0000/PAGESIZE) - i);
	for (; i<(0xa0000/PAGESIZE); i++)
	  {
	     MmPageArray[i].Flags = MM_PHYSICAL_PAGE_FREE;
	     MmPageArray[i].ReferenceCount = 0;
	     InsertTailList(&MiFreeUnzeroedPageListHead,
			    &MmPageArray[i].ListEntry);
	  }
	MmStats.NrReservedPages += ((0x100000/PAGESIZE) - i);
	for (; i<(0x100000 / PAGESIZE); i++)
	  {
	     MmPageArray[i].Flags = MM_PHYSICAL_PAGE_BIOS;
	     MmPageArray[i].ReferenceCount = 1;
	     InsertTailList(&MiBiosPageListHead,
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
	     InsertTailList(&MiFreeUnzeroedPageListHead,
			    &MmPageArray[i].ListEntry);
	  }
	MmStats.NrReservedPages += (0x60000 / PAGESIZE);
	for (; i<(0x100000 / PAGESIZE); i++)
	  {
	     MmPageArray[i].Flags = MM_PHYSICAL_PAGE_BIOS;
	     MmPageArray[i].ReferenceCount = 1;
	     InsertTailList(&MiBiosPageListHead,
			    &MmPageArray[i].ListEntry);
	  }
	MmStats.NrFreePages += (((ULONG_PTR) FirstPhysKernelAddress/PAGESIZE) - i);
	for (; i<((ULONG_PTR) FirstPhysKernelAddress/PAGESIZE); i++)
	  {
	     MmPageArray[i].Flags = MM_PHYSICAL_PAGE_FREE;
	     MmPageArray[i].ReferenceCount = 0;
	     InsertTailList(&MiFreeUnzeroedPageListHead,
			    &MmPageArray[i].ListEntry);
	  }
	MmStats.NrSystemPages +=
	  (((ULONG_PTR) LastPhysKernelAddress/PAGESIZE) - i);
	for (; i<((ULONG_PTR) LastPhysKernelAddress/PAGESIZE); i++)
	  {
	     MmPageArray[i].Flags = MM_PHYSICAL_PAGE_USED;
	     MmPageArray[i].ReferenceCount = 1;
	     InsertTailList(&MiUsedPageListHeads[MC_NPPOOL],
			    &MmPageArray[i].ListEntry);
	  }
     }
   
   MmStats.NrFreePages += (MemorySizeInPages - i);
   for (; i<MemorySizeInPages; i++)
     {
	MmPageArray[i].Flags = MM_PHYSICAL_PAGE_FREE;
	MmPageArray[i].ReferenceCount = 0;
	InsertTailList(&MiFreeUnzeroedPageListHead,
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


VOID
MmSetFlagsPage(IN ULONG_PTR  PhysicalAddress,
  IN ULONG  Flags)
{
   ULONG Start = PhysicalAddress / PAGESIZE;
   KIRQL oldIrql;
   
   KeAcquireSpinLock(&MiPageListLock, &oldIrql);
   MmPageArray[Start].Flags = Flags;
   KeReleaseSpinLock(&MiPageListLock, oldIrql);
}


VOID
MmSetRmapListHeadPage(IN ULONG_PTR  PhysicalAddress,
  IN struct _MM_RMAP_ENTRY*  ListHead)
{
  ULONG Start = PhysicalAddress / PAGESIZE;

  VALIDATE_PHYSICAL_ADDRESS(PhysicalAddress);

	VALIDATE_RMAP_LIST(ListHead);

  MmPageArray[Start].RmapListHead = ListHead;
}

struct _MM_RMAP_ENTRY*
MmGetRmapListHeadPage(IN ULONG_PTR  PhysicalAddress)
{
  ULONG Start = PhysicalAddress / PAGESIZE;

  VALIDATE_PHYSICAL_ADDRESS(PhysicalAddress);

	VALIDATE_RMAP_LIST(MmPageArray[Start].RmapListHead);

  return(MmPageArray[Start].RmapListHead);
}


VOID 
MmSetRmapCallback(IN ULONG_PTR  PhysicalAddress,
  IN PRMAP_DELETE_CALLBACK  RmapDelete,
  IN PVOID  RmapDeleteContext)
{
  ULONG Start = PhysicalAddress / PAGESIZE;

  VALIDATE_PHYSICAL_ADDRESS(PhysicalAddress);

  MmPageArray[Start].RmapDelete = RmapDelete;
  MmPageArray[Start].RmapDeleteContext = RmapDeleteContext;
}


VOID 
MmGetRmapCallback(IN ULONG_PTR  PhysicalAddress,
  IN PRMAP_DELETE_CALLBACK  *RmapDelete,
  IN PVOID  *RmapDeleteContext)
{
  ULONG Start = PhysicalAddress / PAGESIZE;

  VALIDATE_PHYSICAL_ADDRESS(PhysicalAddress);

  *RmapDelete = MmPageArray[Start].RmapDelete;
  *RmapDeleteContext = MmPageArray[Start].RmapDeleteContext;
}


VOID 
MmMarkPageMapped(IN ULONG_PTR  PhysicalAddress)
{
	ULONG Start = PhysicalAddress / PAGESIZE;
	KIRQL oldIrql;
	
  VALIDATE_PHYSICAL_ADDRESS(PhysicalAddress);

	KeAcquireSpinLock(&MiPageListLock, &oldIrql);
	MmPageArray[Start].MapCount++;
	KeReleaseSpinLock(&MiPageListLock, oldIrql);
}

VOID 
MmMarkPageUnmapped(IN ULONG_PTR  PhysicalAddress)
{
	ULONG Start = PhysicalAddress / PAGESIZE;
	KIRQL oldIrql;

  VALIDATE_PHYSICAL_ADDRESS(PhysicalAddress);
   
	KeAcquireSpinLock(&MiPageListLock, &oldIrql);
	MmPageArray[Start].MapCount--;
	KeReleaseSpinLock(&MiPageListLock, oldIrql);
}

ULONG
MmGetFlagsPage(IN ULONG_PTR  PhysicalAddress)
{
	ULONG Start = PhysicalAddress / PAGESIZE;
	KIRQL oldIrql;
	ULONG Flags;

  VALIDATE_PHYSICAL_ADDRESS(PhysicalAddress);

	KeAcquireSpinLock(&MiPageListLock, &oldIrql);
	Flags = MmPageArray[Start].Flags;
	KeReleaseSpinLock(&MiPageListLock, oldIrql);
	
	return(Flags);
}


VOID
MmSetSavedSwapEntryPage(IN ULONG_PTR  PhysicalAddress,
  IN SWAPENTRY  SavedSwapEntry)
{
	ULONG Start = PhysicalAddress / PAGESIZE;
	KIRQL oldIrql;

	VALIDATE_PHYSICAL_ADDRESS(PhysicalAddress);

	VALIDATE_SWAP_ENTRY(SavedSwapEntry);
	
	KeAcquireSpinLock(&MiPageListLock, &oldIrql);
	MmPageArray[Start].SavedSwapEntry = SavedSwapEntry;
	KeReleaseSpinLock(&MiPageListLock, oldIrql);
}

SWAPENTRY 
MmGetSavedSwapEntryPage(IN ULONG_PTR  PhysicalAddress)
{
	ULONG Start = PhysicalAddress / PAGESIZE;
	SWAPENTRY SavedSwapEntry;
	KIRQL oldIrql;

  VALIDATE_PHYSICAL_ADDRESS(PhysicalAddress);

	KeAcquireSpinLock(&MiPageListLock, &oldIrql);
	SavedSwapEntry = MmPageArray[Start].SavedSwapEntry;
	KeReleaseSpinLock(&MiPageListLock, oldIrql);
	
	VALIDATE_SWAP_ENTRY(SavedSwapEntry);
	
	return(SavedSwapEntry);
}


VOID
MmSetSavedPageOp(IN ULONG_PTR  PhysicalAddress,
  IN PMM_PAGEOP  PageOp)
{
	ULONG Start = PhysicalAddress / PAGESIZE;

	VALIDATE_PHYSICAL_ADDRESS(PhysicalAddress);
	
  VALIDATE_PAGEOP(PageOp);

	MmPageArray[Start].PageOp = PageOp;
}


PMM_PAGEOP
MmGetSavedPageOp(IN ULONG_PTR  PhysicalAddress)
{
	ULONG Start = PhysicalAddress / PAGESIZE;
	PMM_PAGEOP PageOp;

  VALIDATE_PHYSICAL_ADDRESS(PhysicalAddress);

	PageOp = MmPageArray[Start].PageOp;

  VALIDATE_PAGEOP(PageOp);

	return(PageOp);
}


VOID MmReferencePage(IN ULONG_PTR  PhysicalAddress)
{
	ULONG Start = PhysicalAddress / PAGESIZE;
	KIRQL oldIrql;

  VALIDATE_PHYSICAL_ADDRESS(PhysicalAddress);

	DPRINT("MmReferencePage(PhysicalAddress %x)\n", PhysicalAddress);

	if (PhysicalAddress == 0)
	 {
     assert(FALSE);
	 }

	KeAcquireSpinLock(&MiPageListLock, &oldIrql);

	if (MM_PTYPE(MmPageArray[Start].Flags) == MM_PHYSICAL_PAGE_FREE)
	  {
	    assertmsg(FALSE, ("Referencing non-used page\n"));
	  }

	MmPageArray[Start].ReferenceCount++;
	KeReleaseSpinLock(&MiPageListLock, oldIrql);
}

ULONG
MmGetReferenceCountPage(IN ULONG_PTR  PhysicalAddress)
{
	ULONG Start = PhysicalAddress / PAGESIZE;
	KIRQL oldIrql;
	ULONG RCount;

  VALIDATE_PHYSICAL_ADDRESS(PhysicalAddress);

	DPRINT("MmGetReferenceCountPage(PhysicalAddress %x)\n", PhysicalAddress);
	
	if (PhysicalAddress == 0)
	 {
	KeBugCheck(0);
	 }

	KeAcquireSpinLock(&MiPageListLock, &oldIrql);
	
	if (MM_PTYPE(MmPageArray[Start].Flags) == MM_PHYSICAL_PAGE_FREE)
	 {
	DbgPrint("Getting reference count for free page\n");
	KeBugCheck(0);
	 }
	
	RCount = MmPageArray[Start].ReferenceCount;
	
	KeReleaseSpinLock(&MiPageListLock, oldIrql);
	return(RCount);
}

BOOLEAN
MmIsUsablePage(IN ULONG_PTR  PhysicalAddress)
{
  ULONG Start = PhysicalAddress / PAGESIZE;

  VALIDATE_PHYSICAL_ADDRESS(PhysicalAddress);

	DPRINT("MmGetReferenceCountPage(PhysicalAddress %x)\n", PhysicalAddress);
	
	if (PhysicalAddress == 0)
	 {
	KeBugCheck(0);
	 }
	
	if (MM_PTYPE(MmPageArray[Start].Flags) == MM_PHYSICAL_PAGE_FREE)
	 {
	   return(FALSE);
	 }
	
	return(TRUE);
}


VOID MmDereferencePage(IN ULONG_PTR  PhysicalAddress)
{
	ULONG Start = PhysicalAddress / PAGESIZE;
	KIRQL oldIrql;

  VALIDATE_PHYSICAL_ADDRESS(PhysicalAddress);

	DPRINT("MmDereferencePage(PhysicalAddress %x)\n", PhysicalAddress);

	if (PhysicalAddress == 0)
	 {
	KeBugCheck(0);
	 }

	KeAcquireSpinLock(&MiPageListLock, &oldIrql);

	if (MM_PTYPE(MmPageArray[Start].Flags) == MM_PHYSICAL_PAGE_FREE)
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
	   if (MmPageArray[Start].SavedSwapEntry != 0)
	{
	 DbgPrint("Freeing page with swap entry.\n");
	 KeBugCheck(0);
	}
	   if (MmPageArray[Start].Flags == MM_PHYSICAL_PAGE_FREE)
	{
	 DbgPrint("Freeing page with flags %x\n",
	    MmPageArray[Start].Flags);
	 KeBugCheck(0);
	}
	   MmPageArray[Start].Flags = MM_PHYSICAL_PAGE_FREE;
	   InsertTailList(&MiFreeUnzeroedPageListHead,
	      &MmPageArray[Start].ListEntry);
	 }
	KeReleaseSpinLock(&MiPageListLock, oldIrql);
}

ULONG
MiGetLockCountPage(IN ULONG_PTR  PhysicalAddress)
{
	ULONG Start = PhysicalAddress / PAGESIZE;
	KIRQL oldIrql;
	ULONG LockCount;

  VALIDATE_PHYSICAL_ADDRESS(PhysicalAddress);

	DPRINT("MmGetLockCountPage(PhysicalAddress %x)\n", PhysicalAddress);

	if (PhysicalAddress == 0)
	 {
	KeBugCheck(0);
	 }

	KeAcquireSpinLock(&MiPageListLock, &oldIrql);

	if (MM_PTYPE(MmPageArray[Start].Flags) == MM_PHYSICAL_PAGE_FREE)
	 {
	DbgPrint("Getting lock count for free page\n");
	KeBugCheck(0);
	 }
	
	LockCount = MmPageArray[Start].LockCount;
	KeReleaseSpinLock(&MiPageListLock, oldIrql);
	
	return(LockCount);
}


VOID
MmLockPage(IN ULONG_PTR  PhysicalAddress)
{
	ULONG Start = PhysicalAddress / PAGESIZE;
	KIRQL oldIrql;

  VALIDATE_PHYSICAL_ADDRESS(PhysicalAddress);
   
	DPRINT("MmLockPage(PhysicalAddress %x)\n", PhysicalAddress);

	if (PhysicalAddress == 0)
	 {
	KeBugCheck(0);
	 }

	KeAcquireSpinLock(&MiPageListLock, &oldIrql);

	if (MM_PTYPE(MmPageArray[Start].Flags) == MM_PHYSICAL_PAGE_FREE)
	 {
	DbgPrint("Locking free page\n");
	KeBugCheck(0);
	 }
	
	MmPageArray[Start].LockCount++;
	KeReleaseSpinLock(&MiPageListLock, oldIrql);
}


VOID
MmUnlockPage(IN ULONG_PTR  PhysicalAddress)
{
	ULONG Start = PhysicalAddress / PAGESIZE;
	KIRQL oldIrql;

  VALIDATE_PHYSICAL_ADDRESS(PhysicalAddress);
   
	DPRINT("MmUnlockPage(PhysicalAddress %x)\n", PhysicalAddress);
	
	if (PhysicalAddress == 0)
	 {
	KeBugCheck(0);
	 }

	KeAcquireSpinLock(&MiPageListLock, &oldIrql);

	if (MM_PTYPE(MmPageArray[Start].Flags) == MM_PHYSICAL_PAGE_FREE)
	 {
	DbgPrint("Unlocking free page\n");
	KeBugCheck(0);
	 }

	MmPageArray[Start].LockCount--;
	KeReleaseSpinLock(&MiPageListLock, oldIrql);
}


ULONG_PTR
MmAllocPage(IN ULONG  Consumer,
 IN SWAPENTRY  SavedSwapEntry)
{
	ULONG_PTR Page;
	PLIST_ENTRY ListEntry;
	PPHYSICAL_PAGE PageDescriptor;
	KIRQL oldIrql;
	BOOLEAN NeedClear = FALSE;

	VALIDATE_SWAP_ENTRY(SavedSwapEntry);

	DPRINT("MmAllocPage()\n");

	KeAcquireSpinLock(&MiPageListLock, &oldIrql);
	if (IsListEmpty(&MiFreeZeroedPageListHead))
	 {
	   if (IsListEmpty(&MiFreeUnzeroedPageListHead))
	    {
	      if (IsListEmpty(&MiStandbyPageListHead))
				 {
				   DPRINT1("MmAllocPage(): Out of memory\n");
				   KeReleaseSpinLock(&MiPageListLock, oldIrql);
				   return(0);
				 }
	      ListEntry = RemoveTailList(&MiStandbyPageListHead);
	      PageDescriptor = CONTAINING_RECORD(ListEntry, PHYSICAL_PAGE, ListEntry);
        Page = MiPageFromDescriptor(PageDescriptor);
        DPRINT1("Real free page at 0x%.08x\n", Page);
	      MmDeleteAllRmaps(Page);
        MiFreePageMemoryConsumer(Consumer, Page);
	    }
	  else
	    {
	      ListEntry = RemoveTailList(&MiFreeUnzeroedPageListHead);
	      PageDescriptor = CONTAINING_RECORD(ListEntry, PHYSICAL_PAGE, ListEntry);
        Page = MiPageFromDescriptor(PageDescriptor);
	    }

	   KeReleaseSpinLock(&MiPageListLock, oldIrql);

	   NeedClear = TRUE;
	 }
	else
	 {
	   ListEntry = RemoveTailList(&MiFreeZeroedPageListHead);
	   KeReleaseSpinLock(&MiPageListLock, oldIrql);

	   PageDescriptor = CONTAINING_RECORD(ListEntry, PHYSICAL_PAGE, ListEntry);
       Page = MiPageFromDescriptor(PageDescriptor);
	 }

	if (PageDescriptor->Flags != MM_PHYSICAL_PAGE_FREE)
	 {
	DbgPrint("Got non-free page from free list\n");
	KeBugCheck(0);
	 }
	PageDescriptor->Flags = MM_PHYSICAL_PAGE_USED;
	PageDescriptor->ReferenceCount = 1;
	PageDescriptor->LockCount = 0;
	PageDescriptor->MapCount = 0;
	PageDescriptor->SavedSwapEntry = SavedSwapEntry;
	ExInterlockedInsertTailList(&MiUsedPageListHeads[Consumer], ListEntry,
		       &MiPageListLock);
	InterlockedIncrement(&MmStats.NrSystemPages);
	InterlockedDecrement(&MmStats.NrFreePages);

	if (NeedClear)
	 {
	   MiZeroPage(Page);
	 }

	DPRINT("MmAllocPage() = %x\n", Page);
	return(Page);
}


/*
 * This routine is used by both the Working Set Manager and the Modified Page
 * Writer. The Working Set Manager uses this routine to remove a physical page
 * from a working set and put it on either the standby page list (if the page
 * is not dirty) or on the modified page list (if the page is dirty).
 * The Modified Page Writer uses this routine to put the page on the standby
 * page list (if the page was successfully written to secondary storage) or
 * on the modified page list (if the page could not be written to secondary
 * storage).
 */
VOID
MiReclaimPage(IN ULONG_PTR  PhysicalAddress,
  IN BOOLEAN  Dirty)
{
  ULONG Start = PhysicalAddress / PAGESIZE;
  KIRQL OldIrql;

  VALIDATE_PHYSICAL_ADDRESS(PhysicalAddress);

	DPRINT("MmReclaimPage(PhysicalAddress 0x%.08x)\n", PhysicalAddress);

	if (PhysicalAddress == 0)
    {
      DPRINT1("Cannot reclaim physical page at 0x%.08x\n", PhysicalAddress);
      KeBugCheck(0);
    }

  KeAcquireSpinLock(&MiPageListLock, &OldIrql);

	if (MM_PTYPE(MmPageArray[Start].Flags) == MM_PHYSICAL_PAGE_FREE)
    {
      DPRINT1("Cannot reclaim free page\n");
      KeBugCheck(0);
    }

  /* Remove the page from the page list it is on. If it is the Working Set
     Manager that is reclaiming the page, this is the consumer's used page
     list. If it is the Modified Page Writer that is reclaiming the page,
     this is the MPW page list */
  RemoveEntryList(&MmPageArray[Start].ListEntry);

  if (Dirty)
    {
      DPRINT1("Sending to modified page list 0x%.08x\n", PhysicalAddress);
      MmPageArray[Start].Flags = MM_PHYSICAL_PAGE_MODIFIED;
      InsertTailList(&MiModifiedPageListHead, &MmPageArray[Start].ListEntry);
      InterlockedIncrement(&MiModifiedPageListSize);

      if (MmPageArray[Start].RmapListHead == NULL)
				{
          DPRINT1("No Rmaps\n");
          KeBugCheck(0);
				}

      KeReleaseSpinLock(&MiPageListLock, OldIrql);

		if (MiModifiedPageListSize > MiMaximumModifiedPageListSize)
			{
			/* Too many modified pages exists so tell the modified page
				writer to write out some modified pages so they can be freed */
			MiSignalModifiedPageWriter();
			}
	  }
	else
	{
		MmPageArray[Start].Flags = MM_PHYSICAL_PAGE_STANDBY;

		DPRINT1("Sending to standby page list 0x%.08x\n", PhysicalAddress);

		InsertTailList(&MiStandbyPageListHead, &MmPageArray[Start].ListEntry);
		InterlockedIncrement(&MiStandbyPageListSize);
		KeReleaseSpinLock(&MiPageListLock, OldIrql);
		MiSatisfyAllocationRequest();
	}
}
