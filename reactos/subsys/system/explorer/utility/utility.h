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
 // utility.h
 //
 // Martin Fuchs, 23.07.2003
 //


 // standard windows headers
#define WIN32_LEAN_AND_MEAN
#define WIN32_EXTRA_LEAN
#include <windows.h>

 // Unicode support
#ifdef UNICODE
#define	_UNICODE
#endif
#include <tchar.h>

#include <windowsx.h>	// for SelectBrush(), ListBox_SetSel(), SubclassWindow(), ...
#include <commctrl.h>

#include <malloc.h>		// for alloca()
#include <assert.h>
#include <time.h>


#ifdef __cplusplus

 // STL headers for strings and streams
#include <string>
#include <iostream>
using namespace std;

#if defined(_MSC_VER) && !defined(_NO_COMUTIL)

 // COM utility headers
#include <comdef.h>
using namespace _com_util;

#endif	// _MSC_VER


struct CommonControlInit
{
	CommonControlInit(DWORD flags=ICC_LISTVIEW_CLASSES)
	{
		INITCOMMONCONTROLSEX icc = {sizeof(INITCOMMONCONTROLSEX), flags};

		InitCommonControlsEx(&icc);
	}
};


struct WaitCursor
{
	WaitCursor()
	{
		_old_cursor = SetCursor(LoadCursor(0, IDC_WAIT));
	}

	~WaitCursor()
	{
		SetCursor(_old_cursor);
	}

protected:
	HCURSOR	_old_cursor;
};


struct FullScreenParameters {
	FullScreenParameters()
	 :	_mode(FALSE)
	{
	}

	BOOL	_mode;
	RECT	_orgPos;
	BOOL	_wasZoomed;
};


#endif // __cplusplus


#ifdef __cplusplus
extern "C" {
#endif


#ifdef _MSC_VER
#define	LONGLONGARG TEXT("I64")
#else
#define	LONGLONGARG TEXT("L")
#endif


#ifndef _tcsrchr
#ifdef UNICODE
#define	_tcsrchr wcsrchr
#else
#define	_tcsrchr strrchr
#endif
#endif

#ifndef _stprintf
#ifdef UNICODE
#define	_stprintf wcsrintf
#else
#define	_stprintf sprintf
#endif
#endif


 // display 
extern void display_error(HWND hwnd, DWORD error);

 // convert time_t to WIN32 FILETIME
extern BOOL time_to_filetime(const time_t* t, FILETIME* ftime);

 // search for windows of a specific classname
extern int find_window_class(LPCTSTR classname);

 // launch a program or document file
extern BOOL launch_file(HWND hwnd, LPCTSTR cmd, UINT nCmdShow);
#ifdef UNICODE
extern BOOL launch_fileA(HWND hwnd, LPSTR cmd, UINT nCmdShow);
#else
#define	launch_fileA launch_file
#endif


#ifdef __cplusplus
} // extern "C"
#endif

