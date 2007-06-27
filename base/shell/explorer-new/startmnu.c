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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <precomp.h>

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
    IContextMenu *pcm;
    HRESULT hRet;
    HMENU hPopup;

    hRet = IShellFolder_GetUIObjectOf(psf,
                                      hWndOwner,
                                      1,
                                      (LPCITEMIDLIST*)&pidl, /* FIXME: shouldn't need a typecast! */
                                      &IID_IContextMenu,
                                      NULL,
                                      (PVOID*)&pcm);
    if (SUCCEEDED(hRet))
    {
        hPopup = CreatePopupMenu();

        if (hPopup != NULL)
        {
            hRet = IContextMenu_QueryContextMenu(pcm,
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

            IContextMenu_Release(pcm);
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
    PSTARTMNU_CTMENU_CTX psmcmc = (PSTARTMNU_CTMENU_CTX)pcmContext;

    if (uiCmdId != 0)
    {
        switch (uiCmdId)
        {
            case ID_SHELL_CMD_FIRST ... ID_SHELL_CMD_LAST:
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

                IContextMenu_InvokeCommand(psmcmc->pcm,
                                           &cmici);
                break;
            }

            default:
                ITrayWindow_ExecContextMenuCmd((ITrayWindow *)Context,
                                               uiCmdId);
                break;
        }
    }

    IContextMenu_Release(psmcmc->pcm);

    HeapFree(hProcessHeap,
             0,
             psmcmc);
}

static VOID
AddStartContextMenuItems(IN HWND hWndOwner,
                         IN HMENU hPopup)
{
    TCHAR szBuf[MAX_PATH];
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
    IShellFolder *psfStart, *psfDesktop;
    IContextMenu *pcm;
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
                hRet = IShellFolder_BindToObject(psfDesktop,
                                                 pidlStart,
                                                 NULL,
                                                 (REFIID)&IID_IShellFolder, /* FIXME: Shouldn't require a typecast */
                                                 (PVOID*)&psfStart);
                if (SUCCEEDED(hRet))
                {
                    hPopup = CreateContextMenuFromShellFolderPidl(hWndOwner,
                                                                  psfStart,
                                                                  pidlLast,
                                                                  &pcm);

                    if (hPopup != NULL)
                    {
                        PSTARTMNU_CTMENU_CTX psmcmc;

                        psmcmc = HeapAlloc(hProcessHeap,
                                           0,
                                           sizeof(*psmcmc));
                        if (psmcmc != NULL)
                        {
                            psmcmc->pcm = pcm;
                            psmcmc->pidl = pidlLast;

                            AddStartContextMenuItems(hWndOwner,
                                                     hPopup);

                            *((PSTARTMNU_CTMENU_CTX*)ppcmContext) = psmcmc;
                            return hPopup;
                        }
                        else
                        {
                            IContextMenu_Release(pcm);

                            DestroyMenu(hPopup);
                            hPopup = NULL;
                        }
                    }

                    IShellFolder_Release(psfStart);
                }

                IShellFolder_Release(psfDesktop);
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

static const IStartMenuSiteVtbl IStartMenuSiteImpl_Vtbl;
static const IServiceProviderVtbl IServiceProviderImpl_Vtbl;
static const IStartMenuCallbackVtbl IStartMenuCallbackImpl_Vtbl;
static const IOleCommandTargetVtbl IOleCommandTargetImpl_Vtbl;

typedef struct
{
    const IStartMenuSiteVtbl *lpVtbl;
    const IServiceProviderVtbl *lpServiceProviderVtbl;
    const IStartMenuCallbackVtbl *lpStartMenuCallbackVtbl;
    const IOleCommandTargetVtbl *lpOleCommandTargetVtbl;
    LONG Ref;

    ITrayWindow *Tray;
} IStartMenuSiteImpl;

static IUnknown *
IUnknown_from_IStartMenuSiteImpl(IStartMenuSiteImpl *This)
{
    return (IUnknown *)&This->lpVtbl;
}

IMPL_CASTS(IStartMenuSite, IStartMenuSite, lpVtbl)
IMPL_CASTS(IServiceProvider, IStartMenuSite, lpServiceProviderVtbl)
IMPL_CASTS(IStartMenuCallback, IStartMenuSite, lpStartMenuCallbackVtbl)
IMPL_CASTS(IOleCommandTarget, IStartMenuSite, lpOleCommandTargetVtbl)

/*******************************************************************/

static ULONG STDMETHODCALLTYPE
IStartMenuSiteImpl_AddRef(IN OUT IStartMenuSite *iface)
{
    IStartMenuSiteImpl *This = IStartMenuSiteImpl_from_IStartMenuSite(iface);

    return InterlockedIncrement(&This->Ref);
}

static VOID
IStartMenuSiteImpl_Free(IN OUT IStartMenuSiteImpl *This)
{
    HeapFree(hProcessHeap,
             0,
             This);
}

static ULONG STDMETHODCALLTYPE
IStartMenuSiteImpl_Release(IN OUT IStartMenuSite *iface)
{
    IStartMenuSiteImpl *This = IStartMenuSiteImpl_from_IStartMenuSite(iface);
    ULONG Ret;

    Ret = InterlockedDecrement(&This->Ref);

    if (Ret == 0)
        IStartMenuSiteImpl_Free(This);

    return Ret;
}

static HRESULT STDMETHODCALLTYPE
IStartMenuSiteImpl_QueryInterface(IN OUT IStartMenuSite *iface,
                                  IN REFIID riid,
                                  OUT LPVOID *ppvObj)
{
    IStartMenuSiteImpl *This;

    if (ppvObj == NULL)
        return E_POINTER;

    This = IStartMenuSiteImpl_from_IStartMenuSite(iface);

    if (IsEqualIID(riid,
                   &IID_IUnknown))
    {
        *ppvObj = IUnknown_from_IStartMenuSiteImpl(This);
    }
    else if (IsEqualIID(riid,
                        &IID_IServiceProvider))
    {
        *ppvObj = IServiceProvider_from_IStartMenuSiteImpl(This);
    }
    else if (IsEqualIID(riid,
                        &IID_IStartMenuCallback) ||
             IsEqualIID(riid,
                        &IID_IOleWindow))
    {
        *ppvObj = IStartMenuCallback_from_IStartMenuSiteImpl(This);
    }
    else if (IsEqualIID(riid,
                        &IID_IOleCommandTarget))
    {
        *ppvObj = IOleCommandTarget_from_IStartMenuSiteImpl(This);
    }
    else
    {
        DbgPrint("IStartMenuSite::QueryInterface queried unsupported interface: "
                 "{0x%8x,0x%4x,0x%4x,{0x%2x,0x%2x,0x%2x,0x%2x,0x%2x,0x%2x,0x%2x,0x%2x}}\n",
                 riid->Data1, riid->Data2, riid->Data3, riid->Data4[0], riid->Data4[1],
                 riid->Data4[2], riid->Data4[3], riid->Data4[4], riid->Data4[5],
                 riid->Data4[6], riid->Data4[7]);
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }

    IStartMenuSiteImpl_AddRef(iface);
    return S_OK;
}

static const IStartMenuSiteVtbl IStartMenuSiteImpl_Vtbl =
{
    /*** IUnknown methods ***/
    IStartMenuSiteImpl_QueryInterface,
    IStartMenuSiteImpl_AddRef,
    IStartMenuSiteImpl_Release,
    /*** IStartMenuSite methods ***/
};

/*******************************************************************/

METHOD_IUNKNOWN_INHERITED_ADDREF(IServiceProvider, IStartMenuSite)
METHOD_IUNKNOWN_INHERITED_RELEASE(IServiceProvider, IStartMenuSite)
METHOD_IUNKNOWN_INHERITED_QUERYINTERFACE(IServiceProvider, IStartMenuSite)

static HRESULT STDMETHODCALLTYPE
IStartMenuSiteImpl_QueryService(IN OUT IServiceProvider *iface,
                                IN REFGUID guidService,
                                IN REFIID riid,
                                OUT PVOID *ppvObject)
{
    IStartMenuSiteImpl *This = IStartMenuSiteImpl_from_IServiceProvider(iface);

    if (IsEqualGUID(guidService,
                    &SID_SMenuPopup))
    {
        return IStartMenuSiteImpl_QueryInterface(IStartMenuSite_from_IStartMenuSiteImpl(This),
                                                 riid,
                                                 ppvObject);
    }

    return E_NOINTERFACE;
}

static const IServiceProviderVtbl IServiceProviderImpl_Vtbl =
{
    /*** IUnknown methods ***/
    METHOD_IUNKNOWN_INHERITED_QUERYINTERFACE_NAME(IServiceProvider, IStartMenuSite),
    METHOD_IUNKNOWN_INHERITED_ADDREF_NAME(IServiceProvider, IStartMenuSite),
    METHOD_IUNKNOWN_INHERITED_RELEASE_NAME(IServiceProvider, IStartMenuSite),
    /*** IServiceProvider methods ***/
    IStartMenuSiteImpl_QueryService
};

/*******************************************************************/

METHOD_IUNKNOWN_INHERITED_ADDREF(IStartMenuCallback, IStartMenuSite)
METHOD_IUNKNOWN_INHERITED_RELEASE(IStartMenuCallback, IStartMenuSite)
METHOD_IUNKNOWN_INHERITED_QUERYINTERFACE(IStartMenuCallback, IStartMenuSite)

static HRESULT STDMETHODCALLTYPE
IStartMenuSiteImpl_GetWindow(IN OUT IStartMenuCallback *iface,
                             OUT HWND *phwnd)
{
    IStartMenuSiteImpl *This = IStartMenuSiteImpl_from_IStartMenuCallback(iface);
    DbgPrint("IStartMenuCallback::GetWindow\n");

    *phwnd = ITrayWindow_GetHWND(This->Tray);
    if (*phwnd != NULL)
        return S_OK;

    return E_FAIL;
}

static HRESULT STDMETHODCALLTYPE
IStartMenuSiteImpl_ContextSensitiveHelp(IN OUT IStartMenuCallback *iface,
                                        IN BOOL fEnterMode)
{
    DbgPrint("IStartMenuCallback::ContextSensitiveHelp\n");
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE
IStartMenuSiteImpl_Execute(IN OUT IStartMenuCallback *iface,
                           IN IShellFolder *pShellFolder,
                           IN LPCITEMIDLIST pidl)
{
    IStartMenuSiteImpl *This = IStartMenuSiteImpl_from_IStartMenuCallback(iface);

    DbgPrint("IStartMenuCallback::Execute\n");
    return SHInvokeDefaultCommand(ITrayWindow_GetHWND(This->Tray),
                                  pShellFolder,
                                  pidl);
}

static HRESULT STDMETHODCALLTYPE
IStartMenuSiteImpl_Unknown(IN OUT IStartMenuCallback *iface,
                           IN PVOID Unknown1,
                           IN PVOID Unknown2,
                           IN PVOID Unknown3,
                           IN PVOID Unknown4)
{
    DbgPrint("IStartMenuCallback::Unknown(0x%p,0x%p,0x%p,0x%p)\n", Unknown1, Unknown2, Unknown3, Unknown4);
    return E_NOTIMPL;
}

static BOOL
ShowUndockMenuItem(VOID)
{
    DbgPrint("ShowUndockMenuItem() not implemented!\n");
    /* FIXME: How do we detect this?! */
    return FALSE;
}

static BOOL
ShowSynchronizeMenuItem(VOID)
{
    DbgPrint("ShowSynchronizeMenuItem() not implemented!\n");
    /* FIXME: How do we detect this?! */
    return FALSE;
}

static HRESULT STDMETHODCALLTYPE
IStartMenuSiteImpl_AppendMenu(IN OUT IStartMenuCallback *iface,
                              OUT HMENU* phMenu)
{
    HMENU hMenu, hSettingsMenu;
    DWORD dwLogoff;
    BOOL bWantLogoff;
    UINT uLastItemsCount = 5; /* 5 menu items below the last separator */
    TCHAR szUser[128];

    DbgPrint("IStartMenuCallback::AppendMenu\n");

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

    /* FIXME: Favorites */

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
    if (SHRestricted(REST_NOFIND))
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

static const IStartMenuCallbackVtbl IStartMenuCallbackImpl_Vtbl =
{
    /*** IUnknown methods ***/
    METHOD_IUNKNOWN_INHERITED_QUERYINTERFACE_NAME(IStartMenuCallback, IStartMenuSite),
    METHOD_IUNKNOWN_INHERITED_ADDREF_NAME(IStartMenuCallback, IStartMenuSite),
    METHOD_IUNKNOWN_INHERITED_RELEASE_NAME(IStartMenuCallback, IStartMenuSite),
    /*** IOleWindow methods ***/
    IStartMenuSiteImpl_GetWindow,
    IStartMenuSiteImpl_ContextSensitiveHelp,
    /*** IStartMenuCallback methods ***/
    IStartMenuSiteImpl_Execute,
    IStartMenuSiteImpl_Unknown,
    IStartMenuSiteImpl_AppendMenu
};

/*******************************************************************/

METHOD_IUNKNOWN_INHERITED_ADDREF(IOleCommandTarget, IStartMenuSite)
METHOD_IUNKNOWN_INHERITED_RELEASE(IOleCommandTarget, IStartMenuSite)
METHOD_IUNKNOWN_INHERITED_QUERYINTERFACE(IOleCommandTarget, IStartMenuSite)

static HRESULT STDMETHODCALLTYPE
IStartMenuSiteImpl_QueryStatus(IN OUT IOleCommandTarget *iface,
                               IN const GUID *pguidCmdGroup  OPTIONAL,
                               IN ULONG cCmds,
                               IN OUT OLECMD *prgCmds,
                               IN OUT OLECMDTEXT *pCmdText  OPTIONAL)
{
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE
IStartMenuSiteImpl_Exec(IN OUT IOleCommandTarget *iface,
                        IN const GUID *pguidCmdGroup  OPTIONAL,
                        IN DWORD nCmdID,
                        IN DWORD nCmdExecOpt,
                        IN VARIANTARG *pvaIn  OPTIONAL,
                        IN VARIANTARG *pvaOut  OPTIONAL)
{
    return E_NOTIMPL;
}

static const IOleCommandTargetVtbl IOleCommandTargetImpl_Vtbl =
{
    /*** IUnknown methods ***/
    METHOD_IUNKNOWN_INHERITED_QUERYINTERFACE_NAME(IOleCommandTarget, IStartMenuSite),
    METHOD_IUNKNOWN_INHERITED_ADDREF_NAME(IOleCommandTarget, IStartMenuSite),
    METHOD_IUNKNOWN_INHERITED_RELEASE_NAME(IOleCommandTarget, IStartMenuSite),
    /*** IOleCommandTarget ***/
    IStartMenuSiteImpl_QueryStatus,
    IStartMenuSiteImpl_Exec
};

/*******************************************************************/

static IStartMenuSiteImpl*
IStartMenuSiteImpl_Construct(IN ITrayWindow *Tray)
{
    IStartMenuSiteImpl *This;

    This = HeapAlloc(hProcessHeap,
                     0,
                     sizeof(*This));
    if (This == NULL)
        return NULL;

    ZeroMemory(This,
               sizeof(*This));

    This->lpVtbl = &IStartMenuSiteImpl_Vtbl;
    This->lpServiceProviderVtbl = &IServiceProviderImpl_Vtbl;
    This->lpStartMenuCallbackVtbl = &IStartMenuCallbackImpl_Vtbl;
    This->lpOleCommandTargetVtbl = &IOleCommandTargetImpl_Vtbl;
    This->Ref = 1;

    This->Tray = Tray;

    return This;
}

static IStartMenuSite*
CreateStartMenuSite(IN ITrayWindow *Tray)
{
    IStartMenuSiteImpl *This;

    This = IStartMenuSiteImpl_Construct(Tray);
    if (This != NULL)
    {
        return IStartMenuSite_from_IStartMenuSiteImpl(This);
    }

    return NULL;
}

HRESULT
UpdateStartMenu(IN OUT IMenuPopup *pMenuPopup,
                IN HBITMAP hbmBanner  OPTIONAL,
                IN BOOL bSmallIcons)
{
    IBanneredBar *pbb;
    HRESULT hRet;

    hRet = IMenuPopup_QueryInterface(pMenuPopup,
                                     &IID_IBanneredBar,
                                     (PVOID)&pbb);
    if (SUCCEEDED(hRet))
    {
        hRet = IBanneredBar_SetBitmap(pbb,
                                      hbmBanner);


        /* Update the icon size */
        hRet = IBanneredBar_SetIconSize(pbb,
                                        bSmallIcons ? BMICON_SMALL : BMICON_LARGE);

        IBanneredBar_Release(pbb);
    }

    return hRet;
}

IMenuPopup*
CreateStartMenu(IN ITrayWindow *Tray,
                OUT IMenuBand **ppMenuBand,
                IN HBITMAP hbmBanner  OPTIONAL,
                IN BOOL bSmallIcons)
{
    HRESULT hRet;
    IObjectWithSite *pOws = NULL;
    IMenuPopup *pMp = NULL;
    IStartMenuSite *pSms = NULL;
    IMenuBand *pMb = NULL;
    IInitializeObject *pIo;
    IUnknown *pUnk;
    IBandSite *pBs;
    DWORD dwBandId = 0;

    pSms = CreateStartMenuSite(Tray);
    if (pSms == NULL)
        return NULL;

    hRet = CoCreateInstance(&CLSID_StartMenu,
                            NULL,
                            CLSCTX_INPROC_SERVER,
                            &IID_IMenuPopup,
                            (PVOID*)&pMp);
    if (SUCCEEDED(hRet))
    {
        hRet = IMenuPopup_QueryInterface(pMp,
                                         &IID_IObjectWithSite,
                                         (PVOID*)&pOws);
        if (SUCCEEDED(hRet))
        {
            /* Set the menu site so we can handle messages */
            hRet = IObjectWithSite_SetSite(pOws,
                                           (IUnknown*)pSms);
            if (SUCCEEDED(hRet))
            {
                /* Initialize the menu object */
                hRet = IMenuPopup_QueryInterface(pMp,
                                                 &IID_IInitializeObject,
                                                 (PVOID*)&pIo);
                if (SUCCEEDED(hRet))
                {
                    hRet = IInitializeObject_Initialize(pIo);

                    IInitializeObject_Release(pIo);
                }
                else
                    hRet = S_OK;

                /* Everything is initialized now. Let's get the IMenuBand interface. */
                if (SUCCEEDED(hRet))
                {
                    hRet = IMenuPopup_GetClient(pMp,
                                                &pUnk);

                    if (SUCCEEDED(hRet))
                    {
                        hRet = IUnknown_QueryInterface(pUnk,
                                                       &IID_IBandSite,
                                                       (PVOID*)&pBs);

                        if (SUCCEEDED(hRet))
                        {
                            /* Finally we have the IBandSite interface, there's only one
                               band in it that apparently provides the IMenuBand interface */
                            hRet = IBandSite_EnumBands(pBs,
                                                       0,
                                                       &dwBandId);
                            if (SUCCEEDED(hRet))
                            {
                                hRet = IBandSite_GetBandObject(pBs,
                                                               dwBandId,
                                                               &IID_IMenuBand,
                                                               (PVOID*)&pMb);
                            }

                            IBandSite_Release(pBs);
                        }

                        IUnknown_Release(pUnk);
                    }
                }
            }

            IObjectWithSite_Release(pOws);
        }
    }

    IStartMenuSite_Release(pSms);

    if (!SUCCEEDED(hRet))
    {
        DbgPrint("Failed to initialize the start menu: 0x%x!\n", hRet);

        if (pMp != NULL)
            IMenuPopup_Release(pMp);

        if (pMb != NULL)
            IMenuBand_Release(pMb);

        return NULL;
    }

    UpdateStartMenu(pMp,
                    hbmBanner,
                    bSmallIcons);

    *ppMenuBand = pMb;
    return pMp;
}
