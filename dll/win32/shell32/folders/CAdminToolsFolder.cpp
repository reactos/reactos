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

CAdminToolsFolder::CAdminToolsFolder()
{
    m_pidlInner = NULL;
}

CAdminToolsFolder::~CAdminToolsFolder()
{
    if(m_pidlInner)
        SHFree(m_pidlInner);
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
    static const DWORD dwAdminToolsAttributes =
        SFGAO_STORAGE | SFGAO_STORAGEANCESTOR | SFGAO_FILESYSANCESTOR |
        SFGAO_FOLDER | SFGAO_FILESYSTEM | SFGAO_HASSUBFOLDER;

    if(cidl)
        return m_pisfInner->GetAttributesOf(cidl, apidl, rgfInOut);

    if (!rgfInOut || !apidl)
        return E_INVALIDARG;

    if (*rgfInOut == 0)
        *rgfInOut = ~0;

    *rgfInOut &= dwAdminToolsAttributes;

    /* make sure SFGAO_VALIDATE is cleared, some apps depend on that */
    *rgfInOut &= ~SFGAO_VALIDATE;

    return S_OK;
}

HRESULT WINAPI CAdminToolsFolder::GetUIObjectOf(HWND hwndOwner, UINT cidl, PCUITEMID_CHILD_ARRAY apidl,
        REFIID riid, UINT * prgfInOut, LPVOID * ppvOut)
{
    return m_pisfInner->GetUIObjectOf(hwndOwner, cidl, apidl, riid, prgfInOut, ppvOut);
}

HRESULT WINAPI CAdminToolsFolder::GetDisplayNameOf(PCUITEMID_CHILD pidl, DWORD dwFlags, LPSTRRET strRet)
{
    if (!strRet || !pidl)
        return E_INVALIDARG;

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

HRESULT WINAPI CAdminToolsFolder::SetNameOf(HWND hwndOwner, PCUITEMID_CHILD pidl,    /* simple pidl */
        LPCOLESTR lpName, DWORD dwFlags, PITEMID_CHILD *pPidlOut)
{
    return m_pisfInner->SetNameOf(hwndOwner, pidl, lpName, dwFlags, pPidlOut);
}

HRESULT WINAPI CAdminToolsFolder::GetDefaultSearchGUID(GUID *pguid)
{
    return m_pisfInner->GetDefaultSearchGUID(pguid);
}

HRESULT WINAPI CAdminToolsFolder::EnumSearches(IEnumExtraSearch ** ppenum)
{
    return m_pisfInner->EnumSearches(ppenum);
}

HRESULT WINAPI CAdminToolsFolder::GetDefaultColumn(DWORD dwRes, ULONG *pSort, ULONG *pDisplay)
{
    return m_pisfInner->GetDefaultColumn(dwRes, pSort, pDisplay);
}

HRESULT WINAPI CAdminToolsFolder::GetDefaultColumnState(UINT iColumn, DWORD *pcsFlags)
{
    return m_pisfInner->GetDefaultColumnState(iColumn, pcsFlags);
}

HRESULT WINAPI CAdminToolsFolder::GetDetailsEx(PCUITEMID_CHILD pidl, const SHCOLUMNID *pscid, VARIANT *pv)
{
    return m_pisfInner->GetDetailsEx(pidl, pscid, pv);
}

HRESULT WINAPI CAdminToolsFolder::GetDetailsOf(PCUITEMID_CHILD pidl, UINT iColumn, SHELLDETAILS *psd)
{
    return m_pisfInner->GetDetailsOf(pidl, iColumn, psd);
}

HRESULT WINAPI CAdminToolsFolder::MapColumnToSCID(UINT column, SHCOLUMNID *pscid)
{
    return m_pisfInner->MapColumnToSCID(column, pscid);
}

HRESULT WINAPI CAdminToolsFolder::GetClassID(CLSID *lpClassId)
{
    if (!lpClassId)
        return E_POINTER;

    memcpy(lpClassId, &CLSID_AdminFolderShortcut, sizeof(CLSID));

    return S_OK;
}

HRESULT WINAPI CAdminToolsFolder::Initialize(PCIDLIST_ABSOLUTE pidl)
{
    m_pidlInner = ILClone(pidl);
    if (!m_pidlInner)
        return E_OUTOFMEMORY;

    return SHELL32_CoCreateInitSF(m_pidlInner,
                                  &CLSID_ShellFSFolder,
                                  CSIDL_COMMON_ADMINTOOLS,
                                  IID_PPV_ARG(IShellFolder2, &m_pisfInner));
}

HRESULT WINAPI CAdminToolsFolder::GetCurFolder(PIDLIST_ABSOLUTE *pidl)
{
    if (!pidl)
        return E_POINTER;
    *pidl = ILClone(m_pidlInner);
    return S_OK;
}
