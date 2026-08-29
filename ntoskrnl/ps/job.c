/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Core functions for managing Job Objects, a kernel mechanism
 *              for managing multiple processes as a single unit.
 * COPYRIGHT:   Copyright 2004-2012 Alex Ionescu (alex@relsoft.net) (stubs)
 *              Copyright 2004-2005 Thomas Weidenmueller <w3seek@reactos.com>
 *              Copyright 2015-2016 Samuel Serapión Vega <encoded@reactos.org>
 *              Copyright 2017 Mark Jansen <mark.jansen@reactos.org>
 *              Copyright 2018 Pierre Schweitzer <pierre@reactos.org>
 *              Copyright 2022 Timo Kreuzer <timo.kreuzer@reactos.org>
 *              Copyright 2024-2026 Gleb Surikov <glebs.surikovs@gmail.com>
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

POBJECT_TYPE PsJobType = NULL;

LIST_ENTRY PsJobListHead;
static FAST_MUTEX PsJobListLock;

BOOLEAN PspUseJobSchedulingClasses;

CHAR PspJobSchedulingClasses[PSP_JOB_SCHEDULING_CLASSES] =
{
    1 * 6,
    2 * 6,
    3 * 6,
    4 * 6,
    5 * 6,
    6 * 6,
    7 * 6,
    8 * 6,
    9 * 6,
    10 * 6
};

GENERIC_MAPPING PspJobMapping =
{
    STANDARD_RIGHTS_READ | JOB_OBJECT_QUERY,

    STANDARD_RIGHTS_WRITE | JOB_OBJECT_ASSIGN_PROCESS |
    JOB_OBJECT_SET_ATTRIBUTES | JOB_OBJECT_TERMINATE,

    STANDARD_RIGHTS_EXECUTE | SYNCHRONIZE,

    STANDARD_RIGHTS_ALL | THREAD_ALL_ACCESS // bug fixed only in vista
};

ULONG PspJobInfoLengths[] =
{
    0x0,
    sizeof(JOBOBJECT_BASIC_ACCOUNTING_INFORMATION),
    sizeof(JOBOBJECT_BASIC_LIMIT_INFORMATION),
    sizeof(JOBOBJECT_BASIC_PROCESS_ID_LIST),
    sizeof(JOBOBJECT_BASIC_UI_RESTRICTIONS),
    sizeof(JOBOBJECT_SECURITY_LIMIT_INFORMATION),
    sizeof(JOBOBJECT_END_OF_JOB_TIME_INFORMATION),
    sizeof(JOBOBJECT_ASSOCIATE_COMPLETION_PORT),
    sizeof(JOBOBJECT_BASIC_AND_IO_ACCOUNTING_INFORMATION),
    sizeof(JOBOBJECT_EXTENDED_LIMIT_INFORMATION),
    0x4
};

ULONG PspJobInfoAlign[] =
{
    0x0,
    sizeof(ULONG),
    sizeof(ULONG),
    sizeof(ULONG),
    sizeof(ULONG),
    sizeof(ULONG),
    sizeof(ULONG),
    sizeof(HANDLE),
    sizeof(ULONG),
    sizeof(ULONG),
    sizeof(ULONG)
};

/* DATA TYPE DEFINITIONS *****************************************************/

/*!
 * Context structure used to pass the job object and exit status to
 * the process termination callback.
 *
 * @param[in] Job
 *     A pointer to the job object.
 *
 * @param[in] ExitStatus
 *     The exit status to be used for all terminated processes.
 */
typedef struct PSP_TERMINATE_PROCESS_CONTEXT
{
    PEJOB Job;
    NTSTATUS ExitStatus;
} PSP_TERMINATE_PROCESS_CONTEXT, *PPSP_TERMINATE_PROCESS_CONTEXT;

/*!
 * Context structure used to collect process IDs for a job object.
 *
 * @param[in, out] ProcessIdList
 *     A pointer to the output process identifier list.
 *
 * @param[in, out] NextProcessId
 *     A pointer to the next output array entry.
 *
 * @param[in, out] RemainingLength
 *     The number of bytes remaining in the output array.
 */
typedef struct PSP_QUERY_JOB_PROCESS_ID_CONTEXT
{
    PJOBOBJECT_BASIC_PROCESS_ID_LIST ProcessIdList;
    PULONG_PTR NextProcessId;
    SIZE_T RemainingLength;
} PSP_QUERY_JOB_PROCESS_ID_CONTEXT, *PPSP_QUERY_JOB_PROCESS_ID_CONTEXT;

/* FUNCTIONS *****************************************************************/

CODE_SEG("INIT")
VOID
NTAPI
PspInitializeJobStructures(VOID)
{
    InitializeListHead(&PsJobListHead);
    ExInitializeFastMutex(&PsJobListLock);
}

/*!
 * Returns the next process directly assigned to a job while the job lock is
 * held.
 *
 * @param[in] Job
 *     A pointer to the job object being enumerated.
 *
 * @param[in, optional] PreviousProcess
 *     A pointer to the process after which enumeration should
 *     continue, or NULL to return the first process.
 *
 * @return
 *     A pointer to the next assigned process, or NULL if enumeration has completed.
 *
 * @remarks
 *     The caller must hold the job lock shared or exclusive.
 *
 *     Neither PreviousProcess nor the returned process is referenced by this
 *     function. The caller must ensure that PreviousProcess remains linked to
 *     the job and that direct job membership isn't modified during the
 *     enumeration.
 */
static
PEPROCESS
PspGetNextProcessInJobLocked(
    _In_ PEJOB Job,
    _In_opt_ PEPROCESS PreviousProcess
)
{
    PLIST_ENTRY Entry;
    PEPROCESS Process;

    ASSERT(ExIsResourceAcquiredSharedLite(&Job->JobLock) != 0 ||
           ExIsResourceAcquiredExclusiveLite(&Job->JobLock) != 0);

    /* Check if we're already starting somewhere */
    if (PreviousProcess != NULL)
    {
        ASSERT(PreviousProcess->Job == Job);
        ASSERT(!IsListEmpty(&PreviousProcess->JobLinks));

        /* Start where we left off */
        Entry = PreviousProcess->JobLinks.Flink;
    }
    else
    {
        /* Start at the beginning */
        Entry = Job->ProcessListHead.Flink;
    }

    /* Check if enumeration has completed */
    if (Entry == &Job->ProcessListHead)
    {
        return NULL;
    }

    ASSERT(Entry->Flink != NULL);
    ASSERT(Entry->Blink != NULL);
    ASSERT(Entry->Flink->Blink == Entry);
    ASSERT(Entry->Blink->Flink == Entry);

    Process = CONTAINING_RECORD(Entry, EPROCESS, JobLinks);
    ASSERT(Process->Job == Job);
    return Process;
}

/*!
 * Returns the next referenced process directly assigned to a job.
 *
 * @param[in] Job
 *     A pointer to the job object being enumerated.
 *
 * @param[in, optional] PreviousProcess
 *     A referenced process returned by the previous call, or NULL to start
 *     enumeration. When non-NULL, its reference is consumed by this function,
 *     regardless of whether another process is returned.
 *
 * @return
 *     A referenced pointer to the next process, or NULL if enumeration has
 *     completed.
 *
 * @remarks
 *     The returned reference must either be passed to the next call or
 *     released with ObDereferenceObject().
 *
 *     The job lock is not held when this function returns.
 *
 *     This relies on job membership remaining fixed until process object deletion.
 *     The reference to PreviousProcess therefore keeps its JobLinks valid until
 *     the next process has been selected.
 */
static
PEPROCESS
PspGetNextProcessInJob(
    _In_ PEJOB Job,
    _In_opt_ PEPROCESS PreviousProcess
)
{
    PEPROCESS Candidate;
    PEPROCESS NextProcess = NULL;

    ExEnterCriticalRegionAndAcquireResourceShared(&Job->JobLock);

    Candidate = PreviousProcess;

    while ((Candidate = PspGetNextProcessInJobLocked(Job, Candidate)) != NULL)
    {
        /*
         * Skip processes whose deletion has begun. JobLinks remains valid
         * while the job lock is held, allowing enumeration to continue.
         */
        if (ObReferenceObjectSafe(Candidate))
        {
            NextProcess = Candidate;
            break;
        }
    }

    ExReleaseResourceAndLeaveCriticalRegion(&Job->JobLock);

    /*
     * This must occur after releasing the job lock. The dereference can invoke
     * the process delete procedure, which removes JobLinks under the same lock.
     */
    if (PreviousProcess != NULL)
    {
        ObDereferenceObject(PreviousProcess);
    }

    return NextProcess;
}

/*!
 * Enumerates all processes currently associated with the specified job object
 * and invokes a callback for each process.
 *
 * @param[in] Job
 *     A pointer to the job object whose processes are to be enumerated.
 *
 * @param[in] Callback
 *     A pointer to the callback invoked for each referenced process.
 *
 * @param[in, optional] Context
 *     An optional context pointer passed to the callback.
 *
 * @return
 *     STATUS_SUCCESS if every callback succeeds. Otherwise, the first
 *     unsuccessful callback status is returned.
 *
 * @remarks
 *     Enumeration stops when a callback returns an unsuccessful status.
 *
 *     The callback is invoked _without_ the job lock held.
 */
NTSTATUS
NTAPI
PspEnumerateProcessesInJob(
    _In_ PEJOB Job,
    _In_ PJOB_ENUMERATOR_CALLBACK Callback,
    _In_opt_ PVOID Context
)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PEPROCESS Process;

    /* Iterate through all processes in the job */
    for (Process = PspGetNextProcessInJob(Job, NULL);
         Process != NULL;
         Process = PspGetNextProcessInJob(Job, Process))
    {
        Status = Callback(Process, Context);
        if (!NT_SUCCESS(Status))
        {
            /*
             * On successful iteration, PspGetNextProcessInJob consumes
             * this reference. On failure, it must be released explicitly.
             */
            ObDereferenceObject(Process);
            break;
        }
    }

    return Status;
}

/*!
 * Enumerates all assigned processes while the caller holds the job
 * lock and invokes a callback for each process.
 *
 * @param[in] Job
 *     A pointer to the job object whose processes are to be enumerated.
 *
 * @param[in] Callback
 *     A pointer to the callback invoked for each process.
 *
 * @param[in, optional] Context
 *     An optional context pointer passed to the callback.
 *
 * @return
 *     STATUS_SUCCESS if every callback succeeds. Otherwise, the first
 *     unsuccessful callback status is returned.
 *
 * @remarks
 *     Enumeration stops when a callback returns an unsuccessful status.
 *
 *     The caller must hold the job lock shared or exclusive.
 *
 *     The callback must not release or recursively acquire the job lock,
 *     modify job membership, dereference the process object, or retain
 *     the process pointer after returning.
 */
static
NTSTATUS
PspEnumerateProcessesInJobLocked(
    _In_ PEJOB Job,
    _In_ PJOB_ENUMERATOR_CALLBACK Callback,
    _In_opt_ PVOID Context
)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PEPROCESS Process;

    ASSERT(ExIsResourceAcquiredSharedLite(&Job->JobLock) != 0 ||
           ExIsResourceAcquiredExclusiveLite(&Job->JobLock) != 0);

    /* Iterate through all processes in the job */
    for (Process = PspGetNextProcessInJobLocked(Job, NULL);
         Process != NULL;
         Process = PspGetNextProcessInJobLocked(Job, Process))
    {
        Status = Callback(Process, Context);
        if (!NT_SUCCESS(Status))
            break;
    }

    return Status;
}

/*!
 * Queues a message to a job's completion port.
 *
 * @param[in] Job
 *     A pointer to the job receiving the notification.
 *
 * @param[in] Message
 *     The job notification message (JOB_OBJECT_MSG_*).
 *
 * @param[in, optional] CompletionValue
 *     The message specific completion value.
 *
 * @param[in] Quota
 *     Specifies whether the completion packet is charged as quota.
 *
 * @return
 *     STATUS_SUCCESS if the message was queued successfully.
 *     Otherwise, an appropriate NTSTATUS error code.
 *
 * @remarks
 *     The caller must hold the job lock shared or exclusive.
 *     The caller must ensure that the job has an associated completion port.
 */
NTSTATUS
NTAPI
PspSendJobMessageLocked(
    _In_ PEJOB Job,
    _In_ ULONG Message,
    _In_opt_ PVOID CompletionValue,
    _In_ BOOLEAN Quota
)
{
    ASSERT(Job->CompletionPort != NULL);

    ASSERT(ExIsResourceAcquiredSharedLite(&Job->JobLock) != 0 ||
           ExIsResourceAcquiredExclusiveLite(&Job->JobLock) != 0);

    return IoSetIoCompletion(Job->CompletionPort,
                             Job->CompletionKey,
                             CompletionValue,
                             STATUS_SUCCESS,
                             Message,
                             Quota);
}

/*!
 * Assigns a process to a job object.
 
 * @param[in] Process
 *     Pointer to the process to be assigned to the job.
 *
 * @param[in] Job
 *     Pointer to the job object to which the process is to be assigned.
 *
 * @returns
 *     STATUS_SUCCESS if the process is successfully assigned to the job.
 *     An appropriate NTSTATUS error code otherwise.
 */
NTSTATUS
NTAPI
PspAssignProcessToJob(
    _In_ PEPROCESS Process,
    _In_ PEJOB Job
)
{
    NTSTATUS Status = STATUS_SUCCESS;
    NTSTATUS CalloutStatus = STATUS_SUCCESS;
    PVOID PreviousJob;

    if (!ExAcquireRundownProtection(&Process->RundownProtect))
    {
        return STATUS_PROCESS_IS_TERMINATING;
    }

    ExEnterCriticalRegionAndAcquireResourceExclusive(&Job->JobLock);

    /* https://learn.microsoft.com/en-us/windows/win32/api/jobapi2/nf-jobapi2-assignprocesstojobobject:
       "If the job or any of its parent jobs in the job chain is terminating
       when AssignProcessToJob is called, the function fails" */
    if (FlagOn(Job->JobFlags, PSP_JOB_TERMINATING))
    {
        Status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    /* Prevent processes from being added to the job if it is flagged
       for closing and has a limit on process termination on closing */
    if (FlagOn(Job->LimitFlags, JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE) &&
        FlagOn(Job->JobFlags, PSP_JOB_CLOSE_DONE))
    {
        Status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    /* Check if the job has a limit on the number of active processes */
    if (FlagOn(Job->LimitFlags, JOB_OBJECT_LIMIT_ACTIVE_PROCESS) &&
        Job->ActiveProcesses >= Job->ActiveProcessLimit)
    {
        /* Check if job limit on active processes has been reached */
        if (Job->CompletionPort)
        {
            /* If the job has a completion port, notify the job that the
               limit on the number of active processes has been exceeded */
            (VOID)PspSendJobMessageLocked(Job,
                                          JOB_OBJECT_MSG_ACTIVE_PROCESS_LIMIT,
                                          NULL,
                                          TRUE);
        }

        Status = STATUS_QUOTA_EXCEEDED;
        goto Exit;
    }

    /* Acquire the reference owned by Process->Job before publishing the pointer.
       This ensures that every observable non-NULL Process->Job is already
       backed by its lifetime reference. If another assignment wins the race,
       release the unused reference. */
    ObReferenceObject(Job);

    /* JobLock protects the target job, but another caller may simultaneously
       hold a different job's lock while trying to assign the same process */
    PreviousJob = InterlockedCompareExchangePointer((PVOID)&Process->Job,
                                                    Job,
                                                    NULL);
    if (PreviousJob)
    {
        ObDereferenceObject(Job);
        Status = STATUS_ACCESS_DENIED;
        goto Exit;
    }

    /* Assignment is committed at this point. No subsequent structural
       operation may fail.

       Readers of Job->ProcessListHead are blocked by JobLock until the list
       and counters are complete. */

    ASSERT(IsListEmpty(&Process->JobLinks));

    InsertTailList(&Job->ProcessListHead, &Process->JobLinks);

    Job->TotalProcesses++;
    Job->ActiveProcesses++;

    if (Job->CompletionPort && Process->UniqueProcessId)
    {
        /* If the job has a completion port and the process has a unique ID,
           notify the job of the new process */
        (VOID)PspSendJobMessageLocked(Job,
                                      JOB_OBJECT_MSG_NEW_PROCESS,
                                      Process->UniqueProcessId,
                                      FALSE);
    }

    /* Hand the process to win32k, which enforces the UI restrictions. One
       that has not connected to win32k yet is picked up when it does. */
    if (Job->UIRestrictionsClass != 0 && Process->Win32Process != NULL)
    {
        CalloutStatus = PspInvokeW32JobCallout(Job,
                                               PsW32JobCalloutAddProcess,
                                               Process->Win32Process);
    }

Exit:
    ExReleaseResourceAndLeaveCriticalRegion(&Job->JobLock);
    ExReleaseRundownProtection(&Process->RundownProtect);

    /* The assignment is committed, but win32k will not be enforcing the UI
       restrictions, so the process must not run at all. The caller is told
       the assignment failed, since that is what it asked for. */
    if (!NT_SUCCESS(CalloutStatus))
    {
        DPRINT1("Failed to apply the UI restrictions of job %p to process %p: 0x%lx\n",
                Job, Process, CalloutStatus);

        (VOID)PsTerminateProcess(Process, CalloutStatus);
        Status = CalloutStatus;
    }

    /* TODO: Ensure that job limits are respected */

    return Status;
}

/*!
 * Marks a process inactive in its assigned job.
 *
 * @param[in] Job
 *     A pointer to the process's assigned job.
 *
 * @param[in] Process
 *     A pointer to the process being marked inactive.
 *
 * @return
 *     TRUE if this call performed the active-to-inactive transition and
 *     reduced the job's active process count to zero; otherwise, FALSE.
 *
 * @remarks
 *     The caller must hold the job lock exclusively.
 */
static 
BOOLEAN
PspDeactivateProcessFromJobLocked(
    _In_ PEJOB Job,
    _In_ PEPROCESS Process
)
{
    ASSERT(Process->Job == Job);

    ASSERT(ExIsResourceAcquiredExclusiveLite(&Job->JobLock) != 0);

    if (FlagOn(Process->JobStatus, PSP_JOB_NOT_REALLY_ACTIVE))
    {
        return FALSE;
    }

    ASSERT(Job->ActiveProcesses != 0);

    Job->ActiveProcesses--;

    InterlockedOr((PLONG)&Process->JobStatus, PSP_JOB_NOT_REALLY_ACTIVE);

    return Job->ActiveProcesses == 0;
}

/*!
 * Removes a process from its assigned job.
 *
 * @param[in] Process
 *     A pointer to the process being removed from its assigned job.
 *
 * @remarks
 *     This function is called from PspDeleteProcess() during process object
 *     deletion. The process must still be linked to its assigned job.
 */
VOID
NTAPI
PspRemoveProcessFromJob(
    _In_ PEPROCESS Process
)
{
    PEJOB Job;
    BOOLEAN ActiveProcessZero;

    Job = Process->Job;
    ASSERT(Job != NULL);

    ExEnterCriticalRegionAndAcquireResourceExclusive(&Job->JobLock);

    ASSERT(Process->Job == Job);
    ASSERT(Process->JobLinks.Flink != NULL);
    ASSERT(Process->JobLinks.Blink != NULL);
    ASSERT(!IsListEmpty(&Process->JobLinks));

    /* Remove the process from the job's process list */
    RemoveEntryList(&Process->JobLinks);
    InitializeListHead(&Process->JobLinks);

    /* Decrement the job's active process count if it is still active */
    ActiveProcessZero = PspDeactivateProcessFromJobLocked(Job, Process);

    /* TODO: Ensure that job limits are respected */

    /* If no active processes remain, notify the job completion port */
    if (ActiveProcessZero && Job->CompletionPort)
    {
        (VOID)PspSendJobMessageLocked(Job,
                                      JOB_OBJECT_MSG_ACTIVE_PROCESS_ZERO,
                                      NULL,
                                      FALSE);
    }

    ExReleaseResourceAndLeaveCriticalRegion(&Job->JobLock);
}

/*!
 * Handles the exit of a process from the specified job object.
 *
 * @param[in] Process
 *     A pointer to the process that is exiting the job.
 *
 * @remark
 *     This function is called from PspExitThread() when the last thread exits.
 *     The process must be assigned to a job.
 */
VOID
NTAPI
PspExitProcessFromJob(
    _In_ PEPROCESS Process
)
{
    PEJOB Job;
    BOOLEAN ActiveProcessZero;

    Job = Process->Job;
    ASSERT(Job != NULL);

    ExEnterCriticalRegionAndAcquireResourceExclusive(&Job->JobLock);

    /* Job membership is immutable in the current implementation */
    ASSERT(Process->Job == Job);

    /* Decrement the job's active process count if the process is still active */
    ActiveProcessZero = PspDeactivateProcessFromJobLocked(Job, Process);

    /* If no active processes remain, notify the job completion port */
    if (ActiveProcessZero && Job->CompletionPort)
    {
        (VOID)PspSendJobMessageLocked(Job,
                                      JOB_OBJECT_MSG_ACTIVE_PROCESS_ZERO,
                                      NULL,
                                      FALSE);
    }

    /* TODO: Ensure that job limits are respected */

    ExReleaseResourceAndLeaveCriticalRegion(&Job->JobLock);
}

/*!
 * Callback function to terminate a process and update the job's
 * active process counter.
 *
 * @param[in] Process
 *     A pointer to the process object to be terminated.
 *
 * @param[in] Context
 *     A pointer to a PSP_TERMINATE_PROCESS_CONTEXT structure.
 *
 * @returns
 *     STATUS_SUCCESS.
 *
 * @remark
 *     The callback is invoked _without_ the job lock held. Process carries the
 *     reference acquired by the job enumerator for the duration of the call.
 */
static
NTSTATUS
NTAPI
PspTerminateProcessCallback(
    _In_ PEPROCESS Process,
    _In_ PVOID Context
)
{
    NTSTATUS Status;
    BOOLEAN ActiveProcessZero;
    PPSP_TERMINATE_PROCESS_CONTEXT TerminateContext = (PPSP_TERMINATE_PROCESS_CONTEXT)Context;
    PEJOB Job = TerminateContext->Job;
    NTSTATUS ExitStatus = TerminateContext->ExitStatus;

    ASSERT(Job != NULL);
    ASSERT(Process->Job == Job);

    /* Avoid entering process termination when the process has already
       completed its active job transition */
    ExEnterCriticalRegionAndAcquireResourceExclusive(&Job->JobLock);

    if (FlagOn(Process->JobStatus, PSP_JOB_NOT_REALLY_ACTIVE))
    {
        goto Exit;
    }

    /* Terminate the process */
    Status = PsTerminateProcess(Process, ExitStatus);

    /* PsTerminateProcess can return STATUS_NOTHING_TO_TERMINATE
       when it finds no threads (the ordinary process exit remains
       responsible for completing job accounting in that case),
       that should be treated as a no-op for job traversal */
    if (!NT_SUCCESS(Status))
    {
        goto Exit;
    }

    /* Decrement the job's active process count if the process is still active */
    ActiveProcessZero = PspDeactivateProcessFromJobLocked(Job, Process);

    /* If there are no active processes left in the job, notify anyone waiting
       for the job object by signaling completion */
    if (ActiveProcessZero)
    {
        /* It is intended that the event is set to a signaled
           state only in the termination path */
        KeSetEvent(&Job->Event, IO_NO_INCREMENT, FALSE);

        if (Job->CompletionPort)
        {
            (VOID)PspSendJobMessageLocked(Job,
                                          JOB_OBJECT_MSG_ACTIVE_PROCESS_ZERO,
                                          NULL,
                                          FALSE);
        }
    }

Exit:

    ExReleaseResourceAndLeaveCriticalRegion(&Job->JobLock);

    return STATUS_SUCCESS;
}

/*!
 * Terminates all processes currently associated with the specified job object.
 *
 * @param[in] Job
 *     A pointer to the job object to be terminated.
 *
 * @param[in] ExitStatus
 *     The exit status to be used for all terminated processes.
 *
 * @returns
 *     STATUS_SUCCESS if the job object was successfully terminated.
 *     Otherwise, an appropriate NTSTATUS error code.
 */
NTSTATUS
NTAPI
PspTerminateJobObject(
    _In_ PEJOB Job,
    _In_ NTSTATUS ExitStatus
)
{
    NTSTATUS Status;
    LONG PreviousFlags;
    PSP_TERMINATE_PROCESS_CONTEXT Context;

    PreviousFlags = InterlockedOr((PLONG)&Job->JobFlags, PSP_JOB_TERMINATING);

    /* Termination is idempotent, another caller already owns the traversal */
    if (PreviousFlags & PSP_JOB_TERMINATING)
    {
        return STATUS_SUCCESS;
    }

    Context.Job = Job;
    Context.ExitStatus = ExitStatus;

    Status = PspEnumerateProcessesInJob(Job,
                                        PspTerminateProcessCallback,
                                        &Context);

    /* The termination callback always returns STATUS_SUCCESS because
       per-process termination failures are handled locally */
    ASSERT(NT_SUCCESS(Status));

    InterlockedAnd((PLONG)&Job->JobFlags, ~PSP_JOB_TERMINATING);

    return Status;
}

/*!
 * Handles the closing of a job object.
 *
 * @param[in] Process
 *     Unused.
 *
 * @param[in] ObjectBody
 *     A pointer to the job object being closed.
 *
 * @param[in] GrantedAccess
 *     Unused.
 *
 * @param[in] ProcessHandleCount
 *     Unused.
 *
 * @param[in] SystemHandleCount
 *     The number of system handles currently open for the job object.
 *
 * @remark
 *     This function terminates the job if the
 *     JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE flag is set.
 */
VOID
NTAPI
PspCloseJob(
    _In_ PEPROCESS Process,
    _In_ PVOID ObjectBody,
    _In_ ACCESS_MASK GrantedAccess,
    _In_ ULONG_PTR ProcessHandleCount,
    _In_ ULONG_PTR SystemHandleCount
)
{
    NTSTATUS Status;
    PEJOB Job = (PEJOB)ObjectBody;
    PVOID CompletionPort = NULL;

    PAGED_CODE();

    UNREFERENCED_PARAMETER(Process);
    UNREFERENCED_PARAMETER(GrantedAccess);
    UNREFERENCED_PARAMETER(ProcessHandleCount);

    /* Proceed only when the last handle is left */
    if (SystemHandleCount > 1)
    {
        DPRINT1("PspJobClose called with unexpected SystemHandleCount: %lu\n",
                SystemHandleCount);
        return;
    }

    /* Flag the job as closed */
    InterlockedOr((PLONG)&Job->JobFlags, PSP_JOB_CLOSE_DONE);

    ExEnterCriticalRegionAndAcquireResourceExclusive(&Job->JobLock);

    /* If the job is set to kill on close, terminate all associated processes */
    if (FlagOn(Job->LimitFlags, JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE))
    {
        /* Keep the completion port associated during termination so that
           final job messages can still be delivered */
        ExReleaseResourceAndLeaveCriticalRegion(&Job->JobLock);

        Status = PspTerminateJobObject(Job, STATUS_SUCCESS);
        ASSERT(NT_SUCCESS(Status));

        ExEnterCriticalRegionAndAcquireResourceExclusive(&Job->JobLock);
    }

    CompletionPort = Job->CompletionPort;
    Job->CompletionPort = NULL;
    Job->CompletionKey = NULL;

    ExReleaseResourceAndLeaveCriticalRegion(&Job->JobLock);

    if (CompletionPort)
    {
        ObDereferenceObject(CompletionPort);
    }
}

/*!
 * Deletes the specified job object and cleans up associated resources.
 *
 * @param[in] ObjectBody
 *     A pointer to the job object to be deleted.
 */
VOID
NTAPI
PspDeleteJob(_In_ PVOID ObjectBody)
{
    PEJOB Job = (PEJOB)ObjectBody;

    PAGED_CODE();

    /* Let win32k tear down any per-job state it keeps for UI restrictions */
    if (Job->UIRestrictionsClass != 0)
    {
        (VOID)PspInvokeW32JobCallout(Job, PsW32JobCalloutTerminate, NULL);
        Job->UIRestrictionsClass = 0;
    }

    Job->LimitFlags = 0;

    /* Remove the reference to the completion port if associated */
    if (Job->CompletionPort)
    {
        ObDereferenceObject(Job->CompletionPort);
        Job->CompletionPort = NULL;
    }

    /* TODO: Ensure that job sets are respected */

    /* Unlink the job object under lock */
    ExAcquireFastMutex(&PsJobListLock);

    ASSERT(!IsListEmpty(&Job->JobLinks));
    RemoveEntryList(&Job->JobLinks);

    ExReleaseFastMutex(&PsJobListLock);

    /* TODO: Clean up security information */

    /* Delete the resource associated with the job object */
    ExDeleteResource(&Job->JobLock);
}

/*!
 * Helper function to set limit information for a job object,
 * either basic or extended limits.
 *
 * @param[in] Job
 *     The job object being modified.
 *
 * @param[in] ExtendedLimit
 *     A pointer to the structure containing the limit information to be set.
 *
 * @param[in] IsExtendedLimit
 *     A boolean value indicating whether the limit information is extended.
 *
 * @returns
 *     STATUS_SUCCESS if the job limits are successfully set.
 *     Otherwise, an appropriate NTSTATUS error code.
 */
static
NTSTATUS
PspSetJobLimitsBasicOrExtended(
    _In_ PEJOB Job,
    _In_ PJOBOBJECT_EXTENDED_LIMIT_INFORMATION ExtendedLimit,
    _In_ BOOLEAN IsExtendedLimit
)
{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG AllowedFlags;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();

    ASSERT(KeAreAllApcsDisabled());

    AllowedFlags = IsExtendedLimit
                       ? PSP_JOB_EXTENDED_LIMIT_VALID_FLAGS
                       : PSP_JOB_BASIC_LIMIT_VALID_FLAGS;

    /* Validate flags */
    if (ExtendedLimit->BasicLimitInformation.LimitFlags & ~AllowedFlags)
    {
        DPRINT1("Invalid LimitFlags specified: 0x%08X\n",
                ExtendedLimit->BasicLimitInformation.LimitFlags & ~AllowedFlags);
        return STATUS_INVALID_PARAMETER;
    }

    if (ExtendedLimit->BasicLimitInformation.LimitFlags & JOB_OBJECT_LIMIT_PRESERVE_JOB_TIME &&
        ExtendedLimit->BasicLimitInformation.LimitFlags & JOB_OBJECT_LIMIT_JOB_TIME)
    {
        DPRINT1("Invalid LimitFlags combination specified "
                "(PRESERVE_JOB_TIME and JOB_TIME are mutually exclusive)\n");
        return STATUS_INVALID_PARAMETER;
    }

    /* Acquire the job lock */
    ExAcquireResourceExclusiveLite(&Job->JobLock, TRUE);

    /*
     * Basic Limits
     */

    if (ExtendedLimit->BasicLimitInformation.LimitFlags & JOB_OBJECT_LIMIT_WORKINGSET)
    {
        /* https://learn.microsoft.com/en-us/windows/win32/api/winnt/ns-winnt-jobobject_basic_limit_information:
           "If MaximumWorkingSetSize is nonzero, MinimumWorkingSetSize cannot be zero"
           "If MinimumWorkingSetSize is nonzero, MaximumWorkingSetSize cannot be zero"
           Also check that the minimum doesn't exceed the maximum or both aren't equal to zero. */
        if ((ExtendedLimit->BasicLimitInformation.MaximumWorkingSetSize > 0 &&
                ExtendedLimit->BasicLimitInformation.MinimumWorkingSetSize <= 0)
            ||
            (ExtendedLimit->BasicLimitInformation.MinimumWorkingSetSize > 0 &&
                ExtendedLimit->BasicLimitInformation.MaximumWorkingSetSize <= 0)
            ||
            ExtendedLimit->BasicLimitInformation.MaximumWorkingSetSize <
            ExtendedLimit->BasicLimitInformation.MinimumWorkingSetSize)
        {
            Status = STATUS_INVALID_PARAMETER;
            goto ExitFromBasicLimits;
        }

        Job->MinimumWorkingSetSize = ExtendedLimit->BasicLimitInformation.MinimumWorkingSetSize;
        Job->MaximumWorkingSetSize = ExtendedLimit->BasicLimitInformation.MaximumWorkingSetSize;
    }

    if (ExtendedLimit->BasicLimitInformation.LimitFlags & JOB_OBJECT_LIMIT_PROCESS_TIME)
    {
        Job->PerProcessUserTimeLimit.QuadPart =
            ExtendedLimit->BasicLimitInformation.PerProcessUserTimeLimit.QuadPart;
    }

    if (ExtendedLimit->BasicLimitInformation.LimitFlags & JOB_OBJECT_LIMIT_JOB_TIME)
    {
        Job->PerJobUserTimeLimit.QuadPart =
            ExtendedLimit->BasicLimitInformation.PerJobUserTimeLimit.QuadPart;
    }

    if (ExtendedLimit->BasicLimitInformation.LimitFlags & JOB_OBJECT_LIMIT_ACTIVE_PROCESS)
    {
        Job->ActiveProcessLimit = ExtendedLimit->BasicLimitInformation.ActiveProcessLimit;
    }

    if (ExtendedLimit->BasicLimitInformation.LimitFlags & JOB_OBJECT_LIMIT_AFFINITY)
    {
        /* https://learn.microsoft.com/en-us/windows/win32/api/winnt/ns-winnt-jobobject_basic_limit_information:
           "The affinity must be a subset of the system affinity mask obtained
           by calling the GetProcessAffinityMask function"
           The lpSystemAffinityMask obtained with GetProcessAffinityMask() corresponds
           to ActiveProcessorsAffinityMask, which in turn corresponds to KeActiveProcessors */
        if (ExtendedLimit->BasicLimitInformation.Affinity !=
            (ExtendedLimit->BasicLimitInformation.Affinity & KeActiveProcessors))
        {
            Status = STATUS_INVALID_PARAMETER;
            goto ExitFromBasicLimits;
        }

        Job->Affinity = ExtendedLimit->BasicLimitInformation.Affinity;
    }

    if (ExtendedLimit->BasicLimitInformation.LimitFlags & JOB_OBJECT_LIMIT_PRIORITY_CLASS)
    {
        if (ExtendedLimit->BasicLimitInformation.PriorityClass > PROCESS_PRIORITY_CLASS_ABOVE_NORMAL ||
            ExtendedLimit->BasicLimitInformation.PriorityClass <= PROCESS_PRIORITY_CLASS_INVALID)
        {
            Status = STATUS_INVALID_PARAMETER;
            goto ExitFromBasicLimits;
        }

        /* https://learn.microsoft.com/en-us/windows/win32/api/winnt/ns-winnt-jobobject_basic_limit_information:
           "The calling process must enable the SE_INC_BASE_PRIORITY_NAME
           privilege" */
        if (SeCheckPrivilegedObject(SeIncreaseBasePriorityPrivilege,
                                    Job,
                                    JOB_OBJECT_SET_ATTRIBUTES,
                                    PreviousMode))
        {
            Job->PriorityClass = ExtendedLimit->BasicLimitInformation.PriorityClass;
        }
        else
        {
            Status = STATUS_PRIVILEGE_NOT_HELD;
            goto ExitFromBasicLimits;
        }
    }

    if (ExtendedLimit->BasicLimitInformation.LimitFlags & JOB_OBJECT_LIMIT_SCHEDULING_CLASS)
    {
        if (ExtendedLimit->BasicLimitInformation.SchedulingClass >= PSP_JOB_SCHEDULING_CLASSES)
        {
            Status = STATUS_INVALID_PARAMETER;
            goto ExitFromBasicLimits;
        }

        /* https://learn.microsoft.com/en-us/windows/win32/api/winnt/ns-winnt-jobobject_basic_limit_information:
           "To use a scheduling class greater than 5, the calling process must
           enable the SE_INC_BASE_PRIORITY_NAME privilege" */
        if (ExtendedLimit->BasicLimitInformation.SchedulingClass > PSP_JOB_SCHEDULING_CLASS_DEFAULT)
        {
            if (SeCheckPrivilegedObject(SeIncreaseBasePriorityPrivilege,
                                        Job,
                                        JOB_OBJECT_SET_ATTRIBUTES,
                                        PreviousMode))
            {
                Job->SchedulingClass = ExtendedLimit->BasicLimitInformation.SchedulingClass;
            }
            else
            {
                Status = STATUS_PRIVILEGE_NOT_HELD;
                goto ExitFromBasicLimits;
            }
        }
        else
        {
            Job->SchedulingClass = ExtendedLimit->BasicLimitInformation.SchedulingClass;
        }
    }

    /*
     * Extended Memory Limits
     */

    /* Acquire the memory limits lock */
    KeAcquireGuardedMutexUnsafe(&Job->MemoryLimitsLock);

    if (ExtendedLimit->BasicLimitInformation.LimitFlags & JOB_OBJECT_LIMIT_PROCESS_MEMORY)
    {
        Job->ProcessMemoryLimit = ExtendedLimit->ProcessMemoryLimit >> PAGE_SHIFT;
    }

    if (ExtendedLimit->BasicLimitInformation.LimitFlags & JOB_OBJECT_LIMIT_JOB_MEMORY)
    {
        Job->JobMemoryLimit = ExtendedLimit->JobMemoryLimit >> PAGE_SHIFT;
    }

    /* Update the job's limit flags with the new ones. This includes dealing
       with those extended limits that only set some flag */
    Job->LimitFlags = ExtendedLimit->BasicLimitInformation.LimitFlags;


    /* Release locks */

    KeReleaseGuardedMutexUnsafe(&Job->MemoryLimitsLock);

ExitFromBasicLimits:

    ExReleaseResourceLite(&Job->JobLock);

    return Status;
}

/*!
 * Queues an initial new-process notification for a process already assigned
 * to a job.
 *
 * @param[in] Process
 *     A borrowed pointer to the process being notified.
 *
 * @param[in] Context
 *     A pointer to the job associated with the completion port.
 *
 * @return
 *     STATUS_SUCCESS.
 *
 * @remarks
 *     The callback is invoked while the job lock is held exclusively.
 *     Notification failures are recorded for diagnostic purposes and do not
 *     undo the completion port association.
 */
static
NTSTATUS
NTAPI
PspAssociateCompletionPortCallback(
    _In_ PEPROCESS Process,
    _In_ PVOID Context)
{
    PEJOB Job;

    Job = (PEJOB)Context;

    ASSERT(Process->Job == Job);
    ASSERT(Job->CompletionPort != NULL);

    ASSERT(ExIsResourceAcquiredExclusiveLite(&Job->JobLock) != 0);

    /* Ensure the process is active and has a valid unique process ID */
    if (!FlagOn(Process->JobStatus, PSP_JOB_NOT_REALLY_ACTIVE) &&
        Process->UniqueProcessId)
    {
        (VOID)PspSendJobMessageLocked(Job,
                                      JOB_OBJECT_MSG_NEW_PROCESS,
                                      Process->UniqueProcessId,
                                      FALSE);
    }

    return STATUS_SUCCESS;
}

/*!
 * Associates an I/O completion port with a job object and enumerates its
 * processes to propagate the association.
 *
 * @param[in] Job
 *     A pointer to the job object to which the I/O completion port will be
 *     associated.
 *
 * @param[in] AssociateCpInfo
 *     A structure containing information used to associate a completion port
 *     with a job (the handle of the I/O completion port and the key).
 *
 * @return
 *     STATUS_SUCCESS if the completion port was associated with the job.
 *     Otherwise, an appropriate NTSTATUS error code.
 *
 * @remarks
 *     Once the completion port is installed, failure to queue an initial
 *     process notification is recorded diagnostically and does not undo the
 *     association.
 */
static
NTSTATUS
PspAssociateCompletionPortWithJob(
    _In_ PEJOB Job,
    _In_ PJOBOBJECT_ASSOCIATE_COMPLETION_PORT AssociateCpInfo
)
{
    NTSTATUS Status;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    HANDLE IoCompletion;

    ASSERT(KeAreAllApcsDisabled());

    if (!AssociateCpInfo->CompletionPort)
    {
        return STATUS_INVALID_PARAMETER;
    }

    Status = ObReferenceObjectByHandle(AssociateCpInfo->CompletionPort,
                                       IO_COMPLETION_MODIFY_STATE,
                                       IoCompletionType,
                                       PreviousMode,
                                       &IoCompletion,
                                       NULL);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    ExAcquireResourceExclusiveLite(&Job->JobLock, TRUE);

    /* Check if the job already has a completion port or is in a final state */
    if (Job->CompletionPort || FlagOn(Job->JobFlags, PSP_JOB_CLOSE_DONE))
    {
        ExReleaseResourceLite(&Job->JobLock);
        ObDereferenceObject(IoCompletion);
        return STATUS_INVALID_PARAMETER;
    }

    Job->CompletionKey = AssociateCpInfo->CompletionKey;
    Job->CompletionPort = IoCompletion;

    /* Inform all processes in the job about the association
       N.B. Assignment is serialized by JobLock; a process is therefore covered
       either by this enumeration or by the normal assignment path */
    Status = PspEnumerateProcessesInJobLocked(Job,
                                              PspAssociateCompletionPortCallback,
                                              Job);

    ASSERT(NT_SUCCESS(Status));

    ExReleaseResourceLite(&Job->JobLock);

    /* The completion port association is committed at this point. Initial process
       notifications are best-effort and a failure to queue one must not turn
       a successful association into a failure. */
    return STATUS_SUCCESS;
}

/*!
 * Collects accounting information, such as total user time, kernel time, page
 * fault counts, and I/O operations, for the given job object.
 *
 * @param[in] Job
 *     Pointer to the job object whose accounting information is being queried.
 *
 * @param[out] BasicAndIo
 *     Pointer to a structure that will be filled with basic accounting and
 *     I/O information about the job.
 *
 * @return
 *     STATUS_SUCCESS on success.
 *     Otherwise, an appropriate NTSTATUS error code.
 */
static
NTSTATUS
PspQueryJobBasicAccountingInfo(
    _In_ PEJOB Job,
    _Out_ PJOBOBJECT_BASIC_AND_IO_ACCOUNTING_INFORMATION BasicAndIo
)
{
    PLIST_ENTRY NextEntry;
    PROCESS_VALUES Values;

    /* Zero the basic accounting information */
    RtlZeroMemory(&BasicAndIo->BasicInfo, sizeof(BasicAndIo->BasicInfo));

    /* Lock the job object */
    ExEnterCriticalRegionAndAcquireResourceShared(&Job->JobLock);

    /* Initialize with job's accumulated accounting data */
    BasicAndIo->BasicInfo.TotalUserTime.QuadPart = Job->TotalUserTime.QuadPart;
    BasicAndIo->BasicInfo.TotalKernelTime.QuadPart = Job->TotalKernelTime.QuadPart;
    BasicAndIo->BasicInfo.ThisPeriodTotalUserTime.QuadPart = Job->ThisPeriodTotalUserTime.QuadPart;
    BasicAndIo->BasicInfo.ThisPeriodTotalKernelTime.QuadPart = Job->ThisPeriodTotalKernelTime.QuadPart;
    BasicAndIo->BasicInfo.TotalPageFaultCount = Job->TotalPageFaultCount;
    BasicAndIo->BasicInfo.TotalProcesses = Job->TotalProcesses;
    BasicAndIo->BasicInfo.ActiveProcesses = Job->ActiveProcesses;
    BasicAndIo->BasicInfo.TotalTerminatedProcesses = Job->TotalTerminatedProcesses;

    /* Set I/O info (even though it may not be returned in some cases) */
    BasicAndIo->IoInfo.ReadOperationCount = Job->ReadOperationCount;
    BasicAndIo->IoInfo.WriteOperationCount = Job->WriteOperationCount;
    BasicAndIo->IoInfo.OtherOperationCount = Job->OtherOperationCount;
    BasicAndIo->IoInfo.ReadTransferCount = Job->ReadTransferCount;
    BasicAndIo->IoInfo.WriteTransferCount = Job->WriteTransferCount;
    BasicAndIo->IoInfo.OtherTransferCount = Job->OtherTransferCount;

    /* Sum up accounting data for each active process in the job */
    for (NextEntry = Job->ProcessListHead.Flink;
         NextEntry != &Job->ProcessListHead;
         NextEntry = NextEntry->Flink)
    {
        PEPROCESS Process = CONTAINING_RECORD(NextEntry, EPROCESS, JobLinks);

        /* Skip folded accounting processes */
        if (!FlagOn(Process->JobStatus, PSP_JOB_ACCOUNTING_FOLDED))
        {
            KeQueryValuesProcess(&Process->Pcb, &Values);

            /* Accumulate user and kernel times, and I/O counts */
            BasicAndIo->BasicInfo.TotalUserTime.QuadPart += Values.TotalUserTime.QuadPart;
            BasicAndIo->BasicInfo.TotalKernelTime.QuadPart += Values.TotalKernelTime.QuadPart;
            BasicAndIo->IoInfo.ReadOperationCount += Values.IoInfo.ReadOperationCount;
            BasicAndIo->IoInfo.WriteOperationCount += Values.IoInfo.WriteOperationCount;
            BasicAndIo->IoInfo.OtherOperationCount += Values.IoInfo.OtherOperationCount;
            BasicAndIo->IoInfo.ReadTransferCount += Values.IoInfo.ReadTransferCount;
            BasicAndIo->IoInfo.WriteTransferCount += Values.IoInfo.WriteTransferCount;
            BasicAndIo->IoInfo.OtherTransferCount += Values.IoInfo.OtherTransferCount;
        }
    }

    /* Release the job lock */
    ExReleaseResourceAndLeaveCriticalRegion(&Job->JobLock);

    return STATUS_SUCCESS;
}

/*!
 * Retrieves basic or extended limit information for a job object.
 *
 * @param[in] Job
 *     Pointer to the job object whose limit information is being queried.
 *
 * @param[in] Extended
 *     A boolean value indicating whether extended limit information is being
 *     requested or only basic limit information.
 *
 * @param[out] ExtendedLimit
 *     Pointer to a structure that will be filled with basic or extended limit
 *     information about the job.
 *
 * @return
 *     STATUS_SUCCESS on success.
 *     Otherwise, an appropriate NTSTATUS error code.
 */
static
NTSTATUS
PspQueryJobLimitInformation(
    _In_ PEJOB Job,
    _In_ BOOLEAN Extended,
    _Out_ PJOBOBJECT_EXTENDED_LIMIT_INFORMATION ExtendedLimit
)
{
    PKTHREAD CurrentThread = KeGetCurrentThread();

    /* Lock the job object */
    KeEnterGuardedRegionThread(CurrentThread);
    ExAcquireResourceSharedLite(&Job->JobLock, TRUE);

    /* Copy basic limit information */
    ExtendedLimit->BasicLimitInformation.LimitFlags = Job->LimitFlags;
    ExtendedLimit->BasicLimitInformation.MinimumWorkingSetSize = Job->MinimumWorkingSetSize;
    ExtendedLimit->BasicLimitInformation.MaximumWorkingSetSize = Job->MaximumWorkingSetSize;
    ExtendedLimit->BasicLimitInformation.ActiveProcessLimit = Job->ActiveProcessLimit;
    ExtendedLimit->BasicLimitInformation.PriorityClass = Job->PriorityClass;
    ExtendedLimit->BasicLimitInformation.SchedulingClass = Job->SchedulingClass;
    ExtendedLimit->BasicLimitInformation.Affinity = Job->Affinity;
    ExtendedLimit->BasicLimitInformation.PerProcessUserTimeLimit.QuadPart = Job->PerProcessUserTimeLimit.QuadPart;
    ExtendedLimit->BasicLimitInformation.PerJobUserTimeLimit.QuadPart = Job->PerJobUserTimeLimit.QuadPart;

    /* If extended limits are requested, include memory limits */
    if (Extended)
    {
        KeAcquireGuardedMutexUnsafe(&Job->MemoryLimitsLock);

        ExtendedLimit->ProcessMemoryLimit = Job->ProcessMemoryLimit << PAGE_SHIFT;
        ExtendedLimit->JobMemoryLimit = Job->JobMemoryLimit << PAGE_SHIFT;
        ExtendedLimit->PeakProcessMemoryUsed = Job->PeakProcessMemoryUsed << PAGE_SHIFT;
        ExtendedLimit->PeakJobMemoryUsed = Job->PeakJobMemoryUsed << PAGE_SHIFT;

        KeReleaseGuardedMutexUnsafe(&Job->MemoryLimitsLock);

        /* Zero out IoInfo to avoid kernel memory leaks */
        RtlZeroMemory(&ExtendedLimit->IoInfo, sizeof(ExtendedLimit->IoInfo));
    }

    /* Release the job lock */
    ExReleaseResourceLite(&Job->JobLock);
    KeLeaveGuardedRegionThread(CurrentThread);

    return STATUS_SUCCESS;
}

/*!
 * Callback function used to collect the process IDs for all active processes
 * in a job object.
 *
 * @param[in] Process
 *     A pointer to the process whose ID is being added to the process list.
 *
 * @param[in, out] Context
 *     A pointer to the context structure that tracks the process ID collection.
 *     This context holds the list of process IDs, the length of the buffer,
 *     and the status of the collection operation.
 *
 * @return
 *     STATUS_SUCCESS on successful collection of the process ID.
 *     Otherwise, an appropriate NTSTATUS error code if the buffer is
 *     insufficient or another error occurs.
 */
static
NTSTATUS
NTAPI
PspQueryJobProcessIdListCallback(
    _In_ PEPROCESS Process,
    _Inout_ PVOID Context
)
{
    PPSP_QUERY_JOB_PROCESS_ID_CONTEXT QueryContext = (PPSP_QUERY_JOB_PROCESS_ID_CONTEXT)Context;

    /* Skip processes that are not really active */
    if (FlagOn(Process->JobStatus, PSP_JOB_NOT_REALLY_ACTIVE))
    {
        /* Continue to the next process */
        return STATUS_SUCCESS;
    }

    /* An active process may be linked before its process identifier has been
       assigned - such a process is not representable in this information class */
    if (Process->UniqueProcessId == NULL)
    {
        ASSERT(QueryContext->ProcessIdList->NumberOfAssignedProcesses != 0);

        QueryContext->ProcessIdList->NumberOfAssignedProcesses--;

        return STATUS_SUCCESS;
    }

    if (QueryContext->RemainingLength < sizeof(ULONG_PTR))
    {
        return STATUS_BUFFER_OVERFLOW;
    }

    *QueryContext->NextProcessId++ = (ULONG_PTR)Process->UniqueProcessId;

    QueryContext->RemainingLength -= sizeof(ULONG_PTR);

    QueryContext->ProcessIdList->NumberOfProcessIdsInList++;

    return STATUS_SUCCESS;
}

/*!
 * Collects a list of process IDs for all processes associated with a job.
 *
 * @param[in] Job
 *     Pointer to the job object whose process IDs are being queried.
 *
 * @param[out] ProcIdList
 *     Pointer to a structure that will be filled with the list of process IDs
 *     and information about the number of assigned and returned process IDs.
 *
 * @param[in] JobInformationLength
 *     Specifies the size, in bytes, of the buffer that will receive the process
 *     ID list.
 *
 * @param[out] ReturnRequiredLength
 *     Pointer to a variable that receives the size, in bytes, of the
 *     information written to the buffer. If there is a buffer overflow, the
 *     required size is returned.
 *
 * @return
 *     STATUS_SUCCESS on success.
 *     STATUS_BUFFER_OVERFLOW if the buffer is too small to hold all IDs.
 *     Otherwise, an appropriate NTSTATUS error code.
 */
static
NTSTATUS
PspQueryJobProcessIdList(
    _In_ PEJOB Job,
    _Out_writes_bytes_(JobInformationLength) PJOBOBJECT_BASIC_PROCESS_ID_LIST ProcIdList,
    _In_ ULONG JobInformationLength,
    _Out_ PULONG ReturnRequiredLength
)
{
    NTSTATUS Status;
    PSP_QUERY_JOB_PROCESS_ID_CONTEXT QueryContext;

    /* Check if the buffer provided is large enough to hold at least the
       fixed portion of JOBOBJECT_BASIC_PROCESS_ID_LIST */
    if (JobInformationLength < sizeof(JOBOBJECT_BASIC_PROCESS_ID_LIST))
    {
        return STATUS_INFO_LENGTH_MISMATCH;
    }

    QueryContext.ProcessIdList = ProcIdList;
    QueryContext.NextProcessId = &ProcIdList->ProcessIdList[0];
    QueryContext.RemainingLength = JobInformationLength - FIELD_OFFSET(JOBOBJECT_BASIC_PROCESS_ID_LIST,
                                                                       ProcessIdList);

    Status = STATUS_SUCCESS;

    ExEnterCriticalRegionAndAcquireResourceShared(&Job->JobLock);

    _SEH2_TRY
    {
        ProcIdList->NumberOfAssignedProcesses = Job->ActiveProcesses;
        ProcIdList->NumberOfProcessIdsInList = 0;

        Status = PspEnumerateProcessesInJobLocked(Job,
                                                  PspQueryJobProcessIdListCallback,
                                                  &QueryContext);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END;

    ExReleaseResourceAndLeaveCriticalRegion(&Job->JobLock);

    if (NT_SUCCESS(Status) || Status == STATUS_BUFFER_OVERFLOW)
    {
        /* Report the bytes actually written */
        *ReturnRequiredLength = (ULONG)(JobInformationLength - QueryContext.RemainingLength);
    }
    else
    {
        *ReturnRequiredLength = 0;
    }

    return Status;
}

/*
 * @implemented
 */
PVOID
NTAPI
PsGetJobLock(PEJOB Job)
{
    ASSERT(Job);
    return (PVOID)&Job->JobLock;
}

/*
 * @implemented
 */
ULONG
NTAPI
PsGetJobSessionId(PEJOB Job)
{
    ASSERT(Job);
    return Job->SessionId;
}

/*
 * @implemented
 */
ULONG
NTAPI
PsGetJobUIRestrictionsClass(PEJOB Job)
{
    ASSERT(Job);
    return Job->UIRestrictionsClass;
}

/*
 * @implemented
 */
VOID
NTAPI
PsSetJobUIRestrictionsClass(
    PEJOB Job,
    ULONG UIRestrictionsClass
)
{
    ASSERT(Job);
    (void)InterlockedExchangeUL(&Job->UIRestrictionsClass, UIRestrictionsClass);
}

/*!
 * Invokes the win32k job callout, if win32k has registered one.
 *
 * @param[in] Job
 *     A pointer to the job object the callout applies to.
 *
 * @param[in] CalloutType
 *     The operation win32k is being asked to perform.
 *
 * @param[in, optional] Data
 *     Class specific data. For PsW32JobCalloutSetInformation this is the new
 *     UI restrictions class, for PsW32JobCalloutAddProcess the W32PROCESS of
 *     the process being added to the job.
 *
 * @returns
 *     The status returned by win32k, or STATUS_SUCCESS when no callout has
 *     been registered (i.e. the win32 subsystem is not loaded yet).
 *
 * @remarks
 *     FIXME: TODO: We do not attach to the session of the job, as win32k is only ever
 *     loaded in one session.
 */
NTSTATUS
NTAPI
PspInvokeW32JobCallout(
    _In_ PEJOB Job,
    _In_ PSW32JOBCALLOUTTYPE CalloutType,
    _In_opt_ PVOID Data
)
{
    WIN32_JOBCALLOUT_PARAMETERS Parameters;

    /* Nothing to do if win32k has not registered a callout */
    if (PspW32JobCallout == NULL)
    {
        return STATUS_SUCCESS;
    }

    Parameters.Job = Job;
    Parameters.CalloutType = CalloutType;
    Parameters.Data = Data;

    return PspW32JobCallout(&Parameters);
}

/*!
 * Applies a new basic UI restrictions class to a job object.
 *
 * @param[in] Job
 *     A pointer to the job object being modified.
 *
 * @param[in] UIRestrictionsClass
 *     The new set of JOB_OBJECT_UILIMIT_* flags.
 *
 * @returns
 *     STATUS_SUCCESS if the restrictions were applied.
 *     STATUS_INVALID_PARAMETER if unknown restriction flags were given.
 *     An appropriate NTSTATUS error code otherwise.
 *
 * @remarks
 *     The restrictions are only stored once win32k has accepted them, as it
 *     may fail to allocate the per-job state it needs to enforce them.
 */
static
NTSTATUS
PspSetJobUIRestrictions(
    _In_ PEJOB Job,
    _In_ ULONG UIRestrictionsClass
)
{
    NTSTATUS Status = STATUS_SUCCESS;

    /* Reject restrictions we do not know about */
    if (UIRestrictionsClass & ~JOB_OBJECT_UILIMIT_ALL)
    {
        return STATUS_INVALID_PARAMETER;
    }

    ExEnterCriticalRegionAndAcquireResourceExclusive(&Job->JobLock);

    /* Only bother win32k if something actually changes */
    if (Job->UIRestrictionsClass != UIRestrictionsClass)
    {
        Status = PspInvokeW32JobCallout(Job,
                                        PsW32JobCalloutSetInformation,
                                        UlongToPtr(UIRestrictionsClass));
        if (NT_SUCCESS(Status))
        {
            Job->UIRestrictionsClass = UIRestrictionsClass;
        }
    }

    ExReleaseResourceAndLeaveCriticalRegion(&Job->JobLock);

    return Status;
}

/*!
 * Creates a job object.
 *
 * @param[out] JobHandle
 *     A pointer to a handle that will receive the handle of the created job object.
 *
 * @param[in] DesiredAccess
 *     Specifies the desired access rights for the job object.
 *
 * @param[in, optional] ObjectAttributes
 *     An optional pointer to an object attributes block
 *
 * @returns
 *     STATUS_SUCCESS if the job object is successfully created.
 *     An appropriate NTSTATUS error code otherwise.
 */
NTSTATUS
NTAPI
NtCreateJobObject(
    _Out_ PHANDLE JobHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ POBJECT_ATTRIBUTES ObjectAttributes
)
{
    HANDLE Handle;
    PEJOB Job;
    KPROCESSOR_MODE PreviousMode;
    PEPROCESS CurrentProcess;
    NTSTATUS Status;

    PAGED_CODE();

    PreviousMode = ExGetPreviousMode();
    CurrentProcess = PsGetCurrentProcess();

    /* Check for valid buffers */
    if (PreviousMode != KernelMode)
    {
        _SEH2_TRY
        {
            ProbeForWriteHandle(JobHandle);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            return _SEH2_GetExceptionCode();
        }
        _SEH2_END;
    }

    /* Create the job object */
    Status = ObCreateObject(PreviousMode,
                            PsJobType,
                            ObjectAttributes,
                            PreviousMode,
                            NULL,
                            sizeof(EJOB),
                            0,
                            0,
                            (PVOID *)&Job);

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to create job object, Status 0x%08lx\n", Status);
        return Status;
    }

    /* Initialize the job object */

    RtlZeroMemory(Job, sizeof(*Job));

    InitializeListHead(&Job->JobSetLinks);
    InitializeListHead(&Job->ProcessListHead);

    /* Make sure that early destruction doesn't attempt to remove
       the object from the list before it even gets added */
    InitializeListHead(&Job->JobLinks);

    /* Inherit the session ID from the caller */
    Job->SessionId = PsGetProcessSessionId(CurrentProcess);

    /* Initialize the job limits lock */
    KeInitializeGuardedMutex(&Job->MemoryLimitsLock);

    /* Initialize the job lock */
    (VOID)ExInitializeResource(&Job->JobLock);

    /* Initialize the event object within the job */
    KeInitializeEvent(&Job->Event, NotificationEvent, FALSE);

    /* Set the scheduling class */
    Job->SchedulingClass = PSP_JOB_SCHEDULING_CLASS_DEFAULT;

    /* Link the object into the global job list */
    ExAcquireFastMutex(&PsJobListLock);
    InsertTailList(&PsJobListHead, &Job->JobLinks);
    ExReleaseFastMutex(&PsJobListLock);

    /* Insert the job object into the object table  */
    Status = ObInsertObject(Job,
                            NULL,
                            DesiredAccess,
                            0,
                            NULL,
                            &Handle);

    if (NT_SUCCESS(Status))
    {
        /* Pass the handle back to the caller */
        _SEH2_TRY
        {
            *JobHandle = Handle;
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            Status = _SEH2_GetExceptionCode();
        }
        _SEH2_END;
    }

    return Status;
}

NTSTATUS
NTAPI
NtCreateJobSet(IN ULONG NumJob,
               IN PJOB_SET_ARRAY UserJobSet,
               IN ULONG Flags)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/*!
 * Opens a handle to an existing job object.
 *
 * @param JobHandle
 *     A pointer to a handle that will receive the handle of the created job
 *     object.
 *
 * @param DesiredAccess
 *     Specifies the desired access rights for the job object.
 *
 * @param ObjectAttributes
 *     Pointer to the OBJECT_ATTRIBUTES structure specifying the object name and
 *     attributes.
 *
 * @returns
 *     STATUS_SUCCESS if the job object is successfully created.
 *     An appropriate NTSTATUS error code otherwise.
 */
NTSTATUS
NTAPI
NtOpenJobObject(
    _Out_ PHANDLE JobHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ POBJECT_ATTRIBUTES ObjectAttributes
)
{
    KPROCESSOR_MODE PreviousMode;
    HANDLE Handle;
    NTSTATUS Status;

    PAGED_CODE();

    PreviousMode = ExGetPreviousMode();

    /* Check for valid buffers */
    if (PreviousMode != KernelMode)
    {
        _SEH2_TRY
        {
            ProbeForWriteHandle(JobHandle);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            _SEH2_YIELD(return _SEH2_GetExceptionCode());
        }
        _SEH2_END;
    }

    Status = ObOpenObjectByName(ObjectAttributes,
                                PsJobType,
                                PreviousMode,
                                NULL,
                                DesiredAccess,
                                NULL,
                                &Handle);
    if (NT_SUCCESS(Status))
    {
        _SEH2_TRY
        {
            *JobHandle = Handle;
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            Status = _SEH2_GetExceptionCode();
        }
        _SEH2_END;
    }

    return Status;
}

/*!
 * Assigns a process to a job object.
 *
 * @param[in] JobHandle
 *     Handle to the job object.
 *
 * @param[in] ProcessHandle
 *     Handle to the process to be assigned.
 *
 * @returns
 *     STATUS_SUCCESS if the process is successfully assigned to the job.
 *     An appropriate NTSTATUS error code otherwise.
 */
NTSTATUS
NTAPI
NtAssignProcessToJobObject(
    _In_ HANDLE JobHandle,
    _In_ HANDLE ProcessHandle
)
{
    PEPROCESS Process;
    PEJOB Job;
    KPROCESSOR_MODE PreviousMode;
    ULONG SessionId;
    NTSTATUS Status;

    PAGED_CODE();

    PreviousMode = ExGetPreviousMode();

    /* Reference the job. JOB_OBJECT_ASSIGN_PROCESS rights are required
       for assignment */
    Status = ObReferenceObjectByHandle(JobHandle,
                                       JOB_OBJECT_ASSIGN_PROCESS,
                                       PsJobType,
                                       PreviousMode,
                                       (PVOID *)&Job,
                                       NULL);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    /* Reference the process. The handle must have the PROCESS_SET_QUOTA and 
       PROCESS_TERMINATE access rights. */
    Status = ObReferenceObjectByHandle(ProcessHandle,
                                       PROCESS_SET_QUOTA | PROCESS_TERMINATE,
                                       PsProcessType,
                                       PreviousMode,
                                       (PVOID *)&Process,
                                       NULL);
    if (!NT_SUCCESS(Status))
    {
        ObDereferenceObject(Job);
        return Status;
    }

    /* Get the session ID - it must match the process and the job creator */
    SessionId = PsGetProcessSessionId(Process);

    if (Process->Job != NULL || SessionId != Job->SessionId)
    {
        /* Return STATUS_ACCESS_DENIED if the process is already assigned
           to a job or the session ID is different */
        ObDereferenceObject(Job);
        ObDereferenceObject(Process);
        return STATUS_ACCESS_DENIED;
    }

    Status = PspAssignProcessToJob(Process, Job);

    if (Status == STATUS_QUOTA_EXCEEDED)
    {
        /* Preserve the assignment failure */
        (VOID)PsTerminateProcess(Process, STATUS_QUOTA_EXCEEDED);
    }

    ObDereferenceObject(Job);
    ObDereferenceObject(Process);

    return Status;
}

/*!
 * Determines if a specified process is associated with a specified or any job.
 *
 * @param[in] ProcessHandle
 *     A handle to the process being queried.
 *
 * @param[in, optional] JobHandle
 *     An optional handle to the job object being compared. If NULL,
 *     the function checks if the process is associated with any job.
 *
 * @returns
 *     STATUS_PROCESS_IN_JOB if the process is in the job or any job (when
 *     JobHandle is NULL).
 *     STATUS_PROCESS_NOT_IN_JOB if the process is not in the job or any job.
 *     Otherwise, an appropriate NTSTATUS error code.
 */
NTSTATUS
NTAPI
NtIsProcessInJob(
    _In_ HANDLE ProcessHandle,
    _In_opt_ HANDLE JobHandle
)
{
    KPROCESSOR_MODE PreviousMode;
    PEPROCESS Process;
    PEJOB ProcessJob;
    PEJOB JobObjectFromHandle;
    NTSTATUS Status;

    PreviousMode = ExGetPreviousMode();

    PAGED_CODE();

    /* Check if the process handle is the current process */
    if (ProcessHandle == NtCurrentProcess())
    {
        /* If so, directly use the current process object */
        Process = PsGetCurrentProcess();
    }
    else
    {
        /* Reference the process object by handle */
        Status = ObReferenceObjectByHandle(ProcessHandle,
                                           PROCESS_QUERY_INFORMATION,
                                           PsProcessType,
                                           PreviousMode,
                                           (PVOID *)&Process,
                                           NULL);
        if (!NT_SUCCESS(Status))
        {
            return Status;
        }
    }

    /* Get the job object associated with the process */
    ProcessJob = Process->Job;

    if (ProcessJob != NULL)
    {
        /* If no specific job handle is provided,
           the process is assigned to a job */
        if (JobHandle == NULL)
        {
            Status = STATUS_PROCESS_IN_JOB;
        }
        else
        {
            /* Get the job object from the provided job handle
               and compare it with the process job */
            Status = ObReferenceObjectByHandle(JobHandle,
                                               JOB_OBJECT_QUERY,
                                               PsJobType,
                                               PreviousMode,
                                               (PVOID *)&JobObjectFromHandle,
                                               NULL);
            if (NT_SUCCESS(Status))
            {
                /* Compare the job objects */
                if ((ProcessJob == JobObjectFromHandle))
                {
                    Status = STATUS_PROCESS_IN_JOB;
                }
                else
                {
                    Status = STATUS_PROCESS_NOT_IN_JOB;
                }

                /* Dereference the job object handle */
                ObDereferenceObject(JobObjectFromHandle);
            }
        }
    }
    else
    {
        /* The process is not assigned to any job */
        Status = STATUS_PROCESS_NOT_IN_JOB;
    }

    /* Dereference the process object if it was referenced */
    if (ProcessHandle != NtCurrentProcess())
    {
        ObDereferenceObject(Process);
    }

    return Status;
}

/*!
 * Terminates all processes currently associated with the specified job object.
 *
 * @param[in] JobHandle
 *     A handle to the job object to be terminated.
 *
 * @param[in] ExitStatus
 *     The exit status to be used for all terminated processes.
 *
 * @returns
 *     STATUS_SUCCESS if the job object was successfully terminated.
 *     Otherwise, an appropriate NTSTATUS error code.
 */
NTSTATUS
NTAPI
NtTerminateJobObject(
    _In_ HANDLE JobHandle,
    _In_ NTSTATUS ExitStatus
)
{
    KPROCESSOR_MODE PreviousMode;
    PEJOB Job;
    NTSTATUS Status;

    PAGED_CODE();

    PreviousMode = ExGetPreviousMode();

    Status = ObReferenceObjectByHandle(JobHandle,
                                       JOB_OBJECT_TERMINATE,
                                       PsJobType,
                                       PreviousMode,
                                       (PVOID *)&Job,
                                       NULL);
    if (NT_SUCCESS(Status))
    {
        Status = PspTerminateJobObject(Job, ExitStatus);
        ObDereferenceObject(Job);
    }

    return Status;
}

/*!
 * Retrieves information about a job object based on the requested information
 * class.
 *
 * @param[in, optional] JobHandle
 *     Handle to the job object for which information is being queried. The
 *     handle must have the JOB_OBJECT_QUERY access right. If NULL, the current
 *     process' job is used.
 *
 * @param[in] JobInformationClass
 *     Specifies the type of information to query.
 *
 * @param[out] JobInformation
 *     Pointer to a buffer that receives the requested job object information.
 *
 * @param[in] JobInformationLength
 *     Specifies the size, in bytes, of the JobInformation buffer.
 *
 * @param[out, optional] ReturnLength
 *     Pointer to a variable that receives the size, in bytes, of the
 *     information written to the JobInformation buffer. Specify NULL to not
 *     receive this information.
 *
 * @return
 *     STATUS_SUCCESS on success.
 *     Otherwise, an appropriate NTSTATUS error code.
 *
 * @remarks
 *     Not fully implemented. The function currently does not support all
 *     information classes.
 */
NTSTATUS
NTAPI
NtQueryInformationJobObject(
    _In_opt_ HANDLE JobHandle,
    _In_ JOBOBJECTINFOCLASS JobInformationClass,
    _Out_writes_bytes_(JobInformationLength) PVOID JobInformation,
    _In_ ULONG JobInformationLength,
    _Out_opt_ PULONG ReturnLength
)
{
    PEJOB Job;
    NTSTATUS Status;
    BOOLEAN NoOutput;
    PVOID JobInfoBuffer;
    PKTHREAD CurrentThread;
    KPROCESSOR_MODE PreviousMode;
    JOBOBJECT_EXTENDED_LIMIT_INFORMATION ExtendedLimit;
    JOBOBJECT_BASIC_AND_IO_ACCOUNTING_INFORMATION BasicAndIo;
    JOBOBJECT_BASIC_UI_RESTRICTIONS UiRestrictions;
    ULONG RequiredLength, RequiredAlign, ReturnRequiredLength;

    PAGED_CODE();

    CurrentThread  = KeGetCurrentThread();

    /* Validate that JobInformationClass is in the expected range */
    if (JobInformationClass > JobObjectJobSetInformation ||
        JobInformationClass < JobObjectBasicAccountingInformation)
    {
        return STATUS_INVALID_INFO_CLASS;
    }

    /* Determine the required length and alignment for the class */
    RequiredLength = PspJobInfoLengths[JobInformationClass];
    RequiredAlign = PspJobInfoAlign[JobInformationClass];
    ReturnRequiredLength = RequiredLength;

    /* If length mismatch (needed versus provided) */
    if (JobInformationLength != RequiredLength)
    {
        /* This can only be accepted if the class is variable length
           (JobObjectBasicProcessIdList or JobObjectSecurityLimitInformation) or
           if size is bigger than needed */
        if ((JobInformationClass != JobObjectBasicProcessIdList &&
                JobInformationClass != JobObjectSecurityLimitInformation) ||
            JobInformationLength < RequiredLength)
        {
            return STATUS_INFO_LENGTH_MISMATCH;
        }

        /* Set what we need to copy out */
        RequiredLength = JobInformationLength;
    }

    PreviousMode = ExGetPreviousMode();

    /* If the request is coming from user mode, probe the user buffer */
    if (PreviousMode != KernelMode)
    {
        ASSERT(RequiredAlign == 1 ||
               RequiredAlign == 2 ||
               RequiredAlign == 4 ||
               RequiredAlign == 8 ||
               RequiredAlign == 16);

        _SEH2_TRY
        {
            /* Probe the buffer */
            if (JobInformation != NULL)
            {
                ProbeForWrite(JobInformation, JobInformationLength, RequiredAlign);
            }

            /* Probe the return length if required */
            if (ReturnLength != NULL)
            {
                ProbeForWriteUlong(ReturnLength);
            }
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            _SEH2_YIELD(return _SEH2_GetExceptionCode());
        }
        _SEH2_END;
    }

    /* If a job handle was provided, use it */
    if (JobHandle != NULL)
    {
        Status = ObReferenceObjectByHandle(JobHandle,
                                           JOB_OBJECT_QUERY,
                                           PsJobType,
                                           PreviousMode,
                                           (PVOID *)&Job,
                                           NULL);
        if (!NT_SUCCESS(Status))
        {
            return Status;
        }
    }
    /* Otherwise, get our current process' job, if any */
    else
    {
        PEPROCESS CurrentProcess;

        CurrentProcess = (PEPROCESS)CurrentThread->ApcState.Process;
        Job = CurrentProcess->Job;
        if (Job == NULL)
        {
            return STATUS_ACCESS_DENIED;
        }

        ObReferenceObject(Job);
    }

    /* By default, assume we'll have to copy data */
    NoOutput = FALSE;

    switch (JobInformationClass)
    {
    case JobObjectBasicAccountingInformation:
    case JobObjectBasicAndIoAccountingInformation:
    {
        Status = PspQueryJobBasicAccountingInfo(Job, &BasicAndIo);
        JobInfoBuffer = &BasicAndIo;
        break;
    }
    case JobObjectBasicLimitInformation:
    case JobObjectExtendedLimitInformation:
    {
        Status = PspQueryJobLimitInformation(Job,
                                             JobInformationClass == JobObjectExtendedLimitInformation,
                                             &ExtendedLimit);
        JobInfoBuffer = &ExtendedLimit;
        break;
    }
    case JobObjectBasicProcessIdList:
    {
        Status = PspQueryJobProcessIdList(Job,
                                          (PJOBOBJECT_BASIC_PROCESS_ID_LIST)JobInformation,
                                          JobInformationLength,
                                          &ReturnRequiredLength);

        /* No need to copy data as it's directly filled in the helper */
        NoOutput = TRUE;
        break;
    }
    case JobObjectBasicUIRestrictions:
    {
        UiRestrictions.UIRestrictionsClass = Job->UIRestrictionsClass;
        JobInfoBuffer = &UiRestrictions;
        break;
    }
    case JobObjectSecurityLimitInformation:
    case JobObjectEndOfJobTimeInformation:
    case JobObjectAssociateCompletionPortInformation:
    case JobObjectJobSetInformation:
        DPRINT1("Class %d not implemented\n", JobInformationClass);
        Status = STATUS_NOT_IMPLEMENTED;
        break;
    case MaxJobObjectInfoClass:
    default:
        DPRINT1("Invalid class %d\n", JobInformationClass);
        Status = STATUS_NOT_IMPLEMENTED;
        break;
    }

    /* Job is no longer required */
    ObDereferenceObject(Job);

    /* If we succeeded, copy back data */
    if (NT_SUCCESS(Status))
    {
        _SEH2_TRY
        {
            /* If we have anything to copy, do it */
            if (!NoOutput)
            {
                RtlCopyMemory(JobInformation, JobInfoBuffer, RequiredLength);
            }

            /* And return length if asked */
            if (ReturnLength != NULL)
            {
                *ReturnLength = ReturnRequiredLength;
            }
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            _SEH2_YIELD(return _SEH2_GetExceptionCode());
        }
        _SEH2_END;
    }

    return Status;
}

/*!
 * Sets the information for a job object, updating the specified job object
 * limits.
 *
 * This function is called to modify various job object limits, including basic
 * limits, extended limits, security limits, and other job-specific settings.
 *
 * @param[in] JobHandle
 *     A handle to the job object that is being modified. The handle must have
 *     the JOB_OBJECT_SET_ATTRIBUTES access right.
 *
 * @param[in] JobInformationClass
 *     The class of information to set. This determines the structure and
 *     content of the JobInformation parameter.
 *
 * @param[in] JobInformation
 *     A pointer to a buffer that contains the information to be set. The type
 *     and content of the buffer depend on the value of JobInformationClass.
 *
 * @param[in] JobInformationLength
 *     The size of the buffer pointed to by JobInformation, in bytes.
 *
 * @return
 *     STATUS_SUCCESS on success.
 *     Otherwise, an appropriate NTSTATUS error code.
 *
 * @remarks
 *     Not fully implemented. The function currently does not support all
 *     information classes.
 */
NTSTATUS
NTAPI
NtSetInformationJobObject(
    _In_ HANDLE JobHandle,
    _In_ JOBOBJECTINFOCLASS JobInformationClass,
    _In_reads_bytes_(JobInformationLength) PVOID JobInformation,
    _In_ ULONG JobInformationLength
)
{
    PEJOB Job;
    NTSTATUS Status;
    PKTHREAD CurrentThread;
    ACCESS_MASK DesiredAccess;
    KPROCESSOR_MODE PreviousMode;
    ULONG RequiredLength, RequiredAlign;

    PAGED_CODE();

    CurrentThread = KeGetCurrentThread();

    /* Validate that JobInformationClass is in the expected range */
    if (JobInformationClass > JobObjectJobSetInformation ||
        JobInformationClass < JobObjectBasicAccountingInformation)
    {
        return STATUS_INVALID_INFO_CLASS;
    }

    /* Determine the required length and alignment for the class */
    RequiredLength = PspJobInfoLengths[JobInformationClass];
    RequiredAlign = PspJobInfoAlign[JobInformationClass];

    PreviousMode = ExGetPreviousMode();

    /* If the request is coming from user mode, probe the user buffer */
    if (PreviousMode != KernelMode)
    {
        ASSERT(RequiredAlign == 1 ||
               RequiredAlign == 2 ||
               RequiredAlign == 4 ||
               RequiredAlign == 8 ||
               RequiredAlign == 16);

        _SEH2_TRY
        {
            /* Probe out buffer for read */
            if (JobInformationLength != 0)
            {
                ProbeForRead(JobInformation, JobInformationLength, RequiredAlign);
            }
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            _SEH2_YIELD(return _SEH2_GetExceptionCode());
        }
        _SEH2_END;
    }

    /* Validate that the provided buffer length matches the expected size
       for the class */
    if (JobInformationLength != RequiredLength)
    {
        return STATUS_INFO_LENGTH_MISMATCH;
    }

    DesiredAccess = JOB_OBJECT_SET_ATTRIBUTES;

    /* If setting security limits, additional security access rights
       are required */
    if (JobInformationClass == JobObjectSecurityLimitInformation)
    {
        DesiredAccess |= JOB_OBJECT_SET_SECURITY_ATTRIBUTES;
    }

    Status = ObReferenceObjectByHandle(JobHandle,
                                       DesiredAccess,
                                       PsJobType,
                                       PreviousMode,
                                       (PVOID *)&Job,
                                       NULL);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    /* And set the information.
     *
     * N.B. Disable APC delivery for the set operation. The handlers below may
     * acquire locks using unsafe variants which expect the caller to have
     * already established this state.
     */

    KeEnterGuardedRegionThread(CurrentThread);

    switch (JobInformationClass)
    {
    case JobObjectBasicLimitInformation:
    case JobObjectExtendedLimitInformation:
    {
        JOBOBJECT_EXTENDED_LIMIT_INFORMATION ExtendedLimit;
        BOOLEAN IsExtendedLimit = JobInformationClass == JobObjectExtendedLimitInformation;

        _SEH2_TRY
        {
            /* If asking for extending limits */
            if (IsExtendedLimit)
            {
                ExtendedLimit = *(PJOBOBJECT_EXTENDED_LIMIT_INFORMATION)JobInformation;
            }
            else
            {
                RtlZeroMemory(&ExtendedLimit, sizeof(ExtendedLimit));
                ExtendedLimit.BasicLimitInformation =
                    *(PJOBOBJECT_BASIC_LIMIT_INFORMATION)JobInformation;
            }
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            Status = _SEH2_GetExceptionCode();
            goto Exit;
        }
        _SEH2_END;

        Status = PspSetJobLimitsBasicOrExtended(Job,
                                                &ExtendedLimit,
                                                IsExtendedLimit);
        goto Exit;
    }
    case JobObjectAssociateCompletionPortInformation:
    {
        JOBOBJECT_ASSOCIATE_COMPLETION_PORT AssociateCpInfo;

        _SEH2_TRY
        {
            RtlCopyMemory(&AssociateCpInfo,
                          JobInformation,
                          sizeof(AssociateCpInfo));
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            Status = _SEH2_GetExceptionCode();
            break;
        }
        _SEH2_END;

        Status = PspAssociateCompletionPortWithJob(Job, &AssociateCpInfo);
        break;
    }
    case JobObjectBasicUIRestrictions:
    {
        JOBOBJECT_BASIC_UI_RESTRICTIONS UiRestrictions;

        _SEH2_TRY
        {
            UiRestrictions = *(PJOBOBJECT_BASIC_UI_RESTRICTIONS)JobInformation;
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            Status = _SEH2_GetExceptionCode();
            goto Exit;
        }
        _SEH2_END;

        Status = PspSetJobUIRestrictions(Job, UiRestrictions.UIRestrictionsClass);
        break;
    }
    case JobObjectBasicAccountingInformation:
    case JobObjectBasicAndIoAccountingInformation:
    case JobObjectBasicProcessIdList:
    case JobObjectEndOfJobTimeInformation:
    case JobObjectJobSetInformation:
    case JobObjectSecurityLimitInformation:
        DPRINT1("Class %d not implemented\n", JobInformationClass);
        Status = STATUS_NOT_IMPLEMENTED;
        break;
    case MaxJobObjectInfoClass:
    default:
        DPRINT1("Invalid class %d\n", JobInformationClass);
        Status = STATUS_INVALID_PARAMETER;
        break;
    }

Exit:
    KeLeaveGuardedRegionThread(CurrentThread);

    ObDereferenceObject(Job);

    return Status;
}

/* EOF */
