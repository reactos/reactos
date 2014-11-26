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

class CAddressBand :
    public CWindowImpl<CAddressBand, CWindow, CControlWinTraits>,
    public CComCoClass<CAddressBand, &CLSID_SH_AddressBand>,
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IDeskBand,
    public IObjectWithSite,
    public IInputObject,
    public IPersistStream,
    public IOleCommandTarget,
    public IServiceProvider,
    public IWinEventHandler,
    public IAddressBand,
    public IInputObjectSite,
    public IDispatch
{
private:
    CComPtr<IDockingWindowSite>             fSite;
    CComPtr<IAddressEditBox>                fAddressEditBox;
    HWND                                    fEditControl;
    HWND                                    fGoButton;
    HWND                                    fComboBox;
    bool                                    fGoButtonShown;
    DWORD                                   fAdviseCookie;
public:
    CAddressBand();
    virtual ~CAddressBand();
private:
    void FocusChange(BOOL bFocus);
public:
    // *** IDeskBand methods ***
    virtual HRESULT STDMETHODCALLTYPE GetBandInfo(DWORD dwBandID, DWORD dwViewMode, DESKBANDINFO *pdbi);

    // *** IObjectWithSite methods ***
    virtual HRESULT STDMETHODCALLTYPE SetSite(IUnknown *pUnkSite);
    virtual HRESULT STDMETHODCALLTYPE GetSite(REFIID riid, void **ppvSite);

    // *** IOleWindow methods ***
    virtual HRESULT STDMETHODCALLTYPE GetWindow(HWND *lphwnd);
    virtual HRESULT STDMETHODCALLTYPE ContextSensitiveHelp(BOOL fEnterMode);

    // *** IDockingWindow methods ***
    virtual HRESULT STDMETHODCALLTYPE CloseDW(DWORD dwReserved);
    virtual HRESULT STDMETHODCALLTYPE ResizeBorderDW(const RECT *prcBorder, IUnknown *punkToolbarSite, BOOL fReserved);
    virtual HRESULT STDMETHODCALLTYPE ShowDW(BOOL fShow);

    // *** IOleCommandTarget methods ***
    virtual HRESULT STDMETHODCALLTYPE QueryStatus(const GUID *pguidCmdGroup, ULONG cCmds, OLECMD prgCmds[  ], OLECMDTEXT *pCmdText);
    virtual HRESULT STDMETHODCALLTYPE Exec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut);

    // *** IInputObject methods ***
    virtual HRESULT STDMETHODCALLTYPE UIActivateIO(BOOL fActivate, LPMSG lpMsg);
    virtual HRESULT STDMETHODCALLTYPE HasFocusIO();
    virtual HRESULT STDMETHODCALLTYPE TranslateAcceleratorIO(LPMSG lpMsg);

    // *** IWinEventHandler methods ***
    virtual HRESULT STDMETHODCALLTYPE OnWinEvent(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *theResult);
    virtual HRESULT STDMETHODCALLTYPE IsWindowOwner(HWND hWnd);

    // *** IAddressBand methods ***
    virtual HRESULT STDMETHODCALLTYPE FileSysChange(long param8, long paramC);
    virtual HRESULT STDMETHODCALLTYPE Refresh(long param8);

    // *** IServiceProvider methods ***
    virtual HRESULT STDMETHODCALLTYPE QueryService(REFGUID guidService, REFIID riid, void **ppvObject);

    // *** IInputObjectSite methods ***
    virtual HRESULT STDMETHODCALLTYPE OnFocusChangeIS(IUnknown *punkObj, BOOL fSetFocus);

    // *** IPersist methods ***
    virtual HRESULT STDMETHODCALLTYPE GetClassID(CLSID *pClassID);

    // *** IPersistStream methods ***
    virtual HRESULT STDMETHODCALLTYPE IsDirty();
    virtual HRESULT STDMETHODCALLTYPE Load(IStream *pStm);
    virtual HRESULT STDMETHODCALLTYPE Save(IStream *pStm, BOOL fClearDirty);
    virtual HRESULT STDMETHODCALLTYPE GetSizeMax(ULARGE_INTEGER *pcbSize);

    // *** IDispatch methods ***
    virtual HRESULT STDMETHODCALLTYPE GetTypeInfoCount(UINT *pctinfo);
    virtual HRESULT STDMETHODCALLTYPE GetTypeInfo(UINT iTInfo, LCID lcid, ITypeInfo **ppTInfo);
    virtual HRESULT STDMETHODCALLTYPE GetIDsOfNames(REFIID riid, LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId);
    virtual HRESULT STDMETHODCALLTYPE Invoke(DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr);

    // message handlers
    LRESULT OnNotifyClick(WPARAM wParam, NMHDR *notifyHeader, BOOL &bHandled);
    LRESULT OnTipText(UINT idControl, NMHDR *notifyHeader, BOOL &bHandled);
    LRESULT OnEraseBackground(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
    LRESULT OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
    LRESULT OnWindowPosChanging(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);

    BEGIN_MSG_MAP(CAddressBand)
        NOTIFY_CODE_HANDLER(NM_CLICK, OnNotifyClick)
        NOTIFY_CODE_HANDLER(TTN_NEEDTEXTW, OnTipText)
        MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBackground)
        MESSAGE_HANDLER(WM_SIZE, OnSize)
        MESSAGE_HANDLER(WM_WINDOWPOSCHANGING, OnWindowPosChanging)
    END_MSG_MAP()

    DECLARE_REGISTRY_RESOURCEID(IDR_ADDRESSBAND)
    DECLARE_NOT_AGGREGATABLE(CAddressBand)

    DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_COM_MAP(CAddressBand)
        COM_INTERFACE_ENTRY_IID(IID_IDeskBand, IDeskBand)
        COM_INTERFACE_ENTRY_IID(IID_IObjectWithSite, IObjectWithSite)
        COM_INTERFACE_ENTRY_IID(IID_IOleWindow, IOleWindow)
        COM_INTERFACE_ENTRY_IID(IID_IDockingWindow, IDockingWindow)
        COM_INTERFACE_ENTRY_IID(IID_IOleCommandTarget, IOleCommandTarget)
        COM_INTERFACE_ENTRY_IID(IID_IInputObject, IInputObject)
        COM_INTERFACE_ENTRY_IID(IID_IWinEventHandler, IWinEventHandler)
        COM_INTERFACE_ENTRY_IID(IID_IAddressBand, IAddressBand)
        COM_INTERFACE_ENTRY_IID(IID_IServiceProvider, IServiceProvider)
        COM_INTERFACE_ENTRY_IID(IID_IInputObjectSite, IInputObjectSite)
        COM_INTERFACE_ENTRY_IID(IID_IPersist, IPersist)
        COM_INTERFACE_ENTRY_IID(IID_IPersistStream, IPersistStream)
        COM_INTERFACE_ENTRY_IID(IID_IDispatch, IDispatch)
    END_COM_MAP()
};
