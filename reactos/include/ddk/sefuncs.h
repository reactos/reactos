#ifndef _INCLUDE_DDK_SEFUNCS_H
#define _INCLUDE_DDK_SEFUNCS_H
/* $Id$ */

#ifdef __NTOSKRNL__
extern PACL EXPORTED SePublicDefaultDacl;
extern PACL EXPORTED SeSystemDefaultDacl;
#else
extern PACL IMPORTED SePublicDefaultDacl;
extern PACL IMPORTED SeSystemDefaultDacl;
#endif

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

NTSTATUS
STDCALL
SeAssignSecurityEx(
	IN PSECURITY_DESCRIPTOR ParentDescriptor OPTIONAL,
	IN PSECURITY_DESCRIPTOR ExplicitDescriptor OPTIONAL,
	OUT PSECURITY_DESCRIPTOR *NewDescriptor,
	IN GUID *ObjectType OPTIONAL,
	IN BOOLEAN IsDirectoryObject,
	IN ULONG AutoInheritFlags,
	IN PSECURITY_SUBJECT_CONTEXT SubjectContext,
	IN PGENERIC_MAPPING GenericMapping,
	IN POOL_TYPE PoolType
	);


VOID
STDCALL
SeAuditHardLinkCreation(
	IN PUNICODE_STRING FileName,
	IN PUNICODE_STRING LinkName,
	IN BOOLEAN bSuccess
	);

BOOLEAN
STDCALL
SeAuditingFileEvents(
	IN BOOLEAN AccessGranted,
	IN PSECURITY_DESCRIPTOR SecurityDescriptor
	);

BOOLEAN
STDCALL
SeAuditingFileEventsWithContext(
	IN BOOLEAN AccessGranted,
	IN PSECURITY_DESCRIPTOR SecurityDescriptor,
	IN PSECURITY_SUBJECT_CONTEXT SubjectSecurityContext OPTIONAL
	);

BOOLEAN
STDCALL
SeAuditingHardLinkEvents(
	IN BOOLEAN AccessGranted,
	IN PSECURITY_DESCRIPTOR SecurityDescriptor
	);

BOOLEAN
STDCALL
SeAuditingHardLinkEventsWithContext(
	IN BOOLEAN AccessGranted,
	IN PSECURITY_DESCRIPTOR SecurityDescriptor,
	IN PSECURITY_SUBJECT_CONTEXT SubjectSecurityContext OPTIONAL
	);

BOOLEAN
STDCALL
SeAuditingFileOrGlobalEvents(
	IN BOOLEAN AccessGranted,
	IN PSECURITY_DESCRIPTOR SecurityDescriptor,
	IN PSECURITY_SUBJECT_CONTEXT SubjectSecurityContext
	);

VOID STDCALL
SeCaptureSubjectContext(OUT PSECURITY_SUBJECT_CONTEXT SubjectContext);

NTSTATUS STDCALL
SeCreateClientSecurity(IN struct _ETHREAD *Thread,
		       IN PSECURITY_QUALITY_OF_SERVICE Qos,
		       IN BOOLEAN RemoteClient,
		       OUT PSECURITY_CLIENT_CONTEXT ClientContext);

NTSTATUS
STDCALL
SeCreateClientSecurityFromSubjectContext(
	IN PSECURITY_SUBJECT_CONTEXT SubjectContext,
	IN PSECURITY_QUALITY_OF_SERVICE ClientSecurityQos,
	IN BOOLEAN ServerIsRemote,
	OUT PSECURITY_CLIENT_CONTEXT ClientContext
	);

NTSTATUS STDCALL
SeDeassignSecurity(IN OUT PSECURITY_DESCRIPTOR* SecurityDescriptor);

VOID STDCALL
SeDeleteObjectAuditAlarm(IN PVOID Object,
			 IN HANDLE Handle);

NTSTATUS
STDCALL
SeFilterToken(
	IN PACCESS_TOKEN ExistingToken,
	IN ULONG Flags,
	IN PTOKEN_GROUPS SidsToDisable OPTIONAL,
	IN PTOKEN_PRIVILEGES PrivilegesToDelete OPTIONAL,
	IN PTOKEN_GROUPS RestrictedSids OPTIONAL,
	OUT PACCESS_TOKEN * FilteredToken
	);

VOID STDCALL
SeFreePrivileges(IN PPRIVILEGE_SET Privileges);

VOID STDCALL
SeImpersonateClient(IN PSECURITY_CLIENT_CONTEXT ClientContext,
		    IN struct _ETHREAD *ServerThread OPTIONAL);

NTSTATUS
STDCALL
SeImpersonateClientEx(
	IN PSECURITY_CLIENT_CONTEXT ClientContext,
	IN struct _ETHREAD *ServerThread OPTIONAL
	);

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

NTSTATUS
STDCALL
SeQueryInformationToken(
	IN PACCESS_TOKEN Token,
	IN TOKEN_INFORMATION_CLASS TokenInformationClass,
	OUT PVOID *TokenInformation
	);

NTSTATUS STDCALL
SeQuerySecurityDescriptorInfo(IN PSECURITY_INFORMATION SecurityInformation,
			      OUT PSECURITY_DESCRIPTOR SecurityDescriptor,
			      IN OUT PULONG Length,
			      IN PSECURITY_DESCRIPTOR *ObjectsSecurityDescriptor);

NTSTATUS
STDCALL
SeQuerySessionIdToken(
	IN PACCESS_TOKEN,
	IN PULONG pSessionId
	);


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

NTSTATUS
STDCALL
SeSetSecurityDescriptorInfoEx(
	IN PVOID Object OPTIONAL,
	IN PSECURITY_INFORMATION SecurityInformation,
	IN PSECURITY_DESCRIPTOR ModificationDescriptor,
	IN OUT PSECURITY_DESCRIPTOR *ObjectsSecurityDescriptor,
	IN ULONG AutoInheritFlags,
	IN POOL_TYPE PoolType,
	IN PGENERIC_MAPPING GenericMapping
	);

BOOLEAN STDCALL
SeSinglePrivilegeCheck(IN LUID PrivilegeValue,
		       IN KPROCESSOR_MODE PreviousMode);

BOOLEAN
STDCALL
SeTokenIsAdmin(
	IN PACCESS_TOKEN Token
	);

BOOLEAN
STDCALL
SeTokenIsRestricted(
	IN PACCESS_TOKEN Token
	);

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
