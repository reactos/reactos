/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ps/suspend.c
 * PURPOSE:         Thread managment
 *
 * PROGRAMMERS:     David Welch (welch@mcmail.com)
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

ULONG
STDCALL
KeResumeThread(PKTHREAD Thread);

/* FUNCTIONS *****************************************************************/

/*
 * FUNCTION: Decrements a thread's resume count
 * ARGUMENTS:
 *        ThreadHandle = Handle to the thread that should be resumed
 *        ResumeCount =  The resulting resume count.
 * RETURNS: Status
 */
NTSTATUS
STDCALL
NtResumeThread(IN HANDLE ThreadHandle,
               IN PULONG SuspendCount  OPTIONAL)
{
    PETHREAD Thread;
    ULONG Prev;
    KPROCESSOR_MODE PreviousMode;
    NTSTATUS Status = STATUS_SUCCESS;

    PAGED_CODE();

    PreviousMode = ExGetPreviousMode();

    DPRINT("NtResumeThead(ThreadHandle %lx  SuspendCount %p)\n",
           ThreadHandle, SuspendCount);

    /* Check buffer validity */
    if(SuspendCount && PreviousMode == UserMode) {

        _SEH_TRY {

            ProbeForWriteUlong(SuspendCount);
         } _SEH_HANDLE {

            Status = _SEH_GetExceptionCode();

        } _SEH_END;

        if(!NT_SUCCESS(Status)) return Status;
    }

    /* Get the Thread Object */
    Status = ObReferenceObjectByHandle(ThreadHandle,
                                       THREAD_SUSPEND_RESUME,
                                       PsThreadType,
                                       PreviousMode,
                                       (PVOID*)&Thread,
                                       NULL);
    if (!NT_SUCCESS(Status)) {

        return Status;
    }

    /* Call the Kernel Function */
    Prev = KeResumeThread(&Thread->Tcb);

    /* Return it */
    if(SuspendCount) {

        _SEH_TRY {

            *SuspendCount = Prev;

        } _SEH_HANDLE {

            Status = _SEH_GetExceptionCode();

        } _SEH_END;
    }

    /* Dereference and Return */
    ObDereferenceObject ((PVOID)Thread);
    return Status;
}

/*
 * FUNCTION: Increments a thread's suspend count
 * ARGUMENTS:
 *        ThreadHandle = Handle to the thread that should be resumed
 *        PreviousSuspendCount =  The resulting/previous suspend count.
 * REMARK:
 *        A thread will be suspended if its suspend count is greater than 0.
 *        This procedure maps to the win32 SuspendThread function. (
 *        documentation about the the suspend count can be found here aswell )
 *        The suspend count is not increased if it is greater than
 *        MAXIMUM_SUSPEND_COUNT.
 * RETURNS: Status
 */
NTSTATUS
STDCALL
NtSuspendThread(IN HANDLE ThreadHandle,
                IN PULONG PreviousSuspendCount  OPTIONAL)
{
    PETHREAD Thread;
    ULONG Prev;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    NTSTATUS Status = STATUS_SUCCESS;
    PAGED_CODE();

    /* Check buffer validity */
    if(PreviousSuspendCount && PreviousMode != KernelMode)
    {
        _SEH_TRY
        {
            ProbeForWriteUlong(PreviousSuspendCount);
         }
        _SEH_HANDLE
        {
            Status = _SEH_GetExceptionCode();
        } _SEH_END;

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
    } _SEH_END;

    /* Return */
    ObDereferenceObject(Thread);
    return Status;
}


/*
 * @unimplemented
 */
NTSTATUS
STDCALL
NtSuspendProcess(IN HANDLE ProcessHandle)
{
    KPROCESSOR_MODE PreviousMode;
    PEPROCESS Process;
    NTSTATUS Status;

    PAGED_CODE();

    PreviousMode = ExGetPreviousMode();

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

    return Status;
}


/*
 * @unimplemented
 */
NTSTATUS
STDCALL
NtResumeProcess(IN HANDLE ProcessHandle)
{
    KPROCESSOR_MODE PreviousMode;
    PEPROCESS Process;
    NTSTATUS Status;

    PAGED_CODE();

    PreviousMode = ExGetPreviousMode();

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

    return Status;
}

/* EOF */
