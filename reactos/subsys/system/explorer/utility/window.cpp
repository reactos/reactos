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
 // window.cpp
 //
 // Martin Fuchs, 23.07.2003
 //


#include "utility.h"
#include "window.h"

#include "../globals.h"


WindowClass::WindowClass(LPCTSTR classname, WNDPROC wndproc)
{
	memset(this, 0, sizeof(WNDCLASSEX));

	cbSize = sizeof(WNDCLASSEX);
	hInstance = g_Globals._hInstance;

	lpszClassName = classname;
	lpfnWndProc = wndproc;
}


HHOOK Window::s_hcbthook = 0;
Window::WindowCreatorFunc Window::s_window_creator = NULL;
const void* Window::s_new_info = NULL;


HWND Window::Create(WindowCreatorFunc creator,
					DWORD dwExStyle, LPCTSTR lpClassName, LPCTSTR lpWindowName,
					DWORD dwStyle, int x, int y, int w, int h,
					HWND hwndParent, HMENU hMenu, LPVOID lpParam)
{
	s_window_creator = creator;
	s_new_info = NULL;

	return CreateWindowEx(dwExStyle, lpClassName, lpWindowName, dwStyle,
							x, y, w, h,
							hwndParent, hMenu, g_Globals._hInstance, 0/*lpParam*/);
}

HWND Window::Create(WindowCreatorFunc creator, const void* info,
					DWORD dwExStyle, LPCTSTR lpClassName, LPCTSTR lpWindowName,
					DWORD dwStyle, int x, int y, int w, int h,
					HWND hwndParent, HMENU hMenu, LPVOID lpParam)
{
	s_window_creator = creator;
	s_new_info = info;

	return CreateWindowEx(dwExStyle, lpClassName, lpWindowName, dwStyle,
							x, y, w, h,
							hwndParent, hMenu, g_Globals._hInstance, 0/*lpParam*/);
}


static Window* s_new_child_wnd = NULL;

Window* Window::create_mdi_child(HWND hmdiclient, const MDICREATESTRUCT& mcs, WindowCreatorFunc creator, const void* info)
{
	s_window_creator = creator;
	s_new_info = info;
	s_new_child_wnd = NULL;

	s_hcbthook = SetWindowsHookEx(WH_CBT, CBTHookProc, 0, GetCurrentThreadId());

	HWND hwnd = (HWND) SendMessage(hmdiclient, WM_MDICREATE, 0, (LPARAM)&mcs);

	UnhookWindowsHookEx(s_hcbthook);

	Window* child = s_new_child_wnd;
	s_new_info = NULL;
	s_new_child_wnd = NULL;

	if (!hwnd || !child || !child->_hwnd)
		child = NULL;

	return child;
}

LRESULT CALLBACK Window::CBTHookProc(int code, WPARAM wparam, LPARAM lparam)
{
	if (code == HCBT_CREATEWND) {
		 // create Window controller and associate it with the window handle
		Window* child = get_window((HWND)wparam);

		if (child)
			s_new_child_wnd = child;
	}

	return CallNextHookEx(s_hcbthook, code, wparam, lparam);
}


 // get window controller from window handle
 // if not already present, create a new controller

Window* Window::get_window(HWND hwnd)
{
	Window* wnd = (Window*) GetWindowLong(hwnd, GWL_USERDATA);

	if (wnd)
		return wnd;

	if (s_window_creator) {	// protect for recursion
		const void* info = s_new_info;
		s_new_info = NULL;

		WindowCreatorFunc window_creator = s_window_creator;
		s_window_creator = NULL;

		wnd = window_creator(hwnd, info);
	}

	return wnd;
}


LRESULT CALLBACK Window::WndProc(HWND hwnd, UINT nmsg, WPARAM wparam, LPARAM lparam)
{
	Window* pThis = get_window(hwnd);

	if (pThis) {
		switch(nmsg) {
		  case WM_NCDESTROY:
			delete pThis;
			return 0;

		  case WM_COMMAND:
			pThis->Command(LOWORD(wparam), HIWORD(wparam));
			return 0;

		  case WM_NOTIFY:
			return pThis->Notify(wparam, (NMHDR*)lparam);
		}

		return pThis->WndProc(nmsg, wparam, lparam);
	}
	else
		return DefWindowProc(hwnd, nmsg, wparam, lparam);
}

LRESULT Window::WndProc(UINT nmsg, WPARAM wparam, LPARAM lparam)
{
	return DefWindowProc(_hwnd, nmsg, wparam, lparam);
}

int Window::Command(int id, int code)
{
	return 0;
}

int Window::Notify(int id, NMHDR* pnmh)
{
	return 0;
}


ChildWindow::ChildWindow(HWND hwnd)
 :	Window(hwnd)
{
	_left_hwnd = 0;
	_right_hwnd = 0;

	_focus_pane = 0;
	_split_pos = DEFAULT_SPLIT_POS;
	_last_split = DEFAULT_SPLIT_POS;
}


ChildWindow* ChildWindow::create(HWND hmdiclient, const RECT& rect, WindowCreatorFunc creator, LPCTSTR classname, LPCTSTR title)
{
	MDICREATESTRUCT mcs;

	mcs.szClass = classname;
	mcs.szTitle = title;
	mcs.hOwner	= g_Globals._hInstance;
	mcs.x		= rect.left,
	mcs.y		= rect.top;
	mcs.cx		= rect.right - rect.left;
	mcs.cy		= rect.bottom - rect.top;
	mcs.style	= 0;
	mcs.lParam	= 0;

	return static_cast<ChildWindow*>(create_mdi_child(hmdiclient, mcs, creator));
}


LRESULT ChildWindow::WndProc(UINT nmsg, WPARAM wparam, LPARAM lparam)
{
	switch(nmsg) {
	  case WM_PAINT: {
		PAINTSTRUCT ps;
		HBRUSH lastBrush;
		RECT rt;
		GetClientRect(_hwnd, &rt);
		BeginPaint(_hwnd, &ps);
		rt.left = _split_pos-SPLIT_WIDTH/2;
		rt.right = _split_pos+SPLIT_WIDTH/2+1;
		lastBrush = SelectBrush(ps.hdc, (HBRUSH)GetStockObject(COLOR_SPLITBAR));
		Rectangle(ps.hdc, rt.left, rt.top-1, rt.right, rt.bottom+1);
		SelectObject(ps.hdc, lastBrush);
		EndPaint(_hwnd, &ps);
		break;}

	  case WM_SETCURSOR:
		if (LOWORD(lparam) == HTCLIENT) {
			POINT pt;
			GetCursorPos(&pt);
			ScreenToClient(_hwnd, &pt);

			if (pt.x>=_split_pos-SPLIT_WIDTH/2 && pt.x<_split_pos+SPLIT_WIDTH/2+1) {
				SetCursor(LoadCursor(0, IDC_SIZEWE));
				return TRUE;
			}
		}
		goto def;

	  case WM_SIZE:
		if (wparam != SIZE_MINIMIZED)
			resize_children(LOWORD(lparam), HIWORD(lparam));
		goto def;

	  case WM_GETMINMAXINFO:
		DefMDIChildProc(_hwnd, nmsg, wparam, lparam);

		{LPMINMAXINFO lpmmi = (LPMINMAXINFO)lparam;

		lpmmi->ptMaxTrackSize.x <<= 1;	// 2*GetSystemMetrics(SM_CXSCREEN) / SM_CXVIRTUALSCREEN
		lpmmi->ptMaxTrackSize.y <<= 1;	// 2*GetSystemMetrics(SM_CYSCREEN) / SM_CYVIRTUALSCREEN
		break;}

	  case WM_LBUTTONDOWN: {
		RECT rt;
		int x = LOWORD(lparam);

		GetClientRect(_hwnd, &rt);

		if (x>=_split_pos-SPLIT_WIDTH/2 && x<_split_pos+SPLIT_WIDTH/2+1) {
			_last_split = _split_pos;
			SetCapture(_hwnd);
		}

		break;}

	  case WM_LBUTTONUP:
		if (GetCapture() == _hwnd)
			ReleaseCapture();
		break;

	  case WM_KEYDOWN:
		if (wparam == VK_ESCAPE)
			if (GetCapture() == _hwnd) {
				_split_pos = _last_split;
				RECT rt; GetClientRect(_hwnd, &rt);
				resize_children(rt.right, rt.bottom);
				_last_split = -1;
				ReleaseCapture();
				SetCursor(LoadCursor(0, IDC_ARROW));
			}
		break;

	  case WM_MOUSEMOVE:
		if (GetCapture() == _hwnd) {
			int x = LOWORD(lparam);

			RECT rt;
			GetClientRect(_hwnd, &rt);

			if (x>=0 && x<rt.right) {
				_split_pos = x;
				resize_children(rt.right, rt.bottom);
				rt.left = x-SPLIT_WIDTH/2;
				rt.right = x+SPLIT_WIDTH/2+1;
				InvalidateRect(_hwnd, &rt, FALSE);
				UpdateWindow(_left_hwnd);
				UpdateWindow(_hwnd);
				UpdateWindow(_right_hwnd);
			}
		}
		break;

	  case WM_DISPATCH_COMMAND:
		return FALSE;

	  default: def:
		return DefMDIChildProc(_hwnd, nmsg, wparam, lparam);
	}

	return 0;
}


void ChildWindow::resize_children(int cx, int cy)
{
	HDWP hdwp = BeginDeferWindowPos(4);
	RECT rt;

	rt.left   = 0;
	rt.top    = 0;
	rt.right  = cx;
	rt.bottom = cy;

	cx = _split_pos + SPLIT_WIDTH/2;

	DeferWindowPos(hdwp, _left_hwnd, 0, rt.left, rt.top, _split_pos-SPLIT_WIDTH/2-rt.left, rt.bottom-rt.top, SWP_NOZORDER|SWP_NOACTIVATE);

	DeferWindowPos(hdwp, _right_hwnd, 0, rt.left+cx+1, rt.top, rt.right-cx, rt.bottom-rt.top, SWP_NOZORDER|SWP_NOACTIVATE);

	EndDeferWindowPos(hdwp);
}
