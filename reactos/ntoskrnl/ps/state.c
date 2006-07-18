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
#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

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
        _SEH_TRY
        {
            /* Probe the count */
            ProbeForWriteUlong(SuspendCount);
        }
        _SEH_HANDLE
        {
            /* Get the exception code */
            Status = _SEH_GetExceptionCode();
        }
        _SEH_END;

        /* Fail on exception */
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
            _SEH_TRY
            {
                /* Write state back */
                *SuspendCount = PreviousState;
            }
            _SEH_HANDLE
            {
                /* Get exception code */
                Status = _SEH_GetExceptionCode();
            }
            _SEH_END;
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
        _SEH_TRY
        {
            /* Probe the count */
            ProbeForWriteUlong(SuspendCount);
        }
        _SEH_HANDLE
        {
            /* Get the exception code */
            Status = _SEH_GetExceptionCode();
        }
        _SEH_END;

        /* Fail on exception */
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

    /* Call the Kernel Function */
    Prev = KeResumeThread(&Thread->Tcb);

    /* Check if the caller wanted the count back */
    if (SuspendCount)
    {
        /* Enter SEH for write back */
        _SEH_TRY
        {
            /* Write the count */
            *SuspendCount = Prev;
        }
        _SEH_HANDLE
        {
            /* Get the exception code */
            Status = _SEH_GetExceptionCode();
        }
        _SEH_END;
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
        _SEH_TRY
        {
            /* Probe the count */
            ProbeForWriteUlong(PreviousSuspendCount);
        }
        _SEH_HANDLE
        {
            /* Get the exception code */
            Status = _SEH_GetExceptionCode();
        }
        _SEH_END;

        /* Fail on exception */
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

    /* Guard with SEH because KeSuspendThread can raise an exception */
    _SEH_TRY
    {
        /* Make sure the thread isn't terminating */
        if ((Thread != PsGetCurrentThread()) && (Thread->Terminated))
        {
            ObDereferenceObject(Thread);
            return STATUS_THREAD_IS_TERMINATING;
        }

        /* Call the Kernel function */
        Prev = KeSuspendThread(&Thread->Tcb);

        /* Return the Previous Count */
        if (PreviousSuspendCount) *PreviousSuspendCount = Prev;
    }
    _SEH_HANDLE
    {
        Status = _SEH_GetExceptionCode();

        /* Don't fail if we merely couldn't write the handle back */
        if (Status != STATUS_SUSPEND_COUNT_EXCEEDED) Status = STATUS_SUCCESS;
    }
    _SEH_END;

    /* Return */
    ObDereferenceObject(Thread);
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
        /* FIXME */
        Status = STATUS_NOT_IMPLEMENTED;
        DPRINT1("NtSuspendProcess not yet implemented!\n");
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
        /* FIXME */
        Status = STATUS_NOT_IMPLEMENTED;
        DPRINT1("NtResumeProcess not yet implemented!\n");
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

/* EOF */
