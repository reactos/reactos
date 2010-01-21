/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            ntoskrnl/mm/ARM3/virtual.c
 * PURPOSE:         ARM Memory Manager Virtual Memory Management
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

#line 15 "ARM³::VIRTUAL"
#define MODULE_INVOLVED_IN_ARM3
#include "../ARM3/miarm.h"

#define MI_MAPPED_COPY_PAGES  14
#define MI_POOL_COPY_BYTES    512
#define MI_MAX_TRANSFER_SIZE  64 * 1024

NTSTATUS NTAPI
MiProtectVirtualMemory(IN PEPROCESS Process,
                       IN OUT PVOID *BaseAddress,
                       IN OUT PSIZE_T NumberOfBytesToProtect,
                       IN ULONG NewAccessProtection,
                       OUT PULONG OldAccessProtection  OPTIONAL);

/* PRIVATE FUNCTIONS **********************************************************/

LONG
MiGetExceptionInfo(IN PEXCEPTION_POINTERS ExceptionInfo,
                   OUT PBOOLEAN HaveBadAddress, 
                   OUT PULONG_PTR BadAddress)
{
    PEXCEPTION_RECORD ExceptionRecord;
    PAGED_CODE();

    //
    // Assume default
    //
    *HaveBadAddress = FALSE;

    //
    // Get the exception record
    //
    ExceptionRecord = ExceptionInfo->ExceptionRecord;

    //
    // Look at the exception code
    //
    if ((ExceptionRecord->ExceptionCode == STATUS_ACCESS_VIOLATION) ||
        (ExceptionRecord->ExceptionCode == STATUS_GUARD_PAGE_VIOLATION) ||
        (ExceptionRecord->ExceptionCode == STATUS_IN_PAGE_ERROR))
    {
        //
        // We can tell the address if we have more than one parameter
        //
        if (ExceptionRecord->NumberParameters > 1)
        {
            //
            // Return the address
            //
            *HaveBadAddress = TRUE;
            *BadAddress = ExceptionRecord->ExceptionInformation[1];
        }
    }

    //
    // Continue executing the next handler
    //
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

    //
    // Calculate the maximum amount of data to move
    //
    TotalSize = MI_MAPPED_COPY_PAGES * PAGE_SIZE;
    if (BufferSize <= TotalSize) TotalSize = BufferSize;
    CurrentSize = TotalSize;
    RemainingSize = BufferSize;

    //
    // Loop as long as there is still data
    //
    while (RemainingSize > 0)
    {
        //
        // Check if this transfer will finish everything off
        //
        if (RemainingSize < CurrentSize) CurrentSize = RemainingSize;

        //
        // Attach to the source address space
        //
        KeStackAttachProcess(&SourceProcess->Pcb, &ApcState);

        //
        // Reset state for this pass
        //
        MdlAddress = NULL;
        PagesLocked = FALSE;
        FailedInMoving = FALSE;
        ASSERT(FailedInProbe == FALSE);

        //
        // Protect user-mode copy
        //
        _SEH2_TRY
        {
            //
            // If this is our first time, probe the buffer
            //
            if ((CurrentAddress == SourceAddress) && (PreviousMode != KernelMode))
            {
                //
                // Catch a failure here
                //
                FailedInProbe = TRUE;

                //
                // Do the probe
                //
                ProbeForRead(SourceAddress, BufferSize, sizeof(CHAR));

                //
                // Passed
                //
                FailedInProbe = FALSE;
            }

            //
            // Initialize and probe and lock the MDL
            //
            MmInitializeMdl(Mdl, CurrentAddress, CurrentSize);
            MmProbeAndLockPages(Mdl, PreviousMode, IoReadAccess);
            PagesLocked = TRUE;

            //
            // Now map the pages
            //
            MdlAddress = MmMapLockedPagesSpecifyCache(Mdl,
                                                      KernelMode,
                                                      MmCached,
                                                      NULL,
                                                      FALSE,
                                                      HighPagePriority);
            if (!MdlAddress)
            {
                //
                // Use our SEH handler to pick this up
                //
                FailedInMapping = TRUE;
                ExRaiseStatus(STATUS_INSUFFICIENT_RESOURCES);
            }

            //
            // Now let go of the source and grab to the target process
            //
            KeUnstackDetachProcess(&ApcState);
            KeStackAttachProcess(&TargetProcess->Pcb, &ApcState);

            //
            // Check if this is our first time through
            //
            if ((CurrentAddress == SourceAddress) && (PreviousMode != KernelMode))
            {
                //
                // Catch a failure here
                //
                FailedInProbe = TRUE;

                //
                // Do the probe
                //
                ProbeForWrite(TargetAddress, BufferSize, sizeof(CHAR));

                //
                // Passed
                //
                FailedInProbe = FALSE;
            }

            //
            // Now do the actual move
            //
            FailedInMoving = TRUE;
            RtlCopyMemory(CurrentTargetAddress, MdlAddress, CurrentSize);
        }
        _SEH2_EXCEPT(MiGetExceptionInfo(_SEH2_GetExceptionInformation(),
                                        &HaveBadAddress,
                                        &BadAddress))
        {
            //
            // Detach from whoever we may be attached to
            //
            KeUnstackDetachProcess(&ApcState);

            //
            // Check if we had mapped the pages
            //
            if (MdlAddress) MmUnmapLockedPages(MdlAddress, Mdl);

            //
            // Check if we had locked the pages
            //
            if (PagesLocked) MmUnlockPages(Mdl);
            
            //
            // Check if we hit working set quota
            //
            if (_SEH2_GetExceptionCode() == STATUS_WORKING_SET_QUOTA)
            {
                //
                // Return the error
                //
                return STATUS_WORKING_SET_QUOTA;
            }

            //
            // Check if we failed during the probe or mapping
            //
            if ((FailedInProbe) || (FailedInMapping))
            {
                //
                // Exit
                //
                Status = _SEH2_GetExceptionCode();
                _SEH2_YIELD(return Status);
            }

            //
            // Otherwise, we failed  probably during the move
            //
            *ReturnSize = BufferSize - RemainingSize;
            if (FailedInMoving)
            {
                //
                // Check if we know exactly where we stopped copying
                //
                if (HaveBadAddress)
                {
                    //
                    // Return the exact number of bytes copied
                    //
                    *ReturnSize = BadAddress - (ULONG_PTR)SourceAddress;
                }
            }

            //
            // Return partial copy
            //
            Status = STATUS_PARTIAL_COPY;
        }
        _SEH2_END;

        //
        // Check for SEH status
        //
        if (Status != STATUS_SUCCESS) return Status;

        //
        // Detach from target
        //
        KeUnstackDetachProcess(&ApcState);

        //
        // Unmap and unlock
        //
        MmUnmapLockedPages(MdlAddress, Mdl);
        MmUnlockPages(Mdl);

        //
        // Update location and size
        //
        RemainingSize -= CurrentSize;
        CurrentAddress = (PVOID)((ULONG_PTR)CurrentAddress + CurrentSize);
        CurrentTargetAddress = (PVOID)((ULONG_PTR)CurrentTargetAddress + CurrentSize);
    }

    //
    // All bytes read
    //
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

    //
    // Calculate the maximum amount of data to move
    //
    TotalSize = MI_MAX_TRANSFER_SIZE;
    if (BufferSize <= MI_MAX_TRANSFER_SIZE) TotalSize = BufferSize;
    CurrentSize = TotalSize;
    RemainingSize = BufferSize;

    //
    // Check if we can use the stack
    //
    if (BufferSize <= MI_POOL_COPY_BYTES)
    {
        //
        // Use it
        //
        PoolAddress = (PVOID)StackBuffer;
    }
    else
    {
        //
        // Allocate pool
        //
        PoolAddress = ExAllocatePoolWithTag(NonPagedPool, TotalSize, 'VmRw');
        if (!PoolAddress) ASSERT(FALSE);
        HavePoolAddress = TRUE;
    }

    //
    // Loop as long as there is still data
    //
    while (RemainingSize > 0)
    {
        //
        // Check if this transfer will finish everything off
        //
        if (RemainingSize < CurrentSize) CurrentSize = RemainingSize;

        //
        // Attach to the source address space
        //
        KeStackAttachProcess(&SourceProcess->Pcb, &ApcState);

        //
        // Reset state for this pass
        //
        FailedInMoving = FALSE;
        ASSERT(FailedInProbe == FALSE);

        //
        // Protect user-mode copy
        //
        _SEH2_TRY
        {
            //
            // If this is our first time, probe the buffer
            //
            if ((CurrentAddress == SourceAddress) && (PreviousMode != KernelMode))
            {
                //
                // Catch a failure here
                //
                FailedInProbe = TRUE;

                //
                // Do the probe
                //
                ProbeForRead(SourceAddress, BufferSize, sizeof(CHAR));

                //
                // Passed
                //
                FailedInProbe = FALSE;
            }

            //
            // Do the copy
            //
            RtlCopyMemory(PoolAddress, CurrentAddress, CurrentSize);

            //
            // Now let go of the source and grab to the target process
            //
            KeUnstackDetachProcess(&ApcState);
            KeStackAttachProcess(&TargetProcess->Pcb, &ApcState);

            //
            // Check if this is our first time through
            //
            if ((CurrentAddress == SourceAddress) && (PreviousMode != KernelMode))
            {
                //
                // Catch a failure here
                //
                FailedInProbe = TRUE;

                //
                // Do the probe
                //
                ProbeForWrite(TargetAddress, BufferSize, sizeof(CHAR));

                //
                // Passed
                //
                FailedInProbe = FALSE;
            }

            //
            // Now do the actual move
            //
            FailedInMoving = TRUE;
            RtlCopyMemory(CurrentTargetAddress, PoolAddress, CurrentSize);
        }
        _SEH2_EXCEPT(MiGetExceptionInfo(_SEH2_GetExceptionInformation(),
                                        &HaveBadAddress,
                                        &BadAddress))
        {
            //
            // Detach from whoever we may be attached to
            //
            KeUnstackDetachProcess(&ApcState);

            //
            // Check if we had allocated pool
            //
            if (HavePoolAddress) ExFreePool(PoolAddress);

            //
            // Check if we failed during the probe
            //
            if (FailedInProbe)
            {
                //
                // Exit
                //
                Status = _SEH2_GetExceptionCode();
                _SEH2_YIELD(return Status);
            }

            //
            // Otherwise, we failed, probably during the move
            //
            *ReturnSize = BufferSize - RemainingSize;
            if (FailedInMoving)
            {
                //
                // Check if we know exactly where we stopped copying
                //
                if (HaveBadAddress)
                {
                    //
                    // Return the exact number of bytes copied
                    //
                    *ReturnSize = BadAddress - (ULONG_PTR)SourceAddress;
                }
            }

            //
            // Return partial copy
            //
            Status = STATUS_PARTIAL_COPY;
        }
        _SEH2_END;

        //
        // Check for SEH status
        //
        if (Status != STATUS_SUCCESS) return Status;

        //
        // Detach from target
        //
        KeUnstackDetachProcess(&ApcState);

        //
        // Update location and size
        //
        RemainingSize -= CurrentSize;
        CurrentAddress = (PVOID)((ULONG_PTR)CurrentAddress + CurrentSize);
        CurrentTargetAddress = (PVOID)((ULONG_PTR)CurrentTargetAddress +
                                       CurrentSize);
    }

    //
    // Check if we had allocated pool
    //
    if (HavePoolAddress) ExFreePool(PoolAddress);

    //
    // All bytes read
    //
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

    //
    // Don't accept zero-sized buffers
    //
    if (!BufferSize) return STATUS_SUCCESS;

    //
    // If we are copying from ourselves, lock the target instead
    //
    if (SourceProcess == PsGetCurrentProcess()) Process = TargetProcess;

    //
    // Acquire rundown protection
    //
    if (!ExAcquireRundownProtection(&Process->RundownProtect))
    {
        //
        // Fail
        //
        return STATUS_PROCESS_IS_TERMINATING;
    }

    //
    // See if we should use the pool copy
    //
    if (BufferSize > MI_POOL_COPY_BYTES)
    {
        //
        // Use MDL-copy
        //
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
        //
        // Do pool copy
        //
        Status = MiDoPoolCopy(SourceProcess,
                              SourceAddress,
                              TargetProcess,
                              TargetAddress,
                              BufferSize,
                              PreviousMode,
                              ReturnSize);
    }

    //
    // Release the lock
    //
    ExReleaseRundownProtection(&Process->RundownProtect);
    return Status;
}
                                                               
NTSTATUS
NTAPI
MmFlushVirtualMemory(IN PEPROCESS Process,
                     IN OUT PVOID *BaseAddress,
                     IN OUT PSIZE_T RegionSize,
                     OUT PIO_STATUS_BLOCK IoStatusBlock)
{
    PAGED_CODE();
    UNIMPLEMENTED;
    
    //
    // Fake success
    //
    return STATUS_SUCCESS;
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
    return Address;
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

    //
    // Check if we came from user mode
    //
    if (PreviousMode != KernelMode)
    {
        //
        // Validate the read addresses
        //
        if ((((ULONG_PTR)BaseAddress + NumberOfBytesToRead) < (ULONG_PTR)BaseAddress) ||
            (((ULONG_PTR)Buffer + NumberOfBytesToRead) < (ULONG_PTR)Buffer) ||
            (((ULONG_PTR)BaseAddress + NumberOfBytesToRead) > MmUserProbeAddress) ||
            (((ULONG_PTR)Buffer + NumberOfBytesToRead) > MmUserProbeAddress))
        {
            //
            // Don't allow to write into kernel space
            //
            return STATUS_ACCESS_VIOLATION;
        }

        //
        // Enter SEH for probe
        //
        _SEH2_TRY
        {
            //
            // Probe the output value
            //
            if (NumberOfBytesRead) ProbeForWriteSize_t(NumberOfBytesRead);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            //
            // Get exception code
            //
            _SEH2_YIELD(return _SEH2_GetExceptionCode());
        }
        _SEH2_END;
    }
    
    //
    // Don't do zero-byte transfers
    //
    if (NumberOfBytesToRead)
    {
        //
        // Reference the process
        //
        Status = ObReferenceObjectByHandle(ProcessHandle,
                                           PROCESS_VM_READ,
                                           PsProcessType,
                                           PreviousMode,
                                           (PVOID*)(&Process),
                                           NULL);
        if (NT_SUCCESS(Status))
        {
            //
            // Do the copy
            //
            Status = MmCopyVirtualMemory(Process,
                                         BaseAddress,
                                         PsGetCurrentProcess(),
                                         Buffer,
                                         NumberOfBytesToRead,
                                         PreviousMode,
                                         &BytesRead);
            
            //
            // Dereference the process
            //
            ObDereferenceObject(Process);
        }
    }

    //
    // Check if the caller sent this parameter
    //
    if (NumberOfBytesRead)
    {
        //
        // Enter SEH to guard write
        //
        _SEH2_TRY
        {
            //
            // Return the number of bytes read
            //
            *NumberOfBytesRead = BytesRead;
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
        }
        _SEH2_END;
    }

    //
    // Return status
    //
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

    //
    // Check if we came from user mode
    //
    if (PreviousMode != KernelMode)
    {
        //
        // Validate the read addresses
        //
        if ((((ULONG_PTR)BaseAddress + NumberOfBytesToWrite) < (ULONG_PTR)BaseAddress) ||
            (((ULONG_PTR)Buffer + NumberOfBytesToWrite) < (ULONG_PTR)Buffer) ||
            (((ULONG_PTR)BaseAddress + NumberOfBytesToWrite) > MmUserProbeAddress) ||
            (((ULONG_PTR)Buffer + NumberOfBytesToWrite) > MmUserProbeAddress))
        {
            //
            // Don't allow to write into kernel space
            //
            return STATUS_ACCESS_VIOLATION;
        }

        //
        // Enter SEH for probe
        //
        _SEH2_TRY
        {
            //
            // Probe the output value
            //
            if (NumberOfBytesWritten) ProbeForWriteSize_t(NumberOfBytesWritten);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            //
            // Get exception code
            //
            _SEH2_YIELD(return _SEH2_GetExceptionCode());
        }
        _SEH2_END;
    }

    //
    // Don't do zero-byte transfers
    //
    if (NumberOfBytesToWrite)
    {
        //
        // Reference the process
        //
        Status = ObReferenceObjectByHandle(ProcessHandle,
                                           PROCESS_VM_WRITE,
                                           PsProcessType,
                                           PreviousMode,
                                           (PVOID*)&Process,
                                           NULL);
        if (NT_SUCCESS(Status))
        {
            //
            // Do the copy
            //
            Status = MmCopyVirtualMemory(PsGetCurrentProcess(),
                                         Buffer,
                                         Process,
                                         BaseAddress,
                                         NumberOfBytesToWrite,
                                         PreviousMode,
                                         &BytesWritten);
            
            //
            // Dereference the process
            //
            ObDereferenceObject(Process);
        }
    }

    //
    // Check if the caller sent this parameter
    //
    if (NumberOfBytesWritten)
    {
        //
        // Enter SEH to guard write
        //
        _SEH2_TRY
        {
            //
            // Return the number of bytes written
            //
            *NumberOfBytesWritten = BytesWritten;
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
        }
        _SEH2_END;
    }

    //
    // Return status
    //
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
    PEPROCESS CurrentProcess = PsGetCurrentProcess();
    PVOID BaseAddress = NULL;
    SIZE_T NumberOfBytesToProtect = 0;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    NTSTATUS Status;
    BOOLEAN Attached = FALSE;
    KAPC_STATE ApcState;
    PAGED_CODE();

    //
    // Check for valid protection flags
    //
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
        //
        // Fail
        //
        return STATUS_INVALID_PAGE_PROTECTION;
    }

    //
    // Check if we came from user mode
    //
    if (PreviousMode != KernelMode)
    {
        //
        // Enter SEH for probing
        //
        _SEH2_TRY
        {
            //
            // Validate all outputs
            //
            ProbeForWritePointer(UnsafeBaseAddress);
            ProbeForWriteSize_t(UnsafeNumberOfBytesToProtect);
            ProbeForWriteUlong(UnsafeOldAccessProtection);

            //
            // Capture them
            //
            BaseAddress = *UnsafeBaseAddress;
            NumberOfBytesToProtect = *UnsafeNumberOfBytesToProtect;
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            //
            // Get exception code
            //
            _SEH2_YIELD(return _SEH2_GetExceptionCode());
        }
        _SEH2_END;
    }
    else
    {
        //
        // Capture directly
        //
        BaseAddress = *UnsafeBaseAddress;
        NumberOfBytesToProtect = *UnsafeNumberOfBytesToProtect;
    }

    //
    // Catch illegal base address
    //
    if (BaseAddress > MM_HIGHEST_USER_ADDRESS) return STATUS_INVALID_PARAMETER_2;

    //
    // Catch illegal region size
    //
    if ((MmUserProbeAddress - (ULONG_PTR)BaseAddress) < NumberOfBytesToProtect)
    {
        //
        // Fail
        //
        return STATUS_INVALID_PARAMETER_3;
    }

    //
    // 0 is also illegal
    //
    if (!NumberOfBytesToProtect) return STATUS_INVALID_PARAMETER_3;

    //
    // Get a reference to the process
    //
    Status = ObReferenceObjectByHandle(ProcessHandle,
                                       PROCESS_VM_OPERATION,
                                       PsProcessType,
                                       PreviousMode,
                                       (PVOID*)(&Process),
                                       NULL);
    if (!NT_SUCCESS(Status)) return Status;
    
    //
    // Check if we should attach
    //
    if (CurrentProcess != Process)
    {
        //
        // Do it
        //
        KeStackAttachProcess(&Process->Pcb, &ApcState);
        Attached = TRUE;
    }

    //
    // Do the actual work
    //
    Status = MiProtectVirtualMemory(Process,
                                    &BaseAddress,
                                    &NumberOfBytesToProtect,
                                    NewAccessProtection,
                                    &OldAccessProtection);
    
    //
    // Detach if needed
    //
    if (Attached) KeUnstackDetachProcess(&ApcState);

    //
    // Release reference
    //
    ObDereferenceObject(Process);

    //
    // Enter SEH to return data
    //
    _SEH2_TRY
    {
        //
        // Return data to user
        //
        *UnsafeOldAccessProtection = OldAccessProtection;
        *UnsafeBaseAddress = BaseAddress;
        *UnsafeNumberOfBytesToProtect = NumberOfBytesToProtect;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
    }
    _SEH2_END;

    //
    // Return status
    //
    return Status;
}

NTSTATUS
NTAPI
NtLockVirtualMemory(IN HANDLE ProcessHandle,
                    IN OUT PVOID *BaseAddress,
                    IN OUT PSIZE_T NumberOfBytesToLock,
                    IN ULONG MapType)
{
    PEPROCESS Process;
    PEPROCESS CurrentProcess = PsGetCurrentProcess();
    NTSTATUS Status;
    BOOLEAN Attached = FALSE;
    KAPC_STATE ApcState;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    PVOID CapturedBaseAddress;
    SIZE_T CapturedBytesToLock;
    PAGED_CODE();
    
    //
    // Validate flags
    //
    if ((MapType & ~(MAP_PROCESS | MAP_SYSTEM)))
    {
        //
        // Invalid set of flags
        //
        return STATUS_INVALID_PARAMETER;
    }
    
    //
    // At least one flag must be specified
    //
    if (!(MapType & (MAP_PROCESS | MAP_SYSTEM)))
    {
        //
        // No flag given
        //
        return STATUS_INVALID_PARAMETER;
    }
    
    //
    // Enter SEH for probing
    //
    _SEH2_TRY
    {
        //
        // Validate output data
        //
        ProbeForWritePointer(BaseAddress);
        ProbeForWriteSize_t(NumberOfBytesToLock);
        
        //
        // Capture it
        //
        CapturedBaseAddress = *BaseAddress;
        CapturedBytesToLock = *NumberOfBytesToLock;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        //
        // Get exception code
        //
        _SEH2_YIELD(return _SEH2_GetExceptionCode());
    }
    _SEH2_END;
    
    //
    // Catch illegal base address
    //
    if (CapturedBaseAddress > MM_HIGHEST_USER_ADDRESS) return STATUS_INVALID_PARAMETER;
    
    //
    // Catch illegal region size
    //
    if ((MmUserProbeAddress - (ULONG_PTR)CapturedBaseAddress) < CapturedBytesToLock)
    {
        //
        // Fail
        //
        return STATUS_INVALID_PARAMETER;
    }
    
    //
    // 0 is also illegal
    //
    if (!CapturedBytesToLock) return STATUS_INVALID_PARAMETER;
    
    //
    // Get a reference to the process
    //
    Status = ObReferenceObjectByHandle(ProcessHandle,
                                       PROCESS_VM_OPERATION,
                                       PsProcessType,
                                       PreviousMode,
                                       (PVOID*)(&Process),
                                       NULL);
    if (!NT_SUCCESS(Status)) return Status;
    
    //
    // Check if this is is system-mapped
    //
    if (MapType & MAP_SYSTEM)
    {
        //
        // Check for required privilege
        //
        if (!SeSinglePrivilegeCheck(SeLockMemoryPrivilege, PreviousMode))
        {
            //
            // Fail: Don't have it
            //
            ObDereferenceObject(Process);
            return STATUS_PRIVILEGE_NOT_HELD;
        }
    }
    
    //
    // Check if we should attach
    //
    if (CurrentProcess != Process)
    {
        //
        // Do it
        //
        KeStackAttachProcess(&Process->Pcb, &ApcState);
        Attached = TRUE;
    }
    
    //
    // Oops :(
    //
    UNIMPLEMENTED;
    
    //
    // Detach if needed
    //
    if (Attached) KeUnstackDetachProcess(&ApcState);
    
    //
    // Release reference
    //
    ObDereferenceObject(Process);
    
    //
    // Enter SEH to return data
    //
    _SEH2_TRY
    {
        //
        // Return data to user
        //
        *BaseAddress = CapturedBaseAddress;
        *NumberOfBytesToLock = 0;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        //
        // Get exception code
        //
        _SEH2_YIELD(return _SEH2_GetExceptionCode());
    }
    _SEH2_END;
    
    //
    // Return status
    //
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
NtUnlockVirtualMemory(IN HANDLE ProcessHandle,
                      IN OUT PVOID *BaseAddress,
                      IN OUT PSIZE_T NumberOfBytesToUnlock,
                      IN ULONG MapType)
{
    PEPROCESS Process;
    PEPROCESS CurrentProcess = PsGetCurrentProcess();
    NTSTATUS Status;
    BOOLEAN Attached = FALSE;
    KAPC_STATE ApcState;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    PVOID CapturedBaseAddress;
    SIZE_T CapturedBytesToUnlock;
    PAGED_CODE();
    
    //
    // Validate flags
    //
    if ((MapType & ~(MAP_PROCESS | MAP_SYSTEM)))
    {
        //
        // Invalid set of flags
        //
        return STATUS_INVALID_PARAMETER;
    }
    
    //
    // At least one flag must be specified
    //
    if (!(MapType & (MAP_PROCESS | MAP_SYSTEM)))
    {
        //
        // No flag given
        //
        return STATUS_INVALID_PARAMETER;
    }
    
    //
    // Enter SEH for probing
    //
    _SEH2_TRY
    {
        //
        // Validate output data
        //
        ProbeForWritePointer(BaseAddress);
        ProbeForWriteSize_t(NumberOfBytesToUnlock);
        
        //
        // Capture it
        //
        CapturedBaseAddress = *BaseAddress;
        CapturedBytesToUnlock = *NumberOfBytesToUnlock;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        //
        // Get exception code
        //
        _SEH2_YIELD(return _SEH2_GetExceptionCode());
    }
    _SEH2_END;
    
    //
    // Catch illegal base address
    //
    if (CapturedBaseAddress > MM_HIGHEST_USER_ADDRESS) return STATUS_INVALID_PARAMETER;
    
    //
    // Catch illegal region size
    //
    if ((MmUserProbeAddress - (ULONG_PTR)CapturedBaseAddress) < CapturedBytesToUnlock)
    {
        //
        // Fail
        //
        return STATUS_INVALID_PARAMETER;
    }
    
    //
    // 0 is also illegal
    //
    if (!CapturedBytesToUnlock) return STATUS_INVALID_PARAMETER;
    
    //
    // Get a reference to the process
    //
    Status = ObReferenceObjectByHandle(ProcessHandle,
                                       PROCESS_VM_OPERATION,
                                       PsProcessType,
                                       PreviousMode,
                                       (PVOID*)(&Process),
                                       NULL);
    if (!NT_SUCCESS(Status)) return Status;
    
    //
    // Check if this is is system-mapped
    //
    if (MapType & MAP_SYSTEM)
    {
        //
        // Check for required privilege
        //
        if (!SeSinglePrivilegeCheck(SeLockMemoryPrivilege, PreviousMode))
        {
            //
            // Fail: Don't have it
            //
            ObDereferenceObject(Process);
            return STATUS_PRIVILEGE_NOT_HELD;
        }
    }
    
    //
    // Check if we should attach
    //
    if (CurrentProcess != Process)
    {
        //
        // Do it
        //
        KeStackAttachProcess(&Process->Pcb, &ApcState);
        Attached = TRUE;
    }
    
    //
    // Oops :(
    //
    UNIMPLEMENTED;
    
    //
    // Detach if needed
    //
    if (Attached) KeUnstackDetachProcess(&ApcState);
    
    //
    // Release reference
    //
    ObDereferenceObject(Process);
    
    //
    // Enter SEH to return data
    //
    _SEH2_TRY
    {
        //
        // Return data to user
        //
        *BaseAddress = PAGE_ALIGN(CapturedBaseAddress);
        *NumberOfBytesToUnlock = 0;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        //
        // Get exception code
        //
        _SEH2_YIELD(return _SEH2_GetExceptionCode());
    }
    _SEH2_END;
    
    //
    // Return status
    //
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
NtFlushVirtualMemory(IN HANDLE ProcessHandle,
                     IN OUT PVOID *BaseAddress,
                     IN OUT PSIZE_T NumberOfBytesToFlush,
                     OUT PIO_STATUS_BLOCK IoStatusBlock)
{
    PEPROCESS Process;
    NTSTATUS Status;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    PVOID CapturedBaseAddress;
    SIZE_T CapturedBytesToFlush;
    IO_STATUS_BLOCK LocalStatusBlock;
    PAGED_CODE();
    
    //
    // Check if we came from user mode
    //
    if (PreviousMode != KernelMode)
    {
        //
        // Enter SEH for probing
        //
        _SEH2_TRY
        {
            //
            // Validate all outputs
            //
            ProbeForWritePointer(BaseAddress);
            ProbeForWriteSize_t(NumberOfBytesToFlush);
            ProbeForWriteIoStatusBlock(IoStatusBlock);
            
            //
            // Capture them
            //
            CapturedBaseAddress = *BaseAddress;
            CapturedBytesToFlush = *NumberOfBytesToFlush;
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            //
            // Get exception code
            //
            _SEH2_YIELD(return _SEH2_GetExceptionCode());
        }
        _SEH2_END;
    }
    else
    {
        //
        // Capture directly
        //
        CapturedBaseAddress = *BaseAddress;
        CapturedBytesToFlush = *NumberOfBytesToFlush;
    }
    
    //
    // Catch illegal base address
    //
    if (CapturedBaseAddress > MM_HIGHEST_USER_ADDRESS) return STATUS_INVALID_PARAMETER;
    
    //
    // Catch illegal region size
    //
    if ((MmUserProbeAddress - (ULONG_PTR)CapturedBaseAddress) < CapturedBytesToFlush)
    {
        //
        // Fail
        //
        return STATUS_INVALID_PARAMETER;
    }
    
    //
    // Get a reference to the process
    //
    Status = ObReferenceObjectByHandle(ProcessHandle,
                                       PROCESS_VM_OPERATION,
                                       PsProcessType,
                                       PreviousMode,
                                       (PVOID*)(&Process),
                                       NULL);
    if (!NT_SUCCESS(Status)) return Status;
    
    //
    // Do it
    //
    Status = MmFlushVirtualMemory(Process,
                                  &CapturedBaseAddress,
                                  &CapturedBytesToFlush,
                                  &LocalStatusBlock);
    
    //
    // Release reference
    //
    ObDereferenceObject(Process);
    
    //
    // Enter SEH to return data
    //
    _SEH2_TRY
    {
        //
        // Return data to user
        //
        *BaseAddress = PAGE_ALIGN(CapturedBaseAddress);
        *NumberOfBytesToFlush = 0;
        *IoStatusBlock = LocalStatusBlock;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
    }
    _SEH2_END;
    
    //
    // Return status
    //
    return Status;
}

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
NtGetWriteWatch(IN HANDLE ProcessHandle,
                IN ULONG Flags,
                IN PVOID BaseAddress,
                IN SIZE_T RegionSize,
                IN PVOID *UserAddressArray,
                OUT PULONG_PTR EntriesInUserAddressArray,
                OUT PULONG Granularity)
{
    PEPROCESS Process;
    NTSTATUS Status;
    PVOID EndAddress;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    ULONG_PTR CapturedEntryCount;
    PAGED_CODE();
    
    //
    // Check if we came from user mode
    //
    if (PreviousMode != KernelMode)
    {
        //
        // Enter SEH for probing
        //
        _SEH2_TRY
        {
            //
            // Catch illegal base address
            //
            if (BaseAddress > MM_HIGHEST_USER_ADDRESS) return STATUS_INVALID_PARAMETER_2;
            
            //
            // Catch illegal region size
            //
            if ((MmUserProbeAddress - (ULONG_PTR)BaseAddress) < RegionSize)
            {
                //
                // Fail
                //
                return STATUS_INVALID_PARAMETER_3;
            }
            
            //
            // Validate all data
            //
            ProbeForWriteSize_t(EntriesInUserAddressArray);
            ProbeForWriteUlong(Granularity);
            
            //
            // Capture them
            //
            CapturedEntryCount = *EntriesInUserAddressArray;
            
            //
            // Must have a count
            //
            if (CapturedEntryCount == 0) return STATUS_INVALID_PARAMETER_5;
            
            //
            // Can't be larger than the maximum
            //
            if (CapturedEntryCount > (MAXULONG_PTR / sizeof(ULONG_PTR)))
            {
                //
                // Fail
                //
                return STATUS_INVALID_PARAMETER_5;
            }
            
            //
            // Probe the actual array
            //
            ProbeForWrite(UserAddressArray,
                          CapturedEntryCount * sizeof(PVOID),
                          sizeof(PVOID));
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            //
            // Get exception code
            //
            _SEH2_YIELD(return _SEH2_GetExceptionCode());
        }
        _SEH2_END;
    }
    else
    {
        //
        // Capture directly
        //
        CapturedEntryCount = *EntriesInUserAddressArray;
        ASSERT(CapturedEntryCount != 0);
    }
    
    //
    // Check if this is a local request
    //
    if (ProcessHandle == NtCurrentProcess())
    {
        //
        // No need to reference the process
        //
        Process = PsGetCurrentProcess();
    }
    else 
    {
        //
        // Reference the target
        //
        Status = ObReferenceObjectByHandle(ProcessHandle,
                                           PROCESS_VM_OPERATION,
                                           PsProcessType,
                                           PreviousMode,
                                           (PVOID *)&Process,
                                           NULL);
        if (!NT_SUCCESS(Status)) return Status;
    }
    
    //
    // Compute the last address and validate it
    //
    EndAddress = (PVOID)((ULONG_PTR)BaseAddress + RegionSize - 1);
    if (BaseAddress > EndAddress)
    {
        //
        // Fail
        //
        if (ProcessHandle != NtCurrentProcess()) ObDereferenceObject(Process);
        return STATUS_INVALID_PARAMETER_4;
    }
    
    //
    // Oops :(
    //
    UNIMPLEMENTED;
    
    //
    // Dereference if needed
    //
    if (ProcessHandle != NtCurrentProcess()) ObDereferenceObject(Process);
    
    //
    // Enter SEH to return data
    //
    _SEH2_TRY
    {
        //
        // Return data to user
        //
        *EntriesInUserAddressArray = 0;
        *Granularity = PAGE_SIZE;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        //
        // Get exception code
        //
        Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END;
    
    //
    // Return success
    //
    return STATUS_SUCCESS;
}

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
NtResetWriteWatch(IN HANDLE ProcessHandle,
                  IN PVOID BaseAddress,
                  IN SIZE_T RegionSize)
{
    PVOID EndAddress;
    PEPROCESS Process;
    NTSTATUS Status;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    ASSERT (KeGetCurrentIrql() == PASSIVE_LEVEL);
    
    //
    // Catch illegal base address
    //
    if (BaseAddress > MM_HIGHEST_USER_ADDRESS) return STATUS_INVALID_PARAMETER_2;
    
    //
    // Catch illegal region size
    //
    if ((MmUserProbeAddress - (ULONG_PTR)BaseAddress) < RegionSize)
    {
        //
        // Fail
        //
        return STATUS_INVALID_PARAMETER_3;
    }
    
    //
    // Check if this is a local request
    //
    if (ProcessHandle == NtCurrentProcess())
    {
        //
        // No need to reference the process
        //
        Process = PsGetCurrentProcess();
    }
    else 
    {
        //
        // Reference the target
        //
        Status = ObReferenceObjectByHandle(ProcessHandle,
                                           PROCESS_VM_OPERATION,
                                           PsProcessType,
                                           PreviousMode,
                                           (PVOID *)&Process,
                                           NULL);
        if (!NT_SUCCESS(Status)) return Status;
    }
    
    //
    // Compute the last address and validate it
    //
    EndAddress = (PVOID)((ULONG_PTR)BaseAddress + RegionSize - 1);
    if (BaseAddress > EndAddress)
    {
        //
        // Fail
        //
        if (ProcessHandle != NtCurrentProcess()) ObDereferenceObject(Process);
        return STATUS_INVALID_PARAMETER_3;
    }
    
    //
    // Oops :(
    //
    UNIMPLEMENTED;
    
    //
    // Dereference if needed
    //
    if (ProcessHandle != NtCurrentProcess()) ObDereferenceObject(Process);
    
    //
    // Return success
    //
    return STATUS_SUCCESS;
}

/* EOF */
