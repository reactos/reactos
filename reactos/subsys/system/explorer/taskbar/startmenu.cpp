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
		 // hide files like "desktop.ini"
		if (entry->_data.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN)
			continue;

		 // hide subfolders if requested
		if (!subfolders)
			if (entry->_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				continue;

		const ShellEntry* shell_entry = static_cast<const ShellEntry*>(entry);

		AddButton(dir._folder, shell_entry);
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
		ShellEntryMap::const_iterator found = _entry_map.find(id);

		if (found != _entry_map.end()) {
			ActivateEntry(const_cast<ShellEntry*>(found->second));
			break;
		}

		return super::Command(id, code);}
	}

	return 0;
}

UINT StartMenu::AddButton(LPCTSTR text, HICON hIcon, UINT id)
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

	return id;
}

UINT StartMenu::AddButton(const ShellFolder folder, const ShellEntry* entry)
{
	HICON hIcon = entry->_hicon;

	if (entry->_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		hIcon = SmallIcon(IDI_EXPLORER);

	const String& entry_name = folder.get_name(entry->_pidl);

	UINT id = AddButton(entry_name, hIcon);

	_entry_map[id] = entry;

	return id;
}

void StartMenu::ActivateEntry(ShellEntry* entry)
{
	if (entry->_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
		StartMenuFolders new_folders;

		new_folders.push_back(entry->create_absolute_pidl(_hwnd));

		WindowRect my_pos(_hwnd);
		StartMenu::Create(my_pos.right, my_pos.top+STARTMENU_HEIGHT-4, new_folders, _hwnd);
	} else {
		entry->launch_entry(_hwnd);
	}
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
	AddButton(ResString(IDS_PROGRAMS),	0, IDC_PROGRAMS);
	AddButton(ResString(IDS_EXPLORE),	SmallIcon(IDI_EXPLORER), IDC_EXPLORE);
	AddButton(ResString(IDS_FAVORITES),	0, IDC_FAVORITES);
	AddButton(ResString(IDS_DOCUMENTS),	0, IDC_DOCUMENTS);
	AddButton(ResString(IDS_SETTINGS),	0, IDC_SETTINGS);
	AddButton(ResString(IDS_SEARCH),	0, IDC_SEARCH);
	AddButton(ResString(IDS_START_HELP),0, IDC_START_HELP);
	AddButton(ResString(IDS_LAUNCH),	0, IDC_LAUNCH);
	AddButton(ResString(IDS_SHUTDOWN),	SmallIcon(IDI_LOGOFF), IDC_SHUTDOWN);
	AddButton(ResString(IDS_LOGOFF),	SmallIcon(IDI_LOGOFF), IDC_LOGOFF);

	return 0;
}

int StartMenuRoot::Command(int id, int code)
{
	switch(id) {
	  case IDC_PROGRAMS: {
		StartMenuFolders prg_folders;

		prg_folders.push_back(SpecialFolder(CSIDL_COMMON_PROGRAMS, _hwnd));
		prg_folders.push_back(SpecialFolder(CSIDL_PROGRAMS, _hwnd));

		WindowRect my_pos(_hwnd);
		StartMenu::Create(my_pos.right, my_pos.top+STARTMENU_HEIGHT-4, prg_folders, _hwnd);
		break;}

	  case IDC_EXPLORE:
		explorer_show_frame(_hwnd, SW_SHOWNORMAL);
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
