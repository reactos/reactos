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
 // utility.cpp
 //
 // Martin Fuchs, 23.07.2003
 //


#include "utility.h"
#include <shellapi.h>

#include <time.h>


DWORD WINAPI Thread::ThreadProc(void* para)
{
	Thread* pThis = (Thread*) para;

	int ret = pThis->Run();

	pThis->_alive = false;

	return ret;
}


void display_error(HWND hwnd, DWORD error)
{
	PTSTR msg;

	if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM,
		0, error, MAKELANGID(LANG_NEUTRAL,SUBLANG_DEFAULT), (PTSTR)&msg, 0, NULL))
		MessageBox(hwnd, msg, TEXT("Winefile"), MB_OK);
	else
		MessageBox(hwnd, TEXT("Error"), TEXT("Winefile"), MB_OK);

	LocalFree(msg);
}


BOOL time_to_filetime(const time_t* t, FILETIME* ftime)
{
	struct tm* tm = gmtime(t);
	SYSTEMTIME stime;

	if (!tm)
		return FALSE;

	stime.wYear = tm->tm_year+1900;
	stime.wMonth = tm->tm_mon+1;
	/*	stime.wDayOfWeek */
	stime.wDay = tm->tm_mday;
	stime.wHour = tm->tm_hour;
	stime.wMinute = tm->tm_min;
	stime.wSecond = tm->tm_sec;

	return SystemTimeToFileTime(&stime, ftime);
}


BOOL launch_file(HWND hwnd, LPCTSTR cmd, UINT nCmdShow)
{
	HINSTANCE hinst = ShellExecute(hwnd, NULL/*operation*/, cmd, NULL/*parameters*/, NULL/*dir*/, nCmdShow);

	if ((int)hinst <= 32) {
		display_error(hwnd, GetLastError());
		return FALSE;
	}

	return TRUE;
}

#ifdef UNICODE
BOOL launch_fileA(HWND hwnd, LPSTR cmd, UINT nCmdShow)
{
	HINSTANCE hinst = ShellExecuteA(hwnd, NULL/*operation*/, cmd, NULL/*parameters*/, NULL/*dir*/, nCmdShow);

	if ((int)hinst <= 32) {
		display_error(hwnd, GetLastError());
		return FALSE;
	}

	return TRUE;
}
#endif


/* search for already running win[e]files */

static int g_foundPrevInstance = 0;

static BOOL CALLBACK EnumWndProc(HWND hwnd, LPARAM lparam)
{
	TCHAR cls[128];

	GetClassName(hwnd, cls, 128);

	if (!lstrcmp(cls, (LPCTSTR)lparam)) {
		g_foundPrevInstance++;
		return FALSE;
	}

	return TRUE;
}

/* search for window of given class name to allow only one running instance */
int find_window_class(LPCTSTR classname)
{
	EnumWindows(EnumWndProc, (LPARAM)classname);

	if (g_foundPrevInstance)
		return 1;

	return 0;
}


typedef void (WINAPI*RUNDLLPROC)(HWND hwnd, HINSTANCE hinst, LPCTSTR cmdline, DWORD nCmdShow);

BOOL RunDLL(HWND hwnd, LPCTSTR dllname, LPCSTR procname, LPCTSTR cmdline, UINT nCmdShow)
{
	HMODULE hmod = LoadLibrary(dllname);
	if (!hmod)
		return FALSE;

/*TODO
	<Windows NT/2000>
	It is possible to create a Unicode version of the function.
	Rundll32 first tries to find a function named EntryPointW.
	If it cannot find this function, it tries EntryPointA, then EntryPoint.
	To create a DLL that supports ANSI on Windows 95/98/Me and Unicode otherwise,
	export two functions: EntryPointW and EntryPoint.
*/
	RUNDLLPROC proc = (RUNDLLPROC)GetProcAddress(hmod, procname);
	if (!proc) {
		FreeLibrary(hmod);
		return FALSE;
	}

	proc(hwnd, hmod, cmdline, nCmdShow);

	FreeLibrary(hmod);

	return TRUE;
}
