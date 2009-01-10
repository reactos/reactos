/*
 * Shell Desktop
 *
 * Copyright 2008 Thomas Bluemel
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

#include <precomp.h>

WINE_DEFAULT_DEBUG_CHANNEL(desktop);

BOOL WINAPI SetShellWindowEx(HWND, HWND);

#define SHDESK_TAG 0x4b534544

static const WCHAR szProgmanClassName[] = {'P','r','o','g','m','a','n'};
static const WCHAR szProgmanWindowName[] = {
    'P','r','o','g','r','a','m',' ','M','a','n','a','g','e','r'
};

static const IShellBrowserVtbl SHDESK_Vtbl;
static const ICommDlgBrowserVtbl SHDESK_ICommDlgBrowser_Vtbl;
static const IServiceProviderVtbl SHDESK_IServiceProvider_Vtbl;

typedef struct _SHDESK
{
    DWORD Tag;
    const IShellBrowserVtbl *lpVtbl;
    const ICommDlgBrowserVtbl *lpVtblCommDlgBrowser;
    const IServiceProviderVtbl *lpVtblServiceProvider;
    LONG Ref;
    HWND hWnd;
    HWND hWndShellView;
    HWND hWndDesktopListView;
    IShellDesktop *ShellDesk;
    IShellView *DesktopView;
    IShellBrowser *DefaultShellBrowser;
    LPITEMIDLIST pidlDesktopDirectory;
    LPITEMIDLIST pidlDesktop;
} SHDESK, *PSHDESK;

static IUnknown *
IUnknown_from_impl(SHDESK *This)
{
    return (IUnknown *)&This->lpVtbl;
}

static IShellBrowser *
IShellBrowser_from_impl(SHDESK *This)
{
    return (IShellBrowser *)&This->lpVtbl;
}

static IOleWindow *
IOleWindow_from_impl(SHDESK *This)
{
    return (IOleWindow *)&This->lpVtbl;
}

static ICommDlgBrowser *
ICommDlgBrowser_from_impl(SHDESK *This)
{
    return (ICommDlgBrowser *)&This->lpVtblCommDlgBrowser;
}

static IServiceProvider *
IServiceProvider_from_impl(SHDESK *This)
{
    return (IServiceProvider *)&This->lpVtblServiceProvider;
}

static SHDESK *
impl_from_IShellBrowser(IShellBrowser *iface)
{
    return (SHDESK *)((ULONG_PTR)iface - FIELD_OFFSET(SHDESK, lpVtbl));
}

static SHDESK *
impl_from_ICommDlgBrowser(ICommDlgBrowser *iface)
{
    return (SHDESK *)((ULONG_PTR)iface - FIELD_OFFSET(SHDESK, lpVtblCommDlgBrowser));
}

static SHDESK *
impl_from_IServiceProvider(IServiceProvider *iface)
{
    return (SHDESK *)((ULONG_PTR)iface - FIELD_OFFSET(SHDESK, lpVtblServiceProvider));
}

static void
SHDESK_Free(SHDESK *This)
{
    if (This->ShellDesk != NULL)
        IShellDesktop_Release(This->ShellDesk);

    if (This->DesktopView != NULL)
    {
        if (This->hWndShellView != NULL)
            IShellView_DestroyViewWindow(This->DesktopView);

        IShellView_Release(This->DesktopView);
        This->DesktopView = NULL;
        This->hWndShellView = NULL;
        This->hWndDesktopListView = NULL;
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

    ZeroMemory(This, sizeof(SHDESK));
    LocalFree((HLOCAL)This);
}

static ULONG STDMETHODCALLTYPE
SHDESK_Release(IShellBrowser *iface)
{
    SHDESK *This = impl_from_IShellBrowser(iface);
    ULONG Ret;

    Ret = InterlockedDecrement(&This->Ref);
    if (Ret == 0)
        SHDESK_Free(This);

    return Ret;
}

static ULONG STDMETHODCALLTYPE
SHDESK_AddRef(IShellBrowser *iface)
{
    SHDESK *This = impl_from_IShellBrowser(iface);

    return InterlockedIncrement(&This->Ref);
}

static HRESULT STDMETHODCALLTYPE
SHDESK_QueryInterface(IShellBrowser *iface, REFIID riid, LPVOID *ppvObj)
{
    SHDESK *This;

    if (ppvObj == NULL)
        return E_POINTER;

    This = impl_from_IShellBrowser(iface);

    if (IsEqualIID(riid, &IID_IUnknown))
    {
        *ppvObj = IUnknown_from_impl(This);
    }
    else if (This->DefaultShellBrowser != NULL)
    {
        return IShellBrowser_QueryInterface(This->DefaultShellBrowser, riid, ppvObj);
    }
    else if (IsEqualIID(riid, &IID_IOleWindow))
    {
        *ppvObj = IOleWindow_from_impl(This);
    }
    else if (IsEqualIID(riid, &IID_IShellBrowser))
    {
        *ppvObj = IShellBrowser_from_impl(This);
    }
    else if (IsEqualIID(riid, &IID_ICommDlgBrowser))
    {
        *ppvObj = ICommDlgBrowser_from_impl(This);
    }
    else if (IsEqualIID(riid, &IID_IServiceProvider))
    {
        *ppvObj = IServiceProvider_from_impl(This);
    }
    else
    {
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }

    SHDESK_AddRef(iface);
    return S_OK;
}

static PSHDESK
SHDESK_Create(HWND hWnd, LPCREATESTRUCT lpCreateStruct)
{
    IShellFolder *psfDesktopFolder;
    IShellDesktop *ShellDesk;
    CSFV csfv;
    SHDESK *This;
    HRESULT hRet;

    ShellDesk = (IShellDesktop *)lpCreateStruct->lpCreateParams;
    if (ShellDesk == NULL)
    {
        WARN("No IShellDesk interface provided!");
        return NULL;
    }

    This = (PSHDESK)LocalAlloc(LMEM_FIXED, sizeof(SHDESK));
    if (This == NULL)
        return NULL;

    ZeroMemory(This, sizeof(SHDESK));
    This->Tag = SHDESK_TAG;
    This->lpVtbl = &SHDESK_Vtbl;
    This->lpVtblCommDlgBrowser = &SHDESK_ICommDlgBrowser_Vtbl;
    This->lpVtblServiceProvider = &SHDESK_IServiceProvider_Vtbl;
    This->Ref = 1;
    This->hWnd = hWnd;
    This->ShellDesk = ShellDesk;
    IShellDesktop_AddRef(ShellDesk);

    This->pidlDesktopDirectory = SHCloneSpecialIDList(This->hWnd, CSIDL_DESKTOPDIRECTORY, FALSE);
    hRet = SHGetSpecialFolderLocation(This->hWnd, CSIDL_DESKTOP, &This->pidlDesktop);
    if (!SUCCEEDED(hRet))
        goto Fail;

    hRet = SHGetDesktopFolder(&psfDesktopFolder);
    if (!SUCCEEDED(hRet))
        goto Fail;

    ZeroMemory(&csfv, sizeof(csfv));
    csfv.cbSize = sizeof(csfv);
    csfv.pshf = psfDesktopFolder;
    csfv.psvOuter = NULL;

    hRet = SHCreateShellFolderViewEx(&csfv, &This->DesktopView);
    IShellFolder_Release(psfDesktopFolder);

    if (!SUCCEEDED(hRet))
    {
Fail:
        SHDESK_Release(IShellBrowser_from_impl(This));
        return NULL;
    }

    return This;
}

static BOOL
SHDESK_CreateDeskWnd(SHDESK *This)
{
    IShellBrowser *ShellBrowser;
    FOLDERSETTINGS fs;
    RECT rcClient;
    HWND hwndTray;
    HRESULT hRet;

    if (!GetClientRect(This->hWnd,
                       &rcClient))
    {
        return FALSE;
    }

    ShellBrowser = IShellBrowser_from_impl(This);

    fs.ViewMode = FVM_ICON;
    fs.fFlags = FWF_DESKTOP | FWF_NOCLIENTEDGE  | FWF_NOSCROLL | FWF_TRANSPARENT;
    hRet = IShellView_CreateViewWindow(This->DesktopView, NULL, &fs, ShellBrowser, &rcClient, &This->hWndShellView);
    if (!SUCCEEDED(hRet))
        return FALSE;

    if (SUCCEEDED (IShellDesktop_GetTrayWindow(This->ShellDesk,
                                               &hwndTray)))
    {
        SetShellWindowEx (This->hWnd,
                          hwndTray); // FIXME: Shouldn't this be the desktop listview?
    }
    return TRUE;
}

static HRESULT STDMETHODCALLTYPE
SHDESK_GetWindow(IShellBrowser *iface, HWND *phwnd)
{
    SHDESK *This = impl_from_IShellBrowser(iface);

    if (This->hWnd != NULL)
    {
        *phwnd = This->hWnd;
        return S_OK;
    }

    *phwnd = NULL;
    return E_UNEXPECTED;
}

static HRESULT STDMETHODCALLTYPE
SHDESK_ContextSensitiveHelp(IShellBrowser *iface, BOOL fEnterMode)
{
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE
SHDESK_InsertMenusSB(IShellBrowser *iface, HMENU hmenuShared, LPOLEMENUGROUPWIDTHS lpMenuWidths)
{
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE
SHDESK_SetMenuSB(IShellBrowser *iface, HMENU hmenuShared, HOLEMENU holemenuRes, HWND hwndActiveObject)
{
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE
SHDESK_RemoveMenusSB(IShellBrowser *iface, HMENU hmenuShared)
{
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE
SHDESK_SetStatusTextSB(IShellBrowser *iface, LPCOLESTR lpszStatusText)
{
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE
SHDESK_EnableModelessSB(IShellBrowser *iface, BOOL fEnable)
{
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE
SHDESK_TranslateAcceleratorSB(IShellBrowser *iface, LPMSG lpmsg, WORD wID)
{
    return S_FALSE;
}

static HRESULT STDMETHODCALLTYPE
SHDESK_BrowseObject(IShellBrowser *iface, LPCITEMIDLIST pidl, UINT wFlags)
{
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE
SHDESK_GetViewStateStream(IShellBrowser *iface, DWORD grfMode, IStream **ppStrm)
{
    return E_NOTIMPL;
}

static HWND
DesktopGetWindowControl(IN SHDESK *This,
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
SHDESK_GetControlWindow(IShellBrowser *iface, UINT id, HWND *lphwnd)
{
    SHDESK *This = impl_from_IShellBrowser(iface);
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
SHDESK_SendControlMsg(IShellBrowser *iface, UINT id, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *pret)
{
    SHDESK *This = impl_from_IShellBrowser(iface);
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
SHDESK_QueryActiveShellView(IShellBrowser *iface, IShellView **ppshv)
{
    IShellView *ActiveView;
    SHDESK *This = impl_from_IShellBrowser(iface);

    ActiveView = This->DesktopView;
    SHDESK_AddRef(iface);
    *ppshv = ActiveView;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
SHDESK_OnViewWindowActive(IShellBrowser *iface, IShellView *ppshv)
{
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE
SHDESK_SetToolbarItems(IShellBrowser *iface, LPTBBUTTON lpButtons, UINT nButtons, UINT uFlags)
{
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE
SHDESK_ICommDlgBrowser_QueryInterface(ICommDlgBrowser *iface, REFIID riid, LPVOID *ppvObj)
{
    SHDESK *This = impl_from_ICommDlgBrowser(iface);
    IShellBrowser *ShellBrowser = IShellBrowser_from_impl(This);

    return SHDESK_QueryInterface(ShellBrowser, riid, ppvObj);
}

static ULONG STDMETHODCALLTYPE
SHDESK_ICommDlgBrowser_Release(ICommDlgBrowser *iface)
{
    SHDESK *This = impl_from_ICommDlgBrowser(iface);
    IShellBrowser *ShellBrowser = IShellBrowser_from_impl(This);

    return SHDESK_Release(ShellBrowser);
}

static ULONG STDMETHODCALLTYPE
SHDESK_ICommDlgBrowser_AddRef(ICommDlgBrowser *iface)
{
    SHDESK *This = impl_from_ICommDlgBrowser(iface);
    IShellBrowser *ShellBrowser = IShellBrowser_from_impl(This);

    return SHDESK_AddRef(ShellBrowser);
}

static HRESULT STDMETHODCALLTYPE
SHDESK_ICommDlgBrowser_OnDefaultCommand(ICommDlgBrowser *iface, IShellView *ppshv)
{
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE
SHDESK_ICommDlgBrowser_OnStateChange(ICommDlgBrowser *iface, IShellView *ppshv, ULONG uChange)
{
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
SHDESK_ICommDlgBrowser_IncludeObject(ICommDlgBrowser *iface, IShellView *ppshv, LPCITEMIDLIST pidl)
{
    return S_OK;
}

static const ICommDlgBrowserVtbl SHDESK_ICommDlgBrowser_Vtbl =
{
    /* IUnknown */
    SHDESK_ICommDlgBrowser_QueryInterface,
    SHDESK_ICommDlgBrowser_AddRef,
    SHDESK_ICommDlgBrowser_Release,
    /* ICommDlgBrowser */
    SHDESK_ICommDlgBrowser_OnDefaultCommand,
    SHDESK_ICommDlgBrowser_OnStateChange,
    SHDESK_ICommDlgBrowser_IncludeObject
};

static HRESULT STDMETHODCALLTYPE
SHDESK_IServiceProvider_QueryInterface(IServiceProvider *iface, REFIID riid, LPVOID *ppvObj)
{
    SHDESK *This = impl_from_IServiceProvider(iface);
    IShellBrowser *ShellBrowser = IShellBrowser_from_impl(This);

    return SHDESK_QueryInterface(ShellBrowser, riid, ppvObj);
}

static ULONG STDMETHODCALLTYPE
SHDESK_IServiceProvider_Release(IServiceProvider *iface)
{
    SHDESK *This = impl_from_IServiceProvider(iface);
    IShellBrowser *ShellBrowser = IShellBrowser_from_impl(This);

    return SHDESK_Release(ShellBrowser);
}

static ULONG STDMETHODCALLTYPE
SHDESK_IServiceProvider_AddRef(IServiceProvider *iface)
{
    SHDESK *This = impl_from_IServiceProvider(iface);
    IShellBrowser *ShellBrowser = IShellBrowser_from_impl(This);

    return SHDESK_AddRef(ShellBrowser);
}

static HRESULT STDMETHODCALLTYPE
SHDESK_IServiceProvider_QueryService(IServiceProvider *iface, REFGUID guidService, REFIID riid, PVOID *ppv)
{
    /* FIXME - handle guidService */
    return SHDESK_IServiceProvider_QueryInterface(iface, riid, ppv);
}

static const IServiceProviderVtbl SHDESK_IServiceProvider_Vtbl =
{
    /* IUnknown */
    SHDESK_IServiceProvider_QueryInterface,
    SHDESK_IServiceProvider_AddRef,
    SHDESK_IServiceProvider_Release,
    /* IServiceProvider */
    SHDESK_IServiceProvider_QueryService
};

static BOOL
SHDESK_MessageLoop(SHDESK *This)
{
    MSG Msg;
    BOOL bRet;

    while ((bRet = GetMessage(&Msg, NULL, 0, 0)) != 0)
    {
        if (bRet != -1)
        {
            TranslateMessage(&Msg);
            DispatchMessage(&Msg);
        }
    }

    return TRUE;
}

static LRESULT CALLBACK
ProgmanWindowProc(IN HWND hwnd,
                  IN UINT uMsg,
                  IN WPARAM wParam,
                  IN LPARAM lParam)
{
    SHDESK *This = NULL;
    LRESULT Ret = FALSE;

    if (uMsg != WM_NCCREATE)
    {
        This = (SHDESK*)GetWindowLongPtrW(hwnd,
                                          0);
        if (This == NULL)
            goto DefMsgHandler;
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

                    /* FIXME: Update work area */
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
                    SendMessageW(This->hWndShellView,
                                 WM_SYSCOLORCHANGE,
                                 wParam,
                                 lParam);
                }
                break;
            }

            case WM_CREATE:
            {
                IShellDesktop_RegisterDesktopWindow(This->ShellDesk,
                                                    This->hWnd);

                if (!SHDESK_CreateDeskWnd(This))
                    WARN("Could not create the desktop view control!\n");
                break;
            }

            case WM_NCCREATE:
            {
                LPCREATESTRUCT CreateStruct = (LPCREATESTRUCT)lParam;
                This = SHDESK_Create(hwnd, CreateStruct);
                if (This == NULL)
                {
                    WARN("Failed to create desktop structure\n");
                    break;
                }

                SetWindowLongPtrW(hwnd,
                                  0,
                                  (LONG_PTR)This);
                Ret = TRUE;
                break;
            }

            case WM_NCDESTROY:
            {
                SHDESK_Free(This);
                break;
            }

            default:
DefMsgHandler:
                Ret = DefWindowProcW(hwnd, uMsg, wParam, lParam);
                break;
        }
    }

    return Ret;
}

static BOOL
RegisterProgmanWindowClass(VOID)
{
    WNDCLASSW wcProgman;

    wcProgman.style = CS_DBLCLKS;
    wcProgman.lpfnWndProc = ProgmanWindowProc;
    wcProgman.cbClsExtra = 0;
    wcProgman.cbWndExtra = sizeof(PSHDESK);
    wcProgman.hInstance = shell32_hInstance;
    wcProgman.hIcon = NULL;
    wcProgman.hCursor = LoadCursor(NULL,
                                   IDC_ARROW);
    wcProgman.hbrBackground = (HBRUSH)(COLOR_BACKGROUND + 1);
    wcProgman.lpszMenuName = NULL;
    wcProgman.lpszClassName = szProgmanClassName;

    return RegisterClassW(&wcProgman) != 0;
}


/*************************************************************************
 * SHCreateDesktop			[SHELL32.200]
 *
 */
HANDLE WINAPI SHCreateDesktop(IShellDesktop *ShellDesk)
{
    HWND hWndDesk;
    RECT rcDesk;

    if (ShellDesk == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return NULL;
    }

    if (RegisterProgmanWindowClass() == 0)
    {
        WARN("Failed to register the Progman window class!\n");
        return NULL;
    }

    rcDesk.left = GetSystemMetrics(SM_XVIRTUALSCREEN);
    rcDesk.top = GetSystemMetrics(SM_YVIRTUALSCREEN);
    rcDesk.right = rcDesk.left + GetSystemMetrics(SM_CXVIRTUALSCREEN);
    rcDesk.bottom = rcDesk.top + GetSystemMetrics(SM_CYVIRTUALSCREEN);

    if (IsRectEmpty(&rcDesk))
    {
        rcDesk.left = rcDesk.top = 0;
        rcDesk.right = GetSystemMetrics(SM_CXSCREEN);
        rcDesk.bottom = GetSystemMetrics(SM_CYSCREEN);
    }

    hWndDesk = CreateWindowExW(0, szProgmanClassName, szProgmanWindowName,
        WS_POPUP | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
        rcDesk.left, rcDesk.top, rcDesk.right, rcDesk.bottom,
        NULL, NULL, shell32_hInstance, (LPVOID)ShellDesk);
    if (hWndDesk != NULL)
        return (HANDLE)GetWindowLongPtr(hWndDesk, 0);

    return NULL;
}

/*************************************************************************
 * SHCreateDesktop			[SHELL32.201]
 *
 */
BOOL WINAPI SHDesktopMessageLoop(HANDLE hDesktop)
{
    PSHDESK Desk = (PSHDESK)hDesktop;

    if (Desk == NULL || Desk->Tag != SHDESK_TAG)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    return SHDESK_MessageLoop(Desk);
}

static const IShellBrowserVtbl SHDESK_Vtbl =
{
    /* IUnknown */
    SHDESK_QueryInterface,
    SHDESK_AddRef,
    SHDESK_Release,
    /* IOleWindow */
    SHDESK_GetWindow,
    SHDESK_ContextSensitiveHelp,
    /* IShellBrowser */
    SHDESK_InsertMenusSB,
    SHDESK_SetMenuSB,
    SHDESK_RemoveMenusSB,
    SHDESK_SetStatusTextSB,
    SHDESK_EnableModelessSB,
    SHDESK_TranslateAcceleratorSB,
    SHDESK_BrowseObject,
    SHDESK_GetViewStateStream,
    SHDESK_GetControlWindow,
    SHDESK_SendControlMsg,
    SHDESK_QueryActiveShellView,
    SHDESK_OnViewWindowActive,
    SHDESK_SetToolbarItems
};
