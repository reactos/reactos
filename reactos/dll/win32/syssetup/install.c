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
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS system libraries
 * PURPOSE:           System setup
 * FILE:              lib/syssetup/install.c
 * PROGRAMER:         Eric Kohl
 */

/* INCLUDES *****************************************************************/

#define WIN32_NO_STATUS
#include <windows.h>
#define NTOS_MODE_USER
#include <ndk/ntndk.h>

#include <commctrl.h>
#include <stdio.h>
#include <io.h>
#include <tchar.h>
#include <stdlib.h>

#include <samlib/samlib.h>
#include <syssetup/syssetup.h>
#include <userenv.h>
#include <setupapi.h>

#include <shlobj.h>
#include <objidl.h>
#include <shlwapi.h>

#include "globals.h"
#include "resource.h"

#include <debug.h>

DWORD WINAPI
CMP_WaitNoPendingInstallEvents(DWORD dwTimeout);

/* GLOBALS ******************************************************************/

PSID DomainSid = NULL;
PSID AdminSid = NULL;

HINF hSysSetupInf = INVALID_HANDLE_VALUE;

/* FUNCTIONS ****************************************************************/

static VOID
DebugPrint(char* fmt,...)
{
    char buffer[512];
    va_list ap;

    va_start(ap, fmt);
    vsprintf(buffer, fmt, ap);
    va_end(ap);

    LogItem(SYSSETUP_SEVERITY_FATAL_ERROR, L"Failed");

    strcat(buffer, "\nRebooting now!");
    MessageBoxA(NULL,
                buffer,
                "ReactOS Setup",
                MB_OK);
}


HRESULT CreateShellLink(LPCTSTR linkPath, LPCTSTR cmd, LPCTSTR arg, LPCTSTR dir, LPCTSTR iconPath, int icon_nr, LPCTSTR comment)
{
    IShellLink* psl;
    IPersistFile* ppf;
#ifndef _UNICODE
    WCHAR buffer[MAX_PATH];
#endif /* _UNICODE */

    HRESULT hr = CoCreateInstance(&CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, &IID_IShellLink, (LPVOID*)&psl);

    if (SUCCEEDED(hr))
    {
        hr = psl->lpVtbl->SetPath(psl, cmd);

        if (arg)
        {
            hr = psl->lpVtbl->SetArguments(psl, arg);
        }

        if (dir)
        {
            hr = psl->lpVtbl->SetWorkingDirectory(psl, dir);
        }

        if (iconPath)
        {
            hr = psl->lpVtbl->SetIconLocation(psl, iconPath, icon_nr);
        }

        if (comment)
        {
            hr = psl->lpVtbl->SetDescription(psl, comment);
        }

        hr = psl->lpVtbl->QueryInterface(psl, &IID_IPersistFile, (LPVOID*)&ppf);

        if (SUCCEEDED(hr))
        {
#ifdef _UNICODE
            hr = ppf->lpVtbl->Save(ppf, linkPath, TRUE);
#else /* _UNICODE */
            MultiByteToWideChar(CP_ACP, 0, linkPath, -1, buffer, MAX_PATH);

            hr = ppf->lpVtbl->Save(ppf, buffer, TRUE);
#endif /* _UNICODE */

            ppf->lpVtbl->Release(ppf);
        }

        psl->lpVtbl->Release(psl);
    }

    return hr;
}


static BOOL
CreateShortcut(int csidl, LPCTSTR folder, UINT nIdName, LPCTSTR command, UINT nIdTitle, BOOL bCheckExistence)
{
    TCHAR path[MAX_PATH];
    TCHAR title[256];
    TCHAR name[256];
    LPTSTR p = path;
    TCHAR szSystemPath[MAX_PATH];
    TCHAR szProgramPath[MAX_PATH];

    if (bCheckExistence)
    {
        if (!GetSystemDirectory(szSystemPath, sizeof(szSystemPath)/sizeof(szSystemPath[0])))
            return FALSE;
        _tcscpy(szProgramPath, szSystemPath);
        _tcscat(szProgramPath, _T("\\"));
        if ((_taccess(_tcscat(szProgramPath, command), 0 )) == -1)
            /* Expected error, don't return FALSE */
            return TRUE;
    }

    if (!SHGetSpecialFolderPath(0, path, csidl, TRUE))
        return FALSE;

    if (folder)
    {
        p = PathAddBackslash(p);
        _tcscpy(p, folder);
    }

    p = PathAddBackslash(p);

    if (!LoadString(hDllInstance, nIdName, name, sizeof(name)/sizeof(name[0])))
        return FALSE;
    _tcscpy(p, name);

    if (!LoadString(hDllInstance, nIdTitle, title, sizeof(title)/sizeof(title[0])))
        return FALSE;

    return SUCCEEDED(CreateShellLink(path, command, _T(""), NULL, NULL, 0, title));
}


static BOOL
CreateShortcutFolder(int csidl, UINT nID, LPTSTR name, int nameLen)
{
    TCHAR path[MAX_PATH];
    LPTSTR p;

    if (!SHGetSpecialFolderPath(0, path, csidl, TRUE))
        return FALSE;

    if (!LoadString(hDllInstance, nID, name, nameLen))
        return FALSE;

    p = PathAddBackslash(path);
    _tcscpy(p, name);

    return CreateDirectory(path, NULL) || GetLastError()==ERROR_ALREADY_EXISTS;
}


static BOOL
CreateRandomSid(
    OUT PSID *Sid)
{
    SID_IDENTIFIER_AUTHORITY SystemAuthority = {SECURITY_NT_AUTHORITY};
    LARGE_INTEGER SystemTime;
    PULONG Seed;
    NTSTATUS Status;

    NtQuerySystemTime(&SystemTime);
    Seed = &SystemTime.u.LowPart;

    Status = RtlAllocateAndInitializeSid(
        &SystemAuthority,
        4,
        SECURITY_NT_NON_UNIQUE,
        RtlUniform(Seed),
        RtlUniform(Seed),
        RtlUniform(Seed),
        SECURITY_NULL_RID,
        SECURITY_NULL_RID,
        SECURITY_NULL_RID,
        SECURITY_NULL_RID,
        Sid);
    return NT_SUCCESS(Status);
}


static VOID
AppendRidToSid(
    OUT PSID *Dst,
    IN PSID Src,
    IN ULONG NewRid)
{
    ULONG Rid[8] = {0, 0, 0, 0, 0, 0, 0, 0};
    UCHAR RidCount;
    ULONG i;

    RidCount = *RtlSubAuthorityCountSid (Src);

    for (i = 0; i < RidCount; i++)
        Rid[i] = *RtlSubAuthoritySid (Src, i);

    if (RidCount < 8)
    {
        Rid[RidCount] = NewRid;
        RidCount++;
    }

    RtlAllocateAndInitializeSid(
        RtlIdentifierAuthoritySid(Src),
        RidCount,
        Rid[0],
        Rid[1],
        Rid[2],
        Rid[3],
        Rid[4],
        Rid[5],
        Rid[6],
        Rid[7],
        Dst);
}


static VOID
CreateTempDir(
    IN LPCWSTR VarName)
{
    TCHAR szTempDir[MAX_PATH];
    TCHAR szBuffer[MAX_PATH];
    DWORD dwLength;
    HKEY hKey;

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                     _T("SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment"),
                     0,
                     KEY_QUERY_VALUE,
                     &hKey))
    {
        DebugPrint("Error: %lu\n", GetLastError());
        return;
    }

    /* Get temp dir */
    dwLength = MAX_PATH * sizeof(TCHAR);
    if (RegQueryValueEx(hKey,
                        VarName,
                        NULL,
                        NULL,
                        (LPBYTE)szBuffer,
                        &dwLength))
    {
        DebugPrint("Error: %lu\n", GetLastError());
        RegCloseKey(hKey);
        return;
    }

    /* Expand it */
    if (!ExpandEnvironmentStrings(szBuffer,
                                  szTempDir,
                                  MAX_PATH))
    {
        DebugPrint("Error: %lu\n", GetLastError());
        RegCloseKey(hKey);
        return;
    }

    /* Create profiles directory */
    if (!CreateDirectory(szTempDir, NULL))
    {
        if (GetLastError() != ERROR_ALREADY_EXISTS)
        {
            DebugPrint("Error: %lu\n", GetLastError());
            RegCloseKey(hKey);
            return;
        }
    }

    RegCloseKey(hKey);
}


BOOL
ProcessSysSetupInf(VOID)
{
    INFCONTEXT InfContext;
    TCHAR LineBuffer[256];
    DWORD LineLength;

    if (!SetupFindFirstLine(hSysSetupInf,
                            _T("DeviceInfsToInstall"),
                            NULL,
                            &InfContext))
    {
        return FALSE;
    }

    do
    {
        if (!SetupGetStringField(&InfContext,
                                 0,
                                 LineBuffer,
                                 sizeof(LineBuffer)/sizeof(LineBuffer[0]),
                                 &LineLength))
        {
            return FALSE;
        }

        if (!SetupDiInstallClass(NULL, LineBuffer, DI_QUIETINSTALL, NULL))
        {
            return FALSE;
        }
    }
    while (SetupFindNextLine(&InfContext, &InfContext));

    return TRUE;
}


static BOOL
EnableUserModePnpManager(VOID)
{
    SC_HANDLE hSCManager = NULL;
    SC_HANDLE hService = NULL;
    BOOL ret = FALSE;

    hSCManager = OpenSCManager(NULL, NULL, 0);
    if (hSCManager == NULL)
        goto cleanup;

    hService = OpenService(hSCManager, _T("PlugPlay"), SERVICE_CHANGE_CONFIG | SERVICE_START);
    if (hService == NULL)
        goto cleanup;

    ret = ChangeServiceConfig(
        hService,
        SERVICE_NO_CHANGE, SERVICE_AUTO_START, SERVICE_NO_CHANGE,
        NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    if (!ret)
        goto cleanup;

    ret = StartService(hService, 0, NULL);
    if (!ret)
        goto cleanup;

    ret = TRUE;

cleanup:
    if (hSCManager != NULL)
        CloseServiceHandle(hSCManager);
    if (hService != NULL)
        CloseServiceHandle(hService);
    return ret;
}


static BOOL CALLBACK
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
            TCHAR szMsg[256];

            if (!LoadString(hDllInstance, IDS_STATUS_INSTALL_DEV, szMsg, sizeof(szMsg)/sizeof(szMsg[0])))
                return FALSE;
            SetDlgItemText(hwndDlg, IDC_STATUSLABEL, szMsg);
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
        DebugPrint("SetupOpenInfFileW() failed to open 'syssetup.inf' (Error: %lu)\n", GetLastError());
        return FALSE;
    }

    if (!ProcessSysSetupInf())
    {
        DebugPrint("ProcessSysSetupInf() failed!\n");
        SetupCloseInfFile(hSysSetupInf);
        return FALSE;
    }

    CreateThread(
        NULL,
        0,
        ShowStatusMessageThread,
        (LPVOID)&hWnd,
        0,
        NULL);

    if (!EnableUserModePnpManager())
    {
       DebugPrint("EnableUserModePnpManager() failed!\n");
       SetupCloseInfFile(hSysSetupInf);
       EndDialog(hWnd, 0);
       return FALSE;
    }

    if (CMP_WaitNoPendingInstallEvents(INFINITE) != WAIT_OBJECT_0)
    {
      DebugPrint("CMP_WaitNoPendingInstallEvents() failed!\n");
      SetupCloseInfFile(hSysSetupInf);
      EndDialog(hWnd, 0);
      return FALSE;
    }

    EndDialog(hWnd, 0);
    return TRUE;
}

DWORD WINAPI
InstallLiveCD(IN HINSTANCE hInstance)
{
    STARTUPINFO StartupInfo;
    PROCESS_INFORMATION ProcessInformation;
    BOOL res;

    if (!CommonInstall())
        goto cleanup;
    SetupCloseInfFile(hSysSetupInf);

    /* Run the shell */
    StartupInfo.cb = sizeof(StartupInfo);
    StartupInfo.lpReserved = NULL;
    StartupInfo.lpDesktop = NULL;
    StartupInfo.lpTitle = NULL;
    StartupInfo.dwFlags = 0;
    StartupInfo.cbReserved2 = 0;
    StartupInfo.lpReserved2 = 0;
    res = CreateProcess(
        _T("userinit.exe"),
        NULL,
        NULL,
        NULL,
        FALSE,
        0,
        NULL,
        NULL,
        &StartupInfo,
        &ProcessInformation);
    if (!res)
        goto cleanup;

    /* Wait for process termination */
    WaitForSingleObject(ProcessInformation.hProcess, INFINITE);

cleanup:
    MessageBoxA(
        NULL,
        "You can shutdown your computer, or press ENTER to reboot",
        "ReactOS LiveCD",
        MB_OK);
    return 0;
}


static BOOL
CreateShortcuts(VOID)
{
    TCHAR szFolder[256];

    CoInitialize(NULL);

    /* Create desktop shortcuts */
    CreateShortcut(CSIDL_DESKTOP, NULL, IDS_SHORT_CMD, _T("cmd.exe"), IDS_CMT_CMD, FALSE);

    /* Create program startmenu shortcuts */
    CreateShortcut(CSIDL_PROGRAMS, NULL, IDS_SHORT_EXPLORER, _T("explorer.exe"), IDS_CMT_EXPLORER, FALSE);
    CreateShortcut(CSIDL_PROGRAMS, NULL, IDS_SHORT_DOWNLOADER, _T("downloader.exe"), IDS_CMT_DOWNLOADER, TRUE);
    CreateShortcut(CSIDL_PROGRAMS, NULL, IDS_SHORT_FIREFOX, _T("getfirefox.exe"), IDS_CMT_GETFIREFOX, TRUE);

    /* Create administrative tools startmenu shortcuts */
    CreateShortcut(CSIDL_COMMON_ADMINTOOLS, NULL, IDS_SHORT_SERVICE, _T("servman.exe"), IDS_CMT_SERVMAN, FALSE);
    CreateShortcut(CSIDL_COMMON_ADMINTOOLS, NULL, IDS_SHORT_DEVICE, _T("devmgmt.exe"), IDS_CMT_DEVMGMT, FALSE);

    /* Create and fill Accessories subfolder */
    if (CreateShortcutFolder(CSIDL_PROGRAMS, IDS_ACCESSORIES, szFolder, sizeof(szFolder)/sizeof(szFolder[0])))
    {
        CreateShortcut(CSIDL_PROGRAMS, szFolder, IDS_SHORT_CALC, _T("calc.exe"), IDS_CMT_CALC, FALSE);
        CreateShortcut(CSIDL_PROGRAMS, szFolder, IDS_SHORT_CMD, _T("cmd.exe"), IDS_CMT_CMD, FALSE);
        CreateShortcut(CSIDL_PROGRAMS, szFolder, IDS_SHORT_NOTEPAD, _T("notepad.exe"), IDS_CMT_NOTEPAD, FALSE);
        CreateShortcut(CSIDL_PROGRAMS, szFolder, IDS_SHORT_REGEDIT, _T("regedit.exe"), IDS_CMT_REGEDIT, FALSE);
        CreateShortcut(CSIDL_PROGRAMS, szFolder, IDS_SHORT_WORDPAD, _T("wordpad.exe"), IDS_CMT_WORDPAD, FALSE);
        CreateShortcut(CSIDL_PROGRAMS, szFolder, IDS_SHORT_SNAP, _T("screenshot.exe"), IDS_CMT_SCREENSHOT, TRUE);
    }

    /* Create Games subfolder and fill if the exe is available */
    if (CreateShortcutFolder(CSIDL_PROGRAMS, IDS_GAMES, szFolder, sizeof(szFolder)/sizeof(szFolder[0])))
    {
        CreateShortcut(CSIDL_PROGRAMS, szFolder, IDS_SHORT_SOLITAIRE, _T("sol.exe"), IDS_CMT_SOLITAIRE, FALSE);
        CreateShortcut(CSIDL_PROGRAMS, szFolder, IDS_SHORT_WINEMINE, _T("winemine.exe"), IDS_CMT_WINEMINE, FALSE);
    }

    CoUninitialize();

    return TRUE;
}

DWORD WINAPI
InstallReactOS(HINSTANCE hInstance)
{
    TCHAR szBuffer[MAX_PATH];
    DWORD LastError;

    InitializeSetupActionLog(FALSE);
    LogItem(SYSSETUP_SEVERITY_INFORMATION, L"Installing ReactOS");

    /* Set user langage to the system language */
    SetUserDefaultLCID(GetSystemDefaultLCID());
    SetThreadLocale(GetSystemDefaultLCID());

    if (!InitializeProfiles())
    {
        DebugPrint("InitializeProfiles() failed");
        return 0;
    }

    if (!CreateShortcuts())
    {
        DebugPrint("InitializeProfiles() failed");
        return 0;
    }

    /* Initialize the Security Account Manager (SAM) */
    if (!SamInitializeSAM())
    {
        DebugPrint("SamInitializeSAM() failed!");
        return 0;
    }

    /* Create the semi-random Domain-SID */
    if (!CreateRandomSid(&DomainSid))
    {
        DebugPrint("Domain-SID creation failed!");
        return 0;
    }

    /* Set the Domain SID (aka Computer SID) */
    if (!SamSetDomainSid(DomainSid))
    {
        DebugPrint("SamSetDomainSid() failed!");
        RtlFreeSid(DomainSid);
        return 0;
    }

    /* Append the Admin-RID */
    AppendRidToSid(&AdminSid, DomainSid, DOMAIN_USER_RID_ADMIN);

    /* Create the Administrator account */
    if (!SamCreateUser(L"Administrator", L"", AdminSid))
    {
        /* Check what the error was.
         * If the Admin Account already exists, then it means Setup
         * wasn't allowed to finish properly. Instead of rebooting
         * and not completing it, let it restart instead
         */
        LastError = GetLastError();
        if (LastError != ERROR_USER_EXISTS)
        {
            DebugPrint("SamCreateUser() failed!");
            RtlFreeSid(AdminSid);
            RtlFreeSid(DomainSid);
            return 0;
        }
    }

    /* Create the Administrator profile */
    if (!CreateUserProfileW(AdminSid, L"Administrator"))
    {
        DebugPrint("CreateUserProfileW() failed!");
        RtlFreeSid(AdminSid);
        RtlFreeSid(DomainSid);
        return 0;
    }

    RtlFreeSid(AdminSid);
    RtlFreeSid(DomainSid);

    CreateTempDir(L"TEMP");
    CreateTempDir(L"TMP");

    if (GetWindowsDirectory(szBuffer, sizeof(szBuffer) / sizeof(TCHAR)))
    {
        PathAddBackslash(szBuffer);
        _tcscat(szBuffer, _T("system"));
        CreateDirectory(szBuffer, NULL);
    }

    if (!CommonInstall())
        return 0;

    InstallWizard();

    SetupCloseInfFile(hSysSetupInf);

    LogItem(SYSSETUP_SEVERITY_INFORMATION, L"Installing ReactOS done");
    TerminateSetupActionLog();

    /// THE FOLLOWING DPRINT IS FOR THE SYSTEM REGRESSION TOOL
    /// DO NOT REMOVE!!!
    DbgPrint("SYSREG_CHECKPOINT:SYSSETUP_COMPLETE\n");

    return 0;
}


/*
 * @unimplemented
 */
DWORD STDCALL
SetupChangeFontSize(
    IN HANDLE hWnd,
    IN LPCWSTR lpszFontSize)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}
