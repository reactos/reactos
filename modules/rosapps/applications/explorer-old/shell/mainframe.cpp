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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */


 //
 // Explorer clone
 //
 // mainframe.cpp
 //
 // Martin Fuchs, 23.07.2003
 //


#include <precomp.h>

/* We can't include webchild.h here - otherwise MinGW produces errors like: "multiple definition of `QACONTAINERFLAGS'"
#include "webchild.h"
*/
extern HWND create_webchildwindow(const WebChildWndInfo& info);

#include "../dialogs/settings.h"	// for MdiSdiDlg

//#define _NO_REBAR

HWND MainFrameBase::Create(const ExplorerCmd& cmd)
{
	HWND hFrame;

#ifndef _NO_MDI
	if (cmd._mdi)
		hFrame = MDIMainFrame::Create();
	else
#endif
		hFrame = SDIMainFrame::Create();

	if (hFrame) {
		HWND hwndOld = g_Globals._hMainWnd;

		g_Globals._hMainWnd = hFrame;

		if (hwndOld)
			DestroyWindow(hwndOld);

		ShowWindow(hFrame, cmd._cmdShow);
		UpdateWindow(hFrame);

		 // Open the first child window after initializing the application
		if (cmd.IsValidPath()) {
			 // We use the static s_path variable to store the path string in order
			 // to avoid accessing prematurely freed memory in the PostMessage handlers.
			static String s_path = cmd._path;

			PostMessage(hFrame, PM_OPEN_WINDOW, cmd._flags, (LPARAM)(LPCTSTR)s_path);
		} else
			PostMessage(hFrame, PM_OPEN_WINDOW, OWM_EXPLORE|OWM_DETAILS, 0);
	}

	return hFrame;
}


int MainFrameBase::OpenShellFolders(LPIDA pida, HWND hFrameWnd)
{
	int cnt = 0;

	LPCITEMIDLIST parent_pidl = (LPCITEMIDLIST) ((LPBYTE)pida+pida->aoffset[0]);
	ShellFolder folder(parent_pidl);
	LOG(FmtString(TEXT("MainFrameBase::OpenShellFolders(): parent_pidl=%s"), (LPCTSTR)FileSysShellPath(parent_pidl)));

	for(int i=pida->cidl; i>0; --i) {
		LPCITEMIDLIST pidl = (LPCITEMIDLIST) ((LPBYTE)pida+pida->aoffset[i]);

		SFGAOF attribs = SFGAO_FOLDER;
		HRESULT hr = folder->GetAttributesOf(1, &pidl, &attribs);

		if (SUCCEEDED(hr))
			if (attribs & SFGAO_FOLDER) {
				try {
					XMLPos explorer_options = g_Globals.get_cfg("general/explorer");

					bool mdi = XMLBool(explorer_options, "mdi", true);
					bool separateFolders = XMLBool(explorer_options, "separate-folders", false);

					ShellPath pidl_abs = ShellPath(pidl).create_absolute_pidl(parent_pidl);
					LOG(FmtString(TEXT("MainFrameBase::OpenShellFolders(): pidl_abs=%s"), (LPCTSTR)FileSysShellPath(pidl_abs)));

					if (hFrameWnd && (mdi || !separateFolders)) {
						int flags = OWM_PIDL;

						if (separateFolders)
							flags |= OWM_SEPARATE;

						if (SendMessage(hFrameWnd, PM_OPEN_WINDOW, flags, (LPARAM)(LPCITEMIDLIST)pidl_abs))
							++cnt;
					} else {
						HWND hwnd;
#ifndef _NO_MDI
						if (mdi)
							hwnd = MDIMainFrame::Create(pidl_abs, 0);
						else
#endif
							hwnd = SDIMainFrame::Create(pidl_abs, 0);

						if (hwnd)
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


MainFrameBase::MainFrameBase(HWND hwnd)
 :	super(hwnd)
{
    HDC hDC = GetDC(NULL);
#ifndef _NO_REBAR
	//static TCHAR Title1[] = TEXT("Toolbar");
	static TCHAR Title2[] = TEXT("Address :");
#endif

    if (hDC)
    {
        DWORD ilMask;
        INT bpp = GetDeviceCaps(hDC, BITSPIXEL);
        ReleaseDC(NULL, hDC);

        if (bpp <= 4)
            ilMask = ILC_COLOR4;
        else if (bpp <= 8)
            ilMask = ILC_COLOR8;
        else if (bpp <= 16)
            ilMask = ILC_COLOR16;
        else if (bpp <= 24)
            ilMask = ILC_COLOR24;
        else if (bpp <= 32)
            ilMask = ILC_COLOR32;
        else
            ilMask = ILC_COLOR;

        ilMask |= ILC_MASK;

        _himl = ImageList_Create(GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), ilMask, 2, 0);
    }

	_hMenuFrame = GetMenu(hwnd);
	_hMenuWindow = GetSubMenu(_hMenuFrame, GetMenuItemCount(_hMenuFrame)-3);

	_menu_info._hMenuView = GetSubMenu(_hMenuFrame, 1);

	_hAccel = LoadAccelerators(g_Globals._hInstance, MAKEINTRESOURCE(IDA_EXPLORER));


	TBBUTTON toolbarBtns[] = {
#ifdef _NO_REBAR
		{0, 0, 0, BTNS_SEP, {0, 0}, 0, 0},
#endif
		{7, ID_GO_BACK, TBSTATE_ENABLED, BTNS_BUTTON, {0, 0}, 0, 0},
		{8, ID_GO_FORWARD, TBSTATE_ENABLED, BTNS_BUTTON, {0, 0}, 0, 0},
		{9, ID_GO_UP, TBSTATE_ENABLED, BTNS_BUTTON, {0, 0}, 0, 0},
		{10, ID_GO_HOME, TBSTATE_ENABLED, BTNS_BUTTON, {0, 0}, 0, 0},
		{11, ID_GO_SEARCH, TBSTATE_ENABLED, BTNS_BUTTON, {0, 0}, 0, 0},
		{12, ID_REFRESH, TBSTATE_ENABLED, BTNS_BUTTON, {0, 0}, 0, 0},
		{13, ID_STOP, TBSTATE_ENABLED, BTNS_BUTTON, {0, 0}, 0, 0}
	};

	_htoolbar = CreateToolbarEx(hwnd,
#ifndef _NO_REBAR
		CCS_NOPARENTALIGN|CCS_NORESIZE|CCS_NODIVIDER|
#endif
		WS_CHILD|TBSTYLE_FLAT|WS_VISIBLE, IDW_TOOLBAR, 2, g_Globals._hInstance, IDB_TOOLBAR,
		toolbarBtns, sizeof(toolbarBtns)/sizeof(TBBUTTON),
		16, 16, 16, 16, sizeof(TBBUTTON));

	CheckMenuItem(_menu_info._hMenuView, ID_VIEW_TOOL_BAR, MF_BYCOMMAND|MF_CHECKED);


	 // address bar 
	_haddrcombo = CreateWindowEx(0,
								WC_COMBOBOX,
								TEXT("Address"),
								WS_CHILD|WS_TABSTOP|WS_BORDER|WS_VISIBLE|CBS_DROPDOWN|
								CCS_NOPARENTALIGN|CCS_NORESIZE|CCS_NODIVIDER|CCS_NOMOVEY,
								0, 0, 0, 0,
								hwnd,
								(HMENU)IDW_ADDRESSBAR,
								g_Globals._hInstance,
								0);

	HFONT hFont = (HFONT)GetStockObject(ANSI_VAR_FONT);
	SendMessageW(_haddrcombo, WM_SETFONT, (WPARAM)hFont, 0);


	_hstatusbar = CreateStatusWindow(WS_CHILD|WS_VISIBLE, 0, hwnd, IDW_STATUSBAR);
	CheckMenuItem(_menu_info._hMenuView, ID_VIEW_STATUSBAR, MF_BYCOMMAND|MF_CHECKED);

	_hsidebar = CreateWindowEx(WS_EX_STATICEDGE, WC_TREEVIEW, TEXT("Sidebar"),
					WS_CHILD|WS_TABSTOP|WS_BORDER|/*WS_VISIBLE|*/TVS_HASLINES|TVS_HASBUTTONS|TVS_SHOWSELALWAYS|TVS_INFOTIP,
					-1, -1, 200, 0, _hwnd, (HMENU)IDW_SIDEBAR, g_Globals._hInstance, 0);

	_himl_old = TreeView_SetImageList(_hsidebar, _himl, TVSIL_NORMAL);

	CheckMenuItem(_menu_info._hMenuView, ID_VIEW_SIDE_BAR, MF_BYCOMMAND|MF_UNCHECKED/*MF_CHECKED*/);


	 // create rebar window to manage toolbar and drivebar
#ifndef _NO_REBAR
	_hwndrebar = CreateWindowEx(WS_EX_TOOLWINDOW,
								REBARCLASSNAME,
								NULL,
								WS_CHILD|WS_VISIBLE|WS_BORDER|WS_CLIPSIBLINGS|WS_CLIPCHILDREN|
								 RBS_VARHEIGHT|RBS_DBLCLKTOGGLE| RBS_AUTOSIZE|RBS_BANDBORDERS,
								0, 0, 0, 0,
								_hwnd,
								NULL,
								g_Globals._hInstance,
								0);

	int btn_hgt = HIWORD(SendMessage(_htoolbar, TB_GETBUTTONSIZE, 0, 0));

	REBARBANDINFO rbBand;

	rbBand.cbSize = sizeof(REBARBANDINFO);
	rbBand.fMask  = RBBIM_TEXT|RBBIM_STYLE|RBBIM_CHILD|RBBIM_CHILDSIZE|RBBIM_SIZE;
	rbBand.fStyle = RBBS_CHILDEDGE|RBBS_GRIPPERALWAYS|RBBS_HIDETITLE;

	rbBand.cxMinChild = 0;
	rbBand.cyMinChild = 0;
	rbBand.cyChild = 0;
	rbBand.cyMaxChild = 0;
	rbBand.cyIntegral = btn_hgt;

	rbBand.lpText = NULL;//Title1
	rbBand.hwndChild = _htoolbar;
	rbBand.cxMinChild = 0;
	rbBand.cyMinChild = btn_hgt;
	rbBand.cx = 284;
	SendMessage(_hwndrebar, RB_INSERTBAND, (WPARAM)0, (LPARAM)&rbBand);

	rbBand.fStyle &= ~RBBS_HIDETITLE;
	rbBand.fStyle |= RBBS_BREAK;
    rbBand.lpText = Title2;
	rbBand.hwndChild = _haddrcombo;
	rbBand.cxMinChild = 0;
	rbBand.cyMinChild = btn_hgt;
	rbBand.cx = 400;
	SendMessage(_hwndrebar, RB_INSERTBAND, (WPARAM)3, (LPARAM)&rbBand);
#endif
}


MainFrameBase::~MainFrameBase()
{
	(void)TreeView_SetImageList(_hsidebar, _himl_old, TVSIL_NORMAL);
	ImageList_Destroy(_himl);

	 // don't exit desktop when closing file manager window
	if (!g_Globals._desktop_mode)
		if (g_Globals._hMainWnd == _hwnd)	// don't quit when switching between MDI and SDI mode
			PostQuitMessage(0);
}


LRESULT MainFrameBase::WndProc(UINT nmsg, WPARAM wparam, LPARAM lparam)
{
	LRESULT res;

	if (ProcessMessage(nmsg, wparam, lparam, &res))
		return res;
	else
		return super::WndProc(nmsg, wparam, lparam);
}

bool MainFrameBase::ProcessMessage(UINT nmsg, WPARAM wparam, LPARAM lparam, LRESULT* pres)
{
	switch(nmsg) {
	  case PM_TRANSLATE_MSG:
		*pres = TranslateMsg((MSG*)lparam);
		return true;

	  case WM_SHOWWINDOW:
		if (wparam) {	// trigger child resizing after window creation - now we can succesfully call IsWindowVisible()
            int height = SendMessage(_hwndrebar, RB_GETBARHEIGHT, 0, 0);
    		MoveWindow(_hwndrebar, 0, 0, LOWORD(lparam), height, TRUE);
			resize_frame_client();
        }
		return false; // goto def;

	  case WM_CLOSE:
		DestroyWindow(_hwnd);
		g_Globals._hMainWnd = 0;
		break;

	  case WM_DESTROY:
		break;

	  case WM_SIZE: {
		resize_frame(LOWORD(lparam), HIWORD(lparam));
		SendMessage(_hwndrebar, WM_SIZE, 0, 0);
		break;}	// do not pass message to DefFrameProc

	  case WM_GETMINMAXINFO: {
		LPMINMAXINFO lpmmi = (LPMINMAXINFO)lparam;

		lpmmi->ptMaxTrackSize.x <<= 1;/*2*GetSystemMetrics(SM_CXSCREEN) / SM_CXVIRTUALSCREEN */
		lpmmi->ptMaxTrackSize.y <<= 1;/*2*GetSystemMetrics(SM_CYSCREEN) / SM_CYVIRTUALSCREEN */
		break;}

	  case PM_FRM_CALC_CLIENT:
		frame_get_clientspace((PRECT)lparam);
		*pres = TRUE;
		return true;

	  case PM_FRM_GET_MENUINFO:
		*pres = (LPARAM)&_menu_info;
		return true;

	  case PM_GET_CONTROLWINDOW:
		if (wparam == FCW_STATUS) {
			*pres = (LRESULT)(HWND)_hstatusbar;
			return true;
		}
		break;

	  case PM_SETSTATUSTEXT:
		SendMessage(_hstatusbar, SB_SETTEXT, 0, lparam);
		break;

	  case WM_SYSCOLORCHANGE:
		SendMessage(_hwndrebar, WM_SYSCOLORCHANGE, 0, 0);
		SendMessage(_htoolbar, WM_SYSCOLORCHANGE, 0, 0);
		break;

	  default:
		return false;
	}

	*pres = 0;
	return true;
}

BOOL MainFrameBase::TranslateMsg(MSG* pmsg)
{
	if (TranslateAccelerator(_hwnd, _hAccel, pmsg))
		return TRUE;

	return FALSE;
}


int MainFrameBase::Command(int id, int code)
{
	CONTEXT("MainFrameBase::Command()");

	switch(id) {
	  case ID_FILE_EXIT:
		SendMessage(_hwnd, WM_CLOSE, 0, 0);
		break;

	  case ID_VIEW_TOOL_BAR:
		toggle_child(_hwnd, id, _htoolbar, 0);
		break;

	  case ID_VIEW_STATUSBAR:
		toggle_child(_hwnd, id, _hstatusbar);
		break;

	  case ID_VIEW_SIDE_BAR:
		 // lazy initialization
		if (!TreeView_GetCount(_hsidebar))
			FillBookmarks();

		toggle_child(_hwnd, id, _hsidebar);
		break;

	  case ID_EXECUTE: {
		ExecuteDialog dlg = {{0}, 0};

		if (DialogBoxParam(g_Globals._hInstance, MAKEINTRESOURCE(IDD_EXECUTE), _hwnd, ExecuteDialog::WndProc, (LPARAM)&dlg) == IDOK) {
			CONTEXT("ID_EXECUTE - ShellExecute()");

			HINSTANCE hinst = ShellExecute(_hwnd, NULL/*operation*/, dlg.cmd/*file*/, NULL/*parameters*/, NULL/*dir*/, dlg.cmdshow);

			if ((INT_PTR)hinst <= 32)
				display_error(_hwnd, GetLastError());
		}
		break;}

	  case ID_HELP:
		WinHelp(_hwnd, TEXT("explorer")/*file explorer.hlp*/, HELP_INDEX, 0);
		break;

	  case ID_VIEW_FULLSCREEN:
		CheckMenuItem(_menu_info._hMenuView, id, toggle_fullscreen()?MF_CHECKED:0);
		break;

	  case ID_TOOLS_OPTIONS:
		Dialog::DoModal(IDD_MDI_SDI, WINDOW_CREATOR(MdiSdiDlg), _hwnd);
		break;

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
		return 1;	// no command handlers in Window::Command()
	}

	return 0;
}

int MainFrameBase::Notify(int id, NMHDR* pnmh)
{
	switch(pnmh->code) {
		 // resize children windows when the rebar size changes

      case RBN_AUTOSIZE:
		resize_frame_client();
		break;

	  case TVN_GETINFOTIP: {
		NMTVGETINFOTIP* pnmgit = (NMTVGETINFOTIP*)pnmh;

		if (pnmgit->lParam) {
			const BookmarkNode& node = *(BookmarkNode*)pnmgit->lParam;

			if (node._type == BookmarkNode::BMNT_FOLDER) {
				 // display tooltips for bookmark folders
				if (!node._pfolder->_description.empty())
					lstrcpyn(pnmgit->pszText, node._pfolder->_description.c_str(), pnmgit->cchTextMax);
			} else if (node._type == BookmarkNode::BMNT_BOOKMARK) {
				 // display tooltips for bookmark folders
				String txt = node._pbookmark->_description;

				if (!node._pbookmark->_url.empty()) {
					if (!txt.empty())
						txt += TEXT("  -  ");

					txt += node._pbookmark->_url;
				}

				lstrcpyn(pnmgit->pszText, txt.c_str(), pnmgit->cchTextMax);
			}
		}
		break;}

	  case NM_DBLCLK: {
		HTREEITEM hitem = TreeView_GetSelection(_hsidebar);
		LPARAM lparam = TreeView_GetItemData(_hsidebar, hitem);

		if (lparam) {
			const BookmarkNode& node = *(BookmarkNode*)lparam;

			if (node._type == BookmarkNode::BMNT_BOOKMARK) {
				bool new_window = GetAsyncKeyState(VK_SHIFT)<0;

				go_to(node._pbookmark->_url, new_window);
			}
		}
		break;}
	}

	return 0;
}


void MainFrameBase::resize_frame(int cx, int cy)
{
	if (cy <= 0)
		return;	// avoid resizing children when receiving RBN_AUTOSIZE while getting minimized

	RECT rect = {0, 0, cx, cy};

	if (_hwndrebar) {
        int height = SendMessage(_hwndrebar, RB_GETBARHEIGHT, 0, 0);
		rect.top += height;
        rect.top += 5;

		SetWindowPos(_haddrcombo, NULL, 0, 0, cx, height, SWP_NOMOVE);
	} else {
		if (IsWindowVisible(_htoolbar)) {
			SendMessage(_htoolbar, WM_SIZE, 0, 0);
			WindowRect rt(_htoolbar);
			rect.top = rt.bottom;
		//	rect.bottom -= rt.bottom;
		}
	}

	if (IsWindowVisible(_hstatusbar)) {
		int parts[] = {300, 500};

		SendMessage(_hstatusbar, WM_SIZE, 0, 0);
		SendMessage(_hstatusbar, SB_SETPARTS, 2, (LPARAM)&parts);
		ClientRect rt(_hstatusbar);
		rect.bottom -= rt.bottom;
	}

	if (IsWindowVisible(_hsidebar)) {
		WindowRect rt(_hsidebar);
		rect.left += rt.right-rt.left;

		SetWindowPos(_hsidebar, 0, -1, rect.top-1, rt.right-rt.left, rect.bottom-rect.top+1, SWP_NOACTIVATE|SWP_NOZORDER);
	}
}

void MainFrameBase::resize_frame_client()
{
	ClientRect rect(_hwnd);

	resize_frame(rect.right, rect.bottom);
}

void MainFrameBase::frame_get_clientspace(PRECT prect)
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

BOOL MainFrameBase::toggle_fullscreen()
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

void MainFrameBase::fullscreen_move()
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


void MainFrameBase::toggle_child(HWND hwnd, UINT cmd, HWND hchild, int band_idx)
{
	BOOL vis = IsWindowVisible(hchild);

	CheckMenuItem(_menu_info._hMenuView, cmd, vis?MF_BYCOMMAND:MF_BYCOMMAND|MF_CHECKED);

	if (band_idx != -1)
		SendMessage(_hwndrebar, RB_SHOWBAND, band_idx, !vis);
	else
		ShowWindow(hchild, vis? SW_HIDE: SW_SHOW);

	if (_fullscreen._mode)
		fullscreen_move();

	resize_frame_client();
}

void MainFrameBase::FillBookmarks()
{
	HiddenWindow hide(_hsidebar);
	WindowCanvas canvas(_hwnd);

	TreeView_DeleteAllItems(_hsidebar);

	g_Globals._icon_cache.get_icon(ICID_FAVORITES).add_to_imagelist(_himl, canvas);
	g_Globals._icon_cache.get_icon(ICID_BOOKMARK).add_to_imagelist(_himl, canvas);
	ImageList_AddAlphaIcon(_himl, SmallIcon(IDI_DOT), GetStockBrush(WHITE_BRUSH), canvas);
	g_Globals._icon_cache.get_icon(ICID_FOLDER).add_to_imagelist(_himl, canvas);
	g_Globals._icon_cache.get_icon(ICID_FOLDER).add_to_imagelist(_himl, canvas);

	TV_INSERTSTRUCT tvi;

	tvi.hParent = TVI_ROOT;
	tvi.hInsertAfter = TVI_LAST;
	tvi.item.mask = TVIF_TEXT|TVIF_IMAGE|TVIF_SELECTEDIMAGE;
	ResString sFavorites(IDS_FAVORITES);
	tvi.item.pszText = sFavorites.str();
	tvi.item.iSelectedImage = tvi.item.iImage = 0;

	HTREEITEM hitem_bookmarks = TreeView_InsertItem(_hsidebar, &tvi);

	g_Globals._favorites.fill_tree(_hsidebar, hitem_bookmarks, _himl, canvas);

	TreeView_Expand(_hsidebar, hitem_bookmarks, TVE_EXPAND);
}


bool MainFrameBase::go_to(LPCTSTR url, bool new_window)
{
	///@todo SDI implementation

	return false;
}


#ifndef _NO_MDI

MDIMainFrame::MDIMainFrame(HWND hwnd)
 :	super(hwnd)
{
	TBBUTTON mdiBtns[] = {
		{0, 0, 0, BTNS_SEP, {0, 0}, 0, 0},
		{0, ID_WINDOW_NEW, TBSTATE_ENABLED, BTNS_BUTTON, {0, 0}, 0, 0},
		{1, ID_WINDOW_CASCADE, TBSTATE_ENABLED, BTNS_BUTTON, {0, 0}, 0, 0},
		{2, ID_WINDOW_TILE_HORZ, TBSTATE_ENABLED, BTNS_BUTTON, {0, 0}, 0, 0},
		{3, ID_WINDOW_TILE_VERT, TBSTATE_ENABLED, BTNS_BUTTON, {0, 0}, 0, 0}
	};

	SendMessage(_htoolbar, TB_ADDBUTTONS, sizeof(mdiBtns)/sizeof(TBBUTTON), (LPARAM)&mdiBtns);

	CLIENTCREATESTRUCT ccs;

	ccs.hWindowMenu = _hMenuWindow;
	ccs.idFirstChild = IDW_FIRST_CHILD;

	_hmdiclient = CreateWindowEx(WS_EX_CLIENTEDGE|WS_EX_NOPARENTNOTIFY, TEXT("MDICLIENT"), NULL,
					WS_CHILD|WS_CLIPCHILDREN|WS_VSCROLL|WS_HSCROLL|WS_VISIBLE|WS_BORDER,
					0, 0, 0, 0,
					hwnd, 0, g_Globals._hInstance, &ccs);

	TBBUTTON extraBtns = {0, 0, TBSTATE_ENABLED, BTNS_SEP, {0, 0}, 0, 0};

#ifndef _NO_REBAR
	_hextrabar = CreateToolbarEx(hwnd,
				CCS_NOPARENTALIGN|CCS_NORESIZE|CCS_NODIVIDER|
				WS_CHILD|TBSTYLE_FLAT|WS_VISIBLE|CCS_NOMOVEY|TBSTYLE_LIST,
				IDW_EXTRABAR, 2, g_Globals._hInstance, IDB_DRIVEBAR, NULL, 0,
				16, 16, 16, 16, sizeof(TBBUTTON));
#else
	_hextrabar = CreateToolbarEx(hwnd,
				WS_CHILD|WS_VISIBLE|CCS_NOMOVEY|TBSTYLE_LIST,CCS_NODIVIDER|
				IDW_EXTRABAR, 2, g_Globals._hInstance, IDB_DRIVEBAR, &extraBtns, 1,
				16, 13, 16, 13, sizeof(TBBUTTON));
#endif

	CheckMenuItem(_menu_info._hMenuView, ID_VIEW_EXTRA_BAR, MF_BYCOMMAND|MF_CHECKED);


	extraBtns.fsStyle = BTNS_BUTTON;

#ifdef __WINE__
	 // insert unix file system button
	extraBtns.iString = SendMessage(_hextrabar, TB_ADDSTRING, 0, (LPARAM)TEXT("/\0"));
	extraBtns.idCommand = ID_DRIVE_UNIX_FS;
	SendMessage(_hextrabar, TB_INSERTBUTTON, INT_MAX, (LPARAM)&extraBtns);
#endif

	 // insert explorer window button
	extraBtns.iString = SendMessage(_hextrabar, TB_ADDSTRING, 0, (LPARAM)TEXT("Explore\0"));
	extraBtns.idCommand = ID_DRIVE_DESKTOP;
	SendMessage(_hextrabar, TB_INSERTBUTTON, INT_MAX, (LPARAM)&extraBtns);

	 // insert shell namespace button
	extraBtns.iString = SendMessage(_hextrabar, TB_ADDSTRING, 0, (LPARAM)TEXT("Shell\0"));
	extraBtns.idCommand = ID_DRIVE_SHELL_NS;
	extraBtns.iBitmap = 6;
	SendMessage(_hextrabar, TB_INSERTBUTTON, INT_MAX, (LPARAM)&extraBtns);

	 // insert web control button
	extraBtns.iString = SendMessage(_hextrabar, TB_ADDSTRING, 0, (LPARAM)TEXT("Web\0"));
	extraBtns.idCommand = ID_WEB_WINDOW;
	extraBtns.iBitmap = 7;
	SendMessage(_hextrabar, TB_INSERTBUTTON, INT_MAX, (LPARAM)&extraBtns);

	if ((HIWORD(GetVersion())>>14) == W_VER_NT) {
		 // insert NT object namespace button
		extraBtns.iString = SendMessage(_hextrabar, TB_ADDSTRING, 0, (LPARAM)TEXT("NT Obj\0"));
		extraBtns.idCommand = ID_DRIVE_NTOBJ_NS;
		extraBtns.iBitmap = 8;
		SendMessage(_hextrabar, TB_INSERTBUTTON, INT_MAX, (LPARAM)&extraBtns);
	}

	 // insert Registry button
	extraBtns.iString = SendMessage(_hextrabar, TB_ADDSTRING, 0, (LPARAM)TEXT("Reg.\0"));
	extraBtns.idCommand = ID_DRIVE_REGISTRY;
	extraBtns.iBitmap = 9;
	SendMessage(_hextrabar, TB_INSERTBUTTON, INT_MAX, (LPARAM)&extraBtns);

#ifdef _DEBUG
	 // insert FAT direct file system access button
	extraBtns.iString = SendMessage(_hextrabar, TB_ADDSTRING, 0, (LPARAM)TEXT("FAT\0"));
	extraBtns.idCommand = ID_DRIVE_FAT;
	extraBtns.iBitmap = 10;
	SendMessage(_hextrabar, TB_INSERTBUTTON, INT_MAX, (LPARAM)&extraBtns);
#endif


	TBBUTTON drivebarBtn = {0, 0, TBSTATE_ENABLED, BTNS_SEP, {0, 0}, 0, 0};
#ifndef _NO_WIN_FS
	PTSTR p;

#ifndef _NO_REBAR
	_hdrivebar = CreateToolbarEx(hwnd,
				CCS_NOPARENTALIGN|CCS_NORESIZE|CCS_NODIVIDER|
				WS_CHILD|WS_VISIBLE|TBSTYLE_FLAT|CCS_NOMOVEY|TBSTYLE_LIST,
				IDW_DRIVEBAR, 2, g_Globals._hInstance, IDB_DRIVEBAR, NULL, 0,
				16, 16, 16, 16, sizeof(TBBUTTON));
#else
	_hdrivebar = CreateToolbarEx(hwnd,
				WS_CHILD|WS_VISIBLE|CCS_NOMOVEY|TBSTYLE_LIST|CCS_NODIVIDER,
				IDW_DRIVEBAR, 2, g_Globals._hInstance, IDB_DRIVEBAR, &drivebarBtn, 1,
				16, 13, 16, 13, sizeof(TBBUTTON));
#endif
#endif

	CheckMenuItem(_menu_info._hMenuView, ID_VIEW_DRIVE_BAR, MF_BYCOMMAND|MF_UNCHECKED);


#ifndef _NO_WIN_FS
	GetLogicalDriveStrings(BUFFER_LEN, _drives);

	 // register windows drive root strings
	SendMessage(_hdrivebar, TB_ADDSTRING, 0, (LPARAM)_drives);

	drivebarBtn.fsStyle = BTNS_BUTTON;
	drivebarBtn.idCommand = ID_DRIVE_FIRST;

	for(p=_drives; *p; ) {
		switch(GetDriveType(p)) {
			case DRIVE_REMOVABLE:	drivebarBtn.iBitmap = 1;	break;
			case DRIVE_CDROM:		drivebarBtn.iBitmap = 3;	break;
			case DRIVE_REMOTE:		drivebarBtn.iBitmap = 4;	break;
			case DRIVE_RAMDISK: 	drivebarBtn.iBitmap = 5;	break;
			default:/*DRIVE_FIXED*/ drivebarBtn.iBitmap = 2;
		}

		SendMessage(_hdrivebar, TB_INSERTBUTTON, INT_MAX, (LPARAM)&drivebarBtn);
		++drivebarBtn.idCommand;
		++drivebarBtn.iString;

		while(*p++);
	}
#endif


#ifndef _NO_REBAR
	int btn_hgt = HIWORD(SendMessage(_htoolbar, TB_GETBUTTONSIZE, 0, 0));

	REBARBANDINFO rbBand;

	rbBand.cbSize = sizeof(REBARBANDINFO);
	rbBand.fMask  = RBBIM_TEXT|RBBIM_STYLE|RBBIM_CHILD|RBBIM_CHILDSIZE|RBBIM_SIZE;
#ifndef RBBS_HIDETITLE // missing in MinGW headers as of 25.02.2004
#define RBBS_HIDETITLE	0x400
#endif
	rbBand.fStyle = RBBS_CHILDEDGE|RBBS_GRIPPERALWAYS|RBBS_HIDETITLE;

	TCHAR ExtrasBand[] = _T("Extras");
	rbBand.lpText = ExtrasBand;
	rbBand.hwndChild = _hextrabar;
	rbBand.cxMinChild = 0;
	rbBand.cyMinChild = btn_hgt;
	rbBand.cx = 284;
	SendMessage(_hwndrebar, RB_INSERTBAND, (WPARAM)1, (LPARAM)&rbBand);

#ifndef _NO_WIN_FS
	rbBand.fStyle |= RBBS_BREAK;
	TCHAR DrivesBand[] = _T("Drives");
	rbBand.lpText = DrivesBand;
	rbBand.hwndChild = _hdrivebar;
	rbBand.cxMinChild = 0;
	rbBand.cyMinChild = btn_hgt;
	rbBand.cx = 400;
	SendMessage(_hwndrebar, RB_INSERTBAND, (WPARAM)2, (LPARAM)&rbBand);

	// hide the drivebar by default
	SendMessage(_hwndrebar, RB_SHOWBAND, 2, FALSE);
#endif
#endif
}


HWND MDIMainFrame::Create()
{
	HMENU hMenuFrame = LoadMenu(g_Globals._hInstance, MAKEINTRESOURCE(IDM_MDIFRAME));

	return Window::Create(WINDOW_CREATOR(MDIMainFrame), 0,
				(LPCTSTR)(int)g_Globals._hframeClass, ResString(IDS_TITLE), WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
				CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
				0/*hwndDesktop*/, hMenuFrame);
}

HWND MDIMainFrame::Create(LPCTSTR path, int mode)
{
	HWND hFrame = Create();
	if (!hFrame)
		return 0;

	ShowWindow(hFrame, SW_SHOW);

	MDIMainFrame* pMainFrame = GET_WINDOW(MDIMainFrame, hFrame);

	if (pMainFrame)
		pMainFrame->CreateChild(path, mode);

	return hFrame;
}

HWND MDIMainFrame::Create(LPCITEMIDLIST pidl, int mode)
{
	HWND hFrame = Create();
	if (!hFrame)
		return 0;

	ShowWindow(hFrame, SW_SHOW);

	MDIMainFrame* pMainFrame = GET_WINDOW(MDIMainFrame, hFrame);

	if (pMainFrame)
		pMainFrame->CreateChild(pidl, mode);

	return hFrame;
}


ChildWindow* MDIMainFrame::CreateChild(LPCTSTR path, int mode)
{
	return reinterpret_cast<ChildWindow*>(SendMessage(_hwnd, PM_OPEN_WINDOW, mode, (LPARAM)path));
}

ChildWindow* MDIMainFrame::CreateChild(LPCITEMIDLIST pidl, int mode)
{
	return reinterpret_cast<ChildWindow*>(SendMessage(_hwnd, PM_OPEN_WINDOW, mode|OWM_PIDL, (LPARAM)pidl));
}


BOOL MDIMainFrame::TranslateMsg(MSG* pmsg)
{
	if (_hmdiclient && TranslateMDISysAccel(_hmdiclient, pmsg))
		return TRUE;

	return super::TranslateMsg(pmsg);
}

LRESULT MDIMainFrame::WndProc(UINT nmsg, WPARAM wparam, LPARAM lparam)
{
	switch(nmsg) {
	  case PM_OPEN_WINDOW: {
		CONTEXT("MDIMainFrame PM_OPEN_WINDOW");

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
					LOG(FmtString(TEXT("MDIMainFrame PM_OPEN_WINDOW: path=%s"), path));
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

		if (path && _tcsstr(path, TEXT("://"))) {	// "http://...", "ftp://", ...
			OBJ_CONTEXT("create WebChild window", path);

			return (LRESULT)GET_WINDOW(ChildWindow, create_webchildwindow(WebChildWndInfo(_hmdiclient, path)));
		} else {
			OBJ_CONTEXT("create ShellChildWndInfo", path);

			 // Shell Namespace as default view
			ShellChildWndInfo create_info(_hmdiclient, path, shell_path);

			if (wparam & OWM_ROOTED)
				create_info._root_shell_path = shell_path;
			else
				create_info._root_shell_path = DesktopFolderPath();	//SpecialFolderPath(CSIDL_DRIVES, _hwnd);

			create_info._pos.showCmd = wparam&OWM_SEPARATE? SW_SHOWNORMAL: SW_SHOWMAXIMIZED;
			create_info._pos.rcNormalPosition.left = CW_USEDEFAULT;
			create_info._pos.rcNormalPosition.top = CW_USEDEFAULT;
			create_info._pos.rcNormalPosition.right = CW_USEDEFAULT;
			create_info._pos.rcNormalPosition.bottom = CW_USEDEFAULT;

			create_info._open_mode = wparam;

		//	FileChildWindow::create(_hmdiclient, create_info);
			return (LRESULT)MDIShellBrowserChild::create(create_info);
		}
		return TRUE;}	// success

	  case WM_SYSCOLORCHANGE: {
		LRESULT res;
		HWND hChild;

		/* Forward WM_SYSCOLORCHANGE to common controls */
		SendMessage(_hextrabar, WM_SYSCOLORCHANGE, 0, 0);
		SendMessage(_hdrivebar, WM_SYSCOLORCHANGE, 0, 0);

		for(hChild = GetNextWindow(_hmdiclient,GW_CHILD); hChild; hChild = GetNextWindow(hChild, GW_HWNDNEXT))
			SendMessage(hChild, WM_SYSCOLORCHANGE, 0, 0);

		super::ProcessMessage(nmsg, wparam, lparam, &res);
		break; }

	  default: {
		LRESULT res;

		if (super::ProcessMessage(nmsg, wparam, lparam, &res))
			return res;
		else
			return DefFrameProc(_hwnd, _hmdiclient, nmsg, wparam, lparam);
	  }
	}

	return 0;
}

int MDIMainFrame::Command(int id, int code)
{
	CONTEXT("MDIMainFrame::Command()");

	HWND hwndClient = (HWND) SendMessage(_hmdiclient, WM_MDIGETACTIVE, 0, 0);

	if (hwndClient)
		if (SendMessage(hwndClient, PM_DISPATCH_COMMAND, MAKELONG(id,code), 0))
			return 0;

#ifndef _NO_WIN_FS
	if (id>=ID_DRIVE_FIRST && id<=ID_DRIVE_FIRST+0xFF) {
		TCHAR drv[_MAX_DRIVE], path[MAX_PATH];
		LPCTSTR root = _drives;

		for(int i=id-ID_DRIVE_FIRST; i--; root++)
			while(*root)
				++root;

		if (activate_drive_window(root))
			return 0;

		_tsplitpath_s(root, drv, COUNTOF(drv), NULL, 0, NULL, 0, NULL, 0);

		if (!SetCurrentDirectory(drv)) {
			display_error(_hwnd, GetLastError());
			return 0;
		}

		GetCurrentDirectory(MAX_PATH, path); ///@todo store last directory per drive

		FileChildWindow::create(FileChildWndInfo(_hmdiclient, path));

		return 1;
	}
#endif

	switch(id) {
	  case ID_WINDOW_NEW: {
		TCHAR path[MAX_PATH];

		GetCurrentDirectory(MAX_PATH, path);

		FileChildWindow::create(FileChildWndInfo(_hmdiclient, path));
		break;}

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

	  case ID_VIEW_EXTRA_BAR:
		toggle_child(_hwnd, id, _hextrabar, 1);
		break;

#ifndef _NO_WIN_FS
	  case ID_VIEW_DRIVE_BAR:
		toggle_child(_hwnd, id, _hdrivebar, 2);
		break;
#endif

#ifdef __WINE__
	  case ID_DRIVE_UNIX_FS: {
		TCHAR path[MAX_PATH];
		FileChildWindow* child;

		getcwd(path, COUNTOF(path));

		if (activate_child_window(TEXT("unixfs")))
			break;

		FileChildWindow::create(_hmdiclient, FileChildWndInfo(path));
		break;}
#endif

	  case ID_DRIVE_DESKTOP: {
		TCHAR path[MAX_PATH];

		if (activate_child_window(TEXT("Desktop")))
			break;

		GetCurrentDirectory(MAX_PATH, path);

		MDIShellBrowserChild::create(ShellChildWndInfo(_hmdiclient, path, DesktopFolderPath()));
		break;}

	  case ID_DRIVE_SHELL_NS: {
		TCHAR path[MAX_PATH];
		GetCurrentDirectory(MAX_PATH, path);

		if (activate_child_window(TEXT("Shell")))
			break;

		FileChildWindow::create(ShellChildWndInfo(_hmdiclient, path, DesktopFolderPath()));
		break;}

	  case ID_DRIVE_NTOBJ_NS: {
		if (activate_child_window(TEXT("NTOBJ")))
			break;

		FileChildWindow::create(NtObjChildWndInfo(_hmdiclient, TEXT("\\")));
	  break;}

	  case ID_DRIVE_REGISTRY: {
		if (activate_child_window(TEXT("Registry")))
			break;

		FileChildWindow::create(RegistryChildWndInfo(_hmdiclient, TEXT("\\")));
	  break;}

	  case ID_DRIVE_FAT: {

	  	///@todo prompt for image file

		if (activate_child_window(TEXT("FAT")))
			break;

		FileChildWindow::create(FATChildWndInfo(_hmdiclient, TEXT("FAT Image")));	//@@
	  break;}

	  case ID_WEB_WINDOW:
#ifdef _DEBUG
		create_webchildwindow(WebChildWndInfo(_hmdiclient, TEXT("http://localhost")));
#else
		create_webchildwindow(WebChildWndInfo(_hmdiclient, TEXT("https://reactos.org")));
#endif
		break;

	  case ID_EXPLORER_FAQ:
		create_webchildwindow(WebChildWndInfo(_hmdiclient, TEXT("http://www.sky.franken.de/explorer/")));
		break;

	  case ID_VIEW_SDI:
		MainFrameBase::Create(ExplorerCmd());
		break;

	///@todo There are even more menu items!

	  default:
		if (super::Command(id, code) == 0)
			return 0;
		else
			return DefFrameProc(_hwnd, _hmdiclient, WM_COMMAND, MAKELONG(id,code), 0);
	}

	return 0;
}


void MDIMainFrame::frame_get_clientspace(PRECT prect)
{
	super::frame_get_clientspace(prect);

#ifndef _NO_WIN_FS
	if (IsWindowVisible(_hdrivebar)) {
		ClientRect rt(_hdrivebar);
		prect->top += rt.bottom+2;
	}
#endif
}

void MDIMainFrame::resize_frame(int cx, int cy)
{
	if (cy <= 0)
		return;	// avoid resizing children when receiving RBN_AUTOSIZE while getting minimized

	RECT rect = {0, 0, cx, cy};

	if (_hwndrebar) {
        int height = SendMessage(_hwndrebar, RB_GETBARHEIGHT, 0, 0);
		rect.top += height;
        rect.top += 5;
	} else {
		if (IsWindowVisible(_htoolbar)) {
			SendMessage(_htoolbar, WM_SIZE, 0, 0);
			WindowRect rt(_htoolbar);
			rect.top = rt.bottom;
		//	rect.bottom -= rt.bottom;
		}

		if (IsWindowVisible(_hextrabar)) {
			SendMessage(_hextrabar, WM_SIZE, 0, 0);
			WindowRect rt(_hextrabar);
			int new_top = --rect.top + rt.bottom;
			MoveWindow(_hextrabar, 0, rect.top, rt.right, new_top, TRUE);
			rect.top = new_top;
		//	rect.bottom -= rt.bottom;
		}

#ifndef _NO_WIN_FS
		if (IsWindowVisible(_hdrivebar)) {
			SendMessage(_hdrivebar, WM_SIZE, 0, 0);
			WindowRect rt(_hdrivebar);
			int new_top = --rect.top + rt.bottom;
			MoveWindow(_hdrivebar, 0, rect.top, rt.right, new_top, TRUE);
			rect.top = new_top;
		//	rect.bottom -= rt.bottom;
		}
#endif
	}

	if (IsWindowVisible(_hstatusbar)) {
		int parts[] = {300, 500};

		SendMessage(_hstatusbar, WM_SIZE, 0, 0);
		SendMessage(_hstatusbar, SB_SETPARTS, 2, (LPARAM)&parts);
		ClientRect rt(_hstatusbar);
		rect.bottom -= rt.bottom;
	}

	if (IsWindowVisible(_hsidebar)) {
		WindowRect rt(_hsidebar);
		rect.left += rt.right-rt.left;

		SetWindowPos(_hsidebar, 0, -1, rect.top-1, rt.right-rt.left, rect.bottom-rect.top+1, SWP_NOACTIVATE|SWP_NOZORDER);
	}

	MoveWindow(_hmdiclient, rect.left-1, rect.top-1, rect.right-rect.left+1, rect.bottom-rect.top+1, TRUE);
}

bool MDIMainFrame::activate_drive_window(LPCTSTR path)
{
	TCHAR drv1[_MAX_DRIVE], drv2[_MAX_DRIVE];
	HWND child_wnd;

	_tsplitpath_s(path, drv1, COUNTOF(drv1), NULL, 0, NULL, 0, NULL, 0);

	 // search for a already open window for the same drive
	for(child_wnd=GetNextWindow(_hmdiclient,GW_CHILD); child_wnd; child_wnd=GetNextWindow(child_wnd, GW_HWNDNEXT)) {
		FileChildWindow* child = (FileChildWindow*) SendMessage(child_wnd, PM_GET_FILEWND_PTR, 0, 0);

		if (child) {
			_tsplitpath_s(child->get_root()._path, drv2, COUNTOF(drv2), NULL, 0, NULL, 0, NULL, 0);

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

bool MDIMainFrame::activate_child_window(LPCTSTR filesys)
{
	HWND child_wnd;

	 // search for a already open window of the given file system name
	for(child_wnd=GetNextWindow(_hmdiclient,GW_CHILD); child_wnd; child_wnd=GetNextWindow(child_wnd, GW_HWNDNEXT)) {
		FileChildWindow* child = (FileChildWindow*) SendMessage(child_wnd, PM_GET_FILEWND_PTR, 0, 0);

		if (child) {
			if (!lstrcmpi(child->get_root()._fs, filesys)) {
				SendMessage(_hmdiclient, WM_MDIACTIVATE, (WPARAM)child_wnd, 0);

				if (IsMinimized(child_wnd))
					ShowWindow(child_wnd, SW_SHOWNORMAL);

				return true;
			}
		} else {
			ShellBrowser* shell_child = (ShellBrowser*) SendMessage(child_wnd, PM_GET_SHELLBROWSER_PTR, 0, 0);

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

bool MDIMainFrame::go_to(LPCTSTR url, bool new_window)
{
	if (!new_window) {
		HWND hwndClient = (HWND) SendMessage(_hmdiclient, WM_MDIGETACTIVE, 0, 0);

		if (hwndClient)
			if (SendMessage(hwndClient, PM_JUMP_TO_URL, 0, (LPARAM)url))
				return true;
	}

	if (CreateChild(url))
		return true;

	return super::go_to(url, new_window);
}

#endif // _NO_MDI


SDIMainFrame::SDIMainFrame(HWND hwnd)
 :	super(hwnd)
{
	_split_pos = DEFAULT_SPLIT_POS;
	_last_split = DEFAULT_SPLIT_POS;

	/* wait for PM_OPEN_WINDOW message before creating a shell view
	update_shell_browser();*/
}

HWND SDIMainFrame::Create()
{
	HMENU hMenuFrame = LoadMenu(g_Globals._hInstance, MAKEINTRESOURCE(IDM_SDIFRAME));

	return Window::Create(WINDOW_CREATOR(SDIMainFrame), 0,
				(LPCTSTR)(int)g_Globals._hframeClass, ResString(IDS_TITLE), WS_OVERLAPPEDWINDOW,
				CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
				0/*hwndDesktop*/, hMenuFrame);
}

HWND SDIMainFrame::Create(LPCITEMIDLIST pidl, int mode)
{
	HWND hFrame = Create();
	if (!hFrame)
		return 0;

	ShowWindow(hFrame, SW_SHOW);

	SDIMainFrame* pFrame = GET_WINDOW(SDIMainFrame, hFrame);

	if (pFrame)
		pFrame->jump_to(pidl, mode);

	return hFrame;
}

HWND SDIMainFrame::Create(LPCTSTR path, int mode)
{
	HWND hFrame = Create();
	if (!hFrame)
		return 0;

	ShowWindow(hFrame, SW_SHOW);

	MDIMainFrame* pMainFrame = GET_WINDOW(MDIMainFrame, hFrame);

	if (pMainFrame)
		pMainFrame->CreateChild(path, mode);

	return hFrame;
}

LRESULT SDIMainFrame::WndProc(UINT nmsg, WPARAM wparam, LPARAM lparam)
{
	switch(nmsg) {
	  case WM_SIZE:
		resize_frame(LOWORD(lparam), HIWORD(lparam));
		break;

	  case WM_PAINT: {
		PaintCanvas canvas(_hwnd);

		if (_left_hwnd) {
			ClientRect rt(_hwnd);
			rt.left = _split_pos-SPLIT_WIDTH/2;
			rt.right = _split_pos+SPLIT_WIDTH/2+1;

			if (_right_hwnd) {
				WindowRect right_rect(_right_hwnd);
				ScreenToClient(_hwnd, &right_rect);
				rt.top = right_rect.top;
				rt.bottom = right_rect.bottom;
			}

			HBRUSH lastBrush = SelectBrush(canvas, GetStockBrush(COLOR_SPLITBAR));
			Rectangle(canvas, rt.left, rt.top-1, rt.right, rt.bottom+1);
			SelectObject(canvas, lastBrush);
		}
		break;}

	  case WM_SETCURSOR:
  		if (_left_hwnd)
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

	  case WM_LBUTTONDOWN:
		if (_left_hwnd) {
			int x = GET_X_LPARAM(lparam);

			ClientRect rt(_hwnd);

			if (x>=_split_pos-SPLIT_WIDTH/2 && x<_split_pos+SPLIT_WIDTH/2+1) {
				_last_split = _split_pos;
				SetCapture(_hwnd);
			}
		}
		break;

	  case WM_LBUTTONUP:
		if (GetCapture() == _hwnd)
			ReleaseCapture();
		break;

	  case WM_KEYDOWN:
		if (wparam == VK_ESCAPE)
			if (GetCapture() == _hwnd) {
				_split_pos = _last_split;
				resize_children();
				_last_split = -1;
				ReleaseCapture();
				SetCursor(LoadCursor(0, IDC_ARROW));
			}
		break;

	  case WM_MOUSEMOVE:
		if (GetCapture() == _hwnd) {
			int x = GET_X_LPARAM(lparam);

			ClientRect rt(_hwnd);

			if (x>=0 && x<rt.right) {
				_split_pos = x;
				resize_children();
				rt.left = x-SPLIT_WIDTH/2;
				rt.right = x+SPLIT_WIDTH/2+1;
				InvalidateRect(_hwnd, &rt, FALSE);
				UpdateWindow(_left_hwnd);
				UpdateWindow(_hwnd);
				UpdateWindow(_right_hwnd);
			}
		}
		break;

	  case PM_OPEN_WINDOW: {
		CONTEXT("SDIMainFrame PM_OPEN_WINDOW");

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
					LOG(FmtString(TEXT("SDIMainFrame PM_OPEN_WINDOW: path=%s"), path));
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
			shell_path = path;
		}

		if (wparam & OWM_ROOTED)
			_shellpath_info._root_shell_path = shell_path;
		else
			_shellpath_info._root_shell_path = DesktopFolderPath();	//SpecialFolderPath(CSIDL_DRIVES, _hwnd);

		jump_to(shell_path, wparam);	///@todo content of 'path' not used any more
		return TRUE;}	// success

	  default: def:
		return super::WndProc(nmsg, wparam, lparam);
	}

	return 0;
}

int SDIMainFrame::Command(int id, int code)
{
	switch(id) {
	  case ID_VIEW_MDI:
		MainFrameBase::Create(ExplorerCmd(_url, true));
		break;

	  default:
		return super::Command(id, code);
	}

	return 0;
}

void SDIMainFrame::resize_frame(int cx, int cy)
{
	if (cy <= 0)
		return;	// avoid resizing children when receiving RBN_AUTOSIZE while getting minimized

	RECT rect = {0, 0, cx, cy};

	if (_hwndrebar) {
		int height = ClientRect(_hwndrebar).bottom;
		rect.top += height;
        rect.top += 5;
	} else {
		if (IsWindowVisible(_htoolbar)) {
			SendMessage(_htoolbar, WM_SIZE, 0, 0);
			WindowRect rt(_htoolbar);
			rect.top = rt.bottom;
		//	rect.bottom -= rt.bottom;
		}
	}

	if (IsWindowVisible(_hstatusbar)) {
		int parts[] = {300, 500};

		SendMessage(_hstatusbar, WM_SIZE, 0, 0);
		SendMessage(_hstatusbar, SB_SETPARTS, 2, (LPARAM)&parts);
		ClientRect rt(_hstatusbar);
		rect.bottom -= rt.bottom;
	}

	if (IsWindowVisible(_hsidebar)) {
		WindowRect rt(_hsidebar);
		rect.left += rt.right-rt.left;

		SetWindowPos(_hsidebar, 0, -1, rect.top-1, rt.right-rt.left, rect.bottom-rect.top+1, SWP_NOACTIVATE|SWP_NOZORDER);
	}

	_clnt_rect = rect;

	resize_children();
}

void SDIMainFrame::resize_children()
{
	HDWP hdwp = BeginDeferWindowPos(2);

	int cx = _clnt_rect.left;

	if (_left_hwnd) {
		cx = _split_pos + SPLIT_WIDTH/2;

		hdwp = DeferWindowPos(hdwp, _left_hwnd, 0, _clnt_rect.left, _clnt_rect.top, _split_pos-SPLIT_WIDTH/2-_clnt_rect.left, _clnt_rect.bottom-_clnt_rect.top, SWP_NOZORDER|SWP_NOACTIVATE);
	} else {
		//_split_pos = -1;
		cx = 0;
	}

	if (_right_hwnd)
		hdwp = DeferWindowPos(hdwp, _right_hwnd, 0, _clnt_rect.left+cx+1, _clnt_rect.top, _clnt_rect.right-cx, _clnt_rect.bottom-_clnt_rect.top, SWP_NOZORDER|SWP_NOACTIVATE);

	EndDeferWindowPos(hdwp);
}

void SDIMainFrame::update_clnt_rect()
{
	ClientRect rect(_hwnd);

	resize_frame(rect.right, rect.bottom);
}

void SDIMainFrame::update_shell_browser()
{
	int split_pos = DEFAULT_SPLIT_POS;

	if (_shellBrowser.get()) {
		split_pos = _split_pos;
		delete _shellBrowser.release();
	}

	 // create explorer treeview
	if (_shellpath_info._open_mode & OWM_EXPLORE) {
		if (!_left_hwnd) {
			ClientRect rect(_hwnd);

			_left_hwnd = CreateWindowEx(WS_EX_CLIENTEDGE, WC_TREEVIEW, NULL,
							WS_CHILD|WS_TABSTOP|WS_VISIBLE|TVS_HASLINES|TVS_LINESATROOT|TVS_HASBUTTONS|TVS_NOTOOLTIPS|TVS_SHOWSELALWAYS,
							0, rect.top, split_pos-SPLIT_WIDTH/2, rect.bottom-rect.top,
							_hwnd, (HMENU)IDC_FILETREE, g_Globals._hInstance, 0);

			 // display tree window as long as the shell view is not yet visible
			resize_frame(rect.right, rect.bottom);
			MoveWindow(_left_hwnd, rect.left, rect.top, rect.right-rect.left, rect.bottom-rect.top, TRUE);
		}
	} else {
		if (_left_hwnd) {
			DestroyWindow(_left_hwnd);
			_left_hwnd = 0;
		}
	}

	_shellBrowser = auto_ptr<ShellBrowser>(new ShellBrowser(_hwnd, _hwnd, _left_hwnd, _right_hwnd,
												_shellpath_info, this, _cm_ifs));

	if (_left_hwnd)
		_shellBrowser->Init();

	 // update _clnt_rect and set size of new created shell view windows
	update_clnt_rect();
}

void SDIMainFrame::entry_selected(Entry* entry)
{
	if (_left_hwnd)
		_shellBrowser->select_folder(entry, false);

	_shellBrowser->UpdateFolderView(entry->get_shell_folder());

	 // set size of new created shell view windows
	resize_children();
}

void SDIMainFrame::set_url(LPCTSTR url)
{
	if (_url != url) {
		_url = url;
		SetWindowText(_haddrcombo, url);
	}
}


void SDIMainFrame::jump_to(LPCTSTR path, int mode)
{/*@@todo
	if (_shellBrowser.get() && (_shellpath_info._open_mode&~OWM_PIDL)==(mode&~OWM_PIDL)) {
		_shellBrowser->jump_to(path);

		_shellpath_info._shell_path = path;
	} else */{
		_shellpath_info._open_mode = mode;
		_shellpath_info._shell_path = path;

		update_shell_browser();
	}
}

void SDIMainFrame::jump_to(LPCITEMIDLIST path, int mode)
{
	if (_shellBrowser.get() && (_shellpath_info._open_mode&~OWM_PIDL)==(mode&~OWM_PIDL)) {
		ShellPath shell_path = path;

		_shellBrowser->jump_to(shell_path);

		_shellpath_info._shell_path = shell_path;
	} else {
		_shellpath_info._open_mode = mode;
		_shellpath_info._shell_path = path;

		update_shell_browser();
	}
}
