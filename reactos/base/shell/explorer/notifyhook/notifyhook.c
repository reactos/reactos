/*
 * Copyright 2004 Martin Fuchs
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


 //
 // NotifyHook DLL for ROS Explorer
 //
 // notifyhook.cpp
 //
 // Martin Fuchs, 17.03.2004
 //


#include "../utility/utility.h"

#include "notifyhook.h"


static HINSTANCE s_hInstance;
static UINT WM_GETMODULEPATH;
static HHOOK s_hNotifyHook;


BOOL APIENTRY DllMain(HINSTANCE hInst, DWORD dwReason, LPVOID param)
{
	switch(dwReason) {
	  case DLL_PROCESS_ATTACH:
		s_hInstance = hInst;
		DisableThreadLibraryCalls(hInst);
		WM_GETMODULEPATH = RegisterWindowMessageA("WM_GETMODULEPATH");
		break;
	}

    return TRUE;
}


struct COPYDATA_STRUCT {
	HWND	_hwnd;
	int		_len;
	char	_path[MAX_PATH];
};

LRESULT CALLBACK NotifyHookProc(int code, WPARAM wparam, LPARAM lparam)
{
	MSG* pmsg = (MSG*)lparam;

	if (pmsg->message == WM_GETMODULEPATH) {
		struct COPYDATA_STRUCT cds;
		COPYDATASTRUCT data;

		cds._hwnd = pmsg->hwnd;
		cds._len = GetWindowModuleFileNameA(pmsg->hwnd, cds._path, COUNTOF(cds._path));

		data.dwData = WM_GETMODULEPATH;
		data.cbData = sizeof(cds);
		data.lpData = &cds;

		SendMessage((HWND)pmsg->wParam, WM_COPYDATA, (WPARAM)pmsg->hwnd, (LPARAM)&data);

		return 0;
	}

	return CallNextHookEx(s_hNotifyHook, code, wparam, lparam);
}


UINT InstallNotifyHook()
{
	s_hNotifyHook = SetWindowsHookEx(WH_GETMESSAGE, NotifyHookProc, s_hInstance, 0);

	return WM_GETMODULEPATH;
}

void DeinstallNotifyHook()
{
	UnhookWindowsHookEx(s_hNotifyHook);
	s_hNotifyHook = 0;
}


void GetWindowModulePath(HWND hwnd)
{
	SendMessage(hwnd, WM_GETMODULEPATH, 0, 0);
}

 // retrieve module path by receiving WM_COPYDATA message
DECL_NOTIFYHOOK int GetWindowModulePathCopyData(LPARAM lparam, HWND* phwnd, LPSTR buffer, int size)
{
	PCOPYDATASTRUCT data = (PCOPYDATASTRUCT) lparam;

	if (data->dwData == WM_GETMODULEPATH) {
		struct COPYDATA_STRUCT* cds = (struct COPYDATA_STRUCT*) data->lpData;

		*phwnd = cds->_hwnd;
		lstrcpyn(buffer, cds->_path, size);

		return cds->_len;
	} else
		return 0;
}
