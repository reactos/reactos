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
#include "shellclasses.h"
#include "window.h"

#include "../globals.h"


WindowClass::WindowClass(LPCTSTR classname, UINT style_, WNDPROC wndproc)
{
	memset(this, 0, sizeof(WNDCLASSEX));

	cbSize = sizeof(WNDCLASSEX);
	style = style_;
	hInstance = g_Globals._hInstance;
	hCursor = LoadCursor(0, IDC_ARROW);

	lpszClassName = classname;
	lpfnWndProc = wndproc;

	_atomClass = 0;
}


IconWindowClass::IconWindowClass(LPCTSTR classname, UINT nid, UINT style, WNDPROC wndproc)
 :	WindowClass(classname, style, wndproc)
{
	hIcon = ResIcon(nid);
	hIconSm = SmallIcon(nid);
}


HHOOK Window::s_hcbtHook = 0;
Window::WINDOWCREATORFUNC Window::s_window_creator = NULL;
const void* Window::s_new_info = NULL;


HWND Window::Create(WINDOWCREATORFUNC creator, DWORD dwExStyle,
					LPCTSTR lpClassName, LPCTSTR lpWindowName,
					DWORD dwStyle, int x, int y, int w, int h,
					HWND hwndParent, HMENU hMenu, LPVOID lpParam)
{
	s_window_creator = creator;
	s_new_info = NULL;

	return CreateWindowEx(dwExStyle, lpClassName, lpWindowName, dwStyle,
							x, y, w, h,
							hwndParent, hMenu, g_Globals._hInstance, 0/*lpParam*/);
}

HWND Window::Create(WINDOWCREATORFUNC creator, const void* info, DWORD dwExStyle,
					LPCTSTR lpClassName, LPCTSTR lpWindowName,
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

Window* Window::create_mdi_child(HWND hmdiclient, const MDICREATESTRUCT& mcs, WINDOWCREATORFUNC creator, const void* info)
{
	s_window_creator = creator;
	s_new_info = info;
	s_new_child_wnd = NULL;

	s_hcbtHook = SetWindowsHookEx(WH_CBT, CBTHookProc, 0, GetCurrentThreadId());

	HWND hwnd = (HWND) SendMessage(hmdiclient, WM_MDICREATE, 0, (LPARAM)&mcs);

	UnhookWindowsHookEx(s_hcbtHook);

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

	return CallNextHookEx(s_hcbtHook, code, wparam, lparam);
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

		WINDOWCREATORFUNC window_creator = s_window_creator;
		s_window_creator = NULL;

		wnd = window_creator(hwnd, info);
	}

	return wnd;
}


LRESULT	Window::Init(LPCREATESTRUCT pcs)
{
	return 0;
}


LRESULT CALLBACK Window::WindowWndProc(HWND hwnd, UINT nmsg, WPARAM wparam, LPARAM lparam)
{
	Window* pThis = get_window(hwnd);

	if (pThis) {
		switch(nmsg) {
		  case WM_COMMAND:
			return pThis->Command(LOWORD(wparam), HIWORD(wparam));

		  case WM_NOTIFY:
			return pThis->Notify(wparam, (NMHDR*)lparam);

		  case WM_CREATE:
			return pThis->Init((LPCREATESTRUCT)lparam);

		  case WM_NCDESTROY:
			delete pThis;
			return 0;

		  default:
			return pThis->WndProc(nmsg, wparam, lparam);
		}
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


SubclassedWindow::SubclassedWindow(HWND hwnd)
 :	super(hwnd)
{
	_orgWndProc = SubclassWindow(_hwnd, WindowWndProc);

	if (!_orgWndProc)
		delete this;
}

LRESULT SubclassedWindow::WndProc(UINT nmsg, WPARAM wparam, LPARAM lparam)
{
	return CallWindowProc(_orgWndProc, _hwnd, nmsg, wparam, lparam);
}


ChildWindow::ChildWindow(HWND hwnd)
 :	super(hwnd)
{
	_left_hwnd = 0;
	_right_hwnd = 0;

	_focus_pane = 0;
	_split_pos = DEFAULT_SPLIT_POS;
	_last_split = DEFAULT_SPLIT_POS;
}


ChildWindow* ChildWindow::create(HWND hmdiclient, const RECT& rect, WINDOWCREATORFUNC creator, LPCTSTR classname, LPCTSTR title)
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
		ClientRect rt(_hwnd);
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
		int x = LOWORD(lparam);

		ClientRect rt(_hwnd);

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
				ClientRect rt(_hwnd);
				resize_children(rt.right, rt.bottom);
				_last_split = -1;
				ReleaseCapture();
				SetCursor(LoadCursor(0, IDC_ARROW));
			}
		break;

	  case WM_MOUSEMOVE:
		if (GetCapture() == _hwnd) {
			int x = LOWORD(lparam);

			ClientRect rt(_hwnd);

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


WindowSet Window::s_pretranslate_windows;

void Window::register_pretranslate(HWND hwnd)
{
	s_pretranslate_windows.insert(hwnd);
}

void Window::unregister_pretranslate(HWND hwnd)
{
	s_pretranslate_windows.erase(hwnd);
}

BOOL Window::pretranslate_msg(LPMSG pmsg)
{
	for(WindowSet::const_iterator it=Window::s_pretranslate_windows.begin(); it!=s_pretranslate_windows.end(); ++it)
		if (SendMessage(*it, WM_TRANSLATE_MSG, 0, (LPARAM)pmsg))
			return TRUE;

	return FALSE;
}


WindowSet Window::s_dialogs;

void Window::register_dialog(HWND hwnd)
{
	s_dialogs.insert(hwnd);
}

void Window::unregister_dialog(HWND hwnd)
{
	s_dialogs.erase(hwnd);
}

BOOL Window::dispatch_dialog_msg(MSG* pmsg)
{
	for(WindowSet::const_iterator it=Window::s_dialogs.begin(); it!=s_dialogs.end(); ++it)
		if (IsDialogMessage(*it, pmsg))
			return TRUE;

	return FALSE;
}


PreTranslateWindow::PreTranslateWindow(HWND hwnd)
 :	super(hwnd)
{
	register_pretranslate(hwnd);
}

PreTranslateWindow::~PreTranslateWindow()
{
	unregister_pretranslate(_hwnd);
}


Dialog::Dialog(HWND hwnd)
 :	super(hwnd)
{
	register_dialog(hwnd);
}

Dialog::~Dialog()
{
	unregister_dialog(_hwnd);
}


int Window::MessageLoop()
{
	MSG msg;

	while(GetMessage(&msg, 0, 0, 0)) {
		if (pretranslate_msg(&msg))
			continue;

		if (dispatch_dialog_msg(&msg))
			continue;

		TranslateMessage(&msg);

		try {
			DispatchMessage(&msg);
		} catch(COMException& e) {
			HandleException(e, g_Globals._hMainWnd);
		}
	}

	return msg.wParam;
}


Button::Button(HWND parent, LPCTSTR text, int left, int top, int width, int height,
				UINT id, DWORD flags, DWORD exStyle)
{
	_hwnd = CreateWindowEx(exStyle, TEXT("BUTTON"), text, flags, left, top, width, height,
							parent, (HMENU)id, g_Globals._hInstance, 0);
}


static RECT s_MyDrawText_Rect = {0, 0, 0, 0};

static BOOL CALLBACK MyDrawText(HDC hdc, LPARAM data, int cnt)
{
	::DrawText(hdc, (LPCTSTR)data, cnt, &s_MyDrawText_Rect, DT_SINGLELINE);
	return TRUE;
}


LRESULT ColorButton::WndProc(UINT message, WPARAM wparam, LPARAM lparam)
{
	if (message == WM_DISPATCH_DRAWITEM) {
		LPDRAWITEMSTRUCT dis = (LPDRAWITEMSTRUCT) lparam;
		UINT style = DFCS_BUTTONPUSH;

		if (dis->itemState & ODS_DISABLED)
			style |= DFCS_INACTIVE;

		RECT textRect = {dis->rcItem.left+2, dis->rcItem.top+2, dis->rcItem.right-4, dis->rcItem.bottom-4};

		if (dis->itemState & ODS_SELECTED) {
			style |= DFCS_PUSHED;
			++textRect.left;	++textRect.top;
			++textRect.right;	++textRect.bottom;
		}

		DrawFrameControl(dis->hDC, &dis->rcItem, DFC_BUTTON, style);

		TCHAR text[BUFFER_LEN];
		GetWindowText(_hwnd, text, BUFFER_LEN);

		if (dis->itemState & (ODS_DISABLED|ODS_GRAYED)) {
			COLORREF gray = GetSysColor(COLOR_GRAYTEXT);

			if (gray) {
				{
				TextColor lcColor(dis->hDC, GetSysColor(COLOR_BTNHIGHLIGHT));
				RECT shadowRect = {textRect.left+1, textRect.top+1, textRect.right+1, textRect.bottom+1};
				DrawText(dis->hDC, text, -1, &shadowRect, DT_SINGLELINE|DT_VCENTER|DT_CENTER);
				}

				BkMode mode(dis->hDC, TRANSPARENT);
				TextColor lcColor(dis->hDC, gray);
				DrawText(dis->hDC, text, -1, &textRect, DT_SINGLELINE|DT_VCENTER|DT_CENTER);
			} else {
				int old_r = textRect.right;
				int old_b = textRect.bottom;
				DrawText(dis->hDC, text, -1, &textRect, DT_SINGLELINE|DT_VCENTER|DT_CENTER|DT_CALCRECT);
				int x = textRect.left + (old_r-textRect.right)/2;
				int y = textRect.top + (old_b-textRect.bottom)/2;
				int w = textRect.right-textRect.left;
				int h = textRect.bottom-textRect.top;
				s_MyDrawText_Rect.right = w;
				s_MyDrawText_Rect.bottom = h;
				GrayString(dis->hDC, GetSysColorBrush(COLOR_GRAYTEXT), MyDrawText, (LPARAM)text, -1, x, y, w, h);
			}
		} else {
			TextColor lcColor(dis->hDC, _textColor);
			DrawText(dis->hDC, text, -1, &textRect, DT_SINGLELINE|DT_VCENTER|DT_CENTER);
		}

		if (dis->itemState & ODS_FOCUS) {
			RECT rect = {
				dis->rcItem.left+3, dis->rcItem.top+3,
				dis->rcItem.right-dis->rcItem.left-4, dis->rcItem.bottom-dis->rcItem.top-4
			};
			if (dis->itemState & ODS_SELECTED) {
				++rect.left;	++rect.top;
				++rect.right;	++rect.bottom;
			}
			DrawFocusRect(dis->hDC, &rect);
		}

		return TRUE;
	} else
		return super::WndProc(message, wparam, lparam);
}


LRESULT PictureButton::WndProc(UINT message, WPARAM wparam, LPARAM lparam)
{
	if (message == WM_DISPATCH_DRAWITEM) {
		LPDRAWITEMSTRUCT dis = (LPDRAWITEMSTRUCT) lparam;
		UINT style = DFCS_BUTTONPUSH;

		if (dis->itemState & ODS_DISABLED)
			style |= DFCS_INACTIVE;

		POINT iconPos = {dis->rcItem.left+2, (dis->rcItem.top+dis->rcItem.bottom-16)/2};
		RECT textRect = {dis->rcItem.left+16+4, dis->rcItem.top+2, dis->rcItem.right-4, dis->rcItem.bottom-4};

		if (dis->itemState & ODS_SELECTED) {
			style |= DFCS_PUSHED;
			++iconPos.x;		++iconPos.y;
			++textRect.left;	++textRect.top;
			++textRect.right;	++textRect.bottom;
		}

		if (_flat) {
			if (GetWindowStyle(_hwnd) & BS_FLAT)	// Only with BS_FLAT set, there will be drawn a frame without highlight.
				DrawEdge(dis->hDC, &dis->rcItem, EDGE_RAISED, BF_RECT|BF_FLAT);
		} else
			DrawFrameControl(dis->hDC, &dis->rcItem, DFC_BUTTON, style);

		DrawIconEx(dis->hDC, iconPos.x, iconPos.y, _hicon, 16, 16, 0, GetSysColorBrush(COLOR_BTNFACE), DI_NORMAL);

		TCHAR text[BUFFER_LEN];
		GetWindowText(_hwnd, text, BUFFER_LEN);

		if (dis->itemState & (ODS_DISABLED|ODS_GRAYED)) {
			COLORREF gray = GetSysColor(COLOR_GRAYTEXT);

			if (gray) {
				{
				TextColor lcColor(dis->hDC, GetSysColor(COLOR_BTNHIGHLIGHT));
				RECT shadowRect = {textRect.left+1, textRect.top+1, textRect.right+1, textRect.bottom+1};
				DrawText(dis->hDC, text, -1, &shadowRect, DT_SINGLELINE|DT_VCENTER/*|DT_CENTER*/);
				}

				BkMode mode(dis->hDC, TRANSPARENT);
				TextColor lcColor(dis->hDC, gray);
				DrawText(dis->hDC, text, -1, &textRect, DT_SINGLELINE|DT_VCENTER/*|DT_CENTER*/);
			} else {
				int old_r = textRect.right;
				int old_b = textRect.bottom;
				DrawText(dis->hDC, text, -1, &textRect, DT_SINGLELINE|DT_VCENTER/*|DT_CENTER*/|DT_CALCRECT);
				int x = textRect.left + (old_r-textRect.right)/2;
				int y = textRect.top + (old_b-textRect.bottom)/2;
				int w = textRect.right-textRect.left;
				int h = textRect.bottom-textRect.top;
				s_MyDrawText_Rect.right = w;
				s_MyDrawText_Rect.bottom = h;
				GrayString(dis->hDC, GetSysColorBrush(COLOR_GRAYTEXT), MyDrawText, (LPARAM)text, -1, x, y, w, h);
			}
		} else {
			//BkMode mode(dis->hDC, TRANSPARENT);
			TextColor lcColor(dis->hDC, GetSysColor(COLOR_BTNTEXT));
			DrawText(dis->hDC, text, -1, &textRect, DT_SINGLELINE|DT_VCENTER/*|DT_CENTER*/);
		}

		if (dis->itemState & ODS_FOCUS) {
			RECT rect = {
				dis->rcItem.left+3, dis->rcItem.top+3,
				dis->rcItem.right-dis->rcItem.left-4, dis->rcItem.bottom-dis->rcItem.top-4
			};
			if (dis->itemState & ODS_SELECTED) {
				++rect.left;	++rect.top;
				++rect.right;	++rect.bottom;
			}
			DrawFocusRect(dis->hDC, &rect);
		}

		return TRUE;
	} else
		return super::WndProc(message, wparam, lparam);
}


LRESULT StartmenuEntry::WndProc(UINT message, WPARAM wparam, LPARAM lparam)
{
	if (message == WM_DISPATCH_DRAWITEM) {
		LPDRAWITEMSTRUCT dis = (LPDRAWITEMSTRUCT) lparam;
		UINT style = DFCS_BUTTONPUSH;

		if (dis->itemState & ODS_DISABLED)
			style |= DFCS_INACTIVE;

		POINT iconPos = {dis->rcItem.left+2, (dis->rcItem.top+dis->rcItem.bottom-16)/2};
		RECT textRect = {dis->rcItem.left+16+4, dis->rcItem.top+2, dis->rcItem.right-4, dis->rcItem.bottom-4};

		if (dis->itemState & ODS_SELECTED) {
			style |= DFCS_PUSHED;
			++iconPos.x;		++iconPos.y;
			++textRect.left;	++textRect.top;
			++textRect.right;	++textRect.bottom;
		}

		int bk_color = COLOR_BTNFACE;
		int text_color = COLOR_BTNTEXT;

		if (dis->itemState & ODS_FOCUS) {
			bk_color = COLOR_HIGHLIGHT;
			text_color = COLOR_HIGHLIGHTTEXT;
		}

		HBRUSH bk_brush = GetSysColorBrush(bk_color);

		FillRect(dis->hDC, &dis->rcItem, bk_brush);
		DrawIconEx(dis->hDC, iconPos.x, iconPos.y, _hicon, 16, 16, 0, bk_brush, DI_NORMAL);

		TCHAR text[BUFFER_LEN];
		GetWindowText(_hwnd, text, BUFFER_LEN);

		if (dis->itemState & (ODS_DISABLED|ODS_GRAYED)) {
			COLORREF gray = GetSysColor(COLOR_GRAYTEXT);

			if (gray) {
				{
				TextColor lcColor(dis->hDC, GetSysColor(COLOR_BTNHIGHLIGHT));
				RECT shadowRect = {textRect.left+1, textRect.top+1, textRect.right+1, textRect.bottom+1};
				DrawText(dis->hDC, text, -1, &shadowRect, DT_SINGLELINE|DT_VCENTER/*|DT_CENTER*/);
				}

				BkMode mode(dis->hDC, TRANSPARENT);
				TextColor lcColor(dis->hDC, gray);
				DrawText(dis->hDC, text, -1, &textRect, DT_SINGLELINE|DT_VCENTER/*|DT_CENTER*/);
			} else {
				int old_r = textRect.right;
				int old_b = textRect.bottom;
				DrawText(dis->hDC, text, -1, &textRect, DT_SINGLELINE|DT_VCENTER/*|DT_CENTER*/|DT_CALCRECT);
				int x = textRect.left + (old_r-textRect.right)/2;
				int y = textRect.top + (old_b-textRect.bottom)/2;
				int w = textRect.right-textRect.left;
				int h = textRect.bottom-textRect.top;
				s_MyDrawText_Rect.right = w;
				s_MyDrawText_Rect.bottom = h;
				GrayString(dis->hDC, GetSysColorBrush(COLOR_GRAYTEXT), MyDrawText, (LPARAM)text, -1, x, y, w, h);
			}
		} else {
			BkMode mode(dis->hDC, TRANSPARENT);
			TextColor lcColor(dis->hDC, GetSysColor(text_color));
			DrawText(dis->hDC, text, -1, &textRect, DT_SINGLELINE|DT_VCENTER/*|DT_CENTER*/);
		}

		return TRUE;
	} else
		return super::WndProc(message, wparam, lparam);
}
