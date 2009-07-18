/*
 * X11 event driver
 *
 * Copyright 1993 Alexandre Julliard
 *	     1999 Noel Borthwick
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

#ifdef HAVE_POLL_H
#include <poll.h>
#endif
#ifdef HAVE_SYS_POLL_H
#include <sys/poll.h>
#endif
#include <X11/Xatom.h>
#include <X11/keysym.h>
#include <X11/Xlib.h>
#include <X11/Xresource.h>
#include <X11/Xutil.h>

#include <assert.h>
#include <stdarg.h>
#include <string.h>

#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "wingdi.h"

#include "x11drv.h"

/* avoid conflict with field names in included win32 headers */
#undef Status
#include "shlobj.h"  /* DROPFILES */
#include "shellapi.h"

#include "wine/server.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(event);

extern BOOL ximInComposeMode;

#define DndNotDnd       -1    /* OffiX drag&drop */
#define DndUnknown      0
#define DndRawData      1
#define DndFile         2
#define DndFiles        3
#define DndText         4
#define DndDir          5
#define DndLink         6
#define DndExe          7

#define DndEND          8

#define DndURL          128   /* KDE drag&drop */

  /* Event handlers */
static void X11DRV_FocusIn( HWND hwnd, XEvent *event );
static void X11DRV_FocusOut( HWND hwnd, XEvent *event );
static void X11DRV_Expose( HWND hwnd, XEvent *event );
static void X11DRV_MapNotify( HWND hwnd, XEvent *event );
static void X11DRV_ConfigureNotify( HWND hwnd, XEvent *event );
static void X11DRV_PropertyNotify( HWND hwnd, XEvent *event );
static void X11DRV_ClientMessage( HWND hwnd, XEvent *event );

struct event_handler
{
    int                  type;    /* event type */
    x11drv_event_handler handler; /* corresponding handler function */
};

#define MAX_EVENT_HANDLERS 64

static struct event_handler handlers[MAX_EVENT_HANDLERS] =
{
    /* list must be sorted by event type */
    { KeyPress,         X11DRV_KeyEvent },
    { KeyRelease,       X11DRV_KeyEvent },
    { ButtonPress,      X11DRV_ButtonPress },
    { ButtonRelease,    X11DRV_ButtonRelease },
    { MotionNotify,     X11DRV_MotionNotify },
    { EnterNotify,      X11DRV_EnterNotify },
    /* LeaveNotify */
    { FocusIn,          X11DRV_FocusIn },
    { FocusOut,         X11DRV_FocusOut },
    { KeymapNotify,     X11DRV_KeymapNotify },
    { Expose,           X11DRV_Expose },
    /* GraphicsExpose */
    /* NoExpose */
    /* VisibilityNotify */
    /* CreateNotify */
    { DestroyNotify,    X11DRV_DestroyNotify },
    /* UnmapNotify */
    { MapNotify,        X11DRV_MapNotify },
    /* MapRequest */
    /* ReparentNotify */
    { ConfigureNotify,  X11DRV_ConfigureNotify },
    /* ConfigureRequest */
    /* GravityNotify */
    /* ResizeRequest */
    /* CirculateNotify */
    /* CirculateRequest */
    { PropertyNotify,   X11DRV_PropertyNotify },
    { SelectionClear,   X11DRV_SelectionClear },
    { SelectionRequest, X11DRV_SelectionRequest },
    /* SelectionNotify */
    /* ColormapNotify */
    { ClientMessage,    X11DRV_ClientMessage },
    { MappingNotify,    X11DRV_MappingNotify },
};

static int nb_event_handlers = 18;  /* change this if you add handlers above */


/* return the name of an X event */
static const char *dbgstr_event( int type )
{
    static const char * const event_names[] =
    {
        "KeyPress", "KeyRelease", "ButtonPress", "ButtonRelease",
        "MotionNotify", "EnterNotify", "LeaveNotify", "FocusIn", "FocusOut",
        "KeymapNotify", "Expose", "GraphicsExpose", "NoExpose", "VisibilityNotify",
        "CreateNotify", "DestroyNotify", "UnmapNotify", "MapNotify", "MapRequest",
        "ReparentNotify", "ConfigureNotify", "ConfigureRequest", "GravityNotify",
        "ResizeRequest", "CirculateNotify", "CirculateRequest", "PropertyNotify",
        "SelectionClear", "SelectionRequest", "SelectionNotify", "ColormapNotify",
        "ClientMessage", "MappingNotify"
    };

    if (type >= KeyPress && type <= MappingNotify) return event_names[type - KeyPress];
    return wine_dbg_sprintf( "Extension event %d", type );
}


/***********************************************************************
 *           find_handler
 *
 * Find the handler for a given event type. Caller must hold the x11 lock.
 */
static inline x11drv_event_handler find_handler( int type )
{
    int min = 0, max = nb_event_handlers - 1;

    while (min <= max)
    {
        int pos = (min + max) / 2;
        if (handlers[pos].type == type) return handlers[pos].handler;
        if (handlers[pos].type > type) max = pos - 1;
        else min = pos + 1;
    }
    return NULL;
}


/***********************************************************************
 *           X11DRV_register_event_handler
 *
 * Register a handler for a given event type.
 * If already registered, overwrite the previous handler.
 */
void X11DRV_register_event_handler( int type, x11drv_event_handler handler )
{
    int min, max;

    wine_tsx11_lock();
    min = 0;
    max = nb_event_handlers - 1;
    while (min <= max)
    {
        int pos = (min + max) / 2;
        if (handlers[pos].type == type)
        {
            handlers[pos].handler = handler;
            goto done;
        }
        if (handlers[pos].type > type) max = pos - 1;
        else min = pos + 1;
    }
    /* insert it between max and min */
    memmove( &handlers[min+1], &handlers[min], (nb_event_handlers - min) * sizeof(handlers[0]) );
    handlers[min].type = type;
    handlers[min].handler = handler;
    nb_event_handlers++;
    assert( nb_event_handlers <= MAX_EVENT_HANDLERS );
done:
    wine_tsx11_unlock();
    TRACE("registered handler %p for event %d count %d\n", handler, type, nb_event_handlers );
}


/***********************************************************************
 *           filter_event
 */
static Bool filter_event( Display *display, XEvent *event, char *arg )
{
    ULONG_PTR mask = (ULONG_PTR)arg;

    if ((mask & QS_ALLINPUT) == QS_ALLINPUT) return 1;

    switch(event->type)
    {
    case KeyPress:
    case KeyRelease:
    case KeymapNotify:
    case MappingNotify:
        return (mask & QS_KEY) != 0;
    case ButtonPress:
    case ButtonRelease:
        return (mask & QS_MOUSEBUTTON) != 0;
    case MotionNotify:
    case EnterNotify:
    case LeaveNotify:
        return (mask & QS_MOUSEMOVE) != 0;
    case Expose:
        return (mask & QS_PAINT) != 0;
    case FocusIn:
    case FocusOut:
    case MapNotify:
    case UnmapNotify:
    case ConfigureNotify:
    case PropertyNotify:
    case ClientMessage:
        return (mask & QS_POSTMESSAGE) != 0;
    default:
        return (mask & QS_SENDMESSAGE) != 0;
    }
}


enum event_merge_action
{
    MERGE_DISCARD,  /* discard the old event */
    MERGE_HANDLE,   /* handle the old event */
    MERGE_KEEP      /* keep the old event for future merging */
};

/***********************************************************************
 *           merge_events
 *
 * Try to merge 2 consecutive events.
 */
static enum event_merge_action merge_events( XEvent *prev, XEvent *next )
{
    switch (prev->type)
    {
    case ConfigureNotify:
        switch (next->type)
        {
        case ConfigureNotify:
            if (prev->xany.window == next->xany.window)
            {
                TRACE( "discarding duplicate ConfigureNotify for window %lx\n", prev->xany.window );
                return MERGE_DISCARD;
            }
            break;
        case Expose:
        case PropertyNotify:
            return MERGE_KEEP;
        }
        break;
    case MotionNotify:
        if (prev->xany.window == next->xany.window && next->type == MotionNotify)
        {
            TRACE( "discarding duplicate MotionNotify for window %lx\n", prev->xany.window );
            return MERGE_DISCARD;
        }
        break;
    }
    return MERGE_HANDLE;
}


/***********************************************************************
 *           call_event_handler
 */
static inline void call_event_handler( Display *display, XEvent *event )
{
    HWND hwnd;
    x11drv_event_handler handler;
    XEvent *prev;
    struct x11drv_thread_data *thread_data;

    if (!(handler = find_handler( event->type )))
    {
        TRACE( "%s for win %lx, ignoring\n", dbgstr_event( event->type ), event->xany.window );
        return;  /* no handler, ignore it */
    }

    if (XFindContext( display, event->xany.window, winContext, (char **)&hwnd ) != 0)
        hwnd = 0;  /* not for a registered window */
    if (!hwnd && event->xany.window == root_window) hwnd = GetDesktopWindow();

    TRACE( "%s for hwnd/window %p/%lx\n",
           dbgstr_event( event->type ), hwnd, event->xany.window );
    wine_tsx11_unlock();
    thread_data = x11drv_thread_data();
    prev = thread_data->current_event;
    thread_data->current_event = event;
    handler( hwnd, event );
    thread_data->current_event = prev;
    wine_tsx11_lock();
}


/***********************************************************************
 *           process_events
 */
static int process_events( Display *display, Bool (*filter)(), ULONG_PTR arg )
{
    XEvent event, prev_event;
    int count = 0;
    enum event_merge_action action = MERGE_DISCARD;

    prev_event.type = 0;
    wine_tsx11_lock();
    while (XCheckIfEvent( display, &event, filter, (char *)arg ))
    {
        count++;
        if (XFilterEvent( &event, None ))
        {
            /*
             * SCIM on linux filters key events strangely. It does not filter the
             * KeyPress events for these keys however it does filter the
             * KeyRelease events. This causes wine to become very confused as
             * to the keyboard state.
             *
             * We need to let those KeyRelease events be processed so that the
             * keyboard state is correct.
             */
            if (event.type == KeyRelease)
            {
                KeySym keysym = 0;
                XKeyEvent *keyevent = &event.xkey;

                XLookupString(keyevent, NULL, 0, &keysym, NULL);
                if (!(keysym == XK_Shift_L ||
                    keysym == XK_Shift_R ||
                    keysym == XK_Control_L ||
                    keysym == XK_Control_R ||
                    keysym == XK_Alt_R ||
                    keysym == XK_Alt_L ||
                    keysym == XK_Meta_R ||
                    keysym == XK_Meta_L))
                        continue; /* not a key we care about, ignore it */
            }
            else
                continue;  /* filtered, ignore it */
        }
        if (prev_event.type) action = merge_events( &prev_event, &event );
        switch( action )
        {
        case MERGE_DISCARD:  /* discard prev, keep new */
            prev_event = event;
            break;
        case MERGE_HANDLE:  /* handle prev, keep new */
            call_event_handler( display, &prev_event );
            prev_event = event;
            break;
        case MERGE_KEEP:  /* handle new, keep prev for future merging */
            call_event_handler( display, &event );
            break;
        }
    }
    XFlush( gdi_display );
    if (prev_event.type) call_event_handler( display, &prev_event );
    wine_tsx11_unlock();
    if (count) TRACE( "processed %d events\n", count );
    return count;
}


/***********************************************************************
 *           MsgWaitForMultipleObjectsEx   (X11DRV.@)
 */
DWORD CDECL X11DRV_MsgWaitForMultipleObjectsEx( DWORD count, const HANDLE *handles,
                                                DWORD timeout, DWORD mask, DWORD flags )
{
    DWORD ret;
    struct x11drv_thread_data *data = TlsGetValue( thread_data_tls_index );

    if (!data)
    {
        if (!count && !timeout) return WAIT_TIMEOUT;
        return WaitForMultipleObjectsEx( count, handles, flags & MWMO_WAITALL,
                                         timeout, flags & MWMO_ALERTABLE );
    }

    if (data->current_event) mask = 0;  /* don't process nested events */

    if (process_events( data->display, filter_event, mask )) ret = count - 1;
    else if (count || timeout)
    {
        ret = WaitForMultipleObjectsEx( count, handles, flags & MWMO_WAITALL,
                                        timeout, flags & MWMO_ALERTABLE );
        if (ret == count - 1) process_events( data->display, filter_event, mask );
    }
    else ret = WAIT_TIMEOUT;

    return ret;
}

/***********************************************************************
 *           EVENT_x11_time_to_win32_time
 *
 * Make our timer and the X timer line up as best we can
 *  Pass 0 to retrieve the current adjustment value (times -1)
 */
DWORD EVENT_x11_time_to_win32_time(Time time)
{
  static DWORD adjust = 0;
  DWORD now = GetTickCount();
  DWORD ret;

  if (! adjust && time != 0)
  {
    ret = now;
    adjust = time - now;
  }
  else
  {
      /* If we got an event in the 'future', then our clock is clearly wrong. 
         If we got it more than 10000 ms in the future, then it's most likely
         that the clock has wrapped.  */

      ret = time - adjust;
      if (ret > now && ((ret - now) < 10000) && time != 0)
      {
        adjust += ret - now;
        ret    -= ret - now;
      }
  }

  return ret;

}

/*******************************************************************
 *         can_activate_window
 *
 * Check if we can activate the specified window.
 */
static inline BOOL can_activate_window( HWND hwnd )
{
    LONG style = GetWindowLongW( hwnd, GWL_STYLE );
    if (!(style & WS_VISIBLE)) return FALSE;
    if ((style & (WS_POPUP|WS_CHILD)) == WS_CHILD) return FALSE;
    if (style & WS_MINIMIZE) return FALSE;
    if (hwnd == GetDesktopWindow()) return FALSE;
    return !(style & WS_DISABLED);
}


/**********************************************************************
 *              set_focus
 */
static void set_focus( Display *display, HWND hwnd, Time time )
{
    HWND focus;
    Window win;

    TRACE( "setting foreground window to %p\n", hwnd );
    SetForegroundWindow( hwnd );

    focus = GetFocus();
    if (focus) focus = GetAncestor( focus, GA_ROOT );
    win = X11DRV_get_whole_window(focus);

    if (win)
    {
        TRACE( "setting focus to %p (%lx) time=%ld\n", focus, win, time );
        wine_tsx11_lock();
        XSetInputFocus( display, win, RevertToParent, time );
        wine_tsx11_unlock();
    }
}


/**********************************************************************
 *              handle_wm_protocols
 */
static void handle_wm_protocols( HWND hwnd, XClientMessageEvent *event )
{
    Atom protocol = (Atom)event->data.l[0];

    if (!protocol) return;

    if (protocol == x11drv_atom(WM_DELETE_WINDOW))
    {
        /* Ignore the delete window request if the window has been disabled
         * and we are in managed mode. This is to disallow applications from
         * being closed by the window manager while in a modal state.
         */
        if (IsWindowEnabled(hwnd))
        {
            HMENU hSysMenu;

            if (GetClassLongW(hwnd, GCL_STYLE) & CS_NOCLOSE) return;
            hSysMenu = GetSystemMenu(hwnd, FALSE);
            if (hSysMenu)
            {
                UINT state = GetMenuState(hSysMenu, SC_CLOSE, MF_BYCOMMAND);
                if (state == 0xFFFFFFFF || (state & (MF_DISABLED | MF_GRAYED)))
                    return;
            }
            if (GetActiveWindow() != hwnd)
            {
                LRESULT ma = SendMessageW( hwnd, WM_MOUSEACTIVATE,
                                           (WPARAM)GetAncestor( hwnd, GA_ROOT ),
                                           MAKELONG(HTCLOSE,WM_LBUTTONDOWN) );
                switch(ma)
                {
                    case MA_NOACTIVATEANDEAT:
                    case MA_ACTIVATEANDEAT:
                        return;
                    case MA_NOACTIVATE:
                        break;
                    case MA_ACTIVATE:
                    case 0:
                        SetActiveWindow(hwnd);
                        break;
                    default:
                        WARN( "unknown WM_MOUSEACTIVATE code %d\n", (int) ma );
                        break;
                }
            }
            PostMessageW( hwnd, WM_X11DRV_DELETE_WINDOW, 0, 0 );
        }
    }
    else if (protocol == x11drv_atom(WM_TAKE_FOCUS))
    {
        Time event_time = (Time)event->data.l[1];
        HWND last_focus = x11drv_thread_data()->last_focus;

        TRACE( "got take focus msg for %p, enabled=%d, visible=%d (style %08x), focus=%p, active=%p, fg=%p, last=%p\n",
               hwnd, IsWindowEnabled(hwnd), IsWindowVisible(hwnd), GetWindowLongW(hwnd, GWL_STYLE),
               GetFocus(), GetActiveWindow(), GetForegroundWindow(), last_focus );

        if (can_activate_window(hwnd))
        {
            /* simulate a mouse click on the caption to find out
             * whether the window wants to be activated */
            LRESULT ma = SendMessageW( hwnd, WM_MOUSEACTIVATE,
                                       (WPARAM)GetAncestor( hwnd, GA_ROOT ),
                                       MAKELONG(HTCAPTION,WM_LBUTTONDOWN) );
            if (ma != MA_NOACTIVATEANDEAT && ma != MA_NOACTIVATE)
            {
                set_focus( event->display, hwnd, event_time );
                return;
            }
        }
        /* try to find some other window to give the focus to */
        hwnd = GetFocus();
        if (hwnd) hwnd = GetAncestor( hwnd, GA_ROOT );
        if (!hwnd) hwnd = GetActiveWindow();
        if (!hwnd) hwnd = last_focus;
        if (hwnd && can_activate_window(hwnd)) set_focus( event->display, hwnd, event_time );
    }
    else if (protocol == x11drv_atom(_NET_WM_PING))
    {
      XClientMessageEvent xev;
      xev = *event;
      
      TRACE("NET_WM Ping\n");
      wine_tsx11_lock();
      xev.window = DefaultRootWindow(xev.display);
      XSendEvent(xev.display, xev.window, False, SubstructureRedirectMask | SubstructureNotifyMask, (XEvent*)&xev);
      wine_tsx11_unlock();
      /* this line is semi-stolen from gtk2 */
      TRACE("NET_WM Pong\n");
    }
}


static const char * const focus_details[] =
{
    "NotifyAncestor",
    "NotifyVirtual",
    "NotifyInferior",
    "NotifyNonlinear",
    "NotifyNonlinearVirtual",
    "NotifyPointer",
    "NotifyPointerRoot",
    "NotifyDetailNone"
};

/**********************************************************************
 *              X11DRV_FocusIn
 */
static void X11DRV_FocusIn( HWND hwnd, XEvent *xev )
{
    XFocusChangeEvent *event = &xev->xfocus;
    XIC xic;

    if (!hwnd) return;

    TRACE( "win %p xwin %lx detail=%s\n", hwnd, event->window, focus_details[event->detail] );

    if (event->detail == NotifyPointer) return;

    if ((xic = X11DRV_get_ic( hwnd )))
    {
        wine_tsx11_lock();
        XSetICFocus( xic );
        wine_tsx11_unlock();
    }
    if (use_take_focus) return;  /* ignore FocusIn if we are using take focus */

    if (!can_activate_window(hwnd))
    {
        HWND hwnd = GetFocus();
        if (hwnd) hwnd = GetAncestor( hwnd, GA_ROOT );
        if (!hwnd) hwnd = GetActiveWindow();
        if (!hwnd) hwnd = x11drv_thread_data()->last_focus;
        if (hwnd && can_activate_window(hwnd)) set_focus( event->display, hwnd, CurrentTime );
    }
    else SetForegroundWindow( hwnd );
}


/**********************************************************************
 *              X11DRV_FocusOut
 *
 * Note: only top-level windows get FocusOut events.
 */
static void X11DRV_FocusOut( HWND hwnd, XEvent *xev )
{
    XFocusChangeEvent *event = &xev->xfocus;
    HWND hwnd_tmp;
    Window focus_win;
    int revert;
    XIC xic;

    if (!hwnd) return;

    TRACE( "win %p xwin %lx detail=%s\n", hwnd, event->window, focus_details[event->detail] );

    if (event->detail == NotifyPointer) return;
    if (ximInComposeMode) return;

    x11drv_thread_data()->last_focus = hwnd;
    if ((xic = X11DRV_get_ic( hwnd )))
    {
        wine_tsx11_lock();
        XUnsetICFocus( xic );
        wine_tsx11_unlock();
    }
    if (hwnd != GetForegroundWindow()) return;
    SendMessageW( hwnd, WM_CANCELMODE, 0, 0 );

    /* don't reset the foreground window, if the window which is
       getting the focus is a Wine window */

    wine_tsx11_lock();
    XGetInputFocus( event->display, &focus_win, &revert );
    if (focus_win)
    {
        if (XFindContext( event->display, focus_win, winContext, (char **)&hwnd_tmp ) != 0)
            focus_win = 0;
    }
    wine_tsx11_unlock();

    if (!focus_win)
    {
        /* Abey : 6-Oct-99. Check again if the focus out window is the
           Foreground window, because in most cases the messages sent
           above must have already changed the foreground window, in which
           case we don't have to change the foreground window to 0 */
        if (hwnd == GetForegroundWindow())
        {
            TRACE( "lost focus, setting fg to desktop\n" );
            SetForegroundWindow( GetDesktopWindow() );
        }
    }
}


/***********************************************************************
 *           X11DRV_Expose
 */
static void X11DRV_Expose( HWND hwnd, XEvent *xev )
{
    XExposeEvent *event = &xev->xexpose;
    RECT rect;
    struct x11drv_win_data *data;
    int flags = RDW_INVALIDATE | RDW_ERASE;

    TRACE( "win %p (%lx) %d,%d %dx%d\n",
           hwnd, event->window, event->x, event->y, event->width, event->height );

    if (!(data = X11DRV_get_win_data( hwnd ))) return;

    if (event->window == data->whole_window)
    {
        rect.left = data->whole_rect.left + event->x;
        rect.top  = data->whole_rect.top + event->y;
        flags |= RDW_FRAME;
    }
    else
    {
        rect.left = data->client_rect.left + event->x;
        rect.top  = data->client_rect.top + event->y;
    }
    rect.right  = rect.left + event->width;
    rect.bottom = rect.top + event->height;

    if (event->window != root_window)
    {
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

        /* make position relative to client area instead of parent */
        OffsetRect( &rect, -data->client_rect.left, -data->client_rect.top );
        flags |= RDW_ALLCHILDREN;
    }

    RedrawWindow( hwnd, &rect, 0, flags );
}


/**********************************************************************
 *		X11DRV_MapNotify
 */
static void X11DRV_MapNotify( HWND hwnd, XEvent *event )
{
    struct x11drv_win_data *data;

    if (!(data = X11DRV_get_win_data( hwnd ))) return;
    if (!data->mapped) return;

    if (!data->managed)
    {
        HWND hwndFocus = GetFocus();
        if (hwndFocus && IsChild( hwnd, hwndFocus )) X11DRV_SetFocus(hwndFocus);  /* FIXME */
    }
}


/***********************************************************************
 *     is_net_wm_state_maximized
 */
static BOOL is_net_wm_state_maximized( Display *display, struct x11drv_win_data *data )
{
    Atom type, *state;
    int format, ret = 0;
    unsigned long i, count, remaining;

    wine_tsx11_lock();
    if (!XGetWindowProperty( display, data->whole_window, x11drv_atom(_NET_WM_STATE), 0,
                             65536/sizeof(CARD32), False, XA_ATOM, &type, &format, &count,
                             &remaining, (unsigned char **)&state ))
    {
        if (type == XA_ATOM && format == 32)
        {
            for (i = 0; i < count; i++)
            {
                if (state[i] == x11drv_atom(_NET_WM_STATE_MAXIMIZED_VERT) ||
                    state[i] == x11drv_atom(_NET_WM_STATE_MAXIMIZED_HORZ))
                    ret++;
            }
        }
        XFree( state );
    }
    wine_tsx11_unlock();
    return (ret == 2);
}


/***********************************************************************
 *		X11DRV_ConfigureNotify
 */
void X11DRV_ConfigureNotify( HWND hwnd, XEvent *xev )
{
    XConfigureEvent *event = &xev->xconfigure;
    struct x11drv_win_data *data;
    RECT rect;
    UINT flags;
    int cx, cy, x = event->x, y = event->y;

    if (!hwnd) return;
    if (!(data = X11DRV_get_win_data( hwnd ))) return;
    if (!data->mapped || data->iconic || !data->managed) return;

    /* Get geometry */

    if (!event->send_event)  /* normal event, need to map coordinates to the root */
    {
        Window child;
        wine_tsx11_lock();
        XTranslateCoordinates( event->display, data->whole_window, root_window,
                               0, 0, &x, &y, &child );
        wine_tsx11_unlock();
    }
    rect.left   = x;
    rect.top    = y;
    rect.right  = x + event->width;
    rect.bottom = y + event->height;
    OffsetRect( &rect, virtual_screen_rect.left, virtual_screen_rect.top );
    TRACE( "win %p/%lx new X rect %d,%d,%dx%d (event %d,%d,%dx%d)\n",
           hwnd, data->whole_window, rect.left, rect.top, rect.right-rect.left, rect.bottom-rect.top,
           event->x, event->y, event->width, event->height );
    X11DRV_X_to_window_rect( data, &rect );

    if (is_net_wm_state_maximized( event->display, data ))
    {
        if (!IsZoomed( data->hwnd ))
        {
            TRACE( "win %p/%lx is maximized\n", data->hwnd, data->whole_window );
            SendMessageW( data->hwnd, WM_SYSCOMMAND, SC_MAXIMIZE, 0 );
            return;
        }
    }
    else
    {
        if (IsZoomed( data->hwnd ))
        {
            TRACE( "window %p/%lx is no longer maximized\n", data->hwnd, data->whole_window );
            SendMessageW( data->hwnd, WM_SYSCOMMAND, SC_RESTORE, 0 );
            return;
        }
    }

    /* Compare what has changed */

    x     = rect.left;
    y     = rect.top;
    cx    = rect.right - rect.left;
    cy    = rect.bottom - rect.top;
    flags = SWP_NOACTIVATE | SWP_NOZORDER;

    if (data->window_rect.left == x && data->window_rect.top == y) flags |= SWP_NOMOVE;
    else
        TRACE( "%p moving from (%d,%d) to (%d,%d)\n",
               hwnd, data->window_rect.left, data->window_rect.top, x, y );

    if ((data->window_rect.right - data->window_rect.left == cx &&
         data->window_rect.bottom - data->window_rect.top == cy) ||
        (IsRectEmpty( &data->window_rect ) && event->width == 1 && event->height == 1))
    {
        if (flags & SWP_NOMOVE) return;  /* if nothing changed, don't do anything */
        flags |= SWP_NOSIZE;
    }
    else
        TRACE( "%p resizing from (%dx%d) to (%dx%d)\n",
               hwnd, data->window_rect.right - data->window_rect.left,
               data->window_rect.bottom - data->window_rect.top, cx, cy );

    SetWindowPos( hwnd, 0, x, y, cx, cy, flags );
}


/***********************************************************************
 *           get_window_wm_state
 */
static int get_window_wm_state( Display *display, struct x11drv_win_data *data )
{
    struct
    {
        CARD32 state;
        XID     icon;
    } *state;
    Atom type;
    int format, ret = -1;
    unsigned long count, remaining;

    wine_tsx11_lock();
    if (!XGetWindowProperty( display, data->whole_window, x11drv_atom(WM_STATE), 0,
                             sizeof(*state)/sizeof(CARD32), False, x11drv_atom(WM_STATE),
                             &type, &format, &count, &remaining, (unsigned char **)&state ))
    {
        if (type == x11drv_atom(WM_STATE) && get_property_size( format, count ) >= sizeof(*state))
            ret = state->state;
        XFree( state );
    }
    wine_tsx11_unlock();
    return ret;
}


/***********************************************************************
 *           handle_wm_state_notify
 *
 * Handle a PropertyNotify for WM_STATE.
 */
static void handle_wm_state_notify( struct x11drv_win_data *data, XPropertyEvent *event,
                                    BOOL update_window )
{
    switch(event->state)
    {
    case PropertyDelete:
        TRACE( "%p/%lx: WM_STATE deleted from %d\n", data->hwnd, data->whole_window, data->wm_state );
        data->wm_state = WithdrawnState;
        break;
    case PropertyNewValue:
        {
            int old_state = data->wm_state;
            int new_state = get_window_wm_state( event->display, data );
            if (new_state != -1 && new_state != data->wm_state)
            {
                TRACE( "%p/%lx: new WM_STATE %d from %d\n",
                       data->hwnd, data->whole_window, new_state, old_state );
                data->wm_state = new_state;
                /* ignore the initial state transition out of withdrawn state */
                /* metacity does Withdrawn->NormalState->IconicState when mapping an iconic window */
                if (!old_state) return;
            }
        }
        break;
    }

    if (!update_window || !data->managed || !data->mapped) return;

    if (data->iconic && data->wm_state == NormalState)  /* restore window */
    {
        data->iconic = FALSE;
        if (is_net_wm_state_maximized( event->display, data ))
        {
            TRACE( "restoring to max %p/%lx\n", data->hwnd, data->whole_window );
            SendMessageW( data->hwnd, WM_SYSCOMMAND, SC_MAXIMIZE, 0 );
        }
        else
        {
            TRACE( "restoring win %p/%lx\n", data->hwnd, data->whole_window );
            SendMessageW( data->hwnd, WM_SYSCOMMAND, SC_RESTORE, 0 );
        }
    }
    else if (!data->iconic && data->wm_state == IconicState)
    {
        TRACE( "minimizing win %p/%lx\n", data->hwnd, data->whole_window );
        data->iconic = TRUE;
        SendMessageW( data->hwnd, WM_SYSCOMMAND, SC_MINIMIZE, 0 );
    }
}


/***********************************************************************
 *           X11DRV_PropertyNotify
 */
static void X11DRV_PropertyNotify( HWND hwnd, XEvent *xev )
{
    XPropertyEvent *event = &xev->xproperty;
    struct x11drv_win_data *data;

    if (!hwnd) return;
    if (!(data = X11DRV_get_win_data( hwnd ))) return;

    if (event->atom == x11drv_atom(WM_STATE)) handle_wm_state_notify( data, event, TRUE );
}


/* event filter to wait for a WM_STATE change notification on a window */
static Bool is_wm_state_notify( Display *display, XEvent *event, XPointer arg )
{
    if (event->xany.window != (Window)arg) return 0;
    return (event->type == DestroyNotify ||
            (event->type == PropertyNotify && event->xproperty.atom == x11drv_atom(WM_STATE)));
}

/***********************************************************************
 *           wait_for_withdrawn_state
 */
void wait_for_withdrawn_state( Display *display, struct x11drv_win_data *data, BOOL set )
{
    DWORD end = GetTickCount() + 2000;

    if (!data->managed) return;

    TRACE( "waiting for window %p/%lx to become %swithdrawn\n",
           data->hwnd, data->whole_window, set ? "" : "not " );

    while (data->whole_window && ((data->wm_state == WithdrawnState) == !set))
    {
        XEvent event;
        int count = 0;

        wine_tsx11_lock();
        while (XCheckIfEvent( display, &event, is_wm_state_notify, (char *)data->whole_window ))
        {
            count++;
            if (XFilterEvent( &event, None )) continue;  /* filtered, ignore it */
            if (event.type == DestroyNotify) call_event_handler( display, &event );
            else
            {
                wine_tsx11_unlock();
                handle_wm_state_notify( data, &event.xproperty, FALSE );
                wine_tsx11_lock();
            }
        }
        wine_tsx11_unlock();

        if (!count)
        {
            struct pollfd pfd;
            int timeout = end - GetTickCount();

            pfd.fd = ConnectionNumber(display);
            pfd.events = POLLIN;
            if (timeout <= 0 || poll( &pfd, 1, timeout ) != 1)
            {
                FIXME( "window %p/%lx wait timed out\n", data->hwnd, data->whole_window );
                break;
            }
        }
    }
    TRACE( "window %p/%lx state now %d\n", data->hwnd, data->whole_window, data->wm_state );
}


static HWND find_drop_window( HWND hQueryWnd, LPPOINT lpPt )
{
    RECT tempRect;

    if (!IsWindowEnabled(hQueryWnd)) return 0;
    
    GetWindowRect(hQueryWnd, &tempRect);

    if(!PtInRect(&tempRect, *lpPt)) return 0;

    if (!IsIconic( hQueryWnd ))
    {
        POINT pt = *lpPt;
        ScreenToClient( hQueryWnd, &pt );
        GetClientRect( hQueryWnd, &tempRect );

        if (PtInRect( &tempRect, pt))
        {
            HWND ret = ChildWindowFromPointEx( hQueryWnd, pt, CWP_SKIPINVISIBLE|CWP_SKIPDISABLED );
            if (ret && ret != hQueryWnd)
            {
                ret = find_drop_window( ret, lpPt );
                if (ret) return ret;
            }
        }
    }

    if(!(GetWindowLongA( hQueryWnd, GWL_EXSTYLE ) & WS_EX_ACCEPTFILES)) return 0;
    
    ScreenToClient(hQueryWnd, lpPt);

    return hQueryWnd;
}

/**********************************************************************
 *           EVENT_DropFromOffix
 *
 * don't know if it still works (last Changelog is from 96/11/04)
 */
static void EVENT_DropFromOffiX( HWND hWnd, XClientMessageEvent *event )
{
    struct x11drv_win_data *data;
    unsigned long	data_length;
    unsigned long	aux_long;
    unsigned char*	p_data = NULL;
    Atom atom_aux;
    int			x, y, dummy;
    BOOL	        bAccept;
    Window		win, w_aux_root, w_aux_child;

    win = X11DRV_get_whole_window(hWnd);
    wine_tsx11_lock();
    XQueryPointer( event->display, win, &w_aux_root, &w_aux_child,
                   &x, &y, &dummy, &dummy, (unsigned int*)&aux_long);
    x += virtual_screen_rect.left;
    y += virtual_screen_rect.top;
    wine_tsx11_unlock();

    if (!(data = X11DRV_get_win_data( hWnd ))) return;

    /* find out drop point and drop window */
    if( x < 0 || y < 0 ||
        x > (data->whole_rect.right - data->whole_rect.left) ||
        y > (data->whole_rect.bottom - data->whole_rect.top) )
    {   
	bAccept = GetWindowLongW( hWnd, GWL_EXSTYLE ) & WS_EX_ACCEPTFILES;
	x = 0;
	y = 0; 
    }
    else
    {
    	POINT	pt = { x, y };
        HWND    hwndDrop = find_drop_window( hWnd, &pt );
	if (hwndDrop)
	{
	    x = pt.x;
	    y = pt.y;
	    bAccept = TRUE;
	}
	else
	{
	    bAccept = FALSE;
	}
    }

    if (!bAccept) return;

    wine_tsx11_lock();
    XGetWindowProperty( event->display, DefaultRootWindow(event->display),
                        x11drv_atom(DndSelection), 0, 65535, FALSE,
                        AnyPropertyType, &atom_aux, &dummy,
                        &data_length, &aux_long, &p_data);
    wine_tsx11_unlock();

    if( !aux_long && p_data)  /* don't bother if > 64K */
    {
        char *p = (char *)p_data;
        char *p_drop;

        aux_long = 0;
        while( *p )  /* calculate buffer size */
        {
            INT len = GetShortPathNameA( p, NULL, 0 );
            if (len) aux_long += len + 1;
            p += strlen(p) + 1;
        }
        if( aux_long && aux_long < 65535 )
        {
            HDROP                 hDrop;
            DROPFILES *lpDrop;

            aux_long += sizeof(DROPFILES) + 1;
            hDrop = GlobalAlloc( GMEM_SHARE, aux_long );
            lpDrop = GlobalLock( hDrop );

            if( lpDrop )
            {
                lpDrop->pFiles = sizeof(DROPFILES);
                lpDrop->pt.x = x;
                lpDrop->pt.y = y;
                lpDrop->fNC = FALSE;
                lpDrop->fWide = FALSE;
                p_drop = (char *)(lpDrop + 1);
                p = (char *)p_data;
                while(*p)
                {
                    if (GetShortPathNameA( p, p_drop, aux_long - (p_drop - (char *)lpDrop) ))
                        p_drop += strlen( p_drop ) + 1;
                    p += strlen(p) + 1;
                }
                *p_drop = '\0';
                PostMessageA( hWnd, WM_DROPFILES, (WPARAM)hDrop, 0L );
            }
        }
    }
    wine_tsx11_lock();
    if( p_data ) XFree(p_data);
    wine_tsx11_unlock();
}

/**********************************************************************
 *           EVENT_DropURLs
 *
 * drop items are separated by \n
 * each item is prefixed by its mime type
 *
 * event->data.l[3], event->data.l[4] contains drop x,y position
 */
static void EVENT_DropURLs( HWND hWnd, XClientMessageEvent *event )
{
  struct x11drv_win_data *win_data;
  unsigned long	data_length;
  unsigned long	aux_long, drop_len = 0;
  unsigned char	*p_data = NULL; /* property data */
  char		*p_drop = NULL;
  char          *p, *next;
  int		x, y;
  DROPFILES *lpDrop;
  HDROP hDrop;
  union {
    Atom	atom_aux;
    int         i;
    Window      w_aux;
    unsigned int u;
  }		u; /* unused */

  if (!(GetWindowLongW( hWnd, GWL_EXSTYLE ) & WS_EX_ACCEPTFILES)) return;

  wine_tsx11_lock();
  XGetWindowProperty( event->display, DefaultRootWindow(event->display),
                      x11drv_atom(DndSelection), 0, 65535, FALSE,
                      AnyPropertyType, &u.atom_aux, &u.i,
                      &data_length, &aux_long, &p_data);
  wine_tsx11_unlock();
  if (aux_long)
    WARN("property too large, truncated!\n");
  TRACE("urls=%s\n", p_data);

  if( !aux_long && p_data) {	/* don't bother if > 64K */
    /* calculate length */
    p = (char*) p_data;
    next = strchr(p, '\n');
    while (p) {
      if (next) *next=0;
      if (strncmp(p,"file:",5) == 0 ) {
	INT len = GetShortPathNameA( p+5, NULL, 0 );
	if (len) drop_len += len + 1;
      }
      if (next) {
	*next = '\n';
	p = next + 1;
	next = strchr(p, '\n');
      } else {
	p = NULL;
      }
    }

    if( drop_len && drop_len < 65535 ) {
      wine_tsx11_lock();
      XQueryPointer( event->display, root_window, &u.w_aux, &u.w_aux,
                     &x, &y, &u.i, &u.i, &u.u);
      x += virtual_screen_rect.left;
      y += virtual_screen_rect.top;
      wine_tsx11_unlock();

      drop_len += sizeof(DROPFILES) + 1;
      hDrop = GlobalAlloc( GMEM_SHARE, drop_len );
      lpDrop = GlobalLock( hDrop );

      if( lpDrop && (win_data = X11DRV_get_win_data( hWnd )))
      {
	  lpDrop->pFiles = sizeof(DROPFILES);
	  lpDrop->pt.x = x;
	  lpDrop->pt.y = y;
	  lpDrop->fNC =
	    ( x < (win_data->client_rect.left - win_data->whole_rect.left)  ||
	      y < (win_data->client_rect.top - win_data->whole_rect.top)    ||
	      x > (win_data->client_rect.right - win_data->whole_rect.left) ||
	      y > (win_data->client_rect.bottom - win_data->whole_rect.top) );
	  lpDrop->fWide = FALSE;
	  p_drop = (char*)(lpDrop + 1);
      }

      /* create message content */
      if (p_drop) {
	p = (char*) p_data;
	next = strchr(p, '\n');
	while (p) {
	  if (next) *next=0;
	  if (strncmp(p,"file:",5) == 0 ) {
	    INT len = GetShortPathNameA( p+5, p_drop, 65535 );
	    if (len) {
	      TRACE("drop file %s as %s\n", p+5, p_drop);
	      p_drop += len+1;
	    } else {
	      WARN("can't convert file %s to dos name\n", p+5);
	    }
	  } else {
	    WARN("unknown mime type %s\n", p);
	  }
	  if (next) {
	    *next = '\n';
	    p = next + 1;
	    next = strchr(p, '\n');
	  } else {
	    p = NULL;
	  }
	  *p_drop = '\0';
	}

        GlobalUnlock(hDrop);
        PostMessageA( hWnd, WM_DROPFILES, (WPARAM)hDrop, 0L );
      }
    }
    wine_tsx11_lock();
    if( p_data ) XFree(p_data);
    wine_tsx11_unlock();
  }
}

/**********************************************************************
 *              handle_dnd_protocol
 */
static void handle_dnd_protocol( HWND hwnd, XClientMessageEvent *event )
{
    Window root, child;
    int root_x, root_y, child_x, child_y;
    unsigned int u;

    /* query window (drag&drop event contains only drag window) */
    wine_tsx11_lock();
    XQueryPointer( event->display, root_window, &root, &child,
                   &root_x, &root_y, &child_x, &child_y, &u);
    if (XFindContext( event->display, child, winContext, (char **)&hwnd ) != 0) hwnd = 0;
    wine_tsx11_unlock();
    if (!hwnd) return;
    if (event->data.l[0] == DndFile || event->data.l[0] == DndFiles)
        EVENT_DropFromOffiX(hwnd, event);
    else if (event->data.l[0] == DndURL)
        EVENT_DropURLs(hwnd, event);
}


struct client_message_handler
{
    int    atom;                                  /* protocol atom */
    void (*handler)(HWND, XClientMessageEvent *); /* corresponding handler function */
};

static const struct client_message_handler client_messages[] =
{
    { XATOM_WM_PROTOCOLS, handle_wm_protocols },
    { XATOM_DndProtocol,  handle_dnd_protocol },
    { XATOM_XdndEnter,    X11DRV_XDND_EnterEvent },
    { XATOM_XdndPosition, X11DRV_XDND_PositionEvent },
    { XATOM_XdndDrop,     X11DRV_XDND_DropEvent },
    { XATOM_XdndLeave,    X11DRV_XDND_LeaveEvent }
};


/**********************************************************************
 *           X11DRV_ClientMessage
 */
static void X11DRV_ClientMessage( HWND hwnd, XEvent *xev )
{
    XClientMessageEvent *event = &xev->xclient;
    unsigned int i;

    if (!hwnd) return;

    if (event->format != 32)
    {
        WARN( "Don't know how to handle format %d\n", event->format );
        return;
    }

    for (i = 0; i < sizeof(client_messages)/sizeof(client_messages[0]); i++)
    {
        if (event->message_type == X11DRV_Atoms[client_messages[i].atom - FIRST_XATOM])
        {
            client_messages[i].handler( hwnd, event );
            return;
        }
    }
    TRACE( "no handler found for %ld\n", event->message_type );
}


/***********************************************************************
 *		X11DRV_SendInput  (X11DRV.@)
 */
UINT CDECL X11DRV_SendInput( UINT count, LPINPUT inputs, int size )
{
    UINT i;

    for (i = 0; i < count; i++, inputs++)
    {
        switch(inputs->type)
        {
        case INPUT_MOUSE:
            X11DRV_send_mouse_input( 0, inputs->u.mi.dwFlags, inputs->u.mi.dx, inputs->u.mi.dy,
                                     inputs->u.mi.mouseData, inputs->u.mi.time,
                                     inputs->u.mi.dwExtraInfo, LLMHF_INJECTED );
            break;
        case INPUT_KEYBOARD:
            X11DRV_send_keyboard_input( inputs->u.ki.wVk, inputs->u.ki.wScan, inputs->u.ki.dwFlags,
                                        inputs->u.ki.time, inputs->u.ki.dwExtraInfo, LLKHF_INJECTED );
            break;
        case INPUT_HARDWARE:
            FIXME( "INPUT_HARDWARE not supported\n" );
            break;
        }
    }
    return count;
}
