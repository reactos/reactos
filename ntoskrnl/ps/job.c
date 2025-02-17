/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ps/job.c
 * PURPOSE:         Core functions for managing Job Objects, a kernel mechanism
 *                  for managing multiple processes as a single unit
 * PROGRAMMERS:     2004-2012 Alex Ionescu (alex@relsoft.net) (stubs)
 *                  2004-2005 Thomas Weidenmueller <w3seek@reactos.com>
 *                  2015-2016 Samuel Serapión Vega (encoded@reactos.org)
 *                  2017 Mark Jansen (mark.jansen@reactos.org)
 *                  2018 Pierre Schweitzer (pierre@reactos.org)
 *                  2022 Timo Kreuzer (timo.kreuzer@reactos.org)
 *                  2024 Gleb Surikov (glebs.surikovs@gmail.com)
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
    sizeof(ULONG),
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
typedef struct TERMINATE_PROCESS_CONTEXT
{
    PEJOB Job;
    NTSTATUS ExitStatus;
} TERMINATE_PROCESS_CONTEXT, *PTERMINATE_PROCESS_CONTEXT;

/*!
 * Context structure used to collect process IDs for a job object.
 *
 * @param[in, out] ProcIdList
 *     A pointer to the structure that holds the process IDs and the count of
 *     assigned processes.
 *
 * @param[in, out] ListLength
 *     The remaining length of the process ID list buffer, adjusted as process
 *     IDs are added.
 *
 * @param[in, out] IdListArray
 *     A pointer to the position in the process ID array where the next process
 *     ID will be added.
 *
 * @param[in, out] Status
 *     Holds the status of the process ID collection operation.
 */
typedef struct QUERY_JOB_PROCESS_ID_CONTEXT
{
    PJOBOBJECT_BASIC_PROCESS_ID_LIST ProcIdList;
    ULONG ListLength;
    ULONG_PTR *IdListArray;
    NTSTATUS Status;
} QUERY_JOB_PROCESS_ID_CONTEXT, *PQUERY_JOB_PROCESS_ID_CONTEXT;

/*!
 * Context structure used to associate a job object with a completion port.
 *
 * @param[in] CompletionPort
 *     A pointer to the I/O completion port.
 *
 * @param[in] CompletionKey
 *     A pointer to the key associated with the completion port.
 */
typedef struct ASSOCIATE_COMPLETION_PORT_CONTEXT
{
    PVOID CompletionPort;
    PVOID CompletionKey;
} ASSOCIATE_COMPLETION_PORT_CONTEXT, *PASSOCIATE_COMPLETION_PORT_CONTEXT;

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
 * Advances the job enumerator to the next process in the job's process list.
 *
 * @param Job
 *     Pointer to the job object containing the process list.
 *
 * @param Process
 *     Pointer to the current process obtained from a previous call to
 *     PspAdvanceJobEnumerator.
 *
 * @return
 *     Pointer to the next valid process, or NULL if no more processes are
 *     available.
 */
static
PEPROCESS
PspAdvanceJobEnumerator(
    _In_ PEJOB Job,
    _In_opt_ PEPROCESS Process
)
{
    PLIST_ENTRY Entry;
    PEPROCESS Next;

    ExEnterCriticalRegionAndAcquireResourceExclusive(&Job->JobLock);

    /* If Process is NULL, the enumeration starts from the first process.
       Otherwise, continue from the next process */
    if (Process)
    {
        Entry = Process->JobLinks.Flink;
    }
    else
    {
        Entry = Job->ProcessListHead.Flink;
    }

    /* Iterate through the job's process list */
    while (Entry != &Job->ProcessListHead)
    {
        Next = CONTAINING_RECORD(Entry, EPROCESS, JobLinks);

        /* We use the safe variant because it returns FALSE if
           the object is being deleted */
        if (ObReferenceObjectSafe(Next))
        {
            goto Found;
        }

        /* Move to the next entry in the lsit */
        Entry = Entry->Flink;
    }

    /* Reached the end */
    Next = NULL;

Found:

    ExReleaseResourceAndLeaveCriticalRegion(&Job->JobLock);

    if (Process)
    {
        ObDereferenceObject(Process);
    }

    return Next;
}

/*!
 * Enumerates all processes currently associated with the specified job object
 * and calls the provided callback function for each process.
 *
 * @param[in] Job
 *     A pointer to the job object whose processes are to be enumerated.
 *
 * @param[in] Callback
 *     A pointer to the PJOB_ENUMERATOR_CALLBACK callback function to be
 *     called for each process.
 *
 * @param[in, optional] Context
 *     An optional pointer to a context to be passed to the callback function.
 *
 * @param[in] BreakOnCallbackFailure
 *     A boolean that, if TRUE, indicates that enumeration should stop early if
 *     the callback function returns an error. If FALSE, the enumeration
 *     continues even if the callback function fails.
 *
 * @returns
 *     STATUS_SUCCESS if the enumeration completed successfully.
 *     An appropriate NTSTATUS error code otherwise.
 *
 * @remarks
 *     If BreakOnCallbackFailure is TRUE and not all callbacks returned success,
 *     the function may still return STATUS_SUCCESS.
 */
NTSTATUS
NTAPI
PspEnumerateProcessesInJob(
    _In_ PEJOB Job,
    _In_ PJOB_ENUMERATOR_CALLBACK Callback,
    _In_opt_ PVOID Context,
    _In_ BOOLEAN BreakOnCallbackFailure
)
{
    NTSTATUS Status = STATUS_SUCCESS;
    BOOLEAN AnyCallbackFailed = FALSE;
    PEPROCESS Process;

    /* Get the first process from the job */
    Process = PspAdvanceJobEnumerator(Job, NULL);

    /* Iterate through all processes in the job */
    while (Process)
    {
        /* Call the provided callback */
        Status = Callback(Process, Context);
        if (!NT_SUCCESS(Status))
        {
            AnyCallbackFailed = TRUE;
            if (BreakOnCallbackFailure)
            {
                break;
            }
        }

        /* Move to the next process */
        Process = PspAdvanceJobEnumerator(Job, Process);
    }

    if (NT_SUCCESS(Status) && AnyCallbackFailed)
    {
        DPRINT1("PspEnumerateProcessesInJob(Job: %p, Callback: %p, Context: %p,"
                " BreakOnCallbackFailure: %u) - Partial success report, not all"
                " callbacks returned success\n",
                Job, Callback, Context, BreakOnCallbackFailure);
    }

    return Status;
}

/*!
 * Assigns a process to a job object.
 *
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

    DPRINT1("PspAssignProcessToJob(Process: %p, Job: %p)\n", Process, Job);

    ExEnterCriticalRegionAndAcquireResourceExclusive(&Job->JobLock);

    /* Check if the job has a limit on the number of active processes */
    if (Job->LimitFlags & JOB_OBJECT_LIMIT_ACTIVE_PROCESS)
    {
        /* Check if job limit on active processes has been reached */
        if (Job->ActiveProcesses > Job->ActiveProcessLimit)
        {
            if (Job->CompletionPort)
            {
                /* If the job has a completion port, notify the job that the
                   limit on the number of active processes has been exceeded */
                Status = IoSetIoCompletion(Job->CompletionPort,
                                           Job->CompletionKey,
                                           NULL,
                                           STATUS_SUCCESS,
                                           JOB_OBJECT_MSG_ACTIVE_PROCESS_LIMIT,
                                           TRUE);
            }
            else
            {
                Status = STATUS_QUOTA_EXCEEDED;
            }

            return Status;
        }
    }

    /* https://learn.microsoft.com/en-us/windows/win32/api/jobapi2/nf-jobapi2-assignprocesstojobobject:
       "If the job or any of its parent jobs in the job chain is terminating
       when AssignProcessToJob is called, the function fails" */
    if (Job->JobFlags & JOB_OBJECT_TERMINATING)
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Prevent processes from being added to the job if it is flagged for
       closing and has a limit on process termination on closing */
    if (Job->LimitFlags & JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE &&
        Job->JobFlags & JOB_OBJECT_CLOSE_DONE)
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Assign process to job object by inserting into the job's process list */
    InsertTailList(&Job->ProcessListHead, &Process->JobLinks);

    /* Increment the job's process counters */
    Job->TotalProcesses++;
    Job->ActiveProcesses++;

    if (Job->CompletionPort && Process->UniqueProcessId)
    {
        /* If the job has a completion port and the process has a unique ID,
           notify the job of the new process */
        Status = IoSetIoCompletion(Job->CompletionPort,
                                   Job->CompletionKey,
                                   Process->UniqueProcessId,
                                   STATUS_SUCCESS,
                                   JOB_OBJECT_MSG_NEW_PROCESS,
                                   FALSE);
    }

    ExReleaseResourceAndLeaveCriticalRegion(&Job->JobLock);

    /* TODO: Ensure that job limits are respected */

    return Status;
}

/*!
 * Removes a process from the specified job object.
 *
 * @param[in] Process
 *     A pointer to the process to be removed from the job.
 *
 * @param[in] Job
 *     A pointer to the job object from which the process is to be removed.
 *
 * @remark This function is called from PspDeleteProcess() as the process
 *         is destroyed.
 */
VOID
NTAPI
PspRemoveProcessFromJob(
    _In_ PEPROCESS Process,
    _In_ PEJOB Job
)
{
    DPRINT1("PspRemoveProcessFromJob(Process: %p, Job: %p)\n", Process, Job);

    ExEnterCriticalRegionAndAcquireResourceExclusive(&Job->JobLock);

    /* Remove the process from the job's process list */
    RemoveEntryList(&Process->JobLinks);

    /* Decrement the job's active process count if the process is still active*/
    if (!(Process->JobStatus & JOB_NOT_REALLY_ACTIVE))
    {
        /* Assert that the job's active process count does not underflow */
        ASSERT((Job->ActiveProcesses - 1) < Job->ActiveProcesses);

        Job->ActiveProcesses--;

        /* Flag this process as inactive to prevent the number of active
           processes from repeatedly decrementing */
        InterlockedOr((PLONG)&Process->JobStatus, JOB_NOT_REALLY_ACTIVE);
    }

    /* TODO: Ensure that job limits are respected */

    ExReleaseResourceAndLeaveCriticalRegion(&Job->JobLock);
}

/*!
 * Handles the exit of a process from the specified job object.
 *
 * @param[in] Job
 *     A pointer to the job object from which the process is exiting.
 *
 * @param[in] Process
 *     A pointer to the process that is exiting the job.
 *
 * @remark This function is called from PspExitThread() as the last thread
 *         exits.
 */
VOID
NTAPI
PspExitProcessFromJob(
    _In_ PEJOB Job,
    _In_ PEPROCESS Process
)
{
    DPRINT1("PspExitProcessFromJob(Job: %p, Process: %p)\n", Job, Process);

    /* Make sure we are not interrupted */
    ExEnterCriticalRegionAndAcquireResourceExclusive(&Job->JobLock);

    /* Check if the process is part of the specified job */
    if (Process->Job == Job)
    {
        /* Decrement the job's active process count if the process is still
           active */
        if (!(Process->JobStatus & JOB_NOT_REALLY_ACTIVE))
        {
            /* Assert that the job's active process count does not underflow */
            ASSERT((Job->ActiveProcesses - 1) < Job->ActiveProcesses);

            Job->ActiveProcesses--;

            /* Flag this process as inactive to prevent the number of active
               processes from repeatedly decrementing */
            InterlockedOr((PLONG)&Process->JobStatus, JOB_NOT_REALLY_ACTIVE);
        }

        /* If no active processes remain, notify the job completion port */
        if (Job->ActiveProcesses == 0 && Job->CompletionPort)
        {
            IoSetIoCompletion(Job->CompletionPort,
                              Job->CompletionKey,
                              NULL,
                              STATUS_SUCCESS,
                              JOB_OBJECT_MSG_ACTIVE_PROCESS_ZERO,
                              FALSE);
        }
    }

    /* TODO: Ensure that job limits are respected */

    /* Resume APCs and release lock */
    ExReleaseResourceAndLeaveCriticalRegion(&Job->JobLock);
}

/*!
 * Callback function to terminate a process and update the job's
 * active process counter.
 *
 * @param[in] Process
 *     A pointer to the process object to be terminated.
 *
 * @param[in, optional] Context
 *     An optional pointer to a context, in this case, a structure containing
 *     the job object and the exit status.
 *
 * @returns
 *     STATUS_SUCCESS if the process was successfully terminated.
 *     Otherwise, an appropriate NTSTATUS error code.
 *
 * @remark
 *     When this callback function is executed, the job lock is held by
 *     PspEnumerateProcessesInJob(). It releases the lock after the callback
 *     returns.
 */
static
NTSTATUS
PspTerminateProcessCallback(
    _In_ PEPROCESS Process,
    _In_opt_ PVOID Context
)
{
    NTSTATUS Status;
    PTERMINATE_PROCESS_CONTEXT TerminateContext = (PTERMINATE_PROCESS_CONTEXT)Context;
    PEJOB Job = TerminateContext->Job;
    NTSTATUS ExitStatus = TerminateContext->ExitStatus;

    /* If the process is already inactive, no need to terminate */
    if (Process->JobStatus & JOB_NOT_REALLY_ACTIVE)
    {
        return STATUS_INVALID_PARAMETER;
    }

    ExEnterCriticalRegionAndAcquireResourceExclusive(&Job->JobLock);

    /* Flag the job as terminating */
    InterlockedOr((PLONG)&Job->JobFlags, JOB_OBJECT_TERMINATING);

    /* Terminate the process */
    Status = PsTerminateProcess(Process, ExitStatus);

    if (NT_SUCCESS(Status))
    {
        /* Decrement the job's active process count, but only if the process is
           still active */
        if (!(Process->JobStatus & JOB_NOT_REALLY_ACTIVE))
        {
            Job->ActiveProcesses--;

            /* Flag this process as inactive to prevent the number of active
               processes from repeatedly decrementing */
            InterlockedOr((PLONG)&Process->JobStatus,
                          JOB_NOT_REALLY_ACTIVE);

            /* Check if there are no active processes left in the job */
            if (Job->ActiveProcesses == 0)
            {
                /* If so, notify anyone waiting for the job object by signaling
                   completion */
                KeSetEvent(&Job->Event, IO_NO_INCREMENT, FALSE);
            }
        }
    }

    /* Clear the terminating flag */
    InterlockedAnd((PLONG)&Job->JobFlags, ~JOB_OBJECT_TERMINATING);

    ExReleaseResourceAndLeaveCriticalRegion(&Job->JobLock);

    return Status;
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
    TERMINATE_PROCESS_CONTEXT Context;
    Context.Job = Job;
    Context.ExitStatus = ExitStatus;

    DPRINT1("PspTerminateJobObject(Job: %p, ExitStatus: %x)\n",
            Job,
            ExitStatus);

    return PspEnumerateProcessesInJob(Job,
                                      PspTerminateProcessCallback,
                                      &Context,
                                      FALSE);
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
 * @param[in] HandleCount
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
    _In_ ULONG HandleCount,
    _In_ ULONG SystemHandleCount
)
{
    PEJOB Job = (PEJOB)ObjectBody;

    PAGED_CODE();

    UNREFERENCED_PARAMETER(Process);
    UNREFERENCED_PARAMETER(GrantedAccess);
    UNREFERENCED_PARAMETER(HandleCount);

    DPRINT1("PspCloseJob(Process: %p, ObjectBody: %p, GrantedAccess: %x, "
            "HandleCount: %u, SystemHandleCount: %u)\n",
            Process,
            ObjectBody,
            GrantedAccess,
            HandleCount,
            SystemHandleCount);

    /* Proceed only when the last handle is left */
    if (SystemHandleCount != 1)
    {
        return;
    }

    /* Flag the job as closed */
    InterlockedOr((PLONG)&Job->JobFlags, JOB_OBJECT_CLOSE_DONE);

    /* If the job is set to kill on close, terminate all associated processes */
    if (Job->LimitFlags & JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE)
    {
        NTSTATUS Status = PspTerminateJobObject(Job, STATUS_SUCCESS);
        ASSERT(NT_SUCCESS(Status));
    }

    /* Set the completion port to NULL to clean up */
    Job->CompletionPort = NULL;
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

    DPRINT1("PspDeleteJob(ObjectBody: %p)\n", ObjectBody);

    PAGED_CODE();

    Job->LimitFlags = 0;

    /* Remove the reference to the completion port if associated */
    if (Job->CompletionPort != NULL)
    {
        ObDereferenceObject(Job->CompletionPort);
    }

    /* TODO: Ensure that job sets are respected */

    /* Unlink the job object */
    if (Job->JobLinks.Flink != NULL)
    {
        ExAcquireFastMutex(&PsJobListLock);

        /* Remove the job from the list */
        RemoveEntryList(&Job->JobLinks);

        ExReleaseFastMutex(&PsJobListLock);
    }

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
 *
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

    const ULONG AllowedBasicFlags = JOB_OBJECT_LIMIT_WORKINGSET |
        JOB_OBJECT_LIMIT_PROCESS_TIME |
        JOB_OBJECT_LIMIT_JOB_TIME |
        JOB_OBJECT_LIMIT_ACTIVE_PROCESS |
        JOB_OBJECT_LIMIT_AFFINITY |
        JOB_OBJECT_LIMIT_PRIORITY_CLASS |
        JOB_OBJECT_LIMIT_PRESERVE_JOB_TIME |
        JOB_OBJECT_LIMIT_SCHEDULING_CLASS;

    const ULONG AllowedExtendedFlags = AllowedBasicFlags |
        JOB_OBJECT_LIMIT_BREAKAWAY_OK |
        JOB_OBJECT_LIMIT_DIE_ON_UNHANDLED_EXCEPTION |
        JOB_OBJECT_LIMIT_PROCESS_MEMORY |
        JOB_OBJECT_LIMIT_JOB_MEMORY |
        JOB_OBJECT_LIMIT_SILENT_BREAKAWAY_OK |
        JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;

    AllowedFlags = IsExtendedLimit ? AllowedExtendedFlags : AllowedBasicFlags;

    /* Validate flags */
    if (ExtendedLimit->BasicLimitInformation.LimitFlags & ~AllowedFlags)
    {
        DPRINT1("Invalid LimitFlags specified\n");
        return STATUS_INVALID_PARAMETER;
    }

    if ((ExtendedLimit->BasicLimitInformation.LimitFlags & JOB_OBJECT_LIMIT_PRESERVE_JOB_TIME) &&
        (ExtendedLimit->BasicLimitInformation.LimitFlags & JOB_OBJECT_LIMIT_JOB_TIME))
    {
        DPRINT1("Invalid LimitFlags combination specified\n");
        return STATUS_INVALID_PARAMETER;
    }

    /* Acquire the job lock */
    ExAcquireResourceSharedLite(&Job->JobLock, TRUE);

    /*
     * Basic Limits
     */

    if (ExtendedLimit->BasicLimitInformation.LimitFlags & JOB_OBJECT_LIMIT_WORKINGSET)
    {
        /* https://learn.microsoft.com/en-us/windows/win32/api/winnt/ns-winnt-jobobject_basic_limit_information:
           "If MaximumWorkingSetSize is nonzero, MinimumWorkingSetSize cannot be zero"
           "If MinimumWorkingSetSize is nonzero, MaximumWorkingSetSize cannot be zero"
           Also check that the minimum doesn't exceed the maximum or both aren't
           equal to zero. */
        if ((ExtendedLimit->BasicLimitInformation.MaximumWorkingSetSize > 0 &&
                ExtendedLimit->BasicLimitInformation.MinimumWorkingSetSize <= 0)
            ||
            (ExtendedLimit->BasicLimitInformation.MinimumWorkingSetSize > 0 &&
                ExtendedLimit->BasicLimitInformation.MaximumWorkingSetSize <= 0)
            ||
            (ExtendedLimit->BasicLimitInformation.MaximumWorkingSetSize <
                ExtendedLimit->BasicLimitInformation.MinimumWorkingSetSize)
            ||
            (!ExtendedLimit->BasicLimitInformation.MaximumWorkingSetSize &&
                ExtendedLimit->BasicLimitInformation.MaximumWorkingSetSize))
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
        if (ExtendedLimit->BasicLimitInformation.Affinity != (ExtendedLimit->BasicLimitInformation.Affinity & KeActiveProcessors))
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
        if (ExtendedLimit->BasicLimitInformation.SchedulingClass > 5)
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
 * Callback function to associate an I/O completion port with a process.
 *
 * @param[in] Process
 *     A pointer to the process.
 *
 * @param[in, optional] Context
 *     A pointer to a context structure containing the I/O completion port and
 *     its associated key. This is passed in by the caller of the enumeration.
 *
 * @return
 *     STATUS_SUCCESS if the I/O completion port was successfully associated
 *     with the process.
 *     Otherwise, an appropriate NTSTATUS error code.
 */
static
NTSTATUS
PspAssociateCompletionPortCallback(
    _In_ PEPROCESS Process,
    _In_opt_ PVOID Context
)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PASSOCIATE_COMPLETION_PORT_CONTEXT AssociateCpContext =
        (PASSOCIATE_COMPLETION_PORT_CONTEXT)Context;

    /* Ensure the process is active and has a valid unique process ID */
    if (!(Process->JobStatus & JOB_NOT_REALLY_ACTIVE) &&
        Process->UniqueProcessId)
    {
        Status = IoSetIoCompletion(AssociateCpContext->CompletionPort,
                                   AssociateCpContext->CompletionKey,
                                   Process->UniqueProcessId,
                                   STATUS_SUCCESS,
                                   JOB_OBJECT_MSG_NEW_PROCESS,
                                   FALSE);
    }

    return Status;
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
 *     STATUS_SUCCESS if the I/O completion port was successfully associated
 *     with the job and its processes.
 *     Otherwise, an appropriate NTSTATUS error code.
 */
static
NTSTATUS
PspAssociateCompletionPortWithJob(
    _In_ PEJOB Job,
    _In_ PJOBOBJECT_ASSOCIATE_COMPLETION_PORT AssociateCpInfo
)
{
    NTSTATUS Status = STATUS_SUCCESS;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    HANDLE IoCompletion;
    ASSOCIATE_COMPLETION_PORT_CONTEXT Context;

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
    if (Job->CompletionPort || (Job->JobFlags & JOB_OBJECT_CLOSE_DONE) != 0)
    {
        ObDereferenceObject(IoCompletion);
        ExReleaseResourceLite(&Job->JobLock);
        return STATUS_INVALID_PARAMETER;
    }

    Job->CompletionKey = AssociateCpInfo->CompletionKey;
    Job->CompletionPort = IoCompletion;

    Context.CompletionPort = Job->CompletionPort;
    Context.CompletionKey = Job->CompletionKey;

    Status = PspEnumerateProcessesInJob(Job,
                                        PspAssociateCompletionPortCallback,
                                        &Context,
                                        FALSE);

    ExReleaseResourceLite(&Job->JobLock);

    return Status;
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
PspQueryBasicAccountingInfo(
    _In_ PEJOB Job,
    _Out_ PJOBOBJECT_BASIC_AND_IO_ACCOUNTING_INFORMATION BasicAndIo
)
{
    PLIST_ENTRY NextEntry;
    PROCESS_VALUES Values;

    /* Zero the basic accounting information */
    RtlZeroMemory(&BasicAndIo->BasicInfo,
                  sizeof(JOBOBJECT_BASIC_ACCOUNTING_INFORMATION));

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
        if (!BooleanFlagOn(Process->JobStatus, ACCOUNTING_FOLDED))
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
PspQueryLimitInformation(
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
        RtlZeroMemory(&ExtendedLimit->IoInfo, sizeof(IO_COUNTERS));
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
 * @param[in, out, optional] Context
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
PspQueryJobProcessIdListCallback(
    _In_ PEPROCESS Process,
    _In_opt_ PVOID Context
)
{
    PQUERY_JOB_PROCESS_ID_CONTEXT ProcContext = (PQUERY_JOB_PROCESS_ID_CONTEXT)Context;

    /* Skip processes that are not really active */
    if (Process->JobStatus & JOB_NOT_REALLY_ACTIVE)
    {
        /* Continue to the next process */
        return STATUS_SUCCESS;
    }

    /* Check if there is enough space in the list to add another process ID */
    if (ProcContext->ListLength >= sizeof(ULONG_PTR))
    {
        if (ExAcquireRundownProtection(&Process->RundownProtect))
        {
            /* Add the process ID to the list */
            *ProcContext->IdListArray++ = (ULONG_PTR)Process->UniqueProcessId;

            /* Adjust the remaining buffer space and increment the process
               count */
            ProcContext->ListLength -= sizeof(ULONG_PTR);
            ProcContext->ProcIdList->NumberOfProcessIdsInList++;

            ExReleaseRundownProtection(&Process->RundownProtect);
        }
    }
    else
    {
        /* Break the enumeration on buffer overflow */
        ProcContext->Status = STATUS_BUFFER_OVERFLOW;
        return ProcContext->Status;
    }

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
    NTSTATUS Status = STATUS_SUCCESS;
    QUERY_JOB_PROCESS_ID_CONTEXT ProcContext;

    /* Check if the buffer provided is large enough to hold at least the
       fixed portion of JOBOBJECT_BASIC_PROCESS_ID_LIST */
    if (JobInformationLength < sizeof(JOBOBJECT_BASIC_PROCESS_ID_LIST))
    {
        return STATUS_INFO_LENGTH_MISMATCH;
    }

    /* Initialize the process context */
    ProcContext.ProcIdList = ProcIdList;
    ProcContext.ListLength =
        JobInformationLength - FIELD_OFFSET(JOBOBJECT_BASIC_PROCESS_ID_LIST,
                                            ProcessIdList);
    ProcContext.IdListArray = &ProcIdList->ProcessIdList[0];
    ProcContext.Status = STATUS_SUCCESS;

    /* Fill in the number of assigned processes */
    ProcIdList->NumberOfAssignedProcesses = Job->ActiveProcesses;
    ProcIdList->NumberOfProcessIdsInList = 0;

    /* Use the enumerator to collect the process IDs
       N.B. The enumeration will stop if the callback fails */
    Status = PspEnumerateProcessesInJob(Job,
                                        PspQueryJobProcessIdListCallback,
                                        &ProcContext,
                                        TRUE);

    /* Calculate how much of the buffer was used */
    *ReturnRequiredLength = JobInformationLength - ProcContext.ListLength;

    /* Ensure the right error is propagated if the buffer was too small */
    return ProcContext.Status == STATUS_BUFFER_OVERFLOW
               ? STATUS_BUFFER_OVERFLOW
               : Status;
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
 * @unimplemented
 */
VOID
NTAPI
PsSetJobUIRestrictionsClass(PEJOB Job,
    ULONG UIRestrictionsClass)
{
    ASSERT(Job);
    (void)InterlockedExchangeUL(&Job->UIRestrictionsClass, UIRestrictionsClass);
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

    DPRINT1("NtCreateJobObject(JobHandle: %p)\n", JobHandle);

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
            _SEH2_YIELD(return _SEH2_GetExceptionCode());
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

    if (NT_SUCCESS(Status))
    {
        /* Initialize the job object */

        RtlZeroMemory(Job, sizeof(EJOB));

        InitializeListHead(&Job->JobSetLinks);
        InitializeListHead(&Job->ProcessListHead);

        /* Make sure that early destruction doesn't attempt to remove the object
           from the list before it even gets added */
        InitializeListHead(&Job->JobLinks);

        /* Inherit the session ID from the caller */
        Job->SessionId = PsGetProcessSessionId(CurrentProcess);

        /* Initialize the job limits lock */
        KeInitializeGuardedMutex(&Job->MemoryLimitsLock);

        /* Initialize the job lock */
        Status = ExInitializeResource(&Job->JobLock);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Failed to initialize job lock\n");
            ObDereferenceObject(Job);
            return Status;
        }

        /* Initialize the event object within the job */
        KeInitializeEvent(&Job->Event, NotificationEvent, FALSE);

        /* Set the scheduling class. The default is '5' per Yosifovich, P.,
           "Windows 10 System Programming, Part 1", p.264, (2020) */
        Job->SchedulingClass = 5;

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
    if (!(NT_SUCCESS(Status)))
    {
        return Status;
    }

    /* Reference the process. Make sure we have enough rights, especially to
       terminate the process. Otherwise, one could abuse job objects to
       terminate processes without having the rights to do so */
    Status = ObReferenceObjectByHandle(ProcessHandle,
                                       PROCESS_TERMINATE,
                                       PsProcessType,
                                       PreviousMode,
                                       (PVOID *)&Process,
                                       NULL);
    if (!(NT_SUCCESS(Status)))
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

    if (ExAcquireRundownProtection(&Process->RundownProtect))
    {
        /* Capture a reference for the process lifetime */
        ObReferenceObject(Job);

        /* Ensure the process is not already assigned to a job */
        ASSERT(Process->Job == NULL);

        /* Try to atomically compare-and-exchange the job pointer */
        if (InterlockedCompareExchangePointer((PVOID)&Process->Job, Job, NULL))
        {
            /* At this point, the job was referenced twice */
            ObDereferenceObjectEx(Job, 2);
            Status = STATUS_ACCESS_DENIED;
        }
        else
        {
            /* Assign the process to the job */
            Status = PspAssignProcessToJob(Process, Job);
        }

        /* TODO: UI restrictions class */

        ExReleaseRundownProtection(&Process->RundownProtect);
    }
    else
    {
        Status = STATUS_PROCESS_IS_TERMINATING;
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

    DPRINT1("NtIsProcessInJob(ProcessHandle: %p, JobHandle: %p)\n",
            ProcessHandle,
            JobHandle);

    PreviousMode = ExGetPreviousMode();

    PAGED_CODE();

    /* Check if the process handle is the current process */
    if (ProcessHandle == PsGetCurrentProcess())
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
        if (!(NT_SUCCESS(Status)))
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
            /* Get the job object from the provided job handle and compare it
               with the process job */
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
    if (ProcessHandle == PsGetCurrentProcess())
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
        ASSERT(((RequiredAlign) == 1) ||
               ((RequiredAlign) == 2) ||
               ((RequiredAlign) == 4) ||
               ((RequiredAlign) == 8) ||
               ((RequiredAlign) == 16));

        _SEH2_TRY
        {
            /* Probe the buffer */
            if (JobInformation != NULL)
            {
                ProbeForWrite(JobInformation,
                              JobInformationLength,
                              RequiredAlign);
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
                                           (PVOID*)&Job,
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
        Status = PspQueryBasicAccountingInfo(Job, &BasicAndIo);
        JobInfoBuffer = &BasicAndIo;
        break;
    }
    case JobObjectBasicLimitInformation:
    case JobObjectExtendedLimitInformation:
    {
        Status = PspQueryLimitInformation(Job,
                                          (JobInformationClass == JobObjectExtendedLimitInformation),
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
        ASSERT(((RequiredAlign) == 1) ||
               ((RequiredAlign) == 2) ||
               ((RequiredAlign) == 4) ||
               ((RequiredAlign) == 8) ||
               ((RequiredAlign) == 16));

        _SEH2_TRY
        {
            /* Probe out buffer for read */
            if (JobInformationLength != 0)
            {
                ProbeForRead(JobInformation,
                             JobInformationLength,
                             RequiredAlign);
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
                                       (PVOID*)&Job,
                                       NULL);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    /* And set the information */
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
            if (JobInformationClass == JobObjectExtendedLimitInformation)
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
    case JobObjectBasicAccountingInformation:
    case JobObjectBasicAndIoAccountingInformation:
    case JobObjectBasicProcessIdList:
    case JobObjectBasicUIRestrictions:
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
