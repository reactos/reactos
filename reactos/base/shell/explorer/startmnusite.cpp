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

/*****************************************************************************
 ** IStartMenuSite ***********************************************************
 *****************************************************************************/

class CStartMenuSite :
    public CComCoClass<CStartMenuSite>,
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IStartMenuSite,
    public IServiceProvider,
    public ITrayPriv,
    public IOleCommandTarget,
    public IMenuPopup
{
    CComPtr<ITrayWindow> Tray;
    CComPtr<IMenuPopup> StartMenuPopup;

public:
    CStartMenuSite()
    {
    }

    virtual ~CStartMenuSite() {}

    /*******************************************************************/

    virtual HRESULT STDMETHODCALLTYPE QueryService(
        IN REFGUID guidService,
        IN REFIID riid,
        OUT PVOID *ppvObject)
    {
        if (IsEqualGUID(guidService, SID_SMenuPopup))
        {
            return QueryInterface(riid, ppvObject);
        }

        return E_NOINTERFACE;
    }

    /*******************************************************************/

    virtual HRESULT STDMETHODCALLTYPE GetWindow(
        OUT HWND *phwnd)
    {
        TRACE("ITrayPriv::GetWindow\n");

        *phwnd = Tray->GetHWND();
        if (*phwnd != NULL)
            return S_OK;

        return E_FAIL;
    }

    virtual HRESULT STDMETHODCALLTYPE ContextSensitiveHelp(
        IN BOOL fEnterMode)
    {
        TRACE("ITrayPriv::ContextSensitiveHelp\n");
        return E_NOTIMPL;
    }

    virtual HRESULT STDMETHODCALLTYPE Execute(
        IN IShellFolder *pShellFolder,
        IN LPCITEMIDLIST pidl)
    {
        HRESULT ret = S_FALSE;

        TRACE("ITrayPriv::Execute\n");

        ret = SHInvokeDefaultCommand(Tray->GetHWND(), pShellFolder, pidl);

        return ret;
    }

    virtual HRESULT STDMETHODCALLTYPE Unknown(
        IN PVOID Unknown1,
        IN PVOID Unknown2,
        IN PVOID Unknown3,
        IN PVOID Unknown4)
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

    virtual HRESULT STDMETHODCALLTYPE AppendMenu(
        OUT HMENU* phMenu)
    {
        HMENU hMenu, hSettingsMenu;
        DWORD dwLogoff;
        BOOL bWantLogoff;
        UINT uLastItemsCount = 5; /* 5 menu items below the last separator */
        WCHAR szUser[128];

        TRACE("ITrayPriv::AppendMenu\n");

        hMenu = LoadPopupMenu(hExplorerInstance,
                              MAKEINTRESOURCE(IDM_STARTMENU));
        *phMenu = hMenu;
        if (hMenu == NULL)
            return E_FAIL;

        /* Remove menu items that don't apply */

        dwLogoff = SHRestricted(REST_STARTMENULOGOFF);
        bWantLogoff = (dwLogoff == 2 ||
                       SHRestricted(REST_FORCESTARTMENULOGOFF) ||
                       GetExplorerRegValueSet(HKEY_CURRENT_USER,
                       TEXT("Advanced"),
                       TEXT("StartMenuLogoff")));

        /* Favorites */
        if (!GetExplorerRegValueSet(HKEY_CURRENT_USER,
            TEXT("Advanced"),
            TEXT("StartMenuFavorites")))
        {
            DeleteMenu(hMenu,
                       IDM_FAVORITES,
                       MF_BYCOMMAND);
        }

        /* Documents */
        if (SHRestricted(REST_NORECENTDOCSMENU))
        {
            DeleteMenu(hMenu,
                       IDM_DOCUMENTS,
                       MF_BYCOMMAND);
        }

        /* Settings */
        hSettingsMenu = FindSubMenu(hMenu,
                                    IDM_SETTINGS,
                                    FALSE);
        if (hSettingsMenu != NULL)
        {
            if (SHRestricted(REST_NOSETFOLDERS))
            {
                /* Control Panel */
                if (SHRestricted(REST_NOCONTROLPANEL))
                {
                    DeleteMenu(hSettingsMenu,
                               IDM_CONTROLPANEL,
                               MF_BYCOMMAND);

                    /* Delete the separator below it */
                    DeleteMenu(hSettingsMenu,
                               0,
                               MF_BYPOSITION);
                }

                /* Network Connections */
                if (SHRestricted(REST_NONETWORKCONNECTIONS))
                {
                    DeleteMenu(hSettingsMenu,
                               IDM_NETWORKCONNECTIONS,
                               MF_BYCOMMAND);
                }

                /* Printers and Faxes */
                DeleteMenu(hSettingsMenu,
                           IDM_PRINTERSANDFAXES,
                           MF_BYCOMMAND);
            }

            /* Security */
            if (GetSystemMetrics(SM_REMOTECONTROL) == 0 ||
                SHRestricted(REST_NOSECURITY))
            {
                DeleteMenu(hSettingsMenu,
                           IDM_SECURITY,
                           MF_BYCOMMAND);
            }

            if (GetMenuItemCount(hSettingsMenu) == 0)
            {
                DeleteMenu(hMenu,
                           IDM_SETTINGS,
                           MF_BYCOMMAND);
            }
        }

        /* Search */
        /* FIXME: Enable after implementing */
        /* if (SHRestricted(REST_NOFIND)) */
    {
        DeleteMenu(hMenu,
                   IDM_SEARCH,
                   MF_BYCOMMAND);
    }

        /* FIXME: Help */

        /* Run */
        if (SHRestricted(REST_NORUN))
        {
            DeleteMenu(hMenu,
                       IDM_RUN,
                       MF_BYCOMMAND);
        }

        /* Synchronize */
        if (!ShowSynchronizeMenuItem())
        {
            DeleteMenu(hMenu,
                       IDM_SYNCHRONIZE,
                       MF_BYCOMMAND);
            uLastItemsCount--;
        }

        /* Log off */
        if (dwLogoff != 1 && bWantLogoff)
        {
            /* FIXME: We need a more sophisticated way to determine whether to show
                      or hide it, it might be hidden in too many cases!!! */

            /* Update Log Off menu item */
            if (!GetCurrentLoggedOnUserName(szUser,
                sizeof(szUser) / sizeof(szUser[0])))
            {
                szUser[0] = _T('\0');
            }

            if (!FormatMenuString(hMenu,
                IDM_LOGOFF,
                MF_BYCOMMAND,
                szUser))
            {
                /* We couldn't update the menu item, delete it... */
                DeleteMenu(hMenu,
                           IDM_LOGOFF,
                           MF_BYCOMMAND);
            }
        }
        else
        {
            DeleteMenu(hMenu,
                       IDM_LOGOFF,
                       MF_BYCOMMAND);
            uLastItemsCount--;
        }


        /* Disconnect */
        if (GetSystemMetrics(SM_REMOTECONTROL) == 0)
        {
            DeleteMenu(hMenu,
                       IDM_DISCONNECT,
                       MF_BYCOMMAND);
            uLastItemsCount--;
        }

        /* Undock computer */
        if (!ShowUndockMenuItem())
        {
            DeleteMenu(hMenu,
                       IDM_UNDOCKCOMPUTER,
                       MF_BYCOMMAND);
            uLastItemsCount--;
        }

        /* Shut down */
        if (SHRestricted(REST_NOCLOSE))
        {
            DeleteMenu(hMenu,
                       IDM_SHUTDOWN,
                       MF_BYCOMMAND);
            uLastItemsCount--;
        }

        if (uLastItemsCount == 0)
        {
            /* Remove the separator at the end of the menu */
            DeleteMenu(hMenu,
                       IDM_LASTSTARTMENU_SEPARATOR,
                       MF_BYCOMMAND);
        }

        return S_OK;
    }

    /*******************************************************************/

    virtual HRESULT STDMETHODCALLTYPE QueryStatus(
        IN const GUID *pguidCmdGroup  OPTIONAL,
        IN ULONG cCmds,
        IN OUT OLECMD *prgCmds,
        IN OUT OLECMDTEXT *pCmdText  OPTIONAL)
    {
        return E_NOTIMPL;
    }

    virtual HRESULT STDMETHODCALLTYPE Exec(
        IN const GUID *pguidCmdGroup  OPTIONAL,
        IN DWORD nCmdID,
        IN DWORD nCmdExecOpt,
        IN VARIANTARG *pvaIn  OPTIONAL,
        IN VARIANTARG *pvaOut  OPTIONAL)
    {
        return E_NOTIMPL;
    }

    /*******************************************************************/

    virtual HRESULT STDMETHODCALLTYPE SetClient(IUnknown *punkClient)
    {
        return E_NOTIMPL;
    }

    virtual HRESULT STDMETHODCALLTYPE GetClient(IUnknown ** ppunkClient)
    {
        return E_NOTIMPL;
    }

    virtual HRESULT STDMETHODCALLTYPE OnPosRectChangeDB(RECT *prc)
    {
        return E_NOTIMPL;
    }

    virtual HRESULT STDMETHODCALLTYPE Popup(POINTL *ppt, RECTL *prcExclude, MP_POPUPFLAGS dwFlags)
    {
        return E_NOTIMPL;
    }

    virtual HRESULT STDMETHODCALLTYPE OnSelect(DWORD dwSelectType)
    {
        return E_NOTIMPL;
    }

    virtual HRESULT STDMETHODCALLTYPE SetSubMenu(IMenuPopup *pmp, BOOL fSet)
    {
        if (!fSet)
        {
            return Tray_OnStartMenuDismissed();
        }

        return S_OK;
    }

    /*******************************************************************/

    HRESULT Initialize(IN ITrayWindow *tray)
    {
        Tray = tray;
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

HRESULT CreateStartMenuSite(IN OUT ITrayWindow *Tray, const IID & riid, PVOID * ppv)
{
    return ShellObjectCreatorInit<CStartMenuSite>(Tray, riid, ppv);
}
