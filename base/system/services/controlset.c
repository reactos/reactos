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

static DWORD dwCurrentControlSet;
static DWORD dwDefaultControlSet;
static DWORD dwFailedControlSet;
static DWORD dwLastKnownGoodControlSet;


/* FUNCTIONS *****************************************************************/

static
DWORD
ScmCopyKey(HKEY hDstKey,
           HKEY hSrcKey)
{
#if (_WIN32_WINNT >= 0x0600)
    return RegCopyTreeW(hSrcKey,
                        NULL,
                        hDstKey);
#else
    FILETIME LastWrite;
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

    DPRINT("ScmCopyKey()\n");

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
                                   &LastWrite);
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

            dwError = ScmCopyKey(hDstSubKey,
                                hSrcSubKey);
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

        lpDataBuffer = HeapAlloc(GetProcessHeap(),
                                 0,
                                 dwMaxValueLength);
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

    DPRINT("ScmCopyKey() done \n");

    return ERROR_SUCCESS;
#endif
}


static
BOOL
ScmGetControlSetValues(VOID)
{
    HKEY hSelectKey;
    DWORD dwType;
    DWORD dwSize;
    LONG lError;

    DPRINT("ScmGetControlSetValues() called\n");

    lError = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                           L"System\\Select",
                           0,
                           KEY_READ,
                           &hSelectKey);
    if (lError != ERROR_SUCCESS)
        return FALSE;

    dwSize = sizeof(DWORD);
    lError = RegQueryValueExW(hSelectKey,
                              L"Current",
                              0,
                              &dwType,
                              (LPBYTE)&dwCurrentControlSet,
                              &dwSize);
    if (lError != ERROR_SUCCESS)
    {
        dwCurrentControlSet = 0;
    }

    dwSize = sizeof(DWORD);
    lError = RegQueryValueExW(hSelectKey,
                              L"Default",
                              0,
                              &dwType,
                              (LPBYTE)&dwDefaultControlSet,
                              &dwSize);
    if (lError != ERROR_SUCCESS)
    {
        dwDefaultControlSet = 0;
    }

    dwSize = sizeof(DWORD);
    lError = RegQueryValueExW(hSelectKey,
                              L"Failed",
                              0,
                              &dwType,
                              (LPBYTE)&dwFailedControlSet,
                              &dwSize);
    if (lError != ERROR_SUCCESS)
    {
        dwFailedControlSet = 0;
    }

    dwSize = sizeof(DWORD);
    lError = RegQueryValueExW(hSelectKey,
                              L"LastKnownGood",
                              0,
                              &dwType,
                              (LPBYTE)&dwLastKnownGoodControlSet,
                              &dwSize);
    if (lError != ERROR_SUCCESS)
    {
        dwLastKnownGoodControlSet = 0;
    }

    RegCloseKey(hSelectKey);

    DPRINT("ControlSets:\n");
    DPRINT("Current: %lu\n", dwCurrentControlSet);
    DPRINT("Default: %lu\n", dwDefaultControlSet);
    DPRINT("Failed: %lu\n", dwFailedControlSet);
    DPRINT("LastKnownGood: %lu\n", dwLastKnownGoodControlSet);

    return TRUE;
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


BOOL
ScmUpdateControlSets(VOID)
{
    WCHAR szCurrentControlSetName[32];
    WCHAR szNewControlSetName[32];
    HKEY hCurrentControlSetKey = NULL;
    HKEY hNewControlSetKey = NULL;
    DWORD dwNewControlSet, dwDisposition;
    DWORD dwError;

    /* Do not create a new control set when the system setup is running */
    if (ScmGetSetupInProgress() != 0)
    {
        DPRINT1("No new control set because we are in setup mode!\n");
        return TRUE;
    }

    /* Get the control set values */
    if (!ScmGetControlSetValues())
        return FALSE;

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
        return FALSE;
    }

    /* Create the current control set name */
    swprintf(szCurrentControlSetName, L"SYSTEM\\ControlSet%03lu", dwCurrentControlSet);
    DPRINT("Current control set: %S\n", szCurrentControlSetName);

    /* Create the new control set name */
    swprintf(szNewControlSetName, L"SYSTEM\\ControlSet%03lu", dwNewControlSet);
    DPRINT("New control set: %S\n", szNewControlSetName);

    /* Open the current control set key */
    dwError = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                            szCurrentControlSetName,
                            0,
                            KEY_READ,
                            &hCurrentControlSetKey);
    if (dwError != ERROR_SUCCESS)
        goto done;

    /* Create the new control set key */
    dwError = RegCreateKeyExW(HKEY_LOCAL_MACHINE,
                              szNewControlSetName,
                              0,
                              NULL,
                              REG_OPTION_NON_VOLATILE,
                              KEY_WRITE,
                              NULL,
                              &hNewControlSetKey,
                              &dwDisposition);
    if (dwError != ERROR_SUCCESS)
        goto done;

    /* Copy the current control set to the new control set */
    dwError = ScmCopyKey(hNewControlSetKey,
                         hCurrentControlSetKey);
    if (dwError != ERROR_SUCCESS)
        goto done;

    /* Set the new 'LastKnownGood' control set */
    dwError = ScmSetLastKnownGoodControlSet(dwNewControlSet);

done:
    if (hNewControlSetKey != NULL)
        RegCloseKey(hNewControlSetKey);

    if (hCurrentControlSetKey != NULL)
        RegCloseKey(hCurrentControlSetKey);

    return (dwError == ERROR_SUCCESS);
}

/* EOF */
