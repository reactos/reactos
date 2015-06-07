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

/******************************************************************************
 *                            Security Manager Types                          *
 ******************************************************************************/

#ifndef SID_IDENTIFIER_AUTHORITY_DEFINED
#define SID_IDENTIFIER_AUTHORITY_DEFINED
typedef struct _SID_IDENTIFIER_AUTHORITY {
  UCHAR Value[6];
} SID_IDENTIFIER_AUTHORITY,*PSID_IDENTIFIER_AUTHORITY,*LPSID_IDENTIFIER_AUTHORITY;
#endif

#ifndef SID_DEFINED
#define SID_DEFINED
typedef struct _SID {
  UCHAR Revision;
  UCHAR SubAuthorityCount;
  SID_IDENTIFIER_AUTHORITY IdentifierAuthority;
#ifdef MIDL_PASS
  [size_is(SubAuthorityCount)] ULONG SubAuthority[*];
#else
  ULONG SubAuthority[ANYSIZE_ARRAY];
#endif
} SID, *PISID;
#endif

#define SID_REVISION                    1
#define SID_MAX_SUB_AUTHORITIES         15
#define SID_RECOMMENDED_SUB_AUTHORITIES 1

#ifndef MIDL_PASS
#define SECURITY_MAX_SID_SIZE (sizeof(SID) - sizeof(ULONG) + (SID_MAX_SUB_AUTHORITIES * sizeof(ULONG)))
#endif

typedef enum _SID_NAME_USE {
  SidTypeUser = 1,
  SidTypeGroup,
  SidTypeDomain,
  SidTypeAlias,
  SidTypeWellKnownGroup,
  SidTypeDeletedAccount,
  SidTypeInvalid,
  SidTypeUnknown,
  SidTypeComputer,
  SidTypeLabel
} SID_NAME_USE, *PSID_NAME_USE;

typedef struct _SID_AND_ATTRIBUTES {
#ifdef MIDL_PASS
  PISID Sid;
#else
  PSID Sid;
#endif
  ULONG Attributes;
} SID_AND_ATTRIBUTES, *PSID_AND_ATTRIBUTES;
typedef SID_AND_ATTRIBUTES SID_AND_ATTRIBUTES_ARRAY[ANYSIZE_ARRAY];
typedef SID_AND_ATTRIBUTES_ARRAY *PSID_AND_ATTRIBUTES_ARRAY;

#define SID_HASH_SIZE 32
typedef ULONG_PTR SID_HASH_ENTRY, *PSID_HASH_ENTRY;

typedef struct _SID_AND_ATTRIBUTES_HASH {
  ULONG SidCount;
  PSID_AND_ATTRIBUTES SidAttr;
  SID_HASH_ENTRY Hash[SID_HASH_SIZE];
} SID_AND_ATTRIBUTES_HASH, *PSID_AND_ATTRIBUTES_HASH;

/* Universal well-known SIDs */

#define SECURITY_NULL_SID_AUTHORITY         {0,0,0,0,0,0}

/* S-1-1 */
#define SECURITY_WORLD_SID_AUTHORITY        {0,0,0,0,0,1}

/* S-1-2 */
#define SECURITY_LOCAL_SID_AUTHORITY        {0,0,0,0,0,2}

/* S-1-3 */
#define SECURITY_CREATOR_SID_AUTHORITY      {0,0,0,0,0,3}

/* S-1-4 */
#define SECURITY_NON_UNIQUE_AUTHORITY       {0,0,0,0,0,4}

#define SECURITY_RESOURCE_MANAGER_AUTHORITY {0,0,0,0,0,9}

#define SECURITY_NULL_RID                   (0x00000000L)
#define SECURITY_WORLD_RID                  (0x00000000L)
#define SECURITY_LOCAL_RID                  (0x00000000L)
#define SECURITY_LOCAL_LOGON_RID            (0x00000001L)

#define SECURITY_CREATOR_OWNER_RID          (0x00000000L)
#define SECURITY_CREATOR_GROUP_RID          (0x00000001L)
#define SECURITY_CREATOR_OWNER_SERVER_RID   (0x00000002L)
#define SECURITY_CREATOR_GROUP_SERVER_RID   (0x00000003L)
#define SECURITY_CREATOR_OWNER_RIGHTS_RID   (0x00000004L)

/* NT well-known SIDs */

/* S-1-5 */
#define SECURITY_NT_AUTHORITY               {0,0,0,0,0,5}

#define SECURITY_DIALUP_RID                          (0x00000001L)
#define SECURITY_NETWORK_RID                         (0x00000002L)
#define SECURITY_BATCH_RID                           (0x00000003L)
#define SECURITY_INTERACTIVE_RID                     (0x00000004L)
#define SECURITY_LOGON_IDS_RID                       (0x00000005L)
#define SECURITY_LOGON_IDS_RID_COUNT                 (3L)
#define SECURITY_SERVICE_RID                         (0x00000006L)
#define SECURITY_ANONYMOUS_LOGON_RID                 (0x00000007L)
#define SECURITY_PROXY_RID                           (0x00000008L)
#define SECURITY_ENTERPRISE_CONTROLLERS_RID          (0x00000009L)
#define SECURITY_SERVER_LOGON_RID                    SECURITY_ENTERPRISE_CONTROLLERS_RID
#define SECURITY_PRINCIPAL_SELF_RID                  (0x0000000AL)
#define SECURITY_AUTHENTICATED_USER_RID              (0x0000000BL)
#define SECURITY_RESTRICTED_CODE_RID                 (0x0000000CL)
#define SECURITY_TERMINAL_SERVER_RID                 (0x0000000DL)
#define SECURITY_REMOTE_LOGON_RID                    (0x0000000EL)
#define SECURITY_THIS_ORGANIZATION_RID               (0x0000000FL)
#define SECURITY_IUSER_RID                           (0x00000011L)
#define SECURITY_LOCAL_SYSTEM_RID                    (0x00000012L)
#define SECURITY_LOCAL_SERVICE_RID                   (0x00000013L)
#define SECURITY_NETWORK_SERVICE_RID                 (0x00000014L)
#define SECURITY_NT_NON_UNIQUE                       (0x00000015L)
#define SECURITY_NT_NON_UNIQUE_SUB_AUTH_COUNT        (3L)
#define SECURITY_ENTERPRISE_READONLY_CONTROLLERS_RID (0x00000016L)

#define SECURITY_BUILTIN_DOMAIN_RID        (0x00000020L)
#define SECURITY_WRITE_RESTRICTED_CODE_RID (0x00000021L)


#define SECURITY_PACKAGE_BASE_RID     (0x00000040L)
#define SECURITY_PACKAGE_RID_COUNT    (2L)
#define SECURITY_PACKAGE_NTLM_RID     (0x0000000AL)
#define SECURITY_PACKAGE_SCHANNEL_RID (0x0000000EL)
#define SECURITY_PACKAGE_DIGEST_RID   (0x00000015L)

#define SECURITY_CRED_TYPE_BASE_RID          (0x00000041L)
#define SECURITY_CRED_TYPE_RID_COUNT         (2L)
#define SECURITY_CRED_TYPE_THIS_ORG_CERT_RID (0x00000001L)

#define SECURITY_MIN_BASE_RID                               (0x00000050L)
#define SECURITY_SERVICE_ID_BASE_RID                        (0x00000050L)
#define SECURITY_SERVICE_ID_RID_COUNT                       (6L)
#define SECURITY_RESERVED_ID_BASE_RID                       (0x00000051L)
#define SECURITY_APPPOOL_ID_BASE_RID                        (0x00000052L)
#define SECURITY_APPPOOL_ID_RID_COUNT                       (6L)
#define SECURITY_VIRTUALSERVER_ID_BASE_RID                  (0x00000053L)
#define SECURITY_VIRTUALSERVER_ID_RID_COUNT                 (6L)
#define SECURITY_USERMODEDRIVERHOST_ID_BASE_RID             (0x00000054L)
#define SECURITY_USERMODEDRIVERHOST_ID_RID_COUNT            (6L)
#define SECURITY_CLOUD_INFRASTRUCTURE_SERVICES_ID_BASE_RID  (0x00000055L)
#define SECURITY_CLOUD_INFRASTRUCTURE_SERVICES_ID_RID_COUNT (6L)
#define SECURITY_WMIHOST_ID_BASE_RID                        (0x00000056L)
#define SECURITY_WMIHOST_ID_RID_COUNT                       (6L)
#define SECURITY_TASK_ID_BASE_RID                           (0x00000057L)
#define SECURITY_NFS_ID_BASE_RID                            (0x00000058L)
#define SECURITY_COM_ID_BASE_RID                            (0x00000059L)
#define SECURITY_VIRTUALACCOUNT_ID_RID_COUNT                (6L)

#define SECURITY_MAX_BASE_RID (0x0000006FL)

#define SECURITY_MAX_ALWAYS_FILTERED (0x000003E7L)
#define SECURITY_MIN_NEVER_FILTERED  (0x000003E8L)

#define SECURITY_OTHER_ORGANIZATION_RID (0x000003E8L)

#define SECURITY_WINDOWSMOBILE_ID_BASE_RID (0x00000070L)

/* Well-known domain relative sub-authority values (RIDs) */

#define DOMAIN_GROUP_RID_ENTERPRISE_READONLY_DOMAIN_CONTROLLERS (0x000001F2L)

#define FOREST_USER_RID_MAX (0x000001F3L)

/* Well-known users */

#define DOMAIN_USER_RID_ADMIN  (0x000001F4L)
#define DOMAIN_USER_RID_GUEST  (0x000001F5L)
#define DOMAIN_USER_RID_KRBTGT (0x000001F6L)

#define DOMAIN_USER_RID_MAX (0x000003E7L)

/* Well-known groups */

#define DOMAIN_GROUP_RID_ADMINS               (0x00000200L)
#define DOMAIN_GROUP_RID_USERS                (0x00000201L)
#define DOMAIN_GROUP_RID_GUESTS               (0x00000202L)
#define DOMAIN_GROUP_RID_COMPUTERS            (0x00000203L)
#define DOMAIN_GROUP_RID_CONTROLLERS          (0x00000204L)
#define DOMAIN_GROUP_RID_CERT_ADMINS          (0x00000205L)
#define DOMAIN_GROUP_RID_SCHEMA_ADMINS        (0x00000206L)
#define DOMAIN_GROUP_RID_ENTERPRISE_ADMINS    (0x00000207L)
#define DOMAIN_GROUP_RID_POLICY_ADMINS        (0x00000208L)
#define DOMAIN_GROUP_RID_READONLY_CONTROLLERS (0x00000209L)

/* Well-known aliases */

#define DOMAIN_ALIAS_RID_ADMINS      (0x00000220L)
#define DOMAIN_ALIAS_RID_USERS       (0x00000221L)
#define DOMAIN_ALIAS_RID_GUESTS      (0x00000222L)
#define DOMAIN_ALIAS_RID_POWER_USERS (0x00000223L)

#define DOMAIN_ALIAS_RID_ACCOUNT_OPS (0x00000224L)
#define DOMAIN_ALIAS_RID_SYSTEM_OPS  (0x00000225L)
#define DOMAIN_ALIAS_RID_PRINT_OPS   (0x00000226L)
#define DOMAIN_ALIAS_RID_BACKUP_OPS  (0x00000227L)

#define DOMAIN_ALIAS_RID_REPLICATOR                     (0x00000228L)
#define DOMAIN_ALIAS_RID_RAS_SERVERS                    (0x00000229L)
#define DOMAIN_ALIAS_RID_PREW2KCOMPACCESS               (0x0000022AL)
#define DOMAIN_ALIAS_RID_REMOTE_DESKTOP_USERS           (0x0000022BL)
#define DOMAIN_ALIAS_RID_NETWORK_CONFIGURATION_OPS      (0x0000022CL)
#define DOMAIN_ALIAS_RID_INCOMING_FOREST_TRUST_BUILDERS (0x0000022DL)

#define DOMAIN_ALIAS_RID_MONITORING_USERS    (0x0000022EL)
#define DOMAIN_ALIAS_RID_LOGGING_USERS       (0x0000022FL)
#define DOMAIN_ALIAS_RID_AUTHORIZATIONACCESS (0x00000230L)
#define DOMAIN_ALIAS_RID_TS_LICENSE_SERVERS  (0x00000231L)
#define DOMAIN_ALIAS_RID_DCOM_USERS          (0x00000232L)

#define DOMAIN_ALIAS_RID_IUSERS                         (0x00000238L)
#define DOMAIN_ALIAS_RID_CRYPTO_OPERATORS               (0x00000239L)
#define DOMAIN_ALIAS_RID_CACHEABLE_PRINCIPALS_GROUP     (0x0000023BL)
#define DOMAIN_ALIAS_RID_NON_CACHEABLE_PRINCIPALS_GROUP (0x0000023CL)
#define DOMAIN_ALIAS_RID_EVENT_LOG_READERS_GROUP        (0x0000023DL)
#define DOMAIN_ALIAS_RID_CERTSVC_DCOM_ACCESS_GROUP      (0x0000023EL)

#define SECURITY_MANDATORY_LABEL_AUTHORITY       {0,0,0,0,0,16}
#define SECURITY_MANDATORY_UNTRUSTED_RID         (0x00000000L)
#define SECURITY_MANDATORY_LOW_RID               (0x00001000L)
#define SECURITY_MANDATORY_MEDIUM_RID            (0x00002000L)
#define SECURITY_MANDATORY_HIGH_RID              (0x00003000L)
#define SECURITY_MANDATORY_SYSTEM_RID            (0x00004000L)
#define SECURITY_MANDATORY_PROTECTED_PROCESS_RID (0x00005000L)

/* SECURITY_MANDATORY_MAXIMUM_USER_RID is the highest RID that
   can be set by a usermode caller.*/

#define SECURITY_MANDATORY_MAXIMUM_USER_RID SECURITY_MANDATORY_SYSTEM_RID

#define MANDATORY_LEVEL_TO_MANDATORY_RID(IL) (IL * 0x1000)

/* Allocate the System Luid.  The first 1000 LUIDs are reserved.
   Use #999 here (0x3e7 = 999) */

#define SYSTEM_LUID          {0x3e7, 0x0}
#define ANONYMOUS_LOGON_LUID {0x3e6, 0x0}
#define LOCALSERVICE_LUID    {0x3e5, 0x0}
#define NETWORKSERVICE_LUID  {0x3e4, 0x0}
#define IUSER_LUID           {0x3e3, 0x0}

typedef struct _ACE_HEADER {
  UCHAR AceType;
  UCHAR AceFlags;
  USHORT AceSize;
} ACE_HEADER, *PACE_HEADER;

#define ACCESS_MIN_MS_ACE_TYPE                  (0x0)
#define ACCESS_ALLOWED_ACE_TYPE                 (0x0)
#define ACCESS_DENIED_ACE_TYPE                  (0x1)
#define SYSTEM_AUDIT_ACE_TYPE                   (0x2)
#define SYSTEM_ALARM_ACE_TYPE                   (0x3)
#define ACCESS_MAX_MS_V2_ACE_TYPE               (0x3)
#define ACCESS_ALLOWED_COMPOUND_ACE_TYPE        (0x4)
#define ACCESS_MAX_MS_V3_ACE_TYPE               (0x4)
#define ACCESS_MIN_MS_OBJECT_ACE_TYPE           (0x5)
#define ACCESS_ALLOWED_OBJECT_ACE_TYPE          (0x5)
#define ACCESS_DENIED_OBJECT_ACE_TYPE           (0x6)
#define SYSTEM_AUDIT_OBJECT_ACE_TYPE            (0x7)
#define SYSTEM_ALARM_OBJECT_ACE_TYPE            (0x8)
#define ACCESS_MAX_MS_OBJECT_ACE_TYPE           (0x8)
#define ACCESS_MAX_MS_V4_ACE_TYPE               (0x8)
#define ACCESS_MAX_MS_ACE_TYPE                  (0x8)
#define ACCESS_ALLOWED_CALLBACK_ACE_TYPE        (0x9)
#define ACCESS_DENIED_CALLBACK_ACE_TYPE         (0xA)
#define ACCESS_ALLOWED_CALLBACK_OBJECT_ACE_TYPE (0xB)
#define ACCESS_DENIED_CALLBACK_OBJECT_ACE_TYPE  (0xC)
#define SYSTEM_AUDIT_CALLBACK_ACE_TYPE          (0xD)
#define SYSTEM_ALARM_CALLBACK_ACE_TYPE          (0xE)
#define SYSTEM_AUDIT_CALLBACK_OBJECT_ACE_TYPE   (0xF)
#define SYSTEM_ALARM_CALLBACK_OBJECT_ACE_TYPE   (0x10)
#define ACCESS_MAX_MS_V5_ACE_TYPE               (0x11)
#define SYSTEM_MANDATORY_LABEL_ACE_TYPE         (0x11)

/* The following are the inherit flags that go into the AceFlags field
   of an Ace header. */

#define OBJECT_INHERIT_ACE       (0x1)
#define CONTAINER_INHERIT_ACE    (0x2)
#define NO_PROPAGATE_INHERIT_ACE (0x4)
#define INHERIT_ONLY_ACE         (0x8)
#define INHERITED_ACE            (0x10)
#define VALID_INHERIT_FLAGS      (0x1F)

#define SUCCESSFUL_ACCESS_ACE_FLAG (0x40)
#define FAILED_ACCESS_ACE_FLAG     (0x80)

typedef struct _ACCESS_ALLOWED_ACE {
  ACE_HEADER Header;
  ACCESS_MASK Mask;
  ULONG SidStart;
} ACCESS_ALLOWED_ACE, *PACCESS_ALLOWED_ACE;

typedef struct _ACCESS_DENIED_ACE {
  ACE_HEADER Header;
  ACCESS_MASK Mask;
  ULONG SidStart;
} ACCESS_DENIED_ACE, *PACCESS_DENIED_ACE;

typedef struct _SYSTEM_AUDIT_ACE {
  ACE_HEADER Header;
  ACCESS_MASK Mask;
  ULONG SidStart;
} SYSTEM_AUDIT_ACE, *PSYSTEM_AUDIT_ACE;

typedef struct _SYSTEM_ALARM_ACE {
  ACE_HEADER Header;
  ACCESS_MASK Mask;
  ULONG SidStart;
} SYSTEM_ALARM_ACE, *PSYSTEM_ALARM_ACE;

typedef struct _SYSTEM_MANDATORY_LABEL_ACE {
  ACE_HEADER Header;
  ACCESS_MASK Mask;
  ULONG SidStart;
} SYSTEM_MANDATORY_LABEL_ACE, *PSYSTEM_MANDATORY_LABEL_ACE;

#define SYSTEM_MANDATORY_LABEL_NO_WRITE_UP   0x1
#define SYSTEM_MANDATORY_LABEL_NO_READ_UP    0x2
#define SYSTEM_MANDATORY_LABEL_NO_EXECUTE_UP 0x4
#define SYSTEM_MANDATORY_LABEL_VALID_MASK    (SYSTEM_MANDATORY_LABEL_NO_WRITE_UP | \
                                              SYSTEM_MANDATORY_LABEL_NO_READ_UP  | \
                                              SYSTEM_MANDATORY_LABEL_NO_EXECUTE_UP)

#define SECURITY_DESCRIPTOR_MIN_LENGTH (sizeof(SECURITY_DESCRIPTOR))

typedef USHORT SECURITY_DESCRIPTOR_CONTROL, *PSECURITY_DESCRIPTOR_CONTROL;

#define SE_OWNER_DEFAULTED       0x0001
#define SE_GROUP_DEFAULTED       0x0002
#define SE_DACL_PRESENT          0x0004
#define SE_DACL_DEFAULTED        0x0008
#define SE_SACL_PRESENT          0x0010
#define SE_SACL_DEFAULTED        0x0020
#define SE_DACL_UNTRUSTED        0x0040
#define SE_SERVER_SECURITY       0x0080
#define SE_DACL_AUTO_INHERIT_REQ 0x0100
#define SE_SACL_AUTO_INHERIT_REQ 0x0200
#define SE_DACL_AUTO_INHERITED   0x0400
#define SE_SACL_AUTO_INHERITED   0x0800
#define SE_DACL_PROTECTED        0x1000
#define SE_SACL_PROTECTED        0x2000
#define SE_RM_CONTROL_VALID      0x4000
#define SE_SELF_RELATIVE         0x8000

typedef struct _SECURITY_DESCRIPTOR_RELATIVE {
  UCHAR Revision;
  UCHAR Sbz1;
  SECURITY_DESCRIPTOR_CONTROL Control;
  ULONG Owner;
  ULONG Group;
  ULONG Sacl;
  ULONG Dacl;
} SECURITY_DESCRIPTOR_RELATIVE, *PISECURITY_DESCRIPTOR_RELATIVE;

typedef struct _SECURITY_DESCRIPTOR {
  UCHAR Revision;
  UCHAR Sbz1;
  SECURITY_DESCRIPTOR_CONTROL Control;
  PSID Owner;
  PSID Group;
  PACL Sacl;
  PACL Dacl;
} SECURITY_DESCRIPTOR, *PISECURITY_DESCRIPTOR;

typedef struct _OBJECT_TYPE_LIST {
  USHORT Level;
  USHORT Sbz;
  GUID *ObjectType;
} OBJECT_TYPE_LIST, *POBJECT_TYPE_LIST;

#define ACCESS_OBJECT_GUID       0
#define ACCESS_PROPERTY_SET_GUID 1
#define ACCESS_PROPERTY_GUID     2
#define ACCESS_MAX_LEVEL         4

typedef enum _AUDIT_EVENT_TYPE {
  AuditEventObjectAccess,
  AuditEventDirectoryServiceAccess
} AUDIT_EVENT_TYPE, *PAUDIT_EVENT_TYPE;

#define AUDIT_ALLOW_NO_PRIVILEGE 0x1

#define ACCESS_DS_SOURCE_A "DS"
#define ACCESS_DS_SOURCE_W L"DS"
#define ACCESS_DS_OBJECT_TYPE_NAME_A "Directory Service Object"
#define ACCESS_DS_OBJECT_TYPE_NAME_W L"Directory Service Object"

#define ACCESS_REASON_TYPE_MASK 0xffff0000
#define ACCESS_REASON_DATA_MASK 0x0000ffff

typedef enum _ACCESS_REASON_TYPE {
  AccessReasonNone = 0x00000000,
  AccessReasonAllowedAce = 0x00010000,
  AccessReasonDeniedAce = 0x00020000,
  AccessReasonAllowedParentAce = 0x00030000,
  AccessReasonDeniedParentAce = 0x00040000,
  AccessReasonMissingPrivilege = 0x00100000,
  AccessReasonFromPrivilege = 0x00200000,
  AccessReasonIntegrityLevel = 0x00300000,
  AccessReasonOwnership = 0x00400000,
  AccessReasonNullDacl = 0x00500000,
  AccessReasonEmptyDacl = 0x00600000,
  AccessReasonNoSD = 0x00700000,
  AccessReasonNoGrant = 0x00800000
} ACCESS_REASON_TYPE;

typedef ULONG ACCESS_REASON;

typedef struct _ACCESS_REASONS {
  ACCESS_REASON Data[32];
} ACCESS_REASONS, *PACCESS_REASONS;

#define SE_SECURITY_DESCRIPTOR_FLAG_NO_OWNER_ACE 0x00000001
#define SE_SECURITY_DESCRIPTOR_FLAG_NO_LABEL_ACE 0x00000002
#define SE_SECURITY_DESCRIPTOR_VALID_FLAGS       0x00000003

typedef struct _SE_SECURITY_DESCRIPTOR {
  ULONG Size;
  ULONG Flags;
  PSECURITY_DESCRIPTOR SecurityDescriptor;
} SE_SECURITY_DESCRIPTOR, *PSE_SECURITY_DESCRIPTOR;

typedef struct _SE_ACCESS_REQUEST {
  ULONG Size;
  PSE_SECURITY_DESCRIPTOR SeSecurityDescriptor;
  ACCESS_MASK DesiredAccess;
  ACCESS_MASK PreviouslyGrantedAccess;
  PSID PrincipalSelfSid;
  PGENERIC_MAPPING GenericMapping;
  ULONG ObjectTypeListCount;
  POBJECT_TYPE_LIST ObjectTypeList;
} SE_ACCESS_REQUEST, *PSE_ACCESS_REQUEST;

#define TOKEN_ASSIGN_PRIMARY    (0x0001)
#define TOKEN_DUPLICATE         (0x0002)
#define TOKEN_IMPERSONATE       (0x0004)
#define TOKEN_QUERY             (0x0008)
#define TOKEN_QUERY_SOURCE      (0x0010)
#define TOKEN_ADJUST_PRIVILEGES (0x0020)
#define TOKEN_ADJUST_GROUPS     (0x0040)
#define TOKEN_ADJUST_DEFAULT    (0x0080)
#define TOKEN_ADJUST_SESSIONID  (0x0100)

#define TOKEN_ALL_ACCESS_P (STANDARD_RIGHTS_REQUIRED |\
                            TOKEN_ASSIGN_PRIMARY     |\
                            TOKEN_DUPLICATE          |\
                            TOKEN_IMPERSONATE        |\
                            TOKEN_QUERY              |\
                            TOKEN_QUERY_SOURCE       |\
                            TOKEN_ADJUST_PRIVILEGES  |\
                            TOKEN_ADJUST_GROUPS      |\
                            TOKEN_ADJUST_DEFAULT)

#if ((defined(_WIN32_WINNT) && (_WIN32_WINNT > 0x0400)) || (!defined(_WIN32_WINNT)))
#define TOKEN_ALL_ACCESS (TOKEN_ALL_ACCESS_P | TOKEN_ADJUST_SESSIONID)
#else
#define TOKEN_ALL_ACCESS (TOKEN_ALL_ACCESS_P)
#endif

#define TOKEN_READ (STANDARD_RIGHTS_READ | TOKEN_QUERY)

#define TOKEN_WRITE (STANDARD_RIGHTS_WRITE   |\
                     TOKEN_ADJUST_PRIVILEGES |\
                     TOKEN_ADJUST_GROUPS     |\
                     TOKEN_ADJUST_DEFAULT)

#define TOKEN_EXECUTE (STANDARD_RIGHTS_EXECUTE)

typedef enum _TOKEN_TYPE {
  TokenPrimary = 1,
  TokenImpersonation
} TOKEN_TYPE, *PTOKEN_TYPE;

typedef enum _TOKEN_INFORMATION_CLASS {
  TokenUser = 1,
  TokenGroups,
  TokenPrivileges,
  TokenOwner,
  TokenPrimaryGroup,
  TokenDefaultDacl,
  TokenSource,
  TokenType,
  TokenImpersonationLevel,
  TokenStatistics,
  TokenRestrictedSids,
  TokenSessionId,
  TokenGroupsAndPrivileges,
  TokenSessionReference,
  TokenSandBoxInert,
  TokenAuditPolicy,
  TokenOrigin,
  TokenElevationType,
  TokenLinkedToken,
  TokenElevation,
  TokenHasRestrictions,
  TokenAccessInformation,
  TokenVirtualizationAllowed,
  TokenVirtualizationEnabled,
  TokenIntegrityLevel,
  TokenUIAccess,
  TokenMandatoryPolicy,
  TokenLogonSid,
  MaxTokenInfoClass
} TOKEN_INFORMATION_CLASS, *PTOKEN_INFORMATION_CLASS;

typedef struct _TOKEN_USER {
  SID_AND_ATTRIBUTES User;
} TOKEN_USER, *PTOKEN_USER;

typedef struct _TOKEN_GROUPS {
  ULONG GroupCount;
#ifdef MIDL_PASS
  [size_is(GroupCount)] SID_AND_ATTRIBUTES Groups[*];
#else
  SID_AND_ATTRIBUTES Groups[ANYSIZE_ARRAY];
#endif
} TOKEN_GROUPS, *PTOKEN_GROUPS, *LPTOKEN_GROUPS;

typedef struct _TOKEN_PRIVILEGES {
  ULONG PrivilegeCount;
  LUID_AND_ATTRIBUTES Privileges[ANYSIZE_ARRAY];
} TOKEN_PRIVILEGES, *PTOKEN_PRIVILEGES, *LPTOKEN_PRIVILEGES;

typedef struct _TOKEN_OWNER {
  PSID Owner;
} TOKEN_OWNER, *PTOKEN_OWNER;

typedef struct _TOKEN_PRIMARY_GROUP {
  PSID PrimaryGroup;
} TOKEN_PRIMARY_GROUP, *PTOKEN_PRIMARY_GROUP;

typedef struct _TOKEN_DEFAULT_DACL {
  PACL DefaultDacl;
} TOKEN_DEFAULT_DACL, *PTOKEN_DEFAULT_DACL;

typedef struct _TOKEN_GROUPS_AND_PRIVILEGES {
  ULONG SidCount;
  ULONG SidLength;
  PSID_AND_ATTRIBUTES Sids;
  ULONG RestrictedSidCount;
  ULONG RestrictedSidLength;
  PSID_AND_ATTRIBUTES RestrictedSids;
  ULONG PrivilegeCount;
  ULONG PrivilegeLength;
  PLUID_AND_ATTRIBUTES Privileges;
  LUID AuthenticationId;
} TOKEN_GROUPS_AND_PRIVILEGES, *PTOKEN_GROUPS_AND_PRIVILEGES;

typedef struct _TOKEN_LINKED_TOKEN {
  HANDLE LinkedToken;
} TOKEN_LINKED_TOKEN, *PTOKEN_LINKED_TOKEN;

typedef struct _TOKEN_ELEVATION {
  ULONG TokenIsElevated;
} TOKEN_ELEVATION, *PTOKEN_ELEVATION;

typedef struct _TOKEN_MANDATORY_LABEL {
  SID_AND_ATTRIBUTES Label;
} TOKEN_MANDATORY_LABEL, *PTOKEN_MANDATORY_LABEL;

#define TOKEN_MANDATORY_POLICY_OFF             0x0
#define TOKEN_MANDATORY_POLICY_NO_WRITE_UP     0x1
#define TOKEN_MANDATORY_POLICY_NEW_PROCESS_MIN 0x2

#define TOKEN_MANDATORY_POLICY_VALID_MASK (TOKEN_MANDATORY_POLICY_NO_WRITE_UP | \
                                           TOKEN_MANDATORY_POLICY_NEW_PROCESS_MIN)

#define POLICY_AUDIT_SUBCATEGORY_COUNT (56)

typedef struct _TOKEN_AUDIT_POLICY {
  UCHAR PerUserPolicy[((POLICY_AUDIT_SUBCATEGORY_COUNT) >> 1) + 1];
} TOKEN_AUDIT_POLICY, *PTOKEN_AUDIT_POLICY;

#define TOKEN_SOURCE_LENGTH 8

typedef struct _TOKEN_SOURCE {
  CHAR SourceName[TOKEN_SOURCE_LENGTH];
  LUID SourceIdentifier;
} TOKEN_SOURCE, *PTOKEN_SOURCE;

typedef struct _TOKEN_STATISTICS {
  LUID TokenId;
  LUID AuthenticationId;
  LARGE_INTEGER ExpirationTime;
  TOKEN_TYPE TokenType;
  SECURITY_IMPERSONATION_LEVEL ImpersonationLevel;
  ULONG DynamicCharged;
  ULONG DynamicAvailable;
  ULONG GroupCount;
  ULONG PrivilegeCount;
  LUID ModifiedId;
} TOKEN_STATISTICS, *PTOKEN_STATISTICS;

typedef struct _TOKEN_CONTROL {
  LUID TokenId;
  LUID AuthenticationId;
  LUID ModifiedId;
  TOKEN_SOURCE TokenSource;
} TOKEN_CONTROL, *PTOKEN_CONTROL;

typedef struct _TOKEN_ORIGIN {
  LUID OriginatingLogonSession;
} TOKEN_ORIGIN, *PTOKEN_ORIGIN;

typedef enum _MANDATORY_LEVEL {
  MandatoryLevelUntrusted = 0,
  MandatoryLevelLow,
  MandatoryLevelMedium,
  MandatoryLevelHigh,
  MandatoryLevelSystem,
  MandatoryLevelSecureProcess,
  MandatoryLevelCount
} MANDATORY_LEVEL, *PMANDATORY_LEVEL;


typedef struct _SE_ACCESS_REPLY {
  ULONG Size;
  ULONG ResultListCount;
  PACCESS_MASK GrantedAccess;
  PNTSTATUS AccessStatus;
  PACCESS_REASONS AccessReason;
  PPRIVILEGE_SET* Privileges;
} SE_ACCESS_REPLY, *PSE_ACCESS_REPLY;

typedef enum _SE_AUDIT_OPERATION {
  AuditPrivilegeObject,
  AuditPrivilegeService,
  AuditAccessCheck,
  AuditOpenObject,
  AuditOpenObjectWithTransaction,
  AuditCloseObject,
  AuditDeleteObject,
  AuditOpenObjectForDelete,
  AuditOpenObjectForDeleteWithTransaction,
  AuditCloseNonObject,
  AuditOpenNonObject,
  AuditObjectReference,
  AuditHandleCreation,
} SE_AUDIT_OPERATION, *PSE_AUDIT_OPERATION;

typedef struct _SE_AUDIT_INFO {
  ULONG Size;
  AUDIT_EVENT_TYPE AuditType;
  SE_AUDIT_OPERATION AuditOperation;
  ULONG AuditFlags;
  UNICODE_STRING SubsystemName;
  UNICODE_STRING ObjectTypeName;
  UNICODE_STRING ObjectName;
  PVOID HandleId;
  GUID* TransactionId;
  LUID* OperationId;
  BOOLEAN ObjectCreation;
  BOOLEAN GenerateOnClose;
} SE_AUDIT_INFO, *PSE_AUDIT_INFO;

typedef struct _TOKEN_MANDATORY_POLICY {
  ULONG Policy;
} TOKEN_MANDATORY_POLICY, *PTOKEN_MANDATORY_POLICY;

typedef struct _TOKEN_ACCESS_INFORMATION {
  PSID_AND_ATTRIBUTES_HASH SidHash;
  PSID_AND_ATTRIBUTES_HASH RestrictedSidHash;
  PTOKEN_PRIVILEGES Privileges;
  LUID AuthenticationId;
  TOKEN_TYPE TokenType;
  SECURITY_IMPERSONATION_LEVEL ImpersonationLevel;
  TOKEN_MANDATORY_POLICY MandatoryPolicy;
  ULONG Flags;
} TOKEN_ACCESS_INFORMATION, *PTOKEN_ACCESS_INFORMATION;

#define TOKEN_HAS_TRAVERSE_PRIVILEGE    0x0001
#define TOKEN_HAS_BACKUP_PRIVILEGE      0x0002
#define TOKEN_HAS_RESTORE_PRIVILEGE     0x0004
#define TOKEN_WRITE_RESTRICTED          0x0008
#define TOKEN_IS_RESTRICTED             0x0010
#define TOKEN_SESSION_NOT_REFERENCED    0x0020
#define TOKEN_SANDBOX_INERT             0x0040
#define TOKEN_HAS_IMPERSONATE_PRIVILEGE 0x0080
#define SE_BACKUP_PRIVILEGES_CHECKED    0x0100
#define TOKEN_VIRTUALIZE_ALLOWED        0x0200
#define TOKEN_VIRTUALIZE_ENABLED        0x0400
#define TOKEN_IS_FILTERED               0x0800
#define TOKEN_UIACCESS                  0x1000
#define TOKEN_NOT_LOW                   0x2000

typedef struct _SE_EXPORTS {
  LUID SeCreateTokenPrivilege;
  LUID SeAssignPrimaryTokenPrivilege;
  LUID SeLockMemoryPrivilege;
  LUID SeIncreaseQuotaPrivilege;
  LUID SeUnsolicitedInputPrivilege;
  LUID SeTcbPrivilege;
  LUID SeSecurityPrivilege;
  LUID SeTakeOwnershipPrivilege;
  LUID SeLoadDriverPrivilege;
  LUID SeCreatePagefilePrivilege;
  LUID SeIncreaseBasePriorityPrivilege;
  LUID SeSystemProfilePrivilege;
  LUID SeSystemtimePrivilege;
  LUID SeProfileSingleProcessPrivilege;
  LUID SeCreatePermanentPrivilege;
  LUID SeBackupPrivilege;
  LUID SeRestorePrivilege;
  LUID SeShutdownPrivilege;
  LUID SeDebugPrivilege;
  LUID SeAuditPrivilege;
  LUID SeSystemEnvironmentPrivilege;
  LUID SeChangeNotifyPrivilege;
  LUID SeRemoteShutdownPrivilege;
  PSID SeNullSid;
  PSID SeWorldSid;
  PSID SeLocalSid;
  PSID SeCreatorOwnerSid;
  PSID SeCreatorGroupSid;
  PSID SeNtAuthoritySid;
  PSID SeDialupSid;
  PSID SeNetworkSid;
  PSID SeBatchSid;
  PSID SeInteractiveSid;
  PSID SeLocalSystemSid;
  PSID SeAliasAdminsSid;
  PSID SeAliasUsersSid;
  PSID SeAliasGuestsSid;
  PSID SeAliasPowerUsersSid;
  PSID SeAliasAccountOpsSid;
  PSID SeAliasSystemOpsSid;
  PSID SeAliasPrintOpsSid;
  PSID SeAliasBackupOpsSid;
  PSID SeAuthenticatedUsersSid;
  PSID SeRestrictedSid;
  PSID SeAnonymousLogonSid;
  LUID SeUndockPrivilege;
  LUID SeSyncAgentPrivilege;
  LUID SeEnableDelegationPrivilege;
  PSID SeLocalServiceSid;
  PSID SeNetworkServiceSid;
  LUID SeManageVolumePrivilege;
  LUID SeImpersonatePrivilege;
  LUID SeCreateGlobalPrivilege;
  LUID SeTrustedCredManAccessPrivilege;
  LUID SeRelabelPrivilege;
  LUID SeIncreaseWorkingSetPrivilege;
  LUID SeTimeZonePrivilege;
  LUID SeCreateSymbolicLinkPrivilege;
  PSID SeIUserSid;
  PSID SeUntrustedMandatorySid;
  PSID SeLowMandatorySid;
  PSID SeMediumMandatorySid;
  PSID SeHighMandatorySid;
  PSID SeSystemMandatorySid;
  PSID SeOwnerRightsSid;
} SE_EXPORTS, *PSE_EXPORTS;

typedef NTSTATUS
(NTAPI *PSE_LOGON_SESSION_TERMINATED_ROUTINE)(
  IN PLUID LogonId);

typedef struct _SECURITY_CLIENT_CONTEXT {
  SECURITY_QUALITY_OF_SERVICE SecurityQos;
  PACCESS_TOKEN ClientToken;
  BOOLEAN DirectlyAccessClientToken;
  BOOLEAN DirectAccessEffectiveOnly;
  BOOLEAN ServerIsRemote;
  TOKEN_CONTROL ClientTokenControl;
} SECURITY_CLIENT_CONTEXT, *PSECURITY_CLIENT_CONTEXT;

/******************************************************************************
 *                            Object Manager Types                            *
 ******************************************************************************/

typedef enum _OBJECT_INFORMATION_CLASS {
  ObjectBasicInformation = 0,
  ObjectTypeInformation = 2,
  /* Not for public use */
  ObjectNameInformation = 1,
  ObjectTypesInformation = 3,
  ObjectHandleFlagInformation = 4,
  ObjectSessionInformation = 5,
  MaxObjectInfoClass
} OBJECT_INFORMATION_CLASS;


/******************************************************************************
 *                           Runtime Library Types                            *
 ******************************************************************************/


#define RTL_SYSTEM_VOLUME_INFORMATION_FOLDER    L"System Volume Information"

_Function_class_(RTL_ALLOCATE_STRING_ROUTINE)
_IRQL_requires_max_(PASSIVE_LEVEL)
__drv_allocatesMem(Mem)
typedef PVOID
(NTAPI *PRTL_ALLOCATE_STRING_ROUTINE)(
  _In_ SIZE_T NumberOfBytes);

#if _WIN32_WINNT >= 0x0600
_Function_class_(RTL_REALLOCATE_STRING_ROUTINE)
_IRQL_requires_max_(PASSIVE_LEVEL)
__drv_allocatesMem(Mem)
typedef PVOID
(NTAPI *PRTL_REALLOCATE_STRING_ROUTINE)(
  _In_ SIZE_T NumberOfBytes,
  IN PVOID Buffer);
#endif

typedef VOID
(NTAPI *PRTL_FREE_STRING_ROUTINE)(
  _In_ __drv_freesMem(Mem) _Post_invalid_ PVOID Buffer);

extern NTKERNELAPI const PRTL_ALLOCATE_STRING_ROUTINE RtlAllocateStringRoutine;
extern NTKERNELAPI const PRTL_FREE_STRING_ROUTINE RtlFreeStringRoutine;

#if _WIN32_WINNT >= 0x0600
extern NTKERNELAPI const PRTL_REALLOCATE_STRING_ROUTINE RtlReallocateStringRoutine;
#endif

_Function_class_(RTL_HEAP_COMMIT_ROUTINE)
_IRQL_requires_same_
typedef NTSTATUS
(NTAPI *PRTL_HEAP_COMMIT_ROUTINE) (
  _In_ PVOID Base,
  _Inout_ PVOID *CommitAddress,
  _Inout_ PSIZE_T CommitSize);

typedef struct _RTL_HEAP_PARAMETERS {
  ULONG Length;
  SIZE_T SegmentReserve;
  SIZE_T SegmentCommit;
  SIZE_T DeCommitFreeBlockThreshold;
  SIZE_T DeCommitTotalFreeThreshold;
  SIZE_T MaximumAllocationSize;
  SIZE_T VirtualMemoryThreshold;
  SIZE_T InitialCommit;
  SIZE_T InitialReserve;
  PRTL_HEAP_COMMIT_ROUTINE CommitRoutine;
  SIZE_T Reserved[2];
} RTL_HEAP_PARAMETERS, *PRTL_HEAP_PARAMETERS;

#if (NTDDI_VERSION >= NTDDI_WIN2K)

typedef struct _GENERATE_NAME_CONTEXT {
  USHORT Checksum;
  BOOLEAN CheckSumInserted;
  _Field_range_(<=, 8) UCHAR NameLength;
  WCHAR NameBuffer[8];
  _Field_range_(<=, 4) ULONG ExtensionLength;
  WCHAR ExtensionBuffer[4];
  ULONG LastIndexValue;
} GENERATE_NAME_CONTEXT, *PGENERATE_NAME_CONTEXT;

typedef struct _PREFIX_TABLE_ENTRY {
  CSHORT NodeTypeCode;
  CSHORT NameLength;
  struct _PREFIX_TABLE_ENTRY *NextPrefixTree;
  RTL_SPLAY_LINKS Links;
  PSTRING Prefix;
} PREFIX_TABLE_ENTRY, *PPREFIX_TABLE_ENTRY;

typedef struct _PREFIX_TABLE {
  CSHORT NodeTypeCode;
  CSHORT NameLength;
  PPREFIX_TABLE_ENTRY NextPrefixTree;
} PREFIX_TABLE, *PPREFIX_TABLE;

typedef struct _UNICODE_PREFIX_TABLE_ENTRY {
  CSHORT NodeTypeCode;
  CSHORT NameLength;
  struct _UNICODE_PREFIX_TABLE_ENTRY *NextPrefixTree;
  struct _UNICODE_PREFIX_TABLE_ENTRY *CaseMatch;
  RTL_SPLAY_LINKS Links;
  PUNICODE_STRING Prefix;
} UNICODE_PREFIX_TABLE_ENTRY, *PUNICODE_PREFIX_TABLE_ENTRY;

typedef struct _UNICODE_PREFIX_TABLE {
  CSHORT NodeTypeCode;
  CSHORT NameLength;
  PUNICODE_PREFIX_TABLE_ENTRY NextPrefixTree;
  PUNICODE_PREFIX_TABLE_ENTRY LastNextEntry;
} UNICODE_PREFIX_TABLE, *PUNICODE_PREFIX_TABLE;

#endif /* (NTDDI_VERSION >= NTDDI_WIN2K) */

#if (NTDDI_VERSION >= NTDDI_WINXP)
typedef struct _COMPRESSED_DATA_INFO {
  USHORT CompressionFormatAndEngine;
  UCHAR CompressionUnitShift;
  UCHAR ChunkShift;
  UCHAR ClusterShift;
  UCHAR Reserved;
  USHORT NumberOfChunks;
  ULONG CompressedChunkSizes[ANYSIZE_ARRAY];
} COMPRESSED_DATA_INFO, *PCOMPRESSED_DATA_INFO;
#endif
/******************************************************************************
 *                         Runtime Library Functions                          *
 ******************************************************************************/


#if (NTDDI_VERSION >= NTDDI_WIN2K)


_Must_inspect_result_
_Ret_maybenull_
_Post_writable_byte_size_(Size)
NTSYSAPI
PVOID
NTAPI
RtlAllocateHeap(
  _In_ HANDLE HeapHandle,
  _In_opt_ ULONG Flags,
  _In_ SIZE_T Size);

_Success_(return != 0)
NTSYSAPI
BOOLEAN
NTAPI
RtlFreeHeap(
  _In_ PVOID HeapHandle,
  _In_opt_ ULONG Flags,
  _In_ _Post_invalid_ PVOID BaseAddress);

NTSYSAPI
VOID
NTAPI
RtlCaptureContext(
  _Out_ PCONTEXT ContextRecord);

_Ret_range_(<, MAXLONG)
NTSYSAPI
ULONG
NTAPI
RtlRandom(
  _Inout_ PULONG Seed);

_IRQL_requires_max_(APC_LEVEL)
_Success_(return != 0)
_Must_inspect_result_
NTSYSAPI
BOOLEAN
NTAPI
RtlCreateUnicodeString(
  _Out_ _At_(DestinationString->Buffer, __drv_allocatesMem(Mem))
    PUNICODE_STRING DestinationString,
  _In_z_ PCWSTR SourceString);

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSYSAPI
BOOLEAN
NTAPI
RtlPrefixString(
  _In_ const STRING *String1,
  _In_ const STRING *String2,
  _In_ BOOLEAN CaseInsensitive);

_IRQL_requires_max_(APC_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
RtlAppendStringToString(
  _Inout_ PSTRING Destination,
  _In_ const STRING *Source);

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSYSAPI
NTSTATUS
NTAPI
RtlOemStringToUnicodeString(
  _When_(AllocateDestinationString, _Out_ _At_(DestinationString->Buffer, __drv_allocatesMem(Mem)))
  _When_(!AllocateDestinationString, _Inout_)
    PUNICODE_STRING DestinationString,
  _In_ PCOEM_STRING SourceString,
  _In_ BOOLEAN AllocateDestinationString);

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSYSAPI
NTSTATUS
NTAPI
RtlUnicodeStringToOemString(
  _When_(AllocateDestinationString, _Out_ _At_(DestinationString->Buffer, __drv_allocatesMem(Mem)))
  _When_(!AllocateDestinationString, _Inout_)
    POEM_STRING DestinationString,
  _In_ PCUNICODE_STRING SourceString,
  _In_ BOOLEAN AllocateDestinationString);

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSYSAPI
NTSTATUS
NTAPI
RtlUpcaseUnicodeStringToOemString(
  _When_(AllocateDestinationString, _Out_ _At_(DestinationString->Buffer, __drv_allocatesMem(Mem)))
  _When_(!AllocateDestinationString, _Inout_)
    POEM_STRING DestinationString,
  _In_ PCUNICODE_STRING SourceString,
  _In_ BOOLEAN AllocateDestinationString);

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSYSAPI
NTSTATUS
NTAPI
RtlOemStringToCountedUnicodeString(
  _When_(AllocateDestinationString, _Out_ _At_(DestinationString->Buffer, __drv_allocatesMem(Mem)))
  _When_(!AllocateDestinationString, _Inout_)
    PUNICODE_STRING DestinationString,
  _In_ PCOEM_STRING SourceString,
  _In_ BOOLEAN AllocateDestinationString);

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSYSAPI
NTSTATUS
NTAPI
RtlUnicodeStringToCountedOemString(
  _When_(AllocateDestinationString, _Out_ _At_(DestinationString->Buffer, __drv_allocatesMem(Mem)))
  _When_(!AllocateDestinationString, _Inout_)
    POEM_STRING DestinationString,
  _In_ PCUNICODE_STRING SourceString,
  _In_ BOOLEAN AllocateDestinationString);

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSYSAPI
NTSTATUS
NTAPI
RtlUpcaseUnicodeStringToCountedOemString(
  _When_(AllocateDestinationString, _Out_ _At_(DestinationString->Buffer, __drv_allocatesMem(Mem)))
  _When_(!AllocateDestinationString, _Inout_)
    POEM_STRING DestinationString,
  _In_ PCUNICODE_STRING SourceString,
  _In_ BOOLEAN AllocateDestinationString);

_IRQL_requires_max_(PASSIVE_LEVEL)
_When_(AllocateDestinationString, _Must_inspect_result_)
NTSYSAPI
NTSTATUS
NTAPI
RtlDowncaseUnicodeString(
  _When_(AllocateDestinationString, _Out_ _At_(UniDest->Buffer, __drv_allocatesMem(Mem)))
  _When_(!AllocateDestinationString, _Inout_)
    PUNICODE_STRING UniDest,
  _In_ PCUNICODE_STRING UniSource,
  _In_ BOOLEAN AllocateDestinationString);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
VOID
NTAPI
RtlFreeOemString(
  _Inout_ _At_(OemString->Buffer, __drv_freesMem(Mem)) POEM_STRING OemString);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
ULONG
NTAPI
RtlxUnicodeStringToOemSize(
  _In_ PCUNICODE_STRING UnicodeString);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
ULONG
NTAPI
RtlxOemStringToUnicodeSize(
  _In_ PCOEM_STRING OemString);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
RtlMultiByteToUnicodeN(
  _Out_writes_bytes_to_(MaxBytesInUnicodeString, *BytesInUnicodeString) PWCH UnicodeString,
  _In_ ULONG MaxBytesInUnicodeString,
  _Out_opt_ PULONG BytesInUnicodeString,
  _In_reads_bytes_(BytesInMultiByteString) const CHAR *MultiByteString,
  _In_ ULONG BytesInMultiByteString);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
RtlMultiByteToUnicodeSize(
  _Out_ PULONG BytesInUnicodeString,
  _In_reads_bytes_(BytesInMultiByteString) const CHAR *MultiByteString,
  _In_ ULONG BytesInMultiByteString);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
RtlUnicodeToMultiByteSize(
  _Out_ PULONG BytesInMultiByteString,
  _In_reads_bytes_(BytesInUnicodeString) PCWCH UnicodeString,
  _In_ ULONG BytesInUnicodeString);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
RtlUnicodeToMultiByteN(
  _Out_writes_bytes_to_(MaxBytesInMultiByteString, *BytesInMultiByteString) PCHAR MultiByteString,
  _In_ ULONG MaxBytesInMultiByteString,
  _Out_opt_ PULONG BytesInMultiByteString,
  _In_reads_bytes_(BytesInUnicodeString) PCWCH UnicodeString,
  _In_ ULONG BytesInUnicodeString);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
RtlUpcaseUnicodeToMultiByteN(
  _Out_writes_bytes_to_(MaxBytesInMultiByteString, *BytesInMultiByteString) PCHAR MultiByteString,
  _In_ ULONG MaxBytesInMultiByteString,
  _Out_opt_ PULONG BytesInMultiByteString,
  _In_reads_bytes_(BytesInUnicodeString) PCWCH UnicodeString,
  _In_ ULONG BytesInUnicodeString);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
RtlOemToUnicodeN(
  _Out_writes_bytes_to_(MaxBytesInUnicodeString, *BytesInUnicodeString) PWSTR UnicodeString,
  _In_ ULONG MaxBytesInUnicodeString,
  _Out_opt_ PULONG BytesInUnicodeString,
  _In_reads_bytes_(BytesInOemString) PCCH OemString,
  _In_ ULONG BytesInOemString);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
RtlUnicodeToOemN(
  _Out_writes_bytes_to_(MaxBytesInOemString, *BytesInOemString) PCHAR OemString,
  _In_ ULONG MaxBytesInOemString,
  _Out_opt_ PULONG BytesInOemString,
  _In_reads_bytes_(BytesInUnicodeString) PCWCH UnicodeString,
  _In_ ULONG BytesInUnicodeString);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
RtlUpcaseUnicodeToOemN(
  _Out_writes_bytes_to_(MaxBytesInOemString, *BytesInOemString) PCHAR OemString,
  _In_ ULONG MaxBytesInOemString,
  _Out_opt_ PULONG BytesInOemString,
  _In_reads_bytes_(BytesInUnicodeString) PCWCH UnicodeString,
  _In_ ULONG BytesInUnicodeString);

#if (NTDDI_VERSION >= NTDDI_VISTASP1)
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
RtlGenerate8dot3Name(
  _In_ PCUNICODE_STRING Name,
  _In_ BOOLEAN AllowExtendedCharacters,
  _Inout_ PGENERATE_NAME_CONTEXT Context,
  _Inout_ PUNICODE_STRING Name8dot3);
#else
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
VOID
NTAPI
RtlGenerate8dot3Name(
  _In_ PCUNICODE_STRING Name,
  _In_ BOOLEAN AllowExtendedCharacters,
  _Inout_ PGENERATE_NAME_CONTEXT Context,
  _Inout_ PUNICODE_STRING Name8dot3);
#endif

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSYSAPI
BOOLEAN
NTAPI
RtlIsNameLegalDOS8Dot3(
  _In_ PCUNICODE_STRING Name,
  _Inout_opt_ POEM_STRING OemName,
  _Out_opt_ PBOOLEAN NameContainsSpaces);

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSYSAPI
BOOLEAN
NTAPI
RtlIsValidOemCharacter(
  _Inout_ PWCHAR Char);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
VOID
NTAPI
PfxInitialize(
  _Out_ PPREFIX_TABLE PrefixTable);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
BOOLEAN
NTAPI
PfxInsertPrefix(
  _In_ PPREFIX_TABLE PrefixTable,
  _In_ __drv_aliasesMem PSTRING Prefix,
  _Out_ PPREFIX_TABLE_ENTRY PrefixTableEntry);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
VOID
NTAPI
PfxRemovePrefix(
  _In_ PPREFIX_TABLE PrefixTable,
  _In_ PPREFIX_TABLE_ENTRY PrefixTableEntry);

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSYSAPI
PPREFIX_TABLE_ENTRY
NTAPI
PfxFindPrefix(
  _In_ PPREFIX_TABLE PrefixTable,
  _In_ PSTRING FullName);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
VOID
NTAPI
RtlInitializeUnicodePrefix(
  _Out_ PUNICODE_PREFIX_TABLE PrefixTable);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
BOOLEAN
NTAPI
RtlInsertUnicodePrefix(
  _In_ PUNICODE_PREFIX_TABLE PrefixTable,
  _In_ __drv_aliasesMem PUNICODE_STRING Prefix,
  _Out_ PUNICODE_PREFIX_TABLE_ENTRY PrefixTableEntry);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
VOID
NTAPI
RtlRemoveUnicodePrefix(
  _In_ PUNICODE_PREFIX_TABLE PrefixTable,
  _In_ PUNICODE_PREFIX_TABLE_ENTRY PrefixTableEntry);

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSYSAPI
PUNICODE_PREFIX_TABLE_ENTRY
NTAPI
RtlFindUnicodePrefix(
  _In_ PUNICODE_PREFIX_TABLE PrefixTable,
  _In_ PUNICODE_STRING FullName,
  _In_ ULONG CaseInsensitiveIndex);

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSYSAPI
PUNICODE_PREFIX_TABLE_ENTRY
NTAPI
RtlNextUnicodePrefix(
  _In_ PUNICODE_PREFIX_TABLE PrefixTable,
  _In_ BOOLEAN Restart);

_Must_inspect_result_
NTSYSAPI
SIZE_T
NTAPI
RtlCompareMemoryUlong(
  _In_reads_bytes_(Length) PVOID Source,
  _In_ SIZE_T Length,
  _In_ ULONG Pattern);

_Success_(return != 0)
NTSYSAPI
BOOLEAN
NTAPI
RtlTimeToSecondsSince1980(
  _In_ PLARGE_INTEGER Time,
  _Out_ PULONG ElapsedSeconds);

NTSYSAPI
VOID
NTAPI
RtlSecondsSince1980ToTime(
  _In_ ULONG ElapsedSeconds,
  _Out_ PLARGE_INTEGER Time);

_Success_(return != 0)
_Must_inspect_result_
NTSYSAPI
BOOLEAN
NTAPI
RtlTimeToSecondsSince1970(
    _In_ PLARGE_INTEGER Time,
    _Out_ PULONG ElapsedSeconds);

NTSYSAPI
VOID
NTAPI
RtlSecondsSince1970ToTime(
  _In_ ULONG ElapsedSeconds,
  _Out_ PLARGE_INTEGER Time);

_IRQL_requires_max_(APC_LEVEL)
_Must_inspect_result_
NTSYSAPI
BOOLEAN
NTAPI
RtlValidSid(
  _In_ PSID Sid);

_Must_inspect_result_
NTSYSAPI
BOOLEAN
NTAPI
RtlEqualSid(
  _In_ PSID Sid1,
  _In_ PSID Sid2);

_IRQL_requires_max_(APC_LEVEL)
_Must_inspect_result_
NTSYSAPI
BOOLEAN
NTAPI
RtlEqualPrefixSid(
  _In_ PSID Sid1,
  _In_ PSID Sid2);

_IRQL_requires_max_(APC_LEVEL)
NTSYSAPI
ULONG
NTAPI
RtlLengthRequiredSid(
  _In_ ULONG SubAuthorityCount);

NTSYSAPI
PVOID
NTAPI
RtlFreeSid(
  _In_ _Post_invalid_ PSID Sid);

_Must_inspect_result_
NTSYSAPI
NTSTATUS
NTAPI
RtlAllocateAndInitializeSid(
  _In_ PSID_IDENTIFIER_AUTHORITY IdentifierAuthority,
  _In_ UCHAR SubAuthorityCount,
  _In_ ULONG SubAuthority0,
  _In_ ULONG SubAuthority1,
  _In_ ULONG SubAuthority2,
  _In_ ULONG SubAuthority3,
  _In_ ULONG SubAuthority4,
  _In_ ULONG SubAuthority5,
  _In_ ULONG SubAuthority6,
  _In_ ULONG SubAuthority7,
  _Outptr_ PSID *Sid);

_IRQL_requires_max_(APC_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
RtlInitializeSid(
  _Out_ PSID Sid,
  _In_ PSID_IDENTIFIER_AUTHORITY IdentifierAuthority,
  _In_ UCHAR SubAuthorityCount);

NTSYSAPI
PULONG
NTAPI
RtlSubAuthoritySid(
  _In_ PSID Sid,
  _In_ ULONG SubAuthority);

_Post_satisfies_(return >= 8 && return <= SECURITY_MAX_SID_SIZE)
NTSYSAPI
ULONG
NTAPI
RtlLengthSid(
  _In_ PSID Sid);

_IRQL_requires_max_(APC_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
RtlCopySid(
  _In_ ULONG DestinationSidLength,
  _Out_writes_bytes_(DestinationSidLength) PSID DestinationSid,
  _In_ PSID SourceSid);

_IRQL_requires_max_(APC_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
RtlConvertSidToUnicodeString(
  _Inout_ PUNICODE_STRING UnicodeString,
  _In_ PSID Sid,
  _In_ BOOLEAN AllocateDestinationString);

_IRQL_requires_max_(APC_LEVEL)
NTSYSAPI
VOID
NTAPI
RtlCopyLuid(
  _Out_ PLUID DestinationLuid,
  _In_ PLUID SourceLuid);

_IRQL_requires_max_(APC_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
RtlCreateAcl(
  _Out_writes_bytes_(AclLength) PACL Acl,
  _In_ ULONG AclLength,
  _In_ ULONG AclRevision);

_IRQL_requires_max_(APC_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
RtlAddAce(
  _Inout_ PACL Acl,
  _In_ ULONG AceRevision,
  _In_ ULONG StartingAceIndex,
  _In_reads_bytes_(AceListLength) PVOID AceList,
  _In_ ULONG AceListLength);

_IRQL_requires_max_(APC_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
RtlDeleteAce(
  _Inout_ PACL Acl,
  _In_ ULONG AceIndex);

NTSYSAPI
NTSTATUS
NTAPI
RtlGetAce(
  _In_ PACL Acl,
  _In_ ULONG AceIndex,
  _Outptr_ PVOID *Ace);

_IRQL_requires_max_(APC_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
RtlAddAccessAllowedAce(
  _Inout_ PACL Acl,
  _In_ ULONG AceRevision,
  _In_ ACCESS_MASK AccessMask,
  _In_ PSID Sid);

_IRQL_requires_max_(APC_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
RtlAddAccessAllowedAceEx(
  _Inout_ PACL Acl,
  _In_ ULONG AceRevision,
  _In_ ULONG AceFlags,
  _In_ ACCESS_MASK AccessMask,
  _In_ PSID Sid);

_IRQL_requires_max_(APC_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
RtlCreateSecurityDescriptorRelative(
  _Out_ PISECURITY_DESCRIPTOR_RELATIVE SecurityDescriptor,
  _In_ ULONG Revision);

NTSYSAPI
NTSTATUS
NTAPI
RtlGetDaclSecurityDescriptor(
  _In_ PSECURITY_DESCRIPTOR SecurityDescriptor,
  _Out_ PBOOLEAN DaclPresent,
  _Out_ PACL *Dacl,
  _Out_ PBOOLEAN DaclDefaulted);

_IRQL_requires_max_(APC_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
RtlSetOwnerSecurityDescriptor(
  _Inout_ PSECURITY_DESCRIPTOR SecurityDescriptor,
  _In_opt_ PSID Owner,
  _In_opt_ BOOLEAN OwnerDefaulted);

_IRQL_requires_max_(APC_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
RtlGetOwnerSecurityDescriptor(
  _In_ PSECURITY_DESCRIPTOR SecurityDescriptor,
  _Out_ PSID *Owner,
  _Out_ PBOOLEAN OwnerDefaulted);

_IRQL_requires_max_(APC_LEVEL)
_When_(Status < 0, _Out_range_(>, 0))
_When_(Status >= 0, _Out_range_(==, 0))
NTSYSAPI
ULONG
NTAPI
RtlNtStatusToDosError(
  _In_ NTSTATUS Status);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
RtlCustomCPToUnicodeN(
  _In_ PCPTABLEINFO CustomCP,
  _Out_writes_bytes_to_(MaxBytesInUnicodeString, *BytesInUnicodeString) PWCH UnicodeString,
  _In_ ULONG MaxBytesInUnicodeString,
  _Out_opt_ PULONG BytesInUnicodeString,
  _In_reads_bytes_(BytesInCustomCPString) PCH CustomCPString,
  _In_ ULONG BytesInCustomCPString);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
RtlUnicodeToCustomCPN(
  _In_ PCPTABLEINFO CustomCP,
  _Out_writes_bytes_to_(MaxBytesInCustomCPString, *BytesInCustomCPString) PCH CustomCPString,
  _In_ ULONG MaxBytesInCustomCPString,
  _Out_opt_ PULONG BytesInCustomCPString,
  _In_reads_bytes_(BytesInUnicodeString) PWCH UnicodeString,
  _In_ ULONG BytesInUnicodeString);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
RtlUpcaseUnicodeToCustomCPN(
  _In_ PCPTABLEINFO CustomCP,
  _Out_writes_bytes_to_(MaxBytesInCustomCPString, *BytesInCustomCPString) PCH CustomCPString,
  _In_ ULONG MaxBytesInCustomCPString,
  _Out_opt_ PULONG BytesInCustomCPString,
  _In_reads_bytes_(BytesInUnicodeString) PWCH UnicodeString,
  _In_ ULONG BytesInUnicodeString);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
VOID
NTAPI
RtlInitCodePageTable(
  _In_ PUSHORT TableBase,
  _Out_ PCPTABLEINFO CodePageTable);


#endif /* (NTDDI_VERSION >= NTDDI_WIN2K) */


#if (NTDDI_VERSION >= NTDDI_WINXP)



_Must_inspect_result_
NTSYSAPI
PVOID
NTAPI
RtlCreateHeap(
  _In_ ULONG Flags,
  _In_opt_ PVOID HeapBase,
  _In_opt_ SIZE_T ReserveSize,
  _In_opt_ SIZE_T CommitSize,
  _In_opt_ PVOID Lock,
  _In_opt_ PRTL_HEAP_PARAMETERS Parameters);

NTSYSAPI
PVOID
NTAPI
RtlDestroyHeap(
  _In_ _Post_invalid_ PVOID HeapHandle);

NTSYSAPI
USHORT
NTAPI
RtlCaptureStackBackTrace(
  _In_ ULONG FramesToSkip,
  _In_ ULONG FramesToCapture,
  _Out_writes_to_(FramesToCapture, return) PVOID *BackTrace,
  _Out_opt_ PULONG BackTraceHash);

_Ret_range_(<, MAXLONG)
NTSYSAPI
ULONG
NTAPI
RtlRandomEx(
  _Inout_ PULONG Seed);

_IRQL_requires_max_(DISPATCH_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
RtlInitUnicodeStringEx(
  _Out_ PUNICODE_STRING DestinationString,
  _In_opt_z_ __drv_aliasesMem PCWSTR SourceString);

_Must_inspect_result_
NTSYSAPI
NTSTATUS
NTAPI
RtlValidateUnicodeString(
  _In_ ULONG Flags,
  _In_ PCUNICODE_STRING String);

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSYSAPI
NTSTATUS
NTAPI
RtlDuplicateUnicodeString(
  _In_ ULONG Flags,
  _In_ PCUNICODE_STRING SourceString,
  _Out_ _At_(DestinationString->Buffer, __drv_allocatesMem(Mem)) PUNICODE_STRING DestinationString);

NTSYSAPI
NTSTATUS
NTAPI
RtlGetCompressionWorkSpaceSize(
  _In_ USHORT CompressionFormatAndEngine,
  _Out_ PULONG CompressBufferWorkSpaceSize,
  _Out_ PULONG CompressFragmentWorkSpaceSize);

NTSYSAPI
NTSTATUS
NTAPI
RtlCompressBuffer(
  _In_ USHORT CompressionFormatAndEngine,
  _In_reads_bytes_(UncompressedBufferSize) PUCHAR UncompressedBuffer,
  _In_ ULONG UncompressedBufferSize,
  _Out_writes_bytes_to_(CompressedBufferSize, *FinalCompressedSize) PUCHAR CompressedBuffer,
  _In_ ULONG CompressedBufferSize,
  _In_ ULONG UncompressedChunkSize,
  _Out_ PULONG FinalCompressedSize,
  _In_ PVOID WorkSpace);

_IRQL_requires_max_(APC_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
RtlDecompressBuffer(
  _In_ USHORT CompressionFormat,
  _Out_writes_bytes_to_(UncompressedBufferSize, *FinalUncompressedSize) PUCHAR UncompressedBuffer,
  _In_ ULONG UncompressedBufferSize,
  _In_reads_bytes_(CompressedBufferSize) PUCHAR CompressedBuffer,
  _In_ ULONG CompressedBufferSize,
  _Out_ PULONG FinalUncompressedSize);

_IRQL_requires_max_(APC_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
RtlDecompressFragment(
  _In_ USHORT CompressionFormat,
  _Out_writes_bytes_to_(UncompressedFragmentSize, *FinalUncompressedSize) PUCHAR UncompressedFragment,
  _In_ ULONG UncompressedFragmentSize,
  _In_reads_bytes_(CompressedBufferSize) PUCHAR CompressedBuffer,
  _In_ ULONG CompressedBufferSize,
  _In_range_(<, CompressedBufferSize) ULONG FragmentOffset,
  _Out_ PULONG FinalUncompressedSize,
  _In_ PVOID WorkSpace);

_IRQL_requires_max_(APC_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
RtlDescribeChunk(
  _In_ USHORT CompressionFormat,
  _Inout_ PUCHAR *CompressedBuffer,
  _In_ PUCHAR EndOfCompressedBufferPlus1,
  _Out_ PUCHAR *ChunkBuffer,
  _Out_ PULONG ChunkSize);

_IRQL_requires_max_(APC_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
RtlReserveChunk(
  _In_ USHORT CompressionFormat,
  _Inout_ PUCHAR *CompressedBuffer,
  _In_ PUCHAR EndOfCompressedBufferPlus1,
  _Out_ PUCHAR *ChunkBuffer,
  _In_ ULONG ChunkSize);

_IRQL_requires_max_(APC_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
RtlDecompressChunks(
  _Out_writes_bytes_(UncompressedBufferSize) PUCHAR UncompressedBuffer,
  _In_ ULONG UncompressedBufferSize,
  _In_reads_bytes_(CompressedBufferSize) PUCHAR CompressedBuffer,
  _In_ ULONG CompressedBufferSize,
  _In_reads_bytes_(CompressedTailSize) PUCHAR CompressedTail,
  _In_ ULONG CompressedTailSize,
  _In_ PCOMPRESSED_DATA_INFO CompressedDataInfo);

_IRQL_requires_max_(APC_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
RtlCompressChunks(
  _In_reads_bytes_(UncompressedBufferSize) PUCHAR UncompressedBuffer,
  _In_ ULONG UncompressedBufferSize,
  _Out_writes_bytes_(CompressedBufferSize) PUCHAR CompressedBuffer,
  _In_range_(>=, (UncompressedBufferSize - (UncompressedBufferSize / 16))) ULONG CompressedBufferSize,
  _Inout_updates_bytes_(CompressedDataInfoLength) PCOMPRESSED_DATA_INFO CompressedDataInfo,
  _In_range_(>, sizeof(COMPRESSED_DATA_INFO)) ULONG CompressedDataInfoLength,
  _In_ PVOID WorkSpace);

_IRQL_requires_max_(APC_LEVEL)
NTSYSAPI
PSID_IDENTIFIER_AUTHORITY
NTAPI
RtlIdentifierAuthoritySid(
  _In_ PSID Sid);

NTSYSAPI
PUCHAR
NTAPI
RtlSubAuthorityCountSid(
  _In_ PSID Sid);

_When_(Status < 0, _Out_range_(>, 0))
_When_(Status >= 0, _Out_range_(==, 0))
NTSYSAPI
ULONG
NTAPI
RtlNtStatusToDosErrorNoTeb(
  _In_ NTSTATUS Status);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
RtlCreateSystemVolumeInformationFolder(
  _In_ PCUNICODE_STRING VolumeRootPath);

#if defined(_M_AMD64)

FORCEINLINE
VOID
RtlFillMemoryUlong(
  _Out_writes_bytes_all_(Length) PVOID Destination,
  _In_ SIZE_T Length,
  _In_ ULONG Pattern)
{
  PULONG Address = (PULONG)Destination;
  if ((Length /= 4) != 0) {
    if (((ULONG64)Address & 4) != 0) {
      *Address = Pattern;
      if ((Length -= 1) == 0) {
        return;
      }
      Address += 1;
    }
    __stosq((PULONG64)(Address), Pattern | ((ULONG64)Pattern << 32), Length / 2);
    if ((Length & 1) != 0) Address[Length - 1] = Pattern;
  }
  return;
}

#define RtlFillMemoryUlonglong(Destination, Length, Pattern)                \
    __stosq((PULONG64)(Destination), Pattern, (Length) / 8)

#else

NTSYSAPI
VOID
NTAPI
RtlFillMemoryUlong(
  OUT PVOID Destination,
  IN SIZE_T Length,
  IN ULONG Pattern);

NTSYSAPI
VOID
NTAPI
RtlFillMemoryUlonglong(
  _Out_writes_bytes_all_(Length) PVOID Destination,
  _In_ SIZE_T Length,
  _In_ ULONGLONG Pattern);

#endif /* defined(_M_AMD64) */

#endif /* (NTDDI_VERSION >= NTDDI_WINXP) */

#if (NTDDI_VERSION >= NTDDI_WS03)
_IRQL_requires_max_(DISPATCH_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
RtlInitAnsiStringEx(
  _Out_ PANSI_STRING DestinationString,
  _In_opt_z_ __drv_aliasesMem PCSZ SourceString);
#endif

#if (NTDDI_VERSION >= NTDDI_WS03SP1)

_IRQL_requires_max_(APC_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
RtlGetSaclSecurityDescriptor(
  _In_ PSECURITY_DESCRIPTOR SecurityDescriptor,
  _Out_ PBOOLEAN SaclPresent,
  _Out_ PACL *Sacl,
  _Out_ PBOOLEAN SaclDefaulted);

_IRQL_requires_max_(APC_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
RtlSetGroupSecurityDescriptor(
  _Inout_ PSECURITY_DESCRIPTOR SecurityDescriptor,
  _In_opt_ PSID Group,
  _In_opt_ BOOLEAN GroupDefaulted);

_IRQL_requires_max_(APC_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
RtlGetGroupSecurityDescriptor(
  _In_ PSECURITY_DESCRIPTOR SecurityDescriptor,
  _Out_ PSID *Group,
  _Out_ PBOOLEAN GroupDefaulted);

_IRQL_requires_max_(APC_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
RtlAbsoluteToSelfRelativeSD(
  _In_ PSECURITY_DESCRIPTOR AbsoluteSecurityDescriptor,
  _Out_writes_bytes_to_opt_(*BufferLength, *BufferLength) PSECURITY_DESCRIPTOR SelfRelativeSecurityDescriptor,
  _Inout_ PULONG BufferLength);

_IRQL_requires_max_(APC_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
RtlSelfRelativeToAbsoluteSD(
  _In_ PSECURITY_DESCRIPTOR SelfRelativeSecurityDescriptor,
  _Out_writes_bytes_to_opt_(*AbsoluteSecurityDescriptorSize, *AbsoluteSecurityDescriptorSize) PSECURITY_DESCRIPTOR AbsoluteSecurityDescriptor,
  _Inout_ PULONG AbsoluteSecurityDescriptorSize,
  _Out_writes_bytes_to_opt_(*DaclSize, *DaclSize) PACL Dacl,
  _Inout_ PULONG DaclSize,
  _Out_writes_bytes_to_opt_(*SaclSize, *SaclSize) PACL Sacl,
  _Inout_ PULONG SaclSize,
  _Out_writes_bytes_to_opt_(*OwnerSize, *OwnerSize) PSID Owner,
  _Inout_ PULONG OwnerSize,
  _Out_writes_bytes_to_opt_(*PrimaryGroupSize, *PrimaryGroupSize) PSID PrimaryGroup,
  _Inout_ PULONG PrimaryGroupSize);

#endif /* (NTDDI_VERSION >= NTDDI_WS03SP1) */

#if (NTDDI_VERSION >= NTDDI_VISTA)

NTSYSAPI
NTSTATUS
NTAPI
RtlNormalizeString(
  _In_ ULONG NormForm,
  _In_ PCWSTR SourceString,
  _In_ LONG SourceStringLength,
  _Out_writes_to_(*DestinationStringLength, *DestinationStringLength) PWSTR DestinationString,
  _Inout_ PLONG DestinationStringLength);

NTSYSAPI
NTSTATUS
NTAPI
RtlIsNormalizedString(
  _In_ ULONG NormForm,
  _In_ PCWSTR SourceString,
  _In_ LONG SourceStringLength,
  _Out_ PBOOLEAN Normalized);

NTSYSAPI
NTSTATUS
NTAPI
RtlIdnToAscii(
  _In_ ULONG Flags,
  _In_ PCWSTR SourceString,
  _In_ LONG SourceStringLength,
  _Out_writes_to_(*DestinationStringLength, *DestinationStringLength) PWSTR DestinationString,
  _Inout_ PLONG DestinationStringLength);

NTSYSAPI
NTSTATUS
NTAPI
RtlIdnToUnicode(
  IN ULONG Flags,
  IN PCWSTR SourceString,
  IN LONG SourceStringLength,
  OUT PWSTR DestinationString,
  IN OUT PLONG DestinationStringLength);

NTSYSAPI
NTSTATUS
NTAPI
RtlIdnToNameprepUnicode(
  _In_ ULONG Flags,
  _In_ PCWSTR SourceString,
  _In_ LONG SourceStringLength,
  _Out_writes_to_(*DestinationStringLength, *DestinationStringLength) PWSTR DestinationString,
  _Inout_ PLONG DestinationStringLength);

NTSYSAPI
NTSTATUS
NTAPI
RtlCreateServiceSid(
  _In_ PUNICODE_STRING ServiceName,
  _Out_writes_bytes_opt_(*ServiceSidLength) PSID ServiceSid,
  _Inout_ PULONG ServiceSidLength);

NTSYSAPI
LONG
NTAPI
RtlCompareAltitudes(
  _In_ PCUNICODE_STRING Altitude1,
  _In_ PCUNICODE_STRING Altitude2);


#endif /* (NTDDI_VERSION >= NTDDI_VISTA) */

#if (NTDDI_VERSION >= NTDDI_WIN7)

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSYSAPI
NTSTATUS
NTAPI
RtlUnicodeToUTF8N(
  _Out_writes_bytes_to_(UTF8StringMaxByteCount, *UTF8StringActualByteCount) PCHAR UTF8StringDestination,
  _In_ ULONG UTF8StringMaxByteCount,
  _Out_ PULONG UTF8StringActualByteCount,
  _In_reads_bytes_(UnicodeStringByteCount) PCWCH UnicodeStringSource,
  _In_ ULONG UnicodeStringByteCount);

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSYSAPI
NTSTATUS
NTAPI
RtlUTF8ToUnicodeN(
  _Out_writes_bytes_to_(UnicodeStringMaxByteCount, *UnicodeStringActualByteCount) PWSTR UnicodeStringDestination,
  _In_ ULONG UnicodeStringMaxByteCount,
  _Out_ PULONG UnicodeStringActualByteCount,
  _In_reads_bytes_(UTF8StringByteCount) PCCH UTF8StringSource,
  _In_ ULONG UTF8StringByteCount);

_IRQL_requires_max_(APC_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
RtlReplaceSidInSd(
  _Inout_ PSECURITY_DESCRIPTOR SecurityDescriptor,
  _In_ PSID OldSid,
  _In_ PSID NewSid,
  _Out_ ULONG *NumChanges);

NTSYSAPI
NTSTATUS
NTAPI
RtlCreateVirtualAccountSid(
  _In_ PCUNICODE_STRING Name,
  _In_ ULONG BaseSubAuthority,
  _Out_writes_bytes_(*SidLength) PSID Sid,
  _Inout_ PULONG SidLength);

#endif /* (NTDDI_VERSION >= NTDDI_WIN7) */


#if defined(_AMD64_) || defined(_IA64_)



#endif /* defined(_AMD64_) || defined(_IA64_) */



#define RTL_DUPLICATE_UNICODE_STRING_NULL_TERMINATE 1
#define RTL_DUPLICATE_UNICODE_STRING_ALLOCATE_NULL_STRING 2

#define RtlUnicodeStringToOemSize(STRING) (NLS_MB_OEM_CODE_PAGE_TAG ?                                \
                                           RtlxUnicodeStringToOemSize(STRING) :                      \
                                           ((STRING)->Length + sizeof(UNICODE_NULL)) / sizeof(WCHAR) \
)

#define RtlOemStringToUnicodeSize(STRING) (                 \
    NLS_MB_OEM_CODE_PAGE_TAG ?                              \
    RtlxOemStringToUnicodeSize(STRING) :                    \
    ((STRING)->Length + sizeof(ANSI_NULL)) * sizeof(WCHAR)  \
)

#define RtlOemStringToCountedUnicodeSize(STRING) (                    \
    (ULONG)(RtlOemStringToUnicodeSize(STRING) - sizeof(UNICODE_NULL)) \
)

#define RtlOffsetToPointer(B,O) ((PCHAR)(((PCHAR)(B)) + ((ULONG_PTR)(O))))
#define RtlPointerToOffset(B,P) ((ULONG)(((PCHAR)(P)) - ((PCHAR)(B))))

_IRQL_requires_max_(PASSIVE_LEVEL)
__kernel_entry
NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryObject(
  _In_opt_ HANDLE Handle,
  _In_ OBJECT_INFORMATION_CLASS ObjectInformationClass,
  _Out_writes_bytes_opt_(ObjectInformationLength) PVOID ObjectInformation,
  _In_ ULONG ObjectInformationLength,
  _Out_opt_ PULONG ReturnLength);

#if (NTDDI_VERSION >= NTDDI_WIN2K)

_Must_inspect_result_
__kernel_entry
NTSYSCALLAPI
NTSTATUS
NTAPI
NtOpenThreadToken(
  _In_ HANDLE ThreadHandle,
  _In_ ACCESS_MASK DesiredAccess,
  _In_ BOOLEAN OpenAsSelf,
  _Out_ PHANDLE TokenHandle);

_Must_inspect_result_
__kernel_entry
NTSYSCALLAPI
NTSTATUS
NTAPI
NtOpenProcessToken(
  _In_ HANDLE ProcessHandle,
  _In_ ACCESS_MASK DesiredAccess,
  _Out_ PHANDLE TokenHandle);

_When_(TokenInformationClass == TokenAccessInformation,
  _At_(TokenInformationLength,
       _In_range_(>=, sizeof(TOKEN_ACCESS_INFORMATION))))
_Must_inspect_result_
__kernel_entry
NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryInformationToken(
  _In_ HANDLE TokenHandle,
  _In_ TOKEN_INFORMATION_CLASS TokenInformationClass,
  _Out_writes_bytes_to_opt_(TokenInformationLength, *ReturnLength) PVOID TokenInformation,
  _In_ ULONG TokenInformationLength,
  _Out_ PULONG ReturnLength);

_Must_inspect_result_
__kernel_entry
NTSYSCALLAPI
NTSTATUS
NTAPI
NtAdjustPrivilegesToken(
    _In_ HANDLE TokenHandle,
    _In_ BOOLEAN DisableAllPrivileges,
    _In_opt_ PTOKEN_PRIVILEGES NewState,
    _In_ ULONG BufferLength,
    _Out_writes_bytes_to_opt_(BufferLength, *ReturnLength) PTOKEN_PRIVILEGES PreviousState,
    _When_(PreviousState != NULL, _Out_) PULONG ReturnLength);

__kernel_entry
NTSYSCALLAPI
NTSTATUS
NTAPI
NtCreateFile(
  _Out_ PHANDLE FileHandle,
  _In_ ACCESS_MASK DesiredAccess,
  _In_ POBJECT_ATTRIBUTES ObjectAttributes,
  _Out_ PIO_STATUS_BLOCK IoStatusBlock,
  _In_opt_ PLARGE_INTEGER AllocationSize,
  _In_ ULONG FileAttributes,
  _In_ ULONG ShareAccess,
  _In_ ULONG CreateDisposition,
  _In_ ULONG CreateOptions,
  _In_reads_bytes_opt_(EaLength) PVOID EaBuffer,
  _In_ ULONG EaLength);

__kernel_entry
NTSYSCALLAPI
NTSTATUS
NTAPI
NtDeviceIoControlFile(
  _In_ HANDLE FileHandle,
  _In_opt_ HANDLE Event,
  _In_opt_ PIO_APC_ROUTINE ApcRoutine,
  _In_opt_ PVOID ApcContext,
  _Out_ PIO_STATUS_BLOCK IoStatusBlock,
  _In_ ULONG IoControlCode,
  _In_reads_bytes_opt_(InputBufferLength) PVOID InputBuffer,
  _In_ ULONG InputBufferLength,
  _Out_writes_bytes_opt_(OutputBufferLength) PVOID OutputBuffer,
  _In_ ULONG OutputBufferLength);

__kernel_entry
NTSYSCALLAPI
NTSTATUS
NTAPI
NtFsControlFile(
  _In_ HANDLE FileHandle,
  _In_opt_ HANDLE Event,
  _In_opt_ PIO_APC_ROUTINE ApcRoutine,
  _In_opt_ PVOID ApcContext,
  _Out_ PIO_STATUS_BLOCK IoStatusBlock,
  _In_ ULONG FsControlCode,
  _In_reads_bytes_opt_(InputBufferLength) PVOID InputBuffer,
  _In_ ULONG InputBufferLength,
  _Out_writes_bytes_opt_(OutputBufferLength) PVOID OutputBuffer,
  _In_ ULONG OutputBufferLength);

__kernel_entry
NTSYSCALLAPI
NTSTATUS
NTAPI
NtLockFile(
  _In_ HANDLE FileHandle,
  _In_opt_ HANDLE Event,
  _In_opt_ PIO_APC_ROUTINE ApcRoutine,
  _In_opt_ PVOID ApcContext,
  _Out_ PIO_STATUS_BLOCK IoStatusBlock,
  _In_ PLARGE_INTEGER ByteOffset,
  _In_ PLARGE_INTEGER Length,
  _In_ ULONG Key,
  _In_ BOOLEAN FailImmediately,
  _In_ BOOLEAN ExclusiveLock);

__kernel_entry
NTSYSCALLAPI
NTSTATUS
NTAPI
NtOpenFile(
  _Out_ PHANDLE FileHandle,
  _In_ ACCESS_MASK DesiredAccess,
  _In_ POBJECT_ATTRIBUTES ObjectAttributes,
  _Out_ PIO_STATUS_BLOCK IoStatusBlock,
  _In_ ULONG ShareAccess,
  _In_ ULONG OpenOptions);

__kernel_entry
NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryDirectoryFile(
  _In_ HANDLE FileHandle,
  _In_opt_ HANDLE Event,
  _In_opt_ PIO_APC_ROUTINE ApcRoutine,
  _In_opt_ PVOID ApcContext,
  _Out_ PIO_STATUS_BLOCK IoStatusBlock,
  _Out_writes_bytes_(Length) PVOID FileInformation,
  _In_ ULONG Length,
  _In_ FILE_INFORMATION_CLASS FileInformationClass,
  _In_ BOOLEAN ReturnSingleEntry,
  _In_opt_ PUNICODE_STRING FileName,
  _In_ BOOLEAN RestartScan);

__kernel_entry
NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryInformationFile(
  _In_ HANDLE FileHandle,
  _Out_ PIO_STATUS_BLOCK IoStatusBlock,
  _Out_writes_bytes_(Length) PVOID FileInformation,
  _In_ ULONG Length,
  _In_ FILE_INFORMATION_CLASS FileInformationClass);

__kernel_entry
NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryQuotaInformationFile(
  _In_ HANDLE FileHandle,
  _Out_ PIO_STATUS_BLOCK IoStatusBlock,
  _Out_writes_bytes_(Length) PVOID Buffer,
  _In_ ULONG Length,
  _In_ BOOLEAN ReturnSingleEntry,
  _In_reads_bytes_opt_(SidListLength) PVOID SidList,
  _In_ ULONG SidListLength,
  _In_reads_bytes_opt_((8 + (4 * ((SID *)StartSid)->SubAuthorityCount))) PSID StartSid,
  _In_ BOOLEAN RestartScan);

__kernel_entry
NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryVolumeInformationFile(
  _In_ HANDLE FileHandle,
  _Out_ PIO_STATUS_BLOCK IoStatusBlock,
  _Out_writes_bytes_(Length) PVOID FsInformation,
  _In_ ULONG Length,
  _In_ FS_INFORMATION_CLASS FsInformationClass);

__kernel_entry
NTSYSCALLAPI
NTSTATUS
NTAPI
NtReadFile(
  _In_ HANDLE FileHandle,
  _In_opt_ HANDLE Event,
  _In_opt_ PIO_APC_ROUTINE ApcRoutine,
  _In_opt_ PVOID ApcContext,
  _Out_ PIO_STATUS_BLOCK IoStatusBlock,
  _Out_writes_bytes_(Length) PVOID Buffer,
  _In_ ULONG Length,
  _In_opt_ PLARGE_INTEGER ByteOffset,
  _In_opt_ PULONG Key);

__kernel_entry
NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetInformationFile(
  _In_ HANDLE FileHandle,
  _Out_ PIO_STATUS_BLOCK IoStatusBlock,
  _In_reads_bytes_(Length) PVOID FileInformation,
  _In_ ULONG Length,
  _In_ FILE_INFORMATION_CLASS FileInformationClass);

__kernel_entry
NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetQuotaInformationFile(
  _In_ HANDLE FileHandle,
  _Out_ PIO_STATUS_BLOCK IoStatusBlock,
  _In_reads_bytes_(Length) PVOID Buffer,
  _In_ ULONG Length);

__kernel_entry
NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetVolumeInformationFile(
  _In_ HANDLE FileHandle,
  _Out_ PIO_STATUS_BLOCK IoStatusBlock,
  _In_reads_bytes_(Length) PVOID FsInformation,
  _In_ ULONG Length,
  _In_ FS_INFORMATION_CLASS FsInformationClass);

__kernel_entry
NTSYSCALLAPI
NTSTATUS
NTAPI
NtWriteFile(
  _In_ HANDLE FileHandle,
  _In_opt_ HANDLE Event,
  _In_opt_ PIO_APC_ROUTINE ApcRoutine,
  _In_opt_ PVOID ApcContext,
  _Out_ PIO_STATUS_BLOCK IoStatusBlock,
  _In_reads_bytes_(Length) PVOID Buffer,
  _In_ ULONG Length,
  _In_opt_ PLARGE_INTEGER ByteOffset,
  _In_opt_ PULONG Key);

__kernel_entry
NTSYSCALLAPI
NTSTATUS
NTAPI
NtUnlockFile(
  _In_ HANDLE FileHandle,
  _Out_ PIO_STATUS_BLOCK IoStatusBlock,
  _In_ PLARGE_INTEGER ByteOffset,
  _In_ PLARGE_INTEGER Length,
  _In_ ULONG Key);

_IRQL_requires_max_(PASSIVE_LEVEL)
__kernel_entry
NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetSecurityObject(
  _In_ HANDLE Handle,
  _In_ SECURITY_INFORMATION SecurityInformation,
  _In_ PSECURITY_DESCRIPTOR SecurityDescriptor);

_IRQL_requires_max_(PASSIVE_LEVEL)
__kernel_entry
NTSYSCALLAPI
NTSTATUS
NTAPI
NtQuerySecurityObject(
  _In_ HANDLE Handle,
  _In_ SECURITY_INFORMATION SecurityInformation,
  _Out_writes_bytes_opt_(Length) PSECURITY_DESCRIPTOR SecurityDescriptor,
  _In_ ULONG Length,
  _Out_ PULONG LengthNeeded);

_IRQL_requires_max_(PASSIVE_LEVEL)
__kernel_entry
NTSYSCALLAPI
NTSTATUS
NTAPI
NtClose(
  _In_ HANDLE Handle);

#endif

#if (NTDDI_VERSION >= NTDDI_WINXP)

_Must_inspect_result_
__kernel_entry
NTSYSCALLAPI
NTSTATUS
NTAPI
NtOpenThreadTokenEx(
  _In_ HANDLE ThreadHandle,
  _In_ ACCESS_MASK DesiredAccess,
  _In_ BOOLEAN OpenAsSelf,
  _In_ ULONG HandleAttributes,
  _Out_ PHANDLE TokenHandle);

_Must_inspect_result_
__kernel_entry
NTSYSCALLAPI
NTSTATUS
NTAPI
NtOpenProcessTokenEx(
  _In_ HANDLE ProcessHandle,
  _In_ ACCESS_MASK DesiredAccess,
  _In_ ULONG HandleAttributes,
  _Out_ PHANDLE TokenHandle);

_Must_inspect_result_
NTSYSAPI
NTSTATUS
NTAPI
NtOpenJobObjectToken(
  _In_ HANDLE JobHandle,
  _In_ ACCESS_MASK DesiredAccess,
  _Out_ PHANDLE TokenHandle);

_Must_inspect_result_
__kernel_entry
NTSYSCALLAPI
NTSTATUS
NTAPI
NtDuplicateToken(
  _In_ HANDLE ExistingTokenHandle,
  _In_ ACCESS_MASK DesiredAccess,
  _In_opt_ POBJECT_ATTRIBUTES ObjectAttributes,
  _In_ BOOLEAN EffectiveOnly,
  _In_ TOKEN_TYPE TokenType,
  _Out_ PHANDLE NewTokenHandle);

_Must_inspect_result_
__kernel_entry
NTSYSCALLAPI
NTSTATUS
NTAPI
NtFilterToken(
  _In_ HANDLE ExistingTokenHandle,
  _In_ ULONG Flags,
  _In_opt_ PTOKEN_GROUPS SidsToDisable,
  _In_opt_ PTOKEN_PRIVILEGES PrivilegesToDelete,
  _In_opt_ PTOKEN_GROUPS RestrictedSids,
  _Out_ PHANDLE NewTokenHandle);

_Must_inspect_result_
__kernel_entry
NTSYSCALLAPI
NTSTATUS
NTAPI
NtImpersonateAnonymousToken(
  _In_ HANDLE ThreadHandle);

_Must_inspect_result_
__kernel_entry
NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetInformationToken(
  _In_ HANDLE TokenHandle,
  _In_ TOKEN_INFORMATION_CLASS TokenInformationClass,
  _In_reads_bytes_(TokenInformationLength) PVOID TokenInformation,
  _In_ ULONG TokenInformationLength);

_Must_inspect_result_
__kernel_entry
NTSYSCALLAPI
NTSTATUS
NTAPI
NtAdjustGroupsToken(
  _In_ HANDLE TokenHandle,
  _In_ BOOLEAN ResetToDefault,
  _In_opt_ PTOKEN_GROUPS NewState,
  _In_opt_ ULONG BufferLength,
  _Out_writes_bytes_to_opt_(BufferLength, *ReturnLength) PTOKEN_GROUPS PreviousState,
  _Out_ PULONG ReturnLength);

_Must_inspect_result_
__kernel_entry
NTSYSCALLAPI
NTSTATUS
NTAPI
NtPrivilegeCheck(
  _In_ HANDLE ClientToken,
  _Inout_ PPRIVILEGE_SET RequiredPrivileges,
  _Out_ PBOOLEAN Result);

_Must_inspect_result_
__kernel_entry
NTSYSCALLAPI
NTSTATUS
NTAPI
NtAccessCheckAndAuditAlarm(
  _In_ PUNICODE_STRING SubsystemName,
  _In_opt_ PVOID HandleId,
  _In_ PUNICODE_STRING ObjectTypeName,
  _In_ PUNICODE_STRING ObjectName,
  _In_ PSECURITY_DESCRIPTOR SecurityDescriptor,
  _In_ ACCESS_MASK DesiredAccess,
  _In_ PGENERIC_MAPPING GenericMapping,
  _In_ BOOLEAN ObjectCreation,
  _Out_ PACCESS_MASK GrantedAccess,
  _Out_ PNTSTATUS AccessStatus,
  _Out_ PBOOLEAN GenerateOnClose);

_Must_inspect_result_
__kernel_entry
NTSYSCALLAPI
NTSTATUS
NTAPI
NtAccessCheckByTypeAndAuditAlarm(
  _In_ PUNICODE_STRING SubsystemName,
  _In_opt_ PVOID HandleId,
  _In_ PUNICODE_STRING ObjectTypeName,
  _In_ PUNICODE_STRING ObjectName,
  _In_ PSECURITY_DESCRIPTOR SecurityDescriptor,
  _In_opt_ PSID PrincipalSelfSid,
  _In_ ACCESS_MASK DesiredAccess,
  _In_ AUDIT_EVENT_TYPE AuditType,
  _In_ ULONG Flags,
  _In_reads_opt_(ObjectTypeLength) POBJECT_TYPE_LIST ObjectTypeList,
  _In_ ULONG ObjectTypeLength,
  _In_ PGENERIC_MAPPING GenericMapping,
  _In_ BOOLEAN ObjectCreation,
  _Out_ PACCESS_MASK GrantedAccess,
  _Out_ PNTSTATUS AccessStatus,
  _Out_ PBOOLEAN GenerateOnClose);

_Must_inspect_result_
__kernel_entry
NTSYSCALLAPI
NTSTATUS
NTAPI
NtAccessCheckByTypeResultListAndAuditAlarm(
  _In_ PUNICODE_STRING SubsystemName,
  _In_opt_ PVOID HandleId,
  _In_ PUNICODE_STRING ObjectTypeName,
  _In_ PUNICODE_STRING ObjectName,
  _In_ PSECURITY_DESCRIPTOR SecurityDescriptor,
  _In_opt_ PSID PrincipalSelfSid,
  _In_ ACCESS_MASK DesiredAccess,
  _In_ AUDIT_EVENT_TYPE AuditType,
  _In_ ULONG Flags,
  _In_reads_opt_(ObjectTypeListLength) POBJECT_TYPE_LIST ObjectTypeList,
  _In_ ULONG ObjectTypeListLength,
  _In_ PGENERIC_MAPPING GenericMapping,
  _In_ BOOLEAN ObjectCreation,
  _Out_writes_(ObjectTypeListLength) PACCESS_MASK GrantedAccess,
  _Out_writes_(ObjectTypeListLength) PNTSTATUS AccessStatus,
  _Out_ PBOOLEAN GenerateOnClose);

_Must_inspect_result_
__kernel_entry
NTSYSCALLAPI
NTSTATUS
NTAPI
NtAccessCheckByTypeResultListAndAuditAlarmByHandle(
  _In_ PUNICODE_STRING SubsystemName,
  _In_opt_ PVOID HandleId,
  _In_ HANDLE ClientToken,
  _In_ PUNICODE_STRING ObjectTypeName,
  _In_ PUNICODE_STRING ObjectName,
  _In_ PSECURITY_DESCRIPTOR SecurityDescriptor,
  _In_opt_ PSID PrincipalSelfSid,
  _In_ ACCESS_MASK DesiredAccess,
  _In_ AUDIT_EVENT_TYPE AuditType,
  _In_ ULONG Flags,
  _In_reads_opt_(ObjectTypeListLength) POBJECT_TYPE_LIST ObjectTypeList,
  _In_ ULONG ObjectTypeListLength,
  _In_ PGENERIC_MAPPING GenericMapping,
  _In_ BOOLEAN ObjectCreation,
  _Out_writes_(ObjectTypeListLength) PACCESS_MASK GrantedAccess,
  _Out_writes_(ObjectTypeListLength) PNTSTATUS AccessStatus,
  _Out_ PBOOLEAN GenerateOnClose);

__kernel_entry
NTSYSCALLAPI
NTSTATUS
NTAPI
NtOpenObjectAuditAlarm(
  _In_ PUNICODE_STRING SubsystemName,
  _In_opt_ PVOID HandleId,
  _In_ PUNICODE_STRING ObjectTypeName,
  _In_ PUNICODE_STRING ObjectName,
  _In_opt_ PSECURITY_DESCRIPTOR SecurityDescriptor,
  _In_ HANDLE ClientToken,
  _In_ ACCESS_MASK DesiredAccess,
  _In_ ACCESS_MASK GrantedAccess,
  _In_opt_ PPRIVILEGE_SET Privileges,
  _In_ BOOLEAN ObjectCreation,
  _In_ BOOLEAN AccessGranted,
  _Out_ PBOOLEAN GenerateOnClose);

__kernel_entry
NTSYSCALLAPI
NTSTATUS
NTAPI
NtPrivilegeObjectAuditAlarm(
  _In_ PUNICODE_STRING SubsystemName,
  _In_opt_ PVOID HandleId,
  _In_ HANDLE ClientToken,
  _In_ ACCESS_MASK DesiredAccess,
  _In_ PPRIVILEGE_SET Privileges,
  _In_ BOOLEAN AccessGranted);

__kernel_entry
NTSYSCALLAPI
NTSTATUS
NTAPI
NtCloseObjectAuditAlarm(
  _In_ PUNICODE_STRING SubsystemName,
  _In_opt_ PVOID HandleId,
  _In_ BOOLEAN GenerateOnClose);

__kernel_entry
NTSYSCALLAPI
NTSTATUS
NTAPI
NtDeleteObjectAuditAlarm(
  _In_ PUNICODE_STRING SubsystemName,
  _In_opt_ PVOID HandleId,
  _In_ BOOLEAN GenerateOnClose);

__kernel_entry
NTSYSCALLAPI
NTSTATUS
NTAPI
NtPrivilegedServiceAuditAlarm(
  _In_ PUNICODE_STRING SubsystemName,
  _In_ PUNICODE_STRING ServiceName,
  _In_ HANDLE ClientToken,
  _In_ PPRIVILEGE_SET Privileges,
  _In_ BOOLEAN AccessGranted);

__kernel_entry
NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetInformationThread(
  _In_ HANDLE ThreadHandle,
  _In_ THREADINFOCLASS ThreadInformationClass,
  _In_reads_bytes_(ThreadInformationLength) PVOID ThreadInformation,
  _In_ ULONG ThreadInformationLength);

_Must_inspect_result_
__kernel_entry
NTSYSCALLAPI
NTSTATUS
NTAPI
NtCreateSection(
  _Out_ PHANDLE SectionHandle,
  _In_ ACCESS_MASK DesiredAccess,
  _In_opt_ POBJECT_ATTRIBUTES ObjectAttributes,
  _In_opt_ PLARGE_INTEGER MaximumSize,
  _In_ ULONG SectionPageProtection,
  _In_ ULONG AllocationAttributes,
  _In_opt_ HANDLE FileHandle);

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

_IRQL_requires_same_
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
NTAPI
LsaRegisterLogonProcess(
  _In_ PLSA_STRING LogonProcessName,
  _Out_ PHANDLE LsaHandle,
  _Out_ PLSA_OPERATIONAL_MODE SecurityMode);

_IRQL_requires_same_
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
NTAPI
LsaLogonUser(
  _In_ HANDLE LsaHandle,
  _In_ PLSA_STRING OriginName,
  _In_ SECURITY_LOGON_TYPE LogonType,
  _In_ ULONG AuthenticationPackage,
  _In_reads_bytes_(AuthenticationInformationLength) PVOID AuthenticationInformation,
  _In_ ULONG AuthenticationInformationLength,
  _In_opt_ PTOKEN_GROUPS LocalGroups,
  _In_ PTOKEN_SOURCE SourceContext,
  _Out_ PVOID *ProfileBuffer,
  _Out_ PULONG ProfileBufferLength,
  _Inout_ PLUID LogonId,
  _Out_ PHANDLE Token,
  _Out_ PQUOTA_LIMITS Quotas,
  _Out_ PNTSTATUS SubStatus);

_IRQL_requires_same_
NTSTATUS
NTAPI
LsaFreeReturnBuffer(
  _In_ PVOID Buffer);

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



#define FILE_OPLOCK_BROKEN_TO_LEVEL_2   0x00000007
#define FILE_OPLOCK_BROKEN_TO_NONE      0x00000008
#define FILE_OPBATCH_BREAK_UNDERWAY     0x00000009

/* also in winnt.h */
#define FILE_NOTIFY_CHANGE_FILE_NAME    0x00000001
#define FILE_NOTIFY_CHANGE_DIR_NAME     0x00000002
#define FILE_NOTIFY_CHANGE_NAME         0x00000003
#define FILE_NOTIFY_CHANGE_ATTRIBUTES   0x00000004
#define FILE_NOTIFY_CHANGE_SIZE         0x00000008
#define FILE_NOTIFY_CHANGE_LAST_WRITE   0x00000010
#define FILE_NOTIFY_CHANGE_LAST_ACCESS  0x00000020
#define FILE_NOTIFY_CHANGE_CREATION     0x00000040
#define FILE_NOTIFY_CHANGE_EA           0x00000080
#define FILE_NOTIFY_CHANGE_SECURITY     0x00000100
#define FILE_NOTIFY_CHANGE_STREAM_NAME  0x00000200
#define FILE_NOTIFY_CHANGE_STREAM_SIZE  0x00000400
#define FILE_NOTIFY_CHANGE_STREAM_WRITE 0x00000800
#define FILE_NOTIFY_VALID_MASK          0x00000fff

#define FILE_ACTION_ADDED                   0x00000001
#define FILE_ACTION_REMOVED                 0x00000002
#define FILE_ACTION_MODIFIED                0x00000003
#define FILE_ACTION_RENAMED_OLD_NAME        0x00000004
#define FILE_ACTION_RENAMED_NEW_NAME        0x00000005
#define FILE_ACTION_ADDED_STREAM            0x00000006
#define FILE_ACTION_REMOVED_STREAM          0x00000007
#define FILE_ACTION_MODIFIED_STREAM         0x00000008
#define FILE_ACTION_REMOVED_BY_DELETE       0x00000009
#define FILE_ACTION_ID_NOT_TUNNELLED        0x0000000A
#define FILE_ACTION_TUNNELLED_ID_COLLISION  0x0000000B
/* end  winnt.h */

#define FILE_PIPE_BYTE_STREAM_TYPE          0x00000000
#define FILE_PIPE_MESSAGE_TYPE              0x00000001

#define FILE_PIPE_ACCEPT_REMOTE_CLIENTS     0x00000000
#define FILE_PIPE_REJECT_REMOTE_CLIENTS     0x00000002

#define FILE_PIPE_ACCEPT_REMOTE_CLIENTS     0x00000000
#define FILE_PIPE_REJECT_REMOTE_CLIENTS     0x00000002
#define FILE_PIPE_TYPE_VALID_MASK           0x00000003

#define FILE_PIPE_BYTE_STREAM_MODE          0x00000000
#define FILE_PIPE_MESSAGE_MODE              0x00000001

#define FILE_PIPE_QUEUE_OPERATION           0x00000000
#define FILE_PIPE_COMPLETE_OPERATION        0x00000001

#define FILE_PIPE_INBOUND                   0x00000000
#define FILE_PIPE_OUTBOUND                  0x00000001
#define FILE_PIPE_FULL_DUPLEX               0x00000002

#define FILE_PIPE_DISCONNECTED_STATE        0x00000001
#define FILE_PIPE_LISTENING_STATE           0x00000002
#define FILE_PIPE_CONNECTED_STATE           0x00000003
#define FILE_PIPE_CLOSING_STATE             0x00000004

#define FILE_PIPE_CLIENT_END                0x00000000
#define FILE_PIPE_SERVER_END                0x00000001

#define FILE_CASE_SENSITIVE_SEARCH          0x00000001
#define FILE_CASE_PRESERVED_NAMES           0x00000002
#define FILE_UNICODE_ON_DISK                0x00000004
#define FILE_PERSISTENT_ACLS                0x00000008
#define FILE_FILE_COMPRESSION               0x00000010
#define FILE_VOLUME_QUOTAS                  0x00000020
#define FILE_SUPPORTS_SPARSE_FILES          0x00000040
#define FILE_SUPPORTS_REPARSE_POINTS        0x00000080
#define FILE_SUPPORTS_REMOTE_STORAGE        0x00000100
#define FILE_VOLUME_IS_COMPRESSED           0x00008000
#define FILE_SUPPORTS_OBJECT_IDS            0x00010000
#define FILE_SUPPORTS_ENCRYPTION            0x00020000
#define FILE_NAMED_STREAMS                  0x00040000
#define FILE_READ_ONLY_VOLUME               0x00080000
#define FILE_SEQUENTIAL_WRITE_ONCE          0x00100000
#define FILE_SUPPORTS_TRANSACTIONS          0x00200000
#define FILE_SUPPORTS_HARD_LINKS            0x00400000
#define FILE_SUPPORTS_EXTENDED_ATTRIBUTES   0x00800000
#define FILE_SUPPORTS_OPEN_BY_FILE_ID       0x01000000
#define FILE_SUPPORTS_USN_JOURNAL           0x02000000

#define FILE_NEED_EA                    0x00000080

#define FILE_EA_TYPE_BINARY             0xfffe
#define FILE_EA_TYPE_ASCII              0xfffd
#define FILE_EA_TYPE_BITMAP             0xfffb
#define FILE_EA_TYPE_METAFILE           0xfffa
#define FILE_EA_TYPE_ICON               0xfff9
#define FILE_EA_TYPE_EA                 0xffee
#define FILE_EA_TYPE_MVMT               0xffdf
#define FILE_EA_TYPE_MVST               0xffde
#define FILE_EA_TYPE_ASN1               0xffdd
#define FILE_EA_TYPE_FAMILY_IDS         0xff01

typedef struct _FILE_NOTIFY_INFORMATION {
  ULONG NextEntryOffset;
  ULONG Action;
  ULONG FileNameLength;
  WCHAR FileName[1];
} FILE_NOTIFY_INFORMATION, *PFILE_NOTIFY_INFORMATION;

typedef struct _FILE_DIRECTORY_INFORMATION {
  ULONG NextEntryOffset;
  ULONG FileIndex;
  LARGE_INTEGER CreationTime;
  LARGE_INTEGER LastAccessTime;
  LARGE_INTEGER LastWriteTime;
  LARGE_INTEGER ChangeTime;
  LARGE_INTEGER EndOfFile;
  LARGE_INTEGER AllocationSize;
  ULONG FileAttributes;
  ULONG FileNameLength;
  WCHAR FileName[1];
} FILE_DIRECTORY_INFORMATION, *PFILE_DIRECTORY_INFORMATION;

typedef struct _FILE_FULL_DIR_INFORMATION {
  ULONG NextEntryOffset;
  ULONG FileIndex;
  LARGE_INTEGER CreationTime;
  LARGE_INTEGER LastAccessTime;
  LARGE_INTEGER LastWriteTime;
  LARGE_INTEGER ChangeTime;
  LARGE_INTEGER EndOfFile;
  LARGE_INTEGER AllocationSize;
  ULONG FileAttributes;
  ULONG FileNameLength;
  ULONG EaSize;
  WCHAR FileName[1];
} FILE_FULL_DIR_INFORMATION, *PFILE_FULL_DIR_INFORMATION;

typedef struct _FILE_ID_FULL_DIR_INFORMATION {
  ULONG NextEntryOffset;
  ULONG FileIndex;
  LARGE_INTEGER CreationTime;
  LARGE_INTEGER LastAccessTime;
  LARGE_INTEGER LastWriteTime;
  LARGE_INTEGER ChangeTime;
  LARGE_INTEGER EndOfFile;
  LARGE_INTEGER AllocationSize;
  ULONG FileAttributes;
  ULONG FileNameLength;
  ULONG EaSize;
  LARGE_INTEGER FileId;
  WCHAR FileName[1];
} FILE_ID_FULL_DIR_INFORMATION, *PFILE_ID_FULL_DIR_INFORMATION;

typedef struct _FILE_BOTH_DIR_INFORMATION {
  ULONG NextEntryOffset;
  ULONG FileIndex;
  LARGE_INTEGER CreationTime;
  LARGE_INTEGER LastAccessTime;
  LARGE_INTEGER LastWriteTime;
  LARGE_INTEGER ChangeTime;
  LARGE_INTEGER EndOfFile;
  LARGE_INTEGER AllocationSize;
  ULONG FileAttributes;
  ULONG FileNameLength;
  ULONG EaSize;
  CCHAR ShortNameLength;
  WCHAR ShortName[12];
  WCHAR FileName[1];
} FILE_BOTH_DIR_INFORMATION, *PFILE_BOTH_DIR_INFORMATION;

typedef struct _FILE_ID_BOTH_DIR_INFORMATION {
  ULONG NextEntryOffset;
  ULONG FileIndex;
  LARGE_INTEGER CreationTime;
  LARGE_INTEGER LastAccessTime;
  LARGE_INTEGER LastWriteTime;
  LARGE_INTEGER ChangeTime;
  LARGE_INTEGER EndOfFile;
  LARGE_INTEGER AllocationSize;
  ULONG FileAttributes;
  ULONG FileNameLength;
  ULONG EaSize;
  CCHAR ShortNameLength;
  WCHAR ShortName[12];
  LARGE_INTEGER FileId;
  WCHAR FileName[1];
} FILE_ID_BOTH_DIR_INFORMATION, *PFILE_ID_BOTH_DIR_INFORMATION;

typedef struct _FILE_NAMES_INFORMATION {
  ULONG NextEntryOffset;
  ULONG FileIndex;
  ULONG FileNameLength;
  WCHAR FileName[1];
} FILE_NAMES_INFORMATION, *PFILE_NAMES_INFORMATION;

typedef struct _FILE_ID_GLOBAL_TX_DIR_INFORMATION {
  ULONG NextEntryOffset;
  ULONG FileIndex;
  LARGE_INTEGER CreationTime;
  LARGE_INTEGER LastAccessTime;
  LARGE_INTEGER LastWriteTime;
  LARGE_INTEGER ChangeTime;
  LARGE_INTEGER EndOfFile;
  LARGE_INTEGER AllocationSize;
  ULONG FileAttributes;
  ULONG FileNameLength;
  LARGE_INTEGER FileId;
  GUID LockingTransactionId;
  ULONG TxInfoFlags;
  WCHAR FileName[1];
} FILE_ID_GLOBAL_TX_DIR_INFORMATION, *PFILE_ID_GLOBAL_TX_DIR_INFORMATION;

#define FILE_ID_GLOBAL_TX_DIR_INFO_FLAG_WRITELOCKED         0x00000001
#define FILE_ID_GLOBAL_TX_DIR_INFO_FLAG_VISIBLE_TO_TX       0x00000002
#define FILE_ID_GLOBAL_TX_DIR_INFO_FLAG_VISIBLE_OUTSIDE_TX  0x00000004

typedef struct _FILE_OBJECTID_INFORMATION {
  LONGLONG FileReference;
  UCHAR ObjectId[16];
  _ANONYMOUS_UNION union {
    _ANONYMOUS_STRUCT struct {
      UCHAR BirthVolumeId[16];
      UCHAR BirthObjectId[16];
      UCHAR DomainId[16];
    } DUMMYSTRUCTNAME;
    UCHAR ExtendedInfo[48];
  } DUMMYUNIONNAME;
} FILE_OBJECTID_INFORMATION, *PFILE_OBJECTID_INFORMATION;

#define ANSI_DOS_STAR                   ('<')
#define ANSI_DOS_QM                     ('>')
#define ANSI_DOS_DOT                    ('"')

#define DOS_STAR                        (L'<')
#define DOS_QM                          (L'>')
#define DOS_DOT                         (L'"')

typedef struct _FILE_INTERNAL_INFORMATION {
  LARGE_INTEGER IndexNumber;
} FILE_INTERNAL_INFORMATION, *PFILE_INTERNAL_INFORMATION;

typedef struct _FILE_EA_INFORMATION {
  ULONG EaSize;
} FILE_EA_INFORMATION, *PFILE_EA_INFORMATION;

typedef struct _FILE_ACCESS_INFORMATION {
  ACCESS_MASK AccessFlags;
} FILE_ACCESS_INFORMATION, *PFILE_ACCESS_INFORMATION;

typedef struct _FILE_MODE_INFORMATION {
  ULONG Mode;
} FILE_MODE_INFORMATION, *PFILE_MODE_INFORMATION;

typedef struct _FILE_ALL_INFORMATION {
  FILE_BASIC_INFORMATION BasicInformation;
  FILE_STANDARD_INFORMATION StandardInformation;
  FILE_INTERNAL_INFORMATION InternalInformation;
  FILE_EA_INFORMATION EaInformation;
  FILE_ACCESS_INFORMATION AccessInformation;
  FILE_POSITION_INFORMATION PositionInformation;
  FILE_MODE_INFORMATION ModeInformation;
  FILE_ALIGNMENT_INFORMATION AlignmentInformation;
  FILE_NAME_INFORMATION NameInformation;
} FILE_ALL_INFORMATION, *PFILE_ALL_INFORMATION;

typedef struct _FILE_ALLOCATION_INFORMATION {
  LARGE_INTEGER AllocationSize;
} FILE_ALLOCATION_INFORMATION, *PFILE_ALLOCATION_INFORMATION;

typedef struct _FILE_COMPRESSION_INFORMATION {
  LARGE_INTEGER CompressedFileSize;
  USHORT CompressionFormat;
  UCHAR CompressionUnitShift;
  UCHAR ChunkShift;
  UCHAR ClusterShift;
  UCHAR Reserved[3];
} FILE_COMPRESSION_INFORMATION, *PFILE_COMPRESSION_INFORMATION;

typedef struct _FILE_LINK_INFORMATION {
  BOOLEAN ReplaceIfExists;
  HANDLE RootDirectory;
  ULONG FileNameLength;
  WCHAR FileName[1];
} FILE_LINK_INFORMATION, *PFILE_LINK_INFORMATION;

typedef struct _FILE_MOVE_CLUSTER_INFORMATION {
  ULONG ClusterCount;
  HANDLE RootDirectory;
  ULONG FileNameLength;
  WCHAR FileName[1];
} FILE_MOVE_CLUSTER_INFORMATION, *PFILE_MOVE_CLUSTER_INFORMATION;

typedef struct _FILE_RENAME_INFORMATION {
  BOOLEAN ReplaceIfExists;
  HANDLE RootDirectory;
  ULONG FileNameLength;
  WCHAR FileName[1];
} FILE_RENAME_INFORMATION, *PFILE_RENAME_INFORMATION;

typedef struct _FILE_STREAM_INFORMATION {
  ULONG NextEntryOffset;
  ULONG StreamNameLength;
  LARGE_INTEGER StreamSize;
  LARGE_INTEGER StreamAllocationSize;
  WCHAR StreamName[1];
} FILE_STREAM_INFORMATION, *PFILE_STREAM_INFORMATION;

typedef struct _FILE_TRACKING_INFORMATION {
  HANDLE DestinationFile;
  ULONG ObjectInformationLength;
  CHAR ObjectInformation[1];
} FILE_TRACKING_INFORMATION, *PFILE_TRACKING_INFORMATION;

typedef struct _FILE_COMPLETION_INFORMATION {
  HANDLE Port;
  PVOID Key;
} FILE_COMPLETION_INFORMATION, *PFILE_COMPLETION_INFORMATION;

typedef struct _FILE_PIPE_INFORMATION {
  ULONG ReadMode;
  ULONG CompletionMode;
} FILE_PIPE_INFORMATION, *PFILE_PIPE_INFORMATION;

typedef struct _FILE_PIPE_LOCAL_INFORMATION {
  ULONG NamedPipeType;
  ULONG NamedPipeConfiguration;
  ULONG MaximumInstances;
  ULONG CurrentInstances;
  ULONG InboundQuota;
  ULONG ReadDataAvailable;
  ULONG OutboundQuota;
  ULONG WriteQuotaAvailable;
  ULONG NamedPipeState;
  ULONG NamedPipeEnd;
} FILE_PIPE_LOCAL_INFORMATION, *PFILE_PIPE_LOCAL_INFORMATION;

typedef struct _FILE_PIPE_REMOTE_INFORMATION {
  LARGE_INTEGER CollectDataTime;
  ULONG MaximumCollectionCount;
} FILE_PIPE_REMOTE_INFORMATION, *PFILE_PIPE_REMOTE_INFORMATION;

typedef struct _FILE_MAILSLOT_QUERY_INFORMATION {
  ULONG MaximumMessageSize;
  ULONG MailslotQuota;
  ULONG NextMessageSize;
  ULONG MessagesAvailable;
  LARGE_INTEGER ReadTimeout;
} FILE_MAILSLOT_QUERY_INFORMATION, *PFILE_MAILSLOT_QUERY_INFORMATION;

typedef struct _FILE_MAILSLOT_SET_INFORMATION {
  PLARGE_INTEGER ReadTimeout;
} FILE_MAILSLOT_SET_INFORMATION, *PFILE_MAILSLOT_SET_INFORMATION;

typedef struct _FILE_REPARSE_POINT_INFORMATION {
  LONGLONG FileReference;
  ULONG Tag;
} FILE_REPARSE_POINT_INFORMATION, *PFILE_REPARSE_POINT_INFORMATION;

typedef struct _FILE_LINK_ENTRY_INFORMATION {
  ULONG NextEntryOffset;
  LONGLONG ParentFileId;
  ULONG FileNameLength;
  WCHAR FileName[1];
} FILE_LINK_ENTRY_INFORMATION, *PFILE_LINK_ENTRY_INFORMATION;

typedef struct _FILE_LINKS_INFORMATION {
  ULONG BytesNeeded;
  ULONG EntriesReturned;
  FILE_LINK_ENTRY_INFORMATION Entry;
} FILE_LINKS_INFORMATION, *PFILE_LINKS_INFORMATION;

typedef struct _FILE_NETWORK_PHYSICAL_NAME_INFORMATION {
  ULONG FileNameLength;
  WCHAR FileName[1];
} FILE_NETWORK_PHYSICAL_NAME_INFORMATION, *PFILE_NETWORK_PHYSICAL_NAME_INFORMATION;

typedef struct _FILE_STANDARD_LINK_INFORMATION {
  ULONG NumberOfAccessibleLinks;
  ULONG TotalNumberOfLinks;
  BOOLEAN DeletePending;
  BOOLEAN Directory;
} FILE_STANDARD_LINK_INFORMATION, *PFILE_STANDARD_LINK_INFORMATION;

typedef struct _FILE_GET_EA_INFORMATION {
  ULONG NextEntryOffset;
  UCHAR EaNameLength;
  CHAR  EaName[1];
} FILE_GET_EA_INFORMATION, *PFILE_GET_EA_INFORMATION;

#define REMOTE_PROTOCOL_FLAG_LOOPBACK       0x00000001
#define REMOTE_PROTOCOL_FLAG_OFFLINE        0x00000002

typedef struct _FILE_REMOTE_PROTOCOL_INFORMATION {
  USHORT StructureVersion;
  USHORT StructureSize;
  ULONG  Protocol;
  USHORT ProtocolMajorVersion;
  USHORT ProtocolMinorVersion;
  USHORT ProtocolRevision;
  USHORT Reserved;
  ULONG  Flags;
  struct {
    ULONG Reserved[8];
  } GenericReserved;
  struct {
    ULONG Reserved[16];
  } ProtocolSpecificReserved;
} FILE_REMOTE_PROTOCOL_INFORMATION, *PFILE_REMOTE_PROTOCOL_INFORMATION;

typedef struct _FILE_GET_QUOTA_INFORMATION {
  ULONG NextEntryOffset;
  ULONG SidLength;
  SID Sid;
} FILE_GET_QUOTA_INFORMATION, *PFILE_GET_QUOTA_INFORMATION;

typedef struct _FILE_QUOTA_INFORMATION {
  ULONG NextEntryOffset;
  ULONG SidLength;
  LARGE_INTEGER ChangeTime;
  LARGE_INTEGER QuotaUsed;
  LARGE_INTEGER QuotaThreshold;
  LARGE_INTEGER QuotaLimit;
  SID Sid;
} FILE_QUOTA_INFORMATION, *PFILE_QUOTA_INFORMATION;

typedef struct _FILE_FS_ATTRIBUTE_INFORMATION {
  ULONG FileSystemAttributes;
  ULONG MaximumComponentNameLength;
  ULONG FileSystemNameLength;
  WCHAR FileSystemName[1];
} FILE_FS_ATTRIBUTE_INFORMATION, *PFILE_FS_ATTRIBUTE_INFORMATION;

typedef struct _FILE_FS_DRIVER_PATH_INFORMATION {
  BOOLEAN DriverInPath;
  ULONG DriverNameLength;
  WCHAR DriverName[1];
} FILE_FS_DRIVER_PATH_INFORMATION, *PFILE_FS_DRIVER_PATH_INFORMATION;

typedef struct _FILE_FS_VOLUME_FLAGS_INFORMATION {
  ULONG Flags;
} FILE_FS_VOLUME_FLAGS_INFORMATION, *PFILE_FS_VOLUME_FLAGS_INFORMATION;

#define FILE_VC_QUOTA_NONE              0x00000000
#define FILE_VC_QUOTA_TRACK             0x00000001
#define FILE_VC_QUOTA_ENFORCE           0x00000002
#define FILE_VC_QUOTA_MASK              0x00000003
#define FILE_VC_CONTENT_INDEX_DISABLED  0x00000008
#define FILE_VC_LOG_QUOTA_THRESHOLD     0x00000010
#define FILE_VC_LOG_QUOTA_LIMIT         0x00000020
#define FILE_VC_LOG_VOLUME_THRESHOLD    0x00000040
#define FILE_VC_LOG_VOLUME_LIMIT        0x00000080
#define FILE_VC_QUOTAS_INCOMPLETE       0x00000100
#define FILE_VC_QUOTAS_REBUILDING       0x00000200
#define FILE_VC_VALID_MASK              0x000003ff

typedef struct _FILE_FS_CONTROL_INFORMATION {
  LARGE_INTEGER FreeSpaceStartFiltering;
  LARGE_INTEGER FreeSpaceThreshold;
  LARGE_INTEGER FreeSpaceStopFiltering;
  LARGE_INTEGER DefaultQuotaThreshold;
  LARGE_INTEGER DefaultQuotaLimit;
  ULONG FileSystemControlFlags;
} FILE_FS_CONTROL_INFORMATION, *PFILE_FS_CONTROL_INFORMATION;

#ifndef _FILESYSTEMFSCTL_
#define _FILESYSTEMFSCTL_

#define FSCTL_REQUEST_OPLOCK_LEVEL_1    CTL_CODE(FILE_DEVICE_FILE_SYSTEM,  0, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_REQUEST_OPLOCK_LEVEL_2    CTL_CODE(FILE_DEVICE_FILE_SYSTEM,  1, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_REQUEST_BATCH_OPLOCK      CTL_CODE(FILE_DEVICE_FILE_SYSTEM,  2, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_OPLOCK_BREAK_ACKNOWLEDGE  CTL_CODE(FILE_DEVICE_FILE_SYSTEM,  3, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_OPBATCH_ACK_CLOSE_PENDING CTL_CODE(FILE_DEVICE_FILE_SYSTEM,  4, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_OPLOCK_BREAK_NOTIFY       CTL_CODE(FILE_DEVICE_FILE_SYSTEM,  5, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_LOCK_VOLUME               CTL_CODE(FILE_DEVICE_FILE_SYSTEM,  6, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_UNLOCK_VOLUME             CTL_CODE(FILE_DEVICE_FILE_SYSTEM,  7, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_DISMOUNT_VOLUME           CTL_CODE(FILE_DEVICE_FILE_SYSTEM,  8, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_IS_VOLUME_MOUNTED         CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 10, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_IS_PATHNAME_VALID         CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 11, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_MARK_VOLUME_DIRTY         CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 12, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_QUERY_RETRIEVAL_POINTERS  CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 14, METHOD_NEITHER,  FILE_ANY_ACCESS)
#define FSCTL_GET_COMPRESSION           CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 15, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_SET_COMPRESSION           CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 16, METHOD_BUFFERED, FILE_READ_DATA | FILE_WRITE_DATA)
#define FSCTL_SET_BOOTLOADER_ACCESSED   CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 19, METHOD_NEITHER,  FILE_ANY_ACCESS)

#define FSCTL_OPLOCK_BREAK_ACK_NO_2     CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 20, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_INVALIDATE_VOLUMES        CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 21, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_QUERY_FAT_BPB             CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 22, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_REQUEST_FILTER_OPLOCK     CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 23, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_FILESYSTEM_GET_STATISTICS CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 24, METHOD_BUFFERED, FILE_ANY_ACCESS)

#if (_WIN32_WINNT >= 0x0400)

#define FSCTL_GET_NTFS_VOLUME_DATA      CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 25, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_GET_NTFS_FILE_RECORD      CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 26, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_GET_VOLUME_BITMAP         CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 27, METHOD_NEITHER,  FILE_ANY_ACCESS)
#define FSCTL_GET_RETRIEVAL_POINTERS    CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 28, METHOD_NEITHER,  FILE_ANY_ACCESS)
#define FSCTL_MOVE_FILE                 CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 29, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)
#define FSCTL_IS_VOLUME_DIRTY           CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 30, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_ALLOW_EXTENDED_DASD_IO    CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 32, METHOD_NEITHER,  FILE_ANY_ACCESS)

#endif

#if (_WIN32_WINNT >= 0x0500)

#define FSCTL_FIND_FILES_BY_SID         CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 35, METHOD_NEITHER,  FILE_ANY_ACCESS)
#define FSCTL_SET_OBJECT_ID             CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 38, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)
#define FSCTL_GET_OBJECT_ID             CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 39, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_DELETE_OBJECT_ID          CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 40, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)
#define FSCTL_SET_REPARSE_POINT         CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 41, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)
#define FSCTL_GET_REPARSE_POINT         CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 42, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_DELETE_REPARSE_POINT      CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 43, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)
#define FSCTL_ENUM_USN_DATA             CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 44, METHOD_NEITHER,  FILE_ANY_ACCESS)
#define FSCTL_SECURITY_ID_CHECK         CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 45, METHOD_NEITHER,  FILE_READ_DATA)
#define FSCTL_READ_USN_JOURNAL          CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 46, METHOD_NEITHER,  FILE_ANY_ACCESS)
#define FSCTL_SET_OBJECT_ID_EXTENDED    CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 47, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)
#define FSCTL_CREATE_OR_GET_OBJECT_ID   CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 48, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_SET_SPARSE                CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 49, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)
#define FSCTL_SET_ZERO_DATA             CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 50, METHOD_BUFFERED, FILE_WRITE_DATA)
#define FSCTL_QUERY_ALLOCATED_RANGES    CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 51, METHOD_NEITHER,  FILE_READ_DATA)
#define FSCTL_ENABLE_UPGRADE            CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 52, METHOD_BUFFERED, FILE_WRITE_DATA)
#define FSCTL_SET_ENCRYPTION            CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 53, METHOD_NEITHER,  FILE_ANY_ACCESS)
#define FSCTL_ENCRYPTION_FSCTL_IO       CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 54, METHOD_NEITHER,  FILE_ANY_ACCESS)
#define FSCTL_WRITE_RAW_ENCRYPTED       CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 55, METHOD_NEITHER,  FILE_SPECIAL_ACCESS)
#define FSCTL_READ_RAW_ENCRYPTED        CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 56, METHOD_NEITHER,  FILE_SPECIAL_ACCESS)
#define FSCTL_CREATE_USN_JOURNAL        CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 57, METHOD_NEITHER,  FILE_ANY_ACCESS)
#define FSCTL_READ_FILE_USN_DATA        CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 58, METHOD_NEITHER,  FILE_ANY_ACCESS)
#define FSCTL_WRITE_USN_CLOSE_RECORD    CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 59, METHOD_NEITHER,  FILE_ANY_ACCESS)
#define FSCTL_EXTEND_VOLUME             CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 60, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_QUERY_USN_JOURNAL         CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 61, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_DELETE_USN_JOURNAL        CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 62, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_MARK_HANDLE               CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 63, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_SIS_COPYFILE              CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 64, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_SIS_LINK_FILES            CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 65, METHOD_BUFFERED, FILE_READ_DATA | FILE_WRITE_DATA)
#define FSCTL_RECALL_FILE               CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 69, METHOD_NEITHER, FILE_ANY_ACCESS)
#define FSCTL_READ_FROM_PLEX            CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 71, METHOD_OUT_DIRECT, FILE_READ_DATA)
#define FSCTL_FILE_PREFETCH             CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 72, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)

#endif

#if (_WIN32_WINNT >= 0x0600)

#define FSCTL_MAKE_MEDIA_COMPATIBLE         CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 76, METHOD_BUFFERED, FILE_WRITE_DATA)
#define FSCTL_SET_DEFECT_MANAGEMENT         CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 77, METHOD_BUFFERED, FILE_WRITE_DATA)
#define FSCTL_QUERY_SPARING_INFO            CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 78, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_QUERY_ON_DISK_VOLUME_INFO     CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 79, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_SET_VOLUME_COMPRESSION_STATE  CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 80, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)
#define FSCTL_TXFS_MODIFY_RM                CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 81, METHOD_BUFFERED, FILE_WRITE_DATA)
#define FSCTL_TXFS_QUERY_RM_INFORMATION     CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 82, METHOD_BUFFERED, FILE_READ_DATA)
#define FSCTL_TXFS_ROLLFORWARD_REDO         CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 84, METHOD_BUFFERED, FILE_WRITE_DATA)
#define FSCTL_TXFS_ROLLFORWARD_UNDO         CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 85, METHOD_BUFFERED, FILE_WRITE_DATA)
#define FSCTL_TXFS_START_RM                 CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 86, METHOD_BUFFERED, FILE_WRITE_DATA)
#define FSCTL_TXFS_SHUTDOWN_RM              CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 87, METHOD_BUFFERED, FILE_WRITE_DATA)
#define FSCTL_TXFS_READ_BACKUP_INFORMATION  CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 88, METHOD_BUFFERED, FILE_READ_DATA)
#define FSCTL_TXFS_WRITE_BACKUP_INFORMATION CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 89, METHOD_BUFFERED, FILE_WRITE_DATA)
#define FSCTL_TXFS_CREATE_SECONDARY_RM      CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 90, METHOD_BUFFERED, FILE_WRITE_DATA)
#define FSCTL_TXFS_GET_METADATA_INFO        CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 91, METHOD_BUFFERED, FILE_READ_DATA)
#define FSCTL_TXFS_GET_TRANSACTED_VERSION   CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 92, METHOD_BUFFERED, FILE_READ_DATA)
#define FSCTL_TXFS_SAVEPOINT_INFORMATION    CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 94, METHOD_BUFFERED, FILE_WRITE_DATA)
#define FSCTL_TXFS_CREATE_MINIVERSION       CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 95, METHOD_BUFFERED, FILE_WRITE_DATA)
#define FSCTL_TXFS_TRANSACTION_ACTIVE       CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 99, METHOD_BUFFERED, FILE_READ_DATA)
#define FSCTL_SET_ZERO_ON_DEALLOCATION      CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 101, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)
#define FSCTL_SET_REPAIR                    CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 102, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_GET_REPAIR                    CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 103, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_WAIT_FOR_REPAIR               CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 104, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_INITIATE_REPAIR               CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 106, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_CSC_INTERNAL                  CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 107, METHOD_NEITHER,  FILE_ANY_ACCESS)
#define FSCTL_SHRINK_VOLUME                 CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 108, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)
#define FSCTL_SET_SHORT_NAME_BEHAVIOR       CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 109, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_DFSR_SET_GHOST_HANDLE_STATE   CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 110, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_TXFS_LIST_TRANSACTION_LOCKED_FILES \
                                            CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 120, METHOD_BUFFERED, FILE_READ_DATA)
#define FSCTL_TXFS_LIST_TRANSACTIONS        CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 121, METHOD_BUFFERED, FILE_READ_DATA)
#define FSCTL_QUERY_PAGEFILE_ENCRYPTION     CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 122, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_RESET_VOLUME_ALLOCATION_HINTS CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 123, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_TXFS_READ_BACKUP_INFORMATION2 CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 126, METHOD_BUFFERED, FILE_ANY_ACCESS)

#endif

#if (_WIN32_WINNT >= 0x0601)

#define FSCTL_QUERY_DEPENDENT_VOLUME        CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 124, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_SD_GLOBAL_CHANGE              CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 125, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_LOOKUP_STREAM_FROM_CLUSTER    CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 127, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_TXFS_WRITE_BACKUP_INFORMATION2 CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 128, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_FILE_TYPE_NOTIFICATION        CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 129, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_GET_BOOT_AREA_INFO            CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 140, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_GET_RETRIEVAL_POINTER_BASE    CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 141, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_SET_PERSISTENT_VOLUME_STATE   CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 142, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_QUERY_PERSISTENT_VOLUME_STATE CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 143, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_REQUEST_OPLOCK                CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 144, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_CSV_TUNNEL_REQUEST            CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 145, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_IS_CSV_FILE                   CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 146, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_QUERY_FILE_SYSTEM_RECOGNITION CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 147, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_CSV_GET_VOLUME_PATH_NAME      CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 148, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_CSV_GET_VOLUME_NAME_FOR_VOLUME_MOUNT_POINT CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 149, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_CSV_GET_VOLUME_PATH_NAMES_FOR_VOLUME_NAME CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 150,  METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_IS_FILE_ON_CSV_VOLUME         CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 151,  METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_CSV_INTERNAL                  CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 155,  METHOD_BUFFERED, FILE_ANY_ACCESS)

typedef struct _CSV_NAMESPACE_INFO {
  ULONG Version;
  ULONG DeviceNumber;
  LARGE_INTEGER StartingOffset;
  ULONG SectorSize;
} CSV_NAMESPACE_INFO, *PCSV_NAMESPACE_INFO;

#define CSV_NAMESPACE_INFO_V1 (sizeof(CSV_NAMESPACE_INFO))
#define CSV_INVALID_DEVICE_NUMBER 0xFFFFFFFF

#endif

#if (_WIN32_WINNT >= 0x0602)

#define FSCTL_FILE_LEVEL_TRIM               CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 130, METHOD_BUFFERED, FILE_WRITE_DATA)
#define FSCTL_CORRUPTION_HANDLING           CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 152, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_OFFLOAD_READ                  CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 153, METHOD_BUFFERED, FILE_READ_ACCESS)
#define FSCTL_OFFLOAD_WRITE                 CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 154, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define FSCTL_SET_PURGE_FAILURE_MODE        CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 156, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_QUERY_FILE_LAYOUT             CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 157, METHOD_NEITHER,  FILE_ANY_ACCESS)
#define FSCTL_IS_VOLUME_OWNED_BYCSVFS       CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 158, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_GET_INTEGRITY_INFORMATION     CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 159, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_SET_INTEGRITY_INFORMATION     CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 160, METHOD_BUFFERED, FILE_READ_DATA | FILE_WRITE_DATA)
#define FSCTL_QUERY_FILE_REGIONS            CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 161, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_DEDUP_FILE                    CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 165, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_DEDUP_QUERY_FILE_HASHES       CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 166, METHOD_NEITHER,  FILE_READ_DATA)
#define FSCTL_RKF_INTERNAL                  CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 171, METHOD_NEITHER,  FILE_ANY_ACCESS)
#define FSCTL_SCRUB_DATA                    CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 172, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_REPAIR_COPIES                 CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 173, METHOD_BUFFERED, FILE_READ_DATA | FILE_WRITE_DATA)
#define FSCTL_DISABLE_LOCAL_BUFFERING       CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 174, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_CSV_MGMT_LOCK                 CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 175, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_CSV_QUERY_DOWN_LEVEL_FILE_SYSTEM_CHARACTERISTICS \
                                            CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 176, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_ADVANCE_FILE_ID               CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 177, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_CSV_SYNC_TUNNEL_REQUEST       CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 178, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_CSV_QUERY_VETO_FILE_DIRECT_IO CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 179, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_WRITE_USN_REASON              CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 180, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_CSV_CONTROL                   CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 181, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_GET_REFS_VOLUME_DATA          CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 182, METHOD_BUFFERED, FILE_ANY_ACCESS)

#endif

#define FSCTL_MARK_AS_SYSTEM_HIVE           FSCTL_SET_BOOTLOADER_ACCESSED

typedef struct _PATHNAME_BUFFER {
  ULONG PathNameLength;
  WCHAR Name[1];
} PATHNAME_BUFFER, *PPATHNAME_BUFFER;

typedef struct _FSCTL_QUERY_FAT_BPB_BUFFER {
  UCHAR First0x24BytesOfBootSector[0x24];
} FSCTL_QUERY_FAT_BPB_BUFFER, *PFSCTL_QUERY_FAT_BPB_BUFFER;

#if (_WIN32_WINNT >= 0x0400)

typedef struct _NTFS_VOLUME_DATA_BUFFER {
  LARGE_INTEGER VolumeSerialNumber;
  LARGE_INTEGER NumberSectors;
  LARGE_INTEGER TotalClusters;
  LARGE_INTEGER FreeClusters;
  LARGE_INTEGER TotalReserved;
  ULONG BytesPerSector;
  ULONG BytesPerCluster;
  ULONG BytesPerFileRecordSegment;
  ULONG ClustersPerFileRecordSegment;
  LARGE_INTEGER MftValidDataLength;
  LARGE_INTEGER MftStartLcn;
  LARGE_INTEGER Mft2StartLcn;
  LARGE_INTEGER MftZoneStart;
  LARGE_INTEGER MftZoneEnd;
} NTFS_VOLUME_DATA_BUFFER, *PNTFS_VOLUME_DATA_BUFFER;

typedef struct _NTFS_EXTENDED_VOLUME_DATA {
  ULONG ByteCount;
  USHORT MajorVersion;
  USHORT MinorVersion;
} NTFS_EXTENDED_VOLUME_DATA, *PNTFS_EXTENDED_VOLUME_DATA;

typedef struct _STARTING_LCN_INPUT_BUFFER {
  LARGE_INTEGER StartingLcn;
} STARTING_LCN_INPUT_BUFFER, *PSTARTING_LCN_INPUT_BUFFER;

typedef struct _VOLUME_BITMAP_BUFFER {
  LARGE_INTEGER StartingLcn;
  LARGE_INTEGER BitmapSize;
  UCHAR Buffer[1];
} VOLUME_BITMAP_BUFFER, *PVOLUME_BITMAP_BUFFER;

typedef struct _STARTING_VCN_INPUT_BUFFER {
  LARGE_INTEGER StartingVcn;
} STARTING_VCN_INPUT_BUFFER, *PSTARTING_VCN_INPUT_BUFFER;

typedef struct _RETRIEVAL_POINTERS_BUFFER {
  ULONG ExtentCount;
  LARGE_INTEGER StartingVcn;
  struct {
    LARGE_INTEGER NextVcn;
    LARGE_INTEGER Lcn;
  } Extents[1];
} RETRIEVAL_POINTERS_BUFFER, *PRETRIEVAL_POINTERS_BUFFER;

typedef struct _NTFS_FILE_RECORD_INPUT_BUFFER {
  LARGE_INTEGER FileReferenceNumber;
} NTFS_FILE_RECORD_INPUT_BUFFER, *PNTFS_FILE_RECORD_INPUT_BUFFER;

typedef struct _NTFS_FILE_RECORD_OUTPUT_BUFFER {
  LARGE_INTEGER FileReferenceNumber;
  ULONG FileRecordLength;
  UCHAR FileRecordBuffer[1];
} NTFS_FILE_RECORD_OUTPUT_BUFFER, *PNTFS_FILE_RECORD_OUTPUT_BUFFER;

typedef struct _MOVE_FILE_DATA {
  HANDLE FileHandle;
  LARGE_INTEGER StartingVcn;
  LARGE_INTEGER StartingLcn;
  ULONG ClusterCount;
} MOVE_FILE_DATA, *PMOVE_FILE_DATA;

typedef struct _MOVE_FILE_RECORD_DATA {
  HANDLE FileHandle;
  LARGE_INTEGER SourceFileRecord;
  LARGE_INTEGER TargetFileRecord;
} MOVE_FILE_RECORD_DATA, *PMOVE_FILE_RECORD_DATA;

#if defined(_WIN64)
typedef struct _MOVE_FILE_DATA32 {
  UINT32 FileHandle;
  LARGE_INTEGER StartingVcn;
  LARGE_INTEGER StartingLcn;
  ULONG ClusterCount;
} MOVE_FILE_DATA32, *PMOVE_FILE_DATA32;
#endif

#endif /* (_WIN32_WINNT >= 0x0400) */

#if (_WIN32_WINNT >= 0x0500)

typedef struct _FIND_BY_SID_DATA {
  ULONG Restart;
  SID Sid;
} FIND_BY_SID_DATA, *PFIND_BY_SID_DATA;

typedef struct _FIND_BY_SID_OUTPUT {
  ULONG NextEntryOffset;
  ULONG FileIndex;
  ULONG FileNameLength;
  WCHAR FileName[1];
} FIND_BY_SID_OUTPUT, *PFIND_BY_SID_OUTPUT;

typedef struct _MFT_ENUM_DATA {
  ULONGLONG StartFileReferenceNumber;
  USN LowUsn;
  USN HighUsn;
} MFT_ENUM_DATA, *PMFT_ENUM_DATA;

typedef struct _CREATE_USN_JOURNAL_DATA {
  ULONGLONG MaximumSize;
  ULONGLONG AllocationDelta;
} CREATE_USN_JOURNAL_DATA, *PCREATE_USN_JOURNAL_DATA;

typedef struct _READ_USN_JOURNAL_DATA {
  USN StartUsn;
  ULONG ReasonMask;
  ULONG ReturnOnlyOnClose;
  ULONGLONG Timeout;
  ULONGLONG BytesToWaitFor;
  ULONGLONG UsnJournalID;
} READ_USN_JOURNAL_DATA, *PREAD_USN_JOURNAL_DATA;

typedef struct _USN_RECORD {
  ULONG RecordLength;
  USHORT MajorVersion;
  USHORT MinorVersion;
  ULONGLONG FileReferenceNumber;
  ULONGLONG ParentFileReferenceNumber;
  USN Usn;
  LARGE_INTEGER TimeStamp;
  ULONG Reason;
  ULONG SourceInfo;
  ULONG SecurityId;
  ULONG FileAttributes;
  USHORT FileNameLength;
  USHORT FileNameOffset;
  WCHAR FileName[1];
} USN_RECORD, *PUSN_RECORD;

#define USN_PAGE_SIZE                    (0x1000)

#define USN_REASON_DATA_OVERWRITE        (0x00000001)
#define USN_REASON_DATA_EXTEND           (0x00000002)
#define USN_REASON_DATA_TRUNCATION       (0x00000004)
#define USN_REASON_NAMED_DATA_OVERWRITE  (0x00000010)
#define USN_REASON_NAMED_DATA_EXTEND     (0x00000020)
#define USN_REASON_NAMED_DATA_TRUNCATION (0x00000040)
#define USN_REASON_FILE_CREATE           (0x00000100)
#define USN_REASON_FILE_DELETE           (0x00000200)
#define USN_REASON_EA_CHANGE             (0x00000400)
#define USN_REASON_SECURITY_CHANGE       (0x00000800)
#define USN_REASON_RENAME_OLD_NAME       (0x00001000)
#define USN_REASON_RENAME_NEW_NAME       (0x00002000)
#define USN_REASON_INDEXABLE_CHANGE      (0x00004000)
#define USN_REASON_BASIC_INFO_CHANGE     (0x00008000)
#define USN_REASON_HARD_LINK_CHANGE      (0x00010000)
#define USN_REASON_COMPRESSION_CHANGE    (0x00020000)
#define USN_REASON_ENCRYPTION_CHANGE     (0x00040000)
#define USN_REASON_OBJECT_ID_CHANGE      (0x00080000)
#define USN_REASON_REPARSE_POINT_CHANGE  (0x00100000)
#define USN_REASON_STREAM_CHANGE         (0x00200000)
#define USN_REASON_TRANSACTED_CHANGE     (0x00400000)
#define USN_REASON_CLOSE                 (0x80000000)

typedef struct _USN_JOURNAL_DATA {
  ULONGLONG UsnJournalID;
  USN FirstUsn;
  USN NextUsn;
  USN LowestValidUsn;
  USN MaxUsn;
  ULONGLONG MaximumSize;
  ULONGLONG AllocationDelta;
} USN_JOURNAL_DATA, *PUSN_JOURNAL_DATA;

typedef struct _DELETE_USN_JOURNAL_DATA {
  ULONGLONG UsnJournalID;
  ULONG DeleteFlags;
} DELETE_USN_JOURNAL_DATA, *PDELETE_USN_JOURNAL_DATA;

#define USN_DELETE_FLAG_DELETE              (0x00000001)
#define USN_DELETE_FLAG_NOTIFY              (0x00000002)
#define USN_DELETE_VALID_FLAGS              (0x00000003)

typedef struct _MARK_HANDLE_INFO {
  ULONG UsnSourceInfo;
  HANDLE VolumeHandle;
  ULONG HandleInfo;
} MARK_HANDLE_INFO, *PMARK_HANDLE_INFO;

#if defined(_WIN64)
typedef struct _MARK_HANDLE_INFO32 {
  ULONG UsnSourceInfo;
  UINT32 VolumeHandle;
  ULONG HandleInfo;
} MARK_HANDLE_INFO32, *PMARK_HANDLE_INFO32;
#endif

#define USN_SOURCE_DATA_MANAGEMENT          (0x00000001)
#define USN_SOURCE_AUXILIARY_DATA           (0x00000002)
#define USN_SOURCE_REPLICATION_MANAGEMENT   (0x00000004)

#define MARK_HANDLE_PROTECT_CLUSTERS        (0x00000001)
#define MARK_HANDLE_TXF_SYSTEM_LOG          (0x00000004)
#define MARK_HANDLE_NOT_TXF_SYSTEM_LOG      (0x00000008)

typedef struct _BULK_SECURITY_TEST_DATA {
  ACCESS_MASK DesiredAccess;
  ULONG SecurityIds[1];
} BULK_SECURITY_TEST_DATA, *PBULK_SECURITY_TEST_DATA;

#define VOLUME_IS_DIRTY                  (0x00000001)
#define VOLUME_UPGRADE_SCHEDULED         (0x00000002)
#define VOLUME_SESSION_OPEN              (0x00000004)

typedef struct _FILE_PREFETCH {
  ULONG Type;
  ULONG Count;
  ULONGLONG Prefetch[1];
} FILE_PREFETCH, *PFILE_PREFETCH;

typedef struct _FILE_PREFETCH_EX {
  ULONG Type;
  ULONG Count;
  PVOID Context;
  ULONGLONG Prefetch[1];
} FILE_PREFETCH_EX, *PFILE_PREFETCH_EX;

#define FILE_PREFETCH_TYPE_FOR_CREATE       0x1
#define FILE_PREFETCH_TYPE_FOR_DIRENUM      0x2
#define FILE_PREFETCH_TYPE_FOR_CREATE_EX    0x3
#define FILE_PREFETCH_TYPE_FOR_DIRENUM_EX   0x4

#define FILE_PREFETCH_TYPE_MAX              0x4

typedef struct _FILE_OBJECTID_BUFFER {
  UCHAR ObjectId[16];
  _ANONYMOUS_UNION union {
    _ANONYMOUS_STRUCT struct {
      UCHAR BirthVolumeId[16];
      UCHAR BirthObjectId[16];
      UCHAR DomainId[16];
    } DUMMYSTRUCTNAME;
    UCHAR ExtendedInfo[48];
  } DUMMYUNIONNAME;
} FILE_OBJECTID_BUFFER, *PFILE_OBJECTID_BUFFER;

typedef struct _FILE_SET_SPARSE_BUFFER {
  BOOLEAN SetSparse;
} FILE_SET_SPARSE_BUFFER, *PFILE_SET_SPARSE_BUFFER;

typedef struct _FILE_ZERO_DATA_INFORMATION {
  LARGE_INTEGER FileOffset;
  LARGE_INTEGER BeyondFinalZero;
} FILE_ZERO_DATA_INFORMATION, *PFILE_ZERO_DATA_INFORMATION;

typedef struct _FILE_ALLOCATED_RANGE_BUFFER {
  LARGE_INTEGER FileOffset;
  LARGE_INTEGER Length;
} FILE_ALLOCATED_RANGE_BUFFER, *PFILE_ALLOCATED_RANGE_BUFFER;

typedef struct _ENCRYPTION_BUFFER {
  ULONG EncryptionOperation;
  UCHAR Private[1];
} ENCRYPTION_BUFFER, *PENCRYPTION_BUFFER;

#define FILE_SET_ENCRYPTION         0x00000001
#define FILE_CLEAR_ENCRYPTION       0x00000002
#define STREAM_SET_ENCRYPTION       0x00000003
#define STREAM_CLEAR_ENCRYPTION     0x00000004

#define MAXIMUM_ENCRYPTION_VALUE    0x00000004

typedef struct _DECRYPTION_STATUS_BUFFER {
  BOOLEAN NoEncryptedStreams;
} DECRYPTION_STATUS_BUFFER, *PDECRYPTION_STATUS_BUFFER;

#define ENCRYPTION_FORMAT_DEFAULT        (0x01)

#define COMPRESSION_FORMAT_SPARSE        (0x4000)

typedef struct _REQUEST_RAW_ENCRYPTED_DATA {
  LONGLONG FileOffset;
  ULONG Length;
} REQUEST_RAW_ENCRYPTED_DATA, *PREQUEST_RAW_ENCRYPTED_DATA;

typedef struct _ENCRYPTED_DATA_INFO {
  ULONGLONG StartingFileOffset;
  ULONG OutputBufferOffset;
  ULONG BytesWithinFileSize;
  ULONG BytesWithinValidDataLength;
  USHORT CompressionFormat;
  UCHAR DataUnitShift;
  UCHAR ChunkShift;
  UCHAR ClusterShift;
  UCHAR EncryptionFormat;
  USHORT NumberOfDataBlocks;
  ULONG DataBlockSize[ANYSIZE_ARRAY];
} ENCRYPTED_DATA_INFO, *PENCRYPTED_DATA_INFO;

typedef struct _PLEX_READ_DATA_REQUEST {
  LARGE_INTEGER ByteOffset;
  ULONG ByteLength;
  ULONG PlexNumber;
} PLEX_READ_DATA_REQUEST, *PPLEX_READ_DATA_REQUEST;

typedef struct _SI_COPYFILE {
  ULONG SourceFileNameLength;
  ULONG DestinationFileNameLength;
  ULONG Flags;
  WCHAR FileNameBuffer[1];
} SI_COPYFILE, *PSI_COPYFILE;

#define COPYFILE_SIS_LINK       0x0001
#define COPYFILE_SIS_REPLACE    0x0002
#define COPYFILE_SIS_FLAGS      0x0003

#endif /* (_WIN32_WINNT >= 0x0500) */

#if (_WIN32_WINNT >= 0x0600)

typedef struct _FILE_MAKE_COMPATIBLE_BUFFER {
  BOOLEAN CloseDisc;
} FILE_MAKE_COMPATIBLE_BUFFER, *PFILE_MAKE_COMPATIBLE_BUFFER;

typedef struct _FILE_SET_DEFECT_MGMT_BUFFER {
  BOOLEAN Disable;
} FILE_SET_DEFECT_MGMT_BUFFER, *PFILE_SET_DEFECT_MGMT_BUFFER;

typedef struct _FILE_QUERY_SPARING_BUFFER {
  ULONG SparingUnitBytes;
  BOOLEAN SoftwareSparing;
  ULONG TotalSpareBlocks;
  ULONG FreeSpareBlocks;
} FILE_QUERY_SPARING_BUFFER, *PFILE_QUERY_SPARING_BUFFER;

typedef struct _FILE_QUERY_ON_DISK_VOL_INFO_BUFFER {
  LARGE_INTEGER DirectoryCount;
  LARGE_INTEGER FileCount;
  USHORT FsFormatMajVersion;
  USHORT FsFormatMinVersion;
  WCHAR FsFormatName[12];
  LARGE_INTEGER FormatTime;
  LARGE_INTEGER LastUpdateTime;
  WCHAR CopyrightInfo[34];
  WCHAR AbstractInfo[34];
  WCHAR FormattingImplementationInfo[34];
  WCHAR LastModifyingImplementationInfo[34];
} FILE_QUERY_ON_DISK_VOL_INFO_BUFFER, *PFILE_QUERY_ON_DISK_VOL_INFO_BUFFER;

#define SET_REPAIR_ENABLED                                      (0x00000001)
#define SET_REPAIR_VOLUME_BITMAP_SCAN                           (0x00000002)
#define SET_REPAIR_DELETE_CROSSLINK                             (0x00000004)
#define SET_REPAIR_WARN_ABOUT_DATA_LOSS                         (0x00000008)
#define SET_REPAIR_DISABLED_AND_BUGCHECK_ON_CORRUPT             (0x00000010)
#define SET_REPAIR_VALID_MASK                                   (0x0000001F)

typedef enum _SHRINK_VOLUME_REQUEST_TYPES {
  ShrinkPrepare = 1,
  ShrinkCommit,
  ShrinkAbort
} SHRINK_VOLUME_REQUEST_TYPES, *PSHRINK_VOLUME_REQUEST_TYPES;

typedef struct _SHRINK_VOLUME_INFORMATION {
  SHRINK_VOLUME_REQUEST_TYPES ShrinkRequestType;
  ULONGLONG Flags;
  LONGLONG NewNumberOfSectors;
} SHRINK_VOLUME_INFORMATION, *PSHRINK_VOLUME_INFORMATION;

#define TXFS_RM_FLAG_LOGGING_MODE                           0x00000001
#define TXFS_RM_FLAG_RENAME_RM                              0x00000002
#define TXFS_RM_FLAG_LOG_CONTAINER_COUNT_MAX                0x00000004
#define TXFS_RM_FLAG_LOG_CONTAINER_COUNT_MIN                0x00000008
#define TXFS_RM_FLAG_LOG_GROWTH_INCREMENT_NUM_CONTAINERS    0x00000010
#define TXFS_RM_FLAG_LOG_GROWTH_INCREMENT_PERCENT           0x00000020
#define TXFS_RM_FLAG_LOG_AUTO_SHRINK_PERCENTAGE             0x00000040
#define TXFS_RM_FLAG_LOG_NO_CONTAINER_COUNT_MAX             0x00000080
#define TXFS_RM_FLAG_LOG_NO_CONTAINER_COUNT_MIN             0x00000100
#define TXFS_RM_FLAG_GROW_LOG                               0x00000400
#define TXFS_RM_FLAG_SHRINK_LOG                             0x00000800
#define TXFS_RM_FLAG_ENFORCE_MINIMUM_SIZE                   0x00001000
#define TXFS_RM_FLAG_PRESERVE_CHANGES                       0x00002000
#define TXFS_RM_FLAG_RESET_RM_AT_NEXT_START                 0x00004000
#define TXFS_RM_FLAG_DO_NOT_RESET_RM_AT_NEXT_START          0x00008000
#define TXFS_RM_FLAG_PREFER_CONSISTENCY                     0x00010000
#define TXFS_RM_FLAG_PREFER_AVAILABILITY                    0x00020000

#define TXFS_LOGGING_MODE_SIMPLE        (0x0001)
#define TXFS_LOGGING_MODE_FULL          (0x0002)

#define TXFS_TRANSACTION_STATE_NONE         0x00
#define TXFS_TRANSACTION_STATE_ACTIVE       0x01
#define TXFS_TRANSACTION_STATE_PREPARED     0x02
#define TXFS_TRANSACTION_STATE_NOTACTIVE    0x03

#define TXFS_MODIFY_RM_VALID_FLAGS (TXFS_RM_FLAG_LOGGING_MODE                        | \
                                    TXFS_RM_FLAG_RENAME_RM                           | \
                                    TXFS_RM_FLAG_LOG_CONTAINER_COUNT_MAX             | \
                                    TXFS_RM_FLAG_LOG_CONTAINER_COUNT_MIN             | \
                                    TXFS_RM_FLAG_LOG_GROWTH_INCREMENT_NUM_CONTAINERS | \
                                    TXFS_RM_FLAG_LOG_GROWTH_INCREMENT_PERCENT        | \
                                    TXFS_RM_FLAG_LOG_AUTO_SHRINK_PERCENTAGE          | \
                                    TXFS_RM_FLAG_LOG_NO_CONTAINER_COUNT_MAX          | \
                                    TXFS_RM_FLAG_LOG_NO_CONTAINER_COUNT_MIN          | \
                                    TXFS_RM_FLAG_SHRINK_LOG                          | \
                                    TXFS_RM_FLAG_GROW_LOG                            | \
                                    TXFS_RM_FLAG_ENFORCE_MINIMUM_SIZE                | \
                                    TXFS_RM_FLAG_PRESERVE_CHANGES                    | \
                                    TXFS_RM_FLAG_RESET_RM_AT_NEXT_START              | \
                                    TXFS_RM_FLAG_DO_NOT_RESET_RM_AT_NEXT_START       | \
                                    TXFS_RM_FLAG_PREFER_CONSISTENCY                  | \
                                    TXFS_RM_FLAG_PREFER_AVAILABILITY)

typedef struct _TXFS_MODIFY_RM {
  ULONG Flags;
  ULONG LogContainerCountMax;
  ULONG LogContainerCountMin;
  ULONG LogContainerCount;
  ULONG LogGrowthIncrement;
  ULONG LogAutoShrinkPercentage;
  ULONGLONG Reserved;
  USHORT LoggingMode;
} TXFS_MODIFY_RM, *PTXFS_MODIFY_RM;

#define TXFS_RM_STATE_NOT_STARTED       0
#define TXFS_RM_STATE_STARTING          1
#define TXFS_RM_STATE_ACTIVE            2
#define TXFS_RM_STATE_SHUTTING_DOWN     3

#define TXFS_QUERY_RM_INFORMATION_VALID_FLAGS                           \
                (TXFS_RM_FLAG_LOG_GROWTH_INCREMENT_NUM_CONTAINERS   |   \
                 TXFS_RM_FLAG_LOG_GROWTH_INCREMENT_PERCENT          |   \
                 TXFS_RM_FLAG_LOG_NO_CONTAINER_COUNT_MAX            |   \
                 TXFS_RM_FLAG_LOG_NO_CONTAINER_COUNT_MIN            |   \
                 TXFS_RM_FLAG_RESET_RM_AT_NEXT_START                |   \
                 TXFS_RM_FLAG_DO_NOT_RESET_RM_AT_NEXT_START         |   \
                 TXFS_RM_FLAG_PREFER_CONSISTENCY                    |   \
                 TXFS_RM_FLAG_PREFER_AVAILABILITY)

typedef struct _TXFS_QUERY_RM_INFORMATION {
  ULONG BytesRequired;
  ULONGLONG TailLsn;
  ULONGLONG CurrentLsn;
  ULONGLONG ArchiveTailLsn;
  ULONGLONG LogContainerSize;
  LARGE_INTEGER HighestVirtualClock;
  ULONG LogContainerCount;
  ULONG LogContainerCountMax;
  ULONG LogContainerCountMin;
  ULONG LogGrowthIncrement;
  ULONG LogAutoShrinkPercentage;
  ULONG Flags;
  USHORT LoggingMode;
  USHORT Reserved;
  ULONG RmState;
  ULONGLONG LogCapacity;
  ULONGLONG LogFree;
  ULONGLONG TopsSize;
  ULONGLONG TopsUsed;
  ULONGLONG TransactionCount;
  ULONGLONG OnePCCount;
  ULONGLONG TwoPCCount;
  ULONGLONG NumberLogFileFull;
  ULONGLONG OldestTransactionAge;
  GUID RMName;
  ULONG TmLogPathOffset;
} TXFS_QUERY_RM_INFORMATION, *PTXFS_QUERY_RM_INFORMATION;

#define TXFS_ROLLFORWARD_REDO_FLAG_USE_LAST_REDO_LSN        0x01
#define TXFS_ROLLFORWARD_REDO_FLAG_USE_LAST_VIRTUAL_CLOCK   0x02

#define TXFS_ROLLFORWARD_REDO_VALID_FLAGS                               \
                (TXFS_ROLLFORWARD_REDO_FLAG_USE_LAST_REDO_LSN |         \
                 TXFS_ROLLFORWARD_REDO_FLAG_USE_LAST_VIRTUAL_CLOCK)

typedef struct _TXFS_ROLLFORWARD_REDO_INFORMATION {
  LARGE_INTEGER LastVirtualClock;
  ULONGLONG LastRedoLsn;
  ULONGLONG HighestRecoveryLsn;
  ULONG Flags;
} TXFS_ROLLFORWARD_REDO_INFORMATION, *PTXFS_ROLLFORWARD_REDO_INFORMATION;

#define TXFS_START_RM_FLAG_LOG_CONTAINER_COUNT_MAX              0x00000001
#define TXFS_START_RM_FLAG_LOG_CONTAINER_COUNT_MIN              0x00000002
#define TXFS_START_RM_FLAG_LOG_CONTAINER_SIZE                   0x00000004
#define TXFS_START_RM_FLAG_LOG_GROWTH_INCREMENT_NUM_CONTAINERS  0x00000008
#define TXFS_START_RM_FLAG_LOG_GROWTH_INCREMENT_PERCENT         0x00000010
#define TXFS_START_RM_FLAG_LOG_AUTO_SHRINK_PERCENTAGE           0x00000020
#define TXFS_START_RM_FLAG_LOG_NO_CONTAINER_COUNT_MAX           0x00000040
#define TXFS_START_RM_FLAG_LOG_NO_CONTAINER_COUNT_MIN           0x00000080

#define TXFS_START_RM_FLAG_RECOVER_BEST_EFFORT                  0x00000200
#define TXFS_START_RM_FLAG_LOGGING_MODE                         0x00000400
#define TXFS_START_RM_FLAG_PRESERVE_CHANGES                     0x00000800

#define TXFS_START_RM_FLAG_PREFER_CONSISTENCY                   0x00001000
#define TXFS_START_RM_FLAG_PREFER_AVAILABILITY                  0x00002000

#define TXFS_START_RM_VALID_FLAGS                                           \
                (TXFS_START_RM_FLAG_LOG_CONTAINER_COUNT_MAX             |   \
                 TXFS_START_RM_FLAG_LOG_CONTAINER_COUNT_MIN             |   \
                 TXFS_START_RM_FLAG_LOG_CONTAINER_SIZE                  |   \
                 TXFS_START_RM_FLAG_LOG_GROWTH_INCREMENT_NUM_CONTAINERS |   \
                 TXFS_START_RM_FLAG_LOG_GROWTH_INCREMENT_PERCENT        |   \
                 TXFS_START_RM_FLAG_LOG_AUTO_SHRINK_PERCENTAGE          |   \
                 TXFS_START_RM_FLAG_RECOVER_BEST_EFFORT                 |   \
                 TXFS_START_RM_FLAG_LOG_NO_CONTAINER_COUNT_MAX          |   \
                 TXFS_START_RM_FLAG_LOGGING_MODE                        |   \
                 TXFS_START_RM_FLAG_PRESERVE_CHANGES                    |   \
                 TXFS_START_RM_FLAG_PREFER_CONSISTENCY                  |   \
                 TXFS_START_RM_FLAG_PREFER_AVAILABILITY)

typedef struct _TXFS_START_RM_INFORMATION {
  ULONG Flags;
  ULONGLONG LogContainerSize;
  ULONG LogContainerCountMin;
  ULONG LogContainerCountMax;
  ULONG LogGrowthIncrement;
  ULONG LogAutoShrinkPercentage;
  ULONG TmLogPathOffset;
  USHORT TmLogPathLength;
  USHORT LoggingMode;
  USHORT LogPathLength;
  USHORT Reserved;
  WCHAR LogPath[1];
} TXFS_START_RM_INFORMATION, *PTXFS_START_RM_INFORMATION;

typedef struct _TXFS_GET_METADATA_INFO_OUT {
  struct {
    LONGLONG LowPart;
    LONGLONG HighPart;
  } TxfFileId;
  GUID LockingTransaction;
  ULONGLONG LastLsn;
  ULONG TransactionState;
} TXFS_GET_METADATA_INFO_OUT, *PTXFS_GET_METADATA_INFO_OUT;

#define TXFS_LIST_TRANSACTION_LOCKED_FILES_ENTRY_FLAG_CREATED   0x00000001
#define TXFS_LIST_TRANSACTION_LOCKED_FILES_ENTRY_FLAG_DELETED   0x00000002

typedef struct _TXFS_LIST_TRANSACTION_LOCKED_FILES_ENTRY {
  ULONGLONG Offset;
  ULONG NameFlags;
  LONGLONG FileId;
  ULONG Reserved1;
  ULONG Reserved2;
  LONGLONG Reserved3;
  WCHAR FileName[1];
} TXFS_LIST_TRANSACTION_LOCKED_FILES_ENTRY, *PTXFS_LIST_TRANSACTION_LOCKED_FILES_ENTRY;

typedef struct _TXFS_LIST_TRANSACTION_LOCKED_FILES {
  GUID KtmTransaction;
  ULONGLONG NumberOfFiles;
  ULONGLONG BufferSizeRequired;
  ULONGLONG Offset;
} TXFS_LIST_TRANSACTION_LOCKED_FILES, *PTXFS_LIST_TRANSACTION_LOCKED_FILES;

typedef struct _TXFS_LIST_TRANSACTIONS_ENTRY {
  GUID TransactionId;
  ULONG TransactionState;
  ULONG Reserved1;
  ULONG Reserved2;
  LONGLONG Reserved3;
} TXFS_LIST_TRANSACTIONS_ENTRY, *PTXFS_LIST_TRANSACTIONS_ENTRY;

typedef struct _TXFS_LIST_TRANSACTIONS {
  ULONGLONG NumberOfTransactions;
  ULONGLONG BufferSizeRequired;
} TXFS_LIST_TRANSACTIONS, *PTXFS_LIST_TRANSACTIONS;

typedef struct _TXFS_READ_BACKUP_INFORMATION_OUT {
  _ANONYMOUS_UNION union {
    ULONG BufferLength;
    UCHAR Buffer[1];
  } DUMMYUNIONNAME;
} TXFS_READ_BACKUP_INFORMATION_OUT, *PTXFS_READ_BACKUP_INFORMATION_OUT;

typedef struct _TXFS_WRITE_BACKUP_INFORMATION {
  UCHAR Buffer[1];
} TXFS_WRITE_BACKUP_INFORMATION, *PTXFS_WRITE_BACKUP_INFORMATION;

#define TXFS_TRANSACTED_VERSION_NONTRANSACTED   0xFFFFFFFE
#define TXFS_TRANSACTED_VERSION_UNCOMMITTED     0xFFFFFFFF

typedef struct _TXFS_GET_TRANSACTED_VERSION {
  ULONG ThisBaseVersion;
  ULONG LatestVersion;
  USHORT ThisMiniVersion;
  USHORT FirstMiniVersion;
  USHORT LatestMiniVersion;
} TXFS_GET_TRANSACTED_VERSION, *PTXFS_GET_TRANSACTED_VERSION;

#define TXFS_SAVEPOINT_SET                      0x00000001
#define TXFS_SAVEPOINT_ROLLBACK                 0x00000002
#define TXFS_SAVEPOINT_CLEAR                    0x00000004
#define TXFS_SAVEPOINT_CLEAR_ALL                0x00000010

typedef struct _TXFS_SAVEPOINT_INFORMATION {
  HANDLE KtmTransaction;
  ULONG ActionCode;
  ULONG SavepointId;
} TXFS_SAVEPOINT_INFORMATION, *PTXFS_SAVEPOINT_INFORMATION;

typedef struct _TXFS_CREATE_MINIVERSION_INFO {
  USHORT StructureVersion;
  USHORT StructureLength;
  ULONG BaseVersion;
  USHORT MiniVersion;
} TXFS_CREATE_MINIVERSION_INFO, *PTXFS_CREATE_MINIVERSION_INFO;

typedef struct _TXFS_TRANSACTION_ACTIVE_INFO {
  BOOLEAN TransactionsActiveAtSnapshot;
} TXFS_TRANSACTION_ACTIVE_INFO, *PTXFS_TRANSACTION_ACTIVE_INFO;

#endif /* (_WIN32_WINNT >= 0x0600) */

#if (_WIN32_WINNT >= 0x0601)

#define MARK_HANDLE_REALTIME                (0x00000020)
#define MARK_HANDLE_NOT_REALTIME            (0x00000040)

#define NO_8DOT3_NAME_PRESENT               (0x00000001)
#define REMOVED_8DOT3_NAME                  (0x00000002)

#define PERSISTENT_VOLUME_STATE_SHORT_NAME_CREATION_DISABLED        (0x00000001)

typedef struct _BOOT_AREA_INFO {
  ULONG BootSectorCount;
  struct {
    LARGE_INTEGER Offset;
  } BootSectors[2];
} BOOT_AREA_INFO, *PBOOT_AREA_INFO;

typedef struct _RETRIEVAL_POINTER_BASE {
  LARGE_INTEGER FileAreaOffset;
} RETRIEVAL_POINTER_BASE, *PRETRIEVAL_POINTER_BASE;

typedef struct _FILE_FS_PERSISTENT_VOLUME_INFORMATION {
  ULONG VolumeFlags;
  ULONG FlagMask;
  ULONG Version;
  ULONG Reserved;
} FILE_FS_PERSISTENT_VOLUME_INFORMATION, *PFILE_FS_PERSISTENT_VOLUME_INFORMATION;

typedef struct _FILE_SYSTEM_RECOGNITION_INFORMATION {
  CHAR FileSystem[9];
} FILE_SYSTEM_RECOGNITION_INFORMATION, *PFILE_SYSTEM_RECOGNITION_INFORMATION;

#define OPLOCK_LEVEL_CACHE_READ         (0x00000001)
#define OPLOCK_LEVEL_CACHE_HANDLE       (0x00000002)
#define OPLOCK_LEVEL_CACHE_WRITE        (0x00000004)

#define REQUEST_OPLOCK_INPUT_FLAG_REQUEST               (0x00000001)
#define REQUEST_OPLOCK_INPUT_FLAG_ACK                   (0x00000002)
#define REQUEST_OPLOCK_INPUT_FLAG_COMPLETE_ACK_ON_CLOSE (0x00000004)

#define REQUEST_OPLOCK_CURRENT_VERSION          1

typedef struct _REQUEST_OPLOCK_INPUT_BUFFER {
  USHORT StructureVersion;
  USHORT StructureLength;
  ULONG RequestedOplockLevel;
  ULONG Flags;
} REQUEST_OPLOCK_INPUT_BUFFER, *PREQUEST_OPLOCK_INPUT_BUFFER;

#define REQUEST_OPLOCK_OUTPUT_FLAG_ACK_REQUIRED     (0x00000001)
#define REQUEST_OPLOCK_OUTPUT_FLAG_MODES_PROVIDED   (0x00000002)

typedef struct _REQUEST_OPLOCK_OUTPUT_BUFFER {
  USHORT StructureVersion;
  USHORT StructureLength;
  ULONG OriginalOplockLevel;
  ULONG NewOplockLevel;
  ULONG Flags;
  ACCESS_MASK AccessMode;
  USHORT ShareMode;
} REQUEST_OPLOCK_OUTPUT_BUFFER, *PREQUEST_OPLOCK_OUTPUT_BUFFER;

#define SD_GLOBAL_CHANGE_TYPE_MACHINE_SID   1

typedef struct _SD_CHANGE_MACHINE_SID_INPUT {
  USHORT CurrentMachineSIDOffset;
  USHORT CurrentMachineSIDLength;
  USHORT NewMachineSIDOffset;
  USHORT NewMachineSIDLength;
} SD_CHANGE_MACHINE_SID_INPUT, *PSD_CHANGE_MACHINE_SID_INPUT;

typedef struct _SD_CHANGE_MACHINE_SID_OUTPUT {
  ULONGLONG NumSDChangedSuccess;
  ULONGLONG NumSDChangedFail;
  ULONGLONG NumSDUnused;
  ULONGLONG NumSDTotal;
  ULONGLONG NumMftSDChangedSuccess;
  ULONGLONG NumMftSDChangedFail;
  ULONGLONG NumMftSDTotal;
} SD_CHANGE_MACHINE_SID_OUTPUT, *PSD_CHANGE_MACHINE_SID_OUTPUT;

typedef struct _SD_GLOBAL_CHANGE_INPUT {
  ULONG Flags;
  ULONG ChangeType;
  _ANONYMOUS_UNION union {
    SD_CHANGE_MACHINE_SID_INPUT SdChange;
  } DUMMYUNIONNAME;
} SD_GLOBAL_CHANGE_INPUT, *PSD_GLOBAL_CHANGE_INPUT;

typedef struct _SD_GLOBAL_CHANGE_OUTPUT {
  ULONG Flags;
  ULONG ChangeType;
  _ANONYMOUS_UNION union {
    SD_CHANGE_MACHINE_SID_OUTPUT SdChange;
  } DUMMYUNIONNAME;
} SD_GLOBAL_CHANGE_OUTPUT, *PSD_GLOBAL_CHANGE_OUTPUT;

#define ENCRYPTED_DATA_INFO_SPARSE_FILE    1

typedef struct _EXTENDED_ENCRYPTED_DATA_INFO {
  ULONG ExtendedCode;
  ULONG Length;
  ULONG Flags;
  ULONG Reserved;
} EXTENDED_ENCRYPTED_DATA_INFO, *PEXTENDED_ENCRYPTED_DATA_INFO;

typedef struct _LOOKUP_STREAM_FROM_CLUSTER_INPUT {
  ULONG Flags;
  ULONG NumberOfClusters;
  LARGE_INTEGER Cluster[1];
} LOOKUP_STREAM_FROM_CLUSTER_INPUT, *PLOOKUP_STREAM_FROM_CLUSTER_INPUT;

typedef struct _LOOKUP_STREAM_FROM_CLUSTER_OUTPUT {
  ULONG Offset;
  ULONG NumberOfMatches;
  ULONG BufferSizeRequired;
} LOOKUP_STREAM_FROM_CLUSTER_OUTPUT, *PLOOKUP_STREAM_FROM_CLUSTER_OUTPUT;

#define LOOKUP_STREAM_FROM_CLUSTER_ENTRY_FLAG_PAGE_FILE          0x00000001
#define LOOKUP_STREAM_FROM_CLUSTER_ENTRY_FLAG_DENY_DEFRAG_SET    0x00000002
#define LOOKUP_STREAM_FROM_CLUSTER_ENTRY_FLAG_FS_SYSTEM_FILE     0x00000004
#define LOOKUP_STREAM_FROM_CLUSTER_ENTRY_FLAG_TXF_SYSTEM_FILE    0x00000008

#define LOOKUP_STREAM_FROM_CLUSTER_ENTRY_ATTRIBUTE_MASK          0xff000000
#define LOOKUP_STREAM_FROM_CLUSTER_ENTRY_ATTRIBUTE_DATA          0x01000000
#define LOOKUP_STREAM_FROM_CLUSTER_ENTRY_ATTRIBUTE_INDEX         0x02000000
#define LOOKUP_STREAM_FROM_CLUSTER_ENTRY_ATTRIBUTE_SYSTEM        0x03000000

typedef struct _LOOKUP_STREAM_FROM_CLUSTER_ENTRY {
  ULONG OffsetToNext;
  ULONG Flags;
  LARGE_INTEGER Reserved;
  LARGE_INTEGER Cluster;
  WCHAR FileName[1];
} LOOKUP_STREAM_FROM_CLUSTER_ENTRY, *PLOOKUP_STREAM_FROM_CLUSTER_ENTRY;

typedef struct _FILE_TYPE_NOTIFICATION_INPUT {
  ULONG Flags;
  ULONG NumFileTypeIDs;
  GUID FileTypeID[1];
} FILE_TYPE_NOTIFICATION_INPUT, *PFILE_TYPE_NOTIFICATION_INPUT;

#define FILE_TYPE_NOTIFICATION_FLAG_USAGE_BEGIN     0x00000001
#define FILE_TYPE_NOTIFICATION_FLAG_USAGE_END       0x00000002

DEFINE_GUID(FILE_TYPE_NOTIFICATION_GUID_PAGE_FILE,         0x0d0a64a1, 0x38fc, 0x4db8, 0x9f, 0xe7, 0x3f, 0x43, 0x52, 0xcd, 0x7c, 0x5c);
DEFINE_GUID(FILE_TYPE_NOTIFICATION_GUID_HIBERNATION_FILE,  0xb7624d64, 0xb9a3, 0x4cf8, 0x80, 0x11, 0x5b, 0x86, 0xc9, 0x40, 0xe7, 0xb7);
DEFINE_GUID(FILE_TYPE_NOTIFICATION_GUID_CRASHDUMP_FILE,    0x9d453eb7, 0xd2a6, 0x4dbd, 0xa2, 0xe3, 0xfb, 0xd0, 0xed, 0x91, 0x09, 0xa9);

#ifndef _VIRTUAL_STORAGE_TYPE_DEFINED
#define _VIRTUAL_STORAGE_TYPE_DEFINED
typedef struct _VIRTUAL_STORAGE_TYPE {
  ULONG DeviceId;
  GUID VendorId;
} VIRTUAL_STORAGE_TYPE, *PVIRTUAL_STORAGE_TYPE;
#endif

typedef struct _STORAGE_QUERY_DEPENDENT_VOLUME_REQUEST {
  ULONG RequestLevel;
  ULONG RequestFlags;
} STORAGE_QUERY_DEPENDENT_VOLUME_REQUEST, *PSTORAGE_QUERY_DEPENDENT_VOLUME_REQUEST;

#define QUERY_DEPENDENT_VOLUME_REQUEST_FLAG_HOST_VOLUMES    0x1
#define QUERY_DEPENDENT_VOLUME_REQUEST_FLAG_GUEST_VOLUMES   0x2

typedef struct _STORAGE_QUERY_DEPENDENT_VOLUME_LEV1_ENTRY {
  ULONG EntryLength;
  ULONG DependencyTypeFlags;
  ULONG ProviderSpecificFlags;
  VIRTUAL_STORAGE_TYPE VirtualStorageType;
} STORAGE_QUERY_DEPENDENT_VOLUME_LEV1_ENTRY, *PSTORAGE_QUERY_DEPENDENT_VOLUME_LEV1_ENTRY;

typedef struct _STORAGE_QUERY_DEPENDENT_VOLUME_LEV2_ENTRY {
  ULONG EntryLength;
  ULONG DependencyTypeFlags;
  ULONG ProviderSpecificFlags;
  VIRTUAL_STORAGE_TYPE VirtualStorageType;
  ULONG AncestorLevel;
  ULONG HostVolumeNameOffset;
  ULONG HostVolumeNameSize;
  ULONG DependentVolumeNameOffset;
  ULONG DependentVolumeNameSize;
  ULONG RelativePathOffset;
  ULONG RelativePathSize;
  ULONG DependentDeviceNameOffset;
  ULONG DependentDeviceNameSize;
} STORAGE_QUERY_DEPENDENT_VOLUME_LEV2_ENTRY, *PSTORAGE_QUERY_DEPENDENT_VOLUME_LEV2_ENTRY;

typedef struct _STORAGE_QUERY_DEPENDENT_VOLUME_RESPONSE {
  ULONG ResponseLevel;
  ULONG NumberEntries;
  _ANONYMOUS_UNION union {
    STORAGE_QUERY_DEPENDENT_VOLUME_LEV1_ENTRY Lev1Depends[0];
    STORAGE_QUERY_DEPENDENT_VOLUME_LEV2_ENTRY Lev2Depends[0];
  } DUMMYUNIONNAME;
} STORAGE_QUERY_DEPENDENT_VOLUME_RESPONSE, *PSTORAGE_QUERY_DEPENDENT_VOLUME_RESPONSE;

#endif /* (_WIN32_WINNT >= 0x0601) */

typedef struct _FILESYSTEM_STATISTICS {
  USHORT FileSystemType;
  USHORT Version;
  ULONG SizeOfCompleteStructure;
  ULONG UserFileReads;
  ULONG UserFileReadBytes;
  ULONG UserDiskReads;
  ULONG UserFileWrites;
  ULONG UserFileWriteBytes;
  ULONG UserDiskWrites;
  ULONG MetaDataReads;
  ULONG MetaDataReadBytes;
  ULONG MetaDataDiskReads;
  ULONG MetaDataWrites;
  ULONG MetaDataWriteBytes;
  ULONG MetaDataDiskWrites;
} FILESYSTEM_STATISTICS, *PFILESYSTEM_STATISTICS;

#define FILESYSTEM_STATISTICS_TYPE_NTFS     1
#define FILESYSTEM_STATISTICS_TYPE_FAT      2
#define FILESYSTEM_STATISTICS_TYPE_EXFAT    3

typedef struct _FAT_STATISTICS {
  ULONG CreateHits;
  ULONG SuccessfulCreates;
  ULONG FailedCreates;
  ULONG NonCachedReads;
  ULONG NonCachedReadBytes;
  ULONG NonCachedWrites;
  ULONG NonCachedWriteBytes;
  ULONG NonCachedDiskReads;
  ULONG NonCachedDiskWrites;
} FAT_STATISTICS, *PFAT_STATISTICS;

typedef struct _EXFAT_STATISTICS {
  ULONG CreateHits;
  ULONG SuccessfulCreates;
  ULONG FailedCreates;
  ULONG NonCachedReads;
  ULONG NonCachedReadBytes;
  ULONG NonCachedWrites;
  ULONG NonCachedWriteBytes;
  ULONG NonCachedDiskReads;
  ULONG NonCachedDiskWrites;
} EXFAT_STATISTICS, *PEXFAT_STATISTICS;

typedef struct _NTFS_STATISTICS {
  ULONG LogFileFullExceptions;
  ULONG OtherExceptions;
  ULONG MftReads;
  ULONG MftReadBytes;
  ULONG MftWrites;
  ULONG MftWriteBytes;
  struct {
    USHORT Write;
    USHORT Create;
    USHORT SetInfo;
    USHORT Flush;
  } MftWritesUserLevel;
  USHORT MftWritesFlushForLogFileFull;
  USHORT MftWritesLazyWriter;
  USHORT MftWritesUserRequest;
  ULONG Mft2Writes;
  ULONG Mft2WriteBytes;
  struct {
    USHORT Write;
    USHORT Create;
    USHORT SetInfo;
    USHORT Flush;
  } Mft2WritesUserLevel;
  USHORT Mft2WritesFlushForLogFileFull;
  USHORT Mft2WritesLazyWriter;
  USHORT Mft2WritesUserRequest;
  ULONG RootIndexReads;
  ULONG RootIndexReadBytes;
  ULONG RootIndexWrites;
  ULONG RootIndexWriteBytes;
  ULONG BitmapReads;
  ULONG BitmapReadBytes;
  ULONG BitmapWrites;
  ULONG BitmapWriteBytes;
  USHORT BitmapWritesFlushForLogFileFull;
  USHORT BitmapWritesLazyWriter;
  USHORT BitmapWritesUserRequest;
  struct {
    USHORT Write;
    USHORT Create;
    USHORT SetInfo;
  } BitmapWritesUserLevel;
  ULONG MftBitmapReads;
  ULONG MftBitmapReadBytes;
  ULONG MftBitmapWrites;
  ULONG MftBitmapWriteBytes;
  USHORT MftBitmapWritesFlushForLogFileFull;
  USHORT MftBitmapWritesLazyWriter;
  USHORT MftBitmapWritesUserRequest;
  struct {
    USHORT Write;
    USHORT Create;
    USHORT SetInfo;
    USHORT Flush;
  } MftBitmapWritesUserLevel;
  ULONG UserIndexReads;
  ULONG UserIndexReadBytes;
  ULONG UserIndexWrites;
  ULONG UserIndexWriteBytes;
  ULONG LogFileReads;
  ULONG LogFileReadBytes;
  ULONG LogFileWrites;
  ULONG LogFileWriteBytes;
  struct {
    ULONG Calls;
    ULONG Clusters;
    ULONG Hints;
    ULONG RunsReturned;
    ULONG HintsHonored;
    ULONG HintsClusters;
    ULONG Cache;
    ULONG CacheClusters;
    ULONG CacheMiss;
    ULONG CacheMissClusters;
  } Allocate;
} NTFS_STATISTICS, *PNTFS_STATISTICS;

#endif /* _FILESYSTEMFSCTL_ */

#define SYMLINK_FLAG_RELATIVE   1

typedef struct _REPARSE_DATA_BUFFER {
  ULONG ReparseTag;
  USHORT ReparseDataLength;
  USHORT Reserved;
  _ANONYMOUS_UNION union {
    struct {
      USHORT SubstituteNameOffset;
      USHORT SubstituteNameLength;
      USHORT PrintNameOffset;
      USHORT PrintNameLength;
      ULONG Flags;
      WCHAR PathBuffer[1];
    } SymbolicLinkReparseBuffer;
    struct {
      USHORT SubstituteNameOffset;
      USHORT SubstituteNameLength;
      USHORT PrintNameOffset;
      USHORT PrintNameLength;
      WCHAR PathBuffer[1];
    } MountPointReparseBuffer;
    struct {
      UCHAR DataBuffer[1];
    } GenericReparseBuffer;
  } DUMMYUNIONNAME;
} REPARSE_DATA_BUFFER, *PREPARSE_DATA_BUFFER;

#define REPARSE_DATA_BUFFER_HEADER_SIZE   FIELD_OFFSET(REPARSE_DATA_BUFFER, GenericReparseBuffer)

typedef struct _REPARSE_GUID_DATA_BUFFER {
  ULONG ReparseTag;
  USHORT ReparseDataLength;
  USHORT Reserved;
  GUID ReparseGuid;
  struct {
    UCHAR DataBuffer[1];
  } GenericReparseBuffer;
} REPARSE_GUID_DATA_BUFFER, *PREPARSE_GUID_DATA_BUFFER;

#define REPARSE_GUID_DATA_BUFFER_HEADER_SIZE   FIELD_OFFSET(REPARSE_GUID_DATA_BUFFER, GenericReparseBuffer)

#define MAXIMUM_REPARSE_DATA_BUFFER_SIZE      ( 16 * 1024 )

/* Reserved reparse tags */
#define IO_REPARSE_TAG_RESERVED_ZERO            (0)
#define IO_REPARSE_TAG_RESERVED_ONE             (1)
#define IO_REPARSE_TAG_RESERVED_RANGE           IO_REPARSE_TAG_RESERVED_ONE

#define IsReparseTagMicrosoft(_tag)             (((_tag) & 0x80000000))
#define IsReparseTagNameSurrogate(_tag)         (((_tag) & 0x20000000))

#define IO_REPARSE_TAG_VALID_VALUES             (0xF000FFFF)

#define IsReparseTagValid(tag) (                               \
                  !((tag) & ~IO_REPARSE_TAG_VALID_VALUES) &&   \
                  ((tag) > IO_REPARSE_TAG_RESERVED_RANGE)      \
                )

/* MicroSoft reparse point tags */
#define IO_REPARSE_TAG_MOUNT_POINT              (0xA0000003L)
#define IO_REPARSE_TAG_HSM                      (0xC0000004L)
#define IO_REPARSE_TAG_DRIVE_EXTENDER           (0x80000005L)
#define IO_REPARSE_TAG_HSM2                     (0x80000006L)
#define IO_REPARSE_TAG_SIS                      (0x80000007L)
#define IO_REPARSE_TAG_WIM                      (0x80000008L)
#define IO_REPARSE_TAG_CSV                      (0x80000009L)
#define IO_REPARSE_TAG_DFS                      (0x8000000AL)
#define IO_REPARSE_TAG_FILTER_MANAGER           (0x8000000BL)
#define IO_REPARSE_TAG_SYMLINK                  (0xA000000CL)
#define IO_REPARSE_TAG_IIS_CACHE                (0xA0000010L)
#define IO_REPARSE_TAG_DFSR                     (0x80000012L)

#pragma pack(4)
typedef struct _REPARSE_INDEX_KEY {
  ULONG FileReparseTag;
  LARGE_INTEGER FileId;
} REPARSE_INDEX_KEY, *PREPARSE_INDEX_KEY;
#pragma pack()

#define FSCTL_LMR_GET_LINK_TRACKING_INFORMATION   CTL_CODE(FILE_DEVICE_NETWORK_FILE_SYSTEM,58,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define FSCTL_LMR_SET_LINK_TRACKING_INFORMATION   CTL_CODE(FILE_DEVICE_NETWORK_FILE_SYSTEM,59,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_LMR_ARE_FILE_OBJECTS_ON_SAME_SERVER CTL_CODE(FILE_DEVICE_NETWORK_FILE_SYSTEM,60,METHOD_BUFFERED,FILE_ANY_ACCESS)

#define FSCTL_PIPE_ASSIGN_EVENT             CTL_CODE(FILE_DEVICE_NAMED_PIPE, 0, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_PIPE_DISCONNECT               CTL_CODE(FILE_DEVICE_NAMED_PIPE, 1, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_PIPE_LISTEN                   CTL_CODE(FILE_DEVICE_NAMED_PIPE, 2, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_PIPE_PEEK                     CTL_CODE(FILE_DEVICE_NAMED_PIPE, 3, METHOD_BUFFERED, FILE_READ_DATA)
#define FSCTL_PIPE_QUERY_EVENT              CTL_CODE(FILE_DEVICE_NAMED_PIPE, 4, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_PIPE_TRANSCEIVE               CTL_CODE(FILE_DEVICE_NAMED_PIPE, 5, METHOD_NEITHER,  FILE_READ_DATA | FILE_WRITE_DATA)
#define FSCTL_PIPE_WAIT                     CTL_CODE(FILE_DEVICE_NAMED_PIPE, 6, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_PIPE_IMPERSONATE              CTL_CODE(FILE_DEVICE_NAMED_PIPE, 7, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_PIPE_SET_CLIENT_PROCESS       CTL_CODE(FILE_DEVICE_NAMED_PIPE, 8, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_PIPE_QUERY_CLIENT_PROCESS     CTL_CODE(FILE_DEVICE_NAMED_PIPE, 9, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_PIPE_GET_PIPE_ATTRIBUTE       CTL_CODE(FILE_DEVICE_NAMED_PIPE, 10, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_PIPE_SET_PIPE_ATTRIBUTE       CTL_CODE(FILE_DEVICE_NAMED_PIPE, 11, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_PIPE_GET_CONNECTION_ATTRIBUTE CTL_CODE(FILE_DEVICE_NAMED_PIPE, 12, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_PIPE_SET_CONNECTION_ATTRIBUTE CTL_CODE(FILE_DEVICE_NAMED_PIPE, 13, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_PIPE_GET_HANDLE_ATTRIBUTE     CTL_CODE(FILE_DEVICE_NAMED_PIPE, 14, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_PIPE_SET_HANDLE_ATTRIBUTE     CTL_CODE(FILE_DEVICE_NAMED_PIPE, 15, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_PIPE_FLUSH                    CTL_CODE(FILE_DEVICE_NAMED_PIPE, 16, METHOD_BUFFERED, FILE_WRITE_DATA)

#define FSCTL_PIPE_INTERNAL_READ            CTL_CODE(FILE_DEVICE_NAMED_PIPE, 2045, METHOD_BUFFERED, FILE_READ_DATA)
#define FSCTL_PIPE_INTERNAL_WRITE           CTL_CODE(FILE_DEVICE_NAMED_PIPE, 2046, METHOD_BUFFERED, FILE_WRITE_DATA)
#define FSCTL_PIPE_INTERNAL_TRANSCEIVE      CTL_CODE(FILE_DEVICE_NAMED_PIPE, 2047, METHOD_NEITHER, FILE_READ_DATA | FILE_WRITE_DATA)
#define FSCTL_PIPE_INTERNAL_READ_OVFLOW     CTL_CODE(FILE_DEVICE_NAMED_PIPE, 2048, METHOD_BUFFERED, FILE_READ_DATA)

#define FILE_PIPE_READ_DATA                 0x00000000
#define FILE_PIPE_WRITE_SPACE               0x00000001

typedef struct _FILE_PIPE_ASSIGN_EVENT_BUFFER {
  HANDLE EventHandle;
  ULONG KeyValue;
} FILE_PIPE_ASSIGN_EVENT_BUFFER, *PFILE_PIPE_ASSIGN_EVENT_BUFFER;

typedef struct _FILE_PIPE_EVENT_BUFFER {
  ULONG NamedPipeState;
  ULONG EntryType;
  ULONG ByteCount;
  ULONG KeyValue;
  ULONG NumberRequests;
} FILE_PIPE_EVENT_BUFFER, *PFILE_PIPE_EVENT_BUFFER;

typedef struct _FILE_PIPE_PEEK_BUFFER {
  ULONG NamedPipeState;
  ULONG ReadDataAvailable;
  ULONG NumberOfMessages;
  ULONG MessageLength;
  CHAR Data[1];
} FILE_PIPE_PEEK_BUFFER, *PFILE_PIPE_PEEK_BUFFER;

typedef struct _FILE_PIPE_WAIT_FOR_BUFFER {
  LARGE_INTEGER Timeout;
  ULONG NameLength;
  BOOLEAN TimeoutSpecified;
  WCHAR Name[1];
} FILE_PIPE_WAIT_FOR_BUFFER, *PFILE_PIPE_WAIT_FOR_BUFFER;

typedef struct _FILE_PIPE_CLIENT_PROCESS_BUFFER {
#if !defined(BUILD_WOW6432)
  PVOID ClientSession;
  PVOID ClientProcess;
#else
  ULONGLONG ClientSession;
  ULONGLONG ClientProcess;
#endif
} FILE_PIPE_CLIENT_PROCESS_BUFFER, *PFILE_PIPE_CLIENT_PROCESS_BUFFER;

#define FILE_PIPE_COMPUTER_NAME_LENGTH 15

typedef struct _FILE_PIPE_CLIENT_PROCESS_BUFFER_EX {
#if !defined(BUILD_WOW6432)
  PVOID ClientSession;
  PVOID ClientProcess;
#else
  ULONGLONG ClientSession;
  ULONGLONG ClientProcess;
#endif
  USHORT ClientComputerNameLength;
  WCHAR ClientComputerBuffer[FILE_PIPE_COMPUTER_NAME_LENGTH+1];
} FILE_PIPE_CLIENT_PROCESS_BUFFER_EX, *PFILE_PIPE_CLIENT_PROCESS_BUFFER_EX;

#define FSCTL_MAILSLOT_PEEK             CTL_CODE(FILE_DEVICE_MAILSLOT, 0, METHOD_NEITHER, FILE_READ_DATA)

typedef enum _LINK_TRACKING_INFORMATION_TYPE {
  NtfsLinkTrackingInformation,
  DfsLinkTrackingInformation
} LINK_TRACKING_INFORMATION_TYPE, *PLINK_TRACKING_INFORMATION_TYPE;

typedef struct _LINK_TRACKING_INFORMATION {
  LINK_TRACKING_INFORMATION_TYPE Type;
  UCHAR VolumeId[16];
} LINK_TRACKING_INFORMATION, *PLINK_TRACKING_INFORMATION;

typedef struct _REMOTE_LINK_TRACKING_INFORMATION {
  PVOID TargetFileObject;
  ULONG TargetLinkTrackingInformationLength;
  UCHAR TargetLinkTrackingInformationBuffer[1];
} REMOTE_LINK_TRACKING_INFORMATION, *PREMOTE_LINK_TRACKING_INFORMATION;

#define IO_OPEN_PAGING_FILE                 0x0002
#define IO_OPEN_TARGET_DIRECTORY            0x0004
#define IO_STOP_ON_SYMLINK                  0x0008
#define IO_MM_PAGING_FILE                   0x0010

typedef VOID
(NTAPI *PDRIVER_FS_NOTIFICATION) (
  _In_ PDEVICE_OBJECT DeviceObject,
  _In_ BOOLEAN FsActive);

typedef enum _FS_FILTER_SECTION_SYNC_TYPE {
  SyncTypeOther = 0,
  SyncTypeCreateSection
} FS_FILTER_SECTION_SYNC_TYPE, *PFS_FILTER_SECTION_SYNC_TYPE;

typedef enum _FS_FILTER_STREAM_FO_NOTIFICATION_TYPE {
  NotifyTypeCreate = 0,
  NotifyTypeRetired
} FS_FILTER_STREAM_FO_NOTIFICATION_TYPE, *PFS_FILTER_STREAM_FO_NOTIFICATION_TYPE;

typedef union _FS_FILTER_PARAMETERS {
  struct {
    PLARGE_INTEGER EndingOffset;
    PERESOURCE *ResourceToRelease;
  } AcquireForModifiedPageWriter;
  struct {
    PERESOURCE ResourceToRelease;
  } ReleaseForModifiedPageWriter;
  struct {
    FS_FILTER_SECTION_SYNC_TYPE SyncType;
    ULONG PageProtection;
  } AcquireForSectionSynchronization;
  struct {
    FS_FILTER_STREAM_FO_NOTIFICATION_TYPE NotificationType;
    BOOLEAN POINTER_ALIGNMENT SafeToRecurse;
  } NotifyStreamFileObject;
  struct {
    PVOID Argument1;
    PVOID Argument2;
    PVOID Argument3;
    PVOID Argument4;
    PVOID Argument5;
  } Others;
} FS_FILTER_PARAMETERS, *PFS_FILTER_PARAMETERS;

#define FS_FILTER_ACQUIRE_FOR_SECTION_SYNCHRONIZATION      (UCHAR)-1
#define FS_FILTER_RELEASE_FOR_SECTION_SYNCHRONIZATION      (UCHAR)-2
#define FS_FILTER_ACQUIRE_FOR_MOD_WRITE                    (UCHAR)-3
#define FS_FILTER_RELEASE_FOR_MOD_WRITE                    (UCHAR)-4
#define FS_FILTER_ACQUIRE_FOR_CC_FLUSH                     (UCHAR)-5
#define FS_FILTER_RELEASE_FOR_CC_FLUSH                     (UCHAR)-6

typedef struct _FS_FILTER_CALLBACK_DATA {
  ULONG SizeOfFsFilterCallbackData;
  UCHAR Operation;
  UCHAR Reserved;
  struct _DEVICE_OBJECT *DeviceObject;
  struct _FILE_OBJECT *FileObject;
  FS_FILTER_PARAMETERS Parameters;
} FS_FILTER_CALLBACK_DATA, *PFS_FILTER_CALLBACK_DATA;

typedef NTSTATUS
(NTAPI *PFS_FILTER_CALLBACK) (
  _In_ PFS_FILTER_CALLBACK_DATA Data,
  _Out_ PVOID *CompletionContext);

typedef VOID
(NTAPI *PFS_FILTER_COMPLETION_CALLBACK) (
  _In_ PFS_FILTER_CALLBACK_DATA Data,
  _In_ NTSTATUS OperationStatus,
  _In_ PVOID CompletionContext);

typedef struct _FS_FILTER_CALLBACKS {
  ULONG SizeOfFsFilterCallbacks;
  ULONG Reserved;
  PFS_FILTER_CALLBACK PreAcquireForSectionSynchronization;
  PFS_FILTER_COMPLETION_CALLBACK PostAcquireForSectionSynchronization;
  PFS_FILTER_CALLBACK PreReleaseForSectionSynchronization;
  PFS_FILTER_COMPLETION_CALLBACK PostReleaseForSectionSynchronization;
  PFS_FILTER_CALLBACK PreAcquireForCcFlush;
  PFS_FILTER_COMPLETION_CALLBACK PostAcquireForCcFlush;
  PFS_FILTER_CALLBACK PreReleaseForCcFlush;
  PFS_FILTER_COMPLETION_CALLBACK PostReleaseForCcFlush;
  PFS_FILTER_CALLBACK PreAcquireForModifiedPageWriter;
  PFS_FILTER_COMPLETION_CALLBACK PostAcquireForModifiedPageWriter;
  PFS_FILTER_CALLBACK PreReleaseForModifiedPageWriter;
  PFS_FILTER_COMPLETION_CALLBACK PostReleaseForModifiedPageWriter;
} FS_FILTER_CALLBACKS, *PFS_FILTER_CALLBACKS;

extern NTKERNELAPI KSPIN_LOCK    IoStatisticsLock;
extern NTKERNELAPI ULONG         IoReadOperationCount;
extern NTKERNELAPI ULONG         IoWriteOperationCount;
extern NTKERNELAPI ULONG         IoOtherOperationCount;
extern NTKERNELAPI LARGE_INTEGER IoReadTransferCount;
extern NTKERNELAPI LARGE_INTEGER IoWriteTransferCount;
extern NTKERNELAPI LARGE_INTEGER IoOtherTransferCount;

#define IO_FILE_OBJECT_NON_PAGED_POOL_CHARGE    64
#define IO_FILE_OBJECT_PAGED_POOL_CHARGE        1024

#if (NTDDI_VERSION >= NTDDI_VISTA)
typedef struct _IO_PRIORITY_INFO {
  ULONG Size;
  ULONG ThreadPriority;
  ULONG PagePriority;
  IO_PRIORITY_HINT IoPriority;
} IO_PRIORITY_INFO, *PIO_PRIORITY_INFO;
#endif

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

#define SYSTEM_PAGE_PRIORITY_BITS       3
#define SYSTEM_PAGE_PRIORITY_LEVELS     (1 << SYSTEM_PAGE_PRIORITY_BITS)

/******************************************************************************
 *                              Kernel Types                                  *
 ******************************************************************************/
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


/******************************************************************************
 *                              Kernel Functions                              *
 ******************************************************************************/

NTSTATUS
NTAPI
KeGetProcessorNumberFromIndex(
  _In_ ULONG ProcIndex,
  _Out_ PPROCESSOR_NUMBER ProcNumber);

ULONG
NTAPI
KeGetProcessorIndexFromNumber(
  _In_ PPROCESSOR_NUMBER ProcNumber);

#if (NTDDI_VERSION >= NTDDI_WIN2K)




NTKERNELAPI
VOID
NTAPI
KeInitializeMutant(
  _Out_ PRKMUTANT Mutant,
  _In_ BOOLEAN InitialOwner);

_IRQL_requires_max_(DISPATCH_LEVEL)
NTKERNELAPI
LONG
NTAPI
KeReadStateMutant(
  _In_ PRKMUTANT Mutant);

_When_(Wait==0, _IRQL_requires_max_(DISPATCH_LEVEL))
_When_(Wait==1, _IRQL_requires_max_(APC_LEVEL))
NTKERNELAPI
LONG
NTAPI
KeReleaseMutant(
  _Inout_ PRKMUTANT Mutant,
  _In_ KPRIORITY Increment,
  _In_ BOOLEAN Abandoned,
  _In_ BOOLEAN Wait);

NTKERNELAPI
VOID
NTAPI
KeInitializeQueue(
  _Out_ PRKQUEUE Queue,
  _In_ ULONG Count);

_IRQL_requires_max_(DISPATCH_LEVEL)
NTKERNELAPI
LONG
NTAPI
KeReadStateQueue(
  _In_ PRKQUEUE Queue);

_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
NTKERNELAPI
LONG
NTAPI
KeInsertQueue(
  _Inout_ PRKQUEUE Queue,
  _Inout_ PLIST_ENTRY Entry);

_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
NTKERNELAPI
LONG
NTAPI
KeInsertHeadQueue(
  _Inout_ PRKQUEUE Queue,
  _Inout_ PLIST_ENTRY Entry);

_IRQL_requires_min_(PASSIVE_LEVEL)
_When_((Timeout==NULL || Timeout->QuadPart!=0), _IRQL_requires_max_(APC_LEVEL))
_When_((Timeout!=NULL && Timeout->QuadPart==0), _IRQL_requires_max_(DISPATCH_LEVEL))
NTKERNELAPI
PLIST_ENTRY
NTAPI
KeRemoveQueue(
  _Inout_ PRKQUEUE Queue,
  _In_ KPROCESSOR_MODE WaitMode,
  _In_opt_ PLARGE_INTEGER Timeout);

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
VOID
NTAPI
KeAttachProcess(
  _Inout_ PKPROCESS Process);

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
VOID
NTAPI
KeDetachProcess(VOID);

_IRQL_requires_max_(DISPATCH_LEVEL)
NTKERNELAPI
PLIST_ENTRY
NTAPI
KeRundownQueue(
  _Inout_ PRKQUEUE Queue);

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
VOID
NTAPI
KeStackAttachProcess(
  _Inout_ PKPROCESS Process,
  _Out_ PKAPC_STATE ApcState);

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
VOID
NTAPI
KeUnstackDetachProcess(
  _In_ PKAPC_STATE ApcState);

_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
NTKERNELAPI
UCHAR
NTAPI
KeSetIdealProcessorThread(
  _Inout_ PKTHREAD Thread,
  _In_ UCHAR Processor);

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
BOOLEAN
NTAPI
KeSetKernelStackSwapEnable(
  _In_ BOOLEAN Enable);

#if defined(_X86_)
_Requires_lock_not_held_(*SpinLock)
_Acquires_lock_(*SpinLock)
_IRQL_raises_(SYNCH_LEVEL)
_IRQL_saves_
NTHALAPI
KIRQL
FASTCALL
KeAcquireSpinLockRaiseToSynch(
  _Inout_ PKSPIN_LOCK SpinLock);
#else
_Requires_lock_not_held_(*SpinLock)
_Acquires_lock_(*SpinLock)
_IRQL_raises_(SYNCH_LEVEL)
_IRQL_saves_
NTKERNELAPI
KIRQL
KeAcquireSpinLockRaiseToSynch(
  _Inout_ PKSPIN_LOCK SpinLock);
#endif

#endif /* (NTDDI_VERSION >= NTDDI_WIN2K) */

#if (NTDDI_VERSION >= NTDDI_WINXP)


_Requires_lock_not_held_(Number)
_Acquires_lock_(Number)
_IRQL_raises_(DISPATCH_LEVEL)
_DECL_HAL_KE_IMPORT
KIRQL
FASTCALL
KeAcquireQueuedSpinLock(
  _In_ KSPIN_LOCK_QUEUE_NUMBER Number);

_Requires_lock_held_(Number)
_Releases_lock_(Number)
_DECL_HAL_KE_IMPORT
VOID
FASTCALL
KeReleaseQueuedSpinLock(
  _In_ KSPIN_LOCK_QUEUE_NUMBER Number,
  _In_ KIRQL OldIrql);

_Must_inspect_result_
_Post_satisfies_(return == 1 || return == 0)
_DECL_HAL_KE_IMPORT
LOGICAL
FASTCALL
KeTryToAcquireQueuedSpinLock(
  _In_ KSPIN_LOCK_QUEUE_NUMBER Number,
  _Out_ _At_(*OldIrql, _IRQL_saves_) PKIRQL OldIrql);

#endif /* (NTDDI_VERSION >= NTDDI_WINXP) */





#if (NTDDI_VERSION >= NTDDI_VISTA)

_IRQL_requires_max_(DISPATCH_LEVEL)
NTKERNELAPI
VOID
KeQueryOwnerMutant(
  _In_ PKMUTANT Mutant,
  _Out_ PCLIENT_ID ClientId);

_IRQL_requires_min_(PASSIVE_LEVEL)
_When_((Timeout==NULL || *Timeout!=0), _IRQL_requires_max_(APC_LEVEL))
_When_((Timeout!=NULL && *Timeout==0), _IRQL_requires_max_(DISPATCH_LEVEL))
NTKERNELAPI
ULONG
NTAPI
KeRemoveQueueEx(
  _Inout_ PKQUEUE Queue,
  _In_ KPROCESSOR_MODE WaitMode,
  _In_ BOOLEAN Alertable,
  _In_opt_ PLARGE_INTEGER Timeout,
  _Out_writes_to_(Count, return) PLIST_ENTRY *EntryArray,
  _In_ ULONG Count);

#endif /* (NTDDI_VERSION >= NTDDI_VISTA) */


#define INVALID_PROCESSOR_INDEX     0xffffffff

#define EX_PUSH_LOCK ULONG_PTR
#define PEX_PUSH_LOCK PULONG_PTR
/******************************************************************************
 *                          Executive Functions                               *
 ******************************************************************************/


#define ExDisableResourceBoost ExDisableResourceBoostLite

VOID
NTAPI
ExInitializePushLock(
  _Out_ PEX_PUSH_LOCK PushLock);

#if (NTDDI_VERSION >= NTDDI_WIN2K)

_IRQL_requires_max_(DISPATCH_LEVEL)
NTKERNELAPI
SIZE_T
NTAPI
ExQueryPoolBlockSize(
  _In_ PVOID PoolBlock,
  _Out_ PBOOLEAN QuotaCharged);

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
ExAdjustLookasideDepth(VOID);

_IRQL_requires_max_(DISPATCH_LEVEL)
NTKERNELAPI
VOID
NTAPI
ExDisableResourceBoostLite(
  _In_ PERESOURCE Resource);
#endif /* (NTDDI_VERSION >= NTDDI_WIN2K) */

#if (NTDDI_VERSION >= NTDDI_WINXP)

PSLIST_ENTRY
FASTCALL
InterlockedPushListSList(
  _Inout_ PSLIST_HEADER ListHead,
  _Inout_ __drv_aliasesMem PSLIST_ENTRY List,
  _Inout_ PSLIST_ENTRY ListEnd,
  _In_ ULONG Count);
#endif /* (NTDDI_VERSION >= NTDDI_WINXP) */

/******************************************************************************
 *                            Security Manager Functions                      *
 ******************************************************************************/

#if (NTDDI_VERSION >= NTDDI_WIN2K)


NTKERNELAPI
VOID
NTAPI
SeReleaseSubjectContext(
  _Inout_ PSECURITY_SUBJECT_CONTEXT SubjectContext);

NTKERNELAPI
BOOLEAN
NTAPI
SePrivilegeCheck(
  _Inout_ PPRIVILEGE_SET RequiredPrivileges,
  _In_ PSECURITY_SUBJECT_CONTEXT SubjectContext,
  _In_ KPROCESSOR_MODE AccessMode);

NTKERNELAPI
VOID
NTAPI
SeOpenObjectAuditAlarm(
  _In_ PUNICODE_STRING ObjectTypeName,
  _In_opt_ PVOID Object,
  _In_opt_ PUNICODE_STRING AbsoluteObjectName,
  _In_ PSECURITY_DESCRIPTOR SecurityDescriptor,
  _In_ PACCESS_STATE AccessState,
  _In_ BOOLEAN ObjectCreated,
  _In_ BOOLEAN AccessGranted,
  _In_ KPROCESSOR_MODE AccessMode,
  _Out_ PBOOLEAN GenerateOnClose);

NTKERNELAPI
VOID
NTAPI
SeOpenObjectForDeleteAuditAlarm(
  _In_ PUNICODE_STRING ObjectTypeName,
  _In_opt_ PVOID Object,
  _In_opt_ PUNICODE_STRING AbsoluteObjectName,
  _In_ PSECURITY_DESCRIPTOR SecurityDescriptor,
  _In_ PACCESS_STATE AccessState,
  _In_ BOOLEAN ObjectCreated,
  _In_ BOOLEAN AccessGranted,
  _In_ KPROCESSOR_MODE AccessMode,
  _Out_ PBOOLEAN GenerateOnClose);

NTKERNELAPI
VOID
NTAPI
SeDeleteObjectAuditAlarm(
  _In_ PVOID Object,
  _In_ HANDLE Handle);

NTKERNELAPI
TOKEN_TYPE
NTAPI
SeTokenType(
  _In_ PACCESS_TOKEN Token);

NTKERNELAPI
BOOLEAN
NTAPI
SeTokenIsAdmin(
  _In_ PACCESS_TOKEN Token);

NTKERNELAPI
BOOLEAN
NTAPI
SeTokenIsRestricted(
  _In_ PACCESS_TOKEN Token);

NTKERNELAPI
NTSTATUS
NTAPI
SeQueryAuthenticationIdToken(
  _In_ PACCESS_TOKEN Token,
  _Out_ PLUID AuthenticationId);

NTKERNELAPI
NTSTATUS
NTAPI
SeQuerySessionIdToken(
  _In_ PACCESS_TOKEN Token,
  _Out_ PULONG SessionId);

NTKERNELAPI
NTSTATUS
NTAPI
SeCreateClientSecurity(
  _In_ PETHREAD ClientThread,
  _In_ PSECURITY_QUALITY_OF_SERVICE ClientSecurityQos,
  _In_ BOOLEAN RemoteSession,
  _Out_ PSECURITY_CLIENT_CONTEXT ClientContext);

NTKERNELAPI
VOID
NTAPI
SeImpersonateClient(
  _In_ PSECURITY_CLIENT_CONTEXT ClientContext,
  _In_opt_ PETHREAD ServerThread);

NTKERNELAPI
NTSTATUS
NTAPI
SeImpersonateClientEx(
  _In_ PSECURITY_CLIENT_CONTEXT ClientContext,
  _In_opt_ PETHREAD ServerThread);

NTKERNELAPI
NTSTATUS
NTAPI
SeCreateClientSecurityFromSubjectContext(
  _In_ PSECURITY_SUBJECT_CONTEXT SubjectContext,
  _In_ PSECURITY_QUALITY_OF_SERVICE ClientSecurityQos,
  _In_ BOOLEAN ServerIsRemote,
  _Out_ PSECURITY_CLIENT_CONTEXT ClientContext);

NTKERNELAPI
NTSTATUS
NTAPI
SeQuerySecurityDescriptorInfo(
  _In_ PSECURITY_INFORMATION SecurityInformation,
  _Out_writes_bytes_(*Length) PSECURITY_DESCRIPTOR SecurityDescriptor,
  _Inout_ PULONG Length,
  _Inout_ PSECURITY_DESCRIPTOR *ObjectsSecurityDescriptor);

NTKERNELAPI
NTSTATUS
NTAPI
SeSetSecurityDescriptorInfo(
  _In_opt_ PVOID Object,
  _In_ PSECURITY_INFORMATION SecurityInformation,
  _In_ PSECURITY_DESCRIPTOR SecurityDescriptor,
  _Inout_ PSECURITY_DESCRIPTOR *ObjectsSecurityDescriptor,
  _In_ POOL_TYPE PoolType,
  _In_ PGENERIC_MAPPING GenericMapping);

NTKERNELAPI
NTSTATUS
NTAPI
SeSetSecurityDescriptorInfoEx(
  _In_opt_ PVOID Object,
  _In_ PSECURITY_INFORMATION SecurityInformation,
  _In_ PSECURITY_DESCRIPTOR ModificationDescriptor,
  _Inout_ PSECURITY_DESCRIPTOR *ObjectsSecurityDescriptor,
  _In_ ULONG AutoInheritFlags,
  _In_ POOL_TYPE PoolType,
  _In_ PGENERIC_MAPPING GenericMapping);

NTKERNELAPI
NTSTATUS
NTAPI
SeAppendPrivileges(
  _Inout_ PACCESS_STATE AccessState,
  _In_ PPRIVILEGE_SET Privileges);

NTKERNELAPI
BOOLEAN
NTAPI
SeAuditingFileEvents(
  _In_ BOOLEAN AccessGranted,
  _In_ PSECURITY_DESCRIPTOR SecurityDescriptor);

NTKERNELAPI
BOOLEAN
NTAPI
SeAuditingFileOrGlobalEvents(
  _In_ BOOLEAN AccessGranted,
  _In_ PSECURITY_DESCRIPTOR SecurityDescriptor,
  _In_ PSECURITY_SUBJECT_CONTEXT SubjectSecurityContext);

VOID
NTAPI
SeSetAccessStateGenericMapping(
  _Inout_ PACCESS_STATE AccessState,
  _In_ PGENERIC_MAPPING GenericMapping);

NTKERNELAPI
NTSTATUS
NTAPI
SeRegisterLogonSessionTerminatedRoutine(
  _In_ PSE_LOGON_SESSION_TERMINATED_ROUTINE CallbackRoutine);

NTKERNELAPI
NTSTATUS
NTAPI
SeUnregisterLogonSessionTerminatedRoutine(
  _In_ PSE_LOGON_SESSION_TERMINATED_ROUTINE CallbackRoutine);

NTKERNELAPI
NTSTATUS
NTAPI
SeMarkLogonSessionForTerminationNotification(
  _In_ PLUID LogonId);

NTKERNELAPI
NTSTATUS
NTAPI
SeQueryInformationToken(
  _In_ PACCESS_TOKEN Token,
  _In_ TOKEN_INFORMATION_CLASS TokenInformationClass,
  _Outptr_result_buffer_(_Inexpressible_(token-dependent)) PVOID *TokenInformation);

#endif /* (NTDDI_VERSION >= NTDDI_WIN2K) */
#if (NTDDI_VERSION >= NTDDI_WIN2KSP3)
NTKERNELAPI
BOOLEAN
NTAPI
SeAuditingHardLinkEvents(
  _In_ BOOLEAN AccessGranted,
  _In_ PSECURITY_DESCRIPTOR SecurityDescriptor);
#endif

#if (NTDDI_VERSION >= NTDDI_WINXP)

NTKERNELAPI
NTSTATUS
NTAPI
SeFilterToken(
  _In_ PACCESS_TOKEN ExistingToken,
  _In_ ULONG Flags,
  _In_opt_ PTOKEN_GROUPS SidsToDisable,
  _In_opt_ PTOKEN_PRIVILEGES PrivilegesToDelete,
  _In_opt_ PTOKEN_GROUPS RestrictedSids,
  _Outptr_ PACCESS_TOKEN *FilteredToken);

NTKERNELAPI
VOID
NTAPI
SeAuditHardLinkCreation(
  _In_ PUNICODE_STRING FileName,
  _In_ PUNICODE_STRING LinkName,
  _In_ BOOLEAN bSuccess);

#endif /* (NTDDI_VERSION >= NTDDI_WINXP) */

#if (NTDDI_VERSION >= NTDDI_WINXPSP2)

NTKERNELAPI
BOOLEAN
NTAPI
SeAuditingFileEventsWithContext(
  _In_ BOOLEAN AccessGranted,
  _In_ PSECURITY_DESCRIPTOR SecurityDescriptor,
  _In_opt_ PSECURITY_SUBJECT_CONTEXT SubjectSecurityContext);

NTKERNELAPI
BOOLEAN
NTAPI
SeAuditingHardLinkEventsWithContext(
  _In_ BOOLEAN AccessGranted,
  _In_ PSECURITY_DESCRIPTOR SecurityDescriptor,
  _In_opt_ PSECURITY_SUBJECT_CONTEXT SubjectSecurityContext);

#endif


#if (NTDDI_VERSION >= NTDDI_VISTA)

NTKERNELAPI
VOID
NTAPI
SeOpenObjectAuditAlarmWithTransaction(
  _In_ PUNICODE_STRING ObjectTypeName,
  _In_opt_ PVOID Object,
  _In_opt_ PUNICODE_STRING AbsoluteObjectName,
  _In_ PSECURITY_DESCRIPTOR SecurityDescriptor,
  _In_ PACCESS_STATE AccessState,
  _In_ BOOLEAN ObjectCreated,
  _In_ BOOLEAN AccessGranted,
  _In_ KPROCESSOR_MODE AccessMode,
  _In_opt_ GUID *TransactionId,
  _Out_ PBOOLEAN GenerateOnClose);

NTKERNELAPI
VOID
NTAPI
SeOpenObjectForDeleteAuditAlarmWithTransaction(
  _In_ PUNICODE_STRING ObjectTypeName,
  _In_opt_ PVOID Object,
  _In_opt_ PUNICODE_STRING AbsoluteObjectName,
  _In_ PSECURITY_DESCRIPTOR SecurityDescriptor,
  _In_ PACCESS_STATE AccessState,
  _In_ BOOLEAN ObjectCreated,
  _In_ BOOLEAN AccessGranted,
  _In_ KPROCESSOR_MODE AccessMode,
  _In_opt_ GUID *TransactionId,
  _Out_ PBOOLEAN GenerateOnClose);

NTKERNELAPI
VOID
NTAPI
SeExamineSacl(
  _In_ PACL Sacl,
  _In_ PACCESS_TOKEN Token,
  _In_ ACCESS_MASK DesiredAccess,
  _In_ BOOLEAN AccessGranted,
  _Out_ PBOOLEAN GenerateAudit,
  _Out_ PBOOLEAN GenerateAlarm);

NTKERNELAPI
VOID
NTAPI
SeDeleteObjectAuditAlarmWithTransaction(
  _In_ PVOID Object,
  _In_ HANDLE Handle,
  _In_opt_ GUID *TransactionId);

NTKERNELAPI
VOID
NTAPI
SeQueryTokenIntegrity(
  _In_ PACCESS_TOKEN Token,
  _Inout_ PSID_AND_ATTRIBUTES IntegritySA);

NTKERNELAPI
NTSTATUS
NTAPI
SeSetSessionIdToken(
  _In_ PACCESS_TOKEN Token,
  _In_ ULONG SessionId);

NTKERNELAPI
VOID
NTAPI
SeAuditHardLinkCreationWithTransaction(
  _In_ PUNICODE_STRING FileName,
  _In_ PUNICODE_STRING LinkName,
  _In_ BOOLEAN bSuccess,
  _In_opt_ GUID *TransactionId);

NTKERNELAPI
VOID
NTAPI
SeAuditTransactionStateChange(
  _In_ GUID *TransactionId,
  _In_ GUID *ResourceManagerId,
  _In_ ULONG NewTransactionState);
#endif /* (NTDDI_VERSION >= NTDDI_VISTA) */

#if (NTDDI_VERSION >= NTDDI_VISTA || (NTDDI_VERSION >= NTDDI_WINXPSP2 && NTDDI_VERSION < NTDDI_WS03))
NTKERNELAPI
BOOLEAN
NTAPI
SeTokenIsWriteRestricted(
  _In_ PACCESS_TOKEN Token);
#endif

#if (NTDDI_VERSION >= NTDDI_WIN7)

NTKERNELAPI
BOOLEAN
NTAPI
SeAuditingAnyFileEventsWithContext(
  _In_ PSECURITY_DESCRIPTOR SecurityDescriptor,
  _In_opt_ PSECURITY_SUBJECT_CONTEXT SubjectSecurityContext,
  _Out_opt_ PBOOLEAN StagingEnabled);

NTKERNELAPI
VOID
NTAPI
SeExamineGlobalSacl(
  _In_ PUNICODE_STRING ObjectType,
  _In_ PACL ResourceSacl,
  _In_ PACCESS_TOKEN Token,
  _In_ ACCESS_MASK DesiredAccess,
  _In_ BOOLEAN AccessGranted,
  _Inout_ PBOOLEAN GenerateAudit,
  _Inout_opt_ PBOOLEAN GenerateAlarm);

NTKERNELAPI
VOID
NTAPI
SeMaximumAuditMaskFromGlobalSacl(
  _In_opt_ PUNICODE_STRING ObjectTypeName,
  _In_ ACCESS_MASK GrantedAccess,
  _In_ PACCESS_TOKEN Token,
  _Inout_ PACCESS_MASK AuditMask);

#endif /* (NTDDI_VERSION >= NTDDI_WIN7) */

NTSTATUS
NTAPI
SeReportSecurityEventWithSubCategory(
  _In_ ULONG Flags,
  _In_ PUNICODE_STRING SourceName,
  _In_opt_ PSID UserSid,
  _In_ PSE_ADT_PARAMETER_ARRAY AuditParameters,
  _In_ ULONG AuditSubcategoryId);

BOOLEAN
NTAPI
SeAccessCheckFromState(
  _In_ PSECURITY_DESCRIPTOR SecurityDescriptor,
  _In_ PTOKEN_ACCESS_INFORMATION PrimaryTokenInformation,
  _In_opt_ PTOKEN_ACCESS_INFORMATION ClientTokenInformation,
  _In_ ACCESS_MASK DesiredAccess,
  _In_ ACCESS_MASK PreviouslyGrantedAccess,
  _Outptr_opt_result_maybenull_ PPRIVILEGE_SET *Privileges,
  _In_ PGENERIC_MAPPING GenericMapping,
  _In_ KPROCESSOR_MODE AccessMode,
  _Out_ PACCESS_MASK GrantedAccess,
  _Out_ PNTSTATUS AccessStatus);

NTKERNELAPI
VOID
NTAPI
SeFreePrivileges(
  _In_ PPRIVILEGE_SET Privileges);

NTSTATUS
NTAPI
SeLocateProcessImageName(
  _Inout_ PEPROCESS Process,
  _Outptr_ PUNICODE_STRING *pImageFileName);

#define SeLengthSid( Sid ) \
    (8 + (4 * ((SID *)Sid)->SubAuthorityCount))

#define SeDeleteClientSecurity(C)  {                                           \
            if (SeTokenType((C)->ClientToken) == TokenPrimary) {               \
                PsDereferencePrimaryToken( (C)->ClientToken );                 \
            } else {                                                           \
                PsDereferenceImpersonationToken( (C)->ClientToken );           \
            }                                                                  \
}

#define SeStopImpersonatingClient() PsRevertToSelf()

#define SeQuerySubjectContextToken( SubjectContext )                \
    ( ARGUMENT_PRESENT(                                             \
        ((PSECURITY_SUBJECT_CONTEXT) SubjectContext)->ClientToken   \
        ) ?                                                         \
    ((PSECURITY_SUBJECT_CONTEXT) SubjectContext)->ClientToken :     \
    ((PSECURITY_SUBJECT_CONTEXT) SubjectContext)->PrimaryToken )

extern NTKERNELAPI PSE_EXPORTS SeExports;

/******************************************************************************
 *                          Process Manager Functions                         *
 ******************************************************************************/

_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
NTSTATUS
NTAPI
PsLookupProcessByProcessId(
  _In_ HANDLE ProcessId,
  _Outptr_ PEPROCESS *Process);

_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
NTSTATUS
NTAPI
PsLookupThreadByThreadId(
  _In_ HANDLE UniqueThreadId,
  _Outptr_ PETHREAD *Thread);

#if (NTDDI_VERSION >= NTDDI_WIN2K)


_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
PACCESS_TOKEN
NTAPI
PsReferenceImpersonationToken(
  _Inout_ PETHREAD Thread,
  _Out_ PBOOLEAN CopyOnOpen,
  _Out_ PBOOLEAN EffectiveOnly,
  _Out_ PSECURITY_IMPERSONATION_LEVEL ImpersonationLevel);

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
LARGE_INTEGER
NTAPI
PsGetProcessExitTime(VOID);

_IRQL_requires_max_(DISPATCH_LEVEL)
NTKERNELAPI
BOOLEAN
NTAPI
PsIsThreadTerminating(
  _In_ PETHREAD Thread);

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
NTKERNELAPI
NTSTATUS
NTAPI
PsImpersonateClient(
  _Inout_ PETHREAD Thread,
  _In_opt_ PACCESS_TOKEN Token,
  _In_ BOOLEAN CopyOnOpen,
  _In_ BOOLEAN EffectiveOnly,
  _In_ SECURITY_IMPERSONATION_LEVEL ImpersonationLevel);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTKERNELAPI
BOOLEAN
NTAPI
PsDisableImpersonation(
  _Inout_ PETHREAD Thread,
  _Inout_ PSE_IMPERSONATION_STATE ImpersonationState);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTKERNELAPI
VOID
NTAPI
PsRestoreImpersonation(
  _Inout_ PETHREAD Thread,
  _In_ PSE_IMPERSONATION_STATE ImpersonationState);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTKERNELAPI
VOID
NTAPI
PsRevertToSelf(VOID);

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
VOID
NTAPI
PsChargePoolQuota(
  _In_ PEPROCESS Process,
  _In_ POOL_TYPE PoolType,
  _In_ ULONG_PTR Amount);

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
VOID
NTAPI
PsReturnPoolQuota(
  _In_ PEPROCESS Process,
  _In_ POOL_TYPE PoolType,
  _In_ ULONG_PTR Amount);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTKERNELAPI
NTSTATUS
NTAPI
PsAssignImpersonationToken(
  _In_ PETHREAD Thread,
  _In_opt_ HANDLE Token);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTKERNELAPI
HANDLE
NTAPI
PsReferencePrimaryToken(
  _Inout_ PEPROCESS Process);
#endif /* (NTDDI_VERSION >= NTDDI_WIN2K) */
#if (NTDDI_VERSION >= NTDDI_WINXP)


_IRQL_requires_max_(PASSIVE_LEVEL)
NTKERNELAPI
VOID
NTAPI
PsDereferencePrimaryToken(
  _In_ PACCESS_TOKEN PrimaryToken);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTKERNELAPI
VOID
NTAPI
PsDereferenceImpersonationToken(
  _In_ PACCESS_TOKEN ImpersonationToken);

_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
NTSTATUS
NTAPI
PsChargeProcessPoolQuota(
  _In_ PEPROCESS Process,
  _In_ POOL_TYPE PoolType,
  _In_ ULONG_PTR Amount);

NTKERNELAPI
BOOLEAN
NTAPI
PsIsSystemThread(
  _In_ PETHREAD Thread);
#endif /* (NTDDI_VERSION >= NTDDI_WINXP) */

/******************************************************************************
 *                         I/O Manager Functions                              *
 ******************************************************************************/

#define IoIsFileOpenedExclusively(FileObject) ( \
    (BOOLEAN) !(                                \
    (FileObject)->SharedRead ||                 \
    (FileObject)->SharedWrite ||                \
    (FileObject)->SharedDelete                  \
    )                                           \
)

#if (NTDDI_VERSION == NTDDI_WIN2K)
NTKERNELAPI
NTSTATUS
NTAPI
IoRegisterFsRegistrationChangeEx(
  _In_ PDRIVER_OBJECT DriverObject,
  _In_ PDRIVER_FS_NOTIFICATION DriverNotificationRoutine);
#endif
#if (NTDDI_VERSION >= NTDDI_WIN2K)


NTKERNELAPI
VOID
NTAPI
IoAcquireVpbSpinLock(
  _Out_ PKIRQL Irql);

NTKERNELAPI
NTSTATUS
NTAPI
IoCheckDesiredAccess(
  _Inout_ PACCESS_MASK DesiredAccess,
  _In_ ACCESS_MASK GrantedAccess);

NTKERNELAPI
NTSTATUS
NTAPI
IoCheckEaBufferValidity(
  _In_ PFILE_FULL_EA_INFORMATION EaBuffer,
  _In_ ULONG EaLength,
  _Out_ PULONG ErrorOffset);

NTKERNELAPI
NTSTATUS
NTAPI
IoCheckFunctionAccess(
  _In_ ACCESS_MASK GrantedAccess,
  _In_ UCHAR MajorFunction,
  _In_ UCHAR MinorFunction,
  _In_ ULONG IoControlCode,
  _In_opt_ PVOID Argument1,
  _In_opt_ PVOID Argument2);

NTKERNELAPI
NTSTATUS
NTAPI
IoCheckQuerySetFileInformation(
  _In_ FILE_INFORMATION_CLASS FileInformationClass,
  _In_ ULONG Length,
  _In_ BOOLEAN SetOperation);

NTKERNELAPI
NTSTATUS
NTAPI
IoCheckQuerySetVolumeInformation(
  _In_ FS_INFORMATION_CLASS FsInformationClass,
  _In_ ULONG Length,
  _In_ BOOLEAN SetOperation);

NTKERNELAPI
NTSTATUS
NTAPI
IoCheckQuotaBufferValidity(
  _In_ PFILE_QUOTA_INFORMATION QuotaBuffer,
  _In_ ULONG QuotaLength,
  _Out_ PULONG ErrorOffset);

NTKERNELAPI
PFILE_OBJECT
NTAPI
IoCreateStreamFileObject(
  _In_opt_ PFILE_OBJECT FileObject,
  _In_opt_ PDEVICE_OBJECT DeviceObject);

NTKERNELAPI
PFILE_OBJECT
NTAPI
IoCreateStreamFileObjectLite(
  _In_opt_ PFILE_OBJECT FileObject,
  _In_opt_ PDEVICE_OBJECT DeviceObject);

NTKERNELAPI
BOOLEAN
NTAPI
IoFastQueryNetworkAttributes(
  _In_ POBJECT_ATTRIBUTES ObjectAttributes,
  _In_ ACCESS_MASK DesiredAccess,
  _In_ ULONG OpenOptions,
  _Out_ PIO_STATUS_BLOCK IoStatus,
  _Out_ PFILE_NETWORK_OPEN_INFORMATION Buffer);

NTKERNELAPI
NTSTATUS
NTAPI
IoPageRead(
  _In_ PFILE_OBJECT FileObject,
  _In_ PMDL Mdl,
  _In_ PLARGE_INTEGER Offset,
  _In_ PKEVENT Event,
  _Out_ PIO_STATUS_BLOCK IoStatusBlock);

NTKERNELAPI
PDEVICE_OBJECT
NTAPI
IoGetBaseFileSystemDeviceObject(
  _In_ PFILE_OBJECT FileObject);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTKERNELAPI
PCONFIGURATION_INFORMATION
NTAPI
IoGetConfigurationInformation(VOID);

NTKERNELAPI
ULONG
NTAPI
IoGetRequestorProcessId(
  _In_ PIRP Irp);

NTKERNELAPI
PEPROCESS
NTAPI
IoGetRequestorProcess(
  _In_ PIRP Irp);

NTKERNELAPI
PIRP
NTAPI
IoGetTopLevelIrp(VOID);

NTKERNELAPI
BOOLEAN
NTAPI
IoIsOperationSynchronous(
  _In_ PIRP Irp);

NTKERNELAPI
BOOLEAN
NTAPI
IoIsSystemThread(
  _In_ PETHREAD Thread);

NTKERNELAPI
BOOLEAN
NTAPI
IoIsValidNameGraftingBuffer(
  _In_ PIRP Irp,
  _In_ PREPARSE_DATA_BUFFER ReparseBuffer);

NTKERNELAPI
NTSTATUS
NTAPI
IoQueryFileInformation(
  _In_ PFILE_OBJECT FileObject,
  _In_ FILE_INFORMATION_CLASS FileInformationClass,
  _In_ ULONG Length,
  _Out_ PVOID FileInformation,
  _Out_ PULONG ReturnedLength);

NTKERNELAPI
NTSTATUS
NTAPI
IoQueryVolumeInformation(
  _In_ PFILE_OBJECT FileObject,
  _In_ FS_INFORMATION_CLASS FsInformationClass,
  _In_ ULONG Length,
  _Out_ PVOID FsInformation,
  _Out_ PULONG ReturnedLength);

NTKERNELAPI
VOID
NTAPI
IoQueueThreadIrp(
  _In_ PIRP Irp);

NTKERNELAPI
VOID
NTAPI
IoRegisterFileSystem(
  _In_ __drv_aliasesMem PDEVICE_OBJECT DeviceObject);

NTKERNELAPI
NTSTATUS
NTAPI
IoRegisterFsRegistrationChange(
  _In_ PDRIVER_OBJECT DriverObject,
  _In_ PDRIVER_FS_NOTIFICATION DriverNotificationRoutine);

NTKERNELAPI
VOID
NTAPI
IoReleaseVpbSpinLock(
  _In_ KIRQL Irql);

NTKERNELAPI
VOID
NTAPI
IoSetDeviceToVerify(
  _In_ PETHREAD Thread,
  _In_opt_ PDEVICE_OBJECT DeviceObject);

NTKERNELAPI
NTSTATUS
NTAPI
IoSetInformation(
  _In_ PFILE_OBJECT FileObject,
  _In_ FILE_INFORMATION_CLASS FileInformationClass,
  _In_ ULONG Length,
  _In_ PVOID FileInformation);

NTKERNELAPI
VOID
NTAPI
IoSetTopLevelIrp(
  _In_opt_ PIRP Irp);

NTKERNELAPI
NTSTATUS
NTAPI
IoSynchronousPageWrite(
  _In_ PFILE_OBJECT FileObject,
  _In_ PMDL Mdl,
  _In_ PLARGE_INTEGER FileOffset,
  _In_ PKEVENT Event,
  _Out_ PIO_STATUS_BLOCK IoStatusBlock);

NTKERNELAPI
PEPROCESS
NTAPI
IoThreadToProcess(
  _In_ PETHREAD Thread);

NTKERNELAPI
VOID
NTAPI
IoUnregisterFileSystem(
  _In_ PDEVICE_OBJECT DeviceObject);

NTKERNELAPI
VOID
NTAPI
IoUnregisterFsRegistrationChange(
  _In_ PDRIVER_OBJECT DriverObject,
  _In_ PDRIVER_FS_NOTIFICATION DriverNotificationRoutine);

NTKERNELAPI
NTSTATUS
NTAPI
IoVerifyVolume(
  _In_ PDEVICE_OBJECT DeviceObject,
  _In_ BOOLEAN AllowRawMount);

NTKERNELAPI
NTSTATUS
NTAPI
IoGetRequestorSessionId(
  _In_ PIRP Irp,
  _Out_ PULONG pSessionId);

#endif /* (NTDDI_VERSION >= NTDDI_WIN2K) */


#if (NTDDI_VERSION >= NTDDI_WINXP)


NTKERNELAPI
PFILE_OBJECT
NTAPI
IoCreateStreamFileObjectEx(
  _In_opt_ PFILE_OBJECT FileObject,
  _In_opt_ PDEVICE_OBJECT DeviceObject,
  _Out_opt_ PHANDLE FileObjectHandle);

NTKERNELAPI
NTSTATUS
NTAPI
IoQueryFileDosDeviceName(
  _In_ PFILE_OBJECT FileObject,
  _Out_ POBJECT_NAME_INFORMATION *ObjectNameInformation);

NTKERNELAPI
NTSTATUS
NTAPI
IoEnumerateDeviceObjectList(
  _In_ PDRIVER_OBJECT DriverObject,
  _Out_writes_bytes_to_opt_(DeviceObjectListSize,(*ActualNumberDeviceObjects)*sizeof(PDEVICE_OBJECT))
    PDEVICE_OBJECT *DeviceObjectList,
  _In_ ULONG DeviceObjectListSize,
  _Out_ PULONG ActualNumberDeviceObjects);

NTKERNELAPI
PDEVICE_OBJECT
NTAPI
IoGetLowerDeviceObject(
  _In_ PDEVICE_OBJECT DeviceObject);

NTKERNELAPI
PDEVICE_OBJECT
NTAPI
IoGetDeviceAttachmentBaseRef(
  _In_ PDEVICE_OBJECT DeviceObject);

NTKERNELAPI
NTSTATUS
NTAPI
IoGetDiskDeviceObject(
  _In_ PDEVICE_OBJECT FileSystemDeviceObject,
  _Out_ PDEVICE_OBJECT *DiskDeviceObject);

#endif /* (NTDDI_VERSION >= NTDDI_WINXP) */

#if (NTDDI_VERSION >= NTDDI_WS03SP1)


NTKERNELAPI
NTSTATUS
NTAPI
IoEnumerateRegisteredFiltersList(
  _Out_writes_bytes_to_opt_(DriverObjectListSize,(*ActualNumberDriverObjects)*sizeof(PDRIVER_OBJECT))
    PDRIVER_OBJECT *DriverObjectList,
  _In_ ULONG DriverObjectListSize,
  _Out_ PULONG ActualNumberDriverObjects);
#endif /* (NTDDI_VERSION >= NTDDI_WS03SP1) */

#if (NTDDI_VERSION >= NTDDI_VISTA)

FORCEINLINE
VOID
NTAPI
IoInitializePriorityInfo(
    _In_ PIO_PRIORITY_INFO PriorityInfo)
{
    PriorityInfo->Size = sizeof(IO_PRIORITY_INFO);
    PriorityInfo->ThreadPriority = 0xffff;
    PriorityInfo->IoPriority = IoPriorityNormal;
    PriorityInfo->PagePriority = 0;
}
#endif /* (NTDDI_VERSION >= NTDDI_VISTA) */

#if (NTDDI_VERSION >= NTDDI_WIN7)


NTKERNELAPI
NTSTATUS
NTAPI
IoRegisterFsRegistrationChangeMountAware(
  _In_ PDRIVER_OBJECT DriverObject,
  _In_ PDRIVER_FS_NOTIFICATION DriverNotificationRoutine,
  _In_ BOOLEAN SynchronizeWithMounts);

NTKERNELAPI
NTSTATUS
NTAPI
IoReplaceFileObjectName(
  _In_ PFILE_OBJECT FileObject,
  _In_reads_bytes_(FileNameLength) PWSTR NewFileName,
  _In_ USHORT FileNameLength);
#endif /* (NTDDI_VERSION >= NTDDI_WIN7) */


#define PO_CB_SYSTEM_POWER_POLICY       0
#define PO_CB_AC_STATUS                 1
#define PO_CB_BUTTON_COLLISION          2
#define PO_CB_SYSTEM_STATE_LOCK         3
#define PO_CB_LID_SWITCH_STATE          4
#define PO_CB_PROCESSOR_POWER_POLICY    5


#if (NTDDI_VERSION >= NTDDI_WINXP)
_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
NTSTATUS
NTAPI
PoQueueShutdownWorkItem(
  _Inout_ __drv_aliasesMem PWORK_QUEUE_ITEM WorkItem);
#endif
/******************************************************************************
 *                         Memory manager Types                               *
 ******************************************************************************/
typedef enum _MMFLUSH_TYPE {
  MmFlushForDelete,
  MmFlushForWrite
} MMFLUSH_TYPE;

typedef struct _READ_LIST {
  PFILE_OBJECT FileObject;
  ULONG NumberOfEntries;
  LOGICAL IsImage;
  FILE_SEGMENT_ELEMENT List[ANYSIZE_ARRAY];
} READ_LIST, *PREAD_LIST;

#if (NTDDI_VERSION >= NTDDI_WINXP)

typedef union _MM_PREFETCH_FLAGS {
  struct {
    ULONG Priority : SYSTEM_PAGE_PRIORITY_BITS;
    ULONG RepurposePriority : SYSTEM_PAGE_PRIORITY_BITS;
  } Flags;
  ULONG AllFlags;
} MM_PREFETCH_FLAGS, *PMM_PREFETCH_FLAGS;

#define MM_PREFETCH_FLAGS_MASK ((1 << (2*SYSTEM_PAGE_PRIORITY_BITS)) - 1)

#endif /* (NTDDI_VERSION >= NTDDI_WINXP) */

#define HEAP_NO_SERIALIZE               0x00000001
#define HEAP_GROWABLE                   0x00000002
#define HEAP_GENERATE_EXCEPTIONS        0x00000004
#define HEAP_ZERO_MEMORY                0x00000008
#define HEAP_REALLOC_IN_PLACE_ONLY      0x00000010
#define HEAP_TAIL_CHECKING_ENABLED      0x00000020
#define HEAP_FREE_CHECKING_ENABLED      0x00000040
#define HEAP_DISABLE_COALESCE_ON_FREE   0x00000080

#define HEAP_CREATE_ALIGN_16            0x00010000
#define HEAP_CREATE_ENABLE_TRACING      0x00020000
#define HEAP_CREATE_ENABLE_EXECUTE      0x00040000

#define HEAP_SETTABLE_USER_VALUE        0x00000100
#define HEAP_SETTABLE_USER_FLAG1        0x00000200
#define HEAP_SETTABLE_USER_FLAG2        0x00000400
#define HEAP_SETTABLE_USER_FLAG3        0x00000800
#define HEAP_SETTABLE_USER_FLAGS        0x00000E00

#define HEAP_CLASS_0                    0x00000000
#define HEAP_CLASS_1                    0x00001000
#define HEAP_CLASS_2                    0x00002000
#define HEAP_CLASS_3                    0x00003000
#define HEAP_CLASS_4                    0x00004000
#define HEAP_CLASS_5                    0x00005000
#define HEAP_CLASS_6                    0x00006000
#define HEAP_CLASS_7                    0x00007000
#define HEAP_CLASS_8                    0x00008000
#define HEAP_CLASS_MASK                 0x0000F000

#define HEAP_MAXIMUM_TAG                0x0FFF
#define HEAP_GLOBAL_TAG                 0x0800
#define HEAP_PSEUDO_TAG_FLAG            0x8000
#define HEAP_TAG_SHIFT                  18
#define HEAP_TAG_MASK                  (HEAP_MAXIMUM_TAG << HEAP_TAG_SHIFT)

#define HEAP_CREATE_VALID_MASK         (HEAP_NO_SERIALIZE             |   \
                                        HEAP_GROWABLE                 |   \
                                        HEAP_GENERATE_EXCEPTIONS      |   \
                                        HEAP_ZERO_MEMORY              |   \
                                        HEAP_REALLOC_IN_PLACE_ONLY    |   \
                                        HEAP_TAIL_CHECKING_ENABLED    |   \
                                        HEAP_FREE_CHECKING_ENABLED    |   \
                                        HEAP_DISABLE_COALESCE_ON_FREE |   \
                                        HEAP_CLASS_MASK               |   \
                                        HEAP_CREATE_ALIGN_16          |   \
                                        HEAP_CREATE_ENABLE_TRACING    |   \
                                        HEAP_CREATE_ENABLE_EXECUTE)

/******************************************************************************
 *                       Memory manager Functions                             *
 ******************************************************************************/

FORCEINLINE
ULONG
HEAP_MAKE_TAG_FLAGS(
  _In_ ULONG TagBase,
  _In_ ULONG Tag)
{
  //__assume_bound(TagBase); // FIXME
  return ((ULONG)((TagBase) + ((Tag) << HEAP_TAG_SHIFT)));
}

#if (NTDDI_VERSION >= NTDDI_WIN2K)

NTKERNELAPI
BOOLEAN
NTAPI
MmIsRecursiveIoFault(VOID);

_IRQL_requires_max_ (APC_LEVEL)
NTKERNELAPI
BOOLEAN
NTAPI
MmForceSectionClosed(
  _In_ PSECTION_OBJECT_POINTERS SectionObjectPointer,
  _In_ BOOLEAN DelayClose);

_IRQL_requires_max_ (APC_LEVEL)
NTKERNELAPI
BOOLEAN
NTAPI
MmFlushImageSection(
  _In_ PSECTION_OBJECT_POINTERS SectionObjectPointer,
  _In_ MMFLUSH_TYPE FlushType);

_IRQL_requires_max_ (APC_LEVEL)
NTKERNELAPI
BOOLEAN
NTAPI
MmCanFileBeTruncated(
  _In_ PSECTION_OBJECT_POINTERS SectionObjectPointer,
  _In_opt_ PLARGE_INTEGER NewFileSize);

_IRQL_requires_max_ (APC_LEVEL)
NTKERNELAPI
BOOLEAN
NTAPI
MmSetAddressRangeModified(
  _In_reads_bytes_ (Length) PVOID Address,
  _In_ SIZE_T Length);

#endif /* (NTDDI_VERSION >= NTDDI_WIN2K) */

#if (NTDDI_VERSION >= NTDDI_WINXP)


_IRQL_requires_max_ (PASSIVE_LEVEL)
NTKERNELAPI
NTSTATUS
NTAPI
MmPrefetchPages(
  _In_ ULONG NumberOfLists,
  _In_reads_ (NumberOfLists) PREAD_LIST *ReadLists);

#endif /* (NTDDI_VERSION >= NTDDI_WINXP) */


#if (NTDDI_VERSION >= NTDDI_VISTA)

_IRQL_requires_max_ (APC_LEVEL)
NTKERNELAPI
ULONG
NTAPI
MmDoesFileHaveUserWritableReferences(
  _In_ PSECTION_OBJECT_POINTERS SectionPointer);

_Must_inspect_result_
_At_(*BaseAddress, __drv_allocatesMem(Mem))
__kernel_entry
NTSYSCALLAPI
NTSTATUS
NTAPI
NtAllocateVirtualMemory(
    _In_ HANDLE ProcessHandle,
    _Inout_ _Outptr_result_buffer_(*RegionSize) PVOID *BaseAddress,
    _In_ ULONG_PTR ZeroBits,
    _Inout_ PSIZE_T RegionSize,
    _In_ ULONG AllocationType,
    _In_ ULONG Protect);

__kernel_entry
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSCALLAPI
NTSTATUS
NTAPI
NtFreeVirtualMemory(
    _In_ HANDLE ProcessHandle,
    _Inout_ __drv_freesMem(Mem) PVOID *BaseAddress,
    _Inout_ PSIZE_T RegionSize,
    _In_ ULONG FreeType);

#endif /* (NTDDI_VERSION >= NTDDI_VISTA) */


#if (NTDDI_VERSION >= NTDDI_WIN2K)

NTKERNELAPI
NTSTATUS
NTAPI
ObInsertObject(
  _In_ PVOID Object,
  _Inout_opt_ PACCESS_STATE PassedAccessState,
  _In_opt_ ACCESS_MASK DesiredAccess,
  _In_ ULONG ObjectPointerBias,
  _Out_opt_ PVOID *NewObject,
  _Out_opt_ PHANDLE Handle);

NTKERNELAPI
NTSTATUS
NTAPI
ObOpenObjectByPointer(
  _In_ PVOID Object,
  _In_ ULONG HandleAttributes,
  _In_opt_ PACCESS_STATE PassedAccessState,
  _In_ ACCESS_MASK DesiredAccess,
  _In_opt_ POBJECT_TYPE ObjectType,
  _In_ KPROCESSOR_MODE AccessMode,
  _Out_ PHANDLE Handle);

NTKERNELAPI
VOID
NTAPI
ObMakeTemporaryObject(
  _In_ PVOID Object);

NTKERNELAPI
NTSTATUS
NTAPI
ObQueryNameString(
  _In_ PVOID Object,
  _Out_writes_bytes_opt_(Length) POBJECT_NAME_INFORMATION ObjectNameInfo,
  _In_ ULONG Length,
  _Out_ PULONG ReturnLength);

NTKERNELAPI
NTSTATUS
NTAPI
ObQueryObjectAuditingByHandle(
  _In_ HANDLE Handle,
  _Out_ PBOOLEAN GenerateOnClose);
#endif /* (NTDDI_VERSION >= NTDDI_WIN2K) */

#if (NTDDI_VERSION >= NTDDI_VISTA)

NTKERNELAPI
BOOLEAN
NTAPI
ObIsKernelHandle(
  _In_ HANDLE Handle);
#endif


#if (NTDDI_VERSION >= NTDDI_WIN7)

NTKERNELAPI
NTSTATUS
NTAPI
ObOpenObjectByPointerWithTag(
  _In_ PVOID Object,
  _In_ ULONG HandleAttributes,
  _In_opt_ PACCESS_STATE PassedAccessState,
  _In_ ACCESS_MASK DesiredAccess,
  _In_opt_ POBJECT_TYPE ObjectType,
  _In_ KPROCESSOR_MODE AccessMode,
  _In_ ULONG Tag,
  _Out_ PHANDLE Handle);

NTKERNELAPI
ULONG
NTAPI
ObGetObjectPointerCount(
    _In_ PVOID Object
);

#endif /* (NTDDI_VERSION >= NTDDI_WIN7) */

/* FSRTL Types */

typedef ULONG LBN;
typedef LBN *PLBN;

typedef ULONG VBN;
typedef VBN *PVBN;

#define FSRTL_COMMON_FCB_HEADER_LAYOUT \
  CSHORT NodeTypeCode; \
  CSHORT NodeByteSize; \
  UCHAR Flags; \
  UCHAR IsFastIoPossible; \
  UCHAR Flags2; \
  UCHAR Reserved:4; \
  UCHAR Version:4; \
  PERESOURCE Resource; \
  PERESOURCE PagingIoResource; \
  LARGE_INTEGER AllocationSize; \
  LARGE_INTEGER FileSize; \
  LARGE_INTEGER ValidDataLength;

typedef struct _FSRTL_COMMON_FCB_HEADER {
  FSRTL_COMMON_FCB_HEADER_LAYOUT
} FSRTL_COMMON_FCB_HEADER, *PFSRTL_COMMON_FCB_HEADER;

#ifdef __cplusplus
typedef struct _FSRTL_ADVANCED_FCB_HEADER:FSRTL_COMMON_FCB_HEADER {
#else /* __cplusplus */
typedef struct _FSRTL_ADVANCED_FCB_HEADER {
  FSRTL_COMMON_FCB_HEADER_LAYOUT
#endif  /* __cplusplus */
  PFAST_MUTEX FastMutex;
  LIST_ENTRY FilterContexts;
#if (NTDDI_VERSION >= NTDDI_VISTA)
  EX_PUSH_LOCK PushLock;
  PVOID *FileContextSupportPointer;
#endif
} FSRTL_ADVANCED_FCB_HEADER, *PFSRTL_ADVANCED_FCB_HEADER;

#define FSRTL_FCB_HEADER_V0             (0x00)
#define FSRTL_FCB_HEADER_V1             (0x01)

#define FSRTL_FLAG_FILE_MODIFIED        (0x01)
#define FSRTL_FLAG_FILE_LENGTH_CHANGED  (0x02)
#define FSRTL_FLAG_LIMIT_MODIFIED_PAGES (0x04)
#define FSRTL_FLAG_ACQUIRE_MAIN_RSRC_EX (0x08)
#define FSRTL_FLAG_ACQUIRE_MAIN_RSRC_SH (0x10)
#define FSRTL_FLAG_USER_MAPPED_FILE     (0x20)
#define FSRTL_FLAG_ADVANCED_HEADER      (0x40)
#define FSRTL_FLAG_EOF_ADVANCE_ACTIVE   (0x80)

#define FSRTL_FLAG2_DO_MODIFIED_WRITE        (0x01)
#define FSRTL_FLAG2_SUPPORTS_FILTER_CONTEXTS (0x02)
#define FSRTL_FLAG2_PURGE_WHEN_MAPPED        (0x04)
#define FSRTL_FLAG2_IS_PAGING_FILE           (0x08)

#define FSRTL_FSP_TOP_LEVEL_IRP         (0x01)
#define FSRTL_CACHE_TOP_LEVEL_IRP       (0x02)
#define FSRTL_MOD_WRITE_TOP_LEVEL_IRP   (0x03)
#define FSRTL_FAST_IO_TOP_LEVEL_IRP     (0x04)
#define FSRTL_NETWORK1_TOP_LEVEL_IRP    ((LONG_PTR)0x05)
#define FSRTL_NETWORK2_TOP_LEVEL_IRP    ((LONG_PTR)0x06)
#define FSRTL_MAX_TOP_LEVEL_IRP_FLAG    ((LONG_PTR)0xFFFF)

typedef struct _FSRTL_AUXILIARY_BUFFER {
  PVOID Buffer;
  ULONG Length;
  ULONG Flags;
  PMDL Mdl;
} FSRTL_AUXILIARY_BUFFER, *PFSRTL_AUXILIARY_BUFFER;

#define FSRTL_AUXILIARY_FLAG_DEALLOCATE 0x00000001

typedef enum _FSRTL_COMPARISON_RESULT {
  LessThan = -1,
  EqualTo = 0,
  GreaterThan = 1
} FSRTL_COMPARISON_RESULT;

#define FSRTL_FAT_LEGAL                 0x01
#define FSRTL_HPFS_LEGAL                0x02
#define FSRTL_NTFS_LEGAL                0x04
#define FSRTL_WILD_CHARACTER            0x08
#define FSRTL_OLE_LEGAL                 0x10
#define FSRTL_NTFS_STREAM_LEGAL         (FSRTL_NTFS_LEGAL | FSRTL_OLE_LEGAL)

#define FSRTL_VOLUME_DISMOUNT           1
#define FSRTL_VOLUME_DISMOUNT_FAILED    2
#define FSRTL_VOLUME_LOCK               3
#define FSRTL_VOLUME_LOCK_FAILED        4
#define FSRTL_VOLUME_UNLOCK             5
#define FSRTL_VOLUME_MOUNT              6
#define FSRTL_VOLUME_NEEDS_CHKDSK       7
#define FSRTL_VOLUME_WORM_NEAR_FULL     8
#define FSRTL_VOLUME_WEARING_OUT        9
#define FSRTL_VOLUME_FORCED_CLOSED      10
#define FSRTL_VOLUME_INFO_MAKE_COMPAT   11
#define FSRTL_VOLUME_PREPARING_EJECT    12
#define FSRTL_VOLUME_CHANGE_SIZE        13
#define FSRTL_VOLUME_BACKGROUND_FORMAT  14

typedef VOID
(NTAPI *PFSRTL_STACK_OVERFLOW_ROUTINE)(
  _In_ PVOID Context,
  _In_ PKEVENT Event);

#if (NTDDI_VERSION >= NTDDI_VISTA)

#define FSRTL_UNC_PROVIDER_FLAGS_MAILSLOTS_SUPPORTED    0x00000001
#define FSRTL_UNC_PROVIDER_FLAGS_CSC_ENABLED            0x00000002
#define FSRTL_UNC_PROVIDER_FLAGS_DOMAIN_SVC_AWARE       0x00000004

#define FSRTL_ALLOCATE_ECPLIST_FLAG_CHARGE_QUOTA        0x00000001

#define FSRTL_ALLOCATE_ECP_FLAG_CHARGE_QUOTA            0x00000001
#define FSRTL_ALLOCATE_ECP_FLAG_NONPAGED_POOL           0x00000002

#define FSRTL_ECP_LOOKASIDE_FLAG_NONPAGED_POOL          0x00000002

#define FSRTL_VIRTDISK_FULLY_ALLOCATED  0x00000001
#define FSRTL_VIRTDISK_NO_DRIVE_LETTER  0x00000002

typedef struct _FSRTL_MUP_PROVIDER_INFO_LEVEL_1 {
  ULONG32 ProviderId;
} FSRTL_MUP_PROVIDER_INFO_LEVEL_1, *PFSRTL_MUP_PROVIDER_INFO_LEVEL_1;

typedef struct _FSRTL_MUP_PROVIDER_INFO_LEVEL_2 {
  ULONG32 ProviderId;
  UNICODE_STRING ProviderName;
} FSRTL_MUP_PROVIDER_INFO_LEVEL_2, *PFSRTL_MUP_PROVIDER_INFO_LEVEL_2;

typedef VOID
(*PFSRTL_EXTRA_CREATE_PARAMETER_CLEANUP_CALLBACK) (
  _Inout_ PVOID EcpContext,
  _In_ LPCGUID EcpType);

typedef struct _ECP_LIST ECP_LIST, *PECP_LIST;

typedef ULONG FSRTL_ALLOCATE_ECPLIST_FLAGS;
typedef ULONG FSRTL_ALLOCATE_ECP_FLAGS;
typedef ULONG FSRTL_ECP_LOOKASIDE_FLAGS;

typedef enum _FSRTL_CHANGE_BACKING_TYPE {
  ChangeDataControlArea,
  ChangeImageControlArea,
  ChangeSharedCacheMap
} FSRTL_CHANGE_BACKING_TYPE, *PFSRTL_CHANGE_BACKING_TYPE;

#endif /* (NTDDI_VERSION >= NTDDI_VISTA) */

typedef struct _FSRTL_PER_FILE_CONTEXT {
  LIST_ENTRY Links;
  PVOID OwnerId;
  PVOID InstanceId;
  PFREE_FUNCTION FreeCallback;
} FSRTL_PER_FILE_CONTEXT, *PFSRTL_PER_FILE_CONTEXT;

typedef struct _FSRTL_PER_STREAM_CONTEXT {
  LIST_ENTRY Links;
  PVOID OwnerId;
  PVOID InstanceId;
  PFREE_FUNCTION FreeCallback;
} FSRTL_PER_STREAM_CONTEXT, *PFSRTL_PER_STREAM_CONTEXT;

#if (NTDDI_VERSION >= NTDDI_WIN2K)
typedef VOID
(*PFN_FSRTLTEARDOWNPERSTREAMCONTEXTS) (
  _In_ PFSRTL_ADVANCED_FCB_HEADER AdvancedHeader);
#endif

typedef struct _FSRTL_PER_FILEOBJECT_CONTEXT {
  LIST_ENTRY Links;
  PVOID OwnerId;
  PVOID InstanceId;
} FSRTL_PER_FILEOBJECT_CONTEXT, *PFSRTL_PER_FILEOBJECT_CONTEXT;

#define FSRTL_CC_FLUSH_ERROR_FLAG_NO_HARD_ERROR  0x1
#define FSRTL_CC_FLUSH_ERROR_FLAG_NO_LOG_ENTRY   0x2

typedef NTSTATUS
(NTAPI *PCOMPLETE_LOCK_IRP_ROUTINE) (
  _In_ PVOID Context,
  _In_ PIRP Irp);

typedef struct _FILE_LOCK_INFO {
  LARGE_INTEGER StartingByte;
  LARGE_INTEGER Length;
  BOOLEAN ExclusiveLock;
  ULONG Key;
  PFILE_OBJECT FileObject;
  PVOID ProcessId;
  LARGE_INTEGER EndingByte;
} FILE_LOCK_INFO, *PFILE_LOCK_INFO;

typedef VOID
(NTAPI *PUNLOCK_ROUTINE) (
  _In_ PVOID Context,
  _In_ PFILE_LOCK_INFO FileLockInfo);

typedef struct _FILE_LOCK {
  PCOMPLETE_LOCK_IRP_ROUTINE CompleteLockIrpRoutine;
  PUNLOCK_ROUTINE UnlockRoutine;
  BOOLEAN FastIoIsQuestionable;
  BOOLEAN SpareC[3];
  PVOID LockInformation;
  FILE_LOCK_INFO LastReturnedLockInfo;
  PVOID LastReturnedLock;
  LONG volatile LockRequestsInProgress;
} FILE_LOCK, *PFILE_LOCK;

typedef struct _TUNNEL {
  FAST_MUTEX Mutex;
  PRTL_SPLAY_LINKS Cache;
  LIST_ENTRY TimerQueue;
  USHORT NumEntries;
} TUNNEL, *PTUNNEL;

typedef struct _BASE_MCB {
  ULONG MaximumPairCount;
  ULONG PairCount;
  USHORT PoolType;
  USHORT Flags;
  PVOID Mapping;
} BASE_MCB, *PBASE_MCB;

typedef struct _LARGE_MCB {
  PKGUARDED_MUTEX GuardedMutex;
  BASE_MCB BaseMcb;
} LARGE_MCB, *PLARGE_MCB;

#define MCB_FLAG_RAISE_ON_ALLOCATION_FAILURE 1

typedef struct _MCB {
  LARGE_MCB DummyFieldThatSizesThisStructureCorrectly;
} MCB, *PMCB;

typedef enum _FAST_IO_POSSIBLE {
  FastIoIsNotPossible = 0,
  FastIoIsPossible,
  FastIoIsQuestionable
} FAST_IO_POSSIBLE;

typedef struct _EOF_WAIT_BLOCK {
  LIST_ENTRY EofWaitLinks;
  KEVENT Event;
} EOF_WAIT_BLOCK, *PEOF_WAIT_BLOCK;

typedef PVOID OPLOCK, *POPLOCK;

typedef VOID
(NTAPI *POPLOCK_WAIT_COMPLETE_ROUTINE) (
  _In_ PVOID Context,
  _In_ PIRP Irp);

typedef VOID
(NTAPI *POPLOCK_FS_PREPOST_IRP) (
  _In_ PVOID Context,
  _In_ PIRP Irp);

#if (NTDDI_VERSION >= NTDDI_VISTASP1)
#define OPLOCK_FLAG_COMPLETE_IF_OPLOCKED    0x00000001
#endif

#if (NTDDI_VERSION >= NTDDI_WIN7)
#define OPLOCK_FLAG_OPLOCK_KEY_CHECK_ONLY   0x00000002
#define OPLOCK_FLAG_BACK_OUT_ATOMIC_OPLOCK  0x00000004
#define OPLOCK_FLAG_IGNORE_OPLOCK_KEYS      0x00000008
#define OPLOCK_FSCTRL_FLAG_ALL_KEYS_MATCH   0x00000001
#endif

#if (NTDDI_VERSION >= NTDDI_WIN7)

typedef struct _OPLOCK_KEY_ECP_CONTEXT {
  GUID OplockKey;
  ULONG Reserved;
} OPLOCK_KEY_ECP_CONTEXT, *POPLOCK_KEY_ECP_CONTEXT;

DEFINE_GUID(GUID_ECP_OPLOCK_KEY, 0x48850596, 0x3050, 0x4be7, 0x98, 0x63, 0xfe, 0xc3, 0x50, 0xce, 0x8d, 0x7f);

#endif

typedef PVOID PNOTIFY_SYNC;

#if (NTDDI_VERSION >= NTDDI_WIN7)
typedef struct _ECP_HEADER ECP_HEADER, *PECP_HEADER;
#endif

typedef BOOLEAN
(NTAPI *PCHECK_FOR_TRAVERSE_ACCESS) (
  _In_ PVOID NotifyContext,
  _In_opt_ PVOID TargetContext,
  _In_ PSECURITY_SUBJECT_CONTEXT SubjectContext);

typedef BOOLEAN
(NTAPI *PFILTER_REPORT_CHANGE) (
  _In_ PVOID NotifyContext,
  _In_ PVOID FilterContext);
/* FSRTL Functions */

#define FsRtlEnterFileSystem    KeEnterCriticalRegion
#define FsRtlExitFileSystem     KeLeaveCriticalRegion

#if (NTDDI_VERSION >= NTDDI_WIN2K)

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
NTKERNELAPI
BOOLEAN
NTAPI
FsRtlCopyRead(
  _In_ PFILE_OBJECT FileObject,
  _In_ PLARGE_INTEGER FileOffset,
  _In_ ULONG Length,
  _In_ BOOLEAN Wait,
  _In_ ULONG LockKey,
  _Out_writes_bytes_(Length) PVOID Buffer,
  _Out_ PIO_STATUS_BLOCK IoStatus,
  _In_ PDEVICE_OBJECT DeviceObject);

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
NTKERNELAPI
BOOLEAN
NTAPI
FsRtlCopyWrite(
  _In_ PFILE_OBJECT FileObject,
  _In_ PLARGE_INTEGER FileOffset,
  _In_ ULONG Length,
  _In_ BOOLEAN Wait,
  _In_ ULONG LockKey,
  _In_reads_bytes_(Length) PVOID Buffer,
  _Out_ PIO_STATUS_BLOCK IoStatus,
  _In_ PDEVICE_OBJECT DeviceObject);

_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
BOOLEAN
NTAPI
FsRtlMdlReadDev(
  _In_ PFILE_OBJECT FileObject,
  _In_ PLARGE_INTEGER FileOffset,
  _In_ ULONG Length,
  _In_ ULONG LockKey,
  _Outptr_ PMDL *MdlChain,
  _Out_ PIO_STATUS_BLOCK IoStatus,
  _In_opt_ PDEVICE_OBJECT DeviceObject);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTKERNELAPI
BOOLEAN
NTAPI
FsRtlMdlReadCompleteDev(
  _In_ PFILE_OBJECT FileObject,
  _In_ PMDL MdlChain,
  _In_opt_ PDEVICE_OBJECT DeviceObject);

_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
BOOLEAN
NTAPI
FsRtlPrepareMdlWriteDev(
  _In_ PFILE_OBJECT FileObject,
  _In_ PLARGE_INTEGER FileOffset,
  _In_ ULONG Length,
  _In_ ULONG LockKey,
  _Outptr_ PMDL *MdlChain,
  _Out_ PIO_STATUS_BLOCK IoStatus,
  _In_ PDEVICE_OBJECT DeviceObject);

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
NTKERNELAPI
BOOLEAN
NTAPI
FsRtlMdlWriteCompleteDev(
  _In_ PFILE_OBJECT FileObject,
  _In_ PLARGE_INTEGER FileOffset,
  _In_ PMDL MdlChain,
  _In_opt_ PDEVICE_OBJECT DeviceObject);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTKERNELAPI
VOID
NTAPI
FsRtlAcquireFileExclusive(
  _In_ PFILE_OBJECT FileObject);

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
VOID
NTAPI
FsRtlReleaseFile(
  _In_ PFILE_OBJECT FileObject);

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
NTKERNELAPI
NTSTATUS
NTAPI
FsRtlGetFileSize(
  _In_ PFILE_OBJECT FileObject,
  _Out_ PLARGE_INTEGER FileSize);

_Must_inspect_result_
NTKERNELAPI
BOOLEAN
NTAPI
FsRtlIsTotalDeviceFailure(
  _In_ NTSTATUS Status);

_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
PFILE_LOCK
NTAPI
FsRtlAllocateFileLock(
  _In_opt_ PCOMPLETE_LOCK_IRP_ROUTINE CompleteLockIrpRoutine,
  _In_opt_ PUNLOCK_ROUTINE UnlockRoutine);

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
VOID
NTAPI
FsRtlFreeFileLock(
  _In_ PFILE_LOCK FileLock);

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
VOID
NTAPI
FsRtlInitializeFileLock(
  _Out_ PFILE_LOCK FileLock,
  _In_opt_ PCOMPLETE_LOCK_IRP_ROUTINE CompleteLockIrpRoutine,
  _In_opt_ PUNLOCK_ROUTINE UnlockRoutine);

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
VOID
NTAPI
FsRtlUninitializeFileLock(
  _Inout_ PFILE_LOCK FileLock);

/*
  FsRtlProcessFileLock:

  ret:
    -STATUS_INVALID_DEVICE_REQUEST
    -STATUS_RANGE_NOT_LOCKED from unlock routines.
    -STATUS_PENDING, STATUS_LOCK_NOT_GRANTED from FsRtlPrivateLock
    (redirected IoStatus->Status).

  Internals:
    -switch ( Irp->CurrentStackLocation->MinorFunction )
        lock: return FsRtlPrivateLock;
        unlocksingle: return FsRtlFastUnlockSingle;
        unlockall: return FsRtlFastUnlockAll;
        unlockallbykey: return FsRtlFastUnlockAllByKey;
        default: IofCompleteRequest with STATUS_INVALID_DEVICE_REQUEST;
                 return STATUS_INVALID_DEVICE_REQUEST;

    -'AllwaysZero' is passed thru as 'AllwaysZero' to lock / unlock routines.
    -'Irp' is passet thru as 'Irp' to FsRtlPrivateLock.
*/
_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
NTSTATUS
NTAPI
FsRtlProcessFileLock(
  _In_ PFILE_LOCK FileLock,
  _In_ PIRP Irp,
  _In_opt_ PVOID Context);

/*
  FsRtlCheckLockForReadAccess:

  All this really does is pick out the lock parameters from the irp (io stack
  location?), get IoGetRequestorProcess, and pass values on to
  FsRtlFastCheckLockForRead.
*/
_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
BOOLEAN
NTAPI
FsRtlCheckLockForReadAccess(
  _In_ PFILE_LOCK FileLock,
  _In_ PIRP Irp);

/*
  FsRtlCheckLockForWriteAccess:

  All this really does is pick out the lock parameters from the irp (io stack
  location?), get IoGetRequestorProcess, and pass values on to
  FsRtlFastCheckLockForWrite.
*/
_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
BOOLEAN
NTAPI
FsRtlCheckLockForWriteAccess(
  _In_ PFILE_LOCK FileLock,
  _In_ PIRP Irp);

_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
BOOLEAN
NTAPI
FsRtlFastCheckLockForRead(
  _In_ PFILE_LOCK FileLock,
  _In_ PLARGE_INTEGER FileOffset,
  _In_ PLARGE_INTEGER Length,
  _In_ ULONG Key,
  _In_ PFILE_OBJECT FileObject,
  _In_ PVOID Process);

_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
BOOLEAN
NTAPI
FsRtlFastCheckLockForWrite(
  _In_ PFILE_LOCK FileLock,
  _In_ PLARGE_INTEGER FileOffset,
  _In_ PLARGE_INTEGER Length,
  _In_ ULONG Key,
  _In_ PFILE_OBJECT FileObject,
  _In_ PVOID Process);

/*
  FsRtlGetNextFileLock:

  ret: NULL if no more locks

  Internals:
    FsRtlGetNextFileLock uses FileLock->LastReturnedLockInfo and
    FileLock->LastReturnedLock as storage.
    LastReturnedLock is a pointer to the 'raw' lock inkl. double linked
    list, and FsRtlGetNextFileLock needs this to get next lock on subsequent
    calls with Restart = FALSE.
*/
_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
PFILE_LOCK_INFO
NTAPI
FsRtlGetNextFileLock(
  _In_ PFILE_LOCK FileLock,
  _In_ BOOLEAN Restart);

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
NTSTATUS
NTAPI
FsRtlFastUnlockSingle(
  _In_ PFILE_LOCK FileLock,
  _In_ PFILE_OBJECT FileObject,
  _In_ PLARGE_INTEGER FileOffset,
  _In_ PLARGE_INTEGER Length,
  _In_ PEPROCESS Process,
  _In_ ULONG Key,
  _In_opt_ PVOID Context,
  _In_ BOOLEAN AlreadySynchronized);

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
NTSTATUS
NTAPI
FsRtlFastUnlockAll(
  _In_ PFILE_LOCK FileLock,
  _In_ PFILE_OBJECT FileObject,
  _In_ PEPROCESS Process,
  _In_opt_ PVOID Context);

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
NTSTATUS
NTAPI
FsRtlFastUnlockAllByKey(
  _In_ PFILE_LOCK FileLock,
  _In_ PFILE_OBJECT FileObject,
  _In_ PEPROCESS Process,
  _In_ ULONG Key,
  _In_opt_ PVOID Context);

/*
  FsRtlPrivateLock:

  ret: IoStatus->Status: STATUS_PENDING, STATUS_LOCK_NOT_GRANTED

  Internals:
    -Calls IoCompleteRequest if Irp
    -Uses exception handling / ExRaiseStatus with STATUS_INSUFFICIENT_RESOURCES
*/
_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
__drv_preferredFunction(FsRtlFastLock, "Obsolete")
NTKERNELAPI
BOOLEAN
NTAPI
FsRtlPrivateLock(
  _In_ PFILE_LOCK FileLock,
  _In_ PFILE_OBJECT FileObject,
  _In_ PLARGE_INTEGER FileOffset,
  _In_ PLARGE_INTEGER Length,
  _In_ PEPROCESS Process,
  _In_ ULONG Key,
  _In_ BOOLEAN FailImmediately,
  _In_ BOOLEAN ExclusiveLock,
  _Out_ PIO_STATUS_BLOCK IoStatus,
  _In_opt_ PIRP Irp,
  _In_opt_ __drv_aliasesMem PVOID Context,
  _In_ BOOLEAN AlreadySynchronized);

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
VOID
NTAPI
FsRtlInitializeTunnelCache(
  _In_ PTUNNEL Cache);

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
VOID
NTAPI
FsRtlAddToTunnelCache(
  _In_ PTUNNEL Cache,
  _In_ ULONGLONG DirectoryKey,
  _In_ PUNICODE_STRING ShortName,
  _In_ PUNICODE_STRING LongName,
  _In_ BOOLEAN KeyByShortName,
  _In_ ULONG DataLength,
  _In_reads_bytes_(DataLength) PVOID Data);

_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
BOOLEAN
NTAPI
FsRtlFindInTunnelCache(
  _In_ PTUNNEL Cache,
  _In_ ULONGLONG DirectoryKey,
  _In_ PUNICODE_STRING Name,
  _Out_ PUNICODE_STRING ShortName,
  _Out_ PUNICODE_STRING LongName,
  _Inout_ PULONG DataLength,
  _Out_writes_bytes_to_(*DataLength, *DataLength) PVOID Data);

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
VOID
NTAPI
FsRtlDeleteKeyFromTunnelCache(
  _In_ PTUNNEL Cache,
  _In_ ULONGLONG DirectoryKey);

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
VOID
NTAPI
FsRtlDeleteTunnelCache(
  _In_ PTUNNEL Cache);

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
VOID
NTAPI
FsRtlDissectDbcs(
  _In_ ANSI_STRING Name,
  _Out_ PANSI_STRING FirstPart,
  _Out_ PANSI_STRING RemainingPart);

_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
BOOLEAN
NTAPI
FsRtlDoesDbcsContainWildCards(
  _In_ PANSI_STRING Name);

_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
BOOLEAN
NTAPI
FsRtlIsDbcsInExpression(
  _In_ PANSI_STRING Expression,
  _In_ PANSI_STRING Name);

_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
BOOLEAN
NTAPI
FsRtlIsFatDbcsLegal(
  _In_ ANSI_STRING DbcsName,
  _In_ BOOLEAN WildCardsPermissible,
  _In_ BOOLEAN PathNamePermissible,
  _In_ BOOLEAN LeadingBackslashPermissible);

_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
BOOLEAN
NTAPI
FsRtlIsHpfsDbcsLegal(
  _In_ ANSI_STRING DbcsName,
  _In_ BOOLEAN WildCardsPermissible,
  _In_ BOOLEAN PathNamePermissible,
  _In_ BOOLEAN LeadingBackslashPermissible);

NTKERNELAPI
NTSTATUS
NTAPI
FsRtlNormalizeNtstatus(
  _In_ NTSTATUS Exception,
  _In_ NTSTATUS GenericException);

_Must_inspect_result_
NTKERNELAPI
BOOLEAN
NTAPI
FsRtlIsNtstatusExpected(
  _In_ NTSTATUS Ntstatus);

_IRQL_requires_max_(APC_LEVEL)
__drv_preferredFunction(ExAllocateFromNPagedLookasideList, "The FsRtlAllocateResource routine is obsolete, but is exported to support existing driver binaries. Use ExAllocateFromNPagedLookasideList and ExInitializeResourceLite instead.")
NTKERNELAPI
PERESOURCE
NTAPI
FsRtlAllocateResource(VOID);

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
VOID
NTAPI
FsRtlInitializeLargeMcb(
  _Out_ PLARGE_MCB Mcb,
  _In_ POOL_TYPE PoolType);

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
VOID
NTAPI
FsRtlUninitializeLargeMcb(
  _Inout_ PLARGE_MCB Mcb);

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
VOID
NTAPI
FsRtlResetLargeMcb(
  _Inout_ PLARGE_MCB Mcb,
  _In_ BOOLEAN SelfSynchronized);

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
VOID
NTAPI
FsRtlTruncateLargeMcb(
  _Inout_ PLARGE_MCB Mcb,
  _In_ LONGLONG Vbn);

_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
BOOLEAN
NTAPI
FsRtlAddLargeMcbEntry(
  _Inout_ PLARGE_MCB Mcb,
  _In_ LONGLONG Vbn,
  _In_ LONGLONG Lbn,
  _In_ LONGLONG SectorCount);

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
VOID
NTAPI
FsRtlRemoveLargeMcbEntry(
  _Inout_ PLARGE_MCB Mcb,
  _In_ LONGLONG Vbn,
  _In_ LONGLONG SectorCount);

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
BOOLEAN
NTAPI
FsRtlLookupLargeMcbEntry(
  _In_ PLARGE_MCB Mcb,
  _In_ LONGLONG Vbn,
  _Out_opt_ PLONGLONG Lbn,
  _Out_opt_ PLONGLONG SectorCountFromLbn,
  _Out_opt_ PLONGLONG StartingLbn,
  _Out_opt_ PLONGLONG SectorCountFromStartingLbn,
  _Out_opt_ PULONG Index);

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
BOOLEAN
NTAPI
FsRtlLookupLastLargeMcbEntry(
  _In_ PLARGE_MCB Mcb,
  _Out_ PLONGLONG Vbn,
  _Out_ PLONGLONG Lbn);

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
BOOLEAN
NTAPI
FsRtlLookupLastLargeMcbEntryAndIndex(
  _In_ PLARGE_MCB OpaqueMcb,
  _Out_ PLONGLONG LargeVbn,
  _Out_ PLONGLONG LargeLbn,
  _Out_ PULONG Index);

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
ULONG
NTAPI
FsRtlNumberOfRunsInLargeMcb(
  _In_ PLARGE_MCB Mcb);

_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
BOOLEAN
NTAPI
FsRtlGetNextLargeMcbEntry(
  _In_ PLARGE_MCB Mcb,
  _In_ ULONG RunIndex,
  _Out_ PLONGLONG Vbn,
  _Out_ PLONGLONG Lbn,
  _Out_ PLONGLONG SectorCount);

_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
BOOLEAN
NTAPI
FsRtlSplitLargeMcb(
  _Inout_ PLARGE_MCB Mcb,
  _In_ LONGLONG Vbn,
  _In_ LONGLONG Amount);

_IRQL_requires_max_(APC_LEVEL)
__drv_preferredFunction(FsRtlInitializeLargeMcb, "Obsolete")
NTKERNELAPI
VOID
NTAPI
FsRtlInitializeMcb(
  _Out_ PMCB Mcb,
  _In_ POOL_TYPE PoolType);

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
VOID
NTAPI
FsRtlUninitializeMcb(
  _Inout_ PMCB Mcb);

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
VOID
NTAPI
FsRtlTruncateMcb(
  _Inout_ PMCB Mcb,
  _In_ VBN Vbn);

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
BOOLEAN
NTAPI
FsRtlAddMcbEntry(
  _Inout_ PMCB Mcb,
  _In_ VBN Vbn,
  _In_ LBN Lbn,
  _In_ ULONG SectorCount);

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
VOID
NTAPI
FsRtlRemoveMcbEntry(
  _Inout_ PMCB Mcb,
  _In_ VBN Vbn,
  _In_ ULONG SectorCount);

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
BOOLEAN
NTAPI
FsRtlLookupMcbEntry(
  _In_ PMCB Mcb,
  _In_ VBN Vbn,
  _Out_ PLBN Lbn,
  _Out_opt_ PULONG SectorCount,
  _Out_ PULONG Index);

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
BOOLEAN
NTAPI
FsRtlLookupLastMcbEntry(
  _In_ PMCB Mcb,
  _Out_ PVBN Vbn,
  _Out_ PLBN Lbn);

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
ULONG
NTAPI
FsRtlNumberOfRunsInMcb(
  _In_ PMCB Mcb);

_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
BOOLEAN
NTAPI
FsRtlGetNextMcbEntry(
  _In_ PMCB Mcb,
  _In_ ULONG RunIndex,
  _Out_ PVBN Vbn,
  _Out_ PLBN Lbn,
  _Out_ PULONG SectorCount);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTKERNELAPI
NTSTATUS
NTAPI
FsRtlBalanceReads(
  _In_ PDEVICE_OBJECT TargetDevice);

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
VOID
NTAPI
FsRtlInitializeOplock(
  _Inout_ POPLOCK Oplock);

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
VOID
NTAPI
FsRtlUninitializeOplock(
  _Inout_ POPLOCK Oplock);

_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
NTSTATUS
NTAPI
FsRtlOplockFsctrl(
  _In_ POPLOCK Oplock,
  _In_ PIRP Irp,
  _In_ ULONG OpenCount);

_When_(CompletionRoutine != NULL, _Must_inspect_result_)
_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
NTSTATUS
NTAPI
FsRtlCheckOplock(
  _In_ POPLOCK Oplock,
  _In_ PIRP Irp,
  _In_opt_ PVOID Context,
  _In_opt_ POPLOCK_WAIT_COMPLETE_ROUTINE CompletionRoutine,
  _In_opt_ POPLOCK_FS_PREPOST_IRP PostIrpRoutine);

_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
BOOLEAN
NTAPI
FsRtlOplockIsFastIoPossible(
  _In_ POPLOCK Oplock);

_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
BOOLEAN
NTAPI
FsRtlCurrentBatchOplock(
  _In_ POPLOCK Oplock);

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
NTSTATUS
NTAPI
FsRtlNotifyVolumeEvent(
  _In_ PFILE_OBJECT FileObject,
  _In_ ULONG EventCode);

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
VOID
NTAPI
FsRtlNotifyInitializeSync(
  _In_ PNOTIFY_SYNC *NotifySync);

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
VOID
NTAPI
FsRtlNotifyUninitializeSync(
  _In_ PNOTIFY_SYNC *NotifySync);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTKERNELAPI
VOID
NTAPI
FsRtlNotifyFullChangeDirectory(
  _In_ PNOTIFY_SYNC NotifySync,
  _In_ PLIST_ENTRY NotifyList,
  _In_ PVOID FsContext,
  _In_ PSTRING FullDirectoryName,
  _In_ BOOLEAN WatchTree,
  _In_ BOOLEAN IgnoreBuffer,
  _In_ ULONG CompletionFilter,
  _In_opt_ PIRP NotifyIrp,
  _In_opt_ PCHECK_FOR_TRAVERSE_ACCESS TraverseCallback,
  _In_opt_ PSECURITY_SUBJECT_CONTEXT SubjectContext);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTKERNELAPI
VOID
NTAPI
FsRtlNotifyFilterReportChange(
  _In_ PNOTIFY_SYNC NotifySync,
  _In_ PLIST_ENTRY NotifyList,
  _In_ PSTRING FullTargetName,
  _In_ USHORT TargetNameOffset,
  _In_opt_ PSTRING StreamName,
  _In_opt_ PSTRING NormalizedParentName,
  _In_ ULONG FilterMatch,
  _In_ ULONG Action,
  _In_opt_ PVOID TargetContext,
  _In_opt_ PVOID FilterContext);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTKERNELAPI
VOID
NTAPI
FsRtlNotifyFullReportChange(
  _In_ PNOTIFY_SYNC NotifySync,
  _In_ PLIST_ENTRY NotifyList,
  _In_ PSTRING FullTargetName,
  _In_ USHORT TargetNameOffset,
  _In_opt_ PSTRING StreamName,
  _In_opt_ PSTRING NormalizedParentName,
  _In_ ULONG FilterMatch,
  _In_ ULONG Action,
  _In_opt_ PVOID TargetContext);

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
VOID
NTAPI
FsRtlNotifyCleanup(
  _In_ PNOTIFY_SYNC NotifySync,
  _In_ PLIST_ENTRY NotifyList,
  _In_ PVOID FsContext);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTKERNELAPI
VOID
NTAPI
FsRtlDissectName(
  _In_ UNICODE_STRING Name,
  _Out_ PUNICODE_STRING FirstPart,
  _Out_ PUNICODE_STRING RemainingPart);

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
NTKERNELAPI
BOOLEAN
NTAPI
FsRtlDoesNameContainWildCards(
  _In_ PUNICODE_STRING Name);

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
NTKERNELAPI
BOOLEAN
NTAPI
FsRtlAreNamesEqual(
  _In_ PCUNICODE_STRING Name1,
  _In_ PCUNICODE_STRING Name2,
  _In_ BOOLEAN IgnoreCase,
  _In_reads_opt_(0x10000) PCWCH UpcaseTable);

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
NTKERNELAPI
BOOLEAN
NTAPI
FsRtlIsNameInExpression(
  _In_ PUNICODE_STRING Expression,
  _In_ PUNICODE_STRING Name,
  _In_ BOOLEAN IgnoreCase,
  _In_opt_ PWCHAR UpcaseTable);

_IRQL_requires_max_(DISPATCH_LEVEL)
NTKERNELAPI
VOID
NTAPI
FsRtlPostPagingFileStackOverflow(
  _In_ PVOID Context,
  _In_ PKEVENT Event,
  _In_ PFSRTL_STACK_OVERFLOW_ROUTINE StackOverflowRoutine);

_IRQL_requires_max_(DISPATCH_LEVEL)
NTKERNELAPI
VOID
NTAPI
FsRtlPostStackOverflow (
  _In_ PVOID Context,
  _In_ PKEVENT Event,
  _In_ PFSRTL_STACK_OVERFLOW_ROUTINE StackOverflowRoutine);

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
NTKERNELAPI
NTSTATUS
NTAPI
FsRtlRegisterUncProvider(
  _Out_ PHANDLE MupHandle,
  _In_ PUNICODE_STRING RedirectorDeviceName,
  _In_ BOOLEAN MailslotsSupported);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTKERNELAPI
VOID
NTAPI
FsRtlDeregisterUncProvider(
  _In_ HANDLE Handle);

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
VOID
NTAPI
FsRtlTeardownPerStreamContexts(
  _In_ PFSRTL_ADVANCED_FCB_HEADER AdvancedHeader);

_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
NTSTATUS
NTAPI
FsRtlCreateSectionForDataScan(
  _Out_ PHANDLE SectionHandle,
  _Outptr_ PVOID *SectionObject,
  _Out_opt_ PLARGE_INTEGER SectionFileSize,
  _In_ PFILE_OBJECT FileObject,
  _In_ ACCESS_MASK DesiredAccess,
  _In_opt_ POBJECT_ATTRIBUTES ObjectAttributes,
  _In_opt_ PLARGE_INTEGER MaximumSize,
  _In_ ULONG SectionPageProtection,
  _In_ ULONG AllocationAttributes,
  _In_ ULONG Flags);

#endif /* (NTDDI_VERSION >= NTDDI_WIN2K) */

#if (NTDDI_VERSION >= NTDDI_WINXP)

_IRQL_requires_max_(PASSIVE_LEVEL)
NTKERNELAPI
VOID
NTAPI
FsRtlNotifyFilterChangeDirectory(
  _In_ PNOTIFY_SYNC NotifySync,
  _In_ PLIST_ENTRY NotifyList,
  _In_ PVOID FsContext,
  _In_ PSTRING FullDirectoryName,
  _In_ BOOLEAN WatchTree,
  _In_ BOOLEAN IgnoreBuffer,
  _In_ ULONG CompletionFilter,
  _In_opt_ PIRP NotifyIrp,
  _In_opt_ PCHECK_FOR_TRAVERSE_ACCESS TraverseCallback,
  _In_opt_ PSECURITY_SUBJECT_CONTEXT SubjectContext,
  _In_opt_ PFILTER_REPORT_CHANGE FilterCallback);

_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
NTSTATUS
NTAPI
FsRtlInsertPerStreamContext(
  _In_ PFSRTL_ADVANCED_FCB_HEADER PerStreamContext,
  _In_ PFSRTL_PER_STREAM_CONTEXT Ptr);

_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
PFSRTL_PER_STREAM_CONTEXT
NTAPI
FsRtlLookupPerStreamContextInternal(
  _In_ PFSRTL_ADVANCED_FCB_HEADER StreamContext,
  _In_opt_ PVOID OwnerId,
  _In_opt_ PVOID InstanceId);

_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
PFSRTL_PER_STREAM_CONTEXT
NTAPI
FsRtlRemovePerStreamContext(
  _In_ PFSRTL_ADVANCED_FCB_HEADER StreamContext,
  _In_opt_ PVOID OwnerId,
  _In_opt_ PVOID InstanceId);

NTKERNELAPI
VOID
NTAPI
FsRtlIncrementCcFastReadNotPossible(
  VOID);

NTKERNELAPI
VOID
NTAPI
FsRtlIncrementCcFastReadWait(VOID);

NTKERNELAPI
VOID
NTAPI
FsRtlIncrementCcFastReadNoWait(VOID);

NTKERNELAPI
VOID
NTAPI
FsRtlIncrementCcFastReadResourceMiss(VOID);

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
LOGICAL
NTAPI
FsRtlIsPagingFile(
  _In_ PFILE_OBJECT FileObject);

#endif /* (NTDDI_VERSION >= NTDDI_WINXP) */

#if (NTDDI_VERSION >= NTDDI_WS03)

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
VOID
NTAPI
FsRtlInitializeBaseMcb(
  _Out_ PBASE_MCB Mcb,
  _In_ POOL_TYPE PoolType);

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
VOID
NTAPI
FsRtlUninitializeBaseMcb(
  _In_ PBASE_MCB Mcb);

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
VOID
NTAPI
FsRtlResetBaseMcb(
  _Out_ PBASE_MCB Mcb);

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
VOID
NTAPI
FsRtlTruncateBaseMcb(
  _Inout_ PBASE_MCB Mcb,
  _In_ LONGLONG Vbn);

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
BOOLEAN
NTAPI
FsRtlAddBaseMcbEntry(
  _Inout_ PBASE_MCB Mcb,
  _In_ LONGLONG Vbn,
  _In_ LONGLONG Lbn,
  _In_ LONGLONG SectorCount);

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
BOOLEAN
NTAPI
FsRtlRemoveBaseMcbEntry(
  _Inout_ PBASE_MCB Mcb,
  _In_ LONGLONG Vbn,
  _In_ LONGLONG SectorCount);

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
BOOLEAN
NTAPI
FsRtlLookupBaseMcbEntry(
  _In_ PBASE_MCB Mcb,
  _In_ LONGLONG Vbn,
  _Out_opt_ PLONGLONG Lbn,
  _Out_opt_ PLONGLONG SectorCountFromLbn,
  _Out_opt_ PLONGLONG StartingLbn,
  _Out_opt_ PLONGLONG SectorCountFromStartingLbn,
  _Out_opt_ PULONG Index);

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
BOOLEAN
NTAPI
FsRtlLookupLastBaseMcbEntry(
  _In_ PBASE_MCB Mcb,
  _Out_ PLONGLONG Vbn,
  _Out_ PLONGLONG Lbn);

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
BOOLEAN
NTAPI
FsRtlLookupLastBaseMcbEntryAndIndex(
  _In_ PBASE_MCB OpaqueMcb,
  _Inout_ PLONGLONG LargeVbn,
  _Inout_ PLONGLONG LargeLbn,
  _Inout_ PULONG Index);

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
ULONG
NTAPI
FsRtlNumberOfRunsInBaseMcb(
  _In_ PBASE_MCB Mcb);

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
BOOLEAN
NTAPI
FsRtlGetNextBaseMcbEntry(
  _In_ PBASE_MCB Mcb,
  _In_ ULONG RunIndex,
  _Out_ PLONGLONG Vbn,
  _Out_ PLONGLONG Lbn,
  _Out_ PLONGLONG SectorCount);

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
BOOLEAN
NTAPI
FsRtlSplitBaseMcb(
  _Inout_ PBASE_MCB Mcb,
  _In_ LONGLONG Vbn,
  _In_ LONGLONG Amount);

#endif /* (NTDDI_VERSION >= NTDDI_WS03) */

#if (NTDDI_VERSION >= NTDDI_VISTA)

_When_(!Flags & MCB_FLAG_RAISE_ON_ALLOCATION_FAILURE, _Must_inspect_result_)
_IRQL_requires_max_(APC_LEVEL)
BOOLEAN
NTAPI
FsRtlInitializeBaseMcbEx(
  _Out_ PBASE_MCB Mcb,
  _In_ POOL_TYPE PoolType,
  _In_ USHORT Flags);

_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
NTAPI
FsRtlAddBaseMcbEntryEx(
  _Inout_ PBASE_MCB Mcb,
  _In_ LONGLONG Vbn,
  _In_ LONGLONG Lbn,
  _In_ LONGLONG SectorCount);

_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
BOOLEAN
NTAPI
FsRtlCurrentOplock(
  _In_ POPLOCK Oplock);

_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
NTSTATUS
NTAPI
FsRtlOplockBreakToNone(
  _Inout_ POPLOCK Oplock,
  _In_opt_ PIO_STACK_LOCATION IrpSp,
  _In_ PIRP Irp,
  _In_opt_ PVOID Context,
  _In_opt_ POPLOCK_WAIT_COMPLETE_ROUTINE CompletionRoutine,
  _In_opt_ POPLOCK_FS_PREPOST_IRP PostIrpRoutine);

_IRQL_requires_max_(DISPATCH_LEVEL)
NTKERNELAPI
NTSTATUS
NTAPI
FsRtlNotifyVolumeEventEx(
  _In_ PFILE_OBJECT FileObject,
  _In_ ULONG EventCode,
  _In_ PTARGET_DEVICE_CUSTOM_NOTIFICATION Event);

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
VOID
NTAPI
FsRtlNotifyCleanupAll(
  _In_ PNOTIFY_SYNC NotifySync,
  _In_ PLIST_ENTRY NotifyList);

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
NTAPI
FsRtlRegisterUncProviderEx(
  _Out_ PHANDLE MupHandle,
  _In_ PUNICODE_STRING RedirDevName,
  _In_ PDEVICE_OBJECT DeviceObject,
  _In_ ULONG Flags);

_Must_inspect_result_
_When_(Irp!=NULL, _IRQL_requires_max_(PASSIVE_LEVEL))
_When_(Irp==NULL, _IRQL_requires_max_(APC_LEVEL))
NTKERNELAPI
NTSTATUS
NTAPI
FsRtlCancellableWaitForSingleObject(
  _In_ PVOID Object,
  _In_opt_ PLARGE_INTEGER Timeout,
  _In_opt_ PIRP Irp);

_Must_inspect_result_
_When_(Irp != NULL, _IRQL_requires_max_(PASSIVE_LEVEL))
_When_(Irp == NULL, _IRQL_requires_max_(APC_LEVEL))
NTKERNELAPI
NTSTATUS
NTAPI
FsRtlCancellableWaitForMultipleObjects(
  _In_ ULONG Count,
  _In_reads_(Count) PVOID ObjectArray[],
  _In_ WAIT_TYPE WaitType,
  _In_opt_ PLARGE_INTEGER Timeout,
  _In_opt_ PKWAIT_BLOCK WaitBlockArray,
  _In_opt_ PIRP Irp);

_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
NTSTATUS
NTAPI
FsRtlMupGetProviderInfoFromFileObject(
  _In_ PFILE_OBJECT pFileObject,
  _In_ ULONG Level,
  _Out_writes_bytes_(*pBufferSize) PVOID pBuffer,
  _Inout_ PULONG pBufferSize);

_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
NTSTATUS
NTAPI
FsRtlMupGetProviderIdFromName(
  _In_ PUNICODE_STRING pProviderName,
  _Out_ PULONG32 pProviderId);

NTKERNELAPI
VOID
NTAPI
FsRtlIncrementCcFastMdlReadWait(VOID);

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
NTKERNELAPI
NTSTATUS
NTAPI
FsRtlValidateReparsePointBuffer(
  _In_ ULONG BufferLength,
  _In_reads_bytes_(BufferLength) PREPARSE_DATA_BUFFER ReparseBuffer);

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
NTKERNELAPI
NTSTATUS
NTAPI
FsRtlRemoveDotsFromPath(
  _Inout_updates_bytes_(PathLength) PWSTR OriginalString,
  _In_ USHORT PathLength,
  _Out_ USHORT *NewLength);

_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
NTSTATUS
NTAPI
FsRtlAllocateExtraCreateParameterList(
  _In_ FSRTL_ALLOCATE_ECPLIST_FLAGS Flags,
  _Outptr_ PECP_LIST *EcpList);

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
VOID
NTAPI
FsRtlFreeExtraCreateParameterList(
  _In_ PECP_LIST EcpList);

_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
NTSTATUS
NTAPI
FsRtlAllocateExtraCreateParameter(
  _In_ LPCGUID EcpType,
  _In_ ULONG SizeOfContext,
  _In_ FSRTL_ALLOCATE_ECP_FLAGS Flags,
  _In_opt_ PFSRTL_EXTRA_CREATE_PARAMETER_CLEANUP_CALLBACK CleanupCallback,
  _In_ ULONG PoolTag,
  _Outptr_result_bytebuffer_(SizeOfContext) PVOID *EcpContext);

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
VOID
NTAPI
FsRtlFreeExtraCreateParameter(
  _In_ PVOID EcpContext);

_When_(Flags|FSRTL_ECP_LOOKASIDE_FLAG_NONPAGED_POOL, _IRQL_requires_max_(DISPATCH_LEVEL))
_When_(!(Flags|FSRTL_ECP_LOOKASIDE_FLAG_NONPAGED_POOL), _IRQL_requires_max_(APC_LEVEL))
NTKERNELAPI
VOID
NTAPI
FsRtlInitExtraCreateParameterLookasideList(
  _Inout_ PVOID Lookaside,
  _In_ FSRTL_ECP_LOOKASIDE_FLAGS Flags,
  _In_ SIZE_T Size,
  _In_ ULONG Tag);

_When_(Flags|FSRTL_ECP_LOOKASIDE_FLAG_NONPAGED_POOL, _IRQL_requires_max_(DISPATCH_LEVEL))
_When_(!(Flags|FSRTL_ECP_LOOKASIDE_FLAG_NONPAGED_POOL), _IRQL_requires_max_(APC_LEVEL))
VOID
NTAPI
FsRtlDeleteExtraCreateParameterLookasideList(
  _Inout_ PVOID Lookaside,
  _In_ FSRTL_ECP_LOOKASIDE_FLAGS Flags);

_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
NTSTATUS
NTAPI
FsRtlAllocateExtraCreateParameterFromLookasideList(
  _In_ LPCGUID EcpType,
  ULONG SizeOfContext,
  _In_ FSRTL_ALLOCATE_ECP_FLAGS Flags,
  _In_opt_ PFSRTL_EXTRA_CREATE_PARAMETER_CLEANUP_CALLBACK CleanupCallback,
  _Inout_ PVOID LookasideList,
  _Outptr_ PVOID *EcpContext);

_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
NTSTATUS
NTAPI
FsRtlInsertExtraCreateParameter(
  _Inout_ PECP_LIST EcpList,
  _Inout_ PVOID EcpContext);

_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
NTSTATUS
NTAPI
FsRtlFindExtraCreateParameter(
  _In_ PECP_LIST EcpList,
  _In_ LPCGUID EcpType,
  _Outptr_opt_ PVOID *EcpContext,
  _Out_opt_ ULONG *EcpContextSize);

_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
NTSTATUS
NTAPI
FsRtlRemoveExtraCreateParameter(
  _Inout_ PECP_LIST EcpList,
  _In_ LPCGUID EcpType,
  _Outptr_ PVOID *EcpContext,
  _Out_opt_ ULONG *EcpContextSize);

_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
NTSTATUS
NTAPI
FsRtlGetEcpListFromIrp(
  _In_ PIRP Irp,
  _Outptr_result_maybenull_ PECP_LIST *EcpList);

_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
NTSTATUS
NTAPI
FsRtlSetEcpListIntoIrp(
  _Inout_ PIRP Irp,
  _In_ PECP_LIST EcpList);

_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
NTSTATUS
NTAPI
FsRtlGetNextExtraCreateParameter(
  _In_ PECP_LIST EcpList,
  _In_opt_ PVOID CurrentEcpContext,
  _Out_opt_ LPGUID NextEcpType,
  _Outptr_opt_ PVOID *NextEcpContext,
  _Out_opt_ ULONG *NextEcpContextSize);

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
VOID
NTAPI
FsRtlAcknowledgeEcp(
  _In_ PVOID EcpContext);

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
BOOLEAN
NTAPI
FsRtlIsEcpAcknowledged(
  _In_ PVOID EcpContext);

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
BOOLEAN
NTAPI
FsRtlIsEcpFromUserMode(
  _In_ PVOID EcpContext);

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
NTKERNELAPI
NTSTATUS
NTAPI
FsRtlChangeBackingFileObject(
  _In_opt_ PFILE_OBJECT CurrentFileObject,
  _In_ PFILE_OBJECT NewFileObject,
  _In_ FSRTL_CHANGE_BACKING_TYPE ChangeBackingType,
  _In_ ULONG Flags);

_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
NTSTATUS
NTAPI
FsRtlLogCcFlushError(
  _In_ PUNICODE_STRING FileName,
  _In_ PDEVICE_OBJECT DeviceObject,
  _In_ PSECTION_OBJECT_POINTERS SectionObjectPointer,
  _In_ NTSTATUS FlushError,
  _In_ ULONG Flags);

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
BOOLEAN
NTAPI
FsRtlAreVolumeStartupApplicationsComplete(VOID);

NTKERNELAPI
ULONG
NTAPI
FsRtlQueryMaximumVirtualDiskNestingLevel(VOID);

NTKERNELAPI
NTSTATUS
NTAPI
FsRtlGetVirtualDiskNestingLevel(
  _In_ PDEVICE_OBJECT DeviceObject,
  _Out_ PULONG NestingLevel,
  _Out_opt_ PULONG NestingFlags);

#endif /* (NTDDI_VERSION >= NTDDI_VISTA) */

#if (NTDDI_VERSION >= NTDDI_VISTASP1)
_When_(Flags | OPLOCK_FLAG_BACK_OUT_ATOMIC_OPLOCK, _Must_inspect_result_)
_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
NTSTATUS
NTAPI
FsRtlCheckOplockEx(
  _In_ POPLOCK Oplock,
  _In_ PIRP Irp,
  _In_ ULONG Flags,
  _In_opt_ PVOID Context,
  _In_opt_ POPLOCK_WAIT_COMPLETE_ROUTINE CompletionRoutine,
  _In_opt_ POPLOCK_FS_PREPOST_IRP PostIrpRoutine);

#endif

#if (NTDDI_VERSION >= NTDDI_WIN7)

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
BOOLEAN
NTAPI
FsRtlAreThereCurrentOrInProgressFileLocks(
  _In_ PFILE_LOCK FileLock);

_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
BOOLEAN
NTAPI
FsRtlOplockIsSharedRequest(
  _In_ PIRP Irp);

_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
NTSTATUS
NTAPI
FsRtlOplockBreakH(
  _In_ POPLOCK Oplock,
  _In_ PIRP Irp,
  _In_ ULONG Flags,
  _In_opt_ PVOID Context,
  _In_opt_ POPLOCK_WAIT_COMPLETE_ROUTINE CompletionRoutine,
  _In_opt_ POPLOCK_FS_PREPOST_IRP PostIrpRoutine);

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
BOOLEAN
NTAPI
FsRtlCurrentOplockH(
  _In_ POPLOCK Oplock);

_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
NTSTATUS
NTAPI
FsRtlOplockBreakToNoneEx(
  _Inout_ POPLOCK Oplock,
  _In_ PIRP Irp,
  _In_ ULONG Flags,
  _In_opt_ PVOID Context,
  _In_opt_ POPLOCK_WAIT_COMPLETE_ROUTINE CompletionRoutine,
  _In_opt_ POPLOCK_FS_PREPOST_IRP PostIrpRoutine);

_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
NTSTATUS
NTAPI
FsRtlOplockFsctrlEx(
  _In_ POPLOCK Oplock,
  _In_ PIRP Irp,
  _In_ ULONG OpenCount,
  _In_ ULONG Flags);

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
BOOLEAN
NTAPI
FsRtlOplockKeysEqual(
  _In_opt_ PFILE_OBJECT Fo1,
  _In_opt_ PFILE_OBJECT Fo2);

NTKERNELAPI
NTSTATUS
NTAPI
FsRtlInitializeExtraCreateParameterList(
  _Inout_ PECP_LIST EcpList);

NTKERNELAPI
VOID
NTAPI
FsRtlInitializeExtraCreateParameter(
  _Out_ PECP_HEADER Ecp,
  _In_ ULONG EcpFlags,
  _In_opt_ PFSRTL_EXTRA_CREATE_PARAMETER_CLEANUP_CALLBACK CleanupCallback,
  _In_ ULONG TotalSize,
  _In_ LPCGUID EcpType,
  _In_opt_ PVOID ListAllocatedFrom);

#endif /* (NTDDI_VERSION >= NTDDI_WIN7) */

_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
NTSTATUS
NTAPI
FsRtlInsertPerFileContext(
  _In_ PVOID* PerFileContextPointer,
  _In_ PFSRTL_PER_FILE_CONTEXT Ptr);

_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
PFSRTL_PER_FILE_CONTEXT
NTAPI
FsRtlLookupPerFileContext(
  _In_ PVOID* PerFileContextPointer,
  _In_opt_ PVOID OwnerId,
  _In_opt_ PVOID InstanceId);

_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
PFSRTL_PER_FILE_CONTEXT
NTAPI
FsRtlRemovePerFileContext(
  _In_ PVOID* PerFileContextPointer,
  _In_opt_ PVOID OwnerId,
  _In_opt_ PVOID InstanceId);

_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
VOID
NTAPI
FsRtlTeardownPerFileContexts(
  _In_ PVOID* PerFileContextPointer);

_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
NTSTATUS
NTAPI
FsRtlInsertPerFileObjectContext(
  _In_ PFILE_OBJECT FileObject,
  _In_ PFSRTL_PER_FILEOBJECT_CONTEXT Ptr);

_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
PFSRTL_PER_FILEOBJECT_CONTEXT
NTAPI
FsRtlLookupPerFileObjectContext(
  _In_ PFILE_OBJECT FileObject,
  _In_opt_ PVOID OwnerId,
  _In_opt_ PVOID InstanceId);

_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
NTKERNELAPI
PFSRTL_PER_FILEOBJECT_CONTEXT
NTAPI
FsRtlRemovePerFileObjectContext(
  _In_ PFILE_OBJECT FileObject,
  _In_opt_ PVOID OwnerId,
  _In_opt_ PVOID InstanceId);

NTKERNELAPI
NTSTATUS
NTAPI
FsRtlRegisterFileSystemFilterCallbacks(
  _In_ struct _DRIVER_OBJECT *FilterDriverObject,
  _In_ PFS_FILTER_CALLBACKS Callbacks);

#if (NTDDI_VERSION >= NTDDI_VISTA)
NTKERNELAPI
NTSTATUS
NTAPI
FsRtlNotifyStreamFileObject(
  _In_ struct _FILE_OBJECT * StreamFileObject,
  _In_opt_ struct _DEVICE_OBJECT *DeviceObjectHint,
  _In_ FS_FILTER_STREAM_FO_NOTIFICATION_TYPE NotificationType,
  _In_ BOOLEAN SafeToRecurse);
#endif /* (NTDDI_VERSION >= NTDDI_VISTA) */

#define FsRtlFastLock(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11)            \
     FsRtlPrivateLock(A1, A2, A3, A4, A5, A6, A7, A8, A9, NULL, A10, A11)

#define FsRtlAreThereCurrentFileLocks(FL)                                      \
    ((FL)->FastIoIsQuestionable)

#define FsRtlIncrementLockRequestsInProgress(FL) {                             \
    ASSERT((FL)->LockRequestsInProgress >= 0);                                 \
    (void)                                                                     \
    (InterlockedIncrement((LONG volatile *)&((FL)->LockRequestsInProgress)));  \
}

#define FsRtlDecrementLockRequestsInProgress(FL) {                             \
    ASSERT((FL)->LockRequestsInProgress > 0);                                  \
    (void)                                                                     \
    (InterlockedDecrement((LONG volatile *)&((FL)->LockRequestsInProgress)));  \
}

#ifdef _NTSYSTEM_
extern const UCHAR * const FsRtlLegalAnsiCharacterArray;
#define LEGAL_ANSI_CHARACTER_ARRAY FsRtlLegalAnsiCharacterArray
#else
__CREATE_NTOS_DATA_IMPORT_ALIAS(FsRtlLegalAnsiCharacterArray)
extern const UCHAR * const *FsRtlLegalAnsiCharacterArray;
#define LEGAL_ANSI_CHARACTER_ARRAY (*FsRtlLegalAnsiCharacterArray)
#endif

#define FsRtlIsAnsiCharacterWild(C)                                            \
    FlagOn(LEGAL_ANSI_CHARACTER_ARRAY[(UCHAR)(C)], FSRTL_WILD_CHARACTER)

#define FsRtlIsAnsiCharacterLegalFat(C, WILD)                                  \
    FlagOn(LEGAL_ANSI_CHARACTER_ARRAY[(UCHAR)(C)], (FSRTL_FAT_LEGAL) |         \
                                        ((WILD) ? FSRTL_WILD_CHARACTER : 0 ))

#define FsRtlIsAnsiCharacterLegalHpfs(C, WILD)                                 \
    FlagOn(LEGAL_ANSI_CHARACTER_ARRAY[(UCHAR)(C)], (FSRTL_HPFS_LEGAL) |        \
                                        ((WILD) ? FSRTL_WILD_CHARACTER : 0 ))

#define FsRtlIsAnsiCharacterLegalNtfs(C, WILD)                                 \
    FlagOn(LEGAL_ANSI_CHARACTER_ARRAY[(UCHAR)(C)], (FSRTL_NTFS_LEGAL) |        \
                                        ((WILD) ? FSRTL_WILD_CHARACTER : 0 ))

#define FsRtlIsAnsiCharacterLegalNtfsStream(C,WILD_OK)                         \
    FsRtlTestAnsiCharacter((C), TRUE, (WILD_OK), FSRTL_NTFS_STREAM_LEGAL)

#define FsRtlIsAnsiCharacterLegal(C,FLAGS)                                     \
    FsRtlTestAnsiCharacter((C), TRUE, FALSE, (FLAGS))

#define FsRtlTestAnsiCharacter(C, DEFAULT_RET, WILD_OK, FLAGS)                 \
    (((SCHAR)(C) < 0) ? DEFAULT_RET :                                          \
         FlagOn(LEGAL_ANSI_CHARACTER_ARRAY[(C)],                               \
                (FLAGS) | ((WILD_OK) ? FSRTL_WILD_CHARACTER : 0)))

#define FsRtlIsLeadDbcsCharacter(DBCS_CHAR)                                    \
    ((BOOLEAN)((UCHAR)(DBCS_CHAR) < 0x80 ? FALSE :                             \
              (NLS_MB_CODE_PAGE_TAG &&                                         \
               (NLS_OEM_LEAD_BYTE_INFO[(UCHAR)(DBCS_CHAR)] != 0))))

#define FsRtlIsUnicodeCharacterWild(C)                                         \
    ((((C) >= 0x40) ? FALSE :                                                  \
    FlagOn(LEGAL_ANSI_CHARACTER_ARRAY[(C)], FSRTL_WILD_CHARACTER )))

#define FsRtlInitPerFileContext(_fc, _owner, _inst, _cb)                       \
    ((_fc)->OwnerId = (_owner),                                                \
     (_fc)->InstanceId = (_inst),                                              \
     (_fc)->FreeCallback = (_cb))

#define FsRtlGetPerFileContextPointer(_fo)                                     \
    (FsRtlSupportsPerFileContexts(_fo) ?                                       \
        FsRtlGetPerStreamContextPointer(_fo)->FileContextSupportPointer : NULL)

#define FsRtlSupportsPerFileContexts(_fo)                                      \
    ((FsRtlGetPerStreamContextPointer(_fo) != NULL) &&                         \
     (FsRtlGetPerStreamContextPointer(_fo)->Version >= FSRTL_FCB_HEADER_V1) && \
     (FsRtlGetPerStreamContextPointer(_fo)->FileContextSupportPointer != NULL))

#define FsRtlSetupAdvancedHeaderEx(_advhdr, _fmutx, _fctxptr)                  \
{                                                                              \
    FsRtlSetupAdvancedHeader( _advhdr, _fmutx );                               \
    if ((_fctxptr) != NULL) {                                                  \
        (_advhdr)->FileContextSupportPointer = (_fctxptr);                     \
    }                                                                          \
}

#define FsRtlGetPerStreamContextPointer(FO)                                    \
    ((PFSRTL_ADVANCED_FCB_HEADER)(FO)->FsContext)

#define FsRtlInitPerStreamContext(PSC, O, I, FC)                               \
    ((PSC)->OwnerId = (O),                                                     \
    (PSC)->InstanceId = (I),                                                   \
    (PSC)->FreeCallback = (FC))

#define FsRtlSupportsPerStreamContexts(FO)                                     \
    ((BOOLEAN)((NULL != FsRtlGetPerStreamContextPointer(FO) &&                 \
               FlagOn(FsRtlGetPerStreamContextPointer(FO)->Flags2,             \
               FSRTL_FLAG2_SUPPORTS_FILTER_CONTEXTS)))

#define FsRtlLookupPerStreamContext(_sc, _oid, _iid)                           \
    (((NULL != (_sc)) &&                                                       \
      FlagOn((_sc)->Flags2,FSRTL_FLAG2_SUPPORTS_FILTER_CONTEXTS) &&            \
      !IsListEmpty(&(_sc)->FilterContexts)) ?                                  \
          FsRtlLookupPerStreamContextInternal((_sc), (_oid), (_iid)) : NULL)

_IRQL_requires_max_(APC_LEVEL)
FORCEINLINE
VOID
NTAPI
FsRtlSetupAdvancedHeader(
  _In_ PVOID AdvHdr,
  _In_ PFAST_MUTEX FMutex )
{
  PFSRTL_ADVANCED_FCB_HEADER localAdvHdr = (PFSRTL_ADVANCED_FCB_HEADER)AdvHdr;

  localAdvHdr->Flags |= FSRTL_FLAG_ADVANCED_HEADER;
  localAdvHdr->Flags2 |= FSRTL_FLAG2_SUPPORTS_FILTER_CONTEXTS;
#if (NTDDI_VERSION >= NTDDI_VISTA)
  localAdvHdr->Version = FSRTL_FCB_HEADER_V1;
#else
  localAdvHdr->Version = FSRTL_FCB_HEADER_V0;
#endif
  InitializeListHead( &localAdvHdr->FilterContexts );
  if (FMutex != NULL) {
    localAdvHdr->FastMutex = FMutex;
  }
#if (NTDDI_VERSION >= NTDDI_VISTA)
  *((PULONG_PTR)(&localAdvHdr->PushLock)) = 0;
  localAdvHdr->FileContextSupportPointer = NULL;
#endif
}

#define FsRtlInitPerFileObjectContext(_fc, _owner, _inst)                      \
           ((_fc)->OwnerId = (_owner), (_fc)->InstanceId = (_inst))

#define FsRtlCompleteRequest(IRP, STATUS) {                                    \
    (IRP)->IoStatus.Status = (STATUS);                                         \
    IoCompleteRequest( (IRP), IO_DISK_INCREMENT );                             \
}
/* Common Cache Types */

#define VACB_MAPPING_GRANULARITY        (0x40000)
#define VACB_OFFSET_SHIFT               (18)

typedef struct _PUBLIC_BCB {
  CSHORT NodeTypeCode;
  CSHORT NodeByteSize;
  ULONG MappedLength;
  LARGE_INTEGER MappedFileOffset;
} PUBLIC_BCB, *PPUBLIC_BCB;

typedef struct _CC_FILE_SIZES {
  LARGE_INTEGER AllocationSize;
  LARGE_INTEGER FileSize;
  LARGE_INTEGER ValidDataLength;
} CC_FILE_SIZES, *PCC_FILE_SIZES;

typedef BOOLEAN
(NTAPI *PACQUIRE_FOR_LAZY_WRITE) (
  _In_ PVOID Context,
  _In_ BOOLEAN Wait);

typedef VOID
(NTAPI *PRELEASE_FROM_LAZY_WRITE) (
  _In_ PVOID Context);

typedef BOOLEAN
(NTAPI *PACQUIRE_FOR_READ_AHEAD) (
  _In_ PVOID Context,
  _In_ BOOLEAN Wait);

typedef VOID
(NTAPI *PRELEASE_FROM_READ_AHEAD) (
  _In_ PVOID Context);

typedef struct _CACHE_MANAGER_CALLBACKS {
  PACQUIRE_FOR_LAZY_WRITE AcquireForLazyWrite;
  PRELEASE_FROM_LAZY_WRITE ReleaseFromLazyWrite;
  PACQUIRE_FOR_READ_AHEAD AcquireForReadAhead;
  PRELEASE_FROM_READ_AHEAD ReleaseFromReadAhead;
} CACHE_MANAGER_CALLBACKS, *PCACHE_MANAGER_CALLBACKS;

typedef struct _CACHE_UNINITIALIZE_EVENT {
  struct _CACHE_UNINITIALIZE_EVENT *Next;
  KEVENT Event;
} CACHE_UNINITIALIZE_EVENT, *PCACHE_UNINITIALIZE_EVENT;

typedef VOID
(NTAPI *PDIRTY_PAGE_ROUTINE) (
  _In_ PFILE_OBJECT FileObject,
  _In_ PLARGE_INTEGER FileOffset,
  _In_ ULONG Length,
  _In_ PLARGE_INTEGER OldestLsn,
  _In_ PLARGE_INTEGER NewestLsn,
  _In_ PVOID Context1,
  _In_ PVOID Context2);

typedef VOID
(NTAPI *PFLUSH_TO_LSN) (
  _In_ PVOID LogHandle,
  _In_ LARGE_INTEGER Lsn);

typedef VOID
(NTAPI *PCC_POST_DEFERRED_WRITE) (
  _In_ PVOID Context1,
  _In_ PVOID Context2);

#define UNINITIALIZE_CACHE_MAPS          (1)
#define DO_NOT_RETRY_PURGE               (2)
#define DO_NOT_PURGE_DIRTY_PAGES         (0x4)

#define CC_FLUSH_AND_PURGE_NO_PURGE     (0x1)
/* Common Cache Functions */

#define CcIsFileCached(FO) (                                                         \
    ((FO)->SectionObjectPointer != NULL) &&                                          \
    (((PSECTION_OBJECT_POINTERS)(FO)->SectionObjectPointer)->SharedCacheMap != NULL) \
)

extern NTKERNELAPI ULONG CcFastMdlReadWait;

#if (NTDDI_VERSION >= NTDDI_WIN2K)

NTKERNELAPI
VOID
NTAPI
CcInitializeCacheMap(
  _In_ PFILE_OBJECT FileObject,
  _In_ PCC_FILE_SIZES FileSizes,
  _In_ BOOLEAN PinAccess,
  _In_ PCACHE_MANAGER_CALLBACKS Callbacks,
  _In_ PVOID LazyWriteContext);

NTKERNELAPI
BOOLEAN
NTAPI
CcUninitializeCacheMap(
  _In_ PFILE_OBJECT FileObject,
  _In_opt_ PLARGE_INTEGER TruncateSize,
  _In_opt_ PCACHE_UNINITIALIZE_EVENT UninitializeCompleteEvent);

NTKERNELAPI
VOID
NTAPI
CcSetFileSizes(
  IN PFILE_OBJECT FileObject,
  IN PCC_FILE_SIZES FileSizes);

NTKERNELAPI
VOID
NTAPI
CcSetDirtyPageThreshold(
  _In_ PFILE_OBJECT FileObject,
  _In_ ULONG DirtyPageThreshold);

NTKERNELAPI
VOID
NTAPI
CcFlushCache(
  _In_ PSECTION_OBJECT_POINTERS SectionObjectPointer,
  _In_opt_ PLARGE_INTEGER FileOffset,
  _In_ ULONG Length,
  _Out_opt_ PIO_STATUS_BLOCK IoStatus);

NTKERNELAPI
LARGE_INTEGER
NTAPI
CcGetFlushedValidData(
  _In_ PSECTION_OBJECT_POINTERS SectionObjectPointer,
  _In_ BOOLEAN BcbListHeld);

NTKERNELAPI
BOOLEAN
NTAPI
CcZeroData(
  _In_ PFILE_OBJECT FileObject,
  _In_ PLARGE_INTEGER StartOffset,
  _In_ PLARGE_INTEGER EndOffset,
  _In_ BOOLEAN Wait);

NTKERNELAPI
PVOID
NTAPI
CcRemapBcb(
  _In_ PVOID Bcb);

NTKERNELAPI
VOID
NTAPI
CcRepinBcb(
  _In_ PVOID Bcb);

NTKERNELAPI
VOID
NTAPI
CcUnpinRepinnedBcb(
  _In_ PVOID Bcb,
  _In_ BOOLEAN WriteThrough,
  _Out_ PIO_STATUS_BLOCK IoStatus);

NTKERNELAPI
PFILE_OBJECT
NTAPI
CcGetFileObjectFromSectionPtrs(
  _In_ PSECTION_OBJECT_POINTERS SectionObjectPointer);

NTKERNELAPI
PFILE_OBJECT
NTAPI
CcGetFileObjectFromBcb(
  _In_ PVOID Bcb);

NTKERNELAPI
BOOLEAN
NTAPI
CcCanIWrite(
  _In_opt_ PFILE_OBJECT FileObject,
  _In_ ULONG BytesToWrite,
  _In_ BOOLEAN Wait,
  _In_ BOOLEAN Retrying);

NTKERNELAPI
VOID
NTAPI
CcDeferWrite(
  _In_ PFILE_OBJECT FileObject,
  _In_ PCC_POST_DEFERRED_WRITE PostRoutine,
  _In_ PVOID Context1,
  _In_ PVOID Context2,
  _In_ ULONG BytesToWrite,
  _In_ BOOLEAN Retrying);

NTKERNELAPI
BOOLEAN
NTAPI
CcCopyRead(
  _In_ PFILE_OBJECT FileObject,
  _In_ PLARGE_INTEGER FileOffset,
  _In_ ULONG Length,
  _In_ BOOLEAN Wait,
  _Out_writes_bytes_(Length) PVOID Buffer,
  _Out_ PIO_STATUS_BLOCK IoStatus);

NTKERNELAPI
VOID
NTAPI
CcFastCopyRead(
  _In_ PFILE_OBJECT FileObject,
  _In_ ULONG FileOffset,
  _In_ ULONG Length,
  _In_ ULONG PageCount,
  _Out_writes_bytes_(Length) PVOID Buffer,
  _Out_ PIO_STATUS_BLOCK IoStatus);

NTKERNELAPI
BOOLEAN
NTAPI
CcCopyWrite(
  _In_ PFILE_OBJECT FileObject,
  _In_ PLARGE_INTEGER FileOffset,
  _In_ ULONG Length,
  _In_ BOOLEAN Wait,
  _In_reads_bytes_(Length) PVOID Buffer);

NTKERNELAPI
VOID
NTAPI
CcFastCopyWrite(
  _In_ PFILE_OBJECT FileObject,
  _In_ ULONG FileOffset,
  _In_ ULONG Length,
  _In_reads_bytes_(Length) PVOID Buffer);

NTKERNELAPI
VOID
NTAPI
CcMdlRead(
  _In_ PFILE_OBJECT FileObject,
  _In_ PLARGE_INTEGER FileOffset,
  _In_ ULONG Length,
  _Out_ PMDL *MdlChain,
  _Out_ PIO_STATUS_BLOCK IoStatus);

NTKERNELAPI
VOID
NTAPI
CcMdlReadComplete(
  _In_ PFILE_OBJECT FileObject,
  _In_ PMDL MdlChain);

NTKERNELAPI
VOID
NTAPI
CcPrepareMdlWrite(
  _In_ PFILE_OBJECT FileObject,
  _In_ PLARGE_INTEGER FileOffset,
  _In_ ULONG Length,
  _Out_ PMDL *MdlChain,
  _Out_ PIO_STATUS_BLOCK IoStatus);

NTKERNELAPI
VOID
NTAPI
CcMdlWriteComplete(
  _In_ PFILE_OBJECT FileObject,
  _In_ PLARGE_INTEGER FileOffset,
  _In_ PMDL MdlChain);

NTKERNELAPI
VOID
NTAPI
CcScheduleReadAhead(
  _In_ PFILE_OBJECT FileObject,
  _In_ PLARGE_INTEGER FileOffset,
  _In_ ULONG Length);

NTKERNELAPI
NTSTATUS
NTAPI
CcWaitForCurrentLazyWriterActivity(VOID);

NTKERNELAPI
VOID
NTAPI
CcSetReadAheadGranularity(
  _In_ PFILE_OBJECT FileObject,
  _In_ ULONG Granularity);

NTKERNELAPI
BOOLEAN
NTAPI
CcPinRead(
  _In_ PFILE_OBJECT FileObject,
  _In_ PLARGE_INTEGER FileOffset,
  _In_ ULONG Length,
  _In_ ULONG Flags,
  _Outptr_ PVOID *Bcb,
  _Outptr_result_bytebuffer_(Length) PVOID *Buffer);

NTKERNELAPI
BOOLEAN
NTAPI
CcPinMappedData(
  _In_ PFILE_OBJECT FileObject,
  _In_ PLARGE_INTEGER FileOffset,
  _In_ ULONG Length,
  _In_ ULONG Flags,
  _Inout_ PVOID *Bcb);

NTKERNELAPI
BOOLEAN
NTAPI
CcPreparePinWrite(
  _In_ PFILE_OBJECT FileObject,
  _In_ PLARGE_INTEGER FileOffset,
  _In_ ULONG Length,
  _In_ BOOLEAN Zero,
  _In_ ULONG Flags,
  _Outptr_ PVOID *Bcb,
  _Outptr_result_bytebuffer_(Length) PVOID *Buffer);

NTKERNELAPI
VOID
NTAPI
CcSetDirtyPinnedData(
  _In_ PVOID BcbVoid,
  _In_opt_ PLARGE_INTEGER Lsn);

NTKERNELAPI
VOID
NTAPI
CcUnpinData(
  _In_ PVOID Bcb);

NTKERNELAPI
VOID
NTAPI
CcSetBcbOwnerPointer(
  _In_ PVOID Bcb,
  _In_ PVOID OwnerPointer);

NTKERNELAPI
VOID
NTAPI
CcUnpinDataForThread(
  _In_ PVOID Bcb,
  _In_ ERESOURCE_THREAD ResourceThreadId);

NTKERNELAPI
VOID
NTAPI
CcSetAdditionalCacheAttributes(
  _In_ PFILE_OBJECT FileObject,
  _In_ BOOLEAN DisableReadAhead,
  _In_ BOOLEAN DisableWriteBehind);

NTKERNELAPI
BOOLEAN
NTAPI
CcIsThereDirtyData(
  _In_ PVPB Vpb);

#endif /* (NTDDI_VERSION >= NTDDI_WIN2K) */

#if (NTDDI_VERSION >= NTDDI_WINXP)

NTKERNELAPI
VOID
NTAPI
CcMdlWriteAbort(
  _In_ PFILE_OBJECT FileObject,
  _In_ PMDL MdlChain);

NTKERNELAPI
VOID
NTAPI
CcSetLogHandleForFile(
  _In_ PFILE_OBJECT FileObject,
  _In_ PVOID LogHandle,
  _In_ PFLUSH_TO_LSN FlushToLsnRoutine);

NTKERNELAPI
LARGE_INTEGER
NTAPI
CcGetDirtyPages(
  _In_ PVOID LogHandle,
  _In_ PDIRTY_PAGE_ROUTINE DirtyPageRoutine,
  _In_ PVOID Context1,
  _In_ PVOID Context2);

#endif

#if (NTDDI_VERSION >= NTDDI_WINXP)
_Success_(return!=FALSE)
NTKERNELAPI
BOOLEAN
NTAPI
CcMapData(
  _In_ PFILE_OBJECT FileObject,
  _In_ PLARGE_INTEGER FileOffset,
  _In_ ULONG Length,
  _In_ ULONG Flags,
  _Outptr_ PVOID *Bcb,
  _Outptr_result_bytebuffer_(Length) PVOID *Buffer);
#elif (NTDDI_VERSION >= NTDDI_WIN2K)
NTKERNELAPI
BOOLEAN
NTAPI
CcMapData(
  _In_ PFILE_OBJECT FileObject,
  _In_ PLARGE_INTEGER FileOffset,
  _In_ ULONG Length,
  _In_ BOOLEAN Wait,
  _Outptr_ PVOID *Bcb,
  _Outptr_result_bytebuffer_(Length) PVOID *Buffer);
#endif

#if (NTDDI_VERSION >= NTDDI_VISTA)

NTKERNELAPI
NTSTATUS
NTAPI
CcSetFileSizesEx(
  _In_ PFILE_OBJECT FileObject,
  _In_ PCC_FILE_SIZES FileSizes);

NTKERNELAPI
PFILE_OBJECT
NTAPI
CcGetFileObjectFromSectionPtrsRef(
  _In_ PSECTION_OBJECT_POINTERS SectionObjectPointer);

NTKERNELAPI
VOID
NTAPI
CcSetParallelFlushFile(
  _In_ PFILE_OBJECT FileObject,
  _In_ BOOLEAN EnableParallelFlush);

NTKERNELAPI
BOOLEAN
CcIsThereDirtyDataEx(
  _In_ PVPB Vpb,
  _In_opt_ PULONG NumberOfDirtyPages);

#endif

#if (NTDDI_VERSION >= NTDDI_WIN7)
NTKERNELAPI
VOID
NTAPI
CcCoherencyFlushAndPurgeCache(
  _In_ PSECTION_OBJECT_POINTERS SectionObjectPointer,
  _In_opt_ PLARGE_INTEGER FileOffset,
  _In_ ULONG Length,
  _Out_ PIO_STATUS_BLOCK IoStatus,
  _In_opt_ ULONG Flags);
#endif

#define CcGetFileSizePointer(FO) (                                     \
    ((PLARGE_INTEGER)((FO)->SectionObjectPointer->SharedCacheMap) + 1) \
)

#if (NTDDI_VERSION >= NTDDI_VISTA)
NTKERNELAPI
BOOLEAN
NTAPI
CcPurgeCacheSection(
  _In_ PSECTION_OBJECT_POINTERS SectionObjectPointer,
  _In_opt_ PLARGE_INTEGER FileOffset,
  _In_ ULONG Length,
  _In_ ULONG Flags);
#elif (NTDDI_VERSION >= NTDDI_WIN2K)
NTKERNELAPI
BOOLEAN
NTAPI
CcPurgeCacheSection(
  _In_ PSECTION_OBJECT_POINTERS SectionObjectPointer,
  _In_opt_ PLARGE_INTEGER FileOffset,
  _In_ ULONG Length,
  _In_ BOOLEAN UninitializeCacheMaps);
#endif

#if (NTDDI_VERSION >= NTDDI_WIN7)
NTKERNELAPI
BOOLEAN
NTAPI
CcCopyWriteWontFlush(
  _In_ PFILE_OBJECT FileObject,
  _In_ PLARGE_INTEGER FileOffset,
  _In_ ULONG Length);
#else
#define CcCopyWriteWontFlush(FO, FOFF, LEN) ((LEN) <= 0x10000)
#endif

#define CcReadAhead(FO, FOFF, LEN) (                \
    if ((LEN) >= 256) {                             \
        CcScheduleReadAhead((FO), (FOFF), (LEN));   \
    }                                               \
)
/******************************************************************************
 *                            ZwXxx Functions                                 *
 ******************************************************************************/



_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
ZwQueryEaFile(
  _In_ HANDLE FileHandle,
  _Out_ PIO_STATUS_BLOCK IoStatusBlock,
  _Out_writes_bytes_(Length) PVOID Buffer,
  _In_ ULONG Length,
  _In_ BOOLEAN ReturnSingleEntry,
  _In_reads_bytes_opt_(EaListLength) PVOID EaList,
  _In_ ULONG EaListLength,
  _In_opt_ PULONG EaIndex,
  _In_ BOOLEAN RestartScan);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
ZwSetEaFile(
  _In_ HANDLE FileHandle,
  _Out_ PIO_STATUS_BLOCK IoStatusBlock,
  _In_reads_bytes_(Length) PVOID Buffer,
  _In_ ULONG Length);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
ZwDuplicateToken(
  _In_ HANDLE ExistingTokenHandle,
  _In_ ACCESS_MASK DesiredAccess,
  _In_opt_ POBJECT_ATTRIBUTES ObjectAttributes,
  _In_ BOOLEAN EffectiveOnly,
  _In_ TOKEN_TYPE TokenType,
  _Out_ PHANDLE NewTokenHandle);

#if (NTDDI_VERSION >= NTDDI_WIN2K)

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
ZwQueryObject(
  _In_opt_ HANDLE Handle,
  _In_ OBJECT_INFORMATION_CLASS ObjectInformationClass,
  _Out_writes_bytes_opt_(ObjectInformationLength) PVOID ObjectInformation,
  _In_ ULONG ObjectInformationLength,
  _Out_opt_ PULONG ReturnLength);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
ZwNotifyChangeKey(
  _In_ HANDLE KeyHandle,
  _In_opt_ HANDLE EventHandle,
  _In_opt_ PIO_APC_ROUTINE ApcRoutine,
  _In_opt_ PVOID ApcContext,
  _Out_ PIO_STATUS_BLOCK IoStatusBlock,
  _In_ ULONG NotifyFilter,
  _In_ BOOLEAN WatchSubtree,
  _Out_writes_bytes_opt_(BufferLength) PVOID Buffer,
  _In_ ULONG BufferLength,
  _In_ BOOLEAN Asynchronous);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
ZwCreateEvent(
  _Out_ PHANDLE EventHandle,
  _In_ ACCESS_MASK DesiredAccess,
  _In_opt_ POBJECT_ATTRIBUTES ObjectAttributes,
  _In_ EVENT_TYPE EventType,
  _In_ BOOLEAN InitialState);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
ZwDeleteFile(
  _In_ POBJECT_ATTRIBUTES ObjectAttributes);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
ZwQueryDirectoryFile(
  _In_ HANDLE FileHandle,
  _In_opt_ HANDLE Event,
  _In_opt_ PIO_APC_ROUTINE ApcRoutine,
  _In_opt_ PVOID ApcContext,
  _Out_ PIO_STATUS_BLOCK IoStatusBlock,
  _Out_writes_bytes_(Length) PVOID FileInformation,
  _In_ ULONG Length,
  _In_ FILE_INFORMATION_CLASS FileInformationClass,
  _In_ BOOLEAN ReturnSingleEntry,
  _In_opt_ PUNICODE_STRING FileName,
  _In_ BOOLEAN RestartScan);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
ZwSetVolumeInformationFile(
  _In_ HANDLE FileHandle,
  _Out_ PIO_STATUS_BLOCK IoStatusBlock,
  _In_reads_bytes_(Length) PVOID FsInformation,
  _In_ ULONG Length,
  _In_ FS_INFORMATION_CLASS FsInformationClass);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
ZwFsControlFile(
  _In_ HANDLE FileHandle,
  _In_opt_ HANDLE Event,
  _In_opt_ PIO_APC_ROUTINE ApcRoutine,
  _In_opt_ PVOID ApcContext,
  _Out_ PIO_STATUS_BLOCK IoStatusBlock,
  _In_ ULONG FsControlCode,
  _In_reads_bytes_opt_(InputBufferLength) PVOID InputBuffer,
  _In_ ULONG InputBufferLength,
  _Out_writes_bytes_opt_(OutputBufferLength) PVOID OutputBuffer,
  _In_ ULONG OutputBufferLength);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
ZwDuplicateObject(
  _In_ HANDLE SourceProcessHandle,
  _In_ HANDLE SourceHandle,
  _In_opt_ HANDLE TargetProcessHandle,
  _Out_opt_ PHANDLE TargetHandle,
  _In_ ACCESS_MASK DesiredAccess,
  _In_ ULONG HandleAttributes,
  _In_ ULONG Options);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
ZwOpenDirectoryObject(
  _Out_ PHANDLE DirectoryHandle,
  _In_ ACCESS_MASK DesiredAccess,
  _In_ POBJECT_ATTRIBUTES ObjectAttributes);

_Must_inspect_result_
_At_(*BaseAddress, __drv_allocatesMem(Mem))
__kernel_entry
NTSYSAPI
NTSTATUS
NTAPI
ZwAllocateVirtualMemory(
    _In_ HANDLE ProcessHandle,
    _Inout_ _Outptr_result_buffer_(*RegionSize) PVOID *BaseAddress,
    _In_ ULONG_PTR ZeroBits,
    _Inout_ PSIZE_T RegionSize,
    _In_ ULONG AllocationType,
    _In_ ULONG Protect);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
ZwFreeVirtualMemory(
    _In_ HANDLE ProcessHandle,
    _Inout_ __drv_freesMem(Mem) PVOID *BaseAddress,
    _Inout_ PSIZE_T RegionSize,
    _In_ ULONG FreeType);

_When_(Timeout == NULL, _IRQL_requires_max_(APC_LEVEL))
_When_(Timeout->QuadPart != 0, _IRQL_requires_max_(APC_LEVEL))
_When_(Timeout->QuadPart == 0, _IRQL_requires_max_(DISPATCH_LEVEL))
NTSYSAPI
NTSTATUS
NTAPI
ZwWaitForSingleObject(
  _In_ HANDLE Handle,
  _In_ BOOLEAN Alertable,
  _In_opt_ PLARGE_INTEGER Timeout);

_IRQL_requires_max_(DISPATCH_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
ZwSetEvent(
  _In_ HANDLE EventHandle,
  _Out_opt_ PLONG PreviousState);

_IRQL_requires_max_(APC_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
ZwFlushVirtualMemory(
  _In_ HANDLE ProcessHandle,
  _Inout_ PVOID *BaseAddress,
  _Inout_ PSIZE_T RegionSize,
  _Out_ PIO_STATUS_BLOCK IoStatusBlock);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
ZwQueryInformationToken(
  _In_ HANDLE TokenHandle,
  _In_ TOKEN_INFORMATION_CLASS TokenInformationClass,
  _Out_writes_bytes_to_opt_(Length,*ResultLength) PVOID TokenInformation,
  _In_ ULONG Length,
  _Out_ PULONG ResultLength);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
ZwSetSecurityObject(
  _In_ HANDLE Handle,
  _In_ SECURITY_INFORMATION SecurityInformation,
  _In_ PSECURITY_DESCRIPTOR SecurityDescriptor);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
ZwQuerySecurityObject(
  _In_ HANDLE FileHandle,
  _In_ SECURITY_INFORMATION SecurityInformation,
  _Out_writes_bytes_to_(Length,*ResultLength) PSECURITY_DESCRIPTOR SecurityDescriptor,
  _In_ ULONG Length,
  _Out_ PULONG ResultLength);
#endif /* (NTDDI_VERSION >= NTDDI_WIN2K) */

#if (NTDDI_VERSION >= NTDDI_WINXP)

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
ZwOpenProcessTokenEx(
  _In_ HANDLE ProcessHandle,
  _In_ ACCESS_MASK DesiredAccess,
  _In_ ULONG HandleAttributes,
  _Out_ PHANDLE TokenHandle);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
ZwOpenThreadTokenEx(
  _In_ HANDLE ThreadHandle,
  _In_ ACCESS_MASK DesiredAccess,
  _In_ BOOLEAN OpenAsSelf,
  _In_ ULONG HandleAttributes,
  _Out_ PHANDLE TokenHandle);

#endif /* (NTDDI_VERSION >= NTDDI_WINXP) */

#if (NTDDI_VERSION >= NTDDI_VISTA)

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
ZwLockFile(
  _In_ HANDLE FileHandle,
  _In_opt_ HANDLE Event,
  _In_opt_ PIO_APC_ROUTINE ApcRoutine,
  _In_opt_ PVOID ApcContext,
  _Out_ PIO_STATUS_BLOCK IoStatusBlock,
  _In_ PLARGE_INTEGER ByteOffset,
  _In_ PLARGE_INTEGER Length,
  _In_ ULONG Key,
  _In_ BOOLEAN FailImmediately,
  _In_ BOOLEAN ExclusiveLock);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
ZwUnlockFile(
  _In_ HANDLE FileHandle,
  _Out_ PIO_STATUS_BLOCK IoStatusBlock,
  _In_ PLARGE_INTEGER ByteOffset,
  _In_ PLARGE_INTEGER Length,
  _In_ ULONG Key);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
ZwQueryQuotaInformationFile(
  _In_ HANDLE FileHandle,
  _Out_ PIO_STATUS_BLOCK IoStatusBlock,
  _Out_writes_bytes_(Length) PVOID Buffer,
  _In_ ULONG Length,
  _In_ BOOLEAN ReturnSingleEntry,
  _In_reads_bytes_opt_(SidListLength) PVOID SidList,
  _In_ ULONG SidListLength,
  _In_opt_ PSID StartSid,
  _In_ BOOLEAN RestartScan);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
ZwSetQuotaInformationFile(
  _In_ HANDLE FileHandle,
  _Out_ PIO_STATUS_BLOCK IoStatusBlock,
  _In_reads_bytes_(Length) PVOID Buffer,
  _In_ ULONG Length);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
ZwFlushBuffersFile(
  _In_ HANDLE FileHandle,
  _Out_ PIO_STATUS_BLOCK IoStatusBlock);
#endif /* (NTDDI_VERSION >= NTDDI_VISTA) */
#if (NTDDI_VERSION >= NTDDI_WIN7)

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
ZwSetInformationToken(
  _In_ HANDLE TokenHandle,
  _In_ TOKEN_INFORMATION_CLASS TokenInformationClass,
  _In_reads_bytes_(TokenInformationLength) PVOID TokenInformation,
  _In_ ULONG TokenInformationLength);

#if (VER_PRODUCTBUILD >= 2195)
NTSYSAPI
NTSTATUS
NTAPI
ZwAdjustPrivilegesToken (
    _In_ HANDLE TokenHandle,
    _In_ BOOLEAN DisableAllPrivileges,
    _In_ PTOKEN_PRIVILEGES NewState,
    _In_ ULONG BufferLength,
    _Out_opt_ PTOKEN_PRIVILEGES PreviousState,
    _Out_ PULONG ReturnLength
);
#endif /* (VER_PRODUCTBUILD >= 2195) */

NTSYSAPI
NTSTATUS
NTAPI
ZwAlertThread (
    _In_ HANDLE ThreadHandle
);

NTSYSAPI
NTSTATUS
NTAPI
ZwAccessCheckAndAuditAlarm (
    _In_ PUNICODE_STRING SubsystemName,
    _In_ PVOID HandleId,
    _In_ PUNICODE_STRING ObjectTypeName,
    _In_ PUNICODE_STRING ObjectName,
    _In_ PSECURITY_DESCRIPTOR SecurityDescriptor,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ PGENERIC_MAPPING GenericMapping,
    _In_ BOOLEAN ObjectCreation,
    _Out_ PACCESS_MASK GrantedAccess,
    _Out_ PBOOLEAN AccessStatus,
    _Out_ PBOOLEAN GenerateOnClose
);

#if (VER_PRODUCTBUILD >= 2195)
NTSYSAPI
NTSTATUS
NTAPI
ZwCancelIoFile (
    _In_ HANDLE FileHandle,
    _Out_ PIO_STATUS_BLOCK IoStatusBlock
);
#endif /* (VER_PRODUCTBUILD >= 2195) */

NTSYSAPI
NTSTATUS
NTAPI
ZwClearEvent (
    _In_ HANDLE EventHandle
);

NTSYSAPI
NTSTATUS
NTAPI
ZwCloseObjectAuditAlarm (
    _In_ PUNICODE_STRING SubsystemName,
    _In_ PVOID HandleId,
    _In_ BOOLEAN GenerateOnClose
);

NTSYSAPI
NTSTATUS
NTAPI
ZwCreateSymbolicLinkObject (
    _Out_ PHANDLE SymbolicLinkHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ POBJECT_ATTRIBUTES ObjectAttributes,
    _In_ PUNICODE_STRING TargetName
);

NTSYSAPI
NTSTATUS
NTAPI
ZwFlushInstructionCache (
    _In_ HANDLE ProcessHandle,
    _In_opt_ PVOID BaseAddress,
    _In_ ULONG FlushSize
);

NTSYSAPI
NTSTATUS
NTAPI
ZwFlushBuffersFile(
    _In_ HANDLE FileHandle,
    _Out_ PIO_STATUS_BLOCK IoStatusBlock
);

#if (VER_PRODUCTBUILD >= 2195)
NTSYSAPI
NTSTATUS
NTAPI
ZwInitiatePowerAction (
    _In_ POWER_ACTION SystemAction,
    _In_ SYSTEM_POWER_STATE MinSystemState,
    _In_ ULONG Flags,
    _In_ BOOLEAN Asynchronous
);
#endif /* (VER_PRODUCTBUILD >= 2195) */

NTSYSAPI
NTSTATUS
NTAPI
ZwLoadKey (
    _In_ POBJECT_ATTRIBUTES KeyObjectAttributes,
    _In_ POBJECT_ATTRIBUTES FileObjectAttributes
);

NTSYSAPI
NTSTATUS
NTAPI
ZwOpenProcessToken (
    _In_ HANDLE ProcessHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _Out_ PHANDLE TokenHandle
);

NTSYSAPI
NTSTATUS
NTAPI
ZwOpenThread (
    _Out_ PHANDLE ThreadHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ POBJECT_ATTRIBUTES ObjectAttributes,
    _In_ PCLIENT_ID ClientId
);

NTSYSAPI
NTSTATUS
NTAPI
ZwOpenThreadToken (
    _In_ HANDLE ThreadHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ BOOLEAN OpenAsSelf,
    _Out_ PHANDLE TokenHandle
);

NTSYSAPI
NTSTATUS
NTAPI
ZwPulseEvent (
    _In_ HANDLE EventHandle,
    _In_opt_ PLONG PulseCount
);

NTSYSAPI
NTSTATUS
NTAPI
ZwQueryDefaultLocale (
    _In_ BOOLEAN UserProfile,
    _Out_ PLCID DefaultLocaleId
);

#if (VER_PRODUCTBUILD >= 2195)
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
ZwQueryDirectoryObject(
    _In_ HANDLE DirectoryHandle,
    _Out_ PVOID Buffer,
    _In_ ULONG BufferLength,
    _In_ BOOLEAN ReturnSingleEntry,
    _In_ BOOLEAN RestartScan,
    _Inout_ PULONG Context,
    _Out_opt_ PULONG ReturnLength
);
#endif /* (VER_PRODUCTBUILD >= 2195) */

NTSYSAPI
NTSTATUS
NTAPI
ZwQueryInformationProcess (
    _In_ HANDLE ProcessHandle,
    _In_ PROCESSINFOCLASS ProcessInformationClass,
    _Out_ PVOID ProcessInformation,
    _In_ ULONG ProcessInformationLength,
    _Out_opt_ PULONG ReturnLength
);

NTSYSAPI
NTSTATUS
NTAPI
ZwReplaceKey (
    _In_ POBJECT_ATTRIBUTES NewFileObjectAttributes,
    _In_ HANDLE KeyHandle,
    _In_ POBJECT_ATTRIBUTES OldFileObjectAttributes
);

NTSYSAPI
NTSTATUS
NTAPI
ZwResetEvent (
    _In_ HANDLE EventHandle,
    _Out_opt_ PLONG NumberOfWaitingThreads
);

#if (VER_PRODUCTBUILD >= 2195)
NTSYSAPI
NTSTATUS
NTAPI
ZwRestoreKey (
    _In_ HANDLE KeyHandle,
    _In_ HANDLE FileHandle,
    _In_ ULONG Flags
);
#endif /* (VER_PRODUCTBUILD >= 2195) */

NTSYSAPI
NTSTATUS
NTAPI
ZwSaveKey (
    _In_ HANDLE KeyHandle,
    _In_ HANDLE FileHandle
);

NTSYSAPI
NTSTATUS
NTAPI
ZwSetDefaultLocale (
    _In_ BOOLEAN UserProfile,
    _In_ LCID DefaultLocaleId
);

#if (VER_PRODUCTBUILD >= 2195)
NTSYSAPI
NTSTATUS
NTAPI
ZwSetDefaultUILanguage (
    _In_ LANGID LanguageId
);
#endif /* (VER_PRODUCTBUILD >= 2195) */

NTSYSAPI
NTSTATUS
NTAPI
ZwSetInformationProcess (
    _In_ HANDLE ProcessHandle,
    _In_ PROCESSINFOCLASS ProcessInformationClass,
    _In_ PVOID ProcessInformation,
    _In_ ULONG ProcessInformationLength
);

NTSYSAPI
NTSTATUS
NTAPI
ZwSetSystemTime (
    _In_ PLARGE_INTEGER   NewTime,
    _Out_opt_ PLARGE_INTEGER OldTime
);

NTSYSAPI
NTSTATUS
NTAPI
ZwUnloadKey (
    _In_ POBJECT_ATTRIBUTES KeyObjectAttributes
);

NTSYSAPI
NTSTATUS
NTAPI
ZwWaitForMultipleObjects (
    _In_ ULONG HandleCount,
    _In_ PHANDLE Handles,
    _In_ WAIT_TYPE WaitType,
    _In_ BOOLEAN Alertable,
    _In_opt_ PLARGE_INTEGER Timeout
);

NTSYSAPI
NTSTATUS
NTAPI
ZwYieldExecution (
    VOID
);

#endif /* (NTDDI_VERSION >= NTDDI_WIN7) */

#ifndef __SSPI_H__
#define __SSPI_H__

// for ntifs.h:
#define ISSP_LEVEL 32
#define ISSP_MODE 0

#ifdef MIDL_PASS
#define MIDL_PROP(x) x
#else
#define MIDL_PROP(x)
#endif

#define SEC_TEXT TEXT
#define SEC_FAR
#define SEC_ENTRY __stdcall

#if defined(_NO_KSECDD_IMPORT_)
#define KSECDDDECLSPEC
#else
#define KSECDDDECLSPEC __declspec(dllimport)
#endif

#define SECQOP_WRAP_NO_ENCRYPT 0x80000001
#define SECQOP_WRAP_OOB_DATA   0x40000000

#define SECURITY_ENTRYPOINTW SEC_TEXT("InitSecurityInterfaceW")
#define SECURITY_ENTRYPOINT SECURITY_ENTRYPOINTW

#define SECURITY_SUPPORT_PROVIDER_INTERFACE_VERSION   1
#define SECURITY_SUPPORT_PROVIDER_INTERFACE_VERSION_2 2
#define SECURITY_SUPPORT_PROVIDER_INTERFACE_VERSION_3 3
#define SECURITY_SUPPORT_PROVIDER_INTERFACE_VERSION_4 4

#define SECURITY_NATIVE_DREP  0x00000010
#define SECURITY_NETWORK_DREP 0x00000000

#define SECPKG_ID_NONE 0xFFFF

#define SECPKG_CRED_ATTR_NAMES                1
#define SECPKG_CRED_ATTR_SSI_PROVIDER         2

#define SECPKG_ATTR_SIZES                     0
#define SECPKG_ATTR_NAMES                     1
#define SECPKG_ATTR_LIFESPAN                  2
#define SECPKG_ATTR_DCE_INFO                  3
#define SECPKG_ATTR_STREAM_SIZES              4
#define SECPKG_ATTR_KEY_INFO                  5
#define SECPKG_ATTR_AUTHORITY                 6
#define SECPKG_ATTR_PROTO_INFO                7
#define SECPKG_ATTR_PASSWORD_EXPIRY           8
#define SECPKG_ATTR_SESSION_KEY               9
#define SECPKG_ATTR_PACKAGE_INFO             10
#define SECPKG_ATTR_USER_FLAGS               11
#define SECPKG_ATTR_NEGOTIATION_INFO         12
#define SECPKG_ATTR_NATIVE_NAMES             13
#define SECPKG_ATTR_FLAGS                    14
#define SECPKG_ATTR_USE_VALIDATED            15
#define SECPKG_ATTR_CREDENTIAL_NAME          16
#define SECPKG_ATTR_TARGET_INFORMATION       17
#define SECPKG_ATTR_ACCESS_TOKEN             18
#define SECPKG_ATTR_TARGET                   19
#define SECPKG_ATTR_AUTHENTICATION_ID        20
#define SECPKG_ATTR_LOGOFF_TIME              21
#define SECPKG_ATTR_NEGO_KEYS                22
#define SECPKG_ATTR_PROMPTING_NEEDED         24
#define SECPKG_ATTR_UNIQUE_BINDINGS          25
#define SECPKG_ATTR_ENDPOINT_BINDINGS        26
#define SECPKG_ATTR_CLIENT_SPECIFIED_TARGET  27
#define SECPKG_ATTR_LAST_CLIENT_TOKEN_STATUS 30
#define SECPKG_ATTR_NEGO_PKG_INFO            31
#define SECPKG_ATTR_NEGO_STATUS              32
#define SECPKG_ATTR_CONTEXT_DELETED          33

#define SECPKG_FLAG_INTEGRITY               0x00000001
#define SECPKG_FLAG_PRIVACY                 0x00000002
#define SECPKG_FLAG_TOKEN_ONLY              0x00000004
#define SECPKG_FLAG_DATAGRAM                0x00000008
#define SECPKG_FLAG_CONNECTION              0x00000010
#define SECPKG_FLAG_MULTI_REQUIRED          0x00000020
#define SECPKG_FLAG_CLIENT_ONLY             0x00000040
#define SECPKG_FLAG_EXTENDED_ERROR          0x00000080
#define SECPKG_FLAG_IMPERSONATION           0x00000100
#define SECPKG_FLAG_ACCEPT_WIN32_NAME       0x00000200
#define SECPKG_FLAG_STREAM                  0x00000400
#define SECPKG_FLAG_NEGOTIABLE              0x00000800
#define SECPKG_FLAG_GSS_COMPATIBLE          0x00001000
#define SECPKG_FLAG_LOGON                   0x00002000
#define SECPKG_FLAG_ASCII_BUFFERS           0x00004000
#define SECPKG_FLAG_FRAGMENT                0x00008000
#define SECPKG_FLAG_MUTUAL_AUTH             0x00010000
#define SECPKG_FLAG_DELEGATION              0x00020000
#define SECPKG_FLAG_READONLY_WITH_CHECKSUM  0x00040000
#define SECPKG_FLAG_RESTRICTED_TOKENS       0x00080000
#define SECPKG_FLAG_NEGO_EXTENDER           0x00100000
#define SECPKG_FLAG_NEGOTIABLE2             0x00200000

#define SECPKG_CRED_INBOUND                 0x00000001
#define SECPKG_CRED_OUTBOUND                0x00000002
#define SECPKG_CRED_BOTH                    0x00000003
#define SECPKG_CRED_DEFAULT                 0x00000004
#define SECPKG_CRED_RESERVED                0xF0000000
#define SECPKG_CRED_AUTOLOGON_RESTRICTED    0x00000010
#define SECPKG_CRED_PROCESS_POLICY_ONLY     0x00000020

#define SECPKG_CONTEXT_EXPORT_RESET_NEW     0x00000001
#define SECPKG_CONTEXT_EXPORT_DELETE_OLD    0x00000002
#define SECPKG_CONTEXT_EXPORT_TO_KERNEL     0x00000004

#define SECPKG_ATTR_SUBJECT_SECURITY_ATTRIBUTES 128
#define SECPKG_ATTR_NEGO_INFO_FLAG_NO_KERBEROS 0x1
#define SECPKG_ATTR_NEGO_INFO_FLAG_NO_NTLM     0x2

#define SecPkgContext_NativeNames SecPkgContext_NativeNamesW
#define PSecPkgContext_NativeNames PSecPkgContext_NativeNamesW

#define SECBUFFER_VERSION 0

#define SECBUFFER_EMPTY                 0
#define SECBUFFER_DATA                  1
#define SECBUFFER_TOKEN                 2
#define SECBUFFER_PKG_PARAMS            3
#define SECBUFFER_MISSING               4
#define SECBUFFER_EXTRA                 5
#define SECBUFFER_STREAM_TRAILER        6
#define SECBUFFER_STREAM_HEADER         7
#define SECBUFFER_NEGOTIATION_INFO      8
#define SECBUFFER_PADDING               9
#define SECBUFFER_STREAM               10
#define SECBUFFER_MECHLIST             11
#define SECBUFFER_MECHLIST_SIGNATURE   12
#define SECBUFFER_TARGET               13
#define SECBUFFER_CHANNEL_BINDINGS     14
#define SECBUFFER_CHANGE_PASS_RESPONSE 15
#define SECBUFFER_TARGET_HOST          16
#define SECBUFFER_ALERT                17

#define SECBUFFER_ATTRMASK                0xF0000000
#define SECBUFFER_READONLY                0x80000000
#define SECBUFFER_READONLY_WITH_CHECKSUM  0x10000000
#define SECBUFFER_RESERVED                0x60000000

#define ISC_REQ_DELEGATE                 0x00000001
#define ISC_REQ_MUTUAL_AUTH              0x00000002
#define ISC_REQ_REPLAY_DETECT            0x00000004
#define ISC_REQ_SEQUENCE_DETECT          0x00000008
#define ISC_REQ_CONFIDENTIALITY          0x00000010
#define ISC_REQ_USE_SESSION_KEY          0x00000020
#define ISC_REQ_PROMPT_FOR_CREDS         0x00000040
#define ISC_REQ_USE_SUPPLIED_CREDS       0x00000080
#define ISC_REQ_ALLOCATE_MEMORY          0x00000100
#define ISC_REQ_USE_DCE_STYLE            0x00000200
#define ISC_REQ_DATAGRAM                 0x00000400
#define ISC_REQ_CONNECTION               0x00000800
#define ISC_REQ_CALL_LEVEL               0x00001000
#define ISC_REQ_FRAGMENT_SUPPLIED        0x00002000
#define ISC_REQ_EXTENDED_ERROR           0x00004000
#define ISC_REQ_STREAM                   0x00008000
#define ISC_REQ_INTEGRITY                0x00010000
#define ISC_REQ_IDENTIFY                 0x00020000
#define ISC_REQ_NULL_SESSION             0x00040000
#define ISC_REQ_MANUAL_CRED_VALIDATION   0x00080000
#define ISC_REQ_RESERVED1                0x00100000
#define ISC_REQ_FRAGMENT_TO_FIT          0x00200000
#define ISC_REQ_FORWARD_CREDENTIALS      0x00400000
#define ISC_REQ_NO_INTEGRITY             0x00800000
#define ISC_REQ_USE_HTTP_STYLE           0x01000000

#define ISC_RET_DELEGATE                 0x00000001
#define ISC_RET_MUTUAL_AUTH              0x00000002
#define ISC_RET_REPLAY_DETECT            0x00000004
#define ISC_RET_SEQUENCE_DETECT          0x00000008
#define ISC_RET_CONFIDENTIALITY          0x00000010
#define ISC_RET_USE_SESSION_KEY          0x00000020
#define ISC_RET_USED_COLLECTED_CREDS     0x00000040
#define ISC_RET_USED_SUPPLIED_CREDS      0x00000080
#define ISC_RET_ALLOCATED_MEMORY         0x00000100
#define ISC_RET_USED_DCE_STYLE           0x00000200
#define ISC_RET_DATAGRAM                 0x00000400
#define ISC_RET_CONNECTION               0x00000800
#define ISC_RET_INTERMEDIATE_RETURN      0x00001000
#define ISC_RET_CALL_LEVEL               0x00002000
#define ISC_RET_EXTENDED_ERROR           0x00004000
#define ISC_RET_STREAM                   0x00008000
#define ISC_RET_INTEGRITY                0x00010000
#define ISC_RET_IDENTIFY                 0x00020000
#define ISC_RET_NULL_SESSION             0x00040000
#define ISC_RET_MANUAL_CRED_VALIDATION   0x00080000
#define ISC_RET_RESERVED1                0x00100000
#define ISC_RET_FRAGMENT_ONLY            0x00200000
#define ISC_RET_FORWARD_CREDENTIALS      0x00400000
#define ISC_RET_USED_HTTP_STYLE          0x01000000
#define ISC_RET_NO_ADDITIONAL_TOKEN      0x02000000
#define ISC_RET_REAUTHENTICATION         0x08000000

#define ASC_REQ_DELEGATE                 0x00000001
#define ASC_REQ_MUTUAL_AUTH              0x00000002
#define ASC_REQ_REPLAY_DETECT            0x00000004
#define ASC_REQ_SEQUENCE_DETECT          0x00000008
#define ASC_REQ_CONFIDENTIALITY          0x00000010
#define ASC_REQ_USE_SESSION_KEY          0x00000020
#define ASC_REQ_ALLOCATE_MEMORY          0x00000100
#define ASC_REQ_USE_DCE_STYLE            0x00000200
#define ASC_REQ_DATAGRAM                 0x00000400
#define ASC_REQ_CONNECTION               0x00000800
#define ASC_REQ_CALL_LEVEL               0x00001000
#define ASC_REQ_EXTENDED_ERROR           0x00008000
#define ASC_REQ_STREAM                   0x00010000
#define ASC_REQ_INTEGRITY                0x00020000
#define ASC_REQ_LICENSING                0x00040000
#define ASC_REQ_IDENTIFY                 0x00080000
#define ASC_REQ_ALLOW_NULL_SESSION       0x00100000
#define ASC_REQ_ALLOW_NON_USER_LOGONS    0x00200000
#define ASC_REQ_ALLOW_CONTEXT_REPLAY     0x00400000
#define ASC_REQ_FRAGMENT_TO_FIT          0x00800000
#define ASC_REQ_FRAGMENT_SUPPLIED        0x00002000
#define ASC_REQ_NO_TOKEN                 0x01000000
#define ASC_REQ_PROXY_BINDINGS           0x04000000
//#define SSP_RET_REAUTHENTICATION         0x08000000 // internal

#define ASC_REQ_ALLOW_MISSING_BINDINGS   0x10000000
#define ASC_RET_DELEGATE                 0x00000001
#define ASC_RET_MUTUAL_AUTH              0x00000002
#define ASC_RET_REPLAY_DETECT            0x00000004
#define ASC_RET_SEQUENCE_DETECT          0x00000008
#define ASC_RET_CONFIDENTIALITY          0x00000010
#define ASC_RET_USE_SESSION_KEY          0x00000020
#define ASC_RET_ALLOCATED_MEMORY         0x00000100
#define ASC_RET_USED_DCE_STYLE           0x00000200
#define ASC_RET_DATAGRAM                 0x00000400
#define ASC_RET_CONNECTION               0x00000800
#define ASC_RET_CALL_LEVEL               0x00002000
#define ASC_RET_THIRD_LEG_FAILED         0x00004000
#define ASC_RET_EXTENDED_ERROR           0x00008000
#define ASC_RET_STREAM                   0x00010000
#define ASC_RET_INTEGRITY                0x00020000
#define ASC_RET_LICENSING                0x00040000
#define ASC_RET_IDENTIFY                 0x00080000
#define ASC_RET_NULL_SESSION             0x00100000
#define ASC_RET_ALLOW_NON_USER_LOGONS    0x00200000
#define ASC_RET_ALLOW_CONTEXT_REPLAY     0x00400000
#define ASC_RET_FRAGMENT_ONLY            0x00800000
#define ASC_RET_NO_TOKEN                 0x01000000
#define ASC_RET_NO_ADDITIONAL_TOKEN      0x02000000
#define ASC_RET_NO_PROXY_BINDINGS        0x04000000
//#define SSP_RET_REAUTHENTICATION         0x08000000 // internal
#define ASC_RET_MISSING_BINDINGS         0x10000000

#define SEC_DELETED_HANDLE ((ULONG_PTR)(-2))

#define SecInvalidateHandle(x) \
    ((PSecHandle)(x))->dwLower = ((PSecHandle)(x))->dwUpper = ((ULONG_PTR)((INT_PTR)-1));

#define SecIsValidHandle(x) \
    ( ( ((PSecHandle)(x))->dwLower != (ULONG_PTR)(INT_PTR)-1 ) && \
      ( ((PSecHandle)(x))->dwUpper != (ULONG_PTR)(INT_PTR)-1 ) )

typedef WCHAR SEC_WCHAR;
typedef CHAR SEC_CHAR;
typedef LARGE_INTEGER _SECURITY_INTEGER, SECURITY_INTEGER, *PSECURITY_INTEGER;
typedef SECURITY_INTEGER TimeStamp, *PTimeStamp;
typedef UNICODE_STRING SECURITY_STRING, *PSECURITY_STRING;
#if ISSP_MODE == 0
#define PSSPI_SEC_STRING PSECURITY_STRING
#else
#define PSSPI_SEC_STRING SEC_WCHAR*
#endif

typedef PVOID PSEC_WINNT_AUTH_IDENTITY_OPAQUE;

#ifndef __SECSTATUS_DEFINED__
typedef LONG SECURITY_STATUS;
#define __SECSTATUS_DEFINED__
#endif

typedef enum _SECPKG_CRED_CLASS
{
    SecPkgCredClass_None = 0,
    SecPkgCredClass_Ephemeral = 10,
    SecPkgCredClass_PersistedGeneric = 20,
    SecPkgCredClass_PersistedSpecific = 30,
    SecPkgCredClass_Explicit = 40,
} SECPKG_CRED_CLASS, *PSECPKG_CRED_CLASS;

typedef struct _SEC_NEGOTIATION_INFO
{
    ULONG Size;
    ULONG NameLength;
    SEC_WCHAR *Name;
    PVOID Reserved;
} SEC_NEGOTIATION_INFO, *PSEC_NEGOTIATION_INFO;

typedef struct _SEC_CHANNEL_BINDINGS
{
    ULONG dwInitiatorAddrType;
    ULONG cbInitiatorLength;
    ULONG dwInitiatorOffset;
    ULONG dwAcceptorAddrType;
    ULONG cbAcceptorLength;
    ULONG dwAcceptorOffset;
    ULONG cbApplicationDataLength;
    ULONG dwApplicationDataOffset;
} SEC_CHANNEL_BINDINGS, *PSEC_CHANNEL_BINDINGS;

#ifndef _AUTH_IDENTITY_EX2_DEFINED
#define _AUTH_IDENTITY_EX2_DEFINED
typedef struct _SEC_WINNT_AUTH_IDENTITY_EX2
{
    ULONG Version;
    USHORT cbHeaderLength;
    ULONG cbStructureLength;
    ULONG UserOffset;
    USHORT UserLength;
    ULONG DomainOffset;
    USHORT DomainLength;
    ULONG PackedCredentialsOffset;
    USHORT PackedCredentialsLength;
    ULONG Flags;
    ULONG PackageListOffset;
    USHORT PackageListLength;
} SEC_WINNT_AUTH_IDENTITY_EX2, *PSEC_WINNT_AUTH_IDENTITY_EX2;
#define SEC_WINNT_AUTH_IDENTITY_VERSION_2 0x201
#endif

#ifndef _AUTH_IDENTITY_DEFINED
#define _AUTH_IDENTITY_DEFINED
typedef struct _SEC_WINNT_AUTH_IDENTITY_W
{
    PUSHORT User;
    ULONG UserLength;
    PUSHORT Domain;
    ULONG DomainLength;
    PUSHORT Password;
    ULONG PasswordLength;
    ULONG Flags;
} SEC_WINNT_AUTH_IDENTITY_W, *PSEC_WINNT_AUTH_IDENTITY_W;
#define SEC_WINNT_AUTH_IDENTITY_ANSI    0x1
#define SEC_WINNT_AUTH_IDENTITY_UNICODE 0x2
#define SEC_WINNT_AUTH_IDENTITY SEC_WINNT_AUTH_IDENTITY_W
#define PSEC_WINNT_AUTH_IDENTITY PSEC_WINNT_AUTH_IDENTITY_W
#define _SEC_WINNT_AUTH_IDENTITY _SEC_WINNT_AUTH_IDENTITY_W
#endif

#ifndef SEC_WINNT_AUTH_IDENTITY_VERSION
#define SEC_WINNT_AUTH_IDENTITY_VERSION 0x200
typedef struct _SEC_WINNT_AUTH_IDENTITY_EXW
{
    ULONG Version;
    ULONG Length;
    PUSHORT User;
    ULONG UserLength;
    PUSHORT Domain;
    ULONG DomainLength;
    PUSHORT Password;
    ULONG PasswordLength;
    ULONG Flags;
    PUSHORT PackageList;
    ULONG PackageListLength;
} SEC_WINNT_AUTH_IDENTITY_EXW, *PSEC_WINNT_AUTH_IDENTITY_EXW;
#define SEC_WINNT_AUTH_IDENTITY_EX  SEC_WINNT_AUTH_IDENTITY_EXW
#define PSEC_WINNT_AUTH_IDENTITY_EX PSEC_WINNT_AUTH_IDENTITY_EXW
#endif

#ifndef __SECHANDLE_DEFINED__
typedef struct _SecHandle
{
    ULONG_PTR dwLower;
    ULONG_PTR dwUpper;
} SecHandle, *PSecHandle;
#define __SECHANDLE_DEFINED__
#endif

typedef SecHandle CredHandle, *PCredHandle, CtxtHandle, *PCtxtHandle;

typedef struct _SecBuffer
{
    ULONG cbBuffer;
    ULONG BufferType;
#ifdef MIDL_PASS
    MIDL_PROP([size_is(cbBuffer)]) PCHAR pvBuffer;
#else
    _Field_size_bytes_(cbBuffer) void SEC_FAR *pvBuffer;
#endif
} SecBuffer, *PSecBuffer;

typedef struct _SecBufferDesc
{
    ULONG ulVersion;
    ULONG cBuffers;
    MIDL_PROP([size_is(cBuffers)]) _Field_size_(cBuffers) PSecBuffer pBuffers;
} SecBufferDesc, SEC_FAR *PSecBufferDesc;

typedef struct _SecPkgInfoW
{
    ULONG fCapabilities;
    USHORT wVersion;
    USHORT wRPCID;
    ULONG cbMaxToken;
    MIDL_PROP([string]) SEC_WCHAR *Name;
    MIDL_PROP([string]) SEC_WCHAR *Comment;
} SecPkgInfoW, *PSecPkgInfoW;
#define SecPkgInfo SecPkgInfoW
#define PSecPkgInfo PSecPkgInfoW

typedef struct _SecPkgCredentials_NamesW
{
    MIDL_PROP([string]) SEC_WCHAR *sUserName;
} SecPkgCredentials_NamesW, *PSecPkgCredentials_NamesW;
#define SecPkgCredentials_Names SecPkgCredentials_NamesW
#define PSecPkgCredentials_Names PSecPkgCredentials_NamesW

typedef struct _SecPkgContext_NamesW
{
    SEC_WCHAR *sUserName;
} SecPkgContext_NamesW, *PSecPkgContext_NamesW;
#define SecPkgContext_Names SecPkgContext_NamesW
#define PSecPkgContext_Names PSecPkgContext_NamesW

#if OSVER(NTDDI_VERSION) > NTDDI_WIN2K
typedef struct _SecPkgContext_CredentialNameW
{
    ULONG CredentialType;
    SEC_WCHAR *sCredentialName;
} SecPkgContext_CredentialNameW, *PSecPkgContext_CredentialNameW;
#endif
#define SecPkgContext_CredentialName SecPkgContext_CredentialNameW
#define PSecPkgContext_CredentialName PSecPkgContext_CredentialNameW

typedef struct _SecPkgContext_SubjectAttributes
{
    PVOID AttributeInfo;
} SecPkgContext_SubjectAttributes, *PSecPkgContext_SubjectAttributes;

typedef struct _SecPkgContext_CredInfo
{
    SECPKG_CRED_CLASS CredClass;
    ULONG IsPromptingNeeded;
} SecPkgContext_CredInfo, *PSecPkgContext_CredInfo;

typedef struct _SecPkgContext_NegoPackageInfo
{
    ULONG PackageMask;
} SecPkgContext_NegoPackageInfo, *PSecPkgContext_NegoPackageInfo;

typedef struct _SecPkgContext_NegoStatus
{
    ULONG LastStatus;
} SecPkgContext_NegoStatus, *PSecPkgContext_NegoStatus;

typedef struct _SecPkgContext_Sizes
{
    ULONG cbMaxToken;
    ULONG cbMaxSignature;
    ULONG cbBlockSize;
    ULONG cbSecurityTrailer;
} SecPkgContext_Sizes, *PSecPkgContext_Sizes;

typedef struct _SecPkgContext_StreamSizes
{
    ULONG cbHeader;
    ULONG cbTrailer;
    ULONG cbMaximumMessage;
    ULONG cBuffers;
    ULONG cbBlockSize;
} SecPkgContext_StreamSizes, *PSecPkgContext_StreamSizes;

typedef struct _SecPkgContext_Lifespan
{
    TimeStamp tsStart;
    TimeStamp tsExpiry;
} SecPkgContext_Lifespan, *PSecPkgContext_Lifespan;

typedef struct _SecPkgContext_PasswordExpiry
{
    TimeStamp tsPasswordExpires;
} SecPkgContext_PasswordExpiry, *PSecPkgContext_PasswordExpiry;

typedef struct _SecPkgContext_ProtoInfoW
{
    SEC_WCHAR *sProtocolName;
    ULONG majorVersion;
    ULONG minorVersion;
} SecPkgContext_ProtoInfoW, *PSecPkgContext_ProtoInfoW;
#define SecPkgContext_ProtoInfo SecPkgContext_ProtoInfoW
#define PSecPkgContext_ProtoInfo PSecPkgContext_ProtoInfoW

typedef struct _SecPkgContext_KeyInfoW
{
    SEC_WCHAR *sSignatureAlgorithmName;
    SEC_WCHAR *sEncryptAlgorithmName;
    ULONG KeySize;
    ULONG SignatureAlgorithm;
    ULONG EncryptAlgorithm;
} SecPkgContext_KeyInfoW, *PSecPkgContext_KeyInfoW;
#define SecPkgContext_KeyInfo SecPkgContext_KeyInfoW
#define PSecPkgContext_KeyInfo PSecPkgContext_KeyInfoW

typedef struct _SecPkgContext_SessionKey
{
    ULONG SessionKeyLength;
    _Field_size_bytes_(SessionKeyLength) PUCHAR SessionKey;
} SecPkgContext_SessionKey, *PSecPkgContext_SessionKey;

typedef struct _SecPkgContext_NegoKeys
{
    ULONG KeyType;
    USHORT KeyLength;
    _Field_size_bytes_(KeyLength) PUCHAR KeyValue;
    ULONG  VerifyKeyType;
    USHORT VerifyKeyLength;
    _Field_size_bytes_(VerifyKeyLength) PUCHAR VerifyKeyValue;
} SecPkgContext_NegoKeys, *PSecPkgContext_NegoKeys;

typedef struct _SecPkgContext_DceInfo
{
    ULONG AuthzSvc;
    PVOID pPac;
} SecPkgContext_DceInfo, *PSecPkgContext_DceInfo;

typedef struct _SecPkgContext_PackageInfoW
{
    PSecPkgInfoW PackageInfo;
} SecPkgContext_PackageInfoW, *PSecPkgContext_PackageInfoW;
#define SecPkgContext_PackageInfo SecPkgContext_PackageInfoW
#define PSecPkgContext_PackageInfo PSecPkgContext_PackageInfoW

typedef struct _SecPkgContext_UserFlags
{
    ULONG UserFlags;
} SecPkgContext_UserFlags, *PSecPkgContext_UserFlags;

typedef struct _SecPkgContext_Flags
{
    ULONG Flags;
} SecPkgContext_Flags, *PSecPkgContext_Flags;

typedef struct _SecPkgContext_NegotiationInfoW
{
    PSecPkgInfoW PackageInfo ;
    ULONG NegotiationState ;
} SecPkgContext_NegotiationInfoW, *PSecPkgContext_NegotiationInfoW;

typedef struct _SecPkgContext_AuthorityW
{
    SEC_WCHAR *sAuthorityName;
} SecPkgContext_AuthorityW, *PSecPkgContext_AuthorityW;
#define SecPkgContext_Authority SecPkgContext_AuthorityW
#define PSecPkgContext_Authority PSecPkgContext_AuthorityW


#if NTDDI_VERSION > NTDDI_WS03
typedef struct _SecPkgCredentials_SSIProviderW
{
    SEC_WCHAR *sProviderName;
    ULONG ProviderInfoLength;
    PCHAR ProviderInfo;
} SecPkgCredentials_SSIProviderW, *PSecPkgCredentials_SSIProviderW;
#define SecPkgCredentials_SSIProvider SecPkgCredentials_SSIProviderW
#define PSecPkgCredentials_SSIProvider PSecPkgCredentials_SSIProviderW

typedef struct _SecPkgContext_LogoffTime
{
    TimeStamp tsLogoffTime;
} SecPkgContext_LogoffTime, *PSecPkgContext_LogoffTime;
#endif

/* forward declaration */
typedef struct _SECURITY_FUNCTION_TABLE_W SecurityFunctionTableW, *PSecurityFunctionTableW;
#define SecurityFunctionTable SecurityFunctionTableW
#define PSecurityFunctionTable PSecurityFunctionTableW

typedef
VOID
(SEC_ENTRY * SEC_GET_KEY_FN)(
    PVOID Arg,
    PVOID Principal,
    ULONG KeyVer,
    PVOID *Key,
    SECURITY_STATUS *Status);

KSECDDDECLSPEC
SECURITY_STATUS
SEC_ENTRY
AcceptSecurityContext(
    _In_opt_ PCredHandle phCredential,
    _In_opt_ PCtxtHandle phContext,
    _In_opt_ PSecBufferDesc pInput,
    _In_ ULONG fContextReq,
    _In_ ULONG TargetDataRep,
    _In_opt_ PCtxtHandle phNewContext,
    _In_opt_ PSecBufferDesc pOutput,
    _Out_ PULONG pfContextAttr,
    _Out_opt_ PTimeStamp ptsExpiry);

typedef
SECURITY_STATUS
(SEC_ENTRY * ACCEPT_SECURITY_CONTEXT_FN)(
    PCredHandle,
    PCtxtHandle,
    PSecBufferDesc,
    ULONG,
    ULONG,
    PCtxtHandle,
    PSecBufferDesc,
    PULONG,
    PTimeStamp);

KSECDDDECLSPEC
SECURITY_STATUS
SEC_ENTRY
AcquireCredentialsHandleW(
    _In_opt_ PSSPI_SEC_STRING pPrincipal,
    _In_ PSSPI_SEC_STRING pPackage,
    _In_ ULONG fCredentialUse,
    _In_opt_ PVOID pvLogonId,
    _In_opt_ PVOID pAuthData,
    _In_opt_ SEC_GET_KEY_FN pGetKeyFn,
    _In_opt_ PVOID pvGetKeyArgument,
    _Out_ PCredHandle phCredential,
    _Out_opt_ PTimeStamp ptsExpiry);
#define AcquireCredentialsHandle AcquireCredentialsHandleW

typedef
SECURITY_STATUS
(SEC_ENTRY * ACQUIRE_CREDENTIALS_HANDLE_FN_W)(
    PSSPI_SEC_STRING,
    PSSPI_SEC_STRING,
    ULONG,
    PVOID,
    PVOID,
    SEC_GET_KEY_FN,
    PVOID,
    PCredHandle,
    PTimeStamp);
#define ACQUIRE_CREDENTIALS_HANDLE_FN ACQUIRE_CREDENTIALS_HANDLE_FN_W

SECURITY_STATUS
SEC_ENTRY
AddCredentialsA(
    _In_ PCredHandle hCredentials,
    _In_opt_ LPSTR pszPrincipal,
    _In_ LPSTR pszPackage,
    _In_ ULONG fCredentialUse,
    _In_opt_ PVOID pAuthData,
    _In_opt_ SEC_GET_KEY_FN pGetKeyFn,
    _In_opt_ PVOID pvGetKeyArgument,
    _Out_opt_ PTimeStamp ptsExpiry);

typedef
SECURITY_STATUS
(SEC_ENTRY * ADD_CREDENTIALS_FN_A)(
    PCredHandle,
    SEC_CHAR *,
    SEC_CHAR *,
    ULONG,
    PVOID,
    SEC_GET_KEY_FN,
    PVOID,
    PTimeStamp);

KSECDDDECLSPEC
SECURITY_STATUS
SEC_ENTRY
AddCredentialsW(
    _In_ PCredHandle hCredentials,
    _In_opt_ PSSPI_SEC_STRING pPrincipal,
    _In_ PSSPI_SEC_STRING pPackage,
    _In_ ULONG fCredentialUse,
    _In_opt_ PVOID pAuthData,
    _In_opt_ SEC_GET_KEY_FN pGetKeyFn,
    _In_opt_ PVOID pvGetKeyArgument,
    _Out_opt_ PTimeStamp ptsExpiry);

typedef
SECURITY_STATUS
(SEC_ENTRY * ADD_CREDENTIALS_FN_W)(
    PCredHandle,
    PSSPI_SEC_STRING,
    PSSPI_SEC_STRING,
    ULONG,
    PVOID,
    SEC_GET_KEY_FN,
    PVOID,
    PTimeStamp);

#ifdef UNICODE
#define AddCredentials  AddCredentialsW
#define ADD_CREDENTIALS_FN  ADD_CREDENTIALS_FN_W
#else
#define AddCredentials  AddCredentialsA
#define ADD_CREDENTIALS_FN ADD_CREDENTIALS_FN_A
#endif

KSECDDDECLSPEC
SECURITY_STATUS
SEC_ENTRY
ApplyControlToken(
    _In_ PCtxtHandle phContext,
    _In_ PSecBufferDesc pInput);

typedef
SECURITY_STATUS
(SEC_ENTRY * APPLY_CONTROL_TOKEN_FN)(
    PCtxtHandle, PSecBufferDesc);

#if (ISSP_MODE != 0)

SECURITY_STATUS
SEC_ENTRY
ChangeAccountPasswordA(
    _In_ SEC_CHAR* pszPackageName,
    _In_ SEC_CHAR* pszDomainName,
    _In_ SEC_CHAR* pszAccountName,
    _In_ SEC_CHAR* pszOldPassword,
    _In_ SEC_CHAR* pszNewPassword,
    _In_ BOOLEAN bImpersonating,
    _In_ ULONG dwReserved,
    _Inout_ PSecBufferDesc pOutput);

typedef
SECURITY_STATUS
(SEC_ENTRY * CHANGE_PASSWORD_FN_A)(
    SEC_CHAR *,
    SEC_CHAR *,
    SEC_CHAR *,
    SEC_CHAR *,
    SEC_CHAR *,
    BOOLEAN,
    ULONG,
    PSecBufferDesc);

SECURITY_STATUS
SEC_ENTRY
ChangeAccountPasswordW(
    _In_ SEC_WCHAR* pszPackageName,
    _In_ SEC_WCHAR* pszDomainName,
    _In_ SEC_WCHAR* pszAccountName,
    _In_ SEC_WCHAR* pszOldPassword,
    _In_ SEC_WCHAR* pszNewPassword,
    _In_ BOOLEAN bImpersonating,
    _In_ ULONG dwReserved,
    _Inout_ PSecBufferDesc pOutput);

typedef
SECURITY_STATUS
(SEC_ENTRY * CHANGE_PASSWORD_FN_W)(
    SEC_WCHAR *,
    SEC_WCHAR *,
    SEC_WCHAR *,
    SEC_WCHAR *,
    SEC_WCHAR *,
    BOOLEAN,
    ULONG,
    PSecBufferDesc);

#ifdef UNICODE
#define ChangeAccountPassword ChangeAccountPasswordW
#define CHANGE_PASSWORD_FN CHANGE_PASSWORD_FN_W
#else
#define ChangeAccountPassword ChangeAccountPasswordA
#define CHANGE_PASSWORD_FN CHANGE_PASSWORD_FN_A
#endif

#endif /* ISSP_MODE != 0 */

SECURITY_STATUS
SEC_ENTRY
CompleteAuthToken(
    _In_ PCtxtHandle phContext,
    _In_ PSecBufferDesc pToken);

typedef
SECURITY_STATUS
(SEC_ENTRY * COMPLETE_AUTH_TOKEN_FN)(
    PCtxtHandle,
    PSecBufferDesc);

SECURITY_STATUS
SEC_ENTRY
DecryptMessage(
    _In_ PCtxtHandle phContext,
    _Inout_ PSecBufferDesc pMessage,
    _In_ ULONG MessageSeqNo,
    _Out_opt_ PULONG pfQOP);

typedef
SECURITY_STATUS
(SEC_ENTRY * DECRYPT_MESSAGE_FN)(
    PCtxtHandle,
    PSecBufferDesc,
    ULONG,
    PULONG);

KSECDDDECLSPEC
SECURITY_STATUS
SEC_ENTRY
DeleteSecurityContext(
    _In_ PCtxtHandle phContext);

typedef
SECURITY_STATUS
(SEC_ENTRY * DELETE_SECURITY_CONTEXT_FN)(
    PCtxtHandle);

SECURITY_STATUS
SEC_ENTRY
EncryptMessage(
    _In_ PCtxtHandle phContext,
    _In_ ULONG  fQOP,
    _Inout_ PSecBufferDesc pMessage,
    _In_ ULONG MessageSeqNo);

typedef
SECURITY_STATUS
(SEC_ENTRY * ENCRYPT_MESSAGE_FN)(
    PCtxtHandle,
    ULONG,
    PSecBufferDesc,
    ULONG);

KSECDDDECLSPEC
SECURITY_STATUS
SEC_ENTRY
EnumerateSecurityPackagesW(
    _Out_ PULONG pcPackages,
    _Deref_out_ PSecPkgInfoW* ppPackageInfo);
#define EnumerateSecurityPackages EnumerateSecurityPackagesW

typedef
SECURITY_STATUS
(SEC_ENTRY * ENUMERATE_SECURITY_PACKAGES_FN_W)(
    PULONG,
    PSecPkgInfoW*);
#define ENUMERATE_SECURITY_PACKAGES_FN ENUMERATE_SECURITY_PACKAGES_FN_W

KSECDDDECLSPEC
SECURITY_STATUS
SEC_ENTRY
ExportSecurityContext(
    _In_ PCtxtHandle phContext,
    _In_ ULONG fFlags,
    _Out_ PSecBuffer pPackedContext,
    _Out_ PVOID* pToken);

typedef
SECURITY_STATUS
(SEC_ENTRY * EXPORT_SECURITY_CONTEXT_FN)(
    PCtxtHandle,
    ULONG,
    PSecBuffer,
    PVOID*);

SECURITY_STATUS
SEC_ENTRY
FreeContextBuffer(
    _Inout_ PVOID pvContextBuffer);

typedef
SECURITY_STATUS
(SEC_ENTRY * FREE_CONTEXT_BUFFER_FN)(
    _Inout_ PVOID);

KSECDDDECLSPEC
SECURITY_STATUS
SEC_ENTRY
FreeCredentialsHandle(
    _In_ PCredHandle phCredential);

typedef
SECURITY_STATUS
(SEC_ENTRY * FREE_CREDENTIALS_HANDLE_FN)(
    PCredHandle);

KSECDDDECLSPEC
SECURITY_STATUS
SEC_ENTRY
ImpersonateSecurityContext(
    _In_ PCtxtHandle phContext);

typedef
SECURITY_STATUS
(SEC_ENTRY * IMPERSONATE_SECURITY_CONTEXT_FN)(
    PCtxtHandle);

KSECDDDECLSPEC
SECURITY_STATUS
SEC_ENTRY
ImportSecurityContextW(
    _In_ PSSPI_SEC_STRING pszPackage,
    _In_ PSecBuffer pPackedContext,
    _In_ PVOID Token,
    _Out_ PCtxtHandle phContext);
#define ImportSecurityContext ImportSecurityContextW

typedef
SECURITY_STATUS
(SEC_ENTRY * IMPORT_SECURITY_CONTEXT_FN_W)(
    PSSPI_SEC_STRING,
    PSecBuffer,
    PVOID,
    PCtxtHandle);
#define IMPORT_SECURITY_CONTEXT_FN IMPORT_SECURITY_CONTEXT_FN_W

KSECDDDECLSPEC
SECURITY_STATUS
SEC_ENTRY
InitializeSecurityContextW(
    _In_opt_ PCredHandle phCredential,
    _In_opt_ PCtxtHandle phContext,
    _In_opt_ PSSPI_SEC_STRING pTargetName,
    _In_ ULONG fContextReq,
    _In_ ULONG Reserved1,
    _In_ ULONG TargetDataRep,
    _In_opt_ PSecBufferDesc pInput,
    _In_ ULONG Reserved2,
    _Inout_opt_ PCtxtHandle phNewContext,
    _Inout_opt_ PSecBufferDesc pOutput,
    _Out_ PULONG pfContextAttr,
    _Out_opt_ PTimeStamp ptsExpiry);
#define InitializeSecurityContext InitializeSecurityContextW

typedef
SECURITY_STATUS
(SEC_ENTRY * INITIALIZE_SECURITY_CONTEXT_FN_W)(
    PCredHandle,
    PCtxtHandle,
    PSSPI_SEC_STRING,
    ULONG,
    ULONG,
    ULONG,
    PSecBufferDesc,
    ULONG,
    PCtxtHandle,
    PSecBufferDesc,
    PULONG,
    PTimeStamp);
#define INITIALIZE_SECURITY_CONTEXT_FN INITIALIZE_SECURITY_CONTEXT_FN_W

KSECDDDECLSPEC
PSecurityFunctionTableW
SEC_ENTRY
InitSecurityInterfaceW(VOID);
#define InitSecurityInterface InitSecurityInterfaceW

typedef
PSecurityFunctionTableW
(SEC_ENTRY * INIT_SECURITY_INTERFACE_W)(VOID);
#define INIT_SECURITY_INTERFACE INIT_SECURITY_INTERFACE_W

KSECDDDECLSPEC
SECURITY_STATUS
SEC_ENTRY
MakeSignature(
    _In_ PCtxtHandle phContext,
    _In_ ULONG fQOP,
    _In_ PSecBufferDesc pMessage,
    _In_ ULONG MessageSeqNo);

typedef
SECURITY_STATUS
(SEC_ENTRY * MAKE_SIGNATURE_FN)(
    PCtxtHandle,
    ULONG,
    PSecBufferDesc,
    ULONG);

KSECDDDECLSPEC
SECURITY_STATUS
SEC_ENTRY
QueryContextAttributesW(
    _In_ PCtxtHandle phContext,
    _In_ ULONG ulAttribute,
    _Out_ PVOID pBuffer);
#define QueryContextAttributes QueryContextAttributesW

typedef
SECURITY_STATUS
(SEC_ENTRY * QUERY_CONTEXT_ATTRIBUTES_FN_W)(
    PCtxtHandle,
    ULONG,
    PVOID);
#define QUERY_CONTEXT_ATTRIBUTES_FN QUERY_CONTEXT_ATTRIBUTES_FN_W

KSECDDDECLSPEC
SECURITY_STATUS
SEC_ENTRY
QueryCredentialsAttributesW(
    _In_    PCredHandle phCredential,
    _In_    ULONG ulAttribute,
    _Inout_ PVOID pBuffer);
#define QueryCredentialsAttributes QueryCredentialsAttributesW

typedef
SECURITY_STATUS
(SEC_ENTRY * QUERY_CREDENTIALS_ATTRIBUTES_FN_W)(
    PCredHandle,
    ULONG,
    PVOID);
#define QUERY_CREDENTIALS_ATTRIBUTES_FN QUERY_CREDENTIALS_ATTRIBUTES_FN_W

KSECDDDECLSPEC
SECURITY_STATUS
SEC_ENTRY
QuerySecurityContextToken(
    _In_ PCtxtHandle phContext,
    _Out_ PVOID* Token);

typedef
SECURITY_STATUS
(SEC_ENTRY * QUERY_SECURITY_CONTEXT_TOKEN_FN)(
    PCtxtHandle, PVOID *);

KSECDDDECLSPEC
SECURITY_STATUS
SEC_ENTRY
QuerySecurityPackageInfoW(
    _In_ PSSPI_SEC_STRING pPackageName,
    _Deref_out_ PSecPkgInfoW *ppPackageInfo);
#define QuerySecurityPackageInfo QuerySecurityPackageInfoW

typedef
SECURITY_STATUS
(SEC_ENTRY * QUERY_SECURITY_PACKAGE_INFO_FN_W)(
    PSSPI_SEC_STRING,
    PSecPkgInfoW *);
#define QUERY_SECURITY_PACKAGE_INFO_FN QUERY_SECURITY_PACKAGE_INFO_FN_W

KSECDDDECLSPEC
SECURITY_STATUS
SEC_ENTRY
RevertSecurityContext(
    _In_ PCtxtHandle phContext);

typedef
SECURITY_STATUS
(SEC_ENTRY * REVERT_SECURITY_CONTEXT_FN)(
    PCtxtHandle);

#if (OSVER(NTDDI_VERSION) > NTDDI_WIN2K)
SECURITY_STATUS
SEC_ENTRY
SetContextAttributesW(
    _In_ PCtxtHandle phContext,
    _In_ ULONG ulAttribute,
    _In_bytecount_(cbBuffer) PVOID pBuffer,
    _In_ ULONG cbBuffer);
#define SetContextAttributes SetContextAttributesW

typedef
SECURITY_STATUS
(SEC_ENTRY * SET_CONTEXT_ATTRIBUTES_FN_W)(
    PCtxtHandle,
    ULONG,
    PVOID,
    ULONG);
#define SET_CONTEXT_ATTRIBUTES_FN SET_CONTEXT_ATTRIBUTES_FN_W
#endif

#if (NTDDI_VERSION > NTDDI_WS03)
KSECDDDECLSPEC
SECURITY_STATUS
SEC_ENTRY
SetCredentialsAttributesW(
    _In_ PCredHandle phCredential,
    _In_ ULONG ulAttribute,
    _In_bytecount_(cbBuffer) PVOID pBuffer,
    _In_ ULONG cbBuffer);
#define SetCredentialsAttributes SetCredentialsAttributesW

typedef
SECURITY_STATUS
(SEC_ENTRY * SET_CREDENTIALS_ATTRIBUTES_FN_W)(
    PCredHandle,
    ULONG,
    PVOID,
    ULONG);
#define SET_CREDENTIALS_ATTRIBUTES_FN SET_CREDENTIALS_ATTRIBUTES_FN_W
#endif /* NTDDI_VERSION > NTDDI_WS03 */

KSECDDDECLSPEC
SECURITY_STATUS
SEC_ENTRY
VerifySignature(
    _In_ PCtxtHandle phContext,
    _In_ PSecBufferDesc pMessage,
    _In_ ULONG MessageSeqNo,
    _Out_ PULONG pfQOP);

typedef
SECURITY_STATUS
(SEC_ENTRY * VERIFY_SIGNATURE_FN)(
    PCtxtHandle,
    PSecBufferDesc,
    ULONG,
    PULONG);

#if (ISSP_MODE == 0)

KSECDDDECLSPEC
NTSTATUS
NTAPI
SecMakeSPN(
    _In_ PUNICODE_STRING ServiceClass,
    _In_ PUNICODE_STRING ServiceName,
    _In_opt_ PUNICODE_STRING InstanceName,
    _In_opt_ USHORT InstancePort,
    _In_opt_ PUNICODE_STRING Referrer,
    _Inout_ PUNICODE_STRING Spn,
    _Out_opt_ PULONG Length,
    _In_ BOOLEAN Allocate);

#if (NTDDI_VERSION >= NTDDI_WINXP)
KSECDDDECLSPEC
NTSTATUS
NTAPI
SecMakeSPNEx(
    _In_ PUNICODE_STRING ServiceClass,
    _In_ PUNICODE_STRING ServiceName,
    _In_opt_ PUNICODE_STRING InstanceName,
    _In_opt_ USHORT InstancePort,
    _In_opt_ PUNICODE_STRING Referrer,
    _In_opt_ PUNICODE_STRING TargetInfo,
    _Inout_ PUNICODE_STRING Spn,
    _Out_ PULONG Length OPTIONAL,
    _In_ BOOLEAN Allocate);

KSECDDDECLSPEC
NTSTATUS
SEC_ENTRY
SecLookupAccountSid(
    _In_ PSID Sid,
    _Out_ PULONG NameSize,
    _Inout_ PUNICODE_STRING NameBuffer,
    _Out_ PULONG DomainSize OPTIONAL,
    _Out_opt_ PUNICODE_STRING DomainBuffer,
    _Out_ PSID_NAME_USE NameUse);

KSECDDDECLSPEC
NTSTATUS
SEC_ENTRY
SecLookupAccountName(
    _In_ PUNICODE_STRING Name,
    _Inout_ PULONG SidSize,
    _Out_ PSID Sid,
    _Out_ PSID_NAME_USE NameUse,
    _Out_opt_ PULONG DomainSize, // WDK says _Out_ only + ... OPTIONAL
    _Inout_opt_ PUNICODE_STRING ReferencedDomain);
#endif

#if (NTDDI_VERSION >= NTDDI_WS03)
KSECDDDECLSPEC
NTSTATUS
SEC_ENTRY
SecLookupWellKnownSid(
    _In_ WELL_KNOWN_SID_TYPE SidType,
    _Out_ PSID Sid,
    _In_ ULONG SidBufferSize,
    _Inout_opt_ PULONG SidSize);
#endif

#if (NTDDI_VERSION >= NTDDI_VISTA)
KSECDDDECLSPEC
NTSTATUS
NTAPI
SecMakeSPNEx2(
    _In_ PUNICODE_STRING ServiceClass,
    _In_ PUNICODE_STRING ServiceName,
    _In_opt_ PUNICODE_STRING InstanceName,
    _In_opt_ USHORT InstancePort,
    _In_opt_ PUNICODE_STRING Referrer,
    _In_opt_ PUNICODE_STRING InTargetInfo,
    _Inout_ PUNICODE_STRING Spn,
    _Out_opt_ PULONG TotalSize,
    _In_ BOOLEAN Allocate,
    _In_ BOOLEAN IsTargetInfoMarshaled);
#endif

#endif /* ISSP_MODE == 0 */

#if (NTDDI_VERSION >= NTDDI_WIN7)

SECURITY_STATUS
SEC_ENTRY
SspiEncodeAuthIdentityAsStrings(
    _In_ PSEC_WINNT_AUTH_IDENTITY_OPAQUE pAuthIdentity,
    _Deref_out_opt_ PCWSTR* ppszUserName,
    _Deref_out_opt_ PCWSTR* ppszDomainName,
    _Deref_opt_out_opt_ PCWSTR* ppszPackedCredentialsString);

SECURITY_STATUS
SEC_ENTRY
SspiValidateAuthIdentity(
    _In_ PSEC_WINNT_AUTH_IDENTITY_OPAQUE AuthData);

SECURITY_STATUS
SEC_ENTRY
SspiCopyAuthIdentity(
    _In_ PSEC_WINNT_AUTH_IDENTITY_OPAQUE AuthData,
    _Deref_out_ PSEC_WINNT_AUTH_IDENTITY_OPAQUE* AuthDataCopy);

VOID
SEC_ENTRY
SspiFreeAuthIdentity(
    _In_opt_ PSEC_WINNT_AUTH_IDENTITY_OPAQUE AuthData);

VOID
SEC_ENTRY
SspiZeroAuthIdentity(
    _In_opt_ PSEC_WINNT_AUTH_IDENTITY_OPAQUE AuthData);

VOID
SEC_ENTRY
SspiLocalFree(
    _In_opt_ PVOID DataBuffer);

SECURITY_STATUS
SEC_ENTRY
SspiEncodeStringsAsAuthIdentity(
    _In_opt_ PCWSTR pszUserName,
    _In_opt_ PCWSTR pszDomainName,
    _In_opt_ PCWSTR pszPackedCredentialsString,
    _Deref_out_ PSEC_WINNT_AUTH_IDENTITY_OPAQUE* ppAuthIdentity);

SECURITY_STATUS
SEC_ENTRY
SspiCompareAuthIdentities(
    _In_opt_ PSEC_WINNT_AUTH_IDENTITY_OPAQUE AuthIdentity1,
    _In_opt_ PSEC_WINNT_AUTH_IDENTITY_OPAQUE AuthIdentity2,
    _Out_opt_ PBOOLEAN SameSuppliedUser,
    _Out_opt_ PBOOLEAN SameSuppliedIdentity);

SECURITY_STATUS
SEC_ENTRY
SspiMarshalAuthIdentity(
    _In_ PSEC_WINNT_AUTH_IDENTITY_OPAQUE AuthIdentity,
    _Out_ PULONG AuthIdentityLength,
    _Outptr_result_bytebuffer_(*AuthIdentityLength) PCHAR* AuthIdentityByteArray);

SECURITY_STATUS
SEC_ENTRY
SspiUnmarshalAuthIdentity(
    _In_ PULONG AuthIdentityLength,
    _In_reads_bytes_(AuthIdentityLength) PCHAR AuthIdentityByteArray,
    _Outptr_ PSEC_WINNT_AUTH_IDENTITY_OPAQUE* ppAuthIdentity);

BOOLEAN
SEC_ENTRY
SspiIsPromptingNeeded(
    _In_ PULONG ErrorOrNtStatus);

SECURITY_STATUS
SEC_ENTRY
SspiGetTargetHostName(
    _In_ PCWSTR pszTargetName,
    _Outptr_ PWSTR* pszHostName);

SECURITY_STATUS
SEC_ENTRY
SspiExcludePackage(
    _In_opt_ PSEC_WINNT_AUTH_IDENTITY_OPAQUE AuthIdentity,
    _In_ PCWSTR pszPackageName,
    _Outptr_ PSEC_WINNT_AUTH_IDENTITY_OPAQUE* ppNewAuthIdentity);

#define SEC_WINNT_AUTH_IDENTITY_MARSHALLED 0x04
#define SEC_WINNT_AUTH_IDENTITY_ONLY 0x08

#endif /* NTDDI_VERSION >= NTDDI_WIN7 */

#define FreeCredentialHandle FreeCredentialsHandle
struct _SECURITY_FUNCTION_TABLE_W
{
    ULONG dwVersion;
    ENUMERATE_SECURITY_PACKAGES_FN_W EnumerateSecurityPackagesW;
    QUERY_CREDENTIALS_ATTRIBUTES_FN_W QueryCredentialsAttributesW;
    ACQUIRE_CREDENTIALS_HANDLE_FN_W AcquireCredentialsHandleW;
    FREE_CREDENTIALS_HANDLE_FN FreeCredentialsHandle;
    PVOID Reserved2;
    INITIALIZE_SECURITY_CONTEXT_FN_W InitializeSecurityContextW;
    ACCEPT_SECURITY_CONTEXT_FN AcceptSecurityContext;
    COMPLETE_AUTH_TOKEN_FN CompleteAuthToken;
    DELETE_SECURITY_CONTEXT_FN DeleteSecurityContext;
    APPLY_CONTROL_TOKEN_FN ApplyControlToken;
    QUERY_CONTEXT_ATTRIBUTES_FN_W QueryContextAttributesW;
    IMPERSONATE_SECURITY_CONTEXT_FN ImpersonateSecurityContext;
    REVERT_SECURITY_CONTEXT_FN RevertSecurityContext;
    MAKE_SIGNATURE_FN MakeSignature;
    VERIFY_SIGNATURE_FN VerifySignature;
    FREE_CONTEXT_BUFFER_FN FreeContextBuffer;
    QUERY_SECURITY_PACKAGE_INFO_FN_W QuerySecurityPackageInfoW;
    PVOID Reserved3;
    PVOID Reserved4;
    EXPORT_SECURITY_CONTEXT_FN ExportSecurityContext;
    IMPORT_SECURITY_CONTEXT_FN_W ImportSecurityContextW;
    ADD_CREDENTIALS_FN_W AddCredentialsW ;
    PVOID Reserved8;
    QUERY_SECURITY_CONTEXT_TOKEN_FN QuerySecurityContextToken;
    ENCRYPT_MESSAGE_FN EncryptMessage;
    DECRYPT_MESSAGE_FN DecryptMessage;
#if OSVER(NTDDI_VERSION) > NTDDI_WIN2K
    SET_CONTEXT_ATTRIBUTES_FN_W SetContextAttributesW;
#endif
#if NTDDI_VERSION > NTDDI_WS03SP1
    SET_CREDENTIALS_ATTRIBUTES_FN_W SetCredentialsAttributesW;
#endif
#if ISSP_MODE != 0
    CHANGE_PASSWORD_FN_W ChangeAccountPasswordW;
#else
    PVOID Reserved9;
#endif
};

#endif /* !__SSPI_H__ */

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

#ifdef _NTSYSTEM_
extern PUSHORT NlsOemLeadByteInfo;
#define NLS_OEM_LEAD_BYTE_INFO NlsOemLeadByteInfo
#else
__CREATE_NTOS_DATA_IMPORT_ALIAS(NlsOemLeadByteInfo)
extern PUSHORT *NlsOemLeadByteInfo;
#define NLS_OEM_LEAD_BYTE_INFO (*NlsOemLeadByteInfo)
#endif

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
    _In_ PFILE_OBJECT FileObject,
    _Out_opt_ PLARGE_INTEGER OldestLsn
);

NTKERNELAPI
PVOID
NTAPI
FsRtlAllocatePool (
    _In_ POOL_TYPE PoolType,
    _In_ ULONG NumberOfBytes
);

NTKERNELAPI
PVOID
NTAPI
FsRtlAllocatePoolWithQuota (
    _In_ POOL_TYPE    PoolType,
    _In_ ULONG        NumberOfBytes
);

NTKERNELAPI
PVOID
NTAPI
FsRtlAllocatePoolWithQuotaTag (
    _In_ POOL_TYPE    PoolType,
    _In_ ULONG        NumberOfBytes,
    _In_ ULONG        Tag
);

NTKERNELAPI
PVOID
NTAPI
FsRtlAllocatePoolWithTag (
    _In_ POOL_TYPE    PoolType,
    _In_ ULONG        NumberOfBytes,
    _In_ ULONG        Tag
);

NTKERNELAPI
BOOLEAN
NTAPI
FsRtlMdlReadComplete (
    _In_ PFILE_OBJECT     FileObject,
    _In_ PMDL             MdlChain
);

NTKERNELAPI
BOOLEAN
NTAPI
FsRtlMdlWriteComplete (
    _In_ PFILE_OBJECT     FileObject,
    _In_ PLARGE_INTEGER   FileOffset,
    _In_ PMDL             MdlChain
);

NTKERNELAPI
VOID
NTAPI
FsRtlNotifyChangeDirectory (
    _In_ PNOTIFY_SYNC NotifySync,
    _In_ PVOID        FsContext,
    _In_ PSTRING      FullDirectoryName,
    _In_ PLIST_ENTRY  NotifyList,
    _In_ BOOLEAN      WatchTree,
    _In_ ULONG        CompletionFilter,
    _In_ PIRP         NotifyIrp
);

#if 1
NTKERNELAPI
NTSTATUS
NTAPI
ObCreateObject(
    _In_opt_ KPROCESSOR_MODE ObjectAttributesAccessMode,
    _In_ POBJECT_TYPE ObjectType,
    _In_opt_ POBJECT_ATTRIBUTES ObjectAttributes,
    _In_ KPROCESSOR_MODE AccessMode,
    _Inout_opt_ PVOID ParseContext,
    _In_ ULONG ObjectSize,
    _In_opt_ ULONG PagedPoolCharge,
    _In_opt_ ULONG NonPagedPoolCharge,
    _Out_ PVOID *Object
);

NTKERNELAPI
NTSTATUS
NTAPI
ObReferenceObjectByName (
    _In_ PUNICODE_STRING ObjectName,
    _In_ ULONG Attributes,
    _In_opt_ PACCESS_STATE PassedAccessState,
    _In_opt_ ACCESS_MASK DesiredAccess,
    _In_ POBJECT_TYPE ObjectType,
    _In_ KPROCESSOR_MODE AccessMode,
    _Inout_opt_ PVOID ParseContext,
    _Out_ PVOID *Object
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
    _In_ PCLIENT_ID   Cid,
    _Out_opt_ PEPROCESS   *Process,
    _Out_ PETHREAD    *Thread
);

NTSYSAPI
NTSTATUS
NTAPI
RtlSetSaclSecurityDescriptor (
    _Inout_ PSECURITY_DESCRIPTOR SecurityDescriptor,
    _In_ BOOLEAN                  SaclPresent,
    _In_ PACL                     Sacl,
    _In_ BOOLEAN                  SaclDefaulted
);

#define SeEnableAccessToExports() SeExports = *(PSE_EXPORTS *)SeExports;

#endif

#pragma pack(pop)

#ifdef __cplusplus
}
#endif
