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
 // desktop.cpp
 //
 // Martin Fuchs, 09.08.2003
 //


#include "desktop.h"

#include "../externals.h"


static BOOL (WINAPI*SetShellWindow)(HWND);


BOOL IsAnyDesktopRunning()
{
	HINSTANCE shell32 = GetModuleHandle(TEXT("user32"));

	SetShellWindow = (BOOL(WINAPI*)(HWND)) GetProcAddress(shell32, "SetShellWindow");

	return GetShellWindow() != 0;
}


DesktopWindow::DesktopWindow(HWND hwnd)
 :	super(hwnd)
{
	_pShellView = NULL;

	if (SetShellWindow)
		SetShellWindow(hwnd);
}

DesktopWindow::~DesktopWindow()
{
	if (SetShellWindow)
		SetShellWindow(0);

	if (_pShellView)
		_pShellView->Release();
}


LRESULT DesktopWindow::WndProc(UINT nmsg, WPARAM wparam, LPARAM lparam)
{
	switch(nmsg) {
		case WM_PAINT: {
			// We'd want to draw the desktop wallpaper here. Need to
			// maintain a copy of the wallpaper in an off-screen DC and then
			// bitblt (or stretchblt?) it to the screen appropriately. For
			// now, though, we'll just draw some text.

			PAINTSTRUCT ps;
			HDC DesktopDC = BeginPaint(_hwnd, &ps);

			static const TCHAR Text [] = TEXT("ReactOS 0.1.2 Desktop Example\nby Silver Blade, Martin Fuchs");

			RECT rect;
			GetClientRect(_hwnd, &rect);

			// This next part could be improved by working out how much
			// space the text actually needs...

			rect.left = rect.right - 260;
			rect.top = rect.bottom - 80;
			rect.right = rect.left + 250;
			rect.bottom = rect.top + 40;

			SetTextColor(DesktopDC, 0x00ffffff);
			SetBkMode(DesktopDC, TRANSPARENT);
			DrawText(DesktopDC, Text, -1, &rect, DT_RIGHT);

			EndPaint(_hwnd, &ps);
			break;}

		case WM_LBUTTONDBLCLK:
			explorer_show_frame(_hwnd, SW_SHOWNORMAL);
			break;

		case WM_GETISHELLBROWSER:
			return (LRESULT)static_cast<IShellBrowser*>(this);

		case WM_CLOSE:
			break;	// Over-ride close. We need to close desktop some other way.

		default:
			return super::WndProc(nmsg, wparam, lparam);
	}

	return 0;
}


HWND create_desktop_window(HINSTANCE hInstance)
{
	WindowClass wcDesktop(_T("Program Manager"));

	wcDesktop.style 		= CS_DBLCLKS;
	wcDesktop.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	wcDesktop.hIcon			= LoadIcon(NULL, IDI_APPLICATION);
	wcDesktop.hCursor		= LoadCursor(NULL, IDC_ARROW);


	ATOM desktopClass = wcDesktop.Register();

	int width = GetSystemMetrics(SM_CXSCREEN);
	int height = GetSystemMetrics(SM_CYSCREEN);

	HWND hwndDesktop = Window::Create(WINDOW_CREATOR(DesktopWindow),
					0, (LPCTSTR)desktopClass, _T("Progman"), WS_POPUP|WS_VISIBLE|WS_CLIPCHILDREN,
					0, 0, width, height, 0);

	if (!hwndDesktop)
		return 0;


	ShellFolder folder;

	IShellView* pShellView;
	HRESULT hr = folder->CreateViewObject(hwndDesktop, IID_IShellView, (void**)&pShellView);

	HWND hWndListView = 0;

	if (SUCCEEDED(hr)) {
		FOLDERSETTINGS fs;

		fs.ViewMode = 0;
		fs.fFlags = FVM_ICON;

		RECT rect = {0, 0, width, height};

		DesktopWindow* shell_browser = static_cast<DesktopWindow*>(Window::get_window(hwndDesktop));

		hr = pShellView->CreateViewWindow(NULL, &fs, shell_browser, &rect, &hWndListView);

		if (SUCCEEDED(hr)) {
			HWND hwndFolderView = GetNextWindow(hWndListView, GW_CHILD);

			ShowWindow(hwndFolderView, SW_SHOW);
		}
	}

	return hwndDesktop;
}
