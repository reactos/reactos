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

struct LocalPidlInfo
{
    int side; // -1 local, 0 shared, 1 common
    LPITEMIDLIST pidl;
};

class CEnumMergedFolder :
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IEnumIDList
{

private:
    CComPtr<IShellFolder> m_UserLocalFolder;
    CComPtr<IShellFolder> m_AllUSersFolder;
    CComPtr<IEnumIDList> m_UserLocal;
    CComPtr<IEnumIDList> m_AllUSers;

    HWND m_HwndOwner;
    SHCONTF m_Flags;

    HDSA m_hDsa;
    UINT m_hDsaIndex;
    UINT m_hDsaCount;

public:
    CEnumMergedFolder();
    virtual ~CEnumMergedFolder();

    DECLARE_NOT_AGGREGATABLE(CEnumMergedFolder)
    DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_COM_MAP(CEnumMergedFolder)
        COM_INTERFACE_ENTRY_IID(IID_IEnumIDList, IEnumIDList)
    END_COM_MAP()

    int  DsaDeleteCallback(LocalPidlInfo * info);

    static int CALLBACK s_DsaDeleteCallback(void *pItem, void *pData);

    HRESULT SetSources(IShellFolder * userLocal, IShellFolder * allUSers);
    HRESULT Begin(HWND hwndOwner, SHCONTF flags);
    HRESULT FindPidlInList(LPCITEMIDLIST pcidl, LocalPidlInfo * pinfo);

    virtual HRESULT STDMETHODCALLTYPE Next(
        ULONG celt,
        LPITEMIDLIST *rgelt,
        ULONG *pceltFetched);

    virtual HRESULT STDMETHODCALLTYPE Skip(ULONG celt);
    virtual HRESULT STDMETHODCALLTYPE Reset();
    virtual HRESULT STDMETHODCALLTYPE Clone(IEnumIDList **ppenum);
};

CEnumMergedFolder::CEnumMergedFolder() :
    m_UserLocalFolder(NULL),
    m_AllUSersFolder(NULL),
    m_UserLocal(NULL),
    m_AllUSers(NULL),
    m_HwndOwner(NULL),
    m_Flags(0),
    m_hDsaIndex(0)
{
    m_hDsa = DSA_Create(sizeof(LocalPidlInfo), 10);
}
    
CEnumMergedFolder::~CEnumMergedFolder()
{
    DSA_DestroyCallback(m_hDsa, s_DsaDeleteCallback, this);
}

int  CEnumMergedFolder::DsaDeleteCallback(LocalPidlInfo * info)
{
    ILFree(info->pidl);
    return 0;
}
    
int CALLBACK CEnumMergedFolder::s_DsaDeleteCallback(void *pItem, void *pData)
{
    CEnumMergedFolder * mf = (CEnumMergedFolder*) pData;
    LocalPidlInfo  * item = (LocalPidlInfo*) pItem;
    return mf->DsaDeleteCallback(item);
}

HRESULT CEnumMergedFolder::SetSources(IShellFolder * userLocal, IShellFolder * allUSers)
{
    m_UserLocalFolder = userLocal;
    m_AllUSersFolder = allUSers;
    return S_OK;
}

HRESULT CEnumMergedFolder::Begin(HWND hwndOwner, SHCONTF flags)
{
    HRESULT hr;

    if (m_HwndOwner == hwndOwner && m_Flags == flags)
    {
        return Reset();
    }

    TRACE("Search conditions changed, recreating list...\n");

    hr = m_UserLocalFolder->EnumObjects(hwndOwner, flags, &m_UserLocal);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;
    hr = m_AllUSersFolder->EnumObjects(hwndOwner, flags, &m_AllUSers);
    if (FAILED_UNEXPECTEDLY(hr))
    {
        m_UserLocal = NULL;
        return hr;
    }

    DSA_EnumCallback(m_hDsa, s_DsaDeleteCallback, this);
    DSA_DeleteAllItems(m_hDsa);
    m_hDsaCount = 0;

    HRESULT hr1 = S_OK;
    HRESULT hr2 = S_OK;
    LPITEMIDLIST pidl1 = NULL;
    LPITEMIDLIST pidl2 = NULL;
    int order = 0;
    do
    {
        if (order <= 0)
        {
            if (hr1 == S_OK)
            {
                hr1 = m_UserLocal->Next(1, &pidl1, NULL);
                if (FAILED_UNEXPECTEDLY(hr1))
                    return hr1;
            }
            else
            {
                pidl1 = NULL;
            }
        }
        if (order >= 0)
        {
            if (hr2 == S_OK)
            {
                hr2 = m_AllUSers->Next(1, &pidl2, NULL);
                if (FAILED_UNEXPECTEDLY(hr2))
                    return hr2;
            }
            else
            {
                pidl2 = NULL;
            }
        }

        if (hr1 == S_OK && hr2 == S_OK)
        {
            LPWSTR name1;
            LPWSTR name2;
            STRRET str1 = { STRRET_WSTR };
            STRRET str2 = { STRRET_WSTR };
            hr = m_UserLocalFolder->GetDisplayNameOf(pidl1, SHGDN_FORPARSING | SHGDN_INFOLDER, &str1);
            if (FAILED(hr))
                return hr;
            hr = m_AllUSersFolder->GetDisplayNameOf(pidl2, SHGDN_FORPARSING | SHGDN_INFOLDER, &str2);
            if (FAILED(hr))
                return hr;
            StrRetToStrW(&str1, pidl1, &name1);
            StrRetToStrW(&str2, pidl2, &name2);
            order = StrCmpW(name1, name2);
            CoTaskMemFree(name1);
            CoTaskMemFree(name2);
        }
        else if (hr1 == S_OK)
        {
            order = -1;
        }
        else if (hr2 == S_OK)
        {
            order = 1;
        }
        else
        {
            break;
        }

        LocalPidlInfo info;
        if (order < 0)
        {
            info.side = -1;
            info.pidl = ILClone(pidl1);
            ILFree(pidl1);
        }
        else if (order > 0)
        {
            info.side = 1;
            info.pidl = ILClone(pidl2);
            ILFree(pidl2);
        }
        else // if (order == 0)
        {
            info.side = 0;
            info.pidl = ILClone(pidl1);
            ILFree(pidl1);
            ILFree(pidl2);
        }

        TRACE("Inserting item %d with side %d and pidl { cb=%d }\n", m_hDsaCount, info.side, info.pidl->mkid.cb);
        int idx = DSA_InsertItem(m_hDsa, DSA_APPEND, &info);
        TRACE("New index: %d\n", idx);

        m_hDsaCount++;

    } while (hr1 == S_OK || hr2 == S_OK);

    m_HwndOwner = hwndOwner;
    m_Flags = flags;
        
    return Reset();
}

HRESULT CEnumMergedFolder::FindPidlInList(LPCITEMIDLIST pcidl, LocalPidlInfo * pinfo)
{
    HRESULT hr;

    TRACE("Searching for pidl { cb=%d } in a list of %d items\n", pcidl->mkid.cb, m_hDsaCount);

    for (int i = 0; i < (int)m_hDsaCount; i++)
    {
        LocalPidlInfo * tinfo = (LocalPidlInfo *)DSA_GetItemPtr(m_hDsa, i);
        if (!tinfo)
            return E_FAIL;

        LocalPidlInfo info = *tinfo;

        TRACE("Comparing with item at %d with side %d and pidl { cb=%d }\n", i, info.side, info.pidl->mkid.cb);

        CComPtr<IShellFolder> fld;
        if (info.side <= 0)
            fld = m_UserLocalFolder;
        else
            fld = m_AllUSersFolder;

        hr = m_AllUSersFolder->CompareIDs(0, info.pidl, pcidl);
        if (FAILED_UNEXPECTEDLY(hr))
            return hr;

        if (hr == S_OK)
        {
            *pinfo = info;
            return S_OK;
        }
        else
        {
            TRACE("Comparison returned %d\n", (int) (short) (hr & 0xFFFF));
        }
    }

    TRACE("Pidl not found\n");
    return HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
}

HRESULT STDMETHODCALLTYPE CEnumMergedFolder::Next(
    ULONG celt,
    LPITEMIDLIST *rgelt,
    ULONG *pceltFetched)
{
    if (pceltFetched) *pceltFetched = 0;

    if (m_hDsaIndex == m_hDsaCount)
        return S_FALSE;

    for (int i = 0; i < (int)celt;)
    {
        LocalPidlInfo * tinfo = (LocalPidlInfo *) DSA_GetItemPtr(m_hDsa, m_hDsaIndex);
        if (!tinfo)
            return E_FAIL;

        LocalPidlInfo info = *tinfo;

        TRACE("Returning next item at %d with side %d and pidl { cb=%d }\n", m_hDsaIndex, info.side, info.pidl->mkid.cb);

        // FIXME: ILClone shouldn't be needed here! This should be causing leaks
        if (rgelt) rgelt[i] = ILClone(info.pidl);

        m_hDsaIndex++;
        i++;

        if (m_hDsaIndex == m_hDsaCount)
        {
            if (pceltFetched) *pceltFetched = i;
            return (i == (int)celt) ? S_OK : S_FALSE;
        }
    }

    if (pceltFetched) *pceltFetched = celt;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CEnumMergedFolder::Skip(ULONG celt)
{
    return Next(celt, NULL, NULL);
}

HRESULT STDMETHODCALLTYPE CEnumMergedFolder::Reset()
{
    m_hDsaIndex = 0;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CEnumMergedFolder::Clone(
    IEnumIDList **ppenum)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

//-----------------------------------------------------------------------------
// CMergedFolder

extern "C"
HRESULT WINAPI CMergedFolder_Constructor(IShellFolder* userLocal, IShellFolder* allUsers, REFIID riid, LPVOID *ppv)
{
    *ppv = NULL;

    CMergedFolder * fld = new CComObject<CMergedFolder>();

    if (!fld)
        return E_OUTOFMEMORY;

    HRESULT hr;

    hr = fld->_SetSources(userLocal, allUsers);

    hr = fld->QueryInterface(riid, ppv);
    if (FAILED_UNEXPECTEDLY(hr))
        delete fld;

    return hr;
}

HRESULT CMergedFolder::_SetSources(IShellFolder* userLocal, IShellFolder* allUsers)
{
    m_UserLocal = userLocal;
    m_AllUSers = allUsers;
    m_EnumSource = new CComObject<CEnumMergedFolder>();
    return m_EnumSource->SetSources(m_UserLocal, m_AllUSers);
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
    HRESULT hr = m_EnumSource->QueryInterface(IID_PPV_ARG(IEnumIDList, ppenumIDList));
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;
    return m_EnumSource->Begin(hwndOwner, grfFlags);
}

HRESULT STDMETHODCALLTYPE CMergedFolder::BindToObject(
    LPCITEMIDLIST pidl,
    LPBC pbcReserved,
    REFIID riid,
    void **ppvOut)
{
    LocalPidlInfo info;
    HRESULT hr;

    hr = m_EnumSource->FindPidlInList(pidl, &info);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;
    
    if (info.side < 0)
        return m_UserLocal->BindToObject(pidl, pbcReserved, riid, ppvOut);
    if (info.side > 0)
        return m_AllUSers->BindToObject(pidl, pbcReserved, riid, ppvOut);

    if (riid != IID_IShellFolder)
        return E_FAIL;

    CComPtr<IShellFolder> fld1;
    CComPtr<IShellFolder> fld2;

    hr = m_UserLocal->BindToObject(pidl, pbcReserved, IID_PPV_ARG(IShellFolder, &fld1));
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    hr = m_AllUSers->BindToObject(pidl, pbcReserved, IID_PPV_ARG(IShellFolder, &fld2));
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    return CMergedFolder_Constructor(fld1, fld2, riid, ppvOut);
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
    return m_UserLocal->CompareIDs(lParam, pidl1, pidl2);
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
    LocalPidlInfo info;
    HRESULT hr;

    for (int i = 0; i < (int)cidl; i++)
    {
        LPCITEMIDLIST pidl = apidl[i];

        hr = m_EnumSource->FindPidlInList(pidl, &info);
        if (FAILED_UNEXPECTEDLY(hr))
            return hr;

        SFGAOF * pinOut1 = rgfInOut ? rgfInOut + i : NULL;

        if (info.side <= 0)
            hr = m_UserLocal->GetAttributesOf(1, &pidl, pinOut1);
        else
            hr = m_AllUSers->GetAttributesOf(1, &pidl, pinOut1);

        if (FAILED_UNEXPECTEDLY(hr))
            return hr;
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMergedFolder::GetUIObjectOf(
    HWND hwndOwner,
    UINT cidl,
    LPCITEMIDLIST *apidl,
    REFIID riid,
    UINT *prgfInOut,
    void **ppvOut)
{
    LocalPidlInfo info;
    HRESULT hr;

    for (int i = 0; i < (int)cidl; i++)
    {
        LPCITEMIDLIST pidl = apidl[i];

        hr = m_EnumSource->FindPidlInList(pidl, &info);
        if (FAILED_UNEXPECTEDLY(hr))
            return hr;

        UINT * pinOut1 = prgfInOut ? prgfInOut+i : NULL;
        void** ppvOut1 = ppvOut ? ppvOut + i : NULL;

        if (info.side <= 0)
            hr = m_UserLocal->GetUIObjectOf(hwndOwner, 1, &pidl, riid, pinOut1, ppvOut1);
        else
            hr = m_AllUSers->GetUIObjectOf(hwndOwner, 1, &pidl, riid, pinOut1, ppvOut1);

        if (FAILED_UNEXPECTEDLY(hr))
            return hr;
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMergedFolder::GetDisplayNameOf(
    LPCITEMIDLIST pidl,
    SHGDNF uFlags,
    STRRET *lpName)
{
    LocalPidlInfo info;
    HRESULT hr;

    hr = m_EnumSource->FindPidlInList(pidl, &info);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    if (info.side <= 0)
        hr = m_UserLocal->GetDisplayNameOf(pidl, uFlags, lpName);
    else
        hr = m_AllUSers->GetDisplayNameOf(pidl, uFlags, lpName);

    if (FAILED_UNEXPECTEDLY(hr))
        return hr;
    return S_OK;
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