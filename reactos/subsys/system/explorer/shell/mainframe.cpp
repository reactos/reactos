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
 // mainframe.cpp
 //
 // Martin Fuchs, 23.07.2003
 //


#include "../utility/utility.h"

#include "../explorer.h"
#include "../globals.h"
#include "../externals.h"

#include "../explorer_intres.h"


MainFrame::MainFrame(HWND hwnd)
 :	super(hwnd)
{
	_hMenuFrame = GetMenu(hwnd);
	_hMenuWindow = GetSubMenu(_hMenuFrame, GetMenuItemCount(_hMenuFrame)-2);

	_menu_info._hMenuView = GetSubMenu(_hMenuFrame, 3);
	_menu_info._hMenuOptions = GetSubMenu(_hMenuFrame, 4);

	_hAccel = LoadAccelerators(g_Globals._hInstance, MAKEINTRESOURCE(IDA_EXPLORER));


	CLIENTCREATESTRUCT ccs;

	ccs.hWindowMenu = _hMenuWindow;
	ccs.idFirstChild = IDW_FIRST_CHILD;

#ifndef _NO_MDI
	_hmdiclient = CreateWindowEx(0, TEXT("MDICLIENT"), NULL,
					WS_CHILD|WS_CLIPCHILDREN|WS_VSCROLL|WS_HSCROLL|WS_VISIBLE|WS_BORDER,
					0, 0, 0, 0,
					hwnd, 0, g_Globals._hInstance, &ccs);
#endif

	TBBUTTON toolbarBtns[] = {
		{0, 0, 0, BTNS_SEP, {0, 0}, 0, 0},
		{0, ID_WINDOW_NEW, TBSTATE_ENABLED, BTNS_BUTTON, {0, 0}, 0, 0},
		{1, ID_WINDOW_CASCADE, TBSTATE_ENABLED, BTNS_BUTTON, {0, 0}, 0, 0},
		{2, ID_WINDOW_TILE_HORZ, TBSTATE_ENABLED, BTNS_BUTTON, {0, 0}, 0, 0},
		{3, ID_WINDOW_TILE_VERT, TBSTATE_ENABLED, BTNS_BUTTON, {0, 0}, 0, 0},
/*TODO
		{4, ID_... , TBSTATE_ENABLED, BTNS_BUTTON, {0, 0}, 0, 0},
		{5, ID_... , TBSTATE_ENABLED, BTNS_BUTTON, {0, 0}, 0, 0},
*/	};

	_htoolbar = CreateToolbarEx(hwnd, WS_CHILD|WS_VISIBLE,
		IDW_TOOLBAR, 2, g_Globals._hInstance, IDB_TOOLBAR, toolbarBtns,
		sizeof(toolbarBtns)/sizeof(TBBUTTON), 16, 15, 16, 15, sizeof(TBBUTTON));

	CheckMenuItem(_menu_info._hMenuOptions, ID_VIEW_TOOL_BAR, MF_BYCOMMAND|MF_CHECKED);


	TBBUTTON drivebarBtn = {0, 0, TBSTATE_ENABLED, BTNS_SEP, {0, 0}, 0, 0};
	int btn = 1;
	PTSTR p;

	_hdrivebar = CreateToolbarEx(hwnd, WS_CHILD|WS_VISIBLE|CCS_NOMOVEY|TBSTYLE_LIST,
				IDW_DRIVEBAR, 2, g_Globals._hInstance, IDB_DRIVEBAR, &drivebarBtn,
				1, 16, 13, 16, 13, sizeof(TBBUTTON));
	CheckMenuItem(_menu_info._hMenuOptions, ID_VIEW_DRIVE_BAR, MF_BYCOMMAND|MF_CHECKED);


	GetLogicalDriveStrings(BUFFER_LEN, _drives);

	drivebarBtn.fsStyle = BTNS_BUTTON;

#ifdef __WINE__
	 // insert unix file system button
	SendMessage(_hdrivebar, TB_ADDSTRING, 0, (LPARAM)TEXT("/\0"));

	drivebarBtn.idCommand = ID_DRIVE_UNIX_FS;
	SendMessage(_hdrivebar, TB_INSERTBUTTON, btn++, (LPARAM)&drivebarBtn);
	++drivebarBtn.iString;
#endif

	 // insert explorer window button
	SendMessage(_hdrivebar, TB_ADDSTRING, 0, (LPARAM)TEXT("Explore\0"));

	drivebarBtn.idCommand = ID_DRIVE_DESKTOP;
	SendMessage(_hdrivebar, TB_INSERTBUTTON, btn++, (LPARAM)&drivebarBtn);
	++drivebarBtn.iString;

	 // insert shell namespace button
	SendMessage(_hdrivebar, TB_ADDSTRING, 0, (LPARAM)TEXT("Shell\0"));

	drivebarBtn.idCommand = ID_DRIVE_SHELL_NS;
	SendMessage(_hdrivebar, TB_INSERTBUTTON, btn++, (LPARAM)&drivebarBtn);
	++drivebarBtn.iString;

#define	W_VER_NT 0
	if ((HIWORD(GetVersion())>>14) == W_VER_NT) {
		 // insert NT object namespace button
		SendMessage(_hdrivebar, TB_ADDSTRING, 0, (LPARAM)TEXT("NT Obj\0"));

		drivebarBtn.idCommand = ID_DRIVE_NTOBJ_NS;
		SendMessage(_hdrivebar, TB_INSERTBUTTON, btn++, (LPARAM)&drivebarBtn);
		++drivebarBtn.iString;
	}

	 // insert Registry button
	SendMessage(_hdrivebar, TB_ADDSTRING, 0, (LPARAM)TEXT("Registry\0"));

	drivebarBtn.idCommand = ID_DRIVE_REGISTRY;
	SendMessage(_hdrivebar, TB_INSERTBUTTON, btn++, (LPARAM)&drivebarBtn);
	++drivebarBtn.iString;

	 // register windows drive root strings
	SendMessage(_hdrivebar, TB_ADDSTRING, 0, (LPARAM)_drives);

	drivebarBtn.idCommand = ID_DRIVE_FIRST;

	for(p=_drives; *p; ) {
		 // insert drive letter
		TCHAR b[3] = {tolower(*p)};
		SendMessage(_hdrivebar, TB_ADDSTRING, 0, (LPARAM)b);

		switch(GetDriveType(p)) {
			case DRIVE_REMOVABLE:	drivebarBtn.iBitmap = 1;	break;
			case DRIVE_CDROM:		drivebarBtn.iBitmap = 3;	break;
			case DRIVE_REMOTE:		drivebarBtn.iBitmap = 4;	break;
			case DRIVE_RAMDISK: 	drivebarBtn.iBitmap = 5;	break;
			default:/*DRIVE_FIXED*/ drivebarBtn.iBitmap = 2;
		}

		SendMessage(_hdrivebar, TB_INSERTBUTTON, btn++, (LPARAM)&drivebarBtn);
		++drivebarBtn.idCommand;
		++drivebarBtn.iString;

		while(*p++);
	}


	/* CreateStatusWindow does not accept WS_BORDER
		_hstatusbar = CreateWindowEx(WS_EX_NOPARENTNOTIFY, STATUSCLASSNAME, 0,
						WS_CHILD|WS_VISIBLE|WS_CLIPSIBLINGS|WS_BORDER|CCS_NODIVIDER, 0,0,0,0,
						hwnd, (HMENU)IDW_STATUSBAR, g_Globals._hInstance, 0);*/

	_hstatusbar = CreateStatusWindow(WS_CHILD|WS_VISIBLE, 0, hwnd, IDW_STATUSBAR);
	CheckMenuItem(_menu_info._hMenuOptions, ID_VIEW_STATUSBAR, MF_BYCOMMAND|MF_CHECKED);
}


MainFrame::~MainFrame()
{
	 // don't exit desktop when closing file manager window
	if (!g_Globals._desktop_mode)
		PostQuitMessage(0);
}


HWND MainFrame::Create()
{
	HMENU hMenuFrame = LoadMenu(g_Globals._hInstance, MAKEINTRESOURCE(IDM_MAINFRAME));

	return super::Create(WINDOW_CREATOR(MainFrame), 0,
				(LPCTSTR)(int)g_Globals._hframeClass, ResString(IDS_TITLE), WS_OVERLAPPEDWINDOW,
				CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
				0/*hwndDesktop*/, hMenuFrame);
}

HWND MainFrame::Create(LPCTSTR path, int mode)
{
	HWND hMainFrame = Create();
	if (!hMainFrame)
		return 0;

	ShowWindow(hMainFrame, SW_SHOW);

	MainFrame* pMainFrame = GET_WINDOW(MainFrame, hMainFrame);

	if (pMainFrame)
		pMainFrame->CreateChild(path, mode);

	return hMainFrame;
}

HWND MainFrame::Create(LPCITEMIDLIST pidl, int mode)
{
	HWND hMainFrame = Create();
	if (!hMainFrame)
		return 0;

	ShowWindow(hMainFrame, SW_SHOW);

	MainFrame* pMainFrame = GET_WINDOW(MainFrame, hMainFrame);

	if (pMainFrame)
		pMainFrame->CreateChild(pidl, mode);

	return hMainFrame;
}


ChildWindow* MainFrame::CreateChild(LPCTSTR path, int mode)
{
	return reinterpret_cast<ChildWindow*>(SendMessage(_hwnd, PM_OPEN_WINDOW, mode, (LPARAM)path));
}

ChildWindow* MainFrame::CreateChild(LPCITEMIDLIST pidl, int mode)
{
	return reinterpret_cast<ChildWindow*>(SendMessage(_hwnd, PM_OPEN_WINDOW, mode|OWM_PIDL, (LPARAM)pidl));
}


int MainFrame::OpenShellFolders(LPIDA pida, HWND hFrameWnd)
{
	int cnt = 0;

	LPCITEMIDLIST parent_pidl = (LPCITEMIDLIST) ((LPBYTE)pida+pida->aoffset[0]);
	ShellFolder folder(parent_pidl);

	for(int i=pida->cidl; i>0; --i) {
		LPCITEMIDLIST pidl = (LPCITEMIDLIST) ((LPBYTE)pida+pida->aoffset[i]);

		SFGAOF attribs = SFGAO_FOLDER;
		HRESULT hr = folder->GetAttributesOf(1, &pidl, &attribs);

		if (SUCCEEDED(hr))
			if (attribs & SFGAO_FOLDER) {
				try {
					ShellPath pidl_abs = ShellPath(pidl).create_absolute_pidl(parent_pidl);

					if (hFrameWnd) {
						if (SendMessage(hFrameWnd, PM_OPEN_WINDOW, OWM_PIDL, (LPARAM)(LPCITEMIDLIST)pidl_abs))
							++cnt;
					} else {
						if (MainFrame::Create(pidl_abs, 0))
							++cnt;
					}
				} catch(COMException& e) {
					HandleException(e, g_Globals._hMainWnd);
				}
			}/*TEST
			else { // !(attribs & SFGAO_FOLDER))
				SHELLEXECUTEINFOA shexinfo;

				shexinfo.cbSize = sizeof(SHELLEXECUTEINFOA);
				shexinfo.fMask = SEE_MASK_INVOKEIDLIST;
				shexinfo.hwnd = NULL;
				shexinfo.lpVerb = NULL;
				shexinfo.lpFile = NULL;
				shexinfo.lpParameters = NULL;
				shexinfo.lpDirectory = NULL;
				shexinfo.nShow = SW_NORMAL;
				shexinfo.lpIDList = ILCombine(parent_pidl, pidl);

				if (ShellExecuteExA(&shexinfo))
					++cnt;

				ILFree((LPITEMIDLIST)shexinfo.lpIDList);
			}*/
	}

	return cnt;
}


LRESULT MainFrame::WndProc(UINT nmsg, WPARAM wparam, LPARAM lparam)
{
	switch(nmsg) {
	  case PM_TRANSLATE_MSG: {
		MSG* pmsg = (MSG*) lparam;

#ifndef _NO_MDI
		if (_hmdiclient && TranslateMDISysAccel(_hmdiclient, pmsg))
			return TRUE;
#endif

		if (TranslateAccelerator(_hwnd, _hAccel, pmsg))
			return TRUE;

		return FALSE;}

	  case WM_CLOSE:
		DestroyWindow(_hwnd);
		g_Globals._hMainWnd = 0;
		break;

	  case WM_DESTROY:
		break;

	  case WM_SIZE:
		resize_frame(LOWORD(lparam), HIWORD(lparam));
		break;	// do not pass message to DefFrameProc

	  case WM_GETMINMAXINFO: {
		LPMINMAXINFO lpmmi = (LPMINMAXINFO)lparam;

		lpmmi->ptMaxTrackSize.x <<= 1;/*2*GetSystemMetrics(SM_CXSCREEN) / SM_CXVIRTUALSCREEN */
		lpmmi->ptMaxTrackSize.y <<= 1;/*2*GetSystemMetrics(SM_CYSCREEN) / SM_CYVIRTUALSCREEN */
		break;}

	  case PM_FRM_CALC_CLIENT:
		frame_get_clientspace((PRECT)lparam);
		return TRUE;

	  case PM_FRM_GET_MENUINFO:
		return (LPARAM)&_menu_info;

	  case PM_OPEN_WINDOW: {CONTEXT("PM_OPEN_WINDOW");
		TCHAR buffer[MAX_PATH];
		LPCTSTR path;
		ShellPath shell_path = DesktopFolderPath();

		if (lparam) {
			if (wparam & OWM_PIDL) {
				 // take over PIDL from lparam
				shell_path.assign((LPCITEMIDLIST)lparam);	// create as "rooted" window
				FileSysShellPath fsp(shell_path);
				path = fsp;

				if (path) {
					LOG(FmtString(TEXT("PM_OPEN_WINDOW: path=%s"), path));
					lstrcpy(buffer, path);
					path = buffer;
				}
			} else {
				 // take over path from lparam
				path = (LPCTSTR)lparam;
				shell_path = path;	// create as "rooted" window
			}
		} else {
			///@todo read paths and window placements from registry
			if (!GetCurrentDirectory(MAX_PATH, buffer))
				*buffer = '\0';

			path = buffer;
		}

		{
			OBJ_CONTEXT("create ShellChildWndInfo", path);

			 // Shell Namespace as default view
			ShellChildWndInfo create_info(path, shell_path);

			create_info._pos.showCmd = SW_SHOWMAXIMIZED;
			create_info._pos.rcNormalPosition.left = 0;
			create_info._pos.rcNormalPosition.top = 0;
			create_info._pos.rcNormalPosition.right = 600;
			create_info._pos.rcNormalPosition.bottom = 280;

			create_info._open_mode = (OPEN_WINDOW_MODE)wparam;

		//	FileChildWindow::create(_hmdiclient, create_info);
			return (LRESULT)ShellBrowserChild::create(_hmdiclient, create_info);
		}
		break;}

	  case PM_GET_CONTROLWINDOW:
		if (wparam == FCW_STATUS)
			return (LRESULT)(HWND)_hstatusbar;
		break;

	  default:
#ifndef _NO_MDI
		return DefFrameProc(_hwnd, _hmdiclient, nmsg, wparam, lparam);
#else
		return super::WNdProc(nmsg, wparam, lparam);
#endif
	}

	return 0;
}


int MainFrame::Command(int id, int code)
{
	CONTEXT("MainFrame::Command()");

#ifndef _NO_MDI
	HWND hwndClient = (HWND) SendMessage(_hmdiclient, WM_MDIGETACTIVE, 0, 0);

	if (SendMessage(hwndClient, PM_DISPATCH_COMMAND, MAKELONG(id,code), 0))
		return 0;
#endif

	if (id>=ID_DRIVE_FIRST && id<=ID_DRIVE_FIRST+0xFF) {
		TCHAR drv[_MAX_DRIVE], path[MAX_PATH];
		LPCTSTR root = _drives;

		for(int i=id-ID_DRIVE_FIRST; i--; root++)
			while(*root)
				++root;

		if (activate_drive_window(root))
			return 0;

		_tsplitpath(root, drv, 0, 0, 0);

		if (!SetCurrentDirectory(drv)) {
			display_error(_hwnd, GetLastError());
			return 0;
		}

		GetCurrentDirectory(MAX_PATH, path); ///@todo store last directory per drive

#ifndef _NO_MDI
		FileChildWindow::create(_hmdiclient, FileChildWndInfo(path));
#else
		///@todo SDI implementation
#endif
		return 1;
	}

	switch(id) {
	  case ID_FILE_EXIT:
		SendMessage(_hwnd, WM_CLOSE, 0, 0);
		break;

	  case ID_WINDOW_NEW: {
		TCHAR path[MAX_PATH];

		GetCurrentDirectory(MAX_PATH, path);

#ifndef _NO_MDI
		FileChildWindow::create(_hmdiclient, FileChildWndInfo(path));
#else
		///@todo SDI implementation
#endif
		break;}

#ifndef _NO_MDI
	  case ID_WINDOW_CASCADE:
		SendMessage(_hmdiclient, WM_MDICASCADE, 0, 0);
		break;

	  case ID_WINDOW_TILE_HORZ:
		SendMessage(_hmdiclient, WM_MDITILE, MDITILE_HORIZONTAL, 0);
		break;

	  case ID_WINDOW_TILE_VERT:
		SendMessage(_hmdiclient, WM_MDITILE, MDITILE_VERTICAL, 0);
		break;

	  case ID_WINDOW_ARRANGE:
		SendMessage(_hmdiclient, WM_MDIICONARRANGE, 0, 0);
		break;
#endif

	  case ID_VIEW_TOOL_BAR:
		toggle_child(_hwnd, id, _htoolbar);
		break;

	  case ID_VIEW_DRIVE_BAR:
		toggle_child(_hwnd, id, _hdrivebar);
		break;

	  case ID_VIEW_STATUSBAR:
		toggle_child(_hwnd, id, _hstatusbar);
		break;

	  case ID_EXECUTE: {
		ExecuteDialog dlg = {{0}, 0};

		if (DialogBoxParam(g_Globals._hInstance, MAKEINTRESOURCE(IDD_EXECUTE), _hwnd, ExecuteDialog::WndProc, (LPARAM)&dlg) == IDOK) {
			CONTEXT("ShellExecute()");

			HINSTANCE hinst = ShellExecute(_hwnd, NULL/*operation*/, dlg.cmd/*file*/, NULL/*parameters*/, NULL/*dir*/, dlg.cmdshow);

			if ((int)hinst <= 32)
				display_error(_hwnd, GetLastError());
		}
		break;}

	  case ID_HELP:
		WinHelp(_hwnd, TEXT("explorer")/*file explorer.hlp*/, HELP_INDEX, 0);
		break;

	  case ID_VIEW_FULLSCREEN:
		CheckMenuItem(_menu_info._hMenuOptions, id, toggle_fullscreen()?MF_CHECKED:0);
		break;

#ifdef __WINE__
	  case ID_DRIVE_UNIX_FS: {
		TCHAR path[MAX_PATH];
		FileChildWindow* child;

		if (activate_child_window(TEXT("unixfs")))
			break;

		getcwd(path, MAX_PATH);

#ifndef _NO_MDI
		FileChildWindow::create(_hmdiclient, FileChildWndInfo(path));
#else
		///@todo SDI implementation
#endif
		break;}
#endif
	  case ID_DRIVE_SHELL_NS: {
		TCHAR path[MAX_PATH];

		if (activate_child_window(TEXT("Shell")))
			break;

		GetCurrentDirectory(MAX_PATH, path);

#ifndef _NO_MDI
		FileChildWindow::create(_hmdiclient, ShellChildWndInfo(path,DesktopFolderPath()));
#else
		///@todo SDI implementation
#endif
		break;}

	  case ID_DRIVE_NTOBJ_NS: {
		if (activate_child_window(TEXT("NTOBJ")))
			break;

#ifndef _NO_MDI
		FileChildWindow::create(_hmdiclient, NtObjChildWndInfo(TEXT("\\")));
#else
		///@todo SDI implementation
#endif
	  break;}

	  case ID_DRIVE_REGISTRY: {
		if (activate_child_window(TEXT("Registry")))
			break;

#ifndef _NO_MDI
		FileChildWindow::create(_hmdiclient, RegistryChildWndInfo(TEXT("\\")));
#else
		///@todo SDI implementation
#endif
	  break;}

	  case ID_DRIVE_DESKTOP: {
		TCHAR path[MAX_PATH];

		if (activate_child_window(TEXT("Desktop")))
			break;

		GetCurrentDirectory(MAX_PATH, path);

		ShellBrowserChild::create(_hmdiclient, ShellChildWndInfo(path,DesktopFolderPath()));
		break;}

	///@todo There are even more menu items!

	  case ID_ABOUT_WINDOWS:
		ShellAbout(_hwnd, ResString(IDS_TITLE), NULL, 0);
		break;

	  case ID_ABOUT_EXPLORER:
		explorer_about(_hwnd);
		break;

	  case ID_EXPLORER_FAQ:
		launch_file(_hwnd, TEXT("http://www.sky.franken.de/explorer/"), SW_SHOWNORMAL);
		break;

	  default:
		/*@todo if (wParam >= PM_FIRST_LANGUAGE && wParam <= PM_LAST_LANGUAGE)
			STRING_SelectLanguageByNumber(wParam - PM_FIRST_LANGUAGE);
		else */if ((id<IDW_FIRST_CHILD || id>=IDW_FIRST_CHILD+0x100) &&
			(id<SC_SIZE || id>SC_RESTORE))
			MessageBox(_hwnd, TEXT("Not yet implemented"), ResString(IDS_TITLE), MB_OK);

#ifndef _NO_MDI
		return DefFrameProc(_hwnd, _hmdiclient, WM_COMMAND, MAKELONG(id,code), 0);
#else
		return 0;
#endif
	}

	return 1;
}


void MainFrame::resize_frame_rect(PRECT prect)
{
	int new_top;

	if (IsWindowVisible(_htoolbar)) {
		SendMessage(_htoolbar, WM_SIZE, 0, 0);
		ClientRect rt(_htoolbar);
		prect->top = rt.bottom+3;
		prect->bottom -= rt.bottom+3;
	}

	if (IsWindowVisible(_hdrivebar)) {
		SendMessage(_hdrivebar, WM_SIZE, 0, 0);
		ClientRect rt(_hdrivebar);
		new_top = --prect->top + rt.bottom+3;
		MoveWindow(_hdrivebar, 0, prect->top, rt.right, new_top, TRUE);
		prect->top = new_top;
		prect->bottom -= rt.bottom+2;
	}

	if (IsWindowVisible(_hstatusbar)) {
		int parts[] = {300, 500};

		SendMessage(_hstatusbar, WM_SIZE, 0, 0);
		SendMessage(_hstatusbar, SB_SETPARTS, 2, (LPARAM)&parts);
		ClientRect rt(_hstatusbar);
		prect->bottom -= rt.bottom;
	}

#ifndef _NO_MDI
	MoveWindow(_hmdiclient, prect->left-1,prect->top-1,prect->right+2,prect->bottom+1, TRUE);
#endif
}

void MainFrame::resize_frame(int cx, int cy)
{
	RECT rect;

	rect.left	= 0;
	rect.top	= 0;
	rect.right	= cx;
	rect.bottom = cy;

	resize_frame_rect(&rect);
}

void MainFrame::resize_frame_client()
{
	ClientRect rect(_hwnd);

	resize_frame_rect(&rect);
}

void MainFrame::frame_get_clientspace(PRECT prect)
{
	if (!IsIconic(_hwnd))
		GetClientRect(_hwnd, prect);
	else {
		WINDOWPLACEMENT wp;

		GetWindowPlacement(_hwnd, &wp);

		prect->left = prect->top = 0;
		prect->right = wp.rcNormalPosition.right-wp.rcNormalPosition.left-
						2*(GetSystemMetrics(SM_CXSIZEFRAME)+GetSystemMetrics(SM_CXEDGE));
		prect->bottom = wp.rcNormalPosition.bottom-wp.rcNormalPosition.top-
						2*(GetSystemMetrics(SM_CYSIZEFRAME)+GetSystemMetrics(SM_CYEDGE))-
						GetSystemMetrics(SM_CYCAPTION)-GetSystemMetrics(SM_CYMENUSIZE);
	}

	if (IsWindowVisible(_htoolbar)) {
		ClientRect rt(_htoolbar);
		prect->top += rt.bottom+2;
	}

	if (IsWindowVisible(_hdrivebar)) {
		ClientRect rt(_hdrivebar);
		prect->top += rt.bottom+2;
	}

	if (IsWindowVisible(_hstatusbar)) {
		ClientRect rt(_hstatusbar);
		prect->bottom -= rt.bottom;
	}
}

BOOL MainFrame::toggle_fullscreen()
{
	RECT rt;

	if ((_fullscreen._mode=!_fullscreen._mode)) {
		GetWindowRect(_hwnd, &_fullscreen._orgPos);
		_fullscreen._wasZoomed = IsZoomed(_hwnd);

		Frame_CalcFrameClient(_hwnd, &rt);
		ClientToScreen(_hwnd, (LPPOINT)&rt.left);
		ClientToScreen(_hwnd, (LPPOINT)&rt.right);

		rt.left = _fullscreen._orgPos.left-rt.left;
		rt.top = _fullscreen._orgPos.top-rt.top;
		rt.right = GetSystemMetrics(SM_CXSCREEN)+_fullscreen._orgPos.right-rt.right;
		rt.bottom = GetSystemMetrics(SM_CYSCREEN)+_fullscreen._orgPos.bottom-rt.bottom;

		MoveWindow(_hwnd, rt.left, rt.top, rt.right-rt.left, rt.bottom-rt.top, TRUE);
	} else {
		MoveWindow(_hwnd, _fullscreen._orgPos.left, _fullscreen._orgPos.top,
							_fullscreen._orgPos.right-_fullscreen._orgPos.left,
							_fullscreen._orgPos.bottom-_fullscreen._orgPos.top, TRUE);

		if (_fullscreen._wasZoomed)
			ShowWindow(_hwnd, WS_MAXIMIZE);
	}

	return _fullscreen._mode;
}

void MainFrame::fullscreen_move()
{
	RECT rt, pos;
	GetWindowRect(_hwnd, &pos);

	Frame_CalcFrameClient(_hwnd, &rt);
	ClientToScreen(_hwnd, (LPPOINT)&rt.left);
	ClientToScreen(_hwnd, (LPPOINT)&rt.right);

	rt.left = pos.left-rt.left;
	rt.top = pos.top-rt.top;
	rt.right = GetSystemMetrics(SM_CXSCREEN)+pos.right-rt.right;
	rt.bottom = GetSystemMetrics(SM_CYSCREEN)+pos.bottom-rt.bottom;

	MoveWindow(_hwnd, rt.left, rt.top, rt.right-rt.left, rt.bottom-rt.top, TRUE);
}


void MainFrame::toggle_child(HWND hwnd, UINT cmd, HWND hchild)
{
	BOOL vis = IsWindowVisible(hchild);

	CheckMenuItem(_menu_info._hMenuOptions, cmd, vis?MF_BYCOMMAND:MF_BYCOMMAND|MF_CHECKED);

	ShowWindow(hchild, vis?SW_HIDE:SW_SHOW);

	if (_fullscreen._mode)
		fullscreen_move();

	resize_frame_client();
}

#ifndef _NO_MDI
bool MainFrame::activate_drive_window(LPCTSTR path)
{
	TCHAR drv1[_MAX_DRIVE], drv2[_MAX_DRIVE];
	HWND child_wnd;

	_tsplitpath(path, drv1, 0, 0, 0);

	 // search for a already open window for the same drive
	for(child_wnd=::GetNextWindow(_hmdiclient,GW_CHILD); child_wnd; child_wnd=::GetNextWindow(child_wnd, GW_HWNDNEXT)) {
		FileChildWindow* child = (FileChildWindow*) SendMessage(child_wnd, PM_GET_FILEWND_PTR, 0, 0);

		if (child) {
			_tsplitpath(child->get_root()._path, drv2, 0, 0, 0);

			if (!lstrcmpi(drv2, drv1)) {
				SendMessage(_hmdiclient, WM_MDIACTIVATE, (WPARAM)child_wnd, 0);

				if (IsMinimized(child_wnd))
					ShowWindow(child_wnd, SW_SHOWNORMAL);

				return true;
			}
		}
	}

	return false;
}

bool MainFrame::activate_child_window(LPCTSTR filesys)
{
	HWND child_wnd;

	 // search for a already open window of the given file system name
	for(child_wnd=::GetNextWindow(_hmdiclient,GW_CHILD); child_wnd; child_wnd=::GetNextWindow(child_wnd, GW_HWNDNEXT)) {
		FileChildWindow* child = (FileChildWindow*) SendMessage(child_wnd, PM_GET_FILEWND_PTR, 0, 0);

		if (child) {
			if (!lstrcmpi(child->get_root()._fs, filesys)) {
				SendMessage(_hmdiclient, WM_MDIACTIVATE, (WPARAM)child_wnd, 0);

				if (IsMinimized(child_wnd))
					ShowWindow(child_wnd, SW_SHOWNORMAL);

				return true;
			}
		} else {
			ShellBrowserChild* shell_child = (ShellBrowserChild*) SendMessage(child_wnd, PM_GET_SHELLBROWSER_PTR, 0, 0);

			if (shell_child) {
				if (!lstrcmpi(shell_child->get_root()._fs, filesys)) {
					SendMessage(_hmdiclient, WM_MDIACTIVATE, (WPARAM)child_wnd, 0);

					if (IsMinimized(child_wnd))
						ShowWindow(child_wnd, SW_SHOWNORMAL);

					return true;
				}
			}
		}
	}

	return false;
}
#endif
