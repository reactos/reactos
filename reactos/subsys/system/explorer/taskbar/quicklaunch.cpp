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
 // quicklaunch.cpp
 //
 // Martin Fuchs, 22.08.2003
 //


#include "../utility/utility.h"

#include "../explorer.h"

#include "quicklaunch.h"


QuickLaunchEntry::QuickLaunchEntry()
{
	_hbmp = 0;
}

QuickLaunchMap::~QuickLaunchMap()
{
	while(!empty()) {
		iterator it = begin();
		DeleteBitmap(it->second._hbmp);
		erase(it);
	}
}


QuickLaunchBar::QuickLaunchBar(HWND hwnd)
 :	super(hwnd)
{
	_next_id = IDC_FIRST_QUICK_ID;

	 // delay refresh to some tome later
	PostMessage(hwnd, PM_REFRESH, 0, 0);
}

QuickLaunchBar::~QuickLaunchBar()
{
	delete _dir;
}

HWND QuickLaunchBar::Create(HWND hwndParent)
{
	ClientRect clnt(hwndParent);

	HWND hwnd = CreateToolbarEx(hwndParent,
								WS_CHILD|WS_VISIBLE|CCS_NODIVIDER|CCS_NORESIZE|
								TBSTYLE_FLAT|TBSTYLE_TOOLTIPS|TBSTYLE_WRAPABLE,
								IDW_QUICKLAUNCHBAR, 0, 0, 0, NULL, 0, 0, 0, 16, 16, sizeof(TBBUTTON));

	if (hwnd)
		new QuickLaunchBar(hwnd);

	return hwnd;
}

void QuickLaunchBar::AddShortcuts()
{
	WaitCursor wait;

	try {
		TCHAR path[_MAX_PATH];

		SpecialFolderFSPath app_data(CSIDL_APPDATA, _hwnd);

		_stprintf(path, _T("%s\\")QUICKLAUNCH_FOLDER, (LPCTSTR)app_data);

		_dir = new ShellDirectory(Desktop(), path, _hwnd);
	} catch(COMException&) {
		return;
	}

	_dir->smart_scan();

	ShellFolder desktop_folder;
	HDC hdc_wnd = GetDC(_hwnd);

	TBBUTTON btn = {-2/*I_IMAGENONE*/, 0, TBSTATE_ENABLED, BTNS_BUTTON, {0, 0}, 0, 0};

	for(Entry*entry=_dir->_down; entry; entry=entry->_next) {
		 // hide files like "desktop.ini"
		if (entry->_data.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN)
			continue;

			 // hide subfolders
			if (!(entry->_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
				ShellEntry* shell_entry = static_cast<ShellEntry*>(entry);

				const String& entry_name = desktop_folder.get_name(shell_entry->_pidl);
				HBITMAP hbmp = create_bitmap_from_icon(shell_entry->_hIcon, GetSysColorBrush(COLOR_BTNFACE), hdc_wnd);

				TBADDBITMAP ab = {0, (UINT_PTR)hbmp};
				int bmp_idx = SendMessage(_hwnd, TB_ADDBITMAP, 1, (LPARAM)&ab);

				QuickLaunchEntry qle;

				int id = ++_next_id;

				qle._hbmp = hbmp;
				qle._title = entry_name;
				qle._entry = shell_entry;

				_entries[id] = qle;

				btn.idCommand = id;
				btn.iBitmap = bmp_idx;
				int idx = SendMessage(_hwnd, TB_BUTTONCOUNT, 0, 0);

				SendMessage(_hwnd, TB_INSERTBUTTON, idx, (LPARAM)&btn);
			}
	}

	ReleaseDC(_hwnd, hdc_wnd);
}

LRESULT QuickLaunchBar::WndProc(UINT nmsg, WPARAM wparam, LPARAM lparam)
{
	switch(nmsg) {
	  case PM_REFRESH:
		AddShortcuts();
		break;

	  default:
		return super::WndProc(nmsg, wparam, lparam);
	}

	return 0;
}

int QuickLaunchBar::Command(int id, int code)
{
	_entries[id]._entry->launch_entry(_hwnd);

	return 0;
}
