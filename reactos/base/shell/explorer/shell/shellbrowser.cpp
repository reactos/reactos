/*
 * Copyright 2003, 2004, 2005, 2006 Martin Fuchs
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


#include <precomp.h>

#include "../resource.h"


 // work around GCC's wide string constant bug
#ifdef __GNUC__
const LPCTSTR C_DRIVE = C_DRIVE_STR;
#endif


ShellBrowser::ShellBrowser(HWND hwnd, HWND hwndFrame, HWND left_hwnd, WindowHandle& right_hwnd, ShellPathInfo& create_info,
							BrowserCallback* cb, CtxMenuInterfaces& cm_ifs)
#ifndef __MINGW32__	// IShellFolderViewCB missing in MinGW (as of 25.09.2005)
 :	super(IID_IShellFolderViewCB),
#else
 :
#endif
	_hwnd(hwnd),
	_hwndFrame(hwndFrame),
	_left_hwnd(left_hwnd),
	_right_hwnd(right_hwnd),
	_create_info(create_info),
	_callback(cb),
	_cm_ifs(cm_ifs)
{
	_pShellView = NULL;
	_pDropTarget = NULL;
	_last_sel = 0;

	_cur_dir = NULL;

	_himl = ImageList_Create(GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), ILC_MASK|ILC_COLOR24, 2, 0);
	ImageList_SetBkColor(_himl, GetSysColor(COLOR_WINDOW));
}

ShellBrowser::~ShellBrowser()
{
	(void)TreeView_SetImageList(_left_hwnd, _himl_old, TVSIL_NORMAL);
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


void ShellBrowser::Init()
{
	CONTEXT("ShellBrowser::Init()");

	const String& root_name = GetDesktopFolder().get_name(_create_info._root_shell_path, SHGDN_FORADDRESSBAR);

	_root._drive_type = DRIVE_UNKNOWN;
	lstrcpy(_root._volname, root_name);
	_root._fs_flags = 0;
	lstrcpy(_root._fs, TEXT("Desktop"));

	_root._entry = new ShellDirectory(GetDesktopFolder(), _create_info._root_shell_path, _hwnd);

	_root._entry->read_directory(SCAN_DONT_ACCESS|SCAN_NO_FILESYSTEM);	// avoid to handle desktop root folder as file system directory

	if (_left_hwnd) {
		InitializeTree();
		InitDragDrop();
	}

	jump_to(_create_info._shell_path);

	/* already filled by ShellDirectory constructor
	lstrcpy(_root._entry->_data.cFileName, TEXT("Desktop")); */
}

void ShellBrowser::jump_to(LPCITEMIDLIST pidl)
{
	Entry* entry = NULL;

	if (!_cur_dir)
		_cur_dir = static_cast<ShellDirectory*>(_root._entry);

	//LOG(FmtString(TEXT("ShellBrowser::jump_to(): pidl=%s"), (LPCTSTR)FileSysShellPath(pidl)));

	 // We could call read_tree() here to iterate through the hierarchy and open all folders
	 // from _create_info._root_shell_path (_cur_dir) to _create_info._shell_path (pidl).
	 // To make it easier we just use ILFindChild() instead.
	if (_cur_dir) {
		static DynamicFct<LPITEMIDLIST(WINAPI*)(LPCITEMIDLIST, LPCITEMIDLIST)> ILFindChild(TEXT("SHELL32"), 24);

		if (ILFindChild) {
			for(;;) {
				LPCITEMIDLIST child_pidl = (*ILFindChild)(_cur_dir->create_absolute_pidl(), pidl);
				if (!child_pidl || !child_pidl->mkid.cb)
					break;

				_cur_dir->smart_scan(SORT_NAME, SCAN_DONT_ACCESS);

				entry = _cur_dir->find_entry(child_pidl);
				if (!entry)
					break;

				_cur_dir = static_cast<ShellDirectory*>(entry);
				_callback->entry_selected(entry);
			}
		} else {
			_cur_dir->smart_scan(SORT_NAME, SCAN_DONT_ACCESS);

			entry = _cur_dir->find_entry(pidl);	// This is not correct in the common case, but works on the desktop level.

			if (entry) {
				_cur_dir = static_cast<ShellDirectory*>(entry);
				_callback->entry_selected(entry);
			}
		}
	}

	 // If not already called, now directly call UpdateFolderView() using pidl
	if (!entry)
		UpdateFolderView(ShellFolder(pidl));
}


void ShellBrowser::InitializeTree()
{
	CONTEXT("ShellBrowserChild::InitializeTree()");

	_himl_old = TreeView_SetImageList(_left_hwnd, _himl, TVSIL_NORMAL);
	(void)TreeView_SetScrollTime(_left_hwnd, 100);

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
	(void)TreeView_SelectItem(_left_hwnd, hItem);
	(void)TreeView_Expand(_left_hwnd, hItem, TVE_EXPAND);
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


void ShellBrowser::OnTreeGetDispInfo(int idCtrl, LPNMHDR pnmh)
{
	CONTEXT("ShellBrowser::OnTreeGetDispInfo()");

	LPNMTVDISPINFO lpdi = (LPNMTVDISPINFO)pnmh;
	ShellEntry* entry = (ShellEntry*)lpdi->item.lParam;

	if (entry) {
		if (lpdi->item.mask & TVIF_TEXT)
			lpdi->item.pszText = entry->_display_name;

		if (lpdi->item.mask & (TVIF_IMAGE|TVIF_SELECTEDIMAGE)) {
			if (lpdi->item.mask & TVIF_IMAGE)
				lpdi->item.iImage = get_image_idx(
						entry->safe_extract_icon((ICONCACHE_FLAGS)(ICF_HICON|ICF_OVERLAYS)));

			if (lpdi->item.mask & TVIF_SELECTEDIMAGE)
				lpdi->item.iSelectedImage = get_image_idx(
						entry->safe_extract_icon((ICONCACHE_FLAGS)(ICF_HICON|ICF_OVERLAYS|ICF_OPEN)));
		}
	}
}

int ShellBrowser::get_image_idx(int icon_id)
{
	if (icon_id != ICID_NONE) {
		map<int,int>::const_iterator found = _image_map.find(icon_id);

		if (found != _image_map.end())
			return found->second;

		int idx = ImageList_AddIcon(_himl, g_Globals._icon_cache.get_icon(icon_id).get_hicon());

		_image_map[icon_id] = idx;

		return idx;
	} else
		return -1;
}

void ShellBrowser::invalidate_cache()
{
	(void)TreeView_SetImageList(_left_hwnd, _himl_old, TVSIL_NORMAL);
	ImageList_Destroy(_himl);

	_himl = ImageList_Create(GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), ILC_MASK|ILC_COLOR24, 2, 0);
	ImageList_SetBkColor(_himl, GetSysColor(COLOR_WINDOW));

	_himl_old = TreeView_SetImageList(_left_hwnd, _himl, TVSIL_NORMAL);

	for(map<int,int>::const_iterator it=_image_map.begin(); it!=_image_map.end(); ++it)
		g_Globals._icon_cache.free_icon(it->first);

	_image_map.clear();
}


void ShellBrowser::OnTreeItemExpanding(int idCtrl, LPNMTREEVIEW pnmtv)
{
	CONTEXT("ShellBrowser::OnTreeItemExpanding()");

	if (pnmtv->action == TVE_COLLAPSE)
        (void)TreeView_Expand(_left_hwnd, pnmtv->itemNew.hItem, TVE_COLLAPSE|TVE_COLLAPSERESET);
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

				(void)TreeView_SetItem(_left_hwnd, &tvItem);
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
		entry->smart_scan(SORT_NAME, SCAN_DONT_ACCESS);
	} catch(COMException& e) {
		HandleException(e, g_Globals._hMainWnd);
	}

	 // remove old children items
	HTREEITEM hchild, hnext;

	hnext = TreeView_GetChild(_left_hwnd, hParentItem);

	while((hchild=hnext) != 0) {
		hnext = TreeView_GetNextSibling(_left_hwnd, hchild);
		(void)TreeView_DeleteItem(_left_hwnd, hchild);
	}

	TV_ITEM tvItem;
	TV_INSERTSTRUCT tvInsert;

	for(entry=entry->_down; entry; entry=entry->_next) {
#ifndef _LEFT_FILES
		if (entry->_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
#endif
		{
			 // ignore hidden directories
			if (entry->_data.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN)
				continue;

			 // ignore directory entries "." and ".."
			if (entry->_data.cFileName[0]==TEXT('.') &&
				(entry->_data.cFileName[1]==TEXT('\0') ||
				(entry->_data.cFileName[1]==TEXT('.') && entry->_data.cFileName[2]==TEXT('\0'))))
				continue;

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

			(void)TreeView_InsertItem(_left_hwnd, &tvInsert);

			++cnt;
		}
	}

	SendMessage(_left_hwnd, WM_SETREDRAW, TRUE, 0);

	return cnt;
}


void ShellBrowser::OnTreeItemSelected(int idCtrl, LPNMTREEVIEW pnmtv)
{
	CONTEXT("ShellBrowser::OnTreeItemSelected()");

	Entry* entry = (Entry*)pnmtv->itemNew.lParam;

	_last_sel = pnmtv->itemNew.hItem;

	if (entry)
		_callback->entry_selected(entry);
}


void ShellBrowser::OnTreeItemRClick(int idCtrl, LPNMHDR pnmh)
{
	CONTEXT("ShellBrowser::OnTreeItemRClick()");

	TVHITTESTINFO tvhti;

	GetCursorPos(&tvhti.pt);
	ScreenToClient(_left_hwnd, &tvhti.pt);

	tvhti.flags = LVHT_NOWHERE;
	(void)TreeView_HitTest(_left_hwnd, &tvhti);

	if (TVHT_ONITEM & tvhti.flags) {
		LPARAM itemData = TreeView_GetItemData(_left_hwnd, tvhti.hItem);

		if (itemData) {
			Entry* entry = (Entry*)itemData;
			ClientToScreen(_left_hwnd, &tvhti.pt);

			HRESULT hr = entry->do_context_menu(_hwnd, tvhti.pt, _cm_ifs);

			if (SUCCEEDED(hr))
				refresh();
			else
				CHECKERROR(hr);
		}
	}
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

#ifndef __MINGW32__	// IShellFolderViewCB missing in MinGW (as of 25.09.2005)
	SFV_CREATE sfv_create;

	sfv_create.cbSize = sizeof(SFV_CREATE);
	sfv_create.pshf = folder;
	sfv_create.psvOuter = NULL;
	sfv_create.psfvcb = this;

	HRESULT hr = SHCreateShellFolderView(&sfv_create, &_pShellView);
#else
	HRESULT hr = folder->CreateViewObject(_hwnd, IID_IShellView, (void**)&_pShellView);
#endif

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


#ifndef __MINGW32__	// IShellFolderViewCB missing in MinGW (as of 25.09.2005)

 /// shell view callback
HRESULT STDMETHODCALLTYPE ShellBrowser::MessageSFVCB(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (uMsg == SFVM_INITMENUPOPUP) {
		///@todo never reached
		InsertMenu((HMENU)lParam, 0, MF_BYPOSITION, 12345, TEXT("TEST ENTRY"));
		return S_OK;
	}

	return E_NOTIMPL;
}

#endif


HRESULT ShellBrowser::OnDefaultCommand(LPIDA pida)
{
	CONTEXT("ShellBrowser::OnDefaultCommand()");

	if (pida->cidl >= 1) {
		if (_left_hwnd) {	// explorer mode
			if (_last_sel) {
				ShellDirectory* parent = (ShellDirectory*)TreeView_GetItemData(_left_hwnd, _last_sel);

				if (parent) {
					try {
						parent->smart_scan(SORT_NAME, SCAN_DONT_ACCESS);
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

					///@todo look for hidden or new subfolders and refresh/add new entry instead of opening a new window
					return E_NOTIMPL;
				}
			}
		} else { // no tree control
			if (MainFrameBase::OpenShellFolders(pida, _hwndFrame))
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
					(void)TreeView_Expand(_left_hwnd, hitem, TVE_EXPAND);

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

		entry->smart_scan(SORT_NAME, SCAN_DONT_ACCESS);

		Entry* found = entry->find_entry(p);
		p = entry->get_next_path_component(p);

		if (found)
			hitem = select_entry(hitem, found);

		entry = found;
	}

	return false;
}


bool ShellBrowser::select_folder(Entry* entry, bool expand)
{
	CONTEXT("ShellBrowser::expand_folder()");

	if (!_last_sel)
		return false;

	if (!TreeView_Expand(_left_hwnd, _last_sel, TVE_EXPAND))
		return false;

	for(HTREEITEM hitem=TreeView_GetChild(_left_hwnd,_last_sel); hitem; hitem=TreeView_GetNextSibling(_left_hwnd,hitem)) {
		if ((ShellDirectory*)TreeView_GetItemData(_left_hwnd,hitem) == entry) {
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


void ShellBrowser::refresh()
{
	///@todo
}


#ifndef _NO_MDI

MDIShellBrowserChild::MDIShellBrowserChild(HWND hwnd, const ShellChildWndInfo& info)
 :	super(hwnd, info),
	_create_info(info),
	_shellpath_info(info)	//@@ copies info -> no reference to _create_info !
{
/**todo Conversion of shell path into path string -> store into URL history
	const String& path = GetDesktopFolder().get_name(info._shell_path, SHGDN_FORADDRESSBAR);
	const String& parsingpath = GetDesktopFolder().get_name(info._shell_path, SHGDN_FORPARSING);

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

	update_shell_browser();

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
			///@todo refresh shell child
			_shellBrowser->invalidate_cache();
			break;

		  case ID_VIEW_SDI:
			MainFrameBase::Create(ExplorerCmd(_url, false));
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
							WS_CHILD|WS_TABSTOP|WS_VISIBLE|TVS_HASLINES|TVS_LINESATROOT|TVS_HASBUTTONS|TVS_SHOWSELALWAYS,//|TVS_NOTOOLTIPS
							0, rect.top, split_pos-SPLIT_WIDTH/2, rect.bottom-rect.top,
							_hwnd, (HMENU)IDC_FILETREE, g_Globals._hInstance, 0);
		}
	} else {
		if (_left_hwnd) {
			DestroyWindow(_left_hwnd);
			_left_hwnd = 0;
		}
	}

	_shellBrowser = auto_ptr<ShellBrowser>(new ShellBrowser(_hwnd, _hwndFrame, _left_hwnd, _right_hwnd,
												_shellpath_info, this, _cm_ifs));

	_shellBrowser->Init();
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
	if (_left_hwnd)
		_shellBrowser->select_folder(entry, false);

	_shellBrowser->UpdateFolderView(entry->get_shell_folder());

	 // set size of new created shell view windows
	ClientRect rt(_hwnd);
	resize_children(rt.right, rt.bottom);

	 // set new URL
	TCHAR path[MAX_PATH];

	if (entry->get_path(path, COUNTOF(path))) {
		String url;

		if (path[0] == ':')
			url.printf(TEXT("shell://%s"), path);
		else
			url.printf(TEXT("file://%s"), path);

		set_url(url);
	}
}

#endif
