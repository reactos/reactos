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


#include "../utility/utility.h"

#include "../explorer.h"
#include "../globals.h"

#include "../explorer_intres.h"


ShellBrowserChild::ShellBrowserChild(HWND hwnd, const ShellChildWndInfo& info)
 :	super(hwnd, info),
	_create_info(info)
{
	_pShellView = NULL;
	_pDropTarget = NULL;
	_himlSmall = 0;
	_last_sel = 0;
}

ShellBrowserChild::~ShellBrowserChild()
{
	if (_pShellView)
		_pShellView->Release();

	if (_pDropTarget) {
		_pDropTarget->Release();
		_pDropTarget = NULL;
	}
}


LRESULT ShellBrowserChild::Init(LPCREATESTRUCT pcs)
{
	CONTEXT("ShellBrowserChild::Init()");

	if (super::Init(pcs))
		return 1;

	_hWndFrame = GetParent(pcs->hwndParent);

	ClientRect rect(_hwnd);

	SHFILEINFO sfi;

	_himlSmall = (HIMAGELIST)SHGetFileInfo(TEXT("C:\\"), 0, &sfi, sizeof(SHFILEINFO), SHGFI_SYSICONINDEX|SHGFI_SMALLICON);
//	_himlLarge = (HIMAGELIST)SHGetFileInfo(TEXT("C:\\"), 0, &sfi, sizeof(SHFILEINFO), SHGFI_SYSICONINDEX|SHGFI_LARGEICON);


	const String& root_name = GetDesktopFolder().get_name(_create_info._root_shell_path, SHGDN_FORPARSING);

	_root._drive_type = DRIVE_UNKNOWN;
	lstrcpy(_root._volname, root_name);	// most of the time "Desktop"
	_root._fs_flags = 0;
	lstrcpy(_root._fs, TEXT("Desktop"));

//@@	_root._entry->read_tree(shell_info._root_shell_path.get_folder(), info._shell_path, SORT_NAME/*_sortOrder*/);

/*@todo
	we should call read_tree() here to iterate through the hierarchy and open all folders from shell_info._root_shell_path to shell_info._shell_path
	-> see FileChildWindow::FileChildWindow()
*/
	_root._entry = new ShellDirectory(GetDesktopFolder(), _create_info._root_shell_path, _hwnd);
	_root._entry->read_directory();

	/* already filled by ShellDirectory constructor
	lstrcpy(_root._entry->_data.cFileName, TEXT("Desktop")); */


	 // create explorer treeview
	if (_create_info._open_mode & OWM_EXPLORE)
		_left_hwnd = CreateWindowEx(0, WC_TREEVIEW, NULL,
						WS_CHILD|WS_TABSTOP|WS_VISIBLE|WS_CHILD|TVS_HASLINES|TVS_LINESATROOT|TVS_HASBUTTONS|TVS_SHOWSELALWAYS,//|TVS_NOTOOLTIPS
						0, rect.top, _split_pos-SPLIT_WIDTH/2, rect.bottom-rect.top,
						_hwnd, (HMENU)IDC_FILETREE, g_Globals._hInstance, 0);

	if (_left_hwnd) {
		InitializeTree();

		InitDragDrop();
	} else
		UpdateFolderView(_create_info._shell_path.get_folder());

	return 0;
}


void ShellBrowserChild::InitializeTree()
{
	CONTEXT("ShellBrowserChild::InitializeTree()");

	TreeView_SetImageList(_left_hwnd, _himlSmall, TVSIL_NORMAL);
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
		Tree_DoItemMenu(_left_hwnd, tvhti.hItem , &tvhti.pt);
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

	ShellEntry* entry = (ShellEntry*)pnmtv->itemNew.lParam;

	_last_sel = pnmtv->itemNew.hItem;

	if (entry->_etype == ET_SHELL) {
		IShellFolder* folder;

		if (entry->_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			folder = static_cast<ShellDirectory*>(entry)->_folder;
		else
			folder = entry->get_parent_folder();

		if (!folder) {
			assert(folder);
			return;
		}

		UpdateFolderView(folder);
	}
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

		ClientRect clnt(_hwnd);
		resize_children(clnt.right, clnt.bottom);
	}

	_pShellView->UIActivate(SVUIA_ACTIVATE_NOFOCUS);
}


LRESULT ShellBrowserChild::WndProc(UINT nmsg, WPARAM wparam, LPARAM lparam)
{
	switch(nmsg) {
	  case WM_GETISHELLBROWSER:	// for Registry Explorer Plugin
		return (LRESULT)static_cast<IShellBrowser*>(this);

	  case PM_GET_SHELLBROWSER_PTR:
		return (LRESULT)this;

	  case PM_DISPATCH_COMMAND: {
		switch(LOWORD(wparam)) {
		  case ID_WINDOW_NEW: {CONTEXT("ShellBrowserChild PM_DISPATCH_COMMAND ID_WINDOW_NEW");
			ShellBrowserChild::create(_create_info);
			break;}

		  case ID_BROWSE_BACK:
			break;//@todo

		  case ID_BROWSE_FORWARD:
			break;//@todo

		  case ID_BROWSE_UP:
			break;//@todo

		  default:
			return FALSE;
		}
		return TRUE;}
	
	  default:
		return super::WndProc(nmsg, wparam, lparam);
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
	  default:					return super::Notify(id, pnmh);
	}

	return 0;
}


HRESULT ShellBrowserChild::OnDefaultCommand(LPIDA pida)
{
	CONTEXT("ShellBrowserChild::OnDefaultCommand()");

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
							if (expand_folder(static_cast<ShellDirectory*>(entry)))
								return S_OK;
				}
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

//@@

}
