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
} PHYSICAL_PAGE, *PPHYSICAL_PAGE;

/* GLOBALS ****************************************************************/

static PPHYSICAL_PAGE MmPageArray;

static LIST_ENTRY UsedPageListHead;
static KSPIN_LOCK UsedPageListLock;
static LIST_ENTRY FreePageListHead;
static KSPIN_LOCK FreePageListLock;
static LIST_ENTRY BiosPageListHead;
static KSPIN_LOCK BiosPageListLock;

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
   KeInitializeSpinLock(&UsedPageListLock);
   InitializeListHead(&FreePageListHead);
   KeInitializeSpinLock(&FreePageListLock);
   InitializeListHead(&BiosPageListHead);
   KeInitializeSpinLock(&BiosPageListLock);
   
   Reserved = (MemorySizeInPages * sizeof(PHYSICAL_PAGE)) / PAGESIZE;
   MmPageArray = (PHYSICAL_PAGE *)LastKernelAddress;
   
   DPRINT("Reserved %d\n", Reserved);
   
   MiNrFreePages = 0;
   MiNrUsedPages = 0;
   
   i = 1;
   if ((ULONG)FirstPhysKernelAddress < 0xa0000)
     {
	MiNrFreePages = MiNrFreePages + 
	  ((ULONG)FirstPhysKernelAddress/PAGESIZE);
	for (; i<((ULONG)FirstPhysKernelAddress/PAGESIZE); i++)
	  {
	     MmPageArray[i].Flags = PHYSICAL_PAGE_FREE;
	     InsertTailList(&FreePageListHead,
			    &MmPageArray[i].ListEntry);
	  }
	MiNrUsedPages = MiNrUsedPages +
	  (((0xa0000) / PAGESIZE) - i);
	for (; i<(0xa0000 / PAGESIZE); i++)
	  {
	     MmPageArray[i].Flags = PHYSICAL_PAGE_INUSE;
	     InsertTailList(&UsedPageListHead,
			    &MmPageArray[i].ListEntry);
	  }
	for (; i<(0x100000 / PAGESIZE); i++)
	  {
	     MmPageArray[i].Flags = PHYSICAL_PAGE_BIOS;
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
	     InsertTailList(&FreePageListHead,
			    &MmPageArray[i].ListEntry);
	  }
	for (; i<(0x100000 / PAGESIZE); i++)
	  {
	     MmPageArray[i].Flags = PHYSICAL_PAGE_BIOS;
	     InsertTailList(&BiosPageListHead,
			    &MmPageArray[i].ListEntry);
	  }
	MiNrFreePages = MiNrFreePages + 
	  (((ULONG)FirstPhysKernelAddress/PAGESIZE) - i);
	for (; i<((ULONG)FirstPhysKernelAddress/PAGESIZE); i++)
	  {
	     MmPageArray[i].Flags = PHYSICAL_PAGE_FREE;
	     InsertTailList(&FreePageListHead,
			    &MmPageArray[i].ListEntry);
	  }
	MiNrUsedPages = MiNrUsedPages +
	  (((ULONG)LastPhysKernelAddress/PAGESIZE) - i);
	for (; i<((ULONG)LastPhysKernelAddress/PAGESIZE); i++)
	  {
	     MmPageArray[i].Flags = PHYSICAL_PAGE_INUSE;
	     InsertTailList(&UsedPageListHead,
			    &MmPageArray[i].ListEntry);
	  }
     }
   
   MiNrFreePages = MiNrFreePages + (MemorySizeInPages - i);
   for (; i<MemorySizeInPages; i++)
     {
	MmPageArray[i].Flags = PHYSICAL_PAGE_FREE;
	InsertTailList(&FreePageListHead,
		       &MmPageArray[i].ListEntry);
     }  
   DPRINT("\nMmInitializePageList() = %x\n",
	  LastKernelAddress + Reserved * PAGESIZE);
   return((PVOID)(LastKernelAddress + Reserved * PAGESIZE));
}

VOID MmFreePage(PVOID PhysicalAddress, ULONG Nr)
{
   ULONG i;
   ULONG Start = (ULONG)PhysicalAddress / PAGESIZE;
   KIRQL oldIrql;
   
   DPRINT("MmFreePage(PhysicalAddress %x, Nr %x)\n", PhysicalAddress, Nr);
   
   assert(((ULONG)PhysicalAddress) <= 0x400000);
   
   MiNrFreePages = MiNrFreePages + Nr;
   MiNrUsedPages = MiNrUsedPages - Nr;
   
   for (i=0; i<Nr; i++)
     {
	KeAcquireSpinLock(&UsedPageListLock, &oldIrql);
	RemoveEntryList(&MmPageArray[Start + i].ListEntry);
   	MmPageArray[Start + i].Flags = PHYSICAL_PAGE_FREE;
	KeReleaseSpinLock(&UsedPageListLock, oldIrql);
	
	ExInterlockedInsertTailList(&FreePageListHead, 
				    &MmPageArray[Start + i].ListEntry,
				    &FreePageListLock);
     }
}


PVOID MmAllocPage(VOID)
{
   ULONG offset;
   PLIST_ENTRY ListEntry;
   PPHYSICAL_PAGE PageDescriptor;
   
   DPRINT("MmAllocPage()\n");
   
   ListEntry = ExInterlockedRemoveHeadList(&FreePageListHead, 
					   &FreePageListLock);
   DPRINT("ListEntry %x\n",ListEntry);
   if (ListEntry == NULL)
     {
	DbgPrint("MmAllocPage(): Out of memory\n");
	KeBugCheck(0);
     }
   PageDescriptor = CONTAINING_RECORD(ListEntry, PHYSICAL_PAGE, ListEntry);
   DPRINT("PageDescriptor %x\n",PageDescriptor);
   PageDescriptor->Flags = PHYSICAL_PAGE_INUSE;
   ExInterlockedInsertTailList(&UsedPageListHead, ListEntry, 
			       &UsedPageListLock);
   
   DPRINT("PageDescriptor %x MmPageArray %x\n", PageDescriptor, MmPageArray);
   offset = (ULONG)((ULONG)PageDescriptor - (ULONG)MmPageArray);
   DPRINT("offset %x\n",offset);
   offset = offset / sizeof(PHYSICAL_PAGE) * PAGESIZE;
   DPRINT("offset %x\n",offset);
   
   MiNrUsedPages = MiNrUsedPages + 1;
   MiNrFreePages = MiNrFreePages - 1;
   
   assert(offset <= 0x400000);
   
   DPRINT("MmAllocPage() = %x\n",offset);
   return((PVOID)offset);
}
