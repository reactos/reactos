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

    const IHTMLTableVtbl  *lpHTMLTableVtbl;

    ConnectionPoint cp;
} HTMLTable;

#define HTMLTABLE(x)  ((IHTMLTable*)  &(x)->lpHTMLTableVtbl)

#define HTMLTABLE_THIS(iface) DEFINE_THIS(HTMLTable, HTMLTable, iface)

static HRESULT WINAPI HTMLTable_QueryInterface(IHTMLTable *iface,
                                                         REFIID riid, void **ppv)
{
    HTMLTable *This = HTMLTABLE_THIS(iface);

    return IHTMLDOMNode_QueryInterface(HTMLDOMNODE(&This->element.node), riid, ppv);
}

static ULONG WINAPI HTMLTable_AddRef(IHTMLTable *iface)
{
    HTMLTable *This = HTMLTABLE_THIS(iface);

    return IHTMLDOMNode_AddRef(HTMLDOMNODE(&This->element.node));
}

static ULONG WINAPI HTMLTable_Release(IHTMLTable *iface)
{
    HTMLTable *This = HTMLTABLE_THIS(iface);

    return IHTMLDOMNode_Release(HTMLDOMNODE(&This->element.node));
}

static HRESULT WINAPI HTMLTable_GetTypeInfoCount(IHTMLTable *iface, UINT *pctinfo)
{
    HTMLTable *This = HTMLTABLE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pctinfo);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTable_GetTypeInfo(IHTMLTable *iface, UINT iTInfo,
                                              LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLTable *This = HTMLTABLE_THIS(iface);
    FIXME("(%p)->(%u %u %p)\n", This, iTInfo, lcid, ppTInfo);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTable_GetIDsOfNames(IHTMLTable *iface, REFIID riid,
                                                LPOLESTR *rgszNames, UINT cNames,
                                                LCID lcid, DISPID *rgDispId)
{
    HTMLTable *This = HTMLTABLE_THIS(iface);
    FIXME("(%p)->(%s %p %u %u %p)\n", This, debugstr_guid(riid), rgszNames, cNames,
                                        lcid, rgDispId);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTable_Invoke(IHTMLTable *iface, DISPID dispIdMember,
                            REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
                            VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLTable *This = HTMLTABLE_THIS(iface);
    FIXME("(%p)->(%d %s %d %d %p %p %p %p)\n", This, dispIdMember, debugstr_guid(riid),
            lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTable_put_cols(IHTMLTable *iface, long v)
{
    HTMLTable *This = HTMLTABLE_THIS(iface);
    FIXME("(%p)->(%ld)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTable_get_cols(IHTMLTable *iface, long *p)
{
    HTMLTable *This = HTMLTABLE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTable_put_border(IHTMLTable *iface, VARIANT v)
{
    HTMLTable *This = HTMLTABLE_THIS(iface);
    FIXME("(%p)->(v)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTable_get_border(IHTMLTable *iface, VARIANT *p)
{
    HTMLTable *This = HTMLTABLE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTable_put_frame(IHTMLTable *iface, BSTR v)
{
    HTMLTable *This = HTMLTABLE_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTable_get_frame(IHTMLTable *iface, BSTR *p)
{
    HTMLTable *This = HTMLTABLE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTable_put_rules(IHTMLTable *iface, BSTR v)
{
    HTMLTable *This = HTMLTABLE_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTable_get_rules(IHTMLTable *iface, BSTR *p)
{
    HTMLTable *This = HTMLTABLE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTable_put_cellSpacing(IHTMLTable *iface, VARIANT v)
{
    HTMLTable *This = HTMLTABLE_THIS(iface);
    FIXME("(%p)->(v)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTable_get_cellSpacing(IHTMLTable *iface, VARIANT *p)
{
    HTMLTable *This = HTMLTABLE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTable_put_cellPadding(IHTMLTable *iface, VARIANT v)
{
    HTMLTable *This = HTMLTABLE_THIS(iface);
    FIXME("(%p)->(v)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTable_get_cellPadding(IHTMLTable *iface, VARIANT *p)
{
    HTMLTable *This = HTMLTABLE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTable_put_background(IHTMLTable *iface, BSTR v)
{
    HTMLTable *This = HTMLTABLE_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTable_get_background(IHTMLTable *iface, BSTR *p)
{
    HTMLTable *This = HTMLTABLE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTable_put_bgColor(IHTMLTable *iface, VARIANT v)
{
    HTMLTable *This = HTMLTABLE_THIS(iface);
    FIXME("(%p)->(v)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTable_get_bgColor(IHTMLTable *iface, VARIANT *p)
{
    HTMLTable *This = HTMLTABLE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTable_put_borderColor(IHTMLTable *iface, VARIANT v)
{
    HTMLTable *This = HTMLTABLE_THIS(iface);
    FIXME("(%p)->(v)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTable_get_borderColor(IHTMLTable *iface, VARIANT *p)
{
    HTMLTable *This = HTMLTABLE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTable_put_borderColorLight(IHTMLTable *iface, VARIANT v)
{
    HTMLTable *This = HTMLTABLE_THIS(iface);
    FIXME("(%p)->(v)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTable_get_borderColorLight(IHTMLTable *iface, VARIANT *p)
{
    HTMLTable *This = HTMLTABLE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTable_put_borderColorDark(IHTMLTable *iface, VARIANT v)
{
    HTMLTable *This = HTMLTABLE_THIS(iface);
    FIXME("(%p)->(v)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTable_get_borderColorDark(IHTMLTable *iface, VARIANT *p)
{
    HTMLTable *This = HTMLTABLE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTable_put_align(IHTMLTable *iface, BSTR v)
{
    HTMLTable *This = HTMLTABLE_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTable_get_align(IHTMLTable *iface, BSTR *p)
{
    HTMLTable *This = HTMLTABLE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTable_refresh(IHTMLTable *iface)
{
    HTMLTable *This = HTMLTABLE_THIS(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTable_get_rows(IHTMLTable *iface, IHTMLElementCollection **p)
{
    HTMLTable *This = HTMLTABLE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTable_put_width(IHTMLTable *iface, VARIANT v)
{
    HTMLTable *This = HTMLTABLE_THIS(iface);
    FIXME("(%p)->(v)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTable_get_width(IHTMLTable *iface, VARIANT *p)
{
    HTMLTable *This = HTMLTABLE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTable_put_height(IHTMLTable *iface, VARIANT v)
{
    HTMLTable *This = HTMLTABLE_THIS(iface);
    FIXME("(%p)->(v)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTable_get_height(IHTMLTable *iface, VARIANT *p)
{
    HTMLTable *This = HTMLTABLE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTable_put_dataPageSize(IHTMLTable *iface, long v)
{
    HTMLTable *This = HTMLTABLE_THIS(iface);
    FIXME("(%p)->(%ld)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTable_get_dataPageSize(IHTMLTable *iface, long *p)
{
    HTMLTable *This = HTMLTABLE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTable_nextPage(IHTMLTable *iface)
{
    HTMLTable *This = HTMLTABLE_THIS(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTable_previousPage(IHTMLTable *iface)
{
    HTMLTable *This = HTMLTABLE_THIS(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTable_get_tHead(IHTMLTable *iface, IHTMLTableSection **p)
{
    HTMLTable *This = HTMLTABLE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTable_get_tFoot(IHTMLTable *iface, IHTMLTableSection **p)
{
    HTMLTable *This = HTMLTABLE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTable_get_tBodies(IHTMLTable *iface, IHTMLElementCollection **p)
{
    HTMLTable *This = HTMLTABLE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTable_get_caption(IHTMLTable *iface, IHTMLTableCaption **p)
{
    HTMLTable *This = HTMLTABLE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTable_createTHead(IHTMLTable *iface, IDispatch **head)
{
    HTMLTable *This = HTMLTABLE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, head);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTable_deleteTHead(IHTMLTable *iface)
{
    HTMLTable *This = HTMLTABLE_THIS(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTable_createTFoot(IHTMLTable *iface, IDispatch **foot)
{
    HTMLTable *This = HTMLTABLE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, foot);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTable_deleteTFoot(IHTMLTable *iface)
{
    HTMLTable *This = HTMLTABLE_THIS(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTable_createCaption(IHTMLTable *iface, IHTMLTableCaption **caption)
{
    HTMLTable *This = HTMLTABLE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, caption);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTable_deleteCaption(IHTMLTable *iface)
{
    HTMLTable *This = HTMLTABLE_THIS(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTable_insertRow(IHTMLTable *iface, long index, IDispatch **row)
{
    HTMLTable *This = HTMLTABLE_THIS(iface);
    FIXME("(%p)->(%ld %p)\n", This, index, row);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTable_deleteRow(IHTMLTable *iface, long index)
{
    HTMLTable *This = HTMLTABLE_THIS(iface);
    FIXME("(%p)->(%ld)\n", This, index);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTable_get_readyState(IHTMLTable *iface, BSTR *p)
{
    HTMLTable *This = HTMLTABLE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTable_put_onreadystatechange(IHTMLTable *iface, VARIANT v)
{
    HTMLTable *This = HTMLTABLE_THIS(iface);
    FIXME("(%p)->(v)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTable_get_onreadystatechange(IHTMLTable *iface, VARIANT *p)
{
    HTMLTable *This = HTMLTABLE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

#undef HTMLTABLE_THIS

static const IHTMLTableVtbl HTMLTableVtbl = {
    HTMLTable_QueryInterface,
    HTMLTable_AddRef,
    HTMLTable_Release,
    HTMLTable_GetTypeInfoCount,
    HTMLTable_GetTypeInfo,
    HTMLTable_GetIDsOfNames,
    HTMLTable_Invoke,
    HTMLTable_put_cols,
    HTMLTable_get_cols,
    HTMLTable_put_border,
    HTMLTable_get_border,
    HTMLTable_put_frame,
    HTMLTable_get_frame,
    HTMLTable_put_rules,
    HTMLTable_get_rules,
    HTMLTable_put_cellSpacing,
    HTMLTable_get_cellSpacing,
    HTMLTable_put_cellPadding,
    HTMLTable_get_cellPadding,
    HTMLTable_put_background,
    HTMLTable_get_background,
    HTMLTable_put_bgColor,
    HTMLTable_get_bgColor,
    HTMLTable_put_borderColor,
    HTMLTable_get_borderColor,
    HTMLTable_put_borderColorLight,
    HTMLTable_get_borderColorLight,
    HTMLTable_put_borderColorDark,
    HTMLTable_get_borderColorDark,
    HTMLTable_put_align,
    HTMLTable_get_align,
    HTMLTable_refresh,
    HTMLTable_get_rows,
    HTMLTable_put_width,
    HTMLTable_get_width,
    HTMLTable_put_height,
    HTMLTable_get_height,
    HTMLTable_put_dataPageSize,
    HTMLTable_get_dataPageSize,
    HTMLTable_nextPage,
    HTMLTable_previousPage,
    HTMLTable_get_tHead,
    HTMLTable_get_tFoot,
    HTMLTable_get_tBodies,
    HTMLTable_get_caption,
    HTMLTable_createTHead,
    HTMLTable_deleteTHead,
    HTMLTable_createTFoot,
    HTMLTable_deleteTFoot,
    HTMLTable_createCaption,
    HTMLTable_deleteCaption,
    HTMLTable_insertRow,
    HTMLTable_deleteRow,
    HTMLTable_get_readyState,
    HTMLTable_put_onreadystatechange,
    HTMLTable_get_onreadystatechange
};

#define HTMLTABLE_NODE_THIS(iface) DEFINE_THIS2(HTMLTable, element.node, iface)

static HRESULT HTMLTable_QI(HTMLDOMNode *iface, REFIID riid, void **ppv)
{
    HTMLTable *This = HTMLTABLE_NODE_THIS(iface);

    *ppv = NULL;

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = HTMLTABLE(This);
    }else if(IsEqualGUID(&IID_IDispatch, riid)) {
        TRACE("(%p)->(IID_IDispatch %p)\n", This, ppv);
        *ppv = HTMLTABLE(This);
    }else if(IsEqualGUID(&IID_IHTMLTable, riid)) {
        TRACE("(%p)->(IID_IHTMLTable %p)\n", This, ppv);
        *ppv = HTMLTABLE(This);
    }

    if(*ppv) {
        IUnknown_AddRef((IUnknown*)*ppv);
        return S_OK;
    }

    return HTMLElement_QI(&This->element.node, riid, ppv);
}

static void HTMLTable_destructor(HTMLDOMNode *iface)
{
    HTMLTable *This = HTMLTABLE_NODE_THIS(iface);
    HTMLElement_destructor(&This->element.node);
}

#undef HTMLTABLE_NODE_THIS

static const NodeImplVtbl HTMLTableImplVtbl = {
    HTMLTable_QI,
    HTMLTable_destructor
};

HTMLElement *HTMLTable_Create(nsIDOMHTMLElement *nselem)
{
    HTMLTable *ret = heap_alloc_zero(sizeof(HTMLTable));

    HTMLElement_Init(&ret->element);

    ret->element.node.vtbl = &HTMLTableImplVtbl;
    ret->lpHTMLTableVtbl = &HTMLTableVtbl;

    ConnectionPoint_Init(&ret->cp, &ret->element.cp_container, &DIID_HTMLTableEvents);

    return &ret->element;
}
