/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         Security Account Manager (SAM) Server
 * FILE:            reactos/dll/win32/samsrv/setup.c
 * PURPOSE:         Registry setup routines
 *
 * PROGRAMMERS:     Eric Kohl
 */

/* INCLUDES ****************************************************************/

#include "samsrv.h"

WINE_DEFAULT_DEBUG_CHANNEL(samsrv);


/* GLOBALS *****************************************************************/

#define TICKS_PER_SECOND 10000000LL

SID_IDENTIFIER_AUTHORITY SecurityNtAuthority = {SECURITY_NT_AUTHORITY};


/* FUNCTIONS ***************************************************************/

static INT
SampLoadString(HINSTANCE hInstance,
               UINT uId,
               LPWSTR lpBuffer,
               INT nBufferMax)
{
    HGLOBAL hmem;
    HRSRC hrsrc;
    WCHAR *p;
    int string_num;
    int i;

    /* Use loword (incremented by 1) as resourceid */
    hrsrc = FindResourceW(hInstance,
                          MAKEINTRESOURCEW((LOWORD(uId) >> 4) + 1),
                          (LPWSTR)RT_STRING);
    if (!hrsrc)
        return 0;

    hmem = LoadResource(hInstance, hrsrc);
    if (!hmem)
        return 0;

    p = LockResource(hmem);
    string_num = uId & 0x000f;
    for (i = 0; i < string_num; i++)
        p += *p + 1;

    i = min(nBufferMax - 1, *p);
    if (i > 0)
    {
        memcpy(lpBuffer, p + 1, i * sizeof(WCHAR));
        lpBuffer[i] = 0;
    }
    else
    {
        if (nBufferMax > 1)
        {
            lpBuffer[0] = 0;
            return 0;
        }
    }

    return i;
}


BOOL
SampIsSetupRunning(VOID)
{
    DWORD dwError;
    HKEY hKey;
    DWORD dwType;
    DWORD dwSize;
    DWORD dwSetupType;

    TRACE("SampIsSetupRunning()\n");

    /* Open key */
    dwError = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                            L"SYSTEM\\Setup",
                            0,
                            KEY_QUERY_VALUE,
                            &hKey);
    if (dwError != ERROR_SUCCESS)
        return FALSE;

    /* Read key */
    dwSize = sizeof(DWORD);
    dwError = RegQueryValueExW(hKey,
                               L"SetupType",
                               NULL,
                               &dwType,
                               (LPBYTE)&dwSetupType,
                               &dwSize);

    /* Close key, and check if returned values are correct */
    RegCloseKey(hKey);
    if (dwError != ERROR_SUCCESS || dwType != REG_DWORD || dwSize != sizeof(DWORD))
        return FALSE;

    TRACE("SampIsSetupRunning() returns %s\n", (dwSetupType != 0) ? "TRUE" : "FALSE");
    return (dwSetupType != 0);
}


static PSID
AppendRidToSid(PSID SrcSid,
               ULONG Rid)
{
    ULONG Rids[8] = {0, 0, 0, 0, 0, 0, 0, 0};
    UCHAR RidCount;
    PSID DstSid;
    ULONG i;

    RidCount = *RtlSubAuthorityCountSid(SrcSid);
    if (RidCount >= 8)
        return NULL;

    for (i = 0; i < RidCount; i++)
        Rids[i] = *RtlSubAuthoritySid(SrcSid, i);

    Rids[RidCount] = Rid;
    RidCount++;

    RtlAllocateAndInitializeSid(RtlIdentifierAuthoritySid(SrcSid),
                                RidCount,
                                Rids[0],
                                Rids[1],
                                Rids[2],
                                Rids[3],
                                Rids[4],
                                Rids[5],
                                Rids[6],
                                Rids[7],
                                &DstSid);

    return DstSid;
}


static BOOL
SampAddMemberToAlias(HKEY hDomainKey,
                     ULONG AliasId,
                     PSID MemberSid)
{
    DWORD dwDisposition;
    LPWSTR MemberSidString = NULL;
    WCHAR szKeyName[256];
    HKEY hMembersKey;

    ConvertSidToStringSidW(MemberSid, &MemberSidString);

    swprintf(szKeyName, L"Aliases\\%08lX\\Members", AliasId);

    if (!RegCreateKeyExW(hDomainKey,
                         szKeyName,
                         0,
                         NULL,
                         REG_OPTION_NON_VOLATILE,
                         KEY_ALL_ACCESS,
                         NULL,
                         &hMembersKey,
                         &dwDisposition))
    {
        RegSetValueEx(hMembersKey,
                      MemberSidString,
                      0,
                      REG_BINARY,
                      (LPVOID)MemberSid,
                      RtlLengthSid(MemberSid));

        RegCloseKey(hMembersKey);
    }

    swprintf(szKeyName, L"Aliases\\Members\\%s", MemberSidString);

    if (!RegCreateKeyExW(hDomainKey,
                         szKeyName,
                         0,
                         NULL,
                         REG_OPTION_NON_VOLATILE,
                         KEY_ALL_ACCESS,
                         NULL,
                         &hMembersKey,
                         &dwDisposition))
    {
        swprintf(szKeyName, L"%08lX", AliasId);

        RegSetValueEx(hMembersKey,
                      szKeyName,
                      0,
                      REG_BINARY,
                      (LPVOID)MemberSid,
                      RtlLengthSid(MemberSid));

        RegCloseKey(hMembersKey);
    }

    if (MemberSidString != NULL)
        LocalFree(MemberSidString);

    return TRUE;
}


static BOOL
SampCreateAliasAccount(HKEY hDomainKey,
                       LPCWSTR lpAccountName,
                       LPCWSTR lpDescription,
                       ULONG ulRelativeId)
{
    DWORD dwDisposition;
    WCHAR szAccountKeyName[32];
    HKEY hAccountKey = NULL;
    HKEY hNamesKey = NULL;

    swprintf(szAccountKeyName, L"Aliases\\%08lX", ulRelativeId);

    if (!RegCreateKeyExW(hDomainKey,
                         szAccountKeyName,
                         0,
                         NULL,
                         REG_OPTION_NON_VOLATILE,
                         KEY_ALL_ACCESS,
                         NULL,
                         &hAccountKey,
                         &dwDisposition))
    {
        RegSetValueEx(hAccountKey,
                      L"Name",
                      0,
                      REG_SZ,
                      (LPVOID)lpAccountName,
                      (wcslen(lpAccountName) + 1) * sizeof(WCHAR));

        RegSetValueEx(hAccountKey,
                      L"Description",
                      0,
                      REG_SZ,
                      (LPVOID)lpDescription,
                      (wcslen(lpDescription) + 1) * sizeof(WCHAR));

        RegCloseKey(hAccountKey);
    }

    if (!RegOpenKeyExW(hDomainKey,
                       L"Aliases\\Names",
                       0,
                       KEY_ALL_ACCESS,
                       &hNamesKey))
    {
        RegSetValueEx(hNamesKey,
                      lpAccountName,
                      0,
                      REG_DWORD,
                      (LPVOID)&ulRelativeId,
                      sizeof(ULONG));

        RegCloseKey(hNamesKey);
    }

    return TRUE;
}


#if 0
static BOOL
SampCreateGroupAccount(HKEY hDomainKey,
                       LPCWSTR lpAccountName,
                       ULONG ulRelativeId)
{

    return FALSE;
}
#endif


static BOOL
SampCreateUserAccount(HKEY hDomainKey,
                      LPCWSTR lpAccountName,
                      LPCWSTR lpComment,
                      ULONG ulRelativeId,
                      ULONG UserAccountControl)
{
    SAM_USER_FIXED_DATA FixedUserData;
    UCHAR LogonHours[23];
    LPWSTR lpEmptyString = L"";
    DWORD dwDisposition;
    WCHAR szAccountKeyName[32];
    HKEY hAccountKey = NULL;
    HKEY hNamesKey = NULL;

    /* Initialize fixed user data */
    FixedUserData.Version = 1;
    FixedUserData.Reserved = 0;
    FixedUserData.LastLogon.QuadPart = 0;
    FixedUserData.LastLogoff.QuadPart = 0;
    FixedUserData.PasswordLastSet.QuadPart = 0;
    FixedUserData.AccountExpires.LowPart = MAXULONG;
    FixedUserData.AccountExpires.HighPart = MAXLONG;
    FixedUserData.LastBadPasswordTime.QuadPart = 0;
    FixedUserData.UserId = ulRelativeId;
    FixedUserData.PrimaryGroupId = DOMAIN_GROUP_RID_USERS;
    FixedUserData.UserAccountControl = UserAccountControl;
    FixedUserData.CountryCode = 0;
    FixedUserData.CodePage = 0;
    FixedUserData.BadPasswordCount = 0;
    FixedUserData.LogonCount = 0;
    FixedUserData.AdminCount = 0;
    FixedUserData.OperatorCount = 0;

    swprintf(szAccountKeyName, L"Users\\%08lX", ulRelativeId);

    if (!RegCreateKeyExW(hDomainKey,
                         szAccountKeyName,
                         0,
                         NULL,
                         REG_OPTION_NON_VOLATILE,
                         KEY_ALL_ACCESS,
                         NULL,
                         &hAccountKey,
                         &dwDisposition))
    {
        RegSetValueEx(hAccountKey,
                      L"F",
                      0,
                      REG_BINARY,
                      (LPVOID)&FixedUserData,
                      sizeof(SAM_USER_FIXED_DATA));

        RegSetValueEx(hAccountKey,
                      L"Name",
                      0,
                      REG_SZ,
                      (LPVOID)lpAccountName,
                      (wcslen(lpAccountName) + 1) * sizeof(WCHAR));

        RegSetValueEx(hAccountKey,
                      L"FullName",
                      0,
                      REG_SZ,
                      (LPVOID)lpEmptyString,
                      sizeof(WCHAR));

        RegSetValueEx(hAccountKey,
                      L"HomeDirectory",
                      0,
                      REG_SZ,
                      (LPVOID)lpEmptyString,
                      sizeof(WCHAR));

        RegSetValueEx(hAccountKey,
                      L"HomeDirectoryDrive",
                      0,
                      REG_SZ,
                      (LPVOID)lpEmptyString,
                      sizeof(WCHAR));

        RegSetValueEx(hAccountKey,
                      L"ScriptPath",
                      0,
                      REG_SZ,
                      (LPVOID)lpEmptyString,
                      sizeof(WCHAR));

        RegSetValueEx(hAccountKey,
                      L"ProfilePath",
                      0,
                      REG_SZ,
                      (LPVOID)lpEmptyString,
                      sizeof(WCHAR));

        RegSetValueEx(hAccountKey,
                      L"AdminComment",
                      0,
                      REG_SZ,
                      (LPVOID)lpComment,
                      (wcslen(lpComment) + 1) * sizeof(WCHAR));

        RegSetValueEx(hAccountKey,
                      L"UserComment",
                      0,
                      REG_SZ,
                      (LPVOID)lpEmptyString,
                      sizeof(WCHAR));

        RegSetValueEx(hAccountKey,
                      L"WorkStations",
                      0,
                      REG_SZ,
                      (LPVOID)lpEmptyString,
                      sizeof(WCHAR));

        RegSetValueEx(hAccountKey,
                      L"Parameters",
                      0,
                      REG_SZ,
                      (LPVOID)lpEmptyString,
                      sizeof(WCHAR));

        /* Set LogonHours attribute*/
        *((PUSHORT)LogonHours) = 168;
        memset(&(LogonHours[2]), 0xff, 21);

        RegSetValueEx(hAccountKey,
                      L"LogonHours",
                      0,
                      REG_BINARY,
                      (LPVOID)LogonHours,
                      sizeof(LogonHours));

        /* FIXME: Set Groups attribute*/

        /* Set LMPwd attribute*/
        RegSetValueEx(hAccountKey,
                      L"LMPwd",
                      0,
                      REG_BINARY,
                      NULL,
                      0);

        /* Set NTPwd attribute*/
        RegSetValueEx(hAccountKey,
                      L"NTPwd",
                      0,
                      REG_BINARY,
                      NULL,
                      0);

        /* Set LMPwdHistory attribute*/
        RegSetValueEx(hAccountKey,
                      L"LMPwdHistory",
                      0,
                      REG_BINARY,
                      NULL,
                      0);

        /* Set NTPwdHistory attribute*/
        RegSetValueEx(hAccountKey,
                      L"NTPwdHistory",
                      0,
                      REG_BINARY,
                      NULL,
                      0);

        /* FIXME: Set SecDesc attribute*/

        RegCloseKey(hAccountKey);
    }

    if (!RegOpenKeyExW(hDomainKey,
                       L"Users\\Names",
                       0,
                       KEY_ALL_ACCESS,
                       &hNamesKey))
    {
        RegSetValueEx(hNamesKey,
                      lpAccountName,
                      0,
                      REG_DWORD,
                      (LPVOID)&ulRelativeId,
                      sizeof(ULONG));

        RegCloseKey(hNamesKey);
    }

    return TRUE;
}


static BOOL
SampCreateDomain(IN HKEY hDomainsKey,
                 IN LPCWSTR lpKeyName,
                 IN LPCWSTR lpDomainName,
                 IN PSID lpDomainSid,
                 OUT PHKEY lpDomainKey)
{
    SAM_DOMAIN_FIXED_DATA FixedData;
    LPWSTR lpEmptyString = L"";
    DWORD dwDisposition;
    HKEY hDomainKey = NULL;
    HKEY hAliasesKey = NULL;
    HKEY hGroupsKey = NULL;
    HKEY hUsersKey = NULL;
    HKEY hNamesKey = NULL;

    if (lpDomainKey != NULL)
        *lpDomainKey = NULL;

    /* Initialize the fixed domain data */
    memset(&FixedData, 0, sizeof(SAM_DOMAIN_FIXED_DATA));
    FixedData.Version = 1;
    NtQuerySystemTime(&FixedData.CreationTime);
    FixedData.DomainModifiedCount.QuadPart = 0;
    FixedData.MaxPasswordAge.QuadPart = -(6LL * 7LL * 24LL * 60LL * 60LL * TICKS_PER_SECOND); /* 6 weeks */
    FixedData.MinPasswordAge.QuadPart = 0;                                                    /* right now */
//    FixedData.ForceLogoff.QuadPart = // very far in the future aka never
    FixedData.LockoutDuration.QuadPart = -(30LL * 60LL * TICKS_PER_SECOND);                   /* 30 minutes */
    FixedData.LockoutObservationWindow.QuadPart = -(30LL * 60LL * TICKS_PER_SECOND);          /* 30 minutes */
    FixedData.ModifiedCountAtLastPromotion.QuadPart = 0;
    FixedData.NextRid = 1000;
    FixedData.PasswordProperties = 0;
    FixedData.MinPasswordLength = 0;
    FixedData.PasswordHistoryLength = 0;
    FixedData.LockoutThreshold = 0;
    FixedData.DomainServerState = DomainServerEnabled;
    FixedData.DomainServerRole = DomainServerRolePrimary;
    FixedData.UasCompatibilityRequired = TRUE;

    if (RegCreateKeyExW(hDomainsKey,
                        lpKeyName,
                        0,
                        NULL,
                        REG_OPTION_NON_VOLATILE,
                        KEY_ALL_ACCESS,
                        NULL,
                        &hDomainKey,
                        &dwDisposition))
        return FALSE;

    /* Set the fixed data value */
    if (RegSetValueEx(hDomainKey,
                      L"F",
                      0,
                      REG_BINARY,
                      (LPVOID)&FixedData,
                      sizeof(SAM_DOMAIN_FIXED_DATA)))
        return FALSE;

    if (lpDomainSid != NULL)
    {
        RegSetValueEx(hDomainKey,
                      L"Name",
                      0,
                      REG_SZ,
                      (LPVOID)lpDomainName,
                      (wcslen(lpDomainName) + 1) * sizeof(WCHAR));

        RegSetValueEx(hDomainKey,
                      L"SID",
                      0,
                      REG_BINARY,
                      (LPVOID)lpDomainSid,
                      RtlLengthSid(lpDomainSid));
    }

    RegSetValueEx(hDomainKey,
                  L"OemInformation",
                  0,
                  REG_SZ,
                  (LPVOID)lpEmptyString,
                  sizeof(WCHAR));

    RegSetValueEx(hDomainKey,
                  L"ReplicaSourceNodeName",
                  0,
                  REG_SZ,
                  (LPVOID)lpEmptyString,
                  sizeof(WCHAR));

    /* Create the Alias container */
    if (!RegCreateKeyExW(hDomainKey,
                         L"Aliases",
                         0,
                         NULL,
                         REG_OPTION_NON_VOLATILE,
                         KEY_ALL_ACCESS,
                         NULL,
                         &hAliasesKey,
                         &dwDisposition))
    {
        if (!RegCreateKeyExW(hAliasesKey,
                             L"Names",
                             0,
                             NULL,
                             REG_OPTION_NON_VOLATILE,
                             KEY_ALL_ACCESS,
                             NULL,
                             &hNamesKey,
                             &dwDisposition))
            RegCloseKey(hNamesKey);

        RegCloseKey(hAliasesKey);
    }

    /* Create the Groups container */
    if (!RegCreateKeyExW(hDomainKey,
                         L"Groups",
                         0,
                         NULL,
                         REG_OPTION_NON_VOLATILE,
                         KEY_ALL_ACCESS,
                         NULL,
                         &hGroupsKey,
                         &dwDisposition))
    {
        if (!RegCreateKeyExW(hGroupsKey,
                             L"Names",
                             0,
                             NULL,
                             REG_OPTION_NON_VOLATILE,
                             KEY_ALL_ACCESS,
                             NULL,
                             &hNamesKey,
                             &dwDisposition))
            RegCloseKey(hNamesKey);

        RegCloseKey(hGroupsKey);
    }


    /* Create the Users container */
    if (!RegCreateKeyExW(hDomainKey,
                         L"Users",
                         0,
                         NULL,
                         REG_OPTION_NON_VOLATILE,
                         KEY_ALL_ACCESS,
                         NULL,
                         &hUsersKey,
                         &dwDisposition))
    {
        if (!RegCreateKeyExW(hUsersKey,
                             L"Names",
                             0,
                             NULL,
                             REG_OPTION_NON_VOLATILE,
                             KEY_ALL_ACCESS,
                             NULL,
                             &hNamesKey,
                             &dwDisposition))
            RegCloseKey(hNamesKey);

        RegCloseKey(hUsersKey);
    }

    if (lpDomainKey != NULL)
        *lpDomainKey = hDomainKey;

    return TRUE;
}


NTSTATUS
SampGetAccountDomainInfo(PPOLICY_ACCOUNT_DOMAIN_INFO *AccountDomainInfo)
{
    LSA_OBJECT_ATTRIBUTES ObjectAttributes;
    LSA_HANDLE PolicyHandle;
    NTSTATUS Status;

    TRACE("SampGetAccountDomainInfo\n");

    memset(&ObjectAttributes, 0, sizeof(LSA_OBJECT_ATTRIBUTES));
    ObjectAttributes.Length = sizeof(LSA_OBJECT_ATTRIBUTES);

    Status = LsaOpenPolicy(NULL,
                           &ObjectAttributes,
                           POLICY_VIEW_LOCAL_INFORMATION,
                           &PolicyHandle);
    if (Status != STATUS_SUCCESS)
    {
        ERR("LsaOpenPolicy failed (Status: 0x%08lx)\n", Status);
        return Status;
    }

    Status = LsaQueryInformationPolicy(PolicyHandle,
                                       PolicyAccountDomainInformation,
                                       (PVOID *)AccountDomainInfo);

    LsaClose(PolicyHandle);

    return Status;
}


BOOL
SampInitializeSAM(VOID)
{
    PPOLICY_ACCOUNT_DOMAIN_INFO AccountDomainInfo = NULL;
    DWORD dwDisposition;
    HKEY hSamKey = NULL;
    HKEY hDomainsKey = NULL;
    HKEY hDomainKey = NULL;
    PSID pBuiltinSid = NULL;
    BOOL bResult = TRUE;
    PSID pSid;
    HINSTANCE hInstance;
    WCHAR szComment[256];
    WCHAR szName[80];
    NTSTATUS Status;

    TRACE("SampInitializeSAM() called\n");

    hInstance = GetModuleHandleW(L"samsrv.dll");

    if (RegCreateKeyExW(HKEY_LOCAL_MACHINE,
                        L"SAM\\SAM",
                        0,
                        NULL,
                        REG_OPTION_NON_VOLATILE,
                        KEY_ALL_ACCESS,
                        NULL,
                        &hSamKey,
                        &dwDisposition))
    {
        ERR("Failed to create 'Sam' key! (Error %lu)\n", GetLastError());
        return FALSE;
    }

    if (RegCreateKeyExW(hSamKey,
                        L"Domains",
                        0,
                        NULL,
                        REG_OPTION_NON_VOLATILE,
                        KEY_ALL_ACCESS,
                        NULL,
                        &hDomainsKey,
                        &dwDisposition))
    {
        ERR("Failed to create 'Domains' key! (Error %lu)\n", GetLastError());
        bResult = FALSE;
        goto done;
    }

    RegCloseKey(hSamKey);
    hSamKey = NULL;

    /* Create and initialize the Builtin Domain SID */
    pBuiltinSid = RtlAllocateHeap(RtlGetProcessHeap(), 0, RtlLengthRequiredSid(1));
    if (pBuiltinSid == NULL)
    {
        ERR("Failed to alloacte the Builtin Domain SID\n");
        bResult = FALSE;
        goto done;
    }

    RtlInitializeSid(pBuiltinSid, &SecurityNtAuthority, 1);
    *(RtlSubAuthoritySid(pBuiltinSid, 0)) = SECURITY_BUILTIN_DOMAIN_RID;

    /* Get account domain information */
    Status = SampGetAccountDomainInfo(&AccountDomainInfo);
    if (!NT_SUCCESS(Status))
    {
        ERR("SampGetAccountDomainInfo failed (Status %08lx)\n", Status);
        bResult = FALSE;
        goto done;
    }

    SampLoadString(hInstance, IDS_DOMAIN_BUILTIN_NAME, szName, 80);

    /* Create the Builtin domain */
    if (SampCreateDomain(hDomainsKey,
                         L"Builtin",
                         szName, //L"Builtin", // SampGetResourceString(hInstance, IDS_DOMAIN_BUILTIN_NAME),
                         pBuiltinSid,
                         &hDomainKey))
    {
        SampLoadString(hInstance, IDS_ALIAS_ADMINISTRATORS_NAME, szName, 80);
        SampLoadString(hInstance, IDS_ALIAS_ADMINISTRATORS_COMMENT, szComment, 256);

        SampCreateAliasAccount(hDomainKey,
                               szName,
                               szComment,
                               DOMAIN_ALIAS_RID_ADMINS);

        SampLoadString(hInstance, IDS_ALIAS_USERS_NAME, szName, 80);
        SampLoadString(hInstance, IDS_ALIAS_USERS_COMMENT, szComment, 256);

        SampCreateAliasAccount(hDomainKey,
                               szName,
                               szComment,
                               DOMAIN_ALIAS_RID_USERS);

        SampLoadString(hInstance, IDS_ALIAS_GUESTS_NAME, szName, 80);
        SampLoadString(hInstance, IDS_ALIAS_GUESTS_COMMENT, szComment, 256);

        SampCreateAliasAccount(hDomainKey,
                               szName,
                               szComment,
                               DOMAIN_ALIAS_RID_GUESTS);

        SampLoadString(hInstance, IDS_ALIAS_POWER_USERS_NAME, szName, 80);
        SampLoadString(hInstance, IDS_ALIAS_POWER_USERS_COMMENT, szComment, 256);

        SampCreateAliasAccount(hDomainKey,
                               szName,
                               szComment,
                               DOMAIN_ALIAS_RID_POWER_USERS);


        pSid = AppendRidToSid(AccountDomainInfo->DomainSid,
                              DOMAIN_USER_RID_ADMIN);
        if (pSid != NULL)
        {
            SampAddMemberToAlias(hDomainKey,
                                 DOMAIN_ALIAS_RID_ADMINS,
                                 pSid);

            RtlFreeHeap(RtlGetProcessHeap(), 0, pSid);
        }


        RegCloseKey(hDomainKey);
    }

    /* Create the Account domain */
    if (SampCreateDomain(hDomainsKey,
                         L"Account",
                         L"",
                         AccountDomainInfo->DomainSid,
                         &hDomainKey))
    {
        SampLoadString(hInstance, IDS_USER_ADMINISTRATOR_NAME, szName, 80);
        SampLoadString(hInstance, IDS_USER_ADMINISTRATOR_COMMENT, szComment, 256);

        SampCreateUserAccount(hDomainKey,
                              szName,
                              szComment,
                              DOMAIN_USER_RID_ADMIN,
                              USER_DONT_EXPIRE_PASSWORD | USER_NORMAL_ACCOUNT);

        SampLoadString(hInstance, IDS_USER_GUEST_NAME, szName, 80);
        SampLoadString(hInstance, IDS_USER_GUEST_COMMENT, szComment, 256);

        SampCreateUserAccount(hDomainKey,
                              szName,
                              szComment,
                              DOMAIN_USER_RID_GUEST,
                              USER_ACCOUNT_DISABLED | USER_DONT_EXPIRE_PASSWORD | USER_NORMAL_ACCOUNT);

        RegCloseKey(hDomainKey);
    }

done:
    if (AccountDomainInfo)
        LsaFreeMemory(AccountDomainInfo);

    if (pBuiltinSid)
        RtlFreeHeap(RtlGetProcessHeap(), 0, pBuiltinSid);

    if (hDomainsKey)
        RegCloseKey(hDomainsKey);

    if (hSamKey)
        RegCloseKey(hSamKey);

    TRACE("SampInitializeSAM() done\n");

    return bResult;
}
