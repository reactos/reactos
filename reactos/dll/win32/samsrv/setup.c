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

SID_IDENTIFIER_AUTHORITY SecurityNtAuthority = {SECURITY_NT_AUTHORITY};

/* FUNCTIONS ***************************************************************/

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


static BOOL
SampCreateUserAccount(HKEY hDomainKey,
                      LPCWSTR lpAccountName,
                      ULONG ulRelativeId)
{
    SAM_USER_FIXED_DATA FixedUserData;
    DWORD dwDisposition;
    WCHAR szAccountKeyName[32];
    HKEY hAccountKey = NULL;
    HKEY hNamesKey = NULL;

    /* Initialize fixed user data */
    memset(&FixedUserData, 0, sizeof(SAM_USER_FIXED_DATA));
    FixedUserData.Version = 1;

    FixedUserData.UserId = ulRelativeId;

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
//    FixedData.MaxPasswordAge                 // 6 Weeks
    FixedData.MinPasswordAge.QuadPart = 0;     // Now
//    FixedData.ForceLogoff
//    FixedData.LockoutDuration                // 30 minutes
//    FixedData.LockoutObservationWindow       // 30 minutes
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
                           POLICY_TRUST_ADMIN,
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
    NTSTATUS Status;

    TRACE("SampInitializeSAM() called\n");

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

    /* Create the Builtin domain */
    if (SampCreateDomain(hDomainsKey,
                         L"Builtin",
                         L"Builtin",
                         pBuiltinSid,
                         &hDomainKey))
    {
        SampCreateAliasAccount(hDomainKey,
                               L"Administrators",
                               L"Testabc1234567890",
                               DOMAIN_ALIAS_RID_ADMINS);

        SampCreateAliasAccount(hDomainKey,
                               L"Users",
                               L"Users Group",
                               DOMAIN_ALIAS_RID_USERS);

        SampCreateAliasAccount(hDomainKey,
                               L"Guests",
                               L"Guests Group",
                               DOMAIN_ALIAS_RID_GUESTS);

        SampCreateAliasAccount(hDomainKey,
                               L"Power Users",
                               L"Power Users Group",
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
        SampCreateUserAccount(hDomainKey,
                              L"Administrator",
                              DOMAIN_USER_RID_ADMIN);

        SampCreateUserAccount(hDomainKey,
                              L"Guest",
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
