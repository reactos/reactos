/*
 * Copyright 2006 Jacek Caban for CodeWeavers
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


#include <stdarg.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "winreg.h"
#include "ole2.h"
#include "shlwapi.h"

#include "wine/debug.h"
#include "wine/unicode.h"

#include "mshtml_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(mshtml);

static IHTMLElementCollection *HTMLElementCollection_Create(IUnknown*,HTMLElement**,DWORD);

typedef struct {
    HTMLElement **buf;
    DWORD len;
    DWORD size;
} elem_vector;

static void elem_vector_add(elem_vector *buf, HTMLElement *elem)
{
    if(buf->len == buf->size) {
        buf->size <<= 1;
        buf->buf = heap_realloc(buf->buf, buf->size*sizeof(HTMLElement**));
    }

    buf->buf[buf->len++] = elem;
}

static void elem_vector_normalize(elem_vector *buf)
{
    if(!buf->len) {
        heap_free(buf->buf);
        buf->buf = NULL;
    }else if(buf->size > buf->len) {
        buf->buf = heap_realloc(buf->buf, buf->len*sizeof(HTMLElement**));
    }

    buf->size = buf->len;
}

static BOOL is_elem_node(nsIDOMNode *node)
{
    PRUint16 type=0;

    nsIDOMNode_GetNodeType(node, &type);

    return type == ELEMENT_NODE || type == COMMENT_NODE;
}

#define HTMLELEM_THIS(iface) DEFINE_THIS(HTMLElement, HTMLElement, iface)

#define HTMLELEM_NODE_THIS(iface) DEFINE_THIS2(HTMLElement, node, iface)

static HRESULT WINAPI HTMLElement_QueryInterface(IHTMLElement *iface,
                                                 REFIID riid, void **ppv)
{
    HTMLElement *This = HTMLELEM_THIS(iface);

    return IHTMLDOMNode_QueryInterface(HTMLDOMNODE(&This->node), riid, ppv);
}

static ULONG WINAPI HTMLElement_AddRef(IHTMLElement *iface)
{
    HTMLElement *This = HTMLELEM_THIS(iface);

    return IHTMLDOMNode_AddRef(HTMLDOMNODE(&This->node));
}

static ULONG WINAPI HTMLElement_Release(IHTMLElement *iface)
{
    HTMLElement *This = HTMLELEM_THIS(iface);

    return IHTMLDOMNode_Release(HTMLDOMNODE(&This->node));
}

static HRESULT WINAPI HTMLElement_GetTypeInfoCount(IHTMLElement *iface, UINT *pctinfo)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    return IDispatchEx_GetTypeInfoCount(DISPATCHEX(&This->node.dispex), pctinfo);
}

static HRESULT WINAPI HTMLElement_GetTypeInfo(IHTMLElement *iface, UINT iTInfo,
                                              LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    return IDispatchEx_GetTypeInfo(DISPATCHEX(&This->node.dispex), iTInfo, lcid, ppTInfo);
}

static HRESULT WINAPI HTMLElement_GetIDsOfNames(IHTMLElement *iface, REFIID riid,
                                                LPOLESTR *rgszNames, UINT cNames,
                                                LCID lcid, DISPID *rgDispId)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    return IDispatchEx_GetIDsOfNames(DISPATCHEX(&This->node.dispex), riid, rgszNames, cNames, lcid, rgDispId);
}

static HRESULT WINAPI HTMLElement_Invoke(IHTMLElement *iface, DISPID dispIdMember,
                            REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
                            VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    return IDispatchEx_Invoke(DISPATCHEX(&This->node.dispex), dispIdMember, riid, lcid,
            wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI HTMLElement_setAttribute(IHTMLElement *iface, BSTR strAttributeName,
                                               VARIANT AttributeValue, LONG lFlags)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    nsAString attr_str;
    nsAString value_str;
    nsresult nsres;
    HRESULT hres;
    VARIANT AttributeValueChanged;

    WARN("(%p)->(%s . %08x)\n", This, debugstr_w(strAttributeName), lFlags);

    if(!This->nselem) {
        FIXME("NULL nselem\n");
        return E_NOTIMPL;
    }

    VariantInit(&AttributeValueChanged);

    hres = VariantChangeType(&AttributeValueChanged, &AttributeValue, 0, VT_BSTR);
    if (FAILED(hres)) {
        WARN("couldn't convert input attribute value %d to VT_BSTR\n", V_VT(&AttributeValue));
        return hres;
    }

    nsAString_Init(&attr_str, strAttributeName);
    nsAString_Init(&value_str, V_BSTR(&AttributeValueChanged));

    TRACE("setting %s to %s\n", debugstr_w(strAttributeName),
        debugstr_w(V_BSTR(&AttributeValueChanged)));

    nsres = nsIDOMHTMLElement_SetAttribute(This->nselem, &attr_str, &value_str);
    nsAString_Finish(&attr_str);
    nsAString_Finish(&value_str);

    if(NS_SUCCEEDED(nsres)) {
        hres = S_OK;
    }else {
        ERR("SetAttribute failed: %08x\n", nsres);
        hres = E_FAIL;
    }

    return hres;
}

static HRESULT WINAPI HTMLElement_getAttribute(IHTMLElement *iface, BSTR strAttributeName,
                                               LONG lFlags, VARIANT *AttributeValue)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    nsAString attr_str;
    nsAString value_str;
    const PRUnichar *value;
    nsresult nsres;
    HRESULT hres = S_OK;

    WARN("(%p)->(%s %08x %p)\n", This, debugstr_w(strAttributeName), lFlags, AttributeValue);

    if(!This->nselem) {
        FIXME("NULL nselem\n");
        V_VT(AttributeValue) = VT_NULL;
        return S_OK;
    }

    V_VT(AttributeValue) = VT_NULL;

    nsAString_Init(&attr_str, strAttributeName);
    nsAString_Init(&value_str, NULL);

    nsres = nsIDOMHTMLElement_GetAttribute(This->nselem, &attr_str, &value_str);
    nsAString_Finish(&attr_str);

    if(NS_SUCCEEDED(nsres)) {
        static const WCHAR wszSRC[] = {'s','r','c',0};
        nsAString_GetData(&value_str, &value);
        if(!strcmpiW(strAttributeName, wszSRC))
        {
            WCHAR buffer[256];
            DWORD len;
            BSTR bstrBaseUrl;
            hres = IHTMLDocument2_get_URL(HTMLDOC(This->node.doc), &bstrBaseUrl);
            if(SUCCEEDED(hres)) {
                hres = CoInternetCombineUrl(bstrBaseUrl, value,
                                            URL_ESCAPE_SPACES_ONLY|URL_DONT_ESCAPE_EXTRA_INFO,
                                            buffer, sizeof(buffer)/sizeof(WCHAR), &len, 0);
                SysFreeString(bstrBaseUrl);
                if(SUCCEEDED(hres)) {
                    V_VT(AttributeValue) = VT_BSTR;
                    V_BSTR(AttributeValue) = SysAllocString(buffer);
                    TRACE("attr_value=%s\n", debugstr_w(V_BSTR(AttributeValue)));
                }
            }
        }else if(*value) {
            V_VT(AttributeValue) = VT_BSTR;
            V_BSTR(AttributeValue) = SysAllocString(value);
            TRACE("attr_value=%s\n", debugstr_w(V_BSTR(AttributeValue)));
        }
    }else {
        ERR("GetAttribute failed: %08x\n", nsres);
        hres = E_FAIL;
    }

    nsAString_Finish(&value_str);

    return hres;
}

static HRESULT WINAPI HTMLElement_removeAttribute(IHTMLElement *iface, BSTR strAttributeName,
                                                  LONG lFlags, VARIANT_BOOL *pfSuccess)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_put_className(IHTMLElement *iface, BSTR v)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    nsAString classname_str;
    nsresult nsres;

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    if(!This->nselem) {
        FIXME("NULL nselem\n");
        return E_NOTIMPL;
    }

    nsAString_Init(&classname_str, v);
    nsres = nsIDOMHTMLElement_SetClassName(This->nselem, &classname_str);
    nsAString_Finish(&classname_str);
    if(NS_FAILED(nsres))
        ERR("SetClassName failed: %08x\n", nsres);

    return S_OK;
}

static HRESULT WINAPI HTMLElement_get_className(IHTMLElement *iface, BSTR *p)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    nsAString class_str;
    nsresult nsres;
    HRESULT hres = S_OK;

    TRACE("(%p)->(%p)\n", This, p);

    if(!This->nselem) {
        FIXME("NULL nselem\n");
        return E_NOTIMPL;
    }

    nsAString_Init(&class_str, NULL);
    nsres = nsIDOMHTMLElement_GetClassName(This->nselem, &class_str);

    if(NS_SUCCEEDED(nsres)) {
        const PRUnichar *class;
        nsAString_GetData(&class_str, &class);
        *p = *class ? SysAllocString(class) : NULL;
    }else {
        ERR("GetClassName failed: %08x\n", nsres);
        hres = E_FAIL;
    }

    nsAString_Finish(&class_str);

    TRACE("className=%s\n", debugstr_w(*p));
    return hres;
}

static HRESULT WINAPI HTMLElement_put_id(IHTMLElement *iface, BSTR v)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    nsAString id_str;
    nsresult nsres;

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    if(!This->nselem) {
        FIXME("nselem == NULL\n");
        return S_OK;
    }

    nsAString_Init(&id_str, v);
    nsres = nsIDOMHTMLElement_SetId(This->nselem, &id_str);
    nsAString_Finish(&id_str);
    if(NS_FAILED(nsres))
        ERR("SetId failed: %08x\n", nsres);

    return S_OK;
}

static HRESULT WINAPI HTMLElement_get_id(IHTMLElement *iface, BSTR *p)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    const PRUnichar *id;
    nsAString id_str;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    *p = NULL;

    if(!This->nselem)
        return S_OK;

    nsAString_Init(&id_str, NULL);
    nsres = nsIDOMHTMLElement_GetId(This->nselem, &id_str);
    nsAString_GetData(&id_str, &id);

    if(NS_FAILED(nsres))
        ERR("GetId failed: %08x\n", nsres);
    else if(*id)
        *p = SysAllocString(id);

    nsAString_Finish(&id_str);
    return S_OK;
}

static HRESULT WINAPI HTMLElement_get_tagName(IHTMLElement *iface, BSTR *p)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    const PRUnichar *tag;
    nsAString tag_str;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    if(!This->nselem) {
        static const WCHAR comment_tagW[] = {'!',0};

        WARN("NULL nselem, assuming comment\n");

        *p = SysAllocString(comment_tagW);
        return S_OK;
    }

    nsAString_Init(&tag_str, NULL);
    nsres = nsIDOMHTMLElement_GetTagName(This->nselem, &tag_str);
    if(NS_SUCCEEDED(nsres)) {
        nsAString_GetData(&tag_str, &tag);
        *p = SysAllocString(tag);
    }else {
        ERR("GetTagName failed: %08x\n", nsres);
        *p = NULL;
    }
    nsAString_Finish(&tag_str);

    return S_OK;
}

static HRESULT WINAPI HTMLElement_get_parentElement(IHTMLElement *iface, IHTMLElement **p)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    IHTMLDOMNode *node;
    HRESULT hres;

    TRACE("(%p)->(%p)\n", This, p);

    hres = IHTMLDOMNode_get_parentNode(HTMLDOMNODE(&This->node), &node);
    if(FAILED(hres))
        return hres;

    hres = IHTMLDOMNode_QueryInterface(node, &IID_IHTMLElement, (void**)p);
    IHTMLDOMNode_Release(node);
    if(FAILED(hres))
        *p = NULL;

    return S_OK;
}

static HRESULT WINAPI HTMLElement_get_style(IHTMLElement *iface, IHTMLStyle **p)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    nsIDOMElementCSSInlineStyle *nselemstyle;
    nsIDOMCSSStyleDeclaration *nsstyle;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    if(!This->nselem) {
        FIXME("NULL nselem\n");
        return E_NOTIMPL;
    }

    nsres = nsIDOMHTMLElement_QueryInterface(This->nselem, &IID_nsIDOMElementCSSInlineStyle,
                                             (void**)&nselemstyle);
    if(NS_FAILED(nsres)) {
        ERR("Coud not get nsIDOMCSSStyleDeclaration interface: %08x\n", nsres);
        return E_FAIL;
    }

    nsres = nsIDOMElementCSSInlineStyle_GetStyle(nselemstyle, &nsstyle);
    nsIDOMElementCSSInlineStyle_Release(nselemstyle);
    if(NS_FAILED(nsres)) {
        ERR("GetStyle failed: %08x\n", nsres);
        return E_FAIL;
    }

    /* FIXME: Store style instead of creating a new instance in each call */
    *p = HTMLStyle_Create(nsstyle);

    nsIDOMCSSStyleDeclaration_Release(nsstyle);
    return S_OK;
}

static HRESULT WINAPI HTMLElement_put_onhelp(IHTMLElement *iface, VARIANT v)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_get_onhelp(IHTMLElement *iface, VARIANT *p)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_put_onclick(IHTMLElement *iface, VARIANT v)
{
    HTMLElement *This = HTMLELEM_THIS(iface);

    TRACE("(%p)->()\n", This);

    return set_node_event(&This->node, EVENTID_CLICK, &v);
}

static HRESULT WINAPI HTMLElement_get_onclick(IHTMLElement *iface, VARIANT *p)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_put_ondblclick(IHTMLElement *iface, VARIANT v)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_get_ondblclick(IHTMLElement *iface, VARIANT *p)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_put_onkeydown(IHTMLElement *iface, VARIANT v)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_get_onkeydown(IHTMLElement *iface, VARIANT *p)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_put_onkeyup(IHTMLElement *iface, VARIANT v)
{
    HTMLElement *This = HTMLELEM_THIS(iface);

    TRACE("(%p)->()\n", This);

    return set_node_event(&This->node, EVENTID_KEYUP, &v);
}

static HRESULT WINAPI HTMLElement_get_onkeyup(IHTMLElement *iface, VARIANT *p)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_put_onkeypress(IHTMLElement *iface, VARIANT v)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_get_onkeypress(IHTMLElement *iface, VARIANT *p)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_put_onmouseout(IHTMLElement *iface, VARIANT v)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_get_onmouseout(IHTMLElement *iface, VARIANT *p)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_put_onmouseover(IHTMLElement *iface, VARIANT v)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_get_onmouseover(IHTMLElement *iface, VARIANT *p)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_put_onmousemove(IHTMLElement *iface, VARIANT v)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_get_onmousemove(IHTMLElement *iface, VARIANT *p)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_put_onmousedown(IHTMLElement *iface, VARIANT v)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_get_onmousedown(IHTMLElement *iface, VARIANT *p)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_put_onmouseup(IHTMLElement *iface, VARIANT v)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_get_onmouseup(IHTMLElement *iface, VARIANT *p)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_get_document(IHTMLElement *iface, IDispatch **p)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_put_title(IHTMLElement *iface, BSTR v)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    nsAString title_str;
    nsresult nsres;

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    nsAString_Init(&title_str, v);
    nsres = nsIDOMHTMLElement_SetTitle(This->nselem, &title_str);
    nsAString_Finish(&title_str);
    if(NS_FAILED(nsres))
        ERR("SetTitle failed: %08x\n", nsres);

    return S_OK;
}

static HRESULT WINAPI HTMLElement_get_title(IHTMLElement *iface, BSTR *p)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    nsAString title_str;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsAString_Init(&title_str, NULL);
    nsres = nsIDOMHTMLElement_GetTitle(This->nselem, &title_str);
    if(NS_SUCCEEDED(nsres)) {
        const PRUnichar *title;

        nsAString_GetData(&title_str, &title);
        *p = *title ? SysAllocString(title) : NULL;
    }else {
        ERR("GetTitle failed: %08x\n", nsres);
        return E_FAIL;
    }

    return S_OK;
}

static HRESULT WINAPI HTMLElement_put_language(IHTMLElement *iface, BSTR v)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_get_language(IHTMLElement *iface, BSTR *p)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_put_onselectstart(IHTMLElement *iface, VARIANT v)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_get_onselectstart(IHTMLElement *iface, VARIANT *p)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_scrollIntoView(IHTMLElement *iface, VARIANT varargStart)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_contains(IHTMLElement *iface, IHTMLElement *pChild,
                                           VARIANT_BOOL *pfResult)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    FIXME("(%p)->(%p %p)\n", This, pChild, pfResult);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_get_sourceIndex(IHTMLElement *iface, long *p)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_get_recordNumber(IHTMLElement *iface, VARIANT *p)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_put_lang(IHTMLElement *iface, BSTR v)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_get_lang(IHTMLElement *iface, BSTR *p)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_get_offsetLeft(IHTMLElement *iface, long *p)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_get_offsetTop(IHTMLElement *iface, long *p)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    nsIDOMNSHTMLElement *nselem;
    PRInt32 top = 0;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsres = nsIDOMHTMLElement_QueryInterface(This->nselem, &IID_nsIDOMNSHTMLElement, (void**)&nselem);
    if(NS_FAILED(nsres)) {
        ERR("Could not get nsIDOMNSHTMLElement: %08x\n", nsres);
        return E_FAIL;
    }

    nsres = nsIDOMNSHTMLElement_GetOffsetTop(nselem, &top);
    nsIDOMNSHTMLElement_Release(nselem);
    if(NS_FAILED(nsres)) {
        ERR("GetOffsetTop failed: %08x\n", nsres);
        return E_FAIL;
    }

    *p = top;
    return S_OK;
}

static HRESULT WINAPI HTMLElement_get_offsetWidth(IHTMLElement *iface, long *p)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_get_offsetHeight(IHTMLElement *iface, long *p)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    nsIDOMNSHTMLElement *nselem;
    PRInt32 offset = 0;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsres = nsIDOMHTMLElement_QueryInterface(This->nselem, &IID_nsIDOMNSHTMLElement, (void**)&nselem);
    if(NS_FAILED(nsres)) {
        ERR("Could not get nsIDOMNSHTMLElement: %08x\n", nsres);
        return E_FAIL;
    }

    nsres = nsIDOMNSHTMLElement_GetOffsetHeight(nselem, &offset);
    nsIDOMNSHTMLElement_Release(nselem);
    if(NS_FAILED(nsres)) {
        ERR("GetOffsetHeight failed: %08x\n", nsres);
        return E_FAIL;
    }

    *p = offset;
    return S_OK;
}

static HRESULT WINAPI HTMLElement_get_offsetParent(IHTMLElement *iface, IHTMLElement **p)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_put_innerHTML(IHTMLElement *iface, BSTR v)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    nsIDOMNSHTMLElement *nselem;
    nsAString html_str;
    nsresult nsres;

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    if(!This->nselem) {
        FIXME("NULL nselem\n");
        return E_NOTIMPL;
    }

    nsres = nsIDOMHTMLElement_QueryInterface(This->nselem, &IID_nsIDOMNSHTMLElement, (void**)&nselem);
    if(NS_FAILED(nsres)) {
        ERR("Could not get nsIDOMNSHTMLElement: %08x\n", nsres);
        return E_FAIL;
    }

    nsAString_Init(&html_str, v);
    nsres = nsIDOMNSHTMLElement_SetInnerHTML(nselem, &html_str);
    nsAString_Finish(&html_str);

    if(NS_FAILED(nsres)) {
        FIXME("SetInnerHtml failed %08x\n", nsres);
        return E_FAIL;
    }

    return S_OK;
}

static HRESULT WINAPI HTMLElement_get_innerHTML(IHTMLElement *iface, BSTR *p)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_put_innerText(IHTMLElement *iface, BSTR v)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_get_innerText(IHTMLElement *iface, BSTR *p)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_put_outerHTML(IHTMLElement *iface, BSTR v)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_get_outerHTML(IHTMLElement *iface, BSTR *p)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_put_outerText(IHTMLElement *iface, BSTR v)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_get_outerText(IHTMLElement *iface, BSTR *p)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT HTMLElement_InsertAdjacentNode(HTMLElement *This, BSTR where, nsIDOMNode *nsnode)
{
    static const WCHAR wszBeforeBegin[] = {'b','e','f','o','r','e','B','e','g','i','n',0};
    static const WCHAR wszAfterBegin[] = {'a','f','t','e','r','B','e','g','i','n',0};
    static const WCHAR wszBeforeEnd[] = {'b','e','f','o','r','e','E','n','d',0};
    static const WCHAR wszAfterEnd[] = {'a','f','t','e','r','E','n','d',0};
    nsresult nsres;

    if(!This->nselem) {
        FIXME("NULL nselem\n");
        return E_NOTIMPL;
    }

    if (!strcmpiW(where, wszBeforeBegin))
    {
        nsIDOMNode *unused;
        nsIDOMNode *parent;
        nsres = nsIDOMNode_GetParentNode(This->nselem, &parent);
        if (!parent) return E_INVALIDARG;
        nsres = nsIDOMNode_InsertBefore(parent, nsnode,
                                        (nsIDOMNode *)This->nselem, &unused);
        if (unused) nsIDOMNode_Release(unused);
        nsIDOMNode_Release(parent);
    }
    else if (!strcmpiW(where, wszAfterBegin))
    {
        nsIDOMNode *unused;
        nsIDOMNode *first_child;
        nsIDOMNode_GetFirstChild(This->nselem, &first_child);
        nsres = nsIDOMNode_InsertBefore(This->nselem, nsnode, first_child, &unused);
        if (unused) nsIDOMNode_Release(unused);
        if (first_child) nsIDOMNode_Release(first_child);
    }
    else if (!strcmpiW(where, wszBeforeEnd))
    {
        nsIDOMNode *unused;
        nsres = nsIDOMNode_AppendChild(This->nselem, nsnode, &unused);
        if (unused) nsIDOMNode_Release(unused);
    }
    else if (!strcmpiW(where, wszAfterEnd))
    {
        nsIDOMNode *unused;
        nsIDOMNode *next_sibling;
        nsIDOMNode *parent;
        nsIDOMNode_GetParentNode(This->nselem, &parent);
        if (!parent) return E_INVALIDARG;

        nsIDOMNode_GetNextSibling(This->nselem, &next_sibling);
        if (next_sibling)
        {
            nsres = nsIDOMNode_InsertBefore(parent, nsnode, next_sibling, &unused);
            nsIDOMNode_Release(next_sibling);
        }
        else
            nsres = nsIDOMNode_AppendChild(parent, nsnode, &unused);
        nsIDOMNode_Release(parent);
        if (unused) nsIDOMNode_Release(unused);
    }
    else
    {
        ERR("invalid where: %s\n", debugstr_w(where));
        return E_INVALIDARG;
    }

    if (NS_FAILED(nsres))
        return E_FAIL;
    else
        return S_OK;
}

static HRESULT WINAPI HTMLElement_insertAdjacentHTML(IHTMLElement *iface, BSTR where,
                                                     BSTR html)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    nsresult nsres;
    nsIDOMDocument *nsdoc;
    nsIDOMDocumentRange *nsdocrange;
    nsIDOMRange *range;
    nsIDOMNSRange *nsrange;
    nsIDOMNode *nsnode;
    nsAString ns_html;
    HRESULT hr;

    TRACE("(%p)->(%s %s)\n", This, debugstr_w(where), debugstr_w(html));

    nsres = nsIWebNavigation_GetDocument(This->node.doc->nscontainer->navigation, &nsdoc);
    if(NS_FAILED(nsres))
    {
        ERR("GetDocument failed: %08x\n", nsres);
        return E_FAIL;
    }

    nsres = nsIDOMDocument_QueryInterface(nsdoc, &IID_nsIDOMDocumentRange, (void **)&nsdocrange);
    nsIDOMDocument_Release(nsdoc);
    if(NS_FAILED(nsres))
    {
        ERR("getting nsIDOMDocumentRange failed: %08x\n", nsres);
        return E_FAIL;
    }
    nsres = nsIDOMDocumentRange_CreateRange(nsdocrange, &range);
    nsIDOMDocumentRange_Release(nsdocrange);
    if(NS_FAILED(nsres))
    {
        ERR("CreateRange failed: %08x\n", nsres);
        return E_FAIL;
    }

    nsIDOMRange_SetStartBefore(range, (nsIDOMNode *)This->nselem);

    nsIDOMRange_QueryInterface(range, &IID_nsIDOMNSRange, (void **)&nsrange);
    nsIDOMRange_Release(range);
    if(NS_FAILED(nsres))
    {
        ERR("getting nsIDOMNSRange failed: %08x\n", nsres);
        return E_FAIL;
    }

    nsAString_Init(&ns_html, html);

    nsres = nsIDOMNSRange_CreateContextualFragment(nsrange, &ns_html, (nsIDOMDocumentFragment **)&nsnode);
    nsIDOMNSRange_Release(nsrange);
    nsAString_Finish(&ns_html);

    if(NS_FAILED(nsres) || !nsnode)
    {
        ERR("CreateTextNode failed: %08x\n", nsres);
        return E_FAIL;
    }

    hr = HTMLElement_InsertAdjacentNode(This, where, nsnode);
    nsIDOMNode_Release(nsnode);

    return hr;
}

static HRESULT WINAPI HTMLElement_insertAdjacentText(IHTMLElement *iface, BSTR where,
                                                     BSTR text)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    nsresult nsres;
    nsIDOMDocument *nsdoc;
    nsIDOMNode *nsnode;
    nsAString ns_text;
    HRESULT hr;

    TRACE("(%p)->(%s %s)\n", This, debugstr_w(where), debugstr_w(text));

    nsres = nsIWebNavigation_GetDocument(This->node.doc->nscontainer->navigation, &nsdoc);
    if(NS_FAILED(nsres) || !nsdoc)
    {
        ERR("GetDocument failed: %08x\n", nsres);
        return E_FAIL;
    }

    nsAString_Init(&ns_text, text);

    nsres = nsIDOMDocument_CreateTextNode(nsdoc, &ns_text, (nsIDOMText **)&nsnode);
    nsIDOMDocument_Release(nsdoc);
    nsAString_Finish(&ns_text);

    if(NS_FAILED(nsres) || !nsnode)
    {
        ERR("CreateTextNode failed: %08x\n", nsres);
        return E_FAIL;
    }

    hr = HTMLElement_InsertAdjacentNode(This, where, nsnode);
    nsIDOMNode_Release(nsnode);

    return hr;
}

static HRESULT WINAPI HTMLElement_get_parentTextEdit(IHTMLElement *iface, IHTMLElement **p)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_get_isTextEdit(IHTMLElement *iface, VARIANT_BOOL *p)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_click(IHTMLElement *iface)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_get_filters(IHTMLElement *iface,
                                              IHTMLFiltersCollection **p)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_put_ondragstart(IHTMLElement *iface, VARIANT v)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_get_ondragstart(IHTMLElement *iface, VARIANT *p)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_toString(IHTMLElement *iface, BSTR *String)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    FIXME("(%p)->(%p)\n", This, String);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_put_onbeforeupdate(IHTMLElement *iface, VARIANT v)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_get_onbeforeupdate(IHTMLElement *iface, VARIANT *p)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_put_onafterupdate(IHTMLElement *iface, VARIANT v)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_get_onafterupdate(IHTMLElement *iface, VARIANT *p)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_put_onerrorupdate(IHTMLElement *iface, VARIANT v)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_get_onerrorupdate(IHTMLElement *iface, VARIANT *p)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_put_onrowexit(IHTMLElement *iface, VARIANT v)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_get_onrowexit(IHTMLElement *iface, VARIANT *p)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_put_onrowenter(IHTMLElement *iface, VARIANT v)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_get_onrowenter(IHTMLElement *iface, VARIANT *p)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_put_ondatasetchanged(IHTMLElement *iface, VARIANT v)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_get_ondatasetchanged(IHTMLElement *iface, VARIANT *p)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_put_ondataavailable(IHTMLElement *iface, VARIANT v)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_get_ondataavailable(IHTMLElement *iface, VARIANT *p)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_put_ondatasetcomplete(IHTMLElement *iface, VARIANT v)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_get_ondatasetcomplete(IHTMLElement *iface, VARIANT *p)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_put_onfilterchange(IHTMLElement *iface, VARIANT v)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_get_onfilterchange(IHTMLElement *iface, VARIANT *p)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static void create_child_list(HTMLDocument *doc, HTMLElement *elem, elem_vector *buf)
{
    nsIDOMNodeList *nsnode_list;
    nsIDOMNode *iter;
    PRUint32 list_len = 0, i;
    nsresult nsres;

    nsres = nsIDOMNode_GetChildNodes(elem->node.nsnode, &nsnode_list);
    if(NS_FAILED(nsres)) {
        ERR("GetChildNodes failed: %08x\n", nsres);
        return;
    }

    nsIDOMNodeList_GetLength(nsnode_list, &list_len);
    if(!list_len)
        return;

    buf->size = list_len;
    buf->buf = heap_alloc(buf->size*sizeof(HTMLElement**));

    for(i=0; i<list_len; i++) {
        nsres = nsIDOMNodeList_Item(nsnode_list, i, &iter);
        if(NS_FAILED(nsres)) {
            ERR("Item failed: %08x\n", nsres);
            continue;
        }

        if(is_elem_node(iter))
            elem_vector_add(buf, HTMLELEM_NODE_THIS(get_node(doc, iter, TRUE)));
    }
}

static HRESULT WINAPI HTMLElement_get_children(IHTMLElement *iface, IDispatch **p)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    elem_vector buf = {NULL, 0, 0};

    TRACE("(%p)->(%p)\n", This, p);

    create_child_list(This->node.doc, This, &buf);

    *p = (IDispatch*)HTMLElementCollection_Create((IUnknown*)HTMLELEM(This), buf.buf, buf.len);
    return S_OK;
}

static void create_all_list(HTMLDocument *doc, HTMLDOMNode *elem, elem_vector *buf)
{
    nsIDOMNodeList *nsnode_list;
    nsIDOMNode *iter;
    PRUint32 list_len = 0, i;
    nsresult nsres;

    nsres = nsIDOMNode_GetChildNodes(elem->nsnode, &nsnode_list);
    if(NS_FAILED(nsres)) {
        ERR("GetChildNodes failed: %08x\n", nsres);
        return;
    }

    nsIDOMNodeList_GetLength(nsnode_list, &list_len);
    if(!list_len)
        return;

    for(i=0; i<list_len; i++) {
        nsres = nsIDOMNodeList_Item(nsnode_list, i, &iter);
        if(NS_FAILED(nsres)) {
            ERR("Item failed: %08x\n", nsres);
            continue;
        }

        if(is_elem_node(iter)) {
            HTMLDOMNode *node = get_node(doc, iter, TRUE);

            elem_vector_add(buf, HTMLELEM_NODE_THIS(node));
            create_all_list(doc, node, buf);
        }
    }
}

static HRESULT WINAPI HTMLElement_get_all(IHTMLElement *iface, IDispatch **p)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    elem_vector buf = {NULL, 0, 8};

    TRACE("(%p)->(%p)\n", This, p);

    buf.buf = heap_alloc(buf.size*sizeof(HTMLElement**));

    create_all_list(This->node.doc, &This->node, &buf);
    elem_vector_normalize(&buf);

    *p = (IDispatch*)HTMLElementCollection_Create((IUnknown*)HTMLELEM(This), buf.buf, buf.len);
    return S_OK;
}

#undef HTMLELEM_THIS

static const IHTMLElementVtbl HTMLElementVtbl = {
    HTMLElement_QueryInterface,
    HTMLElement_AddRef,
    HTMLElement_Release,
    HTMLElement_GetTypeInfoCount,
    HTMLElement_GetTypeInfo,
    HTMLElement_GetIDsOfNames,
    HTMLElement_Invoke,
    HTMLElement_setAttribute,
    HTMLElement_getAttribute,
    HTMLElement_removeAttribute,
    HTMLElement_put_className,
    HTMLElement_get_className,
    HTMLElement_put_id,
    HTMLElement_get_id,
    HTMLElement_get_tagName,
    HTMLElement_get_parentElement,
    HTMLElement_get_style,
    HTMLElement_put_onhelp,
    HTMLElement_get_onhelp,
    HTMLElement_put_onclick,
    HTMLElement_get_onclick,
    HTMLElement_put_ondblclick,
    HTMLElement_get_ondblclick,
    HTMLElement_put_onkeydown,
    HTMLElement_get_onkeydown,
    HTMLElement_put_onkeyup,
    HTMLElement_get_onkeyup,
    HTMLElement_put_onkeypress,
    HTMLElement_get_onkeypress,
    HTMLElement_put_onmouseout,
    HTMLElement_get_onmouseout,
    HTMLElement_put_onmouseover,
    HTMLElement_get_onmouseover,
    HTMLElement_put_onmousemove,
    HTMLElement_get_onmousemove,
    HTMLElement_put_onmousedown,
    HTMLElement_get_onmousedown,
    HTMLElement_put_onmouseup,
    HTMLElement_get_onmouseup,
    HTMLElement_get_document,
    HTMLElement_put_title,
    HTMLElement_get_title,
    HTMLElement_put_language,
    HTMLElement_get_language,
    HTMLElement_put_onselectstart,
    HTMLElement_get_onselectstart,
    HTMLElement_scrollIntoView,
    HTMLElement_contains,
    HTMLElement_get_sourceIndex,
    HTMLElement_get_recordNumber,
    HTMLElement_put_lang,
    HTMLElement_get_lang,
    HTMLElement_get_offsetLeft,
    HTMLElement_get_offsetTop,
    HTMLElement_get_offsetWidth,
    HTMLElement_get_offsetHeight,
    HTMLElement_get_offsetParent,
    HTMLElement_put_innerHTML,
    HTMLElement_get_innerHTML,
    HTMLElement_put_innerText,
    HTMLElement_get_innerText,
    HTMLElement_put_outerHTML,
    HTMLElement_get_outerHTML,
    HTMLElement_put_outerText,
    HTMLElement_get_outerText,
    HTMLElement_insertAdjacentHTML,
    HTMLElement_insertAdjacentText,
    HTMLElement_get_parentTextEdit,
    HTMLElement_get_isTextEdit,
    HTMLElement_click,
    HTMLElement_get_filters,
    HTMLElement_put_ondragstart,
    HTMLElement_get_ondragstart,
    HTMLElement_toString,
    HTMLElement_put_onbeforeupdate,
    HTMLElement_get_onbeforeupdate,
    HTMLElement_put_onafterupdate,
    HTMLElement_get_onafterupdate,
    HTMLElement_put_onerrorupdate,
    HTMLElement_get_onerrorupdate,
    HTMLElement_put_onrowexit,
    HTMLElement_get_onrowexit,
    HTMLElement_put_onrowenter,
    HTMLElement_get_onrowenter,
    HTMLElement_put_ondatasetchanged,
    HTMLElement_get_ondatasetchanged,
    HTMLElement_put_ondataavailable,
    HTMLElement_get_ondataavailable,
    HTMLElement_put_ondatasetcomplete,
    HTMLElement_get_ondatasetcomplete,
    HTMLElement_put_onfilterchange,
    HTMLElement_get_onfilterchange,
    HTMLElement_get_children,
    HTMLElement_get_all
};

HRESULT HTMLElement_QI(HTMLDOMNode *iface, REFIID riid, void **ppv)
{
    HTMLElement *This = HTMLELEM_NODE_THIS(iface);

    *ppv =  NULL;

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = HTMLELEM(This);
    }else if(IsEqualGUID(&IID_IDispatch, riid)) {
        TRACE("(%p)->(IID_IDispatch %p)\n", This, ppv);
        *ppv = HTMLELEM(This);
    }else if(IsEqualGUID(&IID_IHTMLElement, riid)) {
        TRACE("(%p)->(IID_IHTMLElement %p)\n", This, ppv);
        *ppv = HTMLELEM(This);
    }else if(IsEqualGUID(&IID_IHTMLElement2, riid)) {
        TRACE("(%p)->(IID_IHTMLElement2 %p)\n", This, ppv);
        *ppv = HTMLELEM2(This);
    }else if(IsEqualGUID(&IID_IConnectionPointContainer, riid)) {
        TRACE("(%p)->(IID_IConnectionPointContainer %p)\n", This, ppv);
        *ppv = CONPTCONT(&This->cp_container);
    }

    if(*ppv) {
        IHTMLElement_AddRef(HTMLELEM(This));
        return S_OK;
    }

    return HTMLDOMNode_QI(&This->node, riid, ppv);
}

void HTMLElement_destructor(HTMLDOMNode *iface)
{
    HTMLElement *This = HTMLELEM_NODE_THIS(iface);

    ConnectionPointContainer_Destroy(&This->cp_container);

    if(This->nselem)
        nsIDOMHTMLElement_Release(This->nselem);

    HTMLDOMNode_destructor(&This->node);
}

static const NodeImplVtbl HTMLElementImplVtbl = {
    HTMLElement_QI,
    HTMLElement_destructor
};

static const tid_t HTMLElement_iface_tids[] = {
    IHTMLDOMNode_tid,
    IHTMLDOMNode2_tid,
    IHTMLElement_tid,
    IHTMLElement2_tid,
    0
};

static dispex_static_data_t HTMLElement_dispex = {
    NULL,
    DispHTMLUnknownElement_tid,
    NULL,
    HTMLElement_iface_tids
};

void HTMLElement_Init(HTMLElement *This)
{
    This->lpHTMLElementVtbl = &HTMLElementVtbl;

    ConnectionPointContainer_Init(&This->cp_container, (IUnknown*)HTMLELEM(This));

    HTMLElement2_Init(This);

    if(!This->node.dispex.data)
        init_dispex(&This->node.dispex, (IUnknown*)HTMLELEM(This), &HTMLElement_dispex);
}

HTMLElement *HTMLElement_Create(HTMLDocument *doc, nsIDOMNode *nsnode, BOOL use_generic)
{
    nsIDOMHTMLElement *nselem;
    HTMLElement *ret = NULL;
    nsAString class_name_str;
    const PRUnichar *class_name;
    nsresult nsres;

    static const WCHAR wszA[]        = {'A',0};
    static const WCHAR wszBODY[]     = {'B','O','D','Y',0};
    static const WCHAR wszIMG[]      = {'I','M','G',0};
    static const WCHAR wszINPUT[]    = {'I','N','P','U','T',0};
    static const WCHAR wszOPTION[]   = {'O','P','T','I','O','N',0};
    static const WCHAR wszSCRIPT[]   = {'S','C','R','I','P','T',0};
    static const WCHAR wszSELECT[]   = {'S','E','L','E','C','T',0};
    static const WCHAR wszTABLE[]    = {'T','A','B','L','E',0};
    static const WCHAR wszTEXTAREA[] = {'T','E','X','T','A','R','E','A',0};

    nsres = nsIDOMNode_QueryInterface(nsnode, &IID_nsIDOMHTMLElement, (void**)&nselem);
    if(NS_FAILED(nsres))
        return NULL;

    nsAString_Init(&class_name_str, NULL);
    nsIDOMHTMLElement_GetTagName(nselem, &class_name_str);

    nsAString_GetData(&class_name_str, &class_name);

    if(!strcmpW(class_name, wszA))
        ret = HTMLAnchorElement_Create(nselem);
    else if(!strcmpW(class_name, wszBODY))
        ret = HTMLBodyElement_Create(nselem);
    else if(!strcmpW(class_name, wszIMG))
        ret = HTMLImgElement_Create(nselem);
    else if(!strcmpW(class_name, wszINPUT))
        ret = HTMLInputElement_Create(nselem);
    else if(!strcmpW(class_name, wszOPTION))
        ret = HTMLOptionElement_Create(nselem);
    else if(!strcmpW(class_name, wszSCRIPT))
        ret = HTMLScriptElement_Create(nselem);
    else if(!strcmpW(class_name, wszSELECT))
        ret = HTMLSelectElement_Create(nselem);
    else if(!strcmpW(class_name, wszTABLE))
        ret = HTMLTable_Create(nselem);
    else if(!strcmpW(class_name, wszTEXTAREA))
        ret = HTMLTextAreaElement_Create(nselem);
    else if(use_generic)
        ret = HTMLGenericElement_Create(nselem);

    if(!ret) {
        ret = heap_alloc_zero(sizeof(HTMLElement));
        HTMLElement_Init(ret);
        ret->node.vtbl = &HTMLElementImplVtbl;
    }

    TRACE("%s ret %p\n", debugstr_w(class_name), ret);

    nsAString_Finish(&class_name_str);

    ret->nselem = nselem;
    HTMLDOMNode_Init(doc, &ret->node, (nsIDOMNode*)nselem);

    return ret;
}

typedef struct {
    DispatchEx dispex;
    const IHTMLElementCollectionVtbl *lpHTMLElementCollectionVtbl;

    IUnknown *ref_unk;
    HTMLElement **elems;
    DWORD len;

    LONG ref;
} HTMLElementCollection;

#define HTMLELEMCOL(x)  ((IHTMLElementCollection*) &(x)->lpHTMLElementCollectionVtbl)

#define ELEMCOL_THIS(iface) DEFINE_THIS(HTMLElementCollection, HTMLElementCollection, iface)

static HRESULT WINAPI HTMLElementCollection_QueryInterface(IHTMLElementCollection *iface,
                                                           REFIID riid, void **ppv)
{
    HTMLElementCollection *This = ELEMCOL_THIS(iface);

    *ppv = NULL;

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = HTMLELEMCOL(This);
    }else if(IsEqualGUID(&IID_IHTMLElementCollection, riid)) {
        TRACE("(%p)->(IID_IHTMLElementCollection %p)\n", This, ppv);
        *ppv = HTMLELEMCOL(This);
    }else if(dispex_query_interface(&This->dispex, riid, ppv)) {
        return *ppv ? S_OK : E_NOINTERFACE;
    }

    if(*ppv) {
        IHTMLElementCollection_AddRef(HTMLELEMCOL(This));
        return S_OK;
    }

    FIXME("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppv);
    return E_NOINTERFACE;
}

static ULONG WINAPI HTMLElementCollection_AddRef(IHTMLElementCollection *iface)
{
    HTMLElementCollection *This = ELEMCOL_THIS(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    return ref;
}

static ULONG WINAPI HTMLElementCollection_Release(IHTMLElementCollection *iface)
{
    HTMLElementCollection *This = ELEMCOL_THIS(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    if(!ref) {
        IUnknown_Release(This->ref_unk);
        heap_free(This->elems);
        heap_free(This);
    }

    return ref;
}

static HRESULT WINAPI HTMLElementCollection_GetTypeInfoCount(IHTMLElementCollection *iface,
                                                             UINT *pctinfo)
{
    HTMLElementCollection *This = ELEMCOL_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pctinfo);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElementCollection_GetTypeInfo(IHTMLElementCollection *iface,
        UINT iTInfo, LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLElementCollection *This = ELEMCOL_THIS(iface);
    FIXME("(%p)->(%u %u %p)\n", This, iTInfo, lcid, ppTInfo);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElementCollection_GetIDsOfNames(IHTMLElementCollection *iface,
        REFIID riid, LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{
    HTMLElementCollection *This = ELEMCOL_THIS(iface);
    FIXME("(%p)->(%s %p %u %u %p)\n", This, debugstr_guid(riid), rgszNames, cNames,
                                        lcid, rgDispId);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElementCollection_Invoke(IHTMLElementCollection *iface,
        DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
        VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLElementCollection *This = ELEMCOL_THIS(iface);
    FIXME("(%p)->(%d %s %d %d %p %p %p %p)\n", This, dispIdMember, debugstr_guid(riid),
            lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElementCollection_toString(IHTMLElementCollection *iface,
                                                     BSTR *String)
{
    HTMLElementCollection *This = ELEMCOL_THIS(iface);
    FIXME("(%p)->(%p)\n", This, String);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElementCollection_put_length(IHTMLElementCollection *iface,
                                                       long v)
{
    HTMLElementCollection *This = ELEMCOL_THIS(iface);
    FIXME("(%p)->(%ld)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElementCollection_get_length(IHTMLElementCollection *iface,
                                                       long *p)
{
    HTMLElementCollection *This = ELEMCOL_THIS(iface);

    TRACE("(%p)->(%p)\n", This, p);

    *p = This->len;
    return S_OK;
}

static HRESULT WINAPI HTMLElementCollection_get__newEnum(IHTMLElementCollection *iface,
                                                         IUnknown **p)
{
    HTMLElementCollection *This = ELEMCOL_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static BOOL is_elem_name(HTMLElement *elem, LPCWSTR name)
{
    const PRUnichar *str;
    nsAString nsstr, nsname;
    BOOL ret = FALSE;
    nsresult nsres;

    static const PRUnichar nameW[] = {'n','a','m','e',0};

    if(!elem->nselem)
        return FALSE;

    nsAString_Init(&nsstr, NULL);
    nsIDOMHTMLElement_GetId(elem->nselem, &nsstr);
    nsAString_GetData(&nsstr, &str);
    if(!strcmpiW(str, name)) {
        nsAString_Finish(&nsstr);
        return TRUE;
    }

    nsAString_Init(&nsname, nameW);
    nsres =  nsIDOMHTMLElement_GetAttribute(elem->nselem, &nsname, &nsstr);
    nsAString_Finish(&nsname);
    if(NS_SUCCEEDED(nsres)) {
        nsAString_GetData(&nsstr, &str);
        ret = !strcmpiW(str, name);
    }

    nsAString_Finish(&nsstr);
    return ret;
}

static HRESULT WINAPI HTMLElementCollection_item(IHTMLElementCollection *iface,
        VARIANT name, VARIANT index, IDispatch **pdisp)
{
    HTMLElementCollection *This = ELEMCOL_THIS(iface);

    TRACE("(%p)->(v(%d) v(%d) %p)\n", This, V_VT(&name), V_VT(&index), pdisp);

    *pdisp = NULL;

    if(V_VT(&name) == VT_I4) {
        TRACE("name is VT_I4: %d\n", V_I4(&name));

        if(V_I4(&name) < 0)
            return E_INVALIDARG;
        if(V_I4(&name) >= This->len)
            return S_OK;

        *pdisp = (IDispatch*)This->elems[V_I4(&name)];
        IDispatch_AddRef(*pdisp);
        TRACE("Returning pdisp=%p\n", pdisp);
        return S_OK;
    }

    if(V_VT(&name) == VT_BSTR) {
        DWORD i;

        TRACE("name is VT_BSTR: %s\n", debugstr_w(V_BSTR(&name)));

        if(V_VT(&index) == VT_I4) {
            LONG idx = V_I4(&index);

            TRACE("index = %d\n", idx);

            if(idx < 0)
                return E_INVALIDARG;

            for(i=0; i<This->len; i++) {
                if(is_elem_name(This->elems[i], V_BSTR(&name)) && !idx--)
                    break;
            }

            if(i != This->len) {
                *pdisp = (IDispatch*)HTMLELEM(This->elems[i]);
                IDispatch_AddRef(*pdisp);
            }

            return S_OK;
        }else {
            elem_vector buf = {NULL, 0, 8};

            buf.buf = heap_alloc(buf.size*sizeof(HTMLElement*));

            for(i=0; i<This->len; i++) {
                if(is_elem_name(This->elems[i], V_BSTR(&name)))
                    elem_vector_add(&buf, This->elems[i]);
            }

            if(buf.len > 1) {
                elem_vector_normalize(&buf);
                *pdisp = (IDispatch*)HTMLElementCollection_Create(This->ref_unk, buf.buf, buf.len);
            }else {
                if(buf.len == 1) {
                    *pdisp = (IDispatch*)HTMLELEM(buf.buf[0]);
                    IDispatch_AddRef(*pdisp);
                }

                heap_free(buf.buf);
            }

            return S_OK;
        }
    }

    FIXME("unsupported arguments\n");
    return E_INVALIDARG;
}

static HRESULT WINAPI HTMLElementCollection_tags(IHTMLElementCollection *iface,
                                                 VARIANT tagName, IDispatch **pdisp)
{
    HTMLElementCollection *This = ELEMCOL_THIS(iface);
    DWORD i;
    nsAString tag_str;
    const PRUnichar *tag;
    elem_vector buf = {NULL, 0, 8};

    if(V_VT(&tagName) != VT_BSTR) {
        WARN("Invalid arg\n");
        return DISP_E_MEMBERNOTFOUND;
    }

    TRACE("(%p)->(%s %p)\n", This, debugstr_w(V_BSTR(&tagName)), pdisp);

    buf.buf = heap_alloc(buf.size*sizeof(HTMLElement*));

    nsAString_Init(&tag_str, NULL);

    for(i=0; i<This->len; i++) {
        if(!This->elems[i]->nselem)
            continue;

        nsIDOMElement_GetTagName(This->elems[i]->nselem, &tag_str);
        nsAString_GetData(&tag_str, &tag);

        if(CompareStringW(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE, tag, -1,
                          V_BSTR(&tagName), -1) == CSTR_EQUAL)
            elem_vector_add(&buf, This->elems[i]);
    }

    nsAString_Finish(&tag_str);
    elem_vector_normalize(&buf);

    TRACE("fount %d tags\n", buf.len);

    *pdisp = (IDispatch*)HTMLElementCollection_Create(This->ref_unk, buf.buf, buf.len);
    return S_OK;
}

#define DISPID_ELEMCOL_0 MSHTML_DISPID_CUSTOM_MIN

static HRESULT HTMLElementCollection_get_dispid(IUnknown *iface, BSTR name, DWORD flags, DISPID *dispid)
{
    HTMLElementCollection *This = ELEMCOL_THIS(iface);
    WCHAR *ptr;
    DWORD idx=0;

    for(ptr = name; *ptr && isdigitW(*ptr); ptr++)
        idx = idx*10 + (*ptr-'0');

    if(*ptr || idx >= This->len)
        return DISP_E_UNKNOWNNAME;

    *dispid = DISPID_ELEMCOL_0 + idx;
    TRACE("ret %x\n", *dispid);
    return S_OK;
}

static HRESULT HTMLElementCollection_invoke(IUnknown *iface, DISPID id, LCID lcid, WORD flags, DISPPARAMS *params,
        VARIANT *res, EXCEPINFO *ei, IServiceProvider *caller)
{
    HTMLElementCollection *This = ELEMCOL_THIS(iface);
    DWORD idx;

    TRACE("(%p)->(%x %x %x %p %p %p %p)\n", This, id, lcid, flags, params, res, ei, caller);

    idx = id - DISPID_ELEMCOL_0;
    if(idx >= This->len)
        return DISP_E_UNKNOWNNAME;

    switch(flags) {
    case INVOKE_PROPERTYGET:
        V_VT(res) = VT_DISPATCH;
        V_DISPATCH(res) = (IDispatch*)HTMLELEM(This->elems[idx]);
        IHTMLElement_AddRef(HTMLELEM(This->elems[idx]));
        break;
    default:
        FIXME("unimplemented flags %x\n", flags);
        return E_NOTIMPL;
    }

    return S_OK;
}

#undef ELEMCOL_THIS

static const IHTMLElementCollectionVtbl HTMLElementCollectionVtbl = {
    HTMLElementCollection_QueryInterface,
    HTMLElementCollection_AddRef,
    HTMLElementCollection_Release,
    HTMLElementCollection_GetTypeInfoCount,
    HTMLElementCollection_GetTypeInfo,
    HTMLElementCollection_GetIDsOfNames,
    HTMLElementCollection_Invoke,
    HTMLElementCollection_toString,
    HTMLElementCollection_put_length,
    HTMLElementCollection_get_length,
    HTMLElementCollection_get__newEnum,
    HTMLElementCollection_item,
    HTMLElementCollection_tags
};

static const dispex_static_data_vtbl_t HTMLElementColection_dispex_vtbl = {
    HTMLElementCollection_get_dispid,
    HTMLElementCollection_invoke
};

static const tid_t HTMLElementCollection_iface_tids[] = {
    IHTMLElementCollection_tid,
    0
};
static dispex_static_data_t HTMLElementCollection_dispex = {
    &HTMLElementColection_dispex_vtbl,
    DispHTMLElementCollection_tid,
    NULL,
    HTMLElementCollection_iface_tids
};

IHTMLElementCollection *create_all_collection(HTMLDOMNode *node)
{
    elem_vector buf = {NULL, 0, 8};

    buf.buf = heap_alloc(buf.size*sizeof(HTMLElement**));

    elem_vector_add(&buf, HTMLELEM_NODE_THIS(node));
    create_all_list(node->doc, node, &buf);
    elem_vector_normalize(&buf);

    return HTMLElementCollection_Create((IUnknown*)HTMLDOMNODE(node), buf.buf, buf.len);
}

IHTMLElementCollection *create_collection_from_nodelist(HTMLDocument *doc, IUnknown *unk, nsIDOMNodeList *nslist)
{
    PRUint32 length = 0, i;
    elem_vector buf;

    nsIDOMNodeList_GetLength(nslist, &length);

    buf.len = buf.size = length;
    if(buf.len) {
        nsIDOMNode *nsnode;

        buf.buf = heap_alloc(buf.size*sizeof(HTMLElement*));

        for(i=0; i<length; i++) {
            nsIDOMNodeList_Item(nslist, i, &nsnode);
            buf.buf[i] = HTMLELEM_NODE_THIS(get_node(doc, nsnode, TRUE));
            nsIDOMNode_Release(nsnode);
        }
    }else {
        buf.buf = NULL;
    }

    return HTMLElementCollection_Create(unk, buf.buf, buf.len);
}

static IHTMLElementCollection *HTMLElementCollection_Create(IUnknown *ref_unk,
            HTMLElement **elems, DWORD len)
{
    HTMLElementCollection *ret = heap_alloc_zero(sizeof(HTMLElementCollection));

    ret->lpHTMLElementCollectionVtbl = &HTMLElementCollectionVtbl;
    ret->ref = 1;
    ret->elems = elems;
    ret->len = len;

    init_dispex(&ret->dispex, (IUnknown*)HTMLELEMCOL(ret), &HTMLElementCollection_dispex);

    IUnknown_AddRef(ref_unk);
    ret->ref_unk = ref_unk;

    TRACE("ret=%p len=%d\n", ret, len);

    return HTMLELEMCOL(ret);
}
