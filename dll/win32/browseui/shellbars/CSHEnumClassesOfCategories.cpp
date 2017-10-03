/*
 * ReactOS Explorer
 *
 * Copyright 2016 Sylvain Deverre <deverre dot sylv at gmail dot com>
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/*
 * Wraps the component categories manager enum
 */

#include "shellbars.h"

#define REGPATH L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Discardable\\PostSetup\\Component Categories"
#define IMPLEMENTING L"Implementing"
#define REQUIRING L"Requiring"

typedef struct categoryCacheHeader
{
    DWORD               dwSize;         // size of header only
    DWORD               version;        // currently 1
    SYSTEMTIME          writeTime;      // time we were written to registry
    DWORD               classCount;     // number of classes following
} CATCACHEHDR, *PCATCACHEHDR;

/*
 * This class manages a cached explorer component categories items, writing cache if it
 * doesn't exist yet.
 * It is used by CSHEnumClassesOfCategories internally.
 */
class CComCatCachedCategory
{
    public:
        CComCatCachedCategory();
        virtual ~CComCatCachedCategory();
        HRESULT WriteCacheToDSA(HDSA pDest);
        HRESULT STDMETHODCALLTYPE Initialize(CATID &catID, BOOL reloadCache);
    private:
        BOOL LoadFromRegistry();
        HRESULT LoadFromComCatMgr();
        HRESULT CacheDSA();
        CATID fCategory;
        HDSA fLocalDsa;
};

CComCatCachedCategory::CComCatCachedCategory()
{
    fLocalDsa = DSA_Create(sizeof(GUID), 5);
}

HRESULT STDMETHODCALLTYPE CComCatCachedCategory::Initialize(CATID &catID, BOOL reloadCache)
{
    HRESULT hr;

    fCategory = catID;
    if (reloadCache || !LoadFromRegistry())
    {
        hr = LoadFromComCatMgr();
        if (FAILED_UNEXPECTEDLY(hr))
            return hr;

        hr = CacheDSA();
        if (FAILED_UNEXPECTEDLY(hr))
            return hr;
    }
    return S_OK;
}

CComCatCachedCategory::~CComCatCachedCategory()
{
    DSA_Destroy(fLocalDsa);
}

BOOL CComCatCachedCategory::LoadFromRegistry()
{
    WCHAR bufKey[MAX_PATH];
    WCHAR guidStr[MAX_PATH];
    DWORD dataSize, i;
    CComHeapPtr<CATCACHEHDR> buffer;
    GUID *guidArray;

    if (!fLocalDsa)
        return FALSE;

    dataSize = 0;
    if (!StringFromGUID2(fCategory, guidStr, MAX_PATH))
        return FALSE;

    wsprintf(bufKey, L"%s\\%s\\%s", REGPATH , guidStr, L"Enum");

    // Try to read key and get proper value size
    if (SHGetValue(HKEY_CURRENT_USER, bufKey, IMPLEMENTING, NULL, NULL, &dataSize))
        return FALSE;

    buffer.Attach((PCATCACHEHDR)CoTaskMemAlloc(dataSize));

    SHGetValue(HKEY_CURRENT_USER, bufKey, IMPLEMENTING, NULL, buffer, &dataSize);
    guidArray = (GUID*)(buffer + 1);
    for (i = 0; i < buffer->classCount; i++)
    {
        // Add class to cache
        DSA_InsertItem(fLocalDsa, DSA_APPEND, guidArray + i);
    }

    return TRUE;
}

HRESULT CComCatCachedCategory::CacheDSA()
{
    WCHAR                               bufKey[MAX_PATH];
    WCHAR                               guidStr[MAX_PATH];
    UINT                                elemCount;
    UINT                                i;
    UINT                                bufferSize;
    CComHeapPtr<CATCACHEHDR>            buffer;
    GUID                                *guidArray;
    GUID                                *tmp;

    elemCount = DSA_GetItemCount(fLocalDsa);
    bufferSize = sizeof(CATCACHEHDR) + elemCount * sizeof(GUID);
    if (!StringFromGUID2(fCategory, guidStr, MAX_PATH))
        return E_FAIL;

    buffer.Attach((PCATCACHEHDR)CoTaskMemAlloc(bufferSize));
    if (!buffer)
        return E_OUTOFMEMORY;

    // Correctly fill cache header
    buffer->dwSize = sizeof(CATCACHEHDR);
    buffer->version = 1;
    GetSystemTime(&buffer->writeTime);
    buffer->classCount = (DWORD)elemCount;

    guidArray = (GUID*)(buffer + 1);
    wsprintf(bufKey, L"%s\\%s\\%s", REGPATH , guidStr, L"Enum");

    // Write DSA contents inside the memory buffer allocated
    for(i = 0; i < elemCount; i++)
    {
        tmp = (GUID*)DSA_GetItemPtr(fLocalDsa, i);
        if (tmp)
        {
            guidArray[i] = *tmp;
        }
    }

    // Save items to registry
    SHSetValue(HKEY_CURRENT_USER, bufKey, IMPLEMENTING, REG_BINARY, buffer, bufferSize);

    guidArray = NULL;
    return S_OK;
}

HRESULT CComCatCachedCategory::LoadFromComCatMgr()
{
    HRESULT hr;
    CComPtr<ICatInformation> pCatInformation;
    CComPtr<IEnumGUID> pEnumGUID;
    ULONG pFetched;
    CLSID tmp;

    // Get component categories manager instance
    hr = CoCreateInstance(CLSID_StdComponentCategoriesMgr, NULL, CLSCTX_INPROC_SERVER,
        IID_PPV_ARG(ICatInformation, &pCatInformation));
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    // Get the proper enumerator
    hr = pCatInformation->EnumClassesOfCategories(1, &fCategory, NULL, NULL, &pEnumGUID);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    // Enumerate elements
    do
    {
        pFetched = 0;
        pEnumGUID->Next(1, &tmp, &pFetched);
        if (pFetched)
        {
            if (DSA_InsertItem(fLocalDsa, DSA_APPEND, &tmp) == E_OUTOFMEMORY)
                return E_OUTOFMEMORY;
        }
    }
    while (pFetched > 0);
    return S_OK;
}

HRESULT CComCatCachedCategory::WriteCacheToDSA(HDSA pDest)
{
    INT i;
    for(i = 0; i < DSA_GetItemCount(fLocalDsa); i++)
    {
        if (DSA_InsertItem(pDest, DSA_APPEND, DSA_GetItemPtr(fLocalDsa, i)) == DSA_ERR)
            return E_OUTOFMEMORY;
    }
    return S_OK;
}

class CSHEnumClassesOfCategories :
    public CComCoClass<CSHEnumClassesOfCategories>,
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IEnumGUID
{
    private:
        CComPtr<ICatInformation> fCatInformation;
        HDSA fDsa;
        ULONG fCursor;

    public:
        CSHEnumClassesOfCategories();
        virtual ~CSHEnumClassesOfCategories();
        virtual HRESULT STDMETHODCALLTYPE Initialize(ULONG cImplemented, CATID *pImplemented, ULONG cRequired, CATID *pRequired);
        // *** IEnumGUID methods ***
        virtual HRESULT STDMETHODCALLTYPE Clone(IEnumCLSID **ppvOut);
        virtual HRESULT STDMETHODCALLTYPE Next(ULONG cElt, CLSID *pElts, ULONG *pFetched);
        virtual HRESULT STDMETHODCALLTYPE Reset();
        virtual HRESULT STDMETHODCALLTYPE Skip(ULONG nbElts);

        BEGIN_COM_MAP(CSHEnumClassesOfCategories)
            COM_INTERFACE_ENTRY_IID(IID_IEnumGUID, IEnumGUID)
        END_COM_MAP()
};

CSHEnumClassesOfCategories::CSHEnumClassesOfCategories()
{
    fCursor = 0;
    fDsa = DSA_Create(sizeof(GUID), 5);
}

CSHEnumClassesOfCategories::~CSHEnumClassesOfCategories()
{
    if (fDsa)
        DSA_Destroy(fDsa);
}

HRESULT CSHEnumClassesOfCategories::Initialize(ULONG cImplemented, CATID *pImplemented, ULONG cRequired, CATID *pRequired)
{
    UINT i;
    HRESULT hr;

    if (!fDsa)
        return E_FAIL;

    if (cRequired > 0 || cImplemented == (ULONG)-1)
    {
        FIXME("Implement required categories class enumeration\n");
        return E_NOTIMPL;
    }

    // Don't do anything if we have nothing
    if (cRequired == 0 && cImplemented == (ULONG)-1)
        return E_FAIL;

    // For each implemented category, create a cache and add it to our local DSA
    for (i = 0; i < cImplemented; i++)
    {
        CComCatCachedCategory cachedCat;
        hr = cachedCat.Initialize(pImplemented[i], FALSE);
        if (FAILED_UNEXPECTEDLY(hr))
            return hr;
        cachedCat.WriteCacheToDSA(fDsa);
    }
    return S_OK;
}

// *** IEnumGUID methods ***

HRESULT STDMETHODCALLTYPE CSHEnumClassesOfCategories::Clone(IEnumCLSID **ppvOut)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CSHEnumClassesOfCategories::Next(ULONG cElt, CLSID *pElts, ULONG *pFetched)
{
    ULONG i;
    ULONG read;
    GUID *tmp;

    if (!pElts)
        return E_INVALIDARG;
    read = 0;
    for (i = 0; i < cElt && (fCursor < (ULONG)DSA_GetItemCount(fDsa)); i++)
    {
        tmp = (GUID*)DSA_GetItemPtr(fDsa, fCursor + i);
        if (!tmp)
            break;
        pElts[i] = *tmp;
        read++;
    }
    fCursor += read;
    if (pFetched)
        *pFetched = read;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CSHEnumClassesOfCategories::Reset()
{
    fCursor = 0;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CSHEnumClassesOfCategories::Skip(ULONG nbElts)
{
    if (fCursor + nbElts >= (ULONG)DSA_GetItemCount(fDsa))
        return E_INVALIDARG;
    fCursor += nbElts;
    return S_OK;
}

/*************************************************************************
 * SHEnumClassesOfCategories	[BROWSEUI.136]
 */
extern "C" HRESULT WINAPI SHEnumClassesOfCategories(ULONG cImplemented, CATID *pImplemented, ULONG cRequired, CATID *pRequired, IEnumGUID **out)
{
    HRESULT hr;

    hr = ShellObjectCreatorInit<CSHEnumClassesOfCategories>(
            cImplemented, pImplemented, cRequired, pRequired, IID_PPV_ARG(IEnumGUID, out));
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;
    return S_OK;
}
