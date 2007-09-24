/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/mm/virtual.c
 * PURPOSE:         Implementing operations on virtual memory.
 *
 * PROGRAMMERS:     David Welch
 */

/* INCLUDE *****************************************************************/

#include <ntoskrnl.h>

#define NDEBUG
#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

NTSTATUS STDCALL
NtFlushVirtualMemory(IN HANDLE ProcessHandle,
                     IN OUT PVOID *BaseAddress,
                     IN OUT PSIZE_T NumberOfBytesToFlush,
                     OUT PIO_STATUS_BLOCK IoStatusBlock)
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
    (PVOID)ObfDereferenceObject,
    MmProbeAndLockPages,
    ExFreePool);
}


NTSTATUS FASTCALL
MiQueryVirtualMemory (IN HANDLE ProcessHandle,
                      IN PVOID Address,
                      IN MEMORY_INFORMATION_CLASS VirtualMemoryInformationClass,
                      OUT PVOID VirtualMemoryInformation,
                      IN ULONG Length,
                      OUT PULONG ResultLength)
{
   NTSTATUS Status;
   PEPROCESS Process;
   MEMORY_AREA* MemoryArea;
   PMADDRESS_SPACE AddressSpace;

   if (Address < MmSystemRangeStart)
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
      AddressSpace = (PMADDRESS_SPACE)&Process->VadRoot;
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
                  case MEMORY_AREA_PEB_OR_TEB:
                     Status = MmQueryAnonMem(MemoryArea, Address, Info,
                                             ResultLength);
		     break;
	          case MEMORY_AREA_SECTION_VIEW:
                     Status = MmQuerySectionView(MemoryArea, Address, Info,
                                                 ResultLength);
                     break;
		  case MEMORY_AREA_NO_ACCESS:
	             Info->Type = MEM_PRIVATE;
                     Info->State = MEM_RESERVE;
	             Info->Protect = MemoryArea->Protect;
		     Info->AllocationProtect = MemoryArea->Protect;
	             Info->BaseAddress = MemoryArea->StartingAddress;
	             Info->AllocationBase = MemoryArea->StartingAddress;
	             Info->RegionSize = (ULONG_PTR)MemoryArea->EndingAddress -
	                                (ULONG_PTR)MemoryArea->StartingAddress;
                     Status = STATUS_SUCCESS;
                     *ResultLength = sizeof(MEMORY_BASIC_INFORMATION);
	             break;
		  case MEMORY_AREA_SHARED_DATA:
	             Info->Type = MEM_PRIVATE;
                     Info->State = MEM_COMMIT;
	             Info->Protect = MemoryArea->Protect;
		     Info->AllocationProtect = MemoryArea->Protect;
	             Info->BaseAddress = MemoryArea->StartingAddress;
	             Info->AllocationBase = MemoryArea->StartingAddress;
	             Info->RegionSize = (ULONG_PTR)MemoryArea->EndingAddress -
	                                (ULONG_PTR)MemoryArea->StartingAddress;
                     Status = STATUS_SUCCESS;
                     *ResultLength = sizeof(MEMORY_BASIC_INFORMATION);
		     break;
		  case MEMORY_AREA_SYSTEM:
	             Info->Type = 0;
                     Info->State = MEM_COMMIT;
	             Info->Protect = MemoryArea->Protect;
		     Info->AllocationProtect = MemoryArea->Protect;
	             Info->BaseAddress = MemoryArea->StartingAddress;
	             Info->AllocationBase = MemoryArea->StartingAddress;
	             Info->RegionSize = (ULONG_PTR)MemoryArea->EndingAddress -
	                                (ULONG_PTR)MemoryArea->StartingAddress;
                     Status = STATUS_SUCCESS;
                     *ResultLength = sizeof(MEMORY_BASIC_INFORMATION);
		     break;
		  case MEMORY_AREA_KERNEL_STACK:
	             Info->Type = 0;
                     Info->State = MEM_COMMIT;
	             Info->Protect = MemoryArea->Protect;
		     Info->AllocationProtect = MemoryArea->Protect;
	             Info->BaseAddress = MemoryArea->StartingAddress;
	             Info->AllocationBase = MemoryArea->StartingAddress;
	             Info->RegionSize = (ULONG_PTR)MemoryArea->EndingAddress -
	                                (ULONG_PTR)MemoryArea->StartingAddress;
                     Status = STATUS_SUCCESS;
                     *ResultLength = sizeof(MEMORY_BASIC_INFORMATION);
		     break;
                  case MEMORY_AREA_PAGED_POOL:
	             Info->Type = 0;
                     Info->State = MEM_COMMIT;
	             Info->Protect = MemoryArea->Protect;
		     Info->AllocationProtect = MemoryArea->Protect;
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
   if (Address < MmSystemRangeStart)
   {
       ASSERT(Process);
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
                      IN MEMORY_INFORMATION_CLASS VirtualMemoryInformationClass,
                      OUT PVOID VirtualMemoryInformation,
                      IN ULONG Length,
                      OUT PULONG UnsafeResultLength)
{
   NTSTATUS Status = STATUS_SUCCESS;
   ULONG ResultLength = 0;
   KPROCESSOR_MODE PreviousMode;
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

   PreviousMode =  ExGetPreviousMode();
   
   if (PreviousMode != KernelMode && UnsafeResultLength != NULL)
     {
       _SEH_TRY
         {
           ProbeForWriteUlong(UnsafeResultLength);
         }
       _SEH_HANDLE
         {
           Status = _SEH_GetExceptionCode();
         }
       _SEH_END;
       
       if (!NT_SUCCESS(Status))
         {
           return Status;
         }
     }

   if (Address >= MmSystemRangeStart)
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

   if (NT_SUCCESS(Status))
   {
      if (PreviousMode != KernelMode)
        {
          _SEH_TRY
            {
              if (ResultLength > 0)
                {
                  ProbeForWrite(VirtualMemoryInformation,
                                ResultLength,
                                1);
                  RtlCopyMemory(VirtualMemoryInformation,
                                &VirtualMemoryInfo,
                                ResultLength);
                }
              if (UnsafeResultLength != NULL)
                {
                  *UnsafeResultLength = ResultLength;
                }
            }
          _SEH_HANDLE
            {
              Status = _SEH_GetExceptionCode();
            }
          _SEH_END;
        }
      else
        {
          if (ResultLength > 0)
            {
              RtlCopyMemory(VirtualMemoryInformation,
                            &VirtualMemoryInfo,
                            ResultLength);
            }

          if (UnsafeResultLength != NULL)
            {
              *UnsafeResultLength = ResultLength;
            }
        }
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

   AddressSpace = (PMADDRESS_SPACE)&(Process)->VadRoot;

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
      Status = STATUS_CONFLICTING_ADDRESSES;
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
   ULONG OldAccessProtection;
   PVOID BaseAddress = NULL;
   ULONG NumberOfBytesToProtect = 0;
   KPROCESSOR_MODE PreviousMode;
   NTSTATUS Status = STATUS_SUCCESS;
   
   PreviousMode = ExGetPreviousMode();
   
   if (PreviousMode != KernelMode)
     {
       _SEH_TRY
         {
           ProbeForWritePointer(UnsafeBaseAddress);
           ProbeForWriteUlong(UnsafeNumberOfBytesToProtect);
           ProbeForWriteUlong(UnsafeOldAccessProtection);

           BaseAddress = *UnsafeBaseAddress;
           NumberOfBytesToProtect = *UnsafeNumberOfBytesToProtect;
         }
       _SEH_HANDLE
         {
           Status = _SEH_GetExceptionCode();
         }
       _SEH_END;
       
       if (!NT_SUCCESS(Status))
         {
           return Status;
         }
     }
   else
     {
       BaseAddress = *UnsafeBaseAddress;
       NumberOfBytesToProtect = *UnsafeNumberOfBytesToProtect;
     }

   if ((ULONG_PTR)BaseAddress + NumberOfBytesToProtect - 1 < (ULONG_PTR)BaseAddress ||
       (ULONG_PTR)BaseAddress + NumberOfBytesToProtect - 1 >= MmUserProbeAddress)
     {
       /* Don't allow to change the protection of a kernel mode address */
       return STATUS_INVALID_PARAMETER_2;
     }

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

   if (PreviousMode != KernelMode)
     {
       _SEH_TRY
         {
           *UnsafeOldAccessProtection = OldAccessProtection;
           *UnsafeBaseAddress = BaseAddress;
           *UnsafeNumberOfBytesToProtect = NumberOfBytesToProtect;
         }
       _SEH_HANDLE
         {
           Status = _SEH_GetExceptionCode();
         }
       _SEH_END;
     }
   else
     {
       *UnsafeOldAccessProtection = OldAccessProtection;
       *UnsafeBaseAddress = BaseAddress;
       *UnsafeNumberOfBytesToProtect = NumberOfBytesToProtect;
     }

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
   PMDL Mdl;
   PVOID SystemAddress;
   KPROCESSOR_MODE PreviousMode;
   PEPROCESS Process, CurrentProcess;
   NTSTATUS Status = STATUS_SUCCESS;

   PAGED_CODE();

   DPRINT("NtReadVirtualMemory(ProcessHandle %x, BaseAddress %x, "
          "Buffer %x, NumberOfBytesToRead %d)\n",ProcessHandle,BaseAddress,
          Buffer,NumberOfBytesToRead);

   if ((ULONG_PTR)BaseAddress + NumberOfBytesToRead - 1 < (ULONG_PTR)BaseAddress ||
       (ULONG_PTR)BaseAddress + NumberOfBytesToRead - 1 >= MmUserProbeAddress)
     {
       /* Don't allow to read from kernel space */
       return STATUS_ACCESS_VIOLATION;
     }

   PreviousMode = ExGetPreviousMode();

   if (PreviousMode != KernelMode)
     {
       if ((ULONG_PTR)Buffer + NumberOfBytesToRead - 1 < (ULONG_PTR)Buffer ||
           (ULONG_PTR)Buffer + NumberOfBytesToRead - 1 >= MmUserProbeAddress)
         {
           /* Don't allow to write into kernel space */
           return STATUS_ACCESS_VIOLATION;
         }
     }

   Status = ObReferenceObjectByHandle(ProcessHandle,
                                      PROCESS_VM_READ,
                                      NULL,
                                      PreviousMode,
                                      (PVOID*)(&Process),
                                      NULL);
   if (!NT_SUCCESS(Status))
   {
      return(Status);
   }

   CurrentProcess = PsGetCurrentProcess();

   if(PreviousMode != KernelMode)
   {
     _SEH_TRY
     {
       if(NumberOfBytesRead != NULL)
       {
         ProbeForWriteUlong(NumberOfBytesRead);
       }
     }
     _SEH_HANDLE
     {
       Status = _SEH_GetExceptionCode();
     }
     _SEH_END;

     if(!NT_SUCCESS(Status))
     {
       return Status;
     }
   }


   if (Process == CurrentProcess)
   {
      _SEH_TRY
      {
        RtlCopyMemory(Buffer, BaseAddress, NumberOfBytesToRead);
      }
      _SEH_HANDLE
      {
        Status = _SEH_GetExceptionCode();
      }
      _SEH_END;
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
      _SEH_TRY
      {
        MmProbeAndLockPages(Mdl,
                            PreviousMode,
                            IoWriteAccess);
      }
      _SEH_HANDLE
      {
        Status = _SEH_GetExceptionCode();
      }
      _SEH_END;

      if(NT_SUCCESS(Status))
      {
        KeAttachProcess(&Process->Pcb);

        SystemAddress = MmGetSystemAddressForMdl(Mdl);

          Status = STATUS_SUCCESS;
          _SEH_TRY {
              Status = STATUS_PARTIAL_COPY;
              RtlCopyMemory(SystemAddress, BaseAddress, NumberOfBytesToRead);
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
      }
      ExFreePool(Mdl);
   }

   ObDereferenceObject(Process);

   if((NT_SUCCESS(Status) || Status == STATUS_PARTIAL_COPY) &&
      NumberOfBytesRead != NULL)
   {
     _SEH_TRY
     {
       *NumberOfBytesRead = NumberOfBytesToRead;
     }
     _SEH_HANDLE
     {
       Status = _SEH_GetExceptionCode();
     }
     _SEH_END;
   }

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
   PMDL Mdl;
   PVOID SystemAddress;
   PEPROCESS Process;
   KPROCESSOR_MODE PreviousMode;
   NTSTATUS CopyStatus, Status = STATUS_SUCCESS;

   DPRINT("NtWriteVirtualMemory(ProcessHandle %x, BaseAddress %x, "
          "Buffer %x, NumberOfBytesToWrite %d)\n",ProcessHandle,BaseAddress,
          Buffer,NumberOfBytesToWrite);

   if ((ULONG_PTR)BaseAddress + NumberOfBytesToWrite - 1 < (ULONG_PTR)BaseAddress ||
       (ULONG_PTR)BaseAddress + NumberOfBytesToWrite - 1 >= MmUserProbeAddress)
     {
       /* Don't allow to write into kernel space */
       return STATUS_ACCESS_VIOLATION;
     }

   PreviousMode = ExGetPreviousMode();
   
   if (PreviousMode != KernelMode)
     {
       if ((ULONG_PTR)Buffer + NumberOfBytesToWrite - 1 < (ULONG_PTR)Buffer ||
           (ULONG_PTR)Buffer + NumberOfBytesToWrite - 1 >= MmUserProbeAddress)
         {
           /* Don't allow to read from kernel space */
           return STATUS_ACCESS_VIOLATION;
         }
       if (NumberOfBytesWritten != NULL)
         {
           _SEH_TRY
             {
               ProbeForWriteUlong(NumberOfBytesWritten);
             }
           _SEH_HANDLE
             {
               Status = _SEH_GetExceptionCode();
             }
           _SEH_END;
       
           if (!NT_SUCCESS(Status))
             {
               return Status;
             }
          }
     }

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

   CopyStatus = STATUS_SUCCESS;

   /* Write memory */
   if (Process == PsGetCurrentProcess())
   {
      if (PreviousMode != KernelMode)
        {
	  _SEH_TRY
            {
              memcpy(BaseAddress, Buffer, NumberOfBytesToWrite);
            }
          _SEH_HANDLE
            {
              CopyStatus = _SEH_GetExceptionCode();
            }
          _SEH_END;
        }
      else
        {
          memcpy(BaseAddress, Buffer, NumberOfBytesToWrite);
        }
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
      _SEH_TRY
        {
          /* Map the MDL. */
          MmProbeAndLockPages(Mdl,
                              UserMode,
                              IoReadAccess);
	}
      _SEH_HANDLE
        {
	  CopyStatus = _SEH_GetExceptionCode();
        }
      _SEH_END;

      if (NT_SUCCESS(CopyStatus))
        {
          /* Copy memory from the mapped MDL into the target buffer. */
          KeAttachProcess(&Process->Pcb);

          SystemAddress = MmGetSystemAddressForMdl(Mdl);
          if (PreviousMode != KernelMode)
            {
              _SEH_TRY
                {
                  memcpy(BaseAddress, SystemAddress, NumberOfBytesToWrite);
                }
              _SEH_HANDLE
                {
                  CopyStatus = _SEH_GetExceptionCode();
                }
              _SEH_END;
            }
          else
            {
              memcpy(BaseAddress, SystemAddress, NumberOfBytesToWrite);
            }

          KeDetachProcess();
	}

      /* Free the MDL. */
      if (Mdl->MappedSystemVa != NULL)
      {
         MmUnmapLockedPages(Mdl->MappedSystemVa, Mdl);
      }
      MmUnlockPages(Mdl);
      ExFreePool(Mdl);
   }
   ObDereferenceObject(Process);

   if (NT_SUCCESS(CopyStatus) && NumberOfBytesWritten != NULL)
     {
       if (PreviousMode != KernelMode)
         {
           _SEH_TRY
             {
               *NumberOfBytesWritten = NumberOfBytesToWrite;
             }
           _SEH_HANDLE
             {
               Status = _SEH_GetExceptionCode();
             }
           _SEH_END;
         }
       else
         {
           *NumberOfBytesWritten = NumberOfBytesToWrite;
         }
     }

   return(NT_SUCCESS(CopyStatus) ? Status : CopyStatus);
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
    if (Length != 0)
    {
        ASSERT(Alignment == 1 || Alignment == 2 || Alignment == 4 || Alignment == 8);

        if (((ULONG_PTR)Address & (Alignment - 1)) != 0)
        {
            ExRaiseStatus (STATUS_DATATYPE_MISALIGNMENT);
        }
        else if ((ULONG_PTR)Address + Length - 1 < (ULONG_PTR)Address ||
                 (ULONG_PTR)Address + Length - 1 >= (ULONG_PTR)MmUserProbeAddress)
        {
            ExRaiseStatus (STATUS_ACCESS_VIOLATION);
        }
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
    volatile CHAR *Current;
    PCHAR Last;

    if (Length != 0)
    {
        ASSERT(Alignment == 1 || Alignment == 2 || Alignment == 4 || Alignment == 8);

        if (((ULONG_PTR)Address & (Alignment - 1)) != 0)
        {
            ExRaiseStatus (STATUS_DATATYPE_MISALIGNMENT);
        }

        Last = (PCHAR)((ULONG_PTR)Address + Length - 1);
        if ((ULONG_PTR)Last < (ULONG_PTR)Address ||
            (ULONG_PTR)Last >= (ULONG_PTR)MmUserProbeAddress)
        {
            ExRaiseStatus (STATUS_ACCESS_VIOLATION);
        }

        /* Check for accessible pages, do *not* touch any memory outside of the
           range!*/
        Current = (volatile CHAR*)Address;
        Last = (PCHAR)(PAGE_ROUND_DOWN(Last));
        do
        {
            *Current = *Current;
            Current = (volatile CHAR*)(PAGE_ROUND_DOWN(Current) + PAGE_SIZE);
        } while (Current <= Last);
    }
}

/* EOF */
