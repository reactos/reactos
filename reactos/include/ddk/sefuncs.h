#ifndef _INCLUDE_DDK_SEFUNCS_H
#define _INCLUDE_DDK_SEFUNCS_H
/* $Id: sefuncs.h,v 1.15 2002/02/20 20:09:52 ekohl Exp $ */

BOOLEAN STDCALL
SeAccessCheck(IN PSECURITY_DESCRIPTOR SecurityDescriptor,
	      IN PSECURITY_SUBJECT_CONTEXT SubjectSecurityContext,
	      IN BOOLEAN SubjectContextLocked,
	      IN ACCESS_MASK DesiredAccess,
	      IN ACCESS_MASK PreviouslyGrantedAccess,
	      OUT PPRIVILEGE_SET* Privileges,
	      IN PGENERIC_MAPPING GenericMapping,
	      IN KPROCESSOR_MODE AccessMode,
	      OUT PACCESS_MODE GrantedAccess,
	      OUT PNTSTATUS AccessStatus);

NTSTATUS STDCALL
SeAssignSecurity(PSECURITY_DESCRIPTOR ParentDescriptor,
		 PSECURITY_DESCRIPTOR ExplicitDescriptor,
		 PSECURITY_DESCRIPTOR* NewDescriptor,
		 BOOLEAN IsDirectoryObject,
		 PSECURITY_SUBJECT_CONTEXT SubjectContext,
		 PGENERIC_MAPPING GenericMapping,
		 POOL_TYPE PoolType);

VOID STDCALL
SeCaptureSubjectContext(OUT PSECURITY_SUBJECT_CONTEXT SubjectContext);

NTSTATUS STDCALL
SeCreateClientSecurity(IN struct _ETHREAD *Thread,
		       IN PSECURITY_QUALITY_OF_SERVICE Qos,
		       IN BOOLEAN RemoteClient,
		       OUT PSECURITY_CLIENT_CONTEXT ClientContext);

NTSTATUS STDCALL
SeDeassignSecurity(PSECURITY_DESCRIPTOR* SecurityDescriptor);

VOID STDCALL
SeImpersonateClient(IN PSECURITY_CLIENT_CONTEXT ClientContext,
		    IN struct _ETHREAD *ServerThread OPTIONAL);

VOID STDCALL
SeReleaseSubjectContext(IN PSECURITY_SUBJECT_CONTEXT SubjectContext);

BOOLEAN STDCALL
SeSinglePrivilegeCheck(LUID PrivilegeValue,
		       KPROCESSOR_MODE PreviousMode);

TOKEN_TYPE STDCALL
SeTokenType(IN PACCESS_TOKEN Token);

#endif /* ndef _INCLUDE_DDK_SEFUNCS_H */
