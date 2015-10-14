/*
 * NTDRV mouse driver
 *
 * Copyright 1998 Ulrich Weigand
 * Copyright 2007 Henri Verbeet
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

#include "winent.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(cursor);

/**********************************************************************/

#define NB_BUTTONS   9     /* Windows can handle 5 buttons and the wheel too */

static const UINT button_down_flags[NB_BUTTONS] =
{
    MOUSEEVENTF_LEFTDOWN,
    MOUSEEVENTF_MIDDLEDOWN,
    MOUSEEVENTF_RIGHTDOWN,
    MOUSEEVENTF_WHEEL,
    MOUSEEVENTF_WHEEL,
    MOUSEEVENTF_XDOWN,  /* FIXME: horizontal wheel */
    MOUSEEVENTF_XDOWN,
    MOUSEEVENTF_XDOWN,
    MOUSEEVENTF_XDOWN
};

static const UINT button_up_flags[NB_BUTTONS] =
{
    MOUSEEVENTF_LEFTUP,
    MOUSEEVENTF_MIDDLEUP,
    MOUSEEVENTF_RIGHTUP,
    0,
    0,
    MOUSEEVENTF_XUP,
    MOUSEEVENTF_XUP,
    MOUSEEVENTF_XUP,
    MOUSEEVENTF_XUP
};

static BYTE TrackSysKey = 0; /* determine whether ALT key up will cause a WM_SYSKEYUP
                                or a WM_KEYUP message */

/***********************************************************************
 *           get_key_state
 */
static WORD get_key_state(BYTE* key_state_table)
{
    WORD ret = 0;

    if (GetSystemMetrics( SM_SWAPBUTTON ))
    {
        if (key_state_table[VK_RBUTTON] & 0x80) ret |= MK_LBUTTON;
        if (key_state_table[VK_LBUTTON] & 0x80) ret |= MK_RBUTTON;
    }
    else
    {
        if (key_state_table[VK_LBUTTON] & 0x80) ret |= MK_LBUTTON;
        if (key_state_table[VK_RBUTTON] & 0x80) ret |= MK_RBUTTON;
    }
    if (key_state_table[VK_MBUTTON] & 0x80)  ret |= MK_MBUTTON;
    if (key_state_table[VK_SHIFT] & 0x80)    ret |= MK_SHIFT;
    if (key_state_table[VK_CONTROL] & 0x80)  ret |= MK_CONTROL;
    if (key_state_table[VK_XBUTTON1] & 0x80) ret |= MK_XBUTTON1;
    if (key_state_table[VK_XBUTTON2] & 0x80) ret |= MK_XBUTTON2;
    return ret;
}

/***********************************************************************
 *           queue_raw_mouse_message
 */
static void queue_raw_mouse_message( UINT message, HWND hwnd, DWORD x, DWORD y,
                                     DWORD data, DWORD time, DWORD extra_info, UINT injected_flags,
                                     BYTE* key_state_table)
{
    MSLLHOOKSTRUCT hook;
    HCURSOR cursor;

    hook.pt.x        = x;
    hook.pt.y        = y;
    hook.mouseData   = MAKELONG( 0, data );
    hook.flags       = injected_flags;
    hook.time        = time;
    hook.dwExtraInfo = extra_info;

    if (HOOK_CallHooks( WH_MOUSE_LL, HC_ACTION, message, (LPARAM)&hook, TRUE ))
        message = 0;  /* ignore it */

    SERVER_START_REQ( send_hardware_message )
    {
        req->id       = (injected_flags & LLMHF_INJECTED) ? 0 : GetCurrentThreadId();
        req->win      = wine_server_user_handle( hwnd );
        req->msg      = message;
        req->wparam   = MAKEWPARAM( get_key_state(key_state_table), data );
        req->lparam   = 0;
        req->x        = x;
        req->y        = y;
        req->time     = time;
        req->info     = extra_info;
        wine_server_call( req );
        cursor = (reply->count >= 0) ? wine_server_ptr_handle(reply->cursor) : 0;
    }
    SERVER_END_REQ;

    TRACE("cursor: 0x%x\n", cursor);
}

/***********************************************************************
 *      NTDRV_SendMouseInput
 */
void NTDRV_SendMouseInput( HWND hwnd, DWORD flags, DWORD x, DWORD y,
                              DWORD data, DWORD time, DWORD extra_info, UINT injected_flags )
{
    POINT pt;
    POINT cursor_pos;
    BYTE key_state_table[256];

    RosUserGetAsyncKeyboardState(key_state_table);

    if (flags & MOUSEEVENTF_MOVE && flags & MOUSEEVENTF_ABSOLUTE)
    {
        if (injected_flags & LLMHF_INJECTED)
        {
            pt.x = (x * GetSystemMetrics(SM_CXVIRTUALSCREEN)) >> 16;
            pt.y = (y * GetSystemMetrics(SM_CYVIRTUALSCREEN)) >> 16;
        }
        else
        {
            pt.x = x;
            pt.y = y;
            
			GetCursorPos(&cursor_pos);
            if (cursor_pos.x == x && cursor_pos.y == y &&
                (flags & ~(MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE)))
                flags &= ~MOUSEEVENTF_MOVE;

        }
    }
    else if (flags & MOUSEEVENTF_MOVE)
    {
        int accel[3], xMult = 1, yMult = 1;

        /* dx and dy can be negative numbers for relative movements */
        SystemParametersInfoW(SPI_GETMOUSE, 0, accel, 0);

        if (abs(x) > accel[0] && accel[2] != 0)
        {
            xMult = 2;
            if ((abs(x) > accel[1]) && (accel[2] == 2)) xMult = 4;
        }
        if (abs(y) > accel[0] && accel[2] != 0)
        {
            yMult = 2;
            if ((abs(y) > accel[1]) && (accel[2] == 2)) yMult = 4;
        }

		GetCursorPos(&cursor_pos);
        pt.x = cursor_pos.x + (long)x * xMult;
        pt.y = cursor_pos.y + (long)y * yMult;
    }
    else
    {
		GetCursorPos(&pt);
    }

    /* get the window handle from cursor position */
    //hwnd = ChildWindowFromPointEx(GetDesktopWindow(), pt, CWP_SKIPINVISIBLE);
    hwnd = SwmGetWindowFromPoint(pt.x, pt.y);

    if (flags & MOUSEEVENTF_MOVE)
    {
        queue_raw_mouse_message( WM_MOUSEMOVE, hwnd, pt.x, pt.y, data, time,
                                 extra_info, injected_flags, key_state_table );
		RosDrv_SetCursorPos(pt.x, pt.y);
    }
    if (flags & MOUSEEVENTF_LEFTDOWN)
    {
        key_state_table[VK_LBUTTON] |= 0xc0;
        queue_raw_mouse_message( GetSystemMetrics(SM_SWAPBUTTON) ? WM_RBUTTONDOWN : WM_LBUTTONDOWN,
                                 hwnd, pt.x, pt.y, data, time, extra_info, injected_flags, key_state_table );
    }
    if (flags & MOUSEEVENTF_LEFTUP)
    {
        key_state_table[VK_LBUTTON] &= ~0x80;
        queue_raw_mouse_message( GetSystemMetrics(SM_SWAPBUTTON) ? WM_RBUTTONUP : WM_LBUTTONUP,
                                 hwnd, pt.x, pt.y, data, time, extra_info, injected_flags, key_state_table );
    }
    if (flags & MOUSEEVENTF_RIGHTDOWN)
    {
        key_state_table[VK_RBUTTON] |= 0xc0;
        queue_raw_mouse_message( GetSystemMetrics(SM_SWAPBUTTON) ? WM_LBUTTONDOWN : WM_RBUTTONDOWN,
                                 hwnd, pt.x, pt.y, data, time, extra_info, injected_flags, key_state_table );
    }
    if (flags & MOUSEEVENTF_RIGHTUP)
    {
        key_state_table[VK_RBUTTON] &= ~0x80;
        queue_raw_mouse_message( GetSystemMetrics(SM_SWAPBUTTON) ? WM_LBUTTONUP : WM_RBUTTONUP,
                                 hwnd, pt.x, pt.y, data, time, extra_info, injected_flags, key_state_table );
    }
    if (flags & MOUSEEVENTF_MIDDLEDOWN)
    {
        key_state_table[VK_MBUTTON] |= 0xc0;
        queue_raw_mouse_message( WM_MBUTTONDOWN, hwnd, pt.x, pt.y, data, time,
                                 extra_info, injected_flags, key_state_table );
    }
    if (flags & MOUSEEVENTF_MIDDLEUP)
    {
        key_state_table[VK_MBUTTON] &= ~0x80;
        queue_raw_mouse_message( WM_MBUTTONUP, hwnd, pt.x, pt.y, data, time,
                                 extra_info, injected_flags, key_state_table );
    }
    if (flags & MOUSEEVENTF_WHEEL)
    {
        queue_raw_mouse_message( WM_MOUSEWHEEL, hwnd, pt.x, pt.y, data, time,
                                 extra_info, injected_flags, key_state_table );
    }
    if (flags & MOUSEEVENTF_XDOWN)
    {
        key_state_table[VK_XBUTTON1 + data - 1] |= 0xc0;
        queue_raw_mouse_message( WM_XBUTTONDOWN, hwnd, pt.x, pt.y, data, time,
                                 extra_info, injected_flags, key_state_table );
    }
    if (flags & MOUSEEVENTF_XUP)
    {
        key_state_table[VK_XBUTTON1 + data - 1] &= ~0x80;
        queue_raw_mouse_message( WM_XBUTTONUP, hwnd, pt.x, pt.y, data, time,
                                 extra_info, injected_flags, key_state_table );
    }

    RosUserSetAsyncKeyboardState(key_state_table);
}


/***********************************************************************
 *           NTDRV_SendKeyboardInput
 */
void NTDRV_SendKeyboardInput( WORD wVk, WORD wScan, DWORD event_flags, DWORD time,
                                 DWORD dwExtraInfo, UINT injected_flags )
{
    UINT message;
    KBDLLHOOKSTRUCT hook;
    WORD flags, wVkStripped, wVkL, wVkR, vk_hook = wVk;
    BYTE key_state_table[256];
    POINT cursor_pos;

    RosUserGetAsyncKeyboardState(key_state_table);

    wVk = LOBYTE(wVk);
    flags = LOBYTE(wScan);

    if (event_flags & KEYEVENTF_EXTENDEDKEY) flags |= KF_EXTENDED;
    /* FIXME: set KF_DLGMODE and KF_MENUMODE when needed */

    /* strip left/right for menu, control, shift */
    switch (wVk)
    {
    case VK_MENU:
    case VK_LMENU:
    case VK_RMENU:
        wVk = (event_flags & KEYEVENTF_EXTENDEDKEY) ? VK_RMENU : VK_LMENU;
        wVkStripped = VK_MENU;
        wVkL = VK_LMENU;
        wVkR = VK_RMENU;
        break;
    case VK_CONTROL:
    case VK_LCONTROL:
    case VK_RCONTROL:
        wVk = (event_flags & KEYEVENTF_EXTENDEDKEY) ? VK_RCONTROL : VK_LCONTROL;
        wVkStripped = VK_CONTROL;
        wVkL = VK_LCONTROL;
        wVkR = VK_RCONTROL;
        break;
    case VK_SHIFT:
    case VK_LSHIFT:
    case VK_RSHIFT:
        wVk = (event_flags & KEYEVENTF_EXTENDEDKEY) ? VK_RSHIFT : VK_LSHIFT;
        wVkStripped = VK_SHIFT;
        wVkL = VK_LSHIFT;
        wVkR = VK_RSHIFT;
        break;
    default:
        wVkStripped = wVkL = wVkR = wVk;
    }

    if (event_flags & KEYEVENTF_KEYUP)
    {
        message = WM_KEYUP;
        if ((key_state_table[VK_MENU] & 0x80) &&
            ((wVkStripped == VK_MENU) || (wVkStripped == VK_CONTROL)
             || !(key_state_table[VK_CONTROL] & 0x80)))
        {
            if( TrackSysKey == VK_MENU || /* <ALT>-down/<ALT>-up sequence */
                (wVkStripped != VK_MENU)) /* <ALT>-down...<something else>-up */
                message = WM_SYSKEYUP;
            TrackSysKey = 0;
        }
        flags |= KF_REPEAT | KF_UP;
    }
    else
    {
        message = WM_KEYDOWN;
        if ((key_state_table[VK_MENU] & 0x80 || wVkStripped == VK_MENU) &&
            !(key_state_table[VK_CONTROL] & 0x80 || wVkStripped == VK_CONTROL))
        {
            message = WM_SYSKEYDOWN;
            TrackSysKey = wVkStripped;
        }
        if (key_state_table[wVk] & 0x80) flags |= KF_REPEAT;
    }

    TRACE("KEY: wParam=%04x, lParam=%08lx, InputKeyState=%x\n",
                wVk, MAKELPARAM( 1, flags ), key_state_table[wVk] );

    /* Hook gets whatever key was sent. */
    hook.vkCode      = vk_hook;
    hook.scanCode    = wScan;
    hook.flags       = (flags >> 8) | injected_flags;
    hook.time        = time;
    hook.dwExtraInfo = dwExtraInfo;
    if (HOOK_CallHooks( WH_KEYBOARD_LL, HC_ACTION, message, (LPARAM)&hook, TRUE )) return;

    if (event_flags & KEYEVENTF_KEYUP)
    {
        key_state_table[wVk] &= ~0x80;
        key_state_table[wVkStripped] = key_state_table[wVkL] | key_state_table[wVkR];
    }
    else
    {
        if (!(key_state_table[wVk] & 0x80)) key_state_table[wVk] ^= 0x01;
        key_state_table[wVk] |= 0xc0;
        key_state_table[wVkStripped] = key_state_table[wVkL] | key_state_table[wVkR];
    }

    if (key_state_table[VK_MENU] & 0x80) flags |= KF_ALTDOWN;

    if (wVkStripped == VK_SHIFT) flags &= ~KF_EXTENDED;

    GetCursorPos(&cursor_pos);

    SERVER_START_REQ( send_hardware_message )
    {
        req->id       = (injected_flags & LLKHF_INJECTED) ? 0 : GetCurrentThreadId();
        req->win      = 0;
        req->msg      = message;
        req->wparam   = wVk;
        req->lparam   = MAKELPARAM( 1 /* repeat count */, flags );
        req->x        = cursor_pos.x;
        req->y        = cursor_pos.y;
        req->time     = time;
        req->info     = dwExtraInfo;
        wine_server_call( req );
    }
    SERVER_END_REQ;

    RosUserSetAsyncKeyboardState(key_state_table);
}

/***********************************************************************
 *		DestroyCursorIcon (NTDRV.@)
 */
void CDECL RosDrv_DestroyCursorIcon( HCURSOR handle )
{
}

void CDECL RosDrv_SetCursor( HCURSOR handle )
{
    ICONINFO iconinfo;

    if(handle == NULL)
    {
        RosUserSetCursor(NULL);
    }
    else
    {
        GetIconInfo(handle, &iconinfo);
        iconinfo.hbmColor = (HBITMAP)MapUserHandle(iconinfo.hbmColor);
        iconinfo.hbmMask  = (HBITMAP)MapUserHandle(iconinfo.hbmMask);
        RosUserSetCursor(&iconinfo);
    }
}

