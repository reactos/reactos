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

static
int
MiGetExceptionInfo(EXCEPTION_POINTERS *ExceptionInfo, BOOLEAN * HaveBadAddress, ULONG_PTR * BadAddress)
{
    PEXCEPTION_RECORD ExceptionRecord;
    PAGED_CODE();

    /* Assume default */
    *HaveBadAddress = FALSE;

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
            *HaveBadAddress = TRUE;
            *BadAddress = ExceptionRecord->ExceptionInformation[1];
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
               IN SIZE_T BufferSize,
               IN KPROCESSOR_MODE PreviousMode,
               OUT PSIZE_T ReturnSize)
{
    PFN_NUMBER MdlBuffer[(sizeof(MDL) / sizeof(PFN_NUMBER)) + MI_MAPPED_COPY_PAGES + 1];
    PMDL Mdl = (PMDL)MdlBuffer;
    SIZE_T TotalSize, CurrentSize, RemainingSize;
    volatile BOOLEAN FailedInProbe = FALSE, FailedInMapping = FALSE, FailedInMoving;
    volatile BOOLEAN PagesLocked;
    PVOID CurrentAddress = SourceAddress, CurrentTargetAddress = TargetAddress;
    volatile PVOID MdlAddress;
    KAPC_STATE ApcState;
    BOOLEAN HaveBadAddress;
    ULONG_PTR BadAddress;
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
        _SEH2_TRY
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
        _SEH2_EXCEPT(MiGetExceptionInfo(_SEH2_GetExceptionInformation(), &HaveBadAddress, &BadAddress))
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
                Status = _SEH2_GetExceptionCode();
                _SEH2_YIELD(return Status);
            }

            /* Otherwise, we failed  probably during the move */
            *ReturnSize = BufferSize - RemainingSize;
            if (FailedInMoving)
            {
                /* Check if we know exactly where we stopped copying */
                if (HaveBadAddress)
                {
                    /* Return the exact number of bytes copied */
                    *ReturnSize = BadAddress - (ULONG_PTR)SourceAddress;
                }
            }

            /* Return partial copy */
            Status = STATUS_PARTIAL_COPY;
        }
        _SEH2_END;

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
             IN SIZE_T BufferSize,
             IN KPROCESSOR_MODE PreviousMode,
             OUT PSIZE_T ReturnSize)
{
    UCHAR StackBuffer[MI_POOL_COPY_BYTES];
    SIZE_T TotalSize, CurrentSize, RemainingSize;
    volatile BOOLEAN FailedInProbe = FALSE, FailedInMoving, HavePoolAddress = FALSE;
    PVOID CurrentAddress = SourceAddress, CurrentTargetAddress = TargetAddress;
    PVOID PoolAddress;
    KAPC_STATE ApcState;
    BOOLEAN HaveBadAddress;
    ULONG_PTR BadAddress;
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
        _SEH2_TRY
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
        _SEH2_EXCEPT(MiGetExceptionInfo(_SEH2_GetExceptionInformation(), &HaveBadAddress, &BadAddress))
        {
            /* Detach from whoever we may be attached to */
            KeUnstackDetachProcess(&ApcState);

            /* Check if we had allocated pool */
            if (HavePoolAddress) ExFreePool(PoolAddress);

            /* Check if we failed during the probe */
            if (FailedInProbe)
            {
                /* Exit */
                Status = _SEH2_GetExceptionCode();
                _SEH2_YIELD(return Status);
            }

            /* Otherwise, we failed  probably during the move */
            *ReturnSize = BufferSize - RemainingSize;
            if (FailedInMoving)
            {
                /* Check if we know exactly where we stopped copying */
                if (HaveBadAddress)
                {
                    /* Return the exact number of bytes copied */
                    *ReturnSize = BadAddress - (ULONG_PTR)SourceAddress;
                }
            }

            /* Return partial copy */
            Status = STATUS_PARTIAL_COPY;
        }
        _SEH2_END;

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
                    IN SIZE_T BufferSize,
                    IN KPROCESSOR_MODE PreviousMode,
                    OUT PSIZE_T ReturnSize)
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
                     IN SIZE_T Length,
                     OUT PSIZE_T ResultLength)
{
    NTSTATUS Status;
    PEPROCESS Process;
    MEMORY_AREA* MemoryArea;
    PMMSUPPORT AddressSpace;

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

    AddressSpace = &Process->Vm;

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
    ObDereferenceObject(Process);

    return Status;
}

NTSTATUS NTAPI
MiProtectVirtualMemory(IN PEPROCESS Process,
                       IN OUT PVOID *BaseAddress,
                       IN OUT PSIZE_T NumberOfBytesToProtect,
                       IN ULONG NewAccessProtection,
                       OUT PULONG OldAccessProtection  OPTIONAL)
{
    PMEMORY_AREA MemoryArea;
    PMMSUPPORT AddressSpace;
    ULONG OldAccessProtection_;
    NTSTATUS Status;

    *NumberOfBytesToProtect =
    PAGE_ROUND_UP((ULONG_PTR)(*BaseAddress) + (*NumberOfBytesToProtect)) -
    PAGE_ROUND_DOWN(*BaseAddress);
    *BaseAddress = (PVOID)PAGE_ROUND_DOWN(*BaseAddress);

    AddressSpace = &Process->Vm;

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
                    IN SIZE_T NumberOfBytesToRead,
                    OUT PSIZE_T NumberOfBytesRead OPTIONAL)
{
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    PEPROCESS Process;
    NTSTATUS Status = STATUS_SUCCESS;
    SIZE_T BytesRead = 0;
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
        _SEH2_TRY
        {
            /* Probe the output value */
            if (NumberOfBytesRead) ProbeForWriteSize_t(NumberOfBytesRead);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            /* Get exception code */
            Status = _SEH2_GetExceptionCode();
        }
        _SEH2_END;

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

        /* Dereference the process */
        ObDereferenceObject(Process);
    }

    /* Check if the caller sent this parameter */
    if (NumberOfBytesRead)
    {
        /* Enter SEH to guard write */
        _SEH2_TRY
        {
            /* Return the number of bytes read */
            *NumberOfBytesRead = BytesRead;
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            /* Handle exception */
            Status = _SEH2_GetExceptionCode();
        }
        _SEH2_END;
    }

    /* Return status */
    return Status;
}

NTSTATUS
NTAPI
NtWriteVirtualMemory(IN HANDLE ProcessHandle,
                     IN PVOID BaseAddress,
                     IN PVOID Buffer,
                     IN SIZE_T NumberOfBytesToWrite,
                     OUT PSIZE_T NumberOfBytesWritten OPTIONAL)
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
        _SEH2_TRY
        {
            /* Probe the output value */
            if (NumberOfBytesWritten) ProbeForWriteSize_t(NumberOfBytesWritten);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            /* Get exception code */
            Status = _SEH2_GetExceptionCode();
        }
        _SEH2_END;

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

        /* Dereference the process */
        ObDereferenceObject(Process);
    }

    /* Check if the caller sent this parameter */
    if (NumberOfBytesWritten)
    {
        /* Enter SEH to guard write */
        _SEH2_TRY
        {
            /* Return the number of bytes read */
            *NumberOfBytesWritten = BytesWritten;
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            /* Handle exception */
            Status = _SEH2_GetExceptionCode();
        }
        _SEH2_END;
    }

    /* Return status */
    return Status;
}

NTSTATUS
NTAPI
NtProtectVirtualMemory(IN HANDLE ProcessHandle,
                       IN OUT PVOID *UnsafeBaseAddress,
                       IN OUT SIZE_T *UnsafeNumberOfBytesToProtect,
                       IN ULONG NewAccessProtection,
                       OUT PULONG UnsafeOldAccessProtection)
{
    PEPROCESS Process;
    ULONG OldAccessProtection;
    ULONG Protection;
    PVOID BaseAddress = NULL;
    SIZE_T NumberOfBytesToProtect = 0;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    NTSTATUS Status = STATUS_SUCCESS;
    PAGED_CODE();

    /* Check for valid protection flags */
    Protection = NewAccessProtection & ~(PAGE_GUARD|PAGE_NOCACHE);
    if (Protection != PAGE_NOACCESS &&
        Protection != PAGE_READONLY &&
        Protection != PAGE_READWRITE &&
        Protection != PAGE_WRITECOPY &&
        Protection != PAGE_EXECUTE &&
        Protection != PAGE_EXECUTE_READ &&
        Protection != PAGE_EXECUTE_READWRITE &&
        Protection != PAGE_EXECUTE_WRITECOPY)
    {
        return STATUS_INVALID_PAGE_PROTECTION;
    }

    /* Check if we came from user mode */
    if (PreviousMode != KernelMode)
    {
        /* Enter SEH for probing */
        _SEH2_TRY
        {
            /* Validate all outputs */
            ProbeForWritePointer(UnsafeBaseAddress);
            ProbeForWriteSize_t(UnsafeNumberOfBytesToProtect);
            ProbeForWriteUlong(UnsafeOldAccessProtection);

            /* Capture them */
            BaseAddress = *UnsafeBaseAddress;
            NumberOfBytesToProtect = *UnsafeNumberOfBytesToProtect;
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            /* Get exception code */
            Status = _SEH2_GetExceptionCode();
        }
        _SEH2_END;

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
    _SEH2_TRY
    {
        /* Return data to user */
        *UnsafeOldAccessProtection = OldAccessProtection;
        *UnsafeBaseAddress = BaseAddress;
        *UnsafeNumberOfBytesToProtect = NumberOfBytesToProtect;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        /* Catch exception */
        Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END;

    /* Return status */
    return Status;
}

NTSTATUS NTAPI
NtQueryVirtualMemory(IN HANDLE ProcessHandle,
                     IN PVOID Address,
                     IN MEMORY_INFORMATION_CLASS VirtualMemoryInformationClass,
                     OUT PVOID VirtualMemoryInformation,
                     IN SIZE_T Length,
                     OUT PSIZE_T UnsafeResultLength)
{
    NTSTATUS Status = STATUS_SUCCESS;
    SIZE_T ResultLength = 0;
    KPROCESSOR_MODE PreviousMode;
    WCHAR ModuleFileNameBuffer[MAX_PATH] = {0};
    UNICODE_STRING ModuleFileName;
    PMEMORY_SECTION_NAME SectionName = NULL;
    PEPROCESS Process;
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

    if (PreviousMode != KernelMode)
    {
        _SEH2_TRY
        {
            ProbeForWrite(VirtualMemoryInformation,
                          Length,
                          sizeof(ULONG_PTR));

            if (UnsafeResultLength) ProbeForWriteSize_t(UnsafeResultLength);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            Status = _SEH2_GetExceptionCode();
        }
        _SEH2_END;

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

    /* FIXME: Move this inside MiQueryVirtualMemory */
    if (VirtualMemoryInformationClass == MemorySectionName)
    {
        Status = ObReferenceObjectByHandle(ProcessHandle,
                                           PROCESS_QUERY_INFORMATION,
                                           NULL,
                                           PreviousMode,
                                           (PVOID*)(&Process),
                                           NULL);

        if (!NT_SUCCESS(Status))
        {
            DPRINT("NtQueryVirtualMemory() = %x\n",Status);
            return(Status);
        }

        RtlInitEmptyUnicodeString(&ModuleFileName, ModuleFileNameBuffer, sizeof(ModuleFileNameBuffer));
        Status = MmGetFileNameForAddress(Address, &ModuleFileName);

        if (NT_SUCCESS(Status))
        {
            SectionName = VirtualMemoryInformation;
            if (PreviousMode != KernelMode)
            {
                _SEH2_TRY
                {
                    RtlInitUnicodeString(&SectionName->SectionFileName, SectionName->NameBuffer);
                    SectionName->SectionFileName.MaximumLength = Length;
                    RtlCopyUnicodeString(&SectionName->SectionFileName, &ModuleFileName);

                    if (UnsafeResultLength != NULL)
                    {
                        *UnsafeResultLength = ModuleFileName.Length;
                    }
                }
                _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
                {
                    Status = _SEH2_GetExceptionCode();
                }
                _SEH2_END;
            }
            else
            {
                RtlInitUnicodeString(&SectionName->SectionFileName, SectionName->NameBuffer);
                SectionName->SectionFileName.MaximumLength = Length;
                RtlCopyUnicodeString(&SectionName->SectionFileName, &ModuleFileName);

                if (UnsafeResultLength != NULL)
                {
                    *UnsafeResultLength = ModuleFileName.Length;
                }
            }
        }
        ObDereferenceObject(Process);
        return Status;
    }
    else
    {
        Status = MiQueryVirtualMemory(ProcessHandle,
                                      Address,
                                      VirtualMemoryInformationClass,
                                      &VirtualMemoryInfo,
                                      Length,
                                      &ResultLength);
    }

    if (NT_SUCCESS(Status))
    {
        if (PreviousMode != KernelMode)
        {
            _SEH2_TRY
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
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                Status = _SEH2_GetExceptionCode();
            }
            _SEH2_END;
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
                    IN SIZE_T NumberOfBytesToLock,
                    OUT PSIZE_T NumberOfBytesLocked OPTIONAL)
{
    UNIMPLEMENTED;
    if (NumberOfBytesLocked) *NumberOfBytesLocked = 0;
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
NtUnlockVirtualMemory(IN HANDLE ProcessHandle,
                      IN PVOID BaseAddress,
                      IN SIZE_T NumberOfBytesToUnlock,
                      OUT PSIZE_T NumberOfBytesUnlocked OPTIONAL)
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
