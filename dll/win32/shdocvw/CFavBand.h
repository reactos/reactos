/*
 * PROJECT:     ReactOS shdocvw
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Favorites bar
 * COPYRIGHT:   Copyright 2024 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#pragma once

#ifdef __cplusplus

#include "CNSCBand.h"

class CFavBand
    : public CNSCBand
    , public CComCoClass<CFavBand, &CLSID_SH_FavBand>
    , public CComObjectRootEx<CComMultiThreadModelNoCS>
{
public:
    CFavBand();
    virtual ~CFavBand();

    STDMETHODIMP GetClassID(CLSID *pClassID) override;
    STDMETHODIMP OnSelectionChanged(_In_ PCIDLIST_ABSOLUTE pidl);

    DECLARE_REGISTRY_RESOURCEID(IDR_FAVBAND)
    DECLARE_NOT_AGGREGATABLE(CFavBand)
    DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_COM_MAP(CFavBand)
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
        COM_INTERFACE_ENTRY2_IID(IID_IUnknown, IUnknown, IContextMenu)
        COM_INTERFACE_ENTRY_IID(IID_IBandNavigate, IBandNavigate)
        COM_INTERFACE_ENTRY_IID(IID_IWinEventHandler, IWinEventHandler)
        COM_INTERFACE_ENTRY_IID(IID_INamespaceProxy, INamespaceProxy)
    END_COM_MAP()

protected:
    INT _GetRootCsidl() override;
    DWORD _GetTVStyle() override;
    DWORD _GetTVExStyle() override;
    DWORD _GetEnumFlags() override;
    BOOL _GetTitle(LPWSTR pszTitle, INT cchTitle) override;
    HRESULT _CreateTreeView(HWND hwndParent) override;
    HRESULT _CreateToolbar(HWND hwndParent) override;
    BOOL _WantsRootItem() override;
    void _SortItems(HTREEITEM hParent) override;
};

#endif // def __cplusplus
