
#ifndef _NTSAM_
#define _NTSAM_

#ifdef __cplusplus
extern "C" {
#endif

#define ALIAS_ADD_MEMBER                0x00000001
#define ALIAS_REMOVE_MEMBER             0x00000002
#define ALIAS_LIST_MEMBERS              0x00000004
#define ALIAS_READ_INFORMATION          0x00000008
#define ALIAS_WRITE_ACCOUNT             0x00000010

#define ALIAS_READ                     (STANDARD_RIGHTS_READ |\
                                        ALIAS_LIST_MEMBERS)

#define ALIAS_WRITE                    (STANDARD_RIGHTS_WRITE |\
                                        ALIAS_ADD_MEMBER |\
                                        ALIAS_REMOVE_MEMBER |\
                                        ALIAS_WRITE_ACCOUNT)

#define ALIAS_EXECUTE                  (STANDARD_RIGHTS_EXECUTE |\
                                        ALIAS_READ_INFORMATION)

#define ALIAS_ALL_ACCESS               (STANDARD_RIGHTS_REQUIRED |\
                                        ALIAS_ADD_MEMBER |\
                                        ALIAS_REMOVE_MEMBER |\
                                        ALIAS_LIST_MEMBERS |\
                                        ALIAS_READ_INFORMATION |\
                                        ALIAS_WRITE_ACCOUNT)

#define DOMAIN_READ_PASSWORD_PARAMETERS 0x00000001
#define DOMAIN_WRITE_PASSWORD_PARAMS    0x00000002
#define DOMAIN_READ_OTHER_PARAMETERS    0x00000004
#define DOMAIN_WRITE_OTHER_PARAMETERS   0x00000008
#define DOMAIN_CREATE_USER              0x00000010
#define DOMAIN_CREATE_GROUP             0x00000020
#define DOMAIN_CREATE_ALIAS             0x00000040
#define DOMAIN_GET_ALIAS_MEMBERSHIP     0x00000080
#define DOMAIN_LIST_ACCOUNTS            0x00000100
#define DOMAIN_LOOKUP                   0x00000200
#define DOMAIN_ADMINISTER_SERVER        0x00000400

#define DOMAIN_READ                    (STANDARD_RIGHTS_READ |\
                                        DOMAIN_READ_OTHER_PARAMETERS |\
                                        DOMAIN_GET_ALIAS_MEMBERSHIP)

#define DOMAIN_WRITE                   (STANDARD_RIGHTS_WRITE |\
                                        DOMAIN_WRITE_PASSWORD_PARAMS |\
                                        DOMAIN_WRITE_OTHER_PARAMETERS |\
                                        DOMAIN_CREATE_USER |\
                                        DOMAIN_CREATE_GROUP |\
                                        DOMAIN_CREATE_ALIAS |\
                                        DOMAIN_ADMINISTER_SERVER)

#define DOMAIN_EXECUTE                 (STANDARD_RIGHTS_EXECUTE |\
                                        DOMAIN_READ_PASSWORD_PARAMETERS |\
                                        DOMAIN_LIST_ACCOUNTS |\
                                        DOMAIN_LOOKUP)

#define DOMAIN_ALL_ACCESS              (STANDARD_RIGHTS_REQUIRED |\
                                        DOMAIN_READ_PASSWORD_PARAMETERS |\
                                        DOMAIN_WRITE_PASSWORD_PARAMS |\
                                        DOMAIN_READ_OTHER_PARAMETERS |\
                                        DOMAIN_WRITE_OTHER_PARAMETERS |\
                                        DOMAIN_CREATE_USER |\
                                        DOMAIN_CREATE_GROUP |\
                                        DOMAIN_CREATE_ALIAS |\
                                        DOMAIN_GET_ALIAS_MEMBERSHIP |\
                                        DOMAIN_LIST_ACCOUNTS |\
                                        DOMAIN_LOOKUP |\
                                        DOMAIN_ADMINISTER_SERVER)

#define GROUP_READ_INFORMATION          0x00000001
#define GROUP_WRITE_ACCOUNT             0x00000002
#define GROUP_ADD_MEMBER                0x00000004
#define GROUP_REMOVE_MEMBER             0x00000008
#define GROUP_LIST_MEMBERS              0x00000010

#define GROUP_READ                     (STANDARD_RIGHTS_READ |\
                                        GROUP_LIST_MEMBERS)

#define GROUP_WRITE                    (STANDARD_RIGHTS_WRITE |\
                                        GROUP_WRITE_ACCOUNT |\
                                        GROUP_ADD_MEMBER |\
                                        GROUP_REMOVE_MEMBER)

#define GROUP_EXECUTE                  (STANDARD_RIGHTS_EXECUTE |\
                                        GROUP_READ_INFORMATION)

#define GROUP_ALL_ACCESS               (STANDARD_RIGHTS_REQUIRED |\
                                        GROUP_READ_INFORMATION |\
                                        GROUP_WRITE_ACCOUNT |\
                                        GROUP_ADD_MEMBER |\
                                        GROUP_REMOVE_MEMBER |\
                                        GROUP_LIST_MEMBERS)

#define SAM_SERVER_CONNECT              0x00000001
#define SAM_SERVER_SHUTDOWN             0x00000002
#define SAM_SERVER_INITIALIZE           0x00000004
#define SAM_SERVER_CREATE_DOMAIN        0x00000008
#define SAM_SERVER_ENUMERATE_DOMAINS    0x00000010
#define SAM_SERVER_LOOKUP_DOMAIN        0x00000020

#define SAM_SERVER_READ                (STANDARD_RIGHTS_READ |\
                                        SAM_SERVER_ENUMERATE_DOMAINS)

#define SAM_SERVER_WRITE               (STANDARD_RIGHTS_WRITE |\
                                        SAM_SERVER_SHUTDOWN |\
                                        SAM_SERVER_INITIALIZE |\
                                        SAM_SERVER_CREATE_DOMAIN)

#define SAM_SERVER_EXECUTE             (STANDARD_RIGHTS_EXECUTE |\
                                        SAM_SERVER_CONNECT |\
                                        SAM_SERVER_LOOKUP_DOMAIN)

#define SAM_SERVER_ALL_ACCESS          (STANDARD_RIGHTS_REQUIRED |\
                                        SAM_SERVER_CONNECT |\
                                        SAM_SERVER_SHUTDOWN |\
                                        SAM_SERVER_INITIALIZE |\
                                        SAM_SERVER_CREATE_DOMAIN |\
                                        SAM_SERVER_ENUMERATE_DOMAINS |\
                                        SAM_SERVER_LOOKUP_DOMAIN)

#define USER_READ_GENERAL               0x00000001
#define USER_READ_PREFERENCES           0x00000002
#define USER_WRITE_PREFERENCES          0x00000004
#define USER_READ_LOGON                 0x00000008
#define USER_READ_ACCOUNT               0x00000010
#define USER_WRITE_ACCOUNT              0x00000020
#define USER_CHANGE_PASSWORD            0x00000040
#define USER_FORCE_PASSWORD_CHANGE      0x00000080
#define USER_LIST_GROUPS                0x00000100
#define USER_READ_GROUP_INFORMATION     0x00000200
#define USER_WRITE_GROUP_INFORMATION    0x00000400

#define USER_READ                      (STANDARD_RIGHTS_READ |\
                                        USER_READ_PREFERENCES |\
                                        USER_READ_LOGON |\
                                        USER_READ_ACCOUNT |\
                                        USER_LIST_GROUPS |\
                                        USER_READ_GROUP_INFORMATION)

#define USER_WRITE                     (STANDARD_RIGHTS_WRITE |\
                                        USER_WRITE_PREFERENCES |\
                                        USER_CHANGE_PASSWORD)

#define USER_EXECUTE                   (STANDARD_RIGHTS_EXECUTE |\
                                        USER_READ_GENERAL |\
                                        USER_CHANGE_PASSWORD)

#define USER_ALL_ACCESS                (STANDARD_RIGHTS_REQUIRED |\
                                        USER_READ_GENERAL |\
                                        USER_READ_PREFERENCES |\
                                        USER_WRITE_PREFERENCES |\
                                        USER_READ_LOGON |\
                                        USER_READ_ACCOUNT |\
                                        USER_WRITE_ACCOUNT |\
                                        USER_CHANGE_PASSWORD |\
                                        USER_FORCE_PASSWORD_CHANGE |\
                                        USER_LIST_GROUPS |\
                                        USER_READ_GROUP_INFORMATION |\
                                        USER_WRITE_GROUP_INFORMATION)

/* User account control bits */
#define USER_ACCOUNT_DISABLED                       0x00000001
#define USER_HOME_DIRECTORY_REQUIRED                0x00000002
#define USER_PASSWORD_NOT_REQUIRED                  0x00000004
#define USER_TEMP_DUPLICATE_ACCOUNT                 0x00000008
#define USER_NORMAL_ACCOUNT                         0x00000010
#define USER_MNS_LOGON_ACCOUNT                      0x00000020
#define USER_INTERDOMAIN_TRUST_ACCOUNT              0x00000040
#define USER_WORKSTATION_TRUST_ACCOUNT              0x00000080
#define USER_SERVER_TRUST_ACCOUNT                   0x00000100
#define USER_DONT_EXPIRE_PASSWORD                   0x00000200
#define USER_ACCOUNT_AUTO_LOCKED                    0x00000400
#define USER_ENCRYPTED_TEXT_PASSWORD_ALLOWED        0x00000800
#define USER_SMARTCARD_REQUIRED                     0x00001000
#define USER_TRUSTED_FOR_DELEGATION                 0x00002000
#define USER_NOT_DELEGATED                          0x00004000
#define USER_USE_DES_KEY_ONLY                       0x00008000
#define USER_DONT_REQUIRE_PREAUTH                   0x00010000
#define USER_PASSWORD_EXPIRED                       0x00020000
#define USER_TRUSTED_TO_AUTHENTICATE_FOR_DELEGATION 0x00040000
#define USER_NO_AUTH_DATA_REQUIRED                  0x00080000
#define USER_PARTIAL_SECRETS_ACCOUNT                0x00100000
#define USER_USE_AES_KEYS                           0x00200000

/* Constants used by LOGON_HOURS.UnitsPerWeek */
#define SAM_DAYS_PER_WEEK        (7)
#define SAM_HOURS_PER_WEEK       (24 * SAM_DAYS_PER_WEEK)
#define SAM_MINUTES_PER_WEEK     (60 * SAM_HOURS_PER_WEEK)

/* Flags used by USER_ALL_INFORMATION.WhichField */
#define USER_ALL_USERNAME           0x00000001
#define USER_ALL_FULLNAME           0x00000002
#define USER_ALL_USERID             0x00000004
#define USER_ALL_PRIMARYGROUPID     0x00000008
#define USER_ALL_ADMINCOMMENT       0x00000010
#define USER_ALL_USERCOMMENT        0x00000020
#define USER_ALL_HOMEDIRECTORY      0x00000040
#define USER_ALL_HOMEDIRECTORYDRIVE 0x00000080
#define USER_ALL_SCRIPTPATH         0x00000100
#define USER_ALL_PROFILEPATH        0x00000200
#define USER_ALL_WORKSTATIONS       0x00000400
#define USER_ALL_LASTLOGON          0x00000800
#define USER_ALL_LASTLOGOFF         0x00001000
#define USER_ALL_LOGONHOURS         0x00002000
#define USER_ALL_BADPASSWORDCOUNT   0x00004000
#define USER_ALL_LOGONCOUNT         0x00008000
#define USER_ALL_PASSWORDCANCHANGE  0x00010000
#define USER_ALL_PASSWORDMUSTCHANGE 0x00020000
#define USER_ALL_PASSWORDLASTSET    0x00040000
#define USER_ALL_ACCOUNTEXPIRES     0x00080000
#define USER_ALL_USERACCOUNTCONTROL 0x00100000
#define USER_ALL_PARAMETERS         0x00200000
#define USER_ALL_COUNTRYCODE        0x00400000
#define USER_ALL_CODEPAGE           0x00800000
#define USER_ALL_NTPASSWORDPRESENT  0x01000000
#define USER_ALL_LMPASSWORDPRESENT  0x02000000
#define USER_ALL_PRIVATEDATA        0x04000000
#define USER_ALL_PASSWORDEXPIRED    0x08000000
#define USER_ALL_SECURITYDESCRIPTOR 0x10000000
#define USER_ALL_OWFPASSWORD        0x20000000
#define USER_ALL_UNDEFINED_MASK     0xC0000000

#define USER_ALL_READ_GENERAL_MASK                0x0000003F
#define USER_ALL_READ_LOGON_MASK                  0x0003FFC0
#define USER_ALL_READ_ACCOUNT_MASK                0x003C0000
#define USER_ALL_READ_PREFERENCES_MASK            0x00C00000
#define USER_ALL_READ_TRUSTED_MASK                0x1F000000
#define USER_ALL_READ_CANT_MASK                   0xC0000000

#define USER_ALL_WRITE_ACCOUNT_MASK               0x003827DB
#define USER_ALL_WRITE_PREFERENCES_MASK           0x00C00020
#define USER_ALL_WRITE_FORCE_PASSWORD_CHANGE_MASK 0x0B000000
#define USER_ALL_WRITE_TRUSTED_MASK               0x1404D800
#define USER_ALL_WRITE_CANT_MASK                  0xC0030004

/* Values used by USER_PWD_CHANGE_FAILURE_INFORMATION.ExtendedFailureReason */
#define SAM_PWD_CHANGE_NO_ERROR                     0
#define SAM_PWD_CHANGE_PASSWORD_TOO_SHORT           1
#define SAM_PWD_CHANGE_PWD_IN_HISTORY               2
#define SAM_PWD_CHANGE_USERNAME_IN_PASSWORD         3
#define SAM_PWD_CHANGE_FULLNAME_IN_PASSWORD         4
#define SAM_PWD_CHANGE_NOT_COMPLEX                  5
#define SAM_PWD_CHANGE_MACHINE_PASSWORD_NOT_DEFAULT 6
#define SAM_PWD_CHANGE_FAILED_BY_FILTER             7
#define SAM_PWD_CHANGE_PASSWORD_TOO_LONG            8
#define SAM_PWD_CHANGE_FAILURE_REASON_MAX           8

/* Flags used by DOMAIN_PASSWORD_INFORMATION.PasswordProperties */
#define DOMAIN_PASSWORD_COMPLEX             0x00000001L
#define DOMAIN_PASSWORD_NO_ANON_CHANGE      0x00000002L
#define DOMAIN_PASSWORD_NO_CLEAR_CHANGE     0x00000004L
#define DOMAIN_LOCKOUT_ADMINS               0x00000008L
#define DOMAIN_PASSWORD_STORE_CLEARTEXT     0x00000010L
#define DOMAIN_REFUSE_PASSWORD_CHANGE       0x00000020L
#define DOMAIN_NO_LM_OWF_CHANGE             0x00000040L

typedef PVOID SAM_HANDLE, *PSAM_HANDLE;
typedef ULONG SAM_ENUMERATE_HANDLE, *PSAM_ENUMERATE_HANDLE;

typedef struct _SAM_RID_ENUMERATION
{
    ULONG RelativeId;
    UNICODE_STRING Name;
} SAM_RID_ENUMERATION, *PSAM_RID_ENUMERATION;

typedef struct _SAM_SID_ENUMERATION
{
    PSID Sid;
    UNICODE_STRING Name;
} SAM_SID_ENUMERATION, *PSAM_SID_ENUMERATION;

typedef enum _ALIAS_INFORMATION_CLASS
{
    AliasGeneralInformation = 1,
    AliasNameInformation,
    AliasAdminCommentInformation
} ALIAS_INFORMATION_CLASS, *PALIAS_INFORMATION_CLASS;

typedef struct _ALIAS_GENERAL_INFORMATION
{
    UNICODE_STRING Name;
    ULONG MemberCount;
    UNICODE_STRING AdminComment;
} ALIAS_GENERAL_INFORMATION, *PALIAS_GENERAL_INFORMATION;

typedef struct _ALIAS_NAME_INFORMATION
{
    UNICODE_STRING Name;
} ALIAS_NAME_INFORMATION, *PALIAS_NAME_INFORMATION;

typedef struct _ALIAS_ADM_COMMENT_INFORMATION
{
    UNICODE_STRING AdminComment;
} ALIAS_ADM_COMMENT_INFORMATION, *PALIAS_ADM_COMMENT_INFORMATION;

typedef enum _DOMAIN_DISPLAY_INFORMATION
{
    DomainDisplayUser = 1,
    DomainDisplayMachine,
    DomainDisplayGroup,
    DomainDisplayOemUser,
    DomainDisplayOemGroup,
    DomainDisplayServer
} DOMAIN_DISPLAY_INFORMATION, *PDOMAIN_DISPLAY_INFORMATION;

typedef struct _DOMAIN_DISPLAY_USER
{
    ULONG Index;
    ULONG Rid;
    ULONG AccountControl;
    UNICODE_STRING LogonName;
    UNICODE_STRING AdminComment;
    UNICODE_STRING FullName;
} DOMAIN_DISPLAY_USER, *PDOMAIN_DISPLAY_USER;

typedef struct _DOMAIN_DISPLAY_MACHINE
{
    ULONG Index;
    ULONG Rid;
    ULONG AccountControl;
    UNICODE_STRING Machine;
    UNICODE_STRING Comment;
} DOMAIN_DISPLAY_MACHINE, *PDOMAIN_DISPLAY_MACHINE;

typedef struct _DOMAIN_DISPLAY_GROUP
{
    ULONG Index;
    ULONG Rid;
    ULONG Attributes;
    UNICODE_STRING Group;
    UNICODE_STRING Comment;
} DOMAIN_DISPLAY_GROUP, *PDOMAIN_DISPLAY_GROUP;

typedef enum _DOMAIN_INFORMATION_CLASS
{
    DomainPasswordInformation = 1,
    DomainGeneralInformation,
    DomainLogoffInformation,
    DomainOemInformation,
    DomainNameInformation,
    DomainReplicationInformation,
    DomainServerRoleInformation,
    DomainModifiedInformation,
    DomainStateInformation,
    DomainUasInformation,
    DomainGeneralInformation2,
    DomainLockoutInformation,
    DomainModifiedInformation2
} DOMAIN_INFORMATION_CLASS;

typedef enum _DOMAIN_SERVER_ENABLE_STATE
{
    DomainServerEnabled = 1,
    DomainServerDisabled
} DOMAIN_SERVER_ENABLE_STATE, *PDOMAIN_SERVER_ENABLE_STATE;

typedef enum _DOMAIN_SERVER_ROLE
{
    DomainServerRoleBackup = 2,
    DomainServerRolePrimary
} DOMAIN_SERVER_ROLE, *PDOMAIN_SERVER_ROLE;

#ifndef _DOMAIN_PASSWORD_INFORMATION_DEFINED
#define _DOMAIN_PASSWORD_INFORMATION_DEFINED
typedef struct _DOMAIN_PASSWORD_INFORMATION
{
    USHORT MinPasswordLength;
    USHORT PasswordHistoryLength;
    ULONG PasswordProperties;
    LARGE_INTEGER MaxPasswordAge;
    LARGE_INTEGER MinPasswordAge;
} DOMAIN_PASSWORD_INFORMATION, *PDOMAIN_PASSWORD_INFORMATION;
#endif

#include "pshpack4.h"
typedef struct _DOMAIN_GENERAL_INFORMATION
{
    LARGE_INTEGER ForceLogoff;
    UNICODE_STRING OemInformation;
    UNICODE_STRING DomainName;
    UNICODE_STRING ReplicaSourceNodeName;
    LARGE_INTEGER DomainModifiedCount;
    DOMAIN_SERVER_ENABLE_STATE DomainServerState;
    DOMAIN_SERVER_ROLE DomainServerRole;
    BOOLEAN UasCompatibilityRequired;
    ULONG UserCount;
    ULONG GroupCount;
    ULONG AliasCount;
} DOMAIN_GENERAL_INFORMATION, *PDOMAIN_GENERAL_INFORMATION;
#include "poppack.h"

typedef struct _DOMAIN_LOGOFF_INFORMATION
{
    LARGE_INTEGER ForceLogoff;
} DOMAIN_LOGOFF_INFORMATION, *PDOMAIN_LOGOFF_INFORMATION;

typedef struct _DOMAIN_OEM_INFORMATION
{
    UNICODE_STRING OemInformation;
} DOMAIN_OEM_INFORMATION, *PDOMAIN_OEM_INFORMATION;

typedef struct _DOMAIN_NAME_INFORMATION
{
    UNICODE_STRING DomainName;
} DOMAIN_NAME_INFORMATION, *PDOMAIN_NAME_INFORMATION;

typedef struct _DOMAIN_REPLICATION_INFORMATION
{
    UNICODE_STRING ReplicaSourceNodeName;
} DOMAIN_REPLICATION_INFORMATION, *PDOMAIN_REPLICATION_INFORMATION;

typedef struct _DOMAIN_SERVER_ROLE_INFORMATION
{
    DOMAIN_SERVER_ROLE DomainServerRole;
} DOMAIN_SERVER_ROLE_INFORMATION, *PDOMAIN_SERVER_ROLE_INFORMATION;

typedef struct _DOMAIN_MODIFIED_INFORMATION
{
    LARGE_INTEGER DomainModifiedCount;
    LARGE_INTEGER CreationTime;
} DOMAIN_MODIFIED_INFORMATION, *PDOMAIN_MODIFIED_INFORMATION;

typedef struct _DOMAIN_STATE_INFORMATION
{
    DOMAIN_SERVER_ENABLE_STATE DomainServerState;
} DOMAIN_STATE_INFORMATION, *PDOMAIN_STATE_INFORMATION;

typedef struct _DOMAIN_UAS_INFORMATION
{
    BOOLEAN UasCompatibilityRequired;
} DOMAIN_UAS_INFORMATION;

#include "pshpack4.h"
typedef struct _DOMAIN_GENERAL_INFORMATION2
{
    DOMAIN_GENERAL_INFORMATION I1;
    LARGE_INTEGER LockoutDuration;
    LARGE_INTEGER LockoutObservationWindow;
    USHORT LockoutThreshold;
} DOMAIN_GENERAL_INFORMATION2, *PDOMAIN_GENERAL_INFORMATION2;
#include "poppack.h"

typedef struct _DOMAIN_LOCKOUT_INFORMATION
{
    LARGE_INTEGER LockoutDuration;
    LARGE_INTEGER LockoutObservationWindow;
    USHORT LockoutThreshold;
} DOMAIN_LOCKOUT_INFORMATION, *PDOMAIN_LOCKOUT_INFORMATION;

typedef struct _DOMAIN_MODIFIED_INFORMATION2
{
    LARGE_INTEGER DomainModifiedCount;
    LARGE_INTEGER CreationTime;
    LARGE_INTEGER ModifiedCountAtLastPromotion;
} DOMAIN_MODIFIED_INFORMATION2, *PDOMAIN_MODIFIED_INFORMATION2;

typedef enum _GROUP_INFORMATION_CLASS
{
    GroupGeneralInformation = 1,
    GroupNameInformation,
    GroupAttributeInformation,
    GroupAdminCommentInformation,
    GroupReplicationInformation
} GROUP_INFORMATION_CLASS;

typedef struct _GROUP_GENERAL_INFORMATION
{
    UNICODE_STRING Name;
    ULONG Attributes;
    ULONG MemberCount;
    UNICODE_STRING AdminComment;
} GROUP_GENERAL_INFORMATION, *PGROUP_GENERAL_INFORMATION;

typedef struct _GROUP_NAME_INFORMATION
{
    UNICODE_STRING Name;
} GROUP_NAME_INFORMATION, *PGROUP_NAME_INFORMATION;

typedef struct _GROUP_ATTRIBUTE_INFORMATION
{
    ULONG Attributes;
} GROUP_ATTRIBUTE_INFORMATION, *PGROUP_ATTRIBUTE_INFORMATION;

typedef struct GROUP_ADM_COMMENT_INFORMATION
{
    UNICODE_STRING AdminComment;
} GROUP_ADM_COMMENT_INFORMATION, *PGROUP_ADM_COMMENT_INFORMATION;

typedef struct _GROUP_MEMBERSHIP
{
    ULONG RelativeId;
    ULONG Attributes;
} GROUP_MEMBERSHIP, *PGROUP_MEMBERSHIP;

typedef struct _LOGON_HOURS
{
    USHORT UnitsPerWeek;
    PUCHAR LogonHours;
} LOGON_HOURS, *PLOGON_HOURS;

typedef struct _SR_SECURITY_DESCRIPTOR
{
    ULONG Length;
    PUCHAR SecurityDescriptor;
} SR_SECURITY_DESCRIPTOR, *PSR_SECURITY_DESCRIPTOR;

typedef enum _USER_INFORMATION_CLASS
{
    UserGeneralInformation = 1,
    UserPreferencesInformation,
    UserLogonInformation,
    UserLogonHoursInformation,
    UserAccountInformation,
    UserNameInformation,
    UserAccountNameInformation,
    UserFullNameInformation,
    UserPrimaryGroupInformation,
    UserHomeInformation,
    UserScriptInformation,
    UserProfileInformation,
    UserAdminCommentInformation,
    UserWorkStationsInformation,
    UserSetPasswordInformation,
    UserControlInformation,
    UserExpiresInformation,
    UserInternal1Information,
    UserInternal2Information,
    UserParametersInformation,
    UserAllInformation,
    UserInternal3Information,
    UserInternal4Information,
    UserInternal5Information,
    UserInternal4InformationNew,
    UserInternal5InformationNew,
    UserInternal6Information,
    UserExtendedInformation,
    UserLogonUIInformation,
} USER_INFORMATION_CLASS, *PUSER_INFORMATION_CLASS;

typedef struct _USER_GENERAL_INFORMATION
{
    UNICODE_STRING UserName;
    UNICODE_STRING FullName;
    ULONG PrimaryGroupId;
    UNICODE_STRING AdminComment;
    UNICODE_STRING UserComment;
} USER_GENERAL_INFORMATION, *PUSER_GENERAL_INFORMATION;

typedef struct _USER_PREFERENCES_INFORMATION
{
    UNICODE_STRING UserComment;
    UNICODE_STRING Reserved1;
    USHORT CountryCode;
    USHORT CodePage;
} USER_PREFERENCES_INFORMATION, *PUSER_PREFERENCES_INFORMATION;

#include "pshpack4.h"
typedef struct _USER_LOGON_INFORMATION
{
    UNICODE_STRING UserName;
    UNICODE_STRING FullName;
    ULONG UserId;
    ULONG PrimaryGroupId;
    UNICODE_STRING HomeDirectory;
    UNICODE_STRING HomeDirectoryDrive;
    UNICODE_STRING ScriptPath;
    UNICODE_STRING ProfilePath;
    UNICODE_STRING WorkStations;
    LARGE_INTEGER LastLogon;
    LARGE_INTEGER LastLogoff;
    LARGE_INTEGER PasswordLastSet;
    LARGE_INTEGER PasswordCanChange;
    LARGE_INTEGER PasswordMustChange;
    LOGON_HOURS LogonHours;
    USHORT BadPasswordCount;
    USHORT LogonCount;
    ULONG UserAccountControl;
} USER_LOGON_INFORMATION, *PUSER_LOGON_INFORMATION;
#include "poppack.h"

typedef struct _USER_LOGON_HOURS_INFORMATION
{
    LOGON_HOURS LogonHours;
} USER_LOGON_HOURS_INFORMATION, *PUSER_LOGON_HOURS_INFORMATION;

#include "pshpack4.h"
typedef struct _USER_ACCOUNT_INFORMATION
{
    UNICODE_STRING UserName;
    UNICODE_STRING FullName;
    ULONG UserId;
    ULONG PrimaryGroupId;
    UNICODE_STRING HomeDirectory;
    UNICODE_STRING HomeDirectoryDrive;
    UNICODE_STRING ScriptPath;
    UNICODE_STRING ProfilePath;
    UNICODE_STRING AdminComment;
    UNICODE_STRING WorkStations;
    LARGE_INTEGER LastLogon;
    LARGE_INTEGER LastLogoff;
    LOGON_HOURS LogonHours;
    USHORT BadPasswordCount;
    USHORT LogonCount;
    LARGE_INTEGER PasswordLastSet;
    LARGE_INTEGER AccountExpires;
    ULONG UserAccountControl;
} USER_ACCOUNT_INFORMATION, *PUSER_ACCOUNT_INFORMATION;
#include "poppack.h"

typedef struct _USER_NAME_INFORMATION
{
    UNICODE_STRING UserName;
    UNICODE_STRING FullName;
} USER_NAME_INFORMATION, *PUSER_NAME_INFORMATION;

typedef struct _USER_ACCOUNT_NAME_INFORMATION
{
    UNICODE_STRING UserName;
} USER_ACCOUNT_NAME_INFORMATION, *PUSER_ACCOUNT_NAME_INFORMATION;

typedef struct _USER_FULL_NAME_INFORMATION
{
    UNICODE_STRING FullName;
} USER_FULL_NAME_INFORMATION, *PUSER_FULL_NAME_INFORMATION;

typedef struct _USER_PRIMARY_GROUP_INFORMATION
{
    ULONG PrimaryGroupId;
} USER_PRIMARY_GROUP_INFORMATION, *PUSER_PRIMARY_GROUP_INFORMATION;

typedef struct _USER_HOME_INFORMATION
{
    UNICODE_STRING HomeDirectory;
    UNICODE_STRING HomeDirectoryDrive;
} USER_HOME_INFORMATION, *PUSER_HOME_INFORMATION;

typedef struct _USER_SCRIPT_INFORMATION
{
    UNICODE_STRING ScriptPath;
} USER_SCRIPT_INFORMATION, *PUSER_SCRIPT_INFORMATION;

typedef struct _USER_PROFILE_INFORMATION
{
    UNICODE_STRING ProfilePath;
} USER_PROFILE_INFORMATION, *PUSER_PROFILE_INFORMATION;

typedef struct _USER_ADMIN_COMMENT_INFORMATION
{
    UNICODE_STRING AdminComment;
} USER_ADMIN_COMMENT_INFORMATION, *PUSER_ADMIN_COMMENT_INFORMATION;

typedef struct _USER_WORKSTATIONS_INFORMATION
{
    UNICODE_STRING WorkStations;
} USER_WORKSTATIONS_INFORMATION, *PUSER_WORKSTATIONS_INFORMATION;

typedef struct _USER_SET_PASSWORD_INFORMATION
{
    UNICODE_STRING Password;
    BOOLEAN PasswordExpired;
} USER_SET_PASSWORD_INFORMATION, *PUSER_SET_PASSWORD_INFORMATION;

typedef struct _USER_CONTROL_INFORMATION
{
    ULONG UserAccountControl;
} USER_CONTROL_INFORMATION, *PUSER_CONTROL_INFORMATION;

typedef struct _USER_EXPIRES_INFORMATION
{
    LARGE_INTEGER AccountExpires;
} USER_EXPIRES_INFORMATION, *PUSER_EXPIRES_INFORMATION;

typedef struct _USER_PARAMETERS_INFORMATION
{
    UNICODE_STRING Parameters;
} USER_PARAMETERS_INFORMATION, *PUSER_PARAMETERS_INFORMATION;

#include "pshpack4.h"
typedef struct _USER_ALL_INFORMATION
{
    LARGE_INTEGER LastLogon;
    LARGE_INTEGER LastLogoff;
    LARGE_INTEGER PasswordLastSet;
    LARGE_INTEGER AccountExpires;
    LARGE_INTEGER PasswordCanChange;
    LARGE_INTEGER PasswordMustChange;
    UNICODE_STRING UserName;
    UNICODE_STRING FullName;
    UNICODE_STRING HomeDirectory;
    UNICODE_STRING HomeDirectoryDrive;
    UNICODE_STRING ScriptPath;
    UNICODE_STRING ProfilePath;
    UNICODE_STRING AdminComment;
    UNICODE_STRING WorkStations;
    UNICODE_STRING UserComment;
    UNICODE_STRING Parameters;
    UNICODE_STRING LmPassword;
    UNICODE_STRING NtPassword;
    UNICODE_STRING PrivateData;
    SR_SECURITY_DESCRIPTOR SecurityDescriptor;
    ULONG UserId;
    ULONG PrimaryGroupId;
    ULONG UserAccountControl;
    ULONG WhichFields;
    LOGON_HOURS LogonHours;
    USHORT BadPasswordCount;
    USHORT LogonCount;
    USHORT CountryCode;
    USHORT CodePage;
    BOOLEAN LmPasswordPresent;
    BOOLEAN NtPasswordPresent;
    BOOLEAN PasswordExpired;
    BOOLEAN PrivateDataSensitive;
} USER_ALL_INFORMATION, *PUSER_ALL_INFORMATION;
#include "poppack.h"

typedef struct _USER_PWD_CHANGE_FAILURE_INFORMATION
{
    ULONG ExtendedFailureReason;
    UNICODE_STRING FilterModuleName;
} USER_PWD_CHANGE_FAILURE_INFORMATION, *PUSER_PWD_CHANGE_FAILURE_INFORMATION;

#define SAM_SID_COMPATIBILITY_ALL    0
#define SAM_SID_COMPATIBILITY_LAX    1
#define SAM_SID_COMPATIBILITY_STRICT 2


NTSTATUS
NTAPI
SamAddMemberToAlias(IN SAM_HANDLE AliasHandle,
                    IN PSID MemberId);

NTSTATUS
NTAPI
SamAddMemberToGroup(IN SAM_HANDLE GroupHandle,
                    IN ULONG MemberId,
                    IN ULONG Attributes);

NTSTATUS
NTAPI
SamAddMultipleMembersToAlias(IN SAM_HANDLE AliasHandle,
                             IN PSID *MemberIds,
                             IN ULONG MemberCount);

NTSTATUS
NTAPI
SamChangePasswordUser(IN SAM_HANDLE UserHandle,
                      IN PUNICODE_STRING OldPassword,
                      IN PUNICODE_STRING NewPassword);

NTSTATUS
NTAPI
SamChangePasswordUser2(IN PUNICODE_STRING ServerName,
                       IN PUNICODE_STRING UserName,
                       IN PUNICODE_STRING OldPassword,
                       IN PUNICODE_STRING NewPassword);

NTSTATUS
NTAPI
SamChangePasswordUser3(IN PUNICODE_STRING ServerName,
                       IN PUNICODE_STRING UserName,
                       IN PUNICODE_STRING OldPassword,
                       IN PUNICODE_STRING NewPassword,
                       OUT PDOMAIN_PASSWORD_INFORMATION *EffectivePasswordPolicy,
                       OUT PUSER_PWD_CHANGE_FAILURE_INFORMATION *PasswordChangeFailureInfo);

NTSTATUS
NTAPI
SamCloseHandle(IN SAM_HANDLE SamHandle);

NTSTATUS
NTAPI
SamConnect(IN OUT PUNICODE_STRING ServerName OPTIONAL,
           OUT PSAM_HANDLE ServerHandle,
           IN ACCESS_MASK DesiredAccess,
           IN POBJECT_ATTRIBUTES ObjectAttributes);

NTSTATUS
NTAPI
SamCreateAliasInDomain(IN SAM_HANDLE DomainHandle,
                       IN PUNICODE_STRING AccountName,
                       IN ACCESS_MASK DesiredAccess,
                       OUT PSAM_HANDLE AliasHandle,
                       OUT PULONG RelativeId);

NTSTATUS
NTAPI
SamCreateGroupInDomain(IN SAM_HANDLE DomainHandle,
                       IN PUNICODE_STRING AccountName,
                       IN ACCESS_MASK DesiredAccess,
                       OUT PSAM_HANDLE GroupHandle,
                       OUT PULONG RelativeId);

NTSTATUS
NTAPI
SamCreateUser2InDomain(IN SAM_HANDLE DomainHandle,
                       IN PUNICODE_STRING AccountName,
                       IN ULONG AccountType,
                       IN ACCESS_MASK DesiredAccess,
                       OUT PSAM_HANDLE UserHandle,
                       OUT PULONG GrantedAccess,
                       OUT PULONG RelativeId);

NTSTATUS
NTAPI
SamCreateUserInDomain(IN SAM_HANDLE DomainHandle,
                      IN PUNICODE_STRING AccountName,
                      IN ACCESS_MASK DesiredAccess,
                      OUT PSAM_HANDLE UserHandle,
                      OUT PULONG RelativeId);

NTSTATUS
NTAPI
SamDeleteAlias(IN SAM_HANDLE AliasHandle);

NTSTATUS
NTAPI
SamDeleteGroup(IN SAM_HANDLE GroupHandle);

NTSTATUS
NTAPI
SamDeleteUser(IN SAM_HANDLE UserHandle);

NTSTATUS
NTAPI
SamEnumerateAliasesInDomain(IN SAM_HANDLE DomainHandle,
                            IN OUT PSAM_ENUMERATE_HANDLE EnumerationContext,
                            OUT PVOID *Buffer,
                            IN ULONG PreferedMaximumLength,
                            OUT PULONG CountReturned);

NTSTATUS
NTAPI
SamEnumerateDomainsInSamServer(IN SAM_HANDLE ServerHandle,
                               IN OUT PSAM_ENUMERATE_HANDLE EnumerationContext,
                               OUT PVOID *Buffer,
                               IN ULONG PreferedMaximumLength,
                               OUT PULONG CountReturned);

NTSTATUS
NTAPI
SamEnumerateGroupsInDomain(IN SAM_HANDLE DomainHandle,
                           IN OUT PSAM_ENUMERATE_HANDLE EnumerationContext,
                           IN PVOID *Buffer,
                           IN ULONG PreferedMaximumLength,
                           OUT PULONG CountReturned);

NTSTATUS
NTAPI
SamEnumerateUsersInDomain(IN SAM_HANDLE DomainHandle,
                          IN OUT PSAM_ENUMERATE_HANDLE EnumerationContext,
                          IN ULONG UserAccountControl,
                          OUT PVOID *Buffer,
                          IN ULONG PreferedMaximumLength,
                          OUT PULONG CountReturned);

NTSTATUS
NTAPI
SamFreeMemory(IN PVOID Buffer);

NTSTATUS
NTAPI
SamGetAliasMembership(IN SAM_HANDLE DomainHandle,
                      IN ULONG PassedCount,
                      IN PSID *Sids,
                      OUT PULONG MembershipCount,
                      OUT PULONG *Aliases);

NTSTATUS
NTAPI
SamGetCompatibilityMode(IN SAM_HANDLE ObjectHandle,
                        OUT PULONG Mode);

NTSTATUS
NTAPI
SamGetDisplayEnumerationIndex(IN SAM_HANDLE DomainHandle,
                              IN DOMAIN_DISPLAY_INFORMATION DisplayInformation,
                              IN PUNICODE_STRING Prefix,
                              OUT PULONG Index);

NTSTATUS
NTAPI
SamGetGroupsForUser(IN SAM_HANDLE UserHandle,
                    OUT PGROUP_MEMBERSHIP *Groups,
                    OUT PULONG MembershipCount);

NTSTATUS
NTAPI
SamGetMembersInAlias(IN SAM_HANDLE AliasHandle,
                     OUT PSID **MemberIds,
                     OUT PULONG MemberCount);

NTSTATUS
NTAPI
SamGetMembersInGroup(IN SAM_HANDLE GroupHandle,
                     OUT PULONG *MemberIds,
                     OUT PULONG *Attributes,
                     OUT PULONG MemberCount);

NTSTATUS
NTAPI
SamLookupDomainInSamServer(IN SAM_HANDLE ServerHandle,
                           IN PUNICODE_STRING Name,
                           OUT PSID *DomainId);

NTSTATUS
NTAPI
SamLookupIdsInDomain(IN SAM_HANDLE DomainHandle,
                     IN ULONG Count,
                     IN PULONG RelativeIds,
                     OUT PUNICODE_STRING *Names,
                     OUT PSID_NAME_USE *Use OPTIONAL);

NTSTATUS
NTAPI
SamLookupNamesInDomain(IN SAM_HANDLE DomainHandle,
                       IN ULONG Count,
                       IN PUNICODE_STRING Names,
                       OUT PULONG *RelativeIds,
                       OUT PSID_NAME_USE *Use);

NTSTATUS
NTAPI
SamOpenAlias(IN SAM_HANDLE DomainHandle,
             IN ACCESS_MASK DesiredAccess,
             IN ULONG AliasId,
             OUT PSAM_HANDLE AliasHandle);

NTSTATUS
NTAPI
SamOpenDomain(IN SAM_HANDLE ServerHandle,
              IN ACCESS_MASK DesiredAccess,
              IN PSID DomainId,
              OUT PSAM_HANDLE DomainHandle);

NTSTATUS
NTAPI
SamOpenGroup(IN SAM_HANDLE DomainHandle,
             IN ACCESS_MASK DesiredAccess,
             IN ULONG GroupId,
             OUT PSAM_HANDLE GroupHandle);

NTSTATUS
NTAPI
SamOpenUser(IN SAM_HANDLE DomainHandle,
            IN ACCESS_MASK DesiredAccess,
            IN ULONG UserId,
            OUT PSAM_HANDLE UserHandle);

NTSTATUS
NTAPI
SamQueryDisplayInformation(IN SAM_HANDLE DomainHandle,
                           IN DOMAIN_DISPLAY_INFORMATION DisplayInformation,
                           IN ULONG Index,
                           IN ULONG EntryCount,
                           IN ULONG PreferredMaximumLength,
                           OUT PULONG TotalAvailable,
                           OUT PULONG TotalReturned,
                           OUT PULONG ReturnedEntryCount,
                           OUT PVOID *SortedBuffer);

NTSTATUS
NTAPI
SamQueryInformationAlias(IN SAM_HANDLE AliasHandle,
                         IN ALIAS_INFORMATION_CLASS AliasInformationClass,
                         OUT PVOID *Buffer);

NTSTATUS
NTAPI
SamQueryInformationDomain(IN SAM_HANDLE DomainHandle,
                          IN DOMAIN_INFORMATION_CLASS DomainInformationClass,
                          OUT PVOID *Buffer);

NTSTATUS
NTAPI
SamQueryInformationGroup(IN SAM_HANDLE GroupHandle,
                         IN GROUP_INFORMATION_CLASS GroupInformationClass,
                         OUT PVOID *Buffer);

NTSTATUS
NTAPI
SamQueryInformationUser(IN SAM_HANDLE UserHandle,
                        IN USER_INFORMATION_CLASS UserInformationClass,
                        OUT PVOID *Buffer);

NTSTATUS
NTAPI
SamQuerySecurityObject(IN SAM_HANDLE ObjectHandle,
                       IN SECURITY_INFORMATION SecurityInformation,
                       OUT PSECURITY_DESCRIPTOR *SecurityDescriptor);

NTSTATUS
NTAPI
SamRemoveMemberFromAlias(IN SAM_HANDLE AliasHandle,
                         IN PSID MemberId);

NTSTATUS
NTAPI
SamRemoveMemberFromForeignDomain(IN SAM_HANDLE DomainHandle,
                                 IN PSID MemberId);

NTSTATUS
NTAPI
SamRemoveMemberFromGroup(IN SAM_HANDLE GroupHandle,
                         IN ULONG MemberId);

NTSTATUS
NTAPI
SamRemoveMultipleMembersFromAlias(IN SAM_HANDLE AliasHandle,
                                  IN PSID *MemberIds,
                                  IN ULONG MemberCount);

NTSTATUS
NTAPI
SamRidToSid(IN SAM_HANDLE ObjectHandle,
            IN ULONG Rid,
            OUT PSID *Sid);

NTSTATUS
NTAPI
SamSetInformationAlias(IN SAM_HANDLE AliasHandle,
                       IN ALIAS_INFORMATION_CLASS AliasInformationClass,
                       IN PVOID Buffer);

NTSTATUS
NTAPI
SamSetInformationDomain(IN SAM_HANDLE DomainHandle,
                        IN DOMAIN_INFORMATION_CLASS DomainInformationClass,
                        IN PVOID Buffer);

NTSTATUS
NTAPI
SamSetInformationGroup(IN SAM_HANDLE GroupHandle,
                       IN GROUP_INFORMATION_CLASS GroupInformationClass,
                       IN PVOID Buffer);

NTSTATUS
NTAPI
SamSetInformationUser(IN SAM_HANDLE UserHandle,
                      IN USER_INFORMATION_CLASS UserInformationClass,
                      IN PVOID Buffer);

NTSTATUS
NTAPI
SamSetMemberAttributesOfGroup(IN SAM_HANDLE GroupHandle,
                              IN ULONG MemberId,
                              IN ULONG Attributes);

NTSTATUS
NTAPI
SamSetSecurityObject(IN SAM_HANDLE ObjectHandle,
                     IN SECURITY_INFORMATION SecurityInformation,
                     IN PSECURITY_DESCRIPTOR SecurityDescriptor);

NTSTATUS
NTAPI
SamShutdownSamServer(IN SAM_HANDLE ServerHandle);

#ifdef __cplusplus
}
#endif

#endif /* _NTSAM_ */
