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
#include <sstream>


DWORD WINAPI Thread::ThreadProc(void* para)
{
	Thread* pThis = (Thread*) para;

	int ret = pThis->Run();

	pThis->_alive = false;

	return ret;
}


void CenterWindow(HWND hwnd)
{
	RECT rt, prt;
	GetWindowRect(hwnd, &rt);

	DWORD style;
	HWND owner = 0;

	for(HWND wh=hwnd; (wh=GetWindow(wh,GW_OWNER))!=0; )
		if (((style=GetWindowStyle(wh))&WS_VISIBLE) && !(style&WS_MINIMIZE))
			{owner=wh; break;}

	if (owner)
		GetWindowRect(owner, &prt);
	else
		SystemParametersInfo(SPI_GETWORKAREA, 0, &prt, 0);	//@@ GetDesktopWindow() wäre auch hilfreich.

	SetWindowPos(hwnd, 0, (prt.left+prt.right+rt.left-rt.right)/2,
					   (prt.top+prt.bottom+rt.top-rt.bottom)/2, 0,0, SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOZORDER);

	MoveVisible(hwnd);
}

void MoveVisible(HWND hwnd)
{
	RECT rc;
	GetWindowRect(hwnd, &rc);
	int left=rc.left, top=rc.top;

	int xmax = GetSystemMetrics(SM_CXSCREEN);
	int ymax = GetSystemMetrics(SM_CYSCREEN);

	if (rc.left < 0)
		rc.left = 0;
	else if (rc.right > xmax)
		if ((rc.left-=rc.right-xmax) < 0)
			rc.left = 0;

	if (rc.top < 0)
		rc.top = 0;
	else if (rc.bottom > ymax)
		if ((rc.top-=rc.bottom-ymax) < 0)
			rc.top = 0;

	if (rc.left!=left || rc.top!=top)
		SetWindowPos(hwnd, 0, rc.left,rc.top, 0,0, SWP_NOZORDER|SWP_NOSIZE|SWP_NOACTIVATE);
}


void display_error(HWND hwnd, DWORD error)	//@@ CONTEXT mit ausgeben -> display_error(HWND hwnd, const Exception& e)
{
	PTSTR msg;

	if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM,
		0, error, MAKELANGID(LANG_NEUTRAL,SUBLANG_DEFAULT), (PTSTR)&msg, 0, NULL)) {
		LOG(FmtString(TEXT("display_error(%#x): %s"), error, msg));

		SetLastError(0);
		MessageBox(hwnd, msg, TEXT("ROS Explorer"), MB_OK);

		if (GetLastError() == ERROR_INVALID_WINDOW_HANDLE)
			MessageBox(0, msg, TEXT("ROS Explorer"), MB_OK);
	} else {
		LOG(FmtString(TEXT("Unknown Error %#x"), error));

		FmtString msg(TEXT("Unknown Error %#x"), error);

		SetLastError(0);
		MessageBox(hwnd, msg, TEXT("ROS Explorer"), MB_OK);

		if (GetLastError() == ERROR_INVALID_WINDOW_HANDLE)
			MessageBox(0, msg, TEXT("ROS Explorer"), MB_OK);
	}

	LocalFree(msg);
}


Context Context::s_main("-NO-CONTEXT-");
Context* Context::s_current = &Context::s_main;

String Context::toString() const
{
	String str = _ctx;

	if (!_obj.empty())
		str.appendf(TEXT("\nObject: %s"), (LPCTSTR)_obj);

	return str;
}

String Context::getStackTrace() const
{
	ostringstream str;

	str << "Context Trace:\n";

	for(const Context*p=this; p && p!=&s_main; p=p->_last) {
		str << "- " << p->_ctx;

		if (!p->_obj.empty())
			str << " obj=" << ANS(p->_obj);

		str << '\n';
	}

	return str.str();
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
	CONTEXT("launch_file()");

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


BOOL RecursiveCreateDirectory(LPCTSTR path_in)
{
	TCHAR path[MAX_PATH], hole_path[MAX_PATH];

	_tcscpy(hole_path, path_in);

	int drv_len = 0;
	LPCTSTR d;

	for(d=hole_path; *d && *d!='/' && *d!='\\'; ++d) {
		++drv_len;

		if (*d == ':')
			break;
	}

	LPTSTR dir = hole_path + drv_len;

	int l;
	LPTSTR p = hole_path + (l=_tcslen(hole_path));

	while(--p>=hole_path && (*p=='/' || *p=='\\'))
		*p = '\0';

	WIN32_FIND_DATA w32fd;

	HANDLE hFind = FindFirstFile(hole_path, &w32fd);

	if (hFind == INVALID_HANDLE_VALUE) {
		_tcsncpy(path, hole_path, drv_len);
		int i = drv_len;

		for(p=dir; *p=='/'||*p=='\\'; p++)
			path[i++] = *p++;

		for(; i<l; i++) {
			memcpy(path, hole_path, i*sizeof(TCHAR));

			for(; hole_path[i] && hole_path[i]!='/' && hole_path[i]!='\\'; i++)
				path[i] = hole_path[i];

			path[i] = '\0';

			hFind = FindFirstFile(path, &w32fd);

			if (hFind != INVALID_HANDLE_VALUE)
				FindClose(hFind);
			else {
				LOG(FmtString(TEXT("CreateDirectory(\"%s\")"), path));

				if (!CreateDirectory(path, 0))
					return FALSE;
			}
		}
	} else
		FindClose(hFind);

	return TRUE;
}
