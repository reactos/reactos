/*
 * PROJECT:     browseui
 * LICENSE:     LGPL-2.1+ (https://spdx.org/licenses/LGPL-2.1+)
 * PURPOSE:     Common registry based settings editor
 * COPYRIGHT:   Copyright 2024 Whindmar Saksit <whindsaks@proton.me>
 */

#pragma once

class CRegTreeOptions :
    public CComCoClass<CRegTreeOptions, &CLSID_CRegTreeOptions>,
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IRegTreeOptions,
    public CObjectWithSiteBase
{
private:
    HWND m_hTree;
    HIMAGELIST m_hIL;

public:
    CRegTreeOptions();
    ~CRegTreeOptions();

    void AddItemsFromRegistry(HKEY hKey, HTREEITEM hParent, HTREEITEM hInsertAfter);
    HRESULT GetSetState(HKEY hKey, DWORD &Type, LPBYTE Data, DWORD &Size, BOOL Set);
    HRESULT GetCheckState(HKEY hKey, BOOL UseDefault = FALSE);
    HRESULT SaveCheckState(HKEY hKey, BOOL Checked);
    void WalkTree(WALK_TREE_CMD Command, HWND hTree, HTREEITEM hTI);

    // *** IRegTreeOptions methods ***
    STDMETHOD(InitTree)(HWND hTV, HKEY hKey, LPCSTR SubKey, char const *pUnknown) override;
    STDMETHOD(WalkTree)(WALK_TREE_CMD Command) override;
    STDMETHOD(ToggleItem)(HTREEITEM hTI) override;
    STDMETHOD(ShowHelp)(HTREEITEM hTI, unsigned long Unknown) override;

    DECLARE_REGISTRY_RESOURCEID(IDR_REGTREEOPTIONS)
    DECLARE_NOT_AGGREGATABLE(CRegTreeOptions)

    BEGIN_COM_MAP(CRegTreeOptions)
        COM_INTERFACE_ENTRY_IID(IID_IRegTreeOptions, IRegTreeOptions)
        COM_INTERFACE_ENTRY_IID(IID_IObjectWithSite, IObjectWithSite)
    END_COM_MAP()
};
