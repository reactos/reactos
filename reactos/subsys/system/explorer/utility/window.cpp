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


Window::WindowMap	Window::s_wnd_map;

Window::CREATORFUNC	Window::s_window_creator = NULL;
const void*			Window::s_new_info = NULL;

HHOOK				Window::s_hcbtHook = 0;


Window::StaticWindowData& Window::GetStaticWindowData()
{
	static StaticWindowData s_initialized_data;

	return s_initialized_data;
}


Window::Window(HWND hwnd)
 :	WindowHandle(hwnd)
{
	Lock lock(GetStaticWindowData()._map_crit_sect);	// protect access to s_wnd_map

	s_wnd_map[_hwnd] = this;
}

Window::~Window()
{
	Lock lock(GetStaticWindowData()._map_crit_sect);	// protect access to s_wnd_map

	s_wnd_map.erase(_hwnd);
}


HWND Window::Create(CREATORFUNC creator, DWORD dwExStyle,
					LPCTSTR lpClassName, LPCTSTR lpWindowName,
					DWORD dwStyle, int x, int y, int w, int h,
					HWND hwndParent, HMENU hMenu/*, LPVOID lpParam*/)
{
	Lock lock(GetStaticWindowData()._create_crit_sect);	// protect access to s_window_creator and s_new_info

	s_window_creator = creator;
	s_new_info = NULL;

	return CreateWindowEx(dwExStyle, lpClassName, lpWindowName, dwStyle,
							x, y, w, h,
							hwndParent, hMenu, g_Globals._hInstance, 0/*lpParam*/);
}

HWND Window::Create(CREATORFUNC_INFO creator, const void* info, DWORD dwExStyle,
					LPCTSTR lpClassName, LPCTSTR lpWindowName,
					DWORD dwStyle, int x, int y, int w, int h,
					HWND hwndParent, HMENU hMenu/*, LPVOID lpParam*/)
{
	Lock lock(GetStaticWindowData()._create_crit_sect);	// protect access to s_window_creator and s_new_info

	s_window_creator = (CREATORFUNC) creator;
	s_new_info = info;

	return CreateWindowEx(dwExStyle, lpClassName, lpWindowName, dwStyle,
							x, y, w, h,
							hwndParent, hMenu, g_Globals._hInstance, 0/*lpParam*/);
}


Window* Window::create_mdi_child(HWND hmdiclient, const MDICREATESTRUCT& mcs, CREATORFUNC_INFO creator, const void* info)
{
	Lock lock(GetStaticWindowData()._create_crit_sect);	// protect access to s_window_creator and s_new_info

	s_window_creator = (CREATORFUNC) creator;
	s_new_info = info;

	s_hcbtHook = SetWindowsHookEx(WH_CBT, MDICBTHookProc, 0, GetCurrentThreadId());

	HWND hwnd = (HWND) SendMessage(hmdiclient, WM_MDICREATE, 0, (LPARAM)&mcs);

	UnhookWindowsHookEx(s_hcbtHook);

	Window* child = get_window(hwnd);
	s_new_info = NULL;

	if (child && (!hwnd || !child->_hwnd))
		child = NULL;

	return child;
}

LRESULT CALLBACK Window::MDICBTHookProc(int code, WPARAM wparam, LPARAM lparam)
{
	if (code == HCBT_CREATEWND) {
		HWND hwnd = (HWND)wparam;

		 // create Window controller and associate it with the window handle
		Window* child = get_window(hwnd);

		if (!child)
			child = create_controller(hwnd);
	}

	return CallNextHookEx(s_hcbtHook, code, wparam, lparam);
}


/*
Window* Window::create_property_sheet(PropertySheetDialog* ppsd, CREATORFUNC creator, const void* info)
{
	Lock lock(GetStaticWindowData()._create_crit_sect);	// protect access to s_window_creator and s_new_info

	s_window_creator = creator;
	s_new_info = info;

	s_hcbtHook = SetWindowsHookEx(WH_CBT, PropSheetCBTHookProc, 0, GetCurrentThreadId());

	HWND hwnd = (HWND) PropertySheet(ppsd);

	UnhookWindowsHookEx(s_hcbtHook);

	Window* child = get_window(hwnd);
	s_new_info = NULL;

	if (child && (!hwnd || !child->_hwnd))
		child = NULL;

	return child;
}
*/

LRESULT CALLBACK Window::PropSheetCBTHookProc(int code, WPARAM wparam, LPARAM lparam)
{
	if (code == HCBT_CREATEWND) {
		HWND hwnd = (HWND)wparam;

		 // create Window controller and associate it with the window handle
		Window* child = get_window(hwnd);

		if (!child)
			child = create_controller(hwnd);
	}

	return CallNextHookEx(s_hcbtHook, code, wparam, lparam);
}


 /// get window controller from window handle

Window* Window::get_window(HWND hwnd)
{
	{
		Lock lock(GetStaticWindowData()._map_crit_sect);	// protect access to s_wnd_map

		WindowMap::const_iterator found = s_wnd_map.find(hwnd);

		if (found!=s_wnd_map.end())
			return found->second;
	}

	return NULL;
}


 /// create controller for a new window

Window* Window::create_controller(HWND hwnd)
{
	if (s_window_creator) {	// protect for recursion and create the window object only for the first window
		Lock lock(GetStaticWindowData()._create_crit_sect);	// protect access to s_window_creator and s_new_info

		const void* info = s_new_info;
		s_new_info = NULL;

		CREATORFUNC window_creator = s_window_creator;
		s_window_creator = NULL;

		if (info)
			return CREATORFUNC_INFO(window_creator)(hwnd, info);
		else
			return CREATORFUNC(window_creator)(hwnd);
	}

	return NULL;
}


LRESULT	Window::Init(LPCREATESTRUCT pcs)
{
	return 0;
}


LRESULT CALLBACK Window::WindowWndProc(HWND hwnd, UINT nmsg, WPARAM wparam, LPARAM lparam)
{
	Window* pThis = get_window(hwnd);

	if (!pThis)
		pThis = create_controller(hwnd);

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
/*@@TODO: replaced by StartMenu::TrackStartmenu()
	HWND hwnd = _hwnd;

	 // close startup menu and other popup menus
	 // This functionality is for tray notification icons missing in MS Windows.
	if (nmsg == WM_SETFOCUS)
		CancelModes((HWND)wparam);	// erronesly cancels desktop bar resize when switching from another process
*/

	return DefWindowProc(_hwnd, nmsg, wparam, lparam);
}

int Window::Command(int id, int code)
{
	return 1;	// WM_COMMAND not yet handled
}

int Window::Notify(int id, NMHDR* pnmh)
{
	return 0;
}

void Window::CancelModes(HWND hwnd)
{
	if (hwnd)
		PostMessage(hwnd, WM_CANCELMODE, 0, 0);
	else
		PostMessage(HWND_BROADCAST, WM_CANCELMODE, 0, 0);
}


SubclassedWindow::SubclassedWindow(HWND hwnd)
 :	super(hwnd)
{
	_orgWndProc = SubclassWindow(_hwnd, SubclassedWndProc);

	if (!_orgWndProc)
		delete this;
}

LRESULT CALLBACK SubclassedWindow::SubclassedWndProc(HWND hwnd, UINT nmsg, WPARAM wparam, LPARAM lparam)
{
	SubclassedWindow* pThis = GET_WINDOW(SubclassedWindow, hwnd);
	assert(pThis);

	if (pThis) {
		switch(nmsg) {
		  case WM_COMMAND:
			if (!pThis->Command(LOWORD(wparam), HIWORD(wparam)))
				return 0;
			break;

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

	return CallWindowProc(pThis->_orgWndProc, hwnd, nmsg, wparam, lparam);
}

LRESULT SubclassedWindow::WndProc(UINT nmsg, WPARAM wparam, LPARAM lparam)
{
/*@@TODO: replaced by StartMenu::TrackStartmenu()
	 // close startup menu and other popup menus
	 // This functionality is for tray notification icons missing in MS Windows.
	if (nmsg == WM_SETFOCUS)
		CancelModes((HWND)wparam);
*/
	return CallWindowProc(_orgWndProc, _hwnd, nmsg, wparam, lparam);
}

int SubclassedWindow::Command(int id, int code)
{
	return 1;	// WM_COMMAND not yet handled
}

int SubclassedWindow::Notify(int id, NMHDR* pnmh)
{
	return CallWindowProc(_orgWndProc, _hwnd, WM_NOTIFY, id, (LPARAM)pnmh);
}


ChildWindow::ChildWindow(HWND hwnd)
 :	super(hwnd)
{
	_focus_pane = 0;
	_split_pos = DEFAULT_SPLIT_POS;
	_last_split = DEFAULT_SPLIT_POS;
}


ChildWindow* ChildWindow::create(HWND hmdiclient, const RECT& rect, CREATORFUNC_INFO creator, LPCTSTR classname, LPCTSTR title, const void* info)
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

	return static_cast<ChildWindow*>(create_mdi_child(hmdiclient, mcs, creator, info));
}


LRESULT ChildWindow::WndProc(UINT nmsg, WPARAM wparam, LPARAM lparam)
{
	switch(nmsg) {
	  case WM_PAINT: {
		PaintCanvas canvas(_hwnd);
		ClientRect rt(_hwnd);
		rt.left = _split_pos-SPLIT_WIDTH/2;
		rt.right = _split_pos+SPLIT_WIDTH/2+1;
		HBRUSH lastBrush = SelectBrush(canvas, GetStockBrush(COLOR_SPLITBAR));
		Rectangle(canvas, rt.left, rt.top-1, rt.right, rt.bottom+1);
		SelectObject(canvas, lastBrush);
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
		int x = GET_X_LPARAM(lparam);

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

	  case PM_DISPATCH_COMMAND:
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

	if (_left_hwnd) {
		cx = _split_pos + SPLIT_WIDTH/2;

		hdwp = DeferWindowPos(hdwp, _left_hwnd, 0, rt.left, rt.top, _split_pos-SPLIT_WIDTH/2-rt.left, rt.bottom-rt.top, SWP_NOZORDER|SWP_NOACTIVATE);
	} else {
		_split_pos = 0;
		cx = 0;
	}

	hdwp = DeferWindowPos(hdwp, _right_hwnd, 0, rt.left+cx+1, rt.top, rt.right-cx, rt.bottom-rt.top, SWP_NOZORDER|SWP_NOACTIVATE);

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
		if (SendMessage(*it, PM_TRANSLATE_MSG, 0, (LPARAM)pmsg))
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


int Window::MessageLoop()
{
	MSG msg;

	while(GetMessage(&msg, 0, 0, 0)) {
		try {
			if (pretranslate_msg(&msg))
				continue;

			if (dispatch_dialog_msg(&msg))
				continue;

			TranslateMessage(&msg);

			try {
				DispatchMessage(&msg);
			} catch(COMException& e) {
				HandleException(e, 0);
			}
		} catch(COMException& e) {
			HandleException(e, 0);
		}
	}

	return msg.wParam;
}


LRESULT	Window::SendParent(UINT nmsg, WPARAM wparam, LPARAM lparam)
{
	HWND parent = GetParent(_hwnd);

	if (!parent)
		return 0;

	return SendMessage(parent, nmsg, wparam, lparam);
}

LRESULT	Window::PostParent(UINT nmsg, WPARAM wparam, LPARAM lparam)
{
	HWND parent = GetParent(_hwnd);

	if (!parent)
		return 0;

	return PostMessage(parent, nmsg, wparam, lparam);
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

int Dialog::DoModal(UINT nid, CREATORFUNC creator, HWND hwndParent)
{
	Lock lock(GetStaticWindowData()._create_crit_sect);	// protect access to s_window_creator and s_new_info

	s_window_creator = creator;
	s_new_info = NULL;

	return DialogBoxParam(g_Globals._hInstance, MAKEINTRESOURCE(nid), hwndParent, DialogProc, 0/*lpParam*/);
}

int Dialog::DoModal(UINT nid, CREATORFUNC_INFO creator, const void* info, HWND hwndParent)
{
	Lock lock(GetStaticWindowData()._create_crit_sect);	// protect access to s_window_creator and s_new_info

	s_window_creator = (CREATORFUNC) creator;
	s_new_info = NULL;

	return DialogBoxParam(g_Globals._hInstance, MAKEINTRESOURCE(nid), hwndParent, DialogProc, 0/*lpParam*/);
}

INT_PTR CALLBACK Window::DialogProc(HWND hwnd, UINT nmsg, WPARAM wparam, LPARAM lparam)
{
	Window* pThis = get_window(hwnd);

	if (pThis) {
		switch(nmsg) {
		  case WM_COMMAND:
			pThis->Command(LOWORD(wparam), HIWORD(wparam));
			return TRUE;	// message has been processed

		  case WM_NOTIFY:
			pThis->Notify(wparam, (NMHDR*)lparam);
			return TRUE;	// message has been processed

		  case WM_NCDESTROY:
			delete pThis;
			return TRUE;	// message has been processed

		  default:
			return pThis->WndProc(nmsg, wparam, lparam);
		}
	} else if (nmsg == WM_INITDIALOG) {
		pThis = create_controller(hwnd);

		if (pThis)
			return pThis->Init(NULL);
	}

	return FALSE;	// message has not been processed
}

LRESULT Dialog::WndProc(UINT nmsg, WPARAM wparam, LPARAM lparam)
{
	return FALSE;	// message has not been processed
}

int Dialog::Command(int id, int code)
{
	if (code == BN_CLICKED) {
		EndDialog(_hwnd, id);
		return TRUE;	// message has been processed
	}

	return FALSE;
}


ResizeManager::ResizeManager(HWND hwnd)
 :	_hwnd(hwnd)
{
	ClientRect clnt(hwnd);
	_last_size.cx = clnt.right;
	_last_size.cy = clnt.bottom;

	WindowRect rect(hwnd);
	_min_wnd_size.cx = rect.right - rect.left;
	_min_wnd_size.cy = rect.bottom - rect.top;
}

void ResizeManager::HandleSize(int cx, int cy)
{
	ClientRect clnt_rect(_hwnd);
	SIZE new_size = {cx, cy};

	int dx = new_size.cx - _last_size.cx;
	int dy = new_size.cy - _last_size.cy;

	if (!dx && !dy)
		return;

	_last_size = new_size;

	HDWP hDWP = BeginDeferWindowPos(size());

	for(ResizeManager::const_iterator it=begin(); it!=end(); ++it) {
		const ResizeEntry& e = *it;
		RECT move = {0};

		if (e._flags & MOVE_LEFT)	// Die verschiedenen Transformationsmatrizen in move ließen sich eigentlich
			move.left += dx;		// cachen oder vorausberechnen, da sie nur von _flags und der Größenänderung abhängig sind.

		if (e._flags & MOVE_RIGHT)
			move.right += dx;

		if (e._flags & MOVE_TOP)
			move.top += dy;

		if (e._flags & MOVE_BOTTOM)
			move.bottom += dy;

		UINT flags = 0;

		if (!move.left && !move.top)
			flags = SWP_NOMOVE;

		if (move.right==move.left && move.bottom==move.top)
			flags |= SWP_NOSIZE;

		if (flags != (SWP_NOMOVE|SWP_NOSIZE)) {
			HWND hwnd = GetDlgItem(_hwnd, e._id);
			WindowRect rect(hwnd);
			ScreenToClient(_hwnd, rect);

			rect.left	+= move.left;
			rect.right	+= move.right;
			rect.top	+= move.top;
			rect.bottom	+= move.bottom;

			hDWP = DeferWindowPos(hDWP, hwnd, 0, rect.left, rect.top, rect.right-rect.left, rect.bottom-rect.top, flags|SWP_NOACTIVATE|SWP_NOZORDER);
		}
	}

	EndDeferWindowPos(hDWP);
}

void ResizeManager::Resize(int dx, int dy)
{
	::SetWindowPos(_hwnd, 0, 0, 0, _min_wnd_size.cx+dx, _min_wnd_size.cy+dy, SWP_NOMOVE|SWP_NOACTIVATE);

	ClientRect clnt_rect(_hwnd);
	HandleSize(clnt_rect.right, clnt_rect.bottom);
}


Button::Button(HWND parent, LPCTSTR title, int left, int top, int width, int height,
				int id, DWORD flags, DWORD exStyle)
 :	WindowHandle(CreateWindowEx(exStyle, TEXT("BUTTON"), title, flags, left, top, width, height,
							parent, (HMENU)id, g_Globals._hInstance, 0))
{
}


LRESULT OwnerdrawnButton::WndProc(UINT message, WPARAM wparam, LPARAM lparam)
{
	if (message == PM_DISPATCH_DRAWITEM) {
		DrawItem((LPDRAWITEMSTRUCT)lparam);
		return TRUE;
	} else
		return super::WndProc(message, wparam, lparam);
}


Static::Static(HWND parent, LPCTSTR title, int left, int top, int width, int height,
				int id, DWORD flags, DWORD exStyle)
 :	WindowHandle(CreateWindowEx(exStyle, TEXT("STATIC"), title, flags, left, top, width, height,
							parent, (HMENU)id, g_Globals._hInstance, 0))
{
}


static RECT s_MyDrawText_Rect = {0, 0, 0, 0};

static BOOL CALLBACK MyDrawText(HDC hdc, LPARAM data, int cnt)
{
	::DrawText(hdc, (LPCTSTR)data, cnt, &s_MyDrawText_Rect, DT_SINGLELINE);
	return TRUE;
}

void DrawGrayText(HDC hdc, LPRECT pRect, LPCTSTR title, int dt_flags)
{
	COLORREF gray = GetSysColor(COLOR_GRAYTEXT);

	if (gray) {
		TextColor lcColor(hdc, GetSysColor(COLOR_BTNHIGHLIGHT));
		RECT shadowRect = {pRect->left+1, pRect->top+1, pRect->right+1, pRect->bottom+1};
		DrawText(hdc, title, -1, &shadowRect, dt_flags);

		SetTextColor(hdc, gray);
		DrawText(hdc, title, -1, pRect, dt_flags);
	} else {
		int old_r = pRect->right;
		int old_b = pRect->bottom;

		DrawText(hdc, title, -1, pRect, dt_flags|DT_CALCRECT);

		int x = pRect->left + (old_r-pRect->right)/2;
		int y = pRect->top + (old_b-pRect->bottom)/2;
		int w = pRect->right-pRect->left;
		int h = pRect->bottom-pRect->top;
		s_MyDrawText_Rect.right = w;
		s_MyDrawText_Rect.bottom = h;

		GrayString(hdc, GetSysColorBrush(COLOR_GRAYTEXT), MyDrawText, (LPARAM)title, -1, x, y, w, h);
	}
}


/* not yet used
void ColorButton::DrawItem(LPDRAWITEMSTRUCT dis)
{
	UINT state = DFCS_BUTTONPUSH;

	if (dis->itemState & ODS_DISABLED)
		state |= DFCS_INACTIVE;

	RECT textRect = {dis->rcItem.left+2, dis->rcItem.top+2, dis->rcItem.right-4, dis->rcItem.bottom-4};

	if (dis->itemState & ODS_SELECTED) {
		state |= DFCS_PUSHED;
		++textRect.left;	++textRect.top;
		++textRect.right;	++textRect.bottom;
	}

	DrawFrameControl(dis->hDC, &dis->rcItem, DFC_BUTTON, state);

	TCHAR title[BUFFER_LEN];
	GetWindowText(_hwnd, title, BUFFER_LEN);

	BkMode bk_mode(dis->hDC, TRANSPARENT);

	if (dis->itemState & (ODS_DISABLED|ODS_GRAYED))
		DrawGrayText(dis, &textRect, title, DT_SINGLELINE|DT_VCENTER|DT_CENTER);
	else {
		TextColor lcColor(dis->hDC, _textColor);
		DrawText(dis->hDC, title, -1, &textRect, DT_SINGLELINE|DT_VCENTER|DT_CENTER);
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
}
*/


void PictureButton::DrawItem(LPDRAWITEMSTRUCT dis)
{
	UINT state = DFCS_BUTTONPUSH;
	int style = GetWindowStyle(_hwnd);

	if (dis->itemState & ODS_DISABLED)
		state |= DFCS_INACTIVE;

	POINT imagePos;
	RECT textRect;
	int dt_flags;

	if (style & BS_BOTTOM) {
		 // align horizontal centered, vertical floating
		imagePos.x = (dis->rcItem.left + dis->rcItem.right - _cx) / 2;
		imagePos.y = dis->rcItem.top + 2;

		textRect.left = dis->rcItem.left + 2;
		textRect.top = dis->rcItem.top + _cy + 4;
		textRect.right = dis->rcItem.right - 4;
		textRect.bottom = dis->rcItem.bottom - 4;

		dt_flags = DT_SINGLELINE|DT_CENTER|DT_VCENTER;
	} else {
		 // horizontal floating, vertical centered
		imagePos.x = dis->rcItem.left + 2;
		imagePos.y = (dis->rcItem.top + dis->rcItem.bottom - _cy)/2;

		textRect.left = dis->rcItem.left + _cx + 4;
		textRect.top = dis->rcItem.top + 2;
		textRect.right = dis->rcItem.right - 4;
		textRect.bottom = dis->rcItem.bottom - 4;

		dt_flags = DT_SINGLELINE|DT_VCENTER/*|DT_CENTER*/;
	}

	if (dis->itemState & ODS_SELECTED) {
		state |= DFCS_PUSHED;
		++imagePos.x;		++imagePos.y;
		++textRect.left;	++textRect.top;
		++textRect.right;	++textRect.bottom;
	}

	if (_flat) {
		FillRect(dis->hDC, &dis->rcItem, _hBrush);

		if (style & BS_FLAT)	// Only with BS_FLAT set, there will be drawn a frame without highlight.
			DrawEdge(dis->hDC, &dis->rcItem, EDGE_RAISED, BF_RECT|BF_FLAT);
	} else
		DrawFrameControl(dis->hDC, &dis->rcItem, DFC_BUTTON, state);

	if (_hIcon)
		DrawIconEx(dis->hDC, imagePos.x, imagePos.y, _hIcon, _cx, _cy, 0, _hBrush, DI_NORMAL);
	else {
		MemCanvas mem_dc;
		BitmapSelection sel(mem_dc, _hBmp);
		BitBlt(dis->hDC, imagePos.x, imagePos.y, _cx, _cy, mem_dc, 0, 0, SRCCOPY);
	}

	TCHAR title[BUFFER_LEN];
	GetWindowText(_hwnd, title, BUFFER_LEN);

	BkMode bk_mode(dis->hDC, TRANSPARENT);

	if (dis->itemState & (ODS_DISABLED|ODS_GRAYED))
		DrawGrayText(dis->hDC, &textRect, title, dt_flags);
	else {
		TextColor lcColor(dis->hDC, GetSysColor(COLOR_BTNTEXT));
		DrawText(dis->hDC, title, -1, &textRect, dt_flags);
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
}


void FlatButton::DrawItem(LPDRAWITEMSTRUCT dis)
{
	UINT style = DFCS_BUTTONPUSH;

	if (dis->itemState & ODS_DISABLED)
		style |= DFCS_INACTIVE;

	RECT textRect = {dis->rcItem.left+2, dis->rcItem.top+2, dis->rcItem.right-4, dis->rcItem.bottom-4};

	if (dis->itemState & ODS_SELECTED) {
		style |= DFCS_PUSHED;
		++textRect.left;	++textRect.top;
		++textRect.right;	++textRect.bottom;
	}

	FillRect(dis->hDC, &dis->rcItem, GetSysColorBrush(COLOR_BTNFACE));

	 // highlight the button?
	if (_active)
		DrawEdge(dis->hDC, &dis->rcItem, EDGE_ETCHED, BF_RECT);
	else if (GetWindowStyle(_hwnd) & BS_FLAT)	// Only with BS_FLAT there will be drawn a frame to show highlighting.
		DrawEdge(dis->hDC, &dis->rcItem, EDGE_RAISED, BF_RECT|BF_FLAT);

	TCHAR txt[BUFFER_LEN];
	int txt_len = GetWindowText(_hwnd, txt, BUFFER_LEN);

	if (dis->itemState & (ODS_DISABLED|ODS_GRAYED)) {
		COLORREF gray = GetSysColor(COLOR_GRAYTEXT);

		if (gray) {
			{
			TextColor lcColor(dis->hDC, GetSysColor(COLOR_BTNHIGHLIGHT));
			RECT shadowRect = {textRect.left+1, textRect.top+1, textRect.right+1, textRect.bottom+1};
			DrawText(dis->hDC, txt, txt_len, &shadowRect, DT_SINGLELINE|DT_VCENTER|DT_CENTER);
			}

			BkMode mode(dis->hDC, TRANSPARENT);
			TextColor lcColor(dis->hDC, gray);
			DrawText(dis->hDC, txt, txt_len, &textRect, DT_SINGLELINE|DT_VCENTER|DT_CENTER);
		} else {
			int old_r = textRect.right;
			int old_b = textRect.bottom;
			DrawText(dis->hDC, txt, txt_len, &textRect, DT_SINGLELINE|DT_VCENTER|DT_CENTER|DT_CALCRECT);
			int x = textRect.left + (old_r-textRect.right)/2;
			int y = textRect.top + (old_b-textRect.bottom)/2;
			int w = textRect.right-textRect.left;
			int h = textRect.bottom-textRect.top;
			s_MyDrawText_Rect.right = w;
			s_MyDrawText_Rect.bottom = h;
			GrayString(dis->hDC, GetSysColorBrush(COLOR_GRAYTEXT), MyDrawText, (LPARAM)txt, txt_len, x, y, w, h);
		}
	} else {
		TextColor lcColor(dis->hDC, _active? _activeColor: _textColor);
		DrawText(dis->hDC, txt, txt_len, &textRect, DT_SINGLELINE|DT_VCENTER|DT_CENTER);
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
}

LRESULT	FlatButton::WndProc(UINT nmsg, WPARAM wparam, LPARAM lparam)
{
	switch(nmsg) {
	  case WM_MOUSEMOVE: {
		bool active = false;

		if (IsWindowEnabled(_hwnd)) {
			DWORD pid_foreground;
			HWND hwnd_foreground = GetForegroundWindow();	//@@ vielleicht besser über WM_ACTIVATEAPP-Abfrage
			GetWindowThreadProcessId(hwnd_foreground, &pid_foreground);

			if (GetCurrentProcessId() == pid_foreground) {
				POINT pt = {GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam)};
				ClientRect clntRect(_hwnd);

				 // highlight the button?
				if (pt.x>=clntRect.left && pt.x<clntRect.right && pt.y>=clntRect.top && pt.y<clntRect.bottom)
					active = true;
			}
		}

		if (active != _active) {
			_active = active;

			if (active) {
				TRACKMOUSEEVENT tme = {sizeof(tme), /*TME_HOVER|*/TME_LEAVE, _hwnd/*, HOVER_DEFAULT*/};
				_TrackMouseEvent(&tme);
			}

			InvalidateRect(_hwnd, NULL, TRUE);
		}

		return 0;}

	  case WM_LBUTTONUP: {
		POINT pt = {GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam)};
		ClientRect clntRect(_hwnd);

		 // no more in the active rectangle?
		if (pt.x<clntRect.left || pt.x>=clntRect.right || pt.y<clntRect.top || pt.y>=clntRect.bottom)
			goto cancel_press;

		goto def;}

	  case WM_CANCELMODE:
	  cancel_press: {
		TRACKMOUSEEVENT tme = {sizeof(tme), /*TME_HOVER|*/TME_LEAVE|TME_CANCEL, _hwnd/*, HOVER_DEFAULT*/};
		_TrackMouseEvent(&tme);
		_active = false;
		ReleaseCapture();}
		// fall through

	  case WM_MOUSELEAVE:
		if (_active) {
			_active = false;

			InvalidateRect(_hwnd, NULL, TRUE);
		}

		return 0;

	  default: def:
		return super::WndProc(nmsg, wparam, lparam);
	}
}


HyperlinkCtrl::HyperlinkCtrl(HWND hwnd, COLORREF colorLink, COLORREF colorVisited)
 :	super(hwnd),
	_cmd(ResString(GetDlgCtrlID(hwnd))),
	_textColor(colorLink),
	_colorVisited(colorVisited),
	_hfont(0),
	_crsr_link(0)
{
	init();
}

HyperlinkCtrl::HyperlinkCtrl(HWND owner, int id, COLORREF colorLink, COLORREF colorVisited)
 :	super(GetDlgItem(owner, id)),
	_cmd(ResString(id)),
	_textColor(colorLink),
	_colorVisited(colorVisited),
	_hfont(0),
	_crsr_link(0)
{
	init();
}

void HyperlinkCtrl::init()
{
	if (_cmd.empty()) {
		TCHAR txt[BUFFER_LEN];
		_cmd.assign(txt, GetWindowText(_hwnd, txt, BUFFER_LEN));
	}
}

HyperlinkCtrl::~HyperlinkCtrl()
{
	if (_hfont)
		DeleteObject(_hfont);
}

LRESULT HyperlinkCtrl::WndProc(UINT message, WPARAM wparam, LPARAM lparam)
{
	switch(message) {
	  case PM_DISPATCH_CTLCOLOR: {
		if (!_hfont) {
			HFONT hfont = (HFONT) SendMessage(_hwnd, WM_GETFONT, 0, 0);
			LOGFONT lf; GetObject(hfont, sizeof(lf), &lf);
			lf.lfUnderline = TRUE;
			_hfont = CreateFontIndirect(&lf);
		}

		HDC hdc = (HDC) wparam;
		SetTextColor(hdc, _textColor);	//@@
		SelectFont(hdc, _hfont);
		SetBkMode(hdc, TRANSPARENT);
		return (LRESULT)GetStockObject(HOLLOW_BRUSH);
	  }

	  case WM_SETCURSOR:
		if (!_crsr_link)
			_crsr_link = LoadCursor(0, IDC_HAND);

		if (_crsr_link)
			SetCursor(_crsr_link);
		return 0;

	  case WM_NCHITTEST:
		return HTCLIENT;	// Aktivierung von Maus-Botschaften

	  case WM_LBUTTONDOWN:
		if (LaunchLink()) {
			_textColor = _colorVisited;
			InvalidateRect(_hwnd, NULL, FALSE);
		} else
			MessageBeep(0);
		return 0;

	  default:
		return super::WndProc(message, wparam, lparam);
	}
}


ToolTip::ToolTip(HWND owner)
 :	super(CreateWindowEx(WS_EX_TOPMOST|WS_EX_NOPARENTNOTIFY, TOOLTIPS_CLASS, 0,
				 WS_POPUP|TTS_NOPREFIX|TTS_ALWAYSTIP, CW_USEDEFAULT,CW_USEDEFAULT,CW_USEDEFAULT,CW_USEDEFAULT,
				 owner, 0, g_Globals._hInstance, 0))
{
	activate();
}


ListSort::ListSort(HWND hwndListview, PFNLVCOMPARE compare_fct)
 :	WindowHandle(hwndListview),
	_compare_fct(compare_fct)
{
	_sort_crit = 0;
	_direction = false;
}

void ListSort::toggle_sort(int idx)
{
	if (_sort_crit == idx)
		_direction = !_direction;
	else {
		_sort_crit = idx;
		_direction = false;
	}
}

void ListSort::sort()
{
	int idx = ListView_GetSelectionMark(_hwnd);
	LPARAM param = ListView_GetItemData(_hwnd, idx);

	ListView_SortItems(_hwnd, _compare_fct, (LPARAM)this);

	if (idx >= 0) {
		idx = ListView_FindItemPara(_hwnd, param);
		ListView_EnsureVisible(_hwnd, idx, FALSE);
	}
}


PropSheetPage::PropSheetPage(UINT nid, Window::CREATORFUNC dlg_creator)
 :	_dlg_creator(dlg_creator)
{
	PROPSHEETPAGE::dwSize		= sizeof(PROPSHEETPAGE);
	PROPSHEETPAGE::dwFlags		= 0;
	PROPSHEETPAGE::hInstance	= g_Globals._hInstance;
	PROPSHEETPAGE::pszTemplate	= MAKEINTRESOURCE(nid);
	PROPSHEETPAGE::pfnDlgProc	= PropSheetPageDlg::DialogProc;
	PROPSHEETPAGE::lParam		= (LPARAM) this;
}


#ifndef PSM_GETRESULT	// currently (as of 18.01.2004) missing in MinGW headers
#define PSM_GETRESULT				(WM_USER + 135)
#define PropSheet_GetResult(hDlg)	SNDMSG(hDlg, PSM_GETRESULT, 0, 0)
#endif


PropertySheetDialog::PropertySheetDialog(HWND owner)
 :	_hwnd(0)
{
	PROPSHEETHEADER::dwSize = sizeof(PROPSHEETHEADER);
	PROPSHEETHEADER::dwFlags = PSH_PROPSHEETPAGE | PSH_MODELESS;
	PROPSHEETHEADER::hwndParent = owner;
	PROPSHEETHEADER::hInstance = g_Globals._hInstance;
}

void PropertySheetDialog::add(PropSheetPage& psp)
{
	_pages.push_back(psp);
}

int	PropertySheetDialog::DoModal(int start_page)
{
	PROPSHEETHEADER::ppsp = (LPCPROPSHEETPAGE) &_pages[0];
	PROPSHEETHEADER::nPages = _pages.size();
	PROPSHEETHEADER::nStartPage = start_page;
/*
	Window* pwnd = Window::create_property_sheet(this, WINDOW_CREATOR(PropertySheetDlg), NULL);
	if (!pwnd)
		return -1;

	HWND hwndPropSheet = *pwnd;
*/
	HWND hwndPropSheet = (HWND) PropertySheet(this);
	HWND hwndparent = GetParent(hwndPropSheet);

	if (hwndparent)
		EnableWindow(hwndparent, FALSE);

	int ret = 0;
	MSG msg;

	while(GetMessage(&msg, 0, 0, 0)) {
		try {
			if (Window::pretranslate_msg(&msg))
				continue;

			if (PropSheet_IsDialogMessage(hwndPropSheet, &msg))
				continue;

			if (Window::dispatch_dialog_msg(&msg))
				continue;

			TranslateMessage(&msg);

			try {
				DispatchMessage(&msg);
			} catch(COMException& e) {
				HandleException(e, 0);
			}

			if (!PropSheet_GetCurrentPageHwnd(hwndPropSheet)) {
				ret = PropSheet_GetResult(hwndPropSheet);
				break;
			}
		} catch(COMException& e) {
			HandleException(e, 0);
		}
	}

	if (hwndparent)
		EnableWindow(hwndparent, TRUE);

	DestroyWindow(hwndPropSheet);

	return ret;
}

HWND PropertySheetDialog::GetCurrentPage()
{
	HWND hdlg = PropSheet_GetCurrentPageHwnd(_hwnd);
	return hdlg;
}


PropSheetPageDlg::PropSheetPageDlg(HWND hwnd)
 :	super(hwnd)
{
}

INT_PTR CALLBACK PropSheetPageDlg::DialogProc(HWND hwnd, UINT nmsg, WPARAM wparam, LPARAM lparam)
{
	PropSheetPageDlg* pThis = GET_WINDOW(PropSheetPageDlg, hwnd);

	if (pThis) {
		switch(nmsg) {
		  case WM_COMMAND:
			pThis->Command(LOWORD(wparam), HIWORD(wparam));
			return TRUE;	// message has been processed

		  case WM_NOTIFY:
			pThis->Notify(wparam, (NMHDR*)lparam);
			return TRUE;	// message has been processed

		  case WM_NCDESTROY:
			delete pThis;
			return TRUE;	// message has been processed

		  default:
			return pThis->WndProc(nmsg, wparam, lparam);
		}
	} else if (nmsg == WM_INITDIALOG) {
		PROPSHEETPAGE* psp = (PROPSHEETPAGE*) lparam;
		PropSheetPage* ppsp = (PropSheetPage*) psp->lParam;

		if (ppsp->_dlg_creator) {
			pThis = static_cast<PropSheetPageDlg*>(ppsp->_dlg_creator(hwnd));

			if (pThis)
				return pThis->Init(NULL);
		}
	}

	return FALSE;	// message has not been processed
}

int PropSheetPageDlg::Command(int id, int code)
{
	// override call to EndDialog in Dialog::Command();

	return FALSE;
}
