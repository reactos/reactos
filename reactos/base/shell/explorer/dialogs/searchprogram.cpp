/*
 * Copyright 2003, 2004, 2005 Martin Fuchs
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
 // dialogs/searchprogram.cpp
 //
 // Explorer dialogs
 //
 // Martin Fuchs, 02.10.2003
 //


#include <precomp.h>

#include "../resource.h"

#include "searchprogram.h"


int CollectProgramsThread::Run()
{
	try {
		collect_programs(SpecialFolderPath(CSIDL_COMMON_PROGRAMS, _hwnd));
	} catch(COMException&) {
	}

	if (_alive)
		try {
			collect_programs(SpecialFolderPath(CSIDL_PROGRAMS, _hwnd));
		} catch(COMException&) {
		}

	if (_alive)
		_cache_valid = true;

	return 0;
}

void CollectProgramsThread::collect_programs(const ShellPath& path)
{
	ShellDirectory* dir = new ShellDirectory(GetDesktopFolder(), path, 0);
	_dirs.push(dir);

	dir->smart_scan(SORT_NONE);

	for(Entry*entry=dir->_down; _alive && entry; entry=entry->_next) {
		if (entry->_shell_attribs & SFGAO_HIDDEN)
			continue;

		if (entry->_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			collect_programs(entry->create_absolute_pidl());
		else if (entry->_shell_attribs & SFGAO_LINK)
			if (_alive)
				_callback(entry, _para);
	}
}

void CollectProgramsThread::free_dirs()
{
	while(!_dirs.empty()) {
		ShellDirectory* dir = _dirs.top();
		dir->free_subentries();
		_dirs.pop();
	}
}


#ifdef _MSC_VER
#pragma warning(disable: 4355)
#endif

FindProgramDlg::FindProgramDlg(HWND hwnd)
 :	super(hwnd),
	_list_ctrl(GetDlgItem(hwnd, IDC_PROGRAMS_FOUND)),
	_thread(collect_programs_callback, hwnd, this),
	_sort(_list_ctrl, CompareFunc/*, (LPARAM)this*/)
{
	TCHAR szTemp[256];
	const size_t nChars = sizeof(szTemp)/sizeof(*szTemp);
	SetWindowIcon(hwnd, IDI_SEARCH);

	_resize_mgr.Add(IDC_FILTER,			RESIZE_X);
	_resize_mgr.Add(IDC_CHECK_ENTRIES,	MOVE_X);
	_resize_mgr.Add(IDC_PROGRAMS_FOUND,	RESIZE);

	_resize_mgr.Resize(+520, +300);

	_haccel = LoadAccelerators(g_Globals._hInstance, MAKEINTRESOURCE(IDA_SEARCH_PROGRAM));

	(void)ListView_SetImageList(_list_ctrl, g_Globals._icon_cache.get_sys_imagelist(), LVSIL_SMALL);

	LV_COLUMN column = {LVCF_FMT|LVCF_WIDTH|LVCF_TEXT, LVCFMT_LEFT, 250};

	LoadString(g_Globals._hInstance, IDS_NAMECOLUMN, szTemp, nChars);
	column.pszText = szTemp;
	(void)ListView_InsertColumn(_list_ctrl, 0, &column);

	column.cx = 300;
	LoadString(g_Globals._hInstance, IDS_PATHCOLUMN, szTemp, nChars);
	column.pszText = szTemp;
	(void)ListView_InsertColumn(_list_ctrl, 1, &column);

	column.cx = 400;
	LoadString(g_Globals._hInstance, IDS_MENUCOLUMN, szTemp, nChars);
	column.pszText = szTemp;
	(void)ListView_InsertColumn(_list_ctrl, 2, &column);

	(void)ListView_SetExtendedListViewStyleEx(_list_ctrl, LVS_EX_FULLROWSELECT, LVS_EX_FULLROWSELECT);

	_common_programs = SpecialFolderFSPath(CSIDL_COMMON_PROGRAMS, hwnd);
	if (!_common_programs.empty())
		_common_programs.append(_T("\\"));

	_user_programs = SpecialFolderFSPath(CSIDL_PROGRAMS, hwnd);
	if (!_user_programs.empty())
		_user_programs.append(_T("\\"));

	CenterWindow(hwnd);

	Refresh();

	register_pretranslate(hwnd);
}

FindProgramDlg::~FindProgramDlg()
{
	_thread.Stop();

	unregister_pretranslate(_hwnd);
}


void FindProgramDlg::Refresh(bool delete_cache)
{
	WaitCursor wait;

	_thread.Stop();

	TCHAR buffer[1024];
	GetWindowText(GetDlgItem(_hwnd, IDC_FILTER), buffer, COUNTOF(buffer));
	CharLower(buffer);
	_lwr_filter = buffer;

	HiddenWindow hide_listctrl(_list_ctrl);

	(void)ListView_DeleteAllItems(_list_ctrl);

	if (delete_cache || !_thread._cache_valid) {
		_thread.free_dirs();
		_thread.Start();
	} else {
		for(FPDCache::const_iterator it=_cache.begin(); it!=_cache.end(); ++it)
			add_entry(*it);
	}
}

void FindProgramDlg::collect_programs_callback(Entry* entry, void* param)
{
	FindProgramDlg* pThis = (FindProgramDlg*) param;

	IShellLink* pShellLink;
	HRESULT hr = entry->GetUIObjectOf(pThis->_hwnd, IID_IShellLink, (LPVOID*)&pShellLink);

	if (SUCCEEDED(hr)) {
		ShellLinkPtr shell_link(pShellLink);

		shell_link->Release();

		/*hr = pShellLink->Resolve(pThis->_hwnd, SLR_NO_UI);
		if (SUCCEEDED(hr))*/ {
			WIN32_FIND_DATA wfd;
			TCHAR path[MAX_PATH];

			hr = pShellLink->GetPath(path, COUNTOF(path)-1, &wfd, SLGP_UNCPRIORITY);

			if (SUCCEEDED(hr)) {
				TCHAR entry_path[MAX_PATH];

				entry->get_path(entry_path, COUNTOF(entry_path));

				String menu_path;

				int len = pThis->_common_programs.size();

				if (len && !_tcsnicmp(entry_path, pThis->_common_programs, len))
					menu_path = ResString(IDS_ALL_USERS) + (String(entry_path)+len);
				else if ((len=pThis->_user_programs.size()) && !_tcsnicmp(entry_path, pThis->_user_programs, len))
					menu_path = String(entry_path)+len;

				 // store info in cache
				FPDEntry new_entry;

				new_entry._entry = entry;
				new_entry._menu_path = menu_path;
				new_entry._path = path;
				new_entry._idxIcon = I_IMAGECALLBACK;

				pThis->_cache.push_front(new_entry);
				FPDEntry& cache_entry = pThis->_cache.front();

				Lock lock(pThis->_thread._crit_sect);

				 // resolve deadlocks while executing Thread::Stop()
				if (!pThis->_thread.is_alive())
					return;

				pThis->add_entry(cache_entry);
			}
		}
	}
}

void FindProgramDlg::add_entry(const FPDEntry& cache_entry)
{
	String lwr_path = cache_entry._path;
	String lwr_name = cache_entry._entry->_display_name;

	lwr_path.toLower();
	lwr_name.toLower();

	if (_lwr_filter.empty())
		if (_tcsstr(lwr_name, _T("uninstal")) || _tcsstr(lwr_name, _T("deinstal")))	// filter out deinstallation links
			return;

	if (!_tcsstr(lwr_path, _lwr_filter) && !_tcsstr(lwr_name, _lwr_filter))
		return;

	LV_ITEM item = {LVIF_TEXT|LVIF_IMAGE|LVIF_PARAM, INT_MAX};

	item.pszText = cache_entry._entry->_display_name;
	item.iImage = cache_entry._idxIcon;
	item.lParam = (LPARAM) &cache_entry;
	item.iItem = ListView_InsertItem(_list_ctrl, &item);	// We could use the information in _sort to enable manual sorting while populating the list.

	item.mask = LVIF_TEXT;

	item.iSubItem = 1;
	item.pszText = (LPTSTR)(LPCTSTR)cache_entry._path;
	(void)ListView_SetItem(_list_ctrl, &item);

	item.iSubItem = 2;
	item.pszText = (LPTSTR)(LPCTSTR)cache_entry._menu_path;
	(void)ListView_SetItem(_list_ctrl, &item);
}

LRESULT FindProgramDlg::WndProc(UINT nmsg, WPARAM wparam, LPARAM lparam)
{
	switch(nmsg) {
	  case WM_CLOSE:
		(void)ListView_SetImageList(_list_ctrl, 0, LVSIL_SMALL);	// detach system image list
		goto def;

	  case PM_TRANSLATE_MSG: {
		MSG* pmsg = (MSG*) lparam;

		if (TranslateAccelerator(_hwnd, _haccel, pmsg))
			return TRUE;

		return FALSE;}

	  default: def:
		return super::WndProc(nmsg, wparam, lparam);
	}

	return 0;
}

int FindProgramDlg::Command(int id, int code)
{
	if (code == BN_CLICKED) {
		switch(id) {
		  case ID_REFRESH:
			Refresh(true);
			break;

		  case IDOK:
			LaunchSelected();
			break;

		  case IDC_CHECK_ENTRIES:
			CheckEntries();
			break;

		  default:
			return super::Command(id, code);
		}

		return 0;
	}
	else if (code == EN_CHANGE) {
		switch(id) {
		  case IDC_FILTER:
			Refresh();
			break;
		}

		return 0;
	}

	return 1;
}

void FindProgramDlg::LaunchSelected()
{
	Lock lock(_thread._crit_sect);

	int count = ListView_GetSelectedCount(_list_ctrl);

	if (count > 1)
		if (MessageBox(_hwnd, ResString(IDS_LAUNCH_MANY_PROGRAMS), ResString(IDS_TITLE), MB_OKCANCEL) != IDOK)
			return;

	for(int idx=-1; (idx=ListView_GetNextItem(_list_ctrl, idx, LVNI_SELECTED))!=-1; ) {
		LPARAM lparam = ListView_GetItemData(_list_ctrl, idx);

		if (lparam) {
			FPDEntry& cache_entry = *(FPDEntry*)lparam;
			cache_entry._entry->launch_entry(_hwnd);
		}
	}
}

int FindProgramDlg::Notify(int id, NMHDR* pnmh)
{
	switch(pnmh->code) {
	  case LVN_GETDISPINFO: {
		LV_DISPINFO* pDispInfo = (LV_DISPINFO*) pnmh;

		if (pnmh->hwndFrom == _list_ctrl) {
			if (pDispInfo->item.mask & LVIF_IMAGE) {
				FPDEntry& cache_entry = *(FPDEntry*)pDispInfo->item.lParam;
				Entry* entry = cache_entry._entry;

				if (entry->_icon_id == ICID_UNKNOWN)
					entry->_icon_id = entry->extract_icon(ICF_SYSCACHE);

				pDispInfo->item.iImage = g_Globals._icon_cache.get_icon(entry->_icon_id).get_sysiml_idx();
				pDispInfo->item.mask |= LVIF_DI_SETITEM;

				return 1;
			}
		}}
		break;

	  case NM_DBLCLK:
		if (pnmh->hwndFrom == _list_ctrl)
			LaunchSelected();
		/*{
			Lock lock(_thread._crit_sect);

			LPNMLISTVIEW pnmv = (LPNMLISTVIEW) pnmh;
			LPARAM lparam = ListView_GetItemData(pnmh->hwndFrom, pnmv->iItem);

			if (lparam) {
				FPDEntry& cache_entry = *(FPDEntry*)lparam;
				cache_entry._entry->launch_entry(_hwnd);
			}
		}*/
		break;

	  case HDN_ITEMCLICK: {
		WaitCursor wait;
		NMHEADER* phdr = (NMHEADER*)pnmh;

		if (GetParent(pnmh->hwndFrom) == _list_ctrl) {
			if (_thread._cache_valid) {	// disable manual sorting while populating the list
				_sort.toggle_sort(phdr->iItem);
				_sort.sort();
			}
		}
		break;}
	}

	return 0;
}

int CALLBACK FindProgramDlg::CompareFunc(LPARAM lparam1, LPARAM lparam2, LPARAM lparamSort)
{
	ListSort* sort = (ListSort*)lparamSort;

	FPDEntry& a = *(FPDEntry*)lparam1;
	FPDEntry& b = *(FPDEntry*)lparam2;

	int cmp = 0;

	switch(sort->_sort_crit) {
	  case 0:
		cmp = _tcsicoll(a._entry->_display_name, b._entry->_display_name);
		break;

	  case 1:
		cmp = _tcsicoll(a._path, b._path);
		break;

	  case 2:
		cmp = _tcsicoll(a._menu_path, b._menu_path);
	}

	return sort->_direction? -cmp: cmp;
}

void FindProgramDlg::CheckEntries()
{
	///@todo check all entries for existing targets, display a list of not working entries and ask the user for permission to delete them
}
