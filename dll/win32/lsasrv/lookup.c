/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         Local Security Authority (LSA) Server
 * FILE:            reactos/dll/win32/lsasrv/lookup.c
 * PURPOSE:         Sid / Name lookup functions
 *
 * PROGRAMMERS:     Eric Kohl
 */

#include "lsasrv.h"

#include "resources.h"

/* GLOBALS *****************************************************************/

typedef wchar_t *PSAMPR_SERVER_NAME;
typedef void *SAMPR_HANDLE;

typedef struct _SAMPR_RETURNED_USTRING_ARRAY
{
    unsigned long Count;
    PRPC_UNICODE_STRING Element;
} SAMPR_RETURNED_USTRING_ARRAY, *PSAMPR_RETURNED_USTRING_ARRAY;

VOID
NTAPI
SamIFree_SAMPR_RETURNED_USTRING_ARRAY(PSAMPR_RETURNED_USTRING_ARRAY Ptr);

VOID
NTAPI
SamIFree_SAMPR_ULONG_ARRAY(PSAMPR_ULONG_ARRAY Ptr);

NTSTATUS
NTAPI
SamrConnect(IN PSAMPR_SERVER_NAME ServerName,
            OUT SAMPR_HANDLE *ServerHandle,
            IN ACCESS_MASK DesiredAccess);

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
SamrLookupIdsInDomain(IN SAMPR_HANDLE DomainHandle,
                      IN ULONG Count,
                      IN ULONG *RelativeIds,
                      OUT PSAMPR_RETURNED_USTRING_ARRAY Names,
                      OUT PSAMPR_ULONG_ARRAY Use);

NTSTATUS
NTAPI
SamrLookupNamesInDomain(IN SAMPR_HANDLE DomainHandle,
                        IN ULONG Count,
                        IN RPC_UNICODE_STRING Names[],
                        OUT PSAMPR_ULONG_ARRAY RelativeIds,
                        OUT PSAMPR_ULONG_ARRAY Use);


typedef struct _WELL_KNOWN_SID
{
    LIST_ENTRY ListEntry;
    PSID Sid;
    UNICODE_STRING AccountName;
    UNICODE_STRING DomainName;
    SID_NAME_USE Use;
} WELL_KNOWN_SID, *PWELL_KNOWN_SID;


LIST_ENTRY WellKnownSidListHead;
PSID LsapWorldSid = NULL;
PSID LsapNetworkSid = NULL;
PSID LsapBatchSid = NULL;
PSID LsapInteractiveSid = NULL;
PSID LsapServiceSid = NULL;
PSID LsapLocalSystemSid = NULL;
PSID LsapAdministratorsSid = NULL;


/* FUNCTIONS ***************************************************************/

BOOLEAN
LsapCreateSid(PSID_IDENTIFIER_AUTHORITY IdentifierAuthority,
              UCHAR SubAuthorityCount,
              PULONG SubAuthorities,
              PWSTR AccountName,
              PWSTR DomainName,
              SID_NAME_USE Use,
              PSID *SidPtr)
{
    PWELL_KNOWN_SID SidEntry;
    PULONG p;
    ULONG i;

    SidEntry = RtlAllocateHeap(RtlGetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(WELL_KNOWN_SID));
    if (SidEntry == NULL)
        return FALSE;

    InitializeListHead(&SidEntry->ListEntry);

    SidEntry->Sid = RtlAllocateHeap(RtlGetProcessHeap(),
                                    0,
                                    RtlLengthRequiredSid(SubAuthorityCount));
    if (SidEntry->Sid == NULL)
    {
        RtlFreeHeap(RtlGetProcessHeap(), 0, SidEntry);
        return FALSE;
    }

    RtlInitializeSid(SidEntry->Sid,
                     IdentifierAuthority,
                     SubAuthorityCount);

    for (i = 0; i < (ULONG)SubAuthorityCount; i++)
    {
        p = RtlSubAuthoritySid(SidEntry->Sid, i);
        *p = SubAuthorities[i];
    }

//    RtlInitUnicodeString(&SidEntry->AccountName,
//                         AccountName);
    SidEntry->AccountName.Length = wcslen(AccountName) * sizeof(WCHAR);
    SidEntry->AccountName.MaximumLength = SidEntry->AccountName.Length + sizeof(WCHAR);
    SidEntry->AccountName.Buffer = RtlAllocateHeap(RtlGetProcessHeap(), 0,
                                                   SidEntry->AccountName.MaximumLength);
    if (SidEntry->AccountName.Buffer == NULL)
    {
        RtlFreeHeap(RtlGetProcessHeap(), 0, SidEntry->Sid);
        RtlFreeHeap(RtlGetProcessHeap(), 0, SidEntry);
        return FALSE;
    }

    wcscpy(SidEntry->AccountName.Buffer,
           AccountName);

//    RtlInitUnicodeString(&SidEntry->DomainName,
//                         DomainName);
    SidEntry->DomainName.Length = wcslen(DomainName) * sizeof(WCHAR);
    SidEntry->DomainName.MaximumLength = SidEntry->DomainName.Length + sizeof(WCHAR);
    SidEntry->DomainName.Buffer = RtlAllocateHeap(RtlGetProcessHeap(), 0,
                                                  SidEntry->DomainName.MaximumLength);
    if (SidEntry->DomainName.Buffer == NULL)
    {
        RtlFreeHeap(RtlGetProcessHeap(), 0, SidEntry->AccountName.Buffer);
        RtlFreeHeap(RtlGetProcessHeap(), 0, SidEntry->Sid);
        RtlFreeHeap(RtlGetProcessHeap(), 0, SidEntry);
        return FALSE;
    }

    wcscpy(SidEntry->DomainName.Buffer,
           DomainName);

    SidEntry->Use = Use;

    InsertTailList(&WellKnownSidListHead,
                   &SidEntry->ListEntry);

    if (SidPtr != NULL)
        *SidPtr = SidEntry->Sid;

    return TRUE;
}


NTSTATUS
LsapInitSids(VOID)
{
    WCHAR szAccountName[80];
    WCHAR szDomainName[80];
    ULONG SubAuthorities[8];
    HINSTANCE hInstance;

    InitializeListHead(&WellKnownSidListHead);

    hInstance = GetModuleHandleW(L"lsasrv.dll");

    /* NT Authority */
    LsapLoadString(hInstance, IDS_NT_AUTHORITY, szAccountName, ARRAYSIZE(szAccountName));
    LsapLoadString(hInstance, IDS_NT_AUTHORITY, szDomainName, ARRAYSIZE(szDomainName));
    LsapCreateSid(&NtAuthority,
                  0,
                  NULL,
                  szAccountName,
                  szDomainName,
                  SidTypeDomain,
                  NULL);

    /* Null Sid */
    LsapLoadString(hInstance, IDS_NULL_RID, szAccountName, ARRAYSIZE(szAccountName));

    SubAuthorities[0] = SECURITY_NULL_RID;
    LsapCreateSid(&NullSidAuthority,
                  1,
                  SubAuthorities,
                  szAccountName,
                  L"",
                  SidTypeWellKnownGroup,
                  NULL);

    /* World Sid */
    LsapLoadString(hInstance, IDS_WORLD_RID, szAccountName, ARRAYSIZE(szAccountName));

    SubAuthorities[0] = SECURITY_WORLD_RID;
    LsapCreateSid(&WorldSidAuthority,
                  1,
                  SubAuthorities,
                  szAccountName,
                  L"",
                  SidTypeWellKnownGroup,
                  &LsapWorldSid);

    /* Local Sid */
    LsapLoadString(hInstance, IDS_LOCAL_RID, szAccountName, ARRAYSIZE(szAccountName));

    SubAuthorities[0] = SECURITY_LOCAL_RID;
    LsapCreateSid(&LocalSidAuthority,
                  1,
                  SubAuthorities,
                  szAccountName,
                  L"",
                  SidTypeWellKnownGroup,
                  NULL);

    /* Creator Owner Sid */
    LsapLoadString(hInstance, IDS_CREATOR_OWNER_RID, szAccountName, ARRAYSIZE(szAccountName));

    SubAuthorities[0] = SECURITY_CREATOR_OWNER_RID;
    LsapCreateSid(&CreatorSidAuthority,
                  1,
                  SubAuthorities,
                  szAccountName,
                  L"",
                  SidTypeWellKnownGroup,
                  NULL);

    /* Creator Group Sid */
    LsapLoadString(hInstance, IDS_CREATOR_GROUP_RID, szAccountName, ARRAYSIZE(szAccountName));

    SubAuthorities[0] = SECURITY_CREATOR_GROUP_RID;
    LsapCreateSid(&CreatorSidAuthority,
                  1,
                  SubAuthorities,
                  szAccountName,
                  L"",
                  SidTypeWellKnownGroup,
                  NULL);

    /* Creator Owner Server Sid */
    LsapLoadString(hInstance, IDS_CREATOR_OWNER_SERVER_RID, szAccountName, ARRAYSIZE(szAccountName));

    SubAuthorities[0] = SECURITY_CREATOR_OWNER_SERVER_RID;
    LsapCreateSid(&CreatorSidAuthority,
                  1,
                  SubAuthorities,
                  szAccountName,
                  L"",
                  SidTypeWellKnownGroup,
                  NULL);

    /* Creator Group Server Sid */
    LsapLoadString(hInstance, IDS_CREATOR_GROUP_SERVER_RID, szAccountName, ARRAYSIZE(szAccountName));

    SubAuthorities[0] = SECURITY_CREATOR_GROUP_SERVER_RID;
    LsapCreateSid(&CreatorSidAuthority,
                  1,
                  SubAuthorities,
                  szAccountName,
                  L"",
                  SidTypeWellKnownGroup,
                  NULL);

    /* Dialup Sid */
    LsapLoadString(hInstance, IDS_DIALUP_RID, szAccountName, ARRAYSIZE(szAccountName));
    LsapLoadString(hInstance, IDS_NT_AUTHORITY, szDomainName, ARRAYSIZE(szDomainName));

    SubAuthorities[0] = SECURITY_DIALUP_RID;
    LsapCreateSid(&NtAuthority,
                  1,
                  SubAuthorities,
                  szAccountName,
                  szDomainName,
                  SidTypeWellKnownGroup,
                  NULL);

    /* Network Sid */
    LsapLoadString(hInstance, IDS_NETWORK_RID, szAccountName, ARRAYSIZE(szAccountName));

    SubAuthorities[0] = SECURITY_NETWORK_RID;
    LsapCreateSid(&NtAuthority,
                  1,
                  SubAuthorities,
                  szAccountName,
                  szDomainName,
                  SidTypeWellKnownGroup,
                  &LsapNetworkSid);

    /* Batch Sid*/
    LsapLoadString(hInstance, IDS_BATCH_RID, szAccountName, ARRAYSIZE(szAccountName));

    SubAuthorities[0] = SECURITY_BATCH_RID;
    LsapCreateSid(&NtAuthority,
                  1,
                  SubAuthorities,
                  szAccountName,
                  szDomainName,
                  SidTypeWellKnownGroup,
                  &LsapBatchSid);

    /* Interactive Sid */
    LsapLoadString(hInstance, IDS_INTERACTIVE_RID, szAccountName, ARRAYSIZE(szAccountName));

    SubAuthorities[0] = SECURITY_INTERACTIVE_RID;
    LsapCreateSid(&NtAuthority,
                  1,
                  SubAuthorities,
                  szAccountName,
                  szDomainName,
                  SidTypeWellKnownGroup,
                  &LsapInteractiveSid);

    /* Service Sid */
    LsapLoadString(hInstance, IDS_SERVICE_RID, szAccountName, ARRAYSIZE(szAccountName));

    SubAuthorities[0] = SECURITY_SERVICE_RID;
    LsapCreateSid(&NtAuthority,
                  1,
                  SubAuthorities,
                  szAccountName,
                  szDomainName,
                  SidTypeWellKnownGroup,
                  &LsapServiceSid);

    /* Anonymous Logon Sid */
    LsapLoadString(hInstance, IDS_ANONYMOUS_LOGON_RID, szAccountName, ARRAYSIZE(szAccountName));

    SubAuthorities[0] = SECURITY_ANONYMOUS_LOGON_RID;
    LsapCreateSid(&NtAuthority,
                  1,
                  SubAuthorities,
                  szAccountName,
                  szDomainName,
                  SidTypeWellKnownGroup,
                  NULL);

    /* Proxy Sid */
    LsapLoadString(hInstance, IDS_PROXY_RID, szAccountName, ARRAYSIZE(szAccountName));

    SubAuthorities[0] = SECURITY_PROXY_RID;
    LsapCreateSid(&NtAuthority,
                  1,
                  SubAuthorities,
                  szAccountName,
                  szDomainName,
                  SidTypeWellKnownGroup,
                  NULL);

    /* Enterprise Controllers Sid */
    LsapLoadString(hInstance, IDS_ENTERPRISE_CONTROLLERS_RID, szAccountName, ARRAYSIZE(szAccountName));

    SubAuthorities[0] = SECURITY_ENTERPRISE_CONTROLLERS_RID;
    LsapCreateSid(&NtAuthority,
                  1,
                  SubAuthorities,
                  szAccountName,
                  szDomainName,
                  SidTypeWellKnownGroup,
                  NULL);

    /* Principal Self Sid */
    LsapLoadString(hInstance, IDS_PRINCIPAL_SELF_RID, szAccountName, ARRAYSIZE(szAccountName));

    SubAuthorities[0] = SECURITY_PRINCIPAL_SELF_RID;
    LsapCreateSid(&NtAuthority,
                  1,
                  SubAuthorities,
                  szAccountName,
                  szDomainName,
                  SidTypeWellKnownGroup,
                  NULL);

    /* Authenticated Users Sid */
    LsapLoadString(hInstance, IDS_AUTHENTICATED_USER_RID, szAccountName, ARRAYSIZE(szAccountName));

    SubAuthorities[0] = SECURITY_AUTHENTICATED_USER_RID;
    LsapCreateSid(&NtAuthority,
                  1,
                  SubAuthorities,
                  szAccountName,
                  szDomainName,
                  SidTypeWellKnownGroup,
                  NULL);

    /* Restricted Code Sid */
    LsapLoadString(hInstance, IDS_RESTRICTED_CODE_RID, szAccountName, ARRAYSIZE(szAccountName));

    SubAuthorities[0] = SECURITY_RESTRICTED_CODE_RID;
    LsapCreateSid(&NtAuthority,
                  1,
                  SubAuthorities,
                  szAccountName,
                  szDomainName,
                  SidTypeWellKnownGroup,
                  NULL);

    /* Terminal Server Sid */
    LsapLoadString(hInstance, IDS_TERMINAL_SERVER_RID, szAccountName, ARRAYSIZE(szAccountName));

    SubAuthorities[0] = SECURITY_TERMINAL_SERVER_RID;
    LsapCreateSid(&NtAuthority,
                  1,
                  SubAuthorities,
                  szAccountName,
                  szDomainName,
                  SidTypeWellKnownGroup,
                  NULL);

    /* Remote Logon Sid */
    LsapLoadString(hInstance, IDS_REMOTE_LOGON_RID, szAccountName, ARRAYSIZE(szAccountName));

    SubAuthorities[0] = SECURITY_REMOTE_LOGON_RID;
    LsapCreateSid(&NtAuthority,
                  1,
                  SubAuthorities,
                  szAccountName,
                  szDomainName,
                  SidTypeWellKnownGroup,
                  NULL);

    /* This Organization Sid */
    LsapLoadString(hInstance, IDS_THIS_ORGANIZATION_RID, szAccountName, ARRAYSIZE(szAccountName));

    SubAuthorities[0] = SECURITY_THIS_ORGANIZATION_RID;
    LsapCreateSid(&NtAuthority,
                  1,
                  SubAuthorities,
                  szAccountName,
                  szDomainName,
                  SidTypeWellKnownGroup,
                  NULL);

    /* Local System Sid */
    LsapLoadString(hInstance, IDS_LOCAL_SYSTEM_RID, szAccountName, ARRAYSIZE(szAccountName));

    SubAuthorities[0] = SECURITY_LOCAL_SYSTEM_RID;
    LsapCreateSid(&NtAuthority,
                  1,
                  SubAuthorities,
                  szAccountName,
                  szDomainName,
                  SidTypeWellKnownGroup,
                  &LsapLocalSystemSid);

    /* Local Service Sid */
    LsapLoadString(hInstance, IDS_LOCAL_SERVICE_RID, szAccountName, ARRAYSIZE(szAccountName));

    SubAuthorities[0] = SECURITY_LOCAL_SERVICE_RID;
    LsapCreateSid(&NtAuthority,
                  1,
                  SubAuthorities,
                  szAccountName,
                  szDomainName,
                  SidTypeWellKnownGroup,
                  NULL);

    LsapCreateSid(&NtAuthority,
                  1,
                  SubAuthorities,
                  L"LOCALSERVICE",
                  L"NT AUTHORITY",
                  SidTypeWellKnownGroup,
                  NULL);

    /* Network Service Sid */
    LsapLoadString(hInstance, IDS_NETWORK_SERVICE_RID, szAccountName, ARRAYSIZE(szAccountName));

    SubAuthorities[0] = SECURITY_NETWORK_SERVICE_RID;
    LsapCreateSid(&NtAuthority,
                  1,
                  SubAuthorities,
                  szAccountName,
                  szDomainName,
                  SidTypeWellKnownGroup,
                  NULL);

    LsapCreateSid(&NtAuthority,
                  1,
                  SubAuthorities,
                  L"NETWORKSERVICE",
                  L"NT AUTHORITY",
                  SidTypeWellKnownGroup,
                  NULL);

    /* Builtin Domain Sid */
    LsapLoadString(hInstance, IDS_BUILTIN_DOMAIN_RID, szAccountName, ARRAYSIZE(szAccountName));
    LsapLoadString(hInstance, IDS_BUILTIN_DOMAIN_RID, szDomainName, ARRAYSIZE(szDomainName));

    SubAuthorities[0] = SECURITY_BUILTIN_DOMAIN_RID;
    LsapCreateSid(&NtAuthority,
                  1,
                  SubAuthorities,
                  szAccountName,
                  szDomainName,
                  SidTypeDomain,
                  NULL);

    /* Administrators Alias Sid */
    LsapLoadString(hInstance, IDS_ALIAS_RID_ADMINS, szAccountName, ARRAYSIZE(szAccountName));

    SubAuthorities[0] = SECURITY_BUILTIN_DOMAIN_RID;
    SubAuthorities[1] = DOMAIN_ALIAS_RID_ADMINS;
    LsapCreateSid(&NtAuthority,
                  2,
                  SubAuthorities,
                  szAccountName,
                  szDomainName,
                  SidTypeAlias,
                  &LsapAdministratorsSid);

    /* Users Alias Sid */
    LsapLoadString(hInstance, IDS_ALIAS_RID_USERS, szAccountName, ARRAYSIZE(szAccountName));

    SubAuthorities[0] = SECURITY_BUILTIN_DOMAIN_RID;
    SubAuthorities[1] = DOMAIN_ALIAS_RID_USERS;
    LsapCreateSid(&NtAuthority,
                  2,
                  SubAuthorities,
                  szAccountName,
                  szDomainName,
                  SidTypeAlias,
                  NULL);

    /* Guests Alias Sid */
    LsapLoadString(hInstance, IDS_ALIAS_RID_GUESTS, szAccountName, ARRAYSIZE(szAccountName));

    SubAuthorities[0] = SECURITY_BUILTIN_DOMAIN_RID;
    SubAuthorities[1] = DOMAIN_ALIAS_RID_GUESTS;
    LsapCreateSid(&NtAuthority,
                  2,
                  SubAuthorities,
                  szAccountName,
                  szDomainName,
                  SidTypeAlias,
                  NULL);

    /* Power User Alias Sid */
    LsapLoadString(hInstance, IDS_ALIAS_RID_POWER_USERS, szAccountName, ARRAYSIZE(szAccountName));

    SubAuthorities[0] = SECURITY_BUILTIN_DOMAIN_RID;
    SubAuthorities[1] = DOMAIN_ALIAS_RID_POWER_USERS;
    LsapCreateSid(&NtAuthority,
                  2,
                  SubAuthorities,
                  szAccountName,
                  szDomainName,
                  SidTypeAlias,
                  NULL);

    /* Account Operators Alias Sid */
    LsapLoadString(hInstance, IDS_ALIAS_RID_ACCOUNT_OPS, szAccountName, ARRAYSIZE(szAccountName));

    SubAuthorities[0] = SECURITY_BUILTIN_DOMAIN_RID;
    SubAuthorities[1] = DOMAIN_ALIAS_RID_ACCOUNT_OPS;
    LsapCreateSid(&NtAuthority,
                  2,
                  SubAuthorities,
                  szAccountName,
                  szDomainName,
                  SidTypeAlias,
                  NULL);

    /* System Operators Alias Sid */
    LsapLoadString(hInstance, IDS_ALIAS_RID_SYSTEM_OPS, szAccountName, ARRAYSIZE(szAccountName));

    SubAuthorities[0] = SECURITY_BUILTIN_DOMAIN_RID;
    SubAuthorities[1] = DOMAIN_ALIAS_RID_SYSTEM_OPS;
    LsapCreateSid(&NtAuthority,
                  2,
                  SubAuthorities,
                  szAccountName,
                  szDomainName,
                  SidTypeAlias,
                  NULL);

    /* Print Operators Alias Sid */
    LsapLoadString(hInstance, IDS_ALIAS_RID_PRINT_OPS, szAccountName, ARRAYSIZE(szAccountName));

    SubAuthorities[0] = SECURITY_BUILTIN_DOMAIN_RID;
    SubAuthorities[1] = DOMAIN_ALIAS_RID_PRINT_OPS;
    LsapCreateSid(&NtAuthority,
                  2,
                  SubAuthorities,
                  szAccountName,
                  szDomainName,
                  SidTypeAlias,
                  NULL);

    /* Backup Operators Alias Sid */
    LsapLoadString(hInstance, IDS_ALIAS_RID_BACKUP_OPS, szAccountName, ARRAYSIZE(szAccountName));

    SubAuthorities[0] = SECURITY_BUILTIN_DOMAIN_RID;
    SubAuthorities[1] = DOMAIN_ALIAS_RID_BACKUP_OPS;
    LsapCreateSid(&NtAuthority,
                  2,
                  SubAuthorities,
                  szAccountName,
                  szDomainName,
                  SidTypeAlias,
                  NULL);

    /* Replicators Alias Sid */
    LsapLoadString(hInstance, IDS_ALIAS_RID_REPLICATOR, szAccountName, ARRAYSIZE(szAccountName));

    SubAuthorities[0] = SECURITY_BUILTIN_DOMAIN_RID;
    SubAuthorities[1] = DOMAIN_ALIAS_RID_REPLICATOR;
    LsapCreateSid(&NtAuthority,
                  2,
                  SubAuthorities,
                  szAccountName,
                  szDomainName,
                  SidTypeAlias,
                  NULL);

    /* RAS Servers Alias Sid */
    LsapLoadString(hInstance, IDS_ALIAS_RID_RAS_SERVERS, szAccountName, ARRAYSIZE(szAccountName));

    SubAuthorities[0] = SECURITY_BUILTIN_DOMAIN_RID;
    SubAuthorities[1] = DOMAIN_ALIAS_RID_RAS_SERVERS;
    LsapCreateSid(&NtAuthority,
                  2,
                  SubAuthorities,
                  szAccountName,
                  szDomainName,
                  SidTypeAlias,
                  NULL);

    /* Pre-Windows 2000 Compatible Access Alias Sid */
    LsapLoadString(hInstance, IDS_ALIAS_RID_PREW2KCOMPACCESS, szAccountName, ARRAYSIZE(szAccountName));

    SubAuthorities[0] = SECURITY_BUILTIN_DOMAIN_RID;
    SubAuthorities[1] = DOMAIN_ALIAS_RID_PREW2KCOMPACCESS;
    LsapCreateSid(&NtAuthority,
                  2,
                  SubAuthorities,
                  szAccountName,
                  szDomainName,
                  SidTypeAlias,
                  NULL);

    /* Remote Desktop Users Alias Sid */
    LsapLoadString(hInstance, IDS_ALIAS_RID_REMOTE_DESKTOP_USERS, szAccountName, ARRAYSIZE(szAccountName));

    SubAuthorities[0] = SECURITY_BUILTIN_DOMAIN_RID;
    SubAuthorities[1] = DOMAIN_ALIAS_RID_REMOTE_DESKTOP_USERS;
    LsapCreateSid(&NtAuthority,
                  2,
                  SubAuthorities,
                  szAccountName,
                  szDomainName,
                  SidTypeAlias,
                  NULL);

    /* Network Configuration Operators Alias Sid */
    LsapLoadString(hInstance, IDS_ALIAS_RID_NETWORK_CONFIGURATION_OPS, szAccountName, ARRAYSIZE(szAccountName));

    SubAuthorities[0] = SECURITY_BUILTIN_DOMAIN_RID;
    SubAuthorities[1] = DOMAIN_ALIAS_RID_NETWORK_CONFIGURATION_OPS;
    LsapCreateSid(&NtAuthority,
                  2,
                  SubAuthorities,
                  szAccountName,
                  szDomainName,
                  SidTypeAlias,
                  NULL);

    /* FIXME: Add more well known sids */

    return STATUS_SUCCESS;
}


PWELL_KNOWN_SID
LsapLookupWellKnownSid(PSID Sid)
{
    PLIST_ENTRY ListEntry;
    PWELL_KNOWN_SID Ptr;

    ListEntry = WellKnownSidListHead.Flink;
    while (ListEntry != &WellKnownSidListHead)
    {
        Ptr = CONTAINING_RECORD(ListEntry,
                                WELL_KNOWN_SID,
                                ListEntry);
        if (RtlEqualSid(Sid, Ptr->Sid))
        {
            return Ptr;
        }

        ListEntry = ListEntry->Flink;
    }

    return NULL;
}


PWELL_KNOWN_SID
LsapLookupIsolatedWellKnownName(PUNICODE_STRING AccountName)
{
    PLIST_ENTRY ListEntry;
    PWELL_KNOWN_SID Ptr;

    ListEntry = WellKnownSidListHead.Flink;
    while (ListEntry != &WellKnownSidListHead)
    {
        Ptr = CONTAINING_RECORD(ListEntry,
                                WELL_KNOWN_SID,
                                ListEntry);
        if (RtlEqualUnicodeString(AccountName, &Ptr->AccountName, TRUE))
        {
            return Ptr;
        }

        ListEntry = ListEntry->Flink;
    }

    return NULL;
}


PWELL_KNOWN_SID
LsapLookupFullyQualifiedWellKnownName(PUNICODE_STRING AccountName,
                                      PUNICODE_STRING DomainName)
{
    PLIST_ENTRY ListEntry;
    PWELL_KNOWN_SID Ptr;

    ListEntry = WellKnownSidListHead.Flink;
    while (ListEntry != &WellKnownSidListHead)
    {
        Ptr = CONTAINING_RECORD(ListEntry,
                                WELL_KNOWN_SID,
                                ListEntry);
        if (RtlEqualUnicodeString(AccountName, &Ptr->AccountName, TRUE) &&
            RtlEqualUnicodeString(DomainName, &Ptr->DomainName, TRUE))
        {
            return Ptr;
        }

        ListEntry = ListEntry->Flink;
    }

    return NULL;
}


static
NTSTATUS
LsapSplitNames(DWORD Count,
               PRPC_UNICODE_STRING Names,
               PRPC_UNICODE_STRING *DomainNames,
               PRPC_UNICODE_STRING *AccountNames)
{
    PRPC_UNICODE_STRING DomainsBuffer = NULL;
    PRPC_UNICODE_STRING AccountsBuffer = NULL;
    ULONG DomainLength;
    ULONG AccountLength;
    ULONG i;
    LPWSTR Ptr;
    NTSTATUS Status = STATUS_SUCCESS;

    DomainsBuffer = MIDL_user_allocate(Count * sizeof(RPC_UNICODE_STRING));
    if (DomainsBuffer == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto done;
    }

    AccountsBuffer = MIDL_user_allocate(Count * sizeof(RPC_UNICODE_STRING));
    if (AccountsBuffer == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto done;
    }

    for (i = 0; i < Count; i++)
    {
//TRACE("Name: %wZ\n", &Names[i]);

        Ptr = wcschr(Names[i].Buffer, L'\\');
        if (Ptr == NULL)
        {
            AccountLength = Names[i].Length / sizeof(WCHAR);

            AccountsBuffer[i].Length = Names[i].Length;
            AccountsBuffer[i].MaximumLength = AccountsBuffer[i].Length + sizeof(WCHAR);
            AccountsBuffer[i].Buffer = MIDL_user_allocate(AccountsBuffer[i].MaximumLength);
            if (AccountsBuffer[i].Buffer == NULL)
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto done;
            }

            CopyMemory(AccountsBuffer[i].Buffer,
                       Names[i].Buffer,
                       AccountsBuffer[i].Length);
            AccountsBuffer[i].Buffer[AccountLength] = UNICODE_NULL;

//TRACE("Account name: %wZ\n", &AccountsBuffer[i]);
        }
        else
        {
            DomainLength = (ULONG)(ULONG_PTR)(Ptr - Names[i].Buffer);
            AccountLength = (Names[i].Length / sizeof(WCHAR)) - DomainLength - 1;
//TRACE("DomainLength: %u\n", DomainLength);
//TRACE("AccountLength: %u\n", AccountLength);

            if (DomainLength > 0)
            {
                DomainsBuffer[i].Length = (USHORT)DomainLength * sizeof(WCHAR);
                DomainsBuffer[i].MaximumLength = DomainsBuffer[i].Length + sizeof(WCHAR);
                DomainsBuffer[i].Buffer = MIDL_user_allocate(DomainsBuffer[i].MaximumLength);
                if (DomainsBuffer[i].Buffer == NULL)
                {
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    goto done;
                }

                CopyMemory(DomainsBuffer[i].Buffer,
                           Names[i].Buffer,
                           DomainsBuffer[i].Length);
                DomainsBuffer[i].Buffer[DomainLength] = UNICODE_NULL;

//TRACE("Domain name: %wZ\n", &DomainsBuffer[i]);
            }

            AccountsBuffer[i].Length = (USHORT)AccountLength * sizeof(WCHAR);
            AccountsBuffer[i].MaximumLength = AccountsBuffer[i].Length + sizeof(WCHAR);
            AccountsBuffer[i].Buffer = MIDL_user_allocate(AccountsBuffer[i].MaximumLength);
            if (AccountsBuffer[i].Buffer == NULL)
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto done;
            }

            CopyMemory(AccountsBuffer[i].Buffer,
                       &(Names[i].Buffer[DomainLength + 1]),
                       AccountsBuffer[i].Length);
            AccountsBuffer[i].Buffer[AccountLength] = UNICODE_NULL;

//TRACE("Account name: %wZ\n", &AccountsBuffer[i]);
        }
    }

done:
    if (!NT_SUCCESS(Status))
    {
        if (AccountsBuffer != NULL)
        {
            for (i = 0; i < Count; i++)
            {
                if (AccountsBuffer[i].Buffer != NULL)
                    MIDL_user_free(AccountsBuffer[i].Buffer);
            }

            MIDL_user_free(AccountsBuffer);
        }

        if (DomainsBuffer != NULL)
        {
            for (i = 0; i < Count; i++)
            {
                if (DomainsBuffer[i].Buffer != NULL)
                    MIDL_user_free(DomainsBuffer[i].Buffer);
            }

            MIDL_user_free(DomainsBuffer);
        }
    }
    else
    {
        *DomainNames = DomainsBuffer;
        *AccountNames = AccountsBuffer;
    }

    return Status;
}


static NTSTATUS
LsapAddDomainToDomainsList(PLSAPR_REFERENCED_DOMAIN_LIST ReferencedDomains,
                           PUNICODE_STRING Name,
                           PSID Sid,
                           PULONG Index)
{
    ULONG i;

    i = 0;
    while (i < ReferencedDomains->Entries &&
           ReferencedDomains->Domains[i].Sid != NULL)
    {
        if (RtlEqualSid(Sid, ReferencedDomains->Domains[i].Sid))
        {
            *Index = i;
            return STATUS_SUCCESS;
        }

        i++;
    }

    ReferencedDomains->Domains[i].Sid = MIDL_user_allocate(RtlLengthSid(Sid));
    if (ReferencedDomains->Domains[i].Sid == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    RtlCopySid(RtlLengthSid(Sid), ReferencedDomains->Domains[i].Sid, Sid);

    ReferencedDomains->Domains[i].Name.Length = Name->Length;
    ReferencedDomains->Domains[i].Name.MaximumLength = Name->MaximumLength;
    ReferencedDomains->Domains[i].Name.Buffer = MIDL_user_allocate(Name->MaximumLength);
    if (ReferencedDomains->Domains[i].Name.Buffer == NULL)
    {
        MIDL_user_free(ReferencedDomains->Domains[i].Sid);
        ReferencedDomains->Domains[i].Sid = NULL;
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlCopyMemory(ReferencedDomains->Domains[i].Name.Buffer,
                  Name->Buffer,
                  Name->MaximumLength);

    ReferencedDomains->Entries++;
    *Index = i;

    return STATUS_SUCCESS;
}


static NTSTATUS
LsapAddAuthorityToDomainsList(
    PLSAPR_REFERENCED_DOMAIN_LIST ReferencedDomains,
    PSID Sid,
    PULONG Index)
{
    SID AuthoritySid;
    ULONG i;

    RtlInitializeSid(&AuthoritySid,
                     RtlIdentifierAuthoritySid(Sid),
                     0);

    i = 0;
    while (i < ReferencedDomains->Entries &&
           ReferencedDomains->Domains[i].Sid != NULL)
    {
        if (RtlEqualSid(&AuthoritySid, ReferencedDomains->Domains[i].Sid))
        {
            *Index = i;
            return STATUS_SUCCESS;
        }

        i++;
    }

    ReferencedDomains->Domains[i].Sid = MIDL_user_allocate(RtlLengthSid(&AuthoritySid));
    if (ReferencedDomains->Domains[i].Sid == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    RtlCopySid(RtlLengthSid(&AuthoritySid), ReferencedDomains->Domains[i].Sid, &AuthoritySid);

    ReferencedDomains->Domains[i].Name.Length = 0;
    ReferencedDomains->Domains[i].Name.MaximumLength = sizeof(WCHAR);
    ReferencedDomains->Domains[i].Name.Buffer = MIDL_user_allocate(sizeof(WCHAR));
    if (ReferencedDomains->Domains[i].Name.Buffer == NULL)
    {
        MIDL_user_free(ReferencedDomains->Domains[i].Sid);
        ReferencedDomains->Domains[i].Sid = NULL;
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    ReferencedDomains->Domains[i].Name.Buffer[0] = UNICODE_NULL;

    ReferencedDomains->Entries++;
    *Index = i;

    return STATUS_SUCCESS;
}


static BOOLEAN
LsapIsPrefixSid(IN PSID PrefixSid,
                IN PSID Sid)
{
    PISID Sid1 = PrefixSid, Sid2 = Sid;
    ULONG i;

    if (Sid1->Revision != Sid2->Revision)
        return FALSE;

    if ((Sid1->IdentifierAuthority.Value[0] != Sid2->IdentifierAuthority.Value[0]) ||
        (Sid1->IdentifierAuthority.Value[1] != Sid2->IdentifierAuthority.Value[1]) ||
        (Sid1->IdentifierAuthority.Value[2] != Sid2->IdentifierAuthority.Value[2]) ||
        (Sid1->IdentifierAuthority.Value[3] != Sid2->IdentifierAuthority.Value[3]) ||
        (Sid1->IdentifierAuthority.Value[4] != Sid2->IdentifierAuthority.Value[4]) ||
        (Sid1->IdentifierAuthority.Value[5] != Sid2->IdentifierAuthority.Value[5]))
        return FALSE;

    if (Sid1->SubAuthorityCount >= Sid2->SubAuthorityCount)
        return FALSE;

    if (Sid1->SubAuthorityCount == 0)
        return TRUE;

    for (i = 0; i < Sid1->SubAuthorityCount; i++)
    {
        if (Sid1->SubAuthority[i] != Sid2->SubAuthority[i])
            return FALSE;
    }

    return TRUE;
}


ULONG
LsapGetRelativeIdFromSid(PSID Sid_)
{
    PISID Sid = Sid_;

    if (Sid->SubAuthorityCount != 0)
        return Sid->SubAuthority[Sid->SubAuthorityCount - 1];

    return 0;
}


static PSID
CreateSidFromSidAndRid(PSID SrcSid,
                       ULONG RelativeId)
{
    UCHAR RidCount;
    PSID DstSid;
    ULONG i;
    ULONG DstSidSize;
    PULONG p, q;

    RidCount = *RtlSubAuthorityCountSid(SrcSid);
    if (RidCount >= 8)
        return NULL;

    DstSidSize = RtlLengthRequiredSid(RidCount + 1);

    DstSid = MIDL_user_allocate(DstSidSize);
    if (DstSid == NULL)
        return NULL;

    RtlInitializeSid(DstSid,
                     RtlIdentifierAuthoritySid(SrcSid),
                     RidCount + 1);

    for (i = 0; i < (ULONG)RidCount; i++)
    {
        p = RtlSubAuthoritySid(SrcSid, i);
        q = RtlSubAuthoritySid(DstSid, i);
        *q = *p;
    }

    q = RtlSubAuthoritySid(DstSid, (ULONG)RidCount);
    *q = RelativeId;

    return DstSid;
}


static PSID
CreateDomainSidFromAccountSid(PSID AccountSid)
{
    UCHAR RidCount;
    PSID DomainSid;
    ULONG i;
    ULONG DstSidSize;
    PULONG p, q;

    RidCount = *RtlSubAuthorityCountSid(AccountSid);
    if (RidCount > 0)
        RidCount--;

    DstSidSize = RtlLengthRequiredSid(RidCount);

    DomainSid = MIDL_user_allocate(DstSidSize);
    if (DomainSid == NULL)
        return NULL;

    RtlInitializeSid(DomainSid,
                     RtlIdentifierAuthoritySid(AccountSid),
                     RidCount);

    for (i = 0; i < (ULONG)RidCount; i++)
    {
        p = RtlSubAuthoritySid(AccountSid, i);
        q = RtlSubAuthoritySid(DomainSid, i);
        *q = *p;
    }

    return DomainSid;
}


static PSID
LsapCopySid(PSID SrcSid)
{
    UCHAR RidCount;
    PSID DstSid;
    ULONG i;
    ULONG DstSidSize;
    PULONG p, q;

    RidCount = *RtlSubAuthorityCountSid(SrcSid);
    DstSidSize = RtlLengthRequiredSid(RidCount);

    DstSid = MIDL_user_allocate(DstSidSize);
    if (DstSid == NULL)
        return NULL;

    RtlInitializeSid(DstSid,
                     RtlIdentifierAuthoritySid(SrcSid),
                     RidCount);

    for (i = 0; i < (ULONG)RidCount; i++)
    {
        p = RtlSubAuthoritySid(SrcSid, i);
        q = RtlSubAuthoritySid(DstSid, i);
        *q = *p;
    }

    return DstSid;
}


static
NTSTATUS
LsapLookupIsolatedNames(DWORD Count,
                        PRPC_UNICODE_STRING DomainNames,
                        PRPC_UNICODE_STRING AccountNames,
                        PLSAPR_REFERENCED_DOMAIN_LIST DomainsBuffer,
                        PLSAPR_TRANSLATED_SID_EX2 SidsBuffer,
                        PULONG Mapped)
{
    UNICODE_STRING EmptyDomainName = RTL_CONSTANT_STRING(L"");
    PWELL_KNOWN_SID ptr, ptr2;
    PSID DomainSid;
    ULONG DomainIndex;
    ULONG i;
    NTSTATUS Status = STATUS_SUCCESS;

    for (i = 0; i < Count; i++)
    {
        /* Ignore names which were already mapped */
        if (SidsBuffer[i].Use != SidTypeUnknown)
            continue;

        /* Ignore fully qualified account names */
        if (DomainNames[i].Length != 0)
            continue;

        TRACE("Mapping name: %wZ\n", &AccountNames[i]);

        /* Look-up all well-known names */
        ptr = LsapLookupIsolatedWellKnownName((PUNICODE_STRING)&AccountNames[i]);
        if (ptr != NULL)
        {
            SidsBuffer[i].Use = ptr->Use;
            SidsBuffer[i].Sid = LsapCopySid(ptr->Sid);
            if (SidsBuffer[i].Sid == NULL)
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto done;
            }

            SidsBuffer[i].DomainIndex = -1;
            SidsBuffer[i].Flags = 0;

            if (ptr->Use == SidTypeDomain)
            {
                Status = LsapAddDomainToDomainsList(DomainsBuffer,
                                                    &ptr->AccountName,
                                                    ptr->Sid,
                                                    &DomainIndex);
                if (!NT_SUCCESS(Status))
                    goto done;

                SidsBuffer[i].DomainIndex = DomainIndex;
            }
            else
            {
                ptr2= LsapLookupIsolatedWellKnownName(&ptr->DomainName);
                if (ptr2 != NULL)
                {
                    Status = LsapAddDomainToDomainsList(DomainsBuffer,
                                                        &ptr2->AccountName,
                                                        ptr2->Sid,
                                                        &DomainIndex);
                    if (!NT_SUCCESS(Status))
                        goto done;

                    SidsBuffer[i].DomainIndex = DomainIndex;
                }
                else
                {
                    DomainSid = CreateDomainSidFromAccountSid(ptr->Sid);
                    if (DomainSid == NULL)
                    {
                        Status = STATUS_INSUFFICIENT_RESOURCES;
                        goto done;
                    }

                    Status = LsapAddDomainToDomainsList(DomainsBuffer,
                                                        &EmptyDomainName,
                                                        DomainSid,
                                                        &DomainIndex);

                    if (DomainSid != NULL)
                    {
                        MIDL_user_free(DomainSid);
                        DomainSid = NULL;
                    }

                    if (!NT_SUCCESS(Status))
                        goto done;

                    SidsBuffer[i].DomainIndex = DomainIndex;
                }
            }

            (*Mapped)++;
            continue;
        }

        /* Look-up the built-in domain */
        if (RtlEqualUnicodeString((PUNICODE_STRING)&AccountNames[i], &BuiltinDomainName, TRUE))
        {
            SidsBuffer[i].Use = SidTypeDomain;
            SidsBuffer[i].Sid = LsapCopySid(BuiltinDomainSid);
            if (SidsBuffer[i].Sid == NULL)
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto done;
            }

            SidsBuffer[i].DomainIndex = -1;
            SidsBuffer[i].Flags = 0;

            Status = LsapAddDomainToDomainsList(DomainsBuffer,
                                                &BuiltinDomainName,
                                                BuiltinDomainSid,
                                                &DomainIndex);
            if (!NT_SUCCESS(Status))
                goto done;

            SidsBuffer[i].DomainIndex = DomainIndex;

            (*Mapped)++;
            continue;
        }

        /* Look-up the account domain */
        if (RtlEqualUnicodeString((PUNICODE_STRING)&AccountNames[i], &AccountDomainName, TRUE))
        {
            SidsBuffer[i].Use = SidTypeDomain;
            SidsBuffer[i].Sid = LsapCopySid(AccountDomainSid);
            if (SidsBuffer[i].Sid == NULL)
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto done;
            }
            SidsBuffer[i].DomainIndex = -1;
            SidsBuffer[i].Flags = 0;

            Status = LsapAddDomainToDomainsList(DomainsBuffer,
                                                &AccountDomainName,
                                                AccountDomainSid,
                                                &DomainIndex);
            if (!NT_SUCCESS(Status))
                goto done;

            SidsBuffer[i].DomainIndex = DomainIndex;

            (*Mapped)++;
            continue;
        }

        /* FIXME: Look-up the primary domain */

        /* FIXME: Look-up the trusted domains */

    }

done:

    return Status;
}


static
NTSTATUS
LsapLookupIsolatedBuiltinNames(DWORD Count,
                               PRPC_UNICODE_STRING DomainNames,
                               PRPC_UNICODE_STRING AccountNames,
                               PLSAPR_REFERENCED_DOMAIN_LIST DomainsBuffer,
                               PLSAPR_TRANSLATED_SID_EX2 SidsBuffer,
                               PULONG Mapped)
{
    SAMPR_HANDLE ServerHandle = NULL;
    SAMPR_HANDLE DomainHandle = NULL;
    SAMPR_ULONG_ARRAY RelativeIds = {0, NULL};
    SAMPR_ULONG_ARRAY Use = {0, NULL};
    ULONG DomainIndex;
    ULONG i;
    NTSTATUS Status = STATUS_SUCCESS;

    Status = SamrConnect(NULL,
                         &ServerHandle,
                         SAM_SERVER_CONNECT | SAM_SERVER_LOOKUP_DOMAIN);
    if (!NT_SUCCESS(Status))
    {
        TRACE("SamrConnect failed (Status %08lx)\n", Status);
        goto done;
    }

    Status = SamrOpenDomain(ServerHandle,
                            DOMAIN_LOOKUP,
                            BuiltinDomainSid,
                            &DomainHandle);
    if (!NT_SUCCESS(Status))
    {
        TRACE("SamOpenDomain failed (Status %08lx)\n", Status);
        goto done;
    }

    for (i = 0; i < Count; i++)
    {
        /* Ignore names which were already mapped */
        if (SidsBuffer[i].Use != SidTypeUnknown)
            continue;

        /* Ignore fully qualified account names */
        if (DomainNames[i].Length != 0)
            continue;

        TRACE("Mapping name: %wZ\n", &AccountNames[i]);

        Status = SamrLookupNamesInDomain(DomainHandle,
                                         1,
                                         &AccountNames[i],
                                         &RelativeIds,
                                         &Use);
        if (NT_SUCCESS(Status))
        {
            TRACE("Found relative ID: %lu\n", RelativeIds.Element[0]);

            SidsBuffer[i].Use = Use.Element[0];
            SidsBuffer[i].Sid = CreateSidFromSidAndRid(BuiltinDomainSid,
                                                       RelativeIds.Element[0]);
            if (SidsBuffer[i].Sid == NULL)
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto done;
            }

            SidsBuffer[i].DomainIndex = -1;
            SidsBuffer[i].Flags = 0;

            Status = LsapAddDomainToDomainsList(DomainsBuffer,
                                                &BuiltinDomainName,
                                                BuiltinDomainSid,
                                                &DomainIndex);
            if (!NT_SUCCESS(Status))
                goto done;

            SidsBuffer[i].DomainIndex = DomainIndex;

            (*Mapped)++;
        }

        SamIFree_SAMPR_ULONG_ARRAY(&RelativeIds);
        SamIFree_SAMPR_ULONG_ARRAY(&Use);
    }

done:
    if (DomainHandle != NULL)
        SamrCloseHandle(&DomainHandle);

    if (ServerHandle != NULL)
        SamrCloseHandle(&ServerHandle);

    return Status;
}


static
NTSTATUS
LsapLookupIsolatedAccountNames(DWORD Count,
                               PRPC_UNICODE_STRING DomainNames,
                               PRPC_UNICODE_STRING AccountNames,
                               PLSAPR_REFERENCED_DOMAIN_LIST DomainsBuffer,
                               PLSAPR_TRANSLATED_SID_EX2 SidsBuffer,
                               PULONG Mapped)
{
    SAMPR_HANDLE ServerHandle = NULL;
    SAMPR_HANDLE DomainHandle = NULL;
    SAMPR_ULONG_ARRAY RelativeIds = {0, NULL};
    SAMPR_ULONG_ARRAY Use = {0, NULL};
    ULONG DomainIndex;
    ULONG i;
    NTSTATUS Status = STATUS_SUCCESS;

    TRACE("()\n");

    Status = SamrConnect(NULL,
                         &ServerHandle,
                         SAM_SERVER_CONNECT | SAM_SERVER_LOOKUP_DOMAIN);
    if (!NT_SUCCESS(Status))
    {
        TRACE("SamrConnect failed (Status %08lx)\n", Status);
        goto done;
    }

    Status = SamrOpenDomain(ServerHandle,
                            DOMAIN_LOOKUP,
                            AccountDomainSid,
                            &DomainHandle);
    if (!NT_SUCCESS(Status))
    {
        TRACE("SamOpenDomain failed (Status %08lx)\n", Status);
        goto done;
    }

    for (i = 0; i < Count; i++)
    {
        /* Ignore names which were already mapped */
        if (SidsBuffer[i].Use != SidTypeUnknown)
            continue;

        /* Ignore fully qualified account names */
        if (DomainNames[i].Length != 0)
            continue;

        TRACE("Mapping name: %wZ\n", &AccountNames[i]);

        Status = SamrLookupNamesInDomain(DomainHandle,
                                         1,
                                         &AccountNames[i],
                                         &RelativeIds,
                                         &Use);
        if (NT_SUCCESS(Status))
        {
            TRACE("Found relative ID: %lu\n", RelativeIds.Element[0]);

            SidsBuffer[i].Use = Use.Element[0];
            SidsBuffer[i].Sid = CreateSidFromSidAndRid(AccountDomainSid,
                                                       RelativeIds.Element[0]);
            if (SidsBuffer[i].Sid == NULL)
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto done;
            }

            SidsBuffer[i].DomainIndex = -1;
            SidsBuffer[i].Flags = 0;

            Status = LsapAddDomainToDomainsList(DomainsBuffer,
                                                &AccountDomainName,
                                                AccountDomainSid,
                                                &DomainIndex);
            if (!NT_SUCCESS(Status))
                goto done;

            SidsBuffer[i].DomainIndex = DomainIndex;

            (*Mapped)++;
        }

        SamIFree_SAMPR_ULONG_ARRAY(&RelativeIds);
        SamIFree_SAMPR_ULONG_ARRAY(&Use);
    }

done:
    if (DomainHandle != NULL)
        SamrCloseHandle(&DomainHandle);

    if (ServerHandle != NULL)
        SamrCloseHandle(&ServerHandle);

    return Status;
}


static
NTSTATUS
LsapLookupFullyQualifiedWellKnownNames(DWORD Count,
                                       PRPC_UNICODE_STRING DomainNames,
                                       PRPC_UNICODE_STRING AccountNames,
                                       PLSAPR_REFERENCED_DOMAIN_LIST DomainsBuffer,
                                       PLSAPR_TRANSLATED_SID_EX2 SidsBuffer,
                                       PULONG Mapped)
{
    UNICODE_STRING EmptyDomainName = RTL_CONSTANT_STRING(L"");
    PWELL_KNOWN_SID ptr, ptr2;
    PSID DomainSid;
    ULONG DomainIndex;
    ULONG i;
    NTSTATUS Status = STATUS_SUCCESS;

    for (i = 0; i < Count; i++)
    {
        /* Ignore names which were already mapped */
        if (SidsBuffer[i].Use != SidTypeUnknown)
            continue;

        /* Ignore isolated account names */
        if (DomainNames[i].Length == 0)
            continue;

        TRACE("Mapping name: %wZ\\%wZ\n", &DomainNames[i], &AccountNames[i]);

        /* Look-up all well-known names */
        ptr = LsapLookupFullyQualifiedWellKnownName((PUNICODE_STRING)&AccountNames[i],
                                                    (PUNICODE_STRING)&DomainNames[i]);
        if (ptr != NULL)
        {
            TRACE("Found it! (%wZ\\%wZ)\n", &ptr->DomainName, &ptr->AccountName);

            SidsBuffer[i].Use = ptr->Use;
            SidsBuffer[i].Sid = LsapCopySid(ptr->Sid);
            if (SidsBuffer[i].Sid == NULL)
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto done;
            }

            SidsBuffer[i].DomainIndex = -1;
            SidsBuffer[i].Flags = 0;

            if (ptr->Use == SidTypeDomain)
            {
                Status = LsapAddDomainToDomainsList(DomainsBuffer,
                                                    &ptr->AccountName,
                                                    ptr->Sid,
                                                    &DomainIndex);
                if (!NT_SUCCESS(Status))
                    goto done;

                SidsBuffer[i].DomainIndex = DomainIndex;
            }
            else
            {
                ptr2= LsapLookupIsolatedWellKnownName(&ptr->DomainName);
                if (ptr2 != NULL)
                {
                    Status = LsapAddDomainToDomainsList(DomainsBuffer,
                                                        &ptr2->AccountName,
                                                        ptr2->Sid,
                                                        &DomainIndex);
                    if (!NT_SUCCESS(Status))
                        goto done;

                    SidsBuffer[i].DomainIndex = DomainIndex;
                }
                else
                {
                    DomainSid = CreateDomainSidFromAccountSid(ptr->Sid);
                    if (DomainSid == NULL)
                    {
                        Status = STATUS_INSUFFICIENT_RESOURCES;
                        goto done;
                    }

                    Status = LsapAddDomainToDomainsList(DomainsBuffer,
                                                        &EmptyDomainName,
                                                        DomainSid,
                                                        &DomainIndex);

                    if (DomainSid != NULL)
                    {
                        MIDL_user_free(DomainSid);
                        DomainSid = NULL;
                    }

                    if (!NT_SUCCESS(Status))
                        goto done;

                    SidsBuffer[i].DomainIndex = DomainIndex;
                }
            }

            (*Mapped)++;
            continue;
        }
    }

done:
    return Status;
}


static
NTSTATUS
LsapLookupBuiltinNames(DWORD Count,
                       PRPC_UNICODE_STRING DomainNames,
                       PRPC_UNICODE_STRING AccountNames,
                       PLSAPR_REFERENCED_DOMAIN_LIST DomainsBuffer,
                       PLSAPR_TRANSLATED_SID_EX2 SidsBuffer,
                       PULONG Mapped)
{
    SAMPR_HANDLE ServerHandle = NULL;
    SAMPR_HANDLE DomainHandle = NULL;
    SAMPR_ULONG_ARRAY RelativeIds = {0, NULL};
    SAMPR_ULONG_ARRAY Use = {0, NULL};
    ULONG DomainIndex;
    ULONG i;
    NTSTATUS Status = STATUS_SUCCESS;

    Status = SamrConnect(NULL,
                         &ServerHandle,
                         SAM_SERVER_CONNECT | SAM_SERVER_LOOKUP_DOMAIN);
    if (!NT_SUCCESS(Status))
    {
        TRACE("SamrConnect failed (Status %08lx)\n", Status);
        goto done;
    }

    Status = SamrOpenDomain(ServerHandle,
                            DOMAIN_LOOKUP,
                            BuiltinDomainSid,
                            &DomainHandle);
    if (!NT_SUCCESS(Status))
    {
        TRACE("SamOpenDomain failed (Status %08lx)\n", Status);
        goto done;
    }

    for (i = 0; i < Count; i++)
    {
        /* Ignore names which were already mapped */
        if (SidsBuffer[i].Use != SidTypeUnknown)
            continue;

        /* Ignore isolated account names */
        if (DomainNames[i].Length == 0)
            continue;

        if (!RtlEqualUnicodeString((PUNICODE_STRING)&DomainNames[i], &BuiltinDomainName, TRUE))
            continue;

        TRACE("Mapping name: %wZ\\%wZ\n", &DomainNames[i], &AccountNames[i]);

        Status = SamrLookupNamesInDomain(DomainHandle,
                                         1,
                                         &AccountNames[i],
                                         &RelativeIds,
                                         &Use);
        if (NT_SUCCESS(Status))
        {
            SidsBuffer[i].Use = Use.Element[0];
            SidsBuffer[i].Sid = CreateSidFromSidAndRid(BuiltinDomainSid,
                                                       RelativeIds.Element[0]);
            if (SidsBuffer[i].Sid == NULL)
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto done;
            }

            SidsBuffer[i].DomainIndex = -1;
            SidsBuffer[i].Flags = 0;

            Status = LsapAddDomainToDomainsList(DomainsBuffer,
                                                &BuiltinDomainName,
                                                BuiltinDomainSid,
                                                &DomainIndex);
            if (!NT_SUCCESS(Status))
                goto done;

            SidsBuffer[i].DomainIndex = DomainIndex;

            (*Mapped)++;
        }

        SamIFree_SAMPR_ULONG_ARRAY(&RelativeIds);
        SamIFree_SAMPR_ULONG_ARRAY(&Use);
    }

done:
    if (DomainHandle != NULL)
        SamrCloseHandle(&DomainHandle);

    if (ServerHandle != NULL)
        SamrCloseHandle(&ServerHandle);

    return Status;
}


static
NTSTATUS
LsapLookupAccountNames(DWORD Count,
                       PRPC_UNICODE_STRING DomainNames,
                       PRPC_UNICODE_STRING AccountNames,
                       PLSAPR_REFERENCED_DOMAIN_LIST DomainsBuffer,
                       PLSAPR_TRANSLATED_SID_EX2 SidsBuffer,
                       PULONG Mapped)
{
    SAMPR_HANDLE ServerHandle = NULL;
    SAMPR_HANDLE DomainHandle = NULL;
    SAMPR_ULONG_ARRAY RelativeIds = {0, NULL};
    SAMPR_ULONG_ARRAY Use = {0, NULL};
    ULONG DomainIndex;
    ULONG i;
    NTSTATUS Status = STATUS_SUCCESS;

    Status = SamrConnect(NULL,
                         &ServerHandle,
                         SAM_SERVER_CONNECT | SAM_SERVER_LOOKUP_DOMAIN);
    if (!NT_SUCCESS(Status))
    {
        TRACE("SamrConnect failed (Status %08lx)\n", Status);
        goto done;
    }

    Status = SamrOpenDomain(ServerHandle,
                            DOMAIN_LOOKUP,
                            AccountDomainSid,
                            &DomainHandle);
    if (!NT_SUCCESS(Status))
    {
        TRACE("SamOpenDomain failed (Status %08lx)\n", Status);
        goto done;
    }

    for (i = 0; i < Count; i++)
    {
        /* Ignore names which were already mapped */
        if (SidsBuffer[i].Use != SidTypeUnknown)
            continue;

        /* Ignore isolated account names */
        if (DomainNames[i].Length == 0)
            continue;

        if (!RtlEqualUnicodeString((PUNICODE_STRING)&DomainNames[i], &AccountDomainName, TRUE))
            continue;

        TRACE("Mapping name: %wZ\\%wZ\n", &DomainNames[i], &AccountNames[i]);

        Status = SamrLookupNamesInDomain(DomainHandle,
                                         1,
                                         &AccountNames[i],
                                         &RelativeIds,
                                         &Use);
        if (NT_SUCCESS(Status))
        {
            SidsBuffer[i].Use = Use.Element[0];
            SidsBuffer[i].Sid = CreateSidFromSidAndRid(AccountDomainSid,
                                                       RelativeIds.Element[0]);
            if (SidsBuffer[i].Sid == NULL)
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto done;
            }

            SidsBuffer[i].DomainIndex = -1;
            SidsBuffer[i].Flags = 0;

            Status = LsapAddDomainToDomainsList(DomainsBuffer,
                                                &AccountDomainName,
                                                AccountDomainSid,
                                                &DomainIndex);
            if (!NT_SUCCESS(Status))
                goto done;

            SidsBuffer[i].DomainIndex = DomainIndex;

            (*Mapped)++;
        }

        SamIFree_SAMPR_ULONG_ARRAY(&RelativeIds);
        SamIFree_SAMPR_ULONG_ARRAY(&Use);
    }

done:
    if (DomainHandle != NULL)
        SamrCloseHandle(&DomainHandle);

    if (ServerHandle != NULL)
        SamrCloseHandle(&ServerHandle);

    return Status;
}


NTSTATUS
LsapLookupNames(DWORD Count,
                PRPC_UNICODE_STRING Names,
                PLSAPR_REFERENCED_DOMAIN_LIST *ReferencedDomains,
                PLSAPR_TRANSLATED_SIDS_EX2 TranslatedSids,
                LSAP_LOOKUP_LEVEL LookupLevel,
                DWORD *MappedCount,
                DWORD LookupOptions,
                DWORD ClientRevision)
{
    PLSAPR_REFERENCED_DOMAIN_LIST DomainsBuffer = NULL;
    PLSAPR_TRANSLATED_SID_EX2 SidsBuffer = NULL;
    PRPC_UNICODE_STRING DomainNames = NULL;
    PRPC_UNICODE_STRING AccountNames = NULL;
    ULONG SidsBufferLength;
    ULONG i;
    ULONG Mapped = 0;
    NTSTATUS Status = STATUS_SUCCESS;

//TRACE("()\n");

    TranslatedSids->Entries = 0;
    TranslatedSids->Sids = NULL;
    *ReferencedDomains = NULL;

    SidsBufferLength = Count * sizeof(LSAPR_TRANSLATED_SID_EX2);
    SidsBuffer = MIDL_user_allocate(SidsBufferLength);
    if (SidsBuffer == NULL)
    {
//TRACE("\n");
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto done;
    }

    DomainsBuffer = MIDL_user_allocate(sizeof(LSAPR_REFERENCED_DOMAIN_LIST));
    if (DomainsBuffer == NULL)
    {
//TRACE("\n");
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto done;
    }

    DomainsBuffer->Domains = MIDL_user_allocate(Count * sizeof(LSA_TRUST_INFORMATION));
    if (DomainsBuffer->Domains == NULL)
    {
//TRACE("\n");
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto done;
    }
    DomainsBuffer->Entries = 0;
    DomainsBuffer->MaxEntries = Count;

    for (i = 0; i < Count; i++)
    {
        SidsBuffer[i].Use = SidTypeUnknown;
        SidsBuffer[i].Sid = NULL;
        SidsBuffer[i].DomainIndex = -1;
        SidsBuffer[i].Flags = 0;
    }

    Status = LsapSplitNames(Count,
                            Names,
                            &DomainNames,
                            &AccountNames);
    if (!NT_SUCCESS(Status))
    {
        TRACE("LsapSplitNames failed! (Status %lx)\n", Status);
        goto done;
    }


    Status = LsapLookupIsolatedNames(Count,
                                     DomainNames,
                                     AccountNames,
                                     DomainsBuffer,
                                     SidsBuffer,
                                     &Mapped);
    if (!NT_SUCCESS(Status) &&
        Status != STATUS_NONE_MAPPED &&
        Status != STATUS_SOME_NOT_MAPPED)
    {
        TRACE("LsapLookupIsolatedNames failed! (Status %lx)\n", Status);
        goto done;
    }

    if (Mapped == Count)
        goto done;


    Status = LsapLookupIsolatedBuiltinNames(Count,
                                            DomainNames,
                                            AccountNames,
                                            DomainsBuffer,
                                            SidsBuffer,
                                            &Mapped);
    if (!NT_SUCCESS(Status) &&
        Status != STATUS_NONE_MAPPED &&
        Status != STATUS_SOME_NOT_MAPPED)
    {
        TRACE("LsapLookupIsolatedBuiltinNames failed! (Status %lx)\n", Status);
        goto done;
    }

    if (Mapped == Count)
        goto done;


    Status = LsapLookupIsolatedAccountNames(Count,
                                            DomainNames,
                                            AccountNames,
                                            DomainsBuffer,
                                            SidsBuffer,
                                            &Mapped);
    if (!NT_SUCCESS(Status) &&
        Status != STATUS_NONE_MAPPED &&
        Status != STATUS_SOME_NOT_MAPPED)
    {
        TRACE("LsapLookupIsolatedAccountNames failed! (Status %lx)\n", Status);
        goto done;
    }

    if (Mapped == Count)
        goto done;

    Status = LsapLookupFullyQualifiedWellKnownNames(Count,
                                                    DomainNames,
                                                    AccountNames,
                                                    DomainsBuffer,
                                                    SidsBuffer,
                                                    &Mapped);
    if (!NT_SUCCESS(Status) &&
        Status != STATUS_NONE_MAPPED &&
        Status != STATUS_SOME_NOT_MAPPED)
    {
        TRACE("LsapLookupFullyQualifiedWellKnownNames failed! (Status %lx)\n", Status);
        goto done;
    }

    if (Mapped == Count)
        goto done;

    Status = LsapLookupBuiltinNames(Count,
                                    DomainNames,
                                    AccountNames,
                                    DomainsBuffer,
                                    SidsBuffer,
                                    &Mapped);
    if (!NT_SUCCESS(Status) &&
        Status != STATUS_NONE_MAPPED &&
        Status != STATUS_SOME_NOT_MAPPED)
    {
        TRACE("LsapLookupBuiltinNames failed! (Status %lx)\n", Status);
        goto done;
    }

    if (Mapped == Count)
        goto done;


    Status = LsapLookupAccountNames(Count,
                                    DomainNames,
                                    AccountNames,
                                    DomainsBuffer,
                                    SidsBuffer,
                                    &Mapped);
    if (!NT_SUCCESS(Status) &&
        Status != STATUS_NONE_MAPPED &&
        Status != STATUS_SOME_NOT_MAPPED)
    {
        TRACE("LsapLookupAccountNames failed! (Status %lx)\n", Status);
        goto done;
    }

    if (Mapped == Count)
        goto done;

done:
//    TRACE("done: Status %lx\n", Status);

    if (DomainNames != NULL)
    {
//TRACE("Free DomainNames\n");
        for (i = 0; i < Count; i++)
        {
            if (DomainNames[i].Buffer != NULL)
                MIDL_user_free(DomainNames[i].Buffer);
        }

        MIDL_user_free(DomainNames);
    }

    if (AccountNames != NULL)
    {
//TRACE("Free AccountNames\n");
        for (i = 0; i < Count; i++)
        {
//TRACE("i: %lu\n", i);
            if (AccountNames[i].Buffer != NULL)
            {
                MIDL_user_free(AccountNames[i].Buffer);
            }
        }

        MIDL_user_free(AccountNames);
    }

    if (!NT_SUCCESS(Status))
    {
//TRACE("Failure!\n");

//TRACE("Free DomainsBuffer\n");
        if (DomainsBuffer != NULL)
        {
            if (DomainsBuffer->Domains != NULL)
                MIDL_user_free(DomainsBuffer->Domains);

            MIDL_user_free(DomainsBuffer);
        }

//TRACE("Free SidsBuffer\n");
        if (SidsBuffer != NULL)
            MIDL_user_free(SidsBuffer);
    }
    else
    {
//TRACE("Success!\n");

        *ReferencedDomains = DomainsBuffer;
        TranslatedSids->Entries = Count;
        TranslatedSids->Sids = SidsBuffer;
        *MappedCount = Mapped;

        if (Mapped == 0)
            Status = STATUS_NONE_MAPPED;
        else if (Mapped < Count)
            Status = STATUS_SOME_NOT_MAPPED;
    }

//    TRACE("done: Status %lx\n", Status);

    return Status;
}


static NTSTATUS
LsapLookupWellKnownSids(PLSAPR_SID_ENUM_BUFFER SidEnumBuffer,
                        PLSAPR_TRANSLATED_NAME_EX NamesBuffer,
                        PLSAPR_REFERENCED_DOMAIN_LIST DomainsBuffer,
                        PULONG Mapped)
{
    PWELL_KNOWN_SID ptr, ptr2;
    LPWSTR SidString = NULL;
    ULONG DomainIndex;
    ULONG i;
    NTSTATUS Status = STATUS_SUCCESS;

    for (i = 0; i < SidEnumBuffer->Entries; i++)
    {
        /* Ignore SIDs which are already mapped */
        if (NamesBuffer[i].Use != SidTypeUnknown)
            continue;

        ConvertSidToStringSidW(SidEnumBuffer->SidInfo[i].Sid, &SidString);
        TRACE("Mapping SID: %S\n", SidString);
        LocalFree(SidString);
        SidString = NULL;

        ptr = LsapLookupWellKnownSid(SidEnumBuffer->SidInfo[i].Sid);
        if (ptr != NULL)
        {
            NamesBuffer[i].Use = ptr->Use;
            NamesBuffer[i].Flags = 0;

            NamesBuffer[i].Name.Length = ptr->AccountName.Length;
            NamesBuffer[i].Name.MaximumLength = ptr->AccountName.MaximumLength;
            NamesBuffer[i].Name.Buffer = MIDL_user_allocate(ptr->AccountName.MaximumLength);
            if (NamesBuffer[i].Name.Buffer == NULL)
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto done;
            }

            RtlCopyMemory(NamesBuffer[i].Name.Buffer, ptr->AccountName.Buffer, ptr->AccountName.MaximumLength);

            if (ptr->DomainName.Length == 0)
            {
                Status = LsapAddAuthorityToDomainsList(DomainsBuffer,
                                                       SidEnumBuffer->SidInfo[i].Sid,
                                                       &DomainIndex);
                if (!NT_SUCCESS(Status))
                    goto done;

                NamesBuffer[i].DomainIndex = DomainIndex;
            }
            else
            {
                ptr2= LsapLookupIsolatedWellKnownName(&ptr->DomainName);
                if (ptr2 != NULL)
                {
                    Status = LsapAddDomainToDomainsList(DomainsBuffer,
                                                        &ptr2->AccountName,
                                                        ptr2->Sid,
                                                        &DomainIndex);
                    if (!NT_SUCCESS(Status))
                        goto done;

                    NamesBuffer[i].DomainIndex = DomainIndex;
                }
            }

            TRACE("Mapped to: %wZ\n", &NamesBuffer[i].Name);

            (*Mapped)++;
        }
    }

done:
    return Status;
}


static NTSTATUS
LsapLookupBuiltinDomainSids(PLSAPR_SID_ENUM_BUFFER SidEnumBuffer,
                            PLSAPR_TRANSLATED_NAME_EX NamesBuffer,
                            PLSAPR_REFERENCED_DOMAIN_LIST DomainsBuffer,
                            PULONG Mapped)
{
    SAMPR_HANDLE ServerHandle = NULL;
    SAMPR_HANDLE DomainHandle = NULL;
    SAMPR_RETURNED_USTRING_ARRAY Names = {0, NULL};
    SAMPR_ULONG_ARRAY Use = {0, NULL};
    LPWSTR SidString = NULL;
    ULONG DomainIndex;
    ULONG RelativeIds[1];
    ULONG i;
    NTSTATUS Status = STATUS_SUCCESS;

    Status = SamrConnect(NULL,
                         &ServerHandle,
                         SAM_SERVER_CONNECT | SAM_SERVER_LOOKUP_DOMAIN);
    if (!NT_SUCCESS(Status))
    {
        TRACE("SamrConnect failed (Status %08lx)\n", Status);
        goto done;
    }

    Status = SamrOpenDomain(ServerHandle,
                            DOMAIN_LOOKUP,
                            BuiltinDomainSid,
                            &DomainHandle);
    if (!NT_SUCCESS(Status))
    {
        TRACE("SamOpenDomain failed (Status %08lx)\n", Status);
        goto done;
    }

    for (i = 0; i < SidEnumBuffer->Entries; i++)
    {
        /* Ignore SIDs which are already mapped */
        if (NamesBuffer[i].Use != SidTypeUnknown)
            continue;

        ConvertSidToStringSidW(SidEnumBuffer->SidInfo[i].Sid, &SidString);
        TRACE("Mapping SID: %S\n", SidString);
        LocalFree(SidString);
        SidString = NULL;

        if (RtlEqualSid(BuiltinDomainSid, SidEnumBuffer->SidInfo[i].Sid))
        {
            TRACE("Found builtin domain!\n");

            NamesBuffer[i].Use = SidTypeDomain;
            NamesBuffer[i].Flags = 0;

            NamesBuffer[i].Name.Length = BuiltinDomainName.Length;
            NamesBuffer[i].Name.MaximumLength = BuiltinDomainName.MaximumLength;
            NamesBuffer[i].Name.Buffer = MIDL_user_allocate(BuiltinDomainName.MaximumLength);
            if (NamesBuffer[i].Name.Buffer == NULL)
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto done;
            }

            RtlCopyMemory(NamesBuffer[i].Name.Buffer, BuiltinDomainName.Buffer, BuiltinDomainName.MaximumLength);

            Status = LsapAddDomainToDomainsList(DomainsBuffer,
                                                &BuiltinDomainName,
                                                BuiltinDomainSid,
                                                &DomainIndex);
            if (!NT_SUCCESS(Status))
                goto done;

            NamesBuffer[i].DomainIndex = DomainIndex;

            TRACE("Mapped to: %wZ\n", &NamesBuffer[i].Name);

            (*Mapped)++;
        }
        else if (LsapIsPrefixSid(BuiltinDomainSid, SidEnumBuffer->SidInfo[i].Sid))
        {
            TRACE("Found builtin domain account!\n");

            RelativeIds[0] = LsapGetRelativeIdFromSid(SidEnumBuffer->SidInfo[i].Sid);

            Status = SamrLookupIdsInDomain(DomainHandle,
                                           1,
                                           RelativeIds,
                                           &Names,
                                           &Use);
            if (NT_SUCCESS(Status))
            {
                NamesBuffer[i].Use = Use.Element[0];
                NamesBuffer[i].Flags = 0;

                NamesBuffer[i].Name.Length = Names.Element[0].Length;
                NamesBuffer[i].Name.MaximumLength = Names.Element[0].MaximumLength;
                NamesBuffer[i].Name.Buffer = MIDL_user_allocate(Names.Element[0].MaximumLength);
                if (NamesBuffer[i].Name.Buffer == NULL)
                {
                    SamIFree_SAMPR_RETURNED_USTRING_ARRAY(&Names);
                    SamIFree_SAMPR_ULONG_ARRAY(&Use);

                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    goto done;
                }

                RtlCopyMemory(NamesBuffer[i].Name.Buffer,
                              Names.Element[0].Buffer,
                              Names.Element[0].MaximumLength);

                SamIFree_SAMPR_RETURNED_USTRING_ARRAY(&Names);
                SamIFree_SAMPR_ULONG_ARRAY(&Use);

                Status = LsapAddDomainToDomainsList(DomainsBuffer,
                                                    &BuiltinDomainName,
                                                    BuiltinDomainSid,
                                                    &DomainIndex);
                if (!NT_SUCCESS(Status))
                    goto done;

                NamesBuffer[i].DomainIndex = DomainIndex;

                TRACE("Mapped to: %wZ\n", &NamesBuffer[i].Name);

                (*Mapped)++;
            }
        }
    }

done:
    if (DomainHandle != NULL)
        SamrCloseHandle(&DomainHandle);

    if (ServerHandle != NULL)
        SamrCloseHandle(&ServerHandle);

    return Status;
}


static NTSTATUS
LsapLookupAccountDomainSids(PLSAPR_SID_ENUM_BUFFER SidEnumBuffer,
                            PLSAPR_TRANSLATED_NAME_EX NamesBuffer,
                            PLSAPR_REFERENCED_DOMAIN_LIST DomainsBuffer,
                            PULONG Mapped)
{
    SAMPR_HANDLE ServerHandle = NULL;
    SAMPR_HANDLE DomainHandle = NULL;
    SAMPR_RETURNED_USTRING_ARRAY Names = {0, NULL};
    SAMPR_ULONG_ARRAY Use = {0, NULL};
    LPWSTR SidString = NULL;
    ULONG DomainIndex;
    ULONG RelativeIds[1];
    ULONG i;
    NTSTATUS Status = STATUS_SUCCESS;

    Status = SamrConnect(NULL,
                         &ServerHandle,
                         SAM_SERVER_CONNECT | SAM_SERVER_LOOKUP_DOMAIN);
    if (!NT_SUCCESS(Status))
    {
        TRACE("SamrConnect failed (Status %08lx)\n", Status);
        goto done;
    }

    Status = SamrOpenDomain(ServerHandle,
                            DOMAIN_LOOKUP,
                            AccountDomainSid,
                            &DomainHandle);
    if (!NT_SUCCESS(Status))
    {
        TRACE("SamOpenDomain failed (Status %08lx)\n", Status);
        goto done;
    }

    for (i = 0; i < SidEnumBuffer->Entries; i++)
    {
        /* Ignore SIDs which are already mapped */
        if (NamesBuffer[i].Use != SidTypeUnknown)
            continue;

        ConvertSidToStringSidW(SidEnumBuffer->SidInfo[i].Sid, &SidString);
        TRACE("Mapping SID: %S\n", SidString);
        LocalFree(SidString);
        SidString = NULL;

        if (RtlEqualSid(AccountDomainSid, SidEnumBuffer->SidInfo[i].Sid))
        {
            TRACE("Found account domain!\n");

            NamesBuffer[i].Use = SidTypeDomain;
            NamesBuffer[i].Flags = 0;

            NamesBuffer[i].Name.Length = AccountDomainName.Length;
            NamesBuffer[i].Name.MaximumLength = AccountDomainName.MaximumLength;
            NamesBuffer[i].Name.Buffer = MIDL_user_allocate(AccountDomainName.MaximumLength);
            if (NamesBuffer[i].Name.Buffer == NULL)
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto done;
            }

            RtlCopyMemory(NamesBuffer[i].Name.Buffer, AccountDomainName.Buffer, AccountDomainName.MaximumLength);

            Status = LsapAddDomainToDomainsList(DomainsBuffer,
                                                &AccountDomainName,
                                                AccountDomainSid,
                                                &DomainIndex);
            if (!NT_SUCCESS(Status))
                goto done;

            NamesBuffer[i].DomainIndex = DomainIndex;

            TRACE("Mapped to: %wZ\n", &NamesBuffer[i].Name);

            (*Mapped)++;
        }
        else if (LsapIsPrefixSid(AccountDomainSid, SidEnumBuffer->SidInfo[i].Sid))
        {
            TRACE("Found account domain account!\n");

            RelativeIds[0] = LsapGetRelativeIdFromSid(SidEnumBuffer->SidInfo[i].Sid);

            Status = SamrLookupIdsInDomain(DomainHandle,
                                           1,
                                           RelativeIds,
                                           &Names,
                                           &Use);
            if (NT_SUCCESS(Status))
            {
                NamesBuffer[i].Use = Use.Element[0];
                NamesBuffer[i].Flags = 0;

                NamesBuffer[i].Name.Length = Names.Element[0].Length;
                NamesBuffer[i].Name.MaximumLength = Names.Element[0].MaximumLength;
                NamesBuffer[i].Name.Buffer = MIDL_user_allocate(Names.Element[0].MaximumLength);
                if (NamesBuffer[i].Name.Buffer == NULL)
                {
                    SamIFree_SAMPR_RETURNED_USTRING_ARRAY(&Names);
                    SamIFree_SAMPR_ULONG_ARRAY(&Use);

                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    goto done;
                }

                RtlCopyMemory(NamesBuffer[i].Name.Buffer,
                              Names.Element[0].Buffer,
                              Names.Element[0].MaximumLength);

                SamIFree_SAMPR_RETURNED_USTRING_ARRAY(&Names);
                SamIFree_SAMPR_ULONG_ARRAY(&Use);

                Status = LsapAddDomainToDomainsList(DomainsBuffer,
                                                    &AccountDomainName,
                                                    AccountDomainSid,
                                                    &DomainIndex);
                if (!NT_SUCCESS(Status))
                    goto done;

                NamesBuffer[i].DomainIndex = DomainIndex;

                TRACE("Mapped to: %wZ\n", &NamesBuffer[i].Name);

                (*Mapped)++;
            }
        }
    }

done:
    if (DomainHandle != NULL)
        SamrCloseHandle(&DomainHandle);

    if (ServerHandle != NULL)
        SamrCloseHandle(&ServerHandle);

    return Status;
}


NTSTATUS
LsapLookupSids(PLSAPR_SID_ENUM_BUFFER SidEnumBuffer,
               PLSAPR_REFERENCED_DOMAIN_LIST *ReferencedDomains,
               PLSAPR_TRANSLATED_NAMES_EX TranslatedNames,
               LSAP_LOOKUP_LEVEL LookupLevel,
               DWORD *MappedCount,
               DWORD LookupOptions,
               DWORD ClientRevision)
{
    PLSAPR_REFERENCED_DOMAIN_LIST DomainsBuffer = NULL;
    PLSAPR_TRANSLATED_NAME_EX NamesBuffer = NULL;
    ULONG NamesBufferLength;
    ULONG i;
    ULONG Mapped = 0;
    NTSTATUS Status = STATUS_SUCCESS;

    NamesBufferLength = SidEnumBuffer->Entries * sizeof(LSAPR_TRANSLATED_NAME_EX);
    NamesBuffer = MIDL_user_allocate(NamesBufferLength);
    if (NamesBuffer == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto done;
    }

    DomainsBuffer = MIDL_user_allocate(sizeof(LSAPR_REFERENCED_DOMAIN_LIST));
    if (DomainsBuffer == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto done;
    }

    DomainsBuffer->Domains = MIDL_user_allocate(SidEnumBuffer->Entries * sizeof(LSA_TRUST_INFORMATION));
    if (DomainsBuffer->Domains == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto done;
    }

    DomainsBuffer->Entries = 0;
    DomainsBuffer->MaxEntries = SidEnumBuffer->Entries;

    /* Initialize all name entries */
    for (i = 0; i < SidEnumBuffer->Entries; i++)
    {
        NamesBuffer[i].Use = SidTypeUnknown;
        NamesBuffer[i].Name.Length = 0;
        NamesBuffer[i].Name.MaximumLength = 0;
        NamesBuffer[i].Name.Buffer = NULL;
        NamesBuffer[i].DomainIndex = -1;
        NamesBuffer[i].Flags = 0;
    }

    /* Look-up well-known SIDs */
    Status = LsapLookupWellKnownSids(SidEnumBuffer,
                                     NamesBuffer,
                                     DomainsBuffer,
                                     &Mapped);
    if (!NT_SUCCESS(Status) &&
        Status != STATUS_NONE_MAPPED &&
        Status != STATUS_SOME_NOT_MAPPED)
        goto done;

    if (Mapped == SidEnumBuffer->Entries)
        goto done;

    /* Look-up builtin domain SIDs */
    Status = LsapLookupBuiltinDomainSids(SidEnumBuffer,
                                         NamesBuffer,
                                         DomainsBuffer,
                                         &Mapped);
    if (!NT_SUCCESS(Status) &&
        Status != STATUS_NONE_MAPPED &&
        Status != STATUS_SOME_NOT_MAPPED)
        goto done;

    if (Mapped == SidEnumBuffer->Entries)
        goto done;

    /* Look-up account domain SIDs */
    Status = LsapLookupAccountDomainSids(SidEnumBuffer,
                                         NamesBuffer,
                                         DomainsBuffer,
                                         &Mapped);
    if (!NT_SUCCESS(Status) &&
        Status != STATUS_NONE_MAPPED &&
        Status != STATUS_SOME_NOT_MAPPED)
        goto done;

    if (Mapped == SidEnumBuffer->Entries)
        goto done;

done:
    TRACE("done Status: %lx  Mapped: %lu\n", Status, Mapped);

    if (!NT_SUCCESS(Status))
    {
        if (DomainsBuffer != NULL)
        {
            if (DomainsBuffer->Domains != NULL)
                MIDL_user_free(DomainsBuffer->Domains);

            MIDL_user_free(DomainsBuffer);
        }

        if (NamesBuffer != NULL)
            MIDL_user_free(NamesBuffer);
    }
    else
    {
        *ReferencedDomains = DomainsBuffer;
        TranslatedNames->Entries = SidEnumBuffer->Entries;
        TranslatedNames->Names = NamesBuffer;
        *MappedCount = Mapped;

        if (Mapped == 0)
            Status = STATUS_NONE_MAPPED;
        else if (Mapped < SidEnumBuffer->Entries)
            Status = STATUS_SOME_NOT_MAPPED;
    }

    return Status;
}

/* EOF */
