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
#include <stdlib.h>		// for _MAX_DIR, ...
#include <stdio.h>		// for sprintf()
#include <time.h>

#ifndef _MAX_PATH
#define _MAX_DRIVE	3
#define _MAX_FNAME	256
#define _MAX_DIR	_MAX_FNAME
#define _MAX_EXT	_MAX_FNAME
#define _MAX_PATH	260
#endif


#ifdef __cplusplus

#ifdef _MSC_VER
#pragma warning(disable: 4786)	// disable warnings about too long debug information symbols
#endif

 // STL headers for strings and streams
#include <string>
#include <iostream>
using namespace std;

#if _MSC_VER>=1300	// VS.Net
#define _NO_COMUTIL	//@@
#endif

#if defined(_MSC_VER) && !defined(_NO_COMUTIL)

 // COM utility headers
#include <comdef.h>
using namespace _com_util;

#endif	// _MSC_VER && !_NO_COMUTIL


#define	for if (0) {} else for


#define	BUFFER_LEN				1024


struct CommonControlInit
{
	CommonControlInit(DWORD flags=ICC_LISTVIEW_CLASSES|ICC_TREEVIEW_CLASSES|ICC_BAR_CLASSES|ICC_PROGRESS_CLASS|ICC_COOL_CLASSES)
	{
		INITCOMMONCONTROLSEX icc = {sizeof(INITCOMMONCONTROLSEX), flags};

		InitCommonControlsEx(&icc);
	}
};


 /// wait cursor

struct WaitCursor	///@todo integrate with WM_SETCURSOR to enable multithreaded background tasks as program launching
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


struct WindowHandle
{
	WindowHandle(HWND hwnd=0)
	 :	_hwnd(hwnd) {}

	operator HWND() const {return _hwnd;}
	HWND* operator&() {return &_hwnd;}

protected:
	HWND	_hwnd;
};


struct HiddenWindow : public WindowHandle
{
	HiddenWindow(HWND hwnd)
	 :	WindowHandle(hwnd)
	{
		SetWindowPos(hwnd, 0, 0, 0, 0, 0, SWP_HIDEWINDOW|SWP_NOREDRAW|SWP_NOMOVE|SWP_NOSIZE|SWP_NOZORDER);
	}

	~HiddenWindow()
	{
		SetWindowPos(_hwnd, 0, 0, 0, 0, 0, SWP_SHOWWINDOW|SWP_NOMOVE|SWP_NOSIZE|SWP_NOZORDER);
	}
};


 /// critical section wrapper

struct CritSect : public CRITICAL_SECTION
{
	CritSect()
	{
		InitializeCriticalSection(this);
	}

	~CritSect()
	{
		DeleteCriticalSection(this);
	}
};


 /// Lock protects a code section utilizing a critical section

struct Lock
{
	Lock(CritSect& crit_sect)
	 :	_crit_sect(crit_sect)
	{
		EnterCriticalSection(&crit_sect);
	}

	~Lock()
	{
		LeaveCriticalSection(&_crit_sect);
	}

protected:
	CritSect&	_crit_sect;
};


 /// Thread base class

struct Thread
{
	Thread()
	 :	_alive(false)
	{
		_hThread = INVALID_HANDLE_VALUE;
	}

	virtual ~Thread()
	{
		Stop();

		CloseHandle(_hThread);
	}

	void Start()
	{
		if (!_alive) {
			_alive = true;
			_hThread = CreateThread(NULL, 0, ThreadProc, this, 0, NULL);
		}
	}

	void Stop()
	{
		if (_alive) {
			{
			Lock lock(_crit_sect);
			_alive = false;
			}

			 // wait for finishing
			WaitForSingleObject(_hThread, INFINITE);
		}
	}

	virtual int Run() = 0;

	bool	is_alive() const {return _alive;}

	CritSect _crit_sect;

protected:
	static DWORD WINAPI ThreadProc(void* para);

	HANDLE	_hThread;
	bool	_alive;
};


 // window utilities

struct ClientRect : public RECT
{
	ClientRect(HWND hwnd)
	{
		GetClientRect(hwnd, this);
	}

	operator LPRECT() {return this;}

	POINT& pos() {return *(LPPOINT)this;}
};

struct WindowRect : public RECT
{
	WindowRect(HWND hwnd)
	{
		GetWindowRect(hwnd, this);
	}

	operator LPRECT() {return this;}

	POINT& pos() {return *(LPPOINT)this;}
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

	operator LPPOINT() {return this;}
};


inline void ClientToScreen(HWND hwnd, RECT* prect)
 {::ClientToScreen(hwnd,(LPPOINT)&prect->left); ::ClientToScreen(hwnd,(LPPOINT)&prect->right);}

inline void ScreenToClient(HWND hwnd, RECT* prect)
 {::ScreenToClient(hwnd,(LPPOINT)&prect->left); ::ScreenToClient(hwnd,(LPPOINT)&prect->right);}


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

	~PaintCanvas()
	{
		EndPaint(_hwnd, this);
	}

	operator HDC() const {return hdc;}

protected:
	HWND	_hwnd;
};

struct Canvas
{
	Canvas(HDC hdc) : _hdc(hdc) {}

	operator HDC() {return _hdc;}

protected:
	HDC _hdc;
};

struct WindowCanvas : public Canvas
{
	WindowCanvas(HWND hwnd)
	 :	Canvas(GetDC(hwnd)), _hwnd(hwnd) {}

	~WindowCanvas() {ReleaseDC(_hwnd, _hdc);}

protected:
	HWND	_hwnd;
};


 // double buffering classes

struct MemCanvas : public Canvas
{
	MemCanvas(HDC hdc=0)
	 :	Canvas(CreateCompatibleDC(hdc)) {assert(_hdc);}

	~MemCanvas() {DeleteDC(_hdc);}
};

struct SelectedBitmap
{
	SelectedBitmap(HDC hdc, HBITMAP hbmp)
	 :	_hdc(hdc), _old_hbmp(SelectBitmap(hdc, hbmp)) {}

	~SelectedBitmap() {DeleteObject(SelectBitmap(_hdc, _old_hbmp));}

protected:
	HDC		_hdc;
	HBITMAP	_old_hbmp;
};

struct BufferCanvas : public MemCanvas
{
	BufferCanvas(HDC hdc, int x, int y, int w, int h)
	 :	MemCanvas(hdc), _hdctarg(hdc),
		_x(x), _y(y), _w(w), _h(h),
		_bmp(_hdc, CreateCompatibleBitmap(hdc, w, h)) {}

	BufferCanvas(HDC hdc, const RECT& rect)
	 :	MemCanvas(hdc), _hdctarg(hdc),
		_x(rect.left), _y(rect.top), _w(rect.right-rect.left), _h(rect.bottom-rect.top),
		_bmp(_hdc, CreateCompatibleBitmap(hdc, _w, _h)) {}

protected:
	HDC 	_hdctarg;
	int 	_x, _y, _w, _h;
	SelectedBitmap _bmp;
};

struct BufferedCanvas : public BufferCanvas
{
	BufferedCanvas(HDC hdc, int x, int y, int w, int h, DWORD mode=SRCCOPY)
	 :	BufferCanvas(hdc, x, y, w, h), _mode(mode) {}

	BufferedCanvas(HDC hdc, const RECT& rect, DWORD mode=SRCCOPY)
	 :	BufferCanvas(hdc, rect), _mode(mode) {}

	~BufferedCanvas() {BitBlt(_hdctarg, _x, _y, _w, _h, _hdc, 0, 0, _mode);}

	DWORD	_mode;
};

struct BufferedPaintCanvas : public PaintCanvas, public BufferedCanvas
{
	BufferedPaintCanvas(HWND hwnd)
	 :	PaintCanvas(hwnd),
		BufferedCanvas(PAINTSTRUCT::hdc, 0, 0, rcPaint.right, rcPaint.bottom)
	{
	}

	operator HDC() {return BufferedCanvas::_hdc;}
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

struct FontSelection
{
	FontSelection(HDC hdc, HFONT hFont)
	 : _hdc(hdc), _old_hFont(SelectFont(hdc, hFont)) {}

	~FontSelection() {SelectFont(_hdc, _old_hFont);}

protected:
	HDC		_hdc;
	HFONT	_old_hFont;
};

struct BitmapSelection
{
	BitmapSelection(HDC hdc, HBITMAP hBmp)
	 : _hdc(hdc), _old_hBmp(SelectBitmap(hdc, hBmp)) {}

	~BitmapSelection() {SelectBitmap(_hdc, _old_hBmp);}

protected:
	HDC		_hdc;
	HBITMAP	_old_hBmp;
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
	String(const super& other) : super(other) {}
	String(const String& other) : super(other) {}

	String& operator=(LPCTSTR s) {assign(s); return *this;}
	String& operator=(const super& s) {assign(s); return *this;}

	operator LPCTSTR() const {return c_str();}
};


 /// link dynamicly to functions by using GetModuleHandle() and GetProcAddress()
template<typename FCT> struct DynamicFct
{
	DynamicFct(LPCTSTR moduleName, UINT ordinal)
	{
		HMODULE hModule = GetModuleHandle(moduleName);

		_fct = (FCT) GetProcAddress(hModule, (LPCSTR)ordinal);
	}

	DynamicFct(LPCTSTR moduleName, LPCSTR name)
	{
		HMODULE hModule = GetModuleHandle(moduleName);

		_fct = (FCT) GetProcAddress(hModule, name);
	}

	FCT operator*() const {return _fct;}
	operator bool() const {return _fct? true: false;}

protected:
	FCT	_fct;
};


 /// link dynamicly to functions by using LoadLibrary() and GetProcAddress()
template<typename FCT> struct DynamicLoadLibFct
{
	DynamicLoadLibFct(LPCTSTR moduleName, UINT ordinal)
	{
		_hModule = LoadLibrary(moduleName);

		_fct = (FCT) GetProcAddress(_hModule, (LPCSTR)ordinal);
	}

	DynamicLoadLibFct(LPCTSTR moduleName, LPCSTR name)
	{
		_hModule = LoadLibrary(moduleName);

		_fct = (FCT) GetProcAddress(_hModule, name);
	}

	~DynamicLoadLibFct()
	{
		FreeLibrary(_hModule);
	}

	FCT operator*() const {return _fct;}
	operator bool() const {return _fct? true: false;}

protected:
	HMODULE _hModule;
	FCT	_fct;
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
#define	_stprintf wcsprintf
#else
#define	_stprintf sprintf
#endif
#endif


#ifdef __WINE__
#ifdef UNICODE
extern void _wsplitpath(const WCHAR* path, WCHAR* drv, WCHAR* dir, WCHAR* name, WCHAR* ext);
#else
extern void _splitpath(const CHAR* path, CHAR* drv, CHAR* dir, CHAR* name, CHAR* ext);
#endif
#endif

#ifndef FILE_ATTRIBUTE_NOT_CONTENT_INDEXED
#define FILE_ATTRIBUTE_ENCRYPTED            0x00000040
#define FILE_ATTRIBUTE_SPARSE_FILE          0x00000200
#define FILE_ATTRIBUTE_REPARSE_POINT        0x00000400
#define FILE_ATTRIBUTE_NOT_CONTENT_INDEXED  0x00002000
#endif


#define	SetDlgCtrlID(hwnd, id) SetWindowLong(hwnd, GWL_ID, id)
#define	SetWindowStyle(hwnd, val) (DWORD)SetWindowLong(hwnd, GWL_STYLE, val)
#define	SetWindowExStyle(h, val) (DWORD)SetWindowLong(hwnd, GWL_EXSTYLE, val)
#define	Window_SetIcon(hwnd, type, hicon) (HICON)SendMessage(hwnd, WM_SETICON, type, (LPARAM)(hicon))



 // center window in respect to its parent window
extern void CenterWindow(HWND hwnd);

 // move window into visibility
extern void MoveVisible(HWND hwnd);

 // display error message
extern void display_error(HWND hwnd, DWORD error);

 // convert time_t to WIN32 FILETIME
extern BOOL time_to_filetime(const time_t* t, FILETIME* ftime);

 // search for windows of a specific classname
extern int find_window_class(LPCTSTR classname);

 // create a bitmap from an icon
extern HBITMAP create_bitmap_from_icon(HICON hIcon, HBRUSH hbrush_bkgnd, HDC hdc_wnd);

 // launch a program or document file
extern BOOL launch_file(HWND hwnd, LPCTSTR cmd, UINT nCmdShow);
#ifdef UNICODE
extern BOOL launch_fileA(HWND hwnd, LPSTR cmd, UINT nCmdShow);
#else
#define	launch_fileA launch_file
#endif

 // call an DLL export like rundll32
BOOL RunDLL(HWND hwnd, LPCTSTR dllname, LPCSTR procname, LPCTSTR cmdline, UINT nCmdShow);


#ifdef __cplusplus
} // extern "C"
#endif

