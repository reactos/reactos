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



