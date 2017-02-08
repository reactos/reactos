/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         Local Security Authority (LSA) Server
 * FILE:            reactos/dll/win32/lsasrv/lookup.c
 * PURPOSE:         Sid / Name lookup functions
 *
 * PROGRAMMERS:     Eric Kohl
 */
#include "lsasrv.h"

WINE_DEFAULT_DEBUG_CHANNEL(lsasrv);

typedef wchar_t *PSAMPR_SERVER_NAME;
typedef void *SAMPR_HANDLE;

typedef struct _SAMPR_RETURNED_USTRING_ARRAY
{
    unsigned long Count;
    PRPC_UNICODE_STRING Element;
} SAMPR_RETURNED_USTRING_ARRAY, *PSAMPR_RETURNED_USTRING_ARRAY;

typedef struct _SAMPR_ULONG_ARRAY
{
    unsigned long Count;
    unsigned long *Element;
} SAMPR_ULONG_ARRAY, *PSAMPR_ULONG_ARRAY;


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

#if 0
typedef struct _AccountSid
{
    WELL_KNOWN_SID_TYPE type;
    LPCWSTR account;
    LPCWSTR domain;
    SID_NAME_USE name_use;
} AccountSid;

static const WCHAR Account_Operators[] = { 'A','c','c','o','u','n','t',' ','O','p','e','r','a','t','o','r','s',0 };
static const WCHAR Administrator[] = {'A','d','m','i','n','i','s','t','r','a','t','o','r',0 };
static const WCHAR Administrators[] = { 'A','d','m','i','n','i','s','t','r','a','t','o','r','s',0 };
static const WCHAR ANONYMOUS_LOGON[] = { 'A','N','O','N','Y','M','O','U','S',' ','L','O','G','O','N',0 };
static const WCHAR Authenticated_Users[] = { 'A','u','t','h','e','n','t','i','c','a','t','e','d',' ','U','s','e','r','s',0 };
static const WCHAR Backup_Operators[] = { 'B','a','c','k','u','p',' ','O','p','e','r','a','t','o','r','s',0 };
static const WCHAR BATCH[] = { 'B','A','T','C','H',0 };
static const WCHAR Blank[] = { 0 };
static const WCHAR BUILTIN[] = { 'B','U','I','L','T','I','N',0 };
static const WCHAR Cert_Publishers[] = { 'C','e','r','t',' ','P','u','b','l','i','s','h','e','r','s',0 };
static const WCHAR CREATOR_GROUP[] = { 'C','R','E','A','T','O','R',' ','G','R','O','U','P',0 };
static const WCHAR CREATOR_GROUP_SERVER[] = { 'C','R','E','A','T','O','R',' ','G','R','O','U','P',' ','S','E','R','V','E','R',0 };
static const WCHAR CREATOR_OWNER[] = { 'C','R','E','A','T','O','R',' ','O','W','N','E','R',0 };
static const WCHAR CREATOR_OWNER_SERVER[] = { 'C','R','E','A','T','O','R',' ','O','W','N','E','R',' ','S','E','R','V','E','R',0 };
static const WCHAR DIALUP[] = { 'D','I','A','L','U','P',0 };
static const WCHAR Digest_Authentication[] = { 'D','i','g','e','s','t',' ','A','u','t','h','e','n','t','i','c','a','t','i','o','n',0 };
static const WCHAR DOMAIN[] = {'D','O','M','A','I','N',0};
static const WCHAR Domain_Admins[] = { 'D','o','m','a','i','n',' ','A','d','m','i','n','s',0 };
static const WCHAR Domain_Computers[] = { 'D','o','m','a','i','n',' ','C','o','m','p','u','t','e','r','s',0 };
static const WCHAR Domain_Controllers[] = { 'D','o','m','a','i','n',' ','C','o','n','t','r','o','l','l','e','r','s',0 };
static const WCHAR Domain_Guests[] = { 'D','o','m','a','i','n',' ','G','u','e','s','t','s',0 };
static const WCHAR Domain_Users[] = { 'D','o','m','a','i','n',' ','U','s','e','r','s',0 };
static const WCHAR Enterprise_Admins[] = { 'E','n','t','e','r','p','r','i','s','e',' ','A','d','m','i','n','s',0 };
static const WCHAR ENTERPRISE_DOMAIN_CONTROLLERS[] = { 'E','N','T','E','R','P','R','I','S','E',' ','D','O','M','A','I','N',' ','C','O','N','T','R','O','L','L','E','R','S',0 };
static const WCHAR Everyone[] = { 'E','v','e','r','y','o','n','e',0 };
static const WCHAR Group_Policy_Creator_Owners[] = { 'G','r','o','u','p',' ','P','o','l','i','c','y',' ','C','r','e','a','t','o','r',' ','O','w','n','e','r','s',0 };
static const WCHAR Guest[] = { 'G','u','e','s','t',0 };
static const WCHAR Guests[] = { 'G','u','e','s','t','s',0 };
static const WCHAR INTERACTIVE[] = { 'I','N','T','E','R','A','C','T','I','V','E',0 };
static const WCHAR LOCAL[] = { 'L','O','C','A','L',0 };
static const WCHAR LOCAL_SERVICE[] = { 'L','O','C','A','L',' ','S','E','R','V','I','C','E',0 };
static const WCHAR NETWORK[] = { 'N','E','T','W','O','R','K',0 };
static const WCHAR Network_Configuration_Operators[] = { 'N','e','t','w','o','r','k',' ','C','o','n','f','i','g','u','r','a','t','i','o','n',' ','O','p','e','r','a','t','o','r','s',0 };
static const WCHAR NETWORK_SERVICE[] = { 'N','E','T','W','O','R','K',' ','S','E','R','V','I','C','E',0 };
static const WCHAR NT_AUTHORITY[] = { 'N','T',' ','A','U','T','H','O','R','I','T','Y',0 };
static const WCHAR NT_Pseudo_Domain[] = { 'N','T',' ','P','s','e','u','d','o',' ','D','o','m','a','i','n',0 };
static const WCHAR NTML_Authentication[] = { 'N','T','M','L',' ','A','u','t','h','e','n','t','i','c','a','t','i','o','n',0 };
static const WCHAR NULL_SID[] = { 'N','U','L','L',' ','S','I','D',0 };
static const WCHAR Other_Organization[] = { 'O','t','h','e','r',' ','O','r','g','a','n','i','z','a','t','i','o','n',0 };
static const WCHAR Performance_Log_Users[] = { 'P','e','r','f','o','r','m','a','n','c','e',' ','L','o','g',' ','U','s','e','r','s',0 };
static const WCHAR Performance_Monitor_Users[] = { 'P','e','r','f','o','r','m','a','n','c','e',' ','M','o','n','i','t','o','r',' ','U','s','e','r','s',0 };
static const WCHAR Power_Users[] = { 'P','o','w','e','r',' ','U','s','e','r','s',0 };
static const WCHAR Pre_Windows_2000_Compatible_Access[] = { 'P','r','e','-','W','i','n','d','o','w','s',' ','2','0','0','0',' ','C','o','m','p','a','t','i','b','l','e',' ','A','c','c','e','s','s',0 };
static const WCHAR Print_Operators[] = { 'P','r','i','n','t',' ','O','p','e','r','a','t','o','r','s',0 };
static const WCHAR PROXY[] = { 'P','R','O','X','Y',0 };
static const WCHAR RAS_and_IAS_Servers[] = { 'R','A','S',' ','a','n','d',' ','I','A','S',' ','S','e','r','v','e','r','s',0 };
static const WCHAR Remote_Desktop_Users[] = { 'R','e','m','o','t','e',' ','D','e','s','k','t','o','p',' ','U','s','e','r','s',0 };
static const WCHAR REMOTE_INTERACTIVE_LOGON[] = { 'R','E','M','O','T','E',' ','I','N','T','E','R','A','C','T','I','V','E',' ','L','O','G','O','N',0 };
static const WCHAR Replicators[] = { 'R','e','p','l','i','c','a','t','o','r','s',0 };
static const WCHAR RESTRICTED[] = { 'R','E','S','T','R','I','C','T','E','D',0 };
static const WCHAR SChannel_Authentication[] = { 'S','C','h','a','n','n','e','l',' ','A','u','t','h','e','n','t','i','c','a','t','i','o','n',0 };
static const WCHAR Schema_Admins[] = { 'S','c','h','e','m','a',' ','A','d','m','i','n','s',0 };
static const WCHAR SELF[] = { 'S','E','L','F',0 };
static const WCHAR Server_Operators[] = { 'S','e','r','v','e','r',' ','O','p','e','r','a','t','o','r','s',0 };
static const WCHAR SERVICE[] = { 'S','E','R','V','I','C','E',0 };
static const WCHAR SYSTEM[] = { 'S','Y','S','T','E','M',0 };
static const WCHAR TERMINAL_SERVER_USER[] = { 'T','E','R','M','I','N','A','L',' ','S','E','R','V','E','R',' ','U','S','E','R',0 };
static const WCHAR This_Organization[] = { 'T','h','i','s',' ','O','r','g','a','n','i','z','a','t','i','o','n',0 };
static const WCHAR Users[] = { 'U','s','e','r','s',0 };

static const AccountSid ACCOUNT_SIDS[] = {
//    { WinNullSid, NULL_SID, Blank, SidTypeWellKnownGroup },
//    { WinWorldSid, Everyone, Blank, SidTypeWellKnownGroup },
//    { WinLocalSid, LOCAL, Blank, SidTypeWellKnownGroup },
//    { WinCreatorOwnerSid, CREATOR_OWNER, Blank, SidTypeWellKnownGroup },
//    { WinCreatorGroupSid, CREATOR_GROUP, Blank, SidTypeWellKnownGroup },
//    { WinCreatorOwnerServerSid, CREATOR_OWNER_SERVER, Blank, SidTypeWellKnownGroup },
//    { WinCreatorGroupServerSid, CREATOR_GROUP_SERVER, Blank, SidTypeWellKnownGroup },
//    { WinNtAuthoritySid, NT_Pseudo_Domain, NT_Pseudo_Domain, SidTypeDomain },
//    { WinDialupSid, DIALUP, NT_AUTHORITY, SidTypeWellKnownGroup },
//    { WinNetworkSid, NETWORK, NT_AUTHORITY, SidTypeWellKnownGroup },
//    { WinBatchSid, BATCH, NT_AUTHORITY, SidTypeWellKnownGroup },
//    { WinInteractiveSid, INTERACTIVE, NT_AUTHORITY, SidTypeWellKnownGroup },
//    { WinServiceSid, SERVICE, NT_AUTHORITY, SidTypeWellKnownGroup },
//    { WinAnonymousSid, ANONYMOUS_LOGON, NT_AUTHORITY, SidTypeWellKnownGroup },
//    { WinProxySid, PROXY, NT_AUTHORITY, SidTypeWellKnownGroup },
//    { WinEnterpriseControllersSid, ENTERPRISE_DOMAIN_CONTROLLERS, NT_AUTHORITY, SidTypeWellKnownGroup },
//    { WinSelfSid, SELF, NT_AUTHORITY, SidTypeWellKnownGroup },
//    { WinAuthenticatedUserSid, Authenticated_Users, NT_AUTHORITY, SidTypeWellKnownGroup },
//    { WinRestrictedCodeSid, RESTRICTED, NT_AUTHORITY, SidTypeWellKnownGroup },
//    { WinTerminalServerSid, TERMINAL_SERVER_USER, NT_AUTHORITY, SidTypeWellKnownGroup },
//    { WinRemoteLogonIdSid, REMOTE_INTERACTIVE_LOGON, NT_AUTHORITY, SidTypeWellKnownGroup },
//    { WinLocalSystemSid, SYSTEM, NT_AUTHORITY, SidTypeWellKnownGroup },
//    { WinLocalServiceSid, LOCAL_SERVICE, NT_AUTHORITY, SidTypeWellKnownGroup },
//    { WinNetworkServiceSid, NETWORK_SERVICE, NT_AUTHORITY, SidTypeWellKnownGroup },
//    { WinBuiltinDomainSid, BUILTIN, BUILTIN, SidTypeDomain },
//    { WinBuiltinAdministratorsSid, Administrators, BUILTIN, SidTypeAlias },
//    { WinBuiltinUsersSid, Users, BUILTIN, SidTypeAlias },
//    { WinBuiltinGuestsSid, Guests, BUILTIN, SidTypeAlias },
//    { WinBuiltinPowerUsersSid, Power_Users, BUILTIN, SidTypeAlias },
//    { WinBuiltinAccountOperatorsSid, Account_Operators, BUILTIN, SidTypeAlias },
//    { WinBuiltinSystemOperatorsSid, Server_Operators, BUILTIN, SidTypeAlias },
//    { WinBuiltinPrintOperatorsSid, Print_Operators, BUILTIN, SidTypeAlias },
//    { WinBuiltinBackupOperatorsSid, Backup_Operators, BUILTIN, SidTypeAlias },
//    { WinBuiltinReplicatorSid, Replicators, BUILTIN, SidTypeAlias },
//    { WinBuiltinPreWindows2000CompatibleAccessSid, Pre_Windows_2000_Compatible_Access, BUILTIN, SidTypeAlias },
//    { WinBuiltinRemoteDesktopUsersSid, Remote_Desktop_Users, BUILTIN, SidTypeAlias },
//    { WinBuiltinNetworkConfigurationOperatorsSid, Network_Configuration_Operators, BUILTIN, SidTypeAlias },
    { WinNTLMAuthenticationSid, NTML_Authentication, NT_AUTHORITY, SidTypeWellKnownGroup },
    { WinDigestAuthenticationSid, Digest_Authentication, NT_AUTHORITY, SidTypeWellKnownGroup },
    { WinSChannelAuthenticationSid, SChannel_Authentication, NT_AUTHORITY, SidTypeWellKnownGroup },
//    { WinThisOrganizationSid, This_Organization, NT_AUTHORITY, SidTypeWellKnownGroup },
    { WinOtherOrganizationSid, Other_Organization, NT_AUTHORITY, SidTypeWellKnownGroup },
    { WinBuiltinPerfMonitoringUsersSid, Performance_Monitor_Users, BUILTIN, SidTypeAlias },
    { WinBuiltinPerfLoggingUsersSid, Performance_Log_Users, BUILTIN, SidTypeAlias },
};
#endif


BOOLEAN
LsapCreateSid(PSID_IDENTIFIER_AUTHORITY IdentifierAuthority,
              UCHAR SubAuthorityCount,
              PULONG SubAuthorities,
              PWSTR AccountName,
              PWSTR DomainName,
              SID_NAME_USE Use)
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

    RtlInitUnicodeString(&SidEntry->AccountName,
                         AccountName);

    RtlInitUnicodeString(&SidEntry->DomainName,
                         DomainName);

    SidEntry->Use = Use;

    InsertTailList(&WellKnownSidListHead,
                   &SidEntry->ListEntry);

    return TRUE;
}


NTSTATUS
LsapInitSids(VOID)
{
    ULONG SubAuthorities[8];

    InitializeListHead(&WellKnownSidListHead);

    /* NT Authority */
    LsapCreateSid(&NtAuthority,
                  0,
                  NULL,
                  L"NT AUTHORITY",
                  L"NT AUTHORITY",
                  SidTypeDomain);

    /* Null Sid */
    SubAuthorities[0] = SECURITY_NULL_RID;
    LsapCreateSid(&NullSidAuthority,
                  1,
                  SubAuthorities,
                  L"NULL SID",
                  L"",
                  SidTypeWellKnownGroup);

    /* World Sid */
    SubAuthorities[0] = SECURITY_WORLD_RID;
    LsapCreateSid(&WorldSidAuthority,
                  1,
                  SubAuthorities,
                  L"Everyone",
                  L"",
                  SidTypeWellKnownGroup);

    /* Local Sid */
    SubAuthorities[0] = SECURITY_LOCAL_RID;
    LsapCreateSid(&LocalSidAuthority,
                  1,
                  SubAuthorities,
                  L"LOCAL",
                  L"",
                  SidTypeWellKnownGroup);

    /* Creator Owner Sid */
    SubAuthorities[0] = SECURITY_CREATOR_OWNER_RID;
    LsapCreateSid(&CreatorSidAuthority,
                  1,
                  SubAuthorities,
                  L"CREATOR OWNER",
                  L"",
                  SidTypeWellKnownGroup);

    /* Creator Group Sid */
    SubAuthorities[0] = SECURITY_CREATOR_GROUP_RID;
    LsapCreateSid(&CreatorSidAuthority,
                  1,
                  SubAuthorities,
                  L"CREATOR GROUP",
                  L"",
                  SidTypeWellKnownGroup);

    /* Creator Owner Server Sid */
    SubAuthorities[0] = SECURITY_CREATOR_OWNER_SERVER_RID;
    LsapCreateSid(&CreatorSidAuthority,
                  1,
                  SubAuthorities,
                  L"CREATOR OWNER SERVER",
                  L"",
                  SidTypeWellKnownGroup);

    /* Creator Group Server Sid */
    SubAuthorities[0] = SECURITY_CREATOR_GROUP_SERVER_RID;
    LsapCreateSid(&CreatorSidAuthority,
                  1,
                  SubAuthorities,
                  L"CREATOR GROUP SERVER",
                  L"",
                  SidTypeWellKnownGroup);

    /* Dialup Sid */
    SubAuthorities[0] = SECURITY_DIALUP_RID;
    LsapCreateSid(&NtAuthority,
                  1,
                  SubAuthorities,
                  L"DIALUP",
                  L"NT AUTHORITY",
                  SidTypeWellKnownGroup);

    /* Network Sid */
    SubAuthorities[0] = SECURITY_NETWORK_RID;
    LsapCreateSid(&NtAuthority,
                  1,
                  SubAuthorities,
                  L"NETWORK",
                  L"NT AUTHORITY",
                  SidTypeWellKnownGroup);

    /* Batch Sid*/
    SubAuthorities[0] = SECURITY_BATCH_RID;
    LsapCreateSid(&NtAuthority,
                  1,
                  SubAuthorities,
                  L"BATCH",
                  L"NT AUTHORITY",
                  SidTypeWellKnownGroup);

    /* Interactive Sid */
    SubAuthorities[0] = SECURITY_INTERACTIVE_RID;
    LsapCreateSid(&NtAuthority,
                  1,
                  SubAuthorities,
                  L"INTERACTIVE",
                  L"NT AUTHORITY",
                  SidTypeWellKnownGroup);

    /* Service Sid */
    SubAuthorities[0] = SECURITY_SERVICE_RID;
    LsapCreateSid(&NtAuthority,
                  1,
                  SubAuthorities,
                  L"SERVICE",
                  L"NT AUTHORITY",
                  SidTypeWellKnownGroup);

    /* Anonymous Logon Sid */
    SubAuthorities[0] = SECURITY_ANONYMOUS_LOGON_RID;
    LsapCreateSid(&NtAuthority,
                  1,
                  SubAuthorities,
                  L"ANONYMOUS LOGON",
                  L"NT AUTHORITY",
                  SidTypeWellKnownGroup);

    /* Proxy Sid */
    SubAuthorities[0] = SECURITY_PROXY_RID;
    LsapCreateSid(&NtAuthority,
                  1,
                  SubAuthorities,
                  L"PROXY",
                  L"NT AUTHORITY",
                  SidTypeWellKnownGroup);

    /* Enterprise Controllers Sid */
    SubAuthorities[0] = SECURITY_ENTERPRISE_CONTROLLERS_RID;
    LsapCreateSid(&NtAuthority,
                  1,
                  SubAuthorities,
                  L"ENTERPRISE DOMAIN CONTROLLERS",
                  L"NT AUTHORITY",
                  SidTypeWellKnownGroup);

    /* Principal Self Sid */
    SubAuthorities[0] = SECURITY_PRINCIPAL_SELF_RID;
    LsapCreateSid(&NtAuthority,
                  1,
                  SubAuthorities,
                  L"SELF",
                  L"NT AUTHORITY",
                  SidTypeWellKnownGroup);

    /* Authenticated Users Sid */
    SubAuthorities[0] = SECURITY_AUTHENTICATED_USER_RID;
    LsapCreateSid(&NtAuthority,
                  1,
                  SubAuthorities,
                  L"Authenticated Users",
                  L"NT AUTHORITY",
                  SidTypeWellKnownGroup);

    /* Restricted Code Sid */
    SubAuthorities[0] = SECURITY_RESTRICTED_CODE_RID;
    LsapCreateSid(&NtAuthority,
                  1,
                  SubAuthorities,
                  L"RESTRICTED",
                  L"NT AUTHORITY",
                  SidTypeWellKnownGroup);

    /* Terminal Server Sid */
    SubAuthorities[0] = SECURITY_TERMINAL_SERVER_RID;
    LsapCreateSid(&NtAuthority,
                  1,
                  SubAuthorities,
                  L"TERMINAL SERVER USER",
                  L"NT AUTHORITY",
                  SidTypeWellKnownGroup);

    /* Remote Logon Sid */
    SubAuthorities[0] = SECURITY_REMOTE_LOGON_RID;
    LsapCreateSid(&NtAuthority,
                  1,
                  SubAuthorities,
                  L"REMOTE INTERACTIVE LOGON",
                  L"NT AUTHORITY",
                  SidTypeWellKnownGroup);

    /* This Organization Sid */
    SubAuthorities[0] = SECURITY_THIS_ORGANIZATION_RID;
    LsapCreateSid(&NtAuthority,
                  1,
                  SubAuthorities,
                  L"This Organization",
                  L"NT AUTHORITY",
                  SidTypeWellKnownGroup);

    /* Local System Sid */
    SubAuthorities[0] = SECURITY_LOCAL_SYSTEM_RID;
    LsapCreateSid(&NtAuthority,
                  1,
                  SubAuthorities,
                  L"SYSTEM",
                  L"NT AUTHORITY",
                  SidTypeWellKnownGroup);

    /* Local Service Sid */
    SubAuthorities[0] = SECURITY_LOCAL_SERVICE_RID;
    LsapCreateSid(&NtAuthority,
                  1,
                  SubAuthorities,
                  L"LOCAL SERVICE",
                  L"NT AUTHORITY",
                  SidTypeWellKnownGroup);

    LsapCreateSid(&NtAuthority,
                  1,
                  SubAuthorities,
                  L"LOCALSERVICE",
                  L"NT AUTHORITY",
                  SidTypeWellKnownGroup);

    /* Network Service Sid */
    SubAuthorities[0] = SECURITY_NETWORK_SERVICE_RID;
    LsapCreateSid(&NtAuthority,
                  1,
                  SubAuthorities,
                  L"NETWORK SERVICE",
                  L"NT AUTHORITY",
                  SidTypeWellKnownGroup);

    LsapCreateSid(&NtAuthority,
                  1,
                  SubAuthorities,
                  L"NETWORKSERVICE",
                  L"NT AUTHORITY",
                  SidTypeWellKnownGroup);

    /* Builtin Domain Sid */
    SubAuthorities[0] = SECURITY_BUILTIN_DOMAIN_RID;
    LsapCreateSid(&NtAuthority,
                  1,
                  SubAuthorities,
                  L"BUILTIN",
                  L"BUILTIN",
                  SidTypeDomain);

    /* Administrators Alias Sid */
    SubAuthorities[0] = SECURITY_BUILTIN_DOMAIN_RID;
    SubAuthorities[1] = DOMAIN_ALIAS_RID_ADMINS;
    LsapCreateSid(&NtAuthority,
                  2,
                  SubAuthorities,
                  L"Administrators",
                  L"BUILTIN",
                  SidTypeAlias);

    /* Users Alias Sid */
    SubAuthorities[0] = SECURITY_BUILTIN_DOMAIN_RID;
    SubAuthorities[1] = DOMAIN_ALIAS_RID_USERS;
    LsapCreateSid(&NtAuthority,
                  2,
                  SubAuthorities,
                  L"Users",
                  L"BUILTIN",
                  SidTypeAlias);

    /* Guests Alias Sid */
    SubAuthorities[0] = SECURITY_BUILTIN_DOMAIN_RID;
    SubAuthorities[1] = DOMAIN_ALIAS_RID_GUESTS;
    LsapCreateSid(&NtAuthority,
                  2,
                  SubAuthorities,
                  L"Guests",
                  L"BUILTIN",
                  SidTypeAlias);

    /* Power User Alias Sid */
    SubAuthorities[0] = SECURITY_BUILTIN_DOMAIN_RID;
    SubAuthorities[1] = DOMAIN_ALIAS_RID_POWER_USERS;
    LsapCreateSid(&NtAuthority,
                  2,
                  SubAuthorities,
                  L"Power User",
                  L"BUILTIN",
                  SidTypeAlias);

    /* Account Operators Alias Sid */
    SubAuthorities[0] = SECURITY_BUILTIN_DOMAIN_RID;
    SubAuthorities[1] = DOMAIN_ALIAS_RID_ACCOUNT_OPS;
    LsapCreateSid(&NtAuthority,
                  2,
                  SubAuthorities,
                  L"Account Operators",
                  L"BUILTIN",
                  SidTypeAlias);

    /* System Operators Alias Sid */
    SubAuthorities[0] = SECURITY_BUILTIN_DOMAIN_RID;
    SubAuthorities[1] = DOMAIN_ALIAS_RID_SYSTEM_OPS;
    LsapCreateSid(&NtAuthority,
                  2,
                  SubAuthorities,
                  L"Server Operators",
                  L"BUILTIN",
                  SidTypeAlias);

    /* Print Operators Alias Sid */
    SubAuthorities[0] = SECURITY_BUILTIN_DOMAIN_RID;
    SubAuthorities[1] = DOMAIN_ALIAS_RID_PRINT_OPS;
    LsapCreateSid(&NtAuthority,
                  2,
                  SubAuthorities,
                  L"Print Operators",
                  L"BUILTIN",
                  SidTypeAlias);

    /* Backup Operators Alias Sid */
    SubAuthorities[0] = SECURITY_BUILTIN_DOMAIN_RID;
    SubAuthorities[1] = DOMAIN_ALIAS_RID_BACKUP_OPS;
    LsapCreateSid(&NtAuthority,
                  2,
                  SubAuthorities,
                  L"Backup Operators",
                  L"BUILTIN",
                  SidTypeAlias);

    /* Replicators Alias Sid */
    SubAuthorities[0] = SECURITY_BUILTIN_DOMAIN_RID;
    SubAuthorities[1] = DOMAIN_ALIAS_RID_REPLICATOR;
    LsapCreateSid(&NtAuthority,
                  2,
                  SubAuthorities,
                  L"Replicators",
                  L"BUILTIN",
                  SidTypeAlias);

#if 0
    /* RAS Servers Alias Sid */
    SubAuthorities[0] = SECURITY_BUILTIN_DOMAIN_RID;
    SubAuthorities[1] = DOMAIN_ALIAS_RID_RAS_SERVERS;
    LsapCreateSid(&NtAuthority,
                  2,
                  SubAuthorities,
                  L"Backup Operators",
                  L"BUILTIN",
                  SidTypeAlias);
#endif

    /* Pre-Windows 2000 Compatible Access Alias Sid */
    SubAuthorities[0] = SECURITY_BUILTIN_DOMAIN_RID;
    SubAuthorities[1] = DOMAIN_ALIAS_RID_PREW2KCOMPACCESS;
    LsapCreateSid(&NtAuthority,
                  2,
                  SubAuthorities,
                  L"Pre-Windows 2000 Compatible Access",
                  L"BUILTIN",
                  SidTypeAlias);

    /* Remote Desktop Users Alias Sid */
    SubAuthorities[0] = SECURITY_BUILTIN_DOMAIN_RID;
    SubAuthorities[1] = DOMAIN_ALIAS_RID_REMOTE_DESKTOP_USERS;
    LsapCreateSid(&NtAuthority,
                  2,
                  SubAuthorities,
                  L"Remote Desktop Users",
                  L"BUILTIN",
                  SidTypeAlias);

    /* Network Configuration Operators Alias Sid */
    SubAuthorities[0] = SECURITY_BUILTIN_DOMAIN_RID;
    SubAuthorities[1] = DOMAIN_ALIAS_RID_NETWORK_CONFIGURATION_OPS;
    LsapCreateSid(&NtAuthority,
                  2,
                  SubAuthorities,
                  L"Network Configuration Operators",
                  L"BUILTIN",
                  SidTypeAlias);

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
    if (ReferencedDomains->Domains[i].Sid == NULL)
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
