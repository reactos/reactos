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
    public IInputObjectSite
{
private:
    CComPtr<IDockingWindowSite>             fSite;
    CComPtr<IAddressEditBox>                fAddressEditBox;
    HWND                                    fEditControl;
    HWND                                    fGoButton;
    HWND                                    fComboBox;
    bool                                    fGoButtonShown;
    HIMAGELIST                              m_himlNormal;
    HIMAGELIST                              m_himlHot;

public:
    CAddressBand();
    virtual ~CAddressBand();
private:
    void FocusChange(BOOL bFocus);
    void CreateGoButton();
public:
    static BOOL ShouldShowGoButton();
    static BOOL IsGoButtonVisible(IUnknown *pUnkBand);

    // *** IDeskBand methods ***
    STDMETHOD(GetBandInfo)(DWORD dwBandID, DWORD dwViewMode, DESKBANDINFO *pdbi) override;

    // *** IObjectWithSite methods ***
    STDMETHOD(SetSite)(IUnknown *pUnkSite) override;
    STDMETHOD(GetSite)(REFIID riid, void **ppvSite) override;

    // *** IOleWindow methods ***
    STDMETHOD(GetWindow)(HWND *lphwnd) override;
    STDMETHOD(ContextSensitiveHelp)(BOOL fEnterMode) override;

    // *** IDockingWindow methods ***
    STDMETHOD(CloseDW)(DWORD dwReserved) override;
    STDMETHOD(ResizeBorderDW)(const RECT *prcBorder, IUnknown *punkToolbarSite, BOOL fReserved) override;
    STDMETHOD(ShowDW)(BOOL fShow) override;

    // *** IOleCommandTarget methods ***
    STDMETHOD(QueryStatus)(const GUID *pguidCmdGroup, ULONG cCmds, OLECMD prgCmds[  ], OLECMDTEXT *pCmdText) override;
    STDMETHOD(Exec)(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut) override;

    // *** IInputObject methods ***
    STDMETHOD(UIActivateIO)(BOOL fActivate, LPMSG lpMsg) override;
    STDMETHOD(HasFocusIO)() override;
    STDMETHOD(TranslateAcceleratorIO)(LPMSG lpMsg) override;

    // *** IWinEventHandler methods ***
    STDMETHOD(OnWinEvent)(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *theResult) override;
    STDMETHOD(IsWindowOwner)(HWND hWnd) override;

    // *** IAddressBand methods ***
    STDMETHOD(FileSysChange)(long param8, long paramC) override;
    STDMETHOD(Refresh)(long param8) override;

    // *** IServiceProvider methods ***
    STDMETHOD(QueryService)(REFGUID guidService, REFIID riid, void **ppvObject) override;

    // *** IInputObjectSite methods ***
    STDMETHOD(OnFocusChangeIS)(IUnknown *punkObj, BOOL fSetFocus) override;

    // *** IPersist methods ***
    STDMETHOD(GetClassID)(CLSID *pClassID) override;

    // *** IPersistStream methods ***
    STDMETHOD(IsDirty)() override;
    STDMETHOD(Load)(IStream *pStm) override;
    STDMETHOD(Save)(IStream *pStm, BOOL fClearDirty) override;
    STDMETHOD(GetSizeMax)(ULARGE_INTEGER *pcbSize) override;

    // message handlers
    LRESULT OnNotifyClick(WPARAM wParam, NMHDR *notifyHeader, BOOL &bHandled);
    LRESULT OnTipText(UINT idControl, NMHDR *notifyHeader, BOOL &bHandled);
    LRESULT OnEraseBackground(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
    LRESULT OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
    LRESULT OnWindowPosChanging(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);

    BEGIN_MSG_MAP(CAddressBand)
        NOTIFY_CODE_HANDLER(NM_CLICK, OnNotifyClick)
        NOTIFY_CODE_HANDLER(TBN_GETINFOTIP, OnTipText)
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
    END_COM_MAP()
};
