/*
 *    Virtual MyDocuments Folder
 *
 *    Copyright 2007    Johannes Anderwald
 *    Copyright 2009    Andrew Hill
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

#include <precomp.h>

WINE_DEFAULT_DEBUG_CHANNEL (mydocs);

CMyDocsFolder::CMyDocsFolder()
{
    m_pidlInner = NULL;
}

CMyDocsFolder::~CMyDocsFolder()
{
    if(m_pidlInner)
        SHFree(m_pidlInner);
}

HRESULT WINAPI CMyDocsFolder::FinalConstruct()
{
    m_pidlInner = _ILCreateMyDocuments();

    if (!m_pidlInner)
        return E_OUTOFMEMORY;

    return S_OK;
}

HRESULT CMyDocsFolder::EnsureFolder()
{
    ATLASSERT(m_pidlInner);

    if (m_pisfInner)
        return S_OK;


    HRESULT hr = SHELL32_CoCreateInitSF(m_pidlInner,
                                        &CLSID_ShellFSFolder,
                                        CSIDL_PERSONAL,
                                        IID_PPV_ARG(IShellFolder2, &m_pisfInner));

    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    return S_OK;
}

HRESULT WINAPI CMyDocsFolder::ParseDisplayName(HWND hwndOwner, LPBC pbc, LPOLESTR lpszDisplayName,
        ULONG *pchEaten, PIDLIST_RELATIVE *ppidl, ULONG *pdwAttributes)
{
    HRESULT hr = EnsureFolder();
    if (FAILED(hr))
        return hr;

    return m_pisfInner->ParseDisplayName(hwndOwner, pbc, lpszDisplayName, pchEaten, ppidl, pdwAttributes);
}

HRESULT WINAPI CMyDocsFolder::EnumObjects(HWND hwndOwner, DWORD dwFlags, LPENUMIDLIST *ppEnumIDList)
{
    HRESULT hr = EnsureFolder();
    if (FAILED(hr))
        return hr;

    return m_pisfInner->EnumObjects(hwndOwner, dwFlags, ppEnumIDList);
}

HRESULT WINAPI CMyDocsFolder::BindToObject(PCUIDLIST_RELATIVE pidl, LPBC pbcReserved, REFIID riid, LPVOID *ppvOut)
{
    HRESULT hr = EnsureFolder();
    if (FAILED(hr))
        return hr;

    return m_pisfInner->BindToObject(pidl, pbcReserved, riid, ppvOut);
}

HRESULT WINAPI CMyDocsFolder::BindToStorage(PCUIDLIST_RELATIVE pidl, LPBC pbcReserved, REFIID riid, LPVOID *ppvOut)
{
    HRESULT hr = EnsureFolder();
    if (FAILED(hr))
        return hr;

    return m_pisfInner->BindToStorage(pidl, pbcReserved, riid, ppvOut);
}

HRESULT WINAPI CMyDocsFolder::CompareIDs(LPARAM lParam, PCUIDLIST_RELATIVE pidl1, PCUIDLIST_RELATIVE pidl2)
{
    HRESULT hr = EnsureFolder();
    if (FAILED(hr))
        return hr;

    return m_pisfInner->CompareIDs(lParam, pidl1, pidl2);
}

HRESULT WINAPI CMyDocsFolder::CreateViewObject(HWND hwndOwner, REFIID riid, LPVOID *ppvOut)
{
    HRESULT hr = EnsureFolder();
    if (FAILED(hr))
        return hr;

    return m_pisfInner->CreateViewObject(hwndOwner, riid, ppvOut);
}

HRESULT WINAPI CMyDocsFolder::GetAttributesOf(UINT cidl, PCUITEMID_CHILD_ARRAY apidl, DWORD *rgfInOut)
{
    static const DWORD dwMyDocumentsAttributes =
        SFGAO_STORAGE | SFGAO_HASPROPSHEET | SFGAO_STORAGEANCESTOR | SFGAO_CANCOPY |
        SFGAO_FILESYSANCESTOR | SFGAO_FOLDER | SFGAO_FILESYSTEM | SFGAO_HASSUBFOLDER | SFGAO_CANRENAME | SFGAO_CANDELETE;

    HRESULT hr = EnsureFolder();
    if (FAILED(hr))
        return hr;

    if(cidl)
        return m_pisfInner->GetAttributesOf(cidl, apidl, rgfInOut);

    if (!rgfInOut)
        return E_INVALIDARG;

    if (*rgfInOut == 0)
        *rgfInOut = ~0;

    *rgfInOut &= dwMyDocumentsAttributes;

    /* make sure SFGAO_VALIDATE is cleared, some apps depend on that */
    *rgfInOut &= ~SFGAO_VALIDATE;

    return S_OK;
}

HRESULT WINAPI CMyDocsFolder::GetUIObjectOf(HWND hwndOwner, UINT cidl, PCUITEMID_CHILD_ARRAY apidl,
        REFIID riid, UINT * prgfInOut, LPVOID * ppvOut)
{
    HRESULT hr = EnsureFolder();
    if (FAILED(hr))
        return hr;

    return m_pisfInner->GetUIObjectOf(hwndOwner, cidl, apidl, riid, prgfInOut, ppvOut);
}

HRESULT WINAPI CMyDocsFolder::GetDisplayNameOf(PCUITEMID_CHILD pidl, DWORD dwFlags, LPSTRRET strRet)
{
    if (!strRet || !pidl)
        return E_INVALIDARG;

    HRESULT hr = EnsureFolder();
    if (FAILED(hr))
        return hr;

    /* If we got an fs item just forward to the fs folder */
    if (!_ILIsSpecialFolder(pidl))
        return m_pisfInner->GetDisplayNameOf(pidl, dwFlags, strRet);

    /* The caller wants our path. Let fs folder handle it */
    if ((GET_SHGDN_RELATION (dwFlags) == SHGDN_NORMAL) &&
        (GET_SHGDN_FOR (dwFlags) & SHGDN_FORPARSING))
    {
        /* Give an empty pidl to the fs folder to make it tell us its path */
        if (pidl->mkid.cb)
            pidl = ILGetNext(pidl);
        return m_pisfInner->GetDisplayNameOf(pidl, dwFlags, strRet);
    }

    ERR("Got empty pidl without SHGDN_FORPARSING\n");
    return E_INVALIDARG;
}

HRESULT WINAPI CMyDocsFolder::SetNameOf(HWND hwndOwner, PCUITEMID_CHILD pidl,    /* simple pidl */
        LPCOLESTR lpName, DWORD dwFlags, PITEMID_CHILD *pPidlOut)
{
    HRESULT hr = EnsureFolder();
    if (FAILED(hr))
        return hr;

    return m_pisfInner->SetNameOf(hwndOwner, pidl, lpName, dwFlags, pPidlOut);
}

HRESULT WINAPI CMyDocsFolder::GetDefaultSearchGUID(GUID *pguid)
{
    HRESULT hr = EnsureFolder();
    if (FAILED(hr))
        return hr;

    return m_pisfInner->GetDefaultSearchGUID(pguid);
}

HRESULT WINAPI CMyDocsFolder::EnumSearches(IEnumExtraSearch ** ppenum)
{
    HRESULT hr = EnsureFolder();
    if (FAILED(hr))
        return hr;

    return m_pisfInner->EnumSearches(ppenum);
}

HRESULT WINAPI CMyDocsFolder::GetDefaultColumn(DWORD dwRes, ULONG *pSort, ULONG *pDisplay)
{
    HRESULT hr = EnsureFolder();
    if (FAILED(hr))
        return hr;

    return m_pisfInner->GetDefaultColumn(dwRes, pSort, pDisplay);
}

HRESULT WINAPI CMyDocsFolder::GetDefaultColumnState(UINT iColumn, DWORD *pcsFlags)
{
    HRESULT hr = EnsureFolder();
    if (FAILED(hr))
        return hr;

    return m_pisfInner->GetDefaultColumnState(iColumn, pcsFlags);
}

HRESULT WINAPI CMyDocsFolder::GetDetailsEx(PCUITEMID_CHILD pidl, const SHCOLUMNID *pscid, VARIANT *pv)
{
    HRESULT hr = EnsureFolder();
    if (FAILED(hr))
        return hr;

    return m_pisfInner->GetDetailsEx(pidl, pscid, pv);
}

HRESULT WINAPI CMyDocsFolder::GetDetailsOf(PCUITEMID_CHILD pidl, UINT iColumn, SHELLDETAILS *psd)
{
    HRESULT hr = EnsureFolder();
    if (FAILED(hr))
        return hr;

    return m_pisfInner->GetDetailsOf(pidl, iColumn, psd);
}

HRESULT WINAPI CMyDocsFolder::MapColumnToSCID(UINT column, SHCOLUMNID *pscid)
{
    HRESULT hr = EnsureFolder();
    if (FAILED(hr))
        return hr;

    return m_pisfInner->MapColumnToSCID(column, pscid);
}

HRESULT WINAPI CMyDocsFolder::GetClassID(CLSID *lpClassId)
{
    if (!lpClassId)
        return E_POINTER;

    memcpy(lpClassId, &CLSID_MyDocuments, sizeof(GUID));

    return S_OK;
}

HRESULT WINAPI CMyDocsFolder::Initialize(PCIDLIST_ABSOLUTE pidl)
{
    if (m_pisfInner)
        return E_INVALIDARG;

    if (m_pidlInner)
        SHFree(m_pidlInner);

    m_pidlInner = ILClone(pidl);

    if (!m_pidlInner)
        return E_OUTOFMEMORY;

    return EnsureFolder();
}

HRESULT WINAPI CMyDocsFolder::GetCurFolder(PIDLIST_ABSOLUTE *pidl)
{
    if (!pidl)
        return E_POINTER;
    *pidl = ILClone(m_pidlInner);
    return S_OK;
}
