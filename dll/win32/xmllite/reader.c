/*
 * IXmlReader implementation
 *
 * Copyright 2010, 2012-2013, 2016-2017 Nikolay Sivov
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

#include <stdio.h>
#include <stdarg.h>
#include <assert.h>
#include "windef.h"
#include "winbase.h"
#include "initguid.h"
#include "objbase.h"
#include "xmllite.h"
#include "xmllite_private.h"
#ifdef __REACTOS__
#include <winnls.h>
#endif

#include "wine/debug.h"
#include "wine/list.h"

WINE_DEFAULT_DEBUG_CHANNEL(xmllite);

/* not defined in public headers */
DEFINE_GUID(IID_IXmlReaderInput, 0x0b3ccc9b, 0x9214, 0x428b, 0xa2, 0xae, 0xef, 0x3a, 0xa8, 0x71, 0xaf, 0xda);

typedef enum
{
    XmlReadInState_Initial,
    XmlReadInState_XmlDecl,
    XmlReadInState_Misc_DTD,
    XmlReadInState_DTD,
    XmlReadInState_DTD_Misc,
    XmlReadInState_Element,
    XmlReadInState_Content,
    XmlReadInState_MiscEnd, /* optional Misc at the end of a document */
    XmlReadInState_Eof
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
    XmlReadResumeState_STag,
    XmlReadResumeState_CharData,
    XmlReadResumeState_Whitespace
} XmlReaderResumeState;

/* saved pointer index to resume from particular input position */
typedef enum
{
    XmlReadResume_Name,  /* PITarget, name for NCName, prefix for QName */
    XmlReadResume_Local, /* local for QName */
    XmlReadResume_Body,  /* PI body, comment text, CDATA text, CharData text */
    XmlReadResume_Last
} XmlReaderResume;

typedef enum
{
    StringValue_LocalName,
    StringValue_Prefix,
    StringValue_QualifiedName,
    StringValue_Value,
    StringValue_Last
} XmlReaderStringValue;

static const WCHAR usasciiW[] = {'U','S','-','A','S','C','I','I',0};
static const WCHAR utf16W[] = {'U','T','F','-','1','6',0};
static const WCHAR utf8W[] = {'U','T','F','-','8',0};

static const WCHAR dblquoteW[] = {'\"',0};
static const WCHAR quoteW[] = {'\'',0};
static const WCHAR ltW[] = {'<',0};
static const WCHAR gtW[] = {'>',0};
static const WCHAR commentW[] = {'<','!','-','-',0};
static const WCHAR piW[] = {'<','?',0};

BOOL is_namestartchar(WCHAR ch);

static const char *debugstr_nodetype(XmlNodeType nodetype)
{
    static const char * const type_names[] =
    {
        "None",
        "Element",
        "Attribute",
        "Text",
        "CDATA",
        "",
        "",
        "ProcessingInstruction",
        "Comment",
        "",
        "DocumentType",
        "",
        "",
        "Whitespace",
        "",
        "EndElement",
        "",
        "XmlDeclaration"
    };

    if (nodetype > _XmlNodeType_Last)
        return wine_dbg_sprintf("unknown type=%d", nodetype);

    return type_names[nodetype];
}

static const char *debugstr_reader_prop(XmlReaderProperty prop)
{
    static const char * const prop_names[] =
    {
        "MultiLanguage",
        "ConformanceLevel",
        "RandomAccess",
        "XmlResolver",
        "DtdProcessing",
        "ReadState",
        "MaxElementDepth",
        "MaxEntityExpansion"
    };

    if (prop > _XmlReaderProperty_Last)
        return wine_dbg_sprintf("unknown property=%d", prop);

    return prop_names[prop];
}

struct xml_encoding_data
{
    const WCHAR *name;
    xml_encoding enc;
    UINT cp;
};

static const struct xml_encoding_data xml_encoding_map[] = {
    { usasciiW, XmlEncoding_USASCII, 20127 },
    { utf16W, XmlEncoding_UTF16, 1200 },
    { utf8W,  XmlEncoding_UTF8,  CP_UTF8 },
};

const WCHAR *get_encoding_name(xml_encoding encoding)
{
    return xml_encoding_map[encoding].name;
}

xml_encoding get_encoding_from_codepage(UINT codepage)
{
    int i;
    for (i = 0; i < ARRAY_SIZE(xml_encoding_map); i++)
    {
        if (xml_encoding_map[i].cp == codepage) return xml_encoding_map[i].enc;
    }
    return XmlEncoding_Unknown;
}

typedef struct
{
    char *data;
    UINT  cur;
    unsigned int allocated;
    unsigned int written;
    BOOL prev_cr;
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
    WCHAR *str;   /* allocated null-terminated string */
    UINT   len;   /* length in WCHARs, altered after ReadValueChunk */
    UINT   start; /* input position where value starts */
} strval;

static WCHAR emptyW[] = {0};
static WCHAR xmlW[] = {'x','m','l',0};
static WCHAR xmlnsW[] = {'x','m','l','n','s',0};
static const strval strval_empty = { emptyW };
static const strval strval_xml = { xmlW, 3 };
static const strval strval_xmlns = { xmlnsW, 5 };

struct reader_position
{
    UINT line_number;
    UINT line_position;
};

enum attribute_flags
{
    ATTRIBUTE_NS_DEFINITION = 0x1,
    ATTRIBUTE_DEFAULT_NS_DEFINITION = 0x2,
};

struct attribute
{
    struct list entry;
    strval prefix;
    strval localname;
    strval qname;
    strval value;
    struct reader_position position;
    unsigned int flags;
};

struct element
{
    struct list entry;
    strval prefix;
    strval localname;
    strval qname;
    struct reader_position position;
};

struct ns
{
    struct list entry;
    strval prefix;
    strval uri;
    struct element *element;
};

typedef struct
{
    IXmlReader IXmlReader_iface;
    LONG ref;
    xmlreaderinput *input;
    IMalloc *imalloc;
    XmlReadState state;
    HRESULT error; /* error set on XmlReadState_Error */
    XmlReaderInternalState instate;
    XmlReaderResumeState resumestate;
    XmlNodeType nodetype;
    DtdProcessing dtdmode;
    IXmlResolver *resolver;
    IUnknown *mlang;
    struct reader_position position;
    struct list attrs; /* attributes list for current node */
    struct attribute *attr; /* current attribute */
    UINT attr_count;
    struct list nsdef;
    struct list ns;
    struct list elements;
    int chunk_read_off;
    strval strvalues[StringValue_Last];
    UINT depth;
    UINT max_depth;
    BOOL is_empty_element;
    struct element empty_element; /* used for empty elements without end tag <a />,
                                     and to keep <?xml reader position */
    UINT resume[XmlReadResume_Last]; /* offsets used to resume reader */
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

/* reader memory allocation functions */
static inline void *reader_alloc(xmlreader *reader, size_t len)
{
    return m_alloc(reader->imalloc, len);
}

static inline void *reader_alloc_zero(xmlreader *reader, size_t len)
{
    void *ret = reader_alloc(reader, len);
    if (ret)
        memset(ret, 0, len);
    return ret;
}

static inline void reader_free(xmlreader *reader, void *mem)
{
    m_free(reader->imalloc, mem);
}

/* Just return pointer from offset, no attempt to read more. */
static inline WCHAR *reader_get_ptr2(const xmlreader *reader, UINT offset)
{
    encoded_buffer *buffer = &reader->input->buffer->utf16;
    return (WCHAR*)buffer->data + offset;
}

static inline WCHAR *reader_get_strptr(const xmlreader *reader, const strval *v)
{
    return v->str ? v->str : reader_get_ptr2(reader, v->start);
}

static HRESULT reader_strvaldup(xmlreader *reader, const strval *src, strval *dest)
{
    *dest = *src;

    if (src->str != strval_empty.str)
    {
        dest->str = reader_alloc(reader, (dest->len+1)*sizeof(WCHAR));
        if (!dest->str) return E_OUTOFMEMORY;
        memcpy(dest->str, reader_get_strptr(reader, src), dest->len*sizeof(WCHAR));
        dest->str[dest->len] = 0;
        dest->start = 0;
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

        size = (lstrlenW(str)+1)*sizeof(WCHAR);
        ret = readerinput_alloc(input, size);
        if (ret) memcpy(ret, str, size);
    }

    return ret;
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

static void reader_clear_attrs(xmlreader *reader)
{
    struct attribute *attr, *attr2;
    LIST_FOR_EACH_ENTRY_SAFE(attr, attr2, &reader->attrs, struct attribute, entry)
    {
        reader_free_strvalued(reader, &attr->localname);
        reader_free_strvalued(reader, &attr->value);
        reader_free(reader, attr);
    }
    list_init(&reader->attrs);
    reader->attr_count = 0;
    reader->attr = NULL;
}

/* attribute data holds pointers to buffer data, so buffer shrink is not possible
   while we are on a node with attributes */
static HRESULT reader_add_attr(xmlreader *reader, strval *prefix, strval *localname, strval *qname,
    strval *value, const struct reader_position *position, unsigned int flags)
{
    struct attribute *attr;
    HRESULT hr;

    attr = reader_alloc(reader, sizeof(*attr));
    if (!attr) return E_OUTOFMEMORY;

    hr = reader_strvaldup(reader, localname, &attr->localname);
    if (hr == S_OK)
    {
        hr = reader_strvaldup(reader, value, &attr->value);
        if (hr != S_OK)
            reader_free_strvalued(reader, &attr->value);
    }
    if (hr != S_OK)
    {
        reader_free(reader, attr);
        return hr;
    }

    if (prefix)
        attr->prefix = *prefix;
    else
        memset(&attr->prefix, 0, sizeof(attr->prefix));
    attr->qname = qname ? *qname : *localname;
    attr->position = *position;
    attr->flags = flags;
    list_add_tail(&reader->attrs, &attr->entry);
    reader->attr_count++;

    return S_OK;
}

/* Returns current element, doesn't check if reader is actually positioned on it. */
static struct element *reader_get_element(xmlreader *reader)
{
    if (reader->is_empty_element)
        return &reader->empty_element;

    return LIST_ENTRY(list_head(&reader->elements), struct element, entry);
}

static inline void reader_init_strvalue(UINT start, UINT len, strval *v)
{
    v->start = start;
    v->len = len;
    v->str = NULL;
}

static inline const char* debug_strval(const xmlreader *reader, const strval *v)
{
    return debugstr_wn(reader_get_strptr(reader, v), v->len);
}

/* used to initialize from constant string */
static inline void reader_init_cstrvalue(WCHAR *str, UINT len, strval *v)
{
    v->start = 0;
    v->len = len;
    v->str = str;
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
static inline int strval_eq(const xmlreader *reader, const strval *str1, const strval *str2)
{
    if (str1->len != str2->len) return 0;
    return !memcmp(reader_get_strptr(reader, str1), reader_get_strptr(reader, str2), str1->len*sizeof(WCHAR));
}

static void reader_clear_elements(xmlreader *reader)
{
    struct element *elem, *elem2;
    LIST_FOR_EACH_ENTRY_SAFE(elem, elem2, &reader->elements, struct element, entry)
    {
        reader_free_strvalued(reader, &elem->prefix);
        reader_free_strvalued(reader, &elem->localname);
        reader_free_strvalued(reader, &elem->qname);
        reader_free(reader, elem);
    }
    list_init(&reader->elements);
    reader_free_strvalued(reader, &reader->empty_element.localname);
    reader_free_strvalued(reader, &reader->empty_element.qname);
    reader->is_empty_element = FALSE;
}

static struct ns *reader_lookup_ns(xmlreader *reader, const strval *prefix)
{
    struct list *nslist = prefix ? &reader->ns : &reader->nsdef;
    struct ns *ns;

    LIST_FOR_EACH_ENTRY_REV(ns, nslist, struct ns, entry) {
        if (strval_eq(reader, prefix, &ns->prefix))
            return ns;
    }

    return NULL;
}

static HRESULT reader_inc_depth(xmlreader *reader)
{
    return (++reader->depth >= reader->max_depth && reader->max_depth) ? SC_E_MAXELEMENTDEPTH : S_OK;
}

static void reader_dec_depth(xmlreader *reader)
{
    if (reader->depth)
        reader->depth--;
}

static HRESULT reader_push_ns(xmlreader *reader, const strval *prefix, const strval *uri, BOOL def)
{
    struct ns *ns;
    HRESULT hr;

    ns = reader_alloc(reader, sizeof(*ns));
    if (!ns) return E_OUTOFMEMORY;

    if (def)
        memset(&ns->prefix, 0, sizeof(ns->prefix));
    else {
        hr = reader_strvaldup(reader, prefix, &ns->prefix);
        if (FAILED(hr)) {
            reader_free(reader, ns);
            return hr;
        }
    }

    hr = reader_strvaldup(reader, uri, &ns->uri);
    if (FAILED(hr)) {
        reader_free_strvalued(reader, &ns->prefix);
        reader_free(reader, ns);
        return hr;
    }

    ns->element = NULL;
    list_add_head(def ? &reader->nsdef : &reader->ns, &ns->entry);
    return hr;
}

static void reader_free_element(xmlreader *reader, struct element *element)
{
    reader_free_strvalued(reader, &element->prefix);
    reader_free_strvalued(reader, &element->localname);
    reader_free_strvalued(reader, &element->qname);
    reader_free(reader, element);
}

static void reader_mark_ns_nodes(xmlreader *reader, struct element *element)
{
    struct ns *ns;

    LIST_FOR_EACH_ENTRY(ns, &reader->ns, struct ns, entry) {
        if (ns->element)
            break;
        ns->element = element;
    }

    LIST_FOR_EACH_ENTRY(ns, &reader->nsdef, struct ns, entry) {
        if (ns->element)
            break;
        ns->element = element;
    }
}

static HRESULT reader_push_element(xmlreader *reader, strval *prefix, strval *localname,
    strval *qname, const struct reader_position *position)
{
    struct element *element;
    HRESULT hr;

    element = reader_alloc_zero(reader, sizeof(*element));
    if (!element)
        return E_OUTOFMEMORY;

    if ((hr = reader_strvaldup(reader, prefix, &element->prefix)) == S_OK &&
            (hr = reader_strvaldup(reader, localname, &element->localname)) == S_OK &&
            (hr = reader_strvaldup(reader, qname, &element->qname)) == S_OK)
    {
        list_add_head(&reader->elements, &element->entry);
        reader_mark_ns_nodes(reader, element);
        reader->is_empty_element = FALSE;
        element->position = *position;
    }
    else
        reader_free_element(reader, element);

    return hr;
}

static void reader_pop_ns_nodes(xmlreader *reader, struct element *element)
{
    struct ns *ns, *ns2;

    LIST_FOR_EACH_ENTRY_SAFE_REV(ns, ns2, &reader->ns, struct ns, entry) {
        if (ns->element != element)
            break;

        list_remove(&ns->entry);
        reader_free_strvalued(reader, &ns->prefix);
        reader_free_strvalued(reader, &ns->uri);
        reader_free(reader, ns);
    }

    if (!list_empty(&reader->nsdef)) {
        ns = LIST_ENTRY(list_head(&reader->nsdef), struct ns, entry);
        if (ns->element == element) {
            list_remove(&ns->entry);
            reader_free_strvalued(reader, &ns->prefix);
            reader_free_strvalued(reader, &ns->uri);
            reader_free(reader, ns);
        }
    }
}

static void reader_pop_element(xmlreader *reader)
{
    struct element *element;

    if (list_empty(&reader->elements))
        return;

    element = LIST_ENTRY(list_head(&reader->elements), struct element, entry);
    list_remove(&element->entry);

    reader_pop_ns_nodes(reader, element);
    reader_free_element(reader, element);

    /* It was a root element, the rest is expected as Misc */
    if (list_empty(&reader->elements))
        reader->instate = XmlReadInState_MiscEnd;
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
        v->start = 0;
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
            memcpy(v->str, reader_get_strptr(reader, value), value->len*sizeof(WCHAR));
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
    buffer->cur = 0;
    buffer->allocated = initial_len;
    buffer->written = 0;
    buffer->prev_cr = FALSE;

    return S_OK;
}

static void free_encoded_buffer(xmlreaderinput *input, encoded_buffer *buffer)
{
    readerinput_free(input, buffer->data);
}

HRESULT get_code_page(xml_encoding encoding, UINT *cp)
{
    if (encoding == XmlEncoding_Unknown)
    {
        FIXME("unsupported encoding %d\n", encoding);
        return E_NOTIMPL;
    }

    *cp = xml_encoding_map[encoding].cp;

    return S_OK;
}

xml_encoding parse_encoding_name(const WCHAR *name, int len)
{
    int min, max, n, c;

    if (!name) return XmlEncoding_Unknown;

    min = 0;
    max = ARRAY_SIZE(xml_encoding_map) - 1;

    while (min <= max)
    {
        n = (min+max)/2;

        if (len != -1)
            c = _wcsnicmp(xml_encoding_map[n].name, name, len);
        else
            c = wcsicmp(xml_encoding_map[n].name, name);
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
static inline HRESULT readerinput_query_for_stream(xmlreaderinput *readerinput)
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
    TRACE("written=%d, alloc=%d, requested=%d, read=%d, ret=0x%08x\n", buffer->written, buffer->allocated, len, read, hr);
    readerinput->pending = hr == E_PENDING;
    if (FAILED(hr)) return hr;
    buffer->written += read;

    return hr;
}

/* grows UTF-16 buffer so it has at least 'length' WCHAR chars free on return */
static void readerinput_grow(xmlreaderinput *readerinput, int length)
{
    encoded_buffer *buffer = &readerinput->buffer->utf16;

    length *= sizeof(WCHAR);
    /* grow if needed, plus 4 bytes to be sure null terminator will fit in */
    if (buffer->allocated < buffer->written + length + 4)
    {
        int grown_size = max(2*buffer->allocated, buffer->allocated + length);
        buffer->data = readerinput_realloc(readerinput, buffer->data, grown_size);
        buffer->allocated = grown_size;
    }
}

static inline BOOL readerinput_is_utf8(xmlreaderinput *readerinput)
{
    static const char startA[] = {'<','?'};
    static const char commentA[] = {'<','!'};
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
    static const char utf8bom[] = {0xef,0xbb,0xbf};
    static const char utf16lebom[] = {0xff,0xfe};
    WCHAR *ptrW;

    *enc = XmlEncoding_Unknown;

    if (buffer->written <= 3)
    {
        HRESULT hr = readerinput_growraw(readerinput);
        if (FAILED(hr)) return hr;
        if (buffer->written < 3) return MX_E_INPUTEND;
    }

    ptrW = (WCHAR *)buffer->data;
    /* try start symbols if we have enough data to do that, input buffer should contain
       first chunk already */
    if (readerinput_is_utf8(readerinput))
        *enc = XmlEncoding_UTF8;
    else if (*ptrW == '<')
    {
        ptrW++;
        if (*ptrW == '?' || *ptrW == '!' || is_namestartchar(*ptrW))
            *enc = XmlEncoding_UTF16;
    }
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

    TRACE("%d\n", len - buffer->cur);
    return len - buffer->cur;
}

/* It's possible that raw buffer has some leftovers from last conversion - some char
   sequence that doesn't represent a full code point. Length argument should be calculated with
   readerinput_get_convlen(), if it's -1 it will be calculated here. */
static void readerinput_shrinkraw(xmlreaderinput *readerinput, int len)
{
    encoded_buffer *buffer = &readerinput->buffer->encoded;

    if (len == -1)
        len = readerinput_get_convlen(readerinput);

    memmove(buffer->data, buffer->data + buffer->cur + (buffer->written - len), len);
    /* everything below cur is lost too */
    buffer->written -= len + buffer->cur;
    /* after this point we don't need cur offset really,
       it's used only to mark where actual data begins when first chunk is read */
    buffer->cur = 0;
}

static void fixup_buffer_cr(encoded_buffer *buffer, int off)
{
    BOOL prev_cr = buffer->prev_cr;
    const WCHAR *src;
    WCHAR *dest;

    src = dest = (WCHAR*)buffer->data + off;
    while ((const char*)src < buffer->data + buffer->written)
    {
        if (*src == '\r')
        {
            *dest++ = '\n';
            src++;
            prev_cr = TRUE;
            continue;
        }
        if(prev_cr && *src == '\n')
            src++;
        else
            *dest++ = *src++;
        prev_cr = FALSE;
    }

    buffer->written = (char*)dest - buffer->data;
    buffer->prev_cr = prev_cr;
    *dest = 0;
}

/* note that raw buffer content is kept */
static void readerinput_switchencoding(xmlreaderinput *readerinput, xml_encoding enc)
{
    encoded_buffer *src = &readerinput->buffer->encoded;
    encoded_buffer *dest = &readerinput->buffer->utf16;
    int len, dest_len;
    UINT cp = ~0u;
    HRESULT hr;
    WCHAR *ptr;

    hr = get_code_page(enc, &cp);
    if (FAILED(hr)) return;

    readerinput->buffer->code_page = cp;
    len = readerinput_get_convlen(readerinput);

    TRACE("switching to cp %d\n", cp);

    /* just copy in this case */
    if (enc == XmlEncoding_UTF16)
    {
        readerinput_grow(readerinput, len);
        memcpy(dest->data, src->data + src->cur, len);
        dest->written += len*sizeof(WCHAR);
    }
    else
    {
        dest_len = MultiByteToWideChar(cp, 0, src->data + src->cur, len, NULL, 0);
        readerinput_grow(readerinput, dest_len);
        ptr = (WCHAR*)dest->data;
        MultiByteToWideChar(cp, 0, src->data + src->cur, len, ptr, dest_len);
        ptr[dest_len] = 0;
        dest->written += dest_len*sizeof(WCHAR);
    }

    fixup_buffer_cr(dest, 0);
}

/* shrinks parsed data a buffer begins with */
static void reader_shrink(xmlreader *reader)
{
    encoded_buffer *buffer = &reader->input->buffer->utf16;

    /* avoid to move too often using threshold shrink length */
    if (buffer->cur*sizeof(WCHAR) > buffer->written / 2)
    {
        buffer->written -= buffer->cur*sizeof(WCHAR);
        memmove(buffer->data, (WCHAR*)buffer->data + buffer->cur, buffer->written);
        buffer->cur = 0;
        *(WCHAR*)&buffer->data[buffer->written] = 0;
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
    int len, dest_len, prev_len;
    HRESULT hr;
    WCHAR *ptr;

    /* get some raw data from stream first */
    hr = readerinput_growraw(readerinput);
    len = readerinput_get_convlen(readerinput);
    prev_len = dest->written / sizeof(WCHAR);

    /* just copy for UTF-16 case */
    if (cp == 1200)
    {
        readerinput_grow(readerinput, len);
        memcpy(dest->data + dest->written, src->data + src->cur, len);
        dest->written += len*sizeof(WCHAR);
    }
    else
    {
        dest_len = MultiByteToWideChar(cp, 0, src->data + src->cur, len, NULL, 0);
        readerinput_grow(readerinput, dest_len);
        ptr = (WCHAR*)(dest->data + dest->written);
        MultiByteToWideChar(cp, 0, src->data + src->cur, len, ptr, dest_len);
        ptr[dest_len] = 0;
        dest->written += dest_len*sizeof(WCHAR);
        /* get rid of processed data */
        readerinput_shrinkraw(readerinput, len);
    }

    fixup_buffer_cr(dest, prev_len);
    return hr;
}

static inline UINT reader_get_cur(xmlreader *reader)
{
    return reader->input->buffer->utf16.cur;
}

static inline WCHAR *reader_get_ptr(xmlreader *reader)
{
    encoded_buffer *buffer = &reader->input->buffer->utf16;
    WCHAR *ptr = (WCHAR*)buffer->data + buffer->cur;
    if (!*ptr) reader_more(reader);
    return (WCHAR*)buffer->data + buffer->cur;
}

static int reader_cmp(xmlreader *reader, const WCHAR *str)
{
    int i=0;
    const WCHAR *ptr = reader_get_ptr(reader);
    while (str[i])
    {
        if (!ptr[i])
        {
            reader_more(reader);
            ptr = reader_get_ptr(reader);
        }
        if (str[i] != ptr[i])
            return ptr[i] - str[i];
        i++;
    }
    return 0;
}

static void reader_update_position(xmlreader *reader, WCHAR ch)
{
    if (ch == '\r')
        reader->position.line_position = 1;
    else if (ch == '\n')
    {
        reader->position.line_number++;
        reader->position.line_position = 1;
    }
    else
        reader->position.line_position++;
}

/* moves cursor n WCHARs forward */
static void reader_skipn(xmlreader *reader, int n)
{
    encoded_buffer *buffer = &reader->input->buffer->utf16;
    const WCHAR *ptr;

    while (*(ptr = reader_get_ptr(reader)) && n--)
    {
        reader_update_position(reader, *ptr);
        buffer->cur++;
    }
}

static inline BOOL is_wchar_space(WCHAR ch)
{
    return ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n';
}

/* [3] S ::= (#x20 | #x9 | #xD | #xA)+ */
static int reader_skipspaces(xmlreader *reader)
{
    const WCHAR *ptr = reader_get_ptr(reader);
    UINT start = reader_get_cur(reader);

    while (is_wchar_space(*ptr))
    {
        reader_skipn(reader, 1);
        ptr = reader_get_ptr(reader);
    }

    return reader_get_cur(reader) - start;
}

/* [26] VersionNum ::= '1.' [0-9]+ */
static HRESULT reader_parse_versionnum(xmlreader *reader, strval *val)
{
    static const WCHAR onedotW[] = {'1','.',0};
    WCHAR *ptr, *ptr2;
    UINT start;

    if (reader_cmp(reader, onedotW)) return WC_E_XMLDECL;

    start = reader_get_cur(reader);
    /* skip "1." */
    reader_skipn(reader, 2);

    ptr2 = ptr = reader_get_ptr(reader);
    while (*ptr >= '0' && *ptr <= '9')
    {
        reader_skipn(reader, 1);
        ptr = reader_get_ptr(reader);
    }

    if (ptr2 == ptr) return WC_E_DIGIT;
    reader_init_strvalue(start, reader_get_cur(reader)-start, val);
    TRACE("version=%s\n", debug_strval(reader, val));
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
    struct reader_position position;
    strval val, name;
    HRESULT hr;

    if (!reader_skipspaces(reader)) return WC_E_WHITESPACE;

    position = reader->position;
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

    return reader_add_attr(reader, NULL, &name, NULL, &val, &position, 0);
}

/* ([A-Za-z0-9._] | '-') */
static inline BOOL is_wchar_encname(WCHAR ch)
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
    WCHAR *start = reader_get_ptr(reader), *ptr;
    xml_encoding enc;
    int len;

    if ((*start < 'A' || *start > 'Z') && (*start < 'a' || *start > 'z'))
        return WC_E_ENCNAME;

    val->start = reader_get_cur(reader);

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
    struct reader_position position;
    strval name, val;
    HRESULT hr;

    if (!reader_skipspaces(reader)) return S_FALSE;

    position = reader->position;
    if (reader_cmp(reader, encodingW)) return S_FALSE;
    name.str = reader_get_ptr(reader);
    name.start = reader_get_cur(reader);
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

    return reader_add_attr(reader, NULL, &name, NULL, &val, &position, 0);
}

/* [32] SDDecl ::= S 'standalone' Eq (("'" ('yes' | 'no') "'") | ('"' ('yes' | 'no') '"')) */
static HRESULT reader_parse_sddecl(xmlreader *reader)
{
    static const WCHAR standaloneW[] = {'s','t','a','n','d','a','l','o','n','e',0};
    static const WCHAR yesW[] = {'y','e','s',0};
    static const WCHAR noW[] = {'n','o',0};
    struct reader_position position;
    strval name, val;
    UINT start;
    HRESULT hr;

    if (!reader_skipspaces(reader)) return S_FALSE;

    position = reader->position;
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
    reader_init_strvalue(start, reader_get_cur(reader)-start, &val);
    TRACE("standalone=%s\n", debug_strval(reader, &val));

    if (reader_cmp(reader, quoteW) && reader_cmp(reader, dblquoteW))
        return WC_E_QUOTE;
    /* skip "'"|'"' */
    reader_skipn(reader, 1);

    return reader_add_attr(reader, NULL, &name, NULL, &val, &position, 0);
}

/* [23] XMLDecl ::= '<?xml' VersionInfo EncodingDecl? SDDecl? S? '?>' */
static HRESULT reader_parse_xmldecl(xmlreader *reader)
{
    static const WCHAR xmldeclW[] = {'<','?','x','m','l',' ',0};
    static const WCHAR declcloseW[] = {'?','>',0};
    struct reader_position position;
    HRESULT hr;

    /* check if we have "<?xml " */
    if (reader_cmp(reader, xmldeclW))
        return S_FALSE;

    reader_skipn(reader, 2);
    position = reader->position;
    reader_skipn(reader, 3);
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
    if (reader_cmp(reader, declcloseW))
        return WC_E_XMLDECL;

    /* skip '?>' */
    reader_skipn(reader, 2);

    reader->nodetype = XmlNodeType_XmlDeclaration;
    reader->empty_element.position = position;
    reader_set_strvalue(reader, StringValue_LocalName, &strval_xml);
    reader_set_strvalue(reader, StringValue_QualifiedName, &strval_xml);

    return S_OK;
}

/* [15] Comment ::= '<!--' ((Char - '-') | ('-' (Char - '-')))* '-->' */
static HRESULT reader_parse_comment(xmlreader *reader)
{
    WCHAR *ptr;
    UINT start;

    if (reader->resumestate == XmlReadResumeState_Comment)
    {
        start = reader->resume[XmlReadResume_Body];
        ptr = reader_get_ptr(reader);
    }
    else
    {
        /* skip '<!--' */
        reader_skipn(reader, 4);
        reader_shrink(reader);
        ptr = reader_get_ptr(reader);
        start = reader_get_cur(reader);
        reader->nodetype = XmlNodeType_Comment;
        reader->resume[XmlReadResume_Body] = start;
        reader->resumestate = XmlReadResumeState_Comment;
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

                    reader_init_strvalue(start, reader_get_cur(reader)-start, &value);
                    TRACE("%s\n", debug_strval(reader, &value));

                    /* skip rest of markup '->' */
                    reader_skipn(reader, 3);

                    reader_set_strvalue(reader, StringValue_Value, &value);
                    reader->resume[XmlReadResume_Body] = 0;
                    reader->resumestate = XmlReadResumeState_Initial;
                    return S_OK;
                }
                else
                    return WC_E_COMMENT;
            }
        }

        reader_skipn(reader, 1);
        ptr++;
    }

    return S_OK;
}

/* [2] Char ::= #x9 | #xA | #xD | [#x20-#xD7FF] | [#xE000-#xFFFD] | [#x10000-#x10FFFF] */
static inline BOOL is_char(WCHAR ch)
{
    return (ch == '\t') || (ch == '\r') || (ch == '\n') ||
           (ch >= 0x20 && ch <= 0xd7ff) ||
           (ch >= 0xd800 && ch <= 0xdbff) || /* high surrogate */
           (ch >= 0xdc00 && ch <= 0xdfff) || /* low surrogate */
           (ch >= 0xe000 && ch <= 0xfffd);
}

/* [13] PubidChar ::= #x20 | #xD | #xA | [a-zA-Z0-9] | [-'()+,./:=?;!*#@$_%] */
BOOL is_pubchar(WCHAR ch)
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

BOOL is_namestartchar(WCHAR ch)
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
BOOL is_ncnamechar(WCHAR ch)
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

BOOL is_namechar(WCHAR ch)
{
    return (ch == ':') || is_ncnamechar(ch);
}

static XmlNodeType reader_get_nodetype(const xmlreader *reader)
{
    /* When we're on attribute always return attribute type, container node type is kept.
       Note that container is not necessarily an element, and attribute doesn't mean it's
       an attribute in XML spec terms. */
    return reader->attr ? XmlNodeType_Attribute : reader->nodetype;
}

/* [4] NameStartChar ::= ":" | [A-Z] | "_" | [a-z] | [#xC0-#xD6] | [#xD8-#xF6] | [#xF8-#x2FF] | [#x370-#x37D] |
                            [#x37F-#x1FFF] | [#x200C-#x200D] | [#x2070-#x218F] | [#x2C00-#x2FEF] | [#x3001-#xD7FF] |
                            [#xF900-#xFDCF] | [#xFDF0-#xFFFD] | [#x10000-#xEFFFF]
   [4a] NameChar ::= NameStartChar | "-" | "." | [0-9] | #xB7 | [#x0300-#x036F] | [#x203F-#x2040]
   [5]  Name     ::= NameStartChar (NameChar)* */
static HRESULT reader_parse_name(xmlreader *reader, strval *name)
{
    WCHAR *ptr;
    UINT start;

    if (reader->resume[XmlReadResume_Name])
    {
        start = reader->resume[XmlReadResume_Name];
        ptr = reader_get_ptr(reader);
    }
    else
    {
        ptr = reader_get_ptr(reader);
        start = reader_get_cur(reader);
        if (!is_namestartchar(*ptr)) return WC_E_NAMECHARACTER;
    }

    while (is_namechar(*ptr))
    {
        reader_skipn(reader, 1);
        ptr = reader_get_ptr(reader);
    }

    if (is_reader_pending(reader))
    {
        reader->resume[XmlReadResume_Name] = start;
        return E_PENDING;
    }
    else
        reader->resume[XmlReadResume_Name] = 0;

    reader_init_strvalue(start, reader_get_cur(reader)-start, name);
    TRACE("name %s:%d\n", debug_strval(reader, name), name->len);

    return S_OK;
}

/* [17] PITarget ::= Name - (('X' | 'x') ('M' | 'm') ('L' | 'l')) */
static HRESULT reader_parse_pitarget(xmlreader *reader, strval *target)
{
    static const WCHAR xmlW[] = {'x','m','l'};
    static const strval xmlval = { (WCHAR*)xmlW, 3 };
    strval name;
    WCHAR *ptr;
    HRESULT hr;
    UINT i;

    hr = reader_parse_name(reader, &name);
    if (FAILED(hr)) return is_reader_pending(reader) ? E_PENDING : WC_E_PI;

    /* now that we got name check for illegal content */
    if (strval_eq(reader, &name, &xmlval))
        return WC_E_LEADINGXML;

    /* PITarget can't be a qualified name */
    ptr = reader_get_strptr(reader, &name);
    for (i = 0; i < name.len; i++)
        if (ptr[i] == ':')
            return i ? NC_E_NAMECOLON : WC_E_PI;

    TRACE("pitarget %s:%d\n", debug_strval(reader, &name), name.len);
    *target = name;
    return S_OK;
}

/* [16] PI ::= '<?' PITarget (S (Char* - (Char* '?>' Char*)))? '?>' */
static HRESULT reader_parse_pi(xmlreader *reader)
{
    strval target;
    WCHAR *ptr;
    UINT start;
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
        reader_set_strvalue(reader, StringValue_LocalName, &target);
        reader_set_strvalue(reader, StringValue_QualifiedName, &target);
        reader_set_strvalue(reader, StringValue_Value, &strval_empty);
        reader->resumestate = XmlReadResumeState_PIBody;
        reader->resume[XmlReadResume_Body] = reader_get_cur(reader);
    default:
        ;
    }

    start = reader->resume[XmlReadResume_Body];
    ptr = reader_get_ptr(reader);
    while (*ptr)
    {
        if (ptr[0] == '?')
        {
            if (ptr[1] == '>')
            {
                UINT cur = reader_get_cur(reader);
                strval value;

                /* strip all leading whitespace chars */
                while (start < cur)
                {
                    ptr = reader_get_ptr2(reader, start);
                    if (!is_wchar_space(*ptr)) break;
                    start++;
                }

                reader_init_strvalue(start, cur-start, &value);

                /* skip '?>' */
                reader_skipn(reader, 2);
                TRACE("%s\n", debug_strval(reader, &value));
                reader->nodetype = XmlNodeType_ProcessingInstruction;
                reader->resumestate = XmlReadResumeState_Initial;
                reader->resume[XmlReadResume_Body] = 0;
                reader_set_strvalue(reader, StringValue_Value, &value);
                return S_OK;
            }
        }

        reader_skipn(reader, 1);
        ptr = reader_get_ptr(reader);
    }

    return S_OK;
}

/* This one is used to parse significant whitespace nodes, like in Misc production */
static HRESULT reader_parse_whitespace(xmlreader *reader)
{
    switch (reader->resumestate)
    {
    case XmlReadResumeState_Initial:
        reader_shrink(reader);
        reader->resumestate = XmlReadResumeState_Whitespace;
        reader->resume[XmlReadResume_Body] = reader_get_cur(reader);
        reader->nodetype = XmlNodeType_Whitespace;
        reader_set_strvalue(reader, StringValue_LocalName, &strval_empty);
        reader_set_strvalue(reader, StringValue_QualifiedName, &strval_empty);
        reader_set_strvalue(reader, StringValue_Value, &strval_empty);
        /* fallthrough */
    case XmlReadResumeState_Whitespace:
    {
        strval value;
        UINT start;

        reader_skipspaces(reader);
        if (is_reader_pending(reader)) return S_OK;

        start = reader->resume[XmlReadResume_Body];
        reader_init_strvalue(start, reader_get_cur(reader)-start, &value);
        reader_set_strvalue(reader, StringValue_Value, &value);
        TRACE("%s\n", debug_strval(reader, &value));
        reader->resumestate = XmlReadResumeState_Initial;
    }
    default:
        ;
    }

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
        case XmlReadResumeState_Whitespace:
            return reader_parse_whitespace(reader);
        default:
            ERR("unknown resume state %d\n", reader->resumestate);
        }
    }

    while (1)
    {
        const WCHAR *cur = reader_get_ptr(reader);

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
    WCHAR *cur = reader_get_ptr(reader), quote;
    UINT start;

    if (*cur != '"' && *cur != '\'') return WC_E_QUOTE;

    quote = *cur;
    reader_skipn(reader, 1);

    cur = reader_get_ptr(reader);
    start = reader_get_cur(reader);
    while (is_char(*cur) && *cur != quote)
    {
        reader_skipn(reader, 1);
        cur = reader_get_ptr(reader);
    }
    reader_init_strvalue(start, reader_get_cur(reader)-start, literal);
    if (*cur == quote) reader_skipn(reader, 1);

    TRACE("%s\n", debug_strval(reader, literal));
    return S_OK;
}

/* [12] PubidLiteral ::= '"' PubidChar* '"' | "'" (PubidChar - "'")* "'"
   [13] PubidChar ::= #x20 | #xD | #xA | [a-zA-Z0-9] | [-'()+,./:=?;!*#@$_%] */
static HRESULT reader_parse_pub_literal(xmlreader *reader, strval *literal)
{
    WCHAR *cur = reader_get_ptr(reader), quote;
    UINT start;

    if (*cur != '"' && *cur != '\'') return WC_E_QUOTE;

    quote = *cur;
    reader_skipn(reader, 1);

    start = reader_get_cur(reader);
    cur = reader_get_ptr(reader);
    while (is_pubchar(*cur) && *cur != quote)
    {
        reader_skipn(reader, 1);
        cur = reader_get_ptr(reader);
    }
    reader_init_strvalue(start, reader_get_cur(reader)-start, literal);
    if (*cur == quote) reader_skipn(reader, 1);

    TRACE("%s\n", debug_strval(reader, literal));
    return S_OK;
}

/* [75] ExternalID ::= 'SYSTEM' S SystemLiteral | 'PUBLIC' S PubidLiteral S SystemLiteral */
static HRESULT reader_parse_externalid(xmlreader *reader)
{
    static WCHAR systemW[] = {'S','Y','S','T','E','M',0};
    static WCHAR publicW[] = {'P','U','B','L','I','C',0};
    struct reader_position position = reader->position;
    strval name, sys;
    HRESULT hr;
    int cnt;

    if (!reader_cmp(reader, publicW)) {
        strval pub;

        /* public id */
        reader_skipn(reader, 6);
        cnt = reader_skipspaces(reader);
        if (!cnt) return WC_E_WHITESPACE;

        hr = reader_parse_pub_literal(reader, &pub);
        if (FAILED(hr)) return hr;

        reader_init_cstrvalue(publicW, lstrlenW(publicW), &name);
        hr = reader_add_attr(reader, NULL, &name, NULL, &pub, &position, 0);
        if (FAILED(hr)) return hr;

        cnt = reader_skipspaces(reader);
        if (!cnt) return S_OK;

        /* optional system id */
        hr = reader_parse_sys_literal(reader, &sys);
        if (FAILED(hr)) return S_OK;

        reader_init_cstrvalue(systemW, lstrlenW(systemW), &name);
        hr = reader_add_attr(reader, NULL, &name, NULL, &sys, &position, 0);
        if (FAILED(hr)) return hr;

        return S_OK;
    } else if (!reader_cmp(reader, systemW)) {
        /* system id */
        reader_skipn(reader, 6);
        cnt = reader_skipspaces(reader);
        if (!cnt) return WC_E_WHITESPACE;

        hr = reader_parse_sys_literal(reader, &sys);
        if (FAILED(hr)) return hr;

        reader_init_cstrvalue(systemW, lstrlenW(systemW), &name);
        return reader_add_attr(reader, NULL, &name, NULL, &sys, &position, 0);
    }

    return S_FALSE;
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

    cur = reader_get_ptr(reader);
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
static HRESULT reader_parse_local(xmlreader *reader, strval *local, BOOL check_for_separator)
{
    WCHAR *ptr;
    UINT start;

    if (reader->resume[XmlReadResume_Local])
    {
        start = reader->resume[XmlReadResume_Local];
        ptr = reader_get_ptr(reader);
    }
    else
    {
        ptr = reader_get_ptr(reader);
        start = reader_get_cur(reader);
    }

    while (is_ncnamechar(*ptr))
    {
        reader_skipn(reader, 1);
        ptr = reader_get_ptr(reader);
    }

    if (check_for_separator && *ptr == ':')
        return NC_E_QNAMECOLON;

    if (is_reader_pending(reader))
    {
         reader->resume[XmlReadResume_Local] = start;
         return E_PENDING;
    }
    else
         reader->resume[XmlReadResume_Local] = 0;

    reader_init_strvalue(start, reader_get_cur(reader)-start, local);

    return S_OK;
}

/* [7 NS]  QName ::= PrefixedName | UnprefixedName
   [8 NS]  PrefixedName ::= Prefix ':' LocalPart
   [9 NS]  UnprefixedName ::= LocalPart
   [10 NS] Prefix ::= NCName */
static HRESULT reader_parse_qname(xmlreader *reader, strval *prefix, strval *local, strval *qname)
{
    WCHAR *ptr;
    UINT start;
    HRESULT hr;

    if (reader->resume[XmlReadResume_Name])
    {
        start = reader->resume[XmlReadResume_Name];
        ptr = reader_get_ptr(reader);
    }
    else
    {
        ptr = reader_get_ptr(reader);
        start = reader_get_cur(reader);
        reader->resume[XmlReadResume_Name] = start;
        if (!is_ncnamechar(*ptr)) return NC_E_QNAMECHARACTER;
    }

    if (reader->resume[XmlReadResume_Local])
    {
        hr = reader_parse_local(reader, local, FALSE);
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
            ptr = reader_get_ptr(reader);
        }

        if (is_reader_pending(reader)) return E_PENDING;

        /* got a qualified name */
        if (*ptr == ':')
        {
            reader_init_strvalue(start, reader_get_cur(reader)-start, prefix);

            /* skip ':' */
            reader_skipn(reader, 1);
            hr = reader_parse_local(reader, local, TRUE);
            if (FAILED(hr)) return hr;
        }
        else
        {
            reader_init_strvalue(reader->resume[XmlReadResume_Name], reader_get_cur(reader)-reader->resume[XmlReadResume_Name], local);
            reader_init_strvalue(0, 0, prefix);
        }
    }

    if (prefix->len)
        TRACE("qname %s:%s\n", debug_strval(reader, prefix), debug_strval(reader, local));
    else
        TRACE("ncname %s\n", debug_strval(reader, local));

    reader_init_strvalue(prefix->len ? prefix->start : local->start,
                        /* count ':' too */
                        (prefix->len ? prefix->len + 1 : 0) + local->len,
                         qname);

    reader->resume[XmlReadResume_Name] = 0;
    reader->resume[XmlReadResume_Local] = 0;

    return S_OK;
}

static WCHAR get_predefined_entity(const xmlreader *reader, const strval *name)
{
    static const WCHAR entltW[]   = {'l','t'};
    static const WCHAR entgtW[]   = {'g','t'};
    static const WCHAR entampW[]  = {'a','m','p'};
    static const WCHAR entaposW[] = {'a','p','o','s'};
    static const WCHAR entquotW[] = {'q','u','o','t'};
    static const strval lt   = { (WCHAR*)entltW,   2 };
    static const strval gt   = { (WCHAR*)entgtW,   2 };
    static const strval amp  = { (WCHAR*)entampW,  3 };
    static const strval apos = { (WCHAR*)entaposW, 4 };
    static const strval quot = { (WCHAR*)entquotW, 4 };
    WCHAR *str = reader_get_strptr(reader, name);

    switch (*str)
    {
    case 'l':
        if (strval_eq(reader, name, &lt)) return '<';
        break;
    case 'g':
        if (strval_eq(reader, name, &gt)) return '>';
        break;
    case 'a':
        if (strval_eq(reader, name, &amp))
            return '&';
        else if (strval_eq(reader, name, &apos))
            return '\'';
        break;
    case 'q':
        if (strval_eq(reader, name, &quot)) return '\"';
        break;
    default:
        ;
    }

    return 0;
}

/* [66] CharRef ::= '&#' [0-9]+ ';' | '&#x' [0-9a-fA-F]+ ';'
   [67] Reference ::= EntityRef | CharRef
   [68] EntityRef ::= '&' Name ';' */
static HRESULT reader_parse_reference(xmlreader *reader)
{
    encoded_buffer *buffer = &reader->input->buffer->utf16;
    WCHAR *start = reader_get_ptr(reader), *ptr;
    UINT cur = reader_get_cur(reader);
    WCHAR ch = 0;
    int len;

    /* skip '&' */
    reader_skipn(reader, 1);
    ptr = reader_get_ptr(reader);

    if (*ptr == '#')
    {
        reader_skipn(reader, 1);
        ptr = reader_get_ptr(reader);

        /* hex char or decimal */
        if (*ptr == 'x')
        {
            reader_skipn(reader, 1);
            ptr = reader_get_ptr(reader);

            while (*ptr != ';')
            {
                if ((*ptr >= '0' && *ptr <= '9'))
                    ch = ch*16 + *ptr - '0';
                else if ((*ptr >= 'a' && *ptr <= 'f'))
                    ch = ch*16 + *ptr - 'a' + 10;
                else if ((*ptr >= 'A' && *ptr <= 'F'))
                    ch = ch*16 + *ptr - 'A' + 10;
                else
                    return ch ? WC_E_SEMICOLON : WC_E_HEXDIGIT;
                reader_skipn(reader, 1);
                ptr = reader_get_ptr(reader);
            }
        }
        else
        {
            while (*ptr != ';')
            {
                if ((*ptr >= '0' && *ptr <= '9'))
                {
                    ch = ch*10 + *ptr - '0';
                    reader_skipn(reader, 1);
                    ptr = reader_get_ptr(reader);
                }
                else
                    return ch ? WC_E_SEMICOLON : WC_E_DIGIT;
            }
        }

        if (!is_char(ch)) return WC_E_XMLCHARACTER;

        /* normalize */
        if (is_wchar_space(ch)) ch = ' ';

        ptr = reader_get_ptr(reader);
        start = reader_get_ptr2(reader, cur);
        len = buffer->written - ((char *)ptr - buffer->data);
        memmove(start + 1, ptr + 1, len);

        buffer->written -= (reader_get_cur(reader) - cur) * sizeof(WCHAR);
        buffer->cur = cur + 1;

        *start = ch;
    }
    else
    {
        strval name;
        HRESULT hr;

        hr = reader_parse_name(reader, &name);
        if (FAILED(hr)) return hr;

        ptr = reader_get_ptr(reader);
        if (*ptr != ';') return WC_E_SEMICOLON;

        /* predefined entities resolve to a single character */
        ch = get_predefined_entity(reader, &name);
        if (ch)
        {
            len = buffer->written - ((char*)ptr - buffer->data) - sizeof(WCHAR);
            memmove(start+1, ptr+1, len);
            buffer->cur = cur + 1;
            buffer->written -= (ptr - start) * sizeof(WCHAR);

            *start = ch;
        }
        else
        {
            FIXME("undeclared entity %s\n", debug_strval(reader, &name));
            return WC_E_UNDECLAREDENTITY;
        }

    }

    return S_OK;
}

/* [10 NS] AttValue ::= '"' ([^<&"] | Reference)* '"' | "'" ([^<&'] | Reference)* "'" */
static HRESULT reader_parse_attvalue(xmlreader *reader, strval *value)
{
    WCHAR *ptr, quote;
    UINT start;

    ptr = reader_get_ptr(reader);

    /* skip opening quote */
    quote = *ptr;
    if (quote != '\"' && quote != '\'') return WC_E_QUOTE;
    reader_skipn(reader, 1);

    ptr = reader_get_ptr(reader);
    start = reader_get_cur(reader);
    while (*ptr)
    {
        if (*ptr == '<') return WC_E_LESSTHAN;

        if (*ptr == quote)
        {
            reader_init_strvalue(start, reader_get_cur(reader)-start, value);
            /* skip closing quote */
            reader_skipn(reader, 1);
            return S_OK;
        }

        if (*ptr == '&')
        {
            HRESULT hr = reader_parse_reference(reader);
            if (FAILED(hr)) return hr;
        }
        else
        {
            /* replace all whitespace chars with ' ' */
            if (is_wchar_space(*ptr)) *ptr = ' ';
            reader_skipn(reader, 1);
        }
        ptr = reader_get_ptr(reader);
    }

    return WC_E_QUOTE;
}

/* [1  NS] NSAttName ::= PrefixedAttName | DefaultAttName
   [2  NS] PrefixedAttName ::= 'xmlns:' NCName
   [3  NS] DefaultAttName  ::= 'xmlns'
   [15 NS] Attribute ::= NSAttName Eq AttValue | QName Eq AttValue */
static HRESULT reader_parse_attribute(xmlreader *reader)
{
    struct reader_position position = reader->position;
    strval prefix, local, qname, value;
    enum attribute_flags flags = 0;
    HRESULT hr;

    hr = reader_parse_qname(reader, &prefix, &local, &qname);
    if (FAILED(hr)) return hr;

    if (strval_eq(reader, &prefix, &strval_xmlns))
        flags |= ATTRIBUTE_NS_DEFINITION;

    if (strval_eq(reader, &qname, &strval_xmlns))
        flags |= ATTRIBUTE_DEFAULT_NS_DEFINITION;

    hr = reader_parse_eq(reader);
    if (FAILED(hr)) return hr;

    hr = reader_parse_attvalue(reader, &value);
    if (FAILED(hr)) return hr;

    if (flags & (ATTRIBUTE_NS_DEFINITION | ATTRIBUTE_DEFAULT_NS_DEFINITION))
        reader_push_ns(reader, &local, &value, !!(flags & ATTRIBUTE_DEFAULT_NS_DEFINITION));

    TRACE("%s=%s\n", debug_strval(reader, &local), debug_strval(reader, &value));
    return reader_add_attr(reader, &prefix, &local, &qname, &value, &position, flags);
}

/* [12 NS] STag ::= '<' QName (S Attribute)* S? '>'
   [14 NS] EmptyElemTag ::= '<' QName (S Attribute)* S? '/>' */
static HRESULT reader_parse_stag(xmlreader *reader, strval *prefix, strval *local, strval *qname)
{
    struct reader_position position = reader->position;
    HRESULT hr;

    hr = reader_parse_qname(reader, prefix, local, qname);
    if (FAILED(hr)) return hr;

    for (;;)
    {
        static const WCHAR endW[] = {'/','>',0};

        reader_skipspaces(reader);

        /* empty element */
        if ((reader->is_empty_element = !reader_cmp(reader, endW)))
        {
            struct element *element = &reader->empty_element;

            /* skip '/>' */
            reader_skipn(reader, 2);

            reader_free_strvalued(reader, &element->qname);
            reader_free_strvalued(reader, &element->localname);

            element->prefix = *prefix;
            reader_strvaldup(reader, qname, &element->qname);
            reader_strvaldup(reader, local, &element->localname);
            element->position = position;
            reader_mark_ns_nodes(reader, element);
            return S_OK;
        }

        /* got a start tag */
        if (!reader_cmp(reader, gtW))
        {
            /* skip '>' */
            reader_skipn(reader, 1);
            return reader_push_element(reader, prefix, local, qname, &position);
        }

        hr = reader_parse_attribute(reader);
        if (FAILED(hr)) return hr;
    }

    return S_OK;
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

        /* this handles empty elements too */
        hr = reader_parse_stag(reader, &prefix, &local, &qname);
        if (FAILED(hr)) return hr;

        /* FIXME: need to check for defined namespace to reject invalid prefix */

        /* if we got empty element and stack is empty go straight to Misc */
        if (reader->is_empty_element && list_empty(&reader->elements))
            reader->instate = XmlReadInState_MiscEnd;
        else
            reader->instate = XmlReadInState_Content;

        reader->nodetype = XmlNodeType_Element;
        reader->resumestate = XmlReadResumeState_Initial;
        reader_set_strvalue(reader, StringValue_Prefix, &prefix);
        reader_set_strvalue(reader, StringValue_QualifiedName, &qname);
        reader_set_strvalue(reader, StringValue_Value, &strval_empty);
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
    struct reader_position position;
    strval prefix, local, qname;
    struct element *element;
    HRESULT hr;

    /* skip '</' */
    reader_skipn(reader, 2);

    position = reader->position;
    hr = reader_parse_qname(reader, &prefix, &local, &qname);
    if (FAILED(hr)) return hr;

    reader_skipspaces(reader);

    if (reader_cmp(reader, gtW)) return WC_E_GREATERTHAN;

    /* skip '>' */
    reader_skipn(reader, 1);

    /* Element stack should never be empty at this point, cause we shouldn't get to
       content parsing if it's empty. */
    element = LIST_ENTRY(list_head(&reader->elements), struct element, entry);
    if (!strval_eq(reader, &element->qname, &qname)) return WC_E_ELEMENTMATCH;

    /* update position stored for start tag, we won't be using it */
    element->position = position;

    reader->nodetype = XmlNodeType_EndElement;
    reader->is_empty_element = FALSE;
    reader_set_strvalue(reader, StringValue_Prefix, &prefix);

    return S_OK;
}

/* [18] CDSect ::= CDStart CData CDEnd
   [19] CDStart ::= '<![CDATA['
   [20] CData ::= (Char* - (Char* ']]>' Char*))
   [21] CDEnd ::= ']]>' */
static HRESULT reader_parse_cdata(xmlreader *reader)
{
    WCHAR *ptr;
    UINT start;

    if (reader->resumestate == XmlReadResumeState_CDATA)
    {
        start = reader->resume[XmlReadResume_Body];
        ptr = reader_get_ptr(reader);
    }
    else
    {
        /* skip markup '<![CDATA[' */
        reader_skipn(reader, 9);
        reader_shrink(reader);
        ptr = reader_get_ptr(reader);
        start = reader_get_cur(reader);
        reader->nodetype = XmlNodeType_CDATA;
        reader->resume[XmlReadResume_Body] = start;
        reader->resumestate = XmlReadResumeState_CDATA;
        reader_set_strvalue(reader, StringValue_Value, NULL);
    }

    while (*ptr)
    {
        if (*ptr == ']' && *(ptr+1) == ']' && *(ptr+2) == '>')
        {
            strval value;

            reader_init_strvalue(start, reader_get_cur(reader)-start, &value);

            /* skip ']]>' */
            reader_skipn(reader, 3);
            TRACE("%s\n", debug_strval(reader, &value));

            reader_set_strvalue(reader, StringValue_Value, &value);
            reader->resume[XmlReadResume_Body] = 0;
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

/* [14] CharData ::= [^<&]* - ([^<&]* ']]>' [^<&]*) */
static HRESULT reader_parse_chardata(xmlreader *reader)
{
    struct reader_position position;
    WCHAR *ptr;
    UINT start;

    if (reader->resumestate == XmlReadResumeState_CharData)
    {
        start = reader->resume[XmlReadResume_Body];
        ptr = reader_get_ptr(reader);
    }
    else
    {
        reader_shrink(reader);
        ptr = reader_get_ptr(reader);
        start = reader_get_cur(reader);
        /* There's no text */
        if (!*ptr || *ptr == '<') return S_OK;
        reader->nodetype = is_wchar_space(*ptr) ? XmlNodeType_Whitespace : XmlNodeType_Text;
        reader->resume[XmlReadResume_Body] = start;
        reader->resumestate = XmlReadResumeState_CharData;
        reader_set_strvalue(reader, StringValue_Value, NULL);
    }

    position = reader->position;
    while (*ptr)
    {
        static const WCHAR ampW[] = {'&',0};

        /* CDATA closing sequence ']]>' is not allowed */
        if (ptr[0] == ']' && ptr[1] == ']' && ptr[2] == '>')
            return WC_E_CDSECTEND;

        /* Found next markup part */
        if (ptr[0] == '<')
        {
            strval value;

            reader->empty_element.position = position;
            reader_init_strvalue(start, reader_get_cur(reader)-start, &value);
            reader_set_strvalue(reader, StringValue_Value, &value);
            reader->resume[XmlReadResume_Body] = 0;
            reader->resumestate = XmlReadResumeState_Initial;
            return S_OK;
        }

        /* this covers a case when text has leading whitespace chars */
        if (!is_wchar_space(*ptr)) reader->nodetype = XmlNodeType_Text;

        if (!reader_cmp(reader, ampW))
            reader_parse_reference(reader);
        else
            reader_skipn(reader, 1);

        ptr = reader_get_ptr(reader);
    }

    return S_OK;
}

/* [43] content ::= CharData? ((element | Reference | CDSect | PI | Comment) CharData?)* */
static HRESULT reader_parse_content(xmlreader *reader)
{
    static const WCHAR cdstartW[] = {'<','!','[','C','D','A','T','A','[',0};
    static const WCHAR etagW[] = {'<','/',0};

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
        case XmlReadResumeState_CharData:
            return reader_parse_chardata(reader);
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

    if (!reader_cmp(reader, ltW))
        return reader_parse_element(reader);

    /* what's left must be CharData */
    return reader_parse_chardata(reader);
}

static HRESULT reader_parse_nextnode(xmlreader *reader)
{
    XmlNodeType nodetype = reader_get_nodetype(reader);
    HRESULT hr;

    if (!is_reader_pending(reader))
    {
        reader->chunk_read_off = 0;
        reader_clear_attrs(reader);
    }

    /* When moving from EndElement or empty element, pop its own namespace definitions */
    switch (nodetype)
    {
    case XmlNodeType_Attribute:
        reader_dec_depth(reader);
        /* fallthrough */
    case XmlNodeType_Element:
        if (reader->is_empty_element)
            reader_pop_ns_nodes(reader, &reader->empty_element);
        else if (FAILED(hr = reader_inc_depth(reader)))
            return hr;
        break;
    case XmlNodeType_EndElement:
        reader_pop_element(reader);
        reader_dec_depth(reader);
        break;
    default:
        ;
    }

    for (;;)
    {
        switch (reader->instate)
        {
        /* if it's a first call for a new input we need to detect stream encoding */
        case XmlReadInState_Initial:
            {
                xml_encoding enc;

                hr = readerinput_growraw(reader->input);
                if (FAILED(hr)) return hr;

                reader->position.line_number = 1;
                reader->position.line_position = 1;

                /* try to detect encoding by BOM or data and set input code page */
                hr = readerinput_detectencoding(reader->input, &enc);
                TRACE("detected encoding %s, 0x%08x\n", enc == XmlEncoding_Unknown ? "(unknown)" :
                        debugstr_w(xml_encoding_map[enc].name), hr);
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
        case XmlReadInState_MiscEnd:
            hr = reader_parse_misc(reader);
            if (hr != S_FALSE) return hr;

            if (*reader_get_ptr(reader))
            {
                WARN("found garbage in the end of XML\n");
                return WC_E_SYNTAX;
            }

            reader->instate = XmlReadInState_Eof;
            reader->state = XmlReadState_EndOfFile;
            reader->nodetype = XmlNodeType_None;
            return hr;
        case XmlReadInState_Eof:
            return S_FALSE;
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

static void reader_clear_ns(xmlreader *reader)
{
    struct ns *ns, *ns2;

    LIST_FOR_EACH_ENTRY_SAFE(ns, ns2, &reader->ns, struct ns, entry) {
        list_remove(&ns->entry);
        reader_free_strvalued(reader, &ns->prefix);
        reader_free_strvalued(reader, &ns->uri);
        reader_free(reader, ns);
    }

    LIST_FOR_EACH_ENTRY_SAFE(ns, ns2, &reader->nsdef, struct ns, entry) {
        list_remove(&ns->entry);
        reader_free_strvalued(reader, &ns->uri);
        reader_free(reader, ns);
    }
}

static void reader_reset_parser(xmlreader *reader)
{
    reader->position.line_number = 0;
    reader->position.line_position = 0;

    reader_clear_elements(reader);
    reader_clear_attrs(reader);
    reader_clear_ns(reader);
    reader_free_strvalues(reader);

    reader->depth = 0;
    reader->nodetype = XmlNodeType_None;
    reader->resumestate = XmlReadResumeState_Initial;
    memset(reader->resume, 0, sizeof(reader->resume));
    reader->is_empty_element = FALSE;
}

static ULONG WINAPI xmlreader_Release(IXmlReader *iface)
{
    xmlreader *This = impl_from_IXmlReader(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(%d)\n", This, ref);

    if (ref == 0)
    {
        IMalloc *imalloc = This->imalloc;
        reader_reset_parser(This);
        if (This->input) IUnknown_Release(&This->input->IXmlReaderInput_iface);
        if (This->resolver) IXmlResolver_Release(This->resolver);
        if (This->mlang) IUnknown_Release(This->mlang);
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

    reader_reset_parser(This);

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
                                         This->imalloc, NULL, FALSE, NULL, &readerinput);
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

    TRACE("(%p)->(%s %p)\n", This, debugstr_reader_prop(property), value);

    if (!value) return E_INVALIDARG;

    switch (property)
    {
        case XmlReaderProperty_MultiLanguage:
            *value = (LONG_PTR)This->mlang;
            if (This->mlang)
                IUnknown_AddRef(This->mlang);
            break;
        case XmlReaderProperty_XmlResolver:
            *value = (LONG_PTR)This->resolver;
            if (This->resolver)
                IXmlResolver_AddRef(This->resolver);
            break;
        case XmlReaderProperty_DtdProcessing:
            *value = This->dtdmode;
            break;
        case XmlReaderProperty_ReadState:
            *value = This->state;
            break;
        case XmlReaderProperty_MaxElementDepth:
            *value = This->max_depth;
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

    TRACE("(%p)->(%s 0x%lx)\n", This, debugstr_reader_prop(property), value);

    switch (property)
    {
        case XmlReaderProperty_MultiLanguage:
            if (This->mlang)
                IUnknown_Release(This->mlang);
            This->mlang = (IUnknown*)value;
            if (This->mlang)
                IUnknown_AddRef(This->mlang);
            if (This->mlang)
                FIXME("Ignoring MultiLanguage %p\n", This->mlang);
            break;
        case XmlReaderProperty_XmlResolver:
            if (This->resolver)
                IXmlResolver_Release(This->resolver);
            This->resolver = (IXmlResolver*)value;
            if (This->resolver)
                IXmlResolver_AddRef(This->resolver);
            break;
        case XmlReaderProperty_DtdProcessing:
            if (value < 0 || value > _DtdProcessing_Last) return E_INVALIDARG;
            This->dtdmode = value;
            break;
        case XmlReaderProperty_MaxElementDepth:
            This->max_depth = value;
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
    XmlNodeType type;
    HRESULT hr;

    TRACE("(%p)->(%p)\n", This, nodetype);

    if (!nodetype)
        nodetype = &type;

    switch (This->state)
    {
    case XmlReadState_Closed:
        hr = S_FALSE;
        break;
    case XmlReadState_Error:
        hr = This->error;
        break;
    default:
        hr = reader_parse_nextnode(This);
        if (SUCCEEDED(hr) && oldtype == XmlNodeType_None && This->nodetype != oldtype)
            This->state = XmlReadState_Interactive;

        if (FAILED(hr))
        {
            This->state = XmlReadState_Error;
            This->nodetype = XmlNodeType_None;
            This->depth = 0;
            This->error = hr;
        }
    }

    TRACE("node type %s\n", debugstr_nodetype(This->nodetype));
    *nodetype = This->nodetype;

    return hr;
}

static HRESULT WINAPI xmlreader_GetNodeType(IXmlReader* iface, XmlNodeType *node_type)
{
    xmlreader *This = impl_from_IXmlReader(iface);

    TRACE("(%p)->(%p)\n", This, node_type);

    if (!node_type)
        return E_INVALIDARG;

    *node_type = reader_get_nodetype(This);
    return This->state == XmlReadState_Closed ? S_FALSE : S_OK;
}

static void reader_set_current_attribute(xmlreader *reader, struct attribute *attr)
{
    reader->attr = attr;
    reader->chunk_read_off = 0;
    reader_set_strvalue(reader, StringValue_Prefix, &attr->prefix);
    reader_set_strvalue(reader, StringValue_QualifiedName, &attr->qname);
    reader_set_strvalue(reader, StringValue_Value, &attr->value);
}

static HRESULT reader_move_to_first_attribute(xmlreader *reader)
{
    if (!reader->attr_count)
        return S_FALSE;

    if (!reader->attr)
        reader_inc_depth(reader);

    reader_set_current_attribute(reader, LIST_ENTRY(list_head(&reader->attrs), struct attribute, entry));

    return S_OK;
}

static HRESULT WINAPI xmlreader_MoveToFirstAttribute(IXmlReader* iface)
{
    xmlreader *This = impl_from_IXmlReader(iface);

    TRACE("(%p)\n", This);

    return reader_move_to_first_attribute(This);
}

static HRESULT WINAPI xmlreader_MoveToNextAttribute(IXmlReader* iface)
{
    xmlreader *This = impl_from_IXmlReader(iface);
    const struct list *next;

    TRACE("(%p)\n", This);

    if (!This->attr_count) return S_FALSE;

    if (!This->attr)
        return reader_move_to_first_attribute(This);

    next = list_next(&This->attrs, &This->attr->entry);
    if (next)
        reader_set_current_attribute(This, LIST_ENTRY(next, struct attribute, entry));

    return next ? S_OK : S_FALSE;
}

static void reader_get_attribute_ns_uri(xmlreader *reader, struct attribute *attr, const WCHAR **uri, UINT *len)
{
    static const WCHAR xmlns_uriW[] = {'h','t','t','p',':','/','/','w','w','w','.','w','3','.','o','r','g','/',
            '2','0','0','0','/','x','m','l','n','s','/',0};
    static const WCHAR xml_uriW[] = {'h','t','t','p',':','/','/','w','w','w','.','w','3','.','o','r','g','/',
            'X','M','L','/','1','9','9','8','/','n','a','m','e','s','p','a','c','e',0};

    /* Check for reserved prefixes first */
    if ((strval_eq(reader, &attr->prefix, &strval_empty) && strval_eq(reader, &attr->localname, &strval_xmlns)) ||
            strval_eq(reader, &attr->prefix, &strval_xmlns))
    {
        *uri = xmlns_uriW;
        *len = ARRAY_SIZE(xmlns_uriW) - 1;
    }
    else if (strval_eq(reader, &attr->prefix, &strval_xml))
    {
        *uri = xml_uriW;
        *len = ARRAY_SIZE(xml_uriW) - 1;
    }
    else
    {
        *uri = NULL;
        *len = 0;
    }

    if (!*uri)
    {
        struct ns *ns;

        if ((ns = reader_lookup_ns(reader, &attr->prefix)))
        {
            *uri = ns->uri.str;
            *len = ns->uri.len;
        }
        else
        {
            *uri = emptyW;
            *len = 0;
        }
    }
}

static void reader_get_attribute_local_name(xmlreader *reader, struct attribute *attr, const WCHAR **name, UINT *len)
{
    if (attr->flags & ATTRIBUTE_DEFAULT_NS_DEFINITION)
    {
        *name = xmlnsW;
        *len = 5;
    }
    else if (attr->flags & ATTRIBUTE_NS_DEFINITION)
    {
        const struct ns *ns = reader_lookup_ns(reader, &attr->localname);
        *name = ns->prefix.str;
        *len = ns->prefix.len;
    }
    else
    {
        *name = attr->localname.str;
        *len = attr->localname.len;
    }
}

static HRESULT WINAPI xmlreader_MoveToAttributeByName(IXmlReader* iface,
    const WCHAR *local_name, const WCHAR *namespace_uri)
{
    xmlreader *This = impl_from_IXmlReader(iface);
    UINT target_name_len, target_uri_len;
    struct attribute *attr;

    TRACE("(%p)->(%s %s)\n", This, debugstr_w(local_name), debugstr_w(namespace_uri));

    if (!local_name)
        return E_INVALIDARG;

    if (!This->attr_count)
        return S_FALSE;

    if (!namespace_uri)
        namespace_uri = emptyW;

    target_name_len = lstrlenW(local_name);
    target_uri_len = lstrlenW(namespace_uri);

    LIST_FOR_EACH_ENTRY(attr, &This->attrs, struct attribute, entry)
    {
        UINT name_len, uri_len;
        const WCHAR *name, *uri;

        reader_get_attribute_local_name(This, attr, &name, &name_len);
        reader_get_attribute_ns_uri(This, attr, &uri, &uri_len);

        if (name_len == target_name_len && uri_len == target_uri_len &&
                !wcscmp(name, local_name) && !wcscmp(uri, namespace_uri))
        {
            reader_set_current_attribute(This, attr);
            return S_OK;
        }
    }

    return S_FALSE;
}

static HRESULT WINAPI xmlreader_MoveToElement(IXmlReader* iface)
{
    xmlreader *This = impl_from_IXmlReader(iface);

    TRACE("(%p)\n", This);

    if (!This->attr_count) return S_FALSE;

    if (This->attr)
        reader_dec_depth(This);

    This->attr = NULL;

    /* FIXME: support other node types with 'attributes' like DTD */
    if (This->is_empty_element) {
        reader_set_strvalue(This, StringValue_Prefix, &This->empty_element.prefix);
        reader_set_strvalue(This, StringValue_QualifiedName, &This->empty_element.qname);
    }
    else {
        struct element *element = LIST_ENTRY(list_head(&This->elements), struct element, entry);
        if (element) {
            reader_set_strvalue(This, StringValue_Prefix, &element->prefix);
            reader_set_strvalue(This, StringValue_QualifiedName, &element->qname);
        }
    }
    This->chunk_read_off = 0;
    reader_set_strvalue(This, StringValue_Value, &strval_empty);

    return S_OK;
}

static HRESULT WINAPI xmlreader_GetQualifiedName(IXmlReader* iface, LPCWSTR *name, UINT *len)
{
    xmlreader *This = impl_from_IXmlReader(iface);
    struct attribute *attribute = This->attr;
    struct element *element;
    UINT length;

    TRACE("(%p)->(%p %p)\n", This, name, len);

    if (!len)
        len = &length;

    switch (reader_get_nodetype(This))
    {
    case XmlNodeType_Text:
    case XmlNodeType_CDATA:
    case XmlNodeType_Comment:
    case XmlNodeType_Whitespace:
        *name = emptyW;
        *len = 0;
        break;
    case XmlNodeType_Element:
    case XmlNodeType_EndElement:
        element = reader_get_element(This);
        if (element->prefix.len)
        {
            *name = element->qname.str;
            *len = element->qname.len;
        }
        else
        {
            *name = element->localname.str;
            *len = element->localname.len;
        }
        break;
    case XmlNodeType_Attribute:
        if (attribute->flags & ATTRIBUTE_DEFAULT_NS_DEFINITION)
        {
            *name = xmlnsW;
            *len = 5;
        } else if (attribute->prefix.len)
        {
            *name = This->strvalues[StringValue_QualifiedName].str;
            *len = This->strvalues[StringValue_QualifiedName].len;
        }
        else
        {
            *name = attribute->localname.str;
            *len = attribute->localname.len;
        }
        break;
    default:
        *name = This->strvalues[StringValue_QualifiedName].str;
        *len = This->strvalues[StringValue_QualifiedName].len;
        break;
    }

    return S_OK;
}

static struct ns *reader_lookup_nsdef(xmlreader *reader)
{
    if (list_empty(&reader->nsdef))
        return NULL;

    return LIST_ENTRY(list_head(&reader->nsdef), struct ns, entry);
}

static HRESULT WINAPI xmlreader_GetNamespaceUri(IXmlReader* iface, const WCHAR **uri, UINT *len)
{
    xmlreader *This = impl_from_IXmlReader(iface);
    const strval *prefix = &This->strvalues[StringValue_Prefix];
    XmlNodeType nodetype;
    struct ns *ns;
    UINT length;

    TRACE("(%p %p %p)\n", iface, uri, len);

    if (!len)
        len = &length;

    switch ((nodetype = reader_get_nodetype(This)))
    {
    case XmlNodeType_Attribute:
        reader_get_attribute_ns_uri(This, This->attr, uri, len);
        break;
    case XmlNodeType_Element:
    case XmlNodeType_EndElement:
        {
            ns = reader_lookup_ns(This, prefix);

            /* pick top default ns if any */
            if (!ns)
                ns = reader_lookup_nsdef(This);

            if (ns) {
                *uri = ns->uri.str;
                *len = ns->uri.len;
            }
            else {
                *uri = emptyW;
                *len = 0;
            }
        }
        break;
    case XmlNodeType_Text:
    case XmlNodeType_CDATA:
    case XmlNodeType_ProcessingInstruction:
    case XmlNodeType_Comment:
    case XmlNodeType_Whitespace:
    case XmlNodeType_XmlDeclaration:
        *uri = emptyW;
        *len = 0;
        break;
    default:
        FIXME("Unhandled node type %d\n", nodetype);
        *uri = NULL;
        *len = 0;
        return E_NOTIMPL;
    }

    return S_OK;
}

static HRESULT WINAPI xmlreader_GetLocalName(IXmlReader* iface, LPCWSTR *name, UINT *len)
{
    xmlreader *This = impl_from_IXmlReader(iface);
    struct element *element;
    UINT length;

    TRACE("(%p)->(%p %p)\n", This, name, len);

    if (!len)
        len = &length;

    switch (reader_get_nodetype(This))
    {
    case XmlNodeType_Text:
    case XmlNodeType_CDATA:
    case XmlNodeType_Comment:
    case XmlNodeType_Whitespace:
        *name = emptyW;
        *len = 0;
        break;
    case XmlNodeType_Element:
    case XmlNodeType_EndElement:
        element = reader_get_element(This);
        *name = element->localname.str;
        *len = element->localname.len;
        break;
    case XmlNodeType_Attribute:
        reader_get_attribute_local_name(This, This->attr, name, len);
        break;
    default:
        *name = This->strvalues[StringValue_LocalName].str;
        *len = This->strvalues[StringValue_LocalName].len;
        break;
    }

    return S_OK;
}

static HRESULT WINAPI xmlreader_GetPrefix(IXmlReader* iface, const WCHAR **ret, UINT *len)
{
    xmlreader *This = impl_from_IXmlReader(iface);
    XmlNodeType nodetype;
    UINT length;

    TRACE("(%p)->(%p %p)\n", This, ret, len);

    if (!len)
        len = &length;

    *ret = emptyW;
    *len = 0;

    switch ((nodetype = reader_get_nodetype(This)))
    {
    case XmlNodeType_Element:
    case XmlNodeType_EndElement:
    case XmlNodeType_Attribute:
    {
        const strval *prefix = &This->strvalues[StringValue_Prefix];
        struct ns *ns;

        if (strval_eq(This, prefix, &strval_xml))
        {
            *ret = xmlW;
            *len = 3;
        }
        else if (strval_eq(This, prefix, &strval_xmlns))
        {
            *ret = xmlnsW;
            *len = 5;
        }
        else if ((ns = reader_lookup_ns(This, prefix)))
        {
            *ret = ns->prefix.str;
            *len = ns->prefix.len;
        }

        break;
    }
    default:
        ;
    }

    return S_OK;
}

static const strval *reader_get_value(xmlreader *reader, BOOL ensure_allocated)
{
    strval *val;

    switch (reader_get_nodetype(reader))
    {
    case XmlNodeType_XmlDeclaration:
    case XmlNodeType_EndElement:
    case XmlNodeType_None:
        return &strval_empty;
    case XmlNodeType_Attribute:
        /* For namespace definition attributes return values from namespace list */
        if (reader->attr->flags & (ATTRIBUTE_NS_DEFINITION | ATTRIBUTE_DEFAULT_NS_DEFINITION))
        {
            struct ns *ns;

            if (!(ns = reader_lookup_ns(reader, &reader->attr->localname)))
                ns = reader_lookup_nsdef(reader);

            return &ns->uri;
        }
        return &reader->attr->value;
    default:
        break;
    }

    val = &reader->strvalues[StringValue_Value];
    if (!val->str && ensure_allocated)
    {
        WCHAR *ptr = reader_alloc(reader, (val->len+1)*sizeof(WCHAR));
        if (!ptr) return NULL;
        memcpy(ptr, reader_get_strptr(reader, val), val->len*sizeof(WCHAR));
        ptr[val->len] = 0;
        val->str = ptr;
    }

    return val;
}

static HRESULT WINAPI xmlreader_GetValue(IXmlReader* iface, const WCHAR **value, UINT *len)
{
    xmlreader *reader = impl_from_IXmlReader(iface);
    const strval *val = &reader->strvalues[StringValue_Value];
    UINT off;

    TRACE("(%p)->(%p %p)\n", reader, value, len);

    *value = NULL;

    if ((reader->nodetype == XmlNodeType_Comment && !val->str && !val->len) || is_reader_pending(reader))
    {
        XmlNodeType type;
        HRESULT hr;

        hr = IXmlReader_Read(iface, &type);
        if (FAILED(hr)) return hr;

        /* return if still pending, partially read values are not reported */
        if (is_reader_pending(reader)) return E_PENDING;
    }

    val = reader_get_value(reader, TRUE);
    if (!val)
        return E_OUTOFMEMORY;

    off = abs(reader->chunk_read_off);
    assert(off <= val->len);
    *value = val->str + off;
    if (len) *len = val->len - off;
    reader->chunk_read_off = -(INT)off;
    return S_OK;
}

static HRESULT WINAPI xmlreader_ReadValueChunk(IXmlReader* iface, WCHAR *buffer, UINT chunk_size, UINT *read)
{
    xmlreader *reader = impl_from_IXmlReader(iface);
    const strval *val;
    UINT len = 0;

    TRACE("(%p)->(%p %u %p)\n", reader, buffer, chunk_size, read);

    val = reader_get_value(reader, FALSE);

    /* If value is already read by GetValue, chunk_read_off is negative and chunked reads are not possible. */
    if (reader->chunk_read_off >= 0)
    {
        assert(reader->chunk_read_off <= val->len);
        len = min(val->len - reader->chunk_read_off, chunk_size);
    }
    if (read) *read = len;

    if (len)
    {
        memcpy(buffer, reader_get_strptr(reader, val) + reader->chunk_read_off, len*sizeof(WCHAR));
        reader->chunk_read_off += len;
    }

    return len || !chunk_size ? S_OK : S_FALSE;
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
    return FALSE;
}

static BOOL WINAPI xmlreader_IsEmptyElement(IXmlReader* iface)
{
    xmlreader *This = impl_from_IXmlReader(iface);
    TRACE("(%p)\n", This);
    /* Empty elements are not placed in stack, it's stored as a global reader flag that makes sense
       when current node is start tag of an element */
    return (reader_get_nodetype(This) == XmlNodeType_Element) ? This->is_empty_element : FALSE;
}

static HRESULT WINAPI xmlreader_GetLineNumber(IXmlReader* iface, UINT *line_number)
{
    xmlreader *This = impl_from_IXmlReader(iface);
    const struct element *element;

    TRACE("(%p %p)\n", This, line_number);

    if (!line_number)
        return E_INVALIDARG;

    switch (reader_get_nodetype(This))
    {
    case XmlNodeType_Element:
    case XmlNodeType_EndElement:
        element = reader_get_element(This);
        *line_number = element->position.line_number;
        break;
    case XmlNodeType_Attribute:
        *line_number = This->attr->position.line_number;
        break;
    case XmlNodeType_Whitespace:
    case XmlNodeType_XmlDeclaration:
        *line_number = This->empty_element.position.line_number;
        break;
    default:
        *line_number = This->position.line_number;
        break;
    }

    return This->state == XmlReadState_Closed ? S_FALSE : S_OK;
}

static HRESULT WINAPI xmlreader_GetLinePosition(IXmlReader* iface, UINT *line_position)
{
    xmlreader *This = impl_from_IXmlReader(iface);
    const struct element *element;

    TRACE("(%p %p)\n", This, line_position);

    if (!line_position)
        return E_INVALIDARG;

    switch (reader_get_nodetype(This))
    {
    case XmlNodeType_Element:
    case XmlNodeType_EndElement:
        element = reader_get_element(This);
        *line_position = element->position.line_position;
        break;
    case XmlNodeType_Attribute:
        *line_position = This->attr->position.line_position;
        break;
    case XmlNodeType_Whitespace:
    case XmlNodeType_XmlDeclaration:
        *line_position = This->empty_element.position.line_position;
        break;
    default:
        *line_position = This->position.line_position;
        break;
    }

    return This->state == XmlReadState_Closed ? S_FALSE : S_OK;
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
    xmlreader *This = impl_from_IXmlReader(iface);
    TRACE("(%p)\n", iface);
    return This->state == XmlReadState_EndOfFile;
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
    HRESULT hr;
    int i;

    TRACE("(%s, %p, %p)\n", wine_dbgstr_guid(riid), obj, imalloc);

    if (imalloc)
        reader = IMalloc_Alloc(imalloc, sizeof(*reader));
    else
        reader = heap_alloc(sizeof(*reader));
    if (!reader)
        return E_OUTOFMEMORY;

    memset(reader, 0, sizeof(*reader));
    reader->IXmlReader_iface.lpVtbl = &xmlreader_vtbl;
    reader->ref = 1;
    reader->state = XmlReadState_Closed;
    reader->instate = XmlReadInState_Initial;
    reader->resumestate = XmlReadResumeState_Initial;
    reader->dtdmode = DtdProcessing_Prohibit;
    reader->imalloc = imalloc;
    if (imalloc) IMalloc_AddRef(imalloc);
    reader->nodetype = XmlNodeType_None;
    list_init(&reader->attrs);
    list_init(&reader->nsdef);
    list_init(&reader->ns);
    list_init(&reader->elements);
    reader->max_depth = 256;

    reader->chunk_read_off = 0;
    for (i = 0; i < StringValue_Last; i++)
        reader->strvalues[i] = strval_empty;

    hr = IXmlReader_QueryInterface(&reader->IXmlReader_iface, riid, obj);
    IXmlReader_Release(&reader->IXmlReader_iface);

    TRACE("returning iface %p, hr %#x\n", *obj, hr);

    return hr;
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
