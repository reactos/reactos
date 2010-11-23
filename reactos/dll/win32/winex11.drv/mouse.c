/*
 * X11 mouse driver
 *
 * Copyright 1998 Ulrich Weigand
 * Copyright 2007 Henri Verbeet
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

#include "config.h"
#include "wine/port.h"

#include <X11/Xlib.h>
#include <X11/cursorfont.h>
#include <stdarg.h>

#ifdef SONAME_LIBXCURSOR
# include <X11/Xcursor/Xcursor.h>
static void *xcursor_handle;
# define MAKE_FUNCPTR(f) static typeof(f) * p##f
MAKE_FUNCPTR(XcursorImageCreate);
MAKE_FUNCPTR(XcursorImageDestroy);
MAKE_FUNCPTR(XcursorImageLoadCursor);
MAKE_FUNCPTR(XcursorImagesCreate);
MAKE_FUNCPTR(XcursorImagesDestroy);
MAKE_FUNCPTR(XcursorImagesLoadCursor);
MAKE_FUNCPTR(XcursorLibraryLoadCursor);
# undef MAKE_FUNCPTR
#endif /* SONAME_LIBXCURSOR */

#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#define OEMRESOURCE
#include "windef.h"
#include "winbase.h"
#include "winreg.h"

#include "x11drv.h"
#include "winternl.h"
#include "wine/server.h"
#include "wine/library.h"
#include "wine/unicode.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(cursor);

/**********************************************************************/

#ifndef Button6Mask
#define Button6Mask (1<<13)
#endif
#ifndef Button7Mask
#define Button7Mask (1<<14)
#endif

#define NB_BUTTONS   9     /* Windows can handle 5 buttons and the wheel too */

static const UINT button_down_flags[NB_BUTTONS] =
{
    MOUSEEVENTF_LEFTDOWN,
    MOUSEEVENTF_MIDDLEDOWN,
    MOUSEEVENTF_RIGHTDOWN,
    MOUSEEVENTF_WHEEL,
    MOUSEEVENTF_WHEEL,
    MOUSEEVENTF_XDOWN,  /* FIXME: horizontal wheel */
    MOUSEEVENTF_XDOWN,
    MOUSEEVENTF_XDOWN,
    MOUSEEVENTF_XDOWN
};

static const UINT button_up_flags[NB_BUTTONS] =
{
    MOUSEEVENTF_LEFTUP,
    MOUSEEVENTF_MIDDLEUP,
    MOUSEEVENTF_RIGHTUP,
    0,
    0,
    MOUSEEVENTF_XUP,
    MOUSEEVENTF_XUP,
    MOUSEEVENTF_XUP,
    MOUSEEVENTF_XUP
};

POINT cursor_pos;
static HWND cursor_window;
static DWORD last_time_modified;
static RECT cursor_clip; /* Cursor clipping rect */
static XContext cursor_context;
static Cursor create_cursor( HANDLE handle );

BOOL CDECL X11DRV_SetCursorPos( INT x, INT y );


/***********************************************************************
 *		X11DRV_Xcursor_Init
 *
 * Load the Xcursor library for use.
 */
void X11DRV_Xcursor_Init(void)
{
#ifdef SONAME_LIBXCURSOR
    xcursor_handle = wine_dlopen(SONAME_LIBXCURSOR, RTLD_NOW, NULL, 0);
    if (!xcursor_handle)  /* wine_dlopen failed. */
    {
        WARN("Xcursor failed to load.  Using fallback code.\n");
        return;
    }
#define LOAD_FUNCPTR(f) \
        p##f = wine_dlsym(xcursor_handle, #f, NULL, 0)

    LOAD_FUNCPTR(XcursorImageCreate);
    LOAD_FUNCPTR(XcursorImageDestroy);
    LOAD_FUNCPTR(XcursorImageLoadCursor);
    LOAD_FUNCPTR(XcursorImagesCreate);
    LOAD_FUNCPTR(XcursorImagesDestroy);
    LOAD_FUNCPTR(XcursorImagesLoadCursor);
    LOAD_FUNCPTR(XcursorLibraryLoadCursor);
#undef LOAD_FUNCPTR
#endif /* SONAME_LIBXCURSOR */
}


/***********************************************************************
 *		clip_point_to_rect
 *
 * Clip point to the provided rectangle
 */
static inline void clip_point_to_rect( LPCRECT rect, LPPOINT pt )
{
    if      (pt->x <  rect->left)   pt->x = rect->left;
    else if (pt->x >= rect->right)  pt->x = rect->right - 1;
    if      (pt->y <  rect->top)    pt->y = rect->top;
    else if (pt->y >= rect->bottom) pt->y = rect->bottom - 1;
}

/***********************************************************************
 *		update_button_state
 *
 * Update the button state with what X provides us
 */
static inline void update_button_state( unsigned int state )
{
    key_state_table[VK_LBUTTON] = (state & Button1Mask ? 0x80 : 0);
    key_state_table[VK_MBUTTON] = (state & Button2Mask ? 0x80 : 0);
    key_state_table[VK_RBUTTON] = (state & Button3Mask ? 0x80 : 0);
    /* X-buttons are not reported from XQueryPointer */
}

/***********************************************************************
 *		get_empty_cursor
 */
static Cursor get_empty_cursor(void)
{
    static Cursor cursor;
    static const char data[] = { 0 };

    wine_tsx11_lock();
    if (!cursor)
    {
        XColor bg;
        Pixmap pixmap;

        bg.red = bg.green = bg.blue = 0x0000;
        pixmap = XCreateBitmapFromData( gdi_display, root_window, data, 1, 1 );
        if (pixmap)
        {
            cursor = XCreatePixmapCursor( gdi_display, pixmap, pixmap, &bg, &bg, 0, 0 );
            XFreePixmap( gdi_display, pixmap );
        }
    }
    wine_tsx11_unlock();
    return cursor;
}

/***********************************************************************
 *		set_window_cursor
 */
void set_window_cursor( HWND hwnd, HCURSOR handle )
{
    struct x11drv_win_data *data;
    Cursor cursor, prev;

    if (!(data = X11DRV_get_win_data( hwnd ))) return;

    wine_tsx11_lock();
    if (!handle) cursor = get_empty_cursor();
    else if (!cursor_context || XFindContext( gdi_display, (XID)handle, cursor_context, (char **)&cursor ))
    {
        /* try to create it */
        wine_tsx11_unlock();
        if (!(cursor = create_cursor( handle ))) return;

        wine_tsx11_lock();
        if (!cursor_context) cursor_context = XUniqueContext();
        if (!XFindContext( gdi_display, (XID)handle, cursor_context, (char **)&prev ))
        {
            /* someone else was here first */
            XFreeCursor( gdi_display, cursor );
            cursor = prev;
        }
        else
        {
            XSaveContext( gdi_display, (XID)handle, cursor_context, (char *)cursor );
            TRACE( "cursor %p created %lx\n", handle, cursor );
        }
    }

    XDefineCursor( gdi_display, data->whole_window, cursor );
    /* make the change take effect immediately */
    XFlush( gdi_display );
    data->cursor = handle;
    wine_tsx11_unlock();
}

/***********************************************************************
 *		update_mouse_state
 *
 * Update the various window states on a mouse event.
 */
static HWND update_mouse_state( HWND hwnd, Window window, int x, int y, unsigned int state, POINT *pt )
{
    struct x11drv_win_data *data = X11DRV_get_win_data( hwnd );

    if (!data) return 0;

    if (window == data->whole_window)
    {
        x += data->whole_rect.left - data->client_rect.left;
        y += data->whole_rect.top - data->client_rect.top;
    }
    pt->x = x;
    pt->y = y;
    if (GetWindowLongW( data->hwnd, GWL_EXSTYLE ) & WS_EX_LAYOUTRTL)
        pt->x = data->client_rect.right - data->client_rect.left - 1 - pt->x;
    MapWindowPoints( hwnd, 0, pt, 1 );

    cursor_window = hwnd;
    if (hwnd != GetDesktopWindow()) hwnd = GetAncestor( hwnd, GA_ROOT );

    /* update the wine server Z-order */

    if (window != x11drv_thread_data()->grab_window &&
        /* ignore event if a button is pressed, since the mouse is then grabbed too */
        !(state & (Button1Mask|Button2Mask|Button3Mask|Button4Mask|Button5Mask|Button6Mask|Button7Mask)))
    {
        RECT rect;
        SetRect( &rect, pt->x, pt->y, pt->x + 1, pt->y + 1 );
        MapWindowPoints( 0, hwnd, (POINT *)&rect, 2 );

        SERVER_START_REQ( update_window_zorder )
        {
            req->window      = wine_server_user_handle( hwnd );
            req->rect.left   = rect.left;
            req->rect.top    = rect.top;
            req->rect.right  = rect.right;
            req->rect.bottom = rect.bottom;
            wine_server_call( req );
        }
        SERVER_END_REQ;
    }
    return hwnd;
}


/***********************************************************************
 *           get_key_state
 */
static WORD get_key_state(void)
{
    WORD ret = 0;

    if (GetSystemMetrics( SM_SWAPBUTTON ))
    {
        if (key_state_table[VK_RBUTTON] & 0x80) ret |= MK_LBUTTON;
        if (key_state_table[VK_LBUTTON] & 0x80) ret |= MK_RBUTTON;
    }
    else
    {
        if (key_state_table[VK_LBUTTON] & 0x80) ret |= MK_LBUTTON;
        if (key_state_table[VK_RBUTTON] & 0x80) ret |= MK_RBUTTON;
    }
    if (key_state_table[VK_MBUTTON] & 0x80)  ret |= MK_MBUTTON;
    if (key_state_table[VK_SHIFT] & 0x80)    ret |= MK_SHIFT;
    if (key_state_table[VK_CONTROL] & 0x80)  ret |= MK_CONTROL;
    if (key_state_table[VK_XBUTTON1] & 0x80) ret |= MK_XBUTTON1;
    if (key_state_table[VK_XBUTTON2] & 0x80) ret |= MK_XBUTTON2;
    return ret;
}


/***********************************************************************
 *           queue_raw_mouse_message
 */
static void queue_raw_mouse_message( UINT message, HWND top_hwnd, HWND cursor_hwnd, DWORD x, DWORD y,
                                     DWORD data, DWORD time, DWORD extra_info, UINT injected_flags )
{
    MSLLHOOKSTRUCT hook;
    HCURSOR cursor;

    hook.pt.x        = x;
    hook.pt.y        = y;
    hook.mouseData   = MAKELONG( 0, data );
    hook.flags       = injected_flags;
    hook.time        = time;
    hook.dwExtraInfo = extra_info;

    last_time_modified = GetTickCount();
    if (HOOK_CallHooks( WH_MOUSE_LL, HC_ACTION, message, (LPARAM)&hook, TRUE ))
        message = 0;  /* ignore it */

    SERVER_START_REQ( send_hardware_message )
    {
        req->id       = (injected_flags & LLMHF_INJECTED) ? 0 : GetCurrentThreadId();
        req->win      = wine_server_user_handle( top_hwnd );
        req->msg      = message;
        req->wparam   = MAKEWPARAM( get_key_state(), data );
        req->lparam   = 0;
        req->x        = x;
        req->y        = y;
        req->time     = time;
        req->info     = extra_info;
        wine_server_call( req );
        cursor = (reply->count >= 0) ? wine_server_ptr_handle(reply->cursor) : 0;
    }
    SERVER_END_REQ;

    if (cursor_hwnd)
    {
        struct x11drv_win_data *data = X11DRV_get_win_data( cursor_hwnd );
        if (data && cursor != data->cursor) set_window_cursor( cursor_hwnd, cursor );
    }
}


/***********************************************************************
 *		X11DRV_send_mouse_input
 */
void X11DRV_send_mouse_input( HWND top_hwnd, HWND cursor_hwnd, DWORD flags, DWORD x, DWORD y,
                              DWORD data, DWORD time, DWORD extra_info, UINT injected_flags )
{
    POINT pt;

    if (!time) time = GetTickCount();

    if (flags & MOUSEEVENTF_MOVE && flags & MOUSEEVENTF_ABSOLUTE)
    {
        if (injected_flags & LLMHF_INJECTED)
        {
            pt.x = (x * screen_width) >> 16;
            pt.y = (y * screen_height) >> 16;
        }
        else
        {
            pt.x = x;
            pt.y = y;
            wine_tsx11_lock();
            if (cursor_pos.x == x && cursor_pos.y == y &&
                (flags & ~(MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE)))
                flags &= ~MOUSEEVENTF_MOVE;
            wine_tsx11_unlock();
        }
    }
    else if (flags & MOUSEEVENTF_MOVE)
    {
        int accel[3], xMult = 1, yMult = 1;

        /* dx and dy can be negative numbers for relative movements */
        SystemParametersInfoW(SPI_GETMOUSE, 0, accel, 0);

        if (abs(x) > accel[0] && accel[2] != 0)
        {
            xMult = 2;
            if ((abs(x) > accel[1]) && (accel[2] == 2)) xMult = 4;
        }
        if (abs(y) > accel[0] && accel[2] != 0)
        {
            yMult = 2;
            if ((abs(y) > accel[1]) && (accel[2] == 2)) yMult = 4;
        }

        wine_tsx11_lock();
        pt.x = cursor_pos.x + (long)x * xMult;
        pt.y = cursor_pos.y + (long)y * yMult;
        wine_tsx11_unlock();
    }
    else
    {
        wine_tsx11_lock();
        pt = cursor_pos;
        wine_tsx11_unlock();
    }

    if (flags & MOUSEEVENTF_MOVE)
    {
        queue_raw_mouse_message( WM_MOUSEMOVE, top_hwnd, cursor_hwnd, pt.x, pt.y, data, time,
                                 extra_info, injected_flags );
        if ((injected_flags & LLMHF_INJECTED) &&
            ((flags & MOUSEEVENTF_ABSOLUTE) || x || y))  /* we have to actually move the cursor */
        {
            X11DRV_SetCursorPos( pt.x, pt.y );
        }
        else
        {
            wine_tsx11_lock();
            clip_point_to_rect( &cursor_clip, &pt);
            cursor_pos = pt;
            wine_tsx11_unlock();
        }
    }
    if (flags & MOUSEEVENTF_LEFTDOWN)
    {
        key_state_table[VK_LBUTTON] |= 0xc0;
        queue_raw_mouse_message( GetSystemMetrics(SM_SWAPBUTTON) ? WM_RBUTTONDOWN : WM_LBUTTONDOWN,
                                 top_hwnd, cursor_hwnd, pt.x, pt.y,
                                 data, time, extra_info, injected_flags );
    }
    if (flags & MOUSEEVENTF_LEFTUP)
    {
        key_state_table[VK_LBUTTON] &= ~0x80;
        queue_raw_mouse_message( GetSystemMetrics(SM_SWAPBUTTON) ? WM_RBUTTONUP : WM_LBUTTONUP,
                                 top_hwnd, cursor_hwnd, pt.x, pt.y,
                                 data, time, extra_info, injected_flags );
    }
    if (flags & MOUSEEVENTF_RIGHTDOWN)
    {
        key_state_table[VK_RBUTTON] |= 0xc0;
        queue_raw_mouse_message( GetSystemMetrics(SM_SWAPBUTTON) ? WM_LBUTTONDOWN : WM_RBUTTONDOWN,
                                 top_hwnd, cursor_hwnd, pt.x, pt.y,
                                 data, time, extra_info, injected_flags );
    }
    if (flags & MOUSEEVENTF_RIGHTUP)
    {
        key_state_table[VK_RBUTTON] &= ~0x80;
        queue_raw_mouse_message( GetSystemMetrics(SM_SWAPBUTTON) ? WM_LBUTTONUP : WM_RBUTTONUP,
                                 top_hwnd, cursor_hwnd, pt.x, pt.y,
                                 data, time, extra_info, injected_flags );
    }
    if (flags & MOUSEEVENTF_MIDDLEDOWN)
    {
        key_state_table[VK_MBUTTON] |= 0xc0;
        queue_raw_mouse_message( WM_MBUTTONDOWN, top_hwnd, cursor_hwnd, pt.x, pt.y,
                                 data, time, extra_info, injected_flags );
    }
    if (flags & MOUSEEVENTF_MIDDLEUP)
    {
        key_state_table[VK_MBUTTON] &= ~0x80;
        queue_raw_mouse_message( WM_MBUTTONUP, top_hwnd, cursor_hwnd, pt.x, pt.y,
                                 data, time, extra_info, injected_flags );
    }
    if (flags & MOUSEEVENTF_WHEEL)
    {
        queue_raw_mouse_message( WM_MOUSEWHEEL, top_hwnd, cursor_hwnd, pt.x, pt.y,
                                 data, time, extra_info, injected_flags );
    }
    if (flags & MOUSEEVENTF_XDOWN)
    {
        key_state_table[VK_XBUTTON1 + data - 1] |= 0xc0;
        queue_raw_mouse_message( WM_XBUTTONDOWN, top_hwnd, cursor_hwnd, pt.x, pt.y,
                                 data, time, extra_info, injected_flags );
    }
    if (flags & MOUSEEVENTF_XUP)
    {
        key_state_table[VK_XBUTTON1 + data - 1] &= ~0x80;
        queue_raw_mouse_message( WM_XBUTTONUP, top_hwnd, cursor_hwnd, pt.x, pt.y,
                                 data, time, extra_info, injected_flags );
    }
}

#ifdef SONAME_LIBXCURSOR

/***********************************************************************
 *              create_xcursor_frame
 *
 * Use Xcursor to create a frame of an X cursor from a Windows one.
 */
static XcursorImage *create_xcursor_frame( HDC hdc, const ICONINFOEXW *iinfo, HANDLE icon,
                                           HBITMAP hbmColor, unsigned char *color_bits, int color_size,
                                           HBITMAP hbmMask, unsigned char *mask_bits, int mask_size,
                                           int width, int height, int istep )
{
    XcursorImage *image, *ret = NULL;
    int x, y, i, has_alpha;
    XcursorPixel *ptr;

    wine_tsx11_lock();
    image = pXcursorImageCreate( width, height );
    wine_tsx11_unlock();
    if (!image)
    {
        ERR("X11 failed to produce a cursor frame!\n");
        goto cleanup;
    }

    image->xhot = iinfo->xHotspot;
    image->yhot = iinfo->yHotspot;
    image->delay = 100; /* TODO: find a way to get the proper delay */

    /* draw the cursor frame to a temporary buffer then copy it into the XcursorImage */
    memset( color_bits, 0x00, color_size );
    SelectObject( hdc, hbmColor );
    if (!DrawIconEx( hdc, 0, 0, icon, width, height, istep, NULL, DI_NORMAL ))
    {
        TRACE("Could not draw frame %d (walk past end of frames).\n", istep);
        goto cleanup;
    }
    memcpy( image->pixels, color_bits, color_size );

    /* check if the cursor frame was drawn with an alpha channel */
    for (i = 0, ptr = image->pixels; i < width * height; i++, ptr++)
        if ((has_alpha = (*ptr & 0xff000000) != 0)) break;

    /* if no alpha channel was drawn then generate it from the mask */
    if (!has_alpha)
    {
        unsigned int width_bytes = (width + 31) / 32 * 4;

        /* draw the cursor mask to a temporary buffer */
        memset( mask_bits, 0xFF, mask_size );
        SelectObject( hdc, hbmMask );
        if (!DrawIconEx( hdc, 0, 0, icon, width, height, istep, NULL, DI_MASK ))
        {
            ERR("Failed to draw frame mask %d.\n", istep);
            goto cleanup;
        }
        /* use the buffer to directly modify the XcursorImage alpha channel */
        for (y = 0, ptr = image->pixels; y < height; y++)
            for (x = 0; x < width; x++, ptr++)
                if (!((mask_bits[y * width_bytes + x / 8] << (x % 8)) & 0x80))
                    *ptr |= 0xff000000;
    }
    ret = image;

cleanup:
    if (ret == NULL) pXcursorImageDestroy( image );
    return ret;
}

/***********************************************************************
 *              create_xcursor_cursor
 *
 * Use Xcursor to create an X cursor from a Windows one.
 */
static Cursor create_xcursor_cursor( HDC hdc, const ICONINFOEXW *iinfo, HANDLE icon, int width, int height )
{
    unsigned char *color_bits, *mask_bits;
    HBITMAP hbmColor = 0, hbmMask = 0;
    XcursorImage **imgs, *image;
    int color_size, mask_size;
    BITMAPINFO *info = NULL;
    XcursorImages *images;
    Cursor cursor = 0;
    int nFrames = 0;

    if (!(imgs = HeapAlloc( GetProcessHeap(), 0, sizeof(XcursorImage*) ))) return 0;

    /* Allocate all of the resources necessary to obtain a cursor frame */
    if (!(info = HeapAlloc( GetProcessHeap(), 0, FIELD_OFFSET( BITMAPINFO, bmiColors[256] )))) goto cleanup;
    info->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    info->bmiHeader.biWidth = width;
    info->bmiHeader.biHeight = -height;
    info->bmiHeader.biPlanes = 1;
    info->bmiHeader.biCompression = BI_RGB;
    info->bmiHeader.biXPelsPerMeter = 0;
    info->bmiHeader.biYPelsPerMeter = 0;
    info->bmiHeader.biClrUsed = 0;
    info->bmiHeader.biClrImportant = 0;
    info->bmiHeader.biBitCount = 32;
    color_size = width * height * 4;
    info->bmiHeader.biSizeImage = color_size;
    hbmColor = CreateDIBSection( hdc, info, DIB_RGB_COLORS, (VOID **) &color_bits, NULL, 0);
    if (!hbmColor)
    {
        ERR("Failed to create DIB section for cursor color data!\n");
        goto cleanup;
    }
    info->bmiHeader.biBitCount = 1;
    mask_size = ((width + 31) / 32 * 4) * height; /* width_bytes * height */
    info->bmiHeader.biSizeImage = mask_size;
    hbmMask = CreateDIBSection( hdc, info, DIB_RGB_COLORS, (VOID **) &mask_bits, NULL, 0);
    if (!hbmMask)
    {
        ERR("Failed to create DIB section for cursor mask data!\n");
        goto cleanup;
    }

    /* Create an XcursorImage for each frame of the cursor */
    while (1)
    {
        XcursorImage **imgstmp;

        image = create_xcursor_frame( hdc, iinfo, icon,
                                      hbmColor, color_bits, color_size,
                                      hbmMask, mask_bits, mask_size,
                                      width, height, nFrames );
        if (!image) break; /* no more drawable frames */

        imgs[nFrames++] = image;
        if (!(imgstmp = HeapReAlloc( GetProcessHeap(), 0, imgs, (nFrames+1)*sizeof(XcursorImage*) ))) goto cleanup;
        imgs = imgstmp;
    }

    /* Build an X cursor out of all of the frames */
    if (!(images = pXcursorImagesCreate( nFrames ))) goto cleanup;
    for (images->nimage = 0; images->nimage < nFrames; images->nimage++)
        images->images[images->nimage] = imgs[images->nimage];
    wine_tsx11_lock();
    cursor = pXcursorImagesLoadCursor( gdi_display, images );
    wine_tsx11_unlock();
    pXcursorImagesDestroy( images ); /* Note: this frees each individual frame (calls XcursorImageDestroy) */
    HeapFree( GetProcessHeap(), 0, imgs );
    imgs = NULL;

cleanup:
    if (imgs)
    {
        /* Failed to produce a cursor, free previously allocated frames */
        for (nFrames--; nFrames >= 0; nFrames--)
            pXcursorImageDestroy( imgs[nFrames] );
        HeapFree( GetProcessHeap(), 0, imgs );
    }
    /* Cleanup all of the resources used to obtain the frame data */
    if (hbmColor) DeleteObject( hbmColor );
    if (hbmMask) DeleteObject( hbmMask );
    HeapFree( GetProcessHeap(), 0, info );
    return cursor;
}


struct system_cursors
{
    WORD id;
    const char *name;
};

static const struct system_cursors user32_cursors[] =
{
    { OCR_NORMAL,      "left_ptr" },
    { OCR_IBEAM,       "xterm" },
    { OCR_WAIT,        "watch" },
    { OCR_CROSS,       "cross" },
    { OCR_UP,          "center_ptr" },
    { OCR_SIZE,        "fleur" },
    { OCR_SIZEALL,     "fleur" },
    { OCR_ICON,        "icon" },
    { OCR_SIZENWSE,    "nwse-resize" },
    { OCR_SIZENESW,    "nesw-resize" },
    { OCR_SIZEWE,      "ew-resize" },
    { OCR_SIZENS,      "ns-resize" },
    { OCR_NO,          "not-allowed" },
    { OCR_HAND,        "hand2" },
    { OCR_APPSTARTING, "left_ptr_watch" },
    { OCR_HELP,        "question_arrow" },
    { 0 }
};

static const struct system_cursors comctl32_cursors[] =
{
    { 102, "move" },
    { 104, "copy" },
    { 105, "left_ptr" },
    { 106, "row-resize" },
    { 107, "row-resize" },
    { 108, "hand2" },
    { 135, "col-resize" },
    { 0 }
};

static const struct system_cursors ole32_cursors[] =
{
    { 1, "no-drop" },
    { 2, "move" },
    { 3, "copy" },
    { 4, "alias" },
    { 0 }
};

static const struct system_cursors riched20_cursors[] =
{
    { 105, "hand2" },
    { 107, "right_ptr" },
    { 109, "copy" },
    { 110, "move" },
    { 111, "no-drop" },
    { 0 }
};

static const struct
{
    const struct system_cursors *cursors;
    WCHAR name[16];
} module_cursors[] =
{
    { user32_cursors, {'u','s','e','r','3','2','.','d','l','l',0} },
    { comctl32_cursors, {'c','o','m','c','t','l','3','2','.','d','l','l',0} },
    { ole32_cursors, {'o','l','e','3','2','.','d','l','l',0} },
    { riched20_cursors, {'r','i','c','h','e','d','2','0','.','d','l','l',0} }
};

/***********************************************************************
 *		create_xcursor_system_cursor
 *
 * Create an X cursor for a system cursor.
 */
static Cursor create_xcursor_system_cursor( const ICONINFOEXW *info )
{
    static const WCHAR idW[] = {'%','h','u',0};
    const struct system_cursors *cursors;
    unsigned int i;
    Cursor cursor = 0;
    HMODULE module;
    HKEY key;
    WCHAR *p, name[MAX_PATH * 2], valueW[64];
    char valueA[64];
    DWORD size, ret;

    if (!pXcursorLibraryLoadCursor) return 0;
    if (!info->szModName[0]) return 0;

    p = strrchrW( info->szModName, '\\' );
    strcpyW( name, p ? p + 1 : info->szModName );
    p = name + strlenW( name );
    *p++ = ',';
    if (info->szResName[0]) strcpyW( p, info->szResName );
    else sprintfW( p, idW, info->wResID );
    valueA[0] = 0;

    /* @@ Wine registry key: HKCU\Software\Wine\X11 Driver\Cursors */
    if (!RegOpenKeyA( HKEY_CURRENT_USER, "Software\\Wine\\X11 Driver\\Cursors", &key ))
    {
        size = sizeof(valueW) / sizeof(WCHAR);
        ret = RegQueryValueExW( key, name, NULL, NULL, (BYTE *)valueW, &size );
        RegCloseKey( key );
        if (!ret)
        {
            if (!valueW[0]) return 0; /* force standard cursor */
            if (!WideCharToMultiByte( CP_UNIXCP, 0, valueW, -1, valueA, sizeof(valueA), NULL, NULL ))
                valueA[0] = 0;
            goto done;
        }
    }

    if (info->szResName[0]) goto done;  /* only integer resources are supported here */
    if (!(module = GetModuleHandleW( info->szModName ))) goto done;

    for (i = 0; i < sizeof(module_cursors)/sizeof(module_cursors[0]); i++)
        if (GetModuleHandleW( module_cursors[i].name ) == module) break;
    if (i == sizeof(module_cursors)/sizeof(module_cursors[0])) goto done;

    cursors = module_cursors[i].cursors;
    for (i = 0; cursors[i].id; i++)
        if (cursors[i].id == info->wResID)
        {
            strcpy( valueA, cursors[i].name );
            break;
        }

done:
    if (valueA[0])
    {
        wine_tsx11_lock();
        cursor = pXcursorLibraryLoadCursor( gdi_display, valueA );
        wine_tsx11_unlock();
        if (!cursor) WARN( "no system cursor found for %s mapped to %s\n",
                           debugstr_w(name), debugstr_a(valueA) );
    }
    else WARN( "no system cursor found for %s\n", debugstr_w(name) );
    return cursor;
}

#endif /* SONAME_LIBXCURSOR */


/***********************************************************************
 *		create_cursor_from_bitmaps
 *
 * Create an X11 cursor from source bitmaps.
 */
static Cursor create_cursor_from_bitmaps( HBITMAP src_xor, HBITMAP src_and, int width, int height,
                                          int xor_y, int and_y, XColor *fg, XColor *bg,
                                          int hotspot_x, int hotspot_y )
{
    HDC src = 0, dst = 0;
    HBITMAP bits = 0, mask = 0, mask_inv = 0;
    Cursor cursor = 0;

    if (!(src = CreateCompatibleDC( 0 ))) goto done;
    if (!(dst = CreateCompatibleDC( 0 ))) goto done;

    if (!(bits = CreateBitmap( width, height, 1, 1, NULL ))) goto done;
    if (!(mask = CreateBitmap( width, height, 1, 1, NULL ))) goto done;
    if (!(mask_inv = CreateBitmap( width, height, 1, 1, NULL ))) goto done;

    /* We have to do some magic here, as cursors are not fully
     * compatible between Windows and X11. Under X11, there are
     * only 3 possible color cursor: black, white and masked. So
     * we map the 4th Windows color (invert the bits on the screen)
     * to black and an additional white bit on an other place
     * (+1,+1). This require some boolean arithmetic:
     *
     *         Windows          |          X11
     * And    Xor      Result   |   Bits     Mask     Result
     *  0      0     black      |    0        1     background
     *  0      1     white      |    1        1     foreground
     *  1      0     no change  |    X        0     no change
     *  1      1     inverted   |    0        1     background
     *
     * which gives:
     *  Bits = not 'And' and 'Xor' or 'And2' and 'Xor2'
     *  Mask = not 'And' or 'Xor' or 'And2' and 'Xor2'
     */
    SelectObject( src, src_and );
    SelectObject( dst, bits );
    BitBlt( dst, 0, 0, width, height, src, 0, and_y, SRCCOPY );
    SelectObject( dst, mask );
    BitBlt( dst, 0, 0, width, height, src, 0, and_y, SRCCOPY );
    SelectObject( dst, mask_inv );
    BitBlt( dst, 0, 0, width, height, src, 0, and_y, SRCCOPY );
    SelectObject( src, src_xor );
    BitBlt( dst, 0, 0, width, height, src, 0, xor_y, SRCAND /* src & dst */ );
    SelectObject( dst, bits );
    BitBlt( dst, 0, 0, width, height, src, 0, xor_y, SRCERASE /* src & ~dst */ );
    SelectObject( dst, mask );
    BitBlt( dst, 0, 0, width, height, src, 0, xor_y, 0xdd0228 /* src | ~dst */ );
    /* additional white */
    SelectObject( src, mask_inv );
    BitBlt( dst, 1, 1, width, height, src, 0, 0, SRCPAINT /* src | dst */);
    SelectObject( dst, bits );
    BitBlt( dst, 1, 1, width, height, src, 0, 0, SRCPAINT /* src | dst */ );

    wine_tsx11_lock();
    cursor = XCreatePixmapCursor( gdi_display, X11DRV_get_pixmap(bits), X11DRV_get_pixmap(mask),
                                  fg, bg, hotspot_x, hotspot_y );
    wine_tsx11_unlock();

done:
    DeleteDC( src );
    DeleteDC( dst );
    DeleteObject( bits );
    DeleteObject( mask );
    DeleteObject( mask_inv );
    return cursor;
}

/***********************************************************************
 *		create_xlib_cursor
 *
 * Create an X cursor from a Windows one.
 */
static Cursor create_xlib_cursor( HDC hdc, const ICONINFOEXW *icon, int width, int height )
{
    XColor fg, bg;
    Cursor cursor = None;
    HBITMAP xor_bitmap = 0;
    BITMAPINFO *info;
    unsigned int *color_bits = NULL, *ptr;
    unsigned char *mask_bits = NULL, *xor_bits = NULL;
    int i, x, y, has_alpha = 0;
    int rfg, gfg, bfg, rbg, gbg, bbg, fgBits, bgBits;
    unsigned int width_bytes = (width + 31) / 32 * 4;

    if (!(info = HeapAlloc( GetProcessHeap(), 0, FIELD_OFFSET( BITMAPINFO, bmiColors[256] ))))
        return FALSE;
    info->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    info->bmiHeader.biWidth = width;
    info->bmiHeader.biHeight = -height;
    info->bmiHeader.biPlanes = 1;
    info->bmiHeader.biBitCount = 1;
    info->bmiHeader.biCompression = BI_RGB;
    info->bmiHeader.biSizeImage = width_bytes * height;
    info->bmiHeader.biXPelsPerMeter = 0;
    info->bmiHeader.biYPelsPerMeter = 0;
    info->bmiHeader.biClrUsed = 0;
    info->bmiHeader.biClrImportant = 0;

    if (!(mask_bits = HeapAlloc( GetProcessHeap(), 0, info->bmiHeader.biSizeImage ))) goto done;
    if (!GetDIBits( hdc, icon->hbmMask, 0, height, mask_bits, info, DIB_RGB_COLORS )) goto done;

    info->bmiHeader.biBitCount = 32;
    info->bmiHeader.biSizeImage = width * height * 4;
    if (!(color_bits = HeapAlloc( GetProcessHeap(), 0, info->bmiHeader.biSizeImage ))) goto done;
    if (!(xor_bits = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, width_bytes * height ))) goto done;
    GetDIBits( hdc, icon->hbmColor, 0, height, color_bits, info, DIB_RGB_COLORS );

    /* compute fg/bg color and xor bitmap based on average of the color values */

    if (!(xor_bitmap = CreateBitmap( width, height, 1, 1, NULL ))) goto done;
    rfg = gfg = bfg = rbg = gbg = bbg = fgBits = 0;
    for (y = 0, ptr = color_bits; y < height; y++)
    {
        for (x = 0; x < width; x++, ptr++)
        {
            int red   = (*ptr >> 16) & 0xff;
            int green = (*ptr >> 8) & 0xff;
            int blue  = (*ptr >> 0) & 0xff;
            if (red + green + blue > 0x40)
            {
                rfg += red;
                gfg += green;
                bfg += blue;
                fgBits++;
                xor_bits[y * width_bytes + x / 8] |= 0x80 >> (x % 8);
            }
            else
            {
                rbg += red;
                gbg += green;
                bbg += blue;
            }
        }
    }
    if (fgBits)
    {
        fg.red   = rfg * 257 / fgBits;
        fg.green = gfg * 257 / fgBits;
        fg.blue  = bfg * 257 / fgBits;
    }
    else fg.red = fg.green = fg.blue = 0;
    bgBits = width * height - fgBits;
    if (bgBits)
    {
        bg.red   = rbg * 257 / bgBits;
        bg.green = gbg * 257 / bgBits;
        bg.blue  = bbg * 257 / bgBits;
    }
    else bg.red = bg.green = bg.blue = 0;

    info->bmiHeader.biBitCount = 1;
    info->bmiHeader.biSizeImage = width_bytes * height;
    SetDIBits( hdc, xor_bitmap, 0, height, xor_bits, info, DIB_RGB_COLORS );

    /* generate mask from the alpha channel if we have one */

    for (i = 0, ptr = color_bits; i < width * height; i++, ptr++)
        if ((has_alpha = (*ptr & 0xff000000) != 0)) break;

    if (has_alpha)
    {
        memset( mask_bits, 0, width_bytes * height );
        for (y = 0, ptr = color_bits; y < height; y++)
            for (x = 0; x < width; x++, ptr++)
                if ((*ptr >> 24) > 25) /* more than 10% alpha */
                    mask_bits[y * width_bytes + x / 8] |= 0x80 >> (x % 8);

        info->bmiHeader.biBitCount = 1;
        info->bmiHeader.biSizeImage = width_bytes * height;
        SetDIBits( hdc, icon->hbmMask, 0, height, mask_bits, info, DIB_RGB_COLORS );

        wine_tsx11_lock();
        cursor = XCreatePixmapCursor( gdi_display,
                                      X11DRV_get_pixmap(xor_bitmap),
                                      X11DRV_get_pixmap(icon->hbmMask),
                                      &fg, &bg, icon->xHotspot, icon->yHotspot );
        wine_tsx11_unlock();
    }
    else
    {
        cursor = create_cursor_from_bitmaps( xor_bitmap, icon->hbmMask, width, height, 0, 0,
                                             &fg, &bg, icon->xHotspot, icon->yHotspot );
    }

done:
    DeleteObject( xor_bitmap );
    HeapFree( GetProcessHeap(), 0, info );
    HeapFree( GetProcessHeap(), 0, color_bits );
    HeapFree( GetProcessHeap(), 0, xor_bits );
    HeapFree( GetProcessHeap(), 0, mask_bits );
    return cursor;
}

/***********************************************************************
 *		create_cursor
 *
 * Create an X cursor from a Windows one.
 */
static Cursor create_cursor( HANDLE handle )
{
    Cursor cursor = 0;
    ICONINFOEXW info;
    BITMAP bm;

    if (!handle) return get_empty_cursor();

    info.cbSize = sizeof(info);
    if (!GetIconInfoExW( handle, &info )) return 0;

#ifdef SONAME_LIBXCURSOR
    if (use_system_cursors && (cursor = create_xcursor_system_cursor( &info )))
    {
        DeleteObject( info.hbmColor );
        DeleteObject( info.hbmMask );
        return cursor;
    }
#endif

    GetObjectW( info.hbmMask, sizeof(bm), &bm );
    if (!info.hbmColor) bm.bmHeight /= 2;

    /* make sure hotspot is valid */
    if (info.xHotspot >= bm.bmWidth || info.yHotspot >= bm.bmHeight)
    {
        info.xHotspot = bm.bmWidth / 2;
        info.yHotspot = bm.bmHeight / 2;
    }

    if (info.hbmColor)
    {
        HDC hdc = CreateCompatibleDC( 0 );
        if (hdc)
        {
#ifdef SONAME_LIBXCURSOR
            if (pXcursorImagesLoadCursor)
                cursor = create_xcursor_cursor( hdc, &info, handle, bm.bmWidth, bm.bmHeight );
#endif
            if (!cursor) cursor = create_xlib_cursor( hdc, &info, bm.bmWidth, bm.bmHeight );
        }
        DeleteObject( info.hbmColor );
        DeleteDC( hdc );
    }
    else
    {
        XColor fg, bg;
        fg.red = fg.green = fg.blue = 0xffff;
        bg.red = bg.green = bg.blue = 0;
        cursor = create_cursor_from_bitmaps( info.hbmMask, info.hbmMask, bm.bmWidth, bm.bmHeight,
                                             bm.bmHeight, 0, &fg, &bg, info.xHotspot, info.yHotspot );
    }

    DeleteObject( info.hbmMask );
    return cursor;
}

/***********************************************************************
 *		DestroyCursorIcon (X11DRV.@)
 */
void CDECL X11DRV_DestroyCursorIcon( HCURSOR handle )
{
    Cursor cursor;

    wine_tsx11_lock();
    if (cursor_context && !XFindContext( gdi_display, (XID)handle, cursor_context, (char **)&cursor ))
    {
        TRACE( "%p xid %lx\n", handle, cursor );
        XFreeCursor( gdi_display, cursor );
        XDeleteContext( gdi_display, (XID)handle, cursor_context );
    }
    wine_tsx11_unlock();
}

/***********************************************************************
 *		SetCursor (X11DRV.@)
 */
void CDECL X11DRV_SetCursor( HCURSOR handle )
{
    if (cursor_window) SendNotifyMessageW( cursor_window, WM_X11DRV_SET_CURSOR, 0, (LPARAM)handle );
}

/***********************************************************************
 *		SetCursorPos (X11DRV.@)
 */
BOOL CDECL X11DRV_SetCursorPos( INT x, INT y )
{
    Display *display = thread_init_display();
    POINT pt;

    TRACE( "warping to (%d,%d)\n", x, y );

    wine_tsx11_lock();
    if (cursor_pos.x == x && cursor_pos.y == y)
    {
        wine_tsx11_unlock();
        /* We still need to generate WM_MOUSEMOVE */
        queue_raw_mouse_message( WM_MOUSEMOVE, 0, 0, x, y, 0, GetCurrentTime(), 0, 0 );
        return TRUE;
    }

    pt.x = x; pt.y = y;
    clip_point_to_rect( &cursor_clip, &pt);
    XWarpPointer( display, root_window, root_window, 0, 0, 0, 0,
                  pt.x - virtual_screen_rect.left, pt.y - virtual_screen_rect.top );
    XFlush( display ); /* avoids bad mouse lag in games that do their own mouse warping */
    cursor_pos = pt;
    wine_tsx11_unlock();
    return TRUE;
}

/***********************************************************************
 *		GetCursorPos (X11DRV.@)
 */
BOOL CDECL X11DRV_GetCursorPos(LPPOINT pos)
{
    Display *display = thread_init_display();
    Window root, child;
    int rootX, rootY, winX, winY;
    unsigned int xstate;

    wine_tsx11_lock();
    if ((GetTickCount() - last_time_modified > 100) &&
        XQueryPointer( display, root_window, &root, &child,
                       &rootX, &rootY, &winX, &winY, &xstate ))
    {
        update_button_state( xstate );
        winX += virtual_screen_rect.left;
        winY += virtual_screen_rect.top;
        TRACE("pointer at (%d,%d)\n", winX, winY );
        cursor_pos.x = winX;
        cursor_pos.y = winY;
    }
    *pos = cursor_pos;
    wine_tsx11_unlock();
    return TRUE;
}


/***********************************************************************
 *		ClipCursor (X11DRV.@)
 *
 * Set the cursor clipping rectangle.
 */
BOOL CDECL X11DRV_ClipCursor( LPCRECT clip )
{
    if (!IntersectRect( &cursor_clip, &virtual_screen_rect, clip ))
        cursor_clip = virtual_screen_rect;

    return TRUE;
}

/***********************************************************************
 *           X11DRV_ButtonPress
 */
void X11DRV_ButtonPress( HWND hwnd, XEvent *xev )
{
    XButtonEvent *event = &xev->xbutton;
    int buttonNum = event->button - 1;
    WORD wData = 0;
    POINT pt;
    HWND top_hwnd;

    if (buttonNum >= NB_BUTTONS) return;

    switch (buttonNum)
    {
    case 3:
        wData = WHEEL_DELTA;
        break;
    case 4:
        wData = -WHEEL_DELTA;
        break;
    case 5:
        wData = XBUTTON1;
        break;
    case 6:
        wData = XBUTTON2;
        break;
    case 7:
        wData = XBUTTON1;
        break;
    case 8:
        wData = XBUTTON2;
        break;
    }

    update_user_time( event->time );
    top_hwnd = update_mouse_state( hwnd, event->window, event->x, event->y, event->state, &pt );
    if (!top_hwnd) return;

    X11DRV_send_mouse_input( top_hwnd, hwnd,
                             button_down_flags[buttonNum] | MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE,
                             pt.x, pt.y, wData, EVENT_x11_time_to_win32_time(event->time), 0, 0 );
}


/***********************************************************************
 *           X11DRV_ButtonRelease
 */
void X11DRV_ButtonRelease( HWND hwnd, XEvent *xev )
{
    XButtonEvent *event = &xev->xbutton;
    int buttonNum = event->button - 1;
    WORD wData = 0;
    POINT pt;
    HWND top_hwnd;

    if (buttonNum >= NB_BUTTONS || !button_up_flags[buttonNum]) return;

    switch (buttonNum)
    {
    case 5:
        wData = XBUTTON1;
        break;
    case 6:
        wData = XBUTTON2;
        break;
    case 7:
        wData = XBUTTON1;
        break;
    case 8:
        wData = XBUTTON2;
        break;
    }

    top_hwnd = update_mouse_state( hwnd, event->window, event->x, event->y, event->state, &pt );
    if (!top_hwnd) return;

    X11DRV_send_mouse_input( top_hwnd, hwnd,
                             button_up_flags[buttonNum] | MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE,
                             pt.x, pt.y, wData, EVENT_x11_time_to_win32_time(event->time), 0, 0 );
}


/***********************************************************************
 *           X11DRV_MotionNotify
 */
void X11DRV_MotionNotify( HWND hwnd, XEvent *xev )
{
    XMotionEvent *event = &xev->xmotion;
    POINT pt;
    HWND top_hwnd;

    TRACE("hwnd %p, event->is_hint %d\n", hwnd, event->is_hint);

    top_hwnd = update_mouse_state( hwnd, event->window, event->x, event->y, event->state, &pt );
    if (!top_hwnd) return;

    X11DRV_send_mouse_input( top_hwnd, hwnd, MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE,
                             pt.x, pt.y, 0, EVENT_x11_time_to_win32_time(event->time), 0, 0 );
}


/***********************************************************************
 *           X11DRV_EnterNotify
 */
void X11DRV_EnterNotify( HWND hwnd, XEvent *xev )
{
    XCrossingEvent *event = &xev->xcrossing;
    POINT pt;
    HWND top_hwnd;

    TRACE("hwnd %p, event->detail %d\n", hwnd, event->detail);

    if (event->detail == NotifyVirtual || event->detail == NotifyNonlinearVirtual) return;
    if (event->window == x11drv_thread_data()->grab_window) return;

    /* simulate a mouse motion event */
    top_hwnd = update_mouse_state( hwnd, event->window, event->x, event->y, event->state, &pt );
    if (!top_hwnd) return;

    X11DRV_send_mouse_input( top_hwnd, hwnd, MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE,
                             pt.x, pt.y, 0, EVENT_x11_time_to_win32_time(event->time), 0, 0 );
}
