/*
 * PROJECT:         ReactOS
 * LICENSE:         GNU LGPL by FSF v2.1 or any later
 * FILE:            dll/win32/winent.drv/window.c
 * PURPOSE:         Windows related functions
 * PROGRAMMERS:     Aleksey Bragin (aleksey@reactos.org)
 *                  Some code is taken from winex11.drv (c) Wine project
 */

/* INCLUDES ***************************************************************/

#include "winent.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(rosuserdrv);

static struct list wnd_data_list = LIST_INIT( wnd_data_list );
static CRITICAL_SECTION wnd_data_cs;
static CRITICAL_SECTION_DEBUG critsect_debug =
{
    0, 0, &wnd_data_cs,
    { &critsect_debug.ProcessLocksList, &critsect_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": wnd_data_cs") }
};
static CRITICAL_SECTION wnd_data_cs = { &critsect_debug, -1, 0, 0, 0, 0 };

static const char whole_window_prop[] = "__arwin_nt_whole_window";
static const char client_window_prop[]= "__arwin_nt_client_window";

SWM_WINDOW_ID root_window = SWM_ROOT_WINDOW_ID;

/* FUNCTIONS **************************************************************/

VOID CDECL RosDrv_UpdateZOrder(HWND hwnd, RECT *rect)
{
        FIXME("hwnd %x rect (%d, %d)-(%d,%d)\n", hwnd,
            rect->top, rect->left, rect->bottom, rect->right);

        SERVER_START_REQ( update_window_zorder )
        {
            req->window      = wine_server_user_handle( hwnd );
            req->rect.left   = rect->left;
            req->rect.top    = rect->top;
            req->rect.right  = rect->right;
            req->rect.bottom = rect->bottom;
            wine_server_call( req );
        }
        SERVER_END_REQ;
}

struct ntdrv_win_data *associate_create( HWND hwnd )
{
    struct ntdrv_win_data *data;

    /* Insert our mapping */
    data = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(struct ntdrv_win_data));
    if(!data) return NULL;

    data->hwnd = hwnd;

    EnterCriticalSection(&wnd_data_cs);
    list_add_tail(&wnd_data_list, &data->entry);
    LeaveCriticalSection(&wnd_data_cs);

    return data;
}

VOID associate_destroy( struct ntdrv_win_data *data )
{
    EnterCriticalSection(&wnd_data_cs);
    list_remove( &data->entry );
    LeaveCriticalSection(&wnd_data_cs);

    HeapFree( GetProcessHeap(), 0, data);
}

struct ntdrv_win_data *associate_find( HWND hwnd )
{
    struct ntdrv_win_data *item;
    BOOL found = FALSE;

    EnterCriticalSection(&wnd_data_cs);

    LIST_FOR_EACH_ENTRY( item, &wnd_data_list, struct ntdrv_win_data, entry )
        if (item->hwnd == hwnd)
        {
            found = TRUE;
            break;
        }

    LeaveCriticalSection(&wnd_data_cs);

    if (!found) item = NULL;

    return item;
}

/**********************************************************************
 *		CreateDesktopWindow   (NTDRV.@)
 */
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
    else
    {
        //SWM_WINDOW_ID win = (SWM_WINDOW_ID)GetPropA( hwnd, whole_window_prop );
        //ERR("win %p, w %d h %d, hwnd %x\n", win, width, height, hwnd);

        //if (win && win != root_window) NTDRV_init_desktop( win, width, height );

        // create a desktop window
    }

    return TRUE;
}

/**********************************************************************
 *		CreateWindow   (NTDRV.@)
 */
BOOL CDECL RosDrv_CreateWindow( HWND hwnd )
{
    WARN("RosDrv_CreateWindow(%x)\n", hwnd);

    if (hwnd == GetDesktopWindow() /*&& root_window != SWM_ROOT_WINDOW_ID*/)
    {
        /* create desktop win data */
        if (!NTDRV_create_desktop_win_data( hwnd )) return FALSE;
    }

    return TRUE;
}

/***********************************************************************
 *		NTDRV_get_win_data
 *
 * Return the private data structure associated with a window.
 */
struct ntdrv_win_data *NTDRV_get_win_data( HWND hwnd )
{
    struct ntdrv_win_data *data;

    if (!hwnd) return NULL;

    data = associate_find( hwnd );

    return data;
}


/***********************************************************************
 *		NTDRV_create_win_data
 *
 * Create a private data window structure for an existing window.
 */
struct ntdrv_win_data *NTDRV_create_win_data( HWND hwnd )
{
    struct ntdrv_win_data *data;
    HWND parent;

    if (!(parent = GetAncestor( hwnd, GA_PARENT ))) return NULL;  /* desktop */

    /* don't create win data for HWND_MESSAGE windows */
    if (parent != GetDesktopWindow() && !GetAncestor( parent, GA_PARENT )) return NULL;

    data = associate_create(hwnd);
    if (!data) return NULL;

    GetWindowRect( hwnd, &data->window_rect );
    MapWindowPoints( 0, parent, (POINT *)&data->window_rect, 2 );
    data->whole_rect = data->window_rect;
    GetClientRect( hwnd, &data->client_rect );
    MapWindowPoints( hwnd, parent, (POINT *)&data->client_rect, 2 );

    if (parent == GetDesktopWindow())
    {
        if (!create_whole_window( data ))
        {
            HeapFree( GetProcessHeap(), 0, data );
            return NULL;
        }
        TRACE( "win %p/%lx/%lx window %s whole %s client %s\n",
               hwnd, data->whole_window, data->client_window, wine_dbgstr_rect( &data->window_rect ),
               wine_dbgstr_rect( &data->whole_rect ), wine_dbgstr_rect( &data->client_rect ));
    }

    return data;
}

/* initialize the desktop window id in the desktop manager process */
struct ntdrv_win_data *NTDRV_create_desktop_win_data( HWND hwnd )
{
    struct ntdrv_win_data *data;

    data = associate_create(hwnd);
    if (!data) return NULL;

    /* Mark it as being a whole window */
    data->whole_window = data->client_window = root_window;
    SetPropA( data->hwnd, whole_window_prop, (HANDLE)root_window );
    SetPropA( data->hwnd, client_window_prop, (HANDLE)root_window );
    //SetPropA( data->hwnd, "__wine_x11_managed", (HANDLE)1 );

    return data;
}

/**********************************************************************
 *		destroy_whole_window
 *
 * Destroy the whole WM window for a given window.
 */
static void destroy_whole_window( struct ntdrv_win_data *data, BOOL already_destroyed )
{
    if (!data->whole_window) return;

    TRACE( "win %p xwin %lx/%lx\n", data->hwnd, data->whole_window, data->client_window );
    if (!already_destroyed) SwmDestroyWindow( data->whole_window );
    data->whole_window = data->client_window = 0;
    //data->wm_state = WithdrawnState;
    //data->net_wm_state = 0;
    data->mapped = FALSE;
    /*if (data->xic)
    {
        XUnsetICFocus( data->xic );
        XDestroyIC( data->xic );
        data->xic = 0;
    }*/
    /* Outlook stops processing messages after destroying a dialog, so we need an explicit flush */
    //XFlush( display );
    //XFree( data->wm_hints );
    //data->wm_hints = NULL;
    RemovePropA( data->hwnd, whole_window_prop );
    RemovePropA( data->hwnd, client_window_prop );
}

/***********************************************************************
 *		NTDRV_destroy_win_data
 *
 * Deletes a private data window structure for an existing window.
 */
void NTDRV_destroy_win_data( HWND hwnd )
{
    struct ntdrv_win_data *data = NTDRV_get_win_data(hwnd);
    if (!data) return;

    /* Remove property and free its data */
    destroy_whole_window( data, FALSE );
    associate_destroy( data );
}

/***********************************************************************
 *     map_window
 */
void map_window( struct ntdrv_win_data *data, DWORD new_style )
{
    TRACE( "win %p/%lx\n", data->hwnd, data->whole_window );

    //sync_window_style( display, data );
    SwmShowWindow( data->whole_window, TRUE, 0 );

    data->mapped = TRUE;
    data->iconic = (new_style & WS_MINIMIZE) != 0;
}


/***********************************************************************
 *     unmap_window
 */
void unmap_window( struct ntdrv_win_data *data )
{
    TRACE( "win %p/%lx\n", data->hwnd, data->whole_window );

    SwmShowWindow( data->whole_window, FALSE, 0 );

    data->mapped = FALSE;
    //data->net_wm_state = 0;
}

/***********************************************************************
 *		sync_window_position
 *
 * Synchronize the SWM window position with the Windows one
 */
void sync_window_position( struct ntdrv_win_data *data,
                           UINT swp_flags, const RECT *old_window_rect,
                           const RECT *old_whole_rect, const RECT *old_client_rect )
{
    //DWORD style = GetWindowLongW( data->hwnd, GWL_STYLE );
    LONG width, height;
    //LONG x, y;

    SwmPosChanged(data->whole_window, &data->whole_rect, old_whole_rect, NULL, swp_flags);

    width = data->whole_rect.right - data->whole_rect.left;
    height = data->whole_rect.bottom - data->whole_rect.top;
    /* if window rect is empty force size to 1x1 */
    if (width <= 0 || height <= 0) width = height = 1;
    if (width > 65535) width = 65535;
    if (height > 65535) height = 65535;

    //GrResizeWindow(data->whole_window, width, height);

    /* only the size is allowed to change for the desktop window */
    /*if (data->whole_window != root_window)
    {
        x = data->whole_rect.left;
        y = data->whole_rect.top;
        GrMoveWindow(data->whole_window, x, y);
    }*/

    if (!(swp_flags & SWP_NOZORDER) || (swp_flags & SWP_SHOWWINDOW))
    {
        /* find window that this one must be after */
        HWND prev = GetWindow( data->hwnd, GW_HWNDPREV );
        while (prev && !(GetWindowLongW( prev, GWL_STYLE ) & WS_VISIBLE))
            prev = GetWindow( prev, GW_HWNDPREV );
        if (!prev)  /* top child */
        {
            //GR_WINDOW_INFO infoptr;
            //GrGetWindowInfo(data->whole_window, &infoptr);

            //GrRaiseWindow(data->whole_window);
            //changes.stack_mode = Above;
            //mask |= CWStackMode;
            SwmSetForeground(data->whole_window);
        }
        /* should use stack_mode Below but most window managers don't get it right */
        /* and Above with a sibling doesn't work so well either, so we ignore it */
    }


    TRACE( "win %p/%lx pos %d,%d,%dx%d\n",
           data->hwnd, data->whole_window, data->whole_rect.left, data->whole_rect.top,
           data->whole_rect.right - data->whole_rect.left,
           data->whole_rect.bottom - data->whole_rect.top );
}

/***********************************************************************
 *		sync_client_position
 *
 * Synchronize the X client window position with the Windows one
 */
static void sync_client_position( struct ntdrv_win_data *data,
                                  UINT swp_flags, const RECT *old_client_rect,
                                  const RECT *old_whole_rect )
{
    LONG width, height;
    //BOOL resize = FALSE;
    RECT old = *old_client_rect;
    RECT new = data->client_rect;

    OffsetRect( &old, -old_whole_rect->left, -old_whole_rect->top );
    OffsetRect( &new, -data->whole_rect.left, -data->whole_rect.top );

    width = old.right - old.left;
    height = old.bottom - old.top;
    if (old.right - old.left != new.right - new.left)
    {
        if ((width = new.right - new.left) <= 0) width = 1;
        else if (width > 65535) width = 65535;
        //resize = TRUE;
    }
    if (old.bottom - old.top != new.bottom - new.top)
    {
        if ((height = new.bottom - new.top) <= 0) height = 1;
        else if (height > 65535) height = 65535;
        //resize = TRUE;
    }

    TRACE( "setting client win %lx pos %d,%d,%dx%d\n",
           data->client_window, new.left, new.top,
           new.right - new.left, new.bottom - new.top );

    //if (resize) GrResizeWindow( data->client_window, width, height );

    if ((old.left != new.left) || (old.top != new.top))
    {
        //GrMoveWindow(data->client_window, new.left, new.top);
    }
}

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

/***********************************************************************
 *		is_window_rect_mapped
 *
 * Check if the SWM whole window should be mapped based on its rectangle
 */
BOOL is_window_rect_mapped( const RECT *rect )
{
    /* don't map if rect is off-screen */
    /*if (rect->left >= virtual_screen_rect.right ||
        rect->top >= virtual_screen_rect.bottom ||
        rect->right <= virtual_screen_rect.left ||
        rect->bottom <= virtual_screen_rect.top)
        return FALSE;*/

    return TRUE;
}

/***********************************************************************
 *              create_client_window
 */
static SWM_WINDOW_ID create_client_window( struct ntdrv_win_data *data )
{
    int cx, cy;
    SWM_WINDOW_ID client;
    RECT winRect;

    if ((cx = data->client_rect.right - data->client_rect.left) <= 0) cx = 1;
    else if (cx > 65535) cx = 65535;
    if ((cy = data->client_rect.bottom - data->client_rect.top) <= 0) cy = 1;
    else if (cy > 65535) cy = 65535;

    winRect.left = data->client_rect.left - data->whole_rect.left;
    winRect.top = data->client_rect.top - data->whole_rect.top;
    winRect.right = winRect.left + cx;
    winRect.bottom = winRect.top + cy;

    client = data->whole_window;//SwmNewWindow( data->whole_window, &winRect, data->hwnd, 0 );
    if (!client)
    {
        return 0;
    }

    if (data->client_window)
    {
        //SwmDestroyWindow( data->client_window );
    }
    data->client_window = client;

    //SwmShowWindow( data->client_window, TRUE, 0 );

    SetPropA( data->hwnd, client_window_prop, (HANDLE)data->client_window );
    return data->client_window;
}

/**********************************************************************
 *		create_whole_window
 *
 * Create the whole X window for a given window
 */
SWM_WINDOW_ID create_whole_window( struct ntdrv_win_data *data )
{
    int cx, cy;
    //int mask;
    //WCHAR text[1024];
    //COLORREF key;
    //BYTE alpha;
    //DWORD layered_flags;
    HRGN win_rgn;
    //GR_WM_PROPERTIES props;
    DWORD ex_style;

    if ((win_rgn = CreateRectRgn( 0, 0, 0, 0 )) &&
        GetWindowRgn( data->hwnd, win_rgn ) == ERROR)
    {
        DeleteObject( win_rgn );
        win_rgn = 0;
    }
    data->shaped = (win_rgn != 0);

    //mask = get_window_attributes( display, data, &attr );

    data->whole_rect = data->window_rect;
    //X11DRV_window_to_X_rect( data, &data->whole_rect );
    if (!(cx = data->whole_rect.right - data->whole_rect.left)) cx = 1;
    else if (cx > 65535) cx = 65535;
    if (!(cy = data->whole_rect.bottom - data->whole_rect.top)) cy = 1;
    else if (cy > 65535) cy = 65535;

    ex_style = GetWindowLongW( data->hwnd, GWL_EXSTYLE );

    data->whole_window = SwmNewWindow( root_window,
                                       &data->whole_rect, data->hwnd, ex_style );

    if (!data->whole_window) goto done;

    if (!create_client_window( data ))
    {
        SwmDestroyWindow( data->whole_window );
        data->whole_window = 0;
        goto done;
    }

    //set_initial_wm_hints( display, data );
    //set_wm_hints( display, data );

    SetPropA( data->hwnd, whole_window_prop, (HANDLE)data->whole_window );
    //SetPropA( data->hwnd, "__wine_x11_managed", (HANDLE)1 );

    /* set the window text */
    //if (!InternalGetWindowText( data->hwnd, text, sizeof(text)/sizeof(WCHAR) )) text[0] = 0;
    //sync_window_text( display, data->whole_window, text );

    /* set the window region */
    //if (win_rgn || IsRectEmpty( &data->window_rect )) sync_window_region( display, data, win_rgn );

    /* set the window opacity */
    //if (!GetLayeredWindowAttributes( data->hwnd, &key, &alpha, &layered_flags )) layered_flags = 0;
    //sync_window_opacity( display, data->whole_window, key, alpha, layered_flags );

    //sync_window_cursor( data );
done:
    if (win_rgn) DeleteObject( win_rgn );
    return data->whole_window;
}

/***********************************************************************
 *		NTDRV_get_whole_window
 *
 * Return the X window associated with the full area of a window
 */
SWM_WINDOW_ID NTDRV_get_whole_window( HWND hwnd )
{
    struct ntdrv_win_data *data = NTDRV_get_win_data( hwnd );

    if (!data)
    {
        if (hwnd == GetDesktopWindow()) return root_window;
        return (SWM_WINDOW_ID)GetPropA( hwnd, whole_window_prop );
    }
    return data->whole_window;
}

/***********************************************************************
 *		NTDRV_get_client_window
 *
 * Return the SWM window associated with the client area of a window
 */
static SWM_WINDOW_ID NTDRV_get_client_window( HWND hwnd )
{
    struct ntdrv_win_data *data = NTDRV_get_win_data( hwnd );

    if (!data)
    {
        if (hwnd == GetDesktopWindow()) return root_window;
        return (SWM_WINDOW_ID)GetPropA( hwnd, client_window_prop );
    }
    return data->client_window;
}

/***********************************************************************
 *		NTDRV_GetDC   (NTDRV.@)
 */
void CDECL RosDrv_GetDC( HDC hdc, HWND hwnd, HWND top, const RECT *win_rect,
                                 const RECT *top_rect, DWORD flags )
{
    struct ntdrv_escape_set_drawable escape;
    struct ntdrv_win_data *data = NTDRV_get_win_data( hwnd );
    HWND parent;

    escape.code        = NTDRV_SET_DRAWABLE;
    escape.clip_children = FALSE;
    escape.gl_copy     = FALSE;
    escape.hwnd        = 0;
    escape.release     = FALSE;

    escape.dc_rect.left         = win_rect->left - top_rect->left;
    escape.dc_rect.top          = win_rect->top - top_rect->top;
    escape.dc_rect.right        = win_rect->right - top_rect->left;
    escape.dc_rect.bottom       = win_rect->bottom - top_rect->top;
    escape.drawable_rect.left   = top_rect->left;
    escape.drawable_rect.top    = top_rect->top;
    escape.drawable_rect.right  = top_rect->right;
    escape.drawable_rect.bottom = top_rect->bottom;

    if (top == hwnd)
    {
        if (flags & DCX_WINDOW)
        {
            escape.drawable = data ? data->whole_window : NTDRV_get_whole_window( hwnd );
        }
        else
            escape.drawable = data ? data->client_window : NTDRV_get_client_window( hwnd );
    }
    else
    {
        for (parent = hwnd; parent && parent != top; parent = GetAncestor( parent, GA_PARENT ))
            if ((escape.drawable = NTDRV_get_client_window( parent ))) break;

        if (escape.drawable)
        {
            POINT pt = { 0, 0 };
            MapWindowPoints( top, parent, &pt, 1 );
            OffsetRect( &escape.dc_rect, pt.x, pt.y );
            OffsetRect( &escape.drawable_rect, -pt.x, -pt.y );

            FIXME("Offset by (%d, %d)\n", pt.x, pt.y);
        }
        else escape.drawable = NTDRV_get_client_window( top );

        if (flags & DCX_CLIPCHILDREN) escape.clip_children = TRUE;
    }

    TRACE("hdc %x, hwnd %x, top %x\n win_rect %s, top_rect %s, clipchildren %d\n", hdc, hwnd, top,
        wine_dbgstr_rect(win_rect), wine_dbgstr_rect(top_rect), escape.clip_children);

    ExtEscape( hdc, NTDRV_ESCAPE, sizeof(escape), (LPSTR)&escape, 0, NULL );
}

void CDECL RosDrv_ReleaseDC( HWND hwnd, HDC hdc )
{
    struct ntdrv_escape_set_drawable escape;
    RECT virtual_screen_rect;

    escape.code        = NTDRV_SET_DRAWABLE;
    escape.drawable    = root_window;
    escape.clip_children = FALSE;
    escape.gl_copy     = FALSE;
    escape.release     = TRUE;
    escape.hwnd = hwnd;

    virtual_screen_rect.right  = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    virtual_screen_rect.bottom = GetSystemMetrics(SM_CYVIRTUALSCREEN);
    virtual_screen_rect.left = 0; virtual_screen_rect.top = 0; 

    escape.drawable_rect = virtual_screen_rect;

    SetRect( &escape.dc_rect, 0, 0, virtual_screen_rect.right - virtual_screen_rect.left,
             virtual_screen_rect.bottom - virtual_screen_rect.top );
    OffsetRect( &escape.dc_rect, -escape.drawable_rect.left, -escape.drawable_rect.top );

    ExtEscape( hdc, NTDRV_ESCAPE, sizeof(escape), (LPSTR)&escape, 0, NULL );
}

/***********************************************************************
 *		SetCapture  (NTDRV.@)
 */
void CDECL RosDrv_SetCapture( HWND hwnd, UINT flags )
{
    if (!(flags & (GUI_INMOVESIZE | GUI_INMENUMODE))) return;

    if (hwnd)
    {
        /* Capturing */
        TRACE("Capture set for hwnd %x\n", hwnd);
    }
    else
    {
        TRACE("Capture released\n");
    }
}


/*****************************************************************
 *		SetParent   (NTDRV.@)
 */
void CDECL RosDrv_SetParent( HWND hwnd, HWND parent, HWND old_parent )
{
    struct ntdrv_win_data *data = NTDRV_get_win_data( hwnd );

    if (!data) return;
    if (parent == old_parent) return;

    if (parent != GetDesktopWindow()) /* a child window */
    {
        if (old_parent == GetDesktopWindow())
        {
            /* destroy the old windows */
            //destroy_whole_window( data, FALSE );
            //destroy_icon_window( data );
            FIXME("Should destroy hwnd %x\n", data->hwnd);
        }
    }
    else  /* new top level window */
    {
        /* FIXME: we ignore errors since we can't really recover anyway */
        //create_whole_window( data );
        FIXME("Should create a new whole window for hwnd %x\n", data->hwnd);
    }
}

/*****************************************************************
 *		SetFocus   (NTDRV.@)
 *
 * Set the Nano focus.
 */
void CDECL RosDrv_SetFocus( HWND hwnd )
{
    struct ntdrv_win_data *data;

    if (!(hwnd = GetAncestor( hwnd, GA_ROOT ))) return;
    if (!(data = NTDRV_get_win_data( hwnd ))) return;
    if (!data->whole_window) return;

    TRACE("SetFocus %x, desk %x\n", hwnd, GetDesktopWindow());

    /* Bring this window to foreground */
    SwmSetForeground(data->whole_window);
}

/***********************************************************************
 *		WindowPosChanging   (NTDRV.@)
 */
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

    //TRACE( "win %x pos is changing. vis rect %s, win rect %s\n",
    //       hwnd, wine_dbgstr_rect(visible_rect), wine_dbgstr_rect(window_rect) );

    *visible_rect = *window_rect;
}

void CDECL RosDrv_WindowPosChanged( HWND hwnd, HWND insert_after, UINT swp_flags,
                                    const RECT *window_rect, const RECT *rectClient,
                                    const RECT *visible_rect, const RECT *valid_rects )
{
    RECT old_whole_rect, old_client_rect, old_window_rect;
    struct ntdrv_win_data *data = NTDRV_get_win_data(hwnd);
    DWORD new_style = GetWindowLongW( hwnd, GWL_STYLE );

    if (!data) return;

    TRACE( "win %x pos changed. new vis rect %s, old whole rect %s, swp_flags %x insert_after %x\n",
           hwnd, wine_dbgstr_rect(visible_rect), wine_dbgstr_rect(&data->whole_rect), swp_flags, insert_after );

    old_window_rect = data->window_rect;
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

    sync_client_position( data, swp_flags, &old_client_rect, &old_whole_rect );

    if (!data->whole_window) return;

    /* Sync window position with the SWM */
    sync_window_position( data, swp_flags,
                          &old_window_rect, &old_whole_rect, &old_client_rect );

    if (data->mapped)
    {
        if (((swp_flags & SWP_HIDEWINDOW) && !(new_style & WS_VISIBLE)) ||
            (!is_window_rect_mapped( window_rect ) && is_window_rect_mapped( &old_window_rect )))
            unmap_window( data );
    }

    /* Pass show/hide information to the window manager */
    if ((new_style & WS_VISIBLE) &&
        ((new_style & WS_MINIMIZE) || is_window_rect_mapped( window_rect )))
    {
        if (!data->mapped)
        {
            map_window( data, new_style );
        }
        else if ((swp_flags & SWP_STATECHANGED) && (!data->iconic != !(new_style & WS_MINIMIZE)))
        {
            data->iconic = (new_style & WS_MINIMIZE) != 0;
            TRACE( "changing win %p iconic state to %u\n", data->hwnd, data->iconic );
        }
    }
}

/* EOF */
