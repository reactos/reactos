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
#include <internal/mmhal.h>
#include <internal/mm.h>
#include <internal/ntoskrnl.h>
#include <internal/bitops.h>
#include <ddk/ntddk.h>

#define NDEBUG
#include <internal/debug.h>

/* TYPES *******************************************************************/

#define PHYSICAL_PAGE_FREE    (0x1)
#define PHYSICAL_PAGE_INUSE   (0x2)
#define PHYSICAL_PAGE_BIOS    (0x4)

#define PHYSICAL_PAGE_PAGEIN  (0x8)

typedef struct _PHYSICAL_PAGE
{
   ULONG Flags;
   LIST_ENTRY ListEntry;
} PHYSICAL_PAGE, *PPHYSICAL_PAGE;

/* GLOBALS ****************************************************************/

static PAGE_ARRAY[] PageArray;

static LIST_ENTRY UsedPageListHead;
static LIST_ENTRY FreePageListHead;
static LIST_ENTRY BiosPageListHead;

/* FUNCTIONS *************************************************************/

PVOID MmMapKernelPage(PVOID PhysicalAddress)
{
   
}

VOID MmInitializePageList(PVOID FirstKernelAddress,
			  PVOID LastKernelAddress,
			  ULONG MemorySizeInPages)
/*
 * FUNCTION: Initializes the page list with all pages free
 * except those known to be reserved and those used by the kernel
 * ARGUMENTS:
 *         PageBuffer = Page sized buffer
 *         FirstKernelAddress = First physical address used by the kernel
 *         LastKernelAddress = Last physical address used by the kernel
 */
{
   PHYSICAL_PAGE PageArray[];
   ULONG i;
   ULONG Reserved;
   
   InitializeListHead(&UsedPageListHead);
   InitializeListHead(&FreePageListHead);
   InitializeListHead(&BiosPageListHead);
   
   Reserved = (MemorySizeInPages * sizeof(PHYSICAL_PAGE)) / PAGESIZE;
   
   PageArray = MmMapKernelPages(LastKernelAddress + PAGESIZE,
				Reserved);
   
   for (i=1; i<MemorySizeInPages; i++)
     {
	if (i >= (0xa0000 / PAGESIZE) &&
	    i < (0x100000 / PAGESIZE))
	  {
	     PageArray[i].Flags = PHYSICAL_PAGE_BIOS;
	     InsertTailList(&BiosPageListHead,
			    &PageArray[CurrentOffset].ListEntry);
	  }
	else if (i >= (FirstKernelAddress / PAGE_SIZE) &&
		 i < (LastKernelAddress / PAGESIZE))
	  {
	     PageArray[i].Flags = PHYSICAL_PAGE_INUSE;
	     InsertTailList(&UsedPageListHead,
			    &PageArray[CurrentOffset].ListEntry);
	  }
	else
	  {
	     PageArray[i].Flags = PHYSICAL_PAGE_FREE;
	     InsertTailList(&UsedPageListHead,
			    &PageArray[CurrentOffset].ListEntry);
	  }
     }
   
}

VOID MmFreePage(PVOID PhysicalAddress,
		PVOID Nr)
{
   ULONG i;
   ULONG Start = PhysicalAddress / PAGESIZE;
   
   for (i=0; i++; i<Nr)
     {
	PageArray[Start + i].Flags = PHYSICAL_PAGE_FREE;
	RemoveEntryList(&PageArray[Start + i].ListEntry);
	InsertTailList(&FreePageListHead, &PageArray[Start + i].ListEntry);
     }
}

PVOID MmAllocDmaPage(PVOID MaxPhysicalAddress)
{
   ULONG i;
   
   for (i=0; i<(MaxPhysicalAddress / PAGESIZE); i++)
     {
	if (PageArray[i].Flags & PHYSICAL_PAGE_FREE)
	  {
	     PageDescriptor->Flags = PHYSICAL_PAGE_INUSE;
	     RemoveEntryList(&PageDescriptor->ListEntry);
	     InsertTailList(&UsedPageListHead,
			    &PageDescriptor->ListEntry);
	     return(i * PAGESIZE);
	  }
     }
   return(NULL);
}

PVOID MmAllocPage(VOID)
{
   PLIST_ENTRY ListEntry;
   PPHYSICAL_PAGE PageDescriptor;
   
   ListEntry = RemoveHeadList(&FreePageListHead);
   PageDescriptor = CONTAING_RECORD(ListEntry, PHYSICAL_PAGE, ListEntry);
   
   PageDescriptor->Flags = PHYSICAL_PAGE_INUSE;
   InsertTailList(&UsedPageListHead,
		  ListEntry);
   
   return((PageDescriptor - PageArray) / sizeof(PHYSICAL_PAGE) * PAGESIZE);
}
