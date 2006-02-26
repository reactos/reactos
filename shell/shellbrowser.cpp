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
 // Explorer clone, lean version
 //
 // shellbrowser.cpp
 //
 // Martin Fuchs, 23.07.2003
 //


#include "precomp.h"

#include "../resource.h"


static LPARAM TreeView_GetItemData(HWND hwndTreeView, HTREEITEM hItem)
{
	TVITEM tvItem;

	tvItem.mask = TVIF_PARAM;
	tvItem.hItem = hItem;

	if (!TreeView_GetItem(hwndTreeView, &tvItem))
		return 0;

	return tvItem.lParam;
}


ShellBrowserChild::ShellBrowserChild(HWND hwnd, HWND left_hwnd, WindowHandle& right_hwnd,
					ShellPathInfo& create_info, CtxMenuInterfaces& cm_ifs)
 :	_hwnd(hwnd),
	_hWndFrame(hwnd),
	_left_hwnd(left_hwnd),
	_right_hwnd(right_hwnd),
	_create_info(create_info),
	_cm_ifs(cm_ifs)
{
	_pShellView = NULL;
	_pDropTarget = NULL;
	_last_sel = 0;

	 // SDI integration
	_split_pos = DEFAULT_SPLIT_POS;
	_last_split = DEFAULT_SPLIT_POS;

	_cur_dir = NULL;

	_himl = ImageList_Create(GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), ILC_MASK|ILC_COLOR24, 2, 0);
	ImageList_SetBkColor(_himl, GetSysColor(COLOR_WINDOW));
}

ShellBrowserChild::~ShellBrowserChild()
{
	TreeView_SetImageList(_left_hwnd, 0, TVSIL_NORMAL);
	ImageList_Destroy(_himl);

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


LRESULT ShellBrowserChild::Init()
{
	CONTEXT("ShellBrowserChild::Init()");

	ClientRect rect(_hwnd);

	const String& root_name = GetDesktopFolder().get_name(_create_info._root_shell_path, SHGDN_FORADDRESSBAR);

	_root._drive_type = DRIVE_UNKNOWN;
	lstrcpy(_root._volname, root_name);
	_root._fs_flags = 0;
	lstrcpy(_root._fs, TEXT("Desktop"));

	_root._entry = new ShellDirectory(GetDesktopFolder(), _create_info._root_shell_path, _hwnd);

	 // -> set_curdir()
	_root._entry->read_directory();

	if (_left_hwnd) {
		InitializeTree();
		InitDragDrop();
	}

	jump_to(_create_info._shell_path);

	return 0;
}


void ShellBrowserChild::InitializeTree()
{
	CONTEXT("ShellBrowserChild::InitializeTree()");

	TreeView_SetImageList(_left_hwnd, _himl, TVSIL_NORMAL);
	TreeView_SetScrollTime(_left_hwnd, 100);

	TV_INSERTSTRUCT tvInsert;
	TV_ITEM& tvItem = tvInsert.item;

	tvInsert.hParent = 0;
	tvInsert.hInsertAfter = TVI_LAST;

	tvItem.mask = TVIF_PARAM | TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_CHILDREN;
	tvItem.lParam = (LPARAM)_root._entry;
	tvItem.pszText = _root._volname; //LPSTR_TEXTCALLBACK;
	tvItem.iImage = tvItem.iSelectedImage = I_IMAGECALLBACK;
	tvItem.cChildren = 1;

	HTREEITEM hItem = TreeView_InsertItem(_left_hwnd, &tvInsert);
	TreeView_SelectItem(_left_hwnd, hItem);
	TreeView_Expand(_left_hwnd, hItem, TVE_EXPAND);
}


bool ShellBrowserChild::InitDragDrop()
{
	CONTEXT("ShellBrowserChild::InitDragDrop()");

	_pDropTarget = new TreeDropTarget(_left_hwnd);

	if (!_pDropTarget)
		return false;

	_pDropTarget->AddRef();

	if (FAILED(RegisterDragDrop(_left_hwnd, _pDropTarget))) {//calls addref
		_pDropTarget->Release(); // free TreeDropTarget
		_pDropTarget = NULL;
		return false;
	} else
		_pDropTarget->Release();

	FORMATETC ftetc;

	ftetc.dwAspect = DVASPECT_CONTENT;
	ftetc.lindex = -1;
	ftetc.tymed = TYMED_HGLOBAL;
	ftetc.cfFormat = CF_HDROP;

	_pDropTarget->AddSuportedFormat(ftetc);

	return true;
}


void ShellBrowserChild::OnTreeItemRClick(int idCtrl, LPNMHDR pnmh)
{
	CONTEXT("ShellBrowserChild::OnTreeItemRClick()");

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

void ShellBrowserChild::Tree_DoItemMenu(HWND hwndTreeView, HTREEITEM hItem, LPPOINT pptScreen)
{
	CONTEXT("ShellBrowserChild::Tree_DoItemMenu()");

	LPARAM itemData = TreeView_GetItemData(hwndTreeView, hItem);

	if (itemData) {
		ShellEntry* entry = (ShellEntry*)itemData;

		ShellDirectory* dir = entry->_up;
		ShellFolder folder = dir? dir->_folder: GetDesktopFolder();
		LPCITEMIDLIST pidl = static_cast<ShellEntry*>(entry)->_pidl;

		CHECKERROR(ShellFolderContextMenu(folder, _hwnd, 1, &pidl, pptScreen->x, pptScreen->y, _cm_ifs));
	}
}

void ShellBrowserChild::OnTreeGetDispInfo(int idCtrl, LPNMHDR pnmh)
{
	CONTEXT("ShellBrowserChild::OnTreeGetDispInfo()");

	LPNMTVDISPINFO lpdi = (LPNMTVDISPINFO)pnmh;
	ShellEntry* entry = (ShellEntry*)lpdi->item.lParam;

	if (entry) {
		if (lpdi->item.mask & TVIF_TEXT)
			lpdi->item.pszText = entry->_display_name;

		if (lpdi->item.mask & (TVIF_IMAGE|TVIF_SELECTEDIMAGE))
			try {
				ShellPath pidl_abs = entry->create_absolute_pidl();	// Caching of absolute PIDLs could enhance performance.
				LPCITEMIDLIST pidl = pidl_abs;

				if (lpdi->item.mask & TVIF_IMAGE)
					lpdi->item.iImage = get_entry_image(entry, pidl, SHGFI_SMALLICON, _image_map);

				if (lpdi->item.mask & TVIF_SELECTEDIMAGE)
					lpdi->item.iSelectedImage = get_entry_image(entry, pidl, SHGFI_SMALLICON|SHGFI_OPENICON, _image_map_open);
			} catch(COMException&) {
				// ignore exception
			}
	}
}

int ShellBrowserChild::get_entry_image(ShellEntry* entry, LPCITEMIDLIST pidl, int shgfi_flags, ImageMap& cache)
{
	SHFILEINFO sfi;
	int idx = -1;

	ImageMap::const_iterator found = cache.find(entry);

	if (found != cache.end())
		return found->second;

	if (SHGetFileInfo((LPCTSTR)pidl, 0, &sfi, sizeof(sfi), shgfi_flags|SHGFI_PIDL|SHGFI_ICON|SHGFI_ADDOVERLAYS)) {
		idx = ImageList_AddIcon(_himl, sfi.hIcon);

		DestroyIcon(sfi.hIcon);
	}

	cache[entry] = idx;

	return idx;
}

void ShellBrowserChild::invalidate_cache()
{
	TreeView_SetImageList(_left_hwnd, 0, TVSIL_NORMAL);
	ImageList_Destroy(_himl);

	TreeView_SetImageList(_left_hwnd, _himl, TVSIL_NORMAL);
	_himl = ImageList_Create(GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), ILC_MASK|ILC_COLOR24, 2, 0);

	_image_map.clear();
	_image_map_open.clear();
}

void ShellBrowserChild::OnTreeItemExpanding(int idCtrl, LPNMTREEVIEW pnmtv)
{
	CONTEXT("ShellBrowserChild::OnTreeItemExpanding()");

	if (pnmtv->action == TVE_COLLAPSE)
        TreeView_Expand(_left_hwnd, pnmtv->itemNew.hItem, TVE_COLLAPSE|TVE_COLLAPSERESET);
    else if (pnmtv->action == TVE_EXPAND) {
		ShellDirectory* dir = (ShellDirectory*)TreeView_GetItemData(_left_hwnd, pnmtv->itemNew.hItem);

		if (dir)
			if (!InsertSubitems(pnmtv->itemNew.hItem, dir)) {
				dir->_shell_attribs &= ~SFGAO_HASSUBFOLDER;

				 // remove subitem "+"
				TV_ITEM tvItem;

				tvItem.mask = TVIF_CHILDREN;
				tvItem.hItem = pnmtv->itemNew.hItem;
				tvItem.cChildren = 0;

				TreeView_SetItem(_left_hwnd, &tvItem);
			}
	}
}

int ShellBrowserChild::InsertSubitems(HTREEITEM hParentItem, ShellDirectory* dir)
{
	CONTEXT("ShellBrowserChild::InsertSubitems()");

	WaitCursor wait;

	int cnt = 0;

	SendMessage(_left_hwnd, WM_SETREDRAW, FALSE, 0);

	try {
		dir->smart_scan();
	} catch(COMException& e) {
		HandleException(e, g_Globals._hMainWnd);
	}

	 // remove old children items
	for(HTREEITEM hchild,hnext=TreeView_GetChild(_left_hwnd, hParentItem); hchild=hnext; ) {
		hnext = TreeView_GetNextSibling(_left_hwnd, hchild);
		TreeView_DeleteItem(_left_hwnd, hchild);
	}

	TV_ITEM tvItem;
	TV_INSERTSTRUCT tvInsert;

	for(ShellEntry*entry=dir->_down; entry; entry=entry->_next) {
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

			++cnt;
		}
	}

	SendMessage(_left_hwnd, WM_SETREDRAW, TRUE, 0);

	return cnt;
}

void ShellBrowserChild::OnTreeItemSelected(int idCtrl, LPNMTREEVIEW pnmtv)
{
	CONTEXT("ShellBrowserChild::OnTreeItemSelected()");

	ShellDirectory* dir = (ShellDirectory*)pnmtv->itemNew.lParam;

	jump_to(dir);

	_last_sel = pnmtv->itemNew.hItem;
}

void ShellBrowserChild::UpdateFolderView(IShellFolder* folder)
{
	CONTEXT("ShellBrowserChild::UpdateFolderView()");

	FOLDERSETTINGS fs;
	IShellView* pLastShellView = _pShellView;

	_folder = folder;

	if (pLastShellView)
		pLastShellView->GetCurrentInfo(&fs);
	else {
		fs.ViewMode = _create_info._open_mode&OWM_DETAILS? FVM_DETAILS: FVM_ICON;
		fs.fFlags = FWF_BESTFITWINDOW;
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

		resize_children();
	}

	_pShellView->UIActivate(SVUIA_ACTIVATE_NOFOCUS);
}


void ShellBrowserChild::resize_children()
{
	RECT rect = _clnt_rect;

	HDWP hdwp = BeginDeferWindowPos(2);

	int cx = rect.left;

	if (_left_hwnd) {
		cx = _split_pos + SPLIT_WIDTH/2;

		hdwp = DeferWindowPos(hdwp, _left_hwnd, 0, rect.left, rect.top, _split_pos-SPLIT_WIDTH/2-rect.left, rect.bottom-rect.top, SWP_NOZORDER|SWP_NOACTIVATE);
	} else {
		_split_pos = 0;
		cx = 0;
	}

	if (_right_hwnd)
		hdwp = DeferWindowPos(hdwp, _right_hwnd, 0, rect.left+cx+1, rect.top, rect.right-cx, rect.bottom-rect.top, SWP_NOZORDER|SWP_NOACTIVATE);

	EndDeferWindowPos(hdwp);
}


bool ShellBrowserChild::WndProc(UINT nmsg, WPARAM wparam, LPARAM lparam, LRESULT& res)
{
	switch(nmsg) {
	  case WM_GETISHELLBROWSER:	// for Registry Explorer Plugin
		res = (LRESULT)static_cast<IShellBrowser*>(this);
		return true;


		// SDI integration:

	  case WM_PAINT: {
		PaintCanvas canvas(_hwnd);
		ClientRect rt(_hwnd);
		rt.left = _split_pos-SPLIT_WIDTH/2;
		rt.right = _split_pos+SPLIT_WIDTH/2+1;

		if (_right_hwnd) {
			WindowRect right_rect(_right_hwnd);
			ScreenToClient(_hwnd, &right_rect);
			rt.top = right_rect.top;
			rt.bottom = right_rect.bottom;
		}

		HBRUSH lastBrush = SelectBrush(canvas, GetStockBrush(COLOR_SPLITBAR));
		Rectangle(canvas, rt.left, rt.top-1, rt.right, rt.bottom+1);
		SelectObject(canvas, lastBrush);
		break;}

	  case WM_SETCURSOR:
		if (LOWORD(lparam) == HTCLIENT) {
			POINT pt;
			GetCursorPos(&pt);
			ScreenToClient(_hwnd, &pt);

			if (pt.x>=_split_pos-SPLIT_WIDTH/2 && pt.x<_split_pos+SPLIT_WIDTH/2+1) {
				SetCursor(LoadCursor(0, IDC_SIZEWE));
				res = TRUE;
				return true;
			}
		}
		goto def;

	  case WM_LBUTTONDOWN: {
		int x = GET_X_LPARAM(lparam);

		ClientRect rt(_hwnd);

		if (x>=_split_pos-SPLIT_WIDTH/2 && x<_split_pos+SPLIT_WIDTH/2+1) {
			_last_split = _split_pos;
			SetCapture(_hwnd);
		}

		break;}

	  case WM_LBUTTONUP:
		if (GetCapture() == _hwnd)
			ReleaseCapture();
		break;

	  case WM_KEYDOWN:
		if (wparam == VK_ESCAPE)
			if (GetCapture() == _hwnd) {
				_split_pos = _last_split;
				resize_children();
				_last_split = -1;
				ReleaseCapture();
				SetCursor(LoadCursor(0, IDC_ARROW));
			}
		break;

	  case WM_MOUSEMOVE:
		if (GetCapture() == _hwnd) {
			int x = LOWORD(lparam);

			ClientRect rt(_hwnd);

			if (x>=0 && x<rt.right) {
				_split_pos = x;
				resize_children();
				rt.left = x-SPLIT_WIDTH/2;
				rt.right = x+SPLIT_WIDTH/2+1;
				InvalidateRect(_hwnd, &rt, FALSE);
				UpdateWindow(_left_hwnd);
				UpdateWindow(_hwnd);
				UpdateWindow(_right_hwnd);
			}
		}
		break;


	  default: def:
		return false;
	}

	res = 0;
	return true;
}

int ShellBrowserChild::Command(int id, int code)
{
	switch(id) {
	  case ID_BROWSE_BACK:
		break;//@todo

	  case ID_BROWSE_FORWARD:
		break;//@todo

	  case ID_BROWSE_UP:
		if (_left_hwnd) {
			//@@ not necessary in this simply case: jump_to(_cur_dir->_up);

			 //@@ -> move into jump_to()
			HTREEITEM hitem = TreeView_GetParent(_left_hwnd, _last_sel);

			if (hitem)
				TreeView_SelectItem(_left_hwnd, hitem);	// sends TVN_SELCHANGED notification
		} else {
			if (_cur_dir->_up)
				jump_to(_cur_dir->_up);
		}
		break;

	  default:
		return 1;
	}

	return 0;
}

int ShellBrowserChild::Notify(int id, NMHDR* pnmh)
{
	switch(pnmh->code) {
	  case TVN_GETDISPINFO:		OnTreeGetDispInfo(id, pnmh);					break;
	  case TVN_ITEMEXPANDING:	OnTreeItemExpanding(id, (LPNMTREEVIEW)pnmh);	break;
	  case TVN_SELCHANGED:		OnTreeItemSelected(id, (LPNMTREEVIEW)pnmh);		break;
	  case NM_RCLICK:			OnTreeItemRClick(id, pnmh);						break;
	}

	return 0;
}


HRESULT ShellBrowserChild::OnDefaultCommand(LPIDA pida)
{
	CONTEXT("ShellBrowserChild::OnDefaultCommand()");

	if (pida->cidl >= 1) {
		if (_left_hwnd) {	// explorer mode
			ShellDirectory* parent = _cur_dir;

			if (parent) {
				try {
					parent->smart_scan();
				} catch(COMException& e) {
					return e.Error();
				}

				UINT firstOffset = pida->aoffset[1];
				LPITEMIDLIST pidl = (LPITEMIDLIST)((LPBYTE)pida+firstOffset);

				ShellEntry* entry = parent->find_entry(pidl);

				if (entry && (entry->_data.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY))
					if (select_folder(static_cast<ShellDirectory*>(entry), true))
						return S_OK;
			}
		} else { // no tree control
			if (MainFrame::OpenShellFolders(pida, _hWndFrame))
				return S_OK;

/* create new Frame Window
			if (MainFrame::OpenShellFolders(pida, 0))
				return S_OK;
*/
		}
	}

	return E_NOTIMPL;
}


bool ShellBrowserChild::select_folder(ShellDirectory* dir, bool expand)
{
	CONTEXT("ShellBrowserChild::expand_folder()");

	if (!_last_sel)
		return false;

	if (!TreeView_Expand(_left_hwnd, _last_sel, TVE_EXPAND))
		return false;

	for(HTREEITEM hitem=TreeView_GetChild(_left_hwnd,_last_sel); hitem; hitem=TreeView_GetNextSibling(_left_hwnd,hitem)) {
		if ((ShellDirectory*)TreeView_GetItemData(_left_hwnd,hitem) == dir) {
			if (TreeView_SelectItem(_left_hwnd, hitem)) {
				if (expand)
					if (!TreeView_Expand(_left_hwnd, hitem, TVE_EXPAND))
						return false;

				return true;
			}

			break;
		}
	}

	return false;
}


void ShellBrowserChild::jump_to(LPCTSTR path)
{
	///@todo implement "file://", ... parsing
	jump_to(ShellPath(path));
}


void ShellBrowserChild::jump_to(LPCITEMIDLIST pidl)
{
	ShellDirectory* dir = NULL;

	if (!_cur_dir)
		_cur_dir = _root._entry;

	if (_cur_dir) {
		static DynamicFct<LPITEMIDLIST(WINAPI*)(LPCITEMIDLIST, LPCITEMIDLIST)> ILFindChild(TEXT("SHELL32"), 24);

		if (ILFindChild) {
			for(;;) {
				LPCITEMIDLIST child_pidl = (*ILFindChild)(_cur_dir->create_absolute_pidl(), pidl);
				if (!child_pidl || !child_pidl->mkid.cb)
					break;

				_cur_dir->smart_scan();

				dir = static_cast<ShellDirectory*>(_cur_dir->find_entry(child_pidl));
				if (!dir)
					break;

				jump_to(dir);
			}
		} else {
			_cur_dir->smart_scan();

			dir = static_cast<ShellDirectory*>(_cur_dir->find_entry(pidl));	// This is not correct in the common case, but works on the desktop level.

			if (dir)
				jump_to(dir);
		}
	}

	 // If not already called, now directly call UpdateFolderView() using pidl.
	if (!dir)
		UpdateFolderView(ShellFolder(pidl));
}

void ShellBrowserChild::jump_to(ShellDirectory* dir)
{
	if (dir == _cur_dir)
		return;

	if (_left_hwnd)
		select_folder(dir, false);

	UpdateFolderView(dir->_folder);

	_cur_dir = dir;
}
