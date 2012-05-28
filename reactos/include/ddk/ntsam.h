
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


typedef PVOID SAM_HANDLE, *PSAM_HANDLE;

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
SamOpenDomain(IN SAM_HANDLE ServerHandle,
              IN ACCESS_MASK DesiredAccess,
              IN PSID DomainId,
              OUT PSAM_HANDLE DomainHandle);

NTSTATUS
NTAPI
SamShutdownSamServer(IN SAM_HANDLE ServerHandle);


#ifdef __cplusplus
}
#endif

#endif /* _NTSAM_ */
