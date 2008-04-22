#ifndef __NTOSKRNL_INCLUDE_INTERNAL_SE_H
#define __NTOSKRNL_INCLUDE_INTERNAL_SE_H

extern POBJECT_TYPE SepTokenObjectType;

/* SID Authorities */
extern SID_IDENTIFIER_AUTHORITY SeNullSidAuthority;
extern SID_IDENTIFIER_AUTHORITY SeWorldSidAuthority;
extern SID_IDENTIFIER_AUTHORITY SeLocalSidAuthority;
extern SID_IDENTIFIER_AUTHORITY SeCreatorSidAuthority;
extern SID_IDENTIFIER_AUTHORITY SeNtSidAuthority;

/* SIDs */
extern PSID SeNullSid;
extern PSID SeWorldSid;
extern PSID SeLocalSid;
extern PSID SeCreatorOwnerSid;
extern PSID SeCreatorGroupSid;
extern PSID SeCreatorOwnerServerSid;
extern PSID SeCreatorGroupServerSid;
extern PSID SeNtAuthoritySid;
extern PSID SeDialupSid;
extern PSID SeNetworkSid;
extern PSID SeBatchSid;
extern PSID SeInteractiveSid;
extern PSID SeServiceSid;
extern PSID SeAnonymousLogonSid;
extern PSID SePrincipalSelfSid;
extern PSID SeLocalSystemSid;
extern PSID SeAuthenticatedUserSid;
extern PSID SeRestrictedCodeSid;
extern PSID SeAliasAdminsSid;
extern PSID SeAliasUsersSid;
extern PSID SeAliasGuestsSid;
extern PSID SeAliasPowerUsersSid;
extern PSID SeAliasAccountOpsSid;
extern PSID SeAliasSystemOpsSid;
extern PSID SeAliasPrintOpsSid;
extern PSID SeAliasBackupOpsSid;
extern PSID SeAuthenticatedUsersSid;
extern PSID SeRestrictedSid;
extern PSID SeAnonymousLogonSid;

/* Privileges */
extern LUID SeCreateTokenPrivilege;
extern LUID SeAssignPrimaryTokenPrivilege;
extern LUID SeLockMemoryPrivilege;
extern LUID SeIncreaseQuotaPrivilege;
extern LUID SeUnsolicitedInputPrivilege;
extern LUID SeTcbPrivilege;
extern LUID SeSecurityPrivilege;
extern LUID SeTakeOwnershipPrivilege;
extern LUID SeLoadDriverPrivilege;
extern LUID SeCreatePagefilePrivilege;
extern LUID SeIncreaseBasePriorityPrivilege;
extern LUID SeSystemProfilePrivilege;
extern LUID SeSystemtimePrivilege;
extern LUID SeProfileSingleProcessPrivilege;
extern LUID SeCreatePermanentPrivilege;
extern LUID SeBackupPrivilege;
extern LUID SeRestorePrivilege;
extern LUID SeShutdownPrivilege;
extern LUID SeDebugPrivilege;
extern LUID SeAuditPrivilege;
extern LUID SeSystemEnvironmentPrivilege;
extern LUID SeChangeNotifyPrivilege;
extern LUID SeRemoteShutdownPrivilege;
extern LUID SeUndockPrivilege;
extern LUID SeSyncAgentPrivilege;
extern LUID SeEnableDelegationPrivilege;

/* DACLs */
extern PACL SePublicDefaultUnrestrictedDacl;
extern PACL SePublicOpenDacl;
extern PACL SePublicOpenUnrestrictedDacl;
extern PACL SeUnrestrictedDacl;

/* SDs */
extern PSECURITY_DESCRIPTOR SePublicDefaultSd;
extern PSECURITY_DESCRIPTOR SePublicDefaultUnrestrictedSd;
extern PSECURITY_DESCRIPTOR SePublicOpenSd;
extern PSECURITY_DESCRIPTOR SePublicOpenUnrestrictedSd;
extern PSECURITY_DESCRIPTOR SeSystemDefaultSd;
extern PSECURITY_DESCRIPTOR SeUnrestrictedSd;

/* Functions */
BOOLEAN
NTAPI
SeInit(VOID);

BOOLEAN
NTAPI
SeInitSRM(VOID);

VOID
NTAPI
SepInitLuid(VOID);

VOID
NTAPI
SepInitPrivileges(VOID);

BOOLEAN
NTAPI
SepInitSecurityIDs(VOID);

BOOLEAN
NTAPI
SepInitDACLs(VOID);

BOOLEAN
NTAPI
SepInitSDs(VOID);

VOID
NTAPI
SeDeassignPrimaryToken(struct _EPROCESS *Process);

NTSTATUS
NTAPI
SeSubProcessToken(
    IN PTOKEN Parent,
    OUT PTOKEN *Token,
    IN BOOLEAN InUse,
    IN ULONG SessionId
);

NTSTATUS
NTAPI
SeInitializeProcessAuditName(
    IN PFILE_OBJECT FileObject,
    IN BOOLEAN DoAudit,
    OUT POBJECT_NAME_INFORMATION *AuditInfo
);

NTSTATUS
NTAPI
SeCreateAccessStateEx(
    IN PETHREAD Thread,
    IN PEPROCESS Process,
    IN OUT PACCESS_STATE AccessState,
    IN PAUX_DATA AuxData,
    IN ACCESS_MASK Access,
    IN PGENERIC_MAPPING GenericMapping
);

NTSTATUS
NTAPI
SeIsTokenChild(
    IN PTOKEN Token,
    OUT PBOOLEAN IsChild
);

NTSTATUS
NTAPI
SepCreateImpersonationTokenDacl(
    PTOKEN Token,
    PTOKEN PrimaryToken,
    PACL *Dacl
);

VOID
NTAPI
SepInitializeTokenImplementation(VOID);

PTOKEN
NTAPI
SepCreateSystemProcessToken(VOID);

BOOLEAN
NTAPI
SeDetailedAuditingWithToken(IN PTOKEN Token);

VOID
NTAPI
SeAuditProcessExit(IN PEPROCESS Process);

VOID
NTAPI
SeAuditProcessCreate(IN PEPROCESS Process);

NTSTATUS
NTAPI
SeExchangePrimaryToken(
    struct _EPROCESS* Process,
    PACCESS_TOKEN NewToken,
    PACCESS_TOKEN* OldTokenP
);

VOID
NTAPI
SeCaptureSubjectContextEx(
    IN PETHREAD Thread,
    IN PEPROCESS Process,
    OUT PSECURITY_SUBJECT_CONTEXT SubjectContext
);

NTSTATUS
NTAPI
SeCaptureLuidAndAttributesArray(
    PLUID_AND_ATTRIBUTES Src,
    ULONG PrivilegeCount,
    KPROCESSOR_MODE PreviousMode,
    PLUID_AND_ATTRIBUTES AllocatedMem,
    ULONG AllocatedLength,
    POOL_TYPE PoolType,
    BOOLEAN CaptureIfKernel,
    PLUID_AND_ATTRIBUTES* Dest,
    PULONG Length
);

VOID
NTAPI
SeReleaseLuidAndAttributesArray(
    PLUID_AND_ATTRIBUTES Privilege,
    KPROCESSOR_MODE PreviousMode,
    BOOLEAN CaptureIfKernel
);

BOOLEAN
NTAPI
SepPrivilegeCheck(
    PTOKEN Token,
    PLUID_AND_ATTRIBUTES Privileges,
    ULONG PrivilegeCount,
    ULONG PrivilegeControl,
    KPROCESSOR_MODE PreviousMode
);

NTSTATUS
NTAPI
SepDuplicateToken(
    PTOKEN Token,
    POBJECT_ATTRIBUTES ObjectAttributes,
    BOOLEAN EffectiveOnly,
    TOKEN_TYPE TokenType,
    SECURITY_IMPERSONATION_LEVEL Level,
    KPROCESSOR_MODE PreviousMode,
    PTOKEN* NewAccessToken
);

NTSTATUS
NTAPI
SepCaptureSecurityQualityOfService(
    IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
    IN KPROCESSOR_MODE AccessMode,
    IN POOL_TYPE PoolType,
    IN BOOLEAN CaptureIfKernel,
    OUT PSECURITY_QUALITY_OF_SERVICE *CapturedSecurityQualityOfService,
    OUT PBOOLEAN Present
);

VOID
NTAPI
SepReleaseSecurityQualityOfService(
    IN PSECURITY_QUALITY_OF_SERVICE CapturedSecurityQualityOfService OPTIONAL,
    IN KPROCESSOR_MODE AccessMode,
    IN BOOLEAN CaptureIfKernel
);

NTSTATUS
NTAPI
SepCaptureSid(
    IN PSID InputSid,
    IN KPROCESSOR_MODE AccessMode,
    IN POOL_TYPE PoolType,
    IN BOOLEAN CaptureIfKernel,
    OUT PSID *CapturedSid
);

VOID
NTAPI
SepReleaseSid(
    IN PSID CapturedSid,
    IN KPROCESSOR_MODE AccessMode,
    IN BOOLEAN CaptureIfKernel
);

NTSTATUS
NTAPI
SepCaptureAcl(
    IN PACL InputAcl,
    IN KPROCESSOR_MODE AccessMode,
    IN POOL_TYPE PoolType,
    IN BOOLEAN CaptureIfKernel,
    OUT PACL *CapturedAcl
);

VOID
NTAPI
SepReleaseAcl(
    IN PACL CapturedAcl,
    IN KPROCESSOR_MODE AccessMode,
    IN BOOLEAN CaptureIfKernel
);

NTSTATUS
NTAPI
SeDefaultObjectMethod(
    PVOID Object,
    SECURITY_OPERATION_CODE OperationType,
    PSECURITY_INFORMATION SecurityInformation,
    PSECURITY_DESCRIPTOR NewSecurityDescriptor,
    PULONG ReturnLength,
    PSECURITY_DESCRIPTOR *OldSecurityDescriptor,
    POOL_TYPE PoolType,
    PGENERIC_MAPPING GenericMapping
);

NTSTATUS
NTAPI
SeSetWorldSecurityDescriptor(
    SECURITY_INFORMATION SecurityInformation,
    PISECURITY_DESCRIPTOR SecurityDescriptor,
    PULONG BufferLength
);

#define SepAcquireTokenLockExclusive(Token)                                    \
  do {                                                                         \
    KeEnterCriticalRegion();                                                   \
    ExAcquireResourceExclusive(((PTOKEN)Token)->TokenLock, TRUE);              \
  while(0)

#define SepAcquireTokenLockShared(Token)                                       \
  do {                                                                         \
    KeEnterCriticalRegion();                                                   \
    ExAcquireResourceShared(((PTOKEN)Token)->TokenLock, TRUE);                 \
  while(0)

#define SepReleaseTokenLock(Token)                                             \
  do {                                                                         \
    ExReleaseResource(((PTOKEN)Token)->TokenLock);                             \
    KeLeaveCriticalRegion();                                                   \
  while(0)

VOID NTAPI
SeQuerySecurityAccessMask(IN SECURITY_INFORMATION SecurityInformation,
                          OUT PACCESS_MASK DesiredAccess);

VOID NTAPI
SeSetSecurityAccessMask(IN SECURITY_INFORMATION SecurityInformation,
                        OUT PACCESS_MASK DesiredAccess);

#endif /* __NTOSKRNL_INCLUDE_INTERNAL_SE_H */

/* EOF */
