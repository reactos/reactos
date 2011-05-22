/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS uxtheme.dll
 * FILE:            dll/win32/uxtheme/themehooks.c
 * PURPOSE:         uxtheme non client area management
 * PROGRAMMER:      Giannis Adamopoulos
 */
 
#include <windows.h>
#include "undocuser.h"
#include "vfwmsgs.h"
#include "uxtheme.h"
#include <tmschema.h>

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(uxtheme);

typedef struct _DRAW_CONTEXT
{
    HWND hWnd;
    HDC hDC;
    HTHEME theme; 
    WINDOWINFO wi;
    BOOL Active; /* wi.dwWindowStatus isn't correct for mdi child windows */
    HRGN hRgn;
    int CaptionHeight;
} DRAW_CONTEXT, *PDRAW_CONTEXT;

typedef enum 
{
    CLOSEBUTTON,
    MAXBUTTON,
    MINBUTTON,
    HELPBUTTON
} CAPTIONBUTTON;

/*
The following values specify all possible vutton states
Note that not all of them are documented but it is easy to 
find them by opening a theme file
*/
typedef enum {
    BUTTON_NORMAL = 1 ,
    BUTTON_HOT ,
    BUTTON_PRESSED ,
    BUTTON_DISABLED ,
    BUTTON_INACTIVE
} THEME_BUTTON_STATES;

#define HAS_MENU(hwnd,style)  ((((style) & (WS_CHILD | WS_POPUP)) != WS_CHILD) && GetMenu(hwnd))

#define BUTTON_GAP_SIZE 2

static BOOL 
IsWindowActive(HWND hWnd, DWORD ExStyle)
{
    BOOL ret;

    if (ExStyle & WS_EX_MDICHILD)
    {
        ret = IsChild(GetForegroundWindow(), hWnd);
        if (ret)
            ret = (hWnd == (HWND)SendMessageW(GetParent(hWnd), WM_MDIGETACTIVE, 0, 0));
    }
    else
    {
        ret = (GetForegroundWindow() == hWnd);
    }

    return ret;
}

HICON
UserGetWindowIcon(HWND hwnd)
{
    HICON hIcon = 0;

    SendMessageTimeout(hwnd, WM_GETICON, ICON_SMALL2, 0, SMTO_ABORTIFHUNG, 1000, (PDWORD_PTR)&hIcon);

    if (!hIcon)
        SendMessageTimeout(hwnd, WM_GETICON, ICON_SMALL, 0, SMTO_ABORTIFHUNG, 1000, (PDWORD_PTR)&hIcon);

    if (!hIcon)
        SendMessageTimeout(hwnd, WM_GETICON, ICON_BIG, 0, SMTO_ABORTIFHUNG, 1000, (PDWORD_PTR)&hIcon);

    if (!hIcon)
        hIcon = (HICON)GetClassLong(hwnd, GCL_HICONSM);

    if (!hIcon)
        hIcon = (HICON)GetClassLong(hwnd, GCL_HICON);

    if(!hIcon)
        hIcon = LoadIcon(NULL, IDI_WINLOGO);

    return hIcon;
}

WCHAR *UserGetWindowCaption(HWND hwnd)
{
    INT len = 512;
    WCHAR *text;
    text = (WCHAR*)HeapAlloc(GetProcessHeap(), 0, len  * sizeof(WCHAR));
    if (text) InternalGetWindowText(hwnd, text, len);
    return text;
}

static void 
ThemeDrawTitle(PDRAW_CONTEXT context, RECT* prcCurrent)
{

}

HRESULT WINAPI ThemeDrawCaptionText(HTHEME hTheme, HDC hdc, int iPartId, int iStateId,
                             LPCWSTR pszText, int iCharCount, DWORD dwTextFlags,
                             DWORD dwTextFlags2, const RECT *pRect, BOOL Active)
{
    HRESULT hr;
    HFONT hFont = NULL;
    HGDIOBJ oldFont = NULL;
    LOGFONTW logfont;
    COLORREF textColor;
    COLORREF oldTextColor;
    int oldBkMode;
    RECT rt;
    
    hr = GetThemeSysFont(0,TMT_CAPTIONFONT,&logfont);

    if(SUCCEEDED(hr)) {
        hFont = CreateFontIndirectW(&logfont);
    }
    CopyRect(&rt, pRect);
    if(hFont)
        oldFont = SelectObject(hdc, hFont);
        
    if(dwTextFlags2 & DTT_GRAYED)
        textColor = GetSysColor(COLOR_GRAYTEXT);
    else if (!Active)
        textColor = GetSysColor(COLOR_INACTIVECAPTIONTEXT);
    else
        textColor = GetSysColor(COLOR_CAPTIONTEXT);
    
    oldTextColor = SetTextColor(hdc, textColor);
    oldBkMode = SetBkMode(hdc, TRANSPARENT);
    DrawTextW(hdc, pszText, iCharCount, &rt, dwTextFlags);
    SetBkMode(hdc, oldBkMode);
    SetTextColor(hdc, oldTextColor);

    if(hFont) {
        SelectObject(hdc, oldFont);
        DeleteObject(hFont);
    }
    return S_OK;
}

static void
ThemeInitDrawContext(PDRAW_CONTEXT pcontext,
                     HWND hWnd,
                     HRGN hRgn)
{
    GetWindowInfo(hWnd, &pcontext->wi);
    pcontext->hWnd = hWnd;
    pcontext->Active = IsWindowActive(hWnd, pcontext->wi.dwExStyle);
    pcontext->theme = OpenThemeData(pcontext->hWnd,  L"WINDOW");

    pcontext->CaptionHeight = pcontext->wi.cyWindowBorders;
    pcontext->CaptionHeight += GetSystemMetrics(pcontext->wi.dwExStyle & WS_EX_TOOLWINDOW ? SM_CYSMCAPTION : SM_CYCAPTION );

    if(hRgn <= 0)
    {
        hRgn = CreateRectRgnIndirect(&pcontext->wi.rcWindow);
        pcontext->hRgn = hRgn;
    }
    else
    {
        pcontext->hRgn = 0;
    }

    pcontext->hDC = GetDCEx(hWnd, hRgn, DCX_WINDOW | DCX_INTERSECTRGN | DCX_USESTYLE | DCX_KEEPCLIPRGN);
}

static void
ThemeCleanupDrawContext(PDRAW_CONTEXT pcontext)
{
    ReleaseDC(pcontext->hWnd ,pcontext->hDC);

    CloseThemeData (pcontext->theme);

    if(pcontext->hRgn != NULL)
    {
        DeleteObject(pcontext->hRgn);
    }
}

static void 
ThemeDrawCaptionButton(PDRAW_CONTEXT pcontext, 
                       RECT* prcCurrent, 
                       CAPTIONBUTTON buttonId, 
                       INT iStateId)
{
    RECT rcPart;
    INT ButtonWidth, ButtonHeight, iPartId;
    
    ButtonHeight = GetThemeSysSize(pcontext->theme,  pcontext->wi.dwExStyle & WS_EX_TOOLWINDOW ? SM_CYSMSIZE : SM_CYSIZE);
    ButtonWidth = GetThemeSysSize(pcontext->theme,  pcontext->wi.dwExStyle & WS_EX_TOOLWINDOW ? SM_CXSMSIZE : SM_CXSIZE);

    switch(buttonId)
    {
    case CLOSEBUTTON:
        iPartId = pcontext->wi.dwExStyle & WS_EX_TOOLWINDOW ? WP_SMALLCLOSEBUTTON : WP_CLOSEBUTTON;
        break;

    case MAXBUTTON:
        if (!(pcontext->wi.dwStyle & WS_MAXIMIZEBOX))
        {
            if (!(pcontext->wi.dwStyle & WS_MINIMIZEBOX))
                return;
            else
                iStateId = BUTTON_DISABLED;
        }

        iPartId = pcontext->wi.dwStyle & WS_MAXIMIZE ? WP_RESTOREBUTTON : WP_MAXBUTTON;
        break;

    case MINBUTTON:
        if (!(pcontext->wi.dwStyle & WS_MINIMIZEBOX))
        {
            if (!(pcontext->wi.dwStyle & WS_MAXIMIZEBOX))
                return;
            else
                iStateId = BUTTON_DISABLED;
        }
 
        iPartId = WP_MINBUTTON;
        break;

    default:
        //FIXME: Implement Help Button 
        return;
    }

    ButtonHeight -= 4;
    ButtonWidth -= 4;

    /* Calculate the position */
    rcPart.top = prcCurrent->top;
    rcPart.right = prcCurrent->right;
    rcPart.bottom = rcPart.top + ButtonHeight ;
    rcPart.left = rcPart.right - ButtonWidth ;
    prcCurrent->right -= ButtonWidth + BUTTON_GAP_SIZE;

    DrawThemeBackground(pcontext->theme, pcontext->hDC, iPartId, iStateId, &rcPart, NULL);
}

static DWORD
ThemeGetButtonState(DWORD htCurrect, DWORD htHot, DWORD htDown, BOOL Active)
{
    if (htHot == htCurrect)
        return BUTTON_HOT;
    if (!Active)
        return BUTTON_INACTIVE;
    if (htDown == htCurrect)
        return BUTTON_PRESSED;

    return BUTTON_NORMAL;
}

/* Used only from mouse event handlers */
static void 
ThemeDrawCaptionButtons(PDRAW_CONTEXT pcontext, DWORD htHot, DWORD htDown)
{
    RECT rcCurrent;

    /* Check if the window has caption buttons */
    if (!((pcontext->wi.dwStyle & WS_CAPTION) && (pcontext->wi.dwStyle & WS_SYSMENU)))
        return ;

    rcCurrent.top = rcCurrent.left = 0;
    rcCurrent.right = pcontext->wi.rcWindow.right - pcontext->wi.rcWindow.left;
    rcCurrent.bottom = pcontext->CaptionHeight;
    
    /* Add a padding around the objects of the caption */
    InflateRect(&rcCurrent, -(int)pcontext->wi.cyWindowBorders-BUTTON_GAP_SIZE, 
                            -(int)pcontext->wi.cyWindowBorders-BUTTON_GAP_SIZE);

    /* Draw the buttons */
    ThemeDrawCaptionButton(pcontext, &rcCurrent, CLOSEBUTTON, 
                           ThemeGetButtonState(HTCLOSE, htHot, htDown, pcontext->Active));
    ThemeDrawCaptionButton(pcontext, &rcCurrent, MAXBUTTON,  
                           ThemeGetButtonState(HTMAXBUTTON, htHot, htDown, pcontext->Active));
    ThemeDrawCaptionButton(pcontext, &rcCurrent, MINBUTTON,
                           ThemeGetButtonState(HTMINBUTTON, htHot, htDown, pcontext->Active));
    ThemeDrawCaptionButton(pcontext, &rcCurrent, HELPBUTTON,
                           ThemeGetButtonState(HTHELP, htHot, htDown, pcontext->Active));
}

static void 
ThemeDrawCaption(PDRAW_CONTEXT pcontext, RECT* prcCurrent)
{
    RECT rcPart;
    int iPart, iState;
    HICON hIcon;
    WCHAR *CaptionText;

    hIcon = UserGetWindowIcon(pcontext->hWnd);
    CaptionText = UserGetWindowCaption(pcontext->hWnd);

    /* Get the caption part and state id */
    if (pcontext->wi.dwExStyle & WS_EX_TOOLWINDOW)
        iPart = WP_SMALLCAPTION;
    else if (pcontext->wi.dwStyle & WS_MAXIMIZE)
        iPart = WP_MAXCAPTION;
    else
        iPart = WP_CAPTION;

    iState = pcontext->Active ? FS_ACTIVE : FS_INACTIVE;

    /* Draw the caption background*/
    rcPart = *prcCurrent;
    rcPart.bottom = pcontext->CaptionHeight;
    prcCurrent->top = rcPart.bottom;
    DrawThemeBackground(pcontext->theme, pcontext->hDC,iPart,iState,&rcPart,NULL);

    /* Add a padding around the objects of the caption */
    InflateRect(&rcPart, -(int)pcontext->wi.cyWindowBorders-BUTTON_GAP_SIZE, 
                         -(int)pcontext->wi.cyWindowBorders-BUTTON_GAP_SIZE);

    /* Draw the caption buttons */
    if (pcontext->wi.dwStyle & WS_SYSMENU)
    {
        iState = pcontext->Active ? BUTTON_NORMAL : BUTTON_INACTIVE;

        ThemeDrawCaptionButton(pcontext, &rcPart, CLOSEBUTTON, iState);
        ThemeDrawCaptionButton(pcontext, &rcPart, MAXBUTTON, iState);
        ThemeDrawCaptionButton(pcontext, &rcPart, MINBUTTON, iState);
        ThemeDrawCaptionButton(pcontext, &rcPart, HELPBUTTON, iState);
    }
    
    rcPart.top += 3 ;

    /* Draw the icon */
    if(hIcon && !(pcontext->wi.dwExStyle & WS_EX_TOOLWINDOW))
    {
        int IconHeight = GetSystemMetrics(SM_CYSMICON);
        int IconWidth = GetSystemMetrics(SM_CXSMICON);
        DrawIconEx(pcontext->hDC, rcPart.left, rcPart.top , hIcon, IconWidth, IconHeight, 0, NULL, DI_NORMAL);
        rcPart.left += IconWidth + 4;
    }

    rcPart.right -= 4;

    /* Draw the caption */
    if(CaptionText)
    {
        /*FIXME: Use DrawThemeTextEx*/
        ThemeDrawCaptionText(pcontext->theme, 
                             pcontext->hDC, 
                             iPart,
                             iState, 
                             CaptionText, 
                             lstrlenW(CaptionText), 
                             DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS, 
                             0, 
                             &rcPart , 
                             pcontext->Active);
        HeapFree(GetProcessHeap(), 0, CaptionText);
    }
}

static void 
ThemeDrawBorders(PDRAW_CONTEXT pcontext, RECT* prcCurrent)
{
    RECT rcPart;
    int iState = pcontext->Active ? FS_ACTIVE : FS_INACTIVE;

    /* Draw the bottom border */
    rcPart = *prcCurrent;
    rcPart.top = rcPart.bottom - pcontext->wi.cyWindowBorders;
    prcCurrent->bottom = rcPart.top;
    DrawThemeBackground(pcontext->theme, pcontext->hDC, WP_FRAMEBOTTOM, iState, &rcPart, NULL);

    /* Draw the left border */
    rcPart = *prcCurrent;
    rcPart.right = pcontext->wi.cxWindowBorders ;
    prcCurrent->left = rcPart.right;
    DrawThemeBackground(pcontext->theme, pcontext->hDC,WP_FRAMELEFT, iState, &rcPart, NULL);

    /* Draw the right border */
    rcPart = *prcCurrent;
    rcPart.left = rcPart.right - pcontext->wi.cxWindowBorders;
    prcCurrent->right = rcPart.left;
    DrawThemeBackground(pcontext->theme, pcontext->hDC,WP_FRAMERIGHT, iState, &rcPart, NULL);
}

static void 
DrawClassicFrame(PDRAW_CONTEXT context, RECT* prcCurrent)
{

}

static void
ThemeDrawMenuBar(PDRAW_CONTEXT pcontext, PRECT prcCurrent)
{

}

static void 
ThemeDrawScrollBar(PDRAW_CONTEXT pcontext, INT Bar)
{

}

static void 
ThemePaintWindow(PDRAW_CONTEXT pcontext, RECT* prcCurrent)
{
    if(!(pcontext->wi.dwStyle & WS_VISIBLE))
        return;

    if(pcontext->wi.dwStyle & WS_MINIMIZE)
    {
        ThemeDrawTitle(pcontext, prcCurrent);
        return;
    }

    if((pcontext->wi.dwStyle & WS_CAPTION)==WS_CAPTION)
    {
        ThemeDrawCaption(pcontext, prcCurrent);
        ThemeDrawBorders(pcontext, prcCurrent);
    }
    else
    {
        DrawClassicFrame(pcontext, prcCurrent);
    }

    if(HAS_MENU(pcontext->hWnd, pcontext->wi.dwStyle))
        ThemeDrawMenuBar(pcontext, prcCurrent);
    
    if(pcontext->wi.dwStyle & WS_HSCROLL)
        ThemeDrawScrollBar(pcontext, OBJID_VSCROLL);

    if(pcontext->wi.dwStyle & WS_VSCROLL)
        ThemeDrawScrollBar(pcontext, OBJID_HSCROLL);
}

/*
    Message handlers
 */

static LRESULT 
ThemeHandleNCPaint(HWND hWnd, HRGN hRgn)
{
    DRAW_CONTEXT context;
    RECT rcCurrent;

    ThemeInitDrawContext(&context, hWnd, hRgn);

    rcCurrent = context.wi.rcWindow;
    OffsetRect( &rcCurrent, -context.wi.rcWindow.left, -context.wi.rcWindow.top);

    ThemePaintWindow(&context, &rcCurrent);
    ThemeCleanupDrawContext(&context);

    return 0;
}

static LRESULT 
ThemeHandleNcMouseMove(HWND hWnd, DWORD ht)
{
    DRAW_CONTEXT context;
    TRACKMOUSEEVENT tme;

    tme.cbSize = sizeof(TRACKMOUSEEVENT);
    tme.dwFlags = TME_QUERY;
    tme.hwndTrack  = hWnd;
    TrackMouseEvent(&tme);
        if (tme.dwFlags != (TME_LEAVE | TME_NONCLIENT))
    {
        tme.hwndTrack  = hWnd;
        tme.dwFlags = TME_LEAVE | TME_NONCLIENT;
        TrackMouseEvent(&tme);
    }

    ThemeInitDrawContext(&context, hWnd, 0);
    ThemeDrawCaptionButtons(&context, ht, 0);
    ThemeCleanupDrawContext(&context);

    return 0;
}

static LRESULT 
ThemeHandleNcMouseLeave(HWND hWnd)
{
    DRAW_CONTEXT context;

    ThemeInitDrawContext(&context, hWnd, 0);
    ThemeDrawCaptionButtons(&context, 0, 0);
    ThemeCleanupDrawContext(&context);

    return 0;
}

static VOID
ThemeHandleButton(HWND hWnd, WPARAM wParam)
{
    MSG Msg;
    BOOL Pressed = TRUE, OldState;
    WPARAM SCMsg, ht;
    ULONG Style;
    DRAW_CONTEXT context;

    Style = GetWindowLongW(hWnd, GWL_STYLE);
    switch (wParam)
    {
        case HTCLOSE:
            if (!(Style & WS_SYSMENU))
                return;
            SCMsg = SC_CLOSE;
            break;
        case HTMINBUTTON:
            if (!(Style & WS_MINIMIZEBOX))
                return;
            SCMsg = ((Style & WS_MINIMIZE) ? SC_RESTORE : SC_MINIMIZE);
            break;
        case HTMAXBUTTON:
            if (!(Style & WS_MAXIMIZEBOX))
                return;
            SCMsg = ((Style & WS_MAXIMIZE) ? SC_RESTORE : SC_MAXIMIZE);
            break;
        default :
            return;
    }

    ThemeInitDrawContext(&context, hWnd, 0);
    ThemeDrawCaptionButtons(&context, 0,  wParam);

    SetCapture(hWnd);

    for (;;)
    {
        if (GetMessageW(&Msg, 0, WM_MOUSEFIRST, WM_MOUSELAST) <= 0)
            break;

        if (Msg.message == WM_LBUTTONUP)
            break;

        if (Msg.message != WM_MOUSEMOVE)
            continue;

        OldState = Pressed;
        ht = SendMessage(hWnd, WM_NCHITTEST, 0, MAKELPARAM(Msg.pt.x, Msg.pt.y));
        Pressed = (ht == wParam);

        ThemeDrawCaptionButtons(&context, 0, Pressed ? wParam: 0);
    }

    ThemeDrawCaptionButtons(&context, 0, 0);
    ThemeCleanupDrawContext(&context);

    ReleaseCapture();

    if (Pressed)
        SendMessageW(hWnd, WM_SYSCOMMAND, SCMsg, 0);
}

LRESULT CALLBACK 
ThemeWndProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam, WNDPROC DefWndProc)
{
    switch(Msg)
    {
    case WM_NCPAINT:
        return ThemeHandleNCPaint(hWnd, (HRGN)wParam);
    case WM_NCACTIVATE:
        ThemeHandleNCPaint(hWnd, (HRGN)1);
        return TRUE;
    case WM_NCMOUSEMOVE:
        return ThemeHandleNcMouseMove(hWnd, wParam);
    case WM_NCMOUSELEAVE:
        return ThemeHandleNcMouseLeave(hWnd);
    case WM_NCLBUTTONDOWN:
        switch (wParam)
        {
            case HTMINBUTTON:
            case HTMAXBUTTON:
            case HTCLOSE:
            {
                ThemeHandleButton(hWnd, wParam);
                return 0;
            }
            default:
                return DefWndProc(hWnd, Msg, wParam, lParam);
        }
    default:
        return DefWndProc(hWnd, Msg, wParam, lParam);
    }
}
