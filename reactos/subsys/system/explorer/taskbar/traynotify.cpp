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

//#include "taskbar.h"	// for _desktop_bar
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

	return *this;
}


NotifyArea::NotifyArea(HWND hwnd)
 :	super(hwnd)
{
//	_desktop_bar = NULL;
	_next_idx = 0;
}

LRESULT NotifyArea::Init(LPCREATESTRUCT pcs)
{
	if (super::Init(pcs))
		return 1;

	ClientRect clnt(_hwnd);

	 // create clock window
	_hwndClock = Window::Create(WINDOW_CREATOR(ClockWindow), 0,
							BtnWindowClass(CLASSNAME_CLOCKWINDOW,CS_DBLCLKS), NULL, WS_CHILD|WS_VISIBLE,
							clnt.right-(CLOCKWINDOW_WIDTH+1), 1, CLOCKWINDOW_WIDTH, clnt.bottom-2, _hwnd);

	SetTimer(_hwnd, 0, 1000, NULL);

	return 0;
}

NotifyArea::~NotifyArea()
{
	KillTimer(_hwnd, 0);
}

LRESULT NotifyArea::WndProc(UINT nmsg, WPARAM wparam, LPARAM lparam)
{
	switch(nmsg) {
	  case WM_PAINT:
		Paint();
		break;

	  case WM_TIMER: {
		Tick();

		ClockWindow* clock_window = static_cast<ClockWindow*>(get_window(_hwndClock));

		if (clock_window)
			clock_window->Tick();
		break;}

	  default:
		if (nmsg>=WM_MOUSEFIRST && nmsg<=WM_MOUSELAST) {
			NotifyIconSet::iterator found = IconHitTest(Point(lparam));

			if (found != _sorted_icons.end()) {
				NotifyInfo& entry = const_cast<NotifyInfo&>(*found);	// Why does GCC 3.3 need this additional const_cast ?!

				 // Notify the message the the owner if it's still alive
				if (IsWindow(entry._hWnd))
					PostMessage(entry._hWnd, entry._uCallbackMessage, entry._uID, nmsg);
				else if (_icon_map.erase(entry))	// delete icons without valid owner window
					Refresh();
			}
		}

		return super::WndProc(nmsg, wparam, lparam);
	}

	return 0;
}

LRESULT NotifyArea::ProcessTrayNotification(int notify_code, NOTIFYICONDATA* pnid)
{
	switch(notify_code) {
	  case NIM_ADD:
	  case NIM_MODIFY:
		if ((int)pnid->uID >= 0) {	//TODO: fix for windows task manager
			NotifyInfo& entry = _icon_map[pnid] = pnid;

			 // a new entry?
			if (entry._idx == -1)
				entry._idx = ++_next_idx;

			Refresh();
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
	for(NotifyIconMap::const_iterator it=_icon_map.begin(); it!=_icon_map.end(); ++it)
		_sorted_icons.insert(it->second);

	InvalidateRect(_hwnd, NULL, TRUE);	// refresh icon display
	UpdateWindow(_hwnd);
}

void NotifyArea::Paint()
{
	PaintCanvas canvas(_hwnd);

	 // draw icons
	int x = 2;
	int y = 2;

	for(NotifyIconSet::const_iterator it=_sorted_icons.begin(); it!=_sorted_icons.end(); ++it)
	{
		const NotifyInfo& entry = *it;

#ifdef NIF_STATE	// currently (as of 21.08.2003) missing in MinGW headers
		if (!(entry._dwState & NIS_HIDDEN))
#endif
		{
			DrawIconEx(canvas, x, y, entry._hIcon, 16, 16, 0, 0, DI_NORMAL);
			x += 20;
		}
	}
}

void NotifyArea::Tick()
{
	bool do_refresh = false;

	 // Look for task icons without valid owner window.
	 // This is an advanced feature, which is missing in MS Windows.
	for(NotifyIconSet::const_iterator it=_sorted_icons.begin(); it!=_sorted_icons.end(); ++it)
	{
		const NotifyInfo& entry = *it;

#ifdef NIF_STATE	// currently (as of 21.08.2003) missing in MinGW headers
		if (!(entry._dwState & NIS_HIDDEN))
#endif
		{
			if (!IsWindow(entry._hWnd))
				if (_icon_map.erase(entry))	// delete icons without valid owner window
					++do_refresh;
		}
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

	for(; it!=_sorted_icons.end(); ++it)
	{
		NotifyInfo& entry = const_cast<NotifyInfo&>(*it);	// Why does GCC 3.3 need this additional const_cast ?!

#ifdef NIF_STATE	// currently (as of 21.08.2003) missing in MinGW headers
		if (!(entry._dwState & NIS_HIDDEN))
#endif
		{
			if (pos.x>=x && pos.x<x+16)
				break;

			x += 20;
		}
	}

	return it;
}


ClockWindow::ClockWindow(HWND hwnd)
 :	super(hwnd)
{
	*_time = _T('\0');
	FormatTime();
}

LRESULT ClockWindow::WndProc(UINT nmsg, WPARAM wparam, LPARAM lparam)
{
	switch(nmsg) {
	  case WM_PAINT:
		Paint();
		break;

	  default:
		return super::WndProc(nmsg, wparam, lparam);
	}

	return 0;
}

void ClockWindow::Tick()
{
	if (FormatTime())
		InvalidateRect(_hwnd, NULL, TRUE);	// refresh displayed time
}

bool ClockWindow::FormatTime()
{
	SYSTEMTIME systime;
	TCHAR buffer[16];

	GetLocalTime(&systime);

	//_stprintf(buffer, TEXT("%02d:%02d:%02d"), systime.wHour, systime.wMinute, systime.wSecond);
	_stprintf(buffer, TEXT("%02d:%02d"), systime.wHour, systime.wMinute);

	if (_tcscmp(buffer, _time)) {
		_tcscpy(_time, buffer);
		return true;
	}

	return false;	// no change
}

void ClockWindow::Paint()
{
	PaintCanvas canvas(_hwnd);

	BkMode bkmode(canvas, TRANSPARENT);
	SelectedFont font(canvas, GetStockFont(ANSI_VAR_FONT));

	DrawText(canvas, _time, -1, ClientRect(_hwnd), DT_SINGLELINE|DT_VCENTER|DT_NOPREFIX);
}
