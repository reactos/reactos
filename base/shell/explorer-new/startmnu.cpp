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

/*
 * Start menu button context menu
 */

typedef struct _STARTMNU_CTMENU_CTX
{
    IContextMenu *pcm;
    LPITEMIDLIST pidl;
} STARTMNU_CTMENU_CTX, *PSTARTMNU_CTMENU_CTX;

static HMENU
CreateStartContextMenu(IN HWND hWndOwner,
                       IN PVOID *ppcmContext,
                       IN PVOID Context  OPTIONAL);

static VOID
OnStartContextMenuCommand(IN HWND hWndOwner,
                          IN UINT uiCmdId,
                          IN PVOID pcmContext  OPTIONAL,
                          IN PVOID Context  OPTIONAL);

const TRAYWINDOW_CTXMENU StartMenuBtnCtxMenu = {
    CreateStartContextMenu,
    OnStartContextMenuCommand
};

static HMENU
CreateContextMenuFromShellFolderPidl(IN HWND hWndOwner,
                                     IN OUT IShellFolder *psf,
                                     IN OUT LPITEMIDLIST pidl,
                                     OUT IContextMenu **ppcm)
{
    CComPtr<IContextMenu> pcm;
    HRESULT hRet;
    HMENU hPopup;

    hRet = psf->GetUIObjectOf(hWndOwner, 1, (LPCITEMIDLIST *)&pidl, IID_NULL_PPV_ARG(IContextMenu, &pcm));
    if (SUCCEEDED(hRet))
    {
        hPopup = CreatePopupMenu();

        if (hPopup != NULL)
        {
            hRet = pcm->QueryContextMenu(
                                                 hPopup,
                                                 0,
                                                 ID_SHELL_CMD_FIRST,
                                                 ID_SHELL_CMD_LAST,
                                                 CMF_VERBSONLY);

            if (SUCCEEDED(hRet))
            {
                *ppcm = pcm;
                return hPopup;
            }

            DestroyMenu(hPopup);
        }
    }

    return NULL;
}

static VOID
OnStartContextMenuCommand(IN HWND hWndOwner,
                          IN UINT uiCmdId,
                          IN PVOID pcmContext  OPTIONAL,
                          IN PVOID Context  OPTIONAL)
{
    PSTARTMNU_CTMENU_CTX psmcmc = (PSTARTMNU_CTMENU_CTX) pcmContext;

    if (uiCmdId != 0)
    {
        if ((uiCmdId >= ID_SHELL_CMD_FIRST) && (uiCmdId <= ID_SHELL_CMD_LAST))
        {
            CMINVOKECOMMANDINFO cmici = {0};
            CHAR szDir[MAX_PATH];

            /* Setup and invoke the shell command */
            cmici.cbSize = sizeof(cmici);
            cmici.hwnd = hWndOwner;
            cmici.lpVerb = (LPCSTR)MAKEINTRESOURCE(uiCmdId - ID_SHELL_CMD_FIRST);
            cmici.nShow = SW_NORMAL;

            /* FIXME: Support Unicode!!! */
            if (SHGetPathFromIDListA(psmcmc->pidl,
                                     szDir))
            {
                cmici.lpDirectory = szDir;
            }

            psmcmc->pcm->InvokeCommand(&cmici);
        }
        else
        {
            ITrayWindow * TrayWnd = (ITrayWindow *) Context;
            TrayWnd->ExecContextMenuCmd(uiCmdId);
        }
    }

    psmcmc->pcm->Release();

    HeapFree(hProcessHeap, 0, psmcmc);
}

static VOID
AddStartContextMenuItems(IN HWND hWndOwner,
                         IN HMENU hPopup)
{
    WCHAR szBuf[MAX_PATH];
    HRESULT hRet;

    /* Add the "Open All Users" menu item */
    if (LoadString(hExplorerInstance,
                   IDS_PROPERTIES,
                   szBuf,
                   sizeof(szBuf) / sizeof(szBuf[0])))
    {
        AppendMenu(hPopup,
                   MF_STRING,
                   ID_SHELL_CMD_PROPERTIES,
                   szBuf);
    }

    if (!SHRestricted(REST_NOCOMMONGROUPS))
    {
        /* Check if we should add menu items for the common start menu */
        hRet = SHGetFolderPath(hWndOwner,
                               CSIDL_COMMON_STARTMENU,
                               NULL,
                               SHGFP_TYPE_CURRENT,
                               szBuf);
        if (SUCCEEDED(hRet) && hRet != S_FALSE)
        {
            /* The directory exists, but only show the items if the
               user can actually make any changes to the common start
               menu. This is most likely only the case if the user
               has administrative rights! */
            if (IsUserAnAdmin())
            {
                AppendMenu(hPopup,
                           MF_SEPARATOR,
                           0,
                           NULL);

                /* Add the "Open All Users" menu item */
                if (LoadString(hExplorerInstance,
                               IDS_OPEN_ALL_USERS,
                               szBuf,
                               sizeof(szBuf) / sizeof(szBuf[0])))
                {
                    AppendMenu(hPopup,
                               MF_STRING,
                               ID_SHELL_CMD_OPEN_ALL_USERS,
                               szBuf);
                }

                /* Add the "Explore All Users" menu item */
                if (LoadString(hExplorerInstance,
                               IDS_EXPLORE_ALL_USERS,
                               szBuf,
                               sizeof(szBuf) / sizeof(szBuf[0])))
                {
                    AppendMenu(hPopup,
                               MF_STRING,
                               ID_SHELL_CMD_EXPLORE_ALL_USERS,
                               szBuf);
                }
            }
        }
    }
}

static HMENU
CreateStartContextMenu(IN HWND hWndOwner,
                       IN PVOID *ppcmContext,
                       IN PVOID Context  OPTIONAL)
{
    LPITEMIDLIST pidlStart, pidlLast;
    CComPtr<IShellFolder> psfStart;
    CComPtr<IShellFolder> psfDesktop;
    CComPtr<IContextMenu> pcm;
    HRESULT hRet;
    HMENU hPopup;

    pidlStart = SHCloneSpecialIDList(hWndOwner,
                                     CSIDL_STARTMENU,
                                     TRUE);

    if (pidlStart != NULL)
    {
        pidlLast = ILClone(ILFindLastID(pidlStart));
        ILRemoveLastID(pidlStart);

        if (pidlLast != NULL)
        {
            hRet = SHGetDesktopFolder(&psfDesktop);
            if (SUCCEEDED(hRet))
            {
                hRet = psfDesktop->BindToObject(pidlStart, NULL, IID_PPV_ARG(IShellFolder, &psfStart));
                if (SUCCEEDED(hRet))
                {
                    hPopup = CreateContextMenuFromShellFolderPidl(hWndOwner,
                                                                  psfStart,
                                                                  pidlLast,
                                                                  &pcm);

                    if (hPopup != NULL)
                    {
                        PSTARTMNU_CTMENU_CTX psmcmc;

                        psmcmc = (PSTARTMNU_CTMENU_CTX) HeapAlloc(hProcessHeap, 0, sizeof(*psmcmc));
                        if (psmcmc != NULL)
                        {
                            psmcmc->pcm = pcm;
                            psmcmc->pidl = pidlLast;

                            AddStartContextMenuItems(hWndOwner,
                                                     hPopup);

                            *ppcmContext = psmcmc;
                            return hPopup;
                        }
                        else
                        {
                            DestroyMenu(hPopup);
                            hPopup = NULL;
                        }
                    }
                }
            }

            ILFree(pidlLast);
        }

        ILFree(pidlStart);
    }

    return NULL;
}

/*****************************************************************************
 ** IStartMenuSite ***********************************************************
 *****************************************************************************/

class IStartMenuSiteImpl :
    public CComCoClass<IStartMenuSiteImpl>,
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
    IStartMenuSiteImpl()
    {
    }

    virtual ~IStartMenuSiteImpl() { }

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
        HMODULE hShlwapi;
        HRESULT ret = S_FALSE;

        TRACE("ITrayPriv::Execute\n");

        hShlwapi = GetModuleHandle(TEXT("SHLWAPI.DLL"));
        if (hShlwapi != NULL)
        {
            SHINVDEFCMD SHInvokeDefCmd;

            /* SHInvokeDefaultCommand */
            SHInvokeDefCmd = (SHINVDEFCMD) GetProcAddress(hShlwapi,
                (LPCSTR) ((LONG) 279));
            if (SHInvokeDefCmd != NULL)
            {
                ret = SHInvokeDefCmd(Tray->GetHWND(),
                    pShellFolder,
                    pidl);
            }
        }

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

    DECLARE_NOT_AGGREGATABLE(IStartMenuSiteImpl)

    DECLARE_PROTECT_FINAL_CONSTRUCT()
    BEGIN_COM_MAP(IStartMenuSiteImpl)
        COM_INTERFACE_ENTRY_IID(IID_IServiceProvider, IServiceProvider)
        COM_INTERFACE_ENTRY_IID(IID_ITrayPriv, ITrayPriv)
        COM_INTERFACE_ENTRY_IID(IID_IOleCommandTarget, IOleCommandTarget)
        COM_INTERFACE_ENTRY_IID(IID_IMenuPopup, IMenuPopup)
        COM_INTERFACE_ENTRY_IID(IID_IOleWindow, IOleWindow)
    END_COM_MAP()
};

HRESULT CreateStartMenuSite(IN OUT ITrayWindow *Tray, const IID & riid, PVOID * ppv)
{
    return ShellObjectCreatorInit<IStartMenuSiteImpl>(Tray, riid, ppv);
}

HRESULT
UpdateStartMenu(IN OUT IMenuPopup *pMenuPopup,
                IN HBITMAP hbmBanner  OPTIONAL,
                IN BOOL bSmallIcons)
{
    CComPtr<IBanneredBar> pbb;
    HRESULT hRet;

    hRet = pMenuPopup->QueryInterface(IID_PPV_ARG(IBanneredBar, &pbb));
    if (SUCCEEDED(hRet))
    {
        hRet = pbb->SetBitmap( hbmBanner);

        /* Update the icon size */
        hRet = pbb->SetIconSize(bSmallIcons ? BMICON_SMALL : BMICON_LARGE);
    }

    return hRet;
}

IMenuPopup *
CreateStartMenu(IN ITrayWindow *Tray,
                OUT IMenuBand **ppMenuBand,
                IN HBITMAP hbmBanner  OPTIONAL,
                IN BOOL bSmallIcons)
{
    HRESULT hr;
    IObjectWithSite *pOws = NULL;
    IMenuPopup *pMp = NULL;
    IUnknown *pSms = NULL;
    IMenuBand *pMb = NULL;
    IInitializeObject *pIo;
    IUnknown *pUnk = NULL;
    IBandSite *pBs = NULL;
    DWORD dwBandId = 0;

    hr = CreateStartMenuSite(Tray, IID_PPV_ARG(IUnknown, &pSms));
    if (FAILED_UNEXPECTEDLY(hr))
        return NULL;

#if 0
    hr = CoCreateInstance(&CLSID_StartMenu,
                          NULL,
                          CLSCTX_INPROC_SERVER,
                          &IID_IMenuPopup,
                          (PVOID *)&pMp);
#else
    hr = _CStartMenu_Constructor(IID_PPV_ARG(IMenuPopup, &pMp));
#endif
    if (FAILED_UNEXPECTEDLY(hr))
    {
        TRACE("CoCreateInstance failed: %x\n", hr);
        goto cleanup;
    }

    hr = pMp->QueryInterface(IID_PPV_ARG(IObjectWithSite, &pOws));
    if (FAILED_UNEXPECTEDLY(hr))
    {
        TRACE("IMenuPopup_QueryInterface failed: %x\n", hr);
        goto cleanup;
    }

    /* Set the menu site so we can handle messages */
    hr = pOws->SetSite(pSms);
    if (FAILED_UNEXPECTEDLY(hr))
    {
        TRACE("IObjectWithSite_SetSite failed: %x\n", hr);
        goto cleanup;
    }

    /* Initialize the menu object */
    hr = pMp->QueryInterface(IID_PPV_ARG(IInitializeObject, &pIo));
    if (SUCCEEDED(hr))
    {
        hr = pIo->Initialize();
        pIo->Release();
    }
    else
        hr = S_OK;

    /* Everything is initialized now. Let's get the IMenuBand interface. */
    if (FAILED_UNEXPECTEDLY(hr))
    {
        TRACE("IMenuPopup_QueryInterface failed: %x\n", hr);
        goto cleanup;
    }

    hr = pMp->GetClient( &pUnk);
    if (FAILED_UNEXPECTEDLY(hr))
    {
        TRACE("IMenuPopup_GetClient failed: %x\n", hr);
        goto cleanup;
    }

    hr = pUnk->QueryInterface(IID_PPV_ARG(IBandSite, &pBs));
    if (FAILED_UNEXPECTEDLY(hr))
    {
        TRACE("IUnknown_QueryInterface pBs failed: %x\n", hr);
        goto cleanup;
    }

    /* Finally we have the IBandSite interface, there's only one
       band in it that apparently provides the IMenuBand interface */
    hr = pBs->EnumBands( 0, &dwBandId);
    if (FAILED_UNEXPECTEDLY(hr))
    {
        TRACE("IBandSite_EnumBands failed: %x\n", hr);
        goto cleanup;
    }

    hr = pBs->GetBandObject( dwBandId, IID_PPV_ARG(IMenuBand, &pMb));
    if (FAILED_UNEXPECTEDLY(hr))
    {
        TRACE("IBandSite_GetBandObject failed: %x\n", hr);
        goto cleanup;
    }

    UpdateStartMenu(pMp,
                    hbmBanner,
                    bSmallIcons);

cleanup:
    if (SUCCEEDED(hr))
        *ppMenuBand = pMb;
    else if (pMb != NULL)
        pMb->Release();

    if (pBs != NULL)
        pBs->Release();
    if (pUnk != NULL)
        pUnk->Release();
    if (pOws != NULL)
        pOws->Release();
    if (pMp != NULL)
        pMp->Release();
    if (pSms != NULL)
        pSms->Release();

    if (FAILED_UNEXPECTEDLY(hr))
        return NULL;
    return pMp;
}
