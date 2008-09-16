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
#include <stdio.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "ole2.h"

#include "wine/debug.h"

#include "mshtml_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(mshtml);

typedef struct {
    HTMLTextContainer textcont;

    const IHTMLBodyElementVtbl *lpHTMLBodyElementVtbl;

    ConnectionPoint cp_propnotif;

    nsIDOMHTMLBodyElement *nsbody;
} HTMLBodyElement;

#define HTMLBODY(x)  ((IHTMLBodyElement*)  &(x)->lpHTMLBodyElementVtbl)

static BOOL variant_to_nscolor(const VARIANT *v, nsAString *nsstr)
{
    switch(V_VT(v)) {
    case VT_BSTR:
        nsAString_Init(nsstr, V_BSTR(v));
        return TRUE;

    case VT_I4: {
        PRUnichar buf[10];
        static const WCHAR formatW[] = {'#','%','x',0};

        wsprintfW(buf, formatW, V_I4(v));
        nsAString_Init(nsstr, buf);
        return TRUE;
    }

    default:
        FIXME("invalid vt=%d\n", V_VT(v));
    }

    return FALSE;

}

static void nscolor_to_variant(const nsAString *nsstr, VARIANT *p)
{
    const PRUnichar *color;

    nsAString_GetData(nsstr, &color);

    if(*color == '#') {
        V_VT(p) = VT_I4;
        V_I4(p) = strtolW(color+1, NULL, 16);
    }else {
        V_VT(p) = VT_BSTR;
        V_BSTR(p) = SysAllocString(color);
    }
}

#define HTMLBODY_THIS(iface) DEFINE_THIS(HTMLBodyElement, HTMLBodyElement, iface)

static HRESULT WINAPI HTMLBodyElement_QueryInterface(IHTMLBodyElement *iface,
                                                     REFIID riid, void **ppv)
{
    HTMLBodyElement *This = HTMLBODY_THIS(iface);

    return IHTMLDOMNode_QueryInterface(HTMLDOMNODE(&This->textcont.element.node), riid, ppv);
}

static ULONG WINAPI HTMLBodyElement_AddRef(IHTMLBodyElement *iface)
{
    HTMLBodyElement *This = HTMLBODY_THIS(iface);

    return IHTMLDOMNode_AddRef(HTMLDOMNODE(&This->textcont.element.node));
}

static ULONG WINAPI HTMLBodyElement_Release(IHTMLBodyElement *iface)
{
    HTMLBodyElement *This = HTMLBODY_THIS(iface);

    return IHTMLDOMNode_Release(HTMLDOMNODE(&This->textcont.element.node));
}

static HRESULT WINAPI HTMLBodyElement_GetTypeInfoCount(IHTMLBodyElement *iface, UINT *pctinfo)
{
    HTMLBodyElement *This = HTMLBODY_THIS(iface);
    return IDispatchEx_GetTypeInfoCount(DISPATCHEX(&This->textcont.element.node.dispex), pctinfo);
}

static HRESULT WINAPI HTMLBodyElement_GetTypeInfo(IHTMLBodyElement *iface, UINT iTInfo,
                                              LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLBodyElement *This = HTMLBODY_THIS(iface);
    return IDispatchEx_GetTypeInfo(DISPATCHEX(&This->textcont.element.node.dispex), iTInfo, lcid, ppTInfo);
}

static HRESULT WINAPI HTMLBodyElement_GetIDsOfNames(IHTMLBodyElement *iface, REFIID riid,
                                                LPOLESTR *rgszNames, UINT cNames,
                                                LCID lcid, DISPID *rgDispId)
{
    HTMLBodyElement *This = HTMLBODY_THIS(iface);
    return IDispatchEx_GetIDsOfNames(DISPATCHEX(&This->textcont.element.node.dispex), riid, rgszNames, cNames, lcid, rgDispId);
}

static HRESULT WINAPI HTMLBodyElement_Invoke(IHTMLBodyElement *iface, DISPID dispIdMember,
                            REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
                            VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLBodyElement *This = HTMLBODY_THIS(iface);
    return IDispatchEx_Invoke(DISPATCHEX(&This->textcont.element.node.dispex), dispIdMember, riid, lcid,
            wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI HTMLBodyElement_put_background(IHTMLBodyElement *iface, BSTR v)
{
    HTMLBodyElement *This = HTMLBODY_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLBodyElement_get_background(IHTMLBodyElement *iface, BSTR *p)
{
    HTMLBodyElement *This = HTMLBODY_THIS(iface);
    nsAString background_str;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsAString_Init(&background_str, NULL);

    nsres = nsIDOMHTMLBodyElement_GetBackground(This->nsbody, &background_str);
    if(NS_SUCCEEDED(nsres)) {
        const PRUnichar *background;
        nsAString_GetData(&background_str, &background);
        *p = *background ? SysAllocString(background) : NULL;
    }else {
        ERR("GetBackground failed: %08x\n", nsres);
        *p = NULL;
    }

    nsAString_Finish(&background_str);

    TRACE("*p = %s\n", debugstr_w(*p));
    return S_OK;
}

static HRESULT WINAPI HTMLBodyElement_put_bgProperties(IHTMLBodyElement *iface, BSTR v)
{
    HTMLBodyElement *This = HTMLBODY_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLBodyElement_get_bgProperties(IHTMLBodyElement *iface, BSTR *p)
{
    HTMLBodyElement *This = HTMLBODY_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLBodyElement_put_leftMargin(IHTMLBodyElement *iface, VARIANT v)
{
    HTMLBodyElement *This = HTMLBODY_THIS(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLBodyElement_get_leftMargin(IHTMLBodyElement *iface, VARIANT *p)
{
    HTMLBodyElement *This = HTMLBODY_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLBodyElement_put_topMargin(IHTMLBodyElement *iface, VARIANT v)
{
    HTMLBodyElement *This = HTMLBODY_THIS(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLBodyElement_get_topMargin(IHTMLBodyElement *iface, VARIANT *p)
{
    HTMLBodyElement *This = HTMLBODY_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLBodyElement_put_rightMargin(IHTMLBodyElement *iface, VARIANT v)
{
    HTMLBodyElement *This = HTMLBODY_THIS(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLBodyElement_get_rightMargin(IHTMLBodyElement *iface, VARIANT *p)
{
    HTMLBodyElement *This = HTMLBODY_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLBodyElement_put_bottomMargin(IHTMLBodyElement *iface, VARIANT v)
{
    HTMLBodyElement *This = HTMLBODY_THIS(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLBodyElement_get_bottomMargin(IHTMLBodyElement *iface, VARIANT *p)
{
    HTMLBodyElement *This = HTMLBODY_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLBodyElement_put_noWrap(IHTMLBodyElement *iface, VARIANT_BOOL v)
{
    HTMLBodyElement *This = HTMLBODY_THIS(iface);
    FIXME("(%p)->(%x)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLBodyElement_get_noWrap(IHTMLBodyElement *iface, VARIANT_BOOL *p)
{
    HTMLBodyElement *This = HTMLBODY_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLBodyElement_put_bgColor(IHTMLBodyElement *iface, VARIANT v)
{
    HTMLBodyElement *This = HTMLBODY_THIS(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLBodyElement_get_bgColor(IHTMLBodyElement *iface, VARIANT *p)
{
    HTMLBodyElement *This = HTMLBODY_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLBodyElement_put_text(IHTMLBodyElement *iface, VARIANT v)
{
    HTMLBodyElement *This = HTMLBODY_THIS(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLBodyElement_get_text(IHTMLBodyElement *iface, VARIANT *p)
{
    HTMLBodyElement *This = HTMLBODY_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLBodyElement_put_link(IHTMLBodyElement *iface, VARIANT v)
{
    HTMLBodyElement *This = HTMLBODY_THIS(iface);
    nsAString link_str;
    nsresult nsres;

    TRACE("(%p)->(v%d)\n", This, V_VT(&v));

    if(!variant_to_nscolor(&v, &link_str))
        return S_OK;

    nsres = nsIDOMHTMLBodyElement_SetLink(This->nsbody, &link_str);
    nsAString_Finish(&link_str);
    if(NS_FAILED(nsres))
        ERR("SetLink failed: %08x\n", nsres);

    return S_OK;
}

static HRESULT WINAPI HTMLBodyElement_get_link(IHTMLBodyElement *iface, VARIANT *p)
{
    HTMLBodyElement *This = HTMLBODY_THIS(iface);
    nsAString link_str;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsAString_Init(&link_str, NULL);
    nsres = nsIDOMHTMLBodyElement_GetLink(This->nsbody, &link_str);
    if(NS_FAILED(nsres))
        ERR("GetLink failed: %08x\n", nsres);

    nscolor_to_variant(&link_str, p);
    nsAString_Finish(&link_str);

    return S_OK;
}

static HRESULT WINAPI HTMLBodyElement_put_vLink(IHTMLBodyElement *iface, VARIANT v)
{
    HTMLBodyElement *This = HTMLBODY_THIS(iface);
    nsAString vlink_str;
    nsresult nsres;

    TRACE("(%p)->(v%d)\n", This, V_VT(&v));

    if(!variant_to_nscolor(&v, &vlink_str))
        return S_OK;

    nsres = nsIDOMHTMLBodyElement_SetVLink(This->nsbody, &vlink_str);
    nsAString_Finish(&vlink_str);
    if(NS_FAILED(nsres))
        ERR("SetLink failed: %08x\n", nsres);

    return S_OK;
}

static HRESULT WINAPI HTMLBodyElement_get_vLink(IHTMLBodyElement *iface, VARIANT *p)
{
    HTMLBodyElement *This = HTMLBODY_THIS(iface);
    nsAString vlink_str;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsAString_Init(&vlink_str, NULL);
    nsres = nsIDOMHTMLBodyElement_GetVLink(This->nsbody, &vlink_str);
    if(NS_FAILED(nsres))
        ERR("GetLink failed: %08x\n", nsres);

    nscolor_to_variant(&vlink_str, p);
    nsAString_Finish(&vlink_str);

    return S_OK;
}

static HRESULT WINAPI HTMLBodyElement_put_aLink(IHTMLBodyElement *iface, VARIANT v)
{
    HTMLBodyElement *This = HTMLBODY_THIS(iface);
    nsAString alink_str;
    nsresult nsres;

    TRACE("(%p)->(v%d)\n", This, V_VT(&v));

    if(!variant_to_nscolor(&v, &alink_str))
        return S_OK;

    nsres = nsIDOMHTMLBodyElement_SetALink(This->nsbody, &alink_str);
    nsAString_Finish(&alink_str);
    if(NS_FAILED(nsres))
        ERR("SetALink failed: %08x\n", nsres);

    return S_OK;
}

static HRESULT WINAPI HTMLBodyElement_get_aLink(IHTMLBodyElement *iface, VARIANT *p)
{
    HTMLBodyElement *This = HTMLBODY_THIS(iface);
    nsAString alink_str;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsAString_Init(&alink_str, NULL);
    nsres = nsIDOMHTMLBodyElement_GetALink(This->nsbody, &alink_str);
    if(NS_FAILED(nsres))
        ERR("GetALink failed: %08x\n", nsres);

    nscolor_to_variant(&alink_str, p);
    nsAString_Finish(&alink_str);

    return S_OK;
}

static HRESULT WINAPI HTMLBodyElement_put_onload(IHTMLBodyElement *iface, VARIANT v)
{
    HTMLBodyElement *This = HTMLBODY_THIS(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLBodyElement_get_onload(IHTMLBodyElement *iface, VARIANT *p)
{
    HTMLBodyElement *This = HTMLBODY_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLBodyElement_put_onunload(IHTMLBodyElement *iface, VARIANT v)
{
    HTMLBodyElement *This = HTMLBODY_THIS(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLBodyElement_get_onunload(IHTMLBodyElement *iface, VARIANT *p)
{
    HTMLBodyElement *This = HTMLBODY_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLBodyElement_put_scroll(IHTMLBodyElement *iface, BSTR v)
{
    HTMLBodyElement *This = HTMLBODY_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLBodyElement_get_scroll(IHTMLBodyElement *iface, BSTR *p)
{
    HTMLBodyElement *This = HTMLBODY_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLBodyElement_put_onselect(IHTMLBodyElement *iface, VARIANT v)
{
    HTMLBodyElement *This = HTMLBODY_THIS(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLBodyElement_get_onselect(IHTMLBodyElement *iface, VARIANT *p)
{
    HTMLBodyElement *This = HTMLBODY_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLBodyElement_put_onbeforeunload(IHTMLBodyElement *iface, VARIANT v)
{
    HTMLBodyElement *This = HTMLBODY_THIS(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLBodyElement_get_onbeforeunload(IHTMLBodyElement *iface, VARIANT *p)
{
    HTMLBodyElement *This = HTMLBODY_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLBodyElement_createTextRange(IHTMLBodyElement *iface, IHTMLTxtRange **range)
{
    HTMLBodyElement *This = HTMLBODY_THIS(iface);
    nsIDOMRange *nsrange = NULL;

    TRACE("(%p)->(%p)\n", This, range);

    if(This->textcont.element.node.doc->nscontainer) {
        nsIDOMDocument *nsdoc;
        nsIDOMDocumentRange *nsdocrange;
        nsresult nsres;

        nsIWebNavigation_GetDocument(This->textcont.element.node.doc->nscontainer->navigation, &nsdoc);
        nsIDOMDocument_QueryInterface(nsdoc, &IID_nsIDOMDocumentRange, (void**)&nsdocrange);
        nsIDOMDocument_Release(nsdoc);

        nsres = nsIDOMDocumentRange_CreateRange(nsdocrange, &nsrange);
        if(NS_SUCCEEDED(nsres)) {
            nsres = nsIDOMRange_SelectNodeContents(nsrange, This->textcont.element.node.nsnode);
            if(NS_FAILED(nsres))
                ERR("SelectNodeContents failed: %08x\n", nsres);
        }else {
            ERR("CreateRange failed: %08x\n", nsres);
        }

        nsIDOMDocumentRange_Release(nsdocrange);
    }

    *range = HTMLTxtRange_Create(This->textcont.element.node.doc, nsrange);
    return S_OK;
}

#undef HTMLBODY_THIS

static const IHTMLBodyElementVtbl HTMLBodyElementVtbl = {
    HTMLBodyElement_QueryInterface,
    HTMLBodyElement_AddRef,
    HTMLBodyElement_Release,
    HTMLBodyElement_GetTypeInfoCount,
    HTMLBodyElement_GetTypeInfo,
    HTMLBodyElement_GetIDsOfNames,
    HTMLBodyElement_Invoke,
    HTMLBodyElement_put_background,
    HTMLBodyElement_get_background,
    HTMLBodyElement_put_bgProperties,
    HTMLBodyElement_get_bgProperties,
    HTMLBodyElement_put_leftMargin,
    HTMLBodyElement_get_leftMargin,
    HTMLBodyElement_put_topMargin,
    HTMLBodyElement_get_topMargin,
    HTMLBodyElement_put_rightMargin,
    HTMLBodyElement_get_rightMargin,
    HTMLBodyElement_put_bottomMargin,
    HTMLBodyElement_get_bottomMargin,
    HTMLBodyElement_put_noWrap,
    HTMLBodyElement_get_noWrap,
    HTMLBodyElement_put_bgColor,
    HTMLBodyElement_get_bgColor,
    HTMLBodyElement_put_text,
    HTMLBodyElement_get_text,
    HTMLBodyElement_put_link,
    HTMLBodyElement_get_link,
    HTMLBodyElement_put_vLink,
    HTMLBodyElement_get_vLink,
    HTMLBodyElement_put_aLink,
    HTMLBodyElement_get_aLink,
    HTMLBodyElement_put_onload,
    HTMLBodyElement_get_onload,
    HTMLBodyElement_put_onunload,
    HTMLBodyElement_get_onunload,
    HTMLBodyElement_put_scroll,
    HTMLBodyElement_get_scroll,
    HTMLBodyElement_put_onselect,
    HTMLBodyElement_get_onselect,
    HTMLBodyElement_put_onbeforeunload,
    HTMLBodyElement_get_onbeforeunload,
    HTMLBodyElement_createTextRange
};

#define HTMLBODY_NODE_THIS(iface) DEFINE_THIS2(HTMLBodyElement, textcont.element.node, iface)

static HRESULT HTMLBodyElement_QI(HTMLDOMNode *iface, REFIID riid, void **ppv)
{
    HTMLBodyElement *This = HTMLBODY_NODE_THIS(iface);

    *ppv = NULL;

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = HTMLBODY(This);
    }else if(IsEqualGUID(&IID_IDispatch, riid)) {
        TRACE("(%p)->(IID_IDispatch %p)\n", This, ppv);
        *ppv = HTMLBODY(This);
    }else if(IsEqualGUID(&IID_IHTMLBodyElement, riid)) {
        TRACE("(%p)->(IID_IHTMLBodyElement %p)\n", This, ppv);
        *ppv = HTMLBODY(This);
    }else if(IsEqualGUID(&IID_IHTMLTextContainer, riid)) {
        TRACE("(%p)->(IID_IHTMLTextContainer %p)\n", &This->textcont, ppv);
        *ppv = HTMLTEXTCONT(&This->textcont);
    }

    if(*ppv) {
        IUnknown_AddRef((IUnknown*)*ppv);
        return S_OK;
    }

    return HTMLElement_QI(&This->textcont.element.node, riid, ppv);
}

static void HTMLBodyElement_destructor(HTMLDOMNode *iface)
{
    HTMLBodyElement *This = HTMLBODY_NODE_THIS(iface);

    nsIDOMHTMLBodyElement_Release(This->nsbody);

    HTMLElement_destructor(&This->textcont.element.node);
}

#undef HTMLBODY_NODE_THIS

static const NodeImplVtbl HTMLBodyElementImplVtbl = {
    HTMLBodyElement_QI,
    HTMLBodyElement_destructor
};

static const tid_t HTMLBodyElement_iface_tids[] = {
    IHTMLBodyElement_tid,
    IHTMLBodyElement2_tid,
    IHTMLControlElement_tid,
    IHTMLDOMNode_tid,
    IHTMLDOMNode2_tid,
    IHTMLElement_tid,
    IHTMLElement2_tid,
    IHTMLElement3_tid,
    IHTMLElement4_tid,
    IHTMLTextContainer_tid,
    IHTMLUniqueName_tid,
    0
};

static dispex_static_data_t HTMLBodyElement_dispex = {
    NULL,
    DispHTMLBody_tid,
    NULL,
    HTMLBodyElement_iface_tids
};

HTMLElement *HTMLBodyElement_Create(nsIDOMHTMLElement *nselem)
{
    HTMLBodyElement *ret = heap_alloc_zero(sizeof(HTMLBodyElement));
    nsresult nsres;

    TRACE("(%p)->(%p)\n", ret, nselem);

    HTMLTextContainer_Init(&ret->textcont);

    ret->lpHTMLBodyElementVtbl = &HTMLBodyElementVtbl;

    init_dispex(&ret->textcont.element.node.dispex, (IUnknown*)HTMLBODY(ret), &HTMLBodyElement_dispex);
    ret->textcont.element.node.vtbl = &HTMLBodyElementImplVtbl;

    ConnectionPoint_Init(&ret->cp_propnotif, &ret->textcont.element.cp_container, &IID_IPropertyNotifySink);

    nsres = nsIDOMHTMLElement_QueryInterface(nselem, &IID_nsIDOMHTMLBodyElement,
                                             (void**)&ret->nsbody);
    if(NS_FAILED(nsres))
        ERR("Could not get nsDOMHTMLBodyElement: %08x\n", nsres);

    return &ret->textcont.element;
}
