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

/*
CFileSysEnumX should not exist. CMyDocsFolder should aggregate a CFSFolder which always
maps the contents of CSIDL_PERSONAL. Therefore, CMyDocsFolder::EnumObjects simply calls
CFSFolder::EnumObjects.
*/

/***********************************************************************
*     MyDocumentsfolder implementation
*/

CMyDocsFolder::CMyDocsFolder()
{
    m_pisfInner = NULL;
    m_pisf2Inner = NULL;
    pidlRoot = NULL;
    sPathTarget = NULL;
}

CMyDocsFolder::~CMyDocsFolder()
{
    SHFree(pidlRoot);
    if (sPathTarget)
        HeapFree(GetProcessHeap(), 0, sPathTarget);
    m_pisfInner.Release();
    m_pisf2Inner.Release();
}

HRESULT WINAPI CMyDocsFolder::FinalConstruct()
{
    WCHAR szMyPath[MAX_PATH];

    if (!SHGetSpecialFolderPathW(0, szMyPath, CSIDL_PERSONAL, TRUE))
        return E_UNEXPECTED;

    pidlRoot = _ILCreateMyDocuments();    /* my qualified pidl */
    sPathTarget = (LPWSTR)SHAlloc((wcslen(szMyPath) + 1) * sizeof(WCHAR));
    wcscpy(sPathTarget, szMyPath);

    WCHAR szPath[MAX_PATH];
    lstrcpynW(szPath, sPathTarget, MAX_PATH);

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
    info.csidl = CSIDL_PERSONAL;
    hr = ppf3->InitializeEx(NULL, pidlRoot, &info);

    return hr;
}

HRESULT WINAPI CMyDocsFolder::ParseDisplayName(HWND hwndOwner, LPBC pbc, LPOLESTR lpszDisplayName,
        ULONG *pchEaten, PIDLIST_RELATIVE *ppidl, ULONG *pdwAttributes)
{
    return m_pisfInner->ParseDisplayName(hwndOwner, pbc, lpszDisplayName, pchEaten, ppidl, pdwAttributes);
}

HRESULT WINAPI CMyDocsFolder::EnumObjects(HWND hwndOwner, DWORD dwFlags, LPENUMIDLIST *ppEnumIDList)
{
    return m_pisfInner->EnumObjects(hwndOwner, dwFlags, ppEnumIDList);
}

HRESULT WINAPI CMyDocsFolder::BindToObject(PCUIDLIST_RELATIVE pidl, LPBC pbcReserved, REFIID riid, LPVOID *ppvOut)
{
    return m_pisfInner->BindToObject(pidl, pbcReserved, riid, ppvOut);
}

HRESULT WINAPI CMyDocsFolder::BindToStorage(PCUIDLIST_RELATIVE pidl, LPBC pbcReserved, REFIID riid, LPVOID *ppvOut)
{
    return m_pisfInner->BindToStorage(pidl, pbcReserved, riid, ppvOut);
}

HRESULT WINAPI CMyDocsFolder::CompareIDs(LPARAM lParam, PCUIDLIST_RELATIVE pidl1, PCUIDLIST_RELATIVE pidl2)
{
    return m_pisfInner->CompareIDs(lParam, pidl1, pidl2);
}

HRESULT WINAPI CMyDocsFolder::CreateViewObject(HWND hwndOwner, REFIID riid, LPVOID *ppvOut)
{
    return m_pisfInner->CreateViewObject(hwndOwner, riid, ppvOut);
}

HRESULT WINAPI CMyDocsFolder::GetAttributesOf(UINT cidl, PCUITEMID_CHILD_ARRAY apidl, DWORD *rgfInOut)
{
    static const DWORD dwMyDocumentsAttributes =
        SFGAO_STORAGE | SFGAO_HASPROPSHEET | SFGAO_STORAGEANCESTOR | SFGAO_CANCOPY |
        SFGAO_FILESYSANCESTOR | SFGAO_FOLDER | SFGAO_FILESYSTEM | SFGAO_HASSUBFOLDER | SFGAO_CANRENAME | SFGAO_CANDELETE;

    if(cidl)
    {
        return m_pisfInner->GetAttributesOf(cidl, apidl, rgfInOut);
    }
    else
    {
        if (!rgfInOut)
            return E_INVALIDARG;
        if (cidl && !apidl)
            return E_INVALIDARG;

        if (*rgfInOut == 0)
            *rgfInOut = ~0;

        *rgfInOut &= dwMyDocumentsAttributes;

        /* make sure SFGAO_VALIDATE is cleared, some apps depend on that */
        *rgfInOut &= ~SFGAO_VALIDATE;

        return S_OK;
    }
}

HRESULT WINAPI CMyDocsFolder::GetUIObjectOf(HWND hwndOwner, UINT cidl, PCUITEMID_CHILD_ARRAY apidl,
        REFIID riid, UINT * prgfInOut, LPVOID * ppvOut)
{
    return m_pisfInner->GetUIObjectOf(hwndOwner, cidl, apidl, riid, prgfInOut, ppvOut);
}

HRESULT WINAPI CMyDocsFolder::GetDisplayNameOf(PCUITEMID_CHILD pidl, DWORD dwFlags, LPSTRRET strRet)
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

    if (_ILIsMyDocuments (pidl) || !pidl->mkid.cb)
    {
        if ((GET_SHGDN_RELATION (dwFlags) == SHGDN_NORMAL) &&
                (GET_SHGDN_FOR (dwFlags) & SHGDN_FORPARSING))
            wcscpy(pszPath, sPathTarget);
        else
            HCR_GetClassNameW(CLSID_MyDocuments, pszPath, MAX_PATH);
        TRACE("CP\n");
    }
    else 
    {
        hr = E_INVALIDARG;
    }

    if (SUCCEEDED(hr))
    {
        strRet->uType = STRRET_WSTR;
        strRet->pOleStr = pszPath;
    }
    else
        CoTaskMemFree(pszPath);

    TRACE ("-- (%p)->(%s,0x%08x)\n", this, debugstr_w(strRet->pOleStr), hr);
    return hr;
}

HRESULT WINAPI CMyDocsFolder::SetNameOf(HWND hwndOwner, PCUITEMID_CHILD pidl,    /* simple pidl */
        LPCOLESTR lpName, DWORD dwFlags, PITEMID_CHILD *pPidlOut)
{
    return m_pisfInner->SetNameOf(hwndOwner, pidl, lpName, dwFlags, pPidlOut);
}

HRESULT WINAPI CMyDocsFolder::GetDefaultSearchGUID(GUID *pguid)
{
    return m_pisf2Inner->GetDefaultSearchGUID(pguid);
}

HRESULT WINAPI CMyDocsFolder::EnumSearches(IEnumExtraSearch ** ppenum)
{
    return m_pisf2Inner->EnumSearches(ppenum);
}

HRESULT WINAPI CMyDocsFolder::GetDefaultColumn(DWORD dwRes, ULONG *pSort, ULONG *pDisplay)
{
    return m_pisf2Inner->GetDefaultColumn(dwRes, pSort, pDisplay);
}

HRESULT WINAPI CMyDocsFolder::GetDefaultColumnState(UINT iColumn, DWORD *pcsFlags)
{
    return m_pisf2Inner->GetDefaultColumnState(iColumn, pcsFlags);
}

HRESULT WINAPI CMyDocsFolder::GetDetailsEx(PCUITEMID_CHILD pidl, const SHCOLUMNID *pscid, VARIANT *pv)
{
    return m_pisf2Inner->GetDetailsEx(pidl, pscid, pv);
}

HRESULT WINAPI CMyDocsFolder::GetDetailsOf(PCUITEMID_CHILD pidl, UINT iColumn, SHELLDETAILS *psd)
{
    return m_pisf2Inner->GetDetailsOf(pidl, iColumn, psd);
}

HRESULT WINAPI CMyDocsFolder::MapColumnToSCID(UINT column, SHCOLUMNID *pscid)
{
    return m_pisf2Inner->MapColumnToSCID(column, pscid);
}

HRESULT WINAPI CMyDocsFolder::GetClassID(CLSID *lpClassId)
{
    TRACE ("(%p)\n", this);

    if (!lpClassId)
        return E_POINTER;

    memcpy(lpClassId, &CLSID_MyDocuments, sizeof(GUID));

    return S_OK;
}

HRESULT WINAPI CMyDocsFolder::Initialize(LPCITEMIDLIST pidl)
{
    TRACE ("(%p)->(%p)\n", this, pidl);

    return S_OK;
}

HRESULT WINAPI CMyDocsFolder::GetCurFolder(LPITEMIDLIST *pidl)
{
    TRACE ("(%p)->(%p)\n", this, pidl);

    if (!pidl) return E_POINTER;
    *pidl = ILClone (pidlRoot);
    return S_OK;
}
