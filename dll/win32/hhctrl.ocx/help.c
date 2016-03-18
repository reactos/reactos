/*
 * Help Viewer Implementation
 *
 * Copyright 2005 James Hawkins
 * Copyright 2007 Jacek Caban for CodeWeavers
 * Copyright 2011 Owen Rudge for CodeWeavers
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

#include "hhctrl.h"

#include <wingdi.h>
#include <wininet.h>

static LRESULT Help_OnSize(HWND hWnd);
static void ExpandContract(HHInfo *pHHInfo);

/* Window type defaults */

#define WINTYPE_DEFAULT_X           280
#define WINTYPE_DEFAULT_Y           100
#define WINTYPE_DEFAULT_WIDTH       740
#define WINTYPE_DEFAULT_HEIGHT      640
#define WINTYPE_DEFAULT_NAVWIDTH    250

#define TAB_TOP_PADDING     8
#define TAB_RIGHT_PADDING   4
#define TAB_MARGIN  8
#define EDIT_HEIGHT         20

struct list window_list = LIST_INIT(window_list);

static const WCHAR szEmpty[] = {0};

struct html_encoded_symbol {
    const char *html_code;
    char        ansi_symbol;
};

/*
 * Table mapping the conversion between HTML encoded symbols and their ANSI code page equivalent.
 * Note: Add additional entries in proper alphabetical order (a binary search is used on this table).
 */
static struct html_encoded_symbol html_encoded_symbols[] =
{
    {"AElig",  0xC6},
    {"Aacute", 0xC1},
    {"Acirc",  0xC2},
    {"Agrave", 0xC0},
    {"Aring",  0xC5},
    {"Atilde", 0xC3},
    {"Auml",   0xC4},
    {"Ccedil", 0xC7},
    {"ETH",    0xD0},
    {"Eacute", 0xC9},
    {"Ecirc",  0xCA},
    {"Egrave", 0xC8},
    {"Euml",   0xCB},
    {"Iacute", 0xCD},
    {"Icirc",  0xCE},
    {"Igrave", 0xCC},
    {"Iuml",   0xCF},
    {"Ntilde", 0xD1},
    {"Oacute", 0xD3},
    {"Ocirc",  0xD4},
    {"Ograve", 0xD2},
    {"Oslash", 0xD8},
    {"Otilde", 0xD5},
    {"Ouml",   0xD6},
    {"THORN",  0xDE},
    {"Uacute", 0xDA},
    {"Ucirc",  0xDB},
    {"Ugrave", 0xD9},
    {"Uuml",   0xDC},
    {"Yacute", 0xDD},
    {"aacute", 0xE1},
    {"acirc",  0xE2},
    {"acute",  0xB4},
    {"aelig",  0xE6},
    {"agrave", 0xE0},
    {"amp",    '&'},
    {"aring",  0xE5},
    {"atilde", 0xE3},
    {"auml",   0xE4},
    {"brvbar", 0xA6},
    {"ccedil", 0xE7},
    {"cedil",  0xB8},
    {"cent",   0xA2},
    {"copy",   0xA9},
    {"curren", 0xA4},
    {"deg",    0xB0},
    {"divide", 0xF7},
    {"eacute", 0xE9},
    {"ecirc",  0xEA},
    {"egrave", 0xE8},
    {"eth",    0xF0},
    {"euml",   0xEB},
    {"frac12", 0xBD},
    {"frac14", 0xBC},
    {"frac34", 0xBE},
    {"gt",     '>'},
    {"iacute", 0xED},
    {"icirc",  0xEE},
    {"iexcl",  0xA1},
    {"igrave", 0xEC},
    {"iquest", 0xBF},
    {"iuml",   0xEF},
    {"laquo",  0xAB},
    {"lt",     '<'},
    {"macr",   0xAF},
    {"micro",  0xB5},
    {"middot", 0xB7},
    {"nbsp",   ' '},
    {"not",    0xAC},
    {"ntilde", 0xF1},
    {"oacute", 0xF3},
    {"ocirc",  0xF4},
    {"ograve", 0xF2},
    {"ordf",   0xAA},
    {"ordm",   0xBA},
    {"oslash", 0xF8},
    {"otilde", 0xF5},
    {"ouml",   0xF6},
    {"para",   0xB6},
    {"plusmn", 0xB1},
    {"pound",  0xA3},
    {"quot",   '"'},
    {"raquo",  0xBB},
    {"reg",    0xAE},
    {"sect",   0xA7},
    {"shy",    0xAD},
    {"sup1",   0xB9},
    {"sup2",   0xB2},
    {"sup3",   0xB3},
    {"szlig",  0xDF},
    {"thorn",  0xFE},
    {"times",  0xD7},
    {"uacute", 0xFA},
    {"ucirc",  0xFB},
    {"ugrave", 0xF9},
    {"uml",    0xA8},
    {"uuml",   0xFC},
    {"yacute", 0xFD},
    {"yen",    0xA5},
    {"yuml",   0xFF}
};

static inline BOOL navigation_visible(HHInfo *info)
{
    return ((info->WinType.fsWinProperties & HHWIN_PROP_TRI_PANE) && !info->WinType.fNotExpanded);
}

/* Loads a string from the resource file */
static LPWSTR HH_LoadString(DWORD dwID)
{
    LPWSTR string = NULL;
    LPCWSTR stringresource;
    int iSize;

    iSize = LoadStringW(hhctrl_hinstance, dwID, (LPWSTR)&stringresource, 0);

    string = heap_alloc((iSize + 2) * sizeof(WCHAR)); /* some strings (tab text) needs double-null termination */
    memcpy(string, stringresource, iSize*sizeof(WCHAR));
    string[iSize] = 0;

    return string;
}

static HRESULT navigate_url(HHInfo *info, LPCWSTR surl)
{
    VARIANT url;
    HRESULT hres;

    TRACE("%s\n", debugstr_w(surl));

    V_VT(&url) = VT_BSTR;
    V_BSTR(&url) = SysAllocString(surl);

    hres = IWebBrowser2_Navigate2(info->web_browser->web_browser, &url, 0, 0, 0, 0);

    VariantClear(&url);

    if(FAILED(hres))
        TRACE("Navigation failed: %08x\n", hres);

    return hres;
}

BOOL NavigateToUrl(HHInfo *info, LPCWSTR surl)
{
    ChmPath chm_path;
    BOOL ret;
    HRESULT hres;

    static const WCHAR url_indicator[] = {':', '/', '/', 0};

    TRACE("%s\n", debugstr_w(surl));

    if (strstrW(surl, url_indicator)) {
        hres = navigate_url(info, surl);
        if(SUCCEEDED(hres))
            return TRUE;
    } /* look up in chm if it doesn't look like a full url */

    SetChmPath(&chm_path, info->pCHMInfo->szFile, surl);
    ret = NavigateToChm(info, chm_path.chm_file, chm_path.chm_index);

    heap_free(chm_path.chm_file);
    heap_free(chm_path.chm_index);

    return ret;
}

static BOOL AppendFullPathURL(LPCWSTR file, LPWSTR buf, LPCWSTR index)
{
    static const WCHAR url_format[] =
        {'m','k',':','@','M','S','I','T','S','t','o','r','e',':','%','s',':',':','%','s','%','s',0};
    static const WCHAR slash[] = {'/',0};
    static const WCHAR empty[] = {0};
    WCHAR full_path[MAX_PATH];

    TRACE("%s %p %s\n", debugstr_w(file), buf, debugstr_w(index));

    if(!GetFullPathNameW(file, sizeof(full_path)/sizeof(full_path[0]), full_path, NULL)) {
        WARN("GetFullPathName failed: %u\n", GetLastError());
        return FALSE;
    }

    wsprintfW(buf, url_format, full_path, (!index || index[0] == '/') ? empty : slash, index);
    return TRUE;
}

BOOL NavigateToChm(HHInfo *info, LPCWSTR file, LPCWSTR index)
{
    WCHAR buf[INTERNET_MAX_URL_LENGTH];

    TRACE("%p %s %s\n", info, debugstr_w(file), debugstr_w(index));

    if ((!info->web_browser) || !AppendFullPathURL(file, buf, index))
        return FALSE;

    return SUCCEEDED(navigate_url(info, buf));
}

static void DoSync(HHInfo *info)
{
    WCHAR buf[INTERNET_MAX_URL_LENGTH];
    HRESULT hres;
    BSTR url;

    hres = IWebBrowser2_get_LocationURL(info->web_browser->web_browser, &url);

    if (FAILED(hres))
    {
        WARN("get_LocationURL failed: %08x\n", hres);
        return;
    }

    /* If we're not currently viewing a page in the active .chm file, abort */
    if ((!AppendFullPathURL(info->WinType.pszFile, buf, NULL)) || (lstrlenW(buf) > lstrlenW(url)))
    {
        SysFreeString(url);
        return;
    }

    if (lstrcmpiW(buf, url) > 0)
    {
        static const WCHAR delimW[] = {':',':','/',0};
        const WCHAR *index;

        index = strstrW(url, delimW);

        if (index)
            ActivateContentTopic(info->tabs[TAB_CONTENTS].hwnd, index + 3, info->content); /* skip over ::/ */
    }

    SysFreeString(url);
}

/* Size Bar */

#define SIZEBAR_WIDTH   4

static const WCHAR szSizeBarClass[] = {
    'H','H',' ','S','i','z','e','B','a','r',0
};

/* Draw the SizeBar */
static void SB_OnPaint(HWND hWnd)
{
    PAINTSTRUCT ps;
    HDC hdc;
    RECT rc;
    
    hdc = BeginPaint(hWnd, &ps);

    GetClientRect(hWnd, &rc);

    /* dark frame */
    rc.right += 1;
    rc.bottom -= 1;
    FrameRect(hdc, &rc, GetStockObject(GRAY_BRUSH));

    /* white highlight */
    SelectObject(hdc, GetStockObject(WHITE_PEN));
    MoveToEx(hdc, rc.right, 1, NULL);
    LineTo(hdc, 1, 1);
    LineTo(hdc, 1, rc.bottom - 1);

    
    MoveToEx(hdc, 0, rc.bottom, NULL);
    LineTo(hdc, rc.right, rc.bottom);

    EndPaint(hWnd, &ps);
}

static void SB_OnLButtonDown(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    SetCapture(hWnd);
}

static void SB_OnLButtonUp(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    HHInfo *pHHInfo = (HHInfo *)GetWindowLongPtrW(hWnd, 0);
    POINT pt;

    pt.x = (short)LOWORD(lParam);
    pt.y = (short)HIWORD(lParam);

    /* update the window sizes */
    pHHInfo->WinType.iNavWidth += pt.x;
    Help_OnSize(hWnd);

    ReleaseCapture();
}

static void SB_OnMouseMove(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    /* ignore WM_MOUSEMOVE if not dragging the SizeBar */
    if (!(wParam & MK_LBUTTON))
        return;
}

static LRESULT CALLBACK SizeBar_WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        case WM_LBUTTONDOWN:
            SB_OnLButtonDown(hWnd, wParam, lParam);
            break;
        case WM_LBUTTONUP:
            SB_OnLButtonUp(hWnd, wParam, lParam);
            break;
        case WM_MOUSEMOVE:
            SB_OnMouseMove(hWnd, wParam, lParam);
            break;
        case WM_PAINT:
            SB_OnPaint(hWnd);
            break;
        default:
            return DefWindowProcW(hWnd, message, wParam, lParam);
    }

    return 0;
}

static void HH_RegisterSizeBarClass(HHInfo *pHHInfo)
{
    WNDCLASSEXW wcex;

    wcex.cbSize         = sizeof(WNDCLASSEXW);
    wcex.style          = 0;
    wcex.lpfnWndProc    = SizeBar_WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = sizeof(LONG_PTR);
    wcex.hInstance      = hhctrl_hinstance;
    wcex.hIcon          = LoadIconW(NULL, (LPCWSTR)IDI_APPLICATION);
    wcex.hCursor        = LoadCursorW(NULL, (LPCWSTR)IDC_SIZEWE);
    wcex.hbrBackground  = (HBRUSH)(COLOR_MENU + 1);
    wcex.lpszMenuName   = NULL;
    wcex.lpszClassName  = szSizeBarClass;
    wcex.hIconSm        = LoadIconW(NULL, (LPCWSTR)IDI_APPLICATION);

    RegisterClassExW(&wcex);
}

static void SB_GetSizeBarRect(HHInfo *info, RECT *rc)
{
    RECT rectWND, rectTB, rectNP;

    GetClientRect(info->WinType.hwndHelp, &rectWND);
    GetClientRect(info->WinType.hwndToolBar, &rectTB);
    GetClientRect(info->WinType.hwndNavigation, &rectNP);

    rc->left = rectNP.right;
    rc->top = rectTB.bottom;
    rc->bottom = rectWND.bottom - rectTB.bottom;
    rc->right = SIZEBAR_WIDTH;
}

static BOOL HH_AddSizeBar(HHInfo *pHHInfo)
{
    HWND hWnd;
    HWND hwndParent = pHHInfo->WinType.hwndHelp;
    DWORD dwStyles = WS_CHILDWINDOW | WS_OVERLAPPED;
    DWORD dwExStyles = WS_EX_LEFT | WS_EX_LTRREADING | WS_EX_RIGHTSCROLLBAR;
    RECT rc;

    if (navigation_visible(pHHInfo))
        dwStyles |= WS_VISIBLE;

    SB_GetSizeBarRect(pHHInfo, &rc);

    hWnd = CreateWindowExW(dwExStyles, szSizeBarClass, szEmpty, dwStyles,
                           rc.left, rc.top, rc.right, rc.bottom,
                           hwndParent, NULL, hhctrl_hinstance, NULL);
    if (!hWnd)
        return FALSE;

    /* store the pointer to the HH info struct */
    SetWindowLongPtrW(hWnd, 0, (LONG_PTR)pHHInfo);

    pHHInfo->hwndSizeBar = hWnd;
    return TRUE;
}

/* Child Window */

static const WCHAR szChildClass[] = {
    'H','H',' ','C','h','i','l','d',0
};

static LRESULT Child_OnPaint(HWND hWnd)
{
    PAINTSTRUCT ps;
    HDC hdc;
    RECT rc;

    hdc = BeginPaint(hWnd, &ps);

    /* Only paint the Navigation pane, identified by the fact
     * that it has a child window
     */
    if (GetWindow(hWnd, GW_CHILD))
    {
        GetClientRect(hWnd, &rc);

        /* set the border color */
        SelectObject(hdc, GetStockObject(DC_PEN));
        SetDCPenColor(hdc, GetSysColor(COLOR_BTNSHADOW));

        /* Draw the top border */
        LineTo(hdc, rc.right, 0);

        SelectObject(hdc, GetStockObject(WHITE_PEN));
        MoveToEx(hdc, 0, 1, NULL);
        LineTo(hdc, rc.right, 1);
    }

    EndPaint(hWnd, &ps);

    return 0;
}

static void ResizeTabChild(HHInfo *info, int tab)
{
    HWND hwnd = info->tabs[tab].hwnd;
    INT width, height;
    RECT rect, tabrc;
    DWORD cnt;

    GetClientRect(info->WinType.hwndNavigation, &rect);
    SendMessageW(info->hwndTabCtrl, TCM_GETITEMRECT, 0, (LPARAM)&tabrc);
    cnt = SendMessageW(info->hwndTabCtrl, TCM_GETROWCOUNT, 0, 0);

    rect.left = TAB_MARGIN;
    rect.top = TAB_TOP_PADDING + cnt*(tabrc.bottom-tabrc.top) + TAB_MARGIN;
    rect.right -= TAB_RIGHT_PADDING + TAB_MARGIN;
    rect.bottom -= TAB_MARGIN;
    width = rect.right-rect.left;
    height = rect.bottom-rect.top;

    SetWindowPos(hwnd, NULL, rect.left, rect.top, width, height,
                 SWP_NOZORDER | SWP_NOACTIVATE);

    switch (tab)
    {
    case TAB_INDEX: {
        int scroll_width = GetSystemMetrics(SM_CXVSCROLL);
        int border_width = GetSystemMetrics(SM_CXBORDER);
        int edge_width = GetSystemMetrics(SM_CXEDGE);

        /* Resize the tab widget column to perfectly fit the tab window and
         * leave sufficient space for the scroll widget.
         */
        SendMessageW(info->tabs[TAB_INDEX].hwnd, LVM_SETCOLUMNWIDTH, 0,
                     width-scroll_width-2*border_width-2*edge_width);

        break;
    }
    case TAB_SEARCH: {
        int scroll_width = GetSystemMetrics(SM_CXVSCROLL);
        int border_width = GetSystemMetrics(SM_CXBORDER);
        int edge_width = GetSystemMetrics(SM_CXEDGE);
        int top_pos = 0;

        SetWindowPos(info->search.hwndEdit, NULL, 0, top_pos, width,
                      EDIT_HEIGHT, SWP_NOZORDER | SWP_NOACTIVATE);
        top_pos += EDIT_HEIGHT + TAB_MARGIN;
        SetWindowPos(info->search.hwndList, NULL, 0, top_pos, width,
                      height-top_pos, SWP_NOZORDER | SWP_NOACTIVATE);
        /* Resize the tab widget column to perfectly fit the tab window and
         * leave sufficient space for the scroll widget.
         */
        SendMessageW(info->search.hwndList, LVM_SETCOLUMNWIDTH, 0,
                     width-scroll_width-2*border_width-2*edge_width);

        break;
    }
    }
}

static LRESULT Child_OnSize(HWND hwnd)
{
    HHInfo *info = (HHInfo*)GetWindowLongPtrW(hwnd, 0);
    RECT rect;

    if(!info || hwnd != info->WinType.hwndNavigation)
        return 0;

    GetClientRect(hwnd, &rect);
    SetWindowPos(info->hwndTabCtrl, HWND_TOP, 0, 0,
                 rect.right - TAB_RIGHT_PADDING,
                 rect.bottom - TAB_TOP_PADDING, SWP_NOMOVE);

    ResizeTabChild(info, TAB_CONTENTS);
    ResizeTabChild(info, TAB_INDEX);
    ResizeTabChild(info, TAB_SEARCH);
    return 0;
}

static LRESULT OnTabChange(HWND hwnd)
{
    HHInfo *info = (HHInfo*)GetWindowLongPtrW(hwnd, 0);
    int tab_id, tab_index, i;

    TRACE("%p\n", hwnd);

    if (!info)
        return 0;

    if(info->tabs[info->current_tab].hwnd)
        ShowWindow(info->tabs[info->current_tab].hwnd, SW_HIDE);

    tab_id = (int) SendMessageW(info->hwndTabCtrl, TCM_GETCURSEL, 0, 0);
    /* convert the ID of the tab to an index in our tab list */
    tab_index = -1;
    for (i=0; i<TAB_NUMTABS; i++)
    {
        if (info->tabs[i].id == tab_id)
        {
            tab_index = i;
            break;
        }
    }
    if (tab_index == -1)
    {
        FIXME("Tab ID %d does not correspond to a valid index in the tab list.\n", tab_id);
        return 0;
    }
    info->current_tab = tab_index;

    if(info->tabs[info->current_tab].hwnd)
        ShowWindow(info->tabs[info->current_tab].hwnd, SW_SHOW);

    return 0;
}

static LRESULT OnTopicChange(HHInfo *info, void *user_data)
{
    LPCWSTR chmfile = NULL, name = NULL, local = NULL;
    ContentItem *citer;
    SearchItem *siter;
    IndexItem *iiter;

    if(!user_data || !info)
        return 0;

    switch (info->current_tab)
    {
    case TAB_CONTENTS:
        citer = (ContentItem *) user_data;
        name = citer->name;
        local = citer->local;
        while(citer) {
            if(citer->merge.chm_file) {
                chmfile = citer->merge.chm_file;
                break;
            }
            citer = citer->parent;
        }
        break;
    case TAB_INDEX:
        iiter = (IndexItem *) user_data;
        if(iiter->nItems == 0) {
            FIXME("No entries for this item!\n");
            return 0;
        }
        if(iiter->nItems > 1) {
            int i = 0;
            LVITEMW lvi;

            SendMessageW(info->popup.hwndList, LVM_DELETEALLITEMS, 0, 0);
            for(i=0;i<iiter->nItems;i++) {
                IndexSubItem *item = &iiter->items[i];
                WCHAR *name = iiter->keyword;

                if(!item->name)
                    item->name = GetDocumentTitle(info->pCHMInfo, item->local);
                if(item->name)
                    name = item->name;
                memset(&lvi, 0, sizeof(lvi));
                lvi.iItem = i;
                lvi.mask = LVIF_TEXT|LVIF_PARAM;
                lvi.cchTextMax = strlenW(name)+1;
                lvi.pszText = name;
                lvi.lParam = (LPARAM) item;
                SendMessageW(info->popup.hwndList, LVM_INSERTITEMW, 0, (LPARAM)&lvi);
            }
            ShowWindow(info->popup.hwndPopup, SW_SHOW);
            return 0;
        }
        name = iiter->items[0].name;
        local = iiter->items[0].local;
        chmfile = iiter->merge.chm_file;
        break;
    case TAB_SEARCH:
        siter = (SearchItem *) user_data;
        name = siter->filename;
        local = siter->filename;
        chmfile = info->pCHMInfo->szFile;
        break;
    default:
        FIXME("Unhandled operation for this tab!\n");
        return 0;
    }

    if(!chmfile)
    {
        FIXME("No help file found for this item!\n");
        return 0;
    }

    TRACE("name %s loal %s\n", debugstr_w(name), debugstr_w(local));

    NavigateToChm(info, chmfile, local);
    return 0;
}

/* Capture the Enter/Return key and send it up to Child_WndProc as an NM_RETURN message */
static LRESULT CALLBACK EditChild_WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    WNDPROC editWndProc = (WNDPROC)GetWindowLongPtrW(hWnd, GWLP_USERDATA);

    if(message == WM_KEYUP && wParam == VK_RETURN)
    {
        NMHDR nmhdr;

        nmhdr.hwndFrom = hWnd;
        nmhdr.code = NM_RETURN;
        SendMessageW(GetParent(GetParent(hWnd)), WM_NOTIFY, wParam, (LPARAM)&nmhdr);
    }
    return editWndProc(hWnd, message, wParam, lParam);
}

static LRESULT CALLBACK Child_WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_PAINT:
        return Child_OnPaint(hWnd);
    case WM_SIZE:
        return Child_OnSize(hWnd);
    case WM_NOTIFY: {
        HHInfo *info = (HHInfo*)GetWindowLongPtrW(hWnd, 0);
        NMHDR *nmhdr = (NMHDR*)lParam;

        switch(nmhdr->code) {
        case TCN_SELCHANGE:
            return OnTabChange(hWnd);
        case TVN_SELCHANGEDW:
            return OnTopicChange(info, (void*)((NMTREEVIEWW *)lParam)->itemNew.lParam);
        case TVN_ITEMEXPANDINGW: {
            TVITEMW *item = &((NMTREEVIEWW *)lParam)->itemNew;
            HWND hwndTreeView = info->tabs[TAB_CONTENTS].hwnd;

            item->mask = TVIF_IMAGE|TVIF_SELECTEDIMAGE;
            if (item->state & TVIS_EXPANDED)
            {
                item->iImage = HHTV_FOLDER;
                item->iSelectedImage = HHTV_FOLDER;
            }
            else
            {
                item->iImage = HHTV_OPENFOLDER;
                item->iSelectedImage = HHTV_OPENFOLDER;
            }
            SendMessageW(hwndTreeView, TVM_SETITEMW, 0, (LPARAM)item);
            return 0;
        }
        case NM_DBLCLK:
            if(!info)
                return 0;
            switch(info->current_tab)
            {
            case TAB_INDEX:
                return OnTopicChange(info, (void*)((NMITEMACTIVATE *)lParam)->lParam);
            case TAB_SEARCH:
                return OnTopicChange(info, (void*)((NMITEMACTIVATE *)lParam)->lParam);
            }
            break;
        case NM_RETURN:
            if(!info)
                return 0;
            switch(info->current_tab) {
            case TAB_INDEX: {
                HWND hwndList = info->tabs[TAB_INDEX].hwnd;
                LVITEMW lvItem;

                lvItem.iItem = (int) SendMessageW(hwndList, LVM_GETSELECTIONMARK, 0, 0);
                lvItem.mask = TVIF_PARAM;
                SendMessageW(hwndList, LVM_GETITEMW, 0, (LPARAM)&lvItem);
                OnTopicChange(info, (void*) lvItem.lParam);
                return 0;
            }
            case TAB_SEARCH: {
                if(nmhdr->hwndFrom == info->search.hwndEdit) {
                    char needle[100];
                    DWORD i, len;

                    len = GetWindowTextA(info->search.hwndEdit, needle, sizeof(needle));
                    if(!len)
                    {
                        FIXME("Unable to get search text.\n");
                        return 0;
                    }
                    /* Convert the requested text for comparison later against the
                     * lower case version of HTML file contents.
                     */
                    for(i=0;i<len;i++)
                        needle[i] = tolower(needle[i]);
                    InitSearch(info, needle);
                    return 0;
                }else if(nmhdr->hwndFrom == info->search.hwndList) {
                    HWND hwndList = info->search.hwndList;
                    LVITEMW lvItem;

                    lvItem.iItem = (int) SendMessageW(hwndList, LVM_GETSELECTIONMARK, 0, 0);
                    lvItem.mask = TVIF_PARAM;
                    SendMessageW(hwndList, LVM_GETITEMW, 0, (LPARAM)&lvItem);
                    OnTopicChange(info, (void*) lvItem.lParam);
                    return 0;
                }
                break;
            }
            }
            break;
        }
        break;
    }
    default:
        return DefWindowProcW(hWnd, message, wParam, lParam);
    }

    return 0;
}

static void HH_RegisterChildWndClass(HHInfo *pHHInfo)
{
    WNDCLASSEXW wcex;

    wcex.cbSize         = sizeof(WNDCLASSEXW);
    wcex.style          = 0;
    wcex.lpfnWndProc    = Child_WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = sizeof(LONG_PTR);
    wcex.hInstance      = hhctrl_hinstance;
    wcex.hIcon          = LoadIconW(NULL, (LPCWSTR)IDI_APPLICATION);
    wcex.hCursor        = LoadCursorW(NULL, (LPCWSTR)IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_BTNFACE + 1);
    wcex.lpszMenuName   = NULL;
    wcex.lpszClassName  = szChildClass;
    wcex.hIconSm        = LoadIconW(NULL, (LPCWSTR)IDI_APPLICATION);

    RegisterClassExW(&wcex);
}

/* Toolbar */

#define ICON_SIZE   20

static void DisplayPopupMenu(HHInfo *info)
{
    HMENU menu, submenu;
    TBBUTTONINFOW button;
    MENUITEMINFOW item;
    POINT coords;
    RECT rect;
    DWORD index;

    menu = LoadMenuW(hhctrl_hinstance, MAKEINTRESOURCEW(MENU_POPUP));

    if (!menu)
        return;

    submenu = GetSubMenu(menu, 0);

    /* Update the Show/Hide menu item */
    item.cbSize = sizeof(MENUITEMINFOW);
    item.fMask = MIIM_FTYPE | MIIM_STATE | MIIM_STRING;
    item.fType = MFT_STRING;
    item.fState = MF_ENABLED;

    if (info->WinType.fNotExpanded)
        item.dwTypeData = HH_LoadString(IDS_SHOWTABS);
    else
        item.dwTypeData = HH_LoadString(IDS_HIDETABS);

    SetMenuItemInfoW(submenu, IDTB_EXPAND, FALSE, &item);
    heap_free(item.dwTypeData);

    /* Find the index toolbar button */
    button.cbSize = sizeof(TBBUTTONINFOW);
    button.dwMask = TBIF_COMMAND;
    index = SendMessageW(info->WinType.hwndToolBar, TB_GETBUTTONINFOW, IDTB_OPTIONS, (LPARAM) &button);

    if (index == -1)
       return;

    /* Get position */
    SendMessageW(info->WinType.hwndToolBar, TB_GETITEMRECT, index, (LPARAM) &rect);

    coords.x = rect.left;
    coords.y = rect.bottom;

    ClientToScreen(info->WinType.hwndToolBar, &coords);
    TrackPopupMenu(submenu, TPM_LEFTALIGN | TPM_TOPALIGN | TPM_LEFTBUTTON | TPM_NOANIMATION, coords.x, coords.y, 0, info->WinType.hwndHelp, NULL);
}

static void TB_OnClick(HWND hWnd, DWORD dwID)
{
    HHInfo *info = (HHInfo *)GetWindowLongPtrW(hWnd, 0);

    switch (dwID)
    {
        case IDTB_STOP:
            DoPageAction(info->web_browser, WB_STOP);
            break;
        case IDTB_REFRESH:
            DoPageAction(info->web_browser, WB_REFRESH);
            break;
        case IDTB_BACK:
            DoPageAction(info->web_browser, WB_GOBACK);
            break;
        case IDTB_HOME:
            NavigateToChm(info, info->pCHMInfo->szFile, info->WinType.pszHome);
            break;
        case IDTB_FORWARD:
            DoPageAction(info->web_browser, WB_GOFORWARD);
            break;
        case IDTB_PRINT:
            DoPageAction(info->web_browser, WB_PRINT);
            break;
        case IDTB_EXPAND:
        case IDTB_CONTRACT:
            ExpandContract(info);
            break;
        case IDTB_SYNC:
            DoSync(info);
            break;
        case IDTB_OPTIONS:
            DisplayPopupMenu(info);
            break;
        case IDTB_NOTES:
        case IDTB_CONTENTS:
        case IDTB_INDEX:
        case IDTB_SEARCH:
        case IDTB_HISTORY:
        case IDTB_FAVORITES:
            /* These are officially unimplemented as of the Windows 7 SDK */
            break;
        case IDTB_BROWSE_FWD:
        case IDTB_BROWSE_BACK:
        case IDTB_JUMP1:
        case IDTB_JUMP2:
        case IDTB_CUSTOMIZE:
        case IDTB_ZOOM:
        case IDTB_TOC_NEXT:
        case IDTB_TOC_PREV:
            break;
    }
}

static void TB_AddButton(TBBUTTON *pButtons, DWORD dwIndex, DWORD dwID, DWORD dwBitmap)
{
    pButtons[dwIndex].iBitmap = dwBitmap;
    pButtons[dwIndex].idCommand = dwID;
    pButtons[dwIndex].fsState = TBSTATE_ENABLED;
    pButtons[dwIndex].fsStyle = BTNS_BUTTON;
    pButtons[dwIndex].dwData = 0;
    pButtons[dwIndex].iString = 0;
}

static void TB_AddButtonsFromFlags(HHInfo *pHHInfo, TBBUTTON *pButtons, DWORD dwButtonFlags, LPDWORD pdwNumButtons)
{
    int nHistBitmaps = 0, nStdBitmaps = 0, nHHBitmaps = 0;
    HWND hToolbar = pHHInfo->WinType.hwndToolBar;
    TBADDBITMAP tbAB;
    DWORD unsupported;

    /* Common bitmaps */
    tbAB.hInst = HINST_COMMCTRL;
    tbAB.nID = IDB_HIST_LARGE_COLOR;
    nHistBitmaps = SendMessageW(hToolbar, TB_ADDBITMAP, 0, (LPARAM)&tbAB);
    tbAB.nID = IDB_STD_LARGE_COLOR;
    nStdBitmaps = SendMessageW(hToolbar, TB_ADDBITMAP, 0, (LPARAM)&tbAB);
    /* hhctrl.ocx bitmaps */
    tbAB.hInst = hhctrl_hinstance;
    tbAB.nID = IDB_HHTOOLBAR;
    nHHBitmaps = SendMessageW(hToolbar, TB_ADDBITMAP, HHTB_NUMBITMAPS, (LPARAM)&tbAB);

    *pdwNumButtons = 0;

    unsupported = dwButtonFlags & (HHWIN_BUTTON_BROWSE_FWD |
        HHWIN_BUTTON_BROWSE_BCK | HHWIN_BUTTON_NOTES | HHWIN_BUTTON_CONTENTS |
        HHWIN_BUTTON_INDEX | HHWIN_BUTTON_SEARCH | HHWIN_BUTTON_HISTORY |
        HHWIN_BUTTON_FAVORITES | HHWIN_BUTTON_JUMP1 | HHWIN_BUTTON_JUMP2 |
        HHWIN_BUTTON_ZOOM | HHWIN_BUTTON_TOC_NEXT | HHWIN_BUTTON_TOC_PREV);
    if (unsupported)
        FIXME("got asked for unsupported buttons: %06x\n", unsupported);

    if (dwButtonFlags & HHWIN_BUTTON_EXPAND)
    {
        TB_AddButton(pButtons, (*pdwNumButtons)++, IDTB_EXPAND, nHHBitmaps + HHTB_EXPAND);
        TB_AddButton(pButtons, (*pdwNumButtons)++, IDTB_CONTRACT, nHHBitmaps + HHTB_CONTRACT);

        if (pHHInfo->WinType.fNotExpanded)
            pButtons[1].fsState |= TBSTATE_HIDDEN;
        else
            pButtons[0].fsState |= TBSTATE_HIDDEN;
    }

    if (dwButtonFlags & HHWIN_BUTTON_BACK)
        TB_AddButton(pButtons, (*pdwNumButtons)++, IDTB_BACK, nHistBitmaps + HIST_BACK);

    if (dwButtonFlags & HHWIN_BUTTON_FORWARD)
        TB_AddButton(pButtons, (*pdwNumButtons)++, IDTB_FORWARD, nHistBitmaps + HIST_FORWARD);

    if (dwButtonFlags & HHWIN_BUTTON_STOP)
        TB_AddButton(pButtons, (*pdwNumButtons)++, IDTB_STOP, nHHBitmaps + HHTB_STOP);

    if (dwButtonFlags & HHWIN_BUTTON_REFRESH)
        TB_AddButton(pButtons, (*pdwNumButtons)++, IDTB_REFRESH, nHHBitmaps + HHTB_REFRESH);

    if (dwButtonFlags & HHWIN_BUTTON_HOME)
        TB_AddButton(pButtons, (*pdwNumButtons)++, IDTB_HOME, nHHBitmaps + HHTB_HOME);

    if (dwButtonFlags & HHWIN_BUTTON_SYNC)
        TB_AddButton(pButtons, (*pdwNumButtons)++, IDTB_SYNC, nHHBitmaps + HHTB_SYNC);

    if (dwButtonFlags & HHWIN_BUTTON_OPTIONS)
        TB_AddButton(pButtons, (*pdwNumButtons)++, IDTB_OPTIONS, nStdBitmaps + STD_PROPERTIES);

    if (dwButtonFlags & HHWIN_BUTTON_PRINT)
        TB_AddButton(pButtons, (*pdwNumButtons)++, IDTB_PRINT, nStdBitmaps + STD_PRINT);
}

static BOOL HH_AddToolbar(HHInfo *pHHInfo)
{
    HWND hToolbar;
    HWND hwndParent = pHHInfo->WinType.hwndHelp;
    DWORD toolbarFlags;
    TBBUTTON buttons[IDTB_TOC_PREV - IDTB_EXPAND];
    DWORD dwStyles, dwExStyles;
    DWORD dwNumButtons, dwIndex;

    if (pHHInfo->WinType.fsWinProperties & HHWIN_PARAM_TB_FLAGS)
        toolbarFlags = pHHInfo->WinType.fsToolBarFlags;
    else
        toolbarFlags = HHWIN_DEF_BUTTONS;

    dwStyles = WS_CHILDWINDOW | TBSTYLE_FLAT | TBSTYLE_WRAPABLE | TBSTYLE_TOOLTIPS | CCS_NODIVIDER;
    dwExStyles = WS_EX_LEFT | WS_EX_LTRREADING | WS_EX_RIGHTSCROLLBAR;

    hToolbar = CreateWindowExW(dwExStyles, TOOLBARCLASSNAMEW, NULL, dwStyles,
                               0, 0, 0, 0, hwndParent, NULL,
                               hhctrl_hinstance, NULL);
    if (!hToolbar)
        return FALSE;
    pHHInfo->WinType.hwndToolBar = hToolbar;

    SendMessageW(hToolbar, TB_SETBITMAPSIZE, 0, MAKELONG(ICON_SIZE, ICON_SIZE));
    SendMessageW(hToolbar, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);
    SendMessageW(hToolbar, WM_SETFONT, (WPARAM)pHHInfo->hFont, TRUE);

    TB_AddButtonsFromFlags(pHHInfo, buttons, toolbarFlags, &dwNumButtons);

    for (dwIndex = 0; dwIndex < dwNumButtons; dwIndex++)
    {
        LPWSTR szBuf = HH_LoadString(buttons[dwIndex].idCommand);
        DWORD dwLen = strlenW(szBuf);
        szBuf[dwLen + 1] = 0; /* Double-null terminate */

        buttons[dwIndex].iString = (DWORD)SendMessageW(hToolbar, TB_ADDSTRINGW, 0, (LPARAM)szBuf);
        heap_free(szBuf);
    }

    SendMessageW(hToolbar, TB_ADDBUTTONSW, dwNumButtons, (LPARAM)buttons);
    SendMessageW(hToolbar, TB_AUTOSIZE, 0, 0);
    if (pHHInfo->WinType.fsWinProperties & HHWIN_PROP_TRI_PANE)
        ShowWindow(hToolbar, SW_SHOW);

    return TRUE;
}

/* Navigation Pane */

static void NP_GetNavigationRect(HHInfo *pHHInfo, RECT *rc)
{
    HWND hwndParent = pHHInfo->WinType.hwndHelp;
    HWND hwndToolbar = pHHInfo->WinType.hwndToolBar;
    RECT rectWND, rectTB;

    GetClientRect(hwndParent, &rectWND);
    GetClientRect(hwndToolbar, &rectTB);

    rc->left = 0;
    rc->top = rectTB.bottom;
    rc->bottom = rectWND.bottom - rectTB.bottom;

    if (!(pHHInfo->WinType.fsValidMembers & HHWIN_PARAM_NAV_WIDTH) &&
          pHHInfo->WinType.iNavWidth == 0)
    {
        pHHInfo->WinType.iNavWidth = WINTYPE_DEFAULT_NAVWIDTH;
    }

    rc->right = pHHInfo->WinType.iNavWidth;
}

static DWORD NP_CreateTab(HINSTANCE hInstance, HWND hwndTabCtrl, DWORD index)
{
    TCITEMW tie;
    LPWSTR tabText = HH_LoadString(index);
    DWORD ret;

    tie.mask = TCIF_TEXT;
    tie.pszText = tabText;

    ret = SendMessageW( hwndTabCtrl, TCM_INSERTITEMW, index, (LPARAM)&tie );

    heap_free(tabText);
    return ret;
}

static BOOL HH_AddNavigationPane(HHInfo *info)
{
    HWND hWnd, hwndTabCtrl;
    HWND hwndParent = info->WinType.hwndHelp;
    DWORD dwStyles = WS_CHILDWINDOW;
    DWORD dwExStyles = WS_EX_LEFT | WS_EX_LTRREADING | WS_EX_RIGHTSCROLLBAR;
    RECT rc;

    if (navigation_visible(info))
        dwStyles |= WS_VISIBLE;

    NP_GetNavigationRect(info, &rc);

    hWnd = CreateWindowExW(dwExStyles, szChildClass, szEmpty, dwStyles,
                           rc.left, rc.top, rc.right, rc.bottom,
                           hwndParent, NULL, hhctrl_hinstance, NULL);
    if (!hWnd)
        return FALSE;

    SetWindowLongPtrW(hWnd, 0, (LONG_PTR)info);

    hwndTabCtrl = CreateWindowExW(dwExStyles, WC_TABCONTROLW, szEmpty, dwStyles | WS_VISIBLE,
                                  0, TAB_TOP_PADDING,
                                  rc.right - TAB_RIGHT_PADDING,
                                  rc.bottom - TAB_TOP_PADDING,
                                  hWnd, NULL, hhctrl_hinstance, NULL);
    if (!hwndTabCtrl)
        return FALSE;

    if (*info->WinType.pszToc)
        info->tabs[TAB_CONTENTS].id = NP_CreateTab(hhctrl_hinstance, hwndTabCtrl, IDS_CONTENTS);

    if (*info->WinType.pszIndex)
        info->tabs[TAB_INDEX].id = NP_CreateTab(hhctrl_hinstance, hwndTabCtrl, IDS_INDEX);

    if (info->WinType.fsWinProperties & HHWIN_PROP_TAB_SEARCH)
        info->tabs[TAB_SEARCH].id = NP_CreateTab(hhctrl_hinstance, hwndTabCtrl, IDS_SEARCH);

    if (info->WinType.fsWinProperties & HHWIN_PROP_TAB_FAVORITES)
        info->tabs[TAB_FAVORITES].id = NP_CreateTab(hhctrl_hinstance, hwndTabCtrl, IDS_FAVORITES);

    SendMessageW(hwndTabCtrl, WM_SETFONT, (WPARAM)info->hFont, TRUE);

    info->hwndTabCtrl = hwndTabCtrl;
    info->WinType.hwndNavigation = hWnd;
    return TRUE;
}

/* HTML Pane */

static void HP_GetHTMLRect(HHInfo *info, RECT *rc)
{
    RECT rectTB, rectWND, rectNP, rectSB;

    GetClientRect(info->WinType.hwndHelp, &rectWND);
    GetClientRect(info->hwndSizeBar, &rectSB);

    rc->left = 0;
    rc->top = 0;
    if (navigation_visible(info))
    {
        GetClientRect(info->WinType.hwndNavigation, &rectNP);
        rc->left += rectNP.right + rectSB.right;
    }
    if (info->WinType.fsWinProperties & HHWIN_PROP_TRI_PANE)
    {
        GetClientRect(info->WinType.hwndToolBar, &rectTB);
        rc->top += rectTB.bottom;
    }
    rc->right = rectWND.right - rc->left;
    rc->bottom = rectWND.bottom - rc->top;
}

static BOOL HH_AddHTMLPane(HHInfo *pHHInfo)
{
    HWND hWnd;
    HWND hwndParent = pHHInfo->WinType.hwndHelp;
    DWORD dwStyles = WS_CHILDWINDOW | WS_VISIBLE | WS_CLIPCHILDREN;
    DWORD dwExStyles = WS_EX_LEFT | WS_EX_LTRREADING | WS_EX_RIGHTSCROLLBAR | WS_EX_CLIENTEDGE;
    RECT rc;

    HP_GetHTMLRect(pHHInfo, &rc);

    hWnd = CreateWindowExW(dwExStyles, szChildClass, szEmpty, dwStyles,
                           rc.left, rc.top, rc.right, rc.bottom,
                           hwndParent, NULL, hhctrl_hinstance, NULL);
    if (!hWnd)
        return FALSE;

    if (!InitWebBrowser(pHHInfo, hWnd))
        return FALSE;

    /* store the pointer to the HH info struct */
    SetWindowLongPtrW(hWnd, 0, (LONG_PTR)pHHInfo);

    ShowWindow(hWnd, SW_SHOW);
    UpdateWindow(hWnd);

    pHHInfo->WinType.hwndHTML = hWnd;
    return TRUE;
}

static BOOL AddContentTab(HHInfo *info)
{
    HIMAGELIST hImageList;
    HBITMAP hBitmap;
    HWND hWnd;

    if(info->tabs[TAB_CONTENTS].id == -1)
        return TRUE; /* No "Contents" tab */
    hWnd = CreateWindowExW(WS_EX_CLIENTEDGE, WC_TREEVIEWW, szEmpty, WS_CHILD | WS_BORDER | TVS_LINESATROOT
                           | TVS_SHOWSELALWAYS | TVS_HASBUTTONS, 50, 50, 100, 100,
                           info->WinType.hwndNavigation, NULL, hhctrl_hinstance, NULL);
    if(!hWnd) {
        ERR("Could not create treeview control\n");
        return FALSE;
    }

    hImageList = ImageList_Create(16, 16, ILC_COLOR32, 0, HHTV_NUMBITMAPS);
    hBitmap = LoadBitmapW(hhctrl_hinstance, MAKEINTRESOURCEW(IDB_HHTREEVIEW));
    ImageList_Add(hImageList, hBitmap, NULL);
    SendMessageW(hWnd, TVM_SETIMAGELIST, TVSIL_NORMAL, (LPARAM)hImageList);

    info->contents.hImageList = hImageList;
    info->tabs[TAB_CONTENTS].hwnd = hWnd;
    ResizeTabChild(info, TAB_CONTENTS);
    ShowWindow(hWnd, SW_SHOW);

    return TRUE;
}

static BOOL AddIndexTab(HHInfo *info)
{
    char hidden_column[] = "Column";
    LVCOLUMNA lvc;

    if(info->tabs[TAB_INDEX].id == -1)
        return TRUE; /* No "Index" tab */
    info->tabs[TAB_INDEX].hwnd = CreateWindowExW(WS_EX_CLIENTEDGE, WC_LISTVIEWW,
           szEmpty, WS_CHILD | WS_BORDER | LVS_SINGLESEL | LVS_REPORT | LVS_NOCOLUMNHEADER, 50, 50, 100, 100,
           info->WinType.hwndNavigation, NULL, hhctrl_hinstance, NULL);
    if(!info->tabs[TAB_INDEX].hwnd) {
        ERR("Could not create ListView control\n");
        return FALSE;
    }
    memset(&lvc, 0, sizeof(lvc));
    lvc.mask = LVCF_TEXT;
    lvc.pszText = hidden_column;
    if(SendMessageW(info->tabs[TAB_INDEX].hwnd, LVM_INSERTCOLUMNA, 0, (LPARAM) &lvc) == -1)
    {
        ERR("Could not create ListView column\n");
        return FALSE;
    }

    ResizeTabChild(info, TAB_INDEX);
    ShowWindow(info->tabs[TAB_INDEX].hwnd, SW_HIDE);

    return TRUE;
}

static BOOL AddSearchTab(HHInfo *info)
{
    HWND hwndList, hwndEdit, hwndContainer;
    char hidden_column[] = "Column";
    WNDPROC editWndProc;
    LVCOLUMNA lvc;

    if(info->tabs[TAB_SEARCH].id == -1)
        return TRUE; /* No "Search" tab */
    hwndContainer = CreateWindowExW(WS_EX_CONTROLPARENT, szChildClass, szEmpty,
                                    WS_CHILD, 0, 0, 0, 0, info->WinType.hwndNavigation,
                                    NULL, hhctrl_hinstance, NULL);
    if(!hwndContainer) {
        ERR("Could not create search window container control.\n");
        return FALSE;
    }
    hwndEdit = CreateWindowExW(WS_EX_CLIENTEDGE, WC_EDITW, szEmpty, WS_CHILD
                                | WS_VISIBLE | ES_LEFT | SS_NOTIFY, 0, 0, 0, 0,
                               hwndContainer, NULL, hhctrl_hinstance, NULL);
    if(!hwndEdit) {
        ERR("Could not create search ListView control.\n");
        return FALSE;
    }
    if(SendMessageW(hwndEdit, WM_SETFONT, (WPARAM) info->hFont, (LPARAM) FALSE) == -1)
    {
        ERR("Could not set font for edit control.\n");
        return FALSE;
    }
    editWndProc = (WNDPROC) SetWindowLongPtrW(hwndEdit, GWLP_WNDPROC, (LONG_PTR)EditChild_WndProc);
    if(!editWndProc) {
        ERR("Could not redirect messages for edit control.\n");
        return FALSE;
    }
    SetWindowLongPtrW(hwndEdit, GWLP_USERDATA, (LONG_PTR)editWndProc);
    hwndList = CreateWindowExW(WS_EX_CLIENTEDGE, WC_LISTVIEWW, szEmpty,
                               WS_CHILD | WS_VISIBLE | WS_BORDER | LVS_SINGLESEL
                                | LVS_REPORT | LVS_NOCOLUMNHEADER, 0, 0, 0, 0,
                               hwndContainer, NULL, hhctrl_hinstance, NULL);
    if(!hwndList) {
        ERR("Could not create search ListView control.\n");
        return FALSE;
    }
    memset(&lvc, 0, sizeof(lvc));
    lvc.mask = LVCF_TEXT;
    lvc.pszText = hidden_column;
    if(SendMessageW(hwndList, LVM_INSERTCOLUMNA, 0, (LPARAM) &lvc) == -1)
    {
        ERR("Could not create ListView column\n");
        return FALSE;
    }

    info->search.hwndEdit = hwndEdit;
    info->search.hwndList = hwndList;
    info->search.hwndContainer = hwndContainer;
    info->tabs[TAB_SEARCH].hwnd = hwndContainer;

    SetWindowLongPtrW(hwndContainer, 0, (LONG_PTR)info);

    ResizeTabChild(info, TAB_SEARCH);

    return TRUE;
}

/* The Index tab's sub-topic popup */

static void ResizePopupChild(HHInfo *info)
{
    int scroll_width = GetSystemMetrics(SM_CXVSCROLL);
    int border_width = GetSystemMetrics(SM_CXBORDER);
    int edge_width = GetSystemMetrics(SM_CXEDGE);
    INT width, height;
    RECT rect;

    if(!info)
        return;

    GetClientRect(info->popup.hwndPopup, &rect);
    SetWindowPos(info->popup.hwndCallback, HWND_TOP, 0, 0,
                 rect.right, rect.bottom, SWP_NOMOVE);

    rect.left = TAB_MARGIN;
    rect.top = TAB_TOP_PADDING + TAB_MARGIN;
    rect.right -= TAB_RIGHT_PADDING + TAB_MARGIN;
    rect.bottom -= TAB_MARGIN;
    width = rect.right-rect.left;
    height = rect.bottom-rect.top;

    SetWindowPos(info->popup.hwndList, NULL, rect.left, rect.top, width, height,
                 SWP_NOZORDER | SWP_NOACTIVATE);

    SendMessageW(info->popup.hwndList, LVM_SETCOLUMNWIDTH, 0,
                 width-scroll_width-2*border_width-2*edge_width);
}

static LRESULT CALLBACK HelpPopup_WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    HHInfo *info = (HHInfo *)GetWindowLongPtrW(hWnd, 0);

    switch (message)
    {
    case WM_SIZE:
        ResizePopupChild(info);
        return 0;
    case WM_DESTROY:
        DestroyWindow(hWnd);
        return 0;
    case WM_CLOSE:
        ShowWindow(hWnd, SW_HIDE);
        return 0;

    default:
        return DefWindowProcW(hWnd, message, wParam, lParam);
    }

    return 0;
}

static LRESULT CALLBACK PopupChild_WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_NOTIFY: {
        NMHDR *nmhdr = (NMHDR*)lParam;
        switch(nmhdr->code)
        {
        case NM_DBLCLK: {
            HHInfo *info = (HHInfo*)GetWindowLongPtrW(hWnd, 0);
            IndexSubItem *iter;

            if(info == 0 || lParam == 0)
                return 0;
            iter = (IndexSubItem*) ((NMITEMACTIVATE *)lParam)->lParam;
            if(iter == 0)
                return 0;
            NavigateToChm(info, info->index->merge.chm_file, iter->local);
            ShowWindow(info->popup.hwndPopup, SW_HIDE);
            return 0;
        }
        case NM_RETURN: {
            HHInfo *info = (HHInfo*)GetWindowLongPtrW(hWnd, 0);
            IndexSubItem *iter;
            LVITEMW lvItem;

            if(info == 0)
                return 0;

            lvItem.iItem = (int) SendMessageW(info->popup.hwndList, LVM_GETSELECTIONMARK, 0, 0);
            lvItem.mask = TVIF_PARAM;
            SendMessageW(info->popup.hwndList, LVM_GETITEMW, 0, (LPARAM)&lvItem);
            iter = (IndexSubItem*) lvItem.lParam;
            NavigateToChm(info, info->index->merge.chm_file, iter->local);
            ShowWindow(info->popup.hwndPopup, SW_HIDE);
            return 0;
        }
        }
        break;
    }
    default:
        return DefWindowProcW(hWnd, message, wParam, lParam);
    }

    return 0;
}

static BOOL AddIndexPopup(HHInfo *info)
{
    static const WCHAR szPopupChildClass[] = {'H','H',' ','P','o','p','u','p',' ','C','h','i','l','d',0};
    static const WCHAR windowCaptionW[] = {'S','e','l','e','c','t',' ','T','o','p','i','c',':',0};
    static const WCHAR windowClassW[] = {'H','H',' ','P','o','p','u','p',0};
    HWND hwndList, hwndPopup, hwndCallback;
    char hidden_column[] = "Column";
    WNDCLASSEXW wcex;
    LVCOLUMNA lvc;

    if(info->tabs[TAB_INDEX].id == -1)
        return TRUE; /* No "Index" tab */

    wcex.cbSize         = sizeof(WNDCLASSEXW);
    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = HelpPopup_WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = sizeof(LONG_PTR);
    wcex.hInstance      = hhctrl_hinstance;
    wcex.hIcon          = LoadIconW(NULL, (LPCWSTR)IDI_APPLICATION);
    wcex.hCursor        = LoadCursorW(NULL, (LPCWSTR)IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_MENU + 1);
    wcex.lpszMenuName   = NULL;
    wcex.lpszClassName  = windowClassW;
    wcex.hIconSm        = LoadIconW(NULL, (LPCWSTR)IDI_APPLICATION);
    RegisterClassExW(&wcex);

    wcex.cbSize         = sizeof(WNDCLASSEXW);
    wcex.style          = 0;
    wcex.lpfnWndProc    = PopupChild_WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = sizeof(LONG_PTR);
    wcex.hInstance      = hhctrl_hinstance;
    wcex.hIcon          = LoadIconW(NULL, (LPCWSTR)IDI_APPLICATION);
    wcex.hCursor        = LoadCursorW(NULL, (LPCWSTR)IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_BTNFACE + 1);
    wcex.lpszMenuName   = NULL;
    wcex.lpszClassName  = szPopupChildClass;
    wcex.hIconSm        = LoadIconW(NULL, (LPCWSTR)IDI_APPLICATION);
    RegisterClassExW(&wcex);

    hwndPopup = CreateWindowExW(WS_EX_LEFT | WS_EX_LTRREADING | WS_EX_APPWINDOW
                                 | WS_EX_WINDOWEDGE | WS_EX_RIGHTSCROLLBAR,
                                windowClassW, windowCaptionW, WS_POPUPWINDOW
                                 | WS_OVERLAPPEDWINDOW | WS_VISIBLE
                                 | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, CW_USEDEFAULT,
                                CW_USEDEFAULT, 300, 200, info->WinType.hwndHelp,
                                NULL, hhctrl_hinstance, NULL);
    if (!hwndPopup)
        return FALSE;

    hwndCallback = CreateWindowExW(WS_EX_LEFT | WS_EX_LTRREADING | WS_EX_RIGHTSCROLLBAR,
                                   szPopupChildClass, szEmpty, WS_CHILDWINDOW | WS_VISIBLE,
                                   0, 0, 0, 0,
                                   hwndPopup, NULL, hhctrl_hinstance, NULL);
    if (!hwndCallback)
        return FALSE;

    ShowWindow(hwndPopup, SW_HIDE);
    hwndList = CreateWindowExW(WS_EX_CLIENTEDGE, WC_LISTVIEWW, szEmpty,
                               WS_CHILD | WS_BORDER | LVS_SINGLESEL | LVS_REPORT
                                | LVS_NOCOLUMNHEADER, 50, 50, 100, 100,
                               hwndCallback, NULL, hhctrl_hinstance, NULL);
    if(!hwndList) {
        ERR("Could not create popup ListView control\n");
        return FALSE;
    }
    memset(&lvc, 0, sizeof(lvc));
    lvc.mask = LVCF_TEXT;
    lvc.pszText = hidden_column;
    if(SendMessageW(hwndList, LVM_INSERTCOLUMNA, 0, (LPARAM) &lvc) == -1)
    {
        ERR("Could not create popup ListView column\n");
        return FALSE;
    }

    info->popup.hwndCallback = hwndCallback;
    info->popup.hwndPopup = hwndPopup;
    info->popup.hwndList = hwndList;
    SetWindowLongPtrW(hwndPopup, 0, (LONG_PTR)info);
    SetWindowLongPtrW(hwndCallback, 0, (LONG_PTR)info);

    ResizePopupChild(info);
    ShowWindow(hwndList, SW_SHOW);

    return TRUE;
}

/* Viewer Window */

static void ExpandContract(HHInfo *pHHInfo)
{
    RECT r, nav;

    pHHInfo->WinType.fNotExpanded = !pHHInfo->WinType.fNotExpanded;
    GetWindowRect(pHHInfo->WinType.hwndHelp, &r);
    NP_GetNavigationRect(pHHInfo, &nav);

    /* hide/show both the nav bar and the size bar */
    if (pHHInfo->WinType.fNotExpanded)
    {
        ShowWindow(pHHInfo->WinType.hwndNavigation, SW_HIDE);
        ShowWindow(pHHInfo->hwndSizeBar, SW_HIDE);
        r.left = r.left + nav.right;

        SendMessageW(pHHInfo->WinType.hwndToolBar, TB_HIDEBUTTON, IDTB_EXPAND, MAKELPARAM(FALSE, 0));
        SendMessageW(pHHInfo->WinType.hwndToolBar, TB_HIDEBUTTON, IDTB_CONTRACT, MAKELPARAM(TRUE, 0));
    }
    else
    {
        ShowWindow(pHHInfo->WinType.hwndNavigation, SW_SHOW);
        ShowWindow(pHHInfo->hwndSizeBar, SW_SHOW);
        r.left = r.left - nav.right;

        SendMessageW(pHHInfo->WinType.hwndToolBar, TB_HIDEBUTTON, IDTB_EXPAND, MAKELPARAM(TRUE, 0));
        SendMessageW(pHHInfo->WinType.hwndToolBar, TB_HIDEBUTTON, IDTB_CONTRACT, MAKELPARAM(FALSE, 0));
    }

    MoveWindow(pHHInfo->WinType.hwndHelp, r.left, r.top, r.right-r.left, r.bottom-r.top, TRUE);
}

static LRESULT Help_OnSize(HWND hWnd)
{
    HHInfo *pHHInfo = (HHInfo *)GetWindowLongPtrW(hWnd, 0);
    DWORD dwSize;
    RECT rc;

    if (!pHHInfo)
        return 0;

    if (navigation_visible(pHHInfo))
    {
        NP_GetNavigationRect(pHHInfo, &rc);
        SetWindowPos(pHHInfo->WinType.hwndNavigation, HWND_TOP, 0, 0,
                     rc.right, rc.bottom, SWP_NOMOVE);

        SB_GetSizeBarRect(pHHInfo, &rc);
        SetWindowPos(pHHInfo->hwndSizeBar, HWND_TOP, rc.left, rc.top,
                     rc.right, rc.bottom, SWP_SHOWWINDOW);

    }

    HP_GetHTMLRect(pHHInfo, &rc);
    SetWindowPos(pHHInfo->WinType.hwndHTML, HWND_TOP, rc.left, rc.top,
                 rc.right, rc.bottom, SWP_SHOWWINDOW);

    /* Resize browser window taking the frame size into account */
    dwSize = GetSystemMetrics(SM_CXFRAME);
    ResizeWebBrowser(pHHInfo, rc.right - dwSize, rc.bottom - dwSize);

    return 0;
}

void UpdateHelpWindow(HHInfo *info)
{
    if (!info->WinType.hwndHelp)
        return;

    WARN("Only the size of the window is currently updated.\n");
    if (info->WinType.fsValidMembers & HHWIN_PARAM_RECT)
    {
        RECT *rect = &info->WinType.rcWindowPos;
        INT x, y, width, height;

        x = rect->left;
        y = rect->top;
        width = rect->right - x;
        height = rect->bottom - y;
        SetWindowPos(info->WinType.hwndHelp, NULL, rect->left, rect->top, width, height,
                     SWP_NOZORDER | SWP_NOACTIVATE);
    }
}

static LRESULT CALLBACK Help_WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_COMMAND:
        if (HIWORD(wParam) == BN_CLICKED)
            TB_OnClick(hWnd, LOWORD(wParam));
        break;
    case WM_SIZE:
        return Help_OnSize(hWnd);
    case WM_CLOSE:
        ReleaseHelpViewer((HHInfo *)GetWindowLongPtrW(hWnd, 0));
        return 0;
    case WM_DESTROY:
        if(hh_process)
            PostQuitMessage(0);
        break;

    default:
        return DefWindowProcW(hWnd, message, wParam, lParam);
    }

    return 0;
}

static BOOL HH_CreateHelpWindow(HHInfo *info)
{
    HWND hWnd, parent = 0;
    RECT winPos = info->WinType.rcWindowPos;
    WNDCLASSEXW wcex;
    DWORD dwStyles, dwExStyles;
    DWORD x, y, width = 0, height = 0;
    LPCWSTR caption;

    static const WCHAR windowClassW[] = {
        'H','H',' ', 'P','a','r','e','n','t',0
    };

    wcex.cbSize         = sizeof(WNDCLASSEXW);
    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = Help_WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = sizeof(LONG_PTR);
    wcex.hInstance      = hhctrl_hinstance;
    wcex.hIcon          = LoadIconW(NULL, (LPCWSTR)IDI_APPLICATION);
    wcex.hCursor        = LoadCursorW(NULL, (LPCWSTR)IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_MENU + 1);
    wcex.lpszMenuName   = NULL;
    wcex.lpszClassName  = windowClassW;
    wcex.hIconSm        = LoadIconW(NULL, (LPCWSTR)IDI_APPLICATION);

    RegisterClassExW(&wcex);

    /* Read in window parameters if available */
    if (info->WinType.fsValidMembers & HHWIN_PARAM_STYLES)
    {
        dwStyles = info->WinType.dwStyles;
        if (!(info->WinType.dwStyles & WS_CHILD))
            dwStyles |= WS_OVERLAPPEDWINDOW;
    }
    else
        dwStyles = WS_OVERLAPPEDWINDOW | WS_VISIBLE |
                   WS_CLIPSIBLINGS | WS_CLIPCHILDREN;

    if (info->WinType.fsValidMembers & HHWIN_PARAM_EXSTYLES)
        dwExStyles = info->WinType.dwExStyles;
    else
        dwExStyles = WS_EX_LEFT | WS_EX_LTRREADING | WS_EX_APPWINDOW |
                     WS_EX_WINDOWEDGE | WS_EX_RIGHTSCROLLBAR;

    if (info->WinType.fsValidMembers & HHWIN_PARAM_RECT)
    {
        x = winPos.left;
        y = winPos.top;
        width = winPos.right - x;
        height = winPos.bottom - y;
    }
    if (!width || !height)
    {
        x = WINTYPE_DEFAULT_X;
        y = WINTYPE_DEFAULT_Y;
        width = WINTYPE_DEFAULT_WIDTH;
        height = WINTYPE_DEFAULT_HEIGHT;
    }

    if (!(info->WinType.fsWinProperties & HHWIN_PROP_TRI_PANE) && info->WinType.fNotExpanded)
    {
        if (!(info->WinType.fsValidMembers & HHWIN_PARAM_NAV_WIDTH) &&
              info->WinType.iNavWidth == 0)
        {
            info->WinType.iNavWidth = WINTYPE_DEFAULT_NAVWIDTH;
        }

        x += info->WinType.iNavWidth;
        width -= info->WinType.iNavWidth;
    }


    caption = info->WinType.pszCaption;
    if (!*caption) caption = info->pCHMInfo->defTitle;

    if (info->WinType.dwStyles & WS_CHILD)
        parent = info->WinType.hwndCaller;

    hWnd = CreateWindowExW(dwExStyles, windowClassW, caption,
                           dwStyles, x, y, width, height, parent, NULL, hhctrl_hinstance, NULL);
    if (!hWnd)
        return FALSE;

    ShowWindow(hWnd, SW_SHOW);
    UpdateWindow(hWnd);

    /* store the pointer to the HH info struct */
    SetWindowLongPtrW(hWnd, 0, (LONG_PTR)info);

    info->WinType.hwndHelp = hWnd;
    return TRUE;
}

static void HH_CreateFont(HHInfo *pHHInfo)
{
    LOGFONTW lf;

    GetObjectW(GetStockObject(DEFAULT_GUI_FONT), sizeof(LOGFONTW), &lf);
    lf.lfWeight = FW_NORMAL;
    lf.lfItalic = FALSE;
    lf.lfUnderline = FALSE;

    pHHInfo->hFont = CreateFontIndirectW(&lf);
}

static void HH_InitRequiredControls(DWORD dwControls)
{
    INITCOMMONCONTROLSEX icex;

    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = dwControls;
    InitCommonControlsEx(&icex);
}

/* Creates the whole package */
static BOOL CreateViewer(HHInfo *pHHInfo)
{
    HH_CreateFont(pHHInfo);

    if (!HH_CreateHelpWindow(pHHInfo))
        return FALSE;

    HH_InitRequiredControls(ICC_BAR_CLASSES);

    if (!HH_AddToolbar(pHHInfo))
        return FALSE;

    HH_RegisterChildWndClass(pHHInfo);

    if (!HH_AddNavigationPane(pHHInfo))
        return FALSE;

    HH_RegisterSizeBarClass(pHHInfo);

    if (!HH_AddSizeBar(pHHInfo))
        return FALSE;

    if (!HH_AddHTMLPane(pHHInfo))
        return FALSE;

    if (!AddContentTab(pHHInfo))
        return FALSE;

    if (!AddIndexTab(pHHInfo))
        return FALSE;

    if (!AddIndexPopup(pHHInfo))
        return FALSE;

    if (!AddSearchTab(pHHInfo))
        return FALSE;

    InitContent(pHHInfo);
    InitIndex(pHHInfo);

    pHHInfo->viewer_initialized = TRUE;
    return TRUE;
}

void wintype_stringsW_free(struct wintype_stringsW *stringsW)
{
    heap_free(stringsW->pszType);
    heap_free(stringsW->pszCaption);
    heap_free(stringsW->pszToc);
    heap_free(stringsW->pszIndex);
    heap_free(stringsW->pszFile);
    heap_free(stringsW->pszHome);
    heap_free(stringsW->pszJump1);
    heap_free(stringsW->pszJump2);
    heap_free(stringsW->pszUrlJump1);
    heap_free(stringsW->pszUrlJump2);
}

void wintype_stringsA_free(struct wintype_stringsA *stringsA)
{
    heap_free(stringsA->pszType);
    heap_free(stringsA->pszCaption);
    heap_free(stringsA->pszToc);
    heap_free(stringsA->pszIndex);
    heap_free(stringsA->pszFile);
    heap_free(stringsA->pszHome);
    heap_free(stringsA->pszJump1);
    heap_free(stringsA->pszJump2);
    heap_free(stringsA->pszUrlJump1);
    heap_free(stringsA->pszUrlJump2);
    heap_free(stringsA->pszCustomTabs);
}

void ReleaseHelpViewer(HHInfo *info)
{
    TRACE("(%p)\n", info);

    if (!info)
        return;

    list_remove(&info->entry);

    wintype_stringsA_free(&info->stringsA);
    wintype_stringsW_free(&info->stringsW);

    if (info->pCHMInfo)
        CloseCHM(info->pCHMInfo);

    ReleaseWebBrowser(info);
    ReleaseContent(info);
    ReleaseIndex(info);
    ReleaseSearch(info);

    if(info->contents.hImageList)
        ImageList_Destroy(info->contents.hImageList);
    if(info->WinType.hwndHelp)
        DestroyWindow(info->WinType.hwndHelp);

    heap_free(info);
    OleUninitialize();
}

HHInfo *CreateHelpViewer(HHInfo *info, LPCWSTR filename, HWND caller)
{
    HHInfo *tmp_info;
    unsigned int i;

    if(!info)
    {
        info = heap_alloc_zero(sizeof(HHInfo));
        list_add_tail(&window_list, &info->entry);
    }

    /* Set the invalid tab ID (-1) as the default value for all
     * of the tabs, this matches a failed TCM_INSERTITEM call.
     */
    for(i=0;i<sizeof(info->tabs)/sizeof(HHTab);i++)
        info->tabs[i].id = -1;

    OleInitialize(NULL);

    info->pCHMInfo = OpenCHM(filename);
    if(!info->pCHMInfo) {
        ReleaseHelpViewer(info);
        return NULL;
    }

    if (!LoadWinTypeFromCHM(info)) {
        ReleaseHelpViewer(info);
        return NULL;
    }
    info->WinType.hwndCaller = caller;

    /* If the window is already open then load the file in that existing window */
    if ((tmp_info = find_window(info->WinType.pszType)) && tmp_info != info)
    {
        ReleaseHelpViewer(info);
        return CreateHelpViewer(tmp_info, filename, caller);
    }

    if(!info->viewer_initialized && !CreateViewer(info)) {
        ReleaseHelpViewer(info);
        return NULL;
    }

    return info;
}

/*
 * Search the table of HTML entities and return the corresponding ANSI symbol.
 */
static char find_html_symbol(const char *entity, int entity_len)
{
    int max = sizeof(html_encoded_symbols)/sizeof(html_encoded_symbols[0])-1;
    int min = 0, dir;

    while(min <= max)
    {
        int pos = (min+max)/2;
        const char *encoded_symbol = html_encoded_symbols[pos].html_code;
        dir = strncmp(encoded_symbol, entity, entity_len);
        if(dir == 0 && !encoded_symbol[entity_len]) return html_encoded_symbols[pos].ansi_symbol;
        if(dir < 0)
            min = pos+1;
        else
            max = pos-1;
    }
    return 0;
}

/*
 * Decode a string containing HTML encoded characters into a unicode string.
 */
WCHAR *decode_html(const char *html_fragment, int html_fragment_len, UINT code_page)
{
    const char *h = html_fragment, *amp, *sem;
    char symbol, *tmp;
    int len, tmp_len = 0;
    WCHAR *unicode_text;

    tmp = heap_alloc(html_fragment_len+1);
    while(1)
    {
        symbol = 0;
        amp = strchr(h, '&');
        if(!amp) break;
        len = amp-h;
        /* Copy the characters prior to the HTML encoded character */
        memcpy(&tmp[tmp_len], h, len);
        tmp_len += len;
        amp++; /* skip ampersand */
        sem = strchr(amp, ';');
        /* Require a semicolon after the ampersand */
        if(!sem)
        {
            h = amp;
            tmp[tmp_len++] = '&';
            continue;
        }
        /* Find the symbol either by using the ANSI character number (prefixed by the pound symbol)
         * or by searching the HTML entity table */
        len = sem-amp;
        if(amp[0] == '#')
        {
            char *endnum = NULL;
            int tmp;

            tmp = (char) strtol(amp, &endnum, 10);
            if(endnum == sem)
                symbol = tmp;
        }
        else
            symbol = find_html_symbol(amp, len);
        if(!symbol)
        {
            FIXME("Failed to translate HTML encoded character '&%.*s;'.\n", len, amp);
            h = amp;
            tmp[tmp_len++] = '&';
            continue;
        }
        /* Insert the new symbol */
        h = sem+1;
        tmp[tmp_len++] = symbol;
    }
    /* Convert any remaining characters */
    len = html_fragment_len-(h-html_fragment);
    memcpy(&tmp[tmp_len], h, len);
    tmp_len += len;
    tmp[tmp_len++] = 0; /* NULL-terminate the string */

    len = MultiByteToWideChar(code_page, 0, tmp, tmp_len, NULL, 0);
    unicode_text = heap_alloc(len*sizeof(WCHAR));
    MultiByteToWideChar(code_page, 0, tmp, tmp_len, unicode_text, len);
    heap_free(tmp);
    return unicode_text;
}

/* Find the HTMLHelp structure for an existing window title */
HHInfo *find_window(const WCHAR *window)
{
    HHInfo *info;

    LIST_FOR_EACH_ENTRY(info, &window_list, HHInfo, entry)
    {
        if (strcmpW(info->WinType.pszType, window) == 0)
            return info;
    }
    return NULL;
}
