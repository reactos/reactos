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

    const IHTMLTextAreaElementVtbl *lpHTMLTextAreaElementVtbl;

    nsIDOMHTMLTextAreaElement *nstextarea;
} HTMLTextAreaElement;

#define HTMLTXTAREA(x)  ((IHTMLTextAreaElement*)  &(x)->lpHTMLTextAreaElementVtbl)

#define HTMLTXTAREA_THIS(iface) DEFINE_THIS(HTMLTextAreaElement, HTMLTextAreaElement, iface)

static HRESULT WINAPI HTMLTextAreaElement_QueryInterface(IHTMLTextAreaElement *iface,
                                                         REFIID riid, void **ppv)
{
    HTMLTextAreaElement *This = HTMLTXTAREA_THIS(iface);

    return IHTMLDOMNode_QueryInterface(HTMLDOMNODE(&This->element.node), riid, ppv);
}

static ULONG WINAPI HTMLTextAreaElement_AddRef(IHTMLTextAreaElement *iface)
{
    HTMLTextAreaElement *This = HTMLTXTAREA_THIS(iface);

    return IHTMLDOMNode_AddRef(HTMLDOMNODE(&This->element.node));
}

static ULONG WINAPI HTMLTextAreaElement_Release(IHTMLTextAreaElement *iface)
{
    HTMLTextAreaElement *This = HTMLTXTAREA_THIS(iface);

    return IHTMLDOMNode_Release(HTMLDOMNODE(&This->element.node));
}

static HRESULT WINAPI HTMLTextAreaElement_GetTypeInfoCount(IHTMLTextAreaElement *iface, UINT *pctinfo)
{
    HTMLTextAreaElement *This = HTMLTXTAREA_THIS(iface);
    return IDispatchEx_GetTypeInfoCount(DISPATCHEX(&This->element.node.dispex), pctinfo);
}

static HRESULT WINAPI HTMLTextAreaElement_GetTypeInfo(IHTMLTextAreaElement *iface, UINT iTInfo,
                                              LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLTextAreaElement *This = HTMLTXTAREA_THIS(iface);
    return IDispatchEx_GetTypeInfo(DISPATCHEX(&This->element.node.dispex), iTInfo, lcid, ppTInfo);
}

static HRESULT WINAPI HTMLTextAreaElement_GetIDsOfNames(IHTMLTextAreaElement *iface, REFIID riid,
                                                LPOLESTR *rgszNames, UINT cNames,
                                                LCID lcid, DISPID *rgDispId)
{
    HTMLTextAreaElement *This = HTMLTXTAREA_THIS(iface);
    return IDispatchEx_GetIDsOfNames(DISPATCHEX(&This->element.node.dispex), riid, rgszNames, cNames, lcid, rgDispId);
}

static HRESULT WINAPI HTMLTextAreaElement_Invoke(IHTMLTextAreaElement *iface, DISPID dispIdMember,
                            REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
                            VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLTextAreaElement *This = HTMLTXTAREA_THIS(iface);
    return IDispatchEx_Invoke(DISPATCHEX(&This->element.node.dispex), dispIdMember, riid, lcid, wFlags, pDispParams,
            pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI HTMLTextAreaElement_get_type(IHTMLTextAreaElement *iface, BSTR *p)
{
    HTMLTextAreaElement *This = HTMLTXTAREA_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTextAreaElement_put_value(IHTMLTextAreaElement *iface, BSTR v)
{
    HTMLTextAreaElement *This = HTMLTXTAREA_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTextAreaElement_get_value(IHTMLTextAreaElement *iface, BSTR *p)
{
    HTMLTextAreaElement *This = HTMLTXTAREA_THIS(iface);
    nsAString value_str;
    const PRUnichar *value;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsAString_Init(&value_str, NULL);

    nsres = nsIDOMHTMLTextAreaElement_GetValue(This->nstextarea, &value_str);
    if(NS_SUCCEEDED(nsres)) {
        nsAString_GetData(&value_str, &value);
        *p = SysAllocString(value);
    }else {
        ERR("GetValue failed: %08x\n", nsres);
    }

    nsAString_Finish(&value_str);

    TRACE("%s\n", debugstr_w(*p));
    return S_OK;
}

static HRESULT WINAPI HTMLTextAreaElement_put_name(IHTMLTextAreaElement *iface, BSTR v)
{
    HTMLTextAreaElement *This = HTMLTXTAREA_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTextAreaElement_get_name(IHTMLTextAreaElement *iface, BSTR *p)
{
    HTMLTextAreaElement *This = HTMLTXTAREA_THIS(iface);
    nsAString name_str;
    const PRUnichar *name;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsAString_Init(&name_str, NULL);

    nsres = nsIDOMHTMLTextAreaElement_GetName(This->nstextarea, &name_str);
    if(NS_SUCCEEDED(nsres)) {
        nsAString_GetData(&name_str, &name);
        *p = SysAllocString(name);
    }else {
        ERR("GetName failed: %08x\n", nsres);
    }

    nsAString_Finish(&name_str);

    TRACE("%s\n", debugstr_w(*p));
    return S_OK;
}

static HRESULT WINAPI HTMLTextAreaElement_put_status(IHTMLTextAreaElement *iface, VARIANT v)
{
    HTMLTextAreaElement *This = HTMLTXTAREA_THIS(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTextAreaElement_get_status(IHTMLTextAreaElement *iface, VARIANT *p)
{
    HTMLTextAreaElement *This = HTMLTXTAREA_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTextAreaElement_put_disabled(IHTMLTextAreaElement *iface, VARIANT_BOOL v)
{
    HTMLTextAreaElement *This = HTMLTXTAREA_THIS(iface);
    FIXME("(%p)->(%x)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTextAreaElement_get_disabled(IHTMLTextAreaElement *iface, VARIANT_BOOL *p)
{
    HTMLTextAreaElement *This = HTMLTXTAREA_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTextAreaElement_get_form(IHTMLTextAreaElement *iface, IHTMLFormElement **p)
{
    HTMLTextAreaElement *This = HTMLTXTAREA_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTextAreaElement_put_defaultValue(IHTMLTextAreaElement *iface, BSTR v)
{
    HTMLTextAreaElement *This = HTMLTXTAREA_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTextAreaElement_get_defaultValue(IHTMLTextAreaElement *iface, BSTR *p)
{
    HTMLTextAreaElement *This = HTMLTXTAREA_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTextAreaElement_select(IHTMLTextAreaElement *iface)
{
    HTMLTextAreaElement *This = HTMLTXTAREA_THIS(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTextAreaElement_put_onchange(IHTMLTextAreaElement *iface, VARIANT v)
{
    HTMLTextAreaElement *This = HTMLTXTAREA_THIS(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTextAreaElement_get_onchange(IHTMLTextAreaElement *iface, VARIANT *p)
{
    HTMLTextAreaElement *This = HTMLTXTAREA_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTextAreaElement_put_onselect(IHTMLTextAreaElement *iface, VARIANT v)
{
    HTMLTextAreaElement *This = HTMLTXTAREA_THIS(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTextAreaElement_get_onselect(IHTMLTextAreaElement *iface, VARIANT *p)
{
    HTMLTextAreaElement *This = HTMLTXTAREA_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTextAreaElement_put_readOnly(IHTMLTextAreaElement *iface, VARIANT_BOOL v)
{
    HTMLTextAreaElement *This = HTMLTXTAREA_THIS(iface);
    FIXME("(%p)->(%x)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTextAreaElement_get_readOnly(IHTMLTextAreaElement *iface, VARIANT_BOOL *p)
{
    HTMLTextAreaElement *This = HTMLTXTAREA_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTextAreaElement_put_rows(IHTMLTextAreaElement *iface, long v)
{
    HTMLTextAreaElement *This = HTMLTXTAREA_THIS(iface);
    FIXME("(%p)->(%ld)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTextAreaElement_get_rows(IHTMLTextAreaElement *iface, long *p)
{
    HTMLTextAreaElement *This = HTMLTXTAREA_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTextAreaElement_put_cols(IHTMLTextAreaElement *iface, long v)
{
    HTMLTextAreaElement *This = HTMLTXTAREA_THIS(iface);
    FIXME("(%p)->(%ld)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTextAreaElement_get_cols(IHTMLTextAreaElement *iface, long *p)
{
    HTMLTextAreaElement *This = HTMLTXTAREA_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTextAreaElement_put_wrap(IHTMLTextAreaElement *iface, BSTR v)
{
    HTMLTextAreaElement *This = HTMLTXTAREA_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTextAreaElement_get_wrap(IHTMLTextAreaElement *iface, BSTR *p)
{
    HTMLTextAreaElement *This = HTMLTXTAREA_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTextAreaElement_createTextRange(IHTMLTextAreaElement *iface,
                                                          IHTMLTxtRange **range)
{
    HTMLTextAreaElement *This = HTMLTXTAREA_THIS(iface);
    FIXME("(%p)->(%p)\n", This, range);
    return E_NOTIMPL;
}

#undef HTMLTXTAREA_THIS

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

#define HTMLTXTAREA_NODE_THIS(iface) DEFINE_THIS2(HTMLTextAreaElement, element.node, iface)

static HRESULT HTMLTextAreaElement_QI(HTMLDOMNode *iface, REFIID riid, void **ppv)
{
    HTMLTextAreaElement *This = HTMLTXTAREA_NODE_THIS(iface);

    *ppv = NULL;

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = HTMLTXTAREA(This);
    }else if(IsEqualGUID(&IID_IDispatch, riid)) {
        TRACE("(%p)->(IID_IDispatch %p)\n", This, ppv);
        *ppv = HTMLTXTAREA(This);
    }else if(IsEqualGUID(&IID_IHTMLTextAreaElement, riid)) {
        TRACE("(%p)->(IID_IHTMLTextAreaElement %p)\n", This, ppv);
        *ppv = HTMLTXTAREA(This);
    }

    if(*ppv) {
        IUnknown_AddRef((IUnknown*)*ppv);
        return S_OK;
    }

    return HTMLElement_QI(&This->element.node, riid, ppv);
}

static void HTMLTextAreaElement_destructor(HTMLDOMNode *iface)
{
    HTMLTextAreaElement *This = HTMLTXTAREA_NODE_THIS(iface);

    nsIDOMHTMLTextAreaElement_Release(This->nstextarea);

    HTMLElement_destructor(&This->element.node);
}

static HRESULT HTMLTextAreaElementImpl_put_disabled(HTMLDOMNode *iface, VARIANT_BOOL v)
{
    HTMLTextAreaElement *This = HTMLTXTAREA_NODE_THIS(iface);
    return IHTMLTextAreaElement_put_disabled(HTMLTXTAREA(This), v);
}

static HRESULT HTMLTextAreaElementImpl_get_disabled(HTMLDOMNode *iface, VARIANT_BOOL *p)
{
    HTMLTextAreaElement *This = HTMLTXTAREA_NODE_THIS(iface);
    return IHTMLTextAreaElement_get_disabled(HTMLTXTAREA(This), p);
}

#undef HTMLTXTAREA_NODE_THIS

static const NodeImplVtbl HTMLTextAreaElementImplVtbl = {
    HTMLTextAreaElement_QI,
    HTMLTextAreaElement_destructor,
    HTMLTextAreaElementImpl_put_disabled,
    HTMLTextAreaElementImpl_get_disabled
};

HTMLElement *HTMLTextAreaElement_Create(nsIDOMHTMLElement *nselem)
{
    HTMLTextAreaElement *ret = heap_alloc_zero(sizeof(HTMLTextAreaElement));
    nsresult nsres;

    HTMLElement_Init(&ret->element);

    ret->lpHTMLTextAreaElementVtbl = &HTMLTextAreaElementVtbl;
    ret->element.node.vtbl = &HTMLTextAreaElementImplVtbl;

    nsres = nsIDOMHTMLElement_QueryInterface(nselem, &IID_nsIDOMHTMLTextAreaElement,
                                             (void**)&ret->nstextarea);
    if(NS_FAILED(nsres))
        ERR("Could not get nsDOMHTMLInputElement: %08x\n", nsres);

    return &ret->element;
}
