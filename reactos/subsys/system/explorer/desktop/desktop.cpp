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
static BOOL (WINAPI*SetShellWindowEx)(HWND, HWND);


BOOL IsAnyDesktopRunning()
{
	HINSTANCE shell32 = GetModuleHandle(TEXT("user32"));

	SetShellWindow = (BOOL(WINAPI*)(HWND)) GetProcAddress(shell32, "SetShellWindow");
	SetShellWindowEx = (BOOL(WINAPI*)(HWND,HWND)) GetProcAddress(shell32, "SetShellWindowEx");

	return GetShellWindow() != 0;
}


DesktopWindow::DesktopWindow(HWND hwnd)
 :	super(hwnd)
{
	_pShellView = NULL;
}

DesktopWindow::~DesktopWindow()
{
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

		case WM_CREATE: {
			HRESULT hr = Desktop()->CreateViewObject(_hwnd, IID_IShellView, (void**)&_pShellView);

			HWND hWndView = 0;

			if (SUCCEEDED(hr)) {
				FOLDERSETTINGS fs;

				fs.ViewMode = FVM_ICON;
				fs.fFlags = FWF_DESKTOP|FWF_TRANSPARENT|FWF_NOCLIENTEDGE|FWF_NOSCROLL|FWF_BESTFITWINDOW|FWF_SNAPTOGRID;

				RECT rect;
				GetClientRect(_hwnd, &rect);

				hr = _pShellView->CreateViewWindow(NULL, &fs, this, &rect, &hWndView);

				//TODO: use IShellBrowser::GetViewStateStream() to restore previous view state

				if (SUCCEEDED(hr)) {
					_pShellView->UIActivate(SVUIA_ACTIVATE_FOCUS);

				/*
					IShellView2* pShellView2;

					hr = _pShellView->QueryInterface(IID_IShellView2, (void**)&pShellView2);

					SV2CVW2_PARAMS params;
					params.cbSize = sizeof(SV2CVW2_PARAMS);
					params.psvPrev = _pShellView;
					params.pfs = &fs;
					params.psbOwner = this;
					params.prcView = &rect;
					params.pvid = params.pvid;//@@

					hr = pShellView2->CreateViewWindow2(&params);
					params.pvid;
				*/

				/*
					IFolderView* pFolderView;

					hr = _pShellView->QueryInterface(IID_IFolderView, (void**)&pFolderView);

					if (SUCCEEDED(hr)) {
						hr = pFolderView->GetAutoArrange();
						hr = pFolderView->SetCurrentViewMode(FVM_DETAILS);
					}
				*/

					HWND hwndFolderView = ::GetNextWindow(hWndView, GW_CHILD);

					ShowWindow(hwndFolderView, SW_SHOW);
				}
			}
 
			if (hWndView && SetShellWindowEx)
				SetShellWindowEx(_hwnd, hWndView);
			else if (SetShellWindow)
				SetShellWindow(_hwnd);
			break;}

		case WM_DESTROY:

			//TODO: use IShellBrowser::GetViewStateStream() and _pShellView->SaveViewState() to store view state
			
			if (SetShellWindow)
				SetShellWindow(0);
			break;

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

	return Window::Create(WINDOW_CREATOR(DesktopWindow),
					0, (LPCTSTR)(int)desktopClass, _T("Progman"), WS_POPUP|WS_VISIBLE|WS_CLIPCHILDREN,
					0, 0, width, height, 0);
}
