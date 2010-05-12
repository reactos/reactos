/******************************************************************************
 *                            Security Manager Functions                      *
 ******************************************************************************/

#if (NTDDI_VERSION >= NTDDI_WIN2K)
$if (_NTDDK_)
NTKERNELAPI
BOOLEAN
NTAPI
SeSinglePrivilegeCheck(
  IN LUID PrivilegeValue,
  IN KPROCESSOR_MODE PreviousMode);
$endif

$if (_WDMDDK_)
NTKERNELAPI
BOOLEAN
NTAPI
SeAccessCheck(
  IN PSECURITY_DESCRIPTOR SecurityDescriptor,
  IN PSECURITY_SUBJECT_CONTEXT SubjectSecurityContext,
  IN BOOLEAN SubjectContextLocked,
  IN ACCESS_MASK DesiredAccess,
  IN ACCESS_MASK PreviouslyGrantedAccess,
  OUT PPRIVILEGE_SET *Privileges OPTIONAL,
  IN PGENERIC_MAPPING GenericMapping,
  IN KPROCESSOR_MODE AccessMode,
  OUT PACCESS_MASK GrantedAccess,
  OUT PNTSTATUS AccessStatus);

NTKERNELAPI
NTSTATUS
NTAPI
SeAssignSecurity(
  IN PSECURITY_DESCRIPTOR ParentDescriptor OPTIONAL,
  IN PSECURITY_DESCRIPTOR ExplicitDescriptor OPTIONAL,
  OUT PSECURITY_DESCRIPTOR *NewDescriptor,
  IN BOOLEAN IsDirectoryObject,
  IN PSECURITY_SUBJECT_CONTEXT SubjectContext,
  IN PGENERIC_MAPPING GenericMapping,
  IN POOL_TYPE PoolType);

NTKERNELAPI
NTSTATUS
NTAPI
SeAssignSecurityEx(
  IN PSECURITY_DESCRIPTOR ParentDescriptor OPTIONAL,
  IN PSECURITY_DESCRIPTOR ExplicitDescriptor OPTIONAL,
  OUT PSECURITY_DESCRIPTOR *NewDescriptor,
  IN GUID *ObjectType OPTIONAL,
  IN BOOLEAN IsDirectoryObject,
  IN ULONG AutoInheritFlags,
  IN PSECURITY_SUBJECT_CONTEXT SubjectContext,
  IN PGENERIC_MAPPING GenericMapping,
  IN POOL_TYPE PoolType);

NTKERNELAPI
NTSTATUS
NTAPI
SeDeassignSecurity(
  IN OUT PSECURITY_DESCRIPTOR *SecurityDescriptor);

NTKERNELAPI
BOOLEAN
NTAPI
SeValidSecurityDescriptor(
  IN ULONG Length,
  IN PSECURITY_DESCRIPTOR SecurityDescriptor);

NTKERNELAPI
ULONG
NTAPI
SeObjectCreateSaclAccessBits(
  IN PSECURITY_DESCRIPTOR SecurityDescriptor);

NTKERNELAPI
VOID
NTAPI
SeReleaseSubjectContext(
  IN OUT PSECURITY_SUBJECT_CONTEXT SubjectContext);

NTKERNELAPI
VOID
NTAPI
SeUnlockSubjectContext(
  IN PSECURITY_SUBJECT_CONTEXT SubjectContext);

NTKERNELAPI
VOID
NTAPI
SeCaptureSubjectContext(
  OUT PSECURITY_SUBJECT_CONTEXT SubjectContext);

NTKERNELAPI
VOID
NTAPI
SeLockSubjectContext(
  IN PSECURITY_SUBJECT_CONTEXT SubjectContext);
$endif

#endif /* (NTDDI_VERSION >= NTDDI_WIN2K) */

$if (_WDMDDK_)
#if (NTDDI_VERSION >= NTDDI_WS03SP1)

NTSTATUS
NTAPI
SeSetAuditParameter(
  IN OUT PSE_ADT_PARAMETER_ARRAY AuditParameters,
  IN SE_ADT_PARAMETER_TYPE Type,
  IN ULONG Index,
  IN PVOID Data);

NTSTATUS
NTAPI
SeReportSecurityEvent(
  IN ULONG Flags,
  IN PUNICODE_STRING SourceName,
  IN PSID UserSid OPTIONAL,
  IN PSE_ADT_PARAMETER_ARRAY AuditParameters);

#endif /* (NTDDI_VERSION >= NTDDI_WS03SP1) */

#if (NTDDI_VERSION >= NTDDI_VISTA)

NTKERNELAPI
ULONG
NTAPI
SeComputeAutoInheritByObjectType(
  IN PVOID ObjectType,
  IN PSECURITY_DESCRIPTOR SecurityDescriptor OPTIONAL,
  IN PSECURITY_DESCRIPTOR ParentSecurityDescriptor OPTIONAL);

#ifdef SE_NTFS_WORLD_CACHE
VOID
NTAPI
SeGetWorldRights(
  IN PSECURITY_DESCRIPTOR SecurityDescriptor,
  IN PGENERIC_MAPPING GenericMapping,
  OUT PACCESS_MASK GrantedAccess);
#endif /* SE_NTFS_WORLD_CACHE */

#endif /* (NTDDI_VERSION >= NTDDI_VISTA) */
$endif
