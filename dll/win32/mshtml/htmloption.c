/*
 * Copyright 2007 Jacek Caban for CodeWeavers
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
#include "ole2.h"

#include "wine/debug.h"

#include "mshtml_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(mshtml);

typedef struct {
    HTMLElement element;

    const IHTMLOptionElementVtbl *lpHTMLOptionElementVtbl;

    nsIDOMHTMLOptionElement *nsoption;
} HTMLOptionElement;

#define HTMLOPTION(x)  ((IHTMLOptionElement*)  &(x)->lpHTMLOptionElementVtbl)

#define HTMLOPTION_THIS(iface) DEFINE_THIS(HTMLOptionElement, HTMLOptionElement, iface)

static HRESULT WINAPI HTMLOptionElement_QueryInterface(IHTMLOptionElement *iface,
        REFIID riid, void **ppv)
{
    HTMLOptionElement *This = HTMLOPTION_THIS(iface);

    return IHTMLDOMNode_QueryInterface(HTMLDOMNODE(&This->element.node), riid, ppv);
}

static ULONG WINAPI HTMLOptionElement_AddRef(IHTMLOptionElement *iface)
{
    HTMLOptionElement *This = HTMLOPTION_THIS(iface);

    return IHTMLDOMNode_AddRef(HTMLDOMNODE(&This->element.node));
}

static ULONG WINAPI HTMLOptionElement_Release(IHTMLOptionElement *iface)
{
    HTMLOptionElement *This = HTMLOPTION_THIS(iface);

    return IHTMLDOMNode_Release(HTMLDOMNODE(&This->element.node));
}

static HRESULT WINAPI HTMLOptionElement_GetTypeInfoCount(IHTMLOptionElement *iface, UINT *pctinfo)
{
    HTMLOptionElement *This = HTMLOPTION_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pctinfo);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLOptionElement_GetTypeInfo(IHTMLOptionElement *iface, UINT iTInfo,
                                              LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLOptionElement *This = HTMLOPTION_THIS(iface);
    FIXME("(%p)->(%u %u %p)\n", This, iTInfo, lcid, ppTInfo);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLOptionElement_GetIDsOfNames(IHTMLOptionElement *iface, REFIID riid,
                                                LPOLESTR *rgszNames, UINT cNames,
                                                LCID lcid, DISPID *rgDispId)
{
    HTMLOptionElement *This = HTMLOPTION_THIS(iface);
    FIXME("(%p)->(%s %p %u %u %p)\n", This, debugstr_guid(riid), rgszNames, cNames,
                                        lcid, rgDispId);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLOptionElement_Invoke(IHTMLOptionElement *iface, DISPID dispIdMember,
                            REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
                            VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLOptionElement *This = HTMLOPTION_THIS(iface);
    FIXME("(%p)->(%d %s %d %d %p %p %p %p)\n", This, dispIdMember, debugstr_guid(riid),
            lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLOptionElement_put_selected(IHTMLOptionElement *iface, VARIANT_BOOL v)
{
    HTMLOptionElement *This = HTMLOPTION_THIS(iface);
    FIXME("(%p)->(%x)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLOptionElement_get_selected(IHTMLOptionElement *iface, VARIANT_BOOL *p)
{
    HTMLOptionElement *This = HTMLOPTION_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLOptionElement_put_value(IHTMLOptionElement *iface, BSTR v)
{
    HTMLOptionElement *This = HTMLOPTION_THIS(iface);
    nsAString value_str;
    nsresult nsres;

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    nsAString_Init(&value_str, v);
    nsres = nsIDOMHTMLOptionElement_SetValue(This->nsoption, &value_str);
    nsAString_Finish(&value_str);
    if(NS_FAILED(nsres))
        ERR("SetValue failed: %08x\n", nsres);

    return S_OK;
}

static HRESULT WINAPI HTMLOptionElement_get_value(IHTMLOptionElement *iface, BSTR *p)
{
    HTMLOptionElement *This = HTMLOPTION_THIS(iface);
    nsAString value_str;
    const PRUnichar *value;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsAString_Init(&value_str, NULL);
    nsres = nsIDOMHTMLOptionElement_GetValue(This->nsoption, &value_str);
    if(NS_SUCCEEDED(nsres)) {
        nsAString_GetData(&value_str, &value);
        *p = SysAllocString(value);
    }else {
        ERR("GetValue failed: %08x\n", nsres);
        *p = NULL;
    }
    nsAString_Finish(&value_str);

    return S_OK;
}

static HRESULT WINAPI HTMLOptionElement_put_defaultSelected(IHTMLOptionElement *iface, VARIANT_BOOL v)
{
    HTMLOptionElement *This = HTMLOPTION_THIS(iface);
    FIXME("(%p)->(%x)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLOptionElement_get_defaultSelected(IHTMLOptionElement *iface, VARIANT_BOOL *p)
{
    HTMLOptionElement *This = HTMLOPTION_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLOptionElement_put_index(IHTMLOptionElement *iface, LONG v)
{
    HTMLOptionElement *This = HTMLOPTION_THIS(iface);
    FIXME("(%p)->(%d)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLOptionElement_get_index(IHTMLOptionElement *iface, LONG *p)
{
    HTMLOptionElement *This = HTMLOPTION_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLOptionElement_put_text(IHTMLOptionElement *iface, BSTR v)
{
    HTMLOptionElement *This = HTMLOPTION_THIS(iface);
    nsIDOMDocument *nsdoc;
    nsIDOMText *text_node;
    nsAString text_str;
    nsIDOMNode *tmp;
    nsresult nsres;

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    while(1) {
        nsIDOMNode *child;

        nsres = nsIDOMHTMLOptionElement_GetFirstChild(This->nsoption, &child);
        if(NS_FAILED(nsres) || !child)
            break;

        nsres = nsIDOMHTMLOptionElement_RemoveChild(This->nsoption, child, &tmp);
        nsIDOMNode_Release(child);
        if(NS_SUCCEEDED(nsres)) {
            nsIDOMNode_Release(tmp);
        }else {
            ERR("RemoveChild failed: %08x\n", nsres);
            break;
        }
    }

    nsres = nsIWebNavigation_GetDocument(This->element.node.doc->nscontainer->navigation, &nsdoc);
    if(NS_FAILED(nsres)) {
        ERR("GetDocument failed: %08x\n", nsres);
        return S_OK;
    }

    nsAString_Init(&text_str, v);
    nsres = nsIDOMDocument_CreateTextNode(nsdoc, &text_str, &text_node);
    nsIDOMDocument_Release(nsdoc);
    nsAString_Finish(&text_str);
    if(NS_FAILED(nsres)) {
        ERR("CreateTextNode failed: %08x\n", nsres);
        return S_OK;
    }

    nsres = nsIDOMHTMLOptionElement_AppendChild(This->nsoption, (nsIDOMNode*)text_node, &tmp);
    if(NS_SUCCEEDED(nsres))
        nsIDOMNode_Release(tmp);
    else
        ERR("AppendChild failed: %08x\n", nsres);

    return S_OK;
}

static HRESULT WINAPI HTMLOptionElement_get_text(IHTMLOptionElement *iface, BSTR *p)
{
    HTMLOptionElement *This = HTMLOPTION_THIS(iface);
    nsAString text_str;
    const PRUnichar *text;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsAString_Init(&text_str, NULL);
    nsres = nsIDOMHTMLOptionElement_GetText(This->nsoption, &text_str);
    if(NS_SUCCEEDED(nsres)) {
        nsAString_GetData(&text_str, &text);
        *p = SysAllocString(text);
    }else {
        ERR("GetText failed: %08x\n", nsres);
        *p = NULL;
    }
    nsAString_Finish(&text_str);

    return S_OK;
}

static HRESULT WINAPI HTMLOptionElement_get_form(IHTMLOptionElement *iface, IHTMLFormElement **p)
{
    HTMLOptionElement *This = HTMLOPTION_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

#undef HTMLOPTION_THIS

static const IHTMLOptionElementVtbl HTMLOptionElementVtbl = {
    HTMLOptionElement_QueryInterface,
    HTMLOptionElement_AddRef,
    HTMLOptionElement_Release,
    HTMLOptionElement_GetTypeInfoCount,
    HTMLOptionElement_GetTypeInfo,
    HTMLOptionElement_GetIDsOfNames,
    HTMLOptionElement_Invoke,
    HTMLOptionElement_put_selected,
    HTMLOptionElement_get_selected,
    HTMLOptionElement_put_value,
    HTMLOptionElement_get_value,
    HTMLOptionElement_put_defaultSelected,
    HTMLOptionElement_get_defaultSelected,
    HTMLOptionElement_put_index,
    HTMLOptionElement_get_index,
    HTMLOptionElement_put_text,
    HTMLOptionElement_get_text,
    HTMLOptionElement_get_form
};

#define HTMLOPTION_NODE_THIS(iface) DEFINE_THIS2(HTMLOptionElement, element.node, iface)

static HRESULT HTMLOptionElement_QI(HTMLDOMNode *iface, REFIID riid, void **ppv)
{
    HTMLOptionElement *This = HTMLOPTION_NODE_THIS(iface);

    *ppv = NULL;

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = HTMLOPTION(This);
    }else if(IsEqualGUID(&IID_IDispatch, riid)) {
        TRACE("(%p)->(IID_IDispatch %p)\n", This, ppv);
        *ppv = HTMLOPTION(This);
    }else if(IsEqualGUID(&IID_IHTMLOptionElement, riid)) {
        TRACE("(%p)->(IID_IHTMLOptionElement %p)\n", This, ppv);
        *ppv = HTMLOPTION(This);
    }

    if(*ppv) {
        IUnknown_AddRef((IUnknown*)*ppv);
        return S_OK;
    }

    return HTMLElement_QI(&This->element.node, riid, ppv);
}

static void HTMLOptionElement_destructor(HTMLDOMNode *iface)
{
    HTMLOptionElement *This = HTMLOPTION_NODE_THIS(iface);

    if(This->nsoption)
        nsIDOMHTMLOptionElement_Release(This->nsoption);

    HTMLElement_destructor(&This->element.node);
}

#undef HTMLOPTION_NODE_THIS

static const NodeImplVtbl HTMLOptionElementImplVtbl = {
    HTMLOptionElement_QI,
    HTMLOptionElement_destructor
};

static const tid_t HTMLOptionElement_iface_tids[] = {
    IHTMLDOMNode_tid,
    IHTMLDOMNode2_tid,
    IHTMLElement_tid,
    IHTMLElement2_tid,
    IHTMLOptionElement_tid,
    0
};
static dispex_static_data_t HTMLOptionElement_dispex = {
    NULL,
    DispHTMLOptionElement_tid,
    NULL,
    HTMLOptionElement_iface_tids
};

HTMLElement *HTMLOptionElement_Create(nsIDOMHTMLElement *nselem)
{
    HTMLOptionElement *ret = heap_alloc_zero(sizeof(HTMLOptionElement));
    nsresult nsres;

    ret->lpHTMLOptionElementVtbl = &HTMLOptionElementVtbl;
    ret->element.node.vtbl = &HTMLOptionElementImplVtbl;

    HTMLElement_Init(&ret->element);
    init_dispex(&ret->element.node.dispex, (IUnknown*)HTMLOPTION(ret), &HTMLOptionElement_dispex);

    nsres = nsIDOMHTMLElement_QueryInterface(nselem, &IID_nsIDOMHTMLOptionElement, (void**)&ret->nsoption);
    if(NS_FAILED(nsres))
        ERR("Could not get nsIDOMHTMLOptionElement interface: %08x\n", nsres);

    return &ret->element;
}

#define HTMLOPTFACTORY_THIS(iface) DEFINE_THIS(HTMLOptionElementFactory, HTMLOptionElementFactory, iface)

static HRESULT WINAPI HTMLOptionElementFactory_QueryInterface(IHTMLOptionElementFactory *iface,
                                                              REFIID riid, void **ppv)
{
    HTMLOptionElementFactory *This = HTMLOPTFACTORY_THIS(iface);

    *ppv = NULL;

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = HTMLOPTFACTORY(This);
    }else if(IsEqualGUID(&IID_IDispatch, riid)) {
        TRACE("(%p)->(IID_IDispatch %p)\n", This, ppv);
        *ppv = HTMLOPTFACTORY(This);
    }else if(IsEqualGUID(&IID_IHTMLOptionElementFactory, riid)) {
        TRACE("(%p)->(IID_IHTMLOptionElementFactory %p)\n", This, ppv);
        *ppv = HTMLOPTFACTORY(This);
    }

    if(*ppv) {
        IUnknown_AddRef((IUnknown*)*ppv);
        return S_OK;
    }

    WARN("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppv);
    return E_NOINTERFACE;
}

static ULONG WINAPI HTMLOptionElementFactory_AddRef(IHTMLOptionElementFactory *iface)
{
    HTMLOptionElementFactory *This = HTMLOPTFACTORY_THIS(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    return ref;
}

static ULONG WINAPI HTMLOptionElementFactory_Release(IHTMLOptionElementFactory *iface)
{
    HTMLOptionElementFactory *This = HTMLOPTFACTORY_THIS(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    if(!ref)
        heap_free(This);

    return ref;
}

static HRESULT WINAPI HTMLOptionElementFactory_GetTypeInfoCount(IHTMLOptionElementFactory *iface, UINT *pctinfo)
{
    HTMLOptionElementFactory *This = HTMLOPTFACTORY_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pctinfo);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLOptionElementFactory_GetTypeInfo(IHTMLOptionElementFactory *iface, UINT iTInfo,
                                              LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLOptionElementFactory *This = HTMLOPTFACTORY_THIS(iface);
    FIXME("(%p)->(%u %u %p)\n", This, iTInfo, lcid, ppTInfo);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLOptionElementFactory_GetIDsOfNames(IHTMLOptionElementFactory *iface, REFIID riid,
                                                LPOLESTR *rgszNames, UINT cNames,
                                                LCID lcid, DISPID *rgDispId)
{
    HTMLOptionElementFactory *This = HTMLOPTFACTORY_THIS(iface);
    FIXME("(%p)->(%s %p %u %u %p)\n", This, debugstr_guid(riid), rgszNames, cNames,
                                        lcid, rgDispId);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLOptionElementFactory_Invoke(IHTMLOptionElementFactory *iface, DISPID dispIdMember,
                            REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
                            VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLOptionElementFactory *This = HTMLOPTFACTORY_THIS(iface);
    FIXME("(%p)->(%d %s %d %d %p %p %p %p)\n", This, dispIdMember, debugstr_guid(riid),
            lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLOptionElementFactory_create(IHTMLOptionElementFactory *iface,
        VARIANT text, VARIANT value, VARIANT defaultselected, VARIANT selected,
        IHTMLOptionElement **optelem)
{
    HTMLOptionElementFactory *This = HTMLOPTFACTORY_THIS(iface);
    nsIDOMDocument *nsdoc;
    nsIDOMElement *nselem;
    nsAString option_str;
    nsresult nsres;
    HRESULT hres;

    static const PRUnichar optionW[] = {'O','P','T','I','O','N',0};

    TRACE("(%p)->(v v v v %p)\n", This, optelem);

    *optelem = NULL;
    if(!This->doc->nscontainer)
        return E_FAIL;

    nsres = nsIWebNavigation_GetDocument(This->doc->nscontainer->navigation, &nsdoc);
    if(NS_FAILED(nsres)) {
        ERR("GetDocument failed: %08x\n", nsres);
        return E_FAIL;
    }

    nsAString_Init(&option_str, optionW);
    nsres = nsIDOMDocument_CreateElement(nsdoc, &option_str, &nselem);
    nsIDOMDocument_Release(nsdoc);
    nsAString_Finish(&option_str);
    if(NS_FAILED(nsres)) {
        ERR("CreateElement failed: %08x\n", nsres);
        return E_FAIL;
    }

    hres = IHTMLDOMNode_QueryInterface(HTMLDOMNODE(get_node(This->doc, (nsIDOMNode*)nselem, TRUE)),
            &IID_IHTMLOptionElement, (void**)optelem);
    nsIDOMElement_Release(nselem);

    if(V_VT(&text) == VT_BSTR)
        IHTMLOptionElement_put_text(*optelem, V_BSTR(&text));
    else if(V_VT(&text) != VT_EMPTY)
        FIXME("Unsupported text vt=%d\n", V_VT(&text));

    if(V_VT(&value) == VT_BSTR)
        IHTMLOptionElement_put_value(*optelem, V_BSTR(&value));
    else if(V_VT(&value) != VT_EMPTY)
        FIXME("Unsupported value vt=%d\n", V_VT(&value));

    if(V_VT(&defaultselected) != VT_EMPTY)
        FIXME("Unsupported defaultselected vt=%d\n", V_VT(&defaultselected));
    if(V_VT(&selected) != VT_EMPTY)
        FIXME("Unsupported selected vt=%d\n", V_VT(&selected));

    return S_OK;
}

#undef HTMLOPTFACTORY_THIS

static const IHTMLOptionElementFactoryVtbl HTMLOptionElementFactoryVtbl = {
    HTMLOptionElementFactory_QueryInterface,
    HTMLOptionElementFactory_AddRef,
    HTMLOptionElementFactory_Release,
    HTMLOptionElementFactory_GetTypeInfoCount,
    HTMLOptionElementFactory_GetTypeInfo,
    HTMLOptionElementFactory_GetIDsOfNames,
    HTMLOptionElementFactory_Invoke,
    HTMLOptionElementFactory_create
};

HTMLOptionElementFactory *HTMLOptionElementFactory_Create(HTMLDocument *doc)
{
    HTMLOptionElementFactory *ret;

    ret = heap_alloc(sizeof(HTMLOptionElementFactory));

    ret->lpHTMLOptionElementFactoryVtbl = &HTMLOptionElementFactoryVtbl;
    ret->ref = 1;
    ret->doc = doc;

    return ret;
}
