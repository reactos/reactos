/*
 * X11 system tray management
 *
 * Copyright (C) 2004 Mike Hearn, for CodeWeavers
 * Copyright (C) 2005 Robert Shearman
 * Copyright (C) 2008 Alexandre Julliard
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

#include <assert.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#include <X11/Xlib.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "commctrl.h"
#include "shellapi.h"

#include "x11drv.h"
#include "wine/list.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(systray);

/* an individual systray icon */
struct tray_icon
{
    struct list    entry;
    HICON          image;    /* the image to render */
    HWND           owner;    /* the HWND passed in to the Shell_NotifyIcon call */
    HWND           window;   /* the adaptor window */
    HWND           tooltip;  /* Icon tooltip */
    UINT           id;       /* the unique id given by the app */
    UINT           callback_message;
    WCHAR          tiptext[256]; /* Tooltip text. If empty => tooltip disabled */
    WCHAR          tiptitle[64]; /* Tooltip title for ballon style tooltips.  If empty => tooltip is not balloon style. */
};

static struct list icon_list = LIST_INIT( icon_list );

static const WCHAR tray_classname[] = {'_','_','w','i','n','e','x','1','1','_','t','r','a','y','_','w','i','n','d','o','w',0};

static BOOL delete_icon( struct tray_icon *icon );

#define SYSTEM_TRAY_REQUEST_DOCK  0
#define SYSTEM_TRAY_BEGIN_MESSAGE   1
#define SYSTEM_TRAY_CANCEL_MESSAGE  2

#define ICON_BORDER 2

/* retrieves icon record by owner window and ID */
static struct tray_icon *get_icon(HWND owner, UINT id)
{
    struct tray_icon *this;

    LIST_FOR_EACH_ENTRY( this, &icon_list, struct tray_icon, entry )
        if ((this->id == id) && (this->owner == owner)) return this;
    return NULL;
}

/* create tooltip window for icon */
static void create_tooltip(struct tray_icon *icon)
{
    static BOOL tooltips_initialized = FALSE;

    if (!tooltips_initialized)
    {
        INITCOMMONCONTROLSEX init_tooltip;

        init_tooltip.dwSize = sizeof(INITCOMMONCONTROLSEX);
        init_tooltip.dwICC = ICC_TAB_CLASSES;

        InitCommonControlsEx(&init_tooltip);
        tooltips_initialized = TRUE;
    }
    if (icon->tiptitle[0] != 0)
    {
        icon->tooltip = CreateWindowExW( WS_EX_TOPMOST, TOOLTIPS_CLASSW, NULL,
                                         WS_POPUP | TTS_ALWAYSTIP | TTS_BALLOON,
                                         CW_USEDEFAULT, CW_USEDEFAULT,
                                         CW_USEDEFAULT, CW_USEDEFAULT,
                                         icon->window, NULL, NULL, NULL);
    }
    else
    {
        icon->tooltip = CreateWindowExW( WS_EX_TOPMOST, TOOLTIPS_CLASSW, NULL,
                                         WS_POPUP | TTS_ALWAYSTIP,
                                         CW_USEDEFAULT, CW_USEDEFAULT,
                                         CW_USEDEFAULT, CW_USEDEFAULT,
                                         icon->window, NULL, NULL, NULL);
    }
    if (icon->tooltip)
    {
        TTTOOLINFOW ti;
        ZeroMemory(&ti, sizeof(ti));
        ti.cbSize = sizeof(TTTOOLINFOW);
        ti.uFlags = TTF_SUBCLASS | TTF_IDISHWND;
        ti.hwnd = icon->window;
        ti.uId = (UINT_PTR)icon->window;
        ti.lpszText = icon->tiptext;
        SendMessageW(icon->tooltip, TTM_ADDTOOLW, 0, (LPARAM)&ti);
    }
}

/* synchronize tooltip text with tooltip window */
static void update_tooltip_text(struct tray_icon *icon)
{
    TTTOOLINFOW ti;

    ZeroMemory(&ti, sizeof(ti));
    ti.cbSize = sizeof(TTTOOLINFOW);
    ti.uFlags = TTF_SUBCLASS | TTF_IDISHWND;
    ti.hwnd = icon->window;
    ti.uId = (UINT_PTR)icon->window;
    ti.lpszText = icon->tiptext;

    SendMessageW(icon->tooltip, TTM_UPDATETIPTEXTW, 0, (LPARAM)&ti);
}

/* window procedure for the tray window */
static LRESULT WINAPI tray_wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    struct tray_icon *icon = NULL;
    BOOL ret;

    WINE_TRACE("hwnd=%p, msg=0x%x\n", hwnd, msg);

    /* set the icon data for the window from the data passed into CreateWindow */
    if (msg == WM_NCCREATE)
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LPARAM)((const CREATESTRUCTW *)lparam)->lpCreateParams);

    icon = (struct tray_icon *) GetWindowLongPtrW(hwnd, GWLP_USERDATA);

    switch (msg)
    {
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            RECT rc;
            HDC hdc;
            int cx = GetSystemMetrics( SM_CXSMICON );
            int cy = GetSystemMetrics( SM_CYSMICON );

            hdc = BeginPaint(hwnd, &ps);
            GetClientRect(hwnd, &rc);
            TRACE("painting rect %s\n", wine_dbgstr_rect(&rc));
            DrawIconEx( hdc, (rc.left + rc.right - cx) / 2, (rc.top + rc.bottom - cy) / 2,
                        icon->image, cx, cy, 0, 0, DI_DEFAULTSIZE|DI_NORMAL );
            EndPaint(hwnd, &ps);
            break;
        }

    case WM_MOUSEMOVE:
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
    case WM_MBUTTONDOWN:
    case WM_MBUTTONUP:
    case WM_LBUTTONDBLCLK:
    case WM_RBUTTONDBLCLK:
    case WM_MBUTTONDBLCLK:
        /* notify the owner hwnd of the message */
        TRACE("relaying 0x%x\n", msg);
        ret = PostMessageW(icon->owner, icon->callback_message, icon->id, msg);
        if (!ret && (GetLastError() == ERROR_INVALID_WINDOW_HANDLE))
        {
            WARN( "application window was destroyed, removing icon %u\n", icon->id );
            delete_icon( icon );
        }
        break;

    case WM_TIMER:
        if (!IsWindow( icon->owner )) delete_icon( icon );
        return 0;

    default:
        return DefWindowProcW(hwnd, msg, wparam, lparam);
    }
    return 0;
}

/* find the X11 window owner the system tray selection */
static Window get_systray_selection_owner( Display *display )
{
    static Atom systray_atom;
    Window ret;

    if (root_window != DefaultRootWindow( display )) return 0;

    wine_tsx11_lock();
    if (!systray_atom)
    {
        if (DefaultScreen( display ) == 0)
            systray_atom = x11drv_atom(_NET_SYSTEM_TRAY_S0);
        else
        {
            char systray_buffer[29]; /* strlen(_NET_SYSTEM_TRAY_S4294967295)+1 */
            sprintf( systray_buffer, "_NET_SYSTEM_TRAY_S%u", DefaultScreen( display ) );
            systray_atom = XInternAtom( display, systray_buffer, False );
        }
    }
    ret = XGetSelectionOwner( display, systray_atom );
    wine_tsx11_unlock();
    return ret;
}


/* dock the given X window with the NETWM system tray */
static void dock_systray_window( Display *display, HWND hwnd, Window systray_window )
{
    struct x11drv_win_data *data;
    XEvent ev;
    XSetWindowAttributes attr;

    if (!(data = X11DRV_get_win_data( hwnd )) &&
        !(data = X11DRV_create_win_data( hwnd ))) return;

    TRACE( "icon window %p/%lx managed %u\n", data->hwnd, data->whole_window, data->managed );

    make_window_embedded( display, data );

    /* send the docking request message */
    ev.xclient.type = ClientMessage;
    ev.xclient.window = systray_window;
    ev.xclient.message_type = x11drv_atom( _NET_SYSTEM_TRAY_OPCODE );
    ev.xclient.format = 32;
    ev.xclient.data.l[0] = CurrentTime;
    ev.xclient.data.l[1] = SYSTEM_TRAY_REQUEST_DOCK;
    ev.xclient.data.l[2] = data->whole_window;
    ev.xclient.data.l[3] = 0;
    ev.xclient.data.l[4] = 0;
    wine_tsx11_lock();
    XSendEvent( display, systray_window, False, NoEventMask, &ev );
    attr.background_pixmap = ParentRelative;
    attr.bit_gravity = ForgetGravity;
    XChangeWindowAttributes( display, data->whole_window, CWBackPixmap | CWBitGravity, &attr );
    XChangeWindowAttributes( display, data->client_window, CWBackPixmap | CWBitGravity, &attr );
    wine_tsx11_unlock();
}


/* hide a tray icon */
static BOOL hide_icon( struct tray_icon *icon )
{
    TRACE( "id=0x%x, hwnd=%p\n", icon->id, icon->owner );

    if (!icon->window) return TRUE;  /* already hidden */

    DestroyWindow(icon->window);
    DestroyWindow(icon->tooltip);
    icon->window = 0;
    icon->tooltip = 0;
    return TRUE;
}

/* make the icon visible */
static BOOL show_icon( struct tray_icon *icon )
{
    RECT rect;
    static BOOL class_registered;
    Window systray_window;
    Display *display = thread_display();

    TRACE( "id=0x%x, hwnd=%p\n", icon->id, icon->owner );

    if (icon->window) return TRUE;  /* already shown */

    if (!class_registered)
    {
        WNDCLASSEXW class;

        ZeroMemory( &class, sizeof(class) );
        class.cbSize        = sizeof(class);
        class.lpfnWndProc   = tray_wndproc;
        class.hCursor       = LoadCursorW( 0, (LPCWSTR)IDC_ARROW );
        class.lpszClassName = tray_classname;
        class.style         = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;

        if (!RegisterClassExW(&class) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
        {
            WINE_ERR( "Could not register tray window class\n" );
            return FALSE;
        }
        class_registered = TRUE;
    }

    if (!(systray_window = get_systray_selection_owner( display ))) return FALSE;

    rect.left = 0;
    rect.top = 0;
    rect.right = GetSystemMetrics( SM_CXSMICON ) + 2*ICON_BORDER;
    rect.bottom = GetSystemMetrics( SM_CYSMICON ) + 2*ICON_BORDER;

    icon->window = CreateWindowExW( WS_EX_APPWINDOW, tray_classname, NULL, WS_CLIPSIBLINGS | WS_POPUP,
                                    CW_USEDEFAULT, CW_USEDEFAULT,
                                    rect.right - rect.left, rect.bottom - rect.top,
                                    NULL, NULL, NULL, icon );
    create_tooltip( icon );
    dock_systray_window( display, icon->window, systray_window );
    SetTimer( icon->window, 1, 1000, NULL );
    ShowWindow( icon->window, SW_SHOWNA );
    return TRUE;
}

/* Modifies an existing icon record */
static BOOL modify_icon( struct tray_icon *icon, NOTIFYICONDATAW *nid )
{
    TRACE( "id=0x%x hwnd=%p flags=%x\n", nid->uID, nid->hWnd, nid->uFlags );

    if ((nid->uFlags & NIF_STATE) && (nid->dwStateMask & NIS_HIDDEN))
    {
        if (nid->dwState & NIS_HIDDEN) hide_icon( icon );
        else show_icon( icon );
    }

    if (nid->uFlags & NIF_ICON)
    {
        if (icon->image) DestroyIcon(icon->image);
        icon->image = CopyIcon(nid->hIcon);
        if (icon->window)
        {
            struct x11drv_win_data *data = X11DRV_get_win_data( icon->window );
            if (data) XClearArea( gdi_display, data->client_window, 0, 0, 0, 0, True );
        }
    }

    if (nid->uFlags & NIF_MESSAGE)
    {
        icon->callback_message = nid->uCallbackMessage;
    }
    if (nid->uFlags & NIF_TIP)
    {
        lstrcpynW(icon->tiptext, nid->szTip, sizeof(icon->tiptext)/sizeof(WCHAR));
        icon->tiptitle[0] = 0;
        if (icon->tooltip) update_tooltip_text(icon);
    }
    if (nid->uFlags & NIF_INFO && nid->cbSize >= NOTIFYICONDATAA_V2_SIZE)
    {
        lstrcpynW(icon->tiptext, nid->szInfo, sizeof(icon->tiptext)/sizeof(WCHAR));
        lstrcpynW(icon->tiptitle, nid->szInfoTitle, sizeof(icon->tiptitle)/sizeof(WCHAR));
        if (icon->tooltip) update_tooltip_text(icon);
    }
    return TRUE;
}

/* Adds a new icon record to the list */
static BOOL add_icon(NOTIFYICONDATAW *nid)
{
    struct tray_icon  *icon;

    WINE_TRACE("id=0x%x, hwnd=%p\n", nid->uID, nid->hWnd);

    if ((icon = get_icon(nid->hWnd, nid->uID)))
    {
        WINE_WARN("duplicate tray icon add, buggy app?\n");
        return FALSE;
    }

    if (!(icon = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*icon))))
    {
        WINE_ERR("out of memory\n");
        return FALSE;
    }

    ZeroMemory(icon, sizeof(struct tray_icon));
    icon->id     = nid->uID;
    icon->owner  = nid->hWnd;

    list_add_tail(&icon_list, &icon->entry);

    /* if hidden state is specified, modify_icon will take care of it */
    if (!((nid->uFlags & NIF_STATE) && (nid->dwStateMask & NIS_HIDDEN)))
        show_icon( icon );

    return modify_icon( icon, nid );
}

/* delete tray icon window and icon structure */
static BOOL delete_icon( struct tray_icon *icon )
{
    hide_icon( icon );
    list_remove( &icon->entry );
    DestroyIcon( icon->image );
    HeapFree( GetProcessHeap(), 0, icon );
    return TRUE;
}


/***********************************************************************
 *              wine_notify_icon   (X11DRV.@)
 *
 * Driver-side implementation of Shell_NotifyIcon.
 */
int CDECL wine_notify_icon( DWORD msg, NOTIFYICONDATAW *data )
{
    BOOL ret = FALSE;
    struct tray_icon *icon;

    switch (msg)
    {
    case NIM_ADD:
        if (!get_systray_selection_owner( thread_init_display() ))
            return -1;  /* fall back to default handling */
        ret = add_icon( data );
        break;
    case NIM_DELETE:
        if ((icon = get_icon( data->hWnd, data->uID ))) ret = delete_icon( icon );
        break;
    case NIM_MODIFY:
        if ((icon = get_icon( data->hWnd, data->uID ))) ret = modify_icon( icon, data );
        break;
    default:
        FIXME( "unhandled tray message: %u\n", msg );
        break;
    }
    return ret;
}
