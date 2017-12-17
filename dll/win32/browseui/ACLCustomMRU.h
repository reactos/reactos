/*
 * PROJECT:     ReactOS browseui
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Custom MRU AutoComplete List
 * COPYRIGHT:   Copyright 2017 Mark Jansen (mark.jansen@reactos.org)
 */

#pragma once

class CACLCustomMRU :
    public CComCoClass<CACLCustomMRU, &CLSID_ACLCustomMRU>,
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IACLCustomMRU
{
private:
    CRegKey m_Key;
    CStringW m_MRUList;
    CSimpleArray<CStringW> m_MRUData;
    bool m_bDirty;

    void PersistMRU();

public:
    CACLCustomMRU();
    ~CACLCustomMRU();

    // *** IACLCustomMRU methods ***
    virtual HRESULT STDMETHODCALLTYPE Initialize(LPCWSTR pwszMRURegKey, DWORD dwMax);
    virtual HRESULT STDMETHODCALLTYPE AddMRUString(LPCWSTR pwszEntry);

public:

    DECLARE_REGISTRY_RESOURCEID(IDR_ACLCUSTOMMRU)
    DECLARE_NOT_AGGREGATABLE(CACLCustomMRU)

    DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_COM_MAP(CACLCustomMRU)
        COM_INTERFACE_ENTRY_IID(IID_IACLCustomMRU, IACLCustomMRU)
    END_COM_MAP()
};
