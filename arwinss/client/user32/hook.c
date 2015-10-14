/*
 * Windows hook functions
 *
 * Copyright 2002 Alexandre Julliard
 * Copyright 2005 Dmitry Timoshkov
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
 *
 * NOTES:
 *   Status of the various hooks:
 *     WH_MSGFILTER                 OK
 *     WH_JOURNALRECORD             Partially implemented
 *     WH_JOURNALPLAYBACK           Partially implemented
 *     WH_KEYBOARD                  OK
 *     WH_GETMESSAGE	            OK (FIXME: A/W mapping?)
 *     WH_CALLWNDPROC	            OK (FIXME: A/W mapping?)
 *     WH_CBT
 *       HCBT_MOVESIZE              OK
 *       HCBT_MINMAX                OK
 *       HCBT_QS                    OK
 *       HCBT_CREATEWND             OK
 *       HCBT_DESTROYWND            OK
 *       HCBT_ACTIVATE              OK
 *       HCBT_CLICKSKIPPED          OK
 *       HCBT_KEYSKIPPED            OK
 *       HCBT_SYSCOMMAND            OK
 *       HCBT_SETFOCUS              OK
 *     WH_SYSMSGFILTER	            OK
 *     WH_MOUSE	                    OK
 *     WH_HARDWARE	            Not supported in Win32
 *     WH_DEBUG	                    Not implemented
 *     WH_SHELL
 *       HSHELL_WINDOWCREATED       OK
 *       HSHELL_WINDOWDESTROYED     OK
 *       HSHELL_ACTIVATESHELLWINDOW Not implemented
 *       HSHELL_WINDOWACTIVATED     Not implemented
 *       HSHELL_GETMINRECT          Not implemented
 *       HSHELL_REDRAW              Not implemented
 *       HSHELL_TASKMAN             Not implemented
 *       HSHELL_LANGUAGE            Not implemented
 *       HSHELL_SYSMENU             Not implemented
 *       HSHELL_ENDTASK             Not implemented
 *       HSHELL_ACCESSIBILITYSTATE  Not implemented
 *       HSHELL_APPCOMMAND          Not implemented
 *       HSHELL_WINDOWREPLACED      Not implemented
 *       HSHELL_WINDOWREPLACING     Not implemented
 *     WH_FOREGROUNDIDLE            Not implemented
 *     WH_CALLWNDPROCRET            OK (FIXME: A/W mapping?)
 *     WH_KEYBOARD_LL               Implemented but should use SendMessage instead
 *     WH_MOUSE_LL                  Implemented but should use SendMessage instead
 */

#include "config.h"
#include "wine/port.h"

#include <stdarg.h>
#include <assert.h>

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "winerror.h"
#include "win.h"
#include "user_private.h"
#include "wine/rosuser.h"
#include "wine/server.h"
#include "wine/unicode.h"
#include "wine/debug.h"
#include "wine/winternl.h"

WINE_DEFAULT_DEBUG_CHANNEL(hook);
WINE_DECLARE_DEBUG_CHANNEL(relay);

struct hook_info
{
    INT id;
    void *proc;
    void *handle;
    DWORD pid, tid;
    BOOL prev_unicode, next_unicode;
    WCHAR module[MAX_PATH];
};

#define WH_WINEVENT (WH_MAXHOOK+1)

static const char * const hook_names[WH_WINEVENT - WH_MINHOOK + 1] =
{
    "WH_MSGFILTER",
    "WH_JOURNALRECORD",
    "WH_JOURNALPLAYBACK",
    "WH_KEYBOARD",
    "WH_GETMESSAGE",
    "WH_CALLWNDPROC",
    "WH_CBT",
    "WH_SYSMSGFILTER",
    "WH_MOUSE",
    "WH_HARDWARE",
    "WH_DEBUG",
    "WH_SHELL",
    "WH_FOREGROUNDIDLE",
    "WH_CALLWNDPROCRET",
    "WH_KEYBOARD_LL",
    "WH_MOUSE_LL",
    "WH_WINEVENT"
};


/***********************************************************************
 *		get_ll_hook_timeout
 *
 */
static UINT get_ll_hook_timeout(void)
{
    /* FIXME: should retrieve LowLevelHooksTimeout in HKEY_CURRENT_USER\Control Panel\Desktop */
    return 2000;
}


/***********************************************************************
 *		set_windows_hook
 *
 * Implementation of SetWindowsHookExA and SetWindowsHookExW.
 */
static HHOOK set_windows_hook( INT id, HOOKPROC proc, HINSTANCE inst, DWORD tid, BOOL unicode )
{
    HHOOK handle = 0;
    WCHAR module[MAX_PATH];
    DWORD len;

    if (!proc)
    {
        SetLastError( ERROR_INVALID_FILTER_PROC );
        return 0;
    }

    if (tid)  /* thread-local hook */
    {
        if (id == WH_JOURNALRECORD ||
            id == WH_JOURNALPLAYBACK ||
            id == WH_KEYBOARD_LL ||
            id == WH_MOUSE_LL ||
            id == WH_SYSMSGFILTER)
        {
            /* these can only be global */
            SetLastError( ERROR_INVALID_PARAMETER );
            return 0;
        }
    }
    else  /* system-global hook */
    {
        if (id == WH_KEYBOARD_LL || id == WH_MOUSE_LL) inst = 0;
        else if (!inst)
        {
            SetLastError( ERROR_HOOK_NEEDS_HMOD );
            return 0;
        }
    }

    if (inst && (!(len = GetModuleFileNameW( inst, module, MAX_PATH )) || len >= MAX_PATH))
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }

    SERVER_START_REQ( set_hook )
    {
        req->id        = id;
        req->pid       = 0;
        req->tid       = tid;
        req->event_min = EVENT_MIN;
        req->event_max = EVENT_MAX;
        req->flags     = WINEVENT_INCONTEXT;
        req->unicode   = unicode;
        if (inst) /* make proc relative to the module base */
        {
            req->proc = wine_server_client_ptr( (void *)((char *)proc - (char *)inst) );
            wine_server_add_data( req, module, strlenW(module) * sizeof(WCHAR) );
        }
        else req->proc = wine_server_client_ptr( proc );

        if (!wine_server_call_err( req ))
        {
            handle = wine_server_ptr_handle( reply->handle );
            get_user_thread_info()->active_hooks = reply->active_hooks;
        }
    }
    SERVER_END_REQ;

    TRACE( "%s %p %x -> %p\n", hook_names[id-WH_MINHOOK], proc, tid, handle );
    return handle;
}


/***********************************************************************
 *		call_hook_AtoW
 */
static LRESULT call_hook_AtoW( HOOKPROC proc, INT id, INT code, WPARAM wparam, LPARAM lparam )
{
    LRESULT ret;
    UNICODE_STRING usBuffer;
    if (id != WH_CBT || code != HCBT_CREATEWND) ret = proc( code, wparam, lparam );
    else
    {
        CBT_CREATEWNDA *cbtcwA = (CBT_CREATEWNDA *)lparam;
        CBT_CREATEWNDW cbtcwW;
        CREATESTRUCTW csW;
        LPWSTR nameW = NULL;
        LPWSTR classW = NULL;

        cbtcwW.lpcs = &csW;
        cbtcwW.hwndInsertAfter = cbtcwA->hwndInsertAfter;
        csW = *(CREATESTRUCTW *)cbtcwA->lpcs;

        if (!IS_INTRESOURCE(cbtcwA->lpcs->lpszName))
        {
            RtlCreateUnicodeStringFromAsciiz(&usBuffer,cbtcwA->lpcs->lpszName);
            csW.lpszName = nameW = usBuffer.Buffer;
        }
        if (!IS_INTRESOURCE(cbtcwA->lpcs->lpszClass))
        {
            RtlCreateUnicodeStringFromAsciiz(&usBuffer,cbtcwA->lpcs->lpszClass);
            csW.lpszClass = classW = usBuffer.Buffer;
        }
        ret = proc( code, wparam, (LPARAM)&cbtcwW );
        cbtcwA->hwndInsertAfter = cbtcwW.hwndInsertAfter;
        HeapFree( GetProcessHeap(), 0, nameW );
        HeapFree( GetProcessHeap(), 0, classW );
    }
    return ret;
}


/***********************************************************************
 *		call_hook_WtoA
 */
static LRESULT call_hook_WtoA( HOOKPROC proc, INT id, INT code, WPARAM wparam, LPARAM lparam )
{
    LRESULT ret;

    if (id != WH_CBT || code != HCBT_CREATEWND) ret = proc( code, wparam, lparam );
    else
    {
        CBT_CREATEWNDW *cbtcwW = (CBT_CREATEWNDW *)lparam;
        CBT_CREATEWNDA cbtcwA;
        CREATESTRUCTA csA;
        int len;
        LPSTR nameA = NULL;
        LPSTR classA = NULL;

        cbtcwA.lpcs = &csA;
        cbtcwA.hwndInsertAfter = cbtcwW->hwndInsertAfter;
        csA = *(CREATESTRUCTA *)cbtcwW->lpcs;

        if (!IS_INTRESOURCE(cbtcwW->lpcs->lpszName)) {
            len = WideCharToMultiByte( CP_ACP, 0, cbtcwW->lpcs->lpszName, -1, NULL, 0, NULL, NULL );
            nameA = HeapAlloc( GetProcessHeap(), 0, len*sizeof(CHAR) );
            WideCharToMultiByte( CP_ACP, 0, cbtcwW->lpcs->lpszName, -1, nameA, len, NULL, NULL );
            csA.lpszName = nameA;
        }

        if (!IS_INTRESOURCE(cbtcwW->lpcs->lpszClass)) {
            len = WideCharToMultiByte( CP_ACP, 0, cbtcwW->lpcs->lpszClass, -1, NULL, 0, NULL, NULL );
            classA = HeapAlloc( GetProcessHeap(), 0, len*sizeof(CHAR) );
            WideCharToMultiByte( CP_ACP, 0, cbtcwW->lpcs->lpszClass, -1, classA, len, NULL, NULL );
            csA.lpszClass = classA;
        }

        ret = proc( code, wparam, (LPARAM)&cbtcwA );
        cbtcwW->hwndInsertAfter = cbtcwA.hwndInsertAfter;
        HeapFree( GetProcessHeap(), 0, nameA );
        HeapFree( GetProcessHeap(), 0, classA );
    }
    return ret;
}


/***********************************************************************
 *		call_hook_proc
 */
static LRESULT call_hook_proc( HOOKPROC proc, INT id, INT code, WPARAM wparam, LPARAM lparam,
                               BOOL prev_unicode, BOOL next_unicode )
{
    LRESULT ret;

    if (TRACE_ON(relay))
        DPRINTF( "%04x:Call hook proc %p (id=%s,code=%x,wp=%08lx,lp=%08lx)\n",
                 GetCurrentThreadId(), proc, hook_names[id-WH_MINHOOK], code, wparam, lparam );

    if (!prev_unicode == !next_unicode) ret = proc( code, wparam, lparam );
    else if (prev_unicode) ret = call_hook_WtoA( proc, id, code, wparam, lparam );
    else ret = call_hook_AtoW( proc, id, code, wparam, lparam );

    if (TRACE_ON(relay))
        DPRINTF( "%04x:Ret  hook proc %p (id=%s,code=%x,wp=%08lx,lp=%08lx) retval=%08lx\n",
                 GetCurrentThreadId(), proc, hook_names[id-WH_MINHOOK], code, wparam, lparam, ret );

    return ret;
}


/***********************************************************************
 *		get_hook_proc
 *
 * Retrieve the hook procedure real value for a module-relative proc
 */
void *get_hook_proc( void *proc, const WCHAR *module )
{
    HMODULE mod;

    if (!(mod = GetModuleHandleW(module)))
    {
        TRACE( "loading %s\n", debugstr_w(module) );
        /* FIXME: the library will never be freed */
        if (!(mod = LoadLibraryExW(module, NULL, LOAD_WITH_ALTERED_SEARCH_PATH))) return NULL;
    }
    return (char *)mod + (ULONG_PTR)proc;
}

/***********************************************************************
 *		call_hook
 *
 * Call hook either in current thread or send message to the destination
 * thread.
 */
static LRESULT call_hook( struct hook_info *info, INT code, WPARAM wparam, LPARAM lparam )
{
    DWORD_PTR ret = 0;

    if (info->tid)
    {
        struct hook_extra_info h_extra;
        h_extra.handle = info->handle;
        h_extra.lparam = lparam;

        TRACE( "calling hook in thread %04x %s code %x wp %lx lp %lx\n",
               info->tid, hook_names[info->id-WH_MINHOOK], code, wparam, lparam );

        switch(info->id)
        {
        case WH_KEYBOARD_LL:
            MSG_SendInternalMessageTimeout( info->pid, info->tid, WM_WINE_KEYBOARD_LL_HOOK,
                                            wparam, (LPARAM)&h_extra, SMTO_ABORTIFHUNG,
                                            get_ll_hook_timeout(), &ret );
            break;
        case WH_MOUSE_LL:
            MSG_SendInternalMessageTimeout( info->pid, info->tid, WM_WINE_MOUSE_LL_HOOK,
                                            wparam, (LPARAM)&h_extra, SMTO_ABORTIFHUNG,
                                            get_ll_hook_timeout(), &ret );
            break;
        default:
            ERR("Unknown hook id %d\n", info->id);
            assert(0);
            break;
        }
    }
    else if (info->proc)
    {
        TRACE( "calling hook %p %s code %x wp %lx lp %lx module %s\n",
               info->proc, hook_names[info->id-WH_MINHOOK], code, wparam,
               lparam, debugstr_w(info->module) );

        if (!info->module[0] ||
            (info->proc = get_hook_proc( info->proc, info->module )) != NULL)
        {
            struct user_thread_info *thread_info = get_user_thread_info();
            HHOOK prev = thread_info->hook;
            BOOL prev_unicode = thread_info->hook_unicode;

            thread_info->hook = info->handle;
            thread_info->hook_unicode = info->next_unicode;
            ret = call_hook_proc( info->proc, info->id, code, wparam, lparam,
                                  info->prev_unicode, info->next_unicode );
            thread_info->hook = prev;
            thread_info->hook_unicode = prev_unicode;
        }
    }
    return ret;
}


/***********************************************************************
 *           HOOK_IsHooked
 */
static BOOL HOOK_IsHooked( INT id )
{
    struct user_thread_info *thread_info = get_user_thread_info();

    if (!thread_info->active_hooks) return TRUE;
    return (thread_info->active_hooks & (1 << (id - WH_MINHOOK))) != 0;
}


/***********************************************************************
 *		HOOK_CallHooks
 */
LRESULT HOOK_CallHooks( INT id, INT code, WPARAM wparam, LPARAM lparam, BOOL unicode )
{
    struct user_thread_info *thread_info = get_user_thread_info();
    struct hook_info info;
    DWORD_PTR ret = 0;

    USER_CheckNotLock();

    if (id == WH_SHELL)
    {
        HWND hook_windows[4]; // FIXME: Not a good way, but dynamic allocation all the time is stupid too
        UINT cbListSize = sizeof(hook_windows);
        if (!RosUserBuildShellHookHwndList(hook_windows, &cbListSize))
        {
            if (cbListSize > sizeof(hook_windows)) ERR("Not enough hook windows array size!\n");
            /* otherwise the list is just empty */
        }
        else
        {
            INT wm_shellhook = RegisterWindowMessageW(L"SHELLHOOK");
            INT wnd_num;

            for (wnd_num = 0; wnd_num < cbListSize / sizeof(HWND); wnd_num++)
                PostMessage(hook_windows[wnd_num], wm_shellhook, code, wparam);
        }
    }

    if (!HOOK_IsHooked( id ))
    {
        TRACE( "skipping hook %s mask %x\n", hook_names[id-WH_MINHOOK], thread_info->active_hooks );
        return 0;
    }

    ZeroMemory( &info, sizeof(info) - sizeof(info.module) );
    info.prev_unicode = unicode;
    info.id = id;

    SERVER_START_REQ( start_hook_chain )
    {
        req->id = info.id;
        req->event = EVENT_MIN;
        wine_server_set_reply( req, info.module, sizeof(info.module)-sizeof(WCHAR) );
        if (!wine_server_call( req ))
        {
            info.module[wine_server_reply_size(req) / sizeof(WCHAR)] = 0;
            info.handle       = wine_server_ptr_handle( reply->handle );
            info.pid          = reply->pid;
            info.tid          = reply->tid;
            info.proc         = wine_server_get_ptr( reply->proc );
            info.next_unicode = reply->unicode;
            thread_info->active_hooks = reply->active_hooks;
        }
    }
    SERVER_END_REQ;

    if (!info.tid && !info.proc) return 0;
    ret = call_hook( &info, code, wparam, lparam );

    SERVER_START_REQ( finish_hook_chain )
    {
        req->id = id;
        wine_server_call( req );
    }
    SERVER_END_REQ;
    return ret;
}


/***********************************************************************
 *		SetWindowsHookA (USER32.@)
 */
HHOOK WINAPI SetWindowsHookA( INT id, HOOKPROC proc )
{
    return SetWindowsHookExA( id, proc, 0, GetCurrentThreadId() );
}


/***********************************************************************
 *		SetWindowsHookW (USER32.@)
 */
HHOOK WINAPI SetWindowsHookW( INT id, HOOKPROC proc )
{
    return SetWindowsHookExW( id, proc, 0, GetCurrentThreadId() );
}


/***********************************************************************
 *		SetWindowsHookExA (USER32.@)
 */
HHOOK WINAPI SetWindowsHookExA( INT id, HOOKPROC proc, HINSTANCE inst, DWORD tid )
{
    return set_windows_hook( id, proc, inst, tid, FALSE );
}

/***********************************************************************
 *		SetWindowsHookExW (USER32.@)
 */
HHOOK WINAPI SetWindowsHookExW( INT id, HOOKPROC proc, HINSTANCE inst, DWORD tid )
{
    return set_windows_hook( id, proc, inst, tid, TRUE );
}


/***********************************************************************
 *		UnhookWindowsHook (USER32.@)
 */
BOOL WINAPI UnhookWindowsHook( INT id, HOOKPROC proc )
{
    BOOL ret;

    TRACE( "%s %p\n", hook_names[id-WH_MINHOOK], proc );

    SERVER_START_REQ( remove_hook )
    {
        req->handle = 0;
        req->id   = id;
        req->proc = wine_server_client_ptr( proc );
        ret = !wine_server_call_err( req );
        if (ret) get_user_thread_info()->active_hooks = reply->active_hooks;
    }
    SERVER_END_REQ;
    if (!ret && GetLastError() == ERROR_INVALID_HANDLE) SetLastError( ERROR_INVALID_HOOK_HANDLE );
    return ret;
}



/***********************************************************************
 *		UnhookWindowsHookEx (USER32.@)
 */
BOOL WINAPI UnhookWindowsHookEx( HHOOK hhook )
{
    BOOL ret;

    SERVER_START_REQ( remove_hook )
    {
        req->handle = wine_server_user_handle( hhook );
        req->id     = 0;
        ret = !wine_server_call_err( req );
        if (ret) get_user_thread_info()->active_hooks = reply->active_hooks;
    }
    SERVER_END_REQ;
    if (!ret && GetLastError() == ERROR_INVALID_HANDLE) SetLastError( ERROR_INVALID_HOOK_HANDLE );
    return ret;
}


/***********************************************************************
 *		CallNextHookEx (USER32.@)
 */
LRESULT WINAPI CallNextHookEx( HHOOK hhook, INT code, WPARAM wparam, LPARAM lparam )
{
    struct user_thread_info *thread_info = get_user_thread_info();
    struct hook_info info;

    ZeroMemory( &info, sizeof(info) - sizeof(info.module) );

    SERVER_START_REQ( get_hook_info )
    {
        req->handle = wine_server_user_handle( thread_info->hook );
        req->get_next = 1;
        req->event = EVENT_MIN;
        wine_server_set_reply( req, info.module, sizeof(info.module)-sizeof(WCHAR) );
        if (!wine_server_call_err( req ))
        {
            info.module[wine_server_reply_size(req) / sizeof(WCHAR)] = 0;
            info.handle       = wine_server_ptr_handle( reply->handle );
            info.id           = reply->id;
            info.pid          = reply->pid;
            info.tid          = reply->tid;
            info.proc         = wine_server_get_ptr( reply->proc );
            info.next_unicode = reply->unicode;
        }
    }
    SERVER_END_REQ;

    info.prev_unicode = thread_info->hook_unicode;
    return call_hook( &info, code, wparam, lparam );
}


LRESULT call_current_hook( HHOOK hhook, INT code, WPARAM wparam, LPARAM lparam )
{
    struct hook_info info;

    ZeroMemory( &info, sizeof(info) - sizeof(info.module) );

    SERVER_START_REQ( get_hook_info )
    {
        req->handle = wine_server_user_handle( hhook );
        req->get_next = 0;
        req->event = EVENT_MIN;
        wine_server_set_reply( req, info.module, sizeof(info.module)-sizeof(WCHAR) );
        if (!wine_server_call_err( req ))
        {
            info.module[wine_server_reply_size(req) / sizeof(WCHAR)] = 0;
            info.handle       = wine_server_ptr_handle( reply->handle );
            info.id           = reply->id;
            info.pid          = reply->pid;
            info.tid          = reply->tid;
            info.proc         = wine_server_get_ptr( reply->proc );
            info.next_unicode = reply->unicode;
        }
    }
    SERVER_END_REQ;

    info.prev_unicode = TRUE;  /* assume Unicode for this function */
    return call_hook( &info, code, wparam, lparam );
}

/***********************************************************************
 *		CallMsgFilterA (USER32.@)
 */
BOOL WINAPI CallMsgFilterA( LPMSG msg, INT code )
{
    if (HOOK_CallHooks( WH_SYSMSGFILTER, code, 0, (LPARAM)msg, FALSE )) return TRUE;
    return HOOK_CallHooks( WH_MSGFILTER, code, 0, (LPARAM)msg, FALSE );
}


/***********************************************************************
 *		CallMsgFilterW (USER32.@)
 */
BOOL WINAPI CallMsgFilterW( LPMSG msg, INT code )
{
    if (HOOK_CallHooks( WH_SYSMSGFILTER, code, 0, (LPARAM)msg, TRUE )) return TRUE;
    return HOOK_CallHooks( WH_MSGFILTER, code, 0, (LPARAM)msg, TRUE );
}


/***********************************************************************
 *           SetWinEventHook                            [USER32.@]
 *
 * Set up an event hook for a set of events.
 *
 * PARAMS
 *  event_min [I] Lowest event handled by pfnProc
 *  event_max [I] Highest event handled by pfnProc
 *  inst      [I] DLL containing pfnProc
 *  proc      [I] Callback event hook function
 *  pid       [I] Process to get events from, or 0 for all processes
 *  tid       [I] Thread to get events from, or 0 for all threads
 *  flags     [I] Flags indicating the status of pfnProc
 *
 * RETURNS
 *  Success: A handle representing the hook.
 *  Failure: A NULL handle.
 */
HWINEVENTHOOK WINAPI SetWinEventHook(DWORD event_min, DWORD event_max,
                                     HMODULE inst, WINEVENTPROC proc,
                                     DWORD pid, DWORD tid, DWORD flags)
{
    HWINEVENTHOOK handle = 0;
    WCHAR module[MAX_PATH];
    DWORD len;

    TRACE("%d,%d,%p,%p,%08x,%04x,%08x\n", event_min, event_max, inst,
          proc, pid, tid, flags);

    if (inst)
    {
        if (!(len = GetModuleFileNameW(inst, module, MAX_PATH)) || len >= MAX_PATH)
            inst = 0;
    }

    if ((flags & WINEVENT_INCONTEXT) && !inst)
    {
        SetLastError(ERROR_HOOK_NEEDS_HMOD);
        return 0;
    }

    if (event_min > event_max)
    {
        SetLastError(ERROR_INVALID_HOOK_FILTER);
        return 0;
    }

    /* FIXME: what if the tid or pid belongs to another process? */
    if (tid)  /* thread-local hook */
        inst = 0;

    SERVER_START_REQ( set_hook )
    {
        req->id        = WH_WINEVENT;
        req->pid       = pid;
        req->tid       = tid;
        req->event_min = event_min;
        req->event_max = event_max;
        req->flags     = flags;
        req->unicode   = 1;
        if (inst) /* make proc relative to the module base */
        {
            req->proc = wine_server_client_ptr( (void *)((char *)proc - (char *)inst) );
            wine_server_add_data( req, module, strlenW(module) * sizeof(WCHAR) );
        }
        else req->proc = wine_server_client_ptr( proc );

        if (!wine_server_call_err( req ))
        {
            handle = wine_server_ptr_handle( reply->handle );
            get_user_thread_info()->active_hooks = reply->active_hooks;
        }
    }
    SERVER_END_REQ;

    TRACE("-> %p\n", handle);
    return handle;
}


/***********************************************************************
 *           UnhookWinEvent                             [USER32.@]
 *
 * Remove an event hook for a set of events.
 *
 * PARAMS
 *  hEventHook [I] Event hook to remove
 *
 * RETURNS
 *  Success: TRUE. The event hook has been removed.
 *  Failure: FALSE, if hEventHook is invalid.
 */
BOOL WINAPI UnhookWinEvent(HWINEVENTHOOK hEventHook)
{
    BOOL ret;

    SERVER_START_REQ( remove_hook )
    {
        req->handle = wine_server_user_handle( hEventHook );
        req->id     = WH_WINEVENT;
        ret = !wine_server_call_err( req );
        if (ret) get_user_thread_info()->active_hooks = reply->active_hooks;
    }
    SERVER_END_REQ;
    return ret;
}

static inline BOOL find_first_hook(DWORD id, DWORD event, HWND hwnd, LONG object_id,
                                   LONG child_id, struct hook_info *info)
{
    struct user_thread_info *thread_info = get_user_thread_info();
    BOOL ret;

    if (!HOOK_IsHooked( id ))
    {
        TRACE( "skipping hook %s mask %x\n", hook_names[id-WH_MINHOOK], thread_info->active_hooks );
        return FALSE;
    }

    SERVER_START_REQ( start_hook_chain )
    {
        req->id = id;
        req->event = event;
        req->window = wine_server_user_handle( hwnd );
        req->object_id = object_id;
        req->child_id = child_id;
        wine_server_set_reply( req, info->module, sizeof(info->module)-sizeof(WCHAR) );
        ret = !wine_server_call( req );
        if (ret)
        {
            info->module[wine_server_reply_size(req) / sizeof(WCHAR)] = 0;
            info->handle    = wine_server_ptr_handle( reply->handle );
            info->proc      = wine_server_get_ptr( reply->proc );
            info->tid       = reply->tid;
            thread_info->active_hooks = reply->active_hooks;
        }
    }
    SERVER_END_REQ;
    return ret && (info->tid || info->proc);
}

static inline BOOL find_next_hook(DWORD event, HWND hwnd, LONG object_id,
                                  LONG child_id, struct hook_info *info)
{
    BOOL ret;

    SERVER_START_REQ( get_hook_info )
    {
        req->handle = wine_server_user_handle( info->handle );
        req->get_next = 1;
        req->event = event;
        req->window = wine_server_user_handle( hwnd );
        req->object_id = object_id;
        req->child_id = child_id;
        wine_server_set_reply( req, info->module, sizeof(info->module)-sizeof(WCHAR) );
        ret = !wine_server_call( req );
        if (ret)
        {
            info->module[wine_server_reply_size(req) / sizeof(WCHAR)] = 0;
            info->handle    = wine_server_ptr_handle( reply->handle );
            info->proc      = wine_server_get_ptr( reply->proc );
            info->tid       = reply->tid;
        }
    }
    SERVER_END_REQ;
    return ret;
}

static inline void find_hook_close(DWORD id)
{
    SERVER_START_REQ( finish_hook_chain )
    {
        req->id = id;
        wine_server_call( req );
    }
    SERVER_END_REQ;
}

/***********************************************************************
 *           NotifyWinEvent                             [USER32.@]
 *
 * Inform the OS that an event has occurred.
 *
 * PARAMS
 *  event     [I] Id of the event
 *  hwnd      [I] Window holding the object that created the event
 *  object_id [I] Type of object that created the event
 *  child_id  [I] Child object of nId, or CHILDID_SELF.
 *
 * RETURNS
 *  Nothing.
 */
void WINAPI NotifyWinEvent(DWORD event, HWND hwnd, LONG object_id, LONG child_id)
{
    struct hook_info info;

    TRACE("%04x,%p,%d,%d\n", event, hwnd, object_id, child_id);

    if (!hwnd)
    {
        SetLastError(ERROR_INVALID_WINDOW_HANDLE);
        return;
    }

    USER_CheckNotLock();

#if 0
    if (event & 0x80000000)
    {
        /* FIXME: on 64-bit platforms we need to invent some other way for
         * passing parameters, nId and nChildId can't hold full [W|L]PARAM.
         * struct call_hook *hook = (LRESULT *)hWnd;
         * wparam = hook->wparam;
         * lparam = hook->lparam;
         */
        LRESULT *ret = (LRESULT *)hwnd;
        INT id, code, unicode;

        id = (dwEvent & 0x7fff0000) >> 16;
        code = event & 0x7fff;
        unicode = event & 0x8000;
        *ret = HOOK_CallHooks(id, code, object_id, child_id, unicode);
        return;
    }
#endif

    if (!find_first_hook(WH_WINEVENT, event, hwnd, object_id, child_id, &info)) return;

    do
    {
        WINEVENTPROC proc = info.proc;
        if (proc)
        {
            TRACE( "calling WH_WINEVENT hook %p event %x hwnd %p %x %x module %s\n",
                   proc, event, hwnd, object_id, child_id, debugstr_w(info.module) );

            if (!info.module[0] || (proc = get_hook_proc( proc, info.module )) != NULL)
            {
                if (TRACE_ON(relay))
                    DPRINTF( "%04x:Call winevent hook proc %p (hhook=%p,event=%x,hwnd=%p,object_id=%x,child_id=%x,tid=%04x,time=%x)\n",
                             GetCurrentThreadId(), proc, info.handle, event, hwnd, object_id,
                             child_id, GetCurrentThreadId(), GetCurrentTime());

                proc( info.handle, event, hwnd, object_id, child_id,
                      GetCurrentThreadId(), GetCurrentTime());

                if (TRACE_ON(relay))
                    DPRINTF( "%04x:Ret  winevent hook proc %p (hhook=%p,event=%x,hwnd=%p,object_id=%x,child_id=%x,tid=%04x,time=%x)\n",
                             GetCurrentThreadId(), proc, info.handle, event, hwnd, object_id,
                             child_id, GetCurrentThreadId(), GetCurrentTime());
            }
        }
        else
            break;
    }
    while (find_next_hook(event, hwnd, object_id, child_id, &info));

    find_hook_close(WH_WINEVENT);
}


/***********************************************************************
 *           IsWinEventHookInstalled                       [USER32.@]
 *
 * Determine if an event hook is installed for an event.
 *
 * PARAMS
 *  dwEvent  [I] Id of the event
 *
 * RETURNS
 *  TRUE,  If there are any hooks installed for the event.
 *  FALSE, Otherwise.
 *
 * BUGS
 *  Not implemented.
 */
BOOL WINAPI IsWinEventHookInstalled(DWORD dwEvent)
{
    /* FIXME: Needed by Office 2007 installer */
    WARN("(%d)-stub!\n", dwEvent);
    return TRUE;
}

/***********************************************************************
 *           RegisterShellHookWindow			[USER32.@]
 */
BOOL WINAPI RegisterShellHookWindow ( HWND hWnd )
{
    return RosUserRegisterShellHookWindow( hWnd );
}


/***********************************************************************
 *           DeregisterShellHookWindow			[USER32.@]
 */
HRESULT WINAPI DeregisterShellHookWindow ( HWND hWnd )
{
    return RosUserDeRegisterShellHookWindow( hWnd );
}
