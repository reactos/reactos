/*
 * provides new shell item service
 *
 * Copyright 2007 Johannes Anderwald (janderwald@reactos.org)
 * Copyright 2009 Andrew Hill
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

#include <precomp.h>

WINE_DEFAULT_DEBUG_CHANNEL(shell);

CNewMenu::CNewMenu()
{
    m_wszPath = NULL;
    m_pShellItems = NULL;
    m_pSite = NULL;
}

CNewMenu::~CNewMenu()
{
    UnloadShellItems();
    if (m_hSubMenu)
        CleanupMenu();
}

void CNewMenu::CleanupMenu()
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

void CNewMenu::UnloadItem(SHELLNEW_ITEM *pItem)
{
    // bail if the item is clearly invalid
    if (NULL == pItem)
        return;

    if (NULL != pItem->pwszTarget)
        free(pItem->pwszTarget);

    free(pItem->pwszDesc);
    free(pItem->pwszIcon);
    free(pItem->pwszExt);

    HeapFree(GetProcessHeap(), 0, pItem);
}

void CNewMenu::UnloadShellItems()
{
    SHELLNEW_ITEM *pCurItem;

    while (m_pShellItems)
    {
        pCurItem = m_pShellItems;
        m_pShellItems = m_pShellItems->pNext;

        UnloadItem(pCurItem);
    }
}

BOOL CNewMenu::GetKeyDescription(LPCWSTR pwszExt, LPWSTR pwszResult)
{
    HKEY hKey;
    DWORD cbDesc;
    WCHAR wszDesc[100];
    LONG Result;

    TRACE("GetKeyDescription: keyname %s\n", debugstr_w(pwszExt));

    if (RegOpenKeyExW(HKEY_CLASSES_ROOT, pwszExt, 0, KEY_READ, &hKey) != ERROR_SUCCESS)
        return FALSE;

    /* Get user friendly name */
    if (RegLoadMUIStringW(hKey, L"\\FriendlyTypeName", pwszResult, MAX_PATH, &cbDesc, 0, NULL) == ERROR_SUCCESS)
    {
        TRACE("result %s\n", debugstr_w(pwszResult));
        RegCloseKey(hKey);
        return TRUE;
    }

    /* Fetch default value */
    cbDesc = sizeof(wszDesc);
    Result = RegGetValueW(hKey, NULL, L"", RRF_RT_REG_SZ, NULL, wszDesc, &cbDesc);
    if(Result == ERROR_SUCCESS)
    {
        if (wcscmp(pwszExt, wszDesc) != 0)
        {
            /* recurse for to a linked key */
            if (!GetKeyDescription(wszDesc, pwszResult))
            {
                /* use description */
                wcscpy(pwszResult, wszDesc);
            }
        }
        else
        {
            /* use default value as description */
            wcscpy(pwszResult, wszDesc);
        }
    }
    else
    {
        /* registry key w/o default key?? */
        ERR("RegGetValue failed with %x\n", Result);
        wcscpy(pwszResult, pwszExt);
    }

    RegCloseKey(hKey);
    return TRUE;
}

CNewMenu::SHELLNEW_ITEM *CNewMenu::LoadItem(LPCWSTR pwszExt)
{
    HKEY hKey;
    DWORD dwIndex = 0;
    WCHAR wszBuf[MAX_PATH];
    WCHAR wszCommand[MAX_PATH];
    WCHAR wszDesc[MAX_PATH] = L"";
    WCHAR wszIcon[MAX_PATH] = L"";
    DWORD cchName, cbCommand;
    SHELLNEW_ITEM *pNewItem = NULL;

    StringCbPrintfW(wszBuf, sizeof(wszBuf), L"%s\\ShellNew", pwszExt);

    TRACE("LoadItem Keyname %s Name %s\n", debugstr_w(pwszExt), debugstr_w(wszBuf));

    if (RegOpenKeyExW(HKEY_CLASSES_ROOT, wszBuf, 0, KEY_READ, &hKey) != ERROR_SUCCESS)
    {
        TRACE("Failed to open key\n");
        return NULL;
    }

    /* Find first valid value */
    while (TRUE)
    {
        cchName = _countof(wszBuf);
        cbCommand = sizeof(wszCommand);
        if (RegEnumValueW(hKey, dwIndex++, wszBuf, &cchName, NULL, NULL, (LPBYTE)wszCommand, &cbCommand) != ERROR_SUCCESS)
            break;

        SHELLNEW_TYPE Type = SHELLNEW_TYPE_INVALID;
        LPWSTR pwszTarget = wszCommand;

        TRACE("wszBuf %s wszCommand %s\n", debugstr_w(wszBuf), debugstr_w(wszCommand));

        /* Handle different types */
        if (!wcsicmp(wszBuf, L"Command"))
            Type = SHELLNEW_TYPE_COMMAND;
        else if (!wcsicmp(wszBuf, L"Data"))
            Type = SHELLNEW_TYPE_DATA;
        else if (!wcsicmp(wszBuf, L"FileName"))
            Type = SHELLNEW_TYPE_FILENAME;
        else if (!wcsicmp(wszBuf, L"NullFile"))
        {
            Type = SHELLNEW_TYPE_NULLFILE;
            pwszTarget = NULL;
        }

        /* Create new item */
        if (Type != SHELLNEW_TYPE_INVALID)
        {
            pNewItem = (SHELLNEW_ITEM *)HeapAlloc(GetProcessHeap(), 0, sizeof(SHELLNEW_ITEM));
            if (!pNewItem)
                break;

            pNewItem->Type = Type;
            if (pwszTarget)
                pNewItem->pwszTarget = _wcsdup(pwszTarget);
            else
                pNewItem->pwszTarget = NULL;

            GetKeyDescription(pwszExt, wszDesc);
            pNewItem->pwszDesc = _wcsdup(wszDesc);
            pNewItem->pwszIcon = _wcsdup(wszIcon);
            pNewItem->pwszExt = _wcsdup(pwszExt);
            pNewItem->pNext = NULL;
            break;
        }
    }

    RegCloseKey(hKey);
    return pNewItem;
}

BOOL
CNewMenu::LoadShellNewItems()
{
    DWORD dwIndex = 0;
    WCHAR wszName[MAX_PATH];
    SHELLNEW_ITEM *pNewItem;
    SHELLNEW_ITEM *pCurItem = NULL;

    /* If there are any unload them */
    UnloadShellItems();

    /* Enumerate all extesions */
    while (RegEnumKeyW(HKEY_CLASSES_ROOT, dwIndex++, wszName, _countof(wszName)) == ERROR_SUCCESS)
    {
        if (wszName[0] != '.' || wcsicmp(wszName, L".lnk") == 0)
            continue;

        pNewItem = LoadItem(wszName);
        if (pNewItem)
        {
            if (!wcsicmp(pNewItem->pwszExt, L".lnk"))
            {
                /* FIXME: Should we load them?
                   Add lnk handlers to begin of the list */
                if (m_pShellItems)
                {
                    pNewItem->pNext = m_pShellItems;
                    m_pShellItems = pNewItem;
                }
                else
                    m_pShellItems = pCurItem = pNewItem;
            }
            else
            {
                /* Add at the end of list */
                if (pCurItem)
                {
                    pCurItem->pNext = pNewItem;
                    pCurItem = pNewItem;
                }
                else
                    pCurItem = m_pShellItems = pNewItem;
            }
        }
    }

    if (m_pShellItems == NULL)
        return FALSE;
    else
        return TRUE;
}

static HBITMAP IconToBitmap(HICON hIcon)
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

UINT
CNewMenu::InsertShellNewItems(HMENU hMenu, UINT idCmdFirst, UINT Pos)
{
    MENUITEMINFOW mii;
    WCHAR wszBuf[256];
    HICON hIcon;
    UINT idCmd = idCmdFirst;

    if (m_pShellItems == NULL)
    {
        if (!LoadShellNewItems())
            return 0;
    }

    ZeroMemory(&mii, sizeof(mii));
    mii.cbSize = sizeof(mii);

    /* Insert new folder action */
    if (!LoadStringW(shell32_hInstance, FCIDM_SHVIEW_NEWFOLDER, wszBuf, _countof(wszBuf)))
        wszBuf[0] = 0;
    mii.fMask = MIIM_ID | MIIM_TYPE | MIIM_STATE | MIIM_DATA;
    mii.fType = MFT_STRING;
    mii.dwTypeData = wszBuf;
    mii.cch = wcslen(mii.dwTypeData);
    mii.wID = idCmd;
    hIcon = (HICON)LoadImage(shell32_hInstance, MAKEINTRESOURCE(IDI_SHELL_FOLDER), IMAGE_ICON, 0, 0, 0);
    if (hIcon)
    {
        mii.fMask |= MIIM_CHECKMARKS;
        mii.hbmpChecked = mii.hbmpUnchecked = IconToBitmap(hIcon);
    }
    if (InsertMenuItemW(hMenu, Pos++, TRUE, &mii))
        ++idCmd;

    /* Insert new shortcut action */
    if (!LoadStringW(shell32_hInstance, FCIDM_SHVIEW_NEWLINK, wszBuf, _countof(wszBuf)))
        wszBuf[0] = 0;
    mii.dwTypeData = wszBuf;
    mii.cch = wcslen(mii.dwTypeData);
    mii.wID = idCmdFirst++;
    hIcon = (HICON)LoadImage(shell32_hInstance, MAKEINTRESOURCE(IDI_SHELL_SHORTCUT), IMAGE_ICON, 0, 0, 0);
    if (hIcon)
    {
        mii.fMask |= MIIM_CHECKMARKS;
        mii.hbmpChecked = mii.hbmpUnchecked = IconToBitmap(hIcon);
    }
    if (InsertMenuItemW(hMenu, Pos++, TRUE, &mii))
        ++idCmd;

    /* Insert seperator for custom new action */
    mii.fMask = MIIM_TYPE | MIIM_ID;
    mii.fType = MFT_SEPARATOR;
    mii.wID = -1;
    InsertMenuItemW(hMenu, Pos++, TRUE, &mii);

    /* Insert rest of items */
    mii.fMask = MIIM_ID | MIIM_TYPE | MIIM_STATE | MIIM_DATA;
    /*
     * FIXME:
     * implement loading of icons
     * and using MFT_OWNERDRAWN
     */
    mii.fType = MFT_STRING;
    mii.fState = MFS_ENABLED;

    SHELLNEW_ITEM *pCurItem = m_pShellItems;
    while (pCurItem)
    {
        TRACE("szDesc %s\n", debugstr_w(pCurItem->pwszDesc));
        mii.dwTypeData = pCurItem->pwszDesc;
        mii.cch = wcslen(mii.dwTypeData);
        mii.wID = idCmd;
        if (InsertMenuItemW(hMenu, Pos++, TRUE, &mii))
            ++idCmd;
        pCurItem = pCurItem->pNext;
    }

    return idCmd - idCmdFirst;
}

HRESULT
CNewMenu::DoShellNewCmd(LPCMINVOKECOMMANDINFO lpcmi, IShellView *psv)
{
    SHELLNEW_ITEM *pCurItem = m_pShellItems;
    LPITEMIDLIST pidl;
    STRRET strTemp;
    WCHAR wszBuf[MAX_PATH];
    WCHAR wszPath[MAX_PATH];
    UINT i, idCmd = LOWORD(lpcmi->lpVerb);
    CComPtr<IFolderView> pFolderView;
    CComPtr<IShellFolder> pParentFolder;
    CComPtr<IPersistFolder3> psf;
    HRESULT hr;

    /* Find shell new item */
    for (i = 1; pCurItem; ++i)
    {
        if (i == idCmd)
            break;

        pCurItem = pCurItem->pNext;
    }

    if (!pCurItem)
        return E_UNEXPECTED;
    //if (m_pSite == NULL)
    //    return E_FAIL;

    /* Get current folder */
    hr = IUnknown_QueryService(psv, SID_IFolderView, IID_IFolderView, (void **)&pFolderView);
    if (FAILED(hr))
        return hr;

    hr = pFolderView->GetFolder(IID_IShellFolder, (void **)&pParentFolder);
    if (FAILED(hr))
        return hr;

    if (pParentFolder->QueryInterface(IID_IPersistFolder2, (LPVOID*)&psf) != S_OK)
    {
        ERR("Failed to get interface IID_IPersistFolder2\n");
        return E_FAIL;
    }

    if (psf->GetCurFolder(&pidl) != S_OK)
    {
        ERR("IPersistFolder2_GetCurFolder failed\n");
        return E_FAIL;
    }

    /* Get folder path */
    if (pParentFolder == NULL || pParentFolder->GetDisplayNameOf(pidl, SHGDN_FORPARSING, &strTemp) != S_OK)
    {
        ERR("IShellFolder_GetDisplayNameOf failed\n");
        return E_FAIL;
    }
    StrRetToBufW(&strTemp, pidl, wszPath, _countof(wszPath));

    switch (pCurItem->Type)
    {
        case SHELLNEW_TYPE_COMMAND:
        {
            LPWSTR Ptr, pwszCmd;
            WCHAR wszTemp[MAX_PATH];
            STARTUPINFOW si;
            PROCESS_INFORMATION pi;

            if (!ExpandEnvironmentStringsW(pCurItem->pwszTarget, wszBuf, MAX_PATH))
            {
                TRACE("ExpandEnvironmentStrings failed\n");
                break;
            }

            /* Expand command parameter, FIXME: there can be more modifiers */
            Ptr = wcsstr(wszBuf, L"%1");
            if (Ptr)
            {
                Ptr[1] = 's';
                StringCbPrintfW(wszTemp, sizeof(wszTemp), wszBuf, wszPath);
                pwszCmd = wszTemp;
            }
            else
                pwszCmd = wszBuf;

            /* Create process */
            ZeroMemory(&si, sizeof(si));
            si.cb = sizeof(si);
            if (CreateProcessW(NULL, pwszCmd, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
            {
                CloseHandle(pi.hProcess);
                CloseHandle(pi.hThread);
            } else
                ERR("Failed to create process\n");
            break;
        }
        case SHELLNEW_TYPE_DATA:
        case SHELLNEW_TYPE_FILENAME:
        case SHELLNEW_TYPE_NULLFILE:
        {
            BOOL bSuccess = TRUE;
            LPWSTR pwszFilename = NULL;
            size_t cchFilenameMax = 0;

            /* Build new file name */
            LoadStringW(shell32_hInstance, FCIDM_SHVIEW_NEW, wszBuf, _countof(wszBuf));
            StringCchCatExW(wszPath, _countof(wszPath), L"\\", &pwszFilename, &cchFilenameMax, 0);
            StringCchPrintfW(pwszFilename, cchFilenameMax, L"%s %s%s", wszBuf, pCurItem->pwszDesc, pCurItem->pwszExt);

            /* Find unique name */
            for (i = 2; PathFileExistsW(wszPath); ++i)
            {
                StringCchPrintfW(pwszFilename, cchFilenameMax, L"%s %s (%u)%s", wszBuf, pCurItem->pwszDesc, i, pCurItem->pwszExt);
                TRACE("New Filename %ls\n", pwszFilename);
            }

            if (pCurItem->Type == SHELLNEW_TYPE_DATA || pCurItem->Type == SHELLNEW_TYPE_NULLFILE)
            {
                /* Create new file */
                HANDLE hFile = CreateFileW(wszPath, GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
                if (hFile != INVALID_HANDLE_VALUE)
                {
                    if (pCurItem->Type == SHELLNEW_TYPE_DATA)
                    {
                        /* Write a content */
                        CHAR szTemp[256];
                        DWORD cbWritten, cbTemp = WideCharToMultiByte(CP_ACP, 0, pCurItem->pwszTarget, -1, szTemp, _countof(szTemp), NULL, NULL);
                        if (cbTemp)
                            WriteFile(hFile, (LPCVOID)szTemp, cbTemp, &cbWritten, NULL);
                        else
                            ERR("WideCharToMultiByte failed\n");
                    }

                    /* Close file now */
                    CloseHandle(hFile);
                }
                else bSuccess = FALSE;
            }
            else if (pCurItem->Type == SHELLNEW_TYPE_FILENAME)
            {
                /* Copy file */
                bSuccess = CopyFileW(pCurItem->pwszTarget, wszPath, FALSE);
            }

            /* Show message if we failed */
            if (bSuccess)
            {
                TRACE("Notifying fs %s\n", debugstr_w(wszPath));
                SHChangeNotify(SHCNE_CREATE, SHCNF_PATHW, (LPCVOID)wszPath, NULL);
            }
            else
            {
                StringCbPrintfW(wszBuf, sizeof(wszBuf), L"Cannot create file: %s", pwszFilename);
                MessageBoxW(NULL, wszBuf, L"Cannot create file", MB_OK|MB_ICONERROR); // FIXME
            }
            break;
        }
        case SHELLNEW_TYPE_INVALID:
            ERR("Invalid type\n");
            break;
    }
    return S_OK;
}

void CNewMenu::CreateNewFolder(IShellView *psv)
{
    WCHAR wszName[MAX_PATH];
    CComPtr<ISFHelper> psfhlp; 
    CComPtr<IFolderView> pFolderView;
    CComPtr<IShellFolder> pParentFolder;
    HRESULT hr;

    //if (m_pSite == NULL)
    //    return;

    /* Get current folder */
    hr = IUnknown_QueryService(psv, SID_IFolderView, IID_IFolderView, (void **)&pFolderView);
    if (FAILED(hr))
        return;

    hr = pFolderView->GetFolder(IID_IShellFolder, (void **)&pParentFolder);
    if (FAILED(hr))
        return;

    hr = pParentFolder->QueryInterface(IID_ISFHelper, (LPVOID*)&psfhlp);
    if (SUCCEEDED(hr))
    {
        LPITEMIDLIST pidl;

        /* Get unique name and create a folder */
        if (psfhlp->GetUniqueName(wszName, _countof(wszName)) != S_OK)
            return;
        if (psfhlp->AddFolder(0, wszName, &pidl) != S_OK)
        {
            WCHAR wszBuf[256];
            StringCbPrintfW(wszBuf, sizeof(wszBuf), L"Cannot create folder: %s", wszName);
            MessageBoxW(NULL, wszBuf, L"Cannot create folder", MB_OK|MB_ICONERROR);
            return;
        }

        /* Do a labeledit */
        psv->Refresh();
        psv->SelectItem(pidl, SVSI_DESELECTOTHERS | SVSI_EDIT | SVSI_ENSUREVISIBLE |
                              SVSI_FOCUSED | SVSI_SELECT);

        SHFree(pidl);
    }
}

HRESULT STDMETHODCALLTYPE CNewMenu::SetSite(IUnknown *pUnkSite)
{
    m_pSite = pUnkSite;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CNewMenu::GetSite(REFIID riid, void **ppvSite)
{
    if (ppvSite == NULL)
        return E_POINTER;
    *ppvSite = m_pSite;
    if (m_pSite != NULL)
        m_pSite->AddRef();
    return S_OK;
}

HRESULT
WINAPI
CNewMenu::QueryContextMenu(HMENU hMenu,
                           UINT indexMenu,
                           UINT idCmdFirst,
                           UINT idCmdLast,
                           UINT uFlags)
{
    WCHAR wszNew[200];
    MENUITEMINFOW mii;
    UINT cItems = 0;

    TRACE("%p %p %u %u %u %u\n", this,
          hMenu, indexMenu, idCmdFirst, idCmdLast, uFlags);

    if (!LoadStringW(shell32_hInstance, FCIDM_SHVIEW_NEW, wszNew, _countof(wszNew)))
        return E_FAIL;

    m_hSubMenu = CreateMenu();
    memset(&mii, 0, sizeof(mii));
    mii.cbSize = sizeof(mii);
    mii.fMask = MIIM_TYPE | MIIM_ID | MIIM_STATE;
    mii.fType = MFT_STRING;
    mii.wID = -1;
    mii.dwTypeData = wszNew;
    mii.cch = wcslen(mii.dwTypeData);
    mii.fState = MFS_ENABLED;

    if (m_hSubMenu)
    {
        cItems = InsertShellNewItems(m_hSubMenu, idCmdFirst, 0);
        mii.fMask |= MIIM_SUBMENU;
        mii.hSubMenu = m_hSubMenu;
    }

    if (!InsertMenuItemW(hMenu, indexMenu, TRUE, &mii))
        return E_FAIL;

    return MAKE_HRESULT(SEVERITY_SUCCESS, 0, cItems);
}

HRESULT
WINAPI
CNewMenu::InvokeCommand(LPCMINVOKECOMMANDINFO lpici)
{
    LPSHELLBROWSER lpSB;
    LPSHELLVIEW lpSV = NULL;
    HRESULT hr;

    lpSB = (LPSHELLBROWSER)SendMessageA(lpici->hwnd, CWM_GETISHELLBROWSER, 0, 0);
    if (lpSB)
        lpSB->QueryActiveShellView(&lpSV);

    if (LOWORD(lpici->lpVerb) == 0)
    {
        CreateNewFolder(lpSV);
        return S_OK;
    }

    hr = DoShellNewCmd(lpici, lpSV);
    if (SUCCEEDED(hr) && lpSV)
        lpSV->Refresh();

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
CNewMenu::HandleMenuMsg(UINT uMsg,
                        WPARAM wParam,
                        LPARAM lParam)
{
    UNIMPLEMENTED;
    return S_OK;
}

HRESULT WINAPI
CNewMenu::Initialize(LPCITEMIDLIST pidlFolder,
                     IDataObject *pdtobj, HKEY hkeyProgID)
{
    return S_OK;
}
