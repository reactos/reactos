/*
 * Copyright 2012 Jacek Caban for CodeWeavers
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

#include "mshtml_private.h"

typedef struct {
    DispatchEx dispex;
    IHTMLStorage IHTMLStorage_iface;
    LONG ref;
} HTMLStorage;

static inline HTMLStorage *impl_from_IHTMLStorage(IHTMLStorage *iface)
{
    return CONTAINING_RECORD(iface, HTMLStorage, IHTMLStorage_iface);
}

static HRESULT WINAPI HTMLStorage_QueryInterface(IHTMLStorage *iface, REFIID riid, void **ppv)
{
    HTMLStorage *This = impl_from_IHTMLStorage(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_mshtml_guid(riid), ppv);

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        *ppv = &This->IHTMLStorage_iface;
    }else if(IsEqualGUID(&IID_IHTMLStorage, riid)) {
        *ppv = &This->IHTMLStorage_iface;
    }else if(dispex_query_interface(&This->dispex, riid, ppv)) {
        return *ppv ? S_OK : E_NOINTERFACE;
    }else {
        *ppv = NULL;
        WARN("(%p)->(%s %p)\n", This, debugstr_mshtml_guid(riid), ppv);
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI HTMLStorage_AddRef(IHTMLStorage *iface)
{
    HTMLStorage *This = impl_from_IHTMLStorage(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    return ref;
}

static ULONG WINAPI HTMLStorage_Release(IHTMLStorage *iface)
{
    HTMLStorage *This = impl_from_IHTMLStorage(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    if(!ref) {
        release_dispex(&This->dispex);
        heap_free(This);
    }

    return ref;
}

static HRESULT WINAPI HTMLStorage_GetTypeInfoCount(IHTMLStorage *iface, UINT *pctinfo)
{
    HTMLStorage *This = impl_from_IHTMLStorage(iface);
    return IDispatchEx_GetTypeInfoCount(&This->dispex.IDispatchEx_iface, pctinfo);
}

static HRESULT WINAPI HTMLStorage_GetTypeInfo(IHTMLStorage *iface, UINT iTInfo,
        LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLStorage *This = impl_from_IHTMLStorage(iface);

    return IDispatchEx_GetTypeInfo(&This->dispex.IDispatchEx_iface, iTInfo, lcid, ppTInfo);
}

static HRESULT WINAPI HTMLStorage_GetIDsOfNames(IHTMLStorage *iface, REFIID riid, LPOLESTR *rgszNames, UINT cNames,
        LCID lcid, DISPID *rgDispId)
{
    HTMLStorage *This = impl_from_IHTMLStorage(iface);

    return IDispatchEx_GetIDsOfNames(&This->dispex.IDispatchEx_iface, riid, rgszNames, cNames,
            lcid, rgDispId);
}

static HRESULT WINAPI HTMLStorage_Invoke(IHTMLStorage *iface, DISPID dispIdMember, REFIID riid, LCID lcid,
        WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLStorage *This = impl_from_IHTMLStorage(iface);

    return IDispatchEx_Invoke(&This->dispex.IDispatchEx_iface, dispIdMember, riid, lcid, wFlags,
            pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI HTMLStorage_get_length(IHTMLStorage *iface, LONG *p)
{
    HTMLStorage *This = impl_from_IHTMLStorage(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStorage_get_remainingSpace(IHTMLStorage *iface, LONG *p)
{
    HTMLStorage *This = impl_from_IHTMLStorage(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStorage_key(IHTMLStorage *iface, LONG lIndex, BSTR *p)
{
    HTMLStorage *This = impl_from_IHTMLStorage(iface);
    FIXME("(%p)->(%d %p)\n", This, lIndex, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStorage_getItem(IHTMLStorage *iface, BSTR bstrKey, VARIANT *p)
{
    HTMLStorage *This = impl_from_IHTMLStorage(iface);
    FIXME("(%p)->(%s %p)\n", This, debugstr_w(bstrKey), p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStorage_setItem(IHTMLStorage *iface, BSTR bstrKey, BSTR bstrValue)
{
    HTMLStorage *This = impl_from_IHTMLStorage(iface);
    FIXME("(%p)->(%s %s)\n", This, debugstr_w(bstrKey), debugstr_w(bstrValue));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStorage_removeItem(IHTMLStorage *iface, BSTR bstrKey)
{
    HTMLStorage *This = impl_from_IHTMLStorage(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(bstrKey));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStorage_clear(IHTMLStorage *iface)
{
    HTMLStorage *This = impl_from_IHTMLStorage(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static const IHTMLStorageVtbl HTMLStorageVtbl = {
    HTMLStorage_QueryInterface,
    HTMLStorage_AddRef,
    HTMLStorage_Release,
    HTMLStorage_GetTypeInfoCount,
    HTMLStorage_GetTypeInfo,
    HTMLStorage_GetIDsOfNames,
    HTMLStorage_Invoke,
    HTMLStorage_get_length,
    HTMLStorage_get_remainingSpace,
    HTMLStorage_key,
    HTMLStorage_getItem,
    HTMLStorage_setItem,
    HTMLStorage_removeItem,
    HTMLStorage_clear
};

static const tid_t HTMLStorage_iface_tids[] = {
    IHTMLStorage_tid,
    0
};
static dispex_static_data_t HTMLStorage_dispex = {
    NULL,
    IHTMLStorage_tid,
    NULL,
    HTMLStorage_iface_tids
};

HRESULT create_storage(IHTMLStorage **p)
{
    HTMLStorage *storage;

    storage = heap_alloc_zero(sizeof(*storage));
    if(!storage)
        return E_OUTOFMEMORY;

    storage->IHTMLStorage_iface.lpVtbl = &HTMLStorageVtbl;
    storage->ref = 1;
    init_dispex(&storage->dispex, (IUnknown*)&storage->IHTMLStorage_iface, &HTMLStorage_dispex);

    *p = &storage->IHTMLStorage_iface;
    return S_OK;
}
