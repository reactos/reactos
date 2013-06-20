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

static BOOL
SampSetupAddMemberToAlias(HKEY hDomainKey,
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
SampSetupCreateAliasAccount(HKEY hDomainKey,
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


static
NTSTATUS
SampSetupAddMemberToGroup(IN HANDLE hDomainKey,
                          IN ULONG GroupId,
                          IN ULONG MemberId)
{
    WCHAR szKeyName[256];
    HANDLE hGroupKey = NULL;
    PULONG MembersBuffer = NULL;
    ULONG MembersCount = 0;
    ULONG Length = 0;
    ULONG i;
    NTSTATUS Status;

    swprintf(szKeyName, L"Groups\\%08lX", GroupId);

    Status = SampRegOpenKey(hDomainKey,
                            szKeyName,
                            KEY_ALL_ACCESS,
                            &hGroupKey);
    if (!NT_SUCCESS(Status))
        return Status;

    Status = SampRegQueryValue(hGroupKey,
                               L"Members",
                               NULL,
                               NULL,
                               &Length);
    if (!NT_SUCCESS(Status) && Status != STATUS_OBJECT_NAME_NOT_FOUND)
        goto done;

    MembersBuffer = midl_user_allocate(Length + sizeof(ULONG));
    if (MembersBuffer == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto done;
    }

    if (Status != STATUS_OBJECT_NAME_NOT_FOUND)
    {
        Status = SampRegQueryValue(hGroupKey,
                                   L"Members",
                                   NULL,
                                   MembersBuffer,
                                   &Length);
        if (!NT_SUCCESS(Status))
            goto done;

        MembersCount = Length / sizeof(ULONG);
    }

    for (i = 0; i < MembersCount; i++)
    {
        if (MembersBuffer[i] == MemberId)
        {
            Status = STATUS_MEMBER_IN_GROUP;
            goto done;
        }
    }

    MembersBuffer[MembersCount] = MemberId;
    Length += sizeof(ULONG);

    Status = SampRegSetValue(hGroupKey,
                             L"Members",
                             REG_BINARY,
                             MembersBuffer,
                             Length);

done:
    if (MembersBuffer != NULL)
        midl_user_free(MembersBuffer);

    if (hGroupKey != NULL)
        SampRegCloseKey(hGroupKey);

    return Status;
}


static
NTSTATUS
SampSetupCreateGroupAccount(HANDLE hDomainKey,
                            LPCWSTR lpAccountName,
                            LPCWSTR lpComment,
                            ULONG ulRelativeId)
{
    SAM_GROUP_FIXED_DATA FixedGroupData;
    WCHAR szAccountKeyName[32];
    HANDLE hAccountKey = NULL;
    HANDLE hNamesKey = NULL;
    NTSTATUS Status;

    /* Initialize fixed group data */
    FixedGroupData.Version = 1;
    FixedGroupData.Reserved = 0;
    FixedGroupData.GroupId = ulRelativeId;
    FixedGroupData.Attributes = 0;

    swprintf(szAccountKeyName, L"Groups\\%08lX", ulRelativeId);

    Status = SampRegCreateKey(hDomainKey,
                              szAccountKeyName,
                              KEY_ALL_ACCESS,
                              &hAccountKey);
    if (!NT_SUCCESS(Status))
        return Status;

    Status = SampRegSetValue(hAccountKey,
                             L"F",
                             REG_BINARY,
                             (LPVOID)&FixedGroupData,
                             sizeof(SAM_GROUP_FIXED_DATA));
    if (!NT_SUCCESS(Status))
        goto done;

    Status = SampRegSetValue(hAccountKey,
                             L"Name",
                             REG_SZ,
                             (LPVOID)lpAccountName,
                             (wcslen(lpAccountName) + 1) * sizeof(WCHAR));
    if (!NT_SUCCESS(Status))
        goto done;

    Status = SampRegSetValue(hAccountKey,
                             L"AdminComment",
                             REG_SZ,
                             (LPVOID)lpComment,
                             (wcslen(lpComment) + 1) * sizeof(WCHAR));
    if (!NT_SUCCESS(Status))
        goto done;

    Status = SampRegOpenKey(hDomainKey,
                            L"Groups\\Names",
                            KEY_ALL_ACCESS,
                            &hNamesKey);
    if (!NT_SUCCESS(Status))
        goto done;

    Status = SampRegSetValue(hNamesKey,
                            lpAccountName,
                            REG_DWORD,
                            (LPVOID)&ulRelativeId,
                            sizeof(ULONG));

done:
    if (hNamesKey != NULL)
        SampRegCloseKey(hNamesKey);

    if (hAccountKey != NULL)
    {
        SampRegCloseKey(hAccountKey);

        if (!NT_SUCCESS(Status))
            SampRegDeleteKey(hDomainKey,
                             szAccountKeyName);
    }

    return Status;
}


static BOOL
SampSetupCreateUserAccount(HKEY hDomainKey,
                           LPCWSTR lpAccountName,
                           LPCWSTR lpComment,
                           ULONG ulRelativeId,
                           ULONG UserAccountControl)
{
    SAM_USER_FIXED_DATA FixedUserData;
    GROUP_MEMBERSHIP GroupMembership;
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

        /* Set Groups attribute*/
        GroupMembership.RelativeId = DOMAIN_GROUP_RID_USERS;
        GroupMembership.Attributes = SE_GROUP_MANDATORY |
                                     SE_GROUP_ENABLED |
                                     SE_GROUP_ENABLED_BY_DEFAULT;

        RegSetValueEx(hAccountKey,
                      L"Groups",
                      0,
                      REG_BINARY,
                      (LPVOID)&GroupMembership,
                      sizeof(GROUP_MEMBERSHIP));

        /* Set LMPwd attribute*/
        RegSetValueEx(hAccountKey,
                      L"LMPwd",
                      0,
                      REG_BINARY,
                      (LPVOID)&EmptyLmHash,
                      sizeof(ENCRYPTED_LM_OWF_PASSWORD));

        /* Set NTPwd attribute*/
        RegSetValueEx(hAccountKey,
                      L"NTPwd",
                      0,
                      REG_BINARY,
                      (LPVOID)&EmptyNtHash,
                      sizeof(ENCRYPTED_NT_OWF_PASSWORD));

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
SampSetupCreateDomain(IN HKEY hDomainsKey,
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
    if (SampSetupCreateDomain(hDomainsKey,
                              L"Builtin",
                              szName,
                              pBuiltinSid,
                              &hDomainKey))
    {
        SampLoadString(hInstance, IDS_ALIAS_ADMINISTRATORS_NAME, szName, 80);
        SampLoadString(hInstance, IDS_ALIAS_ADMINISTRATORS_COMMENT, szComment, 256);

        SampSetupCreateAliasAccount(hDomainKey,
                                    szName,
                                    szComment,
                                    DOMAIN_ALIAS_RID_ADMINS);

        SampLoadString(hInstance, IDS_ALIAS_USERS_NAME, szName, 80);
        SampLoadString(hInstance, IDS_ALIAS_USERS_COMMENT, szComment, 256);

        SampSetupCreateAliasAccount(hDomainKey,
                                    szName,
                                    szComment,
                                    DOMAIN_ALIAS_RID_USERS);

        SampLoadString(hInstance, IDS_ALIAS_GUESTS_NAME, szName, 80);
        SampLoadString(hInstance, IDS_ALIAS_GUESTS_COMMENT, szComment, 256);

        SampSetupCreateAliasAccount(hDomainKey,
                                    szName,
                                    szComment,
                                    DOMAIN_ALIAS_RID_GUESTS);

        SampLoadString(hInstance, IDS_ALIAS_POWER_USERS_NAME, szName, 80);
        SampLoadString(hInstance, IDS_ALIAS_POWER_USERS_COMMENT, szComment, 256);

        SampSetupCreateAliasAccount(hDomainKey,
                                    szName,
                                    szComment,
                                    DOMAIN_ALIAS_RID_POWER_USERS);

        /* Add the Administrator user to the Administrators alias */
        pSid = AppendRidToSid(AccountDomainInfo->DomainSid,
                              DOMAIN_USER_RID_ADMIN);
        if (pSid != NULL)
        {
            SampSetupAddMemberToAlias(hDomainKey,
                                      DOMAIN_ALIAS_RID_ADMINS,
                                      pSid);

            RtlFreeHeap(RtlGetProcessHeap(), 0, pSid);
        }

        /* Add the Guest user to the Guests alias */
        pSid = AppendRidToSid(AccountDomainInfo->DomainSid,
                              DOMAIN_USER_RID_GUEST);
        if (pSid != NULL)
        {
            SampSetupAddMemberToAlias(hDomainKey,
                                      DOMAIN_ALIAS_RID_GUESTS,
                                      pSid);

            RtlFreeHeap(RtlGetProcessHeap(), 0, pSid);
        }

        RegCloseKey(hDomainKey);
    }

    /* Create the Account domain */
    if (SampSetupCreateDomain(hDomainsKey,
                              L"Account",
                              L"",
                              AccountDomainInfo->DomainSid,
                              &hDomainKey))
    {
        SampLoadString(hInstance, IDS_GROUP_NONE_NAME, szName, 80);
        SampLoadString(hInstance, IDS_GROUP_NONE_COMMENT, szComment, 256);

        SampSetupCreateGroupAccount(hDomainKey,
                                    szName,
                                    szComment,
                                    DOMAIN_GROUP_RID_USERS);

        SampLoadString(hInstance, IDS_USER_ADMINISTRATOR_NAME, szName, 80);
        SampLoadString(hInstance, IDS_USER_ADMINISTRATOR_COMMENT, szComment, 256);

        SampSetupCreateUserAccount(hDomainKey,
                                   szName,
                                   szComment,
                                   DOMAIN_USER_RID_ADMIN,
                                   USER_DONT_EXPIRE_PASSWORD | USER_NORMAL_ACCOUNT);

        SampSetupAddMemberToGroup(hDomainKey,
                                  DOMAIN_GROUP_RID_USERS,
                                  DOMAIN_USER_RID_ADMIN);

        SampLoadString(hInstance, IDS_USER_GUEST_NAME, szName, 80);
        SampLoadString(hInstance, IDS_USER_GUEST_COMMENT, szComment, 256);

        SampSetupCreateUserAccount(hDomainKey,
                                   szName,
                                   szComment,
                                   DOMAIN_USER_RID_GUEST,
                                   USER_ACCOUNT_DISABLED | USER_DONT_EXPIRE_PASSWORD | USER_NORMAL_ACCOUNT);

        SampSetupAddMemberToGroup(hDomainKey,
                                  DOMAIN_GROUP_RID_USERS,
                                  DOMAIN_USER_RID_GUEST);

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
