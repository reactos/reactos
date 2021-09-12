/*
 * PROJECT:     ReactOS Service Control Manager
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        base/system/services/controlset.c
 * PURPOSE:     Control Set Management
 * COPYRIGHT:   Copyright 2012 Eric Kohl
 *
 */

/* INCLUDES *****************************************************************/

#include "services.h"

#define NDEBUG
#include <debug.h>


/* GLOBALS *******************************************************************/

static BOOL bBootAccepted = FALSE;


/* FUNCTIONS *****************************************************************/

#if (_WIN32_WINNT < 0x0600)
static
DWORD
ScmCopyTree(
    HKEY hSrcKey,
    HKEY hDstKey)
{
    DWORD dwSubKeys;
    DWORD dwValues;
    DWORD dwType;
    DWORD dwMaxSubKeyNameLength;
    DWORD dwSubKeyNameLength;
    DWORD dwMaxValueNameLength;
    DWORD dwValueNameLength;
    DWORD dwMaxValueLength;
    DWORD dwValueLength;
    DWORD dwDisposition;
    DWORD i;
    LPWSTR lpNameBuffer;
    LPBYTE lpDataBuffer;
    HKEY hDstSubKey;
    HKEY hSrcSubKey;
    DWORD dwError;

    DPRINT("ScmCopyTree()\n");

    dwError = RegQueryInfoKey(hSrcKey,
                              NULL,
                              NULL,
                              NULL,
                              &dwSubKeys,
                              &dwMaxSubKeyNameLength,
                              NULL,
                              &dwValues,
                              &dwMaxValueNameLength,
                              &dwMaxValueLength,
                              NULL,
                              NULL);
    if (dwError != ERROR_SUCCESS)
    {
        DPRINT1("RegQueryInfoKey() failed (Error %lu)\n", dwError);
        return dwError;
    }

    dwMaxSubKeyNameLength++;
    dwMaxValueNameLength++;

    DPRINT("dwSubKeys %lu\n", dwSubKeys);
    DPRINT("dwMaxSubKeyNameLength %lu\n", dwMaxSubKeyNameLength);
    DPRINT("dwValues %lu\n", dwValues);
    DPRINT("dwMaxValueNameLength %lu\n", dwMaxValueNameLength);
    DPRINT("dwMaxValueLength %lu\n", dwMaxValueLength);

    /* Copy subkeys */
    if (dwSubKeys != 0)
    {
        lpNameBuffer = HeapAlloc(GetProcessHeap(),
                                 0,
                                 dwMaxSubKeyNameLength * sizeof(WCHAR));
        if (lpNameBuffer == NULL)
        {
            DPRINT1("Buffer allocation failed\n");
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        for (i = 0; i < dwSubKeys; i++)
        {
            dwSubKeyNameLength = dwMaxSubKeyNameLength;
            dwError = RegEnumKeyExW(hSrcKey,
                                    i,
                                    lpNameBuffer,
                                    &dwSubKeyNameLength,
                                    NULL,
                                    NULL,
                                    NULL,
                                    NULL);
            if (dwError != ERROR_SUCCESS)
            {
                DPRINT1("Subkey enumeration failed (Error %lu)\n", dwError);
                HeapFree(GetProcessHeap(),
                         0,
                         lpNameBuffer);
                return dwError;
            }

            dwError = RegCreateKeyExW(hDstKey,
                                      lpNameBuffer,
                                      0,
                                      NULL,
                                      REG_OPTION_NON_VOLATILE,
                                      KEY_WRITE,
                                      NULL,
                                      &hDstSubKey,
                                      &dwDisposition);
            if (dwError != ERROR_SUCCESS)
            {
                DPRINT1("Subkey creation failed (Error %lu)\n", dwError);
                HeapFree(GetProcessHeap(),
                         0,
                         lpNameBuffer);
                return dwError;
            }

            dwError = RegOpenKeyExW(hSrcKey,
                                    lpNameBuffer,
                                    0,
                                    KEY_READ,
                                    &hSrcSubKey);
            if (dwError != ERROR_SUCCESS)
            {
                DPRINT1("Error: %lu\n", dwError);
                RegCloseKey(hDstSubKey);
                HeapFree(GetProcessHeap(),
                         0,
                         lpNameBuffer);
                return dwError;
            }

            dwError = ScmCopyTree(hSrcSubKey,
                                  hDstSubKey);
            if (dwError != ERROR_SUCCESS)
            {
                DPRINT1("Error: %lu\n", dwError);
                RegCloseKey (hSrcSubKey);
                RegCloseKey (hDstSubKey);
                HeapFree(GetProcessHeap(),
                         0,
                         lpNameBuffer);
                return dwError;
            }

            RegCloseKey(hSrcSubKey);
            RegCloseKey(hDstSubKey);
        }

        HeapFree(GetProcessHeap(),
                 0,
                 lpNameBuffer);
    }

    /* Copy values */
    if (dwValues != 0)
    {
        lpNameBuffer = HeapAlloc(GetProcessHeap(),
                                 0,
                                 dwMaxValueNameLength * sizeof(WCHAR));
        if (lpNameBuffer == NULL)
        {
            DPRINT1("Buffer allocation failed\n");
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        /* RegSetValueExW tries to read behind the maximum length, so give it space for that */
        lpDataBuffer = HeapAlloc(GetProcessHeap(),
                                 HEAP_ZERO_MEMORY,
                                 dwMaxValueLength + sizeof(WCHAR));
        if (lpDataBuffer == NULL)
        {
            DPRINT1("Buffer allocation failed\n");
            HeapFree(GetProcessHeap(),
                     0,
                     lpNameBuffer);
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        for (i = 0; i < dwValues; i++)
        {
            dwValueNameLength = dwMaxValueNameLength;
            dwValueLength = dwMaxValueLength;
            dwError = RegEnumValueW(hSrcKey,
                                    i,
                                    lpNameBuffer,
                                    &dwValueNameLength,
                                    NULL,
                                    &dwType,
                                    lpDataBuffer,
                                    &dwValueLength);
            if (dwError != ERROR_SUCCESS)
            {
                DPRINT1("Error: %lu\n", dwError);
                HeapFree(GetProcessHeap(),
                         0,
                         lpDataBuffer);
                HeapFree(GetProcessHeap(),
                         0,
                         lpNameBuffer);
                return dwError;
            }

            dwError = RegSetValueExW(hDstKey,
                                     lpNameBuffer,
                                     0,
                                     dwType,
                                     lpDataBuffer,
                                     dwValueLength);
            if (dwError != ERROR_SUCCESS)
            {
                DPRINT1("Error: %lu\n", dwError);
                HeapFree(GetProcessHeap(),
                         0,
                         lpDataBuffer);
                HeapFree(GetProcessHeap(),
                         0,
                         lpNameBuffer);
                return dwError;
            }
        }

        HeapFree(GetProcessHeap(),
                 0,
                 lpDataBuffer);

        HeapFree(GetProcessHeap(),
                 0,
                 lpNameBuffer);
    }

    DPRINT("ScmCopyTree() done\n");

    return ERROR_SUCCESS;
}


DWORD
ScmDeleteTree(
    HKEY hKey,
    PCWSTR pszSubKey)
{
    DWORD dwMaxSubkeyLength, dwMaxValueLength;
    DWORD dwMaxLength, dwSize;
    PWSTR pszName = NULL;
    HKEY hSubKey = NULL;
    DWORD dwError;

    if (pszSubKey != NULL)
    {
        dwError = RegOpenKeyExW(hKey, pszSubKey, 0, KEY_READ, &hSubKey);
        if (dwError != ERROR_SUCCESS)
            return dwError;
    }
    else
    {
         hSubKey = hKey;
    }

    /* Get highest length for keys, values */
    dwError = RegQueryInfoKeyW(hSubKey,
                               NULL,
                               NULL,
                               NULL,
                               NULL,
                               &dwMaxSubkeyLength,
                               NULL,
                               NULL,
                               &dwMaxValueLength,
                               NULL,
                               NULL,
                               NULL);
    if (dwError != ERROR_SUCCESS)
        goto done;

    dwMaxSubkeyLength++;
    dwMaxValueLength++;
    dwMaxLength = max(dwMaxSubkeyLength, dwMaxValueLength);

    /* Allocate a buffer for key and value names */
    pszName = HeapAlloc(GetProcessHeap(),
                         0,
                         dwMaxLength * sizeof(WCHAR));
    if (pszName == NULL)
    {
        dwError = ERROR_NOT_ENOUGH_MEMORY;
        goto done;
    }

    /* Recursively delete all the subkeys */
    while (TRUE)
    {
        dwSize = dwMaxLength;
        if (RegEnumKeyExW(hSubKey,
                          0,
                          pszName,
                          &dwSize,
                          NULL,
                          NULL,
                          NULL,
                          NULL))
            break;

        dwError = ScmDeleteTree(hSubKey, pszName);
        if (dwError != ERROR_SUCCESS)
            goto done;
    }

    if (pszSubKey != NULL)
    {
        dwError = RegDeleteKeyW(hKey, pszSubKey);
    }
    else
    {
        while (TRUE)
        {
            dwSize = dwMaxLength;
            if (RegEnumValueW(hKey,
                              0,
                              pszName,
                              &dwSize,
                              NULL,
                              NULL,
                              NULL,
                              NULL))
                break;

            dwError = RegDeleteValueW(hKey, pszName);
            if (dwError != ERROR_SUCCESS)
                goto done;
        }
    }

done:
    if (pszName != NULL)
        HeapFree(GetProcessHeap(), 0, pszName);

    if (pszSubKey != NULL)
        RegCloseKey(hSubKey);

    return dwError;
}
#endif


static
DWORD
ScmGetControlSetValues(
    PDWORD pdwCurrentControlSet,
    PDWORD pdwDefaultControlSet,
    PDWORD pdwFailedControlSet,
    PDWORD pdwLastKnownGoodControlSet)
{
    HKEY hSelectKey;
    DWORD dwType;
    DWORD dwSize;
    DWORD dwError;

    DPRINT("ScmGetControlSetValues() called\n");

    dwError = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                            L"System\\Select",
                            0,
                            KEY_READ,
                            &hSelectKey);
    if (dwError != ERROR_SUCCESS)
        return dwError;

    dwSize = sizeof(DWORD);
    dwError = RegQueryValueExW(hSelectKey,
                               L"Current",
                               0,
                               &dwType,
                               (LPBYTE)pdwCurrentControlSet,
                               &dwSize);
    if (dwError != ERROR_SUCCESS)
    {
        *pdwCurrentControlSet = 0;
    }

    dwSize = sizeof(DWORD);
    dwError = RegQueryValueExW(hSelectKey,
                               L"Default",
                               0,
                               &dwType,
                               (LPBYTE)pdwDefaultControlSet,
                               &dwSize);
    if (dwError != ERROR_SUCCESS)
    {
        *pdwDefaultControlSet = 0;
    }

    dwSize = sizeof(DWORD);
    dwError = RegQueryValueExW(hSelectKey,
                               L"Failed",
                               0,
                               &dwType,
                               (LPBYTE)pdwFailedControlSet,
                               &dwSize);
    if (dwError != ERROR_SUCCESS)
    {
        *pdwFailedControlSet = 0;
    }

    dwSize = sizeof(DWORD);
    dwError = RegQueryValueExW(hSelectKey,
                               L"LastKnownGood",
                               0,
                               &dwType,
                               (LPBYTE)pdwLastKnownGoodControlSet,
                               &dwSize);
    if (dwError != ERROR_SUCCESS)
    {
        *pdwLastKnownGoodControlSet = 0;
    }

    RegCloseKey(hSelectKey);

    DPRINT("ControlSets:\n");
    DPRINT("Current: %lu\n", *pdwCurrentControlSet);
    DPRINT("Default: %lu\n", *pdwDefaultControlSet);
    DPRINT("Failed: %lu\n", *pdwFailedControlSet);
    DPRINT("LastKnownGood: %lu\n", *pdwLastKnownGoodControlSet);

    return dwError;
}


static
DWORD
ScmSetLastKnownGoodControlSet(
    DWORD dwControlSet)
{
    HKEY hSelectKey;
    DWORD dwError;

    dwError = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                            L"System\\Select",
                            0,
                            KEY_WRITE,
                            &hSelectKey);
    if (dwError != ERROR_SUCCESS)
        return dwError;

    dwError = RegSetValueExW(hSelectKey,
                             L"LastKnownGood",
                             0,
                             REG_DWORD,
                             (LPBYTE)&dwControlSet,
                             sizeof(dwControlSet));

    RegFlushKey(hSelectKey);
    RegCloseKey(hSelectKey);

    return dwError;
}


static
DWORD
ScmGetSetupInProgress(VOID)
{
    DWORD dwError;
    HKEY hKey;
    DWORD dwType;
    DWORD dwSize;
    DWORD dwSetupInProgress = (DWORD)-1;

    DPRINT("ScmGetSetupInProgress()\n");

    /* Open key */
    dwError = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                            L"SYSTEM\\Setup",
                            0,
                            KEY_QUERY_VALUE,
                            &hKey);
    if (dwError == ERROR_SUCCESS)
    {
        /* Read key */
        dwSize = sizeof(DWORD);
        RegQueryValueExW(hKey,
                         L"SystemSetupInProgress",
                         NULL,
                         &dwType,
                         (LPBYTE)&dwSetupInProgress,
                         &dwSize);
        RegCloseKey(hKey);
    }

    DPRINT("SetupInProgress: %lu\n", dwSetupInProgress);
    return dwSetupInProgress;
}


static
DWORD
ScmCopyControlSet(
    DWORD dwSourceControlSet,
    DWORD dwDestinationControlSet)
{
    WCHAR szSourceControlSetName[32];
    WCHAR szDestinationControlSetName[32];
    HKEY hSourceControlSetKey = NULL;
    HKEY hDestinationControlSetKey = NULL;
    DWORD dwDisposition;
    DWORD dwError;

    /* Create the source control set name */
    swprintf(szSourceControlSetName, L"SYSTEM\\ControlSet%03lu", dwSourceControlSet);
    DPRINT("Source control set: %S\n", szSourceControlSetName);

    /* Create the destination control set name */
    swprintf(szDestinationControlSetName, L"SYSTEM\\ControlSet%03lu", dwDestinationControlSet);
    DPRINT("Destination control set: %S\n", szDestinationControlSetName);

    /* Open the source control set key */
    dwError = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                            szSourceControlSetName,
                            0,
                            KEY_READ,
                            &hSourceControlSetKey);
    if (dwError != ERROR_SUCCESS)
        goto done;

    /* Create the destination control set key */
    dwError = RegCreateKeyExW(HKEY_LOCAL_MACHINE,
                              szDestinationControlSetName,
                              0,
                              NULL,
                              REG_OPTION_NON_VOLATILE,
                              KEY_WRITE,
                              NULL,
                              &hDestinationControlSetKey,
                              &dwDisposition);
    if (dwError != ERROR_SUCCESS)
        goto done;

    /* Copy the source control set to the destination control set */
#if (_WIN32_WINNT >= 0x0600)
    dwError = RegCopyTreeW(hSourceControlSetKey,
                           NULL,
                           hDestinationControlSetKey);
#else
    dwError = ScmCopyTree(hSourceControlSetKey,
                          hDestinationControlSetKey);
#endif
    if (dwError != ERROR_SUCCESS)
        goto done;

    RegFlushKey(hDestinationControlSetKey);

done:
    if (hDestinationControlSetKey != NULL)
        RegCloseKey(hDestinationControlSetKey);

    if (hSourceControlSetKey != NULL)
        RegCloseKey(hSourceControlSetKey);

    return dwError;
}


static
DWORD
ScmDeleteControlSet(
    DWORD dwControlSet)
{
    WCHAR szControlSetName[32];
    HKEY hControlSetKey = NULL;
    DWORD dwError;

    DPRINT("ScmDeleteControSet(%lu)\n", dwControlSet);

    /* Create the control set name */
    swprintf(szControlSetName, L"SYSTEM\\ControlSet%03lu", dwControlSet);
    DPRINT("Control set: %S\n", szControlSetName);

    /* Open the system key */
    dwError = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                            szControlSetName,
                            0,
                            DELETE | KEY_ENUMERATE_SUB_KEYS | KEY_QUERY_VALUE | KEY_SET_VALUE,
                            &hControlSetKey);
    if (dwError != ERROR_SUCCESS)
        return dwError;

    /* Delete the control set */
#if (_WIN32_WINNT >= 0x0600)
    dwError = RegDeleteTreeW(hControlSetKey,
                             NULL);
#else
    dwError = ScmDeleteTree(hControlSetKey,
                            NULL);
#endif

    /* Open the system key */
    RegCloseKey(hControlSetKey);

    return dwError;
}


DWORD
ScmCreateLastKnownGoodControlSet(VOID)
{
    DWORD dwCurrentControlSet, dwDefaultControlSet;
    DWORD dwFailedControlSet, dwLastKnownGoodControlSet;
    DWORD dwNewControlSet;
    DWORD dwError;

    /* Get the control set values */
    dwError = ScmGetControlSetValues(&dwCurrentControlSet,
                                     &dwDefaultControlSet,
                                     &dwFailedControlSet,
                                     &dwLastKnownGoodControlSet);
    if (dwError != ERROR_SUCCESS)
        return dwError;

    /* First boot after setup? */
    if ((ScmGetSetupInProgress() == 0) &&
        (dwCurrentControlSet == dwLastKnownGoodControlSet))
    {
        DPRINT("First boot after setup!\n");

        /* Search for a new control set number */
        for (dwNewControlSet = 1; dwNewControlSet < 1000; dwNewControlSet++)
        {
            if ((dwNewControlSet != dwCurrentControlSet) &&
                (dwNewControlSet != dwDefaultControlSet) &&
                (dwNewControlSet != dwFailedControlSet) &&
                (dwNewControlSet != dwLastKnownGoodControlSet))
                break;
        }

        /* Fail if we did not find an unused control set!*/
        if (dwNewControlSet >= 1000)
        {
            DPRINT1("Too many control sets!\n");
            return ERROR_NO_MORE_ITEMS;
        }

        /* Copy the current control set */
        dwError = ScmCopyControlSet(dwCurrentControlSet,
                                    dwNewControlSet);
        if (dwError != ERROR_SUCCESS)
            return dwError;

        /* Set the new 'LastKnownGood' control set */
        dwError = ScmSetLastKnownGoodControlSet(dwNewControlSet);
        if (dwError == ERROR_SUCCESS)
        {
            /*
             * Accept the boot here in order to prevent the creation of
             * another control set when a user is going to get logged on
             */
            bBootAccepted = TRUE;
        }
    }

    return dwError;
}


DWORD
ScmAcceptBoot(VOID)
{
    DWORD dwCurrentControlSet, dwDefaultControlSet;
    DWORD dwFailedControlSet, dwLastKnownGoodControlSet;
    DWORD dwNewControlSet;
    DWORD dwError;

    DPRINT("ScmAcceptBoot()\n");

    if (bBootAccepted)
    {
        DPRINT1("Boot has alread been accepted!\n");
        return ERROR_BOOT_ALREADY_ACCEPTED;
    }

    /* Get the control set values */
    dwError = ScmGetControlSetValues(&dwCurrentControlSet,
                                     &dwDefaultControlSet,
                                     &dwFailedControlSet,
                                     &dwLastKnownGoodControlSet);
    if (dwError != ERROR_SUCCESS)
        return dwError;

    /* Search for a new control set number */
    for (dwNewControlSet = 1; dwNewControlSet < 1000; dwNewControlSet++)
    {
        if ((dwNewControlSet != dwCurrentControlSet) &&
            (dwNewControlSet != dwDefaultControlSet) &&
            (dwNewControlSet != dwFailedControlSet) &&
            (dwNewControlSet != dwLastKnownGoodControlSet))
            break;
    }

    /* Fail if we did not find an unused control set!*/
    if (dwNewControlSet >= 1000)
    {
        DPRINT1("Too many control sets!\n");
        return ERROR_NO_MORE_ITEMS;
    }

    /* Copy the current control set */
    dwError = ScmCopyControlSet(dwCurrentControlSet,
                                dwNewControlSet);
    if (dwError != ERROR_SUCCESS)
        return dwError;

    /* Delete the current last known good contol set, if it is not used anywhere else */
    if ((dwLastKnownGoodControlSet != dwCurrentControlSet) &&
        (dwLastKnownGoodControlSet != dwDefaultControlSet) &&
        (dwLastKnownGoodControlSet != dwFailedControlSet))
    {
        ScmDeleteControlSet(dwLastKnownGoodControlSet);
    }

    /* Set the new 'LastKnownGood' control set */
    dwError = ScmSetLastKnownGoodControlSet(dwNewControlSet);
    if (dwError != ERROR_SUCCESS)
        return dwError;

    bBootAccepted = TRUE;

    return ERROR_SUCCESS;
}


DWORD
ScmRunLastKnownGood(VOID)
{
    DPRINT("ScmRunLastKnownGood()\n");

    if (bBootAccepted)
    {
        DPRINT1("Boot has alread been accepted!\n");
        return ERROR_BOOT_ALREADY_ACCEPTED;
    }

    /* FIXME */

    return ERROR_SUCCESS;
}

/* EOF */
