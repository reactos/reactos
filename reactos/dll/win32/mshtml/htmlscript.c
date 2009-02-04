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

    const IHTMLScriptElementVtbl *lpHTMLScriptElementVtbl;

    nsIDOMHTMLScriptElement *nsscript;
} HTMLScriptElement;

#define HTMLSCRIPT(x)  ((IHTMLScriptElement*)  &(x)->lpHTMLScriptElementVtbl)

#define HTMLSCRIPT_THIS(iface) DEFINE_THIS(HTMLScriptElement, HTMLScriptElement, iface)

static HRESULT WINAPI HTMLScriptElement_QueryInterface(IHTMLScriptElement *iface,
        REFIID riid, void **ppv)
{
    HTMLScriptElement *This = HTMLSCRIPT_THIS(iface);

    return IHTMLDOMNode_QueryInterface(HTMLDOMNODE(&This->element.node), riid, ppv);
}

static ULONG WINAPI HTMLScriptElement_AddRef(IHTMLScriptElement *iface)
{
    HTMLScriptElement *This = HTMLSCRIPT_THIS(iface);

    return IHTMLDOMNode_AddRef(HTMLDOMNODE(&This->element.node));
}

static ULONG WINAPI HTMLScriptElement_Release(IHTMLScriptElement *iface)
{
    HTMLScriptElement *This = HTMLSCRIPT_THIS(iface);

    return IHTMLDOMNode_Release(HTMLDOMNODE(&This->element.node));
}

static HRESULT WINAPI HTMLScriptElement_GetTypeInfoCount(IHTMLScriptElement *iface, UINT *pctinfo)
{
    HTMLScriptElement *This = HTMLSCRIPT_THIS(iface);
    return IDispatchEx_GetTypeInfoCount(DISPATCHEX(&This->element.node.dispex), pctinfo);
}

static HRESULT WINAPI HTMLScriptElement_GetTypeInfo(IHTMLScriptElement *iface, UINT iTInfo,
                                              LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLScriptElement *This = HTMLSCRIPT_THIS(iface);
    return IDispatchEx_GetTypeInfo(DISPATCHEX(&This->element.node.dispex), iTInfo, lcid, ppTInfo);
}

static HRESULT WINAPI HTMLScriptElement_GetIDsOfNames(IHTMLScriptElement *iface, REFIID riid,
                                                LPOLESTR *rgszNames, UINT cNames,
                                                LCID lcid, DISPID *rgDispId)
{
    HTMLScriptElement *This = HTMLSCRIPT_THIS(iface);
    return IDispatchEx_GetIDsOfNames(DISPATCHEX(&This->element.node.dispex), riid, rgszNames, cNames, lcid, rgDispId);
}

static HRESULT WINAPI HTMLScriptElement_Invoke(IHTMLScriptElement *iface, DISPID dispIdMember,
                            REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
                            VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLScriptElement *This = HTMLSCRIPT_THIS(iface);
    return IDispatchEx_Invoke(DISPATCHEX(&This->element.node.dispex), dispIdMember, riid, lcid, wFlags, pDispParams,
            pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI HTMLScriptElement_put_src(IHTMLScriptElement *iface, BSTR v)
{
    HTMLScriptElement *This = HTMLSCRIPT_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLScriptElement_get_src(IHTMLScriptElement *iface, BSTR *p)
{
    HTMLScriptElement *This = HTMLSCRIPT_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLScriptElement_put_htmlFor(IHTMLScriptElement *iface, BSTR v)
{
    HTMLScriptElement *This = HTMLSCRIPT_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLScriptElement_get_htmlFor(IHTMLScriptElement *iface, BSTR *p)
{
    HTMLScriptElement *This = HTMLSCRIPT_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLScriptElement_put_event(IHTMLScriptElement *iface, BSTR v)
{
    HTMLScriptElement *This = HTMLSCRIPT_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLScriptElement_get_event(IHTMLScriptElement *iface, BSTR *p)
{
    HTMLScriptElement *This = HTMLSCRIPT_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLScriptElement_put_text(IHTMLScriptElement *iface, BSTR v)
{
    HTMLScriptElement *This = HTMLSCRIPT_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLScriptElement_get_text(IHTMLScriptElement *iface, BSTR *p)
{
    HTMLScriptElement *This = HTMLSCRIPT_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLScriptElement_put_defer(IHTMLScriptElement *iface, VARIANT_BOOL v)
{
    HTMLScriptElement *This = HTMLSCRIPT_THIS(iface);
    HRESULT hr = S_OK;
    nsresult nsres;

    TRACE("(%p)->(%x)\n", This, v);

    nsres = nsIDOMHTMLScriptElement_SetDefer(This->nsscript, v != VARIANT_FALSE);
    if(NS_FAILED(nsres))
    {
        hr = E_FAIL;
    }

    return hr;
}

static HRESULT WINAPI HTMLScriptElement_get_defer(IHTMLScriptElement *iface, VARIANT_BOOL *p)
{
    HTMLScriptElement *This = HTMLSCRIPT_THIS(iface);
    PRBool defer = FALSE;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    if(!p)
        return E_INVALIDARG;

    nsres = nsIDOMHTMLScriptElement_GetDefer(This->nsscript, &defer);
    if(NS_FAILED(nsres)) {
        ERR("GetSrc failed: %08x\n", nsres);
    }

    *p = defer ? VARIANT_TRUE : VARIANT_FALSE;

    TRACE("*p = %d\n", *p);
    return S_OK;
}

static HRESULT WINAPI HTMLScriptElement_get_readyState(IHTMLScriptElement *iface, BSTR *p)
{
    HTMLScriptElement *This = HTMLSCRIPT_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLScriptElement_put_onerror(IHTMLScriptElement *iface, VARIANT v)
{
    HTMLScriptElement *This = HTMLSCRIPT_THIS(iface);
    FIXME("(%p)->(v(%d))\n", This, V_VT(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLScriptElement_get_onerror(IHTMLScriptElement *iface, VARIANT *p)
{
    HTMLScriptElement *This = HTMLSCRIPT_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLScriptElement_put_type(IHTMLScriptElement *iface, BSTR v)
{
    HTMLScriptElement *This = HTMLSCRIPT_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLScriptElement_get_type(IHTMLScriptElement *iface, BSTR *p)
{
    HTMLScriptElement *This = HTMLSCRIPT_THIS(iface);
    const PRUnichar *nstype;
    nsAString nstype_str;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsAString_Init(&nstype_str, NULL);
    nsres = nsIDOMHTMLScriptElement_GetType(This->nsscript, &nstype_str);
    if(NS_FAILED(nsres))
        ERR("GetType failed: %08x\n", nsres);

    nsAString_GetData(&nstype_str, &nstype);
    *p = *nstype ? SysAllocString(nstype) : NULL;
    nsAString_Finish(&nstype_str);

    return S_OK;
}

static const IHTMLScriptElementVtbl HTMLScriptElementVtbl = {
    HTMLScriptElement_QueryInterface,
    HTMLScriptElement_AddRef,
    HTMLScriptElement_Release,
    HTMLScriptElement_GetTypeInfoCount,
    HTMLScriptElement_GetTypeInfo,
    HTMLScriptElement_GetIDsOfNames,
    HTMLScriptElement_Invoke,
    HTMLScriptElement_put_src,
    HTMLScriptElement_get_src,
    HTMLScriptElement_put_htmlFor,
    HTMLScriptElement_get_htmlFor,
    HTMLScriptElement_put_event,
    HTMLScriptElement_get_event,
    HTMLScriptElement_put_text,
    HTMLScriptElement_get_text,
    HTMLScriptElement_put_defer,
    HTMLScriptElement_get_defer,
    HTMLScriptElement_get_readyState,
    HTMLScriptElement_put_onerror,
    HTMLScriptElement_get_onerror,
    HTMLScriptElement_put_type,
    HTMLScriptElement_get_type
};

#undef HTMLSCRIPT_THIS

#define HTMLSCRIPT_NODE_THIS(iface) DEFINE_THIS2(HTMLScriptElement, element.node, iface)

static HRESULT HTMLScriptElement_QI(HTMLDOMNode *iface, REFIID riid, void **ppv)
{
    HTMLScriptElement *This = HTMLSCRIPT_NODE_THIS(iface);

    *ppv = NULL;

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = HTMLSCRIPT(This);
    }else if(IsEqualGUID(&IID_IDispatch, riid)) {
        TRACE("(%p)->(IID_IDispatch %p)\n", This, ppv);
        *ppv = HTMLSCRIPT(This);
    }else if(IsEqualGUID(&IID_IHTMLScriptElement, riid)) {
        TRACE("(%p)->(IID_IHTMLScriptElement %p)\n", This, ppv);
        *ppv = HTMLSCRIPT(This);
    }

    if(*ppv) {
        IUnknown_AddRef((IUnknown*)*ppv);
        return S_OK;
    }

    return HTMLElement_QI(&This->element.node, riid, ppv);
}

static void HTMLScriptElement_destructor(HTMLDOMNode *iface)
{
    HTMLScriptElement *This = HTMLSCRIPT_NODE_THIS(iface);
    HTMLElement_destructor(&This->element.node);
}

#undef HTMLSCRIPT_NODE_THIS

static const NodeImplVtbl HTMLScriptElementImplVtbl = {
    HTMLScriptElement_QI,
    HTMLScriptElement_destructor
};

HTMLElement *HTMLScriptElement_Create(nsIDOMHTMLElement *nselem)
{
    HTMLScriptElement *ret = heap_alloc_zero(sizeof(HTMLScriptElement));
    nsresult nsres;

    HTMLElement_Init(&ret->element);

    ret->lpHTMLScriptElementVtbl = &HTMLScriptElementVtbl;
    ret->element.node.vtbl = &HTMLScriptElementImplVtbl;

    nsres = nsIDOMHTMLElement_QueryInterface(nselem, &IID_nsIDOMHTMLScriptElement, (void**)&ret->nsscript);
    if(NS_FAILED(nsres))
        ERR("Could not get nsIDOMHTMLScriptElement: %08x\n", nsres);

    return &ret->element;
}
