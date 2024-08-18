/*
 * PROJECT:     browseui
 * LICENSE:     LGPL-2.1+ (https://spdx.org/licenses/LGPL-2.1+)
 * PURPOSE:     Common registry based settings editor
 * COPYRIGHT:   Copyright 2024 Whindmar Saksit <whindsaks@proton.me>
 */

#pragma once

class CObjectWithSiteBase :
    public IObjectWithSite
{
public:
    IUnknown* m_pUnkSite;

    CObjectWithSiteBase() : m_pUnkSite(NULL) {}
    virtual ~CObjectWithSiteBase() { SetSite(NULL); }

    STDMETHODIMP SetSite(IUnknown *pUnkSite) override
    {
        IUnknown_Set(&m_pUnkSite, pUnkSite);
        return S_OK;
    }
    STDMETHODIMP GetSite(REFIID riid, void **ppvSite) override
    {
        *ppvSite = NULL;
        return m_pUnkSite ? m_pUnkSite->QueryInterface(riid, ppvSite) : E_FAIL;
    }
};

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

    // *** IRegTreeOptions methods ***
    STDMETHOD(InitTree)(HWND hTV, HKEY hKey, LPCSTR SubKey, char const *param18) override;
    STDMETHOD(WalkTree)(WALK_TREE_CMD Command) override;
    STDMETHOD(ToggleItem)(HTREEITEM hTI) override;
    STDMETHOD(ShowHelp)(HTREEITEM hTI, unsigned long param10) override;

    DECLARE_REGISTRY_RESOURCEID(IDR_REGTREEOPTIONS)
    DECLARE_NOT_AGGREGATABLE(CRegTreeOptions)

    BEGIN_COM_MAP(CRegTreeOptions)
        COM_INTERFACE_ENTRY_IID(IID_IRegTreeOptions, IRegTreeOptions)
        COM_INTERFACE_ENTRY_IID(IID_IObjectWithSite, IObjectWithSite)
    END_COM_MAP()
};
