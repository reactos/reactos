/*
 *    SAX Reader implementation
 *
 * Copyright 2008 Alistair Leslie-Hughes
 * Copyright 2008 Piotr Caban
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
#include "winuser.h"
#include "winnls.h"
#include "ole2.h"
#include "msxml2.h"
#include "wininet.h"
#include "urlmon.h"
#include "winreg.h"
#include "shlwapi.h"

#include "wine/debug.h"

#include "msxml_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(msxml);

#ifdef HAVE_LIBXML2

#include <libxml/SAX2.h>
#include <libxml/parserInternals.h>

typedef struct _saxreader
{
    const struct IVBSAXXMLReaderVtbl *lpVBSAXXMLReaderVtbl;
    const struct ISAXXMLReaderVtbl *lpSAXXMLReaderVtbl;
    LONG ref;
    struct ISAXContentHandler *contentHandler;
    struct IVBSAXContentHandler *vbcontentHandler;
    struct ISAXErrorHandler *errorHandler;
    struct IVBSAXErrorHandler *vberrorHandler;
    struct ISAXLexicalHandler *lexicalHandler;
    struct IVBSAXLexicalHandler *vblexicalHandler;
    struct ISAXDeclHandler *declHandler;
    struct IVBSAXDeclHandler *vbdeclHandler;
    xmlSAXHandler sax;
    BOOL isParsing;
} saxreader;

typedef struct _saxlocator
{
    const struct IVBSAXLocatorVtbl *lpVBSAXLocatorVtbl;
    const struct ISAXLocatorVtbl *lpSAXLocatorVtbl;
    LONG ref;
    saxreader *saxreader;
    HRESULT ret;
    xmlParserCtxtPtr pParserCtxt;
    WCHAR *publicId;
    WCHAR *systemId;
    xmlChar *lastCur;
    int line;
    int realLine;
    int column;
    int realColumn;
    BOOL vbInterface;
    int nsStackSize;
    int nsStackLast;
    int *nsStack;
} saxlocator;

typedef struct _saxattributes
{
    const struct IVBSAXAttributesVtbl *lpVBSAXAttributesVtbl;
    const struct ISAXAttributesVtbl *lpSAXAttributesVtbl;
    LONG ref;
    int nb_attributes;
    BSTR *szLocalname;
    BSTR *szURI;
    BSTR *szValue;
    BSTR *szQName;
} saxattributes;

static inline saxreader *impl_from_IVBSAXXMLReader( IVBSAXXMLReader *iface )
{
    return (saxreader *)((char*)iface - FIELD_OFFSET(saxreader, lpVBSAXXMLReaderVtbl));
}

static inline saxreader *impl_from_ISAXXMLReader( ISAXXMLReader *iface )
{
    return (saxreader *)((char*)iface - FIELD_OFFSET(saxreader, lpSAXXMLReaderVtbl));
}

static inline saxlocator *impl_from_IVBSAXLocator( IVBSAXLocator *iface )
{
    return (saxlocator *)((char*)iface - FIELD_OFFSET(saxlocator, lpVBSAXLocatorVtbl));
}

static inline saxlocator *impl_from_ISAXLocator( ISAXLocator *iface )
{
    return (saxlocator *)((char*)iface - FIELD_OFFSET(saxlocator, lpSAXLocatorVtbl));
}

static inline saxattributes *impl_from_IVBSAXAttributes( IVBSAXAttributes *iface )
{
    return (saxattributes *)((char*)iface - FIELD_OFFSET(saxattributes, lpVBSAXAttributesVtbl));
}

static inline saxattributes *impl_from_ISAXAttributes( ISAXAttributes *iface )
{
    return (saxattributes *)((char*)iface - FIELD_OFFSET(saxattributes, lpSAXAttributesVtbl));
}


static HRESULT namespacePush(saxlocator *locator, int ns)
{
    if(locator->nsStackLast>=locator->nsStackSize)
    {
        int *new_stack;

        new_stack = HeapReAlloc(GetProcessHeap(), 0,
                locator->nsStack, locator->nsStackSize*2);
        if(!new_stack) return E_OUTOFMEMORY;
        locator->nsStack = new_stack;
        locator->nsStackSize *= 2;
    }
    locator->nsStack[locator->nsStackLast++] = ns;

    return S_OK;
}

static int namespacePop(saxlocator *locator)
{
    if(locator->nsStackLast == 0) return 0;
    return locator->nsStack[--locator->nsStackLast];
}

static BSTR bstr_from_xmlCharN(const xmlChar *buf, int len)
{
    DWORD dLen;
    LPWSTR str;
    BSTR bstr;

    if (!buf)
        return NULL;

    dLen = MultiByteToWideChar(CP_UTF8, 0, (LPCSTR)buf, len, NULL, 0);
    if(len != -1) dLen++;
    str = HeapAlloc(GetProcessHeap(), 0, dLen * sizeof (WCHAR));
    if (!str)
        return NULL;
    MultiByteToWideChar(CP_UTF8, 0, (LPCSTR)buf, len, str, dLen);
    if(len != -1) str[dLen-1] = '\0';
    bstr = SysAllocString(str);
    HeapFree(GetProcessHeap(), 0, str);

    return bstr;
}

static BSTR QName_from_xmlChar(const xmlChar *prefix, const xmlChar *name)
{
    DWORD dLen, dLast;
    LPWSTR str;
    BSTR bstr;

    if(!name) return NULL;

    if(!prefix || *prefix=='\0')
        return bstr_from_xmlChar(name);

    dLen = MultiByteToWideChar(CP_UTF8, 0, (LPCSTR)prefix, -1, NULL, 0)
        + MultiByteToWideChar(CP_UTF8, 0, (LPCSTR)name, -1, NULL, 0);
    str = HeapAlloc(GetProcessHeap(), 0, dLen * sizeof(WCHAR));
    if(!str)
        return NULL;

    dLast = MultiByteToWideChar(CP_UTF8, 0, (LPCSTR)prefix, -1, str, dLen);
    str[dLast-1] = ':';
    MultiByteToWideChar(CP_UTF8, 0, (LPCSTR)name, -1, &str[dLast], dLen-dLast);
    bstr = SysAllocString(str);

    HeapFree(GetProcessHeap(), 0, str);

    return bstr;
}

static void format_error_message_from_id(saxlocator *This, HRESULT hr)
{
    xmlStopParser(This->pParserCtxt);
    This->ret = hr;

    if((This->vbInterface && This->saxreader->vberrorHandler)
            || (!This->vbInterface && This->saxreader->errorHandler))
    {
        WCHAR msg[1024];
        if(!FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM,
                    NULL, hr, 0, msg, sizeof(msg), NULL))
        {
            FIXME("MSXML errors not yet supported.\n");
            msg[0] = '\0';
        }

        if(This->vbInterface)
        {
            BSTR bstrMsg = SysAllocString(msg);
            IVBSAXErrorHandler_fatalError(This->saxreader->vberrorHandler,
                    (IVBSAXLocator*)&This->lpVBSAXLocatorVtbl, &bstrMsg, hr);
        }
        else
            ISAXErrorHandler_fatalError(This->saxreader->errorHandler,
                    (ISAXLocator*)&This->lpSAXLocatorVtbl, msg, hr);
    }
}

static void update_position(saxlocator *This, xmlChar *end)
{
    if(This->lastCur == NULL)
    {
        This->lastCur = (xmlChar*)This->pParserCtxt->input->base;
        This->realLine = 1;
        This->realColumn = 1;
    }
    else if(This->lastCur < This->pParserCtxt->input->base)
    {
        This->lastCur = (xmlChar*)This->pParserCtxt->input->base;
        This->realLine = 1;
        This->realColumn = 1;
    }

    if(This->pParserCtxt->input->cur<This->lastCur)
    {
        This->lastCur = (xmlChar*)This->pParserCtxt->input->base;
        This->realLine -= 1;
        This->realColumn = 1;
    }

    if(!end) end = (xmlChar*)This->pParserCtxt->input->cur;

    while(This->lastCur < end)
    {
        if(*(This->lastCur) == '\n')
        {
            This->realLine++;
            This->realColumn = 1;
        }
        else if(*(This->lastCur) == '\r' &&
                (This->lastCur==This->pParserCtxt->input->end ||
                 *(This->lastCur+1)!='\n'))
        {
            This->realLine++;
            This->realColumn = 1;
        }
        else This->realColumn++;

        This->lastCur++;

        /* Count multibyte UTF8 encoded characters once */
        while((*(This->lastCur)&0xC0) == 0x80) This->lastCur++;
    }

    This->line = This->realLine;
    This->column = This->realColumn;
}

/*** IVBSAXAttributes interface ***/
/*** IUnknown methods ***/
static HRESULT WINAPI ivbsaxattributes_QueryInterface(
        IVBSAXAttributes* iface,
        REFIID riid,
        void **ppvObject)
{
    saxattributes *This = impl_from_IVBSAXAttributes(iface);

    TRACE("%p %s %p\n", This, debugstr_guid(riid), ppvObject);

    *ppvObject = NULL;

    if (IsEqualGUID(riid, &IID_IUnknown) ||
            IsEqualGUID(riid, &IID_IDispatch) ||
            IsEqualGUID(riid, &IID_IVBSAXAttributes))
    {
        *ppvObject = iface;
    }
    else
    {
        FIXME("interface %s not implemented\n", debugstr_guid(riid));
        return E_NOINTERFACE;
    }

    IVBSAXAttributes_AddRef(iface);

    return S_OK;
}

static ULONG WINAPI ivbsaxattributes_AddRef(IVBSAXAttributes* iface)
{
    saxattributes *This = impl_from_IVBSAXAttributes(iface);
    return ISAXAttributes_AddRef((ISAXAttributes*)&This->lpSAXAttributesVtbl);
}

static ULONG WINAPI ivbsaxattributes_Release(IVBSAXAttributes* iface)
{
    saxattributes *This = impl_from_IVBSAXAttributes(iface);
    return ISAXAttributes_Release((ISAXAttributes*)&This->lpSAXAttributesVtbl);
}

/*** IDispatch methods ***/
static HRESULT WINAPI ivbsaxattributes_GetTypeInfoCount( IVBSAXAttributes *iface, UINT* pctinfo )
{
    saxattributes *This = impl_from_IVBSAXAttributes( iface );

    TRACE("(%p)->(%p)\n", This, pctinfo);

    *pctinfo = 1;

    return S_OK;
}

static HRESULT WINAPI ivbsaxattributes_GetTypeInfo(
    IVBSAXAttributes *iface,
    UINT iTInfo, LCID lcid, ITypeInfo** ppTInfo )
{
    saxattributes *This = impl_from_IVBSAXAttributes( iface );
    HRESULT hr;

    TRACE("(%p)->(%u %u %p)\n", This, iTInfo, lcid, ppTInfo);

    hr = get_typeinfo(IVBSAXAttributes_tid, ppTInfo);

    return hr;
}

static HRESULT WINAPI ivbsaxattributes_GetIDsOfNames(
    IVBSAXAttributes *iface,
    REFIID riid,
    LPOLESTR* rgszNames,
    UINT cNames,
    LCID lcid,
    DISPID* rgDispId)
{
    saxattributes *This = impl_from_IVBSAXAttributes( iface );
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("(%p)->(%s %p %u %u %p)\n", This, debugstr_guid(riid), rgszNames, cNames,
          lcid, rgDispId);

    if(!rgszNames || cNames == 0 || !rgDispId)
        return E_INVALIDARG;

    hr = get_typeinfo(IVBSAXAttributes_tid, &typeinfo);
    if(SUCCEEDED(hr))
    {
        hr = ITypeInfo_GetIDsOfNames(typeinfo, rgszNames, cNames, rgDispId);
        ITypeInfo_Release(typeinfo);
    }

    return hr;
}

static HRESULT WINAPI ivbsaxattributes_Invoke(
    IVBSAXAttributes *iface,
    DISPID dispIdMember,
    REFIID riid,
    LCID lcid,
    WORD wFlags,
    DISPPARAMS* pDispParams,
    VARIANT* pVarResult,
    EXCEPINFO* pExcepInfo,
    UINT* puArgErr)
{
    saxattributes *This = impl_from_IVBSAXAttributes( iface );
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("(%p)->(%d %s %d %d %p %p %p %p)\n", This, dispIdMember, debugstr_guid(riid),
          lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);

    hr = get_typeinfo(IVBSAXAttributes_tid, &typeinfo);
    if(SUCCEEDED(hr))
    {
        hr = ITypeInfo_Invoke(typeinfo, &(This->lpVBSAXAttributesVtbl), dispIdMember, wFlags, pDispParams,
                pVarResult, pExcepInfo, puArgErr);
        ITypeInfo_Release(typeinfo);
    }

    return hr;
}

/*** IVBSAXAttributes methods ***/
static HRESULT WINAPI ivbsaxattributes_get_length(
        IVBSAXAttributes* iface,
        int *nLength)
{
    saxattributes *This = impl_from_IVBSAXAttributes( iface );
    return ISAXAttributes_getLength(
            (ISAXAttributes*)&This->lpSAXAttributesVtbl,
            nLength);
}

static HRESULT WINAPI ivbsaxattributes_getURI(
        IVBSAXAttributes* iface,
        int nIndex,
        BSTR *uri)
{
    int len;
    saxattributes *This = impl_from_IVBSAXAttributes( iface );
    return ISAXAttributes_getURI(
            (ISAXAttributes*)&This->lpSAXAttributesVtbl,
            nIndex, (const WCHAR**)uri, &len);
}

static HRESULT WINAPI ivbsaxattributes_getLocalName(
        IVBSAXAttributes* iface,
        int nIndex,
        BSTR *localName)
{
    int len;
    saxattributes *This = impl_from_IVBSAXAttributes( iface );
    return ISAXAttributes_getLocalName(
            (ISAXAttributes*)&This->lpSAXAttributesVtbl,
            nIndex, (const WCHAR**)localName, &len);
}

static HRESULT WINAPI ivbsaxattributes_getQName(
        IVBSAXAttributes* iface,
        int nIndex,
        BSTR *QName)
{
    int len;
    saxattributes *This = impl_from_IVBSAXAttributes( iface );
    return ISAXAttributes_getQName(
            (ISAXAttributes*)&This->lpSAXAttributesVtbl,
            nIndex, (const WCHAR**)QName, &len);
}

static HRESULT WINAPI ivbsaxattributes_getIndexFromName(
        IVBSAXAttributes* iface,
        BSTR uri,
        BSTR localName,
        int *index)
{
    saxattributes *This = impl_from_IVBSAXAttributes( iface );
    return ISAXAttributes_getIndexFromName(
            (ISAXAttributes*)&This->lpSAXAttributesVtbl, uri, SysStringLen(uri),
            localName, SysStringLen(localName), index);
}

static HRESULT WINAPI ivbsaxattributes_getIndexFromQName(
        IVBSAXAttributes* iface,
        BSTR QName,
        int *index)
{
    saxattributes *This = impl_from_IVBSAXAttributes( iface );
    return ISAXAttributes_getIndexFromQName(
            (ISAXAttributes*)&This->lpSAXAttributesVtbl, QName,
            SysStringLen(QName), index);
}

static HRESULT WINAPI ivbsaxattributes_getType(
        IVBSAXAttributes* iface,
        int nIndex,
        BSTR *type)
{
    int len;
    saxattributes *This = impl_from_IVBSAXAttributes( iface );
    return ISAXAttributes_getType(
            (ISAXAttributes*)&This->lpSAXAttributesVtbl,
            nIndex, (const WCHAR**)type, &len);
}

static HRESULT WINAPI ivbsaxattributes_getTypeFromName(
        IVBSAXAttributes* iface,
        BSTR uri,
        BSTR localName,
        BSTR *type)
{
    int len;
    saxattributes *This = impl_from_IVBSAXAttributes( iface );
    return ISAXAttributes_getTypeFromName(
            (ISAXAttributes*)&This->lpSAXAttributesVtbl, uri, SysStringLen(uri),
            localName, SysStringLen(localName), (const WCHAR**)type, &len);
}

static HRESULT WINAPI ivbsaxattributes_getTypeFromQName(
        IVBSAXAttributes* iface,
        BSTR QName,
        BSTR *type)
{
    int len;
    saxattributes *This = impl_from_IVBSAXAttributes( iface );
    return ISAXAttributes_getTypeFromQName(
            (ISAXAttributes*)&This->lpSAXAttributesVtbl, QName,
            SysStringLen(QName), (const WCHAR**)type, &len);
}

static HRESULT WINAPI ivbsaxattributes_getValue(
        IVBSAXAttributes* iface,
        int nIndex,
        BSTR *value)
{
    int len;
    saxattributes *This = impl_from_IVBSAXAttributes( iface );
    return ISAXAttributes_getValue(
            (ISAXAttributes*)&This->lpSAXAttributesVtbl,
            nIndex, (const WCHAR**)value, &len);
}

static HRESULT WINAPI ivbsaxattributes_getValueFromName(
        IVBSAXAttributes* iface,
        BSTR uri,
        BSTR localName,
        BSTR *value)
{
    int len;
    saxattributes *This = impl_from_IVBSAXAttributes( iface );
    return ISAXAttributes_getValueFromName(
            (ISAXAttributes*)&This->lpSAXAttributesVtbl, uri, SysStringLen(uri),
            localName, SysStringLen(localName), (const WCHAR**)value, &len);
}

static HRESULT WINAPI ivbsaxattributes_getValueFromQName(
        IVBSAXAttributes* iface,
        BSTR QName,
        BSTR *value)
{
    int len;
    saxattributes *This = impl_from_IVBSAXAttributes( iface );
    return ISAXAttributes_getValueFromQName(
            (ISAXAttributes*)&This->lpSAXAttributesVtbl, QName,
            SysStringLen(QName), (const WCHAR**)value, &len);
}

static const struct IVBSAXAttributesVtbl ivbsaxattributes_vtbl =
{
    ivbsaxattributes_QueryInterface,
    ivbsaxattributes_AddRef,
    ivbsaxattributes_Release,
    ivbsaxattributes_GetTypeInfoCount,
    ivbsaxattributes_GetTypeInfo,
    ivbsaxattributes_GetIDsOfNames,
    ivbsaxattributes_Invoke,
    ivbsaxattributes_get_length,
    ivbsaxattributes_getURI,
    ivbsaxattributes_getLocalName,
    ivbsaxattributes_getQName,
    ivbsaxattributes_getIndexFromName,
    ivbsaxattributes_getIndexFromQName,
    ivbsaxattributes_getType,
    ivbsaxattributes_getTypeFromName,
    ivbsaxattributes_getTypeFromQName,
    ivbsaxattributes_getValue,
    ivbsaxattributes_getValueFromName,
    ivbsaxattributes_getValueFromQName
};

/*** ISAXAttributes interface ***/
/*** IUnknown methods ***/
static HRESULT WINAPI isaxattributes_QueryInterface(
        ISAXAttributes* iface,
        REFIID riid,
        void **ppvObject)
{
    saxattributes *This = impl_from_ISAXAttributes(iface);

    TRACE("%p %s %p\n", This, debugstr_guid(riid), ppvObject);

    *ppvObject = NULL;

    if (IsEqualGUID(riid, &IID_IUnknown) ||
            IsEqualGUID(riid, &IID_ISAXAttributes))
    {
        *ppvObject = iface;
    }
    else
    {
        FIXME("interface %s not implemented\n", debugstr_guid(riid));
        return E_NOINTERFACE;
    }

    ISAXAttributes_AddRef(iface);

    return S_OK;
}

static ULONG WINAPI isaxattributes_AddRef(ISAXAttributes* iface)
{
    saxattributes *This = impl_from_ISAXAttributes(iface);
    TRACE("%p\n", This);
    return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI isaxattributes_Release(ISAXAttributes* iface)
{
    saxattributes *This = impl_from_ISAXAttributes(iface);
    LONG ref;

    TRACE("%p\n", This);

    ref = InterlockedDecrement(&This->ref);
    if (ref==0)
    {
        int index;
        for(index=0; index<This->nb_attributes; index++)
        {
            SysFreeString(This->szLocalname[index]);
            SysFreeString(This->szURI[index]);
            SysFreeString(This->szValue[index]);
            SysFreeString(This->szQName[index]);
        }

        HeapFree(GetProcessHeap(), 0, This->szLocalname);
        HeapFree(GetProcessHeap(), 0, This->szURI);
        HeapFree(GetProcessHeap(), 0, This->szValue);
        HeapFree(GetProcessHeap(), 0, This->szQName);

        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}

/*** ISAXAttributes methods ***/
static HRESULT WINAPI isaxattributes_getLength(
        ISAXAttributes* iface,
        int *length)
{
    saxattributes *This = impl_from_ISAXAttributes( iface );

    *length = This->nb_attributes;
    TRACE("Length set to %d\n", *length);
    return S_OK;
}

static HRESULT WINAPI isaxattributes_getURI(
        ISAXAttributes* iface,
        int nIndex,
        const WCHAR **pUrl,
        int *pUriSize)
{
    saxattributes *This = impl_from_ISAXAttributes( iface );
    TRACE("(%p)->(%d)\n", This, nIndex);

    if(nIndex>=This->nb_attributes || nIndex<0) return E_INVALIDARG;
    if(!pUrl || !pUriSize) return E_POINTER;

    *pUriSize = SysStringLen(This->szURI[nIndex]);
    *pUrl = This->szURI[nIndex];

    return S_OK;
}

static HRESULT WINAPI isaxattributes_getLocalName(
        ISAXAttributes* iface,
        int nIndex,
        const WCHAR **pLocalName,
        int *pLocalNameLength)
{
    saxattributes *This = impl_from_ISAXAttributes( iface );
    TRACE("(%p)->(%d)\n", This, nIndex);

    if(nIndex>=This->nb_attributes || nIndex<0) return E_INVALIDARG;
    if(!pLocalName || !pLocalNameLength) return E_POINTER;

    *pLocalNameLength = SysStringLen(This->szLocalname[nIndex]);
    *pLocalName = This->szLocalname[nIndex];

    return S_OK;
}

static HRESULT WINAPI isaxattributes_getQName(
        ISAXAttributes* iface,
        int nIndex,
        const WCHAR **pQName,
        int *pQNameLength)
{
    saxattributes *This = impl_from_ISAXAttributes( iface );
    TRACE("(%p)->(%d)\n", This, nIndex);

    if(nIndex>=This->nb_attributes || nIndex<0) return E_INVALIDARG;
    if(!pQName || !pQNameLength) return E_POINTER;

    *pQNameLength = SysStringLen(This->szQName[nIndex]);
    *pQName = This->szQName[nIndex];

    return S_OK;
}

static HRESULT WINAPI isaxattributes_getName(
        ISAXAttributes* iface,
        int nIndex,
        const WCHAR **pUri,
        int *pUriLength,
        const WCHAR **pLocalName,
        int *pLocalNameSize,
        const WCHAR **pQName,
        int *pQNameLength)
{
    saxattributes *This = impl_from_ISAXAttributes( iface );
    TRACE("(%p)->(%d)\n", This, nIndex);

    if(nIndex>=This->nb_attributes || nIndex<0) return E_INVALIDARG;
    if(!pUri || !pUriLength || !pLocalName || !pLocalNameSize
            || !pQName || !pQNameLength) return E_POINTER;

    *pUriLength = SysStringLen(This->szURI[nIndex]);
    *pUri = This->szURI[nIndex];
    *pLocalNameSize = SysStringLen(This->szLocalname[nIndex]);
    *pLocalName = This->szLocalname[nIndex];
    *pQNameLength = SysStringLen(This->szQName[nIndex]);
    *pQName = This->szQName[nIndex];

    return S_OK;
}

static HRESULT WINAPI isaxattributes_getIndexFromName(
        ISAXAttributes* iface,
        const WCHAR *pUri,
        int cUriLength,
        const WCHAR *pLocalName,
        int cocalNameLength,
        int *index)
{
    saxattributes *This = impl_from_ISAXAttributes( iface );
    int i;
    TRACE("(%p)->(%s, %d, %s, %d)\n", This, debugstr_w(pUri), cUriLength,
            debugstr_w(pLocalName), cocalNameLength);

    if(!pUri || !pLocalName || !index) return E_POINTER;

    for(i=0; i<This->nb_attributes; i++)
    {
        if(cUriLength!=SysStringLen(This->szURI[i])
                || cocalNameLength!=SysStringLen(This->szLocalname[i]))
            continue;
        if(cUriLength && memcmp(pUri, This->szURI[i],
                    sizeof(WCHAR)*cUriLength))
            continue;
        if(cocalNameLength && memcmp(pLocalName, This->szLocalname[i],
                    sizeof(WCHAR)*cocalNameLength))
            continue;

        *index = i;
        return S_OK;
    }

    return E_INVALIDARG;
}

static HRESULT WINAPI isaxattributes_getIndexFromQName(
        ISAXAttributes* iface,
        const WCHAR *pQName,
        int nQNameLength,
        int *index)
{
    saxattributes *This = impl_from_ISAXAttributes( iface );
    int i;
    TRACE("(%p)->(%s, %d)\n", This, debugstr_w(pQName), nQNameLength);

    if(!pQName || !index) return E_POINTER;
    if(!nQNameLength) return E_INVALIDARG;

    for(i=0; i<This->nb_attributes; i++)
    {
        if(nQNameLength!=SysStringLen(This->szQName[i])) continue;
        if(memcmp(pQName, This->szQName, sizeof(WCHAR)*nQNameLength)) continue;

        *index = i;
        return S_OK;
    }

    return E_INVALIDARG;
}

static HRESULT WINAPI isaxattributes_getType(
        ISAXAttributes* iface,
        int nIndex,
        const WCHAR **pType,
        int *pTypeLength)
{
    saxattributes *This = impl_from_ISAXAttributes( iface );

    FIXME("(%p)->(%d) stub\n", This, nIndex);
    return E_NOTIMPL;
}

static HRESULT WINAPI isaxattributes_getTypeFromName(
        ISAXAttributes* iface,
        const WCHAR *pUri,
        int nUri,
        const WCHAR *pLocalName,
        int nLocalName,
        const WCHAR **pType,
        int *nType)
{
    saxattributes *This = impl_from_ISAXAttributes( iface );

    FIXME("(%p)->(%s, %d, %s, %d) stub\n", This, debugstr_w(pUri), nUri,
            debugstr_w(pLocalName), nLocalName);
    return E_NOTIMPL;
}

static HRESULT WINAPI isaxattributes_getTypeFromQName(
        ISAXAttributes* iface,
        const WCHAR *pQName,
        int nQName,
        const WCHAR **pType,
        int *nType)
{
    saxattributes *This = impl_from_ISAXAttributes( iface );

    FIXME("(%p)->(%s, %d) stub\n", This, debugstr_w(pQName), nQName);
    return E_NOTIMPL;
}

static HRESULT WINAPI isaxattributes_getValue(
        ISAXAttributes* iface,
        int nIndex,
        const WCHAR **pValue,
        int *nValue)
{
    saxattributes *This = impl_from_ISAXAttributes( iface );
    TRACE("(%p)->(%d)\n", This, nIndex);

    if(nIndex>=This->nb_attributes || nIndex<0) return E_INVALIDARG;
    if(!pValue || !nValue) return E_POINTER;

    *nValue = SysStringLen(This->szValue[nIndex]);
    *pValue = This->szValue[nIndex];

    return S_OK;
}

static HRESULT WINAPI isaxattributes_getValueFromName(
        ISAXAttributes* iface,
        const WCHAR *pUri,
        int nUri,
        const WCHAR *pLocalName,
        int nLocalName,
        const WCHAR **pValue,
        int *nValue)
{
    HRESULT hr;
    int index;
    saxattributes *This = impl_from_ISAXAttributes( iface );
    TRACE("(%p)->(%s, %d, %s, %d)\n", This, debugstr_w(pUri), nUri,
            debugstr_w(pLocalName), nLocalName);

    hr = ISAXAttributes_getIndexFromName(iface,
            pUri, nUri, pLocalName, nLocalName, &index);
    if(hr==S_OK) hr = ISAXAttributes_getValue(iface, index, pValue, nValue);

    return hr;
}

static HRESULT WINAPI isaxattributes_getValueFromQName(
        ISAXAttributes* iface,
        const WCHAR *pQName,
        int nQName,
        const WCHAR **pValue,
        int *nValue)
{
    HRESULT hr;
    int index;
    saxattributes *This = impl_from_ISAXAttributes( iface );
    TRACE("(%p)->(%s, %d)\n", This, debugstr_w(pQName), nQName);

    hr = ISAXAttributes_getIndexFromQName(iface, pQName, nQName, &index);
    if(hr==S_OK) hr = ISAXAttributes_getValue(iface, index, pValue, nValue);

    return hr;
}

static const struct ISAXAttributesVtbl isaxattributes_vtbl =
{
    isaxattributes_QueryInterface,
    isaxattributes_AddRef,
    isaxattributes_Release,
    isaxattributes_getLength,
    isaxattributes_getURI,
    isaxattributes_getLocalName,
    isaxattributes_getQName,
    isaxattributes_getName,
    isaxattributes_getIndexFromName,
    isaxattributes_getIndexFromQName,
    isaxattributes_getType,
    isaxattributes_getTypeFromName,
    isaxattributes_getTypeFromQName,
    isaxattributes_getValue,
    isaxattributes_getValueFromName,
    isaxattributes_getValueFromQName
};

static HRESULT SAXAttributes_create(saxattributes **attr,
        int nb_namespaces, const xmlChar **xmlNamespaces,
        int nb_attributes, const xmlChar **xmlAttributes)
{
    saxattributes *attributes;
    int index;
    static const xmlChar xmlns[] = "xmlns";

    attributes = HeapAlloc(GetProcessHeap(), 0, sizeof(*attributes));
    if(!attributes)
        return E_OUTOFMEMORY;

    attributes->lpVBSAXAttributesVtbl = &ivbsaxattributes_vtbl;
    attributes->lpSAXAttributesVtbl = &isaxattributes_vtbl;
    attributes->ref = 1;

    attributes->nb_attributes = nb_namespaces+nb_attributes;

    attributes->szLocalname =
        HeapAlloc(GetProcessHeap(), 0, sizeof(BSTR)*attributes->nb_attributes);
    attributes->szURI =
        HeapAlloc(GetProcessHeap(), 0, sizeof(BSTR)*attributes->nb_attributes);
    attributes->szValue =
        HeapAlloc(GetProcessHeap(), 0, sizeof(BSTR)*attributes->nb_attributes);
    attributes->szQName =
        HeapAlloc(GetProcessHeap(), 0, sizeof(BSTR)*attributes->nb_attributes);

    if(!attributes->szLocalname || !attributes->szURI
            || !attributes->szValue || !attributes->szQName)
    {
        HeapFree(GetProcessHeap(), 0, attributes->szLocalname);
        HeapFree(GetProcessHeap(), 0, attributes->szURI);
        HeapFree(GetProcessHeap(), 0, attributes->szValue);
        HeapFree(GetProcessHeap(), 0, attributes->szQName);
        HeapFree(GetProcessHeap(), 0, attributes);
        return E_FAIL;
    }

    for(index=0; index<nb_namespaces; index++)
    {
        attributes->szLocalname[index] = SysAllocStringLen(NULL, 0);
        attributes->szURI[index] = SysAllocStringLen(NULL, 0);
        attributes->szValue[index] = bstr_from_xmlChar(xmlNamespaces[2*index+1]);
        attributes->szQName[index] = QName_from_xmlChar(xmlns, xmlNamespaces[2*index]);
    }

    for(index=0; index<nb_attributes; index++)
    {
        attributes->szLocalname[nb_namespaces+index] =
            bstr_from_xmlChar(xmlAttributes[index*5]);
        attributes->szURI[nb_namespaces+index] =
            bstr_from_xmlChar(xmlAttributes[index*5+2]);
        attributes->szValue[nb_namespaces+index] =
            bstr_from_xmlCharN(xmlAttributes[index*5+3],
                    xmlAttributes[index*5+4]-xmlAttributes[index*5+3]);
        attributes->szQName[nb_namespaces+index] =
            QName_from_xmlChar(xmlAttributes[index*5+1], xmlAttributes[index*5]);
    }

    *attr = attributes;

    TRACE("returning %p\n", *attr);

    return S_OK;
}

/*** LibXML callbacks ***/
static void libxmlStartDocument(void *ctx)
{
    saxlocator *This = ctx;
    HRESULT hr;

    if((This->vbInterface && This->saxreader->vbcontentHandler)
            || (!This->vbInterface && This->saxreader->contentHandler))
    {
        if(This->vbInterface)
            hr = IVBSAXContentHandler_startDocument(This->saxreader->vbcontentHandler);
        else
            hr = ISAXContentHandler_startDocument(This->saxreader->contentHandler);

        if(hr != S_OK)
            format_error_message_from_id(This, hr);
    }

    update_position(This, NULL);
}

static void libxmlEndDocument(void *ctx)
{
    saxlocator *This = ctx;
    HRESULT hr;

    This->column = 0;
    This->line = 0;

    if(This->ret != S_OK) return;

    if((This->vbInterface && This->saxreader->vbcontentHandler)
            || (!This->vbInterface && This->saxreader->contentHandler))
    {
        if(This->vbInterface)
            hr = IVBSAXContentHandler_endDocument(This->saxreader->vbcontentHandler);
        else
            hr = ISAXContentHandler_endDocument(This->saxreader->contentHandler);

        if(hr != S_OK)
            format_error_message_from_id(This, hr);
    }
}

static void libxmlStartElementNS(
        void *ctx,
        const xmlChar *localname,
        const xmlChar *prefix,
        const xmlChar *URI,
        int nb_namespaces,
        const xmlChar **namespaces,
        int nb_attributes,
        int nb_defaulted,
        const xmlChar **attributes)
{
    BSTR NamespaceUri, LocalName, QName, Prefix, Uri;
    saxlocator *This = ctx;
    HRESULT hr;
    saxattributes *attr;
    int index;

    if(*(This->pParserCtxt->input->cur) == '/')
        update_position(This, (xmlChar*)This->pParserCtxt->input->cur+2);
    else
        update_position(This, (xmlChar*)This->pParserCtxt->input->cur+1);

    hr = namespacePush(This, nb_namespaces);
    if(hr==S_OK && ((This->vbInterface && This->saxreader->vbcontentHandler)
                || (!This->vbInterface && This->saxreader->contentHandler)))
    {
        for(index=0; index<nb_namespaces; index++)
        {
            Prefix = bstr_from_xmlChar(namespaces[2*index]);
            Uri = bstr_from_xmlChar(namespaces[2*index+1]);

            if(This->vbInterface)
                hr = IVBSAXContentHandler_startPrefixMapping(
                        This->saxreader->vbcontentHandler,
                        &Prefix, &Uri);
            else
                hr = ISAXContentHandler_startPrefixMapping(
                        This->saxreader->contentHandler,
                        Prefix, SysStringLen(Prefix),
                        Uri, SysStringLen(Uri));

            SysFreeString(Prefix);
            SysFreeString(Uri);

            if(hr != S_OK)
            {
                format_error_message_from_id(This, hr);
                return;
            }
        }

        NamespaceUri = bstr_from_xmlChar(URI);
        LocalName = bstr_from_xmlChar(localname);
        QName = QName_from_xmlChar(prefix, localname);

        hr = SAXAttributes_create(&attr, nb_namespaces, namespaces, nb_attributes, attributes);
        if(hr == S_OK)
        {
            if(This->vbInterface)
                hr = IVBSAXContentHandler_startElement(
                        This->saxreader->vbcontentHandler,
                        &NamespaceUri, &LocalName, &QName,
                        (IVBSAXAttributes*)&attr->lpVBSAXAttributesVtbl);
            else
                hr = ISAXContentHandler_startElement(
                        This->saxreader->contentHandler,
                        NamespaceUri, SysStringLen(NamespaceUri),
                        LocalName, SysStringLen(LocalName),
                        QName, SysStringLen(QName),
                        (ISAXAttributes*)&attr->lpSAXAttributesVtbl);

            ISAXAttributes_Release((ISAXAttributes*)&attr->lpSAXAttributesVtbl);
        }

        SysFreeString(NamespaceUri);
        SysFreeString(LocalName);
        SysFreeString(QName);
    }

    if(hr != S_OK)
        format_error_message_from_id(This, hr);
}

static void libxmlEndElementNS(
        void *ctx,
        const xmlChar *localname,
        const xmlChar *prefix,
        const xmlChar *URI)
{
    BSTR NamespaceUri, LocalName, QName, Prefix;
    saxlocator *This = ctx;
    HRESULT hr;
    xmlChar *end;
    int nsNr, index;

    end = (xmlChar*)This->pParserCtxt->input->cur;
    if(*(end-1) != '>' || *(end-2) != '/')
        while(*(end-2)!='<' && *(end-1)!='/') end--;

    update_position(This, end);

    nsNr = namespacePop(This);

    if((This->vbInterface && This->saxreader->vbcontentHandler)
            || (!This->vbInterface && This->saxreader->contentHandler))
    {
        NamespaceUri = bstr_from_xmlChar(URI);
        LocalName = bstr_from_xmlChar(localname);
        QName = QName_from_xmlChar(prefix, localname);

        if(This->vbInterface)
            hr = IVBSAXContentHandler_endElement(
                    This->saxreader->vbcontentHandler,
                    &NamespaceUri, &LocalName, &QName);
        else
            hr = ISAXContentHandler_endElement(
                    This->saxreader->contentHandler,
                    NamespaceUri, SysStringLen(NamespaceUri),
                    LocalName, SysStringLen(LocalName),
                    QName, SysStringLen(QName));

        SysFreeString(NamespaceUri);
        SysFreeString(LocalName);
        SysFreeString(QName);

        if(hr != S_OK)
        {
            format_error_message_from_id(This, hr);
            return;
        }

        for(index=This->pParserCtxt->nsNr-2;
                index>=This->pParserCtxt->nsNr-nsNr*2; index-=2)
        {
            Prefix = bstr_from_xmlChar(This->pParserCtxt->nsTab[index]);

            if(This->vbInterface)
                hr = IVBSAXContentHandler_endPrefixMapping(
                        This->saxreader->vbcontentHandler, &Prefix);
            else
                hr = ISAXContentHandler_endPrefixMapping(
                        This->saxreader->contentHandler,
                        Prefix, SysStringLen(Prefix));

            SysFreeString(Prefix);

            if(hr != S_OK)
            {
                format_error_message_from_id(This, hr);
                return;
            }

        }
    }

    update_position(This, NULL);
}

static void libxmlCharacters(
        void *ctx,
        const xmlChar *ch,
        int len)
{
    saxlocator *This = ctx;
    BSTR Chars;
    HRESULT hr;
    xmlChar *cur;
    xmlChar *end;
    BOOL lastEvent = FALSE;

    if((This->vbInterface && !This->saxreader->vbcontentHandler)
            || (!This->vbInterface && !This->saxreader->contentHandler))
        return;

    cur = (xmlChar*)ch;
    if(*(ch-1)=='\r') cur--;
    end = cur;

    if(ch<This->pParserCtxt->input->base || ch>This->pParserCtxt->input->end)
        This->column++;

    while(1)
    {
        while(end-ch<len && *end!='\r') end++;
        if(end-ch==len)
        {
            end--;
            lastEvent = TRUE;
        }

        if(!lastEvent) *end = '\n';

        Chars = bstr_from_xmlCharN(cur, end-cur+1);
        if(This->vbInterface)
            hr = IVBSAXContentHandler_characters(
                    This->saxreader->vbcontentHandler, &Chars);
        else
            hr = ISAXContentHandler_characters(
                    This->saxreader->contentHandler,
                    Chars, SysStringLen(Chars));
        SysFreeString(Chars);

        if(hr != S_OK)
        {
            format_error_message_from_id(This, hr);
            return;
        }

        This->column += end-cur+1;

        if(lastEvent)
            break;

        *end = '\r';
        end++;
        if(*end == '\n')
        {
            end++;
            This->column++;
        }
        cur = end;

        if(end-ch == len) break;
    }

    if(ch<This->pParserCtxt->input->base || ch>This->pParserCtxt->input->end)
        This->column = This->realColumn
            +This->pParserCtxt->input->cur-This->lastCur;
}

static void libxmlSetDocumentLocator(
        void *ctx,
        xmlSAXLocatorPtr loc)
{
    saxlocator *This = ctx;
    HRESULT hr;

    if(This->vbInterface)
        hr = IVBSAXContentHandler_putref_documentLocator(
                This->saxreader->vbcontentHandler,
                (IVBSAXLocator*)&This->lpVBSAXLocatorVtbl);
    else
        hr = ISAXContentHandler_putDocumentLocator(
                This->saxreader->contentHandler,
                (ISAXLocator*)&This->lpSAXLocatorVtbl);

    if(FAILED(hr))
        format_error_message_from_id(This, hr);
}

static void libxmlComment(void *ctx, const xmlChar *value)
{
    saxlocator *This = ctx;
    BSTR bValue;
    HRESULT hr;
    xmlChar *beg = (xmlChar*)This->pParserCtxt->input->cur;

    while(memcmp(beg-4, "<!--", sizeof(char[4]))) beg--;
    update_position(This, beg);

    if(!This->vbInterface && !This->saxreader->lexicalHandler) return;
    if(This->vbInterface && !This->saxreader->vblexicalHandler) return;

    bValue = bstr_from_xmlChar(value);

    if(This->vbInterface)
        hr = IVBSAXLexicalHandler_comment(
                This->saxreader->vblexicalHandler, &bValue);
    else
        hr = ISAXLexicalHandler_comment(
                This->saxreader->lexicalHandler,
                bValue, SysStringLen(bValue));

    SysFreeString(bValue);

    if(FAILED(hr))
        format_error_message_from_id(This, hr);

    update_position(This, NULL);
}

static void libxmlFatalError(void *ctx, const char *msg, ...)
{
    saxlocator *This = ctx;
    char message[1024];
    WCHAR *wszError;
    DWORD len;
    va_list args;

    if((This->vbInterface && !This->saxreader->vberrorHandler)
            || (!This->vbInterface && !This->saxreader->errorHandler))
    {
        xmlStopParser(This->pParserCtxt);
        This->ret = E_FAIL;
        return;
    }

    FIXME("Error handling is not compatible.\n");

    va_start(args, msg);
    vsprintf(message, msg, args);
    va_end(args);

    len = MultiByteToWideChar(CP_UNIXCP, 0, message, -1, NULL, 0);
    wszError = HeapAlloc(GetProcessHeap(), 0, sizeof(WCHAR)*len);
    if(wszError)
        MultiByteToWideChar(CP_UNIXCP, 0, message, -1, wszError, len);

    if(This->vbInterface)
    {
        BSTR bstrError = SysAllocString(wszError);
        IVBSAXErrorHandler_fatalError(This->saxreader->vberrorHandler,
                (IVBSAXLocator*)&This->lpVBSAXLocatorVtbl, &bstrError, E_FAIL);
    }
    else
        ISAXErrorHandler_fatalError(This->saxreader->errorHandler,
                (ISAXLocator*)&This->lpSAXLocatorVtbl, wszError, E_FAIL);

    HeapFree(GetProcessHeap(), 0, wszError);

    xmlStopParser(This->pParserCtxt);
    This->ret = E_FAIL;
}

static void libxmlCDataBlock(void *ctx, const xmlChar *value, int len)
{
    saxlocator *This = ctx;
    HRESULT hr = S_OK;
    xmlChar *beg = (xmlChar*)This->pParserCtxt->input->cur-len;
    xmlChar *cur, *end;
    int realLen;
    BSTR Chars;
    BOOL lastEvent = FALSE, change;

    while(memcmp(beg-9, "<![CDATA[", sizeof(char[9]))) beg--;
    update_position(This, beg);

    if(This->vbInterface && This->saxreader->vblexicalHandler)
        hr = IVBSAXLexicalHandler_startCDATA(This->saxreader->vblexicalHandler);
    if(!This->vbInterface && This->saxreader->lexicalHandler)
        hr = ISAXLexicalHandler_startCDATA(This->saxreader->lexicalHandler);

    if(FAILED(hr))
    {
        format_error_message_from_id(This, hr);
        return;
    }

    realLen = This->pParserCtxt->input->cur-beg-3;
    cur = beg;
    end = beg;

    while(1)
    {
        while(end-beg<realLen && *end!='\r') end++;
        if(end-beg==realLen)
        {
            end--;
            lastEvent = TRUE;
        }
        else if(end-beg==realLen-1 && *end=='\r' && *(end+1)=='\n')
            lastEvent = TRUE;

        if(*end == '\r') change = TRUE;
        else change = FALSE;

        if(change) *end = '\n';

        if((This->vbInterface && This->saxreader->vbcontentHandler) ||
                (!This->vbInterface && This->saxreader->contentHandler))
        {
            Chars = bstr_from_xmlCharN(cur, end-cur+1);
            if(This->vbInterface)
                hr = IVBSAXContentHandler_characters(
                        This->saxreader->vbcontentHandler, &Chars);
            else
                hr = ISAXContentHandler_characters(
                        This->saxreader->contentHandler,
                        Chars, SysStringLen(Chars));
            SysFreeString(Chars);
        }

        if(change) *end = '\r';

        if(lastEvent)
            break;

        This->column += end-cur+2;
        end += 2;
        cur = end;
    }

    if(This->vbInterface && This->saxreader->vblexicalHandler)
        hr = IVBSAXLexicalHandler_endCDATA(This->saxreader->vblexicalHandler);
    if(!This->vbInterface && This->saxreader->lexicalHandler)
        hr = ISAXLexicalHandler_endCDATA(This->saxreader->lexicalHandler);

    if(FAILED(hr))
        format_error_message_from_id(This, hr);

    This->column += 4+end-cur;
}

/*** IVBSAXLocator interface ***/
/*** IUnknown methods ***/
static HRESULT WINAPI ivbsaxlocator_QueryInterface(IVBSAXLocator* iface, REFIID riid, void **ppvObject)
{
    saxlocator *This = impl_from_IVBSAXLocator( iface );

    TRACE("%p %s %p\n", This, debugstr_guid( riid ), ppvObject);

    *ppvObject = NULL;

    if ( IsEqualGUID( riid, &IID_IUnknown ) ||
            IsEqualGUID( riid, &IID_IDispatch) ||
            IsEqualGUID( riid, &IID_IVBSAXLocator ))
    {
        *ppvObject = iface;
    }
    else
    {
        FIXME("interface %s not implemented\n", debugstr_guid(riid));
        return E_NOINTERFACE;
    }

    IVBSAXLocator_AddRef( iface );

    return S_OK;
}

static ULONG WINAPI ivbsaxlocator_AddRef(IVBSAXLocator* iface)
{
    saxlocator *This = impl_from_IVBSAXLocator( iface );
    TRACE("%p\n", This );
    return InterlockedIncrement( &This->ref );
}

static ULONG WINAPI ivbsaxlocator_Release(
        IVBSAXLocator* iface)
{
    saxlocator *This = impl_from_IVBSAXLocator( iface );
    return ISAXLocator_Release((ISAXLocator*)&This->lpVBSAXLocatorVtbl);
}

/*** IDispatch methods ***/
static HRESULT WINAPI ivbsaxlocator_GetTypeInfoCount( IVBSAXLocator *iface, UINT* pctinfo )
{
    saxlocator *This = impl_from_IVBSAXLocator( iface );

    TRACE("(%p)->(%p)\n", This, pctinfo);

    *pctinfo = 1;

    return S_OK;
}

static HRESULT WINAPI ivbsaxlocator_GetTypeInfo(
    IVBSAXLocator *iface,
    UINT iTInfo, LCID lcid, ITypeInfo** ppTInfo )
{
    saxlocator *This = impl_from_IVBSAXLocator( iface );
    HRESULT hr;

    TRACE("(%p)->(%u %u %p)\n", This, iTInfo, lcid, ppTInfo);

    hr = get_typeinfo(IVBSAXLocator_tid, ppTInfo);

    return hr;
}

static HRESULT WINAPI ivbsaxlocator_GetIDsOfNames(
    IVBSAXLocator *iface,
    REFIID riid,
    LPOLESTR* rgszNames,
    UINT cNames,
    LCID lcid,
    DISPID* rgDispId)
{
    saxlocator *This = impl_from_IVBSAXLocator( iface );
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("(%p)->(%s %p %u %u %p)\n", This, debugstr_guid(riid), rgszNames, cNames,
          lcid, rgDispId);

    if(!rgszNames || cNames == 0 || !rgDispId)
        return E_INVALIDARG;

    hr = get_typeinfo(IVBSAXLocator_tid, &typeinfo);
    if(SUCCEEDED(hr))
    {
        hr = ITypeInfo_GetIDsOfNames(typeinfo, rgszNames, cNames, rgDispId);
        ITypeInfo_Release(typeinfo);
    }

    return hr;
}

static HRESULT WINAPI ivbsaxlocator_Invoke(
    IVBSAXLocator *iface,
    DISPID dispIdMember,
    REFIID riid,
    LCID lcid,
    WORD wFlags,
    DISPPARAMS* pDispParams,
    VARIANT* pVarResult,
    EXCEPINFO* pExcepInfo,
    UINT* puArgErr)
{
    saxlocator *This = impl_from_IVBSAXLocator( iface );
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("(%p)->(%d %s %d %d %p %p %p %p)\n", This, dispIdMember, debugstr_guid(riid),
          lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);

    hr = get_typeinfo(IVBSAXLocator_tid, &typeinfo);
    if(SUCCEEDED(hr))
    {
        hr = ITypeInfo_Invoke(typeinfo, &(This->lpVBSAXLocatorVtbl), dispIdMember, wFlags, pDispParams,
                pVarResult, pExcepInfo, puArgErr);
        ITypeInfo_Release(typeinfo);
    }

    return hr;
}

/*** IVBSAXLocator methods ***/
static HRESULT WINAPI ivbsaxlocator_get_columnNumber(
        IVBSAXLocator* iface,
        int *pnColumn)
{
    saxlocator *This = impl_from_IVBSAXLocator( iface );
    return ISAXLocator_getColumnNumber(
            (ISAXLocator*)&This->lpVBSAXLocatorVtbl,
            pnColumn);
}

static HRESULT WINAPI ivbsaxlocator_get_lineNumber(
        IVBSAXLocator* iface,
        int *pnLine)
{
    saxlocator *This = impl_from_IVBSAXLocator( iface );
    return ISAXLocator_getLineNumber(
            (ISAXLocator*)&This->lpVBSAXLocatorVtbl,
            pnLine);
}

static HRESULT WINAPI ivbsaxlocator_get_publicId(
        IVBSAXLocator* iface,
        BSTR* publicId)
{
    saxlocator *This = impl_from_IVBSAXLocator( iface );
    return ISAXLocator_getPublicId(
            (ISAXLocator*)&This->lpVBSAXLocatorVtbl,
            (const WCHAR**)publicId);
}

static HRESULT WINAPI ivbsaxlocator_get_systemId(
        IVBSAXLocator* iface,
        BSTR* systemId)
{
    saxlocator *This = impl_from_IVBSAXLocator( iface );
    return ISAXLocator_getSystemId(
            (ISAXLocator*)&This->lpVBSAXLocatorVtbl,
            (const WCHAR**)systemId);
}

static const struct IVBSAXLocatorVtbl ivbsaxlocator_vtbl =
{
    ivbsaxlocator_QueryInterface,
    ivbsaxlocator_AddRef,
    ivbsaxlocator_Release,
    ivbsaxlocator_GetTypeInfoCount,
    ivbsaxlocator_GetTypeInfo,
    ivbsaxlocator_GetIDsOfNames,
    ivbsaxlocator_Invoke,
    ivbsaxlocator_get_columnNumber,
    ivbsaxlocator_get_lineNumber,
    ivbsaxlocator_get_publicId,
    ivbsaxlocator_get_systemId
};

/*** ISAXLocator interface ***/
/*** IUnknown methods ***/
static HRESULT WINAPI isaxlocator_QueryInterface(ISAXLocator* iface, REFIID riid, void **ppvObject)
{
    saxlocator *This = impl_from_ISAXLocator( iface );

    TRACE("%p %s %p\n", This, debugstr_guid( riid ), ppvObject );

    *ppvObject = NULL;

    if ( IsEqualGUID( riid, &IID_IUnknown ) ||
            IsEqualGUID( riid, &IID_ISAXLocator ))
    {
        *ppvObject = iface;
    }
    else
    {
        FIXME("interface %s not implemented\n", debugstr_guid(riid));
        return E_NOINTERFACE;
    }

    ISAXLocator_AddRef( iface );

    return S_OK;
}

static ULONG WINAPI isaxlocator_AddRef(ISAXLocator* iface)
{
    saxlocator *This = impl_from_ISAXLocator( iface );
    TRACE("%p\n", This );
    return InterlockedIncrement( &This->ref );
}

static ULONG WINAPI isaxlocator_Release(
        ISAXLocator* iface)
{
    saxlocator *This = impl_from_ISAXLocator( iface );
    LONG ref;

    TRACE("%p\n", This );

    ref = InterlockedDecrement( &This->ref );
    if ( ref == 0 )
    {
        SysFreeString(This->publicId);
        SysFreeString(This->systemId);
        HeapFree(GetProcessHeap(), 0, This->nsStack);

        ISAXXMLReader_Release((ISAXXMLReader*)&This->saxreader->lpSAXXMLReaderVtbl);
        HeapFree( GetProcessHeap(), 0, This );
    }

    return ref;
}

/*** ISAXLocator methods ***/
static HRESULT WINAPI isaxlocator_getColumnNumber(
        ISAXLocator* iface,
        int *pnColumn)
{
    saxlocator *This = impl_from_ISAXLocator( iface );

    *pnColumn = This->column;
    return S_OK;
}

static HRESULT WINAPI isaxlocator_getLineNumber(
        ISAXLocator* iface,
        int *pnLine)
{
    saxlocator *This = impl_from_ISAXLocator( iface );

    *pnLine = This->line;
    return S_OK;
}

static HRESULT WINAPI isaxlocator_getPublicId(
        ISAXLocator* iface,
        const WCHAR ** ppwchPublicId)
{
    BSTR publicId;
    saxlocator *This = impl_from_ISAXLocator( iface );

    SysFreeString(This->publicId);

    publicId = bstr_from_xmlChar(xmlSAX2GetPublicId(This->pParserCtxt));
    if(SysStringLen(publicId))
        This->publicId = (WCHAR*)&publicId;
    else
    {
        SysFreeString(publicId);
        This->publicId = NULL;
    }

    *ppwchPublicId = This->publicId;
    return S_OK;
}

static HRESULT WINAPI isaxlocator_getSystemId(
        ISAXLocator* iface,
        const WCHAR ** ppwchSystemId)
{
    BSTR systemId;
    saxlocator *This = impl_from_ISAXLocator( iface );

    SysFreeString(This->systemId);

    systemId = bstr_from_xmlChar(xmlSAX2GetSystemId(This->pParserCtxt));
    if(SysStringLen(systemId))
        This->systemId = (WCHAR*)&systemId;
    else
    {
        SysFreeString(systemId);
        This->systemId = NULL;
    }

    *ppwchSystemId = This->systemId;
    return S_OK;
}

static const struct ISAXLocatorVtbl isaxlocator_vtbl =
{
    isaxlocator_QueryInterface,
    isaxlocator_AddRef,
    isaxlocator_Release,
    isaxlocator_getColumnNumber,
    isaxlocator_getLineNumber,
    isaxlocator_getPublicId,
    isaxlocator_getSystemId
};

static HRESULT SAXLocator_create(saxreader *reader, saxlocator **ppsaxlocator, BOOL vbInterface)
{
    saxlocator *locator;

    locator = HeapAlloc( GetProcessHeap(), 0, sizeof (*locator) );
    if( !locator )
        return E_OUTOFMEMORY;

    locator->lpVBSAXLocatorVtbl = &ivbsaxlocator_vtbl;
    locator->lpSAXLocatorVtbl = &isaxlocator_vtbl;
    locator->ref = 1;
    locator->vbInterface = vbInterface;

    locator->saxreader = reader;
    ISAXXMLReader_AddRef((ISAXXMLReader*)&reader->lpSAXXMLReaderVtbl);

    locator->pParserCtxt = NULL;
    locator->publicId = NULL;
    locator->systemId = NULL;
    locator->lastCur = NULL;
    locator->line = 0;
    locator->column = 0;
    locator->ret = S_OK;
    locator->nsStackSize = 8;
    locator->nsStackLast = 0;
    locator->nsStack = HeapAlloc(GetProcessHeap(), 0, locator->nsStackSize);
    if(!locator->nsStack)
    {
        ISAXXMLReader_Release((ISAXXMLReader*)&reader->lpSAXXMLReaderVtbl);
        HeapFree(GetProcessHeap(), 0, locator);
        return E_OUTOFMEMORY;
    }

    *ppsaxlocator = locator;

    TRACE("returning %p\n", *ppsaxlocator);

    return S_OK;
}

/*** SAXXMLReader internal functions ***/
static HRESULT internal_parseBuffer(saxreader *This, const char *buffer, int size, BOOL vbInterface)
{
    saxlocator *locator;
    HRESULT hr;

    hr = SAXLocator_create(This, &locator, vbInterface);
    if(FAILED(hr))
        return hr;

    locator->pParserCtxt = xmlCreateMemoryParserCtxt(buffer, size);
    if(!locator->pParserCtxt)
    {
        ISAXLocator_Release((ISAXLocator*)&locator->lpSAXLocatorVtbl);
        return E_FAIL;
    }

    locator->pParserCtxt->sax = &locator->saxreader->sax;
    locator->pParserCtxt->userData = locator;

    This->isParsing = TRUE;
    if(xmlParseDocument(locator->pParserCtxt)) hr = E_FAIL;
    else hr = locator->ret;
    This->isParsing = FALSE;

    if(locator->pParserCtxt)
    {
        locator->pParserCtxt->sax = NULL;
        xmlFreeParserCtxt(locator->pParserCtxt);
        locator->pParserCtxt = NULL;
    }

    ISAXLocator_Release((ISAXLocator*)&locator->lpSAXLocatorVtbl);
    return hr;
}

static HRESULT internal_parseStream(saxreader *This, IStream *stream, BOOL vbInterface)
{
    saxlocator *locator;
    HRESULT hr;
    ULONG dataRead;
    char data[1024];

    hr = IStream_Read(stream, data, sizeof(data), &dataRead);
    if(hr != S_OK)
        return hr;

    hr = SAXLocator_create(This, &locator, vbInterface);
    if(FAILED(hr))
        return hr;

    locator->pParserCtxt = xmlCreatePushParserCtxt(
            &locator->saxreader->sax, locator,
            data, dataRead, NULL);
    if(!locator->pParserCtxt)
    {
        ISAXLocator_Release((ISAXLocator*)&locator->lpSAXLocatorVtbl);
        return E_FAIL;
    }

    This->isParsing = TRUE;
    while(1)
    {
        hr = IStream_Read(stream, data, sizeof(data), &dataRead);
        if(hr != S_OK)
            break;

        if(xmlParseChunk(locator->pParserCtxt, data, dataRead, 0)) hr = E_FAIL;
        else hr = locator->ret;

        if(hr != S_OK) break;

        if(dataRead != sizeof(data))
        {
            if(xmlParseChunk(locator->pParserCtxt, data, 0, 1)) hr = E_FAIL;
            else hr = locator->ret;

            break;
        }
    }
    This->isParsing = FALSE;

    locator->pParserCtxt->sax = NULL;
    xmlFreeParserCtxt(locator->pParserCtxt);
    locator->pParserCtxt = NULL;
    ISAXLocator_Release((ISAXLocator*)&locator->lpSAXLocatorVtbl);
    return hr;
}

static HRESULT internal_getEntityResolver(
        saxreader *This,
        void *pEntityResolver,
        BOOL vbInterface)
{
    FIXME("(%p)->(%p) stub\n", This, pEntityResolver);
    return E_NOTIMPL;
}

static HRESULT internal_putEntityResolver(
        saxreader *This,
        void *pEntityResolver,
        BOOL vbInterface)
{
    FIXME("(%p)->(%p) stub\n", This, pEntityResolver);
    return E_NOTIMPL;
}

static HRESULT internal_getContentHandler(
        saxreader* This,
        void *pContentHandler,
        BOOL vbInterface)
{
    TRACE("(%p)->(%p)\n", This, pContentHandler);
    if(pContentHandler == NULL)
        return E_POINTER;
    if((vbInterface && This->vbcontentHandler)
            || (!vbInterface && This->contentHandler))
    {
        if(vbInterface)
            IVBSAXContentHandler_AddRef(This->vbcontentHandler);
        else
            ISAXContentHandler_AddRef(This->contentHandler);
    }
    if(vbInterface) *(IVBSAXContentHandler**)pContentHandler =
        This->vbcontentHandler;
    else *(ISAXContentHandler**)pContentHandler = This->contentHandler;

    return S_OK;
}

static HRESULT internal_putContentHandler(
        saxreader* This,
        void *contentHandler,
        BOOL vbInterface)
{
    TRACE("(%p)->(%p)\n", This, contentHandler);
    if(contentHandler)
    {
        if(vbInterface)
            IVBSAXContentHandler_AddRef((IVBSAXContentHandler*)contentHandler);
        else
            ISAXContentHandler_AddRef((ISAXContentHandler*)contentHandler);
    }
    if((vbInterface && This->vbcontentHandler)
            || (!vbInterface && This->contentHandler))
    {
        if(vbInterface)
            IVBSAXContentHandler_Release(This->vbcontentHandler);
        else
            ISAXContentHandler_Release(This->contentHandler);
    }
    if(vbInterface)
        This->vbcontentHandler = contentHandler;
    else
        This->contentHandler = contentHandler;

    return S_OK;
}

static HRESULT internal_getDTDHandler(
        saxreader* This,
        void *pDTDHandler,
        BOOL vbInterface)
{
    FIXME("(%p)->(%p) stub\n", This, pDTDHandler);
    return E_NOTIMPL;
}

static HRESULT internal_putDTDHandler(
        saxreader* This,
        void *pDTDHandler,
        BOOL vbInterface)
{
    FIXME("(%p)->(%p) stub\n", This, pDTDHandler);
    return E_NOTIMPL;
}

static HRESULT internal_getErrorHandler(
        saxreader* This,
        void *pErrorHandler,
        BOOL vbInterface)
{
    TRACE("(%p)->(%p)\n", This, pErrorHandler);
    if(pErrorHandler == NULL)
        return E_POINTER;

    if(vbInterface && This->vberrorHandler)
        IVBSAXErrorHandler_AddRef(This->vberrorHandler);
    else if(!vbInterface && This->errorHandler)
        ISAXErrorHandler_AddRef(This->errorHandler);

    if(vbInterface)
        *(IVBSAXErrorHandler**)pErrorHandler = This->vberrorHandler;
    else
        *(ISAXErrorHandler**)pErrorHandler = This->errorHandler;

    return S_OK;

}

static HRESULT internal_putErrorHandler(
        saxreader* This,
        void *errorHandler,
        BOOL vbInterface)
{
    TRACE("(%p)->(%p)\n", This, errorHandler);
    if(errorHandler)
    {
        if(vbInterface)
            IVBSAXErrorHandler_AddRef((IVBSAXErrorHandler*)errorHandler);
        else
            ISAXErrorHandler_AddRef((ISAXErrorHandler*)errorHandler);
    }

    if(vbInterface && This->vberrorHandler)
        IVBSAXErrorHandler_Release(This->vberrorHandler);
    else if(!vbInterface && This->errorHandler)
        ISAXErrorHandler_Release(This->errorHandler);

    if(vbInterface)
        This->vberrorHandler = errorHandler;
    else
        This->errorHandler = errorHandler;

    return S_OK;

}

static HRESULT internal_parse(
        saxreader* This,
        VARIANT varInput,
        BOOL vbInterface)
{
    HRESULT hr;

    TRACE("(%p)\n", This);

    hr = S_OK;
    switch(V_VT(&varInput))
    {
        case VT_BSTR:
            hr = internal_parseBuffer(This, (const char*)V_BSTR(&varInput),
                    SysStringByteLen(V_BSTR(&varInput)), vbInterface);
            break;
        case VT_ARRAY|VT_UI1: {
            void *pSAData;
            LONG lBound, uBound;
            ULONG dataRead;

            hr = SafeArrayGetLBound(V_ARRAY(&varInput), 1, &lBound);
            if(hr != S_OK) break;
            hr = SafeArrayGetUBound(V_ARRAY(&varInput), 1, &uBound);
            if(hr != S_OK) break;
            dataRead = (uBound-lBound)*SafeArrayGetElemsize(V_ARRAY(&varInput));
            hr = SafeArrayAccessData(V_ARRAY(&varInput), &pSAData);
            if(hr != S_OK) break;
            hr = internal_parseBuffer(This, pSAData, dataRead, vbInterface);
            SafeArrayUnaccessData(V_ARRAY(&varInput));
            break;
        }
        case VT_UNKNOWN:
        case VT_DISPATCH: {
            IPersistStream *persistStream;
            IStream *stream = NULL;
            IXMLDOMDocument *xmlDoc;

            if(IUnknown_QueryInterface(V_UNKNOWN(&varInput),
                        &IID_IXMLDOMDocument, (void**)&xmlDoc) == S_OK)
            {
                BSTR bstrData;

                IXMLDOMDocument_get_xml(xmlDoc, &bstrData);
                hr = internal_parseBuffer(This, (const char*)bstrData,
                        SysStringByteLen(bstrData), vbInterface);
                IXMLDOMDocument_Release(xmlDoc);
                break;
            }
            if(IUnknown_QueryInterface(V_UNKNOWN(&varInput),
                        &IID_IPersistStream, (void**)&persistStream) == S_OK)
            {
                hr = IPersistStream_Save(persistStream, stream, TRUE);
                IPersistStream_Release(persistStream);
                if(hr != S_OK) break;
            }
            if(stream || IUnknown_QueryInterface(V_UNKNOWN(&varInput),
                        &IID_IStream, (void**)&stream) == S_OK)
            {
                hr = internal_parseStream(This, stream, vbInterface);
                IStream_Release(stream);
                break;
            }
        }
        default:
            WARN("vt %d not implemented\n", V_VT(&varInput));
            hr = E_INVALIDARG;
    }

    return hr;
}

static HRESULT internal_vbonDataAvailable(void *obj, char *ptr, DWORD len)
{
    saxreader *This = obj;

    return internal_parseBuffer(This, ptr, len, TRUE);
}

static HRESULT internal_onDataAvailable(void *obj, char *ptr, DWORD len)
{
    saxreader *This = obj;

    return internal_parseBuffer(This, ptr, len, FALSE);
}

static HRESULT internal_parseURL(
        saxreader* This,
        const WCHAR *url,
        BOOL vbInterface)
{
    bsc_t *bsc;
    HRESULT hr;

    TRACE("(%p)->(%s)\n", This, debugstr_w(url));

    if(vbInterface) hr = bind_url(url, internal_vbonDataAvailable, This, &bsc);
    else hr = bind_url(url, internal_onDataAvailable, This, &bsc);

    if(FAILED(hr))
        return hr;

    detach_bsc(bsc);

    return S_OK;
}

static HRESULT internal_putProperty(
    saxreader* This,
    const WCHAR *pProp,
    VARIANT value,
    BOOL vbInterface)
{
    static const WCHAR wszCharset[] = {
        'c','h','a','r','s','e','t',0
    };
    static const WCHAR wszDeclarationHandler[] = {
        'h','t','t','p',':','/','/','x','m','l','.','o','r','g','/',
        's','a','x','/','p','r','o','p','e','r','t','i','e','s','/',
        'd','e','c','l','a','r','a','t','i','o','n',
        '-','h','a','n','d','l','e','r',0
    };
    static const WCHAR wszDomNode[] = {
        'h','t','t','p',':','/','/','x','m','l','.','o','r','g','/',
        's','a','x','/','p','r','o','p','e','r','t','i','e','s','/',
        'd','o','m','-','n','o','d','e',0
    };
    static const WCHAR wszInputSource[] = {
        'i','n','p','u','t','-','s','o','u','r','c','e',0
    };
    static const WCHAR wszLexicalHandler[] = {
        'h','t','t','p',':','/','/','x','m','l','.','o','r','g','/',
        's','a','x','/','p','r','o','p','e','r','t','i','e','s','/',
        'l','e','x','i','c','a','l','-','h','a','n','d','l','e','r',0
    };
    static const WCHAR wszMaxElementDepth[] = {
        'm','a','x','-','e','l','e','m','e','n','t','-','d','e','p','t','h',0
    };
    static const WCHAR wszMaxXMLSize[] = {
        'm','a','x','-','x','m','l','-','s','i','z','e',0
    };
    static const WCHAR wszSchemaDeclarationHandler[] = {
        's','c','h','e','m','a','-',
        'd','e','c','l','a','r','a','t','i','o','n','-',
        'h','a','n','d','l','e','r',0
    };
    static const WCHAR wszXMLDeclEncoding[] = {
        'x','m','l','d','e','c','l','-','e','n','c','o','d','i','n','g',0
    };
    static const WCHAR wszXMLDeclStandalone[] = {
        'x','m','l','d','e','c','l',
        '-','s','t','a','n','d','a','l','o','n','e',0
    };
    static const WCHAR wszXMLDeclVersion[] = {
        'x','m','l','d','e','c','l','-','v','e','r','s','i','o','n',0
    };

    FIXME("(%p)->(%s): semi-stub\n", This, debugstr_w(pProp));

    if(!memcmp(pProp, wszCharset, sizeof(wszCharset)))
        return E_NOTIMPL;

    if(!memcmp(pProp, wszDeclarationHandler, sizeof(wszDeclarationHandler)))
    {
        if(This->isParsing) return E_FAIL;

        if(V_UNKNOWN(&value))
        {
            if(vbInterface)
                IVBSAXDeclHandler_AddRef((IVBSAXDeclHandler*)V_UNKNOWN(&value));
            else
                ISAXDeclHandler_AddRef((ISAXDeclHandler*)V_UNKNOWN(&value));
        }
        if((vbInterface && This->vbdeclHandler)
                || (!vbInterface && This->declHandler))
        {
            if(vbInterface)
                IVBSAXDeclHandler_Release(This->vbdeclHandler);
            else
                ISAXDeclHandler_Release(This->declHandler);
        }
        if(vbInterface)
            This->vbdeclHandler = (IVBSAXDeclHandler*)V_UNKNOWN(&value);
        else
            This->declHandler = (ISAXDeclHandler*)V_UNKNOWN(&value);
        return S_OK;
    }

    if(!memcmp(pProp, wszDomNode, sizeof(wszDomNode)))
        return E_FAIL;

    if(!memcmp(pProp, wszInputSource, sizeof(wszInputSource)))
        return E_NOTIMPL;

    if(!memcmp(pProp, wszLexicalHandler, sizeof(wszLexicalHandler)))
    {
        if(This->isParsing) return E_FAIL;

        if(V_UNKNOWN(&value))
        {
            if(vbInterface)
                IVBSAXLexicalHandler_AddRef(
                        (IVBSAXLexicalHandler*)V_UNKNOWN(&value));
            else
                ISAXLexicalHandler_AddRef(
                        (ISAXLexicalHandler*)V_UNKNOWN(&value));
        }
        if((vbInterface && This->vblexicalHandler)
                || (!vbInterface && This->lexicalHandler))
        {
            if(vbInterface)
                IVBSAXLexicalHandler_Release(This->vblexicalHandler);
            else
                ISAXLexicalHandler_Release(This->lexicalHandler);
        }
        if(vbInterface)
            This->vblexicalHandler = (IVBSAXLexicalHandler*)V_UNKNOWN(&value);
        else
            This->lexicalHandler = (ISAXLexicalHandler*)V_UNKNOWN(&value);
        return S_OK;
    }

    if(!memcmp(pProp, wszMaxElementDepth, sizeof(wszMaxElementDepth)))
        return E_NOTIMPL;

    if(!memcmp(pProp, wszMaxXMLSize, sizeof(wszMaxXMLSize)))
        return E_NOTIMPL;

    if(!memcmp(pProp, wszSchemaDeclarationHandler,
                sizeof(wszSchemaDeclarationHandler)))
        return E_NOTIMPL;

    if(!memcmp(pProp, wszXMLDeclEncoding, sizeof(wszXMLDeclEncoding)))
        return E_FAIL;

    if(!memcmp(pProp, wszXMLDeclStandalone, sizeof(wszXMLDeclStandalone)))
        return E_FAIL;

    if(!memcmp(pProp, wszXMLDeclVersion, sizeof(wszXMLDeclVersion)))
        return E_FAIL;

    return E_INVALIDARG;
}

/*** IVBSAXXMLReader interface ***/
/*** IUnknown methods ***/
static HRESULT WINAPI saxxmlreader_QueryInterface(IVBSAXXMLReader* iface, REFIID riid, void **ppvObject)
{
    saxreader *This = impl_from_IVBSAXXMLReader( iface );

    TRACE("%p %s %p\n", This, debugstr_guid( riid ), ppvObject );

    *ppvObject = NULL;

    if ( IsEqualGUID( riid, &IID_IUnknown ) ||
         IsEqualGUID( riid, &IID_IDispatch ) ||
         IsEqualGUID( riid, &IID_IVBSAXXMLReader ))
    {
        *ppvObject = iface;
    }
    else if( IsEqualGUID( riid, &IID_ISAXXMLReader ))
    {
        *ppvObject = &This->lpSAXXMLReaderVtbl;
    }
    else
    {
        FIXME("interface %s not implemented\n", debugstr_guid(riid));
        return E_NOINTERFACE;
    }

    IVBSAXXMLReader_AddRef( iface );

    return S_OK;
}

static ULONG WINAPI saxxmlreader_AddRef(IVBSAXXMLReader* iface)
{
    saxreader *This = impl_from_IVBSAXXMLReader( iface );
    TRACE("%p\n", This );
    return InterlockedIncrement( &This->ref );
}

static ULONG WINAPI saxxmlreader_Release(
    IVBSAXXMLReader* iface)
{
    saxreader *This = impl_from_IVBSAXXMLReader( iface );
    LONG ref;

    TRACE("%p\n", This );

    ref = InterlockedDecrement( &This->ref );
    if ( ref == 0 )
    {
        if(This->contentHandler)
            ISAXContentHandler_Release(This->contentHandler);

        if(This->vbcontentHandler)
            IVBSAXContentHandler_Release(This->vbcontentHandler);

        if(This->errorHandler)
            ISAXErrorHandler_Release(This->errorHandler);

        if(This->vberrorHandler)
            IVBSAXErrorHandler_Release(This->vberrorHandler);

        if(This->lexicalHandler)
            ISAXLexicalHandler_Release(This->lexicalHandler);

        if(This->vblexicalHandler)
            IVBSAXLexicalHandler_Release(This->vblexicalHandler);

        if(This->declHandler)
            ISAXDeclHandler_Release(This->declHandler);

        if(This->vbdeclHandler)
            IVBSAXDeclHandler_Release(This->vbdeclHandler);

        HeapFree( GetProcessHeap(), 0, This );
    }

    return ref;
}
/*** IDispatch ***/
static HRESULT WINAPI saxxmlreader_GetTypeInfoCount( IVBSAXXMLReader *iface, UINT* pctinfo )
{
    saxreader *This = impl_from_IVBSAXXMLReader( iface );

    TRACE("(%p)->(%p)\n", This, pctinfo);

    *pctinfo = 1;

    return S_OK;
}

static HRESULT WINAPI saxxmlreader_GetTypeInfo(
    IVBSAXXMLReader *iface,
    UINT iTInfo, LCID lcid, ITypeInfo** ppTInfo )
{
    saxreader *This = impl_from_IVBSAXXMLReader( iface );
    HRESULT hr;

    TRACE("(%p)->(%u %u %p)\n", This, iTInfo, lcid, ppTInfo);

    hr = get_typeinfo(IVBSAXXMLReader_tid, ppTInfo);

    return hr;
}

static HRESULT WINAPI saxxmlreader_GetIDsOfNames(
    IVBSAXXMLReader *iface,
    REFIID riid,
    LPOLESTR* rgszNames,
    UINT cNames,
    LCID lcid,
    DISPID* rgDispId)
{
    saxreader *This = impl_from_IVBSAXXMLReader( iface );
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("(%p)->(%s %p %u %u %p)\n", This, debugstr_guid(riid), rgszNames, cNames,
          lcid, rgDispId);

    if(!rgszNames || cNames == 0 || !rgDispId)
        return E_INVALIDARG;

    hr = get_typeinfo(IVBSAXXMLReader_tid, &typeinfo);
    if(SUCCEEDED(hr))
    {
        hr = ITypeInfo_GetIDsOfNames(typeinfo, rgszNames, cNames, rgDispId);
        ITypeInfo_Release(typeinfo);
    }

    return hr;
}

static HRESULT WINAPI saxxmlreader_Invoke(
    IVBSAXXMLReader *iface,
    DISPID dispIdMember,
    REFIID riid,
    LCID lcid,
    WORD wFlags,
    DISPPARAMS* pDispParams,
    VARIANT* pVarResult,
    EXCEPINFO* pExcepInfo,
    UINT* puArgErr)
{
    saxreader *This = impl_from_IVBSAXXMLReader( iface );
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("(%p)->(%d %s %d %d %p %p %p %p)\n", This, dispIdMember, debugstr_guid(riid),
          lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);

    hr = get_typeinfo(IVBSAXXMLReader_tid, &typeinfo);
    if(SUCCEEDED(hr))
    {
        hr = ITypeInfo_Invoke(typeinfo, &(This->lpVBSAXXMLReaderVtbl), dispIdMember, wFlags, pDispParams,
                pVarResult, pExcepInfo, puArgErr);
        ITypeInfo_Release(typeinfo);
    }

    return hr;
}

/*** IVBSAXXMLReader methods ***/
static HRESULT WINAPI saxxmlreader_getFeature(
    IVBSAXXMLReader* iface,
    const WCHAR *pFeature,
    VARIANT_BOOL *pValue)
{
    saxreader *This = impl_from_IVBSAXXMLReader( iface );

    FIXME("(%p)->(%s %p) stub\n", This, debugstr_w(pFeature), pValue);
    return E_NOTIMPL;
}

static HRESULT WINAPI saxxmlreader_putFeature(
    IVBSAXXMLReader* iface,
    const WCHAR *pFeature,
    VARIANT_BOOL vfValue)
{
    saxreader *This = impl_from_IVBSAXXMLReader( iface );

    FIXME("(%p)->(%s %x) stub\n", This, debugstr_w(pFeature), vfValue);
    return E_NOTIMPL;
}

static HRESULT WINAPI saxxmlreader_getProperty(
    IVBSAXXMLReader* iface,
    const WCHAR *pProp,
    VARIANT *pValue)
{
    saxreader *This = impl_from_IVBSAXXMLReader( iface );

    FIXME("(%p)->(%s %p) stub\n", This, debugstr_w(pProp), pValue);
    return E_NOTIMPL;
}

static HRESULT WINAPI saxxmlreader_putProperty(
    IVBSAXXMLReader* iface,
    const WCHAR *pProp,
    VARIANT value)
{
    saxreader *This = impl_from_IVBSAXXMLReader( iface );
    return internal_putProperty(This, pProp, value, TRUE);
}

static HRESULT WINAPI saxxmlreader_get_entityResolver(
    IVBSAXXMLReader* iface,
    IVBSAXEntityResolver **pEntityResolver)
{
    saxreader *This = impl_from_IVBSAXXMLReader( iface );
    return internal_getEntityResolver(This, pEntityResolver, TRUE);
}

static HRESULT WINAPI saxxmlreader_put_entityResolver(
    IVBSAXXMLReader* iface,
    IVBSAXEntityResolver *pEntityResolver)
{
    saxreader *This = impl_from_IVBSAXXMLReader( iface );
    return internal_putEntityResolver(This, pEntityResolver, TRUE);
}

static HRESULT WINAPI saxxmlreader_get_contentHandler(
    IVBSAXXMLReader* iface,
    IVBSAXContentHandler **ppContentHandler)
{
    saxreader *This = impl_from_IVBSAXXMLReader( iface );
    return internal_getContentHandler(This, ppContentHandler, TRUE);
}

static HRESULT WINAPI saxxmlreader_put_contentHandler(
    IVBSAXXMLReader* iface,
    IVBSAXContentHandler *contentHandler)
{
    saxreader *This = impl_from_IVBSAXXMLReader( iface );
    return internal_putContentHandler(This, contentHandler, TRUE);
}

static HRESULT WINAPI saxxmlreader_get_dtdHandler(
    IVBSAXXMLReader* iface,
    IVBSAXDTDHandler **pDTDHandler)
{
    saxreader *This = impl_from_IVBSAXXMLReader( iface );
    return internal_getDTDHandler(This, pDTDHandler, TRUE);
}

static HRESULT WINAPI saxxmlreader_put_dtdHandler(
    IVBSAXXMLReader* iface,
    IVBSAXDTDHandler *pDTDHandler)
{
    saxreader *This = impl_from_IVBSAXXMLReader( iface );
    return internal_putDTDHandler(This, pDTDHandler, TRUE);
}

static HRESULT WINAPI saxxmlreader_get_errorHandler(
    IVBSAXXMLReader* iface,
    IVBSAXErrorHandler **pErrorHandler)
{
    saxreader *This = impl_from_IVBSAXXMLReader( iface );
    return internal_getErrorHandler(This, pErrorHandler, TRUE);
}

static HRESULT WINAPI saxxmlreader_put_errorHandler(
    IVBSAXXMLReader* iface,
    IVBSAXErrorHandler *errorHandler)
{
    saxreader *This = impl_from_IVBSAXXMLReader( iface );
    return internal_putErrorHandler(This, errorHandler, TRUE);
}

static HRESULT WINAPI saxxmlreader_get_baseURL(
    IVBSAXXMLReader* iface,
    const WCHAR **pBaseUrl)
{
    saxreader *This = impl_from_IVBSAXXMLReader( iface );

    FIXME("(%p)->(%p) stub\n", This, pBaseUrl);
    return E_NOTIMPL;
}

static HRESULT WINAPI saxxmlreader_put_baseURL(
    IVBSAXXMLReader* iface,
    const WCHAR *pBaseUrl)
{
    saxreader *This = impl_from_IVBSAXXMLReader( iface );

    FIXME("(%p)->(%s) stub\n", This, debugstr_w(pBaseUrl));
    return E_NOTIMPL;
}

static HRESULT WINAPI saxxmlreader_get_secureBaseURL(
    IVBSAXXMLReader* iface,
    const WCHAR **pSecureBaseUrl)
{
    saxreader *This = impl_from_IVBSAXXMLReader( iface );

    FIXME("(%p)->(%p) stub\n", This, pSecureBaseUrl);
    return E_NOTIMPL;
}


static HRESULT WINAPI saxxmlreader_put_secureBaseURL(
    IVBSAXXMLReader* iface,
    const WCHAR *secureBaseUrl)
{
    saxreader *This = impl_from_IVBSAXXMLReader( iface );

    FIXME("(%p)->(%s) stub\n", This, debugstr_w(secureBaseUrl));
    return E_NOTIMPL;
}

static HRESULT WINAPI saxxmlreader_parse(
    IVBSAXXMLReader* iface,
    VARIANT varInput)
{
    saxreader *This = impl_from_IVBSAXXMLReader( iface );
    return internal_parse(This, varInput, TRUE);
}

static HRESULT WINAPI saxxmlreader_parseURL(
    IVBSAXXMLReader* iface,
    const WCHAR *url)
{
    saxreader *This = impl_from_IVBSAXXMLReader( iface );
    return internal_parseURL(This, url, TRUE);
}

static const struct IVBSAXXMLReaderVtbl saxreader_vtbl =
{
    saxxmlreader_QueryInterface,
    saxxmlreader_AddRef,
    saxxmlreader_Release,
    saxxmlreader_GetTypeInfoCount,
    saxxmlreader_GetTypeInfo,
    saxxmlreader_GetIDsOfNames,
    saxxmlreader_Invoke,
    saxxmlreader_getFeature,
    saxxmlreader_putFeature,
    saxxmlreader_getProperty,
    saxxmlreader_putProperty,
    saxxmlreader_get_entityResolver,
    saxxmlreader_put_entityResolver,
    saxxmlreader_get_contentHandler,
    saxxmlreader_put_contentHandler,
    saxxmlreader_get_dtdHandler,
    saxxmlreader_put_dtdHandler,
    saxxmlreader_get_errorHandler,
    saxxmlreader_put_errorHandler,
    saxxmlreader_get_baseURL,
    saxxmlreader_put_baseURL,
    saxxmlreader_get_secureBaseURL,
    saxxmlreader_put_secureBaseURL,
    saxxmlreader_parse,
    saxxmlreader_parseURL
};

/*** ISAXXMLReader interface ***/
/*** IUnknown methods ***/
static HRESULT WINAPI isaxxmlreader_QueryInterface(ISAXXMLReader* iface, REFIID riid, void **ppvObject)
{
    saxreader *This = impl_from_ISAXXMLReader( iface );
    return saxxmlreader_QueryInterface((IVBSAXXMLReader*)&This->lpVBSAXXMLReaderVtbl, riid, ppvObject);
}

static ULONG WINAPI isaxxmlreader_AddRef(ISAXXMLReader* iface)
{
    saxreader *This = impl_from_ISAXXMLReader( iface );
    return saxxmlreader_AddRef((IVBSAXXMLReader*)&This->lpVBSAXXMLReaderVtbl);
}

static ULONG WINAPI isaxxmlreader_Release(ISAXXMLReader* iface)
{
    saxreader *This = impl_from_ISAXXMLReader( iface );
    return saxxmlreader_Release((IVBSAXXMLReader*)&This->lpVBSAXXMLReaderVtbl);
}

/*** ISAXXMLReader methods ***/
static HRESULT WINAPI isaxxmlreader_getFeature(
        ISAXXMLReader* iface,
        const WCHAR *pFeature,
        VARIANT_BOOL *pValue)
{
    saxreader *This = impl_from_ISAXXMLReader( iface );
    return IVBSAXXMLReader_getFeature(
            (IVBSAXXMLReader*)&This->lpVBSAXXMLReaderVtbl,
            pFeature, pValue);
}

static HRESULT WINAPI isaxxmlreader_putFeature(
        ISAXXMLReader* iface,
        const WCHAR *pFeature,
        VARIANT_BOOL vfValue)
{
    saxreader *This = impl_from_ISAXXMLReader( iface );
    return IVBSAXXMLReader_putFeature(
            (IVBSAXXMLReader*)&This->lpVBSAXXMLReaderVtbl,
            pFeature, vfValue);
}

static HRESULT WINAPI isaxxmlreader_getProperty(
        ISAXXMLReader* iface,
        const WCHAR *pProp,
        VARIANT *pValue)
{
    saxreader *This = impl_from_ISAXXMLReader( iface );
    return IVBSAXXMLReader_getProperty(
            (IVBSAXXMLReader*)&This->lpVBSAXXMLReaderVtbl,
            pProp, pValue);
}

static HRESULT WINAPI isaxxmlreader_putProperty(
        ISAXXMLReader* iface,
        const WCHAR *pProp,
        VARIANT value)
{
    saxreader *This = impl_from_ISAXXMLReader( iface );
    return internal_putProperty(This, pProp, value, FALSE);
}

static HRESULT WINAPI isaxxmlreader_getEntityResolver(
        ISAXXMLReader* iface,
        ISAXEntityResolver **ppEntityResolver)
{
    saxreader *This = impl_from_ISAXXMLReader( iface );
    return internal_getEntityResolver(This, ppEntityResolver, FALSE);
}

static HRESULT WINAPI isaxxmlreader_putEntityResolver(
        ISAXXMLReader* iface,
        ISAXEntityResolver *pEntityResolver)
{
    saxreader *This = impl_from_ISAXXMLReader( iface );
    return internal_putEntityResolver(This, pEntityResolver, FALSE);
}

static HRESULT WINAPI isaxxmlreader_getContentHandler(
        ISAXXMLReader* iface,
        ISAXContentHandler **pContentHandler)
{
    saxreader *This = impl_from_ISAXXMLReader( iface );
    return internal_getContentHandler(This, pContentHandler, FALSE);
}

static HRESULT WINAPI isaxxmlreader_putContentHandler(
        ISAXXMLReader* iface,
        ISAXContentHandler *contentHandler)
{
    saxreader *This = impl_from_ISAXXMLReader( iface );
    return internal_putContentHandler(This, contentHandler, FALSE);
}

static HRESULT WINAPI isaxxmlreader_getDTDHandler(
        ISAXXMLReader* iface,
        ISAXDTDHandler **pDTDHandler)
{
    saxreader *This = impl_from_ISAXXMLReader( iface );
    return internal_getDTDHandler(This, pDTDHandler, FALSE);
}

static HRESULT WINAPI isaxxmlreader_putDTDHandler(
        ISAXXMLReader* iface,
        ISAXDTDHandler *pDTDHandler)
{
    saxreader *This = impl_from_ISAXXMLReader( iface );
    return internal_putDTDHandler(This, pDTDHandler, FALSE);
}

static HRESULT WINAPI isaxxmlreader_getErrorHandler(
        ISAXXMLReader* iface,
        ISAXErrorHandler **pErrorHandler)
{
    saxreader *This = impl_from_ISAXXMLReader( iface );
    return internal_getErrorHandler(This, pErrorHandler, FALSE);
}

static HRESULT WINAPI isaxxmlreader_putErrorHandler(
        ISAXXMLReader* iface,
        ISAXErrorHandler *errorHandler)
{
    saxreader *This = impl_from_ISAXXMLReader( iface );
    return internal_putErrorHandler(This, errorHandler, FALSE);
}

static HRESULT WINAPI isaxxmlreader_getBaseURL(
        ISAXXMLReader* iface,
        const WCHAR **pBaseUrl)
{
    saxreader *This = impl_from_ISAXXMLReader( iface );
    return IVBSAXXMLReader_get_baseURL(
            (IVBSAXXMLReader*)&This->lpVBSAXXMLReaderVtbl,
            pBaseUrl);
}

static HRESULT WINAPI isaxxmlreader_putBaseURL(
        ISAXXMLReader* iface,
        const WCHAR *pBaseUrl)
{
    saxreader *This = impl_from_ISAXXMLReader( iface );
    return IVBSAXXMLReader_put_baseURL(
            (IVBSAXXMLReader*)&This->lpVBSAXXMLReaderVtbl,
            pBaseUrl);
}

static HRESULT WINAPI isaxxmlreader_getSecureBaseURL(
        ISAXXMLReader* iface,
        const WCHAR **pSecureBaseUrl)
{
    saxreader *This = impl_from_ISAXXMLReader( iface );
    return IVBSAXXMLReader_get_secureBaseURL(
            (IVBSAXXMLReader*)&This->lpVBSAXXMLReaderVtbl,
            pSecureBaseUrl);
}

static HRESULT WINAPI isaxxmlreader_putSecureBaseURL(
        ISAXXMLReader* iface,
        const WCHAR *secureBaseUrl)
{
    saxreader *This = impl_from_ISAXXMLReader( iface );
    return IVBSAXXMLReader_put_secureBaseURL(
            (IVBSAXXMLReader*)&This->lpVBSAXXMLReaderVtbl,
            secureBaseUrl);
}

static HRESULT WINAPI isaxxmlreader_parse(
        ISAXXMLReader* iface,
        VARIANT varInput)
{
    saxreader *This = impl_from_ISAXXMLReader( iface );
    return internal_parse(This, varInput, FALSE);
}

static HRESULT WINAPI isaxxmlreader_parseURL(
        ISAXXMLReader* iface,
        const WCHAR *url)
{
    saxreader *This = impl_from_ISAXXMLReader( iface );
    return internal_parseURL(This, url, FALSE);
}

static const struct ISAXXMLReaderVtbl isaxreader_vtbl =
{
    isaxxmlreader_QueryInterface,
    isaxxmlreader_AddRef,
    isaxxmlreader_Release,
    isaxxmlreader_getFeature,
    isaxxmlreader_putFeature,
    isaxxmlreader_getProperty,
    isaxxmlreader_putProperty,
    isaxxmlreader_getEntityResolver,
    isaxxmlreader_putEntityResolver,
    isaxxmlreader_getContentHandler,
    isaxxmlreader_putContentHandler,
    isaxxmlreader_getDTDHandler,
    isaxxmlreader_putDTDHandler,
    isaxxmlreader_getErrorHandler,
    isaxxmlreader_putErrorHandler,
    isaxxmlreader_getBaseURL,
    isaxxmlreader_putBaseURL,
    isaxxmlreader_getSecureBaseURL,
    isaxxmlreader_putSecureBaseURL,
    isaxxmlreader_parse,
    isaxxmlreader_parseURL
};

HRESULT SAXXMLReader_create(IUnknown *pUnkOuter, LPVOID *ppObj)
{
    saxreader *reader;

    TRACE("(%p,%p)\n", pUnkOuter, ppObj);

    reader = HeapAlloc( GetProcessHeap(), 0, sizeof (*reader) );
    if( !reader )
        return E_OUTOFMEMORY;

    reader->lpVBSAXXMLReaderVtbl = &saxreader_vtbl;
    reader->lpSAXXMLReaderVtbl = &isaxreader_vtbl;
    reader->ref = 1;
    reader->contentHandler = NULL;
    reader->vbcontentHandler = NULL;
    reader->errorHandler = NULL;
    reader->vberrorHandler = NULL;
    reader->lexicalHandler = NULL;
    reader->vblexicalHandler = NULL;
    reader->declHandler = NULL;
    reader->vbdeclHandler = NULL;
    reader->isParsing = FALSE;

    memset(&reader->sax, 0, sizeof(xmlSAXHandler));
    reader->sax.initialized = XML_SAX2_MAGIC;
    reader->sax.startDocument = libxmlStartDocument;
    reader->sax.endDocument = libxmlEndDocument;
    reader->sax.startElementNs = libxmlStartElementNS;
    reader->sax.endElementNs = libxmlEndElementNS;
    reader->sax.characters = libxmlCharacters;
    reader->sax.setDocumentLocator = libxmlSetDocumentLocator;
    reader->sax.comment = libxmlComment;
    reader->sax.error = libxmlFatalError;
    reader->sax.fatalError = libxmlFatalError;
    reader->sax.cdataBlock = libxmlCDataBlock;

    *ppObj = &reader->lpVBSAXXMLReaderVtbl;

    TRACE("returning iface %p\n", *ppObj);

    return S_OK;
}

#else

HRESULT SAXXMLReader_create(IUnknown *pUnkOuter, LPVOID *ppObj)
{
    MESSAGE("This program tried to use a SAX XML Reader object, but\n"
            "libxml2 support was not present at compile time.\n");
    return E_NOTIMPL;
}

#endif
