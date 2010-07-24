/*++ NDK Version: 0098

Copyright (c) Alex Ionescu.  All rights reserved.

Header Name:

    sefuncs.h

Abstract:

    Function definitions for the security manager.

Author:

    Alex Ionescu (alexi@tinykrnl.org) - Updated - 27-Feb-2006

--*/

#ifndef _SEFUNCS_H
#define _SEFUNCS_H

//
// Dependencies
//
#include <umtypes.h>

#ifndef NTOS_MODE_USER

//
// Security Descriptors
//
NTKERNELAPI
NTSTATUS
NTAPI
SeCaptureSecurityDescriptor(
    IN PSECURITY_DESCRIPTOR OriginalSecurityDescriptor,
    IN KPROCESSOR_MODE CurrentMode,
    IN POOL_TYPE PoolType,
    IN BOOLEAN CaptureIfKernel,
    OUT PSECURITY_DESCRIPTOR *CapturedSecurityDescriptor
);

NTKERNELAPI
NTSTATUS
NTAPI
SeReleaseSecurityDescriptor(
    IN PSECURITY_DESCRIPTOR CapturedSecurityDescriptor,
    IN KPROCESSOR_MODE CurrentMode,
    IN BOOLEAN CaptureIfKernelMode
);

//
// Access States
//
NTKERNELAPI
NTSTATUS
NTAPI
SeCreateAccessState(
    PACCESS_STATE AccessState,
    PAUX_ACCESS_DATA AuxData,
    ACCESS_MASK Access,
    PGENERIC_MAPPING GenericMapping
);

NTKERNELAPI
VOID
NTAPI
SeDeleteAccessState(
    IN PACCESS_STATE AccessState
);

//
// Impersonation
//
NTKERNELAPI
SECURITY_IMPERSONATION_LEVEL
NTAPI
SeTokenImpersonationLevel(
    IN PACCESS_TOKEN Token
);

#endif

//
// Native Calls
//
NTSYSCALLAPI
NTSTATUS
NTAPI
NtAccessCheck(
    IN PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN HANDLE ClientToken,
    IN ACCESS_MASK DesiredAccess,
    IN PGENERIC_MAPPING GenericMapping,
    OUT PPRIVILEGE_SET PrivilegeSet,
    OUT PULONG ReturnLength,
    OUT PACCESS_MASK GrantedAccess,
    OUT PNTSTATUS AccessStatus
);

NTSTATUS
NTAPI
NtAccessCheckByType(
    IN PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN PSID PrincipalSelfSid,
    IN HANDLE ClientToken,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_TYPE_LIST ObjectTypeList,
    IN ULONG ObjectTypeLength,
    IN PGENERIC_MAPPING GenericMapping,
    IN PPRIVILEGE_SET PrivilegeSet,
    IN ULONG PrivilegeSetLength,
    OUT PACCESS_MASK GrantedAccess,
    OUT PNTSTATUS AccessStatus
);

NTSTATUS
NTAPI
NtAccessCheckByTypeResultList(
    IN PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN PSID PrincipalSelfSid,
    IN HANDLE ClientToken,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_TYPE_LIST ObjectTypeList,
    IN ULONG ObjectTypeLength,
    IN PGENERIC_MAPPING GenericMapping,
    IN PPRIVILEGE_SET PrivilegeSet,
    IN ULONG PrivilegeSetLength,
    OUT PACCESS_MASK GrantedAccess,
    OUT PNTSTATUS AccessStatus
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtAccessCheckAndAuditAlarm(
    IN PUNICODE_STRING SubsystemName,
    IN PVOID HandleId,
    IN PUNICODE_STRING ObjectTypeName,
    IN PUNICODE_STRING ObjectName,
    IN PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN ACCESS_MASK DesiredAccess,
    IN PGENERIC_MAPPING GenericMapping,
    IN BOOLEAN ObjectCreation,
    OUT PACCESS_MASK GrantedAccess,
    OUT PNTSTATUS AccessStatus,
    OUT PBOOLEAN GenerateOnClose
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtAdjustGroupsToken(
    IN HANDLE TokenHandle,
    IN BOOLEAN ResetToDefault,
    IN PTOKEN_GROUPS NewState,
    IN ULONG BufferLength,
    OUT PTOKEN_GROUPS PreviousState OPTIONAL,
    OUT PULONG ReturnLength
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtAdjustPrivilegesToken(
    IN HANDLE TokenHandle,
    IN BOOLEAN DisableAllPrivileges,
    IN PTOKEN_PRIVILEGES NewState,
    IN ULONG BufferLength,
    OUT PTOKEN_PRIVILEGES PreviousState,
    OUT PULONG ReturnLength
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtAllocateLocallyUniqueId(
    OUT LUID *LocallyUniqueId
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtAllocateUuids(
    PULARGE_INTEGER Time,
    PULONG Range,
    PULONG Sequence,
    PUCHAR Seed
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtCompareTokens(
    IN HANDLE FirstTokenHandle,
    IN HANDLE SecondTokenHandle,
    OUT PBOOLEAN Equal);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtCreateToken(
    OUT PHANDLE TokenHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    IN TOKEN_TYPE TokenType,
    IN PLUID AuthenticationId,
    IN PLARGE_INTEGER ExpirationTime,
    IN PTOKEN_USER TokenUser,
    IN PTOKEN_GROUPS TokenGroups,
    IN PTOKEN_PRIVILEGES TokenPrivileges,
    IN PTOKEN_OWNER TokenOwner,
    IN PTOKEN_PRIMARY_GROUP TokenPrimaryGroup,
    IN PTOKEN_DEFAULT_DACL TokenDefaultDacl,
    IN PTOKEN_SOURCE TokenSource
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtDuplicateToken(
    IN HANDLE ExistingTokenHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
    IN BOOLEAN EffectiveOnly,
    IN TOKEN_TYPE TokenType,
    OUT PHANDLE NewTokenHandle
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtImpersonateAnonymousToken(
    IN HANDLE Thread
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtOpenObjectAuditAlarm(
    IN PUNICODE_STRING SubsystemName,
    IN PVOID HandleId,
    IN PUNICODE_STRING ObjectTypeName,
    IN PUNICODE_STRING ObjectName,
    IN PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN HANDLE ClientToken,
    IN ULONG DesiredAccess,
    IN ULONG GrantedAccess,
    IN PPRIVILEGE_SET Privileges,
    IN BOOLEAN ObjectCreation,
    IN BOOLEAN AccessGranted,
    OUT PBOOLEAN GenerateOnClose
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtOpenProcessToken(
    IN HANDLE ProcessHandle,
    IN ACCESS_MASK DesiredAccess,
    OUT PHANDLE TokenHandle
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtOpenProcessTokenEx(
    IN HANDLE ProcessHandle,
    IN ACCESS_MASK DesiredAccess,
    IN ULONG HandleAttributes,
    OUT PHANDLE TokenHandle
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtPrivilegeCheck(
    IN HANDLE ClientToken,
    IN PPRIVILEGE_SET RequiredPrivileges,
    IN PBOOLEAN Result
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtPrivilegedServiceAuditAlarm(
    IN PUNICODE_STRING SubsystemName,
    IN PUNICODE_STRING ServiceName,
    IN HANDLE ClientToken,
    IN PPRIVILEGE_SET Privileges,
    IN BOOLEAN AccessGranted
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtPrivilegeObjectAuditAlarm(
    IN PUNICODE_STRING SubsystemName,
    IN PVOID HandleId,
    IN HANDLE ClientToken,
    IN ULONG DesiredAccess,
    IN PPRIVILEGE_SET Privileges,
    IN BOOLEAN AccessGranted
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryInformationToken(
    IN HANDLE TokenHandle,
    IN TOKEN_INFORMATION_CLASS TokenInformationClass,
    OUT PVOID TokenInformation,
    IN ULONG TokenInformationLength,
    OUT PULONG ReturnLength
);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetInformationToken(
    IN HANDLE TokenHandle,
    IN TOKEN_INFORMATION_CLASS TokenInformationClass,
    OUT PVOID TokenInformation,
    IN ULONG TokenInformationLength
);

NTSYSAPI
NTSTATUS
NTAPI
ZwAccessCheck(
    IN PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN HANDLE ClientToken,
    IN ACCESS_MASK DesiredAccess,
    IN PGENERIC_MAPPING GenericMapping,
    OUT PPRIVILEGE_SET PrivilegeSet,
    OUT PULONG ReturnLength,
    OUT PACCESS_MASK GrantedAccess,
    OUT PNTSTATUS AccessStatus
);

NTSYSAPI
NTSTATUS
NTAPI
ZwAdjustGroupsToken(
    IN HANDLE TokenHandle,
    IN BOOLEAN ResetToDefault,
    IN PTOKEN_GROUPS NewState,
    IN ULONG BufferLength,
    OUT PTOKEN_GROUPS PreviousState OPTIONAL,
    OUT PULONG ReturnLength
);

NTSYSAPI
NTSTATUS
NTAPI
ZwAdjustPrivilegesToken(
    IN HANDLE TokenHandle,
    IN BOOLEAN DisableAllPrivileges,
    IN PTOKEN_PRIVILEGES NewState,
    IN ULONG BufferLength,
    OUT PTOKEN_PRIVILEGES PreviousState,
    OUT PULONG ReturnLength
);

NTSYSAPI
NTSTATUS
NTAPI
ZwAllocateLocallyUniqueId(
    OUT LUID *LocallyUniqueId
);

NTSYSAPI
NTSTATUS
NTAPI
ZwAllocateUuids(
    PULARGE_INTEGER Time,
    PULONG Range,
    PULONG Sequence,
    PUCHAR Seed
);

NTSYSAPI
NTSTATUS
NTAPI
ZwCreateToken(
    OUT PHANDLE TokenHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    IN TOKEN_TYPE TokenType,
    IN PLUID AuthenticationId,
    IN PLARGE_INTEGER ExpirationTime,
    IN PTOKEN_USER TokenUser,
    IN PTOKEN_GROUPS TokenGroups,
    IN PTOKEN_PRIVILEGES TokenPrivileges,
    IN PTOKEN_OWNER TokenOwner,
    IN PTOKEN_PRIMARY_GROUP TokenPrimaryGroup,
    IN PTOKEN_DEFAULT_DACL TokenDefaultDacl,
    IN PTOKEN_SOURCE TokenSource
);

NTSYSAPI
NTSTATUS
NTAPI
ZwDuplicateToken(
    IN HANDLE ExistingTokenHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
    IN BOOLEAN EffectiveOnly,
    IN TOKEN_TYPE TokenType,
    OUT PHANDLE NewTokenHandle
);

NTSYSAPI
NTSTATUS
NTAPI
ZwImpersonateAnonymousToken(
    IN HANDLE Thread
);

NTSYSAPI
NTSTATUS
NTAPI
ZwOpenObjectAuditAlarm(
    IN PUNICODE_STRING SubsystemName,
    IN PVOID HandleId,
    IN PUNICODE_STRING ObjectTypeName,
    IN PUNICODE_STRING ObjectName,
    IN PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN HANDLE ClientToken,
    IN ULONG DesiredAccess,
    IN ULONG GrantedAccess,
    IN PPRIVILEGE_SET Privileges,
    IN BOOLEAN ObjectCreation,
    IN BOOLEAN AccessGranted,
    OUT PBOOLEAN GenerateOnClose
);

NTSYSAPI
NTSTATUS
NTAPI
ZwOpenProcessToken(
    IN HANDLE ProcessHandle,
    IN ACCESS_MASK DesiredAccess,
    OUT PHANDLE TokenHandle
);

NTSYSAPI
NTSTATUS
NTAPI
ZwOpenProcessTokenEx(
    IN HANDLE ProcessHandle,
    IN ACCESS_MASK DesiredAccess,
    IN ULONG HandleAttributes,
    OUT PHANDLE TokenHandle
);

NTSYSAPI
NTSTATUS
NTAPI
ZwPrivilegeCheck(
    IN HANDLE ClientToken,
    IN PPRIVILEGE_SET RequiredPrivileges,
    IN PBOOLEAN Result
);

NTSYSAPI
NTSTATUS
NTAPI
ZwPrivilegedServiceAuditAlarm(
    IN PUNICODE_STRING SubsystemName,
    IN PUNICODE_STRING ServiceName,
    IN HANDLE ClientToken,
    IN PPRIVILEGE_SET Privileges,
    IN BOOLEAN AccessGranted
);

NTSYSAPI
NTSTATUS
NTAPI
ZwPrivilegeObjectAuditAlarm(
    IN PUNICODE_STRING SubsystemName,
    IN PVOID HandleId,
    IN HANDLE ClientToken,
    IN ULONG DesiredAccess,
    IN PPRIVILEGE_SET Privileges,
    IN BOOLEAN AccessGranted
);

NTSYSAPI
NTSTATUS
NTAPI
ZwQueryInformationToken(
    IN HANDLE TokenHandle,
    IN TOKEN_INFORMATION_CLASS TokenInformationClass,
    OUT PVOID TokenInformation,
    IN ULONG TokenInformationLength,
    OUT PULONG ReturnLength
);

NTSYSAPI
NTSTATUS
NTAPI
ZwSetInformationToken(
    IN HANDLE TokenHandle,
    IN TOKEN_INFORMATION_CLASS TokenInformationClass,
    OUT PVOID TokenInformation,
    IN ULONG TokenInformationLength
);
#endif
