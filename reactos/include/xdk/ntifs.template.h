/*
 * ntifs.h
 *
 * Windows NT Filesystem Driver Developer Kit
 *
 * This file is part of the ReactOS DDK package.
 *
 * Contributors:
 *   Amine Khaldi
 *   Timo Kreuzer (timo.kreuzer@reactos.org)
 *
 * THIS SOFTWARE IS NOT COPYRIGHTED
 *
 * This source code is offered for use in the public domain. You may
 * use, modify or distribute it freely.
 *
 * This code is distributed in the hope that it will be useful but
 * WITHOUT ANY WARRANTY. ALL WARRANTIES, EXPRESS OR IMPLIED ARE HEREBY
 * DISCLAIMED. This includes but is not limited to warranties of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 */

#pragma once

#define _NTIFS_INCLUDED_
#define _GNU_NTIFS_

#ifdef __cplusplus
extern "C" {
#endif

/* Dependencies */
#include <ntddk.h>
#include <excpt.h>
#include <ntdef.h>
#include <ntnls.h>
#include <ntstatus.h>
#include <bugcodes.h>
#include <ntiologc.h>

$define (_NTIFS_)

#ifndef FlagOn
#define FlagOn(_F,_SF)        ((_F) & (_SF))
#endif

#ifndef BooleanFlagOn
#define BooleanFlagOn(F,SF)   ((BOOLEAN)(((F) & (SF)) != 0))
#endif

#ifndef SetFlag
#define SetFlag(_F,_SF)       ((_F) |= (_SF))
#endif

#ifndef ClearFlag
#define ClearFlag(_F,_SF)     ((_F) &= ~(_SF))
#endif

typedef UNICODE_STRING LSA_UNICODE_STRING, *PLSA_UNICODE_STRING;
typedef STRING LSA_STRING, *PLSA_STRING;
typedef OBJECT_ATTRIBUTES LSA_OBJECT_ATTRIBUTES, *PLSA_OBJECT_ATTRIBUTES;

$include (setypes.h)
$include (rtltypes.h)
$include (rtlfuncs.h)

typedef enum _OBJECT_INFORMATION_CLASS {
  ObjectBasicInformation = 0,
  ObjectNameInformation = 1, /* FIXME, not in WDK */
  ObjectTypeInformation = 2,
  ObjectTypesInformation = 3, /* FIXME, not in WDK */
  ObjectHandleFlagInformation = 4, /* FIXME, not in WDK */
  ObjectSessionInformation = 5, /* FIXME, not in WDK */
  MaxObjectInfoClass /* FIXME, not in WDK */
} OBJECT_INFORMATION_CLASS;

NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryObject(
  IN HANDLE Handle OPTIONAL,
  IN OBJECT_INFORMATION_CLASS ObjectInformationClass,
  OUT PVOID ObjectInformation OPTIONAL,
  IN ULONG ObjectInformationLength,
  OUT PULONG ReturnLength OPTIONAL);

#if (NTDDI_VERSION >= NTDDI_WIN2K)

NTSYSCALLAPI
NTSTATUS
NTAPI
NtOpenThreadToken(
  IN HANDLE ThreadHandle,
  IN ACCESS_MASK DesiredAccess,
  IN BOOLEAN OpenAsSelf,
  OUT PHANDLE TokenHandle);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtOpenProcessToken(
  IN HANDLE ProcessHandle,
  IN ACCESS_MASK DesiredAccess,
  OUT PHANDLE TokenHandle);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryInformationToken(
  IN HANDLE TokenHandle,
  IN TOKEN_INFORMATION_CLASS TokenInformationClass,
  OUT PVOID TokenInformation OPTIONAL,
  IN ULONG TokenInformationLength,
  OUT PULONG ReturnLength);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtAdjustPrivilegesToken(
  IN HANDLE TokenHandle,
  IN BOOLEAN DisableAllPrivileges,
  IN PTOKEN_PRIVILEGES NewState OPTIONAL,
  IN ULONG BufferLength,
  OUT PTOKEN_PRIVILEGES PreviousState,
  OUT PULONG ReturnLength OPTIONAL);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtCreateFile(
  OUT PHANDLE FileHandle,
  IN ACCESS_MASK DesiredAccess,
  IN POBJECT_ATTRIBUTES ObjectAttributes,
  OUT PIO_STATUS_BLOCK IoStatusBlock,
  IN PLARGE_INTEGER AllocationSize OPTIONAL,
  IN ULONG FileAttributes,
  IN ULONG ShareAccess,
  IN ULONG CreateDisposition,
  IN ULONG CreateOptions,
  IN PVOID EaBuffer,
  IN ULONG EaLength);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtDeviceIoControlFile(
  IN HANDLE FileHandle,
  IN HANDLE Event OPTIONAL,
  IN PIO_APC_ROUTINE ApcRoutine OPTIONAL,
  IN PVOID ApcContext OPTIONAL,
  OUT PIO_STATUS_BLOCK IoStatusBlock,
  IN ULONG IoControlCode,
  IN PVOID InputBuffer OPTIONAL,
  IN ULONG InputBufferLength,
  OUT PVOID OutputBuffer OPTIONAL,
  IN ULONG OutputBufferLength);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtFsControlFile(
  IN HANDLE FileHandle,
  IN HANDLE Event OPTIONAL,
  IN PIO_APC_ROUTINE ApcRoutine OPTIONAL,
  IN PVOID ApcContext OPTIONAL,
  OUT PIO_STATUS_BLOCK IoStatusBlock,
  IN ULONG FsControlCode,
  IN PVOID InputBuffer OPTIONAL,
  IN ULONG InputBufferLength,
  OUT PVOID OutputBuffer OPTIONAL,
  IN ULONG OutputBufferLength);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtLockFile(
  IN HANDLE FileHandle,
  IN HANDLE Event OPTIONAL,
  IN PIO_APC_ROUTINE ApcRoutine OPTIONAL,
  IN PVOID ApcContext OPTIONAL,
  OUT PIO_STATUS_BLOCK IoStatusBlock,
  IN PLARGE_INTEGER ByteOffset,
  IN PLARGE_INTEGER Length,
  IN ULONG Key,
  IN BOOLEAN FailImmediately,
  IN BOOLEAN ExclusiveLock);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtOpenFile(
  OUT PHANDLE FileHandle,
  IN ACCESS_MASK DesiredAccess,
  IN POBJECT_ATTRIBUTES ObjectAttributes,
  OUT PIO_STATUS_BLOCK IoStatusBlock,
  IN ULONG ShareAccess,
  IN ULONG OpenOptions);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryDirectoryFile(
  IN HANDLE FileHandle,
  IN HANDLE Event OPTIONAL,
  IN PIO_APC_ROUTINE ApcRoutine OPTIONAL,
  IN PVOID ApcContext OPTIONAL,
  OUT PIO_STATUS_BLOCK IoStatusBlock,
  OUT PVOID FileInformation,
  IN ULONG Length,
  IN FILE_INFORMATION_CLASS FileInformationClass,
  IN BOOLEAN ReturnSingleEntry,
  IN PUNICODE_STRING FileName OPTIONAL,
  IN BOOLEAN RestartScan);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryInformationFile(
  IN HANDLE FileHandle,
  OUT PIO_STATUS_BLOCK IoStatusBlock,
  OUT PVOID FileInformation,
  IN ULONG Length,
  IN FILE_INFORMATION_CLASS FileInformationClass);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryQuotaInformationFile(
  IN HANDLE FileHandle,
  OUT PIO_STATUS_BLOCK IoStatusBlock,
  OUT PVOID Buffer,
  IN ULONG Length,
  IN BOOLEAN ReturnSingleEntry,
  IN PVOID SidList,
  IN ULONG SidListLength,
  IN PSID StartSid OPTIONAL,
  IN BOOLEAN RestartScan);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryVolumeInformationFile(
  IN HANDLE FileHandle,
  OUT PIO_STATUS_BLOCK IoStatusBlock,
  OUT PVOID FsInformation,
  IN ULONG Length,
  IN FS_INFORMATION_CLASS FsInformationClass);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtReadFile(
  IN HANDLE FileHandle,
  IN HANDLE Event OPTIONAL,
  IN PIO_APC_ROUTINE ApcRoutine OPTIONAL,
  IN PVOID ApcContext OPTIONAL,
  OUT PIO_STATUS_BLOCK IoStatusBlock,
  OUT PVOID Buffer,
  IN ULONG Length,
  IN PLARGE_INTEGER ByteOffset OPTIONAL,
  IN PULONG Key OPTIONAL);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetInformationFile(
  IN HANDLE FileHandle,
  OUT PIO_STATUS_BLOCK IoStatusBlock,
  IN PVOID FileInformation,
  IN ULONG Length,
  IN FILE_INFORMATION_CLASS FileInformationClass);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetQuotaInformationFile(
  IN HANDLE FileHandle,
  OUT PIO_STATUS_BLOCK IoStatusBlock,
  IN PVOID Buffer,
  IN ULONG Length);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetVolumeInformationFile(
  IN HANDLE FileHandle,
  OUT PIO_STATUS_BLOCK IoStatusBlock,
  IN PVOID FsInformation,
  IN ULONG Length,
  IN FS_INFORMATION_CLASS FsInformationClass);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtWriteFile(
  IN HANDLE FileHandle,
  IN HANDLE Event OPTIONAL,
  IN PIO_APC_ROUTINE ApcRoutine OPTIONAL,
  IN PVOID ApcContext OPTIONAL,
  OUT PIO_STATUS_BLOCK IoStatusBlock,
  IN PVOID Buffer,
  IN ULONG Length,
  IN PLARGE_INTEGER ByteOffset OPTIONAL,
  IN PULONG Key OPTIONAL);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtUnlockFile(
  IN HANDLE FileHandle,
  OUT PIO_STATUS_BLOCK IoStatusBlock,
  IN PLARGE_INTEGER ByteOffset,
  IN PLARGE_INTEGER Length,
  IN ULONG Key);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetSecurityObject(
  IN HANDLE Handle,
  IN SECURITY_INFORMATION SecurityInformation,
  IN PSECURITY_DESCRIPTOR SecurityDescriptor);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtQuerySecurityObject(
  IN HANDLE Handle,
  IN SECURITY_INFORMATION SecurityInformation,
  OUT PSECURITY_DESCRIPTOR SecurityDescriptor,
  IN ULONG Length,
  OUT PULONG LengthNeeded);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtClose(
  IN HANDLE Handle);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtAllocateVirtualMemory(
  IN HANDLE ProcessHandle,
  IN OUT PVOID *BaseAddress,
  IN ULONG_PTR ZeroBits,
  IN OUT PSIZE_T RegionSize,
  IN ULONG AllocationType,
  IN ULONG Protect);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtFreeVirtualMemory(
  IN HANDLE ProcessHandle,
  IN OUT PVOID *BaseAddress,
  IN OUT PSIZE_T RegionSize,
  IN ULONG FreeType);

#endif

#if (NTDDI_VERSION >= NTDDI_WINXP)

NTSYSCALLAPI
NTSTATUS
NTAPI
NtOpenThreadTokenEx(
  IN HANDLE ThreadHandle,
  IN ACCESS_MASK DesiredAccess,
  IN BOOLEAN OpenAsSelf,
  IN ULONG HandleAttributes,
  OUT PHANDLE TokenHandle);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtOpenProcessTokenEx(
  IN HANDLE ProcessHandle,
  IN ACCESS_MASK DesiredAccess,
  IN ULONG HandleAttributes,
  OUT PHANDLE TokenHandle);

NTSYSAPI
NTSTATUS
NTAPI
NtOpenJobObjectToken(
  IN HANDLE JobHandle,
  IN ACCESS_MASK DesiredAccess,
  OUT PHANDLE TokenHandle);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtDuplicateToken(
  IN HANDLE ExistingTokenHandle,
  IN ACCESS_MASK DesiredAccess,
  IN POBJECT_ATTRIBUTES ObjectAttributes,
  IN BOOLEAN EffectiveOnly,
  IN TOKEN_TYPE TokenType,
  OUT PHANDLE NewTokenHandle);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtFilterToken(
  IN HANDLE ExistingTokenHandle,
  IN ULONG Flags,
  IN PTOKEN_GROUPS SidsToDisable OPTIONAL,
  IN PTOKEN_PRIVILEGES PrivilegesToDelete OPTIONAL,
  IN PTOKEN_GROUPS RestrictedSids OPTIONAL,
  OUT PHANDLE NewTokenHandle);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtImpersonateAnonymousToken(
  IN HANDLE ThreadHandle);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetInformationToken(
  IN HANDLE TokenHandle,
  IN TOKEN_INFORMATION_CLASS TokenInformationClass,
  IN PVOID TokenInformation,
  IN ULONG TokenInformationLength);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtAdjustGroupsToken(
  IN HANDLE TokenHandle,
  IN BOOLEAN ResetToDefault,
  IN PTOKEN_GROUPS NewState OPTIONAL,
  IN ULONG BufferLength OPTIONAL,
  OUT PTOKEN_GROUPS PreviousState,
  OUT PULONG ReturnLength);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtPrivilegeCheck(
  IN HANDLE ClientToken,
  IN OUT PPRIVILEGE_SET RequiredPrivileges,
  OUT PBOOLEAN Result);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtAccessCheckAndAuditAlarm(
  IN PUNICODE_STRING SubsystemName,
  IN PVOID HandleId OPTIONAL,
  IN PUNICODE_STRING ObjectTypeName,
  IN PUNICODE_STRING ObjectName,
  IN PSECURITY_DESCRIPTOR SecurityDescriptor,
  IN ACCESS_MASK DesiredAccess,
  IN PGENERIC_MAPPING GenericMapping,
  IN BOOLEAN ObjectCreation,
  OUT PACCESS_MASK GrantedAccess,
  OUT PNTSTATUS AccessStatus,
  OUT PBOOLEAN GenerateOnClose);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtAccessCheckByTypeAndAuditAlarm(
  IN PUNICODE_STRING SubsystemName,
  IN PVOID HandleId,
  IN PUNICODE_STRING ObjectTypeName,
  IN PUNICODE_STRING ObjectName,
  IN PSECURITY_DESCRIPTOR SecurityDescriptor,
  IN PSID PrincipalSelfSid OPTIONAL,
  IN ACCESS_MASK DesiredAccess,
  IN AUDIT_EVENT_TYPE AuditType,
  IN ULONG Flags,
  IN POBJECT_TYPE_LIST ObjectTypeList OPTIONAL,
  IN ULONG ObjectTypeLength,
  IN PGENERIC_MAPPING GenericMapping,
  IN BOOLEAN ObjectCreation,
  OUT PACCESS_MASK GrantedAccess,
  OUT PNTSTATUS AccessStatus,
  OUT PBOOLEAN GenerateOnClose);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtAccessCheckByTypeResultListAndAuditAlarm(
  IN PUNICODE_STRING SubsystemName,
  IN PVOID HandleId OPTIONAL,
  IN PUNICODE_STRING ObjectTypeName,
  IN PUNICODE_STRING ObjectName,
  IN PSECURITY_DESCRIPTOR SecurityDescriptor,
  IN PSID PrincipalSelfSid OPTIONAL,
  IN ACCESS_MASK DesiredAccess,
  IN AUDIT_EVENT_TYPE AuditType,
  IN ULONG Flags,
  IN POBJECT_TYPE_LIST ObjectTypeList OPTIONAL,
  IN ULONG ObjectTypeLength,
  IN PGENERIC_MAPPING GenericMapping,
  IN BOOLEAN ObjectCreation,
  OUT PACCESS_MASK GrantedAccess,
  OUT PNTSTATUS AccessStatus,
  OUT PBOOLEAN GenerateOnClose);

NTSTATUS
NTAPI
NtAccessCheckByTypeResultListAndAuditAlarmByHandle(
  IN PUNICODE_STRING SubsystemName,
  IN PVOID HandleId OPTIONAL,
  IN HANDLE ClientToken,
  IN PUNICODE_STRING ObjectTypeName,
  IN PUNICODE_STRING ObjectName,
  IN PSECURITY_DESCRIPTOR SecurityDescriptor,
  IN PSID PrincipalSelfSid OPTIONAL,
  IN ACCESS_MASK DesiredAccess,
  IN AUDIT_EVENT_TYPE AuditType,
  IN ULONG Flags,
  IN POBJECT_TYPE_LIST ObjectTypeList OPTIONAL,
  IN ULONG ObjectTypeLength,
  IN PGENERIC_MAPPING GenericMapping,
  IN BOOLEAN ObjectCreation,
  OUT PACCESS_MASK GrantedAccess,
  OUT PNTSTATUS AccessStatus,
  OUT PBOOLEAN GenerateOnClose);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtOpenObjectAuditAlarm(
  IN PUNICODE_STRING SubsystemName,
  IN PVOID HandleId OPTIONAL,
  IN PUNICODE_STRING ObjectTypeName,
  IN PUNICODE_STRING ObjectName,
  IN PSECURITY_DESCRIPTOR SecurityDescriptor OPTIONAL,
  IN HANDLE ClientToken,
  IN ACCESS_MASK DesiredAccess,
  IN ACCESS_MASK GrantedAccess,
  IN PPRIVILEGE_SET Privileges OPTIONAL,
  IN BOOLEAN ObjectCreation,
  IN BOOLEAN AccessGranted,
  OUT PBOOLEAN GenerateOnClose);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtPrivilegeObjectAuditAlarm(
  IN PUNICODE_STRING SubsystemName,
  IN PVOID HandleId OPTIONAL,
  IN HANDLE ClientToken,
  IN ACCESS_MASK DesiredAccess,
  IN PPRIVILEGE_SET Privileges,
  IN BOOLEAN AccessGranted);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtCloseObjectAuditAlarm(
  IN PUNICODE_STRING SubsystemName,
  IN PVOID HandleId OPTIONAL,
  IN BOOLEAN GenerateOnClose);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtDeleteObjectAuditAlarm(
  IN PUNICODE_STRING SubsystemName,
  IN PVOID HandleId OPTIONAL,
  IN BOOLEAN GenerateOnClose);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtPrivilegedServiceAuditAlarm(
  IN PUNICODE_STRING SubsystemName,
  IN PUNICODE_STRING ServiceName,
  IN HANDLE ClientToken,
  IN PPRIVILEGE_SET Privileges,
  IN BOOLEAN AccessGranted);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetInformationThread(
  IN HANDLE ThreadHandle,
  IN THREADINFOCLASS ThreadInformationClass,
  IN PVOID ThreadInformation,
  IN ULONG ThreadInformationLength);

NTSYSCALLAPI
NTSTATUS
NTAPI
NtCreateSection(
  OUT PHANDLE SectionHandle,
  IN ACCESS_MASK DesiredAccess,
  IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
  IN PLARGE_INTEGER MaximumSize OPTIONAL,
  IN ULONG SectionPageProtection,
  IN ULONG AllocationAttributes,
  IN HANDLE FileHandle OPTIONAL);

#endif

#define COMPRESSION_FORMAT_NONE         (0x0000)
#define COMPRESSION_FORMAT_DEFAULT      (0x0001)
#define COMPRESSION_FORMAT_LZNT1        (0x0002)
#define COMPRESSION_ENGINE_STANDARD     (0x0000)
#define COMPRESSION_ENGINE_MAXIMUM      (0x0100)
#define COMPRESSION_ENGINE_HIBER        (0x0200)

#define MAX_UNICODE_STACK_BUFFER_LENGTH 256

#define METHOD_FROM_CTL_CODE(ctrlCode)  ((ULONG)(ctrlCode & 3))

#define METHOD_DIRECT_TO_HARDWARE       METHOD_IN_DIRECT
#define METHOD_DIRECT_FROM_HARDWARE     METHOD_OUT_DIRECT

typedef ULONG LSA_OPERATIONAL_MODE, *PLSA_OPERATIONAL_MODE;

typedef enum _SECURITY_LOGON_TYPE {
  UndefinedLogonType = 0,
  Interactive = 2,
  Network,
  Batch,
  Service,
  Proxy,
  Unlock,
  NetworkCleartext,
  NewCredentials,
#if (_WIN32_WINNT >= 0x0501)
  RemoteInteractive,
  CachedInteractive,
#endif
#if (_WIN32_WINNT >= 0x0502)
  CachedRemoteInteractive,
  CachedUnlock
#endif
} SECURITY_LOGON_TYPE, *PSECURITY_LOGON_TYPE;

#ifndef _NTLSA_AUDIT_
#define _NTLSA_AUDIT_

#ifndef GUID_DEFINED
#include <guiddef.h>
#endif

#endif /* _NTLSA_AUDIT_ */

NTSTATUS
NTAPI
LsaRegisterLogonProcess(
  IN PLSA_STRING LogonProcessName,
  OUT PHANDLE LsaHandle,
  OUT PLSA_OPERATIONAL_MODE SecurityMode);

NTSTATUS
NTAPI
LsaLogonUser(
  IN HANDLE LsaHandle,
  IN PLSA_STRING OriginName,
  IN SECURITY_LOGON_TYPE LogonType,
  IN ULONG AuthenticationPackage,
  IN PVOID AuthenticationInformation,
  IN ULONG AuthenticationInformationLength,
  IN PTOKEN_GROUPS LocalGroups OPTIONAL,
  IN PTOKEN_SOURCE SourceContext,
  OUT PVOID *ProfileBuffer,
  OUT PULONG ProfileBufferLength,
  OUT PLUID LogonId,
  OUT PHANDLE Token,
  OUT PQUOTA_LIMITS Quotas,
  OUT PNTSTATUS SubStatus);

NTSTATUS
NTAPI
LsaFreeReturnBuffer(
  IN PVOID Buffer);

#ifndef _NTLSA_IFS_
#define _NTLSA_IFS_
#endif

#define MSV1_0_PACKAGE_NAME     "MICROSOFT_AUTHENTICATION_PACKAGE_V1_0"
#define MSV1_0_PACKAGE_NAMEW    L"MICROSOFT_AUTHENTICATION_PACKAGE_V1_0"
#define MSV1_0_PACKAGE_NAMEW_LENGTH sizeof(MSV1_0_PACKAGE_NAMEW) - sizeof(WCHAR)

#define MSV1_0_SUBAUTHENTICATION_KEY "SYSTEM\\CurrentControlSet\\Control\\Lsa\\MSV1_0"
#define MSV1_0_SUBAUTHENTICATION_VALUE "Auth"

#define MSV1_0_CHALLENGE_LENGTH                8
#define MSV1_0_USER_SESSION_KEY_LENGTH         16
#define MSV1_0_LANMAN_SESSION_KEY_LENGTH       8

#define MSV1_0_CLEARTEXT_PASSWORD_ALLOWED      0x02
#define MSV1_0_UPDATE_LOGON_STATISTICS         0x04
#define MSV1_0_RETURN_USER_PARAMETERS          0x08
#define MSV1_0_DONT_TRY_GUEST_ACCOUNT          0x10
#define MSV1_0_ALLOW_SERVER_TRUST_ACCOUNT      0x20
#define MSV1_0_RETURN_PASSWORD_EXPIRY          0x40
#define MSV1_0_USE_CLIENT_CHALLENGE            0x80
#define MSV1_0_TRY_GUEST_ACCOUNT_ONLY          0x100
#define MSV1_0_RETURN_PROFILE_PATH             0x200
#define MSV1_0_TRY_SPECIFIED_DOMAIN_ONLY       0x400
#define MSV1_0_ALLOW_WORKSTATION_TRUST_ACCOUNT 0x800

#define MSV1_0_DISABLE_PERSONAL_FALLBACK     0x00001000
#define MSV1_0_ALLOW_FORCE_GUEST             0x00002000

#if (_WIN32_WINNT >= 0x0502)
#define MSV1_0_CLEARTEXT_PASSWORD_SUPPLIED   0x00004000
#define MSV1_0_USE_DOMAIN_FOR_ROUTING_ONLY   0x00008000
#endif

#define MSV1_0_SUBAUTHENTICATION_DLL_EX      0x00100000
#define MSV1_0_ALLOW_MSVCHAPV2               0x00010000

#if (_WIN32_WINNT >= 0x0600)
#define MSV1_0_S4U2SELF                      0x00020000
#define MSV1_0_CHECK_LOGONHOURS_FOR_S4U      0x00040000
#endif

#define MSV1_0_SUBAUTHENTICATION_DLL         0xFF000000
#define MSV1_0_SUBAUTHENTICATION_DLL_SHIFT   24
#define MSV1_0_MNS_LOGON                     0x01000000

#define MSV1_0_SUBAUTHENTICATION_DLL_RAS     2
#define MSV1_0_SUBAUTHENTICATION_DLL_IIS     132

#define LOGON_GUEST                 0x01
#define LOGON_NOENCRYPTION          0x02
#define LOGON_CACHED_ACCOUNT        0x04
#define LOGON_USED_LM_PASSWORD      0x08
#define LOGON_EXTRA_SIDS            0x20
#define LOGON_SUBAUTH_SESSION_KEY   0x40
#define LOGON_SERVER_TRUST_ACCOUNT  0x80
#define LOGON_NTLMV2_ENABLED        0x100
#define LOGON_RESOURCE_GROUPS       0x200
#define LOGON_PROFILE_PATH_RETURNED 0x400
#define LOGON_NT_V2                 0x800
#define LOGON_LM_V2                 0x1000
#define LOGON_NTLM_V2               0x2000

#if (_WIN32_WINNT >= 0x0600)

#define LOGON_OPTIMIZED             0x4000
#define LOGON_WINLOGON              0x8000
#define LOGON_PKINIT               0x10000
#define LOGON_NO_OPTIMIZED         0x20000

#endif

#define MSV1_0_SUBAUTHENTICATION_FLAGS 0xFF000000

#define LOGON_GRACE_LOGON              0x01000000

#define MSV1_0_OWF_PASSWORD_LENGTH 16
#define MSV1_0_CRED_LM_PRESENT 0x1
#define MSV1_0_CRED_NT_PRESENT 0x2
#define MSV1_0_CRED_VERSION 0

#define MSV1_0_NTLM3_RESPONSE_LENGTH 16
#define MSV1_0_NTLM3_OWF_LENGTH 16

#if (_WIN32_WINNT == 0x0500)
#define MSV1_0_MAX_NTLM3_LIFE 1800
#else
#define MSV1_0_MAX_NTLM3_LIFE 129600
#endif
#define MSV1_0_MAX_AVL_SIZE 64000

#if (_WIN32_WINNT >= 0x0501)

#define MSV1_0_AV_FLAG_FORCE_GUEST                  0x00000001

#if (_WIN32_WINNT >= 0x0600)
#define MSV1_0_AV_FLAG_MIC_HANDSHAKE_MESSAGES       0x00000002
#endif

#endif

#define MSV1_0_NTLM3_INPUT_LENGTH (sizeof(MSV1_0_NTLM3_RESPONSE) - MSV1_0_NTLM3_RESPONSE_LENGTH)

#if(_WIN32_WINNT >= 0x0502)
#define MSV1_0_NTLM3_MIN_NT_RESPONSE_LENGTH RTL_SIZEOF_THROUGH_FIELD(MSV1_0_NTLM3_RESPONSE, AvPairsOff)
#endif

#define USE_PRIMARY_PASSWORD            0x01
#define RETURN_PRIMARY_USERNAME         0x02
#define RETURN_PRIMARY_LOGON_DOMAINNAME 0x04
#define RETURN_NON_NT_USER_SESSION_KEY  0x08
#define GENERATE_CLIENT_CHALLENGE       0x10
#define GCR_NTLM3_PARMS                 0x20
#define GCR_TARGET_INFO                 0x40
#define RETURN_RESERVED_PARAMETER       0x80
#define GCR_ALLOW_NTLM                 0x100
#define GCR_USE_OEM_SET                0x200
#define GCR_MACHINE_CREDENTIAL         0x400
#define GCR_USE_OWF_PASSWORD           0x800
#define GCR_ALLOW_LM                  0x1000
#define GCR_ALLOW_NO_TARGET           0x2000

typedef enum _MSV1_0_LOGON_SUBMIT_TYPE {
  MsV1_0InteractiveLogon = 2,
  MsV1_0Lm20Logon,
  MsV1_0NetworkLogon,
  MsV1_0SubAuthLogon,
  MsV1_0WorkstationUnlockLogon = 7,
  MsV1_0S4ULogon = 12,
  MsV1_0VirtualLogon = 82
} MSV1_0_LOGON_SUBMIT_TYPE, *PMSV1_0_LOGON_SUBMIT_TYPE;

typedef enum _MSV1_0_PROFILE_BUFFER_TYPE {
  MsV1_0InteractiveProfile = 2,
  MsV1_0Lm20LogonProfile,
  MsV1_0SmartCardProfile
} MSV1_0_PROFILE_BUFFER_TYPE, *PMSV1_0_PROFILE_BUFFER_TYPE;

typedef struct _MSV1_0_INTERACTIVE_LOGON {
  MSV1_0_LOGON_SUBMIT_TYPE MessageType;
  UNICODE_STRING LogonDomainName;
  UNICODE_STRING UserName;
  UNICODE_STRING Password;
} MSV1_0_INTERACTIVE_LOGON, *PMSV1_0_INTERACTIVE_LOGON;

typedef struct _MSV1_0_INTERACTIVE_PROFILE {
  MSV1_0_PROFILE_BUFFER_TYPE MessageType;
  USHORT LogonCount;
  USHORT BadPasswordCount;
  LARGE_INTEGER LogonTime;
  LARGE_INTEGER LogoffTime;
  LARGE_INTEGER KickOffTime;
  LARGE_INTEGER PasswordLastSet;
  LARGE_INTEGER PasswordCanChange;
  LARGE_INTEGER PasswordMustChange;
  UNICODE_STRING LogonScript;
  UNICODE_STRING HomeDirectory;
  UNICODE_STRING FullName;
  UNICODE_STRING ProfilePath;
  UNICODE_STRING HomeDirectoryDrive;
  UNICODE_STRING LogonServer;
  ULONG UserFlags;
} MSV1_0_INTERACTIVE_PROFILE, *PMSV1_0_INTERACTIVE_PROFILE;

typedef struct _MSV1_0_LM20_LOGON {
  MSV1_0_LOGON_SUBMIT_TYPE MessageType;
  UNICODE_STRING LogonDomainName;
  UNICODE_STRING UserName;
  UNICODE_STRING Workstation;
  UCHAR ChallengeToClient[MSV1_0_CHALLENGE_LENGTH];
  STRING CaseSensitiveChallengeResponse;
  STRING CaseInsensitiveChallengeResponse;
  ULONG ParameterControl;
} MSV1_0_LM20_LOGON, * PMSV1_0_LM20_LOGON;

typedef struct _MSV1_0_SUBAUTH_LOGON {
  MSV1_0_LOGON_SUBMIT_TYPE MessageType;
  UNICODE_STRING LogonDomainName;
  UNICODE_STRING UserName;
  UNICODE_STRING Workstation;
  UCHAR ChallengeToClient[MSV1_0_CHALLENGE_LENGTH];
  STRING AuthenticationInfo1;
  STRING AuthenticationInfo2;
  ULONG ParameterControl;
  ULONG SubAuthPackageId;
} MSV1_0_SUBAUTH_LOGON, * PMSV1_0_SUBAUTH_LOGON;

#if (_WIN32_WINNT >= 0x0600)

#define MSV1_0_S4U_LOGON_FLAG_CHECK_LOGONHOURS 0x2

typedef struct _MSV1_0_S4U_LOGON {
  MSV1_0_LOGON_SUBMIT_TYPE MessageType;
  ULONG Flags;
  UNICODE_STRING UserPrincipalName;
  UNICODE_STRING DomainName;
} MSV1_0_S4U_LOGON, *PMSV1_0_S4U_LOGON;

#endif

typedef struct _MSV1_0_LM20_LOGON_PROFILE {
  MSV1_0_PROFILE_BUFFER_TYPE MessageType;
  LARGE_INTEGER KickOffTime;
  LARGE_INTEGER LogoffTime;
  ULONG UserFlags;
  UCHAR UserSessionKey[MSV1_0_USER_SESSION_KEY_LENGTH];
  UNICODE_STRING LogonDomainName;
  UCHAR LanmanSessionKey[MSV1_0_LANMAN_SESSION_KEY_LENGTH];
  UNICODE_STRING LogonServer;
  UNICODE_STRING UserParameters;
} MSV1_0_LM20_LOGON_PROFILE, * PMSV1_0_LM20_LOGON_PROFILE;

typedef struct _MSV1_0_SUPPLEMENTAL_CREDENTIAL {
  ULONG Version;
  ULONG Flags;
  UCHAR LmPassword[MSV1_0_OWF_PASSWORD_LENGTH];
  UCHAR NtPassword[MSV1_0_OWF_PASSWORD_LENGTH];
} MSV1_0_SUPPLEMENTAL_CREDENTIAL, *PMSV1_0_SUPPLEMENTAL_CREDENTIAL;

typedef struct _MSV1_0_NTLM3_RESPONSE {
  UCHAR Response[MSV1_0_NTLM3_RESPONSE_LENGTH];
  UCHAR RespType;
  UCHAR HiRespType;
  USHORT Flags;
  ULONG MsgWord;
  ULONGLONG TimeStamp;
  UCHAR ChallengeFromClient[MSV1_0_CHALLENGE_LENGTH];
  ULONG AvPairsOff;
  UCHAR Buffer[1];
} MSV1_0_NTLM3_RESPONSE, *PMSV1_0_NTLM3_RESPONSE;

typedef enum _MSV1_0_AVID {
  MsvAvEOL,
  MsvAvNbComputerName,
  MsvAvNbDomainName,
  MsvAvDnsComputerName,
  MsvAvDnsDomainName,
#if (_WIN32_WINNT >= 0x0501)
  MsvAvDnsTreeName,
  MsvAvFlags,
#if (_WIN32_WINNT >= 0x0600)
  MsvAvTimestamp,
  MsvAvRestrictions,
  MsvAvTargetName,
  MsvAvChannelBindings,
#endif
#endif
} MSV1_0_AVID;

typedef struct _MSV1_0_AV_PAIR {
  USHORT AvId;
  USHORT AvLen;
} MSV1_0_AV_PAIR, *PMSV1_0_AV_PAIR;

typedef enum _MSV1_0_PROTOCOL_MESSAGE_TYPE {
  MsV1_0Lm20ChallengeRequest = 0,
  MsV1_0Lm20GetChallengeResponse,
  MsV1_0EnumerateUsers,
  MsV1_0GetUserInfo,
  MsV1_0ReLogonUsers,
  MsV1_0ChangePassword,
  MsV1_0ChangeCachedPassword,
  MsV1_0GenericPassthrough,
  MsV1_0CacheLogon,
  MsV1_0SubAuth,
  MsV1_0DeriveCredential,
  MsV1_0CacheLookup,
#if (_WIN32_WINNT >= 0x0501)
  MsV1_0SetProcessOption,
#endif
#if (_WIN32_WINNT >= 0x0600)
  MsV1_0ConfigLocalAliases,
  MsV1_0ClearCachedCredentials,
#endif
} MSV1_0_PROTOCOL_MESSAGE_TYPE, *PMSV1_0_PROTOCOL_MESSAGE_TYPE;

typedef struct _MSV1_0_LM20_CHALLENGE_REQUEST {
  MSV1_0_PROTOCOL_MESSAGE_TYPE MessageType;
} MSV1_0_LM20_CHALLENGE_REQUEST, *PMSV1_0_LM20_CHALLENGE_REQUEST;

typedef struct _MSV1_0_LM20_CHALLENGE_RESPONSE {
  MSV1_0_PROTOCOL_MESSAGE_TYPE MessageType;
  UCHAR ChallengeToClient[MSV1_0_CHALLENGE_LENGTH];
} MSV1_0_LM20_CHALLENGE_RESPONSE, *PMSV1_0_LM20_CHALLENGE_RESPONSE;

typedef struct _MSV1_0_GETCHALLENRESP_REQUEST_V1 {
  MSV1_0_PROTOCOL_MESSAGE_TYPE MessageType;
  ULONG ParameterControl;
  LUID LogonId;
  UNICODE_STRING Password;
  UCHAR ChallengeToClient[MSV1_0_CHALLENGE_LENGTH];
} MSV1_0_GETCHALLENRESP_REQUEST_V1, *PMSV1_0_GETCHALLENRESP_REQUEST_V1;

typedef struct _MSV1_0_GETCHALLENRESP_REQUEST {
  MSV1_0_PROTOCOL_MESSAGE_TYPE MessageType;
  ULONG ParameterControl;
  LUID LogonId;
  UNICODE_STRING Password;
  UCHAR ChallengeToClient[MSV1_0_CHALLENGE_LENGTH];
  UNICODE_STRING UserName;
  UNICODE_STRING LogonDomainName;
  UNICODE_STRING ServerName;
} MSV1_0_GETCHALLENRESP_REQUEST, *PMSV1_0_GETCHALLENRESP_REQUEST;

typedef struct _MSV1_0_GETCHALLENRESP_RESPONSE {
  MSV1_0_PROTOCOL_MESSAGE_TYPE MessageType;
  STRING CaseSensitiveChallengeResponse;
  STRING CaseInsensitiveChallengeResponse;
  UNICODE_STRING UserName;
  UNICODE_STRING LogonDomainName;
  UCHAR UserSessionKey[MSV1_0_USER_SESSION_KEY_LENGTH];
  UCHAR LanmanSessionKey[MSV1_0_LANMAN_SESSION_KEY_LENGTH];
} MSV1_0_GETCHALLENRESP_RESPONSE, *PMSV1_0_GETCHALLENRESP_RESPONSE;

typedef struct _MSV1_0_ENUMUSERS_REQUEST {
  MSV1_0_PROTOCOL_MESSAGE_TYPE MessageType;
} MSV1_0_ENUMUSERS_REQUEST, *PMSV1_0_ENUMUSERS_REQUEST;

typedef struct _MSV1_0_ENUMUSERS_RESPONSE {
  MSV1_0_PROTOCOL_MESSAGE_TYPE MessageType;
  ULONG NumberOfLoggedOnUsers;
  PLUID LogonIds;
  PULONG EnumHandles;
} MSV1_0_ENUMUSERS_RESPONSE, *PMSV1_0_ENUMUSERS_RESPONSE;

typedef struct _MSV1_0_GETUSERINFO_REQUEST {
  MSV1_0_PROTOCOL_MESSAGE_TYPE MessageType;
  LUID LogonId;
} MSV1_0_GETUSERINFO_REQUEST, *PMSV1_0_GETUSERINFO_REQUEST;

typedef struct _MSV1_0_GETUSERINFO_RESPONSE {
  MSV1_0_PROTOCOL_MESSAGE_TYPE MessageType;
  PSID UserSid;
  UNICODE_STRING UserName;
  UNICODE_STRING LogonDomainName;
  UNICODE_STRING LogonServer;
  SECURITY_LOGON_TYPE LogonType;
} MSV1_0_GETUSERINFO_RESPONSE, *PMSV1_0_GETUSERINFO_RESPONSE;

$include (iotypes.h)

typedef struct _PUBLIC_OBJECT_BASIC_INFORMATION {
  ULONG Attributes;
  ACCESS_MASK GrantedAccess;
  ULONG HandleCount;
  ULONG PointerCount;
  ULONG Reserved[10];
} PUBLIC_OBJECT_BASIC_INFORMATION, *PPUBLIC_OBJECT_BASIC_INFORMATION;

typedef struct _PUBLIC_OBJECT_TYPE_INFORMATION {
  UNICODE_STRING TypeName;
  ULONG Reserved [22];
} PUBLIC_OBJECT_TYPE_INFORMATION, *PPUBLIC_OBJECT_TYPE_INFORMATION;

typedef struct _SECURITY_CLIENT_CONTEXT {
  SECURITY_QUALITY_OF_SERVICE SecurityQos;
  PACCESS_TOKEN ClientToken;
  BOOLEAN DirectlyAccessClientToken;
  BOOLEAN DirectAccessEffectiveOnly;
  BOOLEAN ServerIsRemote;
  TOKEN_CONTROL ClientTokenControl;
} SECURITY_CLIENT_CONTEXT, *PSECURITY_CLIENT_CONTEXT;

#define SYSTEM_PAGE_PRIORITY_BITS       3
#define SYSTEM_PAGE_PRIORITY_LEVELS     (1 << SYSTEM_PAGE_PRIORITY_BITS)

typedef struct _KAPC_STATE {
  LIST_ENTRY ApcListHead[MaximumMode];
  PKPROCESS Process;
  BOOLEAN KernelApcInProgress;
  BOOLEAN KernelApcPending;
  BOOLEAN UserApcPending;
} KAPC_STATE, *PKAPC_STATE, *RESTRICTED_POINTER PRKAPC_STATE;

#define KAPC_STATE_ACTUAL_LENGTH (FIELD_OFFSET(KAPC_STATE, UserApcPending) + sizeof(BOOLEAN))

#define ASSERT_QUEUE(Q) ASSERT(((Q)->Header.Type & KOBJECT_TYPE_MASK) == QueueObject);

typedef struct _KQUEUE {
  DISPATCHER_HEADER Header;
  LIST_ENTRY EntryListHead;
  volatile ULONG CurrentCount;
  ULONG MaximumCount;
  LIST_ENTRY ThreadListHead;
} KQUEUE, *PKQUEUE, *RESTRICTED_POINTER PRKQUEUE;

$include (kefuncs.h)
$include (extypes.h)
$include (exfuncs.h)
$include (sefuncs.h)
$include (psfuncs.h)
$include (iofuncs.h)
$include (potypes.h)
$include (pofuncs.h)
$include (mmtypes.h)
$include (mmfuncs.h)
$include (obfuncs.h)
$include (fsrtltypes.h)
$include (fsrtlfuncs.h)
$include (cctypes.h)
$include (ccfuncs.h)
$include (zwfuncs.h)
$include (sspi.h)

/* #if !defined(_X86AMD64_)  FIXME : WHAT ?! */
#if defined(_WIN64)
C_ASSERT(sizeof(ERESOURCE) == 0x68);
C_ASSERT(FIELD_OFFSET(ERESOURCE,ActiveCount) == 0x18);
C_ASSERT(FIELD_OFFSET(ERESOURCE,Flag) == 0x1a);
#else
C_ASSERT(sizeof(ERESOURCE) == 0x38);
C_ASSERT(FIELD_OFFSET(ERESOURCE,ActiveCount) == 0x0c);
C_ASSERT(FIELD_OFFSET(ERESOURCE,Flag) == 0x0e);
#endif
/* #endif */

#if defined(_IA64_)
#if (NTDDI_VERSION >= NTDDI_WIN2K)
//DECLSPEC_DEPRECATED_DDK
NTHALAPI
ULONG
NTAPI
HalGetDmaAlignmentRequirement(
  VOID);
#endif
#endif

#if defined(_M_IX86) || defined(_M_AMD64)
#define HalGetDmaAlignmentRequirement() 1L
#endif

extern NTKERNELAPI PUSHORT NlsOemLeadByteInfo;
#define NLS_OEM_LEAD_BYTE_INFO            NlsOemLeadByteInfo

#ifdef NLS_MB_CODE_PAGE_TAG
#undef NLS_MB_CODE_PAGE_TAG
#endif
#define NLS_MB_CODE_PAGE_TAG              NlsMbOemCodePageTag

#if (NTDDI_VERSION >= NTDDI_VISTA)

typedef enum _NETWORK_OPEN_LOCATION_QUALIFIER {
  NetworkOpenLocationAny,
  NetworkOpenLocationRemote,
  NetworkOpenLocationLoopback
} NETWORK_OPEN_LOCATION_QUALIFIER;

typedef enum _NETWORK_OPEN_INTEGRITY_QUALIFIER {
  NetworkOpenIntegrityAny,
  NetworkOpenIntegrityNone,
  NetworkOpenIntegritySigned,
  NetworkOpenIntegrityEncrypted,
  NetworkOpenIntegrityMaximum
} NETWORK_OPEN_INTEGRITY_QUALIFIER;

#if (NTDDI_VERSION >= NTDDI_WIN7)

#define NETWORK_OPEN_ECP_IN_FLAG_DISABLE_HANDLE_COLLAPSING 0x1
#define NETWORK_OPEN_ECP_IN_FLAG_DISABLE_HANDLE_DURABILITY 0x2
#define NETWORK_OPEN_ECP_IN_FLAG_FORCE_BUFFERED_SYNCHRONOUS_IO_HACK 0x80000000

typedef struct _NETWORK_OPEN_ECP_CONTEXT {
  USHORT Size;
  USHORT Reserved;
  _ANONYMOUS_STRUCT struct {
    struct {
      NETWORK_OPEN_LOCATION_QUALIFIER Location;
      NETWORK_OPEN_INTEGRITY_QUALIFIER Integrity;
      ULONG Flags;
    } in;
    struct {
      NETWORK_OPEN_LOCATION_QUALIFIER Location;
      NETWORK_OPEN_INTEGRITY_QUALIFIER Integrity;
      ULONG Flags;
    } out;
  } DUMMYSTRUCTNAME;
} NETWORK_OPEN_ECP_CONTEXT, *PNETWORK_OPEN_ECP_CONTEXT;

typedef struct _NETWORK_OPEN_ECP_CONTEXT_V0 {
  USHORT Size;
  USHORT Reserved;
  _ANONYMOUS_STRUCT struct {
    struct {
    NETWORK_OPEN_LOCATION_QUALIFIER Location;
    NETWORK_OPEN_INTEGRITY_QUALIFIER Integrity;
    } in;
    struct {
      NETWORK_OPEN_LOCATION_QUALIFIER Location;
      NETWORK_OPEN_INTEGRITY_QUALIFIER Integrity;
    } out;
  } DUMMYSTRUCTNAME;
} NETWORK_OPEN_ECP_CONTEXT_V0, *PNETWORK_OPEN_ECP_CONTEXT_V0;

#elif (NTDDI_VERSION >= NTDDI_VISTA)
typedef struct _NETWORK_OPEN_ECP_CONTEXT {
  USHORT Size;
  USHORT Reserved;
  _ANONYMOUS_STRUCT struct {
    struct {
      NETWORK_OPEN_LOCATION_QUALIFIER Location;
      NETWORK_OPEN_INTEGRITY_QUALIFIER Integrity;
    } in;
    struct {
      NETWORK_OPEN_LOCATION_QUALIFIER Location;
      NETWORK_OPEN_INTEGRITY_QUALIFIER Integrity;
    } out;
  } DUMMYSTRUCTNAME;
} NETWORK_OPEN_ECP_CONTEXT, *PNETWORK_OPEN_ECP_CONTEXT;
#endif

DEFINE_GUID(GUID_ECP_NETWORK_OPEN_CONTEXT, 0xc584edbf, 0x00df, 0x4d28, 0xb8, 0x84, 0x35, 0xba, 0xca, 0x89, 0x11, 0xe8);

#endif /* (NTDDI_VERSION >= NTDDI_VISTA) */


#if (NTDDI_VERSION >= NTDDI_VISTA)

typedef struct _PREFETCH_OPEN_ECP_CONTEXT {
  PVOID Context;
} PREFETCH_OPEN_ECP_CONTEXT, *PPREFETCH_OPEN_ECP_CONTEXT;

DEFINE_GUID(GUID_ECP_PREFETCH_OPEN, 0xe1777b21, 0x847e, 0x4837, 0xaa, 0x45, 0x64, 0x16, 0x1d, 0x28, 0x6, 0x55);

#endif /* (NTDDI_VERSION >= NTDDI_VISTA) */

#if (NTDDI_VERSION >= NTDDI_WIN7)

DEFINE_GUID (GUID_ECP_NFS_OPEN, 0xf326d30c, 0xe5f8, 0x4fe7, 0xab, 0x74, 0xf5, 0xa3, 0x19, 0x6d, 0x92, 0xdb);
DEFINE_GUID (GUID_ECP_SRV_OPEN, 0xbebfaebc, 0xaabf, 0x489d, 0x9d, 0x2c, 0xe9, 0xe3, 0x61, 0x10, 0x28, 0x53);

typedef struct sockaddr_storage *PSOCKADDR_STORAGE_NFS;

typedef struct _NFS_OPEN_ECP_CONTEXT {
  PUNICODE_STRING ExportAlias;
  PSOCKADDR_STORAGE_NFS ClientSocketAddress;
} NFS_OPEN_ECP_CONTEXT, *PNFS_OPEN_ECP_CONTEXT, **PPNFS_OPEN_ECP_CONTEXT;

typedef struct _SRV_OPEN_ECP_CONTEXT {
  PUNICODE_STRING ShareName;
  PSOCKADDR_STORAGE_NFS SocketAddress;
  BOOLEAN OplockBlockState;
  BOOLEAN OplockAppState;
  BOOLEAN OplockFinalState;
} SRV_OPEN_ECP_CONTEXT, *PSRV_OPEN_ECP_CONTEXT;

#endif /* (NTDDI_VERSION >= NTDDI_WIN7) */

#define PIN_WAIT                        (1)
#define PIN_EXCLUSIVE                   (2)
#define PIN_NO_READ                     (4)
#define PIN_IF_BCB                      (8)
#define PIN_CALLER_TRACKS_DIRTY_DATA    (32)
#define PIN_HIGH_PRIORITY               (64)

#define MAP_WAIT                        1
#define MAP_NO_READ                     (16)
#define MAP_HIGH_PRIORITY               (64)

#define IOCTL_REDIR_QUERY_PATH          CTL_CODE(FILE_DEVICE_NETWORK_FILE_SYSTEM, 99, METHOD_NEITHER, FILE_ANY_ACCESS)
#define IOCTL_REDIR_QUERY_PATH_EX       CTL_CODE(FILE_DEVICE_NETWORK_FILE_SYSTEM, 100, METHOD_NEITHER, FILE_ANY_ACCESS)

typedef struct _QUERY_PATH_REQUEST {
  ULONG PathNameLength;
  PIO_SECURITY_CONTEXT SecurityContext;
  WCHAR FilePathName[1];
} QUERY_PATH_REQUEST, *PQUERY_PATH_REQUEST;

typedef struct _QUERY_PATH_REQUEST_EX {
  PIO_SECURITY_CONTEXT pSecurityContext;
  ULONG EaLength;
  PVOID pEaBuffer;
  UNICODE_STRING PathName;
  UNICODE_STRING DomainServiceName;
  ULONG_PTR Reserved[ 3 ];
} QUERY_PATH_REQUEST_EX, *PQUERY_PATH_REQUEST_EX;

typedef struct _QUERY_PATH_RESPONSE {
  ULONG LengthAccepted;
} QUERY_PATH_RESPONSE, *PQUERY_PATH_RESPONSE;

#define VOLSNAPCONTROLTYPE                              0x00000053
#define IOCTL_VOLSNAP_FLUSH_AND_HOLD_WRITES             CTL_CODE(VOLSNAPCONTROLTYPE, 0, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)

/* FIXME : These definitions below don't belong here (or anywhere in ddk really) */
#pragma pack(push,4)

#ifndef VER_PRODUCTBUILD
#define VER_PRODUCTBUILD 10000
#endif

#include "csq.h"

#define FS_LFN_APIS                             0x00004000

#define FILE_STORAGE_TYPE_SPECIFIED             0x00000041  /* FILE_DIRECTORY_FILE | FILE_NON_DIRECTORY_FILE */
#define FILE_STORAGE_TYPE_DEFAULT               (StorageTypeDefault << FILE_STORAGE_TYPE_SHIFT)
#define FILE_STORAGE_TYPE_DIRECTORY             (StorageTypeDirectory << FILE_STORAGE_TYPE_SHIFT)
#define FILE_STORAGE_TYPE_FILE                  (StorageTypeFile << FILE_STORAGE_TYPE_SHIFT)
#define FILE_STORAGE_TYPE_DOCFILE               (StorageTypeDocfile << FILE_STORAGE_TYPE_SHIFT)
#define FILE_STORAGE_TYPE_JUNCTION_POINT        (StorageTypeJunctionPoint << FILE_STORAGE_TYPE_SHIFT)
#define FILE_STORAGE_TYPE_CATALOG               (StorageTypeCatalog << FILE_STORAGE_TYPE_SHIFT)
#define FILE_STORAGE_TYPE_STRUCTURED_STORAGE    (StorageTypeStructuredStorage << FILE_STORAGE_TYPE_SHIFT)
#define FILE_STORAGE_TYPE_EMBEDDING             (StorageTypeEmbedding << FILE_STORAGE_TYPE_SHIFT)
#define FILE_STORAGE_TYPE_STREAM                (StorageTypeStream << FILE_STORAGE_TYPE_SHIFT)
#define FILE_MINIMUM_STORAGE_TYPE               FILE_STORAGE_TYPE_DEFAULT
#define FILE_MAXIMUM_STORAGE_TYPE               FILE_STORAGE_TYPE_STREAM
#define FILE_STORAGE_TYPE_MASK                  0x000f0000
#define FILE_STORAGE_TYPE_SHIFT                 16

#define FILE_VC_QUOTAS_LOG_VIOLATIONS           0x00000004

#ifdef _X86_
#define HARDWARE_PTE    HARDWARE_PTE_X86
#define PHARDWARE_PTE   PHARDWARE_PTE_X86
#endif

#define IO_ATTACH_DEVICE_API            0x80000000

#define IO_TYPE_APC                     18
#define IO_TYPE_DPC                     19
#define IO_TYPE_DEVICE_QUEUE            20
#define IO_TYPE_EVENT_PAIR              21
#define IO_TYPE_INTERRUPT               22
#define IO_TYPE_PROFILE                 23

#define IRP_BEING_VERIFIED              0x10

#define MAILSLOT_CLASS_FIRSTCLASS       1
#define MAILSLOT_CLASS_SECONDCLASS      2

#define MAILSLOT_SIZE_AUTO              0

#define MEM_DOS_LIM                     0x40000000

#define OB_TYPE_TYPE                    1
#define OB_TYPE_DIRECTORY               2
#define OB_TYPE_SYMBOLIC_LINK           3
#define OB_TYPE_TOKEN                   4
#define OB_TYPE_PROCESS                 5
#define OB_TYPE_THREAD                  6
#define OB_TYPE_EVENT                   7
#define OB_TYPE_EVENT_PAIR              8
#define OB_TYPE_MUTANT                  9
#define OB_TYPE_SEMAPHORE               10
#define OB_TYPE_TIMER                   11
#define OB_TYPE_PROFILE                 12
#define OB_TYPE_WINDOW_STATION          13
#define OB_TYPE_DESKTOP                 14
#define OB_TYPE_SECTION                 15
#define OB_TYPE_KEY                     16
#define OB_TYPE_PORT                    17
#define OB_TYPE_ADAPTER                 18
#define OB_TYPE_CONTROLLER              19
#define OB_TYPE_DEVICE                  20
#define OB_TYPE_DRIVER                  21
#define OB_TYPE_IO_COMPLETION           22
#define OB_TYPE_FILE                    23

#define SEC_BASED 0x00200000

/* end winnt.h */

#define TOKEN_HAS_ADMIN_GROUP           0x08

#if (VER_PRODUCTBUILD >= 1381)
#define FSCTL_GET_HFS_INFORMATION       CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 31, METHOD_BUFFERED, FILE_ANY_ACCESS)
#endif /* (VER_PRODUCTBUILD >= 1381) */

#if (VER_PRODUCTBUILD >= 2195)

#define FSCTL_READ_PROPERTY_DATA        CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 33, METHOD_NEITHER, FILE_ANY_ACCESS)
#define FSCTL_WRITE_PROPERTY_DATA       CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 34, METHOD_NEITHER, FILE_ANY_ACCESS)

#define FSCTL_DUMP_PROPERTY_DATA        CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 37,  METHOD_NEITHER, FILE_ANY_ACCESS)

#define FSCTL_HSM_MSG                   CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 66, METHOD_BUFFERED, FILE_READ_DATA | FILE_WRITE_DATA)
#define FSCTL_NSS_CONTROL               CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 67, METHOD_BUFFERED, FILE_WRITE_DATA)
#define FSCTL_HSM_DATA                  CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 68, METHOD_NEITHER, FILE_READ_DATA | FILE_WRITE_DATA)
#define FSCTL_NSS_RCONTROL              CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 70, METHOD_BUFFERED, FILE_READ_DATA)
#endif /* (VER_PRODUCTBUILD >= 2195) */

#define FSCTL_NETWORK_SET_CONFIGURATION_INFO    CTL_CODE(FILE_DEVICE_NETWORK_FILE_SYSTEM, 102, METHOD_IN_DIRECT, FILE_ANY_ACCESS)
#define FSCTL_NETWORK_GET_CONFIGURATION_INFO    CTL_CODE(FILE_DEVICE_NETWORK_FILE_SYSTEM, 103, METHOD_OUT_DIRECT, FILE_ANY_ACCESS)
#define FSCTL_NETWORK_GET_CONNECTION_INFO       CTL_CODE(FILE_DEVICE_NETWORK_FILE_SYSTEM, 104, METHOD_NEITHER, FILE_ANY_ACCESS)
#define FSCTL_NETWORK_ENUMERATE_CONNECTIONS     CTL_CODE(FILE_DEVICE_NETWORK_FILE_SYSTEM, 105, METHOD_NEITHER, FILE_ANY_ACCESS)
#define FSCTL_NETWORK_DELETE_CONNECTION         CTL_CODE(FILE_DEVICE_NETWORK_FILE_SYSTEM, 107, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_NETWORK_GET_STATISTICS            CTL_CODE(FILE_DEVICE_NETWORK_FILE_SYSTEM, 116, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_NETWORK_SET_DOMAIN_NAME           CTL_CODE(FILE_DEVICE_NETWORK_FILE_SYSTEM, 120, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_NETWORK_REMOTE_BOOT_INIT_SCRT     CTL_CODE(FILE_DEVICE_NETWORK_FILE_SYSTEM, 250, METHOD_BUFFERED, FILE_ANY_ACCESS)

typedef enum _FILE_STORAGE_TYPE {
    StorageTypeDefault = 1,
    StorageTypeDirectory,
    StorageTypeFile,
    StorageTypeJunctionPoint,
    StorageTypeCatalog,
    StorageTypeStructuredStorage,
    StorageTypeEmbedding,
    StorageTypeStream
} FILE_STORAGE_TYPE;

typedef struct _OBJECT_BASIC_INFORMATION
{
    ULONG Attributes;
    ACCESS_MASK GrantedAccess;
    ULONG HandleCount;
    ULONG PointerCount;
    ULONG PagedPoolCharge;
    ULONG NonPagedPoolCharge;
    ULONG Reserved[ 3 ];
    ULONG NameInfoSize;
    ULONG TypeInfoSize;
    ULONG SecurityDescriptorSize;
    LARGE_INTEGER CreationTime;
} OBJECT_BASIC_INFORMATION, *POBJECT_BASIC_INFORMATION;

typedef struct _BITMAP_RANGE {
    LIST_ENTRY      Links;
    LONGLONG        BasePage;
    ULONG           FirstDirtyPage;
    ULONG           LastDirtyPage;
    ULONG           DirtyPages;
    PULONG          Bitmap;
} BITMAP_RANGE, *PBITMAP_RANGE;

typedef struct _FILE_COPY_ON_WRITE_INFORMATION {
    BOOLEAN ReplaceIfExists;
    HANDLE  RootDirectory;
    ULONG   FileNameLength;
    WCHAR   FileName[1];
} FILE_COPY_ON_WRITE_INFORMATION, *PFILE_COPY_ON_WRITE_INFORMATION;

typedef struct _FILE_FULL_DIRECTORY_INFORMATION {
    ULONG           NextEntryOffset;
    ULONG           FileIndex;
    LARGE_INTEGER   CreationTime;
    LARGE_INTEGER   LastAccessTime;
    LARGE_INTEGER   LastWriteTime;
    LARGE_INTEGER   ChangeTime;
    LARGE_INTEGER   EndOfFile;
    LARGE_INTEGER   AllocationSize;
    ULONG           FileAttributes;
    ULONG           FileNameLength;
    ULONG           EaSize;
    WCHAR           FileName[ANYSIZE_ARRAY];
} FILE_FULL_DIRECTORY_INFORMATION, *PFILE_FULL_DIRECTORY_INFORMATION;

/* raw internal file lock struct returned from FsRtlGetNextFileLock */
typedef struct _FILE_SHARED_LOCK_ENTRY {
    PVOID           Unknown1;
    PVOID           Unknown2;
    FILE_LOCK_INFO  FileLock;
} FILE_SHARED_LOCK_ENTRY, *PFILE_SHARED_LOCK_ENTRY;

/* raw internal file lock struct returned from FsRtlGetNextFileLock */
typedef struct _FILE_EXCLUSIVE_LOCK_ENTRY {
    LIST_ENTRY      ListEntry;
    PVOID           Unknown1;
    PVOID           Unknown2;
    FILE_LOCK_INFO  FileLock;
} FILE_EXCLUSIVE_LOCK_ENTRY, *PFILE_EXCLUSIVE_LOCK_ENTRY;

typedef struct _FILE_MAILSLOT_PEEK_BUFFER {
    ULONG ReadDataAvailable;
    ULONG NumberOfMessages;
    ULONG MessageLength;
} FILE_MAILSLOT_PEEK_BUFFER, *PFILE_MAILSLOT_PEEK_BUFFER;

typedef struct _FILE_OLE_CLASSID_INFORMATION {
    GUID ClassId;
} FILE_OLE_CLASSID_INFORMATION, *PFILE_OLE_CLASSID_INFORMATION;

typedef struct _FILE_OLE_ALL_INFORMATION {
    FILE_BASIC_INFORMATION          BasicInformation;
    FILE_STANDARD_INFORMATION       StandardInformation;
    FILE_INTERNAL_INFORMATION       InternalInformation;
    FILE_EA_INFORMATION             EaInformation;
    FILE_ACCESS_INFORMATION         AccessInformation;
    FILE_POSITION_INFORMATION       PositionInformation;
    FILE_MODE_INFORMATION           ModeInformation;
    FILE_ALIGNMENT_INFORMATION      AlignmentInformation;
    USN                             LastChangeUsn;
    USN                             ReplicationUsn;
    LARGE_INTEGER                   SecurityChangeTime;
    FILE_OLE_CLASSID_INFORMATION    OleClassIdInformation;
    FILE_OBJECTID_INFORMATION       ObjectIdInformation;
    FILE_STORAGE_TYPE               StorageType;
    ULONG                           OleStateBits;
    ULONG                           OleId;
    ULONG                           NumberOfStreamReferences;
    ULONG                           StreamIndex;
    ULONG                           SecurityId;
    BOOLEAN                         ContentIndexDisable;
    BOOLEAN                         InheritContentIndexDisable;
    FILE_NAME_INFORMATION           NameInformation;
} FILE_OLE_ALL_INFORMATION, *PFILE_OLE_ALL_INFORMATION;

typedef struct _FILE_OLE_DIR_INFORMATION {
    ULONG               NextEntryOffset;
    ULONG               FileIndex;
    LARGE_INTEGER       CreationTime;
    LARGE_INTEGER       LastAccessTime;
    LARGE_INTEGER       LastWriteTime;
    LARGE_INTEGER       ChangeTime;
    LARGE_INTEGER       EndOfFile;
    LARGE_INTEGER       AllocationSize;
    ULONG               FileAttributes;
    ULONG               FileNameLength;
    FILE_STORAGE_TYPE   StorageType;
    GUID                OleClassId;
    ULONG               OleStateBits;
    BOOLEAN             ContentIndexDisable;
    BOOLEAN             InheritContentIndexDisable;
    WCHAR               FileName[1];
} FILE_OLE_DIR_INFORMATION, *PFILE_OLE_DIR_INFORMATION;

typedef struct _FILE_OLE_INFORMATION {
    LARGE_INTEGER                   SecurityChangeTime;
    FILE_OLE_CLASSID_INFORMATION    OleClassIdInformation;
    FILE_OBJECTID_INFORMATION       ObjectIdInformation;
    FILE_STORAGE_TYPE               StorageType;
    ULONG                           OleStateBits;
    BOOLEAN                         ContentIndexDisable;
    BOOLEAN                         InheritContentIndexDisable;
} FILE_OLE_INFORMATION, *PFILE_OLE_INFORMATION;

typedef struct _FILE_OLE_STATE_BITS_INFORMATION {
    ULONG StateBits;
    ULONG StateBitsMask;
} FILE_OLE_STATE_BITS_INFORMATION, *PFILE_OLE_STATE_BITS_INFORMATION;

typedef struct _MAPPING_PAIR {
    ULONGLONG Vcn;
    ULONGLONG Lcn;
} MAPPING_PAIR, *PMAPPING_PAIR;

typedef struct _GET_RETRIEVAL_DESCRIPTOR {
    ULONG           NumberOfPairs;
    ULONGLONG       StartVcn;
    MAPPING_PAIR    Pair[1];
} GET_RETRIEVAL_DESCRIPTOR, *PGET_RETRIEVAL_DESCRIPTOR;

typedef struct _MBCB {
    CSHORT          NodeTypeCode;
    CSHORT          NodeIsInZone;
    ULONG           PagesToWrite;
    ULONG           DirtyPages;
    ULONG           Reserved;
    LIST_ENTRY      BitmapRanges;
    LONGLONG        ResumeWritePage;
    BITMAP_RANGE    BitmapRange1;
    BITMAP_RANGE    BitmapRange2;
    BITMAP_RANGE    BitmapRange3;
} MBCB, *PMBCB;

typedef struct _MOVEFILE_DESCRIPTOR {
     HANDLE         FileHandle;
     ULONG          Reserved;
     LARGE_INTEGER  StartVcn;
     LARGE_INTEGER  TargetLcn;
     ULONG          NumVcns;
     ULONG          Reserved1;
} MOVEFILE_DESCRIPTOR, *PMOVEFILE_DESCRIPTOR;

typedef struct _OBJECT_BASIC_INFO {
    ULONG           Attributes;
    ACCESS_MASK     GrantedAccess;
    ULONG           HandleCount;
    ULONG           ReferenceCount;
    ULONG           PagedPoolUsage;
    ULONG           NonPagedPoolUsage;
    ULONG           Reserved[3];
    ULONG           NameInformationLength;
    ULONG           TypeInformationLength;
    ULONG           SecurityDescriptorLength;
    LARGE_INTEGER   CreateTime;
} OBJECT_BASIC_INFO, *POBJECT_BASIC_INFO;

typedef struct _OBJECT_HANDLE_ATTRIBUTE_INFO {
    BOOLEAN Inherit;
    BOOLEAN ProtectFromClose;
} OBJECT_HANDLE_ATTRIBUTE_INFO, *POBJECT_HANDLE_ATTRIBUTE_INFO;

typedef struct _OBJECT_NAME_INFO {
    UNICODE_STRING  ObjectName;
    WCHAR           ObjectNameBuffer[1];
} OBJECT_NAME_INFO, *POBJECT_NAME_INFO;

typedef struct _OBJECT_PROTECTION_INFO {
    BOOLEAN Inherit;
    BOOLEAN ProtectHandle;
} OBJECT_PROTECTION_INFO, *POBJECT_PROTECTION_INFO;

typedef struct _OBJECT_TYPE_INFO {
    UNICODE_STRING  ObjectTypeName;
    UCHAR           Unknown[0x58];
    WCHAR           ObjectTypeNameBuffer[1];
} OBJECT_TYPE_INFO, *POBJECT_TYPE_INFO;

typedef struct _OBJECT_ALL_TYPES_INFO {
    ULONG               NumberOfObjectTypes;
    OBJECT_TYPE_INFO    ObjectsTypeInfo[1];
} OBJECT_ALL_TYPES_INFO, *POBJECT_ALL_TYPES_INFO;

#if defined(USE_LPC6432)
#define LPC_CLIENT_ID CLIENT_ID64
#define LPC_SIZE_T ULONGLONG
#define LPC_PVOID ULONGLONG
#define LPC_HANDLE ULONGLONG
#else
#define LPC_CLIENT_ID CLIENT_ID
#define LPC_SIZE_T SIZE_T
#define LPC_PVOID PVOID
#define LPC_HANDLE HANDLE
#endif

typedef struct _PORT_MESSAGE
{
    union
    {
        struct
        {
            CSHORT DataLength;
            CSHORT TotalLength;
        } s1;
        ULONG Length;
    } u1;
    union
    {
        struct
        {
            CSHORT Type;
            CSHORT DataInfoOffset;
        } s2;
        ULONG ZeroInit;
    } u2;
    __GNU_EXTENSION union
    {
        LPC_CLIENT_ID ClientId;
        double DoNotUseThisField;
    };
    ULONG MessageId;
    __GNU_EXTENSION union
    {
        LPC_SIZE_T ClientViewSize;
        ULONG CallbackId;
    };
} PORT_MESSAGE, *PPORT_MESSAGE;

#define LPC_KERNELMODE_MESSAGE      (CSHORT)((USHORT)0x8000)

typedef struct _PORT_VIEW
{
    ULONG Length;
    LPC_HANDLE SectionHandle;
    ULONG SectionOffset;
    LPC_SIZE_T ViewSize;
    LPC_PVOID ViewBase;
    LPC_PVOID ViewRemoteBase;
} PORT_VIEW, *PPORT_VIEW;

typedef struct _REMOTE_PORT_VIEW
{
    ULONG Length;
    LPC_SIZE_T ViewSize;
    LPC_PVOID ViewBase;
} REMOTE_PORT_VIEW, *PREMOTE_PORT_VIEW;

typedef struct _VAD_HEADER {
    PVOID       StartVPN;
    PVOID       EndVPN;
    struct _VAD_HEADER* ParentLink;
    struct _VAD_HEADER* LeftLink;
    struct _VAD_HEADER* RightLink;
    ULONG       Flags;          /* LSB = CommitCharge */
    PVOID       ControlArea;
    PVOID       FirstProtoPte;
    PVOID       LastPTE;
    ULONG       Unknown;
    LIST_ENTRY  Secured;
} VAD_HEADER, *PVAD_HEADER;

NTKERNELAPI
LARGE_INTEGER
NTAPI
CcGetLsnForFileObject (
    IN PFILE_OBJECT     FileObject,
    OUT PLARGE_INTEGER  OldestLsn OPTIONAL
);

NTKERNELAPI
PVOID
NTAPI
FsRtlAllocatePool (
    IN POOL_TYPE    PoolType,
    IN ULONG        NumberOfBytes
);

NTKERNELAPI
PVOID
NTAPI
FsRtlAllocatePoolWithQuota (
    IN POOL_TYPE    PoolType,
    IN ULONG        NumberOfBytes
);

NTKERNELAPI
PVOID
NTAPI
FsRtlAllocatePoolWithQuotaTag (
    IN POOL_TYPE    PoolType,
    IN ULONG        NumberOfBytes,
    IN ULONG        Tag
);

NTKERNELAPI
PVOID
NTAPI
FsRtlAllocatePoolWithTag (
    IN POOL_TYPE    PoolType,
    IN ULONG        NumberOfBytes,
    IN ULONG        Tag
);

NTKERNELAPI
BOOLEAN
NTAPI
FsRtlIsFatDbcsLegal (
    IN ANSI_STRING  DbcsName,
    IN BOOLEAN      WildCardsPermissible,
    IN BOOLEAN      PathNamePermissible,
    IN BOOLEAN      LeadingBackslashPermissible
);

NTKERNELAPI
BOOLEAN
NTAPI
FsRtlMdlReadComplete (
    IN PFILE_OBJECT     FileObject,
    IN PMDL             MdlChain
);

NTKERNELAPI
BOOLEAN
NTAPI
FsRtlMdlWriteComplete (
    IN PFILE_OBJECT     FileObject,
    IN PLARGE_INTEGER   FileOffset,
    IN PMDL             MdlChain
);

NTKERNELAPI
VOID
NTAPI
FsRtlNotifyChangeDirectory (
    IN PNOTIFY_SYNC NotifySync,
    IN PVOID        FsContext,
    IN PSTRING      FullDirectoryName,
    IN PLIST_ENTRY  NotifyList,
    IN BOOLEAN      WatchTree,
    IN ULONG        CompletionFilter,
    IN PIRP         NotifyIrp
);

NTKERNELAPI
NTSTATUS
NTAPI
ObCreateObject (
    IN KPROCESSOR_MODE      ObjectAttributesAccessMode OPTIONAL,
    IN POBJECT_TYPE         ObjectType,
    IN POBJECT_ATTRIBUTES   ObjectAttributes OPTIONAL,
    IN KPROCESSOR_MODE      AccessMode,
    IN OUT PVOID            ParseContext OPTIONAL,
    IN ULONG                ObjectSize,
    IN ULONG                PagedPoolCharge OPTIONAL,
    IN ULONG                NonPagedPoolCharge OPTIONAL,
    OUT PVOID               *Object
);

NTKERNELAPI
ULONG
NTAPI
ObGetObjectPointerCount (
    IN PVOID Object
);

NTKERNELAPI
NTSTATUS
NTAPI
ObReferenceObjectByName (
    IN PUNICODE_STRING  ObjectName,
    IN ULONG            Attributes,
    IN PACCESS_STATE    PassedAccessState OPTIONAL,
    IN ACCESS_MASK      DesiredAccess OPTIONAL,
    IN POBJECT_TYPE     ObjectType,
    IN KPROCESSOR_MODE  AccessMode,
    IN OUT PVOID        ParseContext OPTIONAL,
    OUT PVOID           *Object
);

#define PsDereferenceImpersonationToken(T)  \
            {if (ARGUMENT_PRESENT(T)) {     \
                (ObDereferenceObject((T))); \
            } else {                        \
                ;                           \
            }                               \
}

NTKERNELAPI
NTSTATUS
NTAPI
PsLookupProcessThreadByCid (
    IN PCLIENT_ID   Cid,
    OUT PEPROCESS   *Process OPTIONAL,
    OUT PETHREAD    *Thread
);

NTSYSAPI
NTSTATUS
NTAPI
RtlSetSaclSecurityDescriptor (
    IN OUT PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN BOOLEAN                  SaclPresent,
    IN PACL                     Sacl,
    IN BOOLEAN                  SaclDefaulted
);

#define SeEnableAccessToExports() SeExports = *(PSE_EXPORTS *)SeExports;

#if (VER_PRODUCTBUILD >= 2195)

NTSYSAPI
NTSTATUS
NTAPI
ZwAdjustPrivilegesToken (
    IN HANDLE               TokenHandle,
    IN BOOLEAN              DisableAllPrivileges,
    IN PTOKEN_PRIVILEGES    NewState,
    IN ULONG                BufferLength,
    OUT PTOKEN_PRIVILEGES   PreviousState OPTIONAL,
    OUT PULONG              ReturnLength
);

#endif /* (VER_PRODUCTBUILD >= 2195) */

NTSYSAPI
NTSTATUS
NTAPI
ZwAlertThread (
    IN HANDLE ThreadHandle
);

NTSYSAPI
NTSTATUS
NTAPI
ZwAccessCheckAndAuditAlarm (
    IN PUNICODE_STRING      SubsystemName,
    IN PVOID                HandleId,
    IN PUNICODE_STRING      ObjectTypeName,
    IN PUNICODE_STRING      ObjectName,
    IN PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN ACCESS_MASK          DesiredAccess,
    IN PGENERIC_MAPPING     GenericMapping,
    IN BOOLEAN              ObjectCreation,
    OUT PACCESS_MASK        GrantedAccess,
    OUT PBOOLEAN            AccessStatus,
    OUT PBOOLEAN            GenerateOnClose
);

#if (VER_PRODUCTBUILD >= 2195)

NTSYSAPI
NTSTATUS
NTAPI
ZwCancelIoFile (
    IN HANDLE               FileHandle,
    OUT PIO_STATUS_BLOCK    IoStatusBlock
);

#endif /* (VER_PRODUCTBUILD >= 2195) */

NTSYSAPI
NTSTATUS
NTAPI
ZwClearEvent (
    IN HANDLE EventHandle
);

NTSYSAPI
NTSTATUS
NTAPI
ZwCloseObjectAuditAlarm (
    IN PUNICODE_STRING  SubsystemName,
    IN PVOID            HandleId,
    IN BOOLEAN          GenerateOnClose
);

NTSYSAPI
NTSTATUS
NTAPI
ZwCreateSymbolicLinkObject (
    OUT PHANDLE             SymbolicLinkHandle,
    IN ACCESS_MASK          DesiredAccess,
    IN POBJECT_ATTRIBUTES   ObjectAttributes,
    IN PUNICODE_STRING      TargetName
);

NTSYSAPI
NTSTATUS
NTAPI
ZwFlushInstructionCache (
    IN HANDLE   ProcessHandle,
    IN PVOID    BaseAddress OPTIONAL,
    IN ULONG    FlushSize
);

NTSYSAPI
NTSTATUS
NTAPI
ZwFlushBuffersFile(
    IN HANDLE FileHandle,
    OUT PIO_STATUS_BLOCK IoStatusBlock
);

#if (VER_PRODUCTBUILD >= 2195)

NTSYSAPI
NTSTATUS
NTAPI
ZwInitiatePowerAction (
    IN POWER_ACTION         SystemAction,
    IN SYSTEM_POWER_STATE   MinSystemState,
    IN ULONG                Flags,
    IN BOOLEAN              Asynchronous
);

#endif /* (VER_PRODUCTBUILD >= 2195) */

NTSYSAPI
NTSTATUS
NTAPI
ZwLoadKey (
    IN POBJECT_ATTRIBUTES KeyObjectAttributes,
    IN POBJECT_ATTRIBUTES FileObjectAttributes
);

NTSYSAPI
NTSTATUS
NTAPI
ZwOpenProcessToken (
    IN HANDLE       ProcessHandle,
    IN ACCESS_MASK  DesiredAccess,
    OUT PHANDLE     TokenHandle
);

NTSYSAPI
NTSTATUS
NTAPI
ZwOpenThread (
    OUT PHANDLE             ThreadHandle,
    IN ACCESS_MASK          DesiredAccess,
    IN POBJECT_ATTRIBUTES   ObjectAttributes,
    IN PCLIENT_ID           ClientId
);

NTSYSAPI
NTSTATUS
NTAPI
ZwOpenThreadToken (
    IN HANDLE       ThreadHandle,
    IN ACCESS_MASK  DesiredAccess,
    IN BOOLEAN      OpenAsSelf,
    OUT PHANDLE     TokenHandle
);

NTSYSAPI
NTSTATUS
NTAPI
ZwPulseEvent (
    IN HANDLE   EventHandle,
    OUT PLONG   PreviousState OPTIONAL
);

NTSYSAPI
NTSTATUS
NTAPI
ZwQueryDefaultLocale (
    IN BOOLEAN  ThreadOrSystem,
    OUT PLCID   Locale
);

#if (VER_PRODUCTBUILD >= 2195)

NTSYSAPI
NTSTATUS
NTAPI
ZwQueryDirectoryObject (
    IN HANDLE       DirectoryHandle,
    OUT PVOID       Buffer,
    IN ULONG        Length,
    IN BOOLEAN      ReturnSingleEntry,
    IN BOOLEAN      RestartScan,
    IN OUT PULONG   Context,
    OUT PULONG      ReturnLength OPTIONAL
);

#endif /* (VER_PRODUCTBUILD >= 2195) */

NTSYSAPI
NTSTATUS
NTAPI
ZwQueryInformationProcess (
    IN HANDLE           ProcessHandle,
    IN PROCESSINFOCLASS ProcessInformationClass,
    OUT PVOID           ProcessInformation,
    IN ULONG            ProcessInformationLength,
    OUT PULONG          ReturnLength OPTIONAL
);

NTSYSAPI
NTSTATUS
NTAPI
ZwReplaceKey (
    IN POBJECT_ATTRIBUTES   NewFileObjectAttributes,
    IN HANDLE               KeyHandle,
    IN POBJECT_ATTRIBUTES   OldFileObjectAttributes
);

NTSYSAPI
NTSTATUS
NTAPI
ZwResetEvent (
    IN HANDLE   EventHandle,
    OUT PLONG   PreviousState OPTIONAL
);

#if (VER_PRODUCTBUILD >= 2195)

NTSYSAPI
NTSTATUS
NTAPI
ZwRestoreKey (
    IN HANDLE   KeyHandle,
    IN HANDLE   FileHandle,
    IN ULONG    Flags
);

#endif /* (VER_PRODUCTBUILD >= 2195) */

NTSYSAPI
NTSTATUS
NTAPI
ZwSaveKey (
    IN HANDLE KeyHandle,
    IN HANDLE FileHandle
);

NTSYSAPI
NTSTATUS
NTAPI
ZwSetDefaultLocale (
    IN BOOLEAN  ThreadOrSystem,
    IN LCID     Locale
);

#if (VER_PRODUCTBUILD >= 2195)

NTSYSAPI
NTSTATUS
NTAPI
ZwSetDefaultUILanguage (
    IN LANGID LanguageId
);

#endif /* (VER_PRODUCTBUILD >= 2195) */

NTSYSAPI
NTSTATUS
NTAPI
ZwSetInformationProcess (
    IN HANDLE           ProcessHandle,
    IN PROCESSINFOCLASS ProcessInformationClass,
    IN PVOID            ProcessInformation,
    IN ULONG            ProcessInformationLength
);

NTSYSAPI
NTSTATUS
NTAPI
ZwSetSystemTime (
    IN PLARGE_INTEGER   NewTime,
    OUT PLARGE_INTEGER  OldTime OPTIONAL
);

NTSYSAPI
NTSTATUS
NTAPI
ZwUnloadKey (
    IN POBJECT_ATTRIBUTES KeyObjectAttributes
);

NTSYSAPI
NTSTATUS
NTAPI
ZwWaitForMultipleObjects (
    IN ULONG            HandleCount,
    IN PHANDLE          Handles,
    IN WAIT_TYPE        WaitType,
    IN BOOLEAN          Alertable,
    IN PLARGE_INTEGER   Timeout OPTIONAL
);

NTSYSAPI
NTSTATUS
NTAPI
ZwYieldExecution (
    VOID
);

#pragma pack(pop)

#ifdef __cplusplus
}
#endif
