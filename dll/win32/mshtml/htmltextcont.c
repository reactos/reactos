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

static inline HTMLTextContainer *impl_from_IHTMLTextContainer(IHTMLTextContainer *iface)
{
    return CONTAINING_RECORD(iface, HTMLTextContainer, IHTMLTextContainer_iface);
}

static HRESULT WINAPI HTMLTextContainer_QueryInterface(IHTMLTextContainer *iface,
                                                       REFIID riid, void **ppv)
{
    HTMLTextContainer *This = impl_from_IHTMLTextContainer(iface);
    return IHTMLElement_QueryInterface(&This->element.IHTMLElement_iface, riid, ppv);
}

static ULONG WINAPI HTMLTextContainer_AddRef(IHTMLTextContainer *iface)
{
    HTMLTextContainer *This = impl_from_IHTMLTextContainer(iface);
    return IHTMLElement_AddRef(&This->element.IHTMLElement_iface);
}

static ULONG WINAPI HTMLTextContainer_Release(IHTMLTextContainer *iface)
{
    HTMLTextContainer *This = impl_from_IHTMLTextContainer(iface);
    return IHTMLElement_Release(&This->element.IHTMLElement_iface);
}

static HRESULT WINAPI HTMLTextContainer_GetTypeInfoCount(IHTMLTextContainer *iface, UINT *pctinfo)
{
    HTMLTextContainer *This = impl_from_IHTMLTextContainer(iface);
    return IDispatchEx_GetTypeInfoCount(&This->element.node.dispex.IDispatchEx_iface, pctinfo);
}

static HRESULT WINAPI HTMLTextContainer_GetTypeInfo(IHTMLTextContainer *iface, UINT iTInfo,
                                              LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLTextContainer *This = impl_from_IHTMLTextContainer(iface);
    return IDispatchEx_GetTypeInfo(&This->element.node.dispex.IDispatchEx_iface, iTInfo, lcid,
            ppTInfo);
}

static HRESULT WINAPI HTMLTextContainer_GetIDsOfNames(IHTMLTextContainer *iface, REFIID riid,
                                                LPOLESTR *rgszNames, UINT cNames,
                                                LCID lcid, DISPID *rgDispId)
{
    HTMLTextContainer *This = impl_from_IHTMLTextContainer(iface);
    return IDispatchEx_GetIDsOfNames(&This->element.node.dispex.IDispatchEx_iface, riid, rgszNames,
            cNames, lcid, rgDispId);
}

static HRESULT WINAPI HTMLTextContainer_Invoke(IHTMLTextContainer *iface, DISPID dispIdMember,
                            REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
                            VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLTextContainer *This = impl_from_IHTMLTextContainer(iface);
    return IDispatchEx_Invoke(&This->element.node.dispex.IDispatchEx_iface, dispIdMember, riid,
            lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI HTMLTextContainer_createControlRange(IHTMLTextContainer *iface,
                                                           IDispatch **range)
{
    HTMLTextContainer *This = impl_from_IHTMLTextContainer(iface);
    FIXME("(%p)->(%p)\n", This, range);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTextContainer_get_scrollHeight(IHTMLTextContainer *iface, LONG *p)
{
    HTMLTextContainer *This = impl_from_IHTMLTextContainer(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return IHTMLElement2_get_scrollHeight(&This->element.IHTMLElement2_iface, p);
}

static HRESULT WINAPI HTMLTextContainer_get_scrollWidth(IHTMLTextContainer *iface, LONG *p)
{
    HTMLTextContainer *This = impl_from_IHTMLTextContainer(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return IHTMLElement2_get_scrollWidth(&This->element.IHTMLElement2_iface, p);
}

static HRESULT WINAPI HTMLTextContainer_put_scrollTop(IHTMLTextContainer *iface, LONG v)
{
    HTMLTextContainer *This = impl_from_IHTMLTextContainer(iface);

    TRACE("(%p)->(%d)\n", This, v);

    return IHTMLElement2_put_scrollTop(&This->element.IHTMLElement2_iface, v);
}

static HRESULT WINAPI HTMLTextContainer_get_scrollTop(IHTMLTextContainer *iface, LONG *p)
{
    HTMLTextContainer *This = impl_from_IHTMLTextContainer(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return IHTMLElement2_get_scrollTop(&This->element.IHTMLElement2_iface, p);
}

static HRESULT WINAPI HTMLTextContainer_put_scrollLeft(IHTMLTextContainer *iface, LONG v)
{
    HTMLTextContainer *This = impl_from_IHTMLTextContainer(iface);

    TRACE("(%p)->(%d)\n", This, v);

    return IHTMLElement2_put_scrollLeft(&This->element.IHTMLElement2_iface, v);
}

static HRESULT WINAPI HTMLTextContainer_get_scrollLeft(IHTMLTextContainer *iface, LONG *p)
{
    HTMLTextContainer *This = impl_from_IHTMLTextContainer(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return IHTMLElement2_get_scrollLeft(&This->element.IHTMLElement2_iface, p);
}

static HRESULT WINAPI HTMLTextContainer_put_onscroll(IHTMLTextContainer *iface, VARIANT v)
{
    HTMLTextContainer *This = impl_from_IHTMLTextContainer(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTextContainer_get_onscroll(IHTMLTextContainer *iface, VARIANT *p)
{
    HTMLTextContainer *This = impl_from_IHTMLTextContainer(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static const IHTMLTextContainerVtbl HTMLTextContainerVtbl = {
    HTMLTextContainer_QueryInterface,
    HTMLTextContainer_AddRef,
    HTMLTextContainer_Release,
    HTMLTextContainer_GetTypeInfoCount,
    HTMLTextContainer_GetTypeInfo,
    HTMLTextContainer_GetIDsOfNames,
    HTMLTextContainer_Invoke,
    HTMLTextContainer_createControlRange,
    HTMLTextContainer_get_scrollHeight,
    HTMLTextContainer_get_scrollWidth,
    HTMLTextContainer_put_scrollTop,
    HTMLTextContainer_get_scrollTop,
    HTMLTextContainer_put_scrollLeft,
    HTMLTextContainer_get_scrollLeft,
    HTMLTextContainer_put_onscroll,
    HTMLTextContainer_get_onscroll
};

void HTMLTextContainer_Init(HTMLTextContainer *This, HTMLDocumentNode *doc, nsIDOMHTMLElement *nselem,
        dispex_static_data_t *dispex_data)
{
    This->IHTMLTextContainer_iface.lpVtbl = &HTMLTextContainerVtbl;

    HTMLElement_Init(&This->element, doc, nselem, dispex_data);
}
