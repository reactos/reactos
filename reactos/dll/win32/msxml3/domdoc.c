/*
 *    DOM Document implementation
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

#define COBJMACROS
#define NONAMELESSUNION

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
#include "winreg.h"
#include "shlwapi.h"
#include "ocidl.h"
#include "objsafe.h"
#include "dispex.h"

#include "wine/debug.h"
#include "wine/list.h"

#include "msxml_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(msxml);

#ifdef HAVE_LIBXML2

#include <libxml/xmlsave.h>

/* not defined in older versions */
#define XML_SAVE_FORMAT     1
#define XML_SAVE_NO_DECL    2
#define XML_SAVE_NO_EMPTY   4
#define XML_SAVE_NO_XHTML   8
#define XML_SAVE_XHTML     16
#define XML_SAVE_AS_XML    32
#define XML_SAVE_AS_HTML   64

static const WCHAR SZ_PROPERTY_SELECTION_LANGUAGE[] = {'S','e','l','e','c','t','i','o','n','L','a','n','g','u','a','g','e',0};
static const WCHAR SZ_VALUE_XPATH[] = {'X','P','a','t','h',0};
static const WCHAR SZ_VALUE_XSLPATTERN[] = {'X','S','L','P','a','t','t','e','r','n',0};

typedef struct _domdoc
{
    xmlnode node;
    const struct IXMLDOMDocument2Vtbl *lpVtbl;
    const struct IPersistStreamVtbl   *lpvtblIPersistStream;
    const struct IObjectWithSiteVtbl  *lpvtblIObjectWithSite;
    const struct IObjectSafetyVtbl    *lpvtblIObjectSafety;
    const struct ISupportErrorInfoVtbl *lpvtblISupportErrorInfo;
    LONG ref;
    VARIANT_BOOL async;
    VARIANT_BOOL validating;
    VARIANT_BOOL resolving;
    VARIANT_BOOL preserving;
    BOOL bUseXPath;
    IXMLDOMSchemaCollection *schema;
    bsc_t *bsc;
    HRESULT error;

    /* IPersistStream */
    IStream *stream;

    /* IObjectWithSite*/
    IUnknown *site;

    /* IObjectSafety */
    DWORD safeopt;
} domdoc;

/*
  In native windows, the whole lifetime management of XMLDOMNodes is
  managed automatically using reference counts. Wine emulates that by
  maintaining a reference count to the document that is increased for
  each IXMLDOMNode pointer passed out for this document. If all these
  pointers are gone, the document is unreachable and gets freed, that
  is, all nodes in the tree of the document get freed.

  You are able to create nodes that are associated to a document (in
  fact, in msxml's XMLDOM model, all nodes are associated to a document),
  but not in the tree of that document, for example using the createFoo
  functions from IXMLDOMDocument. These nodes do not get cleaned up
  by libxml, so we have to do it ourselves.

  To catch these nodes, a list of "orphan nodes" is introduced.
  It contains pointers to all roots of node trees that are
  associated with the document without being part of the document
  tree. All nodes with parent==NULL (except for the document root nodes)
  should be in the orphan node list of their document. All orphan nodes
  get freed together with the document itself.
 */

typedef struct _xmldoc_priv {
    LONG refs;
    struct list orphans;
} xmldoc_priv;

typedef struct _orphan_entry {
    struct list entry;
    xmlNode * node;
} orphan_entry;

static inline xmldoc_priv * priv_from_xmlDocPtr(xmlDocPtr doc)
{
    return doc->_private;
}

static xmldoc_priv * create_priv(void)
{
    xmldoc_priv *priv;
    priv = heap_alloc( sizeof (*priv) );

    if(priv)
    {
        priv->refs = 0;
        list_init( &priv->orphans );
    }

    return priv;
}

static xmlDocPtr doparse( char *ptr, int len )
{
#ifdef HAVE_XMLREADMEMORY
    /*
     * use xmlReadMemory if possible so we can suppress
     * writing errors to stderr
     */
    return xmlReadMemory( ptr, len, NULL, NULL,
                          XML_PARSE_NOERROR | XML_PARSE_NOWARNING | XML_PARSE_NOBLANKS );
#else
    return xmlParseMemory( ptr, len );
#endif
}

LONG xmldoc_add_ref(xmlDocPtr doc)
{
    LONG ref = InterlockedIncrement(&priv_from_xmlDocPtr(doc)->refs);
    TRACE("%d\n", ref);
    return ref;
}

LONG xmldoc_release(xmlDocPtr doc)
{
    xmldoc_priv *priv = priv_from_xmlDocPtr(doc);
    LONG ref = InterlockedDecrement(&priv->refs);
    TRACE("%d\n", ref);
    if(ref == 0)
    {
        orphan_entry *orphan, *orphan2;
        TRACE("freeing docptr %p\n", doc);

        LIST_FOR_EACH_ENTRY_SAFE( orphan, orphan2, &priv->orphans, orphan_entry, entry )
        {
            xmlFreeNode( orphan->node );
            heap_free( orphan );
        }
        heap_free(doc->_private);

        xmlFreeDoc(doc);
    }

    return ref;
}

HRESULT xmldoc_add_orphan(xmlDocPtr doc, xmlNodePtr node)
{
    xmldoc_priv *priv = priv_from_xmlDocPtr(doc);
    orphan_entry *entry;

    entry = heap_alloc( sizeof (*entry) );
    if(!entry)
        return E_OUTOFMEMORY;

    entry->node = node;
    list_add_head( &priv->orphans, &entry->entry );
    return S_OK;
}

HRESULT xmldoc_remove_orphan(xmlDocPtr doc, xmlNodePtr node)
{
    xmldoc_priv *priv = priv_from_xmlDocPtr(doc);
    orphan_entry *entry, *entry2;

    LIST_FOR_EACH_ENTRY_SAFE( entry, entry2, &priv->orphans, orphan_entry, entry )
    {
        if( entry->node == node )
        {
            list_remove( &entry->entry );
            heap_free( entry );
            return S_OK;
        }
    }

    return S_FALSE;
}

static HRESULT attach_xmldoc( xmlnode *node, xmlDocPtr xml )
{
    if(node->node)
        xmldoc_release(node->node->doc);

    node->node = (xmlNodePtr) xml;
    if(node->node)
        xmldoc_add_ref(node->node->doc);

    return S_OK;
}

static inline domdoc *impl_from_IXMLDOMDocument2( IXMLDOMDocument2 *iface )
{
    return (domdoc *)((char*)iface - FIELD_OFFSET(domdoc, lpVtbl));
}

static inline xmlDocPtr get_doc( domdoc *This )
{
    return (xmlDocPtr)This->node.node;
}

static inline domdoc *impl_from_IPersistStream(IPersistStream *iface)
{
    return (domdoc *)((char*)iface - FIELD_OFFSET(domdoc, lpvtblIPersistStream));
}

static inline domdoc *impl_from_IObjectWithSite(IObjectWithSite *iface)
{
    return (domdoc *)((char*)iface - FIELD_OFFSET(domdoc, lpvtblIObjectWithSite));
}

static inline domdoc *impl_from_IObjectSafety(IObjectSafety *iface)
{
    return (domdoc *)((char*)iface - FIELD_OFFSET(domdoc, lpvtblIObjectSafety));
}

static inline domdoc *impl_from_ISupportErrorInfo(ISupportErrorInfo *iface)
{
    return (domdoc *)((char*)iface - FIELD_OFFSET(domdoc, lpvtblISupportErrorInfo));
}

/************************************************************************
 * xmldoc implementation of IPersistStream.
 */
static HRESULT WINAPI xmldoc_IPersistStream_QueryInterface(
    IPersistStream *iface, REFIID riid, LPVOID *ppvObj)
{
    domdoc *this = impl_from_IPersistStream(iface);
    return IXMLDocument_QueryInterface((IXMLDocument *)this, riid, ppvObj);
}

static ULONG WINAPI xmldoc_IPersistStream_AddRef(
    IPersistStream *iface)
{
    domdoc *this = impl_from_IPersistStream(iface);
    return IXMLDocument_AddRef((IXMLDocument *)this);
}

static ULONG WINAPI xmldoc_IPersistStream_Release(
    IPersistStream *iface)
{
    domdoc *this = impl_from_IPersistStream(iface);
    return IXMLDocument_Release((IXMLDocument *)this);
}

static HRESULT WINAPI xmldoc_IPersistStream_GetClassID(
    IPersistStream *iface, CLSID *classid)
{
    TRACE("(%p,%p): stub!\n", iface, classid);

    if(!classid)
        return E_POINTER;

    *classid = CLSID_DOMDocument2;

    return S_OK;
}

static HRESULT WINAPI xmldoc_IPersistStream_IsDirty(
    IPersistStream *iface)
{
    domdoc *This = impl_from_IPersistStream(iface);

    FIXME("(%p): stub!\n", This);

    return S_FALSE;
}

static HRESULT WINAPI xmldoc_IPersistStream_Load(
    IPersistStream *iface, LPSTREAM pStm)
{
    domdoc *This = impl_from_IPersistStream(iface);
    HRESULT hr;
    HGLOBAL hglobal;
    DWORD read, written, len;
    BYTE buf[4096];
    char *ptr;
    xmlDocPtr xmldoc = NULL;

    TRACE("(%p)->(%p)\n", This, pStm);

    if (!pStm)
        return E_INVALIDARG;

    hr = CreateStreamOnHGlobal(NULL, TRUE, &This->stream);
    if (FAILED(hr))
        return hr;

    do
    {
        IStream_Read(pStm, buf, sizeof(buf), &read);
        hr = IStream_Write(This->stream, buf, read, &written);
    } while(SUCCEEDED(hr) && written != 0 && read != 0);

    if (FAILED(hr))
    {
        ERR("Failed to copy stream\n");
        return hr;
    }

    hr = GetHGlobalFromStream(This->stream, &hglobal);
    if (FAILED(hr))
        return hr;

    len = GlobalSize(hglobal);
    ptr = GlobalLock(hglobal);
    if (len != 0)
        xmldoc = parse_xml(ptr, len);
    GlobalUnlock(hglobal);

    if (!xmldoc)
    {
        ERR("Failed to parse xml\n");
        return E_FAIL;
    }

    xmldoc->_private = create_priv();

    return attach_xmldoc( &This->node, xmldoc );
}

static HRESULT WINAPI xmldoc_IPersistStream_Save(
    IPersistStream *iface, LPSTREAM pStm, BOOL fClearDirty)
{
    domdoc *This = impl_from_IPersistStream(iface);
    HRESULT hr;
    BSTR xmlString;

    TRACE("(%p)->(%p %d)\n", This, pStm, fClearDirty);

    hr = IXMLDOMNode_get_xml( IXMLDOMNode_from_impl(&This->node), &xmlString );
    if(hr == S_OK)
    {
        DWORD count;
        DWORD len = strlenW(xmlString) * sizeof(WCHAR);

        hr = IStream_Write( pStm, xmlString, len, &count );

        SysFreeString(xmlString);
    }

    TRACE("ret 0x%08x\n", hr);

    return hr;
}

static HRESULT WINAPI xmldoc_IPersistStream_GetSizeMax(
    IPersistStream *iface, ULARGE_INTEGER *pcbSize)
{
    domdoc *This = impl_from_IPersistStream(iface);
    TRACE("(%p)->(%p): stub!\n", This, pcbSize);
    return E_NOTIMPL;
}

static const IPersistStreamVtbl xmldoc_IPersistStream_VTable =
{
    xmldoc_IPersistStream_QueryInterface,
    xmldoc_IPersistStream_AddRef,
    xmldoc_IPersistStream_Release,
    xmldoc_IPersistStream_GetClassID,
    xmldoc_IPersistStream_IsDirty,
    xmldoc_IPersistStream_Load,
    xmldoc_IPersistStream_Save,
    xmldoc_IPersistStream_GetSizeMax,
};

/* ISupportErrorInfo interface */
static HRESULT WINAPI support_error_QueryInterface(
    ISupportErrorInfo *iface,
    REFIID riid, void** ppvObj )
{
    domdoc *This = impl_from_ISupportErrorInfo(iface);
    return IXMLDocument_QueryInterface((IXMLDocument *)This, riid, ppvObj);
}

static ULONG WINAPI support_error_AddRef(
    ISupportErrorInfo *iface )
{
    domdoc *This = impl_from_ISupportErrorInfo(iface);
    return IXMLDocument_AddRef((IXMLDocument *)This);
}

static ULONG WINAPI support_error_Release(
    ISupportErrorInfo *iface )
{
    domdoc *This = impl_from_ISupportErrorInfo(iface);
    return IXMLDocument_Release((IXMLDocument *)This);
}

static HRESULT WINAPI support_error_InterfaceSupportsErrorInfo(
    ISupportErrorInfo *iface,
    REFIID riid )
{
    FIXME("(%p)->(%s)\n", iface, debugstr_guid(riid));
    return S_FALSE;
}

static const struct ISupportErrorInfoVtbl support_error_vtbl =
{
    support_error_QueryInterface,
    support_error_AddRef,
    support_error_Release,
    support_error_InterfaceSupportsErrorInfo
};

/* IXMLDOMDocument2 interface */
static HRESULT WINAPI domdoc_QueryInterface( IXMLDOMDocument2 *iface, REFIID riid, void** ppvObject )
{
    domdoc *This = impl_from_IXMLDOMDocument2( iface );

    TRACE("(%p)->(%s %p)\n", This, debugstr_guid( riid ), ppvObject );

    *ppvObject = NULL;

    if ( IsEqualGUID( riid, &IID_IUnknown ) ||
         IsEqualGUID( riid, &IID_IDispatch ) ||
         IsEqualGUID( riid, &IID_IXMLDOMDocument ) ||
         IsEqualGUID( riid, &IID_IXMLDOMDocument2 ) )
    {
        *ppvObject = iface;
    }
    else if ( IsEqualGUID( riid, &IID_IXMLDOMNode ) )
    {
        *ppvObject = IXMLDOMNode_from_impl(&This->node);
    }
    else if (IsEqualGUID(&IID_IPersistStream, riid))
    {
        *ppvObject = &(This->lpvtblIPersistStream);
    }
    else if (IsEqualGUID(&IID_IObjectWithSite, riid))
    {
        *ppvObject = &(This->lpvtblIObjectWithSite);
    }
    else if (IsEqualGUID(&IID_IObjectSafety, riid))
    {
        *ppvObject = &(This->lpvtblIObjectSafety);
    }
    else if( IsEqualGUID( riid, &IID_ISupportErrorInfo ))
    {
        *ppvObject = &This->lpvtblISupportErrorInfo;
    }
    else if(dispex_query_interface(&This->node.dispex, riid, ppvObject))
    {
        return *ppvObject ? S_OK : E_NOINTERFACE;
    }
    else if(IsEqualGUID(&IID_IRunnableObject, riid))
    {
        TRACE("IID_IRunnableObject not supported returning NULL\n");
        return E_NOINTERFACE;
    }
    else
    {
        FIXME("interface %s not implemented\n", debugstr_guid(riid));
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppvObject);

    return S_OK;
}


static ULONG WINAPI domdoc_AddRef(
     IXMLDOMDocument2 *iface )
{
    domdoc *This = impl_from_IXMLDOMDocument2( iface );
    TRACE("%p\n", This );
    return InterlockedIncrement( &This->ref );
}


static ULONG WINAPI domdoc_Release(
     IXMLDOMDocument2 *iface )
{
    domdoc *This = impl_from_IXMLDOMDocument2( iface );
    LONG ref;

    TRACE("%p\n", This );

    ref = InterlockedDecrement( &This->ref );
    if ( ref == 0 )
    {
        if(This->bsc)
            detach_bsc(This->bsc);

        if (This->site)
            IUnknown_Release( This->site );
        destroy_xmlnode(&This->node);
        if(This->schema) IXMLDOMSchemaCollection_Release( This->schema );
        if (This->stream) IStream_Release(This->stream);
        HeapFree( GetProcessHeap(), 0, This );
    }

    return ref;
}

static HRESULT WINAPI domdoc_GetTypeInfoCount( IXMLDOMDocument2 *iface, UINT* pctinfo )
{
    domdoc *This = impl_from_IXMLDOMDocument2( iface );

    TRACE("(%p)->(%p)\n", This, pctinfo);

    *pctinfo = 1;

    return S_OK;
}

static HRESULT WINAPI domdoc_GetTypeInfo(
    IXMLDOMDocument2 *iface,
    UINT iTInfo, LCID lcid, ITypeInfo** ppTInfo )
{
    domdoc *This = impl_from_IXMLDOMDocument2( iface );
    HRESULT hr;

    TRACE("(%p)->(%u %u %p)\n", This, iTInfo, lcid, ppTInfo);

    hr = get_typeinfo(IXMLDOMDocument2_tid, ppTInfo);

    return hr;
}

static HRESULT WINAPI domdoc_GetIDsOfNames(
    IXMLDOMDocument2 *iface,
    REFIID riid,
    LPOLESTR* rgszNames,
    UINT cNames,
    LCID lcid,
    DISPID* rgDispId)
{
    domdoc *This = impl_from_IXMLDOMDocument2( iface );
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("(%p)->(%s %p %u %u %p)\n", This, debugstr_guid(riid), rgszNames, cNames,
          lcid, rgDispId);

    if(!rgszNames || cNames == 0 || !rgDispId)
        return E_INVALIDARG;

    hr = get_typeinfo(IXMLDOMDocument2_tid, &typeinfo);
    if(SUCCEEDED(hr))
    {
        hr = ITypeInfo_GetIDsOfNames(typeinfo, rgszNames, cNames, rgDispId);
        ITypeInfo_Release(typeinfo);
    }

    return hr;
}


static HRESULT WINAPI domdoc_Invoke(
    IXMLDOMDocument2 *iface,
    DISPID dispIdMember,
    REFIID riid,
    LCID lcid,
    WORD wFlags,
    DISPPARAMS* pDispParams,
    VARIANT* pVarResult,
    EXCEPINFO* pExcepInfo,
    UINT* puArgErr)
{
    domdoc *This = impl_from_IXMLDOMDocument2( iface );
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("(%p)->(%d %s %d %d %p %p %p %p)\n", This, dispIdMember, debugstr_guid(riid),
          lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);

    hr = get_typeinfo(IXMLDOMDocument2_tid, &typeinfo);
    if(SUCCEEDED(hr))
    {
        hr = ITypeInfo_Invoke(typeinfo, &(This->lpVtbl), dispIdMember, wFlags, pDispParams,
                pVarResult, pExcepInfo, puArgErr);
        ITypeInfo_Release(typeinfo);
    }

    return hr;
}


static HRESULT WINAPI domdoc_get_nodeName(
    IXMLDOMDocument2 *iface,
    BSTR* name )
{
    domdoc *This = impl_from_IXMLDOMDocument2( iface );
    return IXMLDOMNode_get_nodeName( IXMLDOMNode_from_impl(&This->node), name );
}


static HRESULT WINAPI domdoc_get_nodeValue(
    IXMLDOMDocument2 *iface,
    VARIANT* value )
{
    domdoc *This = impl_from_IXMLDOMDocument2( iface );
    return IXMLDOMNode_get_nodeValue( IXMLDOMNode_from_impl(&This->node), value );
}


static HRESULT WINAPI domdoc_put_nodeValue(
    IXMLDOMDocument2 *iface,
    VARIANT value)
{
    domdoc *This = impl_from_IXMLDOMDocument2( iface );
    return IXMLDOMNode_put_nodeValue( IXMLDOMNode_from_impl(&This->node), value );
}


static HRESULT WINAPI domdoc_get_nodeType(
    IXMLDOMDocument2 *iface,
    DOMNodeType* type )
{
    domdoc *This = impl_from_IXMLDOMDocument2( iface );
    return IXMLDOMNode_get_nodeType( IXMLDOMNode_from_impl(&This->node), type );
}


static HRESULT WINAPI domdoc_get_parentNode(
    IXMLDOMDocument2 *iface,
    IXMLDOMNode** parent )
{
    domdoc *This = impl_from_IXMLDOMDocument2( iface );
    return IXMLDOMNode_get_parentNode( IXMLDOMNode_from_impl(&This->node), parent );
}


static HRESULT WINAPI domdoc_get_childNodes(
    IXMLDOMDocument2 *iface,
    IXMLDOMNodeList** childList )
{
    domdoc *This = impl_from_IXMLDOMDocument2( iface );
    return IXMLDOMNode_get_childNodes( IXMLDOMNode_from_impl(&This->node), childList );
}


static HRESULT WINAPI domdoc_get_firstChild(
    IXMLDOMDocument2 *iface,
    IXMLDOMNode** firstChild )
{
    domdoc *This = impl_from_IXMLDOMDocument2( iface );
    return IXMLDOMNode_get_firstChild( IXMLDOMNode_from_impl(&This->node), firstChild );
}


static HRESULT WINAPI domdoc_get_lastChild(
    IXMLDOMDocument2 *iface,
    IXMLDOMNode** lastChild )
{
    domdoc *This = impl_from_IXMLDOMDocument2( iface );
    return IXMLDOMNode_get_lastChild( IXMLDOMNode_from_impl(&This->node), lastChild );
}


static HRESULT WINAPI domdoc_get_previousSibling(
    IXMLDOMDocument2 *iface,
    IXMLDOMNode** previousSibling )
{
    domdoc *This = impl_from_IXMLDOMDocument2( iface );
    return IXMLDOMNode_get_previousSibling( IXMLDOMNode_from_impl(&This->node), previousSibling );
}


static HRESULT WINAPI domdoc_get_nextSibling(
    IXMLDOMDocument2 *iface,
    IXMLDOMNode** nextSibling )
{
    domdoc *This = impl_from_IXMLDOMDocument2( iface );
    return IXMLDOMNode_get_nextSibling( IXMLDOMNode_from_impl(&This->node), nextSibling );
}


static HRESULT WINAPI domdoc_get_attributes(
    IXMLDOMDocument2 *iface,
    IXMLDOMNamedNodeMap** attributeMap )
{
    domdoc *This = impl_from_IXMLDOMDocument2( iface );
    return IXMLDOMNode_get_attributes( IXMLDOMNode_from_impl(&This->node), attributeMap );
}


static HRESULT WINAPI domdoc_insertBefore(
    IXMLDOMDocument2 *iface,
    IXMLDOMNode* newChild,
    VARIANT refChild,
    IXMLDOMNode** outNewChild )
{
    domdoc *This = impl_from_IXMLDOMDocument2( iface );
    return IXMLDOMNode_insertBefore( IXMLDOMNode_from_impl(&This->node), newChild, refChild, outNewChild );
}


static HRESULT WINAPI domdoc_replaceChild(
    IXMLDOMDocument2 *iface,
    IXMLDOMNode* newChild,
    IXMLDOMNode* oldChild,
    IXMLDOMNode** outOldChild)
{
    domdoc *This = impl_from_IXMLDOMDocument2( iface );
    return IXMLDOMNode_replaceChild( IXMLDOMNode_from_impl(&This->node), newChild, oldChild, outOldChild );
}


static HRESULT WINAPI domdoc_removeChild(
    IXMLDOMDocument2 *iface,
    IXMLDOMNode* childNode,
    IXMLDOMNode** oldChild)
{
    domdoc *This = impl_from_IXMLDOMDocument2( iface );
    return IXMLDOMNode_removeChild( IXMLDOMNode_from_impl(&This->node), childNode, oldChild );
}


static HRESULT WINAPI domdoc_appendChild(
    IXMLDOMDocument2 *iface,
    IXMLDOMNode* newChild,
    IXMLDOMNode** outNewChild)
{
    domdoc *This = impl_from_IXMLDOMDocument2( iface );
    return IXMLDOMNode_appendChild( IXMLDOMNode_from_impl(&This->node), newChild, outNewChild );
}


static HRESULT WINAPI domdoc_hasChildNodes(
    IXMLDOMDocument2 *iface,
    VARIANT_BOOL* hasChild)
{
    domdoc *This = impl_from_IXMLDOMDocument2( iface );
    return IXMLDOMNode_hasChildNodes( IXMLDOMNode_from_impl(&This->node), hasChild );
}


static HRESULT WINAPI domdoc_get_ownerDocument(
    IXMLDOMDocument2 *iface,
    IXMLDOMDocument** DOMDocument)
{
    domdoc *This = impl_from_IXMLDOMDocument2( iface );
    return IXMLDOMNode_get_ownerDocument( IXMLDOMNode_from_impl(&This->node), DOMDocument );
}


static HRESULT WINAPI domdoc_cloneNode(
    IXMLDOMDocument2 *iface,
    VARIANT_BOOL deep,
    IXMLDOMNode** cloneRoot)
{
    domdoc *This = impl_from_IXMLDOMDocument2( iface );
    return IXMLDOMNode_cloneNode( IXMLDOMNode_from_impl(&This->node), deep, cloneRoot );
}


static HRESULT WINAPI domdoc_get_nodeTypeString(
    IXMLDOMDocument2 *iface,
    BSTR* nodeType )
{
    domdoc *This = impl_from_IXMLDOMDocument2( iface );
    return IXMLDOMNode_get_nodeTypeString( IXMLDOMNode_from_impl(&This->node), nodeType );
}


static HRESULT WINAPI domdoc_get_text(
    IXMLDOMDocument2 *iface,
    BSTR* text )
{
    domdoc *This = impl_from_IXMLDOMDocument2( iface );
    return IXMLDOMNode_get_text( IXMLDOMNode_from_impl(&This->node), text );
}


static HRESULT WINAPI domdoc_put_text(
    IXMLDOMDocument2 *iface,
    BSTR text )
{
    domdoc *This = impl_from_IXMLDOMDocument2( iface );
    return IXMLDOMNode_put_text( IXMLDOMNode_from_impl(&This->node), text );
}


static HRESULT WINAPI domdoc_get_specified(
    IXMLDOMDocument2 *iface,
    VARIANT_BOOL* isSpecified )
{
    domdoc *This = impl_from_IXMLDOMDocument2( iface );
    return IXMLDOMNode_get_specified( IXMLDOMNode_from_impl(&This->node), isSpecified );
}


static HRESULT WINAPI domdoc_get_definition(
    IXMLDOMDocument2 *iface,
    IXMLDOMNode** definitionNode )
{
    domdoc *This = impl_from_IXMLDOMDocument2( iface );
    return IXMLDOMNode_get_definition( IXMLDOMNode_from_impl(&This->node), definitionNode );
}


static HRESULT WINAPI domdoc_get_nodeTypedValue(
    IXMLDOMDocument2 *iface,
    VARIANT* typedValue )
{
    domdoc *This = impl_from_IXMLDOMDocument2( iface );
    return IXMLDOMNode_get_nodeTypedValue( IXMLDOMNode_from_impl(&This->node), typedValue );
}

static HRESULT WINAPI domdoc_put_nodeTypedValue(
    IXMLDOMDocument2 *iface,
    VARIANT typedValue )
{
    domdoc *This = impl_from_IXMLDOMDocument2( iface );
    return IXMLDOMNode_put_nodeTypedValue( IXMLDOMNode_from_impl(&This->node), typedValue );
}


static HRESULT WINAPI domdoc_get_dataType(
    IXMLDOMDocument2 *iface,
    VARIANT* dataTypeName )
{
    domdoc *This = impl_from_IXMLDOMDocument2( iface );
    return IXMLDOMNode_get_dataType( IXMLDOMNode_from_impl(&This->node), dataTypeName );
}


static HRESULT WINAPI domdoc_put_dataType(
    IXMLDOMDocument2 *iface,
    BSTR dataTypeName )
{
    domdoc *This = impl_from_IXMLDOMDocument2( iface );
    return IXMLDOMNode_put_dataType( IXMLDOMNode_from_impl(&This->node), dataTypeName );
}


static HRESULT WINAPI domdoc_get_xml(
    IXMLDOMDocument2 *iface,
    BSTR* xmlString )
{
    domdoc *This = impl_from_IXMLDOMDocument2( iface );
    return IXMLDOMNode_get_xml( IXMLDOMNode_from_impl(&This->node), xmlString );
}


static HRESULT WINAPI domdoc_transformNode(
    IXMLDOMDocument2 *iface,
    IXMLDOMNode* styleSheet,
    BSTR* xmlString )
{
    domdoc *This = impl_from_IXMLDOMDocument2( iface );
    return IXMLDOMNode_transformNode( IXMLDOMNode_from_impl(&This->node), styleSheet, xmlString );
}


static HRESULT WINAPI domdoc_selectNodes(
    IXMLDOMDocument2 *iface,
    BSTR queryString,
    IXMLDOMNodeList** resultList )
{
    domdoc *This = impl_from_IXMLDOMDocument2( iface );
    return IXMLDOMNode_selectNodes( IXMLDOMNode_from_impl(&This->node), queryString, resultList );
}


static HRESULT WINAPI domdoc_selectSingleNode(
    IXMLDOMDocument2 *iface,
    BSTR queryString,
    IXMLDOMNode** resultNode )
{
    domdoc *This = impl_from_IXMLDOMDocument2( iface );
    return IXMLDOMNode_selectSingleNode( IXMLDOMNode_from_impl(&This->node), queryString, resultNode );
}


static HRESULT WINAPI domdoc_get_parsed(
    IXMLDOMDocument2 *iface,
    VARIANT_BOOL* isParsed )
{
    domdoc *This = impl_from_IXMLDOMDocument2( iface );
    return IXMLDOMNode_get_parsed( IXMLDOMNode_from_impl(&This->node), isParsed );
}


static HRESULT WINAPI domdoc_get_namespaceURI(
    IXMLDOMDocument2 *iface,
    BSTR* namespaceURI )
{
    domdoc *This = impl_from_IXMLDOMDocument2( iface );
    return IXMLDOMNode_get_namespaceURI( IXMLDOMNode_from_impl(&This->node), namespaceURI );
}


static HRESULT WINAPI domdoc_get_prefix(
    IXMLDOMDocument2 *iface,
    BSTR* prefixString )
{
    domdoc *This = impl_from_IXMLDOMDocument2( iface );
    return IXMLDOMNode_get_prefix( IXMLDOMNode_from_impl(&This->node), prefixString );
}


static HRESULT WINAPI domdoc_get_baseName(
    IXMLDOMDocument2 *iface,
    BSTR* nameString )
{
    domdoc *This = impl_from_IXMLDOMDocument2( iface );
    return IXMLDOMNode_get_baseName( IXMLDOMNode_from_impl(&This->node), nameString );
}


static HRESULT WINAPI domdoc_transformNodeToObject(
    IXMLDOMDocument2 *iface,
    IXMLDOMNode* stylesheet,
    VARIANT outputObject)
{
    domdoc *This = impl_from_IXMLDOMDocument2( iface );
    return IXMLDOMNode_transformNodeToObject( IXMLDOMNode_from_impl(&This->node), stylesheet, outputObject );
}


static HRESULT WINAPI domdoc_get_doctype(
    IXMLDOMDocument2 *iface,
    IXMLDOMDocumentType** documentType )
{
    domdoc *This = impl_from_IXMLDOMDocument2(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}


static HRESULT WINAPI domdoc_get_implementation(
    IXMLDOMDocument2 *iface,
    IXMLDOMImplementation** impl )
{
    domdoc *This = impl_from_IXMLDOMDocument2(iface);

    TRACE("(%p)->(%p)\n", This, impl);

    if(!impl)
        return E_INVALIDARG;

    *impl = (IXMLDOMImplementation*)create_doc_Implementation();

    return S_OK;
}

static HRESULT WINAPI domdoc_get_documentElement(
    IXMLDOMDocument2 *iface,
    IXMLDOMElement** DOMElement )
{
    domdoc *This = impl_from_IXMLDOMDocument2( iface );
    xmlDocPtr xmldoc = NULL;
    xmlNodePtr root = NULL;
    IXMLDOMNode *element_node;
    HRESULT hr;

    TRACE("(%p)->(%p)\n", This, DOMElement);

    if(!DOMElement)
        return E_INVALIDARG;

    *DOMElement = NULL;

    xmldoc = get_doc( This );

    root = xmlDocGetRootElement( xmldoc );
    if ( !root )
        return S_FALSE;

    element_node = create_node( root );
    if(!element_node) return S_FALSE;

    hr = IXMLDOMNode_QueryInterface(element_node, &IID_IXMLDOMElement, (LPVOID*)DOMElement);
    IXMLDOMNode_Release(element_node);

    return hr;
}


static HRESULT WINAPI domdoc_put_documentElement(
    IXMLDOMDocument2 *iface,
    IXMLDOMElement* DOMElement )
{
    domdoc *This = impl_from_IXMLDOMDocument2( iface );
    IXMLDOMNode *elementNode;
    xmlNodePtr oldRoot;
    xmlnode *xmlNode;
    HRESULT hr;

    TRACE("(%p)->(%p)\n", This, DOMElement);

    hr = IXMLDOMElement_QueryInterface( DOMElement, &IID_IXMLDOMNode, (void**)&elementNode );
    if(FAILED(hr))
        return hr;

    xmlNode = impl_from_IXMLDOMNode( elementNode );

    if(!xmlNode->node->parent)
        if(xmldoc_remove_orphan(xmlNode->node->doc, xmlNode->node) != S_OK)
            WARN("%p is not an orphan of %p\n", xmlNode->node->doc, xmlNode->node);

    oldRoot = xmlDocSetRootElement( get_doc(This), xmlNode->node);
    IXMLDOMNode_Release( elementNode );

    if(oldRoot)
        xmldoc_add_orphan(oldRoot->doc, oldRoot);

    return S_OK;
}


static HRESULT WINAPI domdoc_createElement(
    IXMLDOMDocument2 *iface,
    BSTR tagname,
    IXMLDOMElement** element )
{
    domdoc *This = impl_from_IXMLDOMDocument2( iface );
    IXMLDOMNode *node;
    VARIANT type;
    HRESULT hr;

    TRACE("(%p)->(%s %p)\n", This, debugstr_w(tagname), element);

    if (!element || !tagname) return E_INVALIDARG;

    V_VT(&type) = VT_I1;
    V_I1(&type) = NODE_ELEMENT;

    hr = IXMLDOMDocument_createNode(iface, type, tagname, NULL, &node);
    if (hr == S_OK)
    {
        IXMLDOMNode_QueryInterface(node, &IID_IXMLDOMElement, (void**)element);
        IXMLDOMNode_Release(node);
    }

    return hr;
}


static HRESULT WINAPI domdoc_createDocumentFragment(
    IXMLDOMDocument2 *iface,
    IXMLDOMDocumentFragment** frag )
{
    domdoc *This = impl_from_IXMLDOMDocument2( iface );
    IXMLDOMNode *node;
    VARIANT type;
    HRESULT hr;

    TRACE("(%p)->(%p)\n", This, frag);

    if (!frag) return E_INVALIDARG;

    *frag = NULL;

    V_VT(&type) = VT_I1;
    V_I1(&type) = NODE_DOCUMENT_FRAGMENT;

    hr = IXMLDOMDocument_createNode(iface, type, NULL, NULL, &node);
    if (hr == S_OK)
    {
        IXMLDOMNode_QueryInterface(node, &IID_IXMLDOMDocumentFragment, (void**)frag);
        IXMLDOMNode_Release(node);
    }

    return hr;
}


static HRESULT WINAPI domdoc_createTextNode(
    IXMLDOMDocument2 *iface,
    BSTR data,
    IXMLDOMText** text )
{
    domdoc *This = impl_from_IXMLDOMDocument2( iface );
    IXMLDOMNode *node;
    VARIANT type;
    HRESULT hr;

    TRACE("(%p)->(%s %p)\n", This, debugstr_w(data), text);

    if (!text) return E_INVALIDARG;

    *text = NULL;

    V_VT(&type) = VT_I1;
    V_I1(&type) = NODE_TEXT;

    hr = IXMLDOMDocument2_createNode(iface, type, NULL, NULL, &node);
    if (hr == S_OK)
    {
        IXMLDOMNode_QueryInterface(node, &IID_IXMLDOMText, (void**)text);
        IXMLDOMNode_Release(node);
        hr = IXMLDOMText_put_data(*text, data);
    }

    return hr;
}


static HRESULT WINAPI domdoc_createComment(
    IXMLDOMDocument2 *iface,
    BSTR data,
    IXMLDOMComment** comment )
{
    domdoc *This = impl_from_IXMLDOMDocument2( iface );
    VARIANT type;
    HRESULT hr;
    IXMLDOMNode *node;

    TRACE("(%p)->(%s %p)\n", This, debugstr_w(data), comment);

    if (!comment) return E_INVALIDARG;

    *comment = NULL;

    V_VT(&type) = VT_I1;
    V_I1(&type) = NODE_COMMENT;

    hr = IXMLDOMDocument2_createNode(iface, type, NULL, NULL, &node);
    if (hr == S_OK)
    {
        IXMLDOMNode_QueryInterface(node, &IID_IXMLDOMComment, (void**)comment);
        IXMLDOMNode_Release(node);
        hr = IXMLDOMComment_put_data(*comment, data);
    }

    return hr;
}


static HRESULT WINAPI domdoc_createCDATASection(
    IXMLDOMDocument2 *iface,
    BSTR data,
    IXMLDOMCDATASection** cdata )
{
    domdoc *This = impl_from_IXMLDOMDocument2( iface );
    IXMLDOMNode *node;
    VARIANT type;
    HRESULT hr;

    TRACE("(%p)->(%s %p)\n", This, debugstr_w(data), cdata);

    if (!cdata) return E_INVALIDARG;

    *cdata = NULL;

    V_VT(&type) = VT_I1;
    V_I1(&type) = NODE_CDATA_SECTION;

    hr = IXMLDOMDocument2_createNode(iface, type, NULL, NULL, &node);
    if (hr == S_OK)
    {
        IXMLDOMNode_QueryInterface(node, &IID_IXMLDOMCDATASection, (void**)cdata);
        IXMLDOMNode_Release(node);
        hr = IXMLDOMCDATASection_put_data(*cdata, data);
    }

    return hr;
}


static HRESULT WINAPI domdoc_createProcessingInstruction(
    IXMLDOMDocument2 *iface,
    BSTR target,
    BSTR data,
    IXMLDOMProcessingInstruction** pi )
{
    domdoc *This = impl_from_IXMLDOMDocument2( iface );
    IXMLDOMNode *node;
    VARIANT type;
    HRESULT hr;

    TRACE("(%p)->(%s %s %p)\n", This, debugstr_w(target), debugstr_w(data), pi);

    if (!pi) return E_INVALIDARG;

    *pi = NULL;

    V_VT(&type) = VT_I1;
    V_I1(&type) = NODE_PROCESSING_INSTRUCTION;

    hr = IXMLDOMDocument2_createNode(iface, type, target, NULL, &node);
    if (hr == S_OK)
    {
        VARIANT v_data;

        /* this is to bypass check in ::put_data() that blocks "<?xml" PIs */
        V_VT(&v_data)   = VT_BSTR;
        V_BSTR(&v_data) = data;

        hr = IXMLDOMNode_put_nodeValue( node, v_data );

        IXMLDOMNode_QueryInterface(node, &IID_IXMLDOMProcessingInstruction, (void**)pi);
        IXMLDOMNode_Release(node);
    }

    return hr;
}


static HRESULT WINAPI domdoc_createAttribute(
    IXMLDOMDocument2 *iface,
    BSTR name,
    IXMLDOMAttribute** attribute )
{
    domdoc *This = impl_from_IXMLDOMDocument2( iface );
    IXMLDOMNode *node;
    VARIANT type;
    HRESULT hr;

    TRACE("(%p)->(%s %p)\n", This, debugstr_w(name), attribute);

    if (!attribute || !name) return E_INVALIDARG;

    V_VT(&type) = VT_I1;
    V_I1(&type) = NODE_ATTRIBUTE;

    hr = IXMLDOMDocument_createNode(iface, type, name, NULL, &node);
    if (hr == S_OK)
    {
        IXMLDOMNode_QueryInterface(node, &IID_IXMLDOMAttribute, (void**)attribute);
        IXMLDOMNode_Release(node);
    }

    return hr;
}


static HRESULT WINAPI domdoc_createEntityReference(
    IXMLDOMDocument2 *iface,
    BSTR name,
    IXMLDOMEntityReference** entityref )
{
    domdoc *This = impl_from_IXMLDOMDocument2( iface );
    IXMLDOMNode *node;
    VARIANT type;
    HRESULT hr;

    TRACE("(%p)->(%s %p)\n", This, debugstr_w(name), entityref);

    if (!entityref) return E_INVALIDARG;

    *entityref = NULL;

    V_VT(&type) = VT_I1;
    V_I1(&type) = NODE_ENTITY_REFERENCE;

    hr = IXMLDOMDocument2_createNode(iface, type, name, NULL, &node);
    if (hr == S_OK)
    {
        IXMLDOMNode_QueryInterface(node, &IID_IXMLDOMEntityReference, (void**)entityref);
        IXMLDOMNode_Release(node);
    }

    return hr;
}


static HRESULT WINAPI domdoc_getElementsByTagName(
    IXMLDOMDocument2 *iface,
    BSTR tagName,
    IXMLDOMNodeList** resultList )
{
    static const WCHAR xpathformat[] =
            { '/','/','*','[','l','o','c','a','l','-','n','a','m','e','(',')','=','\'','%','s','\'',']',0 };
    domdoc *This = impl_from_IXMLDOMDocument2( iface );
    LPWSTR szPattern;
    HRESULT hr;
    TRACE("(%p)->(%s %p)\n", This, debugstr_w(tagName), resultList);

    if (tagName[0] == '*' && tagName[1] == 0)
    {
        szPattern = heap_alloc(sizeof(WCHAR)*4);
        szPattern[0] = szPattern[1] = '/';
        szPattern[2] = '*';
        szPattern[3] = 0;
    }
    else
    {
        szPattern = heap_alloc(sizeof(WCHAR)*(20+lstrlenW(tagName)+1));
        wsprintfW(szPattern, xpathformat, tagName);
    }

    hr = queryresult_create((xmlNodePtr)get_doc(This), szPattern, resultList);
    heap_free(szPattern);

    return hr;
}

static HRESULT get_node_type(VARIANT Type, DOMNodeType * type)
{
    VARIANT tmp;
    HRESULT hr;

    VariantInit(&tmp);
    hr = VariantChangeType(&tmp, &Type, 0, VT_I4);
    if(FAILED(hr))
        return E_INVALIDARG;

    *type = V_I4(&tmp);

    return S_OK;
}

static HRESULT WINAPI domdoc_createNode(
    IXMLDOMDocument2 *iface,
    VARIANT Type,
    BSTR name,
    BSTR namespaceURI,
    IXMLDOMNode** node )
{
    domdoc *This = impl_from_IXMLDOMDocument2( iface );
    DOMNodeType node_type;
    xmlNodePtr xmlnode;
    xmlChar *xml_name;
    HRESULT hr;

    TRACE("(%p)->(%s %s %p)\n", This, debugstr_w(name), debugstr_w(namespaceURI), node);

    if(!node) return E_INVALIDARG;

    if(namespaceURI && namespaceURI[0])
        FIXME("nodes with namespaces currently not supported.\n");

    hr = get_node_type(Type, &node_type);
    if(FAILED(hr)) return hr;

    TRACE("node_type %d\n", node_type);

    /* exit earlier for types that need name */
    switch(node_type)
    {
    case NODE_ELEMENT:
    case NODE_ATTRIBUTE:
    case NODE_ENTITY_REFERENCE:
    case NODE_PROCESSING_INSTRUCTION:
        if (!name || SysStringLen(name) == 0) return E_FAIL;
    default:
        break;
    }

    xml_name = xmlChar_from_wchar(name);

    switch(node_type)
    {
    case NODE_ELEMENT:
        xmlnode = xmlNewDocNode(get_doc(This), NULL, xml_name, NULL);
        break;
    case NODE_ATTRIBUTE:
        xmlnode = (xmlNodePtr)xmlNewDocProp(get_doc(This), xml_name, NULL);
        break;
    case NODE_TEXT:
        xmlnode = (xmlNodePtr)xmlNewDocText(get_doc(This), NULL);
        break;
    case NODE_CDATA_SECTION:
        xmlnode = xmlNewCDataBlock(get_doc(This), NULL, 0);
        break;
    case NODE_ENTITY_REFERENCE:
        xmlnode = xmlNewReference(get_doc(This), xml_name);
        break;
    case NODE_PROCESSING_INSTRUCTION:
#ifdef HAVE_XMLNEWDOCPI
        xmlnode = xmlNewDocPI(get_doc(This), xml_name, NULL);
#else
        FIXME("xmlNewDocPI() not supported, use libxml2 2.6.15 or greater\n");
        xmlnode = NULL;
#endif
        break;
    case NODE_COMMENT:
        xmlnode = xmlNewDocComment(get_doc(This), NULL);
        break;
    case NODE_DOCUMENT_FRAGMENT:
        xmlnode = xmlNewDocFragment(get_doc(This));
        break;
    /* unsupported types */
    case NODE_DOCUMENT:
    case NODE_DOCUMENT_TYPE:
    case NODE_ENTITY:
    case NODE_NOTATION:
        heap_free(xml_name);
        return E_INVALIDARG;
    default:
        FIXME("unhandled node type %d\n", node_type);
        xmlnode = NULL;
        break;
    }

    *node = create_node(xmlnode);
    heap_free(xml_name);

    if(*node)
    {
        TRACE("created node (%d, %p, %p)\n", node_type, *node, xmlnode);
        xmldoc_add_orphan(xmlnode->doc, xmlnode);
        return S_OK;
    }

    return E_FAIL;
}

static HRESULT WINAPI domdoc_nodeFromID(
    IXMLDOMDocument2 *iface,
    BSTR idString,
    IXMLDOMNode** node )
{
    domdoc *This = impl_from_IXMLDOMDocument2(iface);
    FIXME("(%p)->(%s %p)\n", This, debugstr_w(idString), node);
    return E_NOTIMPL;
}

static HRESULT domdoc_onDataAvailable(void *obj, char *ptr, DWORD len)
{
    domdoc *This = obj;
    xmlDocPtr xmldoc;

    xmldoc = doparse( ptr, len );
    if(xmldoc) {
        xmldoc->_private = create_priv();
        return attach_xmldoc(&This->node, xmldoc);
    }

    return S_OK;
}

static HRESULT doread( domdoc *This, LPWSTR filename )
{
    bsc_t *bsc;
    HRESULT hr;

    hr = bind_url(filename, domdoc_onDataAvailable, This, &bsc);
    if(FAILED(hr))
        return hr;

    if(This->bsc)
        detach_bsc(This->bsc);

    This->bsc = bsc;
    return S_OK;
}

static HRESULT WINAPI domdoc_load(
    IXMLDOMDocument2 *iface,
    VARIANT xmlSource,
    VARIANT_BOOL* isSuccessful )
{
    domdoc *This = impl_from_IXMLDOMDocument2( iface );
    LPWSTR filename = NULL;
    HRESULT hr = S_FALSE;
    IXMLDOMDocument2 *pNewDoc = NULL;
    IStream *pStream = NULL;
    xmlDocPtr xmldoc;

    TRACE("(%p)->type %d\n", This, V_VT(&xmlSource) );

    *isSuccessful = VARIANT_FALSE;

    assert( &This->node );

    switch( V_VT(&xmlSource) )
    {
    case VT_BSTR:
        filename = V_BSTR(&xmlSource);
        break;
    case VT_UNKNOWN:
        hr = IUnknown_QueryInterface(V_UNKNOWN(&xmlSource), &IID_IXMLDOMDocument2, (void**)&pNewDoc);
        if(hr == S_OK)
        {
            if(pNewDoc)
            {
                domdoc *newDoc = impl_from_IXMLDOMDocument2( pNewDoc );
                xmldoc = xmlCopyDoc(get_doc(newDoc), 1);
                hr = attach_xmldoc(&This->node, xmldoc);

                if(SUCCEEDED(hr))
                    *isSuccessful = VARIANT_TRUE;

                return hr;
            }
        }
        hr = IUnknown_QueryInterface(V_UNKNOWN(&xmlSource), &IID_IStream, (void**)&pStream);
        if(hr == S_OK)
        {
            IPersistStream *pDocStream;
            hr = IUnknown_QueryInterface(iface, &IID_IPersistStream, (void**)&pDocStream);
            if(hr == S_OK)
            {
                hr = xmldoc_IPersistStream_Load(pDocStream, pStream);
                IStream_Release(pStream);
                if(hr == S_OK)
                {
                    *isSuccessful = VARIANT_TRUE;

                    TRACE("Using ID_IStream to load Document\n");
                    return S_OK;
                }
                else
                {
                    ERR("xmldoc_IPersistStream_Load failed (%d)\n", hr);
                }
            }
            else
            {
                ERR("QueryInterface IID_IPersistStream failed (%d)\n", hr);
            }
        }
        else
        {
            /* ISequentialStream */
            FIXME("Unknown type not supported (%d) (%p)(%p)\n", hr, pNewDoc, V_UNKNOWN(&xmlSource)->lpVtbl);
        }
        break;
     default:
            FIXME("VT type not supported (%d)\n", V_VT(&xmlSource));
     }

    TRACE("filename (%s)\n", debugstr_w(filename));

    if ( filename )
    {
        hr = doread( This, filename );
    
        if ( FAILED(hr) )
            This->error = E_FAIL;
        else
        {
            hr = This->error = S_OK;
            *isSuccessful = VARIANT_TRUE;
        }
    }

    if(!filename || FAILED(hr)) {
        xmldoc = xmlNewDoc(NULL);
        xmldoc->_private = create_priv();
        hr = attach_xmldoc(&This->node, xmldoc);
        if(SUCCEEDED(hr))
            hr = S_FALSE;
    }

    TRACE("ret (%d)\n", hr);

    return hr;
}


static HRESULT WINAPI domdoc_get_readyState(
    IXMLDOMDocument2 *iface,
    LONG *value )
{
    domdoc *This = impl_from_IXMLDOMDocument2(iface);
    FIXME("(%p)->(%p)\n", This, value);
    return E_NOTIMPL;
}


static HRESULT WINAPI domdoc_get_parseError(
    IXMLDOMDocument2 *iface,
    IXMLDOMParseError** errorObj )
{
    BSTR error_string = NULL;
    static const WCHAR err[] = {'e','r','r','o','r',0};
    domdoc *This = impl_from_IXMLDOMDocument2( iface );

    FIXME("(%p)->(%p): creating a dummy parseError\n", iface, errorObj);

    if(This->error)
        error_string = SysAllocString(err);

    *errorObj = create_parseError(This->error, NULL, error_string, NULL, 0, 0, 0);
    if(!*errorObj) return E_OUTOFMEMORY;
    return S_OK;
}


static HRESULT WINAPI domdoc_get_url(
    IXMLDOMDocument2 *iface,
    BSTR* urlString )
{
    domdoc *This = impl_from_IXMLDOMDocument2(iface);
    FIXME("(%p)->(%p)\n", This, urlString);
    return E_NOTIMPL;
}


static HRESULT WINAPI domdoc_get_async(
    IXMLDOMDocument2 *iface,
    VARIANT_BOOL* isAsync )
{
    domdoc *This = impl_from_IXMLDOMDocument2( iface );

    TRACE("(%p)->(%p: %d)\n", This, isAsync, This->async);
    *isAsync = This->async;
    return S_OK;
}


static HRESULT WINAPI domdoc_put_async(
    IXMLDOMDocument2 *iface,
    VARIANT_BOOL isAsync )
{
    domdoc *This = impl_from_IXMLDOMDocument2( iface );

    TRACE("(%p)->(%d)\n", This, isAsync);
    This->async = isAsync;
    return S_OK;
}


static HRESULT WINAPI domdoc_abort(
    IXMLDOMDocument2 *iface )
{
    domdoc *This = impl_from_IXMLDOMDocument2(iface);
    FIXME("%p\n", This);
    return E_NOTIMPL;
}


static BOOL bstr_to_utf8( BSTR bstr, char **pstr, int *plen )
{
    UINT len, blen = SysStringLen( bstr );
    LPSTR str;

    len = WideCharToMultiByte( CP_UTF8, 0, bstr, blen, NULL, 0, NULL, NULL );
    str = heap_alloc( len );
    if ( !str )
        return FALSE;
    WideCharToMultiByte( CP_UTF8, 0, bstr, blen, str, len, NULL, NULL );
    *plen = len;
    *pstr = str;
    return TRUE;
}

static HRESULT WINAPI domdoc_loadXML(
    IXMLDOMDocument2 *iface,
    BSTR bstrXML,
    VARIANT_BOOL* isSuccessful )
{
    domdoc *This = impl_from_IXMLDOMDocument2( iface );
    xmlDocPtr xmldoc = NULL;
    char *str;
    int len;
    HRESULT hr = S_FALSE, hr2;

    TRACE("(%p)->(%s %p)\n", This, debugstr_w( bstrXML ), isSuccessful );

    assert ( &This->node );

    if ( isSuccessful )
    {
        *isSuccessful = VARIANT_FALSE;

        if ( bstrXML  && bstr_to_utf8( bstrXML, &str, &len ) )
        {
            xmldoc = doparse( str, len );
            heap_free( str );
            if ( !xmldoc )
                This->error = E_FAIL;
            else
            {
                hr = This->error = S_OK;
                *isSuccessful = VARIANT_TRUE;
            }
        }
    }
    if(!xmldoc)
        xmldoc = xmlNewDoc(NULL);

    xmldoc->_private = create_priv();
    hr2 = attach_xmldoc( &This->node, xmldoc );
    if( FAILED(hr2) )
        hr = hr2;

    return hr;
}

static int XMLCALL domdoc_save_writecallback(void *ctx, const char *buffer,
                                             int len)
{
    DWORD written = -1;

    if(!WriteFile(ctx, buffer, len, &written, NULL))
    {
        WARN("write error\n");
        return -1;
    }
    else
        return written;
}

static int XMLCALL domdoc_save_closecallback(void *ctx)
{
    return CloseHandle(ctx) ? 0 : -1;
}

static HRESULT WINAPI domdoc_save(
    IXMLDOMDocument2 *iface,
    VARIANT destination )
{
    domdoc *This = impl_from_IXMLDOMDocument2( iface );
    HANDLE handle;
    xmlSaveCtxtPtr ctx;
    HRESULT ret = S_OK;

    TRACE("(%p)->(var(vt %d, %s))\n", This, V_VT(&destination),
          V_VT(&destination) == VT_BSTR ? debugstr_w(V_BSTR(&destination)) : NULL);

    if(V_VT(&destination) != VT_BSTR && V_VT(&destination) != VT_UNKNOWN)
    {
        FIXME("Unhandled vt %d\n", V_VT(&destination));
        return S_FALSE;
    }

    if(V_VT(&destination) == VT_UNKNOWN)
    {
        IUnknown *pUnk = V_UNKNOWN(&destination);
        IXMLDOMDocument *pDocument;

        ret = IXMLDOMDocument_QueryInterface(pUnk, &IID_IXMLDOMDocument2, (void**)&pDocument);
        if(ret == S_OK)
        {
            BSTR bXML;
            VARIANT_BOOL bSuccessful;

            ret = IXMLDOMDocument_get_xml(iface, &bXML);
            if(ret == S_OK)
            {
                ret = IXMLDOMDocument_loadXML(pDocument, bXML, &bSuccessful);

                SysFreeString(bXML);
            }

            IXMLDOMDocument_Release(pDocument);
        }

        TRACE("ret %d\n", ret);

        return ret;
    }

    handle = CreateFileW( V_BSTR(&destination), GENERIC_WRITE, 0,
                          NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
    if( handle == INVALID_HANDLE_VALUE )
    {
        WARN("failed to create file\n");
        return S_FALSE;
    }

    /* disable top XML declaration */
    ctx = xmlSaveToIO(domdoc_save_writecallback, domdoc_save_closecallback,
                      handle, NULL, XML_SAVE_NO_DECL);
    if (!ctx)
    {
        CloseHandle(handle);
        return S_FALSE;
    }

    if (xmlSaveDoc(ctx, get_doc(This)) == -1) ret = S_FALSE;
    /* will close file through close callback */
    xmlSaveClose(ctx);

    return ret;
}

static HRESULT WINAPI domdoc_get_validateOnParse(
    IXMLDOMDocument2 *iface,
    VARIANT_BOOL* isValidating )
{
    domdoc *This = impl_from_IXMLDOMDocument2( iface );

    TRACE("(%p)->(%p: %d)\n", This, isValidating, This->validating);
    *isValidating = This->validating;
    return S_OK;
}


static HRESULT WINAPI domdoc_put_validateOnParse(
    IXMLDOMDocument2 *iface,
    VARIANT_BOOL isValidating )
{
    domdoc *This = impl_from_IXMLDOMDocument2( iface );

    TRACE("(%p)->(%d)\n", This, isValidating);
    This->validating = isValidating;
    return S_OK;
}


static HRESULT WINAPI domdoc_get_resolveExternals(
    IXMLDOMDocument2 *iface,
    VARIANT_BOOL* isResolving )
{
    domdoc *This = impl_from_IXMLDOMDocument2( iface );

    TRACE("(%p)->(%p: %d)\n", This, isResolving, This->resolving);
    *isResolving = This->resolving;
    return S_OK;
}


static HRESULT WINAPI domdoc_put_resolveExternals(
    IXMLDOMDocument2 *iface,
    VARIANT_BOOL isResolving )
{
    domdoc *This = impl_from_IXMLDOMDocument2( iface );

    TRACE("(%p)->(%d)\n", This, isResolving);
    This->resolving = isResolving;
    return S_OK;
}


static HRESULT WINAPI domdoc_get_preserveWhiteSpace(
    IXMLDOMDocument2 *iface,
    VARIANT_BOOL* isPreserving )
{
    domdoc *This = impl_from_IXMLDOMDocument2( iface );

    TRACE("(%p)->(%p: %d)\n", This, isPreserving, This->preserving);
    *isPreserving = This->preserving;
    return S_OK;
}


static HRESULT WINAPI domdoc_put_preserveWhiteSpace(
    IXMLDOMDocument2 *iface,
    VARIANT_BOOL isPreserving )
{
    domdoc *This = impl_from_IXMLDOMDocument2( iface );

    TRACE("(%p)->(%d)\n", This, isPreserving);
    This->preserving = isPreserving;
    return S_OK;
}


static HRESULT WINAPI domdoc_put_onReadyStateChange(
    IXMLDOMDocument2 *iface,
    VARIANT readyStateChangeSink )
{
    domdoc *This = impl_from_IXMLDOMDocument2( iface );
    FIXME("%p\n", This);
    return E_NOTIMPL;
}


static HRESULT WINAPI domdoc_put_onDataAvailable(
    IXMLDOMDocument2 *iface,
    VARIANT onDataAvailableSink )
{
    domdoc *This = impl_from_IXMLDOMDocument2( iface );
    FIXME("%p\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI domdoc_put_onTransformNode(
    IXMLDOMDocument2 *iface,
    VARIANT onTransformNodeSink )
{
    domdoc *This = impl_from_IXMLDOMDocument2( iface );
    FIXME("%p\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI domdoc_get_namespaces(
    IXMLDOMDocument2* iface,
    IXMLDOMSchemaCollection** schemaCollection )
{
    domdoc *This = impl_from_IXMLDOMDocument2( iface );
    FIXME("(%p)->(%p)\n", This, schemaCollection);
    return E_NOTIMPL;
}

static HRESULT WINAPI domdoc_get_schemas(
    IXMLDOMDocument2* iface,
    VARIANT* var1 )
{
    domdoc *This = impl_from_IXMLDOMDocument2( iface );
    HRESULT hr = S_FALSE;
    IXMLDOMSchemaCollection *cur_schema = This->schema;

    TRACE("(%p)->(%p)\n", This, var1);

    VariantInit(var1); /* Test shows we don't call VariantClear here */
    V_VT(var1) = VT_NULL;

    if(cur_schema)
    {
        hr = IXMLDOMSchemaCollection_QueryInterface(cur_schema, &IID_IDispatch, (void**)&V_DISPATCH(var1));
        if(SUCCEEDED(hr))
            V_VT(var1) = VT_DISPATCH;
    }
    return hr;
}

static HRESULT WINAPI domdoc_putref_schemas(
    IXMLDOMDocument2* iface,
    VARIANT var1)
{
    domdoc *This = impl_from_IXMLDOMDocument2( iface );
    HRESULT hr = E_FAIL;
    IXMLDOMSchemaCollection *new_schema = NULL;

    FIXME("(%p): semi-stub\n", This);
    switch(V_VT(&var1))
    {
    case VT_UNKNOWN:
        hr = IUnknown_QueryInterface(V_UNKNOWN(&var1), &IID_IXMLDOMSchemaCollection, (void**)&new_schema);
        break;

    case VT_DISPATCH:
        hr = IDispatch_QueryInterface(V_DISPATCH(&var1), &IID_IXMLDOMSchemaCollection, (void**)&new_schema);
        break;

    case VT_NULL:
    case VT_EMPTY:
        hr = S_OK;
        break;

    default:
        WARN("Can't get schema from vt %x\n", V_VT(&var1));
    }

    if(SUCCEEDED(hr))
    {
        IXMLDOMSchemaCollection *old_schema = InterlockedExchangePointer((void**)&This->schema, new_schema);
        if(old_schema) IXMLDOMSchemaCollection_Release(old_schema);
    }

    return hr;
}

static HRESULT WINAPI domdoc_validate(
    IXMLDOMDocument2* iface,
    IXMLDOMParseError** err)
{
    domdoc *This = impl_from_IXMLDOMDocument2( iface );
    FIXME("(%p)->(%p)\n", This, err);
    return E_NOTIMPL;
}

static HRESULT WINAPI domdoc_setProperty(
    IXMLDOMDocument2* iface,
    BSTR p,
    VARIANT var)
{
    domdoc *This = impl_from_IXMLDOMDocument2( iface );

    TRACE("(%p)->(%s)\n", This, debugstr_w(p));

    if (lstrcmpiW(p, SZ_PROPERTY_SELECTION_LANGUAGE) == 0)
    {
        VARIANT varStr;
        HRESULT hr;
        BSTR bstr;

        V_VT(&varStr) = VT_EMPTY;
        if (V_VT(&var) != VT_BSTR)
        {
            if (FAILED(hr = VariantChangeType(&varStr, &var, 0, VT_BSTR)))
                return hr;
            bstr = V_BSTR(&varStr);
        }
        else
            bstr = V_BSTR(&var);

        hr = S_OK;
        if (lstrcmpiW(bstr, SZ_VALUE_XPATH) == 0)
            This->bUseXPath = TRUE;
        else if (lstrcmpiW(bstr, SZ_VALUE_XSLPATTERN) == 0)
            This->bUseXPath = FALSE;
        else
            hr = E_FAIL;

        VariantClear(&varStr);
        return hr;
    }

    FIXME("Unknown property %s\n", wine_dbgstr_w(p));
    return E_FAIL;
}

static HRESULT WINAPI domdoc_getProperty(
    IXMLDOMDocument2* iface,
    BSTR p,
    VARIANT* var)
{
    domdoc *This = impl_from_IXMLDOMDocument2( iface );

    TRACE("(%p)->(%p)\n", This, debugstr_w(p));

    if (var == NULL)
        return E_INVALIDARG;
    if (lstrcmpiW(p, SZ_PROPERTY_SELECTION_LANGUAGE) == 0)
    {
        V_VT(var) = VT_BSTR;
        if (This->bUseXPath)
            V_BSTR(var) = SysAllocString(SZ_VALUE_XPATH);
        else
            V_BSTR(var) = SysAllocString(SZ_VALUE_XSLPATTERN);
        return S_OK;
    }

    FIXME("Unknown property %s\n", wine_dbgstr_w(p));
    return E_FAIL;
}

static const struct IXMLDOMDocument2Vtbl domdoc_vtbl =
{
    domdoc_QueryInterface,
    domdoc_AddRef,
    domdoc_Release,
    domdoc_GetTypeInfoCount,
    domdoc_GetTypeInfo,
    domdoc_GetIDsOfNames,
    domdoc_Invoke,
    domdoc_get_nodeName,
    domdoc_get_nodeValue,
    domdoc_put_nodeValue,
    domdoc_get_nodeType,
    domdoc_get_parentNode,
    domdoc_get_childNodes,
    domdoc_get_firstChild,
    domdoc_get_lastChild,
    domdoc_get_previousSibling,
    domdoc_get_nextSibling,
    domdoc_get_attributes,
    domdoc_insertBefore,
    domdoc_replaceChild,
    domdoc_removeChild,
    domdoc_appendChild,
    domdoc_hasChildNodes,
    domdoc_get_ownerDocument,
    domdoc_cloneNode,
    domdoc_get_nodeTypeString,
    domdoc_get_text,
    domdoc_put_text,
    domdoc_get_specified,
    domdoc_get_definition,
    domdoc_get_nodeTypedValue,
    domdoc_put_nodeTypedValue,
    domdoc_get_dataType,
    domdoc_put_dataType,
    domdoc_get_xml,
    domdoc_transformNode,
    domdoc_selectNodes,
    domdoc_selectSingleNode,
    domdoc_get_parsed,
    domdoc_get_namespaceURI,
    domdoc_get_prefix,
    domdoc_get_baseName,
    domdoc_transformNodeToObject,
    domdoc_get_doctype,
    domdoc_get_implementation,
    domdoc_get_documentElement,
    domdoc_put_documentElement,
    domdoc_createElement,
    domdoc_createDocumentFragment,
    domdoc_createTextNode,
    domdoc_createComment,
    domdoc_createCDATASection,
    domdoc_createProcessingInstruction,
    domdoc_createAttribute,
    domdoc_createEntityReference,
    domdoc_getElementsByTagName,
    domdoc_createNode,
    domdoc_nodeFromID,
    domdoc_load,
    domdoc_get_readyState,
    domdoc_get_parseError,
    domdoc_get_url,
    domdoc_get_async,
    domdoc_put_async,
    domdoc_abort,
    domdoc_loadXML,
    domdoc_save,
    domdoc_get_validateOnParse,
    domdoc_put_validateOnParse,
    domdoc_get_resolveExternals,
    domdoc_put_resolveExternals,
    domdoc_get_preserveWhiteSpace,
    domdoc_put_preserveWhiteSpace,
    domdoc_put_onReadyStateChange,
    domdoc_put_onDataAvailable,
    domdoc_put_onTransformNode,
    domdoc_get_namespaces,
    domdoc_get_schemas,
    domdoc_putref_schemas,
    domdoc_validate,
    domdoc_setProperty,
    domdoc_getProperty
};

/* xmldoc implementation of IObjectWithSite */
static HRESULT WINAPI
xmldoc_ObjectWithSite_QueryInterface( IObjectWithSite* iface, REFIID riid, void** ppvObject )
{
    domdoc *This = impl_from_IObjectWithSite(iface);
    return IXMLDocument_QueryInterface( (IXMLDocument *)This, riid, ppvObject );
}

static ULONG WINAPI
xmldoc_ObjectWithSite_AddRef( IObjectWithSite* iface )
{
    domdoc *This = impl_from_IObjectWithSite(iface);
    return IXMLDocument_AddRef((IXMLDocument *)This);
}

static ULONG WINAPI
xmldoc_ObjectWithSite_Release( IObjectWithSite* iface )
{
    domdoc *This = impl_from_IObjectWithSite(iface);
    return IXMLDocument_Release((IXMLDocument *)This);
}

static HRESULT WINAPI
xmldoc_GetSite( IObjectWithSite *iface, REFIID iid, void ** ppvSite )
{
    domdoc *This = impl_from_IObjectWithSite(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_guid( iid ), ppvSite );

    if ( !This->site )
        return E_FAIL;

    return IUnknown_QueryInterface( This->site, iid, ppvSite );
}

static HRESULT WINAPI
xmldoc_SetSite( IObjectWithSite *iface, IUnknown *punk )
{
    domdoc *This = impl_from_IObjectWithSite(iface);

    TRACE("(%p)->(%p)\n", iface, punk);

    if(!punk)
    {
        if(This->site)
        {
            IUnknown_Release( This->site );
            This->site = NULL;
        }

        return S_OK;
    }

    if ( punk )
        IUnknown_AddRef( punk );

    if(This->site)
        IUnknown_Release( This->site );

    This->site = punk;

    return S_OK;
}

static const IObjectWithSiteVtbl domdocObjectSite =
{
    xmldoc_ObjectWithSite_QueryInterface,
    xmldoc_ObjectWithSite_AddRef,
    xmldoc_ObjectWithSite_Release,
    xmldoc_SetSite,
    xmldoc_GetSite,
};

static HRESULT WINAPI xmldoc_Safety_QueryInterface(IObjectSafety *iface, REFIID riid, void **ppv)
{
    domdoc *This = impl_from_IObjectSafety(iface);
    return IXMLDocument_QueryInterface( (IXMLDocument *)This, riid, ppv );
}

static ULONG WINAPI xmldoc_Safety_AddRef(IObjectSafety *iface)
{
    domdoc *This = impl_from_IObjectSafety(iface);
    return IXMLDocument_AddRef((IXMLDocument *)This);
}

static ULONG WINAPI xmldoc_Safety_Release(IObjectSafety *iface)
{
    domdoc *This = impl_from_IObjectSafety(iface);
    return IXMLDocument_Release((IXMLDocument *)This);
}

#define SAFETY_SUPPORTED_OPTIONS (INTERFACESAFE_FOR_UNTRUSTED_CALLER|INTERFACESAFE_FOR_UNTRUSTED_DATA|INTERFACE_USES_SECURITY_MANAGER)

static HRESULT WINAPI xmldoc_Safety_GetInterfaceSafetyOptions(IObjectSafety *iface, REFIID riid,
        DWORD *pdwSupportedOptions, DWORD *pdwEnabledOptions)
{
    domdoc *This = impl_from_IObjectSafety(iface);

    TRACE("(%p)->(%s %p %p)\n", This, debugstr_guid(riid), pdwSupportedOptions, pdwEnabledOptions);

    if(!pdwSupportedOptions || !pdwEnabledOptions)
        return E_POINTER;

    *pdwSupportedOptions = SAFETY_SUPPORTED_OPTIONS;
    *pdwEnabledOptions = This->safeopt;

    return S_OK;
}

static HRESULT WINAPI xmldoc_Safety_SetInterfaceSafetyOptions(IObjectSafety *iface, REFIID riid,
        DWORD dwOptionSetMask, DWORD dwEnabledOptions)
{
    domdoc *This = impl_from_IObjectSafety(iface);
    TRACE("(%p)->(%s %x %x)\n", This, debugstr_guid(riid), dwOptionSetMask, dwEnabledOptions);

    if ((dwOptionSetMask & ~SAFETY_SUPPORTED_OPTIONS) != 0)
        return E_FAIL;

    This->safeopt = dwEnabledOptions & dwOptionSetMask & SAFETY_SUPPORTED_OPTIONS;
    return S_OK;
}

static const IObjectSafetyVtbl domdocObjectSafetyVtbl = {
    xmldoc_Safety_QueryInterface,
    xmldoc_Safety_AddRef,
    xmldoc_Safety_Release,
    xmldoc_Safety_GetInterfaceSafetyOptions,
    xmldoc_Safety_SetInterfaceSafetyOptions
};


static const tid_t domdoc_iface_tids[] = {
    IXMLDOMNode_tid,
    IXMLDOMDocument_tid,
    IXMLDOMDocument2_tid,
    0
};
static dispex_static_data_t domdoc_dispex = {
    NULL,
    IXMLDOMDocument2_tid,
    NULL,
    domdoc_iface_tids
};

HRESULT DOMDocument_create_from_xmldoc(xmlDocPtr xmldoc, IXMLDOMDocument2 **document)
{
    domdoc *doc;

    doc = heap_alloc( sizeof (*doc) );
    if( !doc )
        return E_OUTOFMEMORY;

    doc->lpVtbl = &domdoc_vtbl;
    doc->lpvtblIPersistStream = &xmldoc_IPersistStream_VTable;
    doc->lpvtblIObjectWithSite = &domdocObjectSite;
    doc->lpvtblIObjectSafety = &domdocObjectSafetyVtbl;
    doc->lpvtblISupportErrorInfo = &support_error_vtbl;
    doc->ref = 1;
    doc->async = VARIANT_TRUE;
    doc->validating = 0;
    doc->resolving = 0;
    doc->preserving = 0;
    doc->bUseXPath = FALSE;
    doc->error = S_OK;
    doc->schema = NULL;
    doc->stream = NULL;
    doc->site = NULL;
    doc->safeopt = 0;
    doc->bsc = NULL;

    init_xmlnode(&doc->node, (xmlNodePtr)xmldoc, (IUnknown*)&doc->lpVtbl, &domdoc_dispex);

    *document = (IXMLDOMDocument2*)&doc->lpVtbl;

    TRACE("returning iface %p\n", *document);
    return S_OK;
}

HRESULT DOMDocument_create(IUnknown *pUnkOuter, LPVOID *ppObj)
{
    xmlDocPtr xmldoc;
    HRESULT hr;

    TRACE("(%p,%p)\n", pUnkOuter, ppObj);

    xmldoc = xmlNewDoc(NULL);
    if(!xmldoc)
        return E_OUTOFMEMORY;

    xmldoc->_private = create_priv();

    hr = DOMDocument_create_from_xmldoc(xmldoc, (IXMLDOMDocument2**)ppObj);
    if(FAILED(hr))
        xmlFreeDoc(xmldoc);

    return hr;
}

IUnknown* create_domdoc( xmlNodePtr document )
{
    HRESULT hr;
    LPVOID pObj = NULL;

    TRACE("(%p)\n", document);

    hr = DOMDocument_create_from_xmldoc((xmlDocPtr)document, (IXMLDOMDocument2**)&pObj);
    if (FAILED(hr))
        return NULL;

    return pObj;
}

#else

HRESULT DOMDocument_create(IUnknown *pUnkOuter, LPVOID *ppObj)
{
    MESSAGE("This program tried to use a DOMDocument object, but\n"
            "libxml2 support was not present at compile time.\n");
    return E_NOTIMPL;
}

#endif
