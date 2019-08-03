/*
 * PROJECT:     ReactOS Search Shell Extension
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Search results folder
 * COPYRIGHT:   Copyright 2019 Brock Mammen
 */

#include "CFindFolder.h"

WINE_DEFAULT_DEBUG_CHANNEL(shellfind);

// FIXME: Remove this declaration after the function has been fully implemented
EXTERN_C HRESULT
WINAPI
SHOpenFolderAndSelectItems(LPITEMIDLIST pidlFolder,
                           UINT cidl,
                           PCUITEMID_CHILD_ARRAY apidl,
                           DWORD dwFlags);

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

CFindFolder::CFindFolder() :
    m_hStopEvent(NULL)
{
}

static LPITEMIDLIST _ILCreate(LPCWSTR lpszPath, LPCITEMIDLIST lpcFindDataPidl)
{
    int pathLen = (wcslen(lpszPath) + 1) * sizeof(WCHAR);
    int cbData = sizeof(WORD) + pathLen + lpcFindDataPidl->mkid.cb;
    LPITEMIDLIST pidl = (LPITEMIDLIST) SHAlloc(cbData + sizeof(WORD));
    if (!pidl)
        return NULL;

    LPBYTE p = (LPBYTE) pidl;
    *((WORD *) p) = cbData;
    p += sizeof(WORD);

    memcpy(p, lpszPath, pathLen);
    p += pathLen;

    memcpy(p, lpcFindDataPidl, lpcFindDataPidl->mkid.cb);
    p += lpcFindDataPidl->mkid.cb;

    *((WORD *) p) = 0;

    return pidl;
}

static LPCWSTR _ILGetPath(LPCITEMIDLIST pidl)
{
    if (!pidl || !pidl->mkid.cb)
        return NULL;
    return (LPCWSTR) pidl->mkid.abID;
}

static LPCITEMIDLIST _ILGetFSPidl(LPCITEMIDLIST pidl)
{
    if (!pidl || !pidl->mkid.cb)
        return pidl;
    return (LPCITEMIDLIST) ((LPBYTE) pidl->mkid.abID
                            + ((wcslen((LPCWSTR) pidl->mkid.abID) + 1) * sizeof(WCHAR)));
}

LRESULT CFindFolder::AddItem(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    if (!lParam)
        return 0;

    HRESULT hr;
    LPWSTR path = (LPWSTR) lParam;

    CComPtr<IShellFolder> pShellFolder;
    hr = SHGetDesktopFolder(&pShellFolder);
    if (FAILED_UNEXPECTEDLY(hr))
    {
        LocalFree(path);
        return hr;
    }

    CComHeapPtr<ITEMIDLIST> lpFSPidl;
    DWORD pchEaten;
    hr = pShellFolder->ParseDisplayName(NULL, NULL, path, &pchEaten, &lpFSPidl, NULL);
    if (FAILED_UNEXPECTEDLY(hr))
    {
        LocalFree(path);
        return hr;
    }

    LPITEMIDLIST lpLastFSPidl = ILFindLastID(lpFSPidl);
    CComHeapPtr<ITEMIDLIST> lpSearchPidl(_ILCreate(path, lpLastFSPidl));
    LocalFree(path);
    if (!lpSearchPidl)
    {
        return E_OUTOFMEMORY;
    }

    UINT uItemIndex;
    hr = m_shellFolderView->AddObject(lpSearchPidl, &uItemIndex);

    return hr;
}

LRESULT CFindFolder::UpdateStatus(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    LPWSTR status = (LPWSTR) lParam;
    if (m_shellBrowser)
    {
        m_shellBrowser->SetStatusTextSB(status);
    }
    LocalFree(status);

    return S_OK;
}

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
        return m_pisfInner->GetDetailsOf(_ILGetFSPidl(pidl), iColumn - _countof(g_ColumnDefs) + 1, pDetails);

    pDetails->cxChar = g_ColumnDefs[iColumn].cxChar;
    pDetails->fmt = g_ColumnDefs[iColumn].fmt;

    if (!pidl)
        return SHSetStrRet(&pDetails->str, g_ColumnDefs[iColumn].wzColumnName);

    if (iColumn == 1)
    {
        WCHAR path[MAX_PATH];
        wcscpy(path, _ILGetPath(pidl));
        PathRemoveFileSpecW(path);
        return SHSetStrRet(&pDetails->str, path);
    }

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
    return m_pisfInner->CompareIDs(lParam, _ILGetFSPidl(pidl1), _ILGetFSPidl(pidl2));
}

STDMETHODIMP CFindFolder::CreateViewObject(HWND hwndOwner, REFIID riid, LPVOID *ppvOut)
{
    if (riid == IID_IShellView)
    {
        SFV_CREATE sfvparams = {};
        sfvparams.cbSize = sizeof(SFV_CREATE);
        sfvparams.pshf = this;
        sfvparams.psfvcb = this;
        HRESULT hr = SHCreateShellFolderView(&sfvparams, (IShellView **) ppvOut);
        if (FAILED_UNEXPECTEDLY(hr))
        {
            return hr;
        }

        return ((IShellView * ) * ppvOut)->QueryInterface(IID_PPV_ARG(IShellFolderView, &m_shellFolderView));
    }
    return E_NOINTERFACE;
}

STDMETHODIMP CFindFolder::GetAttributesOf(UINT cidl, PCUITEMID_CHILD_ARRAY apidl, DWORD *rgfInOut)
{
    CComHeapPtr<PCITEMID_CHILD> aFSPidl;
    aFSPidl.Allocate(cidl);
    for (UINT i = 0; i < cidl; i++)
    {
        aFSPidl[i] = _ILGetFSPidl(apidl[i]);
    }

    return m_pisfInner->GetAttributesOf(cidl, aFSPidl, rgfInOut);
}

STDMETHODIMP CFindFolder::GetUIObjectOf(HWND hwndOwner, UINT cidl, PCUITEMID_CHILD_ARRAY apidl, REFIID riid,
                                        UINT *prgfInOut, LPVOID *ppvOut)
{
    if (riid == IID_IDataObject && cidl == 1)
    {
        WCHAR path[MAX_PATH];
        wcscpy(path, (LPCWSTR) apidl[0]->mkid.abID);
        PathRemoveFileSpecW(path);
        CComHeapPtr<ITEMIDLIST> rootPidl(ILCreateFromPathW(path));
        if (!rootPidl)
            return E_OUTOFMEMORY;
        PCITEMID_CHILD aFSPidl[1];
        aFSPidl[0] = _ILGetFSPidl(apidl[0]);
        return IDataObject_Constructor(hwndOwner, rootPidl, aFSPidl, cidl, (IDataObject **) ppvOut);
    }

    if (cidl <= 0)
    {
        return m_pisfInner->GetUIObjectOf(hwndOwner, cidl, apidl, riid, prgfInOut, ppvOut);
    }

    PCITEMID_CHILD *aFSPidl = new PCITEMID_CHILD[cidl];
    for (UINT i = 0; i < cidl; i++)
    {
        aFSPidl[i] = _ILGetFSPidl(apidl[i]);
    }

    if (riid == IID_IContextMenu)
    {
        HKEY hKeys[16];
        UINT cKeys = 0;
        AddFSClassKeysToArray(aFSPidl[0], hKeys, &cKeys);

        DEFCONTEXTMENU dcm;
        dcm.hwnd = hwndOwner;
        dcm.pcmcb = this;
        dcm.pidlFolder = m_pidl;
        dcm.psf = this;
        dcm.cidl = cidl;
        dcm.apidl = apidl;
        dcm.cKeys = cKeys;
        dcm.aKeys = hKeys;
        dcm.punkAssociationInfo = NULL;
        HRESULT hr = SHCreateDefaultContextMenu(&dcm, riid, ppvOut);
        delete[] aFSPidl;

        return hr;
    }

    HRESULT hr = m_pisfInner->GetUIObjectOf(hwndOwner, cidl, aFSPidl, riid, prgfInOut, ppvOut);
    delete[] aFSPidl;

    return hr;
}

STDMETHODIMP CFindFolder::GetDisplayNameOf(PCUITEMID_CHILD pidl, DWORD dwFlags, LPSTRRET pName)
{
    return m_pisfInner->GetDisplayNameOf(_ILGetFSPidl(pidl), dwFlags, pName);
}

STDMETHODIMP CFindFolder::SetNameOf(HWND hwndOwner, PCUITEMID_CHILD pidl, LPCOLESTR lpName, DWORD dwFlags,
                                    PITEMID_CHILD *pPidlOut)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

//// *** IShellFolderViewCB method ***
STDMETHODIMP CFindFolder::MessageSFVCB(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case SFVM_DEFVIEWMODE:
        {
            FOLDERVIEWMODE *pViewMode = (FOLDERVIEWMODE *) lParam;
            *pViewMode = FVM_DETAILS;
            return S_OK;
        }
        case SFVM_WINDOWCREATED:
        {
            SubclassWindow((HWND) wParam);

            CComPtr<IServiceProvider> pServiceProvider;
            HRESULT hr = m_shellFolderView->QueryInterface(IID_PPV_ARG(IServiceProvider, &pServiceProvider));
            if (FAILED_UNEXPECTEDLY(hr))
            {
                return hr;
            }
            return pServiceProvider->QueryService(SID_SShellBrowser, IID_PPV_ARG(IShellBrowser, &m_shellBrowser));
        }
    }
    return E_NOTIMPL;
}

//// *** IContextMenuCB method ***
STDMETHODIMP CFindFolder::CallBack(IShellFolder *psf, HWND hwndOwner, IDataObject *pdtobj, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case DFM_MERGECONTEXTMENU:
        {
            QCMINFO *pqcminfo = (QCMINFO *) lParam;
            _InsertMenuItemW(pqcminfo->hmenu, pqcminfo->indexMenu++, TRUE, pqcminfo->idCmdFirst++, MFT_SEPARATOR, NULL, 0);
            _InsertMenuItemW(pqcminfo->hmenu, pqcminfo->indexMenu++, TRUE, pqcminfo->idCmdFirst++, MFT_STRING, L"Open Containing Folder", MFS_ENABLED);
            _InsertMenuItemW(pqcminfo->hmenu, pqcminfo->indexMenu++, TRUE, pqcminfo->idCmdFirst++, MFT_SEPARATOR, NULL, 0);
            return S_OK;
        }
        case DFM_INVOKECOMMAND:
        case DFM_INVOKECOMMANDEX:
        {
            if (wParam != 1)
                break;

            PCUITEMID_CHILD *apidl;
            UINT cidl;
            HRESULT hr = m_shellFolderView->GetSelectedObjects(&apidl, &cidl);
            if (FAILED_UNEXPECTEDLY(hr))
                return hr;

            for (UINT i = 0; i < cidl; i++)
            {
                CComHeapPtr<ITEMIDLIST> pidl;
                DWORD attrs = 0;
                hr = SHILCreateFromPathW((LPCWSTR) apidl[i]->mkid.abID, &pidl, &attrs);
                if (SUCCEEDED(hr))
                {
                    SHOpenFolderAndSelectItems(NULL, 1, &pidl, 0);
                }
            }

            return S_OK;
        }
        case DFM_GETDEFSTATICID:
            return S_FALSE;
    }
    return Shell_DefaultContextMenuCallBack(m_pisfInner, pdtobj);
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
