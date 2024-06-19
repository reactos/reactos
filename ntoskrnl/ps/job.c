/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ps/job.c
 * PURPOSE:         Job Native Functions
 * PROGRAMMERS:     2004-2012 Alex Ionescu (alex@relsoft.net) (stubs)
 *                  2004-2005 Thomas Weidenmueller <w3seek@reactos.com>
 *                  2015-2016 Samuel Serapión Vega (encoded@reactos.org)
 *                  2017 Mark Jansen (mark.jansen@reactos.org)
 *                  2018 Pierre Schweitzer (pierre@reactos.org)
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

    /* Ensure the process is not already assigned to a job */
    ASSERT(Process->Job == NULL);

    /* Check if the job has a limit on the number of active processes */
    if (Job->LimitFlags & JOB_OBJECT_LIMIT_ACTIVE_PROCESS)
    {
        /* Check if job limit on active processes has been reached */
        if (Job->ActiveProcesses > Job->ActiveProcessLimit)
        {
            if (Job->CompletionPort)
            {
                /* If the job has a completion port, notify the job that the limit
                   on the number of active processes has been exceeded */
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

    /* Ensure we are not interrupted by acquiring the job lock */
    ExEnterCriticalRegionAndAcquireResourceExclusive(&Job->JobLock);

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

    /* Resume APCs and release the job lock */
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
 * @remark This function is called from PspDeleteProcess() as the process is destroyed.
 */
VOID
NTAPI
PspRemoveProcessFromJob(
    _In_ PEPROCESS Process,
    _In_ PEJOB Job
)
{
    ExEnterCriticalRegionAndAcquireResourceExclusive(&Job->JobLock);

    /* Attempt to atomically set the process's job pointer to NULL if it is currently
       set to the specified job */
    if (InterlockedCompareExchangePointer((PVOID)&Process->Job, NULL, Job) == Job)
    {
        /* Remove the process from the job's process list */
        RemoveEntryList(&Process->JobLinks);

        /* Assert that the job's active process count does not underflow */
        ASSERT((Job->ActiveProcesses - 1) < Job->ActiveProcesses);

        /* Decrement the job's active process count */
        Job->ActiveProcesses--;

        /* TODO: Ensure that job limits are respected */
    }
    else
    {
        /* The process is not in the specified job */
        DPRINT1("PspRemoveProcessFromJob(%p,%p) Process not in job\n", Process, Job);
    }

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
 * @remark This function is called from PspExitThread() as the last thread exits.
 */
VOID
NTAPI
PspExitProcessFromJob(
    _In_ PEJOB Job,
    _In_ PEPROCESS Process
)
{
    /* Make sure we are not interrupted */
    ExEnterCriticalRegionAndAcquireResourceExclusive(&Job->JobLock);

    /* Check if the process is part of the specified job */
    if (Process->Job == Job)
    {
        /* Assert that the job's active process count does not underflow */
        ASSERT((Job->ActiveProcesses - 1) < Job->ActiveProcesses);

        /* Decrement the job's active process count */
        Job->ActiveProcesses--;

        /* If the job has a completion port, notify it of the process exit */
        if (Job->CompletionPort)
        {
            IoSetIoCompletion(Job->CompletionPort,
                              Job->CompletionKey,
                              NULL,
                              STATUS_SUCCESS,
                              JOB_OBJECT_MSG_EXIT_PROCESS,
                              FALSE);
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
    NTSTATUS Status = STATUS_SUCCESS;
    PEPROCESS Process;
    PLIST_ENTRY Entry;

    /* Enter critical region and acquire exclusive lock on the job object */
    ExEnterCriticalRegionAndAcquireResourceExclusive(&Job->JobLock);

    /* Initialize entry to the first process in the job's process list */
    Entry = Job->ProcessListHead.Flink;

    /* For each process in job process list */
    while (Entry != Job->ProcessListHead.Flink)
    {
        /* Get process object */
        Process = CONTAINING_RECORD(Entry, EPROCESS, JobLinks);

        /* Move to the next process in the list */
        Entry = Entry->Flink;

        /* Increase the reference count of the process object.
           We use the safe variant here because it returns FALSE if the object
           is being deleted */
        if (ObReferenceObjectSafe(Process))
        {
            /* The process is being deleted, continue on to the next one */
            continue;
        }

        /* Terminate the process */
        Status = PsTerminateProcess(Process, ExitStatus);
        ASSERT(NT_SUCCESS(Status));

        /* Decrement the job's active process counter */
        Job->ActiveProcesses--;

        /* Check if there are no active processes left in the job */
        if (Job->ActiveProcesses == 0)
        {
            /* If so, notify anyone waiting for the job object */
            KeSetEvent(&Job->Event, IO_NO_INCREMENT, FALSE);
        }

        /* Decrease the reference count */
        ObDereferenceObject(Process);
    }

    /* Resume APCs and release lock */
    ExReleaseResourceAndLeaveCriticalRegion(&Job->JobLock);

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
 * @param[in] HandleCount
 *     Unused.
 *
 * @param[in] SystemHandleCount
 *     The number of system handles currently open for the job object.
 *
 * @remark
 *     This function terminates the job if the JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE flag is set.
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

    /* Proceed only when the last handle is left */
    if (SystemHandleCount != 1)
    {
        return;
    }

    /* If the job is set to kill on close, terminate all associated processes */
    if (Job->LimitFlags & JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE)
    {
        PspTerminateJobObject(Job, STATUS_SUCCESS);
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

    PAGED_CODE();

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
    /* FIXME - walk through the job process list and update the restrictions? */
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

        /* Set the scheduling class. The default is '5' per Windows 10 System Programming (Yosifovich, P.) */
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
                /* NOTE: if the caller passed invalid buffers to receive the handle it's his
                   own fault! the object will still be created and live... It's possible
                   to find the handle using ObFindHandleForObject()! */
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
 *     A pointer to a handle that will receive the handle of the created job object.
 *
 * @param DesiredAccess
 *     Specifies the desired access rights for the job object.
 *
 * @param ObjectAttributes
 *     Pointer to the OBJECT_ATTRIBUTES structure specifying the object name and attributes.
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

    /* Reference the job. JOB_OBJECT_ASSIGN_PROCESS rights are required for assignment */
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
       terminate the process. Otherwise, one could abuse job objects to terminate
       processes without having the rights to do so */
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
        /* Return STATUS_ACCESS_DENIED if the process is already assigned to a job or
           the session ID is different */
        ObDereferenceObject(Job);
        ObDereferenceObject(Process);
        return STATUS_ACCESS_DENIED;
    }

    /* TODO: Security checks */

    if (ExAcquireRundownProtection(&Process->RundownProtect))
    {
        /* Capture a reference for the process lifetime */
        ObReferenceObject(Job);

        /* Try to atomically compare-and-exchange the job pointer */
        if (InterlockedCompareExchangePointer((PVOID)&Process->Job, Job, NULL))
        {
            ObDereferenceObject(Job);
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
 * Determines if a specified process is associated with a specified job or any job.
 *
 * @param[in] ProcessHandle
 *     A handle to the process being queried.
 *
 * @param[in, optional] JobHandle
 *     An optional handle to the job object being compared. If NULL, the function
 *     checks if the process is associated with any job.
 *
 * @returns
 *     STATUS_PROCESS_IN_JOB if the process is in the job or any job (when JobHandle is NULL).
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
        /* If no specific job handle is provided, the process is assigned to a job */
        if (JobHandle == NULL)
        {
            Status = STATUS_PROCESS_IN_JOB;
        }
        else
        {
            /* Get the job object from the provided job handle and compare it with the process job */
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

/*
 * @implemented
 */
NTSTATUS
NTAPI
NtQueryInformationJobObject (
    HANDLE JobHandle,
    JOBOBJECTINFOCLASS JobInformationClass,
    PVOID JobInformation,
    ULONG JobInformationLength,
    PULONG ReturnLength )
{
    PEJOB Job;
    NTSTATUS Status;
    BOOLEAN NoOutput;
    PVOID GenericCopy;
    PLIST_ENTRY NextEntry;
    PKTHREAD CurrentThread;
    KPROCESSOR_MODE PreviousMode;
    JOBOBJECT_EXTENDED_LIMIT_INFORMATION ExtendedLimit;
    JOBOBJECT_BASIC_AND_IO_ACCOUNTING_INFORMATION BasicAndIo;
    ULONG RequiredLength, RequiredAlign, SizeToCopy, NeededSize;

    PAGED_CODE();

    CurrentThread  = KeGetCurrentThread();

    /* Validate class */
    if (JobInformationClass > JobObjectJobSetInformation || JobInformationClass < JobObjectBasicAccountingInformation)
    {
        return STATUS_INVALID_INFO_CLASS;
    }

    /* Get associated lengths & alignments */
    RequiredLength = PspJobInfoLengths[JobInformationClass];
    RequiredAlign = PspJobInfoAlign[JobInformationClass];
    SizeToCopy = RequiredLength;
    NeededSize = RequiredLength;

    /* If length mismatch (needed versus provided) */
    if (JobInformationLength != RequiredLength)
    {
        /* This can only be accepted if: JobObjectBasicProcessIdList or JobObjectSecurityLimitInformation
         * Or if size is bigger than needed
         */
        if ((JobInformationClass != JobObjectBasicProcessIdList && JobInformationClass != JobObjectSecurityLimitInformation) ||
            JobInformationLength < RequiredLength)
        {
            return STATUS_INFO_LENGTH_MISMATCH;
        }

        /* Set what we need to copy out */
        SizeToCopy = JobInformationLength;
    }

    PreviousMode = ExGetPreviousMode();
    /* If not comming from umode, we need to probe buffers */
    if (PreviousMode != KernelMode)
    {
        ASSERT(((RequiredAlign) == 1) || ((RequiredAlign) == 2) || ((RequiredAlign) == 4) || ((RequiredAlign) == 8) || ((RequiredAlign) == 16));

        _SEH2_TRY
        {
            /* Probe out buffer for write */
            if (JobInformation != NULL)
            {
                ProbeForWrite(JobInformation, JobInformationLength, RequiredAlign);
            }

            /* But also return length if asked */
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
    /* Select class */
    switch (JobInformationClass)
    {
        /* Basic counters */
        case JobObjectBasicAccountingInformation:
        case JobObjectBasicAndIoAccountingInformation:
            /* Zero basics */
            RtlZeroMemory(&BasicAndIo.BasicInfo, sizeof(JOBOBJECT_BASIC_ACCOUNTING_INFORMATION));

            /* Lock */
            KeEnterGuardedRegionThread(CurrentThread);
            ExAcquireResourceSharedLite(&Job->JobLock, TRUE);

            /* Initialize with job counters */
            BasicAndIo.BasicInfo.TotalUserTime.QuadPart = Job->TotalUserTime.QuadPart;
            BasicAndIo.BasicInfo.TotalKernelTime.QuadPart = Job->TotalKernelTime.QuadPart;
            BasicAndIo.BasicInfo.ThisPeriodTotalUserTime.QuadPart = Job->ThisPeriodTotalUserTime.QuadPart;
            BasicAndIo.BasicInfo.ThisPeriodTotalKernelTime.QuadPart = Job->ThisPeriodTotalKernelTime.QuadPart;
            BasicAndIo.BasicInfo.TotalPageFaultCount = Job->TotalPageFaultCount;
            BasicAndIo.BasicInfo.TotalProcesses = Job->TotalProcesses;
            BasicAndIo.BasicInfo.ActiveProcesses = Job->ActiveProcesses;
            BasicAndIo.BasicInfo.TotalTerminatedProcesses = Job->TotalTerminatedProcesses;

            /* We also set IoInfo, even though we might not return it */
            BasicAndIo.IoInfo.ReadOperationCount = Job->ReadOperationCount;
            BasicAndIo.IoInfo.WriteOperationCount = Job->WriteOperationCount;
            BasicAndIo.IoInfo.OtherOperationCount = Job->OtherOperationCount;
            BasicAndIo.IoInfo.ReadTransferCount = Job->ReadTransferCount;
            BasicAndIo.IoInfo.WriteTransferCount = Job->WriteTransferCount;
            BasicAndIo.IoInfo.OtherTransferCount = Job->OtherTransferCount;

            /* For every process, sum its counters */
            for (NextEntry = Job->ProcessListHead.Flink;
                 NextEntry != &Job->ProcessListHead;
                 NextEntry = NextEntry->Flink)
            {
                PEPROCESS Process;

                Process = CONTAINING_RECORD(NextEntry, EPROCESS, JobLinks);
                if (!BooleanFlagOn(Process->JobStatus, 2))
                {
                    PROCESS_VALUES Values;

                    KeQueryValuesProcess(&Process->Pcb, &Values);
                    BasicAndIo.BasicInfo.TotalUserTime.QuadPart += Values.TotalUserTime.QuadPart;
                    BasicAndIo.BasicInfo.TotalKernelTime.QuadPart += Values.TotalKernelTime.QuadPart;
                    BasicAndIo.IoInfo.ReadOperationCount += Values.IoInfo.ReadOperationCount;
                    BasicAndIo.IoInfo.WriteOperationCount += Values.IoInfo.WriteOperationCount;
                    BasicAndIo.IoInfo.OtherOperationCount += Values.IoInfo.OtherOperationCount;
                    BasicAndIo.IoInfo.ReadTransferCount += Values.IoInfo.ReadTransferCount;
                    BasicAndIo.IoInfo.WriteTransferCount += Values.IoInfo.WriteTransferCount;
                    BasicAndIo.IoInfo.OtherTransferCount += Values.IoInfo.OtherTransferCount;
                }
            }

            /* And done */
            ExReleaseResourceLite(&Job->JobLock);
            KeLeaveGuardedRegionThread(CurrentThread);

            /* We'll copy back the buffer */
            GenericCopy = &BasicAndIo;
            Status = STATUS_SUCCESS;

            break;

        /* Limits information */
        case JobObjectBasicLimitInformation:
        case JobObjectExtendedLimitInformation:
            /* Lock */
            KeEnterGuardedRegionThread(CurrentThread);
            ExAcquireResourceSharedLite(&Job->JobLock, TRUE);

            /* Copy basic information */
            ExtendedLimit.BasicLimitInformation.LimitFlags = Job->LimitFlags;
            ExtendedLimit.BasicLimitInformation.MinimumWorkingSetSize = Job->MinimumWorkingSetSize;
            ExtendedLimit.BasicLimitInformation.MaximumWorkingSetSize = Job->MaximumWorkingSetSize;
            ExtendedLimit.BasicLimitInformation.ActiveProcessLimit = Job->ActiveProcessLimit;
            ExtendedLimit.BasicLimitInformation.PriorityClass = Job->PriorityClass;
            ExtendedLimit.BasicLimitInformation.SchedulingClass = Job->SchedulingClass;
            ExtendedLimit.BasicLimitInformation.Affinity = Job->Affinity;
            ExtendedLimit.BasicLimitInformation.PerProcessUserTimeLimit.QuadPart = Job->PerProcessUserTimeLimit.QuadPart;
            ExtendedLimit.BasicLimitInformation.PerJobUserTimeLimit.QuadPart = Job->PerJobUserTimeLimit.QuadPart;

            /* If asking for extending limits */
            if (JobInformationClass == JobObjectExtendedLimitInformation)
            {
                /* Lock our memory lock */
                KeAcquireGuardedMutexUnsafe(&Job->MemoryLimitsLock);
                /* Return limits */
                ExtendedLimit.ProcessMemoryLimit = Job->ProcessMemoryLimit << PAGE_SHIFT;
                ExtendedLimit.JobMemoryLimit = Job->JobMemoryLimit << PAGE_SHIFT;
                ExtendedLimit.PeakProcessMemoryUsed = Job->PeakProcessMemoryUsed << PAGE_SHIFT;
                ExtendedLimit.PeakJobMemoryUsed = Job->PeakJobMemoryUsed << PAGE_SHIFT;
                KeReleaseGuardedMutexUnsafe(&Job->MemoryLimitsLock);

                /* And done */
                ExReleaseResourceLite(&Job->JobLock);
                KeLeaveGuardedRegionThread(CurrentThread);

                /* We'll never return IoInfo, so zero it out to avoid
                 * kernel memory leak
                 */
                RtlZeroMemory(&ExtendedLimit.IoInfo, sizeof(IO_COUNTERS));
            }
            else
            {
                /* And done */
                ExReleaseResourceLite(&Job->JobLock);
                KeLeaveGuardedRegionThread(CurrentThread);
            }

            /* We'll copy back the buffer */
            GenericCopy = &ExtendedLimit;
            Status = STATUS_SUCCESS;

            break;

        default:
            DPRINT1("Class %d not implemented\n", JobInformationClass);
            Status = STATUS_NOT_IMPLEMENTED;
            break;
    }

    /* Job is no longer required */
    ObDereferenceObject(Job);

    /* If we succeeed, copy back data */
    if (NT_SUCCESS(Status))
    {
        _SEH2_TRY
        {
            /* If we have anything to copy, do it */
            if (!NoOutput)
            {
                RtlCopyMemory(JobInformation, GenericCopy, SizeToCopy);
            }

            /* And return length if asked */
            if (ReturnLength != NULL)
            {
                *ReturnLength = NeededSize;
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

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
NtSetInformationJobObject (
    HANDLE JobHandle,
    JOBOBJECTINFOCLASS JobInformationClass,
    PVOID JobInformation,
    ULONG JobInformationLength)
{
    PEJOB Job;
    NTSTATUS Status;
    PKTHREAD CurrentThread;
    ACCESS_MASK DesiredAccess;
    KPROCESSOR_MODE PreviousMode;
    ULONG RequiredLength, RequiredAlign;

    PAGED_CODE();

    CurrentThread  = KeGetCurrentThread();

    /* Validate class */
    if (JobInformationClass > JobObjectJobSetInformation || JobInformationClass < JobObjectBasicAccountingInformation)
    {
        return STATUS_INVALID_INFO_CLASS;
    }

    /* Get associated lengths & alignments */
    RequiredLength = PspJobInfoLengths[JobInformationClass];
    RequiredAlign = PspJobInfoAlign[JobInformationClass];

    PreviousMode = ExGetPreviousMode();
    /* If not comming from umode, we need to probe buffers */
    if (PreviousMode != KernelMode)
    {
        ASSERT(((RequiredAlign) == 1) || ((RequiredAlign) == 2) || ((RequiredAlign) == 4) || ((RequiredAlign) == 8) || ((RequiredAlign) == 16));

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

    /* Validate input size */
    if (JobInformationLength != RequiredLength)
    {
        return STATUS_INFO_LENGTH_MISMATCH;
    }

    /* Open the given job */
    DesiredAccess = JOB_OBJECT_SET_ATTRIBUTES;
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
        case JobObjectExtendedLimitInformation:
            DPRINT1("Class JobObjectExtendedLimitInformation not implemented\n");
            Status = STATUS_SUCCESS;
            break;

        default:
            DPRINT1("Class %d not implemented\n", JobInformationClass);
            Status = STATUS_NOT_IMPLEMENTED;
            break;
    }
    KeLeaveGuardedRegionThread(CurrentThread);

    ObDereferenceObject(Job);

    return Status;
}

/* EOF */
