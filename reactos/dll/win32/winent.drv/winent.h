/*
 * WINENT driver definitions
 *
 * Copyright 2009 Aleksey Bragin
 * Some parts taken from Wine project (winex11.drv)
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

#include <stdarg.h>
#include <stdio.h>
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "wingdi.h"
#include "shellapi.h"
#define NTOS_USER_MODE
#include <ndk/ntndk.h>
#include <winddi.h>
#include <win32k/ntgdityp.h>
#include "ntrosgdi.h"
#include "wine/rosuser.h"
#include "wine/list.h"
#include "wine/unicode.h"
#include "wine/server.h"
#include <pseh/pseh2.h>

/* GDI escapes */

#define NTDRV_ESCAPE 6789
enum ntdrv_escape_codes
{
    NTDRV_GET_DISPLAY,      /* get X11 display for a DC */
    NTDRV_GET_DRAWABLE,     /* get current drawable for a DC */
    NTDRV_GET_FONT,         /* get current X font for a DC */
    NTDRV_SET_DRAWABLE,     /* set current drawable for a DC */
    NTDRV_START_EXPOSURES,  /* start graphics exposures */
    NTDRV_END_EXPOSURES,    /* end graphics exposures */
    NTDRV_GET_DCE,          /* no longer used */
    NTDRV_SET_DCE,          /* no longer used */
    NTDRV_GET_GLX_DRAWABLE, /* get current glx drawable for a DC */
    NTDRV_SYNC_PIXMAP,      /* sync the dibsection to its pixmap */
    NTDRV_FLUSH_GL_DRAWABLE /* flush changes made to the gl drawable */
};

struct ntdrv_escape_set_drawable
{
    enum ntdrv_escape_codes  code;         /* escape code (NTDRV_SET_DRAWABLE) */
    GR_WINDOW_ID             drawable;
    BOOL                     clip_children;/* ClipByChildren or IncludeInferiors */
    RECT                     dc_rect;      /* DC rectangle relative to drawable */
    RECT                     drawable_rect;/* Drawable rectangle relative to screen */
    HWND                     hwnd;         /* hwnd of which the GetDC is performed */
    int                      gl_copy;      /* whether the GL contents need explicit copying */
    BOOL                     release;      /* whether the DC is acquired or released */
};

/* ntdrv private window data */
struct ntdrv_win_data
{
    struct list  entry;          /* entry in the linked list of win data */
    HWND         hwnd;           /* hwnd that this private data belongs to */
    GR_WINDOW_ID whole_window;   /* SWM window for the complete window */
    GR_WINDOW_ID client_window;  /* SWM window for the client area */
    RECT        window_rect;    /* USER window rectangle relative to parent */
    RECT        whole_rect;     /* SWM window rectangle for the whole window relative to parent */
    RECT        client_rect;    /* client area relative to parent */
    HCURSOR     cursor;         /* current cursor */
    BOOL        mapped : 1;     /* is window mapped? (in either normal or iconic state) */
    BOOL        iconic : 1;     /* is window in iconic state? */
    BOOL        shaped : 1;     /* is window using a custom region shape? */
};

//typedef void (*ntdrv_event_handler)( HWND hwnd, GR_EVENT *event );

extern GR_WINDOW_ID root_window;

static inline void mirror_rect( const RECT *window_rect, RECT *rect )
{
    int width = window_rect->right - window_rect->left;
    int tmp = rect->left;
    rect->left = width - rect->right;
    rect->right = width - tmp;
}

/* clipboard.c */
void NTDRV_InitClipboard(void);
VOID InitHandleMapping();
VOID AddHandleMapping(HGDIOBJ hKernel, HGDIOBJ hUser);
HGDIOBJ MapUserHandle(HGDIOBJ hUser);
VOID RemoveHandleMapping(HGDIOBJ hUser);
VOID CleanupHandleMapping();

/* gdidrv.c */
void CDECL RosDrv_SetDeviceClipping( NTDRV_PDEVICE *physDev, HRGN vis_rgn, HRGN clip_rgn );

/* graphics.c */
INT RosDrv_XWStoDS( NTDRV_PDEVICE *physDev, INT width );
INT RosDrv_YWStoDS( NTDRV_PDEVICE *physDev, INT height );

HGDIOBJ MapHandle(HGDIOBJ hUser);

/* font.c */
VOID
FeSelectFont(NTDRV_PDEVICE *physDev, HFONT hFont);

BOOL
FeTextOut( NTDRV_PDEVICE *physDev, INT x, INT y, UINT flags,
           const RECT *lprect, LPCWSTR wstr, UINT count,
           const INT *lpDx );

/* mouse.c */
void set_window_cursor( HWND hwnd, HCURSOR handle );

void NTDRV_SendMouseInput( HWND hwnd, DWORD flags, DWORD x, DWORD y,
                              DWORD data, DWORD time, DWORD extra_info, UINT injected_flags );

void NTDRV_SendKeyboardInput( WORD wVk, WORD wScan, DWORD event_flags, DWORD time,
                                 DWORD dwExtraInfo, UINT injected_flags );

BOOL CDECL RosDrv_SetCursorPos( INT x, INT y );

LRESULT HOOK_CallHooks( INT id, INT code, WPARAM wparam, LPARAM lparam, BOOL unicode );

BOOL CDECL RosDrv_GetCursorPos( LPPOINT pt );

/* window.c */
struct ntdrv_win_data *NTDRV_get_win_data( HWND hwnd );
struct ntdrv_win_data *NTDRV_create_win_data( HWND hwnd );
struct ntdrv_win_data *NTDRV_create_desktop_win_data( HWND hwnd );
void NTDRV_destroy_win_data( HWND hwnd );
VOID CDECL RosDrv_UpdateZOrder(HWND hwnd, RECT *rect);
void map_window( struct ntdrv_win_data *data, DWORD new_style );
void unmap_window( struct ntdrv_win_data *data );
BOOL is_window_rect_mapped( const RECT *rect );
void sync_window_position( struct ntdrv_win_data *data,
                           UINT swp_flags, const RECT *old_window_rect,
                           const RECT *old_whole_rect, const RECT *old_client_rect );
GR_WINDOW_ID create_whole_window( struct ntdrv_win_data *data );
