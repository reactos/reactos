/* $Id: audit.c,v 1.4 2004/05/20 12:42:11 ekohl Exp $
 *
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           Audit functions
 * FILE:              kernel/se/audit.c
 * PROGRAMER:         Eric Kohl (ekohl@rz-online.de)
 * REVISION HISTORY:
 *                    20/07/2003: Created
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>

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

/* EOF */
