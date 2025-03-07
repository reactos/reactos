/*
 * provides SendTo shell item service
 *
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

CSendToMenu::CSendToMenu()
    : m_hSubMenu(NULL)
    , m_pItems(NULL)
    , m_idCmdFirst(0)
{
    HRESULT hr = SHGetDesktopFolder(&m_pDesktop);
    if (FAILED(hr))
    {
        ERR("SHGetDesktopFolder: %08lX\n", hr);
    }

    GetSpecialFolder(NULL, &m_pSendTo, CSIDL_SENDTO);
}

CSendToMenu::~CSendToMenu()
{
    UnloadAllItems();

    if (m_hSubMenu)
    {
        DestroyMenu(m_hSubMenu);
        m_hSubMenu = NULL;
    }
}

HRESULT CSendToMenu::DoDrop(IDataObject *pDataObject, IDropTarget *pDropTarget)
{
    DWORD dwEffect = DROPEFFECT_MOVE | DROPEFFECT_COPY | DROPEFFECT_LINK;

    BOOL bShift = (GetAsyncKeyState(VK_SHIFT) < 0);
    BOOL bCtrl = (GetAsyncKeyState(VK_CONTROL) < 0);

    // THIS CODE IS NOT HUMAN-FRIENDLY. SORRY.
    // (We have to translate a SendTo action to a Drop action)
    DWORD dwKeyState = MK_LBUTTON;
    if (bShift && bCtrl)
        dwKeyState |= MK_SHIFT | MK_CONTROL;
    else if (!bShift)
        dwKeyState |= MK_CONTROL;
    if (bCtrl)
        dwKeyState |= MK_SHIFT;

    POINTL ptl = { 0, 0 };
    HRESULT hr = pDropTarget->DragEnter(pDataObject, dwKeyState, ptl, &dwEffect);
    if (FAILED_UNEXPECTEDLY(hr))
    {
        pDropTarget->DragLeave();
        return hr;
    }

    if (dwEffect == DROPEFFECT_NONE)
    {
        ERR("DROPEFFECT_NONE\n");
        pDropTarget->DragLeave();
        return E_FAIL;
    }

    // THIS CODE IS NOT HUMAN-FRIENDLY. SORRY.
    // (We have to translate a SendTo action to a Drop action)
    if (bShift && bCtrl)
        dwEffect = DROPEFFECT_LINK;
    else if (!bShift)
        dwEffect = DROPEFFECT_MOVE;
    else
        dwEffect = DROPEFFECT_COPY;

    hr = pDropTarget->Drop(pDataObject, dwKeyState, ptl, &dwEffect);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    return hr;
}

// get an IShellFolder from CSIDL
HRESULT
CSendToMenu::GetSpecialFolder(HWND hwnd, IShellFolder **ppFolder,
                              int csidl, PIDLIST_ABSOLUTE *ppidl)
{
    if (!ppFolder)
        return E_POINTER;
    *ppFolder = NULL;

    if (ppidl)
        *ppidl = NULL;

    CComHeapPtr<ITEMIDLIST_ABSOLUTE> pidl;
    HRESULT hr = SHGetSpecialFolderLocation(hwnd, csidl, &pidl);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    IShellFolder *pFolder = NULL;
    hr = m_pDesktop->BindToObject(pidl, NULL, IID_PPV_ARG(IShellFolder, &pFolder));

    if (ppidl)
        *ppidl = pidl.Detach();

    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    *ppFolder = pFolder;
    return hr;
}

// get a UI object from PIDL
HRESULT CSendToMenu::GetUIObjectFromPidl(HWND hwnd, PIDLIST_ABSOLUTE pidl,
                                         REFIID riid, LPVOID *ppvOut)
{
    *ppvOut = NULL;
    HRESULT hr = SHELL_GetUIObjectOfAbsoluteItem(hwnd, pidl, riid, ppvOut);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    return hr;
}

void CSendToMenu::UnloadAllItems()
{
    SENDTO_ITEM *pItems = m_pItems;
    m_pItems = NULL;
    while (pItems)
    {
        SENDTO_ITEM *pCurItem = pItems;
        pItems = pItems->pNext;
        delete pCurItem;
    }
}

HRESULT CSendToMenu::LoadAllItems(HWND hwnd)
{
    UnloadAllItems();

    CComHeapPtr<ITEMIDLIST_ABSOLUTE> pidlSendTo;

    m_pSendTo.Release();
    HRESULT hr = GetSpecialFolder(hwnd, &m_pSendTo, CSIDL_SENDTO, &pidlSendTo);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    CComPtr<IEnumIDList> pEnumIDList;
    hr = m_pSendTo->EnumObjects(hwnd,
                                SHCONTF_FOLDERS | SHCONTF_NONFOLDERS,
                                &pEnumIDList);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    hr = S_OK;
    PITEMID_CHILD child;
    while (pEnumIDList->Next(1, &child, NULL) == S_OK)
    {
        CComHeapPtr<ITEMID_CHILD> pidlChild(child);

        STRRET strret;
        hr = m_pSendTo->GetDisplayNameOf(pidlChild, SHGDN_NORMAL, &strret);
        if (FAILED_UNEXPECTEDLY(hr))
            continue;

        CComHeapPtr<WCHAR> pszText;
        hr = StrRetToStrW(&strret, pidlChild, &pszText);
        if (FAILED_UNEXPECTEDLY(hr))
            continue;

        CComHeapPtr<ITEMIDLIST_ABSOLUTE> pidlAbsolute;
        pidlAbsolute.Attach(ILCombine(pidlSendTo, pidlChild));

        SHFILEINFOW fi = { NULL };
        const UINT uFlags = SHGFI_PIDL | SHGFI_TYPENAME |
                            SHGFI_ICON | SHGFI_SMALLICON;
        SHGetFileInfoW(reinterpret_cast<LPWSTR>(static_cast<PIDLIST_ABSOLUTE>(pidlAbsolute)), 0,
                       &fi, sizeof(fi), uFlags);

        SENDTO_ITEM *pNewItem =
            new SENDTO_ITEM(pidlChild.Detach(), pszText.Detach(), fi.hIcon);
        if (m_pItems)
        {
            pNewItem->pNext = m_pItems;
        }
        m_pItems = pNewItem;
    }

    return hr;
}

UINT CSendToMenu::InsertSendToItems(HMENU hMenu, UINT idCmdFirst, UINT Pos)
{
    if (m_pItems == NULL)
    {
        HRESULT hr = LoadAllItems(NULL);
        if (FAILED_UNEXPECTEDLY(hr))
            return 0;
    }

    m_idCmdFirst = idCmdFirst;

    UINT idCmd = idCmdFirst;
    for (SENDTO_ITEM *pCurItem = m_pItems; pCurItem; pCurItem = pCurItem->pNext)
    {
        const UINT uFlags = MF_BYPOSITION | MF_STRING | MF_ENABLED;
        if (InsertMenuW(hMenu, Pos, uFlags, idCmd, pCurItem->pszText))
        {
            MENUITEMINFOW mii;
            mii.cbSize = sizeof(mii);
            mii.fMask = MIIM_DATA | MIIM_BITMAP;
            mii.dwItemData = reinterpret_cast<ULONG_PTR>(pCurItem);
            mii.hbmpItem = HBMMENU_CALLBACK;
            SetMenuItemInfoW(hMenu, idCmd, FALSE, &mii);
            ++idCmd;

            // successful
        }
    }

    if (idCmd == idCmdFirst)
    {
        CStringW strNone(MAKEINTRESOURCEW(IDS_NONE));
        AppendMenuW(hMenu, MF_GRAYED | MF_DISABLED | MF_STRING, idCmd, strNone);
        ++idCmd;
    }

    return idCmd - idCmdFirst;
}

CSendToMenu::SENDTO_ITEM *CSendToMenu::FindItemFromIdOffset(UINT IdOffset)
{
    UINT idCmd = m_idCmdFirst + IdOffset;

    MENUITEMINFOW mii = { sizeof(mii) };
    mii.fMask = MIIM_DATA;
    if (GetMenuItemInfoW(m_hSubMenu, idCmd, FALSE, &mii))
        return reinterpret_cast<SENDTO_ITEM *>(mii.dwItemData);

    ERR("GetMenuItemInfoW: %ld\n", GetLastError());
    return NULL;
}

HRESULT CSendToMenu::DoSendToItem(SENDTO_ITEM *pItem, LPCMINVOKECOMMANDINFO lpici)
{
    if (!m_pDataObject)
    {
        ERR("!m_pDataObject\n");
        return E_FAIL;
    }

    HRESULT hr;
    CComPtr<IDropTarget> pDropTarget;
    hr = m_pSendTo->GetUIObjectOf(NULL, 1, &pItem->pidlChild, IID_IDropTarget,
                                  NULL, (LPVOID *)&pDropTarget);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    hr = DoDrop(m_pDataObject, pDropTarget);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    return hr;
}

STDMETHODIMP
CSendToMenu::QueryContextMenu(HMENU hMenu,
                              UINT indexMenu,
                              UINT idCmdFirst,
                              UINT idCmdLast,
                              UINT uFlags)
{
    TRACE("%p %p %u %u %u %u\n", this,
          hMenu, indexMenu, idCmdFirst, idCmdLast, uFlags);

    if (uFlags & (CMF_NOVERBS | CMF_VERBSONLY))
        return MAKE_HRESULT(SEVERITY_SUCCESS, 0, idCmdFirst);

    HMENU hSubMenu = CreateMenu();
    if (!hSubMenu)
    {
        ERR("CreateMenu: %ld\n", GetLastError());
        return E_FAIL;
    }

    UINT cItems = InsertSendToItems(hSubMenu, idCmdFirst, 0);

    CStringW strSendTo(MAKEINTRESOURCEW(IDS_SENDTO_MENU));

    MENUITEMINFOW mii = { sizeof(mii) };
    mii.fMask = MIIM_TYPE | MIIM_ID | MIIM_STATE | MIIM_SUBMENU;
    mii.fType = MFT_STRING;
    mii.wID = -1;
    mii.dwTypeData = strSendTo.GetBuffer();
    mii.cch = wcslen(mii.dwTypeData);
    mii.fState = MFS_ENABLED;
    mii.hSubMenu = hSubMenu;
    if (!InsertMenuItemW(hMenu, indexMenu, TRUE, &mii))
    {
        ERR("InsertMenuItemW: %ld\n", GetLastError());
        return E_FAIL;
    }

    HMENU hOldSubMenu = m_hSubMenu;
    m_hSubMenu = hSubMenu;
    DestroyMenu(hOldSubMenu);

    return MAKE_HRESULT(SEVERITY_SUCCESS, 0, idCmdFirst + cItems);
}

STDMETHODIMP
CSendToMenu::InvokeCommand(LPCMINVOKECOMMANDINFO lpici)
{
    HRESULT hr = E_FAIL;

    WORD idCmd = LOWORD(lpici->lpVerb);
    TRACE("idCmd: %d\n", idCmd);

    SENDTO_ITEM *pItem = FindItemFromIdOffset(idCmd);
    if (pItem)
    {
        hr = DoSendToItem(pItem, lpici);
    }

    TRACE("CSendToMenu::InvokeCommand %x\n", hr);
    return hr;
}

STDMETHODIMP
CSendToMenu::GetCommandString(UINT_PTR idCmd,
                              UINT uType,
                              UINT *pwReserved,
                              LPSTR pszName,
                              UINT cchMax)
{
    FIXME("%p %lu %u %p %p %u\n", this,
          idCmd, uType, pwReserved, pszName, cchMax);

    return E_NOTIMPL;
}

STDMETHODIMP
CSendToMenu::HandleMenuMsg(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    return S_OK;
}

STDMETHODIMP
CSendToMenu::HandleMenuMsg2(UINT uMsg, WPARAM wParam, LPARAM lParam,
                            LRESULT *plResult)
{
    UINT cxSmall = GetSystemMetrics(SM_CXSMICON);
    UINT cySmall = GetSystemMetrics(SM_CYSMICON);

    switch (uMsg)
    {
    case WM_MEASUREITEM:
        {
            MEASUREITEMSTRUCT* lpmis = reinterpret_cast<MEASUREITEMSTRUCT*>(lParam);
            if (!lpmis || lpmis->CtlType != ODT_MENU)
                break;

            UINT cxMenuCheck = GetSystemMetrics(SM_CXMENUCHECK);
            if (lpmis->itemWidth < cxMenuCheck)
                lpmis->itemWidth = cxMenuCheck;
            if (lpmis->itemHeight < cySmall)
                lpmis->itemHeight = cySmall;

            if (plResult)
                *plResult = TRUE;
            break;
        }
    case WM_DRAWITEM:
        {
            DRAWITEMSTRUCT* lpdis = reinterpret_cast<DRAWITEMSTRUCT*>(lParam);
            if (!lpdis || lpdis->CtlType != ODT_MENU)
                break;

            SENDTO_ITEM *pItem = reinterpret_cast<SENDTO_ITEM *>(lpdis->itemData);
            HICON hIcon = NULL;
            if (pItem)
                hIcon = pItem->hIcon;
            if (!hIcon)
                break;

            const RECT& rcItem = lpdis->rcItem;
            INT x = 4;
            INT y = lpdis->rcItem.top;
            y += (rcItem.bottom - rcItem.top - cySmall) / 2;
            DrawIconEx(lpdis->hDC, x, y, hIcon, cxSmall, cySmall,
                       0, NULL, DI_NORMAL);

            if (plResult)
                *plResult = TRUE;
        }
    }

    return S_OK;
}

STDMETHODIMP
CSendToMenu::Initialize(PCIDLIST_ABSOLUTE pidlFolder,
                        IDataObject *pdtobj, HKEY hkeyProgID)
{
    m_pDataObject = pdtobj;
    return S_OK;
}
