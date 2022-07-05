/*
 * PROJECT:         ReactOS Multimedia Control Panel
 * FILE:            dll/cpl/mmsys/sounds.c
 * PURPOSE:         ReactOS Multimedia Control Panel
 * PROGRAMMER:      Thomas Weidenmueller <w3seek@reactos.com>
 *                  Johannes Anderwald <janderwald@reactos.com>
 *                  Dmitry Chapyshev <dmitry@reactos.org>
 *                  Victor Martinez Calvo <victor.martinez@reactos.org>
 *                  Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "mmsys.h"

#include <commdlg.h>
#include <windowsx.h>

#include <debug.h>

typedef struct _LABEL_MAP
{
    WCHAR *szName;
    WCHAR *szDesc;
    WCHAR *szIcon;
    struct _APP_MAP *AppMap;
    struct _LABEL_MAP *Next;
} LABEL_MAP, *PLABEL_MAP;

typedef struct _APP_MAP
{
    WCHAR szName[MAX_PATH];
    WCHAR szDesc[MAX_PATH];
    WCHAR szIcon[MAX_PATH];

    struct _APP_MAP *Next;
    PLABEL_MAP LabelMap;
} APP_MAP, *PAPP_MAP;

typedef struct _LABEL_CONTEXT
{
    PLABEL_MAP LabelMap;
    PAPP_MAP AppMap;
    WCHAR szValue[MAX_PATH];
    struct _LABEL_CONTEXT *Next;
} LABEL_CONTEXT, *PLABEL_CONTEXT;

typedef struct _SOUND_SCHEME_CONTEXT
{
    WCHAR szName[MAX_PATH];
    WCHAR szDesc[MAX_PATH];
    PLABEL_CONTEXT LabelContext;
} SOUND_SCHEME_CONTEXT, *PSOUND_SCHEME_CONTEXT;

typedef struct _GLOBAL_DATA
{
    WCHAR szDefault[MAX_PATH];
    HIMAGELIST hSoundsImageList;
    PLABEL_MAP pLabelMap;
    PAPP_MAP pAppMap;
    UINT NumWavOut;
} GLOBAL_DATA, *PGLOBAL_DATA;


/* A filter string is a list separated by NULL and ends with double NULLs. */
LPWSTR MakeFilter(LPWSTR psz)
{
    WCHAR *pch;

    ASSERT(psz[0] != UNICODE_NULL &&
           psz[wcslen(psz) - 1] == L'|');
    for (pch = psz; *pch != UNICODE_NULL; pch++)
    {
        /* replace vertical bar with NULL */
        if (*pch == L'|')
        {
            *pch = UNICODE_NULL;
        }
    }
    return psz;
}

PLABEL_MAP FindLabel(PGLOBAL_DATA pGlobalData, PAPP_MAP pAppMap, WCHAR *szName)
{
    PLABEL_MAP pMap = pGlobalData->pLabelMap;

    while (pMap)
    {
        ASSERT(pMap);
        ASSERT(pMap->szName);
        if (!wcscmp(pMap->szName, szName))
            return pMap;

        pMap = pMap->Next;
    }

    pMap = pAppMap->LabelMap;

    while (pMap)
    {
        ASSERT(pMap);
        ASSERT(pMap->szName);
        if (!wcscmp(pMap->szName, szName))
            return pMap;

        pMap = pMap->Next;
    }

    pMap = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(LABEL_MAP));
    if (!pMap)
        return NULL;

    pMap->szName = pMap->szDesc = _wcsdup(szName);
    if (!pMap->szName)
    {
        HeapFree(GetProcessHeap(), 0, pMap);
        return NULL;
    }

    pMap->AppMap = pAppMap;
    pMap->Next = pGlobalData->pLabelMap;
    pGlobalData->pLabelMap = pMap;

    return pMap;
}


VOID RemoveLabel(PGLOBAL_DATA pGlobalData, PLABEL_MAP pMap)
{
    PLABEL_MAP pCurMap = pGlobalData->pLabelMap;

    if (pCurMap == pMap)
    {
        pGlobalData->pLabelMap = pGlobalData->pLabelMap->Next;
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

static
VOID
FreeLabelMap(PGLOBAL_DATA pGlobalData)
{
    PLABEL_MAP pCurMap;

    while (pGlobalData->pLabelMap)
    {
        pCurMap = pGlobalData->pLabelMap->Next;

        /* Prevent double freeing (for "FindLabel") */
        if (pGlobalData->pLabelMap->szName != pGlobalData->pLabelMap->szDesc)
        {
            free(pGlobalData->pLabelMap->szName);
        }

        free(pGlobalData->pLabelMap->szDesc);
        free(pGlobalData->pLabelMap->szIcon);

        HeapFree(GetProcessHeap(), 0, pGlobalData->pLabelMap);
        pGlobalData->pLabelMap = pCurMap;
    }
}

PAPP_MAP FindApp(PGLOBAL_DATA pGlobalData, WCHAR *szName)
{
    PAPP_MAP pMap = pGlobalData->pAppMap;

    while (pMap)
    {
        if (!wcscmp(pMap->szName, szName))
            return pMap;

        pMap = pMap->Next;

    }
    return NULL;
}

static
VOID
FreeAppMap(PGLOBAL_DATA pGlobalData)
{
    PAPP_MAP pCurMap;

    while (pGlobalData->pAppMap)
    {
        pCurMap = pGlobalData->pAppMap->Next;
        HeapFree(GetProcessHeap(), 0, pGlobalData->pAppMap);
        pGlobalData->pAppMap = pCurMap;
    }
}

PLABEL_CONTEXT FindLabelContext(PGLOBAL_DATA pGlobalData, PSOUND_SCHEME_CONTEXT pSoundScheme, WCHAR *AppName, WCHAR *LabelName)
{
    PLABEL_CONTEXT pLabelContext;

    pLabelContext = pSoundScheme->LabelContext;

    while (pLabelContext)
    {
        ASSERT(pLabelContext->AppMap);
        ASSERT(pLabelContext->LabelMap);

        if (!_wcsicmp(pLabelContext->AppMap->szName, AppName) && !_wcsicmp(pLabelContext->LabelMap->szName, LabelName))
        {
            return pLabelContext;
        }
        pLabelContext = pLabelContext->Next;
    }

    pLabelContext = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(LABEL_CONTEXT));
    if (!pLabelContext)
        return NULL;

    pLabelContext->AppMap = FindApp(pGlobalData, AppName);
    pLabelContext->LabelMap = FindLabel(pGlobalData, pLabelContext->AppMap, LabelName);
    ASSERT(pLabelContext->AppMap);
    ASSERT(pLabelContext->LabelMap);
    pLabelContext->szValue[0] = UNICODE_NULL;
    pLabelContext->Next = pSoundScheme->LabelContext;
    pSoundScheme->LabelContext = pLabelContext;

    return pLabelContext;
}


BOOL
LoadEventLabel(PGLOBAL_DATA pGlobalData, HKEY hKey, WCHAR *szSubKey)
{
    HKEY hSubKey;
    DWORD cbValue;
    WCHAR szDesc[MAX_PATH];
    WCHAR szData[MAX_PATH];
    PLABEL_MAP pMap;

    if (RegOpenKeyExW(hKey,
                      szSubKey,
                      0,
                      KEY_READ,
                      &hSubKey) != ERROR_SUCCESS)
    {
        return FALSE;
    }

    cbValue = sizeof(szDesc);
    if (RegQueryValueExW(hSubKey,
                         NULL,
                         NULL,
                         NULL,
                         (LPBYTE)szDesc,
                         &cbValue) != ERROR_SUCCESS)
    {
        RegCloseKey(hSubKey);
        return FALSE;
    }

    cbValue = sizeof(szData);
    if (RegQueryValueExW(hSubKey,
                         L"DispFileName",
                         NULL,
                         NULL,
                         (LPBYTE)szData,
                         &cbValue) != ERROR_SUCCESS)
    {
        RegCloseKey(hSubKey);
        return FALSE;
    }

    pMap = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(LABEL_MAP));
    if (!pMap)
    {
        return FALSE;
    }

    pMap->szName = _wcsdup(szSubKey);
    pMap->szDesc = _wcsdup(szDesc);
    pMap->szIcon = _wcsdup(szData);

    if (pGlobalData->pLabelMap)
    {
        pMap->Next = pGlobalData->pLabelMap;
        pGlobalData->pLabelMap = pMap;
    }
    else
    {
        pGlobalData->pLabelMap = pMap;
        pGlobalData->pLabelMap->Next = NULL;
    }
    return TRUE;
}


BOOL
LoadEventLabels(PGLOBAL_DATA pGlobalData)
{
    HKEY hSubKey;
    DWORD dwCurKey;
    WCHAR szName[MAX_PATH];
    DWORD dwName;
    DWORD dwResult;
    DWORD dwCount;
    if (RegOpenKeyExW(HKEY_CURRENT_USER,
                      L"AppEvents\\EventLabels",
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
        dwName = _countof(szName);
        dwResult = RegEnumKeyExW(hSubKey,
                                 dwCurKey,
                                 szName,
                                 &dwName,
                                 NULL,
                                 NULL,
                                 NULL,
                                 NULL);

        if (dwResult == ERROR_SUCCESS)
        {
            if (LoadEventLabel(pGlobalData, hSubKey, szName))
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
AddSoundProfile(HWND hwndDlg, HKEY hKey, WCHAR *szSubKey, BOOL SetDefault)
{
    HKEY hSubKey;
    WCHAR szValue[MAX_PATH];
    DWORD cbValue, dwResult;
    LRESULT lResult;
    PSOUND_SCHEME_CONTEXT pScheme;

    if (RegOpenKeyExW(hKey,
                      szSubKey,
                      0,
                      KEY_READ,
                      &hSubKey) != ERROR_SUCCESS)
    {
        return FALSE;
    }

    cbValue = sizeof(szValue);
    dwResult = RegQueryValueExW(hSubKey,
                                NULL,
                                NULL,
                                NULL,
                                (LPBYTE)szValue,
                                &cbValue);
    RegCloseKey(hSubKey);

    if (dwResult != ERROR_SUCCESS)
        return FALSE;

    /* Try to add the new profile */
    lResult = ComboBox_AddString(GetDlgItem(hwndDlg, IDC_SOUND_SCHEME), szValue);
    if (lResult == CB_ERR)
        return FALSE;

    /* Allocate the profile scheme buffer */
    pScheme = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(SOUND_SCHEME_CONTEXT));
    if (pScheme == NULL)
    {
        /* We failed to allocate the buffer, no need to keep a dangling string in the combobox */
        ComboBox_DeleteString(GetDlgItem(hwndDlg, IDC_SOUND_SCHEME), lResult);
        return FALSE;
    }

    StringCchCopyW(pScheme->szDesc, _countof(pScheme->szDesc), szValue);
    StringCchCopyW(pScheme->szName, _countof(pScheme->szName), szSubKey);

    /* Associate the value with the item in the combobox */
    ComboBox_SetItemData(GetDlgItem(hwndDlg, IDC_SOUND_SCHEME), lResult, pScheme);

    /* Optionally, select the profile */
    if (SetDefault)
    {
        ComboBox_SetCurSel(GetDlgItem(hwndDlg, IDC_SOUND_SCHEME), lResult);
    }

    return TRUE;
}


DWORD
EnumerateSoundProfiles(PGLOBAL_DATA pGlobalData, HWND hwndDlg, HKEY hKey)
{
    HKEY hSubKey;
    DWORD dwName, dwCurKey, dwResult, dwNumSchemes;
    DWORD cbDefault;
    WCHAR szName[MAX_PATH];

    cbDefault = sizeof(pGlobalData->szDefault);
    if (RegQueryValueExW(hKey,
                         NULL,
                         NULL,
                         NULL,
                         (LPBYTE)pGlobalData->szDefault,
                         &cbDefault) != ERROR_SUCCESS)
    {
        return FALSE;
    }

    if (RegOpenKeyExW(hKey,
                      L"Names",
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
        dwName = _countof(szName);
        dwResult = RegEnumKeyExW(hSubKey,
                                 dwCurKey,
                                 szName,
                                 &dwName,
                                 NULL,
                                 NULL,
                                 NULL,
                                 NULL);

        if (dwResult == ERROR_SUCCESS)
        {
            if (AddSoundProfile(hwndDlg, hSubKey, szName, (!_wcsicmp(szName, pGlobalData->szDefault))))
            {
                dwNumSchemes++;
            }
        }

        dwCurKey++;
    } while (dwResult == ERROR_SUCCESS);

    RegCloseKey(hSubKey);
    return dwNumSchemes;
}


PSOUND_SCHEME_CONTEXT FindSoundProfile(HWND hwndDlg, WCHAR *szName)
{
    LRESULT lCount, lIndex, lResult;
    PSOUND_SCHEME_CONTEXT pScheme;
    HWND hwndComboBox;

    hwndComboBox = GetDlgItem(hwndDlg, IDC_SOUND_SCHEME);
    lCount = ComboBox_GetCount(hwndComboBox);
    if (lCount == CB_ERR)
    {
        return NULL;
    }

    for (lIndex = 0; lIndex < lCount; lIndex++)
    {
        lResult = ComboBox_GetItemData(hwndComboBox, lIndex);
        if (lResult == CB_ERR)
        {
            continue;
        }

        pScheme = (PSOUND_SCHEME_CONTEXT)lResult;
        if (!_wcsicmp(pScheme->szName, szName))
        {
            return pScheme;
        }
    }
    return NULL;
}

static
VOID
FreeSoundProfiles(HWND hwndDlg)
{
    LRESULT lCount, lIndex, lResult;
    PSOUND_SCHEME_CONTEXT pScheme;
    PLABEL_CONTEXT pLabelContext;
    HWND hwndComboBox;

    hwndComboBox = GetDlgItem(hwndDlg, IDC_SOUND_SCHEME);
    lCount = ComboBox_GetCount(hwndComboBox);
    if (lCount == CB_ERR)
        return;

    for (lIndex = 0; lIndex < lCount; lIndex++)
    {
        lResult = ComboBox_GetItemData(hwndComboBox, lIndex);
        if (lResult == CB_ERR)
        {
            continue;
        }

        pScheme = (PSOUND_SCHEME_CONTEXT)lResult;

        while (pScheme->LabelContext)
        {
            pLabelContext = pScheme->LabelContext->Next;
            HeapFree(GetProcessHeap(), 0, pScheme->LabelContext);
            pScheme->LabelContext = pLabelContext;
        }

        HeapFree(GetProcessHeap(), 0, pScheme);
    }
}

BOOL
ImportSoundLabel(PGLOBAL_DATA pGlobalData, HWND hwndDlg, HKEY hKey, WCHAR *szProfile, WCHAR *szLabelName, WCHAR *szAppName, PAPP_MAP AppMap, PLABEL_MAP LabelMap)
{
    HKEY hSubKey;
    WCHAR szValue[MAX_PATH];
    WCHAR szBuffer[MAX_PATH];
    DWORD cbValue, cchLength;
    PSOUND_SCHEME_CONTEXT pScheme;
    PLABEL_CONTEXT pLabelContext;
    BOOL bCurrentProfile, bActiveProfile;

    bCurrentProfile = !_wcsicmp(szProfile, L".Current");
    bActiveProfile = !_wcsicmp(szProfile, pGlobalData->szDefault);

    if (RegOpenKeyExW(hKey,
                      szProfile,
                      0,
                      KEY_READ,
                      &hSubKey) != ERROR_SUCCESS)
    {
        return FALSE;
    }

    cbValue = sizeof(szValue);
    if (RegQueryValueExW(hSubKey,
                         NULL,
                         NULL,
                         NULL,
                         (LPBYTE)szValue,
                         &cbValue) != ERROR_SUCCESS)
    {
        return FALSE;
    }

    if (bCurrentProfile)
        pScheme = FindSoundProfile(hwndDlg, pGlobalData->szDefault);
    else
        pScheme = FindSoundProfile(hwndDlg, szProfile);

    if (!pScheme)
    {
        return FALSE;
    }
    pLabelContext = FindLabelContext(pGlobalData, pScheme, AppMap->szName, LabelMap->szName);

    cchLength = ExpandEnvironmentStringsW(szValue, szBuffer, _countof(szBuffer));
    if (cchLength == 0 || cchLength > _countof(szBuffer))
    {
        /* fixme */
        return FALSE;
    }

    if (bCurrentProfile)
        StringCchCopyW(pLabelContext->szValue, _countof(pLabelContext->szValue), szBuffer);
    else if (!bActiveProfile)
        StringCchCopyW(pLabelContext->szValue, _countof(pLabelContext->szValue), szBuffer);

    return TRUE;
}


DWORD
ImportSoundEntry(PGLOBAL_DATA pGlobalData, HWND hwndDlg, HKEY hKey, WCHAR *szLabelName, WCHAR *szAppName, PAPP_MAP pAppMap)
{
    HKEY hSubKey;
    DWORD dwNumProfiles;
    DWORD dwCurKey;
    DWORD dwResult;
    DWORD dwProfile;
    WCHAR szProfile[MAX_PATH];
    PLABEL_MAP pLabel;

    if (RegOpenKeyExW(hKey,
                      szLabelName,
                      0,
                      KEY_READ,
                      &hSubKey) != ERROR_SUCCESS)
    {
        return FALSE;
    }
    pLabel = FindLabel(pGlobalData, pAppMap, szLabelName);

    ASSERT(pLabel);
    RemoveLabel(pGlobalData, pLabel);

    pLabel->AppMap = pAppMap;
    pLabel->Next = pAppMap->LabelMap;
    pAppMap->LabelMap = pLabel;

    dwNumProfiles = 0;
    dwCurKey = 0;
    do
    {
        dwProfile = _countof(szProfile);
        dwResult = RegEnumKeyExW(hSubKey,
                                 dwCurKey,
                                 szProfile,
                                 &dwProfile,
                                 NULL,
                                 NULL,
                                 NULL,
                                 NULL);

        if (dwResult == ERROR_SUCCESS)
        {
            if (ImportSoundLabel(pGlobalData, hwndDlg, hSubKey, szProfile, szLabelName, szAppName, pAppMap, pLabel))
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
ImportAppProfile(PGLOBAL_DATA pGlobalData, HWND hwndDlg, HKEY hKey, WCHAR *szAppName)
{
    HKEY hSubKey;
    WCHAR szDefault[MAX_PATH];
    DWORD cbValue;
    DWORD dwCurKey;
    DWORD dwResult;
    DWORD dwNumEntry;
    DWORD dwName;
    WCHAR szName[MAX_PATH];
    WCHAR szIcon[MAX_PATH];
    PAPP_MAP AppMap;

    AppMap = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(APP_MAP));
    if (!AppMap)
        return 0;

    if (RegOpenKeyExW(hKey,
                      szAppName,
                      0,
                      KEY_READ,
                      &hSubKey) != ERROR_SUCCESS)
    {
        HeapFree(GetProcessHeap(), 0, AppMap);
        return 0;
    }

    cbValue = sizeof(szDefault);
    if (RegQueryValueExW(hSubKey,
                         NULL,
                         NULL,
                         NULL,
                         (LPBYTE)szDefault,
                         &cbValue) != ERROR_SUCCESS)
    {
        RegCloseKey(hSubKey);
        HeapFree(GetProcessHeap(), 0, AppMap);
        return 0;
    }

    cbValue = sizeof(szIcon);
    if (RegQueryValueExW(hSubKey,
                         L"DispFileName",
                         NULL,
                         NULL,
                         (LPBYTE)szIcon,
                         &cbValue) != ERROR_SUCCESS)
    {
        szIcon[0] = UNICODE_NULL;
    }

    /* initialize app map */
    StringCchCopyW(AppMap->szName, _countof(AppMap->szName), szAppName);
    StringCchCopyW(AppMap->szDesc, _countof(AppMap->szDesc), szDefault);
    StringCchCopyW(AppMap->szIcon, _countof(AppMap->szIcon), szIcon);

    AppMap->Next = pGlobalData->pAppMap;
    pGlobalData->pAppMap = AppMap;


    dwCurKey = 0;
    dwNumEntry = 0;
    do
    {
        dwName = _countof(szName);
        dwResult = RegEnumKeyExW(hSubKey,
                                 dwCurKey,
                                 szName,
                                 &dwName,
                                 NULL,
                                 NULL,
                                 NULL,
                                 NULL);
        if (dwResult == ERROR_SUCCESS)
        {
            if (ImportSoundEntry(pGlobalData, hwndDlg, hSubKey, szName, szAppName, AppMap))
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
ImportSoundProfiles(PGLOBAL_DATA pGlobalData, HWND hwndDlg, HKEY hKey)
{
    DWORD dwCurKey;
    DWORD dwResult;
    DWORD dwNumApps;
    WCHAR szName[MAX_PATH];
    HKEY hSubKey;

    if (RegOpenKeyExW(hKey,
                      L"Apps",
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
        dwResult = RegEnumKeyW(hSubKey,
                               dwCurKey,
                               szName,
                               _countof(szName));

        if (dwResult == ERROR_SUCCESS)
        {
            if (ImportAppProfile(pGlobalData, hwndDlg, hSubKey, szName))
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
LoadSoundProfiles(PGLOBAL_DATA pGlobalData, HWND hwndDlg)
{
    HKEY hSubKey;
    DWORD dwNumSchemes;

    if (RegOpenKeyExW(HKEY_CURRENT_USER,
                      L"AppEvents\\Schemes",
                      0,
                      KEY_READ,
                      &hSubKey) != ERROR_SUCCESS)
    {
        return FALSE;
    }

    dwNumSchemes = EnumerateSoundProfiles(pGlobalData, hwndDlg, hSubKey);


    if (dwNumSchemes)
    {
        ImportSoundProfiles(pGlobalData, hwndDlg, hSubKey);
    }

    RegCloseKey(hSubKey);
    return FALSE;
}


BOOL
LoadSoundFiles(HWND hwndDlg)
{
    WCHAR szList[256];
    WCHAR szPath[MAX_PATH];
    WCHAR *ptr;
    WIN32_FIND_DATAW FileData;
    HANDLE hFile;
    LRESULT lResult;
    UINT length;

    /* Add no sound listview item */
    if (LoadStringW(hApplet, IDS_NO_SOUND, szList, _countof(szList)))
    {
        szList[_countof(szList) - 1] = UNICODE_NULL;
        ComboBox_AddString(GetDlgItem(hwndDlg, IDC_SOUND_LIST), szList);
    }

    /* Load sound files */
    length = GetWindowsDirectoryW(szPath, _countof(szPath));
    if (length == 0 || length >= _countof(szPath) - 8)
    {
        return FALSE;
    }

    //PathCchAppend(szPath, _countof(szPath), L"media\\*");
    StringCchCatW(szPath, _countof(szPath), L"\\media\\*");

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
        lResult = SendDlgItemMessageW(hwndDlg, IDC_SOUND_LIST, CB_ADDSTRING, 0, (LPARAM)ptr);
        if (lResult != CB_ERR)
        {
            StringCchCopyW(szPath + (length + 7), _countof(szPath) - (length + 7), FileData.cFileName);
            SendDlgItemMessageW(hwndDlg, IDC_SOUND_LIST, CB_SETITEMDATA, (WPARAM)lResult, (LPARAM)_wcsdup(szPath));
        }
    } while (FindNextFileW(hFile, &FileData) != 0);

    FindClose(hFile);
    return TRUE;
}

static
VOID
FreeSoundFiles(HWND hwndDlg)
{
    LRESULT lCount, lIndex, lResult;
    WCHAR *pSoundPath;
    HWND hwndComboBox;

    hwndComboBox = GetDlgItem(hwndDlg, IDC_SOUND_LIST);
    lCount = ComboBox_GetCount(hwndComboBox);
    if (lCount == CB_ERR)
        return;

    for (lIndex = 0; lIndex < lCount; lIndex++)
    {
        lResult = ComboBox_GetItemData(hwndComboBox, lIndex);
        if (lResult == CB_ERR)
        {
            continue;
        }

        pSoundPath = (WCHAR*)lResult;
        
        free(pSoundPath);
    }
}

BOOL
ShowSoundScheme(PGLOBAL_DATA pGlobalData, HWND hwndDlg)
{
    LRESULT lIndex;
    PSOUND_SCHEME_CONTEXT pScheme;
    PAPP_MAP pAppMap;
    PLABEL_MAP pLabelMap;
    PLABEL_CONTEXT pLabelContext;
    HWND hDlgCtrl, hList;
    TVINSERTSTRUCTW tvItem;
    HTREEITEM hTreeItem;

    hDlgCtrl = GetDlgItem(hwndDlg, IDC_SOUND_SCHEME);
    hList = GetDlgItem(hwndDlg, IDC_SCHEME_LIST);

    if (pGlobalData->hSoundsImageList != NULL)
    {
        TreeView_SetImageList(hList, pGlobalData->hSoundsImageList, TVSIL_NORMAL);
    }

    lIndex = SendMessageW(hDlgCtrl, CB_GETCURSEL, 0, 0);
    if (lIndex == CB_ERR)
    {
        return FALSE;
    }

    lIndex = SendMessageW(hDlgCtrl, CB_GETITEMDATA, (WPARAM)lIndex, 0);
    if (lIndex == CB_ERR)
    {
        return FALSE;
    }
    pScheme = (PSOUND_SCHEME_CONTEXT)lIndex;

    StringCchCopyW(pGlobalData->szDefault, _countof(pGlobalData->szDefault), pScheme->szName);

    pAppMap = pGlobalData->pAppMap;
    while (pAppMap)
    {
        ZeroMemory(&tvItem, sizeof(tvItem));
        tvItem.hParent = TVI_ROOT;
        tvItem.hInsertAfter = TVI_FIRST;

        tvItem.item.mask = TVIF_STATE | TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
        tvItem.item.state = TVIS_EXPANDED;
        tvItem.item.stateMask = TVIS_EXPANDED;
        tvItem.item.pszText = pAppMap->szDesc;
        tvItem.item.iImage = IMAGE_SOUND_SECTION;
        tvItem.item.iSelectedImage = IMAGE_SOUND_SECTION;
        tvItem.item.lParam = (LPARAM)NULL;

        hTreeItem = TreeView_InsertItem(hList, &tvItem);

        pLabelMap = pAppMap->LabelMap;
        while (pLabelMap)
        {
            pLabelContext = FindLabelContext(pGlobalData, pScheme, pAppMap->szName, pLabelMap->szName);

            ZeroMemory(&tvItem, sizeof(tvItem));
            tvItem.hParent = hTreeItem;
            tvItem.hInsertAfter = TVI_SORT;

            tvItem.item.mask = TVIF_STATE | TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
            tvItem.item.state = TVIS_EXPANDED;
            tvItem.item.stateMask = TVIS_EXPANDED;
            tvItem.item.pszText = pLabelMap->szDesc;
            if (pLabelContext->szValue && wcslen(pLabelContext->szValue) > 0)
            {
                tvItem.item.iImage = IMAGE_SOUND_ASSIGNED;
                tvItem.item.iSelectedImage = IMAGE_SOUND_ASSIGNED;
            }
            else
            {
                tvItem.item.iImage = IMAGE_SOUND_NONE;
                tvItem.item.iSelectedImage = IMAGE_SOUND_NONE;
            }
            tvItem.item.lParam = (LPARAM)FindLabelContext(pGlobalData, pScheme, pAppMap->szName, pLabelMap->szName);

            TreeView_InsertItem(hList, &tvItem);

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
    DWORD dwType;
    LRESULT lIndex;
    PSOUND_SCHEME_CONTEXT pScheme;
    HWND hDlgCtrl;
    PLABEL_CONTEXT pLabelContext;
    WCHAR Buffer[100];

    hDlgCtrl = GetDlgItem(hwndDlg, IDC_SOUND_SCHEME);

    lIndex = SendMessageW(hDlgCtrl, CB_GETCURSEL, 0, 0);
    if (lIndex == CB_ERR)
    {
        return FALSE;
    }

    lIndex = SendMessageW(hDlgCtrl, CB_GETITEMDATA, (WPARAM)lIndex, 0);
    if (lIndex == CB_ERR)
    {
        return FALSE;
    }
    pScheme = (PSOUND_SCHEME_CONTEXT)lIndex;

    if (RegOpenKeyExW(HKEY_CURRENT_USER,
                      L"AppEvents\\Schemes",
                      0,
                      KEY_WRITE,
                      &hKey) != ERROR_SUCCESS)
    {
        return FALSE;
    }

    RegSetValueExW(hKey, NULL, 0, REG_SZ, (LPBYTE)pScheme->szName, (wcslen(pScheme->szName) + 1) * sizeof(WCHAR));
    RegCloseKey(hKey);

    if (RegOpenKeyExW(HKEY_CURRENT_USER,
                      L"AppEvents\\Schemes\\Apps",
                      0,
                      KEY_WRITE,
                      &hKey) != ERROR_SUCCESS)
    {
        return FALSE;
    }

    pLabelContext = pScheme->LabelContext;

    while (pLabelContext)
    {
        StringCchPrintfW(Buffer, _countof(Buffer), L"%s\\%s\\.Current", pLabelContext->AppMap->szName, pLabelContext->LabelMap->szName);

        if (RegOpenKeyExW(hKey, Buffer, 0, KEY_WRITE, &hSubKey) == ERROR_SUCCESS)
        {
            dwType = (wcschr(pLabelContext->szValue, L'%') ? REG_EXPAND_SZ : REG_SZ);
            RegSetValueExW(hSubKey, NULL, 0, dwType, (LPBYTE)pLabelContext->szValue, (wcslen(pLabelContext->szValue) + 1) * sizeof(WCHAR));
            RegCloseKey(hSubKey);
        }

        pLabelContext = pLabelContext->Next;
    }
    RegCloseKey(hKey);

    SetWindowLongPtrW(hwndDlg, DWLP_MSGRESULT, (LONG_PTR)PSNRET_NOERROR);
    return TRUE;
}


HIMAGELIST
InitImageList(UINT StartResource,
              UINT EndResource,
              UINT Width,
              UINT Height,
              ULONG type)
{
    HANDLE hImage;
    HIMAGELIST himl;
    UINT i;
    INT ret;

    /* Create the toolbar icon image list */
    himl = ImageList_Create(Width,
                            Height,
                            ILC_MASK | ILC_COLOR32,
                            EndResource - StartResource,
                            0);
    if (himl == NULL)
        return NULL;

    ret = 0;
    for (i = StartResource; i <= EndResource && ret != -1; i++)
    {
        hImage = LoadImageW(hApplet,
                            MAKEINTRESOURCEW(i),
                            type,
                            Width,
                            Height,
                            LR_LOADTRANSPARENT);
        if (hImage == NULL)
        {
            ret = -1;
            break;
        }

        if (type == IMAGE_BITMAP)
        {
            ret = ImageList_AddMasked(himl,
                                      hImage,
                                      RGB(255, 0, 128));
        }
        else if (type == IMAGE_ICON)
        {
            ret = ImageList_AddIcon(himl,
                                    hImage);
        }

        DeleteObject(hImage);
    }

    if (ret == -1)
    {
        ImageList_Destroy(himl);
        himl = NULL;
    }

    return himl;
}


/* Sounds property page dialog callback */
INT_PTR
CALLBACK
SoundsDlgProc(HWND hwndDlg,
              UINT uMsg,
              WPARAM wParam,
              LPARAM lParam)
{
    PGLOBAL_DATA pGlobalData;

    OPENFILENAMEW ofn;
    WCHAR filename[MAX_PATH];
    WCHAR szFilter[256], szTitle[256];
    LPWSTR pFileName;
    LRESULT lResult;

    pGlobalData = (PGLOBAL_DATA)GetWindowLongPtrW(hwndDlg, DWLP_USER);

    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            pGlobalData = (PGLOBAL_DATA)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(GLOBAL_DATA));
            SetWindowLongPtrW(hwndDlg, DWLP_USER, (LONG_PTR)pGlobalData);

            pGlobalData->NumWavOut = waveOutGetNumDevs();

            SendMessageW(GetDlgItem(hwndDlg, IDC_PLAY_SOUND),
                         BM_SETIMAGE, (WPARAM)IMAGE_ICON,
                         (LPARAM)(HANDLE)LoadIconW(hApplet, MAKEINTRESOURCEW(IDI_PLAY_ICON)));

            pGlobalData->hSoundsImageList = InitImageList(IDI_SOUND_SECTION,
                                                          IDI_SOUND_ASSIGNED,
                                                          GetSystemMetrics(SM_CXSMICON),
                                                          GetSystemMetrics(SM_CXSMICON),
                                                          IMAGE_ICON);

            LoadEventLabels(pGlobalData);
            LoadSoundProfiles(pGlobalData, hwndDlg);
            LoadSoundFiles(hwndDlg);
            ShowSoundScheme(pGlobalData, hwndDlg);

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
                    ZeroMemory(&ofn, sizeof(ofn));
                    ofn.lStructSize = sizeof(ofn);
                    ofn.hwndOwner = hwndDlg;
                    ofn.lpstrFile = filename;
                    ofn.lpstrFile[0] = UNICODE_NULL;
                    ofn.nMaxFile = _countof(filename);
                    LoadStringW(hApplet, IDS_WAVE_FILES_FILTER, szFilter, _countof(szFilter));
                    ofn.lpstrFilter = MakeFilter(szFilter);
                    ofn.nFilterIndex = 0;
                    LoadStringW(hApplet, IDS_BROWSE_FOR_SOUND, szTitle, _countof(szTitle));
                    ofn.lpstrTitle = szTitle;
                    ofn.lpstrInitialDir = L"%SystemRoot%\\Media";
                    ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;

                    if (GetOpenFileNameW(&ofn))
                    {
                        // FIXME search if list already contains that sound

                        // extract file name
                        pFileName = wcsrchr(filename, L'\\');
                        ASSERT(pFileName != NULL);
                        pFileName++;

                        // add to list
                        lResult = SendDlgItemMessageW(hwndDlg, IDC_SOUND_LIST, CB_ADDSTRING, 0, (LPARAM)pFileName);
                        if (lResult != CB_ERR)
                        {
                            // add path and select item
                            SendDlgItemMessageW(hwndDlg, IDC_SOUND_LIST, CB_SETITEMDATA, (WPARAM)lResult, (LPARAM)_wcsdup(filename));
                            SendDlgItemMessageW(hwndDlg, IDC_SOUND_LIST, CB_SETCURSEL, (WPARAM)lResult, 0);
                        }
                    }
                    break;
                }
                case IDC_PLAY_SOUND:
                {
                    LRESULT lIndex;
                    lIndex = ComboBox_GetCurSel(GetDlgItem(hwndDlg, IDC_SOUND_LIST));
                    if (lIndex == CB_ERR)
                    {
                        break;
                    }

                    lIndex = ComboBox_GetItemData(GetDlgItem(hwndDlg, IDC_SOUND_LIST), lIndex);
                    if (lIndex != CB_ERR)
                    {
                        PlaySoundW((WCHAR*)lIndex, NULL, SND_FILENAME);
                    }
                    break;
                }
                case IDC_SOUND_SCHEME:
                {
                    if (HIWORD(wParam) == CBN_SELENDOK)
                    {
                        TreeView_DeleteAllItems(GetDlgItem(hwndDlg, IDC_SCHEME_LIST));
                        ShowSoundScheme(pGlobalData, hwndDlg);
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
                        HTREEITEM hItem;
                        TVITEMW item;
                        LRESULT lIndex;

                        hItem = TreeView_GetSelection(GetDlgItem(hwndDlg, IDC_SCHEME_LIST));
                        if (hItem == NULL)
                        {
                            break;
                        }

                        lIndex = ComboBox_GetCurSel(GetDlgItem(hwndDlg, IDC_SOUND_LIST));
                        if (lIndex == CB_ERR)
                        {
                            break;
                        }

                        ZeroMemory(&item, sizeof(item));
                        item.mask = TVIF_PARAM;
                        item.hItem = hItem;
                        if (TreeView_GetItem(GetDlgItem(hwndDlg, IDC_SCHEME_LIST), &item))
                        {
                            LRESULT lResult;
                            pLabelContext = (PLABEL_CONTEXT)item.lParam;

                            lResult = ComboBox_GetItemData(GetDlgItem(hwndDlg, IDC_SOUND_LIST), lIndex);
                            if (lResult == CB_ERR || lResult == 0)
                            {
                                if (lIndex != pLabelContext->szValue[0])
                                {
                                    /* Update the tree view item image */
                                    item.mask = TVIF_IMAGE | TVIF_SELECTEDIMAGE;
                                    item.iImage = IMAGE_SOUND_NONE;
                                    item.iSelectedImage = IMAGE_SOUND_NONE;
                                    TreeView_SetItem(GetDlgItem(hwndDlg, IDC_SCHEME_LIST), &item);

                                    PropSheet_Changed(GetParent(hwndDlg), hwndDlg);

                                    EnableWindow(GetDlgItem(hwndDlg, IDC_PLAY_SOUND), FALSE);
                                }

                                pLabelContext->szValue[0] = UNICODE_NULL;

                                break;
                            }

                            if (_wcsicmp(pLabelContext->szValue, (WCHAR*)lResult) || (lIndex != pLabelContext->szValue[0]))
                            {
                                /* Update the tree view item image */
                                item.mask = TVIF_IMAGE | TVIF_SELECTEDIMAGE;
                                item.iImage = IMAGE_SOUND_ASSIGNED;
                                item.iSelectedImage = IMAGE_SOUND_ASSIGNED;
                                TreeView_SetItem(GetDlgItem(hwndDlg, IDC_SCHEME_LIST), &item);

                                PropSheet_Changed(GetParent(hwndDlg), hwndDlg);

                                ///
                                /// Should store in current member
                                ///
                                StringCchCopyW(pLabelContext->szValue, _countof(pLabelContext->szValue), (WCHAR*)lResult);
                            }

                            if (wcslen((WCHAR*)lResult) && lIndex != 0 && pGlobalData->NumWavOut != 0)
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
        case WM_DESTROY:
        {
            FreeSoundFiles(hwndDlg);
            FreeSoundProfiles(hwndDlg);
            FreeAppMap(pGlobalData);
            FreeLabelMap(pGlobalData);
            if (pGlobalData->hSoundsImageList)
                ImageList_Destroy(pGlobalData->hSoundsImageList);
            HeapFree(GetProcessHeap(), 0, pGlobalData);
            break;
        }
        case WM_NOTIFY:
        {
            PLABEL_CONTEXT pLabelContext;
            WCHAR *ptr;

            LPNMHDR lpnm = (LPNMHDR)lParam;

            switch (lpnm->code)
            {
                case PSN_APPLY:
                {
                    ApplyChanges(hwndDlg);
                    break;
                }
                case TVN_SELCHANGED:
                {
                    LPNMTREEVIEWW nm = (LPNMTREEVIEWW)lParam;
                    LRESULT lCount, lIndex, lResult;

                    pLabelContext = (PLABEL_CONTEXT)nm->itemNew.lParam;
                    if (pLabelContext == NULL)
                    {
                        EnableWindow(GetDlgItem(hwndDlg, IDC_SOUND_LIST), FALSE);
                        EnableWindow(GetDlgItem(hwndDlg, IDC_TEXT_SOUND), FALSE);
                        EnableWindow(GetDlgItem(hwndDlg, IDC_BROWSE_SOUND), FALSE);
                        EnableWindow(GetDlgItem(hwndDlg, IDC_PLAY_SOUND), FALSE);
                        return FALSE;
                    }

                    EnableWindow(GetDlgItem(hwndDlg, IDC_SOUND_LIST), TRUE);
                    EnableWindow(GetDlgItem(hwndDlg, IDC_TEXT_SOUND), TRUE);
                    EnableWindow(GetDlgItem(hwndDlg, IDC_BROWSE_SOUND), TRUE);

                    if (wcslen(pLabelContext->szValue) == 0)
                    {
                        lIndex = ComboBox_SetCurSel(GetDlgItem(hwndDlg, IDC_SOUND_LIST), 0);
                        EnableWindow(GetDlgItem(hwndDlg, IDC_PLAY_SOUND), FALSE);
                        break;
                    }

                    if (pGlobalData->NumWavOut != 0)
                        EnableWindow(GetDlgItem(hwndDlg, IDC_PLAY_SOUND), TRUE);

                    lCount = ComboBox_GetCount(GetDlgItem(hwndDlg, IDC_SOUND_LIST));
                    for (lIndex = 0; lIndex < lCount; lIndex++)
                    {
                        lResult = ComboBox_GetItemData(GetDlgItem(hwndDlg, IDC_SOUND_LIST), lIndex);
                        if (lResult == CB_ERR || lResult == 0)
                            continue;

                        if (!wcscmp((WCHAR*)lResult, pLabelContext->szValue))
                        {
                            ComboBox_SetCurSel(GetDlgItem(hwndDlg, IDC_SOUND_LIST), lIndex);
                            return FALSE;
                        }
                    }

                    ptr = wcsrchr(pLabelContext->szValue, L'\\');
                    if (ptr)
                    {
                        ptr++;
                    }
                    else
                    {
                        ptr = pLabelContext->szValue;
                    }

                    lIndex = ComboBox_AddString(GetDlgItem(hwndDlg, IDC_SOUND_LIST), ptr);
                    if (lIndex != CB_ERR)
                    {
                        ComboBox_SetItemData(GetDlgItem(hwndDlg, IDC_SOUND_LIST), lIndex, _wcsdup(pLabelContext->szValue));
                        ComboBox_SetCurSel(GetDlgItem(hwndDlg, IDC_SOUND_LIST), lIndex);
                    }
                    break;
                }
            }
        }
        break;
    }

    return FALSE;
}
