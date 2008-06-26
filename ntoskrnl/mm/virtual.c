/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/mm/virtual.c
 * PURPOSE:         Implementing operations on virtual memory.
 *
 * PROGRAMMERS:     David Welch
 */

/* INCLUDE ********************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

#define MI_MAPPED_COPY_PAGES  16
#define MI_POOL_COPY_BYTES    512
#define MI_MAX_TRANSFER_SIZE  64 * 1024
#define TAG_VM TAG('V', 'm', 'R', 'w')

/* PRIVATE FUNCTIONS **********************************************************/

_SEH_DEFINE_LOCALS(MiGetExceptionInfo)
{
    volatile BOOLEAN HaveBadAddress;
    volatile ULONG_PTR BadAddress;
};

_SEH_FILTER(MiGetExceptionInfo)
{
    _SEH_ACCESS_LOCALS(MiGetExceptionInfo);
    EXCEPTION_POINTERS *ExceptionInfo = _SEH_GetExceptionPointers();
    PEXCEPTION_RECORD ExceptionRecord;
    PAGED_CODE();
    
    /* Assume default */
    _SEH_VAR(HaveBadAddress) = FALSE;
    
    /* Get the exception record */
    ExceptionRecord = ExceptionInfo->ExceptionRecord;
    
    /* Look at the exception code */
    if ((ExceptionRecord->ExceptionCode == STATUS_ACCESS_VIOLATION) ||
        (ExceptionRecord->ExceptionCode == STATUS_GUARD_PAGE_VIOLATION) ||
        (ExceptionRecord->ExceptionCode == STATUS_IN_PAGE_ERROR))
    {
        /* We can tell the address if we have more than one parameter */
        if (ExceptionRecord->NumberParameters > 1)
        {
            /* Return the address */
            _SEH_VAR(HaveBadAddress) = TRUE;
            _SEH_VAR(BadAddress) = ExceptionRecord->ExceptionInformation[1];   
        }
    }
    
    /* Continue executing the next handler */
    return EXCEPTION_EXECUTE_HANDLER;
}

NTSTATUS
NTAPI
MiDoMappedCopy(IN PEPROCESS SourceProcess,
               IN PVOID SourceAddress,
               IN PEPROCESS TargetProcess,
               OUT PVOID TargetAddress,
               IN ULONG BufferSize,
               IN KPROCESSOR_MODE PreviousMode,
               OUT PULONG ReturnSize)
{
    PFN_NUMBER MdlBuffer[(sizeof(MDL) / sizeof(PFN_NUMBER)) + MI_MAPPED_COPY_PAGES + 1];
    PMDL Mdl = (PMDL)MdlBuffer;
    ULONG TotalSize, CurrentSize, RemainingSize;
    BOOLEAN FailedInProbe = FALSE, FailedInMapping = FALSE, FailedInMoving;
    BOOLEAN PagesLocked;
    PVOID CurrentAddress = SourceAddress, CurrentTargetAddress = TargetAddress;
    PVOID MdlAddress;
    KAPC_STATE ApcState;
    _SEH_DECLARE_LOCALS(MiGetExceptionInfo);
    NTSTATUS Status = STATUS_SUCCESS;
    PAGED_CODE();

    /* Calculate the maximum amount of data to move */
    TotalSize = (MI_MAPPED_COPY_PAGES - 2) * PAGE_SIZE;
    if (BufferSize <= TotalSize) TotalSize = BufferSize;
    CurrentSize = TotalSize;
    RemainingSize = BufferSize;

    /* Loop as long as there is still data */
    while (RemainingSize > 0)
    {
        /* Check if this transfer will finish everything off */
        if (RemainingSize < CurrentSize) CurrentSize = RemainingSize;

        /* Attach to the source address space */
        KeStackAttachProcess(&SourceProcess->Pcb, &ApcState);

        /* Reset state for this pass */
        MdlAddress = NULL;
        PagesLocked = FALSE;
        FailedInMoving = FALSE;
        ASSERT(FailedInProbe == FALSE);

        /* Protect user-mode copy */
        _SEH_TRY
        {
            /* If this is our first time, probe the buffer */
            if ((CurrentAddress == SourceAddress) && (PreviousMode != KernelMode))
            {
                /* Catch a failure here */
                FailedInProbe = TRUE;

                /* Do the probe */
                ProbeForRead(SourceAddress, BufferSize, sizeof(CHAR));

                /* Passed */
                FailedInProbe = FALSE;
            }

            /* Initialize and probe and lock the MDL */
            MmInitializeMdl (Mdl, CurrentAddress, CurrentSize);
            MmProbeAndLockPages (Mdl, PreviousMode, IoReadAccess);
            PagesLocked = TRUE;

            /* Now map the pages */
            MdlAddress = MmMapLockedPagesSpecifyCache(Mdl,
                                                      KernelMode,
                                                      MmCached,
                                                      NULL,
                                                      FALSE,
                                                      HighPagePriority);
            if (!MdlAddress)
            {
                /* Use our SEH handler to pick this up */
                FailedInMapping = TRUE;
                ExRaiseStatus(STATUS_INSUFFICIENT_RESOURCES);
            }

            /* Now let go of the source and grab to the target process */
            KeUnstackDetachProcess(&ApcState);
            KeStackAttachProcess(&TargetProcess->Pcb, &ApcState);

            /* Check if this is our first time through */
            if ((CurrentAddress == SourceAddress) && (PreviousMode != KernelMode))
            {
                /* Catch a failure here */
                FailedInProbe = TRUE;

                /* Do the probe */
                ProbeForWrite(TargetAddress, BufferSize, sizeof(CHAR));

                /* Passed */
                FailedInProbe = FALSE;
            }

            /* Now do the actual move */
            FailedInMoving = TRUE;
            RtlCopyMemory(CurrentTargetAddress, MdlAddress, CurrentSize);
        }
        _SEH_EXCEPT(MiGetExceptionInfo)
        {
            /* Detach from whoever we may be attached to */
            KeUnstackDetachProcess(&ApcState);

            /* Check if we had mapped the pages */
            if (MdlAddress) MmUnmapLockedPages(MdlAddress, Mdl);

            /* Check if we had locked the pages */
            if (PagesLocked) MmUnlockPages(Mdl);

            /* Check if we failed during the probe or mapping */
            if ((FailedInProbe) || (FailedInMapping))
            {
                /* Exit */
                Status = _SEH_GetExceptionCode();
                _SEH_YIELD();
            }

            /* Otherwise, we failed  probably during the move */
            *ReturnSize = BufferSize - RemainingSize;
            if (FailedInMoving)
            {
                /* Check if we know exactly where we stopped copying */
                if (_SEH_VAR(HaveBadAddress))
                {
                    /* Return the exact number of bytes copied */
                    *ReturnSize = _SEH_VAR(BadAddress) - (ULONG_PTR)SourceAddress;
                }
            }

            /* Return partial copy */
            Status = STATUS_PARTIAL_COPY;
        }
        _SEH_END;

        /* Check for SEH status */
        if (Status != STATUS_SUCCESS) return Status;

        /* Detach from target */
        KeUnstackDetachProcess(&ApcState);

        /* Unmap and unlock */
        MmUnmapLockedPages(MdlAddress, Mdl);
        MmUnlockPages(Mdl);

        /* Update location and size */
        RemainingSize -= CurrentSize;
        CurrentAddress = (PVOID)((ULONG_PTR)CurrentAddress + CurrentSize);
        CurrentTargetAddress = (PVOID)((ULONG_PTR)CurrentTargetAddress + CurrentSize);
    }

    /* All bytes read */
    *ReturnSize = BufferSize;
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
MiDoPoolCopy(IN PEPROCESS SourceProcess,
             IN PVOID SourceAddress,
             IN PEPROCESS TargetProcess,
             OUT PVOID TargetAddress,
             IN ULONG BufferSize,
             IN KPROCESSOR_MODE PreviousMode,
             OUT PULONG ReturnSize)
{
    UCHAR StackBuffer[MI_POOL_COPY_BYTES];
    ULONG TotalSize, CurrentSize, RemainingSize;
    BOOLEAN FailedInProbe = FALSE, FailedInMoving, HavePoolAddress = FALSE;
    PVOID CurrentAddress = SourceAddress, CurrentTargetAddress = TargetAddress;
    PVOID PoolAddress;
    KAPC_STATE ApcState;
    _SEH_DECLARE_LOCALS(MiGetExceptionInfo);
    NTSTATUS Status = STATUS_SUCCESS;
    PAGED_CODE();

    /* Calculate the maximum amount of data to move */
    TotalSize = MI_MAX_TRANSFER_SIZE;
    if (BufferSize <= MI_MAX_TRANSFER_SIZE) TotalSize = BufferSize;
    CurrentSize = TotalSize;
    RemainingSize = BufferSize;

    /* Check if we can use the stack */
    if (BufferSize <= MI_POOL_COPY_BYTES)
    {
        /* Use it */
        PoolAddress = (PVOID)StackBuffer;
    }
    else
    {
        /* Allocate pool */
        PoolAddress = ExAllocatePoolWithTag(NonPagedPool, TotalSize, TAG_VM);
        if (!PoolAddress) ASSERT(FALSE);
        HavePoolAddress = TRUE;
    }

    /* Loop as long as there is still data */
    while (RemainingSize > 0)
    {
        /* Check if this transfer will finish everything off */
        if (RemainingSize < CurrentSize) CurrentSize = RemainingSize;

        /* Attach to the source address space */
        KeStackAttachProcess(&SourceProcess->Pcb, &ApcState);

        /* Reset state for this pass */
        FailedInMoving = FALSE;
        ASSERT(FailedInProbe == FALSE);

        /* Protect user-mode copy */
        _SEH_TRY
        {
            /* If this is our first time, probe the buffer */
            if ((CurrentAddress == SourceAddress) && (PreviousMode != KernelMode))
            {
                /* Catch a failure here */
                FailedInProbe = TRUE;

                /* Do the probe */
                ProbeForRead(SourceAddress, BufferSize, sizeof(CHAR));

                /* Passed */
                FailedInProbe = FALSE;
            }

            /* Do the copy */
            RtlCopyMemory(PoolAddress, CurrentAddress, CurrentSize);

            /* Now let go of the source and grab to the target process */
            KeUnstackDetachProcess(&ApcState);
            KeStackAttachProcess(&TargetProcess->Pcb, &ApcState);

            /* Check if this is our first time through */
            if ((CurrentAddress == SourceAddress) && (PreviousMode != KernelMode))
            {
                /* Catch a failure here */
                FailedInProbe = TRUE;

                /* Do the probe */
                ProbeForWrite(TargetAddress, BufferSize, sizeof(CHAR));

                /* Passed */
                FailedInProbe = FALSE;
            }

            /* Now do the actual move */
            FailedInMoving = TRUE;
            RtlCopyMemory(CurrentTargetAddress, PoolAddress, CurrentSize);
        }
        _SEH_EXCEPT(MiGetExceptionInfo)
        {
            /* Detach from whoever we may be attached to */
            KeUnstackDetachProcess(&ApcState);

            /* Check if we had allocated pool */
            if (HavePoolAddress) ExFreePool(PoolAddress);

            /* Check if we failed during the probe */
            if (FailedInProbe)
            {
                /* Exit */
                Status = _SEH_GetExceptionCode();
                _SEH_YIELD();
            }

            /* Otherwise, we failed  probably during the move */
            *ReturnSize = BufferSize - RemainingSize;
            if (FailedInMoving)
            {
                /* Check if we know exactly where we stopped copying */
                if (_SEH_VAR(HaveBadAddress))
                {
                    /* Return the exact number of bytes copied */
                    *ReturnSize = _SEH_VAR(BadAddress) - (ULONG_PTR)SourceAddress;
                }
            }

            /* Return partial copy */
            Status = STATUS_PARTIAL_COPY;
        }
        _SEH_END;

        /* Check for SEH status */
        if (Status != STATUS_SUCCESS) return Status;

        /* Detach from target */
        KeUnstackDetachProcess(&ApcState);

        /* Update location and size */
        RemainingSize -= CurrentSize;
        CurrentAddress = (PVOID)((ULONG_PTR)CurrentAddress + CurrentSize);
        CurrentTargetAddress = (PVOID)((ULONG_PTR)CurrentTargetAddress + CurrentSize);
    }

    /* Check if we had allocated pool */
    if (HavePoolAddress) ExFreePool(PoolAddress);

    /* All bytes read */
    *ReturnSize = BufferSize;
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
MmCopyVirtualMemory(IN PEPROCESS SourceProcess,
                    IN PVOID SourceAddress,
                    IN PEPROCESS TargetProcess,
                    OUT PVOID TargetAddress,
                    IN ULONG BufferSize,
                    IN KPROCESSOR_MODE PreviousMode,
                    OUT PULONG ReturnSize)
{
    NTSTATUS Status;
    PEPROCESS Process = SourceProcess;

    /* Don't accept zero-sized buffers */
    if (!BufferSize) return STATUS_SUCCESS;
    
    /* If we are copying from ourselves, lock the target instead */
    if (SourceProcess == PsGetCurrentProcess()) Process = TargetProcess;
    
    /* Acquire rundown protection */
    if (!ExAcquireRundownProtection(&Process->RundownProtect))
    {
        /* Fail */
        return STATUS_PROCESS_IS_TERMINATING;
    }
    
    /* See if we should use the pool copy */
    if (BufferSize > MI_POOL_COPY_BYTES)
    {
        /* Use MDL-copy */
        Status = MiDoMappedCopy(SourceProcess,
                                SourceAddress,
                                TargetProcess,
                                TargetAddress,
                                BufferSize,
                                PreviousMode,
                                ReturnSize);
    }
    else
    {
        /* Do pool copy */
        Status = MiDoPoolCopy(SourceProcess,
                              SourceAddress,
                              TargetProcess,
                              TargetAddress,
                              BufferSize,
                              PreviousMode,
                              ReturnSize);
    }
    
    /* Release the lock */
    ExReleaseRundownProtection(&Process->RundownProtect);
    return Status;
}

NTSTATUS FASTCALL
MiQueryVirtualMemory(IN HANDLE ProcessHandle,
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
    PAGE_ROUND_UP((ULONG_PTR)(*BaseAddress) + (*NumberOfBytesToProtect)) -
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

/* PUBLIC FUNCTIONS ***********************************************************/

/*
 * @unimplemented
 */
PVOID
NTAPI
MmGetVirtualForPhysical(IN PHYSICAL_ADDRESS PhysicalAddress)
{
    UNIMPLEMENTED;
    return 0;
}

/*
 * @unimplemented
 */
PVOID
NTAPI
MmSecureVirtualMemory(IN PVOID Address,
                      IN SIZE_T Length,
                      IN ULONG Mode)
{
    UNIMPLEMENTED;
    return NULL;
}

/*
 * @unimplemented
 */
VOID
NTAPI
MmUnsecureVirtualMemory(IN PVOID SecureMem)
{
    UNIMPLEMENTED;
}

/* SYSTEM CALLS ***************************************************************/

NTSTATUS
NTAPI
NtReadVirtualMemory(IN HANDLE ProcessHandle,
                    IN PVOID BaseAddress,
                    OUT PVOID Buffer,
                    IN ULONG NumberOfBytesToRead,
                    OUT PULONG NumberOfBytesRead OPTIONAL)
{
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    PEPROCESS Process;
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG BytesRead = 0;
    PAGED_CODE();    

    /* Check if we came from user mode */
    if (PreviousMode != KernelMode)
    {
        /* Validate the read addresses */
        if ((((ULONG_PTR)BaseAddress + NumberOfBytesToRead) < (ULONG_PTR)BaseAddress) ||
            (((ULONG_PTR)Buffer + NumberOfBytesToRead) < (ULONG_PTR)Buffer) ||        
            (((ULONG_PTR)BaseAddress + NumberOfBytesToRead) > MmUserProbeAddress) ||
            (((ULONG_PTR)Buffer + NumberOfBytesToRead) > MmUserProbeAddress))
        {
            /* Don't allow to write into kernel space */
            return STATUS_ACCESS_VIOLATION;
        }
        
        /* Enter SEH for probe */
        _SEH_TRY
        {
            /* Probe the output value */
            if (NumberOfBytesRead) ProbeForWriteUlong(NumberOfBytesRead);
        }
        _SEH_HANDLE
        {
            /* Get exception code */
            Status = _SEH_GetExceptionCode();
        }
        _SEH_END;
        
        /* Return if we failed */
        if (!NT_SUCCESS(Status)) return Status;
    }
    
    /* Reference the process */
    Status = ObReferenceObjectByHandle(ProcessHandle,
                                       PROCESS_VM_READ,
                                       PsProcessType,
                                       PreviousMode,
                                       (PVOID*)(&Process),
                                       NULL);
    if (NT_SUCCESS(Status))
    {
        /* Do the copy */
        Status = MmCopyVirtualMemory(Process,
                                     BaseAddress,
                                     PsGetCurrentProcess(),
                                     Buffer,
                                     NumberOfBytesToRead,
                                     PreviousMode,
                                     &BytesRead);
        
        /* Derefernece the process */
        ObDereferenceObject(Process);
    }
    
    /* Check if the caller sent this parameter */
    if (NumberOfBytesRead)
    {
        /* Enter SEH to guard write */
        _SEH_TRY
        {
            /* Return the number of bytes read */
            *NumberOfBytesRead = BytesRead;
        }
        _SEH_HANDLE
        {
            /* Handle exception */
            Status = _SEH_GetExceptionCode();
        }
        _SEH_END;
    }

    /* Return status */
    return Status;
}

NTSTATUS
NTAPI
NtWriteVirtualMemory(IN HANDLE ProcessHandle,
                     IN PVOID BaseAddress,
                     IN PVOID Buffer,
                     IN ULONG NumberOfBytesToWrite,
                     OUT PULONG NumberOfBytesWritten OPTIONAL)
{
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    PEPROCESS Process;
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG BytesWritten = 0;
    PAGED_CODE();    

    /* Check if we came from user mode */
    if (PreviousMode != KernelMode)
    {
        /* Validate the read addresses */
        if ((((ULONG_PTR)BaseAddress + NumberOfBytesToWrite) < (ULONG_PTR)BaseAddress) ||
            (((ULONG_PTR)Buffer + NumberOfBytesToWrite) < (ULONG_PTR)Buffer) ||        
            (((ULONG_PTR)BaseAddress + NumberOfBytesToWrite) > MmUserProbeAddress) ||
            (((ULONG_PTR)Buffer + NumberOfBytesToWrite) > MmUserProbeAddress))
        {
            /* Don't allow to write into kernel space */
            return STATUS_ACCESS_VIOLATION;
        }
        
        /* Enter SEH for probe */
        _SEH_TRY
        {
            /* Probe the output value */
            if (NumberOfBytesWritten) ProbeForWriteUlong(NumberOfBytesWritten);
        }
        _SEH_HANDLE
        {
            /* Get exception code */
            Status = _SEH_GetExceptionCode();
        }
        _SEH_END;
        
        /* Return if we failed */
        if (!NT_SUCCESS(Status)) return Status;
    }
    
    /* Reference the process */
    Status = ObReferenceObjectByHandle(ProcessHandle,
                                       PROCESS_VM_WRITE,
                                       PsProcessType,
                                       PreviousMode,
                                       (PVOID*)&Process,
                                       NULL);
    if (NT_SUCCESS(Status))
    {
        /* Do the copy */
        Status = MmCopyVirtualMemory(PsGetCurrentProcess(),
                                     Buffer,
                                     Process,
                                     BaseAddress,
                                     NumberOfBytesToWrite,
                                     PreviousMode,
                                     &BytesWritten);
        
        /* Derefernece the process */
        ObDereferenceObject(Process);
    }
    
    /* Check if the caller sent this parameter */
    if (NumberOfBytesWritten)
    {
        /* Enter SEH to guard write */
        _SEH_TRY
        {
            /* Return the number of bytes read */
            *NumberOfBytesWritten = BytesWritten;
        }
        _SEH_HANDLE
        {
            /* Handle exception */
            Status = _SEH_GetExceptionCode();
        }
        _SEH_END;
    }
    
    /* Return status */
    return Status;
}

NTSTATUS
NTAPI
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
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    NTSTATUS Status = STATUS_SUCCESS;

    /* Check if we came from user mode */
    if (PreviousMode != KernelMode)
    {
        /* Enter SEH for probing */
        _SEH_TRY
        {
            /* Validate all outputs */
            ProbeForWritePointer(UnsafeBaseAddress);
            ProbeForWriteUlong(UnsafeNumberOfBytesToProtect);
            ProbeForWriteUlong(UnsafeOldAccessProtection);
            
            /* Capture them */
            BaseAddress = *UnsafeBaseAddress;
            NumberOfBytesToProtect = *UnsafeNumberOfBytesToProtect;
        }
        _SEH_HANDLE
        {
            /* Get exception code */
            Status = _SEH_GetExceptionCode();
        }
        _SEH_END;
        
        /* Return on exception */
        if (!NT_SUCCESS(Status)) return Status;
    }
    else
    {
        /* Capture directly */
        BaseAddress = *UnsafeBaseAddress;
        NumberOfBytesToProtect = *UnsafeNumberOfBytesToProtect;
    }
    
    /* Catch illegal base address */
    if (BaseAddress > (PVOID)MmUserProbeAddress) return STATUS_INVALID_PARAMETER_2;
    
    /* Catch illegal region size  */
    if ((MmUserProbeAddress - (ULONG_PTR)BaseAddress) < NumberOfBytesToProtect)
    {
        /* Fail */
        return STATUS_INVALID_PARAMETER_3;
    }

    /* 0 is also illegal */
    if (!NumberOfBytesToProtect) return STATUS_INVALID_PARAMETER_3;

    /* Get a reference to the process */
    Status = ObReferenceObjectByHandle(ProcessHandle,
                                       PROCESS_VM_OPERATION,
                                       PsProcessType,
                                       PreviousMode,
                                       (PVOID*)(&Process),
                                       NULL);
    if (!NT_SUCCESS(Status)) return Status;

    /* Do the actual work */
    Status = MiProtectVirtualMemory(Process,
                                    &BaseAddress,
                                    &NumberOfBytesToProtect,
                                    NewAccessProtection,
                                    &OldAccessProtection);

    /* Release reference */
    ObDereferenceObject(Process);
    
    /* Enter SEH to return data */
    _SEH_TRY
    {
        /* Return data to user */
        *UnsafeOldAccessProtection = OldAccessProtection;
        *UnsafeBaseAddress = BaseAddress;
        *UnsafeNumberOfBytesToProtect = NumberOfBytesToProtect;
    }
    _SEH_HANDLE
    {
        /* Catch exception */
        Status = _SEH_GetExceptionCode();
    }
    _SEH_END;

    /* Return status */
    return Status;
}

NTSTATUS STDCALL
NtQueryVirtualMemory(IN HANDLE ProcessHandle,
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
    
    Status = MiQueryVirtualMemory(ProcessHandle,
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

NTSTATUS
NTAPI
NtLockVirtualMemory(IN HANDLE ProcessHandle,
                    IN PVOID BaseAddress,
                    IN ULONG NumberOfBytesToLock,
                    OUT PULONG NumberOfBytesLocked OPTIONAL)
{
    UNIMPLEMENTED;
    if (NumberOfBytesLocked) *NumberOfBytesLocked = 0;
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
NtUnlockVirtualMemory(IN HANDLE ProcessHandle,
                      IN PVOID BaseAddress,
                      IN ULONG NumberOfBytesToUnlock,
                      OUT PULONG NumberOfBytesUnlocked OPTIONAL)
{
    UNIMPLEMENTED;
    if (NumberOfBytesUnlocked) *NumberOfBytesUnlocked = 0;
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
NtFlushVirtualMemory(IN HANDLE ProcessHandle,
                     IN OUT PVOID *BaseAddress,
                     IN OUT PSIZE_T NumberOfBytesToFlush,
                     OUT PIO_STATUS_BLOCK IoStatusBlock)
{
    UNIMPLEMENTED;
    return STATUS_SUCCESS;
}

/* EOF */
