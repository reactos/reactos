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
/* $Id$
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
  /* This should be implemented once we support network filesystems */
  DPRINT("NtFlushVirtualMemory is UNIMPLEMENTED\n");
  return(STATUS_SUCCESS);
}


NTSTATUS STDCALL
MiLockVirtualMemory(HANDLE ProcessHandle,
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

  return MiLockVirtualMemory(ProcessHandle,
    BaseAddress,
    NumberOfBytesToLock,
    NumberOfBytesLocked,
    ObReferenceObjectByHandle,
    MmCreateMdl,
    ObfDereferenceObject,
    MmProbeAndLockPages,
    ExFreePool);
}


NTSTATUS FASTCALL
MiQueryVirtualMemory (IN HANDLE ProcessHandle,
                      IN PVOID Address,
                      IN CINT VirtualMemoryInformationClass,
                      OUT PVOID VirtualMemoryInformation,
                      IN ULONG Length,
                      OUT PULONG ResultLength)
{
   NTSTATUS Status;
   PEPROCESS Process;
   MEMORY_AREA* MemoryArea;
   PMADDRESS_SPACE AddressSpace;

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
   MemoryArea = MmLocateMemoryAreaByAddress(AddressSpace, Address);
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
	       Info->Type = 0;
               Info->State = MEM_FREE;
	       Info->Protect = PAGE_NOACCESS;
	       Info->AllocationProtect = 0;
               Info->BaseAddress = (PVOID)PAGE_ROUND_DOWN(Address);
	       Info->AllocationBase = NULL;
	       Info->RegionSize = MmFindGapAtAddress(AddressSpace, Info->BaseAddress);
               Status = STATUS_SUCCESS;
               *ResultLength = sizeof(MEMORY_BASIC_INFORMATION);
	    }
            else
	    {
	       switch(MemoryArea->Type)
	       {
		  case MEMORY_AREA_VIRTUAL_MEMORY:
                     Status = MmQueryAnonMem(MemoryArea, Address, Info,
                                             ResultLength);
		     break;
	          case MEMORY_AREA_SECTION_VIEW:
                     Status = MmQuerySectionView(MemoryArea, Address, Info,
                                                 ResultLength);
                     break;
		  case MEMORY_AREA_NO_ACCESS:
	             Info->Type = 0;
                     Info->State = MEM_FREE;
	             Info->Protect = MemoryArea->Attributes;
		     Info->AllocationProtect = MemoryArea->Attributes;
	             Info->BaseAddress = MemoryArea->StartingAddress;
	             Info->AllocationBase = MemoryArea->StartingAddress;
	             Info->RegionSize = (ULONG_PTR)MemoryArea->EndingAddress - 
	                                (ULONG_PTR)MemoryArea->StartingAddress;
                     Status = STATUS_SUCCESS;
                     *ResultLength = sizeof(MEMORY_BASIC_INFORMATION);
	             break;
		  case MEMORY_AREA_SHARED_DATA:
	             Info->Type = 0;
                     Info->State = MEM_COMMIT;
	             Info->Protect = MemoryArea->Attributes;
		     Info->AllocationProtect = MemoryArea->Attributes;
	             Info->BaseAddress = MemoryArea->StartingAddress;
	             Info->AllocationBase = MemoryArea->StartingAddress;
	             Info->RegionSize = (ULONG_PTR)MemoryArea->EndingAddress - 
	                                (ULONG_PTR)MemoryArea->StartingAddress;
                     Status = STATUS_SUCCESS;
                     *ResultLength = sizeof(MEMORY_BASIC_INFORMATION);
		     break;
		  case MEMORY_AREA_SYSTEM:
		     {
			static int warned = 0;
			if ( !warned )
			{
			  DPRINT1("FIXME: MEMORY_AREA_SYSTEM case incomplete (or possibly wrong) for NtQueryVirtualMemory()\n");
			  warned = 1;
			}
		     }
		     /* FIXME - don't have a clue if this is right, but it's better than nothing */
	             Info->Type = 0;
                     Info->State = MEM_COMMIT;
	             Info->Protect = MemoryArea->Attributes;
		     Info->AllocationProtect = MemoryArea->Attributes;
	             Info->BaseAddress = MemoryArea->StartingAddress;
	             Info->AllocationBase = MemoryArea->StartingAddress;
	             Info->RegionSize = (ULONG_PTR)MemoryArea->EndingAddress - 
	                                (ULONG_PTR)MemoryArea->StartingAddress;
                     Status = STATUS_SUCCESS;
                     *ResultLength = sizeof(MEMORY_BASIC_INFORMATION);
		     break;
		  case MEMORY_AREA_KERNEL_STACK:
		     {
			static int warned = 0;
			if ( !warned )
			{
			  DPRINT1("FIXME: MEMORY_AREA_KERNEL_STACK case incomplete (or possibly wrong) for NtQueryVirtualMemory()\n");
			  warned = 1;
			}
		     }
		     /* FIXME - don't have a clue if this is right, but it's better than nothing */
	             Info->Type = 0;
                     Info->State = MEM_COMMIT;
	             Info->Protect = MemoryArea->Attributes;
		     Info->AllocationProtect = MemoryArea->Attributes;
	             Info->BaseAddress = MemoryArea->StartingAddress;
	             Info->AllocationBase = MemoryArea->StartingAddress;
	             Info->RegionSize = (ULONG_PTR)MemoryArea->EndingAddress - 
	                                (ULONG_PTR)MemoryArea->StartingAddress;
                     Status = STATUS_SUCCESS;
                     *ResultLength = sizeof(MEMORY_BASIC_INFORMATION);
		     break;
		  default:
		     DPRINT1("unhandled memory area type: 0x%x\n", MemoryArea->Type);
	             Status = STATUS_UNSUCCESSFUL;
                     *ResultLength = 0;
	       }
	    }
            break;
         }

      default:
         {
            Status = STATUS_INVALID_INFO_CLASS;
            *ResultLength = 0;
            break;
         }
   }

   MmUnlockAddressSpace(AddressSpace);
   if (Address < (PVOID)KERNEL_BASE)
   {
      ObDereferenceObject(Process);
   }

   return Status;
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
   ULONG ResultLength = 0;
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
      DPRINT1("Invalid parameter\n");
      return STATUS_INVALID_PARAMETER;
   }

   Status = MiQueryVirtualMemory ( ProcessHandle,
       Address,
       VirtualMemoryInformationClass,
       &VirtualMemoryInfo,
       Length,
       &ResultLength );

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


NTSTATUS STDCALL
MiProtectVirtualMemory(IN PEPROCESS Process,
                       IN OUT PVOID *BaseAddress,
                       IN OUT PULONG NumberOfBytesToProtect,
                       IN ULONG NewAccessProtection,
                       OUT PULONG OldAccessProtection  OPTIONAL)
{
   PMEMORY_AREA MemoryArea;
   PMADDRESS_SPACE AddressSpace;
   ULONG OldAccessProtection_;
   NTSTATUS Status;

   *NumberOfBytesToProtect =
      PAGE_ROUND_UP((*BaseAddress) + (*NumberOfBytesToProtect)) -
      PAGE_ROUND_DOWN(*BaseAddress);
   *BaseAddress = (PVOID)PAGE_ROUND_DOWN(*BaseAddress);

   AddressSpace = &Process->AddressSpace;

   MmLockAddressSpace(AddressSpace);
   MemoryArea = MmLocateMemoryAreaByAddress(AddressSpace, *BaseAddress);
   if (MemoryArea == NULL)
   {
      MmUnlockAddressSpace(AddressSpace);
      return STATUS_UNSUCCESSFUL;
   }

   if (OldAccessProtection == NULL)
      OldAccessProtection = &OldAccessProtection_;

   if (MemoryArea->Type == MEMORY_AREA_VIRTUAL_MEMORY)
   {
      Status = MmProtectAnonMem(AddressSpace, MemoryArea, *BaseAddress,
                                *NumberOfBytesToProtect, NewAccessProtection,
                                OldAccessProtection);
   }
   else if (MemoryArea->Type == MEMORY_AREA_SECTION_VIEW)
   {
      Status = MmProtectSectionView(AddressSpace, MemoryArea, *BaseAddress,
                                    *NumberOfBytesToProtect,
                                    NewAccessProtection,
                                    OldAccessProtection);
   }
   else
   {
      /* FIXME: Should we return failure or success in this case? */
      Status = STATUS_SUCCESS;
   }

   MmUnlockAddressSpace(AddressSpace);

   return Status;
}


/* (tMk 2004.II.5)
 * FUNCTION:
 * Called from VirtualProtectEx (lib\kernel32\mem\virtual.c)
 *
 */
NTSTATUS STDCALL
NtProtectVirtualMemory(IN HANDLE ProcessHandle,
                       IN OUT PVOID *UnsafeBaseAddress,
                       IN OUT ULONG *UnsafeNumberOfBytesToProtect,
                       IN ULONG NewAccessProtection,
                       OUT PULONG UnsafeOldAccessProtection)
{
   PEPROCESS Process;
   NTSTATUS Status;
   ULONG OldAccessProtection;
   PVOID BaseAddress;
   ULONG NumberOfBytesToProtect;

   Status = MmCopyFromCaller(&BaseAddress, UnsafeBaseAddress, sizeof(PVOID));
   if (!NT_SUCCESS(Status))
      return Status;
   Status = MmCopyFromCaller(&NumberOfBytesToProtect, UnsafeNumberOfBytesToProtect, sizeof(ULONG));
   if (!NT_SUCCESS(Status))
      return Status;

   /* (tMk 2004.II.5) in Microsoft SDK I read:
    * 'if this parameter is NULL or does not point to a valid variable, the function fails'
    */
   if(UnsafeOldAccessProtection == NULL)
   {
      return(STATUS_INVALID_PARAMETER);
   }

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

   Status = MiProtectVirtualMemory(Process,
                                   &BaseAddress,
                                   &NumberOfBytesToProtect,
                                   NewAccessProtection,
                                   &OldAccessProtection);

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

      KeAttachProcess(&Process->Pcb);

      SystemAddress = MmGetSystemAddressForMdl(Mdl);

        Status = STATUS_SUCCESS;
        _SEH_TRY {
            ProbeForRead(BaseAddress, NumberOfBytesToRead, 1);
            Status = STATUS_PARTIAL_COPY;
            memcpy(SystemAddress, BaseAddress, NumberOfBytesToRead);
            Status = STATUS_SUCCESS;
        } _SEH_HANDLE {
            if(Status != STATUS_PARTIAL_COPY)
                Status = _SEH_GetExceptionCode();
        } _SEH_END;

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
   return(Status);
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
                     OUT PULONG NumberOfBytesWritten  OPTIONAL)
{
   NTSTATUS Status;
   PMDL Mdl;
   PVOID SystemAddress;
   PEPROCESS Process;
   ULONG OldProtection = 0;
   PVOID ProtectBaseAddress;
   ULONG ProtectNumberOfBytes;

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

   /* We have to make sure the target memory is writable.
    *
    * I am not sure if it is correct to do this in any case, but it has to be
    * done at least in some cases because you can use WriteProcessMemory to
    * write into the .text section of a module where memcpy() would crash.
    *  -blight (2005/01/09)
    */
   ProtectBaseAddress = BaseAddress;
   ProtectNumberOfBytes = NumberOfBytesToWrite;

   /* Write memory */
   if (Process == PsGetCurrentProcess())
   {
      Status = MiProtectVirtualMemory(Process,
                                      &ProtectBaseAddress,
                                      &ProtectNumberOfBytes,
                                      PAGE_READWRITE,
                                      &OldProtection);
      if (!NT_SUCCESS(Status))
      {
         ObDereferenceObject(Process);
         return Status;
      }
      memcpy(BaseAddress, Buffer, NumberOfBytesToWrite);
   }
   else
   {
      /* Create MDL describing the source buffer. */
      Mdl = MmCreateMdl(NULL,
                        Buffer,
                        NumberOfBytesToWrite);
      if(Mdl == NULL)
      {
         ObDereferenceObject(Process);
         return(STATUS_NO_MEMORY);
      }

      /* Make the target area writable. */
      Status = MiProtectVirtualMemory(Process,
                                      &ProtectBaseAddress,
                                      &ProtectNumberOfBytes,
                                      PAGE_READWRITE,
                                      &OldProtection);
      if (!NT_SUCCESS(Status))
      {
         ObDereferenceObject(Process);
         ExFreePool(Mdl);
         return Status;
      }

      /* Map the MDL. */
      MmProbeAndLockPages(Mdl,
                          UserMode,
                          IoReadAccess);

      /* Copy memory from the mapped MDL into the target buffer. */
      KeAttachProcess(&Process->Pcb);

      SystemAddress = MmGetSystemAddressForMdl(Mdl);
      memcpy(BaseAddress, SystemAddress, NumberOfBytesToWrite);

      KeDetachProcess();

      /* Free the MDL. */
      if (Mdl->MappedSystemVa != NULL)
      {
         MmUnmapLockedPages(Mdl->MappedSystemVa, Mdl);
      }
      MmUnlockPages(Mdl);
      ExFreePool(Mdl);
   }

   /* Reset the protection of the target memory. */
   Status = MiProtectVirtualMemory(Process,
                                   &ProtectBaseAddress,
                                   &ProtectNumberOfBytes,
                                   OldProtection,
                                   &OldProtection);
   if (!NT_SUCCESS(Status))
   {
      DPRINT1("Failed to reset protection of the target memory! (Status 0x%x)\n", Status);
      /* FIXME: Should we bugcheck here? */
   }

   ObDereferenceObject(Process);

   if (NumberOfBytesWritten != NULL)
      MmCopyToCaller(NumberOfBytesWritten, &NumberOfBytesToWrite, sizeof(ULONG));

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
ProbeForRead (IN CONST VOID *Address,
              IN ULONG Length,
              IN ULONG Alignment)
{
   ASSERT(Alignment == 1 || Alignment == 2 || Alignment == 4 || Alignment == 8);

   if (Length == 0)
      return;

   if (((ULONG_PTR)Address & (Alignment - 1)) != 0)
   {
      ExRaiseStatus (STATUS_DATATYPE_MISALIGNMENT);
   }
   else if ((ULONG_PTR)Address + Length - 1 < (ULONG_PTR)Address ||
            (ULONG_PTR)Address + Length - 1 > (ULONG_PTR)MmUserProbeAddress)
   {
      ExRaiseStatus (STATUS_ACCESS_VIOLATION);
   }
}


/*
 * @implemented
 */
VOID STDCALL
ProbeForWrite (IN CONST VOID *Address,
               IN ULONG Length,
               IN ULONG Alignment)
{
   volatile CHAR *Current;
   PCHAR Last;

   ASSERT(Alignment == 1 || Alignment == 2 || Alignment == 4 || Alignment == 8);

   if (Length == 0)
      return;

   if (((ULONG_PTR)Address & (Alignment - 1)) != 0)
   {
      ExRaiseStatus (STATUS_DATATYPE_MISALIGNMENT);
   }

   Last = (PCHAR)((ULONG_PTR)Address + Length - 1);
   if ((ULONG_PTR)Last < (ULONG_PTR)Address ||
       (ULONG_PTR)Last > (ULONG_PTR)MmUserProbeAddress)
   {
      ExRaiseStatus (STATUS_ACCESS_VIOLATION);
   }

   /* Check for accessible pages */
   Current = (CHAR*)Address;
   do
   {
     *Current = *Current;
     Current = (CHAR*)((ULONG_PTR)Current + PAGE_SIZE);
   } while (Current <= Last);
}

/* EOF */
