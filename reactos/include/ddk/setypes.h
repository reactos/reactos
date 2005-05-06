/* $Id$
 *
 * COPYRIGHT:         See COPYING in the top level directory for details
 * PROJECT:           ReactOS kernel
 * FILE:              include/ddk/setypes.h
 * PURPOSE:           Security manager types
 * REVISION HISTORY:
 *                 ??/??/??:    Created with empty stubs by David Welch
 *                 29/08/98:    ACCESS_TOKEN definition from Boudewijn Dekker
 */

#ifndef __INCLUDE_DDK_SETYPES_H
#define __INCLUDE_DDK_SETYPES_H

#include <ntos/security.h>

/* TOKEN_GROUPS structure */
#define SE_GROUP_MANDATORY                (0x1L)
#define SE_GROUP_ENABLED_BY_DEFAULT       (0x2L)
#define SE_GROUP_ENABLED                  (0x4L)
#define SE_GROUP_OWNER                    (0x8L)
#define SE_GROUP_LOGON_ID                 (0xC0000000L)

/* ACL Defines */
#define ACL_REVISION1     (1)
#define ACL_REVISION2     (2)
#define ACL_REVISION3     (3)
#define MIN_ACL_REVISION  ACL_REVISION2
#define MAX_ACL_REVISION  ACL_REVISION3

#define ACL_REVISION    (2)

/* ACE_HEADER structure */
#define ACCESS_MIN_MS_ACE_TYPE            (0x0)
#define ACCESS_ALLOWED_ACE_TYPE           (0x0)
#define ACCESS_DENIED_ACE_TYPE            (0x1)
#define SYSTEM_AUDIT_ACE_TYPE             (0x2)
#define SYSTEM_ALARM_ACE_TYPE             (0x3)
#define ACCESS_MAX_MS_V2_ACE_TYPE         (0x3)
#define ACCESS_ALLOWED_COMPOUND_ACE_TYPE  (0x4)
#define ACCESS_MAX_MS_V3_ACE_TYPE         (0x4)
#define ACCESS_MAX_MS_ACE_TYPE            (0x4)

/* ACE flags in the ACE_HEADER structure */
#define OBJECT_INHERIT_ACE           (0x1)
#define CONTAINER_INHERIT_ACE        (0x2)
#define NO_PROPAGATE_INHERIT_ACE     (0x4)
#define INHERIT_ONLY_ACE             (0x8)
#define SUCCESSFUL_ACCESS_ACE_FLAG   (0x40)
#define FAILED_ACCESS_ACE_FLAG       (0x80)

/* SECURITY_DESCRIPTOR_CONTROL */
#define SECURITY_DESCRIPTOR_REVISION    (1)
#define SECURITY_DESCRIPTOR_REVISION1   (1)
#define SECURITY_DESCRIPTOR_MIN_LENGTH  (20)
#define SE_OWNER_DEFAULTED              (0x0001)
#define SE_GROUP_DEFAULTED              (0x0002)
#define SE_DACL_PRESENT                 (0x0004)
#define SE_DACL_DEFAULTED               (0x0008)
#define SE_SACL_PRESENT                 (0x0010)
#define SE_SACL_DEFAULTED               (0x0020)
#define SE_RM_CONTROL_VALID             (0x4000)
#define SE_SELF_RELATIVE                (0x8000)

/* PRIVILEGE_SET */
#define SE_PRIVILEGE_ENABLED_BY_DEFAULT (0x1L)
#define SE_PRIVILEGE_ENABLED            (0x2L)
#define SE_PRIVILEGE_USED_FOR_ACCESS    (0x80000000L)
#define PRIVILEGE_SET_ALL_NECESSARY     (0x1)

/* SID */
#define SID_REVISION		(1)
#define SID_MAX_SUB_AUTHORITIES	(15)

typedef struct _SEP_AUDIT_POLICY_CATEGORIES {
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

typedef struct _SEP_AUDIT_POLICY_OVERLAY {
    ULONGLONG PolicyBits:36;
    UCHAR SetBit:1;
} SEP_AUDIT_POLICY_OVERLAY, *PSEP_AUDIT_POLICY_OVERLAY;

typedef struct _SEP_AUDIT_POLICY {
    union {
        SEP_AUDIT_POLICY_CATEGORIES PolicyElements;
        SEP_AUDIT_POLICY_OVERLAY PolicyOverlay;
        ULONGLONG Overlay;
    };
} SEP_AUDIT_POLICY, *PSEP_AUDIT_POLICY;
 
typedef struct _TOKEN {
  TOKEN_SOURCE TokenSource;                         /* 0x00 */
  LUID TokenId;                                     /* 0x10 */
  LUID AuthenticationId;                            /* 0x18 */
  LUID ParentTokenId;                               /* 0x20 */
  LARGE_INTEGER ExpirationTime;                     /* 0x28 */
  struct _ERESOURCE *TokenLock;                     /* 0x30 */
  SEP_AUDIT_POLICY  AuditPolicy;                    /* 0x38 */
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
  PVOID ProxyData;                                  /* 0x90 */
  PVOID AuditData;                                  /* 0x94 */
  LUID OriginatingLogonSession;                     /* 0x98 */
  ULONG VariablePart;                               /* 0xA0 */
} TOKEN, *PTOKEN;

typedef PVOID PACCESS_TOKEN;

typedef struct _SECURITY_SUBJECT_CONTEXT
{
  PACCESS_TOKEN ClientToken;                              /* 0x0 */
  SECURITY_IMPERSONATION_LEVEL ImpersonationLevel;        /* 0x4 */
  PACCESS_TOKEN PrimaryToken;                             /* 0x8 */
  PVOID ProcessAuditId;                                   /* 0xC */
} SECURITY_SUBJECT_CONTEXT, *PSECURITY_SUBJECT_CONTEXT;


typedef struct _SECURITY_CLIENT_CONTEXT
{
    SECURITY_QUALITY_OF_SERVICE SecurityQos;
    PACCESS_TOKEN               ClientToken;
    BOOLEAN                     DirectlyAccessClientToken;
    BOOLEAN                     DirectAccessEffectiveOnly;
    BOOLEAN                     ServerIsRemote;
    TOKEN_CONTROL               ClientTokenControl;
} SECURITY_CLIENT_CONTEXT, *PSECURITY_CLIENT_CONTEXT;

#ifndef __USE_W32API
typedef struct _SE_AUDIT_PROCESS_CREATION_INFO
{
    struct _OBJECT_NAME_INFORMATION *ImageFileName;
} SE_AUDIT_PROCESS_CREATION_INFO, *PSE_AUDIT_PROCESS_CREATION_INFO;
#endif

typedef struct _SE_EXPORTS
{
  /* Privilege values */
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

  /* Universally defined SIDs */
  PSID SeNullSid;
  PSID SeWorldSid;
  PSID SeLocalSid;
  PSID SeCreatorOwnerSid;
  PSID SeCreatorGroupSid;

  /* Nt defined SIDs */
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
} SE_EXPORTS, *PSE_EXPORTS;


typedef NTSTATUS STDCALL_FUNC
(*PSE_LOGON_SESSION_TERMINATED_ROUTINE)(IN PLUID LogonId);


typedef enum _SECURITY_OPERATION_CODE
{
  SetSecurityDescriptor,
  QuerySecurityDescriptor,
  DeleteSecurityDescriptor,
  AssignSecurityDescriptor
} SECURITY_OPERATION_CODE, *PSECURITY_OPERATION_CODE;

typedef struct _ACCESS_STATE
{
  LUID OperationID;
  BOOLEAN SecurityEvaluated;
  BOOLEAN GenerateAudit;
  BOOLEAN GenerateClose;
  BOOLEAN PrivilegesAllocated;
  ULONG Flags;
  ACCESS_MASK RemainingDesiredAccess;
  ACCESS_MASK PreviouslyGrantedAccess;
  ACCESS_MASK OriginallyDesiredAccess;
  SECURITY_SUBJECT_CONTEXT SubjectSecurityContext; /* 0x1C */
  PSECURITY_DESCRIPTOR SecurityDescriptor; /* 0x2C */
  PVOID AuxData; /* 0x30 */
  union
  {
    INITIAL_PRIVILEGE_SET InitialPrivilegeSet;
    PRIVILEGE_SET PrivilegeSet;
  } Privileges;
  BOOLEAN AuditPrivileges;
  UNICODE_STRING ObjectName;
  UNICODE_STRING ObjectTypeName;
} ACCESS_STATE, *PACCESS_STATE;

typedef struct _SE_IMPERSONATION_STATE {
	PVOID		Token;
	BOOLEAN		CopyOnOpen;
	BOOLEAN		EffectiveOnly;
	SECURITY_IMPERSONATION_LEVEL Level;
} SE_IMPERSONATION_STATE, *PSE_IMPERSONATION_STATE;

#endif

/* EOF */
