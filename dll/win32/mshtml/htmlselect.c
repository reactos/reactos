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
#include "ole2.h"

#include "wine/debug.h"

#include "mshtml_private.h"
#include "htmlevent.h"

WINE_DEFAULT_DEBUG_CHANNEL(mshtml);

typedef struct {
    HTMLElement element;

    const IHTMLSelectElementVtbl *lpHTMLSelectElementVtbl;

    nsIDOMHTMLSelectElement *nsselect;
} HTMLSelectElement;

#define HTMLSELECT(x)      ((IHTMLSelectElement*)         &(x)->lpHTMLSelectElementVtbl)

static HRESULT htmlselect_item(HTMLSelectElement *This, int i, IDispatch **ret)
{
    nsIDOMHTMLOptionsCollection *nscol;
    nsIDOMNode *nsnode;
    nsresult nsres;

    nsres = nsIDOMHTMLSelectElement_GetOptions(This->nsselect, &nscol);
    if(NS_FAILED(nsres)) {
        ERR("GetOptions failed: %08x\n", nsres);
        return E_FAIL;
    }

    nsres = nsIDOMHTMLOptionsCollection_Item(nscol, i, &nsnode);
    nsIDOMHTMLOptionsCollection_Release(nscol);
    if(NS_FAILED(nsres)) {
        ERR("Item failed: %08x\n", nsres);
        return E_FAIL;
    }

    if(nsnode) {
        HTMLDOMNode *node;

        node = get_node(This->element.node.doc, nsnode, TRUE);
        nsIDOMNode_Release(nsnode);
        if(!node) {
            ERR("Could not find node\n");
            return E_FAIL;
        }

        IHTMLDOMNode_AddRef(HTMLDOMNODE(node));
        *ret = (IDispatch*)HTMLDOMNODE(node);
    }else {
        *ret = NULL;
    }
    return S_OK;
}

#define HTMLSELECT_THIS(iface) DEFINE_THIS(HTMLSelectElement, HTMLSelectElement, iface)

static HRESULT WINAPI HTMLSelectElement_QueryInterface(IHTMLSelectElement *iface,
                                                         REFIID riid, void **ppv)
{
    HTMLSelectElement *This = HTMLSELECT_THIS(iface);

    return IHTMLDOMNode_QueryInterface(HTMLDOMNODE(&This->element.node), riid, ppv);
}

static ULONG WINAPI HTMLSelectElement_AddRef(IHTMLSelectElement *iface)
{
    HTMLSelectElement *This = HTMLSELECT_THIS(iface);

    return IHTMLDOMNode_AddRef(HTMLDOMNODE(&This->element.node));
}

static ULONG WINAPI HTMLSelectElement_Release(IHTMLSelectElement *iface)
{
    HTMLSelectElement *This = HTMLSELECT_THIS(iface);

    return IHTMLDOMNode_Release(HTMLDOMNODE(&This->element.node));
}

static HRESULT WINAPI HTMLSelectElement_GetTypeInfoCount(IHTMLSelectElement *iface, UINT *pctinfo)
{
    HTMLSelectElement *This = HTMLSELECT_THIS(iface);

    return IDispatchEx_GetTypeInfoCount(DISPATCHEX(&This->element.node.dispex), pctinfo);
}

static HRESULT WINAPI HTMLSelectElement_GetTypeInfo(IHTMLSelectElement *iface, UINT iTInfo,
                                              LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLSelectElement *This = HTMLSELECT_THIS(iface);

    return IDispatchEx_GetTypeInfo(DISPATCHEX(&This->element.node.dispex), iTInfo, lcid, ppTInfo);
}

static HRESULT WINAPI HTMLSelectElement_GetIDsOfNames(IHTMLSelectElement *iface, REFIID riid,
                                                LPOLESTR *rgszNames, UINT cNames,
                                                LCID lcid, DISPID *rgDispId)
{
    HTMLSelectElement *This = HTMLSELECT_THIS(iface);

    return IDispatchEx_GetIDsOfNames(DISPATCHEX(&This->element.node.dispex), riid, rgszNames, cNames, lcid, rgDispId);
}

static HRESULT WINAPI HTMLSelectElement_Invoke(IHTMLSelectElement *iface, DISPID dispIdMember,
                            REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
                            VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLSelectElement *This = HTMLSELECT_THIS(iface);

    return IDispatchEx_Invoke(DISPATCHEX(&This->element.node.dispex), dispIdMember, riid, lcid,
            wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI HTMLSelectElement_put_size(IHTMLSelectElement *iface, LONG v)
{
    HTMLSelectElement *This = HTMLSELECT_THIS(iface);
    FIXME("(%p)->(%d)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLSelectElement_get_size(IHTMLSelectElement *iface, LONG *p)
{
    HTMLSelectElement *This = HTMLSELECT_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLSelectElement_put_multiple(IHTMLSelectElement *iface, VARIANT_BOOL v)
{
    HTMLSelectElement *This = HTMLSELECT_THIS(iface);
    FIXME("(%p)->(%x)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLSelectElement_get_multiple(IHTMLSelectElement *iface, VARIANT_BOOL *p)
{
    HTMLSelectElement *This = HTMLSELECT_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLSelectElement_put_name(IHTMLSelectElement *iface, BSTR v)
{
    HTMLSelectElement *This = HTMLSELECT_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLSelectElement_get_name(IHTMLSelectElement *iface, BSTR *p)
{
    HTMLSelectElement *This = HTMLSELECT_THIS(iface);
    nsAString name_str;
    const PRUnichar *name = NULL;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsAString_Init(&name_str, NULL);

    nsres = nsIDOMHTMLSelectElement_GetName(This->nsselect, &name_str);
    if(NS_SUCCEEDED(nsres)) {
        static const WCHAR wszGarbage[] = {'g','a','r','b','a','g','e',0};

        nsAString_GetData(&name_str, &name);

        /*
         * Native never returns empty string here. If an element has no name,
         * name of previous element or ramdom data is returned.
         */
        *p = SysAllocString(*name ? name : wszGarbage);
    }else {
        ERR("GetName failed: %08x\n", nsres);
    }

    nsAString_Finish(&name_str);

    TRACE("name=%s\n", debugstr_w(*p));
    return S_OK;
}

static HRESULT WINAPI HTMLSelectElement_get_options(IHTMLSelectElement *iface, IDispatch **p)
{
    HTMLSelectElement *This = HTMLSELECT_THIS(iface);

    TRACE("(%p)->(%p)\n", This, p);

    *p = (IDispatch*)HTMLSELECT(This);
    IDispatch_AddRef(*p);
    return S_OK;
}

static HRESULT WINAPI HTMLSelectElement_put_onchange(IHTMLSelectElement *iface, VARIANT v)
{
    HTMLSelectElement *This = HTMLSELECT_THIS(iface);

    TRACE("(%p)->()\n", This);

    return set_node_event(&This->element.node, EVENTID_CHANGE, &v);
}

static HRESULT WINAPI HTMLSelectElement_get_onchange(IHTMLSelectElement *iface, VARIANT *p)
{
    HTMLSelectElement *This = HTMLSELECT_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLSelectElement_put_selectedIndex(IHTMLSelectElement *iface, LONG v)
{
    HTMLSelectElement *This = HTMLSELECT_THIS(iface);
    nsresult nsres;

    TRACE("(%p)->(%d)\n", This, v);

    nsres = nsIDOMHTMLSelectElement_SetSelectedIndex(This->nsselect, v);
    if(NS_FAILED(nsres))
        ERR("SetSelectedIndex failed: %08x\n", nsres);

    return S_OK;
}

static HRESULT WINAPI HTMLSelectElement_get_selectedIndex(IHTMLSelectElement *iface, LONG *p)
{
    HTMLSelectElement *This = HTMLSELECT_THIS(iface);
    PRInt32 idx = 0;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsres = nsIDOMHTMLSelectElement_GetSelectedIndex(This->nsselect, &idx);
    if(NS_FAILED(nsres))
        ERR("GetSelectedIndex failed: %08x\n", nsres);

    *p = idx;
    return S_OK;
}

static HRESULT WINAPI HTMLSelectElement_get_type(IHTMLSelectElement *iface, BSTR *p)
{
    HTMLSelectElement *This = HTMLSELECT_THIS(iface);
    const PRUnichar *type;
    nsAString type_str;
    nsresult nsres;
    HRESULT hres = S_OK;

    TRACE("(%p)->(%p)\n", This, p);

    nsAString_Init(&type_str, NULL);
    nsres = nsIDOMHTMLSelectElement_GetType(This->nsselect, &type_str);
    if(NS_SUCCEEDED(nsres)) {
        nsAString_GetData(&type_str, &type);
        *p = *type ? SysAllocString(type) : NULL;
    }else {
        ERR("GetType failed: %08x\n", nsres);
        hres = E_FAIL;
    }

    nsAString_Finish(&type_str);

    return hres;
}

static HRESULT WINAPI HTMLSelectElement_put_value(IHTMLSelectElement *iface, BSTR v)
{
    HTMLSelectElement *This = HTMLSELECT_THIS(iface);
    nsAString value_str;
    nsresult nsres;

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    nsAString_InitDepend(&value_str, v);
    nsres = nsIDOMHTMLSelectElement_SetValue(This->nsselect, &value_str);
    nsAString_Finish(&value_str);
    if(NS_FAILED(nsres))
        ERR("SetValue failed: %08x\n", nsres);

    return S_OK;
}

static HRESULT WINAPI HTMLSelectElement_get_value(IHTMLSelectElement *iface, BSTR *p)
{
    HTMLSelectElement *This = HTMLSELECT_THIS(iface);
    nsAString value_str;
    const PRUnichar *value = NULL;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsAString_Init(&value_str, NULL);

    nsres = nsIDOMHTMLSelectElement_GetValue(This->nsselect, &value_str);
    if(NS_SUCCEEDED(nsres)) {
        nsAString_GetData(&value_str, &value);
        *p = *value ? SysAllocString(value) : NULL;
    }else {
        ERR("GetValue failed: %08x\n", nsres);
    }

    nsAString_Finish(&value_str);

    TRACE("value=%s\n", debugstr_w(*p));
    return S_OK;
}

static HRESULT WINAPI HTMLSelectElement_put_disabled(IHTMLSelectElement *iface, VARIANT_BOOL v)
{
    HTMLSelectElement *This = HTMLSELECT_THIS(iface);
    nsresult nsres;

    TRACE("(%p)->(%x)\n", This, v);

    nsres = nsIDOMHTMLSelectElement_SetDisabled(This->nsselect, v != VARIANT_FALSE);
    if(NS_FAILED(nsres)) {
        ERR("SetDisabled failed: %08x\n", nsres);
        return E_FAIL;
    }

    return S_OK;
}

static HRESULT WINAPI HTMLSelectElement_get_disabled(IHTMLSelectElement *iface, VARIANT_BOOL *p)
{
    HTMLSelectElement *This = HTMLSELECT_THIS(iface);
    PRBool disabled = FALSE;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsres = nsIDOMHTMLSelectElement_GetDisabled(This->nsselect, &disabled);
    if(NS_FAILED(nsres)) {
        ERR("GetDisabled failed: %08x\n", nsres);
        return E_FAIL;
    }

    *p = disabled ? VARIANT_TRUE : VARIANT_FALSE;
    return S_OK;
}

static HRESULT WINAPI HTMLSelectElement_get_form(IHTMLSelectElement *iface, IHTMLFormElement **p)
{
    HTMLSelectElement *This = HTMLSELECT_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLSelectElement_add(IHTMLSelectElement *iface, IHTMLElement *element,
                                            VARIANT before)
{
    HTMLSelectElement *This = HTMLSELECT_THIS(iface);
    IHTMLDOMNode *node, *tmp;
    HRESULT hres;

    FIXME("(%p)->(%p %s): semi-stub\n", This, element, debugstr_variant(&before));

    if(V_VT(&before) != VT_EMPTY) {
        FIXME("unhandled before %s\n", debugstr_variant(&before));
        return E_NOTIMPL;
    }

    hres = IHTMLElement_QueryInterface(element, &IID_IHTMLDOMNode, (void**)&node);
    if(FAILED(hres))
        return hres;

    hres = IHTMLDOMNode_appendChild(HTMLDOMNODE(&This->element.node), node, &tmp);
    IHTMLDOMNode_Release(node);
    if(SUCCEEDED(hres) && tmp)
        IHTMLDOMNode_Release(tmp);

    return hres;
}

static HRESULT WINAPI HTMLSelectElement_remove(IHTMLSelectElement *iface, LONG index)
{
    HTMLSelectElement *This = HTMLSELECT_THIS(iface);
    FIXME("(%p)->(%d)\n", This, index);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLSelectElement_put_length(IHTMLSelectElement *iface, LONG v)
{
    HTMLSelectElement *This = HTMLSELECT_THIS(iface);
    nsresult nsres;

    TRACE("(%p)->(%d)\n", This, v);

    nsres = nsIDOMHTMLSelectElement_SetLength(This->nsselect, v);
    if(NS_FAILED(nsres))
        ERR("SetLength failed: %08x\n", nsres);

    return S_OK;
}

static HRESULT WINAPI HTMLSelectElement_get_length(IHTMLSelectElement *iface, LONG *p)
{
    HTMLSelectElement *This = HTMLSELECT_THIS(iface);
    PRUint32 length = 0;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsres = nsIDOMHTMLSelectElement_GetLength(This->nsselect, &length);
    if(NS_FAILED(nsres))
        ERR("GetLength failed: %08x\n", nsres);

    *p = length;

    TRACE("ret %d\n", *p);
    return S_OK;
}

static HRESULT WINAPI HTMLSelectElement_get__newEnum(IHTMLSelectElement *iface, IUnknown **p)
{
    HTMLSelectElement *This = HTMLSELECT_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLSelectElement_item(IHTMLSelectElement *iface, VARIANT name,
                                             VARIANT index, IDispatch **pdisp)
{
    HTMLSelectElement *This = HTMLSELECT_THIS(iface);

    TRACE("(%p)->(%s %s %p)\n", This, debugstr_variant(&name), debugstr_variant(&index), pdisp);

    if(!pdisp)
        return E_POINTER;
    *pdisp = NULL;

    if(V_VT(&name) == VT_I4) {
        if(V_I4(&name) < 0)
            return E_INVALIDARG;
        return htmlselect_item(This, V_I4(&name), pdisp);
    }

    FIXME("Unsupported args\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLSelectElement_tags(IHTMLSelectElement *iface, VARIANT tagName,
                                             IDispatch **pdisp)
{
    HTMLSelectElement *This = HTMLSELECT_THIS(iface);
    FIXME("(%p)->(v %p)\n", This, pdisp);
    return E_NOTIMPL;
}

#undef HTMLSELECT_THIS

static const IHTMLSelectElementVtbl HTMLSelectElementVtbl = {
    HTMLSelectElement_QueryInterface,
    HTMLSelectElement_AddRef,
    HTMLSelectElement_Release,
    HTMLSelectElement_GetTypeInfoCount,
    HTMLSelectElement_GetTypeInfo,
    HTMLSelectElement_GetIDsOfNames,
    HTMLSelectElement_Invoke,
    HTMLSelectElement_put_size,
    HTMLSelectElement_get_size,
    HTMLSelectElement_put_multiple,
    HTMLSelectElement_get_multiple,
    HTMLSelectElement_put_name,
    HTMLSelectElement_get_name,
    HTMLSelectElement_get_options,
    HTMLSelectElement_put_onchange,
    HTMLSelectElement_get_onchange,
    HTMLSelectElement_put_selectedIndex,
    HTMLSelectElement_get_selectedIndex,
    HTMLSelectElement_get_type,
    HTMLSelectElement_put_value,
    HTMLSelectElement_get_value,
    HTMLSelectElement_put_disabled,
    HTMLSelectElement_get_disabled,
    HTMLSelectElement_get_form,
    HTMLSelectElement_add,
    HTMLSelectElement_remove,
    HTMLSelectElement_put_length,
    HTMLSelectElement_get_length,
    HTMLSelectElement_get__newEnum,
    HTMLSelectElement_item,
    HTMLSelectElement_tags
};

#define HTMLSELECT_NODE_THIS(iface) DEFINE_THIS2(HTMLSelectElement, element.node, iface)

static HRESULT HTMLSelectElement_QI(HTMLDOMNode *iface, REFIID riid, void **ppv)
{
    HTMLSelectElement *This = HTMLSELECT_NODE_THIS(iface);

    *ppv = NULL;

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = HTMLSELECT(This);
    }else if(IsEqualGUID(&IID_IDispatch, riid)) {
        TRACE("(%p)->(IID_IDispatch %p)\n", This, ppv);
        *ppv = HTMLSELECT(This);
    }else if(IsEqualGUID(&IID_IHTMLSelectElement, riid)) {
        TRACE("(%p)->(IID_IHTMLSelectElement %p)\n", This, ppv);
        *ppv = HTMLSELECT(This);
    }

    if(*ppv) {
        IUnknown_AddRef((IUnknown*)*ppv);
        return S_OK;
    }

    return HTMLElement_QI(&This->element.node, riid, ppv);
}

static void HTMLSelectElement_destructor(HTMLDOMNode *iface)
{
    HTMLSelectElement *This = HTMLSELECT_NODE_THIS(iface);

    nsIDOMHTMLSelectElement_Release(This->nsselect);

    HTMLElement_destructor(&This->element.node);
}

static HRESULT HTMLSelectElementImpl_put_disabled(HTMLDOMNode *iface, VARIANT_BOOL v)
{
    HTMLSelectElement *This = HTMLSELECT_NODE_THIS(iface);
    return IHTMLSelectElement_put_disabled(HTMLSELECT(This), v);
}

static HRESULT HTMLSelectElementImpl_get_disabled(HTMLDOMNode *iface, VARIANT_BOOL *p)
{
    HTMLSelectElement *This = HTMLSELECT_NODE_THIS(iface);
    return IHTMLSelectElement_get_disabled(HTMLSELECT(This), p);
}

#define DISPID_OPTIONCOL_0 MSHTML_DISPID_CUSTOM_MIN

static HRESULT HTMLSelectElement_get_dispid(HTMLDOMNode *iface, BSTR name, DWORD flags, DISPID *dispid)
{
    const WCHAR *ptr;
    DWORD idx = 0;

    for(ptr = name; *ptr && isdigitW(*ptr); ptr++) {
        idx = idx*10 + (*ptr-'0');
        if(idx > MSHTML_CUSTOM_DISPID_CNT) {
            WARN("too big idx\n");
            return DISP_E_UNKNOWNNAME;
        }
    }
    if(*ptr)
        return DISP_E_UNKNOWNNAME;

    *dispid = DISPID_OPTIONCOL_0 + idx;
    return S_OK;
}

static HRESULT HTMLSelectElement_invoke(HTMLDOMNode *iface, DISPID id, LCID lcid, WORD flags, DISPPARAMS *params,
        VARIANT *res, EXCEPINFO *ei, IServiceProvider *caller)
{
    HTMLSelectElement *This = HTMLSELECT_NODE_THIS(iface);

    TRACE("(%p)->(%x %x %x %p %p %p %p)\n", This, id, lcid, flags, params, res, ei, caller);

    switch(flags) {
    case DISPATCH_PROPERTYGET: {
        IDispatch *ret;
        HRESULT hres;

        hres = htmlselect_item(This, id-DISPID_OPTIONCOL_0, &ret);
        if(FAILED(hres))
            return hres;

        if(ret) {
            V_VT(res) = VT_DISPATCH;
            V_DISPATCH(res) = ret;
        }else {
            V_VT(res) = VT_NULL;
        }
        break;
    }

    default:
        FIXME("unimplemented flags %x\n", flags);
        return E_NOTIMPL;
    }

    return S_OK;
}

#undef HTMLSELECT_NODE_THIS

static const NodeImplVtbl HTMLSelectElementImplVtbl = {
    HTMLSelectElement_QI,
    HTMLSelectElement_destructor,
    NULL,
    NULL,
    HTMLSelectElementImpl_put_disabled,
    HTMLSelectElementImpl_get_disabled,
    NULL,
    NULL,
    HTMLSelectElement_get_dispid,
    HTMLSelectElement_invoke
};

static const tid_t HTMLSelectElement_tids[] = {
    HTMLELEMENT_TIDS,
    IHTMLSelectElement_tid,
    0
};

static dispex_static_data_t HTMLSelectElement_dispex = {
    NULL,
    DispHTMLSelectElement_tid,
    NULL,
    HTMLSelectElement_tids
};

HTMLElement *HTMLSelectElement_Create(HTMLDocumentNode *doc, nsIDOMHTMLElement *nselem)
{
    HTMLSelectElement *ret = heap_alloc_zero(sizeof(HTMLSelectElement));
    nsresult nsres;

    ret->lpHTMLSelectElementVtbl = &HTMLSelectElementVtbl;
    ret->element.node.vtbl = &HTMLSelectElementImplVtbl;

    HTMLElement_Init(&ret->element, doc, nselem, &HTMLSelectElement_dispex);

    nsres = nsIDOMHTMLElement_QueryInterface(nselem, &IID_nsIDOMHTMLSelectElement,
                                             (void**)&ret->nsselect);
    if(NS_FAILED(nsres))
        ERR("Could not get nsIDOMHTMLSelectElement interfce: %08x\n", nsres);

    return &ret->element;
}
