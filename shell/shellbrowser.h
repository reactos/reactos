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
 // shellbrowser.h
 //
 // Martin Fuchs, 23.07.2003
 //

#include "../utility/treedroptarget.h"
#include "../utility/shellbrowserimpl.h"


 /// information structure to hold current shell folder information
struct ShellPathInfo
{
	ShellPathInfo(int mode=0) : _open_mode(mode) {}

	ShellPath	_shell_path;
	ShellPath	_root_shell_path;

	int			_open_mode;	//OPEN_WINDOW_MODE
};


#ifndef SHGFI_ADDOVERLAYS // missing in MinGW (as of 28.12.2005)
#define SHGFI_ADDOVERLAYS 0x000000020
#endif

 /// Implementation of IShellBrowserImpl interface in explorer child windows
struct ShellBrowserChild : public IShellBrowserImpl
{
	ShellBrowserChild(HWND hwnd, HWND left_hwnd, WindowHandle& right_hwnd,
						ShellPathInfo& create_info, CtxMenuInterfaces& cm_ifs);
	virtual ~ShellBrowserChild();

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

		HWND hwnd = (HWND)SendMessage(_hWndFrame, PM_GET_CONTROLWINDOW, id, 0);

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

		HWND hstatusbar = (HWND)SendMessage(_hWndFrame, PM_GET_CONTROLWINDOW, id, 0);

		if (hstatusbar) {
			*pret = ::SendMessage(hstatusbar, uMsg, wParam, lParam);
			return S_OK;
		}

		return E_NOTIMPL;
	}

protected:
	HWND	_hwnd;
	HWND	_hWndFrame;
	HWND	_left_hwnd;
	WindowHandle& _right_hwnd;
	ShellPathInfo& _create_info;

	Root	_root;

	ShellFolder	_folder;

	IShellView*	_pShellView;	// current hosted shellview
	TreeDropTarget* _pDropTarget;

	HIMAGELIST	_himl;
	HTREEITEM	_last_sel;

public:
	bool	WndProc(UINT nmsg, WPARAM wparam, LPARAM lparam, LRESULT& res);
	int		Command(int id, int code);
	int 	Notify(int id, NMHDR* pnmh);

	LRESULT	Init();
	void	InitializeTree();
	int		InsertSubitems(HTREEITEM hParentItem, ShellDirectory* dir);
	bool	InitDragDrop();

	HRESULT OnDefaultCommand(LPIDA pida);

	void	OnTreeGetDispInfo(int idCtrl, LPNMHDR pnmh);
	void	OnTreeItemExpanding(int idCtrl, LPNMTREEVIEW pnmtv);
	void	OnTreeItemRClick(int idCtrl, LPNMHDR pnmh);
	void	OnTreeItemSelected(int idCtrl, LPNMTREEVIEW pnmtv);

	void	UpdateFolderView(IShellFolder* folder);
	void	Tree_DoItemMenu(HWND hwndTreeView, HTREEITEM hItem, LPPOINT pptScreen);
	bool	select_folder(ShellDirectory* dir, bool expand);

	// SDI integration
public:
	int 	_split_pos;
	int		_last_split;
	RECT	_clnt_rect;

	void	resize_children();
	void	jump_to(LPCTSTR path);
	void	jump_to(LPCITEMIDLIST pidl);

	void	jump_to(ShellDirectory* dir);

protected:
	ShellDirectory*	_cur_dir;
	CtxMenuInterfaces& _cm_ifs;

	typedef map<ShellEntry*, int> ImageMap;
	ImageMap _image_map;
	ImageMap _image_map_open;

	int		get_entry_image(ShellEntry* entry, LPCITEMIDLIST pidl, int shgfi_flags, ImageMap& cache);
	void	invalidate_cache();
};
