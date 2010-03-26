/******************************************************************************
 *                            Security Manager Types                          *
 ******************************************************************************/
$if (_NTDDK_)
#define SE_UNSOLICITED_INPUT_PRIVILEGE    6

typedef enum _WELL_KNOWN_SID_TYPE {
  WinNullSid = 0,
  WinWorldSid = 1,
  WinLocalSid = 2,
  WinCreatorOwnerSid = 3,
  WinCreatorGroupSid = 4,
  WinCreatorOwnerServerSid = 5,
  WinCreatorGroupServerSid = 6,
  WinNtAuthoritySid = 7,
  WinDialupSid = 8,
  WinNetworkSid = 9,
  WinBatchSid = 10,
  WinInteractiveSid = 11,
  WinServiceSid = 12,
  WinAnonymousSid = 13,
  WinProxySid = 14,
  WinEnterpriseControllersSid = 15,
  WinSelfSid = 16,
  WinAuthenticatedUserSid = 17,
  WinRestrictedCodeSid = 18,
  WinTerminalServerSid = 19,
  WinRemoteLogonIdSid = 20,
  WinLogonIdsSid = 21,
  WinLocalSystemSid = 22,
  WinLocalServiceSid = 23,
  WinNetworkServiceSid = 24,
  WinBuiltinDomainSid = 25,
  WinBuiltinAdministratorsSid = 26,
  WinBuiltinUsersSid = 27,
  WinBuiltinGuestsSid = 28,
  WinBuiltinPowerUsersSid = 29,
  WinBuiltinAccountOperatorsSid = 30,
  WinBuiltinSystemOperatorsSid = 31,
  WinBuiltinPrintOperatorsSid = 32,
  WinBuiltinBackupOperatorsSid = 33,
  WinBuiltinReplicatorSid = 34,
  WinBuiltinPreWindows2000CompatibleAccessSid = 35,
  WinBuiltinRemoteDesktopUsersSid = 36,
  WinBuiltinNetworkConfigurationOperatorsSid = 37,
  WinAccountAdministratorSid = 38,
  WinAccountGuestSid = 39,
  WinAccountKrbtgtSid = 40,
  WinAccountDomainAdminsSid = 41,
  WinAccountDomainUsersSid = 42,
  WinAccountDomainGuestsSid = 43,
  WinAccountComputersSid = 44,
  WinAccountControllersSid = 45,
  WinAccountCertAdminsSid = 46,
  WinAccountSchemaAdminsSid = 47,
  WinAccountEnterpriseAdminsSid = 48,
  WinAccountPolicyAdminsSid = 49,
  WinAccountRasAndIasServersSid = 50,
  WinNTLMAuthenticationSid = 51,
  WinDigestAuthenticationSid = 52,
  WinSChannelAuthenticationSid = 53,
  WinThisOrganizationSid = 54,
  WinOtherOrganizationSid = 55,
  WinBuiltinIncomingForestTrustBuildersSid = 56,
  WinBuiltinPerfMonitoringUsersSid = 57,
  WinBuiltinPerfLoggingUsersSid = 58,
  WinBuiltinAuthorizationAccessSid = 59,
  WinBuiltinTerminalServerLicenseServersSid = 60,
  WinBuiltinDCOMUsersSid = 61,
  WinBuiltinIUsersSid = 62,
  WinIUserSid = 63,
  WinBuiltinCryptoOperatorsSid = 64,
  WinUntrustedLabelSid = 65,
  WinLowLabelSid = 66,
  WinMediumLabelSid = 67,
  WinHighLabelSid = 68,
  WinSystemLabelSid = 69,
  WinWriteRestrictedCodeSid = 70,
  WinCreatorOwnerRightsSid = 71,
  WinCacheablePrincipalsGroupSid = 72,
  WinNonCacheablePrincipalsGroupSid = 73,
  WinEnterpriseReadonlyControllersSid = 74,
  WinAccountReadonlyControllersSid = 75,
  WinBuiltinEventLogReadersGroup = 76,
  WinNewEnterpriseReadonlyControllersSid = 77,
  WinBuiltinCertSvcDComAccessGroup = 78,
  WinMediumPlusLabelSid = 79,
  WinLocalLogonSid = 80,
  WinConsoleLogonSid = 81,
  WinThisOrganizationCertificateSid = 82,
} WELL_KNOWN_SID_TYPE;
$endif

$if (_WDMDDK_)
/* Simple types */
typedef PVOID PSECURITY_DESCRIPTOR;
typedef ULONG SECURITY_INFORMATION, *PSECURITY_INFORMATION;
typedef ULONG ACCESS_MASK, *PACCESS_MASK;
typedef PVOID PACCESS_TOKEN;
typedef PVOID PSID;

#define DELETE                           0x00010000L
#define READ_CONTROL                     0x00020000L
#define WRITE_DAC                        0x00040000L
#define WRITE_OWNER                      0x00080000L
#define SYNCHRONIZE                      0x00100000L
#define STANDARD_RIGHTS_REQUIRED         0x000F0000L
#define STANDARD_RIGHTS_READ             READ_CONTROL
#define STANDARD_RIGHTS_WRITE            READ_CONTROL
#define STANDARD_RIGHTS_EXECUTE          READ_CONTROL
#define STANDARD_RIGHTS_ALL              0x001F0000L
#define SPECIFIC_RIGHTS_ALL              0x0000FFFFL
#define ACCESS_SYSTEM_SECURITY           0x01000000L
#define MAXIMUM_ALLOWED                  0x02000000L
#define GENERIC_READ                     0x80000000L
#define GENERIC_WRITE                    0x40000000L
#define GENERIC_EXECUTE                  0x20000000L
#define GENERIC_ALL                      0x10000000L

typedef struct _GENERIC_MAPPING {
  ACCESS_MASK GenericRead;
  ACCESS_MASK GenericWrite;
  ACCESS_MASK GenericExecute;
  ACCESS_MASK GenericAll;
} GENERIC_MAPPING, *PGENERIC_MAPPING;

#define ACL_REVISION                      2
#define ACL_REVISION_DS                   4

#define ACL_REVISION1                     1
#define ACL_REVISION2                     2
#define ACL_REVISION3                     3
#define ACL_REVISION4                     4
#define MIN_ACL_REVISION                  ACL_REVISION2
#define MAX_ACL_REVISION                  ACL_REVISION4

typedef struct _ACL {
  UCHAR AclRevision;
  UCHAR Sbz1;
  USHORT AclSize;
  USHORT AceCount;
  USHORT Sbz2;
} ACL, *PACL;

/* Current security descriptor revision value */
#define SECURITY_DESCRIPTOR_REVISION     (1)
#define SECURITY_DESCRIPTOR_REVISION1    (1)

/* Privilege attributes */
#define SE_PRIVILEGE_ENABLED_BY_DEFAULT (0x00000001L)
#define SE_PRIVILEGE_ENABLED            (0x00000002L)
#define SE_PRIVILEGE_REMOVED            (0X00000004L)
#define SE_PRIVILEGE_USED_FOR_ACCESS    (0x80000000L)

#define SE_PRIVILEGE_VALID_ATTRIBUTES   (SE_PRIVILEGE_ENABLED_BY_DEFAULT | \
                                         SE_PRIVILEGE_ENABLED            | \
                                         SE_PRIVILEGE_REMOVED            | \
                                         SE_PRIVILEGE_USED_FOR_ACCESS)

#include <pshpack4.h>
typedef struct _LUID_AND_ATTRIBUTES {
  LUID Luid;
  ULONG Attributes;
} LUID_AND_ATTRIBUTES, *PLUID_AND_ATTRIBUTES;
#include <poppack.h>

typedef LUID_AND_ATTRIBUTES LUID_AND_ATTRIBUTES_ARRAY[ANYSIZE_ARRAY];
typedef LUID_AND_ATTRIBUTES_ARRAY *PLUID_AND_ATTRIBUTES_ARRAY;

/* Privilege sets */
#define PRIVILEGE_SET_ALL_NECESSARY (1)

typedef struct _PRIVILEGE_SET {
  ULONG PrivilegeCount;
  ULONG Control;
  LUID_AND_ATTRIBUTES Privilege[ANYSIZE_ARRAY];
} PRIVILEGE_SET,*PPRIVILEGE_SET;

typedef enum _SECURITY_IMPERSONATION_LEVEL {
  SecurityAnonymous,
  SecurityIdentification,
  SecurityImpersonation,
  SecurityDelegation
} SECURITY_IMPERSONATION_LEVEL, * PSECURITY_IMPERSONATION_LEVEL;

#define SECURITY_MAX_IMPERSONATION_LEVEL SecurityDelegation
#define SECURITY_MIN_IMPERSONATION_LEVEL SecurityAnonymous
#define DEFAULT_IMPERSONATION_LEVEL SecurityImpersonation
#define VALID_IMPERSONATION_LEVEL(Level) (((Level) >= SECURITY_MIN_IMPERSONATION_LEVEL) && ((Level) <= SECURITY_MAX_IMPERSONATION_LEVEL))

#define SECURITY_DYNAMIC_TRACKING (TRUE)
#define SECURITY_STATIC_TRACKING (FALSE)

typedef BOOLEAN SECURITY_CONTEXT_TRACKING_MODE, *PSECURITY_CONTEXT_TRACKING_MODE;

typedef struct _SECURITY_QUALITY_OF_SERVICE {
  ULONG Length;
  SECURITY_IMPERSONATION_LEVEL ImpersonationLevel;
  SECURITY_CONTEXT_TRACKING_MODE ContextTrackingMode;
  BOOLEAN EffectiveOnly;
} SECURITY_QUALITY_OF_SERVICE, *PSECURITY_QUALITY_OF_SERVICE;

typedef struct _SE_IMPERSONATION_STATE {
  PACCESS_TOKEN Token;
  BOOLEAN CopyOnOpen;
  BOOLEAN EffectiveOnly;
  SECURITY_IMPERSONATION_LEVEL Level;
} SE_IMPERSONATION_STATE, *PSE_IMPERSONATION_STATE;

#define OWNER_SECURITY_INFORMATION       (0x00000001L)
#define GROUP_SECURITY_INFORMATION       (0x00000002L)
#define DACL_SECURITY_INFORMATION        (0x00000004L)
#define SACL_SECURITY_INFORMATION        (0x00000008L)
#define LABEL_SECURITY_INFORMATION       (0x00000010L)

#define PROTECTED_DACL_SECURITY_INFORMATION     (0x80000000L)
#define PROTECTED_SACL_SECURITY_INFORMATION     (0x40000000L)
#define UNPROTECTED_DACL_SECURITY_INFORMATION   (0x20000000L)
#define UNPROTECTED_SACL_SECURITY_INFORMATION   (0x10000000L)

typedef enum _SECURITY_OPERATION_CODE {
  SetSecurityDescriptor,
  QuerySecurityDescriptor,
  DeleteSecurityDescriptor,
  AssignSecurityDescriptor
} SECURITY_OPERATION_CODE, *PSECURITY_OPERATION_CODE;

#define INITIAL_PRIVILEGE_COUNT           3

typedef struct _INITIAL_PRIVILEGE_SET {
  ULONG PrivilegeCount;
  ULONG Control;
  LUID_AND_ATTRIBUTES Privilege[INITIAL_PRIVILEGE_COUNT];
} INITIAL_PRIVILEGE_SET, * PINITIAL_PRIVILEGE_SET;

#define SE_MIN_WELL_KNOWN_PRIVILEGE         2
#define SE_CREATE_TOKEN_PRIVILEGE           2
#define SE_ASSIGNPRIMARYTOKEN_PRIVILEGE     3
#define SE_LOCK_MEMORY_PRIVILEGE            4
#define SE_INCREASE_QUOTA_PRIVILEGE         5
#define SE_MACHINE_ACCOUNT_PRIVILEGE        6
#define SE_TCB_PRIVILEGE                    7
#define SE_SECURITY_PRIVILEGE               8
#define SE_TAKE_OWNERSHIP_PRIVILEGE         9
#define SE_LOAD_DRIVER_PRIVILEGE            10
#define SE_SYSTEM_PROFILE_PRIVILEGE         11
#define SE_SYSTEMTIME_PRIVILEGE             12
#define SE_PROF_SINGLE_PROCESS_PRIVILEGE    13
#define SE_INC_BASE_PRIORITY_PRIVILEGE      14
#define SE_CREATE_PAGEFILE_PRIVILEGE        15
#define SE_CREATE_PERMANENT_PRIVILEGE       16
#define SE_BACKUP_PRIVILEGE                 17
#define SE_RESTORE_PRIVILEGE                18
#define SE_SHUTDOWN_PRIVILEGE               19
#define SE_DEBUG_PRIVILEGE                  20
#define SE_AUDIT_PRIVILEGE                  21
#define SE_SYSTEM_ENVIRONMENT_PRIVILEGE     22
#define SE_CHANGE_NOTIFY_PRIVILEGE          23
#define SE_REMOTE_SHUTDOWN_PRIVILEGE        24
#define SE_UNDOCK_PRIVILEGE                 25
#define SE_SYNC_AGENT_PRIVILEGE             26
#define SE_ENABLE_DELEGATION_PRIVILEGE      27
#define SE_MANAGE_VOLUME_PRIVILEGE          28
#define SE_IMPERSONATE_PRIVILEGE            29
#define SE_CREATE_GLOBAL_PRIVILEGE          30
#define SE_TRUSTED_CREDMAN_ACCESS_PRIVILEGE 31
#define SE_RELABEL_PRIVILEGE                32
#define SE_INC_WORKING_SET_PRIVILEGE        33
#define SE_TIME_ZONE_PRIVILEGE              34
#define SE_CREATE_SYMBOLIC_LINK_PRIVILEGE   35
#define SE_MAX_WELL_KNOWN_PRIVILEGE         SE_CREATE_SYMBOLIC_LINK_PRIVILEGE

typedef struct _SECURITY_SUBJECT_CONTEXT {
  PACCESS_TOKEN ClientToken;
  SECURITY_IMPERSONATION_LEVEL ImpersonationLevel;
  PACCESS_TOKEN PrimaryToken;
  PVOID ProcessAuditId;
} SECURITY_SUBJECT_CONTEXT, *PSECURITY_SUBJECT_CONTEXT;

typedef struct _ACCESS_STATE {
  LUID OperationID;
  BOOLEAN SecurityEvaluated;
  BOOLEAN GenerateAudit;
  BOOLEAN GenerateOnClose;
  BOOLEAN PrivilegesAllocated;
  ULONG Flags;
  ACCESS_MASK RemainingDesiredAccess;
  ACCESS_MASK PreviouslyGrantedAccess;
  ACCESS_MASK OriginalDesiredAccess;
  SECURITY_SUBJECT_CONTEXT SubjectSecurityContext;
  PSECURITY_DESCRIPTOR SecurityDescriptor;
  PVOID AuxData;
  union {
    INITIAL_PRIVILEGE_SET InitialPrivilegeSet;
    PRIVILEGE_SET PrivilegeSet;
  } Privileges;
  BOOLEAN AuditPrivileges;
  UNICODE_STRING ObjectName;
  UNICODE_STRING ObjectTypeName;
} ACCESS_STATE, *PACCESS_STATE;

typedef VOID
(NTAPI *PNTFS_DEREF_EXPORTED_SECURITY_DESCRIPTOR)(
  IN PVOID Vcb,
  IN PSECURITY_DESCRIPTOR SecurityDescriptor);

#ifndef _NTLSA_IFS_

#ifndef _NTLSA_AUDIT_
#define _NTLSA_AUDIT_

#define SE_MAX_AUDIT_PARAMETERS 32
#define SE_MAX_GENERIC_AUDIT_PARAMETERS 28

#define SE_ADT_OBJECT_ONLY 0x1

#define SE_ADT_PARAMETERS_SELF_RELATIVE    0x00000001
#define SE_ADT_PARAMETERS_SEND_TO_LSA      0x00000002
#define SE_ADT_PARAMETER_EXTENSIBLE_AUDIT  0x00000004
#define SE_ADT_PARAMETER_GENERIC_AUDIT     0x00000008
#define SE_ADT_PARAMETER_WRITE_SYNCHRONOUS 0x00000010

#define LSAP_SE_ADT_PARAMETER_ARRAY_TRUE_SIZE(Parameters) \
  ( sizeof(SE_ADT_PARAMETER_ARRAY) - sizeof(SE_ADT_PARAMETER_ARRAY_ENTRY) * \
    (SE_MAX_AUDIT_PARAMETERS - Parameters->ParameterCount) )

typedef enum _SE_ADT_PARAMETER_TYPE {
  SeAdtParmTypeNone = 0,
  SeAdtParmTypeString,
  SeAdtParmTypeFileSpec,
  SeAdtParmTypeUlong,
  SeAdtParmTypeSid,
  SeAdtParmTypeLogonId,
  SeAdtParmTypeNoLogonId,
  SeAdtParmTypeAccessMask,
  SeAdtParmTypePrivs,
  SeAdtParmTypeObjectTypes,
  SeAdtParmTypeHexUlong,
  SeAdtParmTypePtr,
  SeAdtParmTypeTime,
  SeAdtParmTypeGuid,
  SeAdtParmTypeLuid,
  SeAdtParmTypeHexInt64,
  SeAdtParmTypeStringList,
  SeAdtParmTypeSidList,
  SeAdtParmTypeDuration,
  SeAdtParmTypeUserAccountControl,
  SeAdtParmTypeNoUac,
  SeAdtParmTypeMessage,
  SeAdtParmTypeDateTime,
  SeAdtParmTypeSockAddr,
  SeAdtParmTypeSD,
  SeAdtParmTypeLogonHours,
  SeAdtParmTypeLogonIdNoSid,
  SeAdtParmTypeUlongNoConv,
  SeAdtParmTypeSockAddrNoPort,
  SeAdtParmTypeAccessReason
} SE_ADT_PARAMETER_TYPE, *PSE_ADT_PARAMETER_TYPE;

typedef struct _SE_ADT_OBJECT_TYPE {
  GUID ObjectType;
  USHORT Flags;
  USHORT Level;
  ACCESS_MASK AccessMask;
} SE_ADT_OBJECT_TYPE, *PSE_ADT_OBJECT_TYPE;

typedef struct _SE_ADT_PARAMETER_ARRAY_ENTRY {
  SE_ADT_PARAMETER_TYPE Type;
  ULONG Length;
  ULONG_PTR Data[2];
  PVOID Address;
} SE_ADT_PARAMETER_ARRAY_ENTRY, *PSE_ADT_PARAMETER_ARRAY_ENTRY;

typedef struct _SE_ADT_ACCESS_REASON {
  ACCESS_MASK AccessMask;
  ULONG AccessReasons[32];
  ULONG ObjectTypeIndex;
  ULONG AccessGranted;
  PSECURITY_DESCRIPTOR SecurityDescriptor;
} SE_ADT_ACCESS_REASON, *PSE_ADT_ACCESS_REASON;

typedef struct _SE_ADT_PARAMETER_ARRAY {
  ULONG CategoryId;
  ULONG AuditId;
  ULONG ParameterCount;
  ULONG Length;
  USHORT FlatSubCategoryId;
  USHORT Type;
  ULONG Flags;
  SE_ADT_PARAMETER_ARRAY_ENTRY Parameters[ SE_MAX_AUDIT_PARAMETERS ];
} SE_ADT_PARAMETER_ARRAY, *PSE_ADT_PARAMETER_ARRAY;

#endif /* !_NTLSA_AUDIT_ */
#endif /* !_NTLSA_IFS_ */
$endif

