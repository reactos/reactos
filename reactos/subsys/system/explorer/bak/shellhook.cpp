/*
 * Copyright 2003 Martin Fuchs
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
 // Explorer clone
 //
 // shellhook.cpp
 //
 // Martin Fuchs, 17.08.2003
 //


#include "../utility/utility.h"

#include "shellhook.h"


static HINSTANCE s_hInstance;

struct SharedMem {
	HWND	_callback_hwnd;
	UINT	_callback_msg;
	HHOOK	_hshellHook;
};

static HANDLE s_hMapObject;
static SharedMem* s_pSharedMem;


BOOL APIENTRY DllMain(HINSTANCE hInst, DWORD dwReason, LPVOID)
{
	switch(dwReason) {
	  case DLL_PROCESS_ATTACH: {
		s_hInstance = hInst;
		DisableThreadLibraryCalls(hInst);
		s_hMapObject = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE,
												0, sizeof(SharedMem), _T("ROSExplorerShellHookSharedMem"));
		if (!s_hMapObject)
			return FALSE;

		 // The first process to attach initializes memory.
		BOOL bInit = GetLastError()!=ERROR_ALREADY_EXISTS;

		 // Get a pointer to the file-mapped shared memory.
		s_pSharedMem = (SharedMem*) MapViewOfFile(s_hMapObject, FILE_MAP_WRITE, 0, 0, 0);
		if (!s_pSharedMem)
			return FALSE;

		 // Initialize memory if this is the first process.
		if (bInit)
			memset(s_pSharedMem, 0, sizeof(SharedMem));
		break;}

	  case DLL_PROCESS_DETACH:
		UnmapViewOfFile(s_pSharedMem);
		s_pSharedMem = NULL;
		CloseHandle(s_hMapObject);
		break;
	}

    return TRUE;
}


static void DoCallBack(int code, WPARAM wparam, LPARAM lparam)
{
	UINT callback_msg = s_pSharedMem->_callback_msg;
	HWND callback_hwnd = s_pSharedMem->_callback_hwnd;

	if (callback_msg)
		PostMessage(callback_hwnd, callback_msg, wparam, code);	// lparam unused
}


LRESULT CALLBACK ShellHookProc(int code, WPARAM wparam, LPARAM lparam)
{
	DoCallBack(code, wparam, lparam);

	return CallNextHookEx(s_pSharedMem->_hshellHook, code, wparam, lparam);
}


void InstallShellHook(HWND callback_hwnd, UINT callback_msg)
{
	s_pSharedMem->_callback_hwnd = callback_hwnd;
	s_pSharedMem->_callback_msg = callback_msg;

	s_pSharedMem->_hshellHook = SetWindowsHookEx(WH_SHELL, ShellHookProc, s_hInstance, 0);
}

void DeinstallShellHook()
{
	s_pSharedMem->_callback_hwnd = 0;
	s_pSharedMem->_callback_msg = 0;

	UnhookWindowsHookEx(s_pSharedMem->_hshellHook);
	s_pSharedMem->_hshellHook = 0;
}
