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
#include "controls.h"
#include "win.h"
#include "user_private.h"
#include "wine/unicode.h"
#include "wine/debug.h"

WINE_DECLARE_DEBUG_CHANNEL(msg);
WINE_DECLARE_DEBUG_CHANNEL(relay);
WINE_DEFAULT_DEBUG_CHANNEL(win);

typedef struct tagWINDOWPROC
{
    WNDPROC        procA;    /* ASCII window proc */
    WNDPROC        procW;    /* Unicode window proc */
} WINDOWPROC;

#define MAX_WINPROCS  4096
#define MAX_WINPROC_RECURSION  64
#define WINPROC_PROC16  ((WINDOWPROC *)1)  /* placeholder for 16-bit window procs */

static LRESULT WINAPI ButtonWndProcA( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam );
static LRESULT WINAPI ButtonWndProcW( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam );
static LRESULT WINAPI ComboWndProcA( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam );
static LRESULT WINAPI ComboWndProcW( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam );
LRESULT WINAPI EditWndProcA( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam );
static LRESULT WINAPI EditWndProcW( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam );
static LRESULT WINAPI ListBoxWndProcA( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam );
static LRESULT WINAPI ListBoxWndProcW( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam );
static LRESULT WINAPI MDIClientWndProcA( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam );
static LRESULT WINAPI MDIClientWndProcW( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam );
static LRESULT WINAPI ScrollBarWndProcA( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam );
static LRESULT WINAPI ScrollBarWndProcW( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam );
static LRESULT WINAPI StaticWndProcA( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam );
static LRESULT WINAPI StaticWndProcW( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam );

static WINDOWPROC winproc_array[MAX_WINPROCS] =
{
    { ButtonWndProcA, ButtonWndProcW },        /* WINPROC_BUTTON */
    { ComboWndProcA, ComboWndProcW },          /* WINPROC_COMBO */
    { DefWindowProcA, DefWindowProcW },        /* WINPROC_DEFWND */
    { DefDlgProcA, DefDlgProcW },              /* WINPROC_DIALOG */
    { EditWndProcA, EditWndProcW },            /* WINPROC_EDIT */
    { ListBoxWndProcA, ListBoxWndProcW },      /* WINPROC_LISTBOX */
    { MDIClientWndProcA, MDIClientWndProcW },  /* WINPROC_MDICLIENT */
    { ScrollBarWndProcA, ScrollBarWndProcW },  /* WINPROC_SCROLLBAR */
    { StaticWndProcA, StaticWndProcW },        /* WINPROC_STATIC */
    { NULL, DesktopWndProc },                  /* WINPROC_DESKTOP */
    { NULL, IconTitleWndProc },                /* WINPROC_ICONTITLE */
    { NULL, PopupMenuWndProc },                /* WINPROC_MENU */
    { NULL, MessageWndProc },                  /* WINPROC_MESSAGE */
};

static UINT winproc_used = NB_BUILTIN_WINPROCS;

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

/* find an existing winproc for a given function and type */
/* FIXME: probably should do something more clever than a linear search */
static inline WINDOWPROC *find_winproc( WNDPROC func, BOOL unicode )
{
    unsigned int i;

    for (i = 0; i < NB_BUILTIN_AW_WINPROCS; i++)
    {
        /* match either proc, some apps confuse A and W */
        if (winproc_array[i].procA != func && winproc_array[i].procW != func) continue;
        return &winproc_array[i];
    }
    for (i = NB_BUILTIN_AW_WINPROCS; i < winproc_used; i++)
    {
        if (!unicode && winproc_array[i].procA != func) continue;
        if (unicode && winproc_array[i].procW != func) continue;
        return &winproc_array[i];
    }
    return NULL;
}

/* return the window proc for a given handle, or NULL for an invalid handle,
 * or WINPROC_PROC16 for a handle to a 16-bit proc. */
static inline WINDOWPROC *handle_to_proc( WNDPROC handle )
{
    UINT index = LOWORD(handle);
    if ((ULONG_PTR)handle >> 16 != WINPROC_HANDLE) return NULL;
    if (index >= MAX_WINPROCS) return WINPROC_PROC16;
    if (index >= winproc_used) return NULL;
    return &winproc_array[index];
}

/* create a handle for a given window proc */
static inline WNDPROC proc_to_handle( WINDOWPROC *proc )
{
    return (WNDPROC)(ULONG_PTR)((proc - winproc_array) | (WINPROC_HANDLE << 16));
}

/* allocate and initialize a new winproc */
static inline WINDOWPROC *alloc_winproc( WNDPROC func, BOOL unicode )
{
    WINDOWPROC *proc;

    /* check if the function is already a win proc */
    if (!func) return NULL;
    if ((proc = handle_to_proc( func ))) return proc;

    EnterCriticalSection( &winproc_cs );

    /* check if we already have a winproc for that function */
    if (!(proc = find_winproc( func, unicode )))
    {
        if (winproc_used < MAX_WINPROCS)
        {
            proc = &winproc_array[winproc_used++];
            if (unicode) proc->procW = func;
            else proc->procA = func;
            TRACE( "allocated %p for %c %p (%d/%d used)\n",
                   proc_to_handle(proc), unicode ? 'W' : 'A', func,
                   winproc_used, MAX_WINPROCS );
        }
        else FIXME( "too many winprocs, cannot allocate one for %p\n", func );
    }
    else TRACE( "reusing %p for %p\n", proc_to_handle(proc), func );

    LeaveCriticalSection( &winproc_cs );
    return proc;
}

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


/**********************************************************************
 *	     WINPROC_GetProc
 *
 * Get a window procedure pointer that can be passed to the Windows program.
 */
WNDPROC WINPROC_GetProc( WNDPROC proc, BOOL unicode )
{
    WINDOWPROC *ptr = handle_to_proc( proc );

    if (!ptr || ptr == WINPROC_PROC16) return proc;
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
 *	     WINPROC_AllocProc
 *
 * Allocate a window procedure for a window or class.
 *
 * Note that allocated winprocs are never freed; the idea is that even if an app creates a
 * lot of windows, it will usually only have a limited number of window procedures, so the
 * array won't grow too large, and this way we avoid the need to track allocations per window.
 */
WNDPROC WINPROC_AllocProc( WNDPROC func, BOOL unicode )
{
    WINDOWPROC *proc;

    if (!(proc = alloc_winproc( func, unicode ))) return NULL;
    if (proc == WINPROC_PROC16) return func;
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
    if (ptr == WINPROC_PROC16) return FALSE;  /* 16-bit is always A */
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
 *		WINPROC_call_window
 *
 * Call the window procedure of the specified window.
 */
BOOL WINPROC_call_window( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam,
                          LRESULT *result, BOOL unicode, enum wm_char_mapping mapping )
{
    struct user_thread_info *thread_info = get_user_thread_info();
    WND *wndPtr;
    WNDPROC func;
    WINDOWPROC *proc;

    if (!(wndPtr = WIN_GetPtr( hwnd ))) return FALSE;
    if (wndPtr == WND_OTHER_PROCESS || wndPtr == WND_DESKTOP) return FALSE;
    if (wndPtr->tid != GetCurrentThreadId())
    {
        WIN_ReleasePtr( wndPtr );
        return FALSE;
    }
    func = wndPtr->winproc;
    proc = handle_to_proc( wndPtr->winproc );
    WIN_ReleasePtr( wndPtr );

    if (!proc) return TRUE;

    if (thread_info->recursion_count > MAX_WINPROC_RECURSION) return FALSE;
    thread_info->recursion_count++;

    if (unicode)
    {
        if (proc == WINPROC_PROC16)
            WINPROC_CallProcWtoA( wow_handlers.call_window_proc, hwnd, msg, wParam, lParam, result, func );
        else if (proc->procW)
            call_window_proc( hwnd, msg, wParam, lParam, result, proc->procW );
        else
            WINPROC_CallProcWtoA( call_window_proc, hwnd, msg, wParam, lParam, result, proc->procA );
    }
    else
    {
        if (proc == WINPROC_PROC16)
            wow_handlers.call_window_proc( hwnd, msg, wParam, lParam, result, func );
        else if (proc->procA)
            call_window_proc( hwnd, msg, wParam, lParam, result, proc->procA );
        else
            WINPROC_CallProcAtoW( call_window_proc, hwnd, msg, wParam, lParam, result, proc->procW, mapping );
    }
    thread_info->recursion_count--;
    return TRUE;
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
    else if (proc == WINPROC_PROC16)
        wow_handlers.call_window_proc( hwnd, msg, wParam, lParam, &result, func );
    else if (proc->procA)
        call_window_proc( hwnd, msg, wParam, lParam, &result, proc->procA );
    else
        WINPROC_CallProcAtoW( call_window_proc, hwnd, msg, wParam, lParam, &result,
                              proc->procW, WMCHAR_MAP_CALLWINDOWPROC );
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
    else if (proc == WINPROC_PROC16)
        WINPROC_CallProcWtoA( wow_handlers.call_window_proc, hwnd, msg, wParam, lParam, &result, func );
    else if (proc->procW)
        call_window_proc( hwnd, msg, wParam, lParam, &result, proc->procW );
    else
        WINPROC_CallProcWtoA( call_window_proc, hwnd, msg, wParam, lParam, &result, proc->procA );
    return result;
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
    else if (proc == WINPROC_PROC16)
    {
        ret = wow_handlers.call_dialog_proc( hwnd, msg, wParam, lParam, &result, func );
        SetWindowLongPtrW( hwnd, DWLP_MSGRESULT, result );
    }
    else if (proc->procW)
    {
        ret = WINPROC_CallProcAtoW( call_dialog_proc, hwnd, msg, wParam, lParam, &result,
                                    proc->procW, WMCHAR_MAP_CALLWINDOWPROC );
        SetWindowLongPtrW( hwnd, DWLP_MSGRESULT, result );
    }
    else
        ret = call_dialog_proc( hwnd, msg, wParam, lParam, &result, proc->procA );
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
    else if (proc == WINPROC_PROC16)
    {
        ret = WINPROC_CallProcWtoA( wow_handlers.call_dialog_proc, hwnd, msg, wParam, lParam, &result, func );
        SetWindowLongPtrW( hwnd, DWLP_MSGRESULT, result );
    }
    else if (proc->procA)
    {
        ret = WINPROC_CallProcWtoA( call_dialog_proc, hwnd, msg, wParam, lParam, &result, proc->procA );
        SetWindowLongPtrW( hwnd, DWLP_MSGRESULT, result );
    }
    else
        ret = call_dialog_proc( hwnd, msg, wParam, lParam, &result, proc->procW );
    return ret;
}


/***********************************************************************
 * Window procedures for builtin classes
 */

static LRESULT WINAPI ButtonWndProcA( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    return wow_handlers.button_proc( hwnd, msg, wParam, lParam, FALSE );
}

static LRESULT WINAPI ButtonWndProcW( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    return wow_handlers.button_proc( hwnd, msg, wParam, lParam, TRUE );
}

static LRESULT WINAPI ComboWndProcA( HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam )
{
    return wow_handlers.combo_proc( hwnd, message, wParam, lParam, FALSE );
}

static LRESULT WINAPI ComboWndProcW( HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam )
{
    return wow_handlers.combo_proc( hwnd, message, wParam, lParam, TRUE );
}

LRESULT WINAPI EditWndProcA( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    return wow_handlers.edit_proc( hwnd, msg, wParam, lParam, FALSE );
}

static LRESULT WINAPI EditWndProcW( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    return wow_handlers.edit_proc( hwnd, msg, wParam, lParam, TRUE );
}

static LRESULT WINAPI ListBoxWndProcA( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    return wow_handlers.listbox_proc( hwnd, msg, wParam, lParam, FALSE );
}

static LRESULT WINAPI ListBoxWndProcW( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    return wow_handlers.listbox_proc( hwnd, msg, wParam, lParam, TRUE );
}

static LRESULT WINAPI MDIClientWndProcA( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    return wow_handlers.mdiclient_proc( hwnd, msg, wParam, lParam, FALSE );
}

static LRESULT WINAPI MDIClientWndProcW( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    return wow_handlers.mdiclient_proc( hwnd, msg, wParam, lParam, TRUE );
}

static LRESULT WINAPI ScrollBarWndProcA( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    return wow_handlers.scrollbar_proc( hwnd, msg, wParam, lParam, FALSE );
}

static LRESULT WINAPI ScrollBarWndProcW( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    return wow_handlers.scrollbar_proc( hwnd, msg, wParam, lParam, TRUE );
}

static LRESULT WINAPI StaticWndProcA( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    return wow_handlers.static_proc( hwnd, msg, wParam, lParam, FALSE );
}

static LRESULT WINAPI StaticWndProcW( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    return wow_handlers.static_proc( hwnd, msg, wParam, lParam, TRUE );
}

static DWORD wait_message( DWORD count, CONST HANDLE *handles, DWORD timeout, DWORD mask, DWORD flags )
{
    DWORD ret = USER_Driver->pMsgWaitForMultipleObjectsEx( count, handles, timeout, mask, flags );
    if (ret == WAIT_TIMEOUT && !count && !timeout) NtYieldExecution();
    return ret;
}

/**********************************************************************
 *		UserRegisterWowHandlers (USER32.@)
 *
 * NOTE: no attempt has been made to be compatible here,
 * the Windows function is most likely completely different.
 */
void WINAPI UserRegisterWowHandlers( const struct wow_handlers16 *new, struct wow_handlers32 *orig )
{
    orig->button_proc     = ButtonWndProc_common;
    orig->combo_proc      = ComboWndProc_common;
    orig->edit_proc       = EditWndProc_common;
    orig->listbox_proc    = ListBoxWndProc_common;
    orig->mdiclient_proc  = MDIClientWndProc_common;
    orig->scrollbar_proc  = ScrollBarWndProc_common;
    orig->static_proc     = StaticWndProc_common;
    orig->wait_message    = wait_message;
    orig->create_window   = WIN_CreateWindowEx;
    orig->get_win_handle  = WIN_GetFullHandle;
    orig->alloc_winproc   = WINPROC_AllocProc;
    orig->get_dialog_info = DIALOG_get_info;
    orig->dialog_box_loop = DIALOG_DoDialogBox;
    orig->get_icon_param  = get_icon_param;
    orig->set_icon_param  = set_icon_param;

    wow_handlers = *new;
}

struct wow_handlers16 wow_handlers =
{
    ButtonWndProc_common,
    ComboWndProc_common,
    EditWndProc_common,
    ListBoxWndProc_common,
    MDIClientWndProc_common,
    ScrollBarWndProc_common,
    StaticWndProc_common,
    wait_message,
    WIN_CreateWindowEx,
    NULL,  /* call_window_proc */
    NULL,  /* call_dialog_proc */
    NULL,  /* free_icon_param */
};
