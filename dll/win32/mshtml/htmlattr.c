/*
 * Copyright 2011 Jacek Caban for CodeWeavers
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

static inline HTMLDOMAttribute *impl_from_IHTMLDOMAttribute(IHTMLDOMAttribute *iface)
{
    return CONTAINING_RECORD(iface, HTMLDOMAttribute, IHTMLDOMAttribute_iface);
}

static HRESULT WINAPI HTMLDOMAttribute_QueryInterface(IHTMLDOMAttribute *iface,
                                                 REFIID riid, void **ppv)
{
    HTMLDOMAttribute *This = impl_from_IHTMLDOMAttribute(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_mshtml_guid(riid), ppv);

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        *ppv = &This->IHTMLDOMAttribute_iface;
    }else if(IsEqualGUID(&IID_IHTMLDOMAttribute, riid)) {
        *ppv = &This->IHTMLDOMAttribute_iface;
    }else if(dispex_query_interface(&This->dispex, riid, ppv)) {
        return *ppv ? S_OK : E_NOINTERFACE;
    }else {
        WARN("%s not supported\n", debugstr_mshtml_guid(riid));
        *ppv =  NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI HTMLDOMAttribute_AddRef(IHTMLDOMAttribute *iface)
{
    HTMLDOMAttribute *This = impl_from_IHTMLDOMAttribute(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    return ref;
}

static ULONG WINAPI HTMLDOMAttribute_Release(IHTMLDOMAttribute *iface)
{
    HTMLDOMAttribute *This = impl_from_IHTMLDOMAttribute(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    if(!ref) {
        assert(!This->elem);
        release_dispex(&This->dispex);
        heap_free(This->name);
        heap_free(This);
    }

    return ref;
}

static HRESULT WINAPI HTMLDOMAttribute_GetTypeInfoCount(IHTMLDOMAttribute *iface, UINT *pctinfo)
{
    HTMLDOMAttribute *This = impl_from_IHTMLDOMAttribute(iface);
    return IDispatchEx_GetTypeInfoCount(&This->dispex.IDispatchEx_iface, pctinfo);
}

static HRESULT WINAPI HTMLDOMAttribute_GetTypeInfo(IHTMLDOMAttribute *iface, UINT iTInfo,
                                              LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLDOMAttribute *This = impl_from_IHTMLDOMAttribute(iface);
    return IDispatchEx_GetTypeInfo(&This->dispex.IDispatchEx_iface, iTInfo, lcid, ppTInfo);
}

static HRESULT WINAPI HTMLDOMAttribute_GetIDsOfNames(IHTMLDOMAttribute *iface, REFIID riid,
                                                LPOLESTR *rgszNames, UINT cNames,
                                                LCID lcid, DISPID *rgDispId)
{
    HTMLDOMAttribute *This = impl_from_IHTMLDOMAttribute(iface);
    return IDispatchEx_GetIDsOfNames(&This->dispex.IDispatchEx_iface, riid, rgszNames, cNames,
            lcid, rgDispId);
}

static HRESULT WINAPI HTMLDOMAttribute_Invoke(IHTMLDOMAttribute *iface, DISPID dispIdMember,
                            REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
                            VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLDOMAttribute *This = impl_from_IHTMLDOMAttribute(iface);
    return IDispatchEx_Invoke(&This->dispex.IDispatchEx_iface, dispIdMember, riid, lcid,
            wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI HTMLDOMAttribute_get_nodeName(IHTMLDOMAttribute *iface, BSTR *p)
{
    HTMLDOMAttribute *This = impl_from_IHTMLDOMAttribute(iface);

    TRACE("(%p)->(%p)\n", This, p);

    if(!This->elem) {
        if(!This->name) {
            FIXME("No name available\n");
            return E_FAIL;
        }

        *p = SysAllocString(This->name);
        return *p ? S_OK : E_OUTOFMEMORY;
    }

    return IDispatchEx_GetMemberName(&This->elem->node.dispex.IDispatchEx_iface, This->dispid, p);
}

static HRESULT WINAPI HTMLDOMAttribute_put_nodeValue(IHTMLDOMAttribute *iface, VARIANT v)
{
    HTMLDOMAttribute *This = impl_from_IHTMLDOMAttribute(iface);
    DISPID dispidNamed = DISPID_PROPERTYPUT;
    DISPPARAMS dp = {&v, &dispidNamed, 1, 1};
    EXCEPINFO ei;
    VARIANT ret;

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    if(!This->elem) {
        FIXME("NULL This->elem\n");
        return E_UNEXPECTED;
    }

    memset(&ei, 0, sizeof(ei));

    return IDispatchEx_InvokeEx(&This->elem->node.dispex.IDispatchEx_iface, This->dispid, LOCALE_SYSTEM_DEFAULT,
            DISPATCH_PROPERTYPUT, &dp, &ret, &ei, NULL);
}

static HRESULT WINAPI HTMLDOMAttribute_get_nodeValue(IHTMLDOMAttribute *iface, VARIANT *p)
{
    HTMLDOMAttribute *This = impl_from_IHTMLDOMAttribute(iface);
    DISPPARAMS dp = {NULL, NULL, 0, 0};
    EXCEPINFO ei;

    TRACE("(%p)->(%p)\n", This, p);

    if(!This->elem) {
        FIXME("NULL This->elem\n");
        return E_UNEXPECTED;
    }

    memset(&ei, 0, sizeof(ei));
    return IDispatchEx_InvokeEx(&This->elem->node.dispex.IDispatchEx_iface, This->dispid, LOCALE_SYSTEM_DEFAULT,
            DISPATCH_PROPERTYGET, &dp, p, &ei, NULL);
}

static HRESULT WINAPI HTMLDOMAttribute_get_specified(IHTMLDOMAttribute *iface, VARIANT_BOOL *p)
{
    HTMLDOMAttribute *This = impl_from_IHTMLDOMAttribute(iface);
    nsIDOMAttr *nsattr;
    nsAString nsname;
    BSTR name;
    nsresult nsres;
    HRESULT hres;

    TRACE("(%p)->(%p)\n", This, p);

    if(!This->elem || !This->elem->nselem) {
        FIXME("NULL This->elem\n");
        return E_UNEXPECTED;
    }

    if(get_dispid_type(This->dispid) != DISPEXPROP_BUILTIN) {
        *p = VARIANT_TRUE;
        return S_OK;
    }

    hres = IDispatchEx_GetMemberName(&This->elem->node.dispex.IDispatchEx_iface, This->dispid, &name);
    if(FAILED(hres))
        return hres;

    /* FIXME: This is not exactly right, we have some attributes that don't map directly to Gecko attributes. */
    nsAString_InitDepend(&nsname, name);
    nsres = nsIDOMHTMLElement_GetAttributeNode(This->elem->nselem, &nsname, &nsattr);
    nsAString_Finish(&nsname);
    SysFreeString(name);
    if(NS_FAILED(nsres))
        return E_FAIL;

    /* If the Gecko attribute node can be found, we know that the attribute is specified.
       There is no point in calling GetSpecified */
    if(nsattr) {
        nsIDOMAttr_Release(nsattr);
        *p = VARIANT_TRUE;
    }else {
        *p = VARIANT_FALSE;
    }
    return S_OK;
}

static const IHTMLDOMAttributeVtbl HTMLDOMAttributeVtbl = {
    HTMLDOMAttribute_QueryInterface,
    HTMLDOMAttribute_AddRef,
    HTMLDOMAttribute_Release,
    HTMLDOMAttribute_GetTypeInfoCount,
    HTMLDOMAttribute_GetTypeInfo,
    HTMLDOMAttribute_GetIDsOfNames,
    HTMLDOMAttribute_Invoke,
    HTMLDOMAttribute_get_nodeName,
    HTMLDOMAttribute_put_nodeValue,
    HTMLDOMAttribute_get_nodeValue,
    HTMLDOMAttribute_get_specified
};

static const tid_t HTMLDOMAttribute_iface_tids[] = {
    IHTMLDOMAttribute_tid,
    0
};
static dispex_static_data_t HTMLDOMAttribute_dispex = {
    NULL,
    DispHTMLDOMAttribute_tid,
    0,
    HTMLDOMAttribute_iface_tids
};

HRESULT HTMLDOMAttribute_Create(const WCHAR *name, HTMLElement *elem, DISPID dispid, HTMLDOMAttribute **attr)
{
    HTMLAttributeCollection *col;
    HTMLDOMAttribute *ret;
    HRESULT hres;

    ret = heap_alloc_zero(sizeof(*ret));
    if(!ret)
        return E_OUTOFMEMORY;

    ret->IHTMLDOMAttribute_iface.lpVtbl = &HTMLDOMAttributeVtbl;
    ret->ref = 1;
    ret->dispid = dispid;
    ret->elem = elem;

    init_dispex(&ret->dispex, (IUnknown*)&ret->IHTMLDOMAttribute_iface,
            &HTMLDOMAttribute_dispex);

    /* For attributes attached to an element, (elem,dispid) pair should be valid used for its operation. */
    if(elem) {
        hres = HTMLElement_get_attr_col(&elem->node, &col);
        if(FAILED(hres)) {
            IHTMLDOMAttribute_Release(&ret->IHTMLDOMAttribute_iface);
            return hres;
        }
        IHTMLAttributeCollection_Release(&col->IHTMLAttributeCollection_iface);

        list_add_tail(&elem->attrs->attrs, &ret->entry);
    }

    /* For detached attributes we may still do most operations if we have its name available. */
    if(name) {
        ret->name = heap_strdupW(name);
        if(!ret->name) {
            IHTMLDOMAttribute_Release(&ret->IHTMLDOMAttribute_iface);
            return E_OUTOFMEMORY;
        }
    }

    *attr = ret;
    return S_OK;
}
