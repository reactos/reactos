/*
 * Shell Menu Site
 *
 * Copyright 2014 David Quintana
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
#include "precomp.h"
#include <atlwin.h>
#include <shlwapi_undoc.h>

#include "CMergedFolder.h"

WINE_DEFAULT_DEBUG_CHANNEL(CMergedFolder);

class CEnumMergedFolder :
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IEnumIDList
{
private:
    CComPtr<IEnumIDList> m_UserLocal;
    CComPtr<IEnumIDList> m_AllUSers;
    BOOL m_FirstDone;

public:
    CEnumMergedFolder() : m_UserLocal(NULL), m_AllUSers(NULL), m_FirstDone(FALSE) {}
    ~CEnumMergedFolder() {}

    DECLARE_NOT_AGGREGATABLE(CEnumMergedFolder)
    DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_COM_MAP(CEnumMergedFolder)
        COM_INTERFACE_ENTRY_IID(IID_IEnumIDList, IEnumIDList)
    END_COM_MAP()

    HRESULT Begin(HWND hwndOwner, SHCONTF flags, IShellFolder * userLocal, IShellFolder * allUSers)
    {
        HRESULT hr;
        hr = userLocal->EnumObjects(hwndOwner, flags, &m_UserLocal);
        if (FAILED(hr))
            return hr;
        hr = userLocal->EnumObjects(hwndOwner, flags, &m_AllUSers);
        if (FAILED(hr))
        {
            m_UserLocal = NULL;
            return hr;
        }
        m_FirstDone = FALSE;
        return S_OK;
    }

    virtual HRESULT STDMETHODCALLTYPE Next(
        ULONG celt,
        LPITEMIDLIST *rgelt,
        ULONG *pceltFetched)
    {
        HRESULT hr;

        *pceltFetched = 0;

        if (!m_FirstDone)
        {
            hr = m_UserLocal->Next(celt, rgelt, pceltFetched);
            if (FAILED(hr))
                return hr;
            if (hr == S_FALSE)
                m_FirstDone = true;
            if (celt < 2)
                return hr;
        }

        DWORD offset = *pceltFetched;
        if (*pceltFetched < celt)
        {
            rgelt += *pceltFetched;
            celt = (*pceltFetched - celt);
            *pceltFetched = 0;
        }

        hr = m_UserLocal->Next(celt, rgelt, pceltFetched);
        if (FAILED(hr))
            return hr;

        *pceltFetched += offset;
        return hr;
    }

    virtual HRESULT STDMETHODCALLTYPE Skip(
        ULONG celt)
    {
        UNIMPLEMENTED;
        return E_NOTIMPL;
    }

    virtual HRESULT STDMETHODCALLTYPE Reset(
        )
    {
        if (m_FirstDone)
            m_AllUSers->Reset();
        return m_UserLocal->Reset();
    }

    virtual HRESULT STDMETHODCALLTYPE Clone(
        IEnumIDList **ppenum)
    {
        UNIMPLEMENTED;
        return E_NOTIMPL;
    }
};

extern "C"
HRESULT CMergedFolder_Constructor(IShellFolder* userLocal, IShellFolder* allUsers, REFIID riid, LPVOID *ppv)
{
    *ppv = NULL;

    CMergedFolder * fld = new CComObject<CMergedFolder>();

    if (!fld)
        return E_OUTOFMEMORY;

    HRESULT hr;

    hr = fld->_SetSources(userLocal, allUsers);

    hr = fld->QueryInterface(riid, ppv);
    if (FAILED(hr))
        fld->Release();

    return hr;
}

HRESULT CMergedFolder::_SetSources(IShellFolder* userLocal, IShellFolder* allUsers)
{
    m_UserLocal = userLocal;
    m_AllUSers = allUsers;
    return S_OK;
}

// IShellFolder
HRESULT STDMETHODCALLTYPE CMergedFolder::ParseDisplayName(
    HWND hwndOwner,
    LPBC pbcReserved,
    LPOLESTR lpszDisplayName,
    ULONG *pchEaten,
    LPITEMIDLIST *ppidl,
    ULONG *pdwAttributes)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CMergedFolder::EnumObjects(
    HWND hwndOwner,
    SHCONTF grfFlags,
    IEnumIDList **ppenumIDList)
{
    CEnumMergedFolder * merged = new CComObject<CEnumMergedFolder>();
    *ppenumIDList = merged;
    return merged->Begin(hwndOwner, grfFlags, m_UserLocal, m_AllUSers);
}

HRESULT STDMETHODCALLTYPE CMergedFolder::BindToObject(
    LPCITEMIDLIST pidl,
    LPBC pbcReserved,
    REFIID riid,
    void **ppvOut)
{
    HRESULT hr;

    hr = m_UserLocal->BindToObject(pidl, pbcReserved, riid, ppvOut);
    if (SUCCEEDED(hr))
        return hr;

    hr = m_AllUSers->BindToObject(pidl, pbcReserved, riid, ppvOut);

    return hr;
}

HRESULT STDMETHODCALLTYPE CMergedFolder::BindToStorage(
    LPCITEMIDLIST pidl,
    LPBC pbcReserved,
    REFIID riid,
    void **ppvObj)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CMergedFolder::CompareIDs(
    LPARAM lParam,
    LPCITEMIDLIST pidl1,
    LPCITEMIDLIST pidl2)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CMergedFolder::CreateViewObject(
    HWND hwndOwner,
    REFIID riid,
    void **ppvOut)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CMergedFolder::GetAttributesOf(
    UINT cidl,
    LPCITEMIDLIST *apidl,
    SFGAOF *rgfInOut)
{
    HRESULT hr;

    hr = m_UserLocal->GetAttributesOf(cidl, apidl, rgfInOut);
    if (SUCCEEDED(hr))
        return hr;

    hr = m_AllUSers->GetAttributesOf(cidl, apidl, rgfInOut);

    return hr;
}

HRESULT STDMETHODCALLTYPE CMergedFolder::GetUIObjectOf(
    HWND hwndOwner,
    UINT cidl,
    LPCITEMIDLIST *apidl,
    REFIID riid,
    UINT *prgfInOut,
    void **ppvOut)
{
    HRESULT hr;

    hr = m_UserLocal->GetUIObjectOf(hwndOwner, cidl, apidl, riid, prgfInOut, ppvOut);
    if (SUCCEEDED(hr))
        return hr;

    hr = m_AllUSers->GetUIObjectOf(hwndOwner, cidl, apidl, riid, prgfInOut, ppvOut);

    return hr;
}

HRESULT STDMETHODCALLTYPE CMergedFolder::GetDisplayNameOf(
    LPCITEMIDLIST pidl,
    SHGDNF uFlags,
    STRRET *lpName)
{
    HRESULT hr;

    hr = m_UserLocal->GetDisplayNameOf(pidl, uFlags, lpName);
    if (SUCCEEDED(hr))
        return hr;

    hr = m_AllUSers->GetDisplayNameOf(pidl, uFlags, lpName);

    return hr;
}

HRESULT STDMETHODCALLTYPE CMergedFolder::SetNameOf(
    HWND hwnd,
    LPCITEMIDLIST pidl,
    LPCOLESTR lpszName,
    SHGDNF uFlags,
    LPITEMIDLIST *ppidlOut)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

// IShellFolder2
HRESULT STDMETHODCALLTYPE CMergedFolder::GetDefaultSearchGUID(
    GUID *lpguid)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CMergedFolder::EnumSearches(
    IEnumExtraSearch **ppenum)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CMergedFolder::GetDefaultColumn(
    DWORD dwReserved,
    ULONG *pSort,
    ULONG *pDisplay)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CMergedFolder::GetDefaultColumnState(
    UINT iColumn,
    SHCOLSTATEF *pcsFlags)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CMergedFolder::GetDetailsEx(
    LPCITEMIDLIST pidl,
    const SHCOLUMNID *pscid,
    VARIANT *pv)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CMergedFolder::GetDetailsOf(
    LPCITEMIDLIST pidl,
    UINT iColumn,
    SHELLDETAILS *psd)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CMergedFolder::MapColumnToSCID(
    UINT iColumn,
    SHCOLUMNID *pscid)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}