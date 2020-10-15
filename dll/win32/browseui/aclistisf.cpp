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

#include "precomp.h"

CACListISF::CACListISF()
    : m_dwOptions(ACLO_CURRENTDIR | ACLO_MYCOMPUTER)
    , m_iNextLocation(LT_DIRECTORY)
    , m_fShowHidden(FALSE)
{
    m_szExpand[0] = 0;
}

CACListISF::~CACListISF()
{
}

HRESULT CACListISF::NextLocation()
{
    TRACE("NextLocation(%p)\n", this);
    HRESULT hr;
    switch (m_iNextLocation)
    {
        case LT_DIRECTORY:
            ++m_iNextLocation;
            if (!ILIsEmpty(m_pidlCurDir) && (m_dwOptions & ACLO_CURRENTDIR))
            {
                hr = SetLocation(m_pidlCurDir);
                if (SUCCEEDED(hr))
                {
                    TRACE("LT_DIRECTORY\n");
                    return hr;
                }
            }
            // FALL THROUGH
        case LT_DESKTOP:
            ++m_iNextLocation;
            if (m_dwOptions & ACLO_DESKTOP)
            {
                CComHeapPtr<ITEMIDLIST> pidl;
                hr = SHGetSpecialFolderLocation(NULL, CSIDL_DESKTOP, &pidl);
                if (FAILED(hr))
                    return S_FALSE;
                hr = SetLocation(pidl);
                if (SUCCEEDED(hr))
                {
                    TRACE("LT_DESKTOP\n");
                    return hr;
                }
            }
            // FALL THROUGH
        case LT_MYCOMPUTER:
            ++m_iNextLocation;
            if (m_dwOptions & ACLO_MYCOMPUTER)
            {
                CComHeapPtr<ITEMIDLIST> pidl;
                hr = SHGetSpecialFolderLocation(NULL, CSIDL_DRIVES, &pidl);
                if (FAILED(hr))
                    return S_FALSE;
                hr = SetLocation(pidl);
                if (SUCCEEDED(hr))
                {
                    TRACE("LT_MYCOMPUTER\n");
                    return hr;
                }
            }
            // FALL THROUGH
        case LT_FAVORITES:
            ++m_iNextLocation;
            if (m_dwOptions & ACLO_FAVORITES)
            {
                CComHeapPtr<ITEMIDLIST> pidl;
                hr = SHGetSpecialFolderLocation(NULL, CSIDL_FAVORITES, &pidl);
                if (FAILED(hr))
                    return S_FALSE;
                hr = SetLocation(pidl);
                if (SUCCEEDED(hr))
                {
                    TRACE("LT_FAVORITES\n");
                    return hr;
                }
            }
            // FALL THROUGH
        case LT_MAX:
        default:
            TRACE("LT_MAX\n");
            return S_FALSE;
    }
}

HRESULT CACListISF::SetLocation(LPCITEMIDLIST pidl)
{
    TRACE("SetLocation(%p, %p)\n", this, pidl);

    m_pEnumIDList.Release();
    m_pShellFolder.Release();
    m_pidlLocation.Free();

    if (!pidl)
        return E_FAIL;

    LPITEMIDLIST pidlClone = ILClone(pidl);
    if (!pidlClone)
    {
        ERR("Out of memory\n");
        return E_OUTOFMEMORY;
    }
    m_pidlLocation.Attach(pidlClone);

    CComPtr<IShellFolder> pFolder;
    HRESULT hr = SHGetDesktopFolder(&pFolder);
    if (FAILED(hr))
    {
        ERR("SHGetDesktopFolder failed: 0x%lX\n", hr);
        return hr;
    }

    if (!ILIsEmpty(pidl))
        pFolder->BindToObject(pidl, NULL, IID_PPV_ARG(IShellFolder, &m_pShellFolder));
    else
        m_pShellFolder.Attach(pFolder.Detach());

    SHCONTF Flags = SHCONTF_FOLDERS | SHCONTF_INIT_ON_FIRST_NEXT;
    if (m_fShowHidden)
        Flags |= SHCONTF_INCLUDEHIDDEN;
    if (!(m_dwOptions & ACLO_FILESYSDIRS))
        Flags |= SHCONTF_NONFOLDERS;

    hr = m_pShellFolder->EnumObjects(NULL, Flags, &m_pEnumIDList);
    if (hr != S_OK)
    {
        ERR("EnumObjects failed: 0x%lX\n", hr);
        hr = E_FAIL;
        return hr;
    }
    return hr;
}

HRESULT CACListISF::GetDisplayName(LPCITEMIDLIST pidlChild, CComHeapPtr<WCHAR>& pszChild)
{
    TRACE("GetDisplayName(%p, %p)\n", this, pidlChild);
    HRESULT hr;
    STRRET StrRet;
    pszChild.Free();

    DWORD dwFlags = SHGDN_INFOLDER | SHGDN_FORPARSING | SHGDN_FORADDRESSBAR;
    hr = m_pShellFolder->GetDisplayNameOf(pidlChild, dwFlags, &StrRet);
    if (FAILED(hr))
    {
        dwFlags = SHGDN_INFOLDER | SHGDN_FORPARSING;
        hr = m_pShellFolder->GetDisplayNameOf(pidlChild, dwFlags, &StrRet);
        if (FAILED(hr))
        {
            ERR("m_pShellFolder->GetDisplayNameOf failed: 0x%lX\n", hr);
            return hr;
        }
    }

    hr = StrRetToStrW(&StrRet, NULL, &pszChild);
    if (FAILED(hr))
    {
        ERR("StrRetToStrW failed: 0x%lX\n", hr);
        return hr;
    }
    TRACE("GetDisplayName: '%S'\n", static_cast<LPCWSTR>(pszChild));
    return hr;
}

HRESULT CACListISF::GetPathName(LPCITEMIDLIST pidlChild, CComHeapPtr<WCHAR>& pszPath)
{
    TRACE("GetPathName(%p, %p)\n", this, pidlChild);

    CComHeapPtr<WCHAR> pszChild;
    HRESULT hr = GetDisplayName(pidlChild, pszChild);
    if (FAILED(hr))
    {
        ERR("GetDisplayName failed: 0x%lX\n", hr);
        return hr;
    }

    WCHAR szPath[MAX_PATH];
    szPath[0] = 0;
    if (m_szExpand[0] && m_iNextLocation == LT_DIRECTORY)
    {
        size_t cchExpand = wcslen(m_szExpand);
        if (StrCmpNIW(pszChild, m_szExpand, INT(cchExpand)) != 0 ||
            pszChild[0] != L'\\' || pszChild[1] != L'\\')
        {
            StringCchCopyW(szPath, MAX_PATH, m_szExpand);
        }
    }
    StringCchCatW(szPath, MAX_PATH, pszChild);

    size_t cchMax = wcslen(szPath) + 1;
    CComHeapPtr<WCHAR> pszFullPath;
    if (!pszFullPath.Allocate(cchMax))
    {
        ERR("Out of memory\n");
        return E_OUTOFMEMORY;
    }

    StringCchCopyW(pszFullPath, cchMax, szPath);
    pszPath.Attach(pszFullPath.Detach());
    TRACE("GetPathName: '%S'\n", static_cast<LPCWSTR>(pszPath));
    return S_OK;
}

// *** IEnumString methods ***
STDMETHODIMP CACListISF::Next(ULONG celt, LPOLESTR *rgelt, ULONG *pceltFetched)
{
    TRACE("Next(%p, %d, %p, %p)\n", this, celt, rgelt, pceltFetched);

    if (celt == 0)
        return S_OK;
    if (!rgelt)
        return S_FALSE;

    *rgelt = NULL;
    if (pceltFetched)
        *pceltFetched = 0;

    if (!m_pEnumIDList)
    {
        NextLocation();
        if (!m_pEnumIDList)
            return S_FALSE;
    }

    HRESULT hr;
    CComHeapPtr<ITEMIDLIST> pidlChild;
    CComHeapPtr<WCHAR> pszPathName;
    WCHAR szPath[MAX_PATH];
    ULONG cGot;

    do
    {
        do
        {
            pidlChild.Free();
            hr = m_pEnumIDList->Next(1, &pidlChild, &cGot);
            if (hr != S_OK)
                break;

            pszPathName.Free();
            GetPathName(pidlChild, pszPathName);
            if (pszPathName)
            {
                StringCbCopyW(szPath, sizeof(szPath), pszPathName);
                PathAddBackslashW(szPath);
                size_t cch1 = wcslen(m_szExpand), cch2 = wcslen(szPath);
                if (cch1 <= cch2)
                {
                    szPath[cch1] = 0;
                    if (_wcsicmp(szPath, m_szExpand) != 0)
                        continue;

                    hr = S_OK;
                    break;
                }
            }
            hr = E_FAIL;
        } while (hr != S_OK);
    } while (hr == S_FALSE && NextLocation() == S_OK);

    if (hr == S_OK)
    {
        *rgelt = pszPathName.Detach();
        if (pceltFetched)
            *pceltFetched = 1;
    }
    else
    {
        hr = S_FALSE;
    }

    TRACE("*rgelt: %S\n", *rgelt);
    return hr;
}

STDMETHODIMP CACListISF::Reset()
{
    TRACE("Reset(%p)\n", this);

    m_iNextLocation = LT_DIRECTORY;
    m_szExpand[0] = 0;

    SHELLSTATE ss = { 0 };
    SHGetSetSettings(&ss, SSF_SHOWALLOBJECTS, FALSE);
    m_fShowHidden = ss.fShowAllObjects;

    if (m_dwOptions & ACLO_CURRENTDIR)
    {
        CComHeapPtr<ITEMIDLIST> pidl;
        if (m_pBrowserService)
        {
            m_pBrowserService->GetPidl(&pidl);
            if (pidl)
                m_pidlCurDir.Attach(pidl.Detach());
        }
        HRESULT hr = SetLocation(pidl);
        if (FAILED(hr))
            return S_FALSE;
    }
    return S_OK;
}

STDMETHODIMP CACListISF::Skip(ULONG celt)
{
    TRACE("Skip(%p, %d)\n", this, celt);
    return E_NOTIMPL;
}

STDMETHODIMP CACListISF::Clone(IEnumString **ppOut)
{
    TRACE("Clone(%p, %p)\n", this, ppOut);
    *ppOut = NULL;
    return E_NOTIMPL;
}

// *** IACList methods ***
STDMETHODIMP CACListISF::Expand(LPCOLESTR pszExpand)
{
    TRACE("Expand(%p, %ls)\n", this, pszExpand);

    StringCbCopyW(m_szExpand, sizeof(m_szExpand), pszExpand);

    m_iNextLocation = LT_DIRECTORY;
    CComHeapPtr<ITEMIDLIST> pidl;
    HRESULT hr = SHParseDisplayName(m_szExpand, NULL, &pidl, NULL, NULL);
    if (SUCCEEDED(hr))
    {
        hr = SetLocation(pidl);
        if (FAILED(hr))
        {
            m_szExpand[0] = 0;
        }
    }
    return hr;
}

// *** IACList2 methods ***
STDMETHODIMP CACListISF::SetOptions(DWORD dwFlag)
{
    TRACE("SetOptions(%p, %lu)\n", this, dwFlag);
    m_dwOptions = dwFlag;
    return S_OK;
}

STDMETHODIMP CACListISF::GetOptions(DWORD* pdwFlag)
{
    TRACE("GetOptions(%p, %p)\n", this, pdwFlag);
    if (pdwFlag)
    {
        *pdwFlag = m_dwOptions;
        return S_OK;
    }
    return E_INVALIDARG;
}

// *** IShellService methods ***
STDMETHODIMP CACListISF::SetOwner(IUnknown *punkOwner)
{
    TRACE("SetOwner(%p, %p)\n", this, punkOwner);
    m_pBrowserService.Release();
    punkOwner->QueryInterface(IID_PPV_ARG(IBrowserService, &m_pBrowserService));
    return S_OK;
}

// *** IPersist methods ***
STDMETHODIMP CACListISF::GetClassID(CLSID *pClassID)
{
    TRACE("GetClassID(%p, %p)\n", this, pClassID);
    if (pClassID == NULL)
        return E_POINTER;
    *pClassID = CLSID_ACListISF;
    return S_OK;
}

// *** IPersistFolder methods ***
STDMETHODIMP CACListISF::Initialize(PCIDLIST_ABSOLUTE pidl)
{
    TRACE("Initialize(%p, %p)\n", this, pidl);
    m_pidlCurDir.Attach(ILClone(pidl));
    return Reset();
}

// *** ICurrentWorkingDirectory methods ***
STDMETHODIMP CACListISF::GetDirectory(LPWSTR pwzPath, DWORD cchSize)
{
    TRACE("GetDirectory(%p, %p, %ld)\n", this, pwzPath, cchSize);
    return E_NOTIMPL;
}

STDMETHODIMP CACListISF::SetDirectory(LPCWSTR pwzPath)
{
    TRACE("SetDirectory(%p, %ls, %ld)\n", this, pwzPath);
    LPITEMIDLIST pidl = ILCreateFromPathW(pwzPath);
    if (pidl)
    {
        m_pidlCurDir.Attach(pidl);
        return S_OK;
    }
    return E_OUTOFMEMORY;
}
