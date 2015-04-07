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

    IHTMLTextAreaElement IHTMLTextAreaElement_iface;

    nsIDOMHTMLTextAreaElement *nstextarea;
} HTMLTextAreaElement;

static inline HTMLTextAreaElement *impl_from_IHTMLTextAreaElement(IHTMLTextAreaElement *iface)
{
    return CONTAINING_RECORD(iface, HTMLTextAreaElement, IHTMLTextAreaElement_iface);
}

static HRESULT WINAPI HTMLTextAreaElement_QueryInterface(IHTMLTextAreaElement *iface,
                                                         REFIID riid, void **ppv)
{
    HTMLTextAreaElement *This = impl_from_IHTMLTextAreaElement(iface);

    return IHTMLDOMNode_QueryInterface(&This->element.node.IHTMLDOMNode_iface, riid, ppv);
}

static ULONG WINAPI HTMLTextAreaElement_AddRef(IHTMLTextAreaElement *iface)
{
    HTMLTextAreaElement *This = impl_from_IHTMLTextAreaElement(iface);

    return IHTMLDOMNode_AddRef(&This->element.node.IHTMLDOMNode_iface);
}

static ULONG WINAPI HTMLTextAreaElement_Release(IHTMLTextAreaElement *iface)
{
    HTMLTextAreaElement *This = impl_from_IHTMLTextAreaElement(iface);

    return IHTMLDOMNode_Release(&This->element.node.IHTMLDOMNode_iface);
}

static HRESULT WINAPI HTMLTextAreaElement_GetTypeInfoCount(IHTMLTextAreaElement *iface, UINT *pctinfo)
{
    HTMLTextAreaElement *This = impl_from_IHTMLTextAreaElement(iface);
    return IDispatchEx_GetTypeInfoCount(&This->element.node.dispex.IDispatchEx_iface, pctinfo);
}

static HRESULT WINAPI HTMLTextAreaElement_GetTypeInfo(IHTMLTextAreaElement *iface, UINT iTInfo,
                                              LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLTextAreaElement *This = impl_from_IHTMLTextAreaElement(iface);
    return IDispatchEx_GetTypeInfo(&This->element.node.dispex.IDispatchEx_iface, iTInfo, lcid,
            ppTInfo);
}

static HRESULT WINAPI HTMLTextAreaElement_GetIDsOfNames(IHTMLTextAreaElement *iface, REFIID riid,
                                                LPOLESTR *rgszNames, UINT cNames,
                                                LCID lcid, DISPID *rgDispId)
{
    HTMLTextAreaElement *This = impl_from_IHTMLTextAreaElement(iface);
    return IDispatchEx_GetIDsOfNames(&This->element.node.dispex.IDispatchEx_iface, riid, rgszNames,
            cNames, lcid, rgDispId);
}

static HRESULT WINAPI HTMLTextAreaElement_Invoke(IHTMLTextAreaElement *iface, DISPID dispIdMember,
                            REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
                            VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLTextAreaElement *This = impl_from_IHTMLTextAreaElement(iface);
    return IDispatchEx_Invoke(&This->element.node.dispex.IDispatchEx_iface, dispIdMember, riid,
            lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI HTMLTextAreaElement_get_type(IHTMLTextAreaElement *iface, BSTR *p)
{
    static const WCHAR textareaW[] = {'t','e','x','t','a','r','e','a',0};

    HTMLTextAreaElement *This = impl_from_IHTMLTextAreaElement(iface);

    TRACE("(%p)->(%p)\n", This, p);

    *p = SysAllocString(textareaW);
    if(!*p)
        return E_OUTOFMEMORY;
    return S_OK;
}

static HRESULT WINAPI HTMLTextAreaElement_put_value(IHTMLTextAreaElement *iface, BSTR v)
{
    HTMLTextAreaElement *This = impl_from_IHTMLTextAreaElement(iface);
    nsAString value_str;
    nsresult nsres;

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    nsAString_InitDepend(&value_str, v);
    nsres = nsIDOMHTMLTextAreaElement_SetValue(This->nstextarea, &value_str);
    nsAString_Finish(&value_str);
    if(NS_FAILED(nsres)) {
        ERR("SetValue failed: %08x\n", nsres);
        return E_FAIL;
    }

    return S_OK;
}

static HRESULT WINAPI HTMLTextAreaElement_get_value(IHTMLTextAreaElement *iface, BSTR *p)
{
    HTMLTextAreaElement *This = impl_from_IHTMLTextAreaElement(iface);
    nsAString value_str;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsAString_Init(&value_str, NULL);
    nsres = nsIDOMHTMLTextAreaElement_GetValue(This->nstextarea, &value_str);
    return return_nsstr(nsres, &value_str, p);
}

static HRESULT WINAPI HTMLTextAreaElement_put_name(IHTMLTextAreaElement *iface, BSTR v)
{
    HTMLTextAreaElement *This = impl_from_IHTMLTextAreaElement(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTextAreaElement_get_name(IHTMLTextAreaElement *iface, BSTR *p)
{
    HTMLTextAreaElement *This = impl_from_IHTMLTextAreaElement(iface);
    nsAString name_str;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsAString_Init(&name_str, NULL);
    nsres = nsIDOMHTMLTextAreaElement_GetName(This->nstextarea, &name_str);
    return return_nsstr(nsres, &name_str, p);
}

static HRESULT WINAPI HTMLTextAreaElement_put_status(IHTMLTextAreaElement *iface, VARIANT v)
{
    HTMLTextAreaElement *This = impl_from_IHTMLTextAreaElement(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTextAreaElement_get_status(IHTMLTextAreaElement *iface, VARIANT *p)
{
    HTMLTextAreaElement *This = impl_from_IHTMLTextAreaElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTextAreaElement_put_disabled(IHTMLTextAreaElement *iface, VARIANT_BOOL v)
{
    HTMLTextAreaElement *This = impl_from_IHTMLTextAreaElement(iface);
    FIXME("(%p)->(%x)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTextAreaElement_get_disabled(IHTMLTextAreaElement *iface, VARIANT_BOOL *p)
{
    HTMLTextAreaElement *This = impl_from_IHTMLTextAreaElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTextAreaElement_get_form(IHTMLTextAreaElement *iface, IHTMLFormElement **p)
{
    HTMLTextAreaElement *This = impl_from_IHTMLTextAreaElement(iface);
    nsIDOMHTMLFormElement *nsform;
    nsIDOMNode *nsnode;
    HTMLDOMNode *node;
    nsresult nsres;
    HRESULT hres;

    TRACE("(%p)->(%p)\n", This, p);

    nsres = nsIDOMHTMLTextAreaElement_GetForm(This->nstextarea, &nsform);
    assert(nsres == NS_OK);

    if(!nsform) {
        *p = NULL;
        return S_OK;
    }

    nsres = nsIDOMHTMLFormElement_QueryInterface(nsform, &IID_nsIDOMNode, (void**)&nsnode);
    nsIDOMHTMLFormElement_Release(nsform);
    assert(nsres == NS_OK);

    hres = get_node(This->element.node.doc, nsnode, TRUE, &node);
    nsIDOMNode_Release(nsnode);
    if(FAILED(hres))
        return hres;

    hres = IHTMLDOMNode_QueryInterface(&node->IHTMLDOMNode_iface, &IID_IHTMLFormElement, (void**)p);
    IHTMLDOMNode_Release(&node->IHTMLDOMNode_iface);
    return hres;
}

static HRESULT WINAPI HTMLTextAreaElement_put_defaultValue(IHTMLTextAreaElement *iface, BSTR v)
{
    HTMLTextAreaElement *This = impl_from_IHTMLTextAreaElement(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTextAreaElement_get_defaultValue(IHTMLTextAreaElement *iface, BSTR *p)
{
    HTMLTextAreaElement *This = impl_from_IHTMLTextAreaElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTextAreaElement_select(IHTMLTextAreaElement *iface)
{
    HTMLTextAreaElement *This = impl_from_IHTMLTextAreaElement(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTextAreaElement_put_onchange(IHTMLTextAreaElement *iface, VARIANT v)
{
    HTMLTextAreaElement *This = impl_from_IHTMLTextAreaElement(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTextAreaElement_get_onchange(IHTMLTextAreaElement *iface, VARIANT *p)
{
    HTMLTextAreaElement *This = impl_from_IHTMLTextAreaElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTextAreaElement_put_onselect(IHTMLTextAreaElement *iface, VARIANT v)
{
    HTMLTextAreaElement *This = impl_from_IHTMLTextAreaElement(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTextAreaElement_get_onselect(IHTMLTextAreaElement *iface, VARIANT *p)
{
    HTMLTextAreaElement *This = impl_from_IHTMLTextAreaElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTextAreaElement_put_readOnly(IHTMLTextAreaElement *iface, VARIANT_BOOL v)
{
    HTMLTextAreaElement *This = impl_from_IHTMLTextAreaElement(iface);
    nsresult nsres;

    TRACE("(%p)->(%x)\n", This, v);

    nsres = nsIDOMHTMLTextAreaElement_SetReadOnly(This->nstextarea, v != VARIANT_FALSE);
    if(NS_FAILED(nsres)) {
        ERR("SetReadOnly failed: %08x\n", nsres);
        return E_FAIL;
    }

    return S_OK;
}

static HRESULT WINAPI HTMLTextAreaElement_get_readOnly(IHTMLTextAreaElement *iface, VARIANT_BOOL *p)
{
    HTMLTextAreaElement *This = impl_from_IHTMLTextAreaElement(iface);
    cpp_bool b;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsres = nsIDOMHTMLTextAreaElement_GetReadOnly(This->nstextarea, &b);
    if(NS_FAILED(nsres)) {
        ERR("GetReadOnly failed: %08x\n", nsres);
        return E_FAIL;
    }

    *p = b ? VARIANT_TRUE : VARIANT_FALSE;
    return S_OK;
}

static HRESULT WINAPI HTMLTextAreaElement_put_rows(IHTMLTextAreaElement *iface, LONG v)
{
    HTMLTextAreaElement *This = impl_from_IHTMLTextAreaElement(iface);
    FIXME("(%p)->(%d)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTextAreaElement_get_rows(IHTMLTextAreaElement *iface, LONG *p)
{
    HTMLTextAreaElement *This = impl_from_IHTMLTextAreaElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTextAreaElement_put_cols(IHTMLTextAreaElement *iface, LONG v)
{
    HTMLTextAreaElement *This = impl_from_IHTMLTextAreaElement(iface);
    FIXME("(%p)->(%d)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTextAreaElement_get_cols(IHTMLTextAreaElement *iface, LONG *p)
{
    HTMLTextAreaElement *This = impl_from_IHTMLTextAreaElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTextAreaElement_put_wrap(IHTMLTextAreaElement *iface, BSTR v)
{
    HTMLTextAreaElement *This = impl_from_IHTMLTextAreaElement(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTextAreaElement_get_wrap(IHTMLTextAreaElement *iface, BSTR *p)
{
    HTMLTextAreaElement *This = impl_from_IHTMLTextAreaElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTextAreaElement_createTextRange(IHTMLTextAreaElement *iface,
                                                          IHTMLTxtRange **range)
{
    HTMLTextAreaElement *This = impl_from_IHTMLTextAreaElement(iface);
    FIXME("(%p)->(%p)\n", This, range);
    return E_NOTIMPL;
}

static const IHTMLTextAreaElementVtbl HTMLTextAreaElementVtbl = {
    HTMLTextAreaElement_QueryInterface,
    HTMLTextAreaElement_AddRef,
    HTMLTextAreaElement_Release,
    HTMLTextAreaElement_GetTypeInfoCount,
    HTMLTextAreaElement_GetTypeInfo,
    HTMLTextAreaElement_GetIDsOfNames,
    HTMLTextAreaElement_Invoke,
    HTMLTextAreaElement_get_type,
    HTMLTextAreaElement_put_value,
    HTMLTextAreaElement_get_value,
    HTMLTextAreaElement_put_name,
    HTMLTextAreaElement_get_name,
    HTMLTextAreaElement_put_status,
    HTMLTextAreaElement_get_status,
    HTMLTextAreaElement_put_disabled,
    HTMLTextAreaElement_get_disabled,
    HTMLTextAreaElement_get_form,
    HTMLTextAreaElement_put_defaultValue,
    HTMLTextAreaElement_get_defaultValue,
    HTMLTextAreaElement_select,
    HTMLTextAreaElement_put_onchange,
    HTMLTextAreaElement_get_onchange,
    HTMLTextAreaElement_put_onselect,
    HTMLTextAreaElement_get_onselect,
    HTMLTextAreaElement_put_readOnly,
    HTMLTextAreaElement_get_readOnly,
    HTMLTextAreaElement_put_rows,
    HTMLTextAreaElement_get_rows,
    HTMLTextAreaElement_put_cols,
    HTMLTextAreaElement_get_cols,
    HTMLTextAreaElement_put_wrap,
    HTMLTextAreaElement_get_wrap,
    HTMLTextAreaElement_createTextRange
};

static inline HTMLTextAreaElement *impl_from_HTMLDOMNode(HTMLDOMNode *iface)
{
    return CONTAINING_RECORD(iface, HTMLTextAreaElement, element.node);
}

static HRESULT HTMLTextAreaElement_QI(HTMLDOMNode *iface, REFIID riid, void **ppv)
{
    HTMLTextAreaElement *This = impl_from_HTMLDOMNode(iface);

    *ppv = NULL;

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = &This->IHTMLTextAreaElement_iface;
    }else if(IsEqualGUID(&IID_IDispatch, riid)) {
        TRACE("(%p)->(IID_IDispatch %p)\n", This, ppv);
        *ppv = &This->IHTMLTextAreaElement_iface;
    }else if(IsEqualGUID(&IID_IHTMLTextAreaElement, riid)) {
        TRACE("(%p)->(IID_IHTMLTextAreaElement %p)\n", This, ppv);
        *ppv = &This->IHTMLTextAreaElement_iface;
    }

    if(*ppv) {
        IUnknown_AddRef((IUnknown*)*ppv);
        return S_OK;
    }

    return HTMLElement_QI(&This->element.node, riid, ppv);
}

static HRESULT HTMLTextAreaElementImpl_put_disabled(HTMLDOMNode *iface, VARIANT_BOOL v)
{
    HTMLTextAreaElement *This = impl_from_HTMLDOMNode(iface);
    return IHTMLTextAreaElement_put_disabled(&This->IHTMLTextAreaElement_iface, v);
}

static HRESULT HTMLTextAreaElementImpl_get_disabled(HTMLDOMNode *iface, VARIANT_BOOL *p)
{
    HTMLTextAreaElement *This = impl_from_HTMLDOMNode(iface);
    return IHTMLTextAreaElement_get_disabled(&This->IHTMLTextAreaElement_iface, p);
}

static BOOL HTMLTextAreaElement_is_text_edit(HTMLDOMNode *iface)
{
    return TRUE;
}

static void HTMLTextAreaElement_traverse(HTMLDOMNode *iface, nsCycleCollectionTraversalCallback *cb)
{
    HTMLTextAreaElement *This = impl_from_HTMLDOMNode(iface);

    if(This->nstextarea)
        note_cc_edge((nsISupports*)This->nstextarea, "This->nstextarea", cb);
}

static void HTMLTextAreaElement_unlink(HTMLDOMNode *iface)
{
    HTMLTextAreaElement *This = impl_from_HTMLDOMNode(iface);

    if(This->nstextarea) {
        nsIDOMHTMLTextAreaElement *nstextarea = This->nstextarea;

        This->nstextarea = NULL;
        nsIDOMHTMLTextAreaElement_Release(nstextarea);
    }
}

static const NodeImplVtbl HTMLTextAreaElementImplVtbl = {
    HTMLTextAreaElement_QI,
    HTMLElement_destructor,
    HTMLElement_cpc,
    HTMLElement_clone,
    HTMLElement_handle_event,
    HTMLElement_get_attr_col,
    NULL,
    NULL,
    HTMLTextAreaElementImpl_put_disabled,
    HTMLTextAreaElementImpl_get_disabled,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    HTMLTextAreaElement_traverse,
    HTMLTextAreaElement_unlink,
    HTMLTextAreaElement_is_text_edit
};

static const tid_t HTMLTextAreaElement_iface_tids[] = {
    HTMLELEMENT_TIDS,
    IHTMLTextAreaElement_tid,
    0
};

static dispex_static_data_t HTMLTextAreaElement_dispex = {
    NULL,
    DispHTMLTextAreaElement_tid,
    NULL,
    HTMLTextAreaElement_iface_tids
};

HRESULT HTMLTextAreaElement_Create(HTMLDocumentNode *doc, nsIDOMHTMLElement *nselem, HTMLElement **elem)
{
    HTMLTextAreaElement *ret;
    nsresult nsres;

    ret = heap_alloc_zero(sizeof(HTMLTextAreaElement));
    if(!ret)
        return E_OUTOFMEMORY;

    ret->IHTMLTextAreaElement_iface.lpVtbl = &HTMLTextAreaElementVtbl;
    ret->element.node.vtbl = &HTMLTextAreaElementImplVtbl;

    HTMLElement_Init(&ret->element, doc, nselem, &HTMLTextAreaElement_dispex);

    nsres = nsIDOMHTMLElement_QueryInterface(nselem, &IID_nsIDOMHTMLTextAreaElement, (void**)&ret->nstextarea);
    assert(nsres == NS_OK);

    *elem = &ret->element;
    return S_OK;
}
