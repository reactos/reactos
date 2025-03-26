/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Internal header for the Security Manager
 * COPYRIGHT:   Copyright Eric Kohl
 *              Copyright 2022-2023 George Bișoc <george.bisoc@reactos.org>
 */

#pragma once

//
// Internal ACE type structures
//
typedef struct _KNOWN_ACE
{
    ACE_HEADER Header;
    ACCESS_MASK Mask;
    ULONG SidStart;
} KNOWN_ACE, *PKNOWN_ACE;

typedef struct _KNOWN_OBJECT_ACE
{
    ACE_HEADER Header;
    ACCESS_MASK Mask;
    ULONG Flags;
    ULONG SidStart;
} KNOWN_OBJECT_ACE, *PKNOWN_OBJECT_ACE;

typedef struct _KNOWN_COMPOUND_ACE
{
    ACE_HEADER Header;
    ACCESS_MASK Mask;
    USHORT CompoundAceType;
    USHORT Reserved;
    ULONG SidStart;
} KNOWN_COMPOUND_ACE, *PKNOWN_COMPOUND_ACE;

//
// Access Check Rights
//
typedef struct _ACCESS_CHECK_RIGHTS
{
    ACCESS_MASK RemainingAccessRights;
    ACCESS_MASK GrantedAccessRights;
    ACCESS_MASK DeniedAccessRights;
} ACCESS_CHECK_RIGHTS, *PACCESS_CHECK_RIGHTS;

//
// Internal object type list structure
//
typedef struct _OBJECT_TYPE_LIST_INTERNAL
{
    GUID ObjectTypeGuid;
    USHORT Level;
    ACCESS_CHECK_RIGHTS ObjectAccessRights;
} OBJECT_TYPE_LIST_INTERNAL, *POBJECT_TYPE_LIST_INTERNAL;

typedef enum _ACCESS_CHECK_RIGHT_TYPE
{
    AccessCheckMaximum,
    AccessCheckRegular
} ACCESS_CHECK_RIGHT_TYPE;

//
// Token Audit Policy Information structure
//
typedef struct _TOKEN_AUDIT_POLICY_INFORMATION
{
    ULONG PolicyCount;
    struct
    {
        ULONG Category;
        UCHAR Value;
    } Policies[1];
} TOKEN_AUDIT_POLICY_INFORMATION, *PTOKEN_AUDIT_POLICY_INFORMATION;

//
// Token creation method defines (for debugging purposes)
//
#define TOKEN_CREATE_METHOD    0xCUL
#define TOKEN_DUPLICATE_METHOD 0xDUL
#define TOKEN_FILTER_METHOD    0xFUL

//
// Security descriptor internal helpers
//
FORCEINLINE
PSID
SepGetGroupFromDescriptor(
    _Inout_ PSECURITY_DESCRIPTOR _Descriptor)
{
    PISECURITY_DESCRIPTOR Descriptor = (PISECURITY_DESCRIPTOR)_Descriptor;
    PISECURITY_DESCRIPTOR_RELATIVE SdRel;

    if (Descriptor->Control & SE_SELF_RELATIVE)
    {
        SdRel = (PISECURITY_DESCRIPTOR_RELATIVE)Descriptor;
        if (!SdRel->Group) return NULL;
        return (PSID)((ULONG_PTR)Descriptor + SdRel->Group);
    }
    else
    {
        return Descriptor->Group;
    }
}

FORCEINLINE
PSID
SepGetOwnerFromDescriptor(
    _Inout_ PSECURITY_DESCRIPTOR _Descriptor)
{
    PISECURITY_DESCRIPTOR Descriptor = (PISECURITY_DESCRIPTOR)_Descriptor;
    PISECURITY_DESCRIPTOR_RELATIVE SdRel;

    if (Descriptor->Control & SE_SELF_RELATIVE)
    {
        SdRel = (PISECURITY_DESCRIPTOR_RELATIVE)Descriptor;
        if (!SdRel->Owner) return NULL;
        return (PSID)((ULONG_PTR)Descriptor + SdRel->Owner);
    }
    else
    {
        return Descriptor->Owner;
    }
}

FORCEINLINE
PACL
SepGetDaclFromDescriptor(
    _Inout_ PSECURITY_DESCRIPTOR _Descriptor)
{
    PISECURITY_DESCRIPTOR Descriptor = (PISECURITY_DESCRIPTOR)_Descriptor;
    PISECURITY_DESCRIPTOR_RELATIVE SdRel;

    if (!(Descriptor->Control & SE_DACL_PRESENT)) return NULL;

    if (Descriptor->Control & SE_SELF_RELATIVE)
    {
        SdRel = (PISECURITY_DESCRIPTOR_RELATIVE)Descriptor;
        if (!SdRel->Dacl) return NULL;
        return (PACL)((ULONG_PTR)Descriptor + SdRel->Dacl);
    }
    else
    {
        return Descriptor->Dacl;
    }
}

FORCEINLINE
PACL
SepGetSaclFromDescriptor(
    _Inout_ PSECURITY_DESCRIPTOR _Descriptor)
{
    PISECURITY_DESCRIPTOR Descriptor = (PISECURITY_DESCRIPTOR)_Descriptor;
    PISECURITY_DESCRIPTOR_RELATIVE SdRel;

    if (!(Descriptor->Control & SE_SACL_PRESENT)) return NULL;

    if (Descriptor->Control & SE_SELF_RELATIVE)
    {
        SdRel = (PISECURITY_DESCRIPTOR_RELATIVE)Descriptor;
        if (!SdRel->Sacl) return NULL;
        return (PACL)((ULONG_PTR)Descriptor + SdRel->Sacl);
    }
    else
    {
        return Descriptor->Sacl;
    }
}

#ifndef RTL_H

//
// SID Authorities
//
extern SID_IDENTIFIER_AUTHORITY SeNullSidAuthority;
extern SID_IDENTIFIER_AUTHORITY SeWorldSidAuthority;
extern SID_IDENTIFIER_AUTHORITY SeLocalSidAuthority;
extern SID_IDENTIFIER_AUTHORITY SeCreatorSidAuthority;
extern SID_IDENTIFIER_AUTHORITY SeNtSidAuthority;

//
// SIDs
//
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
extern PSID SeLocalServiceSid;
extern PSID SeNetworkServiceSid;

//
// Privileges
//
extern const LUID SeCreateTokenPrivilege;
extern const LUID SeAssignPrimaryTokenPrivilege;
extern const LUID SeLockMemoryPrivilege;
extern const LUID SeIncreaseQuotaPrivilege;
extern const LUID SeUnsolicitedInputPrivilege;
extern const LUID SeTcbPrivilege;
extern const LUID SeSecurityPrivilege;
extern const LUID SeTakeOwnershipPrivilege;
extern const LUID SeLoadDriverPrivilege;
extern const LUID SeSystemProfilePrivilege;
extern const LUID SeSystemtimePrivilege;
extern const LUID SeProfileSingleProcessPrivilege;
extern const LUID SeIncreaseBasePriorityPrivilege;
extern const LUID SeCreatePagefilePrivilege;
extern const LUID SeCreatePermanentPrivilege;
extern const LUID SeBackupPrivilege;
extern const LUID SeRestorePrivilege;
extern const LUID SeShutdownPrivilege;
extern const LUID SeDebugPrivilege;
extern const LUID SeAuditPrivilege;
extern const LUID SeSystemEnvironmentPrivilege;
extern const LUID SeChangeNotifyPrivilege;
extern const LUID SeRemoteShutdownPrivilege;
extern const LUID SeUndockPrivilege;
extern const LUID SeSyncAgentPrivilege;
extern const LUID SeEnableDelegationPrivilege;
extern const LUID SeManageVolumePrivilege;
extern const LUID SeImpersonatePrivilege;
extern const LUID SeCreateGlobalPrivilege;
extern const LUID SeTrustedCredmanPrivilege;
extern const LUID SeRelabelPrivilege;
extern const LUID SeIncreaseWorkingSetPrivilege;
extern const LUID SeTimeZonePrivilege;
extern const LUID SeCreateSymbolicLinkPrivilege;

//
// DACLs
//
extern PACL SePublicDefaultUnrestrictedDacl;
extern PACL SePublicOpenDacl;
extern PACL SePublicOpenUnrestrictedDacl;
extern PACL SeUnrestrictedDacl;
extern PACL SeSystemAnonymousLogonDacl;

//
// SDs
//
extern PSECURITY_DESCRIPTOR SePublicDefaultSd;
extern PSECURITY_DESCRIPTOR SePublicDefaultUnrestrictedSd;
extern PSECURITY_DESCRIPTOR SePublicOpenSd;
extern PSECURITY_DESCRIPTOR SePublicOpenUnrestrictedSd;
extern PSECURITY_DESCRIPTOR SeSystemDefaultSd;
extern PSECURITY_DESCRIPTOR SeUnrestrictedSd;
extern PSECURITY_DESCRIPTOR SeSystemAnonymousLogonSd;

//
// Anonymous Logon Tokens
//
extern PTOKEN SeAnonymousLogonToken;
extern PTOKEN SeAnonymousLogonTokenNoEveryone;


//
// Token lock management macros
//
#define SepAcquireTokenLockExclusive(Token)                                    \
{                                                                              \
    KeEnterCriticalRegion();                                                   \
    ExAcquireResourceExclusiveLite(((PTOKEN)Token)->TokenLock, TRUE);          \
}
#define SepAcquireTokenLockShared(Token)                                       \
{                                                                              \
    KeEnterCriticalRegion();                                                   \
    ExAcquireResourceSharedLite(((PTOKEN)Token)->TokenLock, TRUE);             \
}

#define SepReleaseTokenLock(Token)                                             \
{                                                                              \
    ExReleaseResourceLite(((PTOKEN)Token)->TokenLock);                         \
    KeLeaveCriticalRegion();                                                   \
}

#if DBG
//
// Security Debug Utility Functions
//
VOID
SepDumpSdDebugInfo(
    _In_opt_ PISECURITY_DESCRIPTOR SecurityDescriptor);

VOID
SepDumpTokenDebugInfo(
   _In_opt_ PTOKEN Token);

VOID
SepDumpAccessRightsStats(
    _In_ PACCESS_CHECK_RIGHTS AccessRights);

VOID
SepDumpAccessAndStatusList(
    _In_ PACCESS_MASK GrantedAccessList,
    _In_ PNTSTATUS AccessStatusList,
    _In_ BOOLEAN IsResultList,
    _In_ POBJECT_TYPE_LIST_INTERNAL ObjectTypeList,
    _In_ ULONG ObjectTypeListLength);
#endif // DBG

//
// Token Functions
//
CODE_SEG("INIT")
VOID
NTAPI
SepInitializeTokenImplementation(VOID);

CODE_SEG("INIT")
PTOKEN
NTAPI
SepCreateSystemProcessToken(VOID);

CODE_SEG("INIT")
PTOKEN
SepCreateSystemAnonymousLogonToken(VOID);

CODE_SEG("INIT")
PTOKEN
SepCreateSystemAnonymousLogonTokenNoEveryone(VOID);

NTSTATUS
NTAPI
SepDuplicateToken(
    _In_ PTOKEN Token,
    _In_opt_ POBJECT_ATTRIBUTES ObjectAttributes,
    _In_ BOOLEAN EffectiveOnly,
    _In_ TOKEN_TYPE TokenType,
    _In_ SECURITY_IMPERSONATION_LEVEL Level,
    _In_ KPROCESSOR_MODE PreviousMode,
    _Out_ PTOKEN* NewAccessToken);

NTSTATUS
NTAPI
SepCreateToken(
    _Out_ PHANDLE TokenHandle,
    _In_ KPROCESSOR_MODE PreviousMode,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ POBJECT_ATTRIBUTES ObjectAttributes,
    _In_ TOKEN_TYPE TokenType,
    _In_ SECURITY_IMPERSONATION_LEVEL ImpersonationLevel,
    _In_ PLUID AuthenticationId,
    _In_ PLARGE_INTEGER ExpirationTime,
    _In_ PSID_AND_ATTRIBUTES User,
    _In_ ULONG GroupCount,
    _In_ PSID_AND_ATTRIBUTES Groups,
    _In_ ULONG GroupsLength,
    _In_ ULONG PrivilegeCount,
    _In_ PLUID_AND_ATTRIBUTES Privileges,
    _In_opt_ PSID Owner,
    _In_ PSID PrimaryGroup,
    _In_opt_ PACL DefaultDacl,
    _In_ PTOKEN_SOURCE TokenSource,
    _In_ BOOLEAN SystemToken);

BOOLEAN
NTAPI
SepTokenIsOwner(
    _In_ PACCESS_TOKEN _Token,
    _In_ PSECURITY_DESCRIPTOR SecurityDescriptor,
    _In_ BOOLEAN TokenLocked);

NTSTATUS
SepCreateTokenLock(
    _Inout_ PTOKEN Token);

VOID
SepDeleteTokenLock(
    _Inout_ PTOKEN Token);

VOID
SepUpdatePrivilegeFlagsToken(
    _Inout_ PTOKEN Token);

NTSTATUS
SepFindPrimaryGroupAndDefaultOwner(
    _In_ PTOKEN Token,
    _In_ PSID PrimaryGroup,
    _In_opt_ PSID DefaultOwner,
    _Out_opt_ PULONG PrimaryGroupIndex,
    _Out_opt_ PULONG DefaultOwnerIndex);

VOID
SepUpdateSinglePrivilegeFlagToken(
    _Inout_ PTOKEN Token,
    _In_ ULONG Index);

VOID
SepUpdatePrivilegeFlagsToken(
    _Inout_ PTOKEN Token);

VOID
SepRemovePrivilegeToken(
    _Inout_ PTOKEN Token,
    _In_ ULONG Index);

VOID
SepRemoveUserGroupToken(
    _Inout_ PTOKEN Token,
    _In_ ULONG Index);

ULONG
SepComputeAvailableDynamicSpace(
    _In_ ULONG DynamicCharged,
    _In_ PSID PrimaryGroup,
    _In_opt_ PACL DefaultDacl);

NTSTATUS
SepRebuildDynamicPartOfToken(
    _In_ PTOKEN Token,
    _In_ ULONG NewDynamicPartSize);

BOOLEAN
NTAPI
SeTokenCanImpersonate(
    _In_ PTOKEN ProcessToken,
    _In_ PTOKEN TokenToImpersonate,
    _In_ SECURITY_IMPERSONATION_LEVEL ImpersonationLevel);

VOID
NTAPI
SeGetTokenControlInformation(
    _In_ PACCESS_TOKEN _Token,
    _Out_ PTOKEN_CONTROL TokenControl);

VOID
NTAPI
SeDeassignPrimaryToken(
    _Inout_ PEPROCESS Process);

NTSTATUS
NTAPI
SeSubProcessToken(
    _In_ PTOKEN Parent,
    _Out_ PTOKEN *Token,
    _In_ BOOLEAN InUse,
    _In_ ULONG SessionId);

NTSTATUS
NTAPI
SeIsTokenChild(
    _In_ PTOKEN Token,
    _Out_ PBOOLEAN IsChild);

NTSTATUS
NTAPI
SeIsTokenSibling(
    _In_ PTOKEN Token,
    _Out_ PBOOLEAN IsSibling);

NTSTATUS
NTAPI
SeExchangePrimaryToken(
    _In_ PEPROCESS Process,
    _In_ PACCESS_TOKEN NewAccessToken,
    _Out_ PACCESS_TOKEN* OldAccessToken);

NTSTATUS
NTAPI
SeCopyClientToken(
    _In_ PACCESS_TOKEN Token,
    _In_ SECURITY_IMPERSONATION_LEVEL Level,
    _In_ KPROCESSOR_MODE PreviousMode,
    _Out_ PACCESS_TOKEN* NewToken);

BOOLEAN
NTAPI
SeTokenIsInert(
    _In_ PTOKEN Token);

ULONG
RtlLengthSidAndAttributes(
    _In_ ULONG Count,
    _In_ PSID_AND_ATTRIBUTES Src);

//
// Security Manager (SeMgr) functions
//
CODE_SEG("INIT")
BOOLEAN
NTAPI
SeInitSystem(VOID);

NTSTATUS
NTAPI
SeDefaultObjectMethod(
    _In_ PVOID Object,
    _In_ SECURITY_OPERATION_CODE OperationType,
    _In_ PSECURITY_INFORMATION SecurityInformation,
    _Inout_opt_ PSECURITY_DESCRIPTOR SecurityDescriptor,
    _Inout_opt_ PULONG ReturnLength,
    _Inout_opt_ PSECURITY_DESCRIPTOR *OldSecurityDescriptor,
    _In_ POOL_TYPE PoolType,
    _In_ PGENERIC_MAPPING GenericMapping);

VOID
NTAPI
SeQuerySecurityAccessMask(
    _In_ SECURITY_INFORMATION SecurityInformation,
    _Out_ PACCESS_MASK DesiredAccess);

VOID
NTAPI
SeSetSecurityAccessMask(
    _In_ SECURITY_INFORMATION SecurityInformation,
    _Out_ PACCESS_MASK DesiredAccess);

//
// Privilege functions
//
CODE_SEG("INIT")
VOID
NTAPI
SepInitPrivileges(VOID);

BOOLEAN
NTAPI
SepPrivilegeCheck(
    _In_ PTOKEN Token,
    _In_ PLUID_AND_ATTRIBUTES Privileges,
    _In_ ULONG PrivilegeCount,
    _In_ ULONG PrivilegeControl,
    _In_ KPROCESSOR_MODE PreviousMode);

NTSTATUS
NTAPI
SePrivilegePolicyCheck(
    _Inout_ PACCESS_MASK DesiredAccess,
    _Inout_ PACCESS_MASK GrantedAccess,
    _In_ PSECURITY_SUBJECT_CONTEXT SubjectContext,
    _In_ PTOKEN Token,
    _Out_opt_ PPRIVILEGE_SET *OutPrivilegeSet,
    _In_ KPROCESSOR_MODE PreviousMode);

BOOLEAN
NTAPI
SeCheckAuditPrivilege(
    _In_ PSECURITY_SUBJECT_CONTEXT SubjectContext,
    _In_ KPROCESSOR_MODE PreviousMode);

BOOLEAN
NTAPI
SeCheckPrivilegedObject(
    _In_ LUID PrivilegeValue,
    _In_ HANDLE ObjectHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ KPROCESSOR_MODE PreviousMode);

NTSTATUS
NTAPI
SeCaptureLuidAndAttributesArray(
    _In_ PLUID_AND_ATTRIBUTES Src,
    _In_ ULONG PrivilegeCount,
    _In_ KPROCESSOR_MODE PreviousMode,
    _In_ PLUID_AND_ATTRIBUTES AllocatedMem,
    _In_ ULONG AllocatedLength,
    _In_ POOL_TYPE PoolType,
    _In_ BOOLEAN CaptureIfKernel,
    _Out_ PLUID_AND_ATTRIBUTES* Dest,
    _Inout_ PULONG Length);

VOID
NTAPI
SeReleaseLuidAndAttributesArray(
    _In_ PLUID_AND_ATTRIBUTES Privilege,
    _In_ KPROCESSOR_MODE PreviousMode,
    _In_ BOOLEAN CaptureIfKernel);

//
// SID functions
//
CODE_SEG("INIT")
BOOLEAN
NTAPI
SepInitSecurityIDs(VOID);

NTSTATUS
NTAPI
SepCaptureSid(
    _In_ PSID InputSid,
    _In_ KPROCESSOR_MODE AccessMode,
    _In_ POOL_TYPE PoolType,
    _In_ BOOLEAN CaptureIfKernel,
    _Out_ PSID *CapturedSid);

VOID
NTAPI
SepReleaseSid(
    _In_ PSID CapturedSid,
    _In_ KPROCESSOR_MODE AccessMode,
    _In_ BOOLEAN CaptureIfKernel);

BOOLEAN
NTAPI
SepSidInToken(
    _In_ PACCESS_TOKEN _Token,
    _In_ PSID Sid);

BOOLEAN
NTAPI
SepSidInTokenEx(
    _In_ PACCESS_TOKEN _Token,
    _In_ PSID PrincipalSelfSid,
    _In_ PSID _Sid,
    _In_ BOOLEAN Deny,
    _In_ BOOLEAN Restricted);

PSID
NTAPI
SepGetSidFromAce(
    _In_ PACE Ace);

NTSTATUS
NTAPI
SeCaptureSidAndAttributesArray(
    _In_ PSID_AND_ATTRIBUTES SrcSidAndAttributes,
    _In_ ULONG AttributeCount,
    _In_ KPROCESSOR_MODE PreviousMode,
    _In_opt_ PVOID AllocatedMem,
    _In_ ULONG AllocatedLength,
    _In_ POOL_TYPE PoolType,
    _In_ BOOLEAN CaptureIfKernel,
    _Out_ PSID_AND_ATTRIBUTES *CapturedSidAndAttributes,
    _Out_ PULONG ResultLength);

VOID
NTAPI
SeReleaseSidAndAttributesArray(
    _In_ _Post_invalid_ PSID_AND_ATTRIBUTES CapturedSidAndAttributes,
    _In_ KPROCESSOR_MODE AccessMode,
    _In_ BOOLEAN CaptureIfKernel);

//
// ACL functions
//
CODE_SEG("INIT")
BOOLEAN
NTAPI
SepInitDACLs(VOID);

NTSTATUS
NTAPI
SepCreateImpersonationTokenDacl(
    _In_ PTOKEN Token,
    _In_ PTOKEN PrimaryToken,
    _Out_ PACL* Dacl);

NTSTATUS
NTAPI
SepCaptureAcl(
    _In_ PACL InputAcl,
    _In_ KPROCESSOR_MODE AccessMode,
    _In_ POOL_TYPE PoolType,
    _In_ BOOLEAN CaptureIfKernel,
    _Out_ PACL *CapturedAcl);

VOID
NTAPI
SepReleaseAcl(
    _In_ PACL CapturedAcl,
    _In_ KPROCESSOR_MODE AccessMode,
    _In_ BOOLEAN CaptureIfKernel);

NTSTATUS
SepPropagateAcl(
    _Out_writes_bytes_opt_(DaclLength) PACL AclDest,
    _Inout_ PULONG AclLength,
    _In_reads_bytes_(AclSource->AclSize) PACL AclSource,
    _In_ PSID Owner,
    _In_ PSID Group,
    _In_ BOOLEAN IsInherited,
    _In_ BOOLEAN IsDirectoryObject,
    _In_ PGENERIC_MAPPING GenericMapping);

PACL
SepSelectAcl(
    _In_opt_ PACL ExplicitAcl,
    _In_ BOOLEAN ExplicitPresent,
    _In_ BOOLEAN ExplicitDefaulted,
    _In_opt_ PACL ParentAcl,
    _In_opt_ PACL DefaultAcl,
    _Out_ PULONG AclLength,
    _In_ PSID Owner,
    _In_ PSID Group,
    _Out_ PBOOLEAN AclPresent,
    _Out_ PBOOLEAN IsInherited,
    _In_ BOOLEAN IsDirectoryObject,
    _In_ PGENERIC_MAPPING GenericMapping);

//
// SD functions
//
CODE_SEG("INIT")
BOOLEAN
NTAPI
SepInitSDs(VOID);

NTSTATUS
NTAPI
SeSetWorldSecurityDescriptor(
    _In_ SECURITY_INFORMATION SecurityInformation,
    _In_ PISECURITY_DESCRIPTOR SecurityDescriptor,
    _In_ PULONG BufferLength);

NTSTATUS
NTAPI
SeComputeQuotaInformationSize(
    _In_ PSECURITY_DESCRIPTOR SecurityDescriptor,
    _Out_ PULONG QuotaInfoSize);

//
// Security Reference Monitor (SeRm) functions
//
BOOLEAN
NTAPI
SeRmInitPhase0(VOID);

BOOLEAN
NTAPI
SeRmInitPhase1(VOID);

NTSTATUS
NTAPI
SepRmInsertLogonSessionIntoToken(
    _Inout_ PTOKEN Token);

NTSTATUS
NTAPI
SepRmRemoveLogonSessionFromToken(
    _Inout_ PTOKEN Token);

NTSTATUS
SepRmReferenceLogonSession(
    _Inout_ PLUID LogonLuid);

NTSTATUS
SepRmDereferenceLogonSession(
    _Inout_ PLUID LogonLuid);

NTSTATUS
NTAPI
SepRegQueryHelper(
    _In_ PCWSTR KeyName,
    _In_ PCWSTR ValueName,
    _In_ ULONG ValueType,
    _In_ ULONG DataLength,
    _Out_ PVOID ValueData);

NTSTATUS
NTAPI
SeGetLogonIdDeviceMap(
    _In_ PLUID LogonId,
    _Out_ PDEVICE_MAP *DeviceMap);

//
// Audit functions
//
NTSTATUS
NTAPI
SeInitializeProcessAuditName(
    _In_ PFILE_OBJECT FileObject,
    _In_ BOOLEAN DoAudit,
    _Out_ POBJECT_NAME_INFORMATION *AuditInfo);

BOOLEAN
NTAPI
SeDetailedAuditingWithToken(
    _In_ PTOKEN Token);

VOID
NTAPI
SeAuditProcessExit(
    _In_ PEPROCESS Process);

VOID
NTAPI
SeAuditProcessCreate(
    _In_ PEPROCESS Process);

VOID
NTAPI
SePrivilegedServiceAuditAlarm(
    _In_opt_ PUNICODE_STRING ServiceName,
    _In_ PSECURITY_SUBJECT_CONTEXT SubjectContext,
    _In_ PPRIVILEGE_SET PrivilegeSet,
    _In_ BOOLEAN AccessGranted);

//
// Subject functions
//
VOID
NTAPI
SeCaptureSubjectContextEx(
    _In_ PETHREAD Thread,
    _In_ PEPROCESS Process,
    _Out_ PSECURITY_SUBJECT_CONTEXT SubjectContext);

//
// Security Quality of Service (SQoS) functions
//
NTSTATUS
NTAPI
SepCaptureSecurityQualityOfService(
    _In_opt_ POBJECT_ATTRIBUTES ObjectAttributes,
    _In_ KPROCESSOR_MODE AccessMode,
    _In_ POOL_TYPE PoolType,
    _In_ BOOLEAN CaptureIfKernel,
    _Out_ PSECURITY_QUALITY_OF_SERVICE *CapturedSecurityQualityOfService,
    _Out_ PBOOLEAN Present);

VOID
NTAPI
SepReleaseSecurityQualityOfService(
    _In_opt_ PSECURITY_QUALITY_OF_SERVICE CapturedSecurityQualityOfService,
    _In_ KPROCESSOR_MODE AccessMode,
    _In_ BOOLEAN CaptureIfKernel);

//
// Object type list functions
//
PGUID
SepGetObjectTypeGuidFromAce(
    _In_ PACE Ace,
    _In_ BOOLEAN IsAceDenied);

BOOLEAN
SepObjectTypeGuidInList(
    _In_reads_(ObjectTypeListLength) POBJECT_TYPE_LIST_INTERNAL ObjectTypeList,
    _In_ ULONG ObjectTypeListLength,
    _In_ PGUID ObjectTypeGuid,
    _Out_ PULONG ObjectIndex);

NTSTATUS
SeCaptureObjectTypeList(
    _In_reads_opt_(ObjectTypeListLength) POBJECT_TYPE_LIST ObjectTypeList,
    _In_ ULONG ObjectTypeListLength,
    _In_ KPROCESSOR_MODE PreviousMode,
    _Out_ POBJECT_TYPE_LIST_INTERNAL *CapturedObjectTypeList);

VOID
SeReleaseObjectTypeList(
    _In_  _Post_invalid_ POBJECT_TYPE_LIST_INTERNAL CapturedObjectTypeList,
    _In_ KPROCESSOR_MODE PreviousMode);

//
// Access state functions
//
NTSTATUS
NTAPI
SeCreateAccessStateEx(
    _In_ PETHREAD Thread,
    _In_ PEPROCESS Process,
    _In_ OUT PACCESS_STATE AccessState,
    _In_ PAUX_ACCESS_DATA AuxData,
    _In_ ACCESS_MASK Access,
    _In_ PGENERIC_MAPPING GenericMapping);

//
// Access check functions
//
BOOLEAN
NTAPI
SeFastTraverseCheck(
    _In_ PSECURITY_DESCRIPTOR SecurityDescriptor,
    _In_ PACCESS_STATE AccessState,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ KPROCESSOR_MODE AccessMode);

#endif

/* EOF */
