/*
 *    Open With  Context Menu extension
 *
 * Copyright 2007 Johannes Anderwald <johannes.anderwald@reactos.org>
 * Copyright 2009 Andrew Hill
 * Copyright 2012 Rafal Harabien
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "precomp.h"

WINE_DEFAULT_DEBUG_CHANNEL(shell);

//
// [HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\policies\system]
// "NoInternetOpenWith"=dword:00000001
//

EXTERN_C BOOL PathIsExeW(LPCWSTR lpszPath);

class COpenWithList
{
    public:
        struct SApp
        {
            WCHAR wszFilename[MAX_PATH];
            WCHAR wszCmd[MAX_PATH];
            //WCHAR wszManufacturer[256];
            WCHAR wszName[256];
            BOOL bHidden;
            BOOL bRecommended;
            BOOL bMRUList;
            HICON hIcon;
        };

        COpenWithList();
        ~COpenWithList();

        BOOL Load();
        SApp *Add(LPCWSTR pwszPath);
        static BOOL SaveApp(SApp *pApp);
        SApp *Find(LPCWSTR pwszFilename);
        static LPCWSTR GetName(SApp *pApp);
        static HICON GetIcon(SApp *pApp);
        static BOOL Execute(SApp *pApp, LPCWSTR pwszFilePath);
        static BOOL IsHidden(SApp *pApp);
        inline BOOL IsNoOpen(VOID) { return m_bNoOpen; }
        BOOL LoadRecommended(LPCWSTR pwszFilePath);
        BOOL SetDefaultHandler(SApp *pApp, LPCWSTR pwszFilename);

        inline SApp *GetList() { return m_pApp; }
        inline UINT GetCount() { return m_cApp; }
        inline UINT GetRecommendedCount() { return m_cRecommended; }

    private:
        typedef struct _LANGANDCODEPAGE
        {
            WORD lang;
            WORD code;
        } LANGANDCODEPAGE, *LPLANGANDCODEPAGE;

        SApp *m_pApp;
        UINT m_cApp, m_cRecommended;
        BOOL m_bNoOpen;

        SApp *AddInternal(LPCWSTR pwszFilename);
        static BOOL LoadInfo(SApp *pApp);
        static VOID GetPathFromCmd(LPWSTR pwszAppPath, LPCWSTR pwszCmd);
        BOOL LoadProgIdList(HKEY hKey, LPCWSTR pwszExt);
        static HANDLE OpenMRUList(HKEY hKey);
        BOOL LoadMRUList(HKEY hKey);
        BOOL LoadAppList(HKEY hKey);
        VOID LoadFromProgIdKey(HKEY hKey, LPCWSTR pwszExt);
        VOID LoadRecommendedFromHKCR(LPCWSTR pwszExt);
        VOID LoadRecommendedFromHKCU(LPCWSTR pwszExt);
        static BOOL AddAppToMRUList(SApp *pApp, LPCWSTR pwszFilename);

        inline VOID SetRecommended(SApp *pApp)
        {
            if (!pApp->bRecommended)
                ++m_cRecommended;
            pApp->bRecommended = TRUE;
        }
};

COpenWithList::COpenWithList():
    m_pApp(NULL), m_cApp(0), m_cRecommended(0), m_bNoOpen(FALSE) {}

COpenWithList::~COpenWithList()
{
    for (UINT i = 0; i < m_cApp; ++i)
        if (m_pApp[i].hIcon)
            DestroyIcon(m_pApp[i].hIcon);

    HeapFree(GetProcessHeap(), 0, m_pApp);
}

BOOL COpenWithList::Load()
{
    HKEY hKey;
    WCHAR wszName[256], wszBuf[100];;
    DWORD i = 0, cchName, dwSize;
    SApp *pApp;

    if (RegOpenKeyEx(HKEY_CLASSES_ROOT, L"Applications", 0, KEY_READ, &hKey) != ERROR_SUCCESS)
    {
        ERR("RegOpenKeyEx HKCR\\Applications failed!\n");
        return FALSE;
    }

    while (TRUE)
    {
        cchName = _countof(wszName);
        if (RegEnumKeyEx(hKey, i++, wszName, &cchName, NULL, NULL, NULL, NULL) != ERROR_SUCCESS)
            break;

        pApp = AddInternal(wszName);

        if (pApp)
        {
            StringCbPrintfW(wszBuf, sizeof(wszBuf), L"%s\\shell\\open\\command", wszName);
            dwSize = sizeof(pApp->wszCmd);
            if (RegGetValueW(hKey, wszBuf, L"", RRF_RT_REG_SZ, NULL, pApp->wszCmd, &dwSize) != ERROR_SUCCESS)
            {
                ERR("Failed to add app %ls\n", wszName);
                pApp->bHidden = TRUE;
            }
            else
                TRACE("App added %ls\n", pApp->wszCmd);
        }
        else
            ERR("AddInternal failed\n");
    }

    RegCloseKey(hKey);
    return TRUE;
}

COpenWithList::SApp *COpenWithList::Add(LPCWSTR pwszPath)
{
    SApp *pApp = AddInternal(PathFindFileNameW(pwszPath));

    if (pApp)
    {
        StringCbPrintfW(pApp->wszCmd, sizeof(pApp->wszCmd), L"\"%s\" %%1", pwszPath);
        SaveApp(pApp);
    }

    return pApp;
}

BOOL COpenWithList::SaveApp(SApp *pApp)
{
    WCHAR wszBuf[256];
    HKEY hKey;

    StringCbPrintfW(wszBuf, sizeof(wszBuf), L"Applications\\%s\\shell\\open\\command", pApp->wszFilename);
    if (RegCreateKeyEx(HKEY_CLASSES_ROOT, wszBuf, 0, NULL, 0, KEY_WRITE, NULL, &hKey, NULL) != ERROR_SUCCESS)
    {
        ERR("RegOpenKeyEx failed\n");
        return FALSE;
    }

    if (RegSetValueEx(hKey, L"", 0, REG_SZ, (PBYTE)pApp->wszCmd, (wcslen(pApp->wszCmd)+1)*sizeof(WCHAR)) != ERROR_SUCCESS)
        ERR("Cannot add app to registry\n");

    RegCloseKey(hKey);
    return TRUE;
}

COpenWithList::SApp *COpenWithList::Find(LPCWSTR pwszFilename)
{
    for (UINT i = 0; i < m_cApp; ++i)
        if (wcsicmp(m_pApp[i].wszFilename, pwszFilename) == 0)
            return &m_pApp[i];
    return NULL;
}

LPCWSTR COpenWithList::GetName(SApp *pApp)
{
    if (!pApp->wszName[0])
    {
        if (!LoadInfo(pApp))
        {
            WARN("Failed to load %ls info\n", pApp->wszFilename);
            StringCbCopyW(pApp->wszName, sizeof(pApp->wszName), pApp->wszFilename);
        }
    }

    TRACE("%ls name: %ls\n", pApp->wszFilename, pApp->wszName);
    return pApp->wszName;
}

HICON COpenWithList::GetIcon(SApp *pApp)
{
    if (!pApp->hIcon)
    {
        WCHAR wszPath[MAX_PATH];

        GetPathFromCmd(wszPath, pApp->wszCmd);
        ExtractIconExW(wszPath, 0, NULL, &pApp->hIcon, 1);
    }

    TRACE("%ls icon: %p\n", pApp->wszFilename, pApp->hIcon);

    return pApp->hIcon;
}

BOOL COpenWithList::Execute(COpenWithList::SApp *pApp, LPCWSTR pwszFilePath)
{
    WCHAR wszBuf[256];
    HKEY hKey;

    /* Add app to registry if it wasnt there before */
    SaveApp(pApp);
    if (!pApp->bMRUList)
        AddAppToMRUList(pApp, pwszFilePath);

    /* Get a handle to the reg key */
    StringCbPrintfW(wszBuf, sizeof(wszBuf), L"Applications\\%s", pApp->wszFilename);
    if (RegCreateKeyEx(HKEY_CLASSES_ROOT, wszBuf, 0, NULL, 0, KEY_WRITE, NULL, &hKey, NULL) != ERROR_SUCCESS)
    {
        ERR("RegOpenKeyEx failed\n");
        return FALSE;
    }

    /* Let ShellExecuteExW do the work */
    SHELLEXECUTEINFOW sei = {sizeof(SHELLEXECUTEINFOW), SEE_MASK_CLASSKEY};
    sei.nShow = SW_SHOWNORMAL;
    sei.hkeyClass = hKey;
    sei.lpFile = pwszFilePath;

    ShellExecuteExW(&sei);

    return TRUE;
}

BOOL COpenWithList::IsHidden(SApp *pApp)
{
    WCHAR wszBuf[100];
    DWORD dwSize = 0;

    if (pApp->bHidden)
        return pApp->bHidden;

    if (FAILED(StringCbPrintfW(wszBuf, sizeof(wszBuf), L"Applications\\%s", pApp->wszFilename)))
    {
        ERR("insufficient buffer\n");
        return FALSE;
    }

    if (RegGetValueW(HKEY_CLASSES_ROOT, wszBuf, L"NoOpenWith", RRF_RT_REG_SZ, NULL, NULL, &dwSize) != ERROR_SUCCESS)
        return FALSE;

    pApp->bHidden = TRUE;
    return TRUE;
}

COpenWithList::SApp *COpenWithList::AddInternal(LPCWSTR pwszFilename)
{
    /* Check for duplicate */
    SApp *pApp = Find(pwszFilename);
    if (pApp)
        return pApp;

    /* Create new item */
    if (!m_pApp)
        m_pApp = static_cast<SApp *>(HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(m_pApp[0])));
    else
        m_pApp = static_cast<SApp *>(HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, m_pApp, (m_cApp + 1)*sizeof(m_pApp[0])));
    if (!m_pApp)
    {
        ERR("Allocation failed\n");
        return NULL;
    }

    pApp = &m_pApp[m_cApp++];
    wcscpy(pApp->wszFilename, pwszFilename);
    return pApp;
}

BOOL COpenWithList::LoadInfo(COpenWithList::SApp *pApp)
{
    UINT cbSize, cchLen;
    LPVOID pBuf;
    WORD wLang = 0, wCode = 0;
    LPLANGANDCODEPAGE lpLangCode;
    WCHAR wszBuf[100];
    WCHAR *pResult;
    WCHAR wszPath[MAX_PATH];

    GetPathFromCmd(wszPath, pApp->wszCmd);
    TRACE("LoadInfo %ls\n", wszPath);

    /* query version info size */
    cbSize = GetFileVersionInfoSizeW(wszPath, NULL);
    if (!cbSize)
    {
        ERR("GetFileVersionInfoSizeW %ls failed: %lu\n", wszPath, GetLastError());
        return FALSE;
    }

    /* allocate buffer */
    pBuf = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, cbSize + 200);
    if (!pBuf)
    {
        ERR("HeapAlloc failed\n");
        return FALSE;
    }

    /* query version info */
    if (!GetFileVersionInfoW(wszPath, 0, cbSize, pBuf))
    {
        ERR("GetFileVersionInfoW %ls failed: %lu\n", wszPath, GetLastError());
        HeapFree(GetProcessHeap(), 0, pBuf);
        return FALSE;
    }

    /* query lang code */
    if (VerQueryValueW(pBuf, L"VarFileInfo\\Translation", (LPVOID*)&lpLangCode, &cbSize))
    {
                /* FIXME: find language from current locale / if not available,
                 * default to english
                 * for now default to first available language
                 */
                wLang = lpLangCode->lang;
                wCode = lpLangCode->code;
    }

    /* Query name */
    swprintf(wszBuf, L"\\StringFileInfo\\%04x%04x\\FileDescription", wLang, wCode);
    if (VerQueryValueW(pBuf, wszBuf, (LPVOID *)&pResult, &cchLen))
        StringCchCopyNW(pApp->wszName, _countof(pApp->wszName), pResult, cchLen);
    else
        ERR("Cannot get app name\n");

    /* Query manufacturer */
    /*swprintf(wszBuf, L"\\StringFileInfo\\%04x%04x\\CompanyName", wLang, wCode);

    if (VerQueryValueW(pBuf, wszBuf, (LPVOID *)&pResult, &cchLen))
        StringCchCopyNW(pApp->wszManufacturer, _countof(pApp->wszManufacturer), pResult, cchLen);*/
    HeapFree(GetProcessHeap(), 0, pBuf);
    return TRUE;
}

VOID COpenWithList::GetPathFromCmd(LPWSTR pwszAppPath, LPCWSTR pwszCmd)
{
    WCHAR wszBuf[MAX_PATH], *pwszDest = wszBuf;

    /* Remove arguments */
    if (pwszCmd[0] == '"')
    {
        for(LPCWSTR pwszSrc = pwszCmd + 1; *pwszSrc && *pwszSrc != '"'; ++pwszSrc)
            *(pwszDest++) = *pwszSrc;
    }
    else
    {
        for(LPCWSTR pwszSrc = pwszCmd; *pwszSrc && *pwszSrc != ' '; ++pwszSrc)
            *(pwszDest++) = *pwszSrc;
    }

    *pwszDest = 0;

    /* Expand evn vers and optionally search for path */
    ExpandEnvironmentStrings(wszBuf, pwszAppPath, MAX_PATH);
    if (!PathFileExists(pwszAppPath))
        SearchPath(NULL, pwszAppPath, NULL, MAX_PATH, pwszAppPath, NULL);
}

BOOL COpenWithList::LoadRecommended(LPCWSTR pwszFilePath)
{
    LPCWSTR pwszExt;

    pwszExt = PathFindExtensionW(pwszFilePath);
    if (!pwszExt[0])
        return FALSE;

    /* load programs directly associated from HKCU */
    LoadRecommendedFromHKCU(pwszExt);

    /* load programs associated from HKCR\Extension */
    LoadRecommendedFromHKCR(pwszExt);

    return TRUE;
}

BOOL COpenWithList::LoadProgIdList(HKEY hKey, LPCWSTR pwszExt)
{
    HKEY hSubkey, hSubkey2;
    WCHAR wszProgId[256];
    DWORD i = 0, cchProgId;

    if (RegOpenKeyExW(hKey, L"OpenWithProgIDs", 0, KEY_READ, &hSubkey) != ERROR_SUCCESS)
        return FALSE;

    while (TRUE)
    {
        /* Enumerate values - value name is ProgId */
        cchProgId = _countof(wszProgId);
        if (RegEnumValue(hSubkey, i++, wszProgId, &cchProgId, NULL, NULL, NULL, NULL) != ERROR_SUCCESS)
            break;

        /* If ProgId exists load it */
        if (RegOpenKeyExW(HKEY_CLASSES_ROOT, wszProgId, 0, KEY_READ, &hSubkey2) == ERROR_SUCCESS)
        {
            LoadFromProgIdKey(hSubkey2, pwszExt);
            RegCloseKey(hSubkey2);
        }
    }

    RegCloseKey(hSubkey);
    return TRUE;
}

HANDLE COpenWithList::OpenMRUList(HKEY hKey)
{
    CREATEMRULISTW Info;

    /* Initialize mru list info */
    Info.cbSize = sizeof(Info);
    Info.nMaxItems = 32;
    Info.dwFlags = MRU_STRING;
    Info.hKey = hKey;
    Info.lpszSubKey = L"OpenWithList";
    Info.lpfnCompare = NULL;

    return CreateMRUListW(&Info);
}

BOOL COpenWithList::LoadMRUList(HKEY hKey)
{
    HANDLE hList;
    int nItem, nCount, nResult;
    WCHAR wszAppFilename[MAX_PATH];

    /* Open MRU list */
    hList = OpenMRUList(hKey);
    if (!hList)
    {
        TRACE("OpenMRUList failed\n");
        return FALSE;
    }

    /* Get list count */
    nCount = EnumMRUListW(hList, -1, NULL, 0);

    for(nItem = 0; nItem < nCount; nItem++)
    {
        nResult = EnumMRUListW(hList, nItem, wszAppFilename, _countof(wszAppFilename));
        if (nResult <= 0)
            continue;

        /* Insert item */
        SApp *pApp = Find(wszAppFilename);

        TRACE("Recommended app %ls: %p\n", wszAppFilename, pApp);
        if (pApp)
        {
            pApp->bMRUList = TRUE;
            SetRecommended(pApp);
        }
    }

    /* Free the MRU list */
    FreeMRUList(hList);
    return TRUE;
}

BOOL COpenWithList::LoadAppList(HKEY hKey)
{
    WCHAR wszAppFilename[MAX_PATH];
    HKEY hSubkey;
    DWORD i = 0, cchAppFilename;

    if (RegOpenKeyExW(hKey, L"OpenWithList", 0, KEY_READ, &hSubkey) != ERROR_SUCCESS)
        return FALSE;

    while (TRUE)
    {
        /* Enum registry keys - each of them is app name */
        cchAppFilename = _countof(wszAppFilename);
        if (RegEnumKeyExW(hSubkey, i++, wszAppFilename, &cchAppFilename, NULL, NULL, NULL, NULL) != ERROR_SUCCESS)
            break;

        /* Set application as recommended */
        SApp *pApp = Find(wszAppFilename);

        TRACE("Recommended app %ls: %p\n", wszAppFilename, pApp);
        if (pApp)
            SetRecommended(pApp);
    }

    RegCloseKey(hSubkey);
    return TRUE;
}

VOID COpenWithList::LoadFromProgIdKey(HKEY hKey, LPCWSTR pwszExt)
{
    WCHAR wszCmd[MAX_PATH], wszPath[MAX_PATH];
    DWORD dwSize = 0;

    /* Check if NoOpen value exists */
    if (RegGetValueW(hKey, NULL, L"NoOpen", RRF_RT_REG_SZ, NULL, NULL, &dwSize) == ERROR_SUCCESS)
    {
        /* Display warning dialog */
        m_bNoOpen = TRUE;
    }

    /* Check if there is a directly available execute key */
    dwSize = sizeof(wszCmd);
    if (RegGetValueW(hKey, L"shell\\open\\command", NULL, RRF_RT_REG_SZ, NULL, (PVOID)wszCmd, &dwSize) == ERROR_SUCCESS)
    {
        /* Erase extra arguments */
        GetPathFromCmd(wszPath, wszCmd);

        /* Add application */
        SApp *pApp = AddInternal(PathFindFileNameW(wszPath));
        TRACE("Add app %ls: %p\n", wszPath, pApp);

        if (pApp)
        {
            StringCbCopyW(pApp->wszCmd, sizeof(pApp->wszCmd), wszCmd);
            SetRecommended(pApp);
            if (!pApp->bMRUList)
                AddAppToMRUList(pApp, pwszExt);
        }
    }
}

VOID COpenWithList::LoadRecommendedFromHKCR(LPCWSTR pwszExt)
{
    HKEY hKey, hSubkey;
    WCHAR wszBuf[MAX_PATH], wszBuf2[MAX_PATH];
    DWORD dwSize;

    /* Check if extension exists */
    if (RegOpenKeyExW(HKEY_CLASSES_ROOT, pwszExt, 0, KEY_READ, &hKey) != ERROR_SUCCESS)
    {
        /* Load items from SystemFileAssociations\Ext key */
        StringCbPrintfW(wszBuf, sizeof(wszBuf), L"SystemFileAssociations\\%s", pwszExt);
        if (RegOpenKeyExW(HKEY_CLASSES_ROOT, wszBuf, 0, KEY_READ, &hKey) != ERROR_SUCCESS)
            return;
    }

    /* Load programs referenced from HKCR\ProgId */
    dwSize = sizeof(wszBuf);
    if (RegGetValueW(hKey, NULL, L"", RRF_RT_REG_SZ, NULL, wszBuf, &dwSize) == ERROR_SUCCESS &&
        RegOpenKeyExW(HKEY_CLASSES_ROOT, wszBuf, 0, KEY_READ, &hSubkey) == ERROR_SUCCESS)
    {
        LoadFromProgIdKey(hSubkey, pwszExt);
        RegCloseKey(hSubkey);
    }
    else
        LoadFromProgIdKey(hKey, pwszExt);

    /* Load items from HKCR\Ext\OpenWithList */
    LoadAppList(hKey);

    /* Load items from HKCR\Ext\OpenWithProgIDs */
    if (RegOpenKeyExW(hKey, L"OpenWithProgIDs", 0, KEY_READ, &hSubkey) == ERROR_SUCCESS)
    {
        LoadProgIdList(hSubkey, pwszExt);
        RegCloseKey(hSubkey);
    }

    /* Load additional items from referenced PerceivedType */
    dwSize = sizeof(wszBuf);
    if (RegGetValueW(hKey, NULL, L"PerceivedType", RRF_RT_REG_SZ, NULL, wszBuf, &dwSize) == ERROR_SUCCESS)
    {
        StringCbPrintfW(wszBuf2, sizeof(wszBuf2), L"SystemFileAssociations\\%s", wszBuf);
        if (RegOpenKeyExW(HKEY_CLASSES_ROOT, wszBuf2, 0, KEY_READ | KEY_WRITE, &hSubkey) == ERROR_SUCCESS)
        {
            /* Load from OpenWithList key */
            LoadAppList(hSubkey);
            RegCloseKey(hSubkey);
        }
    }

    /* Close the key */
    RegCloseKey(hKey);
}

VOID COpenWithList::LoadRecommendedFromHKCU(LPCWSTR pwszExt)
{
    WCHAR wszBuf[MAX_PATH];
    HKEY hKey;

    StringCbPrintfW(wszBuf, sizeof(wszBuf),
                    L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\FileExts\\%s",
                    pwszExt);
    if (RegOpenKeyExW(HKEY_CURRENT_USER, wszBuf, 0, KEY_READ, &hKey) == ERROR_SUCCESS)
    {
        /* Load MRU and ProgId lists */
        LoadMRUList(hKey);
        LoadProgIdList(hKey, pwszExt);

        /* Handle "Aplication" value */
        DWORD cbBuf = sizeof(wszBuf);
        if (RegGetValueW(hKey, NULL, L"Application", RRF_RT_REG_SZ, NULL, wszBuf, &cbBuf) == ERROR_SUCCESS)
        {
            SApp *pApp = Find(wszBuf);
            if (pApp)
                SetRecommended(pApp);
        }

        /* Close the key */
        RegCloseKey(hKey);
    }
}

BOOL COpenWithList::AddAppToMRUList(SApp *pApp, LPCWSTR pwszFilename)
{
    WCHAR wszBuf[100];
    LPCWSTR pwszExt;
    HKEY hKey;
    HANDLE hList;

    /* Get file extension */
    pwszExt = PathFindExtensionW(pwszFilename);
    if (!pwszExt[0])
        return FALSE;

    /* Build registry key */
    if (FAILED(StringCbPrintfW(wszBuf, sizeof(wszBuf),
                               L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\FileExts\\%s",
                               pwszExt)))
    {
        ERR("insufficient buffer\n");
        return FALSE;
    }

    /* Open base key for this file extension */
    if (RegCreateKeyExW(HKEY_CURRENT_USER, wszBuf, 0, NULL, 0, KEY_WRITE | KEY_READ, NULL, &hKey, NULL) != ERROR_SUCCESS)
        return FALSE;

    /* Open MRU list */
    hList = OpenMRUList(hKey);
    if (hList)
    {
        /* Insert the entry */
        AddMRUStringW(hList, pApp->wszFilename);

        /* Close MRU list */
        FreeMRUList(hList);
    }

    RegCloseKey(hKey);
    return TRUE;
}

BOOL COpenWithList::SetDefaultHandler(SApp *pApp, LPCWSTR pwszFilename)
{
    HKEY hKey, hSrcKey, hDestKey;
    WCHAR wszBuf[256];

    TRACE("SetDefaultHandler %ls %ls\n", pApp->wszFilename, pwszFilename);

    /* Extract file extension */
    LPCWSTR pwszExt = PathFindExtensionW(pwszFilename);
    if (!pwszExt[0] || !pwszExt[1])
        return FALSE;

    /* Create file extension key */
    if (RegCreateKeyExW(HKEY_CLASSES_ROOT, pwszExt, 0, NULL, 0, KEY_READ|KEY_WRITE, NULL, &hKey, NULL) != ERROR_SUCCESS)
    {
        ERR("Cannot open ext key");
        return FALSE;
    }

    DWORD dwSize = sizeof(wszBuf);
    LONG lResult = RegGetValueW(hKey, NULL, L"", RRF_RT_REG_SZ, NULL, wszBuf, &dwSize);

    if (lResult == ERROR_FILE_NOT_FOUND)
    {
        /* A new entry was created or the default key is not set: set the prog key id */
        StringCbPrintfW(wszBuf, sizeof(wszBuf), L"%s_auto_file", pwszExt + 1);
        if (RegSetValueExW(hKey, L"", 0, REG_SZ, (const BYTE*)wszBuf, (wcslen(wszBuf) + 1) * sizeof(WCHAR)) != ERROR_SUCCESS)
        {
            RegCloseKey(hKey);
            ERR("RegSetValueExW failed\n");
            return FALSE;
        }
    }
    else if (lResult != ERROR_SUCCESS)
    {
        RegCloseKey(hKey);
        ERR("RegGetValueExW failed: 0x%08x\n", lResult);
        return FALSE;
    }

    /* Close file extension key */
    RegCloseKey(hKey);

    /* Create prog id key */
    if (RegCreateKeyExW(HKEY_CLASSES_ROOT, wszBuf, 0, NULL, 0, KEY_WRITE, NULL, &hKey, NULL) != ERROR_SUCCESS)
    {
        ERR("RegCreateKeyExW failed\n");
        return FALSE;
    }

    /* Check if there already verbs existing for that app */
    StringCbPrintfW(wszBuf, sizeof(wszBuf), L"Applications\\%s\\shell", pApp->wszFilename);
    if (RegOpenKeyExW(HKEY_CLASSES_ROOT, wszBuf, 0, KEY_READ, &hSrcKey) != ERROR_SUCCESS)
    {
        ERR("RegOpenKeyExW %ls failed\n", wszBuf);
        RegCloseKey(hKey);
        return FALSE;
    }

    /* Open destination key */
    if (RegCreateKeyExW(hKey, L"shell", 0, NULL, 0, KEY_WRITE, NULL, &hDestKey, NULL) != ERROR_SUCCESS)
    {
        ERR("RegCreateKeyExW failed\n");
        RegCloseKey(hSrcKey);
        RegCloseKey(hKey);
        return FALSE;
    }

    /* Copy static verbs from Classes\Applications key */
    /* FIXME: SHCopyKey does not copy the security attributes of the keys */
    LSTATUS Result = SHCopyKeyW(hSrcKey, NULL, hDestKey, 0);
    RegCloseKey(hDestKey);
    RegCloseKey(hSrcKey);
    RegCloseKey(hKey);

    if (Result != ERROR_SUCCESS)
    {
        ERR("SHCopyKeyW failed\n");
        return FALSE;
    }

    return TRUE;
}

class COpenWithDialog
{
    public:
        COpenWithDialog(const OPENASINFO *pInfo, COpenWithList *pAppList);
        ~COpenWithDialog();
        static INT_PTR CALLBACK DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
        BOOL IsNoOpen(HWND hwnd);

    private:
        VOID Init(HWND hwnd);
        VOID AddApp(COpenWithList::SApp *pApp, BOOL bSelected);
        VOID Browse();
        VOID Accept();
        static INT_PTR CALLBACK NoOpenDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);
        COpenWithList::SApp *GetCurrentApp();

        const OPENASINFO *m_pInfo;
        COpenWithList *m_pAppList;
        BOOL m_bListAllocated;
        HWND m_hDialog, m_hTreeView;
        HTREEITEM m_hRecommend;
        HTREEITEM m_hOther;
        HIMAGELIST m_hImgList;
        BOOL m_bNoOpen;
};

COpenWithDialog::COpenWithDialog(const OPENASINFO *pInfo, COpenWithList *pAppList = NULL):
    m_pInfo(pInfo), m_pAppList(pAppList), m_hImgList(NULL), m_bNoOpen(FALSE)
{
    if (!m_pAppList)
    {
        m_pAppList = new COpenWithList;
        m_bListAllocated = TRUE;
    }
    else
        m_bListAllocated = FALSE;
}

COpenWithDialog::~COpenWithDialog()
{
    if (m_bListAllocated && m_pAppList)
        delete m_pAppList;
    if (m_hImgList)
        ImageList_Destroy(m_hImgList);
}

INT_PTR CALLBACK COpenWithDialog::NoOpenDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
    switch(Message)
    {
        case WM_INITDIALOG:
        {
            return TRUE;
        }
        case WM_CLOSE:
            EndDialog(hwnd, IDNO);
            break;
        case WM_COMMAND:
            switch(LOWORD(wParam))
            {
                case IDYES:
                    EndDialog(hwnd, IDYES);
                break;
                case IDNO:
                    EndDialog(hwnd, IDNO);
                break;
            }
        break;
        default:
            return FALSE;
    }
    return TRUE;
}

BOOL COpenWithDialog::IsNoOpen(HWND hwnd)
{
    /* Only do the actual check if the file type has the 'NoOpen' flag. */
    if (m_bNoOpen)
    {
        int dReturnValue = DialogBox(shell32_hInstance, MAKEINTRESOURCE(IDD_NOOPEN), hwnd, NoOpenDlgProc);

        if (dReturnValue == IDNO)
            return TRUE;
        else if (dReturnValue == -1)
        {
            ERR("IsNoOpen failed to load the dialog box.");
            return TRUE;
        }
    }

    return FALSE;
}

VOID COpenWithDialog::AddApp(COpenWithList::SApp *pApp, BOOL bSelected)
{
    LPCWSTR pwszName = m_pAppList->GetName(pApp);
    HICON hIcon = m_pAppList->GetIcon(pApp);

    TRACE("AddApp Cmd %ls Name %ls\n", pApp->wszCmd, pwszName);

    /* Add item to the list */
    TVINSERTSTRUCT tvins;

    if (pApp->bRecommended)
        tvins.hParent = tvins.hInsertAfter = m_hRecommend;
    else
        tvins.hParent = tvins.hInsertAfter = m_hOther;

    tvins.item.mask = TVIF_TEXT|TVIF_PARAM;
    tvins.item.pszText = (LPWSTR)pwszName;
    tvins.item.lParam = (LPARAM)pApp;

    if (hIcon)
    {
        tvins.item.mask |= TVIF_IMAGE | TVIF_SELECTEDIMAGE;
        tvins.item.iImage = tvins.item.iSelectedImage = ImageList_AddIcon(m_hImgList, hIcon);
    }

    HTREEITEM hItem = TreeView_InsertItem(m_hTreeView, &tvins);

    if (bSelected)
        TreeView_SelectItem(m_hTreeView, hItem);
}

VOID COpenWithDialog::Browse()
{
    WCHAR wszTitle[64];
    WCHAR wszFilter[256];
    WCHAR wszPath[MAX_PATH];
    OPENFILENAMEW ofn;

    /* Initialize OPENFILENAMEW structure */
    ZeroMemory(&ofn, sizeof(OPENFILENAMEW));
    ofn.lStructSize  = sizeof(OPENFILENAMEW);
    ofn.hInstance = shell32_hInstance;
    ofn.hwndOwner = m_hDialog;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
    ofn.nMaxFile = (sizeof(wszPath) / sizeof(WCHAR));
    ofn.lpstrFile = wszPath;
    ofn.lpstrInitialDir = L"%programfiles%";

    /* Init title */
    if (LoadStringW(shell32_hInstance, IDS_OPEN_WITH, wszTitle, sizeof(wszTitle) / sizeof(WCHAR)))
    {
        ofn.lpstrTitle = wszTitle;
        ofn.nMaxFileTitle = wcslen(wszTitle);
    }

    /* Init the filter string */
    if (LoadStringW(shell32_hInstance, IDS_OPEN_WITH_FILTER, wszFilter, sizeof(wszFilter) / sizeof(WCHAR)))
        ofn.lpstrFilter = wszFilter;
    ZeroMemory(wszPath, sizeof(wszPath));

    /* Create OpenFile dialog */
    if (!GetOpenFileNameW(&ofn))
        return;

    /* Setup context for insert proc */
    COpenWithList::SApp *pApp = m_pAppList->Add(wszPath);
    AddApp(pApp, TRUE);
}

COpenWithList::SApp *COpenWithDialog::GetCurrentApp()
{
    TVITEM tvi;
    tvi.hItem = TreeView_GetSelection(m_hTreeView);
    if (!tvi.hItem)
        return NULL;

    tvi.mask = TVIF_PARAM;
    if (!TreeView_GetItem(m_hTreeView, &tvi))
        return NULL;

    return (COpenWithList::SApp*)tvi.lParam;
}

VOID COpenWithDialog::Init(HWND hwnd)
{
    TRACE("COpenWithDialog::Init hwnd %p\n", hwnd);

    m_hDialog = hwnd;
    SetWindowLongPtr(hwnd, DWLP_USER, (LONG_PTR)this);

    /* Handle register checkbox */
    HWND hRegisterCheckbox = GetDlgItem(hwnd, 14003);
    if (!(m_pInfo->oaifInFlags & OAIF_ALLOW_REGISTRATION))
        EnableWindow(hRegisterCheckbox, FALSE);
    if (m_pInfo->oaifInFlags & OAIF_FORCE_REGISTRATION)
        SendMessage(hRegisterCheckbox, BM_SETCHECK, BST_CHECKED, 0);
    if (m_pInfo->oaifInFlags & OAIF_HIDE_REGISTRATION)
        ShowWindow(hRegisterCheckbox, SW_HIDE);

    if (m_pInfo->pcszFile)
    {
        WCHAR wszBuf[MAX_PATH];
        UINT cchBuf;

        /* Add filename to label */
        cchBuf = GetDlgItemTextW(hwnd, 14001, wszBuf, _countof(wszBuf));
        StringCchCopyW(wszBuf + cchBuf, _countof(wszBuf) - cchBuf, PathFindFileNameW(m_pInfo->pcszFile));
        SetDlgItemTextW(hwnd, 14001, wszBuf);

        /* Load applications from registry */
        m_pAppList->Load();
        m_pAppList->LoadRecommended(m_pInfo->pcszFile);

        /* Determine if the type of file can be opened directly from the shell */
        if (m_pAppList->IsNoOpen() != FALSE)
            m_bNoOpen = TRUE;

        /* Init treeview */
        m_hTreeView = GetDlgItem(hwnd, 14002);
        m_hImgList = ImageList_Create(16, 16,  ILC_COLOR32 | ILC_MASK, m_pAppList->GetCount() + 1, m_pAppList->GetCount() + 1);
        (void)TreeView_SetImageList(m_hTreeView, m_hImgList, TVSIL_NORMAL);

        /* If there are some recommendations add parent nodes: Recommended and Others */
        UINT cRecommended = m_pAppList->GetRecommendedCount();
        if (cRecommended > 0)
        {
            TVINSERTSTRUCT tvins;
            HICON hFolderIcon;

            tvins.hParent = tvins.hInsertAfter = TVI_ROOT;
            tvins.item.mask = TVIF_TEXT|TVIF_STATE|TVIF_IMAGE|TVIF_SELECTEDIMAGE;
            tvins.item.pszText = (LPWSTR)wszBuf;
            tvins.item.state = tvins.item.stateMask = TVIS_EXPANDED;
            hFolderIcon = (HICON)LoadImage(shell32_hInstance, MAKEINTRESOURCE(IDI_SHELL_PROGRAMS_FOLDER), IMAGE_ICON, 0, 0, 0);
            tvins.item.iImage = tvins.item.iSelectedImage = ImageList_AddIcon(m_hImgList, hFolderIcon);

            LoadStringW(shell32_hInstance, IDS_OPEN_WITH_RECOMMENDED, wszBuf, _countof(wszBuf));
            m_hRecommend = TreeView_InsertItem(m_hTreeView, &tvins);

            LoadStringW(shell32_hInstance, IDS_OPEN_WITH_OTHER, wszBuf, _countof(wszBuf));
            m_hOther = TreeView_InsertItem(m_hTreeView, &tvins);
        }
        else
            m_hRecommend = m_hOther = TVI_ROOT;

        /* Add all applications */
        BOOL bNoAppSelected = TRUE;
        COpenWithList::SApp *pAppList = m_pAppList->GetList();
        for (UINT i = 0; i < m_pAppList->GetCount(); ++i)
        {
            if (!COpenWithList::IsHidden(&pAppList[i]))
            {
                if (bNoAppSelected && (pAppList[i].bRecommended || !cRecommended))
                {
                    AddApp(&pAppList[i], TRUE);
                    bNoAppSelected = FALSE;
                }
                else
                    AddApp(&pAppList[i], FALSE);
            }
        }
    }
}

VOID COpenWithDialog::Accept()
{
    COpenWithList::SApp *pApp = GetCurrentApp();
    if (pApp)
    {
        /* Set programm as default handler */
        if (SendDlgItemMessage(m_hDialog, 14003, BM_GETCHECK, 0, 0) == BST_CHECKED)
            m_pAppList->SetDefaultHandler(pApp, m_pInfo->pcszFile);

        /* Execute program */
        if (m_pInfo->oaifInFlags & OAIF_EXEC)
            m_pAppList->Execute(pApp, m_pInfo->pcszFile);

        EndDialog(m_hDialog, 1);
    }
}

INT_PTR CALLBACK COpenWithDialog::DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    COpenWithDialog *pThis = reinterpret_cast<COpenWithDialog *>(GetWindowLongPtr(hwndDlg, DWLP_USER));

    switch(uMsg)
    {
        case WM_INITDIALOG:
        {
            COpenWithDialog *pThis = reinterpret_cast<COpenWithDialog *>(lParam);

            pThis->Init(hwndDlg);
            return TRUE;
        }
        case WM_COMMAND:
            switch(LOWORD(wParam))
            {
                case 14004: /* browse */
                {
                    pThis->Browse();
                    return TRUE;
                }
                case IDOK: /* ok */
                {
                    pThis->Accept();
                    return TRUE;
                }
                case IDCANCEL: /* cancel */
                    EndDialog(hwndDlg, 0);
                    return TRUE;
                default:
                    break;
            }
            break;
        case WM_NOTIFY:
             switch (((LPNMHDR)lParam)->code)
             {
                case TVN_SELCHANGED:
                    EnableWindow(GetDlgItem(hwndDlg, IDOK), pThis->GetCurrentApp() ? TRUE : FALSE);
                    break;
                case NM_DBLCLK:
                case NM_RETURN:
                    pThis->Accept();
                    break;
             }
            break;
        case WM_CLOSE:
            EndDialog(hwndDlg, 0);
            return TRUE;
        default:
            break;
    }
    return FALSE;
}

COpenWithMenu::COpenWithMenu()
{
    m_idCmdFirst = 0;
    m_idCmdLast = 0;
    m_pAppList = new COpenWithList;
}

COpenWithMenu::~COpenWithMenu()
{
    TRACE("Destroying COpenWithMenu(%p)\n", this);

    if (m_hSubMenu)
    {
        INT Count, Index;
        MENUITEMINFOW mii;

        /* get item count */
        Count = GetMenuItemCount(m_hSubMenu);
        if (Count == -1)
            return;

        /* setup menuitem info */
        ZeroMemory(&mii, sizeof(mii));
        mii.cbSize = sizeof(mii);
        mii.fMask = MIIM_DATA | MIIM_FTYPE | MIIM_CHECKMARKS;

        for(Index = 0; Index < Count; Index++)
        {
            if (GetMenuItemInfoW(m_hSubMenu, Index, TRUE, &mii))
            {
                if (mii.hbmpChecked)
                    DeleteObject(mii.hbmpChecked);
            }
        }
    }

    if (m_pAppList)
        delete m_pAppList;
}

HBITMAP COpenWithMenu::IconToBitmap(HICON hIcon)
{
    HDC hdc, hdcScr;
    HBITMAP hbm, hbmOld;
    RECT rc;

    hdcScr = GetDC(NULL);
    hdc = CreateCompatibleDC(hdcScr);
    SetRect(&rc, 0, 0, GetSystemMetrics(SM_CXMENUCHECK), GetSystemMetrics(SM_CYMENUCHECK));
    hbm = CreateCompatibleBitmap(hdcScr, rc.right, rc.bottom);
    ReleaseDC(NULL, hdcScr);

    hbmOld = (HBITMAP)SelectObject(hdc, hbm);
    FillRect(hdc, &rc, (HBRUSH)(COLOR_MENU + 1));
    if (!DrawIconEx(hdc, 0, 0, hIcon, rc.right, rc.bottom, 0, NULL, DI_NORMAL))
        ERR("DrawIcon failed: %x\n", GetLastError());
    SelectObject(hdc, hbmOld);

    DeleteDC(hdc);

    return hbm;
}

VOID COpenWithMenu::AddChooseProgramItem()
{
    MENUITEMINFOW mii;
    WCHAR wszBuf[128];

    ZeroMemory(&mii, sizeof(mii));
    mii.cbSize = sizeof(mii);
    mii.fMask = MIIM_TYPE | MIIM_ID;
    mii.fType = MFT_SEPARATOR;
    mii.wID = -1;
    InsertMenuItemW(m_hSubMenu, -1, TRUE, &mii);

    if (!LoadStringW(shell32_hInstance, IDS_OPEN_WITH_CHOOSE, wszBuf, _countof(wszBuf)))
    {
        ERR("Failed to load string\n");
        return;
    }

    mii.fMask = MIIM_ID | MIIM_TYPE | MIIM_STATE;
    mii.fType = MFT_STRING;
    mii.fState = MFS_ENABLED;
    mii.wID = m_idCmdLast;
    mii.dwTypeData = (LPWSTR)wszBuf;
    mii.cch = wcslen(wszBuf);

    InsertMenuItemW(m_hSubMenu, -1, TRUE, &mii);
}

VOID COpenWithMenu::AddApp(PVOID pApp)
{
    MENUITEMINFOW mii;
    LPCWSTR pwszName = m_pAppList->GetName((COpenWithList::SApp*)pApp);

    ZeroMemory(&mii, sizeof(mii));
    mii.cbSize = sizeof(mii);
    mii.fMask = MIIM_ID | MIIM_TYPE | MIIM_STATE | MIIM_DATA;
    mii.fType = MFT_STRING;
    mii.fState = MFS_ENABLED;
    mii.wID = m_idCmdLast;
    mii.dwTypeData = (LPWSTR)pwszName;
    mii.cch = wcslen(mii.dwTypeData);
    mii.dwItemData = (ULONG_PTR)pApp;

    HICON hIcon = m_pAppList->GetIcon((COpenWithList::SApp*)pApp);
    if (hIcon)
    {
        mii.fMask |= MIIM_CHECKMARKS;
        mii.hbmpChecked = mii.hbmpUnchecked = IconToBitmap(hIcon);
    }

    if (InsertMenuItemW(m_hSubMenu, -1, TRUE, &mii))
        m_idCmdLast++;
}

HRESULT WINAPI COpenWithMenu::QueryContextMenu(
    HMENU hMenu,
    UINT indexMenu,
    UINT idCmdFirst,
    UINT idCmdLast,
    UINT uFlags)
{
    TRACE("hMenu %p indexMenu %u idFirst %u idLast %u uFlags %u\n", hMenu, indexMenu, idCmdFirst, idCmdLast, uFlags);

    INT DefaultPos = GetMenuDefaultItem(hMenu, TRUE, 0);

    WCHAR wszName[100];
    UINT NameId = (DefaultPos == -1 ? IDS_OPEN : IDS_OPEN_WITH);
    if (!LoadStringW(shell32_hInstance, NameId, wszName, _countof(wszName)))
    {
        ERR("Failed to load string\n");
        return E_FAIL;
    }

    /* Init first cmd id and submenu */
    m_idCmdFirst = m_idCmdLast = idCmdFirst;
    m_hSubMenu = NULL;

    /* If we are going to be default item, we shouldn't be submenu */
    if (DefaultPos != -1)
    {
        /* Load applications list */
        m_pAppList->Load();
        m_pAppList->LoadRecommended(m_wszPath);

        /* Create submenu only if there is more than one application and menu has a default item */
        if (m_pAppList->GetRecommendedCount() > 1)
        {
            m_hSubMenu = CreatePopupMenu();

            for(UINT i = 0; i < m_pAppList->GetCount(); ++i)
            {
                COpenWithList::SApp *pApp = m_pAppList->GetList() + i;
                if (pApp->bRecommended)
                    AddApp(pApp);
            }

            AddChooseProgramItem();
        }
    }

    /* Insert menu item */
    MENUITEMINFOW mii;
    ZeroMemory(&mii, sizeof(mii));
    mii.cbSize = sizeof(mii);
    mii.fMask = MIIM_ID | MIIM_TYPE | MIIM_STATE;
    if (m_hSubMenu)
    {
        mii.fMask |= MIIM_SUBMENU;
        mii.hSubMenu = m_hSubMenu;
        mii.wID = -1;
    }
    else
        mii.wID = m_idCmdLast;

    mii.fType = MFT_STRING;
    mii.dwTypeData = (LPWSTR)wszName;
    mii.cch = wcslen(wszName);

    mii.fState = MFS_ENABLED;
    if (DefaultPos == -1)
        mii.fState |= MFS_DEFAULT;

    if (!InsertMenuItemW(hMenu, DefaultPos + 1, TRUE, &mii))
        return E_FAIL;

    return MAKE_HRESULT(SEVERITY_SUCCESS, 0, m_idCmdLast - m_idCmdFirst + 1);
}

HRESULT WINAPI
COpenWithMenu::InvokeCommand(LPCMINVOKECOMMANDINFO lpici)
{
    HRESULT hr = E_FAIL;

    TRACE("This %p idFirst %u idLast %u idCmd %u\n", this, m_idCmdFirst, m_idCmdLast, m_idCmdFirst + LOWORD(lpici->lpVerb));

    if (HIWORD(lpici->lpVerb) == 0 && m_idCmdFirst + LOWORD(lpici->lpVerb) <= m_idCmdLast)
    {
        if (m_idCmdFirst + LOWORD(lpici->lpVerb) == m_idCmdLast)
        {
            OPENASINFO info;
            LPCWSTR pwszExt = PathFindExtensionW(m_wszPath);

            info.pcszFile = m_wszPath;
            info.oaifInFlags = OAIF_EXEC;
            if (pwszExt[0])
                info.oaifInFlags |= OAIF_REGISTER_EXT | OAIF_ALLOW_REGISTRATION;
            info.pcszClass = NULL;
            hr = SHOpenWithDialog(lpici->hwnd, &info);
        }
        else
        {
            /* retrieve menu item info */
            MENUITEMINFOW mii;
            ZeroMemory(&mii, sizeof(mii));
            mii.cbSize = sizeof(mii);
            mii.fMask = MIIM_DATA | MIIM_FTYPE;

            if (GetMenuItemInfoW(m_hSubMenu, LOWORD(lpici->lpVerb), TRUE, &mii) && mii.dwItemData)
            {
                /* launch item with specified app */
                COpenWithList::SApp *pApp = (COpenWithList::SApp*)mii.dwItemData;
                COpenWithList::Execute(pApp, m_wszPath);
                hr = S_OK;
            }
        }
    }

    return hr;
}

HRESULT WINAPI
COpenWithMenu::GetCommandString(UINT_PTR idCmd, UINT uType,
                                UINT* pwReserved, LPSTR pszName, UINT cchMax )
{
    FIXME("%p %lu %u %p %p %u\n", this,
          idCmd, uType, pwReserved, pszName, cchMax );

    return E_NOTIMPL;
}

HRESULT WINAPI COpenWithMenu::HandleMenuMsg(
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam)
{
    TRACE("This %p uMsg %x\n", this, uMsg);

    return E_NOTIMPL;
}

HRESULT WINAPI
COpenWithMenu::Initialize(LPCITEMIDLIST pidlFolder,
                          IDataObject *pdtobj,
                          HKEY hkeyProgID)
{
    STGMEDIUM medium;
    FORMATETC fmt;
    HRESULT hr;
    LPIDA pida;
    LPCITEMIDLIST pidlFolder2;
    LPCITEMIDLIST pidlChild;
    LPCITEMIDLIST pidl;
    LPCWSTR pwszExt;

    TRACE("This %p\n", this);

    if (pdtobj == NULL)
        return E_INVALIDARG;

    fmt.cfFormat = RegisterClipboardFormatW(CFSTR_SHELLIDLIST);
    fmt.ptd = NULL;
    fmt.dwAspect = DVASPECT_CONTENT;
    fmt.lindex = -1;
    fmt.tymed = TYMED_HGLOBAL;

    hr = pdtobj->GetData(&fmt, &medium);

    if (FAILED(hr))
    {
        ERR("pdtobj->GetData failed with 0x%x\n", hr);
        return hr;
    }

    pida = (LPIDA)GlobalLock(medium.hGlobal);
    ASSERT(pida->cidl >= 1);

    pidlFolder2 = (LPCITEMIDLIST) ((LPBYTE)pida + pida->aoffset[0]);
    pidlChild = (LPCITEMIDLIST) ((LPBYTE)pida + pida->aoffset[1]);

    if (!_ILIsValue(pidlChild))
    {
        TRACE("pidl is not a file\n");
        GlobalUnlock(medium.hGlobal);
        GlobalFree(medium.hGlobal);
        return E_FAIL;
    }

    pidl = ILCombine(pidlFolder2, pidlChild);

    GlobalUnlock(medium.hGlobal);
    GlobalFree(medium.hGlobal);

    if (!pidl)
    {
        ERR("no mem\n");
        return E_OUTOFMEMORY;
    }

    if (!SHGetPathFromIDListW(pidl, m_wszPath))
    {
        SHFree((void*)pidl);
        ERR("SHGetPathFromIDListW failed\n");
        return E_FAIL;
    }

    SHFree((void*)pidl);
    TRACE("szPath %s\n", debugstr_w(m_wszPath));

    pwszExt = PathFindExtensionW(m_wszPath);
    if (PathIsExeW(pwszExt) || !_wcsicmp(pwszExt, L".lnk"))
    {
        TRACE("file is a executable or shortcut\n");
        return E_FAIL;
    }

    return S_OK;
}

HRESULT WINAPI
SHOpenWithDialog(HWND hwndParent, const OPENASINFO *poainfo)
{
    INT_PTR ret;

    TRACE("SHOpenWithDialog hwndParent %p poainfo %p\n", hwndParent, poainfo);

    InitCommonControls();

    if (poainfo->pcszClass == NULL && poainfo->pcszFile == NULL)
        return E_FAIL;

    COpenWithDialog pDialog(poainfo);

    if (pDialog.IsNoOpen(hwndParent))
        return S_OK;

    ret = DialogBoxParamW(shell32_hInstance, MAKEINTRESOURCE(IDD_OPEN_WITH), hwndParent,
                          COpenWithDialog::DialogProc, (LPARAM)&pDialog);

    if (ret == (INT_PTR)-1)
    {
        ERR("Failed to create dialog: %u\n", GetLastError());
        return E_FAIL;
    }

    return S_OK;
}
