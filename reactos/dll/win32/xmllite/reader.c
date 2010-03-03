/*
 * IXmlReader implementation
 *
 * Copyright 2010 Nikolay Sivov
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
#include "initguid.h"
#include "objbase.h"
#include "xmllite.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(xmllite);

/* not defined in public headers */
DEFINE_GUID(IID_IXmlReaderInput, 0x0b3ccc9b, 0x9214, 0x428b, 0xa2, 0xae, 0xef, 0x3a, 0xa8, 0x71, 0xaf, 0xda);

static HRESULT xmlreaderinput_query_for_stream(IXmlReaderInput *iface, void **pObj);

typedef struct _xmlreader
{
    const IXmlReaderVtbl *lpVtbl;
    LONG ref;
    IXmlReaderInput *input;
    ISequentialStream *stream;/* stored as sequential stream, cause currently
                                 optimizations possible with IStream aren't implemented */
    XmlReadState state;
    UINT line, pos;           /* reader position in XML stream */
} xmlreader;

typedef struct _xmlreaderinput
{
    const IUnknownVtbl *lpVtbl;
    LONG ref;
    IUnknown *input;          /* reference passed on IXmlReaderInput creation */
} xmlreaderinput;

static inline xmlreader *impl_from_IXmlReader(IXmlReader *iface)
{
    return (xmlreader *)((char*)iface - FIELD_OFFSET(xmlreader, lpVtbl));
}

static inline xmlreaderinput *impl_from_IXmlReaderInput(IXmlReaderInput *iface)
{
    return (xmlreaderinput *)((char*)iface - FIELD_OFFSET(xmlreaderinput, lpVtbl));
}

static HRESULT WINAPI xmlreader_QueryInterface(IXmlReader *iface, REFIID riid, void** ppvObject)
{
    xmlreader *This = impl_from_IXmlReader(iface);

    TRACE("%p %s %p\n", This, debugstr_guid(riid), ppvObject);

    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_IXmlReader))
    {
        *ppvObject = iface;
    }
    else
    {
        FIXME("interface %s not implemented\n", debugstr_guid(riid));
        return E_NOINTERFACE;
    }

    IXmlReader_AddRef(iface);

    return S_OK;
}

static ULONG WINAPI xmlreader_AddRef(IXmlReader *iface)
{
    xmlreader *This = impl_from_IXmlReader(iface);
    TRACE("%p\n", This);
    return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI xmlreader_Release(IXmlReader *iface)
{
    xmlreader *This = impl_from_IXmlReader(iface);
    LONG ref;

    TRACE("%p\n", This);

    ref = InterlockedDecrement(&This->ref);
    if (ref == 0)
    {
        if (This->input)  IUnknown_Release(This->input);
        if (This->stream) IUnknown_Release(This->stream);
        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}

static HRESULT WINAPI xmlreader_SetInput(IXmlReader* iface, IUnknown *input)
{
    xmlreader *This = impl_from_IXmlReader(iface);
    HRESULT hr;

    TRACE("(%p %p)\n", This, input);

    if (This->input)
    {
        IUnknown_Release(This->input);
        This->input  = NULL;
    }

    if (This->stream)
    {
        IUnknown_Release(This->stream);
        This->stream = NULL;
    }

    This->line = This->pos = 0;

    /* just reset current input */
    if (!input)
    {
        This->state = XmlReadState_Initial;
        return S_OK;
    }

    /* now try IXmlReaderInput, ISequentialStream, IStream */
    hr = IUnknown_QueryInterface(input, &IID_IXmlReaderInput, (void**)&This->input);
    if (hr != S_OK)
    {
        /* create IXmlReaderInput basing on supplied interface */
        hr = CreateXmlReaderInputWithEncodingName(input,
                                         NULL, NULL, FALSE, NULL, &This->input);
        if (hr != S_OK) return hr;
    }

    /* set stream for supplied IXmlReaderInput */
    hr = xmlreaderinput_query_for_stream(This->input, (void**)&This->stream);
    if (hr == S_OK)
        This->state = XmlReadState_Initial;

    return hr;
}

static HRESULT WINAPI xmlreader_GetProperty(IXmlReader* iface, UINT property, LONG_PTR *value)
{
    xmlreader *This = impl_from_IXmlReader(iface);

    TRACE("(%p %u %p)\n", This, property, value);

    if (!value) return E_INVALIDARG;

    switch (property)
    {
        case XmlReaderProperty_ReadState:
            *value = This->state;
            break;
        default:
            FIXME("Unimplemented property (%u)\n", property);
            return E_NOTIMPL;
    }

    return S_OK;
}

static HRESULT WINAPI xmlreader_SetProperty(IXmlReader* iface, UINT property, LONG_PTR value)
{
    FIXME("(%p %u %lu): stub\n", iface, property, value);
    return E_NOTIMPL;
}

static HRESULT WINAPI xmlreader_Read(IXmlReader* iface, XmlNodeType *node_type)
{
    FIXME("(%p %p): stub\n", iface, node_type);
    return E_NOTIMPL;
}

static HRESULT WINAPI xmlreader_GetNodeType(IXmlReader* iface, XmlNodeType *node_type)
{
    FIXME("(%p %p): stub\n", iface, node_type);
    return E_NOTIMPL;
}

static HRESULT WINAPI xmlreader_MoveToFirstAttribute(IXmlReader* iface)
{
    FIXME("(%p): stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI xmlreader_MoveToNextAttribute(IXmlReader* iface)
{
    FIXME("(%p): stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI xmlreader_MoveToAttributeByName(IXmlReader* iface,
                                                      LPCWSTR local_name,
                                                      LPCWSTR namespaceUri)
{
    FIXME("(%p %p %p): stub\n", iface, local_name, namespaceUri);
    return E_NOTIMPL;
}

static HRESULT WINAPI xmlreader_MoveToElement(IXmlReader* iface)
{
    FIXME("(%p): stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI xmlreader_GetQualifiedName(IXmlReader* iface, LPCWSTR *qualifiedName,
                                                 UINT *qualifiedName_length)
{
    FIXME("(%p %p %p): stub\n", iface, qualifiedName, qualifiedName_length);
    return E_NOTIMPL;
}

static HRESULT WINAPI xmlreader_GetNamespaceUri(IXmlReader* iface,
                                                LPCWSTR *namespaceUri,
                                                UINT *namespaceUri_length)
{
    FIXME("(%p %p %p): stub\n", iface, namespaceUri, namespaceUri_length);
    return E_NOTIMPL;
}

static HRESULT WINAPI xmlreader_GetLocalName(IXmlReader* iface,
                                             LPCWSTR *local_name,
                                             UINT *local_name_length)
{
    FIXME("(%p %p %p): stub\n", iface, local_name, local_name_length);
    return E_NOTIMPL;
}

static HRESULT WINAPI xmlreader_GetPrefix(IXmlReader* iface,
                                          LPCWSTR *prefix,
                                          UINT *prefix_length)
{
    FIXME("(%p %p %p): stub\n", iface, prefix, prefix_length);
    return E_NOTIMPL;
}

static HRESULT WINAPI xmlreader_GetValue(IXmlReader* iface,
                                         LPCWSTR *value,
                                         UINT *value_length)
{
    FIXME("(%p %p %p): stub\n", iface, value, value_length);
    return E_NOTIMPL;
}

static HRESULT WINAPI xmlreader_ReadValueChunk(IXmlReader* iface,
                                               WCHAR *buffer,
                                               UINT   chunk_size,
                                               UINT  *read)
{
    FIXME("(%p %p %u %p): stub\n", iface, buffer, chunk_size, read);
    return E_NOTIMPL;
}

static HRESULT WINAPI xmlreader_GetBaseUri(IXmlReader* iface,
                                           LPCWSTR *baseUri,
                                           UINT *baseUri_length)
{
    FIXME("(%p %p %p): stub\n", iface, baseUri, baseUri_length);
    return E_NOTIMPL;
}

static BOOL WINAPI xmlreader_IsDefault(IXmlReader* iface)
{
    FIXME("(%p): stub\n", iface);
    return E_NOTIMPL;
}

static BOOL WINAPI xmlreader_IsEmptyElement(IXmlReader* iface)
{
    FIXME("(%p): stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI xmlreader_GetLineNumber(IXmlReader* iface, UINT *lineNumber)
{
    xmlreader *This = impl_from_IXmlReader(iface);

    TRACE("(%p %p)\n", This, lineNumber);

    if (!lineNumber) return E_INVALIDARG;

    *lineNumber = This->line;

    return S_OK;
}

static HRESULT WINAPI xmlreader_GetLinePosition(IXmlReader* iface, UINT *linePosition)
{
    xmlreader *This = impl_from_IXmlReader(iface);

    TRACE("(%p %p)\n", This, linePosition);

    if (!linePosition) return E_INVALIDARG;

    *linePosition = This->pos;

    return S_OK;
}

static HRESULT WINAPI xmlreader_GetAttributeCount(IXmlReader* iface, UINT *attributeCount)
{
    FIXME("(%p %p): stub\n", iface, attributeCount);
    return E_NOTIMPL;
}

static HRESULT WINAPI xmlreader_GetDepth(IXmlReader* iface, UINT *depth)
{
    FIXME("(%p %p): stub\n", iface, depth);
    return E_NOTIMPL;
}

static BOOL WINAPI xmlreader_IsEOF(IXmlReader* iface)
{
    FIXME("(%p): stub\n", iface);
    return E_NOTIMPL;
}

static const struct IXmlReaderVtbl xmlreader_vtbl =
{
    xmlreader_QueryInterface,
    xmlreader_AddRef,
    xmlreader_Release,
    xmlreader_SetInput,
    xmlreader_GetProperty,
    xmlreader_SetProperty,
    xmlreader_Read,
    xmlreader_GetNodeType,
    xmlreader_MoveToFirstAttribute,
    xmlreader_MoveToNextAttribute,
    xmlreader_MoveToAttributeByName,
    xmlreader_MoveToElement,
    xmlreader_GetQualifiedName,
    xmlreader_GetNamespaceUri,
    xmlreader_GetLocalName,
    xmlreader_GetPrefix,
    xmlreader_GetValue,
    xmlreader_ReadValueChunk,
    xmlreader_GetBaseUri,
    xmlreader_IsDefault,
    xmlreader_IsEmptyElement,
    xmlreader_GetLineNumber,
    xmlreader_GetLinePosition,
    xmlreader_GetAttributeCount,
    xmlreader_GetDepth,
    xmlreader_IsEOF
};

/** IXmlReaderInput **/

/* Queries already stored interface for IStream/ISequentialStream.
   Interface supplied on creation will be overwritten */
static HRESULT xmlreaderinput_query_for_stream(IXmlReaderInput *iface, void **pObj)
{
    xmlreaderinput *This = impl_from_IXmlReaderInput(iface);
    HRESULT hr;

    hr = IUnknown_QueryInterface(This->input, &IID_IStream, pObj);
    if (hr != S_OK)
        hr = IUnknown_QueryInterface(This->input, &IID_ISequentialStream, pObj);

    return hr;
}

static HRESULT WINAPI xmlreaderinput_QueryInterface(IXmlReaderInput *iface, REFIID riid, void** ppvObject)
{
    xmlreaderinput *This = impl_from_IXmlReaderInput(iface);

    TRACE("%p %s %p\n", This, debugstr_guid(riid), ppvObject);

    if (IsEqualGUID(riid, &IID_IXmlReaderInput) ||
        IsEqualGUID(riid, &IID_IUnknown))
    {
        *ppvObject = iface;
    }
    else
    {
        FIXME("interface %s not implemented\n", debugstr_guid(riid));
        return E_NOINTERFACE;
    }

    IUnknown_AddRef(iface);

    return S_OK;
}

static ULONG WINAPI xmlreaderinput_AddRef(IXmlReaderInput *iface)
{
    xmlreaderinput *This = impl_from_IXmlReaderInput(iface);
    TRACE("%p\n", This);
    return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI xmlreaderinput_Release(IXmlReaderInput *iface)
{
    xmlreaderinput *This = impl_from_IXmlReaderInput(iface);
    LONG ref;

    TRACE("%p\n", This);

    ref = InterlockedDecrement(&This->ref);
    if (ref == 0)
    {
        if (This->input) IUnknown_Release(This->input);
        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}

static const struct IUnknownVtbl xmlreaderinput_vtbl =
{
    xmlreaderinput_QueryInterface,
    xmlreaderinput_AddRef,
    xmlreaderinput_Release
};

HRESULT WINAPI CreateXmlReader(REFIID riid, void **pObject, IMalloc *pMalloc)
{
    xmlreader *reader;

    TRACE("(%s, %p, %p)\n", wine_dbgstr_guid(riid), pObject, pMalloc);

    if (pMalloc) FIXME("custom IMalloc not supported yet\n");

    if (!IsEqualGUID(riid, &IID_IXmlReader))
    {
        ERR("Unexpected IID requested -> (%s)\n", wine_dbgstr_guid(riid));
        return E_FAIL;
    }

    reader = HeapAlloc(GetProcessHeap(), 0, sizeof (*reader));
    if(!reader) return E_OUTOFMEMORY;

    reader->lpVtbl = &xmlreader_vtbl;
    reader->ref = 1;
    reader->stream = NULL;
    reader->input = NULL;
    reader->state = XmlReadState_Closed;
    reader->line  = reader->pos = 0;

    *pObject = &reader->lpVtbl;

    TRACE("returning iface %p\n", *pObject);

    return S_OK;
}

HRESULT WINAPI CreateXmlReaderInputWithEncodingName(IUnknown *stream,
                                                    IMalloc *pMalloc,
                                                    LPCWSTR encoding,
                                                    BOOL hint,
                                                    LPCWSTR base_uri,
                                                    IXmlReaderInput **ppInput)
{
    xmlreaderinput *readerinput;

    FIXME("%p %p %s %d %s %p: stub\n", stream, pMalloc, wine_dbgstr_w(encoding),
                                       hint, wine_dbgstr_w(base_uri), ppInput);

    if (!stream || !ppInput) return E_INVALIDARG;

    readerinput = HeapAlloc(GetProcessHeap(), 0, sizeof (*readerinput));
    if(!readerinput) return E_OUTOFMEMORY;

    readerinput->lpVtbl = &xmlreaderinput_vtbl;
    readerinput->ref = 1;
    IUnknown_QueryInterface(stream, &IID_IUnknown, (void**)&readerinput->input);

    *ppInput = (IXmlReaderInput*)&readerinput->lpVtbl;

    TRACE("returning iface %p\n", *ppInput);

    return S_OK;
}
