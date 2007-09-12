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
#include <stdio.h>
#include "mmsys.h"
#include "resource.h"

typedef struct __LABEL_CONTEXT__
{
    TCHAR szName[MAX_PATH];
    TCHAR szValue[MAX_PATH];
    struct __LABEL_CONTEXT__ *Next;
}LABEL_CONTEXT, *PLABEL_CONTEXT;

typedef struct __APP_CONTEXT__
{
    TCHAR szName[MAX_PATH];
    TCHAR szDesc[MAX_PATH];
    TCHAR szIcon[MAX_PATH];
    PLABEL_CONTEXT LabelContext;
    struct __APP_CONTEXT__ * Next;
}APP_CONTEXT, *PAPP_CONTEXT;


typedef struct __SOUND_SCHEME_CONTEXT__
{
    TCHAR szName[MAX_PATH];
    TCHAR szDesc[MAX_PATH];
    PAPP_CONTEXT AppContext;
}SOUND_SCHEME_CONTEXT, *PSOUND_SCHEME_CONTEXT;


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
AddSoundProfile(HWND hwndDlg, HKEY hKey, TCHAR * szSubKey, BOOL SetDefault)
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
            PSOUND_SCHEME_CONTEXT pScheme = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(SOUND_SCHEME_CONTEXT));
            if (pScheme != NULL)
            {
                _tcscpy(pScheme->szDesc, szValue);
                _tcscpy(pScheme->szName, szSubKey);
                pScheme->AppContext = NULL;
            }

            SendDlgItemMessage(hwndDlg, IDC_SOUND_SCHEME, CB_SETITEMDATA, (WPARAM)lResult, (LPARAM)pScheme);
            if (SetDefault)
            {
                SendDlgItemMessage(hwndDlg, IDC_SOUND_SCHEME, CB_SETCURSEL, (WPARAM)lResult, (LPARAM)0);
            }
        }
        return TRUE;
    }
    return FALSE;
}



DWORD
EnumerateSoundProfiles(HWND hwndDlg, HKEY hKey)
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
            if (AddSoundProfile(hwndDlg, hSubKey, szName, (!_tcscmp(szName, szDefault))))
            {
                dwNumSchemes++;
            }
        }

        dwCurKey++;
    }while(dwResult == ERROR_SUCCESS);

    RegCloseKey(hSubKey);
    return dwNumSchemes;
}

PSOUND_SCHEME_CONTEXT FindSoundProfile(HWND hwndDlg, TCHAR * szName)
{
    LRESULT lCount, lIndex, lResult;
    PSOUND_SCHEME_CONTEXT pScheme;
    
    lCount = SendDlgItemMessage(hwndDlg, IDC_SOUND_SCHEME, CB_GETCOUNT, (WPARAM)0, (LPARAM)0);
    if (lCount == CB_ERR)
    {
        return NULL;
    }

    for(lIndex = 0; lIndex < lCount; lIndex++)
    {
        lResult = SendDlgItemMessage(hwndDlg, IDC_SOUND_SCHEME, CB_GETITEMDATA, (WPARAM)lIndex, (LPARAM)0);
        if (lResult == CB_ERR)
        {
            continue;
        }

        pScheme = (PSOUND_SCHEME_CONTEXT)lResult;
        if (!_tcsicmp(pScheme->szName, szName))
        {
            return pScheme;
        }
    }
    return FALSE;
}
PAPP_CONTEXT FindAppContext(PSOUND_SCHEME_CONTEXT pScheme, TCHAR * szAppName)
{
    PAPP_CONTEXT pAppContext = pScheme->AppContext;
    while(pAppContext)
    {
        if (!_tcsicmp(pAppContext->szName, szAppName))
            return pScheme->AppContext;
        pAppContext = pAppContext->Next;
    }
    return FALSE;
}



BOOL
ImportSoundLabel(HWND hwndDlg, HKEY hKey, TCHAR * szProfile, TCHAR * szLabelName, TCHAR * szAppName, TCHAR * szDefault)
{
    HKEY hSubKey;
    TCHAR szValue[MAX_PATH];
    DWORD dwValue;
    PSOUND_SCHEME_CONTEXT pScheme;
    PAPP_CONTEXT pAppContext;
    PLABEL_CONTEXT pLabelContext;
    BOOL Create = FALSE;

    //MessageBox(hwndDlg, szProfile, szLabelName, MB_OK);

    if (!_tcsicmp(szProfile, _T(".Current")))
    {
        //ignore current settings for now
        return TRUE;
    }

    if (RegOpenKeyEx(hKey,
                     szProfile,
                     0,
                     KEY_READ,
                     &hSubKey) != ERROR_SUCCESS)
    {
        return FALSE;
    }

    dwValue = sizeof(szValue) / sizeof(TCHAR);
    if (RegQueryValueEx(hSubKey,
                        NULL,
                        NULL,
                        NULL,
                        (LPBYTE)szValue,
                        &dwValue) != ERROR_SUCCESS)
    {
        return FALSE;
    }
    pScheme = FindSoundProfile(hwndDlg, szProfile);
    if (!pScheme)
    {
        //MessageBox(hwndDlg, szProfile, _T("no profile!!"), MB_OK);
        return FALSE;
    }

    pAppContext = FindAppContext(pScheme, szAppName);
    if (!pAppContext)
    {
        pAppContext = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(APP_CONTEXT));
        Create = TRUE;
    }
    
    if (!pAppContext)
    {

        //MessageBox(hwndDlg, szAppName, _T("no appcontext"), MB_OK);
        return FALSE;
    }

    pLabelContext = (PLABEL_CONTEXT)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(LABEL_CONTEXT));
    _tcscpy(pLabelContext->szName, szLabelName);
    _tcscpy(pLabelContext->szValue, szValue);
    pLabelContext->Next = pAppContext->LabelContext;
    pAppContext->LabelContext = pLabelContext;
    
    if (Create)
    {
        _tcscpy(pAppContext->szName, szAppName);
        pAppContext->Next = pScheme->AppContext;
        pScheme->AppContext = pAppContext;
    }
    return TRUE;
}


DWORD
ImportSoundEntry(HWND hwndDlg, HKEY hKey, TCHAR * szLabelName, TCHAR * szAppName, TCHAR * szDefault)
{
    HKEY hSubKey;
    DWORD dwNumProfiles;
    DWORD dwCurKey;
    DWORD dwResult;
    DWORD dwProfile;
    TCHAR szProfile[MAX_PATH];

    if (RegOpenKeyEx(hKey,
                     szLabelName,
                     0,
                     KEY_READ,
                     &hSubKey) != ERROR_SUCCESS)
    {
        return FALSE;
    }

    //MessageBox(hwndDlg, szLabelName, szAppName, MB_OK);

    dwNumProfiles = 0;
    dwCurKey = 0;
    do
    {
        dwProfile = sizeof(szProfile) / sizeof(TCHAR);
        dwResult = RegEnumKeyEx(hSubKey,
                                dwCurKey,
                                szProfile,
                                &dwProfile,
                                NULL,
                                NULL,
                                NULL,
                                NULL);

        if (dwResult == ERROR_SUCCESS)
        {
            if (ImportSoundLabel(hwndDlg, hSubKey, szProfile, szLabelName, szAppName, szDefault))
            {
                dwNumProfiles++;
            }
        }

        dwCurKey++;
    }while(dwResult == ERROR_SUCCESS);

    RegCloseKey(hSubKey);
    return dwNumProfiles;
}



DWORD
ImportAppProfile(HWND hwndDlg, HKEY hKey, TCHAR * szAppName)
{
    HKEY hSubKey;
    TCHAR szDefault[MAX_PATH];
    DWORD dwDefault;
    DWORD dwCurKey;
    DWORD dwResult;
    DWORD dwNumEntry;
    DWORD dwName;
    TCHAR szName[MAX_PATH];

    //MessageBox(hwndDlg, szAppName, _T("Importing...\n"), MB_OK);

    if (RegOpenKeyEx(hKey,
                     szAppName,
                     0,
                     KEY_READ,
                     &hSubKey) != ERROR_SUCCESS)
    {
        return FALSE;
    }

    dwDefault = sizeof(szDefault) / sizeof(TCHAR);
    if (RegQueryValueEx(hSubKey,
                        NULL,
                        NULL,
                        NULL,
                        (LPBYTE)szDefault,
                        &dwDefault) != ERROR_SUCCESS)
    {
        RegCloseKey(hSubKey);
        return FALSE;
    }

    dwCurKey = 0;
    dwNumEntry = 0;
    do
    {
        dwName = sizeof(szName) / sizeof(TCHAR);
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
            if (ImportSoundEntry(hwndDlg, hSubKey, szName, szAppName, szDefault))
            {
                dwNumEntry++;
            }
        }
        dwCurKey++;
    }while(dwResult == ERROR_SUCCESS);
    RegCloseKey(hSubKey);
    return dwNumEntry;
}

BOOL
ImportSoundProfiles(HWND hwndDlg, HKEY hKey)
{
    DWORD dwCurKey;
    DWORD dwResult;
    DWORD dwNumApps;
    TCHAR szName[MAX_PATH];
    HKEY hSubKey;

    if (RegOpenKeyEx(hKey,
                     _T("Apps"),
                     0,
                     KEY_READ,
                     &hSubKey) != ERROR_SUCCESS)
    {
        return FALSE;
    }

    dwNumApps = 0;
    dwCurKey = 0;
    do
    {
        dwResult = RegEnumKey(hSubKey,
                              dwCurKey,
                              szName,
                              sizeof(szName) / sizeof(TCHAR));

        if (dwResult == ERROR_SUCCESS)
        {
            if (ImportAppProfile(hwndDlg, hSubKey, szName))
            {
                dwNumApps++;
            }
        }
        dwCurKey++;
    }while(dwResult == ERROR_SUCCESS);
    RegCloseKey(hSubKey);

    return (dwNumApps != 0);
}



BOOL
LoadSoundProfiles(HWND hwndDlg, HKEY hKey)
{
    HKEY hSubKey;
    DWORD dwNumSchemes;

    if (RegOpenKeyEx(hKey,
                     _T("Schemes"),
                     0,
                     KEY_READ,
                     &hSubKey) != ERROR_SUCCESS)
    {
        return FALSE;    
    }

    dwNumSchemes = EnumerateSoundProfiles(hwndDlg, hSubKey);


    if (dwNumSchemes)
    {
        //MessageBox(hwndDlg, _T("importing sound profiles..."), NULL, MB_OK);
        ImportSoundProfiles(hwndDlg, hSubKey);            
    }
    RegCloseKey(hSubKey);
    return FALSE;
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
   LoadSoundProfiles(hwndDlg, hKey);
   //LoadEventLabels(hwndDlg, hKey);

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
