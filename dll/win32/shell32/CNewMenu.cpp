/*
 * provides new shell item service
 *
 * Copyright 2007 Johannes Anderwald (johannes.anderwald@reactos.org)
 * Copyright 2009 Andrew Hill
 * Copyright 2012 Rafal Harabien
 * Copyright 2019 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
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

CNewMenu::CNewMenu() :
    m_pidlFolder(NULL),
    m_pItems(NULL),
    m_pLinkItem(NULL),
    m_pSite(NULL),
    m_idCmdFirst(0),
    m_idCmdFolder(-1),
    m_idCmdLink(-1),
    m_bCustomIconFolder(FALSE),
    m_bCustomIconLink(FALSE),
    m_hIconFolder(NULL),
    m_hIconLink(NULL)
{
}

CNewMenu::~CNewMenu()
{
    UnloadAllItems();

    if (m_bCustomIconFolder && m_hIconFolder)
        DestroyIcon(m_hIconFolder);
    if (m_bCustomIconLink && m_hIconLink)
        DestroyIcon(m_hIconLink);
    if (m_pidlFolder)
        ILFree(m_pidlFolder);
}

void CNewMenu::UnloadItem(SHELLNEW_ITEM *pItem)
{
    /* Note: free allows NULL as argument */
    free(pItem->pData);
    free(pItem->pwszDesc);
    free(pItem->pwszExt);

    if (pItem->hIcon)
        DestroyIcon(pItem->hIcon);

    HeapFree(GetProcessHeap(), 0, pItem);
}

void CNewMenu::UnloadAllItems()
{
    SHELLNEW_ITEM *pCurItem;

    /* Unload the handler items, including the link item */
    while (m_pItems)
    {
        pCurItem = m_pItems;
        m_pItems = m_pItems->pNext;
        UnloadItem(pCurItem);
    }
    m_pItems = NULL;
    m_pLinkItem = NULL;
}

CNewMenu::SHELLNEW_ITEM *CNewMenu::LoadItem(LPCWSTR pwszExt)
{
    HKEY hKey;
    WCHAR wszBuf[MAX_PATH];
    PBYTE pData = NULL;
    DWORD cbData;

    StringCbPrintfW(wszBuf, sizeof(wszBuf), L"%s\\ShellNew", pwszExt);

    TRACE("LoadItem Keyname %s Name %s\n", debugstr_w(pwszExt), debugstr_w(wszBuf));

    if (RegOpenKeyExW(HKEY_CLASSES_ROOT, wszBuf, 0, KEY_READ, &hKey) != ERROR_SUCCESS)
    {
        TRACE("Failed to open key\n");
        return NULL;
    }

    /* Find first valid value */
    struct
    {
        LPCWSTR pszName;
        SHELLNEW_TYPE Type;
        BOOL bNeedData;
        BOOL bStr;
    } Types[] = {
        {L"FileName", SHELLNEW_TYPE_FILENAME, TRUE, TRUE},
        {L"Command", SHELLNEW_TYPE_COMMAND, TRUE, TRUE},
        {L"Data", SHELLNEW_TYPE_DATA, TRUE, FALSE},
        {L"NullFile", SHELLNEW_TYPE_NULLFILE, FALSE},
        {NULL}
    };
    UINT i;

    for (i = 0; Types[i].pszName; ++i)
    {
        /* Note: We are using ANSI function because strings can be treated as data */
        cbData = 0;
        DWORD dwFlags = Types[i].bStr ? RRF_RT_REG_SZ : RRF_RT_ANY;
        DWORD dwType;
        if (RegGetValueW(hKey, NULL, Types[i].pszName, dwFlags, NULL, NULL, &cbData) == ERROR_SUCCESS)
        {
            if (Types[i].bNeedData && cbData > 0)
            {
                pData = (PBYTE)malloc(cbData);
                RegGetValueW(hKey, NULL, Types[i].pszName, dwFlags, &dwType, pData, &cbData);
                if (!Types[i].bStr && (dwType == REG_SZ || dwType == REG_EXPAND_SZ))
                {
                    PBYTE pData2 = (PBYTE)malloc(cbData);
                    cbData = WideCharToMultiByte(CP_ACP, 0, (LPWSTR)pData, -1, (LPSTR)pData2, cbData, NULL, NULL);
                    free(pData);
                    pData = pData2;
                }
            }
            break;
        }
    }
    RegCloseKey(hKey);

    /* Was any key found? */
    if (!Types[i].pszName)
    {
        free(pData);
        return NULL;
    }

    SHFILEINFOW fi;
    if (!SHGetFileInfoW(pwszExt, FILE_ATTRIBUTE_NORMAL, &fi, sizeof(fi), SHGFI_USEFILEATTRIBUTES | SHGFI_TYPENAME | SHGFI_ICON | SHGFI_SMALLICON))
    {
        free(pData);
        return NULL;
    }

    /* Create new item */
    SHELLNEW_ITEM *pNewItem = static_cast<SHELLNEW_ITEM *>(HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(SHELLNEW_ITEM)));
    if (!pNewItem)
    {
        free(pData);
        return NULL;
    }

    TRACE("new item %ls\n", fi.szTypeName);
    pNewItem->Type = Types[i].Type;
    pNewItem->pData = pData;
    pNewItem->cbData = pData ? cbData : 0;
    pNewItem->pwszExt = _wcsdup(pwszExt);
    pNewItem->pwszDesc = _wcsdup(fi.szTypeName);
    if (fi.hIcon)
        pNewItem->hIcon = fi.hIcon;

    return pNewItem;
}

BOOL
CNewMenu::CacheItems()
{
    HKEY hKey;
    DWORD dwSize = 0;
    DWORD dwIndex = 0;
    LPWSTR lpValue;
    LPWSTR lpValues;
    WCHAR wszName[MAX_PATH];
    SHELLNEW_ITEM *pNewItem;
    SHELLNEW_ITEM *pCurItem = NULL;

    /* Enumerate all extensions */
    while (RegEnumKeyW(HKEY_CLASSES_ROOT, dwIndex++, wszName, _countof(wszName)) == ERROR_SUCCESS)
    {
        if (wszName[0] != L'.')
            continue;

        pNewItem = LoadItem(wszName);
        if (pNewItem)
        {
            dwSize += wcslen(wszName) + 1;
            if (!m_pLinkItem && _wcsicmp(pNewItem->pwszExt, L".lnk") == 0)
            {
                /* The unique link handler */
                m_pLinkItem = pNewItem;
            }

            /* Add at the end of the list */
            if (pCurItem)
            {
                pCurItem->pNext = pNewItem;
                pCurItem = pNewItem;
            }
            else
            {
                pCurItem = m_pItems = pNewItem;
            }
        }
    }

    dwSize++;

    lpValues = (LPWSTR) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwSize * sizeof(WCHAR));
    if (!lpValues)
        return FALSE;

    for (pCurItem = m_pItems, lpValue = lpValues; pCurItem; pCurItem = pCurItem->pNext)
    {
        memcpy(lpValue, pCurItem->pwszExt, (wcslen(pCurItem->pwszExt) + 1) * sizeof(WCHAR));
        lpValue += wcslen(pCurItem->pwszExt) + 1;
    }

    if (RegCreateKeyEx(HKEY_CURRENT_USER, ShellNewKey, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL) != ERROR_SUCCESS)
    {
        HeapFree(GetProcessHeap(), 0, lpValues);
        return FALSE;
    }

    if (RegSetValueExW(hKey, L"Classes", NULL, REG_MULTI_SZ, (LPBYTE)lpValues, dwSize * sizeof(WCHAR)) != ERROR_SUCCESS)
    {
        HeapFree(GetProcessHeap(), 0, lpValues);
        RegCloseKey(hKey);
        return FALSE;
    }

    HeapFree(GetProcessHeap(), 0, lpValues);
    RegCloseKey(hKey);

    return TRUE;
}

BOOL
CNewMenu::LoadCachedItems()
{
    LPWSTR wszName;
    LPWSTR lpValues;
    DWORD dwSize;
    HKEY hKey;
    SHELLNEW_ITEM *pNewItem;
    SHELLNEW_ITEM *pCurItem = NULL;

    if (RegOpenKeyExW(HKEY_CURRENT_USER, ShellNewKey, 0, KEY_READ, &hKey) != ERROR_SUCCESS)
        return FALSE;

    if (RegQueryValueExW(hKey, L"Classes", NULL, NULL, NULL, &dwSize) != ERROR_SUCCESS)
        return FALSE;

    lpValues = (LPWSTR) HeapAlloc(GetProcessHeap(), 0, dwSize);
    if (!lpValues)
        return FALSE;

    if (RegQueryValueExW(hKey, L"Classes", NULL, NULL, (LPBYTE)lpValues, &dwSize) != ERROR_SUCCESS)
    {
        HeapFree(GetProcessHeap(), 0, lpValues);
        return FALSE;
    }

    wszName = lpValues;

    for (; *wszName != '\0'; wszName += wcslen(wszName) + 1)
    {
        pNewItem = LoadItem(wszName);
        if (pNewItem)
        {
            if (!m_pLinkItem && _wcsicmp(pNewItem->pwszExt, L".lnk") == 0)
            {
                /* The unique link handler */
                m_pLinkItem = pNewItem;
            }

            /* Add at the end of the list */
            if (pCurItem)
            {
                pCurItem->pNext = pNewItem;
                pCurItem = pNewItem;
            }
            else
            {
                pCurItem = m_pItems = pNewItem;
            }
        }
    }

    HeapFree(GetProcessHeap(), 0, lpValues);
    RegCloseKey(hKey);

    return TRUE;
}

BOOL
CNewMenu::LoadAllItems()
{
    // TODO: We need to find a way to refresh the cache from time to time, when
    // e.g. new extensions with ShellNew handlers have been added or removed.

    /* If there are any unload them */
    UnloadAllItems();

    if (!LoadCachedItems())
    {
        CacheItems();
    }

    return (m_pItems != NULL);
}

UINT
CNewMenu::InsertShellNewItems(HMENU hMenu, UINT idCmdFirst, UINT Pos)
{
    MENUITEMINFOW mii;
    UINT idCmd = idCmdFirst;
    WCHAR wszBuf[256];

    if (m_pItems == NULL)
    {
        if (!LoadAllItems())
            return 0;
    }

    ZeroMemory(&mii, sizeof(mii));
    mii.cbSize = sizeof(mii);

    m_idCmdFirst = idCmd;

    /* Insert the new folder action */
    if (!LoadStringW(shell32_hInstance, FCIDM_SHVIEW_NEWFOLDER, wszBuf, _countof(wszBuf)))
        wszBuf[0] = 0;
    mii.fMask = MIIM_ID | MIIM_BITMAP | MIIM_STRING;
    mii.dwTypeData = wszBuf;
    mii.cch = wcslen(mii.dwTypeData);
    mii.wID = idCmd;
    mii.hbmpItem = HBMMENU_CALLBACK;
    if (InsertMenuItemW(hMenu, Pos++, TRUE, &mii))
        m_idCmdFolder = idCmd++;

    /* Insert the new shortcut action */
    if (m_pLinkItem)
    {
        if (!LoadStringW(shell32_hInstance, FCIDM_SHVIEW_NEWLINK, wszBuf, _countof(wszBuf)))
            wszBuf[0] = 0;
        mii.dwTypeData = wszBuf;
        mii.cch = wcslen(mii.dwTypeData);
        mii.wID = idCmd;
        if (InsertMenuItemW(hMenu, Pos++, TRUE, &mii))
            m_idCmdLink = idCmd++;
    }

    /* Insert a seperator for the custom new action */
    mii.fMask = MIIM_TYPE | MIIM_ID;
    mii.fType = MFT_SEPARATOR;
    mii.wID = -1;
    InsertMenuItemW(hMenu, Pos++, TRUE, &mii);

    /* Insert the rest of the items */
    mii.fMask = MIIM_ID | MIIM_BITMAP | MIIM_STRING | MIIM_DATA;
    mii.fType = 0;

    for (SHELLNEW_ITEM *pCurItem = m_pItems; pCurItem; pCurItem = pCurItem->pNext)
    {
        /* Skip shortcut item */
        if (pCurItem == m_pLinkItem)
            continue;

        TRACE("szDesc %s\n", debugstr_w(pCurItem->pwszDesc));
        mii.dwItemData = (ULONG_PTR)pCurItem;
        mii.dwTypeData = pCurItem->pwszDesc;
        mii.cch = wcslen(mii.dwTypeData);
        mii.wID = idCmd;
        if (InsertMenuItemW(hMenu, Pos++, TRUE, &mii))
            ++idCmd;
    }

    return idCmd - idCmdFirst;
}

CNewMenu::SHELLNEW_ITEM *CNewMenu::FindItemFromIdOffset(UINT IdOffset)
{
    /* Folder */
    if (m_idCmdFirst + IdOffset == m_idCmdFolder)
        return NULL;

    /* Shortcut */
    if (m_idCmdFirst + IdOffset == m_idCmdLink)
        return m_pLinkItem;

    /* Find shell new item - Retrieve menu item info */
    MENUITEMINFOW mii;
    ZeroMemory(&mii, sizeof(mii));
    mii.cbSize = sizeof(mii);
    mii.fMask = MIIM_DATA;

    if (GetMenuItemInfoW(m_hSubMenu, m_idCmdFirst + IdOffset, FALSE, &mii) && mii.dwItemData)
        return (SHELLNEW_ITEM *)mii.dwItemData;
    else
        return NULL;
}

HRESULT CNewMenu::SelectNewItem(LONG wEventId, UINT uFlags, LPWSTR pszName, BOOL bRename)
{
    CComPtr<IShellBrowser> lpSB;
    CComPtr<IShellView> lpSV;
    HRESULT hr = E_FAIL;
    LPITEMIDLIST pidl;
    PITEMID_CHILD pidlNewItem;
    DWORD dwSelectFlags;

    dwSelectFlags = SVSI_DESELECTOTHERS | SVSI_ENSUREVISIBLE | SVSI_FOCUSED | SVSI_SELECT;
    if (bRename)
        dwSelectFlags |= SVSI_EDIT;

    /* Notify the view object about the new item */
    SHChangeNotify(wEventId, uFlags | SHCNF_FLUSH, (LPCVOID)pszName, NULL);

    if (!m_pSite)
        return S_OK;

    /* Get a pointer to the shell view */
    hr = IUnknown_QueryService(m_pSite, SID_IFolderView, IID_PPV_ARG(IShellView, &lpSV));
    if (FAILED_UNEXPECTEDLY(hr))
        return S_OK;

    /* Attempt to get the pidl of the new item */
    hr = SHILCreateFromPathW(pszName, &pidl, NULL);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    pidlNewItem = ILFindLastID(pidl);

    hr = lpSV->SelectItem(pidlNewItem, dwSelectFlags);

    SHFree(pidl);

    return hr;
}

// Code is duplicated in CDefaultContextMenu
HRESULT CNewMenu::CreateNewFolder(LPCMINVOKECOMMANDINFO lpici)
{
    WCHAR wszPath[MAX_PATH];
    WCHAR wszName[MAX_PATH];
    WCHAR wszNewFolder[25];
    HRESULT hr;

    /* Get folder path */
    hr = SHGetPathFromIDListW(m_pidlFolder, wszPath);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    if (!LoadStringW(shell32_hInstance, IDS_NEWFOLDER, wszNewFolder, _countof(wszNewFolder)))
        return E_FAIL;

    /* Create the name of the new directory */
    if (!PathYetAnotherMakeUniqueName(wszName, wszPath, NULL, wszNewFolder))
        return E_FAIL;

    /* Create the new directory and show the appropriate dialog in case of error */
    if (SHCreateDirectory(lpici->hwnd, wszName) != ERROR_SUCCESS)
        return E_FAIL;

    /* Show and select the new item in the def view */
    SelectNewItem(SHCNE_MKDIR, SHCNF_PATHW, wszName, TRUE);

    return S_OK;
}

HRESULT CNewMenu::NewItemByCommand(SHELLNEW_ITEM *pItem, LPCWSTR wszPath)
{
    WCHAR wszBuf[MAX_PATH];
    LPWSTR Ptr, pwszCmd;
    WCHAR wszTemp[MAX_PATH];
    STARTUPINFOW si;
    PROCESS_INFORMATION pi;

    if (!ExpandEnvironmentStringsW((LPWSTR)pItem->pData, wszBuf, _countof(wszBuf)))
    {
        TRACE("ExpandEnvironmentStrings failed\n");
        return E_FAIL;
    }

    /* Expand command parameter, FIXME: there can be more modifiers */
    Ptr = wcsstr(wszBuf, L"%1");
    if (Ptr)
    {
        Ptr[1] = L's';
        StringCbPrintfW(wszTemp, sizeof(wszTemp), wszBuf, wszPath);
        pwszCmd = wszTemp;
    }
    else
    {
        pwszCmd = wszBuf;
    }

    /* Create process */
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    if (CreateProcessW(NULL, pwszCmd, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
    {
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        return S_OK;
    }
    else
    {
        ERR("Failed to create process\n");
        return E_FAIL;
    }
}

HRESULT CNewMenu::NewItemByNonCommand(SHELLNEW_ITEM *pItem, LPWSTR wszName,
                                      DWORD cchNameMax, LPCWSTR wszPath)
{
    BOOL bSuccess = TRUE;

    CStringW strNewItem;
    strNewItem.Format(IDS_NEWITEMFORMAT, pItem->pwszDesc);
    strNewItem += pItem->pwszExt;

    /* Create the name of the new file */
    if (!PathYetAnotherMakeUniqueName(wszName, wszPath, NULL, strNewItem))
        return E_FAIL;

    /* Create new file */
    HANDLE hFile = CreateFileW(wszName, GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile != INVALID_HANDLE_VALUE)
    {
        if (pItem->Type == SHELLNEW_TYPE_DATA)
        {
            /* Write a content */
            DWORD cbWritten;
            WriteFile(hFile, pItem->pData, pItem->cbData, &cbWritten, NULL);
        }

        /* Close file now */
        CloseHandle(hFile);
    }
    else
    {
        bSuccess = FALSE;
    }

    if (pItem->Type == SHELLNEW_TYPE_FILENAME)
    {
        /* Copy file */
        if (!CopyFileW((LPWSTR)pItem->pData, wszName, FALSE))
            ERR("Copy file failed: %ls\n", (LPWSTR)pItem->pData);
    }

    /* Show message if we failed */
    if (bSuccess)
    {
        TRACE("Notifying fs %s\n", debugstr_w(wszName));
        SelectNewItem(SHCNE_CREATE, SHCNF_PATHW, wszName, pItem != m_pLinkItem);
    }
    else
    {
        CStringW Caption(MAKEINTRESOURCEW(IDS_CREATEFILE_CAPTION));
        CStringW Message(MAKEINTRESOURCEW(IDS_CREATEFILE_DENIED));
        Message.FormatMessage(Message.GetString(), wszName);
        MessageBoxW(0, Message, Caption, MB_ICONEXCLAMATION | MB_OK);
    }

    return S_OK;
}

HRESULT CNewMenu::CreateNewItem(SHELLNEW_ITEM *pItem, LPCMINVOKECOMMANDINFO lpcmi)
{
    HRESULT hr;
    WCHAR wszPath[MAX_PATH], wszName[MAX_PATH];

    /* Get folder path */
    hr = SHGetPathFromIDListW(m_pidlFolder, wszPath);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    if (pItem == m_pLinkItem)
    {
        NewItemByNonCommand(pItem, wszName, _countof(wszName), wszPath);
        NewItemByCommand(pItem, wszName);
        return S_OK;
    }

    switch (pItem->Type)
    {
        case SHELLNEW_TYPE_COMMAND:
            NewItemByCommand(pItem, wszPath);
            break;

        case SHELLNEW_TYPE_DATA:
        case SHELLNEW_TYPE_FILENAME:
        case SHELLNEW_TYPE_NULLFILE:
            NewItemByNonCommand(pItem, wszName, _countof(wszName), wszPath);
            break;

        case SHELLNEW_TYPE_INVALID:
            ERR("Invalid type\n");
            break;
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CNewMenu::SetSite(IUnknown *pUnkSite)
{
    m_pSite = pUnkSite;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CNewMenu::GetSite(REFIID riid, void **ppvSite)
{
    return m_pSite->QueryInterface(riid, ppvSite);
}

HRESULT
WINAPI
CNewMenu::QueryContextMenu(HMENU hMenu,
                           UINT indexMenu,
                           UINT idCmdFirst,
                           UINT idCmdLast,
                           UINT uFlags)
{
    MENUITEMINFOW mii;
    UINT cItems = 0;
    WCHAR wszNew[200];

    TRACE("%p %p %u %u %u %u\n", this,
          hMenu, indexMenu, idCmdFirst, idCmdLast, uFlags);

    if (!LoadStringW(shell32_hInstance, FCIDM_SHVIEW_NEW, wszNew, _countof(wszNew)))
        return E_FAIL;

    m_hSubMenu = CreateMenu();
    if (!m_hSubMenu)
        return E_FAIL;

    cItems = InsertShellNewItems(m_hSubMenu, idCmdFirst, 0);

    ZeroMemory(&mii, sizeof(mii));
    mii.cbSize = sizeof(mii);
    mii.fMask = MIIM_TYPE | MIIM_ID | MIIM_STATE | MIIM_SUBMENU;
    mii.fType = MFT_STRING;
    mii.wID = -1;
    mii.dwTypeData = wszNew;
    mii.cch = wcslen(mii.dwTypeData);
    mii.fState = MFS_ENABLED;
    mii.hSubMenu = m_hSubMenu;

    if (!InsertMenuItemW(hMenu, indexMenu, TRUE, &mii))
        return E_FAIL;

    return MAKE_HRESULT(SEVERITY_SUCCESS, 0, cItems);
}

HRESULT
WINAPI
CNewMenu::InvokeCommand(LPCMINVOKECOMMANDINFO lpici)
{
    HRESULT hr = E_FAIL;

    if (m_idCmdFirst + LOWORD(lpici->lpVerb) == m_idCmdFolder)
    {
        hr = CreateNewFolder(lpici);
    }
    else
    {
        SHELLNEW_ITEM *pItem = FindItemFromIdOffset(LOWORD(lpici->lpVerb));
        if (pItem)
            hr = CreateNewItem(pItem, lpici);
    }

    TRACE("CNewMenu::InvokeCommand %x\n", hr);
    return hr;
}

HRESULT
WINAPI
CNewMenu::GetCommandString(UINT_PTR idCmd,
                           UINT uType,
                           UINT *pwReserved,
                           LPSTR pszName,
                           UINT cchMax)
{
    FIXME("%p %lu %u %p %p %u\n", this,
          idCmd, uType, pwReserved, pszName, cchMax );

    return E_NOTIMPL;
}

HRESULT
WINAPI
CNewMenu::HandleMenuMsg(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    return S_OK;
}

HRESULT
WINAPI
CNewMenu::HandleMenuMsg2(UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *plResult)
{
    switch (uMsg)
    {
    case WM_MEASUREITEM:
        {
            MEASUREITEMSTRUCT* lpmis = reinterpret_cast<MEASUREITEMSTRUCT*>(lParam);
            if (!lpmis || lpmis->CtlType != ODT_MENU)
                break;

            if (lpmis->itemWidth < (UINT)GetSystemMetrics(SM_CXMENUCHECK))
                lpmis->itemWidth = GetSystemMetrics(SM_CXMENUCHECK);
            if (lpmis->itemHeight < 16)
                lpmis->itemHeight = 16;

            if (plResult)
                *plResult = TRUE;
            break;
        }
    case WM_DRAWITEM:
        {
            DRAWITEMSTRUCT* lpdis = reinterpret_cast<DRAWITEMSTRUCT*>(lParam);
            if (!lpdis || lpdis->CtlType != ODT_MENU)
                break;

            DWORD id = LOWORD(lpdis->itemID);
            HICON hIcon = NULL;
            if (m_idCmdFirst + id == m_idCmdFolder)
            {
                hIcon = m_hIconFolder;
            }
            else if (m_idCmdFirst + id == m_idCmdLink)
            {
                hIcon = m_hIconLink;
            }
            else
            {
                SHELLNEW_ITEM *pItem = FindItemFromIdOffset(id);
                if (pItem)
                    hIcon = pItem->hIcon;
            }

            if (!hIcon)
                break;

            DrawIconEx(lpdis->hDC,
                       2,
                       lpdis->rcItem.top + (lpdis->rcItem.bottom - lpdis->rcItem.top - 16) / 2,
                       hIcon,
                       16,
                       16,
                       0, NULL, DI_NORMAL);

            if(plResult)
                *plResult = TRUE;
        }
    }

    return S_OK;
}

HRESULT WINAPI
CNewMenu::Initialize(PCIDLIST_ABSOLUTE pidlFolder,
                     IDataObject *pdtobj, HKEY hkeyProgID)
{
    const INT cx = GetSystemMetrics(SM_CXSMICON), cy = GetSystemMetrics(SM_CYSMICON);
    WCHAR wszIconPath[MAX_PATH];
    int icon_idx;

    m_pidlFolder = ILClone(pidlFolder);

    /* Load folder and shortcut icons */
    if (HLM_GetIconW(IDI_SHELL_FOLDER - 1, wszIconPath, _countof(wszIconPath), &icon_idx))
    {
        ::ExtractIconExW(wszIconPath, icon_idx, &m_hIconFolder, NULL, 1);
        m_bCustomIconFolder = TRUE;
    }
    else
    {
        m_hIconFolder = (HICON)LoadImage(shell32_hInstance, MAKEINTRESOURCE(IDI_SHELL_FOLDER), IMAGE_ICON, cx, cy, LR_SHARED);
    }

    if (HLM_GetIconW(IDI_SHELL_SHORTCUT - 1, wszIconPath, _countof(wszIconPath), &icon_idx))
    {
        ::ExtractIconExW(wszIconPath, icon_idx, &m_hIconLink, NULL, 1);
        m_bCustomIconLink = TRUE;
    }
    else
    {
        m_hIconLink = (HICON)LoadImage(shell32_hInstance, MAKEINTRESOURCE(IDI_SHELL_SHORTCUT), IMAGE_ICON, cx, cy, LR_SHARED);
    }

    return S_OK;
}
