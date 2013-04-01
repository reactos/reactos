/*
 * IXmlReader implementation
 *
 * Copyright 2010, 2012 Nikolay Sivov
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

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#define COBJMACROS

#include <stdarg.h>
#include <windef.h>
#include <winbase.h>
#include <initguid.h>
#include <objbase.h>
#include <xmllite.h>
#include "xmllite_private.h"

#include <wine/debug.h>
#include <wine/list.h>
#include <wine/unicode.h>

WINE_DEFAULT_DEBUG_CHANNEL(xmllite);

/* not defined in public headers */
DEFINE_GUID(IID_IXmlReaderInput, 0x0b3ccc9b, 0x9214, 0x428b, 0xa2, 0xae, 0xef, 0x3a, 0xa8, 0x71, 0xaf, 0xda);

typedef enum
{
    XmlEncoding_UTF16,
    XmlEncoding_UTF8,
    XmlEncoding_Unknown
} xml_encoding;

static const WCHAR utf16W[] = {'U','T','F','-','1','6',0};
static const WCHAR utf8W[] = {'U','T','F','-','8',0};

static const WCHAR dblquoteW[] = {'\"',0};
static const WCHAR quoteW[] = {'\'',0};

struct xml_encoding_data
{
    const WCHAR *name;
    xml_encoding enc;
    UINT cp;
};

static const struct xml_encoding_data xml_encoding_map[] = {
    { utf16W, XmlEncoding_UTF16, ~0 },
    { utf8W,  XmlEncoding_UTF8,  CP_UTF8 }
};

typedef struct
{
    char *data;
    char *cur;
    unsigned int allocated;
    unsigned int written;
} encoded_buffer;

typedef struct input_buffer input_buffer;

typedef struct _xmlreaderinput
{
    IXmlReaderInput IXmlReaderInput_iface;
    LONG ref;
    /* reference passed on IXmlReaderInput creation, is kept when input is created */
    IUnknown *input;
    IMalloc *imalloc;
    xml_encoding encoding;
    BOOL hint;
    WCHAR *baseuri;
    /* stream reference set after SetInput() call from reader,
       stored as sequential stream, cause currently
       optimizations possible with IStream aren't implemented */
    ISequentialStream *stream;
    input_buffer *buffer;
} xmlreaderinput;

typedef struct
{
    const WCHAR *str;
    UINT len;
} strval;

struct attribute
{
    struct list entry;
    strval localname;
    strval value;
};

typedef struct _xmlreader
{
    IXmlReader IXmlReader_iface;
    LONG ref;
    xmlreaderinput *input;
    IMalloc *imalloc;
    XmlReadState state;
    XmlNodeType nodetype;
    DtdProcessing dtdmode;
    UINT line, pos;           /* reader position in XML stream */
    struct list attrs; /* attributes list for current node */
    struct attribute *attr; /* current attribute */
    UINT attr_count;
} xmlreader;

struct input_buffer
{
    encoded_buffer utf16;
    encoded_buffer encoded;
    UINT code_page;
    xmlreaderinput *input;
};

static inline xmlreader *impl_from_IXmlReader(IXmlReader *iface)
{
    return CONTAINING_RECORD(iface, xmlreader, IXmlReader_iface);
}

static inline xmlreaderinput *impl_from_IXmlReaderInput(IXmlReaderInput *iface)
{
    return CONTAINING_RECORD(iface, xmlreaderinput, IXmlReaderInput_iface);
}

static inline void *m_alloc(IMalloc *imalloc, size_t len)
{
    if (imalloc)
        return IMalloc_Alloc(imalloc, len);
    else
        return heap_alloc(len);
}

static inline void *m_realloc(IMalloc *imalloc, void *mem, size_t len)
{
    if (imalloc)
        return IMalloc_Realloc(imalloc, mem, len);
    else
        return heap_realloc(mem, len);
}

static inline void m_free(IMalloc *imalloc, void *mem)
{
    if (imalloc)
        IMalloc_Free(imalloc, mem);
    else
        heap_free(mem);
}

/* reader memory allocation functions */
static inline void *reader_alloc(xmlreader *reader, size_t len)
{
    return m_alloc(reader->imalloc, len);
}

static inline void reader_free(xmlreader *reader, void *mem)
{
    m_free(reader->imalloc, mem);
}

/* reader input memory allocation functions */
static inline void *readerinput_alloc(xmlreaderinput *input, size_t len)
{
    return m_alloc(input->imalloc, len);
}

static inline void *readerinput_realloc(xmlreaderinput *input, void *mem, size_t len)
{
    return m_realloc(input->imalloc, mem, len);
}

static inline void readerinput_free(xmlreaderinput *input, void *mem)
{
    m_free(input->imalloc, mem);
}

static inline WCHAR *readerinput_strdupW(xmlreaderinput *input, const WCHAR *str)
{
    LPWSTR ret = NULL;

    if(str) {
        DWORD size;

        size = (strlenW(str)+1)*sizeof(WCHAR);
        ret = readerinput_alloc(input, size);
        if (ret) memcpy(ret, str, size);
    }

    return ret;
}

static void reader_clear_attrs(xmlreader *reader)
{
    struct attribute *attr, *attr2;
    LIST_FOR_EACH_ENTRY_SAFE(attr, attr2, &reader->attrs, struct attribute, entry)
    {
        reader_free(reader, attr);
    }
    list_init(&reader->attrs);
    reader->attr_count = 0;
}

/* attribute data holds pointers to buffer data, so buffer shrink is not possible
   while we are on a node with attributes */
static HRESULT reader_add_attr(xmlreader *reader, strval *localname, strval *value)
{
    struct attribute *attr;

    attr = reader_alloc(reader, sizeof(*attr));
    if (!attr) return E_OUTOFMEMORY;

    attr->localname = *localname;
    attr->value = *value;
    list_add_tail(&reader->attrs, &attr->entry);
    reader->attr_count++;

    return S_OK;
}

static HRESULT init_encoded_buffer(xmlreaderinput *input, encoded_buffer *buffer)
{
    const int initial_len = 0x2000;
    buffer->data = readerinput_alloc(input, initial_len);
    if (!buffer->data) return E_OUTOFMEMORY;

    memset(buffer->data, 0, 4);
    buffer->cur = buffer->data;
    buffer->allocated = initial_len;
    buffer->written = 0;

    return S_OK;
}

static void free_encoded_buffer(xmlreaderinput *input, encoded_buffer *buffer)
{
    readerinput_free(input, buffer->data);
}

static HRESULT get_code_page(xml_encoding encoding, UINT *cp)
{
    if (encoding == XmlEncoding_Unknown)
    {
        FIXME("unsupported encoding %d\n", encoding);
        return E_NOTIMPL;
    }

    *cp = xml_encoding_map[encoding].cp;

    return S_OK;
}

static xml_encoding parse_encoding_name(const WCHAR *name, int len)
{
    int min, max, n, c;

    if (!name) return XmlEncoding_Unknown;

    min = 0;
    max = sizeof(xml_encoding_map)/sizeof(struct xml_encoding_data) - 1;

    while (min <= max)
    {
        n = (min+max)/2;

        if (len != -1)
            c = strncmpiW(xml_encoding_map[n].name, name, len);
        else
            c = strcmpiW(xml_encoding_map[n].name, name);
        if (!c)
            return xml_encoding_map[n].enc;

        if (c > 0)
            max = n-1;
        else
            min = n+1;
    }

    return XmlEncoding_Unknown;
}

static HRESULT alloc_input_buffer(xmlreaderinput *input)
{
    input_buffer *buffer;
    HRESULT hr;

    input->buffer = NULL;

    buffer = readerinput_alloc(input, sizeof(*buffer));
    if (!buffer) return E_OUTOFMEMORY;

    buffer->input = input;
    buffer->code_page = ~0; /* code page is unknown at this point */
    hr = init_encoded_buffer(input, &buffer->utf16);
    if (hr != S_OK) {
        readerinput_free(input, buffer);
        return hr;
    }

    hr = init_encoded_buffer(input, &buffer->encoded);
    if (hr != S_OK) {
        free_encoded_buffer(input, &buffer->utf16);
        readerinput_free(input, buffer);
        return hr;
    }

    input->buffer = buffer;
    return S_OK;
}

static void free_input_buffer(input_buffer *buffer)
{
    free_encoded_buffer(buffer->input, &buffer->encoded);
    free_encoded_buffer(buffer->input, &buffer->utf16);
    readerinput_free(buffer->input, buffer);
}

static void readerinput_release_stream(xmlreaderinput *readerinput)
{
    if (readerinput->stream) {
        ISequentialStream_Release(readerinput->stream);
        readerinput->stream = NULL;
    }
}

/* Queries already stored interface for IStream/ISequentialStream.
   Interface supplied on creation will be overwritten */
static HRESULT readerinput_query_for_stream(xmlreaderinput *readerinput)
{
    HRESULT hr;

    readerinput_release_stream(readerinput);
    hr = IUnknown_QueryInterface(readerinput->input, &IID_IStream, (void**)&readerinput->stream);
    if (hr != S_OK)
        hr = IUnknown_QueryInterface(readerinput->input, &IID_ISequentialStream, (void**)&readerinput->stream);

    return hr;
}

/* reads a chunk to raw buffer */
static HRESULT readerinput_growraw(xmlreaderinput *readerinput)
{
    encoded_buffer *buffer = &readerinput->buffer->encoded;
    ULONG len = buffer->allocated - buffer->written, read;
    HRESULT hr;

    /* always try to get aligned to 4 bytes, so the only case we can get partialy read characters is
       variable width encodings like UTF-8 */
    len = (len + 3) & ~3;
    /* try to use allocated space or grow */
    if (buffer->allocated - buffer->written < len)
    {
        buffer->allocated *= 2;
        buffer->data = readerinput_realloc(readerinput, buffer->data, buffer->allocated);
        len = buffer->allocated - buffer->written;
    }

    hr = ISequentialStream_Read(readerinput->stream, buffer->data + buffer->written, len, &read);
    if (FAILED(hr)) return hr;
    TRACE("requested %d, read %d, ret 0x%08x\n", len, read, hr);
    buffer->written += read;

    return hr;
}

/* grows UTF-16 buffer so it has at least 'length' bytes free on return */
static void readerinput_grow(xmlreaderinput *readerinput, int length)
{
    encoded_buffer *buffer = &readerinput->buffer->utf16;

    /* grow if needed, plus 4 bytes to be sure null terminator will fit in */
    if (buffer->allocated < buffer->written + length + 4)
    {
        int grown_size = max(2*buffer->allocated, buffer->allocated + length);
        buffer->data = readerinput_realloc(readerinput, buffer->data, grown_size);
        buffer->allocated = grown_size;
    }
}

static HRESULT readerinput_detectencoding(xmlreaderinput *readerinput, xml_encoding *enc)
{
    encoded_buffer *buffer = &readerinput->buffer->encoded;
    static char startA[] = {'<','?','x','m'};
    static WCHAR startW[] = {'<','?'};
    static char utf8bom[] = {0xef,0xbb,0xbf};
    static char utf16lebom[] = {0xff,0xfe};

    *enc = XmlEncoding_Unknown;

    if (buffer->written <= 3) return MX_E_INPUTEND;

    /* try start symbols if we have enough data to do that, input buffer should contain
       first chunk already */
    if (!memcmp(buffer->data, startA, sizeof(startA)))
        *enc = XmlEncoding_UTF8;
    else if (!memcmp(buffer->data, startW, sizeof(startW)))
        *enc = XmlEncoding_UTF16;
    /* try with BOM now */
    else if (!memcmp(buffer->data, utf8bom, sizeof(utf8bom)))
    {
        buffer->cur += sizeof(utf8bom);
        *enc = XmlEncoding_UTF8;
    }
    else if (!memcmp(buffer->data, utf16lebom, sizeof(utf16lebom)))
    {
        buffer->cur += sizeof(utf16lebom);
        *enc = XmlEncoding_UTF16;
    }

    return S_OK;
}

static int readerinput_get_utf8_convlen(xmlreaderinput *readerinput)
{
    encoded_buffer *buffer = &readerinput->buffer->encoded;
    int len = buffer->written;

    /* complete single byte char */
    if (!(buffer->data[len-1] & 0x80)) return len;

    /* find start byte of multibyte char */
    while (--len && !(buffer->data[len] & 0xc0))
        ;

    return len;
}

/* returns byte length of complete char sequence for specified code page, */
static int readerinput_get_convlen(xmlreaderinput *readerinput, UINT cp)
{
    encoded_buffer *buffer = &readerinput->buffer->encoded;
    int len = buffer->written;

    if (cp == CP_UTF8)
        len = readerinput_get_utf8_convlen(readerinput);
    else
        len = buffer->written;

    return len - (buffer->cur - buffer->data);
}

/* note that raw buffer content is kept */
static void readerinput_switchencoding(xmlreaderinput *readerinput, xml_encoding enc)
{
    encoded_buffer *src = &readerinput->buffer->encoded;
    encoded_buffer *dest = &readerinput->buffer->utf16;
    int len, dest_len;
    HRESULT hr;
    UINT cp;

    hr = get_code_page(enc, &cp);
    if (FAILED(hr)) return;

    len = readerinput_get_convlen(readerinput, cp);

    TRACE("switching to cp %d\n", cp);

    /* just copy in this case */
    if (enc == XmlEncoding_UTF16)
    {
        readerinput_grow(readerinput, len);
        memcpy(dest->data, src->cur, len);
        readerinput->buffer->code_page = cp;
        return;
    }

    dest_len = MultiByteToWideChar(cp, 0, src->cur, len, NULL, 0);
    readerinput_grow(readerinput, dest_len);
    MultiByteToWideChar(cp, 0, src->cur, len, (WCHAR*)dest->data, dest_len);
    dest->data[dest_len] = 0;
    readerinput->buffer->code_page = cp;
}

static inline const WCHAR *reader_get_cur(xmlreader *reader)
{
    return (WCHAR*)reader->input->buffer->utf16.cur;
}

static int reader_cmp(xmlreader *reader, const WCHAR *str)
{
    const WCHAR *ptr = reader_get_cur(reader);
    int i = 0;

    return strncmpW(str, ptr, strlenW(str));

    while (str[i]) {
        if (ptr[i] != str[i]) return 0;
        i++;
    }

    return 1;
}

/* moves cursor n WCHARs forward */
static void reader_skipn(xmlreader *reader, int n)
{
    encoded_buffer *buffer = &reader->input->buffer->utf16;
    const WCHAR *ptr = reader_get_cur(reader);

    while (*ptr++ && n--)
    {
        buffer->cur += sizeof(WCHAR);
        reader->pos++;
    }
}

/* [3] S ::= (#x20 | #x9 | #xD | #xA)+ */
static int reader_skipspaces(xmlreader *reader)
{
    encoded_buffer *buffer = &reader->input->buffer->utf16;
    const WCHAR *ptr = reader_get_cur(reader), *start = ptr;

    while (*ptr == ' ' || *ptr == '\t' || *ptr == '\r' || *ptr == '\n')
    {
        buffer->cur += sizeof(WCHAR);
        if (*ptr == '\r')
            reader->pos = 0;
        else if (*ptr == '\n')
        {
            reader->line++;
            reader->pos = 0;
        }
        else
            reader->pos++;
        ptr++;
    }

    return ptr - start;
}

/* [26] VersionNum ::= '1.' [0-9]+ */
static HRESULT reader_parse_versionnum(xmlreader *reader, strval *val)
{
    const WCHAR *ptr, *ptr2, *start = reader_get_cur(reader);
    static const WCHAR onedotW[] = {'1','.',0};

    if (reader_cmp(reader, onedotW)) return WC_E_XMLDECL;
    /* skip "1." */
    reader_skipn(reader, 2);

    ptr2 = ptr = reader_get_cur(reader);
    while (*ptr >= '0' && *ptr <= '9')
        ptr++;

    if (ptr2 == ptr) return WC_E_DIGIT;
    TRACE("version=%s\n", debugstr_wn(start, ptr-start));
    val->str = start;
    val->len = ptr-start;
    reader_skipn(reader, ptr-ptr2);
    return S_OK;
}

/* [25] Eq ::= S? '=' S? */
static HRESULT reader_parse_eq(xmlreader *reader)
{
    static const WCHAR eqW[] = {'=',0};
    reader_skipspaces(reader);
    if (reader_cmp(reader, eqW)) return WC_E_EQUAL;
    /* skip '=' */
    reader_skipn(reader, 1);
    reader_skipspaces(reader);
    return S_OK;
}

/* [24] VersionInfo ::= S 'version' Eq ("'" VersionNum "'" | '"' VersionNum '"') */
static HRESULT reader_parse_versioninfo(xmlreader *reader)
{
    static const WCHAR versionW[] = {'v','e','r','s','i','o','n',0};
    strval val, name;
    HRESULT hr;

    if (!reader_skipspaces(reader)) return WC_E_WHITESPACE;

    if (reader_cmp(reader, versionW)) return WC_E_XMLDECL;
    name.str = reader_get_cur(reader);
    name.len = 7;
    /* skip 'version' */
    reader_skipn(reader, 7);

    hr = reader_parse_eq(reader);
    if (FAILED(hr)) return hr;

    if (reader_cmp(reader, quoteW) && reader_cmp(reader, dblquoteW))
        return WC_E_QUOTE;
    /* skip "'"|'"' */
    reader_skipn(reader, 1);

    hr = reader_parse_versionnum(reader, &val);
    if (FAILED(hr)) return hr;

    if (reader_cmp(reader, quoteW) && reader_cmp(reader, dblquoteW))
        return WC_E_QUOTE;

    /* skip "'"|'"' */
    reader_skipn(reader, 1);

    return reader_add_attr(reader, &name, &val);
}

/* ([A-Za-z0-9._] | '-') */
static inline int is_wchar_encname(WCHAR ch)
{
    return ((ch >= 'A' && ch <= 'Z') ||
            (ch >= 'a' && ch <= 'z') ||
            (ch >= '0' && ch <= '9') ||
            (ch == '.') || (ch == '_') ||
            (ch == '-'));
}

/* [81] EncName ::= [A-Za-z] ([A-Za-z0-9._] | '-')* */
static HRESULT reader_parse_encname(xmlreader *reader, strval *val)
{
    const WCHAR *start = reader_get_cur(reader), *ptr;
    xml_encoding enc;
    int len;

    if ((*start < 'A' || *start > 'Z') && (*start < 'a' || *start > 'z'))
        return WC_E_ENCNAME;

    ptr = start;
    while (is_wchar_encname(*++ptr))
        ;

    len = ptr - start;
    enc = parse_encoding_name(start, len);
    TRACE("encoding name %s\n", debugstr_wn(start, len));
    val->str = start;
    val->len = len;

    if (enc == XmlEncoding_Unknown)
        return WC_E_ENCNAME;

    /* skip encoding name */
    reader_skipn(reader, len);
    return S_OK;
}

/* [80] EncodingDecl ::= S 'encoding' Eq ('"' EncName '"' | "'" EncName "'" ) */
static HRESULT reader_parse_encdecl(xmlreader *reader)
{
    static const WCHAR encodingW[] = {'e','n','c','o','d','i','n','g',0};
    strval name, val;
    HRESULT hr;

    if (!reader_skipspaces(reader)) return WC_E_WHITESPACE;

    if (reader_cmp(reader, encodingW)) return S_FALSE;
    name.str = reader_get_cur(reader);
    name.len = 8;
    /* skip 'encoding' */
    reader_skipn(reader, 8);

    hr = reader_parse_eq(reader);
    if (FAILED(hr)) return hr;

    if (reader_cmp(reader, quoteW) && reader_cmp(reader, dblquoteW))
        return WC_E_QUOTE;
    /* skip "'"|'"' */
    reader_skipn(reader, 1);

    hr = reader_parse_encname(reader, &val);
    if (FAILED(hr)) return hr;

    if (reader_cmp(reader, quoteW) && reader_cmp(reader, dblquoteW))
        return WC_E_QUOTE;

    /* skip "'"|'"' */
    reader_skipn(reader, 1);

    return reader_add_attr(reader, &name, &val);
}

/* [32] SDDecl ::= S 'standalone' Eq (("'" ('yes' | 'no') "'") | ('"' ('yes' | 'no') '"')) */
static HRESULT reader_parse_sddecl(xmlreader *reader)
{
    static const WCHAR standaloneW[] = {'s','t','a','n','d','a','l','o','n','e',0};
    static const WCHAR yesW[] = {'y','e','s',0};
    static const WCHAR noW[] = {'n','o',0};
    const WCHAR *start, *ptr;
    strval name, val;
    HRESULT hr;

    if (!reader_skipspaces(reader)) return WC_E_WHITESPACE;

    if (reader_cmp(reader, standaloneW)) return S_FALSE;
    name.str = reader_get_cur(reader);
    name.len = 10;
    /* skip 'standalone' */
    reader_skipn(reader, 10);

    hr = reader_parse_eq(reader);
    if (FAILED(hr)) return hr;

    if (reader_cmp(reader, quoteW) && reader_cmp(reader, dblquoteW))
        return WC_E_QUOTE;
    /* skip "'"|'"' */
    reader_skipn(reader, 1);

    if (reader_cmp(reader, yesW) && reader_cmp(reader, noW))
        return WC_E_XMLDECL;

    start = reader_get_cur(reader);
    /* skip 'yes'|'no' */
    reader_skipn(reader, reader_cmp(reader, yesW) ? 2 : 3);
    ptr = reader_get_cur(reader);
    TRACE("standalone=%s\n", debugstr_wn(start, ptr-start));
    val.str = start;
    val.len = ptr-start;

    if (reader_cmp(reader, quoteW) && reader_cmp(reader, dblquoteW))
        return WC_E_QUOTE;
    /* skip "'"|'"' */
    reader_skipn(reader, 1);

    return reader_add_attr(reader, &name, &val);
}

/* [23] XMLDecl ::= '<?xml' VersionInfo EncodingDecl? SDDecl? S? '?>' */
static HRESULT reader_parse_xmldecl(xmlreader *reader)
{
    static const WCHAR xmldeclW[] = {'<','?','x','m','l',0};
    static const WCHAR declcloseW[] = {'?','>',0};
    HRESULT hr;

    /* check if we have "<?xml" */
    if (reader_cmp(reader, xmldeclW)) return S_FALSE;

    reader_skipn(reader, 5);
    hr = reader_parse_versioninfo(reader);
    if (FAILED(hr))
        return hr;

    hr = reader_parse_encdecl(reader);
    if (FAILED(hr))
        return hr;

    hr = reader_parse_sddecl(reader);
    if (FAILED(hr))
        return hr;

    reader_skipspaces(reader);
    if (reader_cmp(reader, declcloseW)) return WC_E_XMLDECL;
    reader_skipn(reader, 2);

    return S_OK;
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
    ULONG ref = InterlockedIncrement(&This->ref);
    TRACE("(%p)->(%d)\n", This, ref);
    return ref;
}

static ULONG WINAPI xmlreader_Release(IXmlReader *iface)
{
    xmlreader *This = impl_from_IXmlReader(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(%d)\n", This, ref);

    if (ref == 0)
    {
        IMalloc *imalloc = This->imalloc;
        if (This->input) IUnknown_Release(&This->input->IXmlReaderInput_iface);
        reader_clear_attrs(This);
        reader_free(This, This);
        if (imalloc) IMalloc_Release(imalloc);
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
        readerinput_release_stream(This->input);
        IUnknown_Release(&This->input->IXmlReaderInput_iface);
        This->input = NULL;
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
        IXmlReaderInput *readerinput;

        /* create IXmlReaderInput basing on supplied interface */
        hr = CreateXmlReaderInputWithEncodingName(input,
                                         NULL, NULL, FALSE, NULL, &readerinput);
        if (hr != S_OK) return hr;
        This->input = impl_from_IXmlReaderInput(readerinput);
    }

    /* set stream for supplied IXmlReaderInput */
    hr = readerinput_query_for_stream(This->input);
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
        case XmlReaderProperty_DtdProcessing:
            *value = This->dtdmode;
            break;
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
    xmlreader *This = impl_from_IXmlReader(iface);

    TRACE("(%p %u %lu)\n", iface, property, value);

    switch (property)
    {
        case XmlReaderProperty_DtdProcessing:
            if (value < 0 || value > _DtdProcessing_Last) return E_INVALIDARG;
            This->dtdmode = value;
            break;
        default:
            FIXME("Unimplemented property (%u)\n", property);
            return E_NOTIMPL;
    }

    return S_OK;
}

static HRESULT WINAPI xmlreader_Read(IXmlReader* iface, XmlNodeType *node_type)
{
    xmlreader *This = impl_from_IXmlReader(iface);

    FIXME("(%p)->(%p): stub\n", This, node_type);

    if (This->state == XmlReadState_Closed) return S_FALSE;

    /* if it's a first call for a new input we need to detect stream encoding */
    if (This->state == XmlReadState_Initial)
    {
        xml_encoding enc;
        HRESULT hr;

        hr = readerinput_growraw(This->input);
        if (FAILED(hr)) return hr;

        /* try to detect encoding by BOM or data and set input code page */
        hr = readerinput_detectencoding(This->input, &enc);
        TRACE("detected encoding %s, 0x%08x\n", debugstr_w(xml_encoding_map[enc].name), hr);
        if (FAILED(hr)) return hr;

        /* always switch first time cause we have to put something in */
        readerinput_switchencoding(This->input, enc);

        /* parse xml declaration */
        hr = reader_parse_xmldecl(This);
        if (FAILED(hr)) return hr;

        if (hr == S_OK)
        {
            This->state = XmlReadState_Interactive;
            This->nodetype = *node_type = XmlNodeType_XmlDeclaration;
            return S_OK;
        }
    }

    return E_NOTIMPL;
}

static HRESULT WINAPI xmlreader_GetNodeType(IXmlReader* iface, XmlNodeType *node_type)
{
    xmlreader *This = impl_from_IXmlReader(iface);
    TRACE("(%p)->(%p)\n", This, node_type);

    /* When we're on attribute always return attribute type, container node type is kept.
       Note that container is not necessarily an element, and attribute doesn't mean it's
       an attribute in XML spec terms. */
    *node_type = This->attr ? XmlNodeType_Attribute : This->nodetype;
    return This->state == XmlReadState_Closed ? S_FALSE : S_OK;
}

static HRESULT WINAPI xmlreader_MoveToFirstAttribute(IXmlReader* iface)
{
    xmlreader *This = impl_from_IXmlReader(iface);

    TRACE("(%p)\n", This);

    if (!This->attr_count) return S_FALSE;
    This->attr = LIST_ENTRY(list_head(&This->attrs), struct attribute, entry);
    return S_OK;
}

static HRESULT WINAPI xmlreader_MoveToNextAttribute(IXmlReader* iface)
{
    xmlreader *This = impl_from_IXmlReader(iface);
    const struct list *next;

    TRACE("(%p)\n", This);

    if (!This->attr_count) return S_FALSE;

    if (!This->attr)
        return IXmlReader_MoveToFirstAttribute(iface);

    next = list_next(&This->attrs, &This->attr->entry);
    if (next)
        This->attr = LIST_ENTRY(next, struct attribute, entry);

    return next ? S_OK : S_FALSE;
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
    xmlreader *This = impl_from_IXmlReader(iface);

    TRACE("(%p)\n", This);

    if (!This->attr_count) return S_FALSE;
    This->attr = NULL;
    return S_OK;
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

static HRESULT WINAPI xmlreader_GetAttributeCount(IXmlReader* iface, UINT *count)
{
    xmlreader *This = impl_from_IXmlReader(iface);

    TRACE("(%p)->(%p)\n", This, count);

    if (!count) return E_INVALIDARG;

    *count = This->attr_count;
    return S_OK;
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
        WARN("interface %s not implemented\n", debugstr_guid(riid));
        return E_NOINTERFACE;
    }

    IUnknown_AddRef(iface);

    return S_OK;
}

static ULONG WINAPI xmlreaderinput_AddRef(IXmlReaderInput *iface)
{
    xmlreaderinput *This = impl_from_IXmlReaderInput(iface);
    ULONG ref = InterlockedIncrement(&This->ref);
    TRACE("(%p)->(%d)\n", This, ref);
    return ref;
}

static ULONG WINAPI xmlreaderinput_Release(IXmlReaderInput *iface)
{
    xmlreaderinput *This = impl_from_IXmlReaderInput(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(%d)\n", This, ref);

    if (ref == 0)
    {
        IMalloc *imalloc = This->imalloc;
        if (This->input) IUnknown_Release(This->input);
        if (This->stream) ISequentialStream_Release(This->stream);
        if (This->buffer) free_input_buffer(This->buffer);
        readerinput_free(This, This->baseuri);
        readerinput_free(This, This);
        if (imalloc) IMalloc_Release(imalloc);
    }

    return ref;
}

static const struct IUnknownVtbl xmlreaderinput_vtbl =
{
    xmlreaderinput_QueryInterface,
    xmlreaderinput_AddRef,
    xmlreaderinput_Release
};

HRESULT WINAPI CreateXmlReader(REFIID riid, void **obj, IMalloc *imalloc)
{
    xmlreader *reader;

    TRACE("(%s, %p, %p)\n", wine_dbgstr_guid(riid), obj, imalloc);

    if (!IsEqualGUID(riid, &IID_IXmlReader))
    {
        ERR("Unexpected IID requested -> (%s)\n", wine_dbgstr_guid(riid));
        return E_FAIL;
    }

    if (imalloc)
        reader = IMalloc_Alloc(imalloc, sizeof(*reader));
    else
        reader = heap_alloc(sizeof(*reader));
    if(!reader) return E_OUTOFMEMORY;

    reader->IXmlReader_iface.lpVtbl = &xmlreader_vtbl;
    reader->ref = 1;
    reader->input = NULL;
    reader->state = XmlReadState_Closed;
    reader->dtdmode = DtdProcessing_Prohibit;
    reader->line  = reader->pos = 0;
    reader->imalloc = imalloc;
    if (imalloc) IMalloc_AddRef(imalloc);
    reader->nodetype = XmlNodeType_None;
    list_init(&reader->attrs);
    reader->attr_count = 0;
    reader->attr = NULL;

    *obj = &reader->IXmlReader_iface;

    TRACE("returning iface %p\n", *obj);

    return S_OK;
}

HRESULT WINAPI CreateXmlReaderInputWithEncodingName(IUnknown *stream,
                                                    IMalloc *imalloc,
                                                    LPCWSTR encoding,
                                                    BOOL hint,
                                                    LPCWSTR base_uri,
                                                    IXmlReaderInput **ppInput)
{
    xmlreaderinput *readerinput;
    HRESULT hr;

    TRACE("%p %p %s %d %s %p\n", stream, imalloc, wine_dbgstr_w(encoding),
                                       hint, wine_dbgstr_w(base_uri), ppInput);

    if (!stream || !ppInput) return E_INVALIDARG;

    if (imalloc)
        readerinput = IMalloc_Alloc(imalloc, sizeof(*readerinput));
    else
        readerinput = heap_alloc(sizeof(*readerinput));
    if(!readerinput) return E_OUTOFMEMORY;

    readerinput->IXmlReaderInput_iface.lpVtbl = &xmlreaderinput_vtbl;
    readerinput->ref = 1;
    readerinput->imalloc = imalloc;
    readerinput->stream = NULL;
    if (imalloc) IMalloc_AddRef(imalloc);
    readerinput->encoding = parse_encoding_name(encoding, -1);
    readerinput->hint = hint;
    readerinput->baseuri = readerinput_strdupW(readerinput, base_uri);

    hr = alloc_input_buffer(readerinput);
    if (hr != S_OK)
    {
        readerinput_free(readerinput, readerinput);
        return hr;
    }
    IUnknown_QueryInterface(stream, &IID_IUnknown, (void**)&readerinput->input);

    *ppInput = &readerinput->IXmlReaderInput_iface;

    TRACE("returning iface %p\n", *ppInput);

    return S_OK;
}
