/*
 *  Shell AutoComplete list
 *
 *  Copyright 2015  Thomas Faber
 *  Copyright 2020  Katayama Hirofumi MZ
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#pragma once

class CACListISF :
    public CComCoClass<CACListISF, &CLSID_ACListISF>,
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IEnumString,
    public IACList2,
    public ICurrentWorkingDirectory,
    public IShellService,
    public IPersistFolder
{
private:
    enum LOCATION_TYPE
    {
        LT_DIRECTORY,
        LT_DESKTOP,
        LT_MYCOMPUTER,
        LT_FAVORITES,
        LT_MAX
    };

    DWORD m_dwOptions;
    LOCATION_TYPE m_iNextLocation;
    BOOL m_fShowHidden;
    CStringW m_szRawPath;
    CStringW m_szExpanded;
    CComHeapPtr<ITEMIDLIST> m_pidlLocation;
    CComHeapPtr<ITEMIDLIST> m_pidlCurDir;
    CComPtr<IEnumIDList> m_pEnumIDList;
    CComPtr<IShellFolder> m_pShellFolder;
    CComPtr<IBrowserService> m_pBrowserService;

public:
    CACListISF();
    ~CACListISF();

    HRESULT NextLocation();
    HRESULT SetLocation(LPITEMIDLIST pidl);
    HRESULT GetDisplayName(LPCITEMIDLIST pidlChild, CComHeapPtr<WCHAR>& pszChild);
    HRESULT GetPaths(LPCITEMIDLIST pidlChild, CComHeapPtr<WCHAR>& pszRaw,
                     CComHeapPtr<WCHAR>& pszExpanded);

    // *** IEnumString methods ***
    STDMETHODIMP Next(ULONG celt, LPOLESTR *rgelt, ULONG *pceltFetched) override;
    STDMETHODIMP Skip(ULONG celt) override;
    STDMETHODIMP Reset() override;
    STDMETHODIMP Clone(IEnumString **ppenum) override;

    // *** IACList methods ***
    STDMETHODIMP Expand(LPCOLESTR pszExpand) override;

    // *** IACList2 methods ***
    STDMETHODIMP SetOptions(DWORD dwFlag) override;
    STDMETHODIMP GetOptions(DWORD* pdwFlag) override;

    // FIXME: These virtual keywords below should be removed.

    // *** IShellService methods ***
    virtual STDMETHODIMP SetOwner(IUnknown *punkOwner) override;

    // *** IPersist methods ***
    virtual STDMETHODIMP GetClassID(CLSID *pClassID) override;

    // *** IPersistFolder methods ***
    virtual STDMETHODIMP Initialize(PCIDLIST_ABSOLUTE pidl) override;

    // *** ICurrentWorkingDirectory methods ***
    STDMETHODIMP GetDirectory(LPWSTR pwzPath, DWORD cchSize) override;
    STDMETHODIMP SetDirectory(LPCWSTR pwzPath) override;

public:
    DECLARE_REGISTRY_RESOURCEID(IDR_ACLISTISF)
    DECLARE_NOT_AGGREGATABLE(CACListISF)

    DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_COM_MAP(CACListISF)
        COM_INTERFACE_ENTRY_IID(IID_IEnumString, IEnumString)
        COM_INTERFACE_ENTRY_IID(IID_IACList, IACList)
        COM_INTERFACE_ENTRY_IID(IID_IACList2, IACList2)
        COM_INTERFACE_ENTRY_IID(IID_IShellService, IShellService)
        // Windows doesn't return this
        //COM_INTERFACE_ENTRY_IID(IID_IPersist, IPersist)
        COM_INTERFACE_ENTRY_IID(IID_IPersistFolder, IPersistFolder)
        COM_INTERFACE_ENTRY_IID(IID_ICurrentWorkingDirectory, ICurrentWorkingDirectory)
    END_COM_MAP()
};
