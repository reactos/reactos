#ifndef _INCLUDE_DDK_SEFUNCS_H
#define _INCLUDE_DDK_SEFUNCS_H
/* $Id: sefuncs.h,v 1.6 2000/03/12 01:18:18 ekohl Exp $ */
NTSTATUS STDCALL RtlCreateSecurityDescriptor (PSECURITY_DESCRIPTOR SecurityDescriptor, ULONG Revision);
BOOLEAN STDCALL RtlValidSecurityDescriptor (PSECURITY_DESCRIPTOR SecurityDescriptor);
ULONG STDCALL RtlLengthSecurityDescriptor (PSECURITY_DESCRIPTOR SecurityDescriptor);
NTSTATUS STDCALL RtlSetDaclSecurityDescriptor (PSECURITY_DESCRIPTOR SecurityDescriptor, BOOLEAN DaclPresent, PACL Dacl, BOOLEAN DaclDefaulted);
NTSTATUS STDCALL RtlGetDaclSecurityDescriptor (PSECURITY_DESCRIPTOR SecurityDescriptor, PBOOLEAN DaclPresent, PACL* Dacl, PBOOLEAN DaclDefauted);
NTSTATUS STDCALL RtlSetOwnerSecurityDescriptor (PSECURITY_DESCRIPTOR SecurityDescriptor, PSID Owner, BOOLEAN OwnerDefaulted);
NTSTATUS STDCALL RtlGetOwnerSecurityDescriptor (PSECURITY_DESCRIPTOR SecurityDescriptor, PSID* Owner, PBOOLEAN OwnerDefaulted);
NTSTATUS STDCALL RtlSetGroupSecurityDescriptor (PSECURITY_DESCRIPTOR SecurityDescriptor, PSID Group, BOOLEAN GroupDefaulted);
NTSTATUS STDCALL RtlGetGroupSecurityDescriptor (PSECURITY_DESCRIPTOR SecurityDescriptor, PSID* Group, PBOOLEAN GroupDefaulted);
ULONG STDCALL RtlLengthRequiredSid (UCHAR SubAuthorityCount);
NTSTATUS STDCALL RtlInitializeSid (PSID Sid, PSID_IDENTIFIER_AUTHORITY IdentifierAuthority, UCHAR SubAuthorityCount);
PULONG STDCALL RtlSubAuthoritySid (PSID Sid, ULONG SubAuthority);
NTSTATUS STDCALL RtlCopySid (ULONG BufferLength, PSID Dest, PSID Src);
BOOLEAN STDCALL RtlEqualSid(PSID Sid1, PSID Sid2);
ULONG STDCALL RtlLengthSid (PSID Sid);
BOOLEAN STDCALL RtlValidSid (PSID Sid);
NTSTATUS STDCALL RtlAbsoluteToSelfRelativeSD (PSECURITY_DESCRIPTOR AbsSD, PSECURITY_DESCRIPTOR RelSD, PULONG BufferLength);
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
ULONG STDCALL RtlLengthSid (PSID Sid);
NTSTATUS STDCALL RtlCopySid(ULONG BufferLength, PSID Src, PSID Dest);

VOID SeImpersonateClient(PSE_SOME_STRUCT2 a,
			 PETHREAD Thread);

NTSTATUS SeCreateClientSecurity(PETHREAD Thread,
				PSECURITY_QUALITY_OF_SERVICE Qos,
				ULONG e,
				PSE_SOME_STRUCT2 f);
NTSTATUS SeExchangePrimaryToken(PEPROCESS Process,
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
