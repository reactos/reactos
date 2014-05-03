/*
 * IXmlWriter implementation
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

#include "xmllite_private.h"

/* not defined in public headers */
DEFINE_GUID(IID_IXmlWriterOutput, 0xc1131708, 0x0f59, 0x477f, 0x93, 0x59, 0x7d, 0x33, 0x24, 0x51, 0xbc, 0x1a);

typedef struct
{
    IXmlWriterOutput IXmlWriterOutput_iface;
    LONG ref;
    IUnknown *output;
    IMalloc *imalloc;
    xml_encoding encoding;
} xmlwriteroutput;

typedef struct _xmlwriter
{
    IXmlWriter IXmlWriter_iface;
    LONG ref;
} xmlwriter;

static inline xmlwriter *impl_from_IXmlWriter(IXmlWriter *iface)
{
    return CONTAINING_RECORD(iface, xmlwriter, IXmlWriter_iface);
}

static inline xmlwriteroutput *impl_from_IXmlWriterOutput(IXmlWriterOutput *iface)
{
    return CONTAINING_RECORD(iface, xmlwriteroutput, IXmlWriterOutput_iface);
}

/* reader input memory allocation functions */
static inline void *writeroutput_alloc(xmlwriteroutput *output, size_t len)
{
    return m_alloc(output->imalloc, len);
}

static inline void writeroutput_free(xmlwriteroutput *output, void *mem)
{
    m_free(output->imalloc, mem);
}

static HRESULT WINAPI xmlwriter_QueryInterface(IXmlWriter *iface, REFIID riid, void **ppvObject)
{
    xmlwriter *This = impl_from_IXmlWriter(iface);

    TRACE("%p %s %p\n", This, debugstr_guid(riid), ppvObject);

    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_IXmlWriter))
    {
        *ppvObject = iface;
    }

    IXmlWriter_AddRef(iface);

    return S_OK;
}

static ULONG WINAPI xmlwriter_AddRef(IXmlWriter *iface)
{
    xmlwriter *This = impl_from_IXmlWriter(iface);
    TRACE("%p\n", This);
    return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI xmlwriter_Release(IXmlWriter *iface)
{
    xmlwriter *This = impl_from_IXmlWriter(iface);
    LONG ref;

    TRACE("%p\n", This);

    ref = InterlockedDecrement(&This->ref);
    if (ref == 0)
        heap_free(This);

    return ref;
}

/*** IXmlWriter methods ***/
static HRESULT WINAPI xmlwriter_SetOutput(IXmlWriter *iface, IUnknown *pOutput)
{
    xmlwriter *This = impl_from_IXmlWriter(iface);

    FIXME("%p %p\n", This, pOutput);

    return E_NOTIMPL;
}

static HRESULT WINAPI xmlwriter_GetProperty(IXmlWriter *iface, UINT nProperty, LONG_PTR *ppValue)
{
    xmlwriter *This = impl_from_IXmlWriter(iface);

    FIXME("%p %u %p\n", This, nProperty, ppValue);

    return E_NOTIMPL;
}

static HRESULT WINAPI xmlwriter_SetProperty(IXmlWriter *iface, UINT nProperty, LONG_PTR pValue)
{
    xmlwriter *This = impl_from_IXmlWriter(iface);

    FIXME("%p %u %lu\n", This, nProperty, pValue);

    return E_NOTIMPL;
}

static HRESULT WINAPI xmlwriter_WriteAttributes(IXmlWriter *iface, IXmlReader *pReader,
                                  BOOL fWriteDefaultAttributes)
{
    xmlwriter *This = impl_from_IXmlWriter(iface);

    FIXME("%p %p %d\n", This, pReader, fWriteDefaultAttributes);

    return E_NOTIMPL;
}

static HRESULT WINAPI xmlwriter_WriteAttributeString(IXmlWriter *iface, LPCWSTR pwszPrefix,
                                       LPCWSTR pwszLocalName, LPCWSTR pwszNamespaceUri,
                                       LPCWSTR pwszValue)
{
    xmlwriter *This = impl_from_IXmlWriter(iface);

    FIXME("%p %s %s %s %s\n", This, wine_dbgstr_w(pwszPrefix), wine_dbgstr_w(pwszLocalName),
                        wine_dbgstr_w(pwszNamespaceUri), wine_dbgstr_w(pwszValue));

    return E_NOTIMPL;
}

static HRESULT WINAPI xmlwriter_WriteCData(IXmlWriter *iface, LPCWSTR pwszText)
{
    xmlwriter *This = impl_from_IXmlWriter(iface);

    FIXME("%p %s\n", This, wine_dbgstr_w(pwszText));

    return E_NOTIMPL;
}

static HRESULT WINAPI xmlwriter_WriteCharEntity(IXmlWriter *iface, WCHAR wch)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI xmlwriter_WriteChars(IXmlWriter *iface, const WCHAR *pwch, UINT cwch)
{
    xmlwriter *This = impl_from_IXmlWriter(iface);

    FIXME("%p %s %d\n", This, wine_dbgstr_w(pwch), cwch);

    return E_NOTIMPL;
}

static HRESULT WINAPI xmlwriter_WriteComment(IXmlWriter *iface, LPCWSTR pwszComment)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI xmlwriter_WriteDocType(IXmlWriter *iface, LPCWSTR pwszName, LPCWSTR pwszPublicId,
                               LPCWSTR pwszSystemId, LPCWSTR pwszSubset)
{
    xmlwriter *This = impl_from_IXmlWriter(iface);

    FIXME("%p %s %s %s %s\n", This, wine_dbgstr_w(pwszName), wine_dbgstr_w(pwszPublicId),
                        wine_dbgstr_w(pwszSystemId), wine_dbgstr_w(pwszSubset));

    return E_NOTIMPL;
}

static HRESULT WINAPI xmlwriter_WriteElementString(IXmlWriter *iface, LPCWSTR pwszPrefix,
                                     LPCWSTR pwszLocalName, LPCWSTR pwszNamespaceUri,
                                     LPCWSTR pwszValue)
{
    xmlwriter *This = impl_from_IXmlWriter(iface);

    FIXME("%p %s %s %s %s\n", This, wine_dbgstr_w(pwszPrefix), wine_dbgstr_w(pwszLocalName),
                        wine_dbgstr_w(pwszNamespaceUri), wine_dbgstr_w(pwszValue));

    return E_NOTIMPL;
}

static HRESULT WINAPI xmlwriter_WriteEndDocument(IXmlWriter *iface)
{
    xmlwriter *This = impl_from_IXmlWriter(iface);

    FIXME("%p\n", This);

    return E_NOTIMPL;
}

static HRESULT WINAPI xmlwriter_WriteEndElement(IXmlWriter *iface)
{
    xmlwriter *This = impl_from_IXmlWriter(iface);

    FIXME("%p\n", This);

    return E_NOTIMPL;
}

static HRESULT WINAPI xmlwriter_WriteEntityRef(IXmlWriter *iface, LPCWSTR pwszName)
{
    xmlwriter *This = impl_from_IXmlWriter(iface);

    FIXME("%p %s\n", This, wine_dbgstr_w(pwszName));

    return E_NOTIMPL;
}

static HRESULT WINAPI xmlwriter_WriteFullEndElement(IXmlWriter *iface)
{
    xmlwriter *This = impl_from_IXmlWriter(iface);

    FIXME("%p\n", This);

    return E_NOTIMPL;
}

static HRESULT WINAPI xmlwriter_WriteName(IXmlWriter *iface, LPCWSTR pwszName)
{
    xmlwriter *This = impl_from_IXmlWriter(iface);

    FIXME("%p %s\n", This, wine_dbgstr_w(pwszName));

    return E_NOTIMPL;
}

static HRESULT WINAPI xmlwriter_WriteNmToken(IXmlWriter *iface, LPCWSTR pwszNmToken)
{
    xmlwriter *This = impl_from_IXmlWriter(iface);

    FIXME("%p %s\n", This, wine_dbgstr_w(pwszNmToken));

    return E_NOTIMPL;
}

static HRESULT WINAPI xmlwriter_WriteNode(IXmlWriter *iface, IXmlReader *pReader,
                            BOOL fWriteDefaultAttributes)
{
    xmlwriter *This = impl_from_IXmlWriter(iface);

    FIXME("%p %p %d\n", This, pReader, fWriteDefaultAttributes);

    return E_NOTIMPL;
}

static HRESULT WINAPI xmlwriter_WriteNodeShallow(IXmlWriter *iface, IXmlReader *pReader,
                                   BOOL fWriteDefaultAttributes)
{
    xmlwriter *This = impl_from_IXmlWriter(iface);

    FIXME("%p %p %d\n", This, pReader, fWriteDefaultAttributes);

    return E_NOTIMPL;
}

static HRESULT WINAPI xmlwriter_WriteProcessingInstruction(IXmlWriter *iface, LPCWSTR pwszName,
                                             LPCWSTR pwszText)
{
    xmlwriter *This = impl_from_IXmlWriter(iface);

    FIXME("%p %s %s\n", This, wine_dbgstr_w(pwszName), wine_dbgstr_w(pwszText));

    return E_NOTIMPL;
}

static HRESULT WINAPI xmlwriter_WriteQualifiedName(IXmlWriter *iface, LPCWSTR pwszLocalName,
                                     LPCWSTR pwszNamespaceUri)
{
    xmlwriter *This = impl_from_IXmlWriter(iface);

    FIXME("%p %s %s\n", This, wine_dbgstr_w(pwszLocalName), wine_dbgstr_w(pwszNamespaceUri));

    return E_NOTIMPL;
}

static HRESULT WINAPI xmlwriter_WriteRaw(IXmlWriter *iface, LPCWSTR pwszData)
{
    xmlwriter *This = impl_from_IXmlWriter(iface);

    FIXME("%p %s\n", This, wine_dbgstr_w(pwszData));

    return E_NOTIMPL;
}

static HRESULT WINAPI xmlwriter_WriteRawChars(IXmlWriter *iface,  const WCHAR *pwch, UINT cwch)
{
    xmlwriter *This = impl_from_IXmlWriter(iface);

    FIXME("%p %s %d\n", This, wine_dbgstr_w(pwch), cwch);

    return E_NOTIMPL;
}

static HRESULT WINAPI xmlwriter_WriteStartDocument(IXmlWriter *iface, XmlStandalone standalone)
{
    xmlwriter *This = impl_from_IXmlWriter(iface);

    FIXME("%p\n", This);

    return E_NOTIMPL;
}

static HRESULT WINAPI xmlwriter_WriteStartElement(IXmlWriter *iface, LPCWSTR pwszPrefix,
                                    LPCWSTR pwszLocalName, LPCWSTR pwszNamespaceUri)
{
    xmlwriter *This = impl_from_IXmlWriter(iface);

    FIXME("%p %s %s %s\n", This, wine_dbgstr_w(pwszPrefix), wine_dbgstr_w(pwszLocalName),
                wine_dbgstr_w(pwszNamespaceUri));

    return E_NOTIMPL;
}

static HRESULT WINAPI xmlwriter_WriteString(IXmlWriter *iface, LPCWSTR pwszText)
{
    xmlwriter *This = impl_from_IXmlWriter(iface);

    FIXME("%p %s\n", This, wine_dbgstr_w(pwszText));

    return E_NOTIMPL;
}

static HRESULT WINAPI xmlwriter_WriteSurrogateCharEntity(IXmlWriter *iface, WCHAR wchLow, WCHAR wchHigh)
{
    xmlwriter *This = impl_from_IXmlWriter(iface);

    FIXME("%p %d %d\n", This, wchLow, wchHigh);

    return E_NOTIMPL;
}

static HRESULT WINAPI xmlwriter_WriteWhitespace(IXmlWriter *iface, LPCWSTR pwszWhitespace)
{
    xmlwriter *This = impl_from_IXmlWriter(iface);

    FIXME("%p %s\n", This, wine_dbgstr_w(pwszWhitespace));

    return E_NOTIMPL;
}

static HRESULT WINAPI xmlwriter_Flush(IXmlWriter *iface)
{
    xmlwriter *This = impl_from_IXmlWriter(iface);

    FIXME("%p\n", This);

    return E_NOTIMPL;
}

static const struct IXmlWriterVtbl xmlwriter_vtbl =
{
    xmlwriter_QueryInterface,
    xmlwriter_AddRef,
    xmlwriter_Release,
    xmlwriter_SetOutput,
    xmlwriter_GetProperty,
    xmlwriter_SetProperty,
    xmlwriter_WriteAttributes,
    xmlwriter_WriteAttributeString,
    xmlwriter_WriteCData,
    xmlwriter_WriteCharEntity,
    xmlwriter_WriteChars,
    xmlwriter_WriteComment,
    xmlwriter_WriteDocType,
    xmlwriter_WriteElementString,
    xmlwriter_WriteEndDocument,
    xmlwriter_WriteEndElement,
    xmlwriter_WriteEntityRef,
    xmlwriter_WriteFullEndElement,
    xmlwriter_WriteName,
    xmlwriter_WriteNmToken,
    xmlwriter_WriteNode,
    xmlwriter_WriteNodeShallow,
    xmlwriter_WriteProcessingInstruction,
    xmlwriter_WriteQualifiedName,
    xmlwriter_WriteRaw,
    xmlwriter_WriteRawChars,
    xmlwriter_WriteStartDocument,
    xmlwriter_WriteStartElement,
    xmlwriter_WriteString,
    xmlwriter_WriteSurrogateCharEntity,
    xmlwriter_WriteWhitespace,
    xmlwriter_Flush
};

/** IXmlWriterOutput **/
static HRESULT WINAPI xmlwriteroutput_QueryInterface(IXmlWriterOutput *iface, REFIID riid, void** ppvObject)
{
    xmlwriteroutput *This = impl_from_IXmlWriterOutput(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppvObject);

    if (IsEqualGUID(riid, &IID_IXmlWriterOutput) ||
        IsEqualGUID(riid, &IID_IUnknown))
    {
        *ppvObject = iface;
    }
    else
    {
        WARN("interface %s not implemented\n", debugstr_guid(riid));
        *ppvObject = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef(iface);

    return S_OK;
}

static ULONG WINAPI xmlwriteroutput_AddRef(IXmlWriterOutput *iface)
{
    xmlwriteroutput *This = impl_from_IXmlWriterOutput(iface);
    ULONG ref = InterlockedIncrement(&This->ref);
    TRACE("(%p)->(%d)\n", This, ref);
    return ref;
}

static ULONG WINAPI xmlwriteroutput_Release(IXmlWriterOutput *iface)
{
    xmlwriteroutput *This = impl_from_IXmlWriterOutput(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(%d)\n", This, ref);

    if (ref == 0)
    {
        IMalloc *imalloc = This->imalloc;
        if (This->output) IUnknown_Release(This->output);
        writeroutput_free(This, This);
        if (imalloc) IMalloc_Release(imalloc);
    }

    return ref;
}

static const struct IUnknownVtbl xmlwriteroutputvtbl =
{
    xmlwriteroutput_QueryInterface,
    xmlwriteroutput_AddRef,
    xmlwriteroutput_Release
};

HRESULT WINAPI CreateXmlWriter(REFIID riid, void **pObject, IMalloc *pMalloc)
{
    xmlwriter *writer;

    TRACE("(%s, %p, %p)\n", wine_dbgstr_guid(riid), pObject, pMalloc);

    if (pMalloc) FIXME("custom IMalloc not supported yet\n");

    if (!IsEqualGUID(riid, &IID_IXmlWriter))
    {
        ERR("Unexpected IID requested -> (%s)\n", wine_dbgstr_guid(riid));
        return E_FAIL;
    }

    writer = heap_alloc(sizeof(*writer));
    if(!writer) return E_OUTOFMEMORY;

    writer->IXmlWriter_iface.lpVtbl = &xmlwriter_vtbl;
    writer->ref = 1;

    *pObject = &writer->IXmlWriter_iface;

    TRACE("returning iface %p\n", *pObject);

    return S_OK;
}

HRESULT WINAPI CreateXmlWriterOutputWithEncodingName(IUnknown *stream,
                                                     IMalloc *imalloc,
                                                     LPCWSTR encoding,
                                                     IXmlWriterOutput **output)
{
    xmlwriteroutput *writeroutput;

    TRACE("%p %p %s %p\n", stream, imalloc, debugstr_w(encoding), output);

    if (!stream || !output) return E_INVALIDARG;

    if (imalloc)
        writeroutput = IMalloc_Alloc(imalloc, sizeof(*writeroutput));
    else
        writeroutput = heap_alloc(sizeof(*writeroutput));
    if(!writeroutput) return E_OUTOFMEMORY;

    writeroutput->IXmlWriterOutput_iface.lpVtbl = &xmlwriteroutputvtbl;
    writeroutput->ref = 1;
    writeroutput->imalloc = imalloc;
    if (imalloc) IMalloc_AddRef(imalloc);
    writeroutput->encoding = parse_encoding_name(encoding, -1);

    IUnknown_QueryInterface(stream, &IID_IUnknown, (void**)&writeroutput->output);

    *output = &writeroutput->IXmlWriterOutput_iface;

    TRACE("returning iface %p\n", *output);

    return S_OK;
}
