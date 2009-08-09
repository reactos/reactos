/*
 *    ParseError implementation
 *
 * Copyright 2005 Huw Davies
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
#include <assert.h>
#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "winuser.h"
#include "ole2.h"
#include "msxml2.h"

#include "msxml_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(msxml);

typedef struct
{
    const struct IXMLDOMParseErrorVtbl *lpVtbl;
    LONG ref;
    LONG code, line, linepos, filepos;
    BSTR url, reason, srcText;
} parse_error_t;

static inline parse_error_t *impl_from_IXMLDOMParseError( IXMLDOMParseError *iface )
{
    return (parse_error_t *)((char*)iface - FIELD_OFFSET(parse_error_t, lpVtbl));
}

static HRESULT WINAPI parseError_QueryInterface(
    IXMLDOMParseError *iface,
    REFIID riid,
    void** ppvObject )
{
    TRACE("%p %s %p\n", iface, debugstr_guid(riid), ppvObject);

    if ( IsEqualGUID( riid, &IID_IUnknown ) ||
         IsEqualGUID( riid, &IID_IDispatch ) ||
         IsEqualGUID( riid, &IID_IXMLDOMParseError ) )
    {
        *ppvObject = iface;
    }
    else
    {
        FIXME("interface %s not implemented\n", debugstr_guid(riid));
        return E_NOINTERFACE;
    }

    IXMLDOMParseError_AddRef( iface );

    return S_OK;
}

static ULONG WINAPI parseError_AddRef(
    IXMLDOMParseError *iface )
{
    parse_error_t *This = impl_from_IXMLDOMParseError( iface );
    ULONG ref = InterlockedIncrement( &This->ref );
    TRACE("(%p) ref now %d\n", This, ref);
    return ref;
}

static ULONG WINAPI parseError_Release(
    IXMLDOMParseError *iface )
{
    parse_error_t *This = impl_from_IXMLDOMParseError( iface );
    ULONG ref;

    ref = InterlockedDecrement( &This->ref );
    TRACE("(%p) ref now %d\n", This, ref);
    if ( ref == 0 )
    {
        SysFreeString(This->url);
        SysFreeString(This->reason);
        SysFreeString(This->srcText);
        HeapFree( GetProcessHeap(), 0, This );
    }

    return ref;
}

static HRESULT WINAPI parseError_GetTypeInfoCount(
    IXMLDOMParseError *iface,
    UINT* pctinfo )
{
    parse_error_t *This = impl_from_IXMLDOMParseError( iface );

    TRACE("(%p)->(%p)\n", This, pctinfo);

    *pctinfo = 1;

    return S_OK;
}

static HRESULT WINAPI parseError_GetTypeInfo(
    IXMLDOMParseError *iface,
    UINT iTInfo,
    LCID lcid,
    ITypeInfo** ppTInfo )
{
    parse_error_t *This = impl_from_IXMLDOMParseError( iface );
    HRESULT hr;

    TRACE("(%p)->(%u %u %p)\n", This, iTInfo, lcid, ppTInfo);

    hr = get_typeinfo(IXMLDOMParseError_tid, ppTInfo);

    return hr;
}

static HRESULT WINAPI parseError_GetIDsOfNames(
    IXMLDOMParseError *iface,
    REFIID riid,
    LPOLESTR* rgszNames,
    UINT cNames,
    LCID lcid,
    DISPID* rgDispId )
{
    parse_error_t *This = impl_from_IXMLDOMParseError( iface );
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("(%p)->(%s %p %u %u %p)\n", This, debugstr_guid(riid), rgszNames, cNames,
          lcid, rgDispId);

    if(!rgszNames || cNames == 0 || !rgDispId)
        return E_INVALIDARG;

    hr = get_typeinfo(IXMLDOMParseError_tid, &typeinfo);
    if(SUCCEEDED(hr))
    {
        hr = ITypeInfo_GetIDsOfNames(typeinfo, rgszNames, cNames, rgDispId);
        ITypeInfo_Release(typeinfo);
    }

    return hr;
}

static HRESULT WINAPI parseError_Invoke(
    IXMLDOMParseError *iface,
    DISPID dispIdMember,
    REFIID riid,
    LCID lcid,
    WORD wFlags,
    DISPPARAMS* pDispParams,
    VARIANT* pVarResult,
    EXCEPINFO* pExcepInfo,
    UINT* puArgErr )
{
    parse_error_t *This = impl_from_IXMLDOMParseError( iface );
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("(%p)->(%d %s %d %d %p %p %p %p)\n", This, dispIdMember, debugstr_guid(riid),
          lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);

    hr = get_typeinfo(IXMLDOMParseError_tid, &typeinfo);
    if(SUCCEEDED(hr))
    {
        hr = ITypeInfo_Invoke(typeinfo, &(This->lpVtbl), dispIdMember, wFlags, pDispParams,
                pVarResult, pExcepInfo, puArgErr);
        ITypeInfo_Release(typeinfo);
    }

    return hr;
}

static HRESULT WINAPI parseError_get_errorCode(
    IXMLDOMParseError *iface,
    long *code )
{
    parse_error_t *This = impl_from_IXMLDOMParseError( iface );
    TRACE("(%p)->(%p)\n", This, code);

    *code = This->code;

    if(This->code == 0)
        return S_FALSE;

    return S_OK;
}

static HRESULT WINAPI parseError_get_url(
    IXMLDOMParseError *iface,
    BSTR *url )
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI parseError_get_reason(
    IXMLDOMParseError *iface,
    BSTR *reason )
{
    parse_error_t *This = impl_from_IXMLDOMParseError( iface );
    TRACE("(%p)->(%p)\n", This, reason);
    
    if(!This->reason)
    {
        *reason = NULL;
        return S_FALSE;
    }
    *reason = SysAllocString(This->reason);
    return S_OK;
}

static HRESULT WINAPI parseError_get_srcText(
    IXMLDOMParseError *iface,
    BSTR *srcText )
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI parseError_get_line(
    IXMLDOMParseError *iface,
    long *line )
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI parseError_get_linepos(
    IXMLDOMParseError *iface,
    long *linepos )
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI parseError_get_filepos(
    IXMLDOMParseError *iface,
    long *filepos )
{
    FIXME("\n");
    return E_NOTIMPL;
}

static const struct IXMLDOMParseErrorVtbl parseError_vtbl =
{
    parseError_QueryInterface,
    parseError_AddRef,
    parseError_Release,
    parseError_GetTypeInfoCount,
    parseError_GetTypeInfo,
    parseError_GetIDsOfNames,
    parseError_Invoke,
    parseError_get_errorCode,
    parseError_get_url,
    parseError_get_reason,
    parseError_get_srcText,
    parseError_get_line,
    parseError_get_linepos,
    parseError_get_filepos
};

IXMLDOMParseError *create_parseError( LONG code, BSTR url, BSTR reason, BSTR srcText,
                                      LONG line, LONG linepos, LONG filepos )
{
    parse_error_t *This;

    This = HeapAlloc( GetProcessHeap(), 0, sizeof(*This) );
    if ( !This )
        return NULL;

    This->lpVtbl = &parseError_vtbl;
    This->ref = 1;

    This->code = code;
    This->url = url;
    This->reason = reason;
    This->srcText = srcText;
    This->line = line;
    This->linepos = linepos;
    This->filepos = filepos;

    return (IXMLDOMParseError*) &This->lpVtbl;
}
