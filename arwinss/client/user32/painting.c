/*
 * Window painting functions
 *
 * Copyright 1993, 1994, 1995, 2001, 2004, 2005, 2008 Alexandre Julliard
 * Copyright 1996, 1997, 1999 Alex Korobka
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

#include <assert.h>
#include <stdarg.h>
#include <string.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "wine/winternl.h"
#include "wine/server.h"
#include "win.h"
#include "user_private.h"
#include "controls.h"
#include "wine/list.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(win);


struct dce
{
    struct list entry;         /* entry in global DCE list */
    HDC         hdc;
    HWND        hwnd;
    HRGN        clip_rgn;
    DWORD       flags;
    LONG        count;         /* usage count; 0 or 1 for cache DCEs, always 1 for window DCEs,
                                  always >= 1 for class DCEs */
};

static struct list dce_list = LIST_INIT(dce_list);

static BOOL CALLBACK dc_hook( HDC hDC, WORD code, DWORD_PTR data, LPARAM lParam );

static const WCHAR displayW[] = { 'D','I','S','P','L','A','Y',0 };


/***********************************************************************
 *           dump_rdw_flags
 */
static void dump_rdw_flags(UINT flags)
{
    TRACE("flags:");
    if (flags & RDW_INVALIDATE) TRACE(" RDW_INVALIDATE");
    if (flags & RDW_INTERNALPAINT) TRACE(" RDW_INTERNALPAINT");
    if (flags & RDW_ERASE) TRACE(" RDW_ERASE");
    if (flags & RDW_VALIDATE) TRACE(" RDW_VALIDATE");
    if (flags & RDW_NOINTERNALPAINT) TRACE(" RDW_NOINTERNALPAINT");
    if (flags & RDW_NOERASE) TRACE(" RDW_NOERASE");
    if (flags & RDW_NOCHILDREN) TRACE(" RDW_NOCHILDREN");
    if (flags & RDW_ALLCHILDREN) TRACE(" RDW_ALLCHILDREN");
    if (flags & RDW_UPDATENOW) TRACE(" RDW_UPDATENOW");
    if (flags & RDW_ERASENOW) TRACE(" RDW_ERASENOW");
    if (flags & RDW_FRAME) TRACE(" RDW_FRAME");
    if (flags & RDW_NOFRAME) TRACE(" RDW_NOFRAME");

#define RDW_FLAGS \
    (RDW_INVALIDATE | \
    RDW_INTERNALPAINT | \
    RDW_ERASE | \
    RDW_VALIDATE | \
    RDW_NOINTERNALPAINT | \
    RDW_NOERASE | \
    RDW_NOCHILDREN | \
    RDW_ALLCHILDREN | \
    RDW_UPDATENOW | \
    RDW_ERASENOW | \
    RDW_FRAME | \
    RDW_NOFRAME)

    if (flags & ~RDW_FLAGS) TRACE(" %04x", flags & ~RDW_FLAGS);
    TRACE("\n");
#undef RDW_FLAGS
}


/***********************************************************************
 *		update_visible_region
 *
 * Set the visible region and X11 drawable for the DC associated to
 * a given window.
 */
static void update_visible_region( struct dce *dce )
{
    NTSTATUS status;
    HRGN vis_rgn = 0;
    HWND top_win = 0;
    DWORD flags = dce->flags;
    size_t size = 256;
    RECT win_rect, top_rect;

    /* don't clip siblings if using parent clip region */
    if (flags & DCX_PARENTCLIP) flags &= ~DCX_CLIPSIBLINGS;

    /* fetch the visible region from the server */
    do
    {
        RGNDATA *data = HeapAlloc( GetProcessHeap(), 0, sizeof(*data) + size - 1 );
        if (!data) return;

        SERVER_START_REQ( get_visible_region )
        {
            req->window  = wine_server_user_handle( dce->hwnd );
            req->flags   = flags;
            wine_server_set_reply( req, data->Buffer, size );
            if (!(status = wine_server_call( req )))
            {
                size_t reply_size = wine_server_reply_size( reply );
                data->rdh.dwSize   = sizeof(data->rdh);
                data->rdh.iType    = RDH_RECTANGLES;
                data->rdh.nCount   = reply_size / sizeof(RECT);
                data->rdh.nRgnSize = reply_size;
                vis_rgn = ExtCreateRegion( NULL, size, data );

                top_win         = wine_server_ptr_handle( reply->top_win );
                win_rect.left   = reply->win_rect.left;
                win_rect.top    = reply->win_rect.top;
                win_rect.right  = reply->win_rect.right;
                win_rect.bottom = reply->win_rect.bottom;
                top_rect.left   = reply->top_rect.left;
                top_rect.top    = reply->top_rect.top;
                top_rect.right  = reply->top_rect.right;
                top_rect.bottom = reply->top_rect.bottom;
            }
            else size = reply->total_size;
        }
        SERVER_END_REQ;
        HeapFree( GetProcessHeap(), 0, data );
    } while (status == STATUS_BUFFER_OVERFLOW);

    if (status || !vis_rgn) return;

    USER_Driver->pGetDC( dce->hdc, dce->hwnd, top_win, &win_rect, &top_rect, flags );

    if (dce->clip_rgn) CombineRgn( vis_rgn, vis_rgn, dce->clip_rgn,
                                   (flags & DCX_INTERSECTRGN) ? RGN_AND : RGN_DIFF );

    __wine_set_visible_region( dce->hdc, vis_rgn, &win_rect );
}


/***********************************************************************
 *		reset_dce_attrs
 */
static void reset_dce_attrs( struct dce *dce )
{
    RestoreDC( dce->hdc, 1 );  /* initial save level is always 1 */
    SaveDC( dce->hdc );  /* save the state again for next time */
}


/***********************************************************************
 *		release_dce
 */
static void release_dce( struct dce *dce )
{
    if (!dce->hwnd) return;  /* already released */

    USER_Driver->pReleaseDC( dce->hwnd, dce->hdc );

    if (dce->clip_rgn) DeleteObject( dce->clip_rgn );
    dce->clip_rgn = 0;
    dce->hwnd     = 0;
    dce->flags   &= DCX_CACHE;
}


/***********************************************************************
 *   delete_clip_rgn
 */
static void delete_clip_rgn( struct dce *dce )
{
    if (!dce->clip_rgn) return;  /* nothing to do */

    dce->flags &= ~(DCX_EXCLUDERGN | DCX_INTERSECTRGN);
    DeleteObject( dce->clip_rgn );
    dce->clip_rgn = 0;

    /* make it dirty so that the vis rgn gets recomputed next time */
    SetHookFlags( dce->hdc, DCHF_INVALIDATEVISRGN );
}


/***********************************************************************
 *           alloc_dce
 *
 * Allocate a new DCE.
 */
static struct dce *alloc_dce(void)
{
    struct dce *dce;

    if (!(dce = HeapAlloc( GetProcessHeap(), 0, sizeof(*dce) ))) return NULL;
    if (!(dce->hdc = CreateDCW( displayW, NULL, NULL, NULL )))
    {
        HeapFree( GetProcessHeap(), 0, dce );
        return 0;
    }
    SaveDC( dce->hdc );

    dce->hwnd      = 0;
    dce->clip_rgn  = 0;
    dce->flags     = 0;
    dce->count     = 1;

    /* store DCE handle in DC hook data field */
    SetDCHook( dce->hdc, dc_hook, (DWORD_PTR)dce );
    SetHookFlags( dce->hdc, DCHF_INVALIDATEVISRGN );
    return dce;
}


/***********************************************************************
 *		get_window_dce
 */
static struct dce *get_window_dce( HWND hwnd )
{
    struct dce *dce;
    WND *win = WIN_GetPtr( hwnd );

    if (!win || win == WND_OTHER_PROCESS || win == WND_DESKTOP) return NULL;

    dce = win->dce;
    if (!dce && (dce = get_class_dce( win->class )))
    {
        win->dce = dce;
        dce->count++;
    }
    WIN_ReleasePtr( win );

    if (!dce)  /* try to allocate one */
    {
        struct dce *dce_to_free = NULL;
        LONG class_style = GetClassLongW( hwnd, GCL_STYLE );

        if (class_style & CS_CLASSDC)
        {
            if (!(dce = alloc_dce())) return NULL;

            win = WIN_GetPtr( hwnd );
            if (win && win != WND_OTHER_PROCESS && win != WND_DESKTOP)
            {
                if (win->dce)  /* another thread beat us to it */
                {
                    dce_to_free = dce;
                    dce = win->dce;
                }
                else if ((win->dce = set_class_dce( win->class, dce )) != dce)
                {
                    dce_to_free = dce;
                    dce = win->dce;
                    dce->count++;
                }
                else
                {
                    dce->count++;
                    list_add_tail( &dce_list, &dce->entry );
                }
                WIN_ReleasePtr( win );
            }
            else dce_to_free = dce;
        }
        else if (class_style & CS_OWNDC)
        {
            if (!(dce = alloc_dce())) return NULL;

            win = WIN_GetPtr( hwnd );
            if (win && win != WND_OTHER_PROCESS && win != WND_DESKTOP)
            {
                if (win->dwStyle & WS_CLIPCHILDREN) dce->flags |= DCX_CLIPCHILDREN;
                if (win->dwStyle & WS_CLIPSIBLINGS) dce->flags |= DCX_CLIPSIBLINGS;
                if (win->dce)  /* another thread beat us to it */
                {
                    dce_to_free = dce;
                    dce = win->dce;
                }
                else
                {
                    win->dce = dce;
                    dce->hwnd = hwnd;
                    dce->count++;
                    list_add_tail( &dce_list, &dce->entry );
                }
                WIN_ReleasePtr( win );
            }
            else dce_to_free = dce;
        }

        if (dce_to_free)
        {
            SetDCHook( dce->hdc, NULL, 0 );
            DeleteDC( dce->hdc );
            HeapFree( GetProcessHeap(), 0, dce );
        }
    }
    return dce;
}


/***********************************************************************
 *           free_dce
 *
 * Free a class or window DCE.
 */
void free_dce( struct dce *dce, HWND hwnd )
{
    USER_Lock();

    if (dce)
    {
        if (!--dce->count)
        {
            /* turn it into a cache entry */
            reset_dce_attrs( dce );
            release_dce( dce );
            dce->flags |= DCX_CACHE;
        }
        else if (dce->hwnd == hwnd)
        {
            release_dce( dce );
        }
    }

    /* now check for cache DCEs */

    if (hwnd)
    {
        LIST_FOR_EACH_ENTRY( dce, &dce_list, struct dce, entry )
        {
            if (dce->hwnd != hwnd) continue;
            if (!(dce->flags & DCX_CACHE)) continue;

            if (dce->count) WARN( "GetDC() without ReleaseDC() for window %p\n", hwnd );
            dce->count = 0;
            release_dce( dce );
        }
    }

    USER_Unlock();
}


/***********************************************************************
 *           make_dc_dirty
 *
 * Mark the associated DC as dirty to force a refresh of the visible region
 */
static void make_dc_dirty( struct dce *dce )
{
    if (!dce->count)
    {
        /* Don't bother with visible regions of unused DCEs */
        TRACE("\tpurged %p dce [%p]\n", dce, dce->hwnd);
        release_dce( dce );
    }
    else
    {
        /* Set dirty bits in the hDC and DCE structs */
        TRACE("\tfixed up %p dce [%p]\n", dce, dce->hwnd);
        SetHookFlags( dce->hdc, DCHF_INVALIDATEVISRGN );
    }
}


/***********************************************************************
 *   invalidate_dce
 *
 * It is called from SetWindowPos() - we have to
 * mark as dirty all busy DCEs for windows that have pWnd->parent as
 * an ancestor and whose client rect intersects with specified update
 * rectangle. In addition, pWnd->parent DCEs may need to be updated if
 * DCX_CLIPCHILDREN flag is set.
 */
void invalidate_dce( HWND hwnd, const RECT *extra_rect )
{
    RECT window_rect;
    struct dce *dce;
    HWND hwndScope = GetAncestor( hwnd, GA_PARENT );

    if (!hwndScope) return;

    GetWindowRect( hwnd, &window_rect );

    TRACE("%p scope hwnd = %p %s (%s)\n",
          hwnd, hwndScope, wine_dbgstr_rect(&window_rect), wine_dbgstr_rect(extra_rect) );

    /* walk all DCEs and fixup non-empty entries */

    USER_Lock();
    LIST_FOR_EACH_ENTRY( dce, &dce_list, struct dce, entry )
    {
        TRACE( "%p: hwnd %p dcx %08x %s %s\n", dce, dce->hwnd, dce->flags,
               (dce->flags & DCX_CACHE) ? "Cache" : "Owned", dce->count ? "InUse" : "" );

        if (!dce->hwnd) continue;
        if ((dce->hwnd == hwndScope) && !(dce->flags & DCX_CLIPCHILDREN))
            continue;  /* child window positions don't bother us */

        /* if DCE window is a child of hwnd, it has to be invalidated */
        if (dce->hwnd == hwnd || IsChild( hwnd, dce->hwnd ))
        {
            make_dc_dirty( dce );
        }
        else  /* otherwise check if the window rectangle intersects this DCE window */
        {
            if (hwndScope == GetDesktopWindow() ||
                hwndScope == dce->hwnd || IsChild( hwndScope, dce->hwnd ))
            {
                RECT dce_rect, tmp;
                GetWindowRect( dce->hwnd, &dce_rect );
                if (IntersectRect( &tmp, &dce_rect, &window_rect ) ||
                    (extra_rect && IntersectRect( &tmp, &dce_rect, extra_rect )))
                    make_dc_dirty( dce );
            }
        }
    }
    USER_Unlock();
}

/***********************************************************************
 *		release_dc
 *
 * Implementation of ReleaseDC.
 */
static INT release_dc( HWND hwnd, HDC hdc, BOOL end_paint )
{
    struct dce *dce;
    BOOL ret = FALSE;

    TRACE("%p %p\n", hwnd, hdc );

    USER_Lock();
    dce = (struct dce *)GetDCHook( hdc, NULL );
    if (dce && dce->count)
    {
        if (!(dce->flags & DCX_NORESETATTRS)) reset_dce_attrs( dce );
        if (end_paint || (dce->flags & DCX_CACHE)) delete_clip_rgn( dce );
        if (dce->flags & DCX_CACHE) dce->count = 0;
        ret = TRUE;
    }
    USER_Unlock();
    return ret;
}


/***********************************************************************
 *		dc_hook
 *
 * See "Undoc. Windows" for hints (DC, SetDCHook, SetHookFlags)..
 */
static BOOL CALLBACK dc_hook( HDC hDC, WORD code, DWORD_PTR data, LPARAM lParam )
{
    BOOL retv = TRUE;
    struct dce *dce = (struct dce *)data;

    TRACE("hDC = %p, %u\n", hDC, code);

    if (!dce) return 0;
    assert( dce->hdc == hDC );

    switch( code )
    {
    case DCHC_INVALIDVISRGN:
        /* GDI code calls this when it detects that the
         * DC is dirty (usually after SetHookFlags()). This
         * means that we have to recompute the visible region.
         */
        if (dce->count) update_visible_region( dce );
        else /* non-fatal but shouldn't happen */
            WARN("DC is not in use!\n");
        break;
    case DCHC_DELETEDC:
        /*
         * Windows will not let you delete a DC that is busy
         * (between GetDC and ReleaseDC)
         */
        USER_Lock();
        if (dce->count > 1)
        {
            WARN("Application trying to delete a busy DC %p\n", dce->hdc);
            retv = FALSE;
        }
        else
        {
            list_remove( &dce->entry );
            if (dce->clip_rgn) DeleteObject( dce->clip_rgn );
            HeapFree( GetProcessHeap(), 0, dce );
        }
        USER_Unlock();
        break;
    }
    return retv;
}


/***********************************************************************
 *           get_update_region
 *
 * Return update region (in screen coordinates) for a window.
 */
static HRGN get_update_region( HWND hwnd, UINT *flags, HWND *child )
{
    HRGN hrgn = 0;
    NTSTATUS status;
    RGNDATA *data;
    size_t size = 256;

    do
    {
        if (!(data = HeapAlloc( GetProcessHeap(), 0, sizeof(*data) + size - 1 )))
        {
            SetLastError( ERROR_OUTOFMEMORY );
            return 0;
        }

        SERVER_START_REQ( get_update_region )
        {
            req->window     = wine_server_user_handle( hwnd );
            req->from_child = wine_server_user_handle( child ? *child : 0 );
            req->flags      = *flags;
            wine_server_set_reply( req, data->Buffer, size );
            if (!(status = wine_server_call( req )))
            {
                size_t reply_size = wine_server_reply_size( reply );
                data->rdh.dwSize   = sizeof(data->rdh);
                data->rdh.iType    = RDH_RECTANGLES;
                data->rdh.nCount   = reply_size / sizeof(RECT);
                data->rdh.nRgnSize = reply_size;
                hrgn = ExtCreateRegion( NULL, size, data );
                if (child) *child = wine_server_ptr_handle( reply->child );
                *flags = reply->flags;
            }
            else size = reply->total_size;
        }
        SERVER_END_REQ;
        HeapFree( GetProcessHeap(), 0, data );
    } while (status == STATUS_BUFFER_OVERFLOW);

    if (status) SetLastError( RtlNtStatusToDosError(status) );
    return hrgn;
}


/***********************************************************************
 *           get_update_flags
 *
 * Get only the update flags, not the update region.
 */
static BOOL get_update_flags( HWND hwnd, HWND *child, UINT *flags )
{
    BOOL ret;

    SERVER_START_REQ( get_update_region )
    {
        req->window     = wine_server_user_handle( hwnd );
        req->from_child = wine_server_user_handle( child ? *child : 0 );
        req->flags      = *flags | UPDATE_NOREGION;
        if ((ret = !wine_server_call_err( req )))
        {
            if (child) *child = wine_server_ptr_handle( reply->child );
            *flags = reply->flags;
        }
    }
    SERVER_END_REQ;
    return ret;
}


/***********************************************************************
 *           redraw_window_rects
 *
 * Redraw part of a window.
 */
static BOOL redraw_window_rects( HWND hwnd, UINT flags, const RECT *rects, UINT count )
{
    BOOL ret;

    if (!(flags & (RDW_INVALIDATE|RDW_VALIDATE|RDW_INTERNALPAINT|RDW_NOINTERNALPAINT)))
        return TRUE;  /* nothing to do */

    SERVER_START_REQ( redraw_window )
    {
        req->window = wine_server_user_handle( hwnd );
        req->flags  = flags;
        wine_server_add_data( req, rects, count * sizeof(RECT) );
        ret = !wine_server_call_err( req );
    }
    SERVER_END_REQ;
    return ret;
}


/***********************************************************************
 *           send_ncpaint
 *
 * Send a WM_NCPAINT message if needed, and return the resulting update region (in screen coords).
 * Helper for erase_now and BeginPaint.
 */
static HRGN send_ncpaint( HWND hwnd, HWND *child, UINT *flags )
{
    HRGN whole_rgn = get_update_region( hwnd, flags, child );
    HRGN client_rgn = 0;

    if (child) hwnd = *child;

    if (hwnd == GetDesktopWindow()) return whole_rgn;

    if (whole_rgn)
    {
        RECT client, update;
        INT type;

        /* check if update rgn overlaps with nonclient area */
        type = GetRgnBox( whole_rgn, &update );
        WIN_GetRectangles( hwnd, COORDS_SCREEN, 0, &client );

        if ((*flags & UPDATE_NONCLIENT) ||
            update.left < client.left || update.top < client.top ||
            update.right > client.right || update.bottom > client.bottom)
        {
            client_rgn = CreateRectRgnIndirect( &client );
            CombineRgn( client_rgn, client_rgn, whole_rgn, RGN_AND );

            /* check if update rgn contains complete nonclient area */
            if (type == SIMPLEREGION)
            {
                RECT window;
                GetWindowRect( hwnd, &window );
                if (EqualRect( &window, &update ))
                {
                    DeleteObject( whole_rgn );
                    whole_rgn = (HRGN)1;
                }
            }
        }
        else
        {
            client_rgn = whole_rgn;
            whole_rgn = 0;
        }

        if (whole_rgn) /* NOTE: WM_NCPAINT allows wParam to be 1 */
        {
            if (*flags & UPDATE_NONCLIENT) SendMessageW( hwnd, WM_NCPAINT, (WPARAM)whole_rgn, 0 );
            if (whole_rgn > (HRGN)1) DeleteObject( whole_rgn );
        }
    }
    return client_rgn;
}


/***********************************************************************
 *           send_erase
 *
 * Send a WM_ERASEBKGND message if needed, and optionally return the DC for painting.
 * If a DC is requested, the region is selected into it. In all cases the region is deleted.
 * Helper for erase_now and BeginPaint.
 */
static BOOL send_erase( HWND hwnd, UINT flags, HRGN client_rgn,
                        RECT *clip_rect, HDC *hdc_ret )
{
    BOOL need_erase = (flags & UPDATE_DELAYED_ERASE) != 0;
    HDC hdc = 0;
    RECT dummy;

    if (!clip_rect) clip_rect = &dummy;
    if (hdc_ret || (flags & UPDATE_ERASE))
    {
        UINT dcx_flags = DCX_INTERSECTRGN | DCX_USESTYLE;
        if (IsIconic(hwnd)) dcx_flags |= DCX_WINDOW;

        if ((hdc = GetDCEx( hwnd, client_rgn, dcx_flags )))
        {
            INT type = GetClipBox( hdc, clip_rect );

            if (flags & UPDATE_ERASE)
            {
                /* don't erase if the clip box is empty */
                if (type != NULLREGION)
                    need_erase = !SendMessageW( hwnd, WM_ERASEBKGND, (WPARAM)hdc, 0 );
            }
            if (!hdc_ret) release_dc( hwnd, hdc, TRUE );
        }

        if (hdc_ret) *hdc_ret = hdc;
    }
    if (!hdc) DeleteObject( client_rgn );
    return need_erase;
}


/***********************************************************************
 *           erase_now
 *
 * Implementation of RDW_ERASENOW behavior.
 */
void erase_now( HWND hwnd, UINT rdw_flags )
{
    HWND child = 0;
    HRGN hrgn;
    BOOL need_erase = FALSE;

    /* loop while we find a child to repaint */
    for (;;)
    {
        UINT flags = UPDATE_NONCLIENT | UPDATE_ERASE;

        if (rdw_flags & RDW_NOCHILDREN) flags |= UPDATE_NOCHILDREN;
        else if (rdw_flags & RDW_ALLCHILDREN) flags |= UPDATE_ALLCHILDREN;
        if (need_erase) flags |= UPDATE_DELAYED_ERASE;

        if (!(hrgn = send_ncpaint( hwnd, &child, &flags ))) break;
        need_erase = send_erase( child, flags, hrgn, NULL, NULL );

        if (!flags) break;  /* nothing more to do */
        if ((rdw_flags & RDW_NOCHILDREN) && !need_erase) break;
    }
}


/***********************************************************************
 *           update_now
 *
 * Implementation of RDW_UPDATENOW behavior.
 *
 * FIXME: Windows uses WM_SYNCPAINT to cut down the number of intertask
 * SendMessage() calls. This is a comment inside DefWindowProc() source
 * from 16-bit SDK:
 *
 *   This message avoids lots of inter-app message traffic
 *   by switching to the other task and continuing the
 *   recursion there.
 *
 * wParam         = flags
 * LOWORD(lParam) = hrgnClip
 * HIWORD(lParam) = hwndSkip  (not used; always NULL)
 *
 */
static void update_now( HWND hwnd, UINT rdw_flags )
{
    HWND child = 0;

    /* desktop window never gets WM_PAINT, only WM_ERASEBKGND */
    if (hwnd == GetDesktopWindow()) erase_now( hwnd, rdw_flags | RDW_NOCHILDREN );

    /* loop while we find a child to repaint */
    for (;;)
    {
        UINT flags = UPDATE_PAINT | UPDATE_INTERNALPAINT;

        if (rdw_flags & RDW_NOCHILDREN) flags |= UPDATE_NOCHILDREN;
        else if (rdw_flags & RDW_ALLCHILDREN) flags |= UPDATE_ALLCHILDREN;

        if (!get_update_flags( hwnd, &child, &flags )) break;
        if (!flags) break;  /* nothing more to do */

        SendMessageW( child, WM_PAINT, 0, 0 );
        if (rdw_flags & RDW_NOCHILDREN) break;
    }
}


/*************************************************************************
 *             fix_caret
 *
 * Helper for ScrollWindowEx:
 * If the return value is 0, no special caret handling is necessary.
 * Otherwise the return value is the handle of the window that owns the
 * caret. Its caret needs to be hidden during the scroll operation and
 * moved to new_caret_pos if move_caret is TRUE.
 */
static HWND fix_caret(HWND hWnd, const RECT *scroll_rect, INT dx, INT dy,
                     UINT flags, LPBOOL move_caret, LPPOINT new_caret_pos)
{
    GUITHREADINFO info;
    RECT rect, mapped_rcCaret;
    BOOL hide_caret = FALSE;

    info.cbSize = sizeof(info);
    if (!GetGUIThreadInfo( GetCurrentThreadId(), &info )) return 0;
    if (!info.hwndCaret) return 0;
    
    if (info.hwndCaret == hWnd)
    {
        /* Move the caret if it's (partially) in the source rectangle */
        if (IntersectRect(&rect, scroll_rect, &info.rcCaret))
        {
            *move_caret = TRUE;
            hide_caret = TRUE;
            new_caret_pos->x = info.rcCaret.left + dx;
            new_caret_pos->y = info.rcCaret.top + dy;
        }
        else
        {
            *move_caret = FALSE;
            
            /* Hide the caret if it's in the destination rectangle */
            rect = *scroll_rect;
            OffsetRect(&rect, dx, dy);
            hide_caret = IntersectRect(&rect, &rect, &info.rcCaret);
        }
    }
    else
    {
        if ((flags & SW_SCROLLCHILDREN) && IsChild(hWnd, info.hwndCaret))
        {
            *move_caret = FALSE;
            
            /* Hide the caret if it's in the source or in the destination
               rectangle */
            mapped_rcCaret = info.rcCaret;
            MapWindowPoints(info.hwndCaret, hWnd, (LPPOINT)&mapped_rcCaret, 2);
            
            if (IntersectRect(&rect, scroll_rect, &mapped_rcCaret))
            {
                hide_caret = TRUE;
            }
            else
            {
                rect = *scroll_rect;
                OffsetRect(&rect, dx, dy);
                hide_caret = IntersectRect(&rect, &rect, &mapped_rcCaret);
            }
        }
        else
            return 0;
    }

    if (hide_caret)
    {    
        HideCaret(info.hwndCaret);
        return info.hwndCaret;
    }
    else
        return 0;
}


/***********************************************************************
 *		BeginPaint (USER32.@)
 */
HDC WINAPI BeginPaint( HWND hwnd, PAINTSTRUCT *lps )
{
    HRGN hrgn;
    UINT flags = UPDATE_NONCLIENT | UPDATE_ERASE | UPDATE_PAINT | UPDATE_INTERNALPAINT | UPDATE_NOCHILDREN;

    if (!lps) return 0;

    HideCaret( hwnd );

    if (!(hrgn = send_ncpaint( hwnd, NULL, &flags ))) return 0;

    lps->fErase = send_erase( hwnd, flags, hrgn, &lps->rcPaint, &lps->hdc );

    TRACE("hdc = %p box = (%s), fErase = %d\n",
          lps->hdc, wine_dbgstr_rect(&lps->rcPaint), lps->fErase);

    return lps->hdc;
}


/***********************************************************************
 *		EndPaint (USER32.@)
 */
BOOL WINAPI EndPaint( HWND hwnd, const PAINTSTRUCT *lps )
{
    if (!lps) return FALSE;
    release_dc( hwnd, lps->hdc, TRUE );
    ShowCaret( hwnd );
    return TRUE;
}


/***********************************************************************
 *		GetDCEx (USER32.@)
 */
HDC WINAPI GetDCEx( HWND hwnd, HRGN hrgnClip, DWORD flags )
{
    const DWORD clip_flags = DCX_PARENTCLIP | DCX_CLIPSIBLINGS | DCX_CLIPCHILDREN | DCX_WINDOW;
    const DWORD user_flags = clip_flags | DCX_NORESETATTRS; /* flags that can be set by user */
    struct dce *dce;
    BOOL bUpdateVisRgn = TRUE;
    HWND parent;
    LONG window_style = GetWindowLongW( hwnd, GWL_STYLE );

    if (!hwnd) hwnd = GetDesktopWindow();
    else hwnd = WIN_GetFullHandle( hwnd );

    TRACE("hwnd %p, hrgnClip %p, flags %08x\n", hwnd, hrgnClip, flags);

    if (!IsWindow(hwnd)) return 0;

    /* fixup flags */

    if (flags & (DCX_WINDOW | DCX_PARENTCLIP)) flags |= DCX_CACHE;

    if (flags & DCX_USESTYLE)
    {
        flags &= ~(DCX_CLIPCHILDREN | DCX_CLIPSIBLINGS | DCX_PARENTCLIP);

        if (window_style & WS_CLIPSIBLINGS) flags |= DCX_CLIPSIBLINGS;

        if (!(flags & DCX_WINDOW))
        {
            if (GetClassLongW( hwnd, GCL_STYLE ) & CS_PARENTDC) flags |= DCX_PARENTCLIP;

            if (window_style & WS_CLIPCHILDREN && !(window_style & WS_MINIMIZE))
                flags |= DCX_CLIPCHILDREN;
        }
    }

    if (flags & DCX_WINDOW) flags &= ~DCX_CLIPCHILDREN;

    parent = GetAncestor( hwnd, GA_PARENT );
    if (!parent || (parent == GetDesktopWindow()))
        flags = (flags & ~DCX_PARENTCLIP) | DCX_CLIPSIBLINGS;

    /* it seems parent clip is ignored when clipping siblings or children */
    if (flags & (DCX_CLIPSIBLINGS | DCX_CLIPCHILDREN)) flags &= ~DCX_PARENTCLIP;

    if( flags & DCX_PARENTCLIP )
    {
        LONG parent_style = GetWindowLongW( parent, GWL_STYLE );
        if( (window_style & WS_VISIBLE) && (parent_style & WS_VISIBLE) )
        {
            flags &= ~DCX_CLIPCHILDREN;
            if (parent_style & WS_CLIPSIBLINGS) flags |= DCX_CLIPSIBLINGS;
        }
    }

    /* find a suitable DCE */

    if ((flags & DCX_CACHE) || !(dce = get_window_dce( hwnd )))
    {
        struct dce *dceEmpty = NULL, *dceUnused = NULL;

        /* Strategy: First, we attempt to find a non-empty but unused DCE with
         * compatible flags. Next, we look for an empty entry. If the cache is
         * full we have to purge one of the unused entries.
         */
        USER_Lock();
        LIST_FOR_EACH_ENTRY( dce, &dce_list, struct dce, entry )
        {
            if ((dce->flags & DCX_CACHE) && !dce->count)
            {
                dceUnused = dce;

                if (!dce->hwnd) dceEmpty = dce;
                else if ((dce->hwnd == hwnd) && !((dce->flags ^ flags) & clip_flags))
                {
                    TRACE("\tfound valid %p dce [%p], flags %08x\n",
                          dce, hwnd, dce->flags );
                    bUpdateVisRgn = FALSE;
                    break;
                }
            }
        }

        if (&dce->entry == &dce_list)  /* nothing found */
            dce = dceEmpty ? dceEmpty : dceUnused;

        if (dce) dce->count = 1;

        USER_Unlock();

        /* if there's no dce empty or unused, allocate a new one */
        if (!dce)
        {
            if (!(dce = alloc_dce())) return 0;
            dce->flags = DCX_CACHE;
            USER_Lock();
            list_add_head( &dce_list, &dce->entry );
            USER_Unlock();
        }
    }
    else
    {
        flags |= DCX_NORESETATTRS;
        if (dce->hwnd == hwnd)
        {
            TRACE("\tskipping hVisRgn update\n");
            bUpdateVisRgn = FALSE; /* updated automatically, via DCHook() */
        }
        else
        {
            /* we should free dce->clip_rgn here, but Windows apparently doesn't */
            dce->flags &= ~(DCX_EXCLUDERGN | DCX_INTERSECTRGN);
            dce->clip_rgn = 0;
        }
    }

    if (flags & (DCX_INTERSECTRGN | DCX_EXCLUDERGN))
    {
        /* if the extra clip region has changed, get rid of the old one */
        if (dce->clip_rgn != hrgnClip || ((flags ^ dce->flags) & (DCX_INTERSECTRGN | DCX_EXCLUDERGN)))
            delete_clip_rgn( dce );
        dce->clip_rgn = hrgnClip;
        if (!dce->clip_rgn) dce->clip_rgn = CreateRectRgn( 0, 0, 0, 0 );
        dce->flags |= flags & (DCX_INTERSECTRGN | DCX_EXCLUDERGN);
        bUpdateVisRgn = TRUE;
    }

    if (GetWindowLongW( hwnd, GWL_EXSTYLE ) & WS_EX_LAYOUTRTL) SetLayout( dce->hdc, LAYOUT_RTL );

    dce->hwnd = hwnd;
    dce->flags = (dce->flags & ~user_flags) | (flags & user_flags);

    if (SetHookFlags( dce->hdc, DCHF_VALIDATEVISRGN )) bUpdateVisRgn = TRUE;  /* DC was dirty */

    if (bUpdateVisRgn) update_visible_region( dce );

    TRACE("(%p,%p,0x%x): returning %p\n", hwnd, hrgnClip, flags, dce->hdc);
    return dce->hdc;
}


/***********************************************************************
 *		GetDC (USER32.@)
 *
 * Get a device context.
 *
 * RETURNS
 *	Success: Handle to the device context
 *	Failure: NULL.
 */
HDC WINAPI GetDC( HWND hwnd )
{
    if (!hwnd) return GetDCEx( 0, 0, DCX_CACHE | DCX_WINDOW );
    return GetDCEx( hwnd, 0, DCX_USESTYLE );
}


/***********************************************************************
 *		GetWindowDC (USER32.@)
 */
HDC WINAPI GetWindowDC( HWND hwnd )
{
    return GetDCEx( hwnd, 0, DCX_USESTYLE | DCX_WINDOW );
}


/***********************************************************************
 *		ReleaseDC (USER32.@)
 *
 * Release a device context.
 *
 * RETURNS
 *	Success: Non-zero. Resources used by hdc are released.
 *	Failure: 0.
 */
INT WINAPI ReleaseDC( HWND hwnd, HDC hdc )
{
    return release_dc( hwnd, hdc, FALSE );
}


/**********************************************************************
 *		WindowFromDC (USER32.@)
 */
HWND WINAPI WindowFromDC( HDC hdc )
{
    struct dce *dce;
    HWND hwnd = 0;

    USER_Lock();
    dce = (struct dce *)GetDCHook( hdc, NULL );
    if (dce) hwnd = dce->hwnd;
    USER_Unlock();
    return hwnd;
}


/***********************************************************************
 *		LockWindowUpdate (USER32.@)
 *
 * Enables or disables painting in the chosen window.
 *
 * PARAMS
 *  hwnd [I] handle to a window.
 *
 * RETURNS
 *  If successful, returns nonzero value. Otherwise,
 *  returns 0.
 *
 * NOTES
 *  You can lock only one window at a time.
 */
BOOL WINAPI LockWindowUpdate( HWND hwnd )
{
    static HWND lockedWnd;

    FIXME("(%p), partial stub!\n",hwnd);

    USER_Lock();
    if (lockedWnd)
    {
        if (!hwnd)
        {
            /* Unlock lockedWnd */
            /* FIXME: Do something */
        }
        else
        {
            /* Attempted to lock a second window */
            /* Return FALSE and do nothing */
            USER_Unlock();
            return FALSE;
        }
    }
    lockedWnd = hwnd;
    USER_Unlock();
    return TRUE;
}


/***********************************************************************
 *		RedrawWindow (USER32.@)
 */
BOOL WINAPI RedrawWindow( HWND hwnd, const RECT *rect, HRGN hrgn, UINT flags )
{
    static const RECT empty;
    BOOL ret;

    if (!hwnd) hwnd = GetDesktopWindow();

    if (TRACE_ON(win))
    {
        if (hrgn)
        {
            RECT r;
            GetRgnBox( hrgn, &r );
            TRACE( "%p region %p box %s ", hwnd, hrgn, wine_dbgstr_rect(&r) );
        }
        else if (rect)
            TRACE( "%p rect %s ", hwnd, wine_dbgstr_rect(rect) );
        else
            TRACE( "%p whole window ", hwnd );

        dump_rdw_flags(flags);
    }

    /* process pending expose events before painting */
    if (flags & RDW_UPDATENOW) USER_Driver->pMsgWaitForMultipleObjectsEx( 0, NULL, 0, QS_PAINT, 0 );

    if (rect && !hrgn)
    {
        if (IsRectEmpty( rect )) rect = &empty;
        ret = redraw_window_rects( hwnd, flags, rect, 1 );
    }
    else if (!hrgn)
    {
        ret = redraw_window_rects( hwnd, flags, NULL, 0 );
    }
    else  /* need to build a list of the region rectangles */
    {
        DWORD size;
        RGNDATA *data = NULL;

        if (!(size = GetRegionData( hrgn, 0, NULL ))) return FALSE;
        if (!(data = HeapAlloc( GetProcessHeap(), 0, size ))) return FALSE;
        GetRegionData( hrgn, size, data );
        if (!data->rdh.nCount)  /* empty region -> use a single all-zero rectangle */
            ret = redraw_window_rects( hwnd, flags, &empty, 1 );
        else
            ret = redraw_window_rects( hwnd, flags, (const RECT *)data->Buffer, data->rdh.nCount );
        HeapFree( GetProcessHeap(), 0, data );
    }

    if (flags & RDW_UPDATENOW) update_now( hwnd, flags );
    else if (flags & RDW_ERASENOW) erase_now( hwnd, flags );

    return ret;
}


/***********************************************************************
 *		UpdateWindow (USER32.@)
 */
BOOL WINAPI UpdateWindow( HWND hwnd )
{
    if (!hwnd)
    {
        SetLastError( ERROR_INVALID_WINDOW_HANDLE );
        return FALSE;
    }

    return RedrawWindow( hwnd, NULL, 0, RDW_UPDATENOW | RDW_ALLCHILDREN );
}


/***********************************************************************
 *		InvalidateRgn (USER32.@)
 */
BOOL WINAPI InvalidateRgn( HWND hwnd, HRGN hrgn, BOOL erase )
{
    if (!hwnd)
    {
        SetLastError( ERROR_INVALID_WINDOW_HANDLE );
        return FALSE;
    }

    return RedrawWindow(hwnd, NULL, hrgn, RDW_INVALIDATE | (erase ? RDW_ERASE : 0) );
}


/***********************************************************************
 *		InvalidateRect (USER32.@)
 *
 * MSDN: if hwnd parameter is NULL, InvalidateRect invalidates and redraws
 * all windows and sends WM_ERASEBKGND and WM_NCPAINT.
 */
BOOL WINAPI InvalidateRect( HWND hwnd, const RECT *rect, BOOL erase )
{
    UINT flags = RDW_INVALIDATE | (erase ? RDW_ERASE : 0);

    if (!hwnd)
    {
        flags = RDW_ALLCHILDREN | RDW_INVALIDATE | RDW_FRAME | RDW_ERASE | RDW_ERASENOW;
        rect = NULL;
    }

    return RedrawWindow( hwnd, rect, 0, flags );
}


/***********************************************************************
 *		ValidateRgn (USER32.@)
 */
BOOL WINAPI ValidateRgn( HWND hwnd, HRGN hrgn )
{
    if (!hwnd)
    {
        SetLastError( ERROR_INVALID_WINDOW_HANDLE );
        return FALSE;
    }

    return RedrawWindow( hwnd, NULL, hrgn, RDW_VALIDATE );
}


/***********************************************************************
 *		ValidateRect (USER32.@)
 *
 * MSDN: if hwnd parameter is NULL, ValidateRect invalidates and redraws
 * all windows and sends WM_ERASEBKGND and WM_NCPAINT.
 */
BOOL WINAPI ValidateRect( HWND hwnd, const RECT *rect )
{
    UINT flags = RDW_VALIDATE;

    if (!hwnd)
    {
        flags = RDW_ALLCHILDREN | RDW_INVALIDATE | RDW_FRAME | RDW_ERASE | RDW_ERASENOW;
        rect = NULL;
    }

    return RedrawWindow( hwnd, rect, 0, flags );
}


/***********************************************************************
 *		GetUpdateRgn (USER32.@)
 */
INT WINAPI GetUpdateRgn( HWND hwnd, HRGN hrgn, BOOL erase )
{
    INT retval = ERROR;
    UINT flags = UPDATE_NOCHILDREN;
    HRGN update_rgn;

    if (erase) flags |= UPDATE_NONCLIENT | UPDATE_ERASE;

    if ((update_rgn = send_ncpaint( hwnd, NULL, &flags )))
    {
        retval = CombineRgn( hrgn, update_rgn, 0, RGN_COPY );
        if (send_erase( hwnd, flags, update_rgn, NULL, NULL ))
        {
            flags = UPDATE_DELAYED_ERASE;
            get_update_flags( hwnd, NULL, &flags );
        }
        /* map region to client coordinates */
        map_window_region( 0, hwnd, hrgn );
    }
    return retval;
}


/***********************************************************************
 *		GetUpdateRect (USER32.@)
 */
BOOL WINAPI GetUpdateRect( HWND hwnd, LPRECT rect, BOOL erase )
{
    UINT flags = UPDATE_NOCHILDREN;
    HRGN update_rgn;
    BOOL need_erase;

    if (erase) flags |= UPDATE_NONCLIENT | UPDATE_ERASE;

    if (!(update_rgn = send_ncpaint( hwnd, NULL, &flags ))) return FALSE;

    if (rect)
    {
        if (GetRgnBox( update_rgn, rect ) != NULLREGION)
        {
            HDC hdc = GetDCEx( hwnd, 0, DCX_USESTYLE );
            DWORD layout = SetLayout( hdc, 0 );  /* MapWindowPoints mirrors already */
            MapWindowPoints( 0, hwnd, (LPPOINT)rect, 2 );
            DPtoLP( hdc, (LPPOINT)rect, 2 );
            SetLayout( hdc, layout );
            ReleaseDC( hwnd, hdc );
        }
    }
    need_erase = send_erase( hwnd, flags, update_rgn, NULL, NULL );

    /* check if we still have an update region */
    flags = UPDATE_PAINT | UPDATE_NOCHILDREN;
    if (need_erase) flags |= UPDATE_DELAYED_ERASE;
    return (get_update_flags( hwnd, NULL, &flags ) && (flags & UPDATE_PAINT));
}


/***********************************************************************
 *		ExcludeUpdateRgn (USER32.@)
 */
INT WINAPI ExcludeUpdateRgn( HDC hdc, HWND hwnd )
{
    HRGN update_rgn = CreateRectRgn( 0, 0, 0, 0 );
    INT ret = GetUpdateRgn( hwnd, update_rgn, FALSE );

    if (ret != ERROR)
    {
        POINT pt;

        GetDCOrgEx( hdc, &pt );
        MapWindowPoints( 0, hwnd, &pt, 1 );
        OffsetRgn( update_rgn, -pt.x, -pt.y );
        ret = ExtSelectClipRgn( hdc, update_rgn, RGN_DIFF );
        DeleteObject( update_rgn );
    }
    return ret;
}


/*************************************************************************
 *		ScrollWindowEx (USER32.@)
 *
 * Note: contrary to what the doc says, pixels that are scrolled from the
 *      outside of clipRect to the inside are NOT painted.
 *
 */
INT WINAPI ScrollWindowEx( HWND hwnd, INT dx, INT dy,
                           const RECT *rect, const RECT *clipRect,
                           HRGN hrgnUpdate, LPRECT rcUpdate,
                           UINT flags )
{
    INT   retVal = NULLREGION;
    BOOL  bOwnRgn = TRUE;
    BOOL  bUpdate = (rcUpdate || hrgnUpdate || flags & (SW_INVALIDATE | SW_ERASE));
    int rdw_flags;
    HRGN  hrgnTemp;
    HRGN  hrgnWinupd = 0;
    HDC   hDC;
    RECT  rc, cliprc;
    HWND hwndCaret = NULL;
    BOOL moveCaret = FALSE;
    POINT newCaretPos;

    TRACE( "%p, %d,%d hrgnUpdate=%p rcUpdate = %p %s %04x\n",
           hwnd, dx, dy, hrgnUpdate, rcUpdate, wine_dbgstr_rect(rect), flags );
    TRACE( "clipRect = %s\n", wine_dbgstr_rect(clipRect));
    if( flags & ~( SW_SCROLLCHILDREN | SW_INVALIDATE | SW_ERASE))
        FIXME("some flags (%04x) are unhandled\n", flags);

    rdw_flags = (flags & SW_ERASE) && (flags & SW_INVALIDATE) ?
                                RDW_INVALIDATE | RDW_ERASE  : RDW_INVALIDATE ;

    if (!WIN_IsWindowDrawable( hwnd, TRUE )) return ERROR;
    hwnd = WIN_GetFullHandle( hwnd );

    GetClientRect(hwnd, &rc);

    if (clipRect) IntersectRect(&cliprc,&rc,clipRect);
    else cliprc = rc;

    if (rect) IntersectRect(&rc, &rc, rect);

    if( hrgnUpdate ) bOwnRgn = FALSE;
    else if( bUpdate ) hrgnUpdate = CreateRectRgn( 0, 0, 0, 0 );

    newCaretPos.x = newCaretPos.y = 0;

    if( !IsRectEmpty(&cliprc) && (dx || dy)) {
        DWORD dcxflags = DCX_CACHE;
        DWORD style = GetWindowLongW( hwnd, GWL_STYLE );

        hwndCaret = fix_caret(hwnd, &rc, dx, dy, flags, &moveCaret, &newCaretPos);

        if( style & WS_CLIPSIBLINGS) dcxflags |= DCX_CLIPSIBLINGS;
        if( GetClassLongW( hwnd, GCL_STYLE ) & CS_PARENTDC)
            dcxflags |= DCX_PARENTCLIP;
        if( !(flags & SW_SCROLLCHILDREN) && (style & WS_CLIPCHILDREN))
            dcxflags |= DCX_CLIPCHILDREN;
        hDC = GetDCEx( hwnd, 0, dcxflags);
        if (hDC)
        {
            ScrollDC( hDC, dx, dy, &rc, &cliprc, hrgnUpdate, rcUpdate );

            ReleaseDC( hwnd, hDC );

            if (!bUpdate)
                RedrawWindow( hwnd, NULL, hrgnUpdate, rdw_flags);
        }

        /* If the windows has an update region, this must be
         * scrolled as well. Keep a copy in hrgnWinupd
         * to be added to hrngUpdate at the end. */
        hrgnTemp = CreateRectRgn( 0, 0, 0, 0 );
        retVal = GetUpdateRgn( hwnd, hrgnTemp, FALSE );
        if (retVal != NULLREGION)
        {
            HRGN hrgnClip = CreateRectRgnIndirect(&cliprc);
            if( !bOwnRgn) {
                hrgnWinupd = CreateRectRgn( 0, 0, 0, 0);
                CombineRgn( hrgnWinupd, hrgnTemp, 0, RGN_COPY);
            }
            OffsetRgn( hrgnTemp, dx, dy );
            CombineRgn( hrgnTemp, hrgnTemp, hrgnClip, RGN_AND );
            if( !bOwnRgn)
                CombineRgn( hrgnWinupd, hrgnWinupd, hrgnTemp, RGN_OR );
            RedrawWindow( hwnd, NULL, hrgnTemp, rdw_flags);

           /* Catch the case where the scrolling amount exceeds the size of the
            * original window. This generated a second update area that is the
            * location where the original scrolled content would end up.
            * This second region is not returned by the ScrollDC and sets
            * ScrollWindowEx apart from just a ScrollDC.
            *
            * This has been verified with testing on windows.
            */
            if (abs(dx) > abs(rc.right - rc.left) ||
                abs(dy) > abs(rc.bottom - rc.top))
            {
                SetRectRgn( hrgnTemp, rc.left + dx, rc.top + dy, rc.right+dx, rc.bottom + dy);
                CombineRgn( hrgnTemp, hrgnTemp, hrgnClip, RGN_AND );
                CombineRgn( hrgnUpdate, hrgnUpdate, hrgnTemp, RGN_OR );

                if( !bOwnRgn)
                    CombineRgn( hrgnWinupd, hrgnWinupd, hrgnTemp, RGN_OR );
            }
            DeleteObject( hrgnClip );
        }
        DeleteObject( hrgnTemp );
    } else {
        /* nothing was scrolled */
        if( !bOwnRgn)
            SetRectRgn( hrgnUpdate, 0, 0, 0, 0 );
        SetRectEmpty( rcUpdate);
    }

    if( flags & SW_SCROLLCHILDREN )
    {
        HWND *list = WIN_ListChildren( hwnd );
        if (list)
        {
            int i;
            RECT r, dummy;
            for (i = 0; list[i]; i++)
            {
                WIN_GetRectangles( list[i], COORDS_PARENT, &r, NULL );
                if (!rect || IntersectRect(&dummy, &r, rect))
                    SetWindowPos( list[i], 0, r.left + dx, r.top  + dy, 0, 0,
                                  SWP_NOZORDER | SWP_NOSIZE | SWP_NOACTIVATE |
                                  SWP_NOREDRAW | SWP_DEFERERASE );
            }
            HeapFree( GetProcessHeap(), 0, list );
        }
    }

    if( flags & (SW_INVALIDATE | SW_ERASE) )
        RedrawWindow( hwnd, NULL, hrgnUpdate, rdw_flags |
                      ((flags & SW_SCROLLCHILDREN) ? RDW_ALLCHILDREN : 0 ) );

    if( hrgnWinupd) {
        CombineRgn( hrgnUpdate, hrgnUpdate, hrgnWinupd, RGN_OR);
        DeleteObject( hrgnWinupd);
    }

    if( hwndCaret ) {
        if ( moveCaret ) SetCaretPos( newCaretPos.x, newCaretPos.y );
        ShowCaret(hwndCaret);
    }

    if( bOwnRgn && hrgnUpdate ) DeleteObject( hrgnUpdate );

    return retVal;
}


/*************************************************************************
 *		ScrollWindow (USER32.@)
 *
 */
BOOL WINAPI ScrollWindow( HWND hwnd, INT dx, INT dy,
                          const RECT *rect, const RECT *clipRect )
{
    return (ERROR != ScrollWindowEx( hwnd, dx, dy, rect, clipRect, 0, NULL,
                                     (rect ? 0 : SW_SCROLLCHILDREN) |
                                     SW_INVALIDATE | SW_ERASE ));
}


/*************************************************************************
 *		ScrollDC (USER32.@)
 *
 * dx, dy, lprcScroll and lprcClip are all in logical coordinates (msdn is
 * wrong) hrgnUpdate is returned in device coordinates with rcUpdate in
 * logical coordinates.
 */
BOOL WINAPI ScrollDC( HDC hdc, INT dx, INT dy, const RECT *lprcScroll,
                      const RECT *lprcClip, HRGN hrgnUpdate, LPRECT lprcUpdate )

{
    return USER_Driver->pScrollDC( hdc, dx, dy, lprcScroll, lprcClip, hrgnUpdate, lprcUpdate );
}

/************************************************************************
 *		PrintWindow (USER32.@)
 *
 */
BOOL WINAPI PrintWindow(HWND hwnd, HDC hdcBlt, UINT nFlags)
{
    UINT flags = PRF_CHILDREN | PRF_ERASEBKGND | PRF_OWNED | PRF_CLIENT;
    if(!(nFlags & PW_CLIENTONLY))
    {
        flags |= PRF_NONCLIENT;
    }
    SendMessageW(hwnd, WM_PRINT, (WPARAM)hdcBlt, flags);
    return TRUE;
}
