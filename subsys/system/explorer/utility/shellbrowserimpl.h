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
 // shellbrowserimpl.h
 //
 // Martin Fuchs, 23.07.2003
 //
 // Credits: Thanks to Leon Finker for his explorer cabinet window example
 //

#ifdef __MINGW32__
#include "servprov.h"	// for IServiceProvider
#include "docobj.h" 	// for IOleCommandTarget
#endif


 /// Implementation of IShellBrowser and ICommDlgBrowser interfaces for explorer child windows (see ShellBrowserChild)
struct IShellBrowserImpl
 :	public IShellBrowser,
	public ICommDlgBrowser,
	public IServiceProvider,
	public IOleCommandTarget
{
	IShellBrowserImpl()
	 :	_dwRef(0)
	{
	}

	virtual ~IShellBrowserImpl()
	{
	}

	virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void** ppvObject);

	virtual ULONG STDMETHODCALLTYPE AddRef() {return ++_dwRef;}
	virtual ULONG STDMETHODCALLTYPE Release() {return --_dwRef;}  //not heap based

	// *** IOleWindow methods ***
	virtual HRESULT STDMETHODCALLTYPE ContextSensitiveHelp(BOOL fEnterMode) {return E_NOTIMPL;}

	// *** ICommDlgBrowser methods ***
	virtual HRESULT STDMETHODCALLTYPE OnDefaultCommand(IShellView* ppshv);

	virtual HRESULT STDMETHODCALLTYPE OnStateChange(IShellView* ppshv, ULONG uChange)
	{	//handle selection, rename, focus if needed
		return E_NOTIMPL;
	}

	virtual HRESULT STDMETHODCALLTYPE IncludeObject(IShellView* ppshv, LPCITEMIDLIST pidl)
	{	//filter files if needed
		return S_OK;
	}

	// *** IShellBrowser methods *** (same as IOleInPlaceFrame)
	virtual HRESULT STDMETHODCALLTYPE InsertMenusSB(HMENU hmenuShared, LPOLEMENUGROUPWIDTHS lpMenuWidths) {return E_NOTIMPL;}
	virtual HRESULT STDMETHODCALLTYPE SetMenuSB(HMENU hmenuShared, HOLEMENU holemenuReserved, HWND hwndActiveObject) {return E_NOTIMPL;}
	virtual HRESULT STDMETHODCALLTYPE RemoveMenusSB(HMENU hmenuShared) {return E_NOTIMPL;}
	virtual HRESULT STDMETHODCALLTYPE SetStatusTextSB(LPCOLESTR lpszStatusText) {return E_NOTIMPL;}
	virtual HRESULT STDMETHODCALLTYPE EnableModelessSB(BOOL fEnable) {return E_NOTIMPL;}
	virtual HRESULT STDMETHODCALLTYPE BrowseObject(LPCITEMIDLIST pidl, UINT wFlags) {return E_NOTIMPL;}
	virtual HRESULT STDMETHODCALLTYPE GetViewStateStream(DWORD grfMode, LPSTREAM* ppStrm) {return E_NOTIMPL;}
	virtual HRESULT STDMETHODCALLTYPE OnViewWindowActive(IShellView* ppshv) {return E_NOTIMPL;}
	virtual HRESULT STDMETHODCALLTYPE SetToolbarItems(LPTBBUTTON lpButtons, UINT nButtons, UINT uFlags) {return E_NOTIMPL;}
	virtual HRESULT STDMETHODCALLTYPE TranslateAcceleratorSB(LPMSG lpmsg, WORD wID) {return S_OK;}

	// IServiceProvider
	virtual HRESULT STDMETHODCALLTYPE QueryService(REFGUID guidService, REFIID riid, void** ppvObject);

	// IOleCommandTarget
	virtual HRESULT STDMETHODCALLTYPE QueryStatus(const GUID *pguidCmdGroup, ULONG cCmds, OLECMD prgCmds[], OLECMDTEXT* pCmdText);
	virtual HRESULT STDMETHODCALLTYPE Exec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut);

protected:
	DWORD	_dwRef;

	virtual HRESULT OnDefaultCommand(LPIDA pida) {return E_NOTIMPL;}
};

#ifndef WM_GETISHELLBROWSER
#define WM_GETISHELLBROWSER (WM_USER+7)
#endif
