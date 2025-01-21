/*
 * ReactOS Explorer
 *
 * Copyright 2006 - 2007 Thomas Weidenmueller <w3seek@reactos.org>
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

class CStartMenuSite :
    public CComCoClass<CStartMenuSite>,
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IServiceProvider,
    public ITrayPriv,
    public IOleCommandTarget,
    public IMenuPopup
{
    CComPtr<ITrayWindow> m_Tray;

public:
    CStartMenuSite()
    {
    }

    virtual ~CStartMenuSite() {}

    // *** IServiceProvider methods ***

    STDMETHODIMP
    QueryService(
        IN REFGUID guidService,
        IN REFIID riid,
        OUT PVOID *ppvObject) override
    {
        if (IsEqualGUID(guidService, SID_SMenuPopup))
        {
            return QueryInterface(riid, ppvObject);
        }

        return E_NOINTERFACE;
    }

    // *** IOleWindow methods ***

    STDMETHODIMP
    GetWindow(OUT HWND *phwnd) override
    {
        TRACE("ITrayPriv::GetWindow\n");

        *phwnd = m_Tray->GetHWND();
        if (*phwnd != NULL)
            return S_OK;

        return E_FAIL;
    }

    STDMETHODIMP
    ContextSensitiveHelp(IN BOOL fEnterMode) override
    {
        TRACE("ITrayPriv::ContextSensitiveHelp\n");
        return E_NOTIMPL;
    }

    STDMETHODIMP
    Execute(
        IN IShellFolder *pShellFolder,
        IN LPCITEMIDLIST pidl) override
    {
        HRESULT ret = S_FALSE;

        TRACE("ITrayPriv::Execute\n");

        ret = SHInvokeDefaultCommand(m_Tray->GetHWND(), pShellFolder, pidl);

        return ret;
    }

    STDMETHODIMP
    Unknown(
        IN PVOID Unknown1,
        IN PVOID Unknown2,
        IN PVOID Unknown3,
        IN PVOID Unknown4) override
    {
        TRACE("ITrayPriv::Unknown(0x%p,0x%p,0x%p,0x%p)\n", Unknown1, Unknown2, Unknown3, Unknown4);
        return E_NOTIMPL;
    }

    virtual BOOL
        ShowUndockMenuItem(VOID)
    {
        TRACE("ShowUndockMenuItem() not implemented!\n");
        /* FIXME: How do we detect this?! */
        return FALSE;
    }

    virtual BOOL
        ShowSynchronizeMenuItem(VOID)
    {
        TRACE("ShowSynchronizeMenuItem() not implemented!\n");
        /* FIXME: How do we detect this?! */
        return FALSE;
    }

    STDMETHODIMP
    AppendMenu(OUT HMENU* phMenu) override
    {
        HMENU hMenu, hSettingsMenu;
        DWORD dwLogoff;
        BOOL bWantLogoff;
        UINT uLastItemsCount = 5; /* 5 menu items below the last separator */
        WCHAR szUser[128];

        TRACE("ITrayPriv::AppendMenu\n");

        hMenu = LoadPopupMenu(hExplorerInstance, MAKEINTRESOURCEW(IDM_STARTMENU));
        *phMenu = hMenu;
        if (hMenu == NULL)
            return E_FAIL;

        /* Remove menu items that don't apply */

        /* Favorites */
        if (SHRestricted(REST_NOFAVORITESMENU) ||
            !GetAdvancedBool(L"StartMenuFavorites", FALSE))
        {
            DeleteMenu(hMenu, IDM_FAVORITES, MF_BYCOMMAND);
        }

        /* Documents */
        if (SHRestricted(REST_NORECENTDOCSMENU) ||
            !GetAdvancedBool(L"Start_ShowRecentDocs", TRUE))
        {
            DeleteMenu(hMenu, IDM_DOCUMENTS, MF_BYCOMMAND);
        }

        /* Settings */
        hSettingsMenu = FindSubMenu(hMenu, IDM_SETTINGS, FALSE);

        /* Control Panel */
        if (SHRestricted(REST_NOSETFOLDERS) ||
            SHRestricted(REST_NOCONTROLPANEL) ||
            !GetAdvancedBool(L"Start_ShowControlPanel", TRUE))
        {
            DeleteMenu(hSettingsMenu, IDM_CONTROLPANEL, MF_BYCOMMAND);

            /* Delete the separator below it */
            DeleteMenu(hSettingsMenu, 0, MF_BYPOSITION);
        }

        /* Network Connections */
        if (SHRestricted(REST_NOSETFOLDERS) ||
            SHRestricted(REST_NONETWORKCONNECTIONS) ||
            !GetAdvancedBool(L"Start_ShowNetConn", TRUE))
        {
            DeleteMenu(hSettingsMenu, IDM_NETWORKCONNECTIONS, MF_BYCOMMAND);
        }

        /* Printers and Faxes */
        if (SHRestricted(REST_NOSETFOLDERS) ||
            !GetAdvancedBool(L"Start_ShowPrinters", TRUE))
        {
            DeleteMenu(hSettingsMenu, IDM_PRINTERSANDFAXES, MF_BYCOMMAND);
        }

        /* Security */
        if (SHRestricted(REST_NOSETFOLDERS) ||
            GetSystemMetrics(SM_REMOTECONTROL) == 0 ||
            SHRestricted(REST_NOSECURITY))
        {
            DeleteMenu(hSettingsMenu, IDM_SECURITY, MF_BYCOMMAND);
        }

        /* Delete Settings menu if it was empty */
        if (GetMenuItemCount(hSettingsMenu) == 0)
        {
            DeleteMenu(hMenu, IDM_SETTINGS, MF_BYCOMMAND);
        }

        /* Search */
        if (SHRestricted(REST_NOFIND) ||
            !GetAdvancedBool(L"Start_ShowSearch", TRUE))
        {
            DeleteMenu(hMenu, IDM_SEARCH, MF_BYCOMMAND);
        }

        /* Help */
        if (SHRestricted(REST_NOSMHELP) ||
            !GetAdvancedBool(L"Start_ShowHelp", TRUE))
        {
            DeleteMenu(hMenu, IDM_HELPANDSUPPORT, MF_BYCOMMAND);
        }

        /* Run */
        if (SHRestricted(REST_NORUN) ||
            !GetAdvancedBool(L"StartMenuRun", TRUE))
        {
            DeleteMenu(hMenu, IDM_RUN, MF_BYCOMMAND);
        }

        /* Synchronize */
        if (!ShowSynchronizeMenuItem())
        {
            DeleteMenu(hMenu, IDM_SYNCHRONIZE, MF_BYCOMMAND);
            uLastItemsCount--;
        }

        /* Log off */
        dwLogoff = SHRestricted(REST_STARTMENULOGOFF);
        bWantLogoff = (dwLogoff == 2 ||
                       SHRestricted(REST_FORCESTARTMENULOGOFF) ||
                       GetAdvancedBool(L"StartMenuLogoff", FALSE));
        if (dwLogoff != 1 && bWantLogoff)
        {
            /* FIXME: We need a more sophisticated way to determine whether to show
                      or hide it, it might be hidden in too many cases!!! */

            /* Update Log Off menu item */
            if (!GetCurrentLoggedOnUserName(szUser, _countof(szUser)))
            {
                szUser[0] = _T('\0');
            }

            if (!FormatMenuString(hMenu,
                IDM_LOGOFF,
                MF_BYCOMMAND,
                szUser))
            {
                /* We couldn't update the menu item, delete it... */
                DeleteMenu(hMenu, IDM_LOGOFF, MF_BYCOMMAND);
            }
        }
        else
        {
            DeleteMenu(hMenu, IDM_LOGOFF, MF_BYCOMMAND);
            uLastItemsCount--;
        }

        /* Disconnect */
        if (SHRestricted(REST_NODISCONNECT) ||
            GetSystemMetrics(SM_REMOTECONTROL) == 0)
        {
            DeleteMenu(hMenu, IDM_DISCONNECT, MF_BYCOMMAND);
            uLastItemsCount--;
        }

        /* Undock computer */
        if (!ShowUndockMenuItem())
        {
            DeleteMenu(hMenu, IDM_UNDOCKCOMPUTER, MF_BYCOMMAND);
            uLastItemsCount--;
        }

        /* Shut down */
        if (SHRestricted(REST_NOCLOSE))
        {
            DeleteMenu(hMenu, IDM_SHUTDOWN, MF_BYCOMMAND);
            uLastItemsCount--;
        }

        if (uLastItemsCount == 0)
        {
            /* Remove the separator at the end of the menu */
            DeleteMenu(hMenu, IDM_LASTSTARTMENU_SEPARATOR, MF_BYCOMMAND);
        }

        return S_OK;
    }

    /*******************************************************************/

    STDMETHODIMP
    QueryStatus(
        IN const GUID *pguidCmdGroup  OPTIONAL,
        IN ULONG cCmds,
        IN OUT OLECMD *prgCmds,
        IN OUT OLECMDTEXT *pCmdText  OPTIONAL) override
    {
        return E_NOTIMPL;
    }

    STDMETHODIMP
    Exec(
        IN const GUID *pguidCmdGroup  OPTIONAL,
        IN DWORD nCmdID,
        IN DWORD nCmdExecOpt,
        IN VARIANTARG *pvaIn  OPTIONAL,
        IN VARIANTARG *pvaOut  OPTIONAL) override
    {
        return E_NOTIMPL;
    }

    /*******************************************************************/

    STDMETHODIMP
    SetClient(IUnknown *punkClient) override
    {
        return E_NOTIMPL;
    }

    STDMETHODIMP
    GetClient(IUnknown ** ppunkClient) override
    {
        return E_NOTIMPL;
    }

    STDMETHODIMP
    OnPosRectChangeDB(RECT *prc) override
    {
        return E_NOTIMPL;
    }

    // *** IMenuPopup methods ***

    STDMETHODIMP
    Popup(POINTL *ppt, RECTL *prcExclude, MP_POPUPFLAGS dwFlags) override
    {
        return E_NOTIMPL;
    }

    STDMETHODIMP
    OnSelect(DWORD dwSelectType) override
    {
        return E_NOTIMPL;
    }

    STDMETHODIMP
    SetSubMenu(IMenuPopup *pmp, BOOL fSet) override
    {
        if (!fSet)
        {
            return Tray_OnStartMenuDismissed(m_Tray);
        }

        return S_OK;
    }

    /*******************************************************************/

    HRESULT Initialize(IN ITrayWindow *tray)
    {
        m_Tray = tray;
        return S_OK;
    }

    DECLARE_NOT_AGGREGATABLE(CStartMenuSite)

    DECLARE_PROTECT_FINAL_CONSTRUCT()
    BEGIN_COM_MAP(CStartMenuSite)
        COM_INTERFACE_ENTRY_IID(IID_IServiceProvider, IServiceProvider)
        COM_INTERFACE_ENTRY_IID(IID_ITrayPriv, ITrayPriv)
        COM_INTERFACE_ENTRY_IID(IID_IOleCommandTarget, IOleCommandTarget)
        COM_INTERFACE_ENTRY_IID(IID_IMenuPopup, IMenuPopup)
        COM_INTERFACE_ENTRY_IID(IID_IOleWindow, IOleWindow)
    END_COM_MAP()
};

HRESULT CStartMenuSite_CreateInstance(IN OUT ITrayWindow *Tray, const IID & riid, PVOID * ppv)
{
    return ShellObjectCreatorInit<CStartMenuSite>(Tray, riid, ppv);
}
