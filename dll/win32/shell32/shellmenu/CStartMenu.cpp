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
#include "shellmenu.h"

#include "CMergedFolder.h"

WINE_DEFAULT_DEBUG_CHANNEL(CStartMenu);

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
        if (FAILED_UNEXPECTEDLY(hr))
            return hr;

        hr = IUnknown_GetWindow(m_pTrayPriv, &m_hwndTray);
        if (FAILED_UNEXPECTEDLY(hr))
            return hr;

        hr = m_pTrayPriv->AppendMenuW(&hmenu);
        if (FAILED_UNEXPECTEDLY(hr))
            return hr;

        hr = m_pShellMenu->SetMenu(hmenu, NULL, SMSET_BOTTOM);
        if (FAILED_UNEXPECTEDLY(hr))
            return hr;

        return hr;
    }

    HRESULT OnGetInfo(LPSMDATA psmd, SMINFO *psminfo)
    {
        int iconIndex = 0;

        switch (psmd->uId)
        {
            // Smaller "24x24" icons used for the start menu
            // The bitmaps are still 32x32, but the image is centered
        case IDM_FAVORITES: iconIndex = -IDI_SHELL_FAVOTITES; break;
        case IDM_SEARCH: iconIndex = -IDI_SHELL_SEARCH1; break;
        case IDM_HELPANDSUPPORT: iconIndex = -IDI_SHELL_HELP2; break;
        case IDM_LOGOFF: iconIndex = -IDI_SHELL_LOGOFF1; break;
        case IDM_PROGRAMS:  iconIndex = -IDI_SHELL_PROGRAMS_FOLDER1; break;
        case IDM_DOCUMENTS: iconIndex = -IDI_SHELL_RECENT_DOCUMENTS1; break;
        case IDM_RUN: iconIndex = -IDI_SHELL_RUN1; break;
        case IDM_SHUTDOWN: iconIndex = -IDI_SHELL_SHUTDOWN1; break;
        case IDM_SETTINGS: iconIndex = -IDI_SHELL_CONTROL_PANEL1; break;
        case IDM_MYDOCUMENTS: iconIndex = -IDI_SHELL_MY_DOCUMENTS; break;
        case IDM_MYPICTURES: iconIndex = -IDI_SHELL_MY_PICTURES; break;

        case IDM_CONTROLPANEL: iconIndex = -IDI_SHELL_CONTROL_PANEL; break;
        case IDM_NETWORKCONNECTIONS: iconIndex = -IDI_SHELL_NETWORK_CONNECTIONS2; break;
        case IDM_PRINTERSANDFAXES: iconIndex = -IDI_SHELL_PRINTER2; break;
        case IDM_TASKBARANDSTARTMENU: iconIndex = -IDI_SHELL_TSKBAR_STARTMENU; break;
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

    void InsertRecentItem(HMENU hMenu, UINT nID, INT csidl, BOOL bExpand) const
    {
        WCHAR szPath[MAX_PATH];
        if (!SHGetSpecialFolderPathW(NULL, szPath, csidl, FALSE))
        {
            ERR("SHGetSpecialFolderPathW failed\n");
            return;
        }

        LPWSTR pszText = PathFindFileNameW(szPath);
        if (bExpand)
        {
            MENUITEMINFOW mii = { sizeof(mii), MIIM_TYPE | MIIM_ID | MIIM_SUBMENU };
            mii.fType = MFT_STRING;
            mii.wID = nID;
            mii.hSubMenu = ::CreatePopupMenu();
            mii.dwTypeData = pszText;
            mii.cch = lstrlenW(pszText);
            InsertMenuItemW(hMenu, GetMenuItemCount(hMenu), TRUE, &mii);
        }
        else
        {
            AppendMenuW(hMenu, MF_STRING | MF_ENABLED, nID, pszText);
        }
    }

    HMENU CreateRecentMenu(BOOL bExpandMyDocuments, BOOL bExpandMyPictures) const
    {
        HMENU hMenu = ::CreateMenu();
        InsertRecentItem(hMenu, IDM_MYDOCUMENTS, CSIDL_MYDOCUMENTS, bExpandMyDocuments);
        InsertRecentItem(hMenu, IDM_MYPICTURES, CSIDL_MYPICTURES, bExpandMyPictures);
        AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
        return hMenu;
    }

    HRESULT OnGetSubMenu(LPSMDATA psmd, REFIID iid, void ** pv)
    {
        HRESULT hr;
        int csidl = 0;
        IShellMenu *pShellMenu;

        hr = CMenuBand_CreateInstance(IID_PPV_ARG(IShellMenu, &pShellMenu));
        if (FAILED_UNEXPECTEDLY(hr))
            return hr;

        hr = pShellMenu->Initialize(this, 0, ANCESTORDEFAULT, SMINIT_VERTICAL);
        if (FAILED_UNEXPECTEDLY(hr))
            return hr;

        switch (psmd->uId)
        {
        case IDM_PROGRAMS:  csidl = CSIDL_PROGRAMS; break;
        case IDM_FAVORITES: csidl = CSIDL_FAVORITES; break;
        case IDM_DOCUMENTS: csidl = CSIDL_RECENT; break;
        case IDM_MYDOCUMENTS: csidl = CSIDL_MYDOCUMENTS; break;
        case IDM_MYPICTURES: csidl = CSIDL_MYPICTURES; break;
        }

        if (csidl)
        {
            IShellFolder *psfStartMenu;
            DWORD dwFlags = SMSET_TOP;

            if (csidl == CSIDL_PROGRAMS && m_psfPrograms)
            {
                psfStartMenu = m_psfPrograms;
            }
            else
            {
                if (csidl == CSIDL_RECENT)
                {
                    BOOL bExpandMyDocuments = FALSE; /* FIXME: Get value from registry */
                    BOOL bExpandMyPictures = FALSE;  /* FIXME: Get value from registry */
                    HMENU hMenu = CreateRecentMenu(bExpandMyDocuments, bExpandMyPictures);
                    if (hMenu == NULL)
                        ERR("CreateRecentMenu failed\n");

                    hr = pShellMenu->SetMenu(hMenu, NULL, SMSET_BOTTOM);
                    if (FAILED_UNEXPECTEDLY(hr))
                        return hr;

                    dwFlags = SMSET_BOTTOM;
                }

                LPITEMIDLIST pidlStartMenu;
                IShellFolder *psfDestop;
                hr = SHGetFolderLocation(NULL, csidl, 0, 0, &pidlStartMenu);
                if (FAILED_UNEXPECTEDLY(hr))
                    return hr;

                hr = SHGetDesktopFolder(&psfDestop);
                if (FAILED_UNEXPECTEDLY(hr))
                    return hr;

                hr = psfDestop->BindToObject(pidlStartMenu, NULL, IID_PPV_ARG(IShellFolder, &psfStartMenu));
                if (FAILED_UNEXPECTEDLY(hr))
                    return hr;
            }

            hr = pShellMenu->SetShellFolder(psfStartMenu, NULL, NULL, dwFlags);
            if (FAILED_UNEXPECTEDLY(hr))
                return hr;
        }
        else
        {
            MENUITEMINFO mii;
            mii.cbSize = sizeof(mii);
            mii.fMask = MIIM_SUBMENU;
            if (GetMenuItemInfoW(psmd->hmenu, psmd->uId, FALSE, &mii))
            {
                hr = pShellMenu->SetMenu(mii.hSubMenu, NULL, SMSET_BOTTOM);
                if (FAILED_UNEXPECTEDLY(hr))
                    return hr;
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
        WCHAR szPath[MAX_PATH];

        // HACK: Because our ShellExecute can't handle CLSID components in paths, we can't launch the paths using the "open" verb.
        // FIXME: Change this back to using the path as the filename and the "open" verb, once ShellExecute can handle CLSID path components.

        if (psmd->uId == IDM_CONTROLPANEL)
            ShellExecuteW(NULL, NULL, L"explorer.exe", L"::{20D04FE0-3AEA-1069-A2D8-08002B30309D}\\::{21EC2020-3AEA-1069-A2DD-08002B30309D}", NULL, SW_SHOWNORMAL);
        else if (psmd->uId == IDM_NETWORKCONNECTIONS)
            ShellExecuteW(NULL, NULL, L"explorer.exe", L"::{20D04FE0-3AEA-1069-A2D8-08002B30309D}\\::{21EC2020-3AEA-1069-A2DD-08002B30309D}\\::{7007ACC7-3202-11D1-AAD2-00805FC1270E}", NULL, SW_SHOWNORMAL);
        else if (psmd->uId == IDM_PRINTERSANDFAXES)
            ShellExecuteW(NULL, NULL, L"explorer.exe", L"::{20D04FE0-3AEA-1069-A2D8-08002B30309D}\\::{21EC2020-3AEA-1069-A2DD-08002B30309D}\\::{2227A280-3AEA-1069-A2DE-08002B30309D}", NULL, SW_SHOWNORMAL);
        else if (psmd->uId == IDM_MYDOCUMENTS)
        {
            if (SHGetSpecialFolderPathW(NULL, szPath, CSIDL_PERSONAL, FALSE))
                ShellExecuteW(NULL, NULL, szPath, NULL, NULL, SW_SHOWNORMAL);
            else
                ERR("SHGetSpecialFolderPathW failed\n");
        }
        else if (psmd->uId == IDM_MYPICTURES)
        {
            if (SHGetSpecialFolderPathW(NULL, szPath, CSIDL_MYPICTURES, FALSE))
                ShellExecuteW(NULL, NULL, szPath, NULL, NULL, SW_SHOWNORMAL);
            else
                ERR("SHGetSpecialFolderPathW failed\n");
        }
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
        m_pShellMenu = pShellMenu;
        m_pBandSite = pBandSite;
        m_pDeskBar = pDeskBar;
    }

    ~CShellMenuCallback()
    {
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
                return S_OK;
            return S_FALSE;
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

static HRESULT GetMergedFolder(int folder1, int folder2, IShellFolder ** ppsfStartMenu)
{
    HRESULT hr;
    LPITEMIDLIST pidlUserStartMenu;
    LPITEMIDLIST pidlCommonStartMenu;
    CComPtr<IShellFolder> psfUserStartMenu;
    CComPtr<IShellFolder> psfCommonStartMenu;
    CComPtr<IAugmentedShellFolder> pasf;

    *ppsfStartMenu = NULL;

    hr = SHGetSpecialFolderLocation(NULL, folder1, &pidlUserStartMenu);
    if (FAILED(hr))
    {
        WARN("Failed to get the USER start menu folder. Trying to run with just the COMMON one.\n");

        hr = SHGetSpecialFolderLocation(NULL, folder2, &pidlCommonStartMenu);
        if (FAILED_UNEXPECTEDLY(hr))
            return hr;

        TRACE("COMMON start menu obtained.\n");
        hr = BindToDesktop(pidlCommonStartMenu, ppsfStartMenu);
        ILFree(pidlCommonStartMenu);
        return hr;
    }
#if MERGE_FOLDERS
    hr = SHGetSpecialFolderLocation(NULL, folder2, &pidlCommonStartMenu);
    if (FAILED_UNEXPECTEDLY(hr))
#else
    else
#endif
    {
        WARN("Failed to get the COMMON start menu folder. Will use only the USER contents.\n");
        hr = BindToDesktop(pidlUserStartMenu, ppsfStartMenu);
        ILFree(pidlUserStartMenu);
        return hr;
    }

    TRACE("Both COMMON and USER statr menu folders obtained, merging them...\n");

    hr = BindToDesktop(pidlUserStartMenu, &psfUserStartMenu);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    hr = BindToDesktop(pidlCommonStartMenu, &psfCommonStartMenu);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    hr = CMergedFolder_CreateInstance(IID_PPV_ARG(IAugmentedShellFolder, &pasf));
    if (FAILED_UNEXPECTEDLY(hr))
    {
        *ppsfStartMenu = psfUserStartMenu.Detach();
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

    ILFree(pidlCommonStartMenu);
    ILFree(pidlUserStartMenu);

    return hr;
}

static HRESULT GetStartMenuFolder(IShellFolder ** ppsfStartMenu)
{
    return GetMergedFolder(CSIDL_STARTMENU, CSIDL_COMMON_STARTMENU, ppsfStartMenu);
}

static HRESULT GetProgramsFolder(IShellFolder ** ppsfStartMenu)
{
    return GetMergedFolder(CSIDL_PROGRAMS, CSIDL_COMMON_PROGRAMS, ppsfStartMenu);
}

extern "C"
HRESULT WINAPI
RSHELL_CStartMenu_CreateInstance(REFIID riid, void **ppv)
{
    CComPtr<IShellMenu> pShellMenu;
    CComPtr<IBandSite> pBandSite;
    CComPtr<IDeskBar> pDeskBar;

    HRESULT hr;
    IShellFolder * psf;

    LPITEMIDLIST pidlProgramsAbsolute;
    LPITEMIDLIST pidlPrograms;
    CComPtr<IShellFolder> psfPrograms;

    hr = CMenuBand_CreateInstance(IID_PPV_ARG(IShellMenu, &pShellMenu));
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    hr = CMenuSite_CreateInstance(IID_PPV_ARG(IBandSite, &pBandSite));
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    hr = CMenuDeskBar_CreateInstance(IID_PPV_ARG(IDeskBar, &pDeskBar));
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    CComObject<CShellMenuCallback> *pCallback;
    hr = CComObject<CShellMenuCallback>::CreateInstance(&pCallback);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    pCallback->AddRef(); // CreateInstance returns object with 0 ref count */
    pCallback->Initialize(pShellMenu, pBandSite, pDeskBar);

    hr = pShellMenu->Initialize(pCallback, (UINT) -1, 0, SMINIT_TOPLEVEL | SMINIT_VERTICAL);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    hr = GetStartMenuFolder(&psf);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    /* psf is a merged folder, so now we want to get the pidl of the programs item from the merged folder */
    {
        hr = SHGetSpecialFolderLocation(NULL, CSIDL_PROGRAMS, &pidlProgramsAbsolute);
        if (FAILED_UNEXPECTEDLY(hr))
        {
            WARN("USER Programs folder not found.\n");
            hr = SHGetSpecialFolderLocation(NULL, CSIDL_COMMON_PROGRAMS, &pidlProgramsAbsolute);
            if (FAILED_UNEXPECTEDLY(hr))
                return hr;
        }

        LPCITEMIDLIST pcidlPrograms;
        CComPtr<IShellFolder> psfParent;
        STRRET str;
        TCHAR szDisplayName[MAX_PATH];

        hr = SHBindToParent(pidlProgramsAbsolute, IID_PPV_ARG(IShellFolder, &psfParent), &pcidlPrograms);
        if (FAILED_UNEXPECTEDLY(hr))
            return hr;

        hr = psfParent->GetDisplayNameOf(pcidlPrograms, SHGDN_FORPARSING | SHGDN_INFOLDER, &str);
        if (FAILED_UNEXPECTEDLY(hr))
            return hr;

        StrRetToBuf(&str, pcidlPrograms, szDisplayName, _countof(szDisplayName));
        ILFree(pidlProgramsAbsolute);

        /* We got the display name from the fs folder and we parse it with the merged folder here */
        hr = psf->ParseDisplayName(NULL, NULL, szDisplayName, NULL, &pidlPrograms, NULL);
        if (FAILED_UNEXPECTEDLY(hr))
            return hr;
    }

    hr = GetProgramsFolder(&psfPrograms);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    hr = pCallback->_SetProgramsFolder(psfPrograms, pidlPrograms);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    hr = pShellMenu->SetShellFolder(psf, NULL, NULL, SMSET_TOP);
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
