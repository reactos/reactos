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
 // Explorer clone
 //
 // traynotify.cpp
 //
 // Martin Fuchs, 22.08.2003
 //


#include "../utility/utility.h"

#include "../explorer.h"
#include "../globals.h"

#include "../explorer_intres.h"

#include "traynotify.h"


#include "../notifyhook/notifyhook.h"

NotifyHook::NotifyHook()
 :	WM_GETMODULEPATH(InstallNotifyHook())
{
}

NotifyHook::~NotifyHook()
{
	DeinstallNotifyHook();
}

void NotifyHook::GetModulePath(HWND hwnd, HWND hwndCallback)
{
	PostMessage(hwnd, WM_GETMODULEPATH, (WPARAM)hwndCallback, 0);
}

bool NotifyHook::ModulePathCopyData(LPARAM lparam, HWND* phwnd, String& path)
{
	char buffer[MAX_PATH];

	int l = GetWindowModulePathCopyData(lparam, phwnd, buffer, MAX_PATH);

	if (l) {
		path.assign(buffer, l);
		return true;
	} else
		return false;
}


NotifyIconIndex::NotifyIconIndex(NOTIFYICONDATA* pnid)
{
	_hWnd = pnid->hWnd;
	_uID = pnid->uID;

	 // special handling for windows task manager
	if ((int)_uID < 0)
		_uID = 0;
}

NotifyIconIndex::NotifyIconIndex()
{
	_hWnd = 0;
	_uID = 0;
}


NotifyInfo::NotifyInfo()
{
	_idx = -1;
	_hIcon = 0;
	_dwState = 0;
	_uCallbackMessage = 0;
	_version = 0;

	_mode = NIM_AUTO;
	_lastChange = GetTickCount();
}


 // WCHAR versions von NOTIFYICONDATA
#define	NID_SIZE_W6	 sizeof(NOTIFYICONDATAW)										// _WIN32_IE = 0x600
#define	NID_SIZE_W5	(sizeof(NOTIFYICONDATAW)-sizeof(GUID))							// _WIN32_IE = 0x500
#define	NID_SIZE_W3	(sizeof(NOTIFYICONDATAW)-sizeof(GUID)-(128-64)*sizeof(WCHAR))	// _WIN32_IE < 0x500

 // CHAR versions von NOTIFYICONDATA
#define	NID_SIZE_A6	 sizeof(NOTIFYICONDATAA)
#define	NID_SIZE_A5	(sizeof(NOTIFYICONDATAA)-sizeof(GUID))
#define	NID_SIZE_A3	(sizeof(NOTIFYICONDATAA)-sizeof(GUID)-(128-64)*sizeof(CHAR))

NotifyInfo& NotifyInfo::operator=(NOTIFYICONDATA* pnid)
{
	_hWnd = pnid->hWnd;
	_uID = pnid->uID;

	if (pnid->uFlags & NIF_MESSAGE)
		_uCallbackMessage = pnid->uCallbackMessage;

	if (pnid->uFlags & NIF_ICON) {
		 // Some applications destroy the icon immediatelly after completing the
		 // NIM_ADD/MODIFY message, so we have to make a copy of it.
		if (_hIcon)
			DestroyIcon(_hIcon);

		_hIcon = (HICON) CopyImage(pnid->hIcon, IMAGE_ICON, 16, 16, 0);
	}

#ifdef NIF_STATE	// as of 21.08.2003 missing in MinGW headers
	if (pnid->uFlags & NIF_STATE)
		_dwState = (_dwState&~pnid->dwStateMask) | (pnid->dwState&pnid->dwStateMask);
#endif

	 // store tool tip text
	if (pnid->uFlags & NIF_TIP)
		if (pnid->cbSize==NID_SIZE_W6 || pnid->cbSize==NID_SIZE_W5 || pnid->cbSize==NID_SIZE_W3) {
			 // UNICODE version of NOTIFYICONDATA structure
			LPCWSTR txt = (LPCWSTR)pnid->szTip;
			int max_len = pnid->cbSize==NID_SIZE_W3? 64: 128;

			 // get tooltip string length
			int l = 0;
			for(; l<max_len; ++l)
				if (!txt[l])
					break;

			_tipText.assign(txt, l);
		} else if (pnid->cbSize==NID_SIZE_A6 || pnid->cbSize==NID_SIZE_A5 || pnid->cbSize==NID_SIZE_A3) {
			LPCSTR txt = (LPCSTR)pnid->szTip;
			int max_len = pnid->cbSize==NID_SIZE_A3? 64: 128;

			int l = 0;
			for(int l=0; l<max_len; ++l)
				if (!txt[l])
					break;

			_tipText.assign(txt, l);
		}

	TCHAR title[MAX_PATH];

	if (GetWindowText(_hWnd, title, MAX_PATH))
		_windowTitle = title;

	create_name();

	///@todo test for real changes
	_lastChange = GetTickCount();

	return *this;
}


NotifyArea::NotifyArea(HWND hwnd)
 :	super(hwnd),
	_tooltip(hwnd)
{
	_next_idx = 0;
	_clock_width = 0;
	_last_icon_count = 0;
	_show_hidden = false;

	read_config();
}

NotifyArea::~NotifyArea()
{
	KillTimer(_hwnd, 0);

	write_config();
}

void NotifyArea::read_config()
{
	 // read notification icon settings from XML configuration
	XMLPos pos(&g_Globals._cfg);

	if (pos.go_down("explorer-cfg")) {
		if (pos.go_down("notify-icons")) {
			_show_hidden = XMLBool(pos, "option", "show-hidden");

			XMLChildrenFilter icons(pos, "icon");

			for(XMLChildrenFilter::iterator it=icons.begin(); it!=icons.end(); ++it) {
				const XMLNode& node = **it;

				NotifyIconConfig cfg;

				cfg._name = node["name"];
				cfg._tipText = node["text"];
				cfg._windowTitle = node["window"];
				cfg._modulePath = node["module"];
				const string& mode = node["show"];

				if (mode == "show")
					cfg._mode = NIM_SHOW;
				else if (mode == "hide")
					cfg._mode = NIM_HIDE;
				else //if (mode == "auto")
					cfg._mode = NIM_HIDE;

				_cfg.push_back(cfg);
			}

			pos.back();
		}

		pos.back();
	}
}

void NotifyArea::write_config()
{
	 // write notification icon settings to XML configuration file
	XMLPos pos(&g_Globals._cfg);

	pos.create("explorer-cfg");
	pos.create("notify-icons");

	XMLBoolRef(pos, "option", "show-hidden") = _show_hidden;

	for(NotifyIconCfgList::iterator it=_cfg.begin(); it!=_cfg.end(); ++it) {
		NotifyIconConfig& cfg = *it;

		 // search for the corresponding node using the original name
		pos.create("icon", "name", cfg._name);

		 // refresh name
		cfg.create_name();

		pos["name"] = cfg._name;
		pos["text"] = cfg._tipText;
		pos["window"] = cfg._windowTitle;
		pos["module"] = cfg._modulePath;
		pos["show"] = string_from_mode(cfg._mode);

		pos.back();
	}

	pos.back();
	pos.back();
}

LRESULT NotifyArea::Init(LPCREATESTRUCT pcs)
{
	if (super::Init(pcs))
		return 1;

#ifndef __MINGW32__	// SHRestricted() missing in MinGW (as of 29.10.2003)
	if (!g_Globals._SHRestricted || !SHRestricted(REST_HIDECLOCK))
#endif
	{
		HKEY hkeyStuckRects = 0;
		DWORD buffer[10];
		DWORD len = sizeof(buffer);

		bool hide_clock = false;

		 // check if the clock should be hidden
		if (!RegOpenKey(HKEY_CURRENT_USER, TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\StuckRects2"), &hkeyStuckRects) &&
			!RegQueryValueEx(hkeyStuckRects, TEXT("Settings"), 0, NULL, (LPBYTE)buffer, &len) &&
			len==sizeof(buffer) && buffer[0]==sizeof(buffer))
			hide_clock = buffer[2] & 0x08? true: false;

		if (!hide_clock) {
			 // create clock window
			_hwndClock = ClockWindow::Create(_hwnd);

			if (_hwndClock) {
				ClientRect clock_size(_hwndClock);
				_clock_width = clock_size.right;
			}
		}

		if (hkeyStuckRects)
			RegCloseKey(hkeyStuckRects);
	}

	SetTimer(_hwnd, 0, 1000, NULL);

	return 0;
}

HWND NotifyArea::Create(HWND hwndParent)
{
	static BtnWindowClass wcTrayNotify(CLASSNAME_TRAYNOTIFY, CS_DBLCLKS);

	ClientRect clnt(hwndParent);

#ifndef _ROS_
	return Window::Create(WINDOW_CREATOR(NotifyArea), WS_EX_STATICEDGE,
							wcTrayNotify, TITLE_TRAYNOTIFY, WS_CHILD|WS_VISIBLE,
							clnt.right-(NOTIFYAREA_WIDTH_DEF+1), 1, NOTIFYAREA_WIDTH_DEF, clnt.bottom-2, hwndParent);
#else
	return Window::Create(WINDOW_CREATOR(NotifyArea), 0,
							wcTrayNotify, TITLE_TRAYNOTIFY, WS_CHILD|WS_VISIBLE,
							clnt.right-(NOTIFYAREA_WIDTH_DEF+1), 1, NOTIFYAREA_WIDTH_DEF, clnt.bottom-2, hwndParent);
#endif
}

LRESULT NotifyArea::WndProc(UINT nmsg, WPARAM wparam, LPARAM lparam)
{
	switch(nmsg) {
	  case WM_PAINT:
		Paint();
		break;

	  case WM_TIMER: {
		Refresh();

		ClockWindow* clock_window = GET_WINDOW(ClockWindow, _hwndClock);

		if (clock_window)
			clock_window->TimerTick();
		break;}

	  case PM_REFRESH:
		Refresh(true);
		break;

	  case WM_SIZE: {
		int cx = LOWORD(lparam);
		SetWindowPos(_hwndClock, 0, cx-_clock_width, 0, 0, 0, SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE);
		break;}

	  case PM_GET_WIDTH:
		return _sorted_icons.size()*NOTIFYICON_DIST + NOTIFYAREA_SPACE + _clock_width;

	  case WM_CONTEXTMENU: {
		Point pt(lparam);
		ScreenToClient(_hwnd, &pt);

		if (IconHitTest(pt) == _sorted_icons.end()) { // display menu only when no icon clicked
			PopupMenu menu(IDM_NOTIFYAREA);
			SetMenuDefaultItem(menu, 0, MF_BYPOSITION);
			CheckMenuItem(menu, ID_SHOW_HIDDEN_ICONS, MF_BYCOMMAND|(_show_hidden?MF_CHECKED:MF_UNCHECKED));
			menu.TrackPopupMenu(_hwnd, MAKEPOINTS(lparam));
		}
		break;}

	  case WM_COPYDATA: {	// receive NotifyHook answers
		String path;
		HWND hwnd;

		if (_hook.ModulePathCopyData(lparam, &hwnd, path))
			_window_modules[hwnd] = path;
		break;}

	  default:
		if (nmsg>=WM_MOUSEFIRST && nmsg<=WM_MOUSELAST) {
			 // close startup menu and other popup menus
			 // This functionality is missing in MS Windows.
			if (nmsg==WM_LBUTTONDOWN || nmsg==WM_MBUTTONDOWN || nmsg==WM_RBUTTONDOWN
#ifdef WM_XBUTTONDOWN
				|| nmsg==WM_XBUTTONDOWN
#endif
				)
				CancelModes();

			NotifyIconSet::const_iterator found = IconHitTest(Point(lparam));

			if (found != _sorted_icons.end()) {
				const NotifyInfo& entry = const_cast<NotifyInfo&>(*found);	// Why does GCC 3.3 need this additional const_cast ?!

				 // set activation time stamp
				if (nmsg == WM_LBUTTONDOWN ||	// Some programs need PostMessage() instead of SendMessage().
					nmsg == WM_MBUTTONDOWN ||	// So call SendMessage() only for BUTTONUP and BLCLK messages
#ifdef WM_XBUTTONDOWN
					nmsg == WM_XBUTTONDOWN ||
#endif
					nmsg == WM_RBUTTONDOWN) {
					_icon_map[entry]._lastChange = GetTickCount();
				}

				 // Notify the message if the owner is still alive
				if (IsWindow(entry._hWnd)) {
					if (nmsg == WM_MOUSEMOVE ||		// avoid to call blocking SendMessage() for merely moving the mouse over icons
						nmsg == WM_LBUTTONDOWN ||	// Some programs need PostMessage() instead of SendMessage().
						nmsg == WM_MBUTTONDOWN ||	// So call SendMessage() only for BUTTONUP and BLCLK messages
#ifdef WM_XBUTTONDOWN
						nmsg == WM_XBUTTONDOWN ||
#endif
						nmsg == WM_RBUTTONDOWN)
						PostMessage(entry._hWnd, entry._uCallbackMessage, entry._uID, nmsg);
					else {
						 // allow SetForegroundWindow() in client process
						DWORD pid;

						if (GetWindowThreadProcessId(entry._hWnd, &pid)) {
							 // bind dynamically to AllowSetForegroundWindow() to be compatible to WIN98
							static DynamicFct<BOOL(WINAPI*)(DWORD)> AllowSetForegroundWindow(TEXT("USER32"), "AllowSetForegroundWindow");

							if (AllowSetForegroundWindow)
								(*AllowSetForegroundWindow)(pid);
						}

						SendMessage(entry._hWnd, entry._uCallbackMessage, entry._uID, nmsg);
					}
				}
				else if (_icon_map.erase(entry))	// delete icons without valid owner window
					UpdateIcons();
			}
		}

		return super::WndProc(nmsg, wparam, lparam);
	}

	return 0;
}

int NotifyArea::Command(int id, int code)
{
	switch(id) {
	  case ID_SHOW_HIDDEN_ICONS:
		_show_hidden = !_show_hidden;
		UpdateIcons();
		break;

	  case ID_CONFIG_NOTIFYAREA:
		Dialog::DoModal(IDD_NOTIFYAREA, WINDOW_CREATOR(TrayNotifyDlg), GetParent(_hwnd));
		break;

	  case ID_CONFIG_TIME:
		launch_cpanel(_hwnd, TEXT("timedate.cpl"));
		break;

	  default:
		SendParent(WM_COMMAND, MAKELONG(id,code), 0);
	}

	return 0;
}

int NotifyArea::Notify(int id, NMHDR* pnmh)
{
	if (pnmh->code == TTN_GETDISPINFO) {
		LPNMTTDISPINFO pdi = (LPNMTTDISPINFO)pnmh;

		Point pt(GetMessagePos());
		ScreenToClient(_hwnd, &pt);

		NotifyIconSet::iterator found = IconHitTest(pt);

		if (found != _sorted_icons.end()) {
			NotifyInfo& entry = const_cast<NotifyInfo&>(*found);	// Why does GCC 3.3 need this additional const_cast ?!

			pdi->lpszText = (LPTSTR)entry._tipText.c_str();
		}
	}

	return 0;
}

void NotifyArea::CancelModes()
{
	PostMessage(HWND_BROADCAST, WM_CANCELMODE, 0, 0);

	for(NotifyIconSet::const_iterator it=_sorted_icons.begin(); it!=_sorted_icons.end(); ++it)
		PostMessage(it->_hWnd, WM_CANCELMODE, 0, 0);
}

LRESULT NotifyArea::ProcessTrayNotification(int notify_code, NOTIFYICONDATA* pnid)
{
	switch(notify_code) {
	  case NIM_ADD:
	  case NIM_MODIFY:
		if ((int)pnid->uID >= 0) {	///@todo This is a fix for Windows Task Manager.
			NotifyInfo& entry = _icon_map[pnid] = pnid;

			 // a new entry?
			if (entry._idx == -1)
				entry._idx = ++_next_idx;

#if NOTIFYICON_VERSION>=3	// as of 21.08.2003 missing in MinGW headers
			if (DetermineHideState(entry) && entry._mode==NIM_HIDE)
				entry._dwState |= NIS_HIDDEN;
#endif

			UpdateIcons();	///@todo call only if really changes occurred

			return TRUE;
		}
		break;

	  case NIM_DELETE: {
		NotifyIconMap::iterator found = _icon_map.find(pnid);

		if (found != _icon_map.end()) {
			if (found->second._hIcon)
				DestroyIcon(found->second._hIcon);
			_icon_map.erase(found);
			UpdateIcons();
			return TRUE;
		}
		break;}

#if NOTIFYICON_VERSION>=3	// as of 21.08.2003 missing in MinGW headers
	  case NIM_SETFOCUS:
		SetForegroundWindow(_hwnd);
		return TRUE;

	  case NIM_SETVERSION:
		NotifyIconMap::iterator found = _icon_map.find(pnid);

		if (found != _icon_map.end()) {
			found->second._version = pnid->UNION_MEMBER(uVersion);
			return TRUE;
		} else
			return FALSE;
#endif
	}

	return FALSE;
}

void NotifyArea::UpdateIcons()
{
	_sorted_icons.clear();

	 // sort icon infos by display index
	for(NotifyIconMap::const_iterator it=_icon_map.begin(); it!=_icon_map.end(); ++it) {
		const NotifyInfo& entry = it->second;

#ifdef NIF_STATE	// as of 21.08.2003 missing in MinGW headers
		if (_show_hidden || !(entry._dwState & NIS_HIDDEN))
#endif
			_sorted_icons.insert(entry);
	}

	 // sync tooltip areas to current icon number
	if (_sorted_icons.size() != _last_icon_count) {
		RECT rect = {2, 3, 2+16, 3+16};
		size_t icon_cnt = _sorted_icons.size();

		size_t tt_idx = 0;
		while(tt_idx < icon_cnt) {
			_tooltip.add(_hwnd, tt_idx++, rect);

			rect.left += NOTIFYICON_DIST;
			rect.right += NOTIFYICON_DIST;
		}

		while(tt_idx < _last_icon_count)
			_tooltip.remove(_hwnd, tt_idx++);

		_last_icon_count = _sorted_icons.size();
	}

	SendMessage(GetParent(_hwnd), PM_RESIZE_CHILDREN, 0, 0);

	InvalidateRect(_hwnd, NULL, FALSE);	// refresh icon display
	UpdateWindow(_hwnd);
}

void NotifyArea::Paint()
{
	BufferedPaintCanvas canvas(_hwnd);

	 // first fill with the background color
	FillRect(canvas, &canvas.rcPaint, GetSysColorBrush(COLOR_BTNFACE));

#ifdef _ROS_
	DrawEdge(canvas, ClientRect(_hwnd), BDR_SUNKENOUTER, BF_RECT);
#endif

	 // draw icons
	int x = 2;
	int y = 3;

	for(NotifyIconSet::const_iterator it=_sorted_icons.begin(); it!=_sorted_icons.end(); ++it) {
		DrawIconEx(canvas, x, y, it->_hIcon, 16, 16, 0, 0, DI_NORMAL);

		x += NOTIFYICON_DIST;
	}
}

void NotifyArea::Refresh(bool update)
{
	 // Look for task icons without valid owner window.
	 // This is an extended feature missing in MS Windows.
	for(NotifyIconSet::const_iterator it=_sorted_icons.begin(); it!=_sorted_icons.end(); ++it) {
		const NotifyInfo& entry = *it;

		if (!IsWindow(entry._hWnd))
			if (_icon_map.erase(entry))	// delete icons without valid owner window
				++update;
	}

	DWORD now = GetTickCount();

	 // handle icon hiding
	for(NotifyIconMap::iterator it=_icon_map.begin(); it!=_icon_map.end(); ++it) {
		NotifyInfo& entry = it->second;

		DetermineHideState(entry);

		switch(entry._mode) {
		  case NIM_HIDE:
			if (!(entry._dwState & NIS_HIDDEN)) {
				entry._dwState |= NIS_HIDDEN;
				++update;
			}
			break;

		  case NIM_SHOW:
			if (entry._dwState&NIS_HIDDEN) {
				entry._dwState &= ~NIS_HIDDEN;
				++update;
			}
			break;

		  case NIM_AUTO:
			 // automatically hide icons after long periods of inactivity
			if (!(entry._dwState & NIS_HIDDEN))
				if (now-entry._lastChange > ICON_AUTOHIDE_SECONDS*1000) {
					entry._dwState |= NIS_HIDDEN;
					++update;
				}
			break;
		}
	}

	if (update)
		UpdateIcons();
}

 /// search for a icon at a given client coordinate position
NotifyIconSet::iterator NotifyArea::IconHitTest(const POINT& pos)
{
	if (pos.y<2 || pos.y>=2+16)
		return _sorted_icons.end();

	NotifyIconSet::iterator it = _sorted_icons.begin();

	int x = 2;

	for(; it!=_sorted_icons.end(); ++it) {
		//NotifyInfo& entry = const_cast<NotifyInfo&>(*it);	// Why does GCC 3.3 need this additional const_cast ?!

		if (pos.x>=x && pos.x<x+16)
			break;

		x += NOTIFYICON_DIST;
	}

	return it;
}


void NotifyIconConfig::create_name()
{
	_name = FmtString(TEXT("'%s' - '%s' - '%s'"), _tipText.c_str(), _windowTitle.c_str(), _modulePath.c_str());
}


#if NOTIFYICON_VERSION>=3	// as of 21.08.2003 missing in MinGW headers

bool NotifyIconConfig::match(const NotifyIconConfig& props) const
{
	if (!_tipText.empty() && !props._tipText.empty())
		if (props._tipText == _tipText)
			return true;

	if (!_windowTitle.empty() && !props._windowTitle.empty())
		if (_tcsstr(props._windowTitle, _windowTitle))
			return true;

	if (!_modulePath.empty() && !props._modulePath.empty())
		if (!_tcsicmp(props._modulePath, _modulePath))
			return true;

	return false;
}

bool NotifyArea::DetermineHideState(NotifyInfo& entry)
{
	if (entry._modulePath.empty()) {
		const String& modulePath = _window_modules[entry._hWnd];

		 // request module path for new windows (We will get an asynchronous answer by a WM_COPYDATA message.)
		if (!modulePath.empty())
			entry._modulePath = modulePath;
		else
			_hook.GetModulePath(entry._hWnd, _hwnd);
	}

	for(NotifyIconCfgList::const_iterator it=_cfg.begin(); it!=_cfg.end(); ++it) {
		const NotifyIconConfig& cfg = *it;

		if (cfg.match(entry)) {
			entry._mode = cfg._mode;
			return true;
		}
	}

	return false;
}

#endif


String string_from_mode(NOTIFYICONMODE mode)
{
	switch(mode) {
	  case NIM_SHOW:
		return ResString(IDS_NOTIFY_SHOW);

	  case NIM_HIDE:
		return ResString(IDS_NOTIFY_HIDE);

	  default:	//case NIM_AUTO
		return ResString(IDS_NOTIFY_AUTOHIDE);
	}
}


TrayNotifyDlg::TrayNotifyDlg(HWND hwnd)
 :	super(hwnd),
	_tree_ctrl(GetDlgItem(hwnd, IDC_NOTIFY_ICONS)),
	_himl(ImageList_Create(GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), ILC_COLOR24, 2, 0)),
	_pNotifyArea(static_cast<NotifyArea*>(Window::get_window((HWND)SendMessage(g_Globals._hwndDesktopBar, PM_GET_NOTIFYAREA, 0, 0))))
{
	_selectedItem = 0;

	if (_pNotifyArea) {
		 // save original icon states and configuration data
		for(NotifyIconMap::const_iterator it=_pNotifyArea->_icon_map.begin(); it!=_pNotifyArea->_icon_map.end(); ++it)
			_icon_states_org[it->first] = IconStatePair(it->second._mode, it->second._dwState);

		_cfg_org = _pNotifyArea->_cfg;
	}

	SetWindowIcon(hwnd, IDI_REACTOS/*IDI_SEARCH*/);

	_haccel = LoadAccelerators(g_Globals._hInstance, MAKEINTRESOURCE(IDA_TRAYNOTIFY));

	{
	WindowCanvas canvas(_hwnd);
	HBRUSH hbkgnd = GetStockBrush(WHITE_BRUSH);

	ImageList_AddAlphaIcon(_himl, SmallIcon(IDI_DOT), hbkgnd, canvas);
	ImageList_AddAlphaIcon(_himl, SmallIcon(IDI_DOT_TRANS), hbkgnd, canvas);
	ImageList_AddAlphaIcon(_himl, SmallIcon(IDI_DOT_RED), hbkgnd, canvas);
	}

	TreeView_SetImageList(_tree_ctrl, _himl, TVSIL_NORMAL);

	_resize_mgr.Add(IDC_NOTIFY_ICONS,	RESIZE);
	_resize_mgr.Add(IDC_LABEL1,			MOVE_Y);
	_resize_mgr.Add(IDC_NOTIFY_TOOLTIP,	RESIZE_X|MOVE_Y);
	_resize_mgr.Add(IDC_LABEL2,			MOVE_Y);
	_resize_mgr.Add(IDC_NOTIFY_TITLE,	RESIZE_X|MOVE_Y);
	_resize_mgr.Add(IDC_LABEL3,			MOVE_Y);
	_resize_mgr.Add(IDC_NOTIFY_MODULE,	RESIZE_X|MOVE_Y);

	_resize_mgr.Add(IDC_LABEL4,			MOVE_Y);
	_resize_mgr.Add(IDC_NOTIFY_SHOW,	MOVE_Y);
	_resize_mgr.Add(IDC_NOTIFY_HIDE,	MOVE_Y);
	_resize_mgr.Add(IDC_NOTIFY_AUTOHIDE,MOVE_Y);

	_resize_mgr.Add(IDC_PICTURE,		MOVE);

	_resize_mgr.Add(IDOK,				MOVE);
	_resize_mgr.Add(IDCANCEL,			MOVE);

	_resize_mgr.Resize(+150, +200);

	Refresh();

	SetTimer(_hwnd, 0, 3000, NULL);
	register_pretranslate(hwnd);
}

TrayNotifyDlg::~TrayNotifyDlg()
{
	KillTimer(_hwnd, 0);
	unregister_pretranslate(_hwnd);
	ImageList_Destroy(_himl);
}

void TrayNotifyDlg::Refresh()
{
	///@todo refresh incrementally

	HiddenWindow hide(_tree_ctrl);

	TreeView_DeleteAllItems(_tree_ctrl);

	TV_INSERTSTRUCT tvi;

	tvi.hParent = 0;
	tvi.hInsertAfter = TVI_LAST;

	TV_ITEM& tv = tvi.item;
	tv.mask = TVIF_TEXT|TVIF_IMAGE|TVIF_SELECTEDIMAGE;

	ResString str_cur(IDS_ITEMS_CUR);
	tv.pszText = (LPTSTR)str_cur.c_str();
	tv.iSelectedImage = tv.iImage = 0;	// IDI_DOT
	_hitemCurrent = TreeView_InsertItem(_tree_ctrl, &tvi);

	ResString str_conf(IDS_ITEMS_CONFIGURED);
	tv.pszText = (LPTSTR)str_conf.c_str();
	tv.iSelectedImage = tv.iImage = 2;	// IDI_DOT_RED
	_hitemConfig = TreeView_InsertItem(_tree_ctrl, &tvi);

	tvi.hParent = _hitemCurrent;

	ResString str_visible(IDS_ITEMS_VISIBLE);
	tv.pszText = (LPTSTR)str_visible.c_str();
	tv.iSelectedImage = tv.iImage = 0;	// IDI_DOT
	_hitemCurrent_visible = TreeView_InsertItem(_tree_ctrl, &tvi);

	ResString str_hidden(IDS_ITEMS_HIDDEN);
	tv.pszText = (LPTSTR)str_hidden.c_str();
	tv.iSelectedImage = tv.iImage = 1;	// IDI_DOT_TRANS
	_hitemCurrent_hidden = TreeView_InsertItem(_tree_ctrl, &tvi);

	if (_pNotifyArea) {
		tv.mask |= TVIF_PARAM;

		WindowCanvas canvas(_hwnd);

		 // insert current (visible and hidden) items
		for(NotifyIconMap::const_iterator it=_pNotifyArea->_icon_map.begin(); it!=_pNotifyArea->_icon_map.end(); ++it) {
			const NotifyInfo& entry = it->second;

			InsertItem(entry._dwState&NIS_HIDDEN? _hitemCurrent_hidden: _hitemCurrent_visible, TVI_LAST, entry, canvas);
		}

		 // insert configured items in tree view
		const NotifyIconCfgList& cfg = _pNotifyArea->_cfg;
		for(NotifyIconCfgList::const_iterator it=cfg.begin(); it!=cfg.end(); ++it) {
			const NotifyIconConfig& cfg_entry = *it;

			HICON hicon = 0;

			if (!cfg_entry._modulePath.empty()) {
				if ((int)ExtractIconEx(cfg_entry._modulePath, 0, NULL, &hicon, 1) <= 0)
					hicon = 0;

				if (!hicon) {
					SHFILEINFO sfi;

					if (SHGetFileInfo(cfg_entry._modulePath, 0, &sfi, sizeof(sfi), SHGFI_ICON|SHGFI_SMALLICON))
						hicon = sfi.hIcon;
				}
			}

			InsertItem(_hitemConfig, TVI_SORT, cfg_entry, canvas, hicon, cfg_entry._mode);

			if (hicon)
				DestroyIcon(hicon);
		}

		 // insert new configuration entry
	}

	TreeView_Expand(_tree_ctrl, _hitemCurrent_visible, TVE_EXPAND);
	TreeView_Expand(_tree_ctrl, _hitemCurrent_hidden, TVE_EXPAND);
	TreeView_Expand(_tree_ctrl, _hitemCurrent, TVE_EXPAND);
	TreeView_Expand(_tree_ctrl, _hitemConfig, TVE_EXPAND);

	TreeView_EnsureVisible(_tree_ctrl, _hitemCurrent_visible);
}

void TrayNotifyDlg::InsertItem(HTREEITEM hparent, HTREEITEM after, const NotifyInfo& entry, HDC hdc)
{
	InsertItem(hparent, after, entry, hdc, entry._hIcon, entry._mode);
}

void TrayNotifyDlg::InsertItem(HTREEITEM hparent, HTREEITEM after, const NotifyIconConfig& entry,
								HDC hdc, HICON hicon, NOTIFYICONMODE mode)
{
	String mode_str = string_from_mode(mode);

	switch(mode) {
	  case NIM_SHOW:	mode_str = ResString(IDS_NOTIFY_SHOW);		break;
	  case NIM_HIDE:	mode_str = ResString(IDS_NOTIFY_HIDE);		break;
	  case NIM_AUTO:	mode_str = ResString(IDS_NOTIFY_AUTOHIDE);
	}

	FmtString txt(TEXT("%s  -  %s  [%s]"), entry._tipText.c_str(), entry._windowTitle.c_str(), mode_str.c_str());

	TV_INSERTSTRUCT tvi;

	tvi.hParent = hparent;
	tvi.hInsertAfter = after;

	TV_ITEM& tv = tvi.item;
	tv.mask = TVIF_TEXT|TVIF_IMAGE|TVIF_SELECTEDIMAGE|TVIF_PARAM;

	tv.lParam = (LPARAM)&entry;
	tv.pszText = (LPTSTR)txt.c_str();
	tv.iSelectedImage = tv.iImage = ImageList_AddAlphaIcon(_himl, hicon, GetStockBrush(WHITE_BRUSH), hdc);
	TreeView_InsertItem(_tree_ctrl, &tvi);
}

LRESULT TrayNotifyDlg::WndProc(UINT nmsg, WPARAM wparam, LPARAM lparam)
{
	switch(nmsg) {
	  case PM_TRANSLATE_MSG: {
		MSG* pmsg = (MSG*) lparam;

		if (TranslateAccelerator(_hwnd, _haccel, pmsg))
			return TRUE;

		return FALSE;}

	  case WM_TIMER:
		Refresh();
		break;

	  default:
		return super::WndProc(nmsg, wparam, lparam);
	}

	return 0;
}

int TrayNotifyDlg::Command(int id, int code)
{
	if (code == BN_CLICKED) {
		switch(id) {
		  case ID_REFRESH:
			Refresh();
			break;

		  case IDC_NOTIFY_SHOW:
			SetIconMode(NIM_SHOW);
			break;

		  case IDC_NOTIFY_HIDE:
			SetIconMode(NIM_HIDE);
			break;

		  case IDC_NOTIFY_AUTOHIDE:
			SetIconMode(NIM_AUTO);
			break;

		  case IDOK:
			EndDialog(_hwnd, id);
			break;

		  case IDCANCEL:
			 // rollback changes
			if (_pNotifyArea) {
				 // restore original icon states and configuration data
				_pNotifyArea->_cfg = _cfg_org;

				for(IconStateMap::const_iterator it=_icon_states_org.begin(); it!=_icon_states_org.end(); ++it) {
					NotifyInfo& info = _pNotifyArea->_icon_map[it->first];

					info._mode = it->second.first;
					info._dwState = it->second.second;
				}

				SendMessage(*_pNotifyArea, PM_REFRESH, 0, 0);
			}

			EndDialog(_hwnd, id);
			break;
		}

		return 0;
	}

	return 1;
}

int TrayNotifyDlg::Notify(int id, NMHDR* pnmh)
{
	switch(pnmh->code) {
	  case TVN_SELCHANGED: {
		NMTREEVIEW* pnmtv = (NMTREEVIEW*)pnmh;
		LPARAM lparam = pnmtv->itemNew.lParam;

		if (lparam) {
			const NotifyIconConfig& entry = *(NotifyIconConfig*)lparam;

			SetDlgItemText(_hwnd, IDC_NOTIFY_TOOLTIP, entry._tipText);
			SetDlgItemText(_hwnd, IDC_NOTIFY_TITLE, entry._windowTitle);
			SetDlgItemText(_hwnd, IDC_NOTIFY_MODULE, entry._modulePath);

			CheckRadioButton(_hwnd, IDC_NOTIFY_SHOW, IDC_NOTIFY_AUTOHIDE, IDC_NOTIFY_SHOW+entry._mode);

			HICON hicon = 0; //get_window_icon_big(entry._hWnd, false);

			 // If we could not find an icon associated with the owner window, try to load one from the owning module.
			if (!hicon && !entry._modulePath.empty()) {
				hicon = ExtractIcon(g_Globals._hInstance, entry._modulePath, 0);

				if (!hicon) {
					SHFILEINFO sfi;

					if (SHGetFileInfo(entry._modulePath, 0, &sfi, sizeof(sfi), SHGFI_ICON|SHGFI_LARGEICON))
						hicon = sfi.hIcon;
				}
			}

			if (hicon) {
				SendMessage(GetDlgItem(_hwnd, IDC_PICTURE), STM_SETICON, (LPARAM)hicon, 0);
				DestroyIcon(hicon);
			} else
				SendMessage(GetDlgItem(_hwnd, IDC_PICTURE), STM_SETICON, 0, 0);

			_selectedItem = pnmtv->itemNew.hItem;
		} else {
			/*
			SetDlgItemText(_hwnd, IDC_NOTIFY_TOOLTIP, NULL);
			SetDlgItemText(_hwnd, IDC_NOTIFY_TITLE, NULL);
			SetDlgItemText(_hwnd, IDC_NOTIFY_MODULE, NULL);
			*/
			CheckRadioButton(_hwnd, IDC_NOTIFY_SHOW, IDC_NOTIFY_AUTOHIDE, 0);
		}
		break;}
	}

	return 0;
}

void TrayNotifyDlg::SetIconMode(NOTIFYICONMODE mode)
{
	LPARAM lparam = TreeView_GetItemData(_tree_ctrl, _selectedItem);

	if (!lparam)
		return;

	NotifyIconConfig& entry = *(NotifyIconConfig*)lparam;

	if (entry._mode != mode) {
		entry._mode = mode;

		 // trigger refresh in notify area and this dialog
		if (_pNotifyArea)
			SendMessage(*_pNotifyArea, PM_REFRESH, 0, 0);
	}

	if (_pNotifyArea) {
		bool found = false;

		NotifyIconCfgList& cfg = _pNotifyArea->_cfg;
		for(NotifyIconCfgList::iterator it=cfg.begin(); it!=cfg.end(); ++it) {
			NotifyIconConfig& cfg_entry = *it;

			if (cfg_entry.match(entry)) {
				cfg_entry._mode = mode;
				++found;
				break;
			}
		}

		if (!found) {
			 // insert new configuration entry
			NotifyIconConfig cfg_entry = entry;

			cfg_entry._mode = mode;

			_pNotifyArea->_cfg.push_back(cfg_entry);
		}
	}

	Refresh();
	///@todo select treeview item at new position in tree view -> refresh HTREEITEM in _selectedItem
}


ClockWindow::ClockWindow(HWND hwnd)
 :	super(hwnd),
	_tooltip(hwnd)
{
	*_time = TEXT('\0');
	FormatTime();

	_tooltip.add(_hwnd, _hwnd);
}

HWND ClockWindow::Create(HWND hwndParent)
{
	static BtnWindowClass wcClock(CLASSNAME_CLOCKWINDOW, CS_DBLCLKS);

	ClientRect clnt(hwndParent);

	WindowCanvas canvas(hwndParent);
	FontSelection font(canvas, GetStockFont(DEFAULT_GUI_FONT));

	RECT rect = {0, 0, 0, 0};
	TCHAR buffer[16];

	if (!GetTimeFormat(LOCALE_USER_DEFAULT, TIME_NOSECONDS, NULL, NULL, buffer, sizeof(buffer)/sizeof(TCHAR)))
		_tcscpy(buffer, TEXT("00:00"));

	DrawText(canvas, buffer, -1, &rect, DT_SINGLELINE|DT_NOPREFIX|DT_CALCRECT);
	int clockwindowWidth = rect.right-rect.left + 4;

	return Window::Create(WINDOW_CREATOR(ClockWindow), 0,
							wcClock, NULL, WS_CHILD|WS_VISIBLE,
							clnt.right-(clockwindowWidth), 1, clockwindowWidth, clnt.bottom-2, hwndParent);
}

LRESULT ClockWindow::WndProc(UINT nmsg, WPARAM wparam, LPARAM lparam)
{
	switch(nmsg) {
	  case WM_PAINT:
		Paint();
		break;

	  case WM_LBUTTONDBLCLK:
		launch_cpanel(_hwnd, TEXT("timedate.cpl"));
		break;

	  default:
		return super::WndProc(nmsg, wparam, lparam);
	}

	return 0;
}

int ClockWindow::Notify(int id, NMHDR* pnmh)
{
	if (pnmh->code == TTN_GETDISPINFO) {
		LPNMTTDISPINFO pdi = (LPNMTTDISPINFO)pnmh;

		SYSTEMTIME systime;
		TCHAR buffer[64];

		GetLocalTime(&systime);
		GetDateFormat(LOCALE_USER_DEFAULT, DATE_LONGDATE, &systime, NULL, buffer, 64);

		_tcscpy(pdi->szText, buffer);
	}

	return 0;
}

void ClockWindow::TimerTick()
{
	if (FormatTime())
		InvalidateRect(_hwnd, NULL, TRUE);	// refresh displayed time
}

bool ClockWindow::FormatTime()
{
	TCHAR buffer[16];

	if (GetTimeFormat(LOCALE_USER_DEFAULT, TIME_NOSECONDS, NULL, NULL, buffer, sizeof(buffer)/sizeof(TCHAR)))
		if (_tcscmp(buffer, _time)) {
			_tcscpy(_time, buffer);
			return true;	// The text to display has changed.
		}

	return false;	// no change
}

void ClockWindow::Paint()
{
	PaintCanvas canvas(_hwnd);

#ifdef _ROS_
	DrawEdge(canvas, ClientRect(_hwnd), BDR_SUNKENOUTER, BF_TOP|BF_RIGHT);
#endif

	BkMode bkmode(canvas, TRANSPARENT);
	FontSelection font(canvas, GetStockFont(ANSI_VAR_FONT));

	DrawText(canvas, _time, -1, ClientRect(_hwnd), DT_SINGLELINE|DT_VCENTER|DT_NOPREFIX);
}
