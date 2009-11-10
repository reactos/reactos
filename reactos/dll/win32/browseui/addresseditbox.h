/*
 * ReactOS Explorer
 *
 * Copyright 2009 Andrew Hill <ash77 at domain reactos.org>
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

#ifndef _addresseditbox_h
#define _addresseditbox_h

class CAddressEditBox :
	public CWindowImpl<CAddressEditBox, CWindow, CControlWinTraits>,
	public CComCoClass<CAddressEditBox, &CLSID_AddressEditBox>,
	public CComObjectRootEx<CComMultiThreadModelNoCS>,
	public IShellService,
	public IAddressBand,
	public IAddressEditBox,
	public IWinEventHandler,
	public IOleCommandTarget,
	public IDispatch,
	public IPersistStream
{
private:
	CContainedWindow						fEditWindow;
	CContainedWindow						fComboBoxExWindow;
public:
	CAddressEditBox();
	~CAddressEditBox();
private:
public:
	// *** IShellService methods ***
    virtual HRESULT STDMETHODCALLTYPE SetOwner(IUnknown *);

	// *** IAddressBand methods ***
	virtual HRESULT STDMETHODCALLTYPE FileSysChange(long param8, long paramC);
	virtual HRESULT STDMETHODCALLTYPE Refresh(long param8);

	// *** IAddressEditBox methods ***
	virtual HRESULT STDMETHODCALLTYPE Init(HWND comboboxEx, HWND editControl, long param14, IUnknown *param18);
	virtual HRESULT STDMETHODCALLTYPE SetCurrentDir(long paramC);
	virtual HRESULT STDMETHODCALLTYPE ParseNow(long paramC);
	virtual HRESULT STDMETHODCALLTYPE Execute(long paramC);
	virtual HRESULT STDMETHODCALLTYPE Save(long paramC);

	// *** IWinEventHandler methods ***
	virtual HRESULT STDMETHODCALLTYPE OnWinEvent(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *theResult);
	virtual HRESULT STDMETHODCALLTYPE IsWindowOwner(HWND hWnd);

	// *** IOleCommandTarget methods ***
	virtual HRESULT STDMETHODCALLTYPE QueryStatus(const GUID *pguidCmdGroup, ULONG cCmds, OLECMD prgCmds[  ], OLECMDTEXT *pCmdText);
	virtual HRESULT STDMETHODCALLTYPE Exec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut);

	// *** IDispatch methods ***
	virtual HRESULT STDMETHODCALLTYPE GetTypeInfoCount(UINT *pctinfo);
	virtual HRESULT STDMETHODCALLTYPE GetTypeInfo(UINT iTInfo, LCID lcid, ITypeInfo **ppTInfo);
	virtual HRESULT STDMETHODCALLTYPE GetIDsOfNames(REFIID riid, LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId);
	virtual HRESULT STDMETHODCALLTYPE Invoke(DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr);

	// *** IPersist methods ***
	virtual HRESULT STDMETHODCALLTYPE GetClassID(CLSID *pClassID);

	// *** IPersistStream methods ***
	virtual HRESULT STDMETHODCALLTYPE IsDirty();
	virtual HRESULT STDMETHODCALLTYPE Load(IStream *pStm);
	virtual HRESULT STDMETHODCALLTYPE Save(IStream *pStm, BOOL fClearDirty);
	virtual HRESULT STDMETHODCALLTYPE GetSizeMax(ULARGE_INTEGER *pcbSize);

	// message handlers
//	LRESULT OnSetFocus(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
//	LRESULT OnKillFocus(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);

DECLARE_REGISTRY_RESOURCEID(IDR_ADDRESSEDITBOX)
DECLARE_NOT_AGGREGATABLE(CAddressEditBox)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_MSG_MAP(CAddressEditBox)
//	MESSAGE_HANDLER(WM_NOTIFY, OnNotify)
//	MESSAGE_HANDLER(WM_SETFOCUS, OnSetFocus)
//	MESSAGE_HANDLER(WM_KILLFOCUS, OnKillFocus)
//	ALT_MSG_MAP(1)
//		MESSAGE_HANDLER(WM_SETFOCUS, OnSetFocusSaveButton)
//	ALT_MSG_MAP(2)
//		MESSAGE_HANDLER(WM_SETFOCUS, OnSetFocusSelectButton)
END_MSG_MAP()

BEGIN_COM_MAP(CAddressEditBox)
	COM_INTERFACE_ENTRY_IID(IID_IShellService, IShellService)
	COM_INTERFACE_ENTRY_IID(IID_IAddressBand, IAddressBand)
	COM_INTERFACE_ENTRY_IID(IID_IAddressEditBox, IAddressEditBox)
	COM_INTERFACE_ENTRY_IID(IID_IWinEventHandler, IWinEventHandler)
	COM_INTERFACE_ENTRY_IID(IID_IOleCommandTarget, IOleCommandTarget)
	COM_INTERFACE_ENTRY_IID(IID_IDispatch, IDispatch)
	COM_INTERFACE_ENTRY_IID(IID_IPersist, IPersist)
	COM_INTERFACE_ENTRY_IID(IID_IPersistStream, IPersistStream)
END_COM_MAP()
};

#endif // _addresseditbox_h
