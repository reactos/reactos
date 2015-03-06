/*
 * Fonts folder
 *
 * Copyright 2008       Johannes Anderwald <johannes.anderwald@reactos.org>
 * Copyright 2009       Andrew Hill
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

WINE_DEFAULT_DEBUG_CHANNEL (shell);

/*
This folder should not exist. It is just a file system folder... The \windows\fonts
directory contains a hidden desktop.ini with a UIHandler entry that specifies a class
that lives in fontext.dll. The UI handler creates a custom view for the folder, which
is what we normally see. However, the folder is a perfectly normal CFSFolder.
*/

/***********************************************************************
*   IShellFolder implementation
*/

CFontsFolder::CFontsFolder()
{
    m_pisfInner = NULL;
    m_pisf2Inner = NULL;

    pidlRoot = NULL;
    apidl = NULL;
}

CFontsFolder::~CFontsFolder()
{
    TRACE("-- destroying IShellFolder(%p)\n", this);
    SHFree(pidlRoot);
}

static LPITEMIDLIST _ILCreateFont(void)
{
    return _ILCreateGuid(PT_GUID, CLSID_FontsFolderShortcut);
}

HRESULT WINAPI CFontsFolder::FinalConstruct()
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
    info.csidl = CSIDL_FONTS;
    hr = ppf3->InitializeEx(NULL, _ILCreateFont(), &info);

    return hr;
}

HRESULT WINAPI CFontsFolder::ParseDisplayName(HWND hwndOwner, LPBC pbc, LPOLESTR lpszDisplayName,
        ULONG *pchEaten, PIDLIST_RELATIVE *ppidl, ULONG *pdwAttributes)
{
    return m_pisfInner->ParseDisplayName(hwndOwner, pbc, lpszDisplayName, pchEaten, ppidl, pdwAttributes);
}

HRESULT WINAPI CFontsFolder::EnumObjects(HWND hwndOwner, DWORD dwFlags, LPENUMIDLIST *ppEnumIDList)
{
    return m_pisfInner->EnumObjects(hwndOwner, dwFlags, ppEnumIDList);
}

HRESULT WINAPI CFontsFolder::BindToObject(PCUIDLIST_RELATIVE pidl, LPBC pbcReserved, REFIID riid, LPVOID *ppvOut)
{
    return m_pisfInner->BindToObject(pidl, pbcReserved, riid, ppvOut);
}

HRESULT WINAPI CFontsFolder::BindToStorage(PCUIDLIST_RELATIVE pidl, LPBC pbcReserved, REFIID riid, LPVOID *ppvOut)
{
    return m_pisfInner->BindToStorage(pidl, pbcReserved, riid, ppvOut);
}

HRESULT WINAPI CFontsFolder::CompareIDs(LPARAM lParam, PCUIDLIST_RELATIVE pidl1, PCUIDLIST_RELATIVE pidl2)
{
    return m_pisfInner->CompareIDs(lParam, pidl1, pidl2);
}

HRESULT WINAPI CFontsFolder::CreateViewObject(HWND hwndOwner, REFIID riid, LPVOID *ppvOut)
{
    return m_pisfInner->CreateViewObject(hwndOwner, riid, ppvOut);
}

HRESULT WINAPI CFontsFolder::GetAttributesOf(UINT cidl, PCUITEMID_CHILD_ARRAY apidl, DWORD *rgfInOut)
{
    return m_pisfInner->GetAttributesOf(cidl, apidl, rgfInOut);
}

HRESULT WINAPI CFontsFolder::GetUIObjectOf(HWND hwndOwner, UINT cidl, PCUITEMID_CHILD_ARRAY apidl,
        REFIID riid, UINT * prgfInOut, LPVOID * ppvOut)
{
    return m_pisfInner->GetUIObjectOf(hwndOwner, cidl, apidl, riid, prgfInOut, ppvOut);
}

HRESULT WINAPI CFontsFolder::GetDisplayNameOf(PCUITEMID_CHILD pidl, DWORD dwFlags, LPSTRRET strRet)
{
    if (!_ILIsSpecialFolder(pidl))
        return m_pisfInner->GetDisplayNameOf(pidl, dwFlags, strRet);

    TRACE ("(%p)->(pidl=%p,0x%08x,%p)\n", this, pidl, dwFlags, strRet);
    pdump (pidl);

    if (!strRet)
        return E_INVALIDARG;

if (!pidl->mkid.cb)
    {
        WCHAR wszPath[MAX_PATH];

        if ((GET_SHGDN_RELATION (dwFlags) == SHGDN_NORMAL) &&
                (GET_SHGDN_FOR (dwFlags) & SHGDN_FORPARSING))
        {
            if (!SHGetSpecialFolderPathW(NULL, wszPath, CSIDL_FONTS, FALSE))
                return E_FAIL;
        }
        else if (!HCR_GetClassNameW(CLSID_FontsFolderShortcut, wszPath, MAX_PATH))
            return E_FAIL;

        strRet->pOleStr = (LPWSTR)CoTaskMemAlloc((wcslen(wszPath) + 1) * sizeof(WCHAR));
        if (!strRet->pOleStr)
            return E_OUTOFMEMORY;

        wcscpy(strRet->pOleStr, wszPath);
        strRet->uType = STRRET_WSTR;

        return S_OK;
    }
    else
        return E_INVALIDARG;
}

HRESULT WINAPI CFontsFolder::SetNameOf(HWND hwndOwner, PCUITEMID_CHILD pidl,    /* simple pidl */
        LPCOLESTR lpName, DWORD dwFlags, PITEMID_CHILD *pPidlOut)
{
    return m_pisfInner->SetNameOf(hwndOwner, pidl, lpName, dwFlags, pPidlOut);
}

HRESULT WINAPI CFontsFolder::GetDefaultSearchGUID(GUID *pguid)
{
    return m_pisf2Inner->GetDefaultSearchGUID(pguid);
}

HRESULT WINAPI CFontsFolder::EnumSearches(IEnumExtraSearch ** ppenum)
{
    return m_pisf2Inner->EnumSearches(ppenum);
}

HRESULT WINAPI CFontsFolder::GetDefaultColumn(DWORD dwRes, ULONG *pSort, ULONG *pDisplay)
{
    return m_pisf2Inner->GetDefaultColumn(dwRes, pSort, pDisplay);
}

HRESULT WINAPI CFontsFolder::GetDefaultColumnState(UINT iColumn, DWORD *pcsFlags)
{
    return m_pisf2Inner->GetDefaultColumnState(iColumn, pcsFlags);
}

HRESULT WINAPI CFontsFolder::GetDetailsEx(PCUITEMID_CHILD pidl, const SHCOLUMNID *pscid, VARIANT *pv)
{
    return m_pisf2Inner->GetDetailsEx(pidl, pscid, pv);
}

HRESULT WINAPI CFontsFolder::GetDetailsOf(PCUITEMID_CHILD pidl, UINT iColumn, SHELLDETAILS *psd)
{
    return m_pisf2Inner->GetDetailsOf(pidl, iColumn, psd);
}

HRESULT WINAPI CFontsFolder::MapColumnToSCID(UINT column, SHCOLUMNID *pscid)
{
    return m_pisf2Inner->MapColumnToSCID(column, pscid);
}

/************************************************************************
 *    CFontsFolder::GetClassID
 */
HRESULT WINAPI CFontsFolder::GetClassID(CLSID *lpClassId)
{
    TRACE ("(%p)\n", this);

    if (!lpClassId)
        return E_POINTER;

    *lpClassId = CLSID_FontsFolderShortcut;

    return S_OK;
}

/************************************************************************
 *    CFontsFolder::Initialize
 *
 * NOTES: it makes no sense to change the pidl
 */
HRESULT WINAPI CFontsFolder::Initialize(LPCITEMIDLIST pidl)
{
    TRACE ("(%p)->(%p)\n", this, pidl);

    return E_NOTIMPL;
}

/**************************************************************************
 *    CFontsFolder::GetCurFolder
 */
HRESULT WINAPI CFontsFolder::GetCurFolder(LPITEMIDLIST *pidl)
{
    TRACE ("(%p)->(%p)\n", this, pidl);

    if (!pidl)
        return E_POINTER;

    *pidl = ILClone(pidlRoot);

    return S_OK;
}
