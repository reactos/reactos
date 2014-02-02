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

#include "mshtml_private.h"

static inline HTMLDocument *impl_from_IHTMLDocument5(IHTMLDocument5 *iface)
{
    return CONTAINING_RECORD(iface, HTMLDocument, IHTMLDocument5_iface);
}

static HRESULT WINAPI HTMLDocument5_QueryInterface(IHTMLDocument5 *iface,
        REFIID riid, void **ppv)
{
    HTMLDocument *This = impl_from_IHTMLDocument5(iface);
    return htmldoc_query_interface(This, riid, ppv);
}

static ULONG WINAPI HTMLDocument5_AddRef(IHTMLDocument5 *iface)
{
    HTMLDocument *This = impl_from_IHTMLDocument5(iface);
    return htmldoc_addref(This);
}

static ULONG WINAPI HTMLDocument5_Release(IHTMLDocument5 *iface)
{
    HTMLDocument *This = impl_from_IHTMLDocument5(iface);
    return htmldoc_release(This);
}

static HRESULT WINAPI HTMLDocument5_GetTypeInfoCount(IHTMLDocument5 *iface, UINT *pctinfo)
{
    HTMLDocument *This = impl_from_IHTMLDocument5(iface);
    return IDispatchEx_GetTypeInfoCount(&This->IDispatchEx_iface, pctinfo);
}

static HRESULT WINAPI HTMLDocument5_GetTypeInfo(IHTMLDocument5 *iface, UINT iTInfo,
        LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLDocument *This = impl_from_IHTMLDocument5(iface);
    return IDispatchEx_GetTypeInfo(&This->IDispatchEx_iface, iTInfo, lcid, ppTInfo);
}

static HRESULT WINAPI HTMLDocument5_GetIDsOfNames(IHTMLDocument5 *iface, REFIID riid,
        LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{
    HTMLDocument *This = impl_from_IHTMLDocument5(iface);
    return IDispatchEx_GetIDsOfNames(&This->IDispatchEx_iface, riid, rgszNames, cNames, lcid,
            rgDispId);
}

static HRESULT WINAPI HTMLDocument5_Invoke(IHTMLDocument5 *iface, DISPID dispIdMember,
                            REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
                            VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLDocument *This = impl_from_IHTMLDocument5(iface);
    return IDispatchEx_Invoke(&This->IDispatchEx_iface, dispIdMember, riid, lcid, wFlags,
            pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI HTMLDocument5_put_onmousewheel(IHTMLDocument5 *iface, VARIANT v)
{
    HTMLDocument *This = impl_from_IHTMLDocument5(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument5_get_onmousewheel(IHTMLDocument5 *iface, VARIANT *p)
{
    HTMLDocument *This = impl_from_IHTMLDocument5(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument5_get_doctype(IHTMLDocument5 *iface, IHTMLDOMNode **p)
{
    HTMLDocument *This = impl_from_IHTMLDocument5(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument5_get_implementation(IHTMLDocument5 *iface, IHTMLDOMImplementation **p)
{
    HTMLDocument *This = impl_from_IHTMLDocument5(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument5_createAttribute(IHTMLDocument5 *iface, BSTR bstrattrName,
        IHTMLDOMAttribute **ppattribute)
{
    HTMLDocument *This = impl_from_IHTMLDocument5(iface);
    HTMLDOMAttribute *attr;
    HRESULT hres;

    TRACE("(%p)->(%s %p)\n", This, debugstr_w(bstrattrName), ppattribute);

    hres = HTMLDOMAttribute_Create(bstrattrName, NULL, 0, &attr);
    if(FAILED(hres))
        return hres;

    *ppattribute = &attr->IHTMLDOMAttribute_iface;
    return S_OK;
}

static HRESULT WINAPI HTMLDocument5_createComment(IHTMLDocument5 *iface, BSTR bstrdata,
        IHTMLDOMNode **ppRetNode)
{
    HTMLDocument *This = impl_from_IHTMLDocument5(iface);
    nsIDOMComment *nscomment;
    HTMLElement *elem;
    nsAString str;
    nsresult nsres;
    HRESULT hres;

    TRACE("(%p)->(%s %p)\n", This, debugstr_w(bstrdata), ppRetNode);

    if(!This->doc_node->nsdoc) {
        WARN("NULL nsdoc\n");
        return E_UNEXPECTED;
    }

    nsAString_InitDepend(&str, bstrdata);
    nsres = nsIDOMHTMLDocument_CreateComment(This->doc_node->nsdoc, &str, &nscomment);
    nsAString_Finish(&str);
    if(NS_FAILED(nsres)) {
        ERR("CreateTextNode failed: %08x\n", nsres);
        return E_FAIL;
    }

    hres = HTMLCommentElement_Create(This->doc_node, (nsIDOMNode*)nscomment, &elem);
    nsIDOMComment_Release(nscomment);
    if(FAILED(hres))
        return hres;

    *ppRetNode = &elem->node.IHTMLDOMNode_iface;
    return S_OK;
}

static HRESULT WINAPI HTMLDocument5_put_onfocusin(IHTMLDocument5 *iface, VARIANT v)
{
    HTMLDocument *This = impl_from_IHTMLDocument5(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument5_get_onfocusin(IHTMLDocument5 *iface, VARIANT *p)
{
    HTMLDocument *This = impl_from_IHTMLDocument5(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument5_put_onfocusout(IHTMLDocument5 *iface, VARIANT v)
{
    HTMLDocument *This = impl_from_IHTMLDocument5(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument5_get_onfocusout(IHTMLDocument5 *iface, VARIANT *p)
{
    HTMLDocument *This = impl_from_IHTMLDocument5(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument5_put_onactivate(IHTMLDocument5 *iface, VARIANT v)
{
    HTMLDocument *This = impl_from_IHTMLDocument5(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument5_get_onactivate(IHTMLDocument5 *iface, VARIANT *p)
{
    HTMLDocument *This = impl_from_IHTMLDocument5(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument5_put_ondeactivate(IHTMLDocument5 *iface, VARIANT v)
{
    HTMLDocument *This = impl_from_IHTMLDocument5(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument5_get_ondeactivate(IHTMLDocument5 *iface, VARIANT *p)
{
    HTMLDocument *This = impl_from_IHTMLDocument5(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument5_put_onbeforeactivate(IHTMLDocument5 *iface, VARIANT v)
{
    HTMLDocument *This = impl_from_IHTMLDocument5(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument5_get_onbeforeactivate(IHTMLDocument5 *iface, VARIANT *p)
{
    HTMLDocument *This = impl_from_IHTMLDocument5(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument5_put_onbeforedeactivate(IHTMLDocument5 *iface, VARIANT v)
{
    HTMLDocument *This = impl_from_IHTMLDocument5(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument5_get_onbeforedeactivate(IHTMLDocument5 *iface, VARIANT *p)
{
    HTMLDocument *This = impl_from_IHTMLDocument5(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument5_get_compatMode(IHTMLDocument5 *iface, BSTR *p)
{
    HTMLDocument *This = impl_from_IHTMLDocument5(iface);
    nsAString mode_str;
    const PRUnichar *mode;

    TRACE("(%p)->(%p)\n", This, p);

    if(!This->doc_node->nsdoc) {
        WARN("NULL nsdoc\n");
        return E_UNEXPECTED;
    }

    nsAString_Init(&mode_str, NULL);
    nsIDOMHTMLDocument_GetCompatMode(This->doc_node->nsdoc, &mode_str);

    nsAString_GetData(&mode_str, &mode);
    *p = SysAllocString(mode);
    nsAString_Finish(&mode_str);

    return S_OK;
}

static const IHTMLDocument5Vtbl HTMLDocument5Vtbl = {
    HTMLDocument5_QueryInterface,
    HTMLDocument5_AddRef,
    HTMLDocument5_Release,
    HTMLDocument5_GetTypeInfoCount,
    HTMLDocument5_GetTypeInfo,
    HTMLDocument5_GetIDsOfNames,
    HTMLDocument5_Invoke,
    HTMLDocument5_put_onmousewheel,
    HTMLDocument5_get_onmousewheel,
    HTMLDocument5_get_doctype,
    HTMLDocument5_get_implementation,
    HTMLDocument5_createAttribute,
    HTMLDocument5_createComment,
    HTMLDocument5_put_onfocusin,
    HTMLDocument5_get_onfocusin,
    HTMLDocument5_put_onfocusout,
    HTMLDocument5_get_onfocusout,
    HTMLDocument5_put_onactivate,
    HTMLDocument5_get_onactivate,
    HTMLDocument5_put_ondeactivate,
    HTMLDocument5_get_ondeactivate,
    HTMLDocument5_put_onbeforeactivate,
    HTMLDocument5_get_onbeforeactivate,
    HTMLDocument5_put_onbeforedeactivate,
    HTMLDocument5_get_onbeforedeactivate,
    HTMLDocument5_get_compatMode
};

static inline HTMLDocument *impl_from_IHTMLDocument6(IHTMLDocument6 *iface)
{
    return CONTAINING_RECORD(iface, HTMLDocument, IHTMLDocument6_iface);
}

static HRESULT WINAPI HTMLDocument6_QueryInterface(IHTMLDocument6 *iface,
        REFIID riid, void **ppv)
{
    HTMLDocument *This = impl_from_IHTMLDocument6(iface);
    return htmldoc_query_interface(This, riid, ppv);
}

static ULONG WINAPI HTMLDocument6_AddRef(IHTMLDocument6 *iface)
{
    HTMLDocument *This = impl_from_IHTMLDocument6(iface);
    return htmldoc_addref(This);
}

static ULONG WINAPI HTMLDocument6_Release(IHTMLDocument6 *iface)
{
    HTMLDocument *This = impl_from_IHTMLDocument6(iface);
    return htmldoc_release(This);
}

static HRESULT WINAPI HTMLDocument6_GetTypeInfoCount(IHTMLDocument6 *iface, UINT *pctinfo)
{
    HTMLDocument *This = impl_from_IHTMLDocument6(iface);
    return IDispatchEx_GetTypeInfoCount(&This->IDispatchEx_iface, pctinfo);
}

static HRESULT WINAPI HTMLDocument6_GetTypeInfo(IHTMLDocument6 *iface, UINT iTInfo,
        LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLDocument *This = impl_from_IHTMLDocument6(iface);
    return IDispatchEx_GetTypeInfo(&This->IDispatchEx_iface, iTInfo, lcid, ppTInfo);
}

static HRESULT WINAPI HTMLDocument6_GetIDsOfNames(IHTMLDocument6 *iface, REFIID riid,
        LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{
    HTMLDocument *This = impl_from_IHTMLDocument6(iface);
    return IDispatchEx_GetIDsOfNames(&This->IDispatchEx_iface, riid, rgszNames, cNames, lcid,
            rgDispId);
}

static HRESULT WINAPI HTMLDocument6_Invoke(IHTMLDocument6 *iface, DISPID dispIdMember,
                            REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
                            VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLDocument *This = impl_from_IHTMLDocument6(iface);
    return IDispatchEx_Invoke(&This->IDispatchEx_iface, dispIdMember, riid, lcid, wFlags,
            pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI HTMLDocument6_get_compatible(IHTMLDocument6 *iface,
        IHTMLDocumentCompatibleInfoCollection **p)
{
    HTMLDocument *This = impl_from_IHTMLDocument6(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument6_get_documentMode(IHTMLDocument6 *iface,
        VARIANT *p)
{
    HTMLDocument *This = impl_from_IHTMLDocument6(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument6_get_onstorage(IHTMLDocument6 *iface,
        VARIANT *p)
{
    HTMLDocument *This = impl_from_IHTMLDocument6(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument6_put_onstorage(IHTMLDocument6 *iface, VARIANT v)
{
    HTMLDocument *This = impl_from_IHTMLDocument6(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument6_get_onstoragecommit(IHTMLDocument6 *iface,
        VARIANT *p)
{
    HTMLDocument *This = impl_from_IHTMLDocument6(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument6_put_onstoragecommit(IHTMLDocument6 *iface, VARIANT v)
{
    HTMLDocument *This = impl_from_IHTMLDocument6(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument6_getElementById(IHTMLDocument6 *iface,
        BSTR bstrId, IHTMLElement2 **p)
{
    HTMLDocument *This = impl_from_IHTMLDocument6(iface);
    FIXME("(%p)->(%s %p)\n", This, debugstr_w(bstrId), p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument6_updateSettings(IHTMLDocument6 *iface)
{
    HTMLDocument *This = impl_from_IHTMLDocument6(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static const IHTMLDocument6Vtbl HTMLDocument6Vtbl = {
    HTMLDocument6_QueryInterface,
    HTMLDocument6_AddRef,
    HTMLDocument6_Release,
    HTMLDocument6_GetTypeInfoCount,
    HTMLDocument6_GetTypeInfo,
    HTMLDocument6_GetIDsOfNames,
    HTMLDocument6_Invoke,
    HTMLDocument6_get_compatible,
    HTMLDocument6_get_documentMode,
    HTMLDocument6_put_onstorage,
    HTMLDocument6_get_onstorage,
    HTMLDocument6_put_onstoragecommit,
    HTMLDocument6_get_onstoragecommit,
    HTMLDocument6_getElementById,
    HTMLDocument6_updateSettings
};

void HTMLDocument_HTMLDocument5_Init(HTMLDocument *This)
{
    This->IHTMLDocument5_iface.lpVtbl = &HTMLDocument5Vtbl;
    This->IHTMLDocument6_iface.lpVtbl = &HTMLDocument6Vtbl;
}
