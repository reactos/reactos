#ifndef _INCLUDE_DDK_SEFUNCS_H
#define _INCLUDE_DDK_SEFUNCS_H
/* $Id: sefuncs.h,v 1.14 2001/07/06 21:32:43 ekohl Exp $ */

BOOLEAN STDCALL SeAccessCheck (IN PSECURITY_DESCRIPTOR SecurityDescriptor,
		      IN PSECURITY_SUBJECT_CONTEXT SubjectSecurityContext,
		      IN BOOLEAN SubjectContextLocked,
		      IN ACCESS_MASK DesiredAccess,
		      IN ACCESS_MASK PreviouslyGrantedAccess,
		      OUT PPRIVILEGE_SET* Privileges,
		      IN PGENERIC_MAPPING GenericMapping,
		      IN KPROCESSOR_MODE AccessMode,
		      OUT PACCESS_MODE GrantedAccess,
		      OUT PNTSTATUS AccessStatus);
NTSTATUS STDCALL SeAssignSecurity (PSECURITY_DESCRIPTOR ParentDescriptor,
				   PSECURITY_DESCRIPTOR ExplicitDescriptor,
				   PSECURITY_DESCRIPTOR* NewDescriptor,
				   BOOLEAN IsDirectoryObject,
				   PSECURITY_SUBJECT_CONTEXT SubjectContext,
				   PGENERIC_MAPPING GenericMapping,
				   POOL_TYPE PoolType);
NTSTATUS STDCALL SeDeassignSecurity (PSECURITY_DESCRIPTOR* SecurityDescriptor);
BOOLEAN STDCALL SeSinglePrivilegeCheck (LUID PrivilegeValue, KPROCESSOR_MODE PreviousMode);
VOID STDCALL SeImpersonateClient(PSE_SOME_STRUCT2 a,
				 struct _ETHREAD* Thread);

NTSTATUS STDCALL SeCreateClientSecurity(struct _ETHREAD* Thread,
				PSECURITY_QUALITY_OF_SERVICE Qos,
				ULONG e,
				PSE_SOME_STRUCT2 f);
NTSTATUS SeExchangePrimaryToken(struct _EPROCESS* Process,
				PACCESS_TOKEN NewToken,
				PACCESS_TOKEN* OldTokenP);
VOID STDCALL SeReleaseSubjectContext (PSECURITY_SUBJECT_CONTEXT SubjectContext);
VOID STDCALL SeCaptureSubjectContext (PSECURITY_SUBJECT_CONTEXT SubjectContext);
NTSTATUS SeCaptureLuidAndAttributesArray(PLUID_AND_ATTRIBUTES Src,
					 ULONG PrivilegeCount,
					 KPROCESSOR_MODE PreviousMode,
					 PLUID_AND_ATTRIBUTES AllocatedMem,
					 ULONG AllocatedLength,
					 POOL_TYPE PoolType,
					 ULONG d,
					 PLUID_AND_ATTRIBUTES* Dest,
					 PULONG Length);


#endif /* ndef _INCLUDE_DDK_SEFUNCS_H */
