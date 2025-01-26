/*
 * Copyright 2009 Vincent Povirk for CodeWeavers
 * Copyright 2013 Ludger Sprenker
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

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "objbase.h"

#include "wincodecs_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(wincodecs);

typedef struct PropertyBag {
    IPropertyBag2 IPropertyBag2_iface;
    LONG ref;
    UINT prop_count;
    PROPBAG2 *properties;
    VARIANT *values;
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

    TRACE("(%p) refcount=%lu\n", iface, ref);

    return ref;
}

static ULONG WINAPI PropertyBag_Release(IPropertyBag2 *iface)
{
    PropertyBag *This = impl_from_IPropertyBag2(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) refcount=%lu\n", iface, ref);

    if (ref == 0)
    {
        ULONG i;
        if (This->properties && This->values)
        {
            for (i=0; i < This->prop_count; i++)
            {
                CoTaskMemFree(This->properties[i].pstrName);
                VariantClear( This->values+i );
            }
        }

        free(This->properties);
        free(This->values);
        free(This);
    }

    return ref;
}

static LONG find_item(PropertyBag *This, LPCOLESTR name)
{
    LONG i;
    if (!This->properties)
        return -1;
    if (!name)
        return -1;

    for (i=0; i < This->prop_count; i++)
    {
        if (wcscmp(name, This->properties[i].pstrName) == 0)
            return i;
    }

    return -1;
}

static HRESULT WINAPI PropertyBag_Read(IPropertyBag2 *iface, ULONG cProperties,
    PROPBAG2 *pPropBag, IErrorLog *pErrLog, VARIANT *pvarValue, HRESULT *phrError)
{
    HRESULT res = S_OK;
    ULONG i;
    PropertyBag *This = impl_from_IPropertyBag2(iface);

    TRACE("(%p,%lu,%p,%p,%p,%p)\n", iface, cProperties, pPropBag, pErrLog, pvarValue, phrError);

    for (i=0; i < cProperties; i++)
    {
        LONG idx;
        if (pPropBag[i].dwHint && pPropBag[i].dwHint <= This->prop_count)
            idx = pPropBag[i].dwHint-1;
        else
            idx = find_item(This, pPropBag[i].pstrName);

        if (idx > -1)
        {
            VariantInit(pvarValue+i);
            res = VariantCopy(pvarValue+i, This->values+idx);
            if (FAILED(res))
                break;
            phrError[i] = res;
        }
        else
        {
            res = E_FAIL;
            break;
        }
    }

    return res;
}

static HRESULT WINAPI PropertyBag_Write(IPropertyBag2 *iface, ULONG cProperties,
    PROPBAG2 *pPropBag, VARIANT *pvarValue)
{
    HRESULT res = S_OK;
    ULONG i;
    PropertyBag *This = impl_from_IPropertyBag2(iface);

    TRACE("(%p,%lu,%p,%p)\n", iface, cProperties, pPropBag, pvarValue);

    for (i=0; i < cProperties; i++)
    {
        LONG idx;
        if (pPropBag[i].dwHint && pPropBag[i].dwHint <= This->prop_count)
            idx = pPropBag[i].dwHint-1;
        else
            idx = find_item(This, pPropBag[i].pstrName);

        if (idx > -1)
        {
            if (This->properties[idx].vt != V_VT(pvarValue+i))
                return WINCODEC_ERR_PROPERTYUNEXPECTEDTYPE;
            res = VariantCopy(This->values+idx, pvarValue+i);
            if (FAILED(res))
                return E_FAIL;
        }
        else
        {
            if (pPropBag[i].pstrName)
                FIXME("Application tried to set the unknown option %s.\n",
                      debugstr_w(pPropBag[i].pstrName));

            /* FIXME: Function is not atomar on error, but MSDN does not say anything about it
             *        (no reset of items between 0 and i-1) */
            return E_FAIL;
        }
    }

    return res;
}

static HRESULT WINAPI PropertyBag_CountProperties(IPropertyBag2 *iface, ULONG *pcProperties)
{
    PropertyBag *This = impl_from_IPropertyBag2(iface);

    TRACE("(%p,%p)\n", iface, pcProperties);

    if (!pcProperties)
        return E_INVALIDARG;

    *pcProperties = This->prop_count;

    return S_OK;
}

static HRESULT copy_propbag2(PROPBAG2 *dest, const PROPBAG2 *src)
{
    dest->cfType = src->cfType;
    dest->clsid = src->clsid;
    dest->dwHint = src->dwHint;
    dest->dwType = src->dwType;
    dest->vt = src->vt;
    dest->pstrName = CoTaskMemAlloc((lstrlenW(src->pstrName)+1) * sizeof(WCHAR));
    if(!dest->pstrName)
        return E_OUTOFMEMORY;

    lstrcpyW(dest->pstrName, src->pstrName);

    return S_OK;
}

static HRESULT WINAPI PropertyBag_GetPropertyInfo(IPropertyBag2 *iface, ULONG iProperty,
    ULONG cProperties, PROPBAG2 *pPropBag, ULONG *pcProperties)
{
    HRESULT res = S_OK;
    ULONG i;
    PropertyBag *This = impl_from_IPropertyBag2(iface);

    TRACE("(%p,%lu,%lu,%p,%p)\n", iface, iProperty, cProperties, pPropBag, pcProperties);

    if (iProperty >= This->prop_count && iProperty > 0)
        return WINCODEC_ERR_VALUEOUTOFRANGE;
    if (iProperty+cProperties > This->prop_count )
        return WINCODEC_ERR_VALUEOUTOFRANGE;

    *pcProperties = min(cProperties, This->prop_count-iProperty);

    for (i=0; i < *pcProperties; i++)
    {
        res = copy_propbag2(pPropBag+i, This->properties+iProperty+i);
        if (FAILED(res))
        {
            do {
                CoTaskMemFree( pPropBag[--i].pstrName );
            } while (i);
            break;
        }
    }

    return res;
}

static HRESULT WINAPI PropertyBag_LoadObject(IPropertyBag2 *iface, LPCOLESTR pstrName,
    DWORD dwHint, IUnknown *pUnkObject, IErrorLog *pErrLog)
{
    FIXME("(%p,%s,%lu,%p,%p): stub\n", iface, debugstr_w(pstrName), dwHint, pUnkObject, pErrLog);
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

HRESULT CreatePropertyBag2(const PROPBAG2 *options, UINT count,
                           IPropertyBag2 **ppPropertyBag2)
{
    UINT i;
    HRESULT res = S_OK;
    PropertyBag *This;

    This = malloc(sizeof(PropertyBag));
    if (!This) return E_OUTOFMEMORY;

    This->IPropertyBag2_iface.lpVtbl = &PropertyBag_Vtbl;
    This->ref = 1;
    This->prop_count = count;

    if (count == 0)
    {
        This->properties = NULL;
        This->values = NULL;
    }
    else
    {
        This->properties = calloc(count, sizeof(PROPBAG2));
        This->values = calloc(count, sizeof(VARIANT));

        if (!This->properties || !This->values)
            res = E_OUTOFMEMORY;
        else
            for (i=0; i < count; i++)
            {
                res = copy_propbag2(This->properties+i, options+i);
                if (FAILED(res))
                    break;
                This->properties[i].dwHint = i+1; /* 0 means unset, so we start with 1 */
            }
    }

    if (FAILED(res))
    {
        PropertyBag_Release(&This->IPropertyBag2_iface);
        *ppPropertyBag2 = NULL;
    }
    else
        *ppPropertyBag2 = &This->IPropertyBag2_iface;

    return res;
}
