/*
 * SHDOCVW - Internet Explorer main frame window
 *
 * Copyright 2006 Mike McCormack (for CodeWeavers)
 * Copyright 2006 Jacek Caban (for CodeWeavers)
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

#define COBJMACROS

#include <stdarg.h>
#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "wingdi.h"
#include "winnls.h"
#include "ole2.h"
#include "exdisp.h"
#include "oleidl.h"

#include "shdocvw.h"
#include "mshtmcid.h"
#include "shellapi.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(shdocvw);

#define IDI_APPICON 1

static const WCHAR szIEWinFrame[] = { 'I','E','F','r','a','m','e',0 };

/* Windows uses "Microsoft Internet Explorer" */
static const WCHAR wszWineInternetExplorer[] =
        {'W','i','n','e',' ','I','n','t','e','r','n','e','t',' ','E','x','p','l','o','r','e','r',0};

static INT_PTR CALLBACK ie_dialog_open_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    static InternetExplorer* This;

    switch(msg)
    {
        case WM_INITDIALOG:
            This = (InternetExplorer*)lparam;
            EnableWindow(GetDlgItem(hwnd, IDOK), FALSE);
            return TRUE;

        case WM_COMMAND:
            switch(LOWORD(wparam))
            {
                case IDC_BROWSE_OPEN_URL:
                {
                    HWND hwndurl = GetDlgItem(hwnd, IDC_BROWSE_OPEN_URL);
                    int len = GetWindowTextLengthW(hwndurl);

                    EnableWindow(GetDlgItem(hwnd, IDOK), len ? TRUE : FALSE);
                    break;
                }
                case IDOK:
                {
                    HWND hwndurl = GetDlgItem(hwnd, IDC_BROWSE_OPEN_URL);
                    int len = GetWindowTextLengthW(hwndurl);

                    if(len)
                    {
                        VARIANT url;

                        V_VT(&url) = VT_BSTR;
                        V_BSTR(&url) = SysAllocStringLen(NULL, len);

                        GetWindowTextW(hwndurl, V_BSTR(&url), len);
                        IWebBrowser2_Navigate2(WEBBROWSER2(This), &url, NULL, NULL, NULL, NULL);

                        SysFreeString(V_BSTR(&url));
                    }
                }
                /* fall through */
                case IDCANCEL:
                    EndDialog(hwnd, wparam);
                    return TRUE;
            }
    }
    return FALSE;
}

static void ie_dialog_about(HWND hwnd)
{
    HICON icon = LoadImageW(GetModuleHandleW(0), MAKEINTRESOURCEW(IDI_APPICON), IMAGE_ICON, 48, 48, LR_SHARED);

    ShellAboutW(hwnd, wszWineInternetExplorer, NULL, icon);

    DestroyIcon(icon);
}

static LRESULT iewnd_OnCreate(HWND hwnd, LPCREATESTRUCTW lpcs)
{
    SetWindowLongPtrW(hwnd, 0, (LONG_PTR) lpcs->lpCreateParams);
    return 0;
}

static LRESULT iewnd_OnSize(InternetExplorer *This, INT width, INT height)
{
    if(This->doc_host.hwnd)
        SetWindowPos(This->doc_host.hwnd, NULL, 0, 0, width, height,
                     SWP_NOZORDER | SWP_NOACTIVATE);

    return 0;
}

static LRESULT iewnd_OnDestroy(InternetExplorer *This)
{
    TRACE("%p\n", This);

    This->frame_hwnd = NULL;
    PostQuitMessage(0); /* FIXME */

    return 0;
}

static LRESULT CALLBACK iewnd_OnCommand(InternetExplorer *This, HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch(LOWORD(wparam))
    {
        case ID_BROWSE_OPEN:
            DialogBoxParamW(shdocvw_hinstance, MAKEINTRESOURCEW(IDD_BROWSE_OPEN), hwnd, ie_dialog_open_proc, (LPARAM)This);
            break;

        case ID_BROWSE_PRINT:
            if(This->doc_host.document)
            {
                IOleCommandTarget* target;

                if(FAILED(IUnknown_QueryInterface(This->doc_host.document, &IID_IOleCommandTarget, (LPVOID*)&target)))
                    break;

                IOleCommandTarget_Exec(target, &CGID_MSHTML, IDM_PRINT, OLECMDEXECOPT_DODEFAULT, NULL, NULL);

                IOleCommandTarget_Release(target);
            }
            break;

        case ID_BROWSE_ABOUT:
            ie_dialog_about(hwnd);
            break;

        default:
            return DefWindowProcW(hwnd, msg, wparam, lparam);
    }
    return 0;
}

static LRESULT CALLBACK
ie_window_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    InternetExplorer *This = (InternetExplorer*) GetWindowLongPtrW(hwnd, 0);

    switch (msg)
    {
    case WM_CREATE:
        return iewnd_OnCreate(hwnd, (LPCREATESTRUCTW)lparam);
    case WM_DESTROY:
        return iewnd_OnDestroy(This);
    case WM_SIZE:
        return iewnd_OnSize(This, LOWORD(lparam), HIWORD(lparam));
    case WM_COMMAND:
        return iewnd_OnCommand(This, hwnd, msg, wparam, lparam);
    case WM_DOCHOSTTASK:
        return process_dochost_task(&This->doc_host, lparam);
    }
    return DefWindowProcW(hwnd, msg, wparam, lparam);
}

void register_iewindow_class(void)
{
    WNDCLASSEXW wc;

    memset(&wc, 0, sizeof wc);
    wc.cbSize = sizeof(wc);
    wc.style = 0;
    wc.lpfnWndProc = ie_window_proc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = sizeof(InternetExplorer*);
    wc.hInstance = shdocvw_hinstance;
    wc.hIcon = LoadIconW(GetModuleHandleW(0), MAKEINTRESOURCEW(IDI_APPICON));
    wc.hIconSm = LoadImageW(GetModuleHandleW(0), MAKEINTRESOURCEW(IDI_APPICON), IMAGE_ICON,
                            GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_SHARED);
    wc.hCursor = LoadCursorW(0, MAKEINTRESOURCEW(IDC_ARROW));
    wc.hbrBackground = 0;
    wc.lpszClassName = szIEWinFrame;
    wc.lpszMenuName = NULL;

    RegisterClassExW(&wc);
}

void unregister_iewindow_class(void)
{
    UnregisterClassW(szIEWinFrame, shdocvw_hinstance);
}

static void create_frame_hwnd(InternetExplorer *This)
{
    This->frame_hwnd = CreateWindowExW(
            WS_EX_WINDOWEDGE,
            szIEWinFrame, wszWineInternetExplorer,
            WS_CLIPCHILDREN | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME
                | WS_MINIMIZEBOX | WS_MAXIMIZEBOX,
            CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
            NULL, NULL /* FIXME */, shdocvw_hinstance, This);
}

static IWebBrowser2 *create_ie_window(LPCSTR cmdline)
{
    IWebBrowser2 *wb = NULL;

    InternetExplorer_Create(NULL, &IID_IWebBrowser2, (void**)&wb);
    if(!wb)
        return NULL;

    IWebBrowser2_put_Visible(wb, VARIANT_TRUE);
    IWebBrowser2_put_MenuBar(wb, VARIANT_TRUE);

    if(!*cmdline) {
        IWebBrowser2_GoHome(wb);
    }else {
        VARIANT var_url;
        DWORD len;
        int cmdlen;

        if(!strncasecmp(cmdline, "-nohome", 7))
            cmdline += 7;
        while(*cmdline == ' ' || *cmdline == '\t')
            cmdline++;
        cmdlen = lstrlenA(cmdline);
        if(cmdlen > 2 && cmdline[0] == '"' && cmdline[cmdlen-1] == '"') {
            cmdline++;
            cmdlen -= 2;
        }

        V_VT(&var_url) = VT_BSTR;

        len = MultiByteToWideChar(CP_ACP, 0, cmdline, cmdlen, NULL, 0);
        V_BSTR(&var_url) = SysAllocStringLen(NULL, len);
        MultiByteToWideChar(CP_ACP, 0, cmdline, cmdlen, V_BSTR(&var_url), len);

        /* navigate to the first page */
        IWebBrowser2_Navigate2(wb, &var_url, NULL, NULL, NULL, NULL);

        SysFreeString(V_BSTR(&var_url));
    }

    return wb;
}

HRESULT InternetExplorer_Create(IUnknown *pOuter, REFIID riid, void **ppv)
{
    InternetExplorer *ret;
    HRESULT hres;

    TRACE("(%p %s %p)\n", pOuter, debugstr_guid(riid), ppv);

    ret = heap_alloc_zero(sizeof(InternetExplorer));
    ret->ref = 0;

    ret->doc_host.disp = (IDispatch*)WEBBROWSER2(ret);
    DocHost_Init(&ret->doc_host, (IDispatch*)WEBBROWSER2(ret));

    InternetExplorer_WebBrowser_Init(ret);

    create_frame_hwnd(ret);
    ret->doc_host.frame_hwnd = ret->frame_hwnd;

    hres = IWebBrowser2_QueryInterface(WEBBROWSER2(ret), riid, ppv);
    if(FAILED(hres)) {
        heap_free(ret);
        return hres;
    }

    return hres;
}

/******************************************************************
 *		IEWinMain            (SHDOCVW.101)
 *
 * Only returns on error.
 */
DWORD WINAPI IEWinMain(LPSTR szCommandLine, int nShowWindow)
{
    IWebBrowser2 *wb = NULL;
    MSG msg;
    HRESULT hres;

    TRACE("%s %d\n", debugstr_a(szCommandLine), nShowWindow);

    if(*szCommandLine == '-' || *szCommandLine == '/') {
        if(!strcasecmp(szCommandLine+1, "regserver"))
            return register_iexplore(TRUE);
        if(!strcasecmp(szCommandLine+1, "unregserver"))
            return register_iexplore(FALSE);
    }

    CoInitialize(NULL);

    hres = register_class_object(TRUE);
    if(FAILED(hres)) {
        CoUninitialize();
        ExitProcess(1);
    }

    if(strcasecmp(szCommandLine, "-embedding"))
        wb = create_ie_window(szCommandLine);

    /* run the message loop for this thread */
    while (GetMessageW(&msg, 0, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    if(wb)
        IWebBrowser2_Release(wb);

    register_class_object(FALSE);

    CoUninitialize();

    ExitProcess(0);
    return 0;
}
