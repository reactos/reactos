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


#ifndef BTNS_BUTTON
#define BTNS_BUTTON TBSTYLE_BUTTON //TODO: should be in mingw headers
#define BTNS_SEP TBSTYLE_SEP
#endif


#ifdef __cplusplus

#ifdef _MSC_VER
#pragma warning(disable: 4786)	// disable warnings about too long debug information symbols
#endif

 // STL headers for strings and streams
#include <string>
#include <iostream>
using namespace std;

#if defined(_MSC_VER) && !defined(_NO_COMUTIL)

 // COM utility headers
#include <comdef.h>
using namespace _com_util;

#endif	// _MSC_VER && !_NO_COMUTIL


#define	for if (0) {} else for


#define	BUFFER_LEN				1024


struct CommonControlInit
{
	CommonControlInit(DWORD flags=ICC_LISTVIEW_CLASSES)
	{
		INITCOMMONCONTROLSEX icc = {sizeof(INITCOMMONCONTROLSEX), flags};

		InitCommonControlsEx(&icc);
	}
};


 /// wait cursor

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


 // window utilities

struct ClientRect : public RECT
{
	ClientRect(HWND hwnd)
	{
		GetClientRect(hwnd, this);
	}
};

struct WindowRect : public RECT
{
	WindowRect(HWND hwnd)
	{
		GetWindowRect(hwnd, this);
	}
};

struct Point : public POINT
{
	Point(LONG x_, LONG y_)
	{
		x = x_;
		y = y_;
	}

	 // constructor for being used in processing WM_MOUSEMOVE, WM_LBUTTONDOWN, ... messages
	Point(LPARAM lparam)
	{
		x = GET_X_LPARAM(lparam);
		y = GET_Y_LPARAM(lparam);
	}
};


struct FullScreenParameters
{
	FullScreenParameters()
	 :	_mode(FALSE)
	{
	}

	BOOL	_mode;
	RECT	_orgPos;
	BOOL	_wasZoomed;
};


 // drawing utilities

struct PaintCanvas : public PAINTSTRUCT
{
	PaintCanvas(HWND hwnd)
	 :	_hwnd(hwnd)
	{
		BeginPaint(hwnd, this);
	}

	PaintCanvas()
	{
		EndPaint(_hwnd, this);
	}

	operator HDC() const {return hdc;}

protected:
	HWND	_hwnd;
};

struct TextColor
{
	TextColor(HDC hdc, COLORREF color)
	 : _hdc(hdc), _old_color(SetTextColor(hdc, color)) {}

	~TextColor() {SetTextColor(_hdc, _old_color);}

protected:
	HDC		 _hdc;
	COLORREF _old_color;
};

struct BkMode
{
	BkMode(HDC hdc, int bkmode)
	 : _hdc(hdc), _old_bkmode(SetBkMode(hdc, bkmode)) {}

	~BkMode() {SetBkMode(_hdc, _old_bkmode);}

protected:
	HDC		 _hdc;
	COLORREF _old_bkmode;
};

struct SelectedFont
{
	SelectedFont(HDC hdc, HFONT hFont)
	 : _hdc(hdc), _old_hFont(SelectFont(hdc, hFont)) {}

	~SelectedFont() {SelectFont(_hdc, _old_hFont);}

protected:
	HDC		_hdc;
	HFONT	_old_hFont;
};


struct String
#ifdef UNICODE
 : public wstring
#else
 : public string
#endif
{
#ifdef UNICODE
	typedef wstring super;
#else
	typedef string super;
#endif

	String() {}
	String(LPCTSTR s) : super(s) {}
	String(const String& other) : super(other) {}

	String& operator=(LPCTSTR s) {assign(s); return *this;}
	operator LPCTSTR() const {return c_str();}
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


#define	SetDlgCtrlID(hwnd, id) SetWindowLong(hwnd, GWL_ID, id)


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

