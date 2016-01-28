/*
 * ieframe - Internet Explorer main frame window
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

#include "ieframe.h"

#include <mshtmcid.h>
#include <ddeml.h>

#define IDI_APPICON 1

#define WM_UPDATEADDRBAR    (WM_APP+1)

static const WCHAR szIEWinFrame[] = { 'I','E','F','r','a','m','e',0 };

/* Windows uses "Microsoft Internet Explorer" */
static const WCHAR wszWineInternetExplorer[] =
        {'W','i','n','e',' ','I','n','t','e','r','n','e','t',' ','E','x','p','l','o','r','e','r',0};

static LONG obj_cnt;
static DWORD dde_inst;
static HSZ ddestr_iexplore, ddestr_openurl;
static struct list ie_list = LIST_INIT(ie_list);

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
    IPersistFile* urlfile = NULL;
    HRESULT res;

    lstrcpyW(path, dir);
    PathAppendW(path, search);

    findhandle = FindFirstFileW(path, &finddata);

    if(findhandle == INVALID_HANDLE_VALUE)
        return;

    res = CoCreateInstance(&CLSID_InternetShortcut, NULL, CLSCTX_INPROC_SERVER, &IID_IUniformResourceLocatorW, (PVOID*)&urlobj);

    if(SUCCEEDED(res))
        res = IUnknown_QueryInterface((IUnknown*)urlobj, &IID_IPersistFile, (PVOID*)&urlfile);

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
        IUnknown_Release((IUnknown*)urlobj);

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
    HMENU menu = LoadMenuW(ieframe_instance, MAKEINTRESOURCEW(IDR_BROWSE_MAIN_MENU));
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

    IWebBrowser2_Navigate2(&This->IWebBrowser2_iface, &variant, NULL, NULL, NULL, NULL);

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

                    EnableWindow(GetDlgItem(hwnd, IDOK), len != 0);
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
                        IWebBrowser2_Navigate2(&This->IWebBrowser2_iface, &url, NULL, NULL, NULL, NULL);

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

#ifdef __REACTOS__
static void ie_dialog_properties(HWND hwnd)
{
    ShellExecuteW(hwnd, NULL, L"inetcpl.cpl", NULL, NULL, 0);
}
#endif

static void add_tb_separator(InternetExplorer *ie)
{
    TBBUTTON btn;

    ZeroMemory(&btn, sizeof(btn));

    btn.iBitmap = 3;
    btn.fsStyle = BTNS_SEP;
    SendMessageW(ie->toolbar_hwnd, TB_ADDBUTTONSW, 1, (LPARAM)&btn);
}

static void add_tb_button(InternetExplorer *ie, int bmp, int cmd, int strId)
{
    TBBUTTON btn;
    WCHAR buf[30];

    LoadStringW(ieframe_instance, strId, buf, sizeof(buf)/sizeof(buf[0]));

    btn.iBitmap = bmp;
    btn.idCommand = cmd;
    btn.fsState = TBSTATE_ENABLED;
    btn.fsStyle = BTNS_SHOWTEXT;
    btn.dwData = 0;
    btn.iString = (INT_PTR)buf;

    SendMessageW(ie->toolbar_hwnd, TB_ADDBUTTONSW, 1, (LPARAM)&btn);
}

static void enable_toolbar_button(InternetExplorer *ie, int command, BOOL enable)
{
    SendMessageW(ie->toolbar_hwnd, TB_ENABLEBUTTON, command, enable);
}

static void create_rebar(InternetExplorer *ie)
{
    HWND hwndRebar;
    HWND hwndAddress;
    REBARINFO rebarinf;
    REBARBANDINFOW bandinf;
    WCHAR addr[40];
    HIMAGELIST imagelist;
    SIZE toolbar_size;

    LoadStringW(ieframe_instance, IDS_ADDRESS, addr, sizeof(addr)/sizeof(addr[0]));

    hwndRebar = CreateWindowExW(WS_EX_TOOLWINDOW, REBARCLASSNAMEW, NULL,
            WS_CHILD|WS_VISIBLE|WS_CLIPSIBLINGS|WS_CLIPCHILDREN|RBS_VARHEIGHT|CCS_TOP|CCS_NODIVIDER, 0, 0, 0, 0,
            ie->frame_hwnd, (HMENU)IDC_BROWSE_REBAR, ieframe_instance, NULL);

    rebarinf.cbSize = sizeof(rebarinf);
    rebarinf.fMask = 0;
    rebarinf.himl = NULL;

    SendMessageW(hwndRebar, RB_SETBARINFO, 0, (LPARAM)&rebarinf);

    ie->toolbar_hwnd = CreateWindowExW(TBSTYLE_EX_MIXEDBUTTONS, TOOLBARCLASSNAMEW, NULL, TBSTYLE_FLAT | WS_CHILD | WS_VISIBLE | CCS_NORESIZE,
            0, 0, 0, 0, hwndRebar, (HMENU)IDC_BROWSE_TOOLBAR, ieframe_instance, NULL);

    imagelist = ImageList_LoadImageW(ieframe_instance, MAKEINTRESOURCEW(IDB_IETOOLBAR), 32, 0, CLR_NONE, IMAGE_BITMAP, LR_CREATEDIBSECTION);

    SendMessageW(ie->toolbar_hwnd, TB_SETIMAGELIST, 0, (LPARAM)imagelist);
    SendMessageW(ie->toolbar_hwnd, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);
    add_tb_button(ie, 0, ID_BROWSE_BACK, IDS_TB_BACK);
    add_tb_button(ie, 1, ID_BROWSE_FORWARD, IDS_TB_FORWARD);
    add_tb_button(ie, 2, ID_BROWSE_STOP, IDS_TB_STOP);
    add_tb_button(ie, 3, ID_BROWSE_REFRESH, IDS_TB_REFRESH);
    add_tb_button(ie, 4, ID_BROWSE_HOME, IDS_TB_HOME);
    add_tb_separator(ie);
    add_tb_button(ie, 5, ID_BROWSE_PRINT, IDS_TB_PRINT);
    SendMessageW(ie->toolbar_hwnd, TB_SETBUTTONSIZE, 0, MAKELPARAM(55,50));
    SendMessageW(ie->toolbar_hwnd, TB_GETMAXSIZE, 0, (LPARAM)&toolbar_size);

    bandinf.cbSize = sizeof(bandinf);
    bandinf.fMask = RBBIM_STYLE | RBBIM_CHILD | RBBIM_CHILDSIZE;
    bandinf.fStyle = RBBS_CHILDEDGE;
    bandinf.cxMinChild = toolbar_size.cx;
    bandinf.cyMinChild = toolbar_size.cy+2;
    bandinf.hwndChild = ie->toolbar_hwnd;

    SendMessageW(hwndRebar, RB_INSERTBANDW, -1, (LPARAM)&bandinf);

    hwndAddress = CreateWindowExW(0, WC_COMBOBOXEXW, NULL, WS_BORDER|WS_CHILD|WS_VISIBLE|CBS_DROPDOWN,
            0, 0, 100,20,hwndRebar, (HMENU)IDC_BROWSE_ADDRESSBAR, ieframe_instance, NULL);

    bandinf.fMask |= RBBIM_TEXT;
    bandinf.fStyle = RBBS_CHILDEDGE | RBBS_BREAK;
    bandinf.lpText = addr;
    bandinf.cxMinChild = 100;
    bandinf.cyMinChild = 20;
    bandinf.hwndChild = hwndAddress;

    SendMessageW(hwndRebar, RB_INSERTBANDW, -1, (LPARAM)&bandinf);
}

static LRESULT iewnd_OnCreate(HWND hwnd, LPCREATESTRUCTW lpcs)
{
    InternetExplorer* This = (InternetExplorer*)lpcs->lpCreateParams;
    SetWindowLongPtrW(hwnd, 0, (LONG_PTR) lpcs->lpCreateParams);

    This->doc_host.frame_hwnd = This->frame_hwnd = hwnd;

    This->menu = create_ie_menu();

    This->status_hwnd = CreateStatusWindowW(WS_VISIBLE|WS_CHILD|SBT_NOBORDERS|CCS_NODIVIDER,
                                            NULL, hwnd, IDC_BROWSE_STATUSBAR);
    SendMessageW(This->status_hwnd, SB_SIMPLE, TRUE, 0);

    create_rebar(This);

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

        if(info->fChanged && info->iWhy == CBENF_RETURN)
        {
            VARIANT vt;

            V_VT(&vt) = VT_BSTR;
            V_BSTR(&vt) = SysAllocString(info->szText);

            IWebBrowser2_Navigate2(&This->IWebBrowser2_iface, &vt, NULL, NULL, NULL, NULL);

            SysFreeString(V_BSTR(&vt));

            return 0;
        }
    }

    if(hdr->idFrom == IDC_BROWSE_REBAR && hdr->code == RBN_HEIGHTCHANGE)
    {
        RECT docarea;

        GetClientRect(This->frame_hwnd, &docarea);
        adjust_ie_docobj_rect(This->frame_hwnd, &docarea);

        if(This->doc_host.hwnd)
            SetWindowPos(This->doc_host.hwnd, NULL, docarea.left, docarea.top, docarea.right, docarea.bottom,
                    SWP_NOZORDER | SWP_NOACTIVATE);
    }

    return 0;
}

static LRESULT iewnd_OnDestroy(InternetExplorer *This)
{
    HIMAGELIST list = (HIMAGELIST)SendMessageW(This->toolbar_hwnd, TB_GETIMAGELIST, 0, 0);

    TRACE("%p\n", This);

    free_fav_menu_data(get_fav_menu(This->menu));
    ImageList_Destroy(list);
    This->frame_hwnd = NULL;

    return 0;
}

static LRESULT iewnd_OnCommand(InternetExplorer *This, HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch(LOWORD(wparam))
    {
        case ID_BROWSE_OPEN:
            DialogBoxParamW(ieframe_instance, MAKEINTRESOURCEW(IDD_BROWSE_OPEN), hwnd, ie_dialog_open_proc, (LPARAM)This);
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
            IWebBrowser2_GoHome(&This->IWebBrowser2_iface);
            break;

        case ID_BROWSE_BACK:
            IWebBrowser2_GoBack(&This->IWebBrowser2_iface);
            break;

        case ID_BROWSE_FORWARD:
            IWebBrowser2_GoForward(&This->IWebBrowser2_iface);
            break;

        case ID_BROWSE_STOP:
            IWebBrowser2_Stop(&This->IWebBrowser2_iface);
            break;

        case ID_BROWSE_REFRESH:
            IWebBrowser2_Refresh(&This->IWebBrowser2_iface);
            break;

        case ID_BROWSE_ABOUT:
            ie_dialog_about(hwnd);
            break;

#ifdef __REACTOS__
        case ID_BROWSE_PROPERTIES:
            ie_dialog_properties(hwnd);
            break;
#endif

        case ID_BROWSE_QUIT:
            ShowWindow(hwnd, SW_HIDE);
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

static LRESULT WINAPI ie_window_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    InternetExplorer *This = (InternetExplorer*) GetWindowLongPtrW(hwnd, 0);

    switch (msg)
    {
    case WM_CREATE:
        return iewnd_OnCreate(hwnd, (LPCREATESTRUCTW)lparam);
    case WM_CLOSE:
        TRACE("WM_CLOSE\n");
        ShowWindow(hwnd, SW_HIDE);
        return 0;
    case WM_SHOWWINDOW:
        TRACE("WM_SHOWWINDOW %lx\n", wparam);
        if(wparam) {
            IWebBrowser2_AddRef(&This->IWebBrowser2_iface);
            InterlockedIncrement(&This->extern_ref);
        }else {
            release_extern_ref(This, TRUE);
            IWebBrowser2_Release(&This->IWebBrowser2_iface);
        }
        break;
    case WM_DESTROY:
        return iewnd_OnDestroy(This);
    case WM_SIZE:
        return iewnd_OnSize(This, LOWORD(lparam), HIWORD(lparam));
    case WM_COMMAND:
        return iewnd_OnCommand(This, hwnd, msg, wparam, lparam);
    case WM_NOTIFY:
        return iewnd_OnNotify(This, wparam, lparam);
    case WM_DOCHOSTTASK:
        return process_dochost_tasks(&This->doc_host);
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
    wc.hInstance = ieframe_instance;
    wc.hIcon = LoadIconW(GetModuleHandleW(0), MAKEINTRESOURCEW(IDI_APPICON));
    wc.hIconSm = LoadImageW(GetModuleHandleW(0), MAKEINTRESOURCEW(IDI_APPICON), IMAGE_ICON,
                            GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_SHARED);
    wc.hCursor = LoadCursorW(0, (LPWSTR)IDC_ARROW);
    wc.hbrBackground = 0;
    wc.lpszClassName = szIEWinFrame;
    wc.lpszMenuName = NULL;

    RegisterClassExW(&wc);
}

void unregister_iewindow_class(void)
{
    UnregisterClassW(szIEWinFrame, ieframe_instance);
}

static void create_frame_hwnd(InternetExplorer *This)
{
    CreateWindowExW(
            WS_EX_WINDOWEDGE,
            szIEWinFrame, wszWineInternetExplorer,
            WS_CLIPCHILDREN | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME
                | WS_MINIMIZEBOX | WS_MAXIMIZEBOX,
            CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
            NULL, NULL /* FIXME */, ieframe_instance, This);

    create_doc_view_hwnd(&This->doc_host);
}

static inline InternetExplorer *impl_from_DocHost(DocHost *iface)
{
    return CONTAINING_RECORD(iface, InternetExplorer, doc_host);
}

static ULONG IEDocHost_addref(DocHost *iface)
{
    InternetExplorer *This = impl_from_DocHost(iface);
    return IWebBrowser2_AddRef(&This->IWebBrowser2_iface);
}

static ULONG IEDocHost_release(DocHost *iface)
{
    InternetExplorer *This = impl_from_DocHost(iface);
    return IWebBrowser2_Release(&This->IWebBrowser2_iface);
}

static void DocHostContainer_get_docobj_rect(DocHost *This, RECT *rc)
{
    GetClientRect(This->frame_hwnd, rc);
    adjust_ie_docobj_rect(This->frame_hwnd, rc);
}

static HRESULT DocHostContainer_set_status_text(DocHost *iface, const WCHAR *text)
{
    InternetExplorer *This = impl_from_DocHost(iface);
    return update_ie_statustext(This, text);
}

static void DocHostContainer_on_command_state_change(DocHost *iface, LONG command, BOOL enable)
{
    InternetExplorer *This = impl_from_DocHost(iface);

    switch(command) {
    case CSC_NAVIGATEBACK:
        enable_toolbar_button(This, ID_BROWSE_BACK, enable);
        break;
    case CSC_NAVIGATEFORWARD:
        enable_toolbar_button(This, ID_BROWSE_FORWARD, enable);
        break;
    }
}

static void DocHostContainer_set_url(DocHost* iface, const WCHAR *url)
{
    InternetExplorer *This = impl_from_DocHost(iface);

    This->nohome = FALSE;
    SendMessageW(This->frame_hwnd, WM_UPDATEADDRBAR, 0, (LPARAM)url);
}

static const IDocHostContainerVtbl DocHostContainerVtbl = {
    IEDocHost_addref,
    IEDocHost_release,
    DocHostContainer_get_docobj_rect,
    DocHostContainer_set_status_text,
    DocHostContainer_on_command_state_change,
    DocHostContainer_set_url
};

static HRESULT create_ie(InternetExplorer **ret_obj)
{
    InternetExplorer *ret;

    ret = heap_alloc_zero(sizeof(InternetExplorer));
    if(!ret)
        return E_OUTOFMEMORY;

    ret->ref = 1;

    DocHost_Init(&ret->doc_host, &ret->IWebBrowser2_iface, &DocHostContainerVtbl);

    InternetExplorer_WebBrowser_Init(ret);

    HlinkFrame_Init(&ret->hlink_frame, (IUnknown*)&ret->IWebBrowser2_iface, &ret->doc_host);

    create_frame_hwnd(ret);

    InterlockedIncrement(&obj_cnt);
    list_add_tail(&ie_list, &ret->entry);
    *ret_obj = ret;
    return S_OK;
}

HRESULT WINAPI InternetExplorer_Create(IClassFactory *iface, IUnknown *pOuter, REFIID riid, void **ppv)
{
    InternetExplorer *ret;
    HRESULT hres;

    TRACE("(%p %s %p)\n", pOuter, debugstr_guid(riid), ppv);

    hres = create_ie(&ret);
    if(FAILED(hres))
        return hres;

    hres = IWebBrowser2_QueryInterface(&ret->IWebBrowser2_iface, riid, ppv);
    IWebBrowser2_Release(&ret->IWebBrowser2_iface);
    if(FAILED(hres))
        return hres;

    return S_OK;
}

void released_obj(void)
{
    if(!InterlockedDecrement(&obj_cnt))
        PostQuitMessage(0);
}

static BOOL create_ie_window(const WCHAR *cmdline)
{
    InternetExplorer *ie;
    HRESULT hres;

    hres = create_ie(&ie);
    if(FAILED(hres))
        return FALSE;

    IWebBrowser2_put_Visible(&ie->IWebBrowser2_iface, VARIANT_TRUE);
    IWebBrowser2_put_MenuBar(&ie->IWebBrowser2_iface, VARIANT_TRUE);

    if(!*cmdline) {
        IWebBrowser2_GoHome(&ie->IWebBrowser2_iface);
    }else {
        VARIANT var_url;
        int cmdlen;

        static const WCHAR nohomeW[] = {'-','n','o','h','o','m','e'};

        while(*cmdline == ' ' || *cmdline == '\t')
            cmdline++;
        cmdlen = strlenW(cmdline);
        if(cmdlen > 2 && cmdline[0] == '"' && cmdline[cmdlen-1] == '"') {
            cmdline++;
            cmdlen -= 2;
        }

        if(cmdlen == sizeof(nohomeW)/sizeof(*nohomeW) && !memcmp(cmdline, nohomeW, sizeof(nohomeW))) {
            ie->nohome = TRUE;
        }else {
            V_VT(&var_url) = VT_BSTR;

            V_BSTR(&var_url) = SysAllocStringLen(cmdline, cmdlen);

            /* navigate to the first page */
            IWebBrowser2_Navigate2(&ie->IWebBrowser2_iface, &var_url, NULL, NULL, NULL, NULL);

            SysFreeString(V_BSTR(&var_url));
        }
    }

    IWebBrowser2_Release(&ie->IWebBrowser2_iface);
    return TRUE;
}

static HDDEDATA open_dde_url(WCHAR *dde_url)
{
    InternetExplorer *ie = NULL, *iter;
    WCHAR *url, *url_end;
    VARIANT urlv;
    HRESULT hres;

    TRACE("%s\n", debugstr_w(dde_url));

    url = dde_url;
    if(*url == '"') {
        url++;
        url_end = strchrW(url, '"');
        if(!url_end) {
            FIXME("missing string terminator\n");
            return 0;
        }
        *url_end = 0;
    }else {
        url_end = strchrW(url, ',');
        if(url_end)
            *url_end = 0;
        else
            url_end = url + strlenW(url);
    }

    LIST_FOR_EACH_ENTRY(iter, &ie_list, InternetExplorer, entry) {
        if(iter->nohome) {
            IWebBrowser2_AddRef(&iter->IWebBrowser2_iface);
            ie = iter;
            break;
        }
    }

    if(!ie) {
        hres = create_ie(&ie);
        if(FAILED(hres))
            return 0;
    }

    IWebBrowser2_put_Visible(&ie->IWebBrowser2_iface, VARIANT_TRUE);
    IWebBrowser2_put_MenuBar(&ie->IWebBrowser2_iface, VARIANT_TRUE);

    V_VT(&urlv) = VT_BSTR;
    V_BSTR(&urlv) = SysAllocStringLen(url, url_end-url);
    if(!V_BSTR(&urlv)) {
        IWebBrowser2_Release(&ie->IWebBrowser2_iface);
        return 0;
    }

    hres = IWebBrowser2_Navigate2(&ie->IWebBrowser2_iface, &urlv, NULL, NULL, NULL, NULL);
    if(FAILED(hres))
        return 0;

    IWebBrowser2_Release(&ie->IWebBrowser2_iface);
    return ULongToHandle(DDE_FACK);
}

static HDDEDATA WINAPI dde_proc(UINT type, UINT uFmt, HCONV hConv, HSZ hsz1, HSZ hsz2, HDDEDATA data,
        ULONG_PTR dwData1, ULONG_PTR dwData2)
{
    switch(type) {
    case XTYP_CONNECT:
        TRACE("XTYP_CONNECT %p\n", hsz1);
        return ULongToHandle(!DdeCmpStringHandles(hsz1, ddestr_openurl));

    case XTYP_EXECUTE: {
        WCHAR *url;
        DWORD size;
        HDDEDATA ret;

        TRACE("XTYP_EXECUTE %p\n", data);

        size = DdeGetData(data, NULL, 0, 0);
        if(!size) {
            WARN("size = 0\n");
            break;
        }

        url = heap_alloc(size);
        if(!url)
            break;

        if(DdeGetData(data, (BYTE*)url, size, 0) != size) {
            ERR("error during read\n");
            heap_free(url);
            break;
        }

        ret = open_dde_url(url);

        heap_free(url);
        return ret;
    }

    case XTYP_REQUEST:
        FIXME("XTYP_REQUEST\n");
        break;

    default:
        TRACE("type %d\n", type);
    }

    return NULL;
}

static void init_dde(void)
{
    UINT res;

    static const WCHAR iexploreW[] = {'I','E','x','p','l','o','r','e',0};
    static const WCHAR openurlW[] = {'W','W','W','_','O','p','e','n','U','R','L',0};

    res = DdeInitializeW(&dde_inst, dde_proc, CBF_SKIP_ALLNOTIFICATIONS | CBF_FAIL_ADVISES | CBF_FAIL_POKES, 0);
    if(res != DMLERR_NO_ERROR) {
        WARN("DdeInitialize failed: %u\n", res);
        return;
    }

    ddestr_iexplore = DdeCreateStringHandleW(dde_inst, iexploreW, CP_WINUNICODE);
    if(!ddestr_iexplore)
        WARN("Failed to create string handle: %u\n", DdeGetLastError(dde_inst));

    ddestr_openurl = DdeCreateStringHandleW(dde_inst, openurlW, CP_WINUNICODE);
    if(!ddestr_openurl)
        WARN("Failed to create string handle: %u\n", DdeGetLastError(dde_inst));

    if(!DdeNameService(dde_inst, ddestr_iexplore, 0, DNS_REGISTER))
        WARN("DdeNameService failed\n");
}

static void release_dde(void)
{
    if(ddestr_iexplore)
        DdeNameService(dde_inst, ddestr_iexplore, 0, DNS_UNREGISTER);
    if(ddestr_openurl)
        DdeFreeStringHandle(dde_inst, ddestr_openurl);
    if(ddestr_iexplore)
        DdeFreeStringHandle(dde_inst, ddestr_iexplore);
    DdeUninitialize(dde_inst);
}

/******************************************************************
 *		IEWinMain            (ieframe.101)
 *
 * Only returns on error.
 */
DWORD WINAPI IEWinMain(const WCHAR *cmdline, int nShowWindow)
{
    MSG msg;
    HRESULT hres;

    static const WCHAR embeddingW[] = {'-','e','m','b','e','d','d','i','n','g',0};

    TRACE("%s %d\n", debugstr_w(cmdline), nShowWindow);

    CoInitialize(NULL);

    hres = register_class_object(TRUE);
    if(FAILED(hres)) {
        CoUninitialize();
        ExitProcess(1);
    }

    init_dde();

    if(strcmpiW(cmdline, embeddingW)) {
        if(!create_ie_window(cmdline)) {
            CoUninitialize();
            ExitProcess(1);
        }
    }

    /* run the message loop for this thread */
    while (GetMessageW(&msg, 0, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    register_class_object(FALSE);
    release_dde();

    CoUninitialize();

    ExitProcess(0);
    return 0;
}
