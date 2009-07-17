/*
 * 16-bit windowing functions
 *
 * Copyright 2001 Alexandre Julliard
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

#include "wine/winuser16.h"
#include "wownt32.h"
#include "win.h"
#include "user_private.h"

/* handle <--> handle16 conversions */
#define HANDLE_16(h32)		(LOWORD(h32))
#define HANDLE_32(h16)		((HANDLE)(ULONG_PTR)(h16))

static HWND16 hwndSysModal;

struct wnd_enum_info
{
    WNDENUMPROC16 proc;
    LPARAM        param;
};

/* callback for 16-bit window enumeration functions */
static BOOL CALLBACK wnd_enum_callback( HWND hwnd, LPARAM param )
{
    const struct wnd_enum_info *info = (struct wnd_enum_info *)param;
    WORD args[3];
    DWORD ret;

    args[2] = HWND_16(hwnd);
    args[1] = HIWORD(info->param);
    args[0] = LOWORD(info->param);
    WOWCallback16Ex( (DWORD)info->proc, WCB16_PASCAL, sizeof(args), args, &ret );
    return LOWORD(ret);
}

/* convert insert after window handle to 32-bit */
static inline HWND full_insert_after_hwnd( HWND16 hwnd )
{
    HWND ret = WIN_Handle32( hwnd );
    if (ret == (HWND)0xffff) ret = HWND_TOPMOST;
    return ret;
}

/**************************************************************************
 *              MessageBox   (USER.1)
 */
INT16 WINAPI MessageBox16( HWND16 hwnd, LPCSTR text, LPCSTR title, UINT16 type )
{
    return MessageBoxA( WIN_Handle32(hwnd), text, title, type );
}


/***********************************************************************
 *		SetTimer (USER.10)
 */
UINT16 WINAPI SetTimer16( HWND16 hwnd, UINT16 id, UINT16 timeout, TIMERPROC16 proc )
{
    TIMERPROC proc32 = (TIMERPROC)WINPROC_AllocProc16( (WNDPROC16)proc );
    return SetTimer( WIN_Handle32(hwnd), id, timeout, proc32 );
}


/***********************************************************************
 *		SetSystemTimer (USER.11)
 */
UINT16 WINAPI SetSystemTimer16( HWND16 hwnd, UINT16 id, UINT16 timeout, TIMERPROC16 proc )
{
    TIMERPROC proc32 = (TIMERPROC)WINPROC_AllocProc16( (WNDPROC16)proc );
    return SetSystemTimer( WIN_Handle32(hwnd), id, timeout, proc32 );
}


/**************************************************************************
 *              KillTimer   (USER.12)
 */
BOOL16 WINAPI KillTimer16( HWND16 hwnd, UINT16 id )
{
    return KillTimer( WIN_Handle32(hwnd), id );
}


/**************************************************************************
 *              SetCapture   (USER.18)
 */
HWND16 WINAPI SetCapture16( HWND16 hwnd )
{
    return HWND_16( SetCapture( WIN_Handle32(hwnd) ));
}


/**************************************************************************
 *              ReleaseCapture   (USER.19)
 */
BOOL16 WINAPI ReleaseCapture16(void)
{
    return ReleaseCapture();
}


/**************************************************************************
 *              SetFocus   (USER.22)
 */
HWND16 WINAPI SetFocus16( HWND16 hwnd )
{
    return HWND_16( SetFocus( WIN_Handle32(hwnd) ));
}


/**************************************************************************
 *              GetFocus   (USER.23)
 */
HWND16 WINAPI GetFocus16(void)
{
    return HWND_16( GetFocus() );
}


/**************************************************************************
 *              RemoveProp   (USER.24)
 */
HANDLE16 WINAPI RemoveProp16( HWND16 hwnd, LPCSTR str )
{
    return HANDLE_16(RemovePropA( WIN_Handle32(hwnd), str ));
}


/**************************************************************************
 *              GetProp   (USER.25)
 */
HANDLE16 WINAPI GetProp16( HWND16 hwnd, LPCSTR str )
{
    return HANDLE_16(GetPropA( WIN_Handle32(hwnd), str ));
}


/**************************************************************************
 *              SetProp   (USER.26)
 */
BOOL16 WINAPI SetProp16( HWND16 hwnd, LPCSTR str, HANDLE16 handle )
{
    return SetPropA( WIN_Handle32(hwnd), str, HANDLE_32(handle) );
}


/**************************************************************************
 *              ClientToScreen   (USER.28)
 */
void WINAPI ClientToScreen16( HWND16 hwnd, LPPOINT16 lppnt )
{
    MapWindowPoints16( hwnd, 0, lppnt, 1 );
}


/**************************************************************************
 *              ScreenToClient   (USER.29)
 */
void WINAPI ScreenToClient16( HWND16 hwnd, LPPOINT16 lppnt )
{
    MapWindowPoints16( 0, hwnd, lppnt, 1 );
}


/**************************************************************************
 *              WindowFromPoint   (USER.30)
 */
HWND16 WINAPI WindowFromPoint16( POINT16 pt )
{
    POINT pt32;

    pt32.x = pt.x;
    pt32.y = pt.y;
    return HWND_16( WindowFromPoint( pt32 ) );
}


/**************************************************************************
 *              IsIconic   (USER.31)
 */
BOOL16 WINAPI IsIconic16(HWND16 hwnd)
{
    return IsIconic( WIN_Handle32(hwnd) );
}


/**************************************************************************
 *              GetWindowRect   (USER.32)
 */
void WINAPI GetWindowRect16( HWND16 hwnd, LPRECT16 rect )
{
    RECT rect32;

    GetWindowRect( WIN_Handle32(hwnd), &rect32 );
    rect->left   = rect32.left;
    rect->top    = rect32.top;
    rect->right  = rect32.right;
    rect->bottom = rect32.bottom;
}


/**************************************************************************
 *              GetClientRect   (USER.33)
 */
void WINAPI GetClientRect16( HWND16 hwnd, LPRECT16 rect )
{
    RECT rect32;

    GetClientRect( WIN_Handle32(hwnd), &rect32 );
    rect->left   = rect32.left;
    rect->top    = rect32.top;
    rect->right  = rect32.right;
    rect->bottom = rect32.bottom;
}


/**************************************************************************
 *              EnableWindow   (USER.34)
 */
BOOL16 WINAPI EnableWindow16( HWND16 hwnd, BOOL16 enable )
{
    return EnableWindow( WIN_Handle32(hwnd), enable );
}


/**************************************************************************
 *              IsWindowEnabled   (USER.35)
 */
BOOL16 WINAPI IsWindowEnabled16(HWND16 hwnd)
{
    return IsWindowEnabled( WIN_Handle32(hwnd) );
}


/**************************************************************************
 *              GetWindowText   (USER.36)
 */
INT16 WINAPI GetWindowText16( HWND16 hwnd, SEGPTR lpString, INT16 nMaxCount )
{
    return SendMessage16( hwnd, WM_GETTEXT, nMaxCount, lpString );
}


/**************************************************************************
 *              SetWindowText   (USER.37)
 */
BOOL16 WINAPI SetWindowText16( HWND16 hwnd, SEGPTR lpString )
{
    return SendMessage16( hwnd, WM_SETTEXT, 0, (LPARAM)lpString );
}


/**************************************************************************
 *              GetWindowTextLength   (USER.38)
 */
INT16 WINAPI GetWindowTextLength16( HWND16 hwnd )
{
    return SendMessage16( hwnd, WM_GETTEXTLENGTH, 0, 0 );
}


/***********************************************************************
 *		BeginPaint (USER.39)
 */
HDC16 WINAPI BeginPaint16( HWND16 hwnd, LPPAINTSTRUCT16 lps )
{
    PAINTSTRUCT ps;

    BeginPaint( WIN_Handle32(hwnd), &ps );
    lps->hdc            = HDC_16(ps.hdc);
    lps->fErase         = ps.fErase;
    lps->rcPaint.top    = ps.rcPaint.top;
    lps->rcPaint.left   = ps.rcPaint.left;
    lps->rcPaint.right  = ps.rcPaint.right;
    lps->rcPaint.bottom = ps.rcPaint.bottom;
    lps->fRestore       = ps.fRestore;
    lps->fIncUpdate     = ps.fIncUpdate;
    return lps->hdc;
}


/***********************************************************************
 *		EndPaint (USER.40)
 */
BOOL16 WINAPI EndPaint16( HWND16 hwnd, const PAINTSTRUCT16* lps )
{
    PAINTSTRUCT ps;

    ps.hdc = HDC_32(lps->hdc);
    return EndPaint( WIN_Handle32(hwnd), &ps );
}


/**************************************************************************
 *              ShowWindow   (USER.42)
 */
BOOL16 WINAPI ShowWindow16( HWND16 hwnd, INT16 cmd )
{
    return ShowWindow( WIN_Handle32(hwnd), cmd );
}


/**************************************************************************
 *              CloseWindow   (USER.43)
 */
BOOL16 WINAPI CloseWindow16( HWND16 hwnd )
{
    return CloseWindow( WIN_Handle32(hwnd) );
}


/**************************************************************************
 *              OpenIcon   (USER.44)
 */
BOOL16 WINAPI OpenIcon16( HWND16 hwnd )
{
    return OpenIcon( WIN_Handle32(hwnd) );
}


/**************************************************************************
 *              BringWindowToTop   (USER.45)
 */
BOOL16 WINAPI BringWindowToTop16( HWND16 hwnd )
{
    return BringWindowToTop( WIN_Handle32(hwnd) );
}


/**************************************************************************
 *              GetParent   (USER.46)
 */
HWND16 WINAPI GetParent16( HWND16 hwnd )
{
    return HWND_16( GetParent( WIN_Handle32(hwnd) ));
}


/**************************************************************************
 *              IsWindow   (USER.47)
 */
BOOL16 WINAPI IsWindow16( HWND16 hwnd )
{
    STACK16FRAME *frame = MapSL( (SEGPTR)NtCurrentTeb()->WOW32Reserved );
    frame->es = USER_HeapSel;
    /* don't use WIN_Handle32 here, we don't care about the full handle */
    return IsWindow( HWND_32(hwnd) );
}


/**************************************************************************
 *              IsChild   (USER.48)
 */
BOOL16 WINAPI IsChild16( HWND16 parent, HWND16 child )
{
    return IsChild( WIN_Handle32(parent), WIN_Handle32(child) );
}


/**************************************************************************
 *              IsWindowVisible   (USER.49)
 */
BOOL16 WINAPI IsWindowVisible16( HWND16 hwnd )
{
    return IsWindowVisible( WIN_Handle32(hwnd) );
}


/**************************************************************************
 *              FindWindow   (USER.50)
 */
HWND16 WINAPI FindWindow16( LPCSTR className, LPCSTR title )
{
    return HWND_16( FindWindowA( className, title ));
}


/**************************************************************************
 *              DestroyWindow   (USER.53)
 */
BOOL16 WINAPI DestroyWindow16( HWND16 hwnd )
{
    return DestroyWindow( WIN_Handle32(hwnd) );
}


/*******************************************************************
 *           EnumWindows   (USER.54)
 */
BOOL16 WINAPI EnumWindows16( WNDENUMPROC16 func, LPARAM lParam )
{
    struct wnd_enum_info info;

    info.proc  = func;
    info.param = lParam;
    return EnumWindows( wnd_enum_callback, (LPARAM)&info );
}


/**********************************************************************
 *           EnumChildWindows   (USER.55)
 */
BOOL16 WINAPI EnumChildWindows16( HWND16 parent, WNDENUMPROC16 func, LPARAM lParam )
{
    struct wnd_enum_info info;

    info.proc  = func;
    info.param = lParam;
    return EnumChildWindows( WIN_Handle32(parent), wnd_enum_callback, (LPARAM)&info );
}


/**************************************************************************
 *              MoveWindow   (USER.56)
 */
BOOL16 WINAPI MoveWindow16( HWND16 hwnd, INT16 x, INT16 y, INT16 cx, INT16 cy, BOOL16 repaint )
{
    return MoveWindow( WIN_Handle32(hwnd), x, y, cx, cy, repaint );
}


/***********************************************************************
 *		RegisterClass (USER.57)
 */
ATOM WINAPI RegisterClass16( const WNDCLASS16 *wc )
{
    WNDCLASSEX16 wcex;

    wcex.cbSize        = sizeof(wcex);
    wcex.style         = wc->style;
    wcex.lpfnWndProc   = wc->lpfnWndProc;
    wcex.cbClsExtra    = wc->cbClsExtra;
    wcex.cbWndExtra    = wc->cbWndExtra;
    wcex.hInstance     = wc->hInstance;
    wcex.hIcon         = wc->hIcon;
    wcex.hCursor       = wc->hCursor;
    wcex.hbrBackground = wc->hbrBackground;
    wcex.lpszMenuName  = wc->lpszMenuName;
    wcex.lpszClassName = wc->lpszClassName;
    wcex.hIconSm       = 0;
    return RegisterClassEx16( &wcex );
}


/**************************************************************************
 *              GetClassName   (USER.58)
 */
INT16 WINAPI GetClassName16( HWND16 hwnd, LPSTR buffer, INT16 count )
{
    return GetClassNameA( WIN_Handle32(hwnd), buffer, count );
}


/**************************************************************************
 *              SetActiveWindow   (USER.59)
 */
HWND16 WINAPI SetActiveWindow16( HWND16 hwnd )
{
    return HWND_16( SetActiveWindow( WIN_Handle32(hwnd) ));
}


/**************************************************************************
 *              GetActiveWindow   (USER.60)
 */
HWND16 WINAPI GetActiveWindow16(void)
{
    return HWND_16( GetActiveWindow() );
}


/**************************************************************************
 *              ScrollWindow   (USER.61)
 */
void WINAPI ScrollWindow16( HWND16 hwnd, INT16 dx, INT16 dy, const RECT16 *rect,
                            const RECT16 *clipRect )
{
    RECT rect32, clipRect32;

    if (rect)
    {
        rect32.left   = rect->left;
        rect32.top    = rect->top;
        rect32.right  = rect->right;
        rect32.bottom = rect->bottom;
    }
    if (clipRect)
    {
        clipRect32.left   = clipRect->left;
        clipRect32.top    = clipRect->top;
        clipRect32.right  = clipRect->right;
        clipRect32.bottom = clipRect->bottom;
    }
    ScrollWindow( WIN_Handle32(hwnd), dx, dy, rect ? &rect32 : NULL,
                  clipRect ? &clipRect32 : NULL );
}


/**************************************************************************
 *              SetScrollPos   (USER.62)
 */
INT16 WINAPI SetScrollPos16( HWND16 hwnd, INT16 nBar, INT16 nPos, BOOL16 redraw )
{
    return SetScrollPos( WIN_Handle32(hwnd), nBar, nPos, redraw );
}


/**************************************************************************
 *              GetScrollPos   (USER.63)
 */
INT16 WINAPI GetScrollPos16( HWND16 hwnd, INT16 nBar )
{
    return GetScrollPos( WIN_Handle32(hwnd), nBar );
}


/**************************************************************************
 *              SetScrollRange   (USER.64)
 */
void WINAPI SetScrollRange16( HWND16 hwnd, INT16 nBar, INT16 MinVal, INT16 MaxVal, BOOL16 redraw )
{
    /* Invalid range -> range is set to (0,0) */
    if ((INT)MaxVal - (INT)MinVal > 0x7fff) MinVal = MaxVal = 0;
    SetScrollRange( WIN_Handle32(hwnd), nBar, MinVal, MaxVal, redraw );
}


/**************************************************************************
 *              GetScrollRange   (USER.65)
 */
BOOL16 WINAPI GetScrollRange16( HWND16 hwnd, INT16 nBar, LPINT16 lpMin, LPINT16 lpMax)
{
    INT min, max;
    BOOL ret = GetScrollRange( WIN_Handle32(hwnd), nBar, &min, &max );
    if (lpMin) *lpMin = min;
    if (lpMax) *lpMax = max;
    return ret;
}


/**************************************************************************
 *              GetDC   (USER.66)
 */
HDC16 WINAPI GetDC16( HWND16 hwnd )
{
    return HDC_16(GetDC( WIN_Handle32(hwnd) ));
}


/**************************************************************************
 *              GetWindowDC   (USER.67)
 */
HDC16 WINAPI GetWindowDC16( HWND16 hwnd )
{
    return GetDCEx16( hwnd, 0, DCX_USESTYLE | DCX_WINDOW );
}


/**************************************************************************
 *              ReleaseDC   (USER.68)
 */
INT16 WINAPI ReleaseDC16( HWND16 hwnd, HDC16 hdc )
{
    return (INT16)ReleaseDC( WIN_Handle32(hwnd), HDC_32(hdc) );
}


/**************************************************************************
 *              FlashWindow   (USER.105)
 */
BOOL16 WINAPI FlashWindow16( HWND16 hwnd, BOOL16 bInvert )
{
    return FlashWindow( WIN_Handle32(hwnd), bInvert );
}


/**************************************************************************
 *              WindowFromDC   (USER.117)
 */
HWND16 WINAPI WindowFromDC16( HDC16 hDC )
{
    return HWND_16( WindowFromDC( HDC_32(hDC) ) );
}


/**************************************************************************
 *              UpdateWindow   (USER.124)
 */
void WINAPI UpdateWindow16( HWND16 hwnd )
{
    RedrawWindow16( hwnd, NULL, 0, RDW_UPDATENOW | RDW_ALLCHILDREN );
}


/**************************************************************************
 *              InvalidateRect   (USER.125)
 */
void WINAPI InvalidateRect16( HWND16 hwnd, const RECT16 *rect, BOOL16 erase )
{
    RedrawWindow16( hwnd, rect, 0, RDW_INVALIDATE | (erase ? RDW_ERASE : 0) );
}


/**************************************************************************
 *              InvalidateRgn   (USER.126)
 */
void WINAPI InvalidateRgn16( HWND16 hwnd, HRGN16 hrgn, BOOL16 erase )
{
    RedrawWindow16( hwnd, NULL, hrgn, RDW_INVALIDATE | (erase ? RDW_ERASE : 0) );
}


/**************************************************************************
 *              ValidateRect   (USER.127)
 */
void WINAPI ValidateRect16( HWND16 hwnd, const RECT16 *rect )
{
    RedrawWindow16( hwnd, rect, 0, RDW_VALIDATE | RDW_NOCHILDREN );
}


/**************************************************************************
 *              ValidateRgn   (USER.128)
 */
void WINAPI ValidateRgn16( HWND16 hwnd, HRGN16 hrgn )
{
    RedrawWindow16( hwnd, NULL, hrgn, RDW_VALIDATE | RDW_NOCHILDREN );
}


/**************************************************************************
 *              GetClassWord   (USER.129)
 */
WORD WINAPI GetClassWord16( HWND16 hwnd, INT16 offset )
{
    return GetClassWord( WIN_Handle32(hwnd), offset );
}


/**************************************************************************
 *              SetClassWord   (USER.130)
 */
WORD WINAPI SetClassWord16( HWND16 hwnd, INT16 offset, WORD newval )
{
    return SetClassWord( WIN_Handle32(hwnd), offset, newval );
}


/***********************************************************************
 *		GetClassLong (USER.131)
 */
LONG WINAPI GetClassLong16( HWND16 hwnd16, INT16 offset )
{
    LONG_PTR ret = GetClassLongA( WIN_Handle32(hwnd16), offset );

    switch( offset )
    {
    case GCLP_WNDPROC:
        return (LONG_PTR)WINPROC_GetProc16( (WNDPROC)ret, FALSE );
    case GCLP_MENUNAME:
        return MapLS( (void *)ret );  /* leak */
    default:
        return ret;
    }
}


/***********************************************************************
 *		SetClassLong (USER.132)
 */
LONG WINAPI SetClassLong16( HWND16 hwnd16, INT16 offset, LONG newval )
{
    switch( offset )
    {
    case GCLP_WNDPROC:
        {
            WNDPROC new_proc = WINPROC_AllocProc16( (WNDPROC16)newval );
            WNDPROC old_proc = (WNDPROC)SetClassLongA( WIN_Handle32(hwnd16), offset, (LONG_PTR)new_proc );
            return (LONG)WINPROC_GetProc16( old_proc, FALSE );
        }
    case GCLP_MENUNAME:
        newval = (LONG)MapSL( newval );
        /* fall through */
    default:
        return SetClassLongA( WIN_Handle32(hwnd16), offset, newval );
    }
}


/**************************************************************************
 *              GetWindowWord   (USER.133)
 */
WORD WINAPI GetWindowWord16( HWND16 hwnd, INT16 offset )
{
    return GetWindowWord( WIN_Handle32(hwnd), offset );
}


/**************************************************************************
 *              SetWindowWord   (USER.134)
 */
WORD WINAPI SetWindowWord16( HWND16 hwnd, INT16 offset, WORD newval )
{
    return SetWindowWord( WIN_Handle32(hwnd), offset, newval );
}


/**************************************************************************
 *              OpenClipboard   (USER.137)
 */
BOOL16 WINAPI OpenClipboard16( HWND16 hwnd )
{
    return OpenClipboard( WIN_Handle32(hwnd) );
}


/**************************************************************************
 *              GetClipboardOwner   (USER.140)
 */
HWND16 WINAPI GetClipboardOwner16(void)
{
    return HWND_16( GetClipboardOwner() );
}


/**************************************************************************
 *              SetClipboardViewer   (USER.147)
 */
HWND16 WINAPI SetClipboardViewer16( HWND16 hwnd )
{
    return HWND_16( SetClipboardViewer( WIN_Handle32(hwnd) ));
}


/**************************************************************************
 *              GetClipboardViewer   (USER.148)
 */
HWND16 WINAPI GetClipboardViewer16(void)
{
    return HWND_16( GetClipboardViewer() );
}


/**************************************************************************
 *              ChangeClipboardChain   (USER.149)
 */
BOOL16 WINAPI ChangeClipboardChain16(HWND16 hwnd, HWND16 hwndNext)
{
    return ChangeClipboardChain( WIN_Handle32(hwnd), WIN_Handle32(hwndNext) );
}


/**************************************************************************
 *              GetSystemMenu   (USER.156)
 */
HMENU16 WINAPI GetSystemMenu16( HWND16 hwnd, BOOL16 revert )
{
    return HMENU_16(GetSystemMenu( WIN_Handle32(hwnd), revert ));
}


/**************************************************************************
 *              GetMenu   (USER.157)
 */
HMENU16 WINAPI GetMenu16( HWND16 hwnd )
{
    return HMENU_16(GetMenu( WIN_Handle32(hwnd) ));
}


/**************************************************************************
 *              SetMenu   (USER.158)
 */
BOOL16 WINAPI SetMenu16( HWND16 hwnd, HMENU16 hMenu )
{
    return SetMenu( WIN_Handle32(hwnd), HMENU_32(hMenu) );
}


/**************************************************************************
 *              DrawMenuBar   (USER.160)
 */
void WINAPI DrawMenuBar16( HWND16 hwnd )
{
    DrawMenuBar( WIN_Handle32(hwnd) );
}


/**************************************************************************
 *              HiliteMenuItem   (USER.162)
 */
BOOL16 WINAPI HiliteMenuItem16( HWND16 hwnd, HMENU16 hMenu, UINT16 id, UINT16 wHilite )
{
    return HiliteMenuItem( WIN_Handle32(hwnd), HMENU_32(hMenu), id, wHilite );
}


/**************************************************************************
 *              CreateCaret   (USER.163)
 */
void WINAPI CreateCaret16( HWND16 hwnd, HBITMAP16 bitmap, INT16 width, INT16 height )
{
    CreateCaret( WIN_Handle32(hwnd), HBITMAP_32(bitmap), width, height );
}


/*****************************************************************
 *		DestroyCaret (USER.164)
 */
void WINAPI DestroyCaret16(void)
{
    DestroyCaret();
}


/*****************************************************************
 *		SetCaretPos (USER.165)
 */
void WINAPI SetCaretPos16( INT16 x, INT16 y )
{
    SetCaretPos( x, y );
}


/**************************************************************************
 *              HideCaret   (USER.166)
 */
void WINAPI HideCaret16( HWND16 hwnd )
{
    HideCaret( WIN_Handle32(hwnd) );
}


/**************************************************************************
 *              ShowCaret   (USER.167)
 */
void WINAPI ShowCaret16( HWND16 hwnd )
{
    ShowCaret( WIN_Handle32(hwnd) );
}


/*****************************************************************
 *		SetCaretBlinkTime (USER.168)
 */
void WINAPI SetCaretBlinkTime16( UINT16 msecs )
{
    SetCaretBlinkTime( msecs );
}


/*****************************************************************
 *		GetCaretBlinkTime (USER.169)
 */
UINT16 WINAPI GetCaretBlinkTime16(void)
{
    return GetCaretBlinkTime();
}


/**************************************************************************
 *              ArrangeIconicWindows   (USER.170)
 */
UINT16 WINAPI ArrangeIconicWindows16( HWND16 parent)
{
    return ArrangeIconicWindows( WIN_Handle32(parent) );
}


/**************************************************************************
 *              SwitchToThisWindow   (USER.172)
 */
void WINAPI SwitchToThisWindow16( HWND16 hwnd, BOOL16 restore )
{
    SwitchToThisWindow( WIN_Handle32(hwnd), restore );
}


/**************************************************************************
 *              KillSystemTimer   (USER.182)
 */
BOOL16 WINAPI KillSystemTimer16( HWND16 hwnd, UINT16 id )
{
    return KillSystemTimer( WIN_Handle32(hwnd), id );
}


/*****************************************************************
 *		GetCaretPos (USER.183)
 */
void WINAPI GetCaretPos16( LPPOINT16 pt16 )
{
    POINT pt;
    if (GetCaretPos( &pt ))
    {
        pt16->x = pt.x;
        pt16->y = pt.y;
    }
}


/**************************************************************************
 *              SetSysModalWindow   (USER.188)
 */
HWND16 WINAPI SetSysModalWindow16( HWND16 hwnd )
{
    HWND16 old = hwndSysModal;
    hwndSysModal = hwnd;
    return old;
}


/**************************************************************************
 *              GetSysModalWindow   (USER.189)
 */
HWND16 WINAPI GetSysModalWindow16(void)
{
    return hwndSysModal;
}


/**************************************************************************
 *              GetUpdateRect   (USER.190)
 */
BOOL16 WINAPI GetUpdateRect16( HWND16 hwnd, LPRECT16 rect, BOOL16 erase )
{
    RECT r;
    BOOL16 ret;

    if (!rect) return GetUpdateRect( WIN_Handle32(hwnd), NULL, erase );
    ret = GetUpdateRect( WIN_Handle32(hwnd), &r, erase );
    rect->left   = r.left;
    rect->top    = r.top;
    rect->right  = r.right;
    rect->bottom = r.bottom;
    return ret;
}


/**************************************************************************
 *              ChildWindowFromPoint   (USER.191)
 */
HWND16 WINAPI ChildWindowFromPoint16( HWND16 hwndParent, POINT16 pt )
{
    POINT pt32;

    pt32.x = pt.x;
    pt32.y = pt.y;
    return HWND_16( ChildWindowFromPoint( WIN_Handle32(hwndParent), pt32 ));
}


/***********************************************************************
 *		CascadeChildWindows (USER.198)
 */
void WINAPI CascadeChildWindows16( HWND16 parent, WORD action )
{
    CascadeWindows( WIN_Handle32(parent), action, NULL, 0, NULL );
}


/***********************************************************************
 *		TileChildWindows (USER.199)
 */
void WINAPI TileChildWindows16( HWND16 parent, WORD action )
{
    TileWindows( WIN_Handle32(parent), action, NULL, 0, NULL );
}


/***********************************************************************
 *		GetWindowTask   (USER.224)
 */
HTASK16 WINAPI GetWindowTask16( HWND16 hwnd )
{
    DWORD tid = GetWindowThreadProcessId( HWND_32(hwnd), NULL );
    if (!tid) return 0;
    return HTASK_16(tid);
}

/**********************************************************************
 *		EnumTaskWindows   (USER.225)
 */
BOOL16 WINAPI EnumTaskWindows16( HTASK16 hTask, WNDENUMPROC16 func, LPARAM lParam )
{
    struct wnd_enum_info info;
    DWORD tid = HTASK_32( hTask );

    if (!tid) return FALSE;
    info.proc  = func;
    info.param = lParam;
    return EnumThreadWindows( tid, wnd_enum_callback, (LPARAM)&info );
}


/**************************************************************************
 *              GetTopWindow   (USER.229)
 */
HWND16 WINAPI GetTopWindow16( HWND16 hwnd )
{
    return HWND_16( GetTopWindow( WIN_Handle32(hwnd) ));
}


/**************************************************************************
 *              GetNextWindow   (USER.230)
 */
HWND16 WINAPI GetNextWindow16( HWND16 hwnd, WORD flag )
{
    if ((flag != GW_HWNDNEXT) && (flag != GW_HWNDPREV)) return 0;
    return GetWindow16( hwnd, flag );
}


/**************************************************************************
 *              SetWindowPos   (USER.232)
 */
BOOL16 WINAPI SetWindowPos16( HWND16 hwnd, HWND16 hwndInsertAfter,
                              INT16 x, INT16 y, INT16 cx, INT16 cy, WORD flags)
{
    return SetWindowPos( WIN_Handle32(hwnd), full_insert_after_hwnd(hwndInsertAfter),
                         x, y, cx, cy, flags );
}


/**************************************************************************
 *              SetParent   (USER.233)
 */
HWND16 WINAPI SetParent16( HWND16 hwndChild, HWND16 hwndNewParent )
{
    return HWND_16( SetParent( WIN_Handle32(hwndChild), WIN_Handle32(hwndNewParent) ));
}


/**************************************************************************
 *              GetCapture   (USER.236)
 */
HWND16 WINAPI GetCapture16(void)
{
    return HWND_16( GetCapture() );
}


/**************************************************************************
 *              GetUpdateRgn   (USER.237)
 */
INT16 WINAPI GetUpdateRgn16( HWND16 hwnd, HRGN16 hrgn, BOOL16 erase )
{
    return GetUpdateRgn( WIN_Handle32(hwnd), HRGN_32(hrgn), erase );
}


/**************************************************************************
 *              ExcludeUpdateRgn   (USER.238)
 */
INT16 WINAPI ExcludeUpdateRgn16( HDC16 hdc, HWND16 hwnd )
{
    return ExcludeUpdateRgn( HDC_32(hdc), WIN_Handle32(hwnd) );
}


/**************************************************************************
 *              GetOpenClipboardWindow   (USER.248)
 */
HWND16 WINAPI GetOpenClipboardWindow16(void)
{
    return HWND_16( GetOpenClipboardWindow() );
}


/**************************************************************************
 *              BeginDeferWindowPos   (USER.259)
 */
HDWP16 WINAPI BeginDeferWindowPos16( INT16 count )
{
    return HDWP_16(BeginDeferWindowPos( count ));
}


/**************************************************************************
 *              DeferWindowPos   (USER.260)
 */
HDWP16 WINAPI DeferWindowPos16( HDWP16 hdwp, HWND16 hwnd, HWND16 hwndAfter,
                                INT16 x, INT16 y, INT16 cx, INT16 cy,
                                UINT16 flags )
{
    return HDWP_16(DeferWindowPos( HDWP_32(hdwp), WIN_Handle32(hwnd),
		   full_insert_after_hwnd(hwndAfter), x, y, cx, cy, flags ));
}


/**************************************************************************
 *              EndDeferWindowPos   (USER.261)
 */
BOOL16 WINAPI EndDeferWindowPos16( HDWP16 hdwp )
{
    return EndDeferWindowPos(HDWP_32(hdwp));
}


/**************************************************************************
 *              GetWindow   (USER.262)
 */
HWND16 WINAPI GetWindow16( HWND16 hwnd, WORD rel )
{
    return HWND_16( GetWindow( WIN_Handle32(hwnd), rel ) );
}


/**************************************************************************
 *              ShowOwnedPopups   (USER.265)
 */
void WINAPI ShowOwnedPopups16( HWND16 owner, BOOL16 fShow )
{
    ShowOwnedPopups( WIN_Handle32(owner), fShow );
}


/**************************************************************************
 *              ShowScrollBar   (USER.267)
 */
void WINAPI ShowScrollBar16( HWND16 hwnd, INT16 nBar, BOOL16 fShow )
{
    ShowScrollBar( WIN_Handle32(hwnd), nBar, fShow );
}


/**************************************************************************
 *              IsZoomed   (USER.272)
 */
BOOL16 WINAPI IsZoomed16(HWND16 hwnd)
{
    return IsZoomed( WIN_Handle32(hwnd) );
}


/**************************************************************************
 *              GetDlgCtrlID   (USER.277)
 */
INT16 WINAPI GetDlgCtrlID16( HWND16 hwnd )
{
    return GetDlgCtrlID( WIN_Handle32(hwnd) );
}


/**************************************************************************
 *              GetDesktopHwnd   (USER.278)
 *
 * Exactly the same thing as GetDesktopWindow(), but not documented.
 * Don't ask me why...
 */
HWND16 WINAPI GetDesktopHwnd16(void)
{
    return GetDesktopWindow16();
}


/**************************************************************************
 *              SetSystemMenu   (USER.280)
 */
BOOL16 WINAPI SetSystemMenu16( HWND16 hwnd, HMENU16 hMenu )
{
    return SetSystemMenu( WIN_Handle32(hwnd), HMENU_32(hMenu) );
}


/**************************************************************************
 *              GetDesktopWindow   (USER.286)
 */
HWND16 WINAPI GetDesktopWindow16(void)
{
    return HWND_16( GetDesktopWindow() );
}


/**************************************************************************
 *              GetLastActivePopup   (USER.287)
 */
HWND16 WINAPI GetLastActivePopup16( HWND16 hwnd )
{
    return HWND_16( GetLastActivePopup( WIN_Handle32(hwnd) ));
}


/**************************************************************************
 *              RedrawWindow   (USER.290)
 */
BOOL16 WINAPI RedrawWindow16( HWND16 hwnd, const RECT16 *rectUpdate,
                              HRGN16 hrgnUpdate, UINT16 flags )
{
    if (rectUpdate)
    {
        RECT r;
        r.left   = rectUpdate->left;
        r.top    = rectUpdate->top;
        r.right  = rectUpdate->right;
        r.bottom = rectUpdate->bottom;
        return RedrawWindow(WIN_Handle32(hwnd), &r, HRGN_32(hrgnUpdate), flags);
    }
    return RedrawWindow(WIN_Handle32(hwnd), NULL, HRGN_32(hrgnUpdate), flags);
}


/**************************************************************************
 *              LockWindowUpdate   (USER.294)
 */
BOOL16 WINAPI LockWindowUpdate16( HWND16 hwnd )
{
    return LockWindowUpdate( WIN_Handle32(hwnd) );
}


/**************************************************************************
 *              ScrollWindowEx   (USER.319)
 */
INT16 WINAPI ScrollWindowEx16( HWND16 hwnd, INT16 dx, INT16 dy,
                               const RECT16 *rect, const RECT16 *clipRect,
                               HRGN16 hrgnUpdate, LPRECT16 rcUpdate,
                               UINT16 flags )
{
    RECT rect32, clipRect32, rcUpdate32;
    BOOL16 ret;

    if (rect)
    {
        rect32.left   = rect->left;
        rect32.top    = rect->top;
        rect32.right  = rect->right;
        rect32.bottom = rect->bottom;
    }
    if (clipRect)
    {
        clipRect32.left   = clipRect->left;
        clipRect32.top    = clipRect->top;
        clipRect32.right  = clipRect->right;
        clipRect32.bottom = clipRect->bottom;
    }
    ret = ScrollWindowEx( WIN_Handle32(hwnd), dx, dy, rect ? &rect32 : NULL,
                          clipRect ? &clipRect32 : NULL, HRGN_32(hrgnUpdate),
                          (rcUpdate) ? &rcUpdate32 : NULL, flags );
    if (rcUpdate)
    {
        rcUpdate->left   = rcUpdate32.left;
        rcUpdate->top    = rcUpdate32.top;
        rcUpdate->right  = rcUpdate32.right;
        rcUpdate->bottom = rcUpdate32.bottom;
    }
    return ret;
}


/**************************************************************************
 *              FillWindow   (USER.324)
 */
void WINAPI FillWindow16( HWND16 hwndParent, HWND16 hwnd, HDC16 hdc, HBRUSH16 hbrush )
{
    RECT rect;
    RECT16 rc16;
    GetClientRect( WIN_Handle32(hwnd), &rect );
    DPtoLP( HDC_32(hdc), (LPPOINT)&rect, 2 );
    rc16.left   = rect.left;
    rc16.top    = rect.top;
    rc16.right  = rect.right;
    rc16.bottom = rect.bottom;
    PaintRect16( hwndParent, hwnd, hdc, hbrush, &rc16 );
}


/**************************************************************************
 *              PaintRect   (USER.325)
 */
void WINAPI PaintRect16( HWND16 hwndParent, HWND16 hwnd, HDC16 hdc,
                         HBRUSH16 hbrush, const RECT16 *rect)
{
    if (hbrush <= CTLCOLOR_STATIC)
    {
        HWND parent = WIN_Handle32(hwndParent), hwnd32 = WIN_Handle32(hwnd);

        if (!parent) return;
        hbrush = SendMessageW( parent, WM_CTLCOLORMSGBOX + hbrush, (WPARAM)hdc, (LPARAM)hwnd32 );
        if (!hbrush) hbrush = DefWindowProcW( parent, WM_CTLCOLORMSGBOX + hbrush,
                                              (WPARAM)hdc, (LPARAM)hwnd32 );
    }
    if (hbrush) FillRect16( hdc, rect, hbrush );
}


/**************************************************************************
 *              GetControlBrush   (USER.326)
 */
HBRUSH16 WINAPI GetControlBrush16( HWND16 hwnd, HDC16 hdc, UINT16 ctlType )
{
    HBRUSH16 ret;
    HWND hwnd32 = WIN_Handle32(hwnd);
    HWND parent = GetParent( hwnd32 );

    if (!parent) parent = hwnd32;
    ret = SendMessageW( parent, WM_CTLCOLORMSGBOX + ctlType, (WPARAM)hdc, (LPARAM)hwnd32 );
    if (!ret) ret = DefWindowProcW( parent, WM_CTLCOLORMSGBOX + ctlType,
                                    (WPARAM)hdc, (LPARAM)hwnd32 );
    return ret;
}


/**************************************************************************
 *              GetDCEx   (USER.359)
 */
HDC16 WINAPI GetDCEx16( HWND16 hwnd, HRGN16 hrgnClip, DWORD flags )
{
    return HDC_16(GetDCEx(WIN_Handle32(hwnd), HRGN_32(hrgnClip), flags));
}


/**************************************************************************
 *              GetWindowPlacement   (USER.370)
 */
BOOL16 WINAPI GetWindowPlacement16( HWND16 hwnd, WINDOWPLACEMENT16 *wp16 )
{
    WINDOWPLACEMENT wpl;

    wpl.length = sizeof(wpl);
    if (!GetWindowPlacement( WIN_Handle32(hwnd), &wpl )) return FALSE;
    wp16->length  = sizeof(*wp16);
    wp16->flags   = wpl.flags;
    wp16->showCmd = wpl.showCmd;
    wp16->ptMinPosition.x = wpl.ptMinPosition.x;
    wp16->ptMinPosition.y = wpl.ptMinPosition.y;
    wp16->ptMaxPosition.x = wpl.ptMaxPosition.x;
    wp16->ptMaxPosition.y = wpl.ptMaxPosition.y;
    wp16->rcNormalPosition.left   = wpl.rcNormalPosition.left;
    wp16->rcNormalPosition.top    = wpl.rcNormalPosition.top;
    wp16->rcNormalPosition.right  = wpl.rcNormalPosition.right;
    wp16->rcNormalPosition.bottom = wpl.rcNormalPosition.bottom;
    return TRUE;
}


/**************************************************************************
 *              SetWindowPlacement   (USER.371)
 */
BOOL16 WINAPI SetWindowPlacement16( HWND16 hwnd, const WINDOWPLACEMENT16 *wp16 )
{
    WINDOWPLACEMENT wpl;

    if (!wp16) return FALSE;
    wpl.length  = sizeof(wpl);
    wpl.flags   = wp16->flags;
    wpl.showCmd = wp16->showCmd;
    wpl.ptMinPosition.x = wp16->ptMinPosition.x;
    wpl.ptMinPosition.y = wp16->ptMinPosition.y;
    wpl.ptMaxPosition.x = wp16->ptMaxPosition.x;
    wpl.ptMaxPosition.y = wp16->ptMaxPosition.y;
    wpl.rcNormalPosition.left   = wp16->rcNormalPosition.left;
    wpl.rcNormalPosition.top    = wp16->rcNormalPosition.top;
    wpl.rcNormalPosition.right  = wp16->rcNormalPosition.right;
    wpl.rcNormalPosition.bottom = wp16->rcNormalPosition.bottom;
    return SetWindowPlacement( WIN_Handle32(hwnd), &wpl );
}


/***********************************************************************
 *		RegisterClassEx (USER.397)
 */
ATOM WINAPI RegisterClassEx16( const WNDCLASSEX16 *wc )
{
    WNDCLASSEXA wc32;

    wc32.cbSize        = sizeof(wc32);
    wc32.style         = wc->style;
    wc32.lpfnWndProc   = WINPROC_AllocProc16( wc->lpfnWndProc );
    wc32.cbClsExtra    = wc->cbClsExtra;
    wc32.cbWndExtra    = wc->cbWndExtra;
    wc32.hInstance     = HINSTANCE_32(GetExePtr(wc->hInstance));
    if (!wc32.hInstance) wc32.hInstance = HINSTANCE_32(GetModuleHandle16(NULL));
    wc32.hIcon         = HICON_32(wc->hIcon);
    wc32.hCursor       = HCURSOR_32(wc->hCursor);
    wc32.hbrBackground = HBRUSH_32(wc->hbrBackground);
    wc32.lpszMenuName  = MapSL(wc->lpszMenuName);
    wc32.lpszClassName = MapSL(wc->lpszClassName);
    wc32.hIconSm       = HICON_32(wc->hIconSm);
    return RegisterClassExA( &wc32 );
}


/***********************************************************************
 *		GetClassInfoEx (USER.398)
 *
 * FIXME: this is just a guess, I have no idea if GetClassInfoEx() is the
 * same in Win16 as in Win32. --AJ
 */
BOOL16 WINAPI GetClassInfoEx16( HINSTANCE16 hInst16, SEGPTR name, WNDCLASSEX16 *wc )
{
    WNDCLASSEXA wc32;
    HINSTANCE hInstance;
    BOOL ret;

    if (hInst16 == GetModuleHandle16("user")) hInstance = user32_module;
    else hInstance = HINSTANCE_32(GetExePtr( hInst16 ));

    ret = GetClassInfoExA( hInstance, MapSL(name), &wc32 );

    if (ret)
    {
        wc->lpfnWndProc   = WINPROC_GetProc16( wc32.lpfnWndProc, FALSE );
        wc->style         = wc32.style;
        wc->cbClsExtra    = wc32.cbClsExtra;
        wc->cbWndExtra    = wc32.cbWndExtra;
        wc->hInstance     = (wc32.hInstance == user32_module) ? GetModuleHandle16("user") : HINSTANCE_16(wc32.hInstance);
        wc->hIcon         = HICON_16(wc32.hIcon);
        wc->hIconSm       = HICON_16(wc32.hIconSm);
        wc->hCursor       = HCURSOR_16(wc32.hCursor);
        wc->hbrBackground = HBRUSH_16(wc32.hbrBackground);
        wc->lpszClassName = 0;
        wc->lpszMenuName  = MapLS(wc32.lpszMenuName);  /* FIXME: leak */
    }
    return ret;
}


/**************************************************************************
 *              ChildWindowFromPointEx   (USER.399)
 */
HWND16 WINAPI ChildWindowFromPointEx16( HWND16 hwndParent, POINT16 pt, UINT16 uFlags)
{
    POINT pt32;

    pt32.x = pt.x;
    pt32.y = pt.y;
    return HWND_16( ChildWindowFromPointEx( WIN_Handle32(hwndParent), pt32, uFlags ));
}


/**************************************************************************
 *              GetPriorityClipboardFormat   (USER.402)
 */
INT16 WINAPI GetPriorityClipboardFormat16( UINT16 *list, INT16 count )
{
    int i;

    for (i = 0; i < count; i++)
        if (IsClipboardFormatAvailable( list[i] )) return list[i];
    return -1;
}


/***********************************************************************
 *		UnregisterClass (USER.403)
 */
BOOL16 WINAPI UnregisterClass16( LPCSTR className, HINSTANCE16 hInstance )
{
    if (hInstance == GetModuleHandle16("user")) hInstance = 0;
    return UnregisterClassA( className, HINSTANCE_32(GetExePtr( hInstance )) );
}


/***********************************************************************
 *		GetClassInfo (USER.404)
 */
BOOL16 WINAPI GetClassInfo16( HINSTANCE16 hInst16, SEGPTR name, WNDCLASS16 *wc )
{
    WNDCLASSEX16 wcex;
    UINT16 ret = GetClassInfoEx16( hInst16, name, &wcex );

    if (ret)
    {
        wc->style         = wcex.style;
        wc->lpfnWndProc   = wcex.lpfnWndProc;
        wc->cbClsExtra    = wcex.cbClsExtra;
        wc->cbWndExtra    = wcex.cbWndExtra;
        wc->hInstance     = wcex.hInstance;
        wc->hIcon         = wcex.hIcon;
        wc->hCursor       = wcex.hCursor;
        wc->hbrBackground = wcex.hbrBackground;
        wc->lpszMenuName  = wcex.lpszMenuName;
        wc->lpszClassName = wcex.lpszClassName;
    }
    return ret;
}


/**************************************************************************
 *              TrackPopupMenu   (USER.416)
 */
BOOL16 WINAPI TrackPopupMenu16( HMENU16 hMenu, UINT16 wFlags, INT16 x, INT16 y,
                                INT16 nReserved, HWND16 hwnd, const RECT16 *lpRect )
{
    RECT r;
    if (lpRect)
    {
        r.left   = lpRect->left;
        r.top    = lpRect->top;
        r.right  = lpRect->right;
        r.bottom = lpRect->bottom;
    }
    return TrackPopupMenu( HMENU_32(hMenu), wFlags, x, y, nReserved,
                           WIN_Handle32(hwnd), lpRect ? &r : NULL );
}


/**************************************************************************
 *              FindWindowEx   (USER.427)
 */
HWND16 WINAPI FindWindowEx16( HWND16 parent, HWND16 child, LPCSTR className, LPCSTR title )
{
    return HWND_16( FindWindowExA( WIN_Handle32(parent), WIN_Handle32(child),
                                        className, title ));
}


/***********************************************************************
 *		DefFrameProc (USER.445)
 */
LRESULT WINAPI DefFrameProc16( HWND16 hwnd, HWND16 hwndMDIClient,
                               UINT16 message, WPARAM16 wParam, LPARAM lParam )
{
    switch (message)
    {
    case WM_SETTEXT:
        lParam = (LPARAM)MapSL(lParam);
        /* fall through */
    case WM_COMMAND:
    case WM_NCACTIVATE:
    case WM_SETFOCUS:
    case WM_SIZE:
        return DefFrameProcA( WIN_Handle32(hwnd), WIN_Handle32(hwndMDIClient),
                              message, wParam, lParam );

    case WM_NEXTMENU:
        {
            MDINEXTMENU next_menu;
            DefFrameProcW( WIN_Handle32(hwnd), WIN_Handle32(hwndMDIClient),
                           message, wParam, (LPARAM)&next_menu );
            return MAKELONG( HMENU_16(next_menu.hmenuNext), HWND_16(next_menu.hwndNext) );
        }
    default:
        return DefWindowProc16(hwnd, message, wParam, lParam);
    }
}


/***********************************************************************
 *		DefMDIChildProc (USER.447)
 */
LRESULT WINAPI DefMDIChildProc16( HWND16 hwnd, UINT16 message,
                                  WPARAM16 wParam, LPARAM lParam )
{
    switch (message)
    {
    case WM_SETTEXT:
        return DefMDIChildProcA( WIN_Handle32(hwnd), message, wParam, (LPARAM)MapSL(lParam) );

    case WM_MENUCHAR:
    case WM_CLOSE:
    case WM_SETFOCUS:
    case WM_CHILDACTIVATE:
    case WM_SYSCOMMAND:
    case WM_SETVISIBLE:
    case WM_SIZE:
    case WM_SYSCHAR:
        return DefMDIChildProcW( WIN_Handle32(hwnd), message, wParam, lParam );

    case WM_GETMINMAXINFO:
        {
            MINMAXINFO16 *mmi16 = MapSL(lParam);
            MINMAXINFO mmi;

            mmi.ptReserved.x     = mmi16->ptReserved.x;
            mmi.ptReserved.y     = mmi16->ptReserved.y;
            mmi.ptMaxSize.x      = mmi16->ptMaxSize.x;
            mmi.ptMaxSize.y      = mmi16->ptMaxSize.y;
            mmi.ptMaxPosition.x  = mmi16->ptMaxPosition.x;
            mmi.ptMaxPosition.y  = mmi16->ptMaxPosition.y;
            mmi.ptMinTrackSize.x = mmi16->ptMinTrackSize.x;
            mmi.ptMinTrackSize.y = mmi16->ptMinTrackSize.y;
            mmi.ptMaxTrackSize.x = mmi16->ptMaxTrackSize.x;
            mmi.ptMaxTrackSize.y = mmi16->ptMaxTrackSize.y;

            DefMDIChildProcW( WIN_Handle32(hwnd), message, wParam, (LPARAM)&mmi );

            mmi16->ptReserved.x     = mmi.ptReserved.x;
            mmi16->ptReserved.y     = mmi.ptReserved.y;
            mmi16->ptMaxSize.x      = mmi.ptMaxSize.x;
            mmi16->ptMaxSize.y      = mmi.ptMaxSize.y;
            mmi16->ptMaxPosition.x  = mmi.ptMaxPosition.x;
            mmi16->ptMaxPosition.y  = mmi.ptMaxPosition.y;
            mmi16->ptMinTrackSize.x = mmi.ptMinTrackSize.x;
            mmi16->ptMinTrackSize.y = mmi.ptMinTrackSize.y;
            mmi16->ptMaxTrackSize.x = mmi.ptMaxTrackSize.x;
            mmi16->ptMaxTrackSize.y = mmi.ptMaxTrackSize.y;
            return 0;
        }
    case WM_NEXTMENU:
        {
            MDINEXTMENU next_menu;
            DefMDIChildProcW( WIN_Handle32(hwnd), message, wParam, (LPARAM)&next_menu );
            return MAKELONG( HMENU_16(next_menu.hmenuNext), HWND_16(next_menu.hwndNext) );
        }
    default:
        return DefWindowProc16(hwnd, message, wParam, lParam);
    }
}


/**************************************************************************
 *              DrawAnimatedRects   (USER.448)
 */
BOOL16 WINAPI DrawAnimatedRects16( HWND16 hwnd, INT16 idAni,
                                   const RECT16* lprcFrom, const RECT16* lprcTo )
{
    RECT rcFrom32, rcTo32;
    rcFrom32.left   = lprcFrom->left;
    rcFrom32.top    = lprcFrom->top;
    rcFrom32.right  = lprcFrom->right;
    rcFrom32.bottom = lprcFrom->bottom;
    rcTo32.left     = lprcTo->left;
    rcTo32.top      = lprcTo->top;
    rcTo32.right    = lprcTo->right;
    rcTo32.bottom   = lprcTo->bottom;
    return DrawAnimatedRects( WIN_Handle32(hwnd), idAni, &rcFrom32, &rcTo32 );
}


/***********************************************************************
 *		GetInternalWindowPos (USER.460)
 */
UINT16 WINAPI GetInternalWindowPos16( HWND16 hwnd, LPRECT16 rectWnd, LPPOINT16 ptIcon )
{
    WINDOWPLACEMENT16 wndpl;

    if (!GetWindowPlacement16( hwnd, &wndpl )) return 0;
    if (rectWnd) *rectWnd = wndpl.rcNormalPosition;
    if (ptIcon)  *ptIcon = wndpl.ptMinPosition;
    return wndpl.showCmd;
}


/**************************************************************************
 *              SetInternalWindowPos   (USER.461)
 */
void WINAPI SetInternalWindowPos16( HWND16 hwnd, UINT16 showCmd, LPRECT16 rect, LPPOINT16 pt )
{
    RECT rc32;
    POINT pt32;

    if (rect)
    {
        rc32.left   = rect->left;
        rc32.top    = rect->top;
        rc32.right  = rect->right;
        rc32.bottom = rect->bottom;
    }
    if (pt)
    {
        pt32.x = pt->x;
        pt32.y = pt->y;
    }
    SetInternalWindowPos( WIN_Handle32(hwnd), showCmd,
                          rect ? &rc32 : NULL, pt ? &pt32 : NULL );
}


/**************************************************************************
 *              CalcChildScroll   (USER.462)
 */
void WINAPI CalcChildScroll16( HWND16 hwnd, WORD scroll )
{
    CalcChildScroll( WIN_Handle32(hwnd), scroll );
}


/**************************************************************************
 *              ScrollChildren   (USER.463)
 */
void WINAPI ScrollChildren16(HWND16 hwnd, UINT16 uMsg, WPARAM16 wParam, LPARAM lParam)
{
    ScrollChildren( WIN_Handle32(hwnd), uMsg, wParam, lParam );
}


/**************************************************************************
 *              DragDetect   (USER.465)
 */
BOOL16 WINAPI DragDetect16( HWND16 hwnd, POINT16 pt )
{
    POINT pt32;

    pt32.x = pt.x;
    pt32.y = pt.y;
    return DragDetect( WIN_Handle32(hwnd), pt32 );
}


/**************************************************************************
 *              SetScrollInfo   (USER.475)
 */
INT16 WINAPI SetScrollInfo16( HWND16 hwnd, INT16 nBar, const SCROLLINFO *info, BOOL16 redraw )
{
    return SetScrollInfo( WIN_Handle32(hwnd), nBar, info, redraw );
}


/**************************************************************************
 *              GetScrollInfo   (USER.476)
 */
BOOL16 WINAPI GetScrollInfo16( HWND16 hwnd, INT16 nBar, LPSCROLLINFO info )
{
    return GetScrollInfo( WIN_Handle32(hwnd), nBar, info );
}


/**************************************************************************
 *              EnableScrollBar   (USER.482)
 */
BOOL16 WINAPI EnableScrollBar16( HWND16 hwnd, INT16 nBar, UINT16 flags )
{
    return EnableScrollBar( WIN_Handle32(hwnd), nBar, flags );
}


/**************************************************************************
 *              GetShellWindow   (USER.600)
 */
HWND16 WINAPI GetShellWindow16(void)
{
    return HWND_16( GetShellWindow() );
}


/**************************************************************************
 *              GetForegroundWindow   (USER.608)
 */
HWND16 WINAPI GetForegroundWindow16(void)
{
    return HWND_16( GetForegroundWindow() );
}


/**************************************************************************
 *              SetForegroundWindow   (USER.609)
 */
BOOL16 WINAPI SetForegroundWindow16( HWND16 hwnd )
{
    return SetForegroundWindow( WIN_Handle32(hwnd) );
}


/**************************************************************************
 *              DrawCaptionTemp   (USER.657)
 */
BOOL16 WINAPI DrawCaptionTemp16( HWND16 hwnd, HDC16 hdc, const RECT16 *rect,
                                 HFONT16 hFont, HICON16 hIcon, LPCSTR str, UINT16 uFlags )
{
    RECT rect32;

    if (rect)
    {
        rect32.left   = rect->left;
        rect32.top    = rect->top;
        rect32.right  = rect->right;
        rect32.bottom = rect->bottom;
    }
    return DrawCaptionTempA( WIN_Handle32(hwnd), HDC_32(hdc),
			     rect ? &rect32 : NULL, HFONT_32(hFont),
			     HICON_32(hIcon), str, uFlags & 0x1f );
}


/**************************************************************************
 *              DrawCaption   (USER.660)
 */
BOOL16 WINAPI DrawCaption16( HWND16 hwnd, HDC16 hdc, const RECT16 *rect, UINT16 flags )
{
    RECT rect32;

    if (rect)
    {
        rect32.left   = rect->left;
        rect32.top    = rect->top;
        rect32.right  = rect->right;
        rect32.bottom = rect->bottom;
    }
    return DrawCaption(WIN_Handle32(hwnd), HDC_32(hdc), rect ? &rect32 : NULL, flags);
}


/**************************************************************************
 *              GetMenuItemRect   (USER.665)
 */
BOOL16 WINAPI GetMenuItemRect16( HWND16 hwnd, HMENU16 hMenu, UINT16 uItem,
                                 LPRECT16 rect)
{
     RECT r32;
     BOOL res;
     if (!rect) return FALSE;
     res = GetMenuItemRect( WIN_Handle32(hwnd), HMENU_32(hMenu), uItem, &r32 );
     rect->left   = r32.left;
     rect->top    = r32.top;
     rect->right  = r32.right;
     rect->bottom = r32.bottom;
     return res;
}


/**************************************************************************
 *              SetWindowRgn   (USER.668)
 */
INT16 WINAPI SetWindowRgn16( HWND16 hwnd, HRGN16 hrgn, BOOL16 redraw )
{
    return SetWindowRgn( WIN_Handle32(hwnd), HRGN_32(hrgn), redraw );
}


/**************************************************************************
 *              MessageBoxIndirect   (USER.827)
 */
INT16 WINAPI MessageBoxIndirect16( LPMSGBOXPARAMS16 msgbox )
{
    MSGBOXPARAMSA msgbox32;

    msgbox32.cbSize             = msgbox->cbSize;
    msgbox32.hwndOwner          = WIN_Handle32( msgbox->hwndOwner );
    msgbox32.hInstance          = HINSTANCE_32(msgbox->hInstance);
    msgbox32.lpszText           = MapSL(msgbox->lpszText);
    msgbox32.lpszCaption        = MapSL(msgbox->lpszCaption);
    msgbox32.dwStyle            = msgbox->dwStyle;
    msgbox32.lpszIcon           = MapSL(msgbox->lpszIcon);
    msgbox32.dwContextHelpId    = msgbox->dwContextHelpId;
    msgbox32.lpfnMsgBoxCallback = msgbox->lpfnMsgBoxCallback;
    msgbox32.dwLanguageId       = msgbox->dwLanguageId;
    return MessageBoxIndirectA( &msgbox32 );
}
