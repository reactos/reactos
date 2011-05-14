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

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(uxtheme);

typedef struct _DRAW_CONTEXT
{
    HWND hWnd;
    HDC hDC;
    HTHEME theme; 
    WINDOWINFO wi;
    HRGN hRgn;
} DRAW_CONTEXT, *PDRAW_CONTEXT;

static void
ThemeInitDrawContext(PDRAW_CONTEXT pcontext,
                     HWND hWnd,
                     HRGN hRgn)
{
    GetWindowInfo(hWnd, &pcontext->wi);
    pcontext->hWnd = hWnd;
    pcontext->theme = OpenThemeData(pcontext->hWnd,  L"WINDOW");

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

/*
    Message handlers
 */

static void 
ThemePaintWindow(PDRAW_CONTEXT pcontext, RECT* prcCurrent)
{
    UNIMPLEMENTED;
}

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
    default:
        return DefWndProc(hWnd, Msg, wParam, lParam);
    }
}
