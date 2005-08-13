#ifndef _INCLUDE_NTOS_SEFUNCS_H
#define _INCLUDE_NTOS_SEFUNCS_H

NTSTATUS
STDCALL
SeCaptureSecurityDescriptor(
	IN PSECURITY_DESCRIPTOR OriginalSecurityDescriptor,
	IN KPROCESSOR_MODE CurrentMode,
	IN POOL_TYPE PoolType,
	IN BOOLEAN CaptureIfKernel,
	OUT PSECURITY_DESCRIPTOR *CapturedSecurityDescriptor
	);

VOID
STDCALL
SeCloseObjectAuditAlarm(
	IN PVOID Object,
	IN HANDLE Handle,
	IN BOOLEAN PerformAction
	);

NTSTATUS
STDCALL
SeCreateAccessState(
	PACCESS_STATE AccessState,
	PAUX_DATA AuxData,
	ACCESS_MASK Access,
	PGENERIC_MAPPING GenericMapping
	);

VOID STDCALL
SeDeleteAccessState(IN PACCESS_STATE AccessState);

VOID
STDCALL
SePrivilegeObjectAuditAlarm(
	IN HANDLE Handle,
	IN PSECURITY_SUBJECT_CONTEXT SubjectContext,
	IN ACCESS_MASK DesiredAccess,
	IN PPRIVILEGE_SET Privileges,
	IN BOOLEAN AccessGranted,
	IN KPROCESSOR_MODE CurrentMode
	);

NTSTATUS
STDCALL
SeReleaseSecurityDescriptor(
	IN PSECURITY_DESCRIPTOR CapturedSecurityDescriptor,
	IN KPROCESSOR_MODE CurrentMode,
	IN BOOLEAN CaptureIfKernelMode
	);

SECURITY_IMPERSONATION_LEVEL STDCALL
SeTokenImpersonationLevel(IN PACCESS_TOKEN Token);

BOOLEAN
STDCALL
SeTokenIsWriteRestricted(
	IN PACCESS_TOKEN Token
	);

#endif /* _INCLUDE_NTOS_SEFUNCS_H */

