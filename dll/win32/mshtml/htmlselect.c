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

WINE_DEFAULT_DEBUG_CHANNEL(mshtml);

typedef struct {
    HTMLElement element;

    const IHTMLSelectElementVtbl *lpHTMLSelectElementVtbl;

    nsIDOMHTMLSelectElement *nsselect;
} HTMLSelectElement;

#define HTMLSELECT(x)      ((IHTMLSelectElement*)         &(x)->lpHTMLSelectElementVtbl)

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

static HRESULT WINAPI HTMLSelectElement_put_size(IHTMLSelectElement *iface, long v)
{
    HTMLSelectElement *This = HTMLSELECT_THIS(iface);
    FIXME("(%p)->(%ld)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLSelectElement_get_size(IHTMLSelectElement *iface, long *p)
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
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
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

static HRESULT WINAPI HTMLSelectElement_put_selectedIndex(IHTMLSelectElement *iface, long v)
{
    HTMLSelectElement *This = HTMLSELECT_THIS(iface);
    nsresult nsres;

    TRACE("(%p)->(%ld)\n", This, v);

    nsres = nsIDOMHTMLSelectElement_SetSelectedIndex(This->nsselect, v);
    if(NS_FAILED(nsres))
        ERR("SetSelectedIndex failed: %08x\n", nsres);

    return S_OK;
}

static HRESULT WINAPI HTMLSelectElement_get_selectedIndex(IHTMLSelectElement *iface, long *p)
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

    nsAString_Init(&value_str, v);
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
    FIXME("(%p)->(%x)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLSelectElement_get_disabled(IHTMLSelectElement *iface, VARIANT_BOOL *p)
{
    HTMLSelectElement *This = HTMLSELECT_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
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
    FIXME("(%p)->(%p v)\n", This, element);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLSelectElement_remove(IHTMLSelectElement *iface, long index)
{
    HTMLSelectElement *This = HTMLSELECT_THIS(iface);
    FIXME("(%p)->(%ld)\n", This, index);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLSelectElement_put_length(IHTMLSelectElement *iface, long v)
{
    HTMLSelectElement *This = HTMLSELECT_THIS(iface);
    FIXME("(%p)->(%ld)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLSelectElement_get_length(IHTMLSelectElement *iface, long *p)
{
    HTMLSelectElement *This = HTMLSELECT_THIS(iface);
    PRUint32 length = 0;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsres = nsIDOMHTMLSelectElement_GetLength(This->nsselect, &length);
    if(NS_FAILED(nsres))
        ERR("GetLength failed: %08x\n", nsres);

    *p = length;

    TRACE("ret %ld\n", *p);
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
    FIXME("(%p)->(v v %p)\n", This, pdisp);
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

#undef HTMLSELECT_NODE_THIS

static const NodeImplVtbl HTMLSelectElementImplVtbl = {
    HTMLSelectElement_QI,
    HTMLSelectElement_destructor
};

static const tid_t HTMLSelectElement_tids[] = {
    IHTMLDOMNode_tid,
    IHTMLDOMNode2_tid,
    IHTMLElement_tid,
    IHTMLElement2_tid,
    IHTMLSelectElement_tid,
    0
};

static dispex_static_data_t HTMLSelectElement_dispex = {
    NULL,
    DispHTMLSelectElement_tid,
    NULL,
    HTMLSelectElement_tids
};

HTMLElement *HTMLSelectElement_Create(nsIDOMHTMLElement *nselem)
{
    HTMLSelectElement *ret = heap_alloc_zero(sizeof(HTMLSelectElement));
    nsresult nsres;

    ret->lpHTMLSelectElementVtbl = &HTMLSelectElementVtbl;
    ret->element.node.vtbl = &HTMLSelectElementImplVtbl;

    init_dispex(&ret->element.node.dispex, (IUnknown*)HTMLSELECT(ret), &HTMLSelectElement_dispex);
    HTMLElement_Init(&ret->element);

    nsres = nsIDOMHTMLElement_QueryInterface(nselem, &IID_nsIDOMHTMLSelectElement,
                                             (void**)&ret->nsselect);
    if(NS_FAILED(nsres))
        ERR("Could not get nsIDOMHTMLSelectElement interfce: %08x\n", nsres);

    return &ret->element;
}
