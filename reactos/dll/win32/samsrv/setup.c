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


static BOOL
SampCreateUserAccount(HKEY hDomainKey,
                      LPCWSTR lpAccountName,
                      ULONG ulRelativeId)
{
    DWORD dwDisposition;
    WCHAR szUserKeyName[32];
    HKEY hUserKey = NULL;
    HKEY hNamesKey = NULL;

    swprintf(szUserKeyName, L"Users\\%08lX", ulRelativeId);

    if (!RegCreateKeyExW(hDomainKey,
                         szUserKeyName,
                         0,
                         NULL,
                         REG_OPTION_NON_VOLATILE,
                         KEY_ALL_ACCESS,
                         NULL,
                         &hUserKey,
                         &dwDisposition))
    {
        RegSetValueEx(hUserKey,
                      L"Name",
                      0,
                      REG_SZ,
                      (LPVOID)lpAccountName,
                      (wcslen(lpAccountName) + 1) * sizeof(WCHAR));

        RegCloseKey(hUserKey);
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
                 IN LPCWSTR lpDomainName,
                 IN PSID lpDomainSid,
                 OUT PHKEY lpDomainKey)
{
    DWORD dwDisposition;
    HKEY hDomainKey = NULL;
    HKEY hAliasKey = NULL;
    HKEY hGroupsKey = NULL;
    HKEY hUsersKey = NULL;
    HKEY hNamesKey = NULL;

    if (lpDomainKey != NULL)
        *lpDomainKey = NULL;

    if (RegCreateKeyExW(hDomainsKey,
                        lpDomainName,
                        0,
                        NULL,
                        REG_OPTION_NON_VOLATILE,
                        KEY_ALL_ACCESS,
                        NULL,
                        &hDomainKey,
                        &dwDisposition))
        return FALSE;

    if (lpDomainSid != NULL)
    {
        RegSetValueEx(hDomainKey,
                      L"SID",
                      0,
                      REG_BINARY,
                      (LPVOID)lpDomainSid,
                      RtlLengthSid(lpDomainSid));
    }

    /* Create the Alias container */
    if (!RegCreateKeyExW(hDomainKey,
                         L"Alias",
                         0,
                         NULL,
                         REG_OPTION_NON_VOLATILE,
                         KEY_ALL_ACCESS,
                         NULL,
                         &hAliasKey,
                         &dwDisposition))
    {
        if (!RegCreateKeyExW(hAliasKey,
                             L"Names",
                             0,
                             NULL,
                             REG_OPTION_NON_VOLATILE,
                             KEY_ALL_ACCESS,
                             NULL,
                             &hNamesKey,
                             &dwDisposition))
            RegCloseKey(hNamesKey);

        RegCloseKey(hAliasKey);
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
                         pBuiltinSid,
                         &hDomainKey))
    {

        RegCloseKey(hDomainKey);
    }

    /* Create the Account domain */
    if (SampCreateDomain(hDomainsKey,
                         L"Account",
                         AccountDomainInfo->DomainSid, //NULL,
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
