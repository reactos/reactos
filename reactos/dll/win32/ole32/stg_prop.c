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
#define NONAMELESSUNION
#define NONAMELESSSTRUCT

#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "winuser.h"
#include "wine/unicode.h"
#include "wine/debug.h"
#include "dictionary.h"
#include "storage32.h"
#include "enumx.h"

WINE_DEFAULT_DEBUG_CHANNEL(storage);

static inline StorageImpl *impl_from_IPropertySetStorage( IPropertySetStorage *iface )
{
    return (StorageImpl *)((char*)iface - FIELD_OFFSET(StorageImpl, base.pssVtbl));
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
 const PROPVARIANT *propvar, LCID targetCP, LCID srcCP);

/* Copies the string src, which is encoded using code page srcCP, and returns
 * it in *dst, in the code page specified by targetCP.  The returned string is
 * allocated using CoTaskMemAlloc.
 * If srcCP is CP_UNICODE, src is in fact an LPCWSTR.  Similarly, if targetCP
 * is CP_UNICODE, the returned string is in fact an LPWSTR.
 * Returns S_OK on success, something else on failure.
 */
static HRESULT PropertyStorage_StringCopy(LPCSTR src, LCID srcCP, LPSTR *dst,
 LCID targetCP);

static const IPropertyStorageVtbl IPropertyStorage_Vtbl;
static const IEnumSTATPROPSETSTGVtbl IEnumSTATPROPSETSTG_Vtbl;
static const IEnumSTATPROPSTGVtbl IEnumSTATPROPSTG_Vtbl;
static HRESULT create_EnumSTATPROPSETSTG(StorageImpl *, IEnumSTATPROPSETSTG**);
static HRESULT create_EnumSTATPROPSTG(PropertyStorage_impl *, IEnumSTATPROPSTG**);

/***********************************************************************
 * Implementation of IPropertyStorage
 */
struct tagPropertyStorage_impl
{
    const IPropertyStorageVtbl *vtbl;
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

/************************************************************************
 * IPropertyStorage_fnQueryInterface (IPropertyStorage)
 */
static HRESULT WINAPI IPropertyStorage_fnQueryInterface(
    IPropertyStorage *iface,
    REFIID riid,
    void** ppvObject)
{
    PropertyStorage_impl *This = (PropertyStorage_impl *)iface;

    if ( (This==0) || (ppvObject==0) )
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
    PropertyStorage_impl *This = (PropertyStorage_impl *)iface;
    return InterlockedIncrement(&This->ref);
}

/************************************************************************
 * IPropertyStorage_fnRelease (IPropertyStorage)
 */
static ULONG WINAPI IPropertyStorage_fnRelease(
    IPropertyStorage *iface)
{
    PropertyStorage_impl *This = (PropertyStorage_impl *)iface;
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

    dictionary_find(This->propid_to_prop, (void *)propid, (void **)&ret);
    TRACE("returning %p\n", ret);
    return ret;
}

/* Returns NULL if name is NULL. */
static PROPVARIANT *PropertyStorage_FindPropertyByName(
 PropertyStorage_impl *This, LPCWSTR name)
{
    PROPVARIANT *ret = NULL;
    PROPID propid;

    if (!name)
        return NULL;
    if (This->codePage == CP_UNICODE)
    {
        if (dictionary_find(This->name_to_propid, name, (void **)&propid))
            ret = PropertyStorage_FindProperty(This, propid);
    }
    else
    {
        LPSTR ansiName;
        HRESULT hr = PropertyStorage_StringCopy((LPCSTR)name, CP_UNICODE,
         &ansiName, This->codePage);

        if (SUCCEEDED(hr))
        {
            if (dictionary_find(This->name_to_propid, ansiName,
             (void **)&propid))
                ret = PropertyStorage_FindProperty(This, propid);
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

    dictionary_find(This->propid_to_name, (void *)propid, (void **)&ret);
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
    PropertyStorage_impl *This = (PropertyStorage_impl *)iface;
    HRESULT hr = S_OK;
    ULONG i;

    TRACE("(%p, %d, %p, %p)\n", iface, cpspec, rgpspec, rgpropvar);

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
             rgpspec[i].u.lpwstr);

            if (prop)
                PropertyStorage_PropVariantCopy(&rgpropvar[i], prop, GetACP(),
                 This->codePage);
        }
        else
        {
            switch (rgpspec[i].u.propid)
            {
                case PID_CODEPAGE:
                    rgpropvar[i].vt = VT_I2;
                    rgpropvar[i].u.iVal = This->codePage;
                    break;
                case PID_LOCALE:
                    rgpropvar[i].vt = VT_I4;
                    rgpropvar[i].u.lVal = This->locale;
                    break;
                default:
                {
                    PROPVARIANT *prop = PropertyStorage_FindProperty(This,
                     rgpspec[i].u.propid);

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

static HRESULT PropertyStorage_StringCopy(LPCSTR src, LCID srcCP, LPSTR *dst,
 LCID dstCP)
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
            len = (strlenW((LPCWSTR)src) + 1) * sizeof(WCHAR);
        else
            len = strlen(src) + 1;
        *dst = CoTaskMemAlloc(len * sizeof(WCHAR));
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
    TRACE("returning 0x%08x (%s)\n", hr,
     dstCP == CP_UNICODE ? debugstr_w((LPCWSTR)*dst) : debugstr_a(*dst));
    return hr;
}

static HRESULT PropertyStorage_PropVariantCopy(PROPVARIANT *prop,
 const PROPVARIANT *propvar, LCID targetCP, LCID srcCP)
{
    HRESULT hr = S_OK;

    assert(prop);
    assert(propvar);
    if (propvar->vt == VT_LPSTR)
    {
        hr = PropertyStorage_StringCopy(propvar->u.pszVal, srcCP,
         &prop->u.pszVal, targetCP);
        if (SUCCEEDED(hr))
            prop->vt = VT_LPSTR;
    }
    else
        PropVariantCopy(prop, propvar);
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
 PROPID propid, const PROPVARIANT *propvar, LCID lcid)
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
    TRACE("Setting 0x%08x to type %d\n", propid, propvar->vt);
    if (prop)
    {
        PropVariantClear(prop);
        hr = PropertyStorage_PropVariantCopy(prop, propvar, This->codePage,
         lcid);
    }
    else
    {
        prop = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
         sizeof(PROPVARIANT));
        if (prop)
        {
            hr = PropertyStorage_PropVariantCopy(prop, propvar, This->codePage,
             lcid);
            if (SUCCEEDED(hr))
            {
                dictionary_insert(This->propid_to_prop, (void *)propid, prop);
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
 LPCSTR srcName, LCID cp, PROPID id)
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
        TRACE("Adding prop name %s, propid %d\n",
         This->codePage == CP_UNICODE ? debugstr_w((LPCWSTR)name) :
         debugstr_a(name), id);
        dictionary_insert(This->name_to_propid, name, (void *)id);
        dictionary_insert(This->propid_to_name, (void *)id, name);
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
    PropertyStorage_impl *This = (PropertyStorage_impl *)iface;
    HRESULT hr = S_OK;
    ULONG i;

    TRACE("(%p, %d, %p, %p)\n", iface, cpspec, rgpspec, rgpropvar);

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
             rgpspec[i].u.lpwstr);

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
                     (LPCSTR)rgpspec[i].u.lpwstr, CP_UNICODE, nextId);
                    if (SUCCEEDED(hr))
                        hr = PropertyStorage_StorePropWithId(This, nextId,
                         &rgpropvar[i], GetACP());
                }
            }
        }
        else
        {
            switch (rgpspec[i].u.propid)
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
                    This->codePage = rgpropvar[i].u.iVal;
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
                    This->locale = rgpropvar[i].u.lVal;
                else
                    hr = STG_E_INVALIDPARAMETER;
                break;
            case PID_ILLEGAL:
                /* silently ignore like MSDN says */
                break;
            default:
                if (rgpspec[i].u.propid >= PID_MIN_READONLY)
                    hr = STG_E_INVALIDPARAMETER;
                else
                    hr = PropertyStorage_StorePropWithId(This,
                     rgpspec[i].u.propid, &rgpropvar[i], GetACP());
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
    PropertyStorage_impl *This = (PropertyStorage_impl *)iface;
    ULONG i;
    HRESULT hr;

    TRACE("(%p, %d, %p)\n", iface, cpspec, rgpspec);

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
            PROPID propid;

            if (dictionary_find(This->name_to_propid,
             (void *)rgpspec[i].u.lpwstr, (void **)&propid))
                dictionary_remove(This->propid_to_prop, (void *)propid);
        }
        else
        {
            if (rgpspec[i].u.propid >= PID_FIRST_USABLE &&
             rgpspec[i].u.propid < PID_MIN_READONLY)
                dictionary_remove(This->propid_to_prop,
                 (void *)rgpspec[i].u.propid);
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
    PropertyStorage_impl *This = (PropertyStorage_impl *)iface;
    ULONG i;
    HRESULT hr = S_FALSE;

    TRACE("(%p, %d, %p, %p)\n", iface, cpropid, rgpropid, rglpwstrName);

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
            if (rglpwstrName)
                memcpy(rglpwstrName, name, (len + 1) * sizeof(WCHAR));
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
    PropertyStorage_impl *This = (PropertyStorage_impl *)iface;
    ULONG i;
    HRESULT hr;

    TRACE("(%p, %d, %p, %p)\n", iface, cpropid, rgpropid, rglpwstrName);

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
    PropertyStorage_impl *This = (PropertyStorage_impl *)iface;
    ULONG i;
    HRESULT hr;

    TRACE("(%p, %d, %p)\n", iface, cpropid, rgpropid);

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

        if (dictionary_find(This->propid_to_name, (void *)rgpropid[i],
         (void **)&name))
        {
            dictionary_remove(This->propid_to_name, (void *)rgpropid[i]);
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
    PropertyStorage_impl *This = (PropertyStorage_impl *)iface;
    HRESULT hr;

    TRACE("(%p, 0x%08x)\n", iface, grfCommitFlags);

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
    PropertyStorage_impl *This = (PropertyStorage_impl *)iface;

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
static HRESULT WINAPI IPropertyStorage_fnEnum(
    IPropertyStorage* iface,
    IEnumSTATPROPSTG** ppenum)
{
    PropertyStorage_impl *This = (PropertyStorage_impl *)iface;
    return create_EnumSTATPROPSTG(This, ppenum);
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
    PropertyStorage_impl *This = (PropertyStorage_impl *)iface;

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
    PropertyStorage_impl *This = (PropertyStorage_impl *)iface;
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
            return lstrcmpW(a, b);
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
    TRACE("(%d, %d)\n", (PROPID)a, (PROPID)b);
    return (PROPID)a - (PROPID)b;
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
        str[i] = le16toh(str[i]);
}
#else
#define PropertyStorage_ByteSwapString(s, l)
#endif

/* Reads the dictionary from the memory buffer beginning at ptr.  Interprets
 * the entries according to the values of This->codePage and This->locale.
 * FIXME: there isn't any checking whether the read property extends past the
 * end of the buffer.
 */
static HRESULT PropertyStorage_ReadDictionary(PropertyStorage_impl *This,
 BYTE *ptr)
{
    DWORD numEntries, i;
    HRESULT hr = S_OK;

    assert(This->name_to_propid);
    assert(This->propid_to_name);

    StorageUtl_ReadDWord(ptr, 0, &numEntries);
    TRACE("Reading %d entries:\n", numEntries);
    ptr += sizeof(DWORD);
    for (i = 0; SUCCEEDED(hr) && i < numEntries; i++)
    {
        PROPID propid;
        DWORD cbEntry;

        StorageUtl_ReadDWord(ptr, 0, &propid);
        ptr += sizeof(PROPID);
        StorageUtl_ReadDWord(ptr, 0, &cbEntry);
        ptr += sizeof(DWORD);
        TRACE("Reading entry with ID 0x%08x, %d bytes\n", propid, cbEntry);
        /* Make sure the source string is NULL-terminated */
        if (This->codePage != CP_UNICODE)
            ptr[cbEntry - 1] = '\0';
        else
            *((LPWSTR)ptr + cbEntry / sizeof(WCHAR)) = '\0';
        hr = PropertyStorage_StoreNameWithId(This, (char*)ptr, This->codePage, propid);
        if (This->codePage == CP_UNICODE)
        {
            /* Unicode entries are padded to DWORD boundaries */
            if (cbEntry % sizeof(DWORD))
                ptr += sizeof(DWORD) - (cbEntry % sizeof(DWORD));
        }
        ptr += sizeof(DWORD) + cbEntry;
    }
    return hr;
}

/* FIXME: there isn't any checking whether the read property extends past the
 * end of the buffer.
 */
static HRESULT PropertyStorage_ReadProperty(PropertyStorage_impl *This,
 PROPVARIANT *prop, const BYTE *data)
{
    HRESULT hr = S_OK;

    assert(prop);
    assert(data);
    StorageUtl_ReadDWord(data, 0, (DWORD *)&prop->vt);
    data += sizeof(DWORD);
    switch (prop->vt)
    {
    case VT_EMPTY:
    case VT_NULL:
        break;
    case VT_I1:
        prop->u.cVal = *(const char *)data;
        TRACE("Read char 0x%x ('%c')\n", prop->u.cVal, prop->u.cVal);
        break;
    case VT_UI1:
        prop->u.bVal = *data;
        TRACE("Read byte 0x%x\n", prop->u.bVal);
        break;
    case VT_I2:
        StorageUtl_ReadWord(data, 0, (WORD*)&prop->u.iVal);
        TRACE("Read short %d\n", prop->u.iVal);
        break;
    case VT_UI2:
        StorageUtl_ReadWord(data, 0, &prop->u.uiVal);
        TRACE("Read ushort %d\n", prop->u.uiVal);
        break;
    case VT_INT:
    case VT_I4:
        StorageUtl_ReadDWord(data, 0, (DWORD*)&prop->u.lVal);
        TRACE("Read long %d\n", prop->u.lVal);
        break;
    case VT_UINT:
    case VT_UI4:
        StorageUtl_ReadDWord(data, 0, &prop->u.ulVal);
        TRACE("Read ulong %d\n", prop->u.ulVal);
        break;
    case VT_LPSTR:
    {
        DWORD count;
       
        StorageUtl_ReadDWord(data, 0, &count);
        if (This->codePage == CP_UNICODE && count / 2)
        {
            WARN("Unicode string has odd number of bytes\n");
            hr = STG_E_INVALIDHEADER;
        }
        else
        {
            prop->u.pszVal = CoTaskMemAlloc(count);
            if (prop->u.pszVal)
            {
                memcpy(prop->u.pszVal, data + sizeof(DWORD), count);
                /* This is stored in the code page specified in This->codePage.
                 * Don't convert it, the caller will just store it as-is.
                 */
                if (This->codePage == CP_UNICODE)
                {
                    /* Make sure it's NULL-terminated */
                    prop->u.pszVal[count / sizeof(WCHAR) - 1] = '\0';
                    TRACE("Read string value %s\n",
                     debugstr_w(prop->u.pwszVal));
                }
                else
                {
                    /* Make sure it's NULL-terminated */
                    prop->u.pszVal[count - 1] = '\0';
                    TRACE("Read string value %s\n", debugstr_a(prop->u.pszVal));
                }
            }
            else
                hr = STG_E_INSUFFICIENTMEMORY;
        }
        break;
    }
    case VT_LPWSTR:
    {
        DWORD count;

        StorageUtl_ReadDWord(data, 0, &count);
        prop->u.pwszVal = CoTaskMemAlloc(count * sizeof(WCHAR));
        if (prop->u.pwszVal)
        {
            memcpy(prop->u.pwszVal, data + sizeof(DWORD),
             count * sizeof(WCHAR));
            /* make sure string is NULL-terminated */
            prop->u.pwszVal[count - 1] = '\0';
            PropertyStorage_ByteSwapString(prop->u.pwszVal, count);
            TRACE("Read string value %s\n", debugstr_w(prop->u.pwszVal));
        }
        else
            hr = STG_E_INSUFFICIENTMEMORY;
        break;
    }
    case VT_FILETIME:
        StorageUtl_ReadULargeInteger(data, 0,
         (ULARGE_INTEGER *)&prop->u.filetime);
        break;
    case VT_CF:
        {
            DWORD len = 0, tag = 0;

            StorageUtl_ReadDWord(data, 0, &len);
            StorageUtl_ReadDWord(data, 4, &tag);
            if (len > 8)
            {
                len -= 8;
                prop->u.pclipdata = CoTaskMemAlloc(sizeof (CLIPDATA));
                prop->u.pclipdata->cbSize = len;
                prop->u.pclipdata->ulClipFmt = tag;
                prop->u.pclipdata->pClipData = CoTaskMemAlloc(len);
                memcpy(prop->u.pclipdata->pClipData, data+8, len);
            }
            else
                hr = STG_E_INVALIDPARAMETER;
        }
        break;
    default:
        FIXME("unsupported type %d\n", prop->vt);
        hr = STG_E_INVALIDPARAMETER;
    }
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
            WARN("read only %d\n", count);
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
    TRACE("returning 0x%08x\n", hr);
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
            WARN("read only %d\n", count);
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
    TRACE("returning 0x%08x\n", hr);
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
            WARN("read only %d\n", count);
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
    TRACE("returning 0x%08x\n", hr);
    return hr;
}

static HRESULT PropertyStorage_ReadFromStream(PropertyStorage_impl *This)
{
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
    if (stat.cbSize.u.HighPart)
    {
        WARN("stream too big\n");
        /* maximum size varies, but it can't be this big */
        hr = STG_E_INVALIDHEADER;
        goto end;
    }
    if (stat.cbSize.u.LowPart == 0)
    {
        /* empty stream is okay */
        hr = S_OK;
        goto end;
    }
    else if (stat.cbSize.u.LowPart < sizeof(PROPERTYSETHEADER) +
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
    if (fmtOffset.dwOffset > stat.cbSize.u.LowPart)
    {
        WARN("invalid offset %d (stream length is %d)\n", fmtOffset.dwOffset,
         stat.cbSize.u.LowPart);
        hr = STG_E_INVALIDHEADER;
        goto end;
    }
    /* wackiness alert: if the format ID is FMTID_DocSummaryInformation, there
     * follow not one, but two sections.  The first is the standard properties
     * for the document summary information, and the second is user-defined
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
        WARN("section header too small, got %d\n", sectionHdr.cbSection);
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
    hr = IStream_Read(This->stm, buf, sectionHdr.cbSection -
     sizeof(PROPERTYSECTIONHEADER), &count);
    if (FAILED(hr))
        goto end;
    TRACE("Reading %d properties:\n", sectionHdr.cProperties);
    for (i = 0; SUCCEEDED(hr) && i < sectionHdr.cProperties; i++)
    {
        PROPERTYIDOFFSET *idOffset = (PROPERTYIDOFFSET *)(buf +
         i * sizeof(PROPERTYIDOFFSET));

        if (idOffset->dwOffset < sizeof(PROPERTYSECTIONHEADER) ||
         idOffset->dwOffset >= sectionHdr.cbSection - sizeof(DWORD))
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
                TRACE("Dictionary offset is %d\n", dictOffset);
            }
            else
            {
                PROPVARIANT prop;

                PropVariantInit(&prop);
                if (SUCCEEDED(PropertyStorage_ReadProperty(This, &prop,
                 buf + idOffset->dwOffset - sizeof(PROPERTYSECTIONHEADER))))
                {
                    TRACE("Read property with ID 0x%08x, type %d\n",
                     idOffset->propid, prop.vt);
                    switch(idOffset->propid)
                    {
                    case PID_CODEPAGE:
                        if (prop.vt == VT_I2)
                            This->codePage = (UINT)prop.u.iVal;
                        break;
                    case PID_LOCALE:
                        if (prop.vt == VT_I4)
                            This->locale = (LCID)prop.u.lVal;
                        break;
                    case PID_BEHAVIOR:
                        if (prop.vt == VT_I4 && prop.u.lVal)
                            This->grfFlags |= PROPSETFLAG_CASE_SENSITIVE;
                        /* The format should already be 1, but just in case */
                        This->format = 1;
                        break;
                    default:
                        hr = PropertyStorage_StorePropWithId(This,
                         idOffset->propid, &prop, This->codePage);
                    }
                }
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
    TRACE("Code page is %d, locale is %d\n", This->codePage, This->locale);
    if (dictOffset)
        hr = PropertyStorage_ReadDictionary(This,
         buf + dictOffset - sizeof(PROPERTYSECTIONHEADER));

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
    StorageUtl_WriteWord((BYTE *)&hdr->wByteOrder, 0,
     PROPSETHDR_BYTEORDER_MAGIC);
    StorageUtl_WriteWord((BYTE *)&hdr->wFormat, 0, This->format);
    StorageUtl_WriteDWord((BYTE *)&hdr->dwOSVer, 0, This->originatorOS);
    StorageUtl_WriteGUID((BYTE *)&hdr->clsid, 0, &This->clsid);
    StorageUtl_WriteDWord((BYTE *)&hdr->reserved, 0, 1);
}

static void PropertyStorage_MakeFmtIdOffset(PropertyStorage_impl *This,
 FORMATIDOFFSET *fmtOffset)
{
    assert(fmtOffset);
    StorageUtl_WriteGUID((BYTE *)fmtOffset, 0, &This->fmtid);
    StorageUtl_WriteDWord((BYTE *)fmtOffset, offsetof(FORMATIDOFFSET, dwOffset),
     sizeof(PROPERTYSETHEADER) + sizeof(FORMATIDOFFSET));
}

static void PropertyStorage_MakeSectionHdr(DWORD cbSection, DWORD numProps,
 PROPERTYSECTIONHEADER *hdr)
{
    assert(hdr);
    StorageUtl_WriteDWord((BYTE *)hdr, 0, cbSection);
    StorageUtl_WriteDWord((BYTE *)hdr,
     offsetof(PROPERTYSECTIONHEADER, cProperties), numProps);
}

static void PropertyStorage_MakePropertyIdOffset(DWORD propid, DWORD dwOffset,
 PROPERTYIDOFFSET *propIdOffset)
{
    assert(propIdOffset);
    StorageUtl_WriteDWord((BYTE *)propIdOffset, 0, propid);
    StorageUtl_WriteDWord((BYTE *)propIdOffset,
     offsetof(PROPERTYIDOFFSET, dwOffset), dwOffset);
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
    DWORD propid;
    ULONG count;

    assert(key);
    assert(closure);
    StorageUtl_WriteDWord((LPBYTE)&propid, 0, (DWORD)value);
    c->hr = IStream_Write(This->stm, &propid, sizeof(propid), &count);
    if (FAILED(c->hr))
        goto end;
    c->bytesWritten += sizeof(DWORD);
    if (This->codePage == CP_UNICODE)
    {
        DWORD keyLen, pad = 0;

        StorageUtl_WriteDWord((LPBYTE)&keyLen, 0,
         (lstrlenW((LPCWSTR)key) + 1) * sizeof(WCHAR));
        c->hr = IStream_Write(This->stm, &keyLen, sizeof(keyLen), &count);
        if (FAILED(c->hr))
            goto end;
        c->bytesWritten += sizeof(DWORD);
        /* Rather than allocate a copy, I'll swap the string to little-endian
         * in-place, write it, then swap it back.
         */
        PropertyStorage_ByteSwapString(key, keyLen);
        c->hr = IStream_Write(This->stm, key, keyLen, &count);
        PropertyStorage_ByteSwapString(key, keyLen);
        if (FAILED(c->hr))
            goto end;
        c->bytesWritten += keyLen;
        if (keyLen % sizeof(DWORD))
        {
            c->hr = IStream_Write(This->stm, &pad,
             sizeof(DWORD) - keyLen % sizeof(DWORD), &count);
            if (FAILED(c->hr))
                goto end;
            c->bytesWritten += sizeof(DWORD) - keyLen % sizeof(DWORD);
        }
    }
    else
    {
        DWORD keyLen;

        StorageUtl_WriteDWord((LPBYTE)&keyLen, 0, strlen((LPCSTR)key) + 1);
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
    StorageUtl_WriteDWord((LPBYTE)&dwTemp, 0,
     dictionary_num_entries(This->name_to_propid));
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
        TRACE("adding %d bytes of padding\n", padding);
        *sectionOffset += padding;
    }

end:
    return hr;
}

static HRESULT PropertyStorage_WritePropertyToStream(PropertyStorage_impl *This,
 DWORD propNum, DWORD propid, const PROPVARIANT *var, DWORD *sectionOffset)
{
    HRESULT hr;
    LARGE_INTEGER seek;
    PROPERTYIDOFFSET propIdOffset;
    ULONG count;
    DWORD dwType, bytesWritten;

    assert(var);
    assert(sectionOffset);

    TRACE("%p, %d, 0x%08x, (%d), (%d)\n", This, propNum, propid, var->vt,
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
    StorageUtl_WriteDWord((LPBYTE)&dwType, 0, var->vt);
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
        hr = IStream_Write(This->stm, &var->u.cVal, sizeof(var->u.cVal),
         &count);
        bytesWritten = count;
        break;
    case VT_I2:
    case VT_UI2:
    {
        WORD wTemp;

        StorageUtl_WriteWord((LPBYTE)&wTemp, 0, var->u.iVal);
        hr = IStream_Write(This->stm, &wTemp, sizeof(wTemp), &count);
        bytesWritten = count;
        break;
    }
    case VT_I4:
    case VT_UI4:
    {
        DWORD dwTemp;

        StorageUtl_WriteDWord((LPBYTE)&dwTemp, 0, var->u.lVal);
        hr = IStream_Write(This->stm, &dwTemp, sizeof(dwTemp), &count);
        bytesWritten = count;
        break;
    }
    case VT_LPSTR:
    {
        DWORD len, dwTemp;

        if (This->codePage == CP_UNICODE)
            len = (lstrlenW(var->u.pwszVal) + 1) * sizeof(WCHAR);
        else
            len = lstrlenA(var->u.pszVal) + 1;
        StorageUtl_WriteDWord((LPBYTE)&dwTemp, 0, len);
        hr = IStream_Write(This->stm, &dwTemp, sizeof(dwTemp), &count);
        if (FAILED(hr))
            goto end;
        hr = IStream_Write(This->stm, var->u.pszVal, len, &count);
        bytesWritten = count + sizeof(DWORD);
        break;
    }
    case VT_LPWSTR:
    {
        DWORD len = lstrlenW(var->u.pwszVal) + 1, dwTemp;

        StorageUtl_WriteDWord((LPBYTE)&dwTemp, 0, len);
        hr = IStream_Write(This->stm, &dwTemp, sizeof(dwTemp), &count);
        if (FAILED(hr))
            goto end;
        hr = IStream_Write(This->stm, var->u.pwszVal, len * sizeof(WCHAR),
         &count);
        bytesWritten = count + sizeof(DWORD);
        break;
    }
    case VT_FILETIME:
    {
        FILETIME temp;

        StorageUtl_WriteULargeInteger((BYTE *)&temp, 0,
         (const ULARGE_INTEGER *)&var->u.filetime);
        hr = IStream_Write(This->stm, &temp, sizeof(FILETIME), &count);
        bytesWritten = count;
        break;
    }
    case VT_CF:
    {
        DWORD cf_hdr[2], len;

        len = var->u.pclipdata->cbSize;
        StorageUtl_WriteDWord((LPBYTE)&cf_hdr[0], 0, len + 8);
        StorageUtl_WriteDWord((LPBYTE)&cf_hdr[1], 0, var->u.pclipdata->ulClipFmt);
        hr = IStream_Write(This->stm, cf_hdr, sizeof(cf_hdr), &count);
        if (FAILED(hr))
            goto end;
        hr = IStream_Write(This->stm, &var->u.pclipdata->pClipData, len, &count);
        if (FAILED(hr))
            goto end;
        bytesWritten = count + sizeof cf_hdr;
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
            TRACE("adding %d bytes of padding\n", padding);
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
     (DWORD)key, value, c->sectionOffset);
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
    var.u.iVal = This->codePage;
    hr = PropertyStorage_WritePropertyToStream(This, prop++, PID_CODEPAGE,
     &var, &sectionOffset);
    if (FAILED(hr))
        goto end;

    if (This->locale != LOCALE_SYSTEM_DEFAULT)
    {
        var.vt = VT_I4;
        var.u.lVal = This->locale;
        hr = PropertyStorage_WritePropertyToStream(This, prop++, PID_LOCALE,
         &var, &sectionOffset);
        if (FAILED(hr))
            goto end;
    }

    if (This->grfFlags & PROPSETFLAG_CASE_SENSITIVE)
    {
        var.vt = VT_I4;
        var.u.lVal = 1;
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
    StorageUtl_WriteDWord((LPBYTE)&dwTemp, 0, sectionOffset);
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

    (*pps)->vtbl = &IPropertyStorage_Vtbl;
    (*pps)->ref = 1;
    InitializeCriticalSection(&(*pps)->cs);
    (*pps)->cs.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": PropertyStorage_impl.cs");
    (*pps)->stm = stm;
    (*pps)->fmtid = *rfmtid;
    (*pps)->grfMode = grfMode;

    hr = PropertyStorage_CreateDictionaries(*pps);
    if (FAILED(hr))
    {
        IStream_Release(stm);
        (*pps)->cs.DebugInfo->Spare[0] = 0;
        DeleteCriticalSection(&(*pps)->cs);
        HeapFree(GetProcessHeap(), 0, *pps);
        *pps = NULL;
    }

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
            *pps = (IPropertyStorage *)ps;
            TRACE("PropertyStorage %p constructed\n", ps);
            hr = S_OK;
        }
        else
        {
            PropertyStorage_DestroyDictionaries(ps);
            HeapFree(GetProcessHeap(), 0, ps);
        }
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
        TRACE("Code page is %d, locale is %d\n", ps->codePage, ps->locale);
        *pps = (IPropertyStorage *)ps;
        TRACE("PropertyStorage %p constructed\n", ps);
        hr = S_OK;
    }
    return hr;
}


/***********************************************************************
 * Implementation of IPropertySetStorage
 */

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
    return IStorage_QueryInterface( (IStorage*)This, riid, ppvObject );
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
    return IStorage_AddRef( (IStorage*)This );
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
    return IStorage_Release( (IStorage*)This );
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
    WCHAR name[CCH_MAX_PROPSTG_NAME];
    IStream *stm = NULL;
    HRESULT r;

    TRACE("%p %s %08x %08x %p\n", This, debugstr_guid(rfmtid), grfFlags,
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

    r = IStorage_CreateStream( (IStorage*)This, name, grfMode, 0, 0, &stm );
    if (FAILED(r))
        goto end;

    r = PropertyStorage_ConstructEmpty(stm, rfmtid, grfFlags, grfMode, ppprstg);

end:
    TRACE("returning 0x%08x\n", r);
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
    WCHAR name[CCH_MAX_PROPSTG_NAME];
    HRESULT r;

    TRACE("%p %s %08x %p\n", This, debugstr_guid(rfmtid), grfMode, ppprstg);

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

    r = IStorage_OpenStream((IStorage*) This, name, 0, grfMode, 0, &stm );
    if (FAILED(r))
        goto end;

    r = PropertyStorage_ConstructFromStream(stm, rfmtid, grfMode, ppprstg);

end:
    TRACE("returning 0x%08x\n", r);
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
    IStorage *stg = NULL;
    WCHAR name[CCH_MAX_PROPSTG_NAME];
    HRESULT r;

    TRACE("%p %s\n", This, debugstr_guid(rfmtid));

    if (!rfmtid)
        return E_INVALIDARG;

    r = FmtIdToPropStgName(rfmtid, name);
    if (FAILED(r))
        return r;

    stg = (IStorage*) This;
    return IStorage_DestroyElement(stg, name);
}

/************************************************************************
 * IPropertySetStorage_fnEnum (IPropertySetStorage)
 */
static HRESULT WINAPI IPropertySetStorage_fnEnum(
    IPropertySetStorage *ppstg,
    IEnumSTATPROPSETSTG** ppenum)
{
    StorageImpl *This = impl_from_IPropertySetStorage(ppstg);
    return create_EnumSTATPROPSETSTG(This, ppenum);
}

/************************************************************************
 * Implement IEnumSTATPROPSETSTG using enumx
 */
static HRESULT WINAPI IEnumSTATPROPSETSTG_fnQueryInterface(
    IEnumSTATPROPSETSTG *iface,
    REFIID riid,
    void** ppvObject)
{
    return enumx_QueryInterface((enumx_impl*)iface, riid, ppvObject);
}

static ULONG WINAPI IEnumSTATPROPSETSTG_fnAddRef(
    IEnumSTATPROPSETSTG *iface)
{
    return enumx_AddRef((enumx_impl*)iface);
}

static ULONG WINAPI IEnumSTATPROPSETSTG_fnRelease(
    IEnumSTATPROPSETSTG *iface)
{
    return enumx_Release((enumx_impl*)iface);
}

static HRESULT WINAPI IEnumSTATPROPSETSTG_fnNext(
    IEnumSTATPROPSETSTG *iface,
    ULONG celt,
    STATPROPSETSTG *rgelt,
    ULONG *pceltFetched)
{
    return enumx_Next((enumx_impl*)iface, celt, rgelt, pceltFetched);
}

static HRESULT WINAPI IEnumSTATPROPSETSTG_fnSkip(
    IEnumSTATPROPSETSTG *iface,
    ULONG celt)
{
    return enumx_Skip((enumx_impl*)iface, celt);
}

static HRESULT WINAPI IEnumSTATPROPSETSTG_fnReset(
    IEnumSTATPROPSETSTG *iface)
{
    return enumx_Reset((enumx_impl*)iface);
}

static HRESULT WINAPI IEnumSTATPROPSETSTG_fnClone(
    IEnumSTATPROPSETSTG *iface,
    IEnumSTATPROPSETSTG **ppenum)
{
    return enumx_Clone((enumx_impl*)iface, (enumx_impl**)ppenum);
}

static HRESULT create_EnumSTATPROPSETSTG(
    StorageImpl *This,
    IEnumSTATPROPSETSTG** ppenum)
{
    IStorage *stg = (IStorage*) &This->base.lpVtbl;
    IEnumSTATSTG *penum = NULL;
    STATSTG stat;
    ULONG count;
    HRESULT r;
    STATPROPSETSTG statpss;
    enumx_impl *enumx;

    TRACE("%p %p\n", This, ppenum);

    enumx = enumx_allocate(&IID_IEnumSTATPROPSETSTG,
                           &IEnumSTATPROPSETSTG_Vtbl,
                           sizeof (STATPROPSETSTG));

    /* add all the property set elements into a list */
    r = IStorage_EnumElements(stg, 0, NULL, 0, &penum);
    if (FAILED(r))
        return E_OUTOFMEMORY;

    while (1)
    {
        count = 0;
        r = IEnumSTATSTG_Next(penum, 1, &stat, &count);
        if (FAILED(r))
            break;
        if (!count)
            break;
        if (!stat.pwcsName)
            continue;
        if (stat.pwcsName[0] == 5 && stat.type == STGTY_STREAM)
        {
            PropStgNameToFmtId(stat.pwcsName, &statpss.fmtid);
            TRACE("adding %s (%s)\n", debugstr_w(stat.pwcsName),
                  debugstr_guid(&statpss.fmtid));
            statpss.mtime = stat.mtime;
            statpss.atime = stat.atime;
            statpss.ctime = stat.ctime;
            statpss.grfFlags = stat.grfMode;
            statpss.clsid = stat.clsid;
            enumx_add_element(enumx, &statpss);
        }
        CoTaskMemFree(stat.pwcsName);
    }
    IEnumSTATSTG_Release(penum);

    *ppenum = (IEnumSTATPROPSETSTG*) enumx;

    return S_OK;
}

/************************************************************************
 * Implement IEnumSTATPROPSTG using enumx
 */
static HRESULT WINAPI IEnumSTATPROPSTG_fnQueryInterface(
    IEnumSTATPROPSTG *iface,
    REFIID riid,
    void** ppvObject)
{
    return enumx_QueryInterface((enumx_impl*)iface, riid, ppvObject);
}

static ULONG WINAPI IEnumSTATPROPSTG_fnAddRef(
    IEnumSTATPROPSTG *iface)
{
    return enumx_AddRef((enumx_impl*)iface);
}

static ULONG WINAPI IEnumSTATPROPSTG_fnRelease(
    IEnumSTATPROPSTG *iface)
{
    return enumx_Release((enumx_impl*)iface);
}

static HRESULT WINAPI IEnumSTATPROPSTG_fnNext(
    IEnumSTATPROPSTG *iface,
    ULONG celt,
    STATPROPSTG *rgelt,
    ULONG *pceltFetched)
{
    return enumx_Next((enumx_impl*)iface, celt, rgelt, pceltFetched);
}

static HRESULT WINAPI IEnumSTATPROPSTG_fnSkip(
    IEnumSTATPROPSTG *iface,
    ULONG celt)
{
    return enumx_Skip((enumx_impl*)iface, celt);
}

static HRESULT WINAPI IEnumSTATPROPSTG_fnReset(
    IEnumSTATPROPSTG *iface)
{
    return enumx_Reset((enumx_impl*)iface);
}

static HRESULT WINAPI IEnumSTATPROPSTG_fnClone(
    IEnumSTATPROPSTG *iface,
    IEnumSTATPROPSTG **ppenum)
{
    return enumx_Clone((enumx_impl*)iface, (enumx_impl**)ppenum);
}

static BOOL prop_enum_stat(const void *k, const void *v, void *extra, void *arg)
{
    enumx_impl *enumx = arg;
    PROPID propid = (PROPID) k;
    const PROPVARIANT *prop = v;
    STATPROPSTG stat;

    stat.lpwstrName = NULL;
    stat.propid = propid;
    stat.vt = prop->vt;

    enumx_add_element(enumx, &stat);

    return TRUE;
}

static HRESULT create_EnumSTATPROPSTG(
    PropertyStorage_impl *This,
    IEnumSTATPROPSTG** ppenum)
{
    enumx_impl *enumx;

    TRACE("%p %p\n", This, ppenum);

    enumx = enumx_allocate(&IID_IEnumSTATPROPSTG,
                           &IEnumSTATPROPSTG_Vtbl,
                           sizeof (STATPROPSTG));

    dictionary_enumerate(This->propid_to_prop, prop_enum_stat, enumx);

    *ppenum = (IEnumSTATPROPSTG*) enumx;

    return S_OK;
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

static const IEnumSTATPROPSETSTGVtbl IEnumSTATPROPSETSTG_Vtbl =
{
    IEnumSTATPROPSETSTG_fnQueryInterface,
    IEnumSTATPROPSETSTG_fnAddRef,
    IEnumSTATPROPSETSTG_fnRelease,
    IEnumSTATPROPSETSTG_fnNext,
    IEnumSTATPROPSETSTG_fnSkip,
    IEnumSTATPROPSETSTG_fnReset,
    IEnumSTATPROPSETSTG_fnClone,
};

static const IEnumSTATPROPSTGVtbl IEnumSTATPROPSTG_Vtbl =
{
    IEnumSTATPROPSTG_fnQueryInterface,
    IEnumSTATPROPSTG_fnAddRef,
    IEnumSTATPROPSTG_fnRelease,
    IEnumSTATPROPSTG_fnNext,
    IEnumSTATPROPSTG_fnSkip,
    IEnumSTATPROPSTG_fnReset,
    IEnumSTATPROPSTG_fnClone,
};

/***********************************************************************
 * Format ID <-> name conversion
 */
static const WCHAR szSummaryInfo[] = { 5,'S','u','m','m','a','r','y',
 'I','n','f','o','r','m','a','t','i','o','n',0 };
static const WCHAR szDocSummaryInfo[] = { 5,'D','o','c','u','m','e','n','t',
 'S','u','m','m','a','r','y','I','n','f','o','r','m','a','t','i','o','n',0 };

#define BITS_PER_BYTE    8
#define CHARMASK         0x1f
#define BITS_IN_CHARMASK 5
#define NUM_ALPHA_CHARS  26

/***********************************************************************
 * FmtIdToPropStgName					[ole32.@]
 * Returns the storage name of the format ID rfmtid.
 * PARAMS
 *  rfmtid [I] Format ID for which to return a storage name
 *  str    [O] Storage name associated with rfmtid.
 *
 * RETURNS
 *  E_INVALIDARG if rfmtid or str i NULL, S_OK otherwise.
 *
 * NOTES
 * str must be at least CCH_MAX_PROPSTG_NAME characters in length.
 */
HRESULT WINAPI FmtIdToPropStgName(const FMTID *rfmtid, LPOLESTR str)
{
    static const char fmtMap[] = "abcdefghijklmnopqrstuvwxyz012345";

    TRACE("%s, %p\n", debugstr_guid(rfmtid), str);

    if (!rfmtid) return E_INVALIDARG;
    if (!str) return E_INVALIDARG;

    if (IsEqualGUID(&FMTID_SummaryInformation, rfmtid))
        lstrcpyW(str, szSummaryInfo);
    else if (IsEqualGUID(&FMTID_DocSummaryInformation, rfmtid))
        lstrcpyW(str, szDocSummaryInfo);
    else if (IsEqualGUID(&FMTID_UserDefinedProperties, rfmtid))
        lstrcpyW(str, szDocSummaryInfo);
    else
    {
        const BYTE *fmtptr;
        WCHAR *pstr = str;
        ULONG bitsRemaining = BITS_PER_BYTE;

        *pstr++ = 5;
        for (fmtptr = (const BYTE *)rfmtid; fmtptr < (const BYTE *)rfmtid + sizeof(FMTID); )
        {
            ULONG i = *fmtptr >> (BITS_PER_BYTE - bitsRemaining);

            if (bitsRemaining >= BITS_IN_CHARMASK)
            {
                *pstr = (WCHAR)(fmtMap[i & CHARMASK]);
                if (bitsRemaining == BITS_PER_BYTE && *pstr >= 'a' &&
                 *pstr <= 'z')
                    *pstr += 'A' - 'a';
                pstr++;
                bitsRemaining -= BITS_IN_CHARMASK;
                if (bitsRemaining == 0)
                {
                    fmtptr++;
                    bitsRemaining = BITS_PER_BYTE;
                }
            }
            else
            {
                if (++fmtptr < (const BYTE *)rfmtid + sizeof(FMTID))
                    i |= *fmtptr << bitsRemaining;
                *pstr++ = (WCHAR)(fmtMap[i & CHARMASK]);
                bitsRemaining += BITS_PER_BYTE - BITS_IN_CHARMASK;
            }
        }
        *pstr = 0;
    }
    TRACE("returning %s\n", debugstr_w(str));
    return S_OK;
}

/***********************************************************************
 * PropStgNameToFmtId					[ole32.@]
 * Returns the format ID corresponding to the given name.
 * PARAMS
 *  str    [I] Storage name to convert to a format ID.
 *  rfmtid [O] Format ID corresponding to str.
 *
 * RETURNS
 *  E_INVALIDARG if rfmtid or str is NULL or if str can't be converted to
 *  a format ID, S_OK otherwise.
 */
HRESULT WINAPI PropStgNameToFmtId(const LPOLESTR str, FMTID *rfmtid)
{
    HRESULT hr = STG_E_INVALIDNAME;

    TRACE("%s, %p\n", debugstr_w(str), rfmtid);

    if (!rfmtid) return E_INVALIDARG;
    if (!str) return STG_E_INVALIDNAME;

    if (!lstrcmpiW(str, szDocSummaryInfo))
    {
        *rfmtid = FMTID_DocSummaryInformation;
        hr = S_OK;
    }
    else if (!lstrcmpiW(str, szSummaryInfo))
    {
        *rfmtid = FMTID_SummaryInformation;
        hr = S_OK;
    }
    else
    {
        ULONG bits;
        BYTE *fmtptr = (BYTE *)rfmtid - 1;
        const WCHAR *pstr = str;

        memset(rfmtid, 0, sizeof(*rfmtid));
        for (bits = 0; bits < sizeof(FMTID) * BITS_PER_BYTE;
         bits += BITS_IN_CHARMASK)
        {
            ULONG bitsUsed = bits % BITS_PER_BYTE, bitsStored;
            WCHAR wc;

            if (bitsUsed == 0)
                fmtptr++;
            wc = *++pstr - 'A';
            if (wc > NUM_ALPHA_CHARS)
            {
                wc += 'A' - 'a';
                if (wc > NUM_ALPHA_CHARS)
                {
                    wc += 'a' - '0' + NUM_ALPHA_CHARS;
                    if (wc > CHARMASK)
                    {
                        WARN("invalid character (%d)\n", *pstr);
                        goto end;
                    }
                }
            }
            *fmtptr |= wc << bitsUsed;
            bitsStored = min(BITS_PER_BYTE - bitsUsed, BITS_IN_CHARMASK);
            if (bitsStored < BITS_IN_CHARMASK)
            {
                wc >>= BITS_PER_BYTE - bitsUsed;
                if (bits + bitsStored == sizeof(FMTID) * BITS_PER_BYTE)
                {
                    if (wc != 0)
                    {
                        WARN("extra bits\n");
                        goto end;
                    }
                    break;
                }
                fmtptr++;
                *fmtptr |= (BYTE)wc;
            }
        }
        hr = S_OK;
    }
end:
    return hr;
}
