/*
 * IFilterMapper & IFilterMapper3 Implementations
 *
 * Copyright 2003 Robert Shearman
 * Copyright 2004 Christian Costa
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

#include "quartz_private.h"

#include <initguid.h>
#include <fil_data.h>

#undef ARRAYSIZE
#define ARRAYSIZE(array) (sizeof(array)/sizeof((array)[0]))

typedef struct FilterMapper3Impl
{
    IUnknown IUnknown_inner;
    IFilterMapper3 IFilterMapper3_iface;
    IFilterMapper IFilterMapper_iface;
    IAMFilterData IAMFilterData_iface;
    IUnknown *outer_unk;
    LONG ref;
} FilterMapper3Impl;

static inline FilterMapper3Impl *impl_from_IFilterMapper3( IFilterMapper3 *iface )
{
    return CONTAINING_RECORD(iface, FilterMapper3Impl, IFilterMapper3_iface);
}

static inline FilterMapper3Impl *impl_from_IFilterMapper( IFilterMapper *iface )
{
    return CONTAINING_RECORD(iface, FilterMapper3Impl, IFilterMapper_iface);
}

static inline FilterMapper3Impl *impl_from_IAMFilterData( IAMFilterData *iface )
{
    return CONTAINING_RECORD(iface, FilterMapper3Impl, IAMFilterData_iface);
}

static inline FilterMapper3Impl *impl_from_IUnknown( IUnknown *iface )
{
    return CONTAINING_RECORD(iface, FilterMapper3Impl, IUnknown_inner);
}

static const WCHAR wszClsidSlash[] = {'C','L','S','I','D','\\',0};
static const WCHAR wszSlashInstance[] = {'\\','I','n','s','t','a','n','c','e','\\',0};
static const WCHAR wszSlash[] = {'\\',0};

/* CLSID property in media category Moniker */
static const WCHAR wszClsidName[] = {'C','L','S','I','D',0};
/* FriendlyName property in media category Moniker */
static const WCHAR wszFriendlyName[] = {'F','r','i','e','n','d','l','y','N','a','m','e',0};
/* Merit property in media category Moniker (CLSID_ActiveMovieCategories only) */
static const WCHAR wszMeritName[] = {'M','e','r','i','t',0};
/* FilterData property in media category Moniker (not CLSID_ActiveMovieCategories) */
static const WCHAR wszFilterDataName[] = {'F','i','l','t','e','r','D','a','t','a',0};
/* For filters registered with IFilterMapper */
static const WCHAR wszFilterSlash[] = {'F','i','l','t','e','r','\\',0};
static const WCHAR wszFilter[] = {'F','i','l','t','e','r',0};
/* For pins registered with IFilterMapper */
static const WCHAR wszPins[] = {'P','i','n','s',0};
static const WCHAR wszAllowedMany[] = {'A','l','l','o','w','e','d','M','a','n','y',0};
static const WCHAR wszAllowedZero[] = {'A','l','l','o','w','e','d','Z','e','r','o',0};
static const WCHAR wszDirection[] = {'D','i','r','e','c','t','i','o','n',0};
static const WCHAR wszIsRendered[] = {'I','s','R','e','n','d','e','r','e','d',0};
/* For types registered with IFilterMapper */
static const WCHAR wszTypes[] = {'T','y','p','e','s',0};


/* registry format for REGFILTER2 */
struct REG_RF
{
    DWORD dwVersion;
    DWORD dwMerit;
    DWORD dwPins;
    DWORD dwUnused;
};

struct REG_RFP
{
    BYTE signature[4]; /* e.g. "0pi3" */
    DWORD dwFlags;
    DWORD dwInstances;
    DWORD dwMediaTypes;
    DWORD dwMediums;
    DWORD bCategory; /* is there a category clsid? */
    /* optional: dwOffsetCategoryClsid */
};

struct REG_TYPE
{
    BYTE signature[4]; /* e.g. "0ty3" */
    DWORD dwUnused;
    DWORD dwOffsetMajor;
    DWORD dwOffsetMinor;
};

struct MONIKER_MERIT
{
    IMoniker * pMoniker;
    DWORD dwMerit;
};

struct Vector
{
    LPBYTE pData;
    int capacity; /* in bytes */
    int current; /* pointer to next free byte */
};

/* returns the position it was added at */
static int add_data(struct Vector * v, const BYTE * pData, int size)
{
    int index = v->current;
    if (v->current + size > v->capacity)
    {
        LPBYTE pOldData = v->pData;
        v->capacity = (v->capacity + size) * 2;
        v->pData = CoTaskMemAlloc(v->capacity);
        memcpy(v->pData, pOldData, v->current);
        CoTaskMemFree(pOldData);
    }
    memcpy(v->pData + v->current, pData, size);
    v->current += size;
    return index;
}

static int find_data(const struct Vector * v, const BYTE * pData, int size)
{
    int index;
    for (index = 0; index < v->current; index++)
        if (!memcmp(v->pData + index, pData, size))
            return index;
    /* not found */
    return -1;
}

static void delete_vector(struct Vector * v)
{
    CoTaskMemFree(v->pData);
    v->current = 0;
    v->capacity = 0;
}

/*** IUnknown (inner) methods ***/

static HRESULT WINAPI Inner_QueryInterface(IUnknown *iface, REFIID riid, void **ppv)
{
    FilterMapper3Impl *This = impl_from_IUnknown(iface);

    TRACE("(%p)->(%s, %p)\n", This, debugstr_guid(riid), ppv);

    *ppv = NULL;
    if (IsEqualIID(riid, &IID_IUnknown))
        *ppv = &This->IUnknown_inner;
    else if (IsEqualIID(riid, &IID_IFilterMapper2) || IsEqualIID(riid, &IID_IFilterMapper3))
        *ppv = &This->IFilterMapper3_iface;
    else if (IsEqualIID(riid, &IID_IFilterMapper))
        *ppv = &This->IFilterMapper_iface;
    else if (IsEqualIID(riid, &IID_IAMFilterData))
        *ppv = &This->IAMFilterData_iface;

    if (*ppv != NULL)
    {
        IUnknown_AddRef((IUnknown *)*ppv);
        return S_OK;
    }

    FIXME("No interface for %s\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI Inner_AddRef(IUnknown *iface)
{
    FilterMapper3Impl *This = impl_from_IUnknown(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p)->(): new ref = %d\n", This, ref);

    return ref;
}

static ULONG WINAPI Inner_Release(IUnknown *iface)
{
    FilterMapper3Impl *This = impl_from_IUnknown(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(): new ref = %d\n", This, ref);

    if (ref == 0)
        CoTaskMemFree(This);

    return ref;
}

static const IUnknownVtbl IInner_VTable =
{
    Inner_QueryInterface,
    Inner_AddRef,
    Inner_Release
};

static HRESULT WINAPI FilterMapper3_QueryInterface(IFilterMapper3 * iface, REFIID riid, LPVOID *ppv)
{
    FilterMapper3Impl *This = impl_from_IFilterMapper3(iface);

    return IUnknown_QueryInterface(This->outer_unk, riid, ppv);
}

static ULONG WINAPI FilterMapper3_AddRef(IFilterMapper3 * iface)
{
    FilterMapper3Impl *This = impl_from_IFilterMapper3(iface);

    return IUnknown_AddRef(This->outer_unk);
}

static ULONG WINAPI FilterMapper3_Release(IFilterMapper3 * iface)
{
    FilterMapper3Impl *This = impl_from_IFilterMapper3(iface);

    return IUnknown_Release(This->outer_unk);
}

/*** IFilterMapper3 methods ***/

static HRESULT WINAPI FilterMapper3_CreateCategory(
    IFilterMapper3 * iface,
    REFCLSID clsidCategory,
    DWORD dwCategoryMerit,
    LPCWSTR szDescription)
{
    LPWSTR wClsidAMCat = NULL;
    LPWSTR wClsidCategory = NULL;
    WCHAR wszKeyName[ARRAYSIZE(wszClsidSlash)-1 + ARRAYSIZE(wszSlashInstance)-1 + (CHARS_IN_GUID-1) * 2 + 1];
    HKEY hKey = NULL;
    LONG lRet;
    HRESULT hr;

    TRACE("(%s, %x, %s)\n", debugstr_guid(clsidCategory), dwCategoryMerit, debugstr_w(szDescription));

    hr = StringFromCLSID(&CLSID_ActiveMovieCategories, &wClsidAMCat);

    if (SUCCEEDED(hr))
    {
        hr = StringFromCLSID(clsidCategory, &wClsidCategory);
    }

    if (SUCCEEDED(hr))
    {
        strcpyW(wszKeyName, wszClsidSlash);
        strcatW(wszKeyName, wClsidAMCat);
        strcatW(wszKeyName, wszSlashInstance);
        strcatW(wszKeyName, wClsidCategory);

        lRet = RegCreateKeyExW(HKEY_CLASSES_ROOT, wszKeyName, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL);
        hr = HRESULT_FROM_WIN32(lRet);
    }

    if (SUCCEEDED(hr))
    {
        lRet = RegSetValueExW(hKey, wszFriendlyName, 0, REG_SZ, (const BYTE*)szDescription, (strlenW(szDescription) + 1) * sizeof(WCHAR));
        hr = HRESULT_FROM_WIN32(lRet);
    }

    if (SUCCEEDED(hr))
    {
        lRet = RegSetValueExW(hKey, wszClsidName, 0, REG_SZ, (LPBYTE)wClsidCategory, (strlenW(wClsidCategory) + 1) * sizeof(WCHAR));
        hr = HRESULT_FROM_WIN32(lRet);
    }

    if (SUCCEEDED(hr))
    {
        lRet = RegSetValueExW(hKey, wszMeritName, 0, REG_DWORD, (LPBYTE)&dwCategoryMerit, sizeof(dwCategoryMerit));
        hr = HRESULT_FROM_WIN32(lRet);
    }

    RegCloseKey(hKey);
    CoTaskMemFree(wClsidCategory);
    CoTaskMemFree(wClsidAMCat);

    return hr;
}

static HRESULT WINAPI FilterMapper3_UnregisterFilter(
    IFilterMapper3 * iface,
    const CLSID *pclsidCategory,
    const OLECHAR *szInstance,
    REFCLSID Filter)
{
    WCHAR wszKeyName[MAX_PATH];
    LPWSTR wClsidCategory = NULL;
    LPWSTR wFilter = NULL;
    HRESULT hr;

    TRACE("(%p, %s, %s)\n", pclsidCategory, debugstr_w(szInstance), debugstr_guid(Filter));

    if (!pclsidCategory)
        pclsidCategory = &CLSID_LegacyAmFilterCategory;

    hr = StringFromCLSID(pclsidCategory, &wClsidCategory);

    if (SUCCEEDED(hr))
    {
        strcpyW(wszKeyName, wszClsidSlash);
        strcatW(wszKeyName, wClsidCategory);
        strcatW(wszKeyName, wszSlashInstance);
        if (szInstance)
            strcatW(wszKeyName, szInstance);
        else
        {
            hr = StringFromCLSID(Filter, &wFilter);
            if (SUCCEEDED(hr))
                strcatW(wszKeyName, wFilter);
        }
    }

    if (SUCCEEDED(hr))
    {
        LONG lRet = RegDeleteKeyW(HKEY_CLASSES_ROOT, wszKeyName);
        hr = HRESULT_FROM_WIN32(lRet);
    }

    CoTaskMemFree(wClsidCategory);
    CoTaskMemFree(wFilter);

    return hr;
}

static HRESULT FM2_WriteFriendlyName(IPropertyBag * pPropBag, LPCWSTR szName)
{
    VARIANT var;
    HRESULT ret;
    BSTR value;

    V_VT(&var) = VT_BSTR;
    V_BSTR(&var) = value = SysAllocString(szName);

    ret = IPropertyBag_Write(pPropBag, wszFriendlyName, &var);
    SysFreeString(value);

    return ret;
}

static HRESULT FM2_WriteClsid(IPropertyBag * pPropBag, REFCLSID clsid)
{
    LPWSTR wszClsid = NULL;
    VARIANT var;
    HRESULT hr;

    hr = StringFromCLSID(clsid, &wszClsid);

    if (SUCCEEDED(hr))
    {
        V_VT(&var) = VT_BSTR;
        V_BSTR(&var) = wszClsid;
        hr = IPropertyBag_Write(pPropBag, wszClsidName, &var);
    }
    CoTaskMemFree(wszClsid);
    return hr;
}

static HRESULT FM2_WriteFilterData(const REGFILTER2 * prf2, BYTE **ppData, ULONG *pcbData)
{
    int size = sizeof(struct REG_RF);
    unsigned int i;
    struct Vector mainStore = {NULL, 0, 0};
    struct Vector clsidStore = {NULL, 0, 0};
    struct REG_RF rrf;
    HRESULT hr = S_OK;

    rrf.dwVersion = prf2->dwVersion;
    rrf.dwMerit = prf2->dwMerit;
    rrf.dwPins = prf2->u.s2.cPins2;
    rrf.dwUnused = 0;

    add_data(&mainStore, (LPBYTE)&rrf, sizeof(rrf));

    for (i = 0; i < prf2->u.s2.cPins2; i++)
    {
        size += sizeof(struct REG_RFP);
        if (prf2->u.s2.rgPins2[i].clsPinCategory)
            size += sizeof(DWORD);
        size += prf2->u.s2.rgPins2[i].nMediaTypes * sizeof(struct REG_TYPE);
        size += prf2->u.s2.rgPins2[i].nMediums * sizeof(DWORD);
    }

    for (i = 0; i < prf2->u.s2.cPins2; i++)
    {
        struct REG_RFP rrfp;
        REGFILTERPINS2 rgPin2 = prf2->u.s2.rgPins2[i];
        unsigned int j;

        rrfp.signature[0] = '0';
        rrfp.signature[1] = 'p';
        rrfp.signature[2] = 'i';
        rrfp.signature[3] = '3';
        rrfp.signature[0] += i;
        rrfp.dwFlags = rgPin2.dwFlags;
        rrfp.dwInstances = rgPin2.cInstances;
        rrfp.dwMediaTypes = rgPin2.nMediaTypes;
        rrfp.dwMediums = rgPin2.nMediums;
        rrfp.bCategory = rgPin2.clsPinCategory ? 1 : 0;

        add_data(&mainStore, (LPBYTE)&rrfp, sizeof(rrfp));
        if (rrfp.bCategory)
        {
            DWORD index = find_data(&clsidStore, (const BYTE*)rgPin2.clsPinCategory, sizeof(CLSID));
            if (index == -1)
                index = add_data(&clsidStore, (const BYTE*)rgPin2.clsPinCategory, sizeof(CLSID));
            index += size;

            add_data(&mainStore, (LPBYTE)&index, sizeof(index));
        }

        for (j = 0; j < rgPin2.nMediaTypes; j++)
        {
            struct REG_TYPE rt;
            const CLSID * clsMinorType = rgPin2.lpMediaType[j].clsMinorType ? rgPin2.lpMediaType[j].clsMinorType : &MEDIASUBTYPE_NULL;
            rt.signature[0] = '0';
            rt.signature[1] = 't';
            rt.signature[2] = 'y';
            rt.signature[3] = '3';
            rt.signature[0] += j;
            rt.dwUnused = 0;
            rt.dwOffsetMajor = find_data(&clsidStore, (const BYTE*)rgPin2.lpMediaType[j].clsMajorType, sizeof(CLSID));
            if (rt.dwOffsetMajor == -1)
                rt.dwOffsetMajor = add_data(&clsidStore, (const BYTE*)rgPin2.lpMediaType[j].clsMajorType, sizeof(CLSID));
            rt.dwOffsetMajor += size;
            rt.dwOffsetMinor = find_data(&clsidStore, (const BYTE*)clsMinorType, sizeof(CLSID));
            if (rt.dwOffsetMinor == -1)
                rt.dwOffsetMinor = add_data(&clsidStore, (const BYTE*)clsMinorType, sizeof(CLSID));
            rt.dwOffsetMinor += size;

            add_data(&mainStore, (LPBYTE)&rt, sizeof(rt));
        }

        for (j = 0; j < rgPin2.nMediums; j++)
        {
            DWORD index = find_data(&clsidStore, (const BYTE*)(rgPin2.lpMedium + j), sizeof(REGPINMEDIUM));
            if (index == -1)
                index = add_data(&clsidStore, (const BYTE*)(rgPin2.lpMedium + j), sizeof(REGPINMEDIUM));
            index += size;

            add_data(&mainStore, (LPBYTE)&index, sizeof(index));
        }
    }

    if (SUCCEEDED(hr))
    {
        *pcbData = mainStore.current + clsidStore.current;
        *ppData = CoTaskMemAlloc(*pcbData);
        if (!*ppData)
            hr = E_OUTOFMEMORY;
    }

    if (SUCCEEDED(hr))
    {
        memcpy(*ppData, mainStore.pData, mainStore.current);
        memcpy((*ppData) + mainStore.current, clsidStore.pData, clsidStore.current);
    }

    delete_vector(&mainStore);
    delete_vector(&clsidStore);
    return hr;
}

static HRESULT FM2_ReadFilterData(BYTE *pData, REGFILTER2 * prf2)
{
    HRESULT hr = S_OK;
    struct REG_RF * prrf;
    LPBYTE pCurrent;
    DWORD i;
    REGFILTERPINS2 * rgPins2;

    prrf = (struct REG_RF *)pData;
    pCurrent = pData;

    if (prrf->dwVersion != 2)
    {
        FIXME("Filter registry version %d not supported\n", prrf->dwVersion);
        ZeroMemory(prf2, sizeof(*prf2));
        hr = E_FAIL;
    }

    if (SUCCEEDED(hr))
    {
        TRACE("version = %d, merit = %x, #pins = %d, unused = %x\n",
            prrf->dwVersion, prrf->dwMerit, prrf->dwPins, prrf->dwUnused);

        prf2->dwVersion = prrf->dwVersion;
        prf2->dwMerit = prrf->dwMerit;
        prf2->u.s2.cPins2 = prrf->dwPins;
        rgPins2 = CoTaskMemAlloc(prrf->dwPins * sizeof(*rgPins2));
        prf2->u.s2.rgPins2 = rgPins2;
        pCurrent += sizeof(struct REG_RF);

        for (i = 0; i < prrf->dwPins; i++)
        {
            struct REG_RFP * prrfp = (struct REG_RFP *)pCurrent;
            REGPINTYPES * lpMediaType;
            REGPINMEDIUM * lpMedium;
            UINT j;

            /* FIXME: check signature */

            TRACE("\tsignature = %s\n", debugstr_an((const char*)prrfp->signature, 4));

            TRACE("\tpin[%d]: flags = %x, instances = %d, media types = %d, mediums = %d\n",
                i, prrfp->dwFlags, prrfp->dwInstances, prrfp->dwMediaTypes, prrfp->dwMediums);

            rgPins2[i].dwFlags = prrfp->dwFlags;
            rgPins2[i].cInstances = prrfp->dwInstances;
            rgPins2[i].nMediaTypes = prrfp->dwMediaTypes;
            rgPins2[i].nMediums = prrfp->dwMediums;
            pCurrent += sizeof(struct REG_RFP);
            if (prrfp->bCategory)
            {
                CLSID * clsCat = CoTaskMemAlloc(sizeof(CLSID));
                memcpy(clsCat, pData + *(DWORD*)(pCurrent), sizeof(CLSID));
                pCurrent += sizeof(DWORD);
                rgPins2[i].clsPinCategory = clsCat;
            }
            else
                rgPins2[i].clsPinCategory = NULL;

            if (rgPins2[i].nMediaTypes > 0)
                lpMediaType = CoTaskMemAlloc(rgPins2[i].nMediaTypes * sizeof(*lpMediaType));
            else
                lpMediaType = NULL;

            rgPins2[i].lpMediaType = lpMediaType;

            for (j = 0; j < rgPins2[i].nMediaTypes; j++)
            {
                struct REG_TYPE * prt = (struct REG_TYPE *)pCurrent;
                CLSID * clsMajor = CoTaskMemAlloc(sizeof(CLSID));
                CLSID * clsMinor = CoTaskMemAlloc(sizeof(CLSID));

                /* FIXME: check signature */
                TRACE("\t\tsignature = %s\n", debugstr_an((const char*)prt->signature, 4));

                memcpy(clsMajor, pData + prt->dwOffsetMajor, sizeof(CLSID));
                memcpy(clsMinor, pData + prt->dwOffsetMinor, sizeof(CLSID));

                lpMediaType[j].clsMajorType = clsMajor;
                lpMediaType[j].clsMinorType = clsMinor;

                pCurrent += sizeof(*prt);
            }

            if (rgPins2[i].nMediums > 0)
                lpMedium = CoTaskMemAlloc(rgPins2[i].nMediums * sizeof(*lpMedium));
            else
                lpMedium = NULL;

            rgPins2[i].lpMedium = lpMedium;

            for (j = 0; j < rgPins2[i].nMediums; j++)
            {
                DWORD dwOffset = *(DWORD *)pCurrent;

                memcpy(lpMedium + j, pData + dwOffset, sizeof(REGPINMEDIUM));

                pCurrent += sizeof(dwOffset);
            }
        }

    }

    return hr;
}

static void FM2_DeleteRegFilter(REGFILTER2 * prf2)
{
    UINT i;
    for (i = 0; i < prf2->u.s2.cPins2; i++)
    {
        UINT j;
        if (prf2->u.s2.rgPins2[i].clsPinCategory)
            CoTaskMemFree((LPVOID)prf2->u.s2.rgPins2[i].clsPinCategory);

        for (j = 0; j < prf2->u.s2.rgPins2[i].nMediaTypes; j++)
        {
            CoTaskMemFree((LPVOID)prf2->u.s2.rgPins2[i].lpMediaType[j].clsMajorType);
            CoTaskMemFree((LPVOID)prf2->u.s2.rgPins2[i].lpMediaType[j].clsMinorType);
        }
        CoTaskMemFree((LPVOID)prf2->u.s2.rgPins2[i].lpMediaType);
        CoTaskMemFree((LPVOID)prf2->u.s2.rgPins2[i].lpMedium);
    }
    CoTaskMemFree((LPVOID)prf2->u.s2.rgPins2);
}

static HRESULT WINAPI FilterMapper3_RegisterFilter(
    IFilterMapper3 * iface,
    REFCLSID clsidFilter,
    LPCWSTR szName,
    IMoniker **ppMoniker,
    const CLSID *pclsidCategory,
    const OLECHAR *szInstance,
    const REGFILTER2 *prf2)
{
    IParseDisplayName * pParser = NULL;
    IBindCtx * pBindCtx = NULL;
    IMoniker * pMoniker = NULL;
    IPropertyBag * pPropBag = NULL;
    HRESULT hr;
    LPWSTR pwszParseName = NULL;
    LPWSTR pCurrent;
    static const WCHAR wszDevice[] = {'@','d','e','v','i','c','e',':','s','w',':',0};
    int nameLen;
    ULONG ulEaten;
    LPWSTR szClsidTemp = NULL;
    REGFILTER2 regfilter2;
    REGFILTERPINS2* pregfp2 = NULL;

    TRACE("(%s, %s, %p, %s, %s, %p)\n",
        debugstr_guid(clsidFilter),
        debugstr_w(szName),
        ppMoniker,
        debugstr_guid(pclsidCategory),
        debugstr_w(szInstance),
        prf2);

    if (prf2->dwVersion == 2)
    {
        regfilter2 = *prf2;
    }
    else if (prf2->dwVersion == 1)
    {
        ULONG i;
        DWORD flags;
        /* REGFILTER2 structure is converted from version 1 to 2. Tested on Win2k. */
        regfilter2.dwVersion = 2;
        regfilter2.dwMerit = prf2->dwMerit;
        regfilter2.u.s2.cPins2 = prf2->u.s1.cPins;
        pregfp2 = CoTaskMemAlloc(prf2->u.s1.cPins * sizeof(REGFILTERPINS2));
        regfilter2.u.s2.rgPins2 = pregfp2;
        for (i = 0; i < prf2->u.s1.cPins; i++)
        {
            flags = 0;
            if (prf2->u.s1.rgPins[i].bRendered)
                flags |= REG_PINFLAG_B_RENDERER;
            if (prf2->u.s1.rgPins[i].bOutput)
                flags |= REG_PINFLAG_B_OUTPUT;
            if (prf2->u.s1.rgPins[i].bZero)
                flags |= REG_PINFLAG_B_ZERO;
            if (prf2->u.s1.rgPins[i].bMany)
                flags |= REG_PINFLAG_B_MANY;
            pregfp2[i].dwFlags = flags;
            pregfp2[i].cInstances = 1;
            pregfp2[i].nMediaTypes = prf2->u.s1.rgPins[i].nMediaTypes;
            pregfp2[i].lpMediaType = prf2->u.s1.rgPins[i].lpMediaType;
            pregfp2[i].nMediums = 0;
            pregfp2[i].lpMedium = NULL;
            pregfp2[i].clsPinCategory = NULL;
        }
    }
    else
    {
        FIXME("dwVersion other that 1 or 2 not supported at the moment\n");
        return E_NOTIMPL;
    }

    if (ppMoniker)
        *ppMoniker = NULL;

    if (!pclsidCategory)
        /* MSDN mentions the inexistent CLSID_ActiveMovieFilters GUID.
         * In fact this is the CLSID_LegacyAmFilterCategory one */
        pclsidCategory = &CLSID_LegacyAmFilterCategory;

    /* sizeof... will include the null terminator and
     * the + 1 is for the separator ('\\'). The -1 is
     * because CHARS_IN_GUID includes the null terminator
     */
    nameLen = sizeof(wszDevice)/sizeof(wszDevice[0]) + CHARS_IN_GUID - 1 + 1;

    if (szInstance)
        nameLen += strlenW(szInstance);
    else
        nameLen += CHARS_IN_GUID - 1; /* CHARS_IN_GUID includes null terminator */

    pCurrent = pwszParseName = CoTaskMemAlloc(nameLen*sizeof(WCHAR));
    if (!pwszParseName)
        return E_OUTOFMEMORY;

    strcpyW(pwszParseName, wszDevice);
    pCurrent += strlenW(wszDevice);

    hr = StringFromCLSID(pclsidCategory, &szClsidTemp);

    if (SUCCEEDED(hr))
    {
        memcpy(pCurrent, szClsidTemp, CHARS_IN_GUID * sizeof(WCHAR));
        pCurrent += CHARS_IN_GUID - 1;
        pCurrent[0] = '\\';

        if (szInstance)
            strcpyW(pCurrent+1, szInstance);
        else
        {
            CoTaskMemFree(szClsidTemp);
            szClsidTemp = NULL;

            hr = StringFromCLSID(clsidFilter, &szClsidTemp);
            if (SUCCEEDED(hr))
                strcpyW(pCurrent+1, szClsidTemp);
        }
    }

    if (SUCCEEDED(hr))
        hr = CoCreateInstance(&CLSID_CDeviceMoniker, NULL, CLSCTX_INPROC, &IID_IParseDisplayName, (LPVOID *)&pParser);

    if (SUCCEEDED(hr))
        hr = CreateBindCtx(0, &pBindCtx);

    if (SUCCEEDED(hr))
        hr = IParseDisplayName_ParseDisplayName(pParser, pBindCtx, pwszParseName, &ulEaten, &pMoniker);

    if (pBindCtx)
        IBindCtx_Release(pBindCtx);
    if (pParser)
        IParseDisplayName_Release(pParser);

    if (SUCCEEDED(hr))
        hr = IMoniker_BindToStorage(pMoniker, NULL, NULL, &IID_IPropertyBag, (LPVOID)&pPropBag);

    if (SUCCEEDED(hr))
        hr = FM2_WriteFriendlyName(pPropBag, szName);

    if (SUCCEEDED(hr))
        hr = FM2_WriteClsid(pPropBag, clsidFilter);

    if (SUCCEEDED(hr))
    {
        BYTE *pData;
        ULONG cbData;

        hr = FM2_WriteFilterData(&regfilter2, &pData, &cbData);
        if (SUCCEEDED(hr))
        {
            VARIANT var;
            SAFEARRAY *psa;
            SAFEARRAYBOUND saBound;

            saBound.lLbound = 0;
            saBound.cElements = cbData;
            psa = SafeArrayCreate(VT_UI1, 1, &saBound);
            if (!psa)
            {
                ERR("Couldn't create SAFEARRAY\n");
                hr = E_FAIL;
            }

            if (SUCCEEDED(hr))
            {
                LPBYTE pbSAData;
                hr = SafeArrayAccessData(psa, (LPVOID *)&pbSAData);
                if (SUCCEEDED(hr))
                {
                    memcpy(pbSAData, pData, cbData);
                    hr = SafeArrayUnaccessData(psa);
                }
            }

            V_VT(&var) = VT_ARRAY | VT_UI1;
            V_ARRAY(&var) = psa;

            if (SUCCEEDED(hr))
                hr = IPropertyBag_Write(pPropBag, wszFilterDataName, &var);

            if (psa)
                SafeArrayDestroy(psa);
            CoTaskMemFree(pData);
        }
    }

    if (pPropBag)
        IPropertyBag_Release(pPropBag);
    CoTaskMemFree(szClsidTemp);
    CoTaskMemFree(pwszParseName);

    if (SUCCEEDED(hr) && ppMoniker)
        *ppMoniker = pMoniker;
    else if (pMoniker)
        IMoniker_Release(pMoniker);

    CoTaskMemFree(pregfp2);

    TRACE("-- returning %x\n", hr);

    return hr;
}

/* internal helper function */
static BOOL MatchTypes(
    BOOL bExactMatch,
    DWORD nPinTypes,
    const REGPINTYPES * pPinTypes,
    DWORD nMatchTypes,
    const GUID * pMatchTypes)
{
    BOOL bMatch = FALSE;
    DWORD j;

    if ((nMatchTypes == 0) && (nPinTypes > 0))
        bMatch = TRUE;

    for (j = 0; j < nPinTypes; j++)
    {
        DWORD i;
        for (i = 0; i < nMatchTypes*2; i+=2)
        {
            if (((!bExactMatch && IsEqualGUID(pPinTypes[j].clsMajorType, &GUID_NULL)) || IsEqualGUID(&pMatchTypes[i], &GUID_NULL) || IsEqualGUID(pPinTypes[j].clsMajorType, &pMatchTypes[i])) &&
                ((!bExactMatch && IsEqualGUID(pPinTypes[j].clsMinorType, &GUID_NULL)) || IsEqualGUID(&pMatchTypes[i+1], &GUID_NULL) || IsEqualGUID(pPinTypes[j].clsMinorType, &pMatchTypes[i+1])))
            {
                bMatch = TRUE;
                break;
            }
        }
    }
    return bMatch;
}

/* internal helper function for qsort of MONIKER_MERIT array */
static int mm_compare(const void * left, const void * right)
{
    const struct MONIKER_MERIT * mmLeft = left;
    const struct MONIKER_MERIT * mmRight = right;

    if (mmLeft->dwMerit == mmRight->dwMerit)
        return 0;
    if (mmLeft->dwMerit > mmRight->dwMerit)
        return -1;
    return 1;
}

/* NOTES:
 *   Exact match means whether or not to treat GUID_NULL's in filter data as wild cards
 *    (GUID_NULL's in input to function automatically treated as wild cards)
 *   Input/Output needed means match only on criteria if TRUE (with zero input types
 *    meaning match any input/output pin as long as one exists), otherwise match any
 *    filter that meets the rest of the requirements.
 */
static HRESULT WINAPI FilterMapper3_EnumMatchingFilters(
    IFilterMapper3 * iface,
    IEnumMoniker **ppEnum,
    DWORD dwFlags,
    BOOL bExactMatch,
    DWORD dwMerit,
    BOOL bInputNeeded,
    DWORD cInputTypes,
    const GUID *pInputTypes,
    const REGPINMEDIUM *pMedIn,
    const CLSID *pPinCategoryIn,
    BOOL bRender,
    BOOL bOutputNeeded,
    DWORD cOutputTypes,
    const GUID *pOutputTypes,
    const REGPINMEDIUM *pMedOut,
    const CLSID *pPinCategoryOut)
{
    ICreateDevEnum * pCreateDevEnum;
    IMoniker * pMonikerCat;
    IEnumMoniker * pEnumCat;
    HRESULT hr;
    struct Vector monikers = {NULL, 0, 0};

    TRACE("(%p, %x, %s, %x, %s, %d, %p, %p, %p, %s, %s, %p, %p, %p)\n",
        ppEnum,
        dwFlags,
        bExactMatch ? "true" : "false",
        dwMerit,
        bInputNeeded ? "true" : "false",
        cInputTypes,
        pInputTypes,
        pMedIn,
        pPinCategoryIn,
        bRender ? "true" : "false",
        bOutputNeeded ? "true" : "false",
        pOutputTypes,
        pMedOut,
        pPinCategoryOut);

    if (dwFlags != 0)
    {
        FIXME("dwFlags = %x not implemented\n", dwFlags);
    }

    *ppEnum = NULL;

    hr = CoCreateInstance(&CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC, &IID_ICreateDevEnum, (LPVOID*)&pCreateDevEnum);
    if (FAILED(hr))
        return hr;

    hr = ICreateDevEnum_CreateClassEnumerator(pCreateDevEnum, &CLSID_ActiveMovieCategories, &pEnumCat, 0);
    if (FAILED(hr)) {
        ICreateDevEnum_Release(pCreateDevEnum);
        return hr;
    }

    while (IEnumMoniker_Next(pEnumCat, 1, &pMonikerCat, NULL) == S_OK)
    {
        IPropertyBag * pPropBagCat = NULL;
        VARIANT var;
        HRESULT hrSub; /* this is so that one buggy filter
                          doesn't make the whole lot fail */

        VariantInit(&var);

        hrSub = IMoniker_BindToStorage(pMonikerCat, NULL, NULL, &IID_IPropertyBag, (LPVOID*)&pPropBagCat);

        if (SUCCEEDED(hrSub))
            hrSub = IPropertyBag_Read(pPropBagCat, wszMeritName, &var, NULL);

        if (SUCCEEDED(hrSub) && (V_UI4(&var) >= dwMerit))
        {
            CLSID clsidCat;
            IEnumMoniker * pEnum;
            IMoniker * pMoniker;

            VariantClear(&var);

            if (TRACE_ON(quartz))
            {
                VARIANT temp;
                V_VT(&temp) = VT_EMPTY;
                IPropertyBag_Read(pPropBagCat, wszFriendlyName, &temp, NULL);
                TRACE("Considering category %s\n", debugstr_w(V_BSTR(&temp)));
                VariantClear(&temp);
            }

            hrSub = IPropertyBag_Read(pPropBagCat, wszClsidName, &var, NULL);

            if (SUCCEEDED(hrSub))
                hrSub = CLSIDFromString(V_BSTR(&var), &clsidCat);

            if (SUCCEEDED(hrSub))
                hrSub = ICreateDevEnum_CreateClassEnumerator(pCreateDevEnum, &clsidCat, &pEnum, 0);

            if (hrSub == S_OK)
            {
                while (IEnumMoniker_Next(pEnum, 1, &pMoniker, NULL) == S_OK)
                {
                    IPropertyBag * pPropBag = NULL;
                    VARIANT var;
                    BYTE *pData = NULL;
                    REGFILTER2 rf2;
                    DWORD i;
                    BOOL bInputMatch = !bInputNeeded;
                    BOOL bOutputMatch = !bOutputNeeded;

                    ZeroMemory(&rf2, sizeof(rf2));
                    VariantInit(&var);

                    hrSub = IMoniker_BindToStorage(pMoniker, NULL, NULL, &IID_IPropertyBag, (LPVOID*)&pPropBag);

                    if (TRACE_ON(quartz))
                    {
                        VARIANT temp;
                        V_VT(&temp) = VT_EMPTY;
                        IPropertyBag_Read(pPropBag, wszFriendlyName, &temp, NULL);
                        TRACE("Considering filter %s\n", debugstr_w(V_BSTR(&temp)));
                        VariantClear(&temp);
                    }

                    if (SUCCEEDED(hrSub))
                    {
                        hrSub = IPropertyBag_Read(pPropBag, wszFilterDataName, &var, NULL);
                    }

                    if (SUCCEEDED(hrSub))
                        hrSub = SafeArrayAccessData(V_ARRAY(&var), (LPVOID*)&pData);

                    if (SUCCEEDED(hrSub))
                        hrSub = FM2_ReadFilterData(pData, &rf2);

                    if (pData)
                        SafeArrayUnaccessData(V_ARRAY(&var));

                    VariantClear(&var);

                    /* Logic used for bInputMatch expression:
                     * There exists some pin such that bInputNeeded implies (pin is an input and
                     * (bRender implies pin has render flag) and major/minor types members of
                     * pInputTypes )
                     * bOutputMatch is similar, but without the "bRender implies ..." part
                     * and substituting variables names containing input for output
                     */

                    /* determine whether filter meets requirements */
                    if (SUCCEEDED(hrSub) && (rf2.dwMerit >= dwMerit))
                    {
                        for (i = 0; (i < rf2.u.s2.cPins2) && (!bInputMatch || !bOutputMatch); i++)
                        {
                            const REGFILTERPINS2 * rfp2 = rf2.u.s2.rgPins2 + i;

                            bInputMatch = bInputMatch || (!(rfp2->dwFlags & REG_PINFLAG_B_OUTPUT) &&
                                (!bRender || (rfp2->dwFlags & REG_PINFLAG_B_RENDERER)) &&
                                MatchTypes(bExactMatch, rfp2->nMediaTypes, rfp2->lpMediaType, cInputTypes, pInputTypes));
                            bOutputMatch = bOutputMatch || ((rfp2->dwFlags & REG_PINFLAG_B_OUTPUT) &&
                                MatchTypes(bExactMatch, rfp2->nMediaTypes, rfp2->lpMediaType, cOutputTypes, pOutputTypes));
                        }

                        if (bInputMatch && bOutputMatch)
                        {
                            struct MONIKER_MERIT mm = {pMoniker, rf2.dwMerit};
                            IMoniker_AddRef(pMoniker);
                            add_data(&monikers, (LPBYTE)&mm, sizeof(mm));
                        }
                    }

                    FM2_DeleteRegFilter(&rf2);
                    if (pPropBag)
                        IPropertyBag_Release(pPropBag);
                    IMoniker_Release(pMoniker);
                }
                IEnumMoniker_Release(pEnum);
            }
        }

        VariantClear(&var);
        if (pPropBagCat)
            IPropertyBag_Release(pPropBagCat);
        IMoniker_Release(pMonikerCat);
    }

    if (SUCCEEDED(hr))
    {
        IMoniker ** ppMoniker;
        unsigned int i;
        ULONG nMonikerCount = monikers.current / sizeof(struct MONIKER_MERIT);

        /* sort the monikers in descending merit order */
        qsort(monikers.pData, nMonikerCount,
              sizeof(struct MONIKER_MERIT),
              mm_compare);

        /* construct an IEnumMoniker interface */
        ppMoniker = CoTaskMemAlloc(nMonikerCount * sizeof(IMoniker *));
        for (i = 0; i < nMonikerCount; i++)
        {
            /* no need to AddRef here as already AddRef'd above */
            ppMoniker[i] = ((struct MONIKER_MERIT *)monikers.pData)[i].pMoniker;
        }
        hr = EnumMonikerImpl_Create(ppMoniker, nMonikerCount, ppEnum);
        CoTaskMemFree(ppMoniker);
    }

    delete_vector(&monikers);
    IEnumMoniker_Release(pEnumCat);
    ICreateDevEnum_Release(pCreateDevEnum);

    return hr;
}

static HRESULT WINAPI FilterMapper3_GetICreateDevEnum(IFilterMapper3 *iface, ICreateDevEnum **ppEnum)
{
    TRACE("(%p, %p)\n", iface, ppEnum);
    if (!ppEnum)
        return E_POINTER;
    return CoCreateInstance(&CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC, &IID_ICreateDevEnum, (void**)ppEnum);
}

static const IFilterMapper3Vtbl fm3vtbl =
{

    FilterMapper3_QueryInterface,
    FilterMapper3_AddRef,
    FilterMapper3_Release,

    FilterMapper3_CreateCategory,
    FilterMapper3_UnregisterFilter,
    FilterMapper3_RegisterFilter,
    FilterMapper3_EnumMatchingFilters,
    FilterMapper3_GetICreateDevEnum
};

/*** IUnknown methods ***/

static HRESULT WINAPI FilterMapper_QueryInterface(IFilterMapper * iface, REFIID riid, LPVOID *ppv)
{
    FilterMapper3Impl *This = impl_from_IFilterMapper(iface);

    TRACE("(%p)->(%s, %p)\n", This, debugstr_guid(riid), ppv);

    return FilterMapper3_QueryInterface(&This->IFilterMapper3_iface, riid, ppv);
}

static ULONG WINAPI FilterMapper_AddRef(IFilterMapper * iface)
{
    FilterMapper3Impl *This = impl_from_IFilterMapper(iface);

    return IUnknown_AddRef(This->outer_unk);
}

static ULONG WINAPI FilterMapper_Release(IFilterMapper * iface)
{
    FilterMapper3Impl *This = impl_from_IFilterMapper(iface);

    return IUnknown_Release(This->outer_unk);
}

/*** IFilterMapper methods ***/

static HRESULT WINAPI FilterMapper_EnumMatchingFilters(
    IFilterMapper * iface,
    IEnumRegFilters **ppEnum,
    DWORD dwMerit,
    BOOL bInputNeeded,
    CLSID clsInMaj,
    CLSID clsInSub,
    BOOL bRender,
    BOOL bOutputNeeded,
    CLSID clsOutMaj,
    CLSID clsOutSub)
{
    FilterMapper3Impl *This = impl_from_IFilterMapper(iface);
    GUID InputType[2];
    GUID OutputType[2];
    IEnumMoniker* ppEnumMoniker;
    IMoniker* IMon;
    ULONG nb;
    ULONG idx = 0, nb_mon = 0;
    REGFILTER* regfilters;
    HRESULT hr;

    TRACE("(%p/%p)->(%p, %x, %s, %s, %s, %s, %s, %s, %s) stub!\n",
        iface,This,
        ppEnum,
        dwMerit,
        bInputNeeded ? "true" : "false",
        debugstr_guid(&clsInMaj),
        debugstr_guid(&clsInSub),
        bRender ? "true" : "false",
        bOutputNeeded ? "true" : "false",
        debugstr_guid(&clsOutMaj),
        debugstr_guid(&clsOutSub));

    InputType[0] = clsInMaj;
    InputType[1] = clsInSub;
    OutputType[0] = clsOutMaj;
    OutputType[1] = clsOutSub;

    *ppEnum = NULL;

    hr = IFilterMapper3_EnumMatchingFilters(&This->IFilterMapper3_iface, &ppEnumMoniker, 0, TRUE,
            dwMerit, bInputNeeded, 1, InputType, NULL, &GUID_NULL, bRender, bOutputNeeded, 1,
            OutputType, NULL, &GUID_NULL);

    if (FAILED(hr))
        return hr;
    
    while(IEnumMoniker_Next(ppEnumMoniker, 1, &IMon, &nb) == S_OK)
    {
        IMoniker_Release(IMon);
        nb_mon++;
    }

    if (!nb_mon)
    {
        IEnumMoniker_Release(ppEnumMoniker);
        return IEnumRegFiltersImpl_Construct(NULL, 0, ppEnum);
    }

    regfilters = CoTaskMemAlloc(nb_mon * sizeof(REGFILTER));
    if (!regfilters)
    {
        IEnumMoniker_Release(ppEnumMoniker);
        return E_OUTOFMEMORY;
    }
    ZeroMemory(regfilters, nb_mon * sizeof(REGFILTER)); /* will prevent bad free of Name in case of error. */
    
    IEnumMoniker_Reset(ppEnumMoniker);
    while(IEnumMoniker_Next(ppEnumMoniker, 1, &IMon, &nb) == S_OK)
    {
        IPropertyBag * pPropBagCat = NULL;
        VARIANT var;
        HRESULT hrSub;
        GUID clsid;
        int len;

        VariantInit(&var);

        hrSub = IMoniker_BindToStorage(IMon, NULL, NULL, &IID_IPropertyBag, (LPVOID*)&pPropBagCat);

        if (SUCCEEDED(hrSub))
            hrSub = IPropertyBag_Read(pPropBagCat, wszClsidName, &var, NULL);

        if (SUCCEEDED(hrSub))
            hrSub = CLSIDFromString(V_BSTR(&var), &clsid);

        VariantClear(&var);

        if (SUCCEEDED(hrSub))
            hrSub = IPropertyBag_Read(pPropBagCat, wszFriendlyName, &var, NULL);

        if (SUCCEEDED(hrSub))
        {
            len = (strlenW(V_BSTR(&var))+1) * sizeof(WCHAR);
            if (!(regfilters[idx].Name = CoTaskMemAlloc(len*2)))
                hr = E_OUTOFMEMORY;
        }

        if (SUCCEEDED(hrSub) && regfilters[idx].Name)
        {
            memcpy(regfilters[idx].Name, V_BSTR(&var), len);
            regfilters[idx].Clsid = clsid;
            idx++;
        }

        if (pPropBagCat)
            IPropertyBag_Release(pPropBagCat);
        IMoniker_Release(IMon);
        VariantClear(&var);
    }

    if (SUCCEEDED(hr))
    {
        hr = IEnumRegFiltersImpl_Construct(regfilters, nb_mon, ppEnum);
    }

    for (idx = 0; idx < nb_mon; idx++)
        CoTaskMemFree(regfilters[idx].Name);
    CoTaskMemFree(regfilters);
    IEnumMoniker_Release(ppEnumMoniker);
    
    return hr;
}


static HRESULT WINAPI FilterMapper_RegisterFilter(IFilterMapper * iface, CLSID clsid, LPCWSTR szName, DWORD dwMerit)
{
    HRESULT hr;
    LPWSTR wszClsid = NULL;
    HKEY hKey;
    LONG lRet;
    WCHAR wszKeyName[ARRAYSIZE(wszFilterSlash)-1 + (CHARS_IN_GUID-1) + 1];

    TRACE("(%p)->(%s, %s, %x)\n", iface, debugstr_guid(&clsid), debugstr_w(szName), dwMerit);

    hr = StringFromCLSID(&clsid, &wszClsid);

    if (SUCCEEDED(hr))
    {
        strcpyW(wszKeyName, wszFilterSlash);
        strcatW(wszKeyName, wszClsid);
    
        lRet = RegCreateKeyExW(HKEY_CLASSES_ROOT, wszKeyName, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL);
        hr = HRESULT_FROM_WIN32(lRet);
    }

    if (SUCCEEDED(hr))
    {
        lRet = RegSetValueExW(hKey, NULL, 0, REG_SZ, (const BYTE*)szName, (strlenW(szName) + 1) * sizeof(WCHAR));
        hr = HRESULT_FROM_WIN32(lRet);
        RegCloseKey(hKey);
    }

    if (SUCCEEDED(hr))
    {
        strcpyW(wszKeyName, wszClsidSlash);
        strcatW(wszKeyName, wszClsid);
    
        lRet = RegCreateKeyExW(HKEY_CLASSES_ROOT, wszKeyName, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL);
        hr = HRESULT_FROM_WIN32(lRet);
    }

    if (SUCCEEDED(hr))
    {
        lRet = RegSetValueExW(hKey, wszMeritName, 0, REG_DWORD, (LPBYTE)&dwMerit, sizeof(dwMerit));
        hr = HRESULT_FROM_WIN32(lRet);
        RegCloseKey(hKey);
    }
    
    CoTaskMemFree(wszClsid);

    return hr;
}

static HRESULT WINAPI FilterMapper_RegisterFilterInstance(IFilterMapper * iface, CLSID clsid, LPCWSTR szName, CLSID *MRId)
{
    TRACE("(%p)->(%s, %s, %p)\n", iface, debugstr_guid(&clsid), debugstr_w(szName), MRId);

    /* Not implemented in Windows (tested on Win2k) */

    return E_NOTIMPL;
}

static HRESULT WINAPI FilterMapper_RegisterPin(
    IFilterMapper * iface,
    CLSID Filter,
    LPCWSTR szName,
    BOOL bRendered,
    BOOL bOutput,
    BOOL bZero,
    BOOL bMany,
    CLSID ConnectsToFilter,
    LPCWSTR ConnectsToPin)
{
    HRESULT hr;
    LONG lRet;
    LPWSTR wszClsid = NULL;
    HKEY hKey = NULL;
    HKEY hPinsKey = NULL;
    WCHAR * wszPinsKeyName;
    WCHAR wszKeyName[ARRAYSIZE(wszClsidSlash)-1 + (CHARS_IN_GUID-1) + 1];

    TRACE("(%p)->(%s, %s, %d, %d, %d, %d, %s, %s)\n", iface, debugstr_guid(&Filter), debugstr_w(szName), bRendered,
                bOutput, bZero, bMany, debugstr_guid(&ConnectsToFilter), debugstr_w(ConnectsToPin));

    hr = StringFromCLSID(&Filter, &wszClsid);

    if (SUCCEEDED(hr))
    {
        strcpyW(wszKeyName, wszClsidSlash);
        strcatW(wszKeyName, wszClsid);

        lRet = RegOpenKeyExW(HKEY_CLASSES_ROOT, wszKeyName, 0, KEY_WRITE, &hKey);
        hr = HRESULT_FROM_WIN32(lRet);
    }

    if (SUCCEEDED(hr))
    {
        wszPinsKeyName = CoTaskMemAlloc((strlenW(wszPins) + 1 + strlenW(szName) + 1) * 2);
        if (!wszPinsKeyName)
             hr = E_OUTOFMEMORY;
    }

    if (SUCCEEDED(hr))
    {
        strcpyW(wszPinsKeyName, wszPins);
        strcatW(wszPinsKeyName, wszSlash);
        strcatW(wszPinsKeyName, szName);
    
        lRet = RegCreateKeyExW(hKey, wszPinsKeyName, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hPinsKey, NULL);
        hr = HRESULT_FROM_WIN32(lRet);
        CoTaskMemFree(wszPinsKeyName);
    }

    if (SUCCEEDED(hr))
    {
        lRet = RegSetValueExW(hPinsKey, wszAllowedMany, 0, REG_DWORD, (LPBYTE)&bMany, sizeof(bMany));
        hr = HRESULT_FROM_WIN32(lRet);
    }

    if (SUCCEEDED(hr))
    {
        lRet = RegSetValueExW(hPinsKey, wszAllowedZero, 0, REG_DWORD, (LPBYTE)&bZero, sizeof(bZero));
        hr = HRESULT_FROM_WIN32(lRet);
    }

    if (SUCCEEDED(hr))
    {
        lRet = RegSetValueExW(hPinsKey, wszDirection, 0, REG_DWORD, (LPBYTE)&bOutput, sizeof(bOutput));
        hr = HRESULT_FROM_WIN32(lRet);
    }

    if (SUCCEEDED(hr))
    {
        lRet = RegSetValueExW(hPinsKey, wszIsRendered, 0, REG_DWORD, (LPBYTE)&bRendered, sizeof(bRendered));
        hr = HRESULT_FROM_WIN32(lRet);
    }

    if (SUCCEEDED(hr))
    {
        HKEY hkeyDummy = NULL;

        lRet = RegCreateKeyExW(hPinsKey, wszTypes, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hkeyDummy, NULL);
        hr = HRESULT_FROM_WIN32(lRet);

        if (hkeyDummy) RegCloseKey(hkeyDummy);
    }

    CoTaskMemFree(wszClsid);
    if (hKey)
        RegCloseKey(hKey);
    if (hPinsKey)
        RegCloseKey(hPinsKey);

    return hr;
}


static HRESULT WINAPI FilterMapper_RegisterPinType(
    IFilterMapper * iface,
    CLSID clsFilter,
    LPCWSTR szName,
    CLSID clsMajorType,
    CLSID clsSubType)
{
    HRESULT hr;
    LONG lRet;
    LPWSTR wszClsid = NULL;
    LPWSTR wszClsidMajorType = NULL;
    LPWSTR wszClsidSubType = NULL;
    HKEY hKey = NULL;
    WCHAR * wszTypesKey;
    WCHAR wszKeyName[MAX_PATH];

    TRACE("(%p)->(%s, %s, %s, %s)\n", iface, debugstr_guid(&clsFilter), debugstr_w(szName),
                    debugstr_guid(&clsMajorType), debugstr_guid(&clsSubType));

    hr = StringFromCLSID(&clsFilter, &wszClsid);

    if (SUCCEEDED(hr))
    {
        hr = StringFromCLSID(&clsMajorType, &wszClsidMajorType);
    }

    if (SUCCEEDED(hr))
    {
        hr = StringFromCLSID(&clsSubType, &wszClsidSubType);
    }

    if (SUCCEEDED(hr))
    {
        wszTypesKey = CoTaskMemAlloc((strlenW(wszClsidSlash) + strlenW(wszClsid) + strlenW(wszPins) +
                        strlenW(szName) + strlenW(wszTypes) + 3 + 1) * 2);
        if (!wszTypesKey)
            hr = E_OUTOFMEMORY;
    }

    if (SUCCEEDED(hr))
    {
        strcpyW(wszTypesKey, wszClsidSlash);
        strcatW(wszTypesKey, wszClsid);
        strcatW(wszTypesKey, wszSlash);
        strcatW(wszTypesKey, wszPins);
        strcatW(wszTypesKey, wszSlash);
        strcatW(wszTypesKey, szName);
        strcatW(wszTypesKey, wszSlash);
        strcatW(wszTypesKey, wszTypes);

        lRet = RegOpenKeyExW(HKEY_CLASSES_ROOT, wszTypesKey, 0, KEY_WRITE, &hKey);
        hr = HRESULT_FROM_WIN32(lRet);
        CoTaskMemFree(wszTypesKey);
    }

    if (SUCCEEDED(hr))
    {
        HKEY hkeyDummy = NULL;

        strcpyW(wszKeyName, wszClsidMajorType);
        strcatW(wszKeyName, wszSlash);
        strcatW(wszKeyName, wszClsidSubType);

        lRet = RegCreateKeyExW(hKey, wszKeyName, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hkeyDummy, NULL);
        hr = HRESULT_FROM_WIN32(lRet);
        RegCloseKey(hKey);

        if (hkeyDummy) RegCloseKey(hkeyDummy);
    }

    CoTaskMemFree(wszClsid);
    CoTaskMemFree(wszClsidMajorType);
    CoTaskMemFree(wszClsidSubType);

    return hr;
}

static HRESULT WINAPI FilterMapper_UnregisterFilter(IFilterMapper * iface, CLSID Filter)
{
    HRESULT hr;
    LONG lRet;
    LPWSTR wszClsid = NULL;
    HKEY hKey;
    WCHAR wszKeyName[ARRAYSIZE(wszClsidSlash)-1 + (CHARS_IN_GUID-1) + 1];

    TRACE("(%p)->(%s)\n", iface, debugstr_guid(&Filter));

    hr = StringFromCLSID(&Filter, &wszClsid);

    if (SUCCEEDED(hr))
    {
        lRet = RegOpenKeyExW(HKEY_CLASSES_ROOT, wszFilter, 0, KEY_WRITE, &hKey);
        hr = HRESULT_FROM_WIN32(lRet);
    }

    if (SUCCEEDED(hr))
    {
        lRet = RegDeleteKeyW(hKey, wszClsid);
        hr = HRESULT_FROM_WIN32(lRet);
        RegCloseKey(hKey);
    }

    if (SUCCEEDED(hr))
    {
        strcpyW(wszKeyName, wszClsidSlash);
        strcatW(wszKeyName, wszClsid);

        lRet = RegOpenKeyExW(HKEY_CLASSES_ROOT, wszKeyName, 0, KEY_WRITE, &hKey);
        hr = HRESULT_FROM_WIN32(lRet);
    }

    if (SUCCEEDED(hr))
    {
        lRet = RegDeleteValueW(hKey, wszMeritName);
        if (lRet != ERROR_SUCCESS)
            hr = HRESULT_FROM_WIN32(lRet);

        lRet = RegDeleteTreeW(hKey, wszPins);
        if (lRet != ERROR_SUCCESS)
            hr = HRESULT_FROM_WIN32(lRet);

        RegCloseKey(hKey);
    }

    CoTaskMemFree(wszClsid);

    return hr;
}

static HRESULT WINAPI FilterMapper_UnregisterFilterInstance(IFilterMapper * iface, CLSID MRId)
{
    TRACE("(%p)->(%s)\n", iface, debugstr_guid(&MRId));

    /* Not implemented in Windows (tested on Win2k) */

    return E_NOTIMPL;
}

static HRESULT WINAPI FilterMapper_UnregisterPin(IFilterMapper * iface, CLSID Filter, LPCWSTR Name)
{
    HRESULT hr;
    LONG lRet;
    LPWSTR wszClsid = NULL;
    HKEY hKey = NULL;
    WCHAR * wszPinNameKey;
    WCHAR wszKeyName[ARRAYSIZE(wszClsidSlash)-1 + (CHARS_IN_GUID-1) + 1];

    TRACE("(%p)->(%s, %s)\n", iface, debugstr_guid(&Filter), debugstr_w(Name));

    if (!Name)
        return E_INVALIDARG;

    hr = StringFromCLSID(&Filter, &wszClsid);

    if (SUCCEEDED(hr))
    {
        strcpyW(wszKeyName, wszClsidSlash);
        strcatW(wszKeyName, wszClsid);

        lRet = RegOpenKeyExW(HKEY_CLASSES_ROOT, wszKeyName, 0, KEY_WRITE, &hKey);
        hr = HRESULT_FROM_WIN32(lRet);
    }

    if (SUCCEEDED(hr))
    {
        wszPinNameKey = CoTaskMemAlloc((strlenW(wszPins) + 1 + strlenW(Name) + 1) * 2);
        if (!wszPinNameKey)
            hr = E_OUTOFMEMORY;
    }

    if (SUCCEEDED(hr))
    {
        strcpyW(wszPinNameKey, wszPins);
        strcatW(wszPinNameKey, wszSlash);
        strcatW(wszPinNameKey, Name);

        lRet = RegDeleteTreeW(hKey, wszPinNameKey);
        hr = HRESULT_FROM_WIN32(lRet);
        CoTaskMemFree(wszPinNameKey);
    }

    CoTaskMemFree(wszClsid);
    if (hKey)
        RegCloseKey(hKey);

    return hr;
}

static const IFilterMapperVtbl fmvtbl =
{

    FilterMapper_QueryInterface,
    FilterMapper_AddRef,
    FilterMapper_Release,

    FilterMapper_RegisterFilter,
    FilterMapper_RegisterFilterInstance,
    FilterMapper_RegisterPin,
    FilterMapper_RegisterPinType,
    FilterMapper_UnregisterFilter,
    FilterMapper_UnregisterFilterInstance,
    FilterMapper_UnregisterPin,
    FilterMapper_EnumMatchingFilters
};


/*** IUnknown methods ***/
static HRESULT WINAPI AMFilterData_QueryInterface(IAMFilterData * iface, REFIID riid, LPVOID *ppv)
{
    FilterMapper3Impl *This = impl_from_IAMFilterData(iface);

    return IUnknown_QueryInterface(This->outer_unk, riid, ppv);
}

static ULONG WINAPI AMFilterData_AddRef(IAMFilterData * iface)
{
    FilterMapper3Impl *This = impl_from_IAMFilterData(iface);

    return IUnknown_AddRef(This->outer_unk);
}

static ULONG WINAPI AMFilterData_Release(IAMFilterData * iface)
{
    FilterMapper3Impl *This = impl_from_IAMFilterData(iface);

    return IUnknown_Release(This->outer_unk);
}

/*** IAMFilterData methods ***/
static HRESULT WINAPI AMFilterData_ParseFilterData(IAMFilterData* iface,
                                                   BYTE *pData, ULONG cb,
                                                   BYTE **ppRegFilter2)
{
    FilterMapper3Impl *This = impl_from_IAMFilterData(iface);
    HRESULT hr = S_OK;
    static REGFILTER2 *prf2;

    TRACE("(%p/%p)->(%p, %d, %p)\n", This, iface, pData, cb, ppRegFilter2);

    prf2 = CoTaskMemAlloc(sizeof(*prf2));
    if (!prf2)
        return E_OUTOFMEMORY;
    *ppRegFilter2 = (BYTE *)&prf2;

    hr = FM2_ReadFilterData(pData, prf2);
    if (FAILED(hr))
    {
        CoTaskMemFree(prf2);
        *ppRegFilter2 = NULL;
    }

    return hr;
}

static HRESULT WINAPI AMFilterData_CreateFilterData(IAMFilterData* iface,
                                                    REGFILTER2 *prf2,
                                                    BYTE **pRegFilterData,
                                                    ULONG *pcb)
{
    FilterMapper3Impl *This = impl_from_IAMFilterData(iface);

    TRACE("(%p/%p)->(%p, %p, %p)\n", This, iface, prf2, pRegFilterData, pcb);

    return FM2_WriteFilterData(prf2, pRegFilterData, pcb);
}

static const IAMFilterDataVtbl AMFilterDataVtbl = {
    AMFilterData_QueryInterface,
    AMFilterData_AddRef,
    AMFilterData_Release,
    AMFilterData_ParseFilterData,
    AMFilterData_CreateFilterData
};

HRESULT FilterMapper2_create(IUnknown *pUnkOuter, LPVOID *ppObj)
{
    FilterMapper3Impl * pFM2impl;

    TRACE("(%p, %p)\n", pUnkOuter, ppObj);

    pFM2impl = CoTaskMemAlloc(sizeof(*pFM2impl));
    if (!pFM2impl)
        return E_OUTOFMEMORY;

    pFM2impl->IUnknown_inner.lpVtbl = &IInner_VTable;
    pFM2impl->IFilterMapper3_iface.lpVtbl = &fm3vtbl;
    pFM2impl->IFilterMapper_iface.lpVtbl = &fmvtbl;
    pFM2impl->IAMFilterData_iface.lpVtbl = &AMFilterDataVtbl;
    pFM2impl->ref = 1;

    if (pUnkOuter)
        pFM2impl->outer_unk = pUnkOuter;
    else
        pFM2impl->outer_unk = &pFM2impl->IUnknown_inner;

    *ppObj = &pFM2impl->IUnknown_inner;

    TRACE("-- created at %p\n", pFM2impl);

    return S_OK;
}

HRESULT FilterMapper_create(IUnknown *pUnkOuter, LPVOID *ppObj)
{
    FilterMapper3Impl *pFM2impl;
    HRESULT hr;

    TRACE("(%p, %p)\n", pUnkOuter, ppObj);

    hr = FilterMapper2_create(pUnkOuter, (LPVOID*)&pFM2impl);
    if (FAILED(hr))
        return hr;

    *ppObj = &pFM2impl->IFilterMapper_iface;

    return hr;
}
