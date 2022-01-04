/*
 * PROJECT:     ReactOS browseui
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Custom MRU AutoComplete List
 * COPYRIGHT:   Copyright 2017 Mark Jansen (mark.jansen@reactos.org)
 *              Copyright 2020 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

#pragma once

class CACLCustomMRU :
    public CComCoClass<CACLCustomMRU, &CLSID_ACLCustomMRU>,
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IEnumString,
    public IACList,
    public IACLCustomMRU
{
private:
    CRegKey m_Key;
    CStringW m_MRUList;
    CSimpleArray<CStringW> m_MRUData;
    bool m_bDirty;
    BOOL m_bTypedURLs;
    ULONG m_ielt;

    void PersistMRU();
    HRESULT LoadTypedURLs(DWORD dwMax);
    HRESULT LoadMRUList(DWORD dwMax);

public:
    CACLCustomMRU();
    ~CACLCustomMRU();

    // *** IEnumString methods ***
    STDMETHODIMP Next(ULONG celt, LPWSTR *rgelt, ULONG *pceltFetched) override;
    STDMETHODIMP Skip(ULONG celt) override;
    STDMETHODIMP Reset() override;
    STDMETHODIMP Clone(IEnumString ** ppenum) override;

    // *** IACList methods ***
    STDMETHODIMP Expand(LPCOLESTR pszExpand) override;

    // *** IACLCustomMRU methods ***
    STDMETHODIMP Initialize(LPCWSTR pwszMRURegKey, DWORD dwMax) override;
    STDMETHODIMP AddMRUString(LPCWSTR pwszEntry) override;

public:

    DECLARE_REGISTRY_RESOURCEID(IDR_ACLCUSTOMMRU)
    DECLARE_NOT_AGGREGATABLE(CACLCustomMRU)

    DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_COM_MAP(CACLCustomMRU)
        COM_INTERFACE_ENTRY_IID(IID_IEnumString, IEnumString)
        COM_INTERFACE_ENTRY_IID(IID_IACList, IACList)
        COM_INTERFACE_ENTRY_IID(IID_IACLCustomMRU, IACLCustomMRU)
    END_COM_MAP()
};
