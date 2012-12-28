/*
 * Copyright 2009 Vincent Povirk for CodeWeavers
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

#include "config.h"

#include <stdarg.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "objbase.h"
#include "wincodec.h"

#include "wincodecs_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(wincodecs);

typedef struct PropertyBag {
    IPropertyBag2 IPropertyBag2_iface;
    LONG ref;
} PropertyBag;

static inline PropertyBag *impl_from_IPropertyBag2(IPropertyBag2 *iface)
{
    return CONTAINING_RECORD(iface, PropertyBag, IPropertyBag2_iface);
}

static HRESULT WINAPI PropertyBag_QueryInterface(IPropertyBag2 *iface, REFIID iid,
    void **ppv)
{
    PropertyBag *This = impl_from_IPropertyBag2(iface);
    TRACE("(%p,%s,%p)\n", iface, debugstr_guid(iid), ppv);

    if (!ppv) return E_INVALIDARG;

    if (IsEqualIID(&IID_IUnknown, iid) ||
        IsEqualIID(&IID_IPropertyBag2, iid))
    {
        *ppv = &This->IPropertyBag2_iface;
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI PropertyBag_AddRef(IPropertyBag2 *iface)
{
    PropertyBag *This = impl_from_IPropertyBag2(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) refcount=%u\n", iface, ref);

    return ref;
}

static ULONG WINAPI PropertyBag_Release(IPropertyBag2 *iface)
{
    PropertyBag *This = impl_from_IPropertyBag2(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) refcount=%u\n", iface, ref);

    if (ref == 0)
    {
        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}

static HRESULT WINAPI PropertyBag_Read(IPropertyBag2 *iface, ULONG cProperties,
    PROPBAG2 *pPropBag, IErrorLog *pErrLog, VARIANT *pvarValue, HRESULT *phrError)
{
    FIXME("(%p,%u,%p,%p,%p,%p): stub\n", iface, cProperties, pPropBag, pErrLog, pvarValue, phrError);
    return E_NOTIMPL;
}

static HRESULT WINAPI PropertyBag_Write(IPropertyBag2 *iface, ULONG cProperties,
    PROPBAG2 *pPropBag, VARIANT *pvarValue)
{
    FIXME("(%p,%u,%p,%p): stub\n", iface, cProperties, pPropBag, pvarValue);
    return E_NOTIMPL;
}

static HRESULT WINAPI PropertyBag_CountProperties(IPropertyBag2 *iface, ULONG *pcProperties)
{
    FIXME("(%p,%p): stub\n", iface, pcProperties);
    return E_NOTIMPL;
}

static HRESULT WINAPI PropertyBag_GetPropertyInfo(IPropertyBag2 *iface, ULONG iProperty,
    ULONG cProperties, PROPBAG2 *pPropBag, ULONG *pcProperties)
{
    FIXME("(%p,%u,%u,%p,%p): stub\n", iface, iProperty, cProperties, pPropBag, pcProperties);
    return E_NOTIMPL;
}

static HRESULT WINAPI PropertyBag_LoadObject(IPropertyBag2 *iface, LPCOLESTR pstrName,
    DWORD dwHint, IUnknown *pUnkObject, IErrorLog *pErrLog)
{
    FIXME("(%p,%s,%u,%p,%p): stub\n", iface, debugstr_w(pstrName), dwHint, pUnkObject, pErrLog);
    return E_NOTIMPL;
}

static const IPropertyBag2Vtbl PropertyBag_Vtbl = {
    PropertyBag_QueryInterface,
    PropertyBag_AddRef,
    PropertyBag_Release,
    PropertyBag_Read,
    PropertyBag_Write,
    PropertyBag_CountProperties,
    PropertyBag_GetPropertyInfo,
    PropertyBag_LoadObject
};

HRESULT CreatePropertyBag2(IPropertyBag2 **ppPropertyBag2)
{
    PropertyBag *This;

    This = HeapAlloc(GetProcessHeap(), 0, sizeof(PropertyBag));
    if (!This) return E_OUTOFMEMORY;

    This->IPropertyBag2_iface.lpVtbl = &PropertyBag_Vtbl;
    This->ref = 1;

    *ppPropertyBag2 = &This->IPropertyBag2_iface;

    return S_OK;
}
