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

#include <internal/stddef.h>
#include <ddk/ntddk.h>
#include <internal/mm.h>
#include <internal/mmhal.h>
#include <internal/ntoskrnl.h>
#include <internal/bitops.h>
#include <internal/i386/io.h>

#define NDEBUG
#include <internal/debug.h>

/* TYPES *******************************************************************/

#define PHYSICAL_PAGE_FREE    (0x1)
#define PHYSICAL_PAGE_INUSE   (0x2)
#define PHYSICAL_PAGE_BIOS    (0x4)

typedef struct _PHYSICAL_PAGE
{
   ULONG Flags;
   LIST_ENTRY ListEntry;
   ULONG ReferenceCount;
   KEVENT Event;
} PHYSICAL_PAGE, *PPHYSICAL_PAGE;

/* GLOBALS ****************************************************************/

static PPHYSICAL_PAGE MmPageArray;

static LIST_ENTRY UsedPageListHead;
static KSPIN_LOCK PageListLock;
static LIST_ENTRY FreePageListHead;
static LIST_ENTRY BiosPageListHead;

ULONG MiNrFreePages;
ULONG MiNrUsedPages;

/* FUNCTIONS *************************************************************/

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
     
   MiNrFreePages = 0;
   MiNrUsedPages = 0;
   
   for (i = 0; i < Reserved; i++)
     {
	if (!MmIsPagePresent(NULL, 
			     (PVOID)((ULONG)MmPageArray + (i * PAGESIZE))))
	  {
	     MmSetPage(NULL,
		       (PVOID)((ULONG)MmPageArray + (i * PAGESIZE)),
		       PAGE_READWRITE,
		       (ULONG)(LastPhysKernelAddress - (i * PAGESIZE)));
	  }
     }
   
   i = 1;
   if ((ULONG)FirstPhysKernelAddress < 0xa0000)
     {
	MiNrFreePages = MiNrFreePages + 
	  ((ULONG)FirstPhysKernelAddress/PAGESIZE);
	for (; i<((ULONG)FirstPhysKernelAddress/PAGESIZE); i++)
	  {
	     MmPageArray[i].Flags = PHYSICAL_PAGE_FREE;
	     MmPageArray[i].ReferenceCount = 0;
	     KeInitializeEvent(&MmPageArray[i].Event,
			       NotificationEvent,
			       FALSE);
	     InsertTailList(&FreePageListHead,
			    &MmPageArray[i].ListEntry);
	  }
	MiNrUsedPages = MiNrUsedPages +
	  ((((ULONG)LastPhysKernelAddress) / PAGESIZE) - i);
	for (; i<((ULONG)LastPhysKernelAddress / PAGESIZE); i++)
	  {
	     MmPageArray[i].Flags = PHYSICAL_PAGE_INUSE;
	     MmPageArray[i].ReferenceCount = 1;
	     KeInitializeEvent(&MmPageArray[i].Event,
			       NotificationEvent,
			       FALSE);
	     InsertTailList(&UsedPageListHead,
			    &MmPageArray[i].ListEntry);
	  }
	MiNrFreePages = MiNrFreePages + 
	  ((0xa0000/PAGESIZE) - i);
	for (; i<(0xa0000/PAGESIZE); i++)
	  {
	     MmPageArray[i].Flags = PHYSICAL_PAGE_FREE;
	     MmPageArray[i].ReferenceCount = 0;
	     KeInitializeEvent(&MmPageArray[i].Event,
			       NotificationEvent,
			       FALSE);
	     InsertTailList(&FreePageListHead,
			    &MmPageArray[i].ListEntry);
	  }
	for (; i<(0x100000 / PAGESIZE); i++)
	  {
	     MmPageArray[i].Flags = PHYSICAL_PAGE_BIOS;
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
	MiNrFreePages = MiNrFreePages + (0xa0000 / PAGESIZE);	  
	for (; i<(0xa0000 / PAGESIZE); i++)
	  {
	     MmPageArray[i].Flags = PHYSICAL_PAGE_FREE;
	     MmPageArray[i].ReferenceCount = 0;
	     KeInitializeEvent(&MmPageArray[i].Event,
			       NotificationEvent,
			       FALSE);
	     InsertTailList(&FreePageListHead,
			    &MmPageArray[i].ListEntry);
	  }
	for (; i<(0x100000 / PAGESIZE); i++)
	  {
	     MmPageArray[i].Flags = PHYSICAL_PAGE_BIOS;
	     MmPageArray[i].ReferenceCount = 1;
	     KeInitializeEvent(&MmPageArray[i].Event,
			       NotificationEvent,
			       FALSE);
	     InsertTailList(&BiosPageListHead,
			    &MmPageArray[i].ListEntry);
	  }
	MiNrFreePages = MiNrFreePages + 
	  (((ULONG)FirstPhysKernelAddress/PAGESIZE) - i);
	for (; i<((ULONG)FirstPhysKernelAddress/PAGESIZE); i++)
	  {
	     MmPageArray[i].Flags = PHYSICAL_PAGE_FREE;
	     MmPageArray[i].ReferenceCount = 0;
	     KeInitializeEvent(&MmPageArray[i].Event,
			       NotificationEvent,
			       FALSE);
	     InsertTailList(&FreePageListHead,
			    &MmPageArray[i].ListEntry);
	  }
	MiNrUsedPages = MiNrUsedPages +
	  (((ULONG)LastPhysKernelAddress/PAGESIZE) - i);
	for (; i<((ULONG)LastPhysKernelAddress/PAGESIZE); i++)
	  {
	     MmPageArray[i].Flags = PHYSICAL_PAGE_INUSE;
	     MmPageArray[i].ReferenceCount = 1;
	     KeInitializeEvent(&MmPageArray[i].Event,
			       NotificationEvent,
			       FALSE);
	     InsertTailList(&UsedPageListHead,
			    &MmPageArray[i].ListEntry);
	  }
     }
   
   MiNrFreePages = MiNrFreePages + (MemorySizeInPages - i);
   for (; i<MemorySizeInPages; i++)
     {
	MmPageArray[i].Flags = PHYSICAL_PAGE_FREE;
	MmPageArray[i].ReferenceCount = 0;
	KeInitializeEvent(&MmPageArray[i].Event,
			  NotificationEvent,
			  FALSE);
	InsertTailList(&FreePageListHead,
		       &MmPageArray[i].ListEntry);
     }  
   return((PVOID)LastKernelAddress);
}

VOID MmReferencePage(PVOID PhysicalAddress)
{
   ULONG Start = (ULONG)PhysicalAddress / PAGESIZE;
   KIRQL oldIrql;
   
   DPRINT("MmReferencePage(PhysicalAddress %x)\n", PhysicalAddress);
   
   KeAcquireSpinLock(&PageListLock, &oldIrql);
   MmPageArray[Start].ReferenceCount++;
   KeReleaseSpinLock(&PageListLock, oldIrql);
}

VOID MmDereferencePage(PVOID PhysicalAddress)
{
   ULONG Start = (ULONG)PhysicalAddress / PAGESIZE;
   KIRQL oldIrql;
   
   DPRINT("MmDereferencePage(PhysicalAddress %x)\n", PhysicalAddress);
   
   if (((ULONG)PhysicalAddress) > 0x400000)
     {
	DbgPrint("MmFreePage() failed with %x\n", PhysicalAddress);
	KeBugCheck(0);
     }
   
   KeAcquireSpinLock(&PageListLock, &oldIrql);
   
   MiNrFreePages = MiNrFreePages + 1;
   MiNrUsedPages = MiNrUsedPages - 1;
   
   MmPageArray[Start].ReferenceCount--;
   if (MmPageArray[Start].ReferenceCount == 0)
     {
	RemoveEntryList(&MmPageArray[Start].ListEntry);
	MmPageArray[Start].Flags = PHYSICAL_PAGE_FREE;
	InsertTailList(&FreePageListHead, &MmPageArray[Start].ListEntry);
     }
   KeReleaseSpinLock(&PageListLock, oldIrql);
}


PVOID MmAllocPage(VOID)
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
	DbgPrint("MmAllocPage(): Out of memory\n");
	KeBugCheck(0);
     }
   PageDescriptor = CONTAINING_RECORD(ListEntry, PHYSICAL_PAGE, ListEntry);
   DPRINT("PageDescriptor %x\n",PageDescriptor);
   PageDescriptor->Flags = PHYSICAL_PAGE_INUSE;
   PageDescriptor->ReferenceCount = 1;
   ExInterlockedInsertTailList(&UsedPageListHead, ListEntry, 
			       &PageListLock);
   
   DPRINT("PageDescriptor %x MmPageArray %x\n", PageDescriptor, MmPageArray);
   offset = (ULONG)((ULONG)PageDescriptor - (ULONG)MmPageArray);
   DPRINT("offset %x\n",offset);
   offset = offset / sizeof(PHYSICAL_PAGE) * PAGESIZE;
   DPRINT("offset %x\n",offset);
   
   MiNrUsedPages = MiNrUsedPages + 1;
   MiNrFreePages = MiNrFreePages - 1;
   
   if (offset > 0x400000)
     {
	DbgPrint("Failed in MmAllocPage() with offset %x\n", offset);
	KeBugCheck(0);
     }
   
   DPRINT("MmAllocPage() = %x\n",offset);
   return((PVOID)offset);
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
