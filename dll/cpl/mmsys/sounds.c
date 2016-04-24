/*
 * PROJECT:         ReactOS Multimedia Control Panel
 * FILE:            dll/cpl/mmsys/sounds.c
 * PURPOSE:         ReactOS Multimedia Control Panel
 * PROGRAMMER:      Thomas Weidenmueller <w3seek@reactos.com>
 *                  Johannes Anderwald <janderwald@reactos.com>
 *                  Dmitry Chapyshev <dmitry@reactos.org>
 */

#include "mmsys.h"

#include <commdlg.h>
#include <debug.h>

struct __APP_MAP__;

typedef struct __LABEL_MAP__
{
    TCHAR * szName;
    TCHAR * szDesc;
    TCHAR * szIcon;
    struct __APP_MAP__ * AppMap;
    struct __LABEL_MAP__ * Next;
} LABEL_MAP, *PLABEL_MAP;

typedef struct __APP_MAP__
{
    TCHAR szName[MAX_PATH];
    TCHAR szDesc[MAX_PATH];
    TCHAR szIcon[MAX_PATH];

    struct __APP_MAP__ *Next;
    PLABEL_MAP LabelMap;
} APP_MAP, *PAPP_MAP;

typedef struct __LABEL_CONTEXT__
{
    PLABEL_MAP LabelMap;
    PAPP_MAP AppMap;
    TCHAR szValue[MAX_PATH];
    struct __LABEL_CONTEXT__ *Next;
} LABEL_CONTEXT, *PLABEL_CONTEXT;

typedef struct __SOUND_SCHEME_CONTEXT__
{
    TCHAR szName[MAX_PATH];
    TCHAR szDesc[MAX_PATH];
    PLABEL_CONTEXT LabelContext;
} SOUND_SCHEME_CONTEXT, *PSOUND_SCHEME_CONTEXT;

static PLABEL_MAP s_Map = NULL;
static PAPP_MAP s_App = NULL;

TCHAR szDefault[MAX_PATH];


PLABEL_MAP FindLabel(PAPP_MAP pAppMap, TCHAR * szName)
{
    PLABEL_MAP pMap = s_Map;

    while (pMap)
    {
        ASSERT(pMap);
        ASSERT(pMap->szName);
        if (!_tcscmp(pMap->szName, szName))
            return pMap;

        pMap = pMap->Next;
    }

    pMap = pAppMap->LabelMap;

    while (pMap)
    {
        ASSERT(pMap);
        ASSERT(pMap->szName);
        if (!_tcscmp(pMap->szName, szName))
            return pMap;

        pMap = pMap->Next;
    }

    pMap = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(LABEL_MAP));
    if (!pMap)
        return NULL;

    pMap->szName = pMap->szDesc = _tcsdup(szName);
    if (!pMap->szName)
    {
        HeapFree(GetProcessHeap(), 0, pMap);
        return NULL;
    }

    pMap->AppMap = pAppMap;
    pMap->Next = s_Map;
    s_Map = pMap;

    return pMap;
}


VOID RemoveLabel(PLABEL_MAP pMap)
{
    PLABEL_MAP pCurMap = s_Map;

    if (pCurMap == pMap)
    {
        s_Map = s_Map->Next;
        return;
    }

    while (pCurMap)
    {
        if (pCurMap->Next == pMap)
        {
            pCurMap->Next = pCurMap->Next->Next;
            return;
        }
        pCurMap = pCurMap->Next;
    }
}


PAPP_MAP FindApp(TCHAR * szName)
{
    PAPP_MAP pMap = s_App;

    while (pMap)
    {
        if (!_tcscmp(pMap->szName, szName))
            return pMap;

        pMap = pMap->Next;

    }
    return NULL;
}


PLABEL_CONTEXT FindLabelContext(PSOUND_SCHEME_CONTEXT pSoundScheme, TCHAR * AppName, TCHAR * LabelName)
{
    PLABEL_CONTEXT pLabelContext;

    pLabelContext = pSoundScheme->LabelContext;

    while (pLabelContext)
    {
        ASSERT(pLabelContext->AppMap);
        ASSERT(pLabelContext->LabelMap);

        if (!_tcsicmp(pLabelContext->AppMap->szName, AppName) && !_tcsicmp(pLabelContext->LabelMap->szName, LabelName))
        {
            return pLabelContext;
        }
        pLabelContext = pLabelContext->Next;
    }

    pLabelContext = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(LABEL_CONTEXT));
    if (!pLabelContext)
        return NULL;

    pLabelContext->AppMap = FindApp(AppName);
    pLabelContext->LabelMap = FindLabel(pLabelContext->AppMap, LabelName);
    ASSERT(pLabelContext->AppMap);
    ASSERT(pLabelContext->LabelMap);
    pLabelContext->szValue[0] = _T('\0');
    pLabelContext->Next = pSoundScheme->LabelContext;
    pSoundScheme->LabelContext = pLabelContext;

    return pLabelContext;
}


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

    pMap = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(LABEL_MAP));
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

    } while (dwResult == ERROR_SUCCESS);

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
    } while (dwResult == ERROR_SUCCESS);

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
    return NULL;
}


BOOL
ImportSoundLabel(HWND hwndDlg, HKEY hKey, TCHAR * szProfile, TCHAR * szLabelName, TCHAR * szAppName, PAPP_MAP AppMap, PLABEL_MAP LabelMap)
{
    HKEY hSubKey;
    TCHAR szValue[MAX_PATH];
    TCHAR szBuffer[MAX_PATH];
    DWORD dwValue;
    PSOUND_SCHEME_CONTEXT pScheme;
    PLABEL_CONTEXT pLabelContext;
    BOOL bCurrentProfile, bActiveProfile;

    //MessageBox(hwndDlg, szProfile, szLabelName, MB_OK);

    bCurrentProfile = !_tcsicmp(szProfile, _T(".Current"));
    bActiveProfile = !_tcsicmp(szProfile, szDefault);

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

    if (bCurrentProfile)
        pScheme = FindSoundProfile(hwndDlg, szDefault);
    else
        pScheme = FindSoundProfile(hwndDlg, szProfile);

    if (!pScheme)
    {
        //MessageBox(hwndDlg, szProfile, _T("no profile!!"), MB_OK);
        return FALSE;
    }
    pLabelContext = FindLabelContext(pScheme, AppMap->szName, LabelMap->szName);

    dwValue = ExpandEnvironmentStrings(szValue, szBuffer, sizeof(szBuffer) / sizeof(TCHAR));
    if (dwValue == 0 || dwValue > (sizeof(szBuffer) / sizeof(TCHAR)))
    {
        /* fixme */
        return FALSE;
    }

    if (bCurrentProfile)
        _tcscpy(pLabelContext->szValue, szBuffer);
    else if (!bActiveProfile)
        _tcscpy(pLabelContext->szValue, szBuffer);

    return TRUE;
}


DWORD
ImportSoundEntry(HWND hwndDlg, HKEY hKey, TCHAR * szLabelName, TCHAR * szAppName, PAPP_MAP pAppMap)
{
    HKEY hSubKey;
    DWORD dwNumProfiles;
    DWORD dwCurKey;
    DWORD dwResult;
    DWORD dwProfile;
    TCHAR szProfile[MAX_PATH];
    PLABEL_MAP pLabel;

    if (RegOpenKeyEx(hKey,
                     szLabelName,
                     0,
                     KEY_READ,
                     &hSubKey) != ERROR_SUCCESS)
    {
        return FALSE;
    }
    pLabel = FindLabel(pAppMap, szLabelName);

    ASSERT(pLabel);
    RemoveLabel(pLabel);

    pLabel->AppMap = pAppMap;
    pLabel->Next = pAppMap->LabelMap;
    pAppMap->LabelMap = pLabel;

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
            if (ImportSoundLabel(hwndDlg, hSubKey, szProfile, szLabelName, szAppName, pAppMap, pLabel))
            {
                dwNumProfiles++;
            }
        }

        dwCurKey++;
    } while (dwResult == ERROR_SUCCESS);

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
    PAPP_MAP AppMap;

    //MessageBox(hwndDlg, szAppName, _T("Importing...\n"), MB_OK);

    AppMap = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(APP_MAP));
    if (!AppMap)
        return 0;

    if (RegOpenKeyEx(hKey,
                     szAppName,
                     0,
                     KEY_READ,
                     &hSubKey) != ERROR_SUCCESS)
    {
        HeapFree(GetProcessHeap(), 0, AppMap);
        return 0;
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
        HeapFree(GetProcessHeap(), 0, AppMap);
        return 0;
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

    /* initialize app map */
    _tcscpy(AppMap->szName, szAppName);
    _tcscpy(AppMap->szDesc, szDefault);
    _tcscpy(AppMap->szIcon, szIcon);

    AppMap->Next = s_App;
    s_App = AppMap;


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
            if (ImportSoundEntry(hwndDlg, hSubKey, szName, szAppName, AppMap))
            {
                dwNumEntry++;
            }
        }
        dwCurKey++;
    } while (dwResult == ERROR_SUCCESS);

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
    } while (dwResult == ERROR_SUCCESS);

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

    /* Add no sound listview item */
    if (LoadString(hApplet, IDS_NO_SOUND, szPath, MAX_PATH))
    {
        szPath[(sizeof(szPath)/sizeof(WCHAR))-1] = L'\0';
        SendDlgItemMessageW(hwndDlg, IDC_SOUND_LIST, CB_ADDSTRING, (WPARAM)0, (LPARAM)szPath);
    }

    /* Load sound files */
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
            SendDlgItemMessageW(hwndDlg, IDC_SOUND_LIST, CB_SETITEMDATA, (WPARAM)lResult, (LPARAM)_wcsdup(szPath));
        }
    } while (FindNextFileW(hFile, &FileData) != 0);

    FindClose(hFile);
    return TRUE;
}


BOOL
ShowSoundScheme(HWND hwndDlg)
{
    LRESULT lIndex;
    PSOUND_SCHEME_CONTEXT pScheme;
    PAPP_MAP pAppMap;
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

    _tcscpy(szDefault, pScheme->szName);

    /*  add column for app */
    GetClientRect(hList, &rect);
    ZeroMemory(&dummy, sizeof(LV_COLUMN));
    dummy.mask      = LVCF_WIDTH;
    dummy.iSubItem  = 0;
    dummy.cx        = rect.right - rect.left - GetSystemMetrics(SM_CXVSCROLL);
    (void)ListView_InsertColumn(hList, 0, &dummy);
    ItemIndex = 0;

    pAppMap = s_App;
    while (pAppMap)
    {
        PLABEL_MAP pLabelMap = pAppMap->LabelMap;
        while (pLabelMap)
        {
            ZeroMemory(&listItem, sizeof(LV_ITEM));
            listItem.mask       = LVIF_TEXT | LVIF_PARAM | LVIF_IMAGE;
            listItem.pszText    = pLabelMap->szDesc;
            listItem.lParam     = (LPARAM)FindLabelContext(pScheme, pAppMap->szName, pLabelMap->szName);
            listItem.iItem      = ItemIndex;
            listItem.iImage     = -1;
            (void)ListView_InsertItem(hList, &listItem);
            ItemIndex++;

            pLabelMap = pLabelMap->Next;
        }
        pAppMap = pAppMap->Next;
    }
    return TRUE;
}


BOOL
ApplyChanges(HWND hwndDlg)
{
    HKEY hKey, hSubKey;
    LRESULT lIndex;
    PSOUND_SCHEME_CONTEXT pScheme;
    HWND hDlgCtrl;
    PLABEL_CONTEXT pLabelContext;
    TCHAR Buffer[100];

    hDlgCtrl = GetDlgItem(hwndDlg, IDC_SOUND_SCHEME);

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

    if (RegOpenKeyEx(HKEY_CURRENT_USER,
                     _T("AppEvents\\Schemes"),
                     0,
                     KEY_WRITE,
                     &hKey) != ERROR_SUCCESS)
    {
        return FALSE;
    }

    RegSetValueEx(hKey, NULL, 0, REG_SZ, (LPBYTE)pScheme->szName, (_tcslen(pScheme->szName) +1) * sizeof(TCHAR));
    RegCloseKey(hKey);

    if (RegOpenKeyEx(HKEY_CURRENT_USER,
                     _T("AppEvents\\Schemes\\Apps"),
                     0,
                     KEY_WRITE,
                     &hKey) != ERROR_SUCCESS)
    {
        return FALSE;
    }

    pLabelContext = pScheme->LabelContext;

    while (pLabelContext)
    {
        _stprintf(Buffer, _T("%s\\%s\\.Current"), pLabelContext->AppMap->szName, pLabelContext->LabelMap->szName);

        if (RegOpenKeyEx(hKey, Buffer, 0, KEY_WRITE, &hSubKey) == ERROR_SUCCESS)
        {
            RegSetValueEx(hSubKey, NULL, 0, REG_EXPAND_SZ, (LPBYTE)pLabelContext->szValue, (_tcslen(pLabelContext->szValue) +1) * sizeof(TCHAR));
            RegCloseKey(hSubKey);
        }

        pLabelContext = pLabelContext->Next;
    }
    RegCloseKey(hKey);

    SetWindowLong(hwndDlg, DWL_MSGRESULT, (LONG)PSNRET_NOERROR);
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
    OPENFILENAMEW ofn;
    WCHAR filename[MAX_PATH];
    LPWSTR pFileName;
    LRESULT lResult;

    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            UINT NumWavOut = waveOutGetNumDevs();

            SendMessage(GetDlgItem(hwndDlg, IDC_PLAY_SOUND),
                        BM_SETIMAGE,(WPARAM)IMAGE_ICON,
                        (LPARAM)(HANDLE)LoadIcon(hApplet, MAKEINTRESOURCE(IDI_PLAY_ICON)));

            LoadEventLabels();
            LoadSoundProfiles(hwndDlg);
            LoadSoundFiles(hwndDlg);
            ShowSoundScheme(hwndDlg);

            if (!NumWavOut)
            {
                EnableWindow(GetDlgItem(hwndDlg, IDC_SOUND_SCHEME), FALSE);
                EnableWindow(GetDlgItem(hwndDlg, IDC_SAVEAS_BTN),   FALSE);
                EnableWindow(GetDlgItem(hwndDlg, IDC_DELETE_BTN),   FALSE);
                EnableWindow(GetDlgItem(hwndDlg, IDC_SCHEME_LIST),  FALSE);
            }

            if (wParam == (WPARAM)GetDlgItem(hwndDlg, IDC_SOUND_SCHEME))
                return TRUE;
            SetFocus(GetDlgItem(hwndDlg, IDC_SOUND_SCHEME));
            return FALSE;
        }
        case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
                case IDC_BROWSE_SOUND:
                {
                    ZeroMemory(&ofn, sizeof(OPENFILENAMEW));
                    ofn.lStructSize = sizeof(OPENFILENAMEW);
                    ofn.hwndOwner = hwndDlg;
                    ofn.lpstrFile = filename;
                    ofn.lpstrFile[0] = L'\0';
                    ofn.nMaxFile = MAX_PATH;
                    ofn.lpstrFilter = L"Wave Files (*.wav)\0*.wav\0"; //FIXME non-nls
                    ofn.nFilterIndex = 0;
                    ofn.lpstrFileTitle = L"Search for new sounds"; //FIXME non-nls
                    ofn.nMaxFileTitle = wcslen(ofn.lpstrFileTitle);
                    ofn.lpstrInitialDir = NULL;
                    ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;

                    if (GetOpenFileNameW(&ofn) == TRUE)
                    {
                         // FIXME search if list already contains that sound

                        // extract file name
                        pFileName = wcsrchr(filename, L'\\');
                        ASSERT(pFileName != NULL);
                        pFileName++;

                        // add to list
                        lResult = SendDlgItemMessageW(hwndDlg, IDC_SOUND_LIST, CB_ADDSTRING, (WPARAM)0, (LPARAM)pFileName);
                        if (lResult != CB_ERR)
                        {
                            // add path and select item
                            SendDlgItemMessageW(hwndDlg, IDC_SOUND_LIST, CB_SETITEMDATA, (WPARAM)lResult, (LPARAM)_wcsdup(filename));
                            SendDlgItemMessageW(hwndDlg, IDC_SOUND_LIST, CB_SETCURSEL, (WPARAM)lResult, (LPARAM)0);
                        }
                    }
                    break;
                }
                case IDC_PLAY_SOUND:
                {
                    LRESULT lIndex;
                    lIndex = SendDlgItemMessage(hwndDlg, IDC_SOUND_LIST, CB_GETCURSEL, (WPARAM)0, (LPARAM)0);
                    if (lIndex == CB_ERR)
                    {
                        break;
                    }

                    lIndex = SendDlgItemMessage(hwndDlg, IDC_SOUND_LIST, CB_GETITEMDATA, (WPARAM)lIndex, (LPARAM)0);
                    if (lIndex != CB_ERR)
                    {
                        PlaySound((TCHAR*)lIndex, NULL, SND_FILENAME);
                    }
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
                        PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
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
                        SelCount = ListView_GetSelectionMark(GetDlgItem(hwndDlg, IDC_SCHEME_LIST));
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
                            LRESULT lResult;
                            pLabelContext = (PLABEL_CONTEXT)item.lParam;

                            lResult = SendDlgItemMessage(hwndDlg, IDC_SOUND_LIST, CB_GETITEMDATA, (WPARAM)lIndex, (LPARAM)0);
                            if (lResult == CB_ERR || lResult == 0)
                            {
                                if (lIndex != pLabelContext->szValue[0])
                                    PropSheet_Changed(GetParent(hwndDlg), hwndDlg);

                                pLabelContext->szValue[0] = L'\0';
                                break;
                            }

                            if (_tcsicmp(pLabelContext->szValue, (TCHAR*)lResult) || (lIndex != pLabelContext->szValue[0]))
                            {
                                PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                               ///
                               /// Should store in current member
                               ///
                               _tcscpy(pLabelContext->szValue, (TCHAR*)lResult);
                            }
                            if (_tcslen((TCHAR*)lResult) && lIndex != 0)
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
            TCHAR * ptr;

            LPNMHDR lpnm = (LPNMHDR)lParam;

            switch(lpnm->code)
            {
                case PSN_APPLY:
                {
                    ApplyChanges(hwndDlg);
                    break;
                }
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
