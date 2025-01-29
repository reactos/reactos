/*
 * Window related functions
 *
 * Copyright 1993, 1994 Alexandre Julliard
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

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "user_private.h"
#include "controls.h"
#include "winver.h"
#include "wine/asm.h"
#include "wine/exception.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(win);


#ifdef __i386__
/* Some apps pass a non-stdcall proc to EnumChildWindows,
 * so we need a small assembly wrapper to call the proc.
 */
extern LRESULT enum_callback_wrapper( WNDENUMPROC proc, HWND hwnd, LPARAM lparam );
__ASM_GLOBAL_FUNC( enum_callback_wrapper,
    "pushl %ebp\n\t"
    __ASM_CFI(".cfi_adjust_cfa_offset 4\n\t")
    __ASM_CFI(".cfi_rel_offset %ebp,0\n\t")
    "movl %esp,%ebp\n\t"
    __ASM_CFI(".cfi_def_cfa_register %ebp\n\t")
    "pushl 16(%ebp)\n\t"
    "pushl 12(%ebp)\n\t"
    "call *8(%ebp)\n\t"
    "leave\n\t"
    __ASM_CFI(".cfi_def_cfa %esp,4\n\t")
    __ASM_CFI(".cfi_same_value %ebp\n\t")
    "ret" )
#else
static inline LRESULT enum_callback_wrapper( WNDENUMPROC proc, HWND hwnd, LPARAM lparam )
{
    return proc( hwnd, lparam );
}
#endif /* __i386__ */

/*******************************************************************
 *           enum_windows
 */
static BOOL enum_windows( HDESK desktop, HWND hwnd, DWORD tid, BOOL children,
                          WNDENUMPROC proc, LPARAM param )
{
    HWND *list;
    ULONG i, size = 128;
    BOOL ret = !children;  /* EnumChildWindows returns FALSE on empty list, the others TRUE */
    NTSTATUS status;

    for (;;)
    {
        if (!(list = HeapAlloc( GetProcessHeap(), 0, size * sizeof(HWND) ))) return FALSE;
        status = NtUserBuildHwndList( desktop, hwnd, children, TRUE, tid, size, list, &size );
        if (!status) break;
        HeapFree( GetProcessHeap(), 0, list );
        if (status != STATUS_BUFFER_TOO_SMALL) return FALSE;
    }
    for (i = 0; i < size && list[i] != HWND_BOTTOM; i++)
    {
        if (!IsWindow( list[i] )) continue;
        if (!(ret = enum_callback_wrapper( proc, list[i], param ))) break;
    }
    HeapFree( GetProcessHeap(), 0, list );
    return ret;
}


/*******************************************************************
 *           is_desktop_window
 *
 * Check if window is the desktop or the HWND_MESSAGE top parent.
 */
BOOL is_desktop_window( HWND hwnd )
{
    struct ntuser_thread_info *thread_info = NtUserGetThreadInfo();

    if (!hwnd) return FALSE;
    if (hwnd == UlongToHandle( thread_info->top_window )) return TRUE;
    if (hwnd == UlongToHandle( thread_info->msg_window )) return TRUE;

    if (!HIWORD(hwnd) || HIWORD(hwnd) == 0xffff)
    {
        if (LOWORD(thread_info->top_window) == LOWORD(hwnd)) return TRUE;
        if (LOWORD(thread_info->msg_window) == LOWORD(hwnd)) return TRUE;
    }
    return FALSE;
}


/* check if hwnd is a broadcast magic handle */
static inline BOOL is_broadcast( HWND hwnd )
{
    return hwnd == HWND_BROADCAST || hwnd == HWND_TOPMOST;
}


/***********************************************************************
 *           WIN_IsCurrentProcess
 *
 * Check whether a given window belongs to the current process (and return the full handle).
 */
HWND WIN_IsCurrentProcess( HWND hwnd )
{
    return UlongToHandle( NtUserCallHwnd( hwnd, NtUserIsCurrentProcessWindow ));
}


/***********************************************************************
 *           WIN_IsCurrentThread
 *
 * Check whether a given window belongs to the current thread (and return the full handle).
 */
HWND WIN_IsCurrentThread( HWND hwnd )
{
    return UlongToHandle( NtUserCallHwnd( hwnd, NtUserIsCurrentThreadWindow ));
}


/***********************************************************************
 *           WIN_GetFullHandle
 *
 * Convert a possibly truncated window handle to a full 32-bit handle.
 */
HWND WIN_GetFullHandle( HWND hwnd )
{
    return UlongToHandle( NtUserCallHwnd( hwnd, NtUserGetFullWindowHandle ));
}


/***********************************************************************
 *           WIN_SetStyle
 *
 * Change the style of a window.
 */
ULONG WIN_SetStyle( HWND hwnd, ULONG set_bits, ULONG clear_bits )
{
    /* FIXME: Use SetWindowLong or move callers to win32u instead.
     * We use STYLESTRUCT to pass params, but meaning of its field does not match our usage. */
    STYLESTRUCT style = { .styleNew = set_bits, .styleOld = clear_bits };
    return NtUserCallHwndParam( hwnd, (UINT_PTR)&style, NtUserSetWindowStyle );
}


/***********************************************************************
 *           dump_window_styles
 */
static void dump_window_styles( DWORD style, DWORD exstyle )
{
    TRACE( "style:" );
    if(style & WS_POPUP) TRACE(" WS_POPUP");
    if(style & WS_CHILD) TRACE(" WS_CHILD");
    if(style & WS_MINIMIZE) TRACE(" WS_MINIMIZE");
    if(style & WS_VISIBLE) TRACE(" WS_VISIBLE");
    if(style & WS_DISABLED) TRACE(" WS_DISABLED");
    if(style & WS_CLIPSIBLINGS) TRACE(" WS_CLIPSIBLINGS");
    if(style & WS_CLIPCHILDREN) TRACE(" WS_CLIPCHILDREN");
    if(style & WS_MAXIMIZE) TRACE(" WS_MAXIMIZE");
    if((style & WS_CAPTION) == WS_CAPTION) TRACE(" WS_CAPTION");
    else
    {
        if(style & WS_BORDER) TRACE(" WS_BORDER");
        if(style & WS_DLGFRAME) TRACE(" WS_DLGFRAME");
    }
    if(style & WS_VSCROLL) TRACE(" WS_VSCROLL");
    if(style & WS_HSCROLL) TRACE(" WS_HSCROLL");
    if(style & WS_SYSMENU) TRACE(" WS_SYSMENU");
    if(style & WS_THICKFRAME) TRACE(" WS_THICKFRAME");
    if (style & WS_CHILD)
    {
        if(style & WS_GROUP) TRACE(" WS_GROUP");
        if(style & WS_TABSTOP) TRACE(" WS_TABSTOP");
    }
    else
    {
        if(style & WS_MINIMIZEBOX) TRACE(" WS_MINIMIZEBOX");
        if(style & WS_MAXIMIZEBOX) TRACE(" WS_MAXIMIZEBOX");
    }

    /* FIXME: Add dumping of BS_/ES_/SBS_/LBS_/CBS_/DS_/etc. styles */
#define DUMPED_STYLES \
    ((DWORD)(WS_POPUP | \
     WS_CHILD | \
     WS_MINIMIZE | \
     WS_VISIBLE | \
     WS_DISABLED | \
     WS_CLIPSIBLINGS | \
     WS_CLIPCHILDREN | \
     WS_MAXIMIZE | \
     WS_BORDER | \
     WS_DLGFRAME | \
     WS_VSCROLL | \
     WS_HSCROLL | \
     WS_SYSMENU | \
     WS_THICKFRAME | \
     WS_GROUP | \
     WS_TABSTOP | \
     WS_MINIMIZEBOX | \
     WS_MAXIMIZEBOX))

    if(style & ~DUMPED_STYLES) TRACE(" %08lx", style & ~DUMPED_STYLES);
    TRACE("\n");
#undef DUMPED_STYLES

    TRACE( "exstyle:" );
    if(exstyle & WS_EX_DLGMODALFRAME) TRACE(" WS_EX_DLGMODALFRAME");
    if(exstyle & WS_EX_DRAGDETECT) TRACE(" WS_EX_DRAGDETECT");
    if(exstyle & WS_EX_NOPARENTNOTIFY) TRACE(" WS_EX_NOPARENTNOTIFY");
    if(exstyle & WS_EX_TOPMOST) TRACE(" WS_EX_TOPMOST");
    if(exstyle & WS_EX_ACCEPTFILES) TRACE(" WS_EX_ACCEPTFILES");
    if(exstyle & WS_EX_TRANSPARENT) TRACE(" WS_EX_TRANSPARENT");
    if(exstyle & WS_EX_MDICHILD) TRACE(" WS_EX_MDICHILD");
    if(exstyle & WS_EX_TOOLWINDOW) TRACE(" WS_EX_TOOLWINDOW");
    if(exstyle & WS_EX_WINDOWEDGE) TRACE(" WS_EX_WINDOWEDGE");
    if(exstyle & WS_EX_CLIENTEDGE) TRACE(" WS_EX_CLIENTEDGE");
    if(exstyle & WS_EX_CONTEXTHELP) TRACE(" WS_EX_CONTEXTHELP");
    if(exstyle & WS_EX_RIGHT) TRACE(" WS_EX_RIGHT");
    if(exstyle & WS_EX_RTLREADING) TRACE(" WS_EX_RTLREADING");
    if(exstyle & WS_EX_LEFTSCROLLBAR) TRACE(" WS_EX_LEFTSCROLLBAR");
    if(exstyle & WS_EX_CONTROLPARENT) TRACE(" WS_EX_CONTROLPARENT");
    if(exstyle & WS_EX_STATICEDGE) TRACE(" WS_EX_STATICEDGE");
    if(exstyle & WS_EX_APPWINDOW) TRACE(" WS_EX_APPWINDOW");
    if(exstyle & WS_EX_LAYERED) TRACE(" WS_EX_LAYERED");
    if(exstyle & WS_EX_NOINHERITLAYOUT) TRACE(" WS_EX_NOINHERITLAYOUT");
    if(exstyle & WS_EX_LAYOUTRTL) TRACE(" WS_EX_LAYOUTRTL");
    if(exstyle & WS_EX_COMPOSITED) TRACE(" WS_EX_COMPOSITED");
    if(exstyle & WS_EX_NOACTIVATE) TRACE(" WS_EX_NOACTIVATE");

#define DUMPED_EX_STYLES \
    ((DWORD)(WS_EX_DLGMODALFRAME | \
     WS_EX_DRAGDETECT | \
     WS_EX_NOPARENTNOTIFY | \
     WS_EX_TOPMOST | \
     WS_EX_ACCEPTFILES | \
     WS_EX_TRANSPARENT | \
     WS_EX_MDICHILD | \
     WS_EX_TOOLWINDOW | \
     WS_EX_WINDOWEDGE | \
     WS_EX_CLIENTEDGE | \
     WS_EX_CONTEXTHELP | \
     WS_EX_RIGHT | \
     WS_EX_RTLREADING | \
     WS_EX_LEFTSCROLLBAR | \
     WS_EX_CONTROLPARENT | \
     WS_EX_STATICEDGE | \
     WS_EX_APPWINDOW | \
     WS_EX_LAYERED | \
     WS_EX_NOINHERITLAYOUT | \
     WS_EX_LAYOUTRTL | \
     WS_EX_COMPOSITED |\
     WS_EX_NOACTIVATE))

    if(exstyle & ~DUMPED_EX_STYLES) TRACE(" %08lx", exstyle & ~DUMPED_EX_STYLES);
    TRACE("\n");
#undef DUMPED_EX_STYLES
}

static BOOL is_default_coord( int x )
{
    return x == CW_USEDEFAULT || x == 0x8000;
}

/***********************************************************************
 *           WIN_CreateWindowEx
 *
 * Implementation of CreateWindowEx().
 */
HWND WIN_CreateWindowEx( CREATESTRUCTW *cs, LPCWSTR className, HINSTANCE module, BOOL unicode )
{
    UNICODE_STRING class, window_name = {0};
    HWND hwnd, top_child = 0;
    MDICREATESTRUCTW mdi_cs;
    WNDCLASSEXW info;
    WCHAR name_buf[8];
    HMENU menu;

    if (!get_class_info( module, className, &info, &class, FALSE )) return FALSE;

    TRACE("%s %s%s%s ex=%08lx style=%08lx %d,%d %dx%d parent=%p menu=%p inst=%p params=%p\n",
          unicode ? debugstr_w(cs->lpszName) : debugstr_a((LPCSTR)cs->lpszName),
          debugstr_w(className), class.Buffer != className ? "->" : "",
          class.Buffer != className ? debugstr_wn(class.Buffer, class.Length / sizeof(WCHAR)) : "",
          cs->dwExStyle, cs->style, cs->x, cs->y, cs->cx, cs->cy,
          cs->hwndParent, cs->hMenu, cs->hInstance, cs->lpCreateParams );
    if(TRACE_ON(win)) dump_window_styles( cs->style, cs->dwExStyle );

    /* Fix the styles for MDI children */
    if (cs->dwExStyle & WS_EX_MDICHILD)
    {
        POINT pos[2];
        UINT id = 0;

        if (!NtUserGetMDIClientInfo( cs->hwndParent ))
        {
            WARN("WS_EX_MDICHILD, but parent %p is not MDIClient\n", cs->hwndParent);
            return 0;
        }

        /* cs->lpCreateParams of WM_[NC]CREATE is different for MDI children.
         * MDICREATESTRUCT members have the originally passed values.
         *
         * Note: we rely on the fact that MDICREATESTRUCTA and MDICREATESTRUCTW
         * have the same layout.
         */
        mdi_cs.szClass = cs->lpszClass;
        mdi_cs.szTitle = cs->lpszName;
        mdi_cs.hOwner = cs->hInstance;
        mdi_cs.x = cs->x;
        mdi_cs.y = cs->y;
        mdi_cs.cx = cs->cx;
        mdi_cs.cy = cs->cy;
        mdi_cs.style = cs->style;
        mdi_cs.lParam = (LPARAM)cs->lpCreateParams;

        cs->lpCreateParams = &mdi_cs;

        if (GetWindowLongW(cs->hwndParent, GWL_STYLE) & MDIS_ALLCHILDSTYLES)
        {
            if (cs->style & WS_POPUP)
            {
                TRACE("WS_POPUP with MDIS_ALLCHILDSTYLES is not allowed\n");
                return 0;
            }
            cs->style |= WS_CHILD | WS_CLIPSIBLINGS;
        }
        else
        {
            cs->style &= ~WS_POPUP;
            cs->style |= WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CAPTION |
                WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX;
        }

        top_child = GetWindow(cs->hwndParent, GW_CHILD);

        if (top_child)
        {
            /* Restore current maximized child */
            if((cs->style & WS_VISIBLE) && IsZoomed(top_child))
            {
                TRACE("Restoring current maximized child %p\n", top_child);
                if (cs->style & WS_MAXIMIZE)
                {
                    /* if the new window is maximized don't bother repainting */
                    SendMessageW( top_child, WM_SETREDRAW, FALSE, 0 );
                    NtUserShowWindow( top_child, SW_SHOWNORMAL );
                    SendMessageW( top_child, WM_SETREDRAW, TRUE, 0 );
                }
                else NtUserShowWindow( top_child, SW_SHOWNORMAL );
            }
        }

        MDI_CalcDefaultChildPos( cs->hwndParent, -1, pos, 0, &id );
        if (!(cs->style & WS_POPUP)) cs->hMenu = ULongToHandle(id);

        TRACE( "MDI child id %04x\n", id );

        if (cs->style & (WS_CHILD | WS_POPUP))
        {
            if (is_default_coord( cs->x ))
            {
                cs->x = pos[0].x;
                cs->y = pos[0].y;
            }
            if (is_default_coord( cs->cx ) || !cs->cx) cs->cx = pos[1].x;
            if (is_default_coord( cs->cy ) || !cs->cy) cs->cy = pos[1].y;
        }
    }

    if (!unicode && cs->lpszName)
    {
        const char *nameA = (const char *)cs->lpszName;
        /* resource ID string is a special case */
        if (nameA[0] == '\xff')
        {
            name_buf[0] = 0xffff;
            name_buf[1] = MAKEWORD( nameA[1], nameA[2] );
            name_buf[2] = 0;
            RtlInitUnicodeString( &window_name, name_buf );
        }
        else if (!RtlCreateUnicodeStringFromAsciiz( &window_name, (const char *)cs->lpszName ))
            return 0;
    }
    else RtlInitUnicodeString( &window_name, cs->lpszName );

    menu = cs->hMenu;
    if (!menu && info.lpszMenuName && (cs->style & (WS_CHILD | WS_POPUP)) != WS_CHILD)
        menu = LoadMenuW( cs->hInstance, info.lpszMenuName );

    hwnd = NtUserCreateWindowEx( cs->dwExStyle, &class, NULL, &window_name, cs->style,
                                 cs->x, cs->y, cs->cx, cs->cy, cs->hwndParent, menu, module,
                                 cs->lpCreateParams, 0, cs->hInstance, 0, !unicode );
    if (!hwnd && menu && menu != cs->hMenu) NtUserDestroyMenu( menu );
    if (!unicode && window_name.Buffer != name_buf) RtlFreeUnicodeString( &window_name );
    return hwnd;
}


/***********************************************************************
 *		CreateWindowExA (USER32.@)
 */
HWND WINAPI DECLSPEC_HOTPATCH CreateWindowExA( DWORD exStyle, LPCSTR className,
                                 LPCSTR windowName, DWORD style, INT x,
                                 INT y, INT width, INT height,
                                 HWND parent, HMENU menu,
                                 HINSTANCE instance, LPVOID data )
{
    CREATESTRUCTA cs;

    cs.lpCreateParams = data;
    cs.hInstance      = instance;
    cs.hMenu          = menu;
    cs.hwndParent     = parent;
    cs.x              = x;
    cs.y              = y;
    cs.cx             = width;
    cs.cy             = height;
    cs.style          = style;
    cs.lpszName       = windowName;
    cs.lpszClass      = className;
    cs.dwExStyle      = exStyle;

    if (!IS_INTRESOURCE(className))
    {
        WCHAR bufferW[256];
        if (!MultiByteToWideChar( CP_ACP, 0, className, -1, bufferW, ARRAY_SIZE( bufferW )))
            return 0;
        return wow_handlers.create_window( (CREATESTRUCTW *)&cs, bufferW, instance, FALSE );
    }
    /* Note: we rely on the fact that CREATESTRUCTA and */
    /* CREATESTRUCTW have the same layout. */
    return wow_handlers.create_window( (CREATESTRUCTW *)&cs, (LPCWSTR)className, instance, FALSE );
}


/***********************************************************************
 *		CreateWindowExW (USER32.@)
 */
HWND WINAPI DECLSPEC_HOTPATCH CreateWindowExW( DWORD exStyle, LPCWSTR className,
                                 LPCWSTR windowName, DWORD style, INT x,
                                 INT y, INT width, INT height,
                                 HWND parent, HMENU menu,
                                 HINSTANCE instance, LPVOID data )
{
    CREATESTRUCTW cs;

    cs.lpCreateParams = data;
    cs.hInstance      = instance;
    cs.hMenu          = menu;
    cs.hwndParent     = parent;
    cs.x              = x;
    cs.y              = y;
    cs.cx             = width;
    cs.cy             = height;
    cs.style          = style;
    cs.lpszName       = windowName;
    cs.lpszClass      = className;
    cs.dwExStyle      = exStyle;

    return wow_handlers.create_window( &cs, className, instance, TRUE );
}


/***********************************************************************
 *		CloseWindow (USER32.@)
 */
BOOL WINAPI CloseWindow( HWND hwnd )
{
    if (GetWindowLongW( hwnd, GWL_STYLE ) & WS_CHILD) return FALSE;
    NtUserShowWindow( hwnd, SW_MINIMIZE );
    return TRUE;
}


/***********************************************************************
 *		OpenIcon (USER32.@)
 */
BOOL WINAPI OpenIcon( HWND hwnd )
{
    if (!IsIconic( hwnd )) return FALSE;
    NtUserShowWindow( hwnd, SW_SHOWNORMAL );
    return TRUE;
}


/***********************************************************************
 *		FindWindowExW (USER32.@)
 */
HWND WINAPI FindWindowExW( HWND parent, HWND child, const WCHAR *class, const WCHAR *title )
{
    UNICODE_STRING class_str, title_str;

    if (title) RtlInitUnicodeString( &title_str, title );

    if (class)
    {
        if (IS_INTRESOURCE(class))
        {
            class_str.Buffer = (WCHAR *)class;
            class_str.Length = class_str.MaximumLength = 0;
        }
        else RtlInitUnicodeString( &class_str, class );
    }

    return NtUserFindWindowEx( parent, child, class ? &class_str : NULL,
                               title ? &title_str : NULL, 0 );
}



/***********************************************************************
 *		FindWindowA (USER32.@)
 */
HWND WINAPI FindWindowA( LPCSTR className, LPCSTR title )
{
    HWND ret = FindWindowExA( 0, 0, className, title );
    if (!ret) SetLastError (ERROR_CANNOT_FIND_WND_CLASS);
    return ret;
}


/***********************************************************************
 *		FindWindowExA (USER32.@)
 */
HWND WINAPI FindWindowExA( HWND parent, HWND child, LPCSTR className, LPCSTR title )
{
    LPWSTR titleW = NULL;
    HWND hwnd = 0;

    if (title)
    {
        DWORD len = MultiByteToWideChar( CP_ACP, 0, title, -1, NULL, 0 );
        if (!(titleW = HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR) ))) return 0;
        MultiByteToWideChar( CP_ACP, 0, title, -1, titleW, len );
    }

    if (!IS_INTRESOURCE(className))
    {
        WCHAR classW[256];
        if (MultiByteToWideChar( CP_ACP, 0, className, -1, classW, ARRAY_SIZE( classW )))
            hwnd = FindWindowExW( parent, child, classW, titleW );
    }
    else
    {
        hwnd = FindWindowExW( parent, child, (LPCWSTR)className, titleW );
    }

    HeapFree( GetProcessHeap(), 0, titleW );
    return hwnd;
}


/***********************************************************************
 *		FindWindowW (USER32.@)
 */
HWND WINAPI FindWindowW( LPCWSTR className, LPCWSTR title )
{
    return FindWindowExW( 0, 0, className, title );
}


/**********************************************************************
 *		GetDesktopWindow (USER32.@)
 */
HWND WINAPI GetDesktopWindow(void)
{
    struct ntuser_thread_info *thread_info = NtUserGetThreadInfo();

    if (thread_info->top_window) return UlongToHandle( thread_info->top_window );
    return NtUserGetDesktopWindow();
}


/*******************************************************************
 *		EnableWindow (USER32.@)
 */
BOOL WINAPI EnableWindow( HWND hwnd, BOOL enable )
{
    return NtUserEnableWindow( hwnd, enable );
}


/***********************************************************************
 *		IsWindowEnabled (USER32.@)
 */
BOOL WINAPI IsWindowEnabled( HWND hwnd )
{
    return NtUserIsWindowEnabled( hwnd );
}

/***********************************************************************
 *		IsWindowUnicode (USER32.@)
 */
BOOL WINAPI IsWindowUnicode( HWND hwnd )
{
    return NtUserIsWindowUnicode( hwnd );
}


/***********************************************************************
 *		GetWindowDpiAwarenessContext  (USER32.@)
 */
DPI_AWARENESS_CONTEXT WINAPI GetWindowDpiAwarenessContext( HWND hwnd )
{
    return LongToHandle( NtUserGetWindowDpiAwarenessContext( hwnd ) );
}

/***********************************************************************
 *		GetDpiAwarenessContextForProcess  (USER32.@)
 */
DPI_AWARENESS_CONTEXT WINAPI GetDpiAwarenessContextForProcess(HANDLE process)
{
    return LongToHandle( NtUserGetProcessDpiAwarenessContext( process ) );
}

/***********************************************************************
 *		GetWindowDpiHostingBehavior  (USER32.@)
 */
DPI_HOSTING_BEHAVIOR WINAPI GetWindowDpiHostingBehavior( HWND hwnd )
{
    FIXME("(%p): stub\n", hwnd);
    return DPI_HOSTING_BEHAVIOR_DEFAULT;
}


static LONG_PTR get_window_long_ptr( HWND hwnd, int offset, LONG_PTR ret, BOOL ansi )
{
    if (offset == DWLP_DLGPROC && NtUserGetDialogInfo( hwnd ))
    {
        DLGPROC proc = NtUserGetDialogProc( (DLGPROC)ret, ansi );
        if (proc && proc != WINPROC_PROC16) return (LONG_PTR)proc;
    }
    return ret;
}


static LONG_PTR set_dialog_proc( HWND hwnd, LONG_PTR newval, BOOL ansi )
{
    DLGPROC proc;
    LONG_PTR ret;
    newval = NtUserCallTwoParam( newval, ansi, NtUserAllocWinProc );
    ret = NtUserSetWindowLongPtr( hwnd, DWLP_DLGPROC, newval, ansi );
    proc = NtUserGetDialogProc( (DLGPROC)ret, ansi );
    if (proc) ret = (UINT_PTR)proc;
    return ret;
}


/***********************************************************************
 *              GetDpiForWindow   (USER32.@)
 */
UINT WINAPI GetDpiForWindow( HWND hwnd )
{
    return NtUserGetDpiForWindow( hwnd );
}


/***********************************************************************
 *           SwitchToThisWindow (USER32.@)
 */
void WINAPI SwitchToThisWindow( HWND hwnd, BOOL alt_tab )
{
    if (IsIconic( hwnd )) NtUserShowWindow( hwnd, SW_RESTORE );
    else BringWindowToTop( hwnd );
}


/***********************************************************************
 *           GetWindowRect (USER32.@)
 */
BOOL WINAPI GetWindowRect( HWND hwnd, RECT *rect )
{
    UINT dpi = NTUSER_DPI_CONTEXT_GET_DPI( (UINT_PTR)GetThreadDpiAwarenessContext() );
    BOOL ret = NtUserGetWindowRect( hwnd, rect, dpi );
    if (ret) TRACE( "hwnd %p %s\n", hwnd, wine_dbgstr_rect(rect) );
    return ret;
}


/***********************************************************************
 *           GetWindowRgn (USER32.@)
 */
int WINAPI GetWindowRgn( HWND hwnd, HRGN hrgn )
{
    return NtUserGetWindowRgnEx( hwnd, hrgn, 0 );
}


/***********************************************************************
 *           GetWindowRgnBox (USER32.@)
 */
int WINAPI GetWindowRgnBox( HWND hwnd, RECT *rect )
{
    int ret = ERROR;
    HRGN hrgn;

    if (!rect)
        return ERROR;

    if ((hrgn = CreateRectRgn( 0, 0, 0, 0 )))
    {
        if ((ret = GetWindowRgn( hwnd, hrgn )) != ERROR )
            ret = GetRgnBox( hrgn, rect );
        DeleteObject( hrgn );
    }

    return ret;
}


/***********************************************************************
 *           GetClientRect (USER32.@)
 */
BOOL WINAPI GetClientRect( HWND hwnd, RECT *rect )
{
    UINT dpi = NTUSER_DPI_CONTEXT_GET_DPI( (UINT_PTR)GetThreadDpiAwarenessContext() );
    return NtUserGetClientRect( hwnd, rect, dpi );
}


/*******************************************************************
 *           WindowFromPoint (USER32.@)
 */
HWND WINAPI WindowFromPoint( POINT pt )
{
    return NtUserWindowFromPoint( pt.x, pt.y );
}


/*******************************************************************
 *           ChildWindowFromPoint (USER32.@)
 */
HWND WINAPI ChildWindowFromPoint( HWND parent, POINT pt )
{
    return NtUserChildWindowFromPointEx( parent, pt.x, pt.y, CWP_ALL );
}

/*******************************************************************
 *           RealChildWindowFromPoint (USER32.@)
 */
HWND WINAPI RealChildWindowFromPoint( HWND parent, POINT pt )
{
    return NtUserRealChildWindowFromPoint( parent, pt.x, pt.y );
}

/*******************************************************************
 *           ChildWindowFromPointEx (USER32.@)
 */
HWND WINAPI ChildWindowFromPointEx( HWND parent, POINT pt, UINT flags )
{
    return NtUserChildWindowFromPointEx( parent, pt.x, pt.y, flags );
}


/*******************************************************************
 *           MapWindowPoints (USER32.@)
 */
INT WINAPI MapWindowPoints( HWND hwnd_from, HWND hwnd_to, POINT *points, UINT count )
{
    UINT dpi = NTUSER_DPI_CONTEXT_GET_DPI( (UINT_PTR)GetThreadDpiAwarenessContext() );
    return NtUserMapWindowPoints( hwnd_from, hwnd_to, points, count, dpi );
}


/*******************************************************************
 *           ClientToScreen (USER32.@)
 */
BOOL WINAPI ClientToScreen( HWND hwnd, POINT *pt )
{
    return NtUserClientToScreen( hwnd, pt );
}


/*******************************************************************
 *           ScreenToClient (USER32.@)
 */
BOOL WINAPI ScreenToClient( HWND hwnd, POINT *pt )
{
    return NtUserScreenToClient( hwnd, pt );
}


/***********************************************************************
 *           IsIconic (USER32.@)
 */
BOOL WINAPI IsIconic( HWND hwnd )
{
    return (GetWindowLongW( hwnd, GWL_STYLE ) & WS_MINIMIZE) != 0;
}


/***********************************************************************
 *           IsZoomed (USER32.@)
 */
BOOL WINAPI IsZoomed( HWND hwnd )
{
    return (GetWindowLongW( hwnd, GWL_STYLE ) & WS_MAXIMIZE) != 0;
}


/*******************************************************************
 *           AllowSetForegroundWindow (USER32.@)
 */
BOOL WINAPI AllowSetForegroundWindow( DWORD procid )
{
    /* FIXME: If Win98/2000 style SetForegroundWindow behavior is
     * implemented, then fix this function. */
    return TRUE;
}


/*******************************************************************
 *           LockSetForegroundWindow (USER32.@)
 */
BOOL WINAPI LockSetForegroundWindow( UINT lockcode )
{
    /* FIXME: If Win98/2000 style SetForegroundWindow behavior is
     * implemented, then fix this function. */
    return TRUE;
}


/***********************************************************************
 *           BringWindowToTop (USER32.@)
 */
BOOL WINAPI BringWindowToTop( HWND hwnd )
{
    return NtUserSetWindowPos( hwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE );
}


/***********************************************************************
 *           AnimateWindow (USER32.@)
 */
BOOL WINAPI AnimateWindow( HWND hwnd, DWORD time, DWORD flags )
{
    FIXME( "partial stub\n" );

    /* If trying to show/hide and it's already shown/hidden or invalid window,
     * fail with invalid parameter. */
    if (!IsWindow( hwnd ) || (!(flags & AW_HIDE)) == IsWindowVisible( hwnd ))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    NtUserShowWindow( hwnd, (flags & AW_HIDE) ? SW_HIDE : ((flags & AW_ACTIVATE) ? SW_SHOW : SW_SHOWNA) );
    return TRUE;
}


/***********************************************************************
 *           BeginDeferWindowPos (USER32.@)
 */
HDWP WINAPI BeginDeferWindowPos( INT count )
{
    return NtUserBeginDeferWindowPos( count );
}


/***********************************************************************
 *           DeferWindowPos (USER32.@)
 */
HDWP WINAPI DeferWindowPos( HDWP hdwp, HWND hwnd, HWND after, INT x, INT y,
                            INT cx, INT cy, UINT flags )
{
    return NtUserDeferWindowPosAndBand( hdwp, hwnd, after, x, y, cx, cy, flags, 0, 0 );
}


/***********************************************************************
 *           EndDeferWindowPos (USER32.@)
 */
BOOL WINAPI EndDeferWindowPos( HDWP hdwp )
{
    return NtUserEndDeferWindowPosEx( hdwp, FALSE );
}


/***********************************************************************
 *           ArrangeIconicWindows (USER32.@)
 */
UINT WINAPI ArrangeIconicWindows( HWND parent )
{
    return NtUserArrangeIconicWindows( parent );
}


/**********************************************************************
 *		GetWindowWord (USER32.@)
 */
WORD WINAPI GetWindowWord( HWND hwnd, INT offset )
{
    return NtUserGetWindowWord( hwnd, offset );
}


/**********************************************************************
 *		GetWindowLongA (USER32.@)
 */

#ifdef __i386__

/* This wrapper is here to workaround a ntlea quirk. First of all, ntlea
 * checks whether GetWindowLongA starts with the Win32 hotpatchable prologue,
 * if it can find that, it will use a hooking strategy more difficult for us
 * to deal with. Secondly, it assumes what follows the prologue is a `pushl $-2`,
 * and will try to skip over this instruction when calling `GetWindowLongA`,
 * (i.e. it tries to jump to `GetWindowLongA + 7`, 5 bytes for the prologue, 2
 * bytes for the `pushl`.). We have to anticipate that and make sure the result
 * of doing this won't be a messed up stack, or a desynced PC.
 */
__ASM_STDCALL_FUNC( GetWindowLongA, 8,
        ".byte 0x8b, 0xff, 0x55, 0x8b, 0xec\n" /* Win32 hotpatchable prologue. */
        "pushl $-2\n"
        "addl $4, %esp\n"
        "popl %ebp\n"
        "jmp " __ASM_STDCALL("get_window_longA", 8) )
LONG WINAPI get_window_longA( HWND hwnd, INT offset )
#else
LONG WINAPI DECLSPEC_HOTPATCH GetWindowLongA( HWND hwnd, INT offset )
#endif
{
    switch (offset)
    {
#ifdef _WIN64
    case GWLP_WNDPROC:
    case GWLP_HINSTANCE:
    case GWLP_HWNDPARENT:
        WARN( "Invalid offset %d\n", offset );
        SetLastError( ERROR_INVALID_INDEX );
        return 0;
#endif
    default:
        if (sizeof(void *) == sizeof(LONG))
        {
            LONG_PTR ret = NtUserGetWindowLongA( hwnd, offset );
            return get_window_long_ptr( hwnd, offset, ret, TRUE );
        }
        return NtUserGetWindowLongA( hwnd, offset );
    }
}


/**********************************************************************
 *		GetWindowLongW (USER32.@)
 */
LONG WINAPI GetWindowLongW( HWND hwnd, INT offset )
{
    switch (offset)
    {
#ifdef _WIN64
    case GWLP_WNDPROC:
    case GWLP_HINSTANCE:
    case GWLP_HWNDPARENT:
        WARN( "Invalid offset %d\n", offset );
        SetLastError( ERROR_INVALID_INDEX );
        return 0;
#endif
    default:
        if (sizeof(void *) == sizeof(LONG))
        {
            LONG_PTR ret = NtUserGetWindowLongW( hwnd, offset );
            return get_window_long_ptr( hwnd, offset, ret, FALSE );
        }
        return NtUserGetWindowLongW( hwnd, offset );
    }
}


/**********************************************************************
 *		SetWindowLongA (USER32.@)
 *
 * See SetWindowLongW.
 */
LONG WINAPI DECLSPEC_HOTPATCH SetWindowLongA( HWND hwnd, INT offset, LONG newval )
{
    switch (offset)
    {
#ifdef _WIN64
    case GWLP_WNDPROC:
    case GWLP_HINSTANCE:
    case GWLP_HWNDPARENT:
        WARN( "Invalid offset %d\n", offset );
        SetLastError( ERROR_INVALID_INDEX );
        return 0;
#else
    case DWLP_DLGPROC:
        if (NtUserGetDialogInfo( hwnd )) return set_dialog_proc( hwnd, newval, TRUE );
        /* fall through */
#endif
    default:
        return NtUserSetWindowLong( hwnd, offset, newval, TRUE );
    }
}


/**********************************************************************
 *		SetWindowLongW (USER32.@) Set window attribute
 *
 * SetWindowLong() alters one of a window's attributes or sets a 32-bit (long)
 * value in a window's extra memory.
 *
 * The _hwnd_ parameter specifies the handle to a window that
 * has extra memory. The _newval_ parameter contains the new
 * attribute or extra memory value.  If positive, the _offset_
 * parameter is the byte-addressed location in the window's extra
 * memory to set.  If negative, _offset_ specifies the window
 * attribute to set, and should be one of the following values:
 *
 * GWL_EXSTYLE      The window's extended window style
 *
 * GWL_STYLE        The window's window style.
 *
 * GWLP_WNDPROC     Pointer to the window's window procedure.
 *
 * GWLP_HINSTANCE   The window's application instance handle.
 *
 * GWLP_ID          The window's identifier.
 *
 * GWLP_USERDATA    The window's user-specified data.
 *
 * If the window is a dialog box, the _offset_ parameter can be one of
 * the following values:
 *
 * DWLP_DLGPROC     The address of the window's dialog box procedure.
 *
 * DWLP_MSGRESULT   The return value of a message
 *                  that the dialog box procedure processed.
 *
 * DWLP_USER        Application specific information.
 *
 * RETURNS
 *
 * If successful, returns the previous value located at _offset_. Otherwise,
 * returns 0.
 *
 * NOTES
 *
 * Extra memory for a window class is specified by a nonzero cbWndExtra
 * parameter of the WNDCLASS structure passed to RegisterClass() at the
 * time of class creation.
 *
 * Using GWL_WNDPROC to set a new window procedure effectively creates
 * a window subclass. Use CallWindowProc() in the new windows procedure
 * to pass messages to the superclass's window procedure.
 *
 * The user data is reserved for use by the application which created
 * the window.
 *
 * Do not use GWL_STYLE to change the window's WS_DISABLED style;
 * instead, call the EnableWindow() function to change the window's
 * disabled state.
 *
 * Do not use GWL_HWNDPARENT to reset the window's parent, use
 * SetParent() instead.
 *
 * Win95:
 * When offset is GWL_STYLE and the calling app's ver is 4.0,
 * it sends WM_STYLECHANGING before changing the settings
 * and WM_STYLECHANGED afterwards.
 * App ver 4.0 can't use SetWindowLong to change WS_EX_TOPMOST.
 */
LONG WINAPI DECLSPEC_HOTPATCH SetWindowLongW(
    HWND hwnd,  /* [in] window to alter */
    INT offset, /* [in] offset, in bytes, of location to alter */
    LONG newval /* [in] new value of location */
)
{
    switch (offset)
    {
#ifdef _WIN64
    case GWLP_WNDPROC:
    case GWLP_HINSTANCE:
    case GWLP_HWNDPARENT:
        WARN("Invalid offset %d\n", offset );
        SetLastError( ERROR_INVALID_INDEX );
        return 0;
#else
    case DWLP_DLGPROC:
        if (NtUserGetDialogInfo( hwnd )) return set_dialog_proc( hwnd, newval, FALSE );
        /* fall through */
#endif
    default:
        return NtUserSetWindowLong( hwnd, offset, newval, FALSE );
    }
}


/*******************************************************************
 *		GetWindowTextA (USER32.@)
 */
INT WINAPI GetWindowTextA( HWND hwnd, LPSTR lpString, INT nMaxCount )
{
    WCHAR *buffer;
    int ret = 0;

    if (!lpString || nMaxCount <= 0) return 0;

    __TRY
    {
        lpString[0] = 0;

        if (WIN_IsCurrentProcess( hwnd ))
        {
            ret = (INT)SendMessageA( hwnd, WM_GETTEXT, nMaxCount, (LPARAM)lpString );
        }
        else if ((buffer = HeapAlloc( GetProcessHeap(), 0, nMaxCount * sizeof(WCHAR) )))
        {
            /* when window belongs to other process, don't send a message */
            NtUserInternalGetWindowText( hwnd, buffer, nMaxCount );
            if (!WideCharToMultiByte( CP_ACP, 0, buffer, -1, lpString, nMaxCount, NULL, NULL ))
                lpString[nMaxCount-1] = 0;
            HeapFree( GetProcessHeap(), 0, buffer );
            ret = strlen(lpString);
        }
    }
    __EXCEPT_PAGE_FAULT
    {
        ret = 0;
    }
    __ENDTRY

    return ret;
}


/*******************************************************************
 *		GetWindowTextW (USER32.@)
 */
INT WINAPI GetWindowTextW( HWND hwnd, LPWSTR lpString, INT nMaxCount )
{
    int ret;

    if (!lpString || nMaxCount <= 0) return 0;

    __TRY
    {
        lpString[0] = 0;

        if (WIN_IsCurrentProcess( hwnd ))
        {
            ret = (INT)SendMessageW( hwnd, WM_GETTEXT, nMaxCount, (LPARAM)lpString );
        }
        else
        {
            /* when window belongs to other process, don't send a message */
            ret = NtUserInternalGetWindowText( hwnd, lpString, nMaxCount );
        }
    }
    __EXCEPT_PAGE_FAULT
    {
        ret = 0;
    }
    __ENDTRY

    return ret;
}


/*******************************************************************
 *		SetWindowTextA (USER32.@)
 *		SetWindowText  (USER32.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH SetWindowTextA( HWND hwnd, LPCSTR lpString )
{
    if (is_broadcast(hwnd))
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    if (!WIN_IsCurrentProcess( hwnd ))
        WARN( "setting text %s of other process window %p should not use SendMessage\n",
               debugstr_a(lpString), hwnd );
    return (BOOL)SendMessageA( hwnd, WM_SETTEXT, 0, (LPARAM)lpString );
}


/*******************************************************************
 *		SetWindowTextW (USER32.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH SetWindowTextW( HWND hwnd, LPCWSTR lpString )
{
    if (is_broadcast(hwnd))
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    if (!WIN_IsCurrentProcess( hwnd ))
        WARN( "setting text %s of other process window %p should not use SendMessage\n",
               debugstr_w(lpString), hwnd );
    return (BOOL)SendMessageW( hwnd, WM_SETTEXT, 0, (LPARAM)lpString );
}


/*******************************************************************
 *		GetWindowTextLengthA (USER32.@)
 */
INT WINAPI GetWindowTextLengthA( HWND hwnd )
{
    CPINFO info;

    if (WIN_IsCurrentProcess( hwnd )) return SendMessageA( hwnd, WM_GETTEXTLENGTH, 0, 0 );

    /* when window belongs to other process, don't send a message */
    GetCPInfo( CP_ACP, &info );
    return NtUserGetWindowTextLength( hwnd ) * info.MaxCharSize;
}

/*******************************************************************
 *		GetWindowTextLengthW (USER32.@)
 */
INT WINAPI GetWindowTextLengthW( HWND hwnd )
{
    if (WIN_IsCurrentProcess( hwnd )) return SendMessageW( hwnd, WM_GETTEXTLENGTH, 0, 0 );

    /* when window belongs to other process, don't send a message */
    return NtUserGetWindowTextLength( hwnd );
}


/*******************************************************************
 *		IsWindow (USER32.@)
 */
BOOL WINAPI IsWindow( HWND hwnd )
{
    return NtUserIsWindow( hwnd );
}


/***********************************************************************
 *		GetWindowThreadProcessId (USER32.@)
 */
DWORD WINAPI GetWindowThreadProcessId( HWND hwnd, LPDWORD process )
{
    return NtUserGetWindowThread( hwnd, process );
}


/*****************************************************************
 *		GetParent (USER32.@)
 */
HWND WINAPI GetParent( HWND hwnd )
{
    return NtUserGetParent( hwnd );
}


/*******************************************************************
 *		IsChild (USER32.@)
 */
BOOL WINAPI IsChild( HWND parent, HWND child )
{
    return NtUserIsChild( parent, child );
}


/***********************************************************************
 *		IsWindowVisible (USER32.@)
 */
BOOL WINAPI IsWindowVisible( HWND hwnd )
{
    return NtUserIsWindowVisible( hwnd );
}


/*******************************************************************
 *		GetTopWindow (USER32.@)
 */
HWND WINAPI GetTopWindow( HWND hwnd )
{
    if (!hwnd) hwnd = GetDesktopWindow();
    return GetWindow( hwnd, GW_CHILD );
}


/*******************************************************************
 *		GetWindow (USER32.@)
 */
HWND WINAPI GetWindow( HWND hwnd, UINT rel )
{
    return NtUserGetWindowRelative( hwnd, rel );
}


/*******************************************************************
 *		ShowOwnedPopups (USER32.@)
 */
BOOL WINAPI ShowOwnedPopups( HWND owner, BOOL show )
{
    return NtUserShowOwnedPopups( owner, show );
}


/*******************************************************************
 *		GetLastActivePopup (USER32.@)
 */
HWND WINAPI GetLastActivePopup( HWND hwnd )
{
    return NtUserGetLastActivePopup( hwnd );
}


/*******************************************************************
 *           WIN_ListChildren
 *
 * Build an array of the children of a given window. The array must be
 * freed with HeapFree. Returns NULL when no windows are found.
 */
HWND *WIN_ListChildren( HWND hwnd )
{
    HWND *list;
    ULONG size = 128;
    NTSTATUS status;

    if (!(hwnd = GetWindow( hwnd, GW_CHILD ))) return NULL;

    for (;;)
    {
        if (!(list = HeapAlloc( GetProcessHeap(), 0, size * sizeof(HWND) ))) return NULL;
        status = NtUserBuildHwndList( 0, hwnd, FALSE, TRUE, 0, size, list, &size );
        if (!status && size > 1) break;
        HeapFree( GetProcessHeap(), 0, list );
        if (status != STATUS_BUFFER_TOO_SMALL) return NULL;
    }
    list[size - 1] = 0;
    return list;
}


/*******************************************************************
 *		EnumWindows (USER32.@)
 */
BOOL WINAPI EnumWindows( WNDENUMPROC lpEnumFunc, LPARAM lParam )
{
    return enum_windows( 0, 0, 0, FALSE, lpEnumFunc, lParam );
}


/**********************************************************************
 *		EnumThreadWindows (USER32.@)
 */
BOOL WINAPI EnumThreadWindows( DWORD id, WNDENUMPROC func, LPARAM lParam )
{
    return enum_windows( 0, 0, id, FALSE, func, lParam );
}


/***********************************************************************
 *              EnumDesktopWindows   (USER32.@)
 */
BOOL WINAPI EnumDesktopWindows( HDESK desktop, WNDENUMPROC func, LPARAM lparam )
{
    return enum_windows( desktop, 0, 0, FALSE, func, lparam );
}


/**********************************************************************
 *		EnumChildWindows (USER32.@)
 */
BOOL WINAPI EnumChildWindows( HWND parent, WNDENUMPROC func, LPARAM lParam )
{
    return enum_windows( 0, parent, 0, TRUE, func, lParam );
}


/*******************************************************************
 *		AnyPopup (USER32.@)
 */
BOOL WINAPI AnyPopup(void)
{
    int i;
    BOOL retvalue;
    HWND *list = WIN_ListChildren( GetDesktopWindow() );

    if (!list) return FALSE;
    for (i = 0; list[i]; i++)
    {
        if (IsWindowVisible( list[i] ) && GetWindow( list[i], GW_OWNER )) break;
    }
    retvalue = (list[i] != 0);
    HeapFree( GetProcessHeap(), 0, list );
    return retvalue;
}


/*******************************************************************
 *		FlashWindow (USER32.@)
 */
BOOL WINAPI FlashWindow( HWND hWnd, BOOL bInvert )
{
    FLASHWINFO finfo;

    finfo.cbSize = sizeof(FLASHWINFO);
    finfo.dwFlags = bInvert ? FLASHW_ALL : FLASHW_STOP;
    finfo.uCount = 1;
    finfo.dwTimeout = 0;
    finfo.hwnd = hWnd;
    return NtUserFlashWindowEx( &finfo );
}


/*******************************************************************
 *		GetWindowContextHelpId (USER32.@)
 */
DWORD WINAPI GetWindowContextHelpId( HWND hwnd )
{
    return NtUserGetWindowContextHelpId( hwnd );
}


/*******************************************************************
 *		SetWindowContextHelpId (USER32.@)
 */
BOOL WINAPI SetWindowContextHelpId( HWND hwnd, DWORD id )
{
    return NtUserSetWindowContextHelpId( hwnd, id );
}


/*******************************************************************
 *		DragDetect (USER32.@)
 */
BOOL WINAPI DragDetect( HWND hwnd, POINT pt )
{
    return NtUserDragDetect( hwnd, pt.x, pt.y );
}

/******************************************************************************
 *		GetWindowModuleFileNameA (USER32.@)
 */
UINT WINAPI GetWindowModuleFileNameA( HWND hwnd, LPSTR module, UINT size )
{
    HINSTANCE hinst;

    TRACE( "%p, %p, %u\n", hwnd, module, size );

    if (!WIN_IsCurrentProcess( hwnd ))
    {
        SetLastError( ERROR_INVALID_WINDOW_HANDLE );
        return 0;
    }

    hinst = (HINSTANCE)GetWindowLongPtrA( hwnd, GWLP_HINSTANCE );
    return GetModuleFileNameA( hinst, module, size );
}

/******************************************************************************
 *		GetWindowModuleFileNameW (USER32.@)
 */
UINT WINAPI GetWindowModuleFileNameW( HWND hwnd, LPWSTR module, UINT size )
{
    HINSTANCE hinst;

    TRACE( "%p, %p, %u\n", hwnd, module, size );

    if (!WIN_IsCurrentProcess( hwnd ))
    {
        SetLastError( ERROR_INVALID_WINDOW_HANDLE );
        return 0;
    }

    hinst = (HINSTANCE)GetWindowLongPtrW( hwnd, GWLP_HINSTANCE );
    return GetModuleFileNameW( hinst, module, size );
}

/******************************************************************************
 *              GetWindowInfo (USER32.@)
 *
 * Note: tests show that Windows doesn't check cbSize of the structure.
 */
BOOL WINAPI DECLSPEC_HOTPATCH GetWindowInfo( HWND hwnd, WINDOWINFO *info )
{
    return NtUserGetWindowInfo( hwnd, info );
}

/*****************************************************************************
 *              UpdateLayeredWindowIndirect  (USER32.@)
 */
BOOL WINAPI UpdateLayeredWindowIndirect( HWND hwnd, const UPDATELAYEREDWINDOWINFO *info )
{
    if (!info || info->cbSize != sizeof(*info))
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    return NtUserUpdateLayeredWindow( hwnd, info->hdcDst, info->pptDst, info->psize,
                                      info->hdcSrc, info->pptSrc, info->crKey,
                                      info->pblend, info->dwFlags, info->prcDirty );
}


/*****************************************************************************
 *              UpdateLayeredWindow (USER32.@)
 */
BOOL WINAPI UpdateLayeredWindow( HWND hwnd, HDC hdcDst, POINT *pptDst, SIZE *psize,
                                 HDC hdcSrc, POINT *pptSrc, COLORREF crKey, BLENDFUNCTION *pblend,
                                 DWORD flags)
{
    UPDATELAYEREDWINDOWINFO info;

    if (flags & ULW_EX_NORESIZE)  /* only valid for UpdateLayeredWindowIndirect */
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    info.cbSize   = sizeof(info);
    info.hdcDst   = hdcDst;
    info.pptDst   = pptDst;
    info.psize    = psize;
    info.hdcSrc   = hdcSrc;
    info.pptSrc   = pptSrc;
    info.crKey    = crKey;
    info.pblend   = pblend;
    info.dwFlags  = flags;
    info.prcDirty = NULL;
    return UpdateLayeredWindowIndirect( hwnd, &info );
}


/******************************************************************************
 *                    GetProcessDefaultLayout [USER32.@]
 *
 * Gets the default layout for parentless windows.
 */
BOOL WINAPI GetProcessDefaultLayout( DWORD *layout )
{
    if (!layout)
    {
        SetLastError( ERROR_NOACCESS );
        return FALSE;
    }
    *layout = NtUserGetProcessDefaultLayout();
    if (*layout == ~0u)
    {
        WCHAR *str, buffer[MAX_PATH];
        DWORD i, version_layout = 0;
        UINT len;
        DWORD user_lang = GetUserDefaultLangID();
        DWORD *languages;
        void *data = NULL;

        GetModuleFileNameW( 0, buffer, MAX_PATH );
        if (!(len = GetFileVersionInfoSizeW( buffer, NULL ))) goto done;
        if (!(data = HeapAlloc( GetProcessHeap(), 0, len ))) goto done;
        if (!GetFileVersionInfoW( buffer, 0, len, data )) goto done;
        if (!VerQueryValueW( data, L"\\VarFileInfo\\Translation", (void **)&languages, &len ) || !len) goto done;

        len /= sizeof(DWORD);
        for (i = 0; i < len; i++) if (LOWORD(languages[i]) == user_lang) break;
        if (i == len)  /* try neutral language */
            for (i = 0; i < len; i++)
                if (LOWORD(languages[i]) == MAKELANGID( PRIMARYLANGID(user_lang), SUBLANG_NEUTRAL )) break;
        if (i == len) i = 0;  /* default to the first one */

        swprintf( buffer, ARRAY_SIZE(buffer), L"\\StringFileInfo\\%04x%04x\\FileDescription",
                  LOWORD(languages[i]), HIWORD(languages[i]) );
        if (!VerQueryValueW( data, buffer, (void **)&str, &len )) goto done;
        TRACE( "found description %s\n", debugstr_w( str ));
        if (str[0] == 0x200e && str[1] == 0x200e) version_layout = LAYOUT_RTL;

    done:
        HeapFree( GetProcessHeap(), 0, data );
        NtUserSetProcessDefaultLayout( *layout = version_layout );
    }
    return TRUE;
}


/******************************************************************************
 *                    SetProcessDefaultLayout [USER32.@]
 *
 * Sets the default layout for parentless windows.
 */
BOOL WINAPI SetProcessDefaultLayout( DWORD layout )
{
    return NtUserSetProcessDefaultLayout( layout );
}


/***********************************************************************
 *           UpdateWindow (USER32.@)
 */
BOOL WINAPI UpdateWindow( HWND hwnd )
{
    if (!hwnd)
    {
        SetLastError( ERROR_INVALID_WINDOW_HANDLE );
        return FALSE;
    }

    return NtUserRedrawWindow( hwnd, NULL, 0, RDW_UPDATENOW | RDW_ALLCHILDREN );
}


/***********************************************************************
 *           ValidateRgn (USER32.@)
 */
BOOL WINAPI ValidateRgn( HWND hwnd, HRGN hrgn )
{
    if (!hwnd)
    {
        SetLastError( ERROR_INVALID_WINDOW_HANDLE );
        return FALSE;
    }

    return NtUserRedrawWindow( hwnd, NULL, hrgn, RDW_VALIDATE );
}


/*************************************************************************
 *           ScrollWindow (USER32.@)
 */
BOOL WINAPI ScrollWindow( HWND hwnd, INT dx, INT dy, const RECT *rect, const RECT *clip_rect )
{
    UINT flags = SW_INVALIDATE | SW_ERASE | (rect ? 0 : SW_SCROLLCHILDREN) | SW_NODCCACHE;
    return NtUserScrollWindowEx( hwnd, dx, dy, rect, clip_rect, 0, NULL, flags );
}

#ifdef _WIN64

/* 64bit versions */

#undef GetWindowLongPtrW
#undef GetWindowLongPtrA
#undef SetWindowLongPtrW
#undef SetWindowLongPtrA

/*****************************************************************************
 *              GetWindowLongPtrW (USER32.@)
 */
LONG_PTR WINAPI GetWindowLongPtrW( HWND hwnd, INT offset )
{
    LONG_PTR ret = NtUserGetWindowLongPtrW( hwnd, offset );
    return get_window_long_ptr( hwnd, offset, ret, FALSE );
}

/*****************************************************************************
 *              GetWindowLongPtrA (USER32.@)
 */
LONG_PTR WINAPI GetWindowLongPtrA( HWND hwnd, INT offset )
{
    LONG_PTR ret = NtUserGetWindowLongPtrA( hwnd, offset );
    return get_window_long_ptr( hwnd, offset, ret, TRUE );
}

/*****************************************************************************
 *              SetWindowLongPtrW (USER32.@)
 */
LONG_PTR WINAPI SetWindowLongPtrW( HWND hwnd, INT offset, LONG_PTR newval )
{
    if (offset == DWLP_DLGPROC && NtUserGetDialogInfo( hwnd ))
        return set_dialog_proc( hwnd, newval, FALSE );

    return NtUserSetWindowLongPtr( hwnd, offset, newval, FALSE );
}

/*****************************************************************************
 *              SetWindowLongPtrA (USER32.@)
 */
LONG_PTR WINAPI SetWindowLongPtrA( HWND hwnd, INT offset, LONG_PTR newval )
{
    if (offset == DWLP_DLGPROC && NtUserGetDialogInfo( hwnd ))
        return set_dialog_proc( hwnd, newval, TRUE );

    return NtUserSetWindowLongPtr( hwnd, offset, newval, TRUE );
}

#endif /* _WIN64 */

/*****************************************************************************
 *              GetWindowDisplayAffinity (USER32.@)
 */
BOOL WINAPI GetWindowDisplayAffinity(HWND hwnd, DWORD *affinity)
{
    FIXME("(%p, %p): stub\n", hwnd, affinity);

    if (!hwnd || !affinity)
    {
        SetLastError(hwnd ? ERROR_NOACCESS : ERROR_INVALID_WINDOW_HANDLE);
        return FALSE;
    }

    *affinity = WDA_NONE;
    return TRUE;
}

/*****************************************************************************
 *              SetWindowDisplayAffinity (USER32.@)
 */
BOOL WINAPI SetWindowDisplayAffinity(HWND hwnd, DWORD affinity)
{
    FIXME("(%p, %lu): stub\n", hwnd, affinity);

    if (!hwnd)
    {
        SetLastError(ERROR_INVALID_WINDOW_HANDLE);
        return FALSE;
    }

    SetLastError(ERROR_NOT_ENOUGH_MEMORY);
    return FALSE;
}

/**********************************************************************
 *              SetWindowCompositionAttribute (USER32.@)
 */
BOOL WINAPI SetWindowCompositionAttribute(HWND hwnd, void *data)
{
    FIXME("(%p, %p): stub\n", hwnd, data);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}
