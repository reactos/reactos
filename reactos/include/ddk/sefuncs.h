#ifndef _INCLUDE_DDK_SEFUNCS_H
#define _INCLUDE_DDK_SEFUNCS_H
/* $Id: sefuncs.h,v 1.19 2002/09/08 10:47:45 chorns Exp $ */

BOOLEAN STDCALL
SeAccessCheck(IN PSECURITY_DESCRIPTOR SecurityDescriptor,
	      IN PSECURITY_SUBJECT_CONTEXT SubjectSecurityContext,
	      IN BOOLEAN SubjectContextLocked,
	      IN ACCESS_MASK DesiredAccess,
	      IN ACCESS_MASK PreviouslyGrantedAccess,
	      OUT PPRIVILEGE_SET* Privileges OPTIONAL,
	      IN PGENERIC_MAPPING GenericMapping,
	      IN KPROCESSOR_MODE AccessMode,
	      OUT PACCESS_MODE GrantedAccess,
	      OUT PNTSTATUS AccessStatus);

NTSTATUS STDCALL
SeAppendPrivileges(IN PACCESS_STATE AccessState,
		   IN PPRIVILEGE_SET Privileges);

NTSTATUS STDCALL
SeAssignSecurity(IN PSECURITY_DESCRIPTOR ParentDescriptor OPTIONAL,
		 IN PSECURITY_DESCRIPTOR ExplicitDescriptor,
		 OUT PSECURITY_DESCRIPTOR* NewDescriptor,
		 IN BOOLEAN IsDirectoryObject,
		 IN PSECURITY_SUBJECT_CONTEXT SubjectContext,
		 IN PGENERIC_MAPPING GenericMapping,
		 IN POOL_TYPE PoolType);

BOOLEAN STDCALL
SeAuditingFileEvents(IN BOOLEAN AccessGranted,
		     IN PSECURITY_DESCRIPTOR SecurityDescriptor);

BOOLEAN STDCALL
SeAuditingFileOrGlobalEvents(IN BOOLEAN AccessGranted,
			     IN PSECURITY_DESCRIPTOR SecurityDescriptor,
			     IN PSECURITY_SUBJECT_CONTEXT SubjectContext);

VOID STDCALL
SeCaptureSubjectContext(OUT PSECURITY_SUBJECT_CONTEXT SubjectContext);

NTSTATUS STDCALL
SeCreateAccessState(OUT PACCESS_STATE AccessState,
		    IN PVOID AuxData,
		    IN ACCESS_MASK AccessMask,
		    IN PGENERIC_MAPPING Mapping);

NTSTATUS STDCALL
SeCreateClientSecurity(IN struct _ETHREAD *Thread,
		       IN PSECURITY_QUALITY_OF_SERVICE Qos,
		       IN BOOLEAN RemoteClient,
		       OUT PSECURITY_CLIENT_CONTEXT ClientContext);

NTSTATUS STDCALL
SeDeassignSecurity(IN OUT PSECURITY_DESCRIPTOR* SecurityDescriptor);

VOID STDCALL
SeDeleteAccessState(IN PACCESS_STATE AccessState);

VOID STDCALL
SeDeleteObjectAuditAlarm(IN PVOID Object,
			 IN HANDLE Handle);

VOID STDCALL
SeFreePrivileges(IN PPRIVILEGE_SET Privileges);

VOID STDCALL
SeImpersonateClient(IN PSECURITY_CLIENT_CONTEXT ClientContext,
		    IN struct _ETHREAD *ServerThread OPTIONAL);

VOID STDCALL
SeLockSubjectContext(IN PSECURITY_SUBJECT_CONTEXT SubjectContext);

NTSTATUS STDCALL
SeMarkLogonSessionForTerminationNotification(IN PLUID LogonId);

VOID STDCALL
SeOpenObjectAuditAlarm(IN PUNICODE_STRING ObjectTypeName,
		       IN PVOID Object OPTIONAL,
		       IN PUNICODE_STRING AbsoluteObjectName OPTIONAL,
		       IN PSECURITY_DESCRIPTOR SecurityDescriptor,
		       IN PACCESS_STATE AccessState,
		       IN BOOLEAN ObjectCreated,
		       IN BOOLEAN AccessGranted,
		       IN KPROCESSOR_MODE AccessMode,
		       OUT PBOOLEAN GenerateOnClose);

VOID STDCALL
SeOpenObjectForDeleteAuditAlarm(IN PUNICODE_STRING ObjectTypeName,
				IN PVOID Object OPTIONAL,
				IN PUNICODE_STRING AbsoluteObjectName OPTIONAL,
				IN PSECURITY_DESCRIPTOR SecurityDescriptor,
				IN PACCESS_STATE AccessState,
				IN BOOLEAN ObjectCreated,
				IN BOOLEAN AccessGranted,
				IN KPROCESSOR_MODE AccessMode,
				OUT PBOOLEAN GenerateOnClose);

BOOLEAN STDCALL
SePrivilegeCheck(IN OUT PPRIVILEGE_SET RequiredPrivileges,
		 IN PSECURITY_SUBJECT_CONTEXT SubjectContext,
		 IN KPROCESSOR_MODE AccessMode);

NTSTATUS STDCALL
SeQueryAuthenticationIdToken(IN PACCESS_TOKEN Token,
			     OUT PLUID LogonId);

NTSTATUS STDCALL
SeQuerySecurityDescriptorInfo(IN PSECURITY_INFORMATION SecurityInformation,
			      OUT PSECURITY_DESCRIPTOR SecurityDescriptor,
			      IN OUT PULONG Length,
			      IN PSECURITY_DESCRIPTOR *ObjectsSecurityDescriptor);

NTSTATUS STDCALL
SeRegisterLogonSessionTerminatedRoutine(IN PSE_LOGON_SESSION_TERMINATED_ROUTINE CallbackRoutine);

VOID STDCALL
SeReleaseSubjectContext(IN PSECURITY_SUBJECT_CONTEXT SubjectContext);

VOID STDCALL
SeSetAccessStateGenericMapping(IN PACCESS_STATE AccessState,
			       IN PGENERIC_MAPPING GenericMapping);

NTSTATUS STDCALL
SeSetSecurityDescriptorInfo(IN PVOID Object OPTIONAL,
			    IN PSECURITY_INFORMATION SecurityInformation,
			    IN PSECURITY_DESCRIPTOR SecurityDescriptor,
			    IN OUT PSECURITY_DESCRIPTOR *ObjectsSecurityDescriptor,
			    IN POOL_TYPE PoolType,
			    IN PGENERIC_MAPPING GenericMapping);

BOOLEAN STDCALL
SeSinglePrivilegeCheck(IN LUID PrivilegeValue,
		       IN KPROCESSOR_MODE PreviousMode);

SECURITY_IMPERSONATION_LEVEL STDCALL
SeTokenImpersonationLevel(IN PACCESS_TOKEN Token);

TOKEN_TYPE STDCALL
SeTokenType(IN PACCESS_TOKEN Token);

VOID STDCALL
SeUnlockSubjectContext(IN PSECURITY_SUBJECT_CONTEXT SubjectContext);

NTSTATUS STDCALL
SeUnregisterLogonSessionTerminatedRoutine(IN PSE_LOGON_SESSION_TERMINATED_ROUTINE CallbackRoutine);

BOOLEAN STDCALL
SeValidSecurityDescriptor(IN ULONG Length,
			  IN PSECURITY_DESCRIPTOR SecurityDescriptor);

#endif /* ndef _INCLUDE_DDK_SEFUNCS_H */
