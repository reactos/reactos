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
 // Credits: Thanks to Leon Finker for his explorer window example
 //


struct IShellBrowserImpl : public IShellBrowser, public ICommDlgBrowser
{
	IShellBrowserImpl()
	 :	_dwRef(0)
	{
	}

	STDMETHOD(QueryInterface)(REFIID iid, void **ppvObject)
	{
		if (!ppvObject)
			return E_POINTER;

		if (iid == IID_IUnknown)
			*ppvObject = (IUnknown*)static_cast<IShellBrowser*>(this);
		else if (iid == IID_IOleWindow)
			*ppvObject = static_cast<IOleWindow*>(this);
		else if (iid == IID_IShellBrowser)
			*ppvObject = static_cast<IShellBrowser*>(this);
		else if (iid == IID_ICommDlgBrowser)
			*ppvObject = static_cast<ICommDlgBrowser*>(this);
		else {
			*ppvObject = NULL;
			return E_NOINTERFACE;
		}

		return S_OK;
	}

	STDMETHOD_(ULONG, AddRef)() {return ++_dwRef;}
	STDMETHOD_(ULONG, Release)() {return --_dwRef;}  //not heap based

    // *** IOleWindow methods ***
    STDMETHOD(ContextSensitiveHelp)(BOOL fEnterMode) {return E_NOTIMPL;}

	// *** ICommDlgBrowser methods ***
    STDMETHOD(OnDefaultCommand)(struct IShellView* ppshv)
	{
		return E_NOTIMPL;
	}

    STDMETHOD(OnStateChange)(struct IShellView* ppshv, ULONG uChange)
	{	//handle selection, rename, focus if needed
		return E_NOTIMPL;
	}

    STDMETHOD(IncludeObject)(struct IShellView* ppshv, LPCITEMIDLIST pidl)
	{	//filter files if needed
		return S_OK;
	}

    // *** IShellBrowser methods *** (same as IOleInPlaceFrame)
    STDMETHOD(InsertMenusSB)(HMENU hmenuShared, LPOLEMENUGROUPWIDTHS lpMenuWidths) {return E_NOTIMPL;}
    STDMETHOD(SetMenuSB)(HMENU hmenuShared, HOLEMENU holemenuReserved,HWND hwndActiveObject) {return E_NOTIMPL;}
    STDMETHOD(RemoveMenusSB)(HMENU hmenuShared) {return E_NOTIMPL;}
    STDMETHOD(SetStatusTextSB)(LPCOLESTR lpszStatusText) {return E_NOTIMPL;}
    STDMETHOD(EnableModelessSB)(BOOL fEnable) {return E_NOTIMPL;}
	STDMETHOD(BrowseObject)(LPCITEMIDLIST pidl, UINT wFlags) {return E_NOTIMPL;}
	STDMETHOD(GetViewStateStream)(DWORD grfMode,LPSTREAM  *ppStrm) {return E_NOTIMPL;}
	STDMETHOD(OnViewWindowActive)(struct IShellView *ppshv) {return E_NOTIMPL;}
	STDMETHOD(SetToolbarItems)(LPTBBUTTON lpButtons, UINT nButtons,UINT uFlags) {return E_NOTIMPL;}
	STDMETHOD(TranslateAcceleratorSB)(LPMSG lpmsg, WORD wID) {return S_OK;}

protected:
	DWORD	_dwRef;
};

#ifndef WM_GETISHELLBROWSER
#define WM_GETISHELLBROWSER (WM_USER+7)
#endif
