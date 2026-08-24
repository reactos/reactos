/*
 * ReactOS Explorer
 *
 * Copyright 2026 ReactOS contributors
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

/*
 * items.cpp - item data management for CWin7StartMenu: enumerating the
 * merged user/common Programs folders, searching, building the right-hand
 * links column and executing items.
 */

#include "../precomp.h"

#define SM7_MAX_SEARCH_RESULTS 40
#define SM7_MAX_MENU_DEPTH     4

/*****************************************************************************
 * lifetime management
 */

VOID CWin7StartMenu::ClearItemsOf(CAtlArray<SM7_ITEM> &arr)
{
    for (SIZE_T i = 0; i < arr.GetCount(); i++)
    {
        if (arr[i].pidl)
            ILFree(arr[i].pidl);
        if (arr[i].psf)
            arr[i].psf->Release();
        if (arr[i].hIcon)
            DestroyIcon(arr[i].hIcon);
    }
    arr.RemoveAll();
}

VOID CWin7StartMenu::ClearItems()
{
    ClearItemsOf(m_LeftItems);
    ClearItemsOf(m_RightItems);
}

/*****************************************************************************
 * left column: programs
 */

static BOOL
SM7NameGreater(LPCWSTR pszA, LPCWSTR pszB)
{
    return CompareStringW(LOCALE_INVARIANT, NORM_IGNORECASE,
                          pszA, -1, pszB, -1) == CSTR_GREATER_THAN;
}

/* Stable insertion sort keeping user-folder entries ahead of common ones */
VOID CWin7StartMenu::SortItemsByName(CAtlArray<SM7_ITEM> &arr, SIZE_T iFirst)
{
    for (SIZE_T i = iFirst + 1; i < arr.GetCount(); i++)
    {
        SM7_ITEM tmp = arr[i];
        SIZE_T j = i;
        while (j > iFirst && SM7NameGreater(arr[j - 1].name, tmp.name))
        {
            arr[j] = arr[j - 1];
            --j;
        }
        arr[j] = tmp;
    }
}

HRESULT CWin7StartMenu::AppendFolderItems(CAtlArray<SM7_ITEM> &arr, IN IShellFolder *psfFolder)
{
    CComPtr<IEnumIDList> pEnum;
    LPITEMIDLIST pidlItem;

    HRESULT hr = psfFolder->EnumObjects(NULL, SHCONTF_FOLDERS | SHCONTF_NONFOLDERS, &pEnum);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;
    if (hr == S_FALSE || !pEnum)
        return S_OK;

    SIZE_T cFirst = arr.GetCount();

    while (pEnum->Next(1, &pidlItem, NULL) == S_OK)
    {
        STRRET str;

        if (SUCCEEDED(psfFolder->GetDisplayNameOf(pidlItem, SHGDN_INFOLDER, &str)))
        {
            WCHAR szName[_countof(((SM7_ITEM *)NULL)->name)];
            StrRetToBufW(&str, pidlItem, szName, _countof(szName));

            SM7_ITEM item;
            lstrcpynW(item.name, szName, _countof(item.name));
            item.pidl = ILClone(pidlItem);
            item.psf = psfFolder;
            psfFolder->AddRef();

            SHFILEINFOW fi = { 0 };
            if (SUCCEEDED(SHGetFileInfoW((LPCWSTR)item.pidl, 0, &fi, sizeof(fi),
                                         SHGFI_PIDL | SHGFI_ICON | SHGFI_LARGEICON)))
            {
                item.hIcon = fi.hIcon;
            }

            arr.Append(item);
        }

        ILFree(pidlItem);
    }

    SortItemsByName(arr, cFirst);
    return S_OK;
}

VOID CWin7StartMenu::AppendSingleItem(IN IShellFolder *psfFolder, IN LPCITEMIDLIST pidlItem,
                                      IN LPCWSTR pszName)
{
    SHFILEINFOW fi = { 0 };

    SM7_ITEM item;
    lstrcpynW(item.name, pszName, _countof(item.name));
    item.pidl = ILClone(pidlItem);
    item.psf = psfFolder;
    psfFolder->AddRef();

    if (SUCCEEDED(SHGetFileInfoW((LPCWSTR)item.pidl, 0, &fi, sizeof(fi),
                                 SHGFI_PIDL | SHGFI_ICON | SHGFI_LARGEICON)))
    {
        item.hIcon = fi.hIcon;
    }

    m_LeftItems.Append(item);
}

HRESULT CWin7StartMenu::SearchProgramsRecursive(IN IShellFolder *psfFolder, IN UINT uDepth)
{
    CComPtr<IEnumIDList> pEnum;
    LPITEMIDLIST pidlItem;

    if (uDepth == 0 || m_LeftItems.GetCount() >= SM7_MAX_SEARCH_RESULTS)
        return S_OK;

    HRESULT hr = psfFolder->EnumObjects(NULL, SHCONTF_FOLDERS | SHCONTF_NONFOLDERS, &pEnum);
    if (FAILED_UNEXPECTEDLY(hr) || hr == S_FALSE || !pEnum)
        return S_OK;

    while (pEnum->Next(1, &pidlItem, NULL) == S_OK &&
           m_LeftItems.GetCount() < SM7_MAX_SEARCH_RESULTS)
    {
        STRRET str;
        ULONG ulAttrs = SFGAO_FOLDER;

        psfFolder->GetAttributesOf(1, (LPCITEMIDLIST *)&pidlItem, &ulAttrs);

        if (SUCCEEDED(psfFolder->GetDisplayNameOf(pidlItem, SHGDN_INFOLDER, &str)))
        {
            WCHAR szName[128];
            StrRetToBufW(&str, pidlItem, szName, _countof(szName));

            if (ulAttrs & SFGAO_FOLDER)
            {
                CComPtr<IShellFolder> psfChild;

                if (StrStrIW(szName, m_szFilter))
                    AppendSingleItem(psfFolder, pidlItem, szName);

                if (SUCCEEDED(psfFolder->BindToObject(pidlItem, NULL,
                                                      IID_PPV_ARG(IShellFolder, &psfChild))))
                {
                    SearchProgramsRecursive(psfChild, uDepth - 1);
                }
            }
            else if (StrStrIW(szName, m_szFilter))
            {
                AppendSingleItem(psfFolder, pidlItem, szName);
            }
        }

        ILFree(pidlItem);
    }

    return S_OK;
}

HRESULT CWin7StartMenu::FillLeftItems()
{
    /* Preserve the selection across refills (e.g. while typing) */
    INT panePrev = m_ActivePane;
    INT idxPrev = m_ActiveIndex;

    ClearItemsOf(m_LeftItems);
    SetHover(PANE_NONE, -1);

    if (m_szFilter[0])
    {
        CComHeapPtr<ITEMIDLIST> pidlPrograms;
        CComPtr<IShellFolder> psfDesktop;
        CComPtr<IShellFolder> psfPrograms;

        HRESULT hr = SHGetFolderLocation(NULL, CSIDL_PROGRAMS, 0, 0, &pidlPrograms);
        if (FAILED_UNEXPECTEDLY(hr))
            return hr;
        hr = SHGetDesktopFolder(&psfDesktop);
        if (FAILED_UNEXPECTEDLY(hr))
            return hr;
        hr = psfDesktop->BindToObject(pidlPrograms, NULL, IID_PPV_ARG(IShellFolder, &psfPrograms));
        if (FAILED_UNEXPECTEDLY(hr))
            return hr;

        SearchProgramsRecursive(psfPrograms, SM7_MAX_MENU_DEPTH);
        SortItemsByName(m_LeftItems, 0);
    }
    else
    {
        CComPtr<IShellFolder> psfUser;
        CComPtr<IShellFolder> psfCommon;
        CComHeapPtr<ITEMIDLIST> pidlUser;
        CComHeapPtr<ITEMIDLIST> pidlCommon;

        if (SUCCEEDED(SHGetFolderLocation(NULL, CSIDL_PROGRAMS, 0, 0, &pidlUser)))
        {
            CComPtr<IShellFolder> psfDesktop;
            if (SUCCEEDED(SHGetDesktopFolder(&psfDesktop)))
                psfDesktop->BindToObject(pidlUser, NULL, IID_PPV_ARG(IShellFolder, &psfUser));
        }

        if (SUCCEEDED(SHGetFolderLocation(NULL, CSIDL_COMMON_PROGRAMS, 0, 0, &pidlCommon)))
        {
            CComPtr<IShellFolder> psfDesktop;
            if (SUCCEEDED(SHGetDesktopFolder(&psfDesktop)))
                psfDesktop->BindToObject(pidlCommon, NULL, IID_PPV_ARG(IShellFolder, &psfCommon));
        }

        if (psfUser)
            AppendFolderItems(m_LeftItems, psfUser);

        SIZE_T cBefore = m_LeftItems.GetCount();
        if (psfCommon)
            AppendFolderItems(m_LeftItems, psfCommon);

        SortItemsByName(m_LeftItems, 0);

        /* Drop duplicates: user entries come first and shadow common ones */
        SIZE_T iStart = cBefore > 1 ? cBefore : 1;
        for (SIZE_T i = iStart; i < m_LeftItems.GetCount(); )
        {
            if (CompareStringW(LOCALE_INVARIANT, NORM_IGNORECASE,
                               m_LeftItems[i - 1].name, -1,
                               m_LeftItems[i].name, -1) == CSTR_EQUAL)
            {
                if (m_LeftItems[i].pidl)
                    ILFree(m_LeftItems[i].pidl);
                if (m_LeftItems[i].psf)
                    m_LeftItems[i].psf->Release();
                if (m_LeftItems[i].hIcon)
                    DestroyIcon(m_LeftItems[i].hIcon);
                m_LeftItems.RemoveAt(i);
            }
            else
            {
                ++i;
            }
        }
    }

    /* Restore a sane selection */
    int count = ItemCount(PANE_LEFT);
    if (panePrev == PANE_LEFT && idxPrev >= 0 && count > 0)
        SetActive(PANE_LEFT, min(idxPrev, count - 1));
    else
        SetActive(count > 0 ? PANE_LEFT : PANE_NONE, count > 0 ? 0 : -1);

    return S_OK;
}

/*****************************************************************************
 * right column: fixed link list
 */

VOID CWin7StartMenu::AppendRightPidlItem(UINT csidl, LPCWSTR pszFallbackName, UINT cmd)
{
    CComHeapPtr<ITEMIDLIST> pidl;
    SHFILEINFOW fi = { 0 };

    SM7_ITEM item;
    item.cmd = cmd;

    if (csidl != (UINT)-1 &&
        SUCCEEDED(SHGetSpecialFolderLocation(NULL, csidl, &pidl)))
    {
        if (SUCCEEDED(SHGetFileInfoW((LPCWSTR)(LPCITEMIDLIST)pidl, 0, &fi, sizeof(fi),
                                     SHGFI_PIDL | SHGFI_ICON | SHGFI_LARGEICON |
                                     SHGFI_DISPLAYNAME)))
        {
            lstrcpynW(item.name, fi.szDisplayName, _countof(item.name));
            item.hIcon = fi.hIcon;
        }
        item.pidl = ILClone((LPCITEMIDLIST)pidl);
    }

    if (!item.name[0] && pszFallbackName)
        lstrcpynW(item.name, pszFallbackName, _countof(item.name));

    m_RightItems.Append(item);
}

HRESULT CWin7StartMenu::FillRightItems()
{
    WCHAR szUser[128];

    ClearItemsOf(m_RightItems);

    if (GetCurrentLoggedOnUserName(szUser, _countof(szUser)))
    {
        CComHeapPtr<ITEMIDLIST> pidl;
        SHFILEINFOW fi = { 0 };

        SM7_ITEM item;
        lstrcpynW(item.name, szUser, _countof(item.name));
        item.bBold = TRUE;

        if (SUCCEEDED(SHGetSpecialFolderLocation(NULL, CSIDL_PROFILE, &pidl)) &&
            SUCCEEDED(SHGetFileInfoW((LPCWSTR)(LPCITEMIDLIST)pidl, 0, &fi, sizeof(fi),
                                     SHGFI_PIDL | SHGFI_ICON | SHGFI_LARGEICON)))
        {
            item.hIcon = fi.hIcon;
            item.pidl = ILClone((LPCITEMIDLIST)pidl);
        }

        m_RightItems.Append(item);

        SM7_ITEM sep;
        sep.bSeparator = TRUE;
        m_RightItems.Append(sep);
    }

    if (!SHRestricted(REST_NOSMMYDOCS))
        AppendRightPidlItem(CSIDL_PERSONAL, L"Documents", SM7_CMD_NONE);
    if (!SHRestricted(REST_NOSMMYPICS))
        AppendRightPidlItem(CSIDL_MYPICTURES, L"Pictures", SM7_CMD_NONE);
    AppendRightPidlItem(CSIDL_MYMUSIC, L"Music", SM7_CMD_NONE);
    AppendRightPidlItem(CSIDL_DRIVES, L"Computer", SM7_CMD_NONE);
    if (!SHRestricted(REST_NOSETFOLDERS) && !SHRestricted(REST_NOCONTROLPANEL))
    {
        AppendRightPidlItem(CSIDL_CONTROLS, L"Control Panel", SM7_CMD_NONE);
        AppendRightPidlItem(CSIDL_PRINTERS, L"Devices and Printers", SM7_CMD_NONE);
    }

    /* Help and Support: no pidl, executed through a tray command */
    {
        SM7_ITEM item;
        lstrcpynW(item.name, L"Help and Support", _countof(item.name));
        item.cmd = SM7_CMD_HELP;
        HMODULE hShell32 = GetModuleHandleW(L"shell32.dll");
        if (hShell32)
        {
            item.hIcon = (HICON)LoadImageW(hShell32, MAKEINTRESOURCEW(23),
                                           IMAGE_ICON, 32, 32, LR_DEFAULTCOLOR);
        }
        m_RightItems.Append(item);
    }

    return S_OK;
}

/*****************************************************************************
 * execution
 */

VOID CWin7StartMenu::ExecuteItem(INT pane, INT index)
{
    if (pane != PANE_LEFT && pane != PANE_RIGHT)
        return;

    CAtlArray<SM7_ITEM> &arr = (pane == PANE_RIGHT) ? m_RightItems : m_LeftItems;
    if ((UINT)index >= arr.GetCount())
        return;

    SM7_ITEM &item = arr[index];

    if (item.cmd != SM7_CMD_NONE)
    {
        PostMessageW(m_Tray->GetHWND(), WM_COMMAND, item.cmd, 0);
        Hide();
        return;
    }

    if (!item.psf || !item.pidl)
        return;

    ULONG ulAttrs = SFGAO_FOLDER;
    item.psf->GetAttributesOf(1, (LPCITEMIDLIST *)&item.pidl, &ulAttrs);
    if (ulAttrs & SFGAO_FOLDER)
    {
        TrackFolderSubmenu(item.psf, item.pidl, pane, index);
    }
    else
    {
        ShellExecutePidl(item.pidl);
        Hide();
    }
}

VOID CWin7StartMenu::ShellExecutePidl(LPCITEMIDLIST pidl)
{
    SHELLEXECUTEINFOW sei = { sizeof(sei) };

    sei.fMask = SEE_MASK_IDLIST | SEE_MASK_NOASYNC;
    sei.hwnd = m_Tray->GetHWND();
    sei.nShow = SW_SHOWNORMAL;
    sei.lpIDList = (LPVOID)pidl;
    ShellExecuteExW(&sei);
}

struct CWin7StartMenu::FOLDERMENU_CTX
{
    CAtlArray<SM7_ITEM> items;
    UINT idNext;
};

VOID CWin7StartMenu::AppendFolderMenu(FOLDERMENU_CTX &ctx, HMENU hmenu,
                                      IN IShellFolder *psfFolder, UINT uDepth)
{
    CComPtr<IEnumIDList> pEnum;
    LPITEMIDLIST pidlItem;

    if (uDepth == 0)
        return;
    if (FAILED(psfFolder->EnumObjects(NULL, SHCONTF_FOLDERS | SHCONTF_NONFOLDERS, &pEnum)) ||
        !pEnum)
        return;

    while (pEnum->Next(1, &pidlItem, NULL) == S_OK)
    {
        STRRET str;

        if (FAILED(psfFolder->GetDisplayNameOf(pidlItem, SHGDN_INFOLDER, &str)))
        {
            ILFree(pidlItem);
            continue;
        }

        WCHAR szName[128];
        StrRetToBufW(&str, pidlItem, szName, _countof(szName));

        MENUITEMINFOW mii = { sizeof(mii) };
        mii.fMask = MIIM_ID | MIIM_STRING | MIIM_STATE;
        mii.wID = ctx.idNext++;
        mii.dwTypeData = szName;
        mii.fState = MFS_ENABLED;

        SM7_ITEM item;
        lstrcpynW(item.name, szName, _countof(item.name));
        item.pidl = ILClone(pidlItem);
        item.psf = psfFolder;
        psfFolder->AddRef();
        ctx.items.Append(item);

        ULONG ulAttrs = SFGAO_FOLDER;
        psfFolder->GetAttributesOf(1, (LPCITEMIDLIST *)&pidlItem, &ulAttrs);
        if (ulAttrs & SFGAO_FOLDER)
        {
            CComPtr<IShellFolder> psfChild;
            if (SUCCEEDED(psfFolder->BindToObject(pidlItem, NULL,
                                                  IID_PPV_ARG(IShellFolder, &psfChild))))
            {
                HMENU hsub = CreatePopupMenu();
                AppendFolderMenu(ctx, hsub, psfChild, uDepth - 1);
                mii.fMask |= MIIM_SUBMENU;
                mii.hSubMenu = hsub;
                if (GetMenuItemCount(hsub) == 0)
                    mii.fState |= MFS_DISABLED;
            }
        }

        InsertMenuItemW(hmenu, GetMenuItemCount(hmenu), TRUE, &mii);
        ILFree(pidlItem);
    }
}

/* Program folders are opened as native popup menus for now; a fully styled
   tree view inside the left column is future work. */
VOID CWin7StartMenu::TrackFolderSubmenu(IN IShellFolder *psf, IN LPCITEMIDLIST pidlFolder,
                                        INT pane, INT index)
{
    CComPtr<IShellFolder> psfFolder;

    if (FAILED(psf->BindToObject(pidlFolder, NULL, IID_PPV_ARG(IShellFolder, &psfFolder))))
        return;

    FOLDERMENU_CTX ctx;
    ctx.idNext = 1;

    HMENU hmenu = CreatePopupMenu();
    if (!hmenu)
        return;

    AppendFolderMenu(ctx, hmenu, psfFolder, SM7_MAX_MENU_DEPTH);
    if (GetMenuItemCount(hmenu) == 0)
    {
        DestroyMenu(hmenu);
        ClearItemsOf(ctx.items);
        return;
    }

    RECT rcItem = ItemRect(pane, index);
    POINT pt = { rcItem.right, rcItem.top };
    ClientToScreen(m_hWnd, &pt);

    INT iCmd = TrackPopupMenuEx(hmenu,
                                TPM_RETURNCMD | TPM_LEFTBUTTON | TPM_RIGHTALIGN |
                                TPM_BOTTOMALIGN,
                                pt.x, pt.y, m_hWnd, NULL);

    if (iCmd > 0 && (UINT)(iCmd - 1) < ctx.items.GetCount())
    {
        SM7_ITEM &sel = ctx.items[iCmd - 1];
        ULONG ulAttrs = SFGAO_FOLDER;
        if (sel.psf && sel.pidl &&
            SUCCEEDED(sel.psf->GetAttributesOf(1, (LPCITEMIDLIST *)&sel.pidl, &ulAttrs)) &&
            !(ulAttrs & SFGAO_FOLDER))
        {
            ShellExecutePidl(sel.pidl);
            Hide();
        }
    }

    ClearItemsOf(ctx.items);
    DestroyMenu(hmenu);
}
