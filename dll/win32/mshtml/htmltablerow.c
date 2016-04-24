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

    IHTMLTableRow IHTMLTableRow_iface;

    nsIDOMHTMLTableRowElement *nsrow;
} HTMLTableRow;

static inline HTMLTableRow *impl_from_IHTMLTableRow(IHTMLTableRow *iface)
{
    return CONTAINING_RECORD(iface, HTMLTableRow, IHTMLTableRow_iface);
}

static HRESULT WINAPI HTMLTableRow_QueryInterface(IHTMLTableRow *iface,
        REFIID riid, void **ppv)
{
    HTMLTableRow *This = impl_from_IHTMLTableRow(iface);

    return IHTMLDOMNode_QueryInterface(&This->element.node.IHTMLDOMNode_iface, riid, ppv);
}

static ULONG WINAPI HTMLTableRow_AddRef(IHTMLTableRow *iface)
{
    HTMLTableRow *This = impl_from_IHTMLTableRow(iface);

    return IHTMLDOMNode_AddRef(&This->element.node.IHTMLDOMNode_iface);
}

static ULONG WINAPI HTMLTableRow_Release(IHTMLTableRow *iface)
{
    HTMLTableRow *This = impl_from_IHTMLTableRow(iface);

    return IHTMLDOMNode_Release(&This->element.node.IHTMLDOMNode_iface);
}

static HRESULT WINAPI HTMLTableRow_GetTypeInfoCount(IHTMLTableRow *iface, UINT *pctinfo)
{
    HTMLTableRow *This = impl_from_IHTMLTableRow(iface);
    return IDispatchEx_GetTypeInfoCount(&This->element.node.event_target.dispex.IDispatchEx_iface, pctinfo);
}

static HRESULT WINAPI HTMLTableRow_GetTypeInfo(IHTMLTableRow *iface, UINT iTInfo,
                                              LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLTableRow *This = impl_from_IHTMLTableRow(iface);
    return IDispatchEx_GetTypeInfo(&This->element.node.event_target.dispex.IDispatchEx_iface, iTInfo, lcid,
            ppTInfo);
}

static HRESULT WINAPI HTMLTableRow_GetIDsOfNames(IHTMLTableRow *iface, REFIID riid,
                                                LPOLESTR *rgszNames, UINT cNames,
                                                LCID lcid, DISPID *rgDispId)
{
    HTMLTableRow *This = impl_from_IHTMLTableRow(iface);
    return IDispatchEx_GetIDsOfNames(&This->element.node.event_target.dispex.IDispatchEx_iface, riid, rgszNames,
            cNames, lcid, rgDispId);
}

static HRESULT WINAPI HTMLTableRow_Invoke(IHTMLTableRow *iface, DISPID dispIdMember,
                            REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
                            VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLTableRow *This = impl_from_IHTMLTableRow(iface);
    return IDispatchEx_Invoke(&This->element.node.event_target.dispex.IDispatchEx_iface, dispIdMember, riid,
            lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI HTMLTableRow_put_align(IHTMLTableRow *iface, BSTR v)
{
    HTMLTableRow *This = impl_from_IHTMLTableRow(iface);
    nsAString val;
    nsresult nsres;

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    nsAString_InitDepend(&val, v);

    nsres = nsIDOMHTMLTableRowElement_SetAlign(This->nsrow, &val);
    nsAString_Finish(&val);
    if (NS_FAILED(nsres)){
        ERR("Set Align(%s) failed!\n", debugstr_w(v));
        return E_FAIL;
    }
    return S_OK;
}

static HRESULT WINAPI HTMLTableRow_get_align(IHTMLTableRow *iface, BSTR *p)
{
    HTMLTableRow *This = impl_from_IHTMLTableRow(iface);
    nsAString val;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsAString_Init(&val, NULL);
    nsres = nsIDOMHTMLTableRowElement_GetAlign(This->nsrow, &val);

    return return_nsstr(nsres, &val, p);
}

static HRESULT WINAPI HTMLTableRow_put_vAlign(IHTMLTableRow *iface, BSTR v)
{
    HTMLTableRow *This = impl_from_IHTMLTableRow(iface);
    nsAString val;
    nsresult nsres;

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    nsAString_InitDepend(&val, v);

    nsres = nsIDOMHTMLTableRowElement_SetVAlign(This->nsrow, &val);
    nsAString_Finish(&val);

    if (NS_FAILED(nsres)){
        ERR("Set VAlign(%s) failed!\n", debugstr_w(v));
        return E_FAIL;
    }

    return S_OK;
}

static HRESULT WINAPI HTMLTableRow_get_vAlign(IHTMLTableRow *iface, BSTR *p)
{
    HTMLTableRow *This = impl_from_IHTMLTableRow(iface);
    nsAString val;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsAString_Init(&val, NULL);
    nsres = nsIDOMHTMLTableRowElement_GetVAlign(This->nsrow, &val);

    return return_nsstr(nsres, &val, p);
}

static HRESULT WINAPI HTMLTableRow_put_bgColor(IHTMLTableRow *iface, VARIANT v)
{
    HTMLTableRow *This = impl_from_IHTMLTableRow(iface);
    nsAString val;
    nsresult nsres;

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    if (!variant_to_nscolor(&v, &val))
        return S_OK;

    nsres = nsIDOMHTMLTableRowElement_SetBgColor(This->nsrow, &val);
    nsAString_Finish(&val);

    if (NS_FAILED(nsres)){
        ERR("Set BgColor(%s) failed!\n", debugstr_variant(&v));
        return E_FAIL;
    }

    return S_OK;
}

static HRESULT WINAPI HTMLTableRow_get_bgColor(IHTMLTableRow *iface, VARIANT *p)
{
    HTMLTableRow *This = impl_from_IHTMLTableRow(iface);
    nsAString strColor;
    nsresult nsres;
    HRESULT hres;
    const PRUnichar *color;

    TRACE("(%p)->(%p)\n", This, p);

    nsAString_Init(&strColor, NULL);
    nsres = nsIDOMHTMLTableRowElement_GetBgColor(This->nsrow, &strColor);

    if(NS_SUCCEEDED(nsres)) {
       nsAString_GetData(&strColor, &color);
       V_VT(p) = VT_BSTR;
       hres = nscolor_to_str(color, &V_BSTR(p));
    }else {
       ERR("SetBgColor failed: %08x\n", nsres);
       hres = E_FAIL;
    }

    nsAString_Finish(&strColor);
    return hres;
}

static HRESULT WINAPI HTMLTableRow_put_borderColor(IHTMLTableRow *iface, VARIANT v)
{
    HTMLTableRow *This = impl_from_IHTMLTableRow(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTableRow_get_borderColor(IHTMLTableRow *iface, VARIANT *p)
{
    HTMLTableRow *This = impl_from_IHTMLTableRow(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTableRow_put_borderColorLight(IHTMLTableRow *iface, VARIANT v)
{
    HTMLTableRow *This = impl_from_IHTMLTableRow(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTableRow_get_borderColorLight(IHTMLTableRow *iface, VARIANT *p)
{
    HTMLTableRow *This = impl_from_IHTMLTableRow(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTableRow_put_borderColorDark(IHTMLTableRow *iface, VARIANT v)
{
    HTMLTableRow *This = impl_from_IHTMLTableRow(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTableRow_get_borderColorDark(IHTMLTableRow *iface, VARIANT *p)
{
    HTMLTableRow *This = impl_from_IHTMLTableRow(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTableRow_get_rowIndex(IHTMLTableRow *iface, LONG *p)
{
    HTMLTableRow *This = impl_from_IHTMLTableRow(iface);
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);
    nsres = nsIDOMHTMLTableRowElement_GetRowIndex(This->nsrow, p);
    if(NS_FAILED(nsres)) {
        ERR("Get rowIndex failed: %08x\n", nsres);
        return E_FAIL;
    }
    return S_OK;
}

static HRESULT WINAPI HTMLTableRow_get_sectionRowIndex(IHTMLTableRow *iface, LONG *p)
{
    HTMLTableRow *This = impl_from_IHTMLTableRow(iface);
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);
    nsres = nsIDOMHTMLTableRowElement_GetSectionRowIndex(This->nsrow, p);
    if(NS_FAILED(nsres)) {
        ERR("Get selectionRowIndex failed: %08x\n", nsres);
        return E_FAIL;
    }
    return S_OK;
}

static HRESULT WINAPI HTMLTableRow_get_cells(IHTMLTableRow *iface, IHTMLElementCollection **p)
{
    HTMLTableRow *This = impl_from_IHTMLTableRow(iface);
    nsIDOMHTMLCollection *nscol;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsres = nsIDOMHTMLTableRowElement_GetCells(This->nsrow, &nscol);
    if(NS_FAILED(nsres)) {
        ERR("GetCells failed: %08x\n", nsres);
        return E_FAIL;
    }

    *p = create_collection_from_htmlcol(This->element.node.doc, nscol);

    nsIDOMHTMLCollection_Release(nscol);
    return S_OK;
}

static HRESULT WINAPI HTMLTableRow_insertCell(IHTMLTableRow *iface, LONG index, IDispatch **row)
{
    HTMLTableRow *This = impl_from_IHTMLTableRow(iface);
    nsIDOMHTMLElement *nselem;
    HTMLElement *elem;
    nsresult nsres;
    HRESULT hres;

    TRACE("(%p)->(%d %p)\n", This, index, row);
    nsres = nsIDOMHTMLTableRowElement_InsertCell(This->nsrow, index, &nselem);
    if(NS_FAILED(nsres)) {
        ERR("Insert Cell at %d failed: %08x\n", index, nsres);
        return E_FAIL;
    }

    hres = HTMLTableCell_Create(This->element.node.doc, nselem, &elem);
    nsIDOMHTMLElement_Release(nselem);
    if (FAILED(hres)) {
        ERR("Create TableCell failed: %08x\n", hres);
        return hres;
    }

    *row = (IDispatch *)&elem->IHTMLElement_iface;
    return S_OK;
}

static HRESULT WINAPI HTMLTableRow_deleteCell(IHTMLTableRow *iface, LONG index)
{
    HTMLTableRow *This = impl_from_IHTMLTableRow(iface);
    nsresult nsres;

    TRACE("(%p)->(%d)\n", This, index);
    nsres = nsIDOMHTMLTableRowElement_DeleteCell(This->nsrow, index);
    if(NS_FAILED(nsres)) {
        ERR("Delete Cell failed: %08x\n", nsres);
        return E_FAIL;
    }
    return S_OK;
}

static const IHTMLTableRowVtbl HTMLTableRowVtbl = {
    HTMLTableRow_QueryInterface,
    HTMLTableRow_AddRef,
    HTMLTableRow_Release,
    HTMLTableRow_GetTypeInfoCount,
    HTMLTableRow_GetTypeInfo,
    HTMLTableRow_GetIDsOfNames,
    HTMLTableRow_Invoke,
    HTMLTableRow_put_align,
    HTMLTableRow_get_align,
    HTMLTableRow_put_vAlign,
    HTMLTableRow_get_vAlign,
    HTMLTableRow_put_bgColor,
    HTMLTableRow_get_bgColor,
    HTMLTableRow_put_borderColor,
    HTMLTableRow_get_borderColor,
    HTMLTableRow_put_borderColorLight,
    HTMLTableRow_get_borderColorLight,
    HTMLTableRow_put_borderColorDark,
    HTMLTableRow_get_borderColorDark,
    HTMLTableRow_get_rowIndex,
    HTMLTableRow_get_sectionRowIndex,
    HTMLTableRow_get_cells,
    HTMLTableRow_insertCell,
    HTMLTableRow_deleteCell
};

static inline HTMLTableRow *impl_from_HTMLDOMNode(HTMLDOMNode *iface)
{
    return CONTAINING_RECORD(iface, HTMLTableRow, element.node);
}

static HRESULT HTMLTableRow_QI(HTMLDOMNode *iface, REFIID riid, void **ppv)
{
    HTMLTableRow *This = impl_from_HTMLDOMNode(iface);

    *ppv = NULL;

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = &This->IHTMLTableRow_iface;
    }else if(IsEqualGUID(&IID_IDispatch, riid)) {
        TRACE("(%p)->(IID_IDispatch %p)\n", This, ppv);
        *ppv = &This->IHTMLTableRow_iface;
    }else if(IsEqualGUID(&IID_IHTMLTableRow, riid)) {
        TRACE("(%p)->(IID_IHTMLTableRow %p)\n", This, ppv);
        *ppv = &This->IHTMLTableRow_iface;
    }

    if(*ppv) {
        IUnknown_AddRef((IUnknown*)*ppv);
        return S_OK;
    }

    return HTMLElement_QI(&This->element.node, riid, ppv);
}

static void HTMLTableRow_traverse(HTMLDOMNode *iface, nsCycleCollectionTraversalCallback *cb)
{
    HTMLTableRow *This = impl_from_HTMLDOMNode(iface);

    if(This->nsrow)
        note_cc_edge((nsISupports*)This->nsrow, "This->nstablerow", cb);
}

static void HTMLTableRow_unlink(HTMLDOMNode *iface)
{
    HTMLTableRow *This = impl_from_HTMLDOMNode(iface);

    if(This->nsrow) {
        nsIDOMHTMLTableRowElement *nsrow = This->nsrow;

        This->nsrow = NULL;
        nsIDOMHTMLTableRowElement_Release(nsrow);
    }
}

static const NodeImplVtbl HTMLTableRowImplVtbl = {
    HTMLTableRow_QI,
    HTMLElement_destructor,
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
    HTMLTableRow_traverse,
    HTMLTableRow_unlink
};

static const tid_t HTMLTableRow_iface_tids[] = {
    HTMLELEMENT_TIDS,
    IHTMLTableRow_tid,
    0
};

static dispex_static_data_t HTMLTableRow_dispex = {
    NULL,
    DispHTMLTableRow_tid,
    NULL,
    HTMLTableRow_iface_tids
};

HRESULT HTMLTableRow_Create(HTMLDocumentNode *doc, nsIDOMHTMLElement *nselem, HTMLElement **elem)
{
    HTMLTableRow *ret;
    nsresult nsres;

    ret = heap_alloc_zero(sizeof(HTMLTableRow));
    if(!ret)
        return E_OUTOFMEMORY;

    ret->IHTMLTableRow_iface.lpVtbl = &HTMLTableRowVtbl;
    ret->element.node.vtbl = &HTMLTableRowImplVtbl;

    HTMLElement_Init(&ret->element, doc, nselem, &HTMLTableRow_dispex);

    nsres = nsIDOMHTMLElement_QueryInterface(nselem, &IID_nsIDOMHTMLTableRowElement, (void**)&ret->nsrow);
    assert(nsres == NS_OK);

    *elem = &ret->element;
    return S_OK;
}
