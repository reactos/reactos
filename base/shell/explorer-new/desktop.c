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

#if USE_API_SHCREATEDESKTOP

typedef struct _DESKCREATEINFO
{
    HANDLE hEvent;
    ITrayWindow *Tray;
    HANDLE hDesktop;
} DESKCREATEINFO, *PDESKCREATEINFO;

static DWORD CALLBACK
DesktopThreadProc(IN OUT LPVOID lpParameter)
{
    volatile DESKCREATEINFO *DeskCreateInfo = (volatile DESKCREATEINFO *)lpParameter;
    IShellDesktopTray *pSdt;
    HANDLE hDesktop;
    HRESULT hRet;

    hRet = ITrayWindow_QueryInterface(DeskCreateInfo->Tray,
                                      &IID_IShellDesktopTray,
                                      (PVOID*)&pSdt);
    if (!SUCCEEDED(hRet))
        return 1;

    hDesktop = SHCreateDesktop(pSdt);

    IShellDesktopTray_Release(pSdt);
    if (hDesktop == NULL)
        return 1;

    (void)InterlockedExchangePointer(&DeskCreateInfo->hDesktop,
                                     hDesktop);

    if (!SetEvent(DeskCreateInfo->hEvent))
    {
        /* Failed to notify that we initialized successfully, kill ourselves
           to make the main thread wake up! */
        return 1;
    }

    SHDesktopMessageLoop(hDesktop);

    /* FIXME: Properly rundown the main thread! */
    ExitProcess(0);

    return 0;
}

HANDLE
DesktopCreateWindow(IN OUT ITrayWindow *Tray)
{
    HANDLE hThread;
    HANDLE hEvent;
    DWORD DesktopThreadId;
    HANDLE hDesktop = NULL;
    HANDLE Handles[2];
    DWORD WaitResult;

    hEvent = CreateEvent(NULL,
                         FALSE,
                         FALSE,
                         NULL);
    if (hEvent != NULL)
    {
        volatile DESKCREATEINFO DeskCreateInfo;

        DeskCreateInfo.hEvent = hEvent;
        DeskCreateInfo.Tray = Tray;
        DeskCreateInfo.hDesktop = NULL;

        hThread = CreateThread(NULL,
                               0,
                               DesktopThreadProc,
                               (PVOID)&DeskCreateInfo,
                               0,
                               &DesktopThreadId);
        if (hThread != NULL)
        {
            Handles[0] = hThread;
            Handles[1] = hEvent;

            for (;;)
            {
                WaitResult = MsgWaitForMultipleObjects(sizeof(Handles) / sizeof(Handles[0]),
                                                       Handles,
                                                       FALSE,
                                                       INFINITE,
                                                       QS_ALLEVENTS);
                if (WaitResult == WAIT_OBJECT_0 + (sizeof(Handles) / sizeof(Handles[0])))
                    TrayProcessMessages(Tray);
                else if (WaitResult != WAIT_FAILED && WaitResult != WAIT_OBJECT_0)
                {
                    hDesktop = DeskCreateInfo.hDesktop;
                    break;
                }
            }

            CloseHandle(hThread);
        }

        CloseHandle(hEvent);
    }

    return hDesktop;
}

VOID
DesktopDestroyShellWindow(IN HANDLE hDesktop)
{
    return;
}

#else /* USE_API_SHCREATEDESKTOP == 0 */

/*
 ******************************************************************************
 * NOTE: This could may be reused in a shell implementation of                *
 *       SHCreateDesktop().                                                   *
 ******************************************************************************
 */

#define WM_SHELL_ADDDRIVENOTIFY (WM_USER + 0x100)

static const IShellBrowserVtbl IDesktopShellBrowserImpl_Vtbl;
static const ICommDlgBrowserVtbl IDesktopShellBrowserImpl_ICommDlgBrowser_Vtbl;
static const IServiceProviderVtbl IDesktopShellBrowserImpl_IServiceProvider_Vtbl;
static const IShellFolderViewCBVtbl IDesktopShellBrowserImpl_IShellFolderViewCB_Vtbl;

/*
 * IShellBrowser
 */

typedef struct
{
    const IShellBrowserVtbl *lpVtbl;
    const ICommDlgBrowserVtbl *lpVtblCommDlgBrowser;
    const IServiceProviderVtbl *lpVtblServiceProvider;
    const IShellFolderViewCBVtbl *lpVtblShellFolderViewCB;
    LONG Ref;

    IShellView2 *DesktopView2;
    IShellView *DesktopView;
    IShellBrowser *DefaultShellBrowser;
    HWND hWnd;
    HWND hWndShellView;
    HWND hWndDesktopListView;

    ITrayWindow *Tray;

    LPITEMIDLIST pidlDesktopDirectory;
    LPITEMIDLIST pidlDesktop;
    IDropTarget *DropTarget;

    ULONG ChangeNotificationId;
} IDesktopShellBrowserImpl;

static IUnknown *
IUnknown_from_impl(IDesktopShellBrowserImpl *This)
{
    return (IUnknown *)&This->lpVtbl;
}

static IShellBrowser *
IShellBrowser_from_impl(IDesktopShellBrowserImpl *This)
{
    return (IShellBrowser *)&This->lpVtbl;
}

static IOleWindow *
IOleWindow_from_impl(IDesktopShellBrowserImpl *This)
{
    return (IOleWindow *)&This->lpVtbl;
}

static ICommDlgBrowser *
ICommDlgBrowser_from_impl(IDesktopShellBrowserImpl *This)
{
    return (ICommDlgBrowser *)&This->lpVtblCommDlgBrowser;
}

static IServiceProvider *
IServiceProvider_from_impl(IDesktopShellBrowserImpl *This)
{
    return (IServiceProvider *)&This->lpVtblServiceProvider;
}

static IShellFolderViewCB *
IShellFolderViewCB_from_impl(IDesktopShellBrowserImpl *This)
{
    return (IShellFolderViewCB *)&This->lpVtblShellFolderViewCB;
}

static IDesktopShellBrowserImpl *
impl_from_IShellBrowser(IShellBrowser *iface)
{
    return (IDesktopShellBrowserImpl *)((ULONG_PTR)iface - FIELD_OFFSET(IDesktopShellBrowserImpl,
                                                                        lpVtbl));
}

static IDesktopShellBrowserImpl *
impl_from_ICommDlgBrowser(ICommDlgBrowser *iface)
{
    return (IDesktopShellBrowserImpl *)((ULONG_PTR)iface - FIELD_OFFSET(IDesktopShellBrowserImpl,
                                                                        lpVtblCommDlgBrowser));
}

static IDesktopShellBrowserImpl *
impl_from_IServiceProvider(IServiceProvider *iface)
{
    return (IDesktopShellBrowserImpl *)((ULONG_PTR)iface - FIELD_OFFSET(IDesktopShellBrowserImpl,
                                                                        lpVtblServiceProvider));
}

static IDesktopShellBrowserImpl *
impl_from_IShellFolderViewCB(IShellFolderViewCB *iface)
{
    return (IDesktopShellBrowserImpl *)((ULONG_PTR)iface - FIELD_OFFSET(IDesktopShellBrowserImpl,
                                                                        lpVtblShellFolderViewCB));
}

static VOID
IDesktopShellBrowserImpl_Free(IDesktopShellBrowserImpl *This)
{
    if (This->DropTarget != NULL)
    {
        IDropTarget_Release(This->DropTarget);
        This->DropTarget = NULL;
    }

    if (This->Tray != NULL)
    {
        ITrayWindow_Release(This->Tray);
        This->Tray = NULL;
    }

    if (This->pidlDesktopDirectory != NULL)
    {
        ILFree(This->pidlDesktopDirectory);
        This->pidlDesktopDirectory = NULL;
    }

    if (This->pidlDesktop != NULL)
    {
        ILFree(This->pidlDesktop);
        This->pidlDesktop = NULL;
    }

    HeapFree(hProcessHeap,
             0,
             This);
}

static VOID
IDesktopShellBrowserImpl_Destroy(IDesktopShellBrowserImpl *This)
{
    if (This->ChangeNotificationId != 0)
    {
        SHChangeNotifyDeregister(This->ChangeNotificationId);
        This->ChangeNotificationId = 0;
    }

    if (This->Tray != NULL)
    {
        ITrayWindow_Close(This->Tray);
        This->Tray = NULL;
    }

    if (This->DropTarget != NULL && This->hWndDesktopListView != NULL)
    {
        RevokeDragDrop(This->hWndDesktopListView);
        This->hWndDesktopListView = NULL;
    }

    if (This->DesktopView2 != NULL)
    {
        IShellView2_Release(This->DesktopView2);
    }

    if (This->DesktopView != NULL)
    {
        if (This->hWndShellView != NULL)
        {
            IShellView_DestroyViewWindow(This->DesktopView);
        }

        IShellView_Release(This->DesktopView);
        This->DesktopView = NULL;
        This->hWndShellView = NULL;
        This->hWndDesktopListView = NULL;
    }
}

static HRESULT
IDesktopShellBrowserImpl_GetNotify(IN OUT IDesktopShellBrowserImpl *This,
                                   OUT LPITEMIDLIST *ppidl,
                                   OUT LONG *plEvents)
{
    *ppidl = This->pidlDesktopDirectory;
    *plEvents = SHCNE_DISKEVENTS;
    return S_OK;
}

static ULONG STDMETHODCALLTYPE
IDesktopShellBrowserImpl_Release(IN OUT IShellBrowser *iface)
{
    IDesktopShellBrowserImpl *This = impl_from_IShellBrowser(iface);
    ULONG Ret;

    Ret = InterlockedDecrement(&This->Ref);
    if (Ret == 0)
        IDesktopShellBrowserImpl_Free(This);

    return Ret;
}

static ULONG STDMETHODCALLTYPE
IDesktopShellBrowserImpl_AddRef(IN OUT IShellBrowser *iface)
{
    IDesktopShellBrowserImpl *This = impl_from_IShellBrowser(iface);

    return InterlockedIncrement(&This->Ref);
}

static HRESULT STDMETHODCALLTYPE
IDesktopShellBrowserImpl_QueryInterface(IN OUT IShellBrowser *iface,
                                        IN REFIID riid,
                                        OUT LPVOID *ppvObj)
{
    IDesktopShellBrowserImpl *This;

    if (ppvObj == NULL)
        return E_POINTER;

    This = impl_from_IShellBrowser(iface);

    if (IsEqualIID(riid,
                   &IID_IUnknown))
    {
        *ppvObj = IUnknown_from_impl(This);
    }
    else if (This->DefaultShellBrowser != NULL)
    {
        return IShellBrowser_QueryInterface(This->DefaultShellBrowser,
                                            riid,
                                            ppvObj);
    }
    else if (IsEqualIID(riid,
                        &IID_IOleWindow))
    {
        *ppvObj = IOleWindow_from_impl(This);
    }
    else if (IsEqualIID(riid,
                        &IID_IShellBrowser))
    {
        *ppvObj = IShellBrowser_from_impl(This);
    }
    else if (IsEqualIID(riid,
                        &IID_ICommDlgBrowser))
    {
        *ppvObj = ICommDlgBrowser_from_impl(This);
    }
    else if (IsEqualIID(riid,
                        &IID_IServiceProvider))
    {
        *ppvObj = IServiceProvider_from_impl(This);
    }
    else if (IsEqualIID(riid,
                        &IID_IShellFolderViewCB))
    {
        *ppvObj = IShellFolderViewCB_from_impl(This);
    }
    else
    {
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }

    IDesktopShellBrowserImpl_AddRef(iface);
    return S_OK;
}

static IDesktopShellBrowserImpl *
IDesktopShellBrowserImpl_Construct(IN HWND hwndParent,
                                   IN LPCREATESTRUCT lpCreateStruct  OPTIONAL)
{
    IDesktopShellBrowserImpl *This;
    IShellBrowser *ShellBrowser;
    IShellFolder *psfDesktopFolder;
#if 1
    SFV_CREATE csfv;
#endif
    FOLDERSETTINGS fs;
    HRESULT hr;
    RECT rcClient;
    SHChangeNotifyEntry cne;

    This = HeapAlloc(hProcessHeap,
                     0,
                     sizeof(*This));
    if (This == NULL)
        return NULL;

    ZeroMemory(This,
               sizeof(*This));
    This->lpVtbl = &IDesktopShellBrowserImpl_Vtbl;
    This->lpVtblCommDlgBrowser = &IDesktopShellBrowserImpl_ICommDlgBrowser_Vtbl;
    This->lpVtblServiceProvider = &IDesktopShellBrowserImpl_IServiceProvider_Vtbl;
    This->lpVtblShellFolderViewCB = &IDesktopShellBrowserImpl_IShellFolderViewCB_Vtbl;
    This->Ref = 1;
    This->hWnd = hwndParent;

    ShellBrowser = IShellBrowser_from_impl(This);

    This->pidlDesktopDirectory = SHCloneSpecialIDList(This->hWnd,
                                                      CSIDL_DESKTOPDIRECTORY,
                                                      FALSE);

    hr = SHGetSpecialFolderLocation(This->hWnd,
                                    CSIDL_DESKTOP,
                                    &This->pidlDesktop);
    if (!SUCCEEDED(hr))
        goto Fail;

#if 1
    hr = SHGetDesktopFolder(&psfDesktopFolder);
#else
    hr = CoCreateInstance(&CLSID_ShellDesktop,
                          NULL,
                          CLSCTX_INPROC,
                          &IID_IShellFolder,
                          (PVOID*)&psfDesktopFolder);
#endif
    if (!SUCCEEDED(hr))
        goto Fail;

#if 0
    hr = IShellFolder_CreateViewObject(psfDesktopFolder,
                                       This->hWnd,
                                       &IID_IShellView,
                                       (PVOID*)&This->DesktopView);
#else
    csfv.cbSize = sizeof(csfv);
    csfv.pshf = psfDesktopFolder;
    csfv.psvOuter = NULL;

    hr = IDesktopShellBrowserImpl_QueryInterface(ShellBrowser,
                                                 &IID_IShellFolderViewCB,
                                                 (PVOID*)&csfv.psfvcb);
    if (!SUCCEEDED(hr))
        csfv.psfvcb = NULL;

    hr = SHCreateShellFolderView(&csfv,
                                 &This->DesktopView);

    IShellFolder_Release(psfDesktopFolder);

    if (csfv.psfvcb != NULL)
        csfv.psfvcb->lpVtbl->Release(csfv.psfvcb);
#endif
    if (!SUCCEEDED(hr))
        goto Fail;

    hr = IShellView_QueryInterface(This->DesktopView,
                                   &IID_IShellView2,
                                   (PVOID*)&This->DesktopView2);
    if (!SUCCEEDED(hr))
        This->DesktopView2 = NULL;

    if (lpCreateStruct == NULL)
    {
        if (!GetClientRect(This->hWnd,
                           &rcClient))
        {
            goto Fail;
        }
    }
    else
    {
        rcClient.left = 0;
        rcClient.top = 0;
        rcClient.right = lpCreateStruct->cx;
        rcClient.bottom = lpCreateStruct->cy;
    }

    fs.ViewMode = FVM_ICON;
    fs.fFlags = FWF_DESKTOP | FWF_NOCLIENTEDGE  | FWF_NOSCROLL | FWF_TRANSPARENT;
    if (This->DesktopView2 != NULL)
    {
        SV2CVW2_PARAMS params;

        params.cbSize = sizeof(params);
        params.psvPrev = NULL;
        params.pfs = &fs;
        params.psbOwner = ShellBrowser;
        params.prcView = &rcClient;
        params.pvid = &VID_LargeIcons;
        hr = IShellView2_CreateViewWindow2(This->DesktopView2,
                                           &params);

        if (FAILED(hr))
            goto DefCreateViewWindow;
        This->hWndShellView = params.hwndView;
    }
    else
    {
DefCreateViewWindow:
        hr = IShellView_CreateViewWindow(This->DesktopView,
                                         NULL,
                                         &fs,
                                         ShellBrowser,
                                         &rcClient,
                                         &This->hWndShellView);
    }
    if (!SUCCEEDED(hr))
        goto Fail;

    IShellView_EnableModeless(This->DesktopView,
                              TRUE);

    hr = IShellView_UIActivate(This->DesktopView,
                               SVUIA_ACTIVATE_FOCUS);
    if (!SUCCEEDED(hr))
    {
        IShellView_DestroyViewWindow(This->DesktopView);

Fail:
        /* NOTE: the other references will be released in the
                 WM_NCDESTROY handler! */
        IDesktopShellBrowserImpl_Release(ShellBrowser);
        return NULL;
    }

    /* Now get the real window handle to the list view control!
       We need to assume it is the child window of the default
       shell view window. There doesn't seem to be another way
       to get to the list view window handle... */
    This->hWndDesktopListView = FindWindowEx(This->hWndShellView,
                                             NULL,
                                             WC_LISTVIEW,
                                             NULL);

    if (This->hWndDesktopListView != NULL)
    {
        /* Register drag&drop */
        hr = IShellView_QueryInterface(This->DesktopView,
                                       &IID_IDropTarget,
                                       (PVOID*)&This->DropTarget);
        if (SUCCEEDED(hr))
        {
            hr = RegisterDragDrop(This->hWndDesktopListView,
                                  This->DropTarget);
            if (!SUCCEEDED(hr))
            {
                IDropTarget_Release(This->DropTarget);
                This->DropTarget = NULL;
            }
        }
    }

    /* Register a notification that is triggered when a new drive is added and the shell
       should open that drive in a new window, such as USB sticks. */
    cne.pidl = NULL;
    cne.fRecursive = TRUE;
    This->ChangeNotificationId = SHChangeNotifyRegister(This->hWnd,
                                                        SHCNRF_ShellLevel | SHCNRF_NewDelivery,
                                                        SHCNE_DRIVEADDGUI,
                                                        WM_SHELL_ADDDRIVENOTIFY,
                                                        1,
                                                        &cne);

    /* Create the tray window */
    This->Tray = CreateTrayWindow(This->hWnd);
    return This;
}

static HRESULT STDMETHODCALLTYPE
IDesktopShellBrowserImpl_GetWindow(IN OUT IShellBrowser *iface,
                                   OUT HWND *phwnd)
{
    IDesktopShellBrowserImpl *This = impl_from_IShellBrowser(iface);

    if (This->hWnd != NULL)
    {
        *phwnd = This->hWnd;
        return S_OK;
    }

    *phwnd = NULL;
    return E_UNEXPECTED;
}

static HRESULT STDMETHODCALLTYPE
IDesktopShellBrowserImpl_ContextSensitiveHelp(IN OUT IShellBrowser *iface,
                                              IN BOOL fEnterMode)
{
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE
IDesktopShellBrowserImpl_InsertMenusSB(IN OUT IShellBrowser *iface,
                                       IN HMENU hmenuShared,
                                       OUT LPOLEMENUGROUPWIDTHS lpMenuWidths)
{
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE
IDesktopShellBrowserImpl_SetMenuSB(IN OUT IShellBrowser *iface,
                                   IN HMENU hmenuShared,
                                   IN HOLEMENU holemenuRes,
                                   IN HWND hwndActiveObject)
{
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE
IDesktopShellBrowserImpl_RemoveMenusSB(IN OUT IShellBrowser *iface,
                                       IN HMENU hmenuShared)
{
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE
IDesktopShellBrowserImpl_SetStatusTextSB(IN OUT IShellBrowser *iface,
                                         IN LPCOLESTR lpszStatusText)
{
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE
IDesktopShellBrowserImpl_EnableModelessSB(IN OUT IShellBrowser *iface,
                                          IN BOOL fEnable)
{
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE
IDesktopShellBrowserImpl_TranslateAcceleratorSB(IN OUT IShellBrowser *iface,
                                                IN LPMSG lpmsg,
                                                IN WORD wID)
{
    return S_FALSE;
}

static HRESULT STDMETHODCALLTYPE
IDesktopShellBrowserImpl_BrowseObject(IN OUT IShellBrowser *iface,
                                      IN LPCITEMIDLIST pidl,
                                      IN UINT wFlags)
{
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE
IDesktopShellBrowserImpl_GetViewStateStream(IN OUT IShellBrowser *iface,
                                            IN DWORD grfMode,
                                            OUT IStream **ppStrm)
{
    IDesktopShellBrowserImpl *This = impl_from_IShellBrowser(iface);
    static WCHAR szItemPos[] = L"ItemPos";
    HRESULT hRet;
    IPropertyBag *PropBag = NULL;
    WCHAR szPropName[64];

    /* Determine the name of the property that contains the stream of
       icon positions */
    wcscpy(szPropName,
           szItemPos);
    hRet = SHGetPerScreenResName(szPropName + wcslen(szPropName),
                                 (sizeof(szPropName) / sizeof(szPropName[0])) - wcslen(szPropName),
                                 0);
    if (SUCCEEDED(hRet))
    {
        /* Locate the property bag for the desktop */
        hRet = SHGetViewStatePropertyBag(This->pidlDesktop,
                                         L"Desktop",
                                         SHGVSPB_FOLDERNODEFAULTS | SHGVSPB_ROAM,
                                         &IID_IPropertyBag,
                                         (PVOID*)&PropBag);

        if (SUCCEEDED(hRet))
        {
            /* Create a stream for the ItemPos property */
            hRet = SHPropertyBag_ReadStream(PropBag,
                                            szPropName,
                                            ppStrm);

            IPropertyBag_Release(PropBag);

            if (SUCCEEDED(hRet))
            {
                /* FIXME: For some reason the shell doesn't position the icons... */
            }
        }
    }

    return hRet;
}

static HWND
DesktopGetWindowControl(IN IDesktopShellBrowserImpl *This,
                        IN UINT id)
{
    switch (id)
    {
        case FCW_TOOLBAR:
        case FCW_STATUS:
        case FCW_TREE:
        case FCW_PROGRESS:
            return NULL;

        default:
            return NULL;
    }

}

static HRESULT STDMETHODCALLTYPE
IDesktopShellBrowserImpl_GetControlWindow(IN OUT IShellBrowser *iface,
                                          IN UINT id,
                                          OUT HWND *lphwnd)
{
    IDesktopShellBrowserImpl *This = impl_from_IShellBrowser(iface);
    HWND hWnd;

    hWnd = DesktopGetWindowControl(This,
                                   id);
    if (hWnd != NULL)
    {
        *lphwnd = hWnd;
        return S_OK;
    }

    *lphwnd = NULL;
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE
IDesktopShellBrowserImpl_SendControlMsg(IN OUT IShellBrowser *iface,
                                        IN UINT id,
                                        IN UINT uMsg,
                                        IN WPARAM wParam,
                                        IN LPARAM lParam,
                                        OUT LRESULT *pret)
{
    IDesktopShellBrowserImpl *This = impl_from_IShellBrowser(iface);
    HWND hWnd;

    if (pret == NULL)
        return E_POINTER;

    hWnd = DesktopGetWindowControl(This,
                                   id);
    if (hWnd != NULL)
    {
        *pret = SendMessage(hWnd,
                            uMsg,
                            wParam,
                            lParam);
        return S_OK;
    }

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE
IDesktopShellBrowserImpl_QueryActiveShellView(IN OUT IShellBrowser *iface,
                                              OUT IShellView **ppshv)
{
    IShellView *ActiveView;
    IDesktopShellBrowserImpl *This = impl_from_IShellBrowser(iface);

    ActiveView = This->DesktopView;
    IDesktopShellBrowserImpl_AddRef(iface);
    *ppshv = ActiveView;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
IDesktopShellBrowserImpl_OnViewWindowActive(IN OUT IShellBrowser *iface,
                                            IN OUT IShellView *ppshv)
{
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE
IDesktopShellBrowserImpl_SetToolbarItems(IN OUT IShellBrowser *iface,
                                         IN LPTBBUTTON lpButtons,
                                         IN UINT nButtons,
                                         IN UINT uFlags)
{
    return E_NOTIMPL;
}

static const IShellBrowserVtbl IDesktopShellBrowserImpl_Vtbl =
{
    /* IUnknown */
    IDesktopShellBrowserImpl_QueryInterface,
    IDesktopShellBrowserImpl_AddRef,
    IDesktopShellBrowserImpl_Release,
    /* IOleWindow */
    IDesktopShellBrowserImpl_GetWindow,
    IDesktopShellBrowserImpl_ContextSensitiveHelp,
    /* IShellBrowser */
    IDesktopShellBrowserImpl_InsertMenusSB,
    IDesktopShellBrowserImpl_SetMenuSB,
    IDesktopShellBrowserImpl_RemoveMenusSB,
    IDesktopShellBrowserImpl_SetStatusTextSB,
    IDesktopShellBrowserImpl_EnableModelessSB,
    IDesktopShellBrowserImpl_TranslateAcceleratorSB,
    IDesktopShellBrowserImpl_BrowseObject,
    IDesktopShellBrowserImpl_GetViewStateStream,
    IDesktopShellBrowserImpl_GetControlWindow,
    IDesktopShellBrowserImpl_SendControlMsg,
    IDesktopShellBrowserImpl_QueryActiveShellView,
    IDesktopShellBrowserImpl_OnViewWindowActive,
    IDesktopShellBrowserImpl_SetToolbarItems
};

/*
 * ICommDlgBrowser
 */

static HRESULT STDMETHODCALLTYPE
IDesktopShellBrowserImpl_ICommDlgBrowser_QueryInterface(IN OUT ICommDlgBrowser *iface,
                                                        IN REFIID riid,
                                                        OUT LPVOID *ppvObj)
{
    IDesktopShellBrowserImpl *This = impl_from_ICommDlgBrowser(iface);
    IShellBrowser *ShellBrowser = IShellBrowser_from_impl(This);

    return IDesktopShellBrowserImpl_QueryInterface(ShellBrowser,
                                                   riid,
                                                   ppvObj);
}

static ULONG STDMETHODCALLTYPE
IDesktopShellBrowserImpl_ICommDlgBrowser_Release(IN OUT ICommDlgBrowser *iface)
{
    IDesktopShellBrowserImpl *This = impl_from_ICommDlgBrowser(iface);
    IShellBrowser *ShellBrowser = IShellBrowser_from_impl(This);

    return IDesktopShellBrowserImpl_Release(ShellBrowser);
}

static ULONG STDMETHODCALLTYPE
IDesktopShellBrowserImpl_ICommDlgBrowser_AddRef(IN OUT ICommDlgBrowser *iface)
{
    IDesktopShellBrowserImpl *This = impl_from_ICommDlgBrowser(iface);
    IShellBrowser *ShellBrowser = IShellBrowser_from_impl(This);

    return IDesktopShellBrowserImpl_AddRef(ShellBrowser);
}

static HRESULT STDMETHODCALLTYPE
IDesktopShellBrowserImpl_ICommDlgBrowser_OnDefaultCommand(IN OUT ICommDlgBrowser *iface,
                                                          IN OUT IShellView *ppshv)
{
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE
IDesktopShellBrowserImpl_ICommDlgBrowser_OnStateChange(IN OUT ICommDlgBrowser *iface,
                                                       IN OUT IShellView *ppshv,
                                                       IN ULONG uChange)
{
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
IDesktopShellBrowserImpl_ICommDlgBrowser_IncludeObject(IN OUT ICommDlgBrowser *iface,
                                                       IN OUT IShellView *ppshv,
                                                       IN LPCITEMIDLIST pidl)
{
    return S_OK;
}

static const ICommDlgBrowserVtbl IDesktopShellBrowserImpl_ICommDlgBrowser_Vtbl =
{
    /* IUnknown */
    IDesktopShellBrowserImpl_ICommDlgBrowser_QueryInterface,
    IDesktopShellBrowserImpl_ICommDlgBrowser_AddRef,
    IDesktopShellBrowserImpl_ICommDlgBrowser_Release,
    /* ICommDlgBrowser */
    IDesktopShellBrowserImpl_ICommDlgBrowser_OnDefaultCommand,
    IDesktopShellBrowserImpl_ICommDlgBrowser_OnStateChange,
    IDesktopShellBrowserImpl_ICommDlgBrowser_IncludeObject
};

/*
 * IServiceProvider
 */

static HRESULT STDMETHODCALLTYPE
IDesktopShellBrowserImpl_IServiceProvider_QueryInterface(IN OUT IServiceProvider *iface,
                                                         IN REFIID riid,
                                                         OUT LPVOID *ppvObj)
{
    IDesktopShellBrowserImpl *This = impl_from_IServiceProvider(iface);
    IShellBrowser *ShellBrowser = IShellBrowser_from_impl(This);

    return IDesktopShellBrowserImpl_QueryInterface(ShellBrowser,
                                                   riid,
                                                   ppvObj);
}

static ULONG STDMETHODCALLTYPE
IDesktopShellBrowserImpl_IServiceProvider_Release(IN OUT IServiceProvider *iface)
{
    IDesktopShellBrowserImpl *This = impl_from_IServiceProvider(iface);
    IShellBrowser *ShellBrowser = IShellBrowser_from_impl(This);

    return IDesktopShellBrowserImpl_Release(ShellBrowser);
}

static ULONG STDMETHODCALLTYPE
IDesktopShellBrowserImpl_IServiceProvider_AddRef(IN OUT IServiceProvider *iface)
{
    IDesktopShellBrowserImpl *This = impl_from_IServiceProvider(iface);
    IShellBrowser *ShellBrowser = IShellBrowser_from_impl(This);

    return IDesktopShellBrowserImpl_AddRef(ShellBrowser);
}

static HRESULT STDMETHODCALLTYPE
IDesktopShellBrowserImpl_IServiceProvider_QueryService(IN OUT IServiceProvider *iface,
                                                       IN REFGUID guidService,
                                                       IN REFIID riid,
                                                       OUT PVOID *ppv)
{
    /* FIXME - handle guidService */
    return IDesktopShellBrowserImpl_IServiceProvider_QueryInterface(iface,
                                                                    riid,
                                                                    ppv);
}

static const IServiceProviderVtbl IDesktopShellBrowserImpl_IServiceProvider_Vtbl =
{
    /* IUnknown */
    IDesktopShellBrowserImpl_IServiceProvider_QueryInterface,
    IDesktopShellBrowserImpl_IServiceProvider_AddRef,
    IDesktopShellBrowserImpl_IServiceProvider_Release,
    /* IServiceProvider */
    IDesktopShellBrowserImpl_IServiceProvider_QueryService
};

/*
 * IShellFolderViewCB
 */

static HRESULT STDMETHODCALLTYPE
IDesktopShellBrowserImpl_IShellFolderViewCB_QueryInterface(IN OUT IShellFolderViewCB *iface,
                                                           IN REFIID riid,
                                                           OUT LPVOID *ppvObj)
{
    IDesktopShellBrowserImpl *This = impl_from_IShellFolderViewCB(iface);
    IShellBrowser *ShellBrowser = IShellBrowser_from_impl(This);

    return IDesktopShellBrowserImpl_QueryInterface(ShellBrowser,
                                                   riid,
                                                   ppvObj);
}

static ULONG STDMETHODCALLTYPE
IDesktopShellBrowserImpl_IShellFolderViewCB_Release(IN OUT IShellFolderViewCB *iface)
{
    IDesktopShellBrowserImpl *This = impl_from_IShellFolderViewCB(iface);
    IShellBrowser *ShellBrowser = IShellBrowser_from_impl(This);

    return IDesktopShellBrowserImpl_Release(ShellBrowser);
}

static ULONG STDMETHODCALLTYPE
IDesktopShellBrowserImpl_IShellFolderViewCB_AddRef(IN OUT IShellFolderViewCB *iface)
{
    IDesktopShellBrowserImpl *This = impl_from_IShellFolderViewCB(iface);
    IShellBrowser *ShellBrowser = IShellBrowser_from_impl(This);

    return IDesktopShellBrowserImpl_AddRef(ShellBrowser);
}

static HRESULT STDMETHODCALLTYPE
IDesktopShellBrowserImpl_IShellFolderViewCB_MessageSFVCB(IN OUT IShellFolderViewCB *iface,
                                                         IN UINT uMsg,
                                                         IN WPARAM wParam,
                                                         IN LPARAM lParam)
{
    IDesktopShellBrowserImpl *This = impl_from_IShellFolderViewCB(iface);
    HRESULT hr = S_OK;
    switch (uMsg)
    {
        case SFVM_GETNOTIFY:
            hr = IDesktopShellBrowserImpl_GetNotify(This,
                                                    (LPITEMIDLIST *)wParam,
                                                    (LONG *)lParam);
            break;

        case SFVM_FSNOTIFY:
            hr = S_OK;
            break;

        default:
            hr = E_NOTIMPL;
            break;
    }

    return hr;
}

static const IShellFolderViewCBVtbl IDesktopShellBrowserImpl_IShellFolderViewCB_Vtbl =
{
    /* IUnknown */
    IDesktopShellBrowserImpl_IShellFolderViewCB_QueryInterface,
    IDesktopShellBrowserImpl_IShellFolderViewCB_AddRef,
    IDesktopShellBrowserImpl_IShellFolderViewCB_Release,
    /* IShellFolderViewCB */
    IDesktopShellBrowserImpl_IShellFolderViewCB_MessageSFVCB
};

/*****************************************************************************/

static const TCHAR szProgmanClassName[] = TEXT("Progman");
static const TCHAR szProgmanWindowName[] = TEXT("Program Manager");

static VOID
PositionIcons(IN OUT IDesktopShellBrowserImpl *This,
              IN const RECT *prcDesktopRect)
{
    /* FIXME - Move all icons and align them on the destop. Before moving, we should
               check if we're moving an icon above an existing one. If so, we should
               let the list view control arrange the icons */
    return;
}

static VOID
UpdateWorkArea(IN OUT IDesktopShellBrowserImpl *This,
               IN const RECT *prcDesktopRect)
{
    RECT rcDesktopRect;
    LONG Style;

    Style = GetWindowLong(This->hWndDesktopListView,
                          GWL_STYLE);

    if (!(Style & LVS_AUTOARRANGE ) &&
        GetWindowRect(This->hWnd,
                      &rcDesktopRect))
    {
        /* Only resize the desktop if it was resized to something other
           than the virtual screen size/position! */
        if (!EqualRect(prcDesktopRect,
                       &rcDesktopRect))
        {
            SetWindowPos(This->hWnd,
                         NULL,
                         prcDesktopRect->left,
                         prcDesktopRect->top,
                         prcDesktopRect->right - prcDesktopRect->left,
                         prcDesktopRect->bottom - prcDesktopRect->top,
                         SWP_NOZORDER | SWP_NOACTIVATE);

            /* Let's try to rearrange the icons on the desktop. This is
               especially neccessary when switching screen resolutions... */
            PositionIcons(This,
                          prcDesktopRect);
        }
    }
}

static LRESULT CALLBACK
ProgmanWindowProc(IN HWND hwnd,
                  IN UINT uMsg,
                  IN WPARAM wParam,
                  IN LPARAM lParam)
{
    IDesktopShellBrowserImpl *This = NULL;
    LRESULT Ret = FALSE;

    if (uMsg != WM_NCCREATE)
    {
        This = (IDesktopShellBrowserImpl*)GetWindowLongPtr(hwnd,
                                                           0);
    }

    if (This != NULL || uMsg == WM_NCCREATE)
    {
        switch (uMsg)
        {
            case WM_ERASEBKGND:
                PaintDesktop((HDC)wParam);
                break;

            case WM_GETISHELLBROWSER:
                Ret = (LRESULT)IShellBrowser_from_impl(This);
                break;

            case WM_SHELL_ADDDRIVENOTIFY:
            {
                HANDLE hLock;
                LPITEMIDLIST *ppidl;
                LONG lEventId;

                hLock = SHChangeNotification_Lock((HANDLE)wParam,
                                                  (DWORD)lParam,
                                                  &ppidl,
                                                  &lEventId);
                if (hLock != NULL)
                {
                    /* FIXME */
                    SHChangeNotification_Unlock(hLock);
                }
                break;
            }

            case WM_SIZE:
                if (wParam == SIZE_MINIMIZED)
                {
                    /* Hey, we're the desktop!!! */
                    ShowWindow(hwnd,
                               SW_RESTORE);
                }
                else
                {
                    RECT rcDesktop;

                    rcDesktop.left = GetSystemMetrics(SM_XVIRTUALSCREEN);
                    rcDesktop.top = GetSystemMetrics(SM_YVIRTUALSCREEN);
                    rcDesktop.right = GetSystemMetrics(SM_CXVIRTUALSCREEN);
                    rcDesktop.bottom = GetSystemMetrics(SM_CYVIRTUALSCREEN);

                    UpdateWorkArea(This,
                                   &rcDesktop);
                }
                break;

            case WM_SYSCOLORCHANGE:
            {
                InvalidateRect(This->hWnd,
                               NULL,
                               TRUE);

                if (This->hWndShellView != NULL)
                {
                    /* Forward the message */
                    SendMessage(This->hWndShellView,
                                WM_SYSCOLORCHANGE,
                                wParam,
                                lParam);
                }
                break;
            }

            case WM_NCCREATE:
            {
                LPCREATESTRUCT CreateStruct = (LPCREATESTRUCT)lParam;
                This = IDesktopShellBrowserImpl_Construct(hwnd,
                                                          CreateStruct);
                if (This == NULL)
                    break;

                SetWindowLongPtr(hwnd,
                                 0,
                                 (LONG_PTR)This);

                if (This->hWndShellView != NULL)
                    SetShellWindowEx(This->hWnd,
                                     This->hWndDesktopListView);
                else
                    SetShellWindow(This->hWnd);

                Ret = TRUE;
                break;
            }

            case WM_DESTROY:
            {
                IDesktopShellBrowserImpl_Destroy(This);
                break;
            }

            case WM_NCDESTROY:
            {
                IDesktopShellBrowserImpl_Release(IShellBrowser_from_impl(This));

                PostQuitMessage(0);
                break;
            }

            default:
                Ret = DefWindowProc(hwnd, uMsg, wParam, lParam);
                break;
        }
    }

    return Ret;
}

static BOOL
RegisterProgmanWindowClass(VOID)
{
    WNDCLASS wcProgman;

    wcProgman.style = CS_DBLCLKS;
    wcProgman.lpfnWndProc = ProgmanWindowProc;
    wcProgman.cbClsExtra = 0;
    wcProgman.cbWndExtra = sizeof(IDesktopShellBrowserImpl *);
    wcProgman.hInstance = hExplorerInstance;
    wcProgman.hIcon = NULL;
    wcProgman.hCursor = LoadCursor(NULL,
                                   IDC_ARROW);
    wcProgman.hbrBackground = (HBRUSH)(COLOR_BACKGROUND + 1);
    wcProgman.lpszMenuName = NULL;
    wcProgman.lpszClassName = szProgmanClassName;

    return RegisterClass(&wcProgman) != 0;
}

static VOID
UnregisterProgmanWindowClass(VOID)
{
    UnregisterClass(szProgmanClassName,
                    hExplorerInstance);
}

typedef struct _DESKCREATEINFO
{
    HANDLE hEvent;
    HWND hWndDesktop;
    IDesktopShellBrowserImpl *pDesktop;
} DESKCREATEINFO, *PDESKCREATEINFO;

static DWORD CALLBACK
DesktopThreadProc(IN OUT LPVOID lpParameter)
{
    volatile DESKCREATEINFO *DeskCreateInfo = (volatile DESKCREATEINFO *)lpParameter;
    HWND hwndDesktop;
    IDesktopShellBrowserImpl *pDesktop;
    IShellDesktopTray *pSdt;
    MSG Msg;
    BOOL Ret;

    OleInitialize(NULL);
    hwndDesktop = CreateWindowEx(WS_EX_TOOLWINDOW,
                                 szProgmanClassName,
                                 szProgmanWindowName,
                                 WS_POPUP | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
                                 0,
                                 0,
                                 GetSystemMetrics(SM_CXVIRTUALSCREEN),
                                 GetSystemMetrics(SM_CYVIRTUALSCREEN),
                                 NULL,
                                 NULL,
                                 hExplorerInstance,
                                 NULL);

    DeskCreateInfo->hWndDesktop = hWndDesktop;
    if (hwndDesktop == NULL)
        return 1;

    pDesktop = (IDesktopShellBrowserImpl*)GetWindowLongPtr(hwndDesktop,
                                                           0);
    (void)InterlockedExchangePointer(&DeskCreateInfo->pDesktop,
                                     pDesktop);

    if (!SetEvent(DeskCreateInfo->hEvent))
    {
        /* Failed to notify that we initialized successfully, kill ourselves
           to make the main thread wake up! */
        return 1;
    }

    while (1)
    {
        Ret = (GetMessage(&Msg,
                          NULL,
                          0,
                          0) != 0);

        if (Ret != -1)
        {
            if (!Ret)
                break;

            TranslateMessage(&Msg);
            DispatchMessage(&Msg);
        }
    }

    OleUninitialize();

    /* FIXME: Properly rundown the main thread! */
    ExitProcess(0);

    return 0;
}

HANDLE
DesktopCreateWindow(IN OUT ITrayWindow *Tray)
{
    HANDLE hThread;
    HANDLE hEvent;
    DWORD DesktopThreadId;
    IDesktopShellBrowserImpl *pDesktop = NULL;
    HANDLE Handles[2];
    DWORD WaitResult;
    HWND hWndDesktop = NULL;
    IShellDesktopTray *pSdt;
    HRESULT hRet;

    if (!RegisterProgmanWindowClass())
        return NULL;

    if (!RegisterTrayWindowClass())
    {
        UnregisterProgmanWindowClass();
        return NULL;
    }

    if (!RegisterTaskSwitchWndClass())
    {
        UnregisterProgmanWindowClass();
        UnregisterTrayWindowClass();
        return NULL;
    }

    hEvent = CreateEvent(NULL,
                         FALSE,
                         FALSE,
                         NULL);
    if (hEvent != NULL)
    {
        volatile DESKCREATEINFO DeskCreateInfo;

        DeskCreateInfo.hEvent = hEvent;
        DeskCreateInfo.pDesktop = NULL;

        hThread = CreateThread(NULL,
                               0,
                               DesktopThreadProc,
                               (PVOID)&DeskCreateInfo,
                               0,
                               &DesktopThreadId);
        if (hThread != NULL)
        {
            Handles[0] = hThread;
            Handles[1] = hEvent;

            WaitResult = WaitForMultipleObjects(sizeof(Handles) / sizeof(Handles[0]),
                                                Handles,
                                                FALSE,
                                                INFINITE);
            if (WaitResult != WAIT_FAILED && WaitResult != WAIT_OBJECT_0)
            {
                pDesktop = DeskCreateInfo.pDesktop;
                hWndDesktop = DeskCreateInfo.hWndDesktop;
            }

            CloseHandle(hThread);
        }

        CloseHandle(hEvent);
    }

    if (pDesktop != NULL)
    {
        hRet = ITrayWindow_QueryInterface(Tray,
                                          &IID_IShellDesktopTray,
                                          (PVOID*)&pSdt);
        if (SUCCEEDED(hRet))
        {
            IShellDesktopTray_RegisterDesktopWindow(pSdt,
                                                    hWndDesktop);
            IShellDesktopTray_Release(pSdt);
        }
    }
    else
    {
        UnregisterProgmanWindowClass();
        UnregisterTrayWindowClass();
        UnregisterTaskSwitchWndClass();
    }

    return (HANDLE)pDesktop;
}

VOID
DesktopDestroyShellWindow(IN HANDLE hDesktop)
{
    //IDesktopShellBrowserImpl *pDesktop = (IDesktopShellBrowserImpl *)hDesktop;

    /* FIXME - Destroy window (don't use DestroyWindow() as
               the window belongs to another thread!) */

    UnregisterProgmanWindowClass();
    UnregisterTrayWindowClass();
}

#endif /* USE_API_SHCREATEDESKTOP */
