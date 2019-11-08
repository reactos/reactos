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
#define INITGUID
#include <guiddef.h>

#define MAX_ITEM_COUNT 64

WINE_DEFAULT_DEBUG_CHANNEL(shell);

DEFINE_GUID(CLSID_SendToMenu, 0x7BA4C740, 0x9E81, 0x11CF,
            0x99, 0xD3, 0x00, 0xAA, 0x00, 0x4A, 0xE8, 0x37);

CSendToMenu::CSendToMenu()
    : m_hSubMenu(NULL)
    , m_pItems(NULL)
    , m_idCmdFirst(0)
{
    SHGetDesktopFolder(&m_pDesktop);
    m_pSendTo = GetSpecialFolder(NULL, CSIDL_SENDTO);
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
    if (SUCCEEDED(hr) && dwEffect != DROPEFFECT_NONE)
    {
        // THIS CODE IS NOT HUMAN-FRIENDLY. SORRY.
        // (We have to translate a SendTo action to a Drop action)
        if (bShift && bCtrl)
            dwEffect = DROPEFFECT_LINK;
        else if (!bShift)
            dwEffect = DROPEFFECT_MOVE;
        else
            dwEffect = DROPEFFECT_COPY;

        hr = pDropTarget->Drop(pDataObject, dwKeyState, ptl, &dwEffect);
    }
    else
    {
        ERR("DragEnter: %08lX\n", hr);
        pDropTarget->DragLeave();
    }

    return hr;
}

// get an IShellFolder from CSIDL
IShellFolder *CSendToMenu::GetSpecialFolder(HWND hwnd, int csidl)
{
    if (!m_pDesktop)
    {
        SHGetDesktopFolder(&m_pDesktop);
        if (!m_pDesktop)
        {
            ERR("SHGetDesktopFolder\n");
            return NULL;
        }
    }

    LPITEMIDLIST pidl = NULL;
    HRESULT hr = SHGetSpecialFolderLocation(hwnd, csidl, &pidl);
    if (FAILED(hr))
    {
        ERR("SHGetSpecialFolderLocation: %08lX\n", hr);
        return NULL;
    }

    IShellFolder *pFolder = NULL;
    hr = m_pDesktop->BindToObject(pidl, NULL, IID_PPV_ARG(IShellFolder, &pFolder));
    CoTaskMemFree(pidl);
    if (SUCCEEDED(hr))
        return pFolder;

    ERR("BindToObject: %08lX\n", hr);
    return NULL;
}

// get a UI object from PIDL
HRESULT CSendToMenu::GetUIObjectFromPidl(HWND hwnd, LPITEMIDLIST pidl,
                                         REFIID riid, LPVOID *ppvOut)
{
    *ppvOut = NULL;

    LPCITEMIDLIST pidlLast;
    CComPtr<IShellFolder> pFolder;
    HRESULT hr = SHBindToParent(pidl, IID_PPV_ARG(IShellFolder, &pFolder), &pidlLast);
    if (FAILED(hr))
    {
        ERR("SHBindToParent: %08lX\n", hr);
        return hr;
    }

    hr = pFolder->GetUIObjectOf(hwnd, 1, &pidlLast, riid, NULL, ppvOut);
    if (FAILED(hr))
    {
        ERR("GetUIObjectOf: %08lX\n", hr);
    }
    return hr;
}

void CSendToMenu::UnloadItem(SENDTO_ITEM *pItem)
{
    if (!pItem)
        return;

    CoTaskMemFree(pItem->pidlChild);
    CoTaskMemFree(pItem->pszText);
    HeapFree(GetProcessHeap(), 0, pItem);
}

void CSendToMenu::UnloadAllItems()
{
    SENDTO_ITEM *pItems = m_pItems;
    m_pItems = NULL;
    while (pItems)
    {
        SENDTO_ITEM *pCurItem = pItems;
        pItems = pItems->pNext;
        UnloadItem(pCurItem);
    }
}

BOOL CSendToMenu::LoadAllItems(HWND hwnd)
{
    UnloadAllItems();

    if (!m_pSendTo)
    {
        m_pSendTo = GetSpecialFolder(hwnd, CSIDL_SENDTO);
        if (!m_pSendTo)
        {
            ERR("GetSpecialFolder\n");
            return FALSE;
        }
    }

    LPITEMIDLIST pidlParent;
    SHGetSpecialFolderLocation(hwnd, CSIDL_SENDTO, &pidlParent);
    if (!pidlParent)
    {
        ERR("pidlParent\n");
        return FALSE;
    }

    HRESULT hr;
    CComPtr<IEnumIDList> pEnumIDList;
    hr = m_pSendTo->EnumObjects(hwnd,
                                SHCONTF_FOLDERS | SHCONTF_NONFOLDERS,
                                &pEnumIDList);
    if (FAILED(hr))
    {
        ERR("EnumObjects: %08lX\n", hr);
        return FALSE;
    }

    BOOL bOK = TRUE;
    LPITEMIDLIST pidlChild;
    UINT nCount = 0;
    while (pEnumIDList->Next(1, &pidlChild, NULL) == S_OK)
    {
        SENDTO_ITEM *pNewItem = (SENDTO_ITEM *)HeapAlloc(GetProcessHeap(),
                                                         HEAP_ZERO_MEMORY,
                                                         sizeof(SENDTO_ITEM));
        if (!pNewItem)
        {
            ERR("HeapAlloc\n");
            bOK = FALSE;
            CoTaskMemFree(pidlChild);
            break;
        }

        STRRET strret;
        hr = m_pSendTo->GetDisplayNameOf(pidlChild, SHGDN_NORMAL, &strret);
        if (SUCCEEDED(hr))
        {
            LPWSTR pszText = NULL;
            hr = StrRetToStrW(&strret, pidlChild, &pszText);
            if (SUCCEEDED(hr))
            {
                pNewItem->pidlChild = pidlChild;
                pNewItem->pszText = pszText;
                if (m_pItems)
                {
                    pNewItem->pNext = m_pItems;
                }
                m_pItems = pNewItem;

                // successful
                ++nCount;
                if (nCount >= MAX_ITEM_COUNT)
                {
                    break;
                }

                continue;
            }
            else
            {
                ERR("StrRetToStrW: %08lX\n", hr);
            }
        }
        else
        {
            ERR("GetDisplayNameOf: %08lX\n", hr);
        }

        ILFree(pidlChild);
    }

    return bOK;
}

UINT CSendToMenu::InsertSendToItems(HMENU hMenu, UINT idCmdFirst, UINT Pos)
{
    if (m_pItems == NULL)
    {
        if (!LoadAllItems(NULL))
        {
            ERR("LoadAllItems\n");
            return 0;
        }
    }

    m_idCmdFirst = idCmdFirst;

    UINT idCmd = idCmdFirst;
    UINT nCount = 0;
    for (SENDTO_ITEM *pCurItem = m_pItems; pCurItem; pCurItem = pCurItem->pNext)
    {
        const UINT uFlags = MF_BYPOSITION | MF_STRING | MF_ENABLED;
        if (InsertMenuW(hMenu, Pos, uFlags, idCmd, pCurItem->pszText))
        {
            MENUITEMINFOW mii;
            mii.cbSize = sizeof(mii);
            mii.fMask = MIIM_DATA;
            mii.dwItemData = (ULONG_PTR)pCurItem;
            SetMenuItemInfoW(hMenu, idCmd, FALSE, &mii);
            ++idCmd;

            // successful
            ++nCount;
            if (nCount >= MAX_ITEM_COUNT)
            {
                break;
            }
        }
    }

    if (idCmd == idCmdFirst)
    {
        WCHAR szNone[64] = L"(None)";
        LoadStringW(shell32_hInstance, IDS_NONE, szNone, _countof(szNone));

        AppendMenuW(hMenu, MF_GRAYED | MF_DISABLED | MF_STRING, idCmd, szNone);
    }

    return idCmd - idCmdFirst;
}

CSendToMenu::SENDTO_ITEM *CSendToMenu::FindItemFromIdOffset(UINT IdOffset)
{
    UINT idCmd = m_idCmdFirst + IdOffset;

    MENUITEMINFOW mii = { sizeof(mii) };
    mii.fMask = MIIM_DATA;
    if (GetMenuItemInfoW(m_hSubMenu, idCmd, FALSE, &mii))
        return (SENDTO_ITEM *)mii.dwItemData;

    ERR("GetMenuItemInfoW\n");
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
    LPITEMIDLIST pidlChild = pItem->pidlChild;
    hr = m_pSendTo->GetUIObjectOf(NULL, 1, &pidlChild, IID_IDropTarget,
                                  NULL, (LPVOID *)&pDropTarget);
    if (SUCCEEDED(hr))
    {
        hr = DoDrop(m_pDataObject, pDropTarget);
    }
    else
    {
        ERR("GetUIObjectOf: %08lX\n", hr);
    }

    return hr;
}

STDMETHODIMP CSendToMenu::SetSite(IUnknown *pUnkSite)
{
    m_pSite = pUnkSite;
    return S_OK;
}

STDMETHODIMP CSendToMenu::GetSite(REFIID riid, void **ppvSite)
{
    if (!m_pSite)
        return E_FAIL;

    return m_pSite->QueryInterface(riid, ppvSite);
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

    WCHAR wszSendTo[64];
    if (!LoadStringW(shell32_hInstance, IDS_SENDTO,
                     wszSendTo, _countof(wszSendTo)))
    {
        ERR("IDS_SENDTO\n");
        return E_FAIL;
    }

    HMENU hSubMenu = CreateMenu();
    if (!hSubMenu)
    {
        ERR("CreateMenu\n");
        return E_FAIL;
    }

    UINT cItems = InsertSendToItems(hSubMenu, idCmdFirst, 0);

    MENUITEMINFOW mii;
    ZeroMemory(&mii, sizeof(mii));
    mii.cbSize = sizeof(mii);
    mii.fMask = MIIM_TYPE | MIIM_ID | MIIM_STATE | MIIM_SUBMENU;
    mii.fType = MFT_STRING;
    mii.wID = -1;
    mii.dwTypeData = wszSendTo;
    mii.cch = wcslen(mii.dwTypeData);
    mii.fState = MFS_ENABLED;
    mii.hSubMenu = hSubMenu;
    if (!InsertMenuItemW(hMenu, indexMenu, TRUE, &mii))
    {
        ERR("InsertMenuItemW\n");
        return E_FAIL;
    }

    HMENU hOldSubMenu = m_hSubMenu;
    m_hSubMenu = hSubMenu;
    DestroyMenu(hOldSubMenu);

    return MAKE_HRESULT(SEVERITY_SUCCESS, 0, cItems);
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
    else
    {
        ERR("FindItemFromIdOffset\n");
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
    return S_OK;
}

STDMETHODIMP
CSendToMenu::Initialize(PCIDLIST_ABSOLUTE pidlFolder,
                        IDataObject *pdtobj, HKEY hkeyProgID)
{
    m_pDataObject = pdtobj;
    return S_OK;
}
