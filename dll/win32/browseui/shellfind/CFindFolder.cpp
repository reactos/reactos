/*
 * PROJECT:     ReactOS Search Shell Extension
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Search results folder
 * COPYRIGHT:   Copyright 2019 Brock Mammen
 */

#include "CFindFolder.h"

WINE_DEFAULT_DEBUG_CHANNEL(shellfind);

struct FolderViewColumns
{
    LPCWSTR wzColumnName;
    DWORD dwDefaultState;
    int fmt;
    int cxChar;
};

static FolderViewColumns g_ColumnDefs[] =
{
    {L"Name",      SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT, LVCFMT_LEFT, 30},
    {L"In Folder", SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT, LVCFMT_LEFT, 30},
    {L"Relevance", SHCOLSTATE_TYPE_STR,                          LVCFMT_LEFT, 0}
};

// *** IShellFolder2 methods ***
STDMETHODIMP CFindFolder::GetDefaultSearchGUID(GUID *pguid)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

STDMETHODIMP CFindFolder::EnumSearches(IEnumExtraSearch **ppenum)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

STDMETHODIMP CFindFolder::GetDefaultColumn(DWORD, ULONG *pSort, ULONG *pDisplay)
{
    if (pSort)
        *pSort = 0;
    if (pDisplay)
        *pDisplay = 0;
    return S_OK;
}

STDMETHODIMP CFindFolder::GetDefaultColumnState(UINT iColumn, DWORD *pcsFlags)
{
    if (!pcsFlags)
        return E_INVALIDARG;
    if (iColumn >= _countof(g_ColumnDefs))
        return m_pisfInner->GetDefaultColumnState(iColumn - _countof(g_ColumnDefs) + 1, pcsFlags);
    *pcsFlags = g_ColumnDefs[iColumn].dwDefaultState;
    return S_OK;
}

STDMETHODIMP CFindFolder::GetDetailsEx(PCUITEMID_CHILD pidl, const SHCOLUMNID *pscid, VARIANT *pv)
{
    return m_pisfInner->GetDetailsEx(pidl, pscid, pv);
}

STDMETHODIMP CFindFolder::GetDetailsOf(PCUITEMID_CHILD pidl, UINT iColumn, SHELLDETAILS *pDetails)
{
    if (iColumn >= _countof(g_ColumnDefs))
        return m_pisfInner->GetDetailsOf(pidl, iColumn - _countof(g_ColumnDefs) + 1, pDetails);

    pDetails->cxChar = g_ColumnDefs[iColumn].cxChar;
    pDetails->fmt = g_ColumnDefs[iColumn].fmt;

    if (!pidl)
        return SHSetStrRet(&pDetails->str, g_ColumnDefs[iColumn].wzColumnName);

    return GetDisplayNameOf(pidl, SHGDN_NORMAL, &pDetails->str);
}

STDMETHODIMP CFindFolder::MapColumnToSCID(UINT iColumn, SHCOLUMNID *pscid)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

// *** IShellFolder methods ***
STDMETHODIMP CFindFolder::ParseDisplayName(HWND hwndOwner, LPBC pbc, LPOLESTR lpszDisplayName, ULONG *pchEaten,
                                           PIDLIST_RELATIVE *ppidl, ULONG *pdwAttributes)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

STDMETHODIMP CFindFolder::EnumObjects(HWND hwndOwner, DWORD dwFlags, LPENUMIDLIST *ppEnumIDList)
{
    *ppEnumIDList = NULL;
    return S_FALSE;
}

STDMETHODIMP CFindFolder::BindToObject(PCUIDLIST_RELATIVE pidl, LPBC pbcReserved, REFIID riid, LPVOID *ppvOut)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

STDMETHODIMP CFindFolder::BindToStorage(PCUIDLIST_RELATIVE pidl, LPBC pbcReserved, REFIID riid, LPVOID *ppvOut)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

STDMETHODIMP CFindFolder::CompareIDs(LPARAM lParam, PCUIDLIST_RELATIVE pidl1, PCUIDLIST_RELATIVE pidl2)
{
    return m_pisfInner->CompareIDs(lParam, pidl1, pidl2);
}

STDMETHODIMP CFindFolder::CreateViewObject(HWND hwndOwner, REFIID riid, LPVOID *ppvOut)
{
    if (riid == IID_IShellView)
    {
        SFV_CREATE sfvparams = {};
        sfvparams.cbSize = sizeof(SFV_CREATE);
        sfvparams.pshf = this;
        return SHCreateShellFolderView(&sfvparams, (IShellView **) ppvOut);
    }
    return E_NOINTERFACE;
}

STDMETHODIMP CFindFolder::GetAttributesOf(UINT cidl, PCUITEMID_CHILD_ARRAY apidl, DWORD *rgfInOut)
{
    return m_pisfInner->GetAttributesOf(cidl, apidl, rgfInOut);
}

STDMETHODIMP CFindFolder::GetUIObjectOf(HWND hwndOwner, UINT cidl, PCUITEMID_CHILD_ARRAY apidl, REFIID riid,
                                        UINT *prgfInOut, LPVOID *ppvOut)
{
    return m_pisfInner->GetUIObjectOf(hwndOwner, cidl, apidl, riid, prgfInOut, ppvOut);
}

STDMETHODIMP CFindFolder::GetDisplayNameOf(PCUITEMID_CHILD pidl, DWORD dwFlags, LPSTRRET pName)
{
    return m_pisfInner->GetDisplayNameOf(pidl, dwFlags, pName);
}

STDMETHODIMP CFindFolder::SetNameOf(HWND hwndOwner, PCUITEMID_CHILD pidl, LPCOLESTR lpName, DWORD dwFlags,
                                    PITEMID_CHILD *pPidlOut)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}
//// *** IPersistFolder2 methods ***
STDMETHODIMP CFindFolder::GetCurFolder(LPITEMIDLIST *pidl)
{
    *pidl = ILClone(m_pidl);
    return S_OK;
}

// *** IPersistFolder methods ***
STDMETHODIMP CFindFolder::Initialize(LPCITEMIDLIST pidl)
{
    m_pidl = ILClone(pidl);
    if (!m_pidl)
        return E_OUTOFMEMORY;

    return SHELL32_CoCreateInitSF(m_pidl,
                                  NULL,
                                  NULL,
                                  &CLSID_ShellFSFolder,
                                  IID_PPV_ARG(IShellFolder2, &m_pisfInner));
}

// *** IPersist methods ***
STDMETHODIMP CFindFolder::GetClassID(CLSID *pClassId)
{
    if (pClassId == NULL)
        return E_INVALIDARG;
    memcpy(pClassId, &CLSID_FindFolder, sizeof(CLSID));
    return S_OK;
}
