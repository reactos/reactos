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

static GENERIC_MAPPING PiJobMapping = {PROCESS_READ,
                                       PROCESS_WRITE,
                                       PROCESS_EXECUTE,
                                       PROCESS_ALL_ACCESS};

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
 * @unimplemented
 */
NTSTATUS
STDCALL
NtIsProcessInJob(IN HANDLE ProcessHandle, // ProcessHandle must PROCESS_QUERY_INFORMATION grant access.
                 IN HANDLE JobHandle OPTIONAL) // JobHandle must grant JOB_OBJECT_QUERY access. Defaults to the current process's job object.
{
  UNIMPLEMENTED;
  return STATUS_NOT_IMPLEMENTED;
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
  assert(Job);
  return (PVOID)&Job->JobLock;
}


/*
 * @implemented
 */
PVOID
STDCALL
PsGetJobSessionId(PEJOB Job)
{
  assert(Job);
  return (PVOID)Job->SessionId;
}


/*
 * @implemented
 */
ULONG
STDCALL
PsGetJobUIRestrictionsClass(PEJOB Job)
{
  assert(Job);
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
  assert(Job);
  InterlockedExchange((LONG*)&Job->UIRestrictionsClass, (LONG)UIRestrictionsClass);
  /* FIXME - walk through the job process list and update the restrictions? */
}

/* EOF */
