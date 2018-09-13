/*
 * hotkey.c
 *
 *  Copyright (c) 1991,  Microsoft Corporation
 *
 *  DESCRIPTION
 *
 *        This file is for support of program manager under NT Windows.
 *        This file is/was ported from hotkey.c (program manager).
 *
 *  MODIFICATION HISTORY
 *      Initial Version: x/x/90    Author Unknown, since he didn't feel
 *                                like commenting the code...
 *
 *      NT 32b Version: 1/18/91    Jeff Pack
 *                                Intitial port to begin.
 *
 *
 */

#include "progman.h"

#define HK_SHIFT    0x0100
#define HK_CONTROL  0x0200
#define HK_ALT        0x0400
#define HK_EXT        0x0800

#define F_EXT        0x01000000L

TCHAR szHotKey[] = TEXT("pmhotkey");

typedef struct HOTKEYWINDOWBYTES {
    UINT    hotkey;
    int     cyFont;
    HFONT   hfont;
} HOTKEYWINDOWBYTES;

#define HWL_HOTKEY  FIELD_OFFSET(HOTKEYWINDOWBYTES, hotkey)
#define HWL_CYFONT  FIELD_OFFSET(HOTKEYWINDOWBYTES, cyFont)
#define HWLP_FONT   FIELD_OFFSET(HOTKEYWINDOWBYTES, hfont)

/*** SetHotKey --
 *
 *
 * void APIENTRY SetHotKey(HWND hwnd, WORD hk)
 *
 * ENTRY -     HWND    hWnd
 *            WORD    hk
 *
 * EXIT  -    void
 *
 * SYNOPSIS -  ???
 *
 * WARNINGS -
 * EFFECTS  -
 *
 */

void APIENTRY SetHotKey(HWND hwnd, WPARAM hk)
{

    /* don't invalidate if it's the same
     */
    if((LONG)hk == GetWindowLong(hwnd, HWL_HOTKEY)){
        return;
    }
    SetWindowLong(hwnd, HWL_HOTKEY, (LONG)hk);

    InvalidateRect(hwnd,NULL,TRUE);
}

/*** GetKeyName --
 *
 *
 * void APIENTRY GetKeyName(WORD vk, PSTR psz, BOOL fExt)
 *
 * ENTRY -     WORD    hk
 *            PSTR    psz
 *            BOOL    fExt
 *
 * EXIT  -    void
 *
 * SYNOPSIS -  ???
 *
 * WARNINGS -
 * EFFECTS  -
 *
 */

void APIENTRY GetKeyName(UINT vk, LPTSTR psz, BOOL fExt)
{
    LONG scan;
    scan = (LONG)MapVirtualKey(vk,0) << 16;
    if (fExt){
        scan |= F_EXT;
    }

    GetKeyNameText(scan,psz,50);
}

/*** PaintHotKey --
 *
 *
 * void APIENTRY PaintHotKey(register HWND hwnd)
 *
 * ENTRY -     HWND    hWnd
 *
 * EXIT  -    void
 *
 * SYNOPSIS -  ???
 *
 * WARNINGS -
 * EFFECTS  -
 *
 */

void APIENTRY PaintHotKey(register HWND hwnd)
{
    TCHAR sz[128];
    TCHAR szPlus[10];
    WORD cch;
    WORD hk;
    register HDC hdc;
#ifndef ORGCODE
    SIZE size;
#endif
    PAINTSTRUCT ps;
    int x, y;
    HANDLE hFont;
    DWORD dwColor;

    LoadString(hAppInstance, IDS_PLUS, szPlus, CharSizeOf(szPlus));

    if(hk = (WORD)GetWindowLong(hwnd, HWL_HOTKEY)){
        sz[0] = 0;
        cch = 0;
        if (hk & HK_CONTROL){
            GetKeyName(VK_CONTROL,sz,FALSE);
            lstrcat(sz,szPlus);
          }
        if (hk & HK_SHIFT){
            GetKeyName(VK_SHIFT, sz + lstrlen(sz), FALSE);
            lstrcat(sz,szPlus);
        }
        if (hk & HK_ALT){
            GetKeyName(VK_MENU, sz + lstrlen(sz), FALSE);
            lstrcat(sz,szPlus);
        }
        GetKeyName((UINT)LOBYTE(hk), sz + lstrlen(sz), hk & HK_EXT);
    }
    else{
        LoadString(hAppInstance,IDS_NONE,sz,100);
    }

    cch = (WORD)lstrlen(sz);
    HideCaret(hwnd);

    hdc = BeginPaint(hwnd,&ps);

    SetBkMode(hdc, TRANSPARENT);

    hFont = SelectObject(hdc,(HANDLE)GetWindowLongPtr(hwnd,HWLP_FONT));
    x = GetSystemMetrics(SM_CXBORDER);
    y = GetSystemMetrics(SM_CYBORDER);

    if (IsWindowEnabled(hwnd)){
	    dwColor = GetSysColor(COLOR_WINDOWTEXT);
    	dwColor = SetTextColor(hdc,dwColor);
        TextOut(hdc,x,y,sz,cch);
    }
    else if (dwColor = GetSysColor(COLOR_GRAYTEXT)){
        dwColor = SetTextColor(hdc,dwColor);
        TextOut(hdc,x,y,sz,cch);
        SetTextColor(hdc,dwColor);
    }
    else{
        GrayString(hdc,NULL,NULL,(LPARAM)(LPTSTR)sz,cch,x,y,0,0);
    }

#ifdef ORGCODE
    x = (WORD)GetTextExtentPoint(hdc,sz,cch);
#else
    /*Used to return x/y in DWORD, now returns cx,cy in size*/
    GetTextExtentPoint(hdc, sz, cch, &size);
    x = size.cx;
#endif
    if (GetFocus() == hwnd)
    SetCaretPos(x+GetSystemMetrics(SM_CXBORDER), GetSystemMetrics(SM_CYBORDER));
    ShowCaret(hwnd);

    if (hFont){
        SelectObject(hdc,hFont);
    }

    SetBkMode(hdc, OPAQUE);

    EndPaint(hwnd,&ps);
}

/*** HotKeyWndProc --
 *
 *
 * LRESULT APIENTRY HotKeyWndProc(register HWND hwnd, UINT wMsg,
 *                                register WPARAM wParam, LPARAM lParam)
 *
 * ENTRY -     HWND    hWnd
 *            WORD    wMsg
 *            WPARAM    wParam
 *            LPARAM    lParam
 * EXIT  -    LRESULT xxx - returns info, or zero, for nothing to return
 *
 * SYNOPSIS -  ???
 *
 * WARNINGS -
 * EFFECTS  -
 *
 */

LRESULT APIENTRY HotKeyWndProc(register HWND hwnd, UINT wMsg,
                             register WPARAM wParam, LPARAM lParam)
{
    HDC hdc;
    WORD wT;
#ifndef ORGCODE
    SIZE size;
#endif
    switch (wMsg){
    case WM_CREATE:
        SetHotKey(hwnd,0);
        SendMessage(hwnd,WM_SETFONT,(WPARAM)GetStockObject(SYSTEM_FONT),0L);
        break;

    case WM_SETFOCUS:
        InvalidateRect(hwnd,NULL,TRUE);
        CreateCaret(hwnd,NULL,0,GetWindowLong(hwnd,HWL_CYFONT));
        ShowCaret(hwnd);
        break;

    case WM_KILLFOCUS:
        if (!LOBYTE(GetWindowLong(hwnd,HWL_HOTKEY))){
            SetHotKey(hwnd,0);
        }
        DestroyCaret();
        break;

    case WM_GETDLGCODE:
        return DLGC_WANTCHARS | DLGC_WANTARROWS;

    case WM_SETTEXT:
        SetHotKey(hwnd,LOWORD(lParam));
        break;

    case WM_GETTEXT:
        *(LPINT)lParam = GetWindowLong(hwnd,HWL_HOTKEY);
        break;

    case WM_SETHOTKEY:
        SetHotKey(hwnd,(WPARAM) wParam);
        break;

    case WM_GETHOTKEY:
        return GetWindowLong(hwnd,HWL_HOTKEY);

    case WM_LBUTTONDOWN:
        SetFocus(hwnd);
        break;

    case WM_SYSKEYDOWN:
    case WM_KEYDOWN:
        switch (wParam) {
        case VK_RETURN:
        case VK_TAB:
        case VK_SPACE:
        case VK_DELETE:
        case VK_ESCAPE:
        case VK_BACK:
            SetHotKey(hwnd,0);
            return DefWindowProc(hwnd,wMsg,wParam,lParam);
        case VK_MENU:
        case VK_SHIFT:
        case VK_CONTROL:
            wParam = 0;
            /*** fall thru ***/

        default:
            if (GetKeyState(VK_CONTROL) < 0)
                wParam |= HK_CONTROL;
            if (GetKeyState(VK_SHIFT) < 0)
                wParam |= HK_SHIFT;
            if (GetKeyState(VK_MENU) < 0)
                wParam |= HK_ALT;
            if (lParam & F_EXT)
                wParam |= HK_EXT;

            // more than one shift key must be specified.  That is,
            // CONTROL+ALT, CONTROL+SHIFT, SHIFT+ALT
            // get the bitmask of shift keys and then determine whether
            // it has at least two bits set via the bitcount trick

    	    // if not enough control things are present we add them
	        // in.  so the user gets the idea.

            if ((wParam & HK_ALT) && !(wParam & HK_CONTROL)
                                  && !(wParam & HK_SHIFT)) {
                break;
            }
            else {
	            wT = (WORD)(wParam & (HK_CONTROL|HK_SHIFT|HK_ALT));
	            if (!wT || !(wT & (wT - 1)))
        		    wParam |= HK_CONTROL | HK_ALT;

	            SetHotKey(hwnd,wParam);
	            break;
            }
        }
        break;

    case WM_SYSKEYUP:
    case WM_CHAR:
    case WM_SYSCHAR:
    case WM_KEYUP:
    if (!LOBYTE((WORD)GetWindowLong(hwnd,HWL_HOTKEY)))
        SetHotKey(hwnd,0);
    break;


    case WM_GETFONT:
        return GetWindowLongPtr(hwnd,HWLP_FONT);

    case WM_SETFONT:
        lParam = GetWindowLongPtr(hwnd,HWLP_FONT);
        SetWindowLongPtr(hwnd,HWLP_FONT,wParam);
        hdc = GetDC(hwnd);
        wParam = (WPARAM) SelectObject(hdc,(HANDLE)wParam);
        GetTextExtentPoint(hdc, TEXT("C"), 1, &size);
        SetWindowLong(hwnd, HWL_CYFONT, size.cy);
        if (wParam){
            SelectObject(hdc,(HANDLE)wParam);
        }
        ReleaseDC(hwnd,hdc);
        InvalidateRect(hwnd,NULL,TRUE);
        return lParam;

    case WM_PAINT:
        PaintHotKey(hwnd);
        break;

    case WM_ERASEBKGND:
        HideCaret(hwnd);
        lParam = DefWindowProc(hwnd,wMsg,wParam,lParam);
        ShowCaret(hwnd);
        return lParam;

    default:
        return DefWindowProc(hwnd,wMsg,wParam,lParam);
    }
    return 0L;
}

/*** RegisterHotKeyClass --
 *
 *
 * BOOL APIENTRY RegisterHotKeyClass(HANDLE hInstance)
 *
 * ENTRY - HWND    HANDLE hInstance
 * EXIT  - BOOL    xxx - returns return code from RegisterClass
 *
 * SYNOPSIS -  ???
 *
 * WARNINGS -  This (under 16)  took hInstance.  Under win32, hInstance is
 *                NULL, cause there is only one instance.
 * EFFECTS  -
 *
 */

BOOL APIENTRY RegisterHotKeyClass(HANDLE hInstance)
{
    WNDCLASS wc;

    wc.style = 0;
    wc.lpfnWndProc = HotKeyWndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = sizeof(HOTKEYWINDOWBYTES);
    wc.hInstance = hInstance;
    wc.hIcon = NULL;
    wc.hCursor = NULL;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszMenuName = NULL;
    wc.lpszClassName = szHotKey;

    return RegisterClass(&wc);
}
