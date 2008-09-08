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
#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "ole2.h"
#include "msxml2.h"

#include "msxml_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(msxml);

#ifdef HAVE_LIBXML2

typedef struct _domimpl
{
    const struct IXMLDOMImplementationVtbl *lpVtbl;
    LONG ref;
} domimpl;

static inline domimpl *impl_from_IXMLDOMImplementation( IXMLDOMImplementation *iface )
{
    return (domimpl *)((char*)iface - FIELD_OFFSET(domimpl, lpVtbl));
}

static HRESULT WINAPI dimimpl_QueryInterface(
    IXMLDOMImplementation *iface,
    REFIID riid,
    void** ppvObject )
{
    domimpl *This = impl_from_IXMLDOMImplementation( iface );
    TRACE("%p %s %p\n", This, debugstr_guid(riid), ppvObject);

    if ( IsEqualGUID( riid, &IID_IXMLDOMImplementation ) ||
         IsEqualGUID( riid, &IID_IDispatch ) ||
         IsEqualGUID( riid, &IID_IUnknown ) )
    {
        *ppvObject = iface;
    }
    else
    {
        FIXME("Unsupported interface %s\n", debugstr_guid(riid));
        return E_NOINTERFACE;
    }

    IXMLDOMImplementation_AddRef( iface );

    return S_OK;
}

static ULONG WINAPI dimimpl_AddRef(
    IXMLDOMImplementation *iface )
{
    domimpl *This = impl_from_IXMLDOMImplementation( iface );
    return InterlockedIncrement( &This->ref );
}

static ULONG WINAPI dimimpl_Release(
    IXMLDOMImplementation *iface )
{
    domimpl *This = impl_from_IXMLDOMImplementation( iface );
    ULONG ref;

    ref = InterlockedDecrement( &This->ref );
    if ( ref == 0 )
    {
        HeapFree( GetProcessHeap(), 0, This );
    }

    return ref;
}

static HRESULT WINAPI dimimpl_GetTypeInfoCount(
    IXMLDOMImplementation *iface,
    UINT* pctinfo )
{
    domimpl *This = impl_from_IXMLDOMImplementation( iface );

    TRACE("(%p)->(%p)\n", This, pctinfo);

    *pctinfo = 1;

    return S_OK;
}

static HRESULT WINAPI dimimpl_GetTypeInfo(
    IXMLDOMImplementation *iface,
    UINT iTInfo, LCID lcid,
    ITypeInfo** ppTInfo )
{
    domimpl *This = impl_from_IXMLDOMImplementation( iface );
    HRESULT hr;

    TRACE("(%p)->(%u %u %p)\n", This, iTInfo, lcid, ppTInfo);

    hr = get_typeinfo(IXMLDOMImplementation_tid, ppTInfo);

    return hr;
}

static HRESULT WINAPI dimimpl_GetIDsOfNames(
    IXMLDOMImplementation *iface,
    REFIID riid, LPOLESTR* rgszNames,
    UINT cNames, LCID lcid, DISPID* rgDispId )
{
    domimpl *This = impl_from_IXMLDOMImplementation( iface );
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("(%p)->(%s %p %u %u %p)\n", This, debugstr_guid(riid), rgszNames, cNames,
          lcid, rgDispId);

    if(!rgszNames || cNames == 0 || !rgDispId)
        return E_INVALIDARG;

    hr = get_typeinfo(IXMLDOMImplementation_tid, &typeinfo);
    if(SUCCEEDED(hr))
    {
        hr = ITypeInfo_GetIDsOfNames(typeinfo, rgszNames, cNames, rgDispId);
        ITypeInfo_Release(typeinfo);
    }

    return hr;
}

static HRESULT WINAPI dimimpl_Invoke(
    IXMLDOMImplementation *iface,
    DISPID dispIdMember, REFIID riid, LCID lcid,
    WORD wFlags, DISPPARAMS* pDispParams, VARIANT* pVarResult,
    EXCEPINFO* pExcepInfo, UINT* puArgErr )
{
    domimpl *This = impl_from_IXMLDOMImplementation( iface );
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("(%p)->(%d %s %d %d %p %p %p %p)\n", This, dispIdMember, debugstr_guid(riid),
          lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);

    hr = get_typeinfo(IXMLDOMImplementation_tid, &typeinfo);
    if(SUCCEEDED(hr))
    {
        hr = ITypeInfo_Invoke(typeinfo, &(This->lpVtbl), dispIdMember, wFlags, pDispParams,
                pVarResult, pExcepInfo, puArgErr);
        ITypeInfo_Release(typeinfo);
    }

    return hr;
}

static HRESULT WINAPI dimimpl_hasFeature(IXMLDOMImplementation* This, BSTR feature, BSTR version, VARIANT_BOOL *hasFeature)
{
    static const WCHAR bVersion[] = {'1','.','0',0};
    static const WCHAR bXML[] = {'X','M','L',0};
    static const WCHAR bDOM[] = {'D','O','M',0};
    static const WCHAR bMSDOM[] = {'M','S','-','D','O','M',0};
    BOOL bValidFeature = FALSE;
    BOOL bValidVersion = FALSE;

    TRACE("feature(%s) version (%s)\n", debugstr_w(feature), debugstr_w(version));

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

IUnknown* create_doc_Implementation(void)
{
    domimpl *This;

    This = HeapAlloc( GetProcessHeap(), 0, sizeof *This );
    if ( !This )
        return NULL;

    This->lpVtbl = &dimimpl_vtbl;
    This->ref = 1;

    return (IUnknown*) &This->lpVtbl;
}

#endif
