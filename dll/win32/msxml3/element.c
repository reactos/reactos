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

#include "precomp.h"

#ifdef HAVE_LIBXML2

static const xmlChar DT_prefix[] = "dt";
static const xmlChar DT_nsURI[] = "urn:schemas-microsoft-com:datatypes";

typedef struct _domelem
{
    xmlnode node;
    IXMLDOMElement IXMLDOMElement_iface;
    LONG ref;
} domelem;

static const struct nodemap_funcs domelem_attr_map;

static const tid_t domelem_se_tids[] = {
    IXMLDOMNode_tid,
    IXMLDOMElement_tid,
    NULL_tid
};

static inline domelem *impl_from_IXMLDOMElement( IXMLDOMElement *iface )
{
    return CONTAINING_RECORD(iface, domelem, IXMLDOMElement_iface);
}

static inline xmlNodePtr get_element( const domelem *This )
{
    return This->node.node;
}

static HRESULT WINAPI domelem_QueryInterface(
    IXMLDOMElement *iface,
    REFIID riid,
    void** ppvObject )
{
    domelem *This = impl_from_IXMLDOMElement( iface );

    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppvObject);

    if ( IsEqualGUID( riid, &IID_IXMLDOMElement ) ||
         IsEqualGUID( riid, &IID_IXMLDOMNode ) ||
         IsEqualGUID( riid, &IID_IDispatch ) ||
         IsEqualGUID( riid, &IID_IUnknown ) )
    {
        *ppvObject = &This->IXMLDOMElement_iface;
    }
    else if(node_query_interface(&This->node, riid, ppvObject))
    {
        return *ppvObject ? S_OK : E_NOINTERFACE;
    }
    else if(IsEqualGUID( riid, &IID_ISupportErrorInfo ))
    {
        return node_create_supporterrorinfo(domelem_se_tids, ppvObject);
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

static ULONG WINAPI domelem_AddRef(
    IXMLDOMElement *iface )
{
    domelem *This = impl_from_IXMLDOMElement( iface );
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p)->(%d)\n", This, ref);

    return ref;
}

static ULONG WINAPI domelem_Release(
    IXMLDOMElement *iface )
{
    domelem *This = impl_from_IXMLDOMElement( iface );
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(%d)\n", This, ref);

    if(!ref) {
        destroy_xmlnode(&This->node);
        heap_free(This);
    }

    return ref;
}

static HRESULT WINAPI domelem_GetTypeInfoCount(
    IXMLDOMElement *iface,
    UINT* pctinfo )
{
    domelem *This = impl_from_IXMLDOMElement( iface );
    return IDispatchEx_GetTypeInfoCount(&This->node.dispex.IDispatchEx_iface, pctinfo);
}

static HRESULT WINAPI domelem_GetTypeInfo(
    IXMLDOMElement *iface,
    UINT iTInfo, LCID lcid,
    ITypeInfo** ppTInfo )
{
    domelem *This = impl_from_IXMLDOMElement( iface );
    return IDispatchEx_GetTypeInfo(&This->node.dispex.IDispatchEx_iface,
        iTInfo, lcid, ppTInfo);
}

static HRESULT WINAPI domelem_GetIDsOfNames(
    IXMLDOMElement *iface,
    REFIID riid, LPOLESTR* rgszNames,
    UINT cNames, LCID lcid, DISPID* rgDispId )
{
    domelem *This = impl_from_IXMLDOMElement( iface );
    return IDispatchEx_GetIDsOfNames(&This->node.dispex.IDispatchEx_iface,
        riid, rgszNames, cNames, lcid, rgDispId);
}

static HRESULT WINAPI domelem_Invoke(
    IXMLDOMElement *iface,
    DISPID dispIdMember, REFIID riid, LCID lcid,
    WORD wFlags, DISPPARAMS* pDispParams, VARIANT* pVarResult,
    EXCEPINFO* pExcepInfo, UINT* puArgErr )
{
    domelem *This = impl_from_IXMLDOMElement( iface );
    return IDispatchEx_Invoke(&This->node.dispex.IDispatchEx_iface,
        dispIdMember, riid, lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI domelem_get_nodeName(
    IXMLDOMElement *iface,
    BSTR* p )
{
    domelem *This = impl_from_IXMLDOMElement( iface );

    TRACE("(%p)->(%p)\n", This, p);

    return node_get_nodeName(&This->node, p);
}

static HRESULT WINAPI domelem_get_nodeValue(
    IXMLDOMElement *iface,
    VARIANT* value)
{
    domelem *This = impl_from_IXMLDOMElement( iface );

    TRACE("(%p)->(%p)\n", This, value);

    if(!value)
        return E_INVALIDARG;

    V_VT(value) = VT_NULL;
    V_BSTR(value) = NULL; /* tests show that we should do this */
    return S_FALSE;
}

static HRESULT WINAPI domelem_put_nodeValue(
    IXMLDOMElement *iface,
    VARIANT value)
{
    domelem *This = impl_from_IXMLDOMElement( iface );
    TRACE("(%p)->(%s)\n", This, debugstr_variant(&value));
    return E_FAIL;
}

static HRESULT WINAPI domelem_get_nodeType(
    IXMLDOMElement *iface,
    DOMNodeType* domNodeType )
{
    domelem *This = impl_from_IXMLDOMElement( iface );

    TRACE("(%p)->(%p)\n", This, domNodeType);

    *domNodeType = NODE_ELEMENT;
    return S_OK;
}

static HRESULT WINAPI domelem_get_parentNode(
    IXMLDOMElement *iface,
    IXMLDOMNode** parent )
{
    domelem *This = impl_from_IXMLDOMElement( iface );

    TRACE("(%p)->(%p)\n", This, parent);

    return node_get_parent(&This->node, parent);
}

static HRESULT WINAPI domelem_get_childNodes(
    IXMLDOMElement *iface,
    IXMLDOMNodeList** outList)
{
    domelem *This = impl_from_IXMLDOMElement( iface );

    TRACE("(%p)->(%p)\n", This, outList);

    return node_get_child_nodes(&This->node, outList);
}

static HRESULT WINAPI domelem_get_firstChild(
    IXMLDOMElement *iface,
    IXMLDOMNode** domNode)
{
    domelem *This = impl_from_IXMLDOMElement( iface );

    TRACE("(%p)->(%p)\n", This, domNode);

    return node_get_first_child(&This->node, domNode);
}

static HRESULT WINAPI domelem_get_lastChild(
    IXMLDOMElement *iface,
    IXMLDOMNode** domNode)
{
    domelem *This = impl_from_IXMLDOMElement( iface );

    TRACE("(%p)->(%p)\n", This, domNode);

    return node_get_last_child(&This->node, domNode);
}

static HRESULT WINAPI domelem_get_previousSibling(
    IXMLDOMElement *iface,
    IXMLDOMNode** domNode)
{
    domelem *This = impl_from_IXMLDOMElement( iface );

    TRACE("(%p)->(%p)\n", This, domNode);

    return node_get_previous_sibling(&This->node, domNode);
}

static HRESULT WINAPI domelem_get_nextSibling(
    IXMLDOMElement *iface,
    IXMLDOMNode** domNode)
{
    domelem *This = impl_from_IXMLDOMElement( iface );

    TRACE("(%p)->(%p)\n", This, domNode);

    return node_get_next_sibling(&This->node, domNode);
}

static HRESULT WINAPI domelem_get_attributes(
    IXMLDOMElement *iface,
    IXMLDOMNamedNodeMap** map)
{
    domelem *This = impl_from_IXMLDOMElement( iface );

    TRACE("(%p)->(%p)\n", This, map);

    *map = create_nodemap(This->node.node, &domelem_attr_map);
    return S_OK;
}

static HRESULT WINAPI domelem_insertBefore(
    IXMLDOMElement *iface,
    IXMLDOMNode* newNode, VARIANT refChild,
    IXMLDOMNode** old_node)
{
    domelem *This = impl_from_IXMLDOMElement( iface );
    DOMNodeType type;
    HRESULT hr;

    TRACE("(%p)->(%p %s %p)\n", This, newNode, debugstr_variant(&refChild), old_node);

    hr = IXMLDOMNode_get_nodeType(newNode, &type);
    if (hr != S_OK) return hr;

    TRACE("new node type %d\n", type);
    switch (type)
    {
        case NODE_DOCUMENT:
        case NODE_DOCUMENT_TYPE:
        case NODE_ENTITY:
        case NODE_NOTATION:
            if (old_node) *old_node = NULL;
            return E_FAIL;
        default:
            return node_insert_before(&This->node, newNode, &refChild, old_node);
    }
}

static HRESULT WINAPI domelem_replaceChild(
    IXMLDOMElement *iface,
    IXMLDOMNode* newNode,
    IXMLDOMNode* oldNode,
    IXMLDOMNode** outOldNode)
{
    domelem *This = impl_from_IXMLDOMElement( iface );

    TRACE("(%p)->(%p %p %p)\n", This, newNode, oldNode, outOldNode);

    return node_replace_child(&This->node, newNode, oldNode, outOldNode);
}

static HRESULT WINAPI domelem_removeChild(
    IXMLDOMElement *iface,
    IXMLDOMNode *child, IXMLDOMNode **oldChild)
{
    domelem *This = impl_from_IXMLDOMElement( iface );
    TRACE("(%p)->(%p %p)\n", This, child, oldChild);
    return node_remove_child(&This->node, child, oldChild);
}

static HRESULT WINAPI domelem_appendChild(
    IXMLDOMElement *iface,
    IXMLDOMNode *child, IXMLDOMNode **outChild)
{
    domelem *This = impl_from_IXMLDOMElement( iface );
    TRACE("(%p)->(%p %p)\n", This, child, outChild);
    return node_append_child(&This->node, child, outChild);
}

static HRESULT WINAPI domelem_hasChildNodes(
    IXMLDOMElement *iface,
    VARIANT_BOOL *ret)
{
    domelem *This = impl_from_IXMLDOMElement( iface );
    TRACE("(%p)->(%p)\n", This, ret);
    return node_has_childnodes(&This->node, ret);
}

static HRESULT WINAPI domelem_get_ownerDocument(
    IXMLDOMElement   *iface,
    IXMLDOMDocument **doc)
{
    domelem *This = impl_from_IXMLDOMElement( iface );
    TRACE("(%p)->(%p)\n", This, doc);
    return node_get_owner_doc(&This->node, doc);
}

static HRESULT WINAPI domelem_cloneNode(
    IXMLDOMElement *iface,
    VARIANT_BOOL deep, IXMLDOMNode** outNode)
{
    domelem *This = impl_from_IXMLDOMElement( iface );
    TRACE("(%p)->(%d %p)\n", This, deep, outNode);
    return node_clone( &This->node, deep, outNode );
}

static HRESULT WINAPI domelem_get_nodeTypeString(
    IXMLDOMElement *iface,
    BSTR* p)
{
    domelem *This = impl_from_IXMLDOMElement( iface );
    static const WCHAR elementW[] = {'e','l','e','m','e','n','t',0};

    TRACE("(%p)->(%p)\n", This, p);

    return return_bstr(elementW, p);
}

static HRESULT WINAPI domelem_get_text(
    IXMLDOMElement *iface,
    BSTR* p)
{
    domelem *This = impl_from_IXMLDOMElement( iface );
    TRACE("(%p)->(%p)\n", This, p);
    return node_get_text(&This->node, p);
}

static HRESULT WINAPI domelem_put_text(
    IXMLDOMElement *iface,
    BSTR p)
{
    domelem *This = impl_from_IXMLDOMElement( iface );
    TRACE("(%p)->(%s)\n", This, debugstr_w(p));
    return node_put_text( &This->node, p );
}

static HRESULT WINAPI domelem_get_specified(
    IXMLDOMElement *iface,
    VARIANT_BOOL* isSpecified)
{
    domelem *This = impl_from_IXMLDOMElement( iface );
    FIXME("(%p)->(%p) stub!\n", This, isSpecified);
    *isSpecified = VARIANT_TRUE;
    return S_OK;
}

static HRESULT WINAPI domelem_get_definition(
    IXMLDOMElement *iface,
    IXMLDOMNode** definitionNode)
{
    domelem *This = impl_from_IXMLDOMElement( iface );
    FIXME("(%p)->(%p)\n", This, definitionNode);
    return E_NOTIMPL;
}

static inline BYTE hex_to_byte(xmlChar c)
{
    if(c <= '9') return c-'0';
    if(c <= 'F') return c-'A'+10;
    return c-'a'+10;
}

static inline BYTE base64_to_byte(xmlChar c)
{
    if(c == '+') return 62;
    if(c == '/') return 63;
    if(c <= '9') return c-'0'+52;
    if(c <= 'Z') return c-'A';
    return c-'a'+26;
}

static inline HRESULT variant_from_dt(XDR_DT dt, xmlChar* str, VARIANT* v)
{
    VARIANT src;
    HRESULT hr = S_OK;
    BOOL handled = FALSE;

    VariantInit(&src);

    switch (dt)
    {
    case DT_INVALID:
    case DT_STRING:
    case DT_NMTOKEN:
    case DT_NMTOKENS:
    case DT_NUMBER:
    case DT_URI:
    case DT_UUID:
        {
            V_VT(v) = VT_BSTR;
            V_BSTR(v) = bstr_from_xmlChar(str);

            if(!V_BSTR(v))
                return E_OUTOFMEMORY;
            handled = TRUE;
        }
        break;
    case DT_DATE:
    case DT_DATE_TZ:
    case DT_DATETIME:
    case DT_DATETIME_TZ:
    case DT_TIME:
    case DT_TIME_TZ:
        {
            WCHAR *p, *e;
            SYSTEMTIME st;
            DOUBLE date = 0.0;

            st.wYear = 1899;
            st.wMonth = 12;
            st.wDay = 30;
            st.wDayOfWeek = st.wHour = st.wMinute = st.wSecond = st.wMilliseconds = 0;

            V_VT(&src) = VT_BSTR;
            V_BSTR(&src) = bstr_from_xmlChar(str);

            if(!V_BSTR(&src))
                return E_OUTOFMEMORY;

            p = V_BSTR(&src);
            e = p + SysStringLen(V_BSTR(&src));

            if(p+4<e && *(p+4)=='-') /* parse date (yyyy-mm-dd) */
            {
                st.wYear = atoiW(p);
                st.wMonth = atoiW(p+5);
                st.wDay = atoiW(p+8);
                p += 10;

                if(*p == 'T') p++;
            }

            if(p+2<e && *(p+2)==':') /* parse time (hh:mm:ss.?) */
            {
                st.wHour = atoiW(p);
                st.wMinute = atoiW(p+3);
                st.wSecond = atoiW(p+6);
                p += 8;

                if(*p == '.')
                {
                    p++;
                    while(isdigitW(*p)) p++;
                }
            }

            SystemTimeToVariantTime(&st, &date);
            V_VT(v) = VT_DATE;
            V_DATE(v) = date;

            if(*p == '+') /* parse timezone offset (+hh:mm) */
                V_DATE(v) += (DOUBLE)atoiW(p+1)/24 + (DOUBLE)atoiW(p+4)/1440;
            else if(*p == '-') /* parse timezone offset (-hh:mm) */
                V_DATE(v) -= (DOUBLE)atoiW(p+1)/24 + (DOUBLE)atoiW(p+4)/1440;

            VariantClear(&src);
            handled = TRUE;
        }
        break;
    case DT_BIN_HEX:
        {
            SAFEARRAYBOUND sab;
            int i, len;

            len = xmlStrlen(str)/2;
            sab.lLbound = 0;
            sab.cElements = len;

            V_VT(v) = (VT_ARRAY|VT_UI1);
            V_ARRAY(v) = SafeArrayCreate(VT_UI1, 1, &sab);

            if(!V_ARRAY(v))
                return E_OUTOFMEMORY;

            for(i=0; i<len; i++)
                ((BYTE*)V_ARRAY(v)->pvData)[i] = (hex_to_byte(str[2*i])<<4)
                    + hex_to_byte(str[2*i+1]);
            handled = TRUE;
        }
        break;
    case DT_BIN_BASE64:
        {
            SAFEARRAYBOUND sab;
            xmlChar *c1, *c2;
            int i, len;

            /* remove all formatting chars */
            c1 = c2 = str;
            len = 0;
            while (*c2)
            {
                if ( *c2 == ' '  || *c2 == '\t' ||
                     *c2 == '\n' || *c2 == '\r' )
                {
                    c2++;
                    continue;
                }
                *c1++ = *c2++;
                len++;
            }

            /* skip padding */
            if(str[len-2] == '=') i = 2;
            else if(str[len-1] == '=') i = 1;
            else i = 0;

            sab.lLbound = 0;
            sab.cElements = len/4*3-i;

            V_VT(v) = (VT_ARRAY|VT_UI1);
            V_ARRAY(v) = SafeArrayCreate(VT_UI1, 1, &sab);

            if(!V_ARRAY(v))
                return E_OUTOFMEMORY;

            for(i=0; i<len/4; i++)
            {
                ((BYTE*)V_ARRAY(v)->pvData)[3*i] = (base64_to_byte(str[4*i])<<2)
                    + (base64_to_byte(str[4*i+1])>>4);
                if(3*i+1 < sab.cElements)
                    ((BYTE*)V_ARRAY(v)->pvData)[3*i+1] = (base64_to_byte(str[4*i+1])<<4)
                        + (base64_to_byte(str[4*i+2])>>2);
                if(3*i+2 < sab.cElements)
                    ((BYTE*)V_ARRAY(v)->pvData)[3*i+2] = (base64_to_byte(str[4*i+2])<<6)
                        + base64_to_byte(str[4*i+3]);
            }
            handled = TRUE;
        }
        break;
    case DT_BOOLEAN:
        V_VT(v) = VT_BOOL;
        break;
    case DT_FIXED_14_4:
        V_VT(v) = VT_CY;
        break;
    case DT_I1:
        V_VT(v) = VT_I1;
        break;
    case DT_I2:
        V_VT(v) = VT_I2;
        break;
    case DT_I4:
    case DT_INT:
        V_VT(v) = VT_I4;
        break;
    case DT_I8:
        V_VT(v) = VT_I8;
        break;
    case DT_R4:
        V_VT(v) = VT_R4;
        break;
    case DT_FLOAT:
    case DT_R8:
        V_VT(v) = VT_R8;
        break;
    case DT_UI1:
        V_VT(v) = VT_UI1;
        break;
    case DT_UI2:
        V_VT(v) = VT_UI2;
        break;
    case DT_UI4:
        V_VT(v) = VT_UI4;
        break;
    case DT_UI8:
        V_VT(v) = VT_UI8;
        break;
    case DT_CHAR:
    case DT_ENTITY:
    case DT_ENTITIES:
    case DT_ENUMERATION:
    case DT_ID:
    case DT_IDREF:
    case DT_IDREFS:
    case DT_NOTATION:
        FIXME("need to handle dt:%s\n", debugstr_dt(dt));
        V_VT(v) = VT_BSTR;
        V_BSTR(v) = bstr_from_xmlChar(str);
        if (!V_BSTR(v))
            return E_OUTOFMEMORY;
        handled = TRUE;
        break;
    default:
        WARN("unknown type %d\n", dt);
    }

    if (!handled)
    {
        V_VT(&src) = VT_BSTR;
        V_BSTR(&src) = bstr_from_xmlChar(str);

        if(!V_BSTR(&src))
            return E_OUTOFMEMORY;

        hr = VariantChangeTypeEx(v, &src,
                MAKELCID(MAKELANGID(LANG_ENGLISH,SUBLANG_ENGLISH_US),SORT_DEFAULT),0, V_VT(v));
        VariantClear(&src);
    }
    return hr;
}

static XDR_DT element_get_dt(xmlNodePtr node)
{
    XDR_DT dt = DT_INVALID;

    TRACE("(%p)\n", node);
    if(node->type != XML_ELEMENT_NODE)
    {
        FIXME("invalid element node\n");
        return dt;
    }

    if (node->ns && xmlStrEqual(node->ns->href, DT_nsURI))
    {
        dt = str_to_dt(node->name, -1);
    }
    else
    {
        xmlChar* pVal = xmlGetNsProp(node, BAD_CAST "dt", DT_nsURI);
        if (pVal)
        {
            dt = str_to_dt(pVal, -1);
            xmlFree(pVal);
        }
        else if (node->doc)
        {
            IXMLDOMDocument3* doc = (IXMLDOMDocument3*)create_domdoc((xmlNodePtr)node->doc);
            if (doc)
            {
                VARIANT v;
                VariantInit(&v);

                if (IXMLDOMDocument3_get_schemas(doc, &v) == S_OK &&
                    V_VT(&v) == VT_DISPATCH)
                {
                    dt = SchemaCache_get_node_dt((IXMLDOMSchemaCollection2*)V_DISPATCH(&v), node);
                }
                VariantClear(&v);
                IXMLDOMDocument3_Release(doc);
            }
        }
    }

    TRACE("=> dt:%s\n", debugstr_dt(dt));
    return dt;
}

static HRESULT WINAPI domelem_get_nodeTypedValue(
    IXMLDOMElement *iface,
    VARIANT* v)
{
    domelem *This = impl_from_IXMLDOMElement( iface );
    XDR_DT dt;
    xmlChar* content;
    HRESULT hr;

    TRACE("(%p)->(%p)\n", This, v);

    if(!v) return E_INVALIDARG;

    V_VT(v) = VT_NULL;

    dt = element_get_dt(get_element(This));
    content = xmlNodeGetContent(get_element(This));
    hr = variant_from_dt(dt, content, v);
    xmlFree(content);

    return hr;
}

static HRESULT encode_base64(const BYTE *buf, int len, BSTR *ret)
{
    static const char b64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    const BYTE *d = buf;
    int bytes, pad_bytes, div;
    DWORD needed;
    WCHAR *ptr;

    bytes = (len*8 + 5)/6;
    pad_bytes = (bytes % 4) ? 4 - (bytes % 4) : 0;

    TRACE("%d, bytes is %d, pad bytes is %d\n", len, bytes, pad_bytes);
    needed = bytes + pad_bytes + 1;

    *ret = SysAllocStringLen(NULL, needed);
    if (!*ret) return E_OUTOFMEMORY;

    /* Three bytes of input give 4 chars of output */
    div = len / 3;

    ptr = *ret;
    while (div > 0)
    {
        /* first char is the first 6 bits of the first byte*/
        *ptr++ = b64[ ( d[0] >> 2) & 0x3f ];
        /* second char is the last 2 bits of the first byte and the first 4
         * bits of the second byte */
        *ptr++ = b64[ ((d[0] << 4) & 0x30) | (d[1] >> 4 & 0x0f)];
        /* third char is the last 4 bits of the second byte and the first 2
         * bits of the third byte */
        *ptr++ = b64[ ((d[1] << 2) & 0x3c) | (d[2] >> 6 & 0x03)];
        /* fourth char is the remaining 6 bits of the third byte */
        *ptr++ = b64[   d[2]       & 0x3f];
        d += 3;
        div--;
    }

    switch (pad_bytes)
    {
        case 1:
            /* first char is the first 6 bits of the first byte*/
            *ptr++ = b64[ ( d[0] >> 2) & 0x3f ];
            /* second char is the last 2 bits of the first byte and the first 4
             * bits of the second byte */
            *ptr++ = b64[ ((d[0] << 4) & 0x30) | (d[1] >> 4 & 0x0f)];
            /* third char is the last 4 bits of the second byte padded with
             * two zeroes */
            *ptr++ = b64[ ((d[1] << 2) & 0x3c) ];
            /* fourth char is a = to indicate one byte of padding */
            *ptr++ = '=';
            break;
        case 2:
            /* first char is the first 6 bits of the first byte*/
            *ptr++ = b64[ ( d[0] >> 2) & 0x3f ];
            /* second char is the last 2 bits of the first byte padded with
             * four zeroes*/
            *ptr++ = b64[ ((d[0] << 4) & 0x30)];
            /* third char is = to indicate padding */
            *ptr++ = '=';
            /* fourth char is = to indicate padding */
            *ptr++ = '=';
            break;
    }

    return S_OK;
}

static HRESULT encode_binhex(const BYTE *buf, int len, BSTR *ret)
{
    static const char byte_to_hex[16] = "0123456789abcdef";
    int i;

    *ret = SysAllocStringLen(NULL, len*2);
    if (!*ret) return E_OUTOFMEMORY;

    for (i = 0; i < len; i++)
    {
        (*ret)[2*i]   = byte_to_hex[buf[i] >> 4];
        (*ret)[2*i+1] = byte_to_hex[0x0f & buf[i]];
    }

    return S_OK;
}

static HRESULT WINAPI domelem_put_nodeTypedValue(
    IXMLDOMElement *iface,
    VARIANT value)
{
    domelem *This = impl_from_IXMLDOMElement( iface );
    XDR_DT dt;
    HRESULT hr;

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&value));

    dt = element_get_dt(get_element(This));
    switch (dt)
    {
    /* for untyped node coerce to BSTR and set */
    case DT_INVALID:
        if (V_VT(&value) != VT_BSTR)
        {
            VARIANT content;
            VariantInit(&content);
            hr = VariantChangeType(&content, &value, 0, VT_BSTR);
            if (hr == S_OK)
            {
                hr = node_set_content(&This->node, V_BSTR(&content));
                VariantClear(&content);
            }
        }
        else
            hr = node_set_content(&This->node, V_BSTR(&value));
        break;
    case DT_BIN_BASE64:
        if (V_VT(&value) == VT_BSTR)
            hr = node_set_content(&This->node, V_BSTR(&value));
        else if (V_VT(&value) == (VT_UI1|VT_ARRAY))
        {
            UINT dim = SafeArrayGetDim(V_ARRAY(&value));
            LONG lbound, ubound;
            BSTR encoded;
            BYTE *ptr;
            int len;

            if (dim > 1)
                FIXME("unexpected array dimension count %u\n", dim);

            SafeArrayGetUBound(V_ARRAY(&value), 1, &ubound);
            SafeArrayGetLBound(V_ARRAY(&value), 1, &lbound);

            len = (ubound - lbound + 1)*SafeArrayGetElemsize(V_ARRAY(&value));

            hr = SafeArrayAccessData(V_ARRAY(&value), (void*)&ptr);
            if (FAILED(hr)) return hr;

            hr = encode_base64(ptr, len, &encoded);
            SafeArrayUnaccessData(V_ARRAY(&value));
            if (FAILED(hr)) return hr;

            hr = node_set_content(&This->node, encoded);
            SysFreeString(encoded);
        }
        else
        {
            FIXME("unhandled variant type %d for dt:%s\n", V_VT(&value), debugstr_dt(dt));
            return E_NOTIMPL;
        }
        break;
    case DT_BIN_HEX:
        if (V_VT(&value) == (VT_UI1|VT_ARRAY))
        {
            UINT dim = SafeArrayGetDim(V_ARRAY(&value));
            LONG lbound, ubound;
            BSTR encoded;
            BYTE *ptr;
            int len;

            if (dim > 1)
                FIXME("unexpected array dimension count %u\n", dim);

            SafeArrayGetUBound(V_ARRAY(&value), 1, &ubound);
            SafeArrayGetLBound(V_ARRAY(&value), 1, &lbound);

            len = (ubound - lbound + 1)*SafeArrayGetElemsize(V_ARRAY(&value));

            hr = SafeArrayAccessData(V_ARRAY(&value), (void*)&ptr);
            if (FAILED(hr)) return hr;

            hr = encode_binhex(ptr, len, &encoded);
            SafeArrayUnaccessData(V_ARRAY(&value));
            if (FAILED(hr)) return hr;

            hr = node_set_content(&This->node, encoded);
            SysFreeString(encoded);
        }
        else
        {
            FIXME("unhandled variant type %d for dt:%s\n", V_VT(&value), debugstr_dt(dt));
            return E_NOTIMPL;
        }
        break;
    default:
        FIXME("not implemented for dt:%s\n", debugstr_dt(dt));
        return E_NOTIMPL;
    }

    return hr;
}

static HRESULT WINAPI domelem_get_dataType(
    IXMLDOMElement *iface,
    VARIANT* typename)
{
    domelem *This = impl_from_IXMLDOMElement( iface );
    XDR_DT dt;

    TRACE("(%p)->(%p)\n", This, typename);

    if (!typename)
        return E_INVALIDARG;

    dt = element_get_dt(get_element(This));
    switch (dt)
    {
        case DT_BIN_BASE64:
        case DT_BIN_HEX:
        case DT_BOOLEAN:
        case DT_CHAR:
        case DT_DATE:
        case DT_DATE_TZ:
        case DT_DATETIME:
        case DT_DATETIME_TZ:
        case DT_FIXED_14_4:
        case DT_FLOAT:
        case DT_I1:
        case DT_I2:
        case DT_I4:
        case DT_I8:
        case DT_INT:
        case DT_NUMBER:
        case DT_R4:
        case DT_R8:
        case DT_TIME:
        case DT_TIME_TZ:
        case DT_UI1:
        case DT_UI2:
        case DT_UI4:
        case DT_UI8:
        case DT_URI:
        case DT_UUID:
            V_VT(typename) = VT_BSTR;
            V_BSTR(typename) = SysAllocString(dt_to_bstr(dt));

            if (!V_BSTR(typename))
                return E_OUTOFMEMORY;
            break;
        default:
            /* Other types (DTD equivalents) do not return anything here,
             * but the pointer part of the VARIANT is set to NULL */
            V_VT(typename) = VT_NULL;
            V_BSTR(typename) = NULL;
            break;
    }
    return (V_VT(typename) != VT_NULL) ? S_OK : S_FALSE;
}

static HRESULT WINAPI domelem_put_dataType(
    IXMLDOMElement *iface,
    BSTR dtName)
{
    domelem *This = impl_from_IXMLDOMElement( iface );
    HRESULT hr = E_FAIL;
    xmlChar *str;
    XDR_DT dt;

    TRACE("(%p)->(%s)\n", This, debugstr_w(dtName));

    if(dtName == NULL)
        return E_INVALIDARG;

    dt = bstr_to_dt(dtName, -1);

    /* An example of this is. The Text in the node needs to be a 0 or 1 for a boolean type.
       This applies to changing types (string->bool) or setting a new one
     */
    str = xmlNodeGetContent(get_element(This));
    hr = dt_validate(dt, str);
    xmlFree(str);

    /* Check all supported types. */
    if (hr == S_OK)
    {
        switch (dt)
        {
        case DT_BIN_BASE64:
        case DT_BIN_HEX:
        case DT_BOOLEAN:
        case DT_CHAR:
        case DT_DATE:
        case DT_DATE_TZ:
        case DT_DATETIME:
        case DT_DATETIME_TZ:
        case DT_FIXED_14_4:
        case DT_FLOAT:
        case DT_I1:
        case DT_I2:
        case DT_I4:
        case DT_I8:
        case DT_INT:
        case DT_NMTOKEN:
        case DT_NMTOKENS:
        case DT_NUMBER:
        case DT_R4:
        case DT_R8:
        case DT_STRING:
        case DT_TIME:
        case DT_TIME_TZ:
        case DT_UI1:
        case DT_UI2:
        case DT_UI4:
        case DT_UI8:
        case DT_URI:
        case DT_UUID:
            {
                xmlAttrPtr attr = xmlHasNsProp(get_element(This), DT_prefix, DT_nsURI);
                if (attr)
                {
                    attr = xmlSetNsProp(get_element(This), attr->ns, DT_prefix, dt_to_str(dt));
                    hr = S_OK;
                }
                else
                {
                    xmlNsPtr ns = xmlNewNs(get_element(This), DT_nsURI, DT_prefix);
                    if (ns)
                    {
                        attr = xmlNewNsProp(get_element(This), ns, DT_prefix, dt_to_str(dt));
                        if (attr)
                        {
                            xmlAddChild(get_element(This), (xmlNodePtr)attr);
                            hr = S_OK;
                        }
                        else
                            ERR("Failed to create Attribute\n");
                    }
                    else
                        ERR("Failed to create Namespace\n");
                }
            }
            break;
        default:
            FIXME("need to handle dt:%s\n", debugstr_dt(dt));
            break;
        }
    }

    return hr;
}

static HRESULT WINAPI domelem_get_xml(
    IXMLDOMElement *iface,
    BSTR* p)
{
    domelem *This = impl_from_IXMLDOMElement( iface );

    TRACE("(%p)->(%p)\n", This, p);

    return node_get_xml(&This->node, TRUE, p);
}

static HRESULT WINAPI domelem_transformNode(
    IXMLDOMElement *iface,
    IXMLDOMNode *node, BSTR *p)
{
    domelem *This = impl_from_IXMLDOMElement( iface );
    TRACE("(%p)->(%p %p)\n", This, node, p);
    return node_transform_node(&This->node, node, p);
}

static HRESULT WINAPI domelem_selectNodes(
    IXMLDOMElement *iface,
    BSTR p, IXMLDOMNodeList** outList)
{
    domelem *This = impl_from_IXMLDOMElement( iface );
    TRACE("(%p)->(%s %p)\n", This, debugstr_w(p), outList);
    return node_select_nodes(&This->node, p, outList);
}

static HRESULT WINAPI domelem_selectSingleNode(
    IXMLDOMElement *iface,
    BSTR p, IXMLDOMNode** outNode)
{
    domelem *This = impl_from_IXMLDOMElement( iface );
    TRACE("(%p)->(%s %p)\n", This, debugstr_w(p), outNode);
    return node_select_singlenode(&This->node, p, outNode);
}

static HRESULT WINAPI domelem_get_parsed(
    IXMLDOMElement *iface,
    VARIANT_BOOL* isParsed)
{
    domelem *This = impl_from_IXMLDOMElement( iface );
    FIXME("(%p)->(%p) stub!\n", This, isParsed);
    *isParsed = VARIANT_TRUE;
    return S_OK;
}

static HRESULT WINAPI domelem_get_namespaceURI(
    IXMLDOMElement *iface,
    BSTR* p)
{
    domelem *This = impl_from_IXMLDOMElement( iface );
    TRACE("(%p)->(%p)\n", This, p);
    return node_get_namespaceURI(&This->node, p);
}

static HRESULT WINAPI domelem_get_prefix(
    IXMLDOMElement *iface,
    BSTR* prefix)
{
    domelem *This = impl_from_IXMLDOMElement( iface );
    TRACE("(%p)->(%p)\n", This, prefix);
    return node_get_prefix( &This->node, prefix );
}

static HRESULT WINAPI domelem_get_baseName(
    IXMLDOMElement *iface,
    BSTR* name)
{
    domelem *This = impl_from_IXMLDOMElement( iface );
    TRACE("(%p)->(%p)\n", This, name);
    return node_get_base_name( &This->node, name );
}

static HRESULT WINAPI domelem_transformNodeToObject(
    IXMLDOMElement *iface,
    IXMLDOMNode* domNode, VARIANT var1)
{
    domelem *This = impl_from_IXMLDOMElement( iface );
    FIXME("(%p)->(%p %s)\n", This, domNode, debugstr_variant(&var1));
    return E_NOTIMPL;
}

static HRESULT WINAPI domelem_get_tagName(
    IXMLDOMElement *iface,
    BSTR* p)
{
    domelem *This = impl_from_IXMLDOMElement( iface );
    xmlNodePtr element;
    const xmlChar *prefix;
    xmlChar *qname;

    TRACE("(%p)->(%p)\n", This, p );

    if (!p) return E_INVALIDARG;

    element = get_element( This );
    if ( !element )
        return E_FAIL;

    prefix = element->ns ? element->ns->prefix : NULL;
    qname = xmlBuildQName(element->name, prefix, NULL, 0);

    *p = bstr_from_xmlChar(qname);
    if (qname != element->name) xmlFree(qname);

    return *p ? S_OK : E_OUTOFMEMORY;
}

static HRESULT WINAPI domelem_getAttribute(
    IXMLDOMElement *iface,
    BSTR name, VARIANT* value)
{
    domelem *This = impl_from_IXMLDOMElement( iface );
    xmlNodePtr element;
    xmlChar *xml_name, *xml_value = NULL;
    xmlChar *local, *prefix;
    HRESULT hr = S_FALSE;
    xmlNsPtr ns;

    TRACE("(%p)->(%s %p)\n", This, debugstr_w(name), value);

    if(!value || !name)
        return E_INVALIDARG;

    element = get_element( This );
    if ( !element )
        return E_FAIL;

    V_BSTR(value) = NULL;
    V_VT(value) = VT_NULL;

    xml_name = xmlchar_from_wchar( name );

    if(!xmlValidateNameValue(xml_name))
        hr = E_FAIL;
    else
    {
        if ((local = xmlSplitQName2(xml_name, &prefix)))
        {
            if (xmlStrEqual(prefix, BAD_CAST "xmlns"))
            {
                ns = xmlSearchNs(element->doc, element, local);
                if (ns)
                    xml_value = xmlStrdup(ns->href);
            }
            else
            {
                ns = xmlSearchNs(element->doc, element, prefix);
                if (ns)
                    xml_value = xmlGetNsProp(element, local, ns->href);
            }

            xmlFree(prefix);
            xmlFree(local);
        }
        else
            xml_value = xmlGetNsProp(element, xml_name, NULL);
    }

    heap_free(xml_name);
    if(xml_value)
    {
        V_VT(value) = VT_BSTR;
        V_BSTR(value) = bstr_from_xmlChar( xml_value );
        xmlFree(xml_value);
        hr = S_OK;
    }

    return hr;
}

static HRESULT WINAPI domelem_setAttribute(
    IXMLDOMElement *iface,
    BSTR name, VARIANT value)
{
    domelem *This = impl_from_IXMLDOMElement( iface );
    xmlChar *xml_name, *xml_value, *local, *prefix;
    xmlNodePtr element;
    HRESULT hr = S_OK;

    TRACE("(%p)->(%s %s)\n", This, debugstr_w(name), debugstr_variant(&value));

    element = get_element( This );
    if ( !element )
        return E_FAIL;

    if (V_VT(&value) != VT_BSTR)
    {
        VARIANT var;

        VariantInit(&var);
        hr = VariantChangeType(&var, &value, 0, VT_BSTR);
        if (hr != S_OK)
        {
            FIXME("VariantChangeType failed\n");
            return hr;
        }

        xml_value = xmlchar_from_wchar(V_BSTR(&var));
        VariantClear(&var);
    }
    else
        xml_value = xmlchar_from_wchar(V_BSTR(&value));

    xml_name = xmlchar_from_wchar( name );

    if ((local = xmlSplitQName2(xml_name, &prefix)))
    {
        static const xmlChar* xmlnsA = (const xmlChar*)"xmlns";
        xmlNsPtr ns = NULL;

        /* it's not allowed to modify existing namespace definition */
        if (xmlStrEqual(prefix, xmlnsA))
            ns = xmlSearchNs(element->doc, element, local);

        xmlFree(prefix);
        xmlFree(local);

        if (ns)
        {
            int cmp = xmlStrEqual(ns->href, xml_value);
            heap_free(xml_value);
            heap_free(xml_name);
            return cmp ? S_OK : E_INVALIDARG;
        }
    }

    if (!xmlSetNsProp(element, NULL, xml_name, xml_value))
        hr = E_FAIL;

    heap_free(xml_value);
    heap_free(xml_name);

    return hr;
}

static HRESULT WINAPI domelem_removeAttribute(
    IXMLDOMElement *iface,
    BSTR p)
{
    domelem *This = impl_from_IXMLDOMElement( iface );
    IXMLDOMNamedNodeMap *attr;
    HRESULT hr;

    TRACE("(%p)->(%s)\n", This, debugstr_w(p));

    hr = IXMLDOMElement_get_attributes(iface, &attr);
    if (hr != S_OK) return hr;

    hr = IXMLDOMNamedNodeMap_removeNamedItem(attr, p, NULL);
    IXMLDOMNamedNodeMap_Release(attr);

    return hr;
}

static HRESULT WINAPI domelem_getAttributeNode(
    IXMLDOMElement *iface,
    BSTR p, IXMLDOMAttribute** attributeNode )
{
    domelem *This = impl_from_IXMLDOMElement( iface );
    xmlChar *local, *prefix, *nameA;
    HRESULT hr = S_FALSE;
    xmlNodePtr element;
    xmlAttrPtr attr;

    TRACE("(%p)->(%s %p)\n", This, debugstr_w(p), attributeNode);

    element = get_element( This );
    if (!element) return E_FAIL;

    if (attributeNode) *attributeNode = NULL;

    nameA = xmlchar_from_wchar(p);
    if (!xmlValidateNameValue(nameA))
    {
        heap_free(nameA);
        return E_FAIL;
    }

    if (!attributeNode)
    {
        heap_free(nameA);
        return S_FALSE;
    }

    *attributeNode = NULL;

    local = xmlSplitQName2(nameA, &prefix);

    if (local)
    {
        /* try to get namespace for supplied qualified name */
        xmlNsPtr ns = xmlSearchNs(element->doc, element, prefix);
        xmlFree(prefix);

        attr = xmlHasNsProp(element, local, ns ? ns->href : NULL);
        xmlFree(local);
    }
    else
    {
        attr = xmlHasProp(element, nameA);
        /* attribute has attached namespace and we requested non-qualified
           name - it's a failure case */
        if (attr && attr->ns) attr = NULL;
    }

    heap_free(nameA);

    if (attr)
    {
        IUnknown *unk = create_attribute((xmlNodePtr)attr);
        hr = IUnknown_QueryInterface(unk, &IID_IXMLDOMAttribute, (void**)attributeNode);
        IUnknown_Release(unk);
    }

    return hr;
}

static HRESULT WINAPI domelem_setAttributeNode(
    IXMLDOMElement *iface,
    IXMLDOMAttribute* attribute,
    IXMLDOMAttribute** old)
{
    domelem *This = impl_from_IXMLDOMElement( iface );
    static const WCHAR xmlnsW[] = {'x','m','l','n','s',0};
    xmlChar *name, *value;
    BSTR nameW, prefix;
    xmlnode *attr_node;
    xmlAttrPtr attr;
    VARIANT valueW;
    HRESULT hr;

    FIXME("(%p)->(%p %p): semi-stub\n", This, attribute, old);

    if (!attribute) return E_INVALIDARG;

    attr_node = get_node_obj((IXMLDOMNode*)attribute);
    if (!attr_node) return E_FAIL;

    if (attr_node->parent)
    {
        WARN("attempt to add already used attribute\n");
        return E_FAIL;
    }

    hr = IXMLDOMAttribute_get_nodeName(attribute, &nameW);
    if (hr != S_OK) return hr;

    /* adding xmlns attribute doesn't change a tree or existing namespace definition */
    if (!strcmpW(nameW, xmlnsW))
    {
        SysFreeString(nameW);
        return DISP_E_UNKNOWNNAME;
    }

    hr = IXMLDOMAttribute_get_nodeValue(attribute, &valueW);
    if (hr != S_OK)
    {
        SysFreeString(nameW);
        return hr;
    }

    if (old) *old = NULL;

    TRACE("attribute: %s=%s\n", debugstr_w(nameW), debugstr_w(V_BSTR(&valueW)));

    hr = IXMLDOMAttribute_get_prefix(attribute, &prefix);
    if (hr == S_OK)
    {
        FIXME("namespaces not supported: %s\n", debugstr_w(prefix));
        SysFreeString(prefix);
    }

    name = xmlchar_from_wchar(nameW);
    value = xmlchar_from_wchar(V_BSTR(&valueW));

    if (!name || !value)
    {
        SysFreeString(nameW);
        VariantClear(&valueW);
        heap_free(name);
        heap_free(value);
        return E_OUTOFMEMORY;
    }

    attr = xmlSetNsProp(get_element(This), NULL, name, value);
    if (attr)
        attr_node->parent = (IXMLDOMNode*)iface;

    SysFreeString(nameW);
    VariantClear(&valueW);
    heap_free(name);
    heap_free(value);

    return attr ? S_OK : E_FAIL;
}

static HRESULT WINAPI domelem_removeAttributeNode(
    IXMLDOMElement *iface,
    IXMLDOMAttribute* domAttribute,
    IXMLDOMAttribute** attributeNode)
{
    domelem *This = impl_from_IXMLDOMElement( iface );
    FIXME("(%p)->(%p %p)\n", This, domAttribute, attributeNode);
    return E_NOTIMPL;
}

static HRESULT WINAPI domelem_getElementsByTagName(
    IXMLDOMElement *iface,
    BSTR tagName, IXMLDOMNodeList** resultList)
{
    domelem *This = impl_from_IXMLDOMElement( iface );
    xmlChar *query;
    HRESULT hr;
    BOOL XPath;

    TRACE("(%p)->(%s, %p)\n", This, debugstr_w(tagName), resultList);

    if (!tagName || !resultList) return E_INVALIDARG;

    XPath = is_xpathmode(get_element(This)->doc);
    set_xpathmode(get_element(This)->doc, TRUE);
    query = tagName_to_XPath(tagName);
    hr = create_selection(get_element(This), query, resultList);
    xmlFree(query);
    set_xpathmode(get_element(This)->doc, XPath);

    return hr;
}

static HRESULT WINAPI domelem_normalize(
    IXMLDOMElement *iface )
{
    domelem *This = impl_from_IXMLDOMElement( iface );
    FIXME("%p\n", This);
    return E_NOTIMPL;
}

static const struct IXMLDOMElementVtbl domelem_vtbl =
{
    domelem_QueryInterface,
    domelem_AddRef,
    domelem_Release,
    domelem_GetTypeInfoCount,
    domelem_GetTypeInfo,
    domelem_GetIDsOfNames,
    domelem_Invoke,
    domelem_get_nodeName,
    domelem_get_nodeValue,
    domelem_put_nodeValue,
    domelem_get_nodeType,
    domelem_get_parentNode,
    domelem_get_childNodes,
    domelem_get_firstChild,
    domelem_get_lastChild,
    domelem_get_previousSibling,
    domelem_get_nextSibling,
    domelem_get_attributes,
    domelem_insertBefore,
    domelem_replaceChild,
    domelem_removeChild,
    domelem_appendChild,
    domelem_hasChildNodes,
    domelem_get_ownerDocument,
    domelem_cloneNode,
    domelem_get_nodeTypeString,
    domelem_get_text,
    domelem_put_text,
    domelem_get_specified,
    domelem_get_definition,
    domelem_get_nodeTypedValue,
    domelem_put_nodeTypedValue,
    domelem_get_dataType,
    domelem_put_dataType,
    domelem_get_xml,
    domelem_transformNode,
    domelem_selectNodes,
    domelem_selectSingleNode,
    domelem_get_parsed,
    domelem_get_namespaceURI,
    domelem_get_prefix,
    domelem_get_baseName,
    domelem_transformNodeToObject,
    domelem_get_tagName,
    domelem_getAttribute,
    domelem_setAttribute,
    domelem_removeAttribute,
    domelem_getAttributeNode,
    domelem_setAttributeNode,
    domelem_removeAttributeNode,
    domelem_getElementsByTagName,
    domelem_normalize,
};

static HRESULT domelem_get_qualified_item(const xmlNodePtr node, BSTR name, BSTR uri,
    IXMLDOMNode **item)
{
    xmlAttrPtr attr;
    xmlChar *nameA;
    xmlChar *href;

    TRACE("(%p)->(%s %s %p)\n", node, debugstr_w(name), debugstr_w(uri), item);

    if (!name || !item) return E_INVALIDARG;

    if (uri && *uri)
    {
        href = xmlchar_from_wchar(uri);
        if (!href) return E_OUTOFMEMORY;
    }
    else
        href = NULL;

    nameA = xmlchar_from_wchar(name);
    if (!nameA)
    {
        heap_free(href);
        return E_OUTOFMEMORY;
    }

    attr = xmlHasNsProp(node, nameA, href);

    heap_free(nameA);
    heap_free(href);

    if (!attr)
    {
        *item = NULL;
        return S_FALSE;
    }

    *item = create_node((xmlNodePtr)attr);

    return S_OK;
}

static HRESULT domelem_get_named_item(const xmlNodePtr node, BSTR name, IXMLDOMNode **item)
{
    xmlChar *nameA, *local, *prefix;
    BSTR uriW, localW;
    xmlNsPtr ns;
    HRESULT hr;

    TRACE("(%p)->(%s %p)\n", node, debugstr_w(name), item );

    nameA = xmlchar_from_wchar(name);
    local = xmlSplitQName2(nameA, &prefix);
    heap_free(nameA);

    if (!local)
        return domelem_get_qualified_item(node, name, NULL, item);

    /* try to get namespace uri for supplied qualified name */
    ns = xmlSearchNs(node->doc, node, prefix);

    xmlFree(prefix);

    if (!ns)
    {
        xmlFree(local);
        if (item) *item = NULL;
        return item ? S_FALSE : E_INVALIDARG;
    }

    uriW = bstr_from_xmlChar(ns->href);
    localW = bstr_from_xmlChar(local);
    xmlFree(local);

    TRACE("got qualified node %s, uri=%s\n", debugstr_w(localW), debugstr_w(uriW));

    hr = domelem_get_qualified_item(node, localW, uriW, item);

    SysFreeString(localW);
    SysFreeString(uriW);

    return hr;
}

static HRESULT domelem_set_named_item(xmlNodePtr node, IXMLDOMNode *newItem, IXMLDOMNode **namedItem)
{
    xmlNodePtr nodeNew;
    xmlnode *ThisNew;

    TRACE("(%p)->(%p %p)\n", node, newItem, namedItem );

    if(!newItem)
        return E_INVALIDARG;

    if(namedItem) *namedItem = NULL;

    /* Must be an Attribute */
    ThisNew = get_node_obj( newItem );
    if(!ThisNew) return E_FAIL;

    if(ThisNew->node->type != XML_ATTRIBUTE_NODE)
        return E_FAIL;

    if(!ThisNew->node->parent)
        if(xmldoc_remove_orphan(ThisNew->node->doc, ThisNew->node) != S_OK)
            WARN("%p is not an orphan of %p\n", ThisNew->node, ThisNew->node->doc);

    nodeNew = xmlAddChild(node, ThisNew->node);

    if(namedItem)
        *namedItem = create_node( nodeNew );
    return S_OK;
}

static HRESULT domelem_remove_qualified_item(xmlNodePtr node, BSTR name, BSTR uri, IXMLDOMNode **item)
{
    xmlChar *nameA, *href;
    xmlAttrPtr attr;

    TRACE("(%p)->(%s %s %p)\n", node, debugstr_w(name), debugstr_w(uri), item);

    if (!name) return E_INVALIDARG;

    if (uri && *uri)
    {
        href = xmlchar_from_wchar(uri);
        if (!href) return E_OUTOFMEMORY;
    }
    else
        href = NULL;

    nameA = xmlchar_from_wchar(name);
    if (!nameA)
    {
        heap_free(href);
        return E_OUTOFMEMORY;
    }

    attr = xmlHasNsProp(node, nameA, href);

    heap_free(nameA);
    heap_free(href);

    if (!attr)
    {
        if (item) *item = NULL;
        return S_FALSE;
    }

    if (item)
    {
        xmlUnlinkNode( (xmlNodePtr) attr );
        xmldoc_add_orphan( attr->doc, (xmlNodePtr) attr );
        *item = create_node( (xmlNodePtr) attr );
    }
    else
    {
        if (xmlRemoveProp(attr) == -1)
            ERR("xmlRemoveProp failed\n");
    }

    return S_OK;
}

static HRESULT domelem_remove_named_item(xmlNodePtr node, BSTR name, IXMLDOMNode **item)
{
    TRACE("(%p)->(%s %p)\n", node, debugstr_w(name), item);
    return domelem_remove_qualified_item(node, name, NULL, item);
}

static HRESULT domelem_get_item(const xmlNodePtr node, LONG index, IXMLDOMNode **item)
{
    xmlAttrPtr curr;
    LONG attrIndex;

    TRACE("(%p)->(%d %p)\n", node, index, item);

    *item = NULL;

    if (index < 0)
        return S_FALSE;

    curr = node->properties;

    for (attrIndex = 0; attrIndex < index; attrIndex++) {
        if (curr->next == NULL)
            return S_FALSE;
        else
            curr = curr->next;
    }

    *item = create_node( (xmlNodePtr) curr );

    return S_OK;
}

static HRESULT domelem_get_length(const xmlNodePtr node, LONG *length)
{
    xmlAttrPtr first;
    xmlAttrPtr curr;
    LONG attrCount;

    TRACE("(%p)->(%p)\n", node, length);

    if( !length )
        return E_INVALIDARG;

    first = node->properties;
    if (first == NULL) {
	*length = 0;
	return S_OK;
    }

    curr = first;
    attrCount = 1;
    while (curr->next) {
        attrCount++;
        curr = curr->next;
    }
    *length = attrCount;

    return S_OK;
}

static HRESULT domelem_next_node(const xmlNodePtr node, LONG *iter, IXMLDOMNode **nextNode)
{
    xmlAttrPtr curr;
    LONG i;

    TRACE("(%p)->(%d: %p)\n", node, *iter, nextNode);

    *nextNode = NULL;

    curr = node->properties;

    for (i = 0; i < *iter; i++) {
        if (curr->next == NULL)
            return S_FALSE;
        else
            curr = curr->next;
    }

    (*iter)++;
    *nextNode = create_node((xmlNodePtr)curr);

    return S_OK;
}

static const struct nodemap_funcs domelem_attr_map = {
    domelem_get_named_item,
    domelem_set_named_item,
    domelem_remove_named_item,
    domelem_get_item,
    domelem_get_length,
    domelem_get_qualified_item,
    domelem_remove_qualified_item,
    domelem_next_node
};

static const tid_t domelem_iface_tids[] = {
    IXMLDOMElement_tid,
    0
};

static dispex_static_data_t domelem_dispex = {
    NULL,
    IXMLDOMElement_tid,
    NULL,
    domelem_iface_tids
};

IUnknown* create_element( xmlNodePtr element )
{
    domelem *This;

    This = heap_alloc( sizeof *This );
    if ( !This )
        return NULL;

    This->IXMLDOMElement_iface.lpVtbl = &domelem_vtbl;
    This->ref = 1;

    init_xmlnode(&This->node, element, (IXMLDOMNode*)&This->IXMLDOMElement_iface, &domelem_dispex);

    return (IUnknown*)&This->IXMLDOMElement_iface;
}

#endif
