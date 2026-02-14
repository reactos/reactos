/*
 *    DOM comment node implementation
 *
 * Copyright 2006 Huw Davies
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
#include "ole2.h"
#include "msxml6.h"

#include "msxml_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(msxml);

typedef struct _domcomment
{
    xmlnode node;
    IXMLDOMComment IXMLDOMComment_iface;
    LONG ref;
} domcomment;

static const tid_t domcomment_se_tids[] = {
    IXMLDOMNode_tid,
    IXMLDOMComment_tid,
    NULL_tid
};

static inline domcomment *impl_from_IXMLDOMComment( IXMLDOMComment *iface )
{
    return CONTAINING_RECORD(iface, domcomment, IXMLDOMComment_iface);
}

static HRESULT WINAPI domcomment_QueryInterface(
    IXMLDOMComment *iface,
    REFIID riid,
    void** ppvObject )
{
    domcomment *This = impl_from_IXMLDOMComment( iface );
    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppvObject);

    if ( IsEqualGUID( riid, &IID_IXMLDOMComment ) ||
         IsEqualGUID( riid, &IID_IXMLDOMCharacterData) ||
         IsEqualGUID( riid, &IID_IXMLDOMNode ) ||
         IsEqualGUID( riid, &IID_IDispatch ) ||
         IsEqualGUID( riid, &IID_IUnknown ) )
    {
        *ppvObject = iface;
    }
    else if(node_query_interface(&This->node, riid, ppvObject))
    {
        return *ppvObject ? S_OK : E_NOINTERFACE;
    }
    else if(IsEqualGUID( riid, &IID_ISupportErrorInfo ))
    {
        return node_create_supporterrorinfo(domcomment_se_tids, ppvObject);
    }
    else
    {
        TRACE("Unsupported interface %s\n", debugstr_guid(riid));
        *ppvObject = NULL;
        return E_NOINTERFACE;
    }

    IXMLDOMComment_AddRef(iface);
    return S_OK;
}

static ULONG WINAPI domcomment_AddRef(IXMLDOMComment *iface)
{
    domcomment *comment = impl_from_IXMLDOMComment(iface);
    ULONG ref = InterlockedIncrement(&comment->ref);
    TRACE("%p, refcount %lu.\n", iface, ref);
    return ref;
}

static ULONG WINAPI domcomment_Release(IXMLDOMComment *iface)
{
    domcomment *comment = impl_from_IXMLDOMComment(iface);
    ULONG ref = InterlockedDecrement(&comment->ref);

    TRACE("%p, refcount %lu.\n", iface, ref);

    if (!ref)
    {
        destroy_xmlnode(&comment->node);
        free(comment);
    }

    return ref;
}

static HRESULT WINAPI domcomment_GetTypeInfoCount(
    IXMLDOMComment *iface,
    UINT* pctinfo )
{
    domcomment *This = impl_from_IXMLDOMComment( iface );
    return IDispatchEx_GetTypeInfoCount(&This->node.dispex.IDispatchEx_iface, pctinfo);
}

static HRESULT WINAPI domcomment_GetTypeInfo(
    IXMLDOMComment *iface,
    UINT iTInfo, LCID lcid,
    ITypeInfo** ppTInfo )
{
    domcomment *This = impl_from_IXMLDOMComment( iface );
    return IDispatchEx_GetTypeInfo(&This->node.dispex.IDispatchEx_iface,
        iTInfo, lcid, ppTInfo);
}

static HRESULT WINAPI domcomment_GetIDsOfNames(
    IXMLDOMComment *iface,
    REFIID riid, LPOLESTR* rgszNames,
    UINT cNames, LCID lcid, DISPID* rgDispId )
{
    domcomment *This = impl_from_IXMLDOMComment( iface );
    return IDispatchEx_GetIDsOfNames(&This->node.dispex.IDispatchEx_iface,
        riid, rgszNames, cNames, lcid, rgDispId);
}

static HRESULT WINAPI domcomment_Invoke(
    IXMLDOMComment *iface,
    DISPID dispIdMember, REFIID riid, LCID lcid,
    WORD wFlags, DISPPARAMS* pDispParams, VARIANT* pVarResult,
    EXCEPINFO* pExcepInfo, UINT* puArgErr )
{
    domcomment *This = impl_from_IXMLDOMComment( iface );
    return IDispatchEx_Invoke(&This->node.dispex.IDispatchEx_iface,
        dispIdMember, riid, lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI domcomment_get_nodeName(
    IXMLDOMComment *iface,
    BSTR* p )
{
    domcomment *This = impl_from_IXMLDOMComment( iface );

    static const WCHAR commentW[] = {'#','c','o','m','m','e','n','t',0};

    TRACE("(%p)->(%p)\n", This, p);

    return return_bstr(commentW, p);
}

static HRESULT WINAPI domcomment_get_nodeValue(
    IXMLDOMComment *iface,
    VARIANT* value)
{
    domcomment *This = impl_from_IXMLDOMComment( iface );

    TRACE("(%p)->(%p)\n", This, value);

    return node_get_content(&This->node, value);
}

static HRESULT WINAPI domcomment_put_nodeValue(
    IXMLDOMComment *iface,
    VARIANT value)
{
    domcomment *This = impl_from_IXMLDOMComment( iface );

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&value));

    return node_put_value(&This->node, &value);
}

static HRESULT WINAPI domcomment_get_nodeType(
    IXMLDOMComment *iface,
    DOMNodeType* domNodeType )
{
    domcomment *This = impl_from_IXMLDOMComment( iface );

    TRACE("(%p)->(%p)\n", This, domNodeType);

    *domNodeType = NODE_COMMENT;
    return S_OK;
}

static HRESULT WINAPI domcomment_get_parentNode(
    IXMLDOMComment *iface,
    IXMLDOMNode** parent )
{
    domcomment *This = impl_from_IXMLDOMComment( iface );

    TRACE("(%p)->(%p)\n", This, parent);

    return node_get_parent(&This->node, parent);
}

static HRESULT WINAPI domcomment_get_childNodes(
    IXMLDOMComment *iface,
    IXMLDOMNodeList** outList)
{
    domcomment *This = impl_from_IXMLDOMComment( iface );

    TRACE("(%p)->(%p)\n", This, outList);

    return node_get_child_nodes(&This->node, outList);
}

static HRESULT WINAPI domcomment_get_firstChild(
    IXMLDOMComment *iface,
    IXMLDOMNode** domNode)
{
    domcomment *This = impl_from_IXMLDOMComment( iface );

    TRACE("(%p)->(%p)\n", This, domNode);

    return return_null_node(domNode);
}

static HRESULT WINAPI domcomment_get_lastChild(
    IXMLDOMComment *iface,
    IXMLDOMNode** domNode)
{
    domcomment *This = impl_from_IXMLDOMComment( iface );

    TRACE("(%p)->(%p)\n", This, domNode);

    return return_null_node(domNode);
}

static HRESULT WINAPI domcomment_get_previousSibling(
    IXMLDOMComment *iface,
    IXMLDOMNode** domNode)
{
    domcomment *This = impl_from_IXMLDOMComment( iface );

    TRACE("(%p)->(%p)\n", This, domNode);

    return node_get_previous_sibling(&This->node, domNode);
}

static HRESULT WINAPI domcomment_get_nextSibling(
    IXMLDOMComment *iface,
    IXMLDOMNode** domNode)
{
    domcomment *This = impl_from_IXMLDOMComment( iface );

    TRACE("(%p)->(%p)\n", This, domNode);

    return node_get_next_sibling(&This->node, domNode);
}

static HRESULT WINAPI domcomment_get_attributes(
    IXMLDOMComment *iface,
    IXMLDOMNamedNodeMap** attributeMap)
{
    domcomment *This = impl_from_IXMLDOMComment( iface );

    TRACE("(%p)->(%p)\n", This, attributeMap);

    return return_null_ptr((void**)attributeMap);
}

static HRESULT WINAPI domcomment_insertBefore(
    IXMLDOMComment *iface,
    IXMLDOMNode* newNode, VARIANT refChild,
    IXMLDOMNode** outOldNode)
{
    domcomment *This = impl_from_IXMLDOMComment( iface );

    FIXME("(%p)->(%p %s %p) needs test\n", This, newNode, debugstr_variant(&refChild), outOldNode);

    return node_insert_before(&This->node, newNode, &refChild, outOldNode);
}

static HRESULT WINAPI domcomment_replaceChild(
    IXMLDOMComment *iface,
    IXMLDOMNode* newNode,
    IXMLDOMNode* oldNode,
    IXMLDOMNode** outOldNode)
{
    domcomment *This = impl_from_IXMLDOMComment( iface );

    FIXME("(%p)->(%p %p %p) needs tests\n", This, newNode, oldNode, outOldNode);

    return node_replace_child(&This->node, newNode, oldNode, outOldNode);
}

static HRESULT WINAPI domcomment_removeChild(
    IXMLDOMComment *iface,
    IXMLDOMNode *child, IXMLDOMNode **oldChild)
{
    domcomment *This = impl_from_IXMLDOMComment( iface );
    TRACE("(%p)->(%p %p)\n", This, child, oldChild);
    return node_remove_child(&This->node, child, oldChild);
}

static HRESULT WINAPI domcomment_appendChild(
    IXMLDOMComment *iface,
    IXMLDOMNode *child, IXMLDOMNode **outChild)
{
    domcomment *This = impl_from_IXMLDOMComment( iface );
    TRACE("(%p)->(%p %p)\n", This, child, outChild);
    return node_append_child(&This->node, child, outChild);
}

static HRESULT WINAPI domcomment_hasChildNodes(
    IXMLDOMComment *iface,
    VARIANT_BOOL *ret)
{
    domcomment *This = impl_from_IXMLDOMComment( iface );
    TRACE("(%p)->(%p)\n", This, ret);
    return return_var_false(ret);
}

static HRESULT WINAPI domcomment_get_ownerDocument(
    IXMLDOMComment   *iface,
    IXMLDOMDocument **doc)
{
    domcomment *This = impl_from_IXMLDOMComment( iface );
    TRACE("(%p)->(%p)\n", This, doc);
    return node_get_owner_doc(&This->node, doc);
}

static HRESULT WINAPI domcomment_cloneNode(
    IXMLDOMComment *iface,
    VARIANT_BOOL deep, IXMLDOMNode** outNode)
{
    domcomment *This = impl_from_IXMLDOMComment( iface );
    TRACE("(%p)->(%d %p)\n", This, deep, outNode);
    return node_clone( &This->node, deep, outNode );
}

static HRESULT WINAPI domcomment_get_nodeTypeString(
    IXMLDOMComment *iface,
    BSTR* p)
{
    domcomment *This = impl_from_IXMLDOMComment( iface );
    static const WCHAR commentW[] = {'c','o','m','m','e','n','t',0};

    TRACE("(%p)->(%p)\n", This, p);

    return return_bstr(commentW, p);
}

static HRESULT WINAPI domcomment_get_text(
    IXMLDOMComment *iface,
    BSTR* p)
{
    domcomment *This = impl_from_IXMLDOMComment( iface );
    TRACE("(%p)->(%p)\n", This, p);
    return node_get_text(&This->node, p);
}

static HRESULT WINAPI domcomment_put_text(
    IXMLDOMComment *iface,
    BSTR p)
{
    domcomment *This = impl_from_IXMLDOMComment( iface );
    TRACE("(%p)->(%s)\n", This, debugstr_w(p));
    return node_put_text( &This->node, p );
}

static HRESULT WINAPI domcomment_get_specified(
    IXMLDOMComment *iface,
    VARIANT_BOOL* isSpecified)
{
    domcomment *This = impl_from_IXMLDOMComment( iface );
    FIXME("(%p)->(%p) stub!\n", This, isSpecified);
    *isSpecified = VARIANT_TRUE;
    return S_OK;
}

static HRESULT WINAPI domcomment_get_definition(
    IXMLDOMComment *iface,
    IXMLDOMNode** definitionNode)
{
    domcomment *This = impl_from_IXMLDOMComment( iface );
    FIXME("(%p)->(%p)\n", This, definitionNode);
    return E_NOTIMPL;
}

static HRESULT WINAPI domcomment_get_nodeTypedValue(
    IXMLDOMComment *iface,
    VARIANT* v)
{
    domcomment *This = impl_from_IXMLDOMComment( iface );
    TRACE("(%p)->(%p)\n", This, v);
    return node_get_content(&This->node, v);
}

static HRESULT WINAPI domcomment_put_nodeTypedValue(
    IXMLDOMComment *iface,
    VARIANT typedValue)
{
    domcomment *This = impl_from_IXMLDOMComment( iface );
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&typedValue));
    return E_NOTIMPL;
}

static HRESULT WINAPI domcomment_get_dataType(
    IXMLDOMComment *iface,
    VARIANT* typename)
{
    domcomment *This = impl_from_IXMLDOMComment( iface );
    TRACE("(%p)->(%p)\n", This, typename);
    return return_null_var( typename );
}

static HRESULT WINAPI domcomment_put_dataType(
    IXMLDOMComment *iface,
    BSTR p)
{
    domcomment *This = impl_from_IXMLDOMComment( iface );

    TRACE("(%p)->(%s)\n", This, debugstr_w(p));

    if(!p)
        return E_INVALIDARG;

    return E_FAIL;
}

static HRESULT WINAPI domcomment_get_xml(
    IXMLDOMComment *iface,
    BSTR* p)
{
    domcomment *This = impl_from_IXMLDOMComment( iface );

    TRACE("(%p)->(%p)\n", This, p);

    return node_get_xml(&This->node, FALSE, p);
}

static HRESULT WINAPI domcomment_transformNode(
    IXMLDOMComment *iface,
    IXMLDOMNode *node, BSTR *p)
{
    domcomment *This = impl_from_IXMLDOMComment( iface );
    TRACE("(%p)->(%p %p)\n", This, node, p);
    return node_transform_node(&This->node, node, p);
}

static HRESULT WINAPI domcomment_selectNodes(
    IXMLDOMComment *iface,
    BSTR p, IXMLDOMNodeList** outList)
{
    domcomment *This = impl_from_IXMLDOMComment( iface );
    TRACE("(%p)->(%s %p)\n", This, debugstr_w(p), outList);
    return node_select_nodes(&This->node, p, outList);
}

static HRESULT WINAPI domcomment_selectSingleNode(
    IXMLDOMComment *iface,
    BSTR p, IXMLDOMNode** outNode)
{
    domcomment *This = impl_from_IXMLDOMComment( iface );
    TRACE("(%p)->(%s %p)\n", This, debugstr_w(p), outNode);
    return node_select_singlenode(&This->node, p, outNode);
}

static HRESULT WINAPI domcomment_get_parsed(
    IXMLDOMComment *iface,
    VARIANT_BOOL* isParsed)
{
    domcomment *This = impl_from_IXMLDOMComment( iface );
    FIXME("(%p)->(%p) stub!\n", This, isParsed);
    *isParsed = VARIANT_TRUE;
    return S_OK;
}

static HRESULT WINAPI domcomment_get_namespaceURI(
    IXMLDOMComment *iface,
    BSTR* p)
{
    domcomment *This = impl_from_IXMLDOMComment( iface );
    TRACE("(%p)->(%p)\n", This, p);
    return node_get_namespaceURI(&This->node, p);
}

static HRESULT WINAPI domcomment_get_prefix(
    IXMLDOMComment *iface,
    BSTR* prefix)
{
    domcomment *This = impl_from_IXMLDOMComment( iface );
    TRACE("(%p)->(%p)\n", This, prefix);
    return return_null_bstr( prefix );
}

static HRESULT WINAPI domcomment_get_baseName(
    IXMLDOMComment *iface,
    BSTR* name)
{
    domcomment *This = impl_from_IXMLDOMComment( iface );
    TRACE("(%p)->(%p)\n", This, name);
    return return_null_bstr( name );
}

static HRESULT WINAPI domcomment_transformNodeToObject(
    IXMLDOMComment *iface,
    IXMLDOMNode* domNode, VARIANT var1)
{
    domcomment *This = impl_from_IXMLDOMComment( iface );
    FIXME("(%p)->(%p %s)\n", This, domNode, debugstr_variant(&var1));
    return E_NOTIMPL;
}

static HRESULT WINAPI domcomment_get_data(
    IXMLDOMComment *iface,
    BSTR *p)
{
    domcomment *This = impl_from_IXMLDOMComment( iface );
    HRESULT hr;
    VARIANT vRet;

    TRACE("(%p)->(%p)\n", This, p);

    if(!p)
        return E_INVALIDARG;

    hr = IXMLDOMComment_get_nodeValue( iface, &vRet );
    if(hr == S_OK)
    {
        *p = V_BSTR(&vRet);
    }

    return hr;
}

static HRESULT WINAPI domcomment_put_data(
    IXMLDOMComment *iface,
    BSTR data)
{
    domcomment *This = impl_from_IXMLDOMComment( iface );
    TRACE("(%p)->(%s)\n", This, debugstr_w(data));
    return node_set_content(&This->node, data);
}

static HRESULT WINAPI domcomment_get_length(
    IXMLDOMComment *iface,
    LONG *len)
{
    domcomment *This = impl_from_IXMLDOMComment( iface );
    HRESULT hr;
    BSTR data;

    TRACE("(%p)->(%p)\n", This, len);

    if(!len)
        return E_INVALIDARG;

    hr = IXMLDOMComment_get_data(iface, &data);
    if(hr == S_OK)
    {
        *len = SysStringLen(data);
        SysFreeString(data);
    }

    return hr;
}

static HRESULT WINAPI domcomment_substringData(IXMLDOMComment *iface, LONG offset, LONG count, BSTR *p)
{
    HRESULT hr;
    BSTR data;

    TRACE("%p, %ld, %ld, %p.\n", iface, offset, count, p);

    if(!p)
        return E_INVALIDARG;

    *p = NULL;
    if(offset < 0 || count < 0)
        return E_INVALIDARG;

    if(count == 0)
        return S_FALSE;

    hr = IXMLDOMComment_get_data(iface, &data);
    if(hr == S_OK)
    {
        LONG len = SysStringLen(data);

        if(offset < len)
        {
            if(offset + count > len)
                *p = SysAllocString(&data[offset]);
            else
                *p = SysAllocStringLen(&data[offset], count);
        }
        else
            hr = S_FALSE;

        SysFreeString(data);
    }

    return hr;
}

static HRESULT WINAPI domcomment_appendData(
    IXMLDOMComment *iface,
    BSTR p)
{
    domcomment *This = impl_from_IXMLDOMComment( iface );
    HRESULT hr;
    BSTR data;
    LONG p_len;

    TRACE("(%p)->(%s)\n", This, debugstr_w(p));

    /* Nothing to do if NULL or an Empty string passed in. */
    if((p_len = SysStringLen(p)) == 0) return S_OK;

    hr = IXMLDOMComment_get_data(iface, &data);
    if(hr == S_OK)
    {
        LONG len = SysStringLen(data);
        BSTR str = SysAllocStringLen(NULL, p_len + len);

        memcpy(str, data, len*sizeof(WCHAR));
        memcpy(&str[len], p, p_len*sizeof(WCHAR));
        str[len+p_len] = 0;

        hr = IXMLDOMComment_put_data(iface, str);

        SysFreeString(str);
        SysFreeString(data);
    }

    return hr;
}

static HRESULT WINAPI domcomment_insertData(
    IXMLDOMComment *iface,
    LONG offset, BSTR p)
{
    HRESULT hr;
    BSTR data;
    LONG p_len;

    TRACE("%p, %ld, %s.\n", iface, offset, debugstr_w(p));

    /* If have a NULL or empty string, don't do anything. */
    if((p_len = SysStringLen(p)) == 0)
        return S_OK;

    if(offset < 0)
    {
        return E_INVALIDARG;
    }

    hr = IXMLDOMComment_get_data(iface, &data);
    if(hr == S_OK)
    {
        LONG len = SysStringLen(data);
        BSTR str;

        if(len < offset)
        {
            SysFreeString(data);
            return E_INVALIDARG;
        }

        str = SysAllocStringLen(NULL, len + p_len);
        /* start part, supplied string and end part */
        memcpy(str, data, offset*sizeof(WCHAR));
        memcpy(&str[offset], p, p_len*sizeof(WCHAR));
        memcpy(&str[offset+p_len], &data[offset], (len-offset)*sizeof(WCHAR));
        str[len+p_len] = 0;

        hr = IXMLDOMComment_put_data(iface, str);

        SysFreeString(str);
        SysFreeString(data);
    }

    return hr;
}

static HRESULT WINAPI domcomment_deleteData(
    IXMLDOMComment *iface,
    LONG offset, LONG count)
{
    HRESULT hr;
    LONG len = -1;
    BSTR str;

    TRACE("%p, %ld, %ld.\n", iface, offset, count);

    hr = IXMLDOMComment_get_length(iface, &len);
    if(hr != S_OK) return hr;

    if((offset < 0) || (offset > len) || (count < 0))
        return E_INVALIDARG;

    if(len == 0) return S_OK;

    /* cutting start or end */
    if((offset == 0) || ((count + offset) >= len))
    {
        if(offset == 0)
            IXMLDOMComment_substringData(iface, count, len - count, &str);
        else
            IXMLDOMComment_substringData(iface, 0, offset, &str);
        hr = IXMLDOMComment_put_data(iface, str);
    }
    else
    /* cutting from the inside */
    {
        BSTR str_end;

        IXMLDOMComment_substringData(iface, 0, offset, &str);
        IXMLDOMComment_substringData(iface, offset + count, len - count, &str_end);

        hr = IXMLDOMComment_put_data(iface, str);
        if(hr == S_OK)
            hr = IXMLDOMComment_appendData(iface, str_end);

        SysFreeString(str_end);
    }

    SysFreeString(str);

    return hr;
}

static HRESULT WINAPI domcomment_replaceData(
    IXMLDOMComment *iface,
    LONG offset, LONG count, BSTR p)
{
    HRESULT hr;

    TRACE("%p, %ld, %ld, %s.\n", iface, offset, count, debugstr_w(p));

    hr = IXMLDOMComment_deleteData(iface, offset, count);

    if (hr == S_OK)
       hr = IXMLDOMComment_insertData(iface, offset, p);

    return hr;
}

static const struct IXMLDOMCommentVtbl domcomment_vtbl =
{
    domcomment_QueryInterface,
    domcomment_AddRef,
    domcomment_Release,
    domcomment_GetTypeInfoCount,
    domcomment_GetTypeInfo,
    domcomment_GetIDsOfNames,
    domcomment_Invoke,
    domcomment_get_nodeName,
    domcomment_get_nodeValue,
    domcomment_put_nodeValue,
    domcomment_get_nodeType,
    domcomment_get_parentNode,
    domcomment_get_childNodes,
    domcomment_get_firstChild,
    domcomment_get_lastChild,
    domcomment_get_previousSibling,
    domcomment_get_nextSibling,
    domcomment_get_attributes,
    domcomment_insertBefore,
    domcomment_replaceChild,
    domcomment_removeChild,
    domcomment_appendChild,
    domcomment_hasChildNodes,
    domcomment_get_ownerDocument,
    domcomment_cloneNode,
    domcomment_get_nodeTypeString,
    domcomment_get_text,
    domcomment_put_text,
    domcomment_get_specified,
    domcomment_get_definition,
    domcomment_get_nodeTypedValue,
    domcomment_put_nodeTypedValue,
    domcomment_get_dataType,
    domcomment_put_dataType,
    domcomment_get_xml,
    domcomment_transformNode,
    domcomment_selectNodes,
    domcomment_selectSingleNode,
    domcomment_get_parsed,
    domcomment_get_namespaceURI,
    domcomment_get_prefix,
    domcomment_get_baseName,
    domcomment_transformNodeToObject,
    domcomment_get_data,
    domcomment_put_data,
    domcomment_get_length,
    domcomment_substringData,
    domcomment_appendData,
    domcomment_insertData,
    domcomment_deleteData,
    domcomment_replaceData
};

static const tid_t domcomment_iface_tids[] = {
    IXMLDOMComment_tid,
    0
};

static dispex_static_data_t domcomment_dispex = {
    NULL,
    IXMLDOMComment_tid,
    NULL,
    domcomment_iface_tids
};

IUnknown* create_comment( xmlNodePtr comment )
{
    domcomment *This;

    This = malloc(sizeof(*This));
    if ( !This )
        return NULL;

    This->IXMLDOMComment_iface.lpVtbl = &domcomment_vtbl;
    This->ref = 1;

    init_xmlnode(&This->node, comment, (IXMLDOMNode*)&This->IXMLDOMComment_iface, &domcomment_dispex);

    return (IUnknown*)&This->IXMLDOMComment_iface;
}
