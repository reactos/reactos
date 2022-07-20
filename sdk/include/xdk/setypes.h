/******************************************************************************
 *                            Security Manager Types                          *
 ******************************************************************************/
$if (_WDMDDK_ || _WINNT_)

/* Simple types */
typedef PVOID PSECURITY_DESCRIPTOR;
typedef $ULONG SECURITY_INFORMATION, *PSECURITY_INFORMATION;
typedef $ULONG ACCESS_MASK, *PACCESS_MASK;

typedef PVOID PACCESS_TOKEN;
typedef PVOID PSID;

#define DELETE                   0x00010000L
#define READ_CONTROL             0x00020000L
#define WRITE_DAC                0x00040000L
#define WRITE_OWNER              0x00080000L
#define SYNCHRONIZE              0x00100000L
#define STANDARD_RIGHTS_REQUIRED 0x000F0000L
#define STANDARD_RIGHTS_READ     READ_CONTROL
#define STANDARD_RIGHTS_WRITE    READ_CONTROL
#define STANDARD_RIGHTS_EXECUTE  READ_CONTROL
#define STANDARD_RIGHTS_ALL      0x001F0000L
#define SPECIFIC_RIGHTS_ALL      0x0000FFFFL
#define ACCESS_SYSTEM_SECURITY   0x01000000L
#define MAXIMUM_ALLOWED          0x02000000L
#define GENERIC_READ             0x80000000L
#define GENERIC_WRITE            0x40000000L
#define GENERIC_EXECUTE          0x20000000L
#define GENERIC_ALL              0x10000000L

typedef struct _GENERIC_MAPPING {
  ACCESS_MASK GenericRead;
  ACCESS_MASK GenericWrite;
  ACCESS_MASK GenericExecute;
  ACCESS_MASK GenericAll;
} GENERIC_MAPPING, *PGENERIC_MAPPING;

#define ACL_REVISION    2
#define ACL_REVISION_DS 4

#define ACL_REVISION1    1
#define ACL_REVISION2    2
#define ACL_REVISION3    3
#define ACL_REVISION4    4
#define MIN_ACL_REVISION ACL_REVISION2
#define MAX_ACL_REVISION ACL_REVISION4

typedef struct _ACL {
  $UCHAR AclRevision;
  $UCHAR Sbz1;
  $USHORT AclSize;
  $USHORT AceCount;
  $USHORT Sbz2;
} ACL, *PACL;

/* Current security descriptor revision value */
#define SECURITY_DESCRIPTOR_REVISION     (1)
#define SECURITY_DESCRIPTOR_REVISION1    (1)

/* Privilege attributes */
#define SE_PRIVILEGE_ENABLED_BY_DEFAULT (0x00000001L)
#define SE_PRIVILEGE_ENABLED            (0x00000002L)
#define SE_PRIVILEGE_REMOVED            (0x00000004L)
#define SE_PRIVILEGE_USED_FOR_ACCESS    (0x80000000L)

#define SE_PRIVILEGE_VALID_ATTRIBUTES   (SE_PRIVILEGE_ENABLED_BY_DEFAULT | \
                                         SE_PRIVILEGE_ENABLED            | \
                                         SE_PRIVILEGE_REMOVED            | \
                                         SE_PRIVILEGE_USED_FOR_ACCESS)

#include <pshpack4.h>
typedef struct _LUID_AND_ATTRIBUTES {
  LUID Luid;
  $ULONG Attributes;
} LUID_AND_ATTRIBUTES, *PLUID_AND_ATTRIBUTES;
#include <poppack.h>

typedef LUID_AND_ATTRIBUTES LUID_AND_ATTRIBUTES_ARRAY[ANYSIZE_ARRAY];
typedef LUID_AND_ATTRIBUTES_ARRAY *PLUID_AND_ATTRIBUTES_ARRAY;

/* Privilege sets */
#define PRIVILEGE_SET_ALL_NECESSARY (1)

typedef struct _PRIVILEGE_SET {
  $ULONG PrivilegeCount;
  $ULONG Control;
  LUID_AND_ATTRIBUTES Privilege[ANYSIZE_ARRAY];
} PRIVILEGE_SET, *PPRIVILEGE_SET;

typedef enum _SECURITY_IMPERSONATION_LEVEL {
  SecurityAnonymous,
  SecurityIdentification,
  SecurityImpersonation,
  SecurityDelegation
} SECURITY_IMPERSONATION_LEVEL, * PSECURITY_IMPERSONATION_LEVEL;

#define SECURITY_MAX_IMPERSONATION_LEVEL SecurityDelegation
#define SECURITY_MIN_IMPERSONATION_LEVEL SecurityAnonymous
#define DEFAULT_IMPERSONATION_LEVEL      SecurityImpersonation
#define VALID_IMPERSONATION_LEVEL(Level) (((Level) >= SECURITY_MIN_IMPERSONATION_LEVEL) && ((Level) <= SECURITY_MAX_IMPERSONATION_LEVEL))

#define SECURITY_DYNAMIC_TRACKING (TRUE)
#define SECURITY_STATIC_TRACKING (FALSE)

typedef BOOLEAN SECURITY_CONTEXT_TRACKING_MODE, *PSECURITY_CONTEXT_TRACKING_MODE;

typedef struct _SECURITY_QUALITY_OF_SERVICE {
  $ULONG Length;
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


#define OWNER_SECURITY_INFORMATION (0x00000001L)
#define GROUP_SECURITY_INFORMATION (0x00000002L)
#define DACL_SECURITY_INFORMATION  (0x00000004L)
#define SACL_SECURITY_INFORMATION  (0x00000008L)
#define LABEL_SECURITY_INFORMATION (0x00000010L)

#define PROTECTED_DACL_SECURITY_INFORMATION   (0x80000000L)
#define PROTECTED_SACL_SECURITY_INFORMATION   (0x40000000L)
#define UNPROTECTED_DACL_SECURITY_INFORMATION (0x20000000L)
#define UNPROTECTED_SACL_SECURITY_INFORMATION (0x10000000L)

/* Auto inherit ACE flags */
#define SEF_DACL_AUTO_INHERIT               0x01
#define SEF_SACL_AUTO_INHERIT               0x02
#define SEF_DEFAULT_DESCRIPTOR_FOR_OBJECT   0x04
#define SEF_AVOID_PRIVILEGE_CHECK           0x08
#define SEF_AVOID_OWNER_CHECK               0x10
#define SEF_DEFAULT_OWNER_FROM_PARENT       0x20
#define SEF_DEFAULT_GROUP_FROM_PARENT       0x40
#define SEF_MACL_NO_WRITE_UP                0x100
#define SEF_MACL_NO_READ_UP                 0x200
#define SEF_MACL_NO_EXECUTE_UP              0x400
#define SEF_AI_USE_EXTRA_PARAMS             0x800
#define SEF_AVOID_OWNER_RESTRICTION         0x1000
#define SEF_MACL_VALID_FLAGS (SEF_MACL_NO_WRITE_UP | SEF_MACL_NO_READ_UP | SEF_MACL_NO_EXECUTE_UP)

$endif (_WDMDDK_ || _WINNT_)

$if (_WINNT_)

/* Privilege token filtering flags */
#define DISABLE_MAX_PRIVILEGE 0x1
#define SANDBOX_INERT         0x2
#if (NTDDI_VERSION >= NTDDI_LONGHORN)
#define LUA_TOKEN             0x4
#define WRITE_RESTRICTED      0x8
#endif

$endif (_WINNT_)

$if (_WDMDDK_)

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
  _In_ PVOID Vcb,
  _In_ PSECURITY_DESCRIPTOR SecurityDescriptor);

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
$endif (_WDMDDK_)
$if (_NTDDK_)
#define SE_UNSOLICITED_INPUT_PRIVILEGE    6

$endif (_NTDDK_)
$if (_NTDDK_ || _WINNT_)

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
  WinApplicationPackageAuthoritySid = 83,
  WinBuiltinAnyPackageSid = 84,
  WinCapabilityInternetClientSid = 85,
  WinCapabilityInternetClientServerSid = 86,
  WinCapabilityPrivateNetworkClientServerSid = 87,
  WinCapabilityPicturesLibrarySid = 88,
  WinCapabilityVideosLibrarySid = 89,
  WinCapabilityMusicLibrarySid = 90,
  WinCapabilityDocumentsLibrarySid = 91,
  WinCapabilitySharedUserCertificatesSid = 92,
  WinCapabilityEnterpriseAuthenticationSid = 93,
  WinCapabilityRemovableStorageSid = 94,
  WinBuiltinRDSRemoteAccessServersSid = 95,
  WinBuiltinRDSEndpointServersSid = 96,
  WinBuiltinRDSManagementServersSid = 97,
  WinUserModeDriversSid = 98,
  WinBuiltinHyperVAdminsSid = 99,
  WinAccountCloneableControllersSid = 100,
  WinBuiltinAccessControlAssistanceOperatorsSid = 101,
  WinBuiltinRemoteManagementUsersSid = 102,
  WinAuthenticationAuthorityAssertedSid = 103,
  WinAuthenticationServiceAssertedSid = 104,
  WinLocalAccountSid = 105,
  WinLocalAccountAndAdministratorSid = 106,
  WinAccountProtectedUsersSid = 107,
} WELL_KNOWN_SID_TYPE;

$endif (_NTDDK_ || _WINNT_)
$if (_NTIFS_ || _WINNT_)

#ifndef SID_IDENTIFIER_AUTHORITY_DEFINED
#define SID_IDENTIFIER_AUTHORITY_DEFINED
typedef struct _SID_IDENTIFIER_AUTHORITY {
  $UCHAR Value[6];
} SID_IDENTIFIER_AUTHORITY,*PSID_IDENTIFIER_AUTHORITY,*LPSID_IDENTIFIER_AUTHORITY;
#endif

#ifndef SID_DEFINED
#define SID_DEFINED
typedef struct _SID {
  $UCHAR Revision;
  $UCHAR SubAuthorityCount;
  SID_IDENTIFIER_AUTHORITY IdentifierAuthority;
#ifdef MIDL_PASS
  [size_is(SubAuthorityCount)] $ULONG SubAuthority[*];
#else
  $ULONG SubAuthority[ANYSIZE_ARRAY];
#endif
} SID, *PISID;
#endif

#define SID_REVISION                    1
#define SID_MAX_SUB_AUTHORITIES         15
#define SID_RECOMMENDED_SUB_AUTHORITIES 1

#ifndef MIDL_PASS
#define SECURITY_MAX_SID_SIZE (sizeof(SID) - sizeof($ULONG) + (SID_MAX_SUB_AUTHORITIES * sizeof($ULONG)))
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
  $ULONG Attributes;
} SID_AND_ATTRIBUTES, *PSID_AND_ATTRIBUTES;
typedef SID_AND_ATTRIBUTES SID_AND_ATTRIBUTES_ARRAY[ANYSIZE_ARRAY];
typedef SID_AND_ATTRIBUTES_ARRAY *PSID_AND_ATTRIBUTES_ARRAY;

#define SID_HASH_SIZE 32
typedef ULONG_PTR SID_HASH_ENTRY, *PSID_HASH_ENTRY;

typedef struct _SID_AND_ATTRIBUTES_HASH {
  $ULONG SidCount;
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

/* Logon session reference flags */

#define SEP_LOGON_SESSION_TERMINATION_NOTIFY   0x0001

typedef struct _ACE_HEADER {
  $UCHAR AceType;
  $UCHAR AceFlags;
  $USHORT AceSize;
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
  $ULONG SidStart;
} ACCESS_ALLOWED_ACE, *PACCESS_ALLOWED_ACE;

typedef struct _ACCESS_DENIED_ACE {
  ACE_HEADER Header;
  ACCESS_MASK Mask;
  $ULONG SidStart;
} ACCESS_DENIED_ACE, *PACCESS_DENIED_ACE;

typedef struct _ACCESS_ALLOWED_OBJECT_ACE {
  ACE_HEADER Header;
  ACCESS_MASK Mask;
  $ULONG Flags;
  GUID ObjectType;
  GUID InheritedObjectType;
  $ULONG SidStart;
} ACCESS_ALLOWED_OBJECT_ACE, *PACCESS_ALLOWED_OBJECT_ACE;

typedef struct _ACCESS_DENIED_OBJECT_ACE {
  ACE_HEADER  Header;
  ACCESS_MASK Mask;
  $ULONG Flags;
  GUID ObjectType;
  GUID InheritedObjectType;
  $ULONG SidStart;
} ACCESS_DENIED_OBJECT_ACE, *PACCESS_DENIED_OBJECT_ACE;

typedef struct _SYSTEM_AUDIT_ACE {
  ACE_HEADER Header;
  ACCESS_MASK Mask;
  $ULONG SidStart;
} SYSTEM_AUDIT_ACE, *PSYSTEM_AUDIT_ACE;

typedef struct _SYSTEM_ALARM_ACE {
  ACE_HEADER Header;
  ACCESS_MASK Mask;
  $ULONG SidStart;
} SYSTEM_ALARM_ACE, *PSYSTEM_ALARM_ACE;

typedef struct _SYSTEM_MANDATORY_LABEL_ACE {
  ACE_HEADER Header;
  ACCESS_MASK Mask;
  $ULONG SidStart;
} SYSTEM_MANDATORY_LABEL_ACE, *PSYSTEM_MANDATORY_LABEL_ACE;

#define SYSTEM_MANDATORY_LABEL_NO_WRITE_UP   0x1
#define SYSTEM_MANDATORY_LABEL_NO_READ_UP    0x2
#define SYSTEM_MANDATORY_LABEL_NO_EXECUTE_UP 0x4
#define SYSTEM_MANDATORY_LABEL_VALID_MASK    (SYSTEM_MANDATORY_LABEL_NO_WRITE_UP | \
                                              SYSTEM_MANDATORY_LABEL_NO_READ_UP  | \
                                              SYSTEM_MANDATORY_LABEL_NO_EXECUTE_UP)

#define SECURITY_DESCRIPTOR_MIN_LENGTH (sizeof(SECURITY_DESCRIPTOR))

typedef $USHORT SECURITY_DESCRIPTOR_CONTROL, *PSECURITY_DESCRIPTOR_CONTROL;

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
  $UCHAR Revision;
  $UCHAR Sbz1;
  SECURITY_DESCRIPTOR_CONTROL Control;
  $ULONG Owner;
  $ULONG Group;
  $ULONG Sacl;
  $ULONG Dacl;
} SECURITY_DESCRIPTOR_RELATIVE, *PISECURITY_DESCRIPTOR_RELATIVE;

typedef struct _SECURITY_DESCRIPTOR {
  $UCHAR Revision;
  $UCHAR Sbz1;
  SECURITY_DESCRIPTOR_CONTROL Control;
  PSID Owner;
  PSID Group;
  PACL Sacl;
  PACL Dacl;
} SECURITY_DESCRIPTOR, *PISECURITY_DESCRIPTOR;

typedef struct _OBJECT_TYPE_LIST {
  $USHORT Level;
  $USHORT Sbz;
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

typedef $ULONG ACCESS_REASON;

typedef struct _ACCESS_REASONS {
  ACCESS_REASON Data[32];
} ACCESS_REASONS, *PACCESS_REASONS;

#define SE_SECURITY_DESCRIPTOR_FLAG_NO_OWNER_ACE 0x00000001
#define SE_SECURITY_DESCRIPTOR_FLAG_NO_LABEL_ACE 0x00000002
#define SE_SECURITY_DESCRIPTOR_VALID_FLAGS       0x00000003

typedef struct _SE_SECURITY_DESCRIPTOR {
  $ULONG Size;
  $ULONG Flags;
  PSECURITY_DESCRIPTOR SecurityDescriptor;
} SE_SECURITY_DESCRIPTOR, *PSE_SECURITY_DESCRIPTOR;

typedef struct _SE_ACCESS_REQUEST {
  $ULONG Size;
  PSE_SECURITY_DESCRIPTOR SeSecurityDescriptor;
  ACCESS_MASK DesiredAccess;
  ACCESS_MASK PreviouslyGrantedAccess;
  PSID PrincipalSelfSid;
  PGENERIC_MAPPING GenericMapping;
  $ULONG ObjectTypeListCount;
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
  TokenIsAppContainer,
  TokenCapabilities,
  TokenAppContainerSid,
  TokenAppContainerNumber,
  TokenUserClaimAttributes,
  TokenDeviceClaimAttributes,
  TokenRestrictedUserClaimAttributes,
  TokenRestrictedDeviceClaimAttributes,
  TokenDeviceGroups,
  TokenRestrictedDeviceGroups,
  TokenSecurityAttributes,
  TokenIsRestricted,
  MaxTokenInfoClass
} TOKEN_INFORMATION_CLASS, *PTOKEN_INFORMATION_CLASS;

typedef struct _TOKEN_USER {
  SID_AND_ATTRIBUTES User;
} TOKEN_USER, *PTOKEN_USER;

typedef struct _TOKEN_GROUPS {
  $ULONG GroupCount;
#ifdef MIDL_PASS
  [size_is(GroupCount)] SID_AND_ATTRIBUTES Groups[*];
#else
  SID_AND_ATTRIBUTES Groups[ANYSIZE_ARRAY];
#endif
} TOKEN_GROUPS, *PTOKEN_GROUPS, *LPTOKEN_GROUPS;

typedef struct _TOKEN_PRIVILEGES {
  $ULONG PrivilegeCount;
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
  $ULONG SidCount;
  $ULONG SidLength;
  PSID_AND_ATTRIBUTES Sids;
  $ULONG RestrictedSidCount;
  $ULONG RestrictedSidLength;
  PSID_AND_ATTRIBUTES RestrictedSids;
  $ULONG PrivilegeCount;
  $ULONG PrivilegeLength;
  PLUID_AND_ATTRIBUTES Privileges;
  LUID AuthenticationId;
} TOKEN_GROUPS_AND_PRIVILEGES, *PTOKEN_GROUPS_AND_PRIVILEGES;

typedef struct _TOKEN_LINKED_TOKEN {
  HANDLE LinkedToken;
} TOKEN_LINKED_TOKEN, *PTOKEN_LINKED_TOKEN;

typedef struct _TOKEN_ELEVATION {
  $ULONG TokenIsElevated;
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
  $UCHAR PerUserPolicy[((POLICY_AUDIT_SUBCATEGORY_COUNT) >> 1) + 1];
} TOKEN_AUDIT_POLICY, *PTOKEN_AUDIT_POLICY;

#define TOKEN_SOURCE_LENGTH 8

typedef struct _TOKEN_SOURCE {
  CHAR SourceName[TOKEN_SOURCE_LENGTH];
  LUID SourceIdentifier;
} TOKEN_SOURCE, *PTOKEN_SOURCE;

#include <pshpack4.h>
typedef struct _TOKEN_STATISTICS {
  LUID TokenId;
  LUID AuthenticationId;
  LARGE_INTEGER ExpirationTime;
  TOKEN_TYPE TokenType;
  SECURITY_IMPERSONATION_LEVEL ImpersonationLevel;
  $ULONG DynamicCharged;
  $ULONG DynamicAvailable;
  $ULONG GroupCount;
  $ULONG PrivilegeCount;
  LUID ModifiedId;
} TOKEN_STATISTICS, *PTOKEN_STATISTICS;
#include <poppack.h>

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

$endif(_NTIFS_ || _WINNT_)
$if(_NTIFS_)

typedef struct _SE_ACCESS_REPLY {
  $ULONG Size;
  $ULONG ResultListCount;
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
  $ULONG Policy;
} TOKEN_MANDATORY_POLICY, *PTOKEN_MANDATORY_POLICY;

typedef struct _TOKEN_ACCESS_INFORMATION {
  PSID_AND_ATTRIBUTES_HASH SidHash;
  PSID_AND_ATTRIBUTES_HASH RestrictedSidHash;
  PTOKEN_PRIVILEGES Privileges;
  LUID AuthenticationId;
  TOKEN_TYPE TokenType;
  SECURITY_IMPERSONATION_LEVEL ImpersonationLevel;
  TOKEN_MANDATORY_POLICY MandatoryPolicy;
  $ULONG Flags;
} TOKEN_ACCESS_INFORMATION, *PTOKEN_ACCESS_INFORMATION;

#define TOKEN_HAS_TRAVERSE_PRIVILEGE    0x0001
#define TOKEN_HAS_BACKUP_PRIVILEGE      0x0002
#define TOKEN_HAS_RESTORE_PRIVILEGE     0x0004
#define TOKEN_WRITE_RESTRICTED          0x0008
#define TOKEN_HAS_ADMIN_GROUP           TOKEN_WRITE_RESTRICTED
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

$endif (_NTIFS_)
