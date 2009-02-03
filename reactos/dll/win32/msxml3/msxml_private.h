/*
 *    MSXML Class Factory
 *
 * Copyright 2005 Mike McCormack
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

#ifndef __MSXML_PRIVATE__
#define __MSXML_PRIVATE__

#include "dispex.h"

#include "wine/unicode.h"

#ifndef __WINE_CONFIG_H
# error You must include config.h to use this header
#endif

#ifdef HAVE_LIBXML2

#ifdef HAVE_LIBXML_PARSER_H
#include <libxml/parser.h>
#endif

/* constructors */
extern IUnknown         *create_domdoc( xmlNodePtr document );
extern IUnknown         *create_xmldoc( void );
extern IXMLDOMNode      *create_node( xmlNodePtr node );
extern IUnknown         *create_basic_node( xmlNodePtr node, IUnknown *pUnkOuter );
extern IUnknown         *create_element( xmlNodePtr element, IUnknown *pUnkOuter );
extern IUnknown         *create_attribute( xmlNodePtr attribute );
extern IUnknown         *create_text( xmlNodePtr text );
extern IUnknown         *create_pi( xmlNodePtr pi );
extern IUnknown         *create_comment( xmlNodePtr comment );
extern IUnknown         *create_cdata( xmlNodePtr text );
extern IXMLDOMNodeList  *create_children_nodelist( xmlNodePtr );
extern IXMLDOMNamedNodeMap *create_nodemap( IXMLDOMNode *node );
extern IUnknown         *create_doc_Implementation();
extern IUnknown         *create_doc_fragment( xmlNodePtr fragment );
extern IUnknown         *create_doc_entity_ref( xmlNodePtr entity );

extern HRESULT queryresult_create( xmlNodePtr, LPWSTR, IXMLDOMNodeList ** );

/* data accessors */
xmlNodePtr xmlNodePtr_from_domnode( IXMLDOMNode *iface, xmlElementType type );

/* helpers */
extern xmlChar *xmlChar_from_wchar( LPWSTR str );
extern BSTR bstr_from_xmlChar( const xmlChar *buf );

extern LONG xmldoc_add_ref( xmlDocPtr doc );
extern LONG xmldoc_release( xmlDocPtr doc );
extern HRESULT xmldoc_add_orphan( xmlDocPtr doc, xmlNodePtr node );
extern HRESULT xmldoc_remove_orphan( xmlDocPtr doc, xmlNodePtr node );

extern HRESULT XMLElement_create( IUnknown *pUnkOuter, xmlNodePtr node, LPVOID *ppObj );

extern xmlDocPtr parse_xml(char *ptr, int len);

/* IXMLDOMNode Internal Structure */
typedef struct _xmlnode
{
    const struct IXMLDOMNodeVtbl *lpVtbl;
    const struct IUnknownVtbl *lpInternalUnkVtbl;
    IUnknown *pUnkOuter;
    LONG ref;
    xmlNodePtr node;
} xmlnode;

static inline xmlnode *impl_from_IXMLDOMNode( IXMLDOMNode *iface )
{
    return (xmlnode *)((char*)iface - FIELD_OFFSET(xmlnode, lpVtbl));
}

extern HRESULT DOMDocument_create_from_xmldoc(xmlDocPtr xmldoc, IXMLDOMDocument2 **document);

#endif

void* libxslt_handle;
#ifdef SONAME_LIBXSLT
# ifdef HAVE_LIBXSLT_PATTERN_H
#  include <libxslt/pattern.h>
# endif
# ifdef HAVE_LIBXSLT_TRANSFORM_H
#  include <libxslt/transform.h>
# endif
# include <libxslt/xsltutils.h>
# include <libxslt/xsltInternals.h>

# define MAKE_FUNCPTR(f) extern typeof(f) * p##f
MAKE_FUNCPTR(xsltApplyStylesheet);
MAKE_FUNCPTR(xsltCleanupGlobals);
MAKE_FUNCPTR(xsltFreeStylesheet);
MAKE_FUNCPTR(xsltParseStylesheetDoc);
# undef MAKE_FUNCPTR
#endif

extern IXMLDOMParseError *create_parseError( LONG code, BSTR url, BSTR reason, BSTR srcText,
                                             LONG line, LONG linepos, LONG filepos );
extern HRESULT DOMDocument_create( IUnknown *pUnkOuter, LPVOID *ppObj );
extern HRESULT SchemaCache_create( IUnknown *pUnkOuter, LPVOID *ppObj );
extern HRESULT XMLDocument_create( IUnknown *pUnkOuter, LPVOID *ppObj );
extern HRESULT SAXXMLReader_create(IUnknown *pUnkOuter, LPVOID *ppObj );
extern HRESULT XMLHTTPRequest_create(IUnknown *pUnkOuter, LPVOID *ppObj);

typedef struct bsc_t bsc_t;

HRESULT bind_url(LPCWSTR, HRESULT (*onDataAvailable)(void*,char*,DWORD), void*, bsc_t**);
void detach_bsc(bsc_t*);

/* typelibs */
typedef enum tid_t {
    IXMLDOMAttribute_tid,
    IXMLDOMCDATASection_tid,
    IXMLDOMComment_tid,
    IXMLDOMDocument_tid,
    IXMLDOMDocument2_tid,
    IXMLDOMDocumentFragment_tid,
    IXMLDOMElement_tid,
    IXMLDOMEntityReference_tid,
    IXMLDOMImplementation_tid,
    IXMLDOMNamedNodeMap_tid,
    IXMLDOMNode_tid,
    IXMLDOMNodeList_tid,
    IXMLDOMParseError_tid,
    IXMLDOMProcessingInstruction_tid,
    IXMLDOMSchemaCollection_tid,
    IXMLDOMText_tid,
    IXMLElement_tid,
    IXMLDocument_tid,
    IXMLHTTPRequest_tid,
    IVBSAXAttributes_tid,
    IVBSAXContentHandler_tid,
    IVBSAXDeclHandler_tid,
    IVBSAXDTDHandler_tid,
    IVBSAXEntityResolver_tid,
    IVBSAXErrorHandler_tid,
    IVBSAXLexicalHandler_tid,
    IVBSAXLocator_tid,
    IVBSAXXMLFilter_tid,
    IVBSAXXMLReader_tid,
    IMXAttributes_tid,
    IMXReaderControl_tid,
    IMXWriter_tid,
    LAST_tid
} tid_t;

extern HRESULT get_typeinfo(tid_t tid, ITypeInfo **typeinfo);
extern void release_typelib(void);

typedef struct dispex_data_t dispex_data_t;
typedef struct dispex_dynamic_data_t dispex_dynamic_data_t;

#define MSXML_DISPID_CUSTOM_MIN 0x60000000
#define MSXML_DISPID_CUSTOM_MAX 0x6fffffff

typedef struct {
    HRESULT (*get_dispid)(IUnknown*,BSTR,DWORD,DISPID*);
    HRESULT (*invoke)(IUnknown*,DISPID,LCID,WORD,DISPPARAMS*,VARIANT*,EXCEPINFO*);
} dispex_static_data_vtbl_t;

typedef struct {
    const dispex_static_data_vtbl_t *vtbl;
    const tid_t disp_tid;
    dispex_data_t *data;
    const tid_t* const iface_tids;
} dispex_static_data_t;

typedef struct {
    const IDispatchExVtbl  *lpIDispatchExVtbl;

    IUnknown *outer;

    dispex_static_data_t *data;
    dispex_dynamic_data_t *dynamic_data;
} DispatchEx;

void init_dispex(DispatchEx*,IUnknown*,dispex_static_data_t*);
BOOL dispex_query_interface(DispatchEx*,REFIID,void**);

/* memory allocation functions */

static inline void *heap_alloc(size_t len)
{
    return HeapAlloc(GetProcessHeap(), 0, len);
}

static inline void *heap_alloc_zero(size_t len)
{
    return HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, len);
}

static inline void *heap_realloc(void *mem, size_t len)
{
    return HeapReAlloc(GetProcessHeap(), 0, mem, len);
}

static inline BOOL heap_free(void *mem)
{
    return HeapFree(GetProcessHeap(), 0, mem);
}

static inline LPWSTR heap_strdupW(LPCWSTR str)
{
    LPWSTR ret = NULL;

    if(str) {
        DWORD size;

        size = (strlenW(str)+1)*sizeof(WCHAR);
        ret = heap_alloc(size);
        memcpy(ret, str, size);
    }

    return ret;
}

#endif /* __MSXML_PRIVATE__ */
