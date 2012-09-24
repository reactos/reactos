/*
 * Window definitions
 *
 * Copyright 1993 Alexandre Julliard
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

#ifndef __WINE_WIN_H
#define __WINE_WIN_H

#include <stdarg.h>
#include <windef.h>
#include <winbase.h>
#include <wingdi.h>
#include <winuser.h>

#include "user_private.h"
#include "wine/server_protocol.h"

struct tagCLASS;
struct tagDIALOGINFO;

typedef struct tagWND
{
    struct user_object obj;       /* object header */
    HWND           parent;        /* Window parent */
    HWND           owner;         /* Window owner */
    struct tagCLASS *class;       /* Window class */
    struct dce    *dce;           /* DCE pointer */
    WNDPROC        winproc;       /* Window procedure */
    DWORD          tid;           /* Owner thread id */
    HINSTANCE      hInstance;     /* Window hInstance (from CreateWindow) */
    RECT           rectClient;    /* Client area rel. to parent client area */
    RECT           rectWindow;    /* Whole window rel. to parent client area */
    RECT           normal_rect;   /* Normal window rect saved when maximized/minimized */
    POINT          min_pos;       /* Position for minimized window */
    POINT          max_pos;       /* Position for maximized window */
    HWND           icon_title;    /* Icon title window */
    LPWSTR         text;          /* Window text */
    void          *pScroll;       /* Scroll-bar info */
    DWORD          dwStyle;       /* Window style (from CreateWindow) */
    DWORD          dwExStyle;     /* Extended style (from CreateWindowEx) */
    UINT_PTR       wIDmenu;       /* ID or hmenu (from CreateWindow) */
    DWORD          helpContext;   /* Help context ID */
    UINT           flags;         /* Misc. flags (see below) */
    HMENU          hSysMenu;      /* window's copy of System Menu */
    HICON          hIcon;         /* window's icon */
    HICON          hIconSmall;    /* window's small icon */
    struct tagDIALOGINFO *dlgInfo;/* Dialog additional info (dialogs only) */
    int            cbWndExtra;    /* class cbWndExtra at window creation */
    DWORD_PTR      userdata;      /* User private data */
    DWORD          wExtra[1];     /* Window extra bytes */
} WND;

  /* WND flags values */
#define WIN_RESTORE_MAX           0x0001 /* Maximize when restoring */
#define WIN_NEED_SIZE             0x0002 /* Internal WM_SIZE is needed */
#define WIN_NCACTIVATED           0x0004 /* last WM_NCACTIVATE was positive */
#define WIN_ISMDICLIENT           0x0008 /* Window is an MDIClient */
#define WIN_ISUNICODE             0x0010 /* Window is Unicode */
#define WIN_NEEDS_SHOW_OWNEDPOPUP 0x0020 /* WM_SHOWWINDOW:SC_SHOW must be sent in the next ShowOwnedPopup call */
#define WIN_CHILDREN_MOVED        0x0040 /* children may have moved, ignore stored positions */

  /* Window functions */
extern HWND get_hwnd_message_parent(void) DECLSPEC_HIDDEN;
extern BOOL is_desktop_window( HWND hwnd ) DECLSPEC_HIDDEN;
extern WND *WIN_GetPtr( HWND hwnd ) DECLSPEC_HIDDEN;
extern HWND WIN_GetFullHandle( HWND hwnd ) DECLSPEC_HIDDEN;
extern HWND WIN_IsCurrentProcess( HWND hwnd ) DECLSPEC_HIDDEN;
extern HWND WIN_IsCurrentThread( HWND hwnd ) DECLSPEC_HIDDEN;
extern HWND WIN_SetOwner( HWND hwnd, HWND owner ) DECLSPEC_HIDDEN;
extern ULONG WIN_SetStyle( HWND hwnd, ULONG set_bits, ULONG clear_bits ) DECLSPEC_HIDDEN;
extern BOOL WIN_GetRectangles( HWND hwnd, enum coords_relative relative, RECT *rectWindow, RECT *rectClient ) DECLSPEC_HIDDEN;
extern void map_window_region( HWND from, HWND to, HRGN hrgn ) DECLSPEC_HIDDEN;
extern LRESULT WIN_DestroyWindow( HWND hwnd ) DECLSPEC_HIDDEN;
extern void WIN_DestroyThreadWindows( HWND hwnd ) DECLSPEC_HIDDEN;
extern HWND WIN_CreateWindowEx( CREATESTRUCTW *cs, LPCWSTR className, HINSTANCE module, BOOL unicode ) DECLSPEC_HIDDEN;
extern BOOL WIN_IsWindowDrawable( HWND hwnd, BOOL ) DECLSPEC_HIDDEN;
extern HWND *WIN_ListChildren( HWND hwnd ) DECLSPEC_HIDDEN;
extern LONG_PTR WIN_SetWindowLong( HWND hwnd, INT offset, UINT size, LONG_PTR newval, BOOL unicode ) DECLSPEC_HIDDEN;
extern void MDI_CalcDefaultChildPos( HWND hwndClient, INT total, LPPOINT lpPos, INT delta, UINT *id ) DECLSPEC_HIDDEN;
extern HDESK open_winstation_desktop( HWINSTA hwinsta, LPCWSTR name, DWORD flags, BOOL inherit, ACCESS_MASK access ) DECLSPEC_HIDDEN;

/* user lock */
extern void USER_Lock(void) DECLSPEC_HIDDEN;
extern void USER_Unlock(void) DECLSPEC_HIDDEN;

/* to release pointers retrieved by WIN_GetPtr */
static inline void WIN_ReleasePtr( WND *ptr )
{
    USER_Unlock();
}

#define WND_OTHER_PROCESS ((WND *)1)  /* returned by WIN_GetPtr on unknown window handles */
#define WND_DESKTOP       ((WND *)2)  /* returned by WIN_GetPtr on the desktop window */

extern LRESULT HOOK_CallHooks( INT id, INT code, WPARAM wparam, LPARAM lparam, BOOL unicode ) DECLSPEC_HIDDEN;

extern BOOL WINPOS_RedrawIconTitle( HWND hWnd ) DECLSPEC_HIDDEN;
extern void WINPOS_GetMinMaxInfo( HWND hwnd, POINT *maxSize, POINT *maxPos, POINT *minTrack,
                                  POINT *maxTrack ) DECLSPEC_HIDDEN;
extern LONG WINPOS_HandleWindowPosChanging(HWND hwnd, WINDOWPOS *winpos) DECLSPEC_HIDDEN;
extern HWND WINPOS_WindowFromPoint( HWND hwndScope, POINT pt, INT *hittest ) DECLSPEC_HIDDEN;
extern void WINPOS_ActivateOtherWindow( HWND hwnd ) DECLSPEC_HIDDEN;
extern UINT WINPOS_MinMaximize( HWND hwnd, UINT cmd, LPRECT rect ) DECLSPEC_HIDDEN;
extern void WINPOS_SysCommandSizeMove( HWND hwnd, WPARAM wParam ) DECLSPEC_HIDDEN;

extern BOOL set_window_pos( HWND hwnd, HWND insert_after, UINT swp_flags,
                            const RECT *window_rect, const RECT *client_rect,
                            const RECT *valid_rects ) DECLSPEC_HIDDEN;

static inline void mirror_rect( const RECT *window_rect, RECT *rect )
{
    int width = window_rect->right - window_rect->left;
    int tmp = rect->left;
    rect->left = width - rect->right;
    rect->right = width - tmp;
}

#endif  /* __WINE_WIN_H */
