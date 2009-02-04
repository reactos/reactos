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

    const IHTMLTableRowVtbl *lpHTMLTableRowVtbl;

    nsIDOMHTMLTableRowElement *nsrow;
} HTMLTableRow;

#define HTMLTABLEROW(x)  ((IHTMLTableRow*)  &(x)->lpHTMLTableRowVtbl)

#define HTMLTABLEROW_THIS(iface) DEFINE_THIS(HTMLTableRow, HTMLTableRow, iface)

static HRESULT WINAPI HTMLTableRow_QueryInterface(IHTMLTableRow *iface,
        REFIID riid, void **ppv)
{
    HTMLTableRow *This = HTMLTABLEROW_THIS(iface);

    return IHTMLDOMNode_QueryInterface(HTMLDOMNODE(&This->element.node), riid, ppv);
}

static ULONG WINAPI HTMLTableRow_AddRef(IHTMLTableRow *iface)
{
    HTMLTableRow *This = HTMLTABLEROW_THIS(iface);

    return IHTMLDOMNode_AddRef(HTMLDOMNODE(&This->element.node));
}

static ULONG WINAPI HTMLTableRow_Release(IHTMLTableRow *iface)
{
    HTMLTableRow *This = HTMLTABLEROW_THIS(iface);

    return IHTMLDOMNode_Release(HTMLDOMNODE(&This->element.node));
}

static HRESULT WINAPI HTMLTableRow_GetTypeInfoCount(IHTMLTableRow *iface, UINT *pctinfo)
{
    HTMLTableRow *This = HTMLTABLEROW_THIS(iface);
    return IDispatchEx_GetTypeInfoCount(DISPATCHEX(&This->element.node.dispex), pctinfo);
}

static HRESULT WINAPI HTMLTableRow_GetTypeInfo(IHTMLTableRow *iface, UINT iTInfo,
                                              LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLTableRow *This = HTMLTABLEROW_THIS(iface);
    return IDispatchEx_GetTypeInfo(DISPATCHEX(&This->element.node.dispex), iTInfo, lcid, ppTInfo);
}

static HRESULT WINAPI HTMLTableRow_GetIDsOfNames(IHTMLTableRow *iface, REFIID riid,
                                                LPOLESTR *rgszNames, UINT cNames,
                                                LCID lcid, DISPID *rgDispId)
{
    HTMLTableRow *This = HTMLTABLEROW_THIS(iface);
    return IDispatchEx_GetIDsOfNames(DISPATCHEX(&This->element.node.dispex), riid, rgszNames, cNames, lcid, rgDispId);
}

static HRESULT WINAPI HTMLTableRow_Invoke(IHTMLTableRow *iface, DISPID dispIdMember,
                            REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
                            VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLTableRow *This = HTMLTABLEROW_THIS(iface);
    return IDispatchEx_Invoke(DISPATCHEX(&This->element.node.dispex), dispIdMember, riid, lcid, wFlags, pDispParams,
            pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI HTMLTableRow_put_align(IHTMLTableRow *iface, BSTR v)
{
    HTMLTableRow *This = HTMLTABLEROW_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTableRow_get_align(IHTMLTableRow *iface, BSTR *p)
{
    HTMLTableRow *This = HTMLTABLEROW_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTableRow_put_vAlign(IHTMLTableRow *iface, BSTR v)
{
    HTMLTableRow *This = HTMLTABLEROW_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTableRow_get_vAlign(IHTMLTableRow *iface, BSTR *p)
{
    HTMLTableRow *This = HTMLTABLEROW_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTableRow_put_bgColor(IHTMLTableRow *iface, VARIANT v)
{
    HTMLTableRow *This = HTMLTABLEROW_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTableRow_get_bgColor(IHTMLTableRow *iface, VARIANT *p)
{
    HTMLTableRow *This = HTMLTABLEROW_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTableRow_put_borderColor(IHTMLTableRow *iface, VARIANT v)
{
    HTMLTableRow *This = HTMLTABLEROW_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTableRow_get_borderColor(IHTMLTableRow *iface, VARIANT *p)
{
    HTMLTableRow *This = HTMLTABLEROW_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTableRow_put_borderColorLight(IHTMLTableRow *iface, VARIANT v)
{
    HTMLTableRow *This = HTMLTABLEROW_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTableRow_get_borderColorLight(IHTMLTableRow *iface, VARIANT *p)
{
    HTMLTableRow *This = HTMLTABLEROW_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTableRow_put_borderColorDark(IHTMLTableRow *iface, VARIANT v)
{
    HTMLTableRow *This = HTMLTABLEROW_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTableRow_get_borderColorDark(IHTMLTableRow *iface, VARIANT *p)
{
    HTMLTableRow *This = HTMLTABLEROW_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTableRow_get_rowIndex(IHTMLTableRow *iface, long *p)
{
    HTMLTableRow *This = HTMLTABLEROW_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTableRow_get_selectionRowIndex(IHTMLTableRow *iface, long *p)
{
    HTMLTableRow *This = HTMLTABLEROW_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTableRow_get_cells(IHTMLTableRow *iface, IHTMLElementCollection **p)
{
    HTMLTableRow *This = HTMLTABLEROW_THIS(iface);
    nsIDOMHTMLCollection *nscol;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsres = nsIDOMHTMLTableRowElement_GetCells(This->nsrow, &nscol);
    if(NS_FAILED(nsres)) {
        ERR("GetCells failed: %08x\n", nsres);
        return E_FAIL;
    }

    *p = create_collection_from_htmlcol(This->element.node.doc, (IUnknown*)HTMLTABLEROW(This), nscol);

    nsIDOMHTMLCollection_Release(nscol);
    return S_OK;
}

static HRESULT WINAPI HTMLTableRow_insertCell(IHTMLTableRow *iface, long index, IDispatch **row)
{
    HTMLTableRow *This = HTMLTABLEROW_THIS(iface);
    FIXME("(%p)->(%ld %p)\n", This, index, row);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTableRow_deleteCell(IHTMLTableRow *iface, long index)
{
    HTMLTableRow *This = HTMLTABLEROW_THIS(iface);
    FIXME("(%p)->(%ld)\n", This, index);
    return E_NOTIMPL;
}

#undef HTMLTABLEROW_THIS

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
    HTMLTableRow_get_selectionRowIndex,
    HTMLTableRow_get_cells,
    HTMLTableRow_insertCell,
    HTMLTableRow_deleteCell
};

#define HTMLTABLEROW_NODE_THIS(iface) DEFINE_THIS2(HTMLTableRow, element.node, iface)

static HRESULT HTMLTableRow_QI(HTMLDOMNode *iface, REFIID riid, void **ppv)
{
    HTMLTableRow *This = HTMLTABLEROW_NODE_THIS(iface);

    *ppv = NULL;

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = HTMLTABLEROW(This);
    }else if(IsEqualGUID(&IID_IDispatch, riid)) {
        TRACE("(%p)->(IID_IDispatch %p)\n", This, ppv);
        *ppv = HTMLTABLEROW(This);
    }else if(IsEqualGUID(&IID_IHTMLTableRow, riid)) {
        TRACE("(%p)->(IID_IHTMLTableRow %p)\n", This, ppv);
        *ppv = HTMLTABLEROW(This);
    }

    if(*ppv) {
        IUnknown_AddRef((IUnknown*)*ppv);
        return S_OK;
    }

    return HTMLElement_QI(&This->element.node, riid, ppv);
}

static void HTMLTableRow_destructor(HTMLDOMNode *iface)
{
    HTMLTableRow *This = HTMLTABLEROW_NODE_THIS(iface);

    if(This->nsrow)
        nsIDOMHTMLTableRowElement_Release(This->nsrow);

    HTMLElement_destructor(&This->element.node);
}

#undef HTMLTABLEROW_NODE_THIS

static const NodeImplVtbl HTMLTableRowImplVtbl = {
    HTMLTableRow_QI,
    HTMLTableRow_destructor
};

static const tid_t HTMLTableRow_iface_tids[] = {
    IHTMLDOMNode_tid,
    IHTMLDOMNode2_tid,
    IHTMLElement_tid,
    IHTMLElement2_tid,
    IHTMLElement3_tid,
    IHTMLTableRow_tid,
    0
};

static dispex_static_data_t HTMLTableRow_dispex = {
    NULL,
    DispHTMLTableRow_tid,
    NULL,
    HTMLTableRow_iface_tids
};

HTMLElement *HTMLTableRow_Create(nsIDOMHTMLElement *nselem)
{
    HTMLTableRow *ret = heap_alloc_zero(sizeof(HTMLTableRow));
    nsresult nsres;

    ret->lpHTMLTableRowVtbl = &HTMLTableRowVtbl;
    ret->element.node.vtbl = &HTMLTableRowImplVtbl;

    init_dispex(&ret->element.node.dispex, (IUnknown*)HTMLTABLEROW(ret), &HTMLTableRow_dispex);
    HTMLElement_Init(&ret->element);

    nsres = nsIDOMHTMLElement_QueryInterface(nselem, &IID_nsIDOMHTMLTableRowElement, (void**)&ret->nsrow);
    if(NS_FAILED(nsres))
        ERR("Could not get nsIDOMHTMLTableRowElement iface: %08x\n", nsres);

    return &ret->element;
}
