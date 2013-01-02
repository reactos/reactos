/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         Local Security Authority (LSA) Server
 * FILE:            reactos/dll/win32/lsasrv/lsasrv.h
 * PURPOSE:         Common header file
 *
 * PROGRAMMERS:     Eric Kohl
 */

#define WIN32_NO_STATUS
#include <windows.h>
#define NTOS_MODE_USER
#include <ndk/cmfuncs.h>
#include <ndk/kefuncs.h>
#include <ndk/lpctypes.h>
#include <ndk/lpcfuncs.h>
#include <ndk/obfuncs.h>
#include <ndk/rtlfuncs.h>
#include <ndk/setypes.h>

#include <ntsam.h>
#include <ntlsa.h>
#include <ntsecapi.h>
#include <sddl.h>

#include <string.h>

#include "lsass.h"
#include "lsa_s.h"

#include <wine/debug.h>


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


typedef struct _LSAP_POLICY_AUDIT_EVENTS_DATA
{
    BOOLEAN AuditingMode;
    DWORD MaximumAuditEventCount;
    DWORD AuditEvents[0];
} LSAP_POLICY_AUDIT_EVENTS_DATA, *PLSAP_POLICY_AUDIT_EVENTS_DATA;


extern SID_IDENTIFIER_AUTHORITY NullSidAuthority;
extern SID_IDENTIFIER_AUTHORITY WorldSidAuthority;
extern SID_IDENTIFIER_AUTHORITY LocalSidAuthority;
extern SID_IDENTIFIER_AUTHORITY CreatorSidAuthority;
extern SID_IDENTIFIER_AUTHORITY NtAuthority;

extern PSID BuiltinDomainSid;
extern UNICODE_STRING BuiltinDomainName;
extern PSID AccountDomainSid;
extern UNICODE_STRING AccountDomainName;


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
VOID
LsarStartRpcServer(VOID);

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
LsarpLookupPrivilegeValue(PRPC_UNICODE_STRING Name,
                          PLUID Value);

NTSTATUS
LsarpEnumeratePrivileges(DWORD *EnumerationContext,
                         PLSAPR_PRIVILEGE_ENUM_BUFFER EnumerationBuffer,
                         DWORD PreferedMaximumLength);

NTSTATUS
LsapLookupAccountRightName(ULONG RightValue,
                           PRPC_UNICODE_STRING *Name);

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

/* EOF */
