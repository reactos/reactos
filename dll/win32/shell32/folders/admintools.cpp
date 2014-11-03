/*
 *    Virtual Admin Tools Folder
 *
 *    Copyright 2008                Johannes Anderwald
 *    Copyright 2009                Andrew Hill
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <precomp.h>

WINE_DEFAULT_DEBUG_CHANNEL (shell);

/*
This folder should not exist. It is just a file system folder...
*/

/* List shortcuts of
 * CSIDL_COMMON_ADMINTOOLS
 * Note: CSIDL_ADMINTOOLS is ignored, tested with Window XP SP3+
 */

/***********************************************************************
 *     AdminTools folder implementation
 */

CAdminToolsFolder::CAdminToolsFolder()
{
    m_pisfInner = NULL;
    m_pisf2Inner = NULL;

    szTarget = NULL;
}

CAdminToolsFolder::~CAdminToolsFolder()
{
    HeapFree(GetProcessHeap(), 0, szTarget);
    m_pisfInner.Release();
    m_pisf2Inner.Release();
}

HRESULT WINAPI CAdminToolsFolder::FinalConstruct()
{
    HRESULT hr;
    CComPtr<IPersistFolder3> ppf3;
    hr = SHCoCreateInstance(NULL, &CLSID_ShellFSFolder, NULL, IID_PPV_ARG(IShellFolder, &m_pisfInner));
    if (FAILED(hr))
        return hr;

    hr = m_pisfInner->QueryInterface(IID_PPV_ARG(IShellFolder2, &m_pisf2Inner));
    if (FAILED(hr))
        return hr;

    hr = m_pisfInner->QueryInterface(IID_PPV_ARG(IPersistFolder3, &ppf3));
    if (FAILED(hr))
        return hr;

    PERSIST_FOLDER_TARGET_INFO info;
    ZeroMemory(&info, sizeof(PERSIST_FOLDER_TARGET_INFO));
    info.csidl = CSIDL_COMMON_ADMINTOOLS;
    hr = ppf3->InitializeEx(NULL, _ILCreateAdminTools(), &info);

    szTarget = (LPWSTR)HeapAlloc(GetProcessHeap(), 0, MAX_PATH * sizeof(WCHAR));
    if (szTarget == NULL)
        return E_OUTOFMEMORY;
    if (!SHGetSpecialFolderPathW(NULL, szTarget, CSIDL_COMMON_ADMINTOOLS, FALSE))
        return E_FAIL;

    return S_OK;
}

HRESULT WINAPI CAdminToolsFolder::ParseDisplayName(HWND hwndOwner, LPBC pbc, LPOLESTR lpszDisplayName,
        ULONG *pchEaten, PIDLIST_RELATIVE *ppidl, ULONG *pdwAttributes)
{
    return m_pisfInner->ParseDisplayName(hwndOwner, pbc, lpszDisplayName, pchEaten, ppidl, pdwAttributes);
}

HRESULT WINAPI CAdminToolsFolder::EnumObjects(HWND hwndOwner, DWORD dwFlags, LPENUMIDLIST *ppEnumIDList)
{
    return m_pisfInner->EnumObjects(hwndOwner, dwFlags, ppEnumIDList);
}

HRESULT WINAPI CAdminToolsFolder::BindToObject(PCUIDLIST_RELATIVE pidl, LPBC pbcReserved, REFIID riid, LPVOID *ppvOut)
{
    return m_pisfInner->BindToObject(pidl, pbcReserved, riid, ppvOut);
}

HRESULT WINAPI CAdminToolsFolder::BindToStorage(PCUIDLIST_RELATIVE pidl, LPBC pbcReserved, REFIID riid, LPVOID *ppvOut)
{
    return m_pisfInner->BindToStorage(pidl, pbcReserved, riid, ppvOut);
}

HRESULT WINAPI CAdminToolsFolder::CompareIDs(LPARAM lParam, PCUIDLIST_RELATIVE pidl1, PCUIDLIST_RELATIVE pidl2)
{
    return m_pisfInner->CompareIDs(lParam, pidl1, pidl2);
}

HRESULT WINAPI CAdminToolsFolder::CreateViewObject(HWND hwndOwner, REFIID riid, LPVOID *ppvOut)
{
    return m_pisfInner->CreateViewObject(hwndOwner, riid, ppvOut);
}

HRESULT WINAPI CAdminToolsFolder::GetAttributesOf(UINT cidl, PCUITEMID_CHILD_ARRAY apidl, DWORD *rgfInOut)
{
    return m_pisfInner->GetAttributesOf(cidl, apidl, rgfInOut);
}

HRESULT WINAPI CAdminToolsFolder::GetUIObjectOf(HWND hwndOwner, UINT cidl, PCUITEMID_CHILD_ARRAY apidl,
        REFIID riid, UINT * prgfInOut, LPVOID * ppvOut)
{
    return m_pisfInner->GetUIObjectOf(hwndOwner, cidl, apidl, riid, prgfInOut, ppvOut);
}

HRESULT WINAPI CAdminToolsFolder::GetDisplayNameOf(PCUITEMID_CHILD pidl, DWORD dwFlags, LPSTRRET strRet)
{
    if (!_ILIsSpecialFolder(pidl))
        return m_pisfInner->GetDisplayNameOf(pidl, dwFlags, strRet);

    HRESULT hr = S_OK;
    LPWSTR pszPath;

    TRACE ("(%p)->(pidl=%p,0x%08x,%p)\n", this, pidl, dwFlags, strRet);
    pdump (pidl);

    if (!strRet)
        return E_INVALIDARG;

    pszPath = (LPWSTR)CoTaskMemAlloc((MAX_PATH + 1) * sizeof(WCHAR));
    if (!pszPath)
        return E_OUTOFMEMORY;

    ZeroMemory(pszPath, (MAX_PATH + 1) * sizeof(WCHAR));

    if (!pidl->mkid.cb)
    {
        if ((GET_SHGDN_RELATION (dwFlags) == SHGDN_NORMAL) &&
                (GET_SHGDN_FOR (dwFlags) & SHGDN_FORPARSING))
            wcscpy(pszPath, szTarget);
        else if (!HCR_GetClassNameW(CLSID_AdminFolderShortcut, pszPath, MAX_PATH))
            hr = E_FAIL;
    }

    if (SUCCEEDED(hr))
    {
        strRet->uType = STRRET_WSTR;
        strRet->pOleStr = pszPath;
        TRACE ("-- (%p)->(%s,0x%08x)\n", this, debugstr_w(strRet->pOleStr), hr);
    }
    else
        CoTaskMemFree(pszPath);

    return hr;
}

HRESULT WINAPI CAdminToolsFolder::SetNameOf(HWND hwndOwner, PCUITEMID_CHILD pidl,    /* simple pidl */
        LPCOLESTR lpName, DWORD dwFlags, PITEMID_CHILD *pPidlOut)
{
    return m_pisfInner->SetNameOf(hwndOwner, pidl, lpName, dwFlags, pPidlOut);
}

HRESULT WINAPI CAdminToolsFolder::GetDefaultSearchGUID(GUID *pguid)
{
    return m_pisf2Inner->GetDefaultSearchGUID(pguid);
}

HRESULT WINAPI CAdminToolsFolder::EnumSearches(IEnumExtraSearch ** ppenum)
{
    return m_pisf2Inner->EnumSearches(ppenum);
}

HRESULT WINAPI CAdminToolsFolder::GetDefaultColumn(DWORD dwRes, ULONG *pSort, ULONG *pDisplay)
{
    return m_pisf2Inner->GetDefaultColumn(dwRes, pSort, pDisplay);
}

HRESULT WINAPI CAdminToolsFolder::GetDefaultColumnState(UINT iColumn, DWORD *pcsFlags)
{
    return m_pisf2Inner->GetDefaultColumnState(iColumn, pcsFlags);
}

HRESULT WINAPI CAdminToolsFolder::GetDetailsEx(PCUITEMID_CHILD pidl, const SHCOLUMNID *pscid, VARIANT *pv)
{
    return m_pisf2Inner->GetDetailsEx(pidl, pscid, pv);
}

HRESULT WINAPI CAdminToolsFolder::GetDetailsOf(PCUITEMID_CHILD pidl, UINT iColumn, SHELLDETAILS *psd)
{
    return m_pisf2Inner->GetDetailsOf(pidl, iColumn, psd);
}

HRESULT WINAPI CAdminToolsFolder::MapColumnToSCID(UINT column, SHCOLUMNID *pscid)
{
    return m_pisf2Inner->MapColumnToSCID(column, pscid);
}

/************************************************************************
 *    CAdminToolsFolder::GetClassID
 */
HRESULT WINAPI CAdminToolsFolder::GetClassID(CLSID *lpClassId)
{
    TRACE ("(%p)\n", this);

    memcpy(lpClassId, &CLSID_AdminFolderShortcut, sizeof(CLSID));

    return S_OK;
}

/************************************************************************
 *    CAdminToolsFolder::Initialize
 *
 */
HRESULT WINAPI CAdminToolsFolder::Initialize(LPCITEMIDLIST pidl)
{
    return S_OK;
}

/**************************************************************************
 *    CAdminToolsFolder::GetCurFolder
 */
HRESULT WINAPI CAdminToolsFolder::GetCurFolder(LPITEMIDLIST *pidl)
{
    CComPtr<IPersistFolder2> ppf2;
    m_pisfInner->QueryInterface(IID_PPV_ARG(IPersistFolder2, &ppf2));
    return ppf2->GetCurFolder(pidl);
}
