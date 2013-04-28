/*
 *  ReactOS kernel
 *  Copyright (C) 2003 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS system libraries
 * PURPOSE:           System setup
 * FILE:              dll/win32/syssetup/install.c
 * PROGRAMER:         Eric Kohl
 */

/* INCLUDES *****************************************************************/

#include "precomp.h"

#define NDEBUG
#include <debug.h>

DWORD WINAPI
CMP_WaitNoPendingInstallEvents(DWORD dwTimeout);

/* GLOBALS ******************************************************************/

HINF hSysSetupInf = INVALID_HANDLE_VALUE;

/* FUNCTIONS ****************************************************************/

static VOID
FatalError(char *pszFmt,...)
{
    char szBuffer[512];
    va_list ap;

    va_start(ap, pszFmt);
    vsprintf(szBuffer, pszFmt, ap);
    va_end(ap);

    LogItem(SYSSETUP_SEVERITY_FATAL_ERROR, L"Failed");

    strcat(szBuffer, "\nRebooting now!");
    MessageBoxA(NULL,
                szBuffer,
                "ReactOS Setup",
                MB_OK);
}

static HRESULT
CreateShellLink(
    LPCTSTR pszLinkPath,
    LPCTSTR pszCmd,
    LPCTSTR pszArg,
    LPCTSTR pszDir,
    LPCTSTR pszIconPath,
    int iIconNr,
    LPCTSTR pszComment)
{
    IShellLink *psl;
    IPersistFile *ppf;
#ifndef _UNICODE
    WCHAR wszBuf[MAX_PATH];
#endif /* _UNICODE */

    HRESULT hr = CoCreateInstance(&CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, &IID_IShellLink, (LPVOID*)&psl);

    if (SUCCEEDED(hr))
    {
        hr = psl->lpVtbl->SetPath(psl, pszCmd);

        if (pszArg)
        {
            hr = psl->lpVtbl->SetArguments(psl, pszArg);
        }

        if (pszDir)
        {
            hr = psl->lpVtbl->SetWorkingDirectory(psl, pszDir);
        }

        if (pszIconPath)
        {
            hr = psl->lpVtbl->SetIconLocation(psl, pszIconPath, iIconNr);
        }

        if (pszComment)
        {
            hr = psl->lpVtbl->SetDescription(psl, pszComment);
        }

        hr = psl->lpVtbl->QueryInterface(psl, &IID_IPersistFile, (LPVOID*)&ppf);

        if (SUCCEEDED(hr))
        {
#ifdef _UNICODE
            hr = ppf->lpVtbl->Save(ppf, pszLinkPath, TRUE);
#else /* _UNICODE */
            MultiByteToWideChar(CP_ACP, 0, pszLinkPath, -1, wszBuf, MAX_PATH);

            hr = ppf->lpVtbl->Save(ppf, wszBuf, TRUE);
#endif /* _UNICODE */

            ppf->lpVtbl->Release(ppf);
        }

        psl->lpVtbl->Release(psl);
    }

    return hr;
}


static BOOL
CreateShortcut(
    int csidl,
    LPCTSTR pszFolder,
    UINT nIdName,
    LPCTSTR pszCommand,
    UINT nIdTitle,
    BOOL bCheckExistence,
    INT iIconNr)
{
    TCHAR szPath[MAX_PATH];
    TCHAR szExeName[MAX_PATH];
    TCHAR szTitle[256];
    TCHAR szName[256];
    LPTSTR Ptr = szPath;
    TCHAR szWorkingDirBuf[MAX_PATH];
    LPTSTR pszWorkingDir = NULL;
    LPTSTR lpFilePart;
    DWORD dwLen;

    if (ExpandEnvironmentStrings(pszCommand,
                                 szPath,
                                 sizeof(szPath) / sizeof(szPath[0])) == 0)
    {
        _tcscpy(szPath, pszCommand);
    }

    if (bCheckExistence)
    {
        if ((_taccess(szPath, 0 )) == -1)
            /* Expected error, don't return FALSE */
            return TRUE;
    }

    dwLen = GetFullPathName(szPath,
                            sizeof(szWorkingDirBuf) / sizeof(szWorkingDirBuf[0]),
                            szWorkingDirBuf,
                            &lpFilePart);
    if (dwLen != 0 && dwLen <= sizeof(szWorkingDirBuf) / sizeof(szWorkingDirBuf[0]))
    {
        /* Since those should only be called with (.exe) files,
           lpFilePart has not to be NULL */
        ASSERT(lpFilePart != NULL);

        /* Save the file name */
        _tcscpy(szExeName, lpFilePart);

        /* We're only interested in the path. Cut the file name off.
           Also remove the trailing backslash unless the working directory
           is only going to be a drive, ie. C:\ */
        *(lpFilePart--) = _T('\0');
        if (!(lpFilePart - szWorkingDirBuf == 2 && szWorkingDirBuf[1] == _T(':') &&
              szWorkingDirBuf[2] == _T('\\')))
        {
            *lpFilePart = _T('\0');
        }

        pszWorkingDir = szWorkingDirBuf;
    }


    if (!SHGetSpecialFolderPath(0, szPath, csidl, TRUE))
        return FALSE;

    if (pszFolder)
    {
        Ptr = PathAddBackslash(Ptr);
        _tcscpy(Ptr, pszFolder);
    }

    Ptr = PathAddBackslash(Ptr);

    if (!LoadString(hDllInstance, nIdName, szName, sizeof(szName)/sizeof(szName[0])))
        return FALSE;
    _tcscpy(Ptr, szName);

    if (!LoadString(hDllInstance, nIdTitle, szTitle, sizeof(szTitle)/sizeof(szTitle[0])))
        return FALSE;

    // FIXME: we should pass 'command' straight in here, but shell32 doesn't expand it
    return SUCCEEDED(CreateShellLink(szPath, szExeName, _T(""), pszWorkingDir, szExeName, iIconNr, szTitle));
}

static BOOL
CreateShortcutFolder(int csidl, UINT nID, LPTSTR pszName, int cchNameLen)
{
    TCHAR szPath[MAX_PATH];
    LPTSTR p;

    if (!SHGetSpecialFolderPath(0, szPath, csidl, TRUE))
        return FALSE;

    if (!LoadString(hDllInstance, nID, pszName, cchNameLen))
        return FALSE;

    p = PathAddBackslash(szPath);
    _tcscpy(p, pszName);

    return CreateDirectory(szPath, NULL) || GetLastError()==ERROR_ALREADY_EXISTS;
}


static VOID
CreateTempDir(
    IN LPCWSTR VarName)
{
    WCHAR szTempDir[MAX_PATH];
    WCHAR szBuffer[MAX_PATH];
    DWORD dwLength;
    HKEY hKey;

    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                     L"SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment",
                     0,
                     KEY_QUERY_VALUE,
                     &hKey) != ERROR_SUCCESS)
    {
        FatalError("Error: %lu\n", GetLastError());
        return;
    }

    /* Get temp dir */
    dwLength = MAX_PATH * sizeof(WCHAR);
    if (RegQueryValueExW(hKey,
                        VarName,
                        NULL,
                        NULL,
                        (LPBYTE)szBuffer,
                        &dwLength) != ERROR_SUCCESS)
    {
        FatalError("Error: %lu\n", GetLastError());
        goto cleanup;
    }

    /* Expand it */
    if (!ExpandEnvironmentStringsW(szBuffer,
                                  szTempDir,
                                  MAX_PATH))
    {
        FatalError("Error: %lu\n", GetLastError());
        goto cleanup;
    }

    /* Create profiles directory */
    if (!CreateDirectoryW(szTempDir, NULL))
    {
        if (GetLastError() != ERROR_ALREADY_EXISTS)
        {
            FatalError("Error: %lu\n", GetLastError());
            goto cleanup;
        }
    }

cleanup:
    RegCloseKey(hKey);
}

static BOOL
InstallSysSetupInfDevices(VOID)
{
    INFCONTEXT InfContext;
    WCHAR szLineBuffer[256];
    DWORD dwLineLength;

    if (!SetupFindFirstLineW(hSysSetupInf,
                            L"DeviceInfsToInstall",
                            NULL,
                            &InfContext))
    {
        return FALSE;
    }

    do
    {
        if (!SetupGetStringFieldW(&InfContext,
                                 0,
                                 szLineBuffer,
                                 sizeof(szLineBuffer)/sizeof(szLineBuffer[0]),
                                 &dwLineLength))
        {
            return FALSE;
        }

        if (!SetupDiInstallClassW(NULL, szLineBuffer, DI_QUIETINSTALL, NULL))
        {
            return FALSE;
        }
    }
    while (SetupFindNextLine(&InfContext, &InfContext));

    return TRUE;
}

static BOOL
InstallSysSetupInfComponents(VOID)
{
    INFCONTEXT InfContext;
    WCHAR szNameBuffer[256];
    WCHAR szSectionBuffer[256];
    HINF hComponentInf = INVALID_HANDLE_VALUE;

    if (!SetupFindFirstLineW(hSysSetupInf,
                            L"Infs.Always",
                            NULL,
                            &InfContext))
    {
        DPRINT("No Inf.Always section found\n");
    }
    else
    {
        do
        {
            if (!SetupGetStringFieldW(&InfContext,
                                     1, // Get the component name
                                     szNameBuffer,
                                     sizeof(szNameBuffer)/sizeof(szNameBuffer[0]),
                                     NULL))
            {
                FatalError("Error while trying to get component name \n");
                return FALSE;
            }

            if (!SetupGetStringFieldW(&InfContext,
                                     2, // Get the component install section
                                     szSectionBuffer,
                                     sizeof(szSectionBuffer)/sizeof(szSectionBuffer[0]),
                                     NULL))
            {
                FatalError("Error while trying to get component install section \n");
                return FALSE;
            }

            DPRINT("Trying to execute install section '%S' from '%S' \n", szSectionBuffer, szNameBuffer);

            hComponentInf = SetupOpenInfFileW(szNameBuffer,
                                              NULL,
                                              INF_STYLE_WIN4,
                                              NULL);

            if (hComponentInf == INVALID_HANDLE_VALUE)
            {
                FatalError("SetupOpenInfFileW() failed to open '%S' (Error: %lu)\n", szNameBuffer, GetLastError());
                return FALSE;
            }

            if (!SetupInstallFromInfSectionW(NULL,
                                            hComponentInf,
                                            szSectionBuffer,
                                            SPINST_ALL,
                                            NULL,
                                            NULL,
                                            SP_COPY_NEWER,
                                            SetupDefaultQueueCallbackW,
                                            NULL,
                                            NULL,
                                            NULL))
           {
                FatalError("Error while trying to install : %S (Error: %lu)\n", szNameBuffer, GetLastError());
                SetupCloseInfFile(hComponentInf);
                return FALSE;
           }

           SetupCloseInfFile(hComponentInf);
        }
        while (SetupFindNextLine(&InfContext, &InfContext));
    }

    return TRUE;
}

static BOOL
EnableUserModePnpManager(VOID)
{
    SC_HANDLE hSCManager = NULL;
    SC_HANDLE hService = NULL;
    BOOL bRet = FALSE;

    hSCManager = OpenSCManagerW(NULL, NULL, SC_MANAGER_ENUMERATE_SERVICE);
    if (hSCManager == NULL)
    {
        DPRINT1("Unable to open the service control manager.\n");
        DPRINT1("Last Error %d\n", GetLastError());
        goto cleanup;
    }

    hService = OpenServiceW(hSCManager,
                            L"PlugPlay",
                            SERVICE_CHANGE_CONFIG | SERVICE_START);
    if (hService == NULL)
    {
        DPRINT1("Unable to open PlugPlay service\n");
        goto cleanup;
    }

    bRet = ChangeServiceConfigW(hService,
                               SERVICE_NO_CHANGE,
                               SERVICE_AUTO_START,
                               SERVICE_NO_CHANGE,
                               NULL, NULL, NULL,
                               NULL, NULL, NULL, NULL);
    if (!bRet)
    {
        DPRINT1("Unable to change the service configuration\n");
        goto cleanup;
    }

    bRet = StartServiceW(hService, 0, NULL);
    if (!bRet && (GetLastError() != ERROR_SERVICE_ALREADY_RUNNING))
    {
        DPRINT1("Unable to start service\n");
        goto cleanup;
    }

    bRet = TRUE;

cleanup:
    if (hService != NULL)
        CloseServiceHandle(hService);
    if (hSCManager != NULL)
        CloseServiceHandle(hSCManager);
    return bRet;
}

static INT_PTR CALLBACK
StatusMessageWindowProc(
    IN HWND hwndDlg,
    IN UINT uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam)
{
    UNREFERENCED_PARAMETER(wParam);

    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            WCHAR szMsg[256];

            if (!LoadStringW(hDllInstance, IDS_STATUS_INSTALL_DEV, szMsg, sizeof(szMsg)/sizeof(szMsg[0])))
                return FALSE;
            SetDlgItemTextW(hwndDlg, IDC_STATUSLABEL, szMsg);
            return TRUE;
        }
    }
    return FALSE;
}

static DWORD WINAPI
ShowStatusMessageThread(
    IN LPVOID lpParameter)
{
    HWND *phWnd = (HWND *)lpParameter;
    HWND hWnd;
    MSG Msg;

    hWnd = CreateDialogParam(
        hDllInstance,
        MAKEINTRESOURCE(IDD_STATUSWINDOW_DLG),
        GetDesktopWindow(),
        StatusMessageWindowProc,
        (LPARAM)NULL);
    if (!hWnd)
        return 0;
    *phWnd = hWnd;

    ShowWindow(hWnd, SW_SHOW);

    /* Message loop for the Status window */
    while (GetMessage(&Msg, NULL, 0, 0))
    {
        TranslateMessage(&Msg);
        DispatchMessage(&Msg);
    }

    return 0;
}

static LONG
ReadRegSzKey(
    IN HKEY hKey,
    IN LPCWSTR pszKey,
    OUT LPWSTR* pValue)
{
    LONG rc;
    DWORD dwType;
    DWORD cbData = 0;
    LPWSTR pwszValue;

    if (!pValue)
        return ERROR_INVALID_PARAMETER;

    *pValue = NULL;
    rc = RegQueryValueExW(hKey, pszKey, NULL, &dwType, NULL, &cbData);
    if (rc != ERROR_SUCCESS)
        return rc;
    if (dwType != REG_SZ)
        return ERROR_FILE_NOT_FOUND;
    pwszValue = HeapAlloc(GetProcessHeap(), 0, cbData + sizeof(WCHAR));
    if (!pwszValue)
        return ERROR_NOT_ENOUGH_MEMORY;
    rc = RegQueryValueExW(hKey, pszKey, NULL, NULL, (LPBYTE)pwszValue, &cbData);
    if (rc != ERROR_SUCCESS)
    {
        HeapFree(GetProcessHeap(), 0, pwszValue);
        return rc;
    }
    /* NULL-terminate the string */
    pwszValue[cbData / sizeof(WCHAR)] = '\0';

    *pValue = pwszValue;
    return ERROR_SUCCESS;
}

static BOOL
IsConsoleBoot(VOID)
{
    HKEY hControlKey = NULL;
    LPWSTR pwszSystemStartOptions = NULL;
    LPWSTR pwszCurrentOption, pwszNextOption; /* Pointers into SystemStartOptions */
    BOOL bConsoleBoot = FALSE;
    LONG rc;

    rc = RegOpenKeyExW(
        HKEY_LOCAL_MACHINE,
        L"SYSTEM\\CurrentControlSet\\Control",
        0,
        KEY_QUERY_VALUE,
        &hControlKey);
    if (rc != ERROR_SUCCESS)
        goto cleanup;

    rc = ReadRegSzKey(hControlKey, L"SystemStartOptions", &pwszSystemStartOptions);
    if (rc != ERROR_SUCCESS)
        goto cleanup;

    /* Check for CONSOLE switch in SystemStartOptions */
    pwszCurrentOption = pwszSystemStartOptions;
    while (pwszCurrentOption)
    {
        pwszNextOption = wcschr(pwszCurrentOption, L' ');
        if (pwszNextOption)
            *pwszNextOption = L'\0';
        if (wcsicmp(pwszCurrentOption, L"CONSOLE") == 0)
        {
            DPRINT("Found %S. Switching to console boot\n", pwszCurrentOption);
            bConsoleBoot = TRUE;
            goto cleanup;
        }
        pwszCurrentOption = pwszNextOption ? pwszNextOption + 1 : NULL;
    }

cleanup:
    if (hControlKey != NULL)
        RegCloseKey(hControlKey);
    if (pwszSystemStartOptions)
        HeapFree(GetProcessHeap(), 0, pwszSystemStartOptions);
    return bConsoleBoot;
}

static BOOL
CommonInstall(VOID)
{
    HWND hWnd = NULL;

    hSysSetupInf = SetupOpenInfFileW(
        L"syssetup.inf",
        NULL,
        INF_STYLE_WIN4,
        NULL);
    if (hSysSetupInf == INVALID_HANDLE_VALUE)
    {
        FatalError("SetupOpenInfFileW() failed to open 'syssetup.inf' (Error: %lu)\n", GetLastError());
        return FALSE;
    }

    if (!InstallSysSetupInfDevices())
    {
        FatalError("InstallSysSetupInfDevices() failed!\n");
        goto error;
    }

    if(!InstallSysSetupInfComponents())
    {
        FatalError("InstallSysSetupInfComponents() failed!\n");
        goto error;
    }

    if (!IsConsoleBoot())
    {
        HANDLE hThread;

        hThread = CreateThread(
            NULL,
            0,
            ShowStatusMessageThread,
            (LPVOID)&hWnd,
            0,
            NULL);

        if (hThread)
            CloseHandle(hThread);
    }

    if (!EnableUserModePnpManager())
    {
        FatalError("EnableUserModePnpManager() failed!\n");
        goto error;
    }

    if (CMP_WaitNoPendingInstallEvents(INFINITE) != WAIT_OBJECT_0)
    {
        FatalError("CMP_WaitNoPendingInstallEvents() failed!\n");
        goto error;
    }

    EndDialog(hWnd, 0);
    return TRUE;

error:
    if (hWnd)
        EndDialog(hWnd, 0);
    SetupCloseInfFile(hSysSetupInf);
    return FALSE;
}

DWORD WINAPI
InstallLiveCD(IN HINSTANCE hInstance)
{
    STARTUPINFOW StartupInfo;
    PROCESS_INFORMATION ProcessInformation;
    BOOL bRes;

    if (!CommonInstall())
        goto error;
    
    /* Register components */
    _SEH2_TRY
    {
        if (!SetupInstallFromInfSectionW(NULL,
            hSysSetupInf, L"RegistrationPhase2",
            SPINST_ALL,
            0, NULL, 0, NULL, NULL, NULL, NULL))
        {
            DPRINT1("SetupInstallFromInfSectionW failed!\n");
        }
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        DPRINT1("Catching exception\n");
    }
    _SEH2_END;
    
    SetupCloseInfFile(hSysSetupInf);

    /* Run the shell */
    ZeroMemory(&StartupInfo, sizeof(StartupInfo));
    StartupInfo.cb = sizeof(StartupInfo);
    bRes = CreateProcessW(
        L"userinit.exe",
        NULL,
        NULL,
        NULL,
        FALSE,
        0,
        NULL,
        NULL,
        &StartupInfo,
        &ProcessInformation);
    if (!bRes)
        goto error;

    CloseHandle(ProcessInformation.hThread);
    CloseHandle(ProcessInformation.hProcess);

    return 0;

error:
    MessageBoxW(
        NULL,
        L"Failed to load LiveCD! You can shutdown your computer, or press ENTER to reboot.",
        L"ReactOS LiveCD",
        MB_OK);
    return 0;
}


static BOOL
CreateShortcuts(VOID)
{
    TCHAR szFolder[256];

    CoInitialize(NULL);

    /* Create desktop shortcuts */
    CreateShortcut(CSIDL_DESKTOP, NULL, IDS_SHORT_CMD, _T("%SystemRoot%\\system32\\cmd.exe"), IDS_CMT_CMD, TRUE, 0);

    /* Create program startmenu shortcuts */
    CreateShortcut(CSIDL_PROGRAMS, NULL, IDS_SHORT_EXPLORER, _T("%SystemRoot%\\explorer.exe"), IDS_CMT_EXPLORER, TRUE, 1);
    CreateShortcut(CSIDL_PROGRAMS, NULL, IDS_SHORT_DOWNLOADER, _T("%SystemRoot%\\system32\\rapps.exe"), IDS_CMT_DOWNLOADER, TRUE, 0);

    /* Create administrative tools startmenu shortcuts */
    CreateShortcut(CSIDL_COMMON_ADMINTOOLS, NULL, IDS_SHORT_SERVICE, _T("%SystemRoot%\\system32\\servman.exe"), IDS_CMT_SERVMAN, TRUE, 0);
    CreateShortcut(CSIDL_COMMON_ADMINTOOLS, NULL, IDS_SHORT_DEVICE, _T("%SystemRoot%\\system32\\devmgmt.exe"), IDS_CMT_DEVMGMT, TRUE, 0);
    CreateShortcut(CSIDL_COMMON_ADMINTOOLS, NULL, IDS_SHORT_EVENTVIEW, _T("%SystemRoot%\\system32\\eventvwr.exe"), IDS_CMT_EVENTVIEW, TRUE, 0);
    CreateShortcut(CSIDL_COMMON_ADMINTOOLS, NULL, IDS_SHORT_MSCONFIG, _T("%SystemRoot%\\system32\\msconfig.exe"), IDS_CMT_MSCONFIG, TRUE, 0);

    /* Create and fill Accessories subfolder */
    if (CreateShortcutFolder(CSIDL_PROGRAMS, IDS_ACCESSORIES, szFolder, sizeof(szFolder)/sizeof(szFolder[0])))
    {
        CreateShortcut(CSIDL_PROGRAMS, szFolder, IDS_SHORT_CALC, _T("%SystemRoot%\\system32\\calc.exe"), IDS_CMT_CALC, TRUE, 0);
        CreateShortcut(CSIDL_PROGRAMS, szFolder, IDS_SHORT_CMD, _T("%SystemRoot%\\system32\\cmd.exe"), IDS_CMT_CMD, TRUE, 0);
        CreateShortcut(CSIDL_PROGRAMS, szFolder, IDS_SHORT_NOTEPAD, _T("%SystemRoot%\\system32\\notepad.exe"), IDS_CMT_NOTEPAD, TRUE, 0);
        CreateShortcut(CSIDL_PROGRAMS, szFolder, IDS_SHORT_RDESKTOP, _T("%SystemRoot%\\system32\\mstsc.exe"), IDS_CMT_RDESKTOP, TRUE, 0);
        CreateShortcut(CSIDL_PROGRAMS, szFolder, IDS_SHORT_SNAP, _T("%SystemRoot%\\system32\\screenshot.exe"), IDS_CMT_SCREENSHOT, TRUE, 0);
        CreateShortcut(CSIDL_PROGRAMS, szFolder, IDS_SHORT_WORDPAD, _T("%SystemRoot%\\system32\\wordpad.exe"), IDS_CMT_WORDPAD, TRUE, 0);
        CreateShortcut(CSIDL_PROGRAMS, szFolder, IDS_SHORT_PAINT, _T("%SystemRoot%\\system32\\mspaint.exe"), IDS_CMT_PAINT, TRUE, 0);
    }

    /* Create System Tools subfolder and fill if the exe is available */
    if (CreateShortcutFolder(CSIDL_PROGRAMS, IDS_SYS_TOOLS, szFolder, sizeof(szFolder)/sizeof(szFolder[0])))
    {
        CreateShortcut(CSIDL_PROGRAMS, szFolder, IDS_SHORT_CHARMAP, _T("%SystemRoot%\\system32\\charmap.exe"), IDS_CMT_CHARMAP, TRUE, 0);
        CreateShortcut(CSIDL_PROGRAMS, szFolder, IDS_SHORT_KBSWITCH, _T("%SystemRoot%\\system32\\kbswitch.exe"), IDS_CMT_KBSWITCH, TRUE, 0);
        CreateShortcut(CSIDL_PROGRAMS, szFolder, IDS_SHORT_REGEDIT, _T("%SystemRoot%\\regedit.exe"), IDS_CMT_REGEDIT, TRUE, 0);
        CreateShortcut(CSIDL_PROGRAMS, szFolder, IDS_SHORT_DXDIAG, _T("%SystemRoot%\\system32\\dxdiag.exe"), IDS_CMT_DXDIAG, TRUE, 0);
    }

    /* Create Accessibility subfolder and fill if the exe is available */
    if (CreateShortcutFolder(CSIDL_PROGRAMS, IDS_SYS_ACCESSIBILITY, szFolder, sizeof(szFolder)/sizeof(szFolder[0])))
    {
        CreateShortcut(CSIDL_PROGRAMS, szFolder, IDS_SHORT_MAGNIFY, _T("%SystemRoot%\\system32\\magnify.exe"), IDS_CMT_MAGNIFY, TRUE, 0);
    }

    /* Create Entertainment subfolder and fill if the exe is available */
    if (CreateShortcutFolder(CSIDL_PROGRAMS, IDS_SYS_ENTERTAINMENT, szFolder, sizeof(szFolder)/sizeof(szFolder[0])))
    {
        CreateShortcut(CSIDL_PROGRAMS, szFolder, IDS_SHORT_MPLAY32, _T("%SystemRoot%\\system32\\mplay32.exe"), IDS_CMT_MPLAY32, TRUE, 0);
        CreateShortcut(CSIDL_PROGRAMS, szFolder, IDS_SHORT_SNDVOL32, _T("%SystemRoot%\\system32\\sndvol32.exe"), IDS_CMT_SNDVOL32, TRUE, 0);
        CreateShortcut(CSIDL_PROGRAMS, szFolder, IDS_SHORT_SNDREC32, _T("%SystemRoot%\\system32\\sndrec32.exe"), IDS_CMT_SNDREC32, TRUE, 0);
    }

    /* Create Games subfolder and fill if the exe is available */
    if (CreateShortcutFolder(CSIDL_PROGRAMS, IDS_GAMES, szFolder, sizeof(szFolder)/sizeof(szFolder[0])))
    {
        CreateShortcut(CSIDL_PROGRAMS, szFolder, IDS_SHORT_SOLITAIRE, _T("%SystemRoot%\\system32\\sol.exe"), IDS_CMT_SOLITAIRE, TRUE, 0);
        CreateShortcut(CSIDL_PROGRAMS, szFolder, IDS_SHORT_WINEMINE, _T("%SystemRoot%\\system32\\winmine.exe"), IDS_CMT_WINEMINE, TRUE, 0);
        CreateShortcut(CSIDL_PROGRAMS, szFolder, IDS_SHORT_SPIDER, _T("%SystemRoot%\\system32\\spider.exe"), IDS_CMT_SPIDER, TRUE, 0);
    }

    CoUninitialize();

    return TRUE;
}

static BOOL
SetSetupType(DWORD dwSetupType)
{
    DWORD dwError;
    HKEY hKey;

    dwError = RegOpenKeyExW(
        HKEY_LOCAL_MACHINE,
        L"SYSTEM\\Setup",
        0,
        KEY_SET_VALUE,
        &hKey);
    if (dwError != ERROR_SUCCESS)
        return FALSE;

    dwError = RegSetValueExW(
        hKey,
        L"SetupType",
        0,
        REG_DWORD,
        (LPBYTE)&dwSetupType,
        sizeof(DWORD));
    RegCloseKey(hKey);
    if (dwError != ERROR_SUCCESS)
        return FALSE;

    return TRUE;
}

DWORD WINAPI
InstallReactOS(HINSTANCE hInstance)
{
    TCHAR szBuffer[MAX_PATH];
    HANDLE token;
    TOKEN_PRIVILEGES privs;
    HKEY hKey;

    InitializeSetupActionLog(FALSE);
    LogItem(SYSSETUP_SEVERITY_INFORMATION, L"Installing ReactOS");

    if (!InitializeProfiles())
    {
        FatalError("InitializeProfiles() failed");
        return 0;
    }

    CreateTempDir(L"TEMP");
    CreateTempDir(L"TMP");

    if (GetWindowsDirectory(szBuffer, sizeof(szBuffer) / sizeof(TCHAR)))
    {
        if (RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                          L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion",
                          0,
                          KEY_WRITE,
                          &hKey) == ERROR_SUCCESS)
        {
            RegSetValueExW(hKey,
                           L"PathName",
                           0,
                           REG_SZ,
                           (LPBYTE)szBuffer,
                           (wcslen(szBuffer) + 1) * sizeof(WCHAR));

            RegSetValueExW(hKey,
                           L"SystemRoot",
                           0,
                           REG_SZ,
                           (LPBYTE)szBuffer,
                           (wcslen(szBuffer) + 1) * sizeof(WCHAR));

            RegCloseKey(hKey);
        }

        PathAddBackslash(szBuffer);
        _tcscat(szBuffer, _T("system"));
        CreateDirectory(szBuffer, NULL);
    }

    if (!CommonInstall())
        return 0;

    InstallWizard();

    InstallSecurity();

    if (!CreateShortcuts())
    {
        FatalError("CreateShortcuts() failed");
        return 0;
    }

    /* ROS HACK, as long as NtUnloadKey is not implemented */
    {
        NTSTATUS Status = NtUnloadKey(NULL);
        if (Status == STATUS_NOT_IMPLEMENTED)
        {
            /* Create the Administrator profile */
            PROFILEINFOW ProfileInfo;
            HANDLE hToken;
            BOOL ret;

            ret = LogonUserW(L"Administrator", L"", L"", LOGON32_LOGON_NETWORK, LOGON32_PROVIDER_DEFAULT, &hToken);
            if (!ret)
            {
                FatalError("LogonUserW() failed!");
                return 0;
            }
            ZeroMemory(&ProfileInfo, sizeof(PROFILEINFOW));
            ProfileInfo.dwSize = sizeof(PROFILEINFOW);
            ProfileInfo.lpUserName = L"Administrator";
            ProfileInfo.dwFlags = PI_NOUI;
            LoadUserProfileW(hToken, &ProfileInfo);
            CloseHandle(hToken);
        }
        else
        {
            DPRINT1("ROS HACK not needed anymore. Please remove it\n");
        }
    }
    /* END OF ROS HACK */

    SetupCloseInfFile(hSysSetupInf);
    SetSetupType(0);

    LogItem(SYSSETUP_SEVERITY_INFORMATION, L"Installing ReactOS done");
    TerminateSetupActionLog();

    /* Get shutdown privilege */
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &token))
    {
        FatalError("OpenProcessToken() failed!");
        return 0;
    }
    if (!LookupPrivilegeValue(
        NULL,
        SE_SHUTDOWN_NAME,
        &privs.Privileges[0].Luid))
    {
        FatalError("LookupPrivilegeValue() failed!");
        return 0;
    }
    privs.PrivilegeCount = 1;
    privs.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    if (AdjustTokenPrivileges(
        token,
        FALSE,
        &privs,
        0,
        (PTOKEN_PRIVILEGES)NULL,
        NULL) == 0)
    {
        FatalError("AdjustTokenPrivileges() failed!");
        return 0;
    }

    ExitWindowsEx(EWX_REBOOT, 0);
    return 0;
}


/*
 * @unimplemented
 */
DWORD WINAPI
SetupChangeFontSize(
    IN HANDLE hWnd,
    IN LPCWSTR lpszFontSize)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/*
 * @unimplemented
 */
DWORD WINAPI
SetupChangeLocaleEx(HWND hWnd,
                    LCID Lcid,
                    LPCWSTR lpSrcRootPath,
                    char Unknown,
                    DWORD dwUnused1,
                    DWORD dwUnused2)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/*
 * @implemented
 */
DWORD WINAPI
SetupChangeLocale(HWND hWnd, LCID Lcid)
{
    return SetupChangeLocaleEx(hWnd, Lcid, NULL, 0, 0, 0);
}
