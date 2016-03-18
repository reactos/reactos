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

typedef struct {
    HTMLElement element;

    IHTMLSelectElement IHTMLSelectElement_iface;

    nsIDOMHTMLSelectElement *nsselect;
} HTMLSelectElement;

static inline HTMLSelectElement *impl_from_IHTMLSelectElement(IHTMLSelectElement *iface)
{
    return CONTAINING_RECORD(iface, HTMLSelectElement, IHTMLSelectElement_iface);
}

static HRESULT htmlselect_item(HTMLSelectElement *This, int i, IDispatch **ret)
{
    nsIDOMHTMLOptionsCollection *nscol;
    nsIDOMNode *nsnode;
    nsresult nsres;
    HRESULT hres;

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

        hres = get_node(This->element.node.doc, nsnode, TRUE, &node);
        nsIDOMNode_Release(nsnode);
        if(FAILED(hres))
            return hres;

        *ret = (IDispatch*)&node->IHTMLDOMNode_iface;
    }else {
        *ret = NULL;
    }
    return S_OK;
}

static HRESULT WINAPI HTMLSelectElement_QueryInterface(IHTMLSelectElement *iface,
                                                         REFIID riid, void **ppv)
{
    HTMLSelectElement *This = impl_from_IHTMLSelectElement(iface);

    return IHTMLDOMNode_QueryInterface(&This->element.node.IHTMLDOMNode_iface, riid, ppv);
}

static ULONG WINAPI HTMLSelectElement_AddRef(IHTMLSelectElement *iface)
{
    HTMLSelectElement *This = impl_from_IHTMLSelectElement(iface);

    return IHTMLDOMNode_AddRef(&This->element.node.IHTMLDOMNode_iface);
}

static ULONG WINAPI HTMLSelectElement_Release(IHTMLSelectElement *iface)
{
    HTMLSelectElement *This = impl_from_IHTMLSelectElement(iface);

    return IHTMLDOMNode_Release(&This->element.node.IHTMLDOMNode_iface);
}

static HRESULT WINAPI HTMLSelectElement_GetTypeInfoCount(IHTMLSelectElement *iface, UINT *pctinfo)
{
    HTMLSelectElement *This = impl_from_IHTMLSelectElement(iface);

    return IDispatchEx_GetTypeInfoCount(&This->element.node.event_target.dispex.IDispatchEx_iface, pctinfo);
}

static HRESULT WINAPI HTMLSelectElement_GetTypeInfo(IHTMLSelectElement *iface, UINT iTInfo,
                                              LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLSelectElement *This = impl_from_IHTMLSelectElement(iface);

    return IDispatchEx_GetTypeInfo(&This->element.node.event_target.dispex.IDispatchEx_iface, iTInfo, lcid,
            ppTInfo);
}

static HRESULT WINAPI HTMLSelectElement_GetIDsOfNames(IHTMLSelectElement *iface, REFIID riid,
                                                LPOLESTR *rgszNames, UINT cNames,
                                                LCID lcid, DISPID *rgDispId)
{
    HTMLSelectElement *This = impl_from_IHTMLSelectElement(iface);

    return IDispatchEx_GetIDsOfNames(&This->element.node.event_target.dispex.IDispatchEx_iface, riid, rgszNames,
            cNames, lcid, rgDispId);
}

static HRESULT WINAPI HTMLSelectElement_Invoke(IHTMLSelectElement *iface, DISPID dispIdMember,
                            REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
                            VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLSelectElement *This = impl_from_IHTMLSelectElement(iface);

    return IDispatchEx_Invoke(&This->element.node.event_target.dispex.IDispatchEx_iface, dispIdMember, riid,
            lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI HTMLSelectElement_put_size(IHTMLSelectElement *iface, LONG v)
{
    HTMLSelectElement *This = impl_from_IHTMLSelectElement(iface);
    nsresult nsres;

    TRACE("(%p)->(%d)\n", This, v);
    if(v < 0)
        return CTL_E_INVALIDPROPERTYVALUE;

    nsres = nsIDOMHTMLSelectElement_SetSize(This->nsselect, v);
    if(NS_FAILED(nsres)) {
        ERR("SetSize failed: %08x\n", nsres);
        return E_FAIL;
    }
    return S_OK;
}

static HRESULT WINAPI HTMLSelectElement_get_size(IHTMLSelectElement *iface, LONG *p)
{
    HTMLSelectElement *This = impl_from_IHTMLSelectElement(iface);
    DWORD val;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);
    if(!p)
        return E_INVALIDARG;

    nsres = nsIDOMHTMLSelectElement_GetSize(This->nsselect, &val);
    if(NS_FAILED(nsres)) {
        ERR("GetSize failed: %08x\n", nsres);
        return E_FAIL;
    }
    *p = val;
    return S_OK;
}

static HRESULT WINAPI HTMLSelectElement_put_multiple(IHTMLSelectElement *iface, VARIANT_BOOL v)
{
    HTMLSelectElement *This = impl_from_IHTMLSelectElement(iface);
    nsresult nsres;

    TRACE("(%p)->(%x)\n", This, v);

    nsres = nsIDOMHTMLSelectElement_SetMultiple(This->nsselect, !!v);
    assert(nsres == NS_OK);
    return S_OK;
}

static HRESULT WINAPI HTMLSelectElement_get_multiple(IHTMLSelectElement *iface, VARIANT_BOOL *p)
{
    HTMLSelectElement *This = impl_from_IHTMLSelectElement(iface);
    cpp_bool val;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsres = nsIDOMHTMLSelectElement_GetMultiple(This->nsselect, &val);
    assert(nsres == NS_OK);

    *p = val ? VARIANT_TRUE : VARIANT_FALSE;
    return S_OK;
}

static HRESULT WINAPI HTMLSelectElement_put_name(IHTMLSelectElement *iface, BSTR v)
{
    HTMLSelectElement *This = impl_from_IHTMLSelectElement(iface);
    nsAString str;
    nsresult nsres;

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));
    nsAString_InitDepend(&str, v);
    nsres = nsIDOMHTMLSelectElement_SetName(This->nsselect, &str);
    nsAString_Finish(&str);

    if(NS_FAILED(nsres)) {
        ERR("SetName failed: %08x\n", nsres);
        return E_FAIL;
    }
    return S_OK;
}

static HRESULT WINAPI HTMLSelectElement_get_name(IHTMLSelectElement *iface, BSTR *p)
{
    HTMLSelectElement *This = impl_from_IHTMLSelectElement(iface);
    nsAString name_str;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsAString_Init(&name_str, NULL);
    nsres = nsIDOMHTMLSelectElement_GetName(This->nsselect, &name_str);

    return return_nsstr(nsres, &name_str, p);
}

static HRESULT WINAPI HTMLSelectElement_get_options(IHTMLSelectElement *iface, IDispatch **p)
{
    HTMLSelectElement *This = impl_from_IHTMLSelectElement(iface);

    TRACE("(%p)->(%p)\n", This, p);

    *p = (IDispatch*)&This->IHTMLSelectElement_iface;
    IDispatch_AddRef(*p);
    return S_OK;
}

static HRESULT WINAPI HTMLSelectElement_put_onchange(IHTMLSelectElement *iface, VARIANT v)
{
    HTMLSelectElement *This = impl_from_IHTMLSelectElement(iface);

    TRACE("(%p)->()\n", This);

    return set_node_event(&This->element.node, EVENTID_CHANGE, &v);
}

static HRESULT WINAPI HTMLSelectElement_get_onchange(IHTMLSelectElement *iface, VARIANT *p)
{
    HTMLSelectElement *This = impl_from_IHTMLSelectElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLSelectElement_put_selectedIndex(IHTMLSelectElement *iface, LONG v)
{
    HTMLSelectElement *This = impl_from_IHTMLSelectElement(iface);
    nsresult nsres;

    TRACE("(%p)->(%d)\n", This, v);

    nsres = nsIDOMHTMLSelectElement_SetSelectedIndex(This->nsselect, v);
    if(NS_FAILED(nsres))
        ERR("SetSelectedIndex failed: %08x\n", nsres);

    return S_OK;
}

static HRESULT WINAPI HTMLSelectElement_get_selectedIndex(IHTMLSelectElement *iface, LONG *p)
{
    HTMLSelectElement *This = impl_from_IHTMLSelectElement(iface);
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsres = nsIDOMHTMLSelectElement_GetSelectedIndex(This->nsselect, p);
    if(NS_FAILED(nsres)) {
        ERR("GetSelectedIndex failed: %08x\n", nsres);
        return E_FAIL;
    }

    return S_OK;
}

static HRESULT WINAPI HTMLSelectElement_get_type(IHTMLSelectElement *iface, BSTR *p)
{
    HTMLSelectElement *This = impl_from_IHTMLSelectElement(iface);
    nsAString type_str;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsAString_Init(&type_str, NULL);
    nsres = nsIDOMHTMLSelectElement_GetType(This->nsselect, &type_str);
    return return_nsstr(nsres, &type_str, p);
}

static HRESULT WINAPI HTMLSelectElement_put_value(IHTMLSelectElement *iface, BSTR v)
{
    HTMLSelectElement *This = impl_from_IHTMLSelectElement(iface);
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
    HTMLSelectElement *This = impl_from_IHTMLSelectElement(iface);
    nsAString value_str;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsAString_Init(&value_str, NULL);
    nsres = nsIDOMHTMLSelectElement_GetValue(This->nsselect, &value_str);
    return return_nsstr(nsres, &value_str, p);
}

static HRESULT WINAPI HTMLSelectElement_put_disabled(IHTMLSelectElement *iface, VARIANT_BOOL v)
{
    HTMLSelectElement *This = impl_from_IHTMLSelectElement(iface);
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
    HTMLSelectElement *This = impl_from_IHTMLSelectElement(iface);
    cpp_bool disabled = FALSE;
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
    HTMLSelectElement *This = impl_from_IHTMLSelectElement(iface);
    nsIDOMHTMLFormElement *nsform;
    nsIDOMNode *form_node;
    HTMLDOMNode *node;
    HRESULT hres;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    if(!p)
        return E_POINTER;

    nsres = nsIDOMHTMLSelectElement_GetForm(This->nsselect, &nsform);
    if (NS_FAILED(nsres)) {
        ERR("GetForm failed: %08x, nsform: %p\n", nsres, nsform);
        *p = NULL;
        return E_FAIL;
    }
    if (nsform == NULL) {
        TRACE("nsform not found\n");
        *p = NULL;
        return S_OK;
    }

    nsres = nsIDOMHTMLFormElement_QueryInterface(nsform, &IID_nsIDOMNode, (void**)&form_node);
    nsIDOMHTMLFormElement_Release(nsform);
    assert(nsres == NS_OK);

    hres = get_node(This->element.node.doc, form_node, TRUE, &node);
    nsIDOMNode_Release(form_node);
    if (FAILED(hres))
        return hres;

    hres = IHTMLDOMNode_QueryInterface(&node->IHTMLDOMNode_iface, &IID_IHTMLElement, (void**)p);

    node_release(node);
    return hres;
}

static HRESULT WINAPI HTMLSelectElement_add(IHTMLSelectElement *iface, IHTMLElement *element,
                                            VARIANT before)
{
    HTMLSelectElement *This = impl_from_IHTMLSelectElement(iface);
    nsIWritableVariant *nsvariant;
    HTMLElement *element_obj;
    nsresult nsres;

    TRACE("(%p)->(%p %s)\n", This, element, debugstr_variant(&before));

    element_obj = unsafe_impl_from_IHTMLElement(element);
    if(!element_obj) {
        FIXME("External IHTMLElement implementation?\n");
        return E_INVALIDARG;
    }

    nsvariant = create_nsvariant();
    if(!nsvariant)
        return E_FAIL;

    switch(V_VT(&before)) {
    case VT_EMPTY:
    case VT_ERROR:
        nsres = nsIWritableVariant_SetAsEmpty(nsvariant);
        break;
    case VT_I2:
        nsres = nsIWritableVariant_SetAsInt16(nsvariant, V_I2(&before));
        break;
    default:
        FIXME("unhandled before %s\n", debugstr_variant(&before));
        nsIWritableVariant_Release(nsvariant);
        return E_NOTIMPL;
    }

    if(NS_SUCCEEDED(nsres))
        nsres = nsIDOMHTMLSelectElement_Add(This->nsselect, element_obj->nselem, (nsIVariant*)nsvariant);
    nsIWritableVariant_Release(nsvariant);
    if(NS_FAILED(nsres)) {
        ERR("Add failed: %08x\n", nsres);
        return E_FAIL;
    }

    return S_OK;
}

static HRESULT WINAPI HTMLSelectElement_remove(IHTMLSelectElement *iface, LONG index)
{
    HTMLSelectElement *This = impl_from_IHTMLSelectElement(iface);
    nsresult nsres;
    TRACE("(%p)->(%d)\n", This, index);
    if(index < 0)
        return E_INVALIDARG;

    nsres = nsIDOMHTMLSelectElement_select_Remove(This->nsselect, index);
    if(NS_FAILED(nsres)) {
        ERR("Remove failed: %08x\n", nsres);
        return E_FAIL;
    }
    return S_OK;
}

static HRESULT WINAPI HTMLSelectElement_put_length(IHTMLSelectElement *iface, LONG v)
{
    HTMLSelectElement *This = impl_from_IHTMLSelectElement(iface);
    nsresult nsres;

    TRACE("(%p)->(%d)\n", This, v);

    nsres = nsIDOMHTMLSelectElement_SetLength(This->nsselect, v);
    if(NS_FAILED(nsres))
        ERR("SetLength failed: %08x\n", nsres);

    return S_OK;
}

static HRESULT WINAPI HTMLSelectElement_get_length(IHTMLSelectElement *iface, LONG *p)
{
    HTMLSelectElement *This = impl_from_IHTMLSelectElement(iface);
    UINT32 length = 0;
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
    HTMLSelectElement *This = impl_from_IHTMLSelectElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLSelectElement_item(IHTMLSelectElement *iface, VARIANT name,
                                             VARIANT index, IDispatch **pdisp)
{
    HTMLSelectElement *This = impl_from_IHTMLSelectElement(iface);

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
    HTMLSelectElement *This = impl_from_IHTMLSelectElement(iface);
    FIXME("(%p)->(v %p)\n", This, pdisp);
    return E_NOTIMPL;
}

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

static inline HTMLSelectElement *impl_from_HTMLDOMNode(HTMLDOMNode *iface)
{
    return CONTAINING_RECORD(iface, HTMLSelectElement, element.node);
}

static HRESULT HTMLSelectElement_QI(HTMLDOMNode *iface, REFIID riid, void **ppv)
{
    HTMLSelectElement *This = impl_from_HTMLDOMNode(iface);

    *ppv = NULL;

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = &This->IHTMLSelectElement_iface;
    }else if(IsEqualGUID(&IID_IDispatch, riid)) {
        TRACE("(%p)->(IID_IDispatch %p)\n", This, ppv);
        *ppv = &This->IHTMLSelectElement_iface;
    }else if(IsEqualGUID(&IID_IHTMLSelectElement, riid)) {
        TRACE("(%p)->(IID_IHTMLSelectElement %p)\n", This, ppv);
        *ppv = &This->IHTMLSelectElement_iface;
    }

    if(*ppv) {
        IUnknown_AddRef((IUnknown*)*ppv);
        return S_OK;
    }

    return HTMLElement_QI(&This->element.node, riid, ppv);
}

static HRESULT HTMLSelectElementImpl_put_disabled(HTMLDOMNode *iface, VARIANT_BOOL v)
{
    HTMLSelectElement *This = impl_from_HTMLDOMNode(iface);
    return IHTMLSelectElement_put_disabled(&This->IHTMLSelectElement_iface, v);
}

static HRESULT HTMLSelectElementImpl_get_disabled(HTMLDOMNode *iface, VARIANT_BOOL *p)
{
    HTMLSelectElement *This = impl_from_HTMLDOMNode(iface);
    return IHTMLSelectElement_get_disabled(&This->IHTMLSelectElement_iface, p);
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
    HTMLSelectElement *This = impl_from_HTMLDOMNode(iface);

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

static void HTMLSelectElement_traverse(HTMLDOMNode *iface, nsCycleCollectionTraversalCallback *cb)
{
    HTMLSelectElement *This = impl_from_HTMLDOMNode(iface);

    if(This->nsselect)
        note_cc_edge((nsISupports*)This->nsselect, "This->nsselect", cb);
}

static void HTMLSelectElement_unlink(HTMLDOMNode *iface)
{
    HTMLSelectElement *This = impl_from_HTMLDOMNode(iface);

    if(This->nsselect) {
        nsIDOMHTMLSelectElement *nsselect = This->nsselect;

        This->nsselect = NULL;
        nsIDOMHTMLSelectElement_Release(nsselect);
    }
}

static const NodeImplVtbl HTMLSelectElementImplVtbl = {
    HTMLSelectElement_QI,
    HTMLElement_destructor,
    HTMLElement_cpc,
    HTMLElement_clone,
    HTMLElement_handle_event,
    HTMLElement_get_attr_col,
    NULL,
    NULL,
    HTMLSelectElementImpl_put_disabled,
    HTMLSelectElementImpl_get_disabled,
    NULL,
    NULL,
    HTMLSelectElement_get_dispid,
    HTMLSelectElement_invoke,
    NULL,
    HTMLSelectElement_traverse,
    HTMLSelectElement_unlink
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

HRESULT HTMLSelectElement_Create(HTMLDocumentNode *doc, nsIDOMHTMLElement *nselem, HTMLElement **elem)
{
    HTMLSelectElement *ret;
    nsresult nsres;

    ret = heap_alloc_zero(sizeof(HTMLSelectElement));
    if(!ret)
        return E_OUTOFMEMORY;

    ret->IHTMLSelectElement_iface.lpVtbl = &HTMLSelectElementVtbl;
    ret->element.node.vtbl = &HTMLSelectElementImplVtbl;

    HTMLElement_Init(&ret->element, doc, nselem, &HTMLSelectElement_dispex);

    nsres = nsIDOMHTMLElement_QueryInterface(nselem, &IID_nsIDOMHTMLSelectElement,
                                             (void**)&ret->nsselect);
    assert(nsres == NS_OK);

    *elem = &ret->element;
    return S_OK;
}
