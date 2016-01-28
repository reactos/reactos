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

#include <tchar.h>
#include <wincon.h>
#include <winnls.h>
#include <winsvc.h>
#include <userenv.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <rpcproxy.h>
#include <ndk/cmfuncs.h>

#define NDEBUG
#include <debug.h>

DWORD WINAPI
CMP_WaitNoPendingInstallEvents(DWORD dwTimeout);

DWORD WINAPI
SetupStartService(LPCWSTR lpServiceName, BOOL bWait);

/* GLOBALS ******************************************************************/

HINF hSysSetupInf = INVALID_HANDLE_VALUE;
ADMIN_INFO AdminInfo;

/* FUNCTIONS ****************************************************************/

static VOID
FatalError(char *pszFmt,...)
{
    char szBuffer[512];
    va_list ap;

    va_start(ap, pszFmt);
    vsprintf(szBuffer, pszFmt, ap);
    va_end(ap);

    LogItem(NULL, L"Failed");

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
    LPCTSTR pszFolder,
    LPCTSTR pszName,
    LPCTSTR pszCommand,
    LPCTSTR pszDescription,
    INT iIconNr)
{
    TCHAR szPath[MAX_PATH];
    TCHAR szExeName[MAX_PATH];
    LPTSTR Ptr;
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

    if ((_taccess(szPath, 0 )) == -1)
        /* Expected error, don't return FALSE */
        return TRUE;

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

    _tcscpy(szPath, pszFolder);

    Ptr = PathAddBackslash(szPath);

    _tcscpy(Ptr, pszName);

    // FIXME: we should pass 'command' straight in here, but shell32 doesn't expand it
    return SUCCEEDED(CreateShellLink(szPath, szExeName, _T(""), pszWorkingDir, szExeName, iIconNr, pszDescription));
}


static BOOL CreateShortcutsFromSection(HINF hinf, LPWSTR  pszSection, LPCWSTR pszFolder)
{
    INFCONTEXT Context;
    WCHAR szCommand[MAX_PATH];
    WCHAR szName[MAX_PATH];
    WCHAR szDescription[MAX_PATH];
    INT iIconNr;

    if (!SetupFindFirstLine(hinf, pszSection, NULL, &Context))
        return FALSE;

    do
    {
        if (SetupGetFieldCount(&Context) < 4)
            continue;

        if (!SetupGetStringFieldW(&Context, 1, szCommand, MAX_PATH, NULL))
            continue;

        if (!SetupGetStringFieldW(&Context, 2, szName, MAX_PATH, NULL))
            continue;

        if (!SetupGetStringFieldW(&Context, 3, szDescription, MAX_PATH, NULL))
            continue;

        if (!SetupGetIntField(&Context, 4, &iIconNr))
            continue;

        _tcscat(szName, L".lnk");

        CreateShortcut(pszFolder, szName, szCommand, szDescription, iIconNr);

    }while (SetupFindNextLine(&Context, &Context));

    return TRUE;
}

static BOOL CreateShortcuts(HINF hinf, LPCWSTR szSection)
{
    INFCONTEXT Context;
    WCHAR szPath[MAX_PATH];
    WCHAR szFolder[MAX_PATH];
    WCHAR szFolderSection[MAX_PATH];
    INT csidl;

    CoInitialize(NULL);

    if (!SetupFindFirstLine(hinf, szSection, NULL, &Context))
        return FALSE;

    do
    {
        if (SetupGetFieldCount(&Context) < 2)
            continue;

        if (!SetupGetStringFieldW(&Context, 0, szFolderSection, MAX_PATH, NULL))
            continue;

        if (!SetupGetIntField(&Context, 1, &csidl))
            continue;

        if (!SetupGetStringFieldW(&Context, 2, szFolder, MAX_PATH, NULL))
            continue;

        if (FAILED(SHGetFolderPathAndSubDirW(NULL, csidl|CSIDL_FLAG_CREATE, (HANDLE)-1, SHGFP_TYPE_DEFAULT, szFolder, szPath)))
            continue;

        CreateShortcutsFromSection(hinf, szFolderSection, szPath);

    }while (SetupFindNextLine(&Context, &Context));

    CoUninitialize();

    return TRUE;
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




BOOL
RegisterTypeLibraries (HINF hinf, LPCWSTR szSection)
{
    INFCONTEXT InfContext;
    BOOL res;
    WCHAR szName[MAX_PATH];
    WCHAR szPath[MAX_PATH];
    INT csidl;
    LPWSTR p;
    HMODULE hmod;
    HRESULT hret;

    /* Begin iterating the entries in the inf section */
    res = SetupFindFirstLine(hinf, szSection, NULL, &InfContext);
    if (!res) return FALSE;

    do
    {
        /* Get the name of the current type library */
        if (!SetupGetStringFieldW(&InfContext, 1, szName, MAX_PATH, NULL))
        {
            FatalError("SetupGetStringFieldW failed\n");
            continue;
        }

        if (!SetupGetIntField(&InfContext, 2, &csidl))
            csidl = CSIDL_SYSTEM;

        hret = SHGetFolderPathW(NULL, csidl, NULL, 0, szPath);
        if (FAILED(hret))
        {
            FatalError("SHGetFolderPathW failed hret=0x%lx\n", hret);
            continue;
        }

        p = PathAddBackslash(szPath);
        _tcscpy(p, szName);

        hmod = LoadLibraryW(szName);
        if (hmod == NULL)
        {
            FatalError("LoadLibraryW failed\n");
            continue;
        }

        __wine_register_resources(hmod);

    }while (SetupFindNextLine(&InfContext, &InfContext));

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

/* Install a section of a .inf file
 * Returns TRUE if success, FALSE if failure. Error code can
 * be retrieved with GetLastError()
 */
static
BOOL
InstallInfSection(
    IN HWND hWnd,
    IN LPCWSTR InfFile,
    IN LPCWSTR InfSection OPTIONAL,
    IN LPCWSTR InfService OPTIONAL)
{
    WCHAR Buffer[MAX_PATH];
    HINF hInf = INVALID_HANDLE_VALUE;
    UINT BufferSize;
    PVOID Context = NULL;
    BOOL ret = FALSE;

    /* Get Windows directory */
    BufferSize = MAX_PATH - 5 - wcslen(InfFile);
    if (GetWindowsDirectoryW(Buffer, BufferSize) > BufferSize)
    {
        /* Function failed */
        SetLastError(ERROR_GEN_FAILURE);
        goto cleanup;
    }
    /* We have enough space to add some information in the buffer */
    if (Buffer[wcslen(Buffer) - 1] != '\\')
        wcscat(Buffer, L"\\");
    wcscat(Buffer, L"Inf\\");
    wcscat(Buffer, InfFile);

    /* Install specified section */
    hInf = SetupOpenInfFileW(Buffer, NULL, INF_STYLE_WIN4, NULL);
    if (hInf == INVALID_HANDLE_VALUE)
        goto cleanup;

    Context = SetupInitDefaultQueueCallback(hWnd);
    if (Context == NULL)
        goto cleanup;

    ret = TRUE;
    if (ret && InfSection)
    {
        ret = SetupInstallFromInfSectionW(
            hWnd, hInf,
            InfSection, SPINST_ALL,
            NULL, NULL, SP_COPY_NEWER,
            SetupDefaultQueueCallbackW, Context,
            NULL, NULL);
    }
    if (ret && InfService)
    {
        ret = SetupInstallServicesFromInfSectionW(
            hInf, InfService, 0);
    }

cleanup:
    if (Context)
        SetupTermDefaultQueueCallback(Context);
    if (hInf != INVALID_HANDLE_VALUE)
        SetupCloseInfFile(hInf);
    return ret;
}

DWORD WINAPI
InstallLiveCD(IN HINSTANCE hInstance)
{
    STARTUPINFOW StartupInfo;
    PROCESS_INFORMATION ProcessInformation;
    BOOL bRes;

    /* Hack: Install TCP/IP protocol driver */
    bRes = InstallInfSection(NULL,
                            L"nettcpip.inf",
                            L"MS_TCPIP.PrimaryInstall",
                            L"MS_TCPIP.PrimaryInstall.Services");
    if (!bRes && GetLastError() != ERROR_FILE_NOT_FOUND)
    {
        DPRINT("InstallInfSection() failed with error 0x%lx\n", GetLastError());
    }
    else
    {
        /* Start the TCP/IP protocol driver */
        SetupStartService(L"Tcpip", FALSE);
    }

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

        RegisterTypeLibraries(hSysSetupInf, L"TypeLibraries");
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

static DWORD CALLBACK
HotkeyThread(LPVOID Parameter)
{
    ATOM hotkey;
    MSG msg;

    DPRINT("HotkeyThread start\n");

    hotkey = GlobalAddAtomW(L"Setup Shift+F10 Hotkey");

    if (!RegisterHotKey(NULL, hotkey, MOD_SHIFT, VK_F10))
        DPRINT1("RegisterHotKey failed with %lu\n", GetLastError());

    while (GetMessage(&msg, NULL, 0, 0))
    {
        if (msg.hwnd == NULL && msg.message == WM_HOTKEY && msg.wParam == hotkey)
        {
            STARTUPINFOW si = { sizeof(si) };
            PROCESS_INFORMATION pi;

            if (CreateProcessW(L"cmd.exe",
                               NULL,
                               NULL,
                               NULL,
                               FALSE,
                               CREATE_NEW_CONSOLE,
                               NULL,
                               NULL,
                               &si,
                               &pi))
            {
                CloseHandle(pi.hProcess);
                CloseHandle(pi.hThread);
            }
            else
            {
                DPRINT1("Failed to launch command prompt: %lu\n", GetLastError());
            }
        }
    }

    UnregisterHotKey(NULL, hotkey);
    GlobalDeleteAtom(hotkey);

    DPRINT("HotkeyThread terminate\n");
    return 0;
}


static
VOID
InitializeDefaultUserLocale(VOID)
{
    WCHAR szBuffer[80];
    PWSTR ptr;
    HKEY hLocaleKey;
    DWORD ret;
    DWORD dwSize;
    LCID lcid;
    INT i;

    struct {LCTYPE LCType; PWSTR pValue;} LocaleData[] = {
        /* Number */
        {LOCALE_SDECIMAL, L"sDecimal"},
        {LOCALE_STHOUSAND, L"sThousand"},
        {LOCALE_SNEGATIVESIGN, L"sNegativeSign"},
        {LOCALE_SPOSITIVESIGN, L"sPositiveSign"},
        {LOCALE_SGROUPING, L"sGrouping"},
        {LOCALE_SLIST, L"sList"},
        {LOCALE_SNATIVEDIGITS, L"sNativeDigits"},
        {LOCALE_INEGNUMBER, L"iNegNumber"},
        {LOCALE_IDIGITS, L"iDigits"},
        {LOCALE_ILZERO, L"iLZero"},
        {LOCALE_IMEASURE, L"iMeasure"},
        {LOCALE_IDIGITSUBSTITUTION, L"NumShape"},

        /* Currency */
        {LOCALE_SCURRENCY, L"sCurrency"},
        {LOCALE_SMONDECIMALSEP, L"sMonDecimalSep"},
        {LOCALE_SMONTHOUSANDSEP, L"sMonThousandSep"},
        {LOCALE_SMONGROUPING, L"sMonGrouping"},
        {LOCALE_ICURRENCY, L"iCurrency"},
        {LOCALE_INEGCURR, L"iNegCurr"},
        {LOCALE_ICURRDIGITS, L"iCurrDigits"},

        /* Time */
        {LOCALE_STIMEFORMAT, L"sTimeFormat"},
        {LOCALE_STIME, L"sTime"},
        {LOCALE_S1159, L"s1159"},
        {LOCALE_S2359, L"s2359"},
        {LOCALE_ITIME, L"iTime"},
        {LOCALE_ITIMEMARKPOSN, L"iTimePrefix"},
        {LOCALE_ITLZERO, L"iTLZero"},

        /* Date */
        {LOCALE_SLONGDATE, L"sLongDate"},
        {LOCALE_SSHORTDATE, L"sShortDate"},
        {LOCALE_SDATE, L"sDate"},
        {LOCALE_IFIRSTDAYOFWEEK, L"iFirstDayOfWeek"},
        {LOCALE_IFIRSTWEEKOFYEAR, L"iFirstWeekOfYear"},
        {LOCALE_IDATE, L"iDate"},
        {LOCALE_ICALENDARTYPE, L"iCalendarType"},

        /* Misc */
        {LOCALE_SCOUNTRY, L"sCountry"},
        {LOCALE_SLANGUAGE, L"sLanguage"},
        {LOCALE_ICOUNTRY, L"iCountry"},
        {0, NULL}};

    ret = RegOpenKeyExW(HKEY_USERS,
                        L".DEFAULT\\Control Panel\\International",
                        0,
                        KEY_READ | KEY_WRITE,
                        &hLocaleKey);
    if (ret != ERROR_SUCCESS)
    {
        return;
    }

    dwSize = 9 * sizeof(WCHAR);
    ret = RegQueryValueExW(hLocaleKey,
                           L"Locale",
                           NULL,
                           NULL,
                           (PBYTE)szBuffer,
                           &dwSize);
    if (ret != ERROR_SUCCESS)
        goto done;

    lcid = (LCID)wcstoul(szBuffer, &ptr, 16);
    if (lcid == 0)
        goto done;

    i = 0;
    while (LocaleData[i].pValue != NULL)
    {
        if (GetLocaleInfo(lcid,
                          LocaleData[i].LCType | LOCALE_NOUSEROVERRIDE,
                          szBuffer,
                          sizeof(szBuffer) / sizeof(WCHAR)))
        {
            RegSetValueExW(hLocaleKey,
                           LocaleData[i].pValue,
                           0,
                           REG_SZ,
                           (PBYTE)szBuffer,
                           (wcslen(szBuffer) + 1) * sizeof(WCHAR));
        }

        i++;
    }

done:
    RegCloseKey(hLocaleKey);
}


DWORD
WINAPI
InstallReactOS(HINSTANCE hInstance)
{
    TCHAR szBuffer[MAX_PATH];
    HANDLE token;
    TOKEN_PRIVILEGES privs;
    HKEY hKey;
    HINF hShortcutsInf;
    HANDLE hHotkeyThread;
    BOOL ret;

    InitializeSetupActionLog(FALSE);
    LogItem(NULL, L"Installing ReactOS");

    if (!InitializeProfiles())
    {
        FatalError("InitializeProfiles() failed");
        return 0;
    }

    CreateTempDir(L"TEMP");
    CreateTempDir(L"TMP");

    InitializeDefaultUserLocale();

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

    hHotkeyThread = CreateThread(NULL, 0, HotkeyThread, NULL, 0, NULL);

    /* Hack: Install TCP/IP protocol driver */
    ret = InstallInfSection(NULL,
                            L"nettcpip.inf",
                            L"MS_TCPIP.PrimaryInstall",
                            L"MS_TCPIP.PrimaryInstall.Services");
    if (!ret && GetLastError() != ERROR_FILE_NOT_FOUND)
    {
        DPRINT("InstallInfSection() failed with error 0x%lx\n", GetLastError());
    }
    else
    {
        /* Start the TCP/IP protocol driver */
        SetupStartService(L"Tcpip", FALSE);
    }


    if (!CommonInstall())
        return 0;

    InstallWizard();

    InstallSecurity();

    SetAutoAdminLogon();

    hShortcutsInf = SetupOpenInfFileW(L"shortcuts.inf",
                                      NULL,
                                      INF_STYLE_WIN4,
                                      NULL);
    if (hShortcutsInf == INVALID_HANDLE_VALUE)
    {
        FatalError("Failed to open shortcuts.inf");
        return 0;
    }

    if (!CreateShortcuts(hShortcutsInf, L"ShortcutFolders"))
    {
        FatalError("CreateShortcuts() failed");
        return 0;
    }

    SetupCloseInfFile(hShortcutsInf);

    SetupCloseInfFile(hSysSetupInf);
    SetSetupType(0);

    if (hHotkeyThread)
    {
        PostThreadMessage(GetThreadId(hHotkeyThread), WM_QUIT, 0, 0);
        CloseHandle(hHotkeyThread);
    }

    LogItem(NULL, L"Installing ReactOS done");
    TerminateSetupActionLog();

    if (AdminInfo.Name != NULL)
        RtlFreeHeap(RtlGetProcessHeap(), 0, AdminInfo.Name);

    if (AdminInfo.Domain != NULL)
        RtlFreeHeap(RtlGetProcessHeap(), 0, AdminInfo.Domain);

    if (AdminInfo.Password != NULL)
        RtlFreeHeap(RtlGetProcessHeap(), 0, AdminInfo.Password);

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


DWORD
WINAPI
SetupStartService(
    LPCWSTR lpServiceName,
    BOOL bWait)
{
    SC_HANDLE hManager = NULL;
    SC_HANDLE hService = NULL;
    DWORD dwError = ERROR_SUCCESS;

    hManager = OpenSCManagerW(NULL,
                              NULL,
                              SC_MANAGER_ALL_ACCESS);
    if (hManager == NULL)
    {
        dwError = GetLastError();
        goto done;
    }

    hService = OpenServiceW(hManager,
                            lpServiceName,
                            SERVICE_START);
    if (hService == NULL)
    {
        dwError = GetLastError();
        goto done;
    }

    if (!StartService(hService, 0, NULL))
    {
        dwError = GetLastError();
        goto done;
    }

done:
    if (hService != NULL)
        CloseServiceHandle(hService);

    if (hManager != NULL)
        CloseServiceHandle(hManager);

    return dwError;
}
