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
 // taskbar.cpp
 //
 // Martin Fuchs, 16.08.2003
 //


#include "../utility/utility.h"
#include "../utility/shellclasses.h"

#include "../explorer.h"
#include "../globals.h"
#include "../externals.h"
#include "../explorer_intres.h"

#include "taskbar.h"


#define	IDC_START		0x1000
#define	IDC_LOGOFF		0x1001
#define	IDC_SHUTDOWN	0x1002
#define	IDC_LAUNCH		0x1003
#define	IDC_START_HELP	0x1004
#define	IDC_SEARCH		0x1005
#define	IDC_SETTINGS	0x1006
#define	IDC_DOCUMENTS	0x1007
#define	IDC_FAVORITES	0x1008
#define	IDC_PROGRAMS	0x1009
#define	IDC_EXPLORE		0x100A

#define	IDC_FIRST_APP	0x2000
#define	IDC_FIRST_MENU	0x3000


HWND InitializeExplorerBar(HINSTANCE hInstance)
{
	RECT rect;

	rect.left = -2;	// hide left border
#ifdef TASKBAR_AT_TOP
	rect.top = -2;	// hide top border
#else
	rect.top = GetSystemMetrics(SM_CYSCREEN) - TASKBAR_HEIGHT;
#endif
	rect.right = GetSystemMetrics(SM_CXSCREEN) + 2;
	rect.bottom = rect.top + TASKBAR_HEIGHT + 2;

	return Window::Create(WINDOW_CREATOR(DesktopBar), WS_EX_PALETTEWINDOW,
							BtnWindowClass(CLASSNAME_EXPLORERBAR), TITLE_EXPLORERBAR,
							WS_POPUP|WS_THICKFRAME|WS_CLIPCHILDREN|WS_VISIBLE,
							rect.left, rect.top, rect.right-rect.left, rect.bottom-rect.top, 0);
}


DesktopBar::DesktopBar(HWND hwnd)
 :	super(hwnd)
{
	_hwndTaskBar = 0;
	_hwndStartMenu = 0;
}

DesktopBar::~DesktopBar()
{
	 // exit application after destroying desktop window
	PostQuitMessage(0);
}


LRESULT DesktopBar::Init(LPCREATESTRUCT pcs)
{
	if (super::Init(pcs))
		return 1;

	 // create start button
	new PictureButton(Button(_hwnd, ResString(IDS_START), 2, 2, STARTBUTTON_WIDTH, TASKBAR_HEIGHT-8, IDC_START,
							 WS_VISIBLE|WS_CHILD|BS_PUSHBUTTON|BS_OWNERDRAW),
						SmallIcon(IDI_STARTMENU));

	 // create task bar
	_hwndTaskBar = Window::Create(WINDOW_CREATOR(TaskBar), 0,
							BtnWindowClass(CLASSNAME_TASKBAR), TITLE_TASKBAR, WS_CHILD|WS_VISIBLE,
							TASKBAR_LEFT, 0, ClientRect(_hwnd).right-TASKBAR_LEFT, TASKBAR_HEIGHT, _hwnd);

	TaskBar* taskbar = static_cast<TaskBar*>(Window::get_window(_hwndTaskBar));

	taskbar->_desktop_bar = this;

	return 0;
}


void DesktopBar::create_startmenu()
{
	WindowRect my_pos(_hwnd);

	_hwndStartMenu = StartMenuRoot::Create(my_pos.left, my_pos.top-4, _hwnd);
}


LRESULT DesktopBar::WndProc(UINT nmsg, WPARAM wparam, LPARAM lparam)
{
	switch(nmsg) {
	  case WM_NCHITTEST: {
		LRESULT res = super::WndProc(nmsg, wparam, lparam);
		if (res>=HTSIZEFIRST && res<=HTSIZELAST) {
#ifdef TASKBAR_AT_TOP
			if (res == HTBOTTOM)	// enable vertical resizing at the lower border
#else
			if (res == HTTOP)		// enable vertical resizing at the upper border
#endif
				return res;
			else
				return HTCLIENT;	// disable any other resizing
		}
		return res;}

	  case WM_SYSCOMMAND:
		if ((wparam&0xFFF0) == SC_SIZE) {
#ifdef TASKBAR_AT_TOP
			if (wparam == SC_SIZE+6)// enable vertical resizing at the lower border
#else
			if (wparam == SC_SIZE+3)// enable vertical resizing at the upper border
#endif
				goto def;
			else
				return 0;			// disable any other resizing
		}
		goto def;

	  case WM_SIZE:
		if (_hwndTaskBar)
			MoveWindow(_hwndTaskBar, TASKBAR_LEFT, 0, ClientRect(_hwnd).right-TASKBAR_LEFT, HIWORD(lparam), TRUE);
		break;

	  case WM_CLOSE:
		break;

	  default: def:
		return super::WndProc(nmsg, wparam, lparam);
	}

	return 0;
}


int DesktopBar::Command(int id, int code)
{
	switch(id) {
	  case IDC_START:
		create_startmenu();
		break;
	}

	return 0;
}


static HICON get_window_icon(HWND hwnd)
{
	HICON hicon = 0;

	SendMessageTimeout(hwnd, WM_GETICON, ICON_SMALL2, 0, SMTO_ABORTIFHUNG, 1000, (LPDWORD)&hicon);

	if (!hicon)
		SendMessageTimeout(hwnd, WM_GETICON, ICON_SMALL, 0, SMTO_ABORTIFHUNG, 1000, (LPDWORD)&hicon);

	if (!hicon)
		SendMessageTimeout(hwnd, WM_GETICON, ICON_BIG, 0, SMTO_ABORTIFHUNG, 1000, (LPDWORD)&hicon);

	if (!hicon)
		hicon = (HICON)GetClassLong(hwnd, GCL_HICONSM);

	if (!hicon)
		hicon = (HICON)GetClassLong(hwnd, GCL_HICON);

	if (!hicon)
		SendMessageTimeout(hwnd, WM_QUERYDRAGICON, 0, 0, 0, 1000, (LPDWORD)&hicon);

	if (!hicon)
		hicon = LoadIcon(0, IDI_APPLICATION);

	return hicon;
}

static HBITMAP create_bitmap_from_icon(HICON hicon, HWND hwnd, HBRUSH hbrush_bkgnd)
{
	HDC hdc_wnd = GetDC(hwnd);
	HBITMAP hbmp = CreateCompatibleBitmap(hdc_wnd, 16, 16);
	ReleaseDC(hwnd, hdc_wnd);

	HDC hdc = CreateCompatibleDC(0);
	HBITMAP hbmp_org = SelectBitmap(hdc, hbmp);
	DrawIconEx(hdc, 0, 0, hicon, 16, 16, 0, hbrush_bkgnd, DI_IMAGE);
	SelectBitmap(hdc, hbmp_org);
	DeleteDC(hdc);

	return hbmp;
}


TaskBarEntry::TaskBarEntry()
{
	_id = 0;
	_hbmp = 0;
	_bmp_idx = 0;
	_used = 0;
	_btn_idx = 0;
	_fsState = 0;
}

TaskBarMap::~TaskBarMap()
{
	while(!empty()) {
		iterator it = begin();
		DeleteBitmap(it->second._hbmp);
		erase(it);
	}
}


TaskBar::TaskBar(HWND hwnd)
 :	super(hwnd)
{
	_desktop_bar = NULL;
	_last_foreground_wnd = 0;
}

TaskBar::~TaskBar()
{
	//DeinstallShellHook();
}

LRESULT TaskBar::Init(LPCREATESTRUCT pcs)
{
	if (super::Init(pcs))
		return 1;

	_htoolbar = CreateToolbarEx(_hwnd,
		WS_CHILD|WS_VISIBLE|CCS_NODIVIDER|CCS_TOP|
		TBSTYLE_LIST|TBSTYLE_TOOLTIPS|TBSTYLE_WRAPABLE|TBSTYLE_TRANSPARENT,
		IDW_TASKTOOLBAR, 0, 0, 0, NULL, 0, 0, 0, 16, 16, sizeof(TBBUTTON));

	SendMessage(_htoolbar, TB_SETBUTTONWIDTH, 0, MAKELONG(80,160));
	//SendMessage(_htoolbar, TB_SETEXTENDEDSTYLE, 0, TBSTYLE_EX_MIXEDBUTTONS);
	//SendMessage(_htoolbar, TB_SETDRAWTEXTFLAGS, DT_CENTER|DT_VCENTER, DT_CENTER|DT_VCENTER);
	//SetWindowFont(_htoolbar, GetStockFont(ANSI_VAR_FONT), FALSE);

	_next_id = IDC_FIRST_APP;

	//InstallShellHook(_hwnd, WM_SHELLHOOK_NOTIFY);

	Refresh();

	SetTimer(_hwnd, 0, 200, NULL);

	return 0;
}

LRESULT TaskBar::WndProc(UINT nmsg, WPARAM wparam, LPARAM lparam)
{
	switch(nmsg) {
	  case WM_CLOSE:
		break;

	  case WM_SIZE:
		SendMessage(_htoolbar, WM_SIZE, 0, 0);
		break;

	  case WM_TIMER:
		Refresh();
		return 0;

	  case WM_SHELLHOOK_NOTIFY: {
		int code = lparam;
/*
		switch(code) {
		  case HSHELL_WINDOWCREATED:
		  case HSHELL_WINDOWDESTROYED:
		  case HSHELL_WINDOWACTIVATED:
		  case HSHELL_WINDOWREPLACED:
			Refresh();
			break;
		} */
		Refresh();
		break;}

	  default:
		return super::WndProc(nmsg, wparam, lparam);
	}

	return 0;
}

int TaskBar::Command(int id, int code)
{
	TaskBarMap::iterator found = _map.find_id(id);

	if (found != _map.end()) {
		HWND hwnd = found->first;

		if (hwnd==GetForegroundWindow() || hwnd==_last_foreground_wnd) {
			ShowWindowAsync(hwnd, SW_MINIMIZE);
			_last_foreground_wnd = 0;
		} else {
			 // switch to selected application window
			if (IsIconic(hwnd))
				ShowWindowAsync(hwnd, SW_RESTORE);

			SetForegroundWindow(hwnd);

			_last_foreground_wnd = hwnd;
		}

		Refresh();

		return 0;
	}

	return super::Command(id, code);
}

 // fill task bar with buttons for enumerated top level windows
BOOL CALLBACK TaskBar::EnumWndProc(HWND hwnd, LPARAM lparam)
{
	TaskBar* pThis = (TaskBar*)lparam;

	DWORD style = GetWindowStyle(hwnd);
	DWORD ex_style = GetWindowExStyle(hwnd);

	if ((style&WS_VISIBLE) && !(ex_style&WS_EX_TOOLWINDOW) &&
		!GetParent(hwnd) && !GetWindow(hwnd,GW_OWNER)) {
		TBBUTTON btn = {-2/*I_IMAGENONE*/, 0, TBSTATE_ENABLED/*|TBSTATE_ELLIPSES*/, BTNS_BUTTON, {0, 0}, 0, 0};

		TCHAR title[BUFFER_LEN];

		if (!GetWindowText(hwnd, title, BUFFER_LEN))
			title[0] = '\0';

		TaskBarMap::iterator found = pThis->_map.find(hwnd);
		int last_id = 0;

		if (found != pThis->_map.end()) {
			last_id = found->second._id;

			if (!last_id)
				found->second._id = pThis->_next_id++;
		} else {
			HICON hicon = get_window_icon(hwnd);
			HBITMAP hbmp = create_bitmap_from_icon(hicon, pThis->_htoolbar, GetSysColorBrush(COLOR_BTNFACE));

			TBADDBITMAP ab = {0, (UINT_PTR)hbmp};
			int bmp_idx = SendMessage(pThis->_htoolbar, TB_ADDBITMAP, 1, (LPARAM)&ab);

			TaskBarEntry entry;

			entry._id = pThis->_next_id++;
			entry._hbmp = hbmp;
			entry._bmp_idx = bmp_idx;
			entry._title = title;

			pThis->_map[hwnd] = entry;
			found = pThis->_map.find(hwnd);
		}

		TaskBarEntry& entry = found->second;

		++entry._used;
		btn.idCommand = entry._id;

		if (hwnd == GetForegroundWindow())
			btn.fsState |= TBSTATE_PRESSED|TBSTATE_CHECKED;

		if (!last_id) {
			 // create new toolbar buttons for new windows
			if (title[0])
				btn.iString = (INT_PTR)title;

			btn.iBitmap = entry._bmp_idx;
			entry._btn_idx = SendMessage(pThis->_htoolbar, TB_BUTTONCOUNT, 0, 0);

			SendMessage(pThis->_htoolbar, TB_INSERTBUTTON, entry._btn_idx, (LPARAM)&btn);
		} else {
			 // refresh attributes of existing buttons
			if (btn.fsState != entry._fsState)
				SendMessage(pThis->_htoolbar, TB_SETSTATE, entry._id, MAKELONG(btn.fsState,0));

			if (entry._title != title) {
				TBBUTTONINFO info;

				info.cbSize = sizeof(TBBUTTONINFO);
				info.dwMask = TBIF_TEXT;
				info.pszText = title;

				SendMessage(pThis->_htoolbar, TB_SETBUTTONINFO, entry._id, (LPARAM)&info);

				entry._title = title;
			}
		}

		entry._fsState = btn.fsState;

		 // move minimized windows out of sight
		if (IsIconic(hwnd)) {
			RECT rect;

			GetWindowRect(hwnd, &rect);

			if (rect.bottom > 0)
				SetWindowPos(hwnd, 0, -32000, -32000, 0, 0, SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE);
		}
	}

	return TRUE;
}

void TaskBar::Refresh()
{
	for(TaskBarMap::iterator it=_map.begin(); it!=_map.end(); ++it)
		it->second._used = 0;

	EnumWindows(EnumWndProc, (LPARAM)this);
	//EnumDesktopWindows(GetThreadDesktop(GetCurrentThreadId()), EnumWndProc, (LPARAM)_htoolbar);

	set<int> btn_idx_to_delete;

	for(TaskBarMap::iterator it=_map.begin(); it!=_map.end(); ++it) {
		TaskBarEntry& entry = it->second;

		if (!entry._used && entry._id) {
			 // store button indexes to remove
			btn_idx_to_delete.insert(entry._btn_idx);
			entry._id = 0;
		}
	}

	 // remove buttons from right to left
	for(set<int>::reverse_iterator it=btn_idx_to_delete.rbegin(); it!=btn_idx_to_delete.rend(); ++it) {
		int idx = *it;

		SendMessage(_htoolbar, TB_DELETEBUTTON, idx, 0);

		for(TaskBarMap::iterator it=_map.begin(); it!=_map.end(); ++it) {
			TaskBarEntry& entry = it->second;

			 // adjust button indexes
			if (entry._btn_idx > idx)
				--entry._btn_idx;
		}
	}
}

TaskBarMap::iterator TaskBarMap::find_id(int id)
{
	for(iterator it=begin(); it!=end(); ++it)
		if (it->second._id == id)
			return it;

	return end();
}


BtnWindowClass StartMenu::s_wcStartMenu(CLASSNAME_STARTMENU);

StartMenu::StartMenu(HWND hwnd)
 :	super(hwnd)
{
	_next_id = IDC_FIRST_MENU;
}

StartMenu::StartMenu(HWND hwnd, const StartMenuFolders& info)
 :	super(hwnd)
{
	for(StartMenuFolders::const_iterator it=info.begin(); it!=info.end(); ++it)
		_dirs.push_back(ShellDirectory(Desktop(), *it, _hwnd));
}


/*
HWND StartMenu::Create(int x, int y, HWND hwndParent)
{
	return Window::Create(WINDOW_CREATOR(StartMenu), NULL, s_wcStartMenu, TITLE_STARTMENU,
							WS_POPUP|WS_THICKFRAME|WS_CLIPCHILDREN|WS_VISIBLE, x, y, STARTMENU_WIDTH, 4, hwndParent);
}
*/

HWND StartMenu::Create(int x, int y, const StartMenuFolders& folders, HWND hwndParent)
{
	return Window::Create(WINDOW_CREATOR_INFO(StartMenu,StartMenuFolders), &folders, 0, s_wcStartMenu, NULL,
							WS_POPUP|WS_THICKFRAME|WS_CLIPCHILDREN|WS_VISIBLE, x, y, STARTMENU_WIDTH, 4, hwndParent);
}


LRESULT	StartMenu::Init(LPCREATESTRUCT pcs)
{
	if (super::Init(pcs))
		return 1;

	WaitCursor wait;
	
	for(StartMenuShellDirs::iterator it=_dirs.begin(); it!=_dirs.end(); ++it) {
		ShellDirectory& dir = *it;

		dir.smart_scan();

		AddShellEntries(dir);
	}

	return 0;
}

void StartMenu::AddShellEntries(const ShellDirectory& dir, bool subfolders)
{
	for(const Entry*entry=dir._down; entry; entry=entry->_next) {
		if (entry->_data.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN)
			continue;

		HICON hIcon = entry->_hicon;

		if (entry->_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			if (!subfolders)
				continue;

			hIcon = SmallIcon(IDI_EXPLORER);
		}

		AddButton(dir._folder.get_name(&*static_cast<const ShellEntry*>(entry)->_pidl).c_str(), hIcon);
	}
}

LRESULT StartMenu::WndProc(UINT nmsg, WPARAM wparam, LPARAM lparam)
{
/*@@
	switch(nmsg) {
	  default:
		return super::WndProc(nmsg, wparam, lparam);
	}

	return 0;
*/	return super::WndProc(nmsg, wparam, lparam);
}

int StartMenu::Command(int id, int code)
{
	switch(id) {
	  case IDCANCEL:
		DestroyWindow(_hwnd);
		break;

	  default:
		return super::Command(id, code);
	}

	return 0;
}

void StartMenu::AddButton(LPCTSTR text, HICON hIcon, UINT id)
{
	if (id == (UINT)-1)
		id = ++_next_id;

	WindowRect rect(_hwnd);

	rect.top -= STARTMENU_LINE_HEIGHT;

	if (rect.top < 0) {
		rect.top += STARTMENU_LINE_HEIGHT;
		rect.bottom += STARTMENU_LINE_HEIGHT;
	}

	MoveWindow(_hwnd, rect.left, rect.top, rect.right-rect.left, rect.bottom-rect.top, TRUE);

	StartMenuButton(_hwnd, rect.bottom-rect.top-STARTMENU_LINE_HEIGHT-4, text, id, hIcon);
}


StartMenuRoot::StartMenuRoot(HWND hwnd)
 :	super(hwnd)
{
}

HWND StartMenuRoot::Create(int x, int y, HWND hwndParent)
{
	return Window::Create(WINDOW_CREATOR(StartMenuRoot), 0, s_wcStartMenu, TITLE_STARTMENU,
							WS_POPUP|WS_THICKFRAME|WS_CLIPCHILDREN|WS_VISIBLE, x, y, STARTMENU_WIDTH, 4, hwndParent);
}

LRESULT	StartMenuRoot::Init(LPCREATESTRUCT pcs)
{
	if (super::Init(pcs))
		return 1;

	WaitCursor wait;

	 // insert start menu entries links from "All Users\Start Menu" and "<user name>\Start Menu"
	ShellDirectory cmn_startmenu(Desktop(), SpecialFolder(CSIDL_COMMON_STARTMENU, _hwnd), _hwnd);
	cmn_startmenu.read_directory();
	AddShellEntries(cmn_startmenu, false);

	ShellDirectory usr_startmenu(Desktop(), SpecialFolder(CSIDL_STARTMENU, _hwnd), _hwnd);
	usr_startmenu.read_directory();
	AddShellEntries(usr_startmenu, false);

	 // insert hard coded start entries
	//AddButton(ResString(IDS_PROGRAMS),0, IDC_PROGRAMS);
	AddButton(ResString(IDS_EXPLORE),	SmallIcon(IDI_EXPLORER), IDC_EXPLORE);
	AddButton(ResString(IDS_FAVORITES),	0, IDC_FAVORITES);
	AddButton(ResString(IDS_DOCUMENTS),	0, IDC_DOCUMENTS);
	AddButton(ResString(IDS_SETTINGS),	0, IDC_SETTINGS);
	AddButton(ResString(IDS_SEARCH),	0, IDC_SEARCH);
	AddButton(ResString(IDS_START_HELP),0, IDC_START_HELP);
	AddButton(ResString(IDS_LAUNCH),	0, IDC_LAUNCH);
	AddButton(ResString(IDS_SHUTDOWN),	SmallIcon(IDI_LOGOFF), IDC_SHUTDOWN);
	AddButton(ResString(IDS_LOGOFF),	SmallIcon(IDI_LOGOFF), IDC_LOGOFF);


	 //TEST: open programs menu folder
	StartMenuFolders prg_folders;

	prg_folders.push_back(SpecialFolder(CSIDL_COMMON_PROGRAMS, _hwnd));
	prg_folders.push_back(SpecialFolder(CSIDL_PROGRAMS, _hwnd));

	WindowRect my_pos(_hwnd);
	StartMenu::Create(my_pos.right, my_pos.top+STARTMENU_HEIGHT-4, prg_folders, _hwnd);


	return 0;
}

int StartMenuRoot::Command(int id, int code)
{
	switch(id) {
	  case IDC_LOGOFF:
		DestroyWindow(GetParent(_hwnd));	//TODO: show dialog and ask for acknowledge
		break;

	  case IDC_SHUTDOWN:
		DestroyWindow(GetParent(_hwnd));	//TODO: show dialog box and shut down system
		break;

	  default:
		return super::Command(id, code);
	}

	return 0;
}
