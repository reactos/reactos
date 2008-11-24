/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/ps/state.c
 * PURPOSE:         Process Manager: Process/Thread State Control
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 *                  Thomas Weidenmueller (w3seek@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* PRIVATE FUNCTIONS *********************************************************/

VOID
NTAPI
PspQueueApcSpecialApc(IN PKAPC Apc,
                      IN OUT PKNORMAL_ROUTINE* NormalRoutine,
                      IN OUT PVOID* NormalContext,
                      IN OUT PVOID* SystemArgument1,
                      IN OUT PVOID* SystemArgument2)
{
    /* Free the APC and do nothing else */
    ExFreePool(Apc);
}

NTSTATUS
NTAPI
PsResumeThread(IN PETHREAD Thread,
               OUT PULONG PreviousCount OPTIONAL)
{
    ULONG OldCount;
    PAGED_CODE();

    /* Resume the thread */
    OldCount = KeResumeThread(&Thread->Tcb);

    /* Return the count if asked */
    if (PreviousCount) *PreviousCount = OldCount;
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
PsSuspendThread(IN PETHREAD Thread,
                OUT PULONG PreviousCount OPTIONAL)
{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG OldCount = 0;
    PAGED_CODE();

    /* Guard with SEH because KeSuspendThread can raise an exception */
    _SEH2_TRY
    {
        /* Check if we're suspending ourselves */
        if (Thread == PsGetCurrentThread())
        {
            /* Do the suspend */
            OldCount = KeSuspendThread(&Thread->Tcb);
        }
        else
        {
            /* Acquire rundown */
            if (ExAcquireRundownProtection(&Thread->RundownProtect))
            {
                /* Make sure the thread isn't terminating */
                if (Thread->Terminated)
                {
                    /* Fail */
                    Status = STATUS_THREAD_IS_TERMINATING;
                }
                else
                {
                    /* Otherwise, do the suspend */
                    OldCount = KeSuspendThread(&Thread->Tcb);

                    /* Check if it terminated during the suspend */
                    if (Thread->Terminated)
                    {
                        /* Wake it back up and fail */
                        KeForceResumeThread(&Thread->Tcb);
                        Status = STATUS_THREAD_IS_TERMINATING;
                        OldCount = 0;
                    }
                }

                /* Release rundown protection */
                ExReleaseRundownProtection(&Thread->RundownProtect);
            }
            else
            {
                /* Thread is terminating */
                Status = STATUS_THREAD_IS_TERMINATING;
            }
        }
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = _SEH2_GetExceptionCode();

        /* Don't fail if we merely couldn't write the handle back */
        if (Status != STATUS_SUSPEND_COUNT_EXCEEDED) Status = STATUS_SUCCESS;
    }
    _SEH2_END;

    /* Write back the previous count */
    if (PreviousCount) *PreviousCount = OldCount;
    return Status;
}

NTSTATUS
NTAPI
PsResumeProcess(IN PEPROCESS Process)
{
    PETHREAD Thread;
    PAGED_CODE();

    /* Lock the Process */
    if (!ExAcquireRundownProtection(&Process->RundownProtect))
    {
        /* Process is terminating */
        return STATUS_PROCESS_IS_TERMINATING;
    }

    /* Get the first thread */
    Thread = PsGetNextProcessThread(Process, NULL);
    while (Thread)
    {
        /* Resume it */
        KeResumeThread(&Thread->Tcb);

        /* Move to the next thread */
        Thread = PsGetNextProcessThread(Process, Thread);
    }

    /* Unlock the process */
    ExReleaseRundownProtection(&Process->RundownProtect);
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
PsSuspendProcess(IN PEPROCESS Process)
{
    PETHREAD Thread;
    PAGED_CODE();

    /* Lock the Process */
    if (!ExAcquireRundownProtection(&Process->RundownProtect))
    {
        /* Process is terminating */
        return STATUS_PROCESS_IS_TERMINATING;
    }

    /* Get the first thread */
    Thread = PsGetNextProcessThread(Process, NULL);
    while (Thread)
    {
        /* Resume it */
        PsSuspendThread(Thread, NULL);

        /* Move to the next thread */
        Thread = PsGetNextProcessThread(Process, Thread);
    }

    /* Unlock the process */
    ExReleaseRundownProtection(&Process->RundownProtect);
    return STATUS_SUCCESS;
}

/* PUBLIC FUNCTIONS **********************************************************/

/*
 * @implemented
 */
NTSTATUS
NTAPI
NtAlertThread(IN HANDLE ThreadHandle)
{
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    PETHREAD Thread;
    NTSTATUS Status;

    /* Reference the Object */
    Status = ObReferenceObjectByHandle(ThreadHandle,
                                       THREAD_SUSPEND_RESUME,
                                       PsThreadType,
                                       PreviousMode,
                                       (PVOID*)&Thread,
                                       NULL);
    if (NT_SUCCESS(Status))
    {
        /*
         * Do an alert depending on the processor mode. If some kmode code wants to
         * enforce a umode alert it should call KeAlertThread() directly. If kmode
         * code wants to do a kmode alert it's sufficient to call it with Zw or just
         * use KeAlertThread() directly
         */
        KeAlertThread(&Thread->Tcb, PreviousMode);

        /* Dereference Object */
        ObDereferenceObject(Thread);
    }

    /* Return status */
    return Status;
}

NTSTATUS
NTAPI
NtAlertResumeThread(IN HANDLE ThreadHandle,
                    OUT PULONG SuspendCount)
{
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    PETHREAD Thread;
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG PreviousState;

    /* Check if we came from user mode with a suspend count */
    if ((SuspendCount) && (PreviousMode != KernelMode))
    {
        /* Enter SEH for probing */
        _SEH2_TRY
        {
            /* Probe the count */
            ProbeForWriteUlong(SuspendCount);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            /* Get the exception code */
            Status = _SEH2_GetExceptionCode();
        }
        _SEH2_END;
        if (!NT_SUCCESS(Status)) return Status;
    }

    /* Reference the Object */
    Status = ObReferenceObjectByHandle(ThreadHandle,
                                       THREAD_SUSPEND_RESUME,
                                       PsThreadType,
                                       PreviousMode,
                                       (PVOID*)&Thread,
                                       NULL);
    if (NT_SUCCESS(Status))
    {
        /* Call the Kernel Function */
        PreviousState = KeAlertResumeThread(&Thread->Tcb);

        /* Dereference Object */
        ObDereferenceObject(Thread);

        /* Check if the caller gave a suspend count */
        if (SuspendCount)
        {
            /* Enter SEH for write */
            _SEH2_TRY
            {
                /* Write state back */
                *SuspendCount = PreviousState;
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                /* Get exception code */
                Status = _SEH2_GetExceptionCode();
            }
            _SEH2_END;
        }
    }

    /* Return status */
    return Status;
}

NTSTATUS
NTAPI
NtResumeThread(IN HANDLE ThreadHandle,
               OUT PULONG SuspendCount OPTIONAL)
{
    PETHREAD Thread;
    ULONG Prev;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    NTSTATUS Status = STATUS_SUCCESS;
    PAGED_CODE();

    /* Check if caller gave a suspend count from user mode */
    if ((SuspendCount) && (PreviousMode != KernelMode))
    {
        /* Enter SEH for probing */
        _SEH2_TRY
        {
            /* Probe the count */
            ProbeForWriteUlong(SuspendCount);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            /* Get the exception code */
            Status = _SEH2_GetExceptionCode();
        }
        _SEH2_END;
        if(!NT_SUCCESS(Status)) return Status;
    }

    /* Get the Thread Object */
    Status = ObReferenceObjectByHandle(ThreadHandle,
                                       THREAD_SUSPEND_RESUME,
                                       PsThreadType,
                                       PreviousMode,
                                       (PVOID*)&Thread,
                                       NULL);
    if (!NT_SUCCESS(Status)) return Status;

    /* Call the internal function */
    Status = PsResumeThread(Thread, &Prev);

    /* Check if the caller wanted the count back */
    if (SuspendCount)
    {
        /* Enter SEH for write back */
        _SEH2_TRY
        {
            /* Write the count */
            *SuspendCount = Prev;
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            /* Get the exception code */
            Status = _SEH2_GetExceptionCode();
        }
        _SEH2_END;
    }

    /* Dereference and return */
    ObDereferenceObject(Thread);
    return Status;
}

NTSTATUS
NTAPI
NtSuspendThread(IN HANDLE ThreadHandle,
                OUT PULONG PreviousSuspendCount OPTIONAL)
{
    PETHREAD Thread;
    ULONG Prev;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    NTSTATUS Status = STATUS_SUCCESS;
    PAGED_CODE();

    /* Check if caller gave a suspend count from user mode */
    if ((PreviousSuspendCount) && (PreviousMode != KernelMode))
    {
        /* Enter SEH for probing */
        _SEH2_TRY
        {
            /* Probe the count */
            ProbeForWriteUlong(PreviousSuspendCount);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            /* Get the exception code */
            Status = _SEH2_GetExceptionCode();
        }
        _SEH2_END;
        if(!NT_SUCCESS(Status)) return Status;
    }

    /* Get the Thread Object */
    Status = ObReferenceObjectByHandle(ThreadHandle,
                                       THREAD_SUSPEND_RESUME,
                                       PsThreadType,
                                       PreviousMode,
                                       (PVOID*)&Thread,
                                       NULL);
    if (!NT_SUCCESS(Status)) return Status;

    /* Call the internal function */
    Status = PsSuspendThread(Thread, &Prev);
    ObDereferenceObject(Thread);
    if (!NT_SUCCESS(Status)) return Status;

    /* Protect write with SEH */
    _SEH2_TRY
    {
        /* Return the Previous Count */
        if (PreviousSuspendCount) *PreviousSuspendCount = Prev;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        /* Get the exception code */
        Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END;

    /* Return */
    return Status;
}

NTSTATUS
NTAPI
NtSuspendProcess(IN HANDLE ProcessHandle)
{
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    PEPROCESS Process;
    NTSTATUS Status;
    PAGED_CODE();

    /* Reference the process */
    Status = ObReferenceObjectByHandle(ProcessHandle,
                                       PROCESS_SUSPEND_RESUME,
                                       PsProcessType,
                                       PreviousMode,
                                       (PVOID*)&Process,
                                       NULL);
    if (NT_SUCCESS(Status))
    {
        /* Call the internal function */
        Status = PsSuspendProcess(Process);
        ObDereferenceObject(Process);
    }

    /* Return status */
    return Status;
}

NTSTATUS
NTAPI
NtResumeProcess(IN HANDLE ProcessHandle)
{
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    PEPROCESS Process;
    NTSTATUS Status;
    PAGED_CODE();

    /* Reference the process */
    Status = ObReferenceObjectByHandle(ProcessHandle,
                                       PROCESS_SUSPEND_RESUME,
                                       PsProcessType,
                                       PreviousMode,
                                       (PVOID*)&Process,
                                       NULL);
    if (NT_SUCCESS(Status))
    {
        /* Call the internal function */
        Status = PsResumeProcess(Process);
        ObDereferenceObject(Process);
    }

    /* Return status */
    return Status;
}

NTSTATUS
NTAPI
NtTestAlert(VOID)
{
    /* Check and Alert Thread if needed */
    return KeTestAlertThread(ExGetPreviousMode()) ?
           STATUS_ALERTED : STATUS_SUCCESS;
}

/*++
 * @name NtQueueApcThread
 * NT4
 *
 *    This routine is used to queue an APC from user-mode for the specified
 *    thread.
 *
 * @param ThreadHandle
 *        Handle to the Thread.
 *        This handle must have THREAD_SET_CONTEXT privileges.
 *
 * @param ApcRoutine
 *        Pointer to the APC Routine to call when the APC executes.
 *
 * @param NormalContext
 *        Pointer to the context to send to the Normal Routine.
 *
 * @param SystemArgument[1-2]
 *        Pointer to a set of two parameters that contain untyped data.
 *
 * @return STATUS_SUCCESS or failure cute from associated calls.
 *
 * @remarks The thread must enter an alertable wait before the APC will be
 *          delivered.
 *
 *--*/
NTSTATUS
NTAPI
NtQueueApcThread(IN HANDLE ThreadHandle,
                 IN PKNORMAL_ROUTINE ApcRoutine,
                 IN PVOID NormalContext,
                 IN PVOID SystemArgument1,
                 IN PVOID SystemArgument2)
{
    PKAPC Apc;
    PETHREAD Thread;
    NTSTATUS Status = STATUS_SUCCESS;
    PAGED_CODE();

    /* Get ETHREAD from Handle */
    Status = ObReferenceObjectByHandle(ThreadHandle,
                                       THREAD_SET_CONTEXT,
                                       PsThreadType,
                                       ExGetPreviousMode(),
                                       (PVOID)&Thread,
                                       NULL);
    if (NT_SUCCESS(Status)) return Status;

    /* Check if this is a System Thread */
    if (Thread->SystemThread)
    {
        /* Fail */
        Status = STATUS_INVALID_HANDLE;
        goto Quit;
    }

    /* Allocate an APC */
    Apc = ExAllocatePoolWithTag(NonPagedPool |
                                POOL_QUOTA_FAIL_INSTEAD_OF_RAISE,
                                sizeof(KAPC),
                                TAG_PS_APC);
    if (!Apc)
    {
        /* Fail */
        Status = STATUS_NO_MEMORY;
        goto Quit;
    }

    /* Initialize the APC */
    KeInitializeApc(Apc,
                    &Thread->Tcb,
                    OriginalApcEnvironment,
                    PspQueueApcSpecialApc,
                    NULL,
                    ApcRoutine,
                    UserMode,
                    NormalContext);

    /* Queue it */
    if (!KeInsertQueueApc(Apc,
                          SystemArgument1,
                          SystemArgument2,
                          IO_NO_INCREMENT))
    {
        /* We failed, free it */
        ExFreePool(Apc);
        Status = STATUS_UNSUCCESSFUL;
    }

    /* Dereference Thread and Return */
Quit:
    ObDereferenceObject(Thread);
    return Status;
}

/* EOF */
