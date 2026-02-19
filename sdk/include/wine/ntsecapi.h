/*
 * Copyright (C) 1999 Juergen Schmied
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifndef _NTSECAPI_
#define _NTSECAPI_

#ifndef GUID_DEFINED
# include <guiddef.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif /* defined(__cplusplus) */

/* Policy access rights */
#define POLICY_VIEW_LOCAL_INFORMATION           __MSABI_LONG(0x00000001)
#define POLICY_VIEW_AUDIT_INFORMATION           __MSABI_LONG(0x00000002)
#define POLICY_GET_PRIVATE_INFORMATION          __MSABI_LONG(0x00000004)
#define POLICY_TRUST_ADMIN                      __MSABI_LONG(0x00000008)
#define POLICY_CREATE_ACCOUNT                   __MSABI_LONG(0x00000010)
#define POLICY_CREATE_SECRET                    __MSABI_LONG(0x00000020)
#define POLICY_CREATE_PRIVILEGE                 __MSABI_LONG(0x00000040)
#define POLICY_SET_DEFAULT_QUOTA_LIMITS         __MSABI_LONG(0x00000080)
#define POLICY_SET_AUDIT_REQUIREMENTS           __MSABI_LONG(0x00000100)
#define POLICY_AUDIT_LOG_ADMIN                  __MSABI_LONG(0x00000200)
#define POLICY_SERVER_ADMIN                     __MSABI_LONG(0x00000400)
#define POLICY_LOOKUP_NAMES                     __MSABI_LONG(0x00000800)
#define POLICY_NOTIFICATION                     __MSABI_LONG(0x00001000)

#define POLICY_ALL_ACCESS                       ( \
    STANDARD_RIGHTS_REQUIRED | \
    POLICY_VIEW_LOCAL_INFORMATION | \
    POLICY_VIEW_AUDIT_INFORMATION | \
    POLICY_GET_PRIVATE_INFORMATION | \
    POLICY_TRUST_ADMIN | \
    POLICY_CREATE_ACCOUNT | \
    POLICY_CREATE_SECRET | \
    POLICY_CREATE_PRIVILEGE | \
    POLICY_SET_DEFAULT_QUOTA_LIMITS | \
    POLICY_SET_AUDIT_REQUIREMENTS | \
    POLICY_AUDIT_LOG_ADMIN | \
    POLICY_SERVER_ADMIN | \
    POLICY_LOOKUP_NAMES)


#define POLICY_READ                             ( \
    STANDARD_RIGHTS_READ | \
    POLICY_VIEW_AUDIT_INFORMATION | \
    POLICY_GET_PRIVATE_INFORMATION)

#define POLICY_WRITE                            ( \
   STANDARD_RIGHTS_WRITE | \
   POLICY_TRUST_ADMIN | \
   POLICY_CREATE_ACCOUNT | \
   POLICY_CREATE_SECRET | \
   POLICY_CREATE_PRIVILEGE | \
   POLICY_SET_DEFAULT_QUOTA_LIMITS | \
   POLICY_SET_AUDIT_REQUIREMENTS | \
   POLICY_AUDIT_LOG_ADMIN | \
   POLICY_SERVER_ADMIN)

#define POLICY_EXECUTE                          ( \
   STANDARD_RIGHTS_EXECUTE | \
   POLICY_VIEW_LOCAL_INFORMATION | \
   POLICY_LOOKUP_NAMES)

#define POLICY_AUDIT_EVENT_UNCHANGED __MSABI_LONG(0x00000000)
#define POLICY_AUDIT_EVENT_SUCCESS   __MSABI_LONG(0x00000001)
#define POLICY_AUDIT_EVENT_FAILURE   __MSABI_LONG(0x00000002)
#define POLICY_AUDIT_EVENT_NONE      __MSABI_LONG(0x00000004)

#define POLICY_AUDIT_EVENT_MASK (POLICY_AUDIT_EVENT_SUCCESS | \
                                 POLICY_AUDIT_EVENT_FAILURE | \
                                 POLICY_AUDIT_EVENT_NONE)

/* logon rights names */
#define SE_BATCH_LOGON_NAME \
 TEXT("SeBatchLogonRight")
#define SE_INTERACTIVE_LOGON_NAME \
 TEXT("SeInteractiveLogonRight")
#define SE_NETWORK_LOGON_NAME \
 TEXT("SeNetworkLogonRight")
#define SE_REMOTE_INTERACTIVE_LOGON_NAME \
 TEXT("SeRemoteInteractiveLogonRight")
#define SE_SERVICE_LOGON_NAME \
 TEXT("SeServiceLogonRight")
#define SE_DENY_BATCH_LOGON_NAME \
 TEXT("SeDenyBatchLogonRight")
#define SE_DENY_INTERACTIVE_LOGON_NAME \
 TEXT("SeDenyInteractiveLogonRight")
#define SE_DENY_NETWORK_LOGON_NAME \
 TEXT("SeDenyNetworkLogonRight")
#define SE_DENY_REMOTE_INTERACTIVE_LOGON_NAME \
 TEXT("SeDenyRemoteInteractiveLogonRight")
#define SE_DENY_SERVICE_LOGON_NAME \
 TEXT("SeDenyServiceLogonRight")

#ifndef WINE_NTSTATUS_DECLARED
#define WINE_NTSTATUS_DECLARED
typedef LONG NTSTATUS;
#endif
#ifndef WINE_PNTSTATUS_DECLARED
#define WINE_PNTSTATUS_DECLARED
typedef NTSTATUS *PNTSTATUS;
#endif

typedef enum _SECURITY_LOGON_TYPE
{
    UndefinedLogonType = 0,
    Interactive = 2,
    Network,
    Batch,
    Service,
    Proxy,
    Unlock,
    NetworkCleartext,
    NewCredentials,
    RemoteInteractive,
    CachedInteractive,
    CachedRemoteInteractive,
    CachedUnlock
} SECURITY_LOGON_TYPE, *PSECURITY_LOGON_TYPE;

typedef enum _POLICY_AUDIT_EVENT_TYPE
{
    AuditCategorySystem,
    AuditCategoryLogon,
    AuditCategoryObjectAccess,
    AuditCategoryPrivilegeUse,
    AuditCategoryDetailedTracking,
    AuditCategoryPolicyChange,
    AuditCategoryAccountManagement
} POLICY_AUDIT_EVENT_TYPE, *PPOLICY_AUDIT_EVENT_TYPE;

#ifndef __STRING_DEFINED__
#define __STRING_DEFINED__
typedef struct _STRING {
  USHORT Length;
  USHORT MaximumLength;
  PCHAR Buffer;
} STRING, *PSTRING;
#endif

#ifndef __UNICODE_STRING_DEFINED__
#define __UNICODE_STRING_DEFINED__
typedef struct _UNICODE_STRING {
  USHORT Length;        /* bytes */
  USHORT MaximumLength; /* bytes */
  PWSTR  Buffer;
} UNICODE_STRING, *PUNICODE_STRING;
#endif

#ifndef __OBJECT_ATTRIBUTES_DEFINED__
#define __OBJECT_ATTRIBUTES_DEFINED__
typedef struct _OBJECT_ATTRIBUTES {
  ULONG Length;
  HANDLE RootDirectory;
  PUNICODE_STRING ObjectName;
  ULONG Attributes;
  PVOID SecurityDescriptor;       /* type SECURITY_DESCRIPTOR */
  PVOID SecurityQualityOfService; /* type SECURITY_QUALITY_OF_SERVICE */
} OBJECT_ATTRIBUTES, *POBJECT_ATTRIBUTES;
#endif

#ifndef __SECHANDLE_DEFINED__
#define __SECHANDLE_DEFINED__
typedef struct _SecHandle
{
    ULONG_PTR dwLower;
    ULONG_PTR dwUpper;
} SecHandle, *PSecHandle;
#endif

typedef UNICODE_STRING LSA_UNICODE_STRING, *PLSA_UNICODE_STRING;
typedef STRING LSA_STRING, *PLSA_STRING;

#ifdef _NTDEF_
typedef OBJECT_ATTRIBUTES LSA_OBJECT_ATTRIBUTES, *PLSA_OBJECT_ATTRIBUTES;
#else
typedef struct _LSA_OBJECT_ATTRIBUTES
{
    ULONG Length;
    HANDLE RootDirectory;
    PLSA_UNICODE_STRING ObjectName;
    ULONG Attributes;
    PVOID SecurityDescriptor;
    PVOID SecurityQualityOfService;
} LSA_OBJECT_ATTRIBUTES, *PLSA_OBJECT_ATTRIBUTES;
#endif


typedef PVOID LSA_HANDLE, *PLSA_HANDLE;
typedef ULONG LSA_ENUMERATION_HANDLE, *PLSA_ENUMERATION_HANDLE;
typedef ULONG LSA_OPERATIONAL_MODE, *PLSA_OPERATIONAL_MODE;

typedef enum
{
	PolicyAuditLogInformation = 1,
	PolicyAuditEventsInformation,
	PolicyPrimaryDomainInformation,
	PolicyPdAccountInformation,
	PolicyAccountDomainInformation,
	PolicyLsaServerRoleInformation,
	PolicyReplicaSourceInformation,
	PolicyDefaultQuotaInformation,
	PolicyModificationInformation,
	PolicyAuditFullSetInformation,
	PolicyAuditFullQueryInformation,
	PolicyDnsDomainInformation
} POLICY_INFORMATION_CLASS, *PPOLICY_INFORMATION_CLASS;

typedef ULONG POLICY_AUDIT_EVENT_OPTIONS, *PPOLICY_AUDIT_EVENT_OPTIONS;

typedef struct _POLICY_AUDIT_EVENTS_INFO
{
	BOOLEAN AuditingMode;
	PPOLICY_AUDIT_EVENT_OPTIONS EventAuditingOptions;
	ULONG MaximumAuditEventCount;
} POLICY_AUDIT_EVENTS_INFO, *PPOLICY_AUDIT_EVENTS_INFO;

typedef struct _POLICY_PRIMARY_DOMAIN_INFO
{
    LSA_UNICODE_STRING Name;
    PSID Sid;
} POLICY_PRIMARY_DOMAIN_INFO, *PPOLICY_PRIMARY_DOMAIN_INFO;

typedef struct _POLICY_ACCOUNT_DOMAIN_INFO
{
    LSA_UNICODE_STRING DomainName;
    PSID DomainSid;
} POLICY_ACCOUNT_DOMAIN_INFO, *PPOLICY_ACCOUNT_DOMAIN_INFO;

typedef struct _POLICY_DNS_DOMAIN_INFO
{
    LSA_UNICODE_STRING Name;
    LSA_UNICODE_STRING DnsDomainName;
    LSA_UNICODE_STRING DnsForestName;
    GUID DomainGuid;
    PSID Sid;
} POLICY_DNS_DOMAIN_INFO, *PPOLICY_DNS_DOMAIN_INFO;

typedef enum _POLICY_LSA_SERVER_ROLE
{
    PolicyServerRoleBackup = 2,
    PolicyServerRolePrimary
} POLICY_LSA_SERVER_ROLE, *PPOLICY_LSA_SERVER_ROLE;

typedef struct _POLICY_LSA_SERVER_ROLE_INFO
{
    POLICY_LSA_SERVER_ROLE LsaServerRole;
} POLICY_LSA_SERVER_ROLE_INFO, *PPOLICY_LSA_SERVER_ROLE_INFO;

typedef struct _POLICY_MODIFICATION_INFO
{
    LARGE_INTEGER ModifiedId;
    LARGE_INTEGER DatabaseCreationTime;
} POLICY_MODIFICATION_INFO, *PPOLICY_MODIFICATION_INFO;

typedef struct _LSA_LAST_INTER_LOGON_INFO {
    LARGE_INTEGER LastSuccessfulLogon;
    LARGE_INTEGER LastFailedLogon;
    ULONG FailedAttemptCountSinceLastSuccessfulLogon;
} LSA_LAST_INTER_LOGON_INFO, *PLSA_LAST_INTER_LOGON_INFO;

typedef struct _SECURITY_LOGON_SESSION_DATA {
    ULONG Size;
    LUID LogonId;
    LSA_UNICODE_STRING UserName;
    LSA_UNICODE_STRING LogonDomain;
    LSA_UNICODE_STRING AuthenticationPackage;
    ULONG LogonType;
    ULONG Session;
    PSID Sid;
    LARGE_INTEGER LogonTime;
    LSA_UNICODE_STRING LogonServer;
    LSA_UNICODE_STRING DnsDomainName;
    LSA_UNICODE_STRING Upn;
    ULONG UserFlags;
    LSA_LAST_INTER_LOGON_INFO LastLogonInfo;
    LSA_UNICODE_STRING LogonScript;
    LSA_UNICODE_STRING ProfilePath;
    LSA_UNICODE_STRING HomeDirectory;
    LSA_UNICODE_STRING HomeDirectoryDrive;
    LARGE_INTEGER LogoffTime;
    LARGE_INTEGER KickOffTime;
    LARGE_INTEGER PasswordLastSet;
    LARGE_INTEGER PasswordCanChange;
    LARGE_INTEGER PasswordMustChange;
} SECURITY_LOGON_SESSION_DATA, *PSECURITY_LOGON_SESSION_DATA;

typedef struct
{
    SID_NAME_USE Use;
    LSA_UNICODE_STRING Name;
    LONG DomainIndex;
} LSA_TRANSLATED_NAME, *PLSA_TRANSLATED_NAME;

typedef struct
{
    LSA_UNICODE_STRING Name;
    PSID Sid;
} LSA_TRUST_INFORMATION, *PLSA_TRUST_INFORMATION;

typedef struct
{
    ULONG Entries;
    PLSA_TRUST_INFORMATION Domains;
} LSA_REFERENCED_DOMAIN_LIST, *PLSA_REFERENCED_DOMAIN_LIST;

typedef struct _LSA_TRANSLATED_SID
{
    SID_NAME_USE Use;
    ULONG RelativeId;
    LONG DomainIndex;
} LSA_TRANSLATED_SID, *PLSA_TRANSLATED_SID;

typedef struct _TRUSTED_DOMAIN_INFORMATION_EX
{
    LSA_UNICODE_STRING Name;
    LSA_UNICODE_STRING FlatName;
    PSID Sid;
    ULONG TrustDirection;
    ULONG TrustType;
    ULONG TrustAttributes;
} TRUSTED_DOMAIN_INFORMATION_EX, *PTRUSTED_DOMAIN_INFORMATION_EX;

typedef struct _LSA_AUTH_INFORMATION
{
    LARGE_INTEGER LastUpdateTime;
    ULONG AuthType;
    ULONG AuthInfoLength;
    PUCHAR AuthInfo;
} LSA_AUTH_INFORMATION, *PLSA_AUTH_INFORMATION;

typedef struct _TRUSTED_DOMAIN_AUTH_INFORMATION
{
    ULONG IncomingAuthInfos;
    PLSA_AUTH_INFORMATION IncomingAuthenticationInformation;
    PLSA_AUTH_INFORMATION IncomingPreviousAuthenticationInformation;
    ULONG OutgoingAuthInfos;
    PLSA_AUTH_INFORMATION OutgoingAuthenticationInformation;
    PLSA_AUTH_INFORMATION OutgoingPreviousAuthenticationInformation;
} TRUSTED_DOMAIN_AUTH_INFORMATION, *PTRUSTED_DOMAIN_AUTH_INFORMATION;

typedef struct _LSA_TRANSLATED_SID2
{
    SID_NAME_USE Use;
    PSID Sid;
    LONG DomainIndex;
    ULONG Flags;
} LSA_TRANSLATED_SID2, *PLSA_TRANSLATED_SID2;

typedef enum _TRUSTED_INFORMATION_CLASS
{
    TrustedDomainNameInformation = 1,
    TrustedControllersInformation,
    TrustedPosixOffsetInformation,
    TrustedPasswordInformation,
    TrustedDomainInformationBasic,
    TrustedDomainInformationEx,
    TrustedDomainAuthInformation,
    TrustedDomainFullInformation
} TRUSTED_INFORMATION_CLASS, *PTRUSTED_INFORMATION_CLASS;

typedef enum _POLICY_NOTIFICATION_INFORMATION_CLASS
{
    PolicyNotifyAuditEventsInformation = 1,
    PolicyNotifyAccountDomainInformation,
    PolicyNotifyServerRoleInformation,
    PolicyNotifyDnsDomainInformation,
    PolicyNotifyDomainEfsInformation,
    PolicyNotifyDomainKerberosTicketInformation,
    PolicyNotifyMachineAccountPasswordInformation
} POLICY_NOTIFICATION_INFORMATION_CLASS, *PPOLICY_NOTIFICATION_INFORMATION_CLASS;

typedef struct _AUDIT_POLICY_INFORMATION
{
    GUID    AuditSubCategoryGuid;
    ULONG   AuditingInformation;
    GUID    AuditCategoryGuid;
} AUDIT_POLICY_INFORMATION, *PAUDIT_POLICY_INFORMATION;

enum NEGOTIATE_MESSAGES
{
    NegEnumPackagePrefixes,
    NegGetCallerName,
    NegTransferCredentials,
    NegMsgReserved1,
    NegCallPackageMax
};

typedef struct _NEGOTIATE_CALLER_NAME_REQUEST
{
    ULONG MessageType;
    LUID LogonId;
} NEGOTIATE_CALLER_NAME_REQUEST, *PNEGOTIATE_CALLER_NAME_REQUEST;

typedef struct _NEGOTIATE_CALLER_NAME_RESPONSE
{
    ULONG MessageType;
    PWSTR CallerName;
} NEGOTIATE_CALLER_NAME_RESPONSE, *PNEGOTIATE_CALLER_NAME_RESPONSE;

#define MICROSOFT_KERBEROS_NAME_A "Kerberos"
#if defined(_MSC_VER) || defined(__MINGW32__)
#define MICROSOFT_KERBEROS_NAME_W L"Kerberos"
#else /* _MSC_VER/__MINGW32__ */
static const WCHAR MICROSOFT_KERBEROS_NAME_W[] = { 'K','e','r','b','e','r','o','s',0 };
#endif

#define KERB_TICKET_FLAGS_reserved          0x80000000
#define KERB_TICKET_FLAGS_forwardable       0x40000000
#define KERB_TICKET_FLAGS_forwarded         0x20000000
#define KERB_TICKET_FLAGS_proxiable         0x10000000
#define KERB_TICKET_FLAGS_proxy             0x08000000
#define KERB_TICKET_FLAGS_may_postdate      0x04000000
#define KERB_TICKET_FLAGS_postdated         0x02000000
#define KERB_TICKET_FLAGS_invalid           0x01000000
#define KERB_TICKET_FLAGS_renewable         0x00800000
#define KERB_TICKET_FLAGS_initial           0x00400000
#define KERB_TICKET_FLAGS_pre_authent       0x00200000
#define KERB_TICKET_FLAGS_hw_authent        0x00100000
#define KERB_TICKET_FLAGS_ok_as_delegate    0x00040000
#define KERB_TICKET_FLAGS_name_canonicalize 0x00010000
#define KERB_TICKET_FLAGS_cname_in_pa_data  0x00040000
#define KERB_TICKET_FLAGS_reserved1         0x00000001

typedef enum _KERB_PROTOCOL_MESSAGE_TYPE
{
    KerbDebugRequestMessage = 0,
    KerbQueryTicketCacheMessage,
    KerbChangeMachinePasswordMessage,
    KerbVerifyPacMessage,
    KerbRetrieveTicketMessage,
    KerbUpdateAddressesMessage,
    KerbPurgeTicketCacheMessage,
    KerbChangePasswordMessage,
    KerbRetrieveEncodedTicketMessage,
    KerbDecryptDataMessage,
    KerbAddBindingCacheEntryMessage,
    KerbSetPasswordMessage,
    KerbSetPasswordExMessage,
    KerbVerifyCredentialsMessage,
    KerbQueryTicketCacheExMessage,
    KerbPurgeTicketCacheExMessage,
    KerbRefreshSmartcardCredentialsMessage,
    KerbAddExtraCredentialsMessage,
    KerbQuerySupplementalCredentialsMessage,
    KerbTransferCredentialsMessage,
    KerbQueryTicketCacheEx2Message,
    KerbSubmitTicketMessage,
    KerbAddExtraCredentialsExMessage,
    KerbQueryKdcProxyCacheMessage,
    KerbPurgeKdcProxyCacheMessage,
    KerbQueryTicketCacheEx3Message,
    KerbCleanupMachinePkinitCredsMessage,
    KerbAddBindingCacheEntryExMessage,
    KerbQueryBindingCacheMessage,
    KerbPurgeBindingCacheMessage,
    KerbQueryDomainExtendedPoliciesMessage,
    KerbQueryS4U2ProxyCacheMessage
} KERB_PROTOCOL_MESSAGE_TYPE, *PKERB_PROTOCOL_MESSAGE_TYPE;

typedef struct _KERB_TICKET_CACHE_INFO
{
    UNICODE_STRING ServerName;
    UNICODE_STRING RealmName;
    LARGE_INTEGER StartTime;
    LARGE_INTEGER EndTime;
    LARGE_INTEGER RenewTime;
    LONG EncryptionType;
    ULONG TicketFlags;
} KERB_TICKET_CACHE_INFO, *PKERB_TICKET_CACHE_INFO;

typedef struct _KERB_TICKET_CACHE_INFO_EX
{
    UNICODE_STRING ClientName;
    UNICODE_STRING ClientRealm;

    UNICODE_STRING ServerName;
    UNICODE_STRING ServerRealm;
    LARGE_INTEGER StartTime;
    LARGE_INTEGER EndTime;
    LARGE_INTEGER RenewTime;
    LONG EncryptionType;
    ULONG TicketFlags;
} KERB_TICKET_CACHE_INFO_EX, *PKERB_TICKET_CACHE_INFO_EX;

typedef struct _KERB_TICKET_CACHE_INFO_EX2
{
    UNICODE_STRING ClientName;
    UNICODE_STRING ClientRealm;
    UNICODE_STRING ServerName;
    UNICODE_STRING ServerRealm;
    LARGE_INTEGER StartTime;
    LARGE_INTEGER EndTime;
    LARGE_INTEGER RenewTime;
    LONG EncryptionType;
    ULONG TicketFlags;

    ULONG SessionKeyType;
    ULONG BranchId;
} KERB_TICKET_CACHE_INFO_EX2, *PKERB_TICKET_CACHE_INFO_EX2;

typedef struct _KERB_TICKET_CACHE_INFO_EX3
{
    UNICODE_STRING ClientName;
    UNICODE_STRING ClientRealm;
    UNICODE_STRING ServerName;
    UNICODE_STRING ServerRealm;
    LARGE_INTEGER StartTime;
    LARGE_INTEGER EndTime;
    LARGE_INTEGER RenewTime;
    LONG EncryptionType;
    ULONG TicketFlags;
    ULONG SessionKeyType;
    ULONG BranchId;

    ULONG CacheFlags;
    UNICODE_STRING KdcCalled;
} KERB_TICKET_CACHE_INFO_EX3, *PKERB_TICKET_CACHE_INFO_EX3;

typedef struct _KERB_CRYPTO_KEY
{
    LONG KeyType;
    ULONG Length;
    PUCHAR Value;
} KERB_CRYPTO_KEY, *PKERB_CRYPTO_KEY;

typedef struct _KERB_EXTERNAL_NAME
{
    SHORT          NameType;
    USHORT         NameCount;
    UNICODE_STRING Names[ANYSIZE_ARRAY];
} KERB_EXTERNAL_NAME, *PKERB_EXTERNAL_NAME;

typedef struct _KERB_EXTERNAL_TICKET
{
    PKERB_EXTERNAL_NAME ServiceName;
    PKERB_EXTERNAL_NAME TargetName;
    PKERB_EXTERNAL_NAME ClientName;
    UNICODE_STRING DomainName;
    UNICODE_STRING TargetDomainName;
    UNICODE_STRING AltTargetDomainName;
    KERB_CRYPTO_KEY SessionKey;
    ULONG TicketFlags;
    ULONG Flags;
    LARGE_INTEGER KeyExpirationTime;
    LARGE_INTEGER StartTime;
    LARGE_INTEGER EndTime;
    LARGE_INTEGER RenewUntil;
    LARGE_INTEGER TimeSkew;
    ULONG EncodedTicketSize;
    PUCHAR EncodedTicket;
} KERB_EXTERNAL_TICKET, *PKERB_EXTERNAL_TICKET;

typedef struct _KERB_QUERY_TKT_CACHE_REQUEST
{
    KERB_PROTOCOL_MESSAGE_TYPE MessageType;
    LUID LogonId;
} KERB_QUERY_TKT_CACHE_REQUEST, *PKERB_QUERY_TKT_CACHE_REQUEST;

typedef struct _KERB_QUERY_TKT_CACHE_RESPONSE
{
    KERB_PROTOCOL_MESSAGE_TYPE MessageType;
    ULONG CountOfTickets;
    KERB_TICKET_CACHE_INFO Tickets[ANYSIZE_ARRAY];
} KERB_QUERY_TKT_CACHE_RESPONSE, *PKERB_QUERY_TKT_CACHE_RESPONSE;

typedef struct _KERB_QUERY_TKT_CACHE_EX_RESPONSE
{
    KERB_PROTOCOL_MESSAGE_TYPE MessageType;
    ULONG CountOfTickets;
    KERB_TICKET_CACHE_INFO_EX Tickets[ANYSIZE_ARRAY];
} KERB_QUERY_TKT_CACHE_EX_RESPONSE, *PKERB_QUERY_TKT_CACHE_EX_RESPONSE;

typedef struct _KERB_QUERY_TKT_CACHE_EX2_RESPONSE
{
    KERB_PROTOCOL_MESSAGE_TYPE MessageType;
    ULONG CountOfTickets;
    KERB_TICKET_CACHE_INFO_EX2 Tickets[ANYSIZE_ARRAY];
} KERB_QUERY_TKT_CACHE_EX2_RESPONSE, *PKERB_QUERY_TKT_CACHE_EX2_RESPONSE;

typedef struct _KERB_QUERY_TKT_CACHE_EX3_RESPONSE
{
    KERB_PROTOCOL_MESSAGE_TYPE MessageType;
    ULONG CountOfTickets;
    KERB_TICKET_CACHE_INFO_EX3 Tickets[ANYSIZE_ARRAY];
} KERB_QUERY_TKT_CACHE_EX3_RESPONSE, *PKERB_QUERY_TKT_CACHE_EX3_RESPONSE;

typedef struct _KERB_RETRIEVE_TKT_REQUEST
{
    KERB_PROTOCOL_MESSAGE_TYPE MessageType;
    LUID LogonId;
    UNICODE_STRING TargetName;
    ULONG TicketFlags;
    ULONG CacheOptions;
    LONG EncryptionType;
    SecHandle CredentialsHandle;
} KERB_RETRIEVE_TKT_REQUEST, *PKERB_RETRIEVE_TKT_REQUEST;

typedef struct _KERB_RETRIEVE_TKT_RESPONSE
{
    KERB_EXTERNAL_TICKET Ticket;
} KERB_RETRIEVE_TKT_RESPONSE,*PKERB_RETRIEVE_TKT_RESPONSE;

typedef struct _KERB_PURGE_TKT_CACHE_REQUEST
{
    KERB_PROTOCOL_MESSAGE_TYPE MessageType;
    LUID LogonId;
    UNICODE_STRING ServerName;
    UNICODE_STRING RealmName;
} KERB_PURGE_TKT_CACHE_REQUEST, *PKERB_PURGE_TKT_CACHE_REQUEST;

#define KERB_ETYPE_NULL 0
#define KERB_ETYPE_DES_CBC_CRC 1
#define KERB_ETYPE_DES_CBC_MD4 2
#define KERB_ETYPE_DES_CBC_MD5 3
#define KERB_ETYPE_AES128_CTS_HMAC_SHA1_96 17
#define KERB_ETYPE_AES256_CTS_HMAC_SHA1_96 18

#define KERB_ETYPE_RC4_MD4 -128
#define KERB_ETYPE_RC4_PLAIN2 -129
#define KERB_ETYPE_RC4_LM -130
#define KERB_ETYPE_RC4_SHA -131
#define KERB_ETYPE_DES_PLAIN -132
#define KERB_ETYPE_RC4_HMAC_OLD -133
#define KERB_ETYPE_RC4_PLAIN_OLD -134
#define KERB_ETYPE_RC4_HMAC_OLD_EXP -135
#define KERB_ETYPE_RC4_PLAIN_OLD_EXP -136
#define KERB_ETYPE_RC4_PLAIN -140
#define KERB_ETYPE_RC4_PLAIN_EXP -141
#define KERB_ETYPE_AES128_CTS_HMAC_SHA1_96_PLAIN -148
#define KERB_ETYPE_AES256_CTS_HMAC_SHA1_96_PLAIN -149

#define KERB_ETYPE_DSA_SHA1_CMS 9
#define KERB_ETYPE_RSA_MD5_CMS 10
#define KERB_ETYPE_RSA_SHA1_CMS 11
#define KERB_ETYPE_RC2_CBC_ENV 12
#define KERB_ETYPE_RSA_ENV 13
#define KERB_ETYPE_RSA_ES_OEAP_ENV 14
#define KERB_ETYPE_DES_EDE3_CBC_ENV 15

#define KERB_ETYPE_DSA_SIGN 8
#define KERB_ETYPE_RSA_PRIV 9
#define KERB_ETYPE_RSA_PUB 10
#define KERB_ETYPE_RSA_PUB_MD5 11
#define KERB_ETYPE_RSA_PUB_SHA1 12
#define KERB_ETYPE_PKCS7_PUB 13

#define KERB_ETYPE_DES3_CBC_MD5 5
#define KERB_ETYPE_DES3_CBC_SHA1 7
#define KERB_ETYPE_DES3_CBC_SHA1_KD 16

#define KERB_ETYPE_DES_CBC_MD5_NT 20
#define KERB_ETYPE_RC4_HMAC_NT 23
#define KERB_ETYPE_RC4_HMAC_NT_EXP 24

#define RtlGenRandom                    SystemFunction036
#define RtlEncryptMemory                SystemFunction040
#define RtlDecryptMemory                SystemFunction041

#define LSA_SUCCESS(Error) ((LONG)(Error) >= 0)

WINADVAPI BOOLEAN  WINAPI AuditQuerySystemPolicy(const GUID*,ULONG,AUDIT_POLICY_INFORMATION**);
WINADVAPI BOOLEAN  WINAPI RtlGenRandom(PVOID,ULONG);
WINADVAPI NTSTATUS WINAPI RtlEncryptMemory(PVOID,ULONG,ULONG);
WINADVAPI NTSTATUS WINAPI RtlDecryptMemory(PVOID,ULONG,ULONG);

WINADVAPI NTSTATUS WINAPI LsaAddAccountRights(LSA_HANDLE,PSID,PLSA_UNICODE_STRING,ULONG);
WINADVAPI NTSTATUS WINAPI LsaClose(LSA_HANDLE);
WINADVAPI NTSTATUS WINAPI LsaCreateTrustedDomainEx(LSA_HANDLE,PTRUSTED_DOMAIN_INFORMATION_EX,
                                                   PTRUSTED_DOMAIN_AUTH_INFORMATION,ACCESS_MASK,PLSA_HANDLE);
WINADVAPI NTSTATUS WINAPI LsaDeleteTrustedDomain(LSA_HANDLE,PSID);
WINADVAPI NTSTATUS WINAPI LsaEnumerateAccountRights(LSA_HANDLE,PSID,PLSA_UNICODE_STRING*,PULONG);
WINADVAPI NTSTATUS WINAPI LsaEnumerateAccountsWithUserRight(LSA_HANDLE,PLSA_UNICODE_STRING,PVOID*,PULONG);
WINADVAPI NTSTATUS WINAPI LsaEnumerateTrustedDomains(LSA_HANDLE,PLSA_ENUMERATION_HANDLE,PVOID*,ULONG,PULONG);
WINADVAPI NTSTATUS WINAPI LsaEnumerateTrustedDomainsEx(LSA_HANDLE,PLSA_ENUMERATION_HANDLE,PVOID*,ULONG,PULONG);
WINADVAPI NTSTATUS WINAPI LsaFreeMemory(PVOID);
WINADVAPI NTSTATUS WINAPI LsaLookupNames(LSA_HANDLE,ULONG,PLSA_UNICODE_STRING,PLSA_REFERENCED_DOMAIN_LIST*,
                                         PLSA_TRANSLATED_SID*);
WINADVAPI NTSTATUS WINAPI LsaLookupNames2(LSA_HANDLE,ULONG,ULONG,PLSA_UNICODE_STRING,PLSA_REFERENCED_DOMAIN_LIST*,
                                          PLSA_TRANSLATED_SID2*);
WINADVAPI NTSTATUS WINAPI LsaLookupSids(LSA_HANDLE,ULONG,PSID *,PLSA_REFERENCED_DOMAIN_LIST *,PLSA_TRANSLATED_NAME *);
WINADVAPI ULONG    WINAPI LsaNtStatusToWinError(NTSTATUS);
WINADVAPI NTSTATUS WINAPI LsaOpenPolicy(PLSA_UNICODE_STRING,PLSA_OBJECT_ATTRIBUTES,ACCESS_MASK,PLSA_HANDLE);
WINADVAPI NTSTATUS WINAPI LsaOpenTrustedDomainByName(LSA_HANDLE,PLSA_UNICODE_STRING,ACCESS_MASK,PLSA_HANDLE);
WINADVAPI NTSTATUS WINAPI LsaQueryInformationPolicy(LSA_HANDLE,POLICY_INFORMATION_CLASS,PVOID*);
WINADVAPI NTSTATUS WINAPI LsaQueryTrustedDomainInfo(LSA_HANDLE,PSID,TRUSTED_INFORMATION_CLASS,PVOID*);
WINADVAPI NTSTATUS WINAPI LsaQueryTrustedDomainInfoByName(LSA_HANDLE,PLSA_UNICODE_STRING,TRUSTED_INFORMATION_CLASS,PVOID*);
WINADVAPI NTSTATUS WINAPI LsaRegisterPolicyChangeNotification(POLICY_NOTIFICATION_INFORMATION_CLASS,HANDLE);
WINADVAPI NTSTATUS WINAPI LsaRemoveAccountRights(LSA_HANDLE,PSID,BOOLEAN,PLSA_UNICODE_STRING,ULONG);
WINADVAPI NTSTATUS WINAPI LsaRetrievePrivateData(LSA_HANDLE,PLSA_UNICODE_STRING,PLSA_UNICODE_STRING*);
WINADVAPI NTSTATUS WINAPI LsaSetInformationPolicy(LSA_HANDLE,POLICY_INFORMATION_CLASS,PVOID);
WINADVAPI NTSTATUS WINAPI LsaSetTrustedDomainInfoByName(LSA_HANDLE,PLSA_UNICODE_STRING,TRUSTED_INFORMATION_CLASS,PVOID);
WINADVAPI NTSTATUS WINAPI LsaSetTrustedDomainInformation(LSA_HANDLE,PSID,TRUSTED_INFORMATION_CLASS,PVOID);
WINADVAPI NTSTATUS WINAPI LsaStorePrivateData(LSA_HANDLE,PLSA_UNICODE_STRING,PLSA_UNICODE_STRING);
WINADVAPI NTSTATUS WINAPI LsaUnregisterPolicyChangeNotification(POLICY_NOTIFICATION_INFORMATION_CLASS,HANDLE);

NTSTATUS WINAPI LsaCallAuthenticationPackage(HANDLE,ULONG,PVOID,ULONG,PVOID*,PULONG,PNTSTATUS);
NTSTATUS WINAPI LsaConnectUntrusted(PHANDLE);
NTSTATUS WINAPI LsaDeregisterLogonProcess(HANDLE);
NTSTATUS WINAPI LsaEnumerateLogonSessions(PULONG,PLUID*);
NTSTATUS WINAPI LsaFreeReturnBuffer(PVOID);
NTSTATUS WINAPI LsaGetLogonSessionData(PLUID,PSECURITY_LOGON_SESSION_DATA*);
NTSTATUS WINAPI LsaLogonUser(HANDLE,PLSA_STRING,SECURITY_LOGON_TYPE,ULONG,PVOID,ULONG,PTOKEN_GROUPS,PTOKEN_SOURCE,PVOID*,PULONG,PLUID,PHANDLE,PQUOTA_LIMITS,PNTSTATUS);
NTSTATUS WINAPI LsaLookupAuthenticationPackage(HANDLE,PLSA_STRING,PULONG);
NTSTATUS WINAPI LsaRegisterLogonProcess(PLSA_STRING,PHANDLE,PLSA_OPERATIONAL_MODE);

#ifdef __cplusplus
} /* extern "C" */
#endif /* defined(__cplusplus) */

#endif /* !defined(_NTSECAPI_) */
