/*
 * PROJECT:     ReactOS Search Shell Extension
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Search UI
 * COPYRIGHT:   Copyright 2019 Brock Mammen
 */

#pragma once

#include "shellfind.h"

class CSearchBar :
    public CComCoClass<CSearchBar, &CLSID_FileSearchBand>,
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IDeskBand,
    public IObjectWithSite,
    public IInputObject,
    public IPersistStream,
    public IDispatch,
    public CDialogImpl<CSearchBar>
{

private:
    // *** BaseBarSite information ***
    CComPtr<IUnknown> m_pSite;
    CComPtr<IAddressEditBox> m_AddressEditBox;
    BOOL m_bVisible;

    HRESULT GetSearchResultsFolder(IShellBrowser **ppShellBrowser, HWND *pHwnd, IShellFolder **ppShellFolder);
    BOOL GetAddressEditBoxPath(WCHAR *szPath);
    void SetSearchInProgress(BOOL bInProgress);
    HRESULT TrySubscribeToSearchEvents();

    // *** ATL event handlers ***
    LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
    LRESULT OnSetFocus(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
    LRESULT OnSearchButtonClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL &bHandled);
    LRESULT OnStopButtonClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL &bHandled);
    LRESULT OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);

public:
    CSearchBar();
    virtual ~CSearchBar();

    // *** IOleWindow methods ***
    STDMETHOD(GetWindow)(HWND *lphwnd) override;
    STDMETHOD(ContextSensitiveHelp)(BOOL fEnterMode) override;

    // *** IDockingWindow methods ***
    STDMETHOD(CloseDW)(DWORD dwReserved) override;
    STDMETHOD(ResizeBorderDW)(const RECT *prcBorder, IUnknown *punkToolbarSite, BOOL fReserved) override;
    STDMETHOD(ShowDW)(BOOL fShow) override;

    // *** IDeskBand methods ***
    STDMETHOD(GetBandInfo)(DWORD dwBandID, DWORD dwViewMode, DESKBANDINFO *pdbi) override;

    // *** IObjectWithSite methods ***
    STDMETHOD(SetSite)(IUnknown *pUnkSite) override;
    STDMETHOD(GetSite)(REFIID riid, void **ppvSite) override;

    // *** IInputObject methods ***
    STDMETHOD(UIActivateIO)(BOOL fActivate, LPMSG lpMsg) override;
    STDMETHOD(HasFocusIO)() override;
    STDMETHOD(TranslateAcceleratorIO)(LPMSG lpMsg) override;

    // *** IPersist methods ***
    STDMETHOD(GetClassID)(CLSID *pClassID) override;

    // *** IPersistStream methods ***
    STDMETHOD(IsDirty)() override;
    STDMETHOD(Load)(IStream *pStm) override;
    STDMETHOD(Save)(IStream *pStm, BOOL fClearDirty) override;
    STDMETHOD(GetSizeMax)(ULARGE_INTEGER *pcbSize) override;

    // *** IDispatch methods ***
    STDMETHOD(GetTypeInfoCount)(UINT *pctinfo) override;
    STDMETHOD(GetTypeInfo)(UINT iTInfo, LCID lcid, ITypeInfo **ppTInfo) override;
    STDMETHOD(GetIDsOfNames)(REFIID riid, LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId) override;
    STDMETHOD(Invoke)(DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr) override;

    enum { IDD = IDD_SEARCH_DLG };

    DECLARE_REGISTRY_RESOURCEID(IDR_FILESEARCHBAND)
    DECLARE_NOT_AGGREGATABLE(CSearchBar)

    DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_COM_MAP(CSearchBar)
        COM_INTERFACE_ENTRY_IID(IID_IDispatch, IDispatch)
        COM_INTERFACE_ENTRY_IID(DIID_DWebBrowserEvents, IDispatch)
        COM_INTERFACE_ENTRY_IID(DIID_DSearchCommandEvents, IDispatch)
        COM_INTERFACE_ENTRY2_IID(IID_IOleWindow, IOleWindow, IDeskBand)
        COM_INTERFACE_ENTRY2_IID(IID_IDockingWindow, IDockingWindow, IDeskBand)
        COM_INTERFACE_ENTRY_IID(IID_IDeskBand, IDeskBand)
        COM_INTERFACE_ENTRY_IID(IID_IObjectWithSite, IObjectWithSite)
        COM_INTERFACE_ENTRY_IID(IID_IInputObject, IInputObject)
        COM_INTERFACE_ENTRY2_IID(IID_IPersist, IPersist, IPersistStream)
        COM_INTERFACE_ENTRY_IID(IID_IPersistStream, IPersistStream)
    END_COM_MAP()

    BEGIN_MSG_MAP(CSearchBar)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        MESSAGE_HANDLER(WM_SETFOCUS, OnSetFocus)
        MESSAGE_HANDLER(WM_SIZE, OnSize)
        COMMAND_HANDLER(IDC_SEARCH_BUTTON, BN_CLICKED, OnSearchButtonClicked)
        COMMAND_HANDLER(IDC_SEARCH_STOP_BUTTON, BN_CLICKED, OnStopButtonClicked)
    END_MSG_MAP()
};
