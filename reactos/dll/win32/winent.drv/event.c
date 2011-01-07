/*
 * PROJECT:         ReactOS
 * LICENSE:         GNU LGPL by FSF v2.1 or any later
 * FILE:            dll/win32/winent.drv/event.c
 * PURPOSE:         Event handling routines
 * PROGRAMMERS:     Aleksey Bragin (aleksey@reactos.org)
 *                  Some parts inspired by Wine's winex11.drv
 */

/* INCLUDES ***************************************************************/

#include "winent.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(event);
#if 0
/* GLOBALS ****************************************************************/

static void NTDRV_Expose( HWND hwnd, GR_EVENT *event );

struct event_handler
{
    GR_EVENT_TYPE       type;    /* event type */
    ntdrv_event_handler handler; /* corresponding handler function */
};

#define MAX_EVENT_HANDLERS 64

static struct event_handler handlers[MAX_EVENT_HANDLERS] =
{
    /* list must be sorted by event type */
    //{ KeyPress,         X11DRV_KeyEvent },
    //{ KeyRelease,       X11DRV_KeyEvent },
    //{ ButtonPress,      X11DRV_ButtonPress },
    //{ ButtonRelease,    X11DRV_ButtonRelease },
    //{ MotionNotify,     X11DRV_MotionNotify },
    //{ EnterNotify,      X11DRV_EnterNotify },
    /* LeaveNotify */
    //{ FocusIn,          X11DRV_FocusIn },
    //{ FocusOut,         X11DRV_FocusOut },
    //{ KeymapNotify,     X11DRV_KeymapNotify },
    { GR_EVENT_TYPE_EXPOSURE, NTDRV_Expose },
    /* GraphicsExpose */
    /* NoExpose */
    /* VisibilityNotify */
    /* CreateNotify */
    //{ DestroyNotify,    X11DRV_DestroyNotify },
    /* UnmapNotify */
    //{ MapNotify,        X11DRV_MapNotify },
    /* MapRequest */
    //{ ReparentNotify,   X11DRV_ReparentNotify },
    //{ ConfigureNotify,  X11DRV_ConfigureNotify },
    /* ConfigureRequest */
    //{ GravityNotify,    X11DRV_GravityNotify },
    /* ResizeRequest */
    /* CirculateNotify */
    /* CirculateRequest */
    //{ PropertyNotify,   X11DRV_PropertyNotify },
    //{ SelectionClear,   X11DRV_SelectionClear },
    //{ SelectionRequest, X11DRV_SelectionRequest },
    /* SelectionNotify */
    /* ColormapNotify */
    //{ ClientMessage,    X11DRV_ClientMessage },
    //{ MappingNotify,    X11DRV_MappingNotify },
};

static int nb_event_handlers = 1;  /* change this if you add handlers above */

/* FUNCTIONS **************************************************************/

/***********************************************************************
 *           find_handler
 *
 * Find the handler for a given event type. Caller must hold the x11 lock.
 */
static inline ntdrv_event_handler find_handler( GR_EVENT_TYPE type )
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
#if 0
static enum event_merge_action merge_events( GR_EVENT *prev, GR_EVENT *next )
{
    switch (prev->type)
    {
    case GR_UPDATE_MOVE:
        switch (next->type)
        {
        case GR_UPDATE_MOVE:
            if (prev->wid == next->wid)
            {
                TRACE( "discarding duplicate ConfigureNotify for window %lx\n", prev->wid );
                return MERGE_DISCARD;
            }
            break;
        case GR_EVENT_MASK_EXPOSURE:
        //case PropertyNotify:
            return MERGE_KEEP;
        }
        break;
    case GR_EVENT_MASK_MOUSE_MOTION:
        if (prev->wid == next->wid && next->type == MotionNotify)
        {
            TRACE( "discarding duplicate MotionNotify for window %lx\n", prev->wid );
            return MERGE_DISCARD;
        }xch
        break;
    }

    return MERGE_HANDLE;
}
#endif


/***********************************************************************
 *           call_event_handler
 */
static inline void call_event_handler( GR_EVENT *event )
{
    HWND hwnd;
    ntdrv_event_handler handler;
    //GR_EVENT *prev;
    //struct ntdrv_thread_data *thread_data;

    if (!(handler = find_handler( event->type )))
    {
        TRACE( "%s for win %lx, ignoring\n", "event", -1 );
        return;  /* no handler, ignore it */
    }

    hwnd = GrGetHwnd( event->general.wid );
    //if (!hwnd && event->xany.window == root_window) hwnd = GetDesktopWindow();

    //TRACE( "%lu %s for hwnd/window %p/%lx\n",
    //       event->xany.serial, dbgstr_event( event->type ), hwnd, event->xany.window );
    //wine_tsx11_unlock();
    //thread_data = ntdrv_thread_data();
    //prev = thread_data->current_event;
    //thread_data->current_event = event;
    handler( hwnd, event );
    //thread_data->current_event = prev;
    //wine_tsx11_lock();
}

/***********************************************************************
 *           process_events
 */
static int process_events( int mask )
{
    GR_EVENT event;//, prev_event;
    int count = 0;
    //enum event_merge_action action = MERGE_DISCARD;

    //prev_event.type = 0;
    //wine_tsx11_lock();
    while (GrPeekEvent(&event))
    {
        GrGetNextEvent(&event);
        ERR("Got event type %d\n", event.type);
        count++;
#if 0
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
#else
        call_event_handler( &event );
#endif
    }
    //if (prev_event.type) call_event_handler( &prev_event );
    //XFlush( gdi_display );
    //wine_tsx11_unlock();
    if (count) TRACE( "processed %d events\n", count );
    return count;
}
#endif

/***********************************************************************
 *           MsgWaitForMultipleObjectsEx   (NTDRV.@)
 */
DWORD CDECL RosDrv_MsgWaitForMultipleObjectsEx( DWORD count, const HANDLE *handles, DWORD timeout,
                                                DWORD mask, DWORD flags )
{
    //TRACE("WaitForMultipleObjectsEx(%d %p %d %x %x %x\n", count, handles, timeout, mask, flags);

    if (!count && !timeout) return WAIT_TIMEOUT;
    return WaitForMultipleObjectsEx( count, handles, flags & MWMO_WAITALL,
                                     timeout, flags & MWMO_ALERTABLE );
}

#if 0
/***********************************************************************
 *           X11DRV_Expose
 */
static void NTDRV_Expose( HWND hwnd, GR_EVENT *xev )
{
    GR_EVENT_EXPOSURE *event = &xev->exposure;
    RECT rect;
    struct ntdrv_win_data *data;
    int flags = RDW_INVALIDATE | RDW_ERASE;

    ERR( "win %p (%lx) %d,%d %dx%d\n",
           hwnd, event->wid, event->x, event->y, event->width, event->height );

    if (!(data = NTDRV_get_win_data( hwnd ))) return;

    rect.left   = event->x;
    rect.top    = event->y;
    rect.right  = event->x + event->width;
    rect.bottom = event->y + event->height;
    if (event->wid == data->whole_window)
    {
        OffsetRect( &rect, data->whole_rect.left - data->client_rect.left,
                    data->whole_rect.top - data->client_rect.top );
        flags |= RDW_FRAME;
    }

    //if (event->wid != root_window)
    {
        if (GetWindowLongW( data->hwnd, GWL_EXSTYLE ) & WS_EX_LAYOUTRTL)
            mirror_rect( &data->client_rect, &rect );

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

        flags |= RDW_ALLCHILDREN;
    }
    //else OffsetRect( &rect, virtual_screen_rect.left, virtual_screen_rect.top );

    RedrawWindow( hwnd, &rect, 0, flags );
}
#endif
/* EOF */
