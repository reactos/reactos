/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/se/audit.c
 * PURPOSE:         Audit functions
 * 
 * PROGRAMMERS:     Eric Kohl <eric.kohl@t-online.de>
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#include <internal/debug.h>


/* FUNCTIONS ****************************************************************/

NTSTATUS STDCALL
NtAccessCheckAndAuditAlarm(IN PUNICODE_STRING SubsystemName,
			   IN PHANDLE ObjectHandle,
			   IN PUNICODE_STRING ObjectTypeName,
			   IN PUNICODE_STRING ObjectName,
			   IN PSECURITY_DESCRIPTOR SecurityDescriptor,
			   IN ACCESS_MASK DesiredAccess,
			   IN PGENERIC_MAPPING GenericMapping,
			   IN BOOLEAN ObjectCreation,
			   OUT PACCESS_MASK GrantedAccess,
			   OUT PNTSTATUS AccessStatus,
			   OUT PBOOLEAN GenerateOnClose
	)
{
  UNIMPLEMENTED;
  return(STATUS_NOT_IMPLEMENTED);
}


NTSTATUS STDCALL
NtCloseObjectAuditAlarm(IN PUNICODE_STRING SubsystemName,
			IN PVOID HandleId,
			IN BOOLEAN GenerateOnClose)
{
  UNIMPLEMENTED;
  return(STATUS_NOT_IMPLEMENTED);
}


NTSTATUS STDCALL
NtDeleteObjectAuditAlarm(IN PUNICODE_STRING SubsystemName,
			 IN PVOID HandleId,
			 IN BOOLEAN GenerateOnClose)
{
  UNIMPLEMENTED;
  return(STATUS_NOT_IMPLEMENTED);
}


NTSTATUS STDCALL
NtOpenObjectAuditAlarm(IN PUNICODE_STRING SubsystemName,
		       IN PVOID HandleId,
		       IN PUNICODE_STRING ObjectTypeName,
		       IN PUNICODE_STRING ObjectName,
		       IN PSECURITY_DESCRIPTOR SecurityDescriptor,
		       IN HANDLE ClientToken,
		       IN ULONG DesiredAccess,
		       IN ULONG GrantedAccess,
		       IN PPRIVILEGE_SET Privileges,
		       IN BOOLEAN ObjectCreation,
		       IN BOOLEAN AccessGranted,
		       OUT PBOOLEAN GenerateOnClose)
{
  UNIMPLEMENTED;
  return(STATUS_NOT_IMPLEMENTED);
}


NTSTATUS STDCALL
NtPrivilegedServiceAuditAlarm(IN PUNICODE_STRING SubsystemName,
			      IN PUNICODE_STRING ServiceName,
			      IN HANDLE ClientToken,
			      IN PPRIVILEGE_SET Privileges,
			      IN BOOLEAN AccessGranted)
{
  UNIMPLEMENTED;
  return(STATUS_NOT_IMPLEMENTED);
}


NTSTATUS STDCALL
NtPrivilegeObjectAuditAlarm(IN PUNICODE_STRING SubsystemName,
			    IN PVOID HandleId,
			    IN HANDLE ClientToken,
			    IN ULONG DesiredAccess,
			    IN PPRIVILEGE_SET Privileges,
			    IN BOOLEAN AccessGranted)
{
  UNIMPLEMENTED;
  return(STATUS_NOT_IMPLEMENTED);
}


/*
 * @unimplemented
 */
VOID
STDCALL
SeAuditHardLinkCreation(
	IN PUNICODE_STRING FileName,
	IN PUNICODE_STRING LinkName,
	IN BOOLEAN bSuccess
	)
{
	UNIMPLEMENTED;
}

/*
 * @unimplemented
 */
BOOLEAN
STDCALL
SeAuditingFileEvents(
	IN BOOLEAN AccessGranted,
	IN PSECURITY_DESCRIPTOR SecurityDescriptor
	)
{
	UNIMPLEMENTED;
	return FALSE;
}

/*
 * @unimplemented
 */
BOOLEAN
STDCALL
SeAuditingFileEventsWithContext(
	IN BOOLEAN AccessGranted,
	IN PSECURITY_DESCRIPTOR SecurityDescriptor,
	IN PSECURITY_SUBJECT_CONTEXT SubjectSecurityContext OPTIONAL
	)
{
	UNIMPLEMENTED;
	return FALSE;
}

/*
 * @unimplemented
 */
BOOLEAN
STDCALL
SeAuditingHardLinkEvents(
	IN BOOLEAN AccessGranted,
	IN PSECURITY_DESCRIPTOR SecurityDescriptor
	)
{
	UNIMPLEMENTED;
	return FALSE;
}

/*
 * @unimplemented
 */
BOOLEAN
STDCALL
SeAuditingHardLinkEventsWithContext(
	IN BOOLEAN AccessGranted,
	IN PSECURITY_DESCRIPTOR SecurityDescriptor,
	IN PSECURITY_SUBJECT_CONTEXT SubjectSecurityContext OPTIONAL
	)
{
	UNIMPLEMENTED;
	return FALSE;
}

/*
 * @unimplemented
 */
BOOLEAN
STDCALL
SeAuditingFileOrGlobalEvents(
	IN BOOLEAN AccessGranted,
	IN PSECURITY_DESCRIPTOR SecurityDescriptor,
	IN PSECURITY_SUBJECT_CONTEXT SubjectSecurityContext
	)
{
	UNIMPLEMENTED;
	return FALSE;
}

/*
 * @unimplemented
 */
VOID
STDCALL
SeCloseObjectAuditAlarm(
	IN PVOID Object,
	IN HANDLE Handle,
	IN BOOLEAN PerformAction
	)
{
	UNIMPLEMENTED;
}

/*
 * @unimplemented
 */
VOID STDCALL
SeDeleteObjectAuditAlarm(IN PVOID Object,
			 IN HANDLE Handle)
{
  UNIMPLEMENTED;
}


/*
 * @unimplemented
 */
VOID STDCALL
SeOpenObjectAuditAlarm(IN PUNICODE_STRING ObjectTypeName,
		       IN PVOID Object OPTIONAL,
		       IN PUNICODE_STRING AbsoluteObjectName OPTIONAL,
		       IN PSECURITY_DESCRIPTOR SecurityDescriptor,
		       IN PACCESS_STATE AccessState,
		       IN BOOLEAN ObjectCreated,
		       IN BOOLEAN AccessGranted,
		       IN KPROCESSOR_MODE AccessMode,
		       OUT PBOOLEAN GenerateOnClose)
{
  UNIMPLEMENTED;
}


/*
 * @unimplemented
 */
VOID STDCALL
SeOpenObjectForDeleteAuditAlarm(IN PUNICODE_STRING ObjectTypeName,
				IN PVOID Object OPTIONAL,
				IN PUNICODE_STRING AbsoluteObjectName OPTIONAL,
				IN PSECURITY_DESCRIPTOR SecurityDescriptor,
				IN PACCESS_STATE AccessState,
				IN BOOLEAN ObjectCreated,
				IN BOOLEAN AccessGranted,
				IN KPROCESSOR_MODE AccessMode,
				OUT PBOOLEAN GenerateOnClose)
{
  UNIMPLEMENTED;
}

/*
 * @unimplemented
 */
VOID
STDCALL
SePrivilegeObjectAuditAlarm(
	IN HANDLE Handle,
	IN PSECURITY_SUBJECT_CONTEXT SubjectContext,
	IN ACCESS_MASK DesiredAccess,
	IN PPRIVILEGE_SET Privileges,
	IN BOOLEAN AccessGranted,
	IN KPROCESSOR_MODE CurrentMode
	)
{
	UNIMPLEMENTED;
}

/* EOF */
