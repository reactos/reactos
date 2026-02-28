/*
 *    DOM text node implementation
 *
 * Copyright 2006 Huw Davies
 * Copyright 2007-2008 Alistair Leslie-Hughes
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
#include <libxml/parserInternals.h>
#include <libxml/xmlerror.h>

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "ole2.h"
#include "msxml6.h"

#include "msxml_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(msxml);

typedef struct _domtext
{
    xmlnode node;
    IXMLDOMText IXMLDOMText_iface;
    LONG ref;
} domtext;

static inline domtext *impl_from_IXMLDOMText( IXMLDOMText *iface )
{
    return CONTAINING_RECORD(iface, domtext, IXMLDOMText_iface);
}

static void domtext_reset_noenc(domtext *This)
{
    This->node.node->name = NULL;
}

static HRESULT WINAPI domtext_QueryInterface(
    IXMLDOMText *iface,
    REFIID riid,
    void** ppvObject )
{
    domtext *This = impl_from_IXMLDOMText( iface );
    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppvObject);

    if ( IsEqualGUID( riid, &IID_IXMLDOMText ) ||
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
    else
    {
        TRACE("Unsupported interface %s\n", debugstr_guid(riid));
        *ppvObject = NULL;
        return E_NOINTERFACE;
    }

    IXMLDOMText_AddRef(iface);
    return S_OK;
}

static ULONG WINAPI domtext_AddRef(
    IXMLDOMText *iface )
{
    domtext *This = impl_from_IXMLDOMText( iface );
    ULONG ref = InterlockedIncrement( &This->ref );
    TRACE("%p, refcount %lu.\n", iface, ref);
    return ref;
}

static ULONG WINAPI domtext_Release(
    IXMLDOMText *iface )
{
    domtext *This = impl_from_IXMLDOMText( iface );
    ULONG ref = InterlockedDecrement( &This->ref );

    TRACE("%p, refcount %lu.\n", iface, ref);
    if ( ref == 0 )
    {
        destroy_xmlnode(&This->node);
        free(This);
    }

    return ref;
}

static HRESULT WINAPI domtext_GetTypeInfoCount(
    IXMLDOMText *iface,
    UINT* pctinfo )
{
    domtext *This = impl_from_IXMLDOMText( iface );
    return IDispatchEx_GetTypeInfoCount(&This->node.dispex.IDispatchEx_iface, pctinfo);
}

static HRESULT WINAPI domtext_GetTypeInfo(
    IXMLDOMText *iface,
    UINT iTInfo, LCID lcid,
    ITypeInfo** ppTInfo )
{
    domtext *This = impl_from_IXMLDOMText( iface );
    return IDispatchEx_GetTypeInfo(&This->node.dispex.IDispatchEx_iface,
        iTInfo, lcid, ppTInfo);
}

static HRESULT WINAPI domtext_GetIDsOfNames(
    IXMLDOMText *iface,
    REFIID riid, LPOLESTR* rgszNames,
    UINT cNames, LCID lcid, DISPID* rgDispId )
{
    domtext *This = impl_from_IXMLDOMText( iface );
    return IDispatchEx_GetIDsOfNames(&This->node.dispex.IDispatchEx_iface,
        riid, rgszNames, cNames, lcid, rgDispId);
}

static HRESULT WINAPI domtext_Invoke(
    IXMLDOMText *iface,
    DISPID dispIdMember, REFIID riid, LCID lcid,
    WORD wFlags, DISPPARAMS* pDispParams, VARIANT* pVarResult,
    EXCEPINFO* pExcepInfo, UINT* puArgErr )
{
    domtext *This = impl_from_IXMLDOMText( iface );
    return IDispatchEx_Invoke(&This->node.dispex.IDispatchEx_iface,
        dispIdMember, riid, lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI domtext_get_nodeName(
    IXMLDOMText *iface,
    BSTR* p )
{
    domtext *This = impl_from_IXMLDOMText( iface );

    static const WCHAR textW[] = {'#','t','e','x','t',0};

    TRACE("(%p)->(%p)\n", This, p);

    return return_bstr(textW, p);
}

static HRESULT WINAPI domtext_get_nodeValue(
    IXMLDOMText *iface,
    VARIANT* value )
{
    domtext *This = impl_from_IXMLDOMText( iface );

    TRACE("(%p)->(%p)\n", This, value);

    return node_get_content(&This->node, value);
}

static HRESULT WINAPI domtext_put_nodeValue(
    IXMLDOMText *iface,
    VARIANT value)
{
    domtext *This = impl_from_IXMLDOMText( iface );

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&value));

    domtext_reset_noenc(This);
    return node_put_value(&This->node, &value);
}

static HRESULT WINAPI domtext_get_nodeType(
    IXMLDOMText *iface,
    DOMNodeType* domNodeType )
{
    domtext *This = impl_from_IXMLDOMText( iface );

    TRACE("(%p)->(%p)\n", This, domNodeType);

    *domNodeType = NODE_TEXT;
    return S_OK;
}

static HRESULT WINAPI domtext_get_parentNode(
    IXMLDOMText *iface,
    IXMLDOMNode** parent )
{
    domtext *This = impl_from_IXMLDOMText( iface );

    TRACE("(%p)->(%p)\n", This, parent);

    return node_get_parent(&This->node, parent);
}

static HRESULT WINAPI domtext_get_childNodes(
    IXMLDOMText *iface,
    IXMLDOMNodeList** outList)
{
    domtext *This = impl_from_IXMLDOMText( iface );

    TRACE("(%p)->(%p)\n", This, outList);

    return node_get_child_nodes(&This->node, outList);
}

static HRESULT WINAPI domtext_get_firstChild(
    IXMLDOMText *iface,
    IXMLDOMNode** domNode)
{
    domtext *This = impl_from_IXMLDOMText( iface );

    TRACE("(%p)->(%p)\n", This, domNode);

    return return_null_node(domNode);
}

static HRESULT WINAPI domtext_get_lastChild(
    IXMLDOMText *iface,
    IXMLDOMNode** domNode)
{
    domtext *This = impl_from_IXMLDOMText( iface );

    TRACE("(%p)->(%p)\n", This, domNode);

    return return_null_node(domNode);
}

static HRESULT WINAPI domtext_get_previousSibling(
    IXMLDOMText *iface,
    IXMLDOMNode** domNode)
{
    domtext *This = impl_from_IXMLDOMText( iface );

    TRACE("(%p)->(%p)\n", This, domNode);

    return node_get_previous_sibling(&This->node, domNode);
}

static HRESULT WINAPI domtext_get_nextSibling(
    IXMLDOMText *iface,
    IXMLDOMNode** domNode)
{
    domtext *This = impl_from_IXMLDOMText( iface );

    TRACE("(%p)->(%p)\n", This, domNode);

    return node_get_next_sibling(&This->node, domNode);
}

static HRESULT WINAPI domtext_get_attributes(
    IXMLDOMText *iface,
    IXMLDOMNamedNodeMap** attributeMap)
{
    domtext *This = impl_from_IXMLDOMText( iface );

    TRACE("(%p)->(%p)\n", This, attributeMap);

    return return_null_ptr((void**)attributeMap);
}

static HRESULT WINAPI domtext_insertBefore(
    IXMLDOMText *iface,
    IXMLDOMNode* newNode, VARIANT refChild,
    IXMLDOMNode** outOldNode)
{
    domtext *This = impl_from_IXMLDOMText( iface );

    FIXME("(%p)->(%p %s %p) needs test\n", This, newNode, debugstr_variant(&refChild), outOldNode);

    return node_insert_before(&This->node, newNode, &refChild, outOldNode);
}

static HRESULT WINAPI domtext_replaceChild(
    IXMLDOMText *iface,
    IXMLDOMNode* newNode,
    IXMLDOMNode* oldNode,
    IXMLDOMNode** outOldNode)
{
    domtext *This = impl_from_IXMLDOMText( iface );

    FIXME("(%p)->(%p %p %p) needs test\n", This, newNode, oldNode, outOldNode);

    return node_replace_child(&This->node, newNode, oldNode, outOldNode);
}

static HRESULT WINAPI domtext_removeChild(
    IXMLDOMText *iface,
    IXMLDOMNode *child, IXMLDOMNode **oldChild)
{
    domtext *This = impl_from_IXMLDOMText( iface );
    TRACE("(%p)->(%p %p)\n", This, child, oldChild);
    return node_remove_child(&This->node, child, oldChild);
}

static HRESULT WINAPI domtext_appendChild(
    IXMLDOMText *iface,
    IXMLDOMNode *child, IXMLDOMNode **outChild)
{
    domtext *This = impl_from_IXMLDOMText( iface );
    TRACE("(%p)->(%p %p)\n", This, child, outChild);
    return node_append_child(&This->node, child, outChild);
}

static HRESULT WINAPI domtext_hasChildNodes(
    IXMLDOMText *iface,
    VARIANT_BOOL *ret)
{
    domtext *This = impl_from_IXMLDOMText( iface );
    TRACE("(%p)->(%p)\n", This, ret);
    return return_var_false(ret);
}

static HRESULT WINAPI domtext_get_ownerDocument(
    IXMLDOMText *iface,
    IXMLDOMDocument **doc)
{
    domtext *This = impl_from_IXMLDOMText( iface );
    TRACE("(%p)->(%p)\n", This, doc);
    return node_get_owner_doc(&This->node, doc);
}

static HRESULT WINAPI domtext_cloneNode(
    IXMLDOMText *iface,
    VARIANT_BOOL deep, IXMLDOMNode** outNode)
{
    domtext *This = impl_from_IXMLDOMText( iface );
    TRACE("(%p)->(%d %p)\n", This, deep, outNode);
    return node_clone( &This->node, deep, outNode );
}

static HRESULT WINAPI domtext_get_nodeTypeString(
    IXMLDOMText *iface,
    BSTR* p)
{
    domtext *This = impl_from_IXMLDOMText( iface );
    static const WCHAR textW[] = {'t','e','x','t',0};

    TRACE("(%p)->(%p)\n", This, p);

    return return_bstr(textW, p);
}

static HRESULT WINAPI domtext_get_text(
    IXMLDOMText *iface,
    BSTR* p)
{
    domtext *This = impl_from_IXMLDOMText( iface );
    TRACE("(%p)->(%p)\n", This, p);
    return node_get_text(&This->node, p);
}

static HRESULT WINAPI domtext_put_text(
    IXMLDOMText *iface,
    BSTR p)
{
    domtext *This = impl_from_IXMLDOMText( iface );
    TRACE("(%p)->(%s)\n", This, debugstr_w(p));
    domtext_reset_noenc(This);
    return node_put_text( &This->node, p );
}

static HRESULT WINAPI domtext_get_specified(
    IXMLDOMText *iface,
    VARIANT_BOOL* isSpecified)
{
    domtext *This = impl_from_IXMLDOMText( iface );
    FIXME("(%p)->(%p) stub!\n", This, isSpecified);
    *isSpecified = VARIANT_TRUE;
    return S_OK;
}

static HRESULT WINAPI domtext_get_definition(
    IXMLDOMText *iface,
    IXMLDOMNode** definitionNode)
{
    domtext *This = impl_from_IXMLDOMText( iface );
    FIXME("(%p)->(%p)\n", This, definitionNode);
    return E_NOTIMPL;
}

static HRESULT WINAPI domtext_get_nodeTypedValue(
    IXMLDOMText *iface,
    VARIANT* var1)
{
    domtext *This = impl_from_IXMLDOMText( iface );
    IXMLDOMNode* parent = NULL;
    HRESULT hr;

    TRACE("(%p)->(%p)\n", This, var1);

    if (!var1)
        return E_INVALIDARG;

    hr = IXMLDOMText_get_parentNode(iface, &parent);

    if (hr == S_OK)
    {
        hr = IXMLDOMNode_get_nodeTypedValue(parent, var1);
        IXMLDOMNode_Release(parent);
    }
    else
    {
        V_VT(var1) = VT_NULL;
        V_BSTR(var1) = NULL;
        hr = S_FALSE;
    }

    return hr;
}

static HRESULT WINAPI domtext_put_nodeTypedValue(
    IXMLDOMText *iface,
    VARIANT value)
{
    domtext *This = impl_from_IXMLDOMText( iface );
    IXMLDOMNode* parent = NULL;
    HRESULT hr;

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&value));

    hr = IXMLDOMText_get_parentNode(iface, &parent);

    if (hr == S_OK)
    {
        hr = IXMLDOMNode_put_nodeTypedValue(parent, value);
        IXMLDOMNode_Release(parent);
    }
    else
    {
        hr = S_FALSE;
    }

    return hr;
}

static HRESULT WINAPI domtext_get_dataType(
    IXMLDOMText *iface,
    VARIANT* dtName)
{
    domtext *This = impl_from_IXMLDOMText( iface );
    IXMLDOMNode* parent = NULL;
    HRESULT hr;

    TRACE("(%p)->(%p)\n", This, dtName);

    if (!dtName)
        return E_INVALIDARG;

    hr = IXMLDOMText_get_parentNode(iface, &parent);

    if (hr == S_OK)
    {
        hr = IXMLDOMNode_get_dataType(parent, dtName);
        IXMLDOMNode_Release(parent);
    }
    else
    {
        V_VT(dtName) = VT_NULL;
        V_BSTR(dtName) = NULL;
        hr = S_FALSE;
    }

    return hr;
}

static HRESULT WINAPI domtext_put_dataType(
    IXMLDOMText *iface,
    BSTR dtName)
{
    domtext *This = impl_from_IXMLDOMText( iface );
    IXMLDOMNode* parent = NULL;
    HRESULT hr;

    TRACE("(%p)->(%p)\n", This, dtName);

    if (!dtName)
        return E_INVALIDARG;

    hr = IXMLDOMText_get_parentNode(iface, &parent);

    if (hr == S_OK)
    {
        hr = IXMLDOMNode_put_dataType(parent, dtName);
        IXMLDOMNode_Release(parent);
    }
    else
    {
        hr = S_FALSE;
    }

    return hr;
}

static HRESULT WINAPI domtext_get_xml(
    IXMLDOMText *iface,
    BSTR* p)
{
    domtext *This = impl_from_IXMLDOMText( iface );

    TRACE("(%p)->(%p)\n", This, p);

    return node_get_xml(&This->node, TRUE, p);
}

static HRESULT WINAPI domtext_transformNode(
    IXMLDOMText *iface,
    IXMLDOMNode *node, BSTR *p)
{
    domtext *This = impl_from_IXMLDOMText( iface );
    TRACE("(%p)->(%p %p)\n", This, node, p);
    return node_transform_node(&This->node, node, p);
}

static HRESULT WINAPI domtext_selectNodes(
    IXMLDOMText *iface,
    BSTR p, IXMLDOMNodeList** outList)
{
    domtext *This = impl_from_IXMLDOMText( iface );
    TRACE("(%p)->(%s %p)\n", This, debugstr_w(p), outList);
    return node_select_nodes(&This->node, p, outList);
}

static HRESULT WINAPI domtext_selectSingleNode(
    IXMLDOMText *iface,
    BSTR p, IXMLDOMNode** outNode)
{
    domtext *This = impl_from_IXMLDOMText( iface );
    TRACE("(%p)->(%s %p)\n", This, debugstr_w(p), outNode);
    return node_select_singlenode(&This->node, p, outNode);
}

static HRESULT WINAPI domtext_get_parsed(
    IXMLDOMText *iface,
    VARIANT_BOOL* isParsed)
{
    domtext *This = impl_from_IXMLDOMText( iface );
    FIXME("(%p)->(%p) stub!\n", This, isParsed);
    *isParsed = VARIANT_TRUE;
    return S_OK;
}

static HRESULT WINAPI domtext_get_namespaceURI(
    IXMLDOMText *iface,
    BSTR* p)
{
    domtext *This = impl_from_IXMLDOMText( iface );
    TRACE("(%p)->(%p)\n", This, p);
    return node_get_namespaceURI(&This->node, p);
}

static HRESULT WINAPI domtext_get_prefix(
    IXMLDOMText *iface,
    BSTR* prefix)
{
    domtext *This = impl_from_IXMLDOMText( iface );
    TRACE("(%p)->(%p)\n", This, prefix);
    return return_null_bstr( prefix );
}

static HRESULT WINAPI domtext_get_baseName(
    IXMLDOMText *iface,
    BSTR* name)
{
    domtext *This = impl_from_IXMLDOMText( iface );
    TRACE("(%p)->(%p)\n", This, name);
    return return_null_bstr( name );
}

static HRESULT WINAPI domtext_transformNodeToObject(
    IXMLDOMText *iface,
    IXMLDOMNode* domNode, VARIANT var1)
{
    domtext *This = impl_from_IXMLDOMText( iface );
    FIXME("(%p)->(%p %s)\n", This, domNode, debugstr_variant(&var1));
    return E_NOTIMPL;
}

static HRESULT WINAPI domtext_get_data(
    IXMLDOMText *iface,
    BSTR *p)
{
    domtext *This = impl_from_IXMLDOMText( iface );

    if(!p)
        return E_INVALIDARG;

    *p = bstr_from_xmlChar(This->node.node->content);
    return S_OK;
}

static HRESULT WINAPI domtext_put_data(
    IXMLDOMText *iface,
    BSTR data)
{
    domtext *This = impl_from_IXMLDOMText( iface );
    BSTR normalized_data = NULL;
    HRESULT hr;
    size_t i, j;

    TRACE("(%p)->(%s)\n", This, debugstr_w(data));

    if (data)
    {
        /* normalize line endings */
        normalized_data = SysAllocStringLen(NULL, SysStringLen(data));
        if (!normalized_data) return E_OUTOFMEMORY;
        for (i = 0, j = 0; data[i]; i++)
        {
            if (data[i] == '\r')
            {
                if (data[i + 1] == '\n') i++; /* change \r\n to just \n */
                normalized_data[j++] = '\n'; /* change \r by itself to \n */
            }
            else
                normalized_data[j++] = data[i];
        }
        normalized_data[j] = 0;
    }

    domtext_reset_noenc(This);
    hr = node_set_content(&This->node, normalized_data);

    SysFreeString(normalized_data);
    return hr;
}

static HRESULT WINAPI domtext_get_length(
    IXMLDOMText *iface,
    LONG *len)
{
    domtext *This = impl_from_IXMLDOMText( iface );
    HRESULT hr;
    BSTR data;

    TRACE("(%p)->(%p)\n", This, len);

    if(!len)
        return E_INVALIDARG;

    hr = IXMLDOMText_get_data(iface, &data);
    if(hr == S_OK)
    {
        *len = SysStringLen(data);
        SysFreeString(data);
    }

    return hr;
}

static HRESULT WINAPI domtext_substringData(
    IXMLDOMText *iface,
    LONG offset, LONG count, BSTR *p)
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

    hr = IXMLDOMText_get_data(iface, &data);
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

static HRESULT WINAPI domtext_appendData(
    IXMLDOMText *iface,
    BSTR p)
{
    domtext *This = impl_from_IXMLDOMText( iface );
    HRESULT hr;
    BSTR data;
    LONG p_len;

    TRACE("(%p)->(%s)\n", This, debugstr_w(p));

    /* Nothing to do if NULL or an Empty string passed in. */
    if((p_len = SysStringLen(p)) == 0) return S_OK;

    hr = IXMLDOMText_get_data(iface, &data);
    if(hr == S_OK)
    {
        LONG len = SysStringLen(data);
        BSTR str = SysAllocStringLen(NULL, p_len + len);

        memcpy(str, data, len*sizeof(WCHAR));
        memcpy(&str[len], p, p_len*sizeof(WCHAR));
        str[len+p_len] = 0;

        hr = IXMLDOMText_put_data(iface, str);

        SysFreeString(str);
        SysFreeString(data);
    }

    return hr;
}

static HRESULT WINAPI domtext_insertData(
    IXMLDOMText *iface,
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

    hr = IXMLDOMText_get_data(iface, &data);
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

        hr = IXMLDOMText_put_data(iface, str);

        SysFreeString(str);
        SysFreeString(data);
    }

    return hr;
}

static HRESULT WINAPI domtext_deleteData(
    IXMLDOMText *iface,
    LONG offset, LONG count)
{
    HRESULT hr;
    LONG len = -1;
    BSTR str;

    TRACE("%p, %ld, %ld.\n", iface, offset, count);

    hr = IXMLDOMText_get_length(iface, &len);
    if(hr != S_OK) return hr;

    if((offset < 0) || (offset > len) || (count < 0))
        return E_INVALIDARG;

    if(len == 0) return S_OK;

    /* cutting start or end */
    if((offset == 0) || ((count + offset) >= len))
    {
        if(offset == 0)
            IXMLDOMText_substringData(iface, count, len - count, &str);
        else
            IXMLDOMText_substringData(iface, 0, offset, &str);
        hr = IXMLDOMText_put_data(iface, str);
    }
    else
    /* cutting from the inside */
    {
        BSTR str_end;

        IXMLDOMText_substringData(iface, 0, offset, &str);
        IXMLDOMText_substringData(iface, offset + count, len - count, &str_end);

        hr = IXMLDOMText_put_data(iface, str);
        if(hr == S_OK)
            hr = IXMLDOMText_appendData(iface, str_end);

        SysFreeString(str_end);
    }

    SysFreeString(str);

    return hr;
}

static HRESULT WINAPI domtext_replaceData(
    IXMLDOMText *iface,
    LONG offset, LONG count, BSTR p)
{
    HRESULT hr;

    TRACE("%p, %ld, %ld, %s.\n", iface, offset, count, debugstr_w(p));

    hr = IXMLDOMText_deleteData(iface, offset, count);

    if (hr == S_OK)
       hr = IXMLDOMText_insertData(iface, offset, p);

    return hr;
}

static HRESULT WINAPI domtext_splitText(
    IXMLDOMText *iface,
    LONG offset, IXMLDOMText **txtNode)
{
    LONG length = 0;

    TRACE("%p, %ld, %p.\n", iface, offset, txtNode);

    if (!txtNode || offset < 0) return E_INVALIDARG;

    *txtNode = NULL;

    IXMLDOMText_get_length(iface, &length);

    if (offset > length) return E_INVALIDARG;
    if (offset == length) return S_FALSE;

    FIXME("adjacent text nodes are not supported\n");

    return E_NOTIMPL;
}

static const struct IXMLDOMTextVtbl domtext_vtbl =
{
    domtext_QueryInterface,
    domtext_AddRef,
    domtext_Release,
    domtext_GetTypeInfoCount,
    domtext_GetTypeInfo,
    domtext_GetIDsOfNames,
    domtext_Invoke,
    domtext_get_nodeName,
    domtext_get_nodeValue,
    domtext_put_nodeValue,
    domtext_get_nodeType,
    domtext_get_parentNode,
    domtext_get_childNodes,
    domtext_get_firstChild,
    domtext_get_lastChild,
    domtext_get_previousSibling,
    domtext_get_nextSibling,
    domtext_get_attributes,
    domtext_insertBefore,
    domtext_replaceChild,
    domtext_removeChild,
    domtext_appendChild,
    domtext_hasChildNodes,
    domtext_get_ownerDocument,
    domtext_cloneNode,
    domtext_get_nodeTypeString,
    domtext_get_text,
    domtext_put_text,
    domtext_get_specified,
    domtext_get_definition,
    domtext_get_nodeTypedValue,
    domtext_put_nodeTypedValue,
    domtext_get_dataType,
    domtext_put_dataType,
    domtext_get_xml,
    domtext_transformNode,
    domtext_selectNodes,
    domtext_selectSingleNode,
    domtext_get_parsed,
    domtext_get_namespaceURI,
    domtext_get_prefix,
    domtext_get_baseName,
    domtext_transformNodeToObject,
    domtext_get_data,
    domtext_put_data,
    domtext_get_length,
    domtext_substringData,
    domtext_appendData,
    domtext_insertData,
    domtext_deleteData,
    domtext_replaceData,
    domtext_splitText
};

static const tid_t domtext_iface_tids[] = {
    IXMLDOMText_tid,
    0
};

static dispex_static_data_t domtext_dispex = {
    NULL,
    IXMLDOMText_tid,
    NULL,
    domtext_iface_tids
};

IUnknown* create_text( xmlNodePtr text )
{
    domtext *This;

    This = malloc(sizeof(*This));
    if ( !This )
        return NULL;

    This->IXMLDOMText_iface.lpVtbl = &domtext_vtbl;
    This->ref = 1;

    init_xmlnode(&This->node, text, (IXMLDOMNode*)&This->IXMLDOMText_iface, &domtext_dispex);

    return (IUnknown*)&This->IXMLDOMText_iface;
}
