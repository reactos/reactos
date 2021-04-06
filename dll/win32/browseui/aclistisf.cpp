/*
 *  Shell AutoComplete list
 *
 *  Copyright 2015  Thomas Faber
 *  Copyright 2020-2021 Katayama Hirofumi MZ
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
}

CACListISF::~CACListISF()
{
}

HRESULT CACListISF::NextLocation()
{
    TRACE("(%p)\n", this);
    HRESULT hr;
    switch (m_iNextLocation)
    {
        case LT_DIRECTORY:
            m_iNextLocation = LT_DESKTOP;
            if (!ILIsEmpty(m_pidlCurDir) && (m_dwOptions & ACLO_CURRENTDIR))
            {
                CComHeapPtr<ITEMIDLIST> pidl(ILClone(m_pidlCurDir));
                hr = SetLocation(pidl.Detach());
                if (SUCCEEDED(hr))
                {
                    TRACE("LT_DIRECTORY\n");
                    return hr;
                }
            }
            // FALL THROUGH
        case LT_DESKTOP:
            m_iNextLocation = LT_MYCOMPUTER;
            if (m_dwOptions & ACLO_DESKTOP)
            {
                CComHeapPtr<ITEMIDLIST> pidl;
                hr = SHGetSpecialFolderLocation(NULL, CSIDL_DESKTOP, &pidl);
                if (FAILED_UNEXPECTEDLY(hr))
                    return S_FALSE;
                hr = SetLocation(pidl.Detach());
                if (SUCCEEDED(hr))
                {
                    TRACE("LT_DESKTOP\n");
                    return hr;
                }
            }
            // FALL THROUGH
        case LT_MYCOMPUTER:
            m_iNextLocation = LT_FAVORITES;
            if (m_dwOptions & ACLO_MYCOMPUTER)
            {
                CComHeapPtr<ITEMIDLIST> pidl;
                hr = SHGetSpecialFolderLocation(NULL, CSIDL_DRIVES, &pidl);
                if (FAILED_UNEXPECTEDLY(hr))
                    return S_FALSE;
                hr = SetLocation(pidl.Detach());
                if (SUCCEEDED(hr))
                {
                    TRACE("LT_MYCOMPUTER\n");
                    return hr;
                }
            }
            // FALL THROUGH
        case LT_FAVORITES:
            m_iNextLocation = LT_MAX;
            if (m_dwOptions & ACLO_FAVORITES)
            {
                CComHeapPtr<ITEMIDLIST> pidl;
                hr = SHGetSpecialFolderLocation(NULL, CSIDL_FAVORITES, &pidl);
                if (FAILED_UNEXPECTEDLY(hr))
                    return S_FALSE;
                hr = SetLocation(pidl.Detach());
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

HRESULT CACListISF::SetLocation(LPITEMIDLIST pidl)
{
    TRACE("(%p, %p)\n", this, pidl);

    m_pEnumIDList.Release();
    m_pShellFolder.Release();
    m_pidlLocation.Free();

    if (!pidl)
        return E_FAIL;

    m_pidlLocation.Attach(pidl);

    CComPtr<IShellFolder> pFolder;
    HRESULT hr = SHGetDesktopFolder(&pFolder);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    if (!ILIsEmpty(pidl))
    {
        hr = pFolder->BindToObject(pidl, NULL, IID_PPV_ARG(IShellFolder, &m_pShellFolder));
        if (FAILED_UNEXPECTEDLY(hr))
            return hr;
    }
    else
    {
        m_pShellFolder.Attach(pFolder.Detach());
    }

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
    }
    return hr;
}

HRESULT CACListISF::GetDisplayName(LPCITEMIDLIST pidlChild, CComHeapPtr<WCHAR>& pszChild)
{
    TRACE("(%p, %p)\n", this, pidlChild);
    pszChild.Free();

    STRRET StrRet;
    DWORD dwFlags = SHGDN_INFOLDER | SHGDN_FORPARSING | SHGDN_FORADDRESSBAR;
    HRESULT hr = m_pShellFolder->GetDisplayNameOf(pidlChild, dwFlags, &StrRet);
    if (FAILED(hr))
    {
        dwFlags = SHGDN_INFOLDER | SHGDN_FORPARSING;
        hr = m_pShellFolder->GetDisplayNameOf(pidlChild, dwFlags, &StrRet);
        if (FAILED_UNEXPECTEDLY(hr))
            return hr;
    }

    hr = StrRetToStrW(&StrRet, NULL, &pszChild);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    TRACE("pszChild: '%S'\n", static_cast<LPCWSTR>(pszChild));
    return hr;
}

HRESULT
CACListISF::GetPaths(LPCITEMIDLIST pidlChild, CComHeapPtr<WCHAR>& pszRaw,
                     CComHeapPtr<WCHAR>& pszExpanded)
{
    TRACE("(%p, %p)\n", this, pidlChild);

    CComHeapPtr<WCHAR> pszChild;
    HRESULT hr = GetDisplayName(pidlChild, pszChild);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    CStringW szRawPath, szExpanded;
    if (m_szRawPath.GetLength() && m_iNextLocation == LT_DIRECTORY)
    {
        INT cchExpand = m_szRawPath.GetLength();
        if (StrCmpNIW(pszChild, m_szRawPath, cchExpand) != 0 ||
            pszChild[0] != L'\\' || pszChild[1] != L'\\')
        {
            szRawPath = m_szRawPath;
            szExpanded = m_szExpanded;
        }
    }
    szRawPath += pszChild;
    szExpanded += pszChild;

    SHStrDupW(szRawPath, &pszRaw);
    SHStrDupW(szExpanded, &pszExpanded);
    TRACE("pszRaw: '%S'\n", static_cast<LPCWSTR>(pszRaw));
    TRACE("pszExpanded: '%S'\n", static_cast<LPCWSTR>(pszExpanded));
    return S_OK;
}

// *** IEnumString methods ***
STDMETHODIMP CACListISF::Next(ULONG celt, LPOLESTR *rgelt, ULONG *pceltFetched)
{
    TRACE("(%p, %d, %p, %p)\n", this, celt, rgelt, pceltFetched);

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
    CComHeapPtr<WCHAR> pszRawPath, pszExpanded;

    do
    {
        for (;;)
        {
            pidlChild.Free();
            hr = m_pEnumIDList->Next(1, &pidlChild, NULL);
            if (hr != S_OK)
                break;

            pszRawPath.Free();
            pszExpanded.Free();
            GetPaths(pidlChild, pszRawPath, pszExpanded);
            if (!pszRawPath || !pszExpanded)
                continue;

            if ((m_dwOptions & ACLO_FILESYSDIRS) && !PathIsDirectoryW(pszExpanded))
                continue;
            else if ((m_dwOptions & ACLO_FILESYSONLY) && !PathFileExistsW(pszExpanded))
                continue;

            hr = S_OK;
            break;
        }
    } while (hr == S_FALSE && NextLocation() == S_OK);

    if (hr == S_OK)
    {
        *rgelt = pszRawPath.Detach();
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
    TRACE("(%p)\n", this);

    m_iNextLocation = LT_DIRECTORY;
    m_szRawPath = L"";

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
                Initialize(pidl);
        }
        HRESULT hr = SetLocation(pidl.Detach());
        if (FAILED_UNEXPECTEDLY(hr))
            return S_FALSE;
    }
    return S_OK;
}

STDMETHODIMP CACListISF::Skip(ULONG celt)
{
    TRACE("(%p, %d)\n", this, celt);
    return E_NOTIMPL;
}

STDMETHODIMP CACListISF::Clone(IEnumString **ppOut)
{
    TRACE("(%p, %p)\n", this, ppOut);
    *ppOut = NULL;
    return E_NOTIMPL;
}

// *** IACList methods ***
STDMETHODIMP CACListISF::Expand(LPCOLESTR pszExpand)
{
    TRACE("(%p, %ls)\n", this, pszExpand);

    m_szRawPath = pszExpand;
    m_iNextLocation = LT_DIRECTORY;

    // skip left space
    while (*pszExpand == L' ')
        ++pszExpand;

    // expand environment variables (%WINDIR% etc.)
    WCHAR szExpanded[MAX_PATH], szPath1[MAX_PATH], szPath2[MAX_PATH];
    ExpandEnvironmentStringsW(pszExpand, szExpanded, _countof(szExpanded));
    pszExpand = szExpanded;

    // get full path
    if (szExpanded[0] && szExpanded[1] == L':' && szExpanded[2] == 0)
    {
        // 'C:' --> 'C:\'
        szExpanded[2] = L'\\';
        szExpanded[3] = 0;
    }
    else
    {
        if (PathIsRelativeW(pszExpand) &&
            SHGetPathFromIDListW(m_pidlCurDir, szPath1) &&
            PathCombineW(szPath2, szPath1, pszExpand))
        {
            pszExpand = szPath2;
        }
        GetFullPathNameW(pszExpand, _countof(szPath1), szPath1, NULL);
        pszExpand = szPath1;
    }

    CComHeapPtr<ITEMIDLIST> pidl;
    m_szExpanded = pszExpand;
    HRESULT hr = SHParseDisplayName(m_szExpanded, NULL, &pidl, NULL, NULL);
    if (SUCCEEDED(hr))
    {
        hr = SetLocation(pidl.Detach());
        if (FAILED_UNEXPECTEDLY(hr))
        {
            m_szRawPath = L"";
            m_szExpanded = L"";
        }
    }
    return hr;
}

// *** IACList2 methods ***
STDMETHODIMP CACListISF::SetOptions(DWORD dwFlag)
{
    TRACE("(%p, %lu)\n", this, dwFlag);
    m_dwOptions = dwFlag;
    return S_OK;
}

STDMETHODIMP CACListISF::GetOptions(DWORD* pdwFlag)
{
    TRACE("(%p, %p)\n", this, pdwFlag);
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
    TRACE("(%p, %p)\n", this, punkOwner);
    m_pBrowserService.Release();
    punkOwner->QueryInterface(IID_PPV_ARG(IBrowserService, &m_pBrowserService));
    return S_OK;
}

// *** IPersist methods ***
STDMETHODIMP CACListISF::GetClassID(CLSID *pClassID)
{
    TRACE("(%p, %p)\n", this, pClassID);
    if (pClassID == NULL)
        return E_POINTER;
    *pClassID = CLSID_ACListISF;
    return S_OK;
}

// *** IPersistFolder methods ***
STDMETHODIMP CACListISF::Initialize(PCIDLIST_ABSOLUTE pidl)
{
    TRACE("(%p, %p)\n", this, pidl);
    m_pidlCurDir.Free();
    if (!pidl)
        return S_OK;

    LPITEMIDLIST pidlClone = ILClone(pidl);
    if (!pidlClone)
    {
        ERR("Out of memory\n");
        return E_OUTOFMEMORY;
    }
    m_pidlCurDir.Attach(pidlClone);
    return S_OK;
}

// *** ICurrentWorkingDirectory methods ***
STDMETHODIMP CACListISF::GetDirectory(LPWSTR pwzPath, DWORD cchSize)
{
    TRACE("(%p, %p, %ld)\n", this, pwzPath, cchSize);
    return E_NOTIMPL;
}

STDMETHODIMP CACListISF::SetDirectory(LPCWSTR pwzPath)
{
    TRACE("(%p, %ls, %ld)\n", this, pwzPath);
    LPITEMIDLIST pidl = ILCreateFromPathW(pwzPath);
    if (!pidl)
    {
        ERR("Out of memory\n");
        return E_OUTOFMEMORY;
    }
    m_pidlCurDir.Attach(pidl);
    return S_OK;
}
