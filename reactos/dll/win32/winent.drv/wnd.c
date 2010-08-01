/*
 * PROJECT:         ReactOS
 * LICENSE:         LGPL
 * FILE:            dll/win32/winent.drv/wnd.c
 * PURPOSE:         Windows related functions
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
#include "winent.h"
#include "wine/server.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(rosuserdrv);

static const char window_data_prop[] = "__ros_nt_window_data";

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

/***********************************************************************
 *		NTDRV_get_win_data
 *
 * Return the private data structure associated with a window.
 */
struct ntdrv_win_data *NTDRV_get_win_data( HWND hwnd )
{
    struct ntdrv_win_data *data;

    if (!hwnd) return NULL;

    data = (struct ntdrv_win_data *)GetPropA( hwnd, window_data_prop );

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
    DWORD style, ex_style;
    HWND parent;

    if (!(parent = GetAncestor( hwnd, GA_PARENT ))) return NULL;  /* desktop */

    /* don't create win data for HWND_MESSAGE windows */
    if (parent != GetDesktopWindow() && !GetAncestor( parent, GA_PARENT )) return NULL;

    data = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(struct ntdrv_win_data));
    if (!data) return NULL;
    data->hwnd = hwnd;

    /* Add it as a property to the window */
    SetPropA( hwnd, window_data_prop, (HANDLE)data );

    GetWindowRect( hwnd, &data->window_rect );
    MapWindowPoints( 0, parent, (POINT *)&data->window_rect, 2 );
    data->whole_rect = data->window_rect;
    GetClientRect( hwnd, &data->client_rect );
    MapWindowPoints( hwnd, parent, (POINT *)&data->client_rect, 2 );

    if (parent == GetDesktopWindow())
    {
        TRACE( "win %p window %s whole %s client %s\n",
               hwnd, wine_dbgstr_rect( &data->window_rect ),
               wine_dbgstr_rect( &data->whole_rect ), wine_dbgstr_rect( &data->client_rect ));

        style = GetWindowLongW( data->hwnd, GWL_STYLE );
        ex_style = GetWindowLongW( data->hwnd, GWL_EXSTYLE );

        /* Inform window manager about window rect in screen coords */
        SwmAddWindow(hwnd, &data->window_rect, style, ex_style);
        data->whole_window = (PVOID)1;
    }

    return data;
}

/* initialize the desktop window id in the desktop manager process */
struct ntdrv_win_data *NTDRV_create_desktop_win_data( HWND hwnd )
{
    struct ntdrv_win_data *data;

    data = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(struct ntdrv_win_data));
    if (!data) return NULL;
    data->hwnd = hwnd;

    /* Add it as a property to the window */
    SetPropA( hwnd, window_data_prop, (HANDLE)data );

    /* Mark it as being a whole window */
    data->whole_window = (PVOID)1;

    return data;
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

    /* Remove property */
    RemovePropA( hwnd, window_data_prop );

    /* Inform window manager */
    SwmRemoveWindow( hwnd );

    /* Free window data */
    HeapFree( GetProcessHeap(), 0, data );
}


/* EOF */
