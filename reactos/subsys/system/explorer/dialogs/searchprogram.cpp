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
 // searchprogram.cpp
 //
 // Explorer dialogs
 //
 // Martin Fuchs, 02.10.2003
 //


#include "../utility/utility.h"

#include "../explorer.h"
#include "../globals.h"
#include "../explorer_intres.h"

#include "searchprogram.h"


int CollectProgramsThread::Run()
{
	try {
		collect_programs(SpecialFolderPath(CSIDL_COMMON_PROGRAMS, _hwnd));
	} catch(COMException&) {
	}

	try {
		collect_programs(SpecialFolderPath(CSIDL_PROGRAMS, _hwnd));
	} catch(COMException&) {
	}

	return 0;
}

void CollectProgramsThread::collect_programs(const ShellPath& path)
{
	ShellDirectory dir(Desktop(), path, 0);

	dir.smart_scan();

	for(const Entry*entry=dir._down; entry; entry=entry->_next) {
		if (!_alive)
			break;

		if (entry->_shell_attribs & SFGAO_HIDDEN)
			continue;

		const ShellEntry* shell_entry = static_cast<const ShellEntry*>(entry);

		if (entry->_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			collect_programs(shell_entry->create_absolute_pidl());
		else if (entry->_shell_attribs & SFGAO_LINK)
			if (_alive)
				_callback(dir._folder, shell_entry, _hwnd);
	}
}


FindProgramTopicDlg::FindProgramTopicDlg(HWND hwnd)
 :	super(hwnd),
	_list_ctrl(GetDlgItem(hwnd, IDC_MAILS_FOUND)),
	_thread(collect_programs_callback, _list_ctrl)
{
	SetWindowIcon(hwnd, IDI_REACTOS/*IDI_SEARCH*/);

	_resize_mgr.Add(IDC_TOPIC,		RESIZE_X);
	_resize_mgr.Add(IDC_MAILS_FOUND,RESIZE);

	_resize_mgr.Resize(+520, +300);

	_haccel = LoadAccelerators(g_Globals._hInstance, MAKEINTRESOURCE(IDA_SEARCH_PROGRAM));

	LV_COLUMN column = {LVCF_TEXT|LVCF_FMT|LVCF_WIDTH, LVCFMT_LEFT, 250};

	column.pszText = _T("Name");
	ListView_InsertColumn(_list_ctrl, 0, &column);

	column.cx = 400;
	column.pszText = _T("Path");
	ListView_InsertColumn(_list_ctrl, 1, &column);

	ListView_SetExtendedListViewStyleEx(_list_ctrl, LVS_EX_FULLROWSELECT, LVS_EX_FULLROWSELECT);

	Refresh();
}

void FindProgramTopicDlg::Refresh()
{
	WaitCursor wait;

	ListView_DeleteAllItems(_list_ctrl);

	_thread.Start();
}

void FindProgramTopicDlg::collect_programs_callback(ShellFolder& folder, const ShellEntry* entry, void* param)
{
	LPCITEMIDLIST pidl = entry->_pidl;

	IShellLink* pShellLink;
	HRESULT hr = folder->GetUIObjectOf(NULL, 1, &pidl, IID_IShellLink, NULL, (LPVOID*)&pShellLink);
	if (SUCCEEDED(hr)) {
		WIN32_FIND_DATA wfd;

		/*hr = pShellLink->Resolve(_hwnd, SLR_NO_UI);
		if (SUCCEEDED(NOERROR))*/ {
			TCHAR path[MAX_PATH];

			hr = pShellLink->GetPath(path, MAX_PATH-1, (WIN32_FIND_DATA*)&wfd, SLGP_UNCPRIORITY);

			if (SUCCEEDED(hr)) {
				if (_tcsstr(path, _T(".exe"))) {	//@@
					HWND list_ctrl = (HWND)param;

					LV_ITEM item = {LVIF_TEXT, INT_MAX};

					item.pszText = entry->_display_name;
					item.iItem = ListView_InsertItem(list_ctrl, &item);

					item.iSubItem = 1;
					item.pszText = path;
					ListView_SetItem(list_ctrl, &item);

					int icon;
					hr = pShellLink->GetIconLocation(path, MAX_PATH-1, &icon);

					//TODO: display icon

					//TODO: store info in ShellPathWithFolder

				}
			}
		}
	}

	pShellLink->Release();
}

LRESULT FindProgramTopicDlg::WndProc(UINT message, WPARAM wparam, LPARAM lparam)
{
	switch(message) {
	  default:
		return super::WndProc(message, wparam, lparam);
	}

	return FALSE;
}

int FindProgramTopicDlg::Command(int id, int code)
{
	switch(id) {
	  case ID_REFRESH:
		Refresh();
		break;

	  default:
		return super::Command(id, code);
	}

	return TRUE;
}
