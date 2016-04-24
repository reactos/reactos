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

static const WCHAR autoW[] = {'a','u','t','o',0};
static const WCHAR yesW[] = {'y','e','s',0};
static const WCHAR noW[] = {'n','o',0};
static const WCHAR pxW[] = {'p','x',0};

HRESULT set_frame_doc(HTMLFrameBase *frame, nsIDOMDocument *nsdoc)
{
    nsIDOMWindow *nswindow;
    HTMLOuterWindow *window;
    nsresult nsres;
    HRESULT hres = S_OK;

    if(frame->content_window)
        return S_OK;

    nsres = nsIDOMDocument_GetDefaultView(nsdoc, &nswindow);
    if(NS_FAILED(nsres) || !nswindow)
        return E_FAIL;

    window = nswindow_to_window(nswindow);
    if(!window)
        hres = HTMLOuterWindow_Create(frame->element.node.doc->basedoc.doc_obj, nswindow,
                frame->element.node.doc->basedoc.window, &window);
    nsIDOMWindow_Release(nswindow);
    if(FAILED(hres))
        return hres;

    frame->content_window = window;
    window->frame_element = frame;
    return S_OK;
}

static inline HTMLFrameBase *impl_from_IHTMLFrameBase(IHTMLFrameBase *iface)
{
    return CONTAINING_RECORD(iface, HTMLFrameBase, IHTMLFrameBase_iface);
}

static HRESULT WINAPI HTMLFrameBase_QueryInterface(IHTMLFrameBase *iface, REFIID riid, void **ppv)
{
    HTMLFrameBase *This = impl_from_IHTMLFrameBase(iface);

    return IHTMLDOMNode_QueryInterface(&This->element.node.IHTMLDOMNode_iface, riid, ppv);
}

static ULONG WINAPI HTMLFrameBase_AddRef(IHTMLFrameBase *iface)
{
    HTMLFrameBase *This = impl_from_IHTMLFrameBase(iface);

    return IHTMLDOMNode_AddRef(&This->element.node.IHTMLDOMNode_iface);
}

static ULONG WINAPI HTMLFrameBase_Release(IHTMLFrameBase *iface)
{
    HTMLFrameBase *This = impl_from_IHTMLFrameBase(iface);

    return IHTMLDOMNode_Release(&This->element.node.IHTMLDOMNode_iface);
}

static HRESULT WINAPI HTMLFrameBase_GetTypeInfoCount(IHTMLFrameBase *iface, UINT *pctinfo)
{
    HTMLFrameBase *This = impl_from_IHTMLFrameBase(iface);

    return IDispatchEx_GetTypeInfoCount(&This->element.node.event_target.dispex.IDispatchEx_iface, pctinfo);
}

static HRESULT WINAPI HTMLFrameBase_GetTypeInfo(IHTMLFrameBase *iface, UINT iTInfo,
        LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLFrameBase *This = impl_from_IHTMLFrameBase(iface);

    return IDispatchEx_GetTypeInfo(&This->element.node.event_target.dispex.IDispatchEx_iface, iTInfo, lcid,
            ppTInfo);
}

static HRESULT WINAPI HTMLFrameBase_GetIDsOfNames(IHTMLFrameBase *iface, REFIID riid,
        LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{
    HTMLFrameBase *This = impl_from_IHTMLFrameBase(iface);

    return IDispatchEx_GetIDsOfNames(&This->element.node.event_target.dispex.IDispatchEx_iface, riid, rgszNames,
            cNames, lcid, rgDispId);
}

static HRESULT WINAPI HTMLFrameBase_Invoke(IHTMLFrameBase *iface, DISPID dispIdMember,
        REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
        VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLFrameBase *This = impl_from_IHTMLFrameBase(iface);

    return IDispatchEx_Invoke(&This->element.node.event_target.dispex.IDispatchEx_iface, dispIdMember, riid,
            lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI HTMLFrameBase_put_src(IHTMLFrameBase *iface, BSTR v)
{
    HTMLFrameBase *This = impl_from_IHTMLFrameBase(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    if(!This->content_window || !This->element.node.doc || !This->element.node.doc->basedoc.window) {
        nsAString nsstr;
        nsresult nsres;

        nsAString_InitDepend(&nsstr, v);
        if(This->nsframe)
            nsres = nsIDOMHTMLFrameElement_SetSrc(This->nsframe, &nsstr);
        else
            nsres = nsIDOMHTMLIFrameElement_SetSrc(This->nsiframe, &nsstr);
        nsAString_Finish(&nsstr);
        if(NS_FAILED(nsres)) {
            ERR("SetSrc failed: %08x\n", nsres);
            return E_FAIL;
        }

        return S_OK;
    }

    return navigate_url(This->content_window, v, This->element.node.doc->basedoc.window->uri, BINDING_NAVIGATED);
}

static HRESULT WINAPI HTMLFrameBase_get_src(IHTMLFrameBase *iface, BSTR *p)
{
    HTMLFrameBase *This = impl_from_IHTMLFrameBase(iface);
    nsAString nsstr;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    if(!This->nsframe && !This->nsiframe) {
        ERR("No attached frame object\n");
        return E_UNEXPECTED;
    }

    nsAString_Init(&nsstr, NULL);
    if(This->nsframe)
        nsres = nsIDOMHTMLFrameElement_GetSrc(This->nsframe, &nsstr);
    else
        nsres = nsIDOMHTMLIFrameElement_GetSrc(This->nsiframe, &nsstr);
    return return_nsstr(nsres, &nsstr, p);
}

static HRESULT WINAPI HTMLFrameBase_put_name(IHTMLFrameBase *iface, BSTR v)
{
    HTMLFrameBase *This = impl_from_IHTMLFrameBase(iface);
    nsAString name_str;
    nsresult nsres;

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    if(!This->nsframe && !This->nsiframe) {
        ERR("No attached ns frame object\n");
        return E_UNEXPECTED;
    }

    nsAString_InitDepend(&name_str, v);
    if(This->nsframe)
        nsres = nsIDOMHTMLFrameElement_SetName(This->nsframe, &name_str);
    else
        nsres = nsIDOMHTMLIFrameElement_SetName(This->nsiframe, &name_str);
    nsAString_Finish(&name_str);
    if(NS_FAILED(nsres)) {
        ERR("SetName failed: %08x\n", nsres);
        return E_FAIL;
    }

    return S_OK;
}

static HRESULT WINAPI HTMLFrameBase_get_name(IHTMLFrameBase *iface, BSTR *p)
{
    HTMLFrameBase *This = impl_from_IHTMLFrameBase(iface);
    nsAString nsstr;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    if(!This->nsframe && !This->nsiframe) {
        ERR("No attached ns frame object\n");
        return E_UNEXPECTED;
    }

    nsAString_Init(&nsstr, NULL);
    if(This->nsframe)
        nsres = nsIDOMHTMLFrameElement_GetName(This->nsframe, &nsstr);
    else
        nsres = nsIDOMHTMLIFrameElement_GetName(This->nsiframe, &nsstr);
    return return_nsstr(nsres, &nsstr, p);
}

static HRESULT WINAPI HTMLFrameBase_put_border(IHTMLFrameBase *iface, VARIANT v)
{
    HTMLFrameBase *This = impl_from_IHTMLFrameBase(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLFrameBase_get_border(IHTMLFrameBase *iface, VARIANT *p)
{
    HTMLFrameBase *This = impl_from_IHTMLFrameBase(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLFrameBase_put_frameBorder(IHTMLFrameBase *iface, BSTR v)
{
    HTMLFrameBase *This = impl_from_IHTMLFrameBase(iface);
    nsAString nsstr;
    nsresult nsres;

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    if(!This->nsframe && !This->nsiframe) {
        ERR("No attached ns frame object\n");
        return E_UNEXPECTED;
    }

    nsAString_InitDepend(&nsstr, v);
    if(This->nsframe)
        nsres = nsIDOMHTMLFrameElement_SetFrameBorder(This->nsframe, &nsstr);
    else
        nsres = nsIDOMHTMLIFrameElement_SetFrameBorder(This->nsiframe, &nsstr);
    nsAString_Finish(&nsstr);
    if(NS_FAILED(nsres)) {
        ERR("SetFrameBorder failed: %08x\n", nsres);
        return E_FAIL;
    }

    return S_OK;
}

static HRESULT WINAPI HTMLFrameBase_get_frameBorder(IHTMLFrameBase *iface, BSTR *p)
{
    HTMLFrameBase *This = impl_from_IHTMLFrameBase(iface);
    nsAString nsstr;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    if(!This->nsframe && !This->nsiframe) {
        ERR("No attached ns frame object\n");
        return E_UNEXPECTED;
    }

    nsAString_Init(&nsstr, NULL);
    if(This->nsframe)
        nsres = nsIDOMHTMLFrameElement_GetFrameBorder(This->nsframe, &nsstr);
    else
        nsres = nsIDOMHTMLIFrameElement_GetFrameBorder(This->nsiframe, &nsstr);
    return return_nsstr(nsres, &nsstr, p);
}

static HRESULT WINAPI HTMLFrameBase_put_frameSpacing(IHTMLFrameBase *iface, VARIANT v)
{
    HTMLFrameBase *This = impl_from_IHTMLFrameBase(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLFrameBase_get_frameSpacing(IHTMLFrameBase *iface, VARIANT *p)
{
    HTMLFrameBase *This = impl_from_IHTMLFrameBase(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLFrameBase_put_marginWidth(IHTMLFrameBase *iface, VARIANT v)
{
    HTMLFrameBase *This = impl_from_IHTMLFrameBase(iface);
    nsAString nsstr;
    nsresult nsres;

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    if(V_VT(&v) != VT_BSTR) {
        FIXME("unsupported %s\n", debugstr_variant(&v));
        return E_NOTIMPL;
    }

    nsAString_InitDepend(&nsstr, V_BSTR(&v));
    if(This->nsframe)
        nsres = nsIDOMHTMLFrameElement_SetMarginWidth(This->nsframe, &nsstr);
    else
        nsres = nsIDOMHTMLIFrameElement_SetMarginWidth(This->nsiframe, &nsstr);
    nsAString_Finish(&nsstr);
    return NS_SUCCEEDED(nsres) ? S_OK : E_FAIL;
}

static HRESULT WINAPI HTMLFrameBase_get_marginWidth(IHTMLFrameBase *iface, VARIANT *p)
{
    HTMLFrameBase *This = impl_from_IHTMLFrameBase(iface);
    nsAString nsstr;
    nsresult nsres;
    HRESULT hres = S_OK;

    TRACE("(%p)->(%p)\n", This, p);

    nsAString_Init(&nsstr, NULL);
    if(This->nsframe)
        nsres = nsIDOMHTMLFrameElement_GetMarginWidth(This->nsframe, &nsstr);
    else
        nsres = nsIDOMHTMLIFrameElement_GetMarginWidth(This->nsiframe, &nsstr);
    if(NS_SUCCEEDED(nsres)) {
        const PRUnichar *str, *end;

        nsAString_GetData(&nsstr, &str);

        if(*str) {
            BSTR ret;

            end = strstrW(str, pxW);
            if(!end)
                end = str+strlenW(str);
            ret = SysAllocStringLen(str, end-str);
            if(ret) {
                V_VT(p) = VT_BSTR;
                V_BSTR(p) = ret;
            }else {
                hres = E_OUTOFMEMORY;
            }
        }else {
            V_VT(p) = VT_BSTR;
            V_BSTR(p) = NULL;
        }
    }else {
        ERR("GetMarginWidth failed: %08x\n", nsres);
        hres = E_FAIL;
    }

    nsAString_Finish(&nsstr);
    return hres;
}

static HRESULT WINAPI HTMLFrameBase_put_marginHeight(IHTMLFrameBase *iface, VARIANT v)
{
    HTMLFrameBase *This = impl_from_IHTMLFrameBase(iface);
    nsAString nsstr;
    nsresult nsres;

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    if(V_VT(&v) != VT_BSTR) {
        FIXME("unsupported %s\n", debugstr_variant(&v));
        return E_NOTIMPL;
    }

    nsAString_InitDepend(&nsstr, V_BSTR(&v));
    if(This->nsframe)
        nsres = nsIDOMHTMLFrameElement_SetMarginHeight(This->nsframe, &nsstr);
    else
        nsres = nsIDOMHTMLIFrameElement_SetMarginHeight(This->nsiframe, &nsstr);
    nsAString_Finish(&nsstr);
    return NS_SUCCEEDED(nsres) ? S_OK : E_FAIL;
}

static HRESULT WINAPI HTMLFrameBase_get_marginHeight(IHTMLFrameBase *iface, VARIANT *p)
{
    HTMLFrameBase *This = impl_from_IHTMLFrameBase(iface);
    nsAString nsstr;
    nsresult nsres;
    HRESULT hres = S_OK;

    TRACE("(%p)->(%p)\n", This, p);

    nsAString_Init(&nsstr, NULL);
    if(This->nsframe)
        nsres = nsIDOMHTMLFrameElement_GetMarginHeight(This->nsframe, &nsstr);
    else
        nsres = nsIDOMHTMLIFrameElement_GetMarginHeight(This->nsiframe, &nsstr);
    if(NS_SUCCEEDED(nsres)) {
        const PRUnichar *str, *end;

        nsAString_GetData(&nsstr, &str);

        if(*str) {
            BSTR ret;

            end = strstrW(str, pxW);
            if(!end)
                end = str+strlenW(str);
            ret = SysAllocStringLen(str, end-str);
            if(ret) {
                V_VT(p) = VT_BSTR;
                V_BSTR(p) = ret;
            }else {
                hres = E_OUTOFMEMORY;
            }
        }else {
            V_VT(p) = VT_BSTR;
            V_BSTR(p) = NULL;
        }
    }else {
        ERR("SetMarginHeight failed: %08x\n", nsres);
        hres = E_FAIL;
    }

    nsAString_Finish(&nsstr);
    return hres;
}

static HRESULT WINAPI HTMLFrameBase_put_noResize(IHTMLFrameBase *iface, VARIANT_BOOL v)
{
    HTMLFrameBase *This = impl_from_IHTMLFrameBase(iface);
    FIXME("(%p)->(%x)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLFrameBase_get_noResize(IHTMLFrameBase *iface, VARIANT_BOOL *p)
{
    HTMLFrameBase *This = impl_from_IHTMLFrameBase(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLFrameBase_put_scrolling(IHTMLFrameBase *iface, BSTR v)
{
    HTMLFrameBase *This = impl_from_IHTMLFrameBase(iface);
    nsAString nsstr;
    nsresult nsres;

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    if(!(!strcmpiW(v, yesW) || !strcmpiW(v, noW) || !strcmpiW(v, autoW)))
        return E_INVALIDARG;

    if(This->nsframe) {
        nsAString_InitDepend(&nsstr, v);
        nsres = nsIDOMHTMLFrameElement_SetScrolling(This->nsframe, &nsstr);
    }else if(This->nsiframe) {
        nsAString_InitDepend(&nsstr, v);
        nsres = nsIDOMHTMLIFrameElement_SetScrolling(This->nsiframe, &nsstr);
    }else {
        ERR("No attached ns frame object\n");
        return E_UNEXPECTED;
    }
    nsAString_Finish(&nsstr);

    if(NS_FAILED(nsres)) {
        ERR("SetScrolling failed: 0x%08x\n", nsres);
        return E_FAIL;
    }

    return S_OK;
}

static HRESULT WINAPI HTMLFrameBase_get_scrolling(IHTMLFrameBase *iface, BSTR *p)
{
    HTMLFrameBase *This = impl_from_IHTMLFrameBase(iface);
    nsAString nsstr;
    const PRUnichar *strdata;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    if(This->nsframe) {
        nsAString_Init(&nsstr, NULL);
        nsres = nsIDOMHTMLFrameElement_GetScrolling(This->nsframe, &nsstr);
    }else if(This->nsiframe) {
        nsAString_Init(&nsstr, NULL);
        nsres = nsIDOMHTMLIFrameElement_GetScrolling(This->nsiframe, &nsstr);
    }else {
        ERR("No attached ns frame object\n");
        return E_UNEXPECTED;
    }

    if(NS_FAILED(nsres)) {
        ERR("GetScrolling failed: 0x%08x\n", nsres);
        nsAString_Finish(&nsstr);
        return E_FAIL;
    }

    nsAString_GetData(&nsstr, &strdata);

    if(*strdata)
        *p = SysAllocString(strdata);
    else
        *p = SysAllocString(autoW);

    nsAString_Finish(&nsstr);

    return *p ? S_OK : E_OUTOFMEMORY;
}

static const IHTMLFrameBaseVtbl HTMLFrameBaseVtbl = {
    HTMLFrameBase_QueryInterface,
    HTMLFrameBase_AddRef,
    HTMLFrameBase_Release,
    HTMLFrameBase_GetTypeInfoCount,
    HTMLFrameBase_GetTypeInfo,
    HTMLFrameBase_GetIDsOfNames,
    HTMLFrameBase_Invoke,
    HTMLFrameBase_put_src,
    HTMLFrameBase_get_src,
    HTMLFrameBase_put_name,
    HTMLFrameBase_get_name,
    HTMLFrameBase_put_border,
    HTMLFrameBase_get_border,
    HTMLFrameBase_put_frameBorder,
    HTMLFrameBase_get_frameBorder,
    HTMLFrameBase_put_frameSpacing,
    HTMLFrameBase_get_frameSpacing,
    HTMLFrameBase_put_marginWidth,
    HTMLFrameBase_get_marginWidth,
    HTMLFrameBase_put_marginHeight,
    HTMLFrameBase_get_marginHeight,
    HTMLFrameBase_put_noResize,
    HTMLFrameBase_get_noResize,
    HTMLFrameBase_put_scrolling,
    HTMLFrameBase_get_scrolling
};

static inline HTMLFrameBase *impl_from_IHTMLFrameBase2(IHTMLFrameBase2 *iface)
{
    return CONTAINING_RECORD(iface, HTMLFrameBase, IHTMLFrameBase2_iface);
}

static HRESULT WINAPI HTMLFrameBase2_QueryInterface(IHTMLFrameBase2 *iface, REFIID riid, void **ppv)
{
    HTMLFrameBase *This = impl_from_IHTMLFrameBase2(iface);

    return IHTMLDOMNode_QueryInterface(&This->element.node.IHTMLDOMNode_iface, riid, ppv);
}

static ULONG WINAPI HTMLFrameBase2_AddRef(IHTMLFrameBase2 *iface)
{
    HTMLFrameBase *This = impl_from_IHTMLFrameBase2(iface);

    return IHTMLDOMNode_AddRef(&This->element.node.IHTMLDOMNode_iface);
}

static ULONG WINAPI HTMLFrameBase2_Release(IHTMLFrameBase2 *iface)
{
    HTMLFrameBase *This = impl_from_IHTMLFrameBase2(iface);

    return IHTMLDOMNode_Release(&This->element.node.IHTMLDOMNode_iface);
}

static HRESULT WINAPI HTMLFrameBase2_GetTypeInfoCount(IHTMLFrameBase2 *iface, UINT *pctinfo)
{
    HTMLFrameBase *This = impl_from_IHTMLFrameBase2(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLFrameBase2_GetTypeInfo(IHTMLFrameBase2 *iface, UINT iTInfo,
        LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLFrameBase *This = impl_from_IHTMLFrameBase2(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLFrameBase2_GetIDsOfNames(IHTMLFrameBase2 *iface, REFIID riid,
        LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{
    HTMLFrameBase *This = impl_from_IHTMLFrameBase2(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLFrameBase2_Invoke(IHTMLFrameBase2 *iface, DISPID dispIdMember,
        REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
        VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLFrameBase *This = impl_from_IHTMLFrameBase2(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLFrameBase2_get_contentWindow(IHTMLFrameBase2 *iface, IHTMLWindow2 **p)
{
    HTMLFrameBase *This = impl_from_IHTMLFrameBase2(iface);

    TRACE("(%p)->(%p)\n", This, p);

    if(This->content_window) {
        IHTMLWindow2_AddRef(&This->content_window->base.IHTMLWindow2_iface);
        *p = &This->content_window->base.IHTMLWindow2_iface;
    }else {
        WARN("NULL content window\n");
        *p = NULL;
    }
    return S_OK;
}

static HRESULT WINAPI HTMLFrameBase2_put_onload(IHTMLFrameBase2 *iface, VARIANT v)
{
    HTMLFrameBase *This = impl_from_IHTMLFrameBase2(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_node_event(&This->element.node, EVENTID_LOAD, &v);
}

static HRESULT WINAPI HTMLFrameBase2_get_onload(IHTMLFrameBase2 *iface, VARIANT *p)
{
    HTMLFrameBase *This = impl_from_IHTMLFrameBase2(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_node_event(&This->element.node, EVENTID_LOAD, p);
}

static HRESULT WINAPI HTMLFrameBase2_put_onreadystatechange(IHTMLFrameBase2 *iface, VARIANT v)
{
    HTMLFrameBase *This = impl_from_IHTMLFrameBase2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLFrameBase2_get_onreadystatechange(IHTMLFrameBase2 *iface, VARIANT *p)
{
    HTMLFrameBase *This = impl_from_IHTMLFrameBase2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLFrameBase2_get_readyState(IHTMLFrameBase2 *iface, BSTR *p)
{
    HTMLFrameBase *This = impl_from_IHTMLFrameBase2(iface);

    TRACE("(%p)->(%p)\n", This, p);

    if(!This->content_window || !This->content_window->base.inner_window->doc) {
        FIXME("no document associated\n");
        return E_FAIL;
    }

    return IHTMLDocument2_get_readyState(&This->content_window->base.inner_window->doc->basedoc.IHTMLDocument2_iface, p);
}

static HRESULT WINAPI HTMLFrameBase2_put_allowTransparency(IHTMLFrameBase2 *iface, VARIANT_BOOL v)
{
    HTMLFrameBase *This = impl_from_IHTMLFrameBase2(iface);

    FIXME("(%p)->(%x) semi-stub\n", This, v);

    return S_OK;
}

static HRESULT WINAPI HTMLFrameBase2_get_allowTransparency(IHTMLFrameBase2 *iface, VARIANT_BOOL *p)
{
    HTMLFrameBase *This = impl_from_IHTMLFrameBase2(iface);

    FIXME("(%p)->(%p) semi-stub\n", This, p);

    *p = VARIANT_TRUE;
    return S_OK;
}

static const IHTMLFrameBase2Vtbl HTMLFrameBase2Vtbl = {
    HTMLFrameBase2_QueryInterface,
    HTMLFrameBase2_AddRef,
    HTMLFrameBase2_Release,
    HTMLFrameBase2_GetTypeInfoCount,
    HTMLFrameBase2_GetTypeInfo,
    HTMLFrameBase2_GetIDsOfNames,
    HTMLFrameBase2_Invoke,
    HTMLFrameBase2_get_contentWindow,
    HTMLFrameBase2_put_onload,
    HTMLFrameBase2_get_onload,
    HTMLFrameBase2_put_onreadystatechange,
    HTMLFrameBase2_get_onreadystatechange,
    HTMLFrameBase2_get_readyState,
    HTMLFrameBase2_put_allowTransparency,
    HTMLFrameBase2_get_allowTransparency
};

HRESULT HTMLFrameBase_QI(HTMLFrameBase *This, REFIID riid, void **ppv)
{
    if(IsEqualGUID(&IID_IHTMLFrameBase, riid)) {
        TRACE("(%p)->(IID_IHTMLFrameBase %p)\n", This, ppv);
        *ppv = &This->IHTMLFrameBase_iface;
    }else if(IsEqualGUID(&IID_IHTMLFrameBase2, riid)) {
        TRACE("(%p)->(IID_IHTMLFrameBase2 %p)\n", This, ppv);
        *ppv = &This->IHTMLFrameBase2_iface;
    }else {
        return HTMLElement_QI(&This->element.node, riid, ppv);
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

void HTMLFrameBase_destructor(HTMLFrameBase *This)
{
    if(This->content_window)
        This->content_window->frame_element = NULL;

    HTMLElement_destructor(&This->element.node);
}

void HTMLFrameBase_Init(HTMLFrameBase *This, HTMLDocumentNode *doc, nsIDOMHTMLElement *nselem,
        dispex_static_data_t *dispex_data)
{
    nsresult nsres;

    This->IHTMLFrameBase_iface.lpVtbl = &HTMLFrameBaseVtbl;
    This->IHTMLFrameBase2_iface.lpVtbl = &HTMLFrameBase2Vtbl;

    HTMLElement_Init(&This->element, doc, nselem, dispex_data);

    nsres = nsIDOMHTMLElement_QueryInterface(nselem, &IID_nsIDOMHTMLFrameElement, (void**)&This->nsframe);
    if(NS_FAILED(nsres)) {
        This->nsframe = NULL;
        nsres = nsIDOMHTMLElement_QueryInterface(nselem, &IID_nsIDOMHTMLIFrameElement, (void**)&This->nsiframe);
        assert(nsres == NS_OK);
    }else {
        This->nsiframe = NULL;
    }
}
