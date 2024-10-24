/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Winlogon
 * FILE:            base/system/winlogon/setup.c
 * PURPOSE:         Setup support functions
 * PROGRAMMERS:     Eric Kohl
 */

/* INCLUDES *****************************************************************/

#include "winlogon.h"
#include <ndk/setypes.h>

SETUP_TYPE g_setupType = SetupType_None;

/* FUNCTIONS ****************************************************************/

SETUP_TYPE
GetSetupType(VOID)
{
    HKEY hKey;
    DWORD dwError;
    DWORD dwType;
    DWORD dwSize;
    DWORD dwSetupType;

    /* Open key */
    dwError = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                            L"SYSTEM\\Setup", // REGSTR_KEY_SYSTEM REGSTR_KEY_SETUP
                            0,
                            KEY_QUERY_VALUE,
                            &hKey);
    if (dwError != ERROR_SUCCESS)
        return 0;

    /* Read value */
    dwSize = sizeof(dwSetupType);
    dwError = RegQueryValueExW(hKey,
                               L"SetupType",
                               NULL,
                               &dwType,
                               (LPBYTE)&dwSetupType,
                               &dwSize);

    /* Close key and check if returned values are correct */
    RegCloseKey(hKey);
    if (dwError != ERROR_SUCCESS || dwType != REG_DWORD || dwSize != sizeof(DWORD))
        return 0;

    TRACE("GetSetupType() returns %lu\n", dwSetupType);
    return (SETUP_TYPE)dwSetupType;
}

// Partially reverts commit c88c4b4ad (r28194)
static BOOL
SetSetupType(
    _In_ DWORD dwSetupType)
{
    HKEY hKey;
    DWORD dwError;

    /* Open key */
    dwError = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                            L"SYSTEM\\Setup",
                            0,
                            KEY_SET_VALUE,
                            &hKey);
    if (dwError != ERROR_SUCCESS)
        return FALSE;

    /* Write value */
    dwError = RegSetValueExW(hKey,
                             L"SetupType",
                             0,
                             REG_DWORD,
                             (LPBYTE)&dwSetupType,
                             sizeof(dwSetupType));

    /* Close key and check for success */
    RegCloseKey(hKey);
    return (dwError == ERROR_SUCCESS);
}

static BOOL
IsSetupShutdownRequired(
    _Out_ SHUTDOWN_ACTION* pShutdownAction)
{
    HKEY hKey;
    DWORD dwError;
    DWORD dwType;
    DWORD dwSize;
    DWORD dwValue;

    /* Open key */
    dwError = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                            L"SYSTEM\\Setup",  // REGSTR_KEY_SYSTEM REGSTR_KEY_SETUP
                            0,
                            KEY_QUERY_VALUE | KEY_SET_VALUE,
                            &hKey);
    if (dwError != ERROR_SUCCESS)
        return FALSE;

    /* Read value */
    dwSize = sizeof(dwValue);
    dwError = RegQueryValueExW(hKey,
                               L"SetupShutdownRequired",
                               NULL,
                               &dwType,
                               (LPBYTE)&dwValue,
                               &dwSize);

    /* Close key and check if returned values are correct */
    RegCloseKey(hKey);
    if (dwError != ERROR_SUCCESS || dwType != REG_DWORD || dwSize != sizeof(DWORD))
        return FALSE;

    /* Delete the value */
    RegDeleteValueW(hKey, L"SetupShutdownRequired");

    TRACE("IsSetupShutdownRequired() returns %lu\n", dwValue);
    *pShutdownAction = dwValue;
    return TRUE;
}

// FIXME: Make the function generic -- see sas.c!HandleShutdown()
static VOID
DoSetupShutdown(
    _In_ SHUTDOWN_ACTION Action)
{
    BOOLEAN Old;

#if DBG
    static const PCSTR s_pszShutdownAction[] =
        { "Shutting down", "Restarting", "Powering off"};

    ERR("WL: %s NT...\n", s_pszShutdownAction[Action]);
#endif

    RtlAdjustPrivilege(SE_SHUTDOWN_PRIVILEGE, TRUE, FALSE, &Old);
    NtShutdownSystem(Action);
    RtlAdjustPrivilege(SE_SHUTDOWN_PRIVILEGE, Old, FALSE, &Old);
    ExitProcess(0);
}


static DWORD
WINAPI
RunSetupThreadProc(
    IN LPVOID lpParameter)
{
    PROCESS_INFORMATION ProcessInformation;
    STARTUPINFOW StartupInfo;
    WCHAR Shell[MAX_PATH];
    WCHAR CommandLine[MAX_PATH];
    BOOL Result;
    DWORD dwError;
    HKEY hKey;
    DWORD dwType;
    DWORD dwSize;
    DWORD dwExitCode;

    TRACE("RunSetup() called\n");

    /* Open key */
    dwError = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                            L"SYSTEM\\Setup",
                            0,
                            KEY_QUERY_VALUE,
                            &hKey);
    if (dwError != ERROR_SUCCESS)
        return FALSE;

    /* Read key */
    dwSize = sizeof(Shell);
    dwError = RegQueryValueExW(hKey,
                               L"CmdLine",
                               NULL,
                               &dwType,
                               (LPBYTE)Shell,
                               &dwSize);
    RegCloseKey(hKey);
    if (dwError != ERROR_SUCCESS)
        return FALSE;

    /* Finish string */
    Shell[dwSize / sizeof(WCHAR)] = UNICODE_NULL;

    /* Expand string (if applicable) */
    if (dwType == REG_EXPAND_SZ)
        ExpandEnvironmentStringsW(Shell, CommandLine, ARRAYSIZE(CommandLine));
    else if (dwType == REG_SZ)
        wcscpy(CommandLine, Shell);
    else
        return FALSE;

    TRACE("Should run '%s' now\n", debugstr_w(CommandLine));

    // SwitchDesktop(WLSession->ApplicationDesktop);

    /* Start process */
    StartupInfo.cb = sizeof(StartupInfo);
    StartupInfo.lpReserved = NULL;
    StartupInfo.lpDesktop = L"WinSta0\\Default";
    StartupInfo.lpTitle = NULL;
    StartupInfo.dwFlags = 0;
    StartupInfo.cbReserved2 = 0;
    StartupInfo.lpReserved2 = 0;

    Result = CreateProcessW(NULL,
                            CommandLine,
                            NULL,
                            NULL,
                            FALSE,
                            DETACHED_PROCESS,
                            NULL,
                            NULL,
                            &StartupInfo,
                            &ProcessInformation);
    if (!Result)
    {
        ERR("Failed to run setup process '%s'\n", debugstr_w(CommandLine));
        // SwitchDesktop(WLSession->WinlogonDesktop);
        return FALSE;
    }

    /* Wait for process termination */
    WaitForSingleObject(ProcessInformation.hProcess, INFINITE);

    GetExitCodeProcess(ProcessInformation.hProcess, &dwExitCode);

    /* Close handles */
    CloseHandle(ProcessInformation.hThread);
    CloseHandle(ProcessInformation.hProcess);

    /* Reset the current setup type */
    if (dwExitCode == 0)
        SetSetupType(SetupType_None);

    // // SwitchDesktop(WLSession->WinlogonDesktop);

    TRACE("RunSetup() done\n");
    return TRUE;
}

static BOOL
RunSetup(VOID)
{
    // HANDLE hThread;

    SwitchDesktop(WLSession->ApplicationDesktop);

#if 0
    hThread = CreateThread(NULL,
                           0,
                           RunSetupThreadProc,
                           NULL,
                           0,
                           NULL);
    if (hThread)
#else
    if (RunSetupThreadProc(NULL))
#endif
    {
        /* Message loop for the current thread */
        /* Pump and dispatch any input events */
        MSG msg;
        // while (MsgWaitForMultipleObjects(1, &hThread, FALSE, INFINITE, QS_ALLINPUT))
        // // if (dwRet == WAIT_OBJECT_0 + 1)
        {
            while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE))
            {
                TranslateMessage(&msg);
                DispatchMessageW(&msg);
            }
        }

        // CloseHandle(hThread);
    }

    SwitchDesktop(WLSession->WinlogonDesktop);

    // return (hThread != NULL);
    return TRUE;
}


VOID
CheckForSetup(VOID)
{
    TRACE("CheckForSetup() called\n");

    /* Check for pending setup */
    switch (g_setupType)
    {
    case SetupType_None:
        /* Nothing to do */
        break;

    case SetupType_Full: case SetupType_Upgrade:
    case SetupType_OOBE:
    {
        SHUTDOWN_ACTION shutdownAction = ShutdownReboot;

#if DBG
        static const PCSTR pszSetupType[] =
            { "None", "", "OOBE", "Reserved", "Upgrade" };

        TRACE("WL: %s%sSetup mode detected\n",
              pszSetupType[g_setupType], pszSetupType[g_setupType][0] ? " " : "");
#endif

        /*
         * We currently support three types of setup actions:
         *
         * - Full setup or upgrade: run it and reboot when done
         *   or if the setup program crashed for whatever reason;
         *
         * - OOBE setup: run it, then check whether it requested
         *   a power action and initiate it if so, otherwise
         *   continue with regular logon.
         */
        RunSetup();

        /* Reboot only if needed */
        if ((g_setupType == SetupType_Full) || (g_setupType == SetupType_Upgrade) ||
            /*(g_setupType == SetupType_OOBE) &&*/IsSetupShutdownRequired(&shutdownAction))
        {
            DoSetupShutdown(shutdownAction);
        }

        break;
    }

    case SetupType_Reserved:
    default:
        /* Unknown setup type: ignore */
        WARN("WL: Unknown Setup type %lu, ignored\n", g_setupType);
        break;
    }
}

/* EOF */
