/*
 * Copyright 2003, 2004, 2005 Martin Fuchs
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


#include <precomp.h>

#include "../resource.h"

#include "../taskbar/desktopbar.h"
#include "../taskbar/taskbar.h"	// for PM_GET_LAST_ACTIVE


static BOOL (WINAPI*SetShellWindow)(HWND);
static BOOL (WINAPI*SetShellWindowEx)(HWND, HWND);


#ifdef _USE_HDESK

Desktop::Desktop(HDESK hdesktop/*, HWINSTA hwinsta*/)
 :	_hdesktop(hdesktop)
//	_hwinsta(hwinsta)
{
}

Desktop::~Desktop()
{
	if (_hdesktop)
		CloseDesktop(_hdesktop);

//	if (_hwinsta)
//		CloseWindowStation(_hwinsta);

	if (_pThread.get()) {
		_pThread->Stop();
		_pThread.release();
	}
}

#endif


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

#ifdef _USE_HDESK
	DesktopPtr& desktop = (*this)[0];

	desktop = DesktopPtr(new Desktop(OpenInputDesktop(0, FALSE, DESKTOP_SWITCHDESKTOP)));
#endif
}

#ifdef _USE_HDESK

void Desktops::SwitchToDesktop(int idx)
{
	if (_current_desktop == idx)
		return;

	DesktopPtr& desktop = (*this)[idx];

	DesktopThread* pThread = NULL;

	if (desktop.get()) {
		if (desktop->_hdesktop)
			if (!SwitchDesktop(desktop->_hdesktop))
				return;
	} else {
		FmtString desktop_name(TEXT("Desktop %d"), idx);

		SECURITY_ATTRIBUTES saAttr = {sizeof(SECURITY_ATTRIBUTES), 0, TRUE};
/*
		HWINSTA hwinsta = CreateWindowStation(TEXT("ExplorerWinStation"), 0, GENERIC_ALL, &saAttr);

		if (!SetProcessWindowStation(hwinsta))
			return;
*/
		HDESK hdesktop = CreateDesktop(desktop_name, NULL, NULL, 0, GENERIC_ALL, &saAttr);
		if (!hdesktop)
			return;

		desktop = DesktopPtr(new Desktop(hdesktop/*, hwinsta*/));

		pThread = new DesktopThread(*desktop);
	}

	_current_desktop = idx;

	if (pThread) {
		desktop->_pThread = DesktopThreadPtr(pThread);
		pThread->Start();
	}
}

int DesktopThread::Run()
{
	if (!SetThreadDesktop(_desktop._hdesktop))
		return -1;

	HDESK hDesk_old = OpenInputDesktop(0, FALSE, DESKTOP_SWITCHDESKTOP);

	if (!SwitchDesktop(_desktop._hdesktop))
		return -1;

	if (!_desktop._hwndDesktop)
		_desktop._hwndDesktop = DesktopWindow::Create();

	int ret = Window::MessageLoop();

	SwitchDesktop(hDesk_old);

	return ret;
}

#else // _USE_HDESK

static BOOL CALLBACK SwitchDesktopEnumFct(HWND hwnd, LPARAM lparam)
{
	WindowSet& windows = *(WindowSet*)lparam;

	if (hwnd!=g_Globals._hwndDesktopBar && hwnd!=g_Globals._hwndDesktop)
		if (IsWindowVisible(hwnd))
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

#endif // _USE_HDESK


static BOOL CALLBACK MinimizeDesktopEnumFct(HWND hwnd, LPARAM lparam)
{
	list<MinimizeStruct>& minimized = *(list<MinimizeStruct>*)lparam;

	if (hwnd!=g_Globals._hwndDesktopBar && hwnd!=g_Globals._hwndDesktop)
		if (IsWindowVisible(hwnd) && !IsIconic(hwnd)) {
			RECT rect;

			if (GetWindowRect(hwnd,&rect))
				if (rect.right>0 && rect.bottom>0 &&
					rect.right>rect.left && rect.bottom>rect.top) {
				minimized.push_back(MinimizeStruct(hwnd, GetWindowStyle(hwnd)));
				ShowWindowAsync(hwnd, SW_MINIMIZE);
			}
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
	SetClassLongPtr(hwnd, GCL_HBRBACKGROUND, COLOR_BACKGROUND+1);

	_display_version = RegGetDWORDValue(HKEY_CURRENT_USER, TEXT("Control Panel\\Desktop"), TEXT("PaintDesktopVersion"), 1);
}

LRESULT BackgroundWindow::WndProc(UINT nmsg, WPARAM wparam, LPARAM lparam)
{
	switch(nmsg) {
	  case WM_ERASEBKGND:
		DrawDesktopBkgnd((HDC)wparam);
		return TRUE;

	  case WM_MBUTTONDBLCLK:
		/* Imagelist icons are missing if MainFrame::Create() is called directly from here!
		explorer_show_frame(SW_SHOWNORMAL); */
		PostMessage(g_Globals._hwndDesktop, nmsg, wparam, lparam);
		break;

	  case PM_DISPLAY_VERSION:
		if (lparam || wparam) {
			DWORD or_mask = wparam;
			DWORD reset_mask = LOWORD(lparam);
			DWORD xor_mask = HIWORD(lparam);
			_display_version = ((_display_version&~reset_mask) | or_mask) ^ xor_mask;
			RegSetDWORDValue(HKEY_CURRENT_USER, TEXT("Control Panel\\Desktop"), TEXT("PaintDesktopVersion"), _display_version);
			///@todo Changing the PaintDesktopVersion-Flag needs a restart of the shell -> display a message box
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
					WS_EX_TOOLWINDOW, wcDesktop, TEXT("Program Manager"), WS_POPUP|WS_VISIBLE,	//|WS_CLIPCHILDREN for SDI frames
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

	  case WM_SYSCOLORCHANGE:
		 // redraw background window
		InvalidateRect(g_Globals._hwndShellView, NULL, TRUE);

		 // forward message to shell view window to redraw icon backgrounds
		SendMessage(g_Globals._hwndShellView, WM_SYSCOLORCHANGE, wparam, lparam);
		break;

	  default: def:
		return super::WndProc(nmsg, wparam, lparam);
	}

	return 0;
}


HRESULT DesktopWindow::OnDefaultCommand(LPIDA pida)
{
#ifndef ROSSHELL	// in shell-only-mode fall through and let shell32 handle the command
	if (MainFrameBase::OpenShellFolders(pida, 0))
		return S_OK;
#endif

	return E_NOTIMPL;
}


DesktopShellView::DesktopShellView(HWND hwnd, IShellView* pShellView)
 :	super(hwnd),
	_pShellView(pShellView)
{
	_hwndListView = GetNextWindow(hwnd, GW_CHILD);

	SetWindowStyle(_hwndListView, GetWindowStyle(_hwndListView)&~LVS_ALIGNMASK);//|LVS_ALIGNTOP|LVS_AUTOARRANGE);

	 // work around for Windows NT, Win 98, ...
	 // Without this the desktop has mysteriously only a size of 800x600 pixels.
	ClientRect rect(hwnd);
	MoveWindow(_hwndListView, 0, 0, rect.right, rect.bottom, TRUE);

	 // subclass background window
	new BackgroundWindow(_hwndListView);

	_icon_algo = 1;	// default icon arrangement

	PositionIcons();
	InitDragDrop();
}


DesktopShellView::~DesktopShellView()
{
	if (FAILED(RevokeDragDrop(_hwnd)))
		assert(0);
}


bool DesktopShellView::InitDragDrop()
{
	CONTEXT("DesktopShellView::InitDragDrop()");

	DesktopDropTarget * pDropTarget = new DesktopDropTarget(_hwnd);

	if (!pDropTarget)
		return false;

	pDropTarget->AddRef();

	if (FAILED(RegisterDragDrop(_hwnd, pDropTarget))) {
		pDropTarget->Release();
		return false;
	}

	FORMATETC ftetc;

	ftetc.dwAspect = DVASPECT_CONTENT;
	ftetc.lindex = -1;
	ftetc.tymed = TYMED_HGLOBAL;
	ftetc.cfFormat = CF_HDROP;

	pDropTarget->AddSuportedFormat(ftetc);
	pDropTarget->Release();

	return true;
}

LRESULT DesktopShellView::WndProc(UINT nmsg, WPARAM wparam, LPARAM lparam)
{
	switch(nmsg) {
	  case WM_CONTEXTMENU:
		if (!DoContextMenu(GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam)))
			DoDesktopContextMenu(GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam));
		break;

	  case PM_SET_ICON_ALGORITHM:
		_icon_algo = wparam;
		PositionIcons();
		break;

	  case PM_GET_ICON_ALGORITHM:
		return _icon_algo;

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

	hr = ShellFolderContextMenu(ShellFolder(parent_pidl), _hwnd, pida->cidl, apidl, x, y, _cm_ifs);

	selection->Release();

	if (SUCCEEDED(hr))
		refresh();
	else
		CHECKERROR(hr);

	return true;
}

HRESULT DesktopShellView::DoDesktopContextMenu(int x, int y)
{
	IContextMenu* pcm;

	HRESULT hr = DesktopFolder()->GetUIObjectOf(_hwnd, 0, NULL, IID_IContextMenu, NULL, (LPVOID*)&pcm);

	if (SUCCEEDED(hr)) {
		pcm = _cm_ifs.query_interfaces(pcm);

		HMENU hmenu = CreatePopupMenu();

		if (hmenu) {
			hr = pcm->QueryContextMenu(hmenu, 0, FCIDM_SHVIEWFIRST, FCIDM_SHVIEWLAST-1, CMF_NORMAL|CMF_EXPLORE);

			if (SUCCEEDED(hr)) {
				AppendMenu(hmenu, MF_SEPARATOR, 0, NULL);
				AppendMenu(hmenu, 0, FCIDM_SHVIEWLAST-1, ResString(IDS_ABOUT_EXPLORER));

				UINT idCmd = TrackPopupMenu(hmenu, TPM_LEFTALIGN|TPM_RETURNCMD|TPM_RIGHTBUTTON, x, y, 0, _hwnd, NULL);

				_cm_ifs.reset();

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
			} else
				_cm_ifs.reset();
			DestroyMenu(hmenu);
		}

		pcm->Release();
	}

	return hr;
}


#define	ARRANGE_BORDER_DOWN	 8
#define	ARRANGE_BORDER_HV	 9
#define	ARRANGE_ROUNDABOUT	10

static const POINTS s_align_start[] = {
	{0, 0},	// left/top
	{0, 0},
	{1, 0},	// right/top
	{1, 0},
	{0, 1},	// left/bottom
	{0, 1},
	{1, 1},	// right/bottom
	{1, 1},

	{0, 0},	// left/top
	{0, 0},
	{0, 0}
};

static const POINTS s_align_dir1[] = {
	{ 0, +1},	// down
	{+1,  0},	// right
	{-1,  0},	// left
	{ 0, +1},	// down
	{ 0, -1},	// up
	{+1,  0},	// right
	{-1,  0},	// left
	{ 0, -1},	// up

	{ 0, +1},	// down
	{+1,  0},	// right
	{+1,  0}	// right
};

static const POINTS s_align_dir2[] = {
	{+1,  0},	// right
	{ 0, +1},	// down
	{ 0, +1},	// down
	{-1,  0},	// left
	{+1,  0},	// right
	{ 0, -1},	// up
	{ 0, -1},	// up
	{-1,  0},	// left

	{+1,  0},	// right
	{ 0, +1},	// down
	{ 0, +1}	// down
};

typedef pair<int,int> IconPos;
typedef map<IconPos, int> IconMap;

void DesktopShellView::PositionIcons(int dir)
{
	DWORD spacing = ListView_GetItemSpacing(_hwndListView, FALSE);

	RECT work_area;
	SystemParametersInfo(SPI_GETWORKAREA, 0, &work_area, 0);

	const POINTS& dir1 = s_align_dir1[_icon_algo];
	const POINTS& dir2 = s_align_dir2[_icon_algo];
	const POINTS& start_pos = s_align_start[_icon_algo];

	int dir_x1 = dir1.x;
	int dir_y1 = dir1.y;
	int dir_x2 = dir2.x;
	int dir_y2 = dir2.y;

	int cx = LOWORD(spacing);
	int cy = HIWORD(spacing);

	int dx1 = dir_x1 * cx;
	int dy1 = dir_y1 * cy;
	int dx2 = dir_x2 * cx;
	int dy2 = dir_y2 * cy;

	int xoffset = (cx-32)/2;
	int yoffset = 4/*(cy-32)/2*/;

	int start_x = start_pos.x * (work_area.right - cx) + xoffset;
	int start_y = start_pos.y * (work_area.bottom - cy) + yoffset;

	int x = start_x;
	int y = start_y;

	int all = ListView_GetItemCount(_hwndListView);
	int i1, i2;

	if (dir > 0) {
		i1 = 0;
		i2 = all;
	} else {
		i1 = all-1;
		i2 = -1;
	}

	IconMap pos_idx;
	int cnt = 0;
	int xhv = start_x;
	int yhv = start_y;

	for(int idx=i1; idx!=i2; idx+=dir) {
		pos_idx[IconPos(y, x)] = idx;

		if (_icon_algo == ARRANGE_BORDER_DOWN) {
			if (++cnt & 1)
				x = work_area.right - x - cx + 2*xoffset;
			else {
				y += dy1;

				if (y + cy - 2 * yoffset > work_area.bottom) {
					y = start_y;
					start_x += dx2;
					x = start_x;
				}
			}

			continue;
		}
		else if (_icon_algo == ARRANGE_BORDER_HV) {
			if (++cnt & 1)
				x = work_area.right - x - cx + 2*xoffset;
			else if (cnt & 2) {
				yhv += cy;
				y = yhv;
				x = start_x;

				if (y + cy - 2 * yoffset > work_area.bottom) {
					start_x += cx;
					xhv = start_x;
					x = xhv;
					start_y += cy;
					yhv = start_y;
					y = yhv;
				}
			} else {
				xhv += cx;
				x = xhv;
				y = start_y;

				if (x + cx - 2 * xoffset > work_area.right) {
					start_x += cx;
					xhv = start_x;
					x = xhv;
					start_y += cy;
					yhv = start_y;
					y = yhv;
				}
			}

			continue;
		}
		else if (_icon_algo == ARRANGE_ROUNDABOUT) {

			///@todo

		}

		x += dx1;
		y += dy1;

		if (x<0 || x+cx-2*xoffset>work_area.right) {
			x = start_x;
			y += dy2;
		} else if (y<0 || y+cy-2*yoffset>work_area.bottom) {
			y = start_y;
			x += dx2;
		}
	}

	 // use a little trick to get the icons where we want them to be...

	//for(IconMap::const_iterator it=pos_idx.end(); --it!=pos_idx.begin(); ) {
	//	const IconPos& pos = it->first;

	//	ListView_SetItemPosition32(_hwndListView, it->second, pos.second, pos.first);
	//}

	for(IconMap::const_iterator it=pos_idx.begin(); it!=pos_idx.end(); ++it) {
		const IconPos& pos = it->first;

		ListView_SetItemPosition32(_hwndListView, it->second, pos.second, pos.first);
	}
}


void DesktopShellView::refresh()
{
	///@todo
}
