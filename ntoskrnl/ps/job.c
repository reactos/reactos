/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ps/job.c
 * PURPOSE:         Job Native Functions
 * PROGRAMMERS:     Alex Ionescu (alex@relsoft.net) (stubs)
 *                  Thomas Weidenmueller <w3seek@reactos.com>
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>


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
    STANDARD_RIGHTS_WRITE | JOB_OBJECT_ASSIGN_PROCESS | JOB_OBJECT_SET_ATTRIBUTES | JOB_OBJECT_TERMINATE | JOB_OBJECT_SET_SECURITY_ATTRIBUTES,
    STANDARD_RIGHTS_EXECUTE | SYNCHRONIZE,
    STANDARD_RIGHTS_ALL | JOB_OBJECT_ALL_ACCESS
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
STDCALL
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

                ExAcquireRundownProtection(&Process->RundownProtect);
                if(NT_SUCCESS(Status))
                {
                    if(Process->Job == NULL && Process->Session == Job->SessionId)
                    {
                        /* Just store the pointer to the job object in the process, we'll
                        assign it later. The reason we can't do this here is that locking
                        the job object might require it to wait, which is a bad thing
                        while holding the process lock! */
                        Process->Job = Job;
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
STDCALL
NtCreateJobObject (
    PHANDLE JobHandle,
    ACCESS_MASK DesiredAccess,
    POBJECT_ATTRIBUTES ObjectAttributes )
{
    HANDLE hJob;
    PEJOB Job;
    KPROCESSOR_MODE PreviousMode;
    PEPROCESS CurrentProcess;
    NTSTATUS Status = STATUS_SUCCESS;

    PAGED_CODE();

    PreviousMode = ExGetPreviousMode();
    CurrentProcess = PsGetCurrentProcess();

    /* check for valid buffers */
    if(PreviousMode != KernelMode)
    {
        _SEH_TRY
        {
            ProbeForWriteHandle(JobHandle);
        }
        _SEH_HANDLE
        {
            Status = _SEH_GetExceptionCode();
        }
        _SEH_END;

        if(!NT_SUCCESS(Status))
        {
            return Status;
        }
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

        /* setup the job object */
        InitializeListHead(&Job->ProcessListHead);
        Job->SessionId = CurrentProcess->Session; /* inherit the session id from the caller */

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
            _SEH_TRY
            {
                /* NOTE: if the caller passed invalid buffers to receive the handle it's his
                own fault! the object will still be created and live... It's possible
                to find the handle using ObFindHandleForObject()! */
                *JobHandle = hJob;
            }
            _SEH_HANDLE
            {
                Status = _SEH_GetExceptionCode();
            }
            _SEH_END;
        }
    }

    return Status;
}


/*
 * @implemented
 */
NTSTATUS
STDCALL
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
STDCALL
NtOpenJobObject (
    PHANDLE JobHandle,
    ACCESS_MASK DesiredAccess,
    POBJECT_ATTRIBUTES ObjectAttributes)
{
    KPROCESSOR_MODE PreviousMode;
    HANDLE hJob;
    NTSTATUS Status = STATUS_SUCCESS;

    PAGED_CODE();

    PreviousMode = ExGetPreviousMode();

    /* check for valid buffers */
    if(PreviousMode != KernelMode)
    {
        _SEH_TRY
        {
            ProbeForWriteHandle(JobHandle);
        }
        _SEH_HANDLE
        {
            Status = _SEH_GetExceptionCode();
        }
        _SEH_END;

        if(!NT_SUCCESS(Status))
        {
            return Status;
        }
    }

    if(NT_SUCCESS(Status))
    {
        Status = ObOpenObjectByName(ObjectAttributes,
            PsJobType,
            PreviousMode,
            NULL,
            DesiredAccess,
            NULL,
            &hJob);
        if(NT_SUCCESS(Status))
        {
            _SEH_TRY
            {
                *JobHandle = hJob;
            }
            _SEH_HANDLE
            {
                Status = _SEH_GetExceptionCode();
            }
            _SEH_END;
        }
    }

    return Status;
}


/*
 * @unimplemented
 */
NTSTATUS
STDCALL
NtQueryInformationJobObject (
    HANDLE JobHandle,
    JOBOBJECTINFOCLASS JobInformationClass,
    PVOID JobInformation,
    ULONG JobInformationLength,
    PULONG ReturnLength )
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/*
 * @unimplemented
 */
NTSTATUS
STDCALL
NtSetInformationJobObject (
    HANDLE JobHandle,
    JOBOBJECTINFOCLASS JobInformationClass,
    PVOID JobInformation,
    ULONG JobInformationLength)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/*
 * @unimplemented
 */
NTSTATUS
STDCALL
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
STDCALL
PsGetJobLock ( PEJOB Job )
{
    ASSERT(Job);
    return (PVOID)&Job->JobLock;
}


/*
 * @implemented
 */
PVOID
STDCALL
PsGetJobSessionId ( PEJOB Job )
{
    ASSERT(Job);
    return (PVOID)Job->SessionId;
}


/*
 * @implemented
 */
ULONG
STDCALL
PsGetJobUIRestrictionsClass ( PEJOB Job )
{
    ASSERT(Job);
    return Job->UIRestrictionsClass;
}


/*
 * @unimplemented
 */
VOID
STDCALL
PsSetJobUIRestrictionsClass(PEJOB Job,
    ULONG UIRestrictionsClass)
{
    ASSERT(Job);
    (void)InterlockedExchangeUL(&Job->UIRestrictionsClass, UIRestrictionsClass);
    /* FIXME - walk through the job process list and update the restrictions? */
}

/* EOF */
