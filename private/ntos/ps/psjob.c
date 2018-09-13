/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    psjob.c

Abstract:

    This module implements bulk of the job object support

Author:

    Mark Lucovsky (markl) 22-May-1997

Revision History:

--*/

#include "psp.h"
#include "winerror.h"

#pragma alloc_text(PAGE, PspJobDelete)
#pragma alloc_text(PAGE, PspJobClose)
#pragma alloc_text(PAGE, NtCreateJobObject)
#pragma alloc_text(PAGE, NtOpenJobObject)
#pragma alloc_text(PAGE, NtAssignProcessToJobObject)
#pragma alloc_text(PAGE, PspAddProcessToJob)
#pragma alloc_text(PAGE, PspRemoveProcessFromJob)
#pragma alloc_text(PAGE, PspExitProcessFromJob)
#pragma alloc_text(PAGE, NtQueryInformationJobObject)
#pragma alloc_text(PAGE, NtSetInformationJobObject)
#pragma alloc_text(PAGE, PspApplyJobLimitsToProcessSet)
#pragma alloc_text(PAGE, PspApplyJobLimitsToProcess)
#pragma alloc_text(PAGE, NtTerminateJobObject)
#pragma alloc_text(PAGE, PspTerminateAllProcessesInJob)
#pragma alloc_text(PAGE, PspFoldProcessAccountingIntoJob)
#pragma alloc_text(PAGE, PspCaptureTokenFilter)

//
// move to io.h
extern POBJECT_TYPE IoCompletionObjectType;


NTSTATUS
NTAPI
NtCreateJobObject (
    OUT PHANDLE JobHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL
    )
{

    PEJOB Job;
    HANDLE Handle;
    KPROCESSOR_MODE PreviousMode;
    NTSTATUS Status;

    PAGED_CODE();

    //
    // Establish an exception handler, probe the output handle address, and
    // attempt to create a job object. If the probe fails, then return the
    // exception code as the service status. Otherwise return the status value
    // returned by the object insertion routine.
    //

    try {

        //
        // Get previous processor mode and probe output handle address if
        // necessary.
        //

        PreviousMode = KeGetPreviousMode();
        if (PreviousMode != KernelMode) {
            ProbeForWriteHandle(JobHandle);
            }

        //
        // Allocate job object.
        //

        Status = ObCreateObject(
                    PreviousMode,
                    PsJobType,
                    ObjectAttributes,
                    PreviousMode,
                    NULL,
                    sizeof(EJOB),
                    0,
                    0,
                    (PVOID *)&Job
                    );

        //
        // If the job object was successfully allocated, then initialize it
        // and attempt to insert the job object in the current
        // process' handle table.
        //

        if (NT_SUCCESS(Status)) {

            RtlZeroMemory(Job,sizeof(EJOB));
            InitializeListHead(&Job->ProcessListHead);
            KeInitializeEvent(&Job->Event,NotificationEvent,FALSE);

            //
            // Job Object gets the SessionId of the Process creating the Job
            // We will use this sessionid to restrict the processes that can
            // be added to a job.
            //
            Job->SessionId = PsGetCurrentProcess()->SessionId;

            //
            // Initialize the scheduling class for the Job
            //
            Job->SchedulingClass = PSP_DEFAULT_SCHEDULING_CLASSES;

            //
            // Insert Job on Job List
            //

            ExAcquireFastMutex(&PspJobListLock);
            InsertTailList(&PspJobList,&Job->JobLinks);
            ExReleaseFastMutex(&PspJobListLock);

            Status = ExInitializeResource(&Job->JobLock);
            if ( !NT_SUCCESS(Status) ) {

                //
                // Note that ExInitializeResource really can't fail and
                // is hard coded to return success.
                //
                ObDereferenceObject(Job);

                }
            else {
                ExInitializeFastMutex(&Job->MemoryLimitsLock);
                Status = ObInsertObject(
                            (PVOID)Job,
                            NULL,
                            DesiredAccess,
                            0,
                            (PVOID *)NULL,
                            &Handle
                            );
                }

            //
            // If the job object was successfully inserted in the current
            // process' handle table, then attempt to write the job object
            // handle value. If the write attempt fails, then do not report
            // an error. When the caller attempts to access the handle value,
            // an access violation will occur.
            //

            if (NT_SUCCESS(Status)) {
                try {
                    *JobHandle = Handle;
                    }
                except(ExSystemExceptionFilter()) {
                    }
                }
            }

        //
        // If an exception occurs during the probe of the output handle address,
        // then always handle the exception and return the exception code as the
        // status value.
        //

        }
    except(ExSystemExceptionFilter()) {
        return GetExceptionCode();
        }

    //
    // Return service status.
    //

    return Status;
}

NTSTATUS
NTAPI
NtOpenJobObject(
    OUT PHANDLE JobHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes
    )
{
    HANDLE Handle;
    KPROCESSOR_MODE PreviousMode;
    NTSTATUS Status;

    PAGED_CODE();

    //
    // Establish an exception handler, probe the output handle address, and
    // attempt to open the job object. If the probe fails, then return the
    // exception code as the service status. Otherwise return the status value
    // returned by the object open routine.
    //

    try {

        //
        // Get previous processor mode and probe output handle address
        // if necessary.
        //

        PreviousMode = KeGetPreviousMode();
        if (PreviousMode != KernelMode) {
            ProbeForWriteHandle(JobHandle);
            }

        //
        // Open handle to the event object with the specified desired access.
        //

        Status = ObOpenObjectByName(
                    ObjectAttributes,
                    PsJobType,
                    PreviousMode,
                    NULL,
                    DesiredAccess,
                    NULL,
                    &Handle
                    );

        //
        // If the open was successful, then attempt to write the job object
        // handle value. If the write attempt fails, then do not report an
        // error. When the caller attempts to access the handle value, an
        // access violation will occur.
        //

        if (NT_SUCCESS(Status)) {
            try {
                *JobHandle = Handle;
                }
            except(ExSystemExceptionFilter()) {
                }
            }

        }

    except(ExSystemExceptionFilter()) {

        //
        // If an exception occurs during the probe of the output job handle,
        // then always handle the exception and return the exception code as the
        // status value.
        //

        Status = GetExceptionCode();
        }

    return Status;
}

NTSTATUS
NTAPI
NtAssignProcessToJobObject(
    IN HANDLE JobHandle,
    IN HANDLE ProcessHandle
    )
{
    PEJOB Job;
    PEPROCESS Process;
    NTSTATUS Status;
    KPROCESSOR_MODE PreviousMode;
    BOOLEAN IsAdmin ;

    PAGED_CODE();

    PreviousMode = KeGetPreviousMode();

    //
    // Reference the process object, lock the process, test for already been assigned
    //

    Status = ObReferenceObjectByHandle(
                ProcessHandle,
                PROCESS_SET_QUOTA | PROCESS_TERMINATE,
                PsProcessType,
                PreviousMode,
                (PVOID *)&Process,
                NULL
                );
    if ( !NT_SUCCESS(Status) ) {
        return Status;
        }

    //
    // Quick Check for prior assignment
    //

    if ( Process->Job ) {
        ObDereferenceObject(Process);
        return STATUS_ACCESS_DENIED;
        }


    //
    // Now reference the job object. Then we need to lock the process and check again
    //

    Status = ObReferenceObjectByHandle(
                JobHandle,
                JOB_OBJECT_ASSIGN_PROCESS,
                PsJobType,
                PreviousMode,
                (PVOID *)&Job,
                NULL
                );
    if ( !NT_SUCCESS(Status) ) {
        ObDereferenceObject(Process);
        return Status;
        }

    //
    // We only allow a process that is running in the Job creator's hydra session
    // to be assigned to the job.
    //

    if ( Process->SessionId != Job->SessionId ) {

        ObDereferenceObject(Process);
        ObDereferenceObject(Job);
        return STATUS_ACCESS_DENIED;

        }


    Status = PsLockProcess(Process,PreviousMode,PsLockPollOnTimeout);
    if ( !NT_SUCCESS(Status) || Process->Job ) {
        if ( !NT_SUCCESS(Status) ) {
            Status = STATUS_PROCESS_IS_TERMINATING;
            }
        else {
            Status = STATUS_ACCESS_DENIED;
            PsUnlockProcess(Process);
            }
        ObDereferenceObject(Process);
        ObDereferenceObject(Job);
        return Status ;
        }

    //
    // Security Rules:  If the job has no-admin set, and it is running
    // as admin, that's not allowed
    //

    if ( Job->SecurityLimitFlags & JOB_OBJECT_SECURITY_NO_ADMIN )
    {
        PsLockProcessSecurityFields();

        IsAdmin = SeTokenIsAdmin( Process->Token );

        PsFreeProcessSecurityFields();

        if ( IsAdmin )
        {
            Status = STATUS_ACCESS_DENIED ;

            PsUnlockProcess( Process );

            ObDereferenceObject( Process );

            ObDereferenceObject( Job );

            return Status ;
        }

    }

    //
    // If the job has a token filter established,
    // use it to filter the

    //
    // ref the job for the process
    //

    ObReferenceObject(Job);

    Process->Job = Job;

    PsUnlockProcess(Process);

    Status = PspAddProcessToJob(Job,Process);
    if ( !NT_SUCCESS(Status) ) {
        Job->TotalTerminatedProcesses++;
        PspTerminateProcess(Process,ERROR_NOT_ENOUGH_QUOTA,PsLockPollOnTimeout);
        }

    //
    // If the job has UI restrictions and this is a GUI process, call ntuser
    //
    if ( ( Job->UIRestrictionsClass != JOB_OBJECT_UILIMIT_NONE ) &&
         ( Process->Win32Process != NULL ) ) {
        WIN32_JOBCALLOUT_PARAMETERS Parms;

        KeEnterCriticalRegion();
        ExAcquireResourceExclusive(&Job->JobLock, TRUE);

        Parms.Job = Job;
        Parms.CalloutType = PsW32JobCalloutAddProcess;
        Parms.Data = Process->Win32Process;
        MmDispatchWin32Callout( PspW32JobCallout,NULL, (PVOID)&Parms, &(Job->SessionId));

        ExReleaseResource(&Job->JobLock);
        KeLeaveCriticalRegion();

        }

    if ( Job->SecurityLimitFlags & JOB_OBJECT_SECURITY_ONLY_TOKEN )
    {
        Status = PspSetPrimaryToken( ProcessHandle, NULL, Job->Token );

        if ( !NT_SUCCESS( Status ) )
        {
            //
            // What?
            //
        }
    }

    ObDereferenceObject(Process);
    ObDereferenceObject(Job);

    return Status;
}

NTSTATUS
PspAddProcessToJob(
    PEJOB Job,
    PEPROCESS Process
    )
{

    NTSTATUS Status;
    SIZE_T MinWs,MaxWs;

    PAGED_CODE();


    KeEnterCriticalRegion();
    ExAcquireResourceExclusive(&Job->JobLock, TRUE);

    InsertTailList(&Job->ProcessListHead,&Process->JobLinks);

    //
    // Update relevant ADD accounting info.
    //

    Job->TotalProcesses++;
    Job->ActiveProcesses++;



    //
    // Test for active process count exceeding limit
    //

    Status = STATUS_SUCCESS;
    if ( Job->LimitFlags & JOB_OBJECT_LIMIT_ACTIVE_PROCESS &&
         Job->ActiveProcesses > Job->ActiveProcessLimit ) {

        PS_SET_CLEAR_BITS (&Process->JobStatus,
                           PS_JOB_STATUS_NOT_REALLY_ACTIVE | PS_JOB_STATUS_ACCOUNTING_FOLDED,
                           PS_JOB_STATUS_LAST_REPORT_MEMORY);

        Job->ActiveProcesses--;

        if ( Job->CompletionPort ) {
            IoSetIoCompletion(
                Job->CompletionPort,
                Job->CompletionKey,
                NULL,
                STATUS_SUCCESS,
                JOB_OBJECT_MSG_ACTIVE_PROCESS_LIMIT,
                TRUE
                );
            }

        Status = STATUS_QUOTA_EXCEEDED;
        }

    if ( Job->LimitFlags & JOB_OBJECT_LIMIT_JOB_TIME && KeReadStateEvent(&Job->Event) ) {
        PS_SET_BITS (&Process->JobStatus, PS_JOB_STATUS_NOT_REALLY_ACTIVE | PS_JOB_STATUS_ACCOUNTING_FOLDED);

        Job->ActiveProcesses--;

        Status = STATUS_QUOTA_EXCEEDED;
        }

    if ( Status == STATUS_SUCCESS ) {

        PspApplyJobLimitsToProcess(Job,Process);

        if ( Process->Job->CompletionPort
             && Process->UniqueProcessId
             && !(Process->JobStatus & PS_JOB_STATUS_NOT_REALLY_ACTIVE)
             && !(Process->JobStatus & PS_JOB_STATUS_NEW_PROCESS_REPORTED)) {

            PS_SET_CLEAR_BITS (&Process->JobStatus,
                               PS_JOB_STATUS_NEW_PROCESS_REPORTED,
                               PS_JOB_STATUS_LAST_REPORT_MEMORY);

            IoSetIoCompletion(
                Job->CompletionPort,
                Job->CompletionKey,
                (PVOID)Process->UniqueProcessId,
                STATUS_SUCCESS,
                JOB_OBJECT_MSG_NEW_PROCESS,
                FALSE
                );
            }

        }

    if ( Job->LimitFlags & JOB_OBJECT_LIMIT_WORKINGSET ) {
        MinWs = Job->MinimumWorkingSetSize;
        MaxWs = Job->MaximumWorkingSetSize;
        }
    else {
        MinWs = 0;
        MaxWs = 0;
        }

    ExReleaseResource(&Job->JobLock);

    KeLeaveCriticalRegion();

    if ( Status == STATUS_SUCCESS ) {

        if ( MinWs != 0 && MaxWs != 0 ) {

            KeEnterCriticalRegion();
            ExAcquireFastMutexUnsafe(&PspWorkingSetChangeHead.Lock);

            KeAttachProcess (&Process->Pcb);
            MmAdjustWorkingSetSize(MinWs,MaxWs,FALSE);

            //
            // call MM to Enable hard workingset
            //

            MmEnforceWorkingSetLimit(&Process->Vm, TRUE);

            KeDetachProcess();

            ExReleaseFastMutexUnsafe(&PspWorkingSetChangeHead.Lock);
            KeLeaveCriticalRegion();

            }
        else {
            MmEnforceWorkingSetLimit(&Process->Vm, FALSE);
            }

        if ( !MmAssignProcessToJob(Process) ) {
            Status = STATUS_QUOTA_EXCEEDED;
            }

        }

    return Status;
}

//
// Only callable from process delete routine !
// This means that if the above fails, failure is termination of the process !
//
VOID
PspRemoveProcessFromJob(
    PEJOB Job,
    PEPROCESS Process
    )
{
    PAGED_CODE();

    KeEnterCriticalRegion();
    ExAcquireResourceExclusive(&Job->JobLock, TRUE);

    RemoveEntryList(&Process->JobLinks);

    //
    // Update REMOVE accounting info
    //


    if ( !(Process->JobStatus & PS_JOB_STATUS_NOT_REALLY_ACTIVE) ) {
        Job->ActiveProcesses--;
        PS_SET_BITS (&Process->JobStatus, PS_JOB_STATUS_NOT_REALLY_ACTIVE);
        }

    PspFoldProcessAccountingIntoJob(Job,Process);

    ExReleaseResource(&Job->JobLock);
    KeLeaveCriticalRegion();
}

VOID
PspExitProcessFromJob(
    PEJOB Job,
    PEPROCESS Process
    )
{

    PAGED_CODE();

    KeEnterCriticalRegion();
    ExAcquireResourceExclusive(&Job->JobLock, TRUE);

    //
    // Update REMOVE accounting info
    //


    if ( !(Process->JobStatus & PS_JOB_STATUS_NOT_REALLY_ACTIVE) ) {
        Job->ActiveProcesses--;
        PS_SET_BITS (&Process->JobStatus, PS_JOB_STATUS_NOT_REALLY_ACTIVE);
        }

    PspFoldProcessAccountingIntoJob(Job,Process);

    ExReleaseResource(&Job->JobLock);
    KeLeaveCriticalRegion();
}

VOID
PspJobDelete(
    IN PVOID Object
    )
{
    PEJOB Job;
    WIN32_JOBCALLOUT_PARAMETERS Parms;

    PAGED_CODE();

    Job = (PEJOB) Object;

    //
    // call ntuser to delete its job structure
    //

    KeEnterCriticalRegion();
    ExAcquireResourceExclusive(&Job->JobLock, TRUE);


    Parms.Job = Job;
    Parms.CalloutType = PsW32JobCalloutTerminate;
    Parms.Data = NULL;
    MmDispatchWin32Callout( PspW32JobCallout,NULL, (PVOID)&Parms, &(Job->SessionId));

    ExReleaseResource(&Job->JobLock);
    KeLeaveCriticalRegion();

    Job->LimitFlags = 0;

    if ( Job->CompletionPort ) {
        ObDereferenceObject(Job->CompletionPort);
        Job->CompletionPort = NULL;
        }


    //
    // Remove Job on Job List
    //

    ExAcquireFastMutex(&PspJobListLock);
    RemoveEntryList(&Job->JobLinks);
    ExReleaseFastMutex(&PspJobListLock);

    //
    // Free Security clutter:
    //

    if ( Job->Token ) {
        ObDereferenceObject( Job->Token );
        Job->Token = NULL ;
        }

    if ( Job->Filter ) {
        if ( Job->Filter->CapturedSids ) {
            ExFreePool( Job->Filter->CapturedSids );
            }

        if ( Job->Filter->CapturedPrivileges ) {
            ExFreePool( Job->Filter->CapturedPrivileges );
            }

        if ( Job->Filter->CapturedGroups ) {
            ExFreePool( Job->Filter->CapturedGroups );
            }

        ExFreePool( Job->Filter );

        }

    ExDeleteResource(&Job->JobLock);
}

VOID
PspJobClose (
    IN PEPROCESS Process,
    IN PVOID Object,
    IN ACCESS_MASK GrantedAccess,
    IN ULONG ProcessHandleCount,
    IN ULONG SystemHandleCount
    )
/*++

Routine Description:

    Called by the object manager when a handle is closed to the object.

Arguments:

    Process - Process doing the close
    Object - Job object being closed
    GrantedAccess - Access ranted for this handle
    ProcessHandleCount - Unused and unmaintained by OB
    SystemHandleCount - Current handle count for this object

Return Value:

    None.

--*/
{
    PEJOB Job = Object;
    PVOID Port;

    PAGED_CODE ();
    //
    // If this isn't the last handle then do nothing.
    //
    if (SystemHandleCount > 1) {
        return;
    }

    KeEnterCriticalRegion();
    ExAcquireResourceExclusive(&Job->JobLock, TRUE);
    ExAcquireFastMutexUnsafe(&Job->MemoryLimitsLock);

    //
    // Release the completion port
    //
    Port = Job->CompletionPort;
    Job->CompletionPort = NULL;
    

    ExReleaseFastMutexUnsafe(&Job->MemoryLimitsLock);
    ExReleaseResource(&Job->JobLock);
    KeLeaveCriticalRegion();

    if (Port != NULL) {
        ObDereferenceObject(Port);
    }
}

ULONG PspJobInfoLengths[] = {
    sizeof(JOBOBJECT_BASIC_ACCOUNTING_INFORMATION),         // JobObjectBasicAccountingInformation
    sizeof(JOBOBJECT_BASIC_LIMIT_INFORMATION),              // JobObjectBasicLimitInformation
    sizeof(JOBOBJECT_BASIC_PROCESS_ID_LIST),                // JobObjectBasicProcessIdList
    sizeof(JOBOBJECT_BASIC_UI_RESTRICTIONS),                // JobObjectBasicUIRestrictions
    sizeof(JOBOBJECT_SECURITY_LIMIT_INFORMATION),           // JobObjectSecurityLimitInformation
    sizeof(JOBOBJECT_END_OF_JOB_TIME_INFORMATION),          // JobObjectEndOfJobTimeInformation
    sizeof(JOBOBJECT_ASSOCIATE_COMPLETION_PORT),            // JobObjectAssociateCompletionPortInformation
    sizeof(JOBOBJECT_BASIC_AND_IO_ACCOUNTING_INFORMATION),  // JobObjectBasicAndIoAccountingInformation
    sizeof(JOBOBJECT_EXTENDED_LIMIT_INFORMATION),           // JobObjectExtendedLimitInformation
    0
    };

#if defined(_ALPHA_)

//
// Alpha's run with aligned stacks, so we can require quad alignment
//

ULONG PspJobInfoAlign[] = {
    sizeof(LARGE_INTEGER),                          // JobObjectBasicAccountingInformation
    sizeof(LARGE_INTEGER),                          // JobObjectBasicLimitInformation
    sizeof(ULONG),                                  // JobObjectBasicProcessIdList
    sizeof(ULONG),                                  // JobObjectBasicUIRestrictions
    sizeof(ULONG),                                  // JobObjectSecurityLimitInformation
    sizeof(ULONG),                                  // JobObjectEndOfJobTimeInformation
    sizeof(PVOID),                                  // JobObjectAssociateCompletionPortInformation
    sizeof(LARGE_INTEGER),                          // JobObjectBasicAndIoAccountingInformation
    sizeof(LARGE_INTEGER),                          // JobObjectExtendedLimitInformation
    0
    };
#else
ULONG PspJobInfoAlign[] = {
    sizeof(ULONG),                                  // JobObjectBasicAccountingInformation
    sizeof(ULONG),                                  // JobObjectBasicLimitInformation
    sizeof(ULONG),                                  // JobObjectBasicProcessIdList
    sizeof(ULONG),                                  // JobObjectBasicUIRestrictions
    sizeof(ULONG),                                  // JobObjectSecurityLimitInformation
    sizeof(ULONG),                                  // JobObjectEndOfJobTimeInformation
    sizeof(PVOID),                                  // JobObjectAssociateCompletionPortInformation
    sizeof(ULONG),                                  // JobObjectBasicAndIoAccountingInformation
    sizeof(ULONG),                                  // JobObjectExtendedLimitInformation
    0
    };
#endif // _ALPHA_

NTSTATUS
NtQueryInformationJobObject(
    IN HANDLE JobHandle,
    IN JOBOBJECTINFOCLASS JobObjectInformationClass,
    OUT PVOID JobObjectInformation,
    IN ULONG JobObjectInformationLength,
    OUT PULONG ReturnLength OPTIONAL
    )
{
    PEJOB Job;
    KPROCESSOR_MODE PreviousMode;
    JOBOBJECT_BASIC_AND_IO_ACCOUNTING_INFORMATION AccountingInfo;
    JOBOBJECT_BASIC_LIMIT_INFORMATION BasicLimitInfo;
    JOBOBJECT_EXTENDED_LIMIT_INFORMATION ExtendedLimitInfo;
    JOBOBJECT_BASIC_UI_RESTRICTIONS BasicUIRestrictions;
    JOBOBJECT_SECURITY_LIMIT_INFORMATION SecurityLimitInfo ;
    NTSTATUS st;
    ULONG RequiredLength, RequiredAlign, ActualReturnLength;
    PVOID ReturnData;
    PEPROCESS Process;
    PLIST_ENTRY Next;
    LARGE_INTEGER UserTime, KernelTime;
    PULONG_PTR NextProcessIdSlot;
    ULONG WorkingLength;
    PJOBOBJECT_BASIC_PROCESS_ID_LIST IdList;
    PUCHAR CurrentOffset ;
    PTOKEN_GROUPS WorkingGroup ;
    PTOKEN_PRIVILEGES WorkingPrivs ;
    ULONG RemainingSidBuffer ;
    PSID TargetSidBuffer ;
    PSID RemainingSid ;
    BOOLEAN AlreadyCopied ;

    PAGED_CODE();

    //
    // Get previous processor mode and probe output argument if necessary.
    //

    if ( JobObjectInformationClass >= MaxJobObjectInfoClass || JobObjectInformationClass <= 0) {
        return STATUS_INVALID_INFO_CLASS;
        }

    RequiredLength = PspJobInfoLengths[JobObjectInformationClass-1];
    RequiredAlign = PspJobInfoAlign[JobObjectInformationClass-1];
    ActualReturnLength = RequiredLength;

    if ( JobObjectInformationLength != RequiredLength ) {

        //
        // BasicProcessIdList is variable length, so make sure header is
        // ok. Can not enforce an exact match !  Security Limits can be
        // as well, due to the token groups and privs
        //
        if ( ( JobObjectInformationClass == JobObjectBasicProcessIdList ) ||
             ( JobObjectInformationClass == JobObjectSecurityLimitInformation ) ) {
            if ( JobObjectInformationLength < RequiredLength ) {
                return STATUS_INFO_LENGTH_MISMATCH;
                }
            else {
                RequiredLength = JobObjectInformationLength;
                }
            }
        else {
            return STATUS_INFO_LENGTH_MISMATCH;
            }
        }


    PreviousMode = KeGetPreviousMode();
    if (PreviousMode != KernelMode) {
        try {
            ProbeForWrite(
                JobObjectInformation,
                JobObjectInformationLength,
                RequiredAlign
                );
            if (ARGUMENT_PRESENT(ReturnLength)) {
                ProbeForWriteUlong(ReturnLength);
                }
            }
        except(EXCEPTION_EXECUTE_HANDLER) {
            return GetExceptionCode();
            }
        }

    //
    // reference the job
    //

    if ( ARGUMENT_PRESENT(JobHandle) ) {
        st = ObReferenceObjectByHandle(
                JobHandle,
                JOB_OBJECT_QUERY,
                PsJobType,
                PreviousMode,
                (PVOID *)&Job,
                NULL
                );
        if ( !NT_SUCCESS(st) ) {
            return st;
            }
        }
    else {

        //
        // if the current process has a job, NULL means the job of the
        // current process. Query is always allowed for this case
        //

        Process = PsGetCurrentProcess();

        if ( Process->Job ) {
            Job = Process->Job;
            ObReferenceObject(Job);
            }
        else {
            return STATUS_ACCESS_DENIED;
            }
        }

    AlreadyCopied = FALSE ;

    KeEnterCriticalRegion();
    ExAcquireResourceShared(&Job->JobLock, TRUE);

    //
    // Check argument validity.
    //

    switch ( JobObjectInformationClass ) {

    case JobObjectBasicAccountingInformation:
    case JobObjectBasicAndIoAccountingInformation:

        //
        // These two cases are identical, EXCEPT that with AndIo, IO information
        // is returned as well, but the first part of the local is identical to
        // basic, and the shorter return'd data length chops what we return.
        //

        RtlZeroMemory(&AccountingInfo.IoInfo,sizeof(AccountingInfo.IoInfo));

        AccountingInfo.BasicInfo.TotalUserTime = Job->TotalUserTime;
        AccountingInfo.BasicInfo.TotalKernelTime = Job->TotalKernelTime;
        AccountingInfo.BasicInfo.ThisPeriodTotalUserTime = Job->ThisPeriodTotalUserTime;
        AccountingInfo.BasicInfo.ThisPeriodTotalKernelTime = Job->ThisPeriodTotalKernelTime;
        AccountingInfo.BasicInfo.TotalPageFaultCount = Job->TotalPageFaultCount;

        AccountingInfo.BasicInfo.TotalProcesses = Job->TotalProcesses;
        AccountingInfo.BasicInfo.ActiveProcesses = Job->ActiveProcesses;
        AccountingInfo.BasicInfo.TotalTerminatedProcesses = Job->TotalTerminatedProcesses;

        AccountingInfo.IoInfo.ReadOperationCount = Job->ReadOperationCount;
        AccountingInfo.IoInfo.WriteOperationCount = Job->WriteOperationCount;
        AccountingInfo.IoInfo.OtherOperationCount = Job->OtherOperationCount;
        AccountingInfo.IoInfo.ReadTransferCount = Job->ReadTransferCount;
        AccountingInfo.IoInfo.WriteTransferCount = Job->WriteTransferCount;
        AccountingInfo.IoInfo.OtherTransferCount = Job->OtherTransferCount;

        //
        // Add in the time and page faults for each process
        //

        Next = Job->ProcessListHead.Flink;

        while ( Next != &Job->ProcessListHead) {

            Process = (PEPROCESS)(CONTAINING_RECORD(Next,EPROCESS,JobLinks));
            if ( !(Process->JobStatus & PS_JOB_STATUS_ACCOUNTING_FOLDED) ) {

                UserTime.QuadPart = UInt32x32To64(Process->Pcb.UserTime,KeMaximumIncrement);
                KernelTime.QuadPart = UInt32x32To64(Process->Pcb.KernelTime,KeMaximumIncrement);

                AccountingInfo.BasicInfo.TotalUserTime.QuadPart += UserTime.QuadPart;
                AccountingInfo.BasicInfo.TotalKernelTime.QuadPart += KernelTime.QuadPart;
                AccountingInfo.BasicInfo.ThisPeriodTotalUserTime.QuadPart += UserTime.QuadPart;
                AccountingInfo.BasicInfo.ThisPeriodTotalKernelTime.QuadPart += KernelTime.QuadPart;
                AccountingInfo.BasicInfo.TotalPageFaultCount += Process->Vm.PageFaultCount;

                AccountingInfo.IoInfo.ReadOperationCount += Process->ReadOperationCount.QuadPart;
                AccountingInfo.IoInfo.WriteOperationCount += Process->WriteOperationCount.QuadPart;
                AccountingInfo.IoInfo.OtherOperationCount += Process->OtherOperationCount.QuadPart;
                AccountingInfo.IoInfo.ReadTransferCount += Process->ReadTransferCount.QuadPart;
                AccountingInfo.IoInfo.WriteTransferCount += Process->WriteTransferCount.QuadPart;
                AccountingInfo.IoInfo.OtherTransferCount += Process->OtherTransferCount.QuadPart;
                }
            Next = Next->Flink;
            }

        ReturnData = &AccountingInfo;
        st = STATUS_SUCCESS;

        break;

    case JobObjectExtendedLimitInformation:
    case JobObjectBasicLimitInformation:

        //
        // Get the Basic Information
        //
        ExtendedLimitInfo.BasicLimitInformation.LimitFlags = Job->LimitFlags;
        ExtendedLimitInfo.BasicLimitInformation.MinimumWorkingSetSize = Job->MinimumWorkingSetSize;
        ExtendedLimitInfo.BasicLimitInformation.MaximumWorkingSetSize = Job->MaximumWorkingSetSize;
        ExtendedLimitInfo.BasicLimitInformation.ActiveProcessLimit = Job->ActiveProcessLimit;
        ExtendedLimitInfo.BasicLimitInformation.PriorityClass = (ULONG)Job->PriorityClass;
        ExtendedLimitInfo.BasicLimitInformation.SchedulingClass = Job->SchedulingClass;
        ExtendedLimitInfo.BasicLimitInformation.Affinity = (ULONG_PTR)Job->Affinity;
        ExtendedLimitInfo.BasicLimitInformation.PerProcessUserTimeLimit.QuadPart = Job->PerProcessUserTimeLimit.QuadPart;
        ExtendedLimitInfo.BasicLimitInformation.PerJobUserTimeLimit.QuadPart = Job->PerJobUserTimeLimit.QuadPart;
        ReturnData = &ExtendedLimitInfo.BasicLimitInformation;

        if ( JobObjectInformationClass == JobObjectExtendedLimitInformation ) {

            //
            // Get Extended Information
            //

            ExAcquireFastMutexUnsafe(&Job->MemoryLimitsLock);

            ExtendedLimitInfo.ProcessMemoryLimit = Job->ProcessMemoryLimit << PAGE_SHIFT;
            ExtendedLimitInfo.JobMemoryLimit = Job->JobMemoryLimit << PAGE_SHIFT;
            ExtendedLimitInfo.PeakJobMemoryUsed = Job->PeakJobMemoryUsed << PAGE_SHIFT;

            ExtendedLimitInfo.PeakProcessMemoryUsed = Job->PeakProcessMemoryUsed << PAGE_SHIFT;

            ExReleaseFastMutexUnsafe(&Job->MemoryLimitsLock);


            //
            // Zero un-used I/O counters
            //
            RtlZeroMemory(&ExtendedLimitInfo.IoInfo,sizeof(ExtendedLimitInfo.IoInfo));

            ReturnData = &ExtendedLimitInfo;
            }

        st = STATUS_SUCCESS;

        break;

    case JobObjectBasicUIRestrictions:

        BasicUIRestrictions.UIRestrictionsClass = Job->UIRestrictionsClass;

        ReturnData = &BasicUIRestrictions;
        st = STATUS_SUCCESS;

        break;

    case JobObjectBasicProcessIdList:

        IdList = (PJOBOBJECT_BASIC_PROCESS_ID_LIST)JobObjectInformation;
        NextProcessIdSlot = &IdList->ProcessIdList[0];
        WorkingLength = FIELD_OFFSET(JOBOBJECT_BASIC_PROCESS_ID_LIST,ProcessIdList);

        AlreadyCopied = TRUE ;

        try {

            //
            // Acounted for in the workinglength = 2*sizeof(ULONG)
            //

            IdList->NumberOfAssignedProcesses = Job->ActiveProcesses;
            IdList->NumberOfProcessIdsInList = 0;

            Next = Job->ProcessListHead.Flink;

            while ( Next != &Job->ProcessListHead) {

                Process = (PEPROCESS)(CONTAINING_RECORD(Next,EPROCESS,JobLinks));
                if ( !(Process->JobStatus & PS_JOB_STATUS_NOT_REALLY_ACTIVE) ) {
                    if ( !Process->UniqueProcessId ) {
                        IdList->NumberOfAssignedProcesses--;
                        }
                    else {
                        if ( (RequiredLength - WorkingLength) >= sizeof(ULONG_PTR) ) {
                            *NextProcessIdSlot++ = (ULONG_PTR)Process->UniqueProcessId;
                            WorkingLength += sizeof(ULONG_PTR);
                            IdList->NumberOfProcessIdsInList++;
                            }
                        else {
                            st = STATUS_BUFFER_OVERFLOW;
                            ActualReturnLength = WorkingLength;
                            break;
                            }
                        }
                    }
                Next = Next->Flink;
                }
            ActualReturnLength = WorkingLength;

            }
        except ( ExSystemExceptionFilter() ) {
            st = GetExceptionCode();
            ActualReturnLength = 0;
            }

        break;

    case JobObjectSecurityLimitInformation:

        RtlZeroMemory( &SecurityLimitInfo, sizeof( SecurityLimitInfo ) );

        SecurityLimitInfo.SecurityLimitFlags = Job->SecurityLimitFlags ;

        ReturnData = &SecurityLimitInfo;

        st = STATUS_SUCCESS;

        //
        // If a filter is present, then we have an ugly marshalling to do.
        //

        if ( Job->Filter )
        {

            //
            // Compute size needed:
            //

            WorkingLength = Job->Filter->CapturedSidsLength +
                            Job->Filter->CapturedGroupsLength +
                            Job->Filter->CapturedPrivilegesLength ;

            WorkingLength = 0 ;

            //
            // For each field, if it is present, include the extra stuff
            //

            if ( Job->Filter->CapturedSidsLength )
            {
                WorkingLength += Job->Filter->CapturedSidsLength +
                                 sizeof( ULONG );
            }

            if ( Job->Filter->CapturedGroupsLength )
            {
                WorkingLength += Job->Filter->CapturedGroupsLength +
                                 sizeof( ULONG );

            }

            if ( Job->Filter->CapturedPrivilegesLength )
            {
                WorkingLength += Job->Filter->CapturedPrivilegesLength +
                                 sizeof( ULONG );
            }

            RequiredLength -= sizeof( SecurityLimitInfo );

            if ( WorkingLength > RequiredLength )
            {
                st = STATUS_BUFFER_OVERFLOW ;
                ActualReturnLength = WorkingLength + sizeof( SecurityLimitInfo );
                break;
            }

            CurrentOffset = (PUCHAR) (JobObjectInformation) + sizeof( SecurityLimitInfo );

            try {

                //
                //
                //

                if ( Job->Filter->CapturedSidsLength )
                {
                    WorkingGroup = (PTOKEN_GROUPS) CurrentOffset ;

                    CurrentOffset += sizeof( ULONG );

                    SecurityLimitInfo.RestrictedSids = WorkingGroup ;

                    WorkingGroup->GroupCount = Job->Filter->CapturedSidCount ;

                    TargetSidBuffer = (PSID) (CurrentOffset +
                                                sizeof( SID_AND_ATTRIBUTES ) *
                                                Job->Filter->CapturedSidCount );

                    st = RtlCopySidAndAttributesArray(
                                Job->Filter->CapturedSidCount,
                                Job->Filter->CapturedSids,
                                WorkingLength,
                                WorkingGroup->Groups,
                                TargetSidBuffer,
                                &RemainingSid,
                                &RemainingSidBuffer );

                    CurrentOffset += Job->Filter->CapturedSidsLength ;

                }

                if ( !NT_SUCCESS( st ) )
                {
                    leave ;
                }

                if ( Job->Filter->CapturedGroupsLength )
                {
                    WorkingGroup = (PTOKEN_GROUPS) CurrentOffset ;

                    CurrentOffset += sizeof( ULONG );

                    SecurityLimitInfo.SidsToDisable = WorkingGroup ;

                    WorkingGroup->GroupCount = Job->Filter->CapturedGroupCount ;

                    TargetSidBuffer = (PSID) (CurrentOffset +
                                                sizeof( SID_AND_ATTRIBUTES ) *
                                                Job->Filter->CapturedGroupCount );

                    st = RtlCopySidAndAttributesArray(
                                Job->Filter->CapturedGroupCount,
                                Job->Filter->CapturedGroups,
                                WorkingLength,
                                WorkingGroup->Groups,
                                TargetSidBuffer,
                                &RemainingSid,
                                &RemainingSidBuffer );

                    CurrentOffset += Job->Filter->CapturedGroupsLength ;

                }

                if ( !NT_SUCCESS( st ) )
                {
                    leave ;
                }

                if ( Job->Filter->CapturedPrivilegesLength )
                {
                    WorkingPrivs = (PTOKEN_PRIVILEGES) CurrentOffset;

                    CurrentOffset += sizeof( ULONG );

                    SecurityLimitInfo.PrivilegesToDelete = WorkingPrivs ;

                    WorkingPrivs->PrivilegeCount = Job->Filter->CapturedPrivilegeCount ;

                    RtlCopyMemory( WorkingPrivs->Privileges,
                                   Job->Filter->CapturedPrivileges,
                                   Job->Filter->CapturedPrivilegesLength );

                }

                AlreadyCopied = TRUE ;

                RtlCopyMemory( JobObjectInformation,
                               &SecurityLimitInfo,
                               sizeof( SecurityLimitInfo ) );


            }
            except (EXCEPTION_EXECUTE_HANDLER) {
                st = GetExceptionCode();
                ActualReturnLength = 0 ;
                break;
            }

        }


        break;

    default:

        st = STATUS_INVALID_INFO_CLASS;
    }

    ExReleaseResource(&Job->JobLock);
    KeLeaveCriticalRegion();


    //
    // Finish Up
    //

    ObDereferenceObject(Job);


    if ( NT_SUCCESS(st) ) {

        //
        // Either of these may cause an access violation. The
        // exception handler will return access violation as
        // status code. No further cleanup needs to be done.
        //

        try {
            if ( !AlreadyCopied ) {
                RtlCopyMemory(JobObjectInformation,ReturnData,RequiredLength);
                }

            if (ARGUMENT_PRESENT(ReturnLength) ) {
                *ReturnLength = ActualReturnLength;
                }
            }
        except(EXCEPTION_EXECUTE_HANDLER) {
            return STATUS_SUCCESS;
            }
        }

    return st;

}

NTSTATUS
NtSetInformationJobObject(
    IN HANDLE JobHandle,
    IN JOBOBJECTINFOCLASS JobObjectInformationClass,
    IN PVOID JobObjectInformation,
    IN ULONG JobObjectInformationLength
    )
{
    PEJOB Job;
    EJOB LocalJob;
    KPROCESSOR_MODE PreviousMode;
    NTSTATUS st;
    JOBOBJECT_EXTENDED_LIMIT_INFORMATION ExtendedLimitInfo;
    JOBOBJECT_BASIC_UI_RESTRICTIONS BasicUIRestrictions;
    JOBOBJECT_SECURITY_LIMIT_INFORMATION SecurityLimitInfo ;
    JOBOBJECT_END_OF_JOB_TIME_INFORMATION EndOfJobInfo;
    JOBOBJECT_ASSOCIATE_COMPLETION_PORT AssociateInfo;
    ULONG RequiredAccess ;
    ULONG RequiredLength, RequiredAlign;
    PEPROCESS Process;
    BOOLEAN HasPrivilege;
    BOOLEAN IsChild ;
    PLIST_ENTRY Next;
    PPS_JOB_TOKEN_FILTER Filter ;
    PVOID IoCompletion;
    PACCESS_TOKEN LocalToken ;
    ULONG ValidFlags;
    ULONG LimitFlags;
    BOOLEAN ProcessWorkingSetHead = FALSE;
    PJOB_WORKING_SET_CHANGE_RECORD WsChangeRecord;


    PAGED_CODE();

    //
    // Get previous processor mode and probe output argument if necessary.
    //

    if ( JobObjectInformationClass >= MaxJobObjectInfoClass || JobObjectInformationClass <= 0) {
        return STATUS_INVALID_INFO_CLASS;
        }
    RequiredLength = PspJobInfoLengths[JobObjectInformationClass-1];
    RequiredAlign = PspJobInfoAlign[JobObjectInformationClass-1];

    PreviousMode = KeGetPreviousMode();
    if (PreviousMode != KernelMode) {
        try {
            ProbeForRead(
                JobObjectInformation,
                JobObjectInformationLength,
                RequiredAlign
                );
            }
        except(EXCEPTION_EXECUTE_HANDLER) {
            return GetExceptionCode();
            }
        }

    if ( JobObjectInformationLength != RequiredLength ) {
        return STATUS_INFO_LENGTH_MISMATCH;
        }

    //
    // reference the job
    //

    if ( JobObjectInformationClass == JobObjectSecurityLimitInformation )
    {
        RequiredAccess = JOB_OBJECT_SET_SECURITY_ATTRIBUTES ;
    }
    else
    {
        RequiredAccess = JOB_OBJECT_SET_ATTRIBUTES ;
    }

    st = ObReferenceObjectByHandle(
            JobHandle,
            RequiredAccess,
            PsJobType,
            PreviousMode,
            (PVOID *)&Job,
            NULL
            );
    if ( !NT_SUCCESS(st) ) {
        return st;
        }

    KeEnterCriticalRegion();
    ExAcquireResourceExclusive(&Job->JobLock, TRUE);

    //
    // Check argument validity.
    //

    switch ( JobObjectInformationClass ) {

    case JobObjectExtendedLimitInformation:
    case JobObjectBasicLimitInformation:
        try {
            RtlCopyMemory(&ExtendedLimitInfo,JobObjectInformation,RequiredLength);
            }
        except(EXCEPTION_EXECUTE_HANDLER) {
            st = GetExceptionCode();
            }
        if ( NT_SUCCESS(st) ) {
            //
            // sanity check LimitFlags
            //
            if ( JobObjectInformationClass == JobObjectBasicLimitInformation) {
                ValidFlags = JOB_OBJECT_BASIC_LIMIT_VALID_FLAGS;
                }
            else {
                ValidFlags = JOB_OBJECT_EXTENDED_LIMIT_VALID_FLAGS;
                }

            if ( ExtendedLimitInfo.BasicLimitInformation.LimitFlags & ~ValidFlags ) {
                st = STATUS_INVALID_PARAMETER;
                }
            else {

                LimitFlags = ExtendedLimitInfo.BasicLimitInformation.LimitFlags;

                //
                // Deal with each of the various limit flags
                //

                LocalJob.LimitFlags = Job->LimitFlags;


                //
                // ACTIVE PROCESS LIMIT
                //
                if ( LimitFlags & JOB_OBJECT_LIMIT_ACTIVE_PROCESS ) {

                    //
                    // Active Process Limit is NOT retroactive. New processes are denied,
                    // but existing ones are not killed just because the limit is
                    // reduced.
                    //

                    LocalJob.LimitFlags |= JOB_OBJECT_LIMIT_ACTIVE_PROCESS;
                    LocalJob.ActiveProcessLimit = ExtendedLimitInfo.BasicLimitInformation.ActiveProcessLimit;
                    }
                else {
                    LocalJob.LimitFlags &= ~JOB_OBJECT_LIMIT_ACTIVE_PROCESS;
                    LocalJob.ActiveProcessLimit = 0;
                    }

                //
                // PRIORITY CLASS LIMIT
                //
                if ( LimitFlags & JOB_OBJECT_LIMIT_PRIORITY_CLASS ) {

                    if ( ExtendedLimitInfo.BasicLimitInformation.PriorityClass > PROCESS_PRIORITY_CLASS_ABOVE_NORMAL ) {
                        st = STATUS_INVALID_PARAMETER;
                        }
                    else {
                        if ( ExtendedLimitInfo.BasicLimitInformation.PriorityClass == PROCESS_PRIORITY_CLASS_HIGH ||
                             ExtendedLimitInfo.BasicLimitInformation.PriorityClass == PROCESS_PRIORITY_CLASS_REALTIME ) {

                            //
                            // Increasing the base priority of a process is a
                            // privileged operation.  Check for the privilege
                            // here.
                            //

                            HasPrivilege = SeCheckPrivilegedObject(
                                               SeIncreaseBasePriorityPrivilege,
                                               JobHandle,
                                               JOB_OBJECT_SET_ATTRIBUTES,
                                               PreviousMode
                                               );

                            if (!HasPrivilege) {
                                st = STATUS_PRIVILEGE_NOT_HELD;
                                }
                            }

                        if ( NT_SUCCESS(st) ) {
                            LocalJob.LimitFlags |= JOB_OBJECT_LIMIT_PRIORITY_CLASS;
                            LocalJob.PriorityClass = (UCHAR)ExtendedLimitInfo.BasicLimitInformation.PriorityClass;
                            }
                        }
                    }
                else {
                    LocalJob.LimitFlags &= ~JOB_OBJECT_LIMIT_PRIORITY_CLASS;
                    LocalJob.PriorityClass = 0;
                    }

                //
                // SCHEDULING CLASS LIMIT
                //
                if ( LimitFlags & JOB_OBJECT_LIMIT_SCHEDULING_CLASS ) {

                    if ( ExtendedLimitInfo.BasicLimitInformation.SchedulingClass >= PSP_NUMBER_OF_SCHEDULING_CLASSES) {
                        st = STATUS_INVALID_PARAMETER;
                        }
                    else {
                        if ( ExtendedLimitInfo.BasicLimitInformation.SchedulingClass > PSP_DEFAULT_SCHEDULING_CLASSES ) {

                            //
                            // Increasing above the default scheduling class
                            // is a
                            // privileged operation.  Check for the privilege
                            // here.
                            //

                            HasPrivilege = SeCheckPrivilegedObject(
                                               SeIncreaseBasePriorityPrivilege,
                                               JobHandle,
                                               JOB_OBJECT_SET_ATTRIBUTES,
                                               PreviousMode
                                               );

                            if (!HasPrivilege) {
                                st = STATUS_PRIVILEGE_NOT_HELD;
                                }
                            }

                        if ( NT_SUCCESS(st) ) {
                            LocalJob.LimitFlags |= JOB_OBJECT_LIMIT_SCHEDULING_CLASS;
                            LocalJob.SchedulingClass = ExtendedLimitInfo.BasicLimitInformation.SchedulingClass;
                            }
                        }
                    }
                else {
                    LocalJob.LimitFlags &= ~JOB_OBJECT_LIMIT_SCHEDULING_CLASS;
                    LocalJob.SchedulingClass = PSP_DEFAULT_SCHEDULING_CLASSES ;
                    }

                //
                // AFFINITY LIMIT
                //
                if ( LimitFlags & JOB_OBJECT_LIMIT_AFFINITY ) {

                    if ( !ExtendedLimitInfo.BasicLimitInformation.Affinity ||
                         (ExtendedLimitInfo.BasicLimitInformation.Affinity != (ExtendedLimitInfo.BasicLimitInformation.Affinity & KeActiveProcessors)) ) {
                        st = STATUS_INVALID_PARAMETER;
                        }
                    else {
                        LocalJob.LimitFlags |= JOB_OBJECT_LIMIT_AFFINITY;
                        LocalJob.Affinity = (KAFFINITY)ExtendedLimitInfo.BasicLimitInformation.Affinity;
                        }
                    }
                else {
                    LocalJob.LimitFlags &= ~JOB_OBJECT_LIMIT_AFFINITY;
                    LocalJob.Affinity = 0;
                    }

                //
                // PROCESS TIME LIMIT
                //
                if ( LimitFlags & JOB_OBJECT_LIMIT_PROCESS_TIME ) {

                    if ( !ExtendedLimitInfo.BasicLimitInformation.PerProcessUserTimeLimit.QuadPart ) {
                        st = STATUS_INVALID_PARAMETER;
                        }
                    else {
                        LocalJob.LimitFlags |= JOB_OBJECT_LIMIT_PROCESS_TIME;
                        LocalJob.PerProcessUserTimeLimit.QuadPart = ExtendedLimitInfo.BasicLimitInformation.PerProcessUserTimeLimit.QuadPart;
                        }
                    }
                else {
                    LocalJob.LimitFlags &= ~JOB_OBJECT_LIMIT_PROCESS_TIME;
                    LocalJob.PerProcessUserTimeLimit.QuadPart = 0;
                    }

                //
                // JOB TIME LIMIT
                //
                if ( LimitFlags & JOB_OBJECT_LIMIT_JOB_TIME ) {

                    if ( !ExtendedLimitInfo.BasicLimitInformation.PerJobUserTimeLimit.QuadPart ) {
                        st = STATUS_INVALID_PARAMETER;
                        }
                    else {
                        LocalJob.LimitFlags |= JOB_OBJECT_LIMIT_JOB_TIME;
                        LocalJob.PerJobUserTimeLimit.QuadPart = ExtendedLimitInfo.BasicLimitInformation.PerJobUserTimeLimit.QuadPart;
                        }
                    }
                else {
                    if ( LimitFlags & JOB_OBJECT_LIMIT_PRESERVE_JOB_TIME ) {

                        //
                        // If we are supposed to preserve existing job time limits, then
                        // preserve them !
                        //

                        LocalJob.LimitFlags |= (Job->LimitFlags & JOB_OBJECT_LIMIT_JOB_TIME);
                        LocalJob.PerJobUserTimeLimit.QuadPart = Job->PerJobUserTimeLimit.QuadPart;
                        }
                    else {
                        LocalJob.LimitFlags &= ~JOB_OBJECT_LIMIT_JOB_TIME;
                        LocalJob.PerJobUserTimeLimit.QuadPart = 0;
                        }
                    }

                //
                // WORKING SET LIMIT
                //
                if ( LimitFlags & JOB_OBJECT_LIMIT_WORKINGSET ) {


                    //
                    // the only issue with this check is that when we enforce through the
                    // processes, we may find a process that can not handle the new working set
                    // limit because it will make the process's working set not fluid
                    //

                    if ( (ExtendedLimitInfo.BasicLimitInformation.MinimumWorkingSetSize == 0 &&
                         ExtendedLimitInfo.BasicLimitInformation.MaximumWorkingSetSize == 0)                 ||

                         (ExtendedLimitInfo.BasicLimitInformation.MinimumWorkingSetSize == (SIZE_T)-1 &&
                         ExtendedLimitInfo.BasicLimitInformation.MaximumWorkingSetSize == (SIZE_T)-1)        ||

                         (ExtendedLimitInfo.BasicLimitInformation.MinimumWorkingSetSize >
                            ExtendedLimitInfo.BasicLimitInformation.MaximumWorkingSetSize)                   ) {


                        st = STATUS_INVALID_PARAMETER;
                        }
                    else {
                        LocalJob.LimitFlags |= JOB_OBJECT_LIMIT_WORKINGSET;
                        LocalJob.MinimumWorkingSetSize = ExtendedLimitInfo.BasicLimitInformation.MinimumWorkingSetSize;
                        LocalJob.MaximumWorkingSetSize = ExtendedLimitInfo.BasicLimitInformation.MaximumWorkingSetSize;
                        }
                    }
                else {
                    LocalJob.LimitFlags &= ~JOB_OBJECT_LIMIT_WORKINGSET;
                    LocalJob.MinimumWorkingSetSize = 0;
                    LocalJob.MaximumWorkingSetSize = 0;
                    }

                if ( JobObjectInformationClass == JobObjectExtendedLimitInformation) {
                    //
                    // PROCESS MEMORY LIMIT
                    //
                    if ( LimitFlags & JOB_OBJECT_LIMIT_PROCESS_MEMORY ) {
                        if ( ExtendedLimitInfo.ProcessMemoryLimit < PAGE_SIZE ) {
                            st = STATUS_INVALID_PARAMETER;
                            }
                        else {
                            LocalJob.LimitFlags |= JOB_OBJECT_LIMIT_PROCESS_MEMORY;
                            LocalJob.ProcessMemoryLimit = ExtendedLimitInfo.ProcessMemoryLimit >> PAGE_SHIFT;
                            }
                        }
                    else {
                        LocalJob.LimitFlags &= ~JOB_OBJECT_LIMIT_PROCESS_MEMORY;
                        LocalJob.ProcessMemoryLimit = 0;
                        }

                    //
                    // JOB WIDE MEMORY LIMIT
                    //
                    if ( LimitFlags & JOB_OBJECT_LIMIT_JOB_MEMORY ) {
                        if ( ExtendedLimitInfo.JobMemoryLimit < PAGE_SIZE ) {
                            st = STATUS_INVALID_PARAMETER;
                            }
                        else {
                            LocalJob.LimitFlags |= JOB_OBJECT_LIMIT_JOB_MEMORY;
                            LocalJob.JobMemoryLimit = ExtendedLimitInfo.JobMemoryLimit >> PAGE_SHIFT;
                            }
                        }
                    else {
                        LocalJob.LimitFlags &= ~JOB_OBJECT_LIMIT_JOB_MEMORY;
                        LocalJob.JobMemoryLimit = 0;
                        }

                    //
                    // JOB_OBJECT_LIMIT_DIE_ON_UNHANDLED_EXCEPTION
                    //
                    if ( LimitFlags & JOB_OBJECT_LIMIT_DIE_ON_UNHANDLED_EXCEPTION ) {
                        LocalJob.LimitFlags |= JOB_OBJECT_LIMIT_DIE_ON_UNHANDLED_EXCEPTION;
                        }
                    else {
                        LocalJob.LimitFlags &= ~JOB_OBJECT_LIMIT_DIE_ON_UNHANDLED_EXCEPTION;
                        }

                    //
                    // JOB_OBJECT_LIMIT_BREAKAWAY_OK
                    //
                    if ( LimitFlags & JOB_OBJECT_LIMIT_BREAKAWAY_OK ) {
                        LocalJob.LimitFlags |= JOB_OBJECT_LIMIT_BREAKAWAY_OK;
                        }
                    else {
                        LocalJob.LimitFlags &= ~JOB_OBJECT_LIMIT_BREAKAWAY_OK;
                        }

                    //
                    // JOB_OBJECT_LIMIT_SILENT_BREAKAWAY_OK
                    //
                    if ( LimitFlags & JOB_OBJECT_LIMIT_SILENT_BREAKAWAY_OK ) {
                        LocalJob.LimitFlags |= JOB_OBJECT_LIMIT_SILENT_BREAKAWAY_OK;
                        }
                    else {
                        LocalJob.LimitFlags &= ~JOB_OBJECT_LIMIT_SILENT_BREAKAWAY_OK;
                        }
                    }

                if ( NT_SUCCESS(st) ) {


                    //
                    // Copy LocalJob to Job
                    //

                    Job->LimitFlags = LocalJob.LimitFlags;
                    Job->MinimumWorkingSetSize = LocalJob.MinimumWorkingSetSize;
                    Job->MaximumWorkingSetSize = LocalJob.MaximumWorkingSetSize;
                    Job->ActiveProcessLimit = LocalJob.ActiveProcessLimit;
                    Job->Affinity = LocalJob.Affinity;
                    Job->PriorityClass = LocalJob.PriorityClass;
                    Job->SchedulingClass = LocalJob.SchedulingClass;
                    Job->PerProcessUserTimeLimit.QuadPart = LocalJob.PerProcessUserTimeLimit.QuadPart;
                    Job->PerJobUserTimeLimit.QuadPart = LocalJob.PerJobUserTimeLimit.QuadPart;

                    if ( JobObjectInformationClass == JobObjectExtendedLimitInformation) {
                        ExAcquireFastMutexUnsafe(&Job->MemoryLimitsLock);
                        Job->ProcessMemoryLimit = LocalJob.ProcessMemoryLimit;
                        Job->JobMemoryLimit = LocalJob.JobMemoryLimit;
                        ExReleaseFastMutexUnsafe(&Job->MemoryLimitsLock);
                        }

                    if ( LimitFlags & JOB_OBJECT_LIMIT_JOB_TIME ) {

                        //
                        // Take any signalled processes and fold their accounting
                        // intothe job. This way a process that exited clean but still
                        // is open won't impact the next period
                        //

                        Next = Job->ProcessListHead.Flink;

                        while ( Next != &Job->ProcessListHead) {

                            Process = (PEPROCESS)(CONTAINING_RECORD(Next,EPROCESS,JobLinks));

                            //
                            // see if process has been signalled.
                            // This indicates that the process has exited. We can't do
                            // this in the exit path becuase of the lock order problem
                            // between the process lock and the job lock since in exit
                            // we hold the process lock for a long time and can't drop
                            // it until thread termination
                            //

                            if ( KeReadStateProcess(&Process->Pcb) ) {
                                PspFoldProcessAccountingIntoJob(Job,Process);
                                }
                            else {

                                LARGE_INTEGER ProcessTime;

                                //
                                // running processes have their current runtime
                                // added to the programmed limit. This way, you
                                // can set a limit on a job with processes in the
                                // job and not have previous runtimes count against
                                // the limit
                                //

                                if ( !(Process->JobStatus & PS_JOB_STATUS_ACCOUNTING_FOLDED) ) {
                                    ProcessTime.QuadPart = UInt32x32To64(Process->Pcb.UserTime,KeMaximumIncrement);
                                    Job->PerJobUserTimeLimit.QuadPart += ProcessTime.QuadPart;
                                    }
                                }

                            Next = Next->Flink;
                            }


                        //
                        // clear period times and reset the job
                        //

                        Job->ThisPeriodTotalUserTime.QuadPart = 0;
                        Job->ThisPeriodTotalKernelTime.QuadPart = 0;

                        KeClearEvent(&Job->Event);

                        }

                    if ( Job->LimitFlags & JOB_OBJECT_LIMIT_WORKINGSET ) {
                        ExAcquireFastMutexUnsafe(&PspWorkingSetChangeHead.Lock);
                        PspWorkingSetChangeHead.MinimumWorkingSetSize = Job->MinimumWorkingSetSize;
                        PspWorkingSetChangeHead.MaximumWorkingSetSize = Job->MaximumWorkingSetSize;
                        ProcessWorkingSetHead = TRUE;
                        }

                    PspApplyJobLimitsToProcessSet(Job);

                    }
                }

            }
        break;

    case JobObjectBasicUIRestrictions:
        try {
            RtlCopyMemory(&BasicUIRestrictions, JobObjectInformation, RequiredLength);
            }
        except(EXCEPTION_EXECUTE_HANDLER) {
            st = GetExceptionCode();
            }

        if ( NT_SUCCESS(st) ) {
            //
            // sanity check UIRestrictionsClass
            //
            if ( BasicUIRestrictions.UIRestrictionsClass & ~JOB_OBJECT_UI_VALID_FLAGS ) {
                st = STATUS_INVALID_PARAMETER;
                }
            else {

                //
                // Check for switching between UI restrictions
                //

                if ( Job->UIRestrictionsClass ^ BasicUIRestrictions.UIRestrictionsClass ) {

                    //
                    // notify ntuser that the UI restrictions have changed
                    //
                    WIN32_JOBCALLOUT_PARAMETERS Parms;

                    Parms.Job = Job;
                    Parms.CalloutType = PsW32JobCalloutSetInformation;
                    Parms.Data = ULongToPtr(BasicUIRestrictions.UIRestrictionsClass);
                    MmDispatchWin32Callout( PspW32JobCallout,NULL, (PVOID)&Parms, &(Job->SessionId) );

                    }


                //
                // save the UI restrictions into the job object
                //

                Job->UIRestrictionsClass = BasicUIRestrictions.UIRestrictionsClass;
                }
            }
        break;

        //
        // SECURITY LIMITS
        //

    case JobObjectSecurityLimitInformation:

        try {
            RtlCopyMemory(  &SecurityLimitInfo,
                            JobObjectInformation,
                            RequiredLength );
            }
        except(EXCEPTION_EXECUTE_HANDLER) {
            st = GetExceptionCode();
            }

        if ( NT_SUCCESS(st) ) {

            if ( SecurityLimitInfo.SecurityLimitFlags &
                    (~JOB_OBJECT_SECURITY_VALID_FLAGS))
            {
                st = STATUS_INVALID_PARAMETER ;
            }
            else
            {
                //
                // Deal with specific options.  Basic rules:  Once a
                // flag is on, it is always on (so even with a handle to
                // the job, a process could not lift the security
                // restrictions).
                //

                if ( SecurityLimitInfo.SecurityLimitFlags &
                            JOB_OBJECT_SECURITY_NO_ADMIN )
                {
                    Job->SecurityLimitFlags |= JOB_OBJECT_SECURITY_NO_ADMIN ;

                    if ( Job->Token )
                    {
                        if ( SeTokenIsAdmin( Job->Token ) )
                        {
                            Job->SecurityLimitFlags &= (~JOB_OBJECT_SECURITY_NO_ADMIN);

                            st = STATUS_INVALID_PARAMETER ;
                        }
                    }
                }

                if ( SecurityLimitInfo.SecurityLimitFlags &
                            JOB_OBJECT_SECURITY_RESTRICTED_TOKEN )
                {
                    if ( Job->SecurityLimitFlags &
                            ( JOB_OBJECT_SECURITY_ONLY_TOKEN | JOB_OBJECT_SECURITY_FILTER_TOKENS ) )
                    {
                        st = STATUS_INVALID_PARAMETER ;
                    }
                    else
                    {
                        Job->SecurityLimitFlags |= JOB_OBJECT_SECURITY_RESTRICTED_TOKEN ;
                    }
                }

                //
                // The forcible token is a little more interesting.  It
                // cannot be reset, so if there is a pointer there already,
                // fail the call.  If a filter is already in place, this is
                // not allowed, either.  If no-admin is set, it is checked
                // at the end, once the token has been ref'd.
                //

                if ( SecurityLimitInfo.SecurityLimitFlags &
                            JOB_OBJECT_SECURITY_ONLY_TOKEN )
                {
                    if ( Job->Token ||
                         (Job->SecurityLimitFlags & JOB_OBJECT_SECURITY_FILTER_TOKENS) )
                    {
                        st = STATUS_INVALID_PARAMETER ;
                    }
                    else
                    {
                        st = ObReferenceObjectByHandle(
                                             SecurityLimitInfo.JobToken,
                                            TOKEN_ASSIGN_PRIMARY |
                                                TOKEN_IMPERSONATE |
                                                TOKEN_DUPLICATE ,
                                            SeTokenObjectType(),
                                            PreviousMode,
                                            &LocalToken,
                                            NULL );

                        if ( NT_SUCCESS( st ) )
                        {
                            st = SeIsChildTokenByPointer( LocalToken,
                                                          &IsChild );

                            if ( !NT_SUCCESS( st ) )
                            {
                                ObDereferenceObject( LocalToken );
                            }
                        }


                        if ( NT_SUCCESS( st ) )
                        {
                            //
                            // If the token supplied is not a restricted token
                            // based on the caller's ID, then they must have
                            // assign primary privilege in order to associate
                            // the token with the job.
                            //

                            if ( !IsChild )
                            {
                                HasPrivilege = SeCheckPrivilegedObject(
                                                   SeAssignPrimaryTokenPrivilege,
                                                   JobHandle,
                                                   JOB_OBJECT_SET_SECURITY_ATTRIBUTES,
                                                   PreviousMode
                                                   );

                                if ( !HasPrivilege )
                                {
                                    st = STATUS_PRIVILEGE_NOT_HELD;
                                }
                            }

                            if ( NT_SUCCESS( st ) )
                            {
                                //
                                // Grab a reference to the token into the job
                                // object
                                //

                                Job->Token = LocalToken ;

                                //
                                // Not surprisingly, specifying no-admin and
                                // supplying an admin token is a no-no.
                                //

                                if ( (Job->SecurityLimitFlags & JOB_OBJECT_SECURITY_NO_ADMIN) &&
                                      SeTokenIsAdmin( Job->Token ) )
                                {
                                    st = STATUS_INVALID_PARAMETER ;

                                    ObDereferenceObject( Job->Token );

                                    Job->Token = NULL ;
                                }
                                else
                                {
                                    Job->SecurityLimitFlags |= JOB_OBJECT_SECURITY_ONLY_TOKEN ;
                                }

                            }
                            else
                            {
                                //
                                // This is the token was a child or otherwise ok,
                                // but assign primary was not held, so the
                                // request was rejected.
                                //

                                ObDereferenceObject( LocalToken );
                            }

                        }

                    }
                }
                if ( SecurityLimitInfo.SecurityLimitFlags &
                            JOB_OBJECT_SECURITY_FILTER_TOKENS )
                {
                    if ( Job->SecurityLimitFlags &
                          ( JOB_OBJECT_SECURITY_ONLY_TOKEN |
                            JOB_OBJECT_SECURITY_FILTER_TOKENS ) )
                    {
                        st = STATUS_INVALID_PARAMETER ;
                    }
                    else
                    {
                        //
                        // capture the token restrictions
                        //

                        st = PspCaptureTokenFilter(
                                PreviousMode,
                                &SecurityLimitInfo,
                                &Filter
                                );

                        if ( NT_SUCCESS( st ) )
                        {
                            Job->SecurityLimitFlags |= JOB_OBJECT_SECURITY_FILTER_TOKENS ;
                            Job->Filter = Filter ;
                        }

                    }
                }

            }
        }
        break;

    case JobObjectEndOfJobTimeInformation:

        try {
            RtlCopyMemory(&EndOfJobInfo,JobObjectInformation,RequiredLength);
            }
        except(EXCEPTION_EXECUTE_HANDLER) {
            st = GetExceptionCode();
            }

        if ( NT_SUCCESS(st) ) {
            //
            // sanity check LimitFlags
            //
            if ( EndOfJobInfo.EndOfJobTimeAction > JOB_OBJECT_POST_AT_END_OF_JOB ) {
                st = STATUS_INVALID_PARAMETER;
                }
            else {
                Job->EndOfJobTimeAction = EndOfJobInfo.EndOfJobTimeAction;
                }
            }
        break;

    case JobObjectAssociateCompletionPortInformation:

        try {
            RtlCopyMemory(&AssociateInfo,JobObjectInformation,RequiredLength);
            }
        except(EXCEPTION_EXECUTE_HANDLER) {
            st = GetExceptionCode();
            }

        if ( NT_SUCCESS(st) ) {
            if ( !Job->CompletionPort && AssociateInfo.CompletionPort ) {
                st = ObReferenceObjectByHandle(
                        AssociateInfo.CompletionPort,
                        IO_COMPLETION_MODIFY_STATE,
                        IoCompletionObjectType,
                        PreviousMode,
                        &IoCompletion,
                        NULL
                        );

                if (NT_SUCCESS(st)) {
                    Job->CompletionKey = AssociateInfo.CompletionKey;
                    Job->CompletionPort = IoCompletion;

                    //
                    // Now whip through ALL existing processes in the job
                    // and send notification messages
                    //

                    Next = Job->ProcessListHead.Flink;

                    while ( Next != &Job->ProcessListHead) {

                        Process = (PEPROCESS)(CONTAINING_RECORD(Next,EPROCESS,JobLinks));


                        //
                        // If the process is really considered part of the job, has
                        // been assigned its id, and has not yet checked in, do it now
                        //

                        if ( !(Process->JobStatus & PS_JOB_STATUS_NOT_REALLY_ACTIVE)
                             && Process->UniqueProcessId
                             && !(Process->JobStatus & PS_JOB_STATUS_NEW_PROCESS_REPORTED)) {

                            PS_SET_CLEAR_BITS (&Process->JobStatus,
                                               PS_JOB_STATUS_NEW_PROCESS_REPORTED,
                                               PS_JOB_STATUS_LAST_REPORT_MEMORY);

                            IoSetIoCompletion(
                                Job->CompletionPort,
                                Job->CompletionKey,
                                (PVOID)Process->UniqueProcessId,
                                STATUS_SUCCESS,
                                JOB_OBJECT_MSG_NEW_PROCESS,
                                FALSE
                                );

                            }
                        Next = Next->Flink;
                        }
                    }
                }
            else {
                st = STATUS_INVALID_PARAMETER;
                }
            }
        break;


    default:

        st = STATUS_INVALID_INFO_CLASS;
    }

    ExReleaseResource(&Job->JobLock);

    //
    // Working Set Changes are processed outside of the job lock.
    //
    // calling MmAdjust CAN NOT cause MM to call PsChangeJobMemoryUsage !
    //

    if ( ProcessWorkingSetHead ) {
        if ( !IsListEmpty(&PspWorkingSetChangeHead.Links) ) {
            while ( !IsListEmpty(&PspWorkingSetChangeHead.Links) ) {
                Next = RemoveHeadList(&PspWorkingSetChangeHead.Links);
                WsChangeRecord = CONTAINING_RECORD(Next,JOB_WORKING_SET_CHANGE_RECORD,Links);
                KeAttachProcess(&WsChangeRecord->Process->Pcb);

                MmAdjustWorkingSetSize(
                    PspWorkingSetChangeHead.MinimumWorkingSetSize,
                    PspWorkingSetChangeHead.MaximumWorkingSetSize,
                    FALSE
                    );

                //
                // call MM to Enable hard workingset
                //

                MmEnforceWorkingSetLimit(&WsChangeRecord->Process->Vm, TRUE);
                KeDetachProcess();
                ObDereferenceObject(WsChangeRecord->Process);
                ExFreePool(WsChangeRecord);
                }
            }
        ExReleaseFastMutexUnsafe(&PspWorkingSetChangeHead.Lock);
        }
    KeLeaveCriticalRegion();


    //
    // Finish Up
    //

    ObDereferenceObject(Job);

    return st;
}

VOID
PspApplyJobLimitsToProcessSet(
    PEJOB Job
    )
{

    PLIST_ENTRY Next;
    PEPROCESS Process;
    PJOB_WORKING_SET_CHANGE_RECORD WsChangeRecord;

    PAGED_CODE();

    //
    // The job object is held exclusive by the caller
    //

    Next = Job->ProcessListHead.Flink;

    while ( Next != &Job->ProcessListHead) {

        Process = (PEPROCESS)(CONTAINING_RECORD(Next,EPROCESS,JobLinks));
        if ( !(Process->JobStatus & PS_JOB_STATUS_NOT_REALLY_ACTIVE) ) {
            if ( Job->LimitFlags & JOB_OBJECT_LIMIT_WORKINGSET ) {
                WsChangeRecord = ExAllocatePoolWithTag(
                                        PagedPool,
                                        sizeof( *WsChangeRecord ),
                                        'rCsP'
                                        );
                if ( WsChangeRecord ) {
                    WsChangeRecord->Process = Process;
                    ObReferenceObject(Process);
                    //
                    // Avoid double delete since process could be in delete routine during the above ref
                    //
                    if ( ObGetObjectPointerCount(Process) > 1 ) {
                        InsertTailList(&PspWorkingSetChangeHead.Links,&WsChangeRecord->Links);
                        }
                    else {
                        //
                        // process is possibly in delete routine waiting to come
                        // out of job. DON'T Dereference !
                        //
                        ExFreePool(WsChangeRecord);
                        }
                    }
                }
            PspApplyJobLimitsToProcess(Job,Process);
            }
        Next = Next->Flink;
        }
}

VOID
PspApplyJobLimitsToProcess(
    PEJOB Job,
    PEPROCESS Process
    )
{

    NTSTATUS Status;
    PLIST_ENTRY Next;
    PETHREAD Thread;

    PAGED_CODE();

    //
    // The job object is held exclusive by the caller
    //

    if ( Job->LimitFlags & JOB_OBJECT_LIMIT_PRIORITY_CLASS ) {
        Process->PriorityClass = Job->PriorityClass;

        PsSetProcessPriorityByClass(
                Process,
                Process->Vm.MemoryPriority == MEMORY_PRIORITY_FOREGROUND ?
                    PsProcessPriorityForeground : PsProcessPriorityBackground
                );
        }

    if ( Job->LimitFlags & JOB_OBJECT_LIMIT_AFFINITY ) {

        //
        // the following allows this api to properly if
        // called while the exiting process is blocked holding the
        // createdeletelock. This can happen during debugger/server
        // lpc transactions that occur in pspexitthread
        //

        Status = PsLockProcess(Process,KeGetPreviousMode(),PsLockPollOnTimeout);

        if ( Status == STATUS_SUCCESS ) {
            Process->Pcb.Affinity = Job->Affinity;

            Next = Process->ThreadListHead.Flink;

            while ( Next != &Process->ThreadListHead) {

                Thread = (PETHREAD)(CONTAINING_RECORD(Next,ETHREAD,ThreadListEntry));
                KeSetAffinityThread(&Thread->Tcb,Job->Affinity);
                Next = Next->Flink;
                }

            PsUnlockProcess(Process);
            }
        }

    if ( !(Job->LimitFlags & JOB_OBJECT_LIMIT_WORKINGSET) ) {
        //
        // call MM to disable hard workingset
        //

        MmEnforceWorkingSetLimit(&Process->Vm, FALSE);
        }

    ExAcquireFastMutexUnsafe(&Job->MemoryLimitsLock);
    if ( Job->LimitFlags & JOB_OBJECT_LIMIT_PROCESS_MEMORY  ) {
        Process->CommitChargeLimit = Job->ProcessMemoryLimit;
        }
    else {
        Process->CommitChargeLimit = 0;
        }
    ExReleaseFastMutexUnsafe(&Job->MemoryLimitsLock);

    //
    // If the process is NOT IDLE Priority Class, and long fixed quantums
    // are in use, use the scheduling class stored in the job object for this process
    //
    if ( Process->PriorityClass != PROCESS_PRIORITY_CLASS_IDLE ) {

        if ( PspUseJobSchedulingClasses ) {
            Process->Pcb.ThreadQuantum = PspJobSchedulingClasses[Job->SchedulingClass];
            }
        //
        // if the scheduling class is PSP_NUMBER_OF_SCHEDULING_CLASSES-1, then
        // give this process non-preemptive scheduling
        //
        if ( Job->SchedulingClass == PSP_NUMBER_OF_SCHEDULING_CLASSES-1 ) {
            KeSetDisableQuantumProcess(&Process->Pcb,TRUE);
            }
        else {
            KeSetDisableQuantumProcess(&Process->Pcb,FALSE);
            }

        }


}

NTSTATUS
NtTerminateJobObject(
    IN HANDLE JobHandle,
    IN NTSTATUS ExitStatus
    )
{
    PEJOB Job;
    NTSTATUS st;
    KPROCESSOR_MODE PreviousMode;

    PAGED_CODE();

    PreviousMode = KeGetPreviousMode();
    st = ObReferenceObjectByHandle(
            JobHandle,
            JOB_OBJECT_TERMINATE,
            PsJobType,
            PreviousMode,
            (PVOID *)&Job,
            NULL
            );
    if ( !NT_SUCCESS(st) ) {
        return st;
        }

    KeEnterCriticalRegion();
    ExAcquireResourceExclusive(&Job->JobLock, TRUE);

    PspTerminateAllProcessesInJob(Job,ExitStatus,PsLockPollOnTimeout);

    ExReleaseResource(&Job->JobLock);
    KeLeaveCriticalRegion();

    ObDereferenceObject(Job);

    return st;
}

VOID
PsEnforceExecutionTimeLimits(
    VOID
    )
{
    PLIST_ENTRY NextJob;
    PLIST_ENTRY NextProcess;
    LARGE_INTEGER RunningJobTime;
    LARGE_INTEGER ProcessTime;
    PEJOB Job;
    PEPROCESS Process;
    NTSTATUS st;

    ExAcquireFastMutex(&PspJobListLock);

    //
    // Look at each job. If time limits are set for the job, then enforce them
    //
    NextJob = PspJobList.Flink;
    while ( NextJob != &PspJobList ) {
        Job = (PEJOB)(CONTAINING_RECORD(NextJob,EJOB,JobLinks));
        if ( Job->LimitFlags & (JOB_OBJECT_LIMIT_PROCESS_TIME | JOB_OBJECT_LIMIT_JOB_TIME) ) {

            //
            // Job looks like a candidate for time enforcing. Need to get the
            // job lock to be sure, but we don't want to hang waiting for the
            // job lock, so skip the job until next time around if we need to
            //
            //

            if ( ExAcquireResourceExclusive(&Job->JobLock, FALSE) ) {

                if ( Job->LimitFlags & (JOB_OBJECT_LIMIT_PROCESS_TIME | JOB_OBJECT_LIMIT_JOB_TIME) ) {

                    //
                    // Job is setup for time limits
                    //

                    RunningJobTime.QuadPart = Job->ThisPeriodTotalUserTime.QuadPart;

                    NextProcess = Job->ProcessListHead.Flink;

                    while ( NextProcess != &Job->ProcessListHead) {

                        Process = (PEPROCESS)(CONTAINING_RECORD(NextProcess,EPROCESS,JobLinks));

                        ProcessTime.QuadPart = UInt32x32To64(Process->Pcb.UserTime,KeMaximumIncrement);

                        if ( !(Process->JobStatus & PS_JOB_STATUS_ACCOUNTING_FOLDED) ) {
                            RunningJobTime.QuadPart += ProcessTime.QuadPart;
                            }

                        if ( Job->LimitFlags & JOB_OBJECT_LIMIT_PROCESS_TIME ) {
                            if ( ProcessTime.QuadPart > Job->PerProcessUserTimeLimit.QuadPart ) {

                                //
                                // Process Time Limit has been exceeded.
                                //
                                // Reference the process. Assert that it is not in its
                                // delete routine. If all is OK, then nuke and dereferece
                                // the process
                                //

                                ObReferenceObject(Process);

                                //
                                // Avoid double delete since process could be in delete routine during the above ref
                                //
                                if ( ObGetObjectPointerCount(Process) > 1 ) {

                                    if ( !(Process->JobStatus & PS_JOB_STATUS_NOT_REALLY_ACTIVE) ) {
                                        if ( PspTerminateProcess(Process,ERROR_NOT_ENOUGH_QUOTA,PsLockReturnTimeout) == STATUS_SUCCESS ) {

                                            Job->TotalTerminatedProcesses++;
                                            PS_SET_CLEAR_BITS (&Process->JobStatus,
                                                               PS_JOB_STATUS_NOT_REALLY_ACTIVE,
                                                               PS_JOB_STATUS_LAST_REPORT_MEMORY);
                                            Job->ActiveProcesses--;

                                            if ( Job->CompletionPort ) {
                                                IoSetIoCompletion(
                                                    Job->CompletionPort,
                                                    Job->CompletionKey,
                                                    (PVOID)Process->UniqueProcessId,
                                                    STATUS_SUCCESS,
                                                    JOB_OBJECT_MSG_END_OF_PROCESS_TIME,
                                                    FALSE
                                                    );
                                                }
                                            PspFoldProcessAccountingIntoJob(Job,Process);

                                            }
                                        }
                                    ObDereferenceObject(Process);
                                    }
                                }
                            }

                        NextProcess = NextProcess->Flink;
                        }
                    if ( Job->LimitFlags & JOB_OBJECT_LIMIT_JOB_TIME ) {
                        if ( RunningJobTime.QuadPart > Job->PerJobUserTimeLimit.QuadPart ) {

                            //
                            // Job Time Limit has been exceeded.
                            //
                            // Perform the appropriate action
                            //

                            switch ( Job->EndOfJobTimeAction ) {

                            case JOB_OBJECT_TERMINATE_AT_END_OF_JOB:
                                if ( PspTerminateAllProcessesInJob(Job,ERROR_NOT_ENOUGH_QUOTA,PsLockReturnTimeout) ) {
                                    if ( Job->ActiveProcesses == 0 ) {
                                        KeSetEvent(&Job->Event,0,FALSE);
                                        if ( Job->CompletionPort ) {
                                            PS_CLEAR_BITS (&Process->JobStatus, PS_JOB_STATUS_LAST_REPORT_MEMORY);
                                            IoSetIoCompletion(
                                                Job->CompletionPort,
                                                Job->CompletionKey,
                                                NULL,
                                                STATUS_SUCCESS,
                                                JOB_OBJECT_MSG_END_OF_JOB_TIME,
                                                FALSE
                                                );
                                            }
                                        }
                                    }
                                break;

                            case JOB_OBJECT_POST_AT_END_OF_JOB:

                                if ( Job->CompletionPort ) {
                                    PS_CLEAR_BITS (&Process->JobStatus, PS_JOB_STATUS_LAST_REPORT_MEMORY);
                                    st = IoSetIoCompletion(
                                            Job->CompletionPort,
                                            Job->CompletionKey,
                                            NULL,
                                            STATUS_SUCCESS,
                                            JOB_OBJECT_MSG_END_OF_JOB_TIME,
                                            FALSE
                                            );
                                    if ( NT_SUCCESS(st) ) {

                                        //
                                        // Clear job level time limit
                                        //

                                        Job->LimitFlags &= ~JOB_OBJECT_LIMIT_JOB_TIME;
                                        Job->PerJobUserTimeLimit.QuadPart = 0;
                                        }
                                    }
                                else {
                                    if ( PspTerminateAllProcessesInJob(Job,ERROR_NOT_ENOUGH_QUOTA,PsLockReturnTimeout) ) {
                                        if ( Job->ActiveProcesses == 0 ) {
                                            KeSetEvent(&Job->Event,0,FALSE);
                                            }
                                        }
                                    }
                                break;
                                }
                            }

                        }

                    }
                ExReleaseResource(&Job->JobLock);
                }
            }
        NextJob = NextJob->Flink;
        }
    ExReleaseFastMutex(&PspJobListLock);
}

BOOLEAN
PspTerminateAllProcessesInJob(
    PEJOB Job,
    NTSTATUS Status,
    PSLOCKPROCESSMODE LockMode
    )
{
    PLIST_ENTRY NextProcess;
    PEPROCESS Process;
    BOOLEAN TerminatedAProcess;

    PAGED_CODE();

    TerminatedAProcess = FALSE;
    NextProcess = Job->ProcessListHead.Flink;

    while ( NextProcess != &Job->ProcessListHead) {

        Process = (PEPROCESS)(CONTAINING_RECORD(NextProcess,EPROCESS,JobLinks));

        //
        // Reference the process. Assert that it is not in its
        // delete routine. If all is OK, then nuke and dereferece
        // the process
        //

        ObReferenceObject(Process);

        //
        // Avoid double delete since process could be in delete routine during the above ref
        //
        if ( ObGetObjectPointerCount(Process) > 1 ) {
            if ( !(Process->JobStatus & PS_JOB_STATUS_NOT_REALLY_ACTIVE) ) {

                if ( PspTerminateProcess(Process,Status,LockMode) == STATUS_SUCCESS ) {

                    //
                    // If the lockmode isn't poll, it means we ran out of
                    // job time, so increment the terminated process count
                    // for each nuked process
                    //
                    if ( LockMode != PsLockPollOnTimeout ) {
                        Job->TotalTerminatedProcesses++;
                        }
                    PS_SET_BITS (&Process->JobStatus, PS_JOB_STATUS_NOT_REALLY_ACTIVE);
                    Job->ActiveProcesses--;

                    PspFoldProcessAccountingIntoJob(Job,Process);

                    TerminatedAProcess = TRUE;
                    }
                }

            ObDereferenceObject(Process);
            }

        NextProcess = NextProcess->Flink;
        }
    return TerminatedAProcess;
}


VOID
PspFoldProcessAccountingIntoJob(
    PEJOB Job,
    PEPROCESS Process
    )
{
    LARGE_INTEGER UserTime, KernelTime;

    if ( !(Process->JobStatus & PS_JOB_STATUS_ACCOUNTING_FOLDED) ) {
        UserTime.QuadPart = UInt32x32To64(Process->Pcb.UserTime,KeMaximumIncrement);
        KernelTime.QuadPart = UInt32x32To64(Process->Pcb.KernelTime,KeMaximumIncrement);

        Job->TotalUserTime.QuadPart += UserTime.QuadPart;
        Job->TotalKernelTime.QuadPart += KernelTime.QuadPart;
        Job->ThisPeriodTotalUserTime.QuadPart += UserTime.QuadPart;
        Job->ThisPeriodTotalKernelTime.QuadPart += KernelTime.QuadPart;

        Job->ReadOperationCount += Process->ReadOperationCount.QuadPart;
        Job->WriteOperationCount += Process->WriteOperationCount.QuadPart;
        Job->OtherOperationCount += Process->OtherOperationCount.QuadPart;
        Job->ReadTransferCount += Process->ReadTransferCount.QuadPart;
        Job->WriteTransferCount += Process->WriteTransferCount.QuadPart;
        Job->OtherTransferCount += Process->OtherTransferCount.QuadPart;

        Job->TotalPageFaultCount += Process->Vm.PageFaultCount;


        if ( Process->CommitChargePeak > Job->PeakProcessMemoryUsed ) {
            Job->PeakProcessMemoryUsed = Process->CommitChargePeak;
            }

        PS_SET_CLEAR_BITS (&Process->JobStatus,
                           PS_JOB_STATUS_ACCOUNTING_FOLDED,
                           PS_JOB_STATUS_LAST_REPORT_MEMORY);

        if ( Job->CompletionPort && Job->ActiveProcesses == 0) {
            IoSetIoCompletion(
                Job->CompletionPort,
                Job->CompletionKey,
                NULL,
                STATUS_SUCCESS,
                JOB_OBJECT_MSG_ACTIVE_PROCESS_ZERO,
                FALSE
                );
            }
        }
}

NTSTATUS
PspCaptureTokenFilter(
    KPROCESSOR_MODE PreviousMode,
    PJOBOBJECT_SECURITY_LIMIT_INFORMATION SecurityLimitInfo,
    PPS_JOB_TOKEN_FILTER * TokenFilter
    )
{
    NTSTATUS Status ;
    PPS_JOB_TOKEN_FILTER Filter ;

    Filter = ExAllocatePoolWithTag( NonPagedPool,
                                    sizeof( PS_JOB_TOKEN_FILTER ),
                                    'fTsP' );

    if ( !Filter )
    {
        *TokenFilter = NULL ;

        return STATUS_INSUFFICIENT_RESOURCES ;
    }

    RtlZeroMemory( Filter, sizeof( PS_JOB_TOKEN_FILTER ) );

    try {

        Status = STATUS_SUCCESS ;

        //
        //  Capture Sids to remove
        //

        if (ARGUMENT_PRESENT( SecurityLimitInfo->SidsToDisable)) {

            ProbeForRead( SecurityLimitInfo->SidsToDisable,
                          sizeof(TOKEN_GROUPS),
                          sizeof(ULONG) );

            Filter->CapturedGroupCount = SecurityLimitInfo->SidsToDisable->GroupCount;

            Status = SeCaptureSidAndAttributesArray(
                        SecurityLimitInfo->SidsToDisable->Groups,
                        Filter->CapturedGroupCount,
                        PreviousMode,
                        NULL, 0,
                        NonPagedPool,
                        TRUE,
                        &Filter->CapturedGroups,
                        &Filter->CapturedGroupsLength
                        );

        }

        //
        //  Capture PrivilegesToDelete
        //

        if (NT_SUCCESS(Status) &&
            ARGUMENT_PRESENT(SecurityLimitInfo->PrivilegesToDelete)) {

            ProbeForRead( SecurityLimitInfo->PrivilegesToDelete,
                          sizeof(TOKEN_PRIVILEGES),
                          sizeof(ULONG) );

            Filter->CapturedPrivilegeCount = SecurityLimitInfo->PrivilegesToDelete->PrivilegeCount;

            Status = SeCaptureLuidAndAttributesArray(
                         SecurityLimitInfo->PrivilegesToDelete->Privileges,
                         Filter->CapturedPrivilegeCount,
                         PreviousMode,
                         NULL, 0,
                         NonPagedPool,
                         TRUE,
                         &Filter->CapturedPrivileges,
                         &Filter->CapturedPrivilegesLength
                         );

        }

        //
        //  Capture Restricted Sids
        //

        if (NT_SUCCESS(Status) &&
            ARGUMENT_PRESENT(SecurityLimitInfo->RestrictedSids)) {

            ProbeForRead( SecurityLimitInfo->RestrictedSids,
                          sizeof(TOKEN_GROUPS),
                          sizeof(ULONG) );

            Filter->CapturedSidCount = SecurityLimitInfo->RestrictedSids->GroupCount;

            Status = SeCaptureSidAndAttributesArray(
                        SecurityLimitInfo->RestrictedSids->Groups,
                        Filter->CapturedSidCount,
                        PreviousMode,
                        NULL, 0,
                        NonPagedPool,
                        TRUE,
                        &Filter->CapturedSids,
                        &Filter->CapturedSidsLength
                        );

        }



    } except(EXCEPTION_EXECUTE_HANDLER) {

        Status = GetExceptionCode();
    }  // end_try

    if ( !NT_SUCCESS( Status ) )
    {
        if ( Filter->CapturedSids )
        {
            ExFreePool( Filter->CapturedSids );
        }

        if ( Filter->CapturedPrivileges )
        {
            ExFreePool( Filter->CapturedPrivileges );
        }

        if ( Filter->CapturedGroups )
        {
            ExFreePool( Filter->CapturedGroups );
        }

        ExFreePool( Filter );

        Filter = NULL ;

    }

    *TokenFilter = Filter ;

    return Status ;


}



BOOLEAN
PsChangeJobMemoryUsage(
    SSIZE_T Amount
    )
{
    PEPROCESS Process;
    PEJOB Job;
    SIZE_T CurrentJobMemoryUsed;
    BOOLEAN ReturnValue;

    ReturnValue = TRUE;
    Process = PsGetCurrentProcess();
    Job = Process->Job;
    if ( Job ) {
        //
        // This routine can be called while hoolding the process lock (during
        // teb deletion... So instead of using the job lock, we must use the
        // memory limits lock. The lock order is always (job lock followed by
        // process lock. The memory limits lock never nests or calls other
        // code while held. It can be grapped while holding the job lock, or the process
        // lock.
        //
        KeEnterCriticalRegion();
        ExAcquireFastMutexUnsafe(&Job->MemoryLimitsLock);

        CurrentJobMemoryUsed = Job->CurrentJobMemoryUsed + Amount;

        if ( Job->LimitFlags & JOB_OBJECT_LIMIT_JOB_MEMORY &&
             CurrentJobMemoryUsed > Job->JobMemoryLimit ) {
            CurrentJobMemoryUsed = Job->CurrentJobMemoryUsed;
            ReturnValue = FALSE;



            //
            // Tell the job port that commit has been exceeded, and process id x
            // was the one that hit it.
            //

            if ( Job->CompletionPort
                 && Process->UniqueProcessId
                 && (Process->JobStatus & PS_JOB_STATUS_NEW_PROCESS_REPORTED)
                 && (Process->JobStatus & PS_JOB_STATUS_LAST_REPORT_MEMORY) == 0) {

                PS_SET_BITS (&Process->JobStatus, PS_JOB_STATUS_LAST_REPORT_MEMORY);
                IoSetIoCompletion(
                    Job->CompletionPort,
                    Job->CompletionKey,
                    (PVOID)Process->UniqueProcessId,
                    STATUS_SUCCESS,
                    JOB_OBJECT_MSG_JOB_MEMORY_LIMIT,
                    TRUE
                    );

                }
            }

        if ( ReturnValue ) {
            //
            // update current and peak counters
            //
            Job->CurrentJobMemoryUsed = CurrentJobMemoryUsed;
            if ( CurrentJobMemoryUsed > Job->PeakJobMemoryUsed ) {
                Job->PeakJobMemoryUsed = CurrentJobMemoryUsed;
                }

            if ( Process->CommitCharge + Amount > Job->PeakProcessMemoryUsed ) {
                Job->PeakProcessMemoryUsed = Process->CommitCharge + Amount;
                }
            }
        ExReleaseFastMutexUnsafe(&Job->MemoryLimitsLock);
        KeLeaveCriticalRegion();
        }

    return ReturnValue;
}


VOID
PsReportProcessMemoryLimitViolation(
    VOID
    )
{
    PEPROCESS Process;
    PEJOB Job;

    Process = PsGetCurrentProcess();
    Job = Process->Job;
    if ( Job && (Job->LimitFlags & JOB_OBJECT_LIMIT_PROCESS_MEMORY) ) {
        KeEnterCriticalRegion();
        ExAcquireFastMutexUnsafe(&Job->MemoryLimitsLock);

        //
        // Tell the job port that commit has been exceeded, and process id x
        // was the one that hit it.
        //

        if ( Job->CompletionPort
             && Process->UniqueProcessId
             && (Process->JobStatus & PS_JOB_STATUS_NEW_PROCESS_REPORTED)
             && (Process->JobStatus & PS_JOB_STATUS_LAST_REPORT_MEMORY) == 0) {

            PS_SET_BITS (&Process->JobStatus, PS_JOB_STATUS_LAST_REPORT_MEMORY);
            IoSetIoCompletion(
                Job->CompletionPort,
                Job->CompletionKey,
                (PVOID)Process->UniqueProcessId,
                STATUS_SUCCESS,
                JOB_OBJECT_MSG_PROCESS_MEMORY_LIMIT,
                TRUE
                );

            }
        ExReleaseFastMutexUnsafe(&Job->MemoryLimitsLock);
        KeLeaveCriticalRegion();

        }
}
