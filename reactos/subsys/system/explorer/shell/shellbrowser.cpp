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
 // Explorer clone, lean version
 //
 // shellbrowser.cpp
 //
 // Martin Fuchs, 23.07.2003
 //


#include "precomp.h"

#include "../explorer_intres.h"


static LPARAM TreeView_GetItemData(HWND hwndTreeView, HTREEITEM hItem)
{
	TVITEM tvItem;

	tvItem.mask = TVIF_PARAM;
	tvItem.hItem = hItem;

	if (!TreeView_GetItem(hwndTreeView, &tvItem))
		return 0;

	return tvItem.lParam;
}


ShellBrowserChild::ShellBrowserChild(HWND hwnd, HWND left_hwnd, WindowHandle& right_hwnd, ShellPathInfo& create_info)
 :	_hwnd(hwnd),
	_left_hwnd(left_hwnd),
	_right_hwnd(right_hwnd),
	_create_info(create_info)
{
	_pShellView = NULL;
	_pDropTarget = NULL;
	_himlSmall = 0;
	_last_sel = 0;

	 // SDI integration
	_split_pos = DEFAULT_SPLIT_POS;
	_last_split = DEFAULT_SPLIT_POS;

	_cur_dir = NULL;

	Init(hwnd);
}

ShellBrowserChild::~ShellBrowserChild()
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


LRESULT ShellBrowserChild::Init(HWND hWndFrame)
{
	CONTEXT("ShellBrowserChild::Init()");

	_hWndFrame = hWndFrame;

	ClientRect rect(_hwnd);

	SHFILEINFO sfi;

	_himlSmall = (HIMAGELIST)SHGetFileInfo(TEXT("C:\\"), 0, &sfi, sizeof(SHFILEINFO), SHGFI_SYSICONINDEX|SHGFI_SMALLICON);
//	_himlLarge = (HIMAGELIST)SHGetFileInfo(TEXT("C:\\"), 0, &sfi, sizeof(SHFILEINFO), SHGFI_SYSICONINDEX|SHGFI_LARGEICON);

	if (_left_hwnd) {
		InitializeTree();
		InitDragDrop();
	}

	const String& root_name = GetDesktopFolder().get_name(_create_info._root_shell_path, SHGDN_FORPARSING);

	_root._drive_type = DRIVE_UNKNOWN;
	lstrcpy(_root._volname, root_name);
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


void ShellBrowserChild::InitializeTree()
{
	CONTEXT("ShellBrowserChild::InitializeTree()");

	TreeView_SetImageList(_left_hwnd, _himlSmall, TVSIL_NORMAL);
	TreeView_SetScrollTime(_left_hwnd, 100);

	TV_ITEM tvItem;

	tvItem.mask = TVIF_PARAM | TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_CHILDREN;
	tvItem.lParam = (LPARAM)_root._entry;
	tvItem.pszText = LPSTR_TEXTCALLBACK;
	tvItem.iImage = tvItem.iSelectedImage = I_IMAGECALLBACK;
	tvItem.cChildren = 1;

	TV_INSERTSTRUCT tvInsert;

	tvInsert.hParent = 0;
	tvInsert.hInsertAfter = TVI_LAST;
	tvInsert.item = tvItem;

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

void ShellBrowserChild::OnTreeGetDispInfo(int idCtrl, LPNMHDR pnmh)
{
	CONTEXT("ShellBrowserChild::OnTreeGetDispInfo()");

	LPNMTVDISPINFO lpdi = (LPNMTVDISPINFO)pnmh;
	ShellEntry* entry = (ShellEntry*)lpdi->item.lParam;

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
			if ((HIMAGELIST)SHGetFileInfo((LPCTSTR)pidl, 0, &sfi, sizeof(sfi), SHGFI_PIDL|SHGFI_SYSICONINDEX|SHGFI_SMALLICON|SHGFI_LINKOVERLAY) == _himlSmall)
				lpdi->item.iImage = sfi.iIcon;
			else
				lpdi->item.iImage = -1;

		if (lpdi->item.mask & TVIF_SELECTEDIMAGE)
			if ((HIMAGELIST)SHGetFileInfo((LPCTSTR)pidl, 0, &sfi, sizeof(sfi), SHGFI_PIDL|SHGFI_SYSICONINDEX|SHGFI_SMALLICON|SHGFI_OPENICON) == _himlSmall)
				lpdi->item.iSelectedImage = sfi.iIcon;
			else
				lpdi->item.iSelectedImage = -1;
	}
}

void ShellBrowserChild::OnTreeItemExpanding(int idCtrl, LPNMTREEVIEW pnmtv)
{
	CONTEXT("ShellBrowserChild::OnTreeItemExpanding()");

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

int ShellBrowserChild::InsertSubitems(HTREEITEM hParentItem, Entry* entry, IShellFolder* pParentFolder)
{
	CONTEXT("ShellBrowserChild::InsertSubitems()");

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

void ShellBrowserChild::OnTreeItemSelected(int idCtrl, LPNMTREEVIEW pnmtv)
{
	CONTEXT("ShellBrowserChild::OnTreeItemSelected()");

	_last_sel = pnmtv->itemNew.hItem;
	Entry* entry = (Entry*)pnmtv->itemNew.lParam;

	jump_to(entry);
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


LRESULT ShellBrowserChild::WndProc(UINT nmsg, WPARAM wparam, LPARAM lparam)
{
	switch(nmsg) {
	  case WM_GETISHELLBROWSER:	// for Registry Explorer Plugin
		return (LRESULT)static_cast<IShellBrowser*>(this);


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
				return TRUE;
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
		return DefWindowProc(_hwnd, nmsg, wparam, lparam);
	}

	return 0;
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
			//@@if (_last_sel) {
				ShellDirectory* parent = _cur_dir;//@@(ShellDirectory*)TreeView_GetItemData(_left_hwnd, _last_sel);

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
							if (expand_folder(static_cast<ShellDirectory*>(entry)))
								return S_OK;
				}
			//@@}
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


bool ShellBrowserChild::expand_folder(ShellDirectory* entry)
{
	CONTEXT("ShellBrowserChild::expand_folder()");

	//HTREEITEM hitem_sel = TreeView_GetSelection(_left_hwnd);
	if (!_last_sel)
		return false;

	if (!TreeView_Expand(_left_hwnd, _last_sel, TVE_EXPAND))
		return false;

	for(HTREEITEM hitem=TreeView_GetChild(_left_hwnd,_last_sel); hitem; hitem=TreeView_GetNextSibling(_left_hwnd,hitem)) {
		if ((ShellDirectory*)TreeView_GetItemData(_left_hwnd,hitem) == entry) {
			if (TreeView_SelectItem(_left_hwnd, hitem) &&
				TreeView_Expand(_left_hwnd, hitem, TVE_EXPAND))
				return true;

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
				jump_to(entry);
		}
	}

		//@@ work around as long as we don't iterate correctly through the ShellEntry tree
	if (!entry) {
		UpdateFolderView(ShellFolder(pidl));
	}
}

void ShellBrowserChild::jump_to(Entry* entry)
{
	if (entry->_etype == ET_SHELL) {
		IShellFolder* folder;
		ShellDirectory* se = static_cast<ShellDirectory*>(entry);

		if (se->_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			folder = static_cast<ShellDirectory*>(se)->_folder;
		else
			folder = se->get_parent_folder();

		if (!folder) {
			assert(folder);
			return;
		}

		UpdateFolderView(folder);

		_cur_dir = se;
	}
}
