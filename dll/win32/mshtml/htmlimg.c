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
#include "htmlevent.h"

WINE_DEFAULT_DEBUG_CHANNEL(mshtml);

typedef struct {
    HTMLElement element;

    const IHTMLImgElementVtbl *lpHTMLImgElementVtbl;

    nsIDOMHTMLImageElement *nsimg;
} HTMLImgElement;

#define HTMLIMG(x)  ((IHTMLImgElement*)  &(x)->lpHTMLImgElementVtbl)

#define HTMLIMG_THIS(iface) DEFINE_THIS(HTMLImgElement, HTMLImgElement, iface)

static HRESULT WINAPI HTMLImgElement_QueryInterface(IHTMLImgElement *iface, REFIID riid, void **ppv)
{
    HTMLImgElement *This = HTMLIMG_THIS(iface);

    return IHTMLDOMNode_QueryInterface(HTMLDOMNODE(&This->element.node), riid, ppv);
}

static ULONG WINAPI HTMLImgElement_AddRef(IHTMLImgElement *iface)
{
    HTMLImgElement *This = HTMLIMG_THIS(iface);

    return IHTMLDOMNode_AddRef(HTMLDOMNODE(&This->element.node));
}

static ULONG WINAPI HTMLImgElement_Release(IHTMLImgElement *iface)
{
    HTMLImgElement *This = HTMLIMG_THIS(iface);

    return IHTMLDOMNode_Release(HTMLDOMNODE(&This->element.node));
}

static HRESULT WINAPI HTMLImgElement_GetTypeInfoCount(IHTMLImgElement *iface, UINT *pctinfo)
{
    HTMLImgElement *This = HTMLIMG_THIS(iface);
    return IDispatchEx_GetTypeInfoCount(DISPATCHEX(&This->element.node.dispex), pctinfo);
}

static HRESULT WINAPI HTMLImgElement_GetTypeInfo(IHTMLImgElement *iface, UINT iTInfo,
                                              LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLImgElement *This = HTMLIMG_THIS(iface);
    return IDispatchEx_GetTypeInfo(DISPATCHEX(&This->element.node.dispex), iTInfo, lcid, ppTInfo);
}

static HRESULT WINAPI HTMLImgElement_GetIDsOfNames(IHTMLImgElement *iface, REFIID riid,
                                                LPOLESTR *rgszNames, UINT cNames,
                                                LCID lcid, DISPID *rgDispId)
{
    HTMLImgElement *This = HTMLIMG_THIS(iface);
    return IDispatchEx_GetIDsOfNames(DISPATCHEX(&This->element.node.dispex), riid, rgszNames, cNames, lcid, rgDispId);
}

static HRESULT WINAPI HTMLImgElement_Invoke(IHTMLImgElement *iface, DISPID dispIdMember,
                            REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
                            VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLImgElement *This = HTMLIMG_THIS(iface);
    return IDispatchEx_Invoke(DISPATCHEX(&This->element.node.dispex), dispIdMember, riid, lcid, wFlags, pDispParams,
            pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI HTMLImgElement_put_isMap(IHTMLImgElement *iface, VARIANT_BOOL v)
{
    HTMLImgElement *This = HTMLIMG_THIS(iface);
    FIXME("(%p)->(%x)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLImgElement_get_isMap(IHTMLImgElement *iface, VARIANT_BOOL *p)
{
    HTMLImgElement *This = HTMLIMG_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLImgElement_put_useMap(IHTMLImgElement *iface, BSTR v)
{
    HTMLImgElement *This = HTMLIMG_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLImgElement_get_useMap(IHTMLImgElement *iface, BSTR *p)
{
    HTMLImgElement *This = HTMLIMG_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLImgElement_get_mimeType(IHTMLImgElement *iface, BSTR *p)
{
    HTMLImgElement *This = HTMLIMG_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLImgElement_get_fileSize(IHTMLImgElement *iface, BSTR *p)
{
    HTMLImgElement *This = HTMLIMG_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLImgElement_get_fileCreatedDate(IHTMLImgElement *iface, BSTR *p)
{
    HTMLImgElement *This = HTMLIMG_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLImgElement_get_fileModifiedDate(IHTMLImgElement *iface, BSTR *p)
{
    HTMLImgElement *This = HTMLIMG_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLImgElement_get_fileUpdatedDate(IHTMLImgElement *iface, BSTR *p)
{
    HTMLImgElement *This = HTMLIMG_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLImgElement_get_protocol(IHTMLImgElement *iface, BSTR *p)
{
    HTMLImgElement *This = HTMLIMG_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLImgElement_get_href(IHTMLImgElement *iface, BSTR *p)
{
    HTMLImgElement *This = HTMLIMG_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLImgElement_get_nameProp(IHTMLImgElement *iface, BSTR *p)
{
    HTMLImgElement *This = HTMLIMG_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLImgElement_put_border(IHTMLImgElement *iface, VARIANT v)
{
    HTMLImgElement *This = HTMLIMG_THIS(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLImgElement_get_border(IHTMLImgElement *iface, VARIANT *p)
{
    HTMLImgElement *This = HTMLIMG_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLImgElement_put_vspace(IHTMLImgElement *iface, LONG v)
{
    HTMLImgElement *This = HTMLIMG_THIS(iface);
    FIXME("(%p)->(%d)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLImgElement_get_vspace(IHTMLImgElement *iface, LONG *p)
{
    HTMLImgElement *This = HTMLIMG_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLImgElement_put_hspace(IHTMLImgElement *iface, LONG v)
{
    HTMLImgElement *This = HTMLIMG_THIS(iface);
    FIXME("(%p)->(%d)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLImgElement_get_hspace(IHTMLImgElement *iface, LONG *p)
{
    HTMLImgElement *This = HTMLIMG_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLImgElement_put_alt(IHTMLImgElement *iface, BSTR v)
{
    HTMLImgElement *This = HTMLIMG_THIS(iface);
    nsAString alt_str;
    nsresult nsres;

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    nsAString_InitDepend(&alt_str, v);
    nsres = nsIDOMHTMLImageElement_SetAlt(This->nsimg, &alt_str);
    nsAString_Finish(&alt_str);
    if(NS_FAILED(nsres))
        ERR("SetAlt failed: %08x\n", nsres);

    return S_OK;
}

static HRESULT WINAPI HTMLImgElement_get_alt(IHTMLImgElement *iface, BSTR *p)
{
    HTMLImgElement *This = HTMLIMG_THIS(iface);
    nsAString alt_str;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsAString_Init(&alt_str, NULL);
    nsres = nsIDOMHTMLImageElement_GetAlt(This->nsimg, &alt_str);
    if(NS_SUCCEEDED(nsres)) {
        const PRUnichar *alt;

        nsAString_GetData(&alt_str, &alt);
        *p = *alt ? SysAllocString(alt) : NULL;
    }else {
        ERR("GetAlt failed: %08x\n", nsres);
    }
    nsAString_Finish(&alt_str);

    return NS_SUCCEEDED(nsres) ? S_OK : E_FAIL;
}

static HRESULT WINAPI HTMLImgElement_put_src(IHTMLImgElement *iface, BSTR v)
{
    HTMLImgElement *This = HTMLIMG_THIS(iface);
    nsAString src_str;
    nsresult nsres;

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    nsAString_InitDepend(&src_str, v);
    nsres = nsIDOMHTMLImageElement_SetSrc(This->nsimg, &src_str);
    nsAString_Finish(&src_str);
    if(NS_FAILED(nsres))
        ERR("SetSrc failed: %08x\n", nsres);

    return NS_OK;
}

static HRESULT WINAPI HTMLImgElement_get_src(IHTMLImgElement *iface, BSTR *p)
{
    HTMLImgElement *This = HTMLIMG_THIS(iface);
    const PRUnichar *src;
    nsAString src_str;
    nsresult nsres;
    HRESULT hres;

    TRACE("(%p)->(%p)\n", This, p);

    nsAString_Init(&src_str, NULL);
    nsres = nsIDOMHTMLImageElement_GetSrc(This->nsimg, &src_str);
    if(NS_FAILED(nsres)) {
        ERR("GetSrc failed: %08x\n", nsres);
        return E_FAIL;
    }

    nsAString_GetData(&src_str, &src);
    hres = nsuri_to_url(src, TRUE, p);
    nsAString_Finish(&src_str);

    return hres;
}

static HRESULT WINAPI HTMLImgElement_put_lowsrc(IHTMLImgElement *iface, BSTR v)
{
    HTMLImgElement *This = HTMLIMG_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLImgElement_get_lowsrc(IHTMLImgElement *iface, BSTR *p)
{
    HTMLImgElement *This = HTMLIMG_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLImgElement_put_vrml(IHTMLImgElement *iface, BSTR v)
{
    HTMLImgElement *This = HTMLIMG_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLImgElement_get_vrml(IHTMLImgElement *iface, BSTR *p)
{
    HTMLImgElement *This = HTMLIMG_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLImgElement_put_dynsrc(IHTMLImgElement *iface, BSTR v)
{
    HTMLImgElement *This = HTMLIMG_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLImgElement_get_dynsrc(IHTMLImgElement *iface, BSTR *p)
{
    HTMLImgElement *This = HTMLIMG_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLImgElement_get_readyState(IHTMLImgElement *iface, BSTR *p)
{
    HTMLImgElement *This = HTMLIMG_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLImgElement_get_complete(IHTMLImgElement *iface, VARIANT_BOOL *p)
{
    HTMLImgElement *This = HTMLIMG_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLImgElement_put_loop(IHTMLImgElement *iface, VARIANT v)
{
    HTMLImgElement *This = HTMLIMG_THIS(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLImgElement_get_loop(IHTMLImgElement *iface, VARIANT *p)
{
    HTMLImgElement *This = HTMLIMG_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLImgElement_put_align(IHTMLImgElement *iface, BSTR v)
{
    HTMLImgElement *This = HTMLIMG_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLImgElement_get_align(IHTMLImgElement *iface, BSTR *p)
{
    HTMLImgElement *This = HTMLIMG_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLImgElement_put_onload(IHTMLImgElement *iface, VARIANT v)
{
    HTMLImgElement *This = HTMLIMG_THIS(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_node_event(&This->element.node, EVENTID_LOAD, &v);
}

static HRESULT WINAPI HTMLImgElement_get_onload(IHTMLImgElement *iface, VARIANT *p)
{
    HTMLImgElement *This = HTMLIMG_THIS(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_node_event(&This->element.node, EVENTID_LOAD, p);
}

static HRESULT WINAPI HTMLImgElement_put_onerror(IHTMLImgElement *iface, VARIANT v)
{
    HTMLImgElement *This = HTMLIMG_THIS(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLImgElement_get_onerror(IHTMLImgElement *iface, VARIANT *p)
{
    HTMLImgElement *This = HTMLIMG_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLImgElement_put_onabort(IHTMLImgElement *iface, VARIANT v)
{
    HTMLImgElement *This = HTMLIMG_THIS(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLImgElement_get_onabort(IHTMLImgElement *iface, VARIANT *p)
{
    HTMLImgElement *This = HTMLIMG_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLImgElement_put_name(IHTMLImgElement *iface, BSTR v)
{
    HTMLImgElement *This = HTMLIMG_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLImgElement_get_name(IHTMLImgElement *iface, BSTR *p)
{
    HTMLImgElement *This = HTMLIMG_THIS(iface);
    nsAString strName;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsAString_Init(&strName, NULL);
    nsres = nsIDOMHTMLImageElement_GetName(This->nsimg, &strName);
    if(NS_SUCCEEDED(nsres)) {
        const PRUnichar *str;

        nsAString_GetData(&strName, &str);
        *p = *str ? SysAllocString(str) : NULL;
    }else {
        ERR("GetName failed: %08x\n", nsres);
    }
    nsAString_Finish(&strName);

    return NS_SUCCEEDED(nsres) ? S_OK : E_FAIL;
}

static HRESULT WINAPI HTMLImgElement_put_width(IHTMLImgElement *iface, LONG v)
{
    HTMLImgElement *This = HTMLIMG_THIS(iface);
    nsresult nsres;

    TRACE("(%p)->(%d)\n", This, v);

    nsres = nsIDOMHTMLImageElement_SetWidth(This->nsimg, v);
    if(NS_FAILED(nsres)) {
        ERR("SetWidth failed: %08x\n", nsres);
        return E_FAIL;
    }

    return S_OK;
}

static HRESULT WINAPI HTMLImgElement_get_width(IHTMLImgElement *iface, LONG *p)
{
    HTMLImgElement *This = HTMLIMG_THIS(iface);
    PRInt32 width;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsres = nsIDOMHTMLImageElement_GetWidth(This->nsimg, &width);
    if(NS_FAILED(nsres)) {
        ERR("GetWidth failed: %08x\n", nsres);
        return E_FAIL;
    }

    *p = width;
    return S_OK;
}

static HRESULT WINAPI HTMLImgElement_put_height(IHTMLImgElement *iface, LONG v)
{
    HTMLImgElement *This = HTMLIMG_THIS(iface);
    nsresult nsres;

    TRACE("(%p)->(%d)\n", This, v);

    nsres = nsIDOMHTMLImageElement_SetHeight(This->nsimg, v);
    if(NS_FAILED(nsres)) {
        ERR("SetHeight failed: %08x\n", nsres);
        return E_FAIL;
    }

    return S_OK;
}

static HRESULT WINAPI HTMLImgElement_get_height(IHTMLImgElement *iface, LONG *p)
{
    HTMLImgElement *This = HTMLIMG_THIS(iface);
    PRInt32 height;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsres = nsIDOMHTMLImageElement_GetHeight(This->nsimg, &height);
    if(NS_FAILED(nsres)) {
        ERR("GetHeight failed: %08x\n", nsres);
        return E_FAIL;
    }

    *p = height;
    return S_OK;
}

static HRESULT WINAPI HTMLImgElement_put_start(IHTMLImgElement *iface, BSTR v)
{
    HTMLImgElement *This = HTMLIMG_THIS(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLImgElement_get_start(IHTMLImgElement *iface, BSTR *p)
{
    HTMLImgElement *This = HTMLIMG_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

#undef HTMLIMG_THIS

static const IHTMLImgElementVtbl HTMLImgElementVtbl = {
    HTMLImgElement_QueryInterface,
    HTMLImgElement_AddRef,
    HTMLImgElement_Release,
    HTMLImgElement_GetTypeInfoCount,
    HTMLImgElement_GetTypeInfo,
    HTMLImgElement_GetIDsOfNames,
    HTMLImgElement_Invoke,
    HTMLImgElement_put_isMap,
    HTMLImgElement_get_isMap,
    HTMLImgElement_put_useMap,
    HTMLImgElement_get_useMap,
    HTMLImgElement_get_mimeType,
    HTMLImgElement_get_fileSize,
    HTMLImgElement_get_fileCreatedDate,
    HTMLImgElement_get_fileModifiedDate,
    HTMLImgElement_get_fileUpdatedDate,
    HTMLImgElement_get_protocol,
    HTMLImgElement_get_href,
    HTMLImgElement_get_nameProp,
    HTMLImgElement_put_border,
    HTMLImgElement_get_border,
    HTMLImgElement_put_vspace,
    HTMLImgElement_get_vspace,
    HTMLImgElement_put_hspace,
    HTMLImgElement_get_hspace,
    HTMLImgElement_put_alt,
    HTMLImgElement_get_alt,
    HTMLImgElement_put_src,
    HTMLImgElement_get_src,
    HTMLImgElement_put_lowsrc,
    HTMLImgElement_get_lowsrc,
    HTMLImgElement_put_vrml,
    HTMLImgElement_get_vrml,
    HTMLImgElement_put_dynsrc,
    HTMLImgElement_get_dynsrc,
    HTMLImgElement_get_readyState,
    HTMLImgElement_get_complete,
    HTMLImgElement_put_loop,
    HTMLImgElement_get_loop,
    HTMLImgElement_put_align,
    HTMLImgElement_get_align,
    HTMLImgElement_put_onload,
    HTMLImgElement_get_onload,
    HTMLImgElement_put_onerror,
    HTMLImgElement_get_onerror,
    HTMLImgElement_put_onabort,
    HTMLImgElement_get_onabort,
    HTMLImgElement_put_name,
    HTMLImgElement_get_name,
    HTMLImgElement_put_width,
    HTMLImgElement_get_width,
    HTMLImgElement_put_height,
    HTMLImgElement_get_height,
    HTMLImgElement_put_start,
    HTMLImgElement_get_start
};

#define HTMLIMG_NODE_THIS(iface) DEFINE_THIS2(HTMLImgElement, element.node, iface)

static HRESULT HTMLImgElement_QI(HTMLDOMNode *iface, REFIID riid, void **ppv)
{
    HTMLImgElement *This = HTMLIMG_NODE_THIS(iface);

    *ppv = NULL;

    if(IsEqualGUID(&IID_IHTMLImgElement, riid)) {
        TRACE("(%p)->(IID_IHTMLImgElement %p)\n", This, ppv);
        *ppv = HTMLIMG(This);
    }else {
        return HTMLElement_QI(&This->element.node, riid, ppv);
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static void HTMLImgElement_destructor(HTMLDOMNode *iface)
{
    HTMLImgElement *This = HTMLIMG_NODE_THIS(iface);

    if(This->nsimg)
        nsIDOMHTMLImageElement_Release(This->nsimg);

    HTMLElement_destructor(&This->element.node);
}

static HRESULT HTMLImgElement_get_readystate(HTMLDOMNode *iface, BSTR *p)
{
    HTMLImgElement *This = HTMLIMG_NODE_THIS(iface);

    return IHTMLImgElement_get_readyState(HTMLIMG(This), p);
}

#undef HTMLIMG_NODE_THIS

static const NodeImplVtbl HTMLImgElementImplVtbl = {
    HTMLImgElement_QI,
    HTMLImgElement_destructor,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    HTMLImgElement_get_readystate
};

static const tid_t HTMLImgElement_iface_tids[] = {
    HTMLELEMENT_TIDS,
    IHTMLImgElement_tid,
    0
};
static dispex_static_data_t HTMLImgElement_dispex = {
    NULL,
    DispHTMLImg_tid,
    NULL,
    HTMLImgElement_iface_tids
};

HTMLElement *HTMLImgElement_Create(HTMLDocumentNode *doc, nsIDOMHTMLElement *nselem)
{
    HTMLImgElement *ret = heap_alloc_zero(sizeof(HTMLImgElement));
    nsresult nsres;

    ret->lpHTMLImgElementVtbl = &HTMLImgElementVtbl;
    ret->element.node.vtbl = &HTMLImgElementImplVtbl;

    nsres = nsIDOMHTMLElement_QueryInterface(nselem, &IID_nsIDOMHTMLImageElement, (void**)&ret->nsimg);
    if(NS_FAILED(nsres))
        ERR("Could not get nsIDOMHTMLImageElement: %08x\n", nsres);

    HTMLElement_Init(&ret->element, doc, nselem, &HTMLImgElement_dispex);

    return &ret->element;
}

#define HTMLIMGFACTORY_THIS(iface) DEFINE_THIS(HTMLImageElementFactory, HTMLImageElementFactory, iface)

static HRESULT WINAPI HTMLImageElementFactory_QueryInterface(IHTMLImageElementFactory *iface,
        REFIID riid, void **ppv)
{
    HTMLImageElementFactory *This = HTMLIMGFACTORY_THIS(iface);

    *ppv = NULL;

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_Unknown %p)\n", This, ppv);
        *ppv = HTMLIMGFACTORY(This);
    }else if(IsEqualGUID(&IID_IHTMLImageElementFactory, riid)) {
        TRACE("(%p)->(IID_IHTMLImageElementFactory %p)\n", This, ppv);
        *ppv = HTMLIMGFACTORY(This);
    }else if(dispex_query_interface(&This->dispex, riid, ppv))
        return *ppv ? S_OK : E_NOINTERFACE;

    if(*ppv) {
        IUnknown_AddRef((IUnknown*)*ppv);
        return S_OK;
    }

    WARN("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppv);
    return E_NOINTERFACE;
}

static ULONG WINAPI HTMLImageElementFactory_AddRef(IHTMLImageElementFactory *iface)
{
    HTMLImageElementFactory *This = HTMLIMGFACTORY_THIS(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    return ref;
}

static ULONG WINAPI HTMLImageElementFactory_Release(IHTMLImageElementFactory *iface)
{
    HTMLImageElementFactory *This = HTMLIMGFACTORY_THIS(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    if(!ref)
        heap_free(This);

    return ref;
}

static HRESULT WINAPI HTMLImageElementFactory_GetTypeInfoCount(IHTMLImageElementFactory *iface,
        UINT *pctinfo)
{
    HTMLImageElementFactory *This = HTMLIMGFACTORY_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pctinfo);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLImageElementFactory_GetTypeInfo(IHTMLImageElementFactory *iface,
        UINT iTInfo, LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLImageElementFactory *This = HTMLIMGFACTORY_THIS(iface);
    FIXME("(%p)->(%u %u %p)\n", This, iTInfo, lcid, ppTInfo);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLImageElementFactory_GetIDsOfNames(IHTMLImageElementFactory *iface,
        REFIID riid, LPOLESTR *rgszNames, UINT cNames, LCID lcid,
        DISPID *rgDispId)
{
    HTMLImageElementFactory *This = HTMLIMGFACTORY_THIS(iface);
    FIXME("(%p)->(%s %p %u %u %p)\n", This, debugstr_guid(riid), rgszNames,
            cNames, lcid, rgDispId);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLImageElementFactory_Invoke(IHTMLImageElementFactory *iface,
        DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags,
        DISPPARAMS *pDispParams, VARIANT *pVarResult, EXCEPINFO *pExcepInfo,
        UINT *puArgErr)
{
    HTMLImageElementFactory *This = HTMLIMGFACTORY_THIS(iface);
    FIXME("(%p)->(%d %s %d %d %p %p %p %p)\n", This, dispIdMember, debugstr_guid(riid),
            lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
    return E_NOTIMPL;
}

static LONG var_to_size(const VARIANT *v)
{
    switch(V_VT(v)) {
    case VT_EMPTY:
        return 0;
    case VT_I4:
        return V_I4(v);
    case VT_BSTR: {
        LONG ret;
        HRESULT hres;

        hres = VarI4FromStr(V_BSTR(v), 0, 0, &ret);
        if(FAILED(hres)) {
            FIXME("VarI4FromStr failed: %08x\n", hres);
            return 0;
        }
        return ret;
    }
    default:
        FIXME("unsupported size %s\n", debugstr_variant(v));
    }
    return 0;
}

static HRESULT WINAPI HTMLImageElementFactory_create(IHTMLImageElementFactory *iface,
        VARIANT width, VARIANT height, IHTMLImgElement **img_elem)
{
    HTMLImageElementFactory *This = HTMLIMGFACTORY_THIS(iface);
    IHTMLImgElement *img;
    HTMLElement *elem;
    nsIDOMHTMLElement *nselem;
    LONG l;
    HRESULT hres;

    static const PRUnichar imgW[] = {'I','M','G',0};

    TRACE("(%p)->(%s %s %p)\n", This, debugstr_variant(&width),
            debugstr_variant(&height), img_elem);

    if(!This->window || !This->window->doc) {
        WARN("NULL doc\n");
        return E_UNEXPECTED;
    }

    *img_elem = NULL;

    hres = create_nselem(This->window->doc, imgW, &nselem);
    if(FAILED(hres))
        return hres;

    elem = HTMLElement_Create(This->window->doc, (nsIDOMNode*)nselem, FALSE);
    if(!elem) {
        ERR("HTMLElement_Create failed\n");
        return E_FAIL;
    }

    hres = IHTMLElement_QueryInterface(HTMLELEM(elem), &IID_IHTMLImgElement, (void**)&img);
    if(FAILED(hres)) {
        ERR("IHTMLElement_QueryInterface failed: 0x%08x\n", hres);
        return hres;
    }

    nsIDOMHTMLElement_Release(nselem);

    l = var_to_size(&width);
    if(l)
        IHTMLImgElement_put_width(img, l);
    l = var_to_size(&height);
    if(l)
        IHTMLImgElement_put_height(img, l);

    *img_elem = img;
    return S_OK;
}

static HRESULT HTMLImageElementFactory_value(IUnknown *iface, LCID lcid,
        WORD flags, DISPPARAMS *params, VARIANT *res, EXCEPINFO *ei,
        IServiceProvider *caller)
{
    HTMLImageElementFactory *This = HTMLIMGFACTORY_THIS(iface);
    IHTMLImgElement *img;
    VARIANT empty, *width, *height;
    HRESULT hres;
    int argc = params->cArgs - params->cNamedArgs;

    V_VT(res) = VT_NULL;

    V_VT(&empty) = VT_EMPTY;

    width = argc >= 1 ? params->rgvarg + (params->cArgs - 1) : &empty;
    height = argc >= 2 ? params->rgvarg + (params->cArgs - 2) : &empty;

    hres = IHTMLImageElementFactory_create(HTMLIMGFACTORY(This), *width, *height, &img);
    if(FAILED(hres))
        return hres;

    V_VT(res) = VT_DISPATCH;
    V_DISPATCH(res) = (IDispatch*)img;

    return S_OK;
}

#undef HTMLIMGFACTORY_THIS

static const IHTMLImageElementFactoryVtbl HTMLImageElementFactoryVtbl = {
    HTMLImageElementFactory_QueryInterface,
    HTMLImageElementFactory_AddRef,
    HTMLImageElementFactory_Release,
    HTMLImageElementFactory_GetTypeInfoCount,
    HTMLImageElementFactory_GetTypeInfo,
    HTMLImageElementFactory_GetIDsOfNames,
    HTMLImageElementFactory_Invoke,
    HTMLImageElementFactory_create
};

static const tid_t HTMLImageElementFactory_iface_tids[] = {
    IHTMLImageElementFactory_tid,
    0
};

static const dispex_static_data_vtbl_t HTMLImageElementFactory_dispex_vtbl = {
    HTMLImageElementFactory_value,
    NULL,
    NULL
};

static dispex_static_data_t HTMLImageElementFactory_dispex = {
    &HTMLImageElementFactory_dispex_vtbl,
    IHTMLImageElementFactory_tid,
    NULL,
    HTMLImageElementFactory_iface_tids
};

HTMLImageElementFactory *HTMLImageElementFactory_Create(HTMLWindow *window)
{
    HTMLImageElementFactory *ret;

    ret = heap_alloc(sizeof(HTMLImageElementFactory));

    ret->lpHTMLImageElementFactoryVtbl = &HTMLImageElementFactoryVtbl;
    ret->ref = 1;
    ret->window = window;

    init_dispex(&ret->dispex, (IUnknown*)HTMLIMGFACTORY(ret), &HTMLImageElementFactory_dispex);

    return ret;
}
