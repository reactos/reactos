/*
 * PROJECT:     ReactOS shdocvw
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Explorer bar
 * COPYRIGHT:   Copyright 2016 Sylvain Deverre <deverre dot sylv at gmail dot com>
 *              Copyright 2020-2024 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#pragma once

#include "CNSCBand.h"

class CExplorerBand
    : public CNSCBand
    , public CComCoClass<CExplorerBand, &CLSID_ExplorerBand>
    , public CComObjectRootEx<CComMultiThreadModelNoCS>
    , public IDispatch
{
public:
    CExplorerBand();
    virtual ~CExplorerBand();

    STDMETHODIMP GetClassID(CLSID *pClassID) override;
    STDMETHODIMP OnSelectionChanged(_In_ PCIDLIST_ABSOLUTE pidl) override;
    STDMETHODIMP Invoke(_In_ PCIDLIST_ABSOLUTE pidl) override;

    // *** IDispatch methods ***
    STDMETHODIMP GetTypeInfoCount(UINT *pctinfo) override;
    STDMETHODIMP GetTypeInfo(UINT iTInfo, LCID lcid, ITypeInfo **ppTInfo) override;
    STDMETHODIMP GetIDsOfNames(REFIID riid, LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId) override;
    STDMETHODIMP Invoke(DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr) override;

    DECLARE_REGISTRY_RESOURCEID(IDR_EXPLORERBAND)
    DECLARE_NOT_AGGREGATABLE(CExplorerBand)

    DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_COM_MAP(CExplorerBand)
        COM_INTERFACE_ENTRY_IID(IID_IDeskBand, IDeskBand)
        COM_INTERFACE_ENTRY2_IID(IID_IDockingWindow, IDockingWindow, IDeskBand)
        COM_INTERFACE_ENTRY2_IID(IID_IOleWindow, IOleWindow, IDeskBand)
        COM_INTERFACE_ENTRY_IID(IID_IObjectWithSite, IObjectWithSite)
        COM_INTERFACE_ENTRY_IID(IID_IInputObject, IInputObject)
        COM_INTERFACE_ENTRY_IID(IID_IPersistStream, IPersistStream)
        COM_INTERFACE_ENTRY2_IID(IID_IPersist, IPersist, IPersistStream)
        COM_INTERFACE_ENTRY_IID(IID_IOleCommandTarget, IOleCommandTarget)
        COM_INTERFACE_ENTRY_IID(IID_IServiceProvider, IServiceProvider)
        COM_INTERFACE_ENTRY_IID(IID_IContextMenu, IContextMenu)
        COM_INTERFACE_ENTRY_IID(IID_IBandNavigate, IBandNavigate)
        COM_INTERFACE_ENTRY_IID(IID_IWinEventHandler, IWinEventHandler)
        COM_INTERFACE_ENTRY_IID(IID_INamespaceProxy, INamespaceProxy)
        COM_INTERFACE_ENTRY_IID(IID_IDispatch, IDispatch)
        COM_INTERFACE_ENTRY2_IID(IID_IUnknown, IUnknown, IDispatch)
    END_COM_MAP()

protected:
    INT _GetRootCsidl() override;
    DWORD _GetTVStyle() override;
    DWORD _GetTVExStyle() override;
    DWORD _GetEnumFlags() override;
    HRESULT _CreateTreeView(HWND hwndParent) override;
    void _DestroyTreeView() override;
    BOOL _WantsRootItem() override;
    BOOL _GetTitle(LPWSTR pszTitle, INT cchTitle) override;
    void _SortItems(HTREEITEM hParent) override;
    BOOL _NavigateToCurrentFolder();
    BOOL _NavigateToPIDL(
        _In_ LPCITEMIDLIST dest,
        _Out_ HTREEITEM *phItem,
        _In_ BOOL bExpand,
        _In_ BOOL bInsert,
        _In_ BOOL bSelect);
    static INT CALLBACK _CompareTreeItems(LPARAM p1, LPARAM p2, LPARAM p3); // Used by _SortItems
};
