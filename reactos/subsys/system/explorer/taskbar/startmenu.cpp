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
 // startmenu.cpp
 //
 // Explorer start menu
 //
 // Martin Fuchs, 19.08.2003
 //


#include "../utility/utility.h"

#include "../explorer.h"
#include "../globals.h"
#include "../externals.h"
#include "../explorer_intres.h"

#include "desktopbar.h"
#include "startmenu.h"


BtnWindowClass StartMenu::s_wcStartMenu(CLASSNAME_STARTMENU);

StartMenu::StartMenu(HWND hwnd)
 :	super(hwnd)
{
	_next_id = IDC_FIRST_MENU;
	_submenu_id = 0;
	_border_left = 0;
}

StartMenu::StartMenu(HWND hwnd, const StartMenuFolders& info)
 :	super(hwnd)
{
	for(StartMenuFolders::const_iterator it=info.begin(); it!=info.end(); ++it)
		if (*it)
			_dirs.push_back(ShellDirectory(Desktop(), *it, _hwnd));

	_next_id = IDC_FIRST_MENU;
	_submenu_id = 0;
	_border_left = 0;
}

StartMenu::~StartMenu()
{
	SendParent(PM_STARTMENU_CLOSED);
}


/*
HWND StartMenu::Create(int x, int y, HWND hwndParent)
{
	return Window::Create(WINDOW_CREATOR(StartMenu), NULL, s_wcStartMenu, TITLE_STARTMENU,
							WS_POPUP|WS_THICKFRAME|WS_CLIPCHILDREN|WS_VISIBLE, x, y, STARTMENU_WIDTH_MIN, 4, hwndParent);
}
*/

Window::CREATORFUNC StartMenu::s_def_creator = STARTMENU_CREATOR(StartMenu);

HWND StartMenu::Create(int x, int y, const StartMenuFolders& folders, HWND hwndParent, CREATORFUNC creator)
{
	//TODO: check, if coordinates x/y are visible on the screen
	return Window::Create(creator, &folders, 0, s_wcStartMenu, NULL,
							WS_POPUP|WS_THICKFRAME|WS_CLIPCHILDREN|WS_VISIBLE, x, y, STARTMENU_WIDTH_MIN, 4/*start height*/, hwndParent);
}


LRESULT	StartMenu::Init(LPCREATESTRUCT pcs)
{
	WaitCursor wait;

	AddEntries();

	if (super::Init(pcs))
		return 1;

	 // create buttons for registered entries in _entries
	if (_entries.empty()) {
		AddButton(ResString(IDS_EMPTY), 0, false, (UINT)-1, WS_VISIBLE|WS_CHILD|BS_OWNERDRAW|WS_DISABLED);
	} else {
		for(ShellEntryMap::const_iterator it=_entries.begin(); it!=_entries.end(); ++it) {
			const StartMenuEntry& sme = it->second;
			bool hasSubmenu = false;

			if (sme._entry && (sme._entry->_data.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY))
				hasSubmenu = true;

			AddButton(sme._title, sme._hIcon, hasSubmenu, it->first);
		}
	}

	return 0;
}

void StartMenu::AddEntries()
{
	for(StartMenuShellDirs::iterator it=_dirs.begin(); it!=_dirs.end(); ++it) {
		StartMenuDirectory& smd = *it;
		ShellDirectory& dir = smd._dir;

		dir.smart_scan();

		AddShellEntries(dir, -1, smd._subfolders);
	}
}


void StartMenu::AddShellEntries(const ShellDirectory& dir, int max, bool subfolders)
{
	int cnt = 0;

	for(const Entry*entry=dir._down; entry; entry=entry->_next) {
		 // hide files like "desktop.ini"
		if (entry->_shell_attribs & SFGAO_HIDDEN)
		//not appropriate for drive roots: if (entry->_data.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN)
			continue;

		 // hide subfolders if requested
		if (!subfolders)
			if (entry->_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				continue;

		 // only 'max' entries shall be added.
		if (++cnt == max)
			break;

		const ShellEntry* shell_entry = static_cast<const ShellEntry*>(entry);

		AddEntry(dir._folder, shell_entry);
	}
}


LRESULT StartMenu::WndProc(UINT nmsg, WPARAM wparam, LPARAM lparam)
{
	switch(nmsg) {
	  case WM_SIZE:
		ResizeButtons(LOWORD(lparam)-_border_left);
		break;

	  case WM_NCHITTEST: {
		LRESULT res = super::WndProc(nmsg, wparam, lparam);

		if (res>=HTSIZEFIRST && res<=HTSIZELAST)
			return HTCLIENT;	// disable window resizing

		return res;}

	  case WM_SYSCOMMAND:
		if ((wparam&0xFFF0) == SC_SIZE)
			return 0;			// disable window resizing
		goto def;

	  case WM_ACTIVATEAPP:
		 // close start menu when activating another application
		if (!wparam)
			CloseStartMenu();
		break;	// don't call super::WndProc in case "this" has been deleted

	  case WM_CANCELMODE:
		CloseStartMenu();
		break;

	  case WM_SETFOCUS:
		break;	// don't post WM_CANCELMODE in Window::WndProc when focusing the startmenu

	  case PM_STARTENTRY_FOCUSED: {	//TODO: use TrackMouseEvent() and WM_MOUSEHOVER to wait a bit before opening submenus
		BOOL hasSubmenu = wparam;
		HWND hctrl = (HWND)lparam;

		 // automatically open submenus
		if (hasSubmenu) {
			UpdateWindow(_hwnd);	// draw focused button before waiting on submenu creation
			//SendMessage(_hwnd, WM_COMMAND, MAKEWPARAM(GetDlgCtrlID(hctrl),BN_CLICKED), (LPARAM)hctrl);
			Command(GetDlgCtrlID(hctrl), BN_CLICKED);
		} else {
			 // close any open submenu
			CloseOtherSubmenus(0);
		}
		break;}

	  case PM_STARTENTRY_LAUNCHED:
		 // route message to the parent menu and close menus after launching an entry
		if (!SendParent(nmsg, wparam, lparam))
			DestroyWindow(_hwnd);
		return 1;	// signal that we have received and processed the message

	  case PM_STARTMENU_CLOSED:
		_submenu = 0;
		break;

	  default: def:
		return super::WndProc(nmsg, wparam, lparam);
	}

	return 0;
}


 // resize child button controls to accomodate for new window size
void StartMenu::ResizeButtons(int cx)
{
	HDWP hdwp = BeginDeferWindowPos(10);

	for(HWND ctrl=GetWindow(_hwnd,GW_CHILD); ctrl; ctrl=GetNextWindow(ctrl,GW_HWNDNEXT)) {
		ClientRect rt(ctrl);

		if (rt.right != cx) {
			int height = rt.bottom - rt.top;

			 // special handling for separator controls
			if (!height && (GetWindowStyle(ctrl)&SS_TYPEMASK)==SS_ETCHEDHORZ)
				height = 2;

			hdwp = DeferWindowPos(hdwp, ctrl, 0, 0, 0, cx, height, SWP_NOMOVE|SWP_NOZORDER|SWP_NOACTIVATE);
		}
	}

	EndDeferWindowPos(hdwp);
}


int StartMenu::Command(int id, int code)
{
	switch(id) {
	  case IDCANCEL:
		DestroyWindow(_hwnd);
		break;

	  default: {
		ShellEntryMap::const_iterator found = _entries.find(id);

		if (found != _entries.end()) {
			ShellEntry* entry = const_cast<ShellEntry*>(found->second._entry);

			if (entry)
				ActivateEntry(id, entry);
			break;
		}

		return super::Command(id, code);}
	}

	return 0;
}


StartMenuEntry& StartMenu::AddEntry(LPCTSTR title, HICON hIcon, UINT id)
{
	if (id == (UINT)-1)
		id = ++_next_id;

	StartMenuEntry& sme = _entries[id];

	sme._title = title;
	sme._hIcon = hIcon;

	return sme;
}

StartMenuEntry& StartMenu::AddEntry(const ShellFolder folder, const ShellEntry* entry)
{
	HICON hIcon = entry->_hIcon;

	if (entry->_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		hIcon = SmallIcon(IDI_EXPLORER);

	const String& entry_name = folder.get_name(entry->_pidl);

	StartMenuEntry& sme = AddEntry(entry_name, hIcon);

	sme._entry = entry;

	return sme;
}


void StartMenu::AddButton(LPCTSTR title, HICON hIcon, bool hasSubmenu, UINT id, DWORD style)
{
	WindowRect rect(_hwnd);

	 // increase window height to make room for the new button
	rect.top -= STARTMENU_LINE_HEIGHT;

	if (rect.top < 0) {
		rect.top += STARTMENU_LINE_HEIGHT;
		rect.bottom += STARTMENU_LINE_HEIGHT;
	}

	 // widen window, if it is too small
	int text_width = StartMenuButton::GetTextWidth(title,_hwnd) + 16/*icon*/ + 10/*placeholder*/ + 16/*arrow*/;

	ClientRect clnt(_hwnd);
	int cx = clnt.right - _border_left;
	if (text_width > cx)
		rect.right += text_width-cx;

	MoveWindow(_hwnd, rect.left, rect.top, rect.right-rect.left, rect.bottom-rect.top, TRUE);

	StartMenuCtrl(_hwnd, _border_left, rect.bottom-rect.top-STARTMENU_LINE_HEIGHT-6, rect.right-rect.left-_border_left,
					title, id, hIcon, hasSubmenu, style);
}

void StartMenu::AddSeparator()
{
	WindowRect rect(_hwnd);

	rect.top -= STARTMENU_SEP_HEIGHT;

	if (rect.top < 0) {
		rect.top += STARTMENU_LINE_HEIGHT;
		rect.bottom += STARTMENU_LINE_HEIGHT;
	}

	MoveWindow(_hwnd, rect.left, rect.top, rect.right-rect.left, rect.bottom-rect.top, TRUE);

	StartMenuSeparator(_hwnd, _border_left, rect.bottom-rect.top-STARTMENU_SEP_HEIGHT-6, rect.right-rect.left-_border_left);
}


bool StartMenu::CloseOtherSubmenus(int id)
{
	if (_submenu) {
		if (IsWindow(_submenu)) {
			if (_submenu_id == id)
				return false;
			else {
				DestroyWindow(_submenu);
				_submenu_id = 0;
				// _submenu should be reset automatically by PM_STARTMENU_CLOSED, but safety first...
			}
		}

		_submenu = 0;
	}

	return true;
}


void StartMenu::CreateSubmenu(int id, const StartMenuFolders& new_folders, CREATORFUNC creator)
{
	 // Only open one submenu at a time.
	if (!CloseOtherSubmenus(id))
		return;

	HWND btn = GetDlgItem(_hwnd, id);
	int x, y;

	if (btn) {
		WindowRect pos(btn);

		x = pos.right-3;	// Submenus should overlap their parent a bit.
		y = pos.top+STARTMENU_LINE_HEIGHT-3;
	} else {
		WindowRect pos(_hwnd);

		x = pos.right-3;
		y = pos.top;
	}

	_submenu_id = id;
	_submenu = StartMenu::Create(x, y, new_folders, _hwnd, creator);
}

void StartMenu::CreateSubmenu(int id, int folder_id, CREATORFUNC creator)
{
	StartMenuFolders new_folders;

	SpecialFolderPath folder(folder_id, _hwnd);

	if (folder)
		new_folders.push_back(folder);

	CreateSubmenu(id, new_folders, creator);
}

void StartMenu::CreateSubmenu(int id, int folder_id1, int folder_id2, CREATORFUNC creator)
{
	StartMenuFolders new_folders;

	SpecialFolderPath folder1(folder_id1, _hwnd);

	if (folder1)
		new_folders.push_back(folder1);

	SpecialFolderPath folder2(folder_id2, _hwnd);

	if (folder2)
		new_folders.push_back(folder2);

	CreateSubmenu(id, new_folders, creator);
}


void StartMenu::ActivateEntry(int id, ShellEntry* entry)
{
	if (entry->_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
		 // Only open one submenu at a time.
		if (!CloseOtherSubmenus(id))
			return;

		StartMenuFolders new_folders;

		new_folders.push_back(entry->create_absolute_pidl(_hwnd));

		//TODO: merge all entries of subdirectories with the same name, like "All Users\...\Accessories" and "<user>\...\Accessories"

		CreateSubmenu(id, new_folders);
	} else {
		entry->launch_entry(_hwnd);	//TODO: launch in the background; specify correct HWND for error message box titles

		 // close start menus after launching the selected entry
		CloseStartMenu(id);
	}
}


 /// close all windows of the start menu popup
void StartMenu::CloseStartMenu(int id)
{
	if (!SendParent(PM_STARTENTRY_LAUNCHED, id, (LPARAM)_hwnd))
		DestroyWindow(_hwnd);
}


int StartMenuButton::GetTextWidth(LPCTSTR title, HWND hwnd)
{
	WindowCanvas canvas(hwnd);
	FontSelection font(canvas, GetStockFont(DEFAULT_GUI_FONT));

	RECT rect = {0, 0, 0, 0};
	DrawText(canvas, title, -1, &rect, DT_SINGLELINE|DT_NOPREFIX|DT_CALCRECT);

	return rect.right-rect.left;
}


LRESULT StartMenuButton::WndProc(UINT nmsg, WPARAM wparam, LPARAM lparam)
{
	switch(nmsg) {
	  case WM_MOUSEMOVE:
		 // automatically set the focus to startmenu entries when moving the mouse over them		
		if (GetFocus()!=_hwnd && !(GetWindowStyle(_hwnd)&WS_DISABLED))
			SetFocus(_hwnd);
		break;

	  case WM_SETFOCUS:
		PostParent(PM_STARTENTRY_FOCUSED, _hasSubmenu, (LPARAM)_hwnd);
		return CallWindowProc(_orgWndProc, _hwnd, nmsg, wparam, lparam);	// don't post WM_CANCELMODE in Window::WndProc when focusing the startmenu

	  case WM_CANCELMODE:
		 // route WM_CANCELMODE to the startmenu window
		return SendParent(nmsg, wparam, lparam);

	  default:
		return super::WndProc(nmsg, wparam, lparam);
	}

	return 0;
}

void StartMenuButton::DrawItem(LPDRAWITEMSTRUCT dis)
{
	UINT style = DFCS_BUTTONPUSH;

	if (dis->itemState & ODS_DISABLED)
		style |= DFCS_INACTIVE;

	POINT iconPos = {dis->rcItem.left+2, (dis->rcItem.top+dis->rcItem.bottom-16)/2};
	RECT textRect = {dis->rcItem.left+16+4, dis->rcItem.top+2, dis->rcItem.right-4, dis->rcItem.bottom-4};

	if (dis->itemState & ODS_SELECTED) {
		style |= DFCS_PUSHED;
		++iconPos.x;		++iconPos.y;
		++textRect.left;	++textRect.top;
		++textRect.right;	++textRect.bottom;
	}

	int bk_color = COLOR_BTNFACE;
	int text_color = COLOR_BTNTEXT;

	if (dis->itemState & ODS_FOCUS) {
		bk_color = COLOR_HIGHLIGHT;
		text_color = COLOR_HIGHLIGHTTEXT;
	}

	HBRUSH bk_brush = GetSysColorBrush(bk_color);

	FillRect(dis->hDC, &dis->rcItem, bk_brush);
	DrawIconEx(dis->hDC, iconPos.x, iconPos.y, _hIcon, 16, 16, 0, bk_brush, DI_NORMAL);

	 // draw submenu arrow at the right
	if (_hasSubmenu) {
		static SmallIcon arrowIcon(IDI_ARROW);
		static SmallIcon selArrowIcon(IDI_ARROW_SELECTED);

		DrawIconEx(dis->hDC, dis->rcItem.right-16, iconPos.y,
					dis->itemState&ODS_FOCUS?selArrowIcon:arrowIcon, 16, 16, 0, bk_brush, DI_NORMAL);
	}

	TCHAR title[BUFFER_LEN];
	GetWindowText(_hwnd, title, BUFFER_LEN);

	if (dis->itemState & (ODS_DISABLED|ODS_GRAYED))
		DrawGrayText(dis, &textRect, title, DT_SINGLELINE|DT_NOPREFIX|DT_VCENTER);
	else {
		BkMode mode(dis->hDC, TRANSPARENT);
		TextColor lcColor(dis->hDC, GetSysColor(text_color));
		DrawText(dis->hDC, title, -1, &textRect, DT_SINGLELINE|DT_NOPREFIX|DT_VCENTER);
	}
}


StartMenuRoot::StartMenuRoot(HWND hwnd)
 :	super(hwnd)
{
	 // insert directory "All Users\Start Menu"
	ShellDirectory cmn_startmenu(Desktop(), SpecialFolderPath(CSIDL_COMMON_STARTMENU, _hwnd), _hwnd);
	_dirs.push_back(StartMenuDirectory(cmn_startmenu, false));	// don't add subfolders

	 // insert directory "<user name>\Start Menu"
	ShellDirectory usr_startmenu(Desktop(), SpecialFolderPath(CSIDL_STARTMENU, _hwnd), _hwnd);
	_dirs.push_back(StartMenuDirectory(usr_startmenu, false));	// don't add subfolders

	 // read size of logo bitmap
	BITMAP bmp_hdr;
	GetObject(ResBitmap(IDB_LOGOV), sizeof(BITMAP), &bmp_hdr);
	_logo_size.cx = bmp_hdr.bmWidth;
	_logo_size.cy = bmp_hdr.bmHeight;

	_border_left = _logo_size.cx;
}


HWND StartMenuRoot::Create(HWND hwndDesktopBar)
{
	WindowRect pos(hwndDesktopBar);

	return Window::Create(WINDOW_CREATOR(StartMenuRoot), 0, s_wcStartMenu, TITLE_STARTMENU,
							WS_POPUP|WS_THICKFRAME|WS_CLIPCHILDREN|WS_VISIBLE, pos.left, pos.top-4, STARTMENU_WIDTH_MIN, 4, hwndDesktopBar);
}


void StartMenuRoot::TrackStartmenu()
{
	//TODO

	MSG msg;

	while(GetMessage(&msg, 0, 0, 0)) {
		try {
			if (pretranslate_msg(&msg))
				continue;

			if (dispatch_dialog_msg(&msg))
				continue;

			TranslateMessage(&msg);

			try {
				DispatchMessage(&msg);
			} catch(COMException& e) {
				HandleException(e, g_Globals._hMainWnd);
			}
		} catch(COMException& e) {
			HandleException(e, g_Globals._hMainWnd);
		}
	}

	//@@return msg.wParam;
}


LRESULT	StartMenuRoot::Init(LPCREATESTRUCT pcs)
{
	 // add buttons for entries in _entries
	if (super::Init(pcs))
		return 1;

	AddButton(ResString(IDS_EXPLORE),	SmallIcon(IDI_EXPLORER), false, IDC_EXPLORE);

	AddSeparator();

	 // insert hard coded start entries
	AddButton(ResString(IDS_PROGRAMS),		0, true, IDC_PROGRAMS);
	AddButton(ResString(IDS_FAVORITES),		0, true, IDC_FAVORITES);
	AddButton(ResString(IDS_DOCUMENTS),		0, true, IDC_DOCUMENTS);
	AddButton(ResString(IDS_RECENT),		0, true, IDC_RECENT);
	AddButton(ResString(IDS_SETTINGS),		0, true, IDC_SETTINGS);
	AddButton(ResString(IDS_SETTINGS_WND),	0, false, IDC_SETTINGS_WND);
	AddButton(ResString(IDS_ADMIN),			0, true, IDC_ADMIN);
	AddButton(ResString(IDS_DRIVES),		0, true, IDC_DRIVES);
	AddButton(ResString(IDS_NETWORK),		0, true, IDC_NETWORK);
	AddButton(ResString(IDS_CONNECTIONS),	0, true, IDC_CONNECTIONS);
	AddButton(ResString(IDS_SEARCH),		0, false, IDC_SEARCH);
	AddButton(ResString(IDS_SEARCH_COMPUTER),0,false, IDC_SEARCH_COMPUTER);
	AddButton(ResString(IDS_START_HELP),	0, false, IDC_START_HELP);
	AddButton(ResString(IDS_LAUNCH),		0, false, IDC_LAUNCH);

	AddSeparator();

	AddButton(ResString(IDS_LOGOFF),	SmallIcon(IDI_LOGOFF), false, IDC_LOGOFF);
	AddButton(ResString(IDS_SHUTDOWN),	SmallIcon(IDI_LOGOFF), false, IDC_SHUTDOWN);

	return 0;
}


LRESULT	StartMenuRoot::WndProc(UINT nmsg, WPARAM wparam, LPARAM lparam)
{
	switch(nmsg) {
	  case WM_PAINT: {
		PaintCanvas canvas(_hwnd);
		MemCanvas mem_dc;
		ResBitmap bmp(IDB_LOGOV);
		BitmapSelection sel(mem_dc, bmp);

		ClientRect clnt(_hwnd);
		int h = min(_logo_size.cy, clnt.bottom);

		RECT rect = {0, 0, _logo_size.cx-1, clnt.bottom-h};
		HBRUSH hbr = CreateSolidBrush(RGB(166,202,240));	// same color as the background color in the logo bitmap
		FillRect(canvas, &rect, hbr);
		PatBlt(canvas, _logo_size.cx-1, 0, 1, clnt.bottom-h, WHITENESS);
		DeleteObject(hbr);

		BitBlt(canvas, 0, clnt.bottom-h, _logo_size.cx, h, mem_dc, 0, 0, SRCCOPY);
		break;}

	  default:
		return super::WndProc(nmsg, wparam, lparam);
	}

	return 0;
}


int StartMenuRoot::Command(int id, int code)
{
	switch(id) {
	  case IDC_PROGRAMS:
		CreateSubmenu(id, CSIDL_COMMON_PROGRAMS, CSIDL_PROGRAMS);
		break;

	  case IDC_EXPLORE:
		explorer_show_frame(_hwnd, SW_SHOWNORMAL);
		CloseStartMenu(id);
		break;

	  case IDC_LAUNCH: {
		HWND hwndDesktopBar = GetWindow(_hwnd, GW_OWNER);
		CloseStartMenu(id);
		ShowLaunchDialog(hwndDesktopBar);
		break;}

	  case IDC_DOCUMENTS:
		CreateSubmenu(id, CSIDL_PERSONAL);
		break;

	  case IDC_RECENT:
		CreateSubmenu(id, CSIDL_RECENT, STARTMENU_CREATOR(RecentStartMenu));
		break;

	  case IDC_SETTINGS:
		CreateSubmenu(id, CSIDL_CONTROLS);
		break;

	  case IDC_SETTINGS_WND:
		CloseStartMenu(id);
		MainFrame::Create(_T("::{20D04FE0-3AEA-1069-A2D8-08002B30309D}\\::{21EC2020-3AEA-1069-A2DD-08002B30309D}"), FALSE);
		break;

	  case IDC_FAVORITES:
		CreateSubmenu(id, CSIDL_FAVORITES);
		break;

	  case IDC_ADMIN:
		CreateSubmenu(id, CSIDL_COMMON_ADMINTOOLS, CSIDL_ADMINTOOLS);
		break;

	  case IDC_NETWORK:
		CreateSubmenu(id, CSIDL_NETWORK);
		break;

	  case IDC_CONNECTIONS:
		CreateSubmenu(id, CSIDL_CONNECTIONS);
		break;

	  case IDC_DRIVES:
		//TODO: exclude removeable drives
		CreateSubmenu(id, CSIDL_DRIVES);
		break;

	  case IDC_SEARCH:
		ShowSearchDialog();
		break;

	  case IDC_SEARCH_COMPUTER:
		ShowSearchComputer();
		break;

	  case IDC_LOGOFF:
		/* The shell32 Dialog prompts about some system setting change. This is not what we want to display here.
		HWND hwndDesktopBar = GetWindow(_hwnd, GW_OWNER);
		CloseStartMenu(id);
		ShowRestartDialog(hwndDesktopBar, EWX_LOGOFF);*/
		DestroyWindow(GetParent(_hwnd));
		break;

	  case IDC_SHUTDOWN: {
		HWND hwndDesktopBar = GetWindow(_hwnd, GW_OWNER);
		CloseStartMenu(id);
		ShowExitWindowsDialog(hwndDesktopBar);
		break;}

	  default:
		return super::Command(id, code);
	}

	return 0;
}


void StartMenuRoot::ShowLaunchDialog(HWND hwndDesktopBar)
{
	 //TODO: All text phrases should be put into the resources.
	static LPCSTR szTitle = "Create New Task";
	static LPCSTR szText = "Type the name of a program, folder, document, or Internet resource, and Task Manager will open it for you.";

	HMODULE hShell32 = GetModuleHandle(_T("SHELL32"));
	RUNFILEDLG RunFileDlg = (RUNFILEDLG)GetProcAddress(hShell32, (LPCSTR)61);

	 // Show "Run..." dialog
	if (RunFileDlg) {
#define	W_VER_NT 0
		if ((HIWORD(GetVersion())>>14) == W_VER_NT) {
			WCHAR wTitle[40], wText[256];

			MultiByteToWideChar(CP_ACP, 0, szTitle, -1, wTitle, 40);
			MultiByteToWideChar(CP_ACP, 0, szText, -1, wText, 256);

			RunFileDlg(hwndDesktopBar, 0, NULL, (LPCSTR)wTitle, (LPCSTR)wText, RFF_CALCDIRECTORY);
		}
		else
			RunFileDlg(hwndDesktopBar, 0, NULL, szTitle, szText, RFF_CALCDIRECTORY);
	}
}

void StartMenuRoot::ShowExitWindowsDialog(HWND hwndOwner)
{
	HMODULE hShell32 = GetModuleHandle(_T("SHELL32"));
	EXITWINDOWSDLG ExitWindowsDlg = (EXITWINDOWSDLG)GetProcAddress(hShell32, (LPCSTR)60);

	if (ExitWindowsDlg)
		ExitWindowsDlg(hwndOwner);
}

void StartMenuRoot::ShowRestartDialog(HWND hwndOwner, UINT flags)
{
	HMODULE hShell32 = GetModuleHandle(_T("SHELL32"));
	RESTARTWINDOWSDLG RestartDlg = (RESTARTWINDOWSDLG)GetProcAddress(hShell32, (LPCSTR)59);

	if (RestartDlg)
		RestartDlg(hwndOwner, L"You selected <Log Off>.\n\n", flags);	//TODO: ANSI string conversion if needed
}

void StartMenuRoot::ShowSearchDialog()
{
	HMODULE hShell32 = GetModuleHandle(_T("SHELL32"));
	SHFINDFILES SHFindFiles = (SHFINDFILES)GetProcAddress(hShell32, (LPCSTR)90);

	if (SHFindFiles)
		SHFindFiles(NULL, NULL);
}

void StartMenuRoot::ShowSearchComputer()
{
	HMODULE hShell32 = GetModuleHandle(_T("SHELL32"));
	SHFINDCOMPUTER SHFindComputer = (SHFINDCOMPUTER)GetProcAddress(hShell32, (LPCSTR)91);

	if (SHFindComputer)
		SHFindComputer(NULL, NULL);
}


RecentStartMenu::RecentStartMenu(HWND hwnd, const StartMenuFolders& info)
 :	super(hwnd, info)
{
}

void RecentStartMenu::AddEntries()
{
	for(StartMenuShellDirs::iterator it=_dirs.begin(); it!=_dirs.end(); ++it) {
		StartMenuDirectory& smd = *it;
		ShellDirectory& dir = smd._dir;

		dir.smart_scan();

		dir.sort_directory(SORT_DATE);
		AddShellEntries(dir, 16, smd._subfolders);	//TODO: read max. count of entries from registry
	}
}
