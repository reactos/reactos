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


#include "../utility/utility.h"
#include "../utility/shellclasses.h"
#include "../utility/shellbrowserimpl.h"
#include "../utility/dragdropimpl.h"
#include "../utility/window.h"

#include "../globals.h"
#include "../externals.h"
#include "../explorer_intres.h"

#include "desktop.h"
#include "../taskbar/desktopbar.h"
#include "../shell/mainframe.h"	// for MainFrame::Create()


static BOOL (WINAPI*SetShellWindow)(HWND);
static BOOL (WINAPI*SetShellWindowEx)(HWND, HWND);


BOOL IsAnyDesktopRunning()
{
	HINSTANCE hUser32 = GetModuleHandle(TEXT("user32"));

	SetShellWindow = (BOOL(WINAPI*)(HWND)) GetProcAddress(hUser32, "SetShellWindow");
	SetShellWindowEx = (BOOL(WINAPI*)(HWND,HWND)) GetProcAddress(hUser32, "SetShellWindowEx");

	return GetShellWindow() != 0;
}


static void draw_desktop_background(HWND hwnd, HDC hdc)
{
	ClientRect rect(hwnd);

	PaintDesktop(hdc);
/*
	HBRUSH bkgndBrush = CreateSolidBrush(RGB(0,32,160));	// dark blue
	FillRect(hdc, &rect, bkgndBrush);
	DeleteBrush(bkgndBrush);
*/

	rect.left = rect.right - 280;
	rect.top = rect.bottom - 56 - DESKTOPBARBAR_HEIGHT;
	rect.right = rect.left + 250;
	rect.bottom = rect.top + 40;

#include "../buildno.h"
	static const LPCTSTR BkgndText = TEXT("ReactOS ")TEXT(KERNEL_VERSION_STR)TEXT(" Explorer\nby Martin Fuchs");

	BkMode bkMode(hdc, TRANSPARENT);

	TextColor textColor(hdc, RGB(128,128,192));
	DrawText(hdc, BkgndText, -1, &rect, DT_RIGHT);

	SetTextColor(hdc, RGB(255,255,255));
	--rect.right;
	++rect.top;
	DrawText(hdc, BkgndText, -1, &rect, DT_RIGHT);
}


LRESULT	BackgroundWindow::WndProc(UINT nmsg, WPARAM wparam, LPARAM lparam)
{
	switch(nmsg) {
	  case WM_ERASEBKGND:
		PaintDesktop((HDC)wparam);
		return TRUE;

	  case WM_MBUTTONDBLCLK:
		explorer_show_frame(_hwnd, SW_SHOWNORMAL);
		break;

	  default:
		return super::WndProc(nmsg, wparam, lparam);
	}

	return 0;
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


HWND DesktopWindow::Create()
{
	IconWindowClass wcDesktop(TEXT("Progman"), IDI_REACTOS, CS_DBLCLKS);
	wcDesktop.hbrBackground = (HBRUSH)(COLOR_BACKGROUND+1);

	int width = GetSystemMetrics(SM_CXSCREEN);
	int height = GetSystemMetrics(SM_CYSCREEN);

	HWND hwndDesktop = Window::Create(WINDOW_CREATOR(DesktopWindow),
					WS_EX_TOOLWINDOW, wcDesktop, TEXT("Program Manager"), WS_POPUP|WS_VISIBLE|WS_CLIPCHILDREN,
					0, 0, width, height, 0);

	 // work around to display desktop bar in Wine
	ShowWindow(GET_WINDOW(DesktopWindow, hwndDesktop)->_desktopBar, SW_SHOW);

	 // work around for Windows NT, Win 98, ...
	 // Without this the desktop has mysteriously only a size of 800x600 pixels.
	MoveWindow(hwndDesktop, 0, 0, width, height, TRUE);

	return hwndDesktop;
}


LRESULT	DesktopWindow::Init(LPCREATESTRUCT pcs)
{
	if (super::Init(pcs))
		return 1;

	HRESULT hr = Desktop()->CreateViewObject(_hwnd, IID_IShellView, (void**)&_pShellView);
/* also possible:
	SFV_CREATE sfv_create;

	sfv_create.cbSize = sizeof(SFV_CREATE);
	sfv_create.pshf = Desktop();
	sfv_create.psvOuter = NULL;
	sfv_create.psfvcb = NULL;

	HRESULT hr = SHCreateShellFolderView(&sfv_create, &_pShellView);
*/
	HWND hWndView = 0;

	if (SUCCEEDED(hr)) {
		FOLDERSETTINGS fs;

		fs.ViewMode = FVM_ICON;
		fs.fFlags = FWF_DESKTOP|FWF_NOCLIENTEDGE|FWF_NOSCROLL|FWF_BESTFITWINDOW|FWF_SNAPTOGRID;	//|FWF_AUTOARRANGE;

		ClientRect rect(_hwnd);

		hr = _pShellView->CreateViewWindow(NULL, &fs, this, &rect, &hWndView);

		///@todo use IShellBrowser::GetViewStateStream() to restore previous view state -> see SHOpenRegStream()

		if (SUCCEEDED(hr)) {
			 // subclass shellview window
			new DesktopShellView(hWndView, _pShellView);

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

			SetWindowStyle(hwndFolderView, (GetWindowStyle(hwndFolderView)&~LVS_ALIGNLEFT)|LVS_ALIGNTOP|LVS_AUTOARRANGE);

			 // work around for Windows NT, Win 98, ...
			 // Without this the desktop has mysteriously only a size of 800x600 pixels.
			MoveWindow(hwndFolderView, 0, 0, rect.right, rect.bottom, TRUE);

			 // subclass background window
			new BackgroundWindow(hwndFolderView);
		}
	}

	if (hWndView && SetShellWindowEx)
		SetShellWindowEx(_hwnd, hWndView);
	else if (SetShellWindow)
		SetShellWindow(_hwnd);

	 // create the explorer bar
	_desktopBar = DesktopBar::Create();
	g_Globals._hwndDesktopBar = _desktopBar;

	return 0;
}


LRESULT DesktopWindow::WndProc(UINT nmsg, WPARAM wparam, LPARAM lparam)
{
	switch(nmsg) {
	  case WM_PAINT:
		draw_desktop_background(_hwnd, PaintCanvas(_hwnd));
		break;

	  case WM_LBUTTONDBLCLK:
	  case WM_RBUTTONDBLCLK:
	  case WM_MBUTTONDBLCLK:
		explorer_show_frame(_hwnd, SW_SHOWNORMAL);
		break;

	  case WM_GETISHELLBROWSER:
		return (LRESULT)static_cast<IShellBrowser*>(this);

	  case WM_DESTROY:

		///@todo use IShellBrowser::GetViewStateStream() and _pShellView->SaveViewState() to store view state
		
		if (SetShellWindow)
			SetShellWindow(0);
		break;

	  case WM_CLOSE:
		ShowExitWindowsDialog(_hwnd);
		break;

	  case WM_SYSCOMMAND:
		if (wparam == SC_TASKLIST) {
			if (_desktopBar)
				SendMessage(_desktopBar, nmsg, wparam, lparam);
		}
		goto def;

	  default: def:
		return super::WndProc(nmsg, wparam, lparam);
	}

	return 0;
}


HRESULT DesktopWindow::OnDefaultCommand(LPIDA pida)
{
	if (MainFrame::OpenShellFolders(pida, 0))
		return S_OK;

	return E_NOTIMPL;
}


DesktopShellView::DesktopShellView(HWND hwnd, IShellView* pShellView)
 :	super(hwnd),
	_pShellView(pShellView)
{
	InitDragDrop();
}

bool DesktopShellView::InitDragDrop()
{
	CONTEXT("DesktopShellView::InitDragDrop()");

	_pDropTarget = new DesktopDropTarget(_hwnd);

	if (!_pDropTarget)
		return false;

	_pDropTarget->AddRef();

	if (FAILED(RegisterDragDrop(_hwnd, _pDropTarget))) {
		_pDropTarget->Release();
		_pDropTarget = NULL;
		return false;
	}
	else
		_pDropTarget->Release();

	FORMATETC ftetc;

	ftetc.dwAspect = DVASPECT_CONTENT;
	ftetc.lindex = -1;
	ftetc.tymed = TYMED_HGLOBAL;
	ftetc.cfFormat = CF_HDROP;

	_pDropTarget->AddSuportedFormat(ftetc);

	return true;
}

LRESULT	DesktopShellView::WndProc(UINT nmsg, WPARAM wparam, LPARAM lparam)
{
	switch(nmsg) {
	  case WM_CONTEXTMENU:
		if (!DoContextMenu(GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam)))
			DoDesktopContextMenu(GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam));
		break;

	  default:
		return super::WndProc(nmsg, wparam, lparam);
	}

	return 0;
}

int DesktopShellView::Command(int id, int code)
{
	return super::Command(id, code);
}

int DesktopShellView::Notify(int id, NMHDR* pnmh)
{
	return super::Notify(id, pnmh);
}

bool DesktopShellView::DoContextMenu(int x, int y)
{
	IDataObject* selection;

	HRESULT hr = _pShellView->GetItemObject(SVGIO_SELECTION, IID_IDataObject, (void**)&selection);
	if (FAILED(hr))
		return false;

	PIDList pidList;

	hr = pidList.GetData(selection);
	if (FAILED(hr)) {
		selection->Release();
		//CHECKERROR(hr);
		return false;
	}

	LPIDA pida = pidList;
	if (!pida->cidl) {
		selection->Release();
		return false;
	}

	LPCITEMIDLIST parent_pidl = (LPCITEMIDLIST) ((LPBYTE)pida+pida->aoffset[0]);

	LPCITEMIDLIST* apidl = (LPCITEMIDLIST*) alloca(pida->cidl*sizeof(LPCITEMIDLIST));

	for(int i=pida->cidl; i>0; --i)
		apidl[i-1] = (LPCITEMIDLIST) ((LPBYTE)pida+pida->aoffset[i]);

	hr = ShellFolderContextMenu(ShellFolder(parent_pidl), _hwnd, pida->cidl, apidl, x, y);

	selection->Release();

	CHECKERROR(hr);

	return true;
}

HRESULT DesktopShellView::DoDesktopContextMenu(int x, int y)
{
	IContextMenu* pcm;

	HRESULT hr = DesktopFolder()->GetUIObjectOf(_hwnd, 0, NULL, IID_IContextMenu, NULL, (LPVOID*)&pcm);

	if (SUCCEEDED(hr)) {
		HMENU hmenu = CreatePopupMenu();

		if (hmenu) {
			hr = pcm->QueryContextMenu(hmenu, 0, FCIDM_SHVIEWFIRST, FCIDM_SHVIEWLAST-1, CMF_NORMAL|CMF_EXPLORE);

			if (SUCCEEDED(hr)) {
				AppendMenu(hmenu, MF_SEPARATOR, 0, NULL);
				AppendMenu(hmenu, 0, FCIDM_SHVIEWLAST-1, ResString(IDS_ABOUT_EXPLORER));

				UINT idCmd = TrackPopupMenu(hmenu, TPM_LEFTALIGN|TPM_RETURNCMD|TPM_RIGHTBUTTON, x, y, 0, _hwnd, NULL);

				if (idCmd == FCIDM_SHVIEWLAST-1) {
					explorer_about(_hwnd);
				} else if (idCmd) {
				  CMINVOKECOMMANDINFO cmi;

				  cmi.cbSize = sizeof(CMINVOKECOMMANDINFO);
				  cmi.fMask = 0;
				  cmi.hwnd = _hwnd;
				  cmi.lpVerb = (LPCSTR)(INT_PTR)(idCmd - FCIDM_SHVIEWFIRST);
				  cmi.lpParameters = NULL;
				  cmi.lpDirectory = NULL;
				  cmi.nShow = SW_SHOWNORMAL;
				  cmi.dwHotKey = 0;
				  cmi.hIcon = 0;

				  hr = pcm->InvokeCommand(&cmi);
				}
			}
		}

		pcm->Release();
	}

	return hr;
}
