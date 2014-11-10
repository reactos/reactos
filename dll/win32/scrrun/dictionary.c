/*
 * Copyright (C) 2012 Alistair Leslie-Hughes
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

#include "scrrun_private.h"

typedef struct
{
    IDictionary IDictionary_iface;

    LONG ref;
} dictionary;

static inline dictionary *impl_from_IDictionary(IDictionary *iface)
{
    return CONTAINING_RECORD(iface, dictionary, IDictionary_iface);
}

static HRESULT WINAPI dictionary_QueryInterface(IDictionary *iface, REFIID riid, void **obj)
{
    dictionary *This = impl_from_IDictionary(iface);
    TRACE("(%p)->(%s, %p)\n", This, debugstr_guid(riid), obj);

    *obj = NULL;

    if(IsEqualIID(riid, &IID_IUnknown) ||
       IsEqualIID(riid, &IID_IDispatch) ||
       IsEqualIID(riid, &IID_IDictionary))
    {
        *obj = &This->IDictionary_iface;
    }
    else if ( IsEqualGUID( riid, &IID_IDispatchEx ))
    {
        TRACE("Interface IDispatchEx not supported - returning NULL\n");
        *obj = NULL;
        return E_NOINTERFACE;
    }
    else if ( IsEqualGUID( riid, &IID_IObjectWithSite ))
    {
        TRACE("Interface IObjectWithSite not supported - returning NULL\n");
        *obj = NULL;
        return E_NOINTERFACE;
    }
    else
    {
        WARN("interface %s not implemented\n", debugstr_guid(riid));
        return E_NOINTERFACE;
    }

    IDictionary_AddRef(iface);
    return S_OK;
}

static ULONG WINAPI dictionary_AddRef(IDictionary *iface)
{
    dictionary *This = impl_from_IDictionary(iface);
    TRACE("(%p)\n", This);

    return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI dictionary_Release(IDictionary *iface)
{
    dictionary *This = impl_from_IDictionary(iface);
    LONG ref;

    TRACE("(%p)\n", This);

    ref = InterlockedDecrement(&This->ref);
    if(ref == 0)
        heap_free(This);

    return ref;
}

static HRESULT WINAPI dictionary_GetTypeInfoCount(IDictionary *iface, UINT *pctinfo)
{
    dictionary *This = impl_from_IDictionary(iface);

    TRACE("(%p)->()\n", This);

    *pctinfo = 1;
    return S_OK;
}

static HRESULT WINAPI dictionary_GetTypeInfo(IDictionary *iface, UINT iTInfo, LCID lcid, ITypeInfo **ppTInfo)
{
    dictionary *This = impl_from_IDictionary(iface);

    TRACE("(%p)->(%u %u %p)\n", This, iTInfo, lcid, ppTInfo);
    return get_typeinfo(IDictionary_tid, ppTInfo);
}

static HRESULT WINAPI dictionary_GetIDsOfNames(IDictionary *iface, REFIID riid, LPOLESTR *rgszNames,
                UINT cNames, LCID lcid, DISPID *rgDispId)
{
    dictionary *This = impl_from_IDictionary(iface);
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("(%p)->(%s %p %u %u %p)\n", This, debugstr_guid(riid), rgszNames, cNames, lcid, rgDispId);

    hr = get_typeinfo(IDictionary_tid, &typeinfo);
    if(SUCCEEDED(hr))
    {
        hr = ITypeInfo_GetIDsOfNames(typeinfo, rgszNames, cNames, rgDispId);
        ITypeInfo_Release(typeinfo);
    }

    return hr;
}

static HRESULT WINAPI dictionary_Invoke(IDictionary *iface, DISPID dispIdMember, REFIID riid,
                LCID lcid, WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult,
                EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    dictionary *This = impl_from_IDictionary(iface);
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("(%p)->(%d %s %d %d %p %p %p %p)\n", This, dispIdMember, debugstr_guid(riid),
           lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);

    hr = get_typeinfo(IDictionary_tid, &typeinfo);
    if(SUCCEEDED(hr))
    {
        hr = ITypeInfo_Invoke(typeinfo, &This->IDictionary_iface, dispIdMember, wFlags,
                pDispParams, pVarResult, pExcepInfo, puArgErr);
        ITypeInfo_Release(typeinfo);
    }

    return hr;
}

static HRESULT WINAPI dictionary_putref_Item(IDictionary *iface, VARIANT *Key, VARIANT *pRetItem)
{
    dictionary *This = impl_from_IDictionary(iface);

    FIXME("(%p)->(%p %p)\n", This, Key, pRetItem);

    return E_NOTIMPL;
}

static HRESULT WINAPI dictionary_put_Item(IDictionary *iface, VARIANT *Key, VARIANT *pRetItem)
{
    dictionary *This = impl_from_IDictionary(iface);

    FIXME("(%p)->(%p %p)\n", This, Key, pRetItem);

    return E_NOTIMPL;
}

static HRESULT WINAPI dictionary_get_Item(IDictionary *iface, VARIANT *Key, VARIANT *pRetItem)
{
    dictionary *This = impl_from_IDictionary(iface);

    FIXME("(%p)->(%p %p)\n", This, Key, pRetItem );

    return E_NOTIMPL;
}

static HRESULT WINAPI dictionary_Add(IDictionary *iface, VARIANT *Key, VARIANT *Item)
{
    dictionary *This = impl_from_IDictionary(iface);

    FIXME("(%p)->(%p %p)\n", This, Key, Item);

    return E_NOTIMPL;
}

static HRESULT WINAPI dictionary_get_Count(IDictionary *iface, LONG *pCount)
{
    dictionary *This = impl_from_IDictionary(iface);

    FIXME("(%p)->(%p)\n", This, pCount);

    *pCount = 0;

    return S_OK;
}

static HRESULT WINAPI dictionary_Exists(IDictionary *iface, VARIANT *Key, VARIANT_BOOL *pExists)
{
    dictionary *This = impl_from_IDictionary(iface);

    FIXME("(%p)->(%p %p)\n", This, Key, pExists);

    return E_NOTIMPL;
}

static HRESULT WINAPI dictionary_Items(IDictionary *iface, VARIANT *pItemsArray)
{
    dictionary *This = impl_from_IDictionary(iface);

    FIXME("(%p)->(%p)\n", This, pItemsArray);

    return E_NOTIMPL;
}

static HRESULT WINAPI dictionary_put_Key(IDictionary *iface, VARIANT *Key, VARIANT *rhs)
{
    dictionary *This = impl_from_IDictionary(iface);

    FIXME("(%p)->(%p %p)\n", This, Key, rhs);

    return E_NOTIMPL;
}

static HRESULT WINAPI dictionary_Keys(IDictionary *iface, VARIANT *pKeysArray)
{
    dictionary *This = impl_from_IDictionary(iface);

    FIXME("(%p)->(%p)\n", This, pKeysArray);

    return E_NOTIMPL;
}

static HRESULT WINAPI dictionary_Remove(IDictionary *iface, VARIANT *Key)
{
    dictionary *This = impl_from_IDictionary(iface);

    FIXME("(%p)->(%p)\n", This, Key);

    return E_NOTIMPL;
}

static HRESULT WINAPI dictionary_RemoveAll(IDictionary *iface)
{
    dictionary *This = impl_from_IDictionary(iface);

    FIXME("(%p)->()\n", This);

    return E_NOTIMPL;
}

static HRESULT WINAPI dictionary_put_CompareMode(IDictionary *iface, CompareMethod pcomp)
{
    dictionary *This = impl_from_IDictionary(iface);

    FIXME("(%p)->()\n", This);

    return E_NOTIMPL;
}

static HRESULT WINAPI dictionary_get_CompareMode(IDictionary *iface, CompareMethod *pcomp)
{
    dictionary *This = impl_from_IDictionary(iface);

    FIXME("(%p)->(%p)\n", This, pcomp);

    return E_NOTIMPL;
}

static HRESULT WINAPI dictionary__NewEnum(IDictionary *iface, IUnknown **ppunk)
{
    dictionary *This = impl_from_IDictionary(iface);

    FIXME("(%p)->(%p)\n", This, ppunk);

    return E_NOTIMPL;
}

static HRESULT WINAPI dictionary_get_HashVal(IDictionary *iface, VARIANT *Key, VARIANT *HashVal)
{
    dictionary *This = impl_from_IDictionary(iface);

    FIXME("(%p)->(%p %p)\n", This, Key, HashVal);

    return E_NOTIMPL;
}


static const struct IDictionaryVtbl dictionary_vtbl =
{
    dictionary_QueryInterface,
    dictionary_AddRef,
    dictionary_Release,
    dictionary_GetTypeInfoCount,
    dictionary_GetTypeInfo,
    dictionary_GetIDsOfNames,
    dictionary_Invoke,
    dictionary_putref_Item,
    dictionary_put_Item,
    dictionary_get_Item,
    dictionary_Add,
    dictionary_get_Count,
    dictionary_Exists,
    dictionary_Items,
    dictionary_put_Key,
    dictionary_Keys,
    dictionary_Remove,
    dictionary_RemoveAll,
    dictionary_put_CompareMode,
    dictionary_get_CompareMode,
    dictionary__NewEnum,
    dictionary_get_HashVal
};

HRESULT WINAPI Dictionary_CreateInstance(IClassFactory *factory,IUnknown *outer,REFIID riid, void **obj)
{
    dictionary *This;

    TRACE("(%p)\n", obj);

    *obj = NULL;

    This = heap_alloc(sizeof(*This));
    if(!This) return E_OUTOFMEMORY;

    This->IDictionary_iface.lpVtbl = &dictionary_vtbl;
    This->ref = 1;

    *obj = &This->IDictionary_iface;

    return S_OK;
}
