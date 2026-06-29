/*
 *    DOM CDATA node implementation
 *
 * Copyright 2007 Alistair Leslie-Hughes
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

typedef struct
{
    xmlnode node;
    IXMLDOMCDATASection IXMLDOMCDATASection_iface;
    LONG ref;
} domcdata;

static const tid_t domcdata_se_tids[] = {
    IXMLDOMNode_tid,
    IXMLDOMCDATASection_tid,
    NULL_tid
};

static inline domcdata *impl_from_IXMLDOMCDATASection( IXMLDOMCDATASection *iface )
{
    return CONTAINING_RECORD(iface, domcdata, IXMLDOMCDATASection_iface);
}

static HRESULT WINAPI domcdata_QueryInterface(
    IXMLDOMCDATASection *iface,
    REFIID riid,
    void** ppvObject )
{
    domcdata *This = impl_from_IXMLDOMCDATASection( iface );
    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppvObject);

    if ( IsEqualGUID( riid, &IID_IXMLDOMCDATASection ) ||
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
        return node_create_supporterrorinfo(domcdata_se_tids, ppvObject);
    }
    else
    {
        TRACE("Unsupported interface %s\n", debugstr_guid(riid));
        *ppvObject = NULL;
        return E_NOINTERFACE;
    }

    IXMLDOMCDATASection_AddRef(iface);
    return S_OK;
}

static ULONG WINAPI domcdata_AddRef(IXMLDOMCDATASection *iface)
{
    domcdata *cdata = impl_from_IXMLDOMCDATASection(iface);
    ULONG ref = InterlockedIncrement(&cdata->ref);
    TRACE("%p, refcount %lu.\n", iface, ref);
    return ref;
}

static ULONG WINAPI domcdata_Release(IXMLDOMCDATASection *iface)
{
    domcdata *cdata = impl_from_IXMLDOMCDATASection(iface);
    ULONG ref = InterlockedDecrement(&cdata->ref);

    TRACE("%p, refcount %lu.\n", iface, ref);

    if (!ref)
    {
        destroy_xmlnode(&cdata->node);
        free(cdata);
    }

    return ref;
}

static HRESULT WINAPI domcdata_GetTypeInfoCount(
    IXMLDOMCDATASection *iface,
    UINT* pctinfo )
{
    domcdata *This = impl_from_IXMLDOMCDATASection( iface );
    return IDispatchEx_GetTypeInfoCount(&This->node.dispex.IDispatchEx_iface, pctinfo);
}

static HRESULT WINAPI domcdata_GetTypeInfo(
    IXMLDOMCDATASection *iface,
    UINT iTInfo, LCID lcid,
    ITypeInfo** ppTInfo )
{
    domcdata *This = impl_from_IXMLDOMCDATASection( iface );
    return IDispatchEx_GetTypeInfo(&This->node.dispex.IDispatchEx_iface,
        iTInfo, lcid, ppTInfo);
}

static HRESULT WINAPI domcdata_GetIDsOfNames(
    IXMLDOMCDATASection *iface,
    REFIID riid, LPOLESTR* rgszNames,
    UINT cNames, LCID lcid, DISPID* rgDispId )
{
    domcdata *This = impl_from_IXMLDOMCDATASection( iface );
    return IDispatchEx_GetIDsOfNames(&This->node.dispex.IDispatchEx_iface,
        riid, rgszNames, cNames, lcid, rgDispId);
}

static HRESULT WINAPI domcdata_Invoke(
    IXMLDOMCDATASection *iface,
    DISPID dispIdMember, REFIID riid, LCID lcid,
    WORD wFlags, DISPPARAMS* pDispParams, VARIANT* pVarResult,
    EXCEPINFO* pExcepInfo, UINT* puArgErr )
{
    domcdata *This = impl_from_IXMLDOMCDATASection( iface );
    return IDispatchEx_Invoke(&This->node.dispex.IDispatchEx_iface,
        dispIdMember, riid, lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI domcdata_get_nodeName(
    IXMLDOMCDATASection *iface,
    BSTR* p )
{
    domcdata *This = impl_from_IXMLDOMCDATASection( iface );

    static const WCHAR cdata_sectionW[] =
        {'#','c','d','a','t','a','-','s','e','c','t','i','o','n',0};

    TRACE("(%p)->(%p)\n", This, p);

    return return_bstr(cdata_sectionW, p);
}

static HRESULT WINAPI domcdata_get_nodeValue(
    IXMLDOMCDATASection *iface,
    VARIANT* value)
{
    domcdata *This = impl_from_IXMLDOMCDATASection( iface );

    TRACE("(%p)->(%p)\n", This, value);

    return node_get_content(&This->node, value);
}

static HRESULT WINAPI domcdata_put_nodeValue(
    IXMLDOMCDATASection *iface,
    VARIANT value)
{
    domcdata *This = impl_from_IXMLDOMCDATASection( iface );

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&value));

    return node_put_value(&This->node, &value);
}

static HRESULT WINAPI domcdata_get_nodeType(
    IXMLDOMCDATASection *iface,
    DOMNodeType* domNodeType )
{
    domcdata *This = impl_from_IXMLDOMCDATASection( iface );

    TRACE("(%p)->(%p)\n", This, domNodeType);

    *domNodeType = NODE_CDATA_SECTION;
    return S_OK;
}

static HRESULT WINAPI domcdata_get_parentNode(
    IXMLDOMCDATASection *iface,
    IXMLDOMNode** parent )
{
    domcdata *This = impl_from_IXMLDOMCDATASection( iface );

    TRACE("(%p)->(%p)\n", This, parent);

    return node_get_parent(&This->node, parent);
}

static HRESULT WINAPI domcdata_get_childNodes(
    IXMLDOMCDATASection *iface,
    IXMLDOMNodeList** outList)
{
    domcdata *This = impl_from_IXMLDOMCDATASection( iface );

    TRACE("(%p)->(%p)\n", This, outList);

    return node_get_child_nodes(&This->node, outList);
}

static HRESULT WINAPI domcdata_get_firstChild(
    IXMLDOMCDATASection *iface,
    IXMLDOMNode** domNode)
{
    domcdata *This = impl_from_IXMLDOMCDATASection( iface );

    TRACE("(%p)->(%p)\n", This, domNode);

    return return_null_node(domNode);
}

static HRESULT WINAPI domcdata_get_lastChild(
    IXMLDOMCDATASection *iface,
    IXMLDOMNode** domNode)
{
    domcdata *This = impl_from_IXMLDOMCDATASection( iface );

    TRACE("(%p)->(%p)\n", This, domNode);

    return return_null_node(domNode);
}

static HRESULT WINAPI domcdata_get_previousSibling(
    IXMLDOMCDATASection *iface,
    IXMLDOMNode** domNode)
{
    domcdata *This = impl_from_IXMLDOMCDATASection( iface );

    TRACE("(%p)->(%p)\n", This, domNode);

    return node_get_previous_sibling(&This->node, domNode);
}

static HRESULT WINAPI domcdata_get_nextSibling(
    IXMLDOMCDATASection *iface,
    IXMLDOMNode** domNode)
{
    domcdata *This = impl_from_IXMLDOMCDATASection( iface );

    TRACE("(%p)->(%p)\n", This, domNode);

    return node_get_next_sibling(&This->node, domNode);
}

static HRESULT WINAPI domcdata_get_attributes(
    IXMLDOMCDATASection *iface,
    IXMLDOMNamedNodeMap** attributeMap)
{
    domcdata *This = impl_from_IXMLDOMCDATASection( iface );

    TRACE("(%p)->(%p)\n", This, attributeMap);

    return return_null_ptr((void**)attributeMap);
}

static HRESULT WINAPI domcdata_insertBefore(
    IXMLDOMCDATASection *iface,
    IXMLDOMNode* newNode, VARIANT refChild,
    IXMLDOMNode** outOldNode)
{
    domcdata *This = impl_from_IXMLDOMCDATASection( iface );
    TRACE("(%p)->(%p %s %p)\n", This, newNode, debugstr_variant(&refChild), outOldNode);
    if (outOldNode) *outOldNode = NULL;
    return E_FAIL;
}

static HRESULT WINAPI domcdata_replaceChild(
    IXMLDOMCDATASection *iface,
    IXMLDOMNode* newNode,
    IXMLDOMNode* oldNode,
    IXMLDOMNode** outOldNode)
{
    domcdata *This = impl_from_IXMLDOMCDATASection( iface );
    TRACE("(%p)->(%p %p %p)\n", This, newNode, oldNode, outOldNode);
    if (outOldNode) *outOldNode = NULL;
    return E_FAIL;
}

static HRESULT WINAPI domcdata_removeChild(
    IXMLDOMCDATASection *iface,
    IXMLDOMNode *child, IXMLDOMNode **oldChild)
{
    domcdata *This = impl_from_IXMLDOMCDATASection( iface );
    TRACE("(%p)->(%p %p)\n", This, child, oldChild);
    if (oldChild) *oldChild = NULL;
    return E_FAIL;
}

static HRESULT WINAPI domcdata_appendChild(
    IXMLDOMCDATASection *iface,
    IXMLDOMNode *child, IXMLDOMNode **outChild)
{
    domcdata *This = impl_from_IXMLDOMCDATASection( iface );
    TRACE("(%p)->(%p %p)\n", This, child, outChild);
    if (outChild) *outChild = NULL;
    return E_FAIL;
}

static HRESULT WINAPI domcdata_hasChildNodes(
    IXMLDOMCDATASection *iface,
    VARIANT_BOOL *ret)
{
    domcdata *This = impl_from_IXMLDOMCDATASection( iface );
    TRACE("(%p)->(%p)\n", This, ret);
    return return_var_false(ret);
}

static HRESULT WINAPI domcdata_get_ownerDocument(
    IXMLDOMCDATASection *iface,
    IXMLDOMDocument    **doc)
{
    domcdata *This = impl_from_IXMLDOMCDATASection( iface );
    TRACE("(%p)->(%p)\n", This, doc);
    return node_get_owner_doc(&This->node, doc);
}

static HRESULT WINAPI domcdata_cloneNode(
    IXMLDOMCDATASection *iface,
    VARIANT_BOOL deep, IXMLDOMNode** outNode)
{
    domcdata *This = impl_from_IXMLDOMCDATASection( iface );
    TRACE("(%p)->(%d %p)\n", This, deep, outNode);
    return node_clone( &This->node, deep, outNode );
}

static HRESULT WINAPI domcdata_get_nodeTypeString(
    IXMLDOMCDATASection *iface,
    BSTR* p)
{
    domcdata *This = impl_from_IXMLDOMCDATASection( iface );
    static const WCHAR cdatasectionW[] = {'c','d','a','t','a','s','e','c','t','i','o','n',0};

    TRACE("(%p)->(%p)\n", This, p);

    return return_bstr(cdatasectionW, p);
}

static HRESULT WINAPI domcdata_get_text(
    IXMLDOMCDATASection *iface,
    BSTR* p)
{
    domcdata *This = impl_from_IXMLDOMCDATASection( iface );
    TRACE("(%p)->(%p)\n", This, p);
    return node_get_text(&This->node, p);
}

static HRESULT WINAPI domcdata_put_text(
    IXMLDOMCDATASection *iface,
    BSTR p)
{
    domcdata *This = impl_from_IXMLDOMCDATASection( iface );
    TRACE("(%p)->(%s)\n", This, debugstr_w(p));
    return node_put_text( &This->node, p );
}

static HRESULT WINAPI domcdata_get_specified(
    IXMLDOMCDATASection *iface,
    VARIANT_BOOL* isSpecified)
{
    domcdata *This = impl_from_IXMLDOMCDATASection( iface );
    FIXME("(%p)->(%p) stub!\n", This, isSpecified);
    *isSpecified = VARIANT_TRUE;
    return S_OK;
}

static HRESULT WINAPI domcdata_get_definition(
    IXMLDOMCDATASection *iface,
    IXMLDOMNode** definitionNode)
{
    domcdata *This = impl_from_IXMLDOMCDATASection( iface );
    FIXME("(%p)->(%p)\n", This, definitionNode);
    return E_NOTIMPL;
}

static HRESULT WINAPI domcdata_get_nodeTypedValue(
    IXMLDOMCDATASection *iface,
    VARIANT* v)
{
    domcdata *This = impl_from_IXMLDOMCDATASection( iface );
    TRACE("(%p)->(%p)\n", This, v);
    return node_get_content(&This->node, v);
}

static HRESULT WINAPI domcdata_put_nodeTypedValue(
    IXMLDOMCDATASection *iface,
    VARIANT typedValue)
{
    domcdata *This = impl_from_IXMLDOMCDATASection( iface );
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&typedValue));
    return E_NOTIMPL;
}

static HRESULT WINAPI domcdata_get_dataType(
    IXMLDOMCDATASection *iface,
    VARIANT* typename)
{
    domcdata *This = impl_from_IXMLDOMCDATASection( iface );
    TRACE("(%p)->(%p)\n", This, typename);
    return return_null_var( typename );
}

static HRESULT WINAPI domcdata_put_dataType(
    IXMLDOMCDATASection *iface,
    BSTR p)
{
    domcdata *This = impl_from_IXMLDOMCDATASection( iface );

    TRACE("(%p)->(%s)\n", This, debugstr_w(p));

    if(!p)
        return E_INVALIDARG;

    return E_FAIL;
}

static HRESULT WINAPI domcdata_get_xml(
    IXMLDOMCDATASection *iface,
    BSTR* p)
{
    domcdata *This = impl_from_IXMLDOMCDATASection( iface );

    TRACE("(%p)->(%p)\n", This, p);

    return node_get_xml(&This->node, FALSE, p);
}

static HRESULT WINAPI domcdata_transformNode(
    IXMLDOMCDATASection *iface,
    IXMLDOMNode *node, BSTR *p)
{
    domcdata *This = impl_from_IXMLDOMCDATASection( iface );
    TRACE("(%p)->(%p %p)\n", This, node, p);
    return node_transform_node(&This->node, node, p);
}

static HRESULT WINAPI domcdata_selectNodes(
    IXMLDOMCDATASection *iface,
    BSTR p, IXMLDOMNodeList** outList)
{
    domcdata *This = impl_from_IXMLDOMCDATASection( iface );
    TRACE("(%p)->(%s %p)\n", This, debugstr_w(p), outList);
    return node_select_nodes(&This->node, p, outList);
}

static HRESULT WINAPI domcdata_selectSingleNode(
    IXMLDOMCDATASection *iface,
    BSTR p, IXMLDOMNode** outNode)
{
    domcdata *This = impl_from_IXMLDOMCDATASection( iface );
    TRACE("(%p)->(%s %p)\n", This, debugstr_w(p), outNode);
    return node_select_singlenode(&This->node, p, outNode);
}

static HRESULT WINAPI domcdata_get_parsed(
    IXMLDOMCDATASection *iface,
    VARIANT_BOOL* isParsed)
{
    domcdata *This = impl_from_IXMLDOMCDATASection( iface );
    FIXME("(%p)->(%p) stub!\n", This, isParsed);
    *isParsed = VARIANT_TRUE;
    return S_OK;
}

static HRESULT WINAPI domcdata_get_namespaceURI(
    IXMLDOMCDATASection *iface,
    BSTR* p)
{
    domcdata *This = impl_from_IXMLDOMCDATASection( iface );
    TRACE("(%p)->(%p)\n", This, p);
    return node_get_namespaceURI(&This->node, p);
}

static HRESULT WINAPI domcdata_get_prefix(
    IXMLDOMCDATASection *iface,
    BSTR* prefix)
{
    domcdata *This = impl_from_IXMLDOMCDATASection( iface );
    TRACE("(%p)->(%p)\n", This, prefix);
    return return_null_bstr( prefix );
}

static HRESULT WINAPI domcdata_get_baseName(
    IXMLDOMCDATASection *iface,
    BSTR* name)
{
    domcdata *This = impl_from_IXMLDOMCDATASection( iface );
    FIXME("(%p)->(%p): needs test\n", This, name);
    return return_null_bstr( name );
}

static HRESULT WINAPI domcdata_transformNodeToObject(
    IXMLDOMCDATASection *iface,
    IXMLDOMNode* domNode, VARIANT var1)
{
    domcdata *This = impl_from_IXMLDOMCDATASection( iface );
    FIXME("(%p)->(%p %s)\n", This, domNode, debugstr_variant(&var1));
    return E_NOTIMPL;
}

static HRESULT WINAPI domcdata_get_data(
    IXMLDOMCDATASection *iface,
    BSTR *p)
{
    HRESULT hr;
    VARIANT vRet;

    if(!p)
        return E_INVALIDARG;

    hr = IXMLDOMCDATASection_get_nodeValue( iface, &vRet );
    if(hr == S_OK)
    {
        *p = V_BSTR(&vRet);
    }

    return hr;
}

static HRESULT WINAPI domcdata_put_data(
    IXMLDOMCDATASection *iface,
    BSTR data)
{
    domcdata *This = impl_from_IXMLDOMCDATASection( iface );
    TRACE("(%p)->(%s)\n", This, debugstr_w(data));
    return node_set_content(&This->node, data);
}

static HRESULT WINAPI domcdata_get_length(
    IXMLDOMCDATASection *iface,
    LONG *len)
{
    domcdata *This = impl_from_IXMLDOMCDATASection( iface );
    HRESULT hr;
    BSTR data;

    TRACE("(%p)->(%p)\n", This, len);

    if(!len)
        return E_INVALIDARG;

    hr = IXMLDOMCDATASection_get_data(iface, &data);
    if(hr == S_OK)
    {
        *len = SysStringLen(data);
        SysFreeString(data);
    }

    return S_OK;
}

static HRESULT WINAPI domcdata_substringData(
    IXMLDOMCDATASection *iface,
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

    hr = IXMLDOMCDATASection_get_data(iface, &data);
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

static HRESULT WINAPI domcdata_appendData(
    IXMLDOMCDATASection *iface,
    BSTR p)
{
    domcdata *This = impl_from_IXMLDOMCDATASection( iface );
    HRESULT hr;
    BSTR data;
    LONG p_len;

    TRACE("(%p)->(%s)\n", This, debugstr_w(p));

    /* Nothing to do if NULL or an Empty string passed in. */
    if((p_len = SysStringLen(p)) == 0) return S_OK;

    hr = IXMLDOMCDATASection_get_data(iface, &data);
    if(hr == S_OK)
    {
        LONG len = SysStringLen(data);
        BSTR str = SysAllocStringLen(NULL, p_len + len);

        memcpy(str, data, len*sizeof(WCHAR));
        memcpy(&str[len], p, p_len*sizeof(WCHAR));
        str[len+p_len] = 0;

        hr = IXMLDOMCDATASection_put_data(iface, str);

        SysFreeString(str);
        SysFreeString(data);
    }

    return hr;
}

static HRESULT WINAPI domcdata_insertData(
    IXMLDOMCDATASection *iface,
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

    hr = IXMLDOMCDATASection_get_data(iface, &data);
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

        hr = IXMLDOMCDATASection_put_data(iface, str);

        SysFreeString(str);
        SysFreeString(data);
    }

    return hr;
}

static HRESULT WINAPI domcdata_deleteData(
    IXMLDOMCDATASection *iface,
    LONG offset, LONG count)
{
    HRESULT hr;
    LONG len = -1;
    BSTR str;

    TRACE("%p, %ld, %ld.\n", iface, offset, count);

    hr = IXMLDOMCDATASection_get_length(iface, &len);
    if(hr != S_OK) return hr;

    if((offset < 0) || (offset > len) || (count < 0))
        return E_INVALIDARG;

    if(len == 0) return S_OK;

    /* cutting start or end */
    if((offset == 0) || ((count + offset) >= len))
    {
        if(offset == 0)
            IXMLDOMCDATASection_substringData(iface, count, len - count, &str);
        else
            IXMLDOMCDATASection_substringData(iface, 0, offset, &str);
        hr = IXMLDOMCDATASection_put_data(iface, str);
    }
    else
    /* cutting from the inside */
    {
        BSTR str_end;

        IXMLDOMCDATASection_substringData(iface, 0, offset, &str);
        IXMLDOMCDATASection_substringData(iface, offset + count, len - count, &str_end);

        hr = IXMLDOMCDATASection_put_data(iface, str);
        if(hr == S_OK)
            hr = IXMLDOMCDATASection_appendData(iface, str_end);

        SysFreeString(str_end);
    }

    SysFreeString(str);

    return hr;
}

static HRESULT WINAPI domcdata_replaceData(
    IXMLDOMCDATASection *iface,
    LONG offset, LONG count, BSTR p)
{
    HRESULT hr;

    TRACE("%p, %ld, %ld, %s.\n", iface, offset, count, debugstr_w(p));

    hr = IXMLDOMCDATASection_deleteData(iface, offset, count);

    if (hr == S_OK)
       hr = IXMLDOMCDATASection_insertData(iface, offset, p);

    return hr;
}

static HRESULT WINAPI domcdata_splitText(
    IXMLDOMCDATASection *iface,
    LONG offset, IXMLDOMText **txtNode)
{
    IXMLDOMDocument *doc;
    LONG length = 0;
    HRESULT hr;

    TRACE("%p, %ld, %p.\n", iface, offset, txtNode);

    if (!txtNode || offset < 0) return E_INVALIDARG;

    *txtNode = NULL;

    IXMLDOMCDATASection_get_length(iface, &length);

    if (offset > length) return E_INVALIDARG;
    if (offset == length) return S_FALSE;

    hr = IXMLDOMCDATASection_get_ownerDocument(iface, &doc);
    if (hr == S_OK)
    {
        BSTR data;

        hr = IXMLDOMCDATASection_substringData(iface, offset, length - offset, &data);
        if (hr == S_OK)
        {
            hr = IXMLDOMDocument_createTextNode(doc, data, txtNode);
            if (hr == S_OK)
            {
                IXMLDOMNode *parent;

                hr = IXMLDOMCDATASection_get_parentNode(iface, &parent);
                if (hr == S_OK)
                {
                    IXMLDOMCDATASection_deleteData(iface, 0, offset);
                    hr = IXMLDOMNode_appendChild(parent, (IXMLDOMNode*)*txtNode, NULL);
                    IXMLDOMNode_Release(parent);
                }
            }
            SysFreeString(data);
        }
        IXMLDOMDocument_Release(doc);
    }

    return hr;
}

static const struct IXMLDOMCDATASectionVtbl domcdata_vtbl =
{
    domcdata_QueryInterface,
    domcdata_AddRef,
    domcdata_Release,
    domcdata_GetTypeInfoCount,
    domcdata_GetTypeInfo,
    domcdata_GetIDsOfNames,
    domcdata_Invoke,
    domcdata_get_nodeName,
    domcdata_get_nodeValue,
    domcdata_put_nodeValue,
    domcdata_get_nodeType,
    domcdata_get_parentNode,
    domcdata_get_childNodes,
    domcdata_get_firstChild,
    domcdata_get_lastChild,
    domcdata_get_previousSibling,
    domcdata_get_nextSibling,
    domcdata_get_attributes,
    domcdata_insertBefore,
    domcdata_replaceChild,
    domcdata_removeChild,
    domcdata_appendChild,
    domcdata_hasChildNodes,
    domcdata_get_ownerDocument,
    domcdata_cloneNode,
    domcdata_get_nodeTypeString,
    domcdata_get_text,
    domcdata_put_text,
    domcdata_get_specified,
    domcdata_get_definition,
    domcdata_get_nodeTypedValue,
    domcdata_put_nodeTypedValue,
    domcdata_get_dataType,
    domcdata_put_dataType,
    domcdata_get_xml,
    domcdata_transformNode,
    domcdata_selectNodes,
    domcdata_selectSingleNode,
    domcdata_get_parsed,
    domcdata_get_namespaceURI,
    domcdata_get_prefix,
    domcdata_get_baseName,
    domcdata_transformNodeToObject,
    domcdata_get_data,
    domcdata_put_data,
    domcdata_get_length,
    domcdata_substringData,
    domcdata_appendData,
    domcdata_insertData,
    domcdata_deleteData,
    domcdata_replaceData,
    domcdata_splitText
};

static const tid_t domcdata_iface_tids[] = {
    IXMLDOMCDATASection_tid,
    0
};

static dispex_static_data_t domcdata_dispex = {
    NULL,
    IXMLDOMCDATASection_tid,
    NULL,
    domcdata_iface_tids
};

IUnknown* create_cdata( xmlNodePtr text )
{
    domcdata *This;

    This = malloc(sizeof(*This));
    if ( !This )
        return NULL;

    This->IXMLDOMCDATASection_iface.lpVtbl = &domcdata_vtbl;
    This->ref = 1;

    init_xmlnode(&This->node, text, (IXMLDOMNode*)&This->IXMLDOMCDATASection_iface, &domcdata_dispex);

    return (IUnknown*)&This->IXMLDOMCDATASection_iface;
}
