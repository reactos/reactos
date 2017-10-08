/*
 * PROJECT:     ReactOS kernel
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Job Native Functions
 * COPYRIGHT:   Copyright 2004-2012 Alex Ionescu (alex@relsoft.net)
 * COPYRIGHT:   Copyright 2004,2005 Thomas Weidenmueller (w3seek@reactos.com)
 * COPYRIGHT:   Copyright 2015,2016 Samuel Serapión Vega (encoded@reactos.org)
 * COPYRIGHT:   Copyright 2017 Mark Jansen (mark.jansen@reactos.org)
 */

/* INCLUDES *****************************************************************/

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

#define JOB_SUPPORTED_LIMIT_FLAGS   \
    (JOB_OBJECT_LIMIT_ACTIVE_PROCESS | \
     JOB_OBJECT_LIMIT_BREAKAWAY_OK | \
     JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE | \
     JOB_OBJECT_LIMIT_SILENT_BREAKAWAY_OK)



/* FUNCTIONS *****************************************************************/

NTSTATUS
NTAPI
PspTerminateJobObject(PEJOB Job,
                      KPROCESSOR_MODE AccessMode,
                      NTSTATUS ExitStatus);


VOID
NTAPI
PspCloseJob(IN PEPROCESS Process OPTIONAL,
            IN PVOID ObjectBody,
            IN ACCESS_MASK GrantedAccess,
            IN ULONG HandleCount,
            IN ULONG SystemHandleCount)
{
    PEJOB Job = (PEJOB)ObjectBody;

    DPRINT("PspCloseJob(%p, %p, %x, %u, %u)\n", Process, ObjectBody, GrantedAccess, HandleCount, SystemHandleCount);

    /* Only run when we are the last one to close it */
    if (SystemHandleCount != 1)
        return;

    if (Job->LimitFlags & JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE)
    {
        KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
        DPRINT("PspCloseJob() ==> Trying to destroy the job!\n");
        ObReferenceObject(Job);
        PspTerminateJobObject(Job, PreviousMode, STATUS_SUCCESS);
        ObDereferenceObject(Job);
    }
}

VOID
NTAPI
PspDeleteJob(PVOID ObjectBody)
{
    PEJOB Job = (PEJOB)ObjectBody;

    /* remove the reference to the completion port if associated */
    if (Job->CompletionPort != NULL)
    {
        ObDereferenceObject(Job->CompletionPort);
    }

    /* unlink the job object */
    if (!IsListEmpty(&Job->JobLinks))
    {
        ExAcquireFastMutex(&PsJobListLock);
        RemoveEntryList(&Job->JobLinks);
        ExReleaseFastMutex(&PsJobListLock);
    }

    ExDeleteResource(&Job->JobLock);
}

VOID
NTAPI
INIT_FUNCTION
PspInitializeJobStructures(VOID)
{
    InitializeListHead(&PsJobListHead);
    ExInitializeFastMutex(&PsJobListLock);
}

NTSTATUS
NTAPI
PspAssignProcessToJob(PEPROCESS Process,
                      PEJOB Job)
{
    NTSTATUS Status = STATUS_SUCCESS;
    DPRINT("PspAssignProcessToJob(%p,%p)\n", Process, Job);

    /* Grab a reference for the lifetime of the process, released in ps/kill.c */
    ObReferenceObject(Job);


    if (Job->LimitFlags & JOB_OBJECT_LIMIT_ACTIVE_PROCESS)
    {
        if (Job->ActiveProcesses >= Job->ActiveProcessLimit)
        {

        }
    }

    /* Make sure we are not interrupted */
    ExEnterCriticalRegionAndAcquireResourceExclusive(&Job->JobLock);

    /* Assign process to job object */
    InsertTailList(&Job->ProcessListHead, &Process->JobLinks);

    /* Increment counters */
    Job->TotalProcesses++;
    Job->ActiveProcesses++;

    if (Job->CompletionPort && Process->UniqueProcessId)
    {
        /* notify the job object of the new proccess */
        Status = IoSetIoCompletion(Job->CompletionPort,
                                   Job->CompletionKey,
                                   (PVOID)Process->UniqueProcessId,
                                   STATUS_SUCCESS,
                                   JOB_OBJECT_MSG_NEW_PROCESS,
                                   FALSE);
    }

    /* resume APCs and release lock */
    ExReleaseResourceAndLeaveCriticalRegion(&Job->JobLock);

    return Status;
}

NTSTATUS
NTAPI
PspTerminateJobObject(PEJOB Job,
                      KPROCESSOR_MODE AccessMode,
                      NTSTATUS ExitStatus)
{
    PEPROCESS Process;
    PLIST_ENTRY Entry;
    NTSTATUS Status;

    DPRINT("PspTerminateJobObject(%p,%x,%x)\n", Job, AccessMode, ExitStatus);

    ExEnterCriticalRegionAndAcquireResourceExclusive(&Job->JobLock);

    Entry = Job->ProcessListHead.Flink;
    /* for each process in job process list */
    while (Entry != Job->ProcessListHead.Flink)
    {
        /* fetch process object */
        Process = CONTAINING_RECORD(Entry, EPROCESS, JobLinks);
        ObReferenceObject(Process);
        Entry = Entry->Flink;

        /* process might be getting deleted already */
        if (ExAcquireRundownProtection(&Process->RundownProtect))
        {
            /* terminate process */
            Status = PsTerminateProcess(Process, ExitStatus);
            ASSERT(NT_SUCCESS(Status));

            ExReleaseRundownProtection(&Process->RundownProtect);
        }
        else
        {
            //__debugbreak();
        }

        /* say good bye and get next one */
        ObDereferenceObject(Process);
    }

    /* resume APCs and release lock */
    ExReleaseResourceAndLeaveCriticalRegion(&Job->JobLock);

    /* notify anyone waiting for the job object that we are done */
    KeSetEvent(&Job->Event, IO_NO_INCREMENT, FALSE);

    return STATUS_SUCCESS;
}

VOID
NTAPI
PspRemoveProcessFromJob(IN PEPROCESS Process,
                        IN PEJOB Job)
{
    /* This function is called as the Process is destroyed, see ps/kill.c */
    DPRINT("PspRemoveProcessFromJob(%p,%p)\n", Process, Job);

    ExEnterCriticalRegionAndAcquireResourceExclusive(&Job->JobLock);

    /* If we are the one that removed the Job object from the process, also unlink the process */
    if (InterlockedCompareExchangePointer((PVOID)&Process->Job, NULL, Job) == Job)
    {
        RemoveEntryList(&Process->JobLinks);
        ASSERT((Job->ActiveProcesses-1) < Job->ActiveProcesses);
        Job->ActiveProcesses--;
    }
    else
    {
        DPRINT1("PspRemoveProcessFromJob(%p,%p) Process not in job\n", Process, Job);
    }

    ExReleaseResourceAndLeaveCriticalRegion(&Job->JobLock);
}

VOID
NTAPI
PspExitProcessFromJob(IN PEJOB Job,
                      IN PEPROCESS Process)
{
    /* This function is called when the last thread exits, see ps/kill.c */
    DPRINT("PspExitProcessFromJob(0x%p, 0x%p)\n", Job, Process);

    /* Make sure we are not interrupted */
    ExEnterCriticalRegionAndAcquireResourceExclusive(&Job->JobLock);

    /* Decrement counters */
    if (Process->Job == Job)
    {
        //ASSERT((Job->ActiveProcesses-1) < Job->ActiveProcesses);
        //Job->ActiveProcesses--;

        if (Job->CompletionPort)
        {
            /* Notify Job of process exit */
            IoSetIoCompletion(Job->CompletionPort,
                              Job->CompletionKey,
                              NULL,
                              STATUS_SUCCESS,
                              JOB_OBJECT_MSG_EXIT_PROCESS,
                              FALSE);
        }

        if (Job->ActiveProcesses == 0 && Job->CompletionPort)
        {
            /* Notify Job that all process have exited */
            IoSetIoCompletion(Job->CompletionPort,
                              Job->CompletionKey,
                              NULL,
                              STATUS_SUCCESS,
                              JOB_OBJECT_MSG_ACTIVE_PROCESS_ZERO,
                              FALSE);
        }
    }
    /* FIXME: Update limits and process stats */

    /* resume APCs and release lock */
    ExReleaseResourceAndLeaveCriticalRegion(&Job->JobLock);
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
NtAssignProcessToJobObject(HANDLE JobHandle,
                           HANDLE ProcessHandle)
{
    PEPROCESS Process;
    KPROCESSOR_MODE PreviousMode;
    NTSTATUS Status;

    PAGED_CODE();
    DPRINT("NtAssignProcessToJobObject(%x,%x)\n", JobHandle, ProcessHandle);

    PreviousMode = ExGetPreviousMode();

    /* make sure we're having a handle with enough rights, especially the to
    terminate the process. otherwise one could abuse the job objects to
    terminate processes without having rights granted to do so! The reason
    I open the process handle before the job handle is that a simple test showed
    that it first complains about a invalid process handle! The other way around
    would be simpler though... */
    Status = ObReferenceObjectByHandle(ProcessHandle,
                                       PROCESS_TERMINATE,
                                       PsProcessType,
                                       PreviousMode,
                                       (PVOID*)&Process,
                                       NULL);
    if (NT_SUCCESS(Status))
    {
        PEJOB Job;
        Status = ObReferenceObjectByHandle(JobHandle,
                                           JOB_OBJECT_ASSIGN_PROCESS,
                                           PsJobType,
                                           PreviousMode,
                                           (PVOID*)&Job,
                                           NULL);
        if (NT_SUCCESS(Status))
        {
            if (ExAcquireRundownProtection(&Process->RundownProtect))
            {
                if (Process->Job == NULL && PsGetProcessSessionId(Process) == Job->SessionId)
                {
                    /* Change the pointer */
                    if (InterlockedCompareExchangePointer((PVOID)&Process->Job, Job, NULL))
                    {
                        /* we already had one set, fail */
                        Status = STATUS_ACCESS_DENIED;
                    }
                }
                else
                {
                    /* process is already assigned to a job or session id differs! */
                    Status = STATUS_ACCESS_DENIED;
                }
                if (NT_SUCCESS(Status))
                {
                    Status = PspAssignProcessToJob(Process, Job);
                }
                ExReleaseRundownProtection(&Process->RundownProtect);
            }
            else
            {
                Status = STATUS_PROCESS_IS_TERMINATING;
            }
            ObDereferenceObject(Job);
        }
        ObDereferenceObject(Process);
    }

    return Status;
}

NTSTATUS
NTAPI
NtCreateJobSet(IN ULONG NumJob,
               IN PJOB_SET_ARRAY UserJobSet,
               IN ULONG Flags)
{
    DPRINT("NtCreateJobSet(%u, %p, 0x%x)\n", NumJob, UserJobSet, Flags);
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
NtCreateJobObject(PHANDLE JobHandle,
                  ACCESS_MASK DesiredAccess,
                  POBJECT_ATTRIBUTES ObjectAttributes)
{
    HANDLE hJob;
    PEJOB Job;
    KPROCESSOR_MODE PreviousMode;
    PEPROCESS CurrentProcess;
    NTSTATUS Status;

    PAGED_CODE();

    PreviousMode = ExGetPreviousMode();
    CurrentProcess = PsGetCurrentProcess();

    /* check for valid buffers */
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

    Status = ObCreateObject(PreviousMode,
                            PsJobType,
                            ObjectAttributes,
                            PreviousMode,
                            NULL,
                            sizeof(EJOB),
                            0,
                            0,
                            (PVOID*)&Job);

    if (NT_SUCCESS(Status))
    {
        /* FIXME - Zero all fields as we don't yet implement all of them */
        RtlZeroMemory(Job, sizeof(EJOB));

        /* make sure that early destruction doesn't attempt to remove the object from
        the list before it even gets added! */
        InitializeListHead(&Job->JobLinks);

        /* setup the job object - FIXME: More to do! */
        InitializeListHead(&Job->JobSetLinks);
        InitializeListHead(&Job->ProcessListHead);

        /* inherit the session id from the caller */
        Job->SessionId = PsGetProcessSessionId(CurrentProcess);

        Status = ExInitializeResource(&Job->JobLock);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Failed to initialize job lock!!!\n");
            ObDereferenceObject(Job);
            return Status;
        }
        KeInitializeEvent(&Job->Event, NotificationEvent, FALSE);

        /* link the object into the global job list */
        ExAcquireFastMutex(&PsJobListLock);
        InsertTailList(&PsJobListHead, &Job->JobLinks);
        ExReleaseFastMutex(&PsJobListLock);

        Status = ObInsertObject(Job,
                                NULL,
                                DesiredAccess,
                                0,
                                NULL,
                                &hJob);
        if (NT_SUCCESS(Status))
        {
            /* pass the handle back to the caller */
            _SEH2_TRY
            {
                /* NOTE: if the caller passed invalid buffers to receive the handle it's his
                own fault! the object will still be created and live... It's possible
                to find the handle using ObFindHandleForObject()! */
                *JobHandle = hJob;
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


/*
 * @implemented
 */
NTSTATUS
NTAPI
NtIsProcessInJob(IN HANDLE ProcessHandle,
                 IN HANDLE JobHandle OPTIONAL)
{
    KPROCESSOR_MODE PreviousMode;
    PEPROCESS Process;
    NTSTATUS Status;

    PreviousMode = ExGetPreviousMode();

    PAGED_CODE();

    DPRINT("NtIsProcessInJob(%x, %x)\n", ProcessHandle, JobHandle);
    Status = ObReferenceObjectByHandle(ProcessHandle,
                                       PROCESS_QUERY_INFORMATION,
                                       PsProcessType,
                                       PreviousMode,
                                       (PVOID*)&Process,
                                       NULL);
    if (NT_SUCCESS(Status))
    {
        if (Process->Job != NULL)
        {
            if (JobHandle == NULL)
            {
                /* the process is assigned to a job */
                Status = STATUS_PROCESS_IN_JOB;
            }
            else /* JobHandle != NULL */
            {
                PEJOB JobObject;

                /* get the job object and compare the object pointer with the one assigned to the process */
                Status = ObReferenceObjectByHandle(JobHandle,
                                                   JOB_OBJECT_QUERY,
                                                   PsJobType,
                                                   PreviousMode,
                                                   (PVOID*)&JobObject,
                                                   NULL);
                if (NT_SUCCESS(Status))
                {
                    Status = ((Process->Job == JobObject) ? STATUS_PROCESS_IN_JOB : STATUS_PROCESS_NOT_IN_JOB);
                    ObDereferenceObject(JobObject);
                }
            }
        }
        else
        {
            /* the process is not assigned to any job */
            Status = STATUS_PROCESS_NOT_IN_JOB;
        }
        ObDereferenceObject(Process);
    }

    return Status;
}


/*
 * @implemented
 */
NTSTATUS
NTAPI
NtOpenJobObject(PHANDLE JobHandle,
                ACCESS_MASK DesiredAccess,
                POBJECT_ATTRIBUTES ObjectAttributes)
{
    KPROCESSOR_MODE PreviousMode;
    HANDLE hJob;
    NTSTATUS Status;

    PAGED_CODE();

    PreviousMode = ExGetPreviousMode();

    /* check for valid buffers */
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
                                &hJob);
    if (NT_SUCCESS(Status))
    {
        _SEH2_TRY
        {
            *JobHandle = hJob;
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            Status = _SEH2_GetExceptionCode();
        }
        _SEH2_END;
    }

    return Status;
}


/*
 * @implemented
 */
NTSTATUS
NTAPI
NtQueryInformationJobObject(HANDLE JobHandle,
                            JOBOBJECTINFOCLASS JobInformationClass,
                            PVOID JobInformation,
                            ULONG JobInformationLength,
                            PULONG ReturnLength)
{
    PEJOB JobObject;
    NTSTATUS Status;
    ULONG Length;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();

    PAGED_CODE();

    DPRINT("NtQueryInformationJobObject(%x,%x,%p,%d)\n", JobHandle, JobInformationClass, JobInformation, JobInformationLength);

    /* Check for user-mode caller */
    if (PreviousMode != KernelMode)
    {
        /* Prepare to probe parameters */
        _SEH2_TRY
        {
            /* Probe the buffer */
            ProbeForWrite(JobInformation,
                          JobInformationLength,
                          sizeof(ULONG));

            /* Probe the return length if required */
            if (ReturnLength) ProbeForWriteUlong(ReturnLength);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            /* Return the exception code */
            _SEH2_YIELD(return _SEH2_GetExceptionCode());
        }
        _SEH2_END;
    }

    /* Grab a reference to Job */
    Status = ObReferenceObjectByHandle(JobHandle,
                                       JOB_OBJECT_QUERY,
                                       PsJobType,
                                       PreviousMode,
                                       (PVOID*)&JobObject,
                                       NULL);

    if (!NT_SUCCESS(Status))
        return Status;

    /* protect the job from changes while we query it */
    ExEnterCriticalRegionAndAcquireResourceExclusive(&JobObject->JobLock);

    _SEH2_TRY
    {
        switch (JobInformationClass)
        {
            case JobObjectBasicAccountingInformation:
            {
                PJOBOBJECT_BASIC_ACCOUNTING_INFORMATION BasicAccounting = JobInformation;
                Length = sizeof(JOBOBJECT_BASIC_ACCOUNTING_INFORMATION);
                if (JobInformationLength != Length)
                {
                    Status = STATUS_INFO_LENGTH_MISMATCH;
                    break;
                }

                BasicAccounting->TotalUserTime.QuadPart = JobObject->TotalUserTime.QuadPart;
                BasicAccounting->TotalKernelTime.QuadPart = JobObject->TotalKernelTime.QuadPart;
                /* FIXME: Add Active process user + kernel time */

                BasicAccounting->ThisPeriodTotalUserTime.QuadPart = JobObject->ThisPeriodTotalUserTime.QuadPart;
                BasicAccounting->ThisPeriodTotalKernelTime.QuadPart = JobObject->ThisPeriodTotalKernelTime.QuadPart;
                BasicAccounting->TotalPageFaultCount = JobObject->TotalPageFaultCount;
                BasicAccounting->TotalProcesses = JobObject->TotalProcesses;
                BasicAccounting->ActiveProcesses = JobObject->ActiveProcesses;
                BasicAccounting->TotalTerminatedProcesses = JobObject->TotalTerminatedProcesses;
                break;
            }
            case JobObjectBasicLimitInformation:
            case JobObjectExtendedLimitInformation:
            {
                PJOBOBJECT_BASIC_LIMIT_INFORMATION BasicLimit = (PJOBOBJECT_BASIC_LIMIT_INFORMATION)JobInformation;
                PJOBOBJECT_EXTENDED_LIMIT_INFORMATION ExtendedLimit = (PJOBOBJECT_EXTENDED_LIMIT_INFORMATION)JobInformation;

                if (JobInformationClass == JobObjectBasicLimitInformation)
                {
                    /* Set the length required and validate it */
                    Length = sizeof(JOBOBJECT_BASIC_LIMIT_INFORMATION);
                    if (JobInformationLength != Length)
                    {
                        Status = STATUS_INFO_LENGTH_MISMATCH;
                        break;
                    }
                }
                else if (JobInformationClass == JobObjectExtendedLimitInformation)
                {
                    /* Set the length required and validate it */
                    Length = sizeof(JOBOBJECT_EXTENDED_LIMIT_INFORMATION);
                    if (JobInformationLength != Length)
                    {
                        Status = STATUS_INFO_LENGTH_MISMATCH;
                        break;
                    }

                    /* copy extended limit field */
                    ExtendedLimit->ProcessMemoryLimit = JobObject->ProcessMemoryLimit;
                    ExtendedLimit->JobMemoryLimit = JobObject->JobMemoryLimit;
                    ExtendedLimit->PeakProcessMemoryUsed = JobObject->PeakProcessMemoryUsed;
                    ExtendedLimit->PeakJobMemoryUsed = JobObject->PeakJobMemoryUsed;
                }

                /* Copy basic limit fields */
                BasicLimit->PerProcessUserTimeLimit = JobObject->PerProcessUserTimeLimit;
                BasicLimit->PerJobUserTimeLimit = JobObject->PerJobUserTimeLimit;
                BasicLimit->LimitFlags = JobObject->LimitFlags;
                BasicLimit->MinimumWorkingSetSize = JobObject->MinimumWorkingSetSize;
                BasicLimit->MaximumWorkingSetSize = JobObject->MaximumWorkingSetSize;
                BasicLimit->ActiveProcessLimit = JobObject->ActiveProcessLimit;
                BasicLimit->Affinity = JobObject->Affinity;
                BasicLimit->PriorityClass = JobObject->PriorityClass;
                BasicLimit->SchedulingClass = JobObject->SchedulingClass;
                break;
            }
            case JobObjectBasicProcessIdList:
            {
                PJOBOBJECT_BASIC_PROCESS_ID_LIST ProcIdList = (PJOBOBJECT_BASIC_PROCESS_ID_LIST)JobInformation;
                PULONG_PTR IdListArray = &ProcIdList->ProcessIdList[0];
                PLIST_ENTRY Next = JobObject->ProcessListHead.Flink;
                ULONG ListLength = JobInformationLength - FIELD_OFFSET(JOBOBJECT_BASIC_PROCESS_ID_LIST, ProcessIdList);

                /* This info param needs enough space for the list */
                Length = sizeof(JOBOBJECT_BASIC_PROCESS_ID_LIST);
                if (JobInformationLength < Length)
                {
                    /* minimum requeried is the structure */
                    Status = STATUS_INFO_LENGTH_MISMATCH;
                    break;
                }

                ProcIdList->NumberOfAssignedProcesses = JobObject->ActiveProcesses;
                ProcIdList->NumberOfProcessIdsInList = 0;

                while (Next != &JobObject->ProcessListHead)
                {
                    PEPROCESS Process = (PEPROCESS)CONTAINING_RECORD(Next, EPROCESS, JobLinks);

                    /* does another ULONG_PTR fit in the structure? */
                    if (ListLength >= sizeof(ULONG_PTR))
                    {
                        /* check process is not shutting down */
                        if (ExAcquireRundownProtection(&Process->RundownProtect))
                        {
                            /* add process to list */
                            *IdListArray++ = (ULONG_PTR)Process->UniqueProcessId;
                            ListLength -= sizeof(ULONG_PTR);
                            ProcIdList->NumberOfProcessIdsInList++;

                            /* release rundown protection */
                            ExReleaseRundownProtection(&Process->RundownProtect);
                        }
                    }
                    else
                    {
                        Status = STATUS_BUFFER_OVERFLOW;
                        Length = ListLength;
                        break;
                    }
                    Next = Next->Flink;
                }

                /* we returned this ammount of bytes */
                Length = JobInformationLength - ListLength;
                break;
            }
            default:
            {
                DPRINT1("Unhandled info class 0x%x\n", JobInformationClass);
                Status = STATUS_NOT_IMPLEMENTED;
                break;
            }
        }
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        /* Return the exception code */
        Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END;

    /* resume APCs and release lock */
    ExReleaseResourceAndLeaveCriticalRegion(&JobObject->JobLock);

    /* Release reference to Job */
    ObDereferenceObject(JobObject);

    /* Protect write with SEH */
    _SEH2_TRY
    {
        /* Check if caller wanted return length */
        if ((ReturnLength) && (Length)) *ReturnLength = Length;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        /* Get exception code */
        Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END;

    return Status;
}


#define JOB_OBJECT_BASIC_LIMIT_VALID_FLAGS          0x000000ff
#define JOB_OBJECT_EXTENDED_LIMIT_VALID_FLAGS       0x00007fff

/*
 * @implemented
 */
NTSTATUS
NTAPI
NtSetInformationJobObject(HANDLE JobHandle,
                          JOBOBJECTINFOCLASS JobInformationClass,
                          PVOID JobInformation,
                          ULONG JobInformationLength)
{
    NTSTATUS Status;
    PEJOB JobObject;
    PVOID IoCompletionPtr;
    KPROCESSOR_MODE PreviousMode;

    PAGED_CODE();
    PreviousMode = KeGetPreviousMode();

    DPRINT("NtSetInformationJobObject(JobHandle=%x,JobInformationClass=%x,JobInformation=%p,JobInformationLength=%d)\n",
           JobHandle, JobInformationClass, JobInformation, JobInformationLength);

    /* Grab a reference to Job */
    Status = ObReferenceObjectByHandle(JobHandle,
                                       JOB_OBJECT_SET_ATTRIBUTES,
                                       PsJobType,
                                       PreviousMode,
                                       (PVOID*)&JobObject,
                                       NULL);

    if (!NT_SUCCESS(Status)) return Status;

    ExEnterCriticalRegionAndAcquireResourceExclusive(&JobObject->JobLock);

    switch (JobInformationClass)
    {
        case JobObjectAssociateCompletionPortInformation:
        {
            JOBOBJECT_ASSOCIATE_COMPLETION_PORT CompletionPortInfo;
            _SEH2_TRY
            {
                CompletionPortInfo = *(PJOBOBJECT_ASSOCIATE_COMPLETION_PORT)JobInformation;
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                /* Get the exception code */
                Status = _SEH2_GetExceptionCode();
                _SEH2_YIELD(break);
            }
            _SEH2_END;

            if (JobInformationLength != sizeof(JOBOBJECT_ASSOCIATE_COMPLETION_PORT))
            {
                Status = STATUS_INFO_LENGTH_MISMATCH;
                break;
            }

            if (!JobObject->CompletionPort && CompletionPortInfo.CompletionPort)
            {
                /* Get the new completion port object */
                Status = ObReferenceObjectByHandle(CompletionPortInfo.CompletionPort,
                                                   IO_COMPLETION_MODIFY_STATE,
                                                   IoCompletionType,
                                                   PreviousMode,
                                                   &IoCompletionPtr,
                                                   NULL);
                if (NT_SUCCESS(Status))
                {
                    JobObject->CompletionPort = IoCompletionPtr;
                    JobObject->CompletionKey = CompletionPortInfo.CompletionKey;
                }
            }
            else
            {
                Status = STATUS_INVALID_PARAMETER;
            }
            break;
        }
        case JobObjectBasicLimitInformation:
        case JobObjectExtendedLimitInformation:
        {
            JOBOBJECT_EXTENDED_LIMIT_INFORMATION ExtendedLimit = { { { 0 } } };
            ULONG Size = sizeof(JOBOBJECT_EXTENDED_LIMIT_INFORMATION);
            ULONG ValidFlags = JOB_OBJECT_EXTENDED_LIMIT_VALID_FLAGS;

            if (JobInformationClass == JobObjectBasicLimitInformation)
            {
                Size = sizeof(JOBOBJECT_BASIC_LIMIT_INFORMATION);
                ValidFlags = JOB_OBJECT_BASIC_LIMIT_VALID_FLAGS;
            }

            _SEH2_TRY
            {
                RtlCopyMemory(&ExtendedLimit, JobInformation, Size);
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                /* Get the exception code */
                Status = _SEH2_GetExceptionCode();
                _SEH2_YIELD(break);
            }
            _SEH2_END;

            if (ExtendedLimit.BasicLimitInformation.LimitFlags & ~ValidFlags)
            {
                DPRINT1("NtSetInformationJobObject(Class=%u, LimitFlags=0x%x) INVALID FLAGS\n",
                        JobInformationClass, ExtendedLimit.BasicLimitInformation.LimitFlags & ~ValidFlags);
                Status = STATUS_INVALID_PARAMETER;
                break;
            }

            if (ExtendedLimit.BasicLimitInformation.LimitFlags & ~JOB_SUPPORTED_LIMIT_FLAGS)
            {
                DPRINT1("NtSetInformationJobObject(Class=%u, LimitFlags=0x%x) UNHANDLED\n",
                        JobInformationClass, ExtendedLimit.BasicLimitInformation.LimitFlags & ~JOB_SUPPORTED_LIMIT_FLAGS);
                Status = STATUS_INVALID_PARAMETER;
            }
            else
            {
                JobObject->LimitFlags = ExtendedLimit.BasicLimitInformation.LimitFlags;
                if (ExtendedLimit.BasicLimitInformation.LimitFlags & JOB_OBJECT_LIMIT_ACTIVE_PROCESS)
                {
                    JobObject->ActiveProcessLimit = ExtendedLimit.BasicLimitInformation.ActiveProcessLimit;
                    /* FIXME: What to do if we are already over this?? */
                }
            }

            break;
        }
        default:
        {
            DPRINT1("Unhandled info class 0x%x\n", JobInformationClass);
            Status = STATUS_NOT_IMPLEMENTED;
            break;
        }
    }

    /* resume APCs and release lock */
    ExReleaseResourceAndLeaveCriticalRegion(&JobObject->JobLock);

    /* Release reference to Job */
    ObDereferenceObject(JobObject);

    return Status;
}


/*
 * @implemented
 */
NTSTATUS
NTAPI
NtTerminateJobObject(HANDLE JobHandle,
                     NTSTATUS ExitStatus)
{
    KPROCESSOR_MODE PreviousMode;
    PEJOB Job;
    NTSTATUS Status;

    PAGED_CODE();

    DPRINT("NtTerminateJobObject(%x,%x)\n", JobHandle, ExitStatus);
    PreviousMode = ExGetPreviousMode();

    Status = ObReferenceObjectByHandle(JobHandle,
                                       JOB_OBJECT_TERMINATE,
                                       PsJobType,
                                       PreviousMode,
                                       (PVOID*)&Job,
                                       NULL);
    if (NT_SUCCESS(Status))
    {
        Status = PspTerminateJobObject(Job, PreviousMode, ExitStatus);
        ObDereferenceObject(Job);
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

/* EOF */
