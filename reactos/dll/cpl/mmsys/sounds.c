/* $Id: main.c 12852 2005-01-06 13:58:04Z mf $
 *
 * PROJECT:         ReactOS Multimedia Control Panel
 * FILE:            lib/cpl/mmsys/mmsys.c
 * PURPOSE:         ReactOS Multimedia Control Panel
 * PROGRAMMER:      Thomas Weidenmueller <w3seek@reactos.com>
 *                  Johannes Anderwald <janderwald@reactos.com>
 */

#include <windows.h>
#include <commctrl.h>
#include <setupapi.h>
#include <cpl.h>
#include <tchar.h>

#include "mmsys.h"
#include "resource.h"

BOOL
LoadEventLabel(HWND hwndDlg, HKEY hKey, TCHAR * szSubKey)
{
    HKEY hSubKey;
    DWORD dwData;
    DWORD dwDesc;
    TCHAR szDesc[MAX_PATH];
    TCHAR szData[MAX_PATH];


    LRESULT lResult;
    if (RegOpenKeyEx(hKey,
                     szSubKey,
                     0,
                     KEY_READ,
                     &hSubKey) != ERROR_SUCCESS)
    {
        return FALSE;
    }

    dwDesc = sizeof(szDesc) / sizeof(TCHAR);
    if (RegQueryValueEx(hSubKey,
                      NULL,
                      NULL,
                      NULL,
                      (LPBYTE)szDesc,
                      &dwDesc) != ERROR_SUCCESS)
    {
        RegCloseKey(hSubKey);
        return FALSE;
    }

    dwData = sizeof(szDesc) / sizeof(TCHAR);
    if (RegQueryValueEx(hSubKey,
                        _T("DispFileName"),
                        NULL,
                        NULL,
                        (LPBYTE)szData,
                        &dwData) != ERROR_SUCCESS)
    {
        RegCloseKey(hSubKey);
        return FALSE;
    }


    //FIXME
    //lResult = SendDlgItemMessage(hwndDlg, 
    lResult = 0;
    return TRUE;
}



BOOL
LoadEventLabels(HWND hwndDlg, HKEY hKey)
{
    HKEY hSubKey;
    DWORD dwCurKey;
    TCHAR szName[MAX_PATH];
    DWORD dwName;
    DWORD dwResult;
    DWORD dwCount;
    if (RegOpenKeyEx(hKey,
                     _T("EventLabels"),
                     0,
                     KEY_READ,
                     &hSubKey) != ERROR_SUCCESS)
    {
        return FALSE;    
    }

    dwCurKey = 0;
    dwCount = 0;
    do
    {
        dwName = sizeof(szName) / sizeof(szName[0]);
        dwResult = RegEnumKeyEx(hSubKey,
                                dwCurKey,
                                szName,
                                &dwName,
                                NULL,
                                NULL,
                                NULL,
                                NULL);

        if (dwResult == ERROR_SUCCESS)
        {
            if (LoadEventLabel(hwndDlg, hSubKey, szName))
            {
                dwCount++;
            }
        }
        dwCurKey++;

    }while(dwResult == ERROR_SUCCESS);

    RegCloseKey(hSubKey);
    return (dwCount != 0);
}

BOOL
AddSoundScheme(HWND hwndDlg, HKEY hKey, TCHAR * szSubKey, BOOL SetDefault)
{
    HKEY hSubKey;
    TCHAR szValue[MAX_PATH];
    DWORD dwValue, dwResult;

    if (RegOpenKeyEx(hKey, 
                     szSubKey,
                     0,
                     KEY_READ,
                     &hSubKey) != ERROR_SUCCESS)
    {
        return FALSE;
    }

    dwValue = sizeof(szValue) / sizeof(TCHAR);
    dwResult = RegQueryValueEx(hSubKey,
                               NULL,
                               NULL,
                               NULL,
                               (LPBYTE)szValue,
                               &dwValue);
    RegCloseKey(hSubKey);
    if (dwResult == ERROR_SUCCESS)
    {
        LRESULT lResult = SendDlgItemMessage(hwndDlg, IDC_SOUND_SCHEME, CB_ADDSTRING, (WPARAM)0, (LPARAM)szValue);
        if (lResult != CB_ERR)
        {
            ///
            /// FIXME store a context struct
            ///
            SendDlgItemMessage(hwndDlg, IDC_SOUND_SCHEME, CB_SETITEMDATA, (WPARAM)lResult, (LPARAM)szSubKey);
            if (SetDefault)
            {
                SendDlgItemMessage(hwndDlg, IDC_SOUND_SCHEME, CB_SETCURSEL, (WPARAM)lResult, (LPARAM)0);
            }
        }
        return TRUE;
    }
    return FALSE;
}



BOOL
EnumerateSoundSchemes(HWND hwndDlg, HKEY hKey)
{
    HKEY hSubKey;
    DWORD dwName, dwCurKey, dwResult, dwNumSchemes;
    TCHAR szName[MAX_PATH];
    TCHAR szDefault[MAX_PATH];

    dwName = sizeof(szDefault) / sizeof(TCHAR);
    if (RegQueryValueEx(hKey,
                        NULL,
                        NULL,
                        NULL,
                        (LPBYTE)szDefault,
                        &dwName) != ERROR_SUCCESS)
    {
        return FALSE;
    }



    if (RegOpenKeyEx(hKey,
                     _T("Names"),
                     0,
                     KEY_READ,
                     &hSubKey) != ERROR_SUCCESS)
    {
        return FALSE;    
    }
    
    dwNumSchemes = 0;
    dwCurKey = 0;
    do
    {
        dwName = sizeof(szName) / sizeof(szName[0]);
        dwResult = RegEnumKeyEx(hSubKey,
                                dwCurKey,
                                szName,
                                &dwName,
                                NULL,
                                NULL,
                                NULL,
                                NULL);

        if (dwResult == ERROR_SUCCESS)
        {
            if (AddSoundScheme(hwndDlg, hSubKey, szName, (!_tcscmp(szName, szDefault))))
            {
                dwNumSchemes++;
            }
        }

        dwCurKey++;
    }while(dwResult == ERROR_SUCCESS);

    RegCloseKey(hSubKey);
    return (dwNumSchemes != 0);
}


BOOL
LoadSoundSchemes(HWND hwndDlg, HKEY hKey)
{
    HKEY hSubKey;
    BOOL Result;

    if (RegOpenKeyEx(hKey,
                     _T("Schemes"),
                     0,
                     KEY_READ,
                     &hSubKey) != ERROR_SUCCESS)
    {
        return FALSE;    
    }

    

    Result = EnumerateSoundSchemes(hwndDlg, hSubKey);
    RegCloseKey(hSubKey);

    return Result;
}



BOOL
InitSoundSettings(HWND hwndDlg)
{
   HKEY hKey;

   if (RegOpenKey(HKEY_CURRENT_USER,
                  _T("AppEvents"),
                  &hKey) != ERROR_SUCCESS)
   {
      return FALSE;
   }

   LoadEventLabels(hwndDlg, hKey);
   LoadSoundSchemes(hwndDlg, hKey);

   RegCloseKey(hKey);
    
   return TRUE;
}


/* Sounds property page dialog callback */
INT_PTR
CALLBACK
SoundsDlgProc(HWND hwndDlg,
	        UINT uMsg,
	        WPARAM wParam,
	        LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    UNREFERENCED_PARAMETER(wParam);
    UNREFERENCED_PARAMETER(hwndDlg);
    switch(uMsg)
    {
        case WM_INITDIALOG:
            InitSoundSettings(hwndDlg);
        break;
    }

    return FALSE;
}
