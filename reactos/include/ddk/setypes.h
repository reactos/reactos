/* $Id: setypes.h,v 1.9 2002/09/08 10:47:45 chorns Exp $
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
#define SE_GROUP_MANDATORY	(0x1L)
#define SE_GROUP_ENABLED_BY_DEFAULT	(0x2L)
#define SE_GROUP_ENABLED	(0x4L)
#define SE_GROUP_OWNER	(0x8L)
#define SE_GROUP_LOGON_ID	(0xc0000000L)

/* ACL Defines */
#define ACL_REVISION  (2)

/* ACE_HEADER structure */
#define ACCESS_ALLOWED_ACE_TYPE      (0x0)
#define ACCESS_DENIED_ACE_TYPE       (0x1)
#define SYSTEM_AUDIT_ACE_TYPE        (0x2)
#define SYSTEM_ALARM_ACE_TYPE        (0x3)

/* ACE flags in the ACE_HEADER structure */
#define OBJECT_INHERIT_ACE           (0x1)
#define CONTAINER_INHERIT_ACE        (0x2)
#define NO_PROPAGATE_INHERIT_ACE     (0x4)
#define INHERIT_ONLY_ACE             (0x8)
#define SUCCESSFUL_ACCESS_ACE_FLAG   (0x40)
#define FAILED_ACCESS_ACE_FLAG       (0x80)

/* SECURITY_DESCRIPTOR_CONTROL */
#define SECURITY_DESCRIPTOR_REVISION	(1)
#define SECURITY_DESCRIPTOR_MIN_LENGTH	(20)
#define SE_OWNER_DEFAULTED	(1)
#define SE_GROUP_DEFAULTED	(2)
#define SE_DACL_PRESENT	(4)
#define SE_DACL_DEFAULTED	(8)
#define SE_SACL_PRESENT	(16)
#define SE_SACL_DEFAULTED	(32)
#define SE_SELF_RELATIVE	(32768)

/* PRIVILEGE_SET */
#define SE_PRIVILEGE_ENABLED_BY_DEFAULT	(0x1L)
#define SE_PRIVILEGE_ENABLED	(0x2L)
#define SE_PRIVILEGE_USED_FOR_ACCESS	(0x80000000L)
#define PRIVILEGE_SET_ALL_NECESSARY	(0x1)

typedef struct _ACCESS_TOKEN
{
  TOKEN_SOURCE			TokenSource;               // 0x00
  LUID				TokenId;                   // 0x10
  LUID				AuthenticationId;          // 0x18
  LARGE_INTEGER			ExpirationTime;            // 0x20
  LUID				ModifiedId;                // 0x28
  ULONG				UserAndGroupCount;         // 0x30
  ULONG				PrivilegeCount;            // 0x34
  ULONG				VariableLength;            // 0x38
  ULONG				DynamicCharged;            // 0x3C
  ULONG				DynamicAvailable;          // 0x40
  ULONG				DefaultOwnerIndex;         // 0x44
  PSID_AND_ATTRIBUTES		UserAndGroups;             // 0x48
  PSID				PrimaryGroup;              // 0x4C
  PLUID_AND_ATTRIBUTES		Privileges;                // 0x50
  ULONG				Unknown1;                  // 0x54
  PACL				DefaultDacl;               // 0x58
  TOKEN_TYPE			TokenType;                 // 0x5C
  SECURITY_IMPERSONATION_LEVEL	ImpersonationLevel;        // 0x60
  UCHAR				TokenFlags;                // 0x64
  UCHAR				TokenInUse;                // 0x65
  UCHAR				Unused[2];                 // 0x66
  PVOID				ProxyData;                 // 0x68
  PVOID				AuditData;                 // 0x6c
  UCHAR				VariablePart[0];           // 0x70
} ACCESS_TOKEN, *PACCESS_TOKEN;


typedef struct _SECURITY_SUBJECT_CONTEXT
{
  PACCESS_TOKEN ClientToken;                              // 0x0
  SECURITY_IMPERSONATION_LEVEL ImpersonationLevel;        // 0x4
  PACCESS_TOKEN PrimaryToken;                             // 0x8
  PVOID ProcessAuditId;                                   // 0xC
} SECURITY_SUBJECT_CONTEXT, *PSECURITY_SUBJECT_CONTEXT;


typedef struct _SECURITY_CLIENT_CONTEXT
{
  SECURITY_QUALITY_OF_SERVICE SecurityQos;	// 0x00
  PACCESS_TOKEN Token;				// 0x0C
  BOOLEAN DirectlyAccessClientToken;		// 0x10
  BOOLEAN DirectAccessEffectiveOnly;		// 0x11
  BOOLEAN ServerIsRemote;			// 0x12
  TOKEN_CONTROL ClientTokenControl;		// 0x14
} SECURITY_CLIENT_CONTEXT, *PSECURITY_CLIENT_CONTEXT;


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


typedef NTSTATUS STDCALL
(*PSE_LOGON_SESSION_TERMINATED_ROUTINE)(IN PLUID LogonId);

#endif

/* EOF */
