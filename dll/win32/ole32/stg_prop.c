/*
 * Compound Storage (32 bit version)
 * Storage implementation
 *
 * This file contains the compound file implementation
 * of the storage interface.
 *
 * Copyright 1999 Francis Beaudet
 * Copyright 1999 Sylvain St-Germain
 * Copyright 1999 Thuy Nguyen
 * Copyright 2005 Mike McCormack
 * Copyright 2005 Juan Lang
 * Copyright 2006 Mike McCormack
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
 *
 * TODO:
 * - I don't honor the maximum property set size.
 * - Certain bogus files could result in reading past the end of a buffer.
 * - Mac-generated files won't be read correctly, even if they're little
 *   endian, because I disregard whether the generator was a Mac.  This means
 *   strings will probably be munged (as I don't understand Mac scripts.)
 * - Not all PROPVARIANT types are supported.
 * - User defined properties are not supported, see comment in
 *   PropertyStorage_ReadFromStream
 */

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define COBJMACROS
#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "winuser.h"
#include "wine/asm.h"
#include "wine/debug.h"
#include "wine/heap.h"
#include "dictionary.h"
#include "storage32.h"
#include "oleauto.h"

WINE_DEFAULT_DEBUG_CHANNEL(storage);

static inline StorageImpl *impl_from_IPropertySetStorage( IPropertySetStorage *iface )
{
    return CONTAINING_RECORD(iface, StorageImpl, base.IPropertySetStorage_iface);
}

/* These are documented in MSDN,
 * but they don't seem to be in any header file.
 */
#define PROPSETHDR_BYTEORDER_MAGIC      0xfffe
#define PROPSETHDR_OSVER_KIND_WIN16     0
#define PROPSETHDR_OSVER_KIND_MAC       1
#define PROPSETHDR_OSVER_KIND_WIN32     2

#define CP_UNICODE 1200

#define MAX_VERSION_0_PROP_NAME_LENGTH 256

#define CFTAG_WINDOWS   (-1L)
#define CFTAG_MACINTOSH (-2L)
#define CFTAG_FMTID     (-3L)
#define CFTAG_NODATA      0L

#define ALIGNED_LENGTH(_Len, _Align) (((_Len)+(_Align))&~(_Align))

typedef struct tagPROPERTYSETHEADER
{
    WORD  wByteOrder; /* always 0xfffe */
    WORD  wFormat;    /* can be zero or one */
    DWORD dwOSVer;    /* OS version of originating system */
    CLSID clsid;      /* application CLSID */
    DWORD reserved;   /* always 1 */
} PROPERTYSETHEADER;

typedef struct tagFORMATIDOFFSET
{
    FMTID fmtid;
    DWORD dwOffset; /* from beginning of stream */
} FORMATIDOFFSET;

typedef struct tagPROPERTYSECTIONHEADER
{
    DWORD cbSection;
    DWORD cProperties;
} PROPERTYSECTIONHEADER;

typedef struct tagPROPERTYIDOFFSET
{
    DWORD propid;
    DWORD dwOffset; /* from beginning of section */
} PROPERTYIDOFFSET;

typedef struct tagPropertyStorage_impl PropertyStorage_impl;

/* Initializes the property storage from the stream (and undoes any uncommitted
 * changes in the process.)  Returns an error if there is an error reading or
 * if the stream format doesn't match what's expected.
 */
static HRESULT PropertyStorage_ReadFromStream(PropertyStorage_impl *);

static HRESULT PropertyStorage_WriteToStream(PropertyStorage_impl *);

/* Creates the dictionaries used by the property storage.  If successful, all
 * the dictionaries have been created.  If failed, none has been.  (This makes
 * it a bit easier to deal with destroying them.)
 */
static HRESULT PropertyStorage_CreateDictionaries(PropertyStorage_impl *);

static void PropertyStorage_DestroyDictionaries(PropertyStorage_impl *);

/* Copies from propvar to prop.  If propvar's type is VT_LPSTR, copies the
 * string using PropertyStorage_StringCopy.
 */
static HRESULT PropertyStorage_PropVariantCopy(PROPVARIANT *prop,
 const PROPVARIANT *propvar, UINT targetCP, UINT srcCP);

/* Copies the string src, which is encoded using code page srcCP, and returns
 * it in *dst, in the code page specified by targetCP.  The returned string is
 * allocated using CoTaskMemAlloc.
 * If srcCP is CP_UNICODE, src is in fact an LPCWSTR.  Similarly, if targetCP
 * is CP_UNICODE, the returned string is in fact an LPWSTR.
 * Returns S_OK on success, something else on failure.
 */
static HRESULT PropertyStorage_StringCopy(LPCSTR src, UINT srcCP, LPSTR *dst,
 UINT targetCP);

static const IPropertyStorageVtbl IPropertyStorage_Vtbl;

/***********************************************************************
 * Implementation of IPropertyStorage
 */
struct tagPropertyStorage_impl
{
    IPropertyStorage IPropertyStorage_iface;
    LONG ref;
    CRITICAL_SECTION cs;
    IStream *stm;
    BOOL  dirty;
    FMTID fmtid;
    CLSID clsid;
    WORD  format;
    DWORD originatorOS;
    DWORD grfFlags;
    DWORD grfMode;
    UINT  codePage;
    LCID  locale;
    PROPID highestProp;
    struct dictionary *name_to_propid;
    struct dictionary *propid_to_name;
    struct dictionary *propid_to_prop;
};

static inline PropertyStorage_impl *impl_from_IPropertyStorage(IPropertyStorage *iface)
{
    return CONTAINING_RECORD(iface, PropertyStorage_impl, IPropertyStorage_iface);
}

struct enum_stat_prop_stg
{
    IEnumSTATPROPSTG IEnumSTATPROPSTG_iface;
    LONG refcount;
    PropertyStorage_impl *storage;
    STATPROPSTG *stats;
    size_t current;
    size_t count;
};

static struct enum_stat_prop_stg *impl_from_IEnumSTATPROPSTG(IEnumSTATPROPSTG *iface)
{
    return CONTAINING_RECORD(iface, struct enum_stat_prop_stg, IEnumSTATPROPSTG_iface);
}

static HRESULT WINAPI enum_stat_prop_stg_QueryInterface(IEnumSTATPROPSTG *iface, REFIID riid, void **obj)
{
    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), obj);

    if (IsEqualIID(riid, &IID_IEnumSTATPROPSTG) ||
            IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
        IEnumSTATPROPSTG_AddRef(iface);
        return S_OK;
    }

    WARN("Unsupported interface %s.\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI enum_stat_prop_stg_AddRef(IEnumSTATPROPSTG *iface)
{
    struct enum_stat_prop_stg *penum = impl_from_IEnumSTATPROPSTG(iface);
    LONG refcount = InterlockedIncrement(&penum->refcount);

    TRACE("%p, refcount %lu.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI enum_stat_prop_stg_Release(IEnumSTATPROPSTG *iface)
{
    struct enum_stat_prop_stg *penum = impl_from_IEnumSTATPROPSTG(iface);
    LONG refcount = InterlockedDecrement(&penum->refcount);

    TRACE("%p, refcount %lu.\n", iface, refcount);

    if (!refcount)
    {
        IPropertyStorage_Release(&penum->storage->IPropertyStorage_iface);
        heap_free(penum->stats);
        heap_free(penum);
    }

    return refcount;
}

static HRESULT WINAPI enum_stat_prop_stg_Next(IEnumSTATPROPSTG *iface, ULONG celt, STATPROPSTG *ret, ULONG *fetched)
{
    struct enum_stat_prop_stg *penum = impl_from_IEnumSTATPROPSTG(iface);
    ULONG count = 0;
    WCHAR *name;

    TRACE("%p, %lu, %p, %p.\n", iface, celt, ret, fetched);

    if (penum->current == ~0u)
        penum->current = 0;

    while (count < celt && penum->current < penum->count)
    {
        *ret = penum->stats[penum->current++];

        if (dictionary_find(penum->storage->propid_to_name, UlongToPtr(ret->propid), (void **)&name))
        {
            SIZE_T size = (lstrlenW(name) + 1) * sizeof(WCHAR);
            ret->lpwstrName = CoTaskMemAlloc(size);
            if (ret->lpwstrName)
                memcpy(ret->lpwstrName, name, size);
        }
        ret++;
        count++;
    }

    if (fetched)
        *fetched = count;

    return count < celt ? S_FALSE : S_OK;
}

static HRESULT WINAPI enum_stat_prop_stg_Skip(IEnumSTATPROPSTG *iface, ULONG celt)
{
    FIXME("%p, %lu.\n", iface, celt);

    return S_OK;
}

static HRESULT WINAPI enum_stat_prop_stg_Reset(IEnumSTATPROPSTG *iface)
{
    struct enum_stat_prop_stg *penum = impl_from_IEnumSTATPROPSTG(iface);

    TRACE("%p.\n", iface);

    penum->current = ~0u;

    return S_OK;
}

static HRESULT WINAPI enum_stat_prop_stg_Clone(IEnumSTATPROPSTG *iface, IEnumSTATPROPSTG **ppenum)
{
    FIXME("%p, %p.\n", iface, ppenum);

    return E_NOTIMPL;
}

static const IEnumSTATPROPSTGVtbl enum_stat_prop_stg_vtbl =
{
    enum_stat_prop_stg_QueryInterface,
    enum_stat_prop_stg_AddRef,
    enum_stat_prop_stg_Release,
    enum_stat_prop_stg_Next,
    enum_stat_prop_stg_Skip,
    enum_stat_prop_stg_Reset,
    enum_stat_prop_stg_Clone,
};

static BOOL prop_enum_stat(const void *k, const void *v, void *extra, void *arg)
{
    struct enum_stat_prop_stg *stg = arg;
    PROPID propid = PtrToUlong(k);
    const PROPVARIANT *prop = v;
    STATPROPSTG *dest;

    dest = &stg->stats[stg->count];

    dest->lpwstrName = NULL;
    dest->propid = propid;
    dest->vt = prop->vt;
    stg->count++;

    return TRUE;
}

static BOOL prop_enum_stat_count(const void *k, const void *v, void *extra, void *arg)
{
    DWORD *count = arg;

    *count += 1;

    return TRUE;
}

static HRESULT create_enum_stat_prop_stg(PropertyStorage_impl *storage, IEnumSTATPROPSTG **ret)
{
    struct enum_stat_prop_stg *enum_obj;
    DWORD count;

    enum_obj = heap_alloc_zero(sizeof(*enum_obj));
    if (!enum_obj)
        return E_OUTOFMEMORY;

    enum_obj->IEnumSTATPROPSTG_iface.lpVtbl = &enum_stat_prop_stg_vtbl;
    enum_obj->refcount = 1;
    enum_obj->storage = storage;
    IPropertyStorage_AddRef(&storage->IPropertyStorage_iface);

    count = 0;
    dictionary_enumerate(storage->propid_to_prop, prop_enum_stat_count, &count);

    if (count)
    {
        if (!(enum_obj->stats = heap_alloc(sizeof(*enum_obj->stats) * count)))
        {
            IEnumSTATPROPSTG_Release(&enum_obj->IEnumSTATPROPSTG_iface);
            return E_OUTOFMEMORY;
        }

        dictionary_enumerate(storage->propid_to_prop, prop_enum_stat, enum_obj);
    }

    *ret = &enum_obj->IEnumSTATPROPSTG_iface;

    return S_OK;
}

/************************************************************************
 * IPropertyStorage_fnQueryInterface (IPropertyStorage)
 */
static HRESULT WINAPI IPropertyStorage_fnQueryInterface(
    IPropertyStorage *iface,
    REFIID riid,
    void** ppvObject)
{
    PropertyStorage_impl *This = impl_from_IPropertyStorage(iface);

    TRACE("(%p, %s, %p)\n", This, debugstr_guid(riid), ppvObject);

    if (!ppvObject)
        return E_INVALIDARG;

    *ppvObject = 0;

    if (IsEqualGUID(&IID_IUnknown, riid) ||
        IsEqualGUID(&IID_IPropertyStorage, riid))
    {
        IPropertyStorage_AddRef(iface);
        *ppvObject = iface;
        return S_OK;
    }

    return E_NOINTERFACE;
}

/************************************************************************
 * IPropertyStorage_fnAddRef (IPropertyStorage)
 */
static ULONG WINAPI IPropertyStorage_fnAddRef(
    IPropertyStorage *iface)
{
    PropertyStorage_impl *This = impl_from_IPropertyStorage(iface);
    return InterlockedIncrement(&This->ref);
}

/************************************************************************
 * IPropertyStorage_fnRelease (IPropertyStorage)
 */
static ULONG WINAPI IPropertyStorage_fnRelease(
    IPropertyStorage *iface)
{
    PropertyStorage_impl *This = impl_from_IPropertyStorage(iface);
    ULONG ref;

    ref = InterlockedDecrement(&This->ref);
    if (ref == 0)
    {
        TRACE("Destroying %p\n", This);
        if (This->dirty)
            IPropertyStorage_Commit(iface, STGC_DEFAULT);
        IStream_Release(This->stm);
        This->cs.DebugInfo->Spare[0] = 0;
        DeleteCriticalSection(&This->cs);
        PropertyStorage_DestroyDictionaries(This);
        HeapFree(GetProcessHeap(), 0, This);
    }
    return ref;
}

static PROPVARIANT *PropertyStorage_FindProperty(PropertyStorage_impl *This,
 DWORD propid)
{
    PROPVARIANT *ret = NULL;

    dictionary_find(This->propid_to_prop, UlongToPtr(propid), (void **)&ret);
    TRACE("returning %p\n", ret);
    return ret;
}

/* Returns NULL if name is NULL. */
static PROPVARIANT *PropertyStorage_FindPropertyByName(
 PropertyStorage_impl *This, LPCWSTR name)
{
    PROPVARIANT *ret = NULL;
    void *propid;

    if (!name)
        return NULL;
    if (This->codePage == CP_UNICODE)
    {
        if (dictionary_find(This->name_to_propid, name, &propid))
            ret = PropertyStorage_FindProperty(This, PtrToUlong(propid));
    }
    else
    {
        LPSTR ansiName;
        HRESULT hr = PropertyStorage_StringCopy((LPCSTR)name, CP_UNICODE,
         &ansiName, This->codePage);

        if (SUCCEEDED(hr))
        {
            if (dictionary_find(This->name_to_propid, ansiName, &propid))
                ret = PropertyStorage_FindProperty(This, PtrToUlong(propid));
            CoTaskMemFree(ansiName);
        }
    }
    TRACE("returning %p\n", ret);
    return ret;
}

static LPWSTR PropertyStorage_FindPropertyNameById(PropertyStorage_impl *This,
 DWORD propid)
{
    LPWSTR ret = NULL;

    dictionary_find(This->propid_to_name, UlongToPtr(propid), (void **)&ret);
    TRACE("returning %p\n", ret);
    return ret;
}

/************************************************************************
 * IPropertyStorage_fnReadMultiple (IPropertyStorage)
 */
static HRESULT WINAPI IPropertyStorage_fnReadMultiple(
    IPropertyStorage* iface,
    ULONG cpspec,
    const PROPSPEC rgpspec[],
    PROPVARIANT rgpropvar[])
{
    PropertyStorage_impl *This = impl_from_IPropertyStorage(iface);
    HRESULT hr = S_OK;
    ULONG i;

    TRACE("%p, %lu, %p, %p\n", iface, cpspec, rgpspec, rgpropvar);

    if (!cpspec)
        return S_FALSE;
    if (!rgpspec || !rgpropvar)
        return E_INVALIDARG;
    EnterCriticalSection(&This->cs);
    for (i = 0; i < cpspec; i++)
    {
        PropVariantInit(&rgpropvar[i]);
        if (rgpspec[i].ulKind == PRSPEC_LPWSTR)
        {
            PROPVARIANT *prop = PropertyStorage_FindPropertyByName(This,
             rgpspec[i].lpwstr);

            if (prop)
                PropertyStorage_PropVariantCopy(&rgpropvar[i], prop, GetACP(),
                 This->codePage);
        }
        else
        {
            switch (rgpspec[i].propid)
            {
                case PID_CODEPAGE:
                    rgpropvar[i].vt = VT_I2;
                    rgpropvar[i].iVal = This->codePage;
                    break;
                case PID_LOCALE:
                    rgpropvar[i].vt = VT_I4;
                    rgpropvar[i].lVal = This->locale;
                    break;
                default:
                {
                    PROPVARIANT *prop = PropertyStorage_FindProperty(This,
                     rgpspec[i].propid);

                    if (prop)
                        PropertyStorage_PropVariantCopy(&rgpropvar[i], prop,
                         GetACP(), This->codePage);
                    else
                        hr = S_FALSE;
                }
            }
        }
    }
    LeaveCriticalSection(&This->cs);
    return hr;
}

static HRESULT PropertyStorage_StringCopy(LPCSTR src, UINT srcCP, LPSTR *dst, UINT dstCP)
{
    HRESULT hr = S_OK;
    int len;

    TRACE("%s, %p, %d, %d\n",
     srcCP == CP_UNICODE ? debugstr_w((LPCWSTR)src) : debugstr_a(src), dst,
     dstCP, srcCP);
    assert(src);
    assert(dst);
    *dst = NULL;
    if (dstCP == srcCP)
    {
        size_t len;

        if (dstCP == CP_UNICODE)
            len = (lstrlenW((LPCWSTR)src) + 1) * sizeof(WCHAR);
        else
            len = strlen(src) + 1;
        *dst = CoTaskMemAlloc(len);
        if (!*dst)
            hr = STG_E_INSUFFICIENTMEMORY;
        else
            memcpy(*dst, src, len);
    }
    else
    {
        if (dstCP == CP_UNICODE)
        {
            len = MultiByteToWideChar(srcCP, 0, src, -1, NULL, 0);
            *dst = CoTaskMemAlloc(len * sizeof(WCHAR));
            if (!*dst)
                hr = STG_E_INSUFFICIENTMEMORY;
            else
                MultiByteToWideChar(srcCP, 0, src, -1, (LPWSTR)*dst, len);
        }
        else
        {
            LPCWSTR wideStr = NULL;
            LPWSTR wideStr_tmp = NULL;

            if (srcCP == CP_UNICODE)
                wideStr = (LPCWSTR)src;
            else
            {
                len = MultiByteToWideChar(srcCP, 0, src, -1, NULL, 0);
                wideStr_tmp = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
                if (wideStr_tmp)
                {
                    MultiByteToWideChar(srcCP, 0, src, -1, wideStr_tmp, len);
                    wideStr = wideStr_tmp;
                }
                else
                    hr = STG_E_INSUFFICIENTMEMORY;
            }
            if (SUCCEEDED(hr))
            {
                len = WideCharToMultiByte(dstCP, 0, wideStr, -1, NULL, 0,
                 NULL, NULL);
                *dst = CoTaskMemAlloc(len);
                if (!*dst)
                    hr = STG_E_INSUFFICIENTMEMORY;
                else
                {
                    BOOL defCharUsed = FALSE;

                    if (WideCharToMultiByte(dstCP, 0, wideStr, -1, *dst, len,
                     NULL, &defCharUsed) == 0 || defCharUsed)
                    {
                        CoTaskMemFree(*dst);
                        *dst = NULL;
                        hr = HRESULT_FROM_WIN32(ERROR_NO_UNICODE_TRANSLATION);
                    }
                }
            }
            HeapFree(GetProcessHeap(), 0, wideStr_tmp);
        }
    }
    TRACE("returning %#lx (%s)\n", hr,
     dstCP == CP_UNICODE ? debugstr_w((LPCWSTR)*dst) : debugstr_a(*dst));
    return hr;
}

static HRESULT PropertyStorage_PropVariantCopy(PROPVARIANT *prop, const PROPVARIANT *propvar,
        UINT targetCP, UINT srcCP)
{
    HRESULT hr = S_OK;

    assert(prop);
    assert(propvar);

    switch (propvar->vt)
    {
    case VT_LPSTR:
        hr = PropertyStorage_StringCopy(propvar->pszVal, srcCP, &prop->pszVal, targetCP);
        if (SUCCEEDED(hr))
            prop->vt = VT_LPSTR;
        break;
    case VT_BSTR:
        if ((prop->bstrVal = SysAllocStringLen(propvar->bstrVal, SysStringLen(propvar->bstrVal))))
            prop->vt = VT_BSTR;
        else
            hr = E_OUTOFMEMORY;
        break;
    default:
        hr = PropVariantCopy(prop, propvar);
    }

    return hr;
}

/* Stores the property with id propid and value propvar into this property
 * storage.  lcid is ignored if propvar's type is not VT_LPSTR.  If propvar's
 * type is VT_LPSTR, converts the string using lcid as the source code page
 * and This->codePage as the target code page before storing.
 * As a side effect, may change This->format to 1 if the type of propvar is
 * a version 1-only property.
 */
static HRESULT PropertyStorage_StorePropWithId(PropertyStorage_impl *This,
 PROPID propid, const PROPVARIANT *propvar, UINT cp)
{
    HRESULT hr = S_OK;
    PROPVARIANT *prop = PropertyStorage_FindProperty(This, propid);

    assert(propvar);
    if (propvar->vt & VT_BYREF || propvar->vt & VT_ARRAY)
        This->format = 1;
    switch (propvar->vt)
    {
    case VT_DECIMAL:
    case VT_I1:
    case VT_INT:
    case VT_UINT:
    case VT_VECTOR|VT_I1:
        This->format = 1;
    }
    TRACE("Setting %#lx to type %d\n", propid, propvar->vt);
    if (prop)
    {
        PropVariantClear(prop);
        hr = PropertyStorage_PropVariantCopy(prop, propvar, This->codePage, cp);
    }
    else
    {
        prop = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
         sizeof(PROPVARIANT));
        if (prop)
        {
            hr = PropertyStorage_PropVariantCopy(prop, propvar, This->codePage, cp);
            if (SUCCEEDED(hr))
            {
                dictionary_insert(This->propid_to_prop, UlongToPtr(propid), prop);
                if (propid > This->highestProp)
                    This->highestProp = propid;
            }
            else
                HeapFree(GetProcessHeap(), 0, prop);
        }
        else
            hr = STG_E_INSUFFICIENTMEMORY;
    }
    return hr;
}

/* Adds the name srcName to the name dictionaries, mapped to property ID id.
 * srcName is encoded in code page cp, and is converted to This->codePage.
 * If cp is CP_UNICODE, srcName is actually a unicode string.
 * As a side effect, may change This->format to 1 if srcName is too long for
 * a version 0 property storage.
 * Doesn't validate id.
 */
static HRESULT PropertyStorage_StoreNameWithId(PropertyStorage_impl *This,
 LPCSTR srcName, UINT cp, PROPID id)
{
    LPSTR name;
    HRESULT hr;

    assert(srcName);

    hr = PropertyStorage_StringCopy(srcName, cp, &name, This->codePage);
    if (SUCCEEDED(hr))
    {
        if (This->codePage == CP_UNICODE)
        {
            if (lstrlenW((LPWSTR)name) >= MAX_VERSION_0_PROP_NAME_LENGTH)
                This->format = 1;
        }
        else
        {
            if (strlen(name) >= MAX_VERSION_0_PROP_NAME_LENGTH)
                This->format = 1;
        }
        TRACE("Adding prop name %s, propid %ld\n",
         This->codePage == CP_UNICODE ? debugstr_w((LPCWSTR)name) :
         debugstr_a(name), id);
        dictionary_insert(This->name_to_propid, name, UlongToPtr(id));
        dictionary_insert(This->propid_to_name, UlongToPtr(id), name);
    }
    return hr;
}

/************************************************************************
 * IPropertyStorage_fnWriteMultiple (IPropertyStorage)
 */
static HRESULT WINAPI IPropertyStorage_fnWriteMultiple(
    IPropertyStorage* iface,
    ULONG cpspec,
    const PROPSPEC rgpspec[],
    const PROPVARIANT rgpropvar[],
    PROPID propidNameFirst)
{
    PropertyStorage_impl *This = impl_from_IPropertyStorage(iface);
    HRESULT hr = S_OK;
    ULONG i;

    TRACE("%p, %lu, %p, %p.\n", iface, cpspec, rgpspec, rgpropvar);

    if (cpspec && (!rgpspec || !rgpropvar))
        return E_INVALIDARG;
    if (!(This->grfMode & STGM_READWRITE))
        return STG_E_ACCESSDENIED;
    EnterCriticalSection(&This->cs);
    This->dirty = TRUE;
    This->originatorOS = (DWORD)MAKELONG(LOWORD(GetVersion()),
     PROPSETHDR_OSVER_KIND_WIN32) ;
    for (i = 0; i < cpspec; i++)
    {
        if (rgpspec[i].ulKind == PRSPEC_LPWSTR)
        {
            PROPVARIANT *prop = PropertyStorage_FindPropertyByName(This,
             rgpspec[i].lpwstr);

            if (prop)
                PropVariantCopy(prop, &rgpropvar[i]);
            else
            {
                /* Note that I don't do the special cases here that I do below,
                 * because naming the special PIDs isn't supported.
                 */
                if (propidNameFirst < PID_FIRST_USABLE ||
                 propidNameFirst >= PID_MIN_READONLY)
                    hr = STG_E_INVALIDPARAMETER;
                else
                {
                    PROPID nextId = max(propidNameFirst, This->highestProp + 1);

                    hr = PropertyStorage_StoreNameWithId(This,
                     (LPCSTR)rgpspec[i].lpwstr, CP_UNICODE, nextId);
                    if (SUCCEEDED(hr))
                        hr = PropertyStorage_StorePropWithId(This, nextId,
                         &rgpropvar[i], GetACP());
                }
            }
        }
        else
        {
            switch (rgpspec[i].propid)
            {
            case PID_DICTIONARY:
                /* Can't set the dictionary */
                hr = STG_E_INVALIDPARAMETER;
                break;
            case PID_CODEPAGE:
                /* Can only set the code page if nothing else has been set */
                if (dictionary_num_entries(This->propid_to_prop) == 0 &&
                 rgpropvar[i].vt == VT_I2)
                {
                    This->codePage = (USHORT)rgpropvar[i].iVal;
                    if (This->codePage == CP_UNICODE)
                        This->grfFlags &= ~PROPSETFLAG_ANSI;
                    else
                        This->grfFlags |= PROPSETFLAG_ANSI;
                }
                else
                    hr = STG_E_INVALIDPARAMETER;
                break;
            case PID_LOCALE:
                /* Can only set the locale if nothing else has been set */
                if (dictionary_num_entries(This->propid_to_prop) == 0 &&
                 rgpropvar[i].vt == VT_I4)
                    This->locale = rgpropvar[i].lVal;
                else
                    hr = STG_E_INVALIDPARAMETER;
                break;
            case PID_ILLEGAL:
                /* silently ignore like MSDN says */
                break;
            default:
                if (rgpspec[i].propid >= PID_MIN_READONLY)
                    hr = STG_E_INVALIDPARAMETER;
                else
                    hr = PropertyStorage_StorePropWithId(This,
                     rgpspec[i].propid, &rgpropvar[i], GetACP());
            }
        }
    }
    if (This->grfFlags & PROPSETFLAG_UNBUFFERED)
        IPropertyStorage_Commit(iface, STGC_DEFAULT);
    LeaveCriticalSection(&This->cs);
    return hr;
}

/************************************************************************
 * IPropertyStorage_fnDeleteMultiple (IPropertyStorage)
 */
static HRESULT WINAPI IPropertyStorage_fnDeleteMultiple(
    IPropertyStorage* iface,
    ULONG cpspec,
    const PROPSPEC rgpspec[])
{
    PropertyStorage_impl *This = impl_from_IPropertyStorage(iface);
    ULONG i;
    HRESULT hr;

    TRACE("%p, %ld, %p.\n", iface, cpspec, rgpspec);

    if (cpspec && !rgpspec)
        return E_INVALIDARG;
    if (!(This->grfMode & STGM_READWRITE))
        return STG_E_ACCESSDENIED;
    hr = S_OK;
    EnterCriticalSection(&This->cs);
    This->dirty = TRUE;
    for (i = 0; i < cpspec; i++)
    {
        if (rgpspec[i].ulKind == PRSPEC_LPWSTR)
        {
            void *propid;

            if (dictionary_find(This->name_to_propid, rgpspec[i].lpwstr, &propid))
                dictionary_remove(This->propid_to_prop, propid);
        }
        else
        {
            if (rgpspec[i].propid >= PID_FIRST_USABLE &&
             rgpspec[i].propid < PID_MIN_READONLY)
                dictionary_remove(This->propid_to_prop, UlongToPtr(rgpspec[i].propid));
            else
                hr = STG_E_INVALIDPARAMETER;
        }
    }
    if (This->grfFlags & PROPSETFLAG_UNBUFFERED)
        IPropertyStorage_Commit(iface, STGC_DEFAULT);
    LeaveCriticalSection(&This->cs);
    return hr;
}

/************************************************************************
 * IPropertyStorage_fnReadPropertyNames (IPropertyStorage)
 */
static HRESULT WINAPI IPropertyStorage_fnReadPropertyNames(
    IPropertyStorage* iface,
    ULONG cpropid,
    const PROPID rgpropid[],
    LPOLESTR rglpwstrName[])
{
    PropertyStorage_impl *This = impl_from_IPropertyStorage(iface);
    ULONG i;
    HRESULT hr = S_FALSE;

    TRACE("%p, %ld, %p, %p.\n", iface, cpropid, rgpropid, rglpwstrName);

    if (cpropid && (!rgpropid || !rglpwstrName))
        return E_INVALIDARG;
    EnterCriticalSection(&This->cs);
    for (i = 0; i < cpropid && SUCCEEDED(hr); i++)
    {
        LPWSTR name = PropertyStorage_FindPropertyNameById(This, rgpropid[i]);

        if (name)
        {
            size_t len = lstrlenW(name);

            hr = S_OK;
            rglpwstrName[i] = CoTaskMemAlloc((len + 1) * sizeof(WCHAR));
            if (rglpwstrName[i])
                memcpy(rglpwstrName[i], name, (len + 1) * sizeof(WCHAR));
            else
                hr = STG_E_INSUFFICIENTMEMORY;
        }
        else
            rglpwstrName[i] = NULL;
    }
    LeaveCriticalSection(&This->cs);
    return hr;
}

/************************************************************************
 * IPropertyStorage_fnWritePropertyNames (IPropertyStorage)
 */
static HRESULT WINAPI IPropertyStorage_fnWritePropertyNames(
    IPropertyStorage* iface,
    ULONG cpropid,
    const PROPID rgpropid[],
    const LPOLESTR rglpwstrName[])
{
    PropertyStorage_impl *This = impl_from_IPropertyStorage(iface);
    ULONG i;
    HRESULT hr;

    TRACE("%p, %lu, %p, %p.\n", iface, cpropid, rgpropid, rglpwstrName);

    if (cpropid && (!rgpropid || !rglpwstrName))
        return E_INVALIDARG;
    if (!(This->grfMode & STGM_READWRITE))
        return STG_E_ACCESSDENIED;
    hr = S_OK;
    EnterCriticalSection(&This->cs);
    This->dirty = TRUE;
    for (i = 0; SUCCEEDED(hr) && i < cpropid; i++)
    {
        if (rgpropid[i] != PID_ILLEGAL)
            hr = PropertyStorage_StoreNameWithId(This, (LPCSTR)rglpwstrName[i],
             CP_UNICODE, rgpropid[i]);
    }
    if (This->grfFlags & PROPSETFLAG_UNBUFFERED)
        IPropertyStorage_Commit(iface, STGC_DEFAULT);
    LeaveCriticalSection(&This->cs);
    return hr;
}

/************************************************************************
 * IPropertyStorage_fnDeletePropertyNames (IPropertyStorage)
 */
static HRESULT WINAPI IPropertyStorage_fnDeletePropertyNames(
    IPropertyStorage* iface,
    ULONG cpropid,
    const PROPID rgpropid[])
{
    PropertyStorage_impl *This = impl_from_IPropertyStorage(iface);
    ULONG i;
    HRESULT hr;

    TRACE("%p, %ld, %p.\n", iface, cpropid, rgpropid);

    if (cpropid && !rgpropid)
        return E_INVALIDARG;
    if (!(This->grfMode & STGM_READWRITE))
        return STG_E_ACCESSDENIED;
    hr = S_OK;
    EnterCriticalSection(&This->cs);
    This->dirty = TRUE;
    for (i = 0; i < cpropid; i++)
    {
        LPWSTR name = NULL;

        if (dictionary_find(This->propid_to_name, UlongToPtr(rgpropid[i]), (void **)&name))
        {
            dictionary_remove(This->propid_to_name, UlongToPtr(rgpropid[i]));
            dictionary_remove(This->name_to_propid, name);
        }
    }
    if (This->grfFlags & PROPSETFLAG_UNBUFFERED)
        IPropertyStorage_Commit(iface, STGC_DEFAULT);
    LeaveCriticalSection(&This->cs);
    return hr;
}

/************************************************************************
 * IPropertyStorage_fnCommit (IPropertyStorage)
 */
static HRESULT WINAPI IPropertyStorage_fnCommit(
    IPropertyStorage* iface,
    DWORD grfCommitFlags)
{
    PropertyStorage_impl *This = impl_from_IPropertyStorage(iface);
    HRESULT hr;

    TRACE("%p, %#lx.\n", iface, grfCommitFlags);

    if (!(This->grfMode & STGM_READWRITE))
        return STG_E_ACCESSDENIED;
    EnterCriticalSection(&This->cs);
    if (This->dirty)
        hr = PropertyStorage_WriteToStream(This);
    else
        hr = S_OK;
    LeaveCriticalSection(&This->cs);
    return hr;
}

/************************************************************************
 * IPropertyStorage_fnRevert (IPropertyStorage)
 */
static HRESULT WINAPI IPropertyStorage_fnRevert(
    IPropertyStorage* iface)
{
    HRESULT hr;
    PropertyStorage_impl *This = impl_from_IPropertyStorage(iface);

    TRACE("%p\n", iface);

    EnterCriticalSection(&This->cs);
    if (This->dirty)
    {
        PropertyStorage_DestroyDictionaries(This);
        hr = PropertyStorage_CreateDictionaries(This);
        if (SUCCEEDED(hr))
            hr = PropertyStorage_ReadFromStream(This);
    }
    else
        hr = S_OK;
    LeaveCriticalSection(&This->cs);
    return hr;
}

/************************************************************************
 * IPropertyStorage_fnEnum (IPropertyStorage)
 */
static HRESULT WINAPI IPropertyStorage_fnEnum(IPropertyStorage *iface, IEnumSTATPROPSTG **ppenum)
{
    PropertyStorage_impl *storage = impl_from_IPropertyStorage(iface);

    TRACE("%p, %p.\n", iface, ppenum);

    return create_enum_stat_prop_stg(storage, ppenum);
}

/************************************************************************
 * IPropertyStorage_fnSetTimes (IPropertyStorage)
 */
static HRESULT WINAPI IPropertyStorage_fnSetTimes(
    IPropertyStorage* iface,
    const FILETIME* pctime,
    const FILETIME* patime,
    const FILETIME* pmtime)
{
    FIXME("\n");
    return E_NOTIMPL;
}

/************************************************************************
 * IPropertyStorage_fnSetClass (IPropertyStorage)
 */
static HRESULT WINAPI IPropertyStorage_fnSetClass(
    IPropertyStorage* iface,
    REFCLSID clsid)
{
    PropertyStorage_impl *This = impl_from_IPropertyStorage(iface);

    TRACE("%p, %s\n", iface, debugstr_guid(clsid));

    if (!clsid)
        return E_INVALIDARG;
    if (!(This->grfMode & STGM_READWRITE))
        return STG_E_ACCESSDENIED;
    This->clsid = *clsid;
    This->dirty = TRUE;
    if (This->grfFlags & PROPSETFLAG_UNBUFFERED)
        IPropertyStorage_Commit(iface, STGC_DEFAULT);
    return S_OK;
}

/************************************************************************
 * IPropertyStorage_fnStat (IPropertyStorage)
 */
static HRESULT WINAPI IPropertyStorage_fnStat(
    IPropertyStorage* iface,
    STATPROPSETSTG* statpsstg)
{
    PropertyStorage_impl *This = impl_from_IPropertyStorage(iface);
    STATSTG stat;
    HRESULT hr;

    TRACE("%p, %p\n", iface, statpsstg);

    if (!statpsstg)
        return E_INVALIDARG;

    hr = IStream_Stat(This->stm, &stat, STATFLAG_NONAME);
    if (SUCCEEDED(hr))
    {
        statpsstg->fmtid = This->fmtid;
        statpsstg->clsid = This->clsid;
        statpsstg->grfFlags = This->grfFlags;
        statpsstg->mtime = stat.mtime;
        statpsstg->ctime = stat.ctime;
        statpsstg->atime = stat.atime;
        statpsstg->dwOSVersion = This->originatorOS;
    }
    return hr;
}

static int PropertyStorage_PropNameCompare(const void *a, const void *b,
 void *extra)
{
    PropertyStorage_impl *This = extra;

    if (This->codePage == CP_UNICODE)
    {
        TRACE("(%s, %s)\n", debugstr_w(a), debugstr_w(b));
        if (This->grfFlags & PROPSETFLAG_CASE_SENSITIVE)
            return wcscmp(a, b);
        else
            return lstrcmpiW(a, b);
    }
    else
    {
        TRACE("(%s, %s)\n", debugstr_a(a), debugstr_a(b));
        if (This->grfFlags & PROPSETFLAG_CASE_SENSITIVE)
            return lstrcmpA(a, b);
        else
            return lstrcmpiA(a, b);
    }
}

static void PropertyStorage_PropNameDestroy(void *k, void *d, void *extra)
{
    CoTaskMemFree(k);
}

static int PropertyStorage_PropCompare(const void *a, const void *b,
 void *extra)
{
    TRACE("%lu, %lu.\n", PtrToUlong(a), PtrToUlong(b));
    return PtrToUlong(a) - PtrToUlong(b);
}

static void PropertyStorage_PropertyDestroy(void *k, void *d, void *extra)
{
    PropVariantClear(d);
    HeapFree(GetProcessHeap(), 0, d);
}

#ifdef WORDS_BIGENDIAN
/* Swaps each character in str to or from little endian; assumes the conversion
 * is symmetric, that is, that lendian16toh is equivalent to htole16.
 */
static void PropertyStorage_ByteSwapString(LPWSTR str, size_t len)
{
    DWORD i;

    /* Swap characters to host order.
     * FIXME: alignment?
     */
    for (i = 0; i < len; i++)
        str[i] = lendian16toh(str[i]);
}
#else
#define PropertyStorage_ByteSwapString(s, l)
#endif


#ifdef __i386__
#define __thiscall_wrapper __stdcall
#else
#define __thiscall_wrapper __cdecl
#endif

static void* __thiscall_wrapper Allocate_CoTaskMemAlloc(void *this, ULONG size)
{
    return CoTaskMemAlloc(size);
}

struct read_buffer
{
    BYTE *data;
    size_t size;
};

static HRESULT buffer_test_offset(const struct read_buffer *buffer, size_t offset, size_t len)
{
    return len > buffer->size || offset > buffer->size - len ? STG_E_READFAULT : S_OK;
}

static HRESULT buffer_read_uint64(const struct read_buffer *buffer, size_t offset, ULARGE_INTEGER *data)
{
    HRESULT hr;

    if (SUCCEEDED(hr = buffer_test_offset(buffer, offset, sizeof(*data))))
        StorageUtl_ReadULargeInteger(buffer->data, offset, data);

    return hr;
}

static HRESULT buffer_read_dword(const struct read_buffer *buffer, size_t offset, DWORD *data)
{
    HRESULT hr;

    if (SUCCEEDED(hr = buffer_test_offset(buffer, offset, sizeof(*data))))
        StorageUtl_ReadDWord(buffer->data, offset, data);

    return hr;
}

static HRESULT buffer_read_word(const struct read_buffer *buffer, size_t offset, WORD *data)
{
    HRESULT hr;

    if (SUCCEEDED(hr = buffer_test_offset(buffer, offset, sizeof(*data))))
        StorageUtl_ReadWord(buffer->data, offset, data);

    return hr;
}

static HRESULT buffer_read_byte(const struct read_buffer *buffer, size_t offset, BYTE *data)
{
    HRESULT hr;

    if (SUCCEEDED(hr = buffer_test_offset(buffer, offset, sizeof(*data))))
        *data = *(buffer->data + offset);

    return hr;
}

static HRESULT buffer_read_len(const struct read_buffer *buffer, size_t offset, void *dest, size_t len)
{
    HRESULT hr;

    if (SUCCEEDED(hr = buffer_test_offset(buffer, offset, len)))
        memcpy(dest, buffer->data + offset, len);

    return hr;
}

static HRESULT propertystorage_read_scalar(PROPVARIANT *prop, const struct read_buffer *buffer, size_t offset,
        UINT codepage, void* (WINAPI *allocate)(void *this, ULONG size), void *allocate_data)
{
    HRESULT hr;

    assert(!(prop->vt & (VT_ARRAY | VT_VECTOR)));

    switch (prop->vt)
    {
    case VT_EMPTY:
    case VT_NULL:
        hr = S_OK;
        break;
    case VT_I1:
        hr = buffer_read_byte(buffer, offset, (BYTE *)&prop->cVal);
        TRACE("Read char 0x%x ('%c')\n", prop->cVal, prop->cVal);
        break;
    case VT_UI1:
        hr = buffer_read_byte(buffer, offset, &prop->bVal);
        TRACE("Read byte 0x%x\n", prop->bVal);
        break;
    case VT_BOOL:
        hr = buffer_read_word(buffer, offset, (WORD *)&prop->boolVal);
        TRACE("Read bool %d\n", prop->boolVal);
        break;
    case VT_I2:
        hr = buffer_read_word(buffer, offset, (WORD *)&prop->iVal);
        TRACE("Read short %d\n", prop->iVal);
        break;
    case VT_UI2:
        hr = buffer_read_word(buffer, offset, &prop->uiVal);
        TRACE("Read ushort %d\n", prop->uiVal);
        break;
    case VT_INT:
    case VT_I4:
        hr = buffer_read_dword(buffer, offset, (DWORD *)&prop->lVal);
        TRACE("Read long %ld\n", prop->lVal);
        break;
    case VT_UINT:
    case VT_UI4:
        hr = buffer_read_dword(buffer, offset, &prop->ulVal);
        TRACE("Read ulong %ld\n", prop->ulVal);
        break;
    case VT_I8:
        hr = buffer_read_uint64(buffer, offset, (ULARGE_INTEGER *)&prop->hVal);
        TRACE("Read long long %s\n", wine_dbgstr_longlong(prop->hVal.QuadPart));
        break;
    case VT_UI8:
        hr = buffer_read_uint64(buffer, offset, &prop->uhVal);
        TRACE("Read ulong long %s\n", wine_dbgstr_longlong(prop->uhVal.QuadPart));
        break;
    case VT_R8:
        hr = buffer_read_len(buffer, offset, &prop->dblVal, sizeof(prop->dblVal));
        TRACE("Read double %f\n", prop->dblVal);
        break;
    case VT_LPSTR:
    {
        DWORD count;

        if (FAILED(hr = buffer_read_dword(buffer, offset, &count)))
            break;

        offset += sizeof(DWORD);

        if (codepage == CP_UNICODE && count % sizeof(WCHAR))
        {
            WARN("Unicode string has odd number of bytes\n");
            hr = STG_E_INVALIDHEADER;
        }
        else
        {
            prop->pszVal = allocate(allocate_data, count);
            if (prop->pszVal)
            {
                if (FAILED(hr = buffer_read_len(buffer, offset, prop->pszVal, count)))
                    break;

                /* This is stored in the code page specified in codepage.
                 * Don't convert it, the caller will just store it as-is.
                 */
                if (codepage == CP_UNICODE)
                {
                    /* Make sure it's NULL-terminated */
                    prop->pszVal[count / sizeof(WCHAR) - 1] = '\0';
                    TRACE("Read string value %s\n",
                     debugstr_w(prop->pwszVal));
                }
                else
                {
                    /* Make sure it's NULL-terminated */
                    prop->pszVal[count - 1] = '\0';
                    TRACE("Read string value %s\n", debugstr_a(prop->pszVal));
                }
            }
            else
                hr = STG_E_INSUFFICIENTMEMORY;
        }
        break;
    }
    case VT_BSTR:
    {
        DWORD count, wcount;

        if (FAILED(hr = buffer_read_dword(buffer, offset, &count)))
            break;

        offset += sizeof(DWORD);

        if (codepage == CP_UNICODE && count % sizeof(WCHAR))
        {
            WARN("Unicode string has odd number of bytes\n");
            hr = STG_E_INVALIDHEADER;
        }
        else
        {
            if (codepage == CP_UNICODE)
                wcount = count / sizeof(WCHAR);
            else
            {
                if (FAILED(hr = buffer_test_offset(buffer, offset, count)))
                    break;
                wcount = MultiByteToWideChar(codepage, 0, (LPCSTR)(buffer->data + offset), count, NULL, 0);
            }

            prop->bstrVal = SysAllocStringLen(NULL, wcount); /* FIXME: use allocator? */

            if (prop->bstrVal)
            {
                if (codepage == CP_UNICODE)
                    hr = buffer_read_len(buffer, offset, prop->bstrVal, count);
                else
                    MultiByteToWideChar(codepage, 0, (LPCSTR)(buffer->data + offset), count, prop->bstrVal, wcount);

                prop->bstrVal[wcount - 1] = '\0';
                TRACE("Read string value %s\n", debugstr_w(prop->bstrVal));
            }
            else
                hr = STG_E_INSUFFICIENTMEMORY;
        }
        break;
    }
    case VT_BLOB:
    {
        DWORD count;

        if (FAILED(hr = buffer_read_dword(buffer, offset, &count)))
            break;

        offset += sizeof(DWORD);

        prop->blob.cbSize = count;
        prop->blob.pBlobData = allocate(allocate_data, count);
        if (prop->blob.pBlobData)
        {
            hr = buffer_read_len(buffer, offset, prop->blob.pBlobData, count);
            TRACE("Read blob value of size %ld\n", count);
        }
        else
            hr = STG_E_INSUFFICIENTMEMORY;
        break;
    }
    case VT_LPWSTR:
    {
        DWORD count;

        if (FAILED(hr = buffer_read_dword(buffer, offset, &count)))
            break;

        offset += sizeof(DWORD);

        prop->pwszVal = allocate(allocate_data, count * sizeof(WCHAR));
        if (prop->pwszVal)
        {
            if (SUCCEEDED(hr = buffer_read_len(buffer, offset, prop->pwszVal, count * sizeof(WCHAR))))
            {
                /* make sure string is NULL-terminated */
                prop->pwszVal[count - 1] = '\0';
                PropertyStorage_ByteSwapString(prop->pwszVal, count);
                TRACE("Read string value %s\n", debugstr_w(prop->pwszVal));
            }
        }
        else
            hr = STG_E_INSUFFICIENTMEMORY;
        break;
    }
    case VT_FILETIME:
        hr = buffer_read_uint64(buffer, offset, (ULARGE_INTEGER *)&prop->filetime);
        break;
    case VT_CF:
        {
            DWORD len = 0, tag = 0;

            if (SUCCEEDED(hr = buffer_read_dword(buffer, offset, &len)))
                hr = buffer_read_dword(buffer, offset + sizeof(DWORD), &tag);
            if (FAILED(hr))
                break;

            offset += 2 * sizeof(DWORD);

            if (len > 8)
            {
                len -= 8;
                prop->pclipdata = allocate(allocate_data, sizeof (CLIPDATA));
                prop->pclipdata->cbSize = len;
                prop->pclipdata->ulClipFmt = tag;
                prop->pclipdata->pClipData = allocate(allocate_data, len - sizeof(prop->pclipdata->ulClipFmt));
                hr = buffer_read_len(buffer, offset, prop->pclipdata->pClipData, len - sizeof(prop->pclipdata->ulClipFmt));
            }
            else
                hr = STG_E_INVALIDPARAMETER;
        }
        break;
    case VT_CLSID:
        if (!(prop->puuid = allocate(allocate_data, sizeof (*prop->puuid))))
            return STG_E_INSUFFICIENTMEMORY;

        if (SUCCEEDED(hr = buffer_test_offset(buffer, offset, sizeof(*prop->puuid))))
            StorageUtl_ReadGUID(buffer->data, offset, prop->puuid);

        break;
    default:
        FIXME("unsupported type %d\n", prop->vt);
        hr = STG_E_INVALIDPARAMETER;
    }

    return hr;
}

static size_t propertystorage_get_elemsize(const PROPVARIANT *prop)
{
    if (!(prop->vt & VT_VECTOR))
        return 0;

    switch (prop->vt & ~VT_VECTOR)
    {
        case VT_I1: return sizeof(*prop->cac.pElems);
        case VT_UI1: return sizeof(*prop->caub.pElems);
        case VT_I2: return sizeof(*prop->cai.pElems);
        case VT_UI2: return sizeof(*prop->caui.pElems);
        case VT_BOOL: return sizeof(*prop->cabool.pElems);
        case VT_I4: return sizeof(*prop->cal.pElems);
        case VT_UI4: return sizeof(*prop->caul.pElems);
        case VT_R4: return sizeof(*prop->caflt.pElems);
        case VT_ERROR: return sizeof(*prop->cascode.pElems);
        case VT_I8: return sizeof(*prop->cah.pElems);
        case VT_UI8: return sizeof(*prop->cauh.pElems);
        case VT_R8: return sizeof(*prop->cadbl.pElems);
        case VT_CY: return sizeof(*prop->cacy.pElems);
        case VT_DATE: return sizeof(*prop->cadate.pElems);
        case VT_FILETIME: return sizeof(*prop->cafiletime.pElems);
        case VT_CLSID: return sizeof(*prop->cauuid.pElems);
        case VT_VARIANT: return sizeof(*prop->capropvar.pElems);
        default:
            FIXME("Unhandled type %#x.\n", prop->vt);
            return 0;
    }
}

static HRESULT PropertyStorage_ReadProperty(PROPVARIANT *prop, const struct read_buffer *buffer,
        size_t offset, UINT codepage, void* (__thiscall_wrapper WINAPI *allocate)(void *this, ULONG size), void *allocate_data)
{
    HRESULT hr;
    DWORD vt;

    assert(prop);
    assert(buffer->data);

    if (FAILED(hr = buffer_read_dword(buffer, offset, &vt)))
        return hr;

    offset += sizeof(DWORD);
    prop->vt = vt;

    if (prop->vt & VT_VECTOR)
    {
        DWORD count, i;

        switch (prop->vt & ~VT_VECTOR)
        {
            case VT_BSTR:
            case VT_VARIANT:
            case VT_LPSTR:
            case VT_LPWSTR:
            case VT_CF:
                FIXME("Vector with variable length elements are not supported.\n");
                return STG_E_INVALIDPARAMETER;
            default:
                ;
        }

        if (SUCCEEDED(hr = buffer_read_dword(buffer, offset, &count)))
        {
            size_t elemsize = propertystorage_get_elemsize(prop);
            PROPVARIANT elem;

            offset += sizeof(DWORD);

            if ((prop->capropvar.pElems = allocate(allocate_data, elemsize * count)))
            {
                prop->capropvar.cElems = count;
                elem.vt = prop->vt & ~VT_VECTOR;

                for (i = 0; i < count; ++i)
                {
                    if (SUCCEEDED(hr = propertystorage_read_scalar(&elem, buffer, offset + i * elemsize, codepage,
                            allocate, allocate_data)))
                    {
                        memcpy(&prop->capropvar.pElems[i], &elem.lVal, elemsize);
                    }
                }
            }
            else
                hr = STG_E_INSUFFICIENTMEMORY;
        }
    }
    else if (prop->vt & VT_ARRAY)
    {
        FIXME("VT_ARRAY properties are not supported.\n");
        hr = STG_E_INVALIDPARAMETER;
    }
    else
        hr = propertystorage_read_scalar(prop, buffer, offset, codepage, allocate, allocate_data);

    return hr;
}

static HRESULT PropertyStorage_ReadHeaderFromStream(IStream *stm,
 PROPERTYSETHEADER *hdr)
{
    BYTE buf[sizeof(PROPERTYSETHEADER)];
    ULONG count = 0;
    HRESULT hr;

    assert(stm);
    assert(hdr);
    hr = IStream_Read(stm, buf, sizeof(buf), &count);
    if (SUCCEEDED(hr))
    {
        if (count != sizeof(buf))
        {
            WARN("read only %ld\n", count);
            hr = STG_E_INVALIDHEADER;
        }
        else
        {
            StorageUtl_ReadWord(buf, offsetof(PROPERTYSETHEADER, wByteOrder),
             &hdr->wByteOrder);
            StorageUtl_ReadWord(buf, offsetof(PROPERTYSETHEADER, wFormat),
             &hdr->wFormat);
            StorageUtl_ReadDWord(buf, offsetof(PROPERTYSETHEADER, dwOSVer),
             &hdr->dwOSVer);
            StorageUtl_ReadGUID(buf, offsetof(PROPERTYSETHEADER, clsid),
             &hdr->clsid);
            StorageUtl_ReadDWord(buf, offsetof(PROPERTYSETHEADER, reserved),
             &hdr->reserved);
        }
    }
    TRACE("returning %#lx\n", hr);
    return hr;
}

static HRESULT PropertyStorage_ReadFmtIdOffsetFromStream(IStream *stm,
 FORMATIDOFFSET *fmt)
{
    BYTE buf[sizeof(FORMATIDOFFSET)];
    ULONG count = 0;
    HRESULT hr;

    assert(stm);
    assert(fmt);
    hr = IStream_Read(stm, buf, sizeof(buf), &count);
    if (SUCCEEDED(hr))
    {
        if (count != sizeof(buf))
        {
            WARN("read only %ld\n", count);
            hr = STG_E_INVALIDHEADER;
        }
        else
        {
            StorageUtl_ReadGUID(buf, offsetof(FORMATIDOFFSET, fmtid),
             &fmt->fmtid);
            StorageUtl_ReadDWord(buf, offsetof(FORMATIDOFFSET, dwOffset),
             &fmt->dwOffset);
        }
    }
    TRACE("returning %#lx\n", hr);
    return hr;
}

static HRESULT PropertyStorage_ReadSectionHeaderFromStream(IStream *stm,
 PROPERTYSECTIONHEADER *hdr)
{
    BYTE buf[sizeof(PROPERTYSECTIONHEADER)];
    ULONG count = 0;
    HRESULT hr;

    assert(stm);
    assert(hdr);
    hr = IStream_Read(stm, buf, sizeof(buf), &count);
    if (SUCCEEDED(hr))
    {
        if (count != sizeof(buf))
        {
            WARN("read only %ld\n", count);
            hr = STG_E_INVALIDHEADER;
        }
        else
        {
            StorageUtl_ReadDWord(buf, offsetof(PROPERTYSECTIONHEADER,
             cbSection), &hdr->cbSection);
            StorageUtl_ReadDWord(buf, offsetof(PROPERTYSECTIONHEADER,
             cProperties), &hdr->cProperties);
        }
    }
    TRACE("returning %#lx\n", hr);
    return hr;
}

/* Reads the dictionary from the memory buffer beginning at ptr.  Interprets
 * the entries according to the values of This->codePage and This->locale.
 */
static HRESULT PropertyStorage_ReadDictionary(PropertyStorage_impl *This, const struct read_buffer *buffer,
        size_t offset)
{
    DWORD numEntries, i;
    HRESULT hr;

    assert(This->name_to_propid);
    assert(This->propid_to_name);

    if (FAILED(hr = buffer_read_dword(buffer, offset, &numEntries)))
        return hr;

    TRACE("Reading %ld entries:\n", numEntries);

    offset += sizeof(DWORD);

    for (i = 0; SUCCEEDED(hr) && i < numEntries; i++)
    {
        PROPID propid;
        DWORD cbEntry;
        WCHAR ch = 0;

        if (SUCCEEDED(hr = buffer_read_dword(buffer, offset, &propid)))
            hr = buffer_read_dword(buffer, offset + sizeof(PROPID), &cbEntry);
        if (FAILED(hr))
            break;

        offset += sizeof(PROPID) + sizeof(DWORD);

        if (FAILED(hr = buffer_test_offset(buffer, offset, This->codePage == CP_UNICODE ?
                ALIGNED_LENGTH(cbEntry * sizeof(WCHAR), 3) : cbEntry)))
        {
            WARN("Broken name length for entry %ld.\n", i);
            return hr;
        }

        /* Make sure the source string is NULL-terminated */
        if (This->codePage != CP_UNICODE)
            buffer_read_byte(buffer, offset + cbEntry - 1, (BYTE *)&ch);
        else
            buffer_read_word(buffer, offset + (cbEntry - 1) * sizeof(WCHAR), &ch);

        if (ch)
        {
            WARN("Dictionary entry name %ld is not null-terminated.\n", i);
            return E_FAIL;
        }

        TRACE("Reading entry with ID %#lx, %ld chars, name %s.\n", propid, cbEntry, This->codePage == CP_UNICODE ?
                debugstr_wn((WCHAR *)buffer->data, cbEntry) : debugstr_an((char *)buffer->data, cbEntry));

        hr = PropertyStorage_StoreNameWithId(This, (char *)buffer->data + offset, This->codePage, propid);
        /* Unicode entries are padded to DWORD boundaries */
        if (This->codePage == CP_UNICODE)
            cbEntry = ALIGNED_LENGTH(cbEntry * sizeof(WCHAR), 3);

        offset += cbEntry;
    }

    return hr;
}

static HRESULT PropertyStorage_ReadFromStream(PropertyStorage_impl *This)
{
    struct read_buffer read_buffer;
    PROPERTYSETHEADER hdr;
    FORMATIDOFFSET fmtOffset;
    PROPERTYSECTIONHEADER sectionHdr;
    LARGE_INTEGER seek;
    ULONG i;
    STATSTG stat;
    HRESULT hr;
    BYTE *buf = NULL;
    ULONG count = 0;
    DWORD dictOffset = 0;

    This->dirty = FALSE;
    This->highestProp = 0;
    hr = IStream_Stat(This->stm, &stat, STATFLAG_NONAME);
    if (FAILED(hr))
        goto end;
    if (stat.cbSize.HighPart)
    {
        WARN("stream too big\n");
        /* maximum size varies, but it can't be this big */
        hr = STG_E_INVALIDHEADER;
        goto end;
    }
    if (stat.cbSize.LowPart == 0)
    {
        /* empty stream is okay */
        hr = S_OK;
        goto end;
    }
    else if (stat.cbSize.LowPart < sizeof(PROPERTYSETHEADER) +
     sizeof(FORMATIDOFFSET))
    {
        WARN("stream too small\n");
        hr = STG_E_INVALIDHEADER;
        goto end;
    }
    seek.QuadPart = 0;
    hr = IStream_Seek(This->stm, seek, STREAM_SEEK_SET, NULL);
    if (FAILED(hr))
        goto end;
    hr = PropertyStorage_ReadHeaderFromStream(This->stm, &hdr);
    /* I've only seen reserved == 1, but the article says I shouldn't disallow
     * higher values.
     */
    if (hdr.wByteOrder != PROPSETHDR_BYTEORDER_MAGIC || hdr.reserved < 1)
    {
        WARN("bad magic in prop set header\n");
        hr = STG_E_INVALIDHEADER;
        goto end;
    }
    if (hdr.wFormat != 0 && hdr.wFormat != 1)
    {
        WARN("bad format version %d\n", hdr.wFormat);
        hr = STG_E_INVALIDHEADER;
        goto end;
    }
    This->format = hdr.wFormat;
    This->clsid = hdr.clsid;
    This->originatorOS = hdr.dwOSVer;
    if (PROPSETHDR_OSVER_KIND(hdr.dwOSVer) == PROPSETHDR_OSVER_KIND_MAC)
        WARN("File comes from a Mac, strings will probably be screwed up\n");
    hr = PropertyStorage_ReadFmtIdOffsetFromStream(This->stm, &fmtOffset);
    if (FAILED(hr))
        goto end;
    if (fmtOffset.dwOffset > stat.cbSize.LowPart)
    {
        WARN("invalid offset %ld (stream length is %ld)\n", fmtOffset.dwOffset, stat.cbSize.LowPart);
        hr = STG_E_INVALIDHEADER;
        goto end;
    }
    seek.QuadPart = fmtOffset.dwOffset;
    hr = IStream_Seek(This->stm, seek, STREAM_SEEK_SET, NULL);
    if (FAILED(hr))
        goto end;
    /* wackiness alert: if the format ID is FMTID_DocSummaryInformation, there
     * follows not one, but two sections.  The first contains the standard properties
     * for the document summary information, and the second consists of user-defined
     * properties.  This is the only case in which multiple sections are
     * allowed.
     * Reading the second stream isn't implemented yet.
     */
    hr = PropertyStorage_ReadSectionHeaderFromStream(This->stm, &sectionHdr);
    if (FAILED(hr))
        goto end;
    /* The section size includes the section header, so check it */
    if (sectionHdr.cbSection < sizeof(PROPERTYSECTIONHEADER))
    {
        WARN("section header too small, got %ld\n", sectionHdr.cbSection);
        hr = STG_E_INVALIDHEADER;
        goto end;
    }
    buf = HeapAlloc(GetProcessHeap(), 0, sectionHdr.cbSection -
     sizeof(PROPERTYSECTIONHEADER));
    if (!buf)
    {
        hr = STG_E_INSUFFICIENTMEMORY;
        goto end;
    }
    read_buffer.data = buf;
    read_buffer.size = sectionHdr.cbSection - sizeof(sectionHdr);

    hr = IStream_Read(This->stm, read_buffer.data, read_buffer.size, &count);
    if (FAILED(hr))
        goto end;
    TRACE("Reading %ld properties:\n", sectionHdr.cProperties);
    for (i = 0; SUCCEEDED(hr) && i < sectionHdr.cProperties; i++)
    {
        PROPERTYIDOFFSET *idOffset = (PROPERTYIDOFFSET *)(read_buffer.data +
         i * sizeof(PROPERTYIDOFFSET));

        if (idOffset->dwOffset < sizeof(PROPERTYSECTIONHEADER) ||
         idOffset->dwOffset > sectionHdr.cbSection - sizeof(DWORD))
            hr = STG_E_INVALIDPOINTER;
        else
        {
            if (idOffset->propid >= PID_FIRST_USABLE &&
             idOffset->propid < PID_MIN_READONLY && idOffset->propid >
             This->highestProp)
                This->highestProp = idOffset->propid;
            if (idOffset->propid == PID_DICTIONARY)
            {
                /* Don't read the dictionary yet, its entries depend on the
                 * code page.  Just store the offset so we know to read it
                 * later.
                 */
                dictOffset = idOffset->dwOffset;
                TRACE("Dictionary offset is %ld\n", dictOffset);
            }
            else
            {
                PROPVARIANT prop;

                PropVariantInit(&prop);
                if (SUCCEEDED(PropertyStorage_ReadProperty(&prop, &read_buffer,
                        idOffset->dwOffset - sizeof(PROPERTYSECTIONHEADER), This->codePage,
                        Allocate_CoTaskMemAlloc, NULL)))
                {
                    TRACE("Read property with ID %#lx, type %d\n", idOffset->propid, prop.vt);
                    switch(idOffset->propid)
                    {
                    case PID_CODEPAGE:
                        if (prop.vt == VT_I2)
                            This->codePage = (USHORT)prop.iVal;
                        break;
                    case PID_LOCALE:
                        if (prop.vt == VT_I4)
                            This->locale = (LCID)prop.lVal;
                        break;
                    case PID_BEHAVIOR:
                        if (prop.vt == VT_I4 && prop.lVal)
                            This->grfFlags |= PROPSETFLAG_CASE_SENSITIVE;
                        /* The format should already be 1, but just in case */
                        This->format = 1;
                        break;
                    default:
                        hr = PropertyStorage_StorePropWithId(This,
                         idOffset->propid, &prop, This->codePage);
                    }
                }
                PropVariantClear(&prop);
            }
        }
    }
    if (!This->codePage)
    {
        /* default to Unicode unless told not to, as specified on msdn */
        if (This->grfFlags & PROPSETFLAG_ANSI)
            This->codePage = GetACP();
        else
            This->codePage = CP_UNICODE;
    }
    if (!This->locale)
        This->locale = LOCALE_SYSTEM_DEFAULT;
    TRACE("Code page is %d, locale is %ld\n", This->codePage, This->locale);
    if (dictOffset)
        hr = PropertyStorage_ReadDictionary(This, &read_buffer, dictOffset - sizeof(PROPERTYSECTIONHEADER));

end:
    HeapFree(GetProcessHeap(), 0, buf);
    if (FAILED(hr))
    {
        dictionary_destroy(This->name_to_propid);
        This->name_to_propid = NULL;
        dictionary_destroy(This->propid_to_name);
        This->propid_to_name = NULL;
        dictionary_destroy(This->propid_to_prop);
        This->propid_to_prop = NULL;
    }
    return hr;
}

static void PropertyStorage_MakeHeader(PropertyStorage_impl *This,
 PROPERTYSETHEADER *hdr)
{
    assert(hdr);
    StorageUtl_WriteWord(&hdr->wByteOrder, 0, PROPSETHDR_BYTEORDER_MAGIC);
    StorageUtl_WriteWord(&hdr->wFormat, 0, This->format);
    StorageUtl_WriteDWord(&hdr->dwOSVer, 0, This->originatorOS);
    StorageUtl_WriteGUID(&hdr->clsid, 0, &This->clsid);
    StorageUtl_WriteDWord(&hdr->reserved, 0, 1);
}

static void PropertyStorage_MakeFmtIdOffset(PropertyStorage_impl *This,
 FORMATIDOFFSET *fmtOffset)
{
    assert(fmtOffset);
    StorageUtl_WriteGUID(fmtOffset, 0, &This->fmtid);
    StorageUtl_WriteDWord(fmtOffset, offsetof(FORMATIDOFFSET, dwOffset),
     sizeof(PROPERTYSETHEADER) + sizeof(FORMATIDOFFSET));
}

static void PropertyStorage_MakeSectionHdr(DWORD cbSection, DWORD numProps,
 PROPERTYSECTIONHEADER *hdr)
{
    assert(hdr);
    StorageUtl_WriteDWord(hdr, 0, cbSection);
    StorageUtl_WriteDWord(hdr, offsetof(PROPERTYSECTIONHEADER, cProperties), numProps);
}

static void PropertyStorage_MakePropertyIdOffset(DWORD propid, DWORD dwOffset,
 PROPERTYIDOFFSET *propIdOffset)
{
    assert(propIdOffset);
    StorageUtl_WriteDWord(propIdOffset, 0, propid);
    StorageUtl_WriteDWord(propIdOffset, offsetof(PROPERTYIDOFFSET, dwOffset), dwOffset);
}

static inline HRESULT PropertyStorage_WriteWStringToStream(IStream *stm,
 LPCWSTR str, DWORD len, DWORD *written)
{
#ifdef WORDS_BIGENDIAN
    WCHAR *leStr = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
    HRESULT hr;

    if (!leStr)
        return E_OUTOFMEMORY;
    memcpy(leStr, str, len * sizeof(WCHAR));
    PropertyStorage_ByteSwapString(leStr, len);
    hr = IStream_Write(stm, leStr, len, written);
    HeapFree(GetProcessHeap(), 0, leStr);
    return hr;
#else
    return IStream_Write(stm, str, len * sizeof(WCHAR), written);
#endif
}

struct DictionaryClosure
{
    HRESULT hr;
    DWORD bytesWritten;
};

static BOOL PropertyStorage_DictionaryWriter(const void *key,
 const void *value, void *extra, void *closure)
{
    PropertyStorage_impl *This = extra;
    struct DictionaryClosure *c = closure;
    DWORD propid, keyLen;
    ULONG count;

    assert(key);
    assert(closure);
    StorageUtl_WriteDWord(&propid, 0, PtrToUlong(value));
    c->hr = IStream_Write(This->stm, &propid, sizeof(propid), &count);
    if (FAILED(c->hr))
        goto end;
    c->bytesWritten += sizeof(DWORD);
    if (This->codePage == CP_UNICODE)
    {
        DWORD pad = 0, pad_len;

        StorageUtl_WriteDWord(&keyLen, 0, lstrlenW((LPCWSTR)key) + 1);
        c->hr = IStream_Write(This->stm, &keyLen, sizeof(keyLen), &count);
        if (FAILED(c->hr))
            goto end;
        c->bytesWritten += sizeof(DWORD);
        c->hr = PropertyStorage_WriteWStringToStream(This->stm, key, keyLen,
         &count);
        if (FAILED(c->hr))
            goto end;
        keyLen *= sizeof(WCHAR);
        c->bytesWritten += keyLen;

        /* Align to 4 bytes. */
        pad_len = sizeof(DWORD) - keyLen % sizeof(DWORD);
        if (pad_len)
        {
            c->hr = IStream_Write(This->stm, &pad, pad_len, &count);
            if (FAILED(c->hr))
                goto end;
            c->bytesWritten += pad_len;
        }
    }
    else
    {
        StorageUtl_WriteDWord(&keyLen, 0, strlen((LPCSTR)key) + 1);
        c->hr = IStream_Write(This->stm, &keyLen, sizeof(keyLen), &count);
        if (FAILED(c->hr))
            goto end;
        c->bytesWritten += sizeof(DWORD);
        c->hr = IStream_Write(This->stm, key, keyLen, &count);
        if (FAILED(c->hr))
            goto end;
        c->bytesWritten += keyLen;
    }
end:
    return SUCCEEDED(c->hr);
}

#define SECTIONHEADER_OFFSET sizeof(PROPERTYSETHEADER) + sizeof(FORMATIDOFFSET)

/* Writes the dictionary to the stream.  Assumes without checking that the
 * dictionary isn't empty.
 */
static HRESULT PropertyStorage_WriteDictionaryToStream(
 PropertyStorage_impl *This, DWORD *sectionOffset)
{
    HRESULT hr;
    LARGE_INTEGER seek;
    PROPERTYIDOFFSET propIdOffset;
    ULONG count;
    DWORD dwTemp;
    struct DictionaryClosure closure;

    assert(sectionOffset);

    /* The dictionary's always the first property written, so seek to its
     * spot.
     */
    seek.QuadPart = SECTIONHEADER_OFFSET + sizeof(PROPERTYSECTIONHEADER);
    hr = IStream_Seek(This->stm, seek, STREAM_SEEK_SET, NULL);
    if (FAILED(hr))
        goto end;
    PropertyStorage_MakePropertyIdOffset(PID_DICTIONARY, *sectionOffset,
     &propIdOffset);
    hr = IStream_Write(This->stm, &propIdOffset, sizeof(propIdOffset), &count);
    if (FAILED(hr))
        goto end;

    seek.QuadPart = SECTIONHEADER_OFFSET + *sectionOffset;
    hr = IStream_Seek(This->stm, seek, STREAM_SEEK_SET, NULL);
    if (FAILED(hr))
        goto end;
    StorageUtl_WriteDWord(&dwTemp, 0, dictionary_num_entries(This->name_to_propid));
    hr = IStream_Write(This->stm, &dwTemp, sizeof(dwTemp), &count);
    if (FAILED(hr))
        goto end;
    *sectionOffset += sizeof(dwTemp);

    closure.hr = S_OK;
    closure.bytesWritten = 0;
    dictionary_enumerate(This->name_to_propid, PropertyStorage_DictionaryWriter,
     &closure);
    hr = closure.hr;
    if (FAILED(hr))
        goto end;
    *sectionOffset += closure.bytesWritten;
    if (closure.bytesWritten % sizeof(DWORD))
    {
        DWORD padding = sizeof(DWORD) - closure.bytesWritten % sizeof(DWORD);
        TRACE("adding %ld bytes of padding\n", padding);
        *sectionOffset += padding;
    }

end:
    return hr;
}

static HRESULT PropertyStorage_WritePropertyToStream(PropertyStorage_impl *This,
 DWORD propNum, DWORD propid, const PROPVARIANT *var, DWORD *sectionOffset)
{
    DWORD len, dwType, dwTemp, bytesWritten;
    HRESULT hr;
    LARGE_INTEGER seek;
    PROPERTYIDOFFSET propIdOffset;
    ULARGE_INTEGER ularge;
    ULONG count;

    assert(var);
    assert(sectionOffset);

    TRACE("%p, %ld, %#lx, %d, %ld.\n", This, propNum, propid, var->vt,
     *sectionOffset);

    seek.QuadPart = SECTIONHEADER_OFFSET + sizeof(PROPERTYSECTIONHEADER) +
     propNum * sizeof(PROPERTYIDOFFSET);
    hr = IStream_Seek(This->stm, seek, STREAM_SEEK_SET, NULL);
    if (FAILED(hr))
        goto end;
    PropertyStorage_MakePropertyIdOffset(propid, *sectionOffset, &propIdOffset);
    hr = IStream_Write(This->stm, &propIdOffset, sizeof(propIdOffset), &count);
    if (FAILED(hr))
        goto end;

    seek.QuadPart = SECTIONHEADER_OFFSET + *sectionOffset;
    hr = IStream_Seek(This->stm, seek, STREAM_SEEK_SET, NULL);
    if (FAILED(hr))
        goto end;
    StorageUtl_WriteDWord(&dwType, 0, var->vt);
    hr = IStream_Write(This->stm, &dwType, sizeof(dwType), &count);
    if (FAILED(hr))
        goto end;
    *sectionOffset += sizeof(dwType);

    switch (var->vt)
    {
    case VT_EMPTY:
    case VT_NULL:
        bytesWritten = 0;
        break;
    case VT_I1:
    case VT_UI1:
        hr = IStream_Write(This->stm, &var->cVal, sizeof(var->cVal),
         &count);
        bytesWritten = count;
        break;
    case VT_I2:
    case VT_UI2:
    {
        WORD wTemp;

        StorageUtl_WriteWord(&wTemp, 0, var->iVal);
        hr = IStream_Write(This->stm, &wTemp, sizeof(wTemp), &count);
        bytesWritten = count;
        break;
    }
    case VT_I4:
    case VT_UI4:
    {
        StorageUtl_WriteDWord(&dwTemp, 0, var->lVal);
        hr = IStream_Write(This->stm, &dwTemp, sizeof(dwTemp), &count);
        bytesWritten = count;
        break;
    }
    case VT_I8:
    case VT_UI8:
    {
        StorageUtl_WriteULargeInteger(&ularge, 0, &var->uhVal);
        hr = IStream_Write(This->stm, &ularge, sizeof(ularge), &bytesWritten);
        break;
    }
    case VT_LPSTR:
    {
        if (This->codePage == CP_UNICODE)
            len = (lstrlenW(var->pwszVal) + 1) * sizeof(WCHAR);
        else
            len = lstrlenA(var->pszVal) + 1;
        StorageUtl_WriteDWord(&dwTemp, 0, len);
        hr = IStream_Write(This->stm, &dwTemp, sizeof(dwTemp), &count);
        if (FAILED(hr))
            goto end;
        hr = IStream_Write(This->stm, var->pszVal, len, &count);
        bytesWritten = count + sizeof(DWORD);
        break;
    }
    case VT_BSTR:
    {
        if (This->codePage == CP_UNICODE)
        {
            len = SysStringByteLen(var->bstrVal) + sizeof(WCHAR);
            StorageUtl_WriteDWord(&dwTemp, 0, len);
            hr = IStream_Write(This->stm, &dwTemp, sizeof(dwTemp), &count);
            if (SUCCEEDED(hr))
                hr = IStream_Write(This->stm, var->bstrVal, len, &count);
        }
        else
        {
            char *str;

            len = WideCharToMultiByte(This->codePage, 0, var->bstrVal, SysStringLen(var->bstrVal) + 1,
                    NULL, 0, NULL, NULL);

            str = heap_alloc(len);
            if (!str)
            {
                hr = E_OUTOFMEMORY;
                goto end;
            }

            WideCharToMultiByte(This->codePage, 0, var->bstrVal, SysStringLen(var->bstrVal),
                    str, len, NULL, NULL);
            StorageUtl_WriteDWord(&dwTemp, 0, len);
            hr = IStream_Write(This->stm, &dwTemp, sizeof(dwTemp), &count);
            if (SUCCEEDED(hr))
                hr = IStream_Write(This->stm, str, len, &count);
            heap_free(str);
        }

        bytesWritten = count + sizeof(DWORD);
        break;
    }
    case VT_LPWSTR:
    {
        len = lstrlenW(var->pwszVal) + 1;

        StorageUtl_WriteDWord(&dwTemp, 0, len);
        hr = IStream_Write(This->stm, &dwTemp, sizeof(dwTemp), &count);
        if (FAILED(hr))
            goto end;
        hr = IStream_Write(This->stm, var->pwszVal, len * sizeof(WCHAR),
         &count);
        bytesWritten = count + sizeof(DWORD);
        break;
    }
    case VT_FILETIME:
    {
        FILETIME temp;

        StorageUtl_WriteULargeInteger(&temp, 0, (const ULARGE_INTEGER *)&var->filetime);
        hr = IStream_Write(This->stm, &temp, sizeof(temp), &count);
        bytesWritten = count;
        break;
    }
    case VT_CF:
    {
        DWORD cf_hdr[2];

        len = var->pclipdata->cbSize;
        StorageUtl_WriteDWord(&cf_hdr[0], 0, len + 8);
        StorageUtl_WriteDWord(&cf_hdr[1], 0, var->pclipdata->ulClipFmt);
        hr = IStream_Write(This->stm, cf_hdr, sizeof(cf_hdr), &count);
        if (FAILED(hr))
            goto end;
        hr = IStream_Write(This->stm, var->pclipdata->pClipData,
                           len - sizeof(var->pclipdata->ulClipFmt), &count);
        if (FAILED(hr))
            goto end;
        bytesWritten = count + sizeof cf_hdr;
        break;
    }
    case VT_CLSID:
    {
        CLSID temp;

        StorageUtl_WriteGUID(&temp, 0, var->puuid);
        hr = IStream_Write(This->stm, &temp, sizeof(temp), &count);
        bytesWritten = count;
        break;
    }
    case VT_BLOB:
    {
        StorageUtl_WriteDWord(&dwTemp, 0, var->blob.cbSize);
        hr = IStream_Write(This->stm, &dwTemp, sizeof(dwTemp), &count);
        if (FAILED(hr))
            goto end;
        hr = IStream_Write(This->stm, var->blob.pBlobData, var->blob.cbSize, &count);
        bytesWritten = count + sizeof(DWORD);
        break;
    }
    default:
        FIXME("unsupported type: %d\n", var->vt);
        return STG_E_INVALIDPARAMETER;
    }

    if (SUCCEEDED(hr))
    {
        *sectionOffset += bytesWritten;
        if (bytesWritten % sizeof(DWORD))
        {
            DWORD padding = sizeof(DWORD) - bytesWritten % sizeof(DWORD);
            TRACE("adding %ld bytes of padding\n", padding);
            *sectionOffset += padding;
        }
    }

end:
    return hr;
}

struct PropertyClosure
{
    HRESULT hr;
    DWORD   propNum;
    DWORD  *sectionOffset;
};

static BOOL PropertyStorage_PropertiesWriter(const void *key, const void *value,
 void *extra, void *closure)
{
    PropertyStorage_impl *This = extra;
    struct PropertyClosure *c = closure;

    assert(key);
    assert(value);
    assert(extra);
    assert(closure);
    c->hr = PropertyStorage_WritePropertyToStream(This, c->propNum++,
                                                  PtrToUlong(key), value, c->sectionOffset);
    return SUCCEEDED(c->hr);
}

static HRESULT PropertyStorage_WritePropertiesToStream(
 PropertyStorage_impl *This, DWORD startingPropNum, DWORD *sectionOffset)
{
    struct PropertyClosure closure;

    assert(sectionOffset);
    closure.hr = S_OK;
    closure.propNum = startingPropNum;
    closure.sectionOffset = sectionOffset;
    dictionary_enumerate(This->propid_to_prop, PropertyStorage_PropertiesWriter,
     &closure);
    return closure.hr;
}

static HRESULT PropertyStorage_WriteHeadersToStream(PropertyStorage_impl *This)
{
    HRESULT hr;
    ULONG count = 0;
    LARGE_INTEGER seek = { {0} };
    PROPERTYSETHEADER hdr;
    FORMATIDOFFSET fmtOffset;

    hr = IStream_Seek(This->stm, seek, STREAM_SEEK_SET, NULL);
    if (FAILED(hr))
        goto end;
    PropertyStorage_MakeHeader(This, &hdr);
    hr = IStream_Write(This->stm, &hdr, sizeof(hdr), &count);
    if (FAILED(hr))
        goto end;
    if (count != sizeof(hdr))
    {
        hr = STG_E_WRITEFAULT;
        goto end;
    }

    PropertyStorage_MakeFmtIdOffset(This, &fmtOffset);
    hr = IStream_Write(This->stm, &fmtOffset, sizeof(fmtOffset), &count);
    if (FAILED(hr))
        goto end;
    if (count != sizeof(fmtOffset))
    {
        hr = STG_E_WRITEFAULT;
        goto end;
    }
    hr = S_OK;

end:
    return hr;
}

static HRESULT PropertyStorage_WriteToStream(PropertyStorage_impl *This)
{
    PROPERTYSECTIONHEADER sectionHdr;
    HRESULT hr;
    ULONG count;
    LARGE_INTEGER seek;
    DWORD numProps, prop, sectionOffset, dwTemp;
    PROPVARIANT var;

    PropertyStorage_WriteHeadersToStream(This);

    /* Count properties.  Always at least one property, the code page */
    numProps = 1;
    if (dictionary_num_entries(This->name_to_propid))
        numProps++;
    if (This->locale != LOCALE_SYSTEM_DEFAULT)
        numProps++;
    if (This->grfFlags & PROPSETFLAG_CASE_SENSITIVE)
        numProps++;
    numProps += dictionary_num_entries(This->propid_to_prop);

    /* Write section header with 0 bytes right now, I'll adjust it after
     * writing properties.
     */
    PropertyStorage_MakeSectionHdr(0, numProps, &sectionHdr);
    seek.QuadPart = SECTIONHEADER_OFFSET;
    hr = IStream_Seek(This->stm, seek, STREAM_SEEK_SET, NULL);
    if (FAILED(hr))
        goto end;
    hr = IStream_Write(This->stm, &sectionHdr, sizeof(sectionHdr), &count);
    if (FAILED(hr))
        goto end;

    prop = 0;
    sectionOffset = sizeof(PROPERTYSECTIONHEADER) +
     numProps * sizeof(PROPERTYIDOFFSET);

    if (dictionary_num_entries(This->name_to_propid))
    {
        prop++;
        hr = PropertyStorage_WriteDictionaryToStream(This, &sectionOffset);
        if (FAILED(hr))
            goto end;
    }

    PropVariantInit(&var);

    var.vt = VT_I2;
    var.iVal = This->codePage;
    hr = PropertyStorage_WritePropertyToStream(This, prop++, PID_CODEPAGE,
     &var, &sectionOffset);
    if (FAILED(hr))
        goto end;

    if (This->locale != LOCALE_SYSTEM_DEFAULT)
    {
        var.vt = VT_I4;
        var.lVal = This->locale;
        hr = PropertyStorage_WritePropertyToStream(This, prop++, PID_LOCALE,
         &var, &sectionOffset);
        if (FAILED(hr))
            goto end;
    }

    if (This->grfFlags & PROPSETFLAG_CASE_SENSITIVE)
    {
        var.vt = VT_I4;
        var.lVal = 1;
        hr = PropertyStorage_WritePropertyToStream(This, prop++, PID_BEHAVIOR,
         &var, &sectionOffset);
        if (FAILED(hr))
            goto end;
    }

    hr = PropertyStorage_WritePropertiesToStream(This, prop, &sectionOffset);
    if (FAILED(hr))
        goto end;

    /* Now write the byte count of the section */
    seek.QuadPart = SECTIONHEADER_OFFSET;
    hr = IStream_Seek(This->stm, seek, STREAM_SEEK_SET, NULL);
    if (FAILED(hr))
        goto end;
    StorageUtl_WriteDWord(&dwTemp, 0, sectionOffset);
    hr = IStream_Write(This->stm, &dwTemp, sizeof(dwTemp), &count);

end:
    return hr;
}

/***********************************************************************
 * PropertyStorage_Construct
 */
static void PropertyStorage_DestroyDictionaries(PropertyStorage_impl *This)
{
    dictionary_destroy(This->name_to_propid);
    This->name_to_propid = NULL;
    dictionary_destroy(This->propid_to_name);
    This->propid_to_name = NULL;
    dictionary_destroy(This->propid_to_prop);
    This->propid_to_prop = NULL;
}

static HRESULT PropertyStorage_CreateDictionaries(PropertyStorage_impl *This)
{
    HRESULT hr = S_OK;

    This->name_to_propid = dictionary_create(
     PropertyStorage_PropNameCompare, PropertyStorage_PropNameDestroy,
     This);
    if (!This->name_to_propid)
    {
        hr = STG_E_INSUFFICIENTMEMORY;
        goto end;
    }
    This->propid_to_name = dictionary_create(PropertyStorage_PropCompare,
     NULL, This);
    if (!This->propid_to_name)
    {
        hr = STG_E_INSUFFICIENTMEMORY;
        goto end;
    }
    This->propid_to_prop = dictionary_create(PropertyStorage_PropCompare,
     PropertyStorage_PropertyDestroy, This);
    if (!This->propid_to_prop)
    {
        hr = STG_E_INSUFFICIENTMEMORY;
        goto end;
    }
end:
    if (FAILED(hr))
        PropertyStorage_DestroyDictionaries(This);
    return hr;
}

static HRESULT PropertyStorage_BaseConstruct(IStream *stm,
 REFFMTID rfmtid, DWORD grfMode, PropertyStorage_impl **pps)
{
    HRESULT hr = S_OK;

    assert(pps);
    assert(rfmtid);
    *pps = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof **pps);
    if (!*pps)
        return E_OUTOFMEMORY;

    (*pps)->IPropertyStorage_iface.lpVtbl = &IPropertyStorage_Vtbl;
    (*pps)->ref = 1;
    InitializeCriticalSectionEx(&(*pps)->cs, 0, RTL_CRITICAL_SECTION_FLAG_FORCE_DEBUG_INFO);
    (*pps)->cs.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": PropertyStorage_impl.cs");
    (*pps)->stm = stm;
    (*pps)->fmtid = *rfmtid;
    (*pps)->grfMode = grfMode;

    hr = PropertyStorage_CreateDictionaries(*pps);
    if (FAILED(hr))
    {
        (*pps)->cs.DebugInfo->Spare[0] = 0;
        DeleteCriticalSection(&(*pps)->cs);
        HeapFree(GetProcessHeap(), 0, *pps);
        *pps = NULL;
    }
    else IStream_AddRef( stm );

    return hr;
}

static HRESULT PropertyStorage_ConstructFromStream(IStream *stm,
 REFFMTID rfmtid, DWORD grfMode, IPropertyStorage** pps)
{
    PropertyStorage_impl *ps;
    HRESULT hr;

    assert(pps);
    hr = PropertyStorage_BaseConstruct(stm, rfmtid, grfMode, &ps);
    if (SUCCEEDED(hr))
    {
        hr = PropertyStorage_ReadFromStream(ps);
        if (SUCCEEDED(hr))
        {
            *pps = &ps->IPropertyStorage_iface;
            TRACE("PropertyStorage %p constructed\n", ps);
            hr = S_OK;
        }
        else IPropertyStorage_Release( &ps->IPropertyStorage_iface );
    }
    return hr;
}

static HRESULT PropertyStorage_ConstructEmpty(IStream *stm,
 REFFMTID rfmtid, DWORD grfFlags, DWORD grfMode, IPropertyStorage** pps)
{
    PropertyStorage_impl *ps;
    HRESULT hr;

    assert(pps);
    hr = PropertyStorage_BaseConstruct(stm, rfmtid, grfMode, &ps);
    if (SUCCEEDED(hr))
    {
        ps->format = 0;
        ps->grfFlags = grfFlags;
        if (ps->grfFlags & PROPSETFLAG_CASE_SENSITIVE)
            ps->format = 1;
        /* default to Unicode unless told not to, as specified on msdn */
        if (ps->grfFlags & PROPSETFLAG_ANSI)
            ps->codePage = GetACP();
        else
            ps->codePage = CP_UNICODE;
        ps->locale = LOCALE_SYSTEM_DEFAULT;
        TRACE("Code page is %d, locale is %ld\n", ps->codePage, ps->locale);
        *pps = &ps->IPropertyStorage_iface;
        TRACE("PropertyStorage %p constructed\n", ps);
        hr = S_OK;
    }
    return hr;
}


/***********************************************************************
 * Implementation of IPropertySetStorage
 */

struct enum_stat_propset_stg
{
    IEnumSTATPROPSETSTG IEnumSTATPROPSETSTG_iface;
    LONG refcount;
    STATPROPSETSTG *stats;
    size_t current;
    size_t count;
};

static struct enum_stat_propset_stg *impl_from_IEnumSTATPROPSETSTG(IEnumSTATPROPSETSTG *iface)
{
    return CONTAINING_RECORD(iface, struct enum_stat_propset_stg, IEnumSTATPROPSETSTG_iface);
}

static HRESULT WINAPI enum_stat_propset_stg_QueryInterface(IEnumSTATPROPSETSTG *iface, REFIID riid, void **obj)
{
    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), obj);

    if (IsEqualIID(riid, &IID_IEnumSTATPROPSETSTG) ||
            IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
        IEnumSTATPROPSETSTG_AddRef(iface);
        return S_OK;
    }

    WARN("Unsupported interface %s.\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI enum_stat_propset_stg_AddRef(IEnumSTATPROPSETSTG *iface)
{
    struct enum_stat_propset_stg *psenum = impl_from_IEnumSTATPROPSETSTG(iface);
    LONG refcount = InterlockedIncrement(&psenum->refcount);

    TRACE("%p, refcount %lu.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI enum_stat_propset_stg_Release(IEnumSTATPROPSETSTG *iface)
{
    struct enum_stat_propset_stg *psenum = impl_from_IEnumSTATPROPSETSTG(iface);
    LONG refcount = InterlockedDecrement(&psenum->refcount);

    TRACE("%p, refcount %lu.\n", iface, refcount);

    if (!refcount)
    {
        heap_free(psenum->stats);
        heap_free(psenum);
    }

    return refcount;
}

static HRESULT WINAPI enum_stat_propset_stg_Next(IEnumSTATPROPSETSTG *iface, ULONG celt,
    STATPROPSETSTG *ret, ULONG *fetched)
{
    struct enum_stat_propset_stg *psenum = impl_from_IEnumSTATPROPSETSTG(iface);
    ULONG count = 0;

    TRACE("%p, %lu, %p, %p.\n", iface, celt, ret, fetched);

    if (psenum->current == ~0u)
        psenum->current = 0;

    while (count < celt && psenum->current < psenum->count)
        ret[count++] = psenum->stats[psenum->current++];

    if (fetched)
        *fetched = count;

    return count < celt ? S_FALSE : S_OK;
}

static HRESULT WINAPI enum_stat_propset_stg_Skip(IEnumSTATPROPSETSTG *iface, ULONG celt)
{
    FIXME("%p, %lu.\n", iface, celt);

    return S_OK;
}

static HRESULT WINAPI enum_stat_propset_stg_Reset(IEnumSTATPROPSETSTG *iface)
{
    struct enum_stat_propset_stg *psenum = impl_from_IEnumSTATPROPSETSTG(iface);

    TRACE("%p.\n", iface);

    psenum->current = ~0u;

    return S_OK;
}

static HRESULT WINAPI enum_stat_propset_stg_Clone(IEnumSTATPROPSETSTG *iface, IEnumSTATPROPSETSTG **ppenum)
{
    FIXME("%p, %p.\n", iface, ppenum);

    return E_NOTIMPL;
}

static const IEnumSTATPROPSETSTGVtbl enum_stat_propset_stg_vtbl =
{
    enum_stat_propset_stg_QueryInterface,
    enum_stat_propset_stg_AddRef,
    enum_stat_propset_stg_Release,
    enum_stat_propset_stg_Next,
    enum_stat_propset_stg_Skip,
    enum_stat_propset_stg_Reset,
    enum_stat_propset_stg_Clone,
};

static HRESULT create_enum_stat_propset_stg(StorageImpl *storage, IEnumSTATPROPSETSTG **ret)
{
    IStorage *stg = &storage->base.IStorage_iface;
    IEnumSTATSTG *penum = NULL;
    STATSTG stat;
    ULONG count;
    HRESULT hr;

    struct enum_stat_propset_stg *enum_obj;

    enum_obj = heap_alloc_zero(sizeof(*enum_obj));
    if (!enum_obj)
        return E_OUTOFMEMORY;

    enum_obj->IEnumSTATPROPSETSTG_iface.lpVtbl = &enum_stat_propset_stg_vtbl;
    enum_obj->refcount = 1;

    /* add all the property set elements into a list */
    hr = IStorage_EnumElements(stg, 0, NULL, 0, &penum);
    if (FAILED(hr))
        goto done;

    /* Allocate stats array and fill it. */
    while ((hr = IEnumSTATSTG_Next(penum, 1, &stat, &count)) == S_OK)
    {
        enum_obj->count++;
        CoTaskMemFree(stat.pwcsName);
    }

    if (FAILED(hr))
        goto done;

    enum_obj->stats = heap_alloc(enum_obj->count * sizeof(*enum_obj->stats));
    if (!enum_obj->stats)
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }
    enum_obj->count = 0;

    if (FAILED(hr = IEnumSTATSTG_Reset(penum)))
        goto done;

    while (IEnumSTATSTG_Next(penum, 1, &stat, &count) == S_OK)
    {
        if (!stat.pwcsName)
            continue;

        if (stat.pwcsName[0] == 5 && stat.type == STGTY_STREAM)
        {
            STATPROPSETSTG *ptr = &enum_obj->stats[enum_obj->count++];

            PropStgNameToFmtId(stat.pwcsName, &ptr->fmtid);

            TRACE("adding %s - %s.\n", debugstr_w(stat.pwcsName), debugstr_guid(&ptr->fmtid));

            ptr->mtime = stat.mtime;
            ptr->atime = stat.atime;
            ptr->ctime = stat.ctime;
            ptr->grfFlags = stat.grfMode;
            ptr->clsid = stat.clsid;
        }
        CoTaskMemFree(stat.pwcsName);
    }

done:

    if (penum)
        IEnumSTATSTG_Release(penum);

    if (SUCCEEDED(hr))
    {
        *ret = &enum_obj->IEnumSTATPROPSETSTG_iface;
    }
    else
    {
        *ret = NULL;
        IEnumSTATPROPSETSTG_Release(&enum_obj->IEnumSTATPROPSETSTG_iface);
    }

    return hr;
}

/************************************************************************
 * IPropertySetStorage_fnQueryInterface (IUnknown)
 *
 *  This method forwards to the common QueryInterface implementation
 */
static HRESULT WINAPI IPropertySetStorage_fnQueryInterface(
    IPropertySetStorage *ppstg,
    REFIID riid,
    void** ppvObject)
{
    StorageImpl *This = impl_from_IPropertySetStorage(ppstg);
    return IStorage_QueryInterface( &This->base.IStorage_iface, riid, ppvObject );
}

/************************************************************************
 * IPropertySetStorage_fnAddRef (IUnknown)
 *
 *  This method forwards to the common AddRef implementation
 */
static ULONG WINAPI IPropertySetStorage_fnAddRef(
    IPropertySetStorage *ppstg)
{
    StorageImpl *This = impl_from_IPropertySetStorage(ppstg);
    return IStorage_AddRef( &This->base.IStorage_iface );
}

/************************************************************************
 * IPropertySetStorage_fnRelease (IUnknown)
 *
 *  This method forwards to the common Release implementation
 */
static ULONG WINAPI IPropertySetStorage_fnRelease(
    IPropertySetStorage *ppstg)
{
    StorageImpl *This = impl_from_IPropertySetStorage(ppstg);
    return IStorage_Release( &This->base.IStorage_iface );
}

/************************************************************************
 * IPropertySetStorage_fnCreate (IPropertySetStorage)
 */
static HRESULT WINAPI IPropertySetStorage_fnCreate(
    IPropertySetStorage *ppstg,
    REFFMTID rfmtid,
    const CLSID* pclsid,
    DWORD grfFlags,
    DWORD grfMode,
    IPropertyStorage** ppprstg)
{
    StorageImpl *This = impl_from_IPropertySetStorage(ppstg);
    WCHAR name[CCH_MAX_PROPSTG_NAME + 1];
    IStream *stm = NULL;
    HRESULT r;

    TRACE("%p, %s %#lx, %#lx, %p.\n", This, debugstr_guid(rfmtid), grfFlags,
     grfMode, ppprstg);

    /* be picky */
    if (grfMode != (STGM_CREATE|STGM_READWRITE|STGM_SHARE_EXCLUSIVE))
    {
        r = STG_E_INVALIDFLAG;
        goto end;
    }

    if (!rfmtid)
    {
        r = E_INVALIDARG;
        goto end;
    }

    /* FIXME: if (grfFlags & PROPSETFLAG_NONSIMPLE), we need to create a
     * storage, not a stream.  For now, disallow it.
     */
    if (grfFlags & PROPSETFLAG_NONSIMPLE)
    {
        FIXME("PROPSETFLAG_NONSIMPLE not supported\n");
        r = STG_E_INVALIDFLAG;
        goto end;
    }

    r = FmtIdToPropStgName(rfmtid, name);
    if (FAILED(r))
        goto end;

    r = IStorage_CreateStream( &This->base.IStorage_iface, name, grfMode, 0, 0, &stm );
    if (FAILED(r))
        goto end;

    r = PropertyStorage_ConstructEmpty(stm, rfmtid, grfFlags, grfMode, ppprstg);

    IStream_Release( stm );

end:
    TRACE("returning %#lx\n", r);
    return r;
}

/************************************************************************
 * IPropertySetStorage_fnOpen (IPropertySetStorage)
 */
static HRESULT WINAPI IPropertySetStorage_fnOpen(
    IPropertySetStorage *ppstg,
    REFFMTID rfmtid,
    DWORD grfMode,
    IPropertyStorage** ppprstg)
{
    StorageImpl *This = impl_from_IPropertySetStorage(ppstg);
    IStream *stm = NULL;
    WCHAR name[CCH_MAX_PROPSTG_NAME + 1];
    HRESULT r;

    TRACE("%p, %s, %#lx, %p.\n", This, debugstr_guid(rfmtid), grfMode, ppprstg);

    /* be picky */
    if (grfMode != (STGM_READWRITE|STGM_SHARE_EXCLUSIVE) &&
        grfMode != (STGM_READ|STGM_SHARE_EXCLUSIVE))
    {
        r = STG_E_INVALIDFLAG;
        goto end;
    }

    if (!rfmtid)
    {
        r = E_INVALIDARG;
        goto end;
    }

    r = FmtIdToPropStgName(rfmtid, name);
    if (FAILED(r))
        goto end;

    r = IStorage_OpenStream( &This->base.IStorage_iface, name, 0, grfMode, 0, &stm );
    if (FAILED(r))
        goto end;

    r = PropertyStorage_ConstructFromStream(stm, rfmtid, grfMode, ppprstg);

    IStream_Release( stm );

end:
    TRACE("returning %#lx\n", r);
    return r;
}

/************************************************************************
 * IPropertySetStorage_fnDelete (IPropertySetStorage)
 */
static HRESULT WINAPI IPropertySetStorage_fnDelete(
    IPropertySetStorage *ppstg,
    REFFMTID rfmtid)
{
    StorageImpl *This = impl_from_IPropertySetStorage(ppstg);
    WCHAR name[CCH_MAX_PROPSTG_NAME + 1];
    HRESULT r;

    TRACE("%p %s\n", This, debugstr_guid(rfmtid));

    if (!rfmtid)
        return E_INVALIDARG;

    r = FmtIdToPropStgName(rfmtid, name);
    if (FAILED(r))
        return r;

    return IStorage_DestroyElement(&This->base.IStorage_iface, name);
}

static HRESULT WINAPI IPropertySetStorage_fnEnum(IPropertySetStorage *iface, IEnumSTATPROPSETSTG **enum_obj)
{
    StorageImpl *storage = impl_from_IPropertySetStorage(iface);

    TRACE("%p, %p.\n", iface, enum_obj);

    if (!enum_obj)
        return E_INVALIDARG;

    return create_enum_stat_propset_stg(storage, enum_obj);
}

/***********************************************************************
 * vtables
 */
const IPropertySetStorageVtbl IPropertySetStorage_Vtbl =
{
    IPropertySetStorage_fnQueryInterface,
    IPropertySetStorage_fnAddRef,
    IPropertySetStorage_fnRelease,
    IPropertySetStorage_fnCreate,
    IPropertySetStorage_fnOpen,
    IPropertySetStorage_fnDelete,
    IPropertySetStorage_fnEnum
};

static const IPropertyStorageVtbl IPropertyStorage_Vtbl =
{
    IPropertyStorage_fnQueryInterface,
    IPropertyStorage_fnAddRef,
    IPropertyStorage_fnRelease,
    IPropertyStorage_fnReadMultiple,
    IPropertyStorage_fnWriteMultiple,
    IPropertyStorage_fnDeleteMultiple,
    IPropertyStorage_fnReadPropertyNames,
    IPropertyStorage_fnWritePropertyNames,
    IPropertyStorage_fnDeletePropertyNames,
    IPropertyStorage_fnCommit,
    IPropertyStorage_fnRevert,
    IPropertyStorage_fnEnum,
    IPropertyStorage_fnSetTimes,
    IPropertyStorage_fnSetClass,
    IPropertyStorage_fnStat,
};

#ifdef __i386__  /* thiscall functions are i386-specific */

#define DEFINE_STDCALL_WRAPPER(num,func,args) \
   __ASM_STDCALL_FUNC(func, args, \
                   "popl %eax\n\t" \
                   "popl %ecx\n\t" \
                   "pushl %eax\n\t" \
                   "movl (%ecx), %eax\n\t" \
                   "jmp *(4*(" #num "))(%eax)" )

DEFINE_STDCALL_WRAPPER(0,Allocate_PMemoryAllocator,8)
extern void* __stdcall Allocate_PMemoryAllocator(void *this, ULONG cbSize);

#else

static void* __cdecl Allocate_PMemoryAllocator(void *this, ULONG cbSize)
{
    void* (__cdecl *fn)(void*,ULONG) = **(void***)this;
    return fn(this, cbSize);
}

#endif

BOOLEAN WINAPI StgConvertPropertyToVariant(const SERIALIZEDPROPERTYVALUE* prop,
    USHORT CodePage, PROPVARIANT* pvar, void* pma)
{
    struct read_buffer read_buffer;
    HRESULT hr;

    read_buffer.data = (BYTE *)prop;
    read_buffer.size = ~(size_t)0;
    hr = PropertyStorage_ReadProperty(pvar, &read_buffer, 0, CodePage, Allocate_PMemoryAllocator, pma);

    if (FAILED(hr))
    {
        FIXME("should raise C++ exception on failure\n");
        PropVariantInit(pvar);
    }

    return FALSE;
}

SERIALIZEDPROPERTYVALUE* WINAPI StgConvertVariantToProperty(const PROPVARIANT *pvar,
    USHORT CodePage, SERIALIZEDPROPERTYVALUE *pprop, ULONG *pcb, PROPID pid,
    BOOLEAN fReserved, ULONG *pcIndirect)
{
    FIXME("%p, %d, %p, %p, %ld, %d, %p.\n", pvar, CodePage, pprop, pcb, pid, fReserved, pcIndirect);

    return NULL;
}

HRESULT WINAPI StgCreatePropStg(IUnknown *unk, REFFMTID fmt, const CLSID *clsid,
                                DWORD flags, DWORD reserved, IPropertyStorage **prop_stg)
{
    IStorage *stg;
    IStream *stm;
    HRESULT r;

    TRACE("%p, %s, %s, %#lx, %ld, %p.\n", unk, debugstr_guid(fmt), debugstr_guid(clsid), flags, reserved, prop_stg);

    if (!fmt || reserved)
    {
        r = E_INVALIDARG;
        goto end;
    }

    if (flags & PROPSETFLAG_NONSIMPLE)
    {
        r = IUnknown_QueryInterface(unk, &IID_IStorage, (void **)&stg);
        if (FAILED(r))
            goto end;

        /* FIXME: if (flags & PROPSETFLAG_NONSIMPLE), we need to create a
         * storage, not a stream.  For now, disallow it.
         */
        FIXME("PROPSETFLAG_NONSIMPLE not supported\n");
        IStorage_Release(stg);
        r = STG_E_INVALIDFLAG;
    }
    else
    {
        r = IUnknown_QueryInterface(unk, &IID_IStream, (void **)&stm);
        if (FAILED(r))
            goto end;

        r = PropertyStorage_ConstructEmpty(stm, fmt, flags,
                STGM_CREATE|STGM_READWRITE|STGM_SHARE_EXCLUSIVE, prop_stg);

        IStream_Release( stm );
    }

end:
    TRACE("returning %#lx\n", r);
    return r;
}

HRESULT WINAPI StgOpenPropStg(IUnknown *unk, REFFMTID fmt, DWORD flags,
                              DWORD reserved, IPropertyStorage **prop_stg)
{
    IStorage *stg;
    IStream *stm;
    HRESULT r;

    TRACE("%p, %s, %#lx, %ld, %p.\n", unk, debugstr_guid(fmt), flags, reserved, prop_stg);

    if (!fmt || reserved)
    {
        r = E_INVALIDARG;
        goto end;
    }

    if (flags & PROPSETFLAG_NONSIMPLE)
    {
        r = IUnknown_QueryInterface(unk, &IID_IStorage, (void **)&stg);
        if (FAILED(r))
            goto end;

        /* FIXME: if (flags & PROPSETFLAG_NONSIMPLE), we need to open a
         * storage, not a stream.  For now, disallow it.
         */
        FIXME("PROPSETFLAG_NONSIMPLE not supported\n");
        IStorage_Release(stg);
        r = STG_E_INVALIDFLAG;
    }
    else
    {
        r = IUnknown_QueryInterface(unk, &IID_IStream, (void **)&stm);
        if (FAILED(r))
            goto end;

        r = PropertyStorage_ConstructFromStream(stm, fmt,
                STGM_READWRITE|STGM_SHARE_EXCLUSIVE, prop_stg);

        IStream_Release( stm );
    }

end:
    TRACE("returning %#lx\n", r);
    return r;
}
