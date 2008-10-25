/*
 * Help Viewer Implementation
 *
 * Copyright 2005 James Hawkins
 * Copyright 2007 Jacek Caban for CodeWeavers
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

#include "wingdi.h"
#include "commctrl.h"
#include "wininet.h"

#include "wine/debug.h"

#include "resource.h"

WINE_DEFAULT_DEBUG_CHANNEL(htmlhelp);

static LRESULT Help_OnSize(HWND hWnd);

/* Window type defaults */

#define WINTYPE_DEFAULT_X           280
#define WINTYPE_DEFAULT_Y           100
#define WINTYPE_DEFAULT_WIDTH       740
#define WINTYPE_DEFAULT_HEIGHT      640
#define WINTYPE_DEFAULT_NAVWIDTH    250

#define TAB_TOP_PADDING     8
#define TAB_RIGHT_PADDING   4
#define TAB_MARGIN  8

static const WCHAR szEmpty[] = {0};

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

    hres = IWebBrowser2_Navigate2(info->web_browser, &url, 0, 0, 0, 0);

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

BOOL NavigateToChm(HHInfo *info, LPCWSTR file, LPCWSTR index)
{
    WCHAR buf[INTERNET_MAX_URL_LENGTH];
    WCHAR full_path[MAX_PATH];
    LPWSTR ptr;

    static const WCHAR url_format[] =
        {'m','k',':','@','M','S','I','T','S','t','o','r','e',':','%','s',':',':','%','s',0};

    TRACE("%p %s %s\n", info, debugstr_w(file), debugstr_w(index));

    if (!info->web_browser)
        return FALSE;

    if(!GetFullPathNameW(file, sizeof(full_path)/sizeof(full_path[0]), full_path, NULL)) {
        WARN("GetFullPathName failed: %u\n", GetLastError());
        return FALSE;
    }

    wsprintfW(buf, url_format, full_path, index);

    /* FIXME: HACK */
    if((ptr = strchrW(buf, '#')))
       *ptr = 0;

    return SUCCEEDED(navigate_url(info, buf));
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
    HHInfo *pHHInfo = (HHInfo *)GetWindowLongPtrW(hWnd, GWLP_USERDATA);
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
    wcex.cbWndExtra     = 0;
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
    DWORD dwStyles = WS_CHILDWINDOW | WS_VISIBLE | WS_OVERLAPPED;
    DWORD dwExStyles = WS_EX_LEFT | WS_EX_LTRREADING | WS_EX_RIGHTSCROLLBAR;
    RECT rc;

    SB_GetSizeBarRect(pHHInfo, &rc);

    hWnd = CreateWindowExW(dwExStyles, szSizeBarClass, szEmpty, dwStyles,
                           rc.left, rc.top, rc.right, rc.bottom,
                           hwndParent, NULL, hhctrl_hinstance, NULL);
    if (!hWnd)
        return FALSE;

    /* store the pointer to the HH info struct */
    SetWindowLongPtrW(hWnd, GWLP_USERDATA, (LONG_PTR)pHHInfo);

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

static void ResizeTabChild(HHInfo *info, HWND hwnd)
{
    RECT rect, tabrc;
    DWORD cnt;

    GetClientRect(info->WinType.hwndNavigation, &rect);
    SendMessageW(info->hwndTabCtrl, TCM_GETITEMRECT, 0, (LPARAM)&tabrc);
    cnt = SendMessageW(info->hwndTabCtrl, TCM_GETROWCOUNT, 0, 0);

    rect.left = TAB_MARGIN;
    rect.top = TAB_TOP_PADDING + cnt*(tabrc.bottom-tabrc.top) + TAB_MARGIN;
    rect.right -= TAB_RIGHT_PADDING + TAB_MARGIN;
    rect.bottom -= TAB_MARGIN;

    SetWindowPos(hwnd, NULL, rect.left, rect.top, rect.right-rect.left,
                 rect.bottom-rect.top, SWP_NOZORDER | SWP_NOACTIVATE);
}

static LRESULT Child_OnSize(HWND hwnd)
{
    HHInfo *info = (HHInfo*)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
    RECT rect;

    if(!info || hwnd != info->WinType.hwndNavigation)
        return 0;

    GetClientRect(hwnd, &rect);
    SetWindowPos(info->hwndTabCtrl, HWND_TOP, 0, 0,
                 rect.right - TAB_RIGHT_PADDING,
                 rect.bottom - TAB_TOP_PADDING, SWP_NOMOVE);

    ResizeTabChild(info, info->tabs[TAB_CONTENTS].hwnd);
    return 0;
}

static LRESULT OnTabChange(HWND hwnd)
{
    HHInfo *info = (HHInfo*)GetWindowLongPtrW(hwnd, GWLP_USERDATA);

    TRACE("%p\n", hwnd);

    if (!info)
        return 0;

    if(info->tabs[info->current_tab].hwnd)
        ShowWindow(info->tabs[info->current_tab].hwnd, SW_HIDE);

    info->current_tab = SendMessageW(info->hwndTabCtrl, TCM_GETCURSEL, 0, 0);

    if(info->tabs[info->current_tab].hwnd)
        ShowWindow(info->tabs[info->current_tab].hwnd, SW_SHOW);

    return 0;
}

static LRESULT OnTopicChange(HWND hwnd, ContentItem *item)
{
    HHInfo *info = (HHInfo*)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
    LPCWSTR chmfile = NULL;
    ContentItem *iter = item;

    if(!item || !info)
        return 0;

    TRACE("name %s loal %s\n", debugstr_w(item->name), debugstr_w(item->local));

    while(iter) {
        if(iter->merge.chm_file) {
            chmfile = iter->merge.chm_file;
            break;
        }
        iter = iter->parent;
    }

    NavigateToChm(info, chmfile, item->local);
    return 0;
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
        NMHDR *nmhdr = (NMHDR*)lParam;
        switch(nmhdr->code) {
        case TCN_SELCHANGE:
            return OnTabChange(hWnd);
        case TVN_SELCHANGEDW:
            return OnTopicChange(hWnd, (ContentItem*)((NMTREEVIEWW *)lParam)->itemNew.lParam);
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
    wcex.cbWndExtra     = 0;
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

static void TB_OnClick(HWND hWnd, DWORD dwID)
{
    HHInfo *info = (HHInfo *)GetWindowLongPtrW(hWnd, GWLP_USERDATA);

    switch (dwID)
    {
        case IDTB_STOP:
            DoPageAction(info, WB_STOP);
            break;
        case IDTB_REFRESH:
            DoPageAction(info, WB_REFRESH);
            break;
        case IDTB_BACK:
            DoPageAction(info, WB_GOBACK);
            break;
        case IDTB_HOME:
            NavigateToChm(info, info->pCHMInfo->szFile, info->WinType.pszHome);
            break;
        case IDTB_FORWARD:
            DoPageAction(info, WB_GOFORWARD);
            break;
        case IDTB_EXPAND:
        case IDTB_CONTRACT:
        case IDTB_SYNC:
        case IDTB_PRINT:
        case IDTB_OPTIONS:
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

static void TB_AddButton(TBBUTTON *pButtons, DWORD dwIndex, DWORD dwID)
{
    /* FIXME: Load the correct button bitmaps */
    pButtons[dwIndex].iBitmap = STD_PRINT;
    pButtons[dwIndex].idCommand = dwID;
    pButtons[dwIndex].fsState = TBSTATE_ENABLED;
    pButtons[dwIndex].fsStyle = BTNS_BUTTON;
    pButtons[dwIndex].dwData = 0;
    pButtons[dwIndex].iString = 0;
}

static void TB_AddButtonsFromFlags(TBBUTTON *pButtons, DWORD dwButtonFlags, LPDWORD pdwNumButtons)
{
    *pdwNumButtons = 0;

    if (dwButtonFlags & HHWIN_BUTTON_EXPAND)
        TB_AddButton(pButtons, (*pdwNumButtons)++, IDTB_EXPAND);

    if (dwButtonFlags & HHWIN_BUTTON_BACK)
        TB_AddButton(pButtons, (*pdwNumButtons)++, IDTB_BACK);

    if (dwButtonFlags & HHWIN_BUTTON_FORWARD)
        TB_AddButton(pButtons, (*pdwNumButtons)++, IDTB_FORWARD);

    if (dwButtonFlags & HHWIN_BUTTON_STOP)
        TB_AddButton(pButtons, (*pdwNumButtons)++, IDTB_STOP);

    if (dwButtonFlags & HHWIN_BUTTON_REFRESH)
        TB_AddButton(pButtons, (*pdwNumButtons)++, IDTB_REFRESH);

    if (dwButtonFlags & HHWIN_BUTTON_HOME)
        TB_AddButton(pButtons, (*pdwNumButtons)++, IDTB_HOME);

    if (dwButtonFlags & HHWIN_BUTTON_SYNC)
        TB_AddButton(pButtons, (*pdwNumButtons)++, IDTB_SYNC);

    if (dwButtonFlags & HHWIN_BUTTON_OPTIONS)
        TB_AddButton(pButtons, (*pdwNumButtons)++, IDTB_OPTIONS);

    if (dwButtonFlags & HHWIN_BUTTON_PRINT)
        TB_AddButton(pButtons, (*pdwNumButtons)++, IDTB_PRINT);

    if (dwButtonFlags & HHWIN_BUTTON_JUMP1)
        TB_AddButton(pButtons, (*pdwNumButtons)++, IDTB_JUMP1);

    if (dwButtonFlags & HHWIN_BUTTON_JUMP2)
        TB_AddButton(pButtons,(*pdwNumButtons)++, IDTB_JUMP2);

    if (dwButtonFlags & HHWIN_BUTTON_ZOOM)
        TB_AddButton(pButtons, (*pdwNumButtons)++, IDTB_ZOOM);

    if (dwButtonFlags & HHWIN_BUTTON_TOC_NEXT)
        TB_AddButton(pButtons, (*pdwNumButtons)++, IDTB_TOC_NEXT);

    if (dwButtonFlags & HHWIN_BUTTON_TOC_PREV)
        TB_AddButton(pButtons, (*pdwNumButtons)++, IDTB_TOC_PREV);
}

static BOOL HH_AddToolbar(HHInfo *pHHInfo)
{
    HWND hToolbar;
    HWND hwndParent = pHHInfo->WinType.hwndHelp;
    DWORD toolbarFlags;
    TBBUTTON buttons[IDTB_TOC_PREV - IDTB_EXPAND];
    TBADDBITMAP tbAB;
    DWORD dwStyles, dwExStyles;
    DWORD dwNumButtons, dwIndex;

    if (pHHInfo->WinType.fsWinProperties & HHWIN_PARAM_TB_FLAGS)
        toolbarFlags = pHHInfo->WinType.fsToolBarFlags;
    else
        toolbarFlags = HHWIN_DEF_BUTTONS;

    TB_AddButtonsFromFlags(buttons, toolbarFlags, &dwNumButtons);

    dwStyles = WS_CHILDWINDOW | WS_VISIBLE | TBSTYLE_FLAT |
               TBSTYLE_WRAPABLE | TBSTYLE_TOOLTIPS | CCS_NODIVIDER;
    dwExStyles = WS_EX_LEFT | WS_EX_LTRREADING | WS_EX_RIGHTSCROLLBAR;

    hToolbar = CreateWindowExW(dwExStyles, TOOLBARCLASSNAMEW, NULL, dwStyles,
                               0, 0, 0, 0, hwndParent, NULL,
                               hhctrl_hinstance, NULL);
    if (!hToolbar)
        return FALSE;

    SendMessageW(hToolbar, TB_SETBITMAPSIZE, 0, MAKELONG(ICON_SIZE, ICON_SIZE));
    SendMessageW(hToolbar, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);
    SendMessageW(hToolbar, WM_SETFONT, (WPARAM)pHHInfo->hFont, TRUE);

    /* FIXME: Load correct icons for all buttons */
    tbAB.hInst = HINST_COMMCTRL;
    tbAB.nID = IDB_STD_LARGE_COLOR;
    SendMessageW(hToolbar, TB_ADDBITMAP, 0, (LPARAM)&tbAB);

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
    ShowWindow(hToolbar, SW_SHOW);

    pHHInfo->WinType.hwndToolBar = hToolbar;
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
    DWORD dwStyles = WS_CHILDWINDOW | WS_VISIBLE;
    DWORD dwExStyles = WS_EX_LEFT | WS_EX_LTRREADING | WS_EX_RIGHTSCROLLBAR;
    RECT rc;

    NP_GetNavigationRect(info, &rc);

    hWnd = CreateWindowExW(dwExStyles, szChildClass, szEmpty, dwStyles,
                           rc.left, rc.top, rc.right, rc.bottom,
                           hwndParent, NULL, hhctrl_hinstance, NULL);
    if (!hWnd)
        return FALSE;

    SetWindowLongPtrW(hWnd, GWLP_USERDATA, (LONG_PTR)info);

    hwndTabCtrl = CreateWindowExW(dwExStyles, WC_TABCONTROLW, szEmpty, dwStyles,
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
    GetClientRect(info->WinType.hwndToolBar, &rectTB);
    GetClientRect(info->WinType.hwndNavigation, &rectNP);
    GetClientRect(info->hwndSizeBar, &rectSB);

    rc->left = rectNP.right + rectSB.right;
    rc->top = rectTB.bottom;
    rc->right = rectWND.right - rc->left;
    rc->bottom = rectWND.bottom - rectTB.bottom;
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
    SetWindowLongPtrW(hWnd, GWLP_USERDATA, (LONG_PTR)pHHInfo);

    ShowWindow(hWnd, SW_SHOW);
    UpdateWindow(hWnd);

    pHHInfo->WinType.hwndHTML = hWnd;
    return TRUE;
}

static BOOL AddContentTab(HHInfo *info)
{
    info->tabs[TAB_CONTENTS].hwnd = CreateWindowExW(WS_EX_CLIENTEDGE, WC_TREEVIEWW,
           szEmpty, WS_CHILD | WS_BORDER | 0x25, 50, 50, 100, 100,
           info->WinType.hwndNavigation, NULL, hhctrl_hinstance, NULL);
    if(!info->tabs[TAB_CONTENTS].hwnd) {
        ERR("Could not create treeview control\n");
        return FALSE;
    }

    ResizeTabChild(info, info->tabs[TAB_CONTENTS].hwnd);
    ShowWindow(info->tabs[TAB_CONTENTS].hwnd, SW_SHOW);

    return TRUE;
}

/* Viewer Window */

static LRESULT Help_OnSize(HWND hWnd)
{
    HHInfo *pHHInfo = (HHInfo *)GetWindowLongPtrW(hWnd, GWLP_USERDATA);
    DWORD dwSize;
    RECT rc;

    if (!pHHInfo)
        return 0;

    NP_GetNavigationRect(pHHInfo, &rc);
    SetWindowPos(pHHInfo->WinType.hwndNavigation, HWND_TOP, 0, 0,
                 rc.right, rc.bottom, SWP_NOMOVE);

    SB_GetSizeBarRect(pHHInfo, &rc);
    SetWindowPos(pHHInfo->hwndSizeBar, HWND_TOP, rc.left, rc.top,
                 rc.right, rc.bottom, SWP_SHOWWINDOW);

    HP_GetHTMLRect(pHHInfo, &rc);
    SetWindowPos(pHHInfo->WinType.hwndHTML, HWND_TOP, rc.left, rc.top,
                 rc.right, rc.bottom, SWP_SHOWWINDOW);

    /* Resize browser window taking the frame size into account */
    dwSize = GetSystemMetrics(SM_CXFRAME);
    ResizeWebBrowser(pHHInfo, rc.right - dwSize, rc.bottom - dwSize);

    return 0;
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
        ReleaseHelpViewer((HHInfo *)GetWindowLongPtrW(hWnd, GWLP_USERDATA));
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
    HWND hWnd;
    RECT winPos = info->WinType.rcWindowPos;
    WNDCLASSEXW wcex;
    DWORD dwStyles, dwExStyles;
    DWORD x, y, width, height;

    static const WCHAR windowClassW[] = {
        'H','H',' ', 'P','a','r','e','n','t',0
    };

    wcex.cbSize         = sizeof(WNDCLASSEXW);
    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = Help_WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
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
        dwStyles = info->WinType.dwStyles;
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
    else
    {
        x = WINTYPE_DEFAULT_X;
        y = WINTYPE_DEFAULT_Y;
        width = WINTYPE_DEFAULT_WIDTH;
        height = WINTYPE_DEFAULT_HEIGHT;
    }

    hWnd = CreateWindowExW(dwExStyles, windowClassW, info->WinType.pszCaption,
                           dwStyles, x, y, width, height, NULL, NULL, hhctrl_hinstance, NULL);
    if (!hWnd)
        return FALSE;

    ShowWindow(hWnd, SW_SHOW);
    UpdateWindow(hWnd);

    /* store the pointer to the HH info struct */
    SetWindowLongPtrW(hWnd, GWLP_USERDATA, (LONG_PTR)info);

    info->WinType.hwndHelp = hWnd;
    return TRUE;
}

static void HH_CreateFont(HHInfo *pHHInfo)
{
    LOGFONTW lf;

    GetObjectW(GetStockObject(ANSI_VAR_FONT), sizeof(LOGFONTW), &lf);
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

    InitContent(pHHInfo);

    return TRUE;
}

void ReleaseHelpViewer(HHInfo *info)
{
    TRACE("(%p)\n", info);

    if (!info)
        return;

    /* Free allocated strings */
    heap_free(info->pszType);
    heap_free(info->pszCaption);
    heap_free(info->pszToc);
    heap_free(info->pszIndex);
    heap_free(info->pszFile);
    heap_free(info->pszHome);
    heap_free(info->pszJump1);
    heap_free(info->pszJump2);
    heap_free(info->pszUrlJump1);
    heap_free(info->pszUrlJump2);

    if (info->pCHMInfo)
        CloseCHM(info->pCHMInfo);

    ReleaseWebBrowser(info);
    ReleaseContent(info);

    if(info->WinType.hwndHelp)
        DestroyWindow(info->WinType.hwndHelp);

    heap_free(info);
    OleUninitialize();
}

HHInfo *CreateHelpViewer(LPCWSTR filename)
{
    HHInfo *info = heap_alloc_zero(sizeof(HHInfo));

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

    if(!CreateViewer(info)) {
        ReleaseHelpViewer(info);
        return NULL;
    }

    return info;
}
