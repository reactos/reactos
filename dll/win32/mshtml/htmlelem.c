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
#include "htmlevent.h"

typedef struct
{
    DispatchEx dispex;
    const IHTMLFiltersCollectionVtbl *lpHTMLFiltersCollectionVtbl;

    LONG ref;
} HTMLFiltersCollection;

#define HTMLFILTERSCOLLECTION(x)     ((IHTMLFiltersCollection*)  &(x)->lpHTMLFiltersCollectionVtbl)

#define HTMLFILTERSCOLLECTION_THIS(iface) \
    DEFINE_THIS(HTMLFiltersCollection, HTMLFiltersCollection, iface)

IHTMLFiltersCollection *HTMLFiltersCollection_Create(void);


WINE_DEFAULT_DEBUG_CHANNEL(mshtml);

#define HTMLELEM_THIS(iface) DEFINE_THIS(HTMLElement, HTMLElement, iface)

HRESULT create_nselem(HTMLDocumentNode *doc, const WCHAR *tag, nsIDOMHTMLElement **ret)
{
    nsIDOMElement *nselem;
    nsAString tag_str;
    nsresult nsres;

    if(!doc->nsdoc) {
        WARN("NULL nsdoc\n");
        return E_UNEXPECTED;
    }

    nsAString_InitDepend(&tag_str, tag);
    nsres = nsIDOMDocument_CreateElement(doc->nsdoc, &tag_str, &nselem);
    nsAString_Finish(&tag_str);
    if(NS_FAILED(nsres)) {
        ERR("CreateElement failed: %08x\n", nsres);
        return E_FAIL;
    }

    nsres = nsIDOMElement_QueryInterface(nselem, &IID_nsIDOMHTMLElement, (void**)ret);
    nsIDOMElement_Release(nselem);
    if(NS_FAILED(nsres)) {
        ERR("Could not get nsIDOMHTMLElement iface: %08x\n", nsres);
        return E_FAIL;
    }

    return S_OK;
}

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
    HRESULT hres;
    DISPID dispid, dispidNamed = DISPID_PROPERTYPUT;
    DISPPARAMS dispParams;
    EXCEPINFO excep;

    TRACE("(%p)->(%s . %08x)\n", This, debugstr_w(strAttributeName), lFlags);

    hres = IDispatchEx_GetDispID(DISPATCHEX(&This->node.dispex), strAttributeName,
            fdexNameCaseInsensitive | fdexNameEnsure, &dispid);
    if(FAILED(hres))
        return hres;

    dispParams.cArgs = 1;
    dispParams.cNamedArgs = 1;
    dispParams.rgdispidNamedArgs = &dispidNamed;
    dispParams.rgvarg = &AttributeValue;

    hres = IDispatchEx_InvokeEx(DISPATCHEX(&This->node.dispex), dispid,
            LOCALE_SYSTEM_DEFAULT, DISPATCH_PROPERTYPUT, &dispParams,
            NULL, &excep, NULL);
    return hres;
}

static HRESULT WINAPI HTMLElement_getAttribute(IHTMLElement *iface, BSTR strAttributeName,
                                               LONG lFlags, VARIANT *AttributeValue)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    DISPID dispid;
    HRESULT hres;
    DISPPARAMS dispParams = {NULL, NULL, 0, 0};
    EXCEPINFO excep;

    TRACE("(%p)->(%s %08x %p)\n", This, debugstr_w(strAttributeName), lFlags, AttributeValue);

    hres = IDispatchEx_GetDispID(DISPATCHEX(&This->node.dispex), strAttributeName,
            fdexNameCaseInsensitive, &dispid);
    if(hres == DISP_E_UNKNOWNNAME) {
        V_VT(AttributeValue) = VT_NULL;
        return S_OK;
    }

    if(FAILED(hres)) {
        V_VT(AttributeValue) = VT_NULL;
        return hres;
    }

    hres = IDispatchEx_InvokeEx(DISPATCHEX(&This->node.dispex), dispid,
            LOCALE_SYSTEM_DEFAULT, DISPATCH_PROPERTYGET, &dispParams,
            AttributeValue, &excep, NULL);

    return hres;
}

static HRESULT WINAPI HTMLElement_removeAttribute(IHTMLElement *iface, BSTR strAttributeName,
                                                  LONG lFlags, VARIANT_BOOL *pfSuccess)
{
    HTMLElement *This = HTMLELEM_THIS(iface);

    TRACE("(%p)->(%s %x %p)\n", This, debugstr_w(strAttributeName), lFlags, pfSuccess);

    return remove_prop(&This->node.dispex, strAttributeName, pfSuccess);
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

    nsAString_InitDepend(&classname_str, v);
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

    nsAString_InitDepend(&id_str, v);
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

    TRACE("(%p)->(%p)\n", This, p);

    return get_node_event(&This->node, EVENTID_CLICK, p);
}

static HRESULT WINAPI HTMLElement_put_ondblclick(IHTMLElement *iface, VARIANT v)
{
    HTMLElement *This = HTMLELEM_THIS(iface);

    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_node_event(&This->node, EVENTID_DBLCLICK, &v);
}

static HRESULT WINAPI HTMLElement_get_ondblclick(IHTMLElement *iface, VARIANT *p)
{
    HTMLElement *This = HTMLELEM_THIS(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_node_event(&This->node, EVENTID_DBLCLICK, p);
}

static HRESULT WINAPI HTMLElement_put_onkeydown(IHTMLElement *iface, VARIANT v)
{
    HTMLElement *This = HTMLELEM_THIS(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_node_event(&This->node, EVENTID_KEYDOWN, &v);
}

static HRESULT WINAPI HTMLElement_get_onkeydown(IHTMLElement *iface, VARIANT *p)
{
    HTMLElement *This = HTMLELEM_THIS(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_node_event(&This->node, EVENTID_KEYDOWN, p);
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

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_node_event(&This->node, EVENTID_MOUSEOUT, &v);
}

static HRESULT WINAPI HTMLElement_get_onmouseout(IHTMLElement *iface, VARIANT *p)
{
    HTMLElement *This = HTMLELEM_THIS(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_node_event(&This->node, EVENTID_MOUSEOUT, p);
}

static HRESULT WINAPI HTMLElement_put_onmouseover(IHTMLElement *iface, VARIANT v)
{
    HTMLElement *This = HTMLELEM_THIS(iface);

    TRACE("(%p)->()\n", This);

    return set_node_event(&This->node, EVENTID_MOUSEOVER, &v);
}

static HRESULT WINAPI HTMLElement_get_onmouseover(IHTMLElement *iface, VARIANT *p)
{
    HTMLElement *This = HTMLELEM_THIS(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_node_event(&This->node, EVENTID_MOUSEOVER, p);
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

    TRACE("(%p)->()\n", This);

    return set_node_event(&This->node, EVENTID_MOUSEDOWN, &v);
}

static HRESULT WINAPI HTMLElement_get_onmousedown(IHTMLElement *iface, VARIANT *p)
{
    HTMLElement *This = HTMLELEM_THIS(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_node_event(&This->node, EVENTID_MOUSEDOWN, p);
}

static HRESULT WINAPI HTMLElement_put_onmouseup(IHTMLElement *iface, VARIANT v)
{
    HTMLElement *This = HTMLELEM_THIS(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_node_event(&This->node, EVENTID_MOUSEUP, &v);
}

static HRESULT WINAPI HTMLElement_get_onmouseup(IHTMLElement *iface, VARIANT *p)
{
    HTMLElement *This = HTMLELEM_THIS(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_node_event(&This->node, EVENTID_MOUSEUP, p);
}

static HRESULT WINAPI HTMLElement_get_document(IHTMLElement *iface, IDispatch **p)
{
    HTMLElement *This = HTMLELEM_THIS(iface);

    TRACE("(%p)->(%p)\n", This, p);

    if(!p)
        return E_POINTER;

    if(This->node.vtbl->get_document)
        return This->node.vtbl->get_document(&This->node, p);

    *p = (IDispatch*)HTMLDOC(&This->node.doc->basedoc);
    IDispatch_AddRef(*p);
    return S_OK;
}

static const WCHAR titleW[] = {'t','i','t','l','e',0};

static HRESULT WINAPI HTMLElement_put_title(IHTMLElement *iface, BSTR v)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    nsAString title_str;
    nsresult nsres;

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    if(!This->nselem) {
        VARIANT *var;
        HRESULT hres;

        hres = dispex_get_dprop_ref(&This->node.dispex, titleW, TRUE, &var);
        if(FAILED(hres))
            return hres;

        VariantClear(var);
        V_VT(var) = VT_BSTR;
        V_BSTR(var) = v ? SysAllocString(v) : NULL;
        return S_OK;
    }

    nsAString_InitDepend(&title_str, v);
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

    if(!This->nselem) {
        VARIANT *var;
        HRESULT hres;

        hres = dispex_get_dprop_ref(&This->node.dispex, titleW, FALSE, &var);
        if(hres == DISP_E_UNKNOWNNAME) {
            *p = NULL;
        }else if(V_VT(var) != VT_BSTR) {
            FIXME("title = %s\n", debugstr_variant(var));
            return E_FAIL;
        }else {
            *p = V_BSTR(var) ? SysAllocString(V_BSTR(var)) : NULL;
        }

        return S_OK;
    }

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

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_node_event(&This->node, EVENTID_SELECTSTART, &v);
}

static HRESULT WINAPI HTMLElement_get_onselectstart(IHTMLElement *iface, VARIANT *p)
{
    HTMLElement *This = HTMLELEM_THIS(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_node_event(&This->node, EVENTID_SELECTSTART, p);
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

static HRESULT WINAPI HTMLElement_get_sourceIndex(IHTMLElement *iface, LONG *p)
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

static HRESULT WINAPI HTMLElement_get_offsetLeft(IHTMLElement *iface, LONG *p)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_get_offsetTop(IHTMLElement *iface, LONG *p)
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

static HRESULT WINAPI HTMLElement_get_offsetWidth(IHTMLElement *iface, LONG *p)
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

    nsres = nsIDOMNSHTMLElement_GetOffsetWidth(nselem, &offset);
    nsIDOMNSHTMLElement_Release(nselem);
    if(NS_FAILED(nsres)) {
        ERR("GetOffsetWidth failed: %08x\n", nsres);
        return E_FAIL;
    }

    *p = offset;
    return S_OK;
}

static HRESULT WINAPI HTMLElement_get_offsetHeight(IHTMLElement *iface, LONG *p)
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

    nsAString_InitDepend(&html_str, v);
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
    nsIDOMNSHTMLElement *nselem;
    nsAString html_str;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    if(!This->nselem) {
        FIXME("NULL nselem\n");
        return E_NOTIMPL;
    }

    nsres = nsIDOMHTMLElement_QueryInterface(This->nselem, &IID_nsIDOMNSHTMLElement, (void**)&nselem);
    if(NS_FAILED(nsres)) {
        ERR("Could not get nsIDOMNSHTMLElement: %08x\n", nsres);
        return E_FAIL;
    }

    nsAString_Init(&html_str, NULL);
    nsres = nsIDOMNSHTMLElement_GetInnerHTML(nselem, &html_str);
    if(NS_SUCCEEDED(nsres)) {
        const PRUnichar *html;

        nsAString_GetData(&html_str, &html);
        *p = *html ? SysAllocString(html) : NULL;
    }else {
        FIXME("SetInnerHtml failed %08x\n", nsres);
        *p = NULL;
    }

    nsAString_Finish(&html_str);
    return S_OK;
}

static HRESULT WINAPI HTMLElement_put_innerText(IHTMLElement *iface, BSTR v)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    nsIDOMNode *nschild, *tmp;
    nsIDOMText *text_node;
    nsAString text_str;
    nsresult nsres;

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    while(1) {
        nsres = nsIDOMHTMLElement_GetLastChild(This->nselem, &nschild);
        if(NS_FAILED(nsres)) {
            ERR("GetLastChild failed: %08x\n", nsres);
            return E_FAIL;
        }
        if(!nschild)
            break;

        nsres = nsIDOMHTMLElement_RemoveChild(This->nselem, nschild, &tmp);
        nsIDOMNode_Release(nschild);
        if(NS_FAILED(nsres)) {
            ERR("RemoveChild failed: %08x\n", nsres);
            return E_FAIL;
        }
        nsIDOMNode_Release(tmp);
    }

    nsAString_InitDepend(&text_str, v);
    nsres = nsIDOMHTMLDocument_CreateTextNode(This->node.doc->nsdoc, &text_str, &text_node);
    nsAString_Finish(&text_str);
    if(NS_FAILED(nsres)) {
        ERR("CreateTextNode failed: %08x\n", nsres);
        return E_FAIL;
    }

    nsres = nsIDOMHTMLElement_AppendChild(This->nselem, (nsIDOMNode*)text_node, &tmp);
    if(NS_FAILED(nsres)) {
        ERR("AppendChild failed: %08x\n", nsres);
        return E_FAIL;
    }

    nsIDOMNode_Release(tmp);
    return S_OK;
}

static HRESULT WINAPI HTMLElement_get_innerText(IHTMLElement *iface, BSTR *p)
{
    HTMLElement *This = HTMLELEM_THIS(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_node_text(&This->node, p);
}

static HRESULT WINAPI HTMLElement_put_outerHTML(IHTMLElement *iface, BSTR v)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    nsIDOMDocumentFragment *nsfragment;
    nsIDOMDocumentRange *nsdocrange;
    nsIDOMNSRange *nsrange;
    nsIDOMNode *nsparent;
    nsIDOMRange *range;
    nsAString html_str;
    nsresult nsres;
    HRESULT hres = S_OK;

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    nsres = nsIDOMHTMLDocument_QueryInterface(This->node.doc->nsdoc, &IID_nsIDOMDocumentRange, (void**)&nsdocrange);
    if(NS_FAILED(nsres))
        return E_FAIL;

    nsres = nsIDOMDocumentRange_CreateRange(nsdocrange, &range);
    nsIDOMDocumentRange_Release(nsdocrange);
    if(NS_FAILED(nsres)) {
        ERR("CreateRange failed: %08x\n", nsres);
        return E_FAIL;
    }

    nsres = nsIDOMRange_QueryInterface(range, &IID_nsIDOMNSRange, (void**)&nsrange);
    nsIDOMRange_Release(range);
    if(NS_FAILED(nsres)) {
        ERR("Could not get nsIDOMNSRange: %08x\n", nsres);
        return E_FAIL;
    }

    nsAString_InitDepend(&html_str, v);
    nsIDOMNSRange_CreateContextualFragment(nsrange, &html_str, &nsfragment);
    nsIDOMNSRange_Release(nsrange);
    nsAString_Finish(&html_str);
    if(NS_FAILED(nsres)) {
        ERR("CreateContextualFragment failed: %08x\n", nsres);
        return E_FAIL;
    }

    nsres = nsIDOMNode_GetParentNode(This->node.nsnode, &nsparent);
    if(NS_SUCCEEDED(nsres) && nsparent) {
        nsIDOMNode *nstmp;

        nsres = nsIDOMNode_ReplaceChild(nsparent, (nsIDOMNode*)nsfragment, This->node.nsnode, &nstmp);
        nsIDOMNode_Release(nsparent);
        if(NS_FAILED(nsres)) {
            ERR("ReplaceChild failed: %08x\n", nsres);
            hres = E_FAIL;
        }else if(nstmp) {
            nsIDOMNode_Release(nstmp);
        }
    }else {
        ERR("GetParentNode failed: %08x\n", nsres);
        hres = E_FAIL;
    }

    nsIDOMDocumentFragment_Release(nsfragment);
    return hres;
}

static HRESULT WINAPI HTMLElement_get_outerHTML(IHTMLElement *iface, BSTR *p)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    nsAString html_str;
    HRESULT hres;

    WARN("(%p)->(%p) semi-stub\n", This, p);

    nsAString_Init(&html_str, NULL);
    hres = nsnode_to_nsstring(This->node.nsnode, &html_str);
    if(SUCCEEDED(hres)) {
        const PRUnichar *html;

        nsAString_GetData(&html_str, &html);
        *p = SysAllocString(html);
        if(!*p)
            hres = E_OUTOFMEMORY;
    }

    nsAString_Finish(&html_str);

    TRACE("ret %s\n", debugstr_w(*p));
    return S_OK;
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

    if (!strcmpiW(where, wszBeforeBegin))
    {
        nsIDOMNode *unused;
        nsIDOMNode *parent;
        nsres = nsIDOMNode_GetParentNode(This->node.nsnode, &parent);
        if (!parent) return E_INVALIDARG;
        nsres = nsIDOMNode_InsertBefore(parent, nsnode, This->node.nsnode, &unused);
        if (unused) nsIDOMNode_Release(unused);
        nsIDOMNode_Release(parent);
    }
    else if (!strcmpiW(where, wszAfterBegin))
    {
        nsIDOMNode *unused;
        nsIDOMNode *first_child;
        nsIDOMNode_GetFirstChild(This->node.nsnode, &first_child);
        nsres = nsIDOMNode_InsertBefore(This->node.nsnode, nsnode, first_child, &unused);
        if (unused) nsIDOMNode_Release(unused);
        if (first_child) nsIDOMNode_Release(first_child);
    }
    else if (!strcmpiW(where, wszBeforeEnd))
    {
        nsIDOMNode *unused;
        nsres = nsIDOMNode_AppendChild(This->node.nsnode, nsnode, &unused);
        if (unused) nsIDOMNode_Release(unused);
    }
    else if (!strcmpiW(where, wszAfterEnd))
    {
        nsIDOMNode *unused;
        nsIDOMNode *next_sibling;
        nsIDOMNode *parent;
        nsIDOMNode_GetParentNode(This->node.nsnode, &parent);
        if (!parent) return E_INVALIDARG;

        nsIDOMNode_GetNextSibling(This->node.nsnode, &next_sibling);
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
    nsIDOMDocumentRange *nsdocrange;
    nsIDOMRange *range;
    nsIDOMNSRange *nsrange;
    nsIDOMNode *nsnode;
    nsAString ns_html;
    nsresult nsres;
    HRESULT hr;

    TRACE("(%p)->(%s %s)\n", This, debugstr_w(where), debugstr_w(html));

    if(!This->node.doc->nsdoc) {
        WARN("NULL nsdoc\n");
        return E_UNEXPECTED;
    }

    nsres = nsIDOMDocument_QueryInterface(This->node.doc->nsdoc, &IID_nsIDOMDocumentRange, (void **)&nsdocrange);
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

    nsIDOMRange_SetStartBefore(range, This->node.nsnode);

    nsIDOMRange_QueryInterface(range, &IID_nsIDOMNSRange, (void **)&nsrange);
    nsIDOMRange_Release(range);
    if(NS_FAILED(nsres))
    {
        ERR("getting nsIDOMNSRange failed: %08x\n", nsres);
        return E_FAIL;
    }

    nsAString_InitDepend(&ns_html, html);

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
    nsIDOMNode *nsnode;
    nsAString ns_text;
    nsresult nsres;
    HRESULT hr;

    TRACE("(%p)->(%s %s)\n", This, debugstr_w(where), debugstr_w(text));

    if(!This->node.doc->nsdoc) {
        WARN("NULL nsdoc\n");
        return E_UNEXPECTED;
    }


    nsAString_InitDepend(&ns_text, text);
    nsres = nsIDOMDocument_CreateTextNode(This->node.doc->nsdoc, &ns_text, (nsIDOMText **)&nsnode);
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

    TRACE("(%p)\n", This);

    return call_event(&This->node, EVENTID_CLICK);
}

static HRESULT WINAPI HTMLElement_get_filters(IHTMLElement *iface,
                                              IHTMLFiltersCollection **p)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    TRACE("(%p)->(%p)\n", This, p);

    if(!p)
        return E_POINTER;

    *p = HTMLFiltersCollection_Create();

    return S_OK;
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

static HRESULT WINAPI HTMLElement_get_children(IHTMLElement *iface, IDispatch **p)
{
    HTMLElement *This = HTMLELEM_THIS(iface);
    nsIDOMNodeList *nsnode_list;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsres = nsIDOMNode_GetChildNodes(This->node.nsnode, &nsnode_list);
    if(NS_FAILED(nsres)) {
        ERR("GetChildNodes failed: %08x\n", nsres);
        return E_FAIL;
    }

    *p = (IDispatch*)create_collection_from_nodelist(This->node.doc, (IUnknown*)HTMLELEM(This), nsnode_list);

    nsIDOMNodeList_Release(nsnode_list);
    return S_OK;
}

static HRESULT WINAPI HTMLElement_get_all(IHTMLElement *iface, IDispatch **p)
{
    HTMLElement *This = HTMLELEM_THIS(iface);

    TRACE("(%p)->(%p)\n", This, p);

    *p = (IDispatch*)create_all_collection(&This->node, FALSE);
    return S_OK;
}

static HRESULT HTMLElement_get_dispid(IUnknown *iface, BSTR name,
        DWORD grfdex, DISPID *pid)
{
    HTMLElement *This = HTMLELEM_THIS(iface);

    if(This->node.vtbl->get_dispid)
        return This->node.vtbl->get_dispid(&This->node, name, grfdex, pid);

    return DISP_E_UNKNOWNNAME;
}

static HRESULT HTMLElement_invoke(IUnknown *iface, DISPID id, LCID lcid,
        WORD flags, DISPPARAMS *params, VARIANT *res, EXCEPINFO *ei,
        IServiceProvider *caller)
{
    HTMLElement *This = HTMLELEM_THIS(iface);

    if(This->node.vtbl->invoke)
        return This->node.vtbl->invoke(&This->node, id, lcid, flags,
                params, res, ei, caller);

    ERR("(%p): element has no invoke method\n", This);
    return E_NOTIMPL;
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
    }else if(IsEqualGUID(&IID_IHTMLElement3, riid)) {
        TRACE("(%p)->(IID_IHTMLElement3 %p)\n", This, ppv);
        *ppv = HTMLELEM3(This);
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
    HTMLELEMENT_TIDS,
    0
};

static dispex_static_data_vtbl_t HTMLElement_dispex_vtbl = {
    NULL,
    HTMLElement_get_dispid,
    HTMLElement_invoke
};

static dispex_static_data_t HTMLElement_dispex = {
    &HTMLElement_dispex_vtbl,
    DispHTMLUnknownElement_tid,
    NULL,
    HTMLElement_iface_tids
};

void HTMLElement_Init(HTMLElement *This, HTMLDocumentNode *doc, nsIDOMHTMLElement *nselem, dispex_static_data_t *dispex_data)
{
    This->lpHTMLElementVtbl = &HTMLElementVtbl;

    HTMLElement2_Init(This);
    HTMLElement3_Init(This);

    if(dispex_data && !dispex_data->vtbl)
        dispex_data->vtbl = &HTMLElement_dispex_vtbl;
    init_dispex(&This->node.dispex, (IUnknown*)HTMLELEM(This), dispex_data ? dispex_data : &HTMLElement_dispex);

    if(nselem)
        nsIDOMHTMLElement_AddRef(nselem);
    This->nselem = nselem;

    HTMLDOMNode_Init(doc, &This->node, (nsIDOMNode*)nselem);

    ConnectionPointContainer_Init(&This->cp_container, (IUnknown*)HTMLELEM(This));
}

HTMLElement *HTMLElement_Create(HTMLDocumentNode *doc, nsIDOMNode *nsnode, BOOL use_generic)
{
    nsIDOMHTMLElement *nselem;
    HTMLElement *ret = NULL;
    nsAString class_name_str;
    const PRUnichar *class_name;
    nsresult nsres;

    static const WCHAR wszA[]        = {'A',0};
    static const WCHAR wszBODY[]     = {'B','O','D','Y',0};
    static const WCHAR wszFORM[]     = {'F','O','R','M',0};
    static const WCHAR wszFRAME[]    = {'F','R','A','M','E',0};
    static const WCHAR wszIFRAME[]   = {'I','F','R','A','M','E',0};
    static const WCHAR wszIMG[]      = {'I','M','G',0};
    static const WCHAR wszINPUT[]    = {'I','N','P','U','T',0};
    static const WCHAR wszOPTION[]   = {'O','P','T','I','O','N',0};
    static const WCHAR wszSCRIPT[]   = {'S','C','R','I','P','T',0};
    static const WCHAR wszSELECT[]   = {'S','E','L','E','C','T',0};
    static const WCHAR wszTABLE[]    = {'T','A','B','L','E',0};
    static const WCHAR wszTR[]       = {'T','R',0};
    static const WCHAR wszTEXTAREA[] = {'T','E','X','T','A','R','E','A',0};

    nsres = nsIDOMNode_QueryInterface(nsnode, &IID_nsIDOMHTMLElement, (void**)&nselem);
    if(NS_FAILED(nsres))
        return NULL;

    nsAString_Init(&class_name_str, NULL);
    nsIDOMHTMLElement_GetTagName(nselem, &class_name_str);

    nsAString_GetData(&class_name_str, &class_name);

    if(!strcmpW(class_name, wszA))
        ret = HTMLAnchorElement_Create(doc, nselem);
    else if(!strcmpW(class_name, wszBODY))
        ret = HTMLBodyElement_Create(doc, nselem);
    else if(!strcmpW(class_name, wszFORM))
        ret = HTMLFormElement_Create(doc, nselem);
    else if(!strcmpW(class_name, wszFRAME))
        ret = HTMLFrameElement_Create(doc, nselem);
    else if(!strcmpW(class_name, wszIFRAME))
        ret = HTMLIFrame_Create(doc, nselem);
    else if(!strcmpW(class_name, wszIMG))
        ret = HTMLImgElement_Create(doc, nselem);
    else if(!strcmpW(class_name, wszINPUT))
        ret = HTMLInputElement_Create(doc, nselem);
    else if(!strcmpW(class_name, wszOPTION))
        ret = HTMLOptionElement_Create(doc, nselem);
    else if(!strcmpW(class_name, wszSCRIPT))
        ret = HTMLScriptElement_Create(doc, nselem);
    else if(!strcmpW(class_name, wszSELECT))
        ret = HTMLSelectElement_Create(doc, nselem);
    else if(!strcmpW(class_name, wszTABLE))
        ret = HTMLTable_Create(doc, nselem);
    else if(!strcmpW(class_name, wszTR))
        ret = HTMLTableRow_Create(doc, nselem);
    else if(!strcmpW(class_name, wszTEXTAREA))
        ret = HTMLTextAreaElement_Create(doc, nselem);
    else if(use_generic)
        ret = HTMLGenericElement_Create(doc, nselem);

    if(!ret) {
        ret = heap_alloc_zero(sizeof(HTMLElement));
        HTMLElement_Init(ret, doc, nselem, &HTMLElement_dispex);
        ret->node.vtbl = &HTMLElementImplVtbl;
    }

    TRACE("%s ret %p\n", debugstr_w(class_name), ret);

    nsIDOMElement_Release(nselem);
    nsAString_Finish(&class_name_str);

    return ret;
}

/* interaface IHTMLFiltersCollection */
static HRESULT WINAPI HTMLFiltersCollection_QueryInterface(IHTMLFiltersCollection *iface, REFIID riid, void **ppv)
{
    HTMLFiltersCollection *This = HTMLFILTERSCOLLECTION_THIS(iface);

    TRACE("%p %s %p\n", This, debugstr_guid( riid ), ppv );

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = HTMLFILTERSCOLLECTION(This);
    }else if(IsEqualGUID(&IID_IHTMLFiltersCollection, riid)) {
        TRACE("(%p)->(IID_IHTMLFiltersCollection %p)\n", This, ppv);
        *ppv = HTMLFILTERSCOLLECTION(This);
    }else if(dispex_query_interface(&This->dispex, riid, ppv)) {
        return *ppv ? S_OK : E_NOINTERFACE;
    }

    if(*ppv) {
        IUnknown_AddRef((IUnknown*)*ppv);
        return S_OK;
    }

    FIXME("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppv);
    return E_NOINTERFACE;
}

static ULONG WINAPI HTMLFiltersCollection_AddRef(IHTMLFiltersCollection *iface)
{
    HTMLFiltersCollection *This = HTMLFILTERSCOLLECTION_THIS(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    return ref;
}

static ULONG WINAPI HTMLFiltersCollection_Release(IHTMLFiltersCollection *iface)
{
    HTMLFiltersCollection *This = HTMLFILTERSCOLLECTION_THIS(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    if(!ref)
    {
        heap_free(This);
    }

    return ref;
}

static HRESULT WINAPI HTMLFiltersCollection_GetTypeInfoCount(IHTMLFiltersCollection *iface, UINT *pctinfo)
{
    HTMLFiltersCollection *This = HTMLFILTERSCOLLECTION_THIS(iface);
    return IDispatchEx_GetTypeInfoCount(DISPATCHEX(&This->dispex), pctinfo);
}

static HRESULT WINAPI HTMLFiltersCollection_GetTypeInfo(IHTMLFiltersCollection *iface,
                                    UINT iTInfo, LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLFiltersCollection *This = HTMLFILTERSCOLLECTION_THIS(iface);
    return IDispatchEx_GetTypeInfo(DISPATCHEX(&This->dispex), iTInfo, lcid, ppTInfo);
}

static HRESULT WINAPI HTMLFiltersCollection_GetIDsOfNames(IHTMLFiltersCollection *iface,
                                    REFIID riid, LPOLESTR *rgszNames, UINT cNames,
                                    LCID lcid, DISPID *rgDispId)
{
    HTMLFiltersCollection *This = HTMLFILTERSCOLLECTION_THIS(iface);
    return IDispatchEx_GetIDsOfNames(DISPATCHEX(&This->dispex), riid, rgszNames, cNames, lcid, rgDispId);
}

static HRESULT WINAPI HTMLFiltersCollection_Invoke(IHTMLFiltersCollection *iface, DISPID dispIdMember, REFIID riid,
                                    LCID lcid, WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult,
                                    EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLFiltersCollection *This = HTMLFILTERSCOLLECTION_THIS(iface);
    return IDispatchEx_Invoke(DISPATCHEX(&This->dispex), dispIdMember, riid, lcid,
            wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI HTMLFiltersCollection_get_length(IHTMLFiltersCollection *iface, LONG *p)
{
    HTMLFiltersCollection *This = HTMLFILTERSCOLLECTION_THIS(iface);

    if(!p)
        return E_POINTER;

    FIXME("(%p)->(%p) Always returning 0\n", This, p);
    *p = 0;

    return S_OK;
}

static HRESULT WINAPI HTMLFiltersCollection_get__newEnum(IHTMLFiltersCollection *iface, IUnknown **p)
{
    HTMLFiltersCollection *This = HTMLFILTERSCOLLECTION_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLFiltersCollection_item(IHTMLFiltersCollection *iface, VARIANT *pvarIndex, VARIANT *pvarResult)
{
    HTMLFiltersCollection *This = HTMLFILTERSCOLLECTION_THIS(iface);
    FIXME("(%p)->(%p, %p)\n", This, pvarIndex, pvarResult);
    return E_NOTIMPL;
}

static const IHTMLFiltersCollectionVtbl HTMLFiltersCollectionVtbl = {
    HTMLFiltersCollection_QueryInterface,
    HTMLFiltersCollection_AddRef,
    HTMLFiltersCollection_Release,
    HTMLFiltersCollection_GetTypeInfoCount,
    HTMLFiltersCollection_GetTypeInfo,
    HTMLFiltersCollection_GetIDsOfNames,
    HTMLFiltersCollection_Invoke,
    HTMLFiltersCollection_get_length,
    HTMLFiltersCollection_get__newEnum,
    HTMLFiltersCollection_item
};

static HRESULT HTMLFiltersCollection_get_dispid(IUnknown *iface, BSTR name, DWORD flags, DISPID *dispid)
{
    WCHAR *ptr;
    int idx = 0;

    for(ptr = name; *ptr && isdigitW(*ptr); ptr++)
        idx = idx*10 + (*ptr-'0');
    if(*ptr)
        return DISP_E_UNKNOWNNAME;

    *dispid = MSHTML_DISPID_CUSTOM_MIN + idx;
    TRACE("ret %x\n", *dispid);
    return S_OK;
}

static HRESULT HTMLFiltersCollection_invoke(IUnknown *iface, DISPID id, LCID lcid, WORD flags, DISPPARAMS *params,
        VARIANT *res, EXCEPINFO *ei, IServiceProvider *caller)
{
    HTMLFiltersCollection *This = HTMLFILTERSCOLLECTION_THIS(iface);

    TRACE("(%p)->(%x %x %x %p %p %p)\n", This, id, lcid, flags, params, res, ei);

    V_VT(res) = VT_DISPATCH;
    V_DISPATCH(res) = NULL;

    FIXME("always returning NULL\n");

    return S_OK;
}

static const dispex_static_data_vtbl_t HTMLFiltersCollection_dispex_vtbl = {
    NULL,
    HTMLFiltersCollection_get_dispid,
    HTMLFiltersCollection_invoke
};

static const tid_t HTMLFiltersCollection_iface_tids[] = {
    IHTMLFiltersCollection_tid,
    0
};
static dispex_static_data_t HTMLFiltersCollection_dispex = {
    &HTMLFiltersCollection_dispex_vtbl,
    IHTMLFiltersCollection_tid,
    NULL,
    HTMLFiltersCollection_iface_tids
};

IHTMLFiltersCollection *HTMLFiltersCollection_Create()
{
    HTMLFiltersCollection *ret = heap_alloc(sizeof(HTMLFiltersCollection));

    ret->lpHTMLFiltersCollectionVtbl = &HTMLFiltersCollectionVtbl;
    ret->ref = 1;

    init_dispex(&ret->dispex, (IUnknown*)HTMLFILTERSCOLLECTION(ret),  &HTMLFiltersCollection_dispex);

    return HTMLFILTERSCOLLECTION(ret);
}
