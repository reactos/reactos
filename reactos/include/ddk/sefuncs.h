NTSTATUS RtlCreateSecurityDescriptor(PSECURITY_DESCRIPTOR SecurityDescriptor,
				     ULONG Revision);

BOOLEAN RtlValidSecurityDescriptor(PSECURITY_DESCRIPTOR SecurityDescriptor);

ULONG RtlLengthSecurityDescriptor(PSECURITY_DESCRIPTOR SecurityDescriptor);

NTSTATUS RtlSetDaclSecurityDescriptor(PSECURITY_DESCRIPTOR SecurityDescriptor,
				      BOOLEAN DaclPresent,
				      PACL Dacl,
				      BOOLEAN DaclDefaulted);

NTSTATUS RtlGetDaclSecurityDescriptor(PSECURITY_DESCRIPTOR SecurityDescriptor,
				      PBOOLEAN DaclPresent,
				      PACL* Dacl,
				      PBOOLEAN DaclDefauted);

NTSTATUS RtlSetOwnerSecurityDescriptor(PSECURITY_DESCRIPTOR SecurityDescriptor,
				       PSID Owner,
				       BOOLEAN OwnerDefaulted);

NTSTATUS RtlGetOwnerSecurityDescriptor(PSECURITY_DESCRIPTOR SecurityDescriptor,
				       PSID* Owner,
				       PBOOLEAN OwnerDefaulted);

NTSTATUS RtlSetGroupSecurityDescriptor(PSECURITY_DESCRIPTOR SecurityDescriptor,
				       PSID Group,
				       BOOLEAN GroupDefaulted);

NTSTATUS RtlGetGroupSecurityDescriptor(PSECURITY_DESCRIPTOR SecurityDescriptor,
				       PSID* Group,
				       PBOOLEAN GroupDefaulted);

ULONG RtlLengthRequiredSid(UCHAR SubAuthorityCount);

NTSTATUS RtlInitializeSid(PSID Sid,
			  PSID_IDENTIFIER_AUTHORITY IdentifierAuthority,
			  UCHAR SubAuthorityCount);

PULONG RtlSubAuthoritySid(PSID Sid, ULONG SubAuthority);

BOOLEAN RtlEqualSid(PSID Sid1, PSID Sid2);

NTSTATUS RtlAbsoluteToSelfRelativeSD(PSECURITY_DESCRIPTOR AbsSD,
				     PSECURITY_DESCRIPTOR RelSD,
				     PULONG BufferLength);

BOOLEAN SeAccessCheck(IN PSECURITY_DESCRIPTOR SecurityDescriptor,
		      IN PSECURITY_DESCRIPTOR_CONTEXT SubjectSecurityContext,
		      IN BOOLEAN SubjectContextLocked,
		      IN ACCESS_MASK DesiredAccess,
		      IN ACCESS_MASK PreviouslyGrantedAccess,
		      OUT PPRIVILEGE_SET* Privileges,
		      IN PGENERIC_MAPPING GenericMapping,
		      IN KPROCESSOR_MODE AccessMode,
		      OUT PACCESS_MODE GrantedAccess,
		      OUT PNTSTATUS AccessStatus);

NTSTATUS SeAssignSecurity(PSECURITY_DESCRIPTOR ParentDescriptor,
			  PSECURITY_DESCRIPTOR ExplicitDescriptor,
			  BOOLEAN IsDirectoryObject,
			  PSECURITY_SUBJECT_CONTEXT SubjectContext,
			  PGENERIC_MAPPING GenericMapping,
			  POOL_TYPE PoolType);

NTSTATUS SeDeassignSecurity(PSECURITY_DESCRIPTOR* SecurityDescriptor);

BOOLEAN SeSinglePrivilegeCheck(LUID PrivilegeValue,
			       KPROCESSOR_MODE PreviousMode);


ULONG RtlLengthSid(PSID Sid);
NTSTATUS RtlCopySid(ULONG BufferLength, PSID Src, PSID Dest);
