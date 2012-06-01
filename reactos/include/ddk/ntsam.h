
#ifndef _NTSAM_
#define _NTSAM_

#ifdef __cplusplus
extern "C" {
#endif

#define DOMAIN_READ_PASSWORD_PARAMETERS 1
#define DOMAIN_WRITE_PASSWORD_PARAMS 2
#define DOMAIN_READ_OTHER_PARAMETERS 4
#define DOMAIN_WRITE_OTHER_PARAMETERS 8
#define DOMAIN_CREATE_USER 16
#define DOMAIN_CREATE_GROUP 32
#define DOMAIN_CREATE_ALIAS 64
#define DOMAIN_GET_ALIAS_MEMBERSHIP 128
#define DOMAIN_LIST_ACCOUNTS 256
#define DOMAIN_LOOKUP 512
#define DOMAIN_ADMINISTER_SERVER 1024

#define SAM_SERVER_CONNECT 1
#define SAM_SERVER_SHUTDOWN 2
#define SAM_SERVER_INITIALIZE 4
#define SAM_SERVER_CREATE_DOMAIN 8
#define SAM_SERVER_ENUMERATE_DOMAINS 16
#define SAM_SERVER_LOOKUP_DOMAIN 32

#define USER_READ_GENERAL 1
#define USER_READ_PREFERENCES 2
#define USER_WRITE_PREFERENCES 4
#define USER_READ_LOGON 8
#define USER_READ_ACCOUNT 16
#define USER_WRITE_ACCOUNT 32
#define USER_CHANGE_PASSWORD 64
#define USER_FORCE_PASSWORD_CHANGE 128
#define USER_LIST_GROUPS 256
#define USER_READ_GROUP_INFORMATION 512
#define USER_WRITE_GROUP_INFORMATION 1024

typedef PVOID SAM_HANDLE, *PSAM_HANDLE;

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

typedef struct _DOMAIN_NAME_INFORMATION
{
    UNICODE_STRING DomainName;
} DOMAIN_NAME_INFORMATION, *PDOMAIN_NAME_INFORMATION;

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

typedef struct _USER_SET_PASSWORD_INFORMATION
{
    UNICODE_STRING Password;
    BOOLEAN PasswordExpired;
} USER_SET_PASSWORD_INFORMATION, *PUSER_SET_PASSWORD_INFORMATION;


NTSTATUS
NTAPI
SamCloseHandle(IN SAM_HANDLE SamHandle);

NTSTATUS
NTAPI
SamConnect(IN OUT PUNICODE_STRING ServerName,
           OUT PSAM_HANDLE ServerHandle,
           IN ACCESS_MASK DesiredAccess,
           IN POBJECT_ATTRIBUTES ObjectAttributes);

NTSTATUS
NTAPI
SamCreateUserInDomain(IN SAM_HANDLE DomainHandle,
                      IN PUNICODE_STRING AccountName,
                      IN ACCESS_MASK DesiredAccess,
                      OUT PSAM_HANDLE UserHandle,
                      OUT PULONG RelativeId);

NTSTATUS
NTAPI
SamFreeMemory(IN PVOID Buffer);

NTSTATUS
NTAPI
SamLookupDomainInSamServer(IN SAM_HANDLE ServerHandle,
                           IN PUNICODE_STRING Name,
                           OUT PSID *DomainId);

NTSTATUS
NTAPI
SamOpenDomain(IN SAM_HANDLE ServerHandle,
              IN ACCESS_MASK DesiredAccess,
              IN PSID DomainId,
              OUT PSAM_HANDLE DomainHandle);

NTSTATUS
NTAPI
SamOpenUser(IN SAM_HANDLE DomainHandle,
            IN ACCESS_MASK DesiredAccess,
            IN ULONG UserId,
            OUT PSAM_HANDLE UserHandle);

NTSTATUS
NTAPI
SamQueryInformationUser(IN SAM_HANDLE UserHandle,
                        IN USER_INFORMATION_CLASS UserInformationClass,
                        OUT PVOID *Buffer);

NTSTATUS
NTAPI
SamSetInformationDomain(IN SAM_HANDLE DomainHandle,
                        IN DOMAIN_INFORMATION_CLASS DomainInformationClass,
                        IN PVOID DomainInformation);

NTSTATUS
NTAPI
SamSetInformationUser(IN SAM_HANDLE UserHandle,
                      IN USER_INFORMATION_CLASS UserInformationClass,
                      IN PVOID Buffer);

NTSTATUS
NTAPI
SamShutdownSamServer(IN SAM_HANDLE ServerHandle);


#ifdef __cplusplus
}
#endif

#endif /* _NTSAM_ */
