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

#include "config.h"

#include <stdarg.h>
#ifdef HAVE_LIBXML2
# include <libxml/parser.h>
# include <libxml/xmlerror.h>
#endif

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "ole2.h"
#include "msxml6.h"

#include "msxml_private.h"

#include "wine/debug.h"

#ifdef HAVE_LIBXML2

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

static HRESULT WINAPI dimimpl_QueryInterface(
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

static ULONG WINAPI dimimpl_AddRef(
    IXMLDOMImplementation *iface )
{
    domimpl *This = impl_from_IXMLDOMImplementation( iface );
    ULONG ref = InterlockedIncrement( &This->ref );
    TRACE("(%p)->(%d)\n", This, ref);
    return ref;
}

static ULONG WINAPI dimimpl_Release(
    IXMLDOMImplementation *iface )
{
    domimpl *This = impl_from_IXMLDOMImplementation( iface );
    ULONG ref = InterlockedDecrement( &This->ref );

    TRACE("(%p)->(%d)\n", This, ref);
    if ( ref == 0 )
        heap_free( This );

    return ref;
}

static HRESULT WINAPI dimimpl_GetTypeInfoCount(
    IXMLDOMImplementation *iface,
    UINT* pctinfo )
{
    domimpl *This = impl_from_IXMLDOMImplementation( iface );
    return IDispatchEx_GetTypeInfoCount(&This->dispex.IDispatchEx_iface, pctinfo);
}

static HRESULT WINAPI dimimpl_GetTypeInfo(
    IXMLDOMImplementation *iface,
    UINT iTInfo, LCID lcid,
    ITypeInfo** ppTInfo )
{
    domimpl *This = impl_from_IXMLDOMImplementation( iface );
    return IDispatchEx_GetTypeInfo(&This->dispex.IDispatchEx_iface,
        iTInfo, lcid, ppTInfo);
}

static HRESULT WINAPI dimimpl_GetIDsOfNames(
    IXMLDOMImplementation *iface,
    REFIID riid, LPOLESTR* rgszNames,
    UINT cNames, LCID lcid, DISPID* rgDispId )
{
    domimpl *This = impl_from_IXMLDOMImplementation( iface );
    return IDispatchEx_GetIDsOfNames(&This->dispex.IDispatchEx_iface,
        riid, rgszNames, cNames, lcid, rgDispId);
}

static HRESULT WINAPI dimimpl_Invoke(
    IXMLDOMImplementation *iface,
    DISPID dispIdMember, REFIID riid, LCID lcid,
    WORD wFlags, DISPPARAMS* pDispParams, VARIANT* pVarResult,
    EXCEPINFO* pExcepInfo, UINT* puArgErr )
{
    domimpl *This = impl_from_IXMLDOMImplementation( iface );
    return IDispatchEx_Invoke(&This->dispex.IDispatchEx_iface,
        dispIdMember, riid, lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI dimimpl_hasFeature(IXMLDOMImplementation* This, BSTR feature, BSTR version, VARIANT_BOOL *hasFeature)
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

static const struct IXMLDOMImplementationVtbl dimimpl_vtbl =
{
    dimimpl_QueryInterface,
    dimimpl_AddRef,
    dimimpl_Release,
    dimimpl_GetTypeInfoCount,
    dimimpl_GetTypeInfo,
    dimimpl_GetIDsOfNames,
    dimimpl_Invoke,
    dimimpl_hasFeature
};

static const tid_t dimimpl_iface_tids[] = {
    IXMLDOMImplementation_tid,
    0
};

static dispex_static_data_t dimimpl_dispex = {
    NULL,
    IXMLDOMImplementation_tid,
    NULL,
    dimimpl_iface_tids
};

IUnknown* create_doc_Implementation(void)
{
    domimpl *This;

    This = heap_alloc( sizeof *This );
    if ( !This )
        return NULL;

    This->IXMLDOMImplementation_iface.lpVtbl = &dimimpl_vtbl;
    This->ref = 1;
    init_dispex(&This->dispex, (IUnknown*)&This->IXMLDOMImplementation_iface, &dimimpl_dispex);

    return (IUnknown*)&This->IXMLDOMImplementation_iface;
}

#endif
