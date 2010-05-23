/*
 * X11DRV desktop window handling
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

#include "config.h"
#include <X11/cursorfont.h>
#include <X11/Xlib.h>

#include "x11drv.h"

/* avoid conflict with field names in included win32 headers */
#undef Status
#include "ddrawi.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(x11drv);

/* data for resolution changing */
static LPDDHALMODEINFO dd_modes;
static unsigned int dd_mode_count;

static unsigned int max_width;
static unsigned int max_height;

static const unsigned int widths[]  = {320, 400, 512, 640, 800, 1024, 1152, 1280, 1400, 1600};
static const unsigned int heights[] = {200, 300, 384, 480, 600,  768,  864, 1024, 1050, 1200};
#define NUM_DESKTOP_MODES (sizeof(widths) / sizeof(widths[0]))

#define _NET_WM_STATE_REMOVE 0
#define _NET_WM_STATE_ADD 1

/* create the mode structures */
static void make_modes(void)
{
    unsigned int i;
    /* original specified desktop size */
    X11DRV_Settings_AddOneMode(screen_width, screen_height, 0, 60);
    for (i=0; i<NUM_DESKTOP_MODES; i++)
    {
        if ( (widths[i] <= max_width) && (heights[i] <= max_height) )
        {
            if ( ( (widths[i] != max_width) || (heights[i] != max_height) ) &&
                 ( (widths[i] != screen_width) || (heights[i] != screen_height) ) )
            {
                /* only add them if they are smaller than the root window and unique */
                X11DRV_Settings_AddOneMode(widths[i], heights[i], 0, 60);
            }
        }
    }
    if ((max_width != screen_width) && (max_height != screen_height))
    {
        /* root window size (if different from desktop window) */
        X11DRV_Settings_AddOneMode(max_width, max_height, 0, 60);
    }
}

static int X11DRV_desktop_GetCurrentMode(void)
{
    unsigned int i;
    DWORD dwBpp = screen_bpp;
    for (i=0; i<dd_mode_count; i++)
    {
        if ( (screen_width == dd_modes[i].dwWidth) &&
             (screen_height == dd_modes[i].dwHeight) && 
             (dwBpp == dd_modes[i].dwBPP))
            return i;
    }
    ERR("In unknown mode, returning default\n");
    return 0;
}

static LONG X11DRV_desktop_SetCurrentMode(int mode)
{
    DWORD dwBpp = screen_bpp;
    if (dwBpp != dd_modes[mode].dwBPP)
    {
        FIXME("Cannot change screen BPP from %d to %d\n", dwBpp, dd_modes[mode].dwBPP);
        /* Ignore the depth mismatch
         *
         * Some (older) applications require a specific bit depth, this will allow them
         * to run. X11drv performs a color depth conversion if needed.
         */
    }
    TRACE("Resizing Wine desktop window to %dx%d\n", dd_modes[mode].dwWidth, dd_modes[mode].dwHeight);
    X11DRV_resize_desktop(dd_modes[mode].dwWidth, dd_modes[mode].dwHeight);
    return DISP_CHANGE_SUCCESSFUL;
}

/***********************************************************************
 *		X11DRV_init_desktop
 *
 * Setup the desktop when not using the root window.
 */
void X11DRV_init_desktop( Window win, unsigned int width, unsigned int height )
{
    root_window = win;
    managed_mode = 0;  /* no managed windows in desktop mode */
    max_width = screen_width;
    max_height = screen_height;
    xinerama_init( width, height );

    /* initialize the available resolutions */
    dd_modes = X11DRV_Settings_SetHandlers("desktop", 
                                           X11DRV_desktop_GetCurrentMode, 
                                           X11DRV_desktop_SetCurrentMode, 
                                           NUM_DESKTOP_MODES+2, 1);
    make_modes();
    X11DRV_Settings_AddDepthModes();
    dd_mode_count = X11DRV_Settings_GetModeCount();
}


/***********************************************************************
 *		X11DRV_create_desktop
 *
 * Create the X11 desktop window for the desktop mode.
 */
Window CDECL X11DRV_create_desktop( UINT width, UINT height )
{
    XSetWindowAttributes win_attr;
    Window win;
    Display *display = thread_init_display();

    TRACE( "%u x %u\n", width, height );

    wine_tsx11_lock();

    /* Create window */
    win_attr.event_mask = ExposureMask | KeyPressMask | KeyReleaseMask | EnterWindowMask |
                          PointerMotionMask | ButtonPressMask | ButtonReleaseMask;
    win_attr.cursor = XCreateFontCursor( display, XC_top_left_arrow );

    if (visual != DefaultVisual( display, DefaultScreen(display) ))
        win_attr.colormap = XCreateColormap( display, DefaultRootWindow(display),
                                             visual, AllocNone );
    else
        win_attr.colormap = None;

    win = XCreateWindow( display, DefaultRootWindow(display),
                         0, 0, width, height, 0, screen_depth, InputOutput, visual,
                         CWEventMask | CWCursor | CWColormap, &win_attr );
    if (win != None && width == screen_width && height == screen_height)
    {
        TRACE("setting desktop to fullscreen\n");
        XChangeProperty( display, win, x11drv_atom(_NET_WM_STATE), XA_ATOM, 32,
            PropModeReplace, (unsigned char*)&x11drv_atom(_NET_WM_STATE_FULLSCREEN),
            1);
    }
    XFlush( display );
    wine_tsx11_unlock();
    if (win != None) X11DRV_init_desktop( win, width, height );
    return win;
}


struct desktop_resize_data
{
    RECT  old_screen_rect;
    RECT  old_virtual_rect;
};

static BOOL CALLBACK update_windows_on_desktop_resize( HWND hwnd, LPARAM lparam )
{
    struct x11drv_win_data *data;
    Display *display = thread_display();
    struct desktop_resize_data *resize_data = (struct desktop_resize_data *)lparam;
    int mask = 0;

    if (!(data = X11DRV_get_win_data( hwnd ))) return TRUE;

    /* update the full screen state */
    update_net_wm_states( display, data );

    if (resize_data->old_virtual_rect.left != virtual_screen_rect.left) mask |= CWX;
    if (resize_data->old_virtual_rect.top != virtual_screen_rect.top) mask |= CWY;
    if (mask && data->whole_window)
    {
        XWindowChanges changes;

        wine_tsx11_lock();
        changes.x = data->whole_rect.left - virtual_screen_rect.left;
        changes.y = data->whole_rect.top - virtual_screen_rect.top;
        XReconfigureWMWindow( display, data->whole_window,
                              DefaultScreen(display), mask, &changes );
        wine_tsx11_unlock();
    }
    return TRUE;
}

static void update_desktop_fullscreen( unsigned int width, unsigned int height)
{
    Display *display = thread_display();
    XEvent xev;

    if (!display || root_window != DefaultRootWindow( display )) return;

    xev.xclient.type = ClientMessage;
    xev.xclient.window = root_window;
    xev.xclient.message_type = x11drv_atom(_NET_WM_STATE);
    xev.xclient.serial = 0;
    xev.xclient.display = display;
    xev.xclient.send_event = True;
    xev.xclient.format = 32;
    if (width == max_width && height == max_height)
        xev.xclient.data.l[0] = _NET_WM_STATE_ADD;
    else
        xev.xclient.data.l[0] = _NET_WM_STATE_REMOVE;
    xev.xclient.data.l[1] = x11drv_atom(_NET_WM_STATE_FULLSCREEN);
    xev.xclient.data.l[2] = 0;
    xev.xclient.data.l[3] = 1;

    TRACE("action=%li\n", xev.xclient.data.l[0]);

    wine_tsx11_lock();
    XSendEvent( display, DefaultRootWindow(display), False,
                SubstructureRedirectMask | SubstructureNotifyMask, &xev );
    wine_tsx11_unlock();
}

/***********************************************************************
 *		X11DRV_resize_desktop
 */
void X11DRV_resize_desktop( unsigned int width, unsigned int height )
{
    HWND hwnd = GetDesktopWindow();
    struct desktop_resize_data resize_data;

    SetRect( &resize_data.old_screen_rect, 0, 0, screen_width, screen_height );
    resize_data.old_virtual_rect = virtual_screen_rect;

    xinerama_init( width, height );

    if (GetWindowThreadProcessId( hwnd, NULL ) != GetCurrentThreadId())
    {
        SendMessageW( hwnd, WM_X11DRV_RESIZE_DESKTOP, 0, MAKELPARAM( width, height ) );
    }
    else
    {
        TRACE( "desktop %p change to (%dx%d)\n", hwnd, width, height );
        update_desktop_fullscreen( width, height );
        SetWindowPos( hwnd, 0, virtual_screen_rect.left, virtual_screen_rect.top,
                      virtual_screen_rect.right - virtual_screen_rect.left,
                      virtual_screen_rect.bottom - virtual_screen_rect.top,
                      SWP_NOZORDER | SWP_NOACTIVATE | SWP_DEFERERASE );
        SendMessageTimeoutW( HWND_BROADCAST, WM_DISPLAYCHANGE, screen_bpp,
                             MAKELPARAM( width, height ), SMTO_ABORTIFHUNG, 2000, NULL );
    }

    EnumWindows( update_windows_on_desktop_resize, (LPARAM)&resize_data );
}
