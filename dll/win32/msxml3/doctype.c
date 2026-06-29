/*
 *    DOM DTD node implementation
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
#include <libxml/parser.h>
#include <libxml/xmlerror.h>

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "winnls.h"
#include "ole2.h"
#include "msxml6.h"

#include "msxml_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(msxml);

typedef struct _domdoctype
{
    xmlnode node;
    IXMLDOMDocumentType IXMLDOMDocumentType_iface;
    LONG ref;
} domdoctype;

static inline domdoctype *impl_from_IXMLDOMDocumentType( IXMLDOMDocumentType *iface )
{
    return CONTAINING_RECORD(iface, domdoctype, IXMLDOMDocumentType_iface);
}

static HRESULT WINAPI domdoctype_QueryInterface(
    IXMLDOMDocumentType *iface,
    REFIID riid,
    void** ppvObject )
{
    domdoctype *This = impl_from_IXMLDOMDocumentType( iface );

    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppvObject);

    if ( IsEqualGUID( riid, &IID_IXMLDOMDocumentType ) ||
         IsEqualGUID( riid, &IID_IXMLDOMNode ) ||
         IsEqualGUID( riid, &IID_IDispatch ) ||
         IsEqualGUID( riid, &IID_IUnknown ) )
    {
        *ppvObject = &This->IXMLDOMDocumentType_iface;
    }
    else if(node_query_interface(&This->node, riid, ppvObject))
    {
        return *ppvObject ? S_OK : E_NOINTERFACE;
    }
    else
    {
        TRACE("interface %s not implemented\n", debugstr_guid(riid));
        *ppvObject = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef( (IUnknown*)*ppvObject );
    return S_OK;
}

static ULONG WINAPI domdoctype_AddRef(IXMLDOMDocumentType *iface)
{
    domdoctype *doctype = impl_from_IXMLDOMDocumentType(iface);
    LONG ref = InterlockedIncrement(&doctype->ref);
    TRACE("%p, refcount %ld.\n", iface, ref);
    return ref;
}

static ULONG WINAPI domdoctype_Release(IXMLDOMDocumentType *iface)
{
    domdoctype *doctype = impl_from_IXMLDOMDocumentType(iface);
    ULONG ref = InterlockedDecrement(&doctype->ref);

    TRACE("%p, refcount %ld.\n", iface, ref);

    if (!ref)
    {
        destroy_xmlnode(&doctype->node);
        free(doctype);
    }

    return ref;
}

static HRESULT WINAPI domdoctype_GetTypeInfoCount(
    IXMLDOMDocumentType *iface,
    UINT* pctinfo )
{
    domdoctype *This = impl_from_IXMLDOMDocumentType( iface );
    return IDispatchEx_GetTypeInfoCount(&This->node.dispex.IDispatchEx_iface, pctinfo);
}

static HRESULT WINAPI domdoctype_GetTypeInfo(
    IXMLDOMDocumentType *iface,
    UINT iTInfo, LCID lcid,
    ITypeInfo** ppTInfo )
{
    domdoctype *This = impl_from_IXMLDOMDocumentType( iface );
    return IDispatchEx_GetTypeInfo(&This->node.dispex.IDispatchEx_iface,
        iTInfo, lcid, ppTInfo);
}

static HRESULT WINAPI domdoctype_GetIDsOfNames(
    IXMLDOMDocumentType *iface,
    REFIID riid, LPOLESTR* rgszNames,
    UINT cNames, LCID lcid, DISPID* rgDispId )
{
    domdoctype *This = impl_from_IXMLDOMDocumentType( iface );
    return IDispatchEx_GetIDsOfNames(&This->node.dispex.IDispatchEx_iface,
        riid, rgszNames, cNames, lcid, rgDispId);
}

static HRESULT WINAPI domdoctype_Invoke(
    IXMLDOMDocumentType *iface,
    DISPID dispIdMember, REFIID riid, LCID lcid,
    WORD wFlags, DISPPARAMS* pDispParams, VARIANT* pVarResult,
    EXCEPINFO* pExcepInfo, UINT* puArgErr )
{
    domdoctype *This = impl_from_IXMLDOMDocumentType( iface );
    return IDispatchEx_Invoke(&This->node.dispex.IDispatchEx_iface,
        dispIdMember, riid, lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI domdoctype_get_nodeName(
    IXMLDOMDocumentType *iface,
    BSTR* p )
{
    domdoctype *This = impl_from_IXMLDOMDocumentType( iface );
    TRACE("(%p)->(%p)\n", This, p);
    return node_get_nodeName(&This->node, p);
}

static HRESULT WINAPI domdoctype_get_nodeValue(
    IXMLDOMDocumentType *iface,
    VARIANT* value)
{
    domdoctype *This = impl_from_IXMLDOMDocumentType( iface );
    FIXME("(%p)->(%p): stub\n", This, value);
    return E_NOTIMPL;
}

static HRESULT WINAPI domdoctype_put_nodeValue(
    IXMLDOMDocumentType *iface,
    VARIANT value)
{
    domdoctype *This = impl_from_IXMLDOMDocumentType( iface );
    FIXME("(%p)->(%s): stub\n", This, debugstr_variant(&value));
    return E_NOTIMPL;
}

static HRESULT WINAPI domdoctype_get_nodeType(
    IXMLDOMDocumentType *iface,
    DOMNodeType* domNodeType )
{
    domdoctype *This = impl_from_IXMLDOMDocumentType( iface );

    TRACE("(%p)->(%p)\n", This, domNodeType);

    *domNodeType = NODE_DOCUMENT_TYPE;
    return S_OK;
}

static HRESULT WINAPI domdoctype_get_parentNode(
    IXMLDOMDocumentType *iface,
    IXMLDOMNode** parent )
{
    domdoctype *This = impl_from_IXMLDOMDocumentType( iface );
    FIXME("(%p)->(%p): stub\n", This, parent);
    return E_NOTIMPL;
}

static HRESULT WINAPI domdoctype_get_childNodes(
    IXMLDOMDocumentType *iface,
    IXMLDOMNodeList** outList)
{
    domdoctype *This = impl_from_IXMLDOMDocumentType( iface );
    FIXME("(%p)->(%p): stub\n", This, outList);
    return E_NOTIMPL;
}

static HRESULT WINAPI domdoctype_get_firstChild(
    IXMLDOMDocumentType *iface,
    IXMLDOMNode** domNode)
{
    domdoctype *This = impl_from_IXMLDOMDocumentType( iface );
    FIXME("(%p)->(%p): stub\n", This, domNode);
    return E_NOTIMPL;
}

static HRESULT WINAPI domdoctype_get_lastChild(
    IXMLDOMDocumentType *iface,
    IXMLDOMNode** domNode)
{
    domdoctype *This = impl_from_IXMLDOMDocumentType( iface );
    FIXME("(%p)->(%p): stub\n", This, domNode);
    return E_NOTIMPL;
}

static HRESULT WINAPI domdoctype_get_previousSibling(
    IXMLDOMDocumentType *iface,
    IXMLDOMNode** domNode)
{
    domdoctype *This = impl_from_IXMLDOMDocumentType( iface );
    FIXME("(%p)->(%p): stub\n", This, domNode);
    return E_NOTIMPL;
}

static HRESULT WINAPI domdoctype_get_nextSibling(
    IXMLDOMDocumentType *iface,
    IXMLDOMNode** domNode)
{
    domdoctype *This = impl_from_IXMLDOMDocumentType( iface );
    FIXME("(%p)->(%p): stub\n", This, domNode);
    return E_NOTIMPL;
}

static HRESULT WINAPI domdoctype_get_attributes(
    IXMLDOMDocumentType *iface,
    IXMLDOMNamedNodeMap** attributeMap)
{
    domdoctype *This = impl_from_IXMLDOMDocumentType( iface );
    FIXME("(%p)->(%p): stub\n", This, attributeMap);
    return E_NOTIMPL;
}

static HRESULT WINAPI domdoctype_insertBefore(
    IXMLDOMDocumentType *iface,
    IXMLDOMNode* newNode, VARIANT refChild,
    IXMLDOMNode** outOldNode)
{
    domdoctype *This = impl_from_IXMLDOMDocumentType( iface );

    FIXME("(%p)->(%p %s %p): stub\n", This, newNode, debugstr_variant(&refChild), outOldNode);

    return E_NOTIMPL;
}

static HRESULT WINAPI domdoctype_replaceChild(
    IXMLDOMDocumentType *iface,
    IXMLDOMNode* newNode,
    IXMLDOMNode* oldNode,
    IXMLDOMNode** outOldNode)
{
    domdoctype *This = impl_from_IXMLDOMDocumentType( iface );

    FIXME("(%p)->(%p %p %p): stub\n", This, newNode, oldNode, outOldNode);

    return E_NOTIMPL;
}

static HRESULT WINAPI domdoctype_removeChild(
    IXMLDOMDocumentType *iface,
    IXMLDOMNode* domNode, IXMLDOMNode** oldNode)
{
    domdoctype *This = impl_from_IXMLDOMDocumentType( iface );
    FIXME("(%p)->(%p %p): stub\n", This, domNode, oldNode);
    return E_NOTIMPL;
}

static HRESULT WINAPI domdoctype_appendChild(
    IXMLDOMDocumentType *iface,
    IXMLDOMNode* newNode, IXMLDOMNode** outNewNode)
{
    domdoctype *This = impl_from_IXMLDOMDocumentType( iface );
    FIXME("(%p)->(%p %p): stub\n", This, newNode, outNewNode);
    return E_NOTIMPL;
}

static HRESULT WINAPI domdoctype_hasChildNodes(
    IXMLDOMDocumentType *iface,
    VARIANT_BOOL* pbool)
{
    domdoctype *This = impl_from_IXMLDOMDocumentType( iface );
    FIXME("(%p)->(%p): stub\n", This, pbool);
    return E_NOTIMPL;
}

static HRESULT WINAPI domdoctype_get_ownerDocument(
    IXMLDOMDocumentType *iface,
    IXMLDOMDocument** domDocument)
{
    domdoctype *This = impl_from_IXMLDOMDocumentType( iface );
    FIXME("(%p)->(%p): stub\n", This, domDocument);
    return E_NOTIMPL;
}

static HRESULT WINAPI domdoctype_cloneNode(
    IXMLDOMDocumentType *iface,
    VARIANT_BOOL deep, IXMLDOMNode** outNode)
{
    domdoctype *This = impl_from_IXMLDOMDocumentType( iface );
    FIXME("(%p)->(%d %p): stub\n", This, deep, outNode);
    return E_NOTIMPL;
}

static HRESULT WINAPI domdoctype_get_nodeTypeString(
    IXMLDOMDocumentType *iface,
    BSTR* p)
{
    domdoctype *This = impl_from_IXMLDOMDocumentType( iface );
    FIXME("(%p)->(%p): stub\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI domdoctype_get_text(
    IXMLDOMDocumentType *iface,
    BSTR* p)
{
    domdoctype *This = impl_from_IXMLDOMDocumentType( iface );
    FIXME("(%p)->(%p): stub\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI domdoctype_put_text(
    IXMLDOMDocumentType *iface,
    BSTR p)
{
    domdoctype *This = impl_from_IXMLDOMDocumentType( iface );
    FIXME("(%p)->(%s): stub\n", This, debugstr_w(p));
    return E_NOTIMPL;
}

static HRESULT WINAPI domdoctype_get_specified(
    IXMLDOMDocumentType *iface,
    VARIANT_BOOL* isSpecified)
{
    domdoctype *This = impl_from_IXMLDOMDocumentType( iface );
    FIXME("(%p)->(%p): stub\n", This, isSpecified);
    return E_NOTIMPL;
}

static HRESULT WINAPI domdoctype_get_definition(
    IXMLDOMDocumentType *iface,
    IXMLDOMNode** definitionNode)
{
    domdoctype *This = impl_from_IXMLDOMDocumentType( iface );
    FIXME("(%p)->(%p)\n", This, definitionNode);
    return E_NOTIMPL;
}

static HRESULT WINAPI domdoctype_get_nodeTypedValue(
    IXMLDOMDocumentType *iface,
    VARIANT* v)
{
    domdoctype *This = impl_from_IXMLDOMDocumentType( iface );
    TRACE("(%p)->(%p)\n", This, v);
    return return_null_var(v);
}

static HRESULT WINAPI domdoctype_put_nodeTypedValue(
    IXMLDOMDocumentType *iface,
    VARIANT value)
{
    domdoctype *This = impl_from_IXMLDOMDocumentType( iface );
    FIXME("(%p)->(%s): stub\n", This, debugstr_variant(&value));
    return E_NOTIMPL;
}

static HRESULT WINAPI domdoctype_get_dataType(
    IXMLDOMDocumentType *iface,
    VARIANT* typename)
{
    domdoctype *This = impl_from_IXMLDOMDocumentType( iface );
    FIXME("(%p)->(%p): stub\n", This, typename);
    return E_NOTIMPL;
}

static HRESULT WINAPI domdoctype_put_dataType(
    IXMLDOMDocumentType *iface,
    BSTR p)
{
    domdoctype *This = impl_from_IXMLDOMDocumentType( iface );
    FIXME("(%p)->(%s): stub\n", This, debugstr_w(p));
    return E_NOTIMPL;
}

static HRESULT WINAPI domdoctype_get_xml(
    IXMLDOMDocumentType *iface,
    BSTR* p)
{
    domdoctype *This = impl_from_IXMLDOMDocumentType( iface );
    FIXME("(%p)->(%p): stub\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI domdoctype_transformNode(
    IXMLDOMDocumentType *iface,
    IXMLDOMNode* domNode, BSTR* p)
{
    domdoctype *This = impl_from_IXMLDOMDocumentType( iface );
    FIXME("(%p)->(%p %p): stub\n", This, domNode, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI domdoctype_selectNodes(
    IXMLDOMDocumentType *iface,
    BSTR p, IXMLDOMNodeList** outList)
{
    domdoctype *This = impl_from_IXMLDOMDocumentType( iface );
    FIXME("(%p)->(%s %p): stub\n", This, debugstr_w(p), outList);
    return E_NOTIMPL;
}

static HRESULT WINAPI domdoctype_selectSingleNode(
    IXMLDOMDocumentType *iface,
    BSTR p, IXMLDOMNode** outNode)
{
    domdoctype *This = impl_from_IXMLDOMDocumentType( iface );
    FIXME("(%p)->(%s %p): stub\n", This, debugstr_w(p), outNode);
    return E_NOTIMPL;
}

static HRESULT WINAPI domdoctype_get_parsed(
    IXMLDOMDocumentType *iface,
    VARIANT_BOOL* isParsed)
{
    domdoctype *This = impl_from_IXMLDOMDocumentType( iface );
    FIXME("(%p)->(%p): stub\n", This, isParsed);
    return E_NOTIMPL;
}

static HRESULT WINAPI domdoctype_get_namespaceURI(
    IXMLDOMDocumentType *iface,
    BSTR* p)
{
    domdoctype *This = impl_from_IXMLDOMDocumentType( iface );
    FIXME("(%p)->(%p): stub\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI domdoctype_get_prefix(
    IXMLDOMDocumentType *iface,
    BSTR* prefix)
{
    domdoctype *This = impl_from_IXMLDOMDocumentType( iface );
    FIXME("(%p)->(%p): stub\n", This, prefix);
    return E_NOTIMPL;
}

static HRESULT WINAPI domdoctype_get_baseName(
    IXMLDOMDocumentType *iface,
    BSTR* name)
{
    domdoctype *This = impl_from_IXMLDOMDocumentType( iface );
    FIXME("(%p)->(%p): stub\n", This, name);
    return E_NOTIMPL;
}

static HRESULT WINAPI domdoctype_transformNodeToObject(
    IXMLDOMDocumentType *iface,
    IXMLDOMNode* domNode, VARIANT var1)
{
    domdoctype *This = impl_from_IXMLDOMDocumentType( iface );
    FIXME("(%p)->(%p %s): stub\n", This, domNode, debugstr_variant(&var1));
    return E_NOTIMPL;
}

static HRESULT WINAPI domdoctype_get_name(
    IXMLDOMDocumentType *iface,
    BSTR *p)
{
    domdoctype *This = impl_from_IXMLDOMDocumentType( iface );
    TRACE("(%p)->(%p)\n", This, p);
    return node_get_nodeName(&This->node, p);
}

static HRESULT WINAPI domdoctype_get_entities(
    IXMLDOMDocumentType *iface,
    IXMLDOMNamedNodeMap **entityMap)
{
    domdoctype *This = impl_from_IXMLDOMDocumentType( iface );
    FIXME("(%p)->(%p): stub\n", This, entityMap);
    return E_NOTIMPL;
}

static HRESULT WINAPI domdoctype_get_notations(
    IXMLDOMDocumentType *iface,
    IXMLDOMNamedNodeMap **notationMap)
{
    domdoctype *This = impl_from_IXMLDOMDocumentType( iface );
    FIXME("(%p)->(%p): stub\n", This, notationMap);
    return E_NOTIMPL;
}

static const struct IXMLDOMDocumentTypeVtbl domdoctype_vtbl =
{
    domdoctype_QueryInterface,
    domdoctype_AddRef,
    domdoctype_Release,
    domdoctype_GetTypeInfoCount,
    domdoctype_GetTypeInfo,
    domdoctype_GetIDsOfNames,
    domdoctype_Invoke,
    domdoctype_get_nodeName,
    domdoctype_get_nodeValue,
    domdoctype_put_nodeValue,
    domdoctype_get_nodeType,
    domdoctype_get_parentNode,
    domdoctype_get_childNodes,
    domdoctype_get_firstChild,
    domdoctype_get_lastChild,
    domdoctype_get_previousSibling,
    domdoctype_get_nextSibling,
    domdoctype_get_attributes,
    domdoctype_insertBefore,
    domdoctype_replaceChild,
    domdoctype_removeChild,
    domdoctype_appendChild,
    domdoctype_hasChildNodes,
    domdoctype_get_ownerDocument,
    domdoctype_cloneNode,
    domdoctype_get_nodeTypeString,
    domdoctype_get_text,
    domdoctype_put_text,
    domdoctype_get_specified,
    domdoctype_get_definition,
    domdoctype_get_nodeTypedValue,
    domdoctype_put_nodeTypedValue,
    domdoctype_get_dataType,
    domdoctype_put_dataType,
    domdoctype_get_xml,
    domdoctype_transformNode,
    domdoctype_selectNodes,
    domdoctype_selectSingleNode,
    domdoctype_get_parsed,
    domdoctype_get_namespaceURI,
    domdoctype_get_prefix,
    domdoctype_get_baseName,
    domdoctype_transformNodeToObject,
    domdoctype_get_name,
    domdoctype_get_entities,
    domdoctype_get_notations
};

static const tid_t domdoctype_iface_tids[] = {
    IXMLDOMDocumentType_tid,
    0
};

static dispex_static_data_t domdoctype_dispex = {
    NULL,
    IXMLDOMDocumentType_tid,
    NULL,
    domdoctype_iface_tids
};

IUnknown* create_doc_type( xmlNodePtr doctype )
{
    domdoctype *This;

    This = malloc(sizeof(*This));
    if ( !This )
        return NULL;

    This->IXMLDOMDocumentType_iface.lpVtbl = &domdoctype_vtbl;
    This->ref = 1;

    init_xmlnode(&This->node, doctype, (IXMLDOMNode*)&This->IXMLDOMDocumentType_iface,
            &domdoctype_dispex);

    return (IUnknown*)&This->IXMLDOMDocumentType_iface;
}
