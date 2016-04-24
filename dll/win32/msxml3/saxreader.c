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

#include "precomp.h"

#ifdef HAVE_LIBXML2
# include <libxml/parserInternals.h>
#endif

#ifdef HAVE_LIBXML2

typedef enum
{
    FeatureUnknown               = 0,
    ExhaustiveErrors             = 1 << 1,
    ExternalGeneralEntities      = 1 << 2,
    ExternalParameterEntities    = 1 << 3,
    ForcedResync                 = 1 << 4,
    NamespacePrefixes            = 1 << 5,
    Namespaces                   = 1 << 6,
    ParameterEntities            = 1 << 7,
    PreserveSystemIndentifiers   = 1 << 8,
    ProhibitDTD                  = 1 << 9,
    SchemaValidation             = 1 << 10,
    ServerHttpRequest            = 1 << 11,
    SuppressValidationfatalError = 1 << 12,
    UseInlineSchema              = 1 << 13,
    UseSchemaLocation            = 1 << 14,
    LexicalHandlerParEntities    = 1 << 15
} saxreader_feature;

/* feature names */
static const WCHAR FeatureExternalGeneralEntitiesW[] = {
    'h','t','t','p',':','/','/','x','m','l','.','o','r','g','/','s','a','x','/',
    'f','e','a','t','u','r','e','s','/','e','x','t','e','r','n','a','l','-','g','e','n','e','r','a','l',
    '-','e','n','t','i','t','i','e','s',0
};

static const WCHAR FeatureExternalParameterEntitiesW[] = {
    'h','t','t','p',':','/','/','x','m','l','.','o','r','g','/','s','a','x','/','f','e','a','t','u','r','e','s',
    '/','e','x','t','e','r','n','a','l','-','p','a','r','a','m','e','t','e','r','-','e','n','t','i','t','i','e','s',0
};

static const WCHAR FeatureLexicalHandlerParEntitiesW[] = {
    'h','t','t','p',':','/','/','x','m','l','.','o','r','g','/','s','a','x','/','f','e','a','t','u','r','e','s',
    '/','l','e','x','i','c','a','l','-','h','a','n','d','l','e','r','/','p','a','r','a','m','e','t','e','r','-','e','n','t','i','t','i','e','s',0
};

static const WCHAR FeatureProhibitDTDW[] = {
    'p','r','o','h','i','b','i','t','-','d','t','d',0
};

static const WCHAR FeatureNamespacesW[] = {
    'h','t','t','p',':','/','/','x','m','l','.','o','r','g','/','s','a','x','/','f','e','a','t','u','r','e','s',
    '/','n','a','m','e','s','p','a','c','e','s',0
};

static const WCHAR FeatureNamespacePrefixesW[] = {
    'h','t','t','p',':','/','/','x','m','l','.','o','r','g','/','s','a','x','/','f','e','a','t','u','r','e','s',
    '/','n','a','m','e','s','p','a','c','e','-','p','r','e','f','i','x','e','s',0
};

struct saxreader_feature_pair
{
    saxreader_feature feature;
    const WCHAR *name;
};

static const struct saxreader_feature_pair saxreader_feature_map[] = {
    { ExternalGeneralEntities, FeatureExternalGeneralEntitiesW },
    { ExternalParameterEntities, FeatureExternalParameterEntitiesW },
    { LexicalHandlerParEntities, FeatureLexicalHandlerParEntitiesW },
    { NamespacePrefixes, FeatureNamespacePrefixesW },
    { Namespaces, FeatureNamespacesW },
    { ProhibitDTD, FeatureProhibitDTDW }
};

static saxreader_feature get_saxreader_feature(const WCHAR *name)
{
    int min, max, n, c;

    min = 0;
    max = sizeof(saxreader_feature_map)/sizeof(struct saxreader_feature_pair) - 1;

    while (min <= max)
    {
        n = (min+max)/2;

        c = strcmpW(saxreader_feature_map[n].name, name);
        if (!c)
            return saxreader_feature_map[n].feature;

        if (c > 0)
            max = n-1;
        else
            min = n+1;
    }

    return FeatureUnknown;
}

struct bstrpool
{
    BSTR *pool;
    unsigned int index;
    unsigned int len;
};

typedef struct
{
    BSTR prefix;
    BSTR uri;
} ns;

typedef struct
{
    struct list entry;
    BSTR prefix;
    BSTR local;
    BSTR qname;
    ns *ns; /* namespaces defined in this particular element */
    int ns_count;
} element_entry;

enum saxhandler_type
{
    SAXContentHandler = 0,
    SAXDeclHandler,
    SAXDTDHandler,
    SAXEntityResolver,
    SAXErrorHandler,
    SAXLexicalHandler,
    SAXHandler_Last
};

struct saxanyhandler_iface
{
    IUnknown *handler;
    IUnknown *vbhandler;
};

struct saxcontenthandler_iface
{
    ISAXContentHandler *handler;
    IVBSAXContentHandler *vbhandler;
};

struct saxerrorhandler_iface
{
    ISAXErrorHandler *handler;
    IVBSAXErrorHandler *vbhandler;
};

struct saxlexicalhandler_iface
{
    ISAXLexicalHandler *handler;
    IVBSAXLexicalHandler *vbhandler;
};

struct saxentityresolver_iface
{
    ISAXEntityResolver *handler;
    IVBSAXEntityResolver *vbhandler;
};

struct saxhandler_iface
{
    union {
        struct saxcontenthandler_iface content;
        struct saxentityresolver_iface entityresolver;
        struct saxerrorhandler_iface error;
        struct saxlexicalhandler_iface lexical;
        struct saxanyhandler_iface anyhandler;
    } u;
};

typedef struct
{
    DispatchEx dispex;
    IVBSAXXMLReader IVBSAXXMLReader_iface;
    ISAXXMLReader ISAXXMLReader_iface;
    LONG ref;

    struct saxhandler_iface saxhandlers[SAXHandler_Last];
    xmlSAXHandler sax;
    BOOL isParsing;
    struct bstrpool pool;
    saxreader_feature features;
    BSTR xmldecl_version;
    MSXML_VERSION version;
} saxreader;

static HRESULT saxreader_put_handler(saxreader *reader, enum saxhandler_type type, void *ptr, BOOL vb)
{
    struct saxanyhandler_iface *iface = &reader->saxhandlers[type].u.anyhandler;
    IUnknown *unk = (IUnknown*)ptr;

    if (unk)
        IUnknown_AddRef(unk);

    if ((vb && iface->vbhandler) || (!vb && iface->handler))
        IUnknown_Release(vb ? iface->vbhandler : iface->handler);

    if (vb)
        iface->vbhandler = unk;
    else
        iface->handler = unk;

    return S_OK;
}

static HRESULT saxreader_get_handler(const saxreader *reader, enum saxhandler_type type, BOOL vb, void **ret)
{
    const struct saxanyhandler_iface *iface = &reader->saxhandlers[type].u.anyhandler;

    if (!ret) return E_POINTER;

    if ((vb && iface->vbhandler) || (!vb && iface->handler))
    {
        if (vb)
            IUnknown_AddRef(iface->vbhandler);
        else
            IUnknown_AddRef(iface->handler);
    }

    *ret = vb ? iface->vbhandler : iface->handler;

    return S_OK;
}

static struct saxcontenthandler_iface *saxreader_get_contenthandler(saxreader *reader)
{
    return &reader->saxhandlers[SAXContentHandler].u.content;
}

static struct saxerrorhandler_iface *saxreader_get_errorhandler(saxreader *reader)
{
    return &reader->saxhandlers[SAXErrorHandler].u.error;
}

static struct saxlexicalhandler_iface *saxreader_get_lexicalhandler(saxreader *reader)
{
    return &reader->saxhandlers[SAXLexicalHandler].u.lexical;
}

typedef struct
{
    IVBSAXLocator IVBSAXLocator_iface;
    ISAXLocator ISAXLocator_iface;
    IVBSAXAttributes IVBSAXAttributes_iface;
    ISAXAttributes ISAXAttributes_iface;
    LONG ref;
    saxreader *saxreader;
    HRESULT ret;
    xmlParserCtxtPtr pParserCtxt;
    BSTR publicId;
    BSTR systemId;
    int line;
    int column;
    BOOL vbInterface;
    struct list elements;

    BSTR namespaceUri;
    int attr_alloc_count;
    int attr_count;
    struct _attributes
    {
        BSTR szLocalname;
        BSTR szURI;
        BSTR szValue;
        BSTR szQName;
    } *attributes;
} saxlocator;

static inline saxreader *impl_from_IVBSAXXMLReader( IVBSAXXMLReader *iface )
{
    return CONTAINING_RECORD(iface, saxreader, IVBSAXXMLReader_iface);
}

static inline saxreader *impl_from_ISAXXMLReader( ISAXXMLReader *iface )
{
    return CONTAINING_RECORD(iface, saxreader, ISAXXMLReader_iface);
}

static inline saxlocator *impl_from_IVBSAXLocator( IVBSAXLocator *iface )
{
    return CONTAINING_RECORD(iface, saxlocator, IVBSAXLocator_iface);
}

static inline saxlocator *impl_from_ISAXLocator( ISAXLocator *iface )
{
    return CONTAINING_RECORD(iface, saxlocator, ISAXLocator_iface);
}

static inline saxlocator *impl_from_IVBSAXAttributes( IVBSAXAttributes *iface )
{
    return CONTAINING_RECORD(iface, saxlocator, IVBSAXAttributes_iface);
}

static inline saxlocator *impl_from_ISAXAttributes( ISAXAttributes *iface )
{
    return CONTAINING_RECORD(iface, saxlocator, ISAXAttributes_iface);
}

static inline BOOL saxreader_has_handler(const saxlocator *locator, enum saxhandler_type type)
{
    struct saxanyhandler_iface *iface = &locator->saxreader->saxhandlers[type].u.anyhandler;
    return (locator->vbInterface && iface->vbhandler) || (!locator->vbInterface && iface->handler);
}

static HRESULT saxreader_saxcharacters(saxlocator *locator, BSTR chars)
{
    struct saxcontenthandler_iface *content = saxreader_get_contenthandler(locator->saxreader);
    HRESULT hr;

    if (!saxreader_has_handler(locator, SAXContentHandler)) return S_OK;

    if (locator->vbInterface)
        hr = IVBSAXContentHandler_characters(content->vbhandler, &chars);
    else
        hr = ISAXContentHandler_characters(content->handler, chars, SysStringLen(chars));

    return hr;
}

/* property names */
static const WCHAR PropertyCharsetW[] = {
    'c','h','a','r','s','e','t',0
};
static const WCHAR PropertyXmlDeclVersionW[] = {
    'x','m','l','d','e','c','l','-','v','e','r','s','i','o','n',0
};
static const WCHAR PropertyDeclHandlerW[] = {
    'h','t','t','p',':','/','/','x','m','l','.','o','r','g','/',
    's','a','x','/','p','r','o','p','e','r','t','i','e','s','/',
    'd','e','c','l','a','r','a','t','i','o','n',
    '-','h','a','n','d','l','e','r',0
};
static const WCHAR PropertyDomNodeW[] = {
    'h','t','t','p',':','/','/','x','m','l','.','o','r','g','/',
    's','a','x','/','p','r','o','p','e','r','t','i','e','s','/',
    'd','o','m','-','n','o','d','e',0
};
static const WCHAR PropertyInputSourceW[] = {
    'i','n','p','u','t','-','s','o','u','r','c','e',0
};
static const WCHAR PropertyLexicalHandlerW[] = {
    'h','t','t','p',':','/','/','x','m','l','.','o','r','g','/',
    's','a','x','/','p','r','o','p','e','r','t','i','e','s','/',
    'l','e','x','i','c','a','l','-','h','a','n','d','l','e','r',0
};
static const WCHAR PropertyMaxElementDepthW[] = {
    'm','a','x','-','e','l','e','m','e','n','t','-','d','e','p','t','h',0
};
static const WCHAR PropertyMaxXMLSizeW[] = {
    'm','a','x','-','x','m','l','-','s','i','z','e',0
};
static const WCHAR PropertySchemaDeclHandlerW[] = {
    's','c','h','e','m','a','-','d','e','c','l','a','r','a','t','i','o','n','-',
    'h','a','n','d','l','e','r',0
};
static const WCHAR PropertyXMLDeclEncodingW[] = {
    'x','m','l','d','e','c','l','-','e','n','c','o','d','i','n','g',0
};
static const WCHAR PropertyXMLDeclStandaloneW[] = {
    'x','m','l','d','e','c','l','-','s','t','a','n','d','a','l','o','n','e',0
};
static const WCHAR PropertyXMLDeclVersionW[] = {
    'x','m','l','d','e','c','l','-','v','e','r','s','i','o','n',0
};

static inline HRESULT set_feature_value(saxreader *reader, saxreader_feature feature, VARIANT_BOOL value)
{
    /* handling of non-VARIANT_* values is version dependent */
    if ((reader->version <  MSXML4) && (value != VARIANT_TRUE))
        value = VARIANT_FALSE;
    if ((reader->version >= MSXML4) && (value != VARIANT_FALSE))
        value = VARIANT_TRUE;

    if (value == VARIANT_TRUE)
        reader->features |=  feature;
    else
        reader->features &= ~feature;

    return S_OK;
}

static inline HRESULT get_feature_value(const saxreader *reader, saxreader_feature feature, VARIANT_BOOL *value)
{
    *value = reader->features & feature ? VARIANT_TRUE : VARIANT_FALSE;
    return S_OK;
}

static BOOL is_namespaces_enabled(const saxreader *reader)
{
    return (reader->version < MSXML4) || (reader->features & Namespaces);
}

static BSTR build_qname(BSTR prefix, BSTR local)
{
    if (prefix && *prefix)
    {
        BSTR qname = SysAllocStringLen(NULL, SysStringLen(prefix) + SysStringLen(local) + 1);
        WCHAR *ptr;

        ptr = qname;
        strcpyW(ptr, prefix);
        ptr += SysStringLen(prefix);
        *ptr++ = ':';
        strcpyW(ptr, local);
        return qname;
    }
    else
        return SysAllocString(local);
}

static element_entry* alloc_element_entry(const xmlChar *local, const xmlChar *prefix, int nb_ns,
    const xmlChar **namespaces)
{
    element_entry *ret;
    int i;

    ret = heap_alloc(sizeof(*ret));
    if (!ret) return ret;

    ret->local  = bstr_from_xmlChar(local);
    ret->prefix = bstr_from_xmlChar(prefix);
    ret->qname  = build_qname(ret->prefix, ret->local);
    ret->ns = nb_ns ? heap_alloc(nb_ns*sizeof(ns)) : NULL;
    ret->ns_count = nb_ns;

    for (i=0; i < nb_ns; i++)
    {
        ret->ns[i].prefix = bstr_from_xmlChar(namespaces[2*i]);
        ret->ns[i].uri = bstr_from_xmlChar(namespaces[2*i+1]);
    }

    return ret;
}

static void free_element_entry(element_entry *element)
{
    int i;

    for (i=0; i<element->ns_count;i++)
    {
        SysFreeString(element->ns[i].prefix);
        SysFreeString(element->ns[i].uri);
    }

    SysFreeString(element->prefix);
    SysFreeString(element->local);
    SysFreeString(element->qname);

    heap_free(element->ns);
    heap_free(element);
}

static void push_element_ns(saxlocator *locator, element_entry *element)
{
    list_add_head(&locator->elements, &element->entry);
}

static element_entry * pop_element_ns(saxlocator *locator)
{
    element_entry *element = LIST_ENTRY(list_head(&locator->elements), element_entry, entry);

    if (element)
        list_remove(&element->entry);

    return element;
}

static BSTR find_element_uri(saxlocator *locator, const xmlChar *uri)
{
    element_entry *element;
    BSTR uriW;
    int i;

    if (!uri) return NULL;

    uriW = bstr_from_xmlChar(uri);

    LIST_FOR_EACH_ENTRY(element, &locator->elements, element_entry, entry)
    {
        for (i=0; i < element->ns_count; i++)
            if (!strcmpW(uriW, element->ns[i].uri))
            {
                SysFreeString(uriW);
                return element->ns[i].uri;
            }
    }

    SysFreeString(uriW);
    ERR("namespace uri not found, %s\n", debugstr_a((char*)uri));
    return NULL;
}

/* used to localize version dependent error check behaviour */
static inline BOOL sax_callback_failed(saxlocator *This, HRESULT hr)
{
    return This->saxreader->version >= MSXML4 ? FAILED(hr) : hr != S_OK;
}

/* index value -1 means it tries to loop for a first time */
static inline BOOL iterate_endprefix_index(saxlocator *This, const element_entry *element, int *i)
{
    if (This->saxreader->version >= MSXML4)
    {
        if (*i == -1) *i = 0; else ++*i;
        return *i < element->ns_count;
    }
    else
    {
        if (*i == -1) *i = element->ns_count-1; else --*i;
        return *i >= 0;
    }
}

static BOOL bstr_pool_insert(struct bstrpool *pool, BSTR pool_entry)
{
    if (!pool->pool)
    {
        pool->pool = HeapAlloc(GetProcessHeap(), 0, 16 * sizeof(*pool->pool));
        if (!pool->pool)
            return FALSE;

        pool->index = 0;
        pool->len = 16;
    }
    else if (pool->index == pool->len)
    {
        BSTR *realloc = HeapReAlloc(GetProcessHeap(), 0, pool->pool, pool->len * 2 * sizeof(*realloc));

        if (!realloc)
            return FALSE;

        pool->pool = realloc;
        pool->len *= 2;
    }

    pool->pool[pool->index++] = pool_entry;
    return TRUE;
}

static void free_bstr_pool(struct bstrpool *pool)
{
    unsigned int i;

    for (i = 0; i < pool->index; i++)
        SysFreeString(pool->pool[i]);

    HeapFree(GetProcessHeap(), 0, pool->pool);

    pool->pool = NULL;
    pool->index = pool->len = 0;
}

static BSTR bstr_from_xmlCharN(const xmlChar *buf, int len)
{
    DWORD dLen;
    BSTR bstr;

    if (!buf)
        return NULL;

    dLen = MultiByteToWideChar(CP_UTF8, 0, (LPCSTR)buf, len, NULL, 0);
    if(len != -1) dLen++;
    bstr = SysAllocStringLen(NULL, dLen-1);
    if (!bstr)
        return NULL;
    MultiByteToWideChar(CP_UTF8, 0, (LPCSTR)buf, len, bstr, dLen);
    if(len != -1) bstr[dLen-1] = '\0';

    return bstr;
}

static BSTR QName_from_xmlChar(const xmlChar *prefix, const xmlChar *name)
{
    xmlChar *qname;
    BSTR bstr;

    if(!name) return NULL;

    if(!prefix || !*prefix)
        return bstr_from_xmlChar(name);

    qname = xmlBuildQName(name, prefix, NULL, 0);
    bstr = bstr_from_xmlChar(qname);
    xmlFree(qname);

    return bstr;
}

static BSTR pooled_bstr_from_xmlChar(struct bstrpool *pool, const xmlChar *buf)
{
    BSTR pool_entry = bstr_from_xmlChar(buf);

    if (pool_entry && !bstr_pool_insert(pool, pool_entry))
    {
        SysFreeString(pool_entry);
        return NULL;
    }

    return pool_entry;
}

static BSTR pooled_bstr_from_xmlCharN(struct bstrpool *pool, const xmlChar *buf, int len)
{
    BSTR pool_entry = bstr_from_xmlCharN(buf, len);

    if (pool_entry && !bstr_pool_insert(pool, pool_entry))
    {
        SysFreeString(pool_entry);
        return NULL;
    }

    return pool_entry;
}

static void format_error_message_from_id(saxlocator *This, HRESULT hr)
{
    struct saxerrorhandler_iface *handler = saxreader_get_errorhandler(This->saxreader);
    xmlStopParser(This->pParserCtxt);
    This->ret = hr;

    if (saxreader_has_handler(This, SAXErrorHandler))
    {
        WCHAR msg[1024];
        if(!FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM,
                    NULL, hr, 0, msg, sizeof(msg)/sizeof(msg[0]), NULL))
        {
            FIXME("MSXML errors not yet supported.\n");
            msg[0] = '\0';
        }

        if(This->vbInterface)
        {
            BSTR bstrMsg = SysAllocString(msg);
            IVBSAXErrorHandler_fatalError(handler->vbhandler,
                    &This->IVBSAXLocator_iface, &bstrMsg, hr);
            SysFreeString(bstrMsg);
        }
        else
            ISAXErrorHandler_fatalError(handler->handler,
                    &This->ISAXLocator_iface, msg, hr);
    }
}

static void update_position(saxlocator *This, BOOL fix_column)
{
    const xmlChar *p = This->pParserCtxt->input->cur-1;
    const xmlChar *baseP = This->pParserCtxt->input->base;

    This->line = xmlSAX2GetLineNumber(This->pParserCtxt);
    if(fix_column)
    {
        This->column = 1;
        for(;p>=baseP && *p!='\n' && *p!='\r'; p--)
            This->column++;
    }
    else
    {
        This->column = xmlSAX2GetColumnNumber(This->pParserCtxt);
    }
}

/*** IVBSAXAttributes interface ***/
static HRESULT WINAPI ivbsaxattributes_QueryInterface(
        IVBSAXAttributes* iface,
        REFIID riid,
        void **ppvObject)
{
    saxlocator *This = impl_from_IVBSAXAttributes(iface);
    TRACE("%p %s %p\n", This, debugstr_guid(riid), ppvObject);
    return IVBSAXLocator_QueryInterface(&This->IVBSAXLocator_iface, riid, ppvObject);
}

static ULONG WINAPI ivbsaxattributes_AddRef(IVBSAXAttributes* iface)
{
    saxlocator *This = impl_from_IVBSAXAttributes(iface);
    return IVBSAXLocator_AddRef(&This->IVBSAXLocator_iface);
}

static ULONG WINAPI ivbsaxattributes_Release(IVBSAXAttributes* iface)
{
    saxlocator *This = impl_from_IVBSAXAttributes(iface);
    return IVBSAXLocator_Release(&This->IVBSAXLocator_iface);
}

static HRESULT WINAPI ivbsaxattributes_GetTypeInfoCount( IVBSAXAttributes *iface, UINT* pctinfo )
{
    saxlocator *This = impl_from_IVBSAXAttributes( iface );

    TRACE("(%p)->(%p)\n", This, pctinfo);

    *pctinfo = 1;

    return S_OK;
}

static HRESULT WINAPI ivbsaxattributes_GetTypeInfo(
    IVBSAXAttributes *iface,
    UINT iTInfo, LCID lcid, ITypeInfo** ppTInfo )
{
    saxlocator *This = impl_from_IVBSAXAttributes( iface );

    TRACE("(%p)->(%u %u %p)\n", This, iTInfo, lcid, ppTInfo);

    return get_typeinfo(IVBSAXAttributes_tid, ppTInfo);
}

static HRESULT WINAPI ivbsaxattributes_GetIDsOfNames(
    IVBSAXAttributes *iface,
    REFIID riid,
    LPOLESTR* rgszNames,
    UINT cNames,
    LCID lcid,
    DISPID* rgDispId)
{
    saxlocator *This = impl_from_IVBSAXAttributes( iface );
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
    saxlocator *This = impl_from_IVBSAXAttributes( iface );
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("(%p)->(%d %s %d %d %p %p %p %p)\n", This, dispIdMember, debugstr_guid(riid),
          lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);

    hr = get_typeinfo(IVBSAXAttributes_tid, &typeinfo);
    if(SUCCEEDED(hr))
    {
        hr = ITypeInfo_Invoke(typeinfo, &This->IVBSAXAttributes_iface, dispIdMember, wFlags,
                pDispParams, pVarResult, pExcepInfo, puArgErr);
        ITypeInfo_Release(typeinfo);
    }

    return hr;
}

/*** IVBSAXAttributes methods ***/
static HRESULT WINAPI ivbsaxattributes_get_length(
        IVBSAXAttributes* iface,
        int *nLength)
{
    saxlocator *This = impl_from_IVBSAXAttributes( iface );
    return ISAXAttributes_getLength(&This->ISAXAttributes_iface, nLength);
}

static HRESULT WINAPI ivbsaxattributes_getURI(
        IVBSAXAttributes* iface,
        int nIndex,
        BSTR *uri)
{
    saxlocator *This = impl_from_IVBSAXAttributes( iface );
    const WCHAR *uriW;
    HRESULT hr;
    int len;

    TRACE("(%p)->(%d %p)\n", This, nIndex, uri);

    if (!uri)
        return E_POINTER;

    *uri = NULL;
    hr = ISAXAttributes_getURI(&This->ISAXAttributes_iface, nIndex, &uriW, &len);
    if (FAILED(hr))
        return hr;

    return return_bstrn(uriW, len, uri);
}

static HRESULT WINAPI ivbsaxattributes_getLocalName(
        IVBSAXAttributes* iface,
        int nIndex,
        BSTR *name)
{
    saxlocator *This = impl_from_IVBSAXAttributes( iface );
    const WCHAR *nameW;
    HRESULT hr;
    int len;

    TRACE("(%p)->(%d %p)\n", This, nIndex, name);

    if (!name)
        return E_POINTER;

    *name = NULL;
    hr = ISAXAttributes_getLocalName(&This->ISAXAttributes_iface, nIndex, &nameW, &len);
    if (FAILED(hr))
        return hr;

    return return_bstrn(nameW, len, name);
}

static HRESULT WINAPI ivbsaxattributes_getQName(
        IVBSAXAttributes* iface,
        int nIndex,
        BSTR *QName)
{
    saxlocator *This = impl_from_IVBSAXAttributes( iface );
    const WCHAR *nameW;
    HRESULT hr;
    int len;

    TRACE("(%p)->(%d %p)\n", This, nIndex, QName);

    if (!QName)
        return E_POINTER;

    *QName = NULL;
    hr = ISAXAttributes_getQName(&This->ISAXAttributes_iface, nIndex, &nameW, &len);
    if (FAILED(hr))
        return hr;

    return return_bstrn(nameW, len, QName);
}

static HRESULT WINAPI ivbsaxattributes_getIndexFromName(
        IVBSAXAttributes* iface,
        BSTR uri,
        BSTR localName,
        int *index)
{
    saxlocator *This = impl_from_IVBSAXAttributes( iface );
    return ISAXAttributes_getIndexFromName(&This->ISAXAttributes_iface, uri, SysStringLen(uri),
            localName, SysStringLen(localName), index);
}

static HRESULT WINAPI ivbsaxattributes_getIndexFromQName(
        IVBSAXAttributes* iface,
        BSTR QName,
        int *index)
{
    saxlocator *This = impl_from_IVBSAXAttributes( iface );
    return ISAXAttributes_getIndexFromQName(&This->ISAXAttributes_iface, QName,
            SysStringLen(QName), index);
}

static HRESULT WINAPI ivbsaxattributes_getType(
        IVBSAXAttributes* iface,
        int nIndex,
        BSTR *type)
{
    saxlocator *This = impl_from_IVBSAXAttributes( iface );
    const WCHAR *typeW;
    HRESULT hr;
    int len;

    TRACE("(%p)->(%d %p)\n", This, nIndex, type);

    if (!type)
        return E_POINTER;

    *type = NULL;
    hr = ISAXAttributes_getType(&This->ISAXAttributes_iface, nIndex, &typeW, &len);
    if (FAILED(hr))
        return hr;

    return return_bstrn(typeW, len, type);
}

static HRESULT WINAPI ivbsaxattributes_getTypeFromName(
        IVBSAXAttributes* iface,
        BSTR uri,
        BSTR localName,
        BSTR *type)
{
    saxlocator *This = impl_from_IVBSAXAttributes( iface );
    const WCHAR *typeW;
    HRESULT hr;
    int len;

    TRACE("(%p)->(%s %s %p)\n", This, debugstr_w(uri), debugstr_w(localName), type);

    if (!type)
        return E_POINTER;

    *type = NULL;
    hr = ISAXAttributes_getTypeFromName(&This->ISAXAttributes_iface, uri, SysStringLen(uri),
            localName, SysStringLen(localName), &typeW, &len);
    if (FAILED(hr))
        return hr;

    return return_bstrn(typeW, len, type);
}

static HRESULT WINAPI ivbsaxattributes_getTypeFromQName(
        IVBSAXAttributes* iface,
        BSTR QName,
        BSTR *type)
{
    saxlocator *This = impl_from_IVBSAXAttributes( iface );
    const WCHAR *typeW;
    HRESULT hr;
    int len;

    TRACE("(%p)->(%s %p)\n", This, debugstr_w(QName), type);

    if (!type)
        return E_POINTER;

    *type = NULL;
    hr = ISAXAttributes_getTypeFromQName(&This->ISAXAttributes_iface, QName, SysStringLen(QName),
            &typeW, &len);
    if (FAILED(hr))
        return hr;

    return return_bstrn(typeW, len, type);
}

static HRESULT WINAPI ivbsaxattributes_getValue(
        IVBSAXAttributes* iface,
        int nIndex,
        BSTR *value)
{
    saxlocator *This = impl_from_IVBSAXAttributes( iface );
    const WCHAR *valueW;
    HRESULT hr;
    int len;

    TRACE("(%p)->(%d %p)\n", This, nIndex, value);

    if (!value)
        return E_POINTER;

    *value = NULL;
    hr = ISAXAttributes_getValue(&This->ISAXAttributes_iface, nIndex, &valueW, &len);
    if (FAILED(hr))
        return hr;

    return return_bstrn(valueW, len, value);
}

static HRESULT WINAPI ivbsaxattributes_getValueFromName(
        IVBSAXAttributes* iface,
        BSTR uri,
        BSTR localName,
        BSTR *value)
{
    saxlocator *This = impl_from_IVBSAXAttributes( iface );
    const WCHAR *valueW;
    HRESULT hr;
    int len;

    TRACE("(%p)->(%s %s %p)\n", This, debugstr_w(uri), debugstr_w(localName), value);

    if (!value)
        return E_POINTER;

    *value = NULL;
    hr = ISAXAttributes_getValueFromName(&This->ISAXAttributes_iface, uri, SysStringLen(uri),
            localName, SysStringLen(localName), &valueW, &len);
    if (FAILED(hr))
        return hr;

    return return_bstrn(valueW, len, value);
}

static HRESULT WINAPI ivbsaxattributes_getValueFromQName(
        IVBSAXAttributes* iface,
        BSTR QName,
        BSTR *value)
{
    saxlocator *This = impl_from_IVBSAXAttributes( iface );
    const WCHAR *valueW;
    HRESULT hr;
    int len;

    TRACE("(%p)->(%s %p)\n", This, debugstr_w(QName), value);

    if (!value)
        return E_POINTER;

    *value = NULL;
    hr = ISAXAttributes_getValueFromQName(&This->ISAXAttributes_iface, QName,
            SysStringLen(QName), &valueW, &len);
    if (FAILED(hr))
        return hr;

    return return_bstrn(valueW, len, value);
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
    saxlocator *This = impl_from_ISAXAttributes(iface);
    TRACE("%p %s %p\n", This, debugstr_guid(riid), ppvObject);
    return ISAXLocator_QueryInterface(&This->ISAXLocator_iface, riid, ppvObject);
}

static ULONG WINAPI isaxattributes_AddRef(ISAXAttributes* iface)
{
    saxlocator *This = impl_from_ISAXAttributes(iface);
    TRACE("%p\n", This);
    return ISAXLocator_AddRef(&This->ISAXLocator_iface);
}

static ULONG WINAPI isaxattributes_Release(ISAXAttributes* iface)
{
    saxlocator *This = impl_from_ISAXAttributes(iface);

    TRACE("%p\n", This);
    return ISAXLocator_Release(&This->ISAXLocator_iface);
}

/*** ISAXAttributes methods ***/
static HRESULT WINAPI isaxattributes_getLength(
        ISAXAttributes* iface,
        int *length)
{
    saxlocator *This = impl_from_ISAXAttributes( iface );

    *length = This->attr_count;
    TRACE("Length set to %d\n", *length);
    return S_OK;
}

static inline BOOL is_valid_attr_index(const saxlocator *locator, int index)
{
    return index < locator->attr_count && index >= 0;
}

static HRESULT WINAPI isaxattributes_getURI(
        ISAXAttributes* iface,
        int index,
        const WCHAR **url,
        int *size)
{
    saxlocator *This = impl_from_ISAXAttributes( iface );
    TRACE("(%p)->(%d)\n", This, index);

    if(!is_valid_attr_index(This, index)) return E_INVALIDARG;
    if(!url || !size) return E_POINTER;

    *size = SysStringLen(This->attributes[index].szURI);
    *url = This->attributes[index].szURI;

    TRACE("(%s:%d)\n", debugstr_w(This->attributes[index].szURI), *size);

    return S_OK;
}

static HRESULT WINAPI isaxattributes_getLocalName(
        ISAXAttributes* iface,
        int index,
        const WCHAR **pLocalName,
        int *pLocalNameLength)
{
    saxlocator *This = impl_from_ISAXAttributes( iface );
    TRACE("(%p)->(%d)\n", This, index);

    if(!is_valid_attr_index(This, index)) return E_INVALIDARG;
    if(!pLocalName || !pLocalNameLength) return E_POINTER;

    *pLocalNameLength = SysStringLen(This->attributes[index].szLocalname);
    *pLocalName = This->attributes[index].szLocalname;

    return S_OK;
}

static HRESULT WINAPI isaxattributes_getQName(
        ISAXAttributes* iface,
        int index,
        const WCHAR **pQName,
        int *pQNameLength)
{
    saxlocator *This = impl_from_ISAXAttributes( iface );
    TRACE("(%p)->(%d)\n", This, index);

    if(!is_valid_attr_index(This, index)) return E_INVALIDARG;
    if(!pQName || !pQNameLength) return E_POINTER;

    *pQNameLength = SysStringLen(This->attributes[index].szQName);
    *pQName = This->attributes[index].szQName;

    return S_OK;
}

static HRESULT WINAPI isaxattributes_getName(
        ISAXAttributes* iface,
        int index,
        const WCHAR **uri,
        int *pUriLength,
        const WCHAR **localName,
        int *pLocalNameSize,
        const WCHAR **QName,
        int *pQNameLength)
{
    saxlocator *This = impl_from_ISAXAttributes( iface );
    TRACE("(%p)->(%d)\n", This, index);

    if(!is_valid_attr_index(This, index)) return E_INVALIDARG;
    if(!uri || !pUriLength || !localName || !pLocalNameSize
            || !QName || !pQNameLength) return E_POINTER;

    *pUriLength = SysStringLen(This->attributes[index].szURI);
    *uri = This->attributes[index].szURI;
    *pLocalNameSize = SysStringLen(This->attributes[index].szLocalname);
    *localName = This->attributes[index].szLocalname;
    *pQNameLength = SysStringLen(This->attributes[index].szQName);
    *QName = This->attributes[index].szQName;

    TRACE("(%s, %s, %s)\n", debugstr_w(*uri), debugstr_w(*localName), debugstr_w(*QName));

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
    saxlocator *This = impl_from_ISAXAttributes( iface );
    int i;
    TRACE("(%p)->(%s, %d, %s, %d)\n", This, debugstr_w(pUri), cUriLength,
            debugstr_w(pLocalName), cocalNameLength);

    if(!pUri || !pLocalName || !index) return E_POINTER;

    for(i=0; i<This->attr_count; i++)
    {
        if(cUriLength!=SysStringLen(This->attributes[i].szURI)
                || cocalNameLength!=SysStringLen(This->attributes[i].szLocalname))
            continue;
        if(cUriLength && memcmp(pUri, This->attributes[i].szURI,
                    sizeof(WCHAR)*cUriLength))
            continue;
        if(cocalNameLength && memcmp(pLocalName, This->attributes[i].szLocalname,
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
    saxlocator *This = impl_from_ISAXAttributes( iface );
    int i;
    TRACE("(%p)->(%s, %d)\n", This, debugstr_w(pQName), nQNameLength);

    if(!pQName || !index) return E_POINTER;
    if(!nQNameLength) return E_INVALIDARG;

    for(i=0; i<This->attr_count; i++)
    {
        if(nQNameLength!=SysStringLen(This->attributes[i].szQName)) continue;
        if(memcmp(pQName, This->attributes[i].szQName, sizeof(WCHAR)*nQNameLength)) continue;

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
    saxlocator *This = impl_from_ISAXAttributes( iface );

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
    saxlocator *This = impl_from_ISAXAttributes( iface );

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
    saxlocator *This = impl_from_ISAXAttributes( iface );

    FIXME("(%p)->(%s, %d) stub\n", This, debugstr_w(pQName), nQName);
    return E_NOTIMPL;
}

static HRESULT WINAPI isaxattributes_getValue(
        ISAXAttributes* iface,
        int index,
        const WCHAR **value,
        int *nValue)
{
    saxlocator *This = impl_from_ISAXAttributes( iface );
    TRACE("(%p)->(%d)\n", This, index);

    if(!is_valid_attr_index(This, index)) return E_INVALIDARG;
    if(!value || !nValue) return E_POINTER;

    *nValue = SysStringLen(This->attributes[index].szValue);
    *value = This->attributes[index].szValue;

    TRACE("(%s:%d)\n", debugstr_w(*value), *nValue);

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
    saxlocator *This = impl_from_ISAXAttributes( iface );
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
    saxlocator *This = impl_from_ISAXAttributes( iface );
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

/* Libxml2 escapes '&' back to char reference '&#38;' in attribute value,
   so when document has escaped value with '&amp;' it's parsed to '&' and then
   escaped to '&#38;'. This function takes care of ampersands only. */
static BSTR saxreader_get_unescaped_value(const xmlChar *buf, int len)
{
    static const WCHAR ampescW[] = {'&','#','3','8',';',0};
    WCHAR *dest, *ptrW, *str;
    DWORD str_len;
    BSTR bstr;

    if (!buf)
        return NULL;

    str_len = MultiByteToWideChar(CP_UTF8, 0, (LPCSTR)buf, len, NULL, 0);
    if (len != -1) str_len++;

    str = heap_alloc(str_len*sizeof(WCHAR));
    if (!str) return NULL;

    MultiByteToWideChar(CP_UTF8, 0, (LPCSTR)buf, len, str, str_len);
    if (len != -1) str[str_len-1] = 0;

    ptrW = str;
    while ((dest = strstrW(ptrW, ampescW)))
    {
        WCHAR *src;

        /* leave first '&' from a reference as a value */
        src = dest + (sizeof(ampescW)/sizeof(WCHAR) - 1);
        dest++;

        /* move together with null terminator */
        memmove(dest, src, (strlenW(src) + 1)*sizeof(WCHAR));

        ptrW++;
    }

    bstr = SysAllocString(str);
    heap_free(str);

    return bstr;
}

static void free_attribute_values(saxlocator *locator)
{
    int i;

    for (i = 0; i < locator->attr_count; i++)
    {
        SysFreeString(locator->attributes[i].szLocalname);
        locator->attributes[i].szLocalname = NULL;

        SysFreeString(locator->attributes[i].szValue);
        locator->attributes[i].szValue = NULL;

        SysFreeString(locator->attributes[i].szQName);
        locator->attributes[i].szQName = NULL;
    }
}

static HRESULT SAXAttributes_populate(saxlocator *locator,
        int nb_namespaces, const xmlChar **xmlNamespaces,
        int nb_attributes, const xmlChar **xmlAttributes)
{
    static const xmlChar xmlns[] = "xmlns";
    static const WCHAR xmlnsW[] = { 'x','m','l','n','s',0 };

    struct _attributes *attrs;
    int i;

    /* skip namespace definitions */
    if ((locator->saxreader->features & NamespacePrefixes) == 0)
        nb_namespaces = 0;

    locator->attr_count = nb_namespaces + nb_attributes;
    if(locator->attr_count > locator->attr_alloc_count)
    {
        int new_size = locator->attr_count * 2;
        attrs = heap_realloc_zero(locator->attributes, new_size * sizeof(struct _attributes));
        if(!attrs)
        {
            free_attribute_values(locator);
            locator->attr_count = 0;
            return E_OUTOFMEMORY;
        }
        locator->attributes = attrs;
        locator->attr_alloc_count = new_size;
    }
    else
    {
        attrs = locator->attributes;
    }

    for (i = 0; i < nb_namespaces; i++)
    {
        SysFreeString(attrs[nb_attributes+i].szLocalname);
        attrs[nb_attributes+i].szLocalname = SysAllocStringLen(NULL, 0);

        attrs[nb_attributes+i].szURI = locator->namespaceUri;

        SysFreeString(attrs[nb_attributes+i].szValue);
        attrs[nb_attributes+i].szValue = bstr_from_xmlChar(xmlNamespaces[2*i+1]);

        SysFreeString(attrs[nb_attributes+i].szQName);
        if(!xmlNamespaces[2*i])
            attrs[nb_attributes+i].szQName = SysAllocString(xmlnsW);
        else
            attrs[nb_attributes+i].szQName = QName_from_xmlChar(xmlns, xmlNamespaces[2*i]);
    }

    for (i = 0; i < nb_attributes; i++)
    {
        static const xmlChar xmlA[] = "xml";

        if (xmlStrEqual(xmlAttributes[i*5+1], xmlA))
            attrs[i].szURI = bstr_from_xmlChar(xmlAttributes[i*5+2]);
        else
            /* that's an important feature to keep same uri pointer for every reported attribute */
            attrs[i].szURI = find_element_uri(locator, xmlAttributes[i*5+2]);

        SysFreeString(attrs[i].szLocalname);
        attrs[i].szLocalname = bstr_from_xmlChar(xmlAttributes[i*5]);

        SysFreeString(attrs[i].szValue);
        attrs[i].szValue = saxreader_get_unescaped_value(xmlAttributes[i*5+3], xmlAttributes[i*5+4]-xmlAttributes[i*5+3]);

        SysFreeString(attrs[i].szQName);
        attrs[i].szQName = QName_from_xmlChar(xmlAttributes[i*5+1], xmlAttributes[i*5]);
    }

    return S_OK;
}

/*** LibXML callbacks ***/
static void libxmlStartDocument(void *ctx)
{
    saxlocator *This = ctx;
    struct saxcontenthandler_iface *handler = saxreader_get_contenthandler(This->saxreader);
    HRESULT hr;

    if (This->saxreader->version >= MSXML4)
    {
        const xmlChar *p = This->pParserCtxt->input->cur-1;
        update_position(This, FALSE);
        while(p>This->pParserCtxt->input->base && *p!='>')
        {
            if(*p=='\n' || (*p=='\r' && *(p+1)!='\n'))
                This->line--;
            p--;
        }
        This->column = 0;
        for(; p>=This->pParserCtxt->input->base && *p!='\n' && *p!='\r'; p--)
            This->column++;
    }

    /* store version value, declaration has to contain version attribute */
    if (This->pParserCtxt->standalone != -1)
    {
        SysFreeString(This->saxreader->xmldecl_version);
        This->saxreader->xmldecl_version = bstr_from_xmlChar(This->pParserCtxt->version);
    }

    if (saxreader_has_handler(This, SAXContentHandler))
    {
        if(This->vbInterface)
            hr = IVBSAXContentHandler_startDocument(handler->vbhandler);
        else
            hr = ISAXContentHandler_startDocument(handler->handler);

        if (sax_callback_failed(This, hr))
            format_error_message_from_id(This, hr);
    }
}

static void libxmlEndDocument(void *ctx)
{
    saxlocator *This = ctx;
    struct saxcontenthandler_iface *handler = saxreader_get_contenthandler(This->saxreader);
    HRESULT hr;

    if (This->saxreader->version >= MSXML4) {
        update_position(This, FALSE);
        if(This->column > 1)
            This->line++;
        This->column = 0;
    } else {
        This->column = 0;
        This->line = 0;
    }

    if(This->ret != S_OK) return;

    if (saxreader_has_handler(This, SAXContentHandler))
    {
        if(This->vbInterface)
            hr = IVBSAXContentHandler_endDocument(handler->vbhandler);
        else
            hr = ISAXContentHandler_endDocument(handler->handler);

        if (sax_callback_failed(This, hr))
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
    saxlocator *This = ctx;
    struct saxcontenthandler_iface *handler = saxreader_get_contenthandler(This->saxreader);
    element_entry *element;
    HRESULT hr = S_OK;
    BSTR uri;

    update_position(This, TRUE);
    if(*(This->pParserCtxt->input->cur) == '/')
        This->column++;
    if(This->saxreader->version < MSXML4)
        This->column++;

    element = alloc_element_entry(localname, prefix, nb_namespaces, namespaces);
    push_element_ns(This, element);

    if (is_namespaces_enabled(This->saxreader))
    {
        int i;

        for (i = 0; i < nb_namespaces && saxreader_has_handler(This, SAXContentHandler); i++)
        {
            if (This->vbInterface)
                hr = IVBSAXContentHandler_startPrefixMapping(
                        handler->vbhandler,
                        &element->ns[i].prefix,
                        &element->ns[i].uri);
            else
                hr = ISAXContentHandler_startPrefixMapping(
                        handler->handler,
                        element->ns[i].prefix,
                        SysStringLen(element->ns[i].prefix),
                        element->ns[i].uri,
                        SysStringLen(element->ns[i].uri));

            if (sax_callback_failed(This, hr))
            {
                format_error_message_from_id(This, hr);
                return;
            }
        }
    }

    uri = find_element_uri(This, URI);
    hr = SAXAttributes_populate(This, nb_namespaces, namespaces, nb_attributes, attributes);
    if (hr == S_OK && saxreader_has_handler(This, SAXContentHandler))
    {
        BSTR local;

        if (is_namespaces_enabled(This->saxreader))
            local = element->local;
        else
            uri = local = NULL;

        if (This->vbInterface)
            hr = IVBSAXContentHandler_startElement(handler->vbhandler,
                    &uri, &local, &element->qname, &This->IVBSAXAttributes_iface);
        else
            hr = ISAXContentHandler_startElement(handler->handler,
                    uri, SysStringLen(uri),
                    local, SysStringLen(local),
                    element->qname, SysStringLen(element->qname),
                    &This->ISAXAttributes_iface);

       if (sax_callback_failed(This, hr))
           format_error_message_from_id(This, hr);
    }
}

static void libxmlEndElementNS(
        void *ctx,
        const xmlChar *localname,
        const xmlChar *prefix,
        const xmlChar *URI)
{
    saxlocator *This = ctx;
    struct saxcontenthandler_iface *handler = saxreader_get_contenthandler(This->saxreader);
    element_entry *element;
    const xmlChar *p;
    BSTR uri, local;
    HRESULT hr;

    update_position(This, FALSE);
    p = This->pParserCtxt->input->cur;

    if (This->saxreader->version >= MSXML4)
    {
        p--;
        while(p>This->pParserCtxt->input->base && *p!='>')
        {
            if(*p=='\n' || (*p=='\r' && *(p+1)!='\n'))
                This->line--;
            p--;
        }
    }
    else if(*(p-1)!='>' || *(p-2)!='/')
    {
        p--;
        while(p-2>=This->pParserCtxt->input->base
                && *(p-2)!='<' && *(p-1)!='/')
        {
            if(*p=='\n' || (*p=='\r' && *(p+1)!='\n'))
                This->line--;
            p--;
        }
    }
    This->column = 0;
    for(; p>=This->pParserCtxt->input->base && *p!='\n' && *p!='\r'; p--)
        This->column++;

    uri = find_element_uri(This, URI);
    element = pop_element_ns(This);

    if (!saxreader_has_handler(This, SAXContentHandler))
    {
        free_attribute_values(This);
        This->attr_count = 0;
        free_element_entry(element);
        return;
    }

    if (is_namespaces_enabled(This->saxreader))
        local = element->local;
    else
        uri = local = NULL;

    if (This->vbInterface)
        hr = IVBSAXContentHandler_endElement(
                handler->vbhandler,
                &uri, &local, &element->qname);
    else
        hr = ISAXContentHandler_endElement(
                handler->handler,
                uri, SysStringLen(uri),
                local, SysStringLen(local),
                element->qname, SysStringLen(element->qname));

    free_attribute_values(This);
    This->attr_count = 0;

    if (sax_callback_failed(This, hr))
    {
        format_error_message_from_id(This, hr);
        free_element_entry(element);
        return;
    }

    if (is_namespaces_enabled(This->saxreader))
    {
        int i = -1;
        while (iterate_endprefix_index(This, element, &i) && saxreader_has_handler(This, SAXContentHandler))
        {
            if (This->vbInterface)
                hr = IVBSAXContentHandler_endPrefixMapping(
                        handler->vbhandler, &element->ns[i].prefix);
            else
                hr = ISAXContentHandler_endPrefixMapping(
                        handler->handler, element->ns[i].prefix, SysStringLen(element->ns[i].prefix));

            if (sax_callback_failed(This, hr)) break;
       }

       if (sax_callback_failed(This, hr))
           format_error_message_from_id(This, hr);
    }

    free_element_entry(element);
}

static void libxmlCharacters(
        void *ctx,
        const xmlChar *ch,
        int len)
{
    saxlocator *This = ctx;
    BSTR Chars;
    HRESULT hr;
    xmlChar *cur, *end;
    BOOL lastEvent = FALSE;

    if (!saxreader_has_handler(This, SAXContentHandler)) return;

    update_position(This, FALSE);
    cur = (xmlChar*)This->pParserCtxt->input->cur;
    while(cur>=This->pParserCtxt->input->base && *cur!='>')
    {
        if(*cur=='\n' || (*cur=='\r' && *(cur+1)!='\n'))
            This->line--;
        cur--;
    }
    This->column = 1;
    for(; cur>=This->pParserCtxt->input->base && *cur!='\n' && *cur!='\r'; cur--)
        This->column++;

    cur = (xmlChar*)ch;
    if(*(ch-1)=='\r') cur--;
    end = cur;

    while(1)
    {
        while(end-ch<len && *end!='\r') end++;
        if(end-ch==len)
        {
            lastEvent = TRUE;
        }
        else
        {
            *end = '\n';
            end++;
        }

        if (This->saxreader->version >= MSXML4)
        {
            xmlChar *p;

            for(p=cur; p!=end; p++)
            {
                if(*p=='\n')
                {
                    This->line++;
                    This->column = 1;
                }
                else
                {
                    This->column++;
                }
            }

            if(!lastEvent)
                This->column = 0;
        }

        Chars = pooled_bstr_from_xmlCharN(&This->saxreader->pool, cur, end-cur);
        hr = saxreader_saxcharacters(This, Chars);

        if (sax_callback_failed(This, hr))
        {
            format_error_message_from_id(This, hr);
            return;
        }

        if (This->saxreader->version < MSXML4)
            This->column += end-cur;

        if(lastEvent)
            break;

        *(end-1) = '\r';
        if(*end == '\n')
        {
            end++;
            This->column++;
        }
        cur = end;

        if(end-ch == len) break;
    }
}

static void libxmlSetDocumentLocator(
        void *ctx,
        xmlSAXLocatorPtr loc)
{
    saxlocator *This = ctx;
    struct saxcontenthandler_iface *handler = saxreader_get_contenthandler(This->saxreader);
    HRESULT hr = S_OK;

    if (saxreader_has_handler(This, SAXContentHandler))
    {
        if(This->vbInterface)
            hr = IVBSAXContentHandler_putref_documentLocator(handler->vbhandler,
                    &This->IVBSAXLocator_iface);
        else
            hr = ISAXContentHandler_putDocumentLocator(handler->handler, &This->ISAXLocator_iface);
    }

    if(FAILED(hr))
        format_error_message_from_id(This, hr);
}

static void libxmlComment(void *ctx, const xmlChar *value)
{
    saxlocator *This = ctx;
    struct saxlexicalhandler_iface *handler = saxreader_get_lexicalhandler(This->saxreader);
    BSTR bValue;
    HRESULT hr;
    const xmlChar *p = This->pParserCtxt->input->cur;

    update_position(This, FALSE);
    while(p-4>=This->pParserCtxt->input->base
            && memcmp(p-4, "<!--", sizeof(char[4])))
    {
        if(*p=='\n' || (*p=='\r' && *(p+1)!='\n'))
            This->line--;
        p--;
    }

    This->column = 0;
    for(; p>=This->pParserCtxt->input->base && *p!='\n' && *p!='\r'; p--)
        This->column++;

    if (!saxreader_has_handler(This, SAXLexicalHandler)) return;

    bValue = pooled_bstr_from_xmlChar(&This->saxreader->pool, value);

    if (This->vbInterface)
        hr = IVBSAXLexicalHandler_comment(handler->vbhandler, &bValue);
    else
        hr = ISAXLexicalHandler_comment(handler->handler, bValue, SysStringLen(bValue));

    if(FAILED(hr))
        format_error_message_from_id(This, hr);
}

static void libxmlFatalError(void *ctx, const char *msg, ...)
{
    saxlocator *This = ctx;
    struct saxerrorhandler_iface *handler = saxreader_get_errorhandler(This->saxreader);
    char message[1024];
    WCHAR *error;
    DWORD len;
    va_list args;

    if(This->ret != S_OK) {
        xmlStopParser(This->pParserCtxt);
        return;
    }

    va_start(args, msg);
    vsprintf(message, msg, args);
    va_end(args);

    len = MultiByteToWideChar(CP_UNIXCP, 0, message, -1, NULL, 0);
    error = heap_alloc(sizeof(WCHAR)*len);
    if(error)
    {
        MultiByteToWideChar(CP_UNIXCP, 0, message, -1, error, len);
        TRACE("fatal error for %p: %s\n", This, debugstr_w(error));
    }

    if (!saxreader_has_handler(This, SAXErrorHandler))
    {
        xmlStopParser(This->pParserCtxt);
        This->ret = E_FAIL;
        heap_free(error);
        return;
    }

    FIXME("Error handling is not compatible.\n");

    if(This->vbInterface)
    {
        BSTR bstrError = SysAllocString(error);
        IVBSAXErrorHandler_fatalError(handler->vbhandler, &This->IVBSAXLocator_iface,
                &bstrError, E_FAIL);
        SysFreeString(bstrError);
    }
    else
        ISAXErrorHandler_fatalError(handler->handler, &This->ISAXLocator_iface, error, E_FAIL);

    heap_free(error);

    xmlStopParser(This->pParserCtxt);
    This->ret = E_FAIL;
}

/* The only reason this helper exists is that CDATA section are reported by chunks,
   newlines are used as delimiter. More than that, reader even alters input data before reporting.

   This helper should be called for substring with trailing newlines.
*/
static BSTR saxreader_get_cdata_chunk(const xmlChar *str, int len)
{
    BSTR bstr = bstr_from_xmlCharN(str, len), ret;
    WCHAR *ptr;

    ptr = bstr + len - 1;
    while ((*ptr == '\r' || *ptr == '\n') && ptr >= bstr)
        ptr--;

    while (*++ptr)
    {
        /* replace returns as:

           - "\r<char>" -> "\n<char>"
           - "\r\r" -> "\r"
           - "\r\n" -> "\n"
        */
        if (*ptr == '\r')
        {
            if (*(ptr+1) == '\r' || *(ptr+1) == '\n')
            {
                /* shift tail */
                memmove(ptr, ptr+1, len-- - (ptr-bstr));
            }
            else
                *ptr = '\n';
        }
    }

    ret = SysAllocStringLen(bstr, len);
    SysFreeString(bstr);
    return ret;
}

static void libxml_cdatablock(void *ctx, const xmlChar *value, int len)
{
    const xmlChar *start, *end;
    saxlocator *locator = ctx;
    struct saxlexicalhandler_iface *lexical = saxreader_get_lexicalhandler(locator->saxreader);
    HRESULT hr = S_OK;
    BSTR chars;
    int i;

    update_position(locator, FALSE);
    if (saxreader_has_handler(locator, SAXLexicalHandler))
    {
       if (locator->vbInterface)
           hr = IVBSAXLexicalHandler_startCDATA(lexical->vbhandler);
       else
           hr = ISAXLexicalHandler_startCDATA(lexical->handler);
    }

    if(FAILED(hr))
    {
        format_error_message_from_id(locator, hr);
        return;
    }

    start = value;
    end = NULL;
    i = 0;

    while (i < len)
    {
        /* scan for newlines */
        if (value[i] == '\r' || value[i] == '\n')
        {
            /* skip newlines/linefeeds */
            while (i < len)
            {
                if (value[i] != '\r' && value[i] != '\n') break;
                    i++;
            }
            end = &value[i];

            /* report */
            chars = saxreader_get_cdata_chunk(start, end-start);
            TRACE("(chunk %s)\n", debugstr_w(chars));
            hr = saxreader_saxcharacters(locator, chars);
            SysFreeString(chars);

            start = &value[i];
            end = NULL;
        }
        i++;
        locator->column++;
    }

    /* no newline chars (or last chunk) report as a whole */
    if (!end && start == value)
    {
        /* report */
        chars = bstr_from_xmlCharN(start, len-(start-value));
        TRACE("(%s)\n", debugstr_w(chars));
        hr = saxreader_saxcharacters(locator, chars);
        SysFreeString(chars);
    }

    if (saxreader_has_handler(locator, SAXLexicalHandler))
    {
        if (locator->vbInterface)
            hr = IVBSAXLexicalHandler_endCDATA(lexical->vbhandler);
        else
            hr = ISAXLexicalHandler_endCDATA(lexical->handler);
    }

    if(FAILED(hr))
        format_error_message_from_id(locator, hr);
}

static xmlParserInputPtr libxmlresolveentity(void *ctx, const xmlChar *publicid, const xmlChar *systemid)
{
    FIXME("entity resolving not implemented, %s, %s\n", publicid, systemid);
    return xmlSAX2ResolveEntity(ctx, publicid, systemid);
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
    else if ( IsEqualGUID( riid, &IID_IVBSAXAttributes ))
    {
        *ppvObject = &This->IVBSAXAttributes_iface;
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
    return ISAXLocator_AddRef(&This->ISAXLocator_iface);
}

static ULONG WINAPI ivbsaxlocator_Release(IVBSAXLocator* iface)
{
    saxlocator *This = impl_from_IVBSAXLocator( iface );
    return ISAXLocator_Release(&This->ISAXLocator_iface);
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

    TRACE("(%p)->(%u %u %p)\n", This, iTInfo, lcid, ppTInfo);

    return get_typeinfo(IVBSAXLocator_tid, ppTInfo);
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
        hr = ITypeInfo_Invoke(typeinfo, &This->IVBSAXLocator_iface, dispIdMember, wFlags,
                pDispParams, pVarResult, pExcepInfo, puArgErr);
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
    return ISAXLocator_getColumnNumber(&This->ISAXLocator_iface, pnColumn);
}

static HRESULT WINAPI ivbsaxlocator_get_lineNumber(
        IVBSAXLocator* iface,
        int *pnLine)
{
    saxlocator *This = impl_from_IVBSAXLocator( iface );
    return ISAXLocator_getLineNumber(&This->ISAXLocator_iface, pnLine);
}

static HRESULT WINAPI ivbsaxlocator_get_publicId(IVBSAXLocator* iface, BSTR *ret)
{
    saxlocator *This = impl_from_IVBSAXLocator( iface );
    const WCHAR *publicidW;
    HRESULT hr;

    TRACE("(%p)->(%p)\n", This, ret);

    if (!ret)
        return E_POINTER;

    *ret = NULL;
    hr = ISAXLocator_getPublicId(&This->ISAXLocator_iface, &publicidW);
    if (FAILED(hr))
        return hr;

    return return_bstr(publicidW, ret);
}

static HRESULT WINAPI ivbsaxlocator_get_systemId(IVBSAXLocator* iface, BSTR *ret)
{
    saxlocator *This = impl_from_IVBSAXLocator( iface );
    const WCHAR *systemidW;
    HRESULT hr;

    TRACE("(%p)->(%p)\n", This, ret);

    if (!ret)
        return E_POINTER;

    *ret = NULL;
    hr = ISAXLocator_getSystemId(&This->ISAXLocator_iface, &systemidW);
    if (FAILED(hr))
        return hr;

    return return_bstr(systemidW, ret);
}

static const struct IVBSAXLocatorVtbl VBSAXLocatorVtbl =
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
    else if ( IsEqualGUID( riid, &IID_ISAXAttributes ))
    {
        *ppvObject = &This->ISAXAttributes_iface;
    }
    else
    {
        WARN("interface %s not implemented\n", debugstr_guid(riid));
        return E_NOINTERFACE;
    }

    ISAXLocator_AddRef( iface );

    return S_OK;
}

static ULONG WINAPI isaxlocator_AddRef(ISAXLocator* iface)
{
    saxlocator *This = impl_from_ISAXLocator( iface );
    ULONG ref = InterlockedIncrement( &This->ref );
    TRACE("(%p)->(%d)\n", This, ref);
    return ref;
}

static ULONG WINAPI isaxlocator_Release(
        ISAXLocator* iface)
{
    saxlocator *This = impl_from_ISAXLocator( iface );
    LONG ref = InterlockedDecrement( &This->ref );

    TRACE("(%p)->(%d)\n", This, ref );

    if (ref == 0)
    {
        element_entry *element, *element2;
        int index;

        SysFreeString(This->publicId);
        SysFreeString(This->systemId);
        SysFreeString(This->namespaceUri);

        for(index = 0; index < This->attr_alloc_count; index++)
        {
            SysFreeString(This->attributes[index].szLocalname);
            SysFreeString(This->attributes[index].szValue);
            SysFreeString(This->attributes[index].szQName);
        }
        heap_free(This->attributes);

        /* element stack */
        LIST_FOR_EACH_ENTRY_SAFE(element, element2, &This->elements, element_entry, entry)
        {
            list_remove(&element->entry);
            free_element_entry(element);
        }

        ISAXXMLReader_Release(&This->saxreader->ISAXXMLReader_iface);
        heap_free( This );
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
        This->publicId = publicId;
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
        This->systemId = systemId;
    else
    {
        SysFreeString(systemId);
        This->systemId = NULL;
    }

    *ppwchSystemId = This->systemId;
    return S_OK;
}

static const struct ISAXLocatorVtbl SAXLocatorVtbl =
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
    static const WCHAR w3xmlns[] = { 'h','t','t','p',':','/','/', 'w','w','w','.','w','3','.',
        'o','r','g','/','2','0','0','0','/','x','m','l','n','s','/',0 };

    saxlocator *locator;

    locator = heap_alloc( sizeof (*locator) );
    if( !locator )
        return E_OUTOFMEMORY;

    locator->IVBSAXLocator_iface.lpVtbl = &VBSAXLocatorVtbl;
    locator->ISAXLocator_iface.lpVtbl = &SAXLocatorVtbl;
    locator->IVBSAXAttributes_iface.lpVtbl = &ivbsaxattributes_vtbl;
    locator->ISAXAttributes_iface.lpVtbl = &isaxattributes_vtbl;
    locator->ref = 1;
    locator->vbInterface = vbInterface;

    locator->saxreader = reader;
    ISAXXMLReader_AddRef(&reader->ISAXXMLReader_iface);

    locator->pParserCtxt = NULL;
    locator->publicId = NULL;
    locator->systemId = NULL;
    locator->line = reader->version < MSXML4 ? 0 : 1;
    locator->column = 0;
    locator->ret = S_OK;
    if (locator->saxreader->version >= MSXML6)
        locator->namespaceUri = SysAllocString(w3xmlns);
    else
        locator->namespaceUri = SysAllocStringLen(NULL, 0);
    if(!locator->namespaceUri)
    {
        ISAXXMLReader_Release(&reader->ISAXXMLReader_iface);
        heap_free(locator);
        return E_OUTOFMEMORY;
    }

    locator->attr_alloc_count = 8;
    locator->attr_count = 0;
    locator->attributes = heap_alloc_zero(sizeof(struct _attributes)*locator->attr_alloc_count);
    if(!locator->attributes)
    {
        ISAXXMLReader_Release(&reader->ISAXXMLReader_iface);
        SysFreeString(locator->namespaceUri);
        heap_free(locator);
        return E_OUTOFMEMORY;
    }

    list_init(&locator->elements);

    *ppsaxlocator = locator;

    TRACE("returning %p\n", *ppsaxlocator);

    return S_OK;
}

/*** SAXXMLReader internal functions ***/
static HRESULT internal_parseBuffer(saxreader *This, const char *buffer, int size, BOOL vbInterface)
{
    xmlCharEncoding encoding = XML_CHAR_ENCODING_NONE;
    xmlChar *enc_name = NULL;
    saxlocator *locator;
    HRESULT hr;

    TRACE("(%p)->(%p %d)\n", This, buffer, size);

    hr = SAXLocator_create(This, &locator, vbInterface);
    if (FAILED(hr))
        return hr;

    if (size >= 4)
    {
        const unsigned char *buff = (unsigned char*)buffer;

        encoding = xmlDetectCharEncoding((xmlChar*)buffer, 4);
        enc_name = (xmlChar*)xmlGetCharEncodingName(encoding);
        TRACE("detected encoding: %s\n", enc_name);
        /* skip BOM, parser won't switch encodings and so won't skip it on its own */
        if ((encoding == XML_CHAR_ENCODING_UTF8) &&
            buff[0] == 0xEF && buff[1] == 0xBB && buff[2] == 0xBF)
        {
            buffer += 3;
            size -= 3;
        }
    }

    /* if libxml2 detection failed try to guess */
    if (encoding == XML_CHAR_ENCODING_NONE)
    {
        const WCHAR *ptr = (WCHAR*)buffer;
        /* xml declaration with possibly specfied encoding will be still handled by parser */
        if ((size >= 2) && *ptr == '<' && ptr[1] != '?')
        {
            enc_name = (xmlChar*)xmlGetCharEncodingName(XML_CHAR_ENCODING_UTF16LE);
            encoding = XML_CHAR_ENCODING_UTF16LE;
        }
    }
    else if (encoding == XML_CHAR_ENCODING_UTF8)
        enc_name = (xmlChar*)xmlGetCharEncodingName(encoding);
    else
        enc_name = NULL;

    locator->pParserCtxt = xmlCreateMemoryParserCtxt(buffer, size);
    if (!locator->pParserCtxt)
    {
        ISAXLocator_Release(&locator->ISAXLocator_iface);
        return E_FAIL;
    }

    if (enc_name)
    {
        locator->pParserCtxt->encoding = xmlStrdup(enc_name);
        if (encoding == XML_CHAR_ENCODING_UTF16LE) {
            TRACE("switching to %s\n", enc_name);
            xmlSwitchEncoding(locator->pParserCtxt, encoding);
        }
    }

    xmlFree(locator->pParserCtxt->sax);
    locator->pParserCtxt->sax = &locator->saxreader->sax;
    locator->pParserCtxt->userData = locator;

    This->isParsing = TRUE;
    if(xmlParseDocument(locator->pParserCtxt) == -1 && locator->ret == S_OK)
        hr = E_FAIL;
    else
        hr = locator->ret;
    This->isParsing = FALSE;

    if(locator->pParserCtxt)
    {
        locator->pParserCtxt->sax = NULL;
        xmlFreeParserCtxt(locator->pParserCtxt);
        locator->pParserCtxt = NULL;
    }

    ISAXLocator_Release(&locator->ISAXLocator_iface);
    return hr;
}

static HRESULT internal_parseStream(saxreader *This, ISequentialStream *stream, BOOL vbInterface)
{
    saxlocator *locator;
    HRESULT hr;
    ULONG dataRead;
    char data[2048];
    int ret;

    dataRead = 0;
    hr = ISequentialStream_Read(stream, data, sizeof(data), &dataRead);
    if(FAILED(hr)) return hr;

    hr = SAXLocator_create(This, &locator, vbInterface);
    if(FAILED(hr)) return hr;

    locator->pParserCtxt = xmlCreatePushParserCtxt(
            &locator->saxreader->sax, locator,
            data, dataRead, NULL);
    if(!locator->pParserCtxt)
    {
        ISAXLocator_Release(&locator->ISAXLocator_iface);
        return E_FAIL;
    }

    This->isParsing = TRUE;

    do {
        dataRead = 0;
        hr = ISequentialStream_Read(stream, data, sizeof(data), &dataRead);
        if (FAILED(hr) || !dataRead) break;

        ret = xmlParseChunk(locator->pParserCtxt, data, dataRead, 0);
        hr = ret!=XML_ERR_OK && locator->ret==S_OK ? E_FAIL : locator->ret;
    }while(hr == S_OK);

    if(SUCCEEDED(hr))
    {
        ret = xmlParseChunk(locator->pParserCtxt, data, 0, 1);
        hr = ret!=XML_ERR_OK && locator->ret==S_OK ? E_FAIL : locator->ret;
    }


    This->isParsing = FALSE;

    xmlFreeParserCtxt(locator->pParserCtxt);
    locator->pParserCtxt = NULL;
    ISAXLocator_Release(&locator->ISAXLocator_iface);
    return hr;
}

static HRESULT internal_parse(
        saxreader* This,
        VARIANT varInput,
        BOOL vbInterface)
{
    HRESULT hr;

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&varInput));

    /* Dispose of the BSTRs in the pool from a prior run, if any. */
    free_bstr_pool(&This->pool);

    switch(V_VT(&varInput))
    {
        case VT_BSTR:
        case VT_BSTR|VT_BYREF:
        {
            BSTR str = V_ISBYREF(&varInput) ? *V_BSTRREF(&varInput) : V_BSTR(&varInput);
            hr = internal_parseBuffer(This, (const char*)str, strlenW(str)*sizeof(WCHAR), vbInterface);
            break;
        }
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
            ISequentialStream *stream = NULL;
            IXMLDOMDocument *xmlDoc;

            if(IUnknown_QueryInterface(V_UNKNOWN(&varInput),
                        &IID_IXMLDOMDocument, (void**)&xmlDoc) == S_OK)
            {
                BSTR bstrData;

                IXMLDOMDocument_get_xml(xmlDoc, &bstrData);
                hr = internal_parseBuffer(This, (const char*)bstrData,
                        SysStringByteLen(bstrData), vbInterface);
                IXMLDOMDocument_Release(xmlDoc);
                SysFreeString(bstrData);
                break;
            }

            if(IUnknown_QueryInterface(V_UNKNOWN(&varInput),
                        &IID_IPersistStream, (void**)&persistStream) == S_OK)
            {
                IStream *stream_copy;

                hr = CreateStreamOnHGlobal(NULL, TRUE, &stream_copy);
                if(hr != S_OK)
                {
                    IPersistStream_Release(persistStream);
                    return hr;
                }

                hr = IPersistStream_Save(persistStream, stream_copy, TRUE);
                IPersistStream_Release(persistStream);
                if(hr == S_OK)
                    IStream_QueryInterface(stream_copy, &IID_ISequentialStream, (void**)&stream);

                IStream_Release(stream_copy);
            }

            /* try base interface first */
            if(!stream)
            {
                IUnknown_QueryInterface(V_UNKNOWN(&varInput), &IID_ISequentialStream, (void**)&stream);
                if (!stream)
                    /* this should never happen if IStream is implemented properly, but just in case */
                    IUnknown_QueryInterface(V_UNKNOWN(&varInput), &IID_IStream, (void**)&stream);
            }

            if(stream)
            {
                hr = internal_parseStream(This, stream, vbInterface);
                ISequentialStream_Release(stream);
            }
            else
            {
                WARN("IUnknown* input doesn't support any of expected interfaces\n");
                hr = E_INVALIDARG;
            }

            break;
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
    IMoniker *mon;
    bsc_t *bsc;
    HRESULT hr;

    TRACE("(%p)->(%s)\n", This, debugstr_w(url));

    hr = create_moniker_from_url(url, &mon);
    if(FAILED(hr))
        return hr;

    if(vbInterface) hr = bind_url(mon, internal_vbonDataAvailable, This, &bsc);
    else hr = bind_url(mon, internal_onDataAvailable, This, &bsc);
    IMoniker_Release(mon);

    if(FAILED(hr))
        return hr;

    return detach_bsc(bsc);
}

static HRESULT saxreader_put_handler_from_variant(saxreader *This, enum saxhandler_type type, const VARIANT *v, BOOL vb)
{
    const IID *riid;

    if (V_VT(v) == VT_EMPTY)
        return saxreader_put_handler(This, type, NULL, vb);

    switch (type)
    {
    case SAXDeclHandler:
        riid = vb ? &IID_IVBSAXDeclHandler : &IID_ISAXDeclHandler;
        break;
    case SAXLexicalHandler:
        riid = vb ? &IID_IVBSAXLexicalHandler : &IID_ISAXLexicalHandler;
        break;
    default:
        ERR("wrong handler type %d\n", type);
        return E_FAIL;
    }

    switch (V_VT(v))
    {
    case VT_DISPATCH:
    case VT_UNKNOWN:
    {
        IUnknown *handler = NULL;

        if (V_UNKNOWN(v))
        {
            HRESULT hr = IUnknown_QueryInterface(V_UNKNOWN(v), riid, (void**)&handler);
            if (FAILED(hr)) return hr;
        }

        saxreader_put_handler(This, type, handler, vb);
        if (handler) IUnknown_Release(handler);
        break;
    }
    default:
        ERR("value type %d not supported\n", V_VT(v));
        return E_INVALIDARG;
    }

    return S_OK;
}

static HRESULT internal_putProperty(
    saxreader* This,
    const WCHAR *prop,
    VARIANT value,
    BOOL vbInterface)
{
    VARIANT *v;

    TRACE("(%p)->(%s %s)\n", This, debugstr_w(prop), debugstr_variant(&value));

    if (This->isParsing) return E_FAIL;

    v = V_VT(&value) == (VT_VARIANT|VT_BYREF) ? V_VARIANTREF(&value) : &value;
    if(!memcmp(prop, PropertyDeclHandlerW, sizeof(PropertyDeclHandlerW)))
        return saxreader_put_handler_from_variant(This, SAXDeclHandler, v, vbInterface);

    if(!memcmp(prop, PropertyLexicalHandlerW, sizeof(PropertyLexicalHandlerW)))
        return saxreader_put_handler_from_variant(This, SAXLexicalHandler, v, vbInterface);

    if(!memcmp(prop, PropertyMaxXMLSizeW, sizeof(PropertyMaxXMLSizeW)))
    {
        if (V_VT(v) == VT_I4 && V_I4(v) == 0) return S_OK;
        FIXME("(%p)->(%s): max-xml-size unsupported\n", This, debugstr_variant(v));
        return E_NOTIMPL;
    }

    if(!memcmp(prop, PropertyMaxElementDepthW, sizeof(PropertyMaxElementDepthW)))
    {
        if (V_VT(v) == VT_I4 && V_I4(v) == 0) return S_OK;
        FIXME("(%p)->(%s): max-element-depth unsupported\n", This, debugstr_variant(v));
        return E_NOTIMPL;
    }

    FIXME("(%p)->(%s:%s): unsupported property\n", This, debugstr_w(prop), debugstr_variant(v));

    if(!memcmp(prop, PropertyCharsetW, sizeof(PropertyCharsetW)))
        return E_NOTIMPL;

    if(!memcmp(prop, PropertyDomNodeW, sizeof(PropertyDomNodeW)))
        return E_FAIL;

    if(!memcmp(prop, PropertyInputSourceW, sizeof(PropertyInputSourceW)))
        return E_NOTIMPL;

    if(!memcmp(prop, PropertySchemaDeclHandlerW, sizeof(PropertySchemaDeclHandlerW)))
        return E_NOTIMPL;

    if(!memcmp(prop, PropertyXMLDeclEncodingW, sizeof(PropertyXMLDeclEncodingW)))
        return E_FAIL;

    if(!memcmp(prop, PropertyXMLDeclStandaloneW, sizeof(PropertyXMLDeclStandaloneW)))
        return E_FAIL;

    if(!memcmp(prop, PropertyXMLDeclVersionW, sizeof(PropertyXMLDeclVersionW)))
        return E_FAIL;

    return E_INVALIDARG;
}

static HRESULT internal_getProperty(const saxreader* This, const WCHAR *prop, VARIANT *value, BOOL vb)
{
    TRACE("(%p)->(%s)\n", This, debugstr_w(prop));

    if (!value) return E_POINTER;

    if (!memcmp(PropertyLexicalHandlerW, prop, sizeof(PropertyLexicalHandlerW)))
    {
        V_VT(value) = VT_UNKNOWN;
        saxreader_get_handler(This, SAXLexicalHandler, vb, (void**)&V_UNKNOWN(value));
        return S_OK;
    }

    if (!memcmp(PropertyDeclHandlerW, prop, sizeof(PropertyDeclHandlerW)))
    {
        V_VT(value) = VT_UNKNOWN;
        saxreader_get_handler(This, SAXDeclHandler, vb, (void**)&V_UNKNOWN(value));
        return S_OK;
    }

    if (!memcmp(PropertyXmlDeclVersionW, prop, sizeof(PropertyXmlDeclVersionW)))
    {
        V_VT(value) = VT_BSTR;
        V_BSTR(value) = SysAllocString(This->xmldecl_version);
        return S_OK;
    }

    FIXME("(%p)->(%s) unsupported property\n", This, debugstr_w(prop));

    return E_NOTIMPL;
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
        *ppvObject = &This->ISAXXMLReader_iface;
    }
    else if (dispex_query_interface(&This->dispex, riid, ppvObject))
    {
        return *ppvObject ? S_OK : E_NOINTERFACE;
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
        int i;

        for (i = 0; i < SAXHandler_Last; i++)
        {
            struct saxanyhandler_iface *saxiface = &This->saxhandlers[i].u.anyhandler;

            if (saxiface->handler)
                IUnknown_Release(saxiface->handler);

            if (saxiface->vbhandler)
                IUnknown_Release(saxiface->vbhandler);
        }

        SysFreeString(This->xmldecl_version);
        free_bstr_pool(&This->pool);

        heap_free( This );
    }

    return ref;
}
/*** IDispatch ***/
static HRESULT WINAPI saxxmlreader_GetTypeInfoCount( IVBSAXXMLReader *iface, UINT* pctinfo )
{
    saxreader *This = impl_from_IVBSAXXMLReader( iface );
    return IDispatchEx_GetTypeInfoCount(&This->dispex.IDispatchEx_iface, pctinfo);
}

static HRESULT WINAPI saxxmlreader_GetTypeInfo(
    IVBSAXXMLReader *iface,
    UINT iTInfo, LCID lcid, ITypeInfo** ppTInfo )
{
    saxreader *This = impl_from_IVBSAXXMLReader( iface );
    return IDispatchEx_GetTypeInfo(&This->dispex.IDispatchEx_iface,
        iTInfo, lcid, ppTInfo);
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
    return IDispatchEx_GetIDsOfNames(&This->dispex.IDispatchEx_iface,
        riid, rgszNames, cNames, lcid, rgDispId);
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
    return IDispatchEx_Invoke(&This->dispex.IDispatchEx_iface,
        dispIdMember, riid, lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
}

/*** IVBSAXXMLReader methods ***/
static HRESULT WINAPI saxxmlreader_getFeature(
    IVBSAXXMLReader* iface,
    BSTR feature_name,
    VARIANT_BOOL *value)
{
    saxreader *This = impl_from_IVBSAXXMLReader( iface );
    return ISAXXMLReader_getFeature(&This->ISAXXMLReader_iface, feature_name, value);
}

static HRESULT WINAPI saxxmlreader_putFeature(
    IVBSAXXMLReader* iface,
    BSTR feature_name,
    VARIANT_BOOL value)
{
    saxreader *This = impl_from_IVBSAXXMLReader( iface );
    return ISAXXMLReader_putFeature(&This->ISAXXMLReader_iface, feature_name, value);
}

static HRESULT WINAPI saxxmlreader_getProperty(
    IVBSAXXMLReader* iface,
    BSTR prop,
    VARIANT *value)
{
    saxreader *This = impl_from_IVBSAXXMLReader( iface );
    return internal_getProperty(This, prop, value, TRUE);
}

static HRESULT WINAPI saxxmlreader_putProperty(
    IVBSAXXMLReader* iface,
    BSTR pProp,
    VARIANT value)
{
    saxreader *This = impl_from_IVBSAXXMLReader( iface );
    return internal_putProperty(This, pProp, value, TRUE);
}

static HRESULT WINAPI saxxmlreader_get_entityResolver(
    IVBSAXXMLReader* iface,
    IVBSAXEntityResolver **resolver)
{
    saxreader *This = impl_from_IVBSAXXMLReader( iface );
    return saxreader_get_handler(This, SAXEntityResolver, TRUE, (void**)resolver);
}

static HRESULT WINAPI saxxmlreader_put_entityResolver(
    IVBSAXXMLReader* iface,
    IVBSAXEntityResolver *resolver)
{
    saxreader *This = impl_from_IVBSAXXMLReader( iface );
    return saxreader_put_handler(This, SAXEntityResolver, resolver, TRUE);
}

static HRESULT WINAPI saxxmlreader_get_contentHandler(
    IVBSAXXMLReader* iface,
    IVBSAXContentHandler **handler)
{
    saxreader *This = impl_from_IVBSAXXMLReader( iface );
    return saxreader_get_handler(This, SAXContentHandler, TRUE, (void**)handler);
}

static HRESULT WINAPI saxxmlreader_put_contentHandler(
    IVBSAXXMLReader* iface,
    IVBSAXContentHandler *handler)
{
    saxreader *This = impl_from_IVBSAXXMLReader( iface );
    return saxreader_put_handler(This, SAXContentHandler, handler, TRUE);
}

static HRESULT WINAPI saxxmlreader_get_dtdHandler(
    IVBSAXXMLReader* iface,
    IVBSAXDTDHandler **handler)
{
    saxreader *This = impl_from_IVBSAXXMLReader( iface );
    return saxreader_get_handler(This, SAXDTDHandler, TRUE, (void**)handler);
}

static HRESULT WINAPI saxxmlreader_put_dtdHandler(
    IVBSAXXMLReader* iface,
    IVBSAXDTDHandler *handler)
{
    saxreader *This = impl_from_IVBSAXXMLReader( iface );
    return saxreader_put_handler(This, SAXDTDHandler, handler, TRUE);
}

static HRESULT WINAPI saxxmlreader_get_errorHandler(
    IVBSAXXMLReader* iface,
    IVBSAXErrorHandler **handler)
{
    saxreader *This = impl_from_IVBSAXXMLReader( iface );
    return saxreader_get_handler(This, SAXErrorHandler, TRUE, (void**)handler);
}

static HRESULT WINAPI saxxmlreader_put_errorHandler(
    IVBSAXXMLReader* iface,
    IVBSAXErrorHandler *handler)
{
    saxreader *This = impl_from_IVBSAXXMLReader( iface );
    return saxreader_put_handler(This, SAXErrorHandler, handler, TRUE);
}

static HRESULT WINAPI saxxmlreader_get_baseURL(
    IVBSAXXMLReader* iface,
    BSTR *pBaseUrl)
{
    saxreader *This = impl_from_IVBSAXXMLReader( iface );

    FIXME("(%p)->(%p) stub\n", This, pBaseUrl);
    return E_NOTIMPL;
}

static HRESULT WINAPI saxxmlreader_put_baseURL(
    IVBSAXXMLReader* iface,
    BSTR pBaseUrl)
{
    saxreader *This = impl_from_IVBSAXXMLReader( iface );
    return ISAXXMLReader_putBaseURL(&This->ISAXXMLReader_iface, pBaseUrl);
}

static HRESULT WINAPI saxxmlreader_get_secureBaseURL(
    IVBSAXXMLReader* iface,
    BSTR *pSecureBaseUrl)
{
    saxreader *This = impl_from_IVBSAXXMLReader( iface );

    FIXME("(%p)->(%p) stub\n", This, pSecureBaseUrl);
    return E_NOTIMPL;
}

static HRESULT WINAPI saxxmlreader_put_secureBaseURL(
    IVBSAXXMLReader* iface,
    BSTR secureBaseUrl)
{
    saxreader *This = impl_from_IVBSAXXMLReader( iface );
    return ISAXXMLReader_putSecureBaseURL(&This->ISAXXMLReader_iface, secureBaseUrl);
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
    BSTR url)
{
    saxreader *This = impl_from_IVBSAXXMLReader( iface );
    return internal_parseURL(This, url, TRUE);
}

static const struct IVBSAXXMLReaderVtbl VBSAXXMLReaderVtbl =
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
    return IVBSAXXMLReader_QueryInterface(&This->IVBSAXXMLReader_iface, riid, ppvObject);
}

static ULONG WINAPI isaxxmlreader_AddRef(ISAXXMLReader* iface)
{
    saxreader *This = impl_from_ISAXXMLReader( iface );
    return IVBSAXXMLReader_AddRef(&This->IVBSAXXMLReader_iface);
}

static ULONG WINAPI isaxxmlreader_Release(ISAXXMLReader* iface)
{
    saxreader *This = impl_from_ISAXXMLReader( iface );
    return IVBSAXXMLReader_Release(&This->IVBSAXXMLReader_iface);
}

/*** ISAXXMLReader methods ***/
static HRESULT WINAPI isaxxmlreader_getFeature(
        ISAXXMLReader* iface,
        const WCHAR *feature_name,
        VARIANT_BOOL *value)
{
    saxreader *This = impl_from_ISAXXMLReader( iface );
    saxreader_feature feature;

    TRACE("(%p)->(%s %p)\n", This, debugstr_w(feature_name), value);

    feature = get_saxreader_feature(feature_name);
    if (feature == Namespaces || feature == NamespacePrefixes)
        return get_feature_value(This, feature, value);

    FIXME("(%p)->(%s %p) stub\n", This, debugstr_w(feature_name), value);
    return E_NOTIMPL;
}

static HRESULT WINAPI isaxxmlreader_putFeature(
        ISAXXMLReader* iface,
        const WCHAR *feature_name,
        VARIANT_BOOL value)
{
    saxreader *This = impl_from_ISAXXMLReader( iface );
    saxreader_feature feature;

    TRACE("(%p)->(%s %x)\n", This, debugstr_w(feature_name), value);

    feature = get_saxreader_feature(feature_name);

    /* accepted cases */
    if ((feature == ExternalGeneralEntities   && value == VARIANT_FALSE) ||
        (feature == ExternalParameterEntities && value == VARIANT_FALSE) ||
         feature == Namespaces ||
         feature == NamespacePrefixes)
    {
        return set_feature_value(This, feature, value);
    }

    if (feature == LexicalHandlerParEntities || feature == ProhibitDTD)
    {
        FIXME("(%p)->(%s %x) stub\n", This, debugstr_w(feature_name), value);
        return set_feature_value(This, feature, value);
    }

    FIXME("(%p)->(%s %x) stub\n", This, debugstr_w(feature_name), value);
    return E_NOTIMPL;
}

static HRESULT WINAPI isaxxmlreader_getProperty(
        ISAXXMLReader* iface,
        const WCHAR *prop,
        VARIANT *value)
{
    saxreader *This = impl_from_ISAXXMLReader( iface );
    return internal_getProperty(This, prop, value, FALSE);
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
        ISAXEntityResolver **resolver)
{
    saxreader *This = impl_from_ISAXXMLReader( iface );
    return saxreader_get_handler(This, SAXEntityResolver, FALSE, (void**)resolver);
}

static HRESULT WINAPI isaxxmlreader_putEntityResolver(
        ISAXXMLReader* iface,
        ISAXEntityResolver *resolver)
{
    saxreader *This = impl_from_ISAXXMLReader( iface );
    return saxreader_put_handler(This, SAXEntityResolver, resolver, FALSE);
}

static HRESULT WINAPI isaxxmlreader_getContentHandler(
        ISAXXMLReader* iface,
        ISAXContentHandler **handler)
{
    saxreader *This = impl_from_ISAXXMLReader( iface );
    return saxreader_get_handler(This, SAXContentHandler, FALSE, (void**)handler);
}

static HRESULT WINAPI isaxxmlreader_putContentHandler(
    ISAXXMLReader* iface,
    ISAXContentHandler *handler)
{
    saxreader *This = impl_from_ISAXXMLReader( iface );
    return saxreader_put_handler(This, SAXContentHandler, handler, FALSE);
}

static HRESULT WINAPI isaxxmlreader_getDTDHandler(
        ISAXXMLReader* iface,
        ISAXDTDHandler **handler)
{
    saxreader *This = impl_from_ISAXXMLReader( iface );
    return saxreader_get_handler(This, SAXDTDHandler, FALSE, (void**)handler);
}

static HRESULT WINAPI isaxxmlreader_putDTDHandler(
        ISAXXMLReader* iface,
        ISAXDTDHandler *handler)
{
    saxreader *This = impl_from_ISAXXMLReader( iface );
    return saxreader_put_handler(This, SAXDTDHandler, handler, FALSE);
}

static HRESULT WINAPI isaxxmlreader_getErrorHandler(
        ISAXXMLReader* iface,
        ISAXErrorHandler **handler)
{
    saxreader *This = impl_from_ISAXXMLReader( iface );
    return saxreader_get_handler(This, SAXErrorHandler, FALSE, (void**)handler);
}

static HRESULT WINAPI isaxxmlreader_putErrorHandler(ISAXXMLReader* iface, ISAXErrorHandler *handler)
{
    saxreader *This = impl_from_ISAXXMLReader( iface );
    return saxreader_put_handler(This, SAXErrorHandler, handler, FALSE);
}

static HRESULT WINAPI isaxxmlreader_getBaseURL(
        ISAXXMLReader* iface,
        const WCHAR **base_url)
{
    saxreader *This = impl_from_ISAXXMLReader( iface );

    FIXME("(%p)->(%p) stub\n", This, base_url);
    return E_NOTIMPL;
}

static HRESULT WINAPI isaxxmlreader_putBaseURL(
        ISAXXMLReader* iface,
        const WCHAR *pBaseUrl)
{
    saxreader *This = impl_from_ISAXXMLReader( iface );

    FIXME("(%p)->(%s) stub\n", This, debugstr_w(pBaseUrl));
    return E_NOTIMPL;
}

static HRESULT WINAPI isaxxmlreader_getSecureBaseURL(
        ISAXXMLReader* iface,
        const WCHAR **pSecureBaseUrl)
{
    saxreader *This = impl_from_ISAXXMLReader( iface );
    FIXME("(%p)->(%p) stub\n", This, pSecureBaseUrl);
    return E_NOTIMPL;
}

static HRESULT WINAPI isaxxmlreader_putSecureBaseURL(
        ISAXXMLReader* iface,
        const WCHAR *secureBaseUrl)
{
    saxreader *This = impl_from_ISAXXMLReader( iface );

    FIXME("(%p)->(%s) stub\n", This, debugstr_w(secureBaseUrl));
    return E_NOTIMPL;
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

static const struct ISAXXMLReaderVtbl SAXXMLReaderVtbl =
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

static const tid_t saxreader_iface_tids[] = {
    IVBSAXXMLReader_tid,
    0
};
static dispex_static_data_t saxreader_dispex = {
    NULL,
    IVBSAXXMLReader_tid,
    NULL,
    saxreader_iface_tids
};

HRESULT SAXXMLReader_create(MSXML_VERSION version, LPVOID *ppObj)
{
    saxreader *reader;

    TRACE("(%p)\n", ppObj);

    reader = heap_alloc( sizeof (*reader) );
    if( !reader )
        return E_OUTOFMEMORY;

    reader->IVBSAXXMLReader_iface.lpVtbl = &VBSAXXMLReaderVtbl;
    reader->ISAXXMLReader_iface.lpVtbl = &SAXXMLReaderVtbl;
    reader->ref = 1;
    memset(reader->saxhandlers, 0, sizeof(reader->saxhandlers));
    reader->isParsing = FALSE;
    reader->xmldecl_version = NULL;
    reader->pool.pool = NULL;
    reader->pool.index = 0;
    reader->pool.len = 0;
    reader->features = Namespaces | NamespacePrefixes;
    reader->version = version;

    init_dispex(&reader->dispex, (IUnknown*)&reader->IVBSAXXMLReader_iface, &saxreader_dispex);

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
    reader->sax.cdataBlock = libxml_cdatablock;
    reader->sax.resolveEntity = libxmlresolveentity;

    *ppObj = &reader->IVBSAXXMLReader_iface;

    TRACE("returning iface %p\n", *ppObj);

    return S_OK;
}

#else

HRESULT SAXXMLReader_create(MSXML_VERSION version, LPVOID *ppObj)
{
    MESSAGE("This program tried to use a SAX XML Reader object, but\n"
            "libxml2 support was not present at compile time.\n");
    return E_NOTIMPL;
}

#endif
