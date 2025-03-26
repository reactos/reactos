/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         Local Security Authority (LSA) Server
 * FILE:            reactos/dll/win32/lsasrv/lsasrv.h
 * PURPOSE:         Common header file
 *
 * PROGRAMMERS:     Eric Kohl
 */

#ifndef _LSASRV_H
#define _LSASRV_H

#include <stdarg.h>

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#include <windef.h>
#include <winbase.h>
#include <winreg.h>

#define NTOS_MODE_USER
#include <ndk/cmfuncs.h>
#include <ndk/exfuncs.h>
#include <ndk/kefuncs.h>
#include <ndk/mmfuncs.h>
#include <ndk/obfuncs.h>
#include <ndk/psfuncs.h>
#include <ndk/rtlfuncs.h>
#include <ndk/sefuncs.h>
#include <ndk/ketypes.h>
#include <ndk/setypes.h>

#include <ntsam.h>
#include <ntlsa.h>
#include <sddl.h>

#include <srmp.h>

#include <lsass.h>
#include <lsa_s.h>

#include <wine/debug.h>
WINE_DEFAULT_DEBUG_CHANNEL(lsasrv);

typedef enum _LSA_DB_OBJECT_TYPE
{
    LsaDbIgnoreObject,
    LsaDbPolicyObject,
    LsaDbAccountObject,
    LsaDbDomainObject,
    LsaDbSecretObject
} LSA_DB_OBJECT_TYPE, *PLSA_DB_OBJECT_TYPE;

typedef struct _LSA_DB_OBJECT
{
    ULONG Signature;
    LSA_DB_OBJECT_TYPE ObjectType;
    ULONG RefCount;
    ACCESS_MASK Access;
    HANDLE KeyHandle;
    BOOLEAN Trusted;
    struct _LSA_DB_OBJECT *ParentObject;
} LSA_DB_OBJECT, *PLSA_DB_OBJECT;

#define LSAP_DB_SIGNATURE 0x12345678

#define POLICY_AUDIT_EVENT_TYPE_COUNT (AuditCategoryAccountLogon - AuditCategorySystem + 1)
typedef struct _LSAP_POLICY_AUDIT_EVENTS_DATA
{
    BOOLEAN AuditingMode;
    DWORD AuditEvents[POLICY_AUDIT_EVENT_TYPE_COUNT];
    DWORD MaximumAuditEventCount;
} LSAP_POLICY_AUDIT_EVENTS_DATA, *PLSAP_POLICY_AUDIT_EVENTS_DATA;

typedef struct _LSAP_LOGON_CONTEXT
{
    LIST_ENTRY Entry;
    HANDLE ClientProcessHandle;
    HANDLE ConnectionHandle;
    BOOL TrustedCaller;
} LSAP_LOGON_CONTEXT, *PLSAP_LOGON_CONTEXT;

typedef struct _SAMPR_ULONG_ARRAY
{
    unsigned long Count;
    unsigned long *Element;
} SAMPR_ULONG_ARRAY, *PSAMPR_ULONG_ARRAY;

extern NT_PRODUCT_TYPE LsapProductType;

extern SID_IDENTIFIER_AUTHORITY NullSidAuthority;
extern SID_IDENTIFIER_AUTHORITY WorldSidAuthority;
extern SID_IDENTIFIER_AUTHORITY LocalSidAuthority;
extern SID_IDENTIFIER_AUTHORITY CreatorSidAuthority;
extern SID_IDENTIFIER_AUTHORITY NtAuthority;

extern PSID BuiltinDomainSid;
extern UNICODE_STRING BuiltinDomainName;
extern PSID AccountDomainSid;
extern UNICODE_STRING AccountDomainName;

extern PSID LsapWorldSid;
extern PSID LsapNetworkSid;
extern PSID LsapBatchSid;
extern PSID LsapInteractiveSid;
extern PSID LsapServiceSid;
extern PSID LsapLocalSystemSid;
extern PSID LsapAdministratorsSid;


/* authpackage.c */
NTSTATUS
LsapInitAuthPackages(VOID);

NTSTATUS
LsapLookupAuthenticationPackage(PLSA_API_MSG RequestMsg,
                                PLSAP_LOGON_CONTEXT LogonContext);

NTSTATUS
LsapCallAuthenticationPackage(PLSA_API_MSG RequestMsg,
                              PLSAP_LOGON_CONTEXT LogonContext);

NTSTATUS
LsapLogonUser(PLSA_API_MSG RequestMsg,
              PLSAP_LOGON_CONTEXT LogonContext);

VOID
LsapTerminateLogon(
    _In_ PLUID LogonId);


/* authport.c */
NTSTATUS
StartAuthenticationPort(VOID);

/* database.c */
NTSTATUS
LsapInitDatabase(VOID);

NTSTATUS
LsapCreateDbObject(IN PLSA_DB_OBJECT ParentObject,
                   IN LPWSTR ContainerName,
                   IN LPWSTR ObjectName,
                   IN LSA_DB_OBJECT_TYPE HandleType,
                   IN ACCESS_MASK DesiredAccess,
                   IN BOOLEAN Trusted,
                   OUT PLSA_DB_OBJECT *DbObject);

NTSTATUS
LsapOpenDbObject(IN PLSA_DB_OBJECT ParentObject,
                 IN LPWSTR ContainerName,
                 IN LPWSTR ObjectName,
                 IN LSA_DB_OBJECT_TYPE ObjectType,
                 IN ACCESS_MASK DesiredAccess,
                 IN BOOLEAN Trusted,
                 OUT PLSA_DB_OBJECT *DbObject);

NTSTATUS
LsapValidateDbObject(IN LSAPR_HANDLE Handle,
                     IN LSA_DB_OBJECT_TYPE HandleType,
                     IN ACCESS_MASK GrantedAccess,
                     OUT PLSA_DB_OBJECT *DbObject);

NTSTATUS
LsapCloseDbObject(IN PLSA_DB_OBJECT DbObject);

NTSTATUS
LsapDeleteDbObject(IN PLSA_DB_OBJECT DbObject);

NTSTATUS
LsapGetObjectAttribute(PLSA_DB_OBJECT DbObject,
                       LPWSTR AttributeName,
                       LPVOID AttributeData,
                       PULONG AttributeSize);

NTSTATUS
LsapSetObjectAttribute(PLSA_DB_OBJECT DbObject,
                       LPWSTR AttributeName,
                       LPVOID AttributeData,
                       ULONG AttributeSize);

NTSTATUS
LsapDeleteObjectAttribute(PLSA_DB_OBJECT DbObject,
                          LPWSTR AttributeName);

/* dssetup.c */
VOID
DsSetupInit(VOID);

/* lookup.c */
NTSTATUS
LsapInitSids(VOID);

ULONG
LsapGetRelativeIdFromSid(PSID Sid);

NTSTATUS
LsapLookupNames(DWORD Count,
                PRPC_UNICODE_STRING Names,
                PLSAPR_REFERENCED_DOMAIN_LIST *ReferencedDomains,
                PLSAPR_TRANSLATED_SIDS_EX2 TranslatedSids,
                LSAP_LOOKUP_LEVEL LookupLevel,
                DWORD *MappedCount,
                DWORD LookupOptions,
                DWORD ClientRevision);

NTSTATUS
LsapLookupSids(PLSAPR_SID_ENUM_BUFFER SidEnumBuffer,
               PLSAPR_REFERENCED_DOMAIN_LIST *ReferencedDomains,
               PLSAPR_TRANSLATED_NAMES_EX TranslatedNames,
               LSAP_LOOKUP_LEVEL LookupLevel,
               DWORD *MappedCount,
               DWORD LookupOptions,
               DWORD ClientRevision);

/* lsarpc.c */
NTSTATUS
LsarStartRpcServer(VOID);

/* notify.c */
VOID
LsapInitNotificationList(VOID);

NTSTATUS
LsapRegisterNotification(
    PLSA_API_MSG RequestMsg);

VOID
LsapNotifyPolicyChange(
    POLICY_NOTIFICATION_INFORMATION_CLASS InformationClass);

/* policy.c */
NTSTATUS
LsarQueryAuditLog(PLSA_DB_OBJECT PolicyObject,
                  PLSAPR_POLICY_INFORMATION *PolicyInformation);

NTSTATUS
LsarQueryAuditEvents(PLSA_DB_OBJECT PolicyObject,
                     PLSAPR_POLICY_INFORMATION *PolicyInformation);

NTSTATUS
LsarQueryPrimaryDomain(PLSA_DB_OBJECT PolicyObject,
                       PLSAPR_POLICY_INFORMATION *PolicyInformation);

NTSTATUS
LsarQueryPdAccount(PLSA_DB_OBJECT PolicyObject,
                   PLSAPR_POLICY_INFORMATION *PolicyInformation);

NTSTATUS
LsarQueryAccountDomain(PLSA_DB_OBJECT PolicyObject,
                       PLSAPR_POLICY_INFORMATION *PolicyInformation);

NTSTATUS
LsarQueryServerRole(PLSA_DB_OBJECT PolicyObject,
                    PLSAPR_POLICY_INFORMATION *PolicyInformation);

NTSTATUS
LsarQueryReplicaSource(PLSA_DB_OBJECT PolicyObject,
                       PLSAPR_POLICY_INFORMATION *PolicyInformation);

NTSTATUS
LsarQueryDefaultQuota(PLSA_DB_OBJECT PolicyObject,
                      PLSAPR_POLICY_INFORMATION *PolicyInformation);

NTSTATUS
LsarQueryModification(PLSA_DB_OBJECT PolicyObject,
                      PLSAPR_POLICY_INFORMATION *PolicyInformation);

NTSTATUS
LsarQueryAuditFull(PLSA_DB_OBJECT PolicyObject,
                   PLSAPR_POLICY_INFORMATION *PolicyInformation);

NTSTATUS
LsarQueryDnsDomain(PLSA_DB_OBJECT PolicyObject,
                   PLSAPR_POLICY_INFORMATION *PolicyInformation);

NTSTATUS
LsarQueryDnsDomainInt(PLSA_DB_OBJECT PolicyObject,
                      PLSAPR_POLICY_INFORMATION *PolicyInformation);

NTSTATUS
LsarQueryLocalAccountDomain(PLSA_DB_OBJECT PolicyObject,
                            PLSAPR_POLICY_INFORMATION *PolicyInformation);

NTSTATUS
LsarSetAuditLog(PLSA_DB_OBJECT PolicyObject,
                PPOLICY_AUDIT_LOG_INFO Info);

NTSTATUS
LsarSetAuditEvents(PLSA_DB_OBJECT PolicyObject,
                   PLSAPR_POLICY_AUDIT_EVENTS_INFO Info);

NTSTATUS
LsarSetPrimaryDomain(PLSA_DB_OBJECT PolicyObject,
                     PLSAPR_POLICY_PRIMARY_DOM_INFO Info);

NTSTATUS
LsarSetAccountDomain(PLSA_DB_OBJECT PolicyObject,
                     PLSAPR_POLICY_ACCOUNT_DOM_INFO Info);

NTSTATUS
LsarSetServerRole(PLSA_DB_OBJECT PolicyObject,
                  PPOLICY_LSA_SERVER_ROLE_INFO Info);

NTSTATUS
LsarSetReplicaSource(PLSA_DB_OBJECT PolicyObject,
                     PPOLICY_LSA_REPLICA_SRCE_INFO Info);

NTSTATUS
LsarSetDefaultQuota(PLSA_DB_OBJECT PolicyObject,
                    PPOLICY_DEFAULT_QUOTA_INFO Info);

NTSTATUS
LsarSetModification(PLSA_DB_OBJECT PolicyObject,
                    PPOLICY_MODIFICATION_INFO Info);

NTSTATUS
LsarSetAuditFull(PLSA_DB_OBJECT PolicyObject,
                 PPOLICY_AUDIT_FULL_QUERY_INFO Info);

NTSTATUS
LsarSetDnsDomain(PLSA_DB_OBJECT PolicyObject,
                 PLSAPR_POLICY_DNS_DOMAIN_INFO Info);

NTSTATUS
LsarSetDnsDomainInt(PLSA_DB_OBJECT PolicyObject,
                    PLSAPR_POLICY_DNS_DOMAIN_INFO Info);

NTSTATUS
LsarSetLocalAccountDomain(PLSA_DB_OBJECT PolicyObject,
                          PLSAPR_POLICY_ACCOUNT_DOM_INFO Info);

/* privileges.c */
NTSTATUS
LsarpLookupPrivilegeName(PLUID Value,
                         PRPC_UNICODE_STRING *Name);

NTSTATUS
LsarpLookupPrivilegeDisplayName(PRPC_UNICODE_STRING Name,
                                USHORT ClientLanguage,
                                USHORT ClientSystemDefaultLanguage,
                                PRPC_UNICODE_STRING *DisplayName,
                                USHORT *LanguageReturned);

PLUID
LsarpLookupPrivilegeValue(
    IN PRPC_UNICODE_STRING Name);

NTSTATUS
LsarpEnumeratePrivileges(DWORD *EnumerationContext,
                         PLSAPR_PRIVILEGE_ENUM_BUFFER EnumerationBuffer,
                         DWORD PreferedMaximumLength);

NTSTATUS
LsapLookupAccountRightName(ULONG RightValue,
                           PRPC_UNICODE_STRING *Name);

ACCESS_MASK
LsapLookupAccountRightValue(
    IN PRPC_UNICODE_STRING Name);

/* registry.h */
NTSTATUS
LsapRegCloseKey(IN HANDLE KeyHandle);

NTSTATUS
LsapRegCreateKey(IN HANDLE ParentKeyHandle,
                 IN LPCWSTR KeyName,
                 IN ACCESS_MASK DesiredAccess,
                 OUT HANDLE KeyHandle);

NTSTATUS
LsapRegDeleteSubKey(IN HANDLE ParentKeyHandle,
                    IN LPCWSTR KeyName);

NTSTATUS
LsapRegDeleteKey(IN HANDLE KeyHandle);

NTSTATUS
LsapRegEnumerateSubKey(IN HANDLE KeyHandle,
                       IN ULONG Index,
                       IN ULONG Length,
                       OUT LPWSTR Buffer);

NTSTATUS
LsapRegOpenKey(IN HANDLE ParentKeyHandle,
               IN LPCWSTR KeyName,
               IN ACCESS_MASK DesiredAccess,
               OUT HANDLE KeyHandle);

NTSTATUS
LsapRegQueryKeyInfo(IN HANDLE KeyHandle,
                    OUT PULONG SubKeyCount,
                    OUT PULONG MaxSubKeyNameLength,
                    OUT PULONG ValueCount);

NTSTATUS
LsapRegDeleteValue(IN HANDLE KeyHandle,
                   IN LPWSTR ValueName);

NTSTATUS
LsapRegEnumerateValue(IN HANDLE KeyHandle,
                      IN ULONG Index,
                      OUT LPWSTR Name,
                      IN OUT PULONG NameLength,
                      OUT PULONG Type OPTIONAL,
                      OUT PVOID Data OPTIONAL,
                      IN OUT PULONG DataLength OPTIONAL);

NTSTATUS
LsapRegQueryValue(IN HANDLE KeyHandle,
                  IN LPWSTR ValueName,
                  OUT PULONG Type OPTIONAL,
                  OUT LPVOID Data OPTIONAL,
                  IN OUT PULONG DataLength OPTIONAL);

NTSTATUS
LsapRegSetValue(IN HANDLE KeyHandle,
                IN LPWSTR ValueName,
                IN ULONG Type,
                IN LPVOID Data,
                IN ULONG DataLength);

/* security.c */
NTSTATUS
LsapCreatePolicySd(PSECURITY_DESCRIPTOR *PolicySd,
                   PULONG PolicySdSize);

NTSTATUS
LsapCreateAccountSd(PSECURITY_DESCRIPTOR *AccountSd,
                    PULONG AccountSdSize);

NTSTATUS
LsapCreateSecretSd(PSECURITY_DESCRIPTOR *SecretSd,
                   PULONG SecretSdSize);

NTSTATUS
LsapCreateTokenSd(
    _In_ const TOKEN_USER *User,
    _Outptr_ PSECURITY_DESCRIPTOR *TokenSd,
    _Out_ PULONG TokenSdSize);

/* session.c */
VOID
LsapInitLogonSessions(VOID);

NTSTATUS
NTAPI
LsapCreateLogonSession(IN PLUID LogonId);

NTSTATUS
NTAPI
LsapDeleteLogonSession(IN PLUID LogonId);

NTSTATUS
NTAPI
LsapAddCredential(
    _In_ PLUID LogonId,
    _In_ ULONG AuthenticationPackage,
    _In_ PLSA_STRING PrimaryKeyValue,
    _In_ PLSA_STRING Credential);

NTSTATUS
NTAPI
LsapGetCredentials(
    _In_ PLUID LogonId,
    _In_ ULONG AuthenticationPackage,
    _Inout_ PULONG QueryContext,
    _In_ BOOLEAN RetrieveAllCredentials,
    _Inout_ PLSA_STRING PrimaryKeyValue,
    _Out_ PULONG PrimaryKeyLength,
    _Out_ PLSA_STRING Credentials);

NTSTATUS
NTAPI
LsapDeleteCredential(
    _In_ PLUID LogonId,
    _In_ ULONG AuthenticationPackage,
    _In_ PLSA_STRING PrimaryKeyValue);

NTSTATUS
LsapSetLogonSessionData(
    _In_ PLUID LogonId,
    _In_ ULONG LogonType,
    _In_ PUNICODE_STRING UserName,
    _In_ PUNICODE_STRING LogonDomain,
    _In_ PSID Sid);

NTSTATUS
LsapEnumLogonSessions(IN OUT PLSA_API_MSG RequestMsg);

NTSTATUS
LsapGetLogonSessionData(IN OUT PLSA_API_MSG RequestMsg);

/* srm.c */
NTSTATUS
LsapRmInitializeServer(VOID);

NTSTATUS
LsapRmCreateLogonSession(
    PLUID LogonId);

NTSTATUS
LsapRmDeleteLogonSession(
    PLUID LogonId);

/* utils.c */
INT
LsapLoadString(HINSTANCE hInstance,
               UINT uId,
               LPWSTR lpBuffer,
               INT nBufferMax);

INT
LsapGetResourceStringLengthEx(
    _In_ HINSTANCE hInstance,
    _In_ UINT uId,
    _In_ USHORT usLanguage);

INT
LsapLoadStringEx(
    _In_ HINSTANCE hInstance,
    _In_ UINT uId,
    _In_ USHORT usLanguage,
    _Out_ LPWSTR lpBuffer,
    _Out_ INT nBufferMax);

PSID
LsapAppendRidToSid(
    PSID SrcSid,
    ULONG Rid);

#endif /* _LSASRV_H */
