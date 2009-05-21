/*
 *    IXMLHTTPRequest implementation
 *
 * Copyright 2008 Alistair Leslie-Hughes
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

typedef struct _httprequest
{
    const struct IXMLHTTPRequestVtbl *lpVtbl;
    LONG ref;
} httprequest;

static inline httprequest *impl_from_IXMLHTTPRequest( IXMLHTTPRequest *iface )
{
    return (httprequest *)((char*)iface - FIELD_OFFSET(httprequest, lpVtbl));
}

static HRESULT WINAPI httprequest_QueryInterface(IXMLHTTPRequest *iface, REFIID riid, void **ppvObject)
{
    httprequest *This = impl_from_IXMLHTTPRequest( iface );
    TRACE("%p %s %p\n", This, debugstr_guid(riid), ppvObject);

    if ( IsEqualGUID( riid, &IID_IXMLHTTPRequest) ||
         IsEqualGUID( riid, &IID_IDispatch) ||
         IsEqualGUID( riid, &IID_IUnknown) )
    {
        *ppvObject = iface;
    }
    else
    {
        FIXME("Unsupported interface %s\n", debugstr_guid(riid));
        return E_NOINTERFACE;
    }

    IXMLHTTPRequest_AddRef( iface );

    return S_OK;
}

static ULONG WINAPI httprequest_AddRef(IXMLHTTPRequest *iface)
{
    httprequest *This = impl_from_IXMLHTTPRequest( iface );
    return InterlockedIncrement( &This->ref );
}

static ULONG WINAPI httprequest_Release(IXMLHTTPRequest *iface)
{
    httprequest *This = impl_from_IXMLHTTPRequest( iface );
    ULONG ref;

    ref = InterlockedDecrement( &This->ref );
    if ( ref == 0 )
    {
        HeapFree( GetProcessHeap(), 0, This );
    }

    return ref;
}

static HRESULT WINAPI httprequest_GetTypeInfoCount(IXMLHTTPRequest *iface, UINT *pctinfo)
{
    httprequest *This = impl_from_IXMLHTTPRequest( iface );

    TRACE("(%p)->(%p)\n", This, pctinfo);

    *pctinfo = 1;

    return S_OK;
}

static HRESULT WINAPI httprequest_GetTypeInfo(IXMLHTTPRequest *iface, UINT iTInfo,
        LCID lcid, ITypeInfo **ppTInfo)
{
    httprequest *This = impl_from_IXMLHTTPRequest( iface );
    HRESULT hr;

    TRACE("(%p)->(%u %u %p)\n", This, iTInfo, lcid, ppTInfo);

    hr = get_typeinfo(IXMLHTTPRequest_tid, ppTInfo);

    return hr;
}

static HRESULT WINAPI httprequest_GetIDsOfNames(IXMLHTTPRequest *iface, REFIID riid,
        LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{
    httprequest *This = impl_from_IXMLHTTPRequest( iface );
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("(%p)->(%s %p %u %u %p)\n", This, debugstr_guid(riid), rgszNames, cNames,
          lcid, rgDispId);

    if(!rgszNames || cNames == 0 || !rgDispId)
        return E_INVALIDARG;

    hr = get_typeinfo(IXMLHTTPRequest_tid, &typeinfo);
    if(SUCCEEDED(hr))
    {
        hr = ITypeInfo_GetIDsOfNames(typeinfo, rgszNames, cNames, rgDispId);
        ITypeInfo_Release(typeinfo);
    }

    return hr;
}

static HRESULT WINAPI httprequest_Invoke(IXMLHTTPRequest *iface, DISPID dispIdMember, REFIID riid,
        LCID lcid, WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult,
        EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    httprequest *This = impl_from_IXMLHTTPRequest( iface );
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("(%p)->(%d %s %d %d %p %p %p %p)\n", This, dispIdMember, debugstr_guid(riid),
          lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);

    hr = get_typeinfo(IXMLHTTPRequest_tid, &typeinfo);
    if(SUCCEEDED(hr))
    {
        hr = ITypeInfo_Invoke(typeinfo, &(This->lpVtbl), dispIdMember, wFlags, pDispParams,
                pVarResult, pExcepInfo, puArgErr);
        ITypeInfo_Release(typeinfo);
    }

    return hr;
}

static HRESULT WINAPI httprequest_open(IXMLHTTPRequest *iface, BSTR bstrMethod, BSTR bstrUrl,
        VARIANT varAsync, VARIANT bstrUser, VARIANT bstrPassword)
{
    httprequest *This = impl_from_IXMLHTTPRequest( iface );

    FIXME("stub (%p)\n", This);

    return E_NOTIMPL;
}

static HRESULT WINAPI httprequest_setRequestHeader(IXMLHTTPRequest *iface, BSTR bstrHeader, BSTR bstrValue)
{
    httprequest *This = impl_from_IXMLHTTPRequest( iface );

    FIXME("stub (%p) %s %s\n", This, debugstr_w(bstrHeader), debugstr_w(bstrValue));

    return E_NOTIMPL;
}

static HRESULT WINAPI httprequest_getResponseHeader(IXMLHTTPRequest *iface, BSTR bstrHeader, BSTR *pbstrValue)
{
    httprequest *This = impl_from_IXMLHTTPRequest( iface );

    FIXME("stub (%p) %s %p\n", This, debugstr_w(bstrHeader), pbstrValue);

    return E_NOTIMPL;
}

static HRESULT WINAPI httprequest_getAllResponseHeaders(IXMLHTTPRequest *iface, BSTR *pbstrHeaders)
{
    httprequest *This = impl_from_IXMLHTTPRequest( iface );

    FIXME("stub (%p) %p\n", This, pbstrHeaders);

    return E_NOTIMPL;
}

static HRESULT WINAPI httprequest_send(IXMLHTTPRequest *iface, VARIANT varBody)
{
    httprequest *This = impl_from_IXMLHTTPRequest( iface );

    FIXME("stub (%p)\n", This);

    return E_NOTIMPL;
}

static HRESULT WINAPI httprequest_abort(IXMLHTTPRequest *iface)
{
    httprequest *This = impl_from_IXMLHTTPRequest( iface );

    FIXME("stub (%p)\n", This);

    return E_NOTIMPL;
}

static HRESULT WINAPI httprequest_get_status(IXMLHTTPRequest *iface, LONG *plStatus)
{
    httprequest *This = impl_from_IXMLHTTPRequest( iface );

    FIXME("stub %p %p\n", This, plStatus);

    return E_NOTIMPL;
}

static HRESULT WINAPI httprequest_get_statusText(IXMLHTTPRequest *iface, BSTR *pbstrStatus)
{
    httprequest *This = impl_from_IXMLHTTPRequest( iface );

    FIXME("stub %p %p\n", This, pbstrStatus);

    return E_NOTIMPL;
}

static HRESULT WINAPI httprequest_get_responseXML(IXMLHTTPRequest *iface, IDispatch **ppBody)
{
    httprequest *This = impl_from_IXMLHTTPRequest( iface );

    FIXME("stub %p %p\n", This, ppBody);

    return E_NOTIMPL;
}

static HRESULT WINAPI httprequest_get_responseText(IXMLHTTPRequest *iface, BSTR *pbstrBody)
{
    httprequest *This = impl_from_IXMLHTTPRequest( iface );

    FIXME("stub %p %p\n", This, pbstrBody);

    return E_NOTIMPL;
}

static HRESULT WINAPI httprequest_get_responseBody(IXMLHTTPRequest *iface, VARIANT *pvarBody)
{
    httprequest *This = impl_from_IXMLHTTPRequest( iface );

    FIXME("stub %p %p\n", This, pvarBody);

    return E_NOTIMPL;
}

static HRESULT WINAPI httprequest_get_responseStream(IXMLHTTPRequest *iface, VARIANT *pvarBody)
{
    httprequest *This = impl_from_IXMLHTTPRequest( iface );

    FIXME("stub %p %p\n", This, pvarBody);

    return E_NOTIMPL;
}

static HRESULT WINAPI httprequest_get_readyState(IXMLHTTPRequest *iface, LONG *plState)
{
    httprequest *This = impl_from_IXMLHTTPRequest( iface );

    FIXME("stub %p %p\n", This, plState);

    return E_NOTIMPL;
}

static HRESULT WINAPI httprequest_put_onreadystatechange(IXMLHTTPRequest *iface, IDispatch *pReadyStateSink)
{
    httprequest *This = impl_from_IXMLHTTPRequest( iface );

    FIXME("stub %p %p\n", This, pReadyStateSink);

    return E_NOTIMPL;
}

static const struct IXMLHTTPRequestVtbl dimimpl_vtbl =
{
    httprequest_QueryInterface,
    httprequest_AddRef,
    httprequest_Release,
    httprequest_GetTypeInfoCount,
    httprequest_GetTypeInfo,
    httprequest_GetIDsOfNames,
    httprequest_Invoke,
    httprequest_open,
    httprequest_setRequestHeader,
    httprequest_getResponseHeader,
    httprequest_getAllResponseHeaders,
    httprequest_send,
    httprequest_abort,
    httprequest_get_status,
    httprequest_get_statusText,
    httprequest_get_responseXML,
    httprequest_get_responseText,
    httprequest_get_responseBody,
    httprequest_get_responseStream,
    httprequest_get_readyState,
    httprequest_put_onreadystatechange
};

HRESULT XMLHTTPRequest_create(IUnknown *pUnkOuter, LPVOID *ppObj)
{
    httprequest *req;
    HRESULT hr = S_OK;

    TRACE("(%p,%p)\n", pUnkOuter, ppObj);

    req = HeapAlloc( GetProcessHeap(), 0, sizeof (*req) );
    if( !req )
        return E_OUTOFMEMORY;

    req->lpVtbl = &dimimpl_vtbl;
    req->ref = 1;

    *ppObj = &req->lpVtbl;

    TRACE("returning iface %p\n", *ppObj);

    return hr;
}

#else

HRESULT XMLHTTPRequest_create(IUnknown *pUnkOuter, LPVOID *ppObj)
{
    MESSAGE("This program tried to use a XMLHTTPRequest object, but\n"
            "libxml2 support was not present at compile time.\n");
    return E_NOTIMPL;
}

#endif
