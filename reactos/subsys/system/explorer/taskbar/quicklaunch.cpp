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
#include "../globals.h"

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
	CONTEXT("QuickLaunchBar::QuickLaunchBar()");

	_dir = NULL;

	_next_id = IDC_FIRST_QUICK_ID;

	HWND hwndToolTip = (HWND) SendMessage(hwnd, TB_GETTOOLTIPS, 0, 0);

	SetWindowStyle(hwndToolTip, GetWindowStyle(hwndToolTip)|TTS_ALWAYSTIP);

	 // delay refresh to some tome later
	PostMessage(hwnd, PM_REFRESH, 0, 0);
}

QuickLaunchBar::~QuickLaunchBar()
{
	delete _dir;
}

HWND QuickLaunchBar::Create(HWND hwndParent)
{
	CONTEXT("QuickLaunchBar::Create()");

	ClientRect clnt(hwndParent);

	HWND hwnd = CreateToolbarEx(hwndParent,
								WS_CHILD|WS_VISIBLE|CCS_NODIVIDER|CCS_NORESIZE|
								TBSTYLE_TOOLTIPS|TBSTYLE_WRAPABLE|TBSTYLE_FLAT,
								IDW_QUICKLAUNCHBAR, 0, 0, 0, NULL, 0, 0, 0, 16, 16, sizeof(TBBUTTON));

	if (hwnd)
		new QuickLaunchBar(hwnd);

	return hwnd;
}

void QuickLaunchBar::AddShortcuts()
{
	CONTEXT("QuickLaunchBar::AddShortcuts()");

	WaitCursor wait;

	try {
		TCHAR path[MAX_PATH];

		SpecialFolderFSPath app_data(CSIDL_APPDATA, _hwnd);	// perhaps also look into CSIDL_COMMON_APPDATA ?

		_stprintf(path, TEXT("%s\\")QUICKLAUNCH_FOLDER, (LPCTSTR)app_data);

		CreateDirectory(path, NULL);
		_dir = new ShellDirectory(Desktop(), path, _hwnd);

		_dir->smart_scan(SCAN_EXTRACT_ICONS|SCAN_FILESYSTEM);
	} catch(COMException&) {
		return;
	}


	ShellFolder desktop_folder;
	WindowCanvas canvas(_hwnd);

	TBBUTTON btn = {0, 0, TBSTATE_ENABLED, BTNS_BUTTON|BTNS_NOPREFIX, {0, 0}, 0, 0};

	for(Entry*entry=_dir->_down; entry; entry=entry->_next) {
		 // hide files like "desktop.ini"
		if (entry->_data.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN)
			continue;

			 // hide subfolders
			if (!(entry->_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
				HBITMAP hbmp = g_Globals._icon_cache.get_icon_bitmap(entry->_icon_id, GetSysColorBrush(COLOR_BTNFACE), canvas);

				TBADDBITMAP ab = {0, (UINT_PTR)hbmp};
				int bmp_idx = SendMessage(_hwnd, TB_ADDBITMAP, 1, (LPARAM)&ab);

				QuickLaunchEntry qle;

				int id = ++_next_id;

				qle._hbmp = hbmp;
				qle._title = entry->_display_name;	//entry->_etype==ET_SHELL? desktop_folder.get_name(static_cast<ShellEntry*>(entry)->_pidl): entry->_display_name
				qle._entry = entry;

				_entries[id] = qle;

				btn.idCommand = id;
				btn.iBitmap = bmp_idx;
				int idx = SendMessage(_hwnd, TB_BUTTONCOUNT, 0, 0);

				SendMessage(_hwnd, TB_INSERTBUTTON, idx, (LPARAM)&btn);
			}
	}
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
	CONTEXT("QuickLaunchBar::Command()");

	_entries[id]._entry->launch_entry(_hwnd);

	return 0;
}

int QuickLaunchBar::Notify(int id, NMHDR* pnmh)
{
	switch(pnmh->code) {
	  case TTN_GETDISPINFO: {
		NMTTDISPINFO* ttdi = (NMTTDISPINFO*) pnmh;

		int id = ttdi->hdr.idFrom;
		ttdi->lpszText = (LPTSTR)_entries[id]._title.c_str();
#ifdef TTF_DI_SETITEM
		ttdi->uFlags |= TTF_DI_SETITEM;
#endif
		break;}

		return super::Notify(id, pnmh);
	}

	return 0;
}
