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
 // shellbrowser.h
 //
 // Martin Fuchs, 23.07.2003
 //

#include "../utility/treedroptarget.h"
#include "../utility/shellbrowserimpl.h"


struct ShellBrowserChild : public ChildWindow, public IShellBrowserImpl
{
	typedef ChildWindow super;

	ShellBrowserChild(HWND hwnd);
	~ShellBrowserChild();

	static ShellBrowserChild* create(HWND hmdiclient, const FileChildWndInfo& info)
	{
#ifndef _NO_MDI
		ChildWindow* child = ChildWindow::create(hmdiclient, info._pos.rcNormalPosition, WINDOW_CREATOR(ShellBrowserChild), CLASSNAME_CHILDWND);
#else
		//TODO: SDI implementation
#endif

		ShowWindow(child->_hwnd, info._pos.showCmd);

		return static_cast<ShellBrowserChild*>(child);
	}

	//IOleWindow
	STDMETHOD(GetWindow)(HWND* lphwnd)
	{
		*lphwnd = _hwnd;
		return S_OK;
	}

	//IShellBrowser
	STDMETHOD(QueryActiveShellView)(struct IShellView ** ppshv)
	{
		_pShellView->AddRef();
		*ppshv = _pShellView;
		return S_OK;
	}

	STDMETHOD(GetControlWindow)(UINT id, HWND * lphwnd)
	{
		if (!lphwnd)
			return E_POINTER;

		if (id == FCW_TREE) {
			*lphwnd = _left_hwnd;
			return S_OK;
		}

		HWND hwnd = (HWND)SendMessage(_hWndFrame, WM_GET_CONTROLWINDOW, id, 0);

		if (hwnd) {
			*lphwnd = hwnd;
			return S_OK;
		}

		return E_NOTIMPL;
	}

	STDMETHOD(SendControlMsg)(UINT id, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *pret)
	{
		if (!pret)
			return E_POINTER;

		HWND hstatusbar = (HWND)SendMessage(_hWndFrame, WM_GET_CONTROLWINDOW, id, 0);

		if (hstatusbar) {
			*pret = ::SendMessage(hstatusbar, uMsg, wParam, lParam);
			return S_OK;
		}

		return E_NOTIMPL;
	}

protected:
	Root	_root;

	HWND	_hWndFrame;

	IShellView*	_pShellView;	// current hosted shellview
	HIMAGELIST	_himlSmall;		// list
//	HIMAGELIST	_himlLarge;		// shell image
	TreeDropTarget* _pDropTarget;

	LRESULT	WndProc(UINT nmsg, WPARAM wparam, LPARAM lparam);
	int 	Notify(int id, NMHDR* pnmh);

	void	OnCreate(LPCREATESTRUCT);
	void	InitializeTree(/*const FileChildWndInfo& info*/);
	void	InsertSubitems(HTREEITEM hParentItem, Entry* entry, IShellFolder* pParentFolder);
	bool	InitDragDrop();

	void	OnTreeGetDispInfo(int idCtrl, LPNMHDR pnmh);
	void	OnTreeItemExpanding(int idCtrl, LPNMTREEVIEW pnmtv);
	void	OnTreeItemRClick(int idCtrl, LPNMHDR pnmh);
	void	OnTreeItemSelected(int idCtrl, LPNMTREEVIEW pnmtv);

	void	Tree_DoItemMenu(HWND hwndTreeView, HTREEITEM hItem, LPPOINT pptScreen);
};
