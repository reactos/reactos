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
/* $Id: virtual.c,v 1.82 2004/10/28 19:01:58 chorns Exp $
 *
 * PROJECT:     ReactOS kernel
 * FILE:        ntoskrnl/mm/virtual.c
 * PURPOSE:     Implementing operations on virtual memory.
 * PROGRAMMER:  David Welch
 */

/* INCLUDE *****************************************************************/

#include <ntoskrnl.h>

#define NDEBUG
#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

NTSTATUS STDCALL
NtFlushVirtualMemory(IN HANDLE ProcessHandle,
                     IN PVOID BaseAddress,
                     IN ULONG NumberOfBytesToFlush,
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
   return(STATUS_NOT_IMPLEMENTED);
}


NTSTATUS STDCALL
NtLockVirtualMemoryInternal(HANDLE ProcessHandle,
  PVOID BaseAddress,
  ULONG NumberOfBytesToLock,
  PULONG NumberOfBytesLocked,
  PObReferenceObjectByHandle pObReferenceObjectByHandle,
  PMmCreateMdl pMmCreateMdl,
  PObDereferenceObject pObDereferenceObject,
  PMmProbeAndLockPages pMmProbeAndLockPages,
  PExFreePool pExFreePool)
{
  PEPROCESS Process;
  NTSTATUS Status;
  PMDL Mdl;

  Status = pObReferenceObjectByHandle(ProcessHandle,
    PROCESS_VM_WRITE,
    NULL,
    UserMode,
    (PVOID*)(&Process),
    NULL);
  if (!NT_SUCCESS(Status))
    return(Status);

  Mdl = pMmCreateMdl(NULL,
    BaseAddress,
    NumberOfBytesToLock);
  if (Mdl == NULL)
    {
      pObDereferenceObject(Process);
      return(STATUS_NO_MEMORY);
    }

  pMmProbeAndLockPages(Mdl,
    UserMode,
    IoWriteAccess);

  pExFreePool(Mdl);

  pObDereferenceObject(Process);

  *NumberOfBytesLocked = NumberOfBytesToLock;
  return(STATUS_SUCCESS);
}


NTSTATUS STDCALL
NtLockVirtualMemory(HANDLE ProcessHandle,
  PVOID BaseAddress,
  ULONG NumberOfBytesToLock,
  PULONG NumberOfBytesLocked)
{
  DPRINT("NtLockVirtualMemory(ProcessHandle %x, BaseAddress %x, "
    "NumberOfBytesToLock %d, NumberOfBytesLocked %x)\n",
    ProcessHandle,
    BaseAddress,
    NumberOfBytesToLock,
    NumberOfBytesLocked);

  return NtLockVirtualMemoryInternal(ProcessHandle,
    BaseAddress,
    NumberOfBytesToLock,
    NumberOfBytesLocked,
    ObReferenceObjectByHandle,
    MmCreateMdl,
    ObfDereferenceObject,
    MmProbeAndLockPages,
    ExFreePool);
}


/* (tMk 2004.II.4)
 * FUNCTION: 
 * Called from VirtualQueryEx (lib\kernel32\mem\virtual.c)
 *
 */
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
   KPROCESSOR_MODE PrevMode;
   union
   {
      MEMORY_BASIC_INFORMATION BasicInfo;
   }
   VirtualMemoryInfo;

   DPRINT("NtQueryVirtualMemory(ProcessHandle %x, Address %x, "
          "VirtualMemoryInformationClass %d, VirtualMemoryInformation %x, "
          "Length %lu ResultLength %x)\n",ProcessHandle,Address,
          VirtualMemoryInformationClass,VirtualMemoryInformation,
          Length,ResultLength);

   PrevMode =  ExGetPreviousMode();

   if (PrevMode == UserMode && Address >= (PVOID)KERNEL_BASE)
   {
      return STATUS_INVALID_PARAMETER;
   }

   if (Address < (PVOID)KERNEL_BASE)
   {
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
   }
   else
   {
      AddressSpace = MmGetKernelAddressSpace();
   }
   MmLockAddressSpace(AddressSpace);
   MemoryArea = MmOpenMemoryAreaByAddress(AddressSpace,
                                          Address);
   switch(VirtualMemoryInformationClass)
   {
      case MemoryBasicInformation:
         {
	    PMEMORY_BASIC_INFORMATION Info = &VirtualMemoryInfo.BasicInfo;
            if (Length != sizeof(MEMORY_BASIC_INFORMATION))
            {
               MmUnlockAddressSpace(AddressSpace);
               ObDereferenceObject(Process);
               return(STATUS_INFO_LENGTH_MISMATCH);
            }
            
            if (MemoryArea == NULL)
            {
	       Info->Type = 0;
               Info->State = MEM_FREE;
	       Info->Protect = PAGE_NOACCESS;
	       Info->AllocationProtect = 0;
               Info->BaseAddress = (PVOID)PAGE_ROUND_DOWN(Address);
	       Info->AllocationBase = NULL;
	       Info->RegionSize = MmFindGapAtAddress(AddressSpace, Info->BaseAddress);
               Status = STATUS_SUCCESS;
               ResultLength = sizeof(MEMORY_BASIC_INFORMATION);
	    }
            else 
	    {
	       switch(MemoryArea->Type)
	       {
		  case MEMORY_AREA_VIRTUAL_MEMORY:
                     Status = MmQueryAnonMem(MemoryArea, Address, Info,
                                             &ResultLength);
		     break;
	          case MEMORY_AREA_SECTION_VIEW:
                     Status = MmQuerySectionView(MemoryArea, Address, Info,
                                                 &ResultLength);
                     break;
		  case MEMORY_AREA_NO_ACCESS:
	             Info->Type = 0;
                     Info->State = MEM_FREE;
	             Info->Protect = MemoryArea->Attributes;
		     Info->AllocationProtect = MemoryArea->Attributes;
                     Info->BaseAddress = MemoryArea->BaseAddress;
	             Info->AllocationBase = MemoryArea->BaseAddress;
	             Info->RegionSize = MemoryArea->Length;
                     Status = STATUS_SUCCESS;
                     ResultLength = sizeof(MEMORY_BASIC_INFORMATION);
	             break;
		  case MEMORY_AREA_SHARED_DATA:
	             Info->Type = 0;
                     Info->State = MEM_COMMIT;
	             Info->Protect = MemoryArea->Attributes;
		     Info->AllocationProtect = MemoryArea->Attributes;
                     Info->BaseAddress = MemoryArea->BaseAddress;
	             Info->AllocationBase = MemoryArea->BaseAddress;
	             Info->RegionSize = MemoryArea->Length;
                     Status = STATUS_SUCCESS;
                     ResultLength = sizeof(MEMORY_BASIC_INFORMATION);
		     break;
		  default:
	             Status = STATUS_UNSUCCESSFUL;
                     ResultLength = 0;
	       }
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
   if (Address < (PVOID)KERNEL_BASE)
   {
      ObDereferenceObject(Process);
   }

   if (NT_SUCCESS(Status) && ResultLength > 0)
   {
      Status = MmCopyToCaller(VirtualMemoryInformation, &VirtualMemoryInfo, ResultLength);
      if (!NT_SUCCESS(Status))
      {
         ResultLength = 0;
      }
   }
   
   if (UnsafeResultLength != NULL)
   {
      MmCopyToCaller(UnsafeResultLength, &ResultLength, sizeof(ULONG));
   }
   return(Status);
}


/* (tMk 2004.II.5)
 * FUNCTION: 
 * Called from VirtualProtectEx (lib\kernel32\mem\virtual.c)
 *
 */
NTSTATUS STDCALL
NtProtectVirtualMemory(IN HANDLE ProcessHandle,
                       IN PVOID *UnsafeBaseAddress,
                       IN ULONG *UnsafeNumberOfBytesToProtect,
                       IN ULONG NewAccessProtection,
                       OUT PULONG UnsafeOldAccessProtection)
{
   PMEMORY_AREA MemoryArea;
   PEPROCESS Process;
   NTSTATUS Status;
   PMADDRESS_SPACE AddressSpace;
   ULONG OldAccessProtection;
   PVOID BaseAddress;
   ULONG NumberOfBytesToProtect;

   Status = MmCopyFromCaller(&BaseAddress, UnsafeBaseAddress, sizeof(PVOID));
   if (!NT_SUCCESS(Status))
      return Status;
   Status = MmCopyFromCaller(&NumberOfBytesToProtect, UnsafeNumberOfBytesToProtect, sizeof(ULONG));
   if (!NT_SUCCESS(Status))
      return Status;

   // (tMk 2004.II.5) in Microsoft SDK I read: 
   // 'if this parameter is NULL or does not point to a valid variable, the function fails'
   if(UnsafeOldAccessProtection == NULL) 
   {
      return(STATUS_INVALID_PARAMETER);
   }
   
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
   if (!NT_SUCCESS(Status))
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

   MmCopyToCaller(UnsafeOldAccessProtection, &OldAccessProtection, sizeof(ULONG));
   MmCopyToCaller(UnsafeBaseAddress, &BaseAddress, sizeof(PVOID));
   MmCopyToCaller(UnsafeNumberOfBytesToProtect, &NumberOfBytesToProtect, sizeof(ULONG));

   return(Status);
}


/* (tMk 2004.II.05)
 * FUNCTION: 
 * Called from ReadProcessMemory (lib\kernel32\mem\procmem.c) and KlInitPeb(lib\kernel32\process\create.c)
 *
 * NOTE: This function will be correct if MmProbeAndLockPages() would be fully IMPLEMENTED.
 */
NTSTATUS STDCALL
NtReadVirtualMemory(IN HANDLE ProcessHandle,
                    IN PVOID BaseAddress,
                    OUT PVOID Buffer,
                    IN ULONG NumberOfBytesToRead,
                    OUT PULONG NumberOfBytesRead OPTIONAL)
{
   NTSTATUS Status;
   PMDL Mdl;
   PVOID SystemAddress;
   PEPROCESS Process, CurrentProcess;


   DPRINT("NtReadVirtualMemory(ProcessHandle %x, BaseAddress %x, "
          "Buffer %x, NumberOfBytesToRead %d)\n",ProcessHandle,BaseAddress,
          Buffer,NumberOfBytesToRead);

   Status = ObReferenceObjectByHandle(ProcessHandle,
                                      PROCESS_VM_WRITE,
                                      NULL,
                                      UserMode,
                                      (PVOID*)(&Process),
                                      NULL);
   if (!NT_SUCCESS(Status))
   {
      return(Status);
   }

   CurrentProcess = PsGetCurrentProcess();

   if (Process == CurrentProcess)
   {
      memcpy(Buffer, BaseAddress, NumberOfBytesToRead);
   }
   else
   {
      Mdl = MmCreateMdl(NULL,
                        Buffer,
                        NumberOfBytesToRead);
      if(Mdl == NULL) 
      {
         ObDereferenceObject(Process);
         return(STATUS_NO_MEMORY);
      }
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
   }

   ObDereferenceObject(Process);

   if (NumberOfBytesRead)
      *NumberOfBytesRead = NumberOfBytesToRead;
   return(STATUS_SUCCESS);
}

/* (tMk 2004.II.05)
 * FUNCTION:  THIS function doesn't make a sense...
 * Called from VirtualUnlock (lib\kernel32\mem\virtual.c)
 */
NTSTATUS STDCALL
NtUnlockVirtualMemory(HANDLE ProcessHandle,
                      PVOID BaseAddress,
                      ULONG NumberOfBytesToUnlock,
                      PULONG NumberOfBytesUnlocked OPTIONAL)
{
   // AG [08-20-03] : I have *no* idea if this is correct, I just used the
   // other functions as a template and made a few intelligent guesses...

   NTSTATUS Status;
   PMDL Mdl;
   PEPROCESS Process;

   DPRINT("NtUnlockVirtualMemory(ProcessHandle %x, BaseAddress %x, "
          "NumberOfBytesToUnlock %d), NumberOfBytesUnlocked %x\n",ProcessHandle,BaseAddress,
          NumberOfBytesToUnlock, NumberOfBytesUnlocked);

   Status = ObReferenceObjectByHandle(ProcessHandle,
                                      PROCESS_VM_WRITE,
                                      NULL,
                                      UserMode,
                                      (PVOID*)(&Process),
                                      NULL);
   if (!NT_SUCCESS(Status))
   {
      return(Status);
   }

   Mdl = MmCreateMdl(NULL,
                     BaseAddress,
                     NumberOfBytesToUnlock);
   if(Mdl == NULL) 
   {
      ObDereferenceObject(Process);
      return(STATUS_NO_MEMORY);
   }

   ObDereferenceObject(Process);

   if (Mdl->MappedSystemVa != NULL)
   {
      MmUnmapLockedPages(Mdl->MappedSystemVa, Mdl);
   }
   MmUnlockPages(Mdl);
   ExFreePool(Mdl);

   *NumberOfBytesUnlocked = NumberOfBytesToUnlock;

   return(STATUS_SUCCESS);
}


/* (tMk 2004.II.05)
 * FUNCTION:
 * Called from WriteProcessMemory (lib\kernel32\mem\procmem.c) and KlInitPeb(lib\kernel32\process\create.c)
 * 
 * NOTE: This function will be correct if MmProbeAndLockPages() would be fully IMPLEMENTED.
 */
NTSTATUS STDCALL
NtWriteVirtualMemory(IN HANDLE ProcessHandle,
                     IN PVOID BaseAddress,
                     IN PVOID Buffer,
                     IN ULONG NumberOfBytesToWrite,
                     OUT PULONG NumberOfBytesWritten)
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
   if (!NT_SUCCESS(Status))
   {
      return(Status);
   }

   if (Process == PsGetCurrentProcess())
   {
      memcpy(BaseAddress, Buffer, NumberOfBytesToWrite);
   }
   else
   {
      Mdl = MmCreateMdl(NULL,
                        Buffer,
                        NumberOfBytesToWrite);
      MmProbeAndLockPages(Mdl,
                          UserMode,
                          IoReadAccess);
      if(Mdl == NULL)
      {
         ObDereferenceObject(Process);
         return(STATUS_NO_MEMORY);
      }
      KeAttachProcess(Process);

      SystemAddress = MmGetSystemAddressForMdl(Mdl);
      memcpy(BaseAddress, SystemAddress, NumberOfBytesToWrite);

      KeDetachProcess();

      if (Mdl->MappedSystemVa != NULL)
      {
         MmUnmapLockedPages(Mdl->MappedSystemVa, Mdl);
      }
      MmUnlockPages(Mdl);
      ExFreePool(Mdl);
   }

   ObDereferenceObject(Process);

   *NumberOfBytesWritten = NumberOfBytesToWrite;

   return(STATUS_SUCCESS);
}

/*
 * @unimplemented
 */

PVOID
STDCALL
MmGetVirtualForPhysical (
    IN PHYSICAL_ADDRESS PhysicalAddress
    )
{
	UNIMPLEMENTED;
	return 0;
}

/* FUNCTION:
 * Called from EngSecureMem (subsys\win32k\eng\mem.c)
 * @unimplemented
 */
PVOID STDCALL
MmSecureVirtualMemory (PVOID  Address,
                       SIZE_T Length,
                       ULONG  Mode)
{
   /* Only works for user space */
   if (MmHighestUserAddress < Address)
   {
      return NULL;
   }

   UNIMPLEMENTED;

   return 0;
}


/* FUNCTION:
 * Called from EngUnsecureMem (subsys\win32k\eng\mem.c)
 * @unimplemented
 */
VOID STDCALL
MmUnsecureVirtualMemory(PVOID SecureMem)
{
   if (NULL == SecureMem)
   {
      return;
   }

   UNIMPLEMENTED;
}


/*
 * @implemented
 */
VOID STDCALL
ProbeForRead (IN PVOID Address,
              IN ULONG Length,
              IN ULONG Alignment)
{
   ASSERT(Alignment ==1 || Alignment == 2 || Alignment == 4 || Alignment == 8);

   if (Length == 0)
      return;

   if (((ULONG_PTR)Address & (Alignment - 1)) != 0)
   {
      ExRaiseStatus (STATUS_DATATYPE_MISALIGNMENT);
   }
   else if ((ULONG_PTR)Address + Length < (ULONG_PTR)Address ||
            (ULONG_PTR)Address + Length > (ULONG_PTR)MmUserProbeAddress)
   {
      ExRaiseStatus (STATUS_ACCESS_VIOLATION);
   }
}


/*
 * @implemented
 */
VOID STDCALL
ProbeForWrite (IN PVOID Address,
               IN ULONG Length,
               IN ULONG Alignment)
{
   PULONG Ptr;
   ULONG x;
   ULONG i;

   ASSERT(Alignment ==1 || Alignment == 2 || Alignment == 4 || Alignment == 8);

   if (Length == 0)
      return;

   if (((ULONG_PTR)Address & (Alignment - 1)) != 0)
   {
      ExRaiseStatus (STATUS_DATATYPE_MISALIGNMENT);
   }
   else if ((ULONG_PTR)Address + Length < (ULONG_PTR)Address ||
            (ULONG_PTR)Address + Length > (ULONG_PTR)MmUserProbeAddress)
   {
      ExRaiseStatus (STATUS_ACCESS_VIOLATION);
   }

   /* Check for accessible pages */
   for (i = 0; i < Length; i += PAGE_SIZE)
   {
      Ptr = (PULONG)(((ULONG_PTR)Address & ~(PAGE_SIZE - 1)) + i);
      x = *Ptr;
      *Ptr = x;
   }
}

/* EOF */
