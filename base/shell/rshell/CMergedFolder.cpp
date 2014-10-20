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
    BOOL shared;
    IShellFolder * parent;
    LPITEMIDLIST pidl;
};

class CEnumMergedFolder :
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IEnumIDList
{

private:
    CComPtr<IShellFolder> m_UserLocalFolder;
    CComPtr<IShellFolder> m_AllUSersFolder;

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
    HRESULT FindPidlInList(HWND hwndOwner, LPCITEMIDLIST pcidl, LocalPidlInfo * pinfo);

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
    m_HwndOwner(NULL),
    m_Flags(0),
    m_hDsa(NULL),
    m_hDsaIndex(0),
    m_hDsaCount(0)
{
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

    TRACE("SetSources %p %p\n", userLocal, allUSers);
    return S_OK;
}

HRESULT CEnumMergedFolder::Begin(HWND hwndOwner, SHCONTF flags)
{
    HRESULT hr;
    
    if (m_hDsa && m_HwndOwner == hwndOwner && m_Flags == flags)
    {
        return Reset();
    }

    TRACE("Search conditions changed, recreating list...\n");

    CComPtr<IEnumIDList> userLocal;
    CComPtr<IEnumIDList> allUSers;
    hr = m_UserLocalFolder->EnumObjects(hwndOwner, flags, &userLocal);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;
    hr = m_AllUSersFolder->EnumObjects(hwndOwner, flags, &allUSers);
    if (FAILED_UNEXPECTEDLY(hr))
    {
        userLocal = NULL;
        return hr;
    }

    if (!m_hDsa)
    {
        m_hDsa = DSA_Create(sizeof(LocalPidlInfo), 10);
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
                hr1 = userLocal->Next(1, &pidl1, NULL);
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
                hr2 = allUSers->Next(1, &pidl2, NULL);
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

            TRACE("Both sources are S_OK, comparison between %S and %S returns %d\n", name1, name2, order);

            CoTaskMemFree(name1);
            CoTaskMemFree(name2);
        }
        else if (hr1 == S_OK)
        {
            order = -1;

            TRACE("Both sources are S_OK, forcing %d\n", order);
        }
        else if (hr2 == S_OK)
        {
            order = 1;

            TRACE("Both sources are S_OK, forcing %d\n", order);
        }
        else
        {
            TRACE("None of the sources\n");
            break;
        }

        LocalPidlInfo info = { FALSE };
        if (order < 0)
        {
            info.parent = m_UserLocalFolder;
            info.pidl = ILClone(pidl1);
            ILFree(pidl1);
        }
        else if (order > 0)
        {
            info.parent = m_AllUSersFolder;
            info.pidl = ILClone(pidl2);
            ILFree(pidl2);
        }
        else // if (order == 0)
        {
            info.shared = TRUE;
            info.parent = m_UserLocalFolder;
            info.pidl = ILClone(pidl1);
            ILFree(pidl1);
            ILFree(pidl2);
        }

        TRACE("Inserting item %d with parent %p and pidl { cb=%d }\n", m_hDsaCount, info.parent, info.pidl->mkid.cb);
        int idx = DSA_InsertItem(m_hDsa, DSA_APPEND, &info);
        TRACE("New index: %d\n", idx);

        m_hDsaCount++;

    } while (hr1 == S_OK || hr2 == S_OK);

    m_HwndOwner = hwndOwner;
    m_Flags = flags;
        
    return Reset();
}

HRESULT CEnumMergedFolder::FindPidlInList(HWND hwndOwner, LPCITEMIDLIST pcidl, LocalPidlInfo * pinfo)
{
    HRESULT hr;

    if (!m_hDsa)
    {
        Begin(hwndOwner, SHCONTF_FOLDERS | SHCONTF_NONFOLDERS);
    }

    TRACE("Searching for pidl { cb=%d } in a list of %d items\n", pcidl->mkid.cb, m_hDsaCount);

    for (int i = 0; i < (int)m_hDsaCount; i++)
    {
        LocalPidlInfo * tinfo = (LocalPidlInfo *)DSA_GetItemPtr(m_hDsa, i);
        if (!tinfo)
            return E_FAIL;

        LocalPidlInfo info = *tinfo;

        TRACE("Comparing with item at %d with parent %p and pidl { cb=%d }\n", i, info.parent, info.pidl->mkid.cb);

        hr = info.parent->CompareIDs(0, info.pidl, pcidl);
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

        TRACE("Returning next item at %d with parent %p and pidl { cb=%d }\n", m_hDsaIndex, info.parent, info.pidl->mkid.cb);

        // FIXME: ILClone shouldn't be needed here! This should be causing leaks
        if (rgelt) rgelt[i] = ILClone(info.pidl);
        i++;

        m_hDsaIndex++;
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
HRESULT WINAPI CMergedFolder_Constructor(REFIID riid, LPVOID *ppv)
{
    return ShellObjectCreator<CMergedFolder>(riid, ppv);
}

CMergedFolder::CMergedFolder() :
    m_UserLocal(NULL),
    m_AllUSers(NULL),
    m_EnumSource(NULL),
    m_UserLocalPidl(NULL),
    m_AllUsersPidl(NULL),
    m_shellPidl(NULL)
{
}

CMergedFolder::~CMergedFolder() 
{
    if (m_UserLocalPidl) ILFree(m_UserLocalPidl);
    if (m_AllUsersPidl)  ILFree(m_AllUsersPidl);
}

// IAugmentedShellFolder2
HRESULT STDMETHODCALLTYPE CMergedFolder::AddNameSpace(LPGUID lpGuid, IShellFolder * psf, LPCITEMIDLIST pcidl, ULONG dwUnknown)
{
    if (lpGuid)
    {
        TRACE("FIXME: No idea how to handle the GUID\n");
        return E_NOTIMPL;
    }

    TRACE("AddNameSpace %p %p\n", m_UserLocal.p, m_AllUSers.p);
    
    // FIXME: Use a DSA to store the list of merged namespaces, together with their related info (psf, pidl, ...)
    // For now, assume only 2 will ever be used, and ignore all the other data.
    if (!m_UserLocal)
    {
        m_UserLocal = psf;
        m_UserLocalPidl = ILClone(pcidl);
        return S_OK;
    }

    if (m_AllUSers)
        return E_FAIL;

    m_AllUSers = psf;
    m_AllUsersPidl = ILClone(pcidl);

    m_EnumSource = new CComObject<CEnumMergedFolder>();
    return m_EnumSource->SetSources(m_UserLocal, m_AllUSers);
}

HRESULT STDMETHODCALLTYPE CMergedFolder::GetNameSpaceID(LPCITEMIDLIST pcidl, LPGUID lpGuid)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CMergedFolder::QueryNameSpace(ULONG dwUnknown, LPGUID lpGuid, IShellFolder ** ppsf)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CMergedFolder::EnumNameSpace(ULONG dwUnknown, PULONG lpUnknown)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CMergedFolder::UnWrapIDList(LPCITEMIDLIST pcidl, LONG lUnknown, IShellFolder ** ppsf, LPITEMIDLIST * ppidl1, LPITEMIDLIST *ppidl2, LONG * lpUnknown)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
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
    HRESULT hr;
    LocalPidlInfo info;
    LPITEMIDLIST pidl;

    if (!ppidl) return E_FAIL;

    if (pchEaten) *pchEaten = 0;
    if (pdwAttributes) *pdwAttributes = 0;

    TRACE("ParseDisplayName name=%S\n", lpszDisplayName);

    hr = m_UserLocal->ParseDisplayName(hwndOwner, pbcReserved, lpszDisplayName, pchEaten, &pidl, pdwAttributes);
    if (SUCCEEDED(hr))
    {
        TRACE("ParseDisplayName result local\n");
        hr = m_EnumSource->FindPidlInList(hwndOwner, pidl, &info);
        if (SUCCEEDED(hr))
        {
            ILFree(pidl);
            *ppidl = ILClone(info.pidl);
            return hr;
        }
    }

    hr = m_AllUSers->ParseDisplayName(hwndOwner, pbcReserved, lpszDisplayName, pchEaten, &pidl, pdwAttributes);
    if (SUCCEEDED(hr))
    {
        TRACE("ParseDisplayName result common\n");
        hr = m_EnumSource->FindPidlInList(hwndOwner, pidl, &info);
        if (SUCCEEDED(hr))
        {
            ILFree(pidl);
            *ppidl = ILClone(info.pidl);
            return hr;
        }
    }

    if (ppidl) *ppidl = NULL;
    if (pchEaten) *pchEaten = 0;
    if (pdwAttributes) *pdwAttributes = 0;
    return HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
}

HRESULT STDMETHODCALLTYPE CMergedFolder::EnumObjects(
    HWND hwndOwner,
    SHCONTF grfFlags,
    IEnumIDList **ppenumIDList)
{
    TRACE("EnumObjects\n");
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
    
    hr = m_EnumSource->FindPidlInList(NULL, pidl, &info);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    TRACE("BindToObject shared = %d\n", info.shared);

    if (!info.shared)
        return info.parent->BindToObject(info.pidl, pbcReserved, riid, ppvOut);

    if (riid != IID_IShellFolder)
        return E_FAIL;

    CComPtr<IShellFolder> fld1;
    CComPtr<IShellFolder> fld2;

    hr = m_UserLocal->BindToObject(info.pidl, pbcReserved, IID_PPV_ARG(IShellFolder, &fld1));
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    hr = m_AllUSers->BindToObject(info.pidl, pbcReserved, IID_PPV_ARG(IShellFolder, &fld2));
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    CComPtr<IAugmentedShellFolder> pasf;
    hr = CMergedFolder_Constructor(IID_PPV_ARG(IAugmentedShellFolder, &pasf));
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    hr = pasf->QueryInterface(riid, ppvOut);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    hr = pasf->AddNameSpace(NULL, fld1, info.pidl, 0xFF00);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    hr = pasf->AddNameSpace(NULL, fld2, info.pidl, 0x0000);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    return hr;
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
    TRACE("CompareIDs\n");
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

    TRACE("GetAttributesOf\n");

    for (int i = 0; i < (int)cidl; i++)
    {
        LPCITEMIDLIST pidl = apidl[i];

        hr = m_EnumSource->FindPidlInList(NULL, pidl, &info);
        if (FAILED_UNEXPECTEDLY(hr))
            return hr;

        pidl = info.pidl;

        SFGAOF * pinOut1 = rgfInOut ? rgfInOut + i : NULL;

        hr = info.parent->GetAttributesOf(1, &pidl, pinOut1);

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

    TRACE("GetUIObjectOf\n");

    for (int i = 0; i < (int)cidl; i++)
    {
        LPCITEMIDLIST pidl = apidl[i];

        TRACE("Processing GetUIObjectOf item %d of %u...\n", i, cidl);

        hr = m_EnumSource->FindPidlInList(hwndOwner, pidl, &info);
        if (FAILED_UNEXPECTEDLY(hr))
            return hr;

        pidl = info.pidl;

        TRACE("FindPidlInList succeeded with parent %p and pidl { db=%d }\n", info.parent, info.pidl->mkid.cb);

        UINT * pinOut1 = prgfInOut ? prgfInOut+i : NULL;
        void** ppvOut1 = ppvOut ? ppvOut + i : NULL;

        hr = info.parent->GetUIObjectOf(hwndOwner, 1, &pidl, riid, pinOut1, ppvOut1);

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

    TRACE("GetDisplayNameOf\n");

    hr = m_EnumSource->FindPidlInList(NULL, pidl, &info);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    hr = info.parent->GetDisplayNameOf(info.pidl, uFlags, lpName);

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

// IPersist
HRESULT STDMETHODCALLTYPE CMergedFolder::GetClassID(CLSID *lpClassId)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

// IPersistFolder
HRESULT STDMETHODCALLTYPE CMergedFolder::Initialize(LPCITEMIDLIST pidl)
{
    m_shellPidl = ILClone(pidl);
    return S_OK;
}

// IPersistFolder2
HRESULT STDMETHODCALLTYPE CMergedFolder::GetCurFolder(LPITEMIDLIST * pidl)
{
    if (pidl)
        *pidl = m_shellPidl;
    return S_OK;
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
    LocalPidlInfo info;
    HRESULT hr;

    TRACE("GetDetailsEx\n");

    hr = m_EnumSource->FindPidlInList(NULL, pidl, &info);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    CComPtr<IShellFolder2> parent2;
    hr = info.parent->QueryInterface(IID_PPV_ARG(IShellFolder2, &parent2));
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    hr = parent2->GetDetailsEx(info.pidl, pscid, pv);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMergedFolder::GetDetailsOf(
    LPCITEMIDLIST pidl,
    UINT iColumn,
    SHELLDETAILS *psd)
{
    LocalPidlInfo info;
    HRESULT hr;

    TRACE("GetDetailsOf\n");

    hr = m_EnumSource->FindPidlInList(NULL, pidl, &info);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    CComPtr<IShellFolder2> parent2;
    hr = info.parent->QueryInterface(IID_PPV_ARG(IShellFolder2, &parent2));
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    hr = parent2->GetDetailsOf(info.pidl, iColumn, psd);

    if (FAILED_UNEXPECTEDLY(hr))
        return hr;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMergedFolder::MapColumnToSCID(
    UINT iColumn,
    SHCOLUMNID *pscid)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}