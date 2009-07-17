/*
 * Windows 16-bit hook functions
 *
 * Copyright 1994, 1995, 2002 Alexandre Julliard
 * Copyright 1996 Andrew Lewycky
 *
 * Based on investigations by Alex Korobka
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

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "wownt32.h"
#include "wine/winuser16.h"
#include "win.h"
#include "user_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(hook);


static LRESULT CALLBACK call_WH_MSGFILTER( INT code, WPARAM wp, LPARAM lp );
static LRESULT CALLBACK call_WH_KEYBOARD( INT code, WPARAM wp, LPARAM lp );
static LRESULT CALLBACK call_WH_GETMESSAGE( INT code, WPARAM wp, LPARAM lp );
static LRESULT CALLBACK call_WH_CALLWNDPROC( INT code, WPARAM wp, LPARAM lp );
static LRESULT CALLBACK call_WH_CBT( INT code, WPARAM wp, LPARAM lp );
static LRESULT CALLBACK call_WH_MOUSE( INT code, WPARAM wp, LPARAM lp );
static LRESULT CALLBACK call_WH_SHELL( INT code, WPARAM wp, LPARAM lp );

#define WH_MAXHOOK16 WH_SHELL  /* Win16 only supports up to WH_SHELL */
#define NB_HOOKS16 (WH_MAXHOOK16 - WH_MINHOOK + 1)

static const HOOKPROC hook_procs[NB_HOOKS16] =
{
    call_WH_MSGFILTER,   /* WH_MSGFILTER	*/
    NULL,                /* WH_JOURNALRECORD */
    NULL,                /* WH_JOURNALPLAYBACK */
    call_WH_KEYBOARD,    /* WH_KEYBOARD */
    call_WH_GETMESSAGE,  /* WH_GETMESSAGE */
    call_WH_CALLWNDPROC, /* WH_CALLWNDPROC */
    call_WH_CBT,         /* WH_CBT */
    NULL,                /* WH_SYSMSGFILTER */
    call_WH_MOUSE,       /* WH_MOUSE */
    NULL,                /* WH_HARDWARE */
    NULL,                /* WH_DEBUG */
    call_WH_SHELL        /* WH_SHELL */
};


/* this structure is stored in the thread queue */
struct hook16_queue_info
{
    INT        id;                /* id of current hook */
    HHOOK      hook[NB_HOOKS16];  /* Win32 hook handles */
    HOOKPROC16 proc[NB_HOOKS16];  /* 16-bit hook procedures */
};



/***********************************************************************
 *           map_msg_16_to_32
 */
static inline void map_msg_16_to_32( const MSG16 *msg16, MSG *msg32 )
{
    msg32->hwnd    = WIN_Handle32(msg16->hwnd);
    msg32->message = msg16->message;
    msg32->wParam  = msg16->wParam;
    msg32->lParam  = msg16->lParam;
    msg32->time    = msg16->time;
    msg32->pt.x    = msg16->pt.x;
    msg32->pt.y    = msg16->pt.y;
}


/***********************************************************************
 *           map_msg_32_to_16
 */
static inline void map_msg_32_to_16( const MSG *msg32, MSG16 *msg16 )
{
    msg16->hwnd    = HWND_16(msg32->hwnd);
    msg16->message = msg32->message;
    msg16->wParam  = msg32->wParam;
    msg16->lParam  = msg32->lParam;
    msg16->time    = msg32->time;
    msg16->pt.x    = msg32->pt.x;
    msg16->pt.y    = msg32->pt.y;
}


/***********************************************************************
 *           call_hook_16
 */
static LRESULT call_hook_16( INT id, INT code, WPARAM wp, LPARAM lp )
{
    struct hook16_queue_info *info = get_user_thread_info()->hook16_info;
    WORD args[4];
    DWORD ret;
    INT prev_id = info->id;
    info->id = id;

    args[3] = code;
    args[2] = wp;
    args[1] = HIWORD(lp);
    args[0] = LOWORD(lp);
    WOWCallback16Ex( (DWORD)info->proc[id - WH_MINHOOK], WCB16_PASCAL, sizeof(args), args, &ret );

    info->id = prev_id;

    /* Grrr. While the hook procedure is supposed to have an LRESULT return
       value even in Win16, it seems that for those hook types where the
       return value is interpreted as BOOL, Windows doesn't actually check
       the HIWORD ...  Some buggy Win16 programs, notably WINFILE, rely on
       that, because they neglect to clear DX ... */
    if (id != WH_JOURNALPLAYBACK) ret = LOWORD( ret );
    return ret;
}


struct wndproc_hook_params
{
    HHOOK  hhook;
    INT    code;
    WPARAM wparam;
};

/* callback for WINPROC_Call16To32A */
static LRESULT wndproc_hook_callback( HWND hwnd, UINT msg, WPARAM wp, LPARAM lp,
                                      LRESULT *result, void *arg )
{
    struct wndproc_hook_params *params = arg;
    CWPSTRUCT cwp;

    cwp.hwnd    = hwnd;
    cwp.message = msg;
    cwp.wParam  = wp;
    cwp.lParam  = lp;
    *result = 0;
    return CallNextHookEx( params->hhook, params->code, params->wparam, (LPARAM)&cwp );
}

/* callback for WINPROC_Call32ATo16 */
static LRESULT wndproc_hook_callback16( HWND16 hwnd, UINT16 msg, WPARAM16 wp, LPARAM lp,
                                        LRESULT *result, void *arg )
{
    struct wndproc_hook_params *params = arg;
    CWPSTRUCT16 cwp;
    LRESULT ret;

    cwp.hwnd    = hwnd;
    cwp.message = msg;
    cwp.wParam  = wp;
    cwp.lParam  = lp;

    lp = MapLS( &cwp );
    ret = call_hook_16( WH_CALLWNDPROC, params->code, params->wparam, lp );
    UnMapLS( lp );

    *result = 0;
    return ret;
}


/***********************************************************************
 *		call_WH_MSGFILTER
 */
static LRESULT CALLBACK call_WH_MSGFILTER( INT code, WPARAM wp, LPARAM lp )
{
    MSG *msg32 = (MSG *)lp;
    MSG16 msg16;
    LRESULT ret;

    map_msg_32_to_16( msg32, &msg16 );
    lp = MapLS( &msg16 );
    ret = call_hook_16( WH_MSGFILTER, code, wp, lp );
    UnMapLS( lp );
    return ret;
}


/***********************************************************************
 *		call_WH_KEYBOARD
 */
static LRESULT CALLBACK call_WH_KEYBOARD( INT code, WPARAM wp, LPARAM lp )
{
    return call_hook_16( WH_KEYBOARD, code, wp, lp );
}


/***********************************************************************
 *		call_WH_GETMESSAGE
 */
static LRESULT CALLBACK call_WH_GETMESSAGE( INT code, WPARAM wp, LPARAM lp )
{
    MSG *msg32 = (MSG *)lp;
    MSG16 msg16;
    LRESULT ret;

    map_msg_32_to_16( msg32, &msg16 );

    lp = MapLS( &msg16 );
    ret = call_hook_16( WH_GETMESSAGE, code, wp, lp );
    UnMapLS( lp );

    map_msg_16_to_32( &msg16, msg32 );
    return ret;
}


/***********************************************************************
 *		call_WH_CALLWNDPROC
 */
static LRESULT CALLBACK call_WH_CALLWNDPROC( INT code, WPARAM wp, LPARAM lp )
{
    struct wndproc_hook_params params;
    CWPSTRUCT *cwp32 = (CWPSTRUCT *)lp;
    LRESULT result;

    params.code   = code;
    params.wparam = wp;
    return WINPROC_CallProc32ATo16( wndproc_hook_callback16, cwp32->hwnd, cwp32->message,
                                    cwp32->wParam, cwp32->lParam, &result, &params );
}


/***********************************************************************
 *		call_WH_CBT
 */
static LRESULT CALLBACK call_WH_CBT( INT code, WPARAM wp, LPARAM lp )
{
    LRESULT ret = 0;

    switch (code)
    {
    case HCBT_CREATEWND:
        {
            CBT_CREATEWNDA *cbtcw32 = (CBT_CREATEWNDA *)lp;
            CBT_CREATEWND16 cbtcw16;
            CREATESTRUCT16 cs16;

            cs16.lpCreateParams = (SEGPTR)cbtcw32->lpcs->lpCreateParams;
            cs16.hInstance      = HINSTANCE_16(cbtcw32->lpcs->hInstance);
            cs16.hMenu          = HMENU_16(cbtcw32->lpcs->hMenu);
            cs16.hwndParent     = HWND_16(cbtcw32->lpcs->hwndParent);
            cs16.cy             = cbtcw32->lpcs->cy;
            cs16.cx             = cbtcw32->lpcs->cx;
            cs16.y              = cbtcw32->lpcs->y;
            cs16.x              = cbtcw32->lpcs->x;
            cs16.style          = cbtcw32->lpcs->style;
            cs16.lpszName       = MapLS( cbtcw32->lpcs->lpszName );
            cs16.lpszClass      = MapLS( cbtcw32->lpcs->lpszClass );
            cs16.dwExStyle      = cbtcw32->lpcs->dwExStyle;

            cbtcw16.lpcs = (CREATESTRUCT16 *)MapLS( &cs16 );
            cbtcw16.hwndInsertAfter = HWND_16( cbtcw32->hwndInsertAfter );

            lp = MapLS( &cbtcw16 );
            ret = call_hook_16( WH_CBT, code, wp, lp );
            UnMapLS( cs16.lpszName );
            UnMapLS( cs16.lpszClass );

            cbtcw32->hwndInsertAfter = WIN_Handle32( cbtcw16.hwndInsertAfter );
            UnMapLS( (SEGPTR)cbtcw16.lpcs );
            UnMapLS( lp );
            break;
        }

    case HCBT_ACTIVATE:
        {
            CBTACTIVATESTRUCT *cas32 = (CBTACTIVATESTRUCT *)lp;
            CBTACTIVATESTRUCT16 cas16;

            cas16.fMouse     = cas32->fMouse;
            cas16.hWndActive = HWND_16( cas32->hWndActive );

            lp = MapLS( &cas16 );
            ret = call_hook_16( WH_CBT, code, wp, lp );
            UnMapLS( lp );
            break;
        }
    case HCBT_CLICKSKIPPED:
        {
            MOUSEHOOKSTRUCT *ms32 = (MOUSEHOOKSTRUCT *)lp;
            MOUSEHOOKSTRUCT16 ms16;

            ms16.pt.x         = ms32->pt.x;
            ms16.pt.y         = ms32->pt.y;
            ms16.hwnd         = HWND_16( ms32->hwnd );
            ms16.wHitTestCode = ms32->wHitTestCode;
            ms16.dwExtraInfo  = ms32->dwExtraInfo;

            lp = MapLS( &ms16 );
            ret = call_hook_16( WH_CBT, code, wp, lp );
            UnMapLS( lp );
            break;
        }
    case HCBT_MOVESIZE:
        {
            RECT *rect32 = (RECT *)lp;
            RECT16 rect16;

            rect16.left   = rect32->left;
            rect16.top    = rect32->top;
            rect16.right  = rect32->right;
            rect16.bottom = rect32->bottom;
            lp = MapLS( &rect16 );
            ret = call_hook_16( WH_CBT, code, wp, lp );
            UnMapLS( lp );
            break;
        }
    }
    return ret;
}


/***********************************************************************
 *		call_WH_MOUSE
 */
static LRESULT CALLBACK call_WH_MOUSE( INT code, WPARAM wp, LPARAM lp )
{
    MOUSEHOOKSTRUCT *ms32 = (MOUSEHOOKSTRUCT *)lp;
    MOUSEHOOKSTRUCT16 ms16;
    LRESULT ret;

    ms16.pt.x         = ms32->pt.x;
    ms16.pt.y         = ms32->pt.y;
    ms16.hwnd         = HWND_16( ms32->hwnd );
    ms16.wHitTestCode = ms32->wHitTestCode;
    ms16.dwExtraInfo  = ms32->dwExtraInfo;

    lp = MapLS( &ms16 );
    ret = call_hook_16( WH_MOUSE, code, wp, lp );
    UnMapLS( lp );
    return ret;
}


/***********************************************************************
 *		call_WH_SHELL
 */
static LRESULT CALLBACK call_WH_SHELL( INT code, WPARAM wp, LPARAM lp )
{
    return call_hook_16( WH_SHELL, code, wp, lp );
}


/***********************************************************************
 *		SetWindowsHook (USER.121)
 */
FARPROC16 WINAPI SetWindowsHook16( INT16 id, HOOKPROC16 proc )
{
    HINSTANCE16 hInst = FarGetOwner16( HIWORD(proc) );

    /* WH_MSGFILTER is the only task-specific hook for SetWindowsHook() */
    HTASK16 hTask = (id == WH_MSGFILTER) ? GetCurrentTask() : 0;

    return (FARPROC16)SetWindowsHookEx16( id, proc, hInst, hTask );
}


/***********************************************************************
 *		SetWindowsHookEx (USER.291)
 */
HHOOK WINAPI SetWindowsHookEx16( INT16 id, HOOKPROC16 proc, HINSTANCE16 hInst, HTASK16 hTask )
{
    struct user_thread_info *thread_info = get_user_thread_info();
    struct hook16_queue_info *info;
    HHOOK hook;
    int index = id - WH_MINHOOK;

    if (id < WH_MINHOOK || id > WH_MAXHOOK16) return 0;
    if (!hook_procs[index])
    {
        FIXME( "hook type %d broken in Win16\n", id );
        return 0;
    }
    if (!hTask) FIXME( "System-global hooks (%d) broken in Win16\n", id );
    else if (hTask != GetCurrentTask())
    {
        FIXME( "setting hook (%d) on other task not supported\n", id );
        return 0;
    }

    if (!(info = thread_info->hook16_info))
    {
        if (!(info = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*info) ))) return 0;
        thread_info->hook16_info = info;
    }
    if (info->hook[index])
    {
        FIXME( "Multiple hooks (%d) for the same task not supported yet\n", id );
        return 0;
    }
    if (!(hook = SetWindowsHookExA( id, hook_procs[index], 0, GetCurrentThreadId() ))) return 0;
    info->hook[index] = hook;
    info->proc[index] = proc;
    return hook;
}


/***********************************************************************
 *		UnhookWindowsHook (USER.234)
 */
BOOL16 WINAPI UnhookWindowsHook16( INT16 id, HOOKPROC16 proc )
{
    struct hook16_queue_info *info;
    int index = id - WH_MINHOOK;

    if (id < WH_MINHOOK || id > WH_MAXHOOK16) return FALSE;
    if (!(info = get_user_thread_info()->hook16_info)) return FALSE;
    if (info->proc[index] != proc) return FALSE;
    if (!UnhookWindowsHookEx( info->hook[index] )) return FALSE;
    info->hook[index] = 0;
    info->proc[index] = 0;
    return TRUE;
}


/***********************************************************************
 *		UnhookWindowsHookEx (USER.292)
 */
BOOL16 WINAPI UnhookWindowsHookEx16( HHOOK hhook )
{
    struct hook16_queue_info *info;
    int index;

    if (!(info = get_user_thread_info()->hook16_info)) return FALSE;
    for (index = 0; index < NB_HOOKS16; index++)
    {
        if (info->hook[index] == hhook)
        {
            info->hook[index] = 0;
            info->proc[index] = 0;
            return UnhookWindowsHookEx( hhook );
        }
    }
    return FALSE;
}


/***********************************************************************
 *		CallMsgFilter32 (USER.823)
 */
BOOL16 WINAPI CallMsgFilter32_16( MSG32_16 *lpmsg16_32, INT16 code, BOOL16 wHaveParamHigh )
{
    MSG msg32;
    BOOL16 ret;

    if (GetSysModalWindow16()) return FALSE;
    msg32.hwnd      = WIN_Handle32( lpmsg16_32->msg.hwnd );
    msg32.message   = lpmsg16_32->msg.message;
    msg32.lParam    = lpmsg16_32->msg.lParam;
    msg32.time      = lpmsg16_32->msg.time;
    msg32.pt.x      = lpmsg16_32->msg.pt.x;
    msg32.pt.y      = lpmsg16_32->msg.pt.y;
    if (wHaveParamHigh) msg32.wParam = MAKELONG(lpmsg16_32->msg.wParam, lpmsg16_32->wParamHigh);
    else msg32.wParam = lpmsg16_32->msg.wParam;

    ret = (BOOL16)CallMsgFilterA(&msg32, code);

    lpmsg16_32->msg.hwnd    = HWND_16( msg32.hwnd );
    lpmsg16_32->msg.message = msg32.message;
    lpmsg16_32->msg.wParam  = LOWORD(msg32.wParam);
    lpmsg16_32->msg.lParam  = msg32.lParam;
    lpmsg16_32->msg.time    = msg32.time;
    lpmsg16_32->msg.pt.x    = msg32.pt.x;
    lpmsg16_32->msg.pt.y    = msg32.pt.y;
    if (wHaveParamHigh) lpmsg16_32->wParamHigh = HIWORD(msg32.wParam);
    return ret;
}


/***********************************************************************
 *		CallMsgFilter (USER.123)
 */
BOOL16 WINAPI CallMsgFilter16( MSG16 *msg, INT16 code )
{
    return CallMsgFilter32_16( (MSG32_16 *)msg, code, FALSE );
}


/***********************************************************************
 *		CallNextHookEx (USER.293)
 */
LRESULT WINAPI CallNextHookEx16( HHOOK hhook, INT16 code, WPARAM16 wparam, LPARAM lparam )
{
    struct hook16_queue_info *info;
    LRESULT ret = 0;

    if (!(info = get_user_thread_info()->hook16_info)) return 0;

    switch (info->id)
    {
    case WH_MSGFILTER:
    {
        MSG16 *msg16 = MapSL(lparam);
        MSG msg32;

        map_msg_16_to_32( msg16, &msg32 );
        ret = CallNextHookEx( hhook, code, wparam, (LPARAM)&msg32 );
        break;
    }

    case WH_GETMESSAGE:
    {
        MSG16 *msg16 = MapSL(lparam);
        MSG msg32;

        map_msg_16_to_32( msg16, &msg32 );
        ret = CallNextHookEx( hhook, code, wparam, (LPARAM)&msg32 );
        map_msg_32_to_16( &msg32, msg16 );
        break;
    }

    case WH_CALLWNDPROC:
    {
        CWPSTRUCT16 *cwp16 = MapSL(lparam);
        LRESULT result;
        struct wndproc_hook_params params;

        params.hhook  = hhook;
        params.code   = code;
        params.wparam = wparam;
        ret = WINPROC_CallProc16To32A( wndproc_hook_callback, cwp16->hwnd, cwp16->message,
                                       cwp16->wParam, cwp16->lParam, &result, &params );
        break;
    }

    case WH_CBT:
        switch (code)
        {
        case HCBT_CREATEWND:
            {
                CBT_CREATEWNDA cbtcw32;
                CREATESTRUCTA cs32;
                CBT_CREATEWND16 *cbtcw16 = MapSL(lparam);
                CREATESTRUCT16 *cs16 = MapSL( (SEGPTR)cbtcw16->lpcs );

                cbtcw32.lpcs = &cs32;
                cbtcw32.hwndInsertAfter = WIN_Handle32( cbtcw16->hwndInsertAfter );

                cs32.lpCreateParams = (LPVOID)cs16->lpCreateParams;
                cs32.hInstance      = HINSTANCE_32(cs16->hInstance);
                cs32.hMenu          = HMENU_32(cs16->hMenu);
                cs32.hwndParent     = WIN_Handle32(cs16->hwndParent);
                cs32.cy             = cs16->cy;
                cs32.cx             = cs16->cx;
                cs32.y              = cs16->y;
                cs32.x              = cs16->x;
                cs32.style          = cs16->style;
                cs32.lpszName       = MapSL( cs16->lpszName );
                cs32.lpszClass      = MapSL( cs16->lpszClass );
                cs32.dwExStyle      = cs16->dwExStyle;

                ret = CallNextHookEx( hhook, code, wparam, (LPARAM)&cbtcw32 );
                cbtcw16->hwndInsertAfter = HWND_16( cbtcw32.hwndInsertAfter );
                break;
            }
        case HCBT_ACTIVATE:
            {
                CBTACTIVATESTRUCT16 *cas16 = MapSL(lparam);
                CBTACTIVATESTRUCT cas32;
                cas32.fMouse = cas16->fMouse;
                cas32.hWndActive = WIN_Handle32(cas16->hWndActive);
                ret = CallNextHookEx( hhook, code, wparam, (LPARAM)&cas32 );
                break;
            }
        case HCBT_CLICKSKIPPED:
            {
                MOUSEHOOKSTRUCT16 *ms16 = MapSL(lparam);
                MOUSEHOOKSTRUCT ms32;

                ms32.pt.x = ms16->pt.x;
                ms32.pt.y = ms16->pt.y;
                /* wHitTestCode may be negative, so convince compiler to do
                   correct sign extension. Yay. :| */
                ms32.wHitTestCode = (INT)(INT16)ms16->wHitTestCode;
                ms32.dwExtraInfo = ms16->dwExtraInfo;
                ms32.hwnd = WIN_Handle32( ms16->hwnd );
                ret = CallNextHookEx( hhook, code, wparam, (LPARAM)&ms32 );
                break;
            }
        case HCBT_MOVESIZE:
            {
                RECT16 *rect16 = MapSL(lparam);
                RECT rect32;

                rect32.left   = rect16->left;
                rect32.top    = rect16->top;
                rect32.right  = rect16->right;
                rect32.bottom = rect16->bottom;
                ret = CallNextHookEx( hhook, code, wparam, (LPARAM)&rect32 );
                break;
            }
        }
        break;

    case WH_MOUSE:
    {
        MOUSEHOOKSTRUCT16 *ms16 = MapSL(lparam);
        MOUSEHOOKSTRUCT ms32;

        ms32.pt.x = ms16->pt.x;
        ms32.pt.y = ms16->pt.y;
        /* wHitTestCode may be negative, so convince compiler to do
           correct sign extension. Yay. :| */
        ms32.wHitTestCode = (INT)((INT16)ms16->wHitTestCode);
        ms32.dwExtraInfo = ms16->dwExtraInfo;
        ms32.hwnd = WIN_Handle32(ms16->hwnd);
        ret = CallNextHookEx( hhook, code, wparam, (LPARAM)&ms32 );
        break;
    }

    case WH_SHELL:
    case WH_KEYBOARD:
        ret = CallNextHookEx( hhook, code, wparam, lparam );
        break;

    case WH_HARDWARE:
    case WH_FOREGROUNDIDLE:
    case WH_CALLWNDPROCRET:
    case WH_SYSMSGFILTER:
    case WH_JOURNALRECORD:
    case WH_JOURNALPLAYBACK:
    default:
        FIXME("\t[%i] 16to32 translation unimplemented\n", info->id);
        ret = CallNextHookEx( hhook, code, wparam, lparam );
        break;
    }
    return ret;
}


/***********************************************************************
 *		DefHookProc (USER.235)
 */
LRESULT WINAPI DefHookProc16( INT16 code, WPARAM16 wparam, LPARAM lparam, HHOOK *hhook )
{
    return CallNextHookEx16( *hhook, code, wparam, lparam );
}
