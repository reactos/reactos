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

#include "precomp.h"

WINE_DEFAULT_DEBUG_CHANNEL(desktop);

#define SHDESK_TAG 0x4b534544

static const WCHAR szProgmanClassName [] = L"Progman";
static const WCHAR szProgmanWindowName [] = L"Program Manager";

class CDesktopBrowser :
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IShellBrowser,
    public ICommDlgBrowser,
    public IServiceProvider
{
public:
    DWORD Tag;
    HACCEL m_hAccel;
private:
    HWND hWnd;
    HWND hWndShellView;
    HWND hWndDesktopListView;
    CComPtr<IShellDesktopTray>        ShellDesk;
    CComPtr<IShellView>                DesktopView;
    CComPtr<IShellBrowser> DefaultShellBrowser;
    LPITEMIDLIST pidlDesktopDirectory;
    LPITEMIDLIST pidlDesktop;

    LRESULT _NotifyTray(UINT uMsg, WPARAM wParam, LPARAM lParam);

public:
    CDesktopBrowser();
    ~CDesktopBrowser();
    HRESULT Initialize(HWND hWndx, IShellDesktopTray *ShellDeskx);
    HWND FindDesktopListView ();
    BOOL CreateDeskWnd();
    HWND DesktopGetWindowControl(IN UINT id);
    LRESULT OnCommand(UINT uMsg, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK ProgmanWindowProc(IN HWND hwnd, IN UINT uMsg, IN WPARAM wParam, IN LPARAM lParam);
    BOOL MessageLoop();

    // *** IOleWindow methods ***
    virtual HRESULT STDMETHODCALLTYPE GetWindow(HWND *lphwnd);
    virtual HRESULT STDMETHODCALLTYPE ContextSensitiveHelp(BOOL fEnterMode);

    // *** IShellBrowser methods ***
    virtual HRESULT STDMETHODCALLTYPE InsertMenusSB(HMENU hmenuShared, LPOLEMENUGROUPWIDTHS lpMenuWidths);
    virtual HRESULT STDMETHODCALLTYPE SetMenuSB(HMENU hmenuShared, HOLEMENU holemenuRes, HWND hwndActiveObject);
    virtual HRESULT STDMETHODCALLTYPE RemoveMenusSB(HMENU hmenuShared);
    virtual HRESULT STDMETHODCALLTYPE SetStatusTextSB(LPCOLESTR pszStatusText);
    virtual HRESULT STDMETHODCALLTYPE EnableModelessSB(BOOL fEnable);
    virtual HRESULT STDMETHODCALLTYPE TranslateAcceleratorSB(MSG *pmsg, WORD wID);
    virtual HRESULT STDMETHODCALLTYPE BrowseObject(LPCITEMIDLIST pidl, UINT wFlags);
    virtual HRESULT STDMETHODCALLTYPE GetViewStateStream(DWORD grfMode, IStream **ppStrm);
    virtual HRESULT STDMETHODCALLTYPE GetControlWindow(UINT id, HWND *lphwnd);
    virtual HRESULT STDMETHODCALLTYPE SendControlMsg(UINT id, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *pret);
    virtual HRESULT STDMETHODCALLTYPE QueryActiveShellView(struct IShellView **ppshv);
    virtual HRESULT STDMETHODCALLTYPE OnViewWindowActive(struct IShellView *ppshv);
    virtual HRESULT STDMETHODCALLTYPE SetToolbarItems(LPTBBUTTON lpButtons, UINT nButtons, UINT uFlags);

    // *** ICommDlgBrowser methods ***
    virtual HRESULT STDMETHODCALLTYPE OnDefaultCommand (struct IShellView *ppshv);
    virtual HRESULT STDMETHODCALLTYPE OnStateChange (struct IShellView *ppshv, ULONG uChange);
    virtual HRESULT STDMETHODCALLTYPE IncludeObject (struct IShellView *ppshv, LPCITEMIDLIST pidl);

    // *** IServiceProvider methods ***
    virtual HRESULT STDMETHODCALLTYPE QueryService(REFGUID guidService, REFIID riid, void **ppvObject);

BEGIN_COM_MAP(CDesktopBrowser)
    COM_INTERFACE_ENTRY_IID(IID_IOleWindow, IOleWindow)
    COM_INTERFACE_ENTRY_IID(IID_IShellBrowser, IShellBrowser)
    COM_INTERFACE_ENTRY_IID(IID_ICommDlgBrowser, ICommDlgBrowser)
    COM_INTERFACE_ENTRY_IID(IID_IServiceProvider, IServiceProvider)
END_COM_MAP()
};

CDesktopBrowser::CDesktopBrowser()
{
    Tag = SHDESK_TAG;
    hWnd = NULL;
    hWndShellView = NULL;
    hWndDesktopListView = NULL;
    DefaultShellBrowser = NULL;
    pidlDesktopDirectory = NULL;
    pidlDesktop = NULL;
}

CDesktopBrowser::~CDesktopBrowser()
{
    if (DesktopView.p != NULL)
    {
        if (hWndShellView != NULL)
            DesktopView->DestroyViewWindow();

        hWndShellView = NULL;
        hWndDesktopListView = NULL;
    }

    if (pidlDesktopDirectory != NULL)
    {
        ILFree(pidlDesktopDirectory);
        pidlDesktopDirectory = NULL;
    }

    if (pidlDesktop != NULL)
    {
        ILFree(pidlDesktop);
        pidlDesktop = NULL;
    }
}

HRESULT CDesktopBrowser::Initialize(HWND hWndx, IShellDesktopTray *ShellDeskx)
{
    CComPtr<IShellFolder>    psfDesktopFolder;
    CSFV                    csfv;
    HRESULT                    hRet;

    hWnd = hWndx;
    ShellDesk = ShellDeskx;
    ShellDesk->AddRef();

    pidlDesktopDirectory = SHCloneSpecialIDList(hWnd, CSIDL_DESKTOPDIRECTORY, FALSE);
    hRet = SHGetSpecialFolderLocation(hWnd, CSIDL_DESKTOP, &pidlDesktop);
    if (FAILED(hRet))
        return hRet;

    hRet = SHGetDesktopFolder(&psfDesktopFolder);
    if (FAILED(hRet))
        return hRet;

    ZeroMemory(&csfv, sizeof(csfv));
    csfv.cbSize = sizeof(csfv);
    csfv.pshf = psfDesktopFolder;
    csfv.psvOuter = NULL;

    hRet = SHCreateShellFolderViewEx(&csfv, &DesktopView);

    return hRet;
}

static CDesktopBrowser *SHDESK_Create(HWND hWnd, LPCREATESTRUCT lpCreateStruct)
{
    CComPtr<IShellDesktopTray>       ShellDesk;
    CComObject<CDesktopBrowser>        *pThis;
    HRESULT                    hRet;

    ShellDesk = (IShellDesktopTray *)lpCreateStruct->lpCreateParams;
    if (ShellDesk == NULL)
    {
        WARN("No IShellDesk interface provided!");
        return NULL;
    }

    pThis = new CComObject<CDesktopBrowser>;
    if (pThis == NULL)
        return NULL;
    pThis->AddRef();

    hRet = pThis->Initialize(hWnd, ShellDesk);
    if (FAILED(hRet))
    {
        pThis->Release();
        return NULL;
    }

    return pThis;
}

HWND CDesktopBrowser::FindDesktopListView ()
{
    return FindWindowExW(hWndShellView, NULL, WC_LISTVIEW, NULL);
}

BOOL CDesktopBrowser::CreateDeskWnd()
{
    FOLDERSETTINGS fs;
    RECT rcClient;
    HRESULT hRet;

    if (!GetClientRect(hWnd, &rcClient))
    {
        return FALSE;
    }

    fs.ViewMode = FVM_ICON;
    fs.fFlags = FWF_DESKTOP | FWF_NOCLIENTEDGE  | FWF_NOSCROLL | FWF_TRANSPARENT;
    hRet = DesktopView->CreateViewWindow(NULL, &fs, (IShellBrowser *)this, &rcClient, &hWndShellView);
    if (!SUCCEEDED(hRet))
        return FALSE;

    SetShellWindowEx(hWnd, FindDesktopListView());

    return TRUE;
}

HRESULT STDMETHODCALLTYPE CDesktopBrowser::GetWindow(HWND *phwnd)
{
    if (hWnd != NULL)
    {
        *phwnd = hWnd;
        return S_OK;
    }

    *phwnd = NULL;
    return E_UNEXPECTED;
}

HRESULT STDMETHODCALLTYPE CDesktopBrowser::ContextSensitiveHelp(BOOL fEnterMode)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CDesktopBrowser::InsertMenusSB(HMENU hmenuShared, LPOLEMENUGROUPWIDTHS lpMenuWidths)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CDesktopBrowser::SetMenuSB(HMENU hmenuShared, HOLEMENU holemenuRes, HWND hwndActiveObject)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CDesktopBrowser::RemoveMenusSB(HMENU hmenuShared)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CDesktopBrowser::SetStatusTextSB(LPCOLESTR lpszStatusText)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CDesktopBrowser::EnableModelessSB(BOOL fEnable)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CDesktopBrowser::TranslateAcceleratorSB(LPMSG lpmsg, WORD wID)
{
    if (!::TranslateAcceleratorW(hWnd, m_hAccel, lpmsg))
        return S_FALSE;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CDesktopBrowser::BrowseObject(LPCITEMIDLIST pidl, UINT wFlags)
{
    /*
     * We should use IShellWindows interface here in order to attempt to
     * find an open shell window that shows the requested pidl and activate it
     */

    return SHOpenNewFrame((LPITEMIDLIST)pidl, NULL, 0, 0);
}

HRESULT STDMETHODCALLTYPE CDesktopBrowser::GetViewStateStream(DWORD grfMode, IStream **ppStrm)
{
    return E_NOTIMPL;
}

HWND CDesktopBrowser::DesktopGetWindowControl(IN UINT id)
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

HRESULT STDMETHODCALLTYPE CDesktopBrowser::GetControlWindow(UINT id, HWND *lphwnd)
{
    HWND hWnd;

    hWnd = DesktopGetWindowControl(id);
    if (hWnd != NULL)
    {
        *lphwnd = hWnd;
        return S_OK;
    }

    *lphwnd = NULL;
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CDesktopBrowser::SendControlMsg(UINT id, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *pret)
{
    HWND                        hWnd;

    if (pret == NULL)
        return E_POINTER;

    hWnd = DesktopGetWindowControl(id);
    if (hWnd != NULL)
    {
        *pret = SendMessageW(hWnd,
                             uMsg,
                             wParam,
                             lParam);
        return S_OK;
    }

    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CDesktopBrowser::QueryActiveShellView(IShellView **ppshv)
{
    *ppshv = DesktopView;
    if (DesktopView != NULL)
        DesktopView->AddRef();

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CDesktopBrowser::OnViewWindowActive(IShellView *ppshv)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CDesktopBrowser::SetToolbarItems(LPTBBUTTON lpButtons, UINT nButtons, UINT uFlags)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CDesktopBrowser::OnDefaultCommand(IShellView *ppshv)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CDesktopBrowser::OnStateChange(IShellView *ppshv, ULONG uChange)
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CDesktopBrowser::IncludeObject(IShellView *ppshv, LPCITEMIDLIST pidl)
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CDesktopBrowser::QueryService(REFGUID guidService, REFIID riid, PVOID *ppv)
{
    /* FIXME - handle guidService */
    return QueryInterface(riid, ppv);
}

BOOL CDesktopBrowser::MessageLoop()
{
    MSG Msg;
    BOOL bRet;

    while ((bRet = GetMessageW(&Msg, NULL, 0, 0)) != 0)
    {
        if (bRet != -1)
        {
            if (DesktopView->TranslateAcceleratorW(&Msg) != S_OK)
            {
                TranslateMessage(&Msg);
                DispatchMessage(&Msg);
            }
        }
    }

    return TRUE;
}

LRESULT CDesktopBrowser::_NotifyTray(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    HWND hwndTray;
    HRESULT hres;

    hres = this->ShellDesk->GetTrayWindow(&hwndTray);

    if (SUCCEEDED(hres))
        PostMessageW(hwndTray, uMsg, wParam, lParam);

    return 0;
}

LRESULT CDesktopBrowser::OnCommand(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (LOWORD(wParam))
    {
        case FCIDM_DESKBROWSER_CLOSE:
            return _NotifyTray(TWM_DOEXITWINDOWS, 0, 0);
        case FCIDM_DESKBROWSER_FOCUS:
            if (GetKeyState(VK_SHIFT))
                return _NotifyTray(TWM_CYCLEFOCUS, 1, 0xFFFFFFFF);
            else
                return _NotifyTray(TWM_CYCLEFOCUS, 1, 1);
        case FCIDM_DESKBROWSER_SEARCH:
            SHFindFiles(NULL, NULL);
            break;
        case FCIDM_DESKBROWSER_REFRESH:
            break;
    }

    return 0;
}

LRESULT CALLBACK CDesktopBrowser::ProgmanWindowProc(IN HWND hwnd, IN UINT uMsg, IN WPARAM wParam, IN LPARAM lParam)
{
    CDesktopBrowser *pThis = NULL;
    LRESULT Ret = FALSE;

    if (uMsg != WM_NCCREATE)
    {
        pThis = reinterpret_cast<CDesktopBrowser *>(GetWindowLongPtrW(hwnd, 0));
        if (pThis == NULL)
            goto DefMsgHandler;
    }

    if (pThis != NULL || uMsg == WM_NCCREATE)
    {
        switch (uMsg)
        {
            case WM_ERASEBKGND:
                return (LRESULT)PaintDesktop((HDC)wParam);

            case WM_GETISHELLBROWSER:
                Ret = (LRESULT)((IShellBrowser *)pThis);
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
                    DBG_UNREFERENCED_LOCAL_VARIABLE(rcDesktop);
                }
                break;

            case WM_SYSCOLORCHANGE:
            case WM_SETTINGCHANGE:
            {
                if (uMsg == WM_SYSCOLORCHANGE || wParam == SPI_SETDESKWALLPAPER || wParam == 0)
                {
                    if (pThis->hWndShellView != NULL)
                    {
                        /* Forward the message */
                        SendMessageW(pThis->hWndShellView,
                                     uMsg,
                                     wParam,
                                     lParam);
                    }
                }
                break;
            }

            case WM_CREATE:
            {
                pThis->ShellDesk->RegisterDesktopWindow(pThis->hWnd);

                if (!pThis->CreateDeskWnd())
                    WARN("Could not create the desktop view control!\n");

                pThis->m_hAccel = LoadAcceleratorsW(shell32_hInstance, MAKEINTRESOURCEW(3));

                break;
            }

            case WM_NCCREATE:
            {
                LPCREATESTRUCT CreateStruct = (LPCREATESTRUCT)lParam;
                pThis = SHDESK_Create(hwnd, CreateStruct);
                if (pThis == NULL)
                {
                    WARN("Failed to create desktop structure\n");
                    break;
                }

                SetWindowLongPtrW(hwnd,
                                  0,
                                  (LONG_PTR)pThis);
                Ret = TRUE;
                break;
            }

            case WM_NCDESTROY:
            {
                pThis->Release();
                break;
            }

            case WM_EXPLORER_OPEN_NEW_WINDOW:
                TRACE("Proxy Desktop message 1035 received.\n");
                SHOnCWMCommandLine((HANDLE)lParam);
                break;

            case WM_COMMAND:
                return pThis->OnCommand(uMsg, wParam, lParam);

            case WM_SETFOCUS:
                SetFocus(pThis->hWndShellView);
                break;
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
    wcProgman.lpfnWndProc = CDesktopBrowser::ProgmanWindowProc;
    wcProgman.cbClsExtra = 0;
    wcProgman.cbWndExtra = sizeof(CDesktopBrowser *);
    wcProgman.hInstance = shell32_hInstance;
    wcProgman.hIcon = NULL;
    wcProgman.hCursor = LoadCursorW(NULL, IDC_ARROW);
    wcProgman.hbrBackground = (HBRUSH)(COLOR_BACKGROUND + 1);
    wcProgman.lpszMenuName = NULL;
    wcProgman.lpszClassName = szProgmanClassName;

    return RegisterClassW(&wcProgman) != 0;
}


/*************************************************************************
 * SHCreateDesktop            [SHELL32.200]
 *
 */
HANDLE WINAPI SHCreateDesktop(IShellDesktopTray *ShellDesk)
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

    hWndDesk = CreateWindowExW(WS_EX_TOOLWINDOW, szProgmanClassName, szProgmanWindowName,
        WS_POPUP | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
        rcDesk.left, rcDesk.top, rcDesk.right, rcDesk.bottom,
        NULL, NULL, shell32_hInstance, (LPVOID)ShellDesk);
    if (hWndDesk != NULL)
        return (HANDLE)GetWindowLongPtrW(hWndDesk, 0);

    return NULL;
}

/*************************************************************************
 * SHCreateDesktop            [SHELL32.201]
 *
 */
BOOL WINAPI SHDesktopMessageLoop(HANDLE hDesktop)
{
    CDesktopBrowser *Desk = static_cast<CDesktopBrowser *>(hDesktop);

    if (Desk == NULL || Desk->Tag != SHDESK_TAG)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    return Desk->MessageLoop();
}
