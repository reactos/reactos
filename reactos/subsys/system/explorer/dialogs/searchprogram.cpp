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
				_callback(dir._folder, shell_entry, _para);
	}

	dir.free_subentries();
}


#pragma warning(disable: 4355)

FindProgramTopicDlg::FindProgramTopicDlg(HWND hwnd)
 :	super(hwnd),
	_list_ctrl(GetDlgItem(hwnd, IDC_MAILS_FOUND)),
	_himl(ImageList_Create(GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), ILC_COLOR32, 0, 0)),
	_thread(collect_programs_callback, hwnd, this)
{
	SetWindowIcon(hwnd, IDI_REACTOS/*IDI_SEARCH*/);

	_resize_mgr.Add(IDC_TOPIC,		RESIZE_X);
	_resize_mgr.Add(IDC_MAILS_FOUND,RESIZE);

	_resize_mgr.Resize(+520, +300);

	_haccel = LoadAccelerators(g_Globals._hInstance, MAKEINTRESOURCE(IDA_SEARCH_PROGRAM));

	ListView_SetImageList(_list_ctrl, _himl, LVSIL_SMALL);
	_idxNoIcon = ImageList_AddIcon(_himl, SmallIcon(IDI_APPICON));

	LV_COLUMN column = {LVCF_FMT|LVCF_WIDTH|LVCF_TEXT, LVCFMT_LEFT, 250};

	column.pszText = _T("Name");
	ListView_InsertColumn(_list_ctrl, 0, &column);

	column.cx = 400;
	column.pszText = _T("Path");
	ListView_InsertColumn(_list_ctrl, 1, &column);

	ListView_SetExtendedListViewStyleEx(_list_ctrl, LVS_EX_FULLROWSELECT, LVS_EX_FULLROWSELECT);

	Refresh();
}

FindProgramTopicDlg::~FindProgramTopicDlg()
{
	ImageList_Destroy(_himl);
}


void FindProgramTopicDlg::Refresh()
{
	WaitCursor wait;

	_thread.Stop();

	TCHAR buffer[1024];
	GetWindowText(GetDlgItem(_hwnd, IDC_TOPIC), buffer, 1024);
	_filter = buffer;

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
				FindProgramTopicDlg* pThis = (FindProgramTopicDlg*) param;

				String lwr_path = path;
				String lwr_name = entry->_display_name;
				String filter = pThis->_filter;

#ifndef __WINE__ //TODO
				_tcslwr((LPTSTR)lwr_path.c_str());
				_tcslwr((LPTSTR)lwr_name.c_str());
				_tcslwr((LPTSTR)filter.c_str());
#endif

				//if (_tcsstr(lwr_path, _T(".exe")))	//@@ filter on ".exe" suffix
				//if (!_tcsstr(lwr_name, _T("uninstal")) && !_tcsstr(lwr_name, _T("deinstal")))	//@@ filter out deinstallation links
				if (_tcsstr(lwr_path, filter) || _tcsstr(lwr_name, filter)) {
					LV_ITEM item = {LVIF_TEXT|LVIF_IMAGE|LVIF_PARAM, INT_MAX};

					item.pszText = entry->_display_name;

					if (entry->_hIcon != (HICON)-1)
						item.iImage = ImageList_AddIcon(pThis->_himl, entry->_hIcon);
					else
						item.iImage = pThis->_idxNoIcon;

					item.lParam = 0;	//@@

					//TODO: store info in ShellPathWithFolder

					Lock lock(pThis->_thread._crit_sect);

					 // resolve deadlocks while executing Thread::Stop()
					if (!pThis->_thread.is_alive())
						return;

					item.iItem = ListView_InsertItem(pThis->_list_ctrl, &item);

					item.mask = LVIF_TEXT;
					item.iSubItem = 1;
					item.pszText = path;

					if (!pThis->_thread.is_alive())
						return;

					ListView_SetItem(pThis->_list_ctrl, &item);
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
	if (code == BN_CLICKED)
		switch(id) {
		  case ID_REFRESH:
			Refresh();
			break;

		  default:
			return super::Command(id, code);
		}
	else if (code == EN_CHANGE)
		switch(id) {
		  case IDC_TOPIC:
			Refresh();
		}

	return TRUE;
}

int FindProgramTopicDlg::Notify(int id, NMHDR* pnmh)
{
	switch(pnmh->code) {
	  case LVN_GETDISPINFO: {
		LV_DISPINFO* pDispInfo = (LV_DISPINFO*) pnmh;

		if (pnmh->hwndFrom == _list_ctrl) {
/*
			if (pDispInfo->item.mask & LVIF_IMAGE) {
				int icon;
				HRESULT hr = pShellLink->GetIconLocation(path, MAX_PATH-1, &icon);

				HICON hIcon = ExtractIcon();
				pDispInfo->item.iImage = ImageList_AddIcon(_himl, hIcon);

				pDispInfo->item.mask |= LVIF_DI_SETITEM;

				return 1;
			}
*/
		}
	  break;}
	}

	return 0;
}
