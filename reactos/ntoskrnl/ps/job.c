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

#include <ddk/ntddk.h>

#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

/*
 * @unimplemented
 */
NTSTATUS 
STDCALL 
NtAssignProcessToJobObject(
	HANDLE JobHandle,
    HANDLE ProcessHandle
	)
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}
/*
 * @unimplemented
 */
NTSTATUS 
STDCALL 
NtCreateJobObject(
	PHANDLE JobHandle, 
	ACCESS_MASK DesiredAccess,
    POBJECT_ATTRIBUTES ObjectAttributes
	)
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}
/*
 * @unimplemented
 */
NTSTATUS
STDCALL
NtIsProcessInJob(
	IN HANDLE ProcessHandle,  // ProcessHandle must PROCESS_QUERY_INFORMATION grant access.
	IN HANDLE JobHandle OPTIONAL	// JobHandle must grant JOB_OBJECT_QUERY access. Defaults to the current process's job object.
	)
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}
/*
 * @unimplemented
 */
NTSTATUS 
STDCALL 
NtOpenJobObject(
	PHANDLE JobHandle, 
	ACCESS_MASK DesiredAccess,
    POBJECT_ATTRIBUTES ObjectAttributes
	)
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}
/*
 * @unimplemented
 */
NTSTATUS 
STDCALL 
NtQueryInformationJobObject(
	HANDLE JobHandle,
    JOBOBJECTINFOCLASS JobInformationClass, 
	PVOID JobInformation,
    ULONG JobInformationLength, 
	PULONG ReturnLength
	)
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}
/*
 * @unimplemented
 */
NTSTATUS 
STDCALL 
NtSetInformationJobObject(
	HANDLE JobHandle,
    JOBOBJECTINFOCLASS JobInformationClass,
    PVOID JobInformation, 
	ULONG JobInformationLength
	)
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
NTSTATUS 
STDCALL 
NtTerminateJobObject(
	HANDLE JobHandle, 
	NTSTATUS ExitStatus
	)
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
PVOID
STDCALL 
PsGetJobLock(
    PVOID */*PEJOB*/ Job
	)
{
	UNIMPLEMENTED;
	return 0;	
}



/*
 * @unimplemented
 */
PVOID
STDCALL
PsGetJobSessionId(
    PVOID /*PEJOB*/	Job
	)
{
	UNIMPLEMENTED;
	return 0;	
}

/*
 * @unimplemented
 */
ULONG
STDCALL
PsGetJobUIRestrictionsClass(
   	PVOID /*PEJOB*/	Job
	)
{
	UNIMPLEMENTED;
	return 0;	
}

/*
 * @unimplemented
 */                       
VOID
STDCALL
PsSetJobUIRestrictionsClass(
    PVOID /*PEJOB*/	Job,
    ULONG	UIRestrictionsClass	
	)
{
	UNIMPLEMENTED;
}
/* EOF */
