/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/mm/marea.c
 * PURPOSE:         Implements memory areas
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <internal/mm.h>
#include <internal/hal/page.h>

#define NDEBUG
#include <internal/debug.h>

/* GLOBALS *******************************************************************/

static LIST_ENTRY SystemAreaList = {NULL,NULL};
static KSPIN_LOCK SystemAreaListLock = {0,};

/* FUNCTIONS *****************************************************************/

VOID MmDumpMemoryAreas(VOID)
{
   PLIST_ENTRY current_entry;
   MEMORY_AREA* current;
   PLIST_ENTRY ListHead = &SystemAreaList;
   ULONG i;
   
   current_entry = ListHead->Flink;
   i=0;
   while (current_entry!=ListHead)
     {
	current = CONTAINING_RECORD(current_entry,MEMORY_AREA,Entry);
	DPRINT("Base %x Length %x End %x Attributes %x Flink %x\n",
	       current->BaseAddress,current->Length,
	       current->BaseAddress+current->Length,current->Attributes,
	       current->Entry.Flink);
	current_entry = current_entry->Flink;
	i++;
	if (i>6)
	  {
	     CHECKPOINT;
	     for(;;);
	  }
     }
   CHECKPOINT;
}

VOID MmLockMemoryAreaList(ULONG Address, PKIRQL oldlvl)
{
   if (Address >= KERNEL_BASE)     
     {
	KeAcquireSpinLock(&SystemAreaListLock,oldlvl);
     }
   else
     {
	PKPROCESS CurrentProcess = KeGetCurrentProcess();
	
	KeAcquireSpinLock(&(CurrentProcess->SpinLock),oldlvl);
     }
}

VOID MmUnlockMemoryAreaList(ULONG Address, PKIRQL oldlvl)
{
   if (Address >= KERNEL_BASE)     
     {
	KeReleaseSpinLock(&SystemAreaListLock,*oldlvl);
     }
   else
     {
	PKPROCESS CurrentProcess = KeGetCurrentProcess();
	KeReleaseSpinLock(&(CurrentProcess->SpinLock),*oldlvl);
     }

}


static PLIST_ENTRY MmGetRelatedListHead(ULONG BaseAddress)
{
   if (BaseAddress >= KERNEL_BASE)
     {
	return(&SystemAreaList);
     }
   else
     {
	PKPROCESS CurrentProcess = KeGetCurrentProcess();
	return(&(CurrentProcess->MemoryAreaList));
     }
}

static MEMORY_AREA* MmInternalOpenMemoryAreaByAddress(PLIST_ENTRY ListHead, 
						      ULONG Address)
{
   PLIST_ENTRY current_entry;
   MEMORY_AREA* current;

   current_entry = ListHead->Flink;
   while (current_entry!=ListHead)
     {
	current = CONTAINING_RECORD(current_entry,MEMORY_AREA,Entry);
	if (current->BaseAddress <= Address &&
	    (current->BaseAddress + current->Length) > Address)
	  {
	     return(current);
	  }
	if (current->BaseAddress > Address)
	  {
	     return(NULL);
	  }
	current_entry = current_entry->Flink;
     }
   return(NULL);
}


MEMORY_AREA* MmInternalOpenMemoryAreaByRegion(PLIST_ENTRY ListHead, 
					      ULONG Address, 
					      ULONG Length)
{
   PLIST_ENTRY current_entry;
   MEMORY_AREA* current;
   ULONG Extent;
   
   DPRINT("MmInternalOpenMemoryAreaByRegion()\n",0);
   
   MmDumpMemoryAreas();
   
   current_entry = ListHead->Flink;
   while (current_entry!=ListHead)
     {
	current = CONTAINING_RECORD(current_entry,MEMORY_AREA,Entry);
	if (current->BaseAddress >= Address &&
	    current->BaseAddress < (Address+Length))
	  {
	     DPRINT("Finished MmInternalOpenMemoryAreaByRegion()\n",0);
	     return(current);
	  }
	Extent = current->BaseAddress + current->Length;
	if (Extent >= Address &&
	    Extent < (Address+Length))
	  {
	     DPRINT("Finished MmInternalOpenMemoryAreaByRegion()\n",0);
	     return(current);
	  }
	if (current->BaseAddress <= Address &&
	    Extent >= (Address+Length))
	  {
	     DPRINT("Finished MmInternalOpenMemoryAreaByRegion()\n",0);
	     return(current);
	  }
	if (current->BaseAddress >= (Address+Length))
	  {
	     DPRINT("Finished MmInternalOpenMemoryAreaByRegion()\n",0);
	     return(NULL);
	  }
	current_entry = current_entry->Flink;
     }
   DPRINT("Finished MmInternalOpenMemoryAreaByRegion()\n",0);
   return(NULL);
}

MEMORY_AREA* MmOpenMemoryAreaByRegion(ULONG Address, ULONG Length)
{
   KIRQL oldlvl;
   MEMORY_AREA* Result;
   PLIST_ENTRY ListHead;
   
   MmLockMemoryAreaList(Address,&oldlvl);
   ListHead = MmGetRelatedListHead(Address);   
   Result = MmInternalOpenMemoryAreaByRegion(ListHead,Address,Length);
   MmUnlockMemoryAreaList(Address,&oldlvl);
   return(Result);
}


MEMORY_AREA* MmOpenMemoryAreaByRegionWithoutLock(ULONG Address, ULONG Length)
{
   MEMORY_AREA* Result;
   PLIST_ENTRY ListHead;
   
   ListHead = MmGetRelatedListHead(Address);   
   Result = MmInternalOpenMemoryAreaByRegion(ListHead,Address,Length);
   return(Result);
}

MEMORY_AREA* MmOpenMemoryAreaByAddress(ULONG Address)
{
   KIRQL oldlvl;
   MEMORY_AREA* Result;
   PLIST_ENTRY ListHead;
   
   DPRINT("MmOpenMemoryAreaByAddress(Address %x)\n",Address);
   
   MmLockMemoryAreaList(Address,&oldlvl);
   ListHead = MmGetRelatedListHead(Address);
   Result = MmInternalOpenMemoryAreaByAddress(ListHead,Address);
   MmUnlockMemoryAreaList(Address,&oldlvl);
   return(Result);
}

MEMORY_AREA* MmOpenMemoryAreaByAddressWithoutLock(ULONG Address)
{
   MEMORY_AREA* Result;
   PLIST_ENTRY ListHead;
   
   ListHead = MmGetRelatedListHead(Address);   
   Result = MmInternalOpenMemoryAreaByAddress(ListHead,Address);
   return(Result);
}

static VOID MmInsertMemoryAreaWithoutLock(MEMORY_AREA* marea)
{
   PLIST_ENTRY ListHead;
   PLIST_ENTRY current_entry;
   PLIST_ENTRY inserted_entry = &(marea->Entry);
   MEMORY_AREA* current;
   MEMORY_AREA* next;   
   
   DPRINT("MmInsertMemoryAreaWithoutLock(marea %x)\n",marea);
   DPRINT("marea->BaseAddress %x\n",marea->BaseAddress);
   DPRINT("marea->Length %x\n",marea->Length);
   
   ListHead=MmGetRelatedListHead(marea->BaseAddress);
   current_entry = ListHead->Flink;
   CHECKPOINT;
   if (IsListEmpty(ListHead))
     {
	CHECKPOINT;
	InsertHeadList(ListHead,&marea->Entry);
	CHECKPOINT;
	return;
     }
   CHECKPOINT;
   current = CONTAINING_RECORD(current_entry,MEMORY_AREA,Entry);
   CHECKPOINT;
   if (current->BaseAddress > marea->BaseAddress)
     {
	CHECKPOINT;
	InsertHeadList(ListHead,&marea->Entry);
	CHECKPOINT;
	return;
     }
   CHECKPOINT;
   while (current_entry->Flink!=ListHead)
     {
//	CHECKPOINT;
	current = CONTAINING_RECORD(current_entry,MEMORY_AREA,Entry);
	next = CONTAINING_RECORD(current_entry->Flink,MEMORY_AREA,Entry);
	if (current->BaseAddress < marea->BaseAddress &&
	    current->Entry.Flink==ListHead)
	  {
	     current_entry->Flink = inserted_entry;
	     inserted_entry->Flink=&ListHead;
	     inserted_entry->Blink=current_entry;
	     return;
	  }
	if (current->BaseAddress < marea->BaseAddress &&
	    next->BaseAddress > marea->BaseAddress)
	  {	     
	     inserted_entry->Flink = current_entry->Flink;
	     inserted_entry->Blink = current_entry->Blink;
	     inserted_entry->Flink->Blink = inserted_entry;
	     current_entry->Flink=inserted_entry;
	     return;
	  }
	current_entry = current_entry->Flink;
     }
   CHECKPOINT;
   InsertTailList(ListHead,inserted_entry);
}

static ULONG MmFindGapWithoutLock(KPROCESSOR_MODE Mode, ULONG Length)
{
   PLIST_ENTRY ListHead;
   PLIST_ENTRY current_entry;
   MEMORY_AREA* current;
   MEMORY_AREA* next;
   ULONG Gap;
   
   DPRINT("MmFindGapWithoutLock(Mode %x Length %x)\n",Mode,Length);
   
   if (Mode == KernelMode)
     {
	ListHead = &SystemAreaList;
     }
   else
     {
	ListHead = &(KeGetCurrentProcess()->MemoryAreaList);
     }
   
   
   current_entry = ListHead->Flink;
   while (current_entry->Flink!=ListHead)
     {
	current = CONTAINING_RECORD(current_entry,MEMORY_AREA,Entry);
	next = CONTAINING_RECORD(current_entry->Flink,MEMORY_AREA,Entry);
	DPRINT("current %x current->BaseAddress %x\n",current,
	       current->BaseAddress);
	DPRINT("current->Length %x\n",current->Length);
	DPRINT("next %x next->BaseAddress %x\n",next,next->BaseAddress);
	Gap = (next->BaseAddress ) -(current->BaseAddress + current->Length);
	DPRINT("Gap %x\n",Gap);
	if (Gap >= Length)
	  {
	     return(current->BaseAddress + current->Length);
	  }
	current_entry = current_entry->Flink;
     }
   current = CONTAINING_RECORD(current_entry,MEMORY_AREA,Entry);
   return(current->BaseAddress + current->Length);
}

NTSTATUS MmInitMemoryAreas(VOID)
{
   DPRINT("MmInitMemoryAreas()\n",0);
   InitializeListHead(&SystemAreaList);
   KeInitializeSpinLock(&SystemAreaListLock);
   return(STATUS_SUCCESS);
}

NTSTATUS MmFreeMemoryArea(PVOID BaseAddress,
			  ULONG Length,
			  BOOLEAN FreePages)
{
   MEMORY_AREA* MemoryArea;
   ULONG i;
   KIRQL oldlvl;
   
   MmLockMemoryAreaList((ULONG)BaseAddress,&oldlvl);
   
   MemoryArea = MmOpenMemoryAreaByAddressWithoutLock((ULONG)BaseAddress);
   if (MemoryArea!=NULL)
     {
	MmUnlockMemoryAreaList((ULONG)BaseAddress,&oldlvl);
	return(STATUS_UNSUCCESSFUL);
     }
   if (FreePages)
     {
	for (i=0;i<=(MemoryArea->Length/PAGESIZE);i++)
	  {
	     free_page(MmGetPhysicalAddress(MemoryArea->BaseAddress+
					    (i*PAGESIZE)).LowPart,1);
	  }
     }
   
   RemoveEntryList(&(MemoryArea->Entry));
   ExFreePool(MemoryArea);
   MmUnlockMemoryAreaList((ULONG)BaseAddress,&oldlvl);
   return(STATUS_SUCCESS);
}

NTSTATUS MmCreateMemoryArea(KPROCESSOR_MODE Mode,
			    ULONG Type,
			    PULONG BaseAddress,
			    ULONG Length,
			    ULONG Attributes,
			    MEMORY_AREA** Result)
{
   KIRQL oldlvl;
   
   DPRINT("MmCreateMemoryArea(Mode %x, Type %d, BaseAddress %x,"
	  "*BaseAddress %x, Length %x, Attributes %x, Result %x)\n",
          Mode,Type,BaseAddress,*BaseAddress,Length,Attributes,Result);
   
   MmLockMemoryAreaList(*BaseAddress,&oldlvl);
   if ((*BaseAddress)==0)
     {
	*BaseAddress = MmFindGapWithoutLock(Mode,Length+(PAGESIZE*2));
	if ((*BaseAddress)==0)
	  {
	     MmUnlockMemoryAreaList(*BaseAddress,&oldlvl);
	     return(STATUS_UNSUCCESSFUL);
	  }
	(*BaseAddress)=(*BaseAddress)+PAGESIZE;
     }
   else
     {
	if (MmOpenMemoryAreaByRegionWithoutLock(*BaseAddress,Length)!=NULL)
	  {
	     MmUnlockMemoryAreaList(*BaseAddress,&oldlvl);
	     return(STATUS_UNSUCCESSFUL);
	  }
     }
   
   *Result = ExAllocatePool(NonPagedPool,sizeof(MEMORY_AREA));
   RtlZeroMemory(*Result,sizeof(MEMORY_AREA));
   (*Result)->Type=Type;
   (*Result)->BaseAddress=*BaseAddress;
   (*Result)->Length=Length;
   (*Result)->Attributes=Attributes;
   DPRINT("&SystemAreaList %x ",&SystemAreaList);
   DPRINT("SystemAreaList.Flink %x ",SystemAreaList.Flink);
   MmInsertMemoryAreaWithoutLock(*Result);
   MmUnlockMemoryAreaList(*BaseAddress,&oldlvl);
   DPRINT("SystemAreaList.Flink %x ",SystemAreaList.Flink);
   DPRINT("(*Result)->Entry.Flink %x\n",(*Result)->Entry.Flink);
   MmDumpMemoryAreas();
   return(STATUS_SUCCESS);
}
