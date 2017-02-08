/*
 * PROJECT:         ReactOS System Control Panel
 * FILE:            base/applications/control/control.c
 * PURPOSE:         ReactOS System Control Panel
 * PROGRAMMERS:     Gero Kuehn (reactos.filter@gkware.com)
 *                  Colin Finck (mail@colinfinck.de)
 */

#include <stdio.h>

#define WIN32_NO_STATUS

#include <windef.h>
#include <winbase.h>
#include <winuser.h>
#include <winreg.h>
#include <shellapi.h>
#include <strsafe.h>

#include "resource.h"

#define MAX_VALUE_NAME 16383

/*
 * Macro for calling "rundll32.exe"
 * According to MSDN, ShellExecute returns a value greater than 32
 * if the operation was successful.
 */
#define RUNDLL(param)   \
    ((INT_PTR)ShellExecuteW(NULL, L"open", L"rundll32.exe", (param), NULL, SW_SHOWDEFAULT) > 32)

VOID
WINAPI
Control_RunDLLW(HWND hWnd, HINSTANCE hInst, LPCWSTR cmd, DWORD nCmdShow);

static INT
OpenShellFolder(LPWSTR lpFolderCLSID)
{
    WCHAR szParameters[MAX_PATH];

    /*
     * Open a shell folder using "explorer.exe". The passed CLSIDs
     * are all subfolders of the "Control Panel" shell folder.
     */
    StringCbCopy(szParameters, sizeof(szParameters), L"/n,::{20D04FE0-3AEA-1069-A2D8-08002B30309D}\\::{21EC2020-3AEA-1069-A2DD-08002B30309D}");
    StringCbCat(szParameters,sizeof(szParameters), lpFolderCLSID);

    return (INT_PTR)ShellExecuteW(NULL,
                                  L"open",
                                  L"explorer.exe",
                                  szParameters,
                                  NULL,
                                  SW_SHOWDEFAULT) > 32;
}

static INT
RunControlPanel(LPCWSTR lpCmd)
{
    /*
     * Old method:
     *
    WCHAR szParameters[MAX_PATH];
    wcscpy(szParameters, L"shell32.dll,Control_RunDLL ");
    wcscat(szParameters, lpCmd);
    return RUNDLL(szParameters);
     */

    /* New method: */
    Control_RunDLLW(GetDesktopWindow(), 0, lpCmd, SW_SHOW);
    return 1;
}

INT
WINAPI
wWinMain(HINSTANCE hInstance,
         HINSTANCE hPrevInstance,
         LPWSTR lpCmdLine,
         INT nCmdShow)
{
    HKEY hKey;

    /* Show the control panel window if no argument or "panel" was passed */
    if (*lpCmdLine == 0 || !_wcsicmp(lpCmdLine, L"panel"))
        return OpenShellFolder(L"");

    /* Check one of the built-in control panel handlers */
    if (!_wcsicmp(lpCmdLine, L"admintools"))           return OpenShellFolder(L"\\::{D20EA4E1-3957-11d2-A40B-0C5020524153}");
    else if (!_wcsicmp(lpCmdLine, L"color"))           return RunControlPanel(L"desk.cpl");       /* TODO: Switch to the "Apperance" tab */
    else if (!_wcsicmp(lpCmdLine, L"date/time"))       return RunControlPanel(L"timedate.cpl");
    else if (!_wcsicmp(lpCmdLine, L"desktop"))         return RunControlPanel(L"desk.cpl");
    else if (!_wcsicmp(lpCmdLine, L"folders"))         return RUNDLL(L"shell32.dll,Options_RunDLL");
    else if (!_wcsicmp(lpCmdLine, L"fonts"))           return OpenShellFolder(L"\\::{D20EA4E1-3957-11d2-A40B-0C5020524152}");
    else if (!_wcsicmp(lpCmdLine, L"infrared"))        return RunControlPanel(L"irprops.cpl");
    else if (!_wcsicmp(lpCmdLine, L"international"))   return RunControlPanel(L"intl.cpl");
    else if (!_wcsicmp(lpCmdLine, L"keyboard"))        return RunControlPanel(L"main.cpl @1");
    else if (!_wcsicmp(lpCmdLine, L"mouse"))           return RunControlPanel(L"main.cpl @0");
    else if (!_wcsicmp(lpCmdLine, L"netconnections"))  return OpenShellFolder(L"\\::{7007ACC7-3202-11D1-AAD2-00805FC1270E}");
    else if (!_wcsicmp(lpCmdLine, L"netware"))         return RunControlPanel(L"nwc.cpl");
    else if (!_wcsicmp(lpCmdLine, L"ports"))           return RunControlPanel(L"sysdm.cpl");      /* TODO: Switch to the "Computer Name" tab */
    else if (!_wcsicmp(lpCmdLine, L"printers"))        return OpenShellFolder(L"\\::{2227A280-3AEA-1069-A2DE-08002B30309D}");
    else if (!_wcsicmp(lpCmdLine, L"scannercamera"))   return OpenShellFolder(L"\\::{E211B736-43FD-11D1-9EFB-0000F8757FCD}");
    else if (!_wcsicmp(lpCmdLine, L"schedtasks"))      return OpenShellFolder(L"\\::{D6277990-4C6A-11CF-8D87-00AA0060F5BF}");
    else if (!_wcsicmp(lpCmdLine, L"telephony"))       return RunControlPanel(L"telephon.cpl");
    else if (!_wcsicmp(lpCmdLine, L"userpasswords"))   return RunControlPanel(L"nusrmgr.cpl");       /* Graphical User Account Manager */
    else if (!_wcsicmp(lpCmdLine, L"userpasswords2"))  return RUNDLL(L"netplwiz.dll,UsersRunDll");   /* Dialog based advanced User Account Manager */

    /* It is none of them, so look for a handler in the registry */
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                      L"Software\\Microsoft\\Windows\\CurrentVersion\\Control Panel\\Cpls",
                      0,
                      KEY_QUERY_VALUE,
                      &hKey) == ERROR_SUCCESS)
    {
        DWORD dwIndex;

        for (dwIndex = 0; ; ++dwIndex)
        {
            DWORD dwDataSize;
            DWORD dwValueSize = MAX_VALUE_NAME;
            WCHAR szValueName[MAX_VALUE_NAME];

            /* Get the value name and data size */
            if (RegEnumValueW(hKey,
                              dwIndex,
                              szValueName,
                              &dwValueSize,
                              0,
                              NULL,
                              NULL,
                              &dwDataSize) != ERROR_SUCCESS)
            {
                break;
            }

            /* Check if the parameter is the value name */
            if (!_wcsicmp(lpCmdLine, szValueName))
            {
                /*
                 * Allocate memory for the data plus two more characters,
                 * so we can quote the file name if required.
                 */
                LPWSTR pszData;
                pszData = HeapAlloc(GetProcessHeap(),
                                    0,
                                    dwDataSize + 2 * sizeof(WCHAR));
                ++pszData;

                /*
                 * This value is the one we are looking for, so get the data.
                 * It is the path to a .cpl file.
                 */
                if (RegQueryValueExW(hKey,
                                     szValueName,
                                     0,
                                     NULL,
                                     (LPBYTE)pszData,
                                     &dwDataSize) == ERROR_SUCCESS)
                {
                    INT nReturnValue;

                    /* Quote the file name if required */
                    if (*pszData != L'\"')
                    {
                        *(--pszData) = L'\"';
                        pszData[dwDataSize / sizeof(WCHAR)] = L'\"';
                        pszData[(dwDataSize / sizeof(WCHAR)) + 1] = 0;
                    }

                    nReturnValue = RunControlPanel(pszData);
                    HeapFree(GetProcessHeap(), 0, pszData);
                    RegCloseKey(hKey);

                    return nReturnValue;
                }

                HeapFree(GetProcessHeap(), 0, pszData);
            }
        }

        RegCloseKey(hKey);
    }

    /*
     * It's none of the known parameters, so interpret the parameter
     * as the file name of a control panel applet.
     */
    return RunControlPanel(lpCmdLine);
}
