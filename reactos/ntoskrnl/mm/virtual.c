/* $Id: virtual.c,v 1.27 2000/04/07 02:24:01 dwelch Exp $
 *
 * COPYRIGHT:   See COPYING in the top directory
 * PROJECT:     ReactOS kernel
 * FILE:        ntoskrnl/mm/virtual.c
 * PURPOSE:     implementing the Virtualxxx section of the win32 api
 * PROGRAMMER:  David Welch
 * UPDATE HISTORY:
 *              09/4/98: Created
 *              10/6/98: Corrections from Fatahi (i_fatahi@hotmail.com)
 *              30/9/98: Implemented ZwxxxVirtualMemory functions
 */
 
/* INCLUDE *****************************************************************/

#include <ddk/ntddk.h>
#include <internal/mm.h>
#include <internal/mmhal.h>
#include <internal/ob.h>
#include <internal/io.h>
#include <internal/ps.h>
#include <string.h>
#include <internal/string.h>

#define NDEBUG
#include <internal/debug.h>

/* FUNCTIONS ****************************************************************/

NTSTATUS MmReleaseMemoryArea(PEPROCESS Process, PMEMORY_AREA Marea)
{
   PVOID i;
   
   DPRINT("MmReleaseMemoryArea(Process %x, Marea %x)\n",Process,Marea);
   
   DPRINT("Releasing %x between %x %x\n",
	  Marea, Marea->BaseAddress, Marea->BaseAddress + Marea->Length);
   
   if (Marea->Type == MEMORY_AREA_SECTION_VIEW_COMMIT ||
       Marea->Type == MEMORY_AREA_SECTION_VIEW_RESERVE)
     {
	MmUnmapViewOfSection(Process, Marea);
     }
   
   for (i = Marea->BaseAddress; 
	i < (Marea->BaseAddress + Marea->Length);
	i = i+PAGESIZE)
     {
	MmDeletePageEntry(Process, i, TRUE);
     }
   ExFreePool(Marea);
   
   return(STATUS_SUCCESS);
}

NTSTATUS MmReleaseMmInfo(PEPROCESS Process)
{
   PLIST_ENTRY CurrentEntry;
   PMEMORY_AREA Current;
   
   DPRINT("MmReleaseMmInfo(Process %x)\n",Process);
   
   MmLockAddressSpace(&Process->Pcb.AddressSpace);
   
   while (!IsListEmpty(&Process->Pcb.AddressSpace.MAreaListHead))
     {
	CurrentEntry = RemoveHeadList(
				     &Process->Pcb.AddressSpace.MAreaListHead);
	Current = CONTAINING_RECORD(CurrentEntry, MEMORY_AREA, Entry);
	
	MmReleaseMemoryArea(Process, Current);
     }
   
   Mmi386ReleaseMmInfo(Process);
   
   MmUnlockAddressSpace(&Process->Pcb.AddressSpace);
   
   DPRINT("Finished MmReleaseMmInfo()\n");
   return(STATUS_SUCCESS);
}

BOOLEAN STDCALL MmIsNonPagedSystemAddressValid(PVOID VirtualAddress)
{
   UNIMPLEMENTED;
}

BOOLEAN STDCALL MmIsAddressValid(PVOID VirtualAddress)
/*
 * FUNCTION: Checks whether the given address is valid for a read or write
 * ARGUMENTS:
 *          VirtualAddress = address to check
 * RETURNS: True if the access would be valid
 *          False if the access would cause a page fault
 * NOTES: This function checks whether a byte access to the page would
 *        succeed. Is this realistic for RISC processors which don't
 *        allow byte granular access?
 */
{
   MEMORY_AREA* MemoryArea;
   PMADDRESS_SPACE AddressSpace;
   
   AddressSpace = &PsGetCurrentProcess()->Pcb.AddressSpace;
   
   MmLockAddressSpace(AddressSpace);
   MemoryArea = MmOpenMemoryAreaByAddress(AddressSpace,
					  VirtualAddress);

   if (MemoryArea == NULL)
     {
	MmUnlockAddressSpace(AddressSpace);
	return(FALSE);
     }
   MmUnlockAddressSpace(AddressSpace);
   return(TRUE);
}


NTSTATUS STDCALL NtAllocateVirtualMemory(IN	HANDLE	ProcessHandle,
					 IN OUT	PVOID	* BaseAddress,
					 IN	ULONG	ZeroBits,
					 IN OUT	PULONG	RegionSize,
					 IN	ULONG	AllocationType, 
					 IN	ULONG	Protect)
/*
 * FUNCTION: Allocates a block of virtual memory in the process address space
 * ARGUMENTS:
 *      ProcessHandle = The handle of the process which owns the virtual memory
 *      BaseAddress   = A pointer to the virtual memory allocated. If you 
 *                      supply a non zero value the system will try to 
 *                      allocate the memory at the address supplied. It round 
 *                      it down to a multiple  of the page size.
 *      ZeroBits  = (OPTIONAL) You can specify the number of high order bits 
 *                      that must be zero, ensuring that the memory will be 
 *                      allocated at a address below a certain value.
 *      RegionSize = The number of bytes to allocate
 *      AllocationType = Indicates the type of virtual memory you like to 
 *                       allocated, can be one of the values : MEM_COMMIT, 
 *                       MEM_RESERVE, MEM_RESET, MEM_TOP_DOWN
 *      Protect = Indicates the protection type of the pages allocated, can be
 *                a combination of PAGE_READONLY, PAGE_READWRITE, 
 *                PAGE_EXECUTE_READ, PAGE_EXECUTE_READWRITE, PAGE_GUARD, 
 *                PAGE_NOACCESS
 * REMARKS:
 *       This function maps to the win32 VirtualAllocEx. Virtual memory is 
 *       process based so the  protocol starts with a ProcessHandle. I 
 *       splitted the functionality of obtaining the actual address and 
 *       specifying the start address in two parameters ( BaseAddress and 
 *       StartAddress ) The NumberOfBytesAllocated specify the range and the 
 *       AllocationType and ProctectionType map to the other two parameters.
 * RETURNS: Status
 */
{
   PEPROCESS Process;
   MEMORY_AREA* MemoryArea;
   ULONG Type;
   NTSTATUS Status;
   PMADDRESS_SPACE AddressSpace;
   
   DPRINT("NtAllocateVirtualMemory(ProcessHandle %x, *BaseAddress %x, "
	  "ZeroBits %d, *RegionSize %x, AllocationType %x, Protect %x)\n",
	  ProcessHandle,*BaseAddress,ZeroBits,*RegionSize,AllocationType,
	  Protect);
   
   Status = ObReferenceObjectByHandle(ProcessHandle,
				      PROCESS_VM_OPERATION,
				      NULL,
				      UserMode,
				      (PVOID*)(&Process),
				      NULL);
   if (Status != STATUS_SUCCESS)
     {
	DPRINT("NtAllocateVirtualMemory() = %x\n",Status);
	return(Status);
     }
   
   if (AllocationType & MEM_RESERVE)
     {
	Type = MEMORY_AREA_RESERVE;
     }
   else
     {
	Type = MEMORY_AREA_COMMIT;
     }
   
   AddressSpace = &Process->Pcb.AddressSpace;
   MmLockAddressSpace(AddressSpace);
   
   if ((*BaseAddress) != 0)
     {
	MemoryArea = MmOpenMemoryAreaByAddress(&Process->Pcb.AddressSpace,
					       *BaseAddress);
	
	if (MemoryArea != NULL)
	  {
	     if (MemoryArea->BaseAddress == (*BaseAddress) &&
		 MemoryArea->Length == *RegionSize)
	       {
		  MemoryArea->Type = Type;
		  MemoryArea->Attributes =Protect;
		  DPRINT("*BaseAddress %x\n",*BaseAddress);
		  MmUnlockAddressSpace(AddressSpace);
		  ObDereferenceObject(Process);
		  return(STATUS_SUCCESS);
	       }
	     
	     MemoryArea = MmSplitMemoryArea(Process,
					    &Process->Pcb.AddressSpace,
					    MemoryArea,
					    *BaseAddress,
					    *RegionSize,
					    Type,
					    Protect);
	     DPRINT("*BaseAddress %x\n",*BaseAddress);
	     MmUnlockAddressSpace(AddressSpace);
	     ObDereferenceObject(Process);
	     return(STATUS_SUCCESS);
	  }
     }
   
   // FIXME RegionSize should be passed as pointer
   // dwelch: Why?

   Status = MmCreateMemoryArea(Process,
			       &Process->Pcb.AddressSpace,
			       Type,
			       BaseAddress,
			       *RegionSize,
			       Protect,
			       &MemoryArea);
   
   if (Status != STATUS_SUCCESS)
     {
	DPRINT("NtAllocateVirtualMemory() = %x\n",Status);
	MmUnlockAddressSpace(AddressSpace);	
	ObDereferenceObject(Process);
	return(Status);
     }
   
   DPRINT("*BaseAddress %x\n",*BaseAddress);
   MmUnlockAddressSpace(AddressSpace);
   ObDereferenceObject(Process);
   return(STATUS_SUCCESS);
}


NTSTATUS STDCALL NtFlushVirtualMemory(IN	HANDLE	ProcessHandle,
				      IN	PVOID	BaseAddress,
				      IN	ULONG	NumberOfBytesToFlush,
				      OUT PULONG NumberOfBytesFlushed OPTIONAL)
/*
 * FUNCTION: Flushes virtual memory to file
 * ARGUMENTS:
 *        ProcessHandle = Points to the process that allocated the virtual 
 *                        memory
 *        BaseAddress = Points to the memory address
 *        NumberOfBytesToFlush = Limits the range to flush,
 *        NumberOfBytesFlushed = Actual number of bytes flushed
 * RETURNS: Status 
 */
{
	UNIMPLEMENTED;
}


NTSTATUS STDCALL NtFreeVirtualMemory(IN	HANDLE	ProcessHandle,
				     IN	PVOID	* BaseAddress,	
				     IN	PULONG	RegionSize,	
				     IN	ULONG	FreeType)
/*
 * FUNCTION: Frees a range of virtual memory
 * ARGUMENTS:
 *        ProcessHandle = Points to the process that allocated the virtual 
 *                        memory
 *        BaseAddress = Points to the memory address, rounded down to a 
 *                      multiple of the pagesize
 *        RegionSize = Limits the range to free, rounded up to a multiple of 
 *                     the paging size
 *        FreeType = Can be one of the values:  MEM_DECOMMIT, or MEM_RELEASE
 * RETURNS: Status 
 */
{
   MEMORY_AREA* MemoryArea;
   NTSTATUS Status;
   PEPROCESS Process;
   PMADDRESS_SPACE AddressSpace;
   
   DPRINT("NtFreeVirtualMemory(ProcessHandle %x, *BaseAddress %x, "
	  "*RegionSize %x, FreeType %x)\n",ProcessHandle,*BaseAddress,
	  *RegionSize,FreeType);
				 
   
   Status = ObReferenceObjectByHandle(ProcessHandle,
				      PROCESS_VM_OPERATION,
				      PsProcessType,
				      UserMode,
				      (PVOID*)(&Process),
				      NULL);
   if (Status != STATUS_SUCCESS)
     {
	return(Status);
     }
   
   AddressSpace = &Process->Pcb.AddressSpace;
   
   MmLockAddressSpace(AddressSpace);
   MemoryArea = MmOpenMemoryAreaByAddress(AddressSpace,
					  *BaseAddress);
   if (MemoryArea == NULL)
     {
	MmUnlockAddressSpace(AddressSpace);
	ObDereferenceObject(Process);
	return(STATUS_UNSUCCESSFUL);
     }
   
   switch (FreeType)
     {
      case MEM_RELEASE:
	if (MemoryArea->BaseAddress != (*BaseAddress))
	  {
	     MmUnlockAddressSpace(AddressSpace);
	     ObDereferenceObject(Process);
	     return(STATUS_UNSUCCESSFUL);
	  }
	MmFreeMemoryArea(&Process->Pcb.AddressSpace,
			 BaseAddress,
			 0,
			 TRUE);
	MmUnlockAddressSpace(AddressSpace);
	ObDereferenceObject(Process);
	return(STATUS_SUCCESS);
	
      case MEM_DECOMMIT:	
	MmSplitMemoryArea(Process,
			  &Process->Pcb.AddressSpace,
			  MemoryArea,
			  *BaseAddress,
			  *RegionSize,
			  MEMORY_AREA_RESERVE,
			  MemoryArea->Attributes);
	MmUnlockAddressSpace(AddressSpace);	
	ObDereferenceObject(Process);
	return(STATUS_SUCCESS);
     }
   MmUnlockAddressSpace(AddressSpace);
   ObDereferenceObject(Process);
   return(STATUS_NOT_IMPLEMENTED);
}


NTSTATUS STDCALL NtLockVirtualMemory(HANDLE	ProcessHandle,
				     PVOID	BaseAddress,
				     ULONG	NumberOfBytesToLock,
				     PULONG	NumberOfBytesLocked)
{
	UNIMPLEMENTED;
}


VOID MmChangeAreaProtection(PEPROCESS Process,
			    PVOID BaseAddress,
			    ULONG Length,
			    ULONG Protect)
{
   ULONG i;
   
   for (i=0; i<(Length/PAGESIZE); i++)
     {
	if (MmIsPagePresent(Process, BaseAddress + (i*PAGESIZE)))
	  {
	     MmSetPageProtect(Process,
			      BaseAddress + (i*PAGESIZE), 
			      Protect);
	  }
     }
}


NTSTATUS STDCALL NtProtectVirtualMemory(IN	HANDLE	ProcessHandle,
					IN	PVOID	BaseAddress,
					IN	ULONG	NumberOfBytesToProtect,
					IN	ULONG	NewAccessProtection,
					OUT	PULONG	OldAccessProtection)
{
   PMEMORY_AREA MemoryArea;
   PEPROCESS Process;
   NTSTATUS Status;
   PMADDRESS_SPACE AddressSpace;
   
   Status = ObReferenceObjectByHandle(ProcessHandle,
				      PROCESS_VM_OPERATION,
				      PsProcessType,
				      UserMode,
				      (PVOID*)(&Process),
				      NULL);
   if (Status != STATUS_SUCCESS)
     {
	DPRINT("NtProtectVirtualMemory() = %x\n",Status);
	return(Status);
     }

   AddressSpace = &Process->Pcb.AddressSpace;
   
   MmLockAddressSpace(AddressSpace);
   MemoryArea = MmOpenMemoryAreaByAddress(AddressSpace,
					  BaseAddress);
   if (MemoryArea == NULL)
     {
	DPRINT("NtProtectVirtualMemory() = %x\n",STATUS_UNSUCCESSFUL);
	MmUnlockAddressSpace(AddressSpace);
	ObDereferenceObject(Process);
	return(STATUS_UNSUCCESSFUL);
     }

   *OldAccessProtection = MemoryArea->Attributes;

   if (MemoryArea->BaseAddress == BaseAddress &&
       MemoryArea->Length == NumberOfBytesToProtect)
     {
	MemoryArea->Attributes = NewAccessProtection;	
     }
   else
     {
	MemoryArea = MmSplitMemoryArea(Process,
				       &Process->Pcb.AddressSpace,
				       MemoryArea,
				       BaseAddress,
				       NumberOfBytesToProtect,
				       MemoryArea->Type,
				       NewAccessProtection);
     }
   MmChangeAreaProtection(Process,
			  BaseAddress,
			  NumberOfBytesToProtect,
			  NewAccessProtection);
   MmUnlockAddressSpace(AddressSpace);
   ObDereferenceObject(Process);
   return(STATUS_SUCCESS);
}


NTSTATUS STDCALL NtQueryVirtualMemory (IN HANDLE ProcessHandle,
				       IN PVOID Address,
				       IN CINT VirtualMemoryInformationClass,
				       OUT PVOID VirtualMemoryInformation,
				       IN ULONG Length,
				       OUT PULONG ResultLength)
{
   NTSTATUS Status;
   PEPROCESS Process;
   MEMORY_AREA* MemoryArea;

   DPRINT("NtQueryVirtualMemory(ProcessHandle %x, Address %x, "
          "VirtualMemoryInformationClass %d, VirtualMemoryInformation %x, "
          "Length %lu ResultLength %x)\n",ProcessHandle,Address,
          VirtualMemoryInformationClass,VirtualMemoryInformation,
          Length,ResultLength);

   switch(VirtualMemoryInformationClass)
     {
        case MemoryBasicInformation:
          {
             PMEMORY_BASIC_INFORMATION Info =
                (PMEMORY_BASIC_INFORMATION)VirtualMemoryInformation;
	     PMADDRESS_SPACE AddressSpace;
	     
             if (Length < sizeof(MEMORY_BASIC_INFORMATION))
               {
                  ObDereferenceObject(Process);
                  return STATUS_INFO_LENGTH_MISMATCH;
               }

             if (ResultLength)
               {
                  *ResultLength = sizeof(MEMORY_BASIC_INFORMATION);
               }

             Status = ObReferenceObjectByHandle(ProcessHandle,
                                                PROCESS_QUERY_INFORMATION,
                                                NULL,
                                                UserMode,
                                                (PVOID*)(&Process),
                                                NULL);

             if (!NT_SUCCESS(Status))
               {
                  DPRINT("NtQueryVirtualMemory() = %x\n",Status);
                  return(Status);
               }

	     AddressSpace = &Process->Pcb.AddressSpace;
	     MmLockAddressSpace(AddressSpace);
             MemoryArea = MmOpenMemoryAreaByAddress(AddressSpace,
                                                    Address);

             if (MemoryArea == NULL)
               {
                  Info->State = MEM_FREE;
                  DPRINT("Virtual memory at %p is free.\n", Address);
		  MmUnlockAddressSpace(AddressSpace);
                  ObDereferenceObject(Process);
                  return (STATUS_SUCCESS);
               }

             if (MemoryArea->Type == MEMORY_AREA_COMMIT)
               {
                  Info->State = MEM_COMMIT;
               }
             else
               {
                  Info->State = MEM_RESERVE;
               }

             Info->BaseAddress = MemoryArea->BaseAddress;
             Info->RegionSize  = MemoryArea->Length;

             DPRINT("BaseAddress %p, RegionSize %x State %x\n",
                    Info->BaseAddress, Info->RegionSize, Info->State);
	     
	     MmUnlockAddressSpace(AddressSpace);
             ObDereferenceObject(Process);
             return STATUS_SUCCESS;
          }
          break;
     }

   return STATUS_INVALID_INFO_CLASS;
}


NTSTATUS STDCALL NtReadVirtualMemory(IN	HANDLE	ProcessHandle,
				     IN	PVOID	BaseAddress,
				     OUT	PVOID	Buffer,
				     IN	ULONG	NumberOfBytesToRead,
				     OUT	PULONG	NumberOfBytesRead)
{
   NTSTATUS Status;
   PMDL Mdl;
   PVOID SystemAddress;
   PEPROCESS Process;
   
   DPRINT("NtReadVirtualMemory(ProcessHandle %x, BaseAddress %x, "
	    "Buffer %x, NumberOfBytesToRead %d)\n",ProcessHandle,BaseAddress,
	    Buffer,NumberOfBytesToRead);
   
   Status = ObReferenceObjectByHandle(ProcessHandle,
				      PROCESS_VM_WRITE,
				      NULL,
				      UserMode,
				      (PVOID*)(&Process),
				      NULL);
   if (Status != STATUS_SUCCESS)
     {
	return(Status);
     }
   
   Mdl = MmCreateMdl(NULL, 
		     Buffer,
		     NumberOfBytesToRead);
   MmProbeAndLockPages(Mdl,
		       UserMode,
		       IoWriteAccess);
   
   KeAttachProcess(Process);
   
   SystemAddress = MmGetSystemAddressForMdl(Mdl);
   memcpy(SystemAddress, BaseAddress, NumberOfBytesToRead);
   
   KeDetachProcess();
   
   ObDereferenceObject(Process);
   
   *NumberOfBytesRead = NumberOfBytesToRead;
   return(STATUS_SUCCESS);
}


NTSTATUS STDCALL NtUnlockVirtualMemory(HANDLE	ProcessHandle,
				       PVOID	BaseAddress,
				       ULONG	NumberOfBytesToUnlock,
				       PULONG NumberOfBytesUnlocked OPTIONAL)
{
	UNIMPLEMENTED;
}


NTSTATUS STDCALL NtWriteVirtualMemory(IN	HANDLE	ProcessHandle,
				      IN	PVOID	BaseAddress,
				      IN	PVOID	Buffer,
				      IN	ULONG	NumberOfBytesToWrite,
				      OUT	PULONG	NumberOfBytesWritten)
{
   NTSTATUS Status;
   PMDL Mdl;
   PVOID SystemAddress;
   PEPROCESS Process;
   
   DPRINT("NtWriteVirtualMemory(ProcessHandle %x, BaseAddress %x, "
	    "Buffer %x, NumberOfBytesToWrite %d)\n",ProcessHandle,BaseAddress,
	    Buffer,NumberOfBytesToWrite);
   
   Status = ObReferenceObjectByHandle(ProcessHandle,
				      PROCESS_VM_WRITE,
				      NULL,
				      UserMode,
				      (PVOID*)(&Process),
				      NULL);
   if (Status != STATUS_SUCCESS)
     {
	return(Status);
     }
   
   Mdl = MmCreateMdl(NULL, 
		     Buffer,
		     NumberOfBytesToWrite);
   MmProbeAndLockPages(Mdl,
		       UserMode,
		       IoReadAccess);
   
   KeAttachProcess(Process);
   
   DPRINT("Attached to process copying memory\n");
   
   SystemAddress = MmGetSystemAddressForMdl(Mdl);
   memcpy(BaseAddress, SystemAddress, NumberOfBytesToWrite);
   
   DPRINT("Done copy\n");
   
   KeDetachProcess();
   
   ObDereferenceObject(Process);
   
   *NumberOfBytesWritten = NumberOfBytesToWrite;
   
   DPRINT("Finished NtWriteVirtualMemory()\n");
   
   return(STATUS_SUCCESS);
}


DWORD
STDCALL
MmSecureVirtualMemory (
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2
	)
{
	UNIMPLEMENTED;
	return 0;
}


VOID
STDCALL
MmUnsecureVirtualMemory (
	DWORD	Unknown0
	)
{
	UNIMPLEMENTED;
}

/* EOF */
