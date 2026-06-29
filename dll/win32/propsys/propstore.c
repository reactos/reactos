/*
 * standard IPropertyStore implementation
 *
 * Copyright 2012 Vincent Povirk for CodeWeavers
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

#define COBJMACROS

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "objbase.h"
#include "rpcproxy.h"
#include "propsys.h"
#include "wine/debug.h"
#include "wine/list.h"

#include "initguid.h"
#include "propsys_private.h"

DEFINE_GUID(FMTID_NamedProperties, 0xd5cdd505, 0x2e9c, 0x101b, 0x93, 0x97, 0x08, 0x00, 0x2b, 0x2c, 0xf9, 0xae);

WINE_DEFAULT_DEBUG_CHANNEL(propsys);

typedef struct {
    struct list entry;
    DWORD pid;
    PROPVARIANT propvar;
    PSC_STATE state;
} propstore_value;

typedef struct {
    struct list entry;
    GUID fmtid;
    struct list values; /* list of struct propstore_value */
    DWORD count;
} propstore_format;

typedef struct {
    IPropertyStoreCache IPropertyStoreCache_iface;
    LONG ref;
    CRITICAL_SECTION lock;
    struct list formats; /* list of struct propstore_format */
} PropertyStore;

static inline PropertyStore *impl_from_IPropertyStoreCache(IPropertyStoreCache *iface)
{
    return CONTAINING_RECORD(iface, PropertyStore, IPropertyStoreCache_iface);
}

static HRESULT WINAPI PropertyStore_QueryInterface(IPropertyStoreCache *iface, REFIID iid,
    void **ppv)
{
    PropertyStore *This = impl_from_IPropertyStoreCache(iface);
    TRACE("(%p,%s,%p)\n", iface, debugstr_guid(iid), ppv);

    if (!ppv) return E_INVALIDARG;

    if (IsEqualIID(&IID_IUnknown, iid) || IsEqualIID(&IID_IPropertyStore, iid) ||
        IsEqualIID(&IID_IPropertyStoreCache, iid))
    {
        *ppv = &This->IPropertyStoreCache_iface;
    }
    else
    {
        FIXME("No interface for %s\n", debugstr_guid(iid));
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI PropertyStore_AddRef(IPropertyStoreCache *iface)
{
    PropertyStore *This = impl_from_IPropertyStoreCache(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) refcount=%lu\n", iface, ref);

    return ref;
}

static void destroy_format(propstore_format *format)
{
    propstore_value *cursor, *cursor2;
    LIST_FOR_EACH_ENTRY_SAFE(cursor, cursor2, &format->values, propstore_value, entry)
    {
        PropVariantClear(&cursor->propvar);
        HeapFree(GetProcessHeap(), 0, cursor);
    }
    HeapFree(GetProcessHeap(), 0, format);
}

static ULONG WINAPI PropertyStore_Release(IPropertyStoreCache *iface)
{
    PropertyStore *This = impl_from_IPropertyStoreCache(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) refcount=%lu\n", iface, ref);

    if (ref == 0)
    {
        propstore_format *cursor, *cursor2;
        This->lock.DebugInfo->Spare[0] = 0;
        DeleteCriticalSection(&This->lock);
        LIST_FOR_EACH_ENTRY_SAFE(cursor, cursor2, &This->formats, propstore_format, entry)
            destroy_format(cursor);
        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}

static HRESULT WINAPI PropertyStore_GetCount(IPropertyStoreCache *iface,
    DWORD *cProps)
{
    PropertyStore *This = impl_from_IPropertyStoreCache(iface);
    propstore_format *format;

    TRACE("%p,%p\n", iface, cProps);

    if (!cProps)
        return E_POINTER;

    *cProps = 0;

    EnterCriticalSection(&This->lock);

    LIST_FOR_EACH_ENTRY(format, &This->formats, propstore_format, entry)
        *cProps += format->count;

    LeaveCriticalSection(&This->lock);

    return S_OK;
}

static HRESULT WINAPI PropertyStore_GetAt(IPropertyStoreCache *iface,
    DWORD iProp, PROPERTYKEY *pkey)
{
    PropertyStore *This = impl_from_IPropertyStoreCache(iface);
    propstore_format *format=NULL, *format_candidate;
    propstore_value *value;
    HRESULT hr;

    TRACE("%p,%ld,%p\n", iface, iProp, pkey);

    if (!pkey)
        return E_POINTER;

    EnterCriticalSection(&This->lock);

    LIST_FOR_EACH_ENTRY(format_candidate, &This->formats, propstore_format, entry)
    {
        if (format_candidate->count > iProp)
        {
            format = format_candidate;
            pkey->fmtid = format->fmtid;
            break;
        }

        iProp -= format_candidate->count;
    }

    if (format)
    {
        LIST_FOR_EACH_ENTRY(value, &format->values, propstore_value, entry)
        {
            if (iProp == 0)
            {
                pkey->pid = value->pid;
                break;
            }

            iProp--;
        }

        hr = S_OK;
    }
    else
        hr = E_INVALIDARG;

    LeaveCriticalSection(&This->lock);

    return hr;
}

static HRESULT PropertyStore_LookupValue(PropertyStore *This, REFPROPERTYKEY key,
                                         BOOL insert, propstore_value **result)
{
    propstore_format *format=NULL, *format_candidate;
    propstore_value *value=NULL, *value_candidate;

    if (IsEqualGUID(&key->fmtid, &FMTID_NamedProperties))
    {
        /* This is used in the property store format [MS-PROPSTORE]
         * for named values and probably gets special treatment. */
        ERR("don't know how to handle FMTID_NamedProperties\n");
        return E_FAIL;
    }

    LIST_FOR_EACH_ENTRY(format_candidate, &This->formats, propstore_format, entry)
    {
        if (IsEqualGUID(&format_candidate->fmtid, &key->fmtid))
        {
            format = format_candidate;
            break;
        }
    }

    if (!format)
    {
        if (!insert)
            return TYPE_E_ELEMENTNOTFOUND;

        format = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*format));
        if (!format)
            return E_OUTOFMEMORY;

        format->fmtid = key->fmtid;
        list_init(&format->values);
        list_add_tail(&This->formats, &format->entry);
    }

    LIST_FOR_EACH_ENTRY(value_candidate, &format->values, propstore_value, entry)
    {
        if (value_candidate->pid == key->pid)
        {
            value = value_candidate;
            break;
        }
    }

    if (!value)
    {
        if (!insert)
            return TYPE_E_ELEMENTNOTFOUND;

        value = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*value));
        if (!value)
            return E_OUTOFMEMORY;

        value->pid = key->pid;
        list_add_tail(&format->values, &value->entry);
        format->count++;
    }

    *result = value;

    return S_OK;
}

static HRESULT WINAPI PropertyStore_GetValue(IPropertyStoreCache *iface,
    REFPROPERTYKEY key, PROPVARIANT *pv)
{
    PropertyStore *This = impl_from_IPropertyStoreCache(iface);
    propstore_value *value;
    HRESULT hr;

    TRACE("%p,%p,%p\n", iface, key, pv);

    if (!pv)
        return E_POINTER;

    EnterCriticalSection(&This->lock);

    hr = PropertyStore_LookupValue(This, key, FALSE, &value);

    if (SUCCEEDED(hr))
        hr = PropVariantCopy(pv, &value->propvar);
    else if (hr == TYPE_E_ELEMENTNOTFOUND)
    {
        PropVariantInit(pv);
        hr = S_OK;
    }

    LeaveCriticalSection(&This->lock);

    return hr;
}

static HRESULT WINAPI PropertyStore_SetValue(IPropertyStoreCache *iface,
    REFPROPERTYKEY key, REFPROPVARIANT propvar)
{
    PropertyStore *This = impl_from_IPropertyStoreCache(iface);
    propstore_value *value;
    HRESULT hr;
    PROPVARIANT temp;

    TRACE("%p,%p,%p\n", iface, key, propvar);

    EnterCriticalSection(&This->lock);

    hr = PropertyStore_LookupValue(This, key, TRUE, &value);

    if (SUCCEEDED(hr))
        hr = PropVariantCopy(&temp, propvar);

    if (SUCCEEDED(hr))
    {
        PropVariantClear(&value->propvar);
        value->propvar = temp;
    }

    LeaveCriticalSection(&This->lock);

    return hr;
}

static HRESULT WINAPI PropertyStore_Commit(IPropertyStoreCache *iface)
{
    FIXME("%p: stub\n", iface);
    return S_OK;
}

static HRESULT WINAPI PropertyStore_GetState(IPropertyStoreCache *iface,
    REFPROPERTYKEY key, PSC_STATE *pstate)
{
    PropertyStore *This = impl_from_IPropertyStoreCache(iface);
    propstore_value *value;
    HRESULT hr;

    TRACE("%p,%p,%p\n", iface, key, pstate);

    EnterCriticalSection(&This->lock);

    hr = PropertyStore_LookupValue(This, key, FALSE, &value);

    if (SUCCEEDED(hr))
        *pstate = value->state;

    LeaveCriticalSection(&This->lock);

    if (FAILED(hr))
        *pstate = PSC_NORMAL;

    return hr;
}

static HRESULT WINAPI PropertyStore_GetValueAndState(IPropertyStoreCache *iface,
    REFPROPERTYKEY key, PROPVARIANT *ppropvar, PSC_STATE *pstate)
{
    PropertyStore *This = impl_from_IPropertyStoreCache(iface);
    propstore_value *value;
    HRESULT hr;

    TRACE("%p,%p,%p,%p\n", iface, key, ppropvar, pstate);

    EnterCriticalSection(&This->lock);

    hr = PropertyStore_LookupValue(This, key, FALSE, &value);

    if (SUCCEEDED(hr))
        hr = PropVariantCopy(ppropvar, &value->propvar);

    if (SUCCEEDED(hr))
        *pstate = value->state;

    LeaveCriticalSection(&This->lock);

    if (FAILED(hr))
    {
        PropVariantInit(ppropvar);
        *pstate = PSC_NORMAL;
    }

    return hr;
}

static HRESULT WINAPI PropertyStore_SetState(IPropertyStoreCache *iface,
    REFPROPERTYKEY key, PSC_STATE pstate)
{
    PropertyStore *This = impl_from_IPropertyStoreCache(iface);
    propstore_value *value;
    HRESULT hr;

    TRACE("%p,%p,%d\n", iface, key, pstate);

    EnterCriticalSection(&This->lock);

    hr = PropertyStore_LookupValue(This, key, FALSE, &value);

    if (SUCCEEDED(hr))
        value->state = pstate;

    LeaveCriticalSection(&This->lock);

    return hr;
}

static HRESULT WINAPI PropertyStore_SetValueAndState(IPropertyStoreCache *iface,
    REFPROPERTYKEY key, const PROPVARIANT *ppropvar, PSC_STATE state)
{
    PropertyStore *This = impl_from_IPropertyStoreCache(iface);
    propstore_value *value;
    HRESULT hr;
    PROPVARIANT temp;

    TRACE("%p,%p,%p,%d\n", iface, key, ppropvar, state);

    EnterCriticalSection(&This->lock);

    hr = PropertyStore_LookupValue(This, key, TRUE, &value);

    if (SUCCEEDED(hr))
        hr = PropVariantCopy(&temp, ppropvar);

    if (SUCCEEDED(hr))
    {
        PropVariantClear(&value->propvar);
        value->propvar = temp;
        value->state = state;
    }

    LeaveCriticalSection(&This->lock);

    return hr;
}

static const IPropertyStoreCacheVtbl PropertyStore_Vtbl = {
    PropertyStore_QueryInterface,
    PropertyStore_AddRef,
    PropertyStore_Release,
    PropertyStore_GetCount,
    PropertyStore_GetAt,
    PropertyStore_GetValue,
    PropertyStore_SetValue,
    PropertyStore_Commit,
    PropertyStore_GetState,
    PropertyStore_GetValueAndState,
    PropertyStore_SetState,
    PropertyStore_SetValueAndState
};

HRESULT PropertyStore_CreateInstance(IUnknown *pUnkOuter, REFIID iid, void** ppv)
{
    PropertyStore *This;
    HRESULT ret;

    TRACE("(%p,%s,%p)\n", pUnkOuter, debugstr_guid(iid), ppv);

    *ppv = NULL;

    if (pUnkOuter) return CLASS_E_NOAGGREGATION;

    This = HeapAlloc(GetProcessHeap(), 0, sizeof(PropertyStore));
    if (!This) return E_OUTOFMEMORY;

    This->IPropertyStoreCache_iface.lpVtbl = &PropertyStore_Vtbl;
    This->ref = 1;
#ifdef __REACTOS__
    InitializeCriticalSection(&This->lock);
#else
    InitializeCriticalSectionEx(&This->lock, 0, RTL_CRITICAL_SECTION_FLAG_FORCE_DEBUG_INFO);
#endif
    This->lock.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": PropertyStore.lock");
    list_init(&This->formats);

    ret = IPropertyStoreCache_QueryInterface(&This->IPropertyStoreCache_iface, iid, ppv);
    IPropertyStoreCache_Release(&This->IPropertyStoreCache_iface);

    return ret;
}

HRESULT WINAPI PSCreatePropertyStoreFromObject(IUnknown *obj, DWORD access, REFIID riid, void **ret)
{
    HRESULT hr;

    TRACE("(%p, %ld, %s, %p)\n", obj, access, debugstr_guid(riid), ret);

    if (!obj || !ret)
        return E_POINTER;

    if (IsEqualIID(riid, &IID_IPropertyStore) && SUCCEEDED(hr = IUnknown_QueryInterface(obj, riid, ret)))
        return hr;

    FIXME("Unimplemented for %s.\n", debugstr_guid(riid));
    return E_NOTIMPL;
}
