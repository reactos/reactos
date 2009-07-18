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
#include <stdarg.h>

#ifdef SONAME_LIBXCURSOR
# include <X11/Xcursor/Xcursor.h>
static void *xcursor_handle;
# define MAKE_FUNCPTR(f) static typeof(f) * p##f
MAKE_FUNCPTR(XcursorImageCreate);
MAKE_FUNCPTR(XcursorImageDestroy);
MAKE_FUNCPTR(XcursorImageLoadCursor);
# undef MAKE_FUNCPTR
#endif /* SONAME_LIBXCURSOR */

#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#include "windef.h"
#include "winbase.h"
#include "wine/winuser16.h"

#include "x11drv.h"
#include "wine/server.h"
#include "wine/library.h"
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
static DWORD last_time_modified;
static RECT cursor_clip; /* Cursor clipping rect */

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
#undef LOAD_FUNCPTR
#endif /* SONAME_LIBXCURSOR */
}


/***********************************************************************
 *		get_coords
 *
 * get the coordinates of a mouse event
 */
static inline void get_coords( HWND hwnd, Window window, int x, int y, POINT *pt )
{
    struct x11drv_win_data *data = X11DRV_get_win_data( hwnd );

    if (!data) return;

    if (window == data->client_window)
    {
        pt->x = x + data->client_rect.left;
        pt->y = y + data->client_rect.top;
    }
    else
    {
        pt->x = x + data->whole_rect.left;
        pt->y = y + data->whole_rect.top;
    }
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
 *		update_mouse_state
 *
 * Update the various window states on a mouse event.
 */
static void update_mouse_state( HWND hwnd, Window window, int x, int y, unsigned int state, POINT *pt )
{
    struct x11drv_thread_data *data = x11drv_thread_data();

    get_coords( hwnd, window, x, y, pt );

    /* update the cursor */

    if (data->cursor_window != window)
    {
        data->cursor_window = window;
        wine_tsx11_lock();
        if (data->cursor) XDefineCursor( data->display, window, data->cursor );
        wine_tsx11_unlock();
    }

    /* update the wine server Z-order */

    if (window != data->grab_window &&
        /* ignore event if a button is pressed, since the mouse is then grabbed too */
        !(state & (Button1Mask|Button2Mask|Button3Mask|Button4Mask|Button5Mask|Button6Mask|Button7Mask)))
    {
        SERVER_START_REQ( update_window_zorder )
        {
            req->window      = wine_server_user_handle( hwnd );
            req->rect.left   = pt->x;
            req->rect.top    = pt->y;
            req->rect.right  = pt->x + 1;
            req->rect.bottom = pt->y + 1;
            wine_server_call( req );
        }
        SERVER_END_REQ;
    }
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
static void queue_raw_mouse_message( UINT message, HWND hwnd, DWORD x, DWORD y,
                                     DWORD data, DWORD time, DWORD extra_info, UINT injected_flags )
{
    MSLLHOOKSTRUCT hook;

    hook.pt.x        = x;
    hook.pt.y        = y;
    hook.mouseData   = MAKELONG( 0, data );
    hook.flags       = injected_flags;
    hook.time        = time;
    hook.dwExtraInfo = extra_info;

    last_time_modified = GetTickCount();
    if (HOOK_CallHooks( WH_MOUSE_LL, HC_ACTION, message, (LPARAM)&hook, TRUE )) return;

    SERVER_START_REQ( send_hardware_message )
    {
        req->id       = (injected_flags & LLMHF_INJECTED) ? 0 : GetCurrentThreadId();
        req->win      = wine_server_user_handle( hwnd );
        req->msg      = message;
        req->wparam   = MAKEWPARAM( get_key_state(), data );
        req->lparam   = 0;
        req->x        = x;
        req->y        = y;
        req->time     = time;
        req->info     = extra_info;
        wine_server_call( req );
    }
    SERVER_END_REQ;

}


/***********************************************************************
 *		X11DRV_send_mouse_input
 */
void X11DRV_send_mouse_input( HWND hwnd, DWORD flags, DWORD x, DWORD y,
                              DWORD data, DWORD time, DWORD extra_info, UINT injected_flags )
{
    POINT pt;

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
        queue_raw_mouse_message( WM_MOUSEMOVE, hwnd, pt.x, pt.y, data, time,
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
                                 hwnd, pt.x, pt.y, data, time, extra_info, injected_flags );
    }
    if (flags & MOUSEEVENTF_LEFTUP)
    {
        key_state_table[VK_LBUTTON] &= ~0x80;
        queue_raw_mouse_message( GetSystemMetrics(SM_SWAPBUTTON) ? WM_RBUTTONUP : WM_LBUTTONUP,
                                 hwnd, pt.x, pt.y, data, time, extra_info, injected_flags );
    }
    if (flags & MOUSEEVENTF_RIGHTDOWN)
    {
        key_state_table[VK_RBUTTON] |= 0xc0;
        queue_raw_mouse_message( GetSystemMetrics(SM_SWAPBUTTON) ? WM_LBUTTONDOWN : WM_RBUTTONDOWN,
                                 hwnd, pt.x, pt.y, data, time, extra_info, injected_flags );
    }
    if (flags & MOUSEEVENTF_RIGHTUP)
    {
        key_state_table[VK_RBUTTON] &= ~0x80;
        queue_raw_mouse_message( GetSystemMetrics(SM_SWAPBUTTON) ? WM_LBUTTONUP : WM_RBUTTONUP,
                                 hwnd, pt.x, pt.y, data, time, extra_info, injected_flags );
    }
    if (flags & MOUSEEVENTF_MIDDLEDOWN)
    {
        key_state_table[VK_MBUTTON] |= 0xc0;
        queue_raw_mouse_message( WM_MBUTTONDOWN, hwnd, pt.x, pt.y, data, time,
                                 extra_info, injected_flags );
    }
    if (flags & MOUSEEVENTF_MIDDLEUP)
    {
        key_state_table[VK_MBUTTON] &= ~0x80;
        queue_raw_mouse_message( WM_MBUTTONUP, hwnd, pt.x, pt.y, data, time,
                                 extra_info, injected_flags );
    }
    if (flags & MOUSEEVENTF_WHEEL)
    {
        queue_raw_mouse_message( WM_MOUSEWHEEL, hwnd, pt.x, pt.y, data, time,
                                 extra_info, injected_flags );
    }
    if (flags & MOUSEEVENTF_XDOWN)
    {
        key_state_table[VK_XBUTTON1 + data - 1] |= 0xc0;
        queue_raw_mouse_message( WM_XBUTTONDOWN, hwnd, pt.x, pt.y, data, time,
                                 extra_info, injected_flags );
    }
    if (flags & MOUSEEVENTF_XUP)
    {
        key_state_table[VK_XBUTTON1 + data - 1] &= ~0x80;
        queue_raw_mouse_message( WM_XBUTTONUP, hwnd, pt.x, pt.y, data, time,
                                 extra_info, injected_flags );
    }
}


/***********************************************************************
 *              check_alpha_zero
 *
 * Generally 32 bit bitmaps have an alpha channel which is used in favor of the
 * AND mask.  However, if all pixels have alpha = 0x00, the bitmap is treated
 * like one without alpha and the masks are used.  As soon as one pixel has
 * alpha != 0x00, and the mask ignored as described in the docs.
 *
 * This is most likely for applications which create the bitmaps with
 * CreateDIBitmap, which creates a device dependent bitmap, so the format that
 * arrives when loading depends on the screen's bpp.  Apps that were written at
 * 8 / 16 bpp times do not know about the 32 bit alpha, so they would get a
 * completely transparent cursor on 32 bit displays.
 *
 * Non-32 bit bitmaps always use the AND mask.
 */
static BOOL check_alpha_zero(CURSORICONINFO *ptr, unsigned char *xor_bits)
{
    int x, y;
    unsigned char *xor_ptr;

    if (ptr->bBitsPerPixel == 32)
    {
        for (y = 0; y < ptr->nHeight; ++y)
        {
            xor_ptr = xor_bits + (y * ptr->nWidthBytes);
            for (x = 0; x < ptr->nWidth; ++x)
            {
                if (xor_ptr[3] != 0x00)
                {
                    return FALSE;
                }
                xor_ptr+=4;
            }
        }
    }

    return TRUE;
}


#ifdef SONAME_LIBXCURSOR

/***********************************************************************
 *              create_cursor_image
 *
 * Create an XcursorImage from a CURSORICONINFO
 */
static XcursorImage *create_cursor_image( CURSORICONINFO *ptr )
{
    static const unsigned char convert_5to8[] =
    {
        0x00, 0x08, 0x10, 0x19, 0x21, 0x29, 0x31, 0x3a,
        0x42, 0x4a, 0x52, 0x5a, 0x63, 0x6b, 0x73, 0x7b,
        0x84, 0x8c, 0x94, 0x9c, 0xa5, 0xad, 0xb5, 0xbd,
        0xc5, 0xce, 0xd6, 0xde, 0xe6, 0xef, 0xf7, 0xff,
    };
    static const unsigned char convert_6to8[] =
    {
        0x00, 0x04, 0x08, 0x0c, 0x10, 0x14, 0x18, 0x1c,
        0x20, 0x24, 0x28, 0x2d, 0x31, 0x35, 0x39, 0x3d,
        0x41, 0x45, 0x49, 0x4d, 0x51, 0x55, 0x59, 0x5d,
        0x61, 0x65, 0x69, 0x6d, 0x71, 0x75, 0x79, 0x7d,
        0x82, 0x86, 0x8a, 0x8e, 0x92, 0x96, 0x9a, 0x9e,
        0xa2, 0xa6, 0xaa, 0xae, 0xb2, 0xb6, 0xba, 0xbe,
        0xc2, 0xc6, 0xca, 0xce, 0xd2, 0xd7, 0xdb, 0xdf,
        0xe3, 0xe7, 0xeb, 0xef, 0xf3, 0xf7, 0xfb, 0xff,
    };
    int x;
    int y;
    int and_size;
    unsigned char *and_bits, *and_ptr, *xor_bits, *xor_ptr;
    int and_width_bytes, xor_width_bytes;
    XcursorPixel *pixel_ptr;
    XcursorImage *image;
    unsigned char tmp;
    BOOL alpha_zero;

    and_width_bytes = 2 * ((ptr->nWidth+15) / 16);
    xor_width_bytes = ptr->nWidthBytes;

    and_size = ptr->nHeight * and_width_bytes;
    and_ptr = and_bits = (unsigned char *)(ptr + 1);

    xor_ptr = xor_bits = and_ptr + and_size;

    image = pXcursorImageCreate( ptr->nWidth, ptr->nHeight );
    pixel_ptr = image->pixels;

    alpha_zero = check_alpha_zero(ptr, xor_bits);

    /* On windows, to calculate the color for a pixel, first an AND is done
     * with the background and the "and" bitmap, then an XOR with the "xor"
     * bitmap. This means that when the data in the "and" bitmap is 0, the
     * pixel will get the color as specified in the "xor" bitmap.
     * However, if the data in the "and" bitmap is 1, the result will be the
     * background XOR'ed with the value in the "xor" bitmap. In case the "xor"
     * data is completely black (0x000000) the pixel will become transparent,
     * in case it's white (0xffffff) the pixel will become the inverse of the
     * background color.
     *
     * Since we can't support inverting colors, we map the grayscale value of
     * the "xor" data to the alpha channel, and xor the color with either
     * black or white.
     */
    for (y = 0; y < ptr->nHeight; ++y)
    {
        and_ptr = and_bits + (y * and_width_bytes);
        xor_ptr = xor_bits + (y * xor_width_bytes);

        for (x = 0; x < ptr->nWidth; ++x)
        {
            /* Xcursor pixel data is in ARGB format, with A in the high byte */
            switch (ptr->bBitsPerPixel)
            {
                case 32:
                    /* BGRA, 8 bits each */
                    *pixel_ptr = *xor_ptr++;
                    *pixel_ptr |= *xor_ptr++ << 8;
                    *pixel_ptr |= *xor_ptr++ << 16;
                    *pixel_ptr |= *xor_ptr++ << 24;
                    break;

                case 24:
                    /* BGR, 8 bits each */
                    *pixel_ptr = *xor_ptr++;
                    *pixel_ptr |= *xor_ptr++ << 8;
                    *pixel_ptr |= *xor_ptr++ << 16;
                    break;

                case 16:
                    /* BGR, 5 red, 6 green, 5 blue */
                    /* [gggbbbbb][rrrrrggg] -> [xxxxxxxx][rrrrrrrr][gggggggg][bbbbbbbb] */
                    *pixel_ptr = convert_5to8[*xor_ptr & 0x1f];
                    tmp = (*xor_ptr++ & 0xe0) >> 5;
                    tmp |= (*xor_ptr & 0x07) << 3;
                    *pixel_ptr |= convert_6to8[tmp] << 16;
                    *pixel_ptr |= convert_5to8[*xor_ptr & 0xf8] << 24;
                    break;

                case 1:
                    if (*xor_ptr & (1 << (7 - (x & 7)))) *pixel_ptr = 0xffffff;
                    else *pixel_ptr = 0;
                    if ((x & 7) == 7) ++xor_ptr;
                    break;

                default:
                    FIXME("Currently no support for cursors with %d bits per pixel\n", ptr->bBitsPerPixel);
                    return 0;
            }

            if (alpha_zero)
            {
                /* Alpha channel */
                if (~*and_ptr & (1 << (7 - (x & 7)))) *pixel_ptr |= 0xff << 24;
                else if (*pixel_ptr)
                {
                    int alpha = (*pixel_ptr & 0xff) * 0.30f
                            + ((*pixel_ptr & 0xff00) >> 8) * 0.55f
                            + ((*pixel_ptr & 0xff0000) >> 16) * 0.15f;
                    *pixel_ptr ^= ((x + y) % 2) ? 0xffffff : 0x000000;
                    *pixel_ptr |= alpha << 24;
                }
                if ((x & 7) == 7) ++and_ptr;
            }
            ++pixel_ptr;
        }
    }

    return image;
}


/***********************************************************************
 *              create_xcursor_cursor
 *
 * Use Xcursor to create an X cursor from a Windows one.
 */
static Cursor create_xcursor_cursor( Display *display, CURSORICONINFO *ptr )
{
    Cursor cursor;
    XcursorImage *image;

    if (!ptr) /* Create an empty cursor */
    {
        image = pXcursorImageCreate( 1, 1 );
        image->xhot = 0;
        image->yhot = 0;
        *(image->pixels) = 0;
        cursor = pXcursorImageLoadCursor( display, image );
        pXcursorImageDestroy( image );

        return cursor;
    }

    image = create_cursor_image( ptr );
    if (!image) return 0;

    /* Make sure hotspot is valid */
    image->xhot = ptr->ptHotSpot.x;
    image->yhot = ptr->ptHotSpot.y;
    if (image->xhot >= image->width ||
        image->yhot >= image->height)
    {
        image->xhot = image->width / 2;
        image->yhot = image->height / 2;
    }

    image->delay = 0;

    cursor = pXcursorImageLoadCursor( display, image );
    pXcursorImageDestroy( image );

    return cursor;
}

#endif /* SONAME_LIBXCURSOR */


/***********************************************************************
 *		create_cursor
 *
 * Create an X cursor from a Windows one.
 */
static Cursor create_cursor( Display *display, CURSORICONINFO *ptr )
{
    Pixmap pixmapBits, pixmapMask, pixmapMaskInv = 0, pixmapAll;
    XColor fg, bg;
    Cursor cursor = None;
    POINT hotspot;
    char *bitMask32 = NULL;
    BOOL alpha_zero = TRUE;

#ifdef SONAME_LIBXCURSOR
    if (pXcursorImageLoadCursor) return create_xcursor_cursor( display, ptr );
#endif

    if (!ptr)  /* Create an empty cursor */
    {
        static const char data[] = { 0 };

        bg.red = bg.green = bg.blue = 0x0000;
        pixmapBits = XCreateBitmapFromData( display, root_window, data, 1, 1 );
        if (pixmapBits)
        {
            cursor = XCreatePixmapCursor( display, pixmapBits, pixmapBits,
                                          &bg, &bg, 0, 0 );
            XFreePixmap( display, pixmapBits );
        }
    }
    else  /* Create the X cursor from the bits */
    {
        XImage *image;
        GC gc;

        TRACE("Bitmap %dx%d planes=%d bpp=%d bytesperline=%d\n",
            ptr->nWidth, ptr->nHeight, ptr->bPlanes, ptr->bBitsPerPixel,
            ptr->nWidthBytes);

        /* Create a pixmap and transfer all the bits to it */

        /* NOTE: Following hack works, but only because XFree depth
         *       1 images really use 1 bit/pixel (and so the same layout
         *       as the Windows cursor data). Perhaps use a more generic
         *       algorithm here.
         */
        /* This pixmap will be written with two bitmaps. The first is
         *  the mask and the second is the image.
         */
        if (!(pixmapAll = XCreatePixmap( display, root_window,
                  ptr->nWidth, ptr->nHeight * 2, 1 )))
            return 0;
        if (!(image = XCreateImage( display, visual,
                1, ZPixmap, 0, (char *)(ptr + 1), ptr->nWidth,
                ptr->nHeight * 2, 16, ptr->nWidthBytes/ptr->bBitsPerPixel)))
        {
            XFreePixmap( display, pixmapAll );
            return 0;
        }
        gc = XCreateGC( display, pixmapAll, 0, NULL );
        XSetGraphicsExposures( display, gc, False );
        image->byte_order = MSBFirst;
        image->bitmap_bit_order = MSBFirst;
        image->bitmap_unit = 16;
        _XInitImageFuncPtrs(image);
        if (ptr->bPlanes * ptr->bBitsPerPixel == 1)
        {
            /* A plain old white on black cursor. */
            fg.red = fg.green = fg.blue = 0xffff;
            bg.red = bg.green = bg.blue = 0x0000;
            XPutImage( display, pixmapAll, gc, image,
                0, 0, 0, 0, ptr->nWidth, ptr->nHeight * 2 );
        }
        else
        {
            int     rbits, gbits, bbits, red, green, blue;
            int     rfg, gfg, bfg, rbg, gbg, bbg;
            int     rscale, gscale, bscale;
            int     x, y, xmax, ymax, byteIndex, xorIndex;
            unsigned char *theMask, *theImage, theChar;
            int     threshold, fgBits, bgBits, bitShifted;
            BYTE    pXorBits[128];   /* Up to 32x32 icons */

            switch (ptr->bBitsPerPixel)
            {
            case 32:
                bitMask32 = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY,
                                       ptr->nWidth * ptr->nHeight / 8 );
                /* Fallthrough */
            case 24:
                rbits = 8;
                gbits = 8;
                bbits = 8;
                threshold = 0x40;
                break;
            case 16:
                rbits = 5;
                gbits = 6;
                bbits = 5;
                threshold = 0x40;
                break;
            default:
                FIXME("Currently no support for cursors with %d bits per pixel\n",
                  ptr->bBitsPerPixel);
                XFreePixmap( display, pixmapAll );
                XFreeGC( display, gc );
                image->data = NULL;
                XDestroyImage( image );
                return 0;
            }
            /* The location of the mask. */
            theMask = (unsigned char *)(ptr + 1);
            /* The mask should still be 1 bit per pixel. The color image
             * should immediately follow the mask.
             */
            theImage = &theMask[ptr->nWidth/8 * ptr->nHeight];
            rfg = gfg = bfg = rbg = gbg = bbg = 0;
            byteIndex = 0;
            xorIndex = 0;
            fgBits = 0;
            bitShifted = 0x01;
            xmax = (ptr->nWidth > 32) ? 32 : ptr->nWidth;
            if (ptr->nWidth > 32) {
                ERR("Got a %dx%d cursor. Cannot handle larger than 32x32.\n",
                  ptr->nWidth, ptr->nHeight);
            }
            ymax = (ptr->nHeight > 32) ? 32 : ptr->nHeight;
            alpha_zero = check_alpha_zero(ptr, theImage);

            memset(pXorBits, 0, 128);
            for (y=0; y<ymax; y++)
            {
                for (x=0; x<xmax; x++)
                {
                   	red = green = blue = 0;
                   	switch (ptr->bBitsPerPixel)
                   	{
			case 32:
			    theChar = theImage[byteIndex++];
			    blue = theChar;
			    theChar = theImage[byteIndex++];
			    green = theChar;
			    theChar = theImage[byteIndex++];
			    red = theChar;
			    theChar = theImage[byteIndex++];
                            /* If the alpha channel is >5% transparent,
                             * assume that we can add it to the bitMask32.
                             */
                            if (theChar > 0x0D)
                                *(bitMask32 + (y*xmax+x)/8) |= 1 << (x & 7);
			    break;
                   	case 24:
                   	    theChar = theImage[byteIndex++];
                   	    blue = theChar;
                   	    theChar = theImage[byteIndex++];
                   	    green = theChar;
                   	    theChar = theImage[byteIndex++];
                   	    red = theChar;
                   	    break;
                   	case 16:
                   	    theChar = theImage[byteIndex++];
                   	    blue = theChar & 0x1F;
                   	    green = (theChar & 0xE0) >> 5;
                   	    theChar = theImage[byteIndex++];
                   	    green |= (theChar & 0x07) << 3;
                   	    red = (theChar & 0xF8) >> 3;
                   	    break;
                   	}

                    if (red+green+blue > threshold)
                    {
                        rfg += red;
                        gfg += green;
                        bfg += blue;
                        fgBits++;
                        pXorBits[xorIndex] |= bitShifted;
                    }
                    else
                    {
                        rbg += red;
                        gbg += green;
                        bbg += blue;
                    }
                    if (x%8 == 7)
                    {
                        bitShifted = 0x01;
                        xorIndex++;
                    }
                    else
                        bitShifted = bitShifted << 1;
                }
            }
            rscale = 1 << (16 - rbits);
            gscale = 1 << (16 - gbits);
            bscale = 1 << (16 - bbits);
            if (fgBits)
            {
                fg.red   = rfg * rscale / fgBits;
                fg.green = gfg * gscale / fgBits;
                fg.blue  = bfg * bscale / fgBits;
            }
            else fg.red = fg.green = fg.blue = 0;
            bgBits = xmax * ymax - fgBits;
            if (bgBits)
            {
                bg.red   = rbg * rscale / bgBits;
                bg.green = gbg * gscale / bgBits;
                bg.blue  = bbg * bscale / bgBits;
            }
            else bg.red = bg.green = bg.blue = 0;
            pixmapBits = XCreateBitmapFromData( display, root_window, (char *)pXorBits, xmax, ymax );
            if (!pixmapBits)
            {
                HeapFree( GetProcessHeap(), 0, bitMask32 );
                XFreePixmap( display, pixmapAll );
                XFreeGC( display, gc );
                image->data = NULL;
                XDestroyImage( image );
                return 0;
            }

            /* Put the mask. */
            XPutImage( display, pixmapAll, gc, image,
                   0, 0, 0, 0, ptr->nWidth, ptr->nHeight );
            XSetFunction( display, gc, GXcopy );
            /* Put the image */
            XCopyArea( display, pixmapBits, pixmapAll, gc,
                       0, 0, xmax, ymax, 0, ptr->nHeight );
            XFreePixmap( display, pixmapBits );
        }
        image->data = NULL;
        XDestroyImage( image );

        /* Now create the 2 pixmaps for bits and mask */

        pixmapBits = XCreatePixmap( display, root_window, ptr->nWidth, ptr->nHeight, 1 );
        if (alpha_zero)
        {
            pixmapMaskInv = XCreatePixmap( display, root_window, ptr->nWidth, ptr->nHeight, 1 );
            pixmapMask = XCreatePixmap( display, root_window, ptr->nWidth, ptr->nHeight, 1 );

            /* Make sure everything went OK so far */
            if (pixmapBits && pixmapMask && pixmapMaskInv)
            {
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
                 *
                 * FIXME: apparently some servers do support 'inverted' color.
                 * I don't know if it's correct per the X spec, but maybe we
                 * ought to take advantage of it.  -- AJ
                 */
                XSetFunction( display, gc, GXcopy );
                XCopyArea( display, pixmapAll, pixmapBits, gc,
                           0, 0, ptr->nWidth, ptr->nHeight, 0, 0 );
                XCopyArea( display, pixmapAll, pixmapMask, gc,
                           0, 0, ptr->nWidth, ptr->nHeight, 0, 0 );
                XCopyArea( display, pixmapAll, pixmapMaskInv, gc,
                           0, 0, ptr->nWidth, ptr->nHeight, 0, 0 );
                XSetFunction( display, gc, GXand );
                XCopyArea( display, pixmapAll, pixmapMaskInv, gc,
                           0, ptr->nHeight, ptr->nWidth, ptr->nHeight, 0, 0 );
                XSetFunction( display, gc, GXandReverse );
                XCopyArea( display, pixmapAll, pixmapBits, gc,
                           0, ptr->nHeight, ptr->nWidth, ptr->nHeight, 0, 0 );
                XSetFunction( display, gc, GXorReverse );
                XCopyArea( display, pixmapAll, pixmapMask, gc,
                           0, ptr->nHeight, ptr->nWidth, ptr->nHeight, 0, 0 );
                /* Additional white */
                XSetFunction( display, gc, GXor );
                XCopyArea( display, pixmapMaskInv, pixmapMask, gc,
                           0, 0, ptr->nWidth, ptr->nHeight, 1, 1 );
                XCopyArea( display, pixmapMaskInv, pixmapBits, gc,
                           0, 0, ptr->nWidth, ptr->nHeight, 1, 1 );
                XSetFunction( display, gc, GXcopy );
            }
        }
        else
        {
            pixmapMask = XCreateBitmapFromData( display, root_window,
                                                bitMask32, ptr->nWidth,
                                                ptr->nHeight );
        }

        /* Make sure hotspot is valid */
        hotspot.x = ptr->ptHotSpot.x;
        hotspot.y = ptr->ptHotSpot.y;
        if (hotspot.x < 0 || hotspot.x >= ptr->nWidth ||
            hotspot.y < 0 || hotspot.y >= ptr->nHeight)
        {
            hotspot.x = ptr->nWidth / 2;
            hotspot.y = ptr->nHeight / 2;
        }

        if (pixmapBits && pixmapMask)
            cursor = XCreatePixmapCursor( display, pixmapBits, pixmapMask,
                                          &fg, &bg, hotspot.x, hotspot.y );

        /* Now free everything */

        if (pixmapAll) XFreePixmap( display, pixmapAll );
        if (pixmapBits) XFreePixmap( display, pixmapBits );
        if (pixmapMask) XFreePixmap( display, pixmapMask );
        if (pixmapMaskInv) XFreePixmap( display, pixmapMaskInv );
        HeapFree( GetProcessHeap(), 0, bitMask32 );
        XFreeGC( display, gc );
    }
    return cursor;
}


/***********************************************************************
 *		SetCursor (X11DRV.@)
 */
void CDECL X11DRV_SetCursor( CURSORICONINFO *lpCursor )
{
    struct x11drv_thread_data *data = x11drv_init_thread_data();
    Cursor cursor;

    if (lpCursor)
        TRACE("%ux%u, planes %u, bpp %u\n",
              lpCursor->nWidth, lpCursor->nHeight, lpCursor->bPlanes, lpCursor->bBitsPerPixel);
    else
        TRACE("NULL\n");

    /* set the same cursor for all top-level windows of the current thread */

    wine_tsx11_lock();
    cursor = create_cursor( data->display, lpCursor );
    if (cursor)
    {
        if (data->cursor) XFreeCursor( data->display, data->cursor );
        data->cursor = cursor;
        if (data->cursor_window)
        {
            XDefineCursor( data->display, data->cursor_window, cursor );
            /* Make the change take effect immediately */
            XFlush( data->display );
        }
    }
    wine_tsx11_unlock();
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
        queue_raw_mouse_message( WM_MOUSEMOVE, NULL, x, y, 0, GetCurrentTime(), 0, 0 );
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

    if (buttonNum >= NB_BUTTONS) return;
    if (!hwnd) return;

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

    update_mouse_state( hwnd, event->window, event->x, event->y, event->state, &pt );

    X11DRV_send_mouse_input( hwnd, button_down_flags[buttonNum] | MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE,
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

    if (buttonNum >= NB_BUTTONS || !button_up_flags[buttonNum]) return;
    if (!hwnd) return;

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

    update_mouse_state( hwnd, event->window, event->x, event->y, event->state, &pt );

    X11DRV_send_mouse_input( hwnd, button_up_flags[buttonNum] | MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE,
                             pt.x, pt.y, wData, EVENT_x11_time_to_win32_time(event->time), 0, 0 );
}


/***********************************************************************
 *           X11DRV_MotionNotify
 */
void X11DRV_MotionNotify( HWND hwnd, XEvent *xev )
{
    XMotionEvent *event = &xev->xmotion;
    POINT pt;

    TRACE("hwnd %p, event->is_hint %d\n", hwnd, event->is_hint);

    if (!hwnd) return;

    update_mouse_state( hwnd, event->window, event->x, event->y, event->state, &pt );

    X11DRV_send_mouse_input( hwnd, MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE,
                             pt.x, pt.y, 0, EVENT_x11_time_to_win32_time(event->time), 0, 0 );
}


/***********************************************************************
 *           X11DRV_EnterNotify
 */
void X11DRV_EnterNotify( HWND hwnd, XEvent *xev )
{
    XCrossingEvent *event = &xev->xcrossing;
    POINT pt;

    TRACE("hwnd %p, event->detail %d\n", hwnd, event->detail);

    if (!hwnd) return;
    if (event->detail == NotifyVirtual || event->detail == NotifyNonlinearVirtual) return;
    if (event->window == x11drv_thread_data()->grab_window) return;

    /* simulate a mouse motion event */
    update_mouse_state( hwnd, event->window, event->x, event->y, event->state, &pt );

    X11DRV_send_mouse_input( hwnd, MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE,
                             pt.x, pt.y, 0, EVENT_x11_time_to_win32_time(event->time), 0, 0 );
}
