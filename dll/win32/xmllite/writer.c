/*
 * IXmlWriter implementation
 *
 * Copyright 2011 Alistair Leslie-Hughes
 * Copyright 2014-2018 Nikolay Sivov for CodeWeavers
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

#include <assert.h>
#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "objbase.h"
#include "xmllite.h"
#include "xmllite_private.h"
#ifdef __REACTOS__
#include <wchar.h>
#include <winnls.h>
#endif
#include "initguid.h"

#include "wine/debug.h"
#include "wine/list.h"

WINE_DEFAULT_DEBUG_CHANNEL(xmllite);

/* not defined in public headers */
DEFINE_GUID(IID_IXmlWriterOutput, 0xc1131708, 0x0f59, 0x477f, 0x93, 0x59, 0x7d, 0x33, 0x24, 0x51, 0xbc, 0x1a);

static const WCHAR xmlnsuriW[] = L"http://www.w3.org/2000/xmlns/";

struct output_buffer
{
    char *data;
    unsigned int allocated;
    unsigned int written;
    UINT codepage;
};

typedef enum
{
    XmlWriterState_Initial,         /* output is not set yet */
    XmlWriterState_Ready,           /* SetOutput() was called, ready to start */
    XmlWriterState_InvalidEncoding, /* SetOutput() was called, but output had invalid encoding */
    XmlWriterState_PIDocStarted,    /* document was started with manually added 'xml' PI */
    XmlWriterState_DocStarted,      /* document was started with WriteStartDocument() */
    XmlWriterState_ElemStarted,     /* writing element */
    XmlWriterState_Content,         /* content is accepted at this point */
    XmlWriterState_DocClosed        /* WriteEndDocument was called */
} XmlWriterState;

typedef struct
{
    IXmlWriterOutput IXmlWriterOutput_iface;
    LONG ref;
    IUnknown *output;
    ISequentialStream *stream;
    IMalloc *imalloc;
    xml_encoding encoding;
    WCHAR *encoding_name; /* exactly as specified on output creation */
    struct output_buffer buffer;
    DWORD written : 1;
} xmlwriteroutput;

static const struct IUnknownVtbl xmlwriteroutputvtbl;

struct element
{
    struct list entry;
    WCHAR *qname;
    unsigned int len; /* qname length in chars */
    struct list ns;
};

struct ns
{
    struct list entry;
    WCHAR *prefix;
    int prefix_len;
    WCHAR *uri;
    BOOL emitted;
    struct element *element;
};

typedef struct _xmlwriter
{
    IXmlWriter IXmlWriter_iface;
    LONG ref;
    IMalloc *imalloc;
    xmlwriteroutput *output;
    unsigned int indent_level;
    BOOL indent;
    BOOL bom;
    BOOL omitxmldecl;
    XmlConformanceLevel conformance;
    XmlWriterState state;
    struct list elements;
    DWORD bomwritten : 1;
    DWORD starttagopen : 1;
    DWORD textnode : 1;
} xmlwriter;

static inline xmlwriter *impl_from_IXmlWriter(IXmlWriter *iface)
{
    return CONTAINING_RECORD(iface, xmlwriter, IXmlWriter_iface);
}

static inline xmlwriteroutput *impl_from_IXmlWriterOutput(IXmlWriterOutput *iface)
{
    return CONTAINING_RECORD(iface, xmlwriteroutput, IXmlWriterOutput_iface);
}

static const char *debugstr_writer_prop(XmlWriterProperty prop)
{
    static const char * const prop_names[] =
    {
        "MultiLanguage",
        "Indent",
        "ByteOrderMark",
        "OmitXmlDeclaration",
        "ConformanceLevel"
    };

    if (prop > _XmlWriterProperty_Last)
        return wine_dbg_sprintf("unknown property=%d", prop);

    return prop_names[prop];
}

static HRESULT create_writer_output(IUnknown *stream, IMalloc *imalloc, xml_encoding encoding,
    const WCHAR *encoding_name, xmlwriteroutput **out);

/* writer output memory allocation functions */
static inline void *writeroutput_alloc(xmlwriteroutput *output, size_t len)
{
    return m_alloc(output->imalloc, len);
}

static inline void writeroutput_free(xmlwriteroutput *output, void *mem)
{
    m_free(output->imalloc, mem);
}

static inline void *writeroutput_realloc(xmlwriteroutput *output, void *mem, size_t len)
{
    return m_realloc(output->imalloc, mem, len);
}

/* writer memory allocation functions */
static inline void *writer_alloc(const xmlwriter *writer, size_t len)
{
    return m_alloc(writer->imalloc, len);
}

static inline void writer_free(const xmlwriter *writer, void *mem)
{
    m_free(writer->imalloc, mem);
}

static BOOL is_empty_string(const WCHAR *str)
{
    return !str || !*str;
}

static struct element *alloc_element(xmlwriter *writer, const WCHAR *prefix, const WCHAR *local)
{
    struct element *ret;
    int len;

    ret = writer_alloc(writer, sizeof(*ret));
    if (!ret) return ret;

    len = is_empty_string(prefix) ? 0 : lstrlenW(prefix) + 1 /* ':' */;
    len += lstrlenW(local);

    ret->qname = writer_alloc(writer, (len + 1)*sizeof(WCHAR));

    if (!ret->qname)
    {
        writer_free(writer, ret);
        return NULL;
    }

    ret->len = len;
    if (is_empty_string(prefix))
        ret->qname[0] = 0;
    else
    {
        lstrcpyW(ret->qname, prefix);
        lstrcatW(ret->qname, L":");
    }
    lstrcatW(ret->qname, local);
    list_init(&ret->ns);

    return ret;
}

static void writer_free_element(xmlwriter *writer, struct element *element)
{
    struct ns *ns, *ns2;

    LIST_FOR_EACH_ENTRY_SAFE(ns, ns2, &element->ns, struct ns, entry)
    {
        list_remove(&ns->entry);
        writer_free(writer, ns->prefix);
        writer_free(writer, ns->uri);
        writer_free(writer, ns);
    }

    writer_free(writer, element->qname);
    writer_free(writer, element);
}

static void writer_free_element_stack(xmlwriter *writer)
{
    struct element *element, *element2;

    LIST_FOR_EACH_ENTRY_SAFE(element, element2, &writer->elements, struct element, entry)
    {
        list_remove(&element->entry);
        writer_free_element(writer, element);
    }
}

static void writer_push_element(xmlwriter *writer, struct element *element)
{
    list_add_head(&writer->elements, &element->entry);
}

static struct element *pop_element(xmlwriter *writer)
{
    struct element *element = LIST_ENTRY(list_head(&writer->elements), struct element, entry);

    if (element)
        list_remove(&element->entry);

    return element;
}

static WCHAR *writer_strndupW(const xmlwriter *writer, const WCHAR *str, int len)
{
    WCHAR *ret;

    if (!str)
        return NULL;

    if (len == -1)
        len = lstrlenW(str);

    ret = writer_alloc(writer, (len + 1) * sizeof(WCHAR));
    if (ret)
    {
        memcpy(ret, str, len * sizeof(WCHAR));
        ret[len] = 0;
    }

    return ret;
}

static WCHAR *writer_strdupW(const xmlwriter *writer, const WCHAR *str)
{
    return writer_strndupW(writer, str, -1);
}

static struct ns *writer_push_ns(xmlwriter *writer, const WCHAR *prefix, int prefix_len, const WCHAR *uri)
{
    struct element *element;
    struct ns *ns;

    element = LIST_ENTRY(list_head(&writer->elements), struct element, entry);
    if (!element)
        return NULL;

    if ((ns = writer_alloc(writer, sizeof(*ns))))
    {
        ns->prefix = writer_strndupW(writer, prefix, prefix_len);
        ns->prefix_len = prefix_len;
        ns->uri = writer_strdupW(writer, uri);
        ns->emitted = FALSE;
        ns->element = element;
        list_add_tail(&element->ns, &ns->entry);
    }

    return ns;
}

static struct ns *writer_find_ns_current(const xmlwriter *writer, const WCHAR *prefix, const WCHAR *uri)
{
    struct element *element;
    struct ns *ns;

    if (is_empty_string(prefix) || is_empty_string(uri))
        return NULL;

    element = LIST_ENTRY(list_head(&writer->elements), struct element, entry);

    LIST_FOR_EACH_ENTRY(ns, &element->ns, struct ns, entry)
    {
        if (!wcscmp(uri, ns->uri) && !wcscmp(prefix, ns->prefix))
            return ns;
    }

    return NULL;
}

static struct ns *writer_find_ns(const xmlwriter *writer, const WCHAR *prefix, const WCHAR *uri)
{
    struct element *element;
    struct ns *ns;

    if (is_empty_string(prefix) && is_empty_string(uri))
        return NULL;

    LIST_FOR_EACH_ENTRY(element, &writer->elements, struct element, entry)
    {
        LIST_FOR_EACH_ENTRY(ns, &element->ns, struct ns, entry)
        {
            if (!uri)
            {
                if (!ns->prefix) continue;
                if (!wcscmp(ns->prefix, prefix))
                    return ns;
            }
            else if (!wcscmp(uri, ns->uri))
            {
                if (prefix && !*prefix)
                    return NULL;
                if (!prefix || !wcscmp(prefix, ns->prefix))
                    return ns;
            }
        }
    }

    return NULL;
}

static HRESULT is_valid_ncname(const WCHAR *str, int *out)
{
    int len = 0;

    *out = 0;

    if (!str || !*str)
        return S_OK;

    while (*str)
    {
        if (!is_ncnamechar(*str))
            return WC_E_NAMECHARACTER;
        len++;
        str++;
    }

    *out = len;
    return S_OK;
}

static HRESULT is_valid_name(const WCHAR *str, unsigned int *out)
{
    unsigned int len = 1;

    *out = 0;

    if (!str || !*str)
        return S_OK;

    if (!is_namestartchar(*str++))
        return WC_E_NAMECHARACTER;

    while (*str)
    {
        if (!is_namechar(*str))
            return WC_E_NAMECHARACTER;
        len++;
        str++;
    }

    *out = len;
    return S_OK;
}

static HRESULT is_valid_pubid(const WCHAR *str, unsigned int *out)
{
    unsigned int len = 0;

    *out = 0;

    if (!str || !*str)
        return S_OK;

    while (*str)
    {
        if (!is_pubchar(*str++))
            return WC_E_PUBLICID;
        len++;
    }

    *out = len;

    return S_OK;
}

static HRESULT init_output_buffer(xmlwriteroutput *output)
{
    struct output_buffer *buffer = &output->buffer;
    const int initial_len = 0x2000;
    UINT cp = ~0u;
    HRESULT hr;

    if (FAILED(hr = get_code_page(output->encoding, &cp)))
        WARN("Failed to get code page for specified encoding.\n");

    buffer->data = writeroutput_alloc(output, initial_len);
    if (!buffer->data) return E_OUTOFMEMORY;

    memset(buffer->data, 0, 4);
    buffer->allocated = initial_len;
    buffer->written = 0;
    buffer->codepage = cp;

    return S_OK;
}

static void free_output_buffer(xmlwriteroutput *output)
{
    struct output_buffer *buffer = &output->buffer;
    writeroutput_free(output, buffer->data);
    buffer->data = NULL;
    buffer->allocated = 0;
    buffer->written = 0;
}

static HRESULT grow_output_buffer(xmlwriteroutput *output, int length)
{
    struct output_buffer *buffer = &output->buffer;
    /* grow if needed, plus 4 bytes to be sure null terminator will fit in */
    if (buffer->allocated < buffer->written + length + 4) {
        int grown_size = max(2*buffer->allocated, buffer->allocated + length);
        char *ptr = writeroutput_realloc(output, buffer->data, grown_size);
        if (!ptr) return E_OUTOFMEMORY;
        buffer->data = ptr;
        buffer->allocated = grown_size;
    }

    return S_OK;
}

static HRESULT write_output_buffer(xmlwriteroutput *output, const WCHAR *data, int len)
{
    struct output_buffer *buffer = &output->buffer;
    int length;
    HRESULT hr;
    char *ptr;

    if (buffer->codepage == 1200) {
        /* For UTF-16 encoding just copy. */
        length = len == -1 ? lstrlenW(data) : len;
        if (length) {
            length *= sizeof(WCHAR);

            hr = grow_output_buffer(output, length);
            if (FAILED(hr)) return hr;
            ptr = buffer->data + buffer->written;

            memcpy(ptr, data, length);
            buffer->written += length;
            ptr += length;
            /* null termination */
            *(WCHAR*)ptr = 0;
        }
    }
    else {
        length = WideCharToMultiByte(buffer->codepage, 0, data, len, NULL, 0, NULL, NULL);
        hr = grow_output_buffer(output, length);
        if (FAILED(hr)) return hr;
        ptr = buffer->data + buffer->written;
        length = WideCharToMultiByte(buffer->codepage, 0, data, len, ptr, length, NULL, NULL);
        buffer->written += len == -1 ? length-1 : length;
    }
    output->written = length != 0;

    return S_OK;
}

static HRESULT write_output_buffer_char(xmlwriteroutput *output, WCHAR ch)
{
    return write_output_buffer(output, &ch, 1);
}

static HRESULT write_output_buffer_quoted(xmlwriteroutput *output, const WCHAR *data, int len)
{
    write_output_buffer_char(output, '"');
    if (!is_empty_string(data))
        write_output_buffer(output, data, len);
    write_output_buffer_char(output, '"');
    return S_OK;
}

/* TODO: test if we need to validate char range */
static HRESULT write_output_qname(xmlwriteroutput *output, const WCHAR *prefix, int prefix_len,
        const WCHAR *local_name, int local_len)
{
    assert(prefix_len >= 0 && local_len >= 0);

    if (prefix_len)
        write_output_buffer(output, prefix, prefix_len);

    if (prefix_len && local_len)
        write_output_buffer_char(output, ':');

    write_output_buffer(output, local_name, local_len);

    return S_OK;
}

static void writeroutput_release_stream(xmlwriteroutput *writeroutput)
{
    if (writeroutput->stream) {
        ISequentialStream_Release(writeroutput->stream);
        writeroutput->stream = NULL;
    }
}

static inline HRESULT writeroutput_query_for_stream(xmlwriteroutput *writeroutput)
{
    HRESULT hr;

    writeroutput_release_stream(writeroutput);
    hr = IUnknown_QueryInterface(writeroutput->output, &IID_IStream, (void**)&writeroutput->stream);
    if (hr != S_OK)
        hr = IUnknown_QueryInterface(writeroutput->output, &IID_ISequentialStream, (void**)&writeroutput->stream);

    return hr;
}

static HRESULT writeroutput_flush_stream(xmlwriteroutput *output)
{
    struct output_buffer *buffer;
    ULONG written, offset = 0;
    HRESULT hr;

    if (!output || !output->stream)
        return S_OK;

    buffer = &output->buffer;

    /* It will loop forever until everything is written or an error occurred. */
    do {
        written = 0;
        hr = ISequentialStream_Write(output->stream, buffer->data + offset, buffer->written, &written);
        if (FAILED(hr)) {
            WARN("write to stream failed %#lx.\n", hr);
            buffer->written = 0;
            return hr;
        }

        offset += written;
        buffer->written -= written;
    } while (buffer->written > 0);

    return S_OK;
}

static HRESULT write_encoding_bom(xmlwriter *writer)
{
    if (!writer->bom || writer->bomwritten) return S_OK;

    if (writer->output->encoding == XmlEncoding_UTF16) {
        static const char utf16bom[] = {0xff, 0xfe};
        struct output_buffer *buffer = &writer->output->buffer;
        int len = sizeof(utf16bom);
        HRESULT hr;

        hr = grow_output_buffer(writer->output, len);
        if (FAILED(hr)) return hr;
        memcpy(buffer->data + buffer->written, utf16bom, len);
        buffer->written += len;
    }

    writer->bomwritten = TRUE;
    return S_OK;
}

static const WCHAR *get_output_encoding_name(xmlwriteroutput *output)
{
    if (output->encoding_name)
        return output->encoding_name;

    return get_encoding_name(output->encoding);
}

static HRESULT write_xmldecl(xmlwriter *writer, XmlStandalone standalone)
{
    write_encoding_bom(writer);
    writer->state = XmlWriterState_DocStarted;
    if (writer->omitxmldecl) return S_OK;

    /* version */
    write_output_buffer(writer->output, L"<?xml version=\"1.0\"", 19);

    /* encoding */
    write_output_buffer(writer->output, L" encoding=", 10);
    write_output_buffer_quoted(writer->output, get_output_encoding_name(writer->output), -1);

    /* standalone */
    if (standalone == XmlStandalone_Omit)
        write_output_buffer(writer->output, L"?>", 2);
    else
    {
        write_output_buffer(writer->output, L" standalone=\"", 13);
        if (standalone == XmlStandalone_Yes)
            write_output_buffer(writer->output, L"yes\"?>", 6);
        else
            write_output_buffer(writer->output, L"no\"?>", 5);
    }

    return S_OK;
}

static void writer_output_ns(xmlwriter *writer, struct element *element)
{
    struct ns *ns;

    LIST_FOR_EACH_ENTRY(ns, &element->ns, struct ns, entry)
    {
        if (ns->emitted)
            continue;

        write_output_qname(writer->output, L" xmlns", 6, ns->prefix, ns->prefix_len);
        write_output_buffer_char(writer->output, '=');
        write_output_buffer_quoted(writer->output, ns->uri, -1);
    }
}

static HRESULT writer_close_starttag(xmlwriter *writer)
{
    HRESULT hr;

    if (!writer->starttagopen) return S_OK;

    writer_output_ns(writer, LIST_ENTRY(list_head(&writer->elements), struct element, entry));
    hr = write_output_buffer_char(writer->output, '>');
    writer->starttagopen = 0;
    return hr;
}

static void writer_inc_indent(xmlwriter *writer)
{
    writer->indent_level++;
}

static void writer_dec_indent(xmlwriter *writer)
{
    if (writer->indent_level)
        writer->indent_level--;
}

static void write_node_indent(xmlwriter *writer)
{
    unsigned int indent_level = writer->indent_level;

    if (!writer->indent || writer->textnode)
    {
        writer->textnode = 0;
        return;
    }

    /* Do state check to prevent newline inserted after BOM. It is assumed that
       state does not change between writing BOM and inserting indentation. */
    if (writer->output->written && writer->state != XmlWriterState_Ready)
        write_output_buffer(writer->output, L"\r\n", 2);
    while (indent_level--)
        write_output_buffer(writer->output, L"  ", 2);

    writer->textnode = 0;
}

static HRESULT WINAPI xmlwriter_QueryInterface(IXmlWriter *iface, REFIID riid, void **ppvObject)
{
    xmlwriter *This = impl_from_IXmlWriter(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppvObject);

    if (IsEqualGUID(riid, &IID_IXmlWriter) ||
        IsEqualGUID(riid, &IID_IUnknown))
    {
        *ppvObject = iface;
    }
    else
    {
        FIXME("interface %s is not supported\n", debugstr_guid(riid));
        *ppvObject = NULL;
        return E_NOINTERFACE;
    }

    IXmlWriter_AddRef(iface);

    return S_OK;
}

static ULONG WINAPI xmlwriter_AddRef(IXmlWriter *iface)
{
    xmlwriter *writer = impl_from_IXmlWriter(iface);
    ULONG ref = InterlockedIncrement(&writer->ref);
    TRACE("%p, refcount %lu.\n", iface, ref);
    return ref;
}

static ULONG WINAPI xmlwriter_Release(IXmlWriter *iface)
{
    xmlwriter *writer = impl_from_IXmlWriter(iface);
    ULONG ref = InterlockedDecrement(&writer->ref);

    TRACE("%p, refcount %lu.\n", iface, ref);

    if (!ref)
    {
        IMalloc *imalloc = writer->imalloc;

        writeroutput_flush_stream(writer->output);
        if (writer->output)
            IUnknown_Release(&writer->output->IXmlWriterOutput_iface);

        writer_free_element_stack(writer);

        writer_free(writer, writer);
        if (imalloc) IMalloc_Release(imalloc);
    }

    return ref;
}

/*** IXmlWriter methods ***/
static HRESULT WINAPI xmlwriter_SetOutput(IXmlWriter *iface, IUnknown *output)
{
    xmlwriter *This = impl_from_IXmlWriter(iface);
    IXmlWriterOutput *writeroutput;
    HRESULT hr;

    TRACE("(%p)->(%p)\n", This, output);

    if (This->output) {
        writeroutput_release_stream(This->output);
        IUnknown_Release(&This->output->IXmlWriterOutput_iface);
        This->output = NULL;
        This->bomwritten = 0;
        This->textnode = 0;
        This->indent_level = 0;
        writer_free_element_stack(This);
    }

    /* just reset current output */
    if (!output) {
        This->state = XmlWriterState_Initial;
        return S_OK;
    }

    /* now try IXmlWriterOutput, ISequentialStream, IStream */
    hr = IUnknown_QueryInterface(output, &IID_IXmlWriterOutput, (void**)&writeroutput);
    if (hr == S_OK) {
        if (writeroutput->lpVtbl == &xmlwriteroutputvtbl)
            This->output = impl_from_IXmlWriterOutput(writeroutput);
        else {
            ERR("got external IXmlWriterOutput implementation: %p, vtbl=%p\n",
                writeroutput, writeroutput->lpVtbl);
            IUnknown_Release(writeroutput);
            return E_FAIL;
        }
    }

    if (hr != S_OK || !writeroutput) {
        /* Create output for given stream. */
        hr = create_writer_output(output, This->imalloc, XmlEncoding_UTF8, NULL, &This->output);
        if (hr != S_OK)
            return hr;
    }

    if (This->output->encoding == XmlEncoding_Unknown)
        This->state = XmlWriterState_InvalidEncoding;
    else
        This->state = XmlWriterState_Ready;
    return writeroutput_query_for_stream(This->output);
}

static HRESULT WINAPI xmlwriter_GetProperty(IXmlWriter *iface, UINT property, LONG_PTR *value)
{
    xmlwriter *This = impl_from_IXmlWriter(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_writer_prop(property), value);

    if (!value) return E_INVALIDARG;

    switch (property)
    {
        case XmlWriterProperty_Indent:
            *value = This->indent;
            break;
        case XmlWriterProperty_ByteOrderMark:
            *value = This->bom;
            break;
        case XmlWriterProperty_OmitXmlDeclaration:
            *value = This->omitxmldecl;
            break;
        case XmlWriterProperty_ConformanceLevel:
            *value = This->conformance;
            break;
        default:
            FIXME("Unimplemented property (%u)\n", property);
            return E_NOTIMPL;
    }

    return S_OK;
}

static HRESULT WINAPI xmlwriter_SetProperty(IXmlWriter *iface, UINT property, LONG_PTR value)
{
    xmlwriter *writer = impl_from_IXmlWriter(iface);

    TRACE("%p, %s, %Id.\n", iface, debugstr_writer_prop(property), value);

    switch (property)
    {
        case XmlWriterProperty_Indent:
            writer->indent = !!value;
            break;
        case XmlWriterProperty_ByteOrderMark:
            writer->bom = !!value;
            break;
        case XmlWriterProperty_OmitXmlDeclaration:
            writer->omitxmldecl = !!value;
            break;
        default:
            FIXME("Unimplemented property (%u)\n", property);
            return E_NOTIMPL;
    }

    return S_OK;
}

static HRESULT writer_write_attribute(IXmlWriter *writer, IXmlReader *reader, BOOL write_default_attributes)
{
    const WCHAR *prefix, *local, *uri, *value;
    HRESULT hr;

    if (IXmlReader_IsDefault(reader) && !write_default_attributes)
        return S_OK;

    if (FAILED(hr = IXmlReader_GetPrefix(reader, &prefix, NULL))) return hr;
    if (FAILED(hr = IXmlReader_GetLocalName(reader, &local, NULL))) return hr;
    if (FAILED(hr = IXmlReader_GetNamespaceUri(reader, &uri, NULL))) return hr;
    if (FAILED(hr = IXmlReader_GetValue(reader, &value, NULL))) return hr;
    return IXmlWriter_WriteAttributeString(writer, prefix, local, uri, value);
}

static HRESULT WINAPI xmlwriter_WriteAttributes(IXmlWriter *iface, IXmlReader *reader, BOOL write_default_attributes)
{
    XmlNodeType node_type;
    HRESULT hr = S_OK;

    TRACE("%p, %p, %d.\n", iface, reader, write_default_attributes);

    if (FAILED(hr = IXmlReader_GetNodeType(reader, &node_type))) return hr;

    switch (node_type)
    {
        case XmlNodeType_Element:
        case XmlNodeType_XmlDeclaration:
        case XmlNodeType_Attribute:
            if (node_type != XmlNodeType_Attribute)
            {
                if (FAILED(hr = IXmlReader_MoveToFirstAttribute(reader))) return hr;
                if (hr == S_FALSE) return S_OK;
            }
            if (FAILED(hr = writer_write_attribute(iface, reader, write_default_attributes))) return hr;
            while (IXmlReader_MoveToNextAttribute(reader) == S_OK)
            {
                if (FAILED(hr = writer_write_attribute(iface, reader, write_default_attributes))) break;
            }
            if (node_type != XmlNodeType_Attribute && SUCCEEDED(hr))
                hr = IXmlReader_MoveToElement(reader);
            break;
        default:
            WARN("Unexpected node type %d.\n", node_type);
            return E_UNEXPECTED;
    }

    return hr;
}

static void write_output_attribute(xmlwriter *writer, const WCHAR *prefix, int prefix_len,
        const WCHAR *local, int local_len, const WCHAR *value)
{
    write_output_buffer_char(writer->output, ' ');
    write_output_qname(writer->output, prefix, prefix_len, local, local_len);
    write_output_buffer_char(writer->output, '=');
    write_output_buffer_quoted(writer->output, value, -1);
}

static BOOL is_valid_xml_space_value(const WCHAR *value)
{
    return value && (!wcscmp(value, L"preserve") || !wcscmp(value, L"default"));
}

static HRESULT WINAPI xmlwriter_WriteAttributeString(IXmlWriter *iface, LPCWSTR prefix,
    LPCWSTR local, LPCWSTR uri, LPCWSTR value)
{
    xmlwriter *writer = impl_from_IXmlWriter(iface);
    BOOL is_xmlns_prefix, is_xmlns_local;
    int prefix_len, local_len;
    struct ns *ns;
    HRESULT hr;

    TRACE("%p, %s, %s, %s, %s.\n", iface, debugstr_w(prefix), debugstr_w(local), debugstr_w(uri), debugstr_w(value));

    switch (writer->state)
    {
    case XmlWriterState_Initial:
        return E_UNEXPECTED;
    case XmlWriterState_Ready:
    case XmlWriterState_DocClosed:
        writer->state = XmlWriterState_DocClosed;
        return WR_E_INVALIDACTION;
    case XmlWriterState_InvalidEncoding:
        return MX_E_ENCODING;
    default:
        ;
    }

    /* Prefix "xmlns" */
    is_xmlns_prefix = prefix && !wcscmp(prefix, L"xmlns");
    if (is_xmlns_prefix && is_empty_string(uri) && is_empty_string(local))
        return WR_E_NSPREFIXDECLARED;

    if (is_empty_string(local))
        return E_INVALIDARG;

    /* Validate prefix and local name */
    if (FAILED(hr = is_valid_ncname(prefix, &prefix_len)))
        return hr;

    if (FAILED(hr = is_valid_ncname(local, &local_len)))
        return hr;

    is_xmlns_local = !wcscmp(local, L"xmlns");

    /* Trivial case, no prefix. */
    if (prefix_len == 0 && is_empty_string(uri))
    {
        write_output_attribute(writer, prefix, prefix_len, local, local_len, value);
        return S_OK;
    }

    /* Predefined "xml" prefix. */
    if (prefix_len && !wcscmp(prefix, L"xml"))
    {
        /* Valid "space" value is enforced. */
        if (!wcscmp(local, L"space") && !is_valid_xml_space_value(value))
            return WR_E_INVALIDXMLSPACE;

        /* Redefinition is not allowed. */
        if (!is_empty_string(uri))
            return WR_E_XMLPREFIXDECLARATION;

        write_output_attribute(writer, prefix, prefix_len, local, local_len, value);

        return S_OK;
    }

    if (is_xmlns_prefix || (prefix_len == 0 && uri && !wcscmp(uri, xmlnsuriW)))
    {
        if (prefix_len && !is_empty_string(uri))
            return WR_E_XMLNSPREFIXDECLARATION;

        /* Look for exact match defined in current element, and write it out. */
        if (!(ns = writer_find_ns_current(writer, prefix, value)))
            ns = writer_push_ns(writer, local, local_len, value);
        ns->emitted = TRUE;

        write_output_attribute(writer, L"xmlns", 5, local, local_len, value);

        return S_OK;
    }

    /* Ignore prefix if URI wasn't specified. */
    if (is_xmlns_local && is_empty_string(uri))
    {
        write_output_attribute(writer, NULL, 0, L"xmlns", 5, value);
        return S_OK;
    }

    if (!(ns = writer_find_ns(writer, prefix, uri)))
    {
        if (is_empty_string(prefix) && !is_empty_string(uri))
        {
            FIXME("Prefix autogeneration is not implemented.\n");
            return E_NOTIMPL;
        }
        if (!is_empty_string(uri))
            ns = writer_push_ns(writer, prefix, prefix_len, uri);
    }

    if (ns)
        write_output_attribute(writer, ns->prefix, ns->prefix_len, local, local_len, value);
    else
        write_output_attribute(writer, prefix, prefix_len, local, local_len, value);

    return S_OK;
}

static void write_cdata_section(xmlwriteroutput *output, const WCHAR *data, int len)
{
    write_output_buffer(output, L"<![CDATA[", 9);
    if (data)
        write_output_buffer(output, data, len);
    write_output_buffer(output, L"]]>", 3);
}

static HRESULT WINAPI xmlwriter_WriteCData(IXmlWriter *iface, LPCWSTR data)
{
    xmlwriter *This = impl_from_IXmlWriter(iface);
    int len;

    TRACE("%p %s\n", This, debugstr_w(data));

    switch (This->state)
    {
    case XmlWriterState_Initial:
        return E_UNEXPECTED;
    case XmlWriterState_ElemStarted:
        writer_close_starttag(This);
        break;
    case XmlWriterState_Ready:
    case XmlWriterState_DocClosed:
        This->state = XmlWriterState_DocClosed;
        return WR_E_INVALIDACTION;
    case XmlWriterState_InvalidEncoding:
        return MX_E_ENCODING;
    default:
        ;
    }

    len = data ? lstrlenW(data) : 0;

    write_node_indent(This);
    if (!len)
        write_cdata_section(This->output, NULL, 0);
    else
    {
        while (len)
        {
            const WCHAR *str = wcsstr(data, L"]]>");
            if (str) {
                str += 2;
                write_cdata_section(This->output, data, str - data);
                len -= str - data;
                data = str;
            }
            else {
                write_cdata_section(This->output, data, len);
                break;
            }
        }
    }

    return S_OK;
}

static HRESULT WINAPI xmlwriter_WriteCharEntity(IXmlWriter *iface, WCHAR ch)
{
    xmlwriter *This = impl_from_IXmlWriter(iface);
    WCHAR bufW[16];

    TRACE("%p %#x\n", This, ch);

    switch (This->state)
    {
    case XmlWriterState_Initial:
        return E_UNEXPECTED;
    case XmlWriterState_InvalidEncoding:
        return MX_E_ENCODING;
    case XmlWriterState_ElemStarted:
        writer_close_starttag(This);
        break;
    case XmlWriterState_DocClosed:
        return WR_E_INVALIDACTION;
    default:
        ;
    }

    swprintf(bufW, ARRAY_SIZE(bufW), L"&#x%x;", ch);
    write_output_buffer(This->output, bufW, -1);

    return S_OK;
}

static HRESULT writer_get_next_write_count(const WCHAR *str, unsigned int length, unsigned int *count)
{
    if (!is_char(*str)) return WC_E_XMLCHARACTER;

    if (IS_HIGH_SURROGATE(*str))
    {
        if (length < 2 || !IS_LOW_SURROGATE(*(str + 1)))
            return WR_E_INVALIDSURROGATEPAIR;

        *count = 2;
    }
    else if (IS_LOW_SURROGATE(*str))
        return WR_E_INVALIDSURROGATEPAIR;
    else
        *count = 1;

    return S_OK;
}

static HRESULT write_escaped_char(xmlwriter *writer, const WCHAR *string, unsigned int count)
{
    HRESULT hr;

    switch (*string)
    {
       case '<':
           hr = write_output_buffer(writer->output, L"&lt;", 4);
           break;
       case '&':
           hr = write_output_buffer(writer->output, L"&amp;", 5);
           break;
       case '>':
           hr = write_output_buffer(writer->output, L"&gt;", 4);
           break;
       default:
           hr = write_output_buffer(writer->output, string, count);
    }

    return hr;
}

static HRESULT write_escaped_string(xmlwriter *writer, const WCHAR *string, unsigned int length)
{
    unsigned int count;
    HRESULT hr = S_OK;

    if (length == ~0u)
    {
        while (*string)
        {
            if (FAILED(hr = writer_get_next_write_count(string, ~0u, &count))) return hr;
            if (FAILED(hr = write_escaped_char(writer, string, count))) return hr;

            string += count;
        }
    }
    else
    {
        while (length)
        {
            if (FAILED(hr = writer_get_next_write_count(string, length, &count))) return hr;
            if (FAILED(hr = write_escaped_char(writer, string, count))) return hr;

            string += count;
            length -= count;
        }
    }

    return hr;
}

static HRESULT WINAPI xmlwriter_WriteChars(IXmlWriter *iface, const WCHAR *characters, UINT length)
{
    xmlwriter *writer = impl_from_IXmlWriter(iface);

    TRACE("%p, %s, %d.\n", iface, debugstr_wn(characters, length), length);

    if ((characters == NULL && length != 0))
        return E_INVALIDARG;

    if (length == 0)
        return S_OK;

    switch (writer->state)
    {
    case XmlWriterState_Initial:
        return E_UNEXPECTED;
    case XmlWriterState_InvalidEncoding:
        return MX_E_ENCODING;
    case XmlWriterState_ElemStarted:
        writer_close_starttag(writer);
        break;
    case XmlWriterState_Ready:
    case XmlWriterState_DocClosed:
        writer->state = XmlWriterState_DocClosed;
        return WR_E_INVALIDACTION;
    default:
        ;
    }

    writer->textnode = 1;
    return write_escaped_string(writer, characters, length);
}

static HRESULT WINAPI xmlwriter_WriteComment(IXmlWriter *iface, LPCWSTR comment)
{
    xmlwriter *This = impl_from_IXmlWriter(iface);

    TRACE("%p %s\n", This, debugstr_w(comment));

    switch (This->state)
    {
    case XmlWriterState_Initial:
        return E_UNEXPECTED;
    case XmlWriterState_InvalidEncoding:
        return MX_E_ENCODING;
    case XmlWriterState_ElemStarted:
        writer_close_starttag(This);
        break;
    case XmlWriterState_DocClosed:
        return WR_E_INVALIDACTION;
    default:
        ;
    }

    write_node_indent(This);
    write_output_buffer(This->output, L"<!--", 4);
    if (comment) {
        int len = lstrlenW(comment), i;

        /* Make sure there's no two hyphen sequences in a string, space is used as a separator to produce compliant
           comment string */
        if (len > 1) {
            for (i = 0; i < len; i++) {
                write_output_buffer(This->output, comment + i, 1);
                if (comment[i] == '-' && (i + 1 < len) && comment[i+1] == '-')
                    write_output_buffer_char(This->output, ' ');
            }
        }
        else
            write_output_buffer(This->output, comment, len);

        if (len && comment[len-1] == '-')
            write_output_buffer_char(This->output, ' ');
    }
    write_output_buffer(This->output, L"-->", 3);

    return S_OK;
}

static HRESULT WINAPI xmlwriter_WriteDocType(IXmlWriter *iface, LPCWSTR name, LPCWSTR pubid,
        LPCWSTR sysid, LPCWSTR subset)
{
    xmlwriter *This = impl_from_IXmlWriter(iface);
    unsigned int name_len, pubid_len;
    HRESULT hr;

    TRACE("(%p)->(%s %s %s %s)\n", This, wine_dbgstr_w(name), wine_dbgstr_w(pubid), wine_dbgstr_w(sysid),
            wine_dbgstr_w(subset));

    switch (This->state)
    {
    case XmlWriterState_Initial:
        return E_UNEXPECTED;
    case XmlWriterState_InvalidEncoding:
        return MX_E_ENCODING;
    case XmlWriterState_Content:
    case XmlWriterState_DocClosed:
        return WR_E_INVALIDACTION;
    default:
        ;
    }

    if (is_empty_string(name))
        return E_INVALIDARG;

    if (FAILED(hr = is_valid_name(name, &name_len)))
        return hr;

    if (FAILED(hr = is_valid_pubid(pubid, &pubid_len)))
        return hr;

    write_output_buffer(This->output, L"<!DOCTYPE ", 10);
    write_output_buffer(This->output, name, name_len);

    if (pubid)
    {
        write_output_buffer(This->output, L" PUBLIC ", 8);
        write_output_buffer_quoted(This->output, pubid, pubid_len);
        write_output_buffer_char(This->output, ' ');
        write_output_buffer_quoted(This->output, sysid, -1);
    }
    else if (sysid)
    {
        write_output_buffer(This->output, L" SYSTEM ", 8);
        write_output_buffer_quoted(This->output, sysid, -1);
    }

    if (subset)
    {
        write_output_buffer_char(This->output, ' ');
        write_output_buffer_char(This->output, '[');
        write_output_buffer(This->output, subset, -1);
        write_output_buffer_char(This->output, ']');
    }
    write_output_buffer_char(This->output, '>');

    This->state = XmlWriterState_Content;

    return S_OK;
}

static HRESULT WINAPI xmlwriter_WriteElementString(IXmlWriter *iface, LPCWSTR prefix,
                                     LPCWSTR local_name, LPCWSTR uri, LPCWSTR value)
{
    xmlwriter *This = impl_from_IXmlWriter(iface);
    int prefix_len, local_len;
    struct ns *ns;
    HRESULT hr;

    TRACE("(%p)->(%s %s %s %s)\n", This, wine_dbgstr_w(prefix), wine_dbgstr_w(local_name),
                        wine_dbgstr_w(uri), wine_dbgstr_w(value));

    switch (This->state)
    {
    case XmlWriterState_Initial:
        return E_UNEXPECTED;
    case XmlWriterState_InvalidEncoding:
        return MX_E_ENCODING;
    case XmlWriterState_ElemStarted:
        writer_close_starttag(This);
        break;
    case XmlWriterState_DocClosed:
        return WR_E_INVALIDACTION;
    default:
        ;
    }

    if (!local_name)
        return E_INVALIDARG;

    /* Validate prefix and local name */
    if (FAILED(hr = is_valid_ncname(prefix, &prefix_len)))
        return hr;

    if (FAILED(hr = is_valid_ncname(local_name, &local_len)))
        return hr;

    ns = writer_find_ns(This, prefix, uri);
    if (!ns && !is_empty_string(prefix) && is_empty_string(uri))
        return WR_E_NSPREFIXWITHEMPTYNSURI;

    if (uri && !wcscmp(uri, xmlnsuriW))
    {
        if (!prefix)
            return WR_E_XMLNSPREFIXDECLARATION;

        if (!is_empty_string(prefix))
            return WR_E_XMLNSURIDECLARATION;
    }

    write_encoding_bom(This);
    write_node_indent(This);

    write_output_buffer_char(This->output, '<');
    if (ns)
        write_output_qname(This->output, ns->prefix, ns->prefix_len, local_name, local_len);
    else
        write_output_qname(This->output, prefix, prefix_len, local_name, local_len);

    if (!ns && (prefix_len || !is_empty_string(uri)))
    {
        write_output_qname(This->output, L" xmlns", 6, prefix, prefix_len);
        write_output_buffer_char(This->output, '=');
        write_output_buffer_quoted(This->output, uri, -1);
    }

    if (value)
    {
        write_output_buffer_char(This->output, '>');
        write_output_buffer(This->output, value, -1);
        write_output_buffer(This->output, L"</", 2);
        write_output_qname(This->output, prefix, prefix_len, local_name, local_len);
        write_output_buffer_char(This->output, '>');
    }
    else
        write_output_buffer(This->output, L" />", 3);

    This->state = XmlWriterState_Content;

    return S_OK;
}

static HRESULT WINAPI xmlwriter_WriteEndDocument(IXmlWriter *iface)
{
    xmlwriter *This = impl_from_IXmlWriter(iface);

    TRACE("%p\n", This);

    switch (This->state)
    {
    case XmlWriterState_Initial:
        return E_UNEXPECTED;
    case XmlWriterState_Ready:
    case XmlWriterState_DocClosed:
        This->state = XmlWriterState_DocClosed;
        return WR_E_INVALIDACTION;
    case XmlWriterState_InvalidEncoding:
        return MX_E_ENCODING;
    default:
        ;
    }

    /* empty element stack */
    while (IXmlWriter_WriteEndElement(iface) == S_OK)
        ;

    This->state = XmlWriterState_DocClosed;
    return S_OK;
}

static HRESULT WINAPI xmlwriter_WriteEndElement(IXmlWriter *iface)
{
    xmlwriter *This = impl_from_IXmlWriter(iface);
    struct element *element;

    TRACE("%p\n", This);

    switch (This->state)
    {
    case XmlWriterState_Initial:
        return E_UNEXPECTED;
    case XmlWriterState_Ready:
    case XmlWriterState_DocClosed:
        This->state = XmlWriterState_DocClosed;
        return WR_E_INVALIDACTION;
    case XmlWriterState_InvalidEncoding:
        return MX_E_ENCODING;
    default:
        ;
    }

    element = pop_element(This);
    if (!element)
        return WR_E_INVALIDACTION;

    writer_dec_indent(This);

    if (This->starttagopen)
    {
        writer_output_ns(This, element);
        write_output_buffer(This->output, L" />", 3);
        This->starttagopen = 0;
    }
    else
    {
        /* Write full end tag. */
        write_node_indent(This);
        write_output_buffer(This->output, L"</", 2);
        write_output_buffer(This->output, element->qname, element->len);
        write_output_buffer_char(This->output, '>');
    }
    writer_free_element(This, element);

    return S_OK;
}

static HRESULT WINAPI xmlwriter_WriteEntityRef(IXmlWriter *iface, LPCWSTR pwszName)
{
    xmlwriter *This = impl_from_IXmlWriter(iface);

    FIXME("%p %s\n", This, wine_dbgstr_w(pwszName));

    switch (This->state)
    {
    case XmlWriterState_Initial:
        return E_UNEXPECTED;
    case XmlWriterState_InvalidEncoding:
        return MX_E_ENCODING;
    case XmlWriterState_DocClosed:
        return WR_E_INVALIDACTION;
    default:
        ;
    }

    return E_NOTIMPL;
}

static HRESULT WINAPI xmlwriter_WriteFullEndElement(IXmlWriter *iface)
{
    xmlwriter *This = impl_from_IXmlWriter(iface);
    struct element *element;

    TRACE("%p\n", This);

    switch (This->state)
    {
    case XmlWriterState_Initial:
        return E_UNEXPECTED;
    case XmlWriterState_Ready:
    case XmlWriterState_DocClosed:
        This->state = XmlWriterState_DocClosed;
        return WR_E_INVALIDACTION;
    case XmlWriterState_InvalidEncoding:
        return MX_E_ENCODING;
    case XmlWriterState_ElemStarted:
        writer_close_starttag(This);
        break;
    default:
        ;
    }

    element = pop_element(This);
    if (!element)
        return WR_E_INVALIDACTION;

    writer_dec_indent(This);

    /* don't force full end tag to the next line */
    if (This->state == XmlWriterState_ElemStarted)
    {
        This->state = XmlWriterState_Content;
        This->textnode = 0;
    }
    else
        write_node_indent(This);

    /* write full end tag */
    write_output_buffer(This->output, L"</", 2);
    write_output_buffer(This->output, element->qname, element->len);
    write_output_buffer_char(This->output, '>');

    writer_free_element(This, element);

    return S_OK;
}

static HRESULT WINAPI xmlwriter_WriteName(IXmlWriter *iface, LPCWSTR pwszName)
{
    xmlwriter *This = impl_from_IXmlWriter(iface);

    FIXME("%p %s\n", This, wine_dbgstr_w(pwszName));

    switch (This->state)
    {
    case XmlWriterState_Initial:
        return E_UNEXPECTED;
    case XmlWriterState_Ready:
    case XmlWriterState_DocClosed:
        This->state = XmlWriterState_DocClosed;
        return WR_E_INVALIDACTION;
    case XmlWriterState_InvalidEncoding:
        return MX_E_ENCODING;
    default:
        ;
    }

    return E_NOTIMPL;
}

static HRESULT WINAPI xmlwriter_WriteNmToken(IXmlWriter *iface, LPCWSTR pwszNmToken)
{
    xmlwriter *This = impl_from_IXmlWriter(iface);

    FIXME("%p %s\n", This, wine_dbgstr_w(pwszNmToken));

    switch (This->state)
    {
    case XmlWriterState_Initial:
        return E_UNEXPECTED;
    case XmlWriterState_Ready:
    case XmlWriterState_DocClosed:
        This->state = XmlWriterState_DocClosed;
        return WR_E_INVALIDACTION;
    case XmlWriterState_InvalidEncoding:
        return MX_E_ENCODING;
    default:
        ;
    }

    return E_NOTIMPL;
}

static HRESULT writer_write_node(IXmlWriter *writer, IXmlReader *reader, BOOL shallow, BOOL write_default_attributes)
{
    XmlStandalone standalone = XmlStandalone_Omit;
    const WCHAR *name, *value, *prefix, *uri;
    unsigned int start_depth = 0, depth;
    XmlNodeType node_type;
    HRESULT hr;

    if (FAILED(hr = IXmlReader_GetNodeType(reader, &node_type))) return hr;

    switch (node_type)
    {
        case XmlNodeType_None:
            if (shallow) return S_OK;
            while ((hr = IXmlReader_Read(reader, NULL)) == S_OK)
            {
                if (FAILED(hr = writer_write_node(writer, reader, FALSE, write_default_attributes))) return hr;
            }
            break;
        case XmlNodeType_Element:
            if (FAILED(hr = IXmlReader_GetPrefix(reader, &prefix, NULL))) return hr;
            if (FAILED(hr = IXmlReader_GetLocalName(reader, &name, NULL))) return hr;
            if (FAILED(hr = IXmlReader_GetNamespaceUri(reader, &uri, NULL))) return hr;
            if (FAILED(hr = IXmlWriter_WriteStartElement(writer, prefix, name, uri))) return hr;
            if (FAILED(hr = IXmlWriter_WriteAttributes(writer, reader, write_default_attributes))) return hr;
            if (IXmlReader_IsEmptyElement(reader))
            {
                hr = IXmlWriter_WriteEndElement(writer);
            }
            else
            {
                if (shallow) return S_OK;
                if (FAILED(hr = IXmlReader_MoveToElement(reader))) return hr;
                if (FAILED(hr = IXmlReader_GetDepth(reader, &start_depth))) return hr;
                while ((hr = IXmlReader_Read(reader, &node_type)) == S_OK)
                {
                    if (FAILED(hr = writer_write_node(writer, reader, FALSE, write_default_attributes))) return hr;
                    if (FAILED(hr = IXmlReader_MoveToElement(reader))) return hr;

                    depth = 0;
                    if (FAILED(hr = IXmlReader_GetDepth(reader, &depth))) return hr;
                    if (node_type == XmlNodeType_EndElement && (start_depth == depth - 1)) break;
                }
            }
            break;
        case XmlNodeType_Attribute:
            break;
        case XmlNodeType_Text:
            if (FAILED(hr = IXmlReader_GetValue(reader, &value, NULL))) return hr;
            hr = IXmlWriter_WriteRaw(writer, value);
            break;
        case XmlNodeType_CDATA:
            if (FAILED(hr = IXmlReader_GetValue(reader, &value, NULL))) return hr;
            hr = IXmlWriter_WriteCData(writer, value);
            break;
        case XmlNodeType_ProcessingInstruction:
            if (FAILED(hr = IXmlReader_GetLocalName(reader, &name, NULL))) return hr;
            if (FAILED(hr = IXmlReader_GetValue(reader, &value, NULL))) return hr;
            hr = IXmlWriter_WriteProcessingInstruction(writer, name, value);
            break;
        case XmlNodeType_Comment:
            if (FAILED(hr = IXmlReader_GetValue(reader, &value, NULL))) return hr;
            hr = IXmlWriter_WriteComment(writer, value);
            break;
        case XmlNodeType_Whitespace:
            if (FAILED(hr = IXmlReader_GetValue(reader, &value, NULL))) return hr;
            hr = IXmlWriter_WriteWhitespace(writer, value);
            break;
        case XmlNodeType_EndElement:
            hr = IXmlWriter_WriteFullEndElement(writer);
            break;
        case XmlNodeType_XmlDeclaration:
            while ((hr = IXmlReader_MoveToNextAttribute(reader)) == S_OK)
            {
                if (FAILED(hr = IXmlReader_GetLocalName(reader, &name, NULL))) return hr;
                if (!wcscmp(name, L"standalone"))
                {
                    if (FAILED(hr = IXmlReader_GetValue(reader, &value, NULL))) return hr;
                    standalone = !wcscmp(value, L"yes") ? XmlStandalone_Yes : XmlStandalone_No;
                }
            }
            if (SUCCEEDED(hr))
                hr = IXmlWriter_WriteStartDocument(writer, standalone);
            break;
        default:
            WARN("Unknown node type %d.\n", node_type);
            return E_UNEXPECTED;
    }

    return hr;
}

static HRESULT WINAPI xmlwriter_WriteNode(IXmlWriter *iface, IXmlReader *reader, BOOL write_default_attributes)
{
    HRESULT hr;

    TRACE("%p, %p, %d.\n", iface, reader, write_default_attributes);

    if (!reader)
        return E_INVALIDARG;

    if (SUCCEEDED(hr = writer_write_node(iface, reader, FALSE, write_default_attributes)))
        hr = IXmlReader_Read(reader, NULL);

    return hr;
}

static HRESULT WINAPI xmlwriter_WriteNodeShallow(IXmlWriter *iface, IXmlReader *reader, BOOL write_default_attributes)
{
    TRACE("%p, %p, %d.\n", iface, reader, write_default_attributes);

    if (!reader)
        return E_INVALIDARG;

    return writer_write_node(iface, reader, TRUE, write_default_attributes);
}

static HRESULT WINAPI xmlwriter_WriteProcessingInstruction(IXmlWriter *iface, LPCWSTR name,
                                             LPCWSTR text)
{
    xmlwriter *This = impl_from_IXmlWriter(iface);

    TRACE("(%p)->(%s %s)\n", This, wine_dbgstr_w(name), wine_dbgstr_w(text));

    switch (This->state)
    {
    case XmlWriterState_Initial:
        return E_UNEXPECTED;
    case XmlWriterState_InvalidEncoding:
        return MX_E_ENCODING;
    case XmlWriterState_DocStarted:
        if (!wcscmp(name, L"xml"))
            return WR_E_INVALIDACTION;
        break;
    case XmlWriterState_ElemStarted:
        writer_close_starttag(This);
        break;
    case XmlWriterState_DocClosed:
        return WR_E_INVALIDACTION;
    default:
        ;
    }

    write_encoding_bom(This);
    write_node_indent(This);
    write_output_buffer(This->output, L"<?", 2);
    write_output_buffer(This->output, name, -1);
    write_output_buffer_char(This->output, ' ');
    write_output_buffer(This->output, text, -1);
    write_output_buffer(This->output, L"?>", 2);

    if (!wcscmp(name, L"xml"))
        This->state = XmlWriterState_PIDocStarted;

    return S_OK;
}

static HRESULT WINAPI xmlwriter_WriteQualifiedName(IXmlWriter *iface, LPCWSTR pwszLocalName,
                                     LPCWSTR pwszNamespaceUri)
{
    xmlwriter *This = impl_from_IXmlWriter(iface);

    FIXME("%p %s %s\n", This, wine_dbgstr_w(pwszLocalName), wine_dbgstr_w(pwszNamespaceUri));

    switch (This->state)
    {
    case XmlWriterState_Initial:
        return E_UNEXPECTED;
    case XmlWriterState_InvalidEncoding:
        return MX_E_ENCODING;
    case XmlWriterState_DocClosed:
        return WR_E_INVALIDACTION;
    default:
        ;
    }

    return E_NOTIMPL;
}

static HRESULT WINAPI xmlwriter_WriteRaw(IXmlWriter *iface, LPCWSTR data)
{
    xmlwriter *This = impl_from_IXmlWriter(iface);
    unsigned int count;
    HRESULT hr = S_OK;

    TRACE("%p %s\n", This, debugstr_w(data));

    if (!data)
        return S_OK;

    switch (This->state)
    {
    case XmlWriterState_Initial:
        return E_UNEXPECTED;
    case XmlWriterState_Ready:
        write_xmldecl(This, XmlStandalone_Omit);
        /* fallthrough */
    case XmlWriterState_DocStarted:
    case XmlWriterState_PIDocStarted:
        break;
    case XmlWriterState_InvalidEncoding:
        return MX_E_ENCODING;
    case XmlWriterState_ElemStarted:
        writer_close_starttag(This);
        break;
    default:
        This->state = XmlWriterState_DocClosed;
        return WR_E_INVALIDACTION;
    }

    while (*data)
    {
        if (FAILED(hr = writer_get_next_write_count(data, ~0u, &count))) return hr;
        if (FAILED(hr = write_output_buffer(This->output, data, count))) return hr;

        data += count;
    }

    return hr;
}

static HRESULT WINAPI xmlwriter_WriteRawChars(IXmlWriter *iface,  const WCHAR *characters, UINT length)
{
    xmlwriter *writer = impl_from_IXmlWriter(iface);
    HRESULT hr = S_OK;
    unsigned int count;

    TRACE("%p, %s, %d.\n", iface, debugstr_wn(characters, length), length);

    if ((characters == NULL && length != 0))
        return E_INVALIDARG;

    if (length == 0)
        return S_OK;

    switch (writer->state)
    {
    case XmlWriterState_Initial:
        return E_UNEXPECTED;
    case XmlWriterState_InvalidEncoding:
        return MX_E_ENCODING;
    case XmlWriterState_DocClosed:
        return WR_E_INVALIDACTION;
    case XmlWriterState_Ready:
        write_xmldecl(writer, XmlStandalone_Omit);
        break;
    case XmlWriterState_ElemStarted:
        writer_close_starttag(writer);
    default:
        ;
    }

    while (length)
    {
        if (FAILED(hr = writer_get_next_write_count(characters, length, &count))) return hr;
        if (FAILED(hr = write_output_buffer(writer->output, characters, count))) return hr;

        characters += count;
        length -= count;
    }

    return hr;
}

static HRESULT WINAPI xmlwriter_WriteStartDocument(IXmlWriter *iface, XmlStandalone standalone)
{
    xmlwriter *This = impl_from_IXmlWriter(iface);

    TRACE("(%p)->(%d)\n", This, standalone);

    switch (This->state)
    {
    case XmlWriterState_Initial:
        return E_UNEXPECTED;
    case XmlWriterState_PIDocStarted:
        This->state = XmlWriterState_DocStarted;
        return S_OK;
    case XmlWriterState_Ready:
        break;
    case XmlWriterState_InvalidEncoding:
        return MX_E_ENCODING;
    default:
        This->state = XmlWriterState_DocClosed;
        return WR_E_INVALIDACTION;
    }

    return write_xmldecl(This, standalone);
}

static HRESULT WINAPI xmlwriter_WriteStartElement(IXmlWriter *iface, LPCWSTR prefix, LPCWSTR local_name, LPCWSTR uri)
{
    xmlwriter *This = impl_from_IXmlWriter(iface);
    int prefix_len, local_len;
    struct element *element;
    struct ns *ns;
    HRESULT hr;

    TRACE("(%p)->(%s %s %s)\n", This, wine_dbgstr_w(prefix), wine_dbgstr_w(local_name), wine_dbgstr_w(uri));

    if (!local_name)
        return E_INVALIDARG;

    switch (This->state)
    {
    case XmlWriterState_Initial:
        return E_UNEXPECTED;
    case XmlWriterState_InvalidEncoding:
        return MX_E_ENCODING;
    case XmlWriterState_DocClosed:
        return WR_E_INVALIDACTION;
    case XmlWriterState_ElemStarted:
        writer_close_starttag(This);
        break;
    default:
        ;
    }

    /* Validate prefix and local name */
    if (FAILED(hr = is_valid_ncname(prefix, &prefix_len)))
        return hr;

    if (FAILED(hr = is_valid_ncname(local_name, &local_len)))
        return hr;

    if (uri && !wcscmp(uri, xmlnsuriW))
    {
        if (!prefix)
            return WR_E_XMLNSPREFIXDECLARATION;

        if (!is_empty_string(prefix))
            return WR_E_XMLNSURIDECLARATION;
    }

    ns = writer_find_ns(This, prefix, uri);

    element = alloc_element(This, prefix, local_name);
    if (!element)
        return E_OUTOFMEMORY;

    write_encoding_bom(This);
    write_node_indent(This);

    This->state = XmlWriterState_ElemStarted;
    This->starttagopen = 1;

    writer_push_element(This, element);

    if (!ns && !is_empty_string(uri))
        writer_push_ns(This, prefix, prefix_len, uri);

    write_output_buffer_char(This->output, '<');
    if (ns)
        write_output_qname(This->output, ns->prefix, ns->prefix_len, local_name, local_len);
    else
        write_output_qname(This->output, prefix, prefix_len, local_name, local_len);
    writer_inc_indent(This);

    return S_OK;
}

static HRESULT WINAPI xmlwriter_WriteString(IXmlWriter *iface, const WCHAR *string)
{
    xmlwriter *This = impl_from_IXmlWriter(iface);

    TRACE("%p %s\n", This, debugstr_w(string));

    if (!string)
        return S_OK;

    switch (This->state)
    {
    case XmlWriterState_Initial:
        return E_UNEXPECTED;
    case XmlWriterState_ElemStarted:
        writer_close_starttag(This);
        break;
    case XmlWriterState_Ready:
    case XmlWriterState_DocClosed:
        This->state = XmlWriterState_DocClosed;
        return WR_E_INVALIDACTION;
    case XmlWriterState_InvalidEncoding:
        return MX_E_ENCODING;
    default:
        ;
    }

    This->textnode = 1;
    return write_escaped_string(This, string, ~0u);
}

static HRESULT WINAPI xmlwriter_WriteSurrogateCharEntity(IXmlWriter *iface, WCHAR wchLow, WCHAR wchHigh)
{
    xmlwriter *writer = impl_from_IXmlWriter(iface);
    int codepoint;
    WCHAR bufW[16];

    TRACE("%p, %d, %d.\n", iface, wchLow, wchHigh);

    if (!IS_SURROGATE_PAIR(wchHigh, wchLow))
        return WC_E_XMLCHARACTER;

    switch (writer->state)
    {
    case XmlWriterState_Initial:
        return E_UNEXPECTED;
    case XmlWriterState_InvalidEncoding:
        return MX_E_ENCODING;
    case XmlWriterState_ElemStarted:
        writer_close_starttag(writer);
        break;
    case XmlWriterState_DocClosed:
        return WR_E_INVALIDACTION;
    default:
        ;
    }

    codepoint = ((wchHigh - 0xd800) * 0x400) + (wchLow - 0xdc00) + 0x10000;
    swprintf(bufW, ARRAY_SIZE(bufW), L"&#x%X;", codepoint);
    write_output_buffer(writer->output, bufW, -1);

    return S_OK;
}

static HRESULT WINAPI xmlwriter_WriteWhitespace(IXmlWriter *iface, LPCWSTR text)
{
    xmlwriter *writer = impl_from_IXmlWriter(iface);
    size_t length = 0;

    TRACE("%p, %s.\n", iface, wine_dbgstr_w(text));

    switch (writer->state)
    {
    case XmlWriterState_Initial:
        return E_UNEXPECTED;
    case XmlWriterState_ElemStarted:
        writer_close_starttag(writer);
        break;
    case XmlWriterState_InvalidEncoding:
        return MX_E_ENCODING;
    case XmlWriterState_Ready:
        break;
    default:
        return WR_E_INVALIDACTION;
    }

    while (text[length])
    {
        if (!is_wchar_space(text[length])) return WR_E_NONWHITESPACE;
        length++;
    }

    write_output_buffer(writer->output, text, length);
    return S_OK;
}

static HRESULT WINAPI xmlwriter_Flush(IXmlWriter *iface)
{
    xmlwriter *This = impl_from_IXmlWriter(iface);

    TRACE("%p\n", This);

    return writeroutput_flush_stream(This->output);
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
    xmlwriteroutput *output = impl_from_IXmlWriterOutput(iface);
    ULONG ref = InterlockedIncrement(&output->ref);
    TRACE("%p, refcount %ld.\n", iface, ref);
    return ref;
}

static ULONG WINAPI xmlwriteroutput_Release(IXmlWriterOutput *iface)
{
    xmlwriteroutput *This = impl_from_IXmlWriterOutput(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("%p, refcount %ld.\n", iface, ref);

    if (ref == 0)
    {
        IMalloc *imalloc = This->imalloc;
        if (This->output) IUnknown_Release(This->output);
        if (This->stream) ISequentialStream_Release(This->stream);
        free_output_buffer(This);
        writeroutput_free(This, This->encoding_name);
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

HRESULT WINAPI CreateXmlWriter(REFIID riid, void **obj, IMalloc *imalloc)
{
    xmlwriter *writer;
    HRESULT hr;

    TRACE("(%s, %p, %p)\n", wine_dbgstr_guid(riid), obj, imalloc);

    if (!(writer = m_alloc(imalloc, sizeof(*writer))))
        return E_OUTOFMEMORY;
    memset(writer, 0, sizeof(*writer));

    writer->IXmlWriter_iface.lpVtbl = &xmlwriter_vtbl;
    writer->ref = 1;
    writer->imalloc = imalloc;
    if (imalloc) IMalloc_AddRef(imalloc);
    writer->bom = TRUE;
    writer->conformance = XmlConformanceLevel_Document;
    writer->state = XmlWriterState_Initial;
    list_init(&writer->elements);

    hr = IXmlWriter_QueryInterface(&writer->IXmlWriter_iface, riid, obj);
    IXmlWriter_Release(&writer->IXmlWriter_iface);

    TRACE("returning iface %p, hr %#lx.\n", *obj, hr);

    return hr;
}

static HRESULT create_writer_output(IUnknown *stream, IMalloc *imalloc, xml_encoding encoding,
    const WCHAR *encoding_name, xmlwriteroutput **out)
{
    xmlwriteroutput *writeroutput;
    HRESULT hr;

    *out = NULL;

    if (!(writeroutput = m_alloc(imalloc, sizeof(*writeroutput))))
        return E_OUTOFMEMORY;
    memset(writeroutput, 0, sizeof(*writeroutput));

    writeroutput->IXmlWriterOutput_iface.lpVtbl = &xmlwriteroutputvtbl;
    writeroutput->ref = 1;
    writeroutput->imalloc = imalloc;
    if (imalloc)
        IMalloc_AddRef(imalloc);
    writeroutput->encoding = encoding;
    hr = init_output_buffer(writeroutput);
    if (FAILED(hr)) {
        IUnknown_Release(&writeroutput->IXmlWriterOutput_iface);
        return hr;
    }

    if (encoding_name)
    {
        unsigned int size = (lstrlenW(encoding_name) + 1) * sizeof(WCHAR);
        writeroutput->encoding_name = writeroutput_alloc(writeroutput, size);
        memcpy(writeroutput->encoding_name, encoding_name, size);
    }

    IUnknown_QueryInterface(stream, &IID_IUnknown, (void**)&writeroutput->output);

    *out = writeroutput;

    TRACE("Created writer output %p\n", *out);

    return S_OK;
}

HRESULT WINAPI CreateXmlWriterOutputWithEncodingName(IUnknown *stream, IMalloc *imalloc, const WCHAR *encoding,
        IXmlWriterOutput **out)
{
    xmlwriteroutput *output;
    xml_encoding xml_enc;
    HRESULT hr;

    TRACE("%p %p %s %p\n", stream, imalloc, debugstr_w(encoding), out);

    if (!stream || !out)
        return E_INVALIDARG;

    *out = NULL;

    xml_enc = encoding ? parse_encoding_name(encoding, -1) : XmlEncoding_UTF8;
    if (SUCCEEDED(hr = create_writer_output(stream, imalloc, xml_enc, encoding, &output)))
        *out = &output->IXmlWriterOutput_iface;

    return hr;
}

HRESULT WINAPI CreateXmlWriterOutputWithEncodingCodePage(IUnknown *stream, IMalloc *imalloc, UINT codepage,
        IXmlWriterOutput **out)
{
    xmlwriteroutput *output;
    xml_encoding xml_enc;
    HRESULT hr;

    TRACE("%p %p %u %p\n", stream, imalloc, codepage, out);

    if (!stream || !out)
        return E_INVALIDARG;

    *out = NULL;

    xml_enc = get_encoding_from_codepage(codepage);
    if (SUCCEEDED(hr = create_writer_output(stream, imalloc, xml_enc, NULL, &output)))
        *out = &output->IXmlWriterOutput_iface;

    return hr;
}
