/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           Security manager
 * FILE:              kernel/se/semgr.c
 * PROGRAMER:         ?
 * REVISION HISTORY:
 *                 26/07/98: Added stubs for security functions
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>

#include <internal/debug.h>

/* FUNCTIONS ***************************************************************/

NTSTATUS STDCALL NtQueryInformationToken(VOID)
{
}

NTSTATUS STDCALL NtQuerySecurityObject(VOID)
{
}

NTSTATUS STDCALL NtSetSecurityObject(VOID)
{
}

NTSTATUS STDCALL NtSetInformationToken(VOID)
{
}

NTSTATUS STDCALL NtPrivilegeCheck(VOID)
{
}

NTSTATUS STDCALL NtPrivilegedServiceAuditAlarm(VOID)
{
}

NTSTATUS STDCALL NtPrivilegeObjectAuditAlarm(VOID)
{
}

NTSTATUS STDCALL NtOpenObjectAuditAlarm(VOID)
{
}

NTSTATUS STDCALL NtOpenProcessToken(VOID)
{
}

NTSTATUS STDCALL NtOpenThreadToken(VOID)
{
}

NTSTATUS STDCALL NtDuplicateToken(VOID)
{
}

NTSTATUS STDCALL NtImpersonateClientOfPort(VOID)
{
}

NTSTATUS STDCALL NtImpersonateThread(VOID)
{
}

NTSTATUS STDCALL NtCreateToken(VOID)
{
}

NTSTATUS STDCALL NtDeleteObjectAuditAlarm(VOID)
{
}


NTSTATUS
STDCALL
NtAllocateLocallyUniqueId(
	OUT PVOID LocallyUniqueId
	)
{
}

NTSTATUS
STDCALL
ZwAllocateLocallyUniqueId(
	OUT PVOID LocallyUniqueId
	)
{
}

NTSTATUS STDCALL NtAccessCheckAndAuditAlarm(VOID)
{
}

NTSTATUS STDCALL NtAdjustGroupsToken(VOID)
{
}

NTSTATUS STDCALL NtAdjustPrivilegesToken(VOID)
{
}

NTSTATUS STDCALL NtAllocateUuids(VOID)
{
}

NTSTATUS STDCALL NtCloseObjectAuditAlarm(VOID)
{
}

NTSTATUS
STDCALL
NtAccessCheck(
	IN PSECURITY_DESCRIPTOR SecurityDescriptor,
	IN HANDLE ClientToken,
	IN ULONG DesiredAcces,
	IN PGENERIC_MAPPING GenericMapping,
	OUT PRIVILEGE_SET PrivilegeSet,
	OUT PULONG ReturnLength,
	OUT PULONG GrantedAccess,
	OUT PULONG AccessStatus
	)
{
}

NTSTATUS
STDCALL
ZwAccessCheck(
	IN PSECURITY_DESCRIPTOR SecurityDescriptor,
	IN HANDLE ClientToken,
	IN ULONG DesiredAcces,
	IN PGENERIC_MAPPING GenericMapping,
	OUT PRIVILEGE_SET PrivilegeSet,
	OUT PULONG ReturnLength,
	OUT PULONG GrantedAccess,
	OUT PULONG AccessStatus
	)
{
}

NTSTATUS RtlCreateSecurityDescriptor(PSECURITY_DESCRIPTOR SecurityDescriptor,
				     ULONG Revision)
{
   UNIMPLEMENTED;
}

ULONG RtlLengthSecurityDescriptor(PSECURITY_DESCRIPTOR SecurityDescriptor)
{
   UNIMPLEMENTED;
}

NTSTATUS RtlSetDaclSecurityDescriptor(PSECURITY_DESCRIPTOR SecurityDescriptor,
				      BOOLEAN DaclPresent,
				      PACL Dacl,
				      BOOLEAN DaclDefaulted)
{
   UNIMPLEMENTED;
}

BOOLEAN RtlValidSecurityDescriptor(PSECURITY_DESCRIPTOR SecurityDescriptor)
{
   UNIMPLEMENTED;
}

BOOLEAN SeSinglePrivilegeCheck(LUID PrivilegeValue,
			       KPROCESSOR_MODE PreviousMode)
{
   UNIMPLEMENTED;
}

NTSTATUS SeDeassignSecurity(PSECURITY_DESCRIPTOR* SecurityDescriptor)
{
   UNIMPLEMENTED;
}

NTSTATUS SeAssignSecurity(PSECURITY_DESCRIPTOR ParentDescriptor,
			  PSECURITY_DESCRIPTOR ExplicitDescriptor,
			  BOOLEAN IsDirectoryObject,
			  PSECURITY_SUBJECT_CONTEXT SubjectContext,
			  PGENERIC_MAPPING GenericMapping,
			  POOL_TYPE PoolType)
{
   UNIMPLEMENTED;
}

BOOLEAN SeAccessCheck(IN PSECURITY_DESCRIPTOR SecurityDescriptor,
		      IN PSECURITY_DESCRIPTOR_CONTEXT SubjectSecurityContext,
		      IN BOOLEAN SubjectContextLocked,
		      IN ACCESS_MASK DesiredAccess,
		      IN ACCESS_MASK PreviouslyGrantedAccess,
		      OUT PPRIVILEGE_SET* Privileges,
		      IN PGENERIC_MAPPING GenericMapping,
		      IN KPROCESSOR_MODE AccessMode,
		      OUT PACCESS_MODE GrantedAccess,
		      OUT PNTSTATUS AccessStatus)
/*
 * FUNCTION: Determines whether the requested access rights can be granted
 * to an object protected by a security descriptor and an object owner
 * ARGUMENTS:
 *      SecurityDescriptor = Security descriptor protected the object
 *      SubjectSecurityContext = Subject's captured security context
 *      SubjectContextLocked = Indicates the user's subject context is locked
 *      DesiredAccess = Access rights the caller is trying to acquire
 *      PreviouslyGrantedAccess = Specified the access rights already granted
 *      Priveleges = ?
 *      GenericMapping = Generic mapping associated with the object
 *      AccessMode = Access mode used for the check
 *      GrantedAccess (OUT) = On return specifies the access granted
 *      AccessStatus (OUT) = Status indicating why access was denied
 * RETURNS: If access was granted, returns TRUE
 */
{
   UNIMPLEMENTED;
}


