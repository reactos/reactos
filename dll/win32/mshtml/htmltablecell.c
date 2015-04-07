/*
 * Copyright 2012 Jacek Caban for CodeWeavers
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

    IHTMLTableCell IHTMLTableCell_iface;

    nsIDOMHTMLTableCellElement *nscell;
} HTMLTableCell;

static inline HTMLTableCell *impl_from_IHTMLTableCell(IHTMLTableCell *iface)
{
    return CONTAINING_RECORD(iface, HTMLTableCell, IHTMLTableCell_iface);
}

static HRESULT WINAPI HTMLTableCell_QueryInterface(IHTMLTableCell *iface, REFIID riid, void **ppv)
{
    HTMLTableCell *This = impl_from_IHTMLTableCell(iface);

    return IHTMLDOMNode_QueryInterface(&This->element.node.IHTMLDOMNode_iface, riid, ppv);
}

static ULONG WINAPI HTMLTableCell_AddRef(IHTMLTableCell *iface)
{
    HTMLTableCell *This = impl_from_IHTMLTableCell(iface);

    return IHTMLDOMNode_AddRef(&This->element.node.IHTMLDOMNode_iface);
}

static ULONG WINAPI HTMLTableCell_Release(IHTMLTableCell *iface)
{
    HTMLTableCell *This = impl_from_IHTMLTableCell(iface);

    return IHTMLDOMNode_Release(&This->element.node.IHTMLDOMNode_iface);
}

static HRESULT WINAPI HTMLTableCell_GetTypeInfoCount(IHTMLTableCell *iface, UINT *pctinfo)
{
    HTMLTableCell *This = impl_from_IHTMLTableCell(iface);
    return IDispatchEx_GetTypeInfoCount(&This->element.node.dispex.IDispatchEx_iface, pctinfo);
}

static HRESULT WINAPI HTMLTableCell_GetTypeInfo(IHTMLTableCell *iface, UINT iTInfo,
                                              LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLTableCell *This = impl_from_IHTMLTableCell(iface);
    return IDispatchEx_GetTypeInfo(&This->element.node.dispex.IDispatchEx_iface, iTInfo, lcid,
            ppTInfo);
}

static HRESULT WINAPI HTMLTableCell_GetIDsOfNames(IHTMLTableCell *iface, REFIID riid,
        LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{
    HTMLTableCell *This = impl_from_IHTMLTableCell(iface);
    return IDispatchEx_GetIDsOfNames(&This->element.node.dispex.IDispatchEx_iface, riid, rgszNames,
            cNames, lcid, rgDispId);
}

static HRESULT WINAPI HTMLTableCell_Invoke(IHTMLTableCell *iface, DISPID dispIdMember, REFIID riid,
        LCID lcid, WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult, EXCEPINFO *pExcepInfo,
        UINT *puArgErr)
{
    HTMLTableCell *This = impl_from_IHTMLTableCell(iface);
    return IDispatchEx_Invoke(&This->element.node.dispex.IDispatchEx_iface, dispIdMember, riid,
            lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI HTMLTableCell_put_rowSpan(IHTMLTableCell *iface, LONG v)
{
    HTMLTableCell *This = impl_from_IHTMLTableCell(iface);
    FIXME("(%p)->(%d)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTableCell_get_rowSpan(IHTMLTableCell *iface, LONG *p)
{
    HTMLTableCell *This = impl_from_IHTMLTableCell(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTableCell_put_colSpan(IHTMLTableCell *iface, LONG v)
{
    HTMLTableCell *This = impl_from_IHTMLTableCell(iface);
    FIXME("(%p)->(%d)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTableCell_get_colSpan(IHTMLTableCell *iface, LONG *p)
{
    HTMLTableCell *This = impl_from_IHTMLTableCell(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTableCell_put_align(IHTMLTableCell *iface, BSTR v)
{
    HTMLTableCell *This = impl_from_IHTMLTableCell(iface);
    nsAString str;
    nsresult nsres;

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    nsAString_InitDepend(&str, v);
    nsres = nsIDOMHTMLTableCellElement_SetAlign(This->nscell, &str);
    nsAString_Finish(&str);
    if (NS_FAILED(nsres)) {
        ERR("Set Align failed: %08x\n", nsres);
        return E_FAIL;
    }
    return S_OK;
}

static HRESULT WINAPI HTMLTableCell_get_align(IHTMLTableCell *iface, BSTR *p)
{
    HTMLTableCell *This = impl_from_IHTMLTableCell(iface);
    nsAString str;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsAString_Init(&str, NULL);
    nsres = nsIDOMHTMLTableCellElement_GetAlign(This->nscell, &str);

    return return_nsstr(nsres, &str, p);
}

static HRESULT WINAPI HTMLTableCell_put_vAlign(IHTMLTableCell *iface, BSTR v)
{
    HTMLTableCell *This = impl_from_IHTMLTableCell(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTableCell_get_vAlign(IHTMLTableCell *iface, BSTR *p)
{
    HTMLTableCell *This = impl_from_IHTMLTableCell(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTableCell_put_bgColor(IHTMLTableCell *iface, VARIANT v)
{
    HTMLTableCell *This = impl_from_IHTMLTableCell(iface);
    nsAString strColor;
    nsresult nsres;

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    if(!variant_to_nscolor(&v, &strColor))
        return S_OK;

    nsres = nsIDOMHTMLTableCellElement_SetBgColor(This->nscell, &strColor);
    nsAString_Finish(&strColor);
    if(NS_FAILED(nsres)) {
        ERR("SetBgColor(%s) failed: %08x\n", debugstr_variant(&v), nsres);
        return E_FAIL;
    }

    return S_OK;
}

static HRESULT WINAPI HTMLTableCell_get_bgColor(IHTMLTableCell *iface, VARIANT *p)
{
    HTMLTableCell *This = impl_from_IHTMLTableCell(iface);
    nsAString strColor;
    nsresult nsres;
    HRESULT hres;

    TRACE("(%p)->(%p)\n", This, p);

    nsAString_Init(&strColor, NULL);
    nsres = nsIDOMHTMLTableCellElement_GetBgColor(This->nscell, &strColor);

    if(NS_SUCCEEDED(nsres)) {
        const PRUnichar *color;
        nsAString_GetData(&strColor, &color);
        V_VT(p) = VT_BSTR;
        hres = nscolor_to_str(color, &V_BSTR(p));
    }else {
        ERR("GetBgColor failed: %08x\n", nsres);
        hres = E_FAIL;
    }
    nsAString_Finish(&strColor);
    return hres;
}

static HRESULT WINAPI HTMLTableCell_put_noWrap(IHTMLTableCell *iface, VARIANT_BOOL v)
{
    HTMLTableCell *This = impl_from_IHTMLTableCell(iface);
    FIXME("(%p)->(%x)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTableCell_get_noWrap(IHTMLTableCell *iface, VARIANT_BOOL *p)
{
    HTMLTableCell *This = impl_from_IHTMLTableCell(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTableCell_put_background(IHTMLTableCell *iface, BSTR v)
{
    HTMLTableCell *This = impl_from_IHTMLTableCell(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTableCell_get_background(IHTMLTableCell *iface, BSTR *p)
{
    HTMLTableCell *This = impl_from_IHTMLTableCell(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTableCell_put_borderColor(IHTMLTableCell *iface, VARIANT v)
{
    HTMLTableCell *This = impl_from_IHTMLTableCell(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTableCell_get_borderColor(IHTMLTableCell *iface, VARIANT *p)
{
    HTMLTableCell *This = impl_from_IHTMLTableCell(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTableCell_put_borderColorLight(IHTMLTableCell *iface, VARIANT v)
{
    HTMLTableCell *This = impl_from_IHTMLTableCell(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTableCell_get_borderColorLight(IHTMLTableCell *iface, VARIANT *p)
{
    HTMLTableCell *This = impl_from_IHTMLTableCell(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTableCell_put_borderColorDark(IHTMLTableCell *iface, VARIANT v)
{
    HTMLTableCell *This = impl_from_IHTMLTableCell(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTableCell_get_borderColorDark(IHTMLTableCell *iface, VARIANT *p)
{
    HTMLTableCell *This = impl_from_IHTMLTableCell(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTableCell_put_width(IHTMLTableCell *iface, VARIANT v)
{
    HTMLTableCell *This = impl_from_IHTMLTableCell(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTableCell_get_width(IHTMLTableCell *iface, VARIANT *p)
{
    HTMLTableCell *This = impl_from_IHTMLTableCell(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTableCell_put_height(IHTMLTableCell *iface, VARIANT v)
{
    HTMLTableCell *This = impl_from_IHTMLTableCell(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTableCell_get_height(IHTMLTableCell *iface, VARIANT *p)
{
    HTMLTableCell *This = impl_from_IHTMLTableCell(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTableCell_get_cellIndex(IHTMLTableCell *iface, LONG *p)
{
    HTMLTableCell *This = impl_from_IHTMLTableCell(iface);
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);
    nsres = nsIDOMHTMLTableCellElement_GetCellIndex(This->nscell, p);
    if (NS_FAILED(nsres)) {
        ERR("Get CellIndex failed: %08x\n", nsres);
        return E_FAIL;
    }

    return S_OK;
}

static const IHTMLTableCellVtbl HTMLTableCellVtbl = {
    HTMLTableCell_QueryInterface,
    HTMLTableCell_AddRef,
    HTMLTableCell_Release,
    HTMLTableCell_GetTypeInfoCount,
    HTMLTableCell_GetTypeInfo,
    HTMLTableCell_GetIDsOfNames,
    HTMLTableCell_Invoke,
    HTMLTableCell_put_rowSpan,
    HTMLTableCell_get_rowSpan,
    HTMLTableCell_put_colSpan,
    HTMLTableCell_get_colSpan,
    HTMLTableCell_put_align,
    HTMLTableCell_get_align,
    HTMLTableCell_put_vAlign,
    HTMLTableCell_get_vAlign,
    HTMLTableCell_put_bgColor,
    HTMLTableCell_get_bgColor,
    HTMLTableCell_put_noWrap,
    HTMLTableCell_get_noWrap,
    HTMLTableCell_put_background,
    HTMLTableCell_get_background,
    HTMLTableCell_put_borderColor,
    HTMLTableCell_get_borderColor,
    HTMLTableCell_put_borderColorLight,
    HTMLTableCell_get_borderColorLight,
    HTMLTableCell_put_borderColorDark,
    HTMLTableCell_get_borderColorDark,
    HTMLTableCell_put_width,
    HTMLTableCell_get_width,
    HTMLTableCell_put_height,
    HTMLTableCell_get_height,
    HTMLTableCell_get_cellIndex
};

static inline HTMLTableCell *impl_from_HTMLDOMNode(HTMLDOMNode *iface)
{
    return CONTAINING_RECORD(iface, HTMLTableCell, element.node);
}

static HRESULT HTMLTableCell_QI(HTMLDOMNode *iface, REFIID riid, void **ppv)
{
    HTMLTableCell *This = impl_from_HTMLDOMNode(iface);

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = &This->IHTMLTableCell_iface;
    }else if(IsEqualGUID(&IID_IDispatch, riid)) {
        TRACE("(%p)->(IID_IDispatch %p)\n", This, ppv);
        *ppv = &This->IHTMLTableCell_iface;
    }else if(IsEqualGUID(&IID_IHTMLTableCell, riid)) {
        TRACE("(%p)->(IID_IHTMLTableCell %p)\n", This, ppv);
        *ppv = &This->IHTMLTableCell_iface;
    }else {
        return HTMLElement_QI(&This->element.node, riid, ppv);
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static void HTMLTableCell_destructor(HTMLDOMNode *iface)
{
    HTMLTableCell *This = impl_from_HTMLDOMNode(iface);

    HTMLElement_destructor(&This->element.node);
}

static void HTMLTableCell_traverse(HTMLDOMNode *iface, nsCycleCollectionTraversalCallback *cb)
{
    HTMLTableCell *This = impl_from_HTMLDOMNode(iface);

    if(This->nscell)
        note_cc_edge((nsISupports*)This->nscell, "This->nstablecell", cb);
}

static void HTMLTableCell_unlink(HTMLDOMNode *iface)
{
    HTMLTableCell *This = impl_from_HTMLDOMNode(iface);

    if(This->nscell) {
        nsIDOMHTMLTableCellElement *nscell = This->nscell;

        This->nscell = NULL;
        nsIDOMHTMLTableCellElement_Release(nscell);
    }
}

static const NodeImplVtbl HTMLTableCellImplVtbl = {
    HTMLTableCell_QI,
    HTMLTableCell_destructor,
    HTMLElement_cpc,
    HTMLElement_clone,
    HTMLElement_handle_event,
    HTMLElement_get_attr_col,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    HTMLTableCell_traverse,
    HTMLTableCell_unlink
};

static const tid_t HTMLTableCell_iface_tids[] = {
    HTMLELEMENT_TIDS,
    IHTMLTableCell_tid,
    0
};

static dispex_static_data_t HTMLTableCell_dispex = {
    NULL,
    DispHTMLTableCell_tid,
    NULL,
    HTMLTableCell_iface_tids
};

HRESULT HTMLTableCell_Create(HTMLDocumentNode *doc, nsIDOMHTMLElement *nselem, HTMLElement **elem)
{
    HTMLTableCell *ret;
    nsresult nsres;

    ret = heap_alloc_zero(sizeof(*ret));
    if(!ret)
        return E_OUTOFMEMORY;

    ret->IHTMLTableCell_iface.lpVtbl = &HTMLTableCellVtbl;
    ret->element.node.vtbl = &HTMLTableCellImplVtbl;

    HTMLElement_Init(&ret->element, doc, nselem, &HTMLTableCell_dispex);

    nsres = nsIDOMHTMLElement_QueryInterface(nselem, &IID_nsIDOMHTMLTableCellElement, (void**)&ret->nscell);
    assert(nsres == NS_OK);

    *elem = &ret->element;
    return S_OK;
}
