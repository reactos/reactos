/*
 * Copyright 2004 Martin Fuchs
 *
 * Pass on icon notification messages to the systray implementation
 * in the currently running shell.
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "shellapi.h"


 /* copy data structure for tray notifications */
typedef struct TrayNotifyCDSA {
	DWORD	cookie;
	DWORD	notify_code;
	NOTIFYICONDATAA	nicon_data;
} TrayNotifyCDSA;

typedef struct TrayNotifyCDSW {
	DWORD	cookie;
	DWORD	notify_code;
	NOTIFYICONDATAW	nicon_data;
} TrayNotifyCDSW;


/*************************************************************************
 * Shell_NotifyIcon			[SHELL32.296]
 * Shell_NotifyIconA			[SHELL32.297]
 */
BOOL WINAPI Shell_NotifyIconA(DWORD dwMessage, PNOTIFYICONDATAA pnid)
{
	HWND hwnd;
	COPYDATASTRUCT data;
	TrayNotifyCDSA notify_data;

	BOOL ret = FALSE;

	notify_data.cookie = 1;
	notify_data.notify_code = dwMessage;
	memcpy(&notify_data.nicon_data, pnid, sizeof(NOTIFYICONDATAA));

	data.dwData = 1;
	data.cbData = sizeof(notify_data);
	data.lpData = &notify_data;

	for(hwnd=0; hwnd=FindWindowExA(0, hwnd, "Shell_TrayWnd", NULL); ) {
		if (SendMessageA(hwnd, WM_COPYDATA, (WPARAM)pnid->hWnd, (LPARAM)&data))
			ret = TRUE;
	}

	return ret;
}

/*************************************************************************
 * Shell_NotifyIconW			[SHELL32.298]
 */
BOOL WINAPI Shell_NotifyIconW(DWORD dwMessage, PNOTIFYICONDATAW pnid)
{
	HWND hwnd;
	COPYDATASTRUCT data;
	TrayNotifyCDSW notify_data;

	BOOL ret = FALSE;

	notify_data.cookie = 1;
	notify_data.notify_code = dwMessage;
	memcpy(&notify_data.nicon_data, pnid, sizeof(NOTIFYICONDATAW));

	data.dwData = 1;
	data.cbData = sizeof(notify_data);
	data.lpData = &notify_data;

	for(hwnd=0; hwnd=FindWindowExW(0, hwnd, L"Shell_TrayWnd", NULL); ) {
		if (SendMessageW(hwnd, WM_COPYDATA, (WPARAM)pnid->hWnd, (LPARAM)&data))
			ret = TRUE;
	}

	return ret;
}
