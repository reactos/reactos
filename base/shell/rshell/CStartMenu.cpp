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

#include "CMergedFolder.h"

//#define TEST_TRACKPOPUPMENU_SUBMENUS


/* NOTE: The following constants *MUST NOT* be changed because
         they're hardcoded and need to be the exact values
         in order to get the start menu to work! */
#define IDM_RUN                     401
#define IDM_LOGOFF                  402
#define IDM_UNDOCKCOMPUTER          410
#define IDM_TASKBARANDSTARTMENU     413
#define IDM_LASTSTARTMENU_SEPARATOR 450
#define IDM_DOCUMENTS               501
#define IDM_HELPANDSUPPORT          503
#define IDM_PROGRAMS                504
#define IDM_CONTROLPANEL            505
#define IDM_SHUTDOWN                506
#define IDM_FAVORITES               507
#define IDM_SETTINGS                508
#define IDM_PRINTERSANDFAXES        510
#define IDM_SEARCH                  520
#define IDM_SYNCHRONIZE             553
#define IDM_NETWORKCONNECTIONS      557
#define IDM_DISCONNECT              5000
#define IDM_SECURITY                5001

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
    CComPtr<IShellFolder> m_psfPrograms;
    
    LPITEMIDLIST m_pidlPrograms;

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
            // Smaller "24x24" icons used for the start menu
            // The bitmaps are still 32x32, but the image is centered
        case IDM_FAVORITES: iconIndex = -322; break;
        case IDM_SEARCH: iconIndex = -323; break;
        case IDM_HELPANDSUPPORT: iconIndex = -324; break;
        case IDM_LOGOFF: iconIndex = -325; break;
        case IDM_PROGRAMS:  iconIndex = -326; break;
        case IDM_DOCUMENTS: iconIndex = -327; break;
        case IDM_RUN: iconIndex = -328; break;
        case IDM_SHUTDOWN: iconIndex = -329; break;
        case IDM_SETTINGS: iconIndex = -330; break;

        case IDM_CONTROLPANEL: iconIndex = -22; break;
        case IDM_NETWORKCONNECTIONS: iconIndex = -257; break;
        case IDM_PRINTERSANDFAXES: iconIndex = -138; break;
        case IDM_TASKBARANDSTARTMENU: iconIndex = -40; break;
        //case IDM_SECURITY: iconIndex = -21; break;
        //case IDM_SYNCHRONIZE: iconIndex = -21; break;
        //case IDM_DISCONNECT: iconIndex = -21; break;
        //case IDM_UNDOCKCOMPUTER: iconIndex = -21; break;
        default:
            return S_FALSE;
        }

        if (iconIndex)
        {
            if ((psminfo->dwMask & SMIM_TYPE) != 0)
                psminfo->dwType = SMIT_STRING;
            if ((psminfo->dwMask & SMIM_ICON) != 0)
                psminfo->iIcon = Shell_GetCachedImageIndex(L"shell32.dll", iconIndex, FALSE);
            if ((psminfo->dwMask & SMIM_FLAGS) != 0)
                psminfo->dwFlags |= SMIF_ICON;
#ifdef TEST_TRACKPOPUPMENU_SUBMENUS
            if ((psminfo->dwMask & SMIM_FLAGS) != 0)
                psminfo->dwFlags |= SMIF_TRACKPOPUP;
#endif
        }
        else
        {
            if ((psminfo->dwMask & SMIM_TYPE) != 0)
                psminfo->dwType = SMIT_SEPARATOR;
        }
        return S_OK;
    }

    HRESULT OnGetSubMenu(LPSMDATA psmd, REFIID iid, void ** pv)
    {
        HRESULT hr;
        int csidl = 0;
        IShellMenu *pShellMenu;

        hr = CMenuBand_Constructor(IID_PPV_ARG(IShellMenu, &pShellMenu));
        if (FAILED_UNEXPECTEDLY(hr))
            return hr;

        hr = pShellMenu->Initialize(this, 0, ANCESTORDEFAULT, SMINIT_VERTICAL);

        switch (psmd->uId)
        {
        case IDM_PROGRAMS:  csidl = CSIDL_PROGRAMS; break;
        case IDM_FAVORITES: csidl = CSIDL_FAVORITES; break;
        case IDM_DOCUMENTS: csidl = CSIDL_RECENT; break;
        }

        if (csidl)
        {
            IShellFolder *psfStartMenu;

            if (csidl == CSIDL_PROGRAMS && m_psfPrograms)
            {
                psfStartMenu = m_psfPrograms;
            }
            else
            {
                LPITEMIDLIST pidlStartMenu;
                IShellFolder *psfDestop;
                hr = SHGetFolderLocation(NULL, csidl, 0, 0, &pidlStartMenu);
                hr = SHGetDesktopFolder(&psfDestop);
                hr = psfDestop->BindToObject(pidlStartMenu, NULL, IID_PPV_ARG(IShellFolder, &psfStartMenu));
            }

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

    HRESULT _SetProgramsFolder(IShellFolder * psf, LPITEMIDLIST pidl)
    {
        m_psfPrograms = psf;
        m_pidlPrograms = pidl;
        return S_OK;
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
        case 0x10000000: // _FilterPIDL from CMenuSFToolbar
            if (psmd->psf->CompareIDs(0, psmd->pidlItem, m_pidlPrograms) == 0)
                return S_FALSE;
            return S_OK;
        }

        return S_FALSE;
    }
};

HRESULT BindToDesktop(LPCITEMIDLIST pidl, IShellFolder ** ppsfResult)
{
    HRESULT hr;
    CComPtr<IShellFolder> psfDesktop;

    *ppsfResult = NULL;

    hr = SHGetDesktopFolder(&psfDesktop);
    if (FAILED(hr))
        return hr;

    hr = psfDesktop->BindToObject(pidl, NULL, IID_PPV_ARG(IShellFolder, ppsfResult));

    return hr;
}

HRESULT GetStartMenuFolder(IShellFolder ** ppsfStartMenu)
{
    HRESULT hr;
    LPITEMIDLIST pidlUserStartMenu;
    LPITEMIDLIST pidlCommonStartMenu;

    *ppsfStartMenu = NULL;

    hr = SHGetSpecialFolderLocation(NULL, CSIDL_STARTMENU, &pidlUserStartMenu);
    if (FAILED(hr))
        return hr;

    if (FAILED(SHGetSpecialFolderLocation(NULL, CSIDL_COMMON_STARTMENU, &pidlCommonStartMenu)))
    {
        hr = BindToDesktop(pidlUserStartMenu, ppsfStartMenu);
        ILFree(pidlUserStartMenu);
        return hr;
    }

    CComPtr<IShellFolder> psfUserStartMenu;
    hr = BindToDesktop(pidlUserStartMenu, &psfUserStartMenu);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    CComPtr<IShellFolder> psfCommonStartMenu;
    hr = BindToDesktop(pidlCommonStartMenu, &psfCommonStartMenu);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

#if CUSTOM_MERGE_FOLDERS
    IShellFolder * psfMerged;
    hr = CMergedFolder_Constructor(psfUserStartMenu, psfCommonStartMenu, IID_PPV_ARG(IShellFolder, &psfMerged));
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;
#else
    CComPtr<IAugmentedShellFolder> pasf;
    hr = CoCreateInstance(CLSID_MergedFolder, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IAugmentedShellFolder, &pasf));
    if (FAILED_UNEXPECTEDLY(hr))
    {
        hr = BindToDesktop(pidlUserStartMenu, ppsfStartMenu);
        ILFree(pidlCommonStartMenu);
        ILFree(pidlUserStartMenu);
        return hr;
    }

    hr = pasf->AddNameSpace(NULL, psfUserStartMenu, pidlUserStartMenu, 0xFF00);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    hr = pasf->AddNameSpace(NULL, psfCommonStartMenu, pidlCommonStartMenu, 0);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    hr = pasf->QueryInterface(IID_PPV_ARG(IShellFolder, ppsfStartMenu));
    pasf.Release();
#endif

    ILFree(pidlCommonStartMenu);
    ILFree(pidlUserStartMenu);

    return hr;
}

extern "C"
HRESULT WINAPI
CStartMenu_Constructor(REFIID riid, void **ppv)
{
    IShellMenu* pShellMenu;
    IBandSite* pBandSite;
    IDeskBar* pDeskBar;

    HRESULT hr;
    IShellFolder * psf;

    LPITEMIDLIST pidlPrograms;
    CComPtr<IShellFolder> psfPrograms;

    hr = CMenuBand_Constructor(IID_PPV_ARG(IShellMenu, &pShellMenu));
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    hr = CMenuSite_Constructor(IID_PPV_ARG(IBandSite, &pBandSite));
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    hr = CMenuDeskBar_Constructor(IID_PPV_ARG(IDeskBar, &pDeskBar));
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    CComObject<CShellMenuCallback> *pCallback;
    hr = CComObject<CShellMenuCallback>::CreateInstance(&pCallback);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;
    pCallback->AddRef(); // CreateInstance returns object with 0 ref count */
    pCallback->Initialize(pShellMenu, pBandSite, pDeskBar);

    pShellMenu->Initialize(pCallback, (UINT) -1, 0, SMINIT_TOPLEVEL | SMINIT_VERTICAL);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;
    
    hr = GetStartMenuFolder(&psf);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    hr = psf->ParseDisplayName(NULL, NULL, L"Programs", NULL, &pidlPrograms, NULL);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    hr = psf->BindToObject(pidlPrograms, NULL, IID_PPV_ARG(IShellFolder, &psfPrograms));
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    hr = pCallback->_SetProgramsFolder(psfPrograms, pidlPrograms);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    hr = pShellMenu->SetShellFolder(psf, NULL, NULL, 0);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    hr = pDeskBar->SetClient(pBandSite);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    hr = pBandSite->AddBand(pShellMenu);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    return pDeskBar->QueryInterface(riid, ppv);
}
