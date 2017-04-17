/*
 *  Band site menu
 *
 *  Copyright 2007  Hervé Poussineua
 *  Copyright 2009  Andrew Hill
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

#include "shellbars.h"
#include <strsafe.h>

CBandSiteMenu::CBandSiteMenu():
    m_menuDsa(NULL),
    m_hmenu(NULL)
{
}

CBandSiteMenu::~CBandSiteMenu()
{
    if (m_hmenu)
        DestroyMenu(m_hmenu);

    if (m_menuDsa)
        DSA_Destroy(m_menuDsa);

    m_BandSite = NULL;
}


HRESULT CBandSiteMenu::CreateMenuPart()
{
    WCHAR wszBandName[MAX_PATH];
    WCHAR wszBandGUID[MAX_PATH];
    WCHAR wRegKey[MAX_PATH];
    UINT cBands;
    DWORD dwDataSize;
    CATID category = CATID_DeskBand;
    HMENU hmenuToolbars;
    DWORD dwRead;
    CComPtr<IEnumGUID> pEnumGUID;
    HRESULT hr;

    if (m_hmenu)
        DestroyMenu(m_hmenu);

    if (m_menuDsa)
        DSA_Destroy(m_menuDsa);

    /* Load the template we will fill in */
    m_hmenu = LoadMenuW(GetModuleHandleW(L"browseui.dll"), MAKEINTRESOURCEW(IDM_TASKBAR_TOOLBARS));
    if (!m_hmenu)
        return HRESULT_FROM_WIN32(GetLastError());

    m_menuDsa = DSA_Create(sizeof(GUID), 5);
    if (!m_menuDsa)
        return E_OUTOFMEMORY;

    /* Get the handle of the submenu where the available items will be shown */
    hmenuToolbars = GetSubMenu(m_hmenu, 0);

    /* Create the category enumerator */
    hr = SHEnumClassesOfCategories(1, &category, 0, NULL, &pEnumGUID);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    /* Enumerate the classes in the  CATID_DeskBand category */
    cBands = 0;
    do
    {
        GUID iter;
        pEnumGUID->Next(1, &iter, &dwRead);
        if (!dwRead)
            continue;

        if (!StringFromGUID2(iter, wszBandGUID, MAX_PATH))
            continue;

        /* Get the band name */
        StringCchPrintfW(wRegKey, MAX_PATH, L"CLSID\\%s", wszBandGUID);
        dwDataSize = MAX_PATH;
        SHGetValue(HKEY_CLASSES_ROOT, wRegKey, NULL, NULL, wszBandName, &dwDataSize);

        /* Insert it */
        InsertMenu(hmenuToolbars, cBands, MF_BYPOSITION, DSA_GetItemCount(m_menuDsa), wszBandName);
        DSA_AppendItem(m_menuDsa, &iter);
        cBands++;
    }
    while (dwRead > 0);

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CBandSiteMenu::SetOwner(IUnknown *pOwner)
{
    TRACE("CBandSiteMenu::SetOwner(%p, %p)\n", this, pOwner);
    
    /* Cache the menu that will be merged every time QueryContextMenu is called */
    CreateMenuPart();

    return pOwner->QueryInterface(IID_PPV_ARG(IBandSite, &m_BandSite));
}

HRESULT STDMETHODCALLTYPE CBandSiteMenu::QueryContextMenu(
    HMENU hmenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags)
{
    CComPtr<IPersist> pBand;
    CLSID BandCLSID;
    DWORD dwBandID;
    UINT uBand = 0;

    TRACE("CBandSiteMenu::QueryContextMenu(%p, %p, %u, %u, %u, 0x%x)\n", this, hmenu, indexMenu, idCmdFirst, idCmdLast, uFlags);

    /* First Merge the menu with the available bands */
    Shell_MergeMenus(hmenu, m_hmenu, indexMenu, idCmdFirst, idCmdLast, MM_DONTREMOVESEPS | MM_SUBMENUSHAVEIDS);

    HMENU hmenuToolbars = GetSubMenu(hmenu, indexMenu);

    /* Enumerate all present bands and mark them as checked in the menu */
    while (SUCCEEDED(m_BandSite->EnumBands(uBand, &dwBandID)))
    {
        if (FAILED(m_BandSite->GetBandObject(dwBandID, IID_PPV_ARG(IPersist, &pBand))))
            continue;

        if (FAILED(pBand->GetClassID(&BandCLSID)))
            continue;

        /* Try to find the clsid of the band in the dsa */
        UINT count = DSA_GetItemCount(m_menuDsa);
        for (UINT i = 0; i < count; i++)
        {
            GUID* pdsaGUID = (GUID*)DSA_GetItemPtr(m_menuDsa, i);
            if (memcmp(pdsaGUID, &BandCLSID, sizeof(GUID)) == 0)
            {
                /* The index in the dsa is also the index in the menu */
                CheckMenuItem(hmenuToolbars, i, MF_CHECKED | MF_BYPOSITION);
            }
        }

        uBand++;
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CBandSiteMenu::InvokeCommand(LPCMINVOKECOMMANDINFO lpici)
{
    /* FIXME: do we need to handle this and how? */
    if (HIWORD(lpici->lpVerb) != NULL)
        return E_FAIL;

    /* Get the GUID of the item that was clicked */
    UINT uID = LOWORD(lpici->lpVerb);
    GUID *pguidToolbar = (GUID *)DSA_GetItemPtr(m_menuDsa, uID);
    if (!pguidToolbar)
        return E_FAIL;

    /* Try to find if a band with a guid is present. If it is remove it and return */
    CComPtr<IPersist> pBand;
    CLSID BandCLSID;
    DWORD dwBandID;
    UINT uBand = 0;
    while (SUCCEEDED(m_BandSite->EnumBands(uBand, &dwBandID)))
    {
        if (FAILED(m_BandSite->GetBandObject(dwBandID, IID_PPV_ARG(IPersist, &pBand))))
            continue;

        if (FAILED(pBand->GetClassID(&BandCLSID)))
            continue;

        if (memcmp(pguidToolbar, &BandCLSID, sizeof(GUID)) == 0)
        {
            /* We found it, remove it */
            m_BandSite->RemoveBand(dwBandID);
            return S_OK;
        }

        uBand++;
    }

    /* It is not present. Add it. */
    CComPtr<IDeskBand> pDeskBand;
    HRESULT hRet = CoCreateInstance(*pguidToolbar, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IDeskBand, &pDeskBand));
    if (FAILED(hRet))
        return hRet;

    hRet = m_BandSite->AddBand(pDeskBand);
    if (FAILED_UNEXPECTEDLY(hRet))
        return hRet;

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CBandSiteMenu::GetCommandString(UINT_PTR idCmd, UINT uType,
    UINT *pwReserved, LPSTR pszName, UINT cchMax)
{
    FIXME("CBandSiteMenu::GetCommandString is UNIMPLEMENTED (%p, %p, %u, %p, %p, %u)\n", this, idCmd, uType, pwReserved, pszName, cchMax);
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CBandSiteMenu::HandleMenuMsg(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    FIXME("CBandSiteMenu::HandleMenuMsg is UNIMPLEMENTED (%p, %u, %p, %p)\n", this, uMsg, wParam, lParam);
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CBandSiteMenu::HandleMenuMsg2(UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *plResult)
{
    FIXME("CBandSiteMenu::HandleMenuMsg2 is UNIMPLEMENTED(%p, %u, %p, %p, %p)\n", this, uMsg, wParam, lParam, plResult);
    return E_NOTIMPL;
}

extern "C"
HRESULT WINAPI CBandSiteMenu_CreateInstance(REFIID riid, void **ppv)
{
    return ShellObjectCreator<CBandSiteMenu>(riid, ppv);
}
