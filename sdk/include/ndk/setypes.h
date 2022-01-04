/*++ NDK Version: 0098

Copyright (c) Alex Ionescu.  All rights reserved.

Header Name:

    setypes.h

Abstract:

    Type definitions for the security manager.

Author:

    Alex Ionescu (alexi@tinykrnl.org) - Updated - 27-Feb-2006

--*/

#ifndef _SETYPES_H
#define _SETYPES_H

//
// Dependencies
//
#include <umtypes.h>

//
// Well Known SIDs
//
#define SECURITY_INTERNETSITE_AUTHORITY     {0,0,0,0,0,7}

#ifdef NTOS_MODE_USER
//
// Privilege constants
//
#define SE_MIN_WELL_KNOWN_PRIVILEGE       (2L)
#define SE_CREATE_TOKEN_PRIVILEGE         (2L)
#define SE_ASSIGNPRIMARYTOKEN_PRIVILEGE   (3L)
#define SE_LOCK_MEMORY_PRIVILEGE          (4L)
#define SE_INCREASE_QUOTA_PRIVILEGE       (5L)
#define SE_UNSOLICITED_INPUT_PRIVILEGE    (6L)
#define SE_MACHINE_ACCOUNT_PRIVILEGE      (6L)
#define SE_TCB_PRIVILEGE                  (7L)
#define SE_SECURITY_PRIVILEGE             (8L)
#define SE_TAKE_OWNERSHIP_PRIVILEGE       (9L)
#define SE_LOAD_DRIVER_PRIVILEGE          (10L)
#define SE_SYSTEM_PROFILE_PRIVILEGE       (11L)
#define SE_SYSTEMTIME_PRIVILEGE           (12L)
#define SE_PROF_SINGLE_PROCESS_PRIVILEGE  (13L)
#define SE_INC_BASE_PRIORITY_PRIVILEGE    (14L)
#define SE_CREATE_PAGEFILE_PRIVILEGE      (15L)
#define SE_CREATE_PERMANENT_PRIVILEGE     (16L)
#define SE_BACKUP_PRIVILEGE               (17L)
#define SE_RESTORE_PRIVILEGE              (18L)
#define SE_SHUTDOWN_PRIVILEGE             (19L)
#define SE_DEBUG_PRIVILEGE                (20L)
#define SE_AUDIT_PRIVILEGE                (21L)
#define SE_SYSTEM_ENVIRONMENT_PRIVILEGE   (22L)
#define SE_CHANGE_NOTIFY_PRIVILEGE        (23L)
#define SE_REMOTE_SHUTDOWN_PRIVILEGE      (24L)
#define SE_UNDOCK_PRIVILEGE               (25L)
#define SE_SYNC_AGENT_PRIVILEGE           (26L)
#define SE_ENABLE_DELEGATION_PRIVILEGE    (27L)
#define SE_MANAGE_VOLUME_PRIVILEGE        (28L)
#define SE_IMPERSONATE_PRIVILEGE          (29L)
#define SE_CREATE_GLOBAL_PRIVILEGE        (30L)
#define SE_MAX_WELL_KNOWN_PRIVILEGE       (SE_CREATE_GLOBAL_PRIVILEGE)

typedef struct _TOKEN_MANDATORY_POLICY {
  ULONG Policy;
} TOKEN_MANDATORY_POLICY, *PTOKEN_MANDATORY_POLICY;

typedef struct _TOKEN_ACCESS_INFORMATION
{
    struct _SID_AND_ATTRIBUTES_HASH *SidHash;
    struct _SID_AND_ATTRIBUTES_HASH *RestrictedSidHash;
    struct _TOKEN_PRIVILEGES *Privileges;
    LUID AuthenticationId;
    TOKEN_TYPE TokenType;
    SECURITY_IMPERSONATION_LEVEL ImpersonationLevel;
    TOKEN_MANDATORY_POLICY MandatoryPolicy;
    ULONG Flags;
} TOKEN_ACCESS_INFORMATION, *PTOKEN_ACCESS_INFORMATION;

#else

//
// User and Group-related SID Attributes
//
#define SE_GROUP_MANDATORY                                  0x00000001
#define SE_GROUP_ENABLED_BY_DEFAULT                         0x00000002
#define SE_GROUP_ENABLED                                    0x00000004
#define SE_GROUP_OWNER                                      0x00000008
#define SE_GROUP_USE_FOR_DENY_ONLY                          0x00000010
#define SE_GROUP_INTEGRITY                                  0x00000020
#define SE_GROUP_INTEGRITY_ENABLED                          0x00000040
#define SE_GROUP_RESOURCE                                   0x20000000
#define SE_GROUP_LOGON_ID                                   0xC0000000

#define SE_GROUP_VALID_ATTRIBUTES                           \
    (SE_GROUP_MANDATORY                                 |   \
     SE_GROUP_ENABLED_BY_DEFAULT                        |   \
     SE_GROUP_ENABLED                                   |   \
     SE_GROUP_OWNER                                     |   \
     SE_GROUP_USE_FOR_DENY_ONLY                         |   \
     SE_GROUP_LOGON_ID                                  |   \
     SE_GROUP_RESOURCE                                  |   \
     SE_GROUP_INTEGRITY                                 |   \
     SE_GROUP_INTEGRITY_ENABLED)

//
// Privilege token filtering flags
//
#define DISABLE_MAX_PRIVILEGE 0x1
#define SANDBOX_INERT         0x2
#if (NTDDI_VERSION >= NTDDI_LONGHORN)
#define LUA_TOKEN             0x4
#define WRITE_RESTRICTED      0x8
#endif

//
// Proxy Class enumeration
//
typedef enum _PROXY_CLASS
{
    ProxyFull = 0,
    ProxyService,
    ProxyTree,
    ProxyDirectory
} PROXY_CLASS;

//
// Audit and Policy Structures
//
typedef struct _SEP_AUDIT_POLICY_CATEGORIES
{
    UCHAR System:4;
    UCHAR Logon:4;
    UCHAR ObjectAccess:4;
    UCHAR PrivilegeUse:4;
    UCHAR DetailedTracking:4;
    UCHAR PolicyChange:4;
    UCHAR AccountManagement:4;
    UCHAR DirectoryServiceAccess:4;
    UCHAR AccountLogon:4;
} SEP_AUDIT_POLICY_CATEGORIES, *PSEP_AUDIT_POLICY_CATEGORIES;

typedef struct _SEP_AUDIT_POLICY_OVERLAY
{
    ULONGLONG PolicyBits:36;
    ULONGLONG SetBit:1;
} SEP_AUDIT_POLICY_OVERLAY, *PSEP_AUDIT_POLICY_OVERLAY;

typedef struct _SEP_AUDIT_POLICY
{
    union
    {
        SEP_AUDIT_POLICY_CATEGORIES PolicyElements;
        SEP_AUDIT_POLICY_OVERLAY PolicyOverlay;
        ULONGLONG Overlay;
    };
} SEP_AUDIT_POLICY, *PSEP_AUDIT_POLICY;

//
// Security Logon Session References
//
typedef struct _SEP_LOGON_SESSION_REFERENCES
{
    struct _SEP_LOGON_SESSION_REFERENCES *Next;
    LUID LogonId;
    ULONG ReferenceCount;
    ULONG Flags;
    PDEVICE_MAP pDeviceMap;
    LIST_ENTRY TokenList;
} SEP_LOGON_SESSION_REFERENCES, *PSEP_LOGON_SESSION_REFERENCES;

typedef struct _SE_AUDIT_PROCESS_CREATION_INFO
{
    POBJECT_NAME_INFORMATION ImageFileName;
} SE_AUDIT_PROCESS_CREATION_INFO, *PSE_AUDIT_PROCESS_CREATION_INFO;

//
// Token Audit Data
//
typedef struct _SECURITY_TOKEN_AUDIT_DATA
{
    ULONG Length;
    ULONG GrantMask;
    ULONG DenyMask;
} SECURITY_TOKEN_AUDIT_DATA, *PSECURITY_TOKEN_AUDIT_DATA;

//
// Token Proxy Data
//
typedef struct _SECURITY_TOKEN_PROXY_DATA
{
    ULONG Length;
    PROXY_CLASS ProxyClass;
    UNICODE_STRING PathInfo;
    ULONG ContainerMask;
    ULONG ObjectMask;
} SECURITY_TOKEN_PROXY_DATA, *PSECURITY_TOKEN_PROXY_DATA;

//
// Token and auxiliary data
//
typedef struct _TOKEN
{
    TOKEN_SOURCE TokenSource;                         /* 0x00 */
    LUID TokenId;                                     /* 0x10 */
    LUID AuthenticationId;                            /* 0x18 */
    LUID ParentTokenId;                               /* 0x20 */
    LARGE_INTEGER ExpirationTime;                     /* 0x28 */
    PERESOURCE TokenLock;                             /* 0x30 */
    SEP_AUDIT_POLICY AuditPolicy;                     /* 0x38 */
    LUID ModifiedId;                                  /* 0x40 */
    ULONG SessionId;                                  /* 0x48 */
    ULONG UserAndGroupCount;                          /* 0x4C */
    ULONG RestrictedSidCount;                         /* 0x50 */
    ULONG PrivilegeCount;                             /* 0x54 */
    ULONG VariableLength;                             /* 0x58 */
    ULONG DynamicCharged;                             /* 0x5C */
    ULONG DynamicAvailable;                           /* 0x60 */
    ULONG DefaultOwnerIndex;                          /* 0x64 */
    PSID_AND_ATTRIBUTES UserAndGroups;                /* 0x68 */
    PSID_AND_ATTRIBUTES RestrictedSids;               /* 0x6C */
    PSID PrimaryGroup;                                /* 0x70 */
    PLUID_AND_ATTRIBUTES Privileges;                  /* 0x74 */
    PULONG DynamicPart;                               /* 0x78 */
    PACL DefaultDacl;                                 /* 0x7C */
    TOKEN_TYPE TokenType;                             /* 0x80 */
    SECURITY_IMPERSONATION_LEVEL ImpersonationLevel;  /* 0x84 */
    ULONG TokenFlags;                                 /* 0x88 */
    BOOLEAN TokenInUse;                               /* 0x8C */
    PSECURITY_TOKEN_PROXY_DATA ProxyData;             /* 0x90 */
    PSECURITY_TOKEN_AUDIT_DATA AuditData;             /* 0x94 */
    PSEP_LOGON_SESSION_REFERENCES LogonSession;       /* 0x98 */
    LUID OriginatingLogonSession;                     /* 0x9C */
    ULONG VariablePart;                               /* 0xA4 */
} TOKEN, *PTOKEN;

typedef struct _AUX_ACCESS_DATA
{
    PPRIVILEGE_SET PrivilegeSet;
    GENERIC_MAPPING GenericMapping;
    ULONG Reserved;
} AUX_ACCESS_DATA, *PAUX_ACCESS_DATA;

//
// External SRM Data
//
extern PACL NTSYSAPI SePublicDefaultDacl;
extern PACL NTSYSAPI SeSystemDefaultDacl;

#endif
#endif
