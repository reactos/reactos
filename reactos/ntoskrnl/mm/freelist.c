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
#include <internal/mmhal.h>
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
   KEVENT Event;
   SWAPENTRY SavedSwapEntry;
   ULONG LockCount;
} PHYSICAL_PAGE, *PPHYSICAL_PAGE;

/* GLOBALS ****************************************************************/

static PPHYSICAL_PAGE MmPageArray;

static LIST_ENTRY UsedPageListHead;
static KSPIN_LOCK PageListLock;
static LIST_ENTRY FreePageListHead;
static LIST_ENTRY BiosPageListHead;

/* FUNCTIONS *************************************************************/

PVOID
MmGetContinuousPages(ULONG NumberOfBytes,
		     PHYSICAL_ADDRESS HighestAcceptableAddress)
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
   for (i = 0; i < (HighestAcceptableAddress.QuadPart / PAGESIZE); i++)
     {
	if (MmPageArray[i].Flags & MM_PHYSICAL_PAGE_FREE)
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
	     if (length == NrPages)
	       {
		  break;
	       }	     
	  }
	else if (start != -1)
	  {
	     start = -1;
	  }
     }
   if (start == -1)
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
	MmPageArray[i].SavedSwapEntry = 0;
	InsertTailList(&UsedPageListHead, &MmPageArray[i].ListEntry);
     }
   return((PVOID)(start * 4096));
}

PVOID MmInitializePageList(PVOID FirstPhysKernelAddress,
			   PVOID LastPhysKernelAddress,
			   ULONG MemorySizeInPages,
			   ULONG LastKernelAddress)
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
   
   InitializeListHead(&UsedPageListHead);
   KeInitializeSpinLock(&PageListLock);
   InitializeListHead(&FreePageListHead);
   InitializeListHead(&BiosPageListHead);
   
   Reserved = (MemorySizeInPages * sizeof(PHYSICAL_PAGE)) / PAGESIZE;
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
	     Status = MmCreateVirtualMapping(NULL,
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
   
   i = 1;
   if ((ULONG)FirstPhysKernelAddress < 0xa0000)
     {
	MmStats.NrFreePages += ((ULONG)FirstPhysKernelAddress/PAGESIZE);
	for (; i<((ULONG)FirstPhysKernelAddress/PAGESIZE); i++)
	  {
	     MmPageArray[i].Flags = MM_PHYSICAL_PAGE_FREE;
	     MmPageArray[i].ReferenceCount = 0;
	     KeInitializeEvent(&MmPageArray[i].Event,
			       NotificationEvent,
			       FALSE);
	     InsertTailList(&FreePageListHead,
			    &MmPageArray[i].ListEntry);
	  }
	MmStats.NrSystemPages += 
	  ((((ULONG)LastPhysKernelAddress) / PAGESIZE) - i);
	for (; i<((ULONG)LastPhysKernelAddress / PAGESIZE); i++)
	  {
	     MmPageArray[i].Flags = MM_PHYSICAL_PAGE_USED;
	     MmPageArray[i].ReferenceCount = 1;
	     KeInitializeEvent(&MmPageArray[i].Event,
			       NotificationEvent,
			       FALSE);
	     InsertTailList(&UsedPageListHead,
			    &MmPageArray[i].ListEntry);
	  }
	MmStats.NrFreePages += ((0xa0000/PAGESIZE) - i);
	for (; i<(0xa0000/PAGESIZE); i++)
	  {
	     MmPageArray[i].Flags = MM_PHYSICAL_PAGE_FREE;
	     MmPageArray[i].ReferenceCount = 0;
	     KeInitializeEvent(&MmPageArray[i].Event,
			       NotificationEvent,
			       FALSE);
	     InsertTailList(&FreePageListHead,
			    &MmPageArray[i].ListEntry);
	  }
	MmStats.NrReservedPages += ((0x100000/PAGESIZE) - i);
	for (; i<(0x100000 / PAGESIZE); i++)
	  {
	     MmPageArray[i].Flags = MM_PHYSICAL_PAGE_BIOS;
	     MmPageArray[i].ReferenceCount = 1;
	     KeInitializeEvent(&MmPageArray[i].Event,
			       NotificationEvent,
			       FALSE);
	     InsertTailList(&BiosPageListHead,
			    &MmPageArray[i].ListEntry);
	  }
     }
   else
     {
	MmStats.NrFreePages += (0xa0000 / PAGESIZE);	  
	for (; i<(0xa0000 / PAGESIZE); i++)
	  {
	     MmPageArray[i].Flags = MM_PHYSICAL_PAGE_FREE;
	     MmPageArray[i].ReferenceCount = 0;
	     KeInitializeEvent(&MmPageArray[i].Event,
			       NotificationEvent,
			       FALSE);
	     InsertTailList(&FreePageListHead,
			    &MmPageArray[i].ListEntry);
	  }
	MmStats.NrReservedPages += (0x60000 / PAGESIZE);
	for (; i<(0x100000 / PAGESIZE); i++)
	  {
	     MmPageArray[i].Flags = MM_PHYSICAL_PAGE_BIOS;
	     MmPageArray[i].ReferenceCount = 1;
	     KeInitializeEvent(&MmPageArray[i].Event,
			       NotificationEvent,
			       FALSE);
	     InsertTailList(&BiosPageListHead,
			    &MmPageArray[i].ListEntry);
	  }
	MmStats.NrFreePages += (((ULONG)FirstPhysKernelAddress/PAGESIZE) - i);
	for (; i<((ULONG)FirstPhysKernelAddress/PAGESIZE); i++)
	  {
	     MmPageArray[i].Flags = MM_PHYSICAL_PAGE_FREE;
	     MmPageArray[i].ReferenceCount = 0;
	     KeInitializeEvent(&MmPageArray[i].Event,
			       NotificationEvent,
			       FALSE);
	     InsertTailList(&FreePageListHead,
			    &MmPageArray[i].ListEntry);
	  }
	MmStats.NrSystemPages += 
	  (((ULONG)LastPhysKernelAddress/PAGESIZE) - i);
	for (; i<((ULONG)LastPhysKernelAddress/PAGESIZE); i++)
	  {
	     MmPageArray[i].Flags = MM_PHYSICAL_PAGE_USED;
	     MmPageArray[i].ReferenceCount = 1;
	     KeInitializeEvent(&MmPageArray[i].Event,
			       NotificationEvent,
			       FALSE);
	     InsertTailList(&UsedPageListHead,
			    &MmPageArray[i].ListEntry);
	  }
     }
   
   MmStats.NrFreePages += (MemorySizeInPages - i);
   for (; i<MemorySizeInPages; i++)
     {
	MmPageArray[i].Flags = MM_PHYSICAL_PAGE_FREE;
	MmPageArray[i].ReferenceCount = 0;
	KeInitializeEvent(&MmPageArray[i].Event,
			  NotificationEvent,
			  FALSE);
	InsertTailList(&FreePageListHead,
		       &MmPageArray[i].ListEntry);
     }  
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

SWAPENTRY MmGetSavedSwapEntryPage(PVOID PhysicalAddress)
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
   
   MmStats.NrFreePages++;
   MmStats.NrSystemPages--;
   
   MmPageArray[Start].ReferenceCount--;
   if (MmPageArray[Start].ReferenceCount == 0)
     {
	RemoveEntryList(&MmPageArray[Start].ListEntry);
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
	InsertTailList(&FreePageListHead, &MmPageArray[Start].ListEntry);
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


PVOID MmAllocPage(SWAPENTRY SavedSwapEntry)
{
   ULONG offset;
   PLIST_ENTRY ListEntry;
   PPHYSICAL_PAGE PageDescriptor;
   
   DPRINT("MmAllocPage()\n");
   
   ListEntry = ExInterlockedRemoveHeadList(&FreePageListHead, 
					   &PageListLock);
   DPRINT("ListEntry %x\n",ListEntry);
   if (ListEntry == NULL)
     {
	DPRINT("MmAllocPage(): Out of memory\n");
	return(NULL);
     }
   PageDescriptor = CONTAINING_RECORD(ListEntry, PHYSICAL_PAGE, ListEntry);
   DPRINT("PageDescriptor %x\n",PageDescriptor);
   if (PageDescriptor->Flags != MM_PHYSICAL_PAGE_FREE)
     {
	DbgPrint("Got non-free page from freelist\n");
	KeBugCheck(0);
     }
   PageDescriptor->Flags = MM_PHYSICAL_PAGE_USED;
   PageDescriptor->ReferenceCount = 1;
   PageDescriptor->LockCount = 0;
   PageDescriptor->SavedSwapEntry = SavedSwapEntry;
   ExInterlockedInsertTailList(&UsedPageListHead, ListEntry, 
			       &PageListLock);
   
   DPRINT("PageDescriptor %x MmPageArray %x\n", PageDescriptor, MmPageArray);
   offset = (ULONG)((ULONG)PageDescriptor - (ULONG)MmPageArray);
   DPRINT("offset %x\n",offset);
   offset = offset / sizeof(PHYSICAL_PAGE) * PAGESIZE;
   DPRINT("offset %x\n",offset);
   
   MmStats.NrSystemPages++;
   MmStats.NrFreePages--;
   
   DPRINT("MmAllocPage() = %x\n",offset);
   return((PVOID)offset);
}

PVOID MmMustAllocPage(SWAPENTRY SavedSwapEntry)
{
   PVOID Page;
   
   Page = MmAllocPage(SavedSwapEntry);
   if (Page == NULL)
     {
	KeBugCheck(0);
	return(NULL);
     }
   
   return(Page);
}

PVOID MmAllocPageMaybeSwap(SWAPENTRY SavedSwapEntry)
{
   PVOID Page;
   
   Page = MmAllocPage(SavedSwapEntry);
   while (Page == NULL)
     {
	MmWaitForFreePages();
	Page = MmAllocPage(SavedSwapEntry);
     };
   return(Page);
}

NTSTATUS MmWaitForPage(PVOID PhysicalAddress)
{
   NTSTATUS Status;
   ULONG Start;
   
   Start = (ULONG)PhysicalAddress / PAGESIZE;
   Status = KeWaitForSingleObject(&MmPageArray[Start].Event,
				  UserRequest,
				  KernelMode,
				  FALSE,
				  NULL);
   return(Status);
}

VOID MmClearWaitPage(PVOID PhysicalAddress)
{
   ULONG Start;
   
   Start = (ULONG)PhysicalAddress / PAGESIZE;
   
   KeClearEvent(&MmPageArray[Start].Event);
}

VOID MmSetWaitPage(PVOID PhysicalAddress)
{
   ULONG Start;
   
   Start = (ULONG)PhysicalAddress / PAGESIZE;
   
   KeSetEvent(&MmPageArray[Start].Event,
	      IO_DISK_INCREMENT,
	      FALSE);
}
