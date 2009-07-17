/*
 * Window procedure callbacks
 *
 * Copyright 1995 Martin von Loewis
 * Copyright 1996 Alexandre Julliard
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

#include "windef.h"
#include "winbase.h"
#include "wownt32.h"
#include "wine/winbase16.h"
#include "wine/winuser16.h"
#include "controls.h"
#include "win.h"
#include "user_private.h"
#include "dde.h"
#include "winternl.h"
#include "wine/unicode.h"
#include "wine/debug.h"

WINE_DECLARE_DEBUG_CHANNEL(msg);
WINE_DECLARE_DEBUG_CHANNEL(relay);
WINE_DEFAULT_DEBUG_CHANNEL(win);

typedef struct tagWINDOWPROC
{
    WNDPROC16      proc16;   /* 16-bit window proc */
    WNDPROC        procA;    /* ASCII window proc */
    WNDPROC        procW;    /* Unicode window proc */
} WINDOWPROC;

#define WINPROC_HANDLE (~0u >> 16)
#define MAX_WINPROCS  8192
#define BUILTIN_WINPROCS 9  /* first BUILTIN_WINPROCS entries are reserved for builtin procs */
#define MAX_WINPROC_RECURSION  64

WNDPROC EDIT_winproc_handle = 0;

static WINDOWPROC winproc_array[MAX_WINPROCS];
static UINT builtin_used;
static UINT winproc_used = BUILTIN_WINPROCS;

static CRITICAL_SECTION winproc_cs;
static CRITICAL_SECTION_DEBUG critsect_debug =
{
    0, 0, &winproc_cs,
    { &critsect_debug.ProcessLocksList, &critsect_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": winproc_cs") }
};
static CRITICAL_SECTION winproc_cs = { &critsect_debug, -1, 0, 0, 0, 0 };

static inline void *get_buffer( void *static_buffer, size_t size, size_t need )
{
    if (size >= need) return static_buffer;
    return HeapAlloc( GetProcessHeap(), 0, need );
}

static inline void free_buffer( void *static_buffer, void *buffer )
{
    if (buffer != static_buffer) HeapFree( GetProcessHeap(), 0, buffer );
}

/* find an existing winproc for a given 16-bit function and type */
/* FIXME: probably should do something more clever than a linear search */
static inline WINDOWPROC *find_winproc16( WNDPROC16 func )
{
    unsigned int i;

    for (i = BUILTIN_WINPROCS; i < winproc_used; i++)
    {
        if (winproc_array[i].proc16 == func) return &winproc_array[i];
    }
    return NULL;
}

/* find an existing winproc for a given function and type */
/* FIXME: probably should do something more clever than a linear search */
static inline WINDOWPROC *find_winproc( WNDPROC funcA, WNDPROC funcW )
{
    unsigned int i;

    for (i = 0; i < builtin_used; i++)
    {
        /* match either proc, some apps confuse A and W */
        if (funcA && winproc_array[i].procA != funcA && winproc_array[i].procW != funcA) continue;
        if (funcW && winproc_array[i].procA != funcW && winproc_array[i].procW != funcW) continue;
        return &winproc_array[i];
    }
    for (i = BUILTIN_WINPROCS; i < winproc_used; i++)
    {
        if (funcA && winproc_array[i].procA != funcA) continue;
        if (funcW && winproc_array[i].procW != funcW) continue;
        return &winproc_array[i];
    }
    return NULL;
}

/* return the window proc for a given handle, or NULL for an invalid handle */
static inline WINDOWPROC *handle_to_proc( WNDPROC handle )
{
    UINT index = LOWORD(handle);
    if ((ULONG_PTR)handle >> 16 != WINPROC_HANDLE) return NULL;
    if (index >= winproc_used) return NULL;
    return &winproc_array[index];
}

/* create a handle for a given window proc */
static inline WNDPROC proc_to_handle( WINDOWPROC *proc )
{
    return (WNDPROC)(ULONG_PTR)((proc - winproc_array) | (WINPROC_HANDLE << 16));
}

/* allocate and initialize a new winproc */
static inline WINDOWPROC *alloc_winproc( WNDPROC funcA, WNDPROC funcW )
{
    WINDOWPROC *proc;

    /* check if the function is already a win proc */
    if (funcA && (proc = handle_to_proc( funcA ))) return proc;
    if (funcW && (proc = handle_to_proc( funcW ))) return proc;
    if (!funcA && !funcW) return NULL;

    EnterCriticalSection( &winproc_cs );

    /* check if we already have a winproc for that function */
    if (!(proc = find_winproc( funcA, funcW )))
    {
        if (funcA && funcW)
        {
            assert( builtin_used < BUILTIN_WINPROCS );
            proc = &winproc_array[builtin_used++];
            proc->procA = funcA;
            proc->procW = funcW;
            TRACE( "allocated %p for builtin %p/%p (%d/%d used)\n",
                   proc_to_handle(proc), funcA, funcW, builtin_used, BUILTIN_WINPROCS );
        }
        else if (winproc_used < MAX_WINPROCS)
        {
            proc = &winproc_array[winproc_used++];
            proc->procA = funcA;
            proc->procW = funcW;
            TRACE( "allocated %p for %c %p (%d/%d used)\n",
                   proc_to_handle(proc), funcA ? 'A' : 'W', funcA ? funcA : funcW,
                   winproc_used, MAX_WINPROCS );
        }
        else FIXME( "too many winprocs, cannot allocate one for %p/%p\n", funcA, funcW );
    }
    else TRACE( "reusing %p for %p/%p\n", proc_to_handle(proc), funcA, funcW );

    LeaveCriticalSection( &winproc_cs );
    return proc;
}


#ifdef __i386__

#include "pshpack1.h"

/* Window procedure 16-to-32-bit thunk */
typedef struct
{
    BYTE        popl_eax;        /* popl  %eax (return address) */
    BYTE        pushl_func;      /* pushl $proc */
    WINDOWPROC *proc;
    BYTE        pushl_eax;       /* pushl %eax */
    BYTE        ljmp;            /* ljmp relay*/
    DWORD       relay_offset;    /* __wine_call_wndproc */
    WORD        relay_sel;
} WINPROC_THUNK;

#include "poppack.h"

#define MAX_THUNKS  (0x10000 / sizeof(WINPROC_THUNK))

static WINPROC_THUNK *thunk_array;
static UINT thunk_selector;
static UINT thunk_used;

/* return the window proc for a given handle, or NULL for an invalid handle */
static inline WINDOWPROC *handle16_to_proc( WNDPROC16 handle )
{
    if (HIWORD(handle) == thunk_selector)
    {
        UINT index = LOWORD(handle) / sizeof(WINPROC_THUNK);
        /* check alignment */
        if (index * sizeof(WINPROC_THUNK) != LOWORD(handle)) return NULL;
        /* check array limits */
        if (index >= thunk_used) return NULL;
        return thunk_array[index].proc;
    }
    return handle_to_proc( (WNDPROC)handle );
}

/* allocate a 16-bit thunk for an existing window proc */
static WNDPROC16 alloc_win16_thunk( WINDOWPROC *proc )
{
    static FARPROC16 relay;
    UINT i;

    if (proc->proc16) return proc->proc16;

    EnterCriticalSection( &winproc_cs );

    if (!thunk_array)  /* allocate the array and its selector */
    {
        LDT_ENTRY entry;

        if (!(thunk_selector = wine_ldt_alloc_entries(1))) goto done;
        if (!(thunk_array = VirtualAlloc( NULL, MAX_THUNKS * sizeof(WINPROC_THUNK), MEM_COMMIT,
                                          PAGE_EXECUTE_READWRITE ))) goto done;
        wine_ldt_set_base( &entry, thunk_array );
        wine_ldt_set_limit( &entry, MAX_THUNKS * sizeof(WINPROC_THUNK) - 1 );
        wine_ldt_set_flags( &entry, WINE_LDT_FLAGS_CODE | WINE_LDT_FLAGS_32BIT );
        wine_ldt_set_entry( thunk_selector, &entry );
        relay = GetProcAddress16( GetModuleHandle16("user"), "__wine_call_wndproc" );
    }

    /* check if it already exists */
    for (i = 0; i < thunk_used; i++) if (thunk_array[i].proc == proc) break;

    if (i == thunk_used)  /* create a new one */
    {
        WINPROC_THUNK *thunk;

        if (thunk_used >= MAX_THUNKS) goto done;
        thunk = &thunk_array[thunk_used++];
        thunk->popl_eax     = 0x58;   /* popl  %eax */
        thunk->pushl_func   = 0x68;   /* pushl $proc */
        thunk->proc         = proc;
        thunk->pushl_eax    = 0x50;   /* pushl %eax */
        thunk->ljmp         = 0xea;   /* ljmp   relay*/
        thunk->relay_offset = OFFSETOF(relay);
        thunk->relay_sel    = SELECTOROF(relay);
    }
    proc->proc16 = (WNDPROC16)MAKESEGPTR( thunk_selector, i * sizeof(WINPROC_THUNK) );
done:
    LeaveCriticalSection( &winproc_cs );
    return proc->proc16;
}

#else  /* __i386__ */

static inline WINDOWPROC *handle16_to_proc( WNDPROC16 handle )
{
    return handle_to_proc( (WNDPROC)handle );
}

static inline WNDPROC16 alloc_win16_thunk( WINDOWPROC *proc )
{
    return 0;
}

#endif  /* __i386__ */


#ifdef __i386__
/* Some window procedures modify register they shouldn't, or are not
 * properly declared stdcall; so we need a small assembly wrapper to
 * call them. */
extern LRESULT WINPROC_wrapper( WNDPROC proc, HWND hwnd, UINT msg,
                                WPARAM wParam, LPARAM lParam );
__ASM_GLOBAL_FUNC( WINPROC_wrapper,
                   "pushl %ebp\n\t"
                   __ASM_CFI(".cfi_adjust_cfa_offset 4\n\t")
                   __ASM_CFI(".cfi_rel_offset %ebp,0\n\t")
                   "movl %esp,%ebp\n\t"
                   __ASM_CFI(".cfi_def_cfa_register %ebp\n\t")
                   "pushl %edi\n\t"
                   __ASM_CFI(".cfi_rel_offset %edi,-4\n\t")
                   "pushl %esi\n\t"
                   __ASM_CFI(".cfi_rel_offset %esi,-8\n\t")
                   "pushl %ebx\n\t"
                   __ASM_CFI(".cfi_rel_offset %ebx,-12\n\t")
                   "subl $12,%esp\n\t"
                   "pushl 24(%ebp)\n\t"
                   "pushl 20(%ebp)\n\t"
                   "pushl 16(%ebp)\n\t"
                   "pushl 12(%ebp)\n\t"
                   "movl 8(%ebp),%eax\n\t"
                   "call *%eax\n\t"
                   "leal -12(%ebp),%esp\n\t"
                   "popl %ebx\n\t"
                   __ASM_CFI(".cfi_same_value %ebx\n\t")
                   "popl %esi\n\t"
                   __ASM_CFI(".cfi_same_value %esi\n\t")
                   "popl %edi\n\t"
                   __ASM_CFI(".cfi_same_value %edi\n\t")
                   "leave\n\t"
                   __ASM_CFI(".cfi_def_cfa %esp,4\n\t")
                   __ASM_CFI(".cfi_same_value %ebp\n\t")
                   "ret" )
#else
static inline LRESULT WINPROC_wrapper( WNDPROC proc, HWND hwnd, UINT msg,
                                       WPARAM wParam, LPARAM lParam )
{
    return proc( hwnd, msg, wParam, lParam );
}
#endif  /* __i386__ */

static void RECT16to32( const RECT16 *from, RECT *to )
{
    to->left   = from->left;
    to->top    = from->top;
    to->right  = from->right;
    to->bottom = from->bottom;
}

static void RECT32to16( const RECT *from, RECT16 *to )
{
    to->left   = from->left;
    to->top    = from->top;
    to->right  = from->right;
    to->bottom = from->bottom;
}

static void MINMAXINFO32to16( const MINMAXINFO *from, MINMAXINFO16 *to )
{
    to->ptReserved.x     = from->ptReserved.x;
    to->ptReserved.y     = from->ptReserved.y;
    to->ptMaxSize.x      = from->ptMaxSize.x;
    to->ptMaxSize.y      = from->ptMaxSize.y;
    to->ptMaxPosition.x  = from->ptMaxPosition.x;
    to->ptMaxPosition.y  = from->ptMaxPosition.y;
    to->ptMinTrackSize.x = from->ptMinTrackSize.x;
    to->ptMinTrackSize.y = from->ptMinTrackSize.y;
    to->ptMaxTrackSize.x = from->ptMaxTrackSize.x;
    to->ptMaxTrackSize.y = from->ptMaxTrackSize.y;
}

static void MINMAXINFO16to32( const MINMAXINFO16 *from, MINMAXINFO *to )
{
    to->ptReserved.x     = from->ptReserved.x;
    to->ptReserved.y     = from->ptReserved.y;
    to->ptMaxSize.x      = from->ptMaxSize.x;
    to->ptMaxSize.y      = from->ptMaxSize.y;
    to->ptMaxPosition.x  = from->ptMaxPosition.x;
    to->ptMaxPosition.y  = from->ptMaxPosition.y;
    to->ptMinTrackSize.x = from->ptMinTrackSize.x;
    to->ptMinTrackSize.y = from->ptMinTrackSize.y;
    to->ptMaxTrackSize.x = from->ptMaxTrackSize.x;
    to->ptMaxTrackSize.y = from->ptMaxTrackSize.y;
}

static void WINDOWPOS32to16( const WINDOWPOS* from, WINDOWPOS16* to )
{
    to->hwnd            = HWND_16(from->hwnd);
    to->hwndInsertAfter = HWND_16(from->hwndInsertAfter);
    to->x               = from->x;
    to->y               = from->y;
    to->cx              = from->cx;
    to->cy              = from->cy;
    to->flags           = from->flags;
}

static void WINDOWPOS16to32( const WINDOWPOS16* from, WINDOWPOS* to )
{
    to->hwnd            = WIN_Handle32(from->hwnd);
    to->hwndInsertAfter = (from->hwndInsertAfter == (HWND16)-1) ?
                           HWND_TOPMOST : WIN_Handle32(from->hwndInsertAfter);
    to->x               = from->x;
    to->y               = from->y;
    to->cx              = from->cx;
    to->cy              = from->cy;
    to->flags           = from->flags;
}

/* The strings are not copied */
static void CREATESTRUCT32Ato16( const CREATESTRUCTA* from, CREATESTRUCT16* to )
{
    to->lpCreateParams = (SEGPTR)from->lpCreateParams;
    to->hInstance      = HINSTANCE_16(from->hInstance);
    to->hMenu          = HMENU_16(from->hMenu);
    to->hwndParent     = HWND_16(from->hwndParent);
    to->cy             = from->cy;
    to->cx             = from->cx;
    to->y              = from->y;
    to->x              = from->x;
    to->style          = from->style;
    to->dwExStyle      = from->dwExStyle;
}

static void CREATESTRUCT16to32A( const CREATESTRUCT16* from, CREATESTRUCTA *to )

{
    to->lpCreateParams = (LPVOID)from->lpCreateParams;
    to->hInstance      = HINSTANCE_32(from->hInstance);
    to->hMenu          = HMENU_32(from->hMenu);
    to->hwndParent     = WIN_Handle32(from->hwndParent);
    to->cy             = from->cy;
    to->cx             = from->cx;
    to->y              = from->y;
    to->x              = from->x;
    to->style          = from->style;
    to->dwExStyle      = from->dwExStyle;
    to->lpszName       = MapSL(from->lpszName);
    to->lpszClass      = MapSL(from->lpszClass);
}

/* The strings are not copied */
static void MDICREATESTRUCT32Ato16( const MDICREATESTRUCTA* from, MDICREATESTRUCT16* to )
{
    to->hOwner = HINSTANCE_16(from->hOwner);
    to->x      = from->x;
    to->y      = from->y;
    to->cx     = from->cx;
    to->cy     = from->cy;
    to->style  = from->style;
    to->lParam = from->lParam;
}

static void MDICREATESTRUCT16to32A( const MDICREATESTRUCT16* from, MDICREATESTRUCTA *to )
{
    to->hOwner = HINSTANCE_32(from->hOwner);
    to->x      = from->x;
    to->y      = from->y;
    to->cx     = from->cx;
    to->cy     = from->cy;
    to->style  = from->style;
    to->lParam = from->lParam;
    to->szTitle = MapSL(from->szTitle);
    to->szClass = MapSL(from->szClass);
}

static WPARAM map_wparam_char_WtoA( WPARAM wParam, DWORD len )
{
    WCHAR wch = wParam;
    BYTE ch[2];

    RtlUnicodeToMultiByteN( (LPSTR)ch, len, &len, &wch, sizeof(wch) );
    if (len == 2)
        return MAKEWPARAM( (ch[0] << 8) | ch[1], HIWORD(wParam) );
    else
        return MAKEWPARAM( ch[0], HIWORD(wParam) );
}

/* call a 32-bit window procedure */
static LRESULT call_window_proc( HWND hwnd, UINT msg, WPARAM wp, LPARAM lp, LRESULT *result, void *arg )
{
    WNDPROC proc = arg;

    USER_CheckNotLock();

    hwnd = WIN_GetFullHandle( hwnd );
    if (TRACE_ON(relay))
        DPRINTF( "%04x:Call window proc %p (hwnd=%p,msg=%s,wp=%08lx,lp=%08lx)\n",
                 GetCurrentThreadId(), proc, hwnd, SPY_GetMsgName(msg, hwnd), wp, lp );

    *result = WINPROC_wrapper( proc, hwnd, msg, wp, lp );

    if (TRACE_ON(relay))
        DPRINTF( "%04x:Ret  window proc %p (hwnd=%p,msg=%s,wp=%08lx,lp=%08lx) retval=%08lx\n",
                 GetCurrentThreadId(), proc, hwnd, SPY_GetMsgName(msg, hwnd), wp, lp, *result );
    return *result;
}

/* call a 32-bit dialog procedure */
static LRESULT call_dialog_proc( HWND hwnd, UINT msg, WPARAM wp, LPARAM lp, LRESULT *result, void *arg )
{
    WNDPROC proc = arg;
    LRESULT ret;

    USER_CheckNotLock();

    hwnd = WIN_GetFullHandle( hwnd );
    if (TRACE_ON(relay))
        DPRINTF( "%04x:Call dialog proc %p (hwnd=%p,msg=%s,wp=%08lx,lp=%08lx)\n",
                 GetCurrentThreadId(), proc, hwnd, SPY_GetMsgName(msg, hwnd), wp, lp );

    ret = WINPROC_wrapper( proc, hwnd, msg, wp, lp );
    *result = GetWindowLongPtrW( hwnd, DWLP_MSGRESULT );

    if (TRACE_ON(relay))
        DPRINTF( "%04x:Ret  dialog proc %p (hwnd=%p,msg=%s,wp=%08lx,lp=%08lx) retval=%08lx result=%08lx\n",
                 GetCurrentThreadId(), proc, hwnd, SPY_GetMsgName(msg, hwnd), wp, lp, ret, *result );
    return ret;
}

/* call a 16-bit window procedure */
static LRESULT call_window_proc16( HWND16 hwnd, UINT16 msg, WPARAM16 wParam, LPARAM lParam,
                                   LRESULT *result, void *arg )
{
    WNDPROC16 proc = arg;
    CONTEXT86 context;
    size_t size = 0;
    struct
    {
        WORD params[5];
        union
        {
            CREATESTRUCT16 cs16;
            DRAWITEMSTRUCT16 dis16;
            COMPAREITEMSTRUCT16 cis16;
        } u;
    } args;

    USER_CheckNotLock();

    /* Window procedures want ax = hInstance, ds = es = ss */

    memset(&context, 0, sizeof(context));
    context.SegDs = context.SegEs = SELECTOROF(NtCurrentTeb()->WOW32Reserved);
    context.SegFs = wine_get_fs();
    context.SegGs = wine_get_gs();
    if (!(context.Eax = GetWindowWord( HWND_32(hwnd), GWLP_HINSTANCE ))) context.Eax = context.SegDs;
    context.SegCs = SELECTOROF(proc);
    context.Eip   = OFFSETOF(proc);
    context.Ebp   = OFFSETOF(NtCurrentTeb()->WOW32Reserved) + FIELD_OFFSET(STACK16FRAME, bp);

    if (lParam)
    {
        /* Some programs (eg. the "Undocumented Windows" examples, JWP) only
           work if structures passed in lParam are placed in the stack/data
           segment. Programmers easily make the mistake of converting lParam
           to a near rather than a far pointer, since Windows apparently
           allows this. We copy the structures to the 16 bit stack; this is
           ugly but makes these programs work. */
        switch (msg)
        {
          case WM_CREATE:
          case WM_NCCREATE:
            size = sizeof(CREATESTRUCT16); break;
          case WM_DRAWITEM:
            size = sizeof(DRAWITEMSTRUCT16); break;
          case WM_COMPAREITEM:
            size = sizeof(COMPAREITEMSTRUCT16); break;
        }
        if (size)
        {
            memcpy( &args.u, MapSL(lParam), size );
            lParam = PtrToUlong(NtCurrentTeb()->WOW32Reserved) - size;
        }
    }

    args.params[4] = hwnd;
    args.params[3] = msg;
    args.params[2] = wParam;
    args.params[1] = HIWORD(lParam);
    args.params[0] = LOWORD(lParam);
    WOWCallback16Ex( 0, WCB16_REGS, sizeof(args.params) + size, &args, (DWORD *)&context );
    *result = MAKELONG( LOWORD(context.Eax), LOWORD(context.Edx) );
    return *result;
}

/* call a 16-bit dialog procedure */
static LRESULT call_dialog_proc16( HWND16 hwnd, UINT16 msg, WPARAM16 wp, LPARAM lp,
                                   LRESULT *result, void *arg )
{
    LRESULT ret = call_window_proc16( hwnd, msg, wp, lp, result, arg );
    *result = GetWindowLongPtrW( WIN_Handle32(hwnd), DWLP_MSGRESULT );
    return LOWORD(ret);
}

/* helper callback for 32W->16 conversion */
static LRESULT call_window_proc_Ato16( HWND hwnd, UINT msg, WPARAM wp, LPARAM lp,
                                       LRESULT *result, void *arg )
{
    return WINPROC_CallProc32ATo16( call_window_proc16, hwnd, msg, wp, lp, result, arg );
}

/* helper callback for 32W->16 conversion */
static LRESULT call_dialog_proc_Ato16( HWND hwnd, UINT msg, WPARAM wp, LPARAM lp,
                                       LRESULT *result, void *arg )
{
    return WINPROC_CallProc32ATo16( call_dialog_proc16, hwnd, msg, wp, lp, result, arg );
}

/* helper callback for 16->32W conversion */
static LRESULT call_window_proc_AtoW( HWND hwnd, UINT msg, WPARAM wp, LPARAM lp,
                                      LRESULT *result, void *arg )
{
    return WINPROC_CallProcAtoW( call_window_proc, hwnd, msg, wp, lp, result,
                                 arg, WMCHAR_MAP_CALLWINDOWPROC );
}

/* helper callback for 16->32W conversion */
static LRESULT call_dialog_proc_AtoW( HWND hwnd, UINT msg, WPARAM wp, LPARAM lp,
                                      LRESULT *result, void *arg )
{
    return WINPROC_CallProcAtoW( call_dialog_proc, hwnd, msg, wp, lp, result,
                                 arg, WMCHAR_MAP_CALLWINDOWPROC );
}


/**********************************************************************
 *	     WINPROC_GetProc16
 *
 * Get a window procedure pointer that can be passed to the Windows program.
 */
WNDPROC16 WINPROC_GetProc16( WNDPROC proc, BOOL unicode )
{
    WINDOWPROC *ptr;

    if (unicode) ptr = alloc_winproc( NULL, proc );
    else ptr = alloc_winproc( proc, NULL );

    if (!ptr) return 0;
    return alloc_win16_thunk( ptr );
}


/**********************************************************************
 *	     WINPROC_GetProc
 *
 * Get a window procedure pointer that can be passed to the Windows program.
 */
WNDPROC WINPROC_GetProc( WNDPROC proc, BOOL unicode )
{
    WINDOWPROC *ptr = handle_to_proc( proc );

    if (!ptr) return proc;
    if (unicode)
    {
        if (ptr->procW) return ptr->procW;
        return proc;
    }
    else
    {
        if (ptr->procA) return ptr->procA;
        return proc;
    }
}


/**********************************************************************
 *	     WINPROC_AllocProc16
 *
 * Allocate a window procedure for a window or class.
 *
 * Note that allocated winprocs are never freed; the idea is that even if an app creates a
 * lot of windows, it will usually only have a limited number of window procedures, so the
 * array won't grow too large, and this way we avoid the need to track allocations per window.
 */
WNDPROC WINPROC_AllocProc16( WNDPROC16 func )
{
    WINDOWPROC *proc;

    if (!func) return NULL;

    /* check if the function is already a win proc */
    if (!(proc = handle16_to_proc( func )))
    {
        EnterCriticalSection( &winproc_cs );

        /* then check if we already have a winproc for that function */
        if (!(proc = find_winproc16( func )))
        {
            if (winproc_used < MAX_WINPROCS)
            {
                proc = &winproc_array[winproc_used++];
                proc->proc16 = func;
                TRACE( "allocated %p for %p/16-bit (%d/%d used)\n",
                       proc_to_handle(proc), func, winproc_used, MAX_WINPROCS );
            }
            else FIXME( "too many winprocs, cannot allocate one for 16-bit %p\n", func );
        }
        else TRACE( "reusing %p for %p/16-bit\n", proc_to_handle(proc), func );

        LeaveCriticalSection( &winproc_cs );
    }
    return proc_to_handle( proc );
}


/**********************************************************************
 *	     WINPROC_AllocProc
 *
 * Allocate a window procedure for a window or class.
 *
 * Note that allocated winprocs are never freed; the idea is that even if an app creates a
 * lot of windows, it will usually only have a limited number of window procedures, so the
 * array won't grow too large, and this way we avoid the need to track allocations per window.
 */
WNDPROC WINPROC_AllocProc( WNDPROC funcA, WNDPROC funcW )
{
    WINDOWPROC *proc;

    if (!(proc = alloc_winproc( funcA, funcW ))) return NULL;
    return proc_to_handle( proc );
}


/**********************************************************************
 *	     WINPROC_IsUnicode
 *
 * Return the window procedure type, or the default value if not a winproc handle.
 */
BOOL WINPROC_IsUnicode( WNDPROC proc, BOOL def_val )
{
    WINDOWPROC *ptr = handle_to_proc( proc );

    if (!ptr) return def_val;
    if (ptr->procA && ptr->procW) return def_val;  /* can be both */
    return (ptr->procW != NULL);
}


/**********************************************************************
 *	     WINPROC_TestLBForStr
 *
 * Return TRUE if the lparam is a string
 */
static inline BOOL WINPROC_TestLBForStr( HWND hwnd, UINT msg )
{
    DWORD style = GetWindowLongA( hwnd, GWL_STYLE );
    if (msg <= CB_MSGMAX)
        return (!(style & (CBS_OWNERDRAWFIXED | CBS_OWNERDRAWVARIABLE)) || (style & CBS_HASSTRINGS));
    else
        return (!(style & (LBS_OWNERDRAWFIXED | LBS_OWNERDRAWVARIABLE)) || (style & LBS_HASSTRINGS));

}


static UINT_PTR convert_handle_16_to_32(HANDLE16 src, unsigned int flags)
{
    HANDLE      dst;
    UINT        sz = GlobalSize16(src);
    LPSTR       ptr16, ptr32;

    if (!(dst = GlobalAlloc(flags, sz)))
        return 0;
    ptr16 = GlobalLock16(src);
    ptr32 = GlobalLock(dst);
    if (ptr16 != NULL && ptr32 != NULL) memcpy(ptr32, ptr16, sz);
    GlobalUnlock16(src);
    GlobalUnlock(dst);

    return (UINT_PTR)dst;
}

static HANDLE16 convert_handle_32_to_16(UINT_PTR src, unsigned int flags)
{
    HANDLE16    dst;
    UINT        sz = GlobalSize((HANDLE)src);
    LPSTR       ptr16, ptr32;

    if (!(dst = GlobalAlloc16(flags, sz)))
        return 0;
    ptr32 = GlobalLock((HANDLE)src);
    ptr16 = GlobalLock16(dst);
    if (ptr16 != NULL && ptr32 != NULL) memcpy(ptr16, ptr32, sz);
    GlobalUnlock((HANDLE)src);
    GlobalUnlock16(dst);

    return dst;
}


/**********************************************************************
 *	     WINPROC_CallProcAtoW
 *
 * Call a window procedure, translating args from Ansi to Unicode.
 */
LRESULT WINPROC_CallProcAtoW( winproc_callback_t callback, HWND hwnd, UINT msg, WPARAM wParam,
                              LPARAM lParam, LRESULT *result, void *arg, enum wm_char_mapping mapping )
{
    LRESULT ret = 0;

    TRACE_(msg)("(hwnd=%p,msg=%s,wp=%08lx,lp=%08lx)\n",
                hwnd, SPY_GetMsgName(msg, hwnd), wParam, lParam);

    switch(msg)
    {
    case WM_NCCREATE:
    case WM_CREATE:
        {
            WCHAR *ptr, buffer[512];
            CREATESTRUCTA *csA = (CREATESTRUCTA *)lParam;
            CREATESTRUCTW csW = *(CREATESTRUCTW *)csA;
            MDICREATESTRUCTW mdi_cs;
            DWORD name_lenA = 0, name_lenW = 0, class_lenA = 0, class_lenW = 0;

            if (!IS_INTRESOURCE(csA->lpszClass))
            {
                class_lenA = strlen(csA->lpszClass) + 1;
                RtlMultiByteToUnicodeSize( &class_lenW, csA->lpszClass, class_lenA );
            }
            if (!IS_INTRESOURCE(csA->lpszName))
            {
                name_lenA = strlen(csA->lpszName) + 1;
                RtlMultiByteToUnicodeSize( &name_lenW, csA->lpszName, name_lenA );
            }

            if (!(ptr = get_buffer( buffer, sizeof(buffer), class_lenW + name_lenW ))) break;

            if (class_lenW)
            {
                csW.lpszClass = ptr;
                RtlMultiByteToUnicodeN( ptr, class_lenW, NULL, csA->lpszClass, class_lenA );
            }
            if (name_lenW)
            {
                csW.lpszName = ptr + class_lenW/sizeof(WCHAR);
                RtlMultiByteToUnicodeN( ptr + class_lenW/sizeof(WCHAR), name_lenW, NULL,
                                        csA->lpszName, name_lenA );
            }

            if (GetWindowLongW(hwnd, GWL_EXSTYLE) & WS_EX_MDICHILD)
            {
                mdi_cs = *(MDICREATESTRUCTW *)csA->lpCreateParams;
                mdi_cs.szTitle = csW.lpszName;
                mdi_cs.szClass = csW.lpszClass;
                csW.lpCreateParams = &mdi_cs;
            }

            ret = callback( hwnd, msg, wParam, (LPARAM)&csW, result, arg );
            free_buffer( buffer, ptr );
        }
        break;

    case WM_MDICREATE:
        {
            WCHAR *ptr, buffer[512];
            DWORD title_lenA = 0, title_lenW = 0, class_lenA = 0, class_lenW = 0;
            MDICREATESTRUCTA *csA = (MDICREATESTRUCTA *)lParam;
            MDICREATESTRUCTW csW;

            memcpy( &csW, csA, sizeof(csW) );

            if (!IS_INTRESOURCE(csA->szTitle))
            {
                title_lenA = strlen(csA->szTitle) + 1;
                RtlMultiByteToUnicodeSize( &title_lenW, csA->szTitle, title_lenA );
            }
            if (!IS_INTRESOURCE(csA->szClass))
            {
                class_lenA = strlen(csA->szClass) + 1;
                RtlMultiByteToUnicodeSize( &class_lenW, csA->szClass, class_lenA );
            }

            if (!(ptr = get_buffer( buffer, sizeof(buffer), title_lenW + class_lenW ))) break;

            if (title_lenW)
            {
                csW.szTitle = ptr;
                RtlMultiByteToUnicodeN( ptr, title_lenW, NULL, csA->szTitle, title_lenA );
            }
            if (class_lenW)
            {
                csW.szClass = ptr + title_lenW/sizeof(WCHAR);
                RtlMultiByteToUnicodeN( ptr + title_lenW/sizeof(WCHAR), class_lenW, NULL,
                                        csA->szClass, class_lenA );
            }
            ret = callback( hwnd, msg, wParam, (LPARAM)&csW, result, arg );
            free_buffer( buffer, ptr );
        }
        break;

    case WM_GETTEXT:
    case WM_ASKCBFORMATNAME:
        {
            WCHAR *ptr, buffer[512];
            LPSTR str = (LPSTR)lParam;
            DWORD len = wParam * sizeof(WCHAR);

            if (!(ptr = get_buffer( buffer, sizeof(buffer), len ))) break;
            ret = callback( hwnd, msg, wParam, (LPARAM)ptr, result, arg );
            if (wParam)
            {
                len = 0;
                if (*result)
                    RtlUnicodeToMultiByteN( str, wParam - 1, &len, ptr, strlenW(ptr) * sizeof(WCHAR) );
                str[len] = 0;
                *result = len;
            }
            free_buffer( buffer, ptr );
        }
        break;

    case LB_ADDSTRING:
    case LB_INSERTSTRING:
    case LB_FINDSTRING:
    case LB_FINDSTRINGEXACT:
    case LB_SELECTSTRING:
    case CB_ADDSTRING:
    case CB_INSERTSTRING:
    case CB_FINDSTRING:
    case CB_FINDSTRINGEXACT:
    case CB_SELECTSTRING:
        if (!lParam || !WINPROC_TestLBForStr( hwnd, msg ))
        {
            ret = callback( hwnd, msg, wParam, lParam, result, arg );
            break;
        }
        /* fall through */
    case WM_SETTEXT:
    case WM_WININICHANGE:
    case WM_DEVMODECHANGE:
    case CB_DIR:
    case LB_DIR:
    case LB_ADDFILE:
    case EM_REPLACESEL:
        if (!lParam) ret = callback( hwnd, msg, wParam, lParam, result, arg );
        else
        {
            WCHAR *ptr, buffer[512];
            LPCSTR strA = (LPCSTR)lParam;
            DWORD lenW, lenA = strlen(strA) + 1;

            RtlMultiByteToUnicodeSize( &lenW, strA, lenA );
            if ((ptr = get_buffer( buffer, sizeof(buffer), lenW )))
            {
                RtlMultiByteToUnicodeN( ptr, lenW, NULL, strA, lenA );
                ret = callback( hwnd, msg, wParam, (LPARAM)ptr, result, arg );
                free_buffer( buffer, ptr );
            }
        }
        break;

    case LB_GETTEXT:
    case CB_GETLBTEXT:
        if (lParam && WINPROC_TestLBForStr( hwnd, msg ))
        {
            WCHAR buffer[512];  /* FIXME: fixed sized buffer */

            ret = callback( hwnd, msg, wParam, (LPARAM)buffer, result, arg );
            if (*result >= 0)
            {
                DWORD len;
                RtlUnicodeToMultiByteN( (LPSTR)lParam, ~0u, &len,
                                        buffer, (strlenW(buffer) + 1) * sizeof(WCHAR) );
                *result = len - 1;
            }
        }
        else ret = callback( hwnd, msg, wParam, lParam, result, arg );
        break;

    case EM_GETLINE:
        {
            WCHAR *ptr, buffer[512];
            WORD len = *(WORD *)lParam;

            if (!(ptr = get_buffer( buffer, sizeof(buffer), len * sizeof(WCHAR) ))) break;
            *((WORD *)ptr) = len;   /* store the length */
            ret = callback( hwnd, msg, wParam, (LPARAM)ptr, result, arg );
            if (*result)
            {
                DWORD reslen;
                RtlUnicodeToMultiByteN( (LPSTR)lParam, len, &reslen, ptr, *result * sizeof(WCHAR) );
                if (reslen < len) ((LPSTR)lParam)[reslen] = 0;
                *result = reslen;
            }
            free_buffer( buffer, ptr );
        }
        break;

    case WM_GETDLGCODE:
        if (lParam)
        {
            MSG newmsg = *(MSG *)lParam;
            if (map_wparam_AtoW( newmsg.message, &newmsg.wParam, WMCHAR_MAP_NOMAPPING ))
                ret = callback( hwnd, msg, wParam, (LPARAM)&newmsg, result, arg );
        }
        else ret = callback( hwnd, msg, wParam, lParam, result, arg );
        break;

    case WM_CHARTOITEM:
    case WM_MENUCHAR:
    case WM_CHAR:
    case WM_DEADCHAR:
    case WM_SYSCHAR:
    case WM_SYSDEADCHAR:
    case EM_SETPASSWORDCHAR:
    case WM_IME_CHAR:
        if (map_wparam_AtoW( msg, &wParam, mapping ))
            ret = callback( hwnd, msg, wParam, lParam, result, arg );
        break;

    case WM_GETTEXTLENGTH:
    case CB_GETLBTEXTLEN:
    case LB_GETTEXTLEN:
        ret = callback( hwnd, msg, wParam, lParam, result, arg );
        if (*result >= 0)
        {
            WCHAR *ptr, buffer[512];
            LRESULT tmp;
            DWORD len = *result + 1;
            /* Determine respective GETTEXT message */
            UINT msgGetText = (msg == WM_GETTEXTLENGTH) ? WM_GETTEXT :
                              ((msg == CB_GETLBTEXTLEN) ? CB_GETLBTEXT : LB_GETTEXT);
            /* wParam differs between the messages */
            WPARAM wp = (msg == WM_GETTEXTLENGTH) ? len : wParam;

            if (!(ptr = get_buffer( buffer, sizeof(buffer), len * sizeof(WCHAR) ))) break;

            if (callback == call_window_proc)  /* FIXME: hack */
                callback( hwnd, msgGetText, wp, (LPARAM)ptr, &tmp, arg );
            else
                tmp = SendMessageW( hwnd, msgGetText, wp, (LPARAM)ptr );
            RtlUnicodeToMultiByteSize( &len, ptr, tmp * sizeof(WCHAR) );
            *result = len;
            free_buffer( buffer, ptr );
        }
        break;

    case WM_PAINTCLIPBOARD:
    case WM_SIZECLIPBOARD:
        FIXME_(msg)( "message %s (0x%x) needs translation, please report\n",
                     SPY_GetMsgName(msg, hwnd), msg );
        break;

    default:
        ret = callback( hwnd, msg, wParam, lParam, result, arg );
        break;
    }
    return ret;
}


/**********************************************************************
 *	     WINPROC_CallProcWtoA
 *
 * Call a window procedure, translating args from Unicode to Ansi.
 */
static LRESULT WINPROC_CallProcWtoA( winproc_callback_t callback, HWND hwnd, UINT msg, WPARAM wParam,
                                     LPARAM lParam, LRESULT *result, void *arg )
{
    LRESULT ret = 0;

    TRACE_(msg)("(hwnd=%p,msg=%s,wp=%08lx,lp=%08lx)\n",
                hwnd, SPY_GetMsgName(msg, hwnd), wParam, lParam);

    switch(msg)
    {
    case WM_NCCREATE:
    case WM_CREATE:
        {
            char buffer[1024], *cls;
            CREATESTRUCTW *csW = (CREATESTRUCTW *)lParam;
            CREATESTRUCTA csA = *(CREATESTRUCTA *)csW;
            MDICREATESTRUCTA mdi_cs;
            DWORD name_lenA = 0, name_lenW = 0, class_lenA = 0, class_lenW = 0;

            if (!IS_INTRESOURCE(csW->lpszClass))
            {
                class_lenW = (strlenW(csW->lpszClass) + 1) * sizeof(WCHAR);
                RtlUnicodeToMultiByteSize(&class_lenA, csW->lpszClass, class_lenW);
            }
            if (!IS_INTRESOURCE(csW->lpszName))
            {
                name_lenW = (strlenW(csW->lpszName) + 1) * sizeof(WCHAR);
                RtlUnicodeToMultiByteSize(&name_lenA, csW->lpszName, name_lenW);
            }

            if (!(cls = get_buffer( buffer, sizeof(buffer), class_lenA + name_lenA ))) break;

            if (class_lenA)
            {
                RtlUnicodeToMultiByteN(cls, class_lenA, NULL, csW->lpszClass, class_lenW);
                csA.lpszClass = cls;
            }
            if (name_lenA)
            {
                char *name = cls + class_lenA;
                RtlUnicodeToMultiByteN(name, name_lenA, NULL, csW->lpszName, name_lenW);
                csA.lpszName = name;
            }

            if (GetWindowLongW(hwnd, GWL_EXSTYLE) & WS_EX_MDICHILD)
            {
                mdi_cs = *(MDICREATESTRUCTA *)csW->lpCreateParams;
                mdi_cs.szTitle = csA.lpszName;
                mdi_cs.szClass = csA.lpszClass;
                csA.lpCreateParams = &mdi_cs;
            }

            ret = callback( hwnd, msg, wParam, (LPARAM)&csA, result, arg );
            free_buffer( buffer, cls );
        }
        break;

    case WM_GETTEXT:
    case WM_ASKCBFORMATNAME:
        {
            char *ptr, buffer[512];
            DWORD len = wParam * 2;

            if (!(ptr = get_buffer( buffer, sizeof(buffer), len ))) break;
            ret = callback( hwnd, msg, wParam, (LPARAM)ptr, result, arg );
            if (len)
            {
                if (*result)
                {
                    RtlMultiByteToUnicodeN( (LPWSTR)lParam, wParam*sizeof(WCHAR), &len, ptr, strlen(ptr)+1 );
                    *result = len/sizeof(WCHAR) - 1;  /* do not count terminating null */
                }
                ((LPWSTR)lParam)[*result] = 0;
            }
            free_buffer( buffer, ptr );
        }
        break;

    case LB_ADDSTRING:
    case LB_INSERTSTRING:
    case LB_FINDSTRING:
    case LB_FINDSTRINGEXACT:
    case LB_SELECTSTRING:
    case CB_ADDSTRING:
    case CB_INSERTSTRING:
    case CB_FINDSTRING:
    case CB_FINDSTRINGEXACT:
    case CB_SELECTSTRING:
        if (!lParam || !WINPROC_TestLBForStr( hwnd, msg ))
        {
            ret = callback( hwnd, msg, wParam, lParam, result, arg );
            break;
        }
        /* fall through */
    case WM_SETTEXT:
    case WM_WININICHANGE:
    case WM_DEVMODECHANGE:
    case CB_DIR:
    case LB_DIR:
    case LB_ADDFILE:
    case EM_REPLACESEL:
        if (!lParam) ret = callback( hwnd, msg, wParam, lParam, result, arg );
        else
        {
            char *ptr, buffer[512];
            LPCWSTR strW = (LPCWSTR)lParam;
            DWORD lenA, lenW = (strlenW(strW) + 1) * sizeof(WCHAR);

            RtlUnicodeToMultiByteSize( &lenA, strW, lenW );
            if ((ptr = get_buffer( buffer, sizeof(buffer), lenA )))
            {
                RtlUnicodeToMultiByteN( ptr, lenA, NULL, strW, lenW );
                ret = callback( hwnd, msg, wParam, (LPARAM)ptr, result, arg );
                free_buffer( buffer, ptr );
            }
        }
        break;

    case WM_MDICREATE:
        {
            char *ptr, buffer[1024];
            DWORD title_lenA = 0, title_lenW = 0, class_lenA = 0, class_lenW = 0;
            MDICREATESTRUCTW *csW = (MDICREATESTRUCTW *)lParam;
            MDICREATESTRUCTA csA;

            memcpy( &csA, csW, sizeof(csA) );

            if (!IS_INTRESOURCE(csW->szTitle))
            {
                title_lenW = (strlenW(csW->szTitle) + 1) * sizeof(WCHAR);
                RtlUnicodeToMultiByteSize( &title_lenA, csW->szTitle, title_lenW );
            }
            if (!IS_INTRESOURCE(csW->szClass))
            {
                class_lenW = (strlenW(csW->szClass) + 1) * sizeof(WCHAR);
                RtlUnicodeToMultiByteSize( &class_lenA, csW->szClass, class_lenW );
            }

            if (!(ptr = get_buffer( buffer, sizeof(buffer), title_lenA + class_lenA ))) break;

            if (title_lenA)
            {
                RtlUnicodeToMultiByteN( ptr, title_lenA, NULL, csW->szTitle, title_lenW );
                csA.szTitle = ptr;
            }
            if (class_lenA)
            {
                RtlUnicodeToMultiByteN( ptr + title_lenA, class_lenA, NULL, csW->szClass, class_lenW );
                csA.szClass = ptr + title_lenA;
            }
            ret = callback( hwnd, msg, wParam, (LPARAM)&csA, result, arg );
            free_buffer( buffer, ptr );
        }
        break;

    case LB_GETTEXT:
    case CB_GETLBTEXT:
        if (lParam && WINPROC_TestLBForStr( hwnd, msg ))
        {
            char buffer[512];  /* FIXME: fixed sized buffer */

            ret = callback( hwnd, msg, wParam, (LPARAM)buffer, result, arg );
            if (*result >= 0)
            {
                DWORD len;
                RtlMultiByteToUnicodeN( (LPWSTR)lParam, ~0u, &len, buffer, strlen(buffer) + 1 );
                *result = len / sizeof(WCHAR) - 1;
            }
        }
        else ret = callback( hwnd, msg, wParam, lParam, result, arg );
        break;

    case EM_GETLINE:
        {
            char *ptr, buffer[512];
            WORD len = *(WORD *)lParam;

            if (!(ptr = get_buffer( buffer, sizeof(buffer), len * 2 ))) break;
            *((WORD *)ptr) = len * 2;   /* store the length */
            ret = callback( hwnd, msg, wParam, (LPARAM)ptr, result, arg );
            if (*result)
            {
                DWORD reslen;
                RtlMultiByteToUnicodeN( (LPWSTR)lParam, len*sizeof(WCHAR), &reslen, ptr, *result );
                *result = reslen / sizeof(WCHAR);
                if (*result < len) ((LPWSTR)lParam)[*result] = 0;
            }
            free_buffer( buffer, ptr );
        }
        break;

    case WM_GETDLGCODE:
        if (lParam)
        {
            MSG newmsg = *(MSG *)lParam;
            switch(newmsg.message)
            {
            case WM_CHAR:
            case WM_DEADCHAR:
            case WM_SYSCHAR:
            case WM_SYSDEADCHAR:
                newmsg.wParam = map_wparam_char_WtoA( newmsg.wParam, 1 );
                break;
            case WM_IME_CHAR:
                newmsg.wParam = map_wparam_char_WtoA( newmsg.wParam, 2 );
                break;
            }
            ret = callback( hwnd, msg, wParam, (LPARAM)&newmsg, result, arg );
        }
        else ret = callback( hwnd, msg, wParam, lParam, result, arg );
        break;

    case WM_CHAR:
        {
            WCHAR wch = wParam;
            char ch[2];
            DWORD len;

            RtlUnicodeToMultiByteN( ch, 2, &len, &wch, sizeof(wch) );
            ret = callback( hwnd, msg, (BYTE)ch[0], lParam, result, arg );
            if (len == 2) ret = callback( hwnd, msg, (BYTE)ch[1], lParam, result, arg );
        }
        break;

    case WM_CHARTOITEM:
    case WM_MENUCHAR:
    case WM_DEADCHAR:
    case WM_SYSCHAR:
    case WM_SYSDEADCHAR:
    case EM_SETPASSWORDCHAR:
        ret = callback( hwnd, msg, map_wparam_char_WtoA(wParam,1), lParam, result, arg );
        break;

    case WM_IME_CHAR:
        ret = callback( hwnd, msg, map_wparam_char_WtoA(wParam,2), lParam, result, arg );
        break;

    case WM_PAINTCLIPBOARD:
    case WM_SIZECLIPBOARD:
        FIXME_(msg)( "message %s (%04x) needs translation, please report\n",
                     SPY_GetMsgName(msg, hwnd), msg );
        break;

    default:
        ret = callback( hwnd, msg, wParam, lParam, result, arg );
        break;
    }

    return ret;
}


/**********************************************************************
 *	     WINPROC_CallProc16To32A
 */
LRESULT WINPROC_CallProc16To32A( winproc_callback_t callback, HWND16 hwnd, UINT16 msg,
                                 WPARAM16 wParam, LPARAM lParam, LRESULT *result, void *arg )
{
    LRESULT ret = 0;
    HWND hwnd32 = WIN_Handle32( hwnd );

    TRACE_(msg)("(hwnd=%p,msg=%s,wp=%08x,lp=%08lx)\n",
                 hwnd32, SPY_GetMsgName(msg, hwnd32), wParam, lParam);

    switch(msg)
    {
    case WM_NCCREATE:
    case WM_CREATE:
        {
            CREATESTRUCT16 *cs16 = MapSL(lParam);
            CREATESTRUCTA cs;
            MDICREATESTRUCTA mdi_cs;

            CREATESTRUCT16to32A( cs16, &cs );
            if (GetWindowLongW(hwnd32, GWL_EXSTYLE) & WS_EX_MDICHILD)
            {
                MDICREATESTRUCT16 *mdi_cs16 = MapSL(cs16->lpCreateParams);
                MDICREATESTRUCT16to32A(mdi_cs16, &mdi_cs);
                cs.lpCreateParams = &mdi_cs;
            }
            ret = callback( hwnd32, msg, wParam, (LPARAM)&cs, result, arg );
            CREATESTRUCT32Ato16( &cs, cs16 );
        }
        break;
    case WM_MDICREATE:
        {
            MDICREATESTRUCT16 *cs16 = MapSL(lParam);
            MDICREATESTRUCTA cs;

            MDICREATESTRUCT16to32A( cs16, &cs );
            ret = callback( hwnd32, msg, wParam, (LPARAM)&cs, result, arg );
            MDICREATESTRUCT32Ato16( &cs, cs16 );
        }
        break;
    case WM_MDIACTIVATE:
        if (lParam)
            ret = callback( hwnd32, msg, (WPARAM)WIN_Handle32( HIWORD(lParam) ),
                            (LPARAM)WIN_Handle32( LOWORD(lParam) ), result, arg );
        else /* message sent to MDI client */
            ret = callback( hwnd32, msg, wParam, lParam, result, arg );
        break;
    case WM_MDIGETACTIVE:
        {
            BOOL maximized = FALSE;
            ret = callback( hwnd32, msg, wParam, (LPARAM)&maximized, result, arg );
            *result = MAKELRESULT( LOWORD(*result), maximized );
        }
        break;
    case WM_MDISETMENU:
        ret = callback( hwnd32, wParam ? WM_MDIREFRESHMENU : WM_MDISETMENU,
                        (WPARAM)HMENU_32(LOWORD(lParam)), (LPARAM)HMENU_32(HIWORD(lParam)),
                        result, arg );
        break;
    case WM_GETMINMAXINFO:
        {
            MINMAXINFO16 *mmi16 = MapSL(lParam);
            MINMAXINFO mmi;

            MINMAXINFO16to32( mmi16, &mmi );
            ret = callback( hwnd32, msg, wParam, (LPARAM)&mmi, result, arg );
            MINMAXINFO32to16( &mmi, mmi16 );
        }
        break;
    case WM_WINDOWPOSCHANGING:
    case WM_WINDOWPOSCHANGED:
        {
            WINDOWPOS16 *winpos16 = MapSL(lParam);
            WINDOWPOS winpos;

            WINDOWPOS16to32( winpos16, &winpos );
            ret = callback( hwnd32, msg, wParam, (LPARAM)&winpos, result, arg );
            WINDOWPOS32to16( &winpos, winpos16 );
        }
        break;
    case WM_NCCALCSIZE:
        {
            NCCALCSIZE_PARAMS16 *nc16 = MapSL(lParam);
            NCCALCSIZE_PARAMS nc;
            WINDOWPOS winpos;

            RECT16to32( &nc16->rgrc[0], &nc.rgrc[0] );
            if (wParam)
            {
                RECT16to32( &nc16->rgrc[1], &nc.rgrc[1] );
                RECT16to32( &nc16->rgrc[2], &nc.rgrc[2] );
                WINDOWPOS16to32( MapSL(nc16->lppos), &winpos );
                nc.lppos = &winpos;
            }
            ret = callback( hwnd32, msg, wParam, (LPARAM)&nc, result, arg );
            RECT32to16( &nc.rgrc[0], &nc16->rgrc[0] );
            if (wParam)
            {
                RECT32to16( &nc.rgrc[1], &nc16->rgrc[1] );
                RECT32to16( &nc.rgrc[2], &nc16->rgrc[2] );
                WINDOWPOS32to16( &winpos, MapSL(nc16->lppos) );
            }
        }
        break;
    case WM_COMPAREITEM:
        {
            COMPAREITEMSTRUCT16* cis16 = MapSL(lParam);
            COMPAREITEMSTRUCT cis;
            cis.CtlType    = cis16->CtlType;
            cis.CtlID      = cis16->CtlID;
            cis.hwndItem   = WIN_Handle32( cis16->hwndItem );
            cis.itemID1    = cis16->itemID1;
            cis.itemData1  = cis16->itemData1;
            cis.itemID2    = cis16->itemID2;
            cis.itemData2  = cis16->itemData2;
            cis.dwLocaleId = 0;  /* FIXME */
            ret = callback( hwnd32, msg, wParam, (LPARAM)&cis, result, arg );
        }
        break;
    case WM_DELETEITEM:
        {
            DELETEITEMSTRUCT16* dis16 = MapSL(lParam);
            DELETEITEMSTRUCT dis;
            dis.CtlType  = dis16->CtlType;
            dis.CtlID    = dis16->CtlID;
            dis.hwndItem = WIN_Handle32( dis16->hwndItem );
            dis.itemData = dis16->itemData;
            ret = callback( hwnd32, msg, wParam, (LPARAM)&dis, result, arg );
        }
        break;
    case WM_MEASUREITEM:
        {
            MEASUREITEMSTRUCT16* mis16 = MapSL(lParam);
            MEASUREITEMSTRUCT mis;
            mis.CtlType    = mis16->CtlType;
            mis.CtlID      = mis16->CtlID;
            mis.itemID     = mis16->itemID;
            mis.itemWidth  = mis16->itemWidth;
            mis.itemHeight = mis16->itemHeight;
            mis.itemData   = mis16->itemData;
            ret = callback( hwnd32, msg, wParam, (LPARAM)&mis, result, arg );
            mis16->itemWidth  = (UINT16)mis.itemWidth;
            mis16->itemHeight = (UINT16)mis.itemHeight;
        }
        break;
    case WM_DRAWITEM:
        {
            DRAWITEMSTRUCT16* dis16 = MapSL(lParam);
            DRAWITEMSTRUCT dis;
            dis.CtlType       = dis16->CtlType;
            dis.CtlID         = dis16->CtlID;
            dis.itemID        = dis16->itemID;
            dis.itemAction    = dis16->itemAction;
            dis.itemState     = dis16->itemState;
            dis.hwndItem      = (dis.CtlType == ODT_MENU) ? (HWND)HMENU_32(dis16->hwndItem)
                                                          : WIN_Handle32( dis16->hwndItem );
            dis.hDC           = HDC_32(dis16->hDC);
            dis.itemData      = dis16->itemData;
            dis.rcItem.left   = dis16->rcItem.left;
            dis.rcItem.top    = dis16->rcItem.top;
            dis.rcItem.right  = dis16->rcItem.right;
            dis.rcItem.bottom = dis16->rcItem.bottom;
            ret = callback( hwnd32, msg, wParam, (LPARAM)&dis, result, arg );
        }
        break;
    case WM_COPYDATA:
        {
            COPYDATASTRUCT16 *cds16 = MapSL(lParam);
            COPYDATASTRUCT cds;
            cds.dwData = cds16->dwData;
            cds.cbData = cds16->cbData;
            cds.lpData = MapSL(cds16->lpData);
            ret = callback( hwnd32, msg, wParam, (LPARAM)&cds, result, arg );
        }
        break;
    case WM_GETDLGCODE:
        if (lParam)
        {
            MSG16 *msg16 = MapSL(lParam);
            MSG msg32;
            msg32.hwnd    = WIN_Handle32( msg16->hwnd );
            msg32.message = msg16->message;
            msg32.wParam  = msg16->wParam;
            msg32.lParam  = msg16->lParam;
            msg32.time    = msg16->time;
            msg32.pt.x    = msg16->pt.x;
            msg32.pt.y    = msg16->pt.y;
            ret = callback( hwnd32, msg, wParam, (LPARAM)&msg32, result, arg );
        }
        else
            ret = callback( hwnd32, msg, wParam, lParam, result, arg );
        break;
    case WM_NEXTMENU:
        {
            MDINEXTMENU next;
            next.hmenuIn   = (HMENU)lParam;
            next.hmenuNext = 0;
            next.hwndNext  = 0;
            ret = callback( hwnd32, msg, wParam, (LPARAM)&next, result, arg );
            *result = MAKELONG( HMENU_16(next.hmenuNext), HWND_16(next.hwndNext) );
        }
        break;
    case WM_ACTIVATE:
    case WM_CHARTOITEM:
    case WM_COMMAND:
    case WM_VKEYTOITEM:
        ret = callback( hwnd32, msg, MAKEWPARAM( wParam, HIWORD(lParam) ),
                        (LPARAM)WIN_Handle32( LOWORD(lParam) ), result, arg );
        break;
    case WM_HSCROLL:
    case WM_VSCROLL:
        ret = callback( hwnd32, msg, MAKEWPARAM( wParam, LOWORD(lParam) ),
                        (LPARAM)WIN_Handle32( HIWORD(lParam) ), result, arg );
        break;
    case WM_CTLCOLOR:
        if (HIWORD(lParam) <= CTLCOLOR_STATIC)
            ret = callback( hwnd32, WM_CTLCOLORMSGBOX + HIWORD(lParam),
                            (WPARAM)HDC_32(wParam), (LPARAM)WIN_Handle32( LOWORD(lParam) ),
                            result, arg );
        break;
    case WM_GETTEXT:
    case WM_SETTEXT:
    case WM_WININICHANGE:
    case WM_DEVMODECHANGE:
    case WM_ASKCBFORMATNAME:
    case WM_NOTIFY:
        ret = callback( hwnd32, msg, wParam, (LPARAM)MapSL(lParam), result, arg );
        break;
    case WM_MENUCHAR:
        ret = callback( hwnd32, msg, MAKEWPARAM( wParam, LOWORD(lParam) ),
                        (LPARAM)HMENU_32(HIWORD(lParam)), result, arg );
        break;
    case WM_MENUSELECT:
        if((LOWORD(lParam) & MF_POPUP) && (LOWORD(lParam) != 0xFFFF))
        {
            HMENU hmenu = HMENU_32(HIWORD(lParam));
            UINT pos = MENU_FindSubMenu( &hmenu, HMENU_32(wParam) );
            if (pos == 0xffff) pos = 0;  /* NO_SELECTED_ITEM */
            wParam = pos;
        }
        ret = callback( hwnd32, msg, MAKEWPARAM( wParam, LOWORD(lParam) ),
                        (LPARAM)HMENU_32(HIWORD(lParam)), result, arg );
        break;
    case WM_PARENTNOTIFY:
        if ((wParam == WM_CREATE) || (wParam == WM_DESTROY))
            ret = callback( hwnd32, msg, MAKEWPARAM( wParam, HIWORD(lParam) ),
                            (LPARAM)WIN_Handle32( LOWORD(lParam) ), result, arg );
        else
            ret = callback( hwnd32, msg, wParam, lParam, result, arg );
        break;
    case WM_ACTIVATEAPP:
        /* We need this when SetActiveWindow sends a Sendmessage16() to
         * a 32-bit window. Might be superfluous with 32-bit interprocess
         * message queues. */
        if (lParam) lParam = HTASK_32(lParam);
        ret = callback( hwnd32, msg, wParam, lParam, result, arg );
        break;
    case WM_DDE_INITIATE:
    case WM_DDE_TERMINATE:
    case WM_DDE_UNADVISE:
    case WM_DDE_REQUEST:
        ret = callback( hwnd32, msg, (WPARAM)WIN_Handle32(wParam), lParam, result, arg );
        break;
    case WM_DDE_ADVISE:
    case WM_DDE_DATA:
    case WM_DDE_POKE:
        {
            HANDLE16 lo16 = LOWORD(lParam);
            UINT_PTR lo32 = 0;
            if (lo16 && !(lo32 = convert_handle_16_to_32(lo16, GMEM_DDESHARE))) break;
            lParam = PackDDElParam( msg, lo32, HIWORD(lParam) );
            ret = callback( hwnd32, msg, (WPARAM)WIN_Handle32(wParam), lParam, result, arg );
        }
        break; /* FIXME don't know how to free allocated memory (handle)  !! */
    case WM_DDE_ACK:
        {
            UINT_PTR lo = LOWORD(lParam);
            UINT_PTR hi = HIWORD(lParam);
            int flag = 0;
            char buf[2];

            if (GlobalGetAtomNameA(hi, buf, 2) > 0) flag |= 1;
            if (GlobalSize16(hi) != 0) flag |= 2;
            switch (flag)
            {
            case 0:
                if (hi)
                {
                    MESSAGE("DDE_ACK: neither atom nor handle!!!\n");
                    hi = 0;
                }
                break;
            case 1:
                break; /* atom, nothing to do */
            case 3:
                MESSAGE("DDE_ACK: %lx both atom and handle... choosing handle\n", hi);
                /* fall thru */
            case 2:
                hi = convert_handle_16_to_32(hi, GMEM_DDESHARE);
                break;
            }
            lParam = PackDDElParam( WM_DDE_ACK, lo, hi );
            ret = callback( hwnd32, msg, (WPARAM)WIN_Handle32(wParam), lParam, result, arg );
        }
        break; /* FIXME don't know how to free allocated memory (handle) !! */
    case WM_DDE_EXECUTE:
        lParam = convert_handle_16_to_32( lParam, GMEM_DDESHARE );
        ret = callback( hwnd32, msg, wParam, lParam, result, arg );
        break; /* FIXME don't know how to free allocated memory (handle) !! */
    case WM_PAINTCLIPBOARD:
    case WM_SIZECLIPBOARD:
        FIXME_(msg)( "message %04x needs translation\n", msg );
        break;
    default:
        ret = callback( hwnd32, msg, wParam, lParam, result, arg );
        break;
    }
    return ret;
}


/**********************************************************************
 *	     __wine_call_wndproc   (USER.1010)
 */
LRESULT WINAPI __wine_call_wndproc( HWND16 hwnd, UINT16 msg, WPARAM16 wParam, LPARAM lParam,
                                    WINDOWPROC *proc )
{
    LRESULT result;

    if (proc->procA)
        WINPROC_CallProc16To32A( call_window_proc, hwnd, msg, wParam, lParam, &result, proc->procA );
    else
        WINPROC_CallProc16To32A( call_window_proc_AtoW, hwnd, msg, wParam, lParam, &result, proc->procW );
    return result;
}


/**********************************************************************
 *	     WINPROC_CallProc32ATo16
 *
 * Call a 16-bit window procedure, translating the 32-bit args.
 */
LRESULT WINPROC_CallProc32ATo16( winproc_callback16_t callback, HWND hwnd, UINT msg,
                                 WPARAM wParam, LPARAM lParam, LRESULT *result, void *arg )
{
    LRESULT ret = 0;

    TRACE_(msg)("(hwnd=%p,msg=%s,wp=%08lx,lp=%08lx)\n",
                hwnd, SPY_GetMsgName(msg, hwnd), wParam, lParam);

    switch(msg)
    {
    case WM_NCCREATE:
    case WM_CREATE:
        {
            CREATESTRUCTA *cs32 = (CREATESTRUCTA *)lParam;
            CREATESTRUCT16 cs;
            MDICREATESTRUCT16 mdi_cs16;
            BOOL mdi_child = (GetWindowLongW(hwnd, GWL_EXSTYLE) & WS_EX_MDICHILD);

            CREATESTRUCT32Ato16( cs32, &cs );
            cs.lpszName  = MapLS( cs32->lpszName );
            cs.lpszClass = MapLS( cs32->lpszClass );

            if (mdi_child)
            {
                MDICREATESTRUCTA *mdi_cs = cs32->lpCreateParams;
                MDICREATESTRUCT32Ato16( mdi_cs, &mdi_cs16 );
                mdi_cs16.szTitle = MapLS( mdi_cs->szTitle );
                mdi_cs16.szClass = MapLS( mdi_cs->szClass );
                cs.lpCreateParams = MapLS( &mdi_cs16 );
            }
            lParam = MapLS( &cs );
            ret = callback( HWND_16(hwnd), msg, wParam, lParam, result, arg );
            UnMapLS( lParam );
            UnMapLS( cs.lpszName );
            UnMapLS( cs.lpszClass );
            if (mdi_child)
            {
                UnMapLS( cs.lpCreateParams );
                UnMapLS( mdi_cs16.szTitle );
                UnMapLS( mdi_cs16.szClass );
            }
        }
        break;
    case WM_MDICREATE:
        {
            MDICREATESTRUCTA *cs32 = (MDICREATESTRUCTA *)lParam;
            MDICREATESTRUCT16 cs;

            MDICREATESTRUCT32Ato16( cs32, &cs );
            cs.szTitle = MapLS( cs32->szTitle );
            cs.szClass = MapLS( cs32->szClass );
            lParam = MapLS( &cs );
            ret = callback( HWND_16(hwnd), msg, wParam, lParam, result, arg );
            UnMapLS( lParam );
            UnMapLS( cs.szTitle );
            UnMapLS( cs.szClass );
        }
        break;
    case WM_MDIACTIVATE:
        if (GetWindowLongW( hwnd, GWL_EXSTYLE ) & WS_EX_MDICHILD)
            ret = callback( HWND_16(hwnd), msg, ((HWND)lParam == hwnd),
                            MAKELPARAM( LOWORD(lParam), LOWORD(wParam) ), result, arg );
        else
            ret = callback( HWND_16(hwnd), msg, HWND_16( wParam ), 0, result, arg );
        break;
    case WM_MDIGETACTIVE:
        ret = callback( HWND_16(hwnd), msg, wParam, lParam, result, arg );
        if (lParam) *(BOOL *)lParam = (BOOL16)HIWORD(*result);
        *result = (LRESULT)WIN_Handle32( LOWORD(*result) );
        break;
    case WM_MDISETMENU:
        ret = callback( HWND_16(hwnd), msg, (lParam == 0),
                        MAKELPARAM( LOWORD(wParam), LOWORD(lParam) ), result, arg );
        break;
    case WM_GETMINMAXINFO:
        {
            MINMAXINFO *mmi32 = (MINMAXINFO *)lParam;
            MINMAXINFO16 mmi;

            MINMAXINFO32to16( mmi32, &mmi );
            lParam = MapLS( &mmi );
            ret = callback( HWND_16(hwnd), msg, wParam, lParam, result, arg );
            UnMapLS( lParam );
            MINMAXINFO16to32( &mmi, mmi32 );
        }
        break;
    case WM_NCCALCSIZE:
        {
            NCCALCSIZE_PARAMS *nc32 = (NCCALCSIZE_PARAMS *)lParam;
            NCCALCSIZE_PARAMS16 nc;
            WINDOWPOS16 winpos;

            RECT32to16( &nc32->rgrc[0], &nc.rgrc[0] );
            if (wParam)
            {
                RECT32to16( &nc32->rgrc[1], &nc.rgrc[1] );
                RECT32to16( &nc32->rgrc[2], &nc.rgrc[2] );
                WINDOWPOS32to16( nc32->lppos, &winpos );
                nc.lppos = MapLS( &winpos );
            }
            lParam = MapLS( &nc );
            ret = callback( HWND_16(hwnd), msg, wParam, lParam, result, arg );
            UnMapLS( lParam );
            RECT16to32( &nc.rgrc[0], &nc32->rgrc[0] );
            if (wParam)
            {
                RECT16to32( &nc.rgrc[1], &nc32->rgrc[1] );
                RECT16to32( &nc.rgrc[2], &nc32->rgrc[2] );
                WINDOWPOS16to32( &winpos, nc32->lppos );
                UnMapLS( nc.lppos );
            }
        }
        break;
    case WM_WINDOWPOSCHANGING:
    case WM_WINDOWPOSCHANGED:
        {
            WINDOWPOS *winpos32 = (WINDOWPOS *)lParam;
            WINDOWPOS16 winpos;

            WINDOWPOS32to16( winpos32, &winpos );
            lParam = MapLS( &winpos );
            ret = callback( HWND_16(hwnd), msg, wParam, lParam, result, arg );
            UnMapLS( lParam );
            WINDOWPOS16to32( &winpos, winpos32 );
        }
        break;
    case WM_COMPAREITEM:
        {
            COMPAREITEMSTRUCT *cis32 = (COMPAREITEMSTRUCT *)lParam;
            COMPAREITEMSTRUCT16 cis;
            cis.CtlType    = cis32->CtlType;
            cis.CtlID      = cis32->CtlID;
            cis.hwndItem   = HWND_16( cis32->hwndItem );
            cis.itemID1    = cis32->itemID1;
            cis.itemData1  = cis32->itemData1;
            cis.itemID2    = cis32->itemID2;
            cis.itemData2  = cis32->itemData2;
            lParam = MapLS( &cis );
            ret = callback( HWND_16(hwnd), msg, wParam, lParam, result, arg );
            UnMapLS( lParam );
        }
        break;
    case WM_DELETEITEM:
        {
            DELETEITEMSTRUCT *dis32 = (DELETEITEMSTRUCT *)lParam;
            DELETEITEMSTRUCT16 dis;
            dis.CtlType  = dis32->CtlType;
            dis.CtlID    = dis32->CtlID;
            dis.itemID   = dis32->itemID;
            dis.hwndItem = (dis.CtlType == ODT_MENU) ? (HWND16)LOWORD(dis32->hwndItem)
                                                     : HWND_16( dis32->hwndItem );
            dis.itemData = dis32->itemData;
            lParam = MapLS( &dis );
            ret = callback( HWND_16(hwnd), msg, wParam, lParam, result, arg );
            UnMapLS( lParam );
        }
        break;
    case WM_DRAWITEM:
        {
            DRAWITEMSTRUCT *dis32 = (DRAWITEMSTRUCT *)lParam;
            DRAWITEMSTRUCT16 dis;
            dis.CtlType       = dis32->CtlType;
            dis.CtlID         = dis32->CtlID;
            dis.itemID        = dis32->itemID;
            dis.itemAction    = dis32->itemAction;
            dis.itemState     = dis32->itemState;
            dis.hwndItem      = HWND_16( dis32->hwndItem );
            dis.hDC           = HDC_16(dis32->hDC);
            dis.itemData      = dis32->itemData;
            dis.rcItem.left   = dis32->rcItem.left;
            dis.rcItem.top    = dis32->rcItem.top;
            dis.rcItem.right  = dis32->rcItem.right;
            dis.rcItem.bottom = dis32->rcItem.bottom;
            lParam = MapLS( &dis );
            ret = callback( HWND_16(hwnd), msg, wParam, lParam, result, arg );
            UnMapLS( lParam );
        }
        break;
    case WM_MEASUREITEM:
        {
            MEASUREITEMSTRUCT *mis32 = (MEASUREITEMSTRUCT *)lParam;
            MEASUREITEMSTRUCT16 mis;
            mis.CtlType    = mis32->CtlType;
            mis.CtlID      = mis32->CtlID;
            mis.itemID     = mis32->itemID;
            mis.itemWidth  = mis32->itemWidth;
            mis.itemHeight = mis32->itemHeight;
            mis.itemData   = mis32->itemData;
            lParam = MapLS( &mis );
            ret = callback( HWND_16(hwnd), msg, wParam, lParam, result, arg );
            UnMapLS( lParam );
            mis32->itemWidth  = mis.itemWidth;
            mis32->itemHeight = mis.itemHeight;
        }
        break;
    case WM_COPYDATA:
        {
            COPYDATASTRUCT *cds32 = (COPYDATASTRUCT *)lParam;
            COPYDATASTRUCT16 cds;

            cds.dwData = cds32->dwData;
            cds.cbData = cds32->cbData;
            cds.lpData = MapLS( cds32->lpData );
            lParam = MapLS( &cds );
            ret = callback( HWND_16(hwnd), msg, wParam, lParam, result, arg );
            UnMapLS( lParam );
            UnMapLS( cds.lpData );
        }
        break;
    case WM_GETDLGCODE:
        if (lParam)
        {
            MSG *msg32 = (MSG *)lParam;
            MSG16 msg16;

            msg16.hwnd    = HWND_16( msg32->hwnd );
            msg16.message = msg32->message;
            msg16.wParam  = msg32->wParam;
            msg16.lParam  = msg32->lParam;
            msg16.time    = msg32->time;
            msg16.pt.x    = msg32->pt.x;
            msg16.pt.y    = msg32->pt.y;
            lParam = MapLS( &msg16 );
            ret = callback( HWND_16(hwnd), msg, wParam, lParam, result, arg );
            UnMapLS( lParam );
        }
        else
            ret = callback( HWND_16(hwnd), msg, wParam, lParam, result, arg );
        break;
    case WM_NEXTMENU:
        {
            MDINEXTMENU *next = (MDINEXTMENU *)lParam;
            ret = callback( HWND_16(hwnd), msg, wParam, (LPARAM)next->hmenuIn, result, arg );
            next->hmenuNext = HMENU_32( LOWORD(*result) );
            next->hwndNext  = WIN_Handle32( HIWORD(*result) );
            *result = 0;
        }
        break;
    case WM_GETTEXT:
    case WM_ASKCBFORMATNAME:
        wParam = min( wParam, 0xff80 ); /* Must be < 64K */
        /* fall through */
    case WM_NOTIFY:
    case WM_SETTEXT:
    case WM_WININICHANGE:
    case WM_DEVMODECHANGE:
        lParam = MapLS( (void *)lParam );
        ret = callback( HWND_16(hwnd), msg, wParam, lParam, result, arg );
        UnMapLS( lParam );
        break;
    case WM_ACTIVATE:
    case WM_CHARTOITEM:
    case WM_COMMAND:
    case WM_VKEYTOITEM:
        ret = callback( HWND_16(hwnd), msg, wParam, MAKELPARAM( (HWND16)lParam, HIWORD(wParam) ),
                        result, arg );
        break;
    case WM_HSCROLL:
    case WM_VSCROLL:
        ret = callback( HWND_16(hwnd), msg, wParam, MAKELPARAM( HIWORD(wParam), (HWND16)lParam ),
                        result, arg );
        break;
    case WM_CTLCOLORMSGBOX:
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORLISTBOX:
    case WM_CTLCOLORBTN:
    case WM_CTLCOLORDLG:
    case WM_CTLCOLORSCROLLBAR:
    case WM_CTLCOLORSTATIC:
        ret = callback( HWND_16(hwnd), WM_CTLCOLOR, wParam,
                        MAKELPARAM( (HWND16)lParam, msg - WM_CTLCOLORMSGBOX ), result, arg );
        break;
    case WM_MENUSELECT:
        if(HIWORD(wParam) & MF_POPUP)
        {
            HMENU hmenu;
            if ((HIWORD(wParam) != 0xffff) || lParam)
            {
                if ((hmenu = GetSubMenu( (HMENU)lParam, LOWORD(wParam) )))
                {
                    ret = callback( HWND_16(hwnd), msg, HMENU_16(hmenu),
                                    MAKELPARAM( HIWORD(wParam), (HMENU16)lParam ), result, arg );
                    break;
                }
            }
        }
        /* fall through */
    case WM_MENUCHAR:
        ret = callback( HWND_16(hwnd), msg, wParam,
                        MAKELPARAM( HIWORD(wParam), (HMENU16)lParam ), result, arg );
        break;
    case WM_PARENTNOTIFY:
        if ((LOWORD(wParam) == WM_CREATE) || (LOWORD(wParam) == WM_DESTROY))
            ret = callback( HWND_16(hwnd), msg, wParam,
                            MAKELPARAM( (HWND16)lParam, HIWORD(wParam) ), result, arg );
        else
            ret = callback( HWND_16(hwnd), msg, wParam, lParam, result, arg );
        break;
    case WM_ACTIVATEAPP:
        ret = callback( HWND_16(hwnd), msg, wParam, HTASK_16( lParam ), result, arg );
        break;
    case WM_PAINT:
        if (IsIconic( hwnd ) && GetClassLongPtrW( hwnd, GCLP_HICON ))
            ret = callback( HWND_16(hwnd), WM_PAINTICON, 1, lParam, result, arg );
        else
            ret = callback( HWND_16(hwnd), WM_PAINT, wParam, lParam, result, arg );
        break;
    case WM_ERASEBKGND:
        if (IsIconic( hwnd ) && GetClassLongPtrW( hwnd, GCLP_HICON )) msg = WM_ICONERASEBKGND;
        ret = callback( HWND_16(hwnd), msg, wParam, lParam, result, arg );
        break;
    case WM_DDE_INITIATE:
    case WM_DDE_TERMINATE:
    case WM_DDE_UNADVISE:
    case WM_DDE_REQUEST:
        ret = callback( HWND_16(hwnd), msg, HWND_16(wParam), lParam, result, arg );
        break;
    case WM_DDE_ADVISE:
    case WM_DDE_DATA:
    case WM_DDE_POKE:
        {
            UINT_PTR lo32, hi;
            HANDLE16 lo16 = 0;

            UnpackDDElParam( msg, lParam, &lo32, &hi );
            if (lo32 && !(lo16 = convert_handle_32_to_16(lo32, GMEM_DDESHARE))) break;
            ret = callback( HWND_16(hwnd), msg, HWND_16(wParam),
                            MAKELPARAM(lo16, hi), result, arg );
        }
        break; /* FIXME don't know how to free allocated memory (handle)  !! */
    case WM_DDE_ACK:
        {
            UINT_PTR lo, hi;
            int flag = 0;
            char buf[2];

            UnpackDDElParam( msg, lParam, &lo, &hi );

            if (GlobalGetAtomNameA((ATOM)hi, buf, sizeof(buf)) > 0) flag |= 1;
            if (GlobalSize((HANDLE)hi) != 0) flag |= 2;
            switch (flag)
            {
            case 0:
                if (hi)
                {
                    MESSAGE("DDE_ACK: neither atom nor handle!!!\n");
                    hi = 0;
                }
                break;
            case 1:
                break; /* atom, nothing to do */
            case 3:
                MESSAGE("DDE_ACK: %lx both atom and handle... choosing handle\n", hi);
                /* fall thru */
            case 2:
                hi = convert_handle_32_to_16(hi, GMEM_DDESHARE);
                break;
            }
            ret = callback( HWND_16(hwnd), msg, HWND_16(wParam),
                            MAKELPARAM(lo, hi), result, arg );
        }
        break; /* FIXME don't know how to free allocated memory (handle) !! */
    case WM_DDE_EXECUTE:
        lParam = convert_handle_32_to_16(lParam, GMEM_DDESHARE);
        ret = callback( HWND_16(hwnd), msg, wParam, lParam, result, arg );
        break; /* FIXME don't know how to free allocated memory (handle) !! */
    case SBM_SETRANGE:
        ret = callback( HWND_16(hwnd), SBM_SETRANGE16, 0, MAKELPARAM(wParam, lParam), result, arg );
        break;
    case SBM_GETRANGE:
        ret = callback( HWND_16(hwnd), SBM_GETRANGE16, wParam, lParam, result, arg );
        *(LPINT)wParam = LOWORD(*result);
        *(LPINT)lParam = HIWORD(*result);
        break;
    case BM_GETCHECK:
    case BM_SETCHECK:
    case BM_GETSTATE:
    case BM_SETSTATE:
    case BM_SETSTYLE:
        ret = callback( HWND_16(hwnd), msg + BM_GETCHECK16 - BM_GETCHECK, wParam, lParam, result, arg );
        break;
    case EM_GETSEL:
    case EM_GETRECT:
    case EM_SETRECT:
    case EM_SETRECTNP:
    case EM_SCROLL:
    case EM_LINESCROLL:
    case EM_SCROLLCARET:
    case EM_GETMODIFY:
    case EM_SETMODIFY:
    case EM_GETLINECOUNT:
    case EM_LINEINDEX:
    case EM_SETHANDLE:
    case EM_GETHANDLE:
    case EM_GETTHUMB:
    case EM_LINELENGTH:
    case EM_REPLACESEL:
    case EM_GETLINE:
    case EM_LIMITTEXT:
    case EM_CANUNDO:
    case EM_UNDO:
    case EM_FMTLINES:
    case EM_LINEFROMCHAR:
    case EM_SETTABSTOPS:
    case EM_SETPASSWORDCHAR:
    case EM_EMPTYUNDOBUFFER:
    case EM_GETFIRSTVISIBLELINE:
    case EM_SETREADONLY:
    case EM_SETWORDBREAKPROC:
    case EM_GETWORDBREAKPROC:
    case EM_GETPASSWORDCHAR:
        ret = callback( HWND_16(hwnd), msg + EM_GETSEL16 - EM_GETSEL, wParam, lParam, result, arg );
        break;
    case EM_SETSEL:
        ret = callback( HWND_16(hwnd), EM_SETSEL16, 0, MAKELPARAM( wParam, lParam ), result, arg );
        break;
    case LB_CARETOFF:
    case LB_CARETON:
    case LB_DELETESTRING:
    case LB_GETANCHORINDEX:
    case LB_GETCARETINDEX:
    case LB_GETCOUNT:
    case LB_GETCURSEL:
    case LB_GETHORIZONTALEXTENT:
    case LB_GETITEMDATA:
    case LB_GETITEMHEIGHT:
    case LB_GETSEL:
    case LB_GETSELCOUNT:
    case LB_GETTEXTLEN:
    case LB_GETTOPINDEX:
    case LB_RESETCONTENT:
    case LB_SELITEMRANGE:
    case LB_SELITEMRANGEEX:
    case LB_SETANCHORINDEX:
    case LB_SETCARETINDEX:
    case LB_SETCOLUMNWIDTH:
    case LB_SETCURSEL:
    case LB_SETHORIZONTALEXTENT:
    case LB_SETITEMDATA:
    case LB_SETITEMHEIGHT:
    case LB_SETSEL:
    case LB_SETTOPINDEX:
        ret = callback( HWND_16(hwnd), msg + LB_ADDSTRING16 - LB_ADDSTRING, wParam, lParam, result, arg );
        break;
    case LB_ADDSTRING:
    case LB_FINDSTRING:
    case LB_FINDSTRINGEXACT:
    case LB_INSERTSTRING:
    case LB_SELECTSTRING:
    case LB_GETTEXT:
    case LB_DIR:
    case LB_ADDFILE:
        lParam = MapLS( (LPSTR)lParam );
        ret = callback( HWND_16(hwnd), msg + LB_ADDSTRING16 - LB_ADDSTRING, wParam, lParam, result, arg );
        UnMapLS( lParam );
        break;
    case LB_GETSELITEMS:
        {
            INT *items32 = (INT *)lParam;
            INT16 *items, buffer[512];
            unsigned int i;

            wParam = min( wParam, 0x7f80 ); /* Must be < 64K */
            if (!(items = get_buffer( buffer, sizeof(buffer), wParam * sizeof(INT16) ))) break;
            lParam = MapLS( items );
            ret = callback( HWND_16(hwnd), LB_GETSELITEMS16, wParam, lParam, result, arg );
            UnMapLS( lParam );
            for (i = 0; i < wParam; i++) items32[i] = items[i];
            free_buffer( buffer, items );
        }
        break;
    case LB_SETTABSTOPS:
        if (wParam)
        {
            INT *stops32 = (INT *)lParam;
            INT16 *stops, buffer[512];
            unsigned int i;

            wParam = min( wParam, 0x7f80 ); /* Must be < 64K */
            if (!(stops = get_buffer( buffer, sizeof(buffer), wParam * sizeof(INT16) ))) break;
            for (i = 0; i < wParam; i++) stops[i] = stops32[i];
            lParam = MapLS( stops );
            ret = callback( HWND_16(hwnd), LB_SETTABSTOPS16, wParam, lParam, result, arg );
            UnMapLS( lParam );
            free_buffer( buffer, stops );
        }
        else ret = callback( HWND_16(hwnd), LB_SETTABSTOPS16, wParam, lParam, result, arg );
        break;
    case CB_DELETESTRING:
    case CB_GETCOUNT:
    case CB_GETLBTEXTLEN:
    case CB_LIMITTEXT:
    case CB_RESETCONTENT:
    case CB_SETEDITSEL:
    case CB_GETCURSEL:
    case CB_SETCURSEL:
    case CB_SHOWDROPDOWN:
    case CB_SETITEMDATA:
    case CB_SETITEMHEIGHT:
    case CB_GETITEMHEIGHT:
    case CB_SETEXTENDEDUI:
    case CB_GETEXTENDEDUI:
    case CB_GETDROPPEDSTATE:
        ret = callback( HWND_16(hwnd), msg + CB_GETEDITSEL16 - CB_GETEDITSEL, wParam, lParam, result, arg );
        break;
    case CB_GETEDITSEL:
        ret = callback( HWND_16(hwnd), CB_GETEDITSEL16, wParam, lParam, result, arg );
        if (wParam) *((PUINT)(wParam)) = LOWORD(*result);
        if (lParam) *((PUINT)(lParam)) = HIWORD(*result);  /* FIXME: substract 1? */
        break;
    case CB_ADDSTRING:
    case CB_FINDSTRING:
    case CB_FINDSTRINGEXACT:
    case CB_INSERTSTRING:
    case CB_SELECTSTRING:
    case CB_DIR:
    case CB_GETLBTEXT:
        lParam = MapLS( (LPSTR)lParam );
        ret = callback( HWND_16(hwnd), msg + CB_GETEDITSEL16 - CB_GETEDITSEL, wParam, lParam, result, arg );
        UnMapLS( lParam );
        break;
    case LB_GETITEMRECT:
    case CB_GETDROPPEDCONTROLRECT:
        {
            RECT *r32 = (RECT *)lParam;
            RECT16 rect;
            lParam = MapLS( &rect );
            ret = callback( HWND_16(hwnd),
                            (msg == LB_GETITEMRECT) ? LB_GETITEMRECT16 : CB_GETDROPPEDCONTROLRECT16,
                            wParam, lParam, result, arg );
            UnMapLS( lParam );
            RECT16to32( &rect, r32 );
        }
        break;
    case WM_PAINTCLIPBOARD:
    case WM_SIZECLIPBOARD:
        FIXME_(msg)( "message %04x needs translation\n", msg );
        break;
    /* the following messages should not be sent to 16-bit apps */
    case WM_SIZING:
    case WM_MOVING:
    case WM_CAPTURECHANGED:
    case WM_STYLECHANGING:
    case WM_STYLECHANGED:
        break;
    default:
        ret = callback( HWND_16(hwnd), msg, wParam, lParam, result, arg );
        break;
    }
    return ret;
}


/**********************************************************************
 *		WINPROC_call_window
 *
 * Call the window procedure of the specified window.
 */
BOOL WINPROC_call_window( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam,
                          LRESULT *result, BOOL unicode, enum wm_char_mapping mapping )
{
    struct user_thread_info *thread_info = get_user_thread_info();
    WND *wndPtr;
    WINDOWPROC *proc;

    if (!(wndPtr = WIN_GetPtr( hwnd ))) return FALSE;
    if (wndPtr == WND_OTHER_PROCESS || wndPtr == WND_DESKTOP) return FALSE;
    if (wndPtr->tid != GetCurrentThreadId())
    {
        WIN_ReleasePtr( wndPtr );
        return FALSE;
    }
    proc = handle_to_proc( wndPtr->winproc );
    WIN_ReleasePtr( wndPtr );

    if (!proc) return TRUE;

    if (thread_info->recursion_count > MAX_WINPROC_RECURSION) return FALSE;
    thread_info->recursion_count++;

    if (unicode)
    {
        if (proc->procW)
            call_window_proc( hwnd, msg, wParam, lParam, result, proc->procW );
        else if (proc->procA)
            WINPROC_CallProcWtoA( call_window_proc, hwnd, msg, wParam, lParam, result, proc->procA );
        else
            WINPROC_CallProcWtoA( call_window_proc_Ato16, hwnd, msg, wParam, lParam, result, proc->proc16 );
    }
    else
    {
        if (proc->procA)
            call_window_proc( hwnd, msg, wParam, lParam, result, proc->procA );
        else if (proc->procW)
            WINPROC_CallProcAtoW( call_window_proc, hwnd, msg, wParam, lParam, result, proc->procW, mapping );
        else
            WINPROC_CallProc32ATo16( call_window_proc16, hwnd, msg, wParam, lParam, result, proc->proc16 );
    }
    thread_info->recursion_count--;
    return TRUE;
}


/**********************************************************************
 *		CallWindowProc (USER.122)
 */
LRESULT WINAPI CallWindowProc16( WNDPROC16 func, HWND16 hwnd, UINT16 msg,
                                 WPARAM16 wParam, LPARAM lParam )
{
    WINDOWPROC *proc;
    LRESULT result;

    if (!func) return 0;

    if (!(proc = handle16_to_proc( func )))
        call_window_proc16( hwnd, msg, wParam, lParam, &result, func );
    else if (proc->procA)
        WINPROC_CallProc16To32A( call_window_proc, hwnd, msg, wParam, lParam, &result, proc->procA );
    else if (proc->procW)
        WINPROC_CallProc16To32A( call_window_proc_AtoW, hwnd, msg, wParam, lParam, &result, proc->procW );
    else
        call_window_proc16( hwnd, msg, wParam, lParam, &result, proc->proc16 );

    return result;
}


/**********************************************************************
 *		CallWindowProcA (USER32.@)
 *
 * The CallWindowProc() function invokes the windows procedure _func_,
 * with _hwnd_ as the target window, the message specified by _msg_, and
 * the message parameters _wParam_ and _lParam_.
 *
 * Some kinds of argument conversion may be done, I'm not sure what.
 *
 * CallWindowProc() may be used for windows subclassing. Use
 * SetWindowLong() to set a new windows procedure for windows of the
 * subclass, and handle subclassed messages in the new windows
 * procedure. The new windows procedure may then use CallWindowProc()
 * with _func_ set to the parent class's windows procedure to dispatch
 * the message to the superclass.
 *
 * RETURNS
 *
 *    The return value is message dependent.
 *
 * CONFORMANCE
 *
 *   ECMA-234, Win32
 */
LRESULT WINAPI CallWindowProcA(
    WNDPROC func,  /* [in] window procedure */
    HWND hwnd,     /* [in] target window */
    UINT msg,      /* [in] message */
    WPARAM wParam, /* [in] message dependent parameter */
    LPARAM lParam  /* [in] message dependent parameter */
) {
    WINDOWPROC *proc;
    LRESULT result;

    if (!func) return 0;

    if (!(proc = handle_to_proc( func )))
        call_window_proc( hwnd, msg, wParam, lParam, &result, func );
    else if (proc->procA)
        call_window_proc( hwnd, msg, wParam, lParam, &result, proc->procA );
    else if (proc->procW)
        WINPROC_CallProcAtoW( call_window_proc, hwnd, msg, wParam, lParam, &result,
                              proc->procW, WMCHAR_MAP_CALLWINDOWPROC );
    else
        WINPROC_CallProc32ATo16( call_window_proc16, hwnd, msg, wParam, lParam, &result, proc->proc16 );
    return result;
}


/**********************************************************************
 *		CallWindowProcW (USER32.@)
 *
 * See CallWindowProcA.
 */
LRESULT WINAPI CallWindowProcW( WNDPROC func, HWND hwnd, UINT msg,
                                  WPARAM wParam, LPARAM lParam )
{
    WINDOWPROC *proc;
    LRESULT result;

    if (!func) return 0;

    if (!(proc = handle_to_proc( func )))
        call_window_proc( hwnd, msg, wParam, lParam, &result, func );
    else if (proc->procW)
        call_window_proc( hwnd, msg, wParam, lParam, &result, proc->procW );
    else if (proc->procA)
        WINPROC_CallProcWtoA( call_window_proc, hwnd, msg, wParam, lParam, &result, proc->procA );
    else
        WINPROC_CallProcWtoA( call_window_proc_Ato16, hwnd, msg, wParam, lParam, &result, proc->proc16 );
    return result;
}


/**********************************************************************
 *		WINPROC_CallDlgProc16
 */
INT_PTR WINPROC_CallDlgProc16( DLGPROC16 func, HWND16 hwnd, UINT16 msg, WPARAM16 wParam, LPARAM lParam )
{
    WINDOWPROC *proc;
    LRESULT result;
    INT_PTR ret;

    if (!func) return 0;

    if (!(proc = handle16_to_proc( (WNDPROC16)func )))
    {
        ret = call_dialog_proc16( hwnd, msg, wParam, lParam, &result, func );
    }
    else if (proc->procA)
    {
        ret = WINPROC_CallProc16To32A( call_dialog_proc, hwnd, msg, wParam, lParam,
                                       &result, proc->procA );
        SetWindowLongPtrW( WIN_Handle32(hwnd), DWLP_MSGRESULT, result );
    }
    else if (proc->procW)
    {
        ret = WINPROC_CallProc16To32A( call_dialog_proc_AtoW, hwnd, msg, wParam, lParam,
                                       &result, proc->procW );
        SetWindowLongPtrW( WIN_Handle32(hwnd), DWLP_MSGRESULT, result );
    }
    else
    {
        ret = call_dialog_proc16( hwnd, msg, wParam, lParam, &result, proc->proc16 );
    }
    return ret;
}


/**********************************************************************
 *		WINPROC_CallDlgProcA
 */
INT_PTR WINPROC_CallDlgProcA( DLGPROC func, HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    WINDOWPROC *proc;
    LRESULT result;
    INT_PTR ret;

    if (!func) return 0;

    if (!(proc = handle_to_proc( func )))
        ret = call_dialog_proc( hwnd, msg, wParam, lParam, &result, func );
    else if (proc->procA)
        ret = call_dialog_proc( hwnd, msg, wParam, lParam, &result, proc->procA );
    else if (proc->procW)
    {
        ret = WINPROC_CallProcAtoW( call_dialog_proc, hwnd, msg, wParam, lParam, &result,
                                    proc->procW, WMCHAR_MAP_CALLWINDOWPROC );
        SetWindowLongPtrW( hwnd, DWLP_MSGRESULT, result );
    }
    else
    {
        ret = WINPROC_CallProc32ATo16( call_dialog_proc16, hwnd, msg, wParam, lParam, &result, proc->proc16 );
        SetWindowLongPtrW( hwnd, DWLP_MSGRESULT, result );
    }
    return ret;
}


/**********************************************************************
 *		WINPROC_CallDlgProcW
 */
INT_PTR WINPROC_CallDlgProcW( DLGPROC func, HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    WINDOWPROC *proc;
    LRESULT result;
    INT_PTR ret;

    if (!func) return 0;

    if (!(proc = handle_to_proc( func )))
        ret = call_dialog_proc( hwnd, msg, wParam, lParam, &result, func );
    else if (proc->procW)
        ret = call_dialog_proc( hwnd, msg, wParam, lParam, &result, proc->procW );
    else if (proc->procA)
    {
        ret = WINPROC_CallProcWtoA( call_dialog_proc, hwnd, msg, wParam, lParam, &result, proc->procA );
        SetWindowLongPtrW( hwnd, DWLP_MSGRESULT, result );
    }
    else
    {
        ret = WINPROC_CallProcWtoA( call_dialog_proc_Ato16, hwnd, msg, wParam, lParam, &result, proc->proc16 );
        SetWindowLongPtrW( hwnd, DWLP_MSGRESULT, result );
    }
    return ret;
}
