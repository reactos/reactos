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

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "winreg.h"
#include "winerror.h"

#include "quartz_private.h"

#include "ole2.h"
#include "olectl.h"
#include "strmif.h"
#include "uuids.h"
#include "initguid.h"
#include "wine/fil_data.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(quartz);

struct enum_reg_filters
{
    IEnumRegFilters IEnumRegFilters_iface;
    LONG refcount;

    unsigned int index, count;
    REGFILTER *filters;
};

static struct enum_reg_filters *impl_from_IEnumRegFilters(IEnumRegFilters *iface)
{
    return CONTAINING_RECORD(iface, struct enum_reg_filters, IEnumRegFilters_iface);
}

static HRESULT WINAPI enum_reg_filters_QueryInterface(IEnumRegFilters *iface, REFIID iid, void **out)
{
    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_IUnknown) || IsEqualGUID(iid, &IID_IEnumRegFilters))
    {
        IEnumRegFilters_AddRef(iface);
        *out = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI enum_reg_filters_AddRef(IEnumRegFilters *iface)
{
    struct enum_reg_filters *enumerator = impl_from_IEnumRegFilters(iface);
    ULONG refcount = InterlockedIncrement(&enumerator->refcount);
    TRACE("%p increasing refcount to %lu.\n", enumerator, refcount);
    return refcount;
}

static ULONG WINAPI enum_reg_filters_Release(IEnumRegFilters *iface)
{
    struct enum_reg_filters *enumerator = impl_from_IEnumRegFilters(iface);
    ULONG refcount = InterlockedDecrement(&enumerator->refcount);
    unsigned int i;

    TRACE("%p decreasing refcount to %lu.\n", enumerator, refcount);
    if (!refcount)
    {
        for (i = 0; i < enumerator->count; ++i)
            free(enumerator->filters[i].Name);
        free(enumerator->filters);
        free(enumerator);
    }
    return refcount;
}

static HRESULT WINAPI enum_reg_filters_Next(IEnumRegFilters *iface, ULONG count,
        REGFILTER **filters, ULONG *ret_count)
{
    struct enum_reg_filters *enumerator = impl_from_IEnumRegFilters(iface);
    unsigned int i;

    TRACE("iface %p, count %lu, filters %p, ret_count %p.\n", iface, count, filters, ret_count);

    for (i = 0; i < count && enumerator->index + i < enumerator->count; ++i)
    {
        REGFILTER *filter = &enumerator->filters[enumerator->index + i];

        if (!(filters[i] = CoTaskMemAlloc(sizeof(REGFILTER) + (wcslen(filter->Name) + 1) * sizeof(WCHAR))))
        {
            while (i--)
                CoTaskMemFree(filters[i]);
            memset(filters, 0, count * sizeof(*filters));
            *ret_count = 0;
            return E_OUTOFMEMORY;
        }

        filters[i]->Clsid = filter->Clsid;
        filters[i]->Name = (WCHAR *)(filters[i] + 1);
        wcscpy(filters[i]->Name, filter->Name);
    }

    enumerator->index += i;
    if (ret_count)
        *ret_count = i;
    return i ? S_OK : S_FALSE;
}

static HRESULT WINAPI enum_reg_filters_Skip(IEnumRegFilters *iface, ULONG count)
{
    TRACE("iface %p, count %lu, unimplemented.\n", iface, count);
    return E_NOTIMPL;
}

static HRESULT WINAPI enum_reg_filters_Reset(IEnumRegFilters *iface)
{
    struct enum_reg_filters *enumerator = impl_from_IEnumRegFilters(iface);

    TRACE("iface %p.\n", iface);

    enumerator->index = 0;
    return S_OK;
}

static HRESULT WINAPI enum_reg_filters_Clone(IEnumRegFilters *iface, IEnumRegFilters **out)
{
    TRACE("iface %p, out %p, unimplemented.\n", iface, out);
    return E_NOTIMPL;
}

static const IEnumRegFiltersVtbl enum_reg_filters_vtbl =
{
    enum_reg_filters_QueryInterface,
    enum_reg_filters_AddRef,
    enum_reg_filters_Release,
    enum_reg_filters_Next,
    enum_reg_filters_Skip,
    enum_reg_filters_Reset,
    enum_reg_filters_Clone,
};

static HRESULT enum_reg_filters_create(REGFILTER *filters, unsigned int count, IEnumRegFilters **out)
{
    struct enum_reg_filters *object;
    unsigned int i;

    *out = NULL;

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    if (!(object->filters = malloc(count * sizeof(*object->filters))))
    {
        free(object);
        return E_OUTOFMEMORY;
    }

    for (i = 0; i < count; ++i)
    {
        object->filters[i].Clsid = filters[i].Clsid;
        if (!(object->filters[i].Name = wcsdup(filters[i].Name)))
        {
            while (i--)
                free(object->filters[i].Name);
            free(object->filters);
            free(object);
            return E_OUTOFMEMORY;
        }
    }

    object->IEnumRegFilters_iface.lpVtbl = &enum_reg_filters_vtbl;
    object->refcount = 1;
    object->count = count;

    TRACE("Created enumerator %p.\n", object);
    *out = &object->IEnumRegFilters_iface;
    return S_OK;
}

struct enum_moniker
{
    IEnumMoniker IEnumMoniker_iface;
    LONG refcount;

    unsigned int index, count;
    IMoniker **filters;
};

static struct enum_moniker *impl_from_IEnumMoniker(IEnumMoniker *iface)
{
    return CONTAINING_RECORD(iface, struct enum_moniker, IEnumMoniker_iface);
}

static HRESULT WINAPI enum_moniker_QueryInterface(IEnumMoniker *iface, REFIID iid, void **out)
{
    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_IUnknown) || IsEqualGUID(iid, &IID_IEnumMoniker))
    {
        IEnumMoniker_AddRef(iface);
        *out = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI enum_moniker_AddRef(IEnumMoniker *iface)
{
    struct enum_moniker *enumerator = impl_from_IEnumMoniker(iface);
    ULONG refcount = InterlockedIncrement(&enumerator->refcount);
    TRACE("%p increasing refcount to %lu.\n", enumerator, refcount);
    return refcount;
}

static ULONG WINAPI enum_moniker_Release(IEnumMoniker *iface)
{
    struct enum_moniker *enumerator = impl_from_IEnumMoniker(iface);
    ULONG refcount = InterlockedDecrement(&enumerator->refcount);
    unsigned int i;

    TRACE("%p decreasing refcount to %lu.\n", enumerator, refcount);
    if (!refcount)
    {
        for (i = 0; i < enumerator->count; ++i)
            IMoniker_Release(enumerator->filters[i]);
        free(enumerator->filters);
        free(enumerator);
    }
    return refcount;
}

static HRESULT WINAPI enum_moniker_Next(IEnumMoniker *iface, ULONG count,
        IMoniker **filters, ULONG *ret_count)
{
    struct enum_moniker *enumerator = impl_from_IEnumMoniker(iface);
    unsigned int i;

    TRACE("iface %p, count %lu, filters %p, ret_count %p.\n", iface, count, filters, ret_count);

    for (i = 0; i < count && enumerator->index + i < enumerator->count; ++i)
        IMoniker_AddRef(filters[i] = enumerator->filters[enumerator->index + i]);

    enumerator->index += i;
    if (ret_count)
        *ret_count = i;
    return i ? S_OK : S_FALSE;
}

static HRESULT WINAPI enum_moniker_Skip(IEnumMoniker *iface, ULONG count)
{
    struct enum_moniker *enumerator = impl_from_IEnumMoniker(iface);

    TRACE("iface %p, count %lu.\n", iface, count);

    enumerator->index += count;
    return S_OK;
}

static HRESULT WINAPI enum_moniker_Reset(IEnumMoniker *iface)
{
    struct enum_moniker *enumerator = impl_from_IEnumMoniker(iface);

    TRACE("iface %p.\n", iface);

    enumerator->index = 0;
    return S_OK;
}

static HRESULT WINAPI enum_moniker_Clone(IEnumMoniker *iface, IEnumMoniker **out)
{
    TRACE("iface %p, out %p, unimplemented.\n", iface, out);
    return E_NOTIMPL;
}

static const IEnumMonikerVtbl enum_moniker_vtbl =
{
    enum_moniker_QueryInterface,
    enum_moniker_AddRef,
    enum_moniker_Release,
    enum_moniker_Next,
    enum_moniker_Skip,
    enum_moniker_Reset,
    enum_moniker_Clone,
};

static HRESULT enum_moniker_create(IMoniker **filters, unsigned int count, IEnumMoniker **out)
{
    struct enum_moniker *object;

    *out = NULL;

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    if (!(object->filters = malloc(count * sizeof(*object->filters))))
    {
        free(object);
        return E_OUTOFMEMORY;
    }
    memcpy(object->filters, filters, count * sizeof(*filters));

    object->IEnumMoniker_iface.lpVtbl = &enum_moniker_vtbl;
    object->refcount = 1;
    object->count = count;

    TRACE("Created enumerator %p.\n", object);
    *out = &object->IEnumMoniker_iface;
    return S_OK;
}

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
    DWORD signature; /* e.g. '0pi3' */
    DWORD dwFlags;
    DWORD dwInstances;
    DWORD dwMediaTypes;
    DWORD dwMediums;
    DWORD bCategory; /* is there a category clsid? */
    /* optional: dwOffsetCategoryClsid */
};

struct REG_TYPE
{
    DWORD signature; /* e.g. '0ty3' */
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
static int add_data(struct Vector *v, const void *pData, int size)
{
    int index = v->current;
    if (v->current + size > v->capacity)
    {
        int new_capacity = (v->capacity + size) * 2;
        BYTE *new_data = CoTaskMemRealloc(v->pData, new_capacity);
        if (!new_data) return -1;
        v->capacity = new_capacity;
        v->pData = new_data;
    }
    memcpy(v->pData + v->current, pData, size);
    v->current += size;
    return index;
}

static int find_data(const struct Vector *v, const void *pData, int size)
{
    int index;
    for (index = 0; index + size <= v->current; index++)
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
    FilterMapper3Impl *mapper = impl_from_IUnknown(iface);
    ULONG refcount = InterlockedIncrement(&mapper->ref);

    TRACE("%p increasing refcount to %lu.\n", mapper, refcount);

    return refcount;
}

static ULONG WINAPI Inner_Release(IUnknown *iface)
{
    FilterMapper3Impl *mapper = impl_from_IUnknown(iface);
    ULONG refcount = InterlockedDecrement(&mapper->ref);

    TRACE("%p decreasing refcount to %lu.\n", mapper, refcount);

    if (!refcount)
    {
        CoTaskMemFree(mapper);
    }

    return refcount;
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

static HRESULT WINAPI FilterMapper3_CreateCategory(IFilterMapper3 *iface,
        REFCLSID category, DWORD merit, const WCHAR *description)
{
    WCHAR guidstr[39], keypath[93];
    HKEY key;
    LONG ret;

    TRACE("iface %p, category %s, merit %#lx, description %s.\n", iface,
            debugstr_guid(category), merit, debugstr_w(description));

    StringFromGUID2(category, guidstr, ARRAY_SIZE(guidstr));
    wcscpy(keypath, L"CLSID\\{da4e3da0-d07d-11d0-bd50-00a0c911ce86}\\Instance\\");
    wcscat(keypath, guidstr);

    if ((ret = RegCreateKeyExW(HKEY_CLASSES_ROOT, keypath, 0, NULL, 0, KEY_WRITE, NULL, &key, NULL)))
        return HRESULT_FROM_WIN32(ret);

    if ((ret = RegSetValueExW(key, L"FriendlyName", 0, REG_SZ,
            (const BYTE *)description, (wcslen(description) + 1) * sizeof(WCHAR))))
    {
        RegCloseKey(key);
        return HRESULT_FROM_WIN32(ret);
    }

    if ((ret = RegSetValueExW(key, L"CLSID", 0, REG_SZ, (const BYTE *)guidstr, sizeof(guidstr))))
    {
        RegCloseKey(key);
        return HRESULT_FROM_WIN32(ret);
    }

    if ((ret = RegSetValueExW(key, L"Merit", 0, REG_DWORD, (const BYTE *)&merit, sizeof(DWORD))))
    {
        RegCloseKey(key);
        return HRESULT_FROM_WIN32(ret);
    }

    RegCloseKey(key);
    return S_OK;
}

static HRESULT WINAPI FilterMapper3_UnregisterFilter(IFilterMapper3 *iface,
        const CLSID *category, const WCHAR *instance, REFCLSID clsid)
{
    WCHAR keypath[93];

    TRACE("iface %p, category %s, instance %s, clsid %s.\n",
            iface, debugstr_guid(category), debugstr_w(instance), debugstr_guid(clsid));

    if (!category)
        category = &CLSID_LegacyAmFilterCategory;

    wcscpy(keypath, L"CLSID\\");
    StringFromGUID2(category, keypath + wcslen(keypath), ARRAY_SIZE(keypath) - wcslen(keypath));
    wcscat(keypath, L"\\Instance\\");
    if (instance)
        wcscat(keypath, instance);
    else
        StringFromGUID2(clsid, keypath + wcslen(keypath), ARRAY_SIZE(keypath) - wcslen(keypath));

    return HRESULT_FROM_WIN32(RegDeleteKeyW(HKEY_CLASSES_ROOT, keypath));
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
    rrf.dwPins = prf2->cPins2;
    rrf.dwUnused = 0;

    add_data(&mainStore, &rrf, sizeof(rrf));

    for (i = 0; i < prf2->cPins2; i++)
    {
        size += sizeof(struct REG_RFP);
        if (prf2->rgPins2[i].clsPinCategory)
            size += sizeof(DWORD);
        size += prf2->rgPins2[i].nMediaTypes * sizeof(struct REG_TYPE);
        size += prf2->rgPins2[i].nMediums * sizeof(DWORD);
    }

    for (i = 0; i < prf2->cPins2; i++)
    {
        struct REG_RFP rrfp;
        REGFILTERPINS2 rgPin2 = prf2->rgPins2[i];
        unsigned int j;

        rrfp.signature = MAKEFOURCC('0'+i,'p','i','3');
        rrfp.dwFlags = rgPin2.dwFlags;
        rrfp.dwInstances = rgPin2.cInstances;
        rrfp.dwMediaTypes = rgPin2.nMediaTypes;
        rrfp.dwMediums = rgPin2.nMediums;
        rrfp.bCategory = rgPin2.clsPinCategory ? 1 : 0;

        add_data(&mainStore, &rrfp, sizeof(rrfp));
        if (rrfp.bCategory)
        {
            DWORD index = find_data(&clsidStore, rgPin2.clsPinCategory, sizeof(CLSID));
            if (index == -1)
                index = add_data(&clsidStore, rgPin2.clsPinCategory, sizeof(CLSID));
            index += size;

            add_data(&mainStore, &index, sizeof(index));
        }

        for (j = 0; j < rgPin2.nMediaTypes; j++)
        {
            struct REG_TYPE rt;
            const CLSID * clsMinorType = rgPin2.lpMediaType[j].clsMinorType ? rgPin2.lpMediaType[j].clsMinorType : &MEDIASUBTYPE_NULL;
            rt.signature = MAKEFOURCC('0'+j,'t','y','3');
            rt.dwUnused = 0;
            rt.dwOffsetMajor = find_data(&clsidStore, rgPin2.lpMediaType[j].clsMajorType, sizeof(CLSID));
            if (rt.dwOffsetMajor == -1)
                rt.dwOffsetMajor = add_data(&clsidStore, rgPin2.lpMediaType[j].clsMajorType, sizeof(CLSID));
            rt.dwOffsetMajor += size;
            rt.dwOffsetMinor = find_data(&clsidStore, clsMinorType, sizeof(CLSID));
            if (rt.dwOffsetMinor == -1)
                rt.dwOffsetMinor = add_data(&clsidStore, clsMinorType, sizeof(CLSID));
            rt.dwOffsetMinor += size;

            add_data(&mainStore, &rt, sizeof(rt));
        }

        for (j = 0; j < rgPin2.nMediums; j++)
        {
            DWORD index = find_data(&clsidStore, rgPin2.lpMedium + j, sizeof(REGPINMEDIUM));
            if (index == -1)
                index = add_data(&clsidStore, rgPin2.lpMedium + j, sizeof(REGPINMEDIUM));
            index += size;

            add_data(&mainStore, &index, sizeof(index));
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
        FIXME("Filter registry version %lu is not supported.\n", prrf->dwVersion);
        ZeroMemory(prf2, sizeof(*prf2));
        hr = E_FAIL;
    }

    if (SUCCEEDED(hr))
    {
        TRACE("dwVersion %lu, dwMerit %#lx, dwPins %lu, dwUnused %#lx.\n",
            prrf->dwVersion, prrf->dwMerit, prrf->dwPins, prrf->dwUnused);

        prf2->dwVersion = prrf->dwVersion;
        prf2->dwMerit = prrf->dwMerit;
        prf2->cPins2 = prrf->dwPins;
        rgPins2 = CoTaskMemAlloc(prrf->dwPins * sizeof(*rgPins2));
        prf2->rgPins2 = rgPins2;
        pCurrent += sizeof(struct REG_RF);

        for (i = 0; i < prrf->dwPins; i++)
        {
            struct REG_RFP * prrfp = (struct REG_RFP *)pCurrent;
            REGPINTYPES * lpMediaType;
            REGPINMEDIUM * lpMedium;
            UINT j;

            /* FIXME: check signature */

            TRACE("\tsignature = %s\n", debugstr_fourcc(prrfp->signature));

            TRACE("\tPin %lu: dwFlags %#lx, dwInstances %lu, dwMediaTypes %lu, dwMediums %lu.\n",
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
                TRACE("\t\tsignature = %s\n", debugstr_fourcc(prt->signature));

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
    for (i = 0; i < prf2->cPins2; i++)
    {
        UINT j;
        CoTaskMemFree((void*)prf2->rgPins2[i].clsPinCategory);

        for (j = 0; j < prf2->rgPins2[i].nMediaTypes; j++)
        {
            CoTaskMemFree((LPVOID)prf2->rgPins2[i].lpMediaType[j].clsMajorType);
            CoTaskMemFree((LPVOID)prf2->rgPins2[i].lpMediaType[j].clsMinorType);
        }
        CoTaskMemFree((LPVOID)prf2->rgPins2[i].lpMediaType);
        CoTaskMemFree((LPVOID)prf2->rgPins2[i].lpMedium);
    }
    CoTaskMemFree((LPVOID)prf2->rgPins2);
}

static HRESULT WINAPI FilterMapper3_RegisterFilter(IFilterMapper3 *iface,
        REFCLSID clsid, const WCHAR *name, IMoniker **ret_moniker,
        const CLSID *category, const WCHAR *instance, const REGFILTER2 *prf2)
{
    WCHAR *display_name, clsid_string[39];
    IParseDisplayName *parser;
    IPropertyBag *prop_bag;
    ULONG filter_data_len;
    IMoniker *moniker;
    BYTE *filter_data;
    VARIANT var;
    ULONG eaten;
    HRESULT hr;
    size_t len;
    REGFILTER2 regfilter2;
    REGFILTERPINS2* pregfp2 = NULL;

    TRACE("iface %p, clsid %s, name %s, ret_moniker %p, category %s, instance %s, prf2 %p.\n",
            iface, debugstr_guid(clsid), debugstr_w(name), ret_moniker,
            debugstr_guid(category), debugstr_w(instance), prf2);

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
        regfilter2.cPins2 = prf2->cPins;
        pregfp2 = CoTaskMemAlloc(prf2->cPins * sizeof(REGFILTERPINS2));
        regfilter2.rgPins2 = pregfp2;
        for (i = 0; i < prf2->cPins; i++)
        {
            flags = 0;
            if (prf2->rgPins[i].bRendered)
                flags |= REG_PINFLAG_B_RENDERER;
            if (prf2->rgPins[i].bOutput)
                flags |= REG_PINFLAG_B_OUTPUT;
            if (prf2->rgPins[i].bZero)
                flags |= REG_PINFLAG_B_ZERO;
            if (prf2->rgPins[i].bMany)
                flags |= REG_PINFLAG_B_MANY;
            pregfp2[i].dwFlags = flags;
            pregfp2[i].cInstances = 1;
            pregfp2[i].nMediaTypes = prf2->rgPins[i].nMediaTypes;
            pregfp2[i].lpMediaType = prf2->rgPins[i].lpMediaType;
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

    if (ret_moniker)
        *ret_moniker = NULL;

    if (!category)
        category = &CLSID_LegacyAmFilterCategory;

    StringFromGUID2(clsid, clsid_string, ARRAY_SIZE(clsid_string));

    len = 50 + (instance ? wcslen(instance) : 38) + 1;
    if (!(display_name = malloc(len * sizeof(WCHAR))))
        return E_OUTOFMEMORY;

    wcscpy(display_name, L"@device:sw:");
    StringFromGUID2(category, display_name + wcslen(display_name), len - wcslen(display_name));
    wcscat(display_name, L"\\");
    wcscat(display_name, instance ? instance : clsid_string);

    if (FAILED(hr = CoCreateInstance(&CLSID_CDeviceMoniker, NULL, CLSCTX_INPROC,
            &IID_IParseDisplayName, (void **)&parser)))
    {
        free(display_name);
        return hr;
    }

    if (FAILED(hr = IParseDisplayName_ParseDisplayName(parser, NULL, display_name, &eaten, &moniker)))
    {
        ERR("Failed to parse display name, hr %#lx.\n", hr);
        IParseDisplayName_Release(parser);
        free(display_name);
        return hr;
    }

    IParseDisplayName_Release(parser);

    if (FAILED(hr = IMoniker_BindToStorage(moniker, NULL, NULL, &IID_IPropertyBag, (void **)&prop_bag)))
    {
        ERR("Failed to get property bag, hr %#lx.\n", hr);
        IMoniker_Release(moniker);
        free(display_name);
        return hr;
    }

    V_VT(&var) = VT_BSTR;
    V_BSTR(&var) = SysAllocString(name);
    if (FAILED(hr = IPropertyBag_Write(prop_bag, L"FriendlyName", &var)))
        ERR("Failed to write friendly name, hr %#lx.\n", hr);
    VariantClear(&var);

    V_VT(&var) = VT_BSTR;
    V_BSTR(&var) = SysAllocString(clsid_string);
    if (FAILED(hr = IPropertyBag_Write(prop_bag, L"CLSID", &var)))
        ERR("Failed to write class ID, hr %#lx.\n", hr);
    VariantClear(&var);

    if (SUCCEEDED(FM2_WriteFilterData(&regfilter2, &filter_data, &filter_data_len)))
    {
        V_VT(&var) = VT_ARRAY | VT_UI1;
        if ((V_ARRAY(&var) = SafeArrayCreateVector(VT_UI1, 0, filter_data_len)))
        {
            memcpy(V_ARRAY(&var)->pvData, filter_data, filter_data_len);
            if (FAILED(hr = IPropertyBag_Write(prop_bag, L"FilterData", &var)))
                ERR("Failed to write filter data, hr %#lx.\n", hr);
            VariantClear(&var);
        }

        CoTaskMemFree(filter_data);
    }

    IPropertyBag_Release(prop_bag);
    free(display_name);

    if (ret_moniker)
        *ret_moniker = moniker;
    else
        IMoniker_Release(moniker);

    CoTaskMemFree(pregfp2);

    return S_OK;
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
static int __cdecl mm_compare(const void * left, const void * right)
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

    TRACE("(%p, %#lx, %s, %#lx, %s, %lu, %p, %p, %p, %s, %s, %p, %p, %p)\n",
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
        FIXME("Ignoring flags %#lx.\n", dwFlags);

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
            hrSub = IPropertyBag_Read(pPropBagCat, L"Merit", &var, NULL);

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
                IPropertyBag_Read(pPropBagCat, L"FriendlyName", &temp, NULL);
                TRACE("Considering category %s\n", debugstr_w(V_BSTR(&temp)));
                VariantClear(&temp);
            }

            hrSub = IPropertyBag_Read(pPropBagCat, L"CLSID", &var, NULL);

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
                        IPropertyBag_Read(pPropBag, L"FriendlyName", &temp, NULL);
                        TRACE("Considering filter %s\n", debugstr_w(V_BSTR(&temp)));
                        VariantClear(&temp);
                    }

                    if (SUCCEEDED(hrSub))
                    {
                        hrSub = IPropertyBag_Read(pPropBag, L"FilterData", &var, NULL);
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
                        for (i = 0; (i < rf2.cPins2) && (!bInputMatch || !bOutputMatch); i++)
                        {
                            const REGFILTERPINS2 * rfp2 = rf2.rgPins2 + i;

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
                            add_data(&monikers, &mm, sizeof(mm));
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
        hr = enum_moniker_create(ppMoniker, nMonikerCount, ppEnum);
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

    TRACE("(%p/%p)->(%p, %#lx, %s, %s, %s, %s, %s, %s, %s)\n",
        This,
        iface,
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
        return enum_reg_filters_create(NULL, 0, ppEnum);
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
            hrSub = IPropertyBag_Read(pPropBagCat, L"CLSID", &var, NULL);

        if (SUCCEEDED(hrSub))
            hrSub = CLSIDFromString(V_BSTR(&var), &clsid);

        VariantClear(&var);

        if (SUCCEEDED(hrSub))
            hrSub = IPropertyBag_Read(pPropBagCat, L"FriendlyName", &var, NULL);

        if (SUCCEEDED(hrSub))
        {
            len = (wcslen(V_BSTR(&var)) + 1) * sizeof(WCHAR);
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
        hr = enum_reg_filters_create(regfilters, idx, ppEnum);
    }

    for (idx = 0; idx < nb_mon; idx++)
        CoTaskMemFree(regfilters[idx].Name);
    CoTaskMemFree(regfilters);
    IEnumMoniker_Release(ppEnumMoniker);
    
    return hr;
}


static HRESULT WINAPI FilterMapper_RegisterFilter(IFilterMapper * iface,
        CLSID clsid, const WCHAR *name, DWORD merit)
{
    WCHAR keypath[46], guidstr[39];
    HKEY key;
    LONG ret;

    TRACE("iface %p, clsid %s, name %s, merit %#lx.\n",
            iface, debugstr_guid(&clsid), debugstr_w(name), merit);

    StringFromGUID2(&clsid, guidstr, ARRAY_SIZE(guidstr));

    wcscpy(keypath, L"Filter\\");
    wcscat(keypath, guidstr);
    if ((ret = RegCreateKeyExW(HKEY_CLASSES_ROOT, keypath, 0, NULL, 0, KEY_WRITE, NULL, &key, NULL)))
        return HRESULT_FROM_WIN32(ret);

    if ((ret = RegSetValueExW(key, NULL, 0, REG_SZ, (const BYTE *)name, (wcslen(name) + 1) * sizeof(WCHAR))))
        ERR("Failed to set filter name, error %lu.\n", ret);
    RegCloseKey(key);

    wcscpy(keypath, L"CLSID\\");
    wcscat(keypath, guidstr);
    if (!(ret = RegCreateKeyExW(HKEY_CLASSES_ROOT, keypath, 0, NULL, 0, KEY_WRITE, NULL, &key, NULL)))
    {
        if ((ret = RegSetValueExW(key, L"Merit", 0, REG_DWORD, (const BYTE *)&merit, sizeof(DWORD))))
            ERR("Failed to set merit, error %lu.\n", ret);
        RegCloseKey(key);
    }
    else
        ERR("Failed to create CLSID key, error %lu.\n", ret);

    return S_OK;
}

static HRESULT WINAPI FilterMapper_RegisterFilterInstance(IFilterMapper * iface, CLSID clsid, LPCWSTR szName, CLSID *MRId)
{
    TRACE("(%p)->(%s, %s, %p)\n", iface, debugstr_guid(&clsid), debugstr_w(szName), MRId);

    /* Not implemented in Windows (tested on Win2k) */

    return E_NOTIMPL;
}

static HRESULT WINAPI FilterMapper_RegisterPin(IFilterMapper *iface, CLSID clsid,
        const WCHAR *name, BOOL rendered, BOOL output, BOOL zero, BOOL many,
        CLSID external_filter, const WCHAR *external_pin)
{
    WCHAR keypath[6 + 38 + 1], *pin_keypath;
    HKEY key, pin_key, type_key;
    LONG ret;

    TRACE("iface %p, clsid %s, name %s, rendered %d, output %d, zero %d, "
            "many %d, external_filter %s, external_pin %s.\n",
            iface, debugstr_guid(&clsid), debugstr_w(name), rendered, output,
            zero, many, debugstr_guid(&external_filter), debugstr_w(external_pin));

    wcscpy(keypath, L"CLSID\\");
    StringFromGUID2(&clsid, keypath + wcslen(keypath), ARRAY_SIZE(keypath) - wcslen(keypath));
    if ((ret = RegOpenKeyExW(HKEY_CLASSES_ROOT, keypath, 0, KEY_WRITE, &key)))
        return HRESULT_FROM_WIN32(ret);

    if (!(pin_keypath = malloc((5 + wcslen(name) + 1) * sizeof(WCHAR))))
    {
        RegCloseKey(key);
        return E_OUTOFMEMORY;
    }
    wcscpy(pin_keypath, L"Pins\\");
    wcscat(pin_keypath, name);

    if ((ret = RegCreateKeyExW(key, pin_keypath, 0, NULL, 0, KEY_WRITE, NULL, &pin_key, NULL)))
    {
        ERR("Failed to open pin key, error %lu.\n", ret);
        free(pin_keypath);
        RegCloseKey(key);
        return HRESULT_FROM_WIN32(ret);
    }
    free(pin_keypath);

    if ((ret = RegSetValueExW(pin_key, L"AllowedMany", 0, REG_DWORD, (const BYTE *)&many, sizeof(DWORD))))
        ERR("Failed to set AllowedMany value, error %lu.\n", ret);
    if ((ret = RegSetValueExW(pin_key, L"AllowedZero", 0, REG_DWORD, (const BYTE *)&zero, sizeof(DWORD))))
        ERR("Failed to set AllowedZero value, error %lu.\n", ret);
    if ((ret = RegSetValueExW(pin_key, L"Direction", 0, REG_DWORD, (const BYTE *)&output, sizeof(DWORD))))
        ERR("Failed to set Direction value, error %lu.\n", ret);
    if ((ret = RegSetValueExW(pin_key, L"IsRendered", 0, REG_DWORD, (const BYTE *)&rendered, sizeof(DWORD))))
        ERR("Failed to set IsRendered value, error %lu.\n", ret);

    if (!(ret = RegCreateKeyExW(pin_key, L"Types", 0, NULL, 0, 0, NULL, &type_key, NULL)))
        RegCloseKey(type_key);
    else
        ERR("Failed to create Types subkey, error %lu.\n", ret);

    RegCloseKey(pin_key);
    RegCloseKey(key);

    return S_OK;
}


static HRESULT WINAPI FilterMapper_RegisterPinType(IFilterMapper *iface,
        CLSID clsid, const WCHAR *pin, CLSID majortype, CLSID subtype)
{
    WCHAR *keypath, type_keypath[38 + 1 + 38 + 1];
    HKEY key, type_key;
    size_t len;
    LONG ret;

    TRACE("iface %p, clsid %s, pin %s, majortype %s, subtype %s.\n", iface,
            debugstr_guid(&clsid), debugstr_w(pin), debugstr_guid(&majortype), debugstr_guid(&subtype));

    len = 6 + 38 + 6 + wcslen(pin) + 6 + 1;
    if (!(keypath = malloc(len * sizeof(WCHAR))))
        return E_OUTOFMEMORY;

    wcscpy(keypath, L"CLSID\\");
    StringFromGUID2(&clsid, keypath + wcslen(keypath), len - wcslen(keypath));
    wcscat(keypath, L"\\Pins\\");
    wcscat(keypath, pin);
    wcscat(keypath, L"\\Types");
    if ((ret = RegOpenKeyExW(HKEY_CLASSES_ROOT, keypath, 0, KEY_CREATE_SUB_KEY, &key)))
    {
        free(keypath);
        return HRESULT_FROM_WIN32(ret);
    }
    free(keypath);

    StringFromGUID2(&majortype, type_keypath, ARRAY_SIZE(type_keypath));
    wcscat(type_keypath, L"\\");
    StringFromGUID2(&subtype, type_keypath + wcslen(type_keypath), ARRAY_SIZE(type_keypath) - wcslen(type_keypath));
    if (!(ret = RegCreateKeyExW(key, type_keypath, 0, NULL, 0, 0, NULL, &type_key, NULL)))
        RegCloseKey(type_key);
    else
        ERR("Failed to create type key, error %lu.\n", ret);

    RegCloseKey(key);
    return HRESULT_FROM_WIN32(ret);
}

static HRESULT WINAPI FilterMapper_UnregisterFilter(IFilterMapper *iface, CLSID clsid)
{
    WCHAR guidstr[39], keypath[6 + 38 + 1];
    LONG ret;
    HKEY key;

    TRACE("iface %p, clsid %s.\n", iface, debugstr_guid(&clsid));

    StringFromGUID2(&clsid, guidstr, ARRAY_SIZE(guidstr));

    if ((ret = RegOpenKeyExW(HKEY_CLASSES_ROOT, L"Filter", 0, 0, &key)))
        return HRESULT_FROM_WIN32(ret);
    if ((ret = RegDeleteKeyW(key, guidstr)))
        ERR("Failed to delete filter key, error %lu.\n", ret);
    RegCloseKey(key);

    wcscpy(keypath, L"CLSID\\");
    wcscat(keypath, guidstr);
    if (!(ret = RegOpenKeyExW(HKEY_CLASSES_ROOT, keypath, 0, KEY_WRITE, &key)))
    {
        if ((ret = RegDeleteValueW(key, L"Merit")))
            ERR("Failed to delete Merit value, error %lu.\n", ret);
        if ((ret = RegDeleteTreeW(key, L"Pins")))
            ERR("Failed to delete Pins key, error %lu.\n", ret);
        RegCloseKey(key);
    }
    else
        ERR("Failed to open CLSID key, error %lu.\n", ret);

    return S_OK;
}

static HRESULT WINAPI FilterMapper_UnregisterFilterInstance(IFilterMapper * iface, CLSID MRId)
{
    TRACE("(%p)->(%s)\n", iface, debugstr_guid(&MRId));

    /* Not implemented in Windows (tested on Win2k) */

    return E_NOTIMPL;
}

static HRESULT WINAPI FilterMapper_UnregisterPin(IFilterMapper * iface, CLSID clsid, const WCHAR *name)
{
    WCHAR keypath[6 + 38 + 5 + 1];
    LONG ret;
    HKEY key;

    TRACE("iface %p, clsid %s, name %s.\n", iface, debugstr_guid(&clsid), debugstr_w(name));

    if (!name)
        return E_INVALIDARG;

    wcscpy(keypath, L"CLSID\\");
    StringFromGUID2(&clsid, keypath + wcslen(keypath), ARRAY_SIZE(keypath) - wcslen(keypath));
    wcscat(keypath, L"\\Pins");
    if ((ret = RegOpenKeyExW(HKEY_CLASSES_ROOT, keypath, 0, 0, &key)))
        return HRESULT_FROM_WIN32(ret);

    if ((ret = RegDeleteTreeW(key, name)))
        ERR("Failed to delete subkey, error %lu.\n", ret);

    RegCloseKey(key);

    return S_OK;
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

    TRACE("mapper %p, data %p, size %lu, parsed_data %p.\n", This, pData, cb, ppRegFilter2);

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

HRESULT filter_mapper_create(IUnknown *pUnkOuter, IUnknown **out)
{
    FilterMapper3Impl * pFM2impl;

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

    TRACE("Created filter mapper %p.\n", pFM2impl);
    *out = &pFM2impl->IUnknown_inner;
    return S_OK;
}
