/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ps/job.c
 * PURPOSE:         Job Native Functions
 * PROGRAMMERS:     Alex Ionescu (alex@relsoft.net) (stubs)
 *                  Thomas Weidenmueller <w3seek@reactos.com>
 *                  Pierre Schweitzer (pierre@reactos.org)
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

VOID
NTAPI
PspDeleteJob ( PVOID ObjectBody )
{
    PEJOB Job = (PEJOB)ObjectBody;

    /* remove the reference to the completion port if associated */
    if(Job->CompletionPort != NULL)
    {
        ObDereferenceObject(Job->CompletionPort);
    }

    /* unlink the job object */
    if(Job->JobLinks.Flink != NULL)
    {
        ExAcquireFastMutex(&PsJobListLock);
        RemoveEntryList(&Job->JobLinks);
        ExReleaseFastMutex(&PsJobListLock);
    }

    ExDeleteResource(&Job->JobLock);
}

CODE_SEG("INIT")
VOID
NTAPI
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
    DPRINT("PspAssignProcessToJob() is unimplemented!\n");
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
PspTerminateJobObject(PEJOB Job,
    KPROCESSOR_MODE AccessMode,
    NTSTATUS ExitStatus )
{
    DPRINT("PspTerminateJobObject() is unimplemented!\n");
    return STATUS_NOT_IMPLEMENTED;
}

VOID
NTAPI
PspRemoveProcessFromJob(IN PEPROCESS Process,
                        IN PEJOB Job)
{
    /* FIXME */
}

VOID
NTAPI
PspExitProcessFromJob(IN PEJOB Job,
                      IN PEPROCESS Process)
{
    /* FIXME */
}

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
NtAssignProcessToJobObject (
    HANDLE JobHandle,
    HANDLE ProcessHandle)
{
    PEPROCESS Process;
    KPROCESSOR_MODE PreviousMode;
    NTSTATUS Status;

    PAGED_CODE();

    PreviousMode = ExGetPreviousMode();

    /* make sure we're having a handle with enough rights, especially the to
    terminate the process. otherwise one could abuse the job objects to
    terminate processes without having rights granted to do so! The reason
    I open the process handle before the job handle is that a simple test showed
    that it first complains about a invalid process handle! The other way around
    would be simpler though... */
    Status = ObReferenceObjectByHandle(
        ProcessHandle,
        PROCESS_TERMINATE,
        PsProcessType,
        PreviousMode,
        (PVOID*)&Process,
        NULL);
    if(NT_SUCCESS(Status))
    {
        if(Process->Job == NULL)
        {
            PEJOB Job;

            Status = ObReferenceObjectByHandle(
                JobHandle,
                JOB_OBJECT_ASSIGN_PROCESS,
                PsJobType,
                PreviousMode,
                (PVOID*)&Job,
                NULL);
            if(NT_SUCCESS(Status))
            {
                /* lock the process so we can safely assign the process. Note that in the
                meanwhile another thread could have assigned this process to a job! */

                if(ExAcquireRundownProtection(&Process->RundownProtect))
                {
                    if(Process->Job == NULL && PsGetProcessSessionId(Process) == Job->SessionId)
                    {
                        /* Just store the pointer to the job object in the process, we'll
                        assign it later. The reason we can't do this here is that locking
                        the job object might require it to wait, which is a bad thing
                        while holding the process lock! */
                        Process->Job = Job;
                        ObReferenceObject(Job);
                    }
                    else
                    {
                        /* process is already assigned to a job or session id differs! */
                        Status = STATUS_ACCESS_DENIED;
                    }
                    ExReleaseRundownProtection(&Process->RundownProtect);

                    if(NT_SUCCESS(Status))
                    {
                        /* let's actually assign the process to the job as we're not holding
                        the process lock anymore! */
                        Status = PspAssignProcessToJob(Process, Job);
                    }
                }

                ObDereferenceObject(Job);
            }
        }
        else
        {
            /* process is already assigned to a job or session id differs! */
            Status = STATUS_ACCESS_DENIED;
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
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
NtCreateJobObject (
    PHANDLE JobHandle,
    ACCESS_MASK DesiredAccess,
    POBJECT_ATTRIBUTES ObjectAttributes )
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

    if(NT_SUCCESS(Status))
    {
        /* FIXME - Zero all fields as we don't yet implement all of them */
        RtlZeroMemory(Job, sizeof(EJOB));

        /* make sure that early destruction doesn't attempt to remove the object from
        the list before it even gets added! */
        Job->JobLinks.Flink = NULL;

        /* setup the job object - FIXME: More to do! */
        InitializeListHead(&Job->JobSetLinks);
        InitializeListHead(&Job->ProcessListHead);

        /* inherit the session id from the caller */
        Job->SessionId = PsGetProcessSessionId(CurrentProcess);

        KeInitializeGuardedMutex(&Job->MemoryLimitsLock);

        Status = ExInitializeResource(&Job->JobLock);
        if(!NT_SUCCESS(Status))
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
        if(NT_SUCCESS(Status))
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
NtIsProcessInJob (
    IN HANDLE ProcessHandle,
    IN HANDLE JobHandle OPTIONAL )
{
    KPROCESSOR_MODE PreviousMode;
    PEPROCESS Process;
    NTSTATUS Status;

    PreviousMode = ExGetPreviousMode();

    PAGED_CODE();

    Status = ObReferenceObjectByHandle(
        ProcessHandle,
        PROCESS_QUERY_INFORMATION,
        PsProcessType,
        PreviousMode,
        (PVOID*)&Process,
        NULL);
    if(NT_SUCCESS(Status))
    {
        /* FIXME - make sure the job object doesn't get exchanged or deleted while trying to
        reference it, e.g. by locking it somehow until it is referenced... */

        PEJOB ProcessJob = Process->Job;

        if(ProcessJob != NULL)
        {
            if(JobHandle == NULL)
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
                if(NT_SUCCESS(Status))
                {
                    Status = ((ProcessJob == JobObject) ? STATUS_PROCESS_IN_JOB : STATUS_PROCESS_NOT_IN_JOB);
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
NtOpenJobObject (
    PHANDLE JobHandle,
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
    if(NT_SUCCESS(Status))
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


/*
 * @unimplemented
 */
NTSTATUS
NTAPI
NtTerminateJobObject (
    HANDLE JobHandle,
    NTSTATUS ExitStatus )
{
    KPROCESSOR_MODE PreviousMode;
    PEJOB Job;
    NTSTATUS Status;

    PAGED_CODE();

    PreviousMode = ExGetPreviousMode();

    Status = ObReferenceObjectByHandle(
        JobHandle,
        JOB_OBJECT_TERMINATE,
        PsJobType,
        PreviousMode,
        (PVOID*)&Job,
        NULL);
    if(NT_SUCCESS(Status))
    {
        Status = PspTerminateJobObject(
            Job,
            PreviousMode,
            ExitStatus);
        ObDereferenceObject(Job);
    }

    return Status;
}


/*
 * @implemented
 */
PVOID
NTAPI
PsGetJobLock ( PEJOB Job )
{
    ASSERT(Job);
    return (PVOID)&Job->JobLock;
}


/*
 * @implemented
 */
ULONG
NTAPI
PsGetJobSessionId ( PEJOB Job )
{
    ASSERT(Job);
    return Job->SessionId;
}


/*
 * @implemented
 */
ULONG
NTAPI
PsGetJobUIRestrictionsClass ( PEJOB Job )
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
