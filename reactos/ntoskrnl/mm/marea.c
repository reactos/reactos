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
#include <internal/mmhal.h>

#define NDEBUG
#include <internal/debug.h>

/* GLOBALS *******************************************************************/

static LIST_ENTRY SystemAreaList = {NULL,NULL};
static KSPIN_LOCK SystemAreaListLock = {0,};

/* FUNCTIONS *****************************************************************/

VOID MmDumpMemoryAreas(PLIST_ENTRY ListHead)
{
   PLIST_ENTRY current_entry;
   MEMORY_AREA* current;
   
   DbgPrint("MmDumpMemoryAreas()\n");
   
   current_entry = ListHead->Flink;
   while (current_entry!=ListHead)
     {
	current = CONTAINING_RECORD(current_entry,MEMORY_AREA,Entry);
	DbgPrint("Base %x Length %x End %x Attributes %x Flink %x\n",
	       current->BaseAddress,current->Length,
	       current->BaseAddress+current->Length,current->Attributes,
	       current->Entry.Flink);
	current_entry = current_entry->Flink;
     }
   DbgPrint("Finished MmDumpMemoryAreas()\n");
}

VOID MmLockMemoryAreaList(PVOID Address, PKIRQL oldlvl)
{
   if (Address >= (PVOID)KERNEL_BASE)     
     {
	KeAcquireSpinLock(&SystemAreaListLock,oldlvl);
     }
   else
     {
	PKPROCESS CurrentProcess = KeGetCurrentProcess();
	
	KeAcquireSpinLock(&(CurrentProcess->SpinLock),oldlvl);
     }
}

VOID MmUnlockMemoryAreaList(PVOID Address, PKIRQL oldlvl)
{
   if (Address >= (PVOID)KERNEL_BASE)     
     {
	KeReleaseSpinLock(&SystemAreaListLock,*oldlvl);
     }
   else
     {
	PKPROCESS CurrentProcess = KeGetCurrentProcess();
	KeReleaseSpinLock(&(CurrentProcess->SpinLock),*oldlvl);
     }

}

VOID MmLockMemoryAreaListByMode(KPROCESSOR_MODE Mode, PKIRQL oldlvl)
{   
   if (Mode == KernelMode)     
     {
	KeAcquireSpinLock(&SystemAreaListLock,oldlvl);
     }
   else
     {
	PKPROCESS CurrentProcess = KeGetCurrentProcess();
	
	KeAcquireSpinLock(&(CurrentProcess->SpinLock),oldlvl);
     }
}

VOID MmUnlockMemoryAreaListByMode(KPROCESSOR_MODE Mode, PKIRQL oldlvl)
{
   if (Mode == KernelMode)     
     {
	KeReleaseSpinLock(&SystemAreaListLock,*oldlvl);
     }
   else
     {
	PKPROCESS CurrentProcess = KeGetCurrentProcess();
	KeReleaseSpinLock(&(CurrentProcess->SpinLock),*oldlvl);
     }

}


static PLIST_ENTRY MmGetRelatedListHead(PEPROCESS Process, PVOID BaseAddress)
{
   if (BaseAddress >= (PVOID)KERNEL_BASE)
     {
	return(&SystemAreaList);
     }
   else
     {
	return(&(Process->Pcb.MemoryAreaList));
     }
}

static MEMORY_AREA* MmInternalOpenMemoryAreaByAddress(PLIST_ENTRY ListHead, 
						      PVOID Address)
{
   PLIST_ENTRY current_entry;
   MEMORY_AREA* current;

//   MmDumpMemoryAreas();
   
   DPRINT("MmInternalOpenMemoryAreaByAddress(ListHead %x, Address %x)\n",
	  ListHead,Address);
   
   if (ListHead==NULL)
     {
	return(NULL);
     }
   
   current_entry = ListHead->Flink;
   while (current_entry!=ListHead)
     {
	current = CONTAINING_RECORD(current_entry,MEMORY_AREA,Entry);
	if (current->BaseAddress <= Address &&
	    (current->BaseAddress + current->Length) > Address)
	  {
	     DPRINT("%s() = %x\n",__FUNCTION__,current);
	     return(current);
	  }
	if (current->BaseAddress > Address)
	  {
	     DPRINT("%s() = NULL\n",__FUNCTION__);
	     return(NULL);
	  }
	current_entry = current_entry->Flink;
     }
   DPRINT("%s() = NULL\n",__FUNCTION__);
   return(NULL);
}


MEMORY_AREA* MmInternalOpenMemoryAreaByRegion(PLIST_ENTRY ListHead, 
					      PVOID Address, 
					      ULONG Length)
{
   PLIST_ENTRY current_entry;
   MEMORY_AREA* current;
   ULONG Extent;
   
   DPRINT("MmInternalOpenMemoryAreaByRegion(ListHead %x, Address %x, "
	    "Length %x)\n",ListHead,Address,Length);
   
// MmDumpMemoryAreas();
   
   current_entry = ListHead->Flink;
   while (current_entry!=ListHead)
     {
	current = CONTAINING_RECORD(current_entry,MEMORY_AREA,Entry);
	DPRINT("current->BaseAddress %x current->Length %x\n",
	       current->BaseAddress,current->Length);
	if (current->BaseAddress >= Address &&
	    current->BaseAddress <= (Address+Length))
	  {
	     DPRINT("Finished MmInternalOpenMemoryAreaByRegion() = %x\n",
		    current);
	     return(current);
	  }
	Extent = (ULONG)current->BaseAddress + current->Length;
	if (Extent > (ULONG)Address &&
	    Extent < (ULONG)(Address+Length))
	  {
	     DPRINT("Finished MmInternalOpenMemoryAreaByRegion() = %x\n",
		    current);
	     return(current);
	  }
	if (current->BaseAddress <= Address &&
	    Extent >= (ULONG)(Address+Length))
	  {
	     DPRINT("Finished MmInternalOpenMemoryAreaByRegion() = %x\n",
		    current);
	     return(current);
	  }
	if (current->BaseAddress >= (Address+Length))
	  {
	     DPRINT("Finished MmInternalOpenMemoryAreaByRegion()= NULL\n",0);
	     return(NULL);
	  }
	current_entry = current_entry->Flink;
     }
   DPRINT("Finished MmInternalOpenMemoryAreaByRegion() = NULL\n",0);
   return(NULL);
}

MEMORY_AREA* MmOpenMemoryAreaByRegion(PEPROCESS Process, 
				      PVOID Address,
				      ULONG Length)
{
   KIRQL oldlvl;
   MEMORY_AREA* Result;
   PLIST_ENTRY ListHead;
   
   DPRINT("MmOpenMemoryByRegion(Process %x, Address %x, Length %x)\n",
	    Process,Address,Length);
   
   MmLockMemoryAreaList(Address,&oldlvl);
   ListHead = MmGetRelatedListHead(Process,Address);   
   Result = MmInternalOpenMemoryAreaByRegion(ListHead,Address,Length);
   MmUnlockMemoryAreaList(Address,&oldlvl);
   return(Result);
}


MEMORY_AREA* MmOpenMemoryAreaByRegionWithoutLock(PEPROCESS Process,
						 PVOID Address, 
						 ULONG Length)
{
   MEMORY_AREA* Result;
   PLIST_ENTRY ListHead;
   
   ListHead = MmGetRelatedListHead(Process, Address);   
   Result = MmInternalOpenMemoryAreaByRegion(ListHead,Address,Length);
   return(Result);
}

MEMORY_AREA* MmOpenMemoryAreaByAddress(PEPROCESS Process, PVOID Address)
{
   KIRQL oldlvl;
   MEMORY_AREA* Result;
   PLIST_ENTRY ListHead;
   
   DPRINT("MmOpenMemoryAreaByAddress(Address %x)\n",Address);
   
   MmLockMemoryAreaList(Address,&oldlvl);
   ListHead = MmGetRelatedListHead(Process, Address);
   Result = MmInternalOpenMemoryAreaByAddress(ListHead,Address);
   MmUnlockMemoryAreaList(Address,&oldlvl);
   return(Result);
}

MEMORY_AREA* MmOpenMemoryAreaByAddressWithoutLock(PEPROCESS Process, 
						  PVOID Address)
{
   MEMORY_AREA* Result;
   PLIST_ENTRY ListHead;
   
   ListHead = MmGetRelatedListHead(Process, Address);   
   Result = MmInternalOpenMemoryAreaByAddress(ListHead, Address);
   return(Result);
}

static VOID MmInsertMemoryAreaWithoutLock(PEPROCESS Process,
					  MEMORY_AREA* marea)
{
   PLIST_ENTRY ListHead;
   PLIST_ENTRY current_entry;
   PLIST_ENTRY inserted_entry = &(marea->Entry);
   MEMORY_AREA* current;
   MEMORY_AREA* next;   
   
   DPRINT("MmInsertMemoryAreaWithoutLock(marea %x)\n",marea);
   DPRINT("marea->BaseAddress %x\n",marea->BaseAddress);
   DPRINT("marea->Length %x\n",marea->Length);
   
   ListHead=MmGetRelatedListHead(Process,marea->BaseAddress);
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
	assert(current->BaseAddress != marea->BaseAddress);
	assert(next->BaseAddress != marea->BaseAddress);
	if (current->BaseAddress < marea->BaseAddress &&
	    current->Entry.Flink==ListHead)
	  {
	     current_entry->Flink = inserted_entry;
	     inserted_entry->Flink=ListHead;
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

static PVOID MmFindGapWithoutLock(PEPROCESS Process,
				  KPROCESSOR_MODE Mode, ULONG Length)
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
	ListHead = &(Process->Pcb.MemoryAreaList);
     }
   
   
   current_entry = ListHead->Flink;
   while (current_entry->Flink!=ListHead)
     {
	current = CONTAINING_RECORD(current_entry,MEMORY_AREA,Entry);
	next = CONTAINING_RECORD(current_entry->Flink,MEMORY_AREA,Entry);
	DPRINT("current %x current->BaseAddress %x ",current,
	       current->BaseAddress);
	DPRINT("current->Length %x\n",current->Length);
	DPRINT("next %x next->BaseAddress %x ",next,next->BaseAddress);
	Gap = (next->BaseAddress ) -(current->BaseAddress + current->Length);
	DPRINT("Base %x Gap %x\n",current->BaseAddress,Gap);
	if (Gap >= Length)
	  {
	     return(current->BaseAddress + PAGE_ROUND_UP(current->Length));
	  }
	current_entry = current_entry->Flink;
     }
   
   if (current_entry == ListHead)
     {
	assert(Mode==UserMode);
	return((PVOID)MM_LOWEST_USER_ADDRESS);
     }
   
   current = CONTAINING_RECORD(current_entry,MEMORY_AREA,Entry);
   //DbgPrint("current %x returning %x\n",current,current->BaseAddress+
//	    current->Length);
   return(current->BaseAddress + PAGE_ROUND_UP(current->Length));
}

NTSTATUS MmInitMemoryAreas(VOID)
/*
 * FUNCTION: Initialize the memory area list
 */
{
   DPRINT("MmInitMemoryAreas()\n",0);
   InitializeListHead(&SystemAreaList);
   KeInitializeSpinLock(&SystemAreaListLock);
   return(STATUS_SUCCESS);
}

NTSTATUS MmFreeMemoryArea(PEPROCESS Process,
			  PVOID BaseAddress,
			  ULONG Length,
			  BOOLEAN FreePages)
{
   MEMORY_AREA* MemoryArea;
   ULONG i;
   KIRQL oldlvl;
   
   DPRINT("MmFreeMemoryArea(Process %x, BaseAddress %x, Length %x,"
          "FreePages %d)\n",Process,BaseAddress,Length,FreePages);			    
   
   MmLockMemoryAreaList(BaseAddress, &oldlvl);
   
   MemoryArea = MmOpenMemoryAreaByAddressWithoutLock(Process,
						     BaseAddress);
   if (MemoryArea==NULL)
     {
	MmUnlockMemoryAreaList(BaseAddress, &oldlvl);
	return(STATUS_UNSUCCESSFUL);
     }
   if (FreePages)
     {
	for (i=0;i<=(MemoryArea->Length/PAGESIZE);i++)
	  {
	     free_page(GET_LARGE_INTEGER_LOW_PART(
               MmGetPhysicalAddress(MemoryArea->BaseAddress + (i*PAGESIZE))), 
                       1);
	  }
     }
   
   RemoveEntryList(&(MemoryArea->Entry));
   ExFreePool(MemoryArea);
   MmUnlockMemoryAreaList(BaseAddress, &oldlvl);
   return(STATUS_SUCCESS);
}

PMEMORY_AREA MmSplitMemoryArea(PEPROCESS Process,
			       PMEMORY_AREA OriginalMemoryArea,
			       PVOID BaseAddress,
			       ULONG Length,
			       ULONG NewType,
			       ULONG NewAttributes)
{
   KIRQL oldlvl;
   PMEMORY_AREA Result;
   PMEMORY_AREA Split;
   
   Result = ExAllocatePool(NonPagedPool,sizeof(MEMORY_AREA));
   RtlZeroMemory(Result,sizeof(MEMORY_AREA));
   Result->Type=NewType;
   Result->BaseAddress=BaseAddress;
   Result->Length=Length;
   Result->Attributes=NewAttributes;
   Result->LockCount=0;
   
   MmLockMemoryAreaList(OriginalMemoryArea->BaseAddress,&oldlvl);
   
//   MmDumpMemoryAreas(MmGetRelatedListHead(Process,BaseAddress));
   
   if (BaseAddress == OriginalMemoryArea->BaseAddress)
     {
	OriginalMemoryArea->BaseAddress = BaseAddress + Length;
	OriginalMemoryArea->Length = OriginalMemoryArea->Length - Length;
	MmInsertMemoryAreaWithoutLock(Process,Result);
	MmUnlockMemoryAreaList(OriginalMemoryArea->BaseAddress,&oldlvl);
	
//	MmDumpMemoryAreas(MmGetRelatedListHead(Process,BaseAddress));

	return(Result);
     }
   if ((BaseAddress + Length) == 
       (OriginalMemoryArea->BaseAddress + OriginalMemoryArea->Length))
     {
	OriginalMemoryArea->Length = OriginalMemoryArea->Length - Length; 
	MmInsertMemoryAreaWithoutLock(Process,Result);
	MmUnlockMemoryAreaList(OriginalMemoryArea->BaseAddress,&oldlvl);
	
//	MmDumpMemoryAreas(MmGetRelatedListHead(Process,BaseAddress));

	return(Result);
     }
      
   Split = ExAllocatePool(NonPagedPool,sizeof(MEMORY_AREA));
   RtlCopyMemory(Split,OriginalMemoryArea,sizeof(MEMORY_AREA));
   Split->BaseAddress = BaseAddress + Length;
   Split->Length = OriginalMemoryArea->Length - (((ULONG)BaseAddress) 
						 + Length);
   
   OriginalMemoryArea->Length = BaseAddress - OriginalMemoryArea->BaseAddress;
      
   MmUnlockMemoryAreaList(OriginalMemoryArea->BaseAddress,&oldlvl);
   
//   MmDumpMemoryAreas(MmGetRelatedListHead(Process,BaseAddress));
   
   return(Split);
}

NTSTATUS MmCreateMemoryArea(KPROCESSOR_MODE Mode,
			    PEPROCESS Process,
			    ULONG Type,
			    PVOID* BaseAddress,
			    ULONG Length,
			    ULONG Attributes,
			    MEMORY_AREA** Result)
{
   KIRQL oldlvl;
   
   DPRINT("MmCreateMemoryArea(Mode %x, Type %d, BaseAddress %x,"
	  "*BaseAddress %x, Length %x, Attributes %x, Result %x)\n",
          Mode,Type,BaseAddress,*BaseAddress,Length,Attributes,Result);

   
   if ((*BaseAddress)==0)
     {
	MmLockMemoryAreaListByMode(Mode,&oldlvl);
     }
   else
     {
	MmLockMemoryAreaList(*BaseAddress,&oldlvl);
     }

   if ((*BaseAddress)==0)
     {
	*BaseAddress = MmFindGapWithoutLock(Process,Mode,PAGE_ROUND_UP(Length)
					    +(PAGESIZE*2));
	if ((*BaseAddress)==0)
	  {
	     MmUnlockMemoryAreaListByMode(Mode,&oldlvl);
	     return(STATUS_UNSUCCESSFUL);
	  }
	(*BaseAddress)=(*BaseAddress)+PAGESIZE;
     }
   else
     {
	(*BaseAddress) = (PVOID)PAGE_ROUND_DOWN((*BaseAddress));
	if (MmOpenMemoryAreaByRegionWithoutLock(Process,
						*BaseAddress,
						Length)!=NULL)
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
   (*Result)->LockCount=0;
   
   MmInsertMemoryAreaWithoutLock(Process,*Result);
   MmUnlockMemoryAreaList(*BaseAddress,&oldlvl);
   
   
   return(STATUS_SUCCESS);
}
