/*
 * PROJECT:         ReactOS
 * LICENSE:         LGPL
 * FILE:            dll/win32/winent.drv/userdrv.c
 * PURPOSE:         User driver stub for ReactOS/Windows
 * PROGRAMMERS:     Aleksey Bragin (aleksey@reactos.org)
 */

/* INCLUDES ***************************************************************/

#include <stdarg.h>
#include <stdio.h>
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "wingdi.h"
#define NTOS_USER_MODE
#include <ndk/ntndk.h>
#include <winddi.h>
#include <win32k/ntgdityp.h>
#include "ntrosgdi.h"
#include "wine/rosuser.h"
#include "wine/list.h"
#include "winent.h"
#include "wine/server.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(rosuserdrv);

/* FUNCTIONS **************************************************************/

/***********************************************************************
 *		move_window_bits
 *
 * Move the window bits when a window is moved.
 */
void move_window_bits( struct ntdrv_win_data *data, const RECT *old_rect, const RECT *new_rect,
                              const RECT *old_client_rect )
{
    RECT src_rect = *old_rect;
    RECT dst_rect = *new_rect;
    HDC hdc_src, hdc_dst;
    HRGN rgn = 0;
    HWND parent = 0;

    if (!data->whole_window)
    {
        OffsetRect( &dst_rect, -data->window_rect.left, -data->window_rect.top );
        parent = GetAncestor( data->hwnd, GA_PARENT );
        hdc_src = GetDCEx( parent, 0, DCX_CACHE );
        hdc_dst = GetDCEx( data->hwnd, 0, DCX_CACHE | DCX_WINDOW );
    }
    else
    {
        OffsetRect( &dst_rect, -data->client_rect.left, -data->client_rect.top );
        /* make src rect relative to the old position of the window */
        OffsetRect( &src_rect, -old_client_rect->left, -old_client_rect->top );
        if (dst_rect.left == src_rect.left && dst_rect.top == src_rect.top) return;
        hdc_src = hdc_dst = GetDCEx( data->hwnd, 0, DCX_CACHE );
    }

    TRACE( "copying bits for win %p (parent %p)/ %s -> %s\n",
           data->hwnd, parent,
           wine_dbgstr_rect(&src_rect), wine_dbgstr_rect(&dst_rect) );
    BitBlt( hdc_dst, dst_rect.left, dst_rect.top,
            dst_rect.right - dst_rect.left, dst_rect.bottom - dst_rect.top,
            hdc_src, src_rect.left, src_rect.top, SRCCOPY );

    ReleaseDC( data->hwnd, hdc_dst );
    if (hdc_src != hdc_dst) ReleaseDC( parent, hdc_src );

    if (rgn)
    {
        if (!data->whole_window)
        {
            /* map region to client rect since we are using DCX_WINDOW */
            OffsetRgn( rgn, data->window_rect.left - data->client_rect.left,
                       data->window_rect.top - data->client_rect.top );
            RedrawWindow( data->hwnd, NULL, rgn,
                          RDW_INVALIDATE | RDW_FRAME | RDW_ERASE | RDW_ALLCHILDREN );
        }
        else RedrawWindow( data->hwnd, NULL, rgn, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN );
        DeleteObject( rgn );
    }
}

HKL CDECL RosDrv_ActivateKeyboardLayout( HKL layout, UINT flags )
{
    return RosUserActivateKeyboardLayout(layout, flags);
}

void CDECL RosDrv_Beep(void)
{
    Beep(500, 100);
}

SHORT CDECL RosDrv_GetAsyncKeyState( INT key )
{
    return RosUserGetAsyncKeyState( key );
}

INT CDECL RosDrv_GetKeyNameText( LONG lparam, LPWSTR buffer, INT size )
{
    return RosUserGetKeyNameText(lparam, buffer, size);
}

HKL CDECL RosDrv_GetKeyboardLayout( DWORD layout )
{
    return RosUserGetKeyboardLayout(layout);
}

BOOL CDECL RosDrv_GetKeyboardLayoutName( LPWSTR name )
{
    return RosUserGetKeyboardLayoutName(name);
}

HKL CDECL RosDrv_LoadKeyboardLayout( LPCWSTR name, UINT flags )
{
    return RosUserLoadKeyboardLayoutEx( NULL, 0, NULL, NULL, NULL,
               wcstoul(name, NULL, 16), flags);
}

UINT CDECL RosDrv_MapVirtualKeyEx( UINT code, UINT type, HKL layout )
{
    return RosUserMapVirtualKeyEx(code, type, 0, layout);
}

UINT CDECL RosDrv_SendInput( UINT count, LPINPUT inputs, int size )
{
    UINT i;

    for (i = 0; i < count; i++, inputs++)
    {
        switch(inputs->type)
        {
        case INPUT_MOUSE:
            NTDRV_SendMouseInput( 0, inputs->mi.dwFlags, inputs->mi.dx, inputs->mi.dy,
                                     inputs->mi.mouseData, inputs->mi.time,
                                     inputs->mi.dwExtraInfo, LLMHF_INJECTED );
            break;
        case INPUT_KEYBOARD:
            NTDRV_SendKeyboardInput( inputs->ki.wVk, inputs->ki.wScan, inputs->ki.dwFlags,
                                     inputs->ki.time, inputs->ki.dwExtraInfo, LLKHF_INJECTED );
            break;
        case INPUT_HARDWARE:
            FIXME( "INPUT_HARDWARE not supported\n" );
            break;
        }
    }
    return count;
}

INT CDECL RosDrv_ToUnicodeEx( UINT virt, UINT scan, const BYTE *state, LPWSTR str,
                                      int size, UINT flags, HKL layout )
{
    return RosUserToUnicodeEx(virt, scan, (BYTE*)state, str, size, flags, layout);
}

BOOL CDECL RosDrv_UnloadKeyboardLayout( HKL layout )
{
    return RosUserUnloadKeyboardLayout(layout);
}

SHORT CDECL RosDrv_VkKeyScanEx( WCHAR ch, HKL layout )
{
    return RosUserVkKeyScanEx(ch, layout, TRUE);
}

BOOL CDECL RosDrv_GetCursorPos( LPPOINT pt )
{
    return RosUserGetCursorPos( pt );
}

BOOL CDECL RosDrv_SetCursorPos( INT x, INT y )
{
    return RosUserSetCursorPos( x, y );
}

BOOL CDECL RosDrv_ClipCursor( LPCRECT clip )
{
    RECT rcScreen, cursor_clip;

    /* Make up virtual screen rectangle*/
    rcScreen.left = 0; rcScreen.top = 0;
    rcScreen.right  = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    rcScreen.bottom = GetSystemMetrics(SM_CYVIRTUALSCREEN);

    /* Intersect with screen rectangle */
    if (!IntersectRect( &cursor_clip, &rcScreen, clip ))
        return RosUserClipCursor(&rcScreen);

    /* Set clipping */
    return RosUserClipCursor( &cursor_clip );
}

BOOL CDECL RosDrv_GetScreenSaveActive(void)
{
    UNIMPLEMENTED;
    return FALSE;
}

void CDECL RosDrv_SetScreenSaveActive( BOOL on )
{
    UNIMPLEMENTED;
}

LONG CDECL RosDrv_ChangeDisplaySettingsEx( LPCWSTR name, LPDEVMODEW mode, HWND hwnd,
                                             DWORD flags, LPVOID lparam )
{
    UNICODE_STRING usDeviceName, *pusDeviceName = NULL;

    if (name)
    {
        RtlInitUnicodeString(&usDeviceName, name);
        pusDeviceName = &usDeviceName;
    }

    return RosUserChangeDisplaySettings(pusDeviceName, mode, hwnd, flags, lparam);
}

BOOL CDECL RosDrv_EnumDisplayMonitors( HDC hdc, LPRECT rect, MONITORENUMPROC proc, LPARAM lp )
{
    int i;
    int nb_monitors;
    HMONITOR *monitors;
    RECT *monitors_rect;

    /* Get the count of the display monitors */
    nb_monitors = RosUserEnumDisplayMonitors(NULL, NULL, 0);
    if (nb_monitors <= 0)
    {
        return FALSE;
    }

    /* Allocate the buffers that will be filled by RosUserEnumDisplayMonitors */
    monitors = HeapAlloc( GetProcessHeap(), 0, nb_monitors * sizeof(HMONITOR));
    monitors_rect = HeapAlloc( GetProcessHeap(), 0, nb_monitors * sizeof(RECT));
    if(!monitors || !monitors_rect)
    {
        return FALSE;
    }

    /* Fill the buffers with the handles and the rects of all display monitors */
    nb_monitors = RosUserEnumDisplayMonitors(monitors, (PRECTL)monitors_rect, nb_monitors);
    if (nb_monitors <= 0)
    {
        return FALSE;
    }

    if (hdc)
    {
        POINT origin;
        RECT limit;

        if (!GetDCOrgEx( hdc, &origin )) return FALSE;
        if (GetClipBox( hdc, &limit ) == ERROR) return FALSE;

        if (rect && !IntersectRect( &limit, &limit, rect )) return TRUE;

        for (i = 0; i < nb_monitors; i++)
        {
            RECT monrect = monitors_rect[i];
            OffsetRect( &monrect, -origin.x, -origin.y );
            if (IntersectRect( &monrect, &monrect, &limit ))
                if (!proc( monitors[i], hdc, &monrect, lp ))
                    return FALSE;
        }
    }
    else
    {
        for (i = 0; i < nb_monitors; i++)
        {
            RECT unused;
            if (!rect || IntersectRect( &unused, &monitors_rect[i], rect ))
                if (!proc( monitors[i], 0, &monitors_rect[i], lp ))
                    return FALSE;
        }
    }
    return TRUE;
}

BOOL CDECL RosDrv_EnumDisplaySettingsEx( LPCWSTR name, DWORD num, LPDEVMODEW mode, DWORD flags )
{
    UNICODE_STRING usDeviceName, *pusDeviceName = NULL;

    if (name)
    {
        RtlInitUnicodeString(&usDeviceName, name);
        pusDeviceName = &usDeviceName;
    }

    return NT_SUCCESS(RosUserEnumDisplaySettings(pusDeviceName, num, mode, flags));
}

BOOL CDECL RosDrv_GetMonitorInfo( HMONITOR handle, LPMONITORINFO info )
{
    return RosUserGetMonitorInfo(handle, info);
}

BOOL CDECL RosDrv_CreateDesktopWindow( HWND hwnd )
{
    unsigned int width, height;

    /* retrieve the real size of the desktop */
    SERVER_START_REQ( get_window_rectangles )
    {
        req->handle = wine_server_user_handle( hwnd );
        wine_server_call( req );
        width  = reply->window.right - reply->window.left;
        height = reply->window.bottom - reply->window.top;
    }
    SERVER_END_REQ;

    TRACE("RosDrv_CreateDesktopWindow(%x), w %d h %d\n", hwnd, width, height);

    SwmAddDesktopWindow(hwnd, width, height);

    if (!width && !height)  /* not initialized yet */
    {
        SERVER_START_REQ( set_window_pos )
        {
            req->handle        = wine_server_user_handle( hwnd );
            req->previous      = 0;
            req->flags         = SWP_NOZORDER;
            req->window.left   = 0;
            req->window.top    = 0;
            req->window.right  = GetSystemMetrics(SM_CXVIRTUALSCREEN);
            req->window.bottom = GetSystemMetrics(SM_CYVIRTUALSCREEN);
            req->client        = req->window;
            wine_server_call( req );
        }
        SERVER_END_REQ;
    }

    return TRUE;
}

BOOL CDECL RosDrv_CreateWindow( HWND hwnd )
{
    WARN("RosDrv_CreateWindow(%x)\n", hwnd);

    if (hwnd == GetDesktopWindow())
    {
        /* create desktop win data */
        if (!NTDRV_create_desktop_win_data( hwnd )) return FALSE;
    }

    return TRUE;
}

void CDECL RosDrv_DestroyWindow( HWND hwnd )
{
    /* Destroy its window data */
    NTDRV_destroy_win_data( hwnd );
}

void CDECL RosDrv_GetDC( HDC hdc, HWND hwnd, HWND top, const RECT *win_rect,
                                 const RECT *top_rect, DWORD flags )
{
    struct ntdrv_escape_set_drawable escape;
    struct ntdrv_win_data *data = NTDRV_get_win_data( hwnd );

    escape.code        = NTDRV_SET_DRAWABLE;
    escape.clip_children = FALSE;
    escape.gl_copy     = FALSE;
    escape.hwnd        = hwnd;
    escape.release     = FALSE;

    if (top == hwnd && data && IsIconic( hwnd ) /*&& data->icon_window*/)
    {
        //escape.drawable = data->icon_window;
    }
    else if (top == hwnd)
    {
        //escape.fbconfig_id = data ? data->fbconfig_id : (XID)GetPropA( hwnd, fbconfig_id_prop );
        /* GL draws to the client area even for window DCs */
        /*escape.gl_drawable = data ? data->client_window : X11DRV_get_client_window( hwnd );
        if (flags & DCX_WINDOW)
            escape.drawable = data ? data->whole_window : X11DRV_get_whole_window( hwnd );
        else
            escape.drawable = escape.gl_drawable;*/
    }
    else
    {
        //escape.drawable    = X11DRV_get_client_window( top );
        //escape.fbconfig_id = data ? data->fbconfig_id : (XID)GetPropA( hwnd, fbconfig_id_prop );
        //escape.gl_drawable = data ? data->gl_drawable : (Drawable)GetPropA( hwnd, gl_drawable_prop );
        //escape.pixmap      = data ? data->pixmap : (Pixmap)GetPropA( hwnd, pixmap_prop );
        //escape.gl_copy     = (escape.gl_drawable != 0);

        if (flags & DCX_CLIPCHILDREN) escape.clip_children = TRUE;
    }

    //FIXME("hdc %x, hwnd %x, top %x\n win_rect %s, top_rect %s\n", hdc, hwnd, top,
    //    wine_dbgstr_rect(win_rect), wine_dbgstr_rect(top_rect));

    escape.dc_rect.left         = win_rect->left - top_rect->left;
    escape.dc_rect.top          = win_rect->top - top_rect->top;
    escape.dc_rect.right        = win_rect->right - top_rect->left;
    escape.dc_rect.bottom       = win_rect->bottom - top_rect->top;
    escape.drawable_rect.left   = top_rect->left;
    escape.drawable_rect.top    = top_rect->top;
    escape.drawable_rect.right  = top_rect->right;
    escape.drawable_rect.bottom = top_rect->bottom;

    ExtEscape( hdc, NTDRV_ESCAPE, sizeof(escape), (LPSTR)&escape, 0, NULL );
}

DWORD CDECL RosDrv_MsgWaitForMultipleObjectsEx( DWORD count, const HANDLE *handles, DWORD timeout,
                                                        DWORD mask, DWORD flags )
{
    //TRACE("WaitForMultipleObjectsEx(%d %p %d %x %x %x\n", count, handles, timeout, mask, flags);

    if (!count && !timeout) return WAIT_TIMEOUT;
    return WaitForMultipleObjectsEx( count, handles, flags & MWMO_WAITALL,
                                     timeout, flags & MWMO_ALERTABLE );
}

void CDECL RosDrv_ReleaseDC( HWND hwnd, HDC hdc )
{
    struct ntdrv_escape_set_drawable escape;

    escape.code        = NTDRV_SET_DRAWABLE;
    escape.gl_copy     = FALSE;
    escape.hwnd        = hwnd;
    escape.release     = TRUE;

    escape.dc_rect.left         = 0;
    escape.dc_rect.top          = 0;
    escape.dc_rect.right        = 0;
    escape.dc_rect.bottom       = 0;
    escape.drawable_rect.left   = 0;
    escape.drawable_rect.top    = 0;
    escape.drawable_rect.right  = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    escape.drawable_rect.bottom = GetSystemMetrics(SM_CYVIRTUALSCREEN);

    ExtEscape( hdc, NTDRV_ESCAPE, sizeof(escape), (LPSTR)&escape, 0, NULL );
}

/*
 * Scroll windows and DCs
 *
 * Copyright 1993  David W. Metcalfe
 * Copyright 1995, 1996 Alex Korobka
 * Copyright 2001 Alexandre Julliard
 */
BOOL CDECL RosDrv_ScrollDC( HDC hdc, INT dx, INT dy, const RECT *lprcScroll,
                            const RECT *lprcClip, HRGN hrgnUpdate, LPRECT lprcUpdate )
{
    RECT rcSrc, rcClip, offset;
    INT dxdev, dydev, res;
    HRGN DstRgn, clipRgn, visrgn;
    //INT code = X11DRV_START_EXPOSURES;

    TRACE("dx,dy %d,%d rcScroll %s rcClip %s hrgnUpdate %p lprcUpdate %p\n",
            dx, dy, wine_dbgstr_rect(lprcScroll), wine_dbgstr_rect(lprcClip),
            hrgnUpdate, lprcUpdate);
    /* enable X-exposure events */
    //if (hrgnUpdate || lprcUpdate)
    //    ExtEscape( hdc, X11DRV_ESCAPE, sizeof(code), (LPSTR)&code, 0, NULL );
    /* get the visible region */
    visrgn=CreateRectRgn( 0, 0, 0, 0);
    GetRandomRgn( hdc, visrgn, SYSRGN);
    if( !(GetVersion() & 0x80000000)) {
        /* Window NT/2k/XP */
        POINT org;
        GetDCOrgEx(hdc, &org);
        OffsetRgn( visrgn, -org.x, -org.y);
    }
    /* intersect with the clipping Region if the DC has one */
    clipRgn = CreateRectRgn( 0, 0, 0, 0);
    if (GetClipRgn( hdc, clipRgn) != 1) {
        DeleteObject(clipRgn);
        clipRgn=NULL;
    } else
        CombineRgn( visrgn, visrgn, clipRgn, RGN_AND);
    /* only those pixels in the scroll rectangle that remain in the clipping
     * rect are scrolled. */
    if( lprcClip) 
        rcClip = *lprcClip;
    else
        GetClipBox( hdc, &rcClip);
    rcSrc = rcClip;
    OffsetRect( &rcClip, -dx, -dy);
    IntersectRect( &rcSrc, &rcSrc, &rcClip);
    /* if an scroll rectangle is specified, only the pixels within that
     * rectangle are scrolled */
    if( lprcScroll)
        IntersectRect( &rcSrc, &rcSrc, lprcScroll);
    /* now convert to device coordinates */
    LPtoDP(hdc, (LPPOINT)&rcSrc, 2);
    TRACE("source rect: %s\n", wine_dbgstr_rect(&rcSrc));
    /* also dx and dy */
    SetRect(&offset, 0, 0, dx, dy);
    LPtoDP(hdc, (LPPOINT)&offset, 2);
    dxdev = offset.right - offset.left;
    dydev = offset.bottom - offset.top;
    /* now intersect with the visible region to get the pixels that will
     * actually scroll */
    DstRgn = CreateRectRgnIndirect( &rcSrc);
    res = CombineRgn( DstRgn, DstRgn, visrgn, RGN_AND);
    /* and translate, giving the destination region */
    OffsetRgn( DstRgn, dxdev, dydev);
    //if( TRACE_ON( scroll)) dump_region( "Destination scroll region: ", DstRgn);
    /* if there are any, do it */
    if( res > NULLREGION) {
        RECT rect ;
        /* clip to the destination region, so we can BitBlt with a simple
         * bounding rectangle */
        if( clipRgn)
            ExtSelectClipRgn( hdc, DstRgn, RGN_AND);
        else
            SelectClipRgn( hdc, DstRgn);
        GetRgnBox( DstRgn, &rect);
        DPtoLP(hdc, (LPPOINT)&rect, 2);
        TRACE("destination rect: %s\n", wine_dbgstr_rect(&rect));

        BitBlt( hdc, rect.left, rect.top,
                    rect.right - rect.left, rect.bottom - rect.top,
                    hdc, rect.left - dx, rect.top - dy, SRCCOPY);
    }
    /* compute the update areas.  This is the combined clip rectangle
     * minus the scrolled region, and intersected with the visible
     * region. */
    if (hrgnUpdate || lprcUpdate)
    {
        HRGN hrgn = hrgnUpdate;
        HRGN ExpRgn = 0;

        /* collect all the exposures */
        //code = X11DRV_END_EXPOSURES;
        //ExtEscape( hdc, X11DRV_ESCAPE, sizeof(code), (LPSTR)&code,
        //        sizeof(ExpRgn), (LPSTR)&ExpRgn );
        /* Intersect clip and scroll rectangles, allowing NULL values */ 
        if( lprcScroll)
            if( lprcClip)
                IntersectRect( &rcClip, lprcClip, lprcScroll);
            else
                rcClip = *lprcScroll;
        else
            if( lprcClip)
                rcClip = *lprcClip;
            else
                GetClipBox( hdc, &rcClip);
        /* Convert the combined clip rectangle to device coordinates */
        LPtoDP(hdc, (LPPOINT)&rcClip, 2);
        if( hrgn )
            SetRectRgn( hrgn, rcClip.left, rcClip.top, rcClip.right,
                    rcClip.bottom);
        else
            hrgn = CreateRectRgnIndirect( &rcClip);
        CombineRgn( hrgn, hrgn, visrgn, RGN_AND);
        CombineRgn( hrgn, hrgn, DstRgn, RGN_DIFF);
        /* add the exposures to this */
        if( ExpRgn) {
            //if( TRACE_ON( scroll)) dump_region( "Expose region: ", ExpRgn);
            CombineRgn( hrgn, hrgn, ExpRgn, RGN_OR);
            DeleteObject( ExpRgn);
        }
        //if( TRACE_ON( scroll)) dump_region( "Update region: ", hrgn);
        if( lprcUpdate) {
            GetRgnBox( hrgn, lprcUpdate );
            /* Put the lprcUpdate in logical coordinates */
            DPtoLP( hdc, (LPPOINT)lprcUpdate, 2 );
            TRACE("returning lprcUpdate %s\n", wine_dbgstr_rect(lprcUpdate));
        }
        if( !hrgnUpdate)
            DeleteObject( hrgn);
    }
    /* restore original clipping region */
    SelectClipRgn( hdc, clipRgn);
    DeleteObject( visrgn);
    DeleteObject( DstRgn);
    if( clipRgn) DeleteObject( clipRgn);
    return TRUE;
}

void CDECL RosDrv_SetCapture( HWND hwnd, UINT flags )
{
    if (!(flags & (GUI_INMOVESIZE | GUI_INMENUMODE))) return;

    if (hwnd)
    {
        /* Capturing */
        FIXME("Capture set for hwnd %x\n", hwnd);
    }
    else
    {
        FIXME("Capture released\n");
    }
}

/*******************************************************************
 *         can_activate_window
 *
 * Check if we can activate the specified window.
 */
/*static inline BOOL can_activate_window( HWND hwnd )
{
    LONG style = GetWindowLongW( hwnd, GWL_STYLE );
    if (!(style & WS_VISIBLE)) return FALSE;
    if ((style & (WS_POPUP|WS_CHILD)) == WS_CHILD) return FALSE;
    if (style & WS_MINIMIZE) return FALSE;
    if (GetWindowLongW( hwnd, GWL_EXSTYLE ) & WS_EX_NOACTIVATE) return FALSE;
    if (hwnd == GetDesktopWindow()) return FALSE;
    return !(style & WS_DISABLED);
}*/

void CDECL RosDrv_SetFocus( HWND hwnd )
{
    struct ntdrv_win_data *data;

    if (!(hwnd = GetAncestor( hwnd, GA_ROOT ))) return;
    if (!(data = NTDRV_get_win_data( hwnd ))) return;
    if (!data->whole_window) return;

    TRACE("SetFocus %x, desk %x\n", hwnd, GetDesktopWindow());

    /* Bring this window to foreground */
    SwmSetForeground(hwnd);
}

void CDECL RosDrv_SetLayeredWindowAttributes( HWND hwnd, COLORREF key, BYTE alpha, DWORD flags )
{
    UNIMPLEMENTED;
}

void CDECL RosDrv_SetParent( HWND hwnd, HWND parent, HWND old_parent )
{
    UNIMPLEMENTED;
}

int CDECL RosDrv_SetWindowRgn( HWND hwnd, HRGN hrgn, BOOL redraw )
{
    UNIMPLEMENTED;
    return 1;
}

void CDECL RosDrv_SetWindowIcon( HWND hwnd, UINT type, HICON icon )
{
    UNIMPLEMENTED;
}

void CDECL RosDrv_SetWindowStyle( HWND hwnd, INT offset, STYLESTRUCT *style )
{
    DWORD changed;
    struct ntdrv_win_data *data;

    if (hwnd == GetDesktopWindow()) return;
    changed = style->styleNew ^ style->styleOld;

    if (offset == GWL_STYLE && (changed & WS_VISIBLE) && (style->styleNew & WS_VISIBLE))
    {
        /* Create private win data if it's missing */
        if (!(data = NTDRV_get_win_data( hwnd )) &&
            !(data = NTDRV_create_win_data( hwnd ))) return;

        /* Do some magic... */
        TRACE("Window %x is being made visible1\n", hwnd);
    }

    if (offset == GWL_STYLE && (changed & WS_DISABLED))
    {
        UNIMPLEMENTED;
    }

    if (offset == GWL_EXSTYLE && (changed & WS_EX_LAYERED))
    {
        /* changing WS_EX_LAYERED resets attributes */
        UNIMPLEMENTED;
    }
}

void CDECL RosDrv_SetWindowText( HWND hwnd, LPCWSTR text )
{
    //UNIMPLEMENTED;
}

UINT CDECL RosDrv_ShowWindow( HWND hwnd, INT cmd, RECT *rect, UINT swp )
{
    DWORD style = GetWindowLongW( hwnd, GWL_STYLE );
    struct ntdrv_win_data *data = NTDRV_get_win_data( hwnd );

    if (!data || !data->whole_window) return swp;
    if (style & WS_MINIMIZE) return swp;
    if (IsRectEmpty( rect )) return swp;

    FIXME( "win %p cmd %d at %s flags %08x\n",
           hwnd, cmd, wine_dbgstr_rect(rect), swp );

    /* ???: only fetch the new rectangle if the ShowWindow was a result of a window manager event */
    // TODO: Need to think about this

    return swp;
}

LRESULT CDECL RosDrv_SysCommand( HWND hwnd, WPARAM wparam, LPARAM lparam )
{
    //UNIMPLEMENTED;
    return -1;
}

LRESULT CDECL RosDrv_WindowMessage( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam )
{
    switch(msg)
    {
    default:
        FIXME( "got window msg %x hwnd %p wp %lx lp %lx\n", msg, hwnd, wparam, lparam );
        return 0;
    }
}

void CDECL RosDrv_WindowPosChanging( HWND hwnd, HWND insert_after, UINT swp_flags,
                                             const RECT *window_rect, const RECT *client_rect,
                                             RECT *visible_rect )
{
    DWORD style = GetWindowLongW( hwnd, GWL_STYLE );
    struct ntdrv_win_data *data = NTDRV_get_win_data(hwnd);

    if (!data)
    {
        /* create the win data if the window is being made visible */
        if (!(style & WS_VISIBLE) && !(swp_flags & SWP_SHOWWINDOW)) return;
        if (!(data = NTDRV_create_win_data( hwnd ))) return;
    }

    SwmPosChanging(hwnd, window_rect);

    //TRACE( "win %x pos is changing. vis rect %s, win rect %s\n",
    //       hwnd, wine_dbgstr_rect(visible_rect), wine_dbgstr_rect(window_rect) );

    *visible_rect = *window_rect;
}

void CDECL RosDrv_WindowPosChanged( HWND hwnd, HWND insert_after, UINT swp_flags,
                                    const RECT *window_rect, const RECT *rectClient,
                                    const RECT *visible_rect, const RECT *valid_rects )
{
    RECT old_whole_rect, old_client_rect;
    struct ntdrv_win_data *data = NTDRV_get_win_data(hwnd);

    if (!data) return;

    TRACE( "win %x pos changed. new vis rect %s, old whole rect %s, swp_flags %x insert_after %x\n",
           hwnd, wine_dbgstr_rect(visible_rect), wine_dbgstr_rect(&data->whole_rect), swp_flags, insert_after );

    old_whole_rect  = data->whole_rect;
    old_client_rect = data->client_rect;
    data->window_rect = *window_rect;
    data->whole_rect  = *visible_rect;
    data->client_rect = *rectClient;

    if (!IsRectEmpty( &valid_rects[0] ))
    {
        int x_offset = old_whole_rect.left - data->whole_rect.left;
        int y_offset = old_whole_rect.top - data->whole_rect.top;

        /* if all that happened is that the whole window moved, copy everything */
        if (!(swp_flags & SWP_FRAMECHANGED) &&
            old_whole_rect.right   - data->whole_rect.right   == x_offset &&
            old_whole_rect.bottom  - data->whole_rect.bottom  == y_offset &&
            old_client_rect.left   - data->client_rect.left   == x_offset &&
            old_client_rect.right  - data->client_rect.right  == x_offset &&
            old_client_rect.top    - data->client_rect.top    == y_offset &&
            old_client_rect.bottom - data->client_rect.bottom == y_offset &&
            !memcmp( &valid_rects[0], &data->client_rect, sizeof(RECT) ))
        {
            /* if we have an SWM window the bits will be moved by the SWM */
            if (!data->whole_window)
                move_window_bits( data, &old_whole_rect, &data->whole_rect, &old_client_rect );
        }
        else
        {
            move_window_bits( data, &valid_rects[1], &valid_rects[0], &old_client_rect );
        }
    }

    if (!data->whole_window) return;

    /* Sync position change */
    if (!(swp_flags & SWP_NOREDRAW)) // HACK: When removing this, explorer's start menu starts to appear partially. Investigate!
    {
        /* SWM: Change windows position */
        SwmPosChanged(hwnd, &data->whole_rect, &old_whole_rect, insert_after, swp_flags);
    }

    /* Pass show/hide information to the window manager */
    if (swp_flags & SWP_SHOWWINDOW)
        SwmShowWindow(hwnd, TRUE, swp_flags);
    else if (swp_flags & SWP_HIDEWINDOW)
        SwmShowWindow(hwnd, FALSE, swp_flags);
}

/* EOF */
