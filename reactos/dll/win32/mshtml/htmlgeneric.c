/*
 * Copyright 2008 Jacek Caban for CodeWeavers
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

    IHTMLGenericElement IHTMLGenericElement_iface;
} HTMLGenericElement;

static inline HTMLGenericElement *impl_from_IHTMLGenericElement(IHTMLGenericElement *iface)
{
    return CONTAINING_RECORD(iface, HTMLGenericElement, IHTMLGenericElement_iface);
}

static HRESULT WINAPI HTMLGenericElement_QueryInterface(IHTMLGenericElement *iface, REFIID riid, void **ppv)
{
    HTMLGenericElement *This = impl_from_IHTMLGenericElement(iface);

    return IHTMLDOMNode_QueryInterface(&This->element.node.IHTMLDOMNode_iface, riid, ppv);
}

static ULONG WINAPI HTMLGenericElement_AddRef(IHTMLGenericElement *iface)
{
    HTMLGenericElement *This = impl_from_IHTMLGenericElement(iface);

    return IHTMLDOMNode_AddRef(&This->element.node.IHTMLDOMNode_iface);
}

static ULONG WINAPI HTMLGenericElement_Release(IHTMLGenericElement *iface)
{
    HTMLGenericElement *This = impl_from_IHTMLGenericElement(iface);

    return IHTMLDOMNode_Release(&This->element.node.IHTMLDOMNode_iface);
}

static HRESULT WINAPI HTMLGenericElement_GetTypeInfoCount(IHTMLGenericElement *iface, UINT *pctinfo)
{
    HTMLGenericElement *This = impl_from_IHTMLGenericElement(iface);
    return IDispatchEx_GetTypeInfoCount(&This->element.node.dispex.IDispatchEx_iface, pctinfo);
}

static HRESULT WINAPI HTMLGenericElement_GetTypeInfo(IHTMLGenericElement *iface, UINT iTInfo,
                                              LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLGenericElement *This = impl_from_IHTMLGenericElement(iface);
    return IDispatchEx_GetTypeInfo(&This->element.node.dispex.IDispatchEx_iface, iTInfo, lcid,
            ppTInfo);
}

static HRESULT WINAPI HTMLGenericElement_GetIDsOfNames(IHTMLGenericElement *iface, REFIID riid,
        LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{
    HTMLGenericElement *This = impl_from_IHTMLGenericElement(iface);
    return IDispatchEx_GetIDsOfNames(&This->element.node.dispex.IDispatchEx_iface, riid, rgszNames,
            cNames, lcid, rgDispId);
}

static HRESULT WINAPI HTMLGenericElement_Invoke(IHTMLGenericElement *iface, DISPID dispIdMember,
        REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
        VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLGenericElement *This = impl_from_IHTMLGenericElement(iface);
    return IDispatchEx_Invoke(&This->element.node.dispex.IDispatchEx_iface, dispIdMember, riid,
            lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI HTMLGenericElement_get_recordset(IHTMLGenericElement *iface, IDispatch **p)
{
    HTMLGenericElement *This = impl_from_IHTMLGenericElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLGenericElement_namedRecordset(IHTMLGenericElement *iface,
        BSTR dataMember, VARIANT *hierarchy, IDispatch **ppRecordset)
{
    HTMLGenericElement *This = impl_from_IHTMLGenericElement(iface);
    FIXME("(%p)->(%s %p %p)\n", This, debugstr_w(dataMember), hierarchy, ppRecordset);
    return E_NOTIMPL;
}

static const IHTMLGenericElementVtbl HTMLGenericElementVtbl = {
    HTMLGenericElement_QueryInterface,
    HTMLGenericElement_AddRef,
    HTMLGenericElement_Release,
    HTMLGenericElement_GetTypeInfoCount,
    HTMLGenericElement_GetTypeInfo,
    HTMLGenericElement_GetIDsOfNames,
    HTMLGenericElement_Invoke,
    HTMLGenericElement_get_recordset,
    HTMLGenericElement_namedRecordset
};

static inline HTMLGenericElement *impl_from_HTMLDOMNode(HTMLDOMNode *iface)
{
    return CONTAINING_RECORD(iface, HTMLGenericElement, element.node);
}

static HRESULT HTMLGenericElement_QI(HTMLDOMNode *iface, REFIID riid, void **ppv)
{
    HTMLGenericElement *This = impl_from_HTMLDOMNode(iface);

    *ppv = NULL;

    if(IsEqualGUID(&IID_IHTMLGenericElement, riid)) {
        TRACE("(%p)->(IID_IHTMLGenericElement %p)\n", This, ppv);
        *ppv = &This->IHTMLGenericElement_iface;
    }else {
        return HTMLElement_QI(&This->element.node, riid, ppv);
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static void HTMLGenericElement_destructor(HTMLDOMNode *iface)
{
    HTMLGenericElement *This = impl_from_HTMLDOMNode(iface);

    HTMLElement_destructor(&This->element.node);
}

static const NodeImplVtbl HTMLGenericElementImplVtbl = {
    HTMLGenericElement_QI,
    HTMLGenericElement_destructor,
    HTMLElement_cpc,
    HTMLElement_clone,
    HTMLElement_handle_event,
    HTMLElement_get_attr_col
};

static const tid_t HTMLGenericElement_iface_tids[] = {
    HTMLELEMENT_TIDS,
    IHTMLGenericElement_tid,
    0
};

static dispex_static_data_t HTMLGenericElement_dispex = {
    NULL,
    DispHTMLGenericElement_tid,
    NULL,
    HTMLGenericElement_iface_tids
};

HRESULT HTMLGenericElement_Create(HTMLDocumentNode *doc, nsIDOMHTMLElement *nselem, HTMLElement **elem)
{
    HTMLGenericElement *ret;

    ret = heap_alloc_zero(sizeof(HTMLGenericElement));
    if(!ret)
        return E_OUTOFMEMORY;

    ret->IHTMLGenericElement_iface.lpVtbl = &HTMLGenericElementVtbl;
    ret->element.node.vtbl = &HTMLGenericElementImplVtbl;

    HTMLElement_Init(&ret->element, doc, nselem, &HTMLGenericElement_dispex);

    *elem = &ret->element;
    return S_OK;
}
