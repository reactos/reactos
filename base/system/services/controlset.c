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

LSTATUS WINAPI RegCopyTreeW(_In_ HKEY, _In_opt_ LPCWSTR, _In_ HKEY);
LSTATUS WINAPI RegDeleteTreeW(_In_ HKEY, _In_opt_ LPCWSTR);

/* GLOBALS *******************************************************************/

static BOOL bBootAccepted = FALSE;


/* FUNCTIONS *****************************************************************/

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
    _swprintf(szSourceControlSetName, L"SYSTEM\\ControlSet%03lu", dwSourceControlSet);
    DPRINT("Source control set: %S\n", szSourceControlSetName);

    /* Create the destination control set name */
    _swprintf(szDestinationControlSetName, L"SYSTEM\\ControlSet%03lu", dwDestinationControlSet);
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
    dwError = RegCopyTreeW(hSourceControlSetKey,
                           NULL,
                           hDestinationControlSetKey);
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
    _swprintf(szControlSetName, L"SYSTEM\\ControlSet%03lu", dwControlSet);
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
    dwError = RegDeleteTreeW(hControlSetKey,
                             NULL);

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
        DPRINT("First boot after setup\n");

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
            DPRINT1("Too many control sets\n");
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
        DPRINT1("Boot has already been accepted\n");
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
        DPRINT1("Too many control sets\n");
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
        DPRINT1("Boot has already been accepted\n");
        return ERROR_BOOT_ALREADY_ACCEPTED;
    }

    /* FIXME */

    return ERROR_SUCCESS;
}

/* EOF */
