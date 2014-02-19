/*
 * ReactOS Explorer
 *
 * Copyright 2014 Giannis Adamopoulos
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
#include "precomp.h"

//#define TEST_TRACKPOPUPMENU_SUBMENUS


/* NOTE: The following constants *MUST NOT* be changed because
         they're hardcoded and need to be the exact values
         in order to get the start menu to work! */
#define IDM_PROGRAMS                504
#define IDM_FAVORITES               507
#define IDM_DOCUMENTS               501
#define IDM_SETTINGS                508
#define IDM_CONTROLPANEL            505
#define IDM_SECURITY                5001
#define IDM_NETWORKCONNECTIONS      557
#define IDM_PRINTERSANDFAXES        510
#define IDM_TASKBARANDSTARTMENU     413
#define IDM_SEARCH                  520
#define IDM_HELPANDSUPPORT          503
#define IDM_RUN                     401
#define IDM_SYNCHRONIZE             553
#define IDM_LOGOFF                  402
#define IDM_DISCONNECT              5000
#define IDM_UNDOCKCOMPUTER          410
#define IDM_SHUTDOWN                506
#define IDM_LASTSTARTMENU_SEPARATOR 450

/*
 * TODO:
 * 1. append the start menu contents from all users
 * 2. implement the context menu for start menu entries (programs, control panel, network connetions, printers)
 * 3. filter out programs folder from the shell folder part of the start menu
 * 4. showing the programs start menu is SLOW compared to windows. this needs some investigation
 */

class CShellMenuCallback :
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IShellMenuCallback
{
private:

    HWND m_hwndTray;
    CComPtr<IShellMenu> m_pShellMenu;
    CComPtr<IBandSite> m_pBandSite;
    CComPtr<IDeskBar> m_pDeskBar;
    CComPtr<ITrayPriv> m_pTrayPriv;

    HRESULT OnInitMenu()
    {
        HMENU hmenu;
        HRESULT hr;

        if (m_pTrayPriv.p)
            return S_OK;

        hr = IUnknown_GetSite(m_pDeskBar, IID_PPV_ARG(ITrayPriv, &m_pTrayPriv));
        hr = IUnknown_GetWindow(m_pTrayPriv, &m_hwndTray);
        hr = m_pTrayPriv->AppendMenuW(&hmenu);
#ifndef TEST_TRACKPOPUPMENU_SUBMENUS
        hr = m_pShellMenu->SetMenu(hmenu, NULL, SMSET_BOTTOM);
#else
        hr = m_pShellMenu->SetMenu(hmenu, m_hwndTray, SMSET_BOTTOM);
#endif

        return hr;
    }

    HRESULT OnGetInfo(LPSMDATA psmd, SMINFO *psminfo)
    {
        int iconIndex = 0;

        switch (psmd->uId)
        {
        case IDM_PROGRAMS:  iconIndex = -20; break;
        case IDM_FAVORITES: iconIndex = -173; break;
        case IDM_DOCUMENTS: iconIndex = -21; break;
        case IDM_SETTINGS: iconIndex = -22; break;
        case IDM_CONTROLPANEL: iconIndex = -22; break;
        //case IDM_SECURITY: iconIndex = -21; break;
        case IDM_NETWORKCONNECTIONS: iconIndex = -257; break;
        case IDM_PRINTERSANDFAXES: iconIndex = -138; break;
        case IDM_TASKBARANDSTARTMENU: iconIndex = -40; break;
        case IDM_SEARCH: iconIndex = -23; break;
        case IDM_HELPANDSUPPORT: iconIndex = -24; break;
        case IDM_RUN: iconIndex = -25; break;
        //case IDM_SYNCHRONIZE: iconIndex = -21; break;
        case IDM_LOGOFF: iconIndex = -45; break;
        //case IDM_DISCONNECT: iconIndex = -21; break;
        //case IDM_UNDOCKCOMPUTER: iconIndex = -21; break;
        case IDM_SHUTDOWN: iconIndex = -28; break;
        default:
            return S_FALSE;
        }

        if (iconIndex)
        {
            if ((psminfo->dwMask & SMIM_ICON) != 0)
                psminfo->iIcon = Shell_GetCachedImageIndex(L"shell32.dll", iconIndex, FALSE);
#ifdef TEST_TRACKPOPUPMENU_SUBMENUS
            if ((psminfo->dwMask & SMIM_FLAGS) != 0)
                psminfo->dwFlags |= SMIF_TRACKPOPUP;
#endif
        }
        return S_OK;
    }

    HRESULT OnGetSubMenu(LPSMDATA psmd, REFIID iid, void ** pv)
    {
        HRESULT hr;
        int csidl = 0;
        IShellMenu *pShellMenu;

        switch (psmd->uId)
        {
        case IDM_PROGRAMS:  csidl = CSIDL_PROGRAMS; break;
        case IDM_FAVORITES: csidl = CSIDL_FAVORITES; break;
        case IDM_DOCUMENTS: csidl = CSIDL_RECENT; break;
        }

#if USE_SYSTEM_MENUBAND
        hr = CoCreateInstance(CLSID_MenuBand,
            NULL,
            CLSCTX_INPROC_SERVER,
            IID_PPV_ARG(IShellMenu, &pShellMenu));
#else
        hr = CMenuBand_Constructor(IID_PPV_ARG(IShellMenu, &pShellMenu));
#endif

        hr = pShellMenu->Initialize(this, 0, ANCESTORDEFAULT, SMINIT_VERTICAL);

        if (csidl)
        {
            LPITEMIDLIST pidlStartMenu;
            IShellFolder *psfDestop, *psfStartMenu;

            hr = SHGetFolderLocation(NULL, csidl, 0, 0, &pidlStartMenu);
            hr = SHGetDesktopFolder(&psfDestop);
            hr = psfDestop->BindToObject(pidlStartMenu, NULL, IID_PPV_ARG(IShellFolder, &psfStartMenu));

            hr = pShellMenu->SetShellFolder(psfStartMenu, NULL, NULL, 0);
        }
        else
        {
            MENUITEMINFO mii;
            mii.cbSize = sizeof(mii);
            mii.fMask = MIIM_SUBMENU;
            if (GetMenuItemInfoW(psmd->hmenu, psmd->uId, FALSE, &mii))
            {
                hr = pShellMenu->SetMenu(mii.hSubMenu, NULL, SMSET_BOTTOM);
            }
        }
        return pShellMenu->QueryInterface(iid, pv);
    }

    HRESULT OnGetContextMenu(LPSMDATA psmd, REFIID iid, void ** pv)
    {
        if (psmd->uId == IDM_PROGRAMS ||
            psmd->uId == IDM_CONTROLPANEL ||
            psmd->uId == IDM_NETWORKCONNECTIONS ||
            psmd->uId == IDM_PRINTERSANDFAXES)
        {
            //UNIMPLEMENTED
        }

        return S_FALSE;
    }

    HRESULT OnGetObject(LPSMDATA psmd, REFIID iid, void ** pv)
    {
        if (IsEqualIID(iid, IID_IShellMenu))
            return OnGetSubMenu(psmd, iid, pv);
        else if (IsEqualIID(iid, IID_IContextMenu))
            return OnGetContextMenu(psmd, iid, pv);

        return S_FALSE;
    }

    HRESULT OnExec(LPSMDATA psmd)
    {
        if (psmd->uId == IDM_CONTROLPANEL)
            ShellExecuteW(NULL, L"open", L"::{20D04FE0-3AEA-1069-A2D8-08002B30309D}\\::{21EC2020-3AEA-1069-A2DD-08002B30309D}", NULL, NULL, 1);
        else if (psmd->uId == IDM_NETWORKCONNECTIONS)
            ShellExecuteW(NULL, L"open", L"::{20D04FE0-3AEA-1069-A2D8-08002B30309D}\\::{21EC2020-3AEA-1069-A2DD-08002B30309D}\\::{7007ACC7-3202-11D1-AAD2-00805FC1270E}", NULL, NULL, 1);
        else if (psmd->uId == IDM_PRINTERSANDFAXES)
            ShellExecuteW(NULL, L"open", L"::{20D04FE0-3AEA-1069-A2D8-08002B30309D}\\::{21EC2020-3AEA-1069-A2DD-08002B30309D}\\::{2227A280-3AEA-1069-A2DE-08002B30309D}", NULL, NULL, 1);
        else
            PostMessageW(m_hwndTray, WM_COMMAND, psmd->uId, 0);

        return S_OK;
    }

public:

    DECLARE_NOT_AGGREGATABLE(CShellMenuCallback)
    DECLARE_PROTECT_FINAL_CONSTRUCT()
    BEGIN_COM_MAP(CShellMenuCallback)
        COM_INTERFACE_ENTRY_IID(IID_IShellMenuCallback, IShellMenuCallback)
    END_COM_MAP()

    void Initialize(
        IShellMenu* pShellMenu,
        IBandSite* pBandSite,
        IDeskBar* pDeskBar)
    {
        m_pShellMenu.Attach(pShellMenu);
        m_pBandSite.Attach(pBandSite);
        m_pDeskBar.Attach(pDeskBar);
    }

    ~CShellMenuCallback()
    {
        m_pShellMenu.Release();
        m_pBandSite.Release();
        m_pDeskBar.Release();
    }

    HRESULT STDMETHODCALLTYPE CallbackSM(
        LPSMDATA psmd,
        UINT uMsg,
        WPARAM wParam,
        LPARAM lParam)
    {
        switch (uMsg)
        {
        case SMC_INITMENU:
            return OnInitMenu();
        case SMC_GETINFO:
            return OnGetInfo(psmd, reinterpret_cast<SMINFO*>(lParam));
        case SMC_GETOBJECT:
            return OnGetObject(psmd, *reinterpret_cast<IID *>(wParam), reinterpret_cast<void **>(lParam));
        case SMC_EXEC:
            return OnExec(psmd);
        case SMC_SFEXEC:
            m_pTrayPriv->Execute(psmd->psf, psmd->pidlItem);
            break;
        }

        return S_FALSE;
    }
};

extern "C"
HRESULT
CStartMenu_Constructor(REFIID riid, void **ppv)
{
    IShellMenu* pShellMenu;
    IBandSite* pBandSite;
    IDeskBar* pDeskBar;
    LPITEMIDLIST pidlStartMenu;

    HRESULT hr;
    IShellFolder *shellFolder;
    IShellFolder *psfStartMenu;

#if USE_SYSTEM_MENUBAND
    hr = CoCreateInstance(CLSID_MenuBand,
                          NULL,
                          CLSCTX_INPROC_SERVER,
                          IID_PPV_ARG(IShellMenu, &pShellMenu));
#elif WRAP_MENUBAND
    hr = CMenuBand_Wrapper(IID_PPV_ARG(IShellMenu, &pShellMenu));
#else
    hr = CMenuBand_Constructor(IID_PPV_ARG(IShellMenu, &pShellMenu));
#endif
    if (FAILED(hr))
        return hr;

#if USE_SYSTEM_MENUSITE
    hr = CoCreateInstance(CLSID_MenuBandSite,
                          NULL,
                          CLSCTX_INPROC_SERVER,
                          IID_PPV_ARG(IBandSite, &pBandSite));
#else
    hr = CMenuSite_Constructor(IID_PPV_ARG(IBandSite, &pBandSite));
#endif
    if (FAILED(hr))
        return hr;

#if USE_SYSTEM_MENUDESKBAR
    hr = CoCreateInstance(CLSID_MenuDeskBar,
                          NULL,
                          CLSCTX_INPROC_SERVER,
                          IID_PPV_ARG(IDeskBar, &pDeskBar));
#elif WRAP_MENUDESKBAR
    hr = CMenuDeskBar_Wrapper(IID_PPV_ARG(IDeskBar, &pDeskBar));
#else
    hr = CMenuDeskBar_Constructor(IID_PPV_ARG(IDeskBar, &pDeskBar));
#endif
    if (FAILED(hr))
        return hr;

    CComObject<CShellMenuCallback> *pCallback;
    hr = CComObject<CShellMenuCallback>::CreateInstance(&pCallback);
    if (FAILED(hr))
        return hr;
    pCallback->AddRef(); // CreateInstance returns object with 0 ref count */
    pCallback->Initialize(pShellMenu, pBandSite, pDeskBar);

    pShellMenu->Initialize(pCallback, (UINT)-1, 0, SMINIT_TOPLEVEL | SMINIT_VERTICAL);
    if (FAILED(hr))
        return hr;

    /* FIXME: Use CLSID_MergedFolder class and IID_IAugmentedShellFolder2 interface here */
    /* CLSID_MergedFolder 26fdc864-be88-46e7-9235-032d8ea5162e */
    /* IID_IAugmentedShellFolder2 8db3b3f4-6cfe-11d1-8ae9-00c04fd918d0 */
    hr = SHGetFolderLocation(NULL, CSIDL_STARTMENU, 0, 0, &pidlStartMenu);
    hr = SHGetDesktopFolder(&shellFolder);
    hr = shellFolder->BindToObject(pidlStartMenu, NULL, IID_IShellFolder, (void**) &psfStartMenu);

    hr = pShellMenu->SetShellFolder(psfStartMenu, NULL, NULL, 0);

    hr = pDeskBar->SetClient(pBandSite);
    if (FAILED(hr))
        return hr;

    hr = pBandSite->AddBand(pShellMenu);
    if (FAILED(hr))
        return hr;

    return pDeskBar->QueryInterface(riid, ppv);
}
