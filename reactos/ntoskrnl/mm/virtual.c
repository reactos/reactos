/* $Id: virtual.c,v 1.30 2000/07/04 08:52:45 dwelch Exp $
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

ULONG MmPageOutVirtualMemory(PMADDRESS_SPACE AddressSpace,
			     PMEMORY_AREA MemoryArea,
			     PVOID Address)
{
   PHYSICAL_ADDRESS PhysicalAddress;
   
   if ((MemoryArea->Attributes & PAGE_READONLY) ||
       (MemoryArea->Attributes & PAGE_EXECUTE_READ) ||
       !MmIsPageDirty(PsGetCurrentProcess(), Address))
     {
	PhysicalAddress = MmGetPhysicalAddress(Address);
	
	MmDereferencePage((PVOID)PhysicalAddress.u.LowPart);
	MmSetPage(PsGetCurrentProcess(),
		  Address,
		  0,
		  0);
	return(1);
     }
   return(0);     
}

NTSTATUS MmNotPresentFaultVirtualMemory(PMADDRESS_SPACE AddressSpace,
					MEMORY_AREA* MemoryArea, 
					PVOID Address)
{
   if (MmIsPagePresent(NULL, Address))
     {
	
	return(STATUS_SUCCESS);
     }
   
   MmSetPage(PsGetCurrentProcess(),
	     Address,
	     MemoryArea->Attributes,
	     (ULONG)MmAllocPage(0));
   
   return(STATUS_SUCCESS);
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
   
   AddressSpace = &Process->AddressSpace;
   MmLockAddressSpace(AddressSpace);
   
   if ((*BaseAddress) != 0)
     {
	MemoryArea = MmOpenMemoryAreaByAddress(&Process->AddressSpace,
					       *BaseAddress);
	
	if (MemoryArea != NULL)
	  {
	     if (MemoryArea->BaseAddress == (*BaseAddress) &&
		 MemoryArea->Length == *RegionSize)
	       {
		  MemoryArea->Type = Type;
		  MemoryArea->Attributes = Protect;
		  DPRINT("*BaseAddress %x\n",*BaseAddress);
		  MmUnlockAddressSpace(AddressSpace);
		  ObDereferenceObject(Process);
		  return(STATUS_SUCCESS);
	       }
	     
	     MemoryArea = MmSplitMemoryArea(Process,
					    &Process->AddressSpace,
					    MemoryArea,
					    *BaseAddress,
					    *RegionSize,
					    Type,
					    Protect);
	     DPRINT("*BaseAddress %x\n",*BaseAddress);
	     /* FIXME: Reserve/dereserve swap pages */
	     MmUnlockAddressSpace(AddressSpace);
	     ObDereferenceObject(Process);
	     return(STATUS_SUCCESS);
	  }
     }
   
   Status = MmCreateMemoryArea(Process,
			       &Process->AddressSpace,
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
   if ((AllocationType & MEM_COMMIT) &&
       ((Protect & PAGE_READWRITE) ||
	(Protect & PAGE_EXECUTE_READWRITE)))
     {
	MmReserveSwapPages(PAGE_ROUND_UP((*RegionSize)));
     }
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
   
   AddressSpace = &Process->AddressSpace;
   
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
	if ((MemoryArea->Type == MEMORY_AREA_COMMIT) &&
	    ((MemoryArea->Attributes & PAGE_READWRITE) ||
	     (MemoryArea->Attributes & PAGE_EXECUTE_READWRITE)))
	  {
	     MmDereserveSwapPages(PAGE_ROUND_UP(MemoryArea->Length));
	  }
	MmFreeMemoryArea(&Process->AddressSpace,
			 BaseAddress,
			 0,
			 TRUE);
	MmUnlockAddressSpace(AddressSpace);
	ObDereferenceObject(Process);
	return(STATUS_SUCCESS);
	
      case MEM_DECOMMIT:	
	if ((MemoryArea->Type == MEMORY_AREA_COMMIT) &&
	    ((MemoryArea->Attributes & PAGE_READWRITE) ||
	     (MemoryArea->Attributes & PAGE_EXECUTE_READWRITE)))
	  {
	     MmDereserveSwapPages(PAGE_ROUND_UP((*RegionSize)));
	  }
	MmSplitMemoryArea(Process,
			  &Process->AddressSpace,
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

   AddressSpace = &Process->AddressSpace;
   
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
				       &Process->AddressSpace,
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

	     AddressSpace = &Process->AddressSpace;
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
