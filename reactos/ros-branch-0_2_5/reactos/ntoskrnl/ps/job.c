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
static KSPIN_LOCK PsJobListLock;

static GENERIC_MAPPING PiJobMapping = {STANDARD_RIGHTS_READ | JOB_OBJECT_QUERY,
                                       STANDARD_RIGHTS_WRITE | JOB_OBJECT_ASSIGN_PROCESS | JOB_OBJECT_SET_ATTRIBUTES | JOB_OBJECT_TERMINATE | JOB_OBJECT_SET_SECURITY_ATTRIBUTES,
                                       STANDARD_RIGHTS_EXECUTE | SYNCHRONIZE,
                                       STANDARD_RIGHTS_ALL | JOB_OBJECT_ALL_ACCESS};

/* FUNCTIONS *****************************************************************/

VOID STDCALL
PiDeleteJob(PVOID ObjectBody)
{
  KIRQL oldIrql;
  PEJOB Job = (PEJOB)ObjectBody;
  
  KeAcquireSpinLock(&PsJobListLock, &oldIrql);
  RemoveEntryList(&Job->JobLinks);
  KeReleaseSpinLock(&PsJobListLock, oldIrql);
}

VOID INIT_FUNCTION
PsInitJobManagment(VOID)
{
  PsJobType = ExAllocatePool(NonPagedPool, sizeof(OBJECT_TYPE));

  PsJobType->Tag = TAG('E', 'J', 'O', 'B');
  PsJobType->TotalObjects = 0;
  PsJobType->TotalHandles = 0;
  PsJobType->MaxObjects = ULONG_MAX;
  PsJobType->MaxHandles = ULONG_MAX;
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
  KeInitializeSpinLock(&PsJobListLock);
}

/*
 * @unimplemented
 */
NTSTATUS 
STDCALL 
NtAssignProcessToJobObject(HANDLE JobHandle,
                           HANDLE ProcessHandle)
{
  UNIMPLEMENTED;
  return STATUS_NOT_IMPLEMENTED;
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
  UNIMPLEMENTED;
  return STATUS_NOT_IMPLEMENTED;
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
  InterlockedExchange((LONG*)&Job->UIRestrictionsClass, (LONG)UIRestrictionsClass);
  /* FIXME - walk through the job process list and update the restrictions? */
}

/* EOF */
