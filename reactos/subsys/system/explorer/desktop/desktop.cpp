/*
 * Copyright 2003, 2004 Martin Fuchs
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
 // Explorer clone, lean version
 //
 // desktop.cpp
 //
 // Martin Fuchs, 09.08.2003
 //


#include "precomp.h"

#include "../taskbar/desktopbar.h"
#include "../taskbar/taskbar.h"	// for PM_GET_LAST_ACTIVE

#include "../explorer_intres.h"


static BOOL (WINAPI*SetShellWindow)(HWND);
static BOOL (WINAPI*SetShellWindowEx)(HWND, HWND);


Desktops::Desktops()
 :	_current_desktop(0)
{
}

Desktops::~Desktops()
{
	 // show all hidden windows
	for(iterator it_dsk=begin(); it_dsk!=end(); ++it_dsk)
		for(WindowSet::iterator it=it_dsk->_windows.begin(); it!=it_dsk->_windows.end(); ++it)
			ShowWindowAsync(*it, SW_SHOW);
}

void Desktops::init()
{
	resize(DESKTOP_COUNT);
}

static BOOL CALLBACK SwitchDesktopEnumFct(HWND hwnd, LPARAM lparam)
{
	WindowSet& windows = *(WindowSet*)lparam;

	if (IsWindowVisible(hwnd))
		if (hwnd!=g_Globals._hwndDesktopBar && hwnd!=g_Globals._hwndDesktop)
			windows.insert(hwnd);

	return TRUE;
}

void Desktops::SwitchToDesktop(int idx)
{
	if (_current_desktop == idx)
		return;

	Desktop& old_desktop = (*this)[_current_desktop];
	WindowSet& windows = old_desktop._windows;
	Desktop& desktop = (*this)[idx];

	windows.clear();

	 // collect window handles of all other desktops
	WindowSet other_wnds;
	for(const_iterator it1=begin(); it1!=end(); ++it1)
		for(WindowSet::const_iterator it2=it1->_windows.begin(); it2!=it1->_windows.end(); ++it2)
			other_wnds.insert(*it2);

	 // save currently visible application windows
	EnumWindows(SwitchDesktopEnumFct, (LPARAM)&windows);

	old_desktop._hwndForeground = (HWND)SendMessage(g_Globals._hwndDesktopBar, PM_GET_LAST_ACTIVE, 0, 0);

	 // hide all windows of the previous desktop
	for(WindowSet::iterator it=windows.begin(); it!=windows.end(); ++it)
		ShowWindowAsync(*it, SW_HIDE);

	 // show all windows of the new desktop
	for(WindowSet::iterator it=desktop._windows.begin(); it!=desktop._windows.end(); ++it)
		ShowWindowAsync(*it, SW_SHOW);

	if (desktop._hwndForeground)
		SetForegroundWindow(desktop._hwndForeground);

	 // remove the window handles of the other desktops from what we found on the previous desktop
	for(WindowSet::const_iterator it=other_wnds.begin(); it!=other_wnds.end(); ++it)
		windows.erase(*it);

	 // We don't need to store the window handles of what's now visible the now current desktop.
	desktop._windows.clear();

	_current_desktop = idx;
}


static BOOL CALLBACK MinimizeDesktopEnumFct(HWND hwnd, LPARAM lparam)
{
	list<MinimizeStruct>& minimized = *(list<MinimizeStruct>*)lparam;

	if (IsWindowVisible(hwnd))
		if (hwnd!=g_Globals._hwndDesktopBar && hwnd!=g_Globals._hwndDesktop)
			if (!IsIconic(hwnd)) {
				minimized.push_back(MinimizeStruct(hwnd, GetWindowStyle(hwnd)));
				ShowWindowAsync(hwnd, SW_MINIMIZE);
			}

	return TRUE;
}

 /// minimize/restore all windows on the desktop
void Desktops::ToggleMinimize()
{
	list<MinimizeStruct>& minimized = (*this)[_current_desktop]._minimized;

	if (minimized.empty()) {
		EnumWindows(MinimizeDesktopEnumFct, (LPARAM)&minimized);
	} else {
		for(list<MinimizeStruct>::const_iterator it=minimized.begin(); it!=minimized.end(); ++it)
			ShowWindowAsync(it->first, it->second&WS_MAXIMIZE? SW_MAXIMIZE: SW_RESTORE);

		minimized.clear();
	}
}


BOOL IsAnyDesktopRunning()
{
	HINSTANCE hUser32 = GetModuleHandle(TEXT("user32"));

	SetShellWindow = (BOOL(WINAPI*)(HWND)) GetProcAddress(hUser32, "SetShellWindow");
	SetShellWindowEx = (BOOL(WINAPI*)(HWND,HWND)) GetProcAddress(hUser32, "SetShellWindowEx");

	return GetShellWindow() != 0;
}


BackgroundWindow::BackgroundWindow(HWND hwnd)
 :	super(hwnd)
{
	 // set background brush for the short moment of displaying the
	 // background color while moving foreground windows
	SetClassLong(hwnd, GCL_HBRBACKGROUND, COLOR_BACKGROUND+1);

	_display_version = RegGetDWORDValue(HKEY_CURRENT_USER, TEXT("Control Panel\\Desktop"), TEXT("PaintDesktopVersion"), 1);
}

LRESULT BackgroundWindow::WndProc(UINT nmsg, WPARAM wparam, LPARAM lparam)
{
	switch(nmsg) {
	  case WM_ERASEBKGND:
		DrawDesktopBkgnd((HDC)wparam);
		return TRUE;

	  case WM_MBUTTONDBLCLK:
		explorer_show_frame(SW_SHOWNORMAL);
		break;

	  case PM_DISPLAY_VERSION:
		if (lparam || wparam) {
			DWORD or_mask = wparam;
			DWORD reset_mask = LOWORD(lparam);
			DWORD xor_mask = HIWORD(lparam);
			_display_version = ((_display_version&~reset_mask) | or_mask) ^ xor_mask;
			InvalidateRect(_hwnd, NULL, TRUE);
		}
		return _display_version;

	  default:
		return super::WndProc(nmsg, wparam, lparam);
	}

	return 0;
}

void BackgroundWindow::DrawDesktopBkgnd(HDC hdc)
{
	PaintDesktop(hdc);

/* special solid background
	HBRUSH bkgndBrush = CreateSolidBrush(RGB(0,32,160));	// dark blue
	FillRect(hdc, &rect, bkgndBrush);
	DeleteBrush(bkgndBrush);
*/
	if (_display_version) {
		static const String s_bkgnd_txt = ResString(IDS_EXPLORER_VERSION_STR) + TEXT("\nby Martin Fuchs");

		FmtString txt(s_bkgnd_txt, (LPCTSTR)ResString(IDS_VERSION_STR));
		ClientRect rect(_hwnd);

		rect.left = rect.right - 280;
		rect.top = rect.bottom - 80 - DESKTOPBARBAR_HEIGHT;
		rect.right = rect.left + 250;
		rect.bottom = rect.top + 40;

		BkMode bkMode(hdc, TRANSPARENT);

		TextColor textColor(hdc, RGB(128,128,192));
		DrawText(hdc, txt, -1, &rect, DT_RIGHT);

		SetTextColor(hdc, RGB(255,255,255));
		--rect.right;
		++rect.top;
		DrawText(hdc, txt, -1, &rect, DT_RIGHT);
	}
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
	static IconWindowClass wcDesktop(TEXT("Progman"), IDI_REACTOS, CS_DBLCLKS);
	/* (disabled because of small ugly temporary artefacts when hiding start menu)
	wcDesktop.hbrBackground = (HBRUSH)(COLOR_BACKGROUND+1); */

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

	HRESULT hr = GetDesktopFolder()->CreateViewObject(_hwnd, IID_IShellView, (void**)&_pShellView);
/* also possible:
	SFV_CREATE sfv_create;

	sfv_create.cbSize = sizeof(SFV_CREATE);
	sfv_create.pshf = GetDesktopFolder();
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
			g_Globals._hwndShellView = hWndView;

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
	  case WM_LBUTTONDBLCLK:
	  case WM_RBUTTONDBLCLK:
	  case WM_MBUTTONDBLCLK:
		explorer_show_frame(SW_SHOWNORMAL);
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
	_hwndListView = ::GetNextWindow(hwnd, GW_CHILD);

	SetWindowStyle(_hwndListView, GetWindowStyle(_hwndListView)&~LVS_ALIGNMASK);//|LVS_ALIGNTOP|LVS_AUTOARRANGE);

	 // work around for Windows NT, Win 98, ...
	 // Without this the desktop has mysteriously only a size of 800x600 pixels.
	ClientRect rect(hwnd);
	MoveWindow(_hwndListView, 0, 0, rect.right, rect.bottom, TRUE);

	 // subclass background window
	new BackgroundWindow(_hwndListView);

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

	  case PM_DISPLAY_VERSION:
		return SendMessage(_hwndListView, nmsg, wparam, lparam);

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
