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

    const IHTMLImgElementVtbl *lpHTMLImgElementVtbl;

    nsIDOMHTMLImageElement *nsimg;
} HTMLImgElement;

#define HTMLIMG(x)  (&(x)->lpHTMLImgElementVtbl)

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

    nsAString_Init(&alt_str, v);
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

    nsAString_Init(&src_str, v);
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
    hres = nsuri_to_url(src, p);
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
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLImgElement_get_onload(IHTMLImgElement *iface, VARIANT *p)
{
    HTMLImgElement *This = HTMLIMG_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
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
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLImgElement_put_width(IHTMLImgElement *iface, LONG v)
{
    HTMLImgElement *This = HTMLIMG_THIS(iface);
    FIXME("(%p)->(%d)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLImgElement_get_width(IHTMLImgElement *iface, LONG *p)
{
    HTMLImgElement *This = HTMLIMG_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLImgElement_put_height(IHTMLImgElement *iface, LONG v)
{
    HTMLImgElement *This = HTMLIMG_THIS(iface);
    FIXME("(%p)->(%d)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLImgElement_get_height(IHTMLImgElement *iface, LONG *p)
{
    HTMLImgElement *This = HTMLIMG_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
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

#undef HTMLIMG_NODE_THIS

static const NodeImplVtbl HTMLImgElementImplVtbl = {
    HTMLImgElement_QI,
    HTMLImgElement_destructor
};

static const tid_t HTMLImgElement_iface_tids[] = {
    IHTMLDOMNode_tid,
    IHTMLDOMNode2_tid,
    IHTMLElement_tid,
    IHTMLElement2_tid,
    IHTMLElement3_tid,
    IHTMLImgElement_tid,
    0
};
static dispex_static_data_t HTMLImgElement_dispex = {
    NULL,
    DispHTMLImg_tid,
    NULL,
    HTMLImgElement_iface_tids
};

HTMLElement *HTMLImgElement_Create(nsIDOMHTMLElement *nselem)
{
    HTMLImgElement *ret = heap_alloc_zero(sizeof(HTMLImgElement));
    nsresult nsres;

    ret->lpHTMLImgElementVtbl = &HTMLImgElementVtbl;
    ret->element.node.vtbl = &HTMLImgElementImplVtbl;

    nsres = nsIDOMHTMLElement_QueryInterface(nselem, &IID_nsIDOMHTMLImageElement, (void**)&ret->nsimg);
    if(NS_FAILED(nsres))
        ERR("Could not get nsIDOMHTMLImageElement: %08x\n", nsres);

    init_dispex(&ret->element.node.dispex, (IUnknown*)HTMLIMG(ret), &HTMLImgElement_dispex);
    HTMLElement_Init(&ret->element);

    return &ret->element;
}
