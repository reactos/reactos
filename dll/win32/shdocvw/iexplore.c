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
#include "winreg.h"
#include "shlwapi.h"
#include "intshcut.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(shdocvw);

#define IDI_APPICON 1

#define DOCHOST_THIS(iface) DEFINE_THIS2(InternetExplorer,doc_host,iface)

static const WCHAR szIEWinFrame[] = { 'I','E','F','r','a','m','e',0 };

/* Windows uses "Microsoft Internet Explorer" */
static const WCHAR wszWineInternetExplorer[] =
        {'W','i','n','e',' ','I','n','t','e','r','n','e','t',' ','E','x','p','l','o','r','e','r',0};

HRESULT update_ie_statustext(InternetExplorer* This, LPCWSTR text)
{
    if(!SendMessageW(This->status_hwnd, SB_SETTEXTW, MAKEWORD(SB_SIMPLEID, 0), (LPARAM)text))
        return E_FAIL;

    return S_OK;
}

static void adjust_ie_docobj_rect(HWND frame, RECT* rc)
{
    HWND hwndRebar = GetDlgItem(frame, IDC_BROWSE_REBAR);
    HWND hwndStatus = GetDlgItem(frame, IDC_BROWSE_STATUSBAR);
    INT barHeight = SendMessageW(hwndRebar, RB_GETBARHEIGHT, 0, 0);

    rc->top += barHeight;
    rc->bottom -= barHeight;

    if(IsWindowVisible(hwndStatus))
    {
        RECT statusrc;

        GetClientRect(hwndStatus, &statusrc);
        rc->bottom -= statusrc.bottom - statusrc.top;
    }
}

static HMENU get_tb_menu(HMENU menu)
{
    HMENU menu_view = GetSubMenu(menu, 1);

    return GetSubMenu(menu_view, 0);
}

static HMENU get_fav_menu(HMENU menu)
{
    return GetSubMenu(menu, 2);
}

static LPWSTR get_fav_url_from_id(HMENU menu, UINT id)
{
    MENUITEMINFOW item;

    item.cbSize = sizeof(item);
    item.fMask = MIIM_DATA;

    if(!GetMenuItemInfoW(menu, id, FALSE, &item))
        return NULL;

    return (LPWSTR)item.dwItemData;
}

static void free_fav_menu_data(HMENU menu)
{
    LPWSTR url;
    int i;

    for(i = 0; (url = get_fav_url_from_id(menu, ID_BROWSE_GOTOFAV_FIRST + i)); i++)
        heap_free( url );
}

static int get_menu_item_count(HMENU menu)
{
    MENUITEMINFOW item;
    int count = 0;
    int i;

    item.cbSize = sizeof(item);
    item.fMask = MIIM_DATA | MIIM_SUBMENU;

    for(i = 0; GetMenuItemInfoW(menu, i, TRUE, &item); i++)
    {
        if(item.hSubMenu)
            count += get_menu_item_count(item.hSubMenu);
        else
            count++;
    }

    return count;
}

static void add_fav_to_menu(HMENU favmenu, HMENU menu, LPWSTR title, LPCWSTR url)
{
    MENUITEMINFOW item;
    /* Subtract the number of standard elements in the Favorites menu */
    int favcount = get_menu_item_count(favmenu) - 2;
    LPWSTR urlbuf;

    if(favcount > (ID_BROWSE_GOTOFAV_MAX - ID_BROWSE_GOTOFAV_FIRST))
    {
        FIXME("Add support for more than %d Favorites\n", favcount);
        return;
    }

    urlbuf = heap_alloc((lstrlenW(url) + 1) * sizeof(WCHAR));

    if(!urlbuf)
        return;

    lstrcpyW(urlbuf, url);

    item.cbSize = sizeof(item);
    item.fMask = MIIM_FTYPE | MIIM_STRING | MIIM_DATA | MIIM_ID;
    item.fType = MFT_STRING;
    item.dwTypeData = title;
    item.wID = ID_BROWSE_GOTOFAV_FIRST + favcount;
    item.dwItemData = (ULONG_PTR)urlbuf;
    InsertMenuItemW(menu, -1, TRUE, &item);
}

static void add_favs_to_menu(HMENU favmenu, HMENU menu, LPCWSTR dir)
{
    WCHAR path[MAX_PATH*2];
    const WCHAR search[] = {'*',0};
    WCHAR* filename;
    HANDLE findhandle;
    WIN32_FIND_DATAW finddata;
    IUniformResourceLocatorW* urlobj;
    IPersistFile* urlfile;
    HRESULT res;

    lstrcpyW(path, dir);
    PathAppendW(path, search);

    findhandle = FindFirstFileW(path, &finddata);

    if(findhandle == INVALID_HANDLE_VALUE)
        return;

    res = CoCreateInstance(&CLSID_InternetShortcut, NULL, CLSCTX_INPROC_SERVER, &IID_IUniformResourceLocatorW, (PVOID*)&urlobj);

    if(SUCCEEDED(res))
        res = IUnknown_QueryInterface(urlobj, &IID_IPersistFile, (PVOID*)&urlfile);

    if(SUCCEEDED(res))
    {
        filename = path + lstrlenW(path) - lstrlenW(search);

        do
        {
            lstrcpyW(filename, finddata.cFileName);

            if(finddata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            {
                MENUITEMINFOW item;
                const WCHAR ignore1[] = {'.','.',0};
                const WCHAR ignore2[] = {'.',0};

                if(!lstrcmpW(filename, ignore1) || !lstrcmpW(filename, ignore2))
                    continue;

                item.cbSize = sizeof(item);
                item.fMask = MIIM_STRING | MIIM_SUBMENU;
                item.dwTypeData = filename;
                item.hSubMenu = CreatePopupMenu();
                InsertMenuItemW(menu, -1, TRUE, &item);
                add_favs_to_menu(favmenu, item.hSubMenu, path);
            } else
            {
                WCHAR* fileext;
                WCHAR* url = NULL;
                const WCHAR urlext[] = {'.','u','r','l',0};

                if(lstrcmpiW(PathFindExtensionW(filename), urlext))
                    continue;

                if(FAILED(IPersistFile_Load(urlfile, path, 0)))
                    continue;

                urlobj->lpVtbl->GetURL(urlobj, &url);

                if(!url)
                    continue;

                fileext = filename + lstrlenW(filename) - lstrlenW(urlext);
                *fileext = 0;
                add_fav_to_menu(favmenu, menu, filename, url);
            }
        } while(FindNextFileW(findhandle, &finddata));
    }

    if(urlfile)
        IPersistFile_Release(urlfile);

    if(urlobj)
        IUnknown_Release(urlobj);

    FindClose(findhandle);
}

static void add_tbs_to_menu(HMENU menu)
{
    HUSKEY toolbar_handle;
    WCHAR toolbar_key[] = {'S','o','f','t','w','a','r','e','\\',
                           'M','i','c','r','o','s','o','f','t','\\',
                           'I','n','t','e','r','n','e','t',' ',
                           'E','x','p','l','o','r','e','r','\\',
                           'T','o','o','l','b','a','r',0};

    if(SHRegOpenUSKeyW(toolbar_key, KEY_READ, NULL, &toolbar_handle, TRUE) == ERROR_SUCCESS)
    {
        HUSKEY classes_handle;
        WCHAR classes_key[] = {'S','o','f','t','w','a','r','e','\\',
                               'C','l','a','s','s','e','s','\\','C','L','S','I','D',0};
        WCHAR guid[39];
        DWORD value_len = sizeof(guid)/sizeof(guid[0]);
        int i;

        if(SHRegOpenUSKeyW(classes_key, KEY_READ, NULL, &classes_handle, TRUE) != ERROR_SUCCESS)
        {
            SHRegCloseUSKey(toolbar_handle);
            ERR("Failed to open key %s\n", debugstr_w(classes_key));
            return;
        }

        for(i = 0; SHRegEnumUSValueW(toolbar_handle, i, guid, &value_len, NULL, NULL, NULL, SHREGENUM_HKLM) == ERROR_SUCCESS; i++)
        {
            WCHAR tb_name[100];
            DWORD tb_name_len = sizeof(tb_name)/sizeof(tb_name[0]);
            HUSKEY tb_class_handle;
            MENUITEMINFOW item;
            LSTATUS ret;
            value_len = sizeof(guid)/sizeof(guid[0]);

            if(lstrlenW(guid) != 38)
            {
                TRACE("Found invalid IE toolbar entry: %s\n", debugstr_w(guid));
                continue;
            }

            if(SHRegOpenUSKeyW(guid, KEY_READ, classes_handle, &tb_class_handle, TRUE) != ERROR_SUCCESS)
            {
                ERR("Failed to get class info for %s\n", debugstr_w(guid));
                continue;
            }

            ret = SHRegQueryUSValueW(tb_class_handle, NULL, NULL, tb_name, &tb_name_len, TRUE, NULL, 0);

            SHRegCloseUSKey(tb_class_handle);

            if(ret != ERROR_SUCCESS)
            {
                ERR("Failed to get toolbar name for %s\n", debugstr_w(guid));
                continue;
            }

            item.cbSize = sizeof(item);
            item.fMask = MIIM_STRING;
            item.dwTypeData = tb_name;
            InsertMenuItemW(menu, GetMenuItemCount(menu), TRUE, &item);
        }

        SHRegCloseUSKey(classes_handle);
        SHRegCloseUSKey(toolbar_handle);
    }
}

static HMENU create_ie_menu(void)
{
    HMENU menu = LoadMenuW(shdocvw_hinstance, MAKEINTRESOURCEW(IDR_BROWSE_MAIN_MENU));
    HMENU favmenu = get_fav_menu(menu);
    WCHAR path[MAX_PATH];

    add_tbs_to_menu(get_tb_menu(menu));

    if(SHGetFolderPathW(NULL, CSIDL_COMMON_FAVORITES, NULL, SHGFP_TYPE_CURRENT, path) == S_OK)
        add_favs_to_menu(favmenu, favmenu, path);

    if(SHGetFolderPathW(NULL, CSIDL_FAVORITES, NULL, SHGFP_TYPE_CURRENT, path) == S_OK)
        add_favs_to_menu(favmenu, favmenu, path);

    return menu;
}

static void ie_navigate(InternetExplorer* This, LPCWSTR url)
{
    VARIANT variant;

    V_VT(&variant) = VT_BSTR;
    V_BSTR(&variant) = SysAllocString(url);

    IWebBrowser2_Navigate2(WEBBROWSER2(This), &variant, NULL, NULL, NULL, NULL);

    SysFreeString(V_BSTR(&variant));
}

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

                        GetWindowTextW(hwndurl, V_BSTR(&url), len + 1);
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

static void add_tb_separator(HWND hwnd)
{
    TBBUTTON btn;

    ZeroMemory(&btn, sizeof(btn));

    btn.iBitmap = 3;
    btn.fsStyle = BTNS_SEP;
    SendMessageW(hwnd, TB_ADDBUTTONSW, 1, (LPARAM)&btn);
}

static void add_tb_button(HWND hwnd, int bmp, int cmd, int strId)
{
    TBBUTTON btn;
    WCHAR buf[30];

    LoadStringW(shdocvw_hinstance, strId, buf, sizeof(buf)/sizeof(buf[0]));

    btn.iBitmap = bmp;
    btn.idCommand = cmd;
    btn.fsState = TBSTATE_ENABLED;
    btn.fsStyle = BTNS_SHOWTEXT;
    btn.dwData = 0;
    btn.iString = (INT_PTR)buf;

    SendMessageW(hwnd, TB_ADDBUTTONSW, 1, (LPARAM)&btn);
}

static void create_rebar(HWND hwnd)
{
    HWND hwndRebar;
    HWND hwndAddress;
    HWND hwndToolbar;
    REBARINFO rebarinf;
    REBARBANDINFOW bandinf;
    WCHAR addr[40];
    HIMAGELIST imagelist;
    WCHAR idb_ietoolbar[] = {'I','D','B','_','I','E','T','O','O','L','B','A','R',0};

    LoadStringW(shdocvw_hinstance, IDS_ADDRESS, addr, sizeof(addr)/sizeof(addr[0]));

    hwndRebar = CreateWindowExW(WS_EX_TOOLWINDOW, REBARCLASSNAMEW, NULL, WS_CHILD|WS_VISIBLE|WS_CLIPSIBLINGS|WS_CLIPCHILDREN|RBS_VARHEIGHT|CCS_TOP|CCS_NODIVIDER, 0, 0, 0, 0, hwnd, (HMENU)IDC_BROWSE_REBAR, shdocvw_hinstance, NULL);

    rebarinf.cbSize = sizeof(rebarinf);
    rebarinf.fMask = 0;
    rebarinf.himl = NULL;
    rebarinf.cbSize = sizeof(rebarinf);

    SendMessageW(hwndRebar, RB_SETBARINFO, 0, (LPARAM)&rebarinf);

    hwndToolbar = CreateWindowExW(TBSTYLE_EX_MIXEDBUTTONS, TOOLBARCLASSNAMEW, NULL, TBSTYLE_FLAT | WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, hwndRebar, (HMENU)IDC_BROWSE_TOOLBAR, shdocvw_hinstance, NULL);

    imagelist = ImageList_LoadImageW(shdocvw_hinstance, idb_ietoolbar, 32, 0, CLR_NONE, IMAGE_BITMAP, LR_CREATEDIBSECTION);

    SendMessageW(hwndToolbar, TB_SETIMAGELIST, 0, (LPARAM)imagelist);
    SendMessageW(hwndToolbar, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);
    add_tb_button(hwndToolbar, 0, 0, IDS_TB_BACK);
    add_tb_button(hwndToolbar, 1, 0, IDS_TB_FORWARD);
    add_tb_button(hwndToolbar, 2, 0, IDS_TB_STOP);
    add_tb_button(hwndToolbar, 3, 0, IDS_TB_REFRESH);
    add_tb_button(hwndToolbar, 4, ID_BROWSE_HOME, IDS_TB_HOME);
    add_tb_separator(hwndToolbar);
    add_tb_button(hwndToolbar, 5, ID_BROWSE_PRINT, IDS_TB_PRINT);
    SendMessageW(hwndToolbar, TB_SETBUTTONSIZE, 0, MAKELPARAM(55,50));
    SendMessageW(hwndToolbar, TB_AUTOSIZE, 0, 0);

    bandinf.cbSize = sizeof(bandinf);
    bandinf.fMask = RBBIM_STYLE | RBBIM_CHILD | RBBIM_CHILDSIZE | RBBIM_SIZE;
    bandinf.fStyle = RBBS_CHILDEDGE;
    bandinf.cx = 100;
    bandinf.cyMinChild = 52;
    bandinf.hwndChild = hwndToolbar;

    SendMessageW(hwndRebar, RB_INSERTBANDW, -1, (LPARAM)&bandinf);

    hwndAddress = CreateWindowExW(0, WC_COMBOBOXEXW, NULL, WS_BORDER|WS_CHILD|WS_VISIBLE|CBS_DROPDOWN, 0, 0, 100,20,hwndRebar, (HMENU)IDC_BROWSE_ADDRESSBAR, shdocvw_hinstance, NULL);

    bandinf.fMask |= RBBIM_TEXT;
    bandinf.fStyle = RBBS_CHILDEDGE | RBBS_BREAK;
    bandinf.lpText = addr;
    bandinf.cyMinChild = 20;
    bandinf.hwndChild = hwndAddress;

    SendMessageW(hwndRebar, RB_INSERTBANDW, -1, (LPARAM)&bandinf);
}

static LRESULT iewnd_OnCreate(HWND hwnd, LPCREATESTRUCTW lpcs)
{
    InternetExplorer* This = (InternetExplorer*)lpcs->lpCreateParams;
    SetWindowLongPtrW(hwnd, 0, (LONG_PTR) lpcs->lpCreateParams);

    This->menu = create_ie_menu();

    This->status_hwnd = CreateStatusWindowW(CCS_NODIVIDER|WS_CHILD|WS_VISIBLE, NULL, hwnd, IDC_BROWSE_STATUSBAR);
    SendMessageW(This->status_hwnd, SB_SIMPLE, TRUE, 0);

    create_rebar(hwnd);

    return 0;
}

static LRESULT iewnd_OnSize(InternetExplorer *This, INT width, INT height)
{
    HWND hwndRebar = GetDlgItem(This->frame_hwnd, IDC_BROWSE_REBAR);
    INT barHeight = SendMessageW(hwndRebar, RB_GETBARHEIGHT, 0, 0);
    RECT docarea = {0, 0, width, height};

    SendMessageW(This->status_hwnd, WM_SIZE, 0, 0);

    adjust_ie_docobj_rect(This->frame_hwnd, &docarea);

    if(This->doc_host.hwnd)
        SetWindowPos(This->doc_host.hwnd, NULL, docarea.left, docarea.top, docarea.right, docarea.bottom,
                     SWP_NOZORDER | SWP_NOACTIVATE);

    SetWindowPos(hwndRebar, NULL, 0, 0, width, barHeight, SWP_NOZORDER | SWP_NOACTIVATE);

    return 0;
}

static LRESULT iewnd_OnNotify(InternetExplorer *This, WPARAM wparam, LPARAM lparam)
{
    NMHDR* hdr = (NMHDR*)lparam;

    if(hdr->idFrom == IDC_BROWSE_ADDRESSBAR && hdr->code == CBEN_ENDEDITW)
    {
        NMCBEENDEDITW* info = (NMCBEENDEDITW*)lparam;

        if(info->fChanged && info->iWhy == CBENF_RETURN && info->szText)
        {
            VARIANT vt;

            V_VT(&vt) = VT_BSTR;
            V_BSTR(&vt) = SysAllocString(info->szText);

            IWebBrowser2_Navigate2(WEBBROWSER2(This), &vt, NULL, NULL, NULL, NULL);

            SysFreeString(V_BSTR(&vt));

            return 0;
        }
    }

    return 0;
}

static LRESULT iewnd_OnDestroy(InternetExplorer *This)
{
    HWND hwndRebar = GetDlgItem(This->frame_hwnd, IDC_BROWSE_REBAR);
    HWND hwndToolbar = GetDlgItem(hwndRebar, IDC_BROWSE_TOOLBAR);
    HIMAGELIST list = (HIMAGELIST)SendMessageW(hwndToolbar, TB_GETIMAGELIST, 0, 0);

    TRACE("%p\n", This);

    free_fav_menu_data(get_fav_menu(This->menu));
    ImageList_Destroy(list);
    This->frame_hwnd = NULL;
    PostQuitMessage(0); /* FIXME */

    return 0;
}

static LRESULT iewnd_OnCommand(InternetExplorer *This, HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
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

        case ID_BROWSE_HOME:
            IWebBrowser2_GoHome(WEBBROWSER2(This));
            break;

        case ID_BROWSE_ABOUT:
            ie_dialog_about(hwnd);
            break;

        case ID_BROWSE_QUIT:
            iewnd_OnDestroy(This);
            break;

        default:
            if(LOWORD(wparam) >= ID_BROWSE_GOTOFAV_FIRST && LOWORD(wparam) <= ID_BROWSE_GOTOFAV_MAX)
            {
                LPCWSTR url = get_fav_url_from_id(get_fav_menu(This->menu), LOWORD(wparam));

                if(url)
                    ie_navigate(This, url);
            }
            return DefWindowProcW(hwnd, msg, wparam, lparam);
    }
    return 0;
}

static LRESULT update_addrbar(InternetExplorer *This, LPARAM lparam)
{
    HWND hwndRebar = GetDlgItem(This->frame_hwnd, IDC_BROWSE_REBAR);
    HWND hwndAddress = GetDlgItem(hwndRebar, IDC_BROWSE_ADDRESSBAR);
    HWND hwndEdit = (HWND)SendMessageW(hwndAddress, CBEM_GETEDITCONTROL, 0, 0);
    LPCWSTR url = (LPCWSTR)lparam;

    SendMessageW(hwndEdit, WM_SETTEXT, 0, (LPARAM)url);

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
    case WM_NOTIFY:
        return iewnd_OnNotify(This, wparam, lparam);
    case WM_DOCHOSTTASK:
        return process_dochost_task(&This->doc_host, lparam);
    case WM_UPDATEADDRBAR:
        return update_addrbar(This, lparam);
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

static void WINAPI DocHostContainer_GetDocObjRect(DocHost* This, RECT* rc)
{
    GetClientRect(This->frame_hwnd, rc);
    adjust_ie_docobj_rect(This->frame_hwnd, rc);
}

static HRESULT WINAPI DocHostContainer_SetStatusText(DocHost* This, LPCWSTR text)
{
    InternetExplorer* ie = DOCHOST_THIS(This);
    return update_ie_statustext(ie, text);
}

static void WINAPI DocHostContainer_SetURL(DocHost* This, LPCWSTR url)
{
    SendMessageW(This->frame_hwnd, WM_UPDATEADDRBAR, 0, (LPARAM)url);
}

static HRESULT DocHostContainer_exec(DocHost* This, const GUID *cmd_group, DWORD cmdid, DWORD execopt, VARIANT *in,
        VARIANT *out)
{
    return S_OK;
}
static const IDocHostContainerVtbl DocHostContainerVtbl = {
    DocHostContainer_GetDocObjRect,
    DocHostContainer_SetStatusText,
    DocHostContainer_SetURL,
    DocHostContainer_exec
};

HRESULT InternetExplorer_Create(IUnknown *pOuter, REFIID riid, void **ppv)
{
    InternetExplorer *ret;
    HRESULT hres;

    TRACE("(%p %s %p)\n", pOuter, debugstr_guid(riid), ppv);

    ret = heap_alloc_zero(sizeof(InternetExplorer));
    ret->ref = 0;

    ret->doc_host.disp = (IDispatch*)WEBBROWSER2(ret);
    DocHost_Init(&ret->doc_host, (IDispatch*)WEBBROWSER2(ret), &DocHostContainerVtbl);

    InternetExplorer_WebBrowser_Init(ret);

    HlinkFrame_Init(&ret->hlink_frame, (IUnknown*)WEBBROWSER2(ret), &ret->doc_host);

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
