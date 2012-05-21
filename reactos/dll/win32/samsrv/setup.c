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
CreateBuiltinAliases(HKEY hAliasesKey)
{
    return TRUE;
}


static BOOL
CreateBuiltinGroups(HKEY hGroupsKey)
{
    return TRUE;
}


static BOOL
CreateBuiltinUsers(HKEY hUsersKey)
{
    return TRUE;
}


BOOL
SampInitializeSAM(VOID)
{
    DWORD dwDisposition;
    HKEY hSamKey;
    HKEY hDomainsKey;
    HKEY hAccountKey;
    HKEY hBuiltinKey;
    HKEY hAliasesKey;
    HKEY hGroupsKey;
    HKEY hUsersKey;

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
        RegCloseKey(hSamKey);
        return FALSE;
    }

    RegCloseKey (hSamKey);

    /* Create the 'Domains\\Account' key */
    if (RegCreateKeyExW(hDomainsKey,
                        L"Account",
                        0,
                        NULL,
                        REG_OPTION_NON_VOLATILE,
                        KEY_ALL_ACCESS,
                        NULL,
                        &hAccountKey,
                        &dwDisposition))
    {
        ERR("Failed to create 'Domains\\Account' key! (Error %lu)\n", GetLastError());
        RegCloseKey(hDomainsKey);
        return FALSE;
    }


    /* Create the 'Account\Aliases' key */
    if (RegCreateKeyExW(hAccountKey,
                        L"Aliases",
                        0,
                        NULL,
                        REG_OPTION_NON_VOLATILE,
                        KEY_ALL_ACCESS,
                        NULL,
                        &hAliasesKey,
                        &dwDisposition))
    {
        ERR("Failed to create 'Account\\Aliases' key! (Error %lu)\n", GetLastError());
        RegCloseKey(hAccountKey);
        RegCloseKey(hDomainsKey);
        return FALSE;
    }

    RegCloseKey (hAliasesKey);


    /* Create the 'Account\Groups' key */
    if (RegCreateKeyExW(hAccountKey,
                        L"Groups",
                        0,
                        NULL,
                        REG_OPTION_NON_VOLATILE,
                        KEY_ALL_ACCESS,
                        NULL,
                        &hGroupsKey,
                        &dwDisposition))
    {
        ERR("Failed to create 'Account\\Groups' key! (Error %lu)\n", GetLastError());
        RegCloseKey(hAccountKey);
        RegCloseKey(hDomainsKey);
        return FALSE;
    }

    RegCloseKey(hGroupsKey);


    /* Create the 'Account\Users' key */
    if (RegCreateKeyExW(hAccountKey,
                        L"Users",
                        0,
                        NULL,
                        REG_OPTION_NON_VOLATILE,
                        KEY_ALL_ACCESS,
                        NULL,
                        &hUsersKey,
                        &dwDisposition))
    {
        ERR("Failed to create 'Account\\Users' key! (Error %lu)\n", GetLastError());
        RegCloseKey(hAccountKey);
        RegCloseKey(hDomainsKey);
        return FALSE;
    }

    RegCloseKey(hUsersKey);

    RegCloseKey(hAccountKey);


    /* Create the 'Domains\\Builtin' */
    if (RegCreateKeyExW(hDomainsKey,
                        L"Builtin",
                        0,
                        NULL,
                        REG_OPTION_NON_VOLATILE,
                        KEY_ALL_ACCESS,
                        NULL,
                        &hBuiltinKey,
                        &dwDisposition))
    {
        ERR("Failed to create Builtin key! (Error %lu)\n", GetLastError());
        RegCloseKey(hDomainsKey);
        return FALSE;
    }


    /* Create the 'Builtin\Aliases' key */
    if (RegCreateKeyExW(hBuiltinKey,
                        L"Aliases",
                        0,
                        NULL,
                        REG_OPTION_NON_VOLATILE,
                        KEY_ALL_ACCESS,
                        NULL,
                        &hAliasesKey,
                        &dwDisposition))
    {
        ERR("Failed to create 'Builtin\\Aliases' key! (Error %lu)\n", GetLastError());
        RegCloseKey(hBuiltinKey);
        RegCloseKey(hDomainsKey);
        return FALSE;
    }

    /* Create builtin aliases */
    if (!CreateBuiltinAliases(hAliasesKey))
    {
        ERR("Failed to create builtin aliases!\n");
        RegCloseKey(hAliasesKey);
        RegCloseKey(hBuiltinKey);
        RegCloseKey(hDomainsKey);
        return FALSE;
    }

    RegCloseKey(hAliasesKey);


    /* Create the 'Builtin\Groups' key */
    if (RegCreateKeyExW(hBuiltinKey,
                        L"Groups",
                        0,
                        NULL,
                        REG_OPTION_NON_VOLATILE,
                        KEY_ALL_ACCESS,
                        NULL,
                        &hGroupsKey,
                        &dwDisposition))
    {
        ERR("Failed to create 'Builtin\\Groups' key! (Error %lu)\n", GetLastError());
        RegCloseKey(hBuiltinKey);
        RegCloseKey(hDomainsKey);
        return FALSE;
    }

    /* Create builtin groups */
    if (!CreateBuiltinGroups(hGroupsKey))
    {
        ERR("Failed to create builtin groups!\n");
        RegCloseKey(hGroupsKey);
        RegCloseKey(hBuiltinKey);
        RegCloseKey(hDomainsKey);
        return FALSE;
    }

    RegCloseKey(hGroupsKey);


    /* Create the 'Builtin\Users' key */
    if (RegCreateKeyExW(hBuiltinKey,
                        L"Users",
                        0,
                        NULL,
                        REG_OPTION_NON_VOLATILE,
                        KEY_ALL_ACCESS,
                        NULL,
                        &hUsersKey,
                        &dwDisposition))
    {
        ERR("Failed to create 'Builtin\\Users' key! (Error %lu)\n", GetLastError());
        RegCloseKey(hBuiltinKey);
        RegCloseKey(hDomainsKey);
        return FALSE;
    }

    /* Create builtin users */
    if (!CreateBuiltinUsers(hUsersKey))
    {
        ERR("Failed to create builtin users!\n");
        RegCloseKey(hUsersKey);
        RegCloseKey(hBuiltinKey);
        RegCloseKey(hDomainsKey);
        return FALSE;
    }

    RegCloseKey(hUsersKey);

    RegCloseKey(hBuiltinKey);

    RegCloseKey(hDomainsKey);

    TRACE("SampInitializeSAM() done\n");

    return TRUE;
}
