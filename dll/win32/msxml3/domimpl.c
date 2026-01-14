/*
 *    DOM Document Implementation implementation
 *
 * Copyright 2007 Alistair Leslie-Hughes
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
#include "winuser.h"
#include "ole2.h"
#include "msxml6.h"

#include "msxml_dispex.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(msxml);

typedef struct _domimpl
{
    DispatchEx dispex;
    IXMLDOMImplementation IXMLDOMImplementation_iface;
    LONG ref;
} domimpl;

static inline domimpl *impl_from_IXMLDOMImplementation( IXMLDOMImplementation *iface )
{
    return CONTAINING_RECORD(iface, domimpl, IXMLDOMImplementation_iface);
}

static HRESULT WINAPI domimpl_QueryInterface(
    IXMLDOMImplementation *iface,
    REFIID riid,
    void** ppvObject )
{
    domimpl *This = impl_from_IXMLDOMImplementation( iface );
    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppvObject);

    if ( IsEqualGUID( riid, &IID_IXMLDOMImplementation ) ||
         IsEqualGUID( riid, &IID_IDispatch ) ||
         IsEqualGUID( riid, &IID_IUnknown ) )
    {
        *ppvObject = iface;
    }
    else if (dispex_query_interface(&This->dispex, riid, ppvObject))
    {
        return *ppvObject ? S_OK : E_NOINTERFACE;
    }
    else
    {
        TRACE("Unsupported interface %s\n", debugstr_guid(riid));
        *ppvObject = NULL;
        return E_NOINTERFACE;
    }

    IXMLDOMImplementation_AddRef( iface );

    return S_OK;
}

static ULONG WINAPI domimpl_AddRef(IXMLDOMImplementation *iface)
{
    domimpl *domimpl = impl_from_IXMLDOMImplementation(iface);
    ULONG ref = InterlockedIncrement(&domimpl->ref);
    TRACE("%p, refcount %lu.\n", iface, ref);
    return ref;
}

static ULONG WINAPI domimpl_Release(IXMLDOMImplementation *iface)
{
    domimpl *domimpl = impl_from_IXMLDOMImplementation(iface);
    ULONG ref = InterlockedDecrement(&domimpl->ref);

    TRACE("%p, refcount %lu.\n", iface, ref);

    if (!ref)
        free(domimpl);

    return ref;
}

static HRESULT WINAPI domimpl_GetTypeInfoCount(
    IXMLDOMImplementation *iface,
    UINT* pctinfo )
{
    domimpl *This = impl_from_IXMLDOMImplementation( iface );
    return IDispatchEx_GetTypeInfoCount(&This->dispex.IDispatchEx_iface, pctinfo);
}

static HRESULT WINAPI domimpl_GetTypeInfo(
    IXMLDOMImplementation *iface,
    UINT iTInfo, LCID lcid,
    ITypeInfo** ppTInfo )
{
    domimpl *This = impl_from_IXMLDOMImplementation( iface );
    return IDispatchEx_GetTypeInfo(&This->dispex.IDispatchEx_iface,
        iTInfo, lcid, ppTInfo);
}

static HRESULT WINAPI domimpl_GetIDsOfNames(
    IXMLDOMImplementation *iface,
    REFIID riid, LPOLESTR* rgszNames,
    UINT cNames, LCID lcid, DISPID* rgDispId )
{
    domimpl *This = impl_from_IXMLDOMImplementation( iface );
    return IDispatchEx_GetIDsOfNames(&This->dispex.IDispatchEx_iface,
        riid, rgszNames, cNames, lcid, rgDispId);
}

static HRESULT WINAPI domimpl_Invoke(
    IXMLDOMImplementation *iface,
    DISPID dispIdMember, REFIID riid, LCID lcid,
    WORD wFlags, DISPPARAMS* pDispParams, VARIANT* pVarResult,
    EXCEPINFO* pExcepInfo, UINT* puArgErr )
{
    domimpl *This = impl_from_IXMLDOMImplementation( iface );
    return IDispatchEx_Invoke(&This->dispex.IDispatchEx_iface,
        dispIdMember, riid, lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI domimpl_hasFeature(IXMLDOMImplementation* This, BSTR feature, BSTR version, VARIANT_BOOL *hasFeature)
{
    static const WCHAR bVersion[] = {'1','.','0',0};
    static const WCHAR bXML[] = {'X','M','L',0};
    static const WCHAR bDOM[] = {'D','O','M',0};
    static const WCHAR bMSDOM[] = {'M','S','-','D','O','M',0};
    BOOL bValidFeature = FALSE;
    BOOL bValidVersion = FALSE;

    TRACE("(%p)->(%s %s %p)\n", This, debugstr_w(feature), debugstr_w(version), hasFeature);

    if(!feature || !hasFeature)
        return E_INVALIDARG;

    *hasFeature = VARIANT_FALSE;

    if(!version || lstrcmpiW(version, bVersion) == 0)
        bValidVersion = TRUE;

    if(lstrcmpiW(feature, bXML) == 0 || lstrcmpiW(feature, bDOM) == 0 || lstrcmpiW(feature, bMSDOM) == 0)
        bValidFeature = TRUE;

    if(bValidVersion && bValidFeature)
        *hasFeature = VARIANT_TRUE;

    return S_OK;
}

static const struct IXMLDOMImplementationVtbl domimpl_vtbl =
{
    domimpl_QueryInterface,
    domimpl_AddRef,
    domimpl_Release,
    domimpl_GetTypeInfoCount,
    domimpl_GetTypeInfo,
    domimpl_GetIDsOfNames,
    domimpl_Invoke,
    domimpl_hasFeature
};

static const tid_t domimpl_iface_tids[] =
{
    IXMLDOMImplementation_tid,
    0
};

static dispex_static_data_t domimpl_dispex =
{
    NULL,
    IXMLDOMImplementation_tid,
    NULL,
    domimpl_iface_tids
};

HRESULT create_dom_implementation(IXMLDOMImplementation **ret)
{
    domimpl *object;

    if (!(object = malloc(sizeof(*object))))
        return E_OUTOFMEMORY;

    object->IXMLDOMImplementation_iface.lpVtbl = &domimpl_vtbl;
    object->ref = 1;
    init_dispex(&object->dispex, (IUnknown *)&object->IXMLDOMImplementation_iface, &domimpl_dispex);

    *ret = &object->IXMLDOMImplementation_iface;

    return S_OK;
}
