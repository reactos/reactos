/*
 * IXmlReader implementation
 *
 * Copyright 2010, 2012-2013 Nikolay Sivov
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

typedef enum
{
    XmlReadInState_Initial,
    XmlReadInState_XmlDecl,
    XmlReadInState_Misc_DTD,
    XmlReadInState_DTD,
    XmlReadInState_DTD_Misc,
    XmlReadInState_Element,
    XmlReadInState_Content,
    XmlReadInState_MiscEnd
} XmlReaderInternalState;

/* This state denotes where parsing was interrupted by input problem.
   Reader resumes parsing using this information. */
typedef enum
{
    XmlReadResumeState_Initial,
    XmlReadResumeState_PITarget,
    XmlReadResumeState_PIBody,
    XmlReadResumeState_CDATA,
    XmlReadResumeState_Comment,
    XmlReadResumeState_STag
} XmlReaderResumeState;

/* saved pointer index to resume from particular input position */
typedef enum
{
    XmlReadResume_Name,  /* PITarget, name for NCName, prefix for QName */
    XmlReadResume_Local, /* local for QName */
    XmlReadResume_Body,  /* PI body, comment text, CDATA text */
    XmlReadResume_Last
} XmlReaderResume;

typedef enum
{
    StringValue_LocalName,
    StringValue_QualifiedName,
    StringValue_Value,
    StringValue_Last
} XmlReaderStringValue;

static const WCHAR utf16W[] = {'U','T','F','-','1','6',0};
static const WCHAR utf8W[] = {'U','T','F','-','8',0};

static const WCHAR dblquoteW[] = {'\"',0};
static const WCHAR quoteW[] = {'\'',0};
static const WCHAR ltW[] = {'<',0};
static const WCHAR gtW[] = {'>',0};
static const WCHAR commentW[] = {'<','!','-','-',0};
static const WCHAR piW[] = {'<','?',0};

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

typedef struct
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
    unsigned int pending : 1;
} xmlreaderinput;

static const struct IUnknownVtbl xmlreaderinputvtbl;

/* Structure to hold parsed string of specific length.

   Reader stores node value as 'start' pointer, on request
   a null-terminated version of it is allocated.

   To init a strval variable use reader_init_strval(),
   to set strval as a reader value use reader_set_strval().
 */
typedef struct
{
    WCHAR *start; /* input position where value starts */
    UINT len;     /* length in WCHARs, altered after ReadValueChunk */
    WCHAR *str;   /* allocated null-terminated string */
} strval;

static WCHAR emptyW[] = {0};
static const strval strval_empty = {emptyW, 0, emptyW};

struct attribute
{
    struct list entry;
    strval localname;
    strval value;
};

struct element
{
    struct list entry;
    strval qname;
};

typedef struct
{
    IXmlReader IXmlReader_iface;
    LONG ref;
    xmlreaderinput *input;
    IMalloc *imalloc;
    XmlReadState state;
    XmlReaderInternalState instate;
    XmlReaderResumeState resumestate;
    XmlNodeType nodetype;
    DtdProcessing dtdmode;
    UINT line, pos;           /* reader position in XML stream */
    struct list attrs; /* attributes list for current node */
    struct attribute *attr; /* current attribute */
    UINT attr_count;
    struct list elements;
    strval strvalues[StringValue_Last];
    UINT depth;
    WCHAR *resume[XmlReadResume_Last]; /* pointers used to resume reader */
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

static HRESULT reader_strvaldup(xmlreader *reader, const strval *src, strval *dest)
{
    *dest = *src;

    if (src->str != strval_empty.str)
    {
        dest->str = reader_alloc(reader, (dest->len+1)*sizeof(WCHAR));
        if (!dest->str) return E_OUTOFMEMORY;
        memcpy(dest->str, src->str, dest->len*sizeof(WCHAR));
        dest->str[dest->len] = 0;
    }

    return S_OK;
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

/* This one frees stored string value if needed */
static void reader_free_strvalued(xmlreader *reader, strval *v)
{
    if (v->str != strval_empty.str)
    {
        reader_free(reader, v->str);
        *v = strval_empty;
    }
}

static inline void reader_init_strvalue(WCHAR *str, UINT len, strval *v)
{
    v->start = v->str = str;
    v->len = len;
}

static void reader_free_strvalue(xmlreader *reader, XmlReaderStringValue type)
{
    reader_free_strvalued(reader, &reader->strvalues[type]);
}

static void reader_free_strvalues(xmlreader *reader)
{
    int type;
    for (type = 0; type < StringValue_Last; type++)
        reader_free_strvalue(reader, type);
}

/* This helper should only be used to test if strings are the same,
   it doesn't try to sort. */
static inline int strval_eq(const strval *str1, const strval *str2)
{
    if (str1->len != str2->len) return 0;
    return !memcmp(str1->str, str2->str, str1->len*sizeof(WCHAR));
}

static void reader_clear_elements(xmlreader *reader)
{
    struct element *elem, *elem2;
    LIST_FOR_EACH_ENTRY_SAFE(elem, elem2, &reader->elements, struct element, entry)
    {
        reader_free_strvalued(reader, &elem->qname);
        reader_free(reader, elem);
    }
    list_init(&reader->elements);
}

static HRESULT reader_inc_depth(xmlreader *reader)
{
    /* FIXME: handle XmlReaderProperty_MaxElementDepth property */
    reader->depth++;
    return S_OK;
}

static HRESULT reader_push_element(xmlreader *reader, strval *qname)
{
    struct element *elem;
    HRESULT hr;

    elem = reader_alloc(reader, sizeof(*elem));
    if (!elem) return E_OUTOFMEMORY;

    hr = reader_strvaldup(reader, qname, &elem->qname);
    if (FAILED(hr)) return hr;

    if (!list_empty(&reader->elements))
    {
        hr = reader_inc_depth(reader);
        if (FAILED(hr)) return hr;
    }

    list_add_head(&reader->elements, &elem->entry);
    return hr;
}

static void reader_pop_element(xmlreader *reader)
{
    struct element *elem = LIST_ENTRY(list_head(&reader->elements), struct element, entry);

    if (elem)
    {
        list_remove(&elem->entry);
        reader_free_strvalued(reader, &elem->qname);
        reader_free(reader, elem);
    }
}

/* Always make a copy, cause strings are supposed to be null terminated. Null pointer for 'value'
   means node value is to be determined. */
static void reader_set_strvalue(xmlreader *reader, XmlReaderStringValue type, const strval *value)
{
    strval *v = &reader->strvalues[type];

    reader_free_strvalue(reader, type);
    if (!value)
    {
        v->str = NULL;
        v->start = NULL;
        v->len = 0;
        return;
    }

    if (value->str == strval_empty.str)
        *v = *value;
    else
    {
        if (type == StringValue_Value)
        {
            /* defer allocation for value string */
            v->str = NULL;
            v->start = value->start;
            v->len = value->len;
        }
        else
        {
            v->str = reader_alloc(reader, (value->len + 1)*sizeof(WCHAR));
            memcpy(v->str, value->start, value->len*sizeof(WCHAR));
            v->str[value->len] = 0;
            v->len = value->len;
        }
    }
}

static inline int is_reader_pending(xmlreader *reader)
{
    return reader->input->pending;
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
    /* to make sure aligned length won't exceed allocated length */
    ULONG len = buffer->allocated - buffer->written - 4;
    ULONG read;
    HRESULT hr;

    /* always try to get aligned to 4 bytes, so the only case we can get partially read characters is
       variable width encodings like UTF-8 */
    len = (len + 3) & ~3;
    /* try to use allocated space or grow */
    if (buffer->allocated - buffer->written < len)
    {
        buffer->allocated *= 2;
        buffer->data = readerinput_realloc(readerinput, buffer->data, buffer->allocated);
        len = buffer->allocated - buffer->written;
    }

    read = 0;
    hr = ISequentialStream_Read(readerinput->stream, buffer->data + buffer->written, len, &read);
    TRACE("requested %d, read %d, ret 0x%08x\n", len, read, hr);
    readerinput->pending = hr == E_PENDING;
    if (FAILED(hr)) return hr;
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

static inline int readerinput_is_utf8(xmlreaderinput *readerinput)
{
    static char startA[] = {'<','?'};
    static char commentA[] = {'<','!'};
    encoded_buffer *buffer = &readerinput->buffer->encoded;
    unsigned char *ptr = (unsigned char*)buffer->data;

    return !memcmp(buffer->data, startA, sizeof(startA)) ||
           !memcmp(buffer->data, commentA, sizeof(commentA)) ||
           /* test start byte */
           (ptr[0] == '<' &&
            (
             (ptr[1] && (ptr[1] <= 0x7f)) ||
             (buffer->data[1] >> 5) == 0x6  || /* 2 bytes */
             (buffer->data[1] >> 4) == 0xe  || /* 3 bytes */
             (buffer->data[1] >> 3) == 0x1e)   /* 4 bytes */
           );
}

static HRESULT readerinput_detectencoding(xmlreaderinput *readerinput, xml_encoding *enc)
{
    encoded_buffer *buffer = &readerinput->buffer->encoded;
    static WCHAR startW[] = {'<','?'};
    static WCHAR commentW[] = {'<','!'};
    static char utf8bom[] = {0xef,0xbb,0xbf};
    static char utf16lebom[] = {0xff,0xfe};

    *enc = XmlEncoding_Unknown;

    if (buffer->written <= 3)
    {
        HRESULT hr = readerinput_growraw(readerinput);
        if (FAILED(hr)) return hr;
        if (buffer->written <= 3) return MX_E_INPUTEND;
    }

    /* try start symbols if we have enough data to do that, input buffer should contain
       first chunk already */
    if (readerinput_is_utf8(readerinput))
        *enc = XmlEncoding_UTF8;
    else if (!memcmp(buffer->data, startW, sizeof(startW)) ||
             !memcmp(buffer->data, commentW, sizeof(commentW)))
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

/* Returns byte length of complete char sequence for buffer code page,
   it's relative to current buffer position which is currently used for BOM handling
   only. */
static int readerinput_get_convlen(xmlreaderinput *readerinput)
{
    encoded_buffer *buffer = &readerinput->buffer->encoded;
    int len;

    if (readerinput->buffer->code_page == CP_UTF8)
        len = readerinput_get_utf8_convlen(readerinput);
    else
        len = buffer->written;

    TRACE("%d\n", len - (int)(buffer->cur - buffer->data));
    return len - (buffer->cur - buffer->data);
}

/* It's possible that raw buffer has some leftovers from last conversion - some char
   sequence that doesn't represent a full code point. Length argument should be calculated with
   readerinput_get_convlen(), if it's -1 it will be calculated here. */
static void readerinput_shrinkraw(xmlreaderinput *readerinput, int len)
{
    encoded_buffer *buffer = &readerinput->buffer->encoded;

    if (len == -1)
        len = readerinput_get_convlen(readerinput);

    memmove(buffer->data, buffer->cur + (buffer->written - len), len);
    /* everything below cur is lost too */
    buffer->written -= len + (buffer->cur - buffer->data);
    /* after this point we don't need cur pointer really,
       it's used only to mark where actual data begins when first chunk is read */
    buffer->cur = buffer->data;
}

/* note that raw buffer content is kept */
static void readerinput_switchencoding(xmlreaderinput *readerinput, xml_encoding enc)
{
    encoded_buffer *src = &readerinput->buffer->encoded;
    encoded_buffer *dest = &readerinput->buffer->utf16;
    int len, dest_len;
    HRESULT hr;
    WCHAR *ptr;
    UINT cp;

    hr = get_code_page(enc, &cp);
    if (FAILED(hr)) return;

    readerinput->buffer->code_page = cp;
    len = readerinput_get_convlen(readerinput);

    TRACE("switching to cp %d\n", cp);

    /* just copy in this case */
    if (enc == XmlEncoding_UTF16)
    {
        readerinput_grow(readerinput, len);
        memcpy(dest->data, src->cur, len);
        dest->written += len*sizeof(WCHAR);
        return;
    }

    dest_len = MultiByteToWideChar(cp, 0, src->cur, len, NULL, 0);
    readerinput_grow(readerinput, dest_len);
    ptr = (WCHAR*)dest->data;
    MultiByteToWideChar(cp, 0, src->cur, len, ptr, dest_len);
    ptr[dest_len] = 0;
    dest->written += dest_len*sizeof(WCHAR);
}

/* shrinks parsed data a buffer begins with */
static void reader_shrink(xmlreader *reader)
{
    encoded_buffer *buffer = &reader->input->buffer->utf16;

    /* avoid to move too often using threshold shrink length */
    if (buffer->cur - buffer->data > buffer->written / 2)
    {
        buffer->written -= buffer->cur - buffer->data;
        memmove(buffer->data, buffer->cur, buffer->written);
        buffer->cur = buffer->data;
        *(WCHAR*)&buffer->cur[buffer->written] = 0;
    }
}

/* This is a normal way for reader to get new data converted from raw buffer to utf16 buffer.
   It won't attempt to shrink but will grow destination buffer if needed */
static HRESULT reader_more(xmlreader *reader)
{
    xmlreaderinput *readerinput = reader->input;
    encoded_buffer *src = &readerinput->buffer->encoded;
    encoded_buffer *dest = &readerinput->buffer->utf16;
    UINT cp = readerinput->buffer->code_page;
    int len, dest_len;
    HRESULT hr;
    WCHAR *ptr;

    /* get some raw data from stream first */
    hr = readerinput_growraw(readerinput);
    len = readerinput_get_convlen(readerinput);

    /* just copy for UTF-16 case */
    if (cp == ~0)
    {
        readerinput_grow(readerinput, len);
        memcpy(dest->data, src->cur, len);
        dest->written += len*sizeof(WCHAR);
        return hr;
    }

    dest_len = MultiByteToWideChar(cp, 0, src->cur, len, NULL, 0);
    readerinput_grow(readerinput, dest_len);
    ptr = (WCHAR*)dest->data;
    MultiByteToWideChar(cp, 0, src->cur, len, ptr, dest_len);
    ptr[dest_len] = 0;
    dest->written += dest_len*sizeof(WCHAR);
    /* get rid of processed data */
    readerinput_shrinkraw(readerinput, len);

    return hr;
}

static inline WCHAR *reader_get_cur(xmlreader *reader)
{
    WCHAR *ptr = (WCHAR*)reader->input->buffer->utf16.cur;
    if (!*ptr) reader_more(reader);
    return ptr;
}

static int reader_cmp(xmlreader *reader, const WCHAR *str)
{
    const WCHAR *ptr = reader_get_cur(reader);
    return strncmpW(str, ptr, strlenW(str));
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

static inline int is_wchar_space(WCHAR ch)
{
    return ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n';
}

/* [3] S ::= (#x20 | #x9 | #xD | #xA)+ */
static int reader_skipspaces(xmlreader *reader)
{
    encoded_buffer *buffer = &reader->input->buffer->utf16;
    const WCHAR *ptr = reader_get_cur(reader), *start = ptr;

    while (is_wchar_space(*ptr))
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
    WCHAR *ptr, *ptr2, *start = reader_get_cur(reader);
    static const WCHAR onedotW[] = {'1','.',0};

    if (reader_cmp(reader, onedotW)) return WC_E_XMLDECL;
    /* skip "1." */
    reader_skipn(reader, 2);

    ptr2 = ptr = reader_get_cur(reader);
    while (*ptr >= '0' && *ptr <= '9')
        ptr++;

    if (ptr2 == ptr) return WC_E_DIGIT;
    TRACE("version=%s\n", debugstr_wn(start, ptr-start));
    reader_init_strvalue(start, ptr-start, val);
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
    reader_init_strvalue(reader_get_cur(reader), 7, &name);
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
    WCHAR *start = reader_get_cur(reader), *ptr;
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
    WCHAR *start, *ptr;
    strval name, val;
    HRESULT hr;

    if (!reader_skipspaces(reader)) return WC_E_WHITESPACE;

    if (reader_cmp(reader, standaloneW)) return S_FALSE;
    reader_init_strvalue(reader_get_cur(reader), 10, &name);
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
    static const WCHAR xmldeclW[] = {'<','?','x','m','l',' ',0};
    static const WCHAR declcloseW[] = {'?','>',0};
    HRESULT hr;

    /* check if we have "<?xml " */
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

    reader->nodetype = XmlNodeType_XmlDeclaration;
    reader_set_strvalue(reader, StringValue_LocalName, &strval_empty);
    reader_set_strvalue(reader, StringValue_QualifiedName, &strval_empty);
    reader_set_strvalue(reader, StringValue_Value, &strval_empty);

    return S_OK;
}

/* [15] Comment ::= '<!--' ((Char - '-') | ('-' (Char - '-')))* '-->' */
static HRESULT reader_parse_comment(xmlreader *reader)
{
    WCHAR *start, *ptr;

    if (reader->resume[XmlReadResume_Body])
    {
        start = reader->resume[XmlReadResume_Body];
        ptr = reader_get_cur(reader);
    }
    else
    {
        /* skip '<!--' */
        reader_skipn(reader, 4);
        reader_shrink(reader);
        ptr = start = reader_get_cur(reader);
        reader->nodetype = XmlNodeType_Comment;
        reader->resume[XmlReadResume_Body] = start;
        reader->resumestate = XmlReadResumeState_Comment;
        reader_set_strvalue(reader, StringValue_LocalName, NULL);
        reader_set_strvalue(reader, StringValue_QualifiedName, NULL);
        reader_set_strvalue(reader, StringValue_Value, NULL);
    }

    /* will exit when there's no more data, it won't attempt to
       read more from stream */
    while (*ptr)
    {
        if (ptr[0] == '-')
        {
            if (ptr[1] == '-')
            {
                if (ptr[2] == '>')
                {
                    strval value;

                    TRACE("%s\n", debugstr_wn(start, ptr-start));
                    /* skip '-->' */
                    reader_skipn(reader, 3);
                    reader_init_strvalue(start, ptr-start, &value);
                    reader_set_strvalue(reader, StringValue_LocalName, &strval_empty);
                    reader_set_strvalue(reader, StringValue_QualifiedName, &strval_empty);
                    reader_set_strvalue(reader, StringValue_Value, &value);
                    reader->resume[XmlReadResume_Body] = NULL;
                    reader->resumestate = XmlReadResumeState_Initial;
                    return S_OK;
                }
                else
                    return WC_E_COMMENT;
            }
            else
                ptr++;
        }
        else
        {
            reader_skipn(reader, 1);
            ptr++;
        }
    }

    return S_OK;
}

/* [2] Char ::= #x9 | #xA | #xD | [#x20-#xD7FF] | [#xE000-#xFFFD] | [#x10000-#x10FFFF] */
static inline int is_char(WCHAR ch)
{
    return (ch == '\t') || (ch == '\r') || (ch == '\n') ||
           (ch >= 0x20 && ch <= 0xd7ff) ||
           (ch >= 0xd800 && ch <= 0xdbff) || /* high surrogate */
           (ch >= 0xdc00 && ch <= 0xdfff) || /* low surrogate */
           (ch >= 0xe000 && ch <= 0xfffd);
}

/* [13] PubidChar ::= #x20 | #xD | #xA | [a-zA-Z0-9] | [-'()+,./:=?;!*#@$_%] */
static inline int is_pubchar(WCHAR ch)
{
    return (ch == ' ') ||
           (ch >= 'a' && ch <= 'z') ||
           (ch >= 'A' && ch <= 'Z') ||
           (ch >= '0' && ch <= '9') ||
           (ch >= '-' && ch <= ';') || /* '()*+,-./:; */
           (ch == '=') || (ch == '?') ||
           (ch == '@') || (ch == '!') ||
           (ch >= '#' && ch <= '%') || /* #$% */
           (ch == '_') || (ch == '\r') || (ch == '\n');
}

static inline int is_namestartchar(WCHAR ch)
{
    return (ch == ':') || (ch >= 'A' && ch <= 'Z') ||
           (ch == '_') || (ch >= 'a' && ch <= 'z') ||
           (ch >= 0xc0   && ch <= 0xd6)   ||
           (ch >= 0xd8   && ch <= 0xf6)   ||
           (ch >= 0xf8   && ch <= 0x2ff)  ||
           (ch >= 0x370  && ch <= 0x37d)  ||
           (ch >= 0x37f  && ch <= 0x1fff) ||
           (ch >= 0x200c && ch <= 0x200d) ||
           (ch >= 0x2070 && ch <= 0x218f) ||
           (ch >= 0x2c00 && ch <= 0x2fef) ||
           (ch >= 0x3001 && ch <= 0xd7ff) ||
           (ch >= 0xd800 && ch <= 0xdbff) || /* high surrogate */
           (ch >= 0xdc00 && ch <= 0xdfff) || /* low surrogate */
           (ch >= 0xf900 && ch <= 0xfdcf) ||
           (ch >= 0xfdf0 && ch <= 0xfffd);
}

/* [4 NS] NCName ::= Name - (Char* ':' Char*) */
static inline int is_ncnamechar(WCHAR ch)
{
    return (ch >= 'A' && ch <= 'Z') ||
           (ch == '_') || (ch >= 'a' && ch <= 'z') ||
           (ch == '-') || (ch == '.') ||
           (ch >= '0'    && ch <= '9')    ||
           (ch == 0xb7)                   ||
           (ch >= 0xc0   && ch <= 0xd6)   ||
           (ch >= 0xd8   && ch <= 0xf6)   ||
           (ch >= 0xf8   && ch <= 0x2ff)  ||
           (ch >= 0x300  && ch <= 0x36f)  ||
           (ch >= 0x370  && ch <= 0x37d)  ||
           (ch >= 0x37f  && ch <= 0x1fff) ||
           (ch >= 0x200c && ch <= 0x200d) ||
           (ch >= 0x203f && ch <= 0x2040) ||
           (ch >= 0x2070 && ch <= 0x218f) ||
           (ch >= 0x2c00 && ch <= 0x2fef) ||
           (ch >= 0x3001 && ch <= 0xd7ff) ||
           (ch >= 0xd800 && ch <= 0xdbff) || /* high surrogate */
           (ch >= 0xdc00 && ch <= 0xdfff) || /* low surrogate */
           (ch >= 0xf900 && ch <= 0xfdcf) ||
           (ch >= 0xfdf0 && ch <= 0xfffd);
}

static inline int is_namechar(WCHAR ch)
{
    return (ch == ':') || is_ncnamechar(ch);
}

/* [4] NameStartChar ::= ":" | [A-Z] | "_" | [a-z] | [#xC0-#xD6] | [#xD8-#xF6] | [#xF8-#x2FF] | [#x370-#x37D] |
                            [#x37F-#x1FFF] | [#x200C-#x200D] | [#x2070-#x218F] | [#x2C00-#x2FEF] | [#x3001-#xD7FF] |
                            [#xF900-#xFDCF] | [#xFDF0-#xFFFD] | [#x10000-#xEFFFF]
   [4a] NameChar ::= NameStartChar | "-" | "." | [0-9] | #xB7 | [#x0300-#x036F] | [#x203F-#x2040]
   [5]  Name     ::= NameStartChar (NameChar)* */
static HRESULT reader_parse_name(xmlreader *reader, strval *name)
{
    WCHAR *ptr, *start;

    if (reader->resume[XmlReadResume_Name])
    {
        start = reader->resume[XmlReadResume_Name];
        ptr = reader_get_cur(reader);
    }
    else
    {
        ptr = start = reader_get_cur(reader);
        if (!is_namestartchar(*ptr)) return WC_E_NAMECHARACTER;
    }

    while (is_namechar(*ptr))
    {
        reader_skipn(reader, 1);
        ptr = reader_get_cur(reader);
    }

    if (is_reader_pending(reader))
    {
        reader->resume[XmlReadResume_Name] = start;
        return E_PENDING;
    }
    else
        reader->resume[XmlReadResume_Name] = NULL;

    TRACE("name %s:%d\n", debugstr_wn(start, ptr-start), (int)(ptr-start));
    reader_init_strvalue(start, ptr-start, name);

    return S_OK;
}

/* [17] PITarget ::= Name - (('X' | 'x') ('M' | 'm') ('L' | 'l')) */
static HRESULT reader_parse_pitarget(xmlreader *reader, strval *target)
{
    static const WCHAR xmlW[] = {'x','m','l'};
    strval name;
    HRESULT hr;
    UINT i;

    hr = reader_parse_name(reader, &name);
    if (FAILED(hr)) return is_reader_pending(reader) ? E_PENDING : WC_E_PI;

    /* now that we got name check for illegal content */
    if (name.len == 3 && !strncmpiW(name.str, xmlW, 3))
        return WC_E_LEADINGXML;

    /* PITarget can't be a qualified name */
    for (i = 0; i < name.len; i++)
        if (name.str[i] == ':')
            return i ? NC_E_NAMECOLON : WC_E_PI;

    TRACE("pitarget %s:%d\n", debugstr_wn(name.str, name.len), name.len);
    *target = name;
    return S_OK;
}

/* [16] PI ::= '<?' PITarget (S (Char* - (Char* '?>' Char*)))? '?>' */
static HRESULT reader_parse_pi(xmlreader *reader)
{
    WCHAR *ptr, *start;
    strval target;
    HRESULT hr;

    switch (reader->resumestate)
    {
    case XmlReadResumeState_Initial:
        /* skip '<?' */
        reader_skipn(reader, 2);
        reader_shrink(reader);
        reader->resumestate = XmlReadResumeState_PITarget;
    case XmlReadResumeState_PITarget:
        hr = reader_parse_pitarget(reader, &target);
        if (FAILED(hr)) return hr;
        reader->resumestate = XmlReadResumeState_PIBody;
    default:
        ;
    }

    ptr = reader_get_cur(reader);
    /* exit earlier if there's no content */
    if (ptr[0] == '?' && ptr[1] == '>')
    {
        /* skip '?>' */
        reader_skipn(reader, 2);
        reader->nodetype = XmlNodeType_ProcessingInstruction;
        reader->resumestate = XmlReadResumeState_Initial;
        reader_set_strvalue(reader, StringValue_LocalName, &target);
        reader_set_strvalue(reader, StringValue_QualifiedName, &target);
        reader_set_strvalue(reader, StringValue_Value, &strval_empty);
        return S_OK;
    }

    if (!reader->resume[XmlReadResume_Body])
    {
        /* now at least a single space char should be there */
        if (!is_wchar_space(*ptr)) return WC_E_WHITESPACE;
        reader_skipspaces(reader);
        ptr = start = reader_get_cur(reader);
        reader->resume[XmlReadResume_Body] = start;
    }
    else
    {
        start = reader->resume[XmlReadResume_Body];
        ptr = reader_get_cur(reader);
    }

    while (*ptr)
    {
        if (ptr[0] == '?')
        {
            if (ptr[1] == '>')
            {
                strval value;

                TRACE("%s\n", debugstr_wn(start, ptr-start));
                /* skip '?>' */
                reader_skipn(reader, 2);
                reader->nodetype = XmlNodeType_ProcessingInstruction;
                reader->resumestate = XmlReadResumeState_Initial;
                reader->resume[XmlReadResume_Body] = NULL;
                reader_init_strvalue(start, ptr-start, &value);
                reader_set_strvalue(reader, StringValue_LocalName, &target);
                reader_set_strvalue(reader, StringValue_QualifiedName, &target);
                reader_set_strvalue(reader, StringValue_Value, &value);
                return S_OK;
            }
            else
            {
                ptr++;
                reader_more(reader);
            }
        }
        else
        {
            reader_skipn(reader, 1);
            ptr = reader_get_cur(reader);
        }
    }

    return S_OK;
}

/* This one is used to parse significant whitespace nodes, like in Misc production */
static HRESULT reader_parse_whitespace(xmlreader *reader)
{
    WCHAR *start, *ptr;

    reader_shrink(reader);
    start = reader_get_cur(reader);

    reader_skipspaces(reader);
    ptr = reader_get_cur(reader);
    TRACE("%s\n", debugstr_wn(start, ptr-start));

    reader->nodetype = XmlNodeType_Whitespace;
    reader_set_strvalue(reader, StringValue_LocalName, &strval_empty);
    reader_set_strvalue(reader, StringValue_QualifiedName, &strval_empty);
    reader_set_strvalue(reader, StringValue_Value, &strval_empty);
    return S_OK;
}

/* [27] Misc ::= Comment | PI | S */
static HRESULT reader_parse_misc(xmlreader *reader)
{
    HRESULT hr = S_FALSE;

    if (reader->resumestate != XmlReadResumeState_Initial)
    {
        hr = reader_more(reader);
        if (FAILED(hr)) return hr;

        /* finish current node */
        switch (reader->resumestate)
        {
        case XmlReadResumeState_PITarget:
        case XmlReadResumeState_PIBody:
            return reader_parse_pi(reader);
        case XmlReadResumeState_Comment:
            return reader_parse_comment(reader);
        default:
            ERR("unknown resume state %d\n", reader->resumestate);
        }
    }

    while (1)
    {
        const WCHAR *cur = reader_get_cur(reader);

        if (is_wchar_space(*cur))
            hr = reader_parse_whitespace(reader);
        else if (!reader_cmp(reader, commentW))
            hr = reader_parse_comment(reader);
        else if (!reader_cmp(reader, piW))
            hr = reader_parse_pi(reader);
        else
            break;

        if (hr != S_FALSE) return hr;
    }

    return hr;
}

/* [11] SystemLiteral ::= ('"' [^"]* '"') | ("'" [^']* "'") */
static HRESULT reader_parse_sys_literal(xmlreader *reader, strval *literal)
{
    WCHAR *start = reader_get_cur(reader), *cur, quote;

    if (*start != '"' && *start != '\'') return WC_E_QUOTE;

    quote = *start;
    reader_skipn(reader, 1);

    cur = start = reader_get_cur(reader);
    while (is_char(*cur) && *cur != quote)
    {
        reader_skipn(reader, 1);
        cur = reader_get_cur(reader);
    }
    if (*cur == quote) reader_skipn(reader, 1);

    literal->str = start;
    literal->len = cur-start;
    TRACE("%s\n", debugstr_wn(start, cur-start));
    return S_OK;
}

/* [12] PubidLiteral ::= '"' PubidChar* '"' | "'" (PubidChar - "'")* "'"
   [13] PubidChar ::= #x20 | #xD | #xA | [a-zA-Z0-9] | [-'()+,./:=?;!*#@$_%] */
static HRESULT reader_parse_pub_literal(xmlreader *reader, strval *literal)
{
    WCHAR *start = reader_get_cur(reader), *cur, quote;

    if (*start != '"' && *start != '\'') return WC_E_QUOTE;

    quote = *start;
    reader_skipn(reader, 1);

    cur = start;
    while (is_pubchar(*cur) && *cur != quote)
    {
        reader_skipn(reader, 1);
        cur = reader_get_cur(reader);
    }

    reader_init_strvalue(start, cur-start, literal);
    TRACE("%s\n", debugstr_wn(start, cur-start));
    return S_OK;
}

/* [75] ExternalID ::= 'SYSTEM' S SystemLiteral | 'PUBLIC' S PubidLiteral S SystemLiteral */
static HRESULT reader_parse_externalid(xmlreader *reader)
{
    static WCHAR systemW[] = {'S','Y','S','T','E','M',0};
    static WCHAR publicW[] = {'P','U','B','L','I','C',0};
    strval name;
    HRESULT hr;
    int cnt;

    if (reader_cmp(reader, systemW))
    {
        if (reader_cmp(reader, publicW))
            return S_FALSE;
        else
        {
            strval pub;

            /* public id */
            reader_skipn(reader, 6);
            cnt = reader_skipspaces(reader);
            if (!cnt) return WC_E_WHITESPACE;

            hr = reader_parse_pub_literal(reader, &pub);
            if (FAILED(hr)) return hr;

            reader_init_strvalue(publicW, strlenW(publicW), &name);
            return reader_add_attr(reader, &name, &pub);
        }
    }
    else
    {
        strval sys;

        /* system id */
        reader_skipn(reader, 6);
        cnt = reader_skipspaces(reader);
        if (!cnt) return WC_E_WHITESPACE;

        hr = reader_parse_sys_literal(reader, &sys);
        if (FAILED(hr)) return hr;

        reader_init_strvalue(systemW, strlenW(systemW), &name);
        return reader_add_attr(reader, &name, &sys);
    }

    return hr;
}

/* [28] doctypedecl ::= '<!DOCTYPE' S Name (S ExternalID)? S? ('[' intSubset ']' S?)? '>' */
static HRESULT reader_parse_dtd(xmlreader *reader)
{
    static const WCHAR doctypeW[] = {'<','!','D','O','C','T','Y','P','E',0};
    strval name;
    WCHAR *cur;
    HRESULT hr;

    /* check if we have "<!DOCTYPE" */
    if (reader_cmp(reader, doctypeW)) return S_FALSE;
    reader_shrink(reader);

    /* DTD processing is not allowed by default */
    if (reader->dtdmode == DtdProcessing_Prohibit) return WC_E_DTDPROHIBITED;

    reader_skipn(reader, 9);
    if (!reader_skipspaces(reader)) return WC_E_WHITESPACE;

    /* name */
    hr = reader_parse_name(reader, &name);
    if (FAILED(hr)) return WC_E_DECLDOCTYPE;

    reader_skipspaces(reader);

    hr = reader_parse_externalid(reader);
    if (FAILED(hr)) return hr;

    reader_skipspaces(reader);

    cur = reader_get_cur(reader);
    if (*cur != '>')
    {
        FIXME("internal subset parsing not implemented\n");
        return E_NOTIMPL;
    }

    /* skip '>' */
    reader_skipn(reader, 1);

    reader->nodetype = XmlNodeType_DocumentType;
    reader_set_strvalue(reader, StringValue_LocalName, &name);
    reader_set_strvalue(reader, StringValue_QualifiedName, &name);

    return S_OK;
}

/* [11 NS] LocalPart ::= NCName */
static HRESULT reader_parse_local(xmlreader *reader, strval *local)
{
    WCHAR *ptr, *start;

    if (reader->resume[XmlReadResume_Local])
    {
        start = reader->resume[XmlReadResume_Local];
        ptr = reader_get_cur(reader);
    }
    else
    {
        ptr = start = reader_get_cur(reader);
    }

    while (is_ncnamechar(*ptr))
    {
        reader_skipn(reader, 1);
        ptr = reader_get_cur(reader);
    }

    if (is_reader_pending(reader))
    {
         reader->resume[XmlReadResume_Local] = start;
         return E_PENDING;
    }
    else
         reader->resume[XmlReadResume_Local] = NULL;

    reader_init_strvalue(start, ptr-start, local);

    return S_OK;
}

/* [7 NS]  QName ::= PrefixedName | UnprefixedName
   [8 NS]  PrefixedName ::= Prefix ':' LocalPart
   [9 NS]  UnprefixedName ::= LocalPart
   [10 NS] Prefix ::= NCName */
static HRESULT reader_parse_qname(xmlreader *reader, strval *prefix, strval *local, strval *qname)
{
    WCHAR *ptr, *start;
    HRESULT hr;

    if (reader->resume[XmlReadResume_Name])
    {
        start = reader->resume[XmlReadResume_Name];
        ptr = reader_get_cur(reader);
    }
    else
    {
        ptr = start = reader_get_cur(reader);
        reader->resume[XmlReadResume_Name] = start;
        if (!is_ncnamechar(*ptr)) return NC_E_QNAMECHARACTER;
    }

    if (reader->resume[XmlReadResume_Local])
    {
        hr = reader_parse_local(reader, local);
        if (FAILED(hr)) return hr;

        reader_init_strvalue(reader->resume[XmlReadResume_Name],
                             local->start - reader->resume[XmlReadResume_Name] - 1,
                             prefix);
    }
    else
    {
        /* skip prefix part */
        while (is_ncnamechar(*ptr))
        {
            reader_skipn(reader, 1);
            ptr = reader_get_cur(reader);
        }

        if (is_reader_pending(reader)) return E_PENDING;

        /* got a qualified name */
        if (*ptr == ':')
        {
            reader_init_strvalue(start, ptr-start, prefix);

            /* skip ':' */
            reader_skipn(reader, 1);
            hr = reader_parse_local(reader, local);
            if (FAILED(hr)) return hr;
        }
        else
        {
            reader_init_strvalue(reader->resume[XmlReadResume_Name], ptr-reader->resume[XmlReadResume_Name], local);
            reader_init_strvalue(NULL, 0, prefix);
        }
    }

    reader_init_strvalue(start, ptr-start, local);

    if (prefix->len)
        TRACE("qname %s:%s\n", debugstr_wn(prefix->start, prefix->len), debugstr_wn(local->start, local->len));
    else
        TRACE("ncname %s\n", debugstr_wn(local->start, local->len));

    reader_init_strvalue(prefix->start ? prefix->start : local->start,
                        /* count ':' too */
                        (prefix->len ? prefix->len + 1 : 0) + local->len,
                         qname);

    reader->resume[XmlReadResume_Name] = NULL;
    reader->resume[XmlReadResume_Local] = NULL;

    return S_OK;
}

/* [12 NS] STag ::= '<' QName (S Attribute)* S? '>'
   [14 NS] EmptyElemTag ::= '<' QName (S Attribute)* S? '/>' */
static HRESULT reader_parse_stag(xmlreader *reader, strval *prefix, strval *local, strval *qname, int *empty)
{
    static const WCHAR endW[] = {'/','>',0};
    HRESULT hr;

    hr = reader_parse_qname(reader, prefix, local, qname);
    if (FAILED(hr)) return hr;

    reader_skipspaces(reader);

    /* empty element */
    if ((*empty = !reader_cmp(reader, endW)))
    {
        /* skip '/>' */
        reader_skipn(reader, 2);
        return S_OK;
    }

    /* got a start tag */
    if (!reader_cmp(reader, gtW))
    {
        /* skip '>' */
        reader_skipn(reader, 1);
        return reader_push_element(reader, qname);
    }

    FIXME("only empty elements/start tags without attribute list supported\n");
    return E_NOTIMPL;
}

/* [39] element ::= EmptyElemTag | STag content ETag */
static HRESULT reader_parse_element(xmlreader *reader)
{
    HRESULT hr;

    switch (reader->resumestate)
    {
    case XmlReadResumeState_Initial:
        /* check if we are really on element */
        if (reader_cmp(reader, ltW)) return S_FALSE;

        /* skip '<' */
        reader_skipn(reader, 1);

        reader_shrink(reader);
        reader->resumestate = XmlReadResumeState_STag;
    case XmlReadResumeState_STag:
    {
        strval qname, prefix, local;
        int empty = 0;

        /* this handles empty elements too */
        hr = reader_parse_stag(reader, &prefix, &local, &qname, &empty);
        if (FAILED(hr)) return hr;

        /* FIXME: need to check for defined namespace to reject invalid prefix,
           currently reject all prefixes */
        if (prefix.len) return NC_E_UNDECLAREDPREFIX;

        /* if we got empty element and stack is empty go straight to Misc */
        if (empty && list_empty(&reader->elements))
            reader->instate = XmlReadInState_MiscEnd;
        else
            reader->instate = XmlReadInState_Content;

        reader->nodetype = XmlNodeType_Element;
        reader->resumestate = XmlReadResumeState_Initial;
        reader_set_strvalue(reader, StringValue_LocalName, &local);
        reader_set_strvalue(reader, StringValue_QualifiedName, &qname);
        break;
    }
    default:
        hr = E_FAIL;
    }

    return hr;
}

/* [13 NS] ETag ::= '</' QName S? '>' */
static HRESULT reader_parse_endtag(xmlreader *reader)
{
    strval prefix, local, qname;
    struct element *elem;
    HRESULT hr;

    /* skip '</' */
    reader_skipn(reader, 2);

    hr = reader_parse_qname(reader, &prefix, &local, &qname);
    if (FAILED(hr)) return hr;

    reader_skipspaces(reader);

    if (reader_cmp(reader, gtW)) return WC_E_GREATERTHAN;

    /* skip '>' */
    reader_skipn(reader, 1);

    /* Element stack should never be empty at this point, cause we shouldn't get to
       content parsing if it's empty. */
    elem = LIST_ENTRY(list_head(&reader->elements), struct element, entry);
    if (!strval_eq(&elem->qname, &qname)) return WC_E_ELEMENTMATCH;

    reader_pop_element(reader);

    reader->nodetype = XmlNodeType_EndElement;
    reader_set_strvalue(reader, StringValue_LocalName, &local);
    reader_set_strvalue(reader, StringValue_QualifiedName, &qname);

    return S_OK;
}

/* [18] CDSect ::= CDStart CData CDEnd
   [19] CDStart ::= '<![CDATA['
   [20] CData ::= (Char* - (Char* ']]>' Char*))
   [21] CDEnd ::= ']]>' */
static HRESULT reader_parse_cdata(xmlreader *reader)
{
    WCHAR *start, *ptr;

    if (reader->resume[XmlReadResume_Body])
    {
        start = reader->resume[XmlReadResume_Body];
        ptr = reader_get_cur(reader);
    }
    else
    {
        /* skip markup '<![CDATA[' */
        reader_skipn(reader, 9);
        reader_shrink(reader);
        ptr = start = reader_get_cur(reader);
        reader->nodetype = XmlNodeType_CDATA;
        reader->resume[XmlReadResume_Body] = start;
        reader->resumestate = XmlReadResumeState_CDATA;
        reader_set_strvalue(reader, StringValue_LocalName, NULL);
        reader_set_strvalue(reader, StringValue_QualifiedName, NULL);
        reader_set_strvalue(reader, StringValue_Value, NULL);
    }

    while (*ptr)
    {
        if (*ptr == ']' && *(ptr+1) == ']' && *(ptr+2) == '>')
        {
            strval value;

            TRACE("%s\n", debugstr_wn(start, ptr-start));
            /* skip ']]>' */
            reader_skipn(reader, 3);
            reader_init_strvalue(start, ptr-start, &value);
            reader_set_strvalue(reader, StringValue_LocalName, &strval_empty);
            reader_set_strvalue(reader, StringValue_QualifiedName, &strval_empty);
            reader_set_strvalue(reader, StringValue_Value, &value);
            reader->resume[XmlReadResume_Body] = NULL;
            reader->resumestate = XmlReadResumeState_Initial;
            return S_OK;
        }
        else
        {
            reader_skipn(reader, 1);
            ptr++;
        }
    }

    return S_OK;
}

/* [66] CharRef ::= '&#' [0-9]+ ';' | '&#x' [0-9a-fA-F]+ ';'
   [67]	Reference ::= EntityRef | CharRef
   [68]	EntityRef ::= '&' Name ';' */
static HRESULT reader_parse_reference(xmlreader *reader)
{
    FIXME("References not supported\n");
    return E_NOTIMPL;
}

/* [14] CharData ::= [^<&]* - ([^<&]* ']]>' [^<&]*) */
static HRESULT reader_parse_chardata(xmlreader *reader)
{
    FIXME("CharData not supported\n");
    return E_NOTIMPL;
}

/* [43] content ::= CharData? ((element | Reference | CDSect | PI | Comment) CharData?)* */
static HRESULT reader_parse_content(xmlreader *reader)
{
    static const WCHAR cdstartW[] = {'<','!','[','C','D','A','T','A','[',0};
    static const WCHAR etagW[] = {'<','/',0};
    static const WCHAR ampW[] = {'&',0};

    if (reader->resumestate != XmlReadResumeState_Initial)
    {
        switch (reader->resumestate)
        {
        case XmlReadResumeState_CDATA:
            return reader_parse_cdata(reader);
        case XmlReadResumeState_Comment:
            return reader_parse_comment(reader);
        case XmlReadResumeState_PIBody:
        case XmlReadResumeState_PITarget:
            return reader_parse_pi(reader);
        default:
            ERR("unknown resume state %d\n", reader->resumestate);
        }
    }

    reader_shrink(reader);

    /* handle end tag here, it indicates end of content as well */
    if (!reader_cmp(reader, etagW))
        return reader_parse_endtag(reader);

    if (!reader_cmp(reader, commentW))
        return reader_parse_comment(reader);

    if (!reader_cmp(reader, piW))
        return reader_parse_pi(reader);

    if (!reader_cmp(reader, cdstartW))
        return reader_parse_cdata(reader);

    if (!reader_cmp(reader, ampW))
        return reader_parse_reference(reader);

    if (!reader_cmp(reader, ltW))
        return reader_parse_element(reader);

    /* what's left must be CharData */
    return reader_parse_chardata(reader);
}

static HRESULT reader_parse_nextnode(xmlreader *reader)
{
    HRESULT hr;

    while (1)
    {
        switch (reader->instate)
        {
        /* if it's a first call for a new input we need to detect stream encoding */
        case XmlReadInState_Initial:
            {
                xml_encoding enc;

                hr = readerinput_growraw(reader->input);
                if (FAILED(hr)) return hr;

                /* try to detect encoding by BOM or data and set input code page */
                hr = readerinput_detectencoding(reader->input, &enc);
                TRACE("detected encoding %s, 0x%08x\n", debugstr_w(xml_encoding_map[enc].name), hr);
                if (FAILED(hr)) return hr;

                /* always switch first time cause we have to put something in */
                readerinput_switchencoding(reader->input, enc);

                /* parse xml declaration */
                hr = reader_parse_xmldecl(reader);
                if (FAILED(hr)) return hr;

                readerinput_shrinkraw(reader->input, -1);
                reader->instate = XmlReadInState_Misc_DTD;
                if (hr == S_OK) return hr;
            }
            break;
        case XmlReadInState_Misc_DTD:
            hr = reader_parse_misc(reader);
            if (FAILED(hr)) return hr;

            if (hr == S_FALSE)
                reader->instate = XmlReadInState_DTD;
            else
                return hr;
            break;
        case XmlReadInState_DTD:
            hr = reader_parse_dtd(reader);
            if (FAILED(hr)) return hr;

            if (hr == S_OK)
            {
                reader->instate = XmlReadInState_DTD_Misc;
                return hr;
            }
            else
                reader->instate = XmlReadInState_Element;
            break;
        case XmlReadInState_DTD_Misc:
            hr = reader_parse_misc(reader);
            if (FAILED(hr)) return hr;

            if (hr == S_FALSE)
                reader->instate = XmlReadInState_Element;
            else
                return hr;
            break;
        case XmlReadInState_Element:
            return reader_parse_element(reader);
        case XmlReadInState_Content:
            return reader_parse_content(reader);
        default:
            FIXME("internal state %d not handled\n", reader->instate);
            return E_NOTIMPL;
        }
    }

    return E_NOTIMPL;
}

static HRESULT WINAPI xmlreader_QueryInterface(IXmlReader *iface, REFIID riid, void** ppvObject)
{
    xmlreader *This = impl_from_IXmlReader(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppvObject);

    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_IXmlReader))
    {
        *ppvObject = iface;
    }
    else
    {
        FIXME("interface %s not implemented\n", debugstr_guid(riid));
        *ppvObject = NULL;
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
        reader_clear_elements(This);
        reader_free_strvalues(This);
        reader_free(This, This);
        if (imalloc) IMalloc_Release(imalloc);
    }

    return ref;
}

static HRESULT WINAPI xmlreader_SetInput(IXmlReader* iface, IUnknown *input)
{
    xmlreader *This = impl_from_IXmlReader(iface);
    IXmlReaderInput *readerinput;
    HRESULT hr;

    TRACE("(%p)->(%p)\n", This, input);

    if (This->input)
    {
        readerinput_release_stream(This->input);
        IUnknown_Release(&This->input->IXmlReaderInput_iface);
        This->input = NULL;
    }

    This->line = This->pos = 0;
    reader_clear_elements(This);
    This->depth = 0;
    This->resumestate = XmlReadResumeState_Initial;
    memset(This->resume, 0, sizeof(This->resume));

    /* just reset current input */
    if (!input)
    {
        This->state = XmlReadState_Initial;
        return S_OK;
    }

    /* now try IXmlReaderInput, ISequentialStream, IStream */
    hr = IUnknown_QueryInterface(input, &IID_IXmlReaderInput, (void**)&readerinput);
    if (hr == S_OK)
    {
        if (readerinput->lpVtbl == &xmlreaderinputvtbl)
            This->input = impl_from_IXmlReaderInput(readerinput);
        else
        {
            ERR("got external IXmlReaderInput implementation: %p, vtbl=%p\n",
                readerinput, readerinput->lpVtbl);
            IUnknown_Release(readerinput);
            return E_FAIL;

        }
    }

    if (hr != S_OK || !readerinput)
    {
        /* create IXmlReaderInput basing on supplied interface */
        hr = CreateXmlReaderInputWithEncodingName(input,
                                         NULL, NULL, FALSE, NULL, &readerinput);
        if (hr != S_OK) return hr;
        This->input = impl_from_IXmlReaderInput(readerinput);
    }

    /* set stream for supplied IXmlReaderInput */
    hr = readerinput_query_for_stream(This->input);
    if (hr == S_OK)
    {
        This->state = XmlReadState_Initial;
        This->instate = XmlReadInState_Initial;
    }

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

static HRESULT WINAPI xmlreader_Read(IXmlReader* iface, XmlNodeType *nodetype)
{
    xmlreader *This = impl_from_IXmlReader(iface);
    XmlNodeType oldtype = This->nodetype;
    HRESULT hr;

    TRACE("(%p)->(%p)\n", This, nodetype);

    if (This->state == XmlReadState_Closed) return S_FALSE;

    hr = reader_parse_nextnode(This);
    if (oldtype == XmlNodeType_None && This->nodetype != oldtype)
        This->state = XmlReadState_Interactive;
    if (hr == S_OK) *nodetype = This->nodetype;

    return hr;
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

static HRESULT WINAPI xmlreader_GetQualifiedName(IXmlReader* iface, LPCWSTR *name, UINT *len)
{
    xmlreader *This = impl_from_IXmlReader(iface);

    TRACE("(%p)->(%p %p)\n", This, name, len);
    *name = This->strvalues[StringValue_QualifiedName].str;
    *len  = This->strvalues[StringValue_QualifiedName].len;
    return S_OK;
}

static HRESULT WINAPI xmlreader_GetNamespaceUri(IXmlReader* iface,
                                                LPCWSTR *namespaceUri,
                                                UINT *namespaceUri_length)
{
    FIXME("(%p %p %p): stub\n", iface, namespaceUri, namespaceUri_length);
    return E_NOTIMPL;
}

static HRESULT WINAPI xmlreader_GetLocalName(IXmlReader* iface, LPCWSTR *name, UINT *len)
{
    xmlreader *This = impl_from_IXmlReader(iface);

    TRACE("(%p)->(%p %p)\n", This, name, len);
    *name = This->strvalues[StringValue_LocalName].str;
    *len  = This->strvalues[StringValue_LocalName].len;
    return S_OK;
}

static HRESULT WINAPI xmlreader_GetPrefix(IXmlReader* iface,
                                          LPCWSTR *prefix,
                                          UINT *prefix_length)
{
    FIXME("(%p %p %p): stub\n", iface, prefix, prefix_length);
    return E_NOTIMPL;
}

static HRESULT WINAPI xmlreader_GetValue(IXmlReader* iface, const WCHAR **value, UINT *len)
{
    xmlreader *reader = impl_from_IXmlReader(iface);
    strval *val = &reader->strvalues[StringValue_Value];

    TRACE("(%p)->(%p %p)\n", reader, value, len);

    *value = NULL;

    if ((reader->nodetype == XmlNodeType_Comment && !val->str) || is_reader_pending(reader))
    {
        XmlNodeType type;
        HRESULT hr;

        hr = IXmlReader_Read(iface, &type);
        if (FAILED(hr)) return hr;

        /* return if still pending, partially read values are not reported */
        if (is_reader_pending(reader)) return E_PENDING;
    }

    if (!val->str)
    {
        val->str = reader_alloc(reader, (val->len+1)*sizeof(WCHAR));
        if (!val->str) return E_OUTOFMEMORY;
        memcpy(val->str, val->start, val->len*sizeof(WCHAR));
        val->str[val->len] = 0;
    }

    *value = val->str;
    if (len) *len = val->len;
    return S_OK;
}

static HRESULT WINAPI xmlreader_ReadValueChunk(IXmlReader* iface, WCHAR *buffer, UINT chunk_size, UINT *read)
{
    xmlreader *reader = impl_from_IXmlReader(iface);
    strval *val = &reader->strvalues[StringValue_Value];
    UINT len;

    TRACE("(%p)->(%p %u %p)\n", reader, buffer, chunk_size, read);

    /* Value is already allocated, chunked reads are not possible. */
    if (val->str) return S_FALSE;

    if (val->len)
    {
        len = min(chunk_size, val->len);
        memcpy(buffer, val->start, len);
        val->start += len;
        val->len -= len;
        if (read) *read = len;
    }

    return S_OK;
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
    xmlreader *This = impl_from_IXmlReader(iface);
    TRACE("(%p)->(%p)\n", This, depth);
    *depth = This->depth;
    return S_OK;
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

    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppvObject);

    if (IsEqualGUID(riid, &IID_IXmlReaderInput) ||
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

static const struct IUnknownVtbl xmlreaderinputvtbl =
{
    xmlreaderinput_QueryInterface,
    xmlreaderinput_AddRef,
    xmlreaderinput_Release
};

HRESULT WINAPI CreateXmlReader(REFIID riid, void **obj, IMalloc *imalloc)
{
    xmlreader *reader;
    int i;

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
    reader->instate = XmlReadInState_Initial;
    reader->resumestate = XmlReadResumeState_Initial;
    reader->dtdmode = DtdProcessing_Prohibit;
    reader->line  = reader->pos = 0;
    reader->imalloc = imalloc;
    if (imalloc) IMalloc_AddRef(imalloc);
    reader->nodetype = XmlNodeType_None;
    list_init(&reader->attrs);
    reader->attr_count = 0;
    reader->attr = NULL;
    list_init(&reader->elements);
    reader->depth = 0;
    memset(reader->resume, 0, sizeof(reader->resume));

    for (i = 0; i < StringValue_Last; i++)
        reader->strvalues[i] = strval_empty;

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

    readerinput->IXmlReaderInput_iface.lpVtbl = &xmlreaderinputvtbl;
    readerinput->ref = 1;
    readerinput->imalloc = imalloc;
    readerinput->stream = NULL;
    if (imalloc) IMalloc_AddRef(imalloc);
    readerinput->encoding = parse_encoding_name(encoding, -1);
    readerinput->hint = hint;
    readerinput->baseuri = readerinput_strdupW(readerinput, base_uri);
    readerinput->pending = 0;

    hr = alloc_input_buffer(readerinput);
    if (hr != S_OK)
    {
        readerinput_free(readerinput, readerinput->baseuri);
        readerinput_free(readerinput, readerinput);
        if (imalloc) IMalloc_Release(imalloc);
        return hr;
    }
    IUnknown_QueryInterface(stream, &IID_IUnknown, (void**)&readerinput->input);

    *ppInput = &readerinput->IXmlReaderInput_iface;

    TRACE("returning iface %p\n", *ppInput);

    return S_OK;
}
