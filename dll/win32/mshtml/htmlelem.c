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

#include "mshtml_private.h"

static const WCHAR aW[]        = {'A',0};
static const WCHAR bodyW[]     = {'B','O','D','Y',0};
static const WCHAR buttonW[]   = {'B','U','T','T','O','N',0};
static const WCHAR embedW[]    = {'E','M','B','E','D',0};
static const WCHAR formW[]     = {'F','O','R','M',0};
static const WCHAR frameW[]    = {'F','R','A','M','E',0};
static const WCHAR headW[]     = {'H','E','A','D',0};
static const WCHAR iframeW[]   = {'I','F','R','A','M','E',0};
static const WCHAR imgW[]      = {'I','M','G',0};
static const WCHAR inputW[]    = {'I','N','P','U','T',0};
static const WCHAR labelW[]    = {'L','A','B','E','L',0};
static const WCHAR linkW[]     = {'L','I','N','K',0};
static const WCHAR metaW[]     = {'M','E','T','A',0};
static const WCHAR objectW[]   = {'O','B','J','E','C','T',0};
static const WCHAR optionW[]   = {'O','P','T','I','O','N',0};
static const WCHAR scriptW[]   = {'S','C','R','I','P','T',0};
static const WCHAR selectW[]   = {'S','E','L','E','C','T',0};
static const WCHAR styleW[]    = {'S','T','Y','L','E',0};
static const WCHAR tableW[]    = {'T','A','B','L','E',0};
static const WCHAR tdW[]       = {'T','D',0};
static const WCHAR textareaW[] = {'T','E','X','T','A','R','E','A',0};
static const WCHAR title_tagW[]= {'T','I','T','L','E',0};
static const WCHAR trW[]       = {'T','R',0};

typedef struct {
    const WCHAR *name;
    HRESULT (*constructor)(HTMLDocumentNode*,nsIDOMHTMLElement*,HTMLElement**);
} tag_desc_t;

static const tag_desc_t tag_descs[] = {
    {aW,         HTMLAnchorElement_Create},
    {bodyW,      HTMLBodyElement_Create},
    {buttonW,    HTMLButtonElement_Create},
    {embedW,     HTMLEmbedElement_Create},
    {formW,      HTMLFormElement_Create},
    {frameW,     HTMLFrameElement_Create},
    {headW,      HTMLHeadElement_Create},
    {iframeW,    HTMLIFrame_Create},
    {imgW,       HTMLImgElement_Create},
    {inputW,     HTMLInputElement_Create},
    {labelW,     HTMLLabelElement_Create},
    {linkW,      HTMLLinkElement_Create},
    {metaW,      HTMLMetaElement_Create},
    {objectW,    HTMLObjectElement_Create},
    {optionW,    HTMLOptionElement_Create},
    {scriptW,    HTMLScriptElement_Create},
    {selectW,    HTMLSelectElement_Create},
    {styleW,     HTMLStyleElement_Create},
    {tableW,     HTMLTable_Create},
    {tdW,        HTMLTableCell_Create},
    {textareaW,  HTMLTextAreaElement_Create},
    {title_tagW, HTMLTitleElement_Create},
    {trW,        HTMLTableRow_Create}
};

static const tag_desc_t *get_tag_desc(const WCHAR *tag_name)
{
    DWORD min=0, max=sizeof(tag_descs)/sizeof(*tag_descs)-1, i;
    int r;

    while(min <= max) {
        i = (min+max)/2;
        r = strcmpW(tag_name, tag_descs[i].name);
        if(!r)
            return tag_descs+i;

        if(r < 0)
            max = i-1;
        else
            min = i+1;
    }

    return NULL;
}

HRESULT replace_node_by_html(nsIDOMHTMLDocument *nsdoc, nsIDOMNode *nsnode, const WCHAR *html)
{
    nsIDOMDocumentFragment *nsfragment;
    nsIDOMNode *nsparent;
    nsIDOMRange *range;
    nsAString html_str;
    nsresult nsres;
    HRESULT hres = S_OK;

    nsres = nsIDOMHTMLDocument_CreateRange(nsdoc, &range);
    if(NS_FAILED(nsres)) {
        ERR("CreateRange failed: %08x\n", nsres);
        return E_FAIL;
    }

    nsAString_InitDepend(&html_str, html);
    nsIDOMRange_CreateContextualFragment(range, &html_str, &nsfragment);
    nsIDOMRange_Release(range);
    nsAString_Finish(&html_str);
    if(NS_FAILED(nsres)) {
        ERR("CreateContextualFragment failed: %08x\n", nsres);
        return E_FAIL;
    }

    nsres = nsIDOMNode_GetParentNode(nsnode, &nsparent);
    if(NS_SUCCEEDED(nsres) && nsparent) {
        nsIDOMNode *nstmp;

        nsres = nsIDOMNode_ReplaceChild(nsparent, (nsIDOMNode*)nsfragment, nsnode, &nstmp);
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

nsresult get_elem_attr_value(nsIDOMHTMLElement *nselem, const WCHAR *name, nsAString *val_str, const PRUnichar **val)
{
    nsAString name_str;
    nsresult nsres;

    nsAString_InitDepend(&name_str, name);
    nsAString_Init(val_str, NULL);
    nsres = nsIDOMHTMLElement_GetAttribute(nselem, &name_str, val_str);
    nsAString_Finish(&name_str);
    if(NS_FAILED(nsres)) {
        ERR("GetAttribute(%s) failed: %08x\n", debugstr_w(name), nsres);
        nsAString_Finish(val_str);
        return nsres;
    }

    nsAString_GetData(val_str, val);
    return NS_OK;
}

HRESULT elem_string_attr_getter(HTMLElement *elem, const WCHAR *name, BOOL use_null, BSTR *p)
{
    const PRUnichar *val;
    nsAString val_str;
    nsresult nsres;
    HRESULT hres = S_OK;

    nsres = get_elem_attr_value(elem->nselem, name, &val_str, &val);
    if(NS_FAILED(nsres))
        return E_FAIL;

    TRACE("%s: returning %s\n", debugstr_w(name), debugstr_w(val));

    if(*val || !use_null) {
        *p = SysAllocString(val);
        if(!*p)
            hres = E_OUTOFMEMORY;
    }else {
        *p = NULL;
    }
    nsAString_Finish(&val_str);
    return hres;
}

HRESULT elem_string_attr_setter(HTMLElement *elem, const WCHAR *name, const WCHAR *value)
{
    nsAString name_str, val_str;
    nsresult nsres;

    nsAString_InitDepend(&name_str, name);
    nsAString_InitDepend(&val_str, value);
    nsres = nsIDOMHTMLElement_SetAttribute(elem->nselem, &name_str, &val_str);
    nsAString_Finish(&name_str);
    nsAString_Finish(&val_str);

    if(NS_FAILED(nsres)) {
        WARN("SetAttribute failed: %08x\n", nsres);
        return E_FAIL;
    }

    return S_OK;
}

typedef struct
{
    DispatchEx dispex;
    IHTMLFiltersCollection IHTMLFiltersCollection_iface;

    LONG ref;
} HTMLFiltersCollection;

static inline HTMLFiltersCollection *impl_from_IHTMLFiltersCollection(IHTMLFiltersCollection *iface)
{
    return CONTAINING_RECORD(iface, HTMLFiltersCollection, IHTMLFiltersCollection_iface);
}

static IHTMLFiltersCollection *HTMLFiltersCollection_Create(void);

static inline HTMLElement *impl_from_IHTMLElement(IHTMLElement *iface)
{
    return CONTAINING_RECORD(iface, HTMLElement, IHTMLElement_iface);
}

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
    nsres = nsIDOMHTMLDocument_CreateElement(doc->nsdoc, &tag_str, &nselem);
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

HRESULT create_element(HTMLDocumentNode *doc, const WCHAR *tag, HTMLElement **ret)
{
    nsIDOMHTMLElement *nselem;
    HRESULT hres;

    /* Use owner doc if called on document fragment */
    if(!doc->nsdoc)
        doc = doc->node.doc;

    hres = create_nselem(doc, tag, &nselem);
    if(FAILED(hres))
        return hres;

    hres = HTMLElement_Create(doc, (nsIDOMNode*)nselem, TRUE, ret);
    nsIDOMHTMLElement_Release(nselem);
    return hres;
}

static HRESULT WINAPI HTMLElement_QueryInterface(IHTMLElement *iface,
                                                 REFIID riid, void **ppv)
{
    HTMLElement *This = impl_from_IHTMLElement(iface);

    return IHTMLDOMNode_QueryInterface(&This->node.IHTMLDOMNode_iface, riid, ppv);
}

static ULONG WINAPI HTMLElement_AddRef(IHTMLElement *iface)
{
    HTMLElement *This = impl_from_IHTMLElement(iface);

    return IHTMLDOMNode_AddRef(&This->node.IHTMLDOMNode_iface);
}

static ULONG WINAPI HTMLElement_Release(IHTMLElement *iface)
{
    HTMLElement *This = impl_from_IHTMLElement(iface);

    return IHTMLDOMNode_Release(&This->node.IHTMLDOMNode_iface);
}

static HRESULT WINAPI HTMLElement_GetTypeInfoCount(IHTMLElement *iface, UINT *pctinfo)
{
    HTMLElement *This = impl_from_IHTMLElement(iface);
    return IDispatchEx_GetTypeInfoCount(&This->node.dispex.IDispatchEx_iface, pctinfo);
}

static HRESULT WINAPI HTMLElement_GetTypeInfo(IHTMLElement *iface, UINT iTInfo,
                                              LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLElement *This = impl_from_IHTMLElement(iface);
    return IDispatchEx_GetTypeInfo(&This->node.dispex.IDispatchEx_iface, iTInfo, lcid, ppTInfo);
}

static HRESULT WINAPI HTMLElement_GetIDsOfNames(IHTMLElement *iface, REFIID riid,
                                                LPOLESTR *rgszNames, UINT cNames,
                                                LCID lcid, DISPID *rgDispId)
{
    HTMLElement *This = impl_from_IHTMLElement(iface);
    return IDispatchEx_GetIDsOfNames(&This->node.dispex.IDispatchEx_iface, riid, rgszNames, cNames,
            lcid, rgDispId);
}

static HRESULT WINAPI HTMLElement_Invoke(IHTMLElement *iface, DISPID dispIdMember,
                            REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
                            VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLElement *This = impl_from_IHTMLElement(iface);
    return IDispatchEx_Invoke(&This->node.dispex.IDispatchEx_iface, dispIdMember, riid, lcid,
            wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI HTMLElement_setAttribute(IHTMLElement *iface, BSTR strAttributeName,
                                               VARIANT AttributeValue, LONG lFlags)
{
    HTMLElement *This = impl_from_IHTMLElement(iface);
    HRESULT hres;
    DISPID dispid, dispidNamed = DISPID_PROPERTYPUT;
    DISPPARAMS dispParams;
    EXCEPINFO excep;

    TRACE("(%p)->(%s %s %08x)\n", This, debugstr_w(strAttributeName), debugstr_variant(&AttributeValue), lFlags);

    hres = IDispatchEx_GetDispID(&This->node.dispex.IDispatchEx_iface, strAttributeName,
            fdexNameCaseInsensitive | fdexNameEnsure, &dispid);
    if(FAILED(hres))
        return hres;

    dispParams.cArgs = 1;
    dispParams.cNamedArgs = 1;
    dispParams.rgdispidNamedArgs = &dispidNamed;
    dispParams.rgvarg = &AttributeValue;

    hres = IDispatchEx_InvokeEx(&This->node.dispex.IDispatchEx_iface, dispid,
            LOCALE_SYSTEM_DEFAULT, DISPATCH_PROPERTYPUT, &dispParams, NULL, &excep, NULL);
    return hres;
}

static HRESULT WINAPI HTMLElement_getAttribute(IHTMLElement *iface, BSTR strAttributeName,
                                               LONG lFlags, VARIANT *AttributeValue)
{
    HTMLElement *This = impl_from_IHTMLElement(iface);
    DISPID dispid;
    HRESULT hres;
    DISPPARAMS dispParams = {NULL, NULL, 0, 0};
    EXCEPINFO excep;

    TRACE("(%p)->(%s %08x %p)\n", This, debugstr_w(strAttributeName), lFlags, AttributeValue);

    hres = IDispatchEx_GetDispID(&This->node.dispex.IDispatchEx_iface, strAttributeName,
            fdexNameCaseInsensitive, &dispid);
    if(hres == DISP_E_UNKNOWNNAME) {
        V_VT(AttributeValue) = VT_NULL;
        return S_OK;
    }

    if(FAILED(hres)) {
        V_VT(AttributeValue) = VT_NULL;
        return hres;
    }

    hres = IDispatchEx_InvokeEx(&This->node.dispex.IDispatchEx_iface, dispid, LOCALE_SYSTEM_DEFAULT,
            DISPATCH_PROPERTYGET, &dispParams, AttributeValue, &excep, NULL);

    return hres;
}

static HRESULT WINAPI HTMLElement_removeAttribute(IHTMLElement *iface, BSTR strAttributeName,
                                                  LONG lFlags, VARIANT_BOOL *pfSuccess)
{
    HTMLElement *This = impl_from_IHTMLElement(iface);

    TRACE("(%p)->(%s %x %p)\n", This, debugstr_w(strAttributeName), lFlags, pfSuccess);

    return remove_prop(&This->node.dispex, strAttributeName, pfSuccess);
}

static HRESULT WINAPI HTMLElement_put_className(IHTMLElement *iface, BSTR v)
{
    HTMLElement *This = impl_from_IHTMLElement(iface);
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
    HTMLElement *This = impl_from_IHTMLElement(iface);
    nsAString class_str;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    if(!This->nselem) {
        FIXME("NULL nselem\n");
        return E_NOTIMPL;
    }

    nsAString_Init(&class_str, NULL);
    nsres = nsIDOMHTMLElement_GetClassName(This->nselem, &class_str);
    return return_nsstr(nsres, &class_str, p);
}

static HRESULT WINAPI HTMLElement_put_id(IHTMLElement *iface, BSTR v)
{
    HTMLElement *This = impl_from_IHTMLElement(iface);
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
    HTMLElement *This = impl_from_IHTMLElement(iface);
    nsAString id_str;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    if(!This->nselem) {
        *p = NULL;
        return S_OK;
    }

    nsAString_Init(&id_str, NULL);
    nsres = nsIDOMHTMLElement_GetId(This->nselem, &id_str);
    return return_nsstr(nsres, &id_str, p);
}

static HRESULT WINAPI HTMLElement_get_tagName(IHTMLElement *iface, BSTR *p)
{
    HTMLElement *This = impl_from_IHTMLElement(iface);
    nsAString tag_str;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    if(!This->nselem) {
        static const WCHAR comment_tagW[] = {'!',0};

        WARN("NULL nselem, assuming comment\n");

        *p = SysAllocString(comment_tagW);
        return *p ? S_OK : E_OUTOFMEMORY;
    }

    nsAString_Init(&tag_str, NULL);
    nsres = nsIDOMHTMLElement_GetTagName(This->nselem, &tag_str);
    return return_nsstr(nsres, &tag_str, p);
}

static HRESULT WINAPI HTMLElement_get_parentElement(IHTMLElement *iface, IHTMLElement **p)
{
    HTMLElement *This = impl_from_IHTMLElement(iface);
    IHTMLDOMNode *node;
    HRESULT hres;

    TRACE("(%p)->(%p)\n", This, p);

    hres = IHTMLDOMNode_get_parentNode(&This->node.IHTMLDOMNode_iface, &node);
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
    HTMLElement *This = impl_from_IHTMLElement(iface);

    TRACE("(%p)->(%p)\n", This, p);

    if(!This->style) {
        HRESULT hres;

        hres = HTMLStyle_Create(This, &This->style);
        if(FAILED(hres))
            return hres;
    }

    *p = &This->style->IHTMLStyle_iface;
    IHTMLStyle_AddRef(*p);
    return S_OK;
}

static HRESULT WINAPI HTMLElement_put_onhelp(IHTMLElement *iface, VARIANT v)
{
    HTMLElement *This = impl_from_IHTMLElement(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_get_onhelp(IHTMLElement *iface, VARIANT *p)
{
    HTMLElement *This = impl_from_IHTMLElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_put_onclick(IHTMLElement *iface, VARIANT v)
{
    HTMLElement *This = impl_from_IHTMLElement(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_node_event(&This->node, EVENTID_CLICK, &v);
}

static HRESULT WINAPI HTMLElement_get_onclick(IHTMLElement *iface, VARIANT *p)
{
    HTMLElement *This = impl_from_IHTMLElement(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_node_event(&This->node, EVENTID_CLICK, p);
}

static HRESULT WINAPI HTMLElement_put_ondblclick(IHTMLElement *iface, VARIANT v)
{
    HTMLElement *This = impl_from_IHTMLElement(iface);

    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_node_event(&This->node, EVENTID_DBLCLICK, &v);
}

static HRESULT WINAPI HTMLElement_get_ondblclick(IHTMLElement *iface, VARIANT *p)
{
    HTMLElement *This = impl_from_IHTMLElement(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_node_event(&This->node, EVENTID_DBLCLICK, p);
}

static HRESULT WINAPI HTMLElement_put_onkeydown(IHTMLElement *iface, VARIANT v)
{
    HTMLElement *This = impl_from_IHTMLElement(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_node_event(&This->node, EVENTID_KEYDOWN, &v);
}

static HRESULT WINAPI HTMLElement_get_onkeydown(IHTMLElement *iface, VARIANT *p)
{
    HTMLElement *This = impl_from_IHTMLElement(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_node_event(&This->node, EVENTID_KEYDOWN, p);
}

static HRESULT WINAPI HTMLElement_put_onkeyup(IHTMLElement *iface, VARIANT v)
{
    HTMLElement *This = impl_from_IHTMLElement(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_node_event(&This->node, EVENTID_KEYUP, &v);
}

static HRESULT WINAPI HTMLElement_get_onkeyup(IHTMLElement *iface, VARIANT *p)
{
    HTMLElement *This = impl_from_IHTMLElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_put_onkeypress(IHTMLElement *iface, VARIANT v)
{
    HTMLElement *This = impl_from_IHTMLElement(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_node_event(&This->node, EVENTID_KEYPRESS, &v);
}

static HRESULT WINAPI HTMLElement_get_onkeypress(IHTMLElement *iface, VARIANT *p)
{
    HTMLElement *This = impl_from_IHTMLElement(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_node_event(&This->node, EVENTID_KEYPRESS, p);
}

static HRESULT WINAPI HTMLElement_put_onmouseout(IHTMLElement *iface, VARIANT v)
{
    HTMLElement *This = impl_from_IHTMLElement(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_node_event(&This->node, EVENTID_MOUSEOUT, &v);
}

static HRESULT WINAPI HTMLElement_get_onmouseout(IHTMLElement *iface, VARIANT *p)
{
    HTMLElement *This = impl_from_IHTMLElement(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_node_event(&This->node, EVENTID_MOUSEOUT, p);
}

static HRESULT WINAPI HTMLElement_put_onmouseover(IHTMLElement *iface, VARIANT v)
{
    HTMLElement *This = impl_from_IHTMLElement(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_node_event(&This->node, EVENTID_MOUSEOVER, &v);
}

static HRESULT WINAPI HTMLElement_get_onmouseover(IHTMLElement *iface, VARIANT *p)
{
    HTMLElement *This = impl_from_IHTMLElement(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_node_event(&This->node, EVENTID_MOUSEOVER, p);
}

static HRESULT WINAPI HTMLElement_put_onmousemove(IHTMLElement *iface, VARIANT v)
{
    HTMLElement *This = impl_from_IHTMLElement(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_node_event(&This->node, EVENTID_MOUSEMOVE, &v);
}

static HRESULT WINAPI HTMLElement_get_onmousemove(IHTMLElement *iface, VARIANT *p)
{
    HTMLElement *This = impl_from_IHTMLElement(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_node_event(&This->node, EVENTID_MOUSEMOVE, p);
}

static HRESULT WINAPI HTMLElement_put_onmousedown(IHTMLElement *iface, VARIANT v)
{
    HTMLElement *This = impl_from_IHTMLElement(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_node_event(&This->node, EVENTID_MOUSEDOWN, &v);
}

static HRESULT WINAPI HTMLElement_get_onmousedown(IHTMLElement *iface, VARIANT *p)
{
    HTMLElement *This = impl_from_IHTMLElement(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_node_event(&This->node, EVENTID_MOUSEDOWN, p);
}

static HRESULT WINAPI HTMLElement_put_onmouseup(IHTMLElement *iface, VARIANT v)
{
    HTMLElement *This = impl_from_IHTMLElement(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_node_event(&This->node, EVENTID_MOUSEUP, &v);
}

static HRESULT WINAPI HTMLElement_get_onmouseup(IHTMLElement *iface, VARIANT *p)
{
    HTMLElement *This = impl_from_IHTMLElement(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_node_event(&This->node, EVENTID_MOUSEUP, p);
}

static HRESULT WINAPI HTMLElement_get_document(IHTMLElement *iface, IDispatch **p)
{
    HTMLElement *This = impl_from_IHTMLElement(iface);

    TRACE("(%p)->(%p)\n", This, p);

    if(!p)
        return E_POINTER;

    if(This->node.vtbl->get_document)
        return This->node.vtbl->get_document(&This->node, p);

    *p = (IDispatch*)&This->node.doc->basedoc.IHTMLDocument2_iface;
    IDispatch_AddRef(*p);
    return S_OK;
}

static const WCHAR titleW[] = {'t','i','t','l','e',0};

static HRESULT WINAPI HTMLElement_put_title(IHTMLElement *iface, BSTR v)
{
    HTMLElement *This = impl_from_IHTMLElement(iface);
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
    HTMLElement *This = impl_from_IHTMLElement(iface);
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
    return return_nsstr(nsres, &title_str, p);
}

static HRESULT WINAPI HTMLElement_put_language(IHTMLElement *iface, BSTR v)
{
    HTMLElement *This = impl_from_IHTMLElement(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_get_language(IHTMLElement *iface, BSTR *p)
{
    HTMLElement *This = impl_from_IHTMLElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_put_onselectstart(IHTMLElement *iface, VARIANT v)
{
    HTMLElement *This = impl_from_IHTMLElement(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_node_event(&This->node, EVENTID_SELECTSTART, &v);
}

static HRESULT WINAPI HTMLElement_get_onselectstart(IHTMLElement *iface, VARIANT *p)
{
    HTMLElement *This = impl_from_IHTMLElement(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_node_event(&This->node, EVENTID_SELECTSTART, p);
}

static HRESULT WINAPI HTMLElement_scrollIntoView(IHTMLElement *iface, VARIANT varargStart)
{
    HTMLElement *This = impl_from_IHTMLElement(iface);
    cpp_bool start = TRUE;
    nsresult nsres;

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&varargStart));

    switch(V_VT(&varargStart)) {
    case VT_EMPTY:
    case VT_ERROR:
	break;
    case VT_BOOL:
	start = V_BOOL(&varargStart) != VARIANT_FALSE;
	break;
    default:
	FIXME("Unsupported argument %s\n", debugstr_variant(&varargStart));
    }

    if(!This->nselem) {
	FIXME("Unsupported for comments\n");
	return E_NOTIMPL;
    }

    nsres = nsIDOMHTMLElement_ScrollIntoView(This->nselem, start, 1);
    assert(nsres == NS_OK);

    return S_OK;
}

static HRESULT WINAPI HTMLElement_contains(IHTMLElement *iface, IHTMLElement *pChild,
                                           VARIANT_BOOL *pfResult)
{
    HTMLElement *This = impl_from_IHTMLElement(iface);
    cpp_bool result = FALSE;

    TRACE("(%p)->(%p %p)\n", This, pChild, pfResult);

    if(pChild) {
        HTMLElement *child;
        nsresult nsres;

        child = unsafe_impl_from_IHTMLElement(pChild);
        if(!child) {
            ERR("not our element\n");
            return E_FAIL;
        }

        nsres = nsIDOMNode_Contains(This->node.nsnode, child->node.nsnode, &result);
        assert(nsres == NS_OK);
    }

    *pfResult = result ? VARIANT_TRUE : VARIANT_FALSE;
    return S_OK;
}

static HRESULT WINAPI HTMLElement_get_sourceIndex(IHTMLElement *iface, LONG *p)
{
    HTMLElement *This = impl_from_IHTMLElement(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_elem_source_index(This, p);
}

static HRESULT WINAPI HTMLElement_get_recordNumber(IHTMLElement *iface, VARIANT *p)
{
    HTMLElement *This = impl_from_IHTMLElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_put_lang(IHTMLElement *iface, BSTR v)
{
    HTMLElement *This = impl_from_IHTMLElement(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_get_lang(IHTMLElement *iface, BSTR *p)
{
    HTMLElement *This = impl_from_IHTMLElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_get_offsetLeft(IHTMLElement *iface, LONG *p)
{
    HTMLElement *This = impl_from_IHTMLElement(iface);
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsres = nsIDOMHTMLElement_GetOffsetLeft(This->nselem, p);
    if(NS_FAILED(nsres)) {
        ERR("GetOffsetLeft failed: %08x\n", nsres);
        return E_FAIL;
    }

    return S_OK;
}

static HRESULT WINAPI HTMLElement_get_offsetTop(IHTMLElement *iface, LONG *p)
{
    HTMLElement *This = impl_from_IHTMLElement(iface);
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsres = nsIDOMHTMLElement_GetOffsetTop(This->nselem, p);
    if(NS_FAILED(nsres)) {
        ERR("GetOffsetTop failed: %08x\n", nsres);
        return E_FAIL;
    }

    return S_OK;
}

static HRESULT WINAPI HTMLElement_get_offsetWidth(IHTMLElement *iface, LONG *p)
{
    HTMLElement *This = impl_from_IHTMLElement(iface);
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsres = nsIDOMHTMLElement_GetOffsetWidth(This->nselem, p);
    if(NS_FAILED(nsres)) {
        ERR("GetOffsetWidth failed: %08x\n", nsres);
        return E_FAIL;
    }

    return S_OK;
}

static HRESULT WINAPI HTMLElement_get_offsetHeight(IHTMLElement *iface, LONG *p)
{
    HTMLElement *This = impl_from_IHTMLElement(iface);
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsres = nsIDOMHTMLElement_GetOffsetHeight(This->nselem, p);
    if(NS_FAILED(nsres)) {
        ERR("GetOffsetHeight failed: %08x\n", nsres);
        return E_FAIL;
    }

    return S_OK;
}

static HRESULT WINAPI HTMLElement_get_offsetParent(IHTMLElement *iface, IHTMLElement **p)
{
    HTMLElement *This = impl_from_IHTMLElement(iface);
    nsIDOMElement *nsparent;
    nsresult nsres;
    HRESULT hres;

    TRACE("(%p)->(%p)\n", This, p);

    nsres = nsIDOMHTMLElement_GetOffsetParent(This->nselem, &nsparent);
    if(NS_FAILED(nsres)) {
        ERR("GetOffsetParent failed: %08x\n", nsres);
        return E_FAIL;
    }

    if(nsparent) {
        HTMLDOMNode *node;

        hres = get_node(This->node.doc, (nsIDOMNode*)nsparent, TRUE, &node);
        nsIDOMElement_Release(nsparent);
        if(FAILED(hres))
            return hres;

        hres = IHTMLDOMNode_QueryInterface(&node->IHTMLDOMNode_iface, &IID_IHTMLElement, (void**)p);
        node_release(node);
    }else {
        *p = NULL;
        hres = S_OK;
    }

    return hres;
}

static HRESULT WINAPI HTMLElement_put_innerHTML(IHTMLElement *iface, BSTR v)
{
    HTMLElement *This = impl_from_IHTMLElement(iface);
    nsAString html_str;
    nsresult nsres;

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    if(!This->nselem) {
        FIXME("NULL nselem\n");
        return E_NOTIMPL;
    }

    nsAString_InitDepend(&html_str, v);
    nsres = nsIDOMHTMLElement_SetInnerHTML(This->nselem, &html_str);
    nsAString_Finish(&html_str);
    if(NS_FAILED(nsres)) {
        FIXME("SetInnerHtml failed %08x\n", nsres);
        return E_FAIL;
    }

    return S_OK;
}

static HRESULT WINAPI HTMLElement_get_innerHTML(IHTMLElement *iface, BSTR *p)
{
    HTMLElement *This = impl_from_IHTMLElement(iface);
    nsAString html_str;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    if(!This->nselem) {
        FIXME("NULL nselem\n");
        return E_NOTIMPL;
    }

    nsAString_Init(&html_str, NULL);
    nsres = nsIDOMHTMLElement_GetInnerHTML(This->nselem, &html_str);
    return return_nsstr(nsres, &html_str, p);
}

static HRESULT WINAPI HTMLElement_put_innerText(IHTMLElement *iface, BSTR v)
{
    HTMLElement *This = impl_from_IHTMLElement(iface);
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
    HTMLElement *This = impl_from_IHTMLElement(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_node_text(&This->node, p);
}

static HRESULT WINAPI HTMLElement_put_outerHTML(IHTMLElement *iface, BSTR v)
{
    HTMLElement *This = impl_from_IHTMLElement(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    return replace_node_by_html(This->node.doc->nsdoc, This->node.nsnode, v);
}

static HRESULT WINAPI HTMLElement_get_outerHTML(IHTMLElement *iface, BSTR *p)
{
    HTMLElement *This = impl_from_IHTMLElement(iface);
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
    return hres;
}

static HRESULT WINAPI HTMLElement_put_outerText(IHTMLElement *iface, BSTR v)
{
    HTMLElement *This = impl_from_IHTMLElement(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_get_outerText(IHTMLElement *iface, BSTR *p)
{
    HTMLElement *This = impl_from_IHTMLElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

HRESULT insert_adjacent_node(HTMLElement *This, const WCHAR *where, nsIDOMNode *nsnode, HTMLDOMNode **ret_node)
{
    nsIDOMNode *ret_nsnode;
    nsresult nsres;
    HRESULT hres = S_OK;

    static const WCHAR beforebeginW[] = {'b','e','f','o','r','e','b','e','g','i','n',0};
    static const WCHAR afterbeginW[] = {'a','f','t','e','r','b','e','g','i','n',0};
    static const WCHAR beforeendW[] = {'b','e','f','o','r','e','e','n','d',0};
    static const WCHAR afterendW[] = {'a','f','t','e','r','e','n','d',0};

    if (!strcmpiW(where, beforebeginW)) {
        nsIDOMNode *parent;

        nsres = nsIDOMNode_GetParentNode(This->node.nsnode, &parent);
        if(NS_FAILED(nsres))
            return E_FAIL;

        if(!parent)
            return E_INVALIDARG;

        nsres = nsIDOMNode_InsertBefore(parent, nsnode, This->node.nsnode, &ret_nsnode);
        nsIDOMNode_Release(parent);
    }else if(!strcmpiW(where, afterbeginW)) {
        nsIDOMNode *first_child;

        nsres = nsIDOMNode_GetFirstChild(This->node.nsnode, &first_child);
        if(NS_FAILED(nsres))
            return E_FAIL;

        nsres = nsIDOMNode_InsertBefore(This->node.nsnode, nsnode, first_child, &ret_nsnode);
        if(NS_FAILED(nsres))
            return E_FAIL;

        if (first_child)
            nsIDOMNode_Release(first_child);
    }else if (!strcmpiW(where, beforeendW)) {
        nsres = nsIDOMNode_AppendChild(This->node.nsnode, nsnode, &ret_nsnode);
    }else if (!strcmpiW(where, afterendW)) {
        nsIDOMNode *next_sibling, *parent;

        nsres = nsIDOMNode_GetParentNode(This->node.nsnode, &parent);
        if(NS_FAILED(nsres))
            return E_FAIL;
        if(!parent)
            return E_INVALIDARG;

        nsres = nsIDOMNode_GetNextSibling(This->node.nsnode, &next_sibling);
        if(NS_SUCCEEDED(nsres)) {
            if(next_sibling) {
                nsres = nsIDOMNode_InsertBefore(parent, nsnode, next_sibling, &ret_nsnode);
                nsIDOMNode_Release(next_sibling);
            }else {
                nsres = nsIDOMNode_AppendChild(parent, nsnode, &ret_nsnode);
            }
        }

        nsIDOMNode_Release(parent);
    }else {
        ERR("invalid where: %s\n", debugstr_w(where));
        return E_INVALIDARG;
    }

    if (NS_FAILED(nsres))
        return E_FAIL;

    if(ret_node)
        hres = get_node(This->node.doc, ret_nsnode, TRUE, ret_node);
    nsIDOMNode_Release(ret_nsnode);
    return hres;
}

static HRESULT WINAPI HTMLElement_insertAdjacentHTML(IHTMLElement *iface, BSTR where,
                                                     BSTR html)
{
    HTMLElement *This = impl_from_IHTMLElement(iface);
    nsIDOMRange *range;
    nsIDOMNode *nsnode;
    nsAString ns_html;
    nsresult nsres;
    HRESULT hr;

    TRACE("(%p)->(%s %s)\n", This, debugstr_w(where), debugstr_w(html));

    if(!This->node.doc->nsdoc) {
        WARN("NULL nsdoc\n");
        return E_UNEXPECTED;
    }

    nsres = nsIDOMHTMLDocument_CreateRange(This->node.doc->nsdoc, &range);
    if(NS_FAILED(nsres))
    {
        ERR("CreateRange failed: %08x\n", nsres);
        return E_FAIL;
    }

    nsIDOMRange_SetStartBefore(range, This->node.nsnode);

    nsAString_InitDepend(&ns_html, html);
    nsres = nsIDOMRange_CreateContextualFragment(range, &ns_html, (nsIDOMDocumentFragment **)&nsnode);
    nsAString_Finish(&ns_html);
    nsIDOMRange_Release(range);

    if(NS_FAILED(nsres) || !nsnode)
    {
        ERR("CreateTextNode failed: %08x\n", nsres);
        return E_FAIL;
    }

    hr = insert_adjacent_node(This, where, nsnode, NULL);
    nsIDOMNode_Release(nsnode);
    return hr;
}

static HRESULT WINAPI HTMLElement_insertAdjacentText(IHTMLElement *iface, BSTR where,
                                                     BSTR text)
{
    HTMLElement *This = impl_from_IHTMLElement(iface);
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
    nsres = nsIDOMHTMLDocument_CreateTextNode(This->node.doc->nsdoc, &ns_text, (nsIDOMText **)&nsnode);
    nsAString_Finish(&ns_text);

    if(NS_FAILED(nsres) || !nsnode)
    {
        ERR("CreateTextNode failed: %08x\n", nsres);
        return E_FAIL;
    }

    hr = insert_adjacent_node(This, where, nsnode, NULL);
    nsIDOMNode_Release(nsnode);

    return hr;
}

static HRESULT WINAPI HTMLElement_get_parentTextEdit(IHTMLElement *iface, IHTMLElement **p)
{
    HTMLElement *This = impl_from_IHTMLElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_get_isTextEdit(IHTMLElement *iface, VARIANT_BOOL *p)
{
    HTMLElement *This = impl_from_IHTMLElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_click(IHTMLElement *iface)
{
    HTMLElement *This = impl_from_IHTMLElement(iface);

    TRACE("(%p)\n", This);

    return call_fire_event(&This->node, EVENTID_CLICK);
}

static HRESULT WINAPI HTMLElement_get_filters(IHTMLElement *iface,
                                              IHTMLFiltersCollection **p)
{
    HTMLElement *This = impl_from_IHTMLElement(iface);
    TRACE("(%p)->(%p)\n", This, p);

    if(!p)
        return E_POINTER;

    *p = HTMLFiltersCollection_Create();

    return S_OK;
}

static HRESULT WINAPI HTMLElement_put_ondragstart(IHTMLElement *iface, VARIANT v)
{
    HTMLElement *This = impl_from_IHTMLElement(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_get_ondragstart(IHTMLElement *iface, VARIANT *p)
{
    HTMLElement *This = impl_from_IHTMLElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_toString(IHTMLElement *iface, BSTR *String)
{
    HTMLElement *This = impl_from_IHTMLElement(iface);
    FIXME("(%p)->(%p)\n", This, String);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_put_onbeforeupdate(IHTMLElement *iface, VARIANT v)
{
    HTMLElement *This = impl_from_IHTMLElement(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_get_onbeforeupdate(IHTMLElement *iface, VARIANT *p)
{
    HTMLElement *This = impl_from_IHTMLElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_put_onafterupdate(IHTMLElement *iface, VARIANT v)
{
    HTMLElement *This = impl_from_IHTMLElement(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_get_onafterupdate(IHTMLElement *iface, VARIANT *p)
{
    HTMLElement *This = impl_from_IHTMLElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_put_onerrorupdate(IHTMLElement *iface, VARIANT v)
{
    HTMLElement *This = impl_from_IHTMLElement(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_get_onerrorupdate(IHTMLElement *iface, VARIANT *p)
{
    HTMLElement *This = impl_from_IHTMLElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_put_onrowexit(IHTMLElement *iface, VARIANT v)
{
    HTMLElement *This = impl_from_IHTMLElement(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_get_onrowexit(IHTMLElement *iface, VARIANT *p)
{
    HTMLElement *This = impl_from_IHTMLElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_put_onrowenter(IHTMLElement *iface, VARIANT v)
{
    HTMLElement *This = impl_from_IHTMLElement(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_get_onrowenter(IHTMLElement *iface, VARIANT *p)
{
    HTMLElement *This = impl_from_IHTMLElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_put_ondatasetchanged(IHTMLElement *iface, VARIANT v)
{
    HTMLElement *This = impl_from_IHTMLElement(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_get_ondatasetchanged(IHTMLElement *iface, VARIANT *p)
{
    HTMLElement *This = impl_from_IHTMLElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_put_ondataavailable(IHTMLElement *iface, VARIANT v)
{
    HTMLElement *This = impl_from_IHTMLElement(iface);

    FIXME("(%p)->(%s) semi-stub\n", This, debugstr_variant(&v));

    return set_node_event(&This->node, EVENTID_DATAAVAILABLE, &v);
}

static HRESULT WINAPI HTMLElement_get_ondataavailable(IHTMLElement *iface, VARIANT *p)
{
    HTMLElement *This = impl_from_IHTMLElement(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_node_event(&This->node, EVENTID_DATAAVAILABLE, p);
}

static HRESULT WINAPI HTMLElement_put_ondatasetcomplete(IHTMLElement *iface, VARIANT v)
{
    HTMLElement *This = impl_from_IHTMLElement(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_get_ondatasetcomplete(IHTMLElement *iface, VARIANT *p)
{
    HTMLElement *This = impl_from_IHTMLElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_put_onfilterchange(IHTMLElement *iface, VARIANT v)
{
    HTMLElement *This = impl_from_IHTMLElement(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_get_onfilterchange(IHTMLElement *iface, VARIANT *p)
{
    HTMLElement *This = impl_from_IHTMLElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement_get_children(IHTMLElement *iface, IDispatch **p)
{
    HTMLElement *This = impl_from_IHTMLElement(iface);
    nsIDOMNodeList *nsnode_list;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsres = nsIDOMNode_GetChildNodes(This->node.nsnode, &nsnode_list);
    if(NS_FAILED(nsres)) {
        ERR("GetChildNodes failed: %08x\n", nsres);
        return E_FAIL;
    }

    *p = (IDispatch*)create_collection_from_nodelist(This->node.doc, nsnode_list);

    nsIDOMNodeList_Release(nsnode_list);
    return S_OK;
}

static HRESULT WINAPI HTMLElement_get_all(IHTMLElement *iface, IDispatch **p)
{
    HTMLElement *This = impl_from_IHTMLElement(iface);

    TRACE("(%p)->(%p)\n", This, p);

    *p = (IDispatch*)create_all_collection(&This->node, FALSE);
    return S_OK;
}

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

HTMLElement *unsafe_impl_from_IHTMLElement(IHTMLElement *iface)
{
    return iface->lpVtbl == &HTMLElementVtbl ? impl_from_IHTMLElement(iface) : NULL;
}

static inline HTMLElement *impl_from_HTMLDOMNode(HTMLDOMNode *iface)
{
    return CONTAINING_RECORD(iface, HTMLElement, node);
}

HRESULT HTMLElement_QI(HTMLDOMNode *iface, REFIID riid, void **ppv)
{
    HTMLElement *This = impl_from_HTMLDOMNode(iface);

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        *ppv = &This->IHTMLElement_iface;
    }else if(IsEqualGUID(&IID_IDispatch, riid)) {
        *ppv = &This->IHTMLElement_iface;
    }else if(IsEqualGUID(&IID_IHTMLElement, riid)) {
        *ppv = &This->IHTMLElement_iface;
    }else if(IsEqualGUID(&IID_IHTMLElement2, riid)) {
        *ppv = &This->IHTMLElement2_iface;
    }else if(IsEqualGUID(&IID_IHTMLElement3, riid)) {
        *ppv = &This->IHTMLElement3_iface;
    }else if(IsEqualGUID(&IID_IHTMLElement4, riid)) {
        *ppv = &This->IHTMLElement4_iface;
    }else if(IsEqualGUID(&IID_IConnectionPointContainer, riid)) {
        *ppv = &This->cp_container.IConnectionPointContainer_iface;
    }else {
        return HTMLDOMNode_QI(&This->node, riid, ppv);
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

void HTMLElement_destructor(HTMLDOMNode *iface)
{
    HTMLElement *This = impl_from_HTMLDOMNode(iface);

    ConnectionPointContainer_Destroy(&This->cp_container);

    if(This->style) {
        This->style->elem = NULL;
        IHTMLStyle_Release(&This->style->IHTMLStyle_iface);
    }
    if(This->runtime_style) {
        This->runtime_style->elem = NULL;
        IHTMLStyle_Release(&This->runtime_style->IHTMLStyle_iface);
    }
    if(This->attrs) {
        HTMLDOMAttribute *attr;

        LIST_FOR_EACH_ENTRY(attr, &This->attrs->attrs, HTMLDOMAttribute, entry)
            attr->elem = NULL;

        This->attrs->elem = NULL;
        IHTMLAttributeCollection_Release(&This->attrs->IHTMLAttributeCollection_iface);
    }

    heap_free(This->filter);

    HTMLDOMNode_destructor(&This->node);
}

HRESULT HTMLElement_clone(HTMLDOMNode *iface, nsIDOMNode *nsnode, HTMLDOMNode **ret)
{
    HTMLElement *This = impl_from_HTMLDOMNode(iface);
    HTMLElement *new_elem;
    HRESULT hres;

    hres = HTMLElement_Create(This->node.doc, nsnode, FALSE, &new_elem);
    if(FAILED(hres))
        return hres;

    if(This->filter) {
        new_elem->filter = heap_strdupW(This->filter);
        if(!new_elem->filter) {
            IHTMLElement_Release(&This->IHTMLElement_iface);
            return E_OUTOFMEMORY;
        }
    }

    *ret = &new_elem->node;
    return S_OK;
}

HRESULT HTMLElement_handle_event(HTMLDOMNode *iface, DWORD eid, nsIDOMEvent *event, BOOL *prevent_default)
{
    HTMLElement *This = impl_from_HTMLDOMNode(iface);

    switch(eid) {
    case EVENTID_KEYDOWN: {
        nsIDOMKeyEvent *key_event;
        nsresult nsres;

        nsres = nsIDOMEvent_QueryInterface(event, &IID_nsIDOMKeyEvent, (void**)&key_event);
        if(NS_SUCCEEDED(nsres)) {
            UINT32 code = 0;

            nsIDOMKeyEvent_GetKeyCode(key_event, &code);

            switch(code) {
            case VK_F1: /* DOM_VK_F1 */
                TRACE("F1 pressed\n");
                fire_event(This->node.doc, EVENTID_HELP, TRUE, This->node.nsnode, NULL, NULL);
                *prevent_default = TRUE;
            }

            nsIDOMKeyEvent_Release(key_event);
        }
    }
    }

    return S_OK;
}

cp_static_data_t HTMLElementEvents2_data = { HTMLElementEvents2_tid, NULL /* FIXME */, TRUE };

const cpc_entry_t HTMLElement_cpc[] = {
    HTMLELEMENT_CPC,
    {NULL}
};

static const NodeImplVtbl HTMLElementImplVtbl = {
    HTMLElement_QI,
    HTMLElement_destructor,
    HTMLElement_cpc,
    HTMLElement_clone,
    HTMLElement_handle_event,
    HTMLElement_get_attr_col
};

static inline HTMLElement *impl_from_DispatchEx(DispatchEx *iface)
{
    return CONTAINING_RECORD(iface, HTMLElement, node.dispex);
}

static HRESULT HTMLElement_get_dispid(DispatchEx *dispex, BSTR name,
        DWORD grfdex, DISPID *pid)
{
    HTMLElement *This = impl_from_DispatchEx(dispex);

    if(This->node.vtbl->get_dispid)
        return This->node.vtbl->get_dispid(&This->node, name, grfdex, pid);

    return DISP_E_UNKNOWNNAME;
}

static HRESULT HTMLElement_invoke(DispatchEx *dispex, DISPID id, LCID lcid,
        WORD flags, DISPPARAMS *params, VARIANT *res, EXCEPINFO *ei,
        IServiceProvider *caller)
{
    HTMLElement *This = impl_from_DispatchEx(dispex);

    if(This->node.vtbl->invoke)
        return This->node.vtbl->invoke(&This->node, id, lcid, flags,
                params, res, ei, caller);

    ERR("(%p): element has no invoke method\n", This);
    return E_NOTIMPL;
}

static HRESULT HTMLElement_populate_props(DispatchEx *dispex)
{
    HTMLElement *This = impl_from_DispatchEx(dispex);
    nsIDOMMozNamedAttrMap *attrs;
    nsIDOMAttr *attr;
    nsAString nsstr;
    const PRUnichar *str;
    BSTR name;
    VARIANT value;
    unsigned i;
    UINT32 len;
    DISPID id;
    nsresult nsres;
    HRESULT hres;

    if(!This->nselem)
        return S_FALSE;

    nsres = nsIDOMHTMLElement_GetAttributes(This->nselem, &attrs);
    if(NS_FAILED(nsres))
        return E_FAIL;

    nsres = nsIDOMMozNamedAttrMap_GetLength(attrs, &len);
    if(NS_FAILED(nsres)) {
        nsIDOMMozNamedAttrMap_Release(attrs);
        return E_FAIL;
    }

    nsAString_Init(&nsstr, NULL);
    for(i=0; i<len; i++) {
        nsres = nsIDOMMozNamedAttrMap_Item(attrs, i, &attr);
        if(NS_FAILED(nsres))
            continue;

        nsres = nsIDOMAttr_GetNodeName(attr, &nsstr);
        if(NS_FAILED(nsres)) {
            nsIDOMAttr_Release(attr);
            continue;
        }

        nsAString_GetData(&nsstr, &str);
        name = SysAllocString(str);
        if(!name) {
            nsIDOMAttr_Release(attr);
            continue;
        }

        hres = IDispatchEx_GetDispID(&dispex->IDispatchEx_iface, name, fdexNameCaseInsensitive, &id);
        if(hres != DISP_E_UNKNOWNNAME) {
            nsIDOMAttr_Release(attr);
            SysFreeString(name);
            continue;
        }

        nsres = nsIDOMAttr_GetNodeValue(attr, &nsstr);
        nsIDOMAttr_Release(attr);
        if(NS_FAILED(nsres)) {
            SysFreeString(name);
            continue;
        }

        nsAString_GetData(&nsstr, &str);
        V_VT(&value) = VT_BSTR;
        if(*str) {
            V_BSTR(&value) = SysAllocString(str);
            if(!V_BSTR(&value)) {
                SysFreeString(name);
                continue;
            }
        } else
            V_BSTR(&value) = NULL;

        IHTMLElement_setAttribute(&This->IHTMLElement_iface, name, value, 0);
        SysFreeString(name);
        VariantClear(&value);
    }
    nsAString_Finish(&nsstr);

    nsIDOMMozNamedAttrMap_Release(attrs);
    return S_OK;
}

static const tid_t HTMLElement_iface_tids[] = {
    HTMLELEMENT_TIDS,
    0
};

static dispex_static_data_vtbl_t HTMLElement_dispex_vtbl = {
    NULL,
    HTMLElement_get_dispid,
    HTMLElement_invoke,
    HTMLElement_populate_props
};

static dispex_static_data_t HTMLElement_dispex = {
    &HTMLElement_dispex_vtbl,
    DispHTMLUnknownElement_tid,
    NULL,
    HTMLElement_iface_tids
};

void HTMLElement_Init(HTMLElement *This, HTMLDocumentNode *doc, nsIDOMHTMLElement *nselem, dispex_static_data_t *dispex_data)
{
    This->IHTMLElement_iface.lpVtbl = &HTMLElementVtbl;

    HTMLElement2_Init(This);
    HTMLElement3_Init(This);

    if(dispex_data && !dispex_data->vtbl)
        dispex_data->vtbl = &HTMLElement_dispex_vtbl;
    init_dispex(&This->node.dispex, (IUnknown*)&This->IHTMLElement_iface,
            dispex_data ? dispex_data : &HTMLElement_dispex);

    if(nselem) {
        HTMLDOMNode_Init(doc, &This->node, (nsIDOMNode*)nselem);

        /* No AddRef, share reference with HTMLDOMNode */
        assert((nsIDOMNode*)nselem == This->node.nsnode);
        This->nselem = nselem;
    }

    This->node.cp_container = &This->cp_container;
    ConnectionPointContainer_Init(&This->cp_container, (IUnknown*)&This->IHTMLElement_iface, This->node.vtbl->cpc_entries);
}

HRESULT HTMLElement_Create(HTMLDocumentNode *doc, nsIDOMNode *nsnode, BOOL use_generic, HTMLElement **ret)
{
    nsIDOMHTMLElement *nselem;
    nsAString class_name_str;
    const PRUnichar *class_name;
    const tag_desc_t *tag;
    HTMLElement *elem;
    nsresult nsres;
    HRESULT hres;

    nsres = nsIDOMNode_QueryInterface(nsnode, &IID_nsIDOMHTMLElement, (void**)&nselem);
    if(NS_FAILED(nsres))
        return E_FAIL;

    nsAString_Init(&class_name_str, NULL);
    nsIDOMHTMLElement_GetTagName(nselem, &class_name_str);

    nsAString_GetData(&class_name_str, &class_name);

    tag = get_tag_desc(class_name);
    if(tag) {
        hres = tag->constructor(doc, nselem, &elem);
    }else if(use_generic) {
        hres = HTMLGenericElement_Create(doc, nselem, &elem);
    }else {
        elem = heap_alloc_zero(sizeof(HTMLElement));
        if(elem) {
            elem->node.vtbl = &HTMLElementImplVtbl;
            HTMLElement_Init(elem, doc, nselem, &HTMLElement_dispex);
            hres = S_OK;
        }else {
            hres = E_OUTOFMEMORY;
        }
    }

    TRACE("%s ret %p\n", debugstr_w(class_name), elem);

    nsIDOMHTMLElement_Release(nselem);
    nsAString_Finish(&class_name_str);
    if(FAILED(hres))
        return hres;

    *ret = elem;
    return S_OK;
}

HRESULT get_elem(HTMLDocumentNode *doc, nsIDOMElement *nselem, HTMLElement **ret)
{
    HTMLDOMNode *node;
    HRESULT hres;

    hres = get_node(doc, (nsIDOMNode*)nselem, TRUE, &node);
    if(FAILED(hres))
        return hres;

    *ret = impl_from_HTMLDOMNode(node);
    return S_OK;
}

/* interface IHTMLFiltersCollection */
static HRESULT WINAPI HTMLFiltersCollection_QueryInterface(IHTMLFiltersCollection *iface, REFIID riid, void **ppv)
{
    HTMLFiltersCollection *This = impl_from_IHTMLFiltersCollection(iface);

    TRACE("%p %s %p\n", This, debugstr_mshtml_guid(riid), ppv );

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        *ppv = &This->IHTMLFiltersCollection_iface;
    }else if(IsEqualGUID(&IID_IHTMLFiltersCollection, riid)) {
        TRACE("(%p)->(IID_IHTMLFiltersCollection %p)\n", This, ppv);
        *ppv = &This->IHTMLFiltersCollection_iface;
    }else if(dispex_query_interface(&This->dispex, riid, ppv)) {
        return *ppv ? S_OK : E_NOINTERFACE;
    }else {
        *ppv = NULL;
        FIXME("(%p)->(%s %p)\n", This, debugstr_mshtml_guid(riid), ppv);
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI HTMLFiltersCollection_AddRef(IHTMLFiltersCollection *iface)
{
    HTMLFiltersCollection *This = impl_from_IHTMLFiltersCollection(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    return ref;
}

static ULONG WINAPI HTMLFiltersCollection_Release(IHTMLFiltersCollection *iface)
{
    HTMLFiltersCollection *This = impl_from_IHTMLFiltersCollection(iface);
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
    HTMLFiltersCollection *This = impl_from_IHTMLFiltersCollection(iface);
    return IDispatchEx_GetTypeInfoCount(&This->dispex.IDispatchEx_iface, pctinfo);
}

static HRESULT WINAPI HTMLFiltersCollection_GetTypeInfo(IHTMLFiltersCollection *iface,
                                    UINT iTInfo, LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLFiltersCollection *This = impl_from_IHTMLFiltersCollection(iface);
    return IDispatchEx_GetTypeInfo(&This->dispex.IDispatchEx_iface, iTInfo, lcid, ppTInfo);
}

static HRESULT WINAPI HTMLFiltersCollection_GetIDsOfNames(IHTMLFiltersCollection *iface,
                                    REFIID riid, LPOLESTR *rgszNames, UINT cNames,
                                    LCID lcid, DISPID *rgDispId)
{
    HTMLFiltersCollection *This = impl_from_IHTMLFiltersCollection(iface);
    return IDispatchEx_GetIDsOfNames(&This->dispex.IDispatchEx_iface, riid, rgszNames, cNames,
            lcid, rgDispId);
}

static HRESULT WINAPI HTMLFiltersCollection_Invoke(IHTMLFiltersCollection *iface, DISPID dispIdMember, REFIID riid,
                                    LCID lcid, WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult,
                                    EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLFiltersCollection *This = impl_from_IHTMLFiltersCollection(iface);
    return IDispatchEx_Invoke(&This->dispex.IDispatchEx_iface, dispIdMember, riid, lcid,
            wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI HTMLFiltersCollection_get_length(IHTMLFiltersCollection *iface, LONG *p)
{
    HTMLFiltersCollection *This = impl_from_IHTMLFiltersCollection(iface);

    if(!p)
        return E_POINTER;

    FIXME("(%p)->(%p) Always returning 0\n", This, p);
    *p = 0;

    return S_OK;
}

static HRESULT WINAPI HTMLFiltersCollection_get__newEnum(IHTMLFiltersCollection *iface, IUnknown **p)
{
    HTMLFiltersCollection *This = impl_from_IHTMLFiltersCollection(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLFiltersCollection_item(IHTMLFiltersCollection *iface, VARIANT *pvarIndex, VARIANT *pvarResult)
{
    HTMLFiltersCollection *This = impl_from_IHTMLFiltersCollection(iface);
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

static HRESULT HTMLFiltersCollection_get_dispid(DispatchEx *dispex, BSTR name, DWORD flags, DISPID *dispid)
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

static HRESULT HTMLFiltersCollection_invoke(DispatchEx *dispex, DISPID id, LCID lcid, WORD flags, DISPPARAMS *params,
        VARIANT *res, EXCEPINFO *ei, IServiceProvider *caller)
{
    TRACE("(%p)->(%x %x %x %p %p %p)\n", dispex, id, lcid, flags, params, res, ei);

    V_VT(res) = VT_DISPATCH;
    V_DISPATCH(res) = NULL;

    FIXME("always returning NULL\n");

    return S_OK;
}

static const dispex_static_data_vtbl_t HTMLFiltersCollection_dispex_vtbl = {
    NULL,
    HTMLFiltersCollection_get_dispid,
    HTMLFiltersCollection_invoke,
    NULL
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

static IHTMLFiltersCollection *HTMLFiltersCollection_Create(void)
{
    HTMLFiltersCollection *ret = heap_alloc(sizeof(HTMLFiltersCollection));

    ret->IHTMLFiltersCollection_iface.lpVtbl = &HTMLFiltersCollectionVtbl;
    ret->ref = 1;

    init_dispex(&ret->dispex, (IUnknown*)&ret->IHTMLFiltersCollection_iface,
            &HTMLFiltersCollection_dispex);

    return &ret->IHTMLFiltersCollection_iface;
}

/* interface IHTMLAttributeCollection */
static inline HTMLAttributeCollection *impl_from_IHTMLAttributeCollection(IHTMLAttributeCollection *iface)
{
    return CONTAINING_RECORD(iface, HTMLAttributeCollection, IHTMLAttributeCollection_iface);
}

static HRESULT WINAPI HTMLAttributeCollection_QueryInterface(IHTMLAttributeCollection *iface, REFIID riid, void **ppv)
{
    HTMLAttributeCollection *This = impl_from_IHTMLAttributeCollection(iface);

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        *ppv = &This->IHTMLAttributeCollection_iface;
    }else if(IsEqualGUID(&IID_IHTMLAttributeCollection, riid)) {
        *ppv = &This->IHTMLAttributeCollection_iface;
    }else if(IsEqualGUID(&IID_IHTMLAttributeCollection2, riid)) {
        *ppv = &This->IHTMLAttributeCollection2_iface;
    }else if(IsEqualGUID(&IID_IHTMLAttributeCollection3, riid)) {
        *ppv = &This->IHTMLAttributeCollection3_iface;
    }else if(dispex_query_interface(&This->dispex, riid, ppv)) {
        return *ppv ? S_OK : E_NOINTERFACE;
    }else {
        *ppv = NULL;
        WARN("(%p)->(%s %p)\n", This, debugstr_mshtml_guid(riid), ppv);
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI HTMLAttributeCollection_AddRef(IHTMLAttributeCollection *iface)
{
    HTMLAttributeCollection *This = impl_from_IHTMLAttributeCollection(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    return ref;
}

static ULONG WINAPI HTMLAttributeCollection_Release(IHTMLAttributeCollection *iface)
{
    HTMLAttributeCollection *This = impl_from_IHTMLAttributeCollection(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    if(!ref) {
        while(!list_empty(&This->attrs)) {
            HTMLDOMAttribute *attr = LIST_ENTRY(list_head(&This->attrs), HTMLDOMAttribute, entry);

            list_remove(&attr->entry);
            attr->elem = NULL;
            IHTMLDOMAttribute_Release(&attr->IHTMLDOMAttribute_iface);
        }

        heap_free(This);
    }

    return ref;
}

static HRESULT WINAPI HTMLAttributeCollection_GetTypeInfoCount(IHTMLAttributeCollection *iface, UINT *pctinfo)
{
    HTMLAttributeCollection *This = impl_from_IHTMLAttributeCollection(iface);
    return IDispatchEx_GetTypeInfoCount(&This->dispex.IDispatchEx_iface, pctinfo);
}

static HRESULT WINAPI HTMLAttributeCollection_GetTypeInfo(IHTMLAttributeCollection *iface, UINT iTInfo,
        LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLAttributeCollection *This = impl_from_IHTMLAttributeCollection(iface);
    return IDispatchEx_GetTypeInfo(&This->dispex.IDispatchEx_iface, iTInfo, lcid, ppTInfo);
}

static HRESULT WINAPI HTMLAttributeCollection_GetIDsOfNames(IHTMLAttributeCollection *iface, REFIID riid,
        LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{
    HTMLAttributeCollection *This = impl_from_IHTMLAttributeCollection(iface);
    return IDispatchEx_GetIDsOfNames(&This->dispex.IDispatchEx_iface, riid, rgszNames, cNames,
            lcid, rgDispId);
}

static HRESULT WINAPI HTMLAttributeCollection_Invoke(IHTMLAttributeCollection *iface, DISPID dispIdMember,
        REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
        VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLAttributeCollection *This = impl_from_IHTMLAttributeCollection(iface);
    return IDispatchEx_Invoke(&This->dispex.IDispatchEx_iface, dispIdMember, riid, lcid,
            wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT get_attr_dispid_by_idx(HTMLAttributeCollection *This, LONG *idx, DISPID *dispid)
{
    IDispatchEx *dispex = &This->elem->node.dispex.IDispatchEx_iface;
    DISPID id = DISPID_STARTENUM;
    LONG len = -1;
    HRESULT hres;

    FIXME("filter non-enumerable attributes out\n");

    while(1) {
        hres = IDispatchEx_GetNextDispID(dispex, fdexEnumAll, id, &id);
        if(FAILED(hres))
            return hres;
        else if(hres == S_FALSE)
            break;

        len++;
        if(len == *idx)
            break;
    }

    if(dispid) {
        *dispid = id;
        return *idx==len ? S_OK : DISP_E_UNKNOWNNAME;
    }

    *idx = len+1;
    return S_OK;
}

static inline HRESULT get_attr_dispid_by_name(HTMLAttributeCollection *This, BSTR name, DISPID *id)
{
    HRESULT hres;

    if(name[0]>='0' && name[0]<='9') {
        WCHAR *end_ptr;
        LONG idx;

        idx = strtoulW(name, &end_ptr, 10);
        if(!*end_ptr) {
            hres = get_attr_dispid_by_idx(This, &idx, id);
            if(SUCCEEDED(hres))
                return hres;
        }
    }

    if(!This->elem) {
        WARN("NULL elem\n");
        return E_UNEXPECTED;
    }

    hres = IDispatchEx_GetDispID(&This->elem->node.dispex.IDispatchEx_iface,
            name, fdexNameCaseInsensitive, id);
    return hres;
}

static inline HRESULT get_domattr(HTMLAttributeCollection *This, DISPID id, LONG *list_pos, HTMLDOMAttribute **attr)
{
    HTMLDOMAttribute *iter;
    LONG pos = 0;
    HRESULT hres;

    *attr = NULL;
    LIST_FOR_EACH_ENTRY(iter, &This->attrs, HTMLDOMAttribute, entry) {
        if(iter->dispid == id) {
            *attr = iter;
            break;
        }
        pos++;
    }

    if(!*attr) {
        if(!This->elem) {
            WARN("NULL elem\n");
            return E_UNEXPECTED;
        }

        hres = HTMLDOMAttribute_Create(NULL, This->elem, id, attr);
        if(FAILED(hres))
            return hres;
    }

    IHTMLDOMAttribute_AddRef(&(*attr)->IHTMLDOMAttribute_iface);
    if(list_pos)
        *list_pos = pos;
    return S_OK;
}

static HRESULT WINAPI HTMLAttributeCollection_get_length(IHTMLAttributeCollection *iface, LONG *p)
{
    HTMLAttributeCollection *This = impl_from_IHTMLAttributeCollection(iface);
    HRESULT hres;

    TRACE("(%p)->(%p)\n", This, p);

    *p = -1;
    hres = get_attr_dispid_by_idx(This, p, NULL);
    return hres;
}

static HRESULT WINAPI HTMLAttributeCollection__newEnum(IHTMLAttributeCollection *iface, IUnknown **p)
{
    HTMLAttributeCollection *This = impl_from_IHTMLAttributeCollection(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLAttributeCollection_item(IHTMLAttributeCollection *iface, VARIANT *name, IDispatch **ppItem)
{
    HTMLAttributeCollection *This = impl_from_IHTMLAttributeCollection(iface);
    HTMLDOMAttribute *attr;
    DISPID id;
    HRESULT hres;

    TRACE("(%p)->(%s %p)\n", This, debugstr_variant(name), ppItem);

    switch(V_VT(name)) {
    case VT_I4:
        hres = get_attr_dispid_by_idx(This, &V_I4(name), &id);
        break;
    case VT_BSTR:
        hres = get_attr_dispid_by_name(This, V_BSTR(name), &id);
        break;
    default:
        FIXME("unsupported name %s\n", debugstr_variant(name));
        hres = E_NOTIMPL;
    }
    if(hres == DISP_E_UNKNOWNNAME)
        return E_INVALIDARG;
    if(FAILED(hres))
        return hres;

    hres = get_domattr(This, id, NULL, &attr);
    if(FAILED(hres))
        return hres;

    *ppItem = (IDispatch*)&attr->IHTMLDOMAttribute_iface;
    return S_OK;
}

static const IHTMLAttributeCollectionVtbl HTMLAttributeCollectionVtbl = {
    HTMLAttributeCollection_QueryInterface,
    HTMLAttributeCollection_AddRef,
    HTMLAttributeCollection_Release,
    HTMLAttributeCollection_GetTypeInfoCount,
    HTMLAttributeCollection_GetTypeInfo,
    HTMLAttributeCollection_GetIDsOfNames,
    HTMLAttributeCollection_Invoke,
    HTMLAttributeCollection_get_length,
    HTMLAttributeCollection__newEnum,
    HTMLAttributeCollection_item
};

static inline HTMLAttributeCollection *impl_from_IHTMLAttributeCollection2(IHTMLAttributeCollection2 *iface)
{
    return CONTAINING_RECORD(iface, HTMLAttributeCollection, IHTMLAttributeCollection2_iface);
}

static HRESULT WINAPI HTMLAttributeCollection2_QueryInterface(IHTMLAttributeCollection2 *iface, REFIID riid, void **ppv)
{
    HTMLAttributeCollection *This = impl_from_IHTMLAttributeCollection2(iface);
    return IHTMLAttributeCollection_QueryInterface(&This->IHTMLAttributeCollection_iface, riid, ppv);
}

static ULONG WINAPI HTMLAttributeCollection2_AddRef(IHTMLAttributeCollection2 *iface)
{
    HTMLAttributeCollection *This = impl_from_IHTMLAttributeCollection2(iface);
    return IHTMLAttributeCollection_AddRef(&This->IHTMLAttributeCollection_iface);
}

static ULONG WINAPI HTMLAttributeCollection2_Release(IHTMLAttributeCollection2 *iface)
{
    HTMLAttributeCollection *This = impl_from_IHTMLAttributeCollection2(iface);
    return IHTMLAttributeCollection_Release(&This->IHTMLAttributeCollection_iface);
}

static HRESULT WINAPI HTMLAttributeCollection2_GetTypeInfoCount(IHTMLAttributeCollection2 *iface, UINT *pctinfo)
{
    HTMLAttributeCollection *This = impl_from_IHTMLAttributeCollection2(iface);
    return IDispatchEx_GetTypeInfoCount(&This->dispex.IDispatchEx_iface, pctinfo);
}

static HRESULT WINAPI HTMLAttributeCollection2_GetTypeInfo(IHTMLAttributeCollection2 *iface, UINT iTInfo,
        LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLAttributeCollection *This = impl_from_IHTMLAttributeCollection2(iface);
    return IDispatchEx_GetTypeInfo(&This->dispex.IDispatchEx_iface, iTInfo, lcid, ppTInfo);
}

static HRESULT WINAPI HTMLAttributeCollection2_GetIDsOfNames(IHTMLAttributeCollection2 *iface, REFIID riid,
        LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{
    HTMLAttributeCollection *This = impl_from_IHTMLAttributeCollection2(iface);
    return IDispatchEx_GetIDsOfNames(&This->dispex.IDispatchEx_iface, riid, rgszNames, cNames,
            lcid, rgDispId);
}

static HRESULT WINAPI HTMLAttributeCollection2_Invoke(IHTMLAttributeCollection2 *iface, DISPID dispIdMember,
        REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
        VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLAttributeCollection *This = impl_from_IHTMLAttributeCollection2(iface);
    return IDispatchEx_Invoke(&This->dispex.IDispatchEx_iface, dispIdMember, riid, lcid,
            wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI HTMLAttributeCollection2_getNamedItem(IHTMLAttributeCollection2 *iface, BSTR bstrName,
        IHTMLDOMAttribute **newretNode)
{
    HTMLAttributeCollection *This = impl_from_IHTMLAttributeCollection2(iface);
    HTMLDOMAttribute *attr;
    DISPID id;
    HRESULT hres;

    TRACE("(%p)->(%s %p)\n", This, debugstr_w(bstrName), newretNode);

    hres = get_attr_dispid_by_name(This, bstrName, &id);
    if(hres == DISP_E_UNKNOWNNAME) {
        *newretNode = NULL;
        return S_OK;
    } else if(FAILED(hres)) {
        return hres;
    }

    hres = get_domattr(This, id, NULL, &attr);
    if(FAILED(hres))
        return hres;

    *newretNode = &attr->IHTMLDOMAttribute_iface;
    return S_OK;
}

static HRESULT WINAPI HTMLAttributeCollection2_setNamedItem(IHTMLAttributeCollection2 *iface,
        IHTMLDOMAttribute *ppNode, IHTMLDOMAttribute **newretNode)
{
    HTMLAttributeCollection *This = impl_from_IHTMLAttributeCollection2(iface);
    FIXME("(%p)->(%p %p)\n", This, ppNode, newretNode);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLAttributeCollection2_removeNamedItem(IHTMLAttributeCollection2 *iface,
        BSTR bstrName, IHTMLDOMAttribute **newretNode)
{
    HTMLAttributeCollection *This = impl_from_IHTMLAttributeCollection2(iface);
    FIXME("(%p)->(%s %p)\n", This, debugstr_w(bstrName), newretNode);
    return E_NOTIMPL;
}

static const IHTMLAttributeCollection2Vtbl HTMLAttributeCollection2Vtbl = {
    HTMLAttributeCollection2_QueryInterface,
    HTMLAttributeCollection2_AddRef,
    HTMLAttributeCollection2_Release,
    HTMLAttributeCollection2_GetTypeInfoCount,
    HTMLAttributeCollection2_GetTypeInfo,
    HTMLAttributeCollection2_GetIDsOfNames,
    HTMLAttributeCollection2_Invoke,
    HTMLAttributeCollection2_getNamedItem,
    HTMLAttributeCollection2_setNamedItem,
    HTMLAttributeCollection2_removeNamedItem
};

static inline HTMLAttributeCollection *impl_from_IHTMLAttributeCollection3(IHTMLAttributeCollection3 *iface)
{
    return CONTAINING_RECORD(iface, HTMLAttributeCollection, IHTMLAttributeCollection3_iface);
}

static HRESULT WINAPI HTMLAttributeCollection3_QueryInterface(IHTMLAttributeCollection3 *iface, REFIID riid, void **ppv)
{
    HTMLAttributeCollection *This = impl_from_IHTMLAttributeCollection3(iface);
    return IHTMLAttributeCollection_QueryInterface(&This->IHTMLAttributeCollection_iface, riid, ppv);
}

static ULONG WINAPI HTMLAttributeCollection3_AddRef(IHTMLAttributeCollection3 *iface)
{
    HTMLAttributeCollection *This = impl_from_IHTMLAttributeCollection3(iface);
    return IHTMLAttributeCollection_AddRef(&This->IHTMLAttributeCollection_iface);
}

static ULONG WINAPI HTMLAttributeCollection3_Release(IHTMLAttributeCollection3 *iface)
{
    HTMLAttributeCollection *This = impl_from_IHTMLAttributeCollection3(iface);
    return IHTMLAttributeCollection_Release(&This->IHTMLAttributeCollection_iface);
}

static HRESULT WINAPI HTMLAttributeCollection3_GetTypeInfoCount(IHTMLAttributeCollection3 *iface, UINT *pctinfo)
{
    HTMLAttributeCollection *This = impl_from_IHTMLAttributeCollection3(iface);
    return IDispatchEx_GetTypeInfoCount(&This->dispex.IDispatchEx_iface, pctinfo);
}

static HRESULT WINAPI HTMLAttributeCollection3_GetTypeInfo(IHTMLAttributeCollection3 *iface, UINT iTInfo,
        LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLAttributeCollection *This = impl_from_IHTMLAttributeCollection3(iface);
    return IDispatchEx_GetTypeInfo(&This->dispex.IDispatchEx_iface, iTInfo, lcid, ppTInfo);
}

static HRESULT WINAPI HTMLAttributeCollection3_GetIDsOfNames(IHTMLAttributeCollection3 *iface, REFIID riid,
        LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{
    HTMLAttributeCollection *This = impl_from_IHTMLAttributeCollection3(iface);
    return IDispatchEx_GetIDsOfNames(&This->dispex.IDispatchEx_iface, riid, rgszNames, cNames,
            lcid, rgDispId);
}

static HRESULT WINAPI HTMLAttributeCollection3_Invoke(IHTMLAttributeCollection3 *iface, DISPID dispIdMember,
        REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
        VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLAttributeCollection *This = impl_from_IHTMLAttributeCollection3(iface);
    return IDispatchEx_Invoke(&This->dispex.IDispatchEx_iface, dispIdMember, riid, lcid,
            wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI HTMLAttributeCollection3_getNamedItem(IHTMLAttributeCollection3 *iface, BSTR bstrName,
        IHTMLDOMAttribute **ppNodeOut)
{
    HTMLAttributeCollection *This = impl_from_IHTMLAttributeCollection3(iface);
    return IHTMLAttributeCollection2_getNamedItem(&This->IHTMLAttributeCollection2_iface, bstrName, ppNodeOut);
}

static HRESULT WINAPI HTMLAttributeCollection3_setNamedItem(IHTMLAttributeCollection3 *iface,
        IHTMLDOMAttribute *pNodeIn, IHTMLDOMAttribute **ppNodeOut)
{
    HTMLAttributeCollection *This = impl_from_IHTMLAttributeCollection3(iface);
    FIXME("(%p)->(%p %p)\n", This, pNodeIn, ppNodeOut);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLAttributeCollection3_removeNamedItem(IHTMLAttributeCollection3 *iface,
        BSTR bstrName, IHTMLDOMAttribute **ppNodeOut)
{
    HTMLAttributeCollection *This = impl_from_IHTMLAttributeCollection3(iface);
    FIXME("(%p)->(%s %p)\n", This, debugstr_w(bstrName), ppNodeOut);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLAttributeCollection3_item(IHTMLAttributeCollection3 *iface, LONG index, IHTMLDOMAttribute **ppNodeOut)
{
    HTMLAttributeCollection *This = impl_from_IHTMLAttributeCollection3(iface);
    HTMLDOMAttribute *attr;
    DISPID id;
    HRESULT hres;

    TRACE("(%p)->(%d %p)\n", This, index, ppNodeOut);

    hres = get_attr_dispid_by_idx(This, &index, &id);
    if(hres == DISP_E_UNKNOWNNAME)
        return E_INVALIDARG;
    if(FAILED(hres))
        return hres;

    hres = get_domattr(This, id, NULL, &attr);
    if(FAILED(hres))
        return hres;

    *ppNodeOut = &attr->IHTMLDOMAttribute_iface;
    return S_OK;
}

static HRESULT WINAPI HTMLAttributeCollection3_get_length(IHTMLAttributeCollection3 *iface, LONG *p)
{
    HTMLAttributeCollection *This = impl_from_IHTMLAttributeCollection3(iface);
    return IHTMLAttributeCollection_get_length(&This->IHTMLAttributeCollection_iface, p);
}

static const IHTMLAttributeCollection3Vtbl HTMLAttributeCollection3Vtbl = {
    HTMLAttributeCollection3_QueryInterface,
    HTMLAttributeCollection3_AddRef,
    HTMLAttributeCollection3_Release,
    HTMLAttributeCollection3_GetTypeInfoCount,
    HTMLAttributeCollection3_GetTypeInfo,
    HTMLAttributeCollection3_GetIDsOfNames,
    HTMLAttributeCollection3_Invoke,
    HTMLAttributeCollection3_getNamedItem,
    HTMLAttributeCollection3_setNamedItem,
    HTMLAttributeCollection3_removeNamedItem,
    HTMLAttributeCollection3_item,
    HTMLAttributeCollection3_get_length
};

static inline HTMLAttributeCollection *HTMLAttributeCollection_from_DispatchEx(DispatchEx *iface)
{
    return CONTAINING_RECORD(iface, HTMLAttributeCollection, dispex);
}

static HRESULT HTMLAttributeCollection_get_dispid(DispatchEx *dispex, BSTR name, DWORD flags, DISPID *dispid)
{
    HTMLAttributeCollection *This = HTMLAttributeCollection_from_DispatchEx(dispex);
    HTMLDOMAttribute *attr;
    LONG pos;
    HRESULT hres;

    TRACE("(%p)->(%s %x %p)\n", This, debugstr_w(name), flags, dispid);

    hres = get_attr_dispid_by_name(This, name, dispid);
    if(FAILED(hres))
        return hres;

    hres = get_domattr(This, *dispid, &pos, &attr);
    if(FAILED(hres))
        return hres;
    IHTMLDOMAttribute_Release(&attr->IHTMLDOMAttribute_iface);

    *dispid = MSHTML_DISPID_CUSTOM_MIN+pos;
    return S_OK;
}

static HRESULT HTMLAttributeCollection_invoke(DispatchEx *dispex, DISPID id, LCID lcid,
        WORD flags, DISPPARAMS *params, VARIANT *res, EXCEPINFO *ei, IServiceProvider *caller)
{
    HTMLAttributeCollection *This = HTMLAttributeCollection_from_DispatchEx(dispex);

    TRACE("(%p)->(%x %x %x %p %p %p %p)\n", This, id, lcid, flags, params, res, ei, caller);

    switch(flags) {
    case DISPATCH_PROPERTYGET: {
        HTMLDOMAttribute *iter;
        DWORD pos;

        pos = id-MSHTML_DISPID_CUSTOM_MIN;

        LIST_FOR_EACH_ENTRY(iter, &This->attrs, HTMLDOMAttribute, entry) {
            if(!pos) {
                IHTMLDOMAttribute_AddRef(&iter->IHTMLDOMAttribute_iface);
                V_VT(res) = VT_DISPATCH;
                V_DISPATCH(res) = (IDispatch*)&iter->IHTMLDOMAttribute_iface;
                return S_OK;
            }
            pos--;
        }

        WARN("invalid arg\n");
        return E_INVALIDARG;
    }

    default:
        FIXME("unimplemented flags %x\n", flags);
        return E_NOTIMPL;
    }
}

static const dispex_static_data_vtbl_t HTMLAttributeCollection_dispex_vtbl = {
    NULL,
    HTMLAttributeCollection_get_dispid,
    HTMLAttributeCollection_invoke,
    NULL
};

static const tid_t HTMLAttributeCollection_iface_tids[] = {
    IHTMLAttributeCollection_tid,
    IHTMLAttributeCollection2_tid,
    IHTMLAttributeCollection3_tid,
    0
};

static dispex_static_data_t HTMLAttributeCollection_dispex = {
    &HTMLAttributeCollection_dispex_vtbl,
    DispHTMLAttributeCollection_tid,
    NULL,
    HTMLAttributeCollection_iface_tids
};

HRESULT HTMLElement_get_attr_col(HTMLDOMNode *iface, HTMLAttributeCollection **ac)
{
    HTMLElement *This = impl_from_HTMLDOMNode(iface);

    if(This->attrs) {
        IHTMLAttributeCollection_AddRef(&This->attrs->IHTMLAttributeCollection_iface);
        *ac = This->attrs;
        return S_OK;
    }

    This->attrs = heap_alloc_zero(sizeof(HTMLAttributeCollection));
    if(!This->attrs)
        return E_OUTOFMEMORY;

    This->attrs->IHTMLAttributeCollection_iface.lpVtbl = &HTMLAttributeCollectionVtbl;
    This->attrs->IHTMLAttributeCollection2_iface.lpVtbl = &HTMLAttributeCollection2Vtbl;
    This->attrs->IHTMLAttributeCollection3_iface.lpVtbl = &HTMLAttributeCollection3Vtbl;
    This->attrs->ref = 2;

    This->attrs->elem = This;
    list_init(&This->attrs->attrs);
    init_dispex(&This->attrs->dispex, (IUnknown*)&This->attrs->IHTMLAttributeCollection_iface,
            &HTMLAttributeCollection_dispex);

    *ac = This->attrs;
    return S_OK;
}
