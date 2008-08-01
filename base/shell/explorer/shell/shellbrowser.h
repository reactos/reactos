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
 // shellbrowser.h
 //
 // Martin Fuchs, 23.07.2003
 //

#include "../utility/treedroptarget.h"
#include "../utility/shellbrowserimpl.h"

#include <memory>

 /// information structure to hold current shell folder information
struct ShellPathInfo
{
	ShellPathInfo(int mode=0) : _open_mode(mode) {}

	ShellPathInfo(const ShellChildWndInfo& info)
	 :	_shell_path(info._shell_path),
		_root_shell_path(info._root_shell_path),
		_open_mode(info._open_mode)
	{
	}

	ShellPath	_shell_path;
	ShellPath	_root_shell_path;

	int			_open_mode;	//OPEN_WINDOW_MODE
};


struct BrowserCallback
{
	virtual ~BrowserCallback() {}
	virtual void entry_selected(Entry* entry) = 0;
};


 /// Implementation of IShellBrowserImpl interface in explorer child windows
struct ShellBrowser : public IShellBrowserImpl
#ifndef __MINGW32__	// IShellFolderViewCB missing in MinGW (as of 25.09.2005)
	,	public IComSrvBase<IShellFolderViewCB, ShellBrowser>, public SimpleComObject
#endif
{
	ShellBrowser(HWND hwnd, HWND hwndFrame, HWND left_hwnd, WindowHandle& right_hwnd, ShellPathInfo& create_info,
					BrowserCallback* cb, CtxMenuInterfaces& cm_ifs);
	virtual ~ShellBrowser();

	//IOleWindow
	virtual HRESULT STDMETHODCALLTYPE GetWindow(HWND* lphwnd)
	{
		*lphwnd = _hwnd;
		return S_OK;
	}

	//IShellBrowser
	virtual HRESULT STDMETHODCALLTYPE QueryActiveShellView(IShellView** ppshv)
	{
		_pShellView->AddRef();
		*ppshv = _pShellView;
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE GetControlWindow(UINT id, HWND* lphwnd)
	{
		if (!lphwnd)
			return E_POINTER;

		if (id == FCW_TREE) {
			*lphwnd = _left_hwnd;
			return S_OK;
		}

		HWND hwnd = (HWND)SendMessage(_hwndFrame, PM_GET_CONTROLWINDOW, id, 0);

		if (hwnd) {
			*lphwnd = hwnd;
			return S_OK;
		}

		return E_NOTIMPL;
	}

	virtual HRESULT STDMETHODCALLTYPE SendControlMsg(UINT id, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT* pret)
	{
		if (!pret)
			return E_POINTER;

		HWND hstatusbar = (HWND)SendMessage(_hwndFrame, PM_GET_CONTROLWINDOW, id, 0);

		if (hstatusbar) {
			*pret = ::SendMessage(hstatusbar, uMsg, wParam, lParam);
			return S_OK;
		}

		return E_NOTIMPL;
	}

	const Root& get_root() const {return _root;}

	void	OnTreeGetDispInfo(int idCtrl, LPNMHDR pnmh);
	void	OnTreeItemExpanding(int idCtrl, LPNMTREEVIEW pnmtv);
	void	OnTreeItemRClick(int idCtrl, LPNMHDR pnmh);
	void	OnTreeItemSelected(int idCtrl, LPNMTREEVIEW pnmtv);

	void	Init();

	int		InsertSubitems(HTREEITEM hParentItem, Entry* entry, IShellFolder* pParentFolder);

	bool	jump_to_pidl(LPCITEMIDLIST pidl);

	HRESULT OnDefaultCommand(LPIDA pida);

	void	UpdateFolderView(IShellFolder* folder);
	HTREEITEM select_entry(HTREEITEM hitem, Entry* entry, bool expand=true);

	bool	select_folder(Entry* entry, bool expand);

	 // for SDIMainFrame
	void	jump_to(LPCITEMIDLIST pidl);

	void	invalidate_cache();

protected:
	HWND			_hwnd;
	HWND			_hwndFrame;
	HWND			_left_hwnd;
	WindowHandle&	_right_hwnd;
	ShellPathInfo&	_create_info;
	HIMAGELIST		_himl;
	HIMAGELIST		_himl_old;
	BrowserCallback* _callback;

	ShellFolder		_folder;

	IShellView*		_pShellView;	// current hosted shellview
	TreeDropTarget*	_pDropTarget;

	HTREEITEM		_last_sel;

	Root			_root;
	ShellDirectory*	_cur_dir;

	CtxMenuInterfaces& _cm_ifs;

	void	InitializeTree();
	bool	InitDragDrop();

#ifndef __MINGW32__	// IShellFolderViewCB missing in MinGW (as of 25.09.2005)
	typedef IComSrvBase<IShellFolderViewCB, ShellBrowser> super;

	 // IShellFolderViewCB
	virtual HRESULT STDMETHODCALLTYPE MessageSFVCB(UINT uMsg, WPARAM wParam, LPARAM lParam);
#endif

	map<int, int> _image_map;

	int		get_image_idx(int icon_id);

	void	refresh();
};


#define	C_DRIVE_STR TEXT("C:\\")

 // work around GCC's wide string constant bug
#ifdef __GNUC__
extern const LPCTSTR C_DRIVE;
#else
#define	C_DRIVE C_DRIVE_STR
#endif

template<typename BASE> struct ShellBrowserChildT
 : public BASE, public BrowserCallback
{
	typedef BASE super;

	 // constructor for SDIMainFrame
	ShellBrowserChildT(HWND hwnd)
	 :	super(hwnd)
	{
	}

	 // constructor for MDIShellBrowserChild
	ShellBrowserChildT(HWND hwnd, const ShellChildWndInfo& info)
	 :	super(hwnd, info)
	{
	}

protected:
	auto_ptr<ShellBrowser> _shellBrowser;

	LRESULT WndProc(UINT nmsg, WPARAM wparam, LPARAM lparam)
	{
		switch(nmsg) {
		  case PM_GET_SHELLBROWSER_PTR:
			return (LRESULT)&*_shellBrowser;

		  case WM_GETISHELLBROWSER:	// for Registry Explorer Plugin
			return (LRESULT)static_cast<IShellBrowser*>(&*_shellBrowser);

		  default:
			return super::WndProc(nmsg, wparam, lparam);
		}

		return 0;
	}

	int Notify(int id, NMHDR* pnmh)
	{
		if (_shellBrowser.get())
			switch(pnmh->code) {
			  case TVN_GETDISPINFO:		_shellBrowser->OnTreeGetDispInfo(id, pnmh);					break;
			  case TVN_SELCHANGED:		_shellBrowser->OnTreeItemSelected(id, (LPNMTREEVIEW)pnmh);	break;
			  case TVN_ITEMEXPANDING:	_shellBrowser->OnTreeItemExpanding(id, (LPNMTREEVIEW)pnmh);	break;
			  case NM_RCLICK:			_shellBrowser->OnTreeItemRClick(id, pnmh);					break;
			  default:					return super::Notify(id, pnmh);
			}
		else
			return super::Notify(id, pnmh);

		return 0;
	}
};


#ifndef _NO_MDI

struct MDIShellBrowserChild : public ExtContextMenuHandlerT<
				ShellBrowserChildT<ChildWindow>
			>
{
	typedef ExtContextMenuHandlerT<
				ShellBrowserChildT<ChildWindow>
			> super;

	MDIShellBrowserChild(HWND hwnd, const ShellChildWndInfo& info);

	static MDIShellBrowserChild* create(const ShellChildWndInfo& info);

	LRESULT	Init(LPCREATESTRUCT);

	virtual String jump_to_int(LPCTSTR url);

protected:
	ShellChildWndInfo _create_info;
	ShellPathInfo	_shellpath_info;

	LRESULT	WndProc(UINT nmsg, WPARAM wparam, LPARAM lparam);

	void	update_shell_browser();

	 // interface BrowserCallback
	virtual void entry_selected(Entry* entry);
};

#endif
