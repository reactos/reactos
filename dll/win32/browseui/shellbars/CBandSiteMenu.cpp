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

#include <browseui_undoc.h>

/* The menu consists of 3 parts. The first is loaded from the resources,
   the second is populated with the classes of the CATID_DeskBand comcat
   and the third part consists of the entries for each CISFBand in the band side.
   The first 5 ids are reserved for the resource menu, the following ids will be
   for the CATID_DeskBand classes and the rest for the CISFBands.
   The ids for the CISFBand menu items are not continuous, in this range
   each menu id is calculated by adding the band id to the last id for the CATID_DeskBand range */
#define FIRST_COMCAT_MENU_ID 0x5

CBandSiteMenu::CBandSiteMenu():
    m_hmenu(NULL),
    m_DesktopPidl(NULL),
    m_QLaunchPidl(NULL)
{
}

CBandSiteMenu::~CBandSiteMenu()
{
    if (m_hmenu)
        DestroyMenu(m_hmenu);

    m_BandSite = NULL;
}

HRESULT WINAPI CBandSiteMenu::FinalConstruct()
{
    return S_OK;
}

HRESULT CBandSiteMenu::_CreateMenuPart()
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

    /* Load the template we will fill in */
    m_hmenu = LoadMenuW(GetModuleHandleW(L"browseui.dll"), MAKEINTRESOURCEW(IDM_TASKBAR_TOOLBARS));
    if (!m_hmenu)
        return HRESULT_FROM_WIN32(GetLastError());

    /* Get the handle of the submenu where the available items will be shown */
    hmenuToolbars = GetSubMenu(m_hmenu, 0);

    /* Create the category enumerator */
    hr = SHEnumClassesOfCategories(1, &category, 0, NULL, &pEnumGUID);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    m_ComCatGuids.RemoveAll();

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
        InsertMenu(hmenuToolbars, cBands, MF_BYPOSITION, m_ComCatGuids.GetSize() + FIRST_COMCAT_MENU_ID, wszBandName);
        m_ComCatGuids.Add(iter);
        cBands++;
    }
    while (dwRead > 0);

    return S_OK;
}

HRESULT CBandSiteMenu::_CreateNewISFBand(HWND hwnd, REFIID riid, void** ppv)
{
    WCHAR path[MAX_PATH];
    WCHAR message[256];
    BROWSEINFOW bi = { hwnd, NULL, path };

    if (LoadStringW(GetModuleHandleW(L"browseui.dll"), IDS_BROWSEFORNEWTOOLAR, message, _countof(message)))
        bi.lpszTitle = message;
    else
        bi.lpszTitle = L"Choose a folder";

    CComHeapPtr<ITEMIDLIST> pidlSelected;
    pidlSelected.Attach(SHBrowseForFolderW(&bi));
    if (pidlSelected == NULL)
        return S_FALSE;

    CComPtr<IShellFolderBand> pISFB;
    HRESULT hr = CISFBand_CreateInstance(IID_IShellFolderBand, (PVOID*)&pISFB);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    hr = pISFB->InitializeSFB(NULL, pidlSelected);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    return pISFB->QueryInterface(riid, ppv);
}

LPITEMIDLIST CBandSiteMenu::_GetQLaunchPidl(BOOL refresh)
{
    if (m_QLaunchPidl != NULL)
    {
        if (refresh)
            m_QLaunchPidl.Free();
        else
            return m_QLaunchPidl;
    }

    WCHAR buffer[MAX_PATH];
    HRESULT hr = SHGetFolderPathAndSubDirW(0, CSIDL_APPDATA, NULL, 0, L"Microsoft\\Internet Explorer\\Quick Launch", buffer);
    if (FAILED_UNEXPECTEDLY(hr))
        return NULL;

    m_QLaunchPidl.Attach(ILCreateFromPathW(buffer));
    return m_QLaunchPidl;
}

HRESULT CBandSiteMenu::_CreateBuiltInISFBand(UINT uID, REFIID riid, void** ppv)
{
    LPITEMIDLIST pidl;
    HRESULT hr;

    switch (uID)
    {
        case IDM_TASKBAR_TOOLBARS_DESKTOP:
        {
            if (m_DesktopPidl != NULL)
                m_DesktopPidl.Free();

            hr = SHGetFolderLocation(0, CSIDL_DESKTOP, NULL, 0, &m_DesktopPidl);
            if (FAILED_UNEXPECTEDLY(hr))
                return hr;

            pidl = m_DesktopPidl;
            break;
        }
        case IDM_TASKBAR_TOOLBARS_QUICKLAUNCH:
        {
            pidl = _GetQLaunchPidl(true);
            break;
        }
    }

    if (pidl == NULL)
        return E_FAIL;

    CComPtr<IShellFolderBand> pISFB;
    hr = CISFBand_CreateInstance(IID_IShellFolderBand, (PVOID*)&pISFB);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    hr = pISFB->InitializeSFB(NULL, pidl);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    /* HACK! We shouldn't pass ISFB_STATE_QLINKSMODE and CISFBand shouldn't handle it! */
    if (uID == IDM_TASKBAR_TOOLBARS_QUICKLAUNCH)
    {
        BANDINFOSFB bisfb = {ISFB_MASK_STATE, ISFB_STATE_QLINKSMODE, ISFB_STATE_QLINKSMODE};
        pISFB->SetBandInfoSFB(&bisfb);
    }

    return pISFB->QueryInterface(riid, ppv);
}

HRESULT CBandSiteMenu::_AddISFBandToMenu(HMENU hmenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, IUnknown* pBand, DWORD dwBandID, UINT *newMenuId)
{
    CComPtr<IShellFolderBand> psfb;
    HRESULT hr = pBand->QueryInterface(IID_PPV_ARG(IShellFolderBand, &psfb));
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    BANDINFOSFB bi = {ISFB_MASK_IDLIST};
    hr = psfb->GetBandInfoSFB(&bi);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    CComHeapPtr<ITEMIDLIST> pidl(bi.pidl);
    if (!pidl)
    {
        ERR("Failed to get the pidl of the CISFBand\n");
        return E_OUTOFMEMORY;
    }

    WCHAR buffer[MAX_PATH];
    hr = ILGetDisplayNameEx(NULL, pidl, buffer, ILGDN_INFOLDER) ? S_OK : E_FAIL;
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    UINT id = idCmdFirst + m_ComCatGuids.GetSize() + FIRST_COMCAT_MENU_ID + dwBandID;
    if (id >= idCmdLast)
        return E_FAIL;

    *newMenuId = id;
    InsertMenu(hmenu, indexMenu, MF_BYPOSITION, id, buffer);
    return S_OK;
}

UINT CBandSiteMenu::_GetMenuIdFromISFBand(IUnknown *pBand)
{
    CComPtr<IShellFolderBand> psfb;
    HRESULT hr = pBand->QueryInterface(IID_PPV_ARG(IShellFolderBand, &psfb));
    if (FAILED_UNEXPECTEDLY(hr))
        return UINT_MAX;

    BANDINFOSFB bi = {ISFB_MASK_IDLIST};
    hr = psfb->GetBandInfoSFB(&bi);
    if (FAILED_UNEXPECTEDLY(hr))
        return UINT_MAX;

    CComHeapPtr<ITEMIDLIST> pidl(bi.pidl);
    if (!pidl)
    {
        ERR("Failed to get the pidl of the CISFBand\n");
        return UINT_MAX;
    }

    if (pidl->mkid.cb == 0)
    {
        return IDM_TASKBAR_TOOLBARS_DESKTOP;
    }

    CComPtr<IShellFolder> psfDesktop;
    hr = SHGetDesktopFolder(&psfDesktop);
    if (FAILED_UNEXPECTEDLY(hr))
        return UINT_MAX;

    LPITEMIDLIST _QLaunchPidl = _GetQLaunchPidl(false);
    if (_QLaunchPidl == NULL)
        return UINT_MAX;

    hr = psfDesktop->CompareIDs(0, pidl, _QLaunchPidl);
    if (FAILED_UNEXPECTEDLY(hr))
        return UINT_MAX;

    if (HRESULT_CODE(hr) == 0)
        return IDM_TASKBAR_TOOLBARS_QUICKLAUNCH;

    return UINT_MAX;
}

UINT CBandSiteMenu::_GetBandIdFromClsid(CLSID* pclsid)
{
    CComPtr<IPersist> pBand;
    CLSID BandCLSID;
    DWORD dwBandID;

    for (UINT uBand = 0; SUCCEEDED(m_BandSite->EnumBands(uBand, &dwBandID)); uBand++)
    {
        if (FAILED(m_BandSite->GetBandObject(dwBandID, IID_PPV_ARG(IPersist, &pBand))))
            continue;

        if (FAILED_UNEXPECTEDLY(pBand->GetClassID(&BandCLSID)))
            continue;

        if (IsEqualGUID(*pclsid, BandCLSID))
            return dwBandID;
    }

    return UINT_MAX;
}

UINT CBandSiteMenu::_GetBandIdForBuiltinISFBand(UINT uID)
{
    CComPtr<IPersist> pBand;
    CLSID BandCLSID;
    DWORD dwBandID;

    for (UINT uBand = 0; SUCCEEDED(m_BandSite->EnumBands(uBand, &dwBandID)); uBand++)
    {
        if (FAILED(m_BandSite->GetBandObject(dwBandID, IID_PPV_ARG(IPersist, &pBand))))
            continue;

        if (FAILED_UNEXPECTEDLY(pBand->GetClassID(&BandCLSID)))
            continue;

        if (!IsEqualGUID(BandCLSID, CLSID_ISFBand))
            continue;

        UINT menuID = _GetMenuIdFromISFBand(pBand);
        if (menuID == uID)
            return dwBandID;
    }

    return UINT_MAX;
}

HRESULT STDMETHODCALLTYPE CBandSiteMenu::SetOwner(IUnknown *pOwner)
{
    TRACE("CBandSiteMenu::SetOwner(%p, %p)\n", this, pOwner);

    /* Cache the menu that will be merged every time QueryContextMenu is called */
    _CreateMenuPart();

    return pOwner->QueryInterface(IID_PPV_ARG(IBandSite, &m_BandSite));
}

HRESULT STDMETHODCALLTYPE CBandSiteMenu::QueryContextMenu(
    HMENU hmenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags)
{
    CComPtr<IPersist> pBand;
    CLSID BandCLSID;
    DWORD dwBandID;
    UINT idMax;

    TRACE("CBandSiteMenu::QueryContextMenu(%p, %p, %u, %u, %u, 0x%x)\n", this, hmenu, indexMenu, idCmdFirst, idCmdLast, uFlags);

    /* First Merge the menu with the available bands */
    idMax = Shell_MergeMenus(hmenu, m_hmenu, indexMenu, idCmdFirst, idCmdLast, MM_DONTREMOVESEPS | MM_SUBMENUSHAVEIDS);

    HMENU hmenuToolbars = GetSubMenu(hmenu, indexMenu);

    /* Enumerate all present bands and mark them as checked in the menu */
    for (UINT uBand = 0; SUCCEEDED(m_BandSite->EnumBands(uBand, &dwBandID)); uBand++)
    {
        if (FAILED(m_BandSite->GetBandObject(dwBandID, IID_PPV_ARG(IPersist, &pBand))))
            continue;

        if (FAILED_UNEXPECTEDLY(pBand->GetClassID(&BandCLSID)))
            continue;

        UINT menuID;
        if (IsEqualGUID(BandCLSID, CLSID_ISFBand))
        {
            menuID = _GetMenuIdFromISFBand(pBand);
            if (menuID == UINT_MAX)
            {
                HRESULT hr;
                hr = _AddISFBandToMenu(hmenuToolbars, 0, idCmdFirst, idCmdLast, pBand, dwBandID, &menuID);
                if (SUCCEEDED(hr) && menuID > idMax)
                    idMax = menuID;
                menuID -= idCmdFirst;
            }
        }
        else
        {
            int i = m_ComCatGuids.Find(BandCLSID);
            menuID = (i == -1 ? UINT_MAX : i + FIRST_COMCAT_MENU_ID);
        }

        if (menuID != UINT_MAX)
            CheckMenuItem(hmenuToolbars, menuID + idCmdFirst, MF_CHECKED);
    }

    return MAKE_HRESULT(SEVERITY_SUCCESS, 0, USHORT(idMax - idCmdFirst +1));
}

HRESULT CBandSiteMenu::_ShowToolbarError(HRESULT hRet)
{
    WCHAR szText[260];
    WCHAR szTitle[256];

    if (!LoadStringW(GetModuleHandleW(L"browseui.dll"), IDS_TOOLBAR_ERR_TEXT, szText, _countof(szText)))
        StringCchCopyW(szText, _countof(szText), L"Cannot create toolbar.");

    if (!LoadStringW(GetModuleHandleW(L"browseui.dll"), IDS_TOOLBAR_ERR_TITLE, szTitle, _countof(szTitle)))
        StringCchCopyW(szTitle, _countof(szTitle), L"Toolbar");

    MessageBoxW(NULL, szText, szTitle, MB_OK | MB_ICONSTOP | MB_SETFOREGROUND);
    return hRet;
}

HRESULT STDMETHODCALLTYPE CBandSiteMenu::InvokeCommand(LPCMINVOKECOMMANDINFO lpici)
{
    HRESULT hRet;
    DWORD dwBandID;

    /* FIXME: do we need to handle this and how? */
    if (HIWORD(lpici->lpVerb) != NULL)
        return E_FAIL;

    UINT uID = LOWORD(lpici->lpVerb);
    if (uID == IDM_TASKBAR_TOOLBARS_NEW)
    {
        CComPtr<IDeskBand> pDeskBand;
        hRet = _CreateNewISFBand(lpici->hwnd, IID_PPV_ARG(IDeskBand, &pDeskBand));
        if (FAILED_UNEXPECTEDLY(hRet))
            return hRet;

        hRet = m_BandSite->AddBand(pDeskBand);
        if (FAILED_UNEXPECTEDLY(hRet))
            return hRet;

        return S_OK;
    }
    else if (uID > (UINT) m_ComCatGuids.GetSize() + FIRST_COMCAT_MENU_ID )
    {
        dwBandID = uID - (m_ComCatGuids.GetSize() + FIRST_COMCAT_MENU_ID );

        m_BandSite->RemoveBand(dwBandID);

        return S_OK;
    }
    else if (uID == IDM_TASKBAR_TOOLBARS_DESKTOP || uID == IDM_TASKBAR_TOOLBARS_QUICKLAUNCH)
    {
        dwBandID = _GetBandIdForBuiltinISFBand(uID);
        if (dwBandID != UINT_MAX)
        {
            m_BandSite->RemoveBand(dwBandID);
        }
        else
        {
            CComPtr<IDeskBand> pDeskBand;
            hRet = _CreateBuiltInISFBand(uID, IID_PPV_ARG(IDeskBand, &pDeskBand));
            if (FAILED_UNEXPECTEDLY(hRet))
                return _ShowToolbarError(hRet);

            hRet = m_BandSite->AddBand(pDeskBand);
            if (FAILED_UNEXPECTEDLY(hRet))
                return _ShowToolbarError(hRet);
        }
        return S_OK;
    }

    /* Get the GUID of the item that was clicked */
    GUID *pguidToolbar = &m_ComCatGuids[uID - FIRST_COMCAT_MENU_ID];
    if (!pguidToolbar)
        return E_FAIL;

    /* Try to find if a band with a guid is present. If it is, remove it and return */
    dwBandID = _GetBandIdFromClsid(pguidToolbar);
    if (dwBandID != UINT_MAX)
    {
        /* We found it, remove it */
        m_BandSite->RemoveBand(dwBandID);
    }
    else
    {
        /* It is not present. Add it. */
        CComPtr<IDeskBand> pDeskBand;
        hRet = CoCreateInstance(*pguidToolbar, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IDeskBand, &pDeskBand));
        if (FAILED_UNEXPECTEDLY(hRet))
            return hRet;

        hRet = m_BandSite->AddBand(pDeskBand);
        if (FAILED_UNEXPECTEDLY(hRet))
            return hRet;
    }

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
HRESULT WINAPI RSHELL_CBandSiteMenu_CreateInstance(REFIID riid, void **ppv)
{
    return ShellObjectCreator<CBandSiteMenu>(riid, ppv);
}
