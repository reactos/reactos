/*
 * Misc 16-bit USER functions
 *
 * Copyright 1993, 1996 Alexandre Julliard
 * Copyright 2002 Patrik Stridvall
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

#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define OEMRESOURCE

#include "wine/winuser16.h"
#include "windef.h"
#include "winbase.h"
#include "wownt32.h"
#include "user_private.h"
#include "win.h"
#include "controls.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(user);

/* handle to handle 16 conversions */
#define HANDLE_16(h32)		(LOWORD(h32))

/* handle16 to handle conversions */
#define HANDLE_32(h16)		((HANDLE)(ULONG_PTR)(h16))
#define HINSTANCE_32(h16)	((HINSTANCE)(ULONG_PTR)(h16))

#define IS_MENU_STRING_ITEM(flags) \
    (((flags) & (MF_STRING | MF_BITMAP | MF_OWNERDRAW | MF_SEPARATOR)) == MF_STRING)

/* UserSeeUserDo parameters */
#define USUD_LOCALALLOC        0x0001
#define USUD_LOCALFREE         0x0002
#define USUD_LOCALCOMPACT      0x0003
#define USUD_LOCALHEAP         0x0004
#define USUD_FIRSTCLASS        0x0005

WORD WINAPI DestroyIcon32(HGLOBAL16, UINT16);


struct gray_string_info
{
    GRAYSTRINGPROC16 proc;
    LPARAM           param;
    char             str[1];
};

/* callback for 16-bit gray string proc with opaque pointer */
static BOOL CALLBACK gray_string_callback( HDC hdc, LPARAM param, INT len )
{
    const struct gray_string_info *info = (struct gray_string_info *)param;
    WORD args[4];
    DWORD ret;

    args[3] = HDC_16(hdc);
    args[2] = HIWORD(info->param);
    args[1] = LOWORD(info->param);
    args[0] = len;
    WOWCallback16Ex( (DWORD)info->proc, WCB16_PASCAL, sizeof(args), args, &ret );
    return LOWORD(ret);
}

/* callback for 16-bit gray string proc with string pointer */
static BOOL CALLBACK gray_string_callback_ptr( HDC hdc, LPARAM param, INT len )
{
    const struct gray_string_info *info;
    char *str = (char *)param;

    info = (struct gray_string_info *)(str - offsetof( struct gray_string_info, str ));
    return gray_string_callback( hdc, (LPARAM)info, len );
}

struct draw_state_info
{
    DRAWSTATEPROC16 proc;
    LPARAM          param;
};

/* callback for 16-bit DrawState functions */
static BOOL CALLBACK draw_state_callback( HDC hdc, LPARAM lparam, WPARAM wparam, int cx, int cy )
{
    const struct draw_state_info *info = (struct draw_state_info *)lparam;
    WORD args[6];
    DWORD ret;

    args[5] = HDC_16(hdc);
    args[4] = HIWORD(info->param);
    args[3] = LOWORD(info->param);
    args[2] = wparam;
    args[1] = cx;
    args[0] = cy;
    WOWCallback16Ex( (DWORD)info->proc, WCB16_PASCAL, sizeof(args), args, &ret );
    return LOWORD(ret);
}


/**********************************************************************
 *		InitApp (USER.5)
 */
INT16 WINAPI InitApp16( HINSTANCE16 hInstance )
{
    /* Create task message queue */
    return (InitThreadInput16( 0, 0 ) != 0);
}


/***********************************************************************
 *		ExitWindows (USER.7)
 */
BOOL16 WINAPI ExitWindows16( DWORD dwReturnCode, UINT16 wReserved )
{
    return ExitWindowsEx( EWX_LOGOFF, 0xffffffff );
}


/***********************************************************************
 *		GetTimerResolution (USER.14)
 */
LONG WINAPI GetTimerResolution16(void)
{
    return (1000);
}


/***********************************************************************
 *		ClipCursor (USER.16)
 */
BOOL16 WINAPI ClipCursor16( const RECT16 *rect )
{
    RECT rect32;

    if (!rect) return ClipCursor( NULL );
    rect32.left   = rect->left;
    rect32.top    = rect->top;
    rect32.right  = rect->right;
    rect32.bottom = rect->bottom;
    return ClipCursor( &rect32 );
}


/***********************************************************************
 *		GetCursorPos (USER.17)
 */
BOOL16 WINAPI GetCursorPos16( POINT16 *pt )
{
    POINT pos;
    if (!pt) return 0;
    GetCursorPos(&pos);
    pt->x = pos.x;
    pt->y = pos.y;
    return 1;
}


/***********************************************************************
 *		SetCursor (USER.69)
 */
HCURSOR16 WINAPI SetCursor16(HCURSOR16 hCursor)
{
  return HCURSOR_16(SetCursor(HCURSOR_32(hCursor)));
}


/***********************************************************************
 *		SetCursorPos (USER.70)
 */
void WINAPI SetCursorPos16( INT16 x, INT16 y )
{
    SetCursorPos( x, y );
}


/***********************************************************************
 *		ShowCursor (USER.71)
 */
INT16 WINAPI ShowCursor16(BOOL16 bShow)
{
  return ShowCursor(bShow);
}


/***********************************************************************
 *		SetRect (USER.72)
 */
void WINAPI SetRect16( LPRECT16 rect, INT16 left, INT16 top, INT16 right, INT16 bottom )
{
    rect->left   = left;
    rect->right  = right;
    rect->top    = top;
    rect->bottom = bottom;
}


/***********************************************************************
 *		SetRectEmpty (USER.73)
 */
void WINAPI SetRectEmpty16( LPRECT16 rect )
{
    rect->left = rect->right = rect->top = rect->bottom = 0;
}


/***********************************************************************
 *		CopyRect (USER.74)
 */
BOOL16 WINAPI CopyRect16( RECT16 *dest, const RECT16 *src )
{
    *dest = *src;
    return TRUE;
}


/***********************************************************************
 *		IsRectEmpty (USER.75)
 *
 * Bug compat: Windows checks for 0 or negative width/height.
 */
BOOL16 WINAPI IsRectEmpty16( const RECT16 *rect )
{
    return ((rect->left >= rect->right) || (rect->top >= rect->bottom));
}


/***********************************************************************
 *		PtInRect (USER.76)
 */
BOOL16 WINAPI PtInRect16( const RECT16 *rect, POINT16 pt )
{
    return ((pt.x >= rect->left) && (pt.x < rect->right) &&
            (pt.y >= rect->top) && (pt.y < rect->bottom));
}


/***********************************************************************
 *		OffsetRect (USER.77)
 */
void WINAPI OffsetRect16( LPRECT16 rect, INT16 x, INT16 y )
{
    rect->left   += x;
    rect->right  += x;
    rect->top    += y;
    rect->bottom += y;
}


/***********************************************************************
 *		InflateRect (USER.78)
 */
void WINAPI InflateRect16( LPRECT16 rect, INT16 x, INT16 y )
{
    rect->left   -= x;
    rect->top    -= y;
    rect->right  += x;
    rect->bottom += y;
}


/***********************************************************************
 *		IntersectRect (USER.79)
 */
BOOL16 WINAPI IntersectRect16( LPRECT16 dest, const RECT16 *src1,
                               const RECT16 *src2 )
{
    if (IsRectEmpty16(src1) || IsRectEmpty16(src2) ||
        (src1->left >= src2->right) || (src2->left >= src1->right) ||
        (src1->top >= src2->bottom) || (src2->top >= src1->bottom))
    {
        SetRectEmpty16( dest );
        return FALSE;
    }
    dest->left   = max( src1->left, src2->left );
    dest->right  = min( src1->right, src2->right );
    dest->top    = max( src1->top, src2->top );
    dest->bottom = min( src1->bottom, src2->bottom );
    return TRUE;
}


/***********************************************************************
 *		UnionRect (USER.80)
 */
BOOL16 WINAPI UnionRect16( LPRECT16 dest, const RECT16 *src1,
                           const RECT16 *src2 )
{
    if (IsRectEmpty16(src1))
    {
        if (IsRectEmpty16(src2))
        {
            SetRectEmpty16( dest );
            return FALSE;
        }
        else *dest = *src2;
    }
    else
    {
        if (IsRectEmpty16(src2)) *dest = *src1;
        else
        {
            dest->left   = min( src1->left, src2->left );
            dest->right  = max( src1->right, src2->right );
            dest->top    = min( src1->top, src2->top );
            dest->bottom = max( src1->bottom, src2->bottom );
        }
    }
    return TRUE;
}


/***********************************************************************
 *		FillRect (USER.81)
 * NOTE
 *   The Win16 variant doesn't support special color brushes like
 *   the Win32 one, despite the fact that Win16, as well as Win32,
 *   supports special background brushes for a window class.
 */
INT16 WINAPI FillRect16( HDC16 hdc, const RECT16 *rect, HBRUSH16 hbrush )
{
    HBRUSH prevBrush;

    /* coordinates are logical so we cannot fast-check 'rect',
     * it will be done later in the PatBlt().
     */

    if (!(prevBrush = SelectObject( HDC_32(hdc), HBRUSH_32(hbrush) ))) return 0;
    PatBlt( HDC_32(hdc), rect->left, rect->top,
              rect->right - rect->left, rect->bottom - rect->top, PATCOPY );
    SelectObject( HDC_32(hdc), prevBrush );
    return 1;
}


/***********************************************************************
 *		InvertRect (USER.82)
 */
void WINAPI InvertRect16( HDC16 hdc, const RECT16 *rect )
{
    PatBlt( HDC_32(hdc), rect->left, rect->top,
              rect->right - rect->left, rect->bottom - rect->top, DSTINVERT );
}


/***********************************************************************
 *		FrameRect (USER.83)
 */
INT16 WINAPI FrameRect16( HDC16 hdc, const RECT16 *rect16, HBRUSH16 hbrush )
{
    RECT rect;

    rect.left   = rect16->left;
    rect.top    = rect16->top;
    rect.right  = rect16->right;
    rect.bottom = rect16->bottom;
    return FrameRect( HDC_32(hdc), &rect, HBRUSH_32(hbrush) );
}


/***********************************************************************
 *		DrawIcon (USER.84)
 */
BOOL16 WINAPI DrawIcon16(HDC16 hdc, INT16 x, INT16 y, HICON16 hIcon)
{
  return DrawIcon(HDC_32(hdc), x, y, HICON_32(hIcon));
}


/***********************************************************************
 *           DrawText    (USER.85)
 */
INT16 WINAPI DrawText16( HDC16 hdc, LPCSTR str, INT16 count, LPRECT16 rect, UINT16 flags )
{
    INT16 ret;

    if (rect)
    {
        RECT rect32;

        rect32.left   = rect->left;
        rect32.top    = rect->top;
        rect32.right  = rect->right;
        rect32.bottom = rect->bottom;
        ret = DrawTextA( HDC_32(hdc), str, count, &rect32, flags );
        rect->left   = rect32.left;
        rect->top    = rect32.top;
        rect->right  = rect32.right;
        rect->bottom = rect32.bottom;
    }
    else ret = DrawTextA( HDC_32(hdc), str, count, NULL, flags);
    return ret;
}


/***********************************************************************
 *		IconSize (USER.86)
 *
 * See "Undocumented Windows". Used by W2.0 paint.exe.
 */
DWORD WINAPI IconSize16(void)
{
  return MAKELONG(GetSystemMetrics(SM_CYICON), GetSystemMetrics(SM_CXICON));
}


/***********************************************************************
 *		AdjustWindowRect (USER.102)
 */
BOOL16 WINAPI AdjustWindowRect16( LPRECT16 rect, DWORD style, BOOL16 menu )
{
    return AdjustWindowRectEx16( rect, style, menu, 0 );
}


/***********************************************************************
 *		MessageBeep (USER.104)
 */
void WINAPI MessageBeep16( UINT16 i )
{
    MessageBeep( i );
}


/**************************************************************************
 *		CloseClipboard (USER.138)
 */
BOOL16 WINAPI CloseClipboard16(void)
{
    return CloseClipboard();
}


/**************************************************************************
 *		EmptyClipboard (USER.139)
 */
BOOL16 WINAPI EmptyClipboard16(void)
{
    return EmptyClipboard();
}


/**************************************************************************
 *		CountClipboardFormats (USER.143)
 */
INT16 WINAPI CountClipboardFormats16(void)
{
    return CountClipboardFormats();
}


/**************************************************************************
 *		EnumClipboardFormats (USER.144)
 */
UINT16 WINAPI EnumClipboardFormats16( UINT16 id )
{
    return EnumClipboardFormats( id );
}


/**************************************************************************
 *		RegisterClipboardFormat (USER.145)
 */
UINT16 WINAPI RegisterClipboardFormat16( LPCSTR name )
{
    return RegisterClipboardFormatA( name );
}


/**************************************************************************
 *		GetClipboardFormatName (USER.146)
 */
INT16 WINAPI GetClipboardFormatName16( UINT16 id, LPSTR buffer, INT16 maxlen )
{
    return GetClipboardFormatNameA( id, buffer, maxlen );
}


/**********************************************************************
 *         CreateMenu    (USER.151)
 */
HMENU16 WINAPI CreateMenu16(void)
{
    return HMENU_16( CreateMenu() );
}


/**********************************************************************
 *         DestroyMenu    (USER.152)
 */
BOOL16 WINAPI DestroyMenu16( HMENU16 hMenu )
{
    return DestroyMenu( HMENU_32(hMenu) );
}


/*******************************************************************
 *         ChangeMenu    (USER.153)
 */
BOOL16 WINAPI ChangeMenu16( HMENU16 hMenu, UINT16 pos, SEGPTR data,
                            UINT16 id, UINT16 flags )
{
    if (flags & MF_APPEND) return AppendMenu16( hMenu, flags & ~MF_APPEND, id, data );

    /* FIXME: Word passes the item id in 'pos' and 0 or 0xffff as id */
    /* for MF_DELETE. We should check the parameters for all others */
    /* MF_* actions also (anybody got a doc on ChangeMenu?). */

    if (flags & MF_DELETE) return DeleteMenu16(hMenu, pos, flags & ~MF_DELETE);
    if (flags & MF_CHANGE) return ModifyMenu16(hMenu, pos, flags & ~MF_CHANGE, id, data );
    if (flags & MF_REMOVE) return RemoveMenu16(hMenu, flags & MF_BYPOSITION ? pos : id,
                                               flags & ~MF_REMOVE );
    /* Default: MF_INSERT */
    return InsertMenu16( hMenu, pos, flags, id, data );
}


/*******************************************************************
 *         CheckMenuItem    (USER.154)
 */
BOOL16 WINAPI CheckMenuItem16( HMENU16 hMenu, UINT16 id, UINT16 flags )
{
    return CheckMenuItem( HMENU_32(hMenu), id, flags );
}


/**********************************************************************
 *         EnableMenuItem    (USER.155)
 */
BOOL16 WINAPI EnableMenuItem16( HMENU16 hMenu, UINT16 wItemID, UINT16 wFlags )
{
    return EnableMenuItem( HMENU_32(hMenu), wItemID, wFlags );
}


/**********************************************************************
 *         GetSubMenu    (USER.159)
 */
HMENU16 WINAPI GetSubMenu16( HMENU16 hMenu, INT16 nPos )
{
    return HMENU_16( GetSubMenu( HMENU_32(hMenu), nPos ) );
}


/*******************************************************************
 *         GetMenuString    (USER.161)
 */
INT16 WINAPI GetMenuString16( HMENU16 hMenu, UINT16 wItemID,
                              LPSTR str, INT16 nMaxSiz, UINT16 wFlags )
{
    return GetMenuStringA( HMENU_32(hMenu), wItemID, str, nMaxSiz, wFlags );
}


/**********************************************************************
 *		WinHelp (USER.171)
 */
BOOL16 WINAPI WinHelp16( HWND16 hWnd, LPCSTR lpHelpFile, UINT16 wCommand,
                         DWORD dwData )
{
    BOOL ret;
    DWORD mutex_count;

    /* We might call WinExec() */
    ReleaseThunkLock(&mutex_count);

    ret = WinHelpA(WIN_Handle32(hWnd), lpHelpFile, wCommand, (DWORD)MapSL(dwData));

    RestoreThunkLock(mutex_count);
    return ret;
}


/***********************************************************************
 *		LoadCursor (USER.173)
 */
HCURSOR16 WINAPI LoadCursor16(HINSTANCE16 hInstance, LPCSTR name)
{
  return HCURSOR_16(LoadCursorA(HINSTANCE_32(hInstance), name));
}


/***********************************************************************
 *		LoadIcon (USER.174)
 */
HICON16 WINAPI LoadIcon16(HINSTANCE16 hInstance, LPCSTR name)
{
  return HICON_16(LoadIconA(HINSTANCE_32(hInstance), name));
}

/**********************************************************************
 *		LoadBitmap (USER.175)
 */
HBITMAP16 WINAPI LoadBitmap16(HINSTANCE16 hInstance, LPCSTR name)
{
  return HBITMAP_16(LoadBitmapA(HINSTANCE_32(hInstance), name));
}


/***********************************************************************
 *		GetSystemMetrics (USER.179)
 */
INT16 WINAPI GetSystemMetrics16( INT16 index )
{
    return GetSystemMetrics( index );
}


/*************************************************************************
 *		GetSysColor (USER.180)
 */
COLORREF WINAPI GetSysColor16( INT16 index )
{
    return GetSysColor( index );
}


/*************************************************************************
 *		SetSysColors (USER.181)
 */
VOID WINAPI SetSysColors16( INT16 count, const INT16 *list16, const COLORREF *values )
{
    INT i, *list;

    if ((list = HeapAlloc( GetProcessHeap(), 0, count * sizeof(*list) )))
    {
        for (i = 0; i < count; i++) list[i] = list16[i];
        SetSysColors( count, list, values );
        HeapFree( GetProcessHeap(), 0, list );
    }
}


/***********************************************************************
 *           GrayString   (USER.185)
 */
BOOL16 WINAPI GrayString16( HDC16 hdc, HBRUSH16 hbr, GRAYSTRINGPROC16 gsprc,
                            LPARAM lParam, INT16 cch, INT16 x, INT16 y,
                            INT16 cx, INT16 cy )
{
    BOOL ret;

    if (!gsprc) return GrayStringA( HDC_32(hdc), HBRUSH_32(hbr), NULL,
                                    (LPARAM)MapSL(lParam), cch, x, y, cx, cy );

    if (cch == -1 || (cch && cx && cy))
    {
        /* lParam can be treated as an opaque pointer */
        struct gray_string_info info;

        info.proc  = gsprc;
        info.param = lParam;
        ret = GrayStringA( HDC_32(hdc), HBRUSH_32(hbr), gray_string_callback,
                           (LPARAM)&info, cch, x, y, cx, cy );
    }
    else  /* here we need some string conversions */
    {
        char *str16 = MapSL(lParam);
        struct gray_string_info *info;

        if (!cch) cch = strlen(str16);
        if (!(info = HeapAlloc( GetProcessHeap(), 0, sizeof(*info) + cch ))) return FALSE;
        info->proc  = gsprc;
        info->param = lParam;
        memcpy( info->str, str16, cch );
        ret = GrayStringA( HDC_32(hdc), HBRUSH_32(hbr), gray_string_callback_ptr,
                           (LPARAM)info->str, cch, x, y, cx, cy );
        HeapFree( GetProcessHeap(), 0, info );
    }
    return ret;
}


/***********************************************************************
 *		SwapMouseButton (USER.186)
 */
BOOL16 WINAPI SwapMouseButton16( BOOL16 fSwap )
{
    return SwapMouseButton( fSwap );
}


/**************************************************************************
 *		IsClipboardFormatAvailable (USER.193)
 */
BOOL16 WINAPI IsClipboardFormatAvailable16( UINT16 wFormat )
{
    return IsClipboardFormatAvailable( wFormat );
}


/***********************************************************************
 *           TabbedTextOut    (USER.196)
 */
LONG WINAPI TabbedTextOut16( HDC16 hdc, INT16 x, INT16 y, LPCSTR lpstr,
                             INT16 count, INT16 nb_tabs, const INT16 *tabs16, INT16 tab_org )
{
    LONG ret;
    INT i, *tabs = HeapAlloc( GetProcessHeap(), 0, nb_tabs * sizeof(tabs) );
    if (!tabs) return 0;
    for (i = 0; i < nb_tabs; i++) tabs[i] = tabs16[i];
    ret = TabbedTextOutA( HDC_32(hdc), x, y, lpstr, count, nb_tabs, tabs, tab_org );
    HeapFree( GetProcessHeap(), 0, tabs );
    return ret;
}


/***********************************************************************
 *           GetTabbedTextExtent    (USER.197)
 */
DWORD WINAPI GetTabbedTextExtent16( HDC16 hdc, LPCSTR lpstr, INT16 count,
                                    INT16 nb_tabs, const INT16 *tabs16 )
{
    LONG ret;
    INT i, *tabs = HeapAlloc( GetProcessHeap(), 0, nb_tabs * sizeof(tabs) );
    if (!tabs) return 0;
    for (i = 0; i < nb_tabs; i++) tabs[i] = tabs16[i];
    ret = GetTabbedTextExtentA( HDC_32(hdc), lpstr, count, nb_tabs, tabs );
    HeapFree( GetProcessHeap(), 0, tabs );
    return ret;
}


/***********************************************************************
 *		UserSeeUserDo (USER.216)
 */
DWORD WINAPI UserSeeUserDo16(WORD wReqType, WORD wParam1, WORD wParam2, WORD wParam3)
{
    STACK16FRAME* stack16 = MapSL((SEGPTR)NtCurrentTeb()->WOW32Reserved);
    HANDLE16 oldDS = stack16->ds;
    DWORD ret = (DWORD)-1;

    stack16->ds = USER_HeapSel;
    switch (wReqType)
    {
    case USUD_LOCALALLOC:
        ret = LocalAlloc16(wParam1, wParam3);
        break;
    case USUD_LOCALFREE:
        ret = LocalFree16(wParam1);
        break;
    case USUD_LOCALCOMPACT:
        ret = LocalCompact16(wParam3);
        break;
    case USUD_LOCALHEAP:
        ret = USER_HeapSel;
        break;
    case USUD_FIRSTCLASS:
        FIXME("return a pointer to the first window class.\n");
        break;
    default:
        WARN("wReqType %04x (unknown)\n", wReqType);
    }
    stack16->ds = oldDS;
    return ret;
}


/*************************************************************************
 *		ScrollDC (USER.221)
 */
BOOL16 WINAPI ScrollDC16( HDC16 hdc, INT16 dx, INT16 dy, const RECT16 *rect,
                          const RECT16 *cliprc, HRGN16 hrgnUpdate,
                          LPRECT16 rcUpdate )
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
    if (cliprc)
    {
        clipRect32.left   = cliprc->left;
        clipRect32.top    = cliprc->top;
        clipRect32.right  = cliprc->right;
        clipRect32.bottom = cliprc->bottom;
    }
    ret = ScrollDC( HDC_32(hdc), dx, dy, rect ? &rect32 : NULL,
                    cliprc ? &clipRect32 : NULL, HRGN_32(hrgnUpdate),
                    &rcUpdate32 );
    if (rcUpdate)
    {
        rcUpdate->left   = rcUpdate32.left;
        rcUpdate->top    = rcUpdate32.top;
        rcUpdate->right  = rcUpdate32.right;
        rcUpdate->bottom = rcUpdate32.bottom;
    }
    return ret;
}


/***********************************************************************
 *		GetSystemDebugState (USER.231)
 */
WORD WINAPI GetSystemDebugState16(void)
{
    return 0;  /* FIXME */
}


/***********************************************************************
 *		EqualRect (USER.244)
 */
BOOL16 WINAPI EqualRect16( const RECT16* rect1, const RECT16* rect2 )
{
    return ((rect1->left == rect2->left) && (rect1->right == rect2->right) &&
            (rect1->top == rect2->top) && (rect1->bottom == rect2->bottom));
}


/***********************************************************************
 *		ExitWindowsExec (USER.246)
 */
BOOL16 WINAPI ExitWindowsExec16( LPCSTR lpszExe, LPCSTR lpszParams )
{
    TRACE("Should run the following in DOS-mode: \"%s %s\"\n",
          lpszExe, lpszParams);
    return ExitWindowsEx( EWX_LOGOFF, 0xffffffff );
}


/***********************************************************************
 *		GetCursor (USER.247)
 */
HCURSOR16 WINAPI GetCursor16(void)
{
  return HCURSOR_16(GetCursor());
}


/**********************************************************************
 *		GetAsyncKeyState (USER.249)
 */
INT16 WINAPI GetAsyncKeyState16( INT16 key )
{
    return GetAsyncKeyState( key );
}


/**********************************************************************
 *         GetMenuState    (USER.250)
 */
UINT16 WINAPI GetMenuState16( HMENU16 hMenu, UINT16 wItemID, UINT16 wFlags )
{
    return GetMenuState( HMENU_32(hMenu), wItemID, wFlags );
}


/**********************************************************************
 *         GetMenuItemCount    (USER.263)
 */
INT16 WINAPI GetMenuItemCount16( HMENU16 hMenu )
{
    return GetMenuItemCount( HMENU_32(hMenu) );
}


/**********************************************************************
 *         GetMenuItemID    (USER.264)
 */
UINT16 WINAPI GetMenuItemID16( HMENU16 hMenu, INT16 nPos )
{
    return GetMenuItemID( HMENU_32(hMenu), nPos );
}


/***********************************************************************
 *		GlobalAddAtom (USER.268)
 */
ATOM WINAPI GlobalAddAtom16(LPCSTR lpString)
{
  return GlobalAddAtomA(lpString);
}

/***********************************************************************
 *		GlobalDeleteAtom (USER.269)
 */
ATOM WINAPI GlobalDeleteAtom16(ATOM nAtom)
{
  return GlobalDeleteAtom(nAtom);
}

/***********************************************************************
 *		GlobalFindAtom (USER.270)
 */
ATOM WINAPI GlobalFindAtom16(LPCSTR lpString)
{
  return GlobalFindAtomA(lpString);
}

/***********************************************************************
 *		GlobalGetAtomName (USER.271)
 */
UINT16 WINAPI GlobalGetAtomName16(ATOM nAtom, LPSTR lpBuffer, INT16 nSize)
{
  return GlobalGetAtomNameA(nAtom, lpBuffer, nSize);
}


/***********************************************************************
 *		ControlPanelInfo (USER.273)
 */
void WINAPI ControlPanelInfo16( INT16 nInfoType, WORD wData, LPSTR lpBuffer )
{
    FIXME("(%d, %04x, %p): stub.\n", nInfoType, wData, lpBuffer);
}


/***********************************************************************
 *           OldSetDeskPattern   (USER.279)
 */
BOOL16 WINAPI SetDeskPattern16(void)
{
    return SystemParametersInfoA( SPI_SETDESKPATTERN, -1, NULL, FALSE );
}


/***********************************************************************
 *		GetSysColorBrush (USER.281)
 */
HBRUSH16 WINAPI GetSysColorBrush16( INT16 index )
{
    return HBRUSH_16( GetSysColorBrush(index) );
}


/***********************************************************************
 *		SelectPalette (USER.282)
 */
HPALETTE16 WINAPI SelectPalette16( HDC16 hdc, HPALETTE16 hpal, BOOL16 bForceBackground )
{
    return HPALETTE_16( SelectPalette( HDC_32(hdc), HPALETTE_32(hpal), bForceBackground ));
}

/***********************************************************************
 *		RealizePalette (USER.283)
 */
UINT16 WINAPI RealizePalette16( HDC16 hdc )
{
    return UserRealizePalette( HDC_32(hdc) );
}


/***********************************************************************
 *		GetFreeSystemResources (USER.284)
 */
WORD WINAPI GetFreeSystemResources16( WORD resType )
{
    STACK16FRAME* stack16 = MapSL((SEGPTR)NtCurrentTeb()->WOW32Reserved);
    HANDLE16 oldDS = stack16->ds;
    HINSTANCE16 gdi_inst;
    int userPercent, gdiPercent;

    if ((gdi_inst = LoadLibrary16( "GDI" )) < 32) return 0;

    switch(resType)
    {
    case GFSR_USERRESOURCES:
        stack16->ds = USER_HeapSel;
        userPercent = (int)LocalCountFree16() * 100 / LocalHeapSize16();
        gdiPercent  = 100;
        stack16->ds = oldDS;
        break;

    case GFSR_GDIRESOURCES:
        stack16->ds = gdi_inst;
        gdiPercent  = (int)LocalCountFree16() * 100 / LocalHeapSize16();
        userPercent = 100;
        stack16->ds = oldDS;
        break;

    case GFSR_SYSTEMRESOURCES:
        stack16->ds = USER_HeapSel;
        userPercent = (int)LocalCountFree16() * 100 / LocalHeapSize16();
        stack16->ds = gdi_inst;
        gdiPercent  = (int)LocalCountFree16() * 100 / LocalHeapSize16();
        stack16->ds = oldDS;
        break;

    default:
        userPercent = gdiPercent = 0;
        break;
    }
    FreeLibrary16( gdi_inst );
    TRACE("<- userPercent %d, gdiPercent %d\n", userPercent, gdiPercent);
    return (WORD)min( userPercent, gdiPercent );
}


/***********************************************************************
 *           SetDeskWallPaper   (USER.285)
 */
BOOL16 WINAPI SetDeskWallPaper16( LPCSTR filename )
{
    return SetDeskWallPaper( filename );
}


/***********************************************************************
 *		keybd_event (USER.289)
 */
void WINAPI keybd_event16( CONTEXT86 *context )
{
    DWORD dwFlags = 0;

    if (HIBYTE(context->Eax) & 0x80) dwFlags |= KEYEVENTF_KEYUP;
    if (HIBYTE(context->Ebx) & 0x01) dwFlags |= KEYEVENTF_EXTENDEDKEY;

    keybd_event( LOBYTE(context->Eax), LOBYTE(context->Ebx),
                 dwFlags, MAKELONG(LOWORD(context->Esi), LOWORD(context->Edi)) );
}


/***********************************************************************
 *		mouse_event (USER.299)
 */
void WINAPI mouse_event16( CONTEXT86 *context )
{
    mouse_event( LOWORD(context->Eax), LOWORD(context->Ebx), LOWORD(context->Ecx),
                 LOWORD(context->Edx), MAKELONG(context->Esi, context->Edi) );
}


/***********************************************************************
 *		GetClipCursor (USER.309)
 */
void WINAPI GetClipCursor16( RECT16 *rect )
{
    if (rect)
    {
        RECT rect32;
        GetClipCursor( &rect32 );
        rect->left   = rect32.left;
        rect->top    = rect32.top;
        rect->right  = rect32.right;
        rect->bottom = rect32.bottom;
    }
}


/***********************************************************************
 *		SignalProc (USER.314)
 */
void WINAPI SignalProc16( HANDLE16 hModule, UINT16 code,
                          UINT16 uExitFn, HINSTANCE16 hInstance, HQUEUE16 hQueue )
{
    if (code == USIG16_DLL_UNLOAD)
    {
        /* HOOK_FreeModuleHooks( hModule ); */
        CLASS_FreeModuleClasses( hModule );
        CURSORICON_FreeModuleIcons( hModule );
    }
}


/***********************************************************************
 *		SetEventHook (USER.321)
 *
 *	Used by Turbo Debugger for Windows
 */
FARPROC16 WINAPI SetEventHook16(FARPROC16 lpfnEventHook)
{
    FIXME("(lpfnEventHook=%p): stub\n", lpfnEventHook);
    return 0;
}


/**********************************************************************
 *		EnableHardwareInput (USER.331)
 */
BOOL16 WINAPI EnableHardwareInput16(BOOL16 bEnable)
{
    FIXME("(%d) - stub\n", bEnable);
    return TRUE;
}


/***********************************************************************
 *		GetMouseEventProc (USER.337)
 */
FARPROC16 WINAPI GetMouseEventProc16(void)
{
    HMODULE16 hmodule = GetModuleHandle16("USER");
    return GetProcAddress16( hmodule, "mouse_event" );
}


/***********************************************************************
 *		IsUserIdle (USER.333)
 */
BOOL16 WINAPI IsUserIdle16(void)
{
    if ( GetAsyncKeyState( VK_LBUTTON ) & 0x8000 )
        return FALSE;
    if ( GetAsyncKeyState( VK_RBUTTON ) & 0x8000 )
        return FALSE;
    if ( GetAsyncKeyState( VK_MBUTTON ) & 0x8000 )
        return FALSE;
    /* Should check for screen saver activation here ... */
    return TRUE;
}


/**********************************************************************
 *              LoadDIBIconHandler (USER.357)
 *
 * RT_ICON resource loader, installed by USER_SignalProc when module
 * is initialized.
 */
HGLOBAL16 WINAPI LoadDIBIconHandler16( HGLOBAL16 hMemObj, HMODULE16 hModule, HRSRC16 hRsrc )
{
    /* If hResource is zero we must allocate a new memory block, if it's
     * non-zero but GlobalLock() returns NULL then it was discarded and
     * we have to recommit some memory, otherwise we just need to check
     * the block size. See LoadProc() in 16-bit SDK for more.
     */
    FIXME( "%x %x %x: stub, not supported anymore\n", hMemObj, hModule, hRsrc );
    return 0;
}

/**********************************************************************
 *		LoadDIBCursorHandler (USER.356)
 *
 * RT_CURSOR resource loader. Same as above.
 */
HGLOBAL16 WINAPI LoadDIBCursorHandler16( HGLOBAL16 hMemObj, HMODULE16 hModule, HRSRC16 hRsrc )
{
    FIXME( "%x %x %x: stub, not supported anymore\n", hMemObj, hModule, hRsrc );
    return 0;
}


/**********************************************************************
 *		IsMenu    (USER.358)
 */
BOOL16 WINAPI IsMenu16( HMENU16 hmenu )
{
    return IsMenu( HMENU_32(hmenu) );
}


/***********************************************************************
 *		DCHook (USER.362)
 */
BOOL16 WINAPI DCHook16( HDC16 hdc, WORD code, DWORD data, LPARAM lParam )
{
    FIXME( "hDC = %x, %i: stub\n", hdc, code );
    return FALSE;
}


/***********************************************************************
 *		SubtractRect (USER.373)
 */
BOOL16 WINAPI SubtractRect16( LPRECT16 dest, const RECT16 *src1,
                              const RECT16 *src2 )
{
    RECT16 tmp;

    if (IsRectEmpty16( src1 ))
    {
        SetRectEmpty16( dest );
        return FALSE;
    }
    *dest = *src1;
    if (IntersectRect16( &tmp, src1, src2 ))
    {
        if (EqualRect16( &tmp, dest ))
        {
            SetRectEmpty16( dest );
            return FALSE;
        }
        if ((tmp.top == dest->top) && (tmp.bottom == dest->bottom))
        {
            if (tmp.left == dest->left) dest->left = tmp.right;
            else if (tmp.right == dest->right) dest->right = tmp.left;
        }
        else if ((tmp.left == dest->left) && (tmp.right == dest->right))
        {
            if (tmp.top == dest->top) dest->top = tmp.bottom;
            else if (tmp.bottom == dest->bottom) dest->bottom = tmp.top;
        }
    }
    return TRUE;
}


/**********************************************************************
 *         SetMenuContextHelpId    (USER.384)
 */
BOOL16 WINAPI SetMenuContextHelpId16( HMENU16 hMenu, DWORD dwContextHelpID)
{
    return SetMenuContextHelpId( HMENU_32(hMenu), dwContextHelpID );
}


/**********************************************************************
 *         GetMenuContextHelpId    (USER.385)
 */
DWORD WINAPI GetMenuContextHelpId16( HMENU16 hMenu )
{
    return GetMenuContextHelpId( HMENU_32(hMenu) );
}


/***********************************************************************
 *		LoadImage (USER.389)
 *
 */
HANDLE16 WINAPI LoadImage16(HINSTANCE16 hinst, LPCSTR name, UINT16 type,
			    INT16 desiredx, INT16 desiredy, UINT16 loadflags)
{
  return HANDLE_16(LoadImageA(HINSTANCE_32(hinst), name, type, desiredx,
			      desiredy, loadflags));
}

/******************************************************************************
 *		CopyImage (USER.390) Creates new image and copies attributes to it
 *
 */
HICON16 WINAPI CopyImage16(HANDLE16 hnd, UINT16 type, INT16 desiredx,
			   INT16 desiredy, UINT16 flags)
{
  return HICON_16(CopyImage(HANDLE_32(hnd), (UINT)type, (INT)desiredx,
			    (INT)desiredy, (UINT)flags));
}

/**********************************************************************
 *		DrawIconEx (USER.394)
 */
BOOL16 WINAPI DrawIconEx16(HDC16 hdc, INT16 xLeft, INT16 yTop, HICON16 hIcon,
			   INT16 cxWidth, INT16 cyWidth, UINT16 istep,
			   HBRUSH16 hbr, UINT16 flags)
{
  return DrawIconEx(HDC_32(hdc), xLeft, yTop, HICON_32(hIcon), cxWidth, cyWidth,
		    istep, HBRUSH_32(hbr), flags);
}

/**********************************************************************
 *		GetIconInfo (USER.395)
 */
BOOL16 WINAPI GetIconInfo16(HICON16 hIcon, LPICONINFO16 iconinfo)
{
  ICONINFO ii32;
  BOOL16 ret = GetIconInfo(HICON_32(hIcon), &ii32);

  iconinfo->fIcon = ii32.fIcon;
  iconinfo->xHotspot = ii32.xHotspot;
  iconinfo->yHotspot = ii32.yHotspot;
  iconinfo->hbmMask  = HBITMAP_16(ii32.hbmMask);
  iconinfo->hbmColor = HBITMAP_16(ii32.hbmColor);
  return ret;
}


/***********************************************************************
 *		FinalUserInit (USER.400)
 */
void WINAPI FinalUserInit16( void )
{
    /* FIXME: Should chain to FinalGdiInit */
}


/***********************************************************************
 *		CreateCursor (USER.406)
 */
HCURSOR16 WINAPI CreateCursor16(HINSTANCE16 hInstance,
				INT16 xHotSpot, INT16 yHotSpot,
				INT16 nWidth, INT16 nHeight,
				LPCVOID lpANDbits, LPCVOID lpXORbits)
{
  CURSORICONINFO info;

  info.ptHotSpot.x = xHotSpot;
  info.ptHotSpot.y = yHotSpot;
  info.nWidth = nWidth;
  info.nHeight = nHeight;
  info.nWidthBytes = 0;
  info.bPlanes = 1;
  info.bBitsPerPixel = 1;

  return CreateCursorIconIndirect16(hInstance, &info, lpANDbits, lpXORbits);
}


/***********************************************************************
 *		InitThreadInput   (USER.409)
 */
HQUEUE16 WINAPI InitThreadInput16( WORD unknown, WORD flags )
{
    /* nothing to do here */
    return 0xbeef;
}


/*******************************************************************
 *         InsertMenu    (USER.410)
 */
BOOL16 WINAPI InsertMenu16( HMENU16 hMenu, UINT16 pos, UINT16 flags,
                            UINT16 id, SEGPTR data )
{
    UINT pos32 = (UINT)pos;
    if ((pos == (UINT16)-1) && (flags & MF_BYPOSITION)) pos32 = (UINT)-1;
    if (IS_MENU_STRING_ITEM(flags) && data)
        return InsertMenuA( HMENU_32(hMenu), pos32, flags, id, MapSL(data) );
    return InsertMenuA( HMENU_32(hMenu), pos32, flags, id, (LPSTR)data );
}


/*******************************************************************
 *         AppendMenu    (USER.411)
 */
BOOL16 WINAPI AppendMenu16(HMENU16 hMenu, UINT16 flags, UINT16 id, SEGPTR data)
{
    return InsertMenu16( hMenu, -1, flags | MF_BYPOSITION, id, data );
}


/**********************************************************************
 *         RemoveMenu   (USER.412)
 */
BOOL16 WINAPI RemoveMenu16( HMENU16 hMenu, UINT16 nPos, UINT16 wFlags )
{
    return RemoveMenu( HMENU_32(hMenu), nPos, wFlags );
}


/**********************************************************************
 *         DeleteMenu    (USER.413)
 */
BOOL16 WINAPI DeleteMenu16( HMENU16 hMenu, UINT16 nPos, UINT16 wFlags )
{
    return DeleteMenu( HMENU_32(hMenu), nPos, wFlags );
}


/*******************************************************************
 *         ModifyMenu    (USER.414)
 */
BOOL16 WINAPI ModifyMenu16( HMENU16 hMenu, UINT16 pos, UINT16 flags,
                            UINT16 id, SEGPTR data )
{
    if (IS_MENU_STRING_ITEM(flags))
        return ModifyMenuA( HMENU_32(hMenu), pos, flags, id, MapSL(data) );
    return ModifyMenuA( HMENU_32(hMenu), pos, flags, id, (LPSTR)data );
}


/**********************************************************************
 *         CreatePopupMenu    (USER.415)
 */
HMENU16 WINAPI CreatePopupMenu16(void)
{
    return HMENU_16( CreatePopupMenu() );
}


/**********************************************************************
 *         SetMenuItemBitmaps    (USER.418)
 */
BOOL16 WINAPI SetMenuItemBitmaps16( HMENU16 hMenu, UINT16 nPos, UINT16 wFlags,
                                    HBITMAP16 hNewUnCheck, HBITMAP16 hNewCheck)
{
    return SetMenuItemBitmaps( HMENU_32(hMenu), nPos, wFlags,
                               HBITMAP_32(hNewUnCheck), HBITMAP_32(hNewCheck) );
}


/***********************************************************************
 *           lstrcmp   (USER.430)
 */
INT16 WINAPI lstrcmp16( LPCSTR str1, LPCSTR str2 )
{
    return strcmp( str1, str2 );
}


/***********************************************************************
 *           AnsiUpper   (USER.431)
 */
SEGPTR WINAPI AnsiUpper16( SEGPTR strOrChar )
{
    /* uppercase only one char if strOrChar < 0x10000 */
    if (HIWORD(strOrChar))
    {
        CharUpperA( MapSL(strOrChar) );
        return strOrChar;
    }
    else return (SEGPTR)CharUpperA( (LPSTR)strOrChar );
}


/***********************************************************************
 *           AnsiLower   (USER.432)
 */
SEGPTR WINAPI AnsiLower16( SEGPTR strOrChar )
{
    /* lowercase only one char if strOrChar < 0x10000 */
    if (HIWORD(strOrChar))
    {
        CharLowerA( MapSL(strOrChar) );
        return strOrChar;
    }
    else return (SEGPTR)CharLowerA( (LPSTR)strOrChar );
}


/***********************************************************************
 *           AnsiUpperBuff   (USER.437)
 */
UINT16 WINAPI AnsiUpperBuff16( LPSTR str, UINT16 len )
{
    CharUpperBuffA( str, len ? len : 65536 );
    return len;
}


/***********************************************************************
 *           AnsiLowerBuff   (USER.438)
 */
UINT16 WINAPI AnsiLowerBuff16( LPSTR str, UINT16 len )
{
    CharLowerBuffA( str, len ? len : 65536 );
    return len;
}


/*******************************************************************
 *              InsertMenuItem   (USER.441)
 *
 * FIXME: untested
 */
BOOL16 WINAPI InsertMenuItem16( HMENU16 hmenu, UINT16 pos, BOOL16 byposition,
                                const MENUITEMINFO16 *mii )
{
    MENUITEMINFOA miia;

    miia.cbSize        = sizeof(miia);
    miia.fMask         = mii->fMask;
    miia.dwTypeData    = (LPSTR)mii->dwTypeData;
    miia.fType         = mii->fType;
    miia.fState        = mii->fState;
    miia.wID           = mii->wID;
    miia.hSubMenu      = HMENU_32(mii->hSubMenu);
    miia.hbmpChecked   = HBITMAP_32(mii->hbmpChecked);
    miia.hbmpUnchecked = HBITMAP_32(mii->hbmpUnchecked);
    miia.dwItemData    = mii->dwItemData;
    miia.cch           = mii->cch;
    if (IS_MENU_STRING_ITEM(miia.fType))
        miia.dwTypeData = MapSL(mii->dwTypeData);
    return InsertMenuItemA( HMENU_32(hmenu), pos, byposition, &miia );
}


/**********************************************************************
 *	     DrawState    (USER.449)
 */
BOOL16 WINAPI DrawState16( HDC16 hdc, HBRUSH16 hbr, DRAWSTATEPROC16 func, LPARAM ldata,
                           WPARAM16 wdata, INT16 x, INT16 y, INT16 cx, INT16 cy, UINT16 flags )
{
    struct draw_state_info info;
    UINT opcode = flags & 0xf;

    if (opcode == DST_TEXT || opcode == DST_PREFIXTEXT)
    {
        /* make sure DrawStateA doesn't try to use ldata as a pointer */
        if (!wdata) wdata = strlen( MapSL(ldata) );
        if (!cx || !cy)
        {
            SIZE s;
            if (!GetTextExtentPoint32A( HDC_32(hdc), MapSL(ldata), wdata, &s )) return FALSE;
            if (!cx) cx = s.cx;
            if (!cy) cy = s.cy;
        }
    }
    info.proc  = func;
    info.param = ldata;
    return DrawStateA( HDC_32(hdc), HBRUSH_32(hbr), draw_state_callback,
                       (LPARAM)&info, wdata, x, y, cx, cy, flags );
}


/**********************************************************************
 *		CreateIconFromResourceEx (USER.450)
 *
 * FIXME: not sure about exact parameter types
 */
HICON16 WINAPI CreateIconFromResourceEx16(LPBYTE bits, UINT16 cbSize,
					  BOOL16 bIcon, DWORD dwVersion,
					  INT16 width, INT16 height,
					  UINT16 cFlag)
{
  return HICON_16(CreateIconFromResourceEx(bits, cbSize, bIcon, dwVersion,
					   width, height, cFlag));
}


/***********************************************************************
 *		AdjustWindowRectEx (USER.454)
 */
BOOL16 WINAPI AdjustWindowRectEx16( LPRECT16 rect, DWORD style, BOOL16 menu, DWORD exStyle )
{
    RECT rect32;
    BOOL ret;

    rect32.left   = rect->left;
    rect32.top    = rect->top;
    rect32.right  = rect->right;
    rect32.bottom = rect->bottom;
    ret = AdjustWindowRectEx( &rect32, style, menu, exStyle );
    rect->left   = rect32.left;
    rect->top    = rect32.top;
    rect->right  = rect32.right;
    rect->bottom = rect32.bottom;
    return ret;
}


/***********************************************************************
 *		DestroyIcon (USER.457)
 */
BOOL16 WINAPI DestroyIcon16(HICON16 hIcon)
{
  return DestroyIcon32(hIcon, 0);
}

/***********************************************************************
 *		DestroyCursor (USER.458)
 */
BOOL16 WINAPI DestroyCursor16(HCURSOR16 hCursor)
{
  return DestroyIcon32(hCursor, 0);
}

/*******************************************************************
 *			DRAG_QueryUpdate16
 *
 * Recursively find a child that contains spDragInfo->pt point
 * and send WM_QUERYDROPOBJECT. Helper for DragObject16.
 */
static BOOL DRAG_QueryUpdate16( HWND hQueryWnd, SEGPTR spDragInfo )
{
    BOOL bResult = 0;
    WPARAM wParam;
    POINT pt, old_pt;
    LPDRAGINFO16 ptrDragInfo = MapSL(spDragInfo);
    RECT tempRect;
    HWND child;

    if (!IsWindowEnabled(hQueryWnd)) return FALSE;

    old_pt.x = ptrDragInfo->pt.x;
    old_pt.y = ptrDragInfo->pt.y;
    pt = old_pt;
    ScreenToClient( hQueryWnd, &pt );
    child = ChildWindowFromPointEx( hQueryWnd, pt, CWP_SKIPINVISIBLE );
    if (!child) return FALSE;

    if (child != hQueryWnd)
    {
        wParam = 0;
        if (DRAG_QueryUpdate16( child, spDragInfo )) return TRUE;
    }
    else
    {
        GetClientRect( hQueryWnd, &tempRect );
        wParam = !PtInRect( &tempRect, pt );
    }

    ptrDragInfo->pt.x = pt.x;
    ptrDragInfo->pt.y = pt.y;
    ptrDragInfo->hScope = HWND_16(hQueryWnd);

    bResult = SendMessage16( HWND_16(hQueryWnd), WM_QUERYDROPOBJECT, wParam, spDragInfo );

    if (!bResult)
    {
        ptrDragInfo->pt.x = old_pt.x;
        ptrDragInfo->pt.y = old_pt.y;
    }
    return bResult;
}


/******************************************************************************
 *		DragObject (USER.464)
 */
DWORD WINAPI DragObject16( HWND16 hwndScope, HWND16 hWnd, UINT16 wObj,
                           HANDLE16 hOfStruct, WORD szList, HCURSOR16 hCursor )
{
    MSG	msg;
    LPDRAGINFO16 lpDragInfo;
    SEGPTR	spDragInfo;
    HCURSOR 	hOldCursor=0, hBummer=0;
    HGLOBAL16	hDragInfo  = GlobalAlloc16( GMEM_SHARE | GMEM_ZEROINIT, 2*sizeof(DRAGINFO16));
    HCURSOR	hCurrentCursor = 0;
    HWND16	hCurrentWnd = 0;

    lpDragInfo = (LPDRAGINFO16) GlobalLock16(hDragInfo);
    spDragInfo = WOWGlobalLock16(hDragInfo);

    if( !lpDragInfo || !spDragInfo ) return 0L;

    if (!(hBummer = LoadCursorA(0, MAKEINTRESOURCEA(OCR_NO))))
    {
        GlobalFree16(hDragInfo);
        return 0L;
    }

    if(hCursor) hOldCursor = SetCursor(HCURSOR_32(hCursor));

    lpDragInfo->hWnd   = hWnd;
    lpDragInfo->hScope = 0;
    lpDragInfo->wFlags = wObj;
    lpDragInfo->hList  = szList; /* near pointer! */
    lpDragInfo->hOfStruct = hOfStruct;
    lpDragInfo->l = 0L;

    SetCapture( HWND_32(hWnd) );
    ShowCursor( TRUE );

    do
    {
        GetMessageW( &msg, 0, WM_MOUSEFIRST, WM_MOUSELAST );

       *(lpDragInfo+1) = *lpDragInfo;

	lpDragInfo->pt.x = msg.pt.x;
	lpDragInfo->pt.y = msg.pt.y;

	/* update DRAGINFO struct */
	if( DRAG_QueryUpdate16(WIN_Handle32(hwndScope), spDragInfo) > 0 )
	    hCurrentCursor = HCURSOR_32(hCursor);
	else
        {
            hCurrentCursor = hBummer;
            lpDragInfo->hScope = 0;
	}
	if( hCurrentCursor )
	    SetCursor(hCurrentCursor);

	/* send WM_DRAGLOOP */
	SendMessage16( hWnd, WM_DRAGLOOP, (WPARAM16)(hCurrentCursor != hBummer),
	                                  (LPARAM) spDragInfo );
	/* send WM_DRAGSELECT or WM_DRAGMOVE */
	if( hCurrentWnd != lpDragInfo->hScope )
	{
	    if( hCurrentWnd )
	        SendMessage16( hCurrentWnd, WM_DRAGSELECT, 0,
		       (LPARAM)MAKELONG(LOWORD(spDragInfo)+sizeof(DRAGINFO16),
				        HIWORD(spDragInfo)) );
	    hCurrentWnd = lpDragInfo->hScope;
	    if( hCurrentWnd )
                SendMessage16( hCurrentWnd, WM_DRAGSELECT, 1, (LPARAM)spDragInfo);
	}
	else
	    if( hCurrentWnd )
	        SendMessage16( hCurrentWnd, WM_DRAGMOVE, 0, (LPARAM)spDragInfo);

    } while( msg.message != WM_LBUTTONUP && msg.message != WM_NCLBUTTONUP );

    ReleaseCapture();
    ShowCursor( FALSE );

    if( hCursor ) SetCursor(hOldCursor);

    if( hCurrentCursor != hBummer )
	msg.lParam = SendMessage16( lpDragInfo->hScope, WM_DROPOBJECT,
				   (WPARAM16)hWnd, (LPARAM)spDragInfo );
    else
        msg.lParam = 0;
    GlobalFree16(hDragInfo);

    return (DWORD)(msg.lParam);
}


/***********************************************************************
 *		DrawFocusRect (USER.466)
 */
void WINAPI DrawFocusRect16( HDC16 hdc, const RECT16* rc )
{
    RECT rect32;

    rect32.left   = rc->left;
    rect32.top    = rc->top;
    rect32.right  = rc->right;
    rect32.bottom = rc->bottom;
    DrawFocusRect( HDC_32(hdc), &rect32 );
}


/***********************************************************************
 *           AnsiNext   (USER.472)
 */
SEGPTR WINAPI AnsiNext16(SEGPTR current)
{
    char *ptr = MapSL(current);
    return current + (CharNextA(ptr) - ptr);
}


/***********************************************************************
 *           AnsiPrev   (USER.473)
 */
SEGPTR WINAPI AnsiPrev16( LPCSTR start, SEGPTR current )
{
    char *ptr = MapSL(current);
    return current - (ptr - CharPrevA( start, ptr ));
}


/****************************************************************************
 *		GetKeyboardLayoutName (USER.477)
 */
INT16 WINAPI GetKeyboardLayoutName16( LPSTR name )
{
    return GetKeyboardLayoutNameA( name );
}


/***********************************************************************
 *           FormatMessage   (USER.606)
 */
DWORD WINAPI FormatMessage16(
    DWORD   dwFlags,
    SEGPTR lpSource,     /* [in] NOTE: not always a valid pointer */
    WORD   dwMessageId,
    WORD   dwLanguageId,
    LPSTR  lpBuffer,     /* [out] NOTE: *((HLOCAL16*)) for FORMAT_MESSAGE_ALLOCATE_BUFFER*/
    WORD   nSize,
    LPDWORD args )       /* [in] NOTE: va_list *args */
{
#ifdef __i386__
/* This implementation is completely dependent on the format of the va_list on x86 CPUs */
    LPSTR       target,t;
    DWORD       talloced;
    LPSTR       from,f;
    DWORD       width = dwFlags & FORMAT_MESSAGE_MAX_WIDTH_MASK;
    BOOL        eos = FALSE;
    LPSTR       allocstring = NULL;

    TRACE("(0x%x,%x,%d,0x%x,%p,%d,%p)\n",
          dwFlags,lpSource,dwMessageId,dwLanguageId,lpBuffer,nSize,args);
        if ((dwFlags & FORMAT_MESSAGE_FROM_SYSTEM)
                && (dwFlags & FORMAT_MESSAGE_FROM_HMODULE)) return 0;
        if ((dwFlags & FORMAT_MESSAGE_FROM_STRING)
                &&((dwFlags & FORMAT_MESSAGE_FROM_SYSTEM)
                        || (dwFlags & FORMAT_MESSAGE_FROM_HMODULE))) return 0;

    if (width && width != FORMAT_MESSAGE_MAX_WIDTH_MASK)
        FIXME("line wrapping (%u) not supported.\n", width);
    from = NULL;
    if (dwFlags & FORMAT_MESSAGE_FROM_STRING)
    {
        char *source = MapSL(lpSource);
        from = HeapAlloc( GetProcessHeap(), 0, strlen(source)+1 );
        strcpy( from, source );
    }
    else if (dwFlags & FORMAT_MESSAGE_FROM_SYSTEM) {
        from = HeapAlloc( GetProcessHeap(),0,200 );
        sprintf(from,"Systemmessage, messageid = 0x%08x\n",dwMessageId);
    }
    else if (dwFlags & FORMAT_MESSAGE_FROM_HMODULE) {
        INT16   bufsize;
        HINSTANCE16 hinst16 = ((HINSTANCE16)lpSource & 0xffff);

        dwMessageId &= 0xFFFF;
        bufsize=LoadString16(hinst16,dwMessageId,NULL,0);
        if (bufsize) {
            from = HeapAlloc( GetProcessHeap(), 0, bufsize +1);
            LoadString16(hinst16,dwMessageId,from,bufsize+1);
        }
    }
    target      = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, 100);
    t   = target;
    talloced= 100;

#define ADD_TO_T(c) \
        *t++=c;\
        if (t-target == talloced) {\
                target  = HeapReAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,target,talloced*2);\
                t       = target+talloced;\
                talloced*=2;\
        }

    if (from) {
        f=from;
        while (*f && !eos) {
            if (*f=='%') {
                int     insertnr;
                char    *fmtstr,*x,*lastf;
                DWORD   *argliststart;

                fmtstr = NULL;
                lastf = f;
                f++;
                if (!*f) {
                    ADD_TO_T('%');
                    continue;
                }
                switch (*f) {
                case '1':case '2':case '3':case '4':case '5':
                case '6':case '7':case '8':case '9':
                    insertnr=*f-'0';
                    switch (f[1]) {
                    case '0':case '1':case '2':case '3':
                    case '4':case '5':case '6':case '7':
                    case '8':case '9':
                        f++;
                        insertnr=insertnr*10+*f-'0';
                        f++;
                        break;
                    default:
                        f++;
                        break;
                    }
                    if (*f=='!') {
                        f++;
                        if (NULL!=(x=strchr(f,'!'))) {
                            *x='\0';
                            fmtstr=HeapAlloc(GetProcessHeap(),0,strlen(f)+2);
                            sprintf(fmtstr,"%%%s",f);
                            f=x+1;
                        } else {
                            fmtstr=HeapAlloc(GetProcessHeap(),0,strlen(f)+2);
                            sprintf(fmtstr,"%%%s",f);
                            f+=strlen(f); /*at \0*/
                        }
                    }
                    else
                    {
                        if(!args) break;
                        fmtstr=HeapAlloc( GetProcessHeap(), 0, 3 );
                        strcpy( fmtstr, "%s" );
                    }
                    if (args) {
                        int     ret;
                        int     sz;
                        LPSTR   b = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sz = 100);

                        argliststart=args+insertnr-1;

                        /* CMF - This makes a BIG assumption about va_list */
                        while ((ret = vsnprintf(b, sz, fmtstr, (va_list) argliststart) < 0) || (ret >= sz)) {
                            sz = (ret == -1 ? sz + 100 : ret + 1);
                            b = HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, b, sz);
                        }
                        for (x=b; *x; x++) ADD_TO_T(*x);
                        HeapFree(GetProcessHeap(), 0, b);
                    } else {
                        /* NULL args - copy formatstr
                         * (probably wrong)
                         */
                        while ((lastf<f)&&(*lastf)) {
                            ADD_TO_T(*lastf++);
                        }
                    }
                    HeapFree(GetProcessHeap(),0,fmtstr);
                    break;
                case '0': /* Just stop processing format string */
                    eos = TRUE;
                    f++;
                    break;
                case 'n': /* 16 bit version just outputs 'n' */
                default:
                    ADD_TO_T(*f++);
                    break;
                }
            } else { /* '\n' or '\r' gets mapped to "\r\n" */
                if(*f == '\n' || *f == '\r') {
                    if (width == 0) {
                        ADD_TO_T('\r');
                        ADD_TO_T('\n');
                        if(*f++ == '\r' && *f == '\n')
                            f++;
                    }
                } else {
                    ADD_TO_T(*f++);
                }
            }
        }
        *t='\0';
    }
    talloced = strlen(target)+1;
    if (nSize && talloced<nSize) {
        target = HeapReAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,target,nSize);
    }
    TRACE("-- %s\n",debugstr_a(target));
    if (dwFlags & FORMAT_MESSAGE_ALLOCATE_BUFFER) {
        /* nSize is the MINIMUM size */
        HLOCAL16 h = LocalAlloc16(LPTR,talloced);
        SEGPTR ptr = LocalLock16(h);
        allocstring = MapSL( ptr );
        memcpy( allocstring,target,talloced);
        LocalUnlock16( h );
        *((HLOCAL16*)lpBuffer) = h;
    } else
        lstrcpynA(lpBuffer,target,nSize);
    HeapFree(GetProcessHeap(),0,target);
    HeapFree(GetProcessHeap(),0,from);
    return (dwFlags & FORMAT_MESSAGE_ALLOCATE_BUFFER) ?
        strlen(allocstring):
        strlen(lpBuffer);
#else
        return 0;
#endif /* __i386__ */
}
#undef ADD_TO_T


/***********************************************************************
 *		ChangeDisplaySettings (USER.620)
 */
LONG WINAPI ChangeDisplaySettings16( LPDEVMODEA devmode, DWORD flags )
{
    return ChangeDisplaySettingsA( devmode, flags );
}


/***********************************************************************
 *		EnumDisplaySettings (USER.621)
 */
BOOL16 WINAPI EnumDisplaySettings16( LPCSTR name, DWORD n, LPDEVMODEA devmode )
{
    return EnumDisplaySettingsA( name, n, devmode );
}

/**********************************************************************
 *          DrawFrameControl  (USER.656)
 */
BOOL16 WINAPI DrawFrameControl16( HDC16 hdc, LPRECT16 rc, UINT16 uType, UINT16 uState )
{
    RECT rect32;
    BOOL ret;

    rect32.left   = rc->left;
    rect32.top    = rc->top;
    rect32.right  = rc->right;
    rect32.bottom = rc->bottom;
    ret = DrawFrameControl( HDC_32(hdc), &rect32, uType, uState );
    rc->left   = rect32.left;
    rc->top    = rect32.top;
    rc->right  = rect32.right;
    rc->bottom = rect32.bottom;
    return ret;
}

/**********************************************************************
 *          DrawEdge   (USER.659)
 */
BOOL16 WINAPI DrawEdge16( HDC16 hdc, LPRECT16 rc, UINT16 edge, UINT16 flags )
{
    RECT rect32;
    BOOL ret;

    rect32.left   = rc->left;
    rect32.top    = rc->top;
    rect32.right  = rc->right;
    rect32.bottom = rc->bottom;
    ret = DrawEdge( HDC_32(hdc), &rect32, edge, flags );
    rc->left   = rect32.left;
    rc->top    = rect32.top;
    rc->right  = rect32.right;
    rc->bottom = rect32.bottom;
    return ret;
}

/**********************************************************************
 *		CheckMenuRadioItem (USER.666)
 */
BOOL16 WINAPI CheckMenuRadioItem16(HMENU16 hMenu, UINT16 first, UINT16 last,
                                   UINT16 check, BOOL16 bypos)
{
     return CheckMenuRadioItem( HMENU_32(hMenu), first, last, check, bypos );
}
