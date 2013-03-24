/*
 * PROJECT:     Authentication Package DLL
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        dll/win32/msv1_0/msv1_0.h
 * PURPOSE:     Common header file
 * COPYRIGHT:   Copyright 2013 Eric Kohl
 */

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H
#include <stdarg.h>
#include <windef.h>
#include <winbase.h>
#include <winreg.h>
#define NTOS_MODE_USER
#include <ndk/cmfuncs.h>
#include <ndk/kefuncs.h>
#include <ndk/lpctypes.h>
#include <ndk/lpcfuncs.h>
#include <ndk/mmfuncs.h>
#include <ndk/obfuncs.h>
#include <ndk/psfuncs.h>
#include <ndk/rtlfuncs.h>
#include <ndk/setypes.h>

#include <sspi.h>
#include <ntsecapi.h>
#include <ntsecpkg.h>
#include <ntsam.h>
#include <ntlsa.h>

#include <samsrv/samsrv.h>
//#include <lsass/lsasrv.h>

#include <wine/debug.h>

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

typedef struct _SAMPR_ULONG_ARRAY
{
    ULONG Count;
    PULONG Element;
} SAMPR_ULONG_ARRAY, *PSAMPR_ULONG_ARRAY;

NTSTATUS
NTAPI
SamIConnect(IN PSAMPR_SERVER_NAME ServerName,
            OUT SAMPR_HANDLE *ServerHandle,
            IN ACCESS_MASK DesiredAccess,
            IN BOOLEAN Trusted);

VOID
NTAPI
SamIFree_SAMPR_ULONG_ARRAY(PSAMPR_ULONG_ARRAY Ptr);

NTSTATUS
NTAPI
SamrCloseHandle(IN OUT SAMPR_HANDLE *SamHandle);

NTSTATUS
NTAPI
SamrOpenDomain(IN SAMPR_HANDLE ServerHandle,
               IN ACCESS_MASK DesiredAccess,
               IN PRPC_SID DomainId,
               OUT SAMPR_HANDLE *DomainHandle);

NTSTATUS
NTAPI
SamrLookupNamesInDomain(IN SAMPR_HANDLE DomainHandle,
                        IN ULONG Count,
                        IN RPC_UNICODE_STRING Names[],
                        OUT PSAMPR_ULONG_ARRAY RelativeIds,
                        OUT PSAMPR_ULONG_ARRAY Use);

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
LsarQueryInformationPolicy(IN LSAPR_HANDLE PolicyHandle,
                           IN POLICY_INFORMATION_CLASS InformationClass,
                           OUT PLSAPR_POLICY_INFORMATION *PolicyInformation);


/* EOF */
