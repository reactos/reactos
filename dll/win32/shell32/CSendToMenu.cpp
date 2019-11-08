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

WINE_DEFAULT_DEBUG_CHANNEL(shell);

DEFINE_GUID(CLSID_SendToMenu, 0x7BA4C740, 0x9E81, 0x11CF, 0x99, 0xD3, 0x00, 0xAA, 0x00, 0x4A, 0xE8, 0x37);

CSendToMenu::CSendToMenu() :
    m_pSite(NULL),
    m_hSubMenu(NULL),
    m_pItems(NULL),
    m_idCmdFirst(0)
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
    POINTL ptl = { 0, 0 };

    DWORD dwEffect = DROPEFFECT_COPY | DROPEFFECT_MOVE | DROPEFFECT_LINK;

    HRESULT hr = pDropTarget->DragEnter(pDataObject, MK_LBUTTON, ptl, &dwEffect);
    if (SUCCEEDED(hr) && dwEffect != DROPEFFECT_NONE)
    {
        hr = pDropTarget->Drop(pDataObject, MK_LBUTTON, ptl, &dwEffect);
    }
    else
    {
        ERR("DragEnter: %08lX\n", hr);
        pDropTarget->DragLeave();
    }

    return hr;
}

IShellFolder *CSendToMenu::GetSpecialFolder(HWND hwnd, int csidl)
{
    if (!m_pDesktop)
        SHGetDesktopFolder(&m_pDesktop);

    LPITEMIDLIST pidl = NULL;
    HRESULT hr = SHGetSpecialFolderLocation(hwnd, csidl, &pidl);
    if (FAILED(hr))
    {
        ERR("SHGetSpecialFolderLocation: %08lX\n", hr);
        return NULL;
    }

    IShellFolder *pFolder = NULL;
    hr = m_pDesktop->BindToObject(pidl, NULL, IID_IShellFolder, (LPVOID *)&pFolder);
    CoTaskMemFree(pidl);
    if (SUCCEEDED(hr))
        return pFolder;

    ERR("BindToObject: %08lX\n", hr);
    return NULL;
}

HRESULT CSendToMenu::GetUIObjectFromPidl(HWND hwnd, LPITEMIDLIST pidl, REFIID riid, LPVOID *ppvOut)
{
    *ppvOut = NULL;

    LPCITEMIDLIST pidlLast;
    CComPtr<IShellFolder> pFolder;
    HRESULT hr = SHBindToParent(pidl, IID_IShellFolder, (LPVOID *)&pFolder, &pidlLast);
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
    CoTaskMemFree(pItem->pidlAbsolute);
    CoTaskMemFree(pItem->pszText);
    HeapFree(GetProcessHeap(), 0, pItem);
}

void CSendToMenu::UnloadAllItems()
{
    SENDTO_ITEM *pCurItem;

    while (m_pItems)
    {
        pCurItem = m_pItems;
        m_pItems = m_pItems->pNext;
        UnloadItem(pCurItem);
    }
    m_pItems = NULL;
}

BOOL
CSendToMenu::LoadAllItems(HWND hwnd)
{
    UnloadAllItems();

    if (!m_pSendTo)
    {
        m_pSendTo = GetSpecialFolder(NULL, CSIDL_SENDTO);
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

    CComPtr<IEnumIDList> pEnumIDList;
    HRESULT hr;
    hr = m_pSendTo->EnumObjects(hwnd,
                                SHCONTF_FOLDERS | SHCONTF_NONFOLDERS,
                                &pEnumIDList);
    if (FAILED(hr))
    {
        ERR("EnumObjects: %08lX\n", hr);
        return FALSE;
    }

    LPITEMIDLIST pidl;
    BOOL bOK = TRUE;
    while (pEnumIDList->Next(1, &pidl, NULL) == S_OK)
    {
        LPITEMIDLIST pidlAbsolute = ILCombine(pidlParent, pidl);
        if (!pidlAbsolute)
        {
            ERR("ILCombine\n");
            bOK = FALSE;
            break;
        }

        SENDTO_ITEM *pNewItem;
        pNewItem = (SENDTO_ITEM *)HeapAlloc(GetProcessHeap(),
                                            HEAP_ZERO_MEMORY,
                                            sizeof(SENDTO_ITEM));
        if (!pNewItem)
        {
            ERR("HeapAlloc\n");
            bOK = FALSE;
            CoTaskMemFree(pidlAbsolute);
            break;
        }

        STRRET strret;
        hr = m_pSendTo->GetDisplayNameOf(pidl, SHGDN_NORMAL, &strret);
        if (SUCCEEDED(hr))
        {
            LPWSTR pszText = NULL;
            hr = StrRetToStrW(&strret, pidl, &pszText);
            if (SUCCEEDED(hr))
            {
                pNewItem->pidlAbsolute = pidlAbsolute;
                pNewItem->pszText = pszText;

                if (m_pItems)
                {
                    pNewItem->pNext = m_pItems;
                    m_pItems = pNewItem;
                }
                else
                {
                    m_pItems = pNewItem;
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

        ILFree(pidlAbsolute);
    }

    return bOK;
}

UINT
CSendToMenu::InsertSendToItems(HMENU hMenu, UINT idCmdFirst, UINT Pos)
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
    for (SENDTO_ITEM *pCurItem = m_pItems; pCurItem; pCurItem = pCurItem->pNext)
    {
        UINT uFlags = MF_BYPOSITION | MF_STRING | MF_ENABLED;
        if (InsertMenuW(hMenu, Pos, uFlags, idCmd, pCurItem->pszText))
        {
            MENUITEMINFOW mii;
            mii.cbSize = sizeof(mii);
            mii.fMask = MIIM_DATA;
            mii.dwItemData = (ULONG_PTR)pCurItem;
            SetMenuItemInfoW(hMenu, idCmd, FALSE, &mii);
            ++idCmd;
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

    MENUITEMINFOW mii;
    ZeroMemory(&mii, sizeof(mii));
    mii.cbSize = sizeof(mii);
    mii.fMask = MIIM_DATA;
    if (GetMenuItemInfoW(m_hSubMenu, idCmd, FALSE, &mii))
        return (SENDTO_ITEM *)mii.dwItemData;

    ERR("GetMenuItemInfoW\n");
    return NULL;
}

HRESULT CSendToMenu::DoSendToItem(SENDTO_ITEM *pItem, LPCMINVOKECOMMANDINFO lpici)
{
    LPITEMIDLIST pidl = pItem->pidlAbsolute;

    HRESULT hr;

    IDataObject *pDataObject;
    hr = GetUIObjectFromPidl(NULL, pidl, IID_IDataObject, (LPVOID *)&pDataObject);
    if (FAILED(hr))
    {
        ERR("GetUIObjectFromPidl: %08lX\n", hr);
        return hr;
    }

    IDropTarget *pDropTarget;
    hr = m_pSendTo->GetUIObjectOf(NULL, 1, &pidl, IID_IDropTarget,
                                  NULL, (LPVOID *)&pDropTarget);
    if (SUCCEEDED(hr))
    {
        hr = DoDrop(pDataObject, pDropTarget);

        pDropTarget->Release();
    }
    else
    {
        ERR("GetUIObjectOf: %08lX\n", hr);
    }

    pDataObject->Release();

    return hr;
}

HRESULT STDMETHODCALLTYPE CSendToMenu::SetSite(IUnknown *pUnkSite)
{
    m_pSite = pUnkSite;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CSendToMenu::GetSite(REFIID riid, void **ppvSite)
{
    return m_pSite->QueryInterface(riid, ppvSite);
}

HRESULT
WINAPI
CSendToMenu::QueryContextMenu(HMENU hMenu,
                              UINT indexMenu,
                              UINT idCmdFirst,
                              UINT idCmdLast,
                              UINT uFlags)
{
    MENUITEMINFOW mii;
    UINT cItems = 0;
    WCHAR wszSendTo[64];
    HMENU hSubMenu, hOldSubMenu;

    TRACE("%p %p %u %u %u %u\n", this,
          hMenu, indexMenu, idCmdFirst, idCmdLast, uFlags);

    if (!LoadStringW(shell32_hInstance, IDS_SENDTO, wszSendTo, _countof(wszSendTo)))
    {
        ERR("IDS_SENDTO\n");
        return E_FAIL;
    }

    hSubMenu = CreateMenu();
    if (!hSubMenu)
    {
        ERR("CreateMenu\n");
        return E_FAIL;
    }

    cItems = InsertSendToItems(hSubMenu, idCmdFirst, 0);

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

    hOldSubMenu = m_hSubMenu;
    m_hSubMenu = hSubMenu;
    DestroyMenu(hOldSubMenu);

    return MAKE_HRESULT(SEVERITY_SUCCESS, 0, cItems);
}

HRESULT
WINAPI
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

HRESULT
WINAPI
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

HRESULT
WINAPI
CSendToMenu::HandleMenuMsg(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    return S_OK;
}

HRESULT
WINAPI
CSendToMenu::HandleMenuMsg2(UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *plResult)
{
    return S_OK;
}

HRESULT WINAPI
CSendToMenu::Initialize(PCIDLIST_ABSOLUTE pidlFolder,
                     IDataObject *pdtobj, HKEY hkeyProgID)
{
    return S_OK;
}
