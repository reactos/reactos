/*
 * Copyright 2005 Martin Fuchs
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
 // ROS Internet Web Browser
 //
 // mainframe.cpp
 //
 // Martin Fuchs, 25.01.2005
 //


#include "precomp.h"

/* We can't include webchild.h here - otherwise MinGW produces errors like: "multiple definition of `QACONTAINERFLAGS'"
#include "webchild.h"
*/
extern HWND create_webchildwindow(const WebChildWndInfo& info);

#include "ibrowser_intres.h"


HWND MainFrameBase::Create(LPCTSTR url, UINT cmdshow)
{
	HWND hMainFrame;

	hMainFrame = MainFrame::Create();
	//@@hMainFrame = MainFrame::Create(url);

	if (hMainFrame) {
		if (url) {
			static String sPath = url;	// copy url to avoid accessing freed memory
			url = sPath;
		}

		ShowWindow(hMainFrame, cmdshow);
		UpdateWindow(hMainFrame);

		 // Open the first child window after initializing the application
		PostMessage(hMainFrame, PM_OPEN_WINDOW, 0, (LPARAM)url);
	}

	return hMainFrame;
}


MainFrameBase::MainFrameBase(HWND hwnd)
 :	super(hwnd),
	_himl(ImageList_Create(GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), ILC_MASK|ILC_COLOR24, 2, 0))
{
	_hMenuFrame = GetMenu(hwnd);
	_hMenuWindow = GetSubMenu(_hMenuFrame, GetMenuItemCount(_hMenuFrame)-3);

	_menu_info._hMenuView = GetSubMenu(_hMenuFrame, 1);

	_hAccel = LoadAccelerators(g_hInstance, MAKEINTRESOURCE(IDA_IBROWSER));


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
		CCS_NOPARENTALIGN|CCS_NORESIZE|
#endif
		WS_CHILD|WS_VISIBLE, IDW_TOOLBAR, 2, g_hInstance, IDB_TOOLBAR,
		toolbarBtns, sizeof(toolbarBtns)/sizeof(TBBUTTON),
		16, 15, 16, 15, sizeof(TBBUTTON));

	CheckMenuItem(_menu_info._hMenuView, ID_VIEW_TOOL_BAR, MF_BYCOMMAND|MF_CHECKED);


	 // address bar
	WindowCanvas canvas(hwnd);
	RECT rect = {0, 0, 0, 0};
	DrawText(canvas, TEXT("My"), -1, &rect, DT_SINGLELINE|DT_NOPREFIX|DT_CALCRECT);
	HFONT hfont = GetStockFont(DEFAULT_GUI_FONT);

	_haddressedit = CreateWindow(TEXT("EDIT"), NULL, WS_CHILD|WS_VISIBLE, 0, 0, 0, rect.bottom,
							hwnd, (HMENU)IDW_ADDRESSBAR, g_hInstance, 0);
	SetWindowFont(_haddressedit, hfont, FALSE);
	new EditController(_haddressedit);

	/* CreateStatusWindow does not accept WS_BORDER
		_hstatusbar = CreateWindowEx(WS_EX_NOPARENTNOTIFY, STATUSCLASSNAME, 0,
						WS_CHILD|WS_VISIBLE|WS_CLIPSIBLINGS|WS_BORDER|CCS_NODIVIDER, 0,0,0,0,
						hwnd, (HMENU)IDW_STATUSBAR, g_hInstance, 0);*/

	_hstatusbar = CreateStatusWindow(WS_CHILD|WS_VISIBLE, 0, hwnd, IDW_STATUSBAR);
	CheckMenuItem(_menu_info._hMenuView, ID_VIEW_STATUSBAR, MF_BYCOMMAND|MF_CHECKED);

	_hsidebar = CreateWindowEx(WS_EX_STATICEDGE, WC_TREEVIEW, TEXT("Sidebar"),
					WS_CHILD|WS_TABSTOP|WS_BORDER|/*WS_VISIBLE|*/WS_CHILD|TVS_HASLINES|TVS_HASBUTTONS|TVS_SHOWSELALWAYS|TVS_INFOTIP,
					-1, -1, 200, 0, _hwnd, (HMENU)IDW_SIDEBAR, g_hInstance, 0);

	TreeView_SetImageList(_hsidebar, _himl, TVSIL_NORMAL);

	CheckMenuItem(_menu_info._hMenuView, ID_VIEW_SIDE_BAR, MF_BYCOMMAND|MF_UNCHECKED/*MF_CHECKED*/);


	 // create rebar window to manage toolbar and drivebar
#ifndef _NO_REBAR
	_hwndrebar = CreateWindowEx(WS_EX_TOOLWINDOW, REBARCLASSNAME, NULL,
					WS_CHILD|WS_VISIBLE|WS_CLIPSIBLINGS|WS_CLIPCHILDREN|
					RBS_VARHEIGHT|RBS_AUTOSIZE|RBS_DBLCLKTOGGLE|
					CCS_NODIVIDER|CCS_NOPARENTALIGN,
					0, 0, 0, 0, _hwnd, 0, g_hInstance, 0);

	int btn_hgt = HIWORD(SendMessage(_htoolbar, TB_GETBUTTONSIZE, 0, 0));

	REBARBANDINFO rbBand;

	rbBand.cbSize = sizeof(REBARBANDINFO);
	rbBand.fMask  = RBBIM_TEXT|RBBIM_STYLE|RBBIM_CHILD|RBBIM_CHILDSIZE|RBBIM_SIZE;
#ifndef RBBS_HIDETITLE // missing in MinGW headers as of 25.02.2004
#define RBBS_HIDETITLE	0x400
#endif
	rbBand.fStyle = RBBS_CHILDEDGE|RBBS_GRIPPERALWAYS|RBBS_HIDETITLE;

	rbBand.cxMinChild = 0;
	rbBand.cyMinChild = 0;
	rbBand.cyChild = 0;
	rbBand.cyMaxChild = 0;
	rbBand.cyIntegral = btn_hgt;

	rbBand.lpText = NULL;//TEXT("Toolbar");
	rbBand.hwndChild = _htoolbar;
	rbBand.cxMinChild = 0;
	rbBand.cyMinChild = btn_hgt + 4;
	rbBand.cx = 284;
	SendMessage(_hwndrebar, RB_INSERTBAND, (WPARAM)-1, (LPARAM)&rbBand);
#endif
}


MainFrameBase::~MainFrameBase()
{
	ImageList_Destroy(_himl);

//@@if (g_Globals._hMainWnd == _hwnd)
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
		if (wparam)	// trigger child resizing after window creation - now we can succesfully call IsWindowVisible()
			resize_frame_client();
		return false; // goto def;

	  case WM_CLOSE:
		DestroyWindow(_hwnd);
//@@		g_Globals._hMainWnd = 0;
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

	  case PM_URL_CHANGED:
		SetWindowText(_haddressedit, (LPCTSTR)lparam);
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

	  case ID_HELP:
		WinHelp(_hwnd, TEXT("ibrowser")/*file ibrowser.hlp*/, HELP_INDEX, 0);
		break;

	  case ID_VIEW_FULLSCREEN:
		CheckMenuItem(_menu_info._hMenuView, id, toggle_fullscreen()?MF_CHECKED:0);
		break;

	  case ID_ABOUT_WINDOWS:
		ShellAbout(_hwnd, ResString(IDS_TITLE), NULL, 0);
		break;

	  case ID_ABOUT_IBROWSER:
		ibrowser_about(_hwnd);
		break;

	  case ID_IBROWSER_FAQ:
		launch_file(_hwnd, TEXT("http://www.sky.franken.de/explorer/"), SW_SHOW);
		break;

	  case IDW_ADDRESSBAR:
		if (code == 1) {
			TCHAR url[BUFFER_LEN];

			if (GetWindowText(_haddressedit, url, BUFFER_LEN))
				go_to(url, false);
		}
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
		int height = ClientRect(_hwndrebar).bottom;
		MoveWindow(_hwndrebar, rect.left, rect.top, rect.right-rect.left, height, TRUE);
		rect.top += height;
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

	if (IsWindowVisible(_haddressedit)) {
		ClientRect rt(_haddressedit);
		rect.bottom -= rt.bottom;

		SetWindowPos(_haddressedit, 0, 0, rect.bottom, rect.right-rect.left, rt.bottom, SWP_NOACTIVATE|SWP_NOZORDER);
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
/*@@
	HiddenWindow hide(_hsidebar);
	WindowCanvas canvas(_hwnd);

	TreeView_DeleteAllItems(_hsidebar);

	g_icon_cache.get_icon(ICID_FAVORITES).add_to_imagelist(_himl, canvas);
	g_icon_cache.get_icon(ICID_BOOKMARK).add_to_imagelist(_himl, canvas);
	ImageList_AddAlphaIcon(_himl, SmallIcon(IDI_DOT), GetStockBrush(WHITE_BRUSH), canvas);
	g_icon_cache.get_icon(ICID_FOLDER).add_to_imagelist(_himl, canvas);
	g_icon_cache.get_icon(ICID_FOLDER).add_to_imagelist(_himl, canvas);

	TV_INSERTSTRUCT tvi;

	tvi.hParent = TVI_ROOT;
	tvi.hInsertAfter = TVI_LAST;
	tvi.item.mask = TVIF_TEXT|TVIF_IMAGE|TVIF_SELECTEDIMAGE;
	ResString sFavorites(IDS_FAVORITES);
	tvi.item.pszText = (LPTSTR)sFavorites.c_str();
	tvi.item.iSelectedImage = tvi.item.iImage = 0;

	HTREEITEM hitem_bookmarks = TreeView_InsertItem(_hsidebar, &tvi);

	g_Globals._favorites.fill_tree(_hsidebar, hitem_bookmarks, _himl, canvas);

	TreeView_Expand(_hsidebar, hitem_bookmarks, TVE_EXPAND);
*/
}


MainFrame::MainFrame(HWND hwnd)
 :	super(hwnd)
{
	_split_pos = DEFAULT_SPLIT_POS;
	_last_split = DEFAULT_SPLIT_POS;
}

HWND MainFrame::Create()
{
	HMENU hMenuFrame = LoadMenu(g_hInstance, MAKEINTRESOURCE(IDM_SDIFRAME));

	return Window::Create(WINDOW_CREATOR(MainFrame), 0,
				(LPCTSTR)(int)g_hframeClass, ResString(IDS_TITLE), WS_OVERLAPPEDWINDOW,
				CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
				0/*hwndDesktop*/, hMenuFrame);
}

/*@@
HWND MainFrame::Create(LPCTSTR url)
{
	HWND hFrame = Create();
	if (!hFrame)
		return 0;

	ShowWindow(hFrame, SW_SHOW);

	MainFrame* pFrame = GET_WINDOW(MainFrame, hFrame);

	if (pFrame)
		pFrame->set_url(url);

	return hFrame;
}
*/

LRESULT MainFrame::WndProc(UINT nmsg, WPARAM wparam, LPARAM lparam)
{
	switch(nmsg) {
	  case WM_SIZE:
		resize_frame(LOWORD(lparam), HIWORD(lparam));
		break;

	  case WM_PAINT: {
		PaintCanvas canvas(_hwnd);

		if (_left_hwnd && _right_hwnd) {
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
				resize_children();
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

	  case PM_OPEN_WINDOW: {CONTEXT("MainFrame PM_OPEN_WINDOW");
		LPCTSTR url = (LPCTSTR)lparam;

		if (!url || !*url)
#ifdef _DEBUG
			url = TEXT("http://localhost");
#else
			url = TEXT("about:blank");
#endif

		if (!_right_hwnd) {
			_right_hwnd = create_webchildwindow(WebChildWndInfo(_hwnd, url));
			resize_children();
		} else
			set_url(url);
		return TRUE;}	// success

	  default: def:
		return super::WndProc(nmsg, wparam, lparam);
	}

	return 0;
}

int MainFrame::Command(int id, int code)
{
	if (_right_hwnd)
		if (SendMessage(_right_hwnd, PM_DISPATCH_COMMAND, MAKELONG(id,code), 0))
			return 0;

	return super::Command(id, code);
}

void MainFrame::resize_frame(int cx, int cy)
{
	if (cy <= 0)
		return;	// avoid resizing children when receiving RBN_AUTOSIZE while getting minimized

	RECT rect = {0, 0, cx, cy};

	if (_hwndrebar) {
		int height = ClientRect(_hwndrebar).bottom;
		MoveWindow(_hwndrebar, rect.left, rect.top, rect.right-rect.left, height, TRUE);
		rect.top += height;
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

	if (IsWindowVisible(_haddressedit)) {
		ClientRect rt(_haddressedit);
		rect.bottom -= rt.bottom;

		SetWindowPos(_haddressedit, 0, 0, rect.bottom, rect.right-rect.left, rt.bottom, SWP_NOACTIVATE|SWP_NOZORDER);
	}

	if (IsWindowVisible(_hsidebar)) {
		WindowRect rt(_hsidebar);
		rect.left += rt.right-rt.left;

		SetWindowPos(_hsidebar, 0, -1, rect.top-1, rt.right-rt.left, rect.bottom-rect.top+1, SWP_NOACTIVATE|SWP_NOZORDER);
	}

	_clnt_rect = rect;

	resize_children();
}

void MainFrame::resize_children()
{
	HDWP hdwp = BeginDeferWindowPos(2);

	int cx = _clnt_rect.left;

	if (_left_hwnd) {
		cx = _split_pos + SPLIT_WIDTH/2;

		hdwp = DeferWindowPos(hdwp, _left_hwnd, 0, _clnt_rect.left, _clnt_rect.top, _split_pos-SPLIT_WIDTH/2-_clnt_rect.left, _clnt_rect.bottom-_clnt_rect.top, SWP_NOZORDER|SWP_NOACTIVATE);
	} else {
		//_split_pos = 0;
		cx = 0;
	}

	if (_right_hwnd)
		hdwp = DeferWindowPos(hdwp, _right_hwnd, 0, _clnt_rect.left+cx, _clnt_rect.top, _clnt_rect.right-cx, _clnt_rect.bottom-_clnt_rect.top, SWP_NOZORDER|SWP_NOACTIVATE);

	EndDeferWindowPos(hdwp);
}

void MainFrame::update_clnt_rect()
{
	ClientRect rect(_hwnd);

	resize_frame(rect.right, rect.bottom);
}

void MainFrame::set_url(LPCTSTR url)
{
	if (_url != url) {
		_url = url;

		SetWindowText(_haddressedit, url); //SendMessage(_hwndFrame, PM_URL_CHANGED, 0, (LPARAM)url);
	}
}

bool MainFrame::go_to(LPCTSTR url, bool new_window)
{
	if (_right_hwnd) {
		SendMessage(_right_hwnd, PM_JUMP_TO_URL, 0, (LPARAM)url);
		return true;
	}

	return false;
}
