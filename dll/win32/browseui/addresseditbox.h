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

#pragma once

class CAddressEditBox :
    public CWindowImpl<CAddressEditBox, CWindow, CControlWinTraits>,
    public CComCoClass<CAddressEditBox, &CLSID_AddressEditBox>,
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IWinEventHandler,
    public IDispatch,
    public IAddressBand,
    public IAddressEditBox,
    public IOleCommandTarget,
    public IPersistStream,
    public IShellService
{
private:
    CContainedWindow                        fCombobox;
    CContainedWindow                        fEditWindow;
    DWORD                                   fAdviseCookie;
    CComPtr<IUnknown>                       fSite;
    LPITEMIDLIST                            pidlLastParsed;
    HWND                                    hComboBoxEx;
public:
    CAddressEditBox();
    ~CAddressEditBox();
private:
    void PopulateComboBox(LPITEMIDLIST pidl);
    void AddComboBoxItem(LPITEMIDLIST pidl, int index, int indent);
    void FillOneLevel(int index, int levelIndent, int indent);
    LPITEMIDLIST GetItemData(int index);
    HRESULT STDMETHODCALLTYPE ShowFileNotFoundError(HRESULT hRet);
    HRESULT GetAbsolutePidl(PIDLIST_ABSOLUTE *pAbsolutePIDL);
    BOOL ExecuteCommandLine();
    BOOL GetComboBoxText(CComHeapPtr<WCHAR>& pszText);
    HRESULT RefreshAddress();
public:
    // *** IShellService methods ***
    STDMETHOD(SetOwner)(IUnknown *) override;

    // *** IAddressBand methods ***
    STDMETHOD(FileSysChange)(long param8, long paramC) override;
    STDMETHOD(Refresh)(long param8) override;

    // *** IAddressEditBox methods ***
    STDMETHOD(Init)(HWND comboboxEx, HWND editControl, long param14, IUnknown *param18) override;
    STDMETHOD(SetCurrentDir)(long paramC) override;
    STDMETHOD(ParseNow)(long paramC) override;
    STDMETHOD(Execute)(long paramC) override;
    STDMETHOD(Save)(long paramC) override;

    // *** IWinEventHandler methods ***
    STDMETHOD(OnWinEvent)(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *theResult) override;
    STDMETHOD(IsWindowOwner)(HWND hWnd) override;

    // *** IOleCommandTarget methods ***
    STDMETHOD(QueryStatus)(const GUID *pguidCmdGroup, ULONG cCmds, OLECMD prgCmds[  ], OLECMDTEXT *pCmdText) override;
    STDMETHOD(Exec)(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut) override;

    // *** IDispatch methods ***
    STDMETHOD(GetTypeInfoCount)(UINT *pctinfo) override;
    STDMETHOD(GetTypeInfo)(UINT iTInfo, LCID lcid, ITypeInfo **ppTInfo) override;
    STDMETHOD(GetIDsOfNames)(REFIID riid, LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId) override;
    STDMETHOD(Invoke)(DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr) override;

    // *** IPersist methods ***
    STDMETHOD(GetClassID)(CLSID *pClassID) override;

    // *** IPersistStream methods ***
    STDMETHOD(IsDirty)() override;
    STDMETHOD(Load)(IStream *pStm) override;
    STDMETHOD(Save)(IStream *pStm, BOOL fClearDirty) override;
    STDMETHOD(GetSizeMax)(ULARGE_INTEGER *pcbSize) override;

    // message handlers
    LRESULT OnSettingChange(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);

    DECLARE_REGISTRY_RESOURCEID(IDR_ADDRESSEDITBOX)
    DECLARE_NOT_AGGREGATABLE(CAddressEditBox)

    DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_MSG_MAP(CAddressEditBox)
        MESSAGE_HANDLER(WM_SETTINGCHANGE, OnSettingChange)
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
