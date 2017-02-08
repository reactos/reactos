/*
 *  Shell AutoComplete list
 *
 *  Copyright 2015  Thomas Faber
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
    public IShellService,
    public IPersistFolder
{
private:
    DWORD m_dwOptions;

public:
    CACListISF();
    ~CACListISF();

    // *** IEnumString methods ***
    virtual HRESULT STDMETHODCALLTYPE Next(ULONG celt, LPOLESTR *rgelt, ULONG *pceltFetched);
    virtual HRESULT STDMETHODCALLTYPE Skip(ULONG celt);
    virtual HRESULT STDMETHODCALLTYPE Reset();
    virtual HRESULT STDMETHODCALLTYPE Clone(IEnumString **ppenum);

    // *** IACList methods ***
    virtual HRESULT STDMETHODCALLTYPE Expand(LPCOLESTR pszExpand);

    // *** IACList2 methods ***
    virtual HRESULT STDMETHODCALLTYPE SetOptions(DWORD dwFlag);
    virtual HRESULT STDMETHODCALLTYPE GetOptions(DWORD* pdwFlag);

    // *** IShellService methods ***
    virtual HRESULT STDMETHODCALLTYPE SetOwner(IUnknown *);

    // *** IPersist methods ***
    virtual HRESULT STDMETHODCALLTYPE GetClassID(CLSID *pClassID);

    // *** IPersistFolder methods ***
    virtual HRESULT STDMETHODCALLTYPE Initialize(LPCITEMIDLIST pidl);

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
    END_COM_MAP()
};
