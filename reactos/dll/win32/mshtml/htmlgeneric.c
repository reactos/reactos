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

    const IHTMLGenericElementVtbl *lpHTMLGenericElementVtbl;
} HTMLGenericElement;

#define HTMLGENERIC(x)  (&(x)->lpHTMLGenericElementVtbl)

#define HTMLGENERIC_THIS(iface) DEFINE_THIS(HTMLGenericElement, HTMLGenericElement, iface)

static HRESULT WINAPI HTMLGenericElement_QueryInterface(IHTMLGenericElement *iface, REFIID riid, void **ppv)
{
    HTMLGenericElement *This = HTMLGENERIC_THIS(iface);

    return IHTMLDOMNode_QueryInterface(HTMLDOMNODE(&This->element.node), riid, ppv);
}

static ULONG WINAPI HTMLGenericElement_AddRef(IHTMLGenericElement *iface)
{
    HTMLGenericElement *This = HTMLGENERIC_THIS(iface);

    return IHTMLDOMNode_AddRef(HTMLDOMNODE(&This->element.node));
}

static ULONG WINAPI HTMLGenericElement_Release(IHTMLGenericElement *iface)
{
    HTMLGenericElement *This = HTMLGENERIC_THIS(iface);

    return IHTMLDOMNode_Release(HTMLDOMNODE(&This->element.node));
}

static HRESULT WINAPI HTMLGenericElement_GetTypeInfoCount(IHTMLGenericElement *iface, UINT *pctinfo)
{
    HTMLGenericElement *This = HTMLGENERIC_THIS(iface);
    return IDispatchEx_GetTypeInfoCount(DISPATCHEX(&This->element.node.dispex), pctinfo);
}

static HRESULT WINAPI HTMLGenericElement_GetTypeInfo(IHTMLGenericElement *iface, UINT iTInfo,
                                              LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLGenericElement *This = HTMLGENERIC_THIS(iface);
    return IDispatchEx_GetTypeInfo(DISPATCHEX(&This->element.node.dispex), iTInfo, lcid, ppTInfo);
}

static HRESULT WINAPI HTMLGenericElement_GetIDsOfNames(IHTMLGenericElement *iface, REFIID riid,
        LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{
    HTMLGenericElement *This = HTMLGENERIC_THIS(iface);
    return IDispatchEx_GetIDsOfNames(DISPATCHEX(&This->element.node.dispex), riid, rgszNames, cNames, lcid, rgDispId);
}

static HRESULT WINAPI HTMLGenericElement_Invoke(IHTMLGenericElement *iface, DISPID dispIdMember,
        REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
        VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLGenericElement *This = HTMLGENERIC_THIS(iface);
    return IDispatchEx_Invoke(DISPATCHEX(&This->element.node.dispex), dispIdMember, riid, lcid, wFlags, pDispParams,
            pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI HTMLGenericElement_get_recordset(IHTMLGenericElement *iface, IDispatch **p)
{
    HTMLGenericElement *This = HTMLGENERIC_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLGenericElement_namedRecordset(IHTMLGenericElement *iface,
        BSTR dataMember, VARIANT *hierarchy, IDispatch **ppRecordset)
{
    HTMLGenericElement *This = HTMLGENERIC_THIS(iface);
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

#define HTMLGENERIC_NODE_THIS(iface) DEFINE_THIS2(HTMLGenericElement, element.node, iface)

static HRESULT HTMLGenericElement_QI(HTMLDOMNode *iface, REFIID riid, void **ppv)
{
    HTMLGenericElement *This = HTMLGENERIC_NODE_THIS(iface);

    *ppv = NULL;

    if(IsEqualGUID(&IID_IHTMLGenericElement, riid)) {
        TRACE("(%p)->(IID_IHTMLGenericElement %p)\n", This, ppv);
        *ppv = HTMLGENERIC(This);
    }else {
        return HTMLElement_QI(&This->element.node, riid, ppv);
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static void HTMLGenericElement_destructor(HTMLDOMNode *iface)
{
    HTMLGenericElement *This = HTMLGENERIC_NODE_THIS(iface);

    HTMLElement_destructor(&This->element.node);
}

#undef HTMLGENERIC_NODE_THIS

static const NodeImplVtbl HTMLGenericElementImplVtbl = {
    HTMLGenericElement_QI,
    HTMLGenericElement_destructor
};

static const tid_t HTMLGenericElement_iface_tids[] = {
    IHTMLDOMNode_tid,
    IHTMLDOMNode2_tid,
    IHTMLElement_tid,
    IHTMLElement2_tid,
    IHTMLElement3_tid,
    IHTMLGenericElement_tid,
    0
};

static dispex_static_data_t HTMLGenericElement_dispex = {
    NULL,
    DispHTMLGenericElement_tid,
    NULL,
    HTMLGenericElement_iface_tids
};

HTMLElement *HTMLGenericElement_Create(nsIDOMHTMLElement *nselem)
{
    HTMLGenericElement *ret;

    ret = heap_alloc_zero(sizeof(HTMLGenericElement));

    ret->lpHTMLGenericElementVtbl = &HTMLGenericElementVtbl;
    ret->element.node.vtbl = &HTMLGenericElementImplVtbl;

    init_dispex(&ret->element.node.dispex, (IUnknown*)HTMLGENERIC(ret), &HTMLGenericElement_dispex);
    HTMLElement_Init(&ret->element);

    return &ret->element;
}
