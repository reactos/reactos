/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ps/jobs.c
 * PURPOSE:         Job Native Functions
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 * UPDATE HISTORY:
 *                  Created 16/07/04
 */

/* Note: Jobs are only supported on Win2K+ */
/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#include <internal/debug.h>

/* GLOBALS *******************************************************************/

POBJECT_TYPE EXPORTED PsJobType = NULL;

LIST_ENTRY PsJobListHead;
static FAST_MUTEX PsJobListLock;

static GENERIC_MAPPING PiJobMapping = {STANDARD_RIGHTS_READ | JOB_OBJECT_QUERY,
                                       STANDARD_RIGHTS_WRITE | JOB_OBJECT_ASSIGN_PROCESS | JOB_OBJECT_SET_ATTRIBUTES | JOB_OBJECT_TERMINATE | JOB_OBJECT_SET_SECURITY_ATTRIBUTES,
                                       STANDARD_RIGHTS_EXECUTE | SYNCHRONIZE,
                                       STANDARD_RIGHTS_ALL | JOB_OBJECT_ALL_ACCESS};

/* FUNCTIONS *****************************************************************/

VOID STDCALL
PiDeleteJob(PVOID ObjectBody)
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

VOID INIT_FUNCTION
PsInitJobManagment(VOID)
{
  PsJobType = ExAllocatePool(NonPagedPool, sizeof(OBJECT_TYPE));

  PsJobType->Tag = TAG('E', 'J', 'O', 'B');
  PsJobType->TotalObjects = 0;
  PsJobType->TotalHandles = 0;
  PsJobType->PeakObjects = 0;
  PsJobType->PeakHandles = 0;
  PsJobType->PagedPoolCharge = 0;
  PsJobType->NonpagedPoolCharge = sizeof(EJOB);
  PsJobType->Mapping = &PiJobMapping;
  PsJobType->Dump = NULL;
  PsJobType->Open = NULL;
  PsJobType->Close = NULL;
  PsJobType->Delete = PiDeleteJob;
  PsJobType->Parse = NULL;
  PsJobType->Security = NULL;
  PsJobType->QueryName = NULL;
  PsJobType->OkayToClose = NULL;
  PsJobType->Create = NULL;
  PsJobType->DuplicationNotify = NULL;
  
  RtlRosInitUnicodeStringFromLiteral(&PsJobType->TypeName, L"Job");
  
  ObpCreateTypeObject(PsJobType);
  
  InitializeListHead(&PsJobListHead);
  ExInitializeFastMutex(&PsJobListLock);
}

NTSTATUS
PspAssignProcessToJob(PEPROCESS Process,
                      PEJOB Job)
{
  DPRINT("PspAssignProcessToJob() is unimplemented!\n");
  return STATUS_NOT_IMPLEMENTED;
}


/*
 * @unimplemented
 */
NTSTATUS 
STDCALL 
NtAssignProcessToJobObject(HANDLE JobHandle,
                           HANDLE ProcessHandle)
{
  PEPROCESS Process;
  KPROCESSOR_MODE PreviousMode;
  NTSTATUS Status;
  
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
  if(NT_SUCCESS(Status))
  {
    if(Process->Job == NULL)
    {
      PEJOB Job;
      
      Status = ObReferenceObjectByHandle(JobHandle,
                                         JOB_OBJECT_ASSIGN_PROCESS,
                                         PsJobType,
                                         PreviousMode,
                                         (PVOID*)&Job,
                                         NULL);
      if(NT_SUCCESS(Status))
      {
        /* lock the process so we can safely assign the process. Note that in the
           meanwhile another thread could have assigned this process to a job! */

        Status = PsLockProcess(Process, FALSE);
        if(NT_SUCCESS(Status))
        {
          if(Process->Job == NULL && Process->SessionId == Job->SessionId)
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
          PsUnlockProcess(Process);
          
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


/*
 * @unimplemented
 */
NTSTATUS 
STDCALL 
NtCreateJobObject(PHANDLE JobHandle,
                  ACCESS_MASK DesiredAccess,
                  POBJECT_ATTRIBUTES ObjectAttributes)
{
  HANDLE hJob;
  PEJOB Job;
  KPROCESSOR_MODE PreviousMode;
  PEPROCESS CurrentProcess;
  NTSTATUS Status = STATUS_SUCCESS;

  PreviousMode = ExGetPreviousMode();
  CurrentProcess = PsGetCurrentProcess();

  /* check for valid buffers */
  if(PreviousMode == UserMode)
  {
    _SEH_TRY
    {
      /* probe with 32bit alignment */
      ProbeForWrite(JobHandle,
                    sizeof(HANDLE),
                    sizeof(ULONG));
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
    Job->SessionId = CurrentProcess->SessionId; /* inherit the session id from the caller */
    
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
  
  return Status;
}


/*
 * @implemented
 */
NTSTATUS
STDCALL
NtIsProcessInJob(IN HANDLE ProcessHandle,
                 IN HANDLE JobHandle OPTIONAL)
{
  KPROCESSOR_MODE PreviousMode;
  PEPROCESS Process;
  NTSTATUS Status;
  
  PreviousMode = ExGetPreviousMode();
  
  Status = ObReferenceObjectByHandle(ProcessHandle,
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
 * @unimplemented
 */
NTSTATUS 
STDCALL 
NtOpenJobObject(PHANDLE JobHandle,
                ACCESS_MASK DesiredAccess,
                POBJECT_ATTRIBUTES ObjectAttributes)
{
  UNIMPLEMENTED;
  return STATUS_NOT_IMPLEMENTED;
}


/*
 * @unimplemented
 */
NTSTATUS 
STDCALL 
NtQueryInformationJobObject(HANDLE JobHandle,
                            JOBOBJECTINFOCLASS JobInformationClass,
                            PVOID JobInformation,
                            ULONG JobInformationLength,
                            PULONG ReturnLength)
{
  UNIMPLEMENTED;
  return STATUS_NOT_IMPLEMENTED;
}


/*
 * @unimplemented
 */
NTSTATUS 
STDCALL 
NtSetInformationJobObject(HANDLE JobHandle,
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
NtTerminateJobObject(HANDLE JobHandle,
                     NTSTATUS ExitStatus)
{
  UNIMPLEMENTED;
  return STATUS_NOT_IMPLEMENTED;
}


/*
 * @implemented
 */
PVOID
STDCALL 
PsGetJobLock(PEJOB Job)
{
  ASSERT(Job);
  return (PVOID)&Job->JobLock;
}


/*
 * @implemented
 */
PVOID
STDCALL
PsGetJobSessionId(PEJOB Job)
{
  ASSERT(Job);
  return (PVOID)Job->SessionId;
}


/*
 * @implemented
 */
ULONG
STDCALL
PsGetJobUIRestrictionsClass(PEJOB Job)
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
  InterlockedExchangeUL(&Job->UIRestrictionsClass, UIRestrictionsClass);
  /* FIXME - walk through the job process list and update the restrictions? */
}

/* EOF */
