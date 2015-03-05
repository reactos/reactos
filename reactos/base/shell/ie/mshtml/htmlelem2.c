/*
 * Copyright 2006-2010 Jacek Caban for CodeWeavers
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
    DispatchEx dispex;
    IHTMLRect IHTMLRect_iface;

    LONG ref;

    nsIDOMClientRect *nsrect;
} HTMLRect;

static inline HTMLRect *impl_from_IHTMLRect(IHTMLRect *iface)
{
    return CONTAINING_RECORD(iface, HTMLRect, IHTMLRect_iface);
}

static HRESULT WINAPI HTMLRect_QueryInterface(IHTMLRect *iface, REFIID riid, void **ppv)
{
    HTMLRect *This = impl_from_IHTMLRect(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_mshtml_guid(riid), ppv);

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        *ppv = &This->IHTMLRect_iface;
    }else if(IsEqualGUID(&IID_IHTMLRect, riid)) {
        *ppv = &This->IHTMLRect_iface;
    }else if(dispex_query_interface(&This->dispex, riid, ppv)) {
        return *ppv ? S_OK : E_NOINTERFACE;
    }else {
        FIXME("(%p)->(%s %p)\n", This, debugstr_mshtml_guid(riid), ppv);
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI HTMLRect_AddRef(IHTMLRect *iface)
{
    HTMLRect *This = impl_from_IHTMLRect(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    return ref;
}

static ULONG WINAPI HTMLRect_Release(IHTMLRect *iface)
{
    HTMLRect *This = impl_from_IHTMLRect(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    if(!ref) {
        if(This->nsrect)
            nsIDOMClientRect_Release(This->nsrect);
        heap_free(This);
    }

    return ref;
}

static HRESULT WINAPI HTMLRect_GetTypeInfoCount(IHTMLRect *iface, UINT *pctinfo)
{
    HTMLRect *This = impl_from_IHTMLRect(iface);
    FIXME("(%p)->(%p)\n", This, pctinfo);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLRect_GetTypeInfo(IHTMLRect *iface, UINT iTInfo,
        LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLRect *This = impl_from_IHTMLRect(iface);

    return IDispatchEx_GetTypeInfo(&This->dispex.IDispatchEx_iface, iTInfo, lcid, ppTInfo);
}

static HRESULT WINAPI HTMLRect_GetIDsOfNames(IHTMLRect *iface, REFIID riid,
        LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{
    HTMLRect *This = impl_from_IHTMLRect(iface);

    return IDispatchEx_GetIDsOfNames(&This->dispex.IDispatchEx_iface, riid, rgszNames, cNames,
            lcid, rgDispId);
}

static HRESULT WINAPI HTMLRect_Invoke(IHTMLRect *iface, DISPID dispIdMember,
        REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
        VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLRect *This = impl_from_IHTMLRect(iface);

    return IDispatchEx_Invoke(&This->dispex.IDispatchEx_iface, dispIdMember, riid, lcid, wFlags,
            pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI HTMLRect_put_left(IHTMLRect *iface, LONG v)
{
    HTMLRect *This = impl_from_IHTMLRect(iface);
    FIXME("(%p)->(%d)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLRect_get_left(IHTMLRect *iface, LONG *p)
{
    HTMLRect *This = impl_from_IHTMLRect(iface);
    float left;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsres = nsIDOMClientRect_GetLeft(This->nsrect, &left);
    if(NS_FAILED(nsres)) {
        ERR("GetLeft failed: %08x\n", nsres);
        return E_FAIL;
    }

    *p = floor(left+0.5);
    return S_OK;
}

static HRESULT WINAPI HTMLRect_put_top(IHTMLRect *iface, LONG v)
{
    HTMLRect *This = impl_from_IHTMLRect(iface);
    FIXME("(%p)->(%d)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLRect_get_top(IHTMLRect *iface, LONG *p)
{
    HTMLRect *This = impl_from_IHTMLRect(iface);
    float top;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsres = nsIDOMClientRect_GetTop(This->nsrect, &top);
    if(NS_FAILED(nsres)) {
        ERR("GetTop failed: %08x\n", nsres);
        return E_FAIL;
    }

    *p = floor(top+0.5);
    return S_OK;
}

static HRESULT WINAPI HTMLRect_put_right(IHTMLRect *iface, LONG v)
{
    HTMLRect *This = impl_from_IHTMLRect(iface);
    FIXME("(%p)->(%d)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLRect_get_right(IHTMLRect *iface, LONG *p)
{
    HTMLRect *This = impl_from_IHTMLRect(iface);
    float right;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsres = nsIDOMClientRect_GetRight(This->nsrect, &right);
    if(NS_FAILED(nsres)) {
        ERR("GetRight failed: %08x\n", nsres);
        return E_FAIL;
    }

    *p = floor(right+0.5);
    return S_OK;
}

static HRESULT WINAPI HTMLRect_put_bottom(IHTMLRect *iface, LONG v)
{
    HTMLRect *This = impl_from_IHTMLRect(iface);
    FIXME("(%p)->(%d)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLRect_get_bottom(IHTMLRect *iface, LONG *p)
{
    HTMLRect *This = impl_from_IHTMLRect(iface);
    float bottom;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsres = nsIDOMClientRect_GetBottom(This->nsrect, &bottom);
    if(NS_FAILED(nsres)) {
        ERR("GetBottom failed: %08x\n", nsres);
        return E_FAIL;
    }

    *p = floor(bottom+0.5);
    return S_OK;
}

static const IHTMLRectVtbl HTMLRectVtbl = {
    HTMLRect_QueryInterface,
    HTMLRect_AddRef,
    HTMLRect_Release,
    HTMLRect_GetTypeInfoCount,
    HTMLRect_GetTypeInfo,
    HTMLRect_GetIDsOfNames,
    HTMLRect_Invoke,
    HTMLRect_put_left,
    HTMLRect_get_left,
    HTMLRect_put_top,
    HTMLRect_get_top,
    HTMLRect_put_right,
    HTMLRect_get_right,
    HTMLRect_put_bottom,
    HTMLRect_get_bottom
};

static const tid_t HTMLRect_iface_tids[] = {
    IHTMLRect_tid,
    0
};
static dispex_static_data_t HTMLRect_dispex = {
    NULL,
    IHTMLRect_tid,
    NULL,
    HTMLRect_iface_tids
};

static HRESULT create_html_rect(nsIDOMClientRect *nsrect, IHTMLRect **ret)
{
    HTMLRect *rect;

    rect = heap_alloc_zero(sizeof(HTMLRect));
    if(!rect)
        return E_OUTOFMEMORY;

    rect->IHTMLRect_iface.lpVtbl = &HTMLRectVtbl;
    rect->ref = 1;

    init_dispex(&rect->dispex, (IUnknown*)&rect->IHTMLRect_iface, &HTMLRect_dispex);

    nsIDOMClientRect_AddRef(nsrect);
    rect->nsrect = nsrect;

    *ret = &rect->IHTMLRect_iface;
    return S_OK;
}

static inline HTMLElement *impl_from_IHTMLElement2(IHTMLElement2 *iface)
{
    return CONTAINING_RECORD(iface, HTMLElement, IHTMLElement2_iface);
}

static HRESULT WINAPI HTMLElement2_QueryInterface(IHTMLElement2 *iface,
                                                  REFIID riid, void **ppv)
{
    HTMLElement *This = impl_from_IHTMLElement2(iface);
    return IHTMLElement_QueryInterface(&This->IHTMLElement_iface, riid, ppv);
}

static ULONG WINAPI HTMLElement2_AddRef(IHTMLElement2 *iface)
{
    HTMLElement *This = impl_from_IHTMLElement2(iface);
    return IHTMLElement_AddRef(&This->IHTMLElement_iface);
}

static ULONG WINAPI HTMLElement2_Release(IHTMLElement2 *iface)
{
    HTMLElement *This = impl_from_IHTMLElement2(iface);
    return IHTMLElement_Release(&This->IHTMLElement_iface);
}

static HRESULT WINAPI HTMLElement2_GetTypeInfoCount(IHTMLElement2 *iface, UINT *pctinfo)
{
    HTMLElement *This = impl_from_IHTMLElement2(iface);
    return IDispatchEx_GetTypeInfoCount(&This->node.dispex.IDispatchEx_iface, pctinfo);
}

static HRESULT WINAPI HTMLElement2_GetTypeInfo(IHTMLElement2 *iface, UINT iTInfo,
                                               LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLElement *This = impl_from_IHTMLElement2(iface);
    return IDispatchEx_GetTypeInfo(&This->node.dispex.IDispatchEx_iface, iTInfo, lcid, ppTInfo);
}

static HRESULT WINAPI HTMLElement2_GetIDsOfNames(IHTMLElement2 *iface, REFIID riid,
                                                LPOLESTR *rgszNames, UINT cNames,
                                                LCID lcid, DISPID *rgDispId)
{
    HTMLElement *This = impl_from_IHTMLElement2(iface);
    return IDispatchEx_GetIDsOfNames(&This->node.dispex.IDispatchEx_iface, riid, rgszNames, cNames,
            lcid, rgDispId);
}

static HRESULT WINAPI HTMLElement2_Invoke(IHTMLElement2 *iface, DISPID dispIdMember,
                            REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
                            VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLElement *This = impl_from_IHTMLElement2(iface);
    return IDispatchEx_Invoke(&This->node.dispex.IDispatchEx_iface, dispIdMember, riid, lcid,
            wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI HTMLElement2_get_scopeName(IHTMLElement2 *iface, BSTR *p)
{
    HTMLElement *This = impl_from_IHTMLElement2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement2_setCapture(IHTMLElement2 *iface, VARIANT_BOOL containerCapture)
{
    HTMLElement *This = impl_from_IHTMLElement2(iface);
    FIXME("(%p)->(%x)\n", This, containerCapture);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement2_releaseCapture(IHTMLElement2 *iface)
{
    HTMLElement *This = impl_from_IHTMLElement2(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement2_put_onlosecapture(IHTMLElement2 *iface, VARIANT v)
{
    HTMLElement *This = impl_from_IHTMLElement2(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement2_get_onlosecapture(IHTMLElement2 *iface, VARIANT *p)
{
    HTMLElement *This = impl_from_IHTMLElement2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement2_componentFromPoint(IHTMLElement2 *iface,
                                                      LONG x, LONG y, BSTR *component)
{
    HTMLElement *This = impl_from_IHTMLElement2(iface);
    FIXME("(%p)->(%d %d %p)\n", This, x, y, component);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement2_doScroll(IHTMLElement2 *iface, VARIANT component)
{
    HTMLElement *This = impl_from_IHTMLElement2(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&component));

    if(!This->node.doc->content_ready
       || !This->node.doc->basedoc.doc_obj->in_place_active)
        return E_PENDING;

    WARN("stub\n");
    return S_OK;
}

static HRESULT WINAPI HTMLElement2_put_onscroll(IHTMLElement2 *iface, VARIANT v)
{
    HTMLElement *This = impl_from_IHTMLElement2(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement2_get_onscroll(IHTMLElement2 *iface, VARIANT *p)
{
    HTMLElement *This = impl_from_IHTMLElement2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement2_put_ondrag(IHTMLElement2 *iface, VARIANT v)
{
    HTMLElement *This = impl_from_IHTMLElement2(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_node_event(&This->node, EVENTID_DRAG, &v);
}

static HRESULT WINAPI HTMLElement2_get_ondrag(IHTMLElement2 *iface, VARIANT *p)
{
    HTMLElement *This = impl_from_IHTMLElement2(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_node_event(&This->node, EVENTID_DRAG, p);
}

static HRESULT WINAPI HTMLElement2_put_ondragend(IHTMLElement2 *iface, VARIANT v)
{
    HTMLElement *This = impl_from_IHTMLElement2(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement2_get_ondragend(IHTMLElement2 *iface, VARIANT *p)
{
    HTMLElement *This = impl_from_IHTMLElement2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement2_put_ondragenter(IHTMLElement2 *iface, VARIANT v)
{
    HTMLElement *This = impl_from_IHTMLElement2(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement2_get_ondragenter(IHTMLElement2 *iface, VARIANT *p)
{
    HTMLElement *This = impl_from_IHTMLElement2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement2_put_ondragover(IHTMLElement2 *iface, VARIANT v)
{
    HTMLElement *This = impl_from_IHTMLElement2(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement2_get_ondragover(IHTMLElement2 *iface, VARIANT *p)
{
    HTMLElement *This = impl_from_IHTMLElement2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement2_put_ondragleave(IHTMLElement2 *iface, VARIANT v)
{
    HTMLElement *This = impl_from_IHTMLElement2(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement2_get_ondragleave(IHTMLElement2 *iface, VARIANT *p)
{
    HTMLElement *This = impl_from_IHTMLElement2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement2_put_ondrop(IHTMLElement2 *iface, VARIANT v)
{
    HTMLElement *This = impl_from_IHTMLElement2(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement2_get_ondrop(IHTMLElement2 *iface, VARIANT *p)
{
    HTMLElement *This = impl_from_IHTMLElement2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement2_put_onbeforecut(IHTMLElement2 *iface, VARIANT v)
{
    HTMLElement *This = impl_from_IHTMLElement2(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement2_get_onbeforecut(IHTMLElement2 *iface, VARIANT *p)
{
    HTMLElement *This = impl_from_IHTMLElement2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement2_put_oncut(IHTMLElement2 *iface, VARIANT v)
{
    HTMLElement *This = impl_from_IHTMLElement2(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement2_get_oncut(IHTMLElement2 *iface, VARIANT *p)
{
    HTMLElement *This = impl_from_IHTMLElement2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement2_put_onbeforecopy(IHTMLElement2 *iface, VARIANT v)
{
    HTMLElement *This = impl_from_IHTMLElement2(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement2_get_onbeforecopy(IHTMLElement2 *iface, VARIANT *p)
{
    HTMLElement *This = impl_from_IHTMLElement2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement2_put_oncopy(IHTMLElement2 *iface, VARIANT v)
{
    HTMLElement *This = impl_from_IHTMLElement2(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement2_get_oncopy(IHTMLElement2 *iface, VARIANT *p)
{
    HTMLElement *This = impl_from_IHTMLElement2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement2_put_onbeforepaste(IHTMLElement2 *iface, VARIANT v)
{
    HTMLElement *This = impl_from_IHTMLElement2(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement2_get_onbeforepaste(IHTMLElement2 *iface, VARIANT *p)
{
    HTMLElement *This = impl_from_IHTMLElement2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement2_put_onpaste(IHTMLElement2 *iface, VARIANT v)
{
    HTMLElement *This = impl_from_IHTMLElement2(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_node_event(&This->node, EVENTID_PASTE, &v);
}

static HRESULT WINAPI HTMLElement2_get_onpaste(IHTMLElement2 *iface, VARIANT *p)
{
    HTMLElement *This = impl_from_IHTMLElement2(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_node_event(&This->node, EVENTID_PASTE, p);
}

static HRESULT WINAPI HTMLElement2_get_currentStyle(IHTMLElement2 *iface, IHTMLCurrentStyle **p)
{
    HTMLElement *This = impl_from_IHTMLElement2(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return HTMLCurrentStyle_Create(This, p);
}

static HRESULT WINAPI HTMLElement2_put_onpropertychange(IHTMLElement2 *iface, VARIANT v)
{
    HTMLElement *This = impl_from_IHTMLElement2(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement2_get_onpropertychange(IHTMLElement2 *iface, VARIANT *p)
{
    HTMLElement *This = impl_from_IHTMLElement2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement2_getClientRects(IHTMLElement2 *iface, IHTMLRectCollection **pRectCol)
{
    HTMLElement *This = impl_from_IHTMLElement2(iface);
    FIXME("(%p)->(%p)\n", This, pRectCol);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement2_getBoundingClientRect(IHTMLElement2 *iface, IHTMLRect **pRect)
{
    HTMLElement *This = impl_from_IHTMLElement2(iface);
    nsIDOMClientRect *nsrect;
    nsresult nsres;
    HRESULT hres;

    TRACE("(%p)->(%p)\n", This, pRect);

    nsres = nsIDOMHTMLElement_GetBoundingClientRect(This->nselem, &nsrect);
    if(NS_FAILED(nsres) || !nsrect) {
        ERR("GetBoindingClientRect failed: %08x\n", nsres);
        return E_FAIL;
    }

    hres = create_html_rect(nsrect, pRect);

    nsIDOMClientRect_Release(nsrect);
    return hres;
}

static HRESULT WINAPI HTMLElement2_setExpression(IHTMLElement2 *iface, BSTR propname,
                                                 BSTR expression, BSTR language)
{
    HTMLElement *This = impl_from_IHTMLElement2(iface);
    FIXME("(%p)->(%s %s %s)\n", This, debugstr_w(propname), debugstr_w(expression),
          debugstr_w(language));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement2_getExpression(IHTMLElement2 *iface, BSTR propname,
                                                 VARIANT *expression)
{
    HTMLElement *This = impl_from_IHTMLElement2(iface);
    FIXME("(%p)->(%s %p)\n", This, debugstr_w(propname), expression);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement2_removeExpression(IHTMLElement2 *iface, BSTR propname,
                                                    VARIANT_BOOL *pfSuccess)
{
    HTMLElement *This = impl_from_IHTMLElement2(iface);
    FIXME("(%p)->(%s %p)\n", This, debugstr_w(propname), pfSuccess);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement2_put_tabIndex(IHTMLElement2 *iface, short v)
{
    HTMLElement *This = impl_from_IHTMLElement2(iface);
    nsresult nsres;

    TRACE("(%p)->(%d)\n", This, v);

    nsres = nsIDOMHTMLElement_SetTabIndex(This->nselem, v);
    if(NS_FAILED(nsres))
        ERR("GetTabIndex failed: %08x\n", nsres);

    return S_OK;
}

static HRESULT WINAPI HTMLElement2_get_tabIndex(IHTMLElement2 *iface, short *p)
{
    HTMLElement *This = impl_from_IHTMLElement2(iface);
    LONG index;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsres = nsIDOMHTMLElement_GetTabIndex(This->nselem, &index);
    if(NS_FAILED(nsres)) {
        ERR("GetTabIndex failed: %08x\n", nsres);
        return E_FAIL;
    }

    *p = index;
    return S_OK;
}

static HRESULT WINAPI HTMLElement2_focus(IHTMLElement2 *iface)
{
    HTMLElement *This = impl_from_IHTMLElement2(iface);
    nsresult nsres;

    TRACE("(%p)\n", This);

    nsres = nsIDOMHTMLElement_Focus(This->nselem);
    if(NS_FAILED(nsres))
        ERR("Focus failed: %08x\n", nsres);

    return S_OK;
}

static HRESULT WINAPI HTMLElement2_put_accessKey(IHTMLElement2 *iface, BSTR v)
{
    HTMLElement *This = impl_from_IHTMLElement2(iface);
    VARIANT var;

    static WCHAR accessKeyW[] = {'a','c','c','e','s','s','K','e','y',0};

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    V_VT(&var) = VT_BSTR;
    V_BSTR(&var) = v;
    return IHTMLElement_setAttribute(&This->IHTMLElement_iface, accessKeyW, var, 0);
}

static HRESULT WINAPI HTMLElement2_get_accessKey(IHTMLElement2 *iface, BSTR *p)
{
    HTMLElement *This = impl_from_IHTMLElement2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement2_put_onblur(IHTMLElement2 *iface, VARIANT v)
{
    HTMLElement *This = impl_from_IHTMLElement2(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_node_event(&This->node, EVENTID_BLUR, &v);
}

static HRESULT WINAPI HTMLElement2_get_onblur(IHTMLElement2 *iface, VARIANT *p)
{
    HTMLElement *This = impl_from_IHTMLElement2(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_node_event(&This->node, EVENTID_BLUR, p);
}

static HRESULT WINAPI HTMLElement2_put_onfocus(IHTMLElement2 *iface, VARIANT v)
{
    HTMLElement *This = impl_from_IHTMLElement2(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_node_event(&This->node, EVENTID_FOCUS, &v);
}

static HRESULT WINAPI HTMLElement2_get_onfocus(IHTMLElement2 *iface, VARIANT *p)
{
    HTMLElement *This = impl_from_IHTMLElement2(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_node_event(&This->node, EVENTID_FOCUS, p);
}

static HRESULT WINAPI HTMLElement2_put_onresize(IHTMLElement2 *iface, VARIANT v)
{
    HTMLElement *This = impl_from_IHTMLElement2(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement2_get_onresize(IHTMLElement2 *iface, VARIANT *p)
{
    HTMLElement *This = impl_from_IHTMLElement2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement2_blur(IHTMLElement2 *iface)
{
    HTMLElement *This = impl_from_IHTMLElement2(iface);
    nsresult nsres;

    TRACE("(%p)\n", This);

    nsres = nsIDOMHTMLElement_Blur(This->nselem);
    if(NS_FAILED(nsres)) {
        ERR("Blur failed: %08x\n", nsres);
        return E_FAIL;
    }

    return S_OK;
}

static HRESULT WINAPI HTMLElement2_addFilter(IHTMLElement2 *iface, IUnknown *pUnk)
{
    HTMLElement *This = impl_from_IHTMLElement2(iface);
    FIXME("(%p)->(%p)\n", This, pUnk);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement2_removeFilter(IHTMLElement2 *iface, IUnknown *pUnk)
{
    HTMLElement *This = impl_from_IHTMLElement2(iface);
    FIXME("(%p)->(%p)\n", This, pUnk);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement2_get_clientHeight(IHTMLElement2 *iface, LONG *p)
{
    HTMLElement *This = impl_from_IHTMLElement2(iface);
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsres = nsIDOMHTMLElement_GetClientHeight(This->nselem, p);
    assert(nsres == NS_OK);
    return S_OK;
}

static HRESULT WINAPI HTMLElement2_get_clientWidth(IHTMLElement2 *iface, LONG *p)
{
    HTMLElement *This = impl_from_IHTMLElement2(iface);
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsres = nsIDOMHTMLElement_GetClientWidth(This->nselem, p);
    assert(nsres == NS_OK);
    return S_OK;
}

static HRESULT WINAPI HTMLElement2_get_clientTop(IHTMLElement2 *iface, LONG *p)
{
    HTMLElement *This = impl_from_IHTMLElement2(iface);
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsres = nsIDOMHTMLElement_GetClientTop(This->nselem, p);
    assert(nsres == NS_OK);

    TRACE("*p = %d\n", *p);
    return S_OK;
}

static HRESULT WINAPI HTMLElement2_get_clientLeft(IHTMLElement2 *iface, LONG *p)
{
    HTMLElement *This = impl_from_IHTMLElement2(iface);
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsres = nsIDOMHTMLElement_GetClientLeft(This->nselem, p);
    assert(nsres == NS_OK);

    TRACE("*p = %d\n", *p);
    return S_OK;
}

static HRESULT WINAPI HTMLElement2_attachEvent(IHTMLElement2 *iface, BSTR event,
                                               IDispatch *pDisp, VARIANT_BOOL *pfResult)
{
    HTMLElement *This = impl_from_IHTMLElement2(iface);

    TRACE("(%p)->(%s %p %p)\n", This, debugstr_w(event), pDisp, pfResult);

    return attach_event(get_node_event_target(&This->node), &This->node.doc->basedoc, event, pDisp, pfResult);
}

static HRESULT WINAPI HTMLElement2_detachEvent(IHTMLElement2 *iface, BSTR event, IDispatch *pDisp)
{
    HTMLElement *This = impl_from_IHTMLElement2(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_w(event), pDisp);

    return detach_event(*get_node_event_target(&This->node), &This->node.doc->basedoc, event, pDisp);
}

static HRESULT WINAPI HTMLElement2_get_readyState(IHTMLElement2 *iface, VARIANT *p)
{
    HTMLElement *This = impl_from_IHTMLElement2(iface);
    BSTR str;

    TRACE("(%p)->(%p)\n", This, p);

    if(This->node.vtbl->get_readystate) {
        HRESULT hres;

        hres = This->node.vtbl->get_readystate(&This->node, &str);
        if(FAILED(hres))
            return hres;
    }else {
        static const WCHAR completeW[] = {'c','o','m','p','l','e','t','e',0};

        str = SysAllocString(completeW);
        if(!str)
            return E_OUTOFMEMORY;
    }

    V_VT(p) = VT_BSTR;
    V_BSTR(p) = str;
    return S_OK;
}

static HRESULT WINAPI HTMLElement2_put_onreadystatechange(IHTMLElement2 *iface, VARIANT v)
{
    HTMLElement *This = impl_from_IHTMLElement2(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_node_event(&This->node, EVENTID_READYSTATECHANGE, &v);
}

static HRESULT WINAPI HTMLElement2_get_onreadystatechange(IHTMLElement2 *iface, VARIANT *p)
{
    HTMLElement *This = impl_from_IHTMLElement2(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_node_event(&This->node, EVENTID_READYSTATECHANGE, p);
}

static HRESULT WINAPI HTMLElement2_put_onrowsdelete(IHTMLElement2 *iface, VARIANT v)
{
    HTMLElement *This = impl_from_IHTMLElement2(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement2_get_onrowsdelete(IHTMLElement2 *iface, VARIANT *p)
{
    HTMLElement *This = impl_from_IHTMLElement2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement2_put_onrowsinserted(IHTMLElement2 *iface, VARIANT v)
{
    HTMLElement *This = impl_from_IHTMLElement2(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement2_get_onrowsinserted(IHTMLElement2 *iface, VARIANT *p)
{
    HTMLElement *This = impl_from_IHTMLElement2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement2_put_oncellchange(IHTMLElement2 *iface, VARIANT v)
{
    HTMLElement *This = impl_from_IHTMLElement2(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement2_get_oncellchange(IHTMLElement2 *iface, VARIANT *p)
{
    HTMLElement *This = impl_from_IHTMLElement2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement2_put_dir(IHTMLElement2 *iface, BSTR v)
{
    HTMLElement *This = impl_from_IHTMLElement2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement2_get_dir(IHTMLElement2 *iface, BSTR *p)
{
    HTMLElement *This = impl_from_IHTMLElement2(iface);
    nsAString dir_str;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    if(!This->nselem) {
        *p = NULL;
        return S_OK;
    }

    nsAString_Init(&dir_str, NULL);
    nsres = nsIDOMHTMLElement_GetDir(This->nselem, &dir_str);
    return return_nsstr(nsres, &dir_str, p);
}

static HRESULT WINAPI HTMLElement2_createControlRange(IHTMLElement2 *iface, IDispatch **range)
{
    HTMLElement *This = impl_from_IHTMLElement2(iface);
    FIXME("(%p)->(%p)\n", This, range);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement2_get_scrollHeight(IHTMLElement2 *iface, LONG *p)
{
    HTMLElement *This = impl_from_IHTMLElement2(iface);
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsres = nsIDOMHTMLElement_GetScrollHeight(This->nselem, p);
    assert(nsres == NS_OK);

    TRACE("*p = %d\n", *p);
    return S_OK;
}

static HRESULT WINAPI HTMLElement2_get_scrollWidth(IHTMLElement2 *iface, LONG *p)
{
    HTMLElement *This = impl_from_IHTMLElement2(iface);
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsres = nsIDOMHTMLElement_GetScrollWidth(This->nselem, p);
    assert(nsres == NS_OK);

    TRACE("*p = %d\n", *p);
    return S_OK;
}

static HRESULT WINAPI HTMLElement2_put_scrollTop(IHTMLElement2 *iface, LONG v)
{
    HTMLElement *This = impl_from_IHTMLElement2(iface);

    TRACE("(%p)->(%d)\n", This, v);

    if(!This->nselem) {
        FIXME("NULL nselem\n");
        return E_NOTIMPL;
    }

    nsIDOMHTMLElement_SetScrollTop(This->nselem, v);
    return S_OK;
}

static HRESULT WINAPI HTMLElement2_get_scrollTop(IHTMLElement2 *iface, LONG *p)
{
    HTMLElement *This = impl_from_IHTMLElement2(iface);
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsres = nsIDOMHTMLElement_GetScrollTop(This->nselem, p);
    assert(nsres == NS_OK);

    TRACE("*p = %d\n", *p);
    return S_OK;
}

static HRESULT WINAPI HTMLElement2_put_scrollLeft(IHTMLElement2 *iface, LONG v)
{
    HTMLElement *This = impl_from_IHTMLElement2(iface);

    TRACE("(%p)->(%d)\n", This, v);

    if(!This->nselem) {
        FIXME("NULL nselem\n");
        return E_NOTIMPL;
    }

    nsIDOMHTMLElement_SetScrollLeft(This->nselem, v);
    return S_OK;
}

static HRESULT WINAPI HTMLElement2_get_scrollLeft(IHTMLElement2 *iface, LONG *p)
{
    HTMLElement *This = impl_from_IHTMLElement2(iface);
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    if(!p)
        return E_INVALIDARG;

    if(!This->nselem)
    {
        FIXME("NULL nselem\n");
        return E_NOTIMPL;
    }

    nsres = nsIDOMHTMLElement_GetScrollLeft(This->nselem, p);
    assert(nsres == NS_OK);

    TRACE("*p = %d\n", *p);
    return S_OK;
}

static HRESULT WINAPI HTMLElement2_clearAttributes(IHTMLElement2 *iface)
{
    HTMLElement *This = impl_from_IHTMLElement2(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement2_mergeAttributes(IHTMLElement2 *iface, IHTMLElement *mergeThis)
{
    HTMLElement *This = impl_from_IHTMLElement2(iface);
    FIXME("(%p)->(%p)\n", This, mergeThis);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement2_put_oncontextmenu(IHTMLElement2 *iface, VARIANT v)
{
    HTMLElement *This = impl_from_IHTMLElement2(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement2_get_oncontextmenu(IHTMLElement2 *iface, VARIANT *p)
{
    HTMLElement *This = impl_from_IHTMLElement2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement2_insertAdjacentElement(IHTMLElement2 *iface, BSTR where,
        IHTMLElement *insertedElement, IHTMLElement **inserted)
{
    HTMLElement *This = impl_from_IHTMLElement2(iface);
    HTMLDOMNode *ret_node;
    HTMLElement *elem;
    HRESULT hres;

    TRACE("(%p)->(%s %p %p)\n", This, debugstr_w(where), insertedElement, inserted);

    elem = unsafe_impl_from_IHTMLElement(insertedElement);
    if(!elem)
        return E_INVALIDARG;

    hres = insert_adjacent_node(This, where, elem->node.nsnode, &ret_node);
    if(FAILED(hres))
        return hres;

    hres = IHTMLDOMNode_QueryInterface(&ret_node->IHTMLDOMNode_iface, &IID_IHTMLElement, (void**)inserted);
    IHTMLDOMNode_Release(&ret_node->IHTMLDOMNode_iface);
    return hres;
}

static HRESULT WINAPI HTMLElement2_applyElement(IHTMLElement2 *iface, IHTMLElement *apply,
                                                BSTR where, IHTMLElement **applied)
{
    HTMLElement *This = impl_from_IHTMLElement2(iface);
    FIXME("(%p)->(%p %s %p)\n", This, apply, debugstr_w(where), applied);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement2_getAdjacentText(IHTMLElement2 *iface, BSTR where, BSTR *text)
{
    HTMLElement *This = impl_from_IHTMLElement2(iface);
    FIXME("(%p)->(%s %p)\n", This, debugstr_w(where), text);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement2_replaceAdjacentText(IHTMLElement2 *iface, BSTR where,
                                                       BSTR newText, BSTR *oldText)
{
    HTMLElement *This = impl_from_IHTMLElement2(iface);
    FIXME("(%p)->(%s %s %p)\n", This, debugstr_w(where), debugstr_w(newText), oldText);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement2_get_canHandleChildren(IHTMLElement2 *iface, VARIANT_BOOL *p)
{
    HTMLElement *This = impl_from_IHTMLElement2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement2_addBehavior(IHTMLElement2 *iface, BSTR bstrUrl,
                                               VARIANT *pvarFactory, LONG *pCookie)
{
    HTMLElement *This = impl_from_IHTMLElement2(iface);
    FIXME("(%p)->(%s %p %p)\n", This, debugstr_w(bstrUrl), pvarFactory, pCookie);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement2_removeBehavior(IHTMLElement2 *iface, LONG cookie,
                                                  VARIANT_BOOL *pfResult)
{
    HTMLElement *This = impl_from_IHTMLElement2(iface);
    FIXME("(%p)->(%d %p)\n", This, cookie, pfResult);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement2_get_runtimeStyle(IHTMLElement2 *iface, IHTMLStyle **p)
{
    HTMLElement *This = impl_from_IHTMLElement2(iface);

    FIXME("(%p)->(%p): hack\n", This, p);

    /* We can't implement correct behavior on top of Gecko (although we could
       try a bit harder). Making runtimeStyle behave like regular style is
       enough for most use cases. */
    if(!This->runtime_style) {
        HRESULT hres;

        hres = HTMLStyle_Create(This, &This->runtime_style);
        if(FAILED(hres))
            return hres;
    }

    *p = &This->runtime_style->IHTMLStyle_iface;
    IHTMLStyle_AddRef(*p);
    return S_OK;
}

static HRESULT WINAPI HTMLElement2_get_behaviorUrns(IHTMLElement2 *iface, IDispatch **p)
{
    HTMLElement *This = impl_from_IHTMLElement2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement2_put_tagUrn(IHTMLElement2 *iface, BSTR v)
{
    HTMLElement *This = impl_from_IHTMLElement2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement2_get_tagUrn(IHTMLElement2 *iface, BSTR *p)
{
    HTMLElement *This = impl_from_IHTMLElement2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement2_put_onbeforeeditfocus(IHTMLElement2 *iface, VARIANT vv)
{
    HTMLElement *This = impl_from_IHTMLElement2(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement2_get_onbeforeeditfocus(IHTMLElement2 *iface, VARIANT *p)
{
    HTMLElement *This = impl_from_IHTMLElement2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement2_get_readyStateValue(IHTMLElement2 *iface, LONG *p)
{
    HTMLElement *This = impl_from_IHTMLElement2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement2_getElementsByTagName(IHTMLElement2 *iface, BSTR v,
                                                       IHTMLElementCollection **pelColl)
{
    HTMLElement *This = impl_from_IHTMLElement2(iface);
    nsIDOMHTMLCollection *nscol;
    nsAString tag_str;
    nsresult nsres;

    TRACE("(%p)->(%s %p)\n", This, debugstr_w(v), pelColl);

    nsAString_InitDepend(&tag_str, v);
    nsres = nsIDOMHTMLElement_GetElementsByTagName(This->nselem, &tag_str, &nscol);
    nsAString_Finish(&tag_str);
    if(NS_FAILED(nsres)) {
        ERR("GetElementByTagName failed: %08x\n", nsres);
        return E_FAIL;
    }

    *pelColl = create_collection_from_htmlcol(This->node.doc, nscol);
    nsIDOMHTMLCollection_Release(nscol);
    return S_OK;
}

static const IHTMLElement2Vtbl HTMLElement2Vtbl = {
    HTMLElement2_QueryInterface,
    HTMLElement2_AddRef,
    HTMLElement2_Release,
    HTMLElement2_GetTypeInfoCount,
    HTMLElement2_GetTypeInfo,
    HTMLElement2_GetIDsOfNames,
    HTMLElement2_Invoke,
    HTMLElement2_get_scopeName,
    HTMLElement2_setCapture,
    HTMLElement2_releaseCapture,
    HTMLElement2_put_onlosecapture,
    HTMLElement2_get_onlosecapture,
    HTMLElement2_componentFromPoint,
    HTMLElement2_doScroll,
    HTMLElement2_put_onscroll,
    HTMLElement2_get_onscroll,
    HTMLElement2_put_ondrag,
    HTMLElement2_get_ondrag,
    HTMLElement2_put_ondragend,
    HTMLElement2_get_ondragend,
    HTMLElement2_put_ondragenter,
    HTMLElement2_get_ondragenter,
    HTMLElement2_put_ondragover,
    HTMLElement2_get_ondragover,
    HTMLElement2_put_ondragleave,
    HTMLElement2_get_ondragleave,
    HTMLElement2_put_ondrop,
    HTMLElement2_get_ondrop,
    HTMLElement2_put_onbeforecut,
    HTMLElement2_get_onbeforecut,
    HTMLElement2_put_oncut,
    HTMLElement2_get_oncut,
    HTMLElement2_put_onbeforecopy,
    HTMLElement2_get_onbeforecopy,
    HTMLElement2_put_oncopy,
    HTMLElement2_get_oncopy,
    HTMLElement2_put_onbeforepaste,
    HTMLElement2_get_onbeforepaste,
    HTMLElement2_put_onpaste,
    HTMLElement2_get_onpaste,
    HTMLElement2_get_currentStyle,
    HTMLElement2_put_onpropertychange,
    HTMLElement2_get_onpropertychange,
    HTMLElement2_getClientRects,
    HTMLElement2_getBoundingClientRect,
    HTMLElement2_setExpression,
    HTMLElement2_getExpression,
    HTMLElement2_removeExpression,
    HTMLElement2_put_tabIndex,
    HTMLElement2_get_tabIndex,
    HTMLElement2_focus,
    HTMLElement2_put_accessKey,
    HTMLElement2_get_accessKey,
    HTMLElement2_put_onblur,
    HTMLElement2_get_onblur,
    HTMLElement2_put_onfocus,
    HTMLElement2_get_onfocus,
    HTMLElement2_put_onresize,
    HTMLElement2_get_onresize,
    HTMLElement2_blur,
    HTMLElement2_addFilter,
    HTMLElement2_removeFilter,
    HTMLElement2_get_clientHeight,
    HTMLElement2_get_clientWidth,
    HTMLElement2_get_clientTop,
    HTMLElement2_get_clientLeft,
    HTMLElement2_attachEvent,
    HTMLElement2_detachEvent,
    HTMLElement2_get_readyState,
    HTMLElement2_put_onreadystatechange,
    HTMLElement2_get_onreadystatechange,
    HTMLElement2_put_onrowsdelete,
    HTMLElement2_get_onrowsdelete,
    HTMLElement2_put_onrowsinserted,
    HTMLElement2_get_onrowsinserted,
    HTMLElement2_put_oncellchange,
    HTMLElement2_get_oncellchange,
    HTMLElement2_put_dir,
    HTMLElement2_get_dir,
    HTMLElement2_createControlRange,
    HTMLElement2_get_scrollHeight,
    HTMLElement2_get_scrollWidth,
    HTMLElement2_put_scrollTop,
    HTMLElement2_get_scrollTop,
    HTMLElement2_put_scrollLeft,
    HTMLElement2_get_scrollLeft,
    HTMLElement2_clearAttributes,
    HTMLElement2_mergeAttributes,
    HTMLElement2_put_oncontextmenu,
    HTMLElement2_get_oncontextmenu,
    HTMLElement2_insertAdjacentElement,
    HTMLElement2_applyElement,
    HTMLElement2_getAdjacentText,
    HTMLElement2_replaceAdjacentText,
    HTMLElement2_get_canHandleChildren,
    HTMLElement2_addBehavior,
    HTMLElement2_removeBehavior,
    HTMLElement2_get_runtimeStyle,
    HTMLElement2_get_behaviorUrns,
    HTMLElement2_put_tagUrn,
    HTMLElement2_get_tagUrn,
    HTMLElement2_put_onbeforeeditfocus,
    HTMLElement2_get_onbeforeeditfocus,
    HTMLElement2_get_readyStateValue,
    HTMLElement2_getElementsByTagName,
};

void HTMLElement2_Init(HTMLElement *This)
{
    This->IHTMLElement2_iface.lpVtbl = &HTMLElement2Vtbl;
}
