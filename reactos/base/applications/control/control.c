/*
 * PROJECT:         ReactOS System Control Panel
 * FILE:            base/applications/control/control.c
 * PURPOSE:         ReactOS System Control Panel
 * PROGRAMMERS:     Gero Kuehn (reactos.filter@gkware.com)
 *                  Colin Finck (mail@colinfinck.de)
 */

#include "control.h"

static const TCHAR szWindowClass[] = _T("DummyControlClass");

HANDLE hProcessHeap;
HINSTANCE hInst;

static
INT
OpenShellFolder(LPTSTR lpFolderCLSID)
{
    TCHAR szParameters[MAX_PATH];

    /* Open a shell folder using "explorer.exe".
       The passed CLSID's are all subfolders of the "Control Panel" shell folder. */
    _tcscpy(szParameters, _T("/n,::{20D04FE0-3AEA-1069-A2D8-08002B30309D}\\::{21EC2020-3AEA-1069-A2DD-08002B30309D}"));
    _tcscat(szParameters, lpFolderCLSID);

    return (int) ShellExecute(NULL,
                              _T("open"),
                              _T("explorer.exe"),
                              szParameters,
                              NULL,
                              SW_SHOWDEFAULT) > 32;
}

static
INT
RunControlPanel(LPTSTR lpCmd)
{
    TCHAR szParameters[MAX_PATH];

    _tcscpy(szParameters, _T("shell32.dll,Control_RunDLL "));
    _tcscat(szParameters, lpCmd);

    return RUNDLL(szParameters);
}

int
WINAPI
_tWinMain(HINSTANCE hInstance,
          HINSTANCE hPrevInstance,
          LPTSTR lpCmdLine,
          int nCmdShow)
{
    HKEY hKey;

    hInst = hInstance;
    hProcessHeap = GetProcessHeap();

    /* Show the control panel window if no argument or "panel" was passed */
    if(lpCmdLine[0] == 0 || !_tcsicmp(lpCmdLine, _T("panel")))
        return OpenShellFolder(_T(""));

    /* Check one of the built-in control panel handlers */
    if (!_tcsicmp(lpCmdLine, _T("admintools")))           return OpenShellFolder(_T("\\::{D20EA4E1-3957-11d2-A40B-0C5020524153}"));
    else if (!_tcsicmp(lpCmdLine, _T("color")))           return RunControlPanel(_T("desk.cpl"));       /* TODO: Switch to the "Apperance" tab */
    else if (!_tcsicmp(lpCmdLine, _T("date/time")))       return RunControlPanel(_T("timedate.cpl"));
    else if (!_tcsicmp(lpCmdLine, _T("desktop")))         return RunControlPanel(_T("desk.cpl"));
    else if (!_tcsicmp(lpCmdLine, _T("folders")))         return RUNDLL(_T("shell32.dll,Options_RunDLL"));
    else if (!_tcsicmp(lpCmdLine, _T("fonts")))           return OpenShellFolder(_T("\\::{D20EA4E1-3957-11d2-A40B-0C5020524152}"));
    else if (!_tcsicmp(lpCmdLine, _T("infrared")))        return RunControlPanel(_T("irprops.cpl"));
    else if (!_tcsicmp(lpCmdLine, _T("international")))   return RunControlPanel(_T("intl.cpl"));
    else if (!_tcsicmp(lpCmdLine, _T("keyboard")))        return RunControlPanel(_T("main.cpl @1"));
    else if (!_tcsicmp(lpCmdLine, _T("mouse")))           return RunControlPanel(_T("main.cpl @0"));
    else if (!_tcsicmp(lpCmdLine, _T("netconnections")))  return OpenShellFolder(_T("\\::{7007ACC7-3202-11D1-AAD2-00805FC1270E}"));
    else if (!_tcsicmp(lpCmdLine, _T("netware")))         return RunControlPanel(_T("nwc.cpl"));
    else if (!_tcsicmp(lpCmdLine, _T("ports")))           return RunControlPanel(_T("sysdm.cpl"));      /* TODO: Switch to the "Computer Name" tab */
    else if (!_tcsicmp(lpCmdLine, _T("printers")))        return OpenShellFolder(_T("\\::{2227A280-3AEA-1069-A2DE-08002B30309D}"));
    else if (!_tcsicmp(lpCmdLine, _T("scannercamera")))   return OpenShellFolder(_T("\\::{E211B736-43FD-11D1-9EFB-0000F8757FCD}"));
    else if (!_tcsicmp(lpCmdLine, _T("schedtasks")))      return OpenShellFolder(_T("\\::{D6277990-4C6A-11CF-8D87-00AA0060F5BF}"));
    else if (!_tcsicmp(lpCmdLine, _T("telephony")))       return RunControlPanel(_T("telephon.cpl"));
    else if (!_tcsicmp(lpCmdLine, _T("userpasswords")))   return RunControlPanel(_T("nusrmgr.cpl"));       /* Graphical User Account Manager */
    else if (!_tcsicmp(lpCmdLine, _T("userpasswords2")))  return RUNDLL(_T("netplwiz.dll,UsersRunDll"));   /* Dialog based advanced User Account Manager */

    /* It is none of them, so look for a handler in the registry */
    if(RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                    _T("Software\\Microsoft\\Windows\\CurrentVersion\\Control Panel\\Cpls"),
                    0,
                    KEY_QUERY_VALUE,
                    &hKey) == ERROR_SUCCESS)
    {
        DWORD dwIndex;

        for(dwIndex = 0; ; ++dwIndex)
        {
            DWORD dwDataSize;
            DWORD dwValueSize = MAX_VALUE_NAME;
            TCHAR szValueName[MAX_VALUE_NAME];

            /* Get the value name and data size */
            if(RegEnumValue(hKey,
                            dwIndex,
                            szValueName,
                            &dwValueSize,
                            0,
                            NULL,
                            NULL,
                            &dwDataSize) != ERROR_SUCCESS)
                break;

            /* Check if the parameter is the value name */
            if(!_tcsicmp(lpCmdLine, szValueName))
            {
                LPTSTR pszData;

                /* Allocate memory for the data plus two more characters, so we can quote the file name if required */
                pszData = (LPTSTR) HeapAlloc(hProcessHeap,
                                             0,
                                             dwDataSize + 2 * sizeof(TCHAR));
                ++pszData;

                /* This value is the one we are looking for, so get the data. It is the path to a .cpl file */
                if(RegQueryValueEx(hKey,
                                   szValueName,
                                   0,
                                   NULL,
                                   (LPBYTE)pszData,
                                   &dwDataSize) == ERROR_SUCCESS)
                {
                    INT nReturnValue;

                    /* Quote the file name if required */
                    if(*pszData != '\"')
                    {
                        *(--pszData) = '\"';
                        pszData[dwDataSize / sizeof(TCHAR)] = '\"';
                        pszData[(dwDataSize / sizeof(TCHAR)) + 1] = 0;
                    }

                    nReturnValue = RunControlPanel(pszData);
                    HeapFree(hProcessHeap,
                             0,
                             pszData);
                    RegCloseKey(hKey);

                    return nReturnValue;
                }

                HeapFree(hProcessHeap,
                         0,
                         pszData);
            }
        }

        RegCloseKey(hKey);
    }

    /* It's none of the known parameters, so interpret the parameter as the file name of a control panel applet */
    return RunControlPanel(lpCmdLine);
}
