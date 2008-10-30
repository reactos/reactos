/* $Id: main.c 12852 2005-01-06 13:58:04Z mf $
 *
 * PROJECT:         ReactOS Multimedia Control Panel
 * FILE:            lib/cpl/mmsys/mmsys.c
 * PURPOSE:         ReactOS Multimedia Control Panel
 * PROGRAMMER:      Thomas Weidenmueller <w3seek@reactos.com>
 *                  Johannes Anderwald <janderwald@reactos.com>
 *                  Dmitry Chapyshev <dmitry@reactos.org>
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

typedef struct __LABEL_MAP__
{
    TCHAR * szName;
    TCHAR * szDesc;
    TCHAR * szIcon;
    struct __LABEL_MAP__ * Next;
}LABEL_MAP, *PLABEL_MAP;

static PLABEL_MAP s_Map = NULL;


BOOL
LoadEventLabel(HKEY hKey, TCHAR * szSubKey)
{
    HKEY hSubKey;
    DWORD dwData;
    DWORD dwDesc;
    TCHAR szDesc[MAX_PATH];
    TCHAR szData[MAX_PATH];
    PLABEL_MAP pMap;

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

    pMap = HeapAlloc(GetProcessHeap(), 0, sizeof(LABEL_MAP));
    if (!pMap)
    {
        return FALSE;
    }
    pMap->szName = _tcsdup(szSubKey);
    pMap->szDesc = _tcsdup(szDesc);
    pMap->szIcon = _tcsdup(szData);

    if (s_Map)
    {
        pMap->Next = s_Map;
        s_Map = pMap;
    }
    else
    {
        s_Map = pMap;
        s_Map->Next = 0;
    }
    return TRUE;
}

BOOL
LoadEventLabels()
{
    HKEY hSubKey;
    DWORD dwCurKey;
    TCHAR szName[MAX_PATH];
    DWORD dwName;
    DWORD dwResult;
    DWORD dwCount;
    if (RegOpenKeyEx(HKEY_CURRENT_USER,
                     _T("AppEvents\\EventLabels"),
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
            if (LoadEventLabel(hSubKey, szName))
            {
                dwCount++;
            }
        }
        dwCurKey++;

    }while(dwResult == ERROR_SUCCESS);

    RegCloseKey(hSubKey);
    return (dwCount != 0);
}
PLABEL_MAP FindLabel(TCHAR * szName)
{
    PLABEL_MAP pMap = s_Map;

    while(pMap)
    {
        if (!_tcscmp(pMap->szName, szName))
            return pMap;

        pMap = pMap->Next;

    }
    return NULL;
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
            if (AddSoundProfile(hwndDlg, hSubKey, szName, (!_tcsicmp(szName, szDefault))))
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
ImportSoundLabel(HWND hwndDlg, HKEY hKey, TCHAR * szProfile, TCHAR * szLabelName, TCHAR * szAppName)
{
    HKEY hSubKey;
    TCHAR szValue[MAX_PATH];
    TCHAR szBuffer[MAX_PATH];
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
    if (!pLabelContext)
    {
        HeapFree(GetProcessHeap(), 0, pLabelContext);
        if (Create)
        {
            HeapFree(GetProcessHeap(), 0, pAppContext);
        }
        return FALSE;
    }
    dwValue = ExpandEnvironmentStrings(szValue, szBuffer, sizeof(szBuffer) / sizeof(TCHAR));
    if (dwValue == 0 || dwValue > (sizeof(szBuffer) / sizeof(TCHAR)))
    {
        /* fixme */
        HeapFree(GetProcessHeap(), 0, pLabelContext);
        if (Create)
        {
            HeapFree(GetProcessHeap(), 0, pAppContext);
        }
        return FALSE;
    }
    _tcscpy(pLabelContext->szValue, szBuffer);
    _tcscpy(pLabelContext->szName, szLabelName);
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
ImportSoundEntry(HWND hwndDlg, HKEY hKey, TCHAR * szLabelName, TCHAR * szAppName)
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
            if (ImportSoundLabel(hwndDlg, hSubKey, szProfile, szLabelName, szAppName))
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
    TCHAR szIcon[MAX_PATH];

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

    dwDefault = sizeof(szIcon) / sizeof(TCHAR);
    if (RegQueryValueEx(hSubKey,
                        _T("DispFileName"),
                        NULL,
                        NULL,
                        (LPBYTE)szIcon,
                        &dwDefault) != ERROR_SUCCESS)
    {
        szIcon[0] = _T('\0');
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
            if (ImportSoundEntry(hwndDlg, hSubKey, szName, szAppName))
            {
                LRESULT lCount = SendDlgItemMessage(hwndDlg, IDC_SOUND_SCHEME, CB_GETCOUNT, (WPARAM)0, (LPARAM)0);
                if (lCount != CB_ERR)
                {
                    LRESULT lIndex;
                    LRESULT lResult;
                    for (lIndex = 0; lIndex < lCount; lIndex++)
                    {
                        lResult = SendDlgItemMessage(hwndDlg, IDC_SOUND_SCHEME, CB_GETITEMDATA, (WPARAM)lIndex, (LPARAM)0);
                        if (lResult != CB_ERR)
                        {
                            PAPP_CONTEXT pAppContext;
                            PSOUND_SCHEME_CONTEXT pScheme = (PSOUND_SCHEME_CONTEXT)lResult;
                            if (!_tcsicmp(pScheme->szName, _T(".None")))
                            {
                                continue;
                            }
                            pAppContext = FindAppContext(pScheme, szAppName);
                            if (pAppContext)
                            {
                                _tcscpy(pAppContext->szDesc, szDefault);
                                _tcscpy(pAppContext->szIcon, szIcon);
                            }

                        }
                    }


                }


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
LoadSoundProfiles(HWND hwndDlg)
{
    HKEY hSubKey;
    DWORD dwNumSchemes;

    if (RegOpenKeyEx(HKEY_CURRENT_USER,
                     _T("AppEvents\\Schemes"),
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
LoadSoundFiles(HWND hwndDlg)
{
    WCHAR szPath[MAX_PATH];
    WCHAR * ptr;
    WIN32_FIND_DATAW FileData;
    HANDLE hFile;
    LRESULT lResult;
    UINT length;

    length = GetWindowsDirectoryW(szPath, MAX_PATH);
    if (length == 0 || length >= MAX_PATH - 9)
    {
        return FALSE;
    }
    if (szPath[length-1] != L'\\')
    {
        szPath[length] = L'\\';
        length++;
    }
    wcscpy(&szPath[length], L"media\\*");
    length += 7;

    hFile = FindFirstFileW(szPath, &FileData);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        return FALSE;
    }
    if (LoadString(hApplet, IDS_NO_SOUND, szPath, MAX_PATH))
    {
        szPath[(sizeof(szPath)/sizeof(WCHAR))-1] = L'\0';
        SendDlgItemMessageW(hwndDlg, IDC_SOUND_LIST, CB_ADDSTRING, (WPARAM)0, (LPARAM)szPath);
    }

    do
    {
        if (FileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            continue;

        ptr = wcsrchr(FileData.cFileName, L'\\');
        if (ptr)
        {
            ptr++;
        }
        else
        {
            ptr = FileData.cFileName;
        }
        lResult = SendDlgItemMessageW(hwndDlg, IDC_SOUND_LIST, CB_ADDSTRING, (WPARAM)0, (LPARAM)ptr);
        if (lResult != CB_ERR)
        {
            wcscpy(&szPath[length-1], FileData.cFileName);
            SendDlgItemMessageW(hwndDlg, IDC_SOUND_LIST, CB_SETITEMDATA, (WPARAM)lResult, (LPARAM)wcsdup(szPath));
        }
    }while(FindNextFileW(hFile, &FileData) != 0);

    FindClose(hFile);
    return TRUE;
}



BOOL
ShowSoundScheme(HWND hwndDlg)
{
    LRESULT lIndex;
    PSOUND_SCHEME_CONTEXT pScheme;
    PAPP_CONTEXT pAppContext;
    LV_ITEM listItem;
    LV_COLUMN dummy;
    HWND hDlgCtrl, hList;
    RECT rect;
    int ItemIndex;
    hDlgCtrl = GetDlgItem(hwndDlg, IDC_SOUND_SCHEME);
    hList = GetDlgItem(hwndDlg, IDC_SCHEME_LIST);

    lIndex = SendMessage(hDlgCtrl, CB_GETCURSEL, (WPARAM)0, (LPARAM)0);
    if (lIndex == CB_ERR)
    {
        return FALSE;
    }

    lIndex = SendMessage(hDlgCtrl, CB_GETITEMDATA, (WPARAM)lIndex, (LPARAM)0);
    if (lIndex == CB_ERR)
    {
        return FALSE;
    }
    pScheme = (PSOUND_SCHEME_CONTEXT)lIndex;
    pAppContext = pScheme->AppContext;
    if (!pAppContext)
    {
        return FALSE;
    }

    /*  add column for app */
    GetClientRect(hList, &rect);
    ZeroMemory(&dummy, sizeof(LV_COLUMN));
    dummy.mask      = LVCF_WIDTH;
    dummy.iSubItem  = 0;
    dummy.cx        = rect.right - rect.left - GetSystemMetrics(SM_CXVSCROLL);
    (void)ListView_InsertColumn(hList, 0, &dummy);
    ItemIndex = 0;
    while(pAppContext)
    {
        PLABEL_CONTEXT pLabelContext = pAppContext->LabelContext;
        if (pLabelContext)
        {
#if 0
            ZeroMemory(&listItem, sizeof(LV_ITEM));
            listItem.mask       = LVIF_TEXT | LVIF_PARAM | LVIF_IMAGE;
            listItem.pszText    = pAppContext->szDesc;
            listItem.lParam     = (LPARAM)pAppContext;
            listItem.iItem      = ItemIndex;
            listItem.iImage     = -1;
            (void)ListView_InsertItem(hList, &listItem);
            ItemIndex++;
#endif
            while(pLabelContext)
            {
                PLABEL_MAP pMap = FindLabel(pLabelContext->szName);
                if (pMap)
                {
                    ZeroMemory(&listItem, sizeof(LV_ITEM));
                    listItem.mask       = LVIF_TEXT | LVIF_PARAM | LVIF_IMAGE;
                    listItem.pszText    = pMap->szDesc;
                    listItem.lParam     = (LPARAM)pLabelContext;
                    listItem.iItem      = ItemIndex;
                    listItem.iImage     = -1;
                    (void)ListView_InsertItem(hList, &listItem);
                    ItemIndex++;
                }
                pLabelContext = pLabelContext->Next;
            }
        }
        pAppContext = pAppContext->Next;
    }
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
    switch(uMsg)
    {
        case WM_INITDIALOG:
        {
            UINT NumWavOut;

            NumWavOut = waveOutGetNumDevs();
            if (!NumWavOut)
            {
                EnableWindow(GetDlgItem(hwndDlg, IDC_SOUND_SCHEME), FALSE);
                EnableWindow(GetDlgItem(hwndDlg, IDC_SAVEAS_BTN),   FALSE);
                EnableWindow(GetDlgItem(hwndDlg, IDC_DELETE_BTN),   FALSE);
                EnableWindow(GetDlgItem(hwndDlg, IDC_SCHEME_LIST),  FALSE);
            }

            SendMessage(GetDlgItem(hwndDlg, IDC_PLAY_SOUND),
                        BM_SETIMAGE,(WPARAM)IMAGE_ICON,
                        (LPARAM)(HANDLE)LoadIcon(hApplet, MAKEINTRESOURCE(IDI_PLAY_ICON)));

            LoadEventLabels();
            LoadSoundProfiles(hwndDlg);
            LoadSoundFiles(hwndDlg);
            ShowSoundScheme(hwndDlg);
            if (wParam == (WPARAM)GetDlgItem(hwndDlg, IDC_SOUND_SCHEME))
                return TRUE;
            SetFocus(GetDlgItem(hwndDlg, IDC_SOUND_SCHEME));
            return FALSE;
        }
        case WM_COMMAND:
        {
            switch(LOWORD(wParam))
            {
                case IDC_PLAY_SOUND:
                {
                    LRESULT lIndex;
                    TCHAR szValue[MAX_PATH];

                    lIndex = SendDlgItemMessage(hwndDlg, IDC_SOUND_LIST, CB_GETCURSEL, (WPARAM)0, (LPARAM)0);
                    if (lIndex == CB_ERR)
                    {
                        break;
                    }
                    SendDlgItemMessage(hwndDlg, IDC_SOUND_LIST, CB_GETLBTEXT, (WPARAM)lIndex, (LPARAM)szValue);
                    PlaySound(szValue, NULL, SND_FILENAME);
                    break;
                }
                case IDC_SOUND_SCHEME:
                {
                    if (HIWORD(wParam) == CBN_SELENDOK)
                    {
                        (void)ListView_DeleteAllItems(GetDlgItem(hwndDlg, IDC_SCHEME_LIST));
                        ShowSoundScheme(hwndDlg);
                        EnableWindow(GetDlgItem(hwndDlg, IDC_SOUND_LIST), FALSE);
                        EnableWindow(GetDlgItem(hwndDlg, IDC_TEXT_SOUND), FALSE);
                        EnableWindow(GetDlgItem(hwndDlg, IDC_PLAY_SOUND), FALSE);
                        EnableWindow(GetDlgItem(hwndDlg, IDC_BROWSE_SOUND), FALSE);
                    }
                    break;
                }
                case IDC_SOUND_LIST:
                {
                    if (HIWORD(wParam) == CBN_SELENDOK)
                    {
                        PLABEL_CONTEXT pLabelContext;
                        INT SelCount;
                        LVITEM item;
                        LRESULT lIndex;
                        SelCount = ListView_GetSelectionMark(GetDlgItem(hwndDlg, IDC_SOUND_LIST));
                        if (SelCount == -1)
                        {
                            break;
                        }
                        lIndex = SendDlgItemMessage(hwndDlg, IDC_SOUND_LIST, CB_GETCURSEL, (WPARAM)0, (LPARAM)0);
                        if (lIndex == CB_ERR)
                        {
                            break;
                        }
                        ZeroMemory(&item, sizeof(LVITEM));
                        item.mask = LVIF_PARAM;
                        item.iItem = SelCount;
                        if (ListView_GetItem(GetDlgItem(hwndDlg, IDC_SCHEME_LIST), &item))
                        {
                            TCHAR szValue[MAX_PATH];
                            pLabelContext = (PLABEL_CONTEXT)item.lParam;
                            SendDlgItemMessage(hwndDlg, IDC_SOUND_LIST, CB_GETLBTEXT, (WPARAM)lIndex, (LPARAM)szValue);
                            ///
                            /// should store in current member
                            ///
                            _tcscpy(pLabelContext->szValue, szValue);
                            if (_tcslen(szValue) && lIndex != 0)
                            {
                                EnableWindow(GetDlgItem(hwndDlg, IDC_PLAY_SOUND), TRUE);
                            }
                            else
                            {
                                EnableWindow(GetDlgItem(hwndDlg, IDC_PLAY_SOUND), FALSE);
                            }
                        }
                    }
                    break;
                }
            }
            break;
        }
        case WM_NOTIFY:
        {
            LVITEM item;
            PLABEL_CONTEXT pLabelContext;
            LPNMHDR lpnm = (LPNMHDR)lParam;
            TCHAR * ptr;
            switch(lpnm->code)
            {
                case LVN_ITEMCHANGED:
                {
                    LPNMLISTVIEW nm = (LPNMLISTVIEW)lParam;

                    if ((nm->uNewState & LVIS_SELECTED) == 0)
                    {
                        return FALSE;
                    }
                    ZeroMemory(&item, sizeof(LVITEM));
                    item.mask = LVIF_PARAM;
                    item.iItem = nm->iItem;
                    if (ListView_GetItem(GetDlgItem(hwndDlg, IDC_SCHEME_LIST), &item))
                    {
                        LRESULT lCount, lIndex, lResult;
                        pLabelContext = (PLABEL_CONTEXT)item.lParam;
                        if (!pLabelContext)
                        {
                            return FALSE;
                        }
                        EnableWindow(GetDlgItem(hwndDlg, IDC_SOUND_LIST), TRUE);
                        EnableWindow(GetDlgItem(hwndDlg, IDC_TEXT_SOUND), TRUE);
                        EnableWindow(GetDlgItem(hwndDlg, IDC_BROWSE_SOUND), TRUE);
                        if (_tcslen(pLabelContext->szValue) == 0)
                        {
                            lIndex = SendDlgItemMessage(hwndDlg, IDC_SOUND_LIST, CB_SETCURSEL, (WPARAM)0, (LPARAM)0);
                            EnableWindow(GetDlgItem(hwndDlg, IDC_PLAY_SOUND), FALSE);
                            break;

                        }
                        EnableWindow(GetDlgItem(hwndDlg, IDC_PLAY_SOUND), TRUE);
                        lCount = SendDlgItemMessage(hwndDlg, IDC_SOUND_LIST, CB_GETCOUNT, (WPARAM)0, (LPARAM)0);
                        for (lIndex = 0; lIndex < lCount; lIndex++)
                        {
                            lResult = SendDlgItemMessage(hwndDlg, IDC_SOUND_LIST, CB_GETITEMDATA, (WPARAM)lIndex, (LPARAM)0);
                            if (lResult == CB_ERR || lResult == 0)
                                continue;

                            if (!_tcscmp((TCHAR*)lResult, pLabelContext->szValue))
                            {
                                SendDlgItemMessage(hwndDlg, IDC_SOUND_LIST, CB_SETCURSEL, (WPARAM)lIndex, (LPARAM)0);
                                return FALSE;
                            }
                        }
                        ptr = _tcsrchr(pLabelContext->szValue, _T('\\'));
                        if (ptr)
                        {
                            ptr++;
                        }
                        else
                        {
                            ptr = pLabelContext->szValue;
                        }
                        lIndex = SendDlgItemMessage(hwndDlg, IDC_SOUND_LIST, CB_ADDSTRING, (WPARAM)0, (LPARAM)ptr);
                        if (lIndex != CB_ERR)
                        {
                            SendDlgItemMessage(hwndDlg, IDC_SOUND_LIST, CB_SETITEMDATA, (WPARAM)lIndex, (LPARAM)_tcsdup(pLabelContext->szValue));
                            SendDlgItemMessage(hwndDlg, IDC_SOUND_LIST, CB_SETCURSEL, (WPARAM)lIndex, (LPARAM)0);
                        }
                    }
                    break;
                }
            }
        }
        break;
    }

    return FALSE;
}
