/*
 * Copyright 2003, 2004, 2005 Martin Fuchs
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


#include <precomp.h>

//#include <shellapi.h>

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
#ifdef __STDC_WANT_SECURE_LIB__
	SYSTEMTIME stime;
	struct tm tm_;
	struct tm* tm = &tm_;

	if (gmtime_s(tm, t) != 0)
		return FALSE;
#else
	struct tm* tm = gmtime(t);
	SYSTEMTIME stime;

	if (!tm)
		return FALSE;
#endif

	stime.wYear = tm->tm_year+1900;
	stime.wMonth = tm->tm_mon+1;
	stime.wDayOfWeek = (WORD)-1;
	stime.wDay = tm->tm_mday;
	stime.wHour = tm->tm_hour;
	stime.wMinute = tm->tm_min;
	stime.wSecond = tm->tm_sec;
	stime.wMilliseconds = 0;

	return SystemTimeToFileTime(&stime, ftime);
}


BOOL launch_file(HWND hwnd, LPCTSTR cmd, UINT nCmdShow, LPCTSTR parameters)
{
	CONTEXT("launch_file()");

	HINSTANCE hinst = ShellExecute(hwnd, NULL/*operation*/, cmd, parameters, NULL/*dir*/, nCmdShow);

	if ((int)hinst <= 32) {
		display_error(hwnd, GetLastError());
		return FALSE;
	}

	return TRUE;
}

#ifdef UNICODE
BOOL launch_fileA(HWND hwnd, LPSTR cmd, UINT nCmdShow, LPCSTR parameters)
{
	HINSTANCE hinst = ShellExecuteA(hwnd, NULL/*operation*/, cmd, parameters, NULL/*dir*/, nCmdShow);

	if ((int)hinst <= 32) {
		display_error(hwnd, GetLastError());
		return FALSE;
	}

	return TRUE;
}
#endif


/* search for already running instance */

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


String get_windows_version_str()
{
	OSVERSIONINFOEX osvi = {sizeof(OSVERSIONINFOEX)};
	BOOL osvie_val;
	String str;

	if (!(osvie_val = GetVersionEx((OSVERSIONINFO*)&osvi))) {
		osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

		if (!GetVersionEx((OSVERSIONINFO*)&osvi))
			return TEXT("???");
	}

	switch(osvi.dwPlatformId) {
	  case VER_PLATFORM_WIN32_NT:
#ifdef _ROS_	// This work around can be removed if ReactOS gets a unique version number.
		str = TEXT("ReactOS");
#else
		if (osvi.dwMajorVersion <= 4)
			str = TEXT("Microsoft Windows NT");
		else if (osvi.dwMajorVersion==5 && osvi.dwMinorVersion==0)
			str = TEXT("Microsoft Windows 2000");
		else if (osvi.dwMajorVersion==5 && osvi.dwMinorVersion==1)
			str = TEXT("Microsoft Windows XP");
#endif

		if (osvie_val) {
			if (osvi.wProductType == VER_NT_WORKSTATION) {
			   if (osvi.wSuiteMask & VER_SUITE_PERSONAL)
				  str += TEXT(" Personal");
			   else
				  str += TEXT(" Professional");
			} else if (osvi.wProductType == VER_NT_SERVER) {
			   if (osvi.wSuiteMask & VER_SUITE_DATACENTER)
				  str += TEXT(" DataCenter Server");
			   else if (osvi.wSuiteMask & VER_SUITE_ENTERPRISE)
				  str += TEXT(" Advanced Server");
			   else
				  str += TEXT(" Server");
			} else if (osvi.wProductType == VER_NT_DOMAIN_CONTROLLER) {
				str += TEXT(" Domain Controller");
			}
		} else {
			TCHAR type[80];
			DWORD dwBufLen;
			HKEY hkey;

			if (!RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("SYSTEM\\CurrentControlSet\\Control\\ProductOptions"), 0, KEY_QUERY_VALUE, &hkey)) {
				RegQueryValueEx(hkey, TEXT("ProductType"), NULL, NULL, (LPBYTE)type, &dwBufLen);
				RegCloseKey(hkey);

				if (!_tcsicmp(TEXT("WINNT"), type))
				   str += TEXT(" Workstation");
				else if (!_tcsicmp(TEXT("LANMANNT"), type))
				   str += TEXT(" Server");
				else if (!_tcsicmp(TEXT("SERVERNT"), type))
					str += TEXT(" Advanced Server");
			}
		}
		break;

	  case VER_PLATFORM_WIN32_WINDOWS:
		if (osvi.dwMajorVersion>4 ||
			(osvi.dwMajorVersion==4 && osvi.dwMinorVersion>0)) {
			if (osvi.dwMinorVersion == 90)
				str = TEXT("Microsoft Windows ME");
			else
				str = TEXT("Microsoft Windows 98");

            if (osvi.szCSDVersion[1] == 'A')
				str += TEXT(" SE");
		} else {
			str = TEXT("Microsoft Windows 95");

            if (osvi.szCSDVersion[1]=='B' || osvi.szCSDVersion[1]=='C')
				str += TEXT(" OSR2");
		}
		break;

	  case VER_PLATFORM_WIN32s:
		str = TEXT("Microsoft Win32s");

	  default:
		return TEXT("???");
	}

	String vstr;

	if (osvi.dwMajorVersion <= 4)
		vstr.printf(TEXT(" Version %d.%d %s Build %d"),
						osvi.dwMajorVersion, osvi.dwMinorVersion,
						osvi.szCSDVersion, osvi.dwBuildNumber&0xFFFF);
	else
		vstr.printf(TEXT(" %s (Build %d)"), osvi.szCSDVersion, osvi.dwBuildNumber&0xFFFF);

	return str + vstr;
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


#ifdef UNICODE
#define CONTROL_RUNDLL "Control_RunDLLW"
#else
#define CONTROL_RUNDLL "Control_RunDLLA"
#endif

BOOL launch_cpanel(HWND hwnd, LPCTSTR applet)
{
	//launch_file(_hwnd, applet, SW_SHOWNORMAL);	// This would be enough, but we want the to use the most direct and fastest call.
	//launch_file(_hwnd, String(TEXT("rundll32.exe /d shell32.dll,Control_RunDLL "))+applet, SW_SHOWNORMAL);

	return RunDLL(hwnd, TEXT("shell32"), CONTROL_RUNDLL, applet, SW_SHOWNORMAL);
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


DWORD RegGetDWORDValue(HKEY root, LPCTSTR path, LPCTSTR valueName, DWORD def)
{
	HKEY hkey;
	DWORD ret;

	if (!RegOpenKey(root, path, &hkey)) {
		DWORD len = sizeof(ret);

		if (RegQueryValueEx(hkey, valueName, 0, NULL, (LPBYTE)&ret, &len))
			ret = def;

		RegCloseKey(hkey);

		return ret;
	} else
		return def;
}


BOOL RegSetDWORDValue(HKEY root, LPCTSTR path, LPCTSTR valueName, DWORD value)
{
	HKEY hkey;
	BOOL ret = FALSE;

	if (!RegOpenKey(root, path, &hkey)) {
		ret = RegSetValueEx(hkey, valueName, 0, REG_DWORD, (LPBYTE)&value, sizeof(value));

		RegCloseKey(hkey);
	}

	return ret;
}


BOOL exists_path(LPCTSTR path)
{
	WIN32_FIND_DATA fd;

	HANDLE hfind = FindFirstFile(path, &fd);

	if (hfind != INVALID_HANDLE_VALUE) {
		FindClose(hfind);

		return TRUE;
	} else
		return FALSE;
}


bool SplitFileSysURL(LPCTSTR url, String& dir_out, String& fname_out)
{
	if (!_tcsnicmp(url, TEXT("file://"), 7)) {
		url += 7;

		 // remove third slash in front of drive characters
		if (*url == '/')
			++url;
	}

	if (exists_path(url)) {
		TCHAR path[_MAX_PATH];

		 // convert slashes to back slashes
		GetFullPathName(url, COUNTOF(path), path, NULL);

		if (GetFileAttributes(path) & FILE_ATTRIBUTE_DIRECTORY)
			fname_out.erase();
		else {
			TCHAR drv[_MAX_DRIVE], dir[_MAX_DIR], fname[_MAX_FNAME], ext[_MAX_EXT];

			_tsplitpath_s(path, drv, COUNTOF(drv), dir, COUNTOF(dir), fname, COUNTOF(fname), ext, COUNTOF(ext));
			_stprintf(path, TEXT("%s%s"), drv, dir);

			fname_out.printf(TEXT("%s%s"), fname, ext);
		}

		dir_out = path;

		return true;
	} else
		return false;
}
