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

#include "taskbar.h"
#include "startmenu.h"


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
		if (*it)
			_dirs.push_back(ShellDirectory(Desktop(), *it, _hwnd));

	_next_id = IDC_FIRST_MENU;
}


/*
HWND StartMenu::Create(int x, int y, HWND hwndParent)
{
	return Window::Create(WINDOW_CREATOR(StartMenu), NULL, s_wcStartMenu, TITLE_STARTMENU,
							WS_POPUP|WS_THICKFRAME|WS_CLIPCHILDREN|WS_VISIBLE, x, y, STARTMENU_WIDTH, 4, hwndParent);
}
*/

Window::CREATORFUNC StartMenu::s_def_creator = STARTMENU_CREATOR(StartMenu);

HWND StartMenu::Create(int x, int y, const StartMenuFolders& folders, HWND hwndParent, CREATORFUNC creator)
{
	return Window::Create(creator, &folders, 0, s_wcStartMenu, NULL,
							WS_POPUP|WS_THICKFRAME|WS_CLIPCHILDREN|WS_VISIBLE, x, y, STARTMENU_WIDTH, 4, hwndParent);
}


LRESULT	StartMenu::Init(LPCREATESTRUCT pcs)
{
	WaitCursor wait;

	AddEntries();

	if (super::Init(pcs))
		return 1;

	 // create buttons for registered entries in _entries
	if (_entries.empty()) {
		AddButton(ResString(IDS_EMPTY), 0, false, (UINT)-1, WS_VISIBLE|WS_CHILD|BS_PUSHBUTTON|BS_OWNERDRAW|WS_DISABLED);
	} else {
		for(ShellEntryMap::const_iterator it=_entries.begin(); it!=_entries.end(); ++it) {
			const StartMenuEntry& sme = it->second;
			bool showArrow = false;

			if (sme._entry && (sme._entry->_data.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY))
				showArrow = true;

			AddButton(sme._title, sme._hIcon, showArrow, it->first);
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
		if (entry->_data.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN)
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
	  case WM_NCHITTEST: {
		LRESULT res = super::WndProc(nmsg, wparam, lparam);

		if (res>=HTSIZEFIRST && res<=HTSIZELAST)
			return HTCLIENT;	// disable window resizing

		return res;}

	  case WM_SYSCOMMAND:
		if ((wparam&0xFFF0) == SC_SIZE)
			return 0;			// disable window resizing
		goto def;

	  default: def:
		return super::WndProc(nmsg, wparam, lparam);
	}

	return 0;
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


void StartMenu::AddButton(LPCTSTR title, HICON hIcon, bool showArrow, UINT id, DWORD style)
{
	WindowRect rect(_hwnd);

	rect.top -= STARTMENU_LINE_HEIGHT;

	if (rect.top < 0) {
		rect.top += STARTMENU_LINE_HEIGHT;
		rect.bottom += STARTMENU_LINE_HEIGHT;
	}

	MoveWindow(_hwnd, rect.left, rect.top, rect.right-rect.left, rect.bottom-rect.top, TRUE);

	StartMenuButton(_hwnd, rect.bottom-rect.top-STARTMENU_LINE_HEIGHT-4, title, id, hIcon, showArrow, style);
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

	StartMenuSeparator(_hwnd, rect.bottom-rect.top-STARTMENU_SEP_HEIGHT-4);
}


void StartMenu::CreateSubmenu(int id, const StartMenuFolders& new_folders, CREATORFUNC creator)
{
	HWND btn = GetDlgItem(_hwnd, id);
	int x, y;

	if (btn) {
		WindowRect pos(btn);

		x = pos.right-8;	// Submenus should overlap their parent a bit.
		y = pos.top;
	} else {
		WindowRect pos(_hwnd);

		x = pos.right-8;
		y = pos.top;
	}

	StartMenu::Create(x, y, new_folders, _hwnd, creator);
}

void StartMenu::CreateSubmenu(int id, int folder_id, CREATORFUNC creator)
{
	StartMenuFolders new_folders;

	SpecialFolder folder(folder_id, _hwnd);

	if (folder)
		new_folders.push_back(folder);

	CreateSubmenu(id, new_folders, creator);
}

void StartMenu::CreateSubmenu(int id, int folder_id1, int folder_id2, CREATORFUNC creator)
{
	StartMenuFolders new_folders;

	SpecialFolder folder1(folder_id1, _hwnd);

	if (folder1)
		new_folders.push_back(folder1);

	SpecialFolder folder2(folder_id2, _hwnd);

	if (folder2)
		new_folders.push_back(folder2);

	CreateSubmenu(id, new_folders, creator);
}


void StartMenu::ActivateEntry(int id, ShellEntry* entry)
{
	if (entry->_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
		StartMenuFolders new_folders;

		new_folders.push_back(entry->create_absolute_pidl(_hwnd));

		CreateSubmenu(id, new_folders);
	} else {
		entry->launch_entry(_hwnd);
	}
}


StartMenuRoot::StartMenuRoot(HWND hwnd)
 :	super(hwnd)
{
	 // insert directory "All Users\Start Menu"
	ShellDirectory cmn_startmenu(Desktop(), SpecialFolder(CSIDL_COMMON_STARTMENU, _hwnd), _hwnd);
	_dirs.push_back(StartMenuDirectory(cmn_startmenu, false));	// dont't add subfolders

	 // insert directory "<user name>\Start Menu"
	ShellDirectory usr_startmenu(Desktop(), SpecialFolder(CSIDL_STARTMENU, _hwnd), _hwnd);
	_dirs.push_back(StartMenuDirectory(usr_startmenu, false));	// dont't add subfolders
}

HWND StartMenuRoot::Create(int x, int y, HWND hwndParent)
{
	return Window::Create(WINDOW_CREATOR(StartMenuRoot), 0, s_wcStartMenu, TITLE_STARTMENU,
							WS_POPUP|WS_THICKFRAME|WS_CLIPCHILDREN|WS_VISIBLE, x, y, STARTMENU_WIDTH, 4, hwndParent);
}

LRESULT	StartMenuRoot::Init(LPCREATESTRUCT pcs)
{
	 // add buttons for entries in _entries
	if (super::Init(pcs))
		return 1;

	AddSeparator();

	 // insert hard coded start entries
	AddButton(ResString(IDS_PROGRAMS),	0, true, IDC_PROGRAMS);
	AddButton(ResString(IDS_EXPLORE),	SmallIcon(IDI_EXPLORER), false, IDC_EXPLORE);
	AddButton(ResString(IDS_FAVORITES),	0, true, IDC_FAVORITES);
	AddButton(ResString(IDS_DOCUMENTS),	0, true, IDC_DOCUMENTS);
	AddButton(ResString(IDS_RECENT),	0, true, IDC_RECENT);
	AddButton(ResString(IDS_SETTINGS),	0, true, IDC_SETTINGS);
	AddButton(ResString(IDS_ADMIN),		0, true, IDC_ADMIN);
	AddButton(ResString(IDS_SEARCH),	0, false, IDC_SEARCH);
	AddButton(ResString(IDS_START_HELP),0, false, IDC_START_HELP);
	AddButton(ResString(IDS_LAUNCH),	0, false, IDC_LAUNCH);

	AddSeparator();

	AddButton(ResString(IDS_SHUTDOWN),	SmallIcon(IDI_LOGOFF), false, IDC_SHUTDOWN);
	AddButton(ResString(IDS_LOGOFF),	SmallIcon(IDI_LOGOFF), false, IDC_LOGOFF);

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
		break;

	  case IDC_DOCUMENTS:
		CreateSubmenu(id, CSIDL_PERSONAL);
		break;

	  case IDC_RECENT:
		CreateSubmenu(id, CSIDL_RECENT, STARTMENU_CREATOR(RecentStartMenu));
		break;

	  case IDC_SETTINGS:
		CreateSubmenu(id, CSIDL_CONTROLS);
		break;

	  case IDC_FAVORITES:
		CreateSubmenu(id, CSIDL_FAVORITES);
		break;

	  case IDC_ADMIN:
		CreateSubmenu(id, CSIDL_COMMON_ADMINTOOLS, CSIDL_ADMINTOOLS);
		break;

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
