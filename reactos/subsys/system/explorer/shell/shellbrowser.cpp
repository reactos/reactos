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
 // shellbrowser.cpp
 //
 // Martin Fuchs, 23.07.2003
 //


#include "../utility/utility.h"
#include "../utility/shellclasses.h"

#include "../explorer.h"
#include "../globals.h"

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


ShellBrowserChild::ShellBrowserChild(HWND hwnd)
 :	super(hwnd)
{
	_hWndFrame = 0;
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


void ShellBrowserChild::OnCreate(LPCREATESTRUCT pcs)
{
	_hWndFrame = GetParent(pcs->hwndParent);

	RECT rect;
	GetClientRect(_hwnd, &rect);

	SHFILEINFO  sfi;

	_himlSmall = (HIMAGELIST)SHGetFileInfo(TEXT("C:\\"), 0, &sfi, sizeof(SHFILEINFO), SHGFI_SYSICONINDEX|SHGFI_SMALLICON);
//	_himlLarge = (HIMAGELIST)SHGetFileInfo(TEXT("C:\\"), 0, &sfi, sizeof(SHFILEINFO), SHGFI_SYSICONINDEX|SHGFI_LARGEICON);


	 // create explorer treeview
	_left_hwnd = CreateWindowEx(0, WC_TREEVIEW, NULL,
					WS_CHILD|WS_TABSTOP|WS_VISIBLE|WS_CHILD|TVS_HASLINES|TVS_LINESATROOT|TVS_HASBUTTONS|TVS_NOTOOLTIPS|TVS_SHOWSELALWAYS,
					0, rect.top, _split_pos-SPLIT_WIDTH/2, rect.bottom-rect.top,
					_hwnd, (HMENU)IDC_FILETREE, g_Globals._hInstance, 0);

	if (_left_hwnd) {
		InitializeTree(/*ShellChildWndInfo(TEXT("C:\\"),DesktopFolder())*/);	//@@ GetCurrentDirectory()

		InitDragDrop();
	}
}


void ShellBrowserChild::InitializeTree(/*const FileChildWndInfo& info*/)
{
	TreeView_SetImageList(_left_hwnd, _himlSmall, TVSIL_NORMAL);
	TreeView_SetScrollTime(_left_hwnd, 100);


	_root._drive_type = DRIVE_UNKNOWN;
	lstrcpy(_root._volname, TEXT("Desktop"));
	_root._fs_flags = 0;
	lstrcpy(_root._fs, TEXT("Shell"));


//@@	_root._entry->read_tree(ShellFolder(shell_info._root_shell_path), info._shell_path, SORT_NAME/*_sortOrder*/);

	//@@ fängt zunächst nur einmal mit dem Desktop-Objekt an
	_root._entry = new ShellDirectory(Desktop(), DesktopFolder(), _hwnd);
	_root._entry->read_directory();

	lstrcpy(_root._entry->_data.cFileName, TEXT("Desktop"));


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
	LPARAM itemData = TreeView_GetItemData(hwndTreeView, hItem);

	if (itemData) {
		HWND hwndParent = ::GetParent(hwndTreeView);
		Entry* entry = (Entry*)itemData;

		IShellFolder* folder;

		if (entry->_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			folder = static_cast<ShellDirectory*>(entry)->_folder;
		else
			folder = entry->_up? static_cast<ShellDirectory*>(entry->_up)->_folder: Desktop();

		folder->AddRef();

		if (folder) {
			LPCITEMIDLIST pidl = static_cast<ShellEntry*>(entry)->_pidl;

			IContextMenu* pcm;
			HRESULT hr = folder->GetUIObjectOf(hwndParent, 1, &pidl, IID_IContextMenu, NULL, (LPVOID*)&pcm);

			if (SUCCEEDED(hr)) {
				HMENU hPopup = CreatePopupMenu();

				if (hPopup) {
					hr = pcm->QueryContextMenu(hPopup, 0, 1, 0x7fff, CMF_NORMAL|CMF_EXPLORE);

					if (SUCCEEDED(hr)) {
					   IContextMenu2* pcm2;

					   pcm->QueryInterface(IID_IContextMenu2, (LPVOID*)&pcm2);

					   UINT idCmd = TrackPopupMenu(hPopup,
											   TPM_LEFTALIGN | TPM_RETURNCMD | TPM_RIGHTBUTTON,
											   pptScreen->x,
											   pptScreen->y,
											   0,
											   hwndParent,
											   NULL);

						if (pcm2) {
						  pcm2->Release();
						  pcm2 = NULL;
						}

						if (idCmd) {
						  CMINVOKECOMMANDINFO  cmi;
						  cmi.cbSize = sizeof(CMINVOKECOMMANDINFO);
						  cmi.fMask = 0;
						  cmi.hwnd = hwndParent;
						  cmi.lpVerb = (LPCSTR)(INT_PTR)(idCmd - 1);
						  cmi.lpParameters = NULL;
						  cmi.lpDirectory = NULL;
						  cmi.nShow = SW_SHOWNORMAL;
						  cmi.dwHotKey = 0;
						  cmi.hIcon = NULL;
						  hr = pcm->InvokeCommand(&cmi);
						}
					}
				}

				pcm->Release();
			}

			folder->Release();
		}
	}
}

void ShellBrowserChild::OnTreeGetDispInfo(int idCtrl, LPNMHDR pnmh)
{
	LPNMTVDISPINFO lpdi = (LPNMTVDISPINFO)pnmh;
	ShellEntry* entry = (ShellEntry*)lpdi->item.lParam;

	if (lpdi->item.mask & TVIF_TEXT) {
		/* if (SHGetFileInfo((LPCTSTR)&*entry->_pidl, 0, &sfi, sizeof(sfi), SHGFI_PIDL|SHGFI_DISPLAYNAME))
			lstrcpy(lpdi->item.pszText, sfi.szDisplayName); */
		lstrcpy(lpdi->item.pszText, entry->_data.cFileName);
	}

	if (lpdi->item.mask & (TVIF_IMAGE|TVIF_SELECTEDIMAGE)) {
		LPITEMIDLIST pidl = entry->create_absolute_pidl(_hwnd);
		SHFILEINFO sfi;

		if (lpdi->item.mask & TVIF_IMAGE) {
			if (SHGetFileInfo((LPCTSTR)pidl, 0, &sfi, sizeof(sfi), SHGFI_PIDL|SHGFI_SYSICONINDEX|SHGFI_SMALLICON|SHGFI_LINKOVERLAY))
				lpdi->item.iImage = sfi.iIcon;
		}

		if (lpdi->item.mask & TVIF_SELECTEDIMAGE) {
			if (SHGetFileInfo((LPCTSTR)pidl, 0, &sfi, sizeof(sfi), SHGFI_PIDL|SHGFI_SYSICONINDEX|SHGFI_SMALLICON|SHGFI_OPENICON))
				lpdi->item.iSelectedImage = sfi.iIcon;
		}

		if (pidl != &*entry->_pidl)
			ShellMalloc()->Free(pidl);
	}
}

void ShellBrowserChild::OnTreeItemExpanding(int idCtrl, LPNMTREEVIEW pnmtv)
{
	if (pnmtv->action == TVE_COLLAPSE)
        TreeView_Expand(_left_hwnd, pnmtv->itemNew.hItem, TVE_COLLAPSE|TVE_COLLAPSERESET);
    else if (pnmtv->action == TVE_EXPAND) {
		ShellDirectory* entry = (ShellDirectory*)TreeView_GetItemData(_left_hwnd, pnmtv->itemNew.hItem);

		if (entry)
			InsertSubitems(pnmtv->itemNew.hItem, entry, entry->_folder);
	}
}

void ShellBrowserChild::InsertSubitems(HTREEITEM hParentItem, Entry* entry, IShellFolder* pParentFolder)
{
	WaitCursor wait;

	SendMessage(_left_hwnd, WM_SETREDRAW, FALSE, 0);

	entry->smart_scan();

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
			tvItem.lParam= (LPARAM)entry;
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
	}

	SendMessage(_left_hwnd, WM_SETREDRAW, TRUE, 0);
}

void ShellBrowserChild::OnTreeItemSelected(int idCtrl, LPNMTREEVIEW pnmtv)
{
	ShellEntry* entry = (ShellEntry*)pnmtv->itemNew.lParam;

	_last_sel = pnmtv->itemNew.hItem;

	IShellFolder* folder;

	if (entry->_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		folder = static_cast<ShellDirectory*>(entry)->_folder;
	else
		folder = entry->_up? static_cast<ShellDirectory*>(entry->_up)->_folder: Desktop();

	if (!folder) {
		assert(folder);
		return;
	}

	FOLDERSETTINGS fs;
	IShellView* pLastShellView = _pShellView;

	if (pLastShellView)
		pLastShellView->GetCurrentInfo(&fs);
	else {
		fs.ViewMode = FVM_DETAILS;
		fs.fFlags = FWF_NOCLIENTEDGE;
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

		RECT clnt;
		GetClientRect(_hwnd, &clnt);
		resize_children(clnt.right, clnt.bottom);
	}

	_pShellView->UIActivate(SVUIA_ACTIVATE_NOFOCUS);
}


LRESULT ShellBrowserChild::WndProc(UINT nmsg, WPARAM wparam, LPARAM lparam)
{
	switch(nmsg) {
	  case WM_GETISHELLBROWSER:	// for Registry Explorer Plugin
		return (LRESULT)static_cast<IShellBrowser*>(this);

	  case WM_CREATE:
		OnCreate((LPCREATESTRUCT)lparam);
		goto def;

	  default: def:
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


 // process default command: look for folders and traverse into them
HRESULT ShellBrowserChild::OnDefaultCommand(IShellView* ppshv)
{
	static UINT CF_IDLIST = RegisterClipboardFormat(CFSTR_SHELLIDLIST);

	HRESULT ret = E_NOTIMPL;

	IDataObject* selection;
	HRESULT hr = ppshv->GetItemObject(SVGIO_SELECTION, IID_IDataObject, (void**)&selection);
	if (FAILED(hr))
		return hr;


    FORMATETC fetc;
    fetc.cfFormat = CF_IDLIST;
    fetc.ptd = NULL;
    fetc.dwAspect = DVASPECT_CONTENT;
    fetc.lindex = -1;
    fetc.tymed = TYMED_HGLOBAL;

    hr = selection->QueryGetData(&fetc);
	if (FAILED(hr))
		return hr;


	STGMEDIUM stgm = {sizeof(STGMEDIUM), {0}, 0};

    hr = selection->GetData(&fetc, &stgm);
	if (FAILED(hr))
		return hr;


    DWORD pData = (DWORD)GlobalLock(stgm.hGlobal);
	CIDA* pIDList = (CIDA*)pData;

	if (pIDList->cidl >= 1) {
		//UINT folderOffset = pIDList->aoffset[0];
		//LPITEMIDLIST folder = (LPITEMIDLIST)(pData+folderOffset);

		UINT firstOffset = pIDList->aoffset[1];
		LPITEMIDLIST pidl = (LPITEMIDLIST)(pData+firstOffset);

		//HTREEITEM hitem_sel = TreeView_GetSelection(_left_hwnd);

		if (_last_sel) {
			ShellDirectory* parent = (ShellDirectory*)TreeView_GetItemData(_left_hwnd, _last_sel);

			if (parent) {
				parent->smart_scan();

				Entry* entry = parent->find_entry(pidl);

				if (entry && (entry->_data.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY))
					if (expand_folder(static_cast<ShellDirectory*>(entry)))
						ret = S_OK;
			}
		}
	}

	GlobalUnlock(stgm.hGlobal);
    ReleaseStgMedium(&stgm);

	selection->Release();

	return ret;
}

bool ShellBrowserChild::expand_folder(ShellDirectory* entry)
{
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
