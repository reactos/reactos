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
 // mainframe.cpp
 //
 // Martin Fuchs, 23.07.2003
 //


#include "precomp.h"

#include "../explorer_intres.h"


MainFrame::MainFrame(HWND hwnd)
 :	super(hwnd)
{
	_hMenuFrame = GetMenu(hwnd);
	_hMenuWindow = GetSubMenu(_hMenuFrame, GetMenuItemCount(_hMenuFrame)-2);

	_menu_info._hMenuView = GetSubMenu(_hMenuFrame, 1);

	_hAccel = LoadAccelerators(g_Globals._hInstance, MAKEINTRESOURCE(IDA_EXPLORER));

	CLIENTCREATESTRUCT ccs;

	ccs.hWindowMenu = _hMenuWindow;
	ccs.idFirstChild = IDW_FIRST_CHILD;

	TBBUTTON toolbarBtns[] = {
		{0, 0, 0, BTNS_SEP, {0, 0}, 0, 0},
		{0, ID_WINDOW_NEW, TBSTATE_ENABLED, BTNS_BUTTON, {0, 0}, 0, 0},
/*TODO
		{4, ID_... , TBSTATE_ENABLED, BTNS_BUTTON, {0, 0}, 0, 0},
		{5, ID_... , TBSTATE_ENABLED, BTNS_BUTTON, {0, 0}, 0, 0},
*/
		{0, 0, 0, BTNS_SEP, {0, 0}, 0, 0},
		{7, ID_BROWSE_BACK, TBSTATE_ENABLED, BTNS_BUTTON, {0, 0}, 0, 0},
		{8, ID_BROWSE_FORWARD, TBSTATE_ENABLED, BTNS_BUTTON, {0, 0}, 0, 0},
		{9, ID_BROWSE_UP, TBSTATE_ENABLED, BTNS_BUTTON, {0, 0}, 0, 0},
	};

	_htoolbar = CreateToolbarEx(hwnd, WS_CHILD|WS_VISIBLE,
		IDW_TOOLBAR, 2, g_Globals._hInstance, IDB_TOOLBAR, toolbarBtns,
		sizeof(toolbarBtns)/sizeof(TBBUTTON), 16, 15, 16, 15, sizeof(TBBUTTON));

	CheckMenuItem(_menu_info._hMenuView, ID_VIEW_TOOL_BAR, MF_BYCOMMAND|MF_CHECKED);


	_hstatusbar = CreateStatusWindow(WS_CHILD|WS_VISIBLE, 0, hwnd, IDW_STATUSBAR);
	CheckMenuItem(_menu_info._hMenuView, ID_VIEW_STATUSBAR, MF_BYCOMMAND|MF_CHECKED);

	/* wait for PM_OPEN_WINDOW message before creating a shell view
	update_explorer_view();*/
}

void MainFrame::update_explorer_view()
{
	int split_pos = DEFAULT_SPLIT_POS;	//@@

	if (_shellBrowser.get()) {
		split_pos = _shellBrowser->_split_pos;
		delete _shellBrowser.release();
	}

	 // create explorer treeview
	if (_create_info._open_mode & OWM_EXPLORE) {
		if (!_left_hwnd) {
			ClientRect rect(_hwnd);

			_left_hwnd = CreateWindowEx(WS_EX_CLIENTEDGE, WC_TREEVIEW, NULL,
						WS_CHILD|WS_TABSTOP|WS_VISIBLE|WS_CHILD|TVS_HASLINES|TVS_LINESATROOT|TVS_HASBUTTONS|TVS_NOTOOLTIPS|TVS_SHOWSELALWAYS,
						0, rect.top, split_pos-SPLIT_WIDTH/2, rect.bottom-rect.top,
						_hwnd, (HMENU)IDC_FILETREE, g_Globals._hInstance, 0);

			 // display tree window as long as the shell view is not yet visible
			resize_frame_rect(&rect);
			MoveWindow(_left_hwnd, rect.left, rect.top, rect.right-rect.left, rect.bottom-rect.top, TRUE);
		}
	} else {
		if (_left_hwnd) {
			DestroyWindow(_left_hwnd);
			_left_hwnd = 0;
		}
	}

	_shellBrowser = auto_ptr<ShellBrowserChild>(new ShellBrowserChild(_hwnd, _left_hwnd, _right_hwnd, _create_info));

	 // update _shellBrowser->_clnt_rect
	ClientRect rect(_hwnd);
	resize_frame(rect.right, rect.bottom);
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
				(LPCTSTR)(int)g_Globals._hframeClass, ResString(IDS_TITLE), WS_OVERLAPPEDWINDOW|WS_CLIPCHILDREN,
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
		pMainFrame->jump_to(path, mode);

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
		pMainFrame->jump_to(pidl, mode);

	return hMainFrame;
}


void MainFrame::jump_to(LPCTSTR path, int mode)
{
	if (_shellBrowser.get() && (_create_info._open_mode&~OWM_PIDL)==(mode&~OWM_PIDL)) {
		_shellBrowser->jump_to(path);

		_create_info._shell_path = path;
	} else {
		_create_info._open_mode = mode;
		_create_info._shell_path = path;
		_create_info._root_shell_path = SpecialFolderPath(CSIDL_DRIVES, _hwnd);	//@@

		update_explorer_view();
	}
}

void MainFrame::jump_to(LPCITEMIDLIST path, int mode)
{
	if (_shellBrowser.get() && (_create_info._open_mode&~OWM_PIDL)==(mode&~OWM_PIDL)) {
		ShellPath shell_path = path;

		_shellBrowser->jump_to(shell_path);

		_create_info._shell_path = shell_path;
	} else {
		_create_info._open_mode = mode;
		_create_info._shell_path = path;
		_create_info._root_shell_path = path;//DesktopFolderPath();	//@@

		update_explorer_view();
	}
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
			}
	}

	return cnt;
}


LRESULT MainFrame::WndProc(UINT nmsg, WPARAM wparam, LPARAM lparam)
{
	switch(nmsg) {
	  case PM_TRANSLATE_MSG: {
		MSG* pmsg = (MSG*) lparam;

		if (TranslateAccelerator(_hwnd, _hAccel, pmsg))
			return TRUE;

		return FALSE;}

	  case WM_ERASEBKGND:	// draw empty background as long as no view is visible
		FillRect((HDC)wparam, ClientRect(_hwnd), GetSysColorBrush(COLOR_BTNFACE));
		return TRUE;

	  case WM_CLOSE:
		DestroyWindow(_hwnd);
		g_Globals._hMainWnd = 0;
		break;

	  case WM_DESTROY:
		break;

	  case WM_SIZE:
		resize_frame(LOWORD(lparam), HIWORD(lparam));
		break;

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

		jump_to(shell_path, (OPEN_WINDOW_MODE)wparam);
		return TRUE;}	// success

	  case PM_GET_CONTROLWINDOW:
		if (wparam == FCW_STATUS)
			return (LRESULT)(HWND)_hstatusbar;
		break;


	  default:
		if (_shellBrowser.get())
			return _shellBrowser->WndProc(nmsg, wparam, lparam);
		else
			return super::WndProc(nmsg, wparam, lparam);
	}

	return 0;
}


int MainFrame::Notify(int id, NMHDR* pnmh)
{
	if (_shellBrowser.get())
		return _shellBrowser->Notify(id, pnmh);

	return super::Notify(id, pnmh);
}


int MainFrame::Command(int id, int code)
{
	CONTEXT("MainFrame::Command()");

	if (_shellBrowser.get())
		if (!_shellBrowser->Command(id, code))
			return 0;

	switch(id) {
	  case ID_FILE_EXIT:
		SendMessage(_hwnd, WM_CLOSE, 0, 0);
		break;

	  case ID_WINDOW_NEW: {
		TCHAR path[MAX_PATH];

		GetCurrentDirectory(MAX_PATH, path);

		MainFrame::Create(path);
		break;}

	  case ID_VIEW_TOOL_BAR:
		toggle_child(_hwnd, id, _htoolbar);
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

	///@todo There are even more menu items!

	  case ID_ABOUT_WINDOWS:
		ShellAbout(_hwnd, ResString(IDS_TITLE), NULL, 0);
		break;

	  case ID_ABOUT_EXPLORER:
		explorer_about(_hwnd);
		break;

	  case ID_EXPLORER_FAQ:
		launch_file(_hwnd, TEXT("http://www.sky.franken.de/explorer/"), SW_SHOW);
		break;

	  default:
		return 1;
	}

	return 0;
}


void MainFrame::resize_frame_rect(PRECT prect)
{
	if (IsWindowVisible(_htoolbar)) {
		SendMessage(_htoolbar, WM_SIZE, 0, 0);
		ClientRect rt(_htoolbar);
		prect->top = rt.bottom+2;
//		prect->bottom -= rt.bottom+2;
	}

	if (IsWindowVisible(_hstatusbar)) {
		int parts[] = {300, 500};

		SendMessage(_hstatusbar, WM_SIZE, 0, 0);
		SendMessage(_hstatusbar, SB_SETPARTS, 2, (LPARAM)&parts);
		ClientRect rt(_hstatusbar);
		prect->bottom -= rt.bottom;
	}
}

void MainFrame::resize_frame(int cx, int cy)
{
	RECT rect;

	rect.left	= 0;
	rect.top	= 0;
	rect.right	= cx;
	rect.bottom = cy;

	resize_frame_rect(&rect);

	if (_shellBrowser.get()) {
		_shellBrowser->_clnt_rect = rect;
		_shellBrowser->resize_children();
	}
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

	if (IsWindowVisible(_hstatusbar)) {
		ClientRect rt(_hstatusbar);
		prect->bottom -= rt.bottom;
	}
}

void MainFrame::toggle_child(HWND hwnd, UINT cmd, HWND hchild)
{
	BOOL vis = IsWindowVisible(hchild);

	CheckMenuItem(_menu_info._hMenuView, cmd, vis?MF_BYCOMMAND:MF_BYCOMMAND|MF_CHECKED);

	ShowWindow(hchild, vis?SW_HIDE:SW_SHOW);

	ClientRect rect(_hwnd);
	resize_frame(rect.right, rect.bottom);
}


BOOL CALLBACK ExecuteDialog::WndProc(HWND hwnd, UINT nmsg, WPARAM wparam, LPARAM lparam)
{
	static struct ExecuteDialog* dlg;

	switch(nmsg) {
	  case WM_INITDIALOG:
		dlg = (struct ExecuteDialog*) lparam;
		return 1;

	  case WM_COMMAND: {
		int id = (int)wparam;

		if (id == IDOK) {
			GetWindowText(GetDlgItem(hwnd, 201), dlg->cmd, MAX_PATH);
			dlg->cmdshow = Button_GetState(GetDlgItem(hwnd,214))&BST_CHECKED?
											SW_SHOWMINIMIZED: SW_SHOWNORMAL;
			EndDialog(hwnd, id);
		} else if (id == IDCANCEL)
			EndDialog(hwnd, id);

		return 1;}
	}

	return 0;
}
