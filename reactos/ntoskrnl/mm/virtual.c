/*
 *  ReactOS kernel
 *  Copyright (C) 1998, 1999, 2000, 2001 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/* $Id: virtual.c,v 1.65 2003/04/28 10:37:40 gvg Exp $
 *
 * PROJECT:     ReactOS kernel
 * FILE:        ntoskrnl/mm/virtual.c
 * PURPOSE:     Implementing operations on virtual memory.
 * PROGRAMMER:  David Welch
 */
 
/* INCLUDE *****************************************************************/

#include <ddk/ntddk.h>
#include <internal/mm.h>
#include <internal/ob.h>
#include <internal/io.h>
#include <internal/ps.h>
#include <internal/pool.h>
#include <internal/safe.h>

#define NDEBUG
#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

NTSTATUS STDCALL 
NtFlushVirtualMemory(IN	HANDLE	ProcessHandle,
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

NTSTATUS STDCALL 
NtLockVirtualMemory(HANDLE	ProcessHandle,
		    PVOID	BaseAddress,
		    ULONG	NumberOfBytesToLock,
		    PULONG	NumberOfBytesLocked)
{
  UNIMPLEMENTED;
}

NTSTATUS STDCALL 
NtQueryVirtualMemory (IN HANDLE ProcessHandle,
		      IN PVOID Address,
		      IN CINT VirtualMemoryInformationClass,
		      OUT PVOID VirtualMemoryInformation,
		      IN ULONG Length,
		      OUT PULONG UnsafeResultLength)
{
   NTSTATUS Status;
   PEPROCESS Process;
   MEMORY_AREA* MemoryArea;
   ULONG ResultLength = 0;
   PMADDRESS_SPACE AddressSpace;
   
   DPRINT("NtQueryVirtualMemory(ProcessHandle %x, Address %x, "
          "VirtualMemoryInformationClass %d, VirtualMemoryInformation %x, "
          "Length %lu ResultLength %x)\n",ProcessHandle,Address,
          VirtualMemoryInformationClass,VirtualMemoryInformation,
          Length,ResultLength);
   
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
   switch(VirtualMemoryInformationClass)
     {
     case MemoryBasicInformation:
       {
	 PMEMORY_BASIC_INFORMATION Info =
	   (PMEMORY_BASIC_INFORMATION)VirtualMemoryInformation;
	 
	 if (Length != sizeof(MEMORY_BASIC_INFORMATION))
	   {
	     MmUnlockAddressSpace(AddressSpace);
	     ObDereferenceObject(Process);
	     return(STATUS_INFO_LENGTH_MISMATCH);
	   }
	 
	 if (MemoryArea == NULL)
	   {
	     Info->State = MEM_FREE;
	     Info->BaseAddress = (PVOID)PAGE_ROUND_DOWN(Address);
	     Status = STATUS_SUCCESS;
	     ResultLength = sizeof(MEMORY_BASIC_INFORMATION);
	   }
	 else if (MemoryArea->Type == MEMORY_AREA_VIRTUAL_MEMORY)
	   {
	     Status = MmQueryAnonMem(MemoryArea, Address, Info, 
				     &ResultLength);
	   }
	 else if (MemoryArea->Type == MEMORY_AREA_SECTION_VIEW)
	   {
	     Status = MmQuerySectionView(MemoryArea, Address, Info, 
					 &ResultLength);
	   }
	 else
	   {
	     Status = STATUS_UNSUCCESSFUL;
	     ResultLength = 0;
	   }
	 break;
       }
       
     default:
       {
	 Status = STATUS_INVALID_INFO_CLASS;
	 ResultLength = 0;
	 break;
       }
     }

   MmUnlockAddressSpace(AddressSpace);
   ObDereferenceObject(Process);
   if (UnsafeResultLength != NULL)
     {
       MmCopyToCaller(UnsafeResultLength, &ResultLength, sizeof(ULONG));
     }
   return(Status);
}

NTSTATUS STDCALL
NtProtectVirtualMemory(IN	HANDLE	ProcessHandle,
		       IN	PVOID	BaseAddress,
		       IN	ULONG	NumberOfBytesToProtect,
		       IN	ULONG	NewAccessProtection,
		       OUT	PULONG	UnsafeOldAccessProtection)
{
   PMEMORY_AREA MemoryArea;
   PEPROCESS Process;
   NTSTATUS Status;
   PMADDRESS_SPACE AddressSpace;
   ULONG OldAccessProtection;

   NumberOfBytesToProtect =
     PAGE_ROUND_UP(BaseAddress + NumberOfBytesToProtect) -
     PAGE_ROUND_DOWN(BaseAddress);
   BaseAddress = (PVOID)PAGE_ROUND_DOWN(BaseAddress);
   
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
       MmUnlockAddressSpace(AddressSpace);
       ObDereferenceObject(Process);
       return(STATUS_UNSUCCESSFUL);
     }
   
   if (MemoryArea->Type == MEMORY_AREA_VIRTUAL_MEMORY)
     {
       Status = MmProtectAnonMem(AddressSpace, MemoryArea, BaseAddress,
				 NumberOfBytesToProtect, NewAccessProtection,
				 &OldAccessProtection);
     }
   else if (MemoryArea->Type == MEMORY_AREA_SECTION_VIEW)
     {
       Status = MmProtectSectionView(AddressSpace, MemoryArea, BaseAddress,
				     NumberOfBytesToProtect, 
				     NewAccessProtection,
				     &OldAccessProtection);
     }
   
   MmUnlockAddressSpace(AddressSpace);
   ObDereferenceObject(Process);

   MmCopyToCaller(UnsafeOldAccessProtection, &OldAccessProtection, 
		  sizeof(ULONG));

   return(Status);
}

NTSTATUS STDCALL 
NtReadVirtualMemory(IN	HANDLE	ProcessHandle,
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
  
  if (Mdl->MappedSystemVa != NULL)
    {	     
      MmUnmapLockedPages(Mdl->MappedSystemVa, Mdl);
    }
  MmUnlockPages(Mdl);
  ExFreePool(Mdl);
  
  ObDereferenceObject(Process);
  
  *NumberOfBytesRead = NumberOfBytesToRead;
  return(STATUS_SUCCESS);
}

NTSTATUS STDCALL 
NtUnlockVirtualMemory(HANDLE	ProcessHandle,
		      PVOID	BaseAddress,
		      ULONG	NumberOfBytesToUnlock,
		      PULONG NumberOfBytesUnlocked OPTIONAL)
{
  UNIMPLEMENTED;
}


NTSTATUS STDCALL 
NtWriteVirtualMemory(IN	HANDLE	ProcessHandle,
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
   
   SystemAddress = MmGetSystemAddressForMdl(Mdl);
   memcpy(BaseAddress, SystemAddress, NumberOfBytesToWrite);
   
   KeDetachProcess();
   
   ObDereferenceObject(Process);

   if (Mdl->MappedSystemVa != NULL)
     {	     
       MmUnmapLockedPages(Mdl->MappedSystemVa, Mdl);
     }
   MmUnlockPages(Mdl);
   ExFreePool(Mdl);
   
   *NumberOfBytesWritten = NumberOfBytesToWrite;
   
   return(STATUS_SUCCESS);
}

DWORD STDCALL
MmSecureVirtualMemory (DWORD	Unknown0,
		       DWORD	Unknown1,
		       DWORD	Unknown2)
{
  UNIMPLEMENTED;
  return 0;
}


VOID STDCALL
MmUnsecureVirtualMemory (DWORD Unknown0)
{
  UNIMPLEMENTED;
}

/* EOF */
