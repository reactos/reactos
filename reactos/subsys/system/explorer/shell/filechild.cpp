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
 // filechild.cpp
 //
 // Martin Fuchs, 23.07.2003
 //


#include "../utility/utility.h"

#include "../explorer.h"
#include "../globals.h"
#include "ntobjfs.h"
#include "regfs.h"
#include "fatfs.h"

#include "../explorer_intres.h"


FileChildWndInfo::FileChildWndInfo(HWND hmdiclient, LPCTSTR path, ENTRY_TYPE etype)
 :	super(hmdiclient),
	_etype(etype)
{
	if (etype == ET_UNKNOWN)
#ifdef __WINE__
		if (*path == '/')
			_etype = ET_UNIX;
		else
#endif
			_etype = ET_WINDOWS;

	_path = path;

	_pos.length = sizeof(WINDOWPLACEMENT);
	_pos.flags = 0;
	_pos.showCmd = SW_SHOWNORMAL;
	_pos.rcNormalPosition.left = CW_USEDEFAULT;
	_pos.rcNormalPosition.top = CW_USEDEFAULT;
	_pos.rcNormalPosition.right = CW_USEDEFAULT;
	_pos.rcNormalPosition.bottom = CW_USEDEFAULT;

	_open_mode = OWM_EXPLORE|OWM_DETAILS;
}


ShellChildWndInfo::ShellChildWndInfo(HWND hmdiclient, LPCTSTR path, const ShellPath& root_shell_path)
 :	FileChildWndInfo(hmdiclient, path, ET_SHELL),
	_shell_path(path&&*path? path: root_shell_path),
	_root_shell_path(root_shell_path)
{
}


NtObjChildWndInfo::NtObjChildWndInfo(HWND hmdiclient, LPCTSTR path)
 :	FileChildWndInfo(hmdiclient, path, ET_NTOBJS)
{
}


RegistryChildWndInfo::RegistryChildWndInfo(HWND hmdiclient, LPCTSTR path)
 :	FileChildWndInfo(hmdiclient, path, ET_REGISTRY)
{
}


FATChildWndInfo::FATChildWndInfo(HWND hmdiclient, LPCTSTR path)
 :	FileChildWndInfo(hmdiclient, path, ET_FAT)
{
}


WebChildWndInfo::WebChildWndInfo(HWND hmdiclient, LPCTSTR url)
 :	FileChildWndInfo(hmdiclient, url, ET_WEB)
{
}


FileChildWindow::FileChildWindow(HWND hwnd, const FileChildWndInfo& info)
 :	ChildWindow(hwnd, info)
{
	CONTEXT("FileChildWindow::FileChildWindow()");

	TCHAR drv[_MAX_DRIVE+1];
	Entry* entry;

	switch(info._etype) {
	  case ET_SHELL: {	//@@ evtl. Aufteilung von FileChildWindow in ShellChildWindow, WinChildWindow, UnixChildWindow
		_root._drive_type = DRIVE_UNKNOWN;
		lstrcpy(drv, TEXT("\\"));
		lstrcpy(_root._volname, TEXT("Desktop"));
		_root._fs_flags = 0;
		lstrcpy(_root._fs, TEXT("Shell"));

		const ShellChildWndInfo& shell_info = static_cast<const ShellChildWndInfo&>(info);
		_root._entry = new ShellDirectory(GetDesktopFolder(), DesktopFolderPath(), hwnd);
		entry = _root._entry->read_tree((LPCTSTR)&*shell_info._shell_path, SORT_NAME);
		break;}

#ifdef __WINE__
	  case ET_UNIX: {
		_root._drive_type = GetDriveType(info._path);

		_tsplitpath(info._path, drv, NULL, NULL, NULL);
		lstrcat(drv, TEXT("/"));
		lstrcpy(_root._volname, TEXT("root fs"));
		_root._fs_flags = 0;
		lstrcpy(_root._fs, TEXT("unixfs"));
		lstrcpy(_root._path, TEXT("/"));
		_root._entry = new UnixDirectory(_root._path);
		entry = _root._entry->read_tree(info._path, SORT_NAME);
		break;}

#endif

	  case ET_NTOBJS:
		_root._drive_type = DRIVE_UNKNOWN;

		_tsplitpath(info._path, drv, NULL, NULL, NULL);
		lstrcat(drv, TEXT("\\"));
		lstrcpy(_root._volname, TEXT("NT Object Namespace"));
		lstrcpy(_root._fs, TEXT("NTOBJ"));
		lstrcpy(_root._path, drv);
		_root._entry = new NtObjDirectory(_root._path);
		entry = _root._entry->read_tree(info._path, SORT_NAME);
		break;

	  case ET_REGISTRY:
		_root._drive_type = DRIVE_UNKNOWN;

		_tsplitpath(info._path, drv, NULL, NULL, NULL);
		lstrcat(drv, TEXT("\\"));
		lstrcpy(_root._volname, TEXT("Registry"));
		lstrcpy(_root._fs, TEXT("Registry"));
		lstrcpy(_root._path, drv);
		_root._entry = new RegistryRoot();
		entry = _root._entry->read_tree(info._path, SORT_NONE);
		break;

	  case ET_FAT:
		_root._drive_type = DRIVE_UNKNOWN;

		_tsplitpath(info._path, drv, NULL, NULL, NULL);
		lstrcat(drv, TEXT("\\"));
		lstrcpy(_root._volname, TEXT("FAT XXX"));	//@@
		lstrcpy(_root._fs, TEXT("FAT"));
		lstrcpy(_root._path, drv);
		_root._entry = new FATDrive(TEXT("c:/reactos-bochs/cdrv.img"));	//TEXT("\\\\.\\F:"));	//@@
		entry = _root._entry->read_tree(info._path, SORT_NONE);
		break;

	  default:	// ET_WINDOWS
		_root._drive_type = GetDriveType(info._path);

		_tsplitpath(info._path, drv, NULL, NULL, NULL);
		lstrcat(drv, TEXT("\\"));
		GetVolumeInformation(drv, _root._volname, _MAX_FNAME, 0, 0, &_root._fs_flags, _root._fs, _MAX_DIR);
		lstrcpy(_root._path, drv);
		_root._entry = new WinDirectory(_root._path);
		entry = _root._entry->read_tree(info._path, SORT_NAME);
	}

	if (info._etype != ET_SHELL)
		wsprintf(_root._entry->_data.cFileName, TEXT("%s - %s"), drv, _root._fs);
/*@@else
		lstrcpy(_root._entry->_data.cFileName, TEXT("GetDesktopFolder"));*/

	_root._entry->_data.dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;


	if (info._open_mode & OWM_EXPLORE)	///@todo Is not-explore-mode for FileChildWindow completely implemented?
		_left_hwnd = *(_left=new Pane(_hwnd, IDW_TREE_LEFT, IDW_HEADER_LEFT, _root._entry, true, COL_CONTENT));

	_right_hwnd = *(_right=new Pane(_hwnd, IDW_TREE_RIGHT, IDW_HEADER_RIGHT, NULL, false,
									COL_TYPE|COL_SIZE|COL_DATE|COL_TIME|COL_ATTRIBUTES|COL_INDEX|COL_LINKS|COL_CONTENT));

	_sortOrder = SORT_NAME;
	_header_wdths_ok = false;

	set_curdir(entry, hwnd);

	if (_left_hwnd) {
		int idx = ListBox_FindItemData(_left_hwnd, ListBox_GetCurSel(_left_hwnd), _left->_cur);
		ListBox_SetCurSel(_left_hwnd, idx);
	}

	 ///@todo scroll to visibility

}

FileChildWindow::~FileChildWindow()
{
}


void FileChildWindow::set_curdir(Entry* entry, HWND hwnd)
{
	CONTEXT("FileChildWindow::set_curdir()");

	_path[0] = TEXT('\0');

	_left->_cur = entry;
	_right->_root = entry&&entry->_down? entry->_down: entry;
	_right->_cur = entry;

	if (entry) {
		if (!entry->_scanned)
			scan_entry(entry, hwnd);
		else {
			ListBox_ResetContent(_right_hwnd);
			_right->insert_entries(entry->_down, -1);
			_right->calc_widths(false);
			_right->set_header();
		}

		entry->get_path(_path);
	}

	if (hwnd)	// only change window title, if the window already exists
		SetWindowText(hwnd, _path);

	if (_path[0])
		if (!SetCurrentDirectory(_path))
			_path[0] = TEXT('\0');
}


 // expand a directory entry

bool FileChildWindow::expand_entry(Entry* dir)
{
	int idx;
	Entry* p;

	if (!dir || dir->_expanded || !dir->_down)
		return false;

	p = dir->_down;

	if (p->_data.cFileName[0]=='.' && p->_data.cFileName[1]=='\0' && p->_next) {
		p = p->_next;

		if (p->_data.cFileName[0]=='.' && p->_data.cFileName[1]=='.' &&
				p->_data.cFileName[2]=='\0' && p->_next)
			p = p->_next;
	}

	 // no subdirectories ?
	if (!(p->_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&	// not a directory?
		!p->_down)	// not a file with NTFS sub-streams?
		return FALSE;

	idx = ListBox_FindItemData(_left_hwnd, 0, dir);

	dir->_expanded = true;

	 // insert entries in left pane
	_left->insert_entries(p, idx);

	if (!_header_wdths_ok) {
		if (_left->calc_widths(false)) {
			_left->set_header();

			_header_wdths_ok = true;
		}
	}

	return true;
}


void FileChildWindow::collapse_entry(Pane* pane, Entry* dir)
{
	int idx = ListBox_FindItemData(*pane, 0, dir);

	SendMessage(*pane, WM_SETREDRAW, FALSE, 0);	//ShowWindow(*pane, SW_HIDE);

	 // hide sub entries
	for(;;) {
		LRESULT res = ListBox_GetItemData(*pane, idx+1);
		Entry* sub = (Entry*) res;

		if (res==LB_ERR || !sub || sub->_level<=dir->_level)
			break;

		ListBox_DeleteString(*pane, idx+1);
	}

	dir->_expanded = false;

	SendMessage(*pane, WM_SETREDRAW, TRUE, 0);	//ShowWindow(*pane, SW_SHOW);
}


FileChildWindow* FileChildWindow::create(const FileChildWndInfo& info)
{
	CONTEXT("FileChildWindow::create()");

	MDICREATESTRUCT mcs;

	mcs.szClass = CLASSNAME_WINEFILETREE;
	mcs.szTitle = (LPTSTR)info._path;
	mcs.hOwner	= g_Globals._hInstance;
	mcs.x		= info._pos.rcNormalPosition.left;
	mcs.y		= info._pos.rcNormalPosition.top;
	mcs.cx		= info._pos.rcNormalPosition.right - info._pos.rcNormalPosition.left;
	mcs.cy		= info._pos.rcNormalPosition.bottom - info._pos.rcNormalPosition.top;
	mcs.style	= 0;
	mcs.lParam	= 0;

	FileChildWindow* child = static_cast<FileChildWindow*>(
		create_mdi_child(info, mcs, WINDOW_CREATOR_INFO(FileChildWindow,FileChildWndInfo)));

	return child;
}


void FileChildWindow::resize_children(int cx, int cy)
{
	HDWP hdwp = BeginDeferWindowPos(4);
	RECT rt;

	rt.left   = 0;
	rt.top    = 0;
	rt.right  = cx;
	rt.bottom = cy;

	cx = _split_pos + SPLIT_WIDTH/2;

	{
		WINDOWPOS wp;
		HD_LAYOUT hdl;

		hdl.prc   = &rt;
		hdl.pwpos = &wp;

		Header_Layout(_left->_hwndHeader, &hdl);

		hdwp = DeferWindowPos(hdwp, _left->_hwndHeader, wp.hwndInsertAfter,
							wp.x-1, wp.y, _split_pos-SPLIT_WIDTH/2+1, wp.cy, wp.flags);

		hdwp = DeferWindowPos(hdwp, _right->_hwndHeader, wp.hwndInsertAfter,
								rt.left+cx+1, wp.y, wp.cx-cx+2, wp.cy, wp.flags);
	}

	if (_left_hwnd)
		hdwp = DeferWindowPos(hdwp, _left_hwnd, 0, rt.left, rt.top, _split_pos-SPLIT_WIDTH/2-rt.left, rt.bottom-rt.top, SWP_NOZORDER|SWP_NOACTIVATE);

	hdwp = DeferWindowPos(hdwp, _right_hwnd, 0, rt.left+cx+1, rt.top, rt.right-cx, rt.bottom-rt.top, SWP_NOZORDER|SWP_NOACTIVATE);

	EndDeferWindowPos(hdwp);
}


LRESULT FileChildWindow::WndProc(UINT nmsg, WPARAM wparam, LPARAM lparam)
{
	switch(nmsg) {
		case WM_DRAWITEM: {
			LPDRAWITEMSTRUCT dis = (LPDRAWITEMSTRUCT)lparam;
			Entry* entry = (Entry*) dis->itemData;

			if (dis->CtlID == IDW_TREE_LEFT)
				_left->draw_item(dis, entry);
			else
				_right->draw_item(dis, entry);

			return TRUE;}

		case WM_SIZE:
			if (wparam != SIZE_MINIMIZED)
				resize_children(LOWORD(lparam), HIWORD(lparam));
			return DefMDIChildProc(_hwnd, nmsg, wparam, lparam);

		case PM_GET_FILEWND_PTR:
			return (LRESULT)this;

		case WM_SETFOCUS: {
			TCHAR path[MAX_PATH];

			if (_left->_cur) {
				_left->_cur->get_path(path);
				SetCurrentDirectory(path);
			}

			SetFocus(_focus_pane? _right_hwnd: _left_hwnd);
			goto def;}

		case PM_DISPATCH_COMMAND: {
			Pane* pane = GetFocus()==_left_hwnd? _left: _right;

			switch(LOWORD(wparam)) {
			  case ID_WINDOW_NEW: {CONTEXT("FileChildWindow PM_DISPATCH_COMMAND ID_WINDOW_NEW");
				if (_root._entry->_etype == ET_SHELL)
					FileChildWindow::create(ShellChildWndInfo(GetParent(_hwnd)/*_hmdiclient*/, _path, DesktopFolderPath()));
				else
					FileChildWindow::create(FileChildWndInfo(GetParent(_hwnd)/*_hmdiclient*/, _path));
				break;}

			  case ID_REFRESH: {CONTEXT("ID_REFRESH");
				bool expanded = _left->_cur->_expanded;

				scan_entry(_left->_cur, _hwnd);

				if (expanded)
					expand_entry(_left->_cur);
				break;}

			  case ID_ACTIVATE: {CONTEXT("ID_ACTIVATE");
				activate_entry(pane, _hwnd);
				break;}

			  default:
				return pane->command(LOWORD(wparam));
			}

			return TRUE;}

		case WM_CONTEXTMENU: {
			 // first select the current item in the listbox
			HWND hpanel = (HWND) wparam;
			POINTS& pos = MAKEPOINTS(lparam);
			POINT pt; POINTSTOPOINT(pt, pos);
			ScreenToClient(hpanel, &pt);
			SendMessage(hpanel, WM_LBUTTONDOWN, 0, MAKELONG(pt.x, pt.y));
			SendMessage(hpanel, WM_LBUTTONUP, 0, MAKELONG(pt.x, pt.y));

			 // now create the popup menu using shell namespace and IContextMenu
			Pane* pane = GetFocus()==_left_hwnd? _left: _right;
			int idx = ListBox_GetCurSel(*pane);
			if (idx != -1) {
				Entry* entry = (Entry*) ListBox_GetItemData(*pane, idx);

				ShellPath shell_path = entry->create_absolute_pidl();
				LPCITEMIDLIST pidl_abs = shell_path;

				IShellFolder* parentFolder;
				LPCITEMIDLIST pidlLast;

				 // get and use the parent folder to display correct context menu in all cases -> correct "Properties" dialog for directories, ...
				if (SUCCEEDED(SHBindToParent(pidl_abs, IID_IShellFolder, (LPVOID*)&parentFolder, &pidlLast))) {
					HRESULT hr = ShellFolderContextMenu(GetDesktopFolder(), _hwnd, 1, &pidlLast, pos.x, pos.y);

					parentFolder->Release();

					CHECKERROR(hr);
				}
			}
			break;}

		default: def:
			return super::WndProc(nmsg, wparam, lparam);
	}

	return 0;
}


int FileChildWindow::Command(int id, int code)
{
	Pane* pane = GetFocus()==_left_hwnd? _left: _right;

	switch(code) {
	  case LBN_SELCHANGE: {
		int idx = ListBox_GetCurSel(*pane);
		Entry* entry = (Entry*) ListBox_GetItemData(*pane, idx);

		if (pane == _left)
			set_curdir(entry, _hwnd);
		else
			pane->_cur = entry;
		break;}

	  case LBN_DBLCLK:
		activate_entry(pane, _hwnd);
		break;
	}

	return 0;
}


void FileChildWindow::activate_entry(Pane* pane, HWND hwnd)
{
	Entry* entry = pane->_cur;

	if (!entry)
		return;

	if ((entry->_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ||	// a directory?
		entry->_down)	// a file with NTFS sub-streams?
	{
		int scanned_old = entry->_scanned;

		if (!scanned_old)
			scan_entry(entry, hwnd);

		if (entry->_data.cFileName[0]==TEXT('.') && entry->_data.cFileName[1]==TEXT('\0'))
			return;

		if (entry->_data.cFileName[0]==TEXT('.') && entry->_data.cFileName[1]==TEXT('.') && entry->_data.cFileName[2]==TEXT('\0')) {
			entry = _left->_cur->_up;
			collapse_entry(_left, entry);
			goto focus_entry;
		} else if (entry->_expanded)
			collapse_entry(pane, _left->_cur);
		else {
			expand_entry(_left->_cur);

			if (!pane->_treePane) focus_entry: {
				int idx = ListBox_FindItemData(_left_hwnd, ListBox_GetCurSel(_left_hwnd), entry);
				ListBox_SetCurSel(_left_hwnd, idx);
				set_curdir(entry, _hwnd);
			}
		}

		if (!scanned_old) {
			pane->calc_widths(FALSE);

			pane->set_header();
		}
	} else {
		entry->launch_entry(_hwnd);
	}
}


void FileChildWindow::scan_entry(Entry* entry, HWND hwnd)
{
	CONTEXT("FileChildWindow::scan_entry()");

	int idx = ListBox_GetCurSel(_left_hwnd);
	HCURSOR old_cursor = SetCursor(LoadCursor(0, IDC_WAIT));

	 // delete sub entries in left pane
	for(;;) {
		LRESULT res = ListBox_GetItemData(_left_hwnd, idx+1);
		Entry* sub = (Entry*) res;

		if (res==LB_ERR || !sub || sub->_level<=entry->_level)
			break;

		ListBox_DeleteString(_left_hwnd, idx+1);
	}

	 // empty right pane
	ListBox_ResetContent(_right_hwnd);

	 // release memory
	entry->free_subentries();
	entry->_expanded = false;

	 // read contents from disk
	entry->read_directory(_sortOrder);

	 // insert found entries in right pane
	_right->insert_entries(entry->_down, -1);

	_right->calc_widths(false);
	_right->set_header();

	_header_wdths_ok = FALSE;

	SetCursor(old_cursor);
}


int FileChildWindow::Notify(int id, NMHDR* pnmh)
{
	return (pnmh->idFrom==IDW_HEADER_LEFT? _left: _right)->Notify(id, pnmh);
}


BOOL CALLBACK ExecuteDialog::WndProc(HWND hwnd, UINT nmsg, WPARAM wparam, LPARAM lparam)
{
	static struct ExecuteDialog* dlg;

	switch(nmsg) {
	  case WM_INITDIALOG:
		dlg = (struct ExecuteDialog*) lparam;
		return 1;

	  case WM_COMMAND: {
		int id = (int)wparam;

		if (id == IDOK) {
			GetWindowText(GetDlgItem(hwnd, 201), dlg->cmd, MAX_PATH);
			dlg->cmdshow = Button_GetState(GetDlgItem(hwnd,214))&BST_CHECKED?
											SW_SHOWMINIMIZED: SW_SHOWNORMAL;
			EndDialog(hwnd, id);
		} else if (id == IDCANCEL)
			EndDialog(hwnd, id);

		return 1;}
	}

	return 0;
}
