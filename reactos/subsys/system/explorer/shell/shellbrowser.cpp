/*
 * Copyright 2003, 2004 Martin Fuchs
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
 // shellbrowser.cpp
 //
 // Martin Fuchs, 23.07.2003
 //


#include "precomp.h"

#include "../explorer_intres.h"


ShellBrowser::ShellBrowser(HWND hwnd, HWND left_hwnd, WindowHandle& right_hwnd, ShellPathInfo& create_info, HIMAGELIST himl, BrowserCallback* cb)
 :	_hwnd(hwnd),
	_left_hwnd(left_hwnd),
	_right_hwnd(right_hwnd),
	_create_info(create_info),
	_himl(himl),
	_callback(cb)
{
	_pShellView = NULL;
	_pDropTarget = NULL;
	_last_sel = 0;

	_cur_dir = NULL;
}

ShellBrowser::~ShellBrowser()
{
	if (_pShellView)
		_pShellView->Release();

	if (_pDropTarget) {
		_pDropTarget->Release();
		_pDropTarget = NULL;
	}

	if (_right_hwnd) {
		DestroyWindow(_right_hwnd);
		_right_hwnd = 0;
	}
}


LRESULT ShellBrowser::Init(HWND hWndFrame)
{
	CONTEXT("ShellBrowser::Init()");

	_hWndFrame = hWndFrame;

	const String& root_name = GetDesktopFolder().get_name(_create_info._root_shell_path, SHGDN_FORPARSING);

	_root._drive_type = DRIVE_UNKNOWN;
	lstrcpy(_root._volname, root_name);	// most of the time "Desktop"
	_root._fs_flags = 0;
	lstrcpy(_root._fs, TEXT("Desktop"));

	_root._entry = new ShellDirectory(GetDesktopFolder(), _create_info._root_shell_path, _hwnd);

	jump_to(_create_info._shell_path);

	 // -> set_curdir()
	_root._entry->read_directory();

	/* already filled by ShellDirectory constructor
	lstrcpy(_root._entry->_data.cFileName, TEXT("Desktop")); */

	return 0;
}

void ShellBrowser::jump_to(LPCITEMIDLIST pidl)
{
	Entry* entry = NULL;

	 //@@
	if (!_cur_dir)
		_cur_dir = static_cast<ShellDirectory*>(_root._entry);

/*@todo
	we should call read_tree() here to iterate through the hierarchy and open all folders from shell_info._root_shell_path to shell_info._shell_path
	_root._entry->read_tree(shell_info._root_shell_path.get_folder(), info._shell_path, SORT_NAME);
	-> see FileChildWindow::FileChildWindow()
*/

	if (_cur_dir) {
		static DynamicFct<LPITEMIDLIST(WINAPI*)(LPCITEMIDLIST, LPCITEMIDLIST)> ILFindChild(TEXT("SHELL32"), 24);

		LPCITEMIDLIST child_pidl;

		if (ILFindChild)
			child_pidl = (*ILFindChild)(_cur_dir->create_absolute_pidl(), pidl);
		else
			child_pidl = pidl;	// This is not correct in the common case, but works on the desktop level.

		if (child_pidl) {
			_cur_dir->smart_scan();

			entry = _cur_dir->find_entry(child_pidl);

			if (entry)
				_callback->entry_selected(entry);
		}
	}

		//@@ work around as long as we don't iterate correctly through the ShellEntry tree
	if (!entry)
		UpdateFolderView(ShellFolder(pidl));
}


void ShellBrowser::InitializeTree(HIMAGELIST himl)
{
	CONTEXT("ShellBrowserChild::InitializeTree()");

	TreeView_SetImageList(_left_hwnd, himl, TVSIL_NORMAL);
	TreeView_SetScrollTime(_left_hwnd, 100);

	TV_INSERTSTRUCT tvInsert;

	tvInsert.hParent = 0;
	tvInsert.hInsertAfter = TVI_LAST;

	TV_ITEM& tvItem = tvInsert.item;
	tvItem.mask = TVIF_PARAM | TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_CHILDREN;
	tvItem.lParam = (LPARAM)_root._entry;
	tvItem.pszText = LPSTR_TEXTCALLBACK;
	tvItem.iImage = tvItem.iSelectedImage = I_IMAGECALLBACK;
	tvItem.cChildren = 1;

	HTREEITEM hItem = TreeView_InsertItem(_left_hwnd, &tvInsert);
	TreeView_SelectItem(_left_hwnd, hItem);
	TreeView_Expand(_left_hwnd, hItem, TVE_EXPAND);
}

bool ShellBrowser::InitDragDrop()
{
	CONTEXT("ShellBrowser::InitDragDrop()");

	_pDropTarget = new TreeDropTarget(_left_hwnd);

	if (!_pDropTarget)
		return false;

	_pDropTarget->AddRef();

	if (FAILED(RegisterDragDrop(_left_hwnd, _pDropTarget))) {//calls addref
		_pDropTarget->Release(); // free TreeDropTarget
		_pDropTarget = NULL;
		return false;
	}
	else
		_pDropTarget->Release();

	FORMATETC ftetc;

	ftetc.dwAspect = DVASPECT_CONTENT;
	ftetc.lindex = -1;
	ftetc.tymed = TYMED_HGLOBAL;
	ftetc.cfFormat = CF_HDROP;

	_pDropTarget->AddSuportedFormat(ftetc);

	return true;
}


void ShellBrowser::OnTreeItemRClick(int idCtrl, LPNMHDR pnmh)
{
	CONTEXT("ShellBrowser::OnTreeItemRClick()");

	TVHITTESTINFO tvhti;

	GetCursorPos(&tvhti.pt);
	ScreenToClient(_left_hwnd, &tvhti.pt);

	tvhti.flags = LVHT_NOWHERE;
	TreeView_HitTest(_left_hwnd, &tvhti);

	if (TVHT_ONITEM & tvhti.flags) {
		ClientToScreen(_left_hwnd, &tvhti.pt);
		Tree_DoItemMenu(_left_hwnd, tvhti.hItem, &tvhti.pt);
	}
}

void ShellBrowser::Tree_DoItemMenu(HWND hwndTreeView, HTREEITEM hItem, LPPOINT pptScreen)
{
	CONTEXT("ShellBrowser::Tree_DoItemMenu()");

	LPARAM itemData = TreeView_GetItemData(hwndTreeView, hItem);

	if (itemData) {
		Entry* entry = (Entry*)itemData;

		if (entry->_etype == ET_SHELL) {
			ShellDirectory* dir = static_cast<ShellDirectory*>(entry->_up);
			ShellFolder folder = dir? dir->_folder: GetDesktopFolder();
			LPCITEMIDLIST pidl = static_cast<ShellEntry*>(entry)->_pidl;

			CHECKERROR(ShellFolderContextMenu(folder, ::GetParent(hwndTreeView), 1, &pidl, pptScreen->x, pptScreen->y));
		} else {
			ShellPath shell_path = entry->create_absolute_pidl();
			LPCITEMIDLIST pidl = shell_path;

			///@todo use parent folder instead of desktop
			CHECKERROR(ShellFolderContextMenu(GetDesktopFolder(), _hwnd, 1, &pidl, pptScreen->x, pptScreen->y));
		}
	}
}

void ShellBrowser::OnTreeGetDispInfo(int idCtrl, LPNMHDR pnmh)
{
	CONTEXT("ShellBrowser::OnTreeGetDispInfo()");

	LPNMTVDISPINFO lpdi = (LPNMTVDISPINFO)pnmh;
	ShellEntry* entry = (ShellEntry*)lpdi->item.lParam;

	if (entry) {
		if (lpdi->item.mask & TVIF_TEXT)
			lpdi->item.pszText = entry->_display_name;

		if (lpdi->item.mask & (/*TVIF_TEXT|*/TVIF_IMAGE|TVIF_SELECTEDIMAGE)) {
			ShellPath pidl_abs = entry->create_absolute_pidl();	// Caching of absolute PIDLs could enhance performance.
			LPCITEMIDLIST pidl = pidl_abs;

			SHFILEINFO sfi;
/*
			if (lpdi->item.mask & TVIF_TEXT)
				if (SHGetFileInfo((LPCTSTR)pidl, 0, &sfi, sizeof(sfi), SHGFI_PIDL|SHGFI_DISPLAYNAME))
					lstrcpy(lpdi->item.pszText, sfi.szDisplayName);	///@todo look at cchTextMax if there is enough space available
				else
					lpdi->item.pszText = entry->_data.cFileName;
*/
			if (lpdi->item.mask & TVIF_IMAGE)
				if ((HIMAGELIST)SHGetFileInfo((LPCTSTR)pidl, 0, &sfi, sizeof(sfi), SHGFI_PIDL|SHGFI_SYSICONINDEX|SHGFI_SMALLICON|SHGFI_LINKOVERLAY) == _himl)
					lpdi->item.iImage = sfi.iIcon;
				else
					lpdi->item.iImage = -1;

			if (lpdi->item.mask & TVIF_SELECTEDIMAGE)
				if ((HIMAGELIST)SHGetFileInfo((LPCTSTR)pidl, 0, &sfi, sizeof(sfi), SHGFI_PIDL|SHGFI_SYSICONINDEX|SHGFI_SMALLICON|SHGFI_OPENICON) == _himl)
					lpdi->item.iSelectedImage = sfi.iIcon;
				else
					lpdi->item.iSelectedImage = -1;
		}
	}
}

void ShellBrowser::OnTreeItemExpanding(int idCtrl, LPNMTREEVIEW pnmtv)
{
	CONTEXT("ShellBrowser::OnTreeItemExpanding()");

	if (pnmtv->action == TVE_COLLAPSE)
        TreeView_Expand(_left_hwnd, pnmtv->itemNew.hItem, TVE_COLLAPSE|TVE_COLLAPSERESET);
    else if (pnmtv->action == TVE_EXPAND) {
		ShellDirectory* entry = (ShellDirectory*)TreeView_GetItemData(_left_hwnd, pnmtv->itemNew.hItem);

		if (entry)
			if (!InsertSubitems(pnmtv->itemNew.hItem, entry, entry->_folder)) {
				entry->_shell_attribs &= ~SFGAO_HASSUBFOLDER;

				 // remove subitem "+"
				TV_ITEM tvItem;

				tvItem.mask = TVIF_CHILDREN;
				tvItem.hItem = pnmtv->itemNew.hItem;
				tvItem.cChildren = 0;

				TreeView_SetItem(_left_hwnd, &tvItem);
			}
	}
}

int ShellBrowser::InsertSubitems(HTREEITEM hParentItem, Entry* entry, IShellFolder* pParentFolder)
{
	CONTEXT("ShellBrowser::InsertSubitems()");

	WaitCursor wait;

	int cnt = 0;

	SendMessage(_left_hwnd, WM_SETREDRAW, FALSE, 0);

	try {
		entry->smart_scan();
	} catch(COMException& e) {
		HandleException(e, g_Globals._hMainWnd);
	}

	TV_ITEM tvItem;
	TV_INSERTSTRUCT tvInsert;

	for(entry=entry->_down; entry; entry=entry->_next) {
#ifndef _LEFT_FILES
		if (entry->_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
#endif
		{
			ZeroMemory(&tvItem, sizeof(tvItem));

			tvItem.mask = TVIF_PARAM | TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_CHILDREN;
			tvItem.pszText = LPSTR_TEXTCALLBACK;
			tvItem.iImage = tvItem.iSelectedImage = I_IMAGECALLBACK;
			tvItem.lParam = (LPARAM)entry;
			tvItem.cChildren = entry->_shell_attribs & SFGAO_HASSUBFOLDER? 1: 0;

			if (entry->_shell_attribs & SFGAO_SHARE) {
				tvItem.mask |= TVIF_STATE;
				tvItem.stateMask |= TVIS_OVERLAYMASK;
				tvItem.state |= INDEXTOOVERLAYMASK(1);
			}

			tvInsert.item = tvItem;
			tvInsert.hInsertAfter = TVI_LAST;
			tvInsert.hParent = hParentItem;

			TreeView_InsertItem(_left_hwnd, &tvInsert);
		}

		++cnt;
	}

	SendMessage(_left_hwnd, WM_SETREDRAW, TRUE, 0);

	return cnt;
}

void ShellBrowser::OnTreeItemSelected(int idCtrl, LPNMTREEVIEW pnmtv)
{
	CONTEXT("ShellBrowser::OnTreeItemSelected()");

	ShellEntry* entry = (ShellEntry*)pnmtv->itemNew.lParam;

	_last_sel = pnmtv->itemNew.hItem;

	if (entry)
		_callback->entry_selected(entry);
}

void ShellBrowser::UpdateFolderView(IShellFolder* folder)
{
	CONTEXT("ShellBrowser::UpdateFolderView()");

	FOLDERSETTINGS fs;
	IShellView* pLastShellView = _pShellView;

	_folder = folder;

	if (pLastShellView)
		pLastShellView->GetCurrentInfo(&fs);
	else {
		fs.ViewMode = _create_info._open_mode&OWM_DETAILS? FVM_DETAILS: FVM_ICON;
		fs.fFlags = FWF_NOCLIENTEDGE|FWF_BESTFITWINDOW;
	}

	HRESULT hr = folder->CreateViewObject(_hwnd, IID_IShellView, (void**)&_pShellView);

	if (FAILED(hr)) {
		_pShellView = NULL;
		return;
	}

	RECT rect = {CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT};
	hr = _pShellView->CreateViewWindow(pLastShellView, &fs, static_cast<IShellBrowser*>(this), &rect, &_right_hwnd/*&m_hWndListView*/);

	if (pLastShellView) {
		pLastShellView->GetCurrentInfo(&fs);
		pLastShellView->UIActivate(SVUIA_DEACTIVATE);
		pLastShellView->DestroyViewWindow();
		pLastShellView->Release();
	}

	_pShellView->UIActivate(SVUIA_ACTIVATE_NOFOCUS);
}


HRESULT ShellBrowser::OnDefaultCommand(LPIDA pida)
{
	CONTEXT("ShellBrowser::OnDefaultCommand()");

	if (pida->cidl >= 1) {
		if (_left_hwnd) {	// explorer mode
			if (_last_sel) {
				ShellDirectory* parent = (ShellDirectory*)TreeView_GetItemData(_left_hwnd, _last_sel);

				if (parent) {
					try {
						parent->smart_scan();
					} catch(COMException& e) {
						return e.Error();
					}

					UINT firstOffset = pida->aoffset[1];
					LPITEMIDLIST pidl = (LPITEMIDLIST)((LPBYTE)pida+firstOffset);

					Entry* entry = parent->find_entry(pidl);

					if (entry && (entry->_data.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY))
						if (entry->_etype == ET_SHELL)
							if (_last_sel && select_entry(_last_sel, entry))
								return S_OK;
				}
			}
		} else { // no tree control
			if (MainFrameBase::OpenShellFolders(pida, _hWndFrame))
				return S_OK;

/* create new Frame Window
			if (MainFrame::OpenShellFolders(pida, 0))
				return S_OK;
*/
		}
	}

	return E_NOTIMPL;
}


HTREEITEM ShellBrowser::select_entry(HTREEITEM hitem, Entry* entry, bool expand)
{
	CONTEXT("ShellBrowser::select_entry()");

	if (expand && !TreeView_Expand(_left_hwnd, hitem, TVE_EXPAND))
		return 0;

	for(hitem=TreeView_GetChild(_left_hwnd,hitem); hitem; hitem=TreeView_GetNextSibling(_left_hwnd,hitem)) {
		if ((Entry*)TreeView_GetItemData(_left_hwnd,hitem) == entry) {
			if (TreeView_SelectItem(_left_hwnd, hitem)) {
				if (expand)
					TreeView_Expand(_left_hwnd, hitem, TVE_EXPAND);

				return hitem;
			}

			break;
		}
	}

	return 0;
}


bool ShellBrowser::jump_to_pidl(LPCITEMIDLIST pidl)
{
	if (!_root._entry)
		return false;

	 // iterate through the hierarchy and open all folders to reach pidl
	WaitCursor wait;

	HTREEITEM hitem = TreeView_GetRoot(_left_hwnd);
	Entry* entry = _root._entry;

	for(const void*p=pidl;;) {
		if (!p)
			return true;

		if (!entry || !hitem)
			break;

		entry->smart_scan(SORT_NAME);

		Entry* found = entry->find_entry(p);
		p = entry->get_next_path_component(p);

		if (found)
			hitem = select_entry(hitem, found);

		entry = found;
	}

	return false;
}


#ifndef _NO_MDI

MDIShellBrowserChild::MDIShellBrowserChild(HWND hwnd, const ShellChildWndInfo& info)
 :	super(hwnd, info),
	_create_info(info),
	_shellpath_info(info)	//@@ copies info -> no referenz to _create_info !
{
/**todo Conversion of shell path into path string -> store into URL history
	 // store path into history
	if (info._path && *info._path)
		_url_history.push(info._path);
*/
}


MDIShellBrowserChild* MDIShellBrowserChild::create(const ShellChildWndInfo& info)
{
	ChildWindow* child = ChildWindow::create(info, info._pos.rcNormalPosition,
		WINDOW_CREATOR_INFO(MDIShellBrowserChild,ShellChildWndInfo), CLASSNAME_CHILDWND, NULL, info._pos.showCmd==SW_SHOWMAXIMIZED? WS_MAXIMIZE: 0);

	return static_cast<MDIShellBrowserChild*>(child);
}


LRESULT MDIShellBrowserChild::Init(LPCREATESTRUCT pcs)
{
	CONTEXT("MDIShellBrowserChild::Init()");

	if (super::Init(pcs))
		return 1;

	init_himl();

	update_shell_browser();

	if (&*_shellBrowser)
		if (_left_hwnd)
			_shellBrowser->Init(_himlSmall);
		else
			_shellBrowser->UpdateFolderView(_create_info._shell_path.get_folder());

	return 0;
}


LRESULT MDIShellBrowserChild::WndProc(UINT nmsg, WPARAM wparam, LPARAM lparam)
{
	switch(nmsg) {
	  case PM_DISPATCH_COMMAND: {
		switch(LOWORD(wparam)) {
		  case ID_WINDOW_NEW: {CONTEXT("MDIShellBrowserChild PM_DISPATCH_COMMAND ID_WINDOW_NEW");
			MDIShellBrowserChild::create(_create_info);
			break;}

		  case ID_REFRESH:
			//@todo refresh shell child
			break;

		  case ID_VIEW_SDI:
			MainFrameBase::Create(_url, false);
			break;

		  default:
			return super::WndProc(nmsg, wparam, lparam);
		}
		return TRUE;}
	
	  default:
		return super::WndProc(nmsg, wparam, lparam);
	}

	return 0;
}

void MDIShellBrowserChild::update_shell_browser()
{
	int split_pos = DEFAULT_SPLIT_POS;

	if (_shellBrowser.get()) {
		split_pos = _split_pos;
		delete _shellBrowser.release();
	}

	 // create explorer treeview
	if (_create_info._open_mode & OWM_EXPLORE) {
		if (!_left_hwnd) {
			ClientRect rect(_hwnd);

			_left_hwnd = CreateWindowEx(0, WC_TREEVIEW, NULL,
							WS_CHILD|WS_TABSTOP|WS_VISIBLE|WS_CHILD|TVS_HASLINES|TVS_LINESATROOT|TVS_HASBUTTONS|TVS_SHOWSELALWAYS,//|TVS_NOTOOLTIPS
							0, rect.top, split_pos-SPLIT_WIDTH/2, rect.bottom-rect.top,
							_hwnd, (HMENU)IDC_FILETREE, g_Globals._hInstance, 0);
		}
	} else {
		if (_left_hwnd) {
			DestroyWindow(_left_hwnd);
			_left_hwnd = 0;
		}
	}

	_shellBrowser = auto_ptr<ShellBrowser>(new ShellBrowser(_hwnd, _left_hwnd, _right_hwnd,
												_shellpath_info, _himlSmall, this));

	_shellBrowser->Init(_hwnd);
}


String MDIShellBrowserChild::jump_to_int(LPCTSTR url)
{
	String dir, fname;

	if (!_tcsnicmp(url, TEXT("shell://"), 8)) {
		if (_shellBrowser->jump_to_pidl(ShellPath(url+8)))
			return url;
	}

	if (SplitFileSysURL(url, dir, fname)) {

		///@todo use fname

		if (_shellBrowser->jump_to_pidl(ShellPath(dir)))
			return FmtString(TEXT("file://%s"), (LPCTSTR)dir);
	}
	
	return String();
}


void MDIShellBrowserChild::entry_selected(Entry* entry)
{
	if (entry->_etype == ET_SHELL) {
		ShellEntry* shell_entry = static_cast<ShellEntry*>(entry);
		IShellFolder* folder;

		if (shell_entry->_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			folder = static_cast<ShellDirectory*>(shell_entry)->_folder;
		else
			folder = shell_entry->get_parent_folder();

		if (!folder) {
			assert(folder);
			return;
		}

		TCHAR path[MAX_PATH];

		if (shell_entry->get_path(path)) {
			String url;

			if (path[0] == ':')
				url.printf(TEXT("shell://%s"), path);
			else
				url.printf(TEXT("file://%s"), path);

			set_url(url);
		}

		_shellBrowser->UpdateFolderView(folder);

		 // set size of new created shell view windows
		ClientRect rt(_hwnd);
		resize_children(rt.right, rt.bottom);
	}
}

#endif
