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
 // traynotify.cpp
 //
 // Martin Fuchs, 22.08.2003
 //


#include "../utility/utility.h"

#include "../explorer.h"
#include "../globals.h"

#include "traynotify.h"


NotifyIconIndex::NotifyIconIndex(NOTIFYICONDATA* pnid)
{
	_hWnd = pnid->hWnd;
	_uID = pnid->uID;

	 // special handling for windows task manager
	if ((int)_uID < 0)
		_uID = 0;
}

NotifyIconIndex::NotifyIconIndex()
{
	_hWnd = 0;
	_uID = 0;
}


NotifyInfo::NotifyInfo()
{
	_idx = -1;
	_hIcon = 0;
	_dwState = 0;
	_uCallbackMessage = 0;
}

NotifyInfo& NotifyInfo::operator=(NOTIFYICONDATA* pnid)
{
	_hWnd = pnid->hWnd;
	_uID = pnid->uID;

	if (pnid->uFlags & NIF_MESSAGE)
		_uCallbackMessage = pnid->uCallbackMessage;

	if (pnid->uFlags & NIF_ICON)
		_hIcon = pnid->hIcon;

#ifdef NIF_STATE	// currently (as of 21.08.2003) missing in MinGW headers
	if (pnid->uFlags & NIF_STATE)
		_dwState = (_dwState&~pnid->dwStateMask) | (pnid->dwState&pnid->dwStateMask);
#endif

	///@todo store and display tool tip texts

	return *this;
}


NotifyArea::NotifyArea(HWND hwnd)
 :	super(hwnd)
{
	_next_idx = 0;
	_clock_width = 0;
}

LRESULT NotifyArea::Init(LPCREATESTRUCT pcs)
{
	if (super::Init(pcs))
		return 1;

#ifndef __MINGW32__	// SHRestricted() missing in MinGW (as of 29.10.2003)
	if (!g_Globals._SHRestricted || !SHRestricted(REST_HIDECLOCK))
#endif
	{
		 // create clock window
		_hwndClock = ClockWindow::Create(_hwnd);

		if (_hwndClock) {
			ClientRect clock_size(_hwndClock);
			_clock_width = clock_size.right;
		}

		SetTimer(_hwnd, 0, 1000, NULL);
	}

	return 0;
}

NotifyArea::~NotifyArea()
{
	KillTimer(_hwnd, 0);
}

HWND NotifyArea::Create(HWND hwndParent)
{
	ClientRect clnt(hwndParent);

	return Window::Create(WINDOW_CREATOR(NotifyArea), WS_EX_STATICEDGE,
							BtnWindowClass(CLASSNAME_TRAYNOTIFY,CS_DBLCLKS), TITLE_TRAYNOTIFY, WS_CHILD|WS_VISIBLE,
							clnt.right-(NOTIFYAREA_WIDTH_DEF+1), 1, NOTIFYAREA_WIDTH_DEF, clnt.bottom-2, hwndParent);
}

LRESULT NotifyArea::WndProc(UINT nmsg, WPARAM wparam, LPARAM lparam)
{
	switch(nmsg) {
	  case WM_PAINT:
		Paint();
		break;

	  case WM_TIMER: {
		TimerTick();

		ClockWindow* clock_window = GET_WINDOW(ClockWindow, _hwndClock);

		if (clock_window)
			clock_window->TimerTick();
		break;}

	  case WM_SIZE: {
		int cx = LOWORD(lparam);
		SetWindowPos(_hwndClock, 0, cx-_clock_width, 0, 0, 0, SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE);
		break;}

	  case PM_GET_WIDTH:
		return _sorted_icons.size()*NOTIFYICON_DIST + NOTIFYAREA_SPACE + _clock_width;

	  case WM_CONTEXTMENU:
		break;	// don't let WM_CONTEXTMENU go through to the desktop bar

	  default:
		if (nmsg>=WM_MOUSEFIRST && nmsg<=WM_MOUSELAST) {
			 // close startup menu and other popup menus
			 // This functionality is missing in MS Windows.
			if (nmsg==WM_LBUTTONDOWN || nmsg==WM_MBUTTONDOWN || nmsg==WM_RBUTTONDOWN
#ifdef WM_XBUTTONDOWN
				|| nmsg==WM_XBUTTONDOWN
#endif
				)
				CancelModes();

			NotifyIconSet::iterator found = IconHitTest(Point(lparam));

			if (found != _sorted_icons.end()) {
				NotifyInfo& entry = const_cast<NotifyInfo&>(*found);	// Why does GCC 3.3 need this additional const_cast ?!

				 // Notify the message if the owner is still alive
				if (IsWindow(entry._hWnd)) {
					 // allow SetForegroundWindow() in Client Process
					DWORD processId;
					GetWindowThreadProcessId(entry._hWnd, &processId);

					 // bind dynamicaly to AllowSetForegroundWindow() to be compatible to WIN98
					static DynamicFct<BOOL(WINAPI*)(DWORD dwProcessId)> AllowSetForegroundWindow(TEXT("USER32"), "AllowSetForegroundWindow");

					if (AllowSetForegroundWindow)
						(*AllowSetForegroundWindow)(processId);

					PostMessage(entry._hWnd, entry._uCallbackMessage, entry._uID, nmsg);
				}
				else if (_icon_map.erase(entry))	// delete icons without valid owner window
					Refresh();
			}
		}

		return super::WndProc(nmsg, wparam, lparam);
	}

	return 0;
}

void NotifyArea::CancelModes()
{
	PostMessage(HWND_BROADCAST, WM_CANCELMODE, 0, 0);

	for(NotifyIconSet::const_iterator it=_sorted_icons.begin(); it!=_sorted_icons.end(); ++it)
		PostMessage(it->_hWnd, WM_CANCELMODE, 0, 0);
}

LRESULT NotifyArea::ProcessTrayNotification(int notify_code, NOTIFYICONDATA* pnid)
{
	switch(notify_code) {
	  case NIM_ADD:
	  case NIM_MODIFY:
		if ((int)pnid->uID >= 0) {	///@todo fix for windows task manager
			NotifyInfo& entry = _icon_map[pnid] = pnid;

			 // a new entry?
			if (entry._idx == -1)
				entry._idx = ++_next_idx;

			Refresh();	///@todo call only if really changes occurred
		}
		break;

	  case NIM_DELETE:
		if (_icon_map.erase(pnid))
			Refresh();
		break;

#if NOTIFYICON_VERSION>=3	// currently (as of 21.08.2003) missing in MinGW headers
	  case NIM_SETFOCUS:
		break;

	  case NIM_SETVERSION:
		break;
#endif
	}

	return 0;
}

void NotifyArea::Refresh()
{
	_sorted_icons.clear();

	 // sort icon infos by display index
	for(NotifyIconMap::const_iterator it=_icon_map.begin(); it!=_icon_map.end(); ++it) {
		const NotifyInfo& entry = it->second;

#ifdef NIF_STATE	// currently (as of 21.08.2003) missing in MinGW headers
		if (!(entry._dwState & NIS_HIDDEN))
#endif
			_sorted_icons.insert(entry);
	}

	SendMessage(GetParent(_hwnd), PM_RESIZE_CHILDREN, 0, 0);

	InvalidateRect(_hwnd, NULL, FALSE);	// refresh icon display
	UpdateWindow(_hwnd);
}

void NotifyArea::Paint()
{
	BufferedPaintCanvas canvas(_hwnd);

	 // first fill with the background color
	FillRect(canvas, &canvas.rcPaint, GetSysColorBrush(COLOR_BTNFACE));

	 // draw icons
	int x = 2;
	int y = 2;

	for(NotifyIconSet::const_iterator it=_sorted_icons.begin(); it!=_sorted_icons.end(); ++it) {
		DrawIconEx(canvas, x, y, it->_hIcon, 16, 16, 0, 0, DI_NORMAL);

		x += NOTIFYICON_DIST;
	}
}

void NotifyArea::TimerTick()
{
	bool do_refresh = false;

	 // Look for task icons without valid owner window.
	 // This is an advanced feature, which is missing in MS Windows.
	for(NotifyIconSet::const_iterator it=_sorted_icons.begin(); it!=_sorted_icons.end(); ++it) {
		const NotifyInfo& entry = *it;

		if (!IsWindow(entry._hWnd))
			if (_icon_map.erase(entry))	// delete icons without valid owner window
				++do_refresh;
	}

	if (do_refresh)
		Refresh();
}

 /// search for a icon at a given client coordinate position
NotifyIconSet::iterator NotifyArea::IconHitTest(const POINT& pos)
{
	if (pos.y<2 || pos.y>=2+16)
		return _sorted_icons.end();

	NotifyIconSet::iterator it = _sorted_icons.begin();

	int x = 2;

	for(; it!=_sorted_icons.end(); ++it) {
		//NotifyInfo& entry = const_cast<NotifyInfo&>(*it);	// Why does GCC 3.3 need this additional const_cast ?!

		if (pos.x>=x && pos.x<x+16)
			break;

		x += NOTIFYICON_DIST;
	}

	return it;
}


ClockWindow::ClockWindow(HWND hwnd)
 :	super(hwnd),
	_tooltip(hwnd)
{
	*_time = TEXT('\0');
	FormatTime();

	_tooltip.add(_hwnd, _hwnd);
}

HWND ClockWindow::Create(HWND hwndParent)
{
	ClientRect clnt(hwndParent);

	WindowCanvas canvas(hwndParent);
	FontSelection font(canvas, GetStockFont(DEFAULT_GUI_FONT));

	RECT rect = {0, 0, 0, 0};
	TCHAR buffer[16];

	if (!GetTimeFormat(LOCALE_USER_DEFAULT, TIME_NOSECONDS, NULL, NULL, buffer, sizeof(buffer)/sizeof(TCHAR)))
		_tcscpy(buffer, TEXT("00:00"));

	DrawText(canvas, buffer, -1, &rect, DT_SINGLELINE|DT_NOPREFIX|DT_CALCRECT);
	int clockwindowWidth = rect.right-rect.left + 4;

	return Window::Create(WINDOW_CREATOR(ClockWindow), 0,
							BtnWindowClass(CLASSNAME_CLOCKWINDOW,CS_DBLCLKS), NULL, WS_CHILD|WS_VISIBLE,
							clnt.right-(clockwindowWidth), 1, clockwindowWidth, clnt.bottom-2, hwndParent);
}

LRESULT ClockWindow::WndProc(UINT nmsg, WPARAM wparam, LPARAM lparam)
{
	switch(nmsg) {
	  case WM_PAINT:
		Paint();
		break;

	  case WM_LBUTTONDBLCLK:
		//launch_file(_hwnd, TEXT("timedate.cpl"), SW_SHOWNORMAL);	// This would be enough, but we want the fastest solution.
		//launch_file(_hwnd, TEXT("rundll32.exe /d shell32.dll,Control_RunDLL timedate.cpl"), SW_SHOWNORMAL);
		RunDLL(_hwnd, TEXT("shell32"), "Control_RunDLL", TEXT("timedate.cpl"), SW_SHOWNORMAL);
		break;

	  default:
		return super::WndProc(nmsg, wparam, lparam);
	}

	return 0;
}

int ClockWindow::Notify(int id, NMHDR* pnmh)
{
	if (pnmh->code == TTN_GETDISPINFO) {
		LPNMTTDISPINFO pdi = (LPNMTTDISPINFO)pnmh;

		SYSTEMTIME systime;
		TCHAR buffer[64];

		GetLocalTime(&systime);
		GetDateFormat(LOCALE_USER_DEFAULT, DATE_LONGDATE, &systime, NULL, buffer, 64);

		_tcscpy(pdi->szText, buffer);
	}

	return 0;
}

void ClockWindow::TimerTick()
{
	if (FormatTime())
		InvalidateRect(_hwnd, NULL, TRUE);	// refresh displayed time
}

bool ClockWindow::FormatTime()
{
	TCHAR buffer[16];

	if (GetTimeFormat(LOCALE_USER_DEFAULT, TIME_NOSECONDS, NULL, NULL, buffer, sizeof(buffer)/sizeof(TCHAR)))
		if (_tcscmp(buffer, _time)) {
			_tcscpy(_time, buffer);
			return true;	// The text to display has changed.
		}

	return false;	// no change
}

void ClockWindow::Paint()
{
	PaintCanvas canvas(_hwnd);

	BkMode bkmode(canvas, TRANSPARENT);
	FontSelection font(canvas, GetStockFont(ANSI_VAR_FONT));

	DrawText(canvas, _time, -1, ClientRect(_hwnd), DT_SINGLELINE|DT_VCENTER|DT_NOPREFIX);
}
