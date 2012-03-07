/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         Local Security Authority (LSA) Server
 * FILE:            reactos/dll/win32/lsasrv/sids.c
 * PURPOSE:         Sid / Name lookup functions
 *
 * PROGRAMMERS:     Eric Kohl
 */
#include "lsasrv.h"

WINE_DEFAULT_DEBUG_CHANNEL(lsasrv);


typedef struct _WELL_KNOWN_SID
{
    LIST_ENTRY ListEntry;
    PSID Sid;
    UNICODE_STRING Name;
    UNICODE_STRING Domain;
    SID_NAME_USE NameUse;
} WELL_KNOWN_SID, *PWELL_KNOWN_SID;


static SID_IDENTIFIER_AUTHORITY NullSidAuthority    = {SECURITY_NULL_SID_AUTHORITY};
static SID_IDENTIFIER_AUTHORITY WorldSidAuthority   = {SECURITY_WORLD_SID_AUTHORITY};
static SID_IDENTIFIER_AUTHORITY LocalSidAuthority   = {SECURITY_LOCAL_SID_AUTHORITY};
static SID_IDENTIFIER_AUTHORITY CreatorSidAuthority = {SECURITY_CREATOR_SID_AUTHORITY};
static SID_IDENTIFIER_AUTHORITY NtAuthority         = {SECURITY_NT_AUTHORITY};

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
              PWSTR Name,
              PWSTR Domain,
              SID_NAME_USE NameUse)
{
    PWELL_KNOWN_SID SidEntry;
    PULONG p;
    ULONG i;

    SidEntry = RtlAllocateHeap(RtlGetProcessHeap(), 0, sizeof(WELL_KNOWN_SID));
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

    RtlInitUnicodeString(&SidEntry->Name,
                         Name);

    RtlInitUnicodeString(&SidEntry->Domain,
                         Domain);

    SidEntry->NameUse = NameUse;

    InsertTailList(&WellKnownSidListHead,
                   &SidEntry->ListEntry);

    return TRUE;
}


NTSTATUS
LsapInitSids(VOID)
{
    ULONG SubAuthorities[5];

    InitializeListHead(&WellKnownSidListHead);

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

    /* NT Domain Sid */
    LsapCreateSid(&NtAuthority,
                  0,
                  NULL,
                  L"NT Pseudo Domain",
                  L"NT Pseudo Domain",
                  SidTypeDomain);

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

    /* Network Service Sid */
    SubAuthorities[0] = SECURITY_NETWORK_SERVICE_RID;
    LsapCreateSid(&NtAuthority,
                  1,
                  SubAuthorities,
                  L"NETWORK SERVICE",
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


NTSTATUS
LsapLookupSids(PLSAPR_SID_ENUM_BUFFER SidEnumBuffer,
               PLSAPR_TRANSLATED_NAME OutputNames)
{
    static const UNICODE_STRING UserName = RTL_CONSTANT_STRING(L"Administrator");
    PWELL_KNOWN_SID ptr;
    ULONG Mapped = 0;
    ULONG i;
    NTSTATUS Status;

    PSID *Sids = (PSID *) SidEnumBuffer->SidInfo;

TRACE("LsapLookupSids(%p, %p)\n", SidEnumBuffer, OutputNames);
TRACE("SidEnumBuffer->Entries: %lu\n", SidEnumBuffer->Entries);
TRACE("SidEnumBuffer->SidInfo: %p\n", SidEnumBuffer->SidInfo);

    for (i = 0; i < SidEnumBuffer->Entries; i++)
    {
TRACE("i: %lu\n", i);
        ptr = LsapLookupWellKnownSid(Sids[i]);
        if (ptr != NULL)
        {
            OutputNames[i].Use = ptr->NameUse;

            OutputNames[i].DomainIndex = i; /* Fixme */

            OutputNames[i].Name.Buffer = MIDL_user_allocate(ptr->Name.MaximumLength);
            OutputNames[i].Name.Length = ptr->Name.Length;
            OutputNames[i].Name.MaximumLength = ptr->Name.MaximumLength;
            RtlCopyMemory(OutputNames[i].Name.Buffer, ptr->Name.Buffer, ptr->Name.MaximumLength);

            Mapped++;
        }
        else
        {
            OutputNames[i].Use = SidTypeWellKnownGroup;
            OutputNames[i].DomainIndex = i;
            OutputNames[i].Name.Buffer = MIDL_user_allocate(UserName.MaximumLength);
            OutputNames[i].Name.Length = UserName.Length;
            OutputNames[i].Name.MaximumLength = UserName.MaximumLength;
            RtlCopyMemory(OutputNames[i].Name.Buffer, UserName.Buffer, UserName.MaximumLength);

            Mapped++;
        }
    }

    if (Mapped == 0)
        Status = STATUS_NONE_MAPPED;
    else if (Mapped < SidEnumBuffer->Entries)
        Status = STATUS_SOME_NOT_MAPPED;
    else
        Status = STATUS_SUCCESS;

    return Status;
}

/* EOF */
