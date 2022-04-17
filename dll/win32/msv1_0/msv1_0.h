/*
 * PROJECT:     Authentication Package DLL
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        dll/win32/msv1_0/msv1_0.h
 * PURPOSE:     Common header file
 * COPYRIGHT:   Copyright 2013 Eric Kohl
 */

#define FIXUP_POINTER(Pointer, Offset) ((Pointer != NULL) ? ((PWSTR)((ULONG_PTR)Pointer + Offset)) : NULL)


typedef struct _RPC_SID
{
    UCHAR Revision;
    UCHAR SubAuthorityCount;
    SID_IDENTIFIER_AUTHORITY IdentifierAuthority;
    DWORD SubAuthority[];
} RPC_SID, *PRPC_SID;

typedef struct _RPC_UNICODE_STRING
{
    unsigned short Length;
    unsigned short MaximumLength;
    wchar_t *Buffer;
} RPC_UNICODE_STRING, *PRPC_UNICODE_STRING;

typedef wchar_t *PSAMPR_SERVER_NAME;
typedef void *SAMPR_HANDLE;

typedef struct _OLD_LARGE_INTEGER
{
    unsigned long LowPart;
    long HighPart;
} OLD_LARGE_INTEGER, *POLD_LARGE_INTEGER;

typedef struct RPC_SHORT_BLOB
{
    unsigned short Length;
    unsigned short MaximumLength;
    unsigned short *Buffer;
} RPC_SHORT_BLOB, *PRPC_SHORT_BLOB;

typedef struct _SAMPR_SR_SECURITY_DESCRIPTOR
{
    unsigned long Length;
    unsigned char *SecurityDescriptor;
} SAMPR_SR_SECURITY_DESCRIPTOR, *PSAMPR_SR_SECURITY_DESCRIPTOR;

typedef struct _ENCRYPTED_LM_OWF_PASSWORD
{
    char data[16];
} ENCRYPTED_LM_OWF_PASSWORD, *PENCRYPTED_LM_OWF_PASSWORD, ENCRYPTED_NT_OWF_PASSWORD, *PENCRYPTED_NT_OWF_PASSWORD;

typedef struct _SAMPR_ULONG_ARRAY
{
    ULONG Count;
    PULONG Element;
} SAMPR_ULONG_ARRAY, *PSAMPR_ULONG_ARRAY;

typedef struct _SAMPR_LOGON_HOURS
{
    unsigned short UnitsPerWeek;
    unsigned char *LogonHours;
} SAMPR_LOGON_HOURS, *PSAMPR_LOGON_HOURS;

#define USER_LOGON_BAD_PASSWORD    0x08000000
#define USER_LOGON_SUCCESS         0x10000000

typedef struct _SAMPR_USER_INTERNAL2_INFORMATION
{
    unsigned long Flags;
    OLD_LARGE_INTEGER LastLogon;
    OLD_LARGE_INTEGER LastLogoff;
    unsigned short BadPasswordCount;
    unsigned short LogonCount;
} SAMPR_USER_INTERNAL2_INFORMATION, *PSAMPR_USER_INTERNAL2_INFORMATION;

typedef struct _SAMPR_USER_ALL_INFORMATION
{
    OLD_LARGE_INTEGER LastLogon;
    OLD_LARGE_INTEGER LastLogoff;
    OLD_LARGE_INTEGER PasswordLastSet;
    OLD_LARGE_INTEGER AccountExpires;
    OLD_LARGE_INTEGER PasswordCanChange;
    OLD_LARGE_INTEGER PasswordMustChange;
    RPC_UNICODE_STRING UserName;
    RPC_UNICODE_STRING FullName;
    RPC_UNICODE_STRING HomeDirectory;
    RPC_UNICODE_STRING HomeDirectoryDrive;
    RPC_UNICODE_STRING ScriptPath;
    RPC_UNICODE_STRING ProfilePath;
    RPC_UNICODE_STRING AdminComment;
    RPC_UNICODE_STRING WorkStations;
    RPC_UNICODE_STRING UserComment;
    RPC_UNICODE_STRING Parameters;
    RPC_SHORT_BLOB LmOwfPassword;
    RPC_SHORT_BLOB NtOwfPassword;
    RPC_UNICODE_STRING PrivateData;
    SAMPR_SR_SECURITY_DESCRIPTOR SecurityDescriptor;
    unsigned long UserId;
    unsigned long PrimaryGroupId;
    unsigned long UserAccountControl;
    unsigned long WhichFields;
    SAMPR_LOGON_HOURS LogonHours;
    unsigned short BadPasswordCount;
    unsigned short LogonCount;
    unsigned short CountryCode;
    unsigned short CodePage;
    unsigned char LmPasswordPresent;
    unsigned char NtPasswordPresent;
    unsigned char PasswordExpired;
    unsigned char PrivateDataSensitive;
} SAMPR_USER_ALL_INFORMATION, *PSAMPR_USER_ALL_INFORMATION;

typedef union _SAMPR_USER_INFO_BUFFER
{
#if 0
    SAMPR_USER_GENERAL_INFORMATION General;
    SAMPR_USER_PREFERENCES_INFORMATION Preferences;
    SAMPR_USER_LOGON_INFORMATION Logon;
    SAMPR_USER_LOGON_HOURS_INFORMATION LogonHours;
    SAMPR_USER_ACCOUNT_INFORMATION Account;
    SAMPR_USER_NAME_INFORMATION Name;
    SAMPR_USER_A_NAME_INFORMATION AccountName;
    SAMPR_USER_F_NAME_INFORMATION FullName;
    USER_PRIMARY_GROUP_INFORMATION PrimaryGroup;
    SAMPR_USER_HOME_INFORMATION Home;
    SAMPR_USER_SCRIPT_INFORMATION Script;
    SAMPR_USER_PROFILE_INFORMATION Profile;
    SAMPR_USER_ADMIN_COMMENT_INFORMATION AdminComment;
    SAMPR_USER_WORKSTATIONS_INFORMATION WorkStations;
    SAMPR_USER_SET_PASSWORD_INFORMATION SetPassword;
    USER_CONTROL_INFORMATION Control;
    USER_EXPIRES_INFORMATION Expires;
    SAMPR_USER_INTERNAL1_INFORMATION Internal1;
#endif
    SAMPR_USER_INTERNAL2_INFORMATION Internal2;
#if 0
    SAMPR_USER_PARAMETERS_INFORMATION Parameters;
#endif
    SAMPR_USER_ALL_INFORMATION All;
#if 0
    SAMPR_USER_INTERNAL4_INFORMATION Internal4;
    SAMPR_USER_INTERNAL5_INFORMATION Internal5;
    SAMPR_USER_INTERNAL4_INFORMATION_NEW Internal4New;
    SAMPR_USER_INTERNAL5_INFORMATION_NEW Internal5New;
#endif
} SAMPR_USER_INFO_BUFFER, *PSAMPR_USER_INFO_BUFFER;


NTSTATUS
NTAPI
SamIConnect(IN PSAMPR_SERVER_NAME ServerName,
            OUT SAMPR_HANDLE *ServerHandle,
            IN ACCESS_MASK DesiredAccess,
            IN BOOLEAN Trusted);

VOID
NTAPI
SamIFreeVoid(PVOID Ptr);

VOID
NTAPI
SamIFree_SAMPR_ULONG_ARRAY(PSAMPR_ULONG_ARRAY Ptr);

VOID
NTAPI
SamIFree_SAMPR_USER_INFO_BUFFER(PSAMPR_USER_INFO_BUFFER Ptr,
                                USER_INFORMATION_CLASS InformationClass);

NTSTATUS
NTAPI
SamrChangePasswordUser(IN SAMPR_HANDLE UserHandle,
                       IN unsigned char LmPresent,
                       IN PENCRYPTED_LM_OWF_PASSWORD OldLmEncryptedWithNewLm,
                       IN PENCRYPTED_LM_OWF_PASSWORD NewLmEncryptedWithOldLm,
                       IN unsigned char NtPresent,
                       IN PENCRYPTED_NT_OWF_PASSWORD OldNtEncryptedWithNewNt,
                       IN PENCRYPTED_NT_OWF_PASSWORD NewNtEncryptedWithOldNt,
                       IN unsigned char NtCrossEncryptionPresent,
                       IN PENCRYPTED_NT_OWF_PASSWORD NewNtEncryptedWithNewLm,
                       IN unsigned char LmCrossEncryptionPresent,
                       IN PENCRYPTED_LM_OWF_PASSWORD NewLmEncryptedWithNewNt);

NTSTATUS
NTAPI
SamrCloseHandle(IN OUT SAMPR_HANDLE *SamHandle);

NTSTATUS
NTAPI
SamrLookupDomainInSamServer(IN SAMPR_HANDLE ServerHandle,
                            IN PRPC_UNICODE_STRING Name,
                            OUT PRPC_SID *DomainId);

NTSTATUS
NTAPI
SamrLookupNamesInDomain(IN SAMPR_HANDLE DomainHandle,
                        IN ULONG Count,
                        IN RPC_UNICODE_STRING Names[],
                        OUT PSAMPR_ULONG_ARRAY RelativeIds,
                        OUT PSAMPR_ULONG_ARRAY Use);

NTSTATUS
NTAPI
SamrOpenDomain(IN SAMPR_HANDLE ServerHandle,
               IN ACCESS_MASK DesiredAccess,
               IN PRPC_SID DomainId,
               OUT SAMPR_HANDLE *DomainHandle);

NTSTATUS
NTAPI
SamrOpenUser(IN SAMPR_HANDLE DomainHandle,
             IN ACCESS_MASK DesiredAccess,
             IN ULONG UserId,
             OUT SAMPR_HANDLE *UserHandle);

NTSTATUS
NTAPI
SamrQueryInformationUser(IN SAMPR_HANDLE UserHandle,
                         IN USER_INFORMATION_CLASS UserInformationClass,
                         OUT PSAMPR_USER_INFO_BUFFER *Buffer);

NTSTATUS
NTAPI
SamrSetInformationUser(IN SAMPR_HANDLE UserHandle,
                       IN USER_INFORMATION_CLASS UserInformationClass,
                       IN PSAMPR_USER_INFO_BUFFER Buffer);

typedef PVOID LSAPR_HANDLE;

typedef struct _LSAPR_POLICY_AUDIT_EVENTS_INFO
{
    BOOLEAN AuditingMode;
    DWORD *EventAuditingOptions;
    DWORD MaximumAuditEventCount;
} LSAPR_POLICY_AUDIT_EVENTS_INFO, *PLSAPR_POLICY_AUDIT_EVENTS_INFO;

typedef struct _LSAPR_POLICY_PRIMARY_DOM_INFO
{
    RPC_UNICODE_STRING Name;
    PRPC_SID Sid;
} LSAPR_POLICY_PRIMARY_DOM_INFO, *PLSAPR_POLICY_PRIMARY_DOM_INFO;

typedef struct _LSAPR_POLICY_ACCOUNT_DOM_INFO
{
    RPC_UNICODE_STRING DomainName;
    PRPC_SID Sid;
} LSAPR_POLICY_ACCOUNT_DOM_INFO, *PLSAPR_POLICY_ACCOUNT_DOM_INFO;

typedef struct _LSAPR_POLICY_PD_ACCOUNT_INFO
{
    RPC_UNICODE_STRING Name;
} LSAPR_POLICY_PD_ACCOUNT_INFO, *PLSAPR_POLICY_PD_ACCOUNT_INFO;

typedef struct _POLICY_LSA_REPLICA_SRCE_INFO
{
    RPC_UNICODE_STRING ReplicaSource;
    RPC_UNICODE_STRING ReplicaAccountName;
} POLICY_LSA_REPLICA_SRCE_INFO, *PPOLICY_LSA_REPLICA_SRCE_INFO;

typedef struct _LSAPR_POLICY_DNS_DOMAIN_INFO
{
    RPC_UNICODE_STRING Name;
    RPC_UNICODE_STRING DnsDomainName;
    RPC_UNICODE_STRING DnsForestName;
    GUID DomainGuid;
    PRPC_SID Sid;
} LSAPR_POLICY_DNS_DOMAIN_INFO, *PLSAPR_POLICY_DNS_DOMAIN_INFO;

typedef union _LSAPR_POLICY_INFORMATION
{
    POLICY_AUDIT_LOG_INFO PolicyAuditLogInfo;
    LSAPR_POLICY_AUDIT_EVENTS_INFO PolicyAuditEventsInfo;
    LSAPR_POLICY_PRIMARY_DOM_INFO PolicyPrimaryDomInfo;
    LSAPR_POLICY_PD_ACCOUNT_INFO PolicyPdAccountInfo;
    LSAPR_POLICY_ACCOUNT_DOM_INFO PolicyAccountDomainInfo;
    POLICY_LSA_SERVER_ROLE_INFO PolicyServerRoleInfo;
    POLICY_LSA_REPLICA_SRCE_INFO PolicyReplicaSourceInfo;
    POLICY_DEFAULT_QUOTA_INFO PolicyDefaultQuotaInfo;
    POLICY_MODIFICATION_INFO PolicyModificationInfo;
    POLICY_AUDIT_FULL_SET_INFO PolicyAuditFullSetInfo;
    POLICY_AUDIT_FULL_QUERY_INFO PolicyAuditFullQueryInfo;
    LSAPR_POLICY_DNS_DOMAIN_INFO PolicyDnsDomainInfo;
    LSAPR_POLICY_DNS_DOMAIN_INFO PolicyDnsDomainInfoInt;
    LSAPR_POLICY_ACCOUNT_DOM_INFO PolicyLocalAccountDomainInfo;
} LSAPR_POLICY_INFORMATION, *PLSAPR_POLICY_INFORMATION;

VOID
NTAPI
LsaIFree_LSAPR_POLICY_INFORMATION(IN POLICY_INFORMATION_CLASS InformationClass,
                                  IN PLSAPR_POLICY_INFORMATION PolicyInformation);

NTSTATUS
WINAPI
LsaIOpenPolicyTrusted(OUT LSAPR_HANDLE *PolicyHandle);

NTSTATUS
WINAPI
LsarClose(IN OUT LSAPR_HANDLE *ObjectHandle);

NTSTATUS
WINAPI
LsarLookupPrivilegeValue(IN LSAPR_HANDLE PolicyHandle,
                         IN PRPC_UNICODE_STRING Name,
                         OUT PLUID Value);

NTSTATUS
WINAPI
LsarQueryInformationPolicy(IN LSAPR_HANDLE PolicyHandle,
                           IN POLICY_INFORMATION_CLASS InformationClass,
                           OUT PLSAPR_POLICY_INFORMATION *PolicyInformation);

NTSTATUS
WINAPI
SystemFunction006(LPCSTR password,
                  LPSTR hash);

NTSTATUS
WINAPI
SystemFunction007(PUNICODE_STRING string,
                  LPBYTE hash);

NTSTATUS
WINAPI
SystemFunction012(const BYTE *in,
                  const BYTE *key,
                  LPBYTE out);

NTSTATUS
NTAPI
LsaApCallPackage(
    _In_ PLSA_CLIENT_REQUEST ClientRequest,
    _In_ PVOID ProtocolSubmitBuffer,
    _In_ PVOID ClientBufferBase,
    _In_ ULONG SubmitBufferLength,
    _Out_ PVOID *ProtocolReturnBuffer,
    _Out_ PULONG ReturnBufferLength,
    _Out_ PNTSTATUS ProtocolStatus);

NTSTATUS
NTAPI
LsaApCallPackagePassthrough(
    _In_ PLSA_CLIENT_REQUEST ClientRequest,
    _In_ PVOID ProtocolSubmitBuffer,
    _In_ PVOID ClientBufferBase,
    _In_ ULONG SubmitBufferLength,
    _Out_ PVOID *ProtocolReturnBuffer,
    _Out_ PULONG ReturnBufferLength,
    _Out_ PNTSTATUS ProtocolStatus);

NTSTATUS
NTAPI
LsaApCallPackageUntrusted(
    _In_ PLSA_CLIENT_REQUEST ClientRequest,
    _In_ PVOID ProtocolSubmitBuffer,
    _In_ PVOID ClientBufferBase,
    _In_ ULONG SubmitBufferLength,
    _Out_ PVOID *ProtocolReturnBuffer,
    _Out_ PULONG ReturnBufferLength,
    _Out_ PNTSTATUS ProtocolStatus);

VOID
NTAPI
LsaApLogonTerminated(
    _In_ PLUID LogonId);

NTSTATUS
NTAPI
LsaApLogonUserEx2(
    _In_ PLSA_CLIENT_REQUEST ClientRequest,
    _In_ SECURITY_LOGON_TYPE LogonType,
    _In_ PVOID ProtocolSubmitBuffer,
    _In_ PVOID ClientBufferBase,
    _In_ ULONG SubmitBufferSize,
    _Out_ PVOID *ProfileBuffer,
    _Out_ PULONG ProfileBufferSize,
    _Out_ PLUID LogonId,
    _Out_ PNTSTATUS SubStatus,
    _Out_ PLSA_TOKEN_INFORMATION_TYPE TokenInformationType,
    _Out_ PVOID *TokenInformation,
    _Out_ PUNICODE_STRING *AccountName,
    _Out_ PUNICODE_STRING *AuthenticatingAuthority,
    _Out_ PUNICODE_STRING *MachineName,
    _Out_ PSECPKG_PRIMARY_CRED PrimaryCredentials,
    _Out_ PSECPKG_SUPPLEMENTAL_CRED_ARRAY *SupplementalCredentials);

/* EOF */
