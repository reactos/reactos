/*
 * XML Parser implementation
 *
 * Copyright 2011 Alistair Leslie-Hughes
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

#include "ole2.h"

#include "initguid.h"
#include "xmlparser.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(msxml);

typedef struct _xmlparser
{
    IXMLParser IXMLParser_iface;
    IXMLNodeFactory *nodefactory;
    IUnknown *input;
    LONG ref;

    int flags;
    XML_PARSER_STATE state;
} xmlparser;

static inline xmlparser *impl_from_IXMLParser( IXMLParser *iface )
{
    return CONTAINING_RECORD(iface, xmlparser, IXMLParser_iface);
}

/*** IUnknown methods ***/
static HRESULT WINAPI xmlparser_QueryInterface(IXMLParser* iface, REFIID riid, void **ppvObject)
{
    xmlparser *This = impl_from_IXMLParser( iface );
    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppvObject);

    if ( IsEqualGUID( riid, &IID_IXMLParser ) ||
         IsEqualGUID( riid, &IID_IXMLNodeSource ) ||
         IsEqualGUID( riid, &IID_IUnknown ) )
    {
        *ppvObject = iface;
    }
    else
    {
        TRACE("Unsupported interface %s\n", debugstr_guid(riid));
        *ppvObject = NULL;
        return E_NOINTERFACE;
    }

    IXMLParser_AddRef(iface);
    return S_OK;
}

static ULONG WINAPI xmlparser_AddRef(IXMLParser* iface)
{
    xmlparser *This = impl_from_IXMLParser( iface );
    ULONG ref = InterlockedIncrement( &This->ref );
    TRACE("%p, refcount %lu.\n", iface, ref);
    return ref;
}

static ULONG WINAPI xmlparser_Release(IXMLParser* iface)
{
    xmlparser *This = impl_from_IXMLParser( iface );
    ULONG ref = InterlockedDecrement( &This->ref );

    TRACE("%p, refcount %lu.\n", iface, ref);
    if ( ref == 0 )
    {
        if(This->input)
            IUnknown_Release(This->input);

        if(This->nodefactory)
            IXMLNodeFactory_Release(This->nodefactory);

        free(This);
    }

    return ref;
}

/*** IXMLNodeSource methods ***/
static HRESULT WINAPI xmlparser_SetFactory(IXMLParser *iface, IXMLNodeFactory *pNodeFactory)
{
    xmlparser *This = impl_from_IXMLParser( iface );

    TRACE("(%p %p)\n", This, pNodeFactory);

    if(This->nodefactory)
        IXMLNodeFactory_Release(This->nodefactory);

    This->nodefactory = pNodeFactory;
    if(This->nodefactory)
        IXMLNodeFactory_AddRef(This->nodefactory);

    return S_OK;
}

static HRESULT WINAPI xmlparser_GetFactory(IXMLParser *iface, IXMLNodeFactory **ppNodeFactory)
{
    xmlparser *This = impl_from_IXMLParser( iface );

    TRACE("(%p, %p)\n", This, ppNodeFactory);

    if(!ppNodeFactory)
        return E_INVALIDARG;

    *ppNodeFactory = This->nodefactory;

    if(*ppNodeFactory)
        IXMLNodeFactory_AddRef(*ppNodeFactory);

    return S_OK;
}

static HRESULT WINAPI xmlparser_Abort(IXMLParser *iface, BSTR bstrErrorInfo)
{
    xmlparser *This = impl_from_IXMLParser( iface );

    FIXME("(%p, %s)\n", This, debugstr_w(bstrErrorInfo));

    return E_NOTIMPL;
}

static ULONG WINAPI xmlparser_GetLineNumber(IXMLParser *iface)
{
    xmlparser *This = impl_from_IXMLParser( iface );

    FIXME("(%p)\n", This);

    return 0;
}

static ULONG WINAPI xmlparser_GetLinePosition(IXMLParser *iface)
{
    xmlparser *This = impl_from_IXMLParser( iface );

    FIXME("(%p)\n", This);

    return 0;
}

static ULONG WINAPI xmlparser_GetAbsolutePosition(IXMLParser *iface)
{
    xmlparser *This = impl_from_IXMLParser( iface );

    FIXME("(%p)\n", This);

    return 0;
}

static HRESULT WINAPI xmlparser_GetLineBuffer(IXMLParser *iface, const WCHAR **ppBuf,
                ULONG *len, ULONG *startPos)
{
    xmlparser *This = impl_from_IXMLParser( iface );

    FIXME("(%p %p %p %p)\n", This, ppBuf, len, startPos);

    return 0;
}

static HRESULT WINAPI xmlparser_GetLastError(IXMLParser *iface)
{
    xmlparser *This = impl_from_IXMLParser( iface );

    FIXME("(%p)\n", This);

    return E_NOTIMPL;
}

static HRESULT WINAPI xmlparser_GetErrorInfo(IXMLParser *iface, BSTR *pErrorInfo)
{
    xmlparser *This = impl_from_IXMLParser( iface );

    FIXME("(%p %p)\n", This, pErrorInfo);

    return E_NOTIMPL;
}

static ULONG WINAPI xmlparser_GetFlags(IXMLParser *iface)
{
    xmlparser *This = impl_from_IXMLParser( iface );

    TRACE("(%p)\n", This);

    return This->flags;
}

static HRESULT WINAPI xmlparser_GetURL(IXMLParser *iface, const WCHAR **ppBuf)
{
    xmlparser *This = impl_from_IXMLParser( iface );

    FIXME("(%p %p)\n", This, ppBuf);

    return E_NOTIMPL;
}

/*** IXMLParser methods ***/
static HRESULT WINAPI xmlparser_SetURL(IXMLParser *iface,const WCHAR *pszBaseUrl,
                const WCHAR *relativeUrl, BOOL async)
{
    xmlparser *This = impl_from_IXMLParser( iface );

    FIXME("(%p %s %s %d)\n", This, debugstr_w(pszBaseUrl), debugstr_w(relativeUrl), async);

    return E_NOTIMPL;
}

static HRESULT WINAPI xmlparser_Load(IXMLParser *iface, BOOL bFullyAvailable,
                IMoniker *pMon, LPBC pBC, DWORD dwMode)
{
    FIXME("%p, %d, %p, %p, %ld.\n", iface, bFullyAvailable, pMon, pBC, dwMode);

    return E_NOTIMPL;
}

static HRESULT WINAPI xmlparser_SetInput(IXMLParser *iface, IUnknown *pStm)
{
    xmlparser *This = impl_from_IXMLParser( iface );

    TRACE("(%p %p)\n", This, pStm);

    if(!pStm)
        return E_INVALIDARG;

    if(This->input)
        IUnknown_Release(This->input);

    This->input = pStm;
    IUnknown_AddRef(This->input);

    return S_OK;
}

static HRESULT WINAPI xmlparser_PushData(IXMLParser *iface, const char *pData,
                ULONG nChars, BOOL fLastBuffer)
{
    FIXME("%p, %s, %lu, %d.\n", iface, debugstr_a(pData), nChars, fLastBuffer);

    return E_NOTIMPL;
}

static HRESULT WINAPI xmlparser_LoadDTD(IXMLParser *iface, const WCHAR *baseUrl,
                const WCHAR *relativeUrl)
{
    xmlparser *This = impl_from_IXMLParser( iface );

    FIXME("(%p %s %s)\n", This, debugstr_w(baseUrl), debugstr_w(relativeUrl));

    return E_NOTIMPL;
}

static HRESULT WINAPI xmlparser_LoadEntity(IXMLParser *iface, const WCHAR *baseUrl,
                const WCHAR *relativeUrl, BOOL fpe)
{
    xmlparser *This = impl_from_IXMLParser( iface );

    FIXME("(%p %s %s %d)\n", This, debugstr_w(baseUrl), debugstr_w(relativeUrl), fpe);

    return E_NOTIMPL;
}

static HRESULT WINAPI xmlparser_ParseEntity(IXMLParser *iface, const WCHAR *text,
                ULONG len, BOOL fpe)
{
    FIXME("%p, %s, %lu, %d.\n", iface, debugstr_w(text), len, fpe);

    return E_NOTIMPL;
}

static HRESULT WINAPI xmlparser_ExpandEntity(IXMLParser *iface, const WCHAR *text,
                ULONG len)
{
    FIXME("%p, %s, %ld.\n", iface, debugstr_w(text), len);

    return E_NOTIMPL;
}

static HRESULT WINAPI xmlparser_SetRoot(IXMLParser *iface, PVOID pRoot)
{
    xmlparser *This = impl_from_IXMLParser( iface );

    FIXME("(%p %p)\n", This, pRoot);

    return E_NOTIMPL;
}

static HRESULT WINAPI xmlparser_GetRoot( IXMLParser *iface, PVOID *ppRoot)
{
    xmlparser *This = impl_from_IXMLParser( iface );

    FIXME("(%p %p)\n", This, ppRoot);

    return E_NOTIMPL;
}

static HRESULT WINAPI xmlparser_Run(IXMLParser *iface, LONG chars)
{
    FIXME("%p, %ld.\n", iface, chars);

    return E_NOTIMPL;
}

static HRESULT WINAPI xmlparser_GetParserState(IXMLParser *iface)
{
    xmlparser *This = impl_from_IXMLParser( iface );

    TRACE("(%p)\n", This);

    return This->state;
}

static HRESULT WINAPI xmlparser_Suspend(IXMLParser *iface)
{
    xmlparser *This = impl_from_IXMLParser( iface );

    FIXME("(%p)\n", This);

    return E_NOTIMPL;
}

static HRESULT WINAPI xmlparser_Reset(IXMLParser *iface)
{
    xmlparser *This = impl_from_IXMLParser( iface );

    FIXME("(%p)\n", This);

    return E_NOTIMPL;
}

static HRESULT WINAPI xmlparser_SetFlags(IXMLParser *iface, ULONG flags)
{
    xmlparser *This = impl_from_IXMLParser( iface );

    TRACE("%p, %lx.\n", iface, flags);

    This->flags = flags;

    return S_OK;
}

static HRESULT WINAPI xmlparser_SetSecureBaseURL(IXMLParser *iface, const WCHAR *baseUrl)
{
    xmlparser *This = impl_from_IXMLParser( iface );

    FIXME("(%p %s)\n", This, debugstr_w(baseUrl));

    return E_NOTIMPL;
}

static HRESULT WINAPI xmlparser_GetSecureBaseURL( IXMLParser *iface, const WCHAR **ppBuf)
{
    xmlparser *This = impl_from_IXMLParser( iface );

    FIXME("(%p %p)\n", This, ppBuf);

    return E_NOTIMPL;
}


static const struct IXMLParserVtbl xmlparser_vtbl =
{
    xmlparser_QueryInterface,
    xmlparser_AddRef,
    xmlparser_Release,
    xmlparser_SetFactory,
    xmlparser_GetFactory,
    xmlparser_Abort,
    xmlparser_GetLineNumber,
    xmlparser_GetLinePosition,
    xmlparser_GetAbsolutePosition,
    xmlparser_GetLineBuffer,
    xmlparser_GetLastError,
    xmlparser_GetErrorInfo,
    xmlparser_GetFlags,
    xmlparser_GetURL,
    xmlparser_SetURL,
    xmlparser_Load,
    xmlparser_SetInput,
    xmlparser_PushData,
    xmlparser_LoadDTD,
    xmlparser_LoadEntity,
    xmlparser_ParseEntity,
    xmlparser_ExpandEntity,
    xmlparser_SetRoot,
    xmlparser_GetRoot,
    xmlparser_Run,
    xmlparser_GetParserState,
    xmlparser_Suspend,
    xmlparser_Reset,
    xmlparser_SetFlags,
    xmlparser_SetSecureBaseURL,
    xmlparser_GetSecureBaseURL
};

HRESULT XMLParser_create(void **ppObj)
{
    xmlparser *This;

    TRACE("(%p)\n", ppObj);

    This = malloc(sizeof(xmlparser));
    if(!This)
        return E_OUTOFMEMORY;

    This->IXMLParser_iface.lpVtbl = &xmlparser_vtbl;
    This->nodefactory = NULL;
    This->input = NULL;
    This->flags = 0;
    This->state = XMLPARSER_IDLE;
    This->ref = 1;

    *ppObj = &This->IXMLParser_iface;

    TRACE("returning iface %p\n", *ppObj);

    return S_OK;
}
