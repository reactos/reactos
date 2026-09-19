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
#include "mshtmdid.h"

#include "wine/debug.h"

#include "mshtml_private.h"
#include "htmlevent.h"

WINE_DEFAULT_DEBUG_CHANNEL(mshtml);

struct HTMLImg {
    HTMLElement element;

    IHTMLImgElement IHTMLImgElement_iface;

    nsIDOMHTMLImageElement *nsimg;
};

static inline HTMLImg *impl_from_IHTMLImgElement(IHTMLImgElement *iface)
{
    return CONTAINING_RECORD(iface, HTMLImg, IHTMLImgElement_iface);
}

DISPEX_IDISPATCH_IMPL(HTMLImgElement, IHTMLImgElement,
                      impl_from_IHTMLImgElement(iface)->element.node.event_target.dispex)

static HRESULT WINAPI HTMLImgElement_put_isMap(IHTMLImgElement *iface, VARIANT_BOOL v)
{
    HTMLImg *This = impl_from_IHTMLImgElement(iface);
    nsresult nsres;

    TRACE("(%p)->(%x)\n", This, v);

    nsres = nsIDOMHTMLImageElement_SetIsMap(This->nsimg, v != VARIANT_FALSE);
    if (NS_FAILED(nsres)) {
        ERR("Set IsMap failed: %08lx\n", nsres);
        return E_FAIL;
    }

    return S_OK;
}

static HRESULT WINAPI HTMLImgElement_get_isMap(IHTMLImgElement *iface, VARIANT_BOOL *p)
{
    HTMLImg *This = impl_from_IHTMLImgElement(iface);
    cpp_bool b;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    if (p == NULL)
        return E_INVALIDARG;

    nsres = nsIDOMHTMLImageElement_GetIsMap(This->nsimg, &b);
    if (NS_FAILED(nsres)) {
        ERR("Get IsMap failed: %08lx\n", nsres);
        return E_FAIL;
    }

    *p = variant_bool(b);
    return S_OK;
}

static HRESULT WINAPI HTMLImgElement_put_useMap(IHTMLImgElement *iface, BSTR v)
{
    HTMLImg *This = impl_from_IHTMLImgElement(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLImgElement_get_useMap(IHTMLImgElement *iface, BSTR *p)
{
    HTMLImg *This = impl_from_IHTMLImgElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLImgElement_get_mimeType(IHTMLImgElement *iface, BSTR *p)
{
    HTMLImg *This = impl_from_IHTMLImgElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLImgElement_get_fileSize(IHTMLImgElement *iface, BSTR *p)
{
    HTMLImg *This = impl_from_IHTMLImgElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLImgElement_get_fileCreatedDate(IHTMLImgElement *iface, BSTR *p)
{
    HTMLImg *This = impl_from_IHTMLImgElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLImgElement_get_fileModifiedDate(IHTMLImgElement *iface, BSTR *p)
{
    HTMLImg *This = impl_from_IHTMLImgElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLImgElement_get_fileUpdatedDate(IHTMLImgElement *iface, BSTR *p)
{
    HTMLImg *This = impl_from_IHTMLImgElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLImgElement_get_protocol(IHTMLImgElement *iface, BSTR *p)
{
    HTMLImg *This = impl_from_IHTMLImgElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLImgElement_get_href(IHTMLImgElement *iface, BSTR *p)
{
    HTMLImg *This = impl_from_IHTMLImgElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLImgElement_get_nameProp(IHTMLImgElement *iface, BSTR *p)
{
    HTMLImg *This = impl_from_IHTMLImgElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLImgElement_put_border(IHTMLImgElement *iface, VARIANT v)
{
    HTMLImg *This = impl_from_IHTMLImgElement(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLImgElement_get_border(IHTMLImgElement *iface, VARIANT *p)
{
    HTMLImg *This = impl_from_IHTMLImgElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLImgElement_put_vspace(IHTMLImgElement *iface, LONG v)
{
    HTMLImg *This = impl_from_IHTMLImgElement(iface);
    FIXME("(%p)->(%ld)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLImgElement_get_vspace(IHTMLImgElement *iface, LONG *p)
{
    HTMLImg *This = impl_from_IHTMLImgElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLImgElement_put_hspace(IHTMLImgElement *iface, LONG v)
{
    HTMLImg *This = impl_from_IHTMLImgElement(iface);
    FIXME("(%p)->(%ld)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLImgElement_get_hspace(IHTMLImgElement *iface, LONG *p)
{
    HTMLImg *This = impl_from_IHTMLImgElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLImgElement_put_alt(IHTMLImgElement *iface, BSTR v)
{
    HTMLImg *This = impl_from_IHTMLImgElement(iface);
    nsAString alt_str;
    nsresult nsres;

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    nsAString_InitDepend(&alt_str, v);
    nsres = nsIDOMHTMLImageElement_SetAlt(This->nsimg, &alt_str);
    nsAString_Finish(&alt_str);
    if(NS_FAILED(nsres))
        ERR("SetAlt failed: %08lx\n", nsres);

    return S_OK;
}

static HRESULT WINAPI HTMLImgElement_get_alt(IHTMLImgElement *iface, BSTR *p)
{
    HTMLImg *This = impl_from_IHTMLImgElement(iface);
    nsAString alt_str;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsAString_Init(&alt_str, NULL);
    nsres = nsIDOMHTMLImageElement_GetAlt(This->nsimg, &alt_str);
    return return_nsstr(nsres, &alt_str, p);
}

static HRESULT WINAPI HTMLImgElement_put_src(IHTMLImgElement *iface, BSTR v)
{
    HTMLImg *This = impl_from_IHTMLImgElement(iface);
    nsAString src_str;
    nsresult nsres;

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    nsAString_InitDepend(&src_str, v);
    nsres = nsIDOMHTMLImageElement_SetSrc(This->nsimg, &src_str);
    nsAString_Finish(&src_str);
    return map_nsresult(nsres);
}

static HRESULT WINAPI HTMLImgElement_get_src(IHTMLImgElement *iface, BSTR *p)
{
    HTMLImg *This = impl_from_IHTMLImgElement(iface);
    const PRUnichar *src;
    nsAString src_str;
    nsresult nsres;
    HRESULT hres = S_OK;

    TRACE("(%p)->(%p)\n", This, p);

    nsAString_Init(&src_str, NULL);
    nsres = nsIDOMHTMLImageElement_GetSrc(This->nsimg, &src_str);
    if(NS_SUCCEEDED(nsres)) {
        nsAString_GetData(&src_str, &src);

        if(!wcsnicmp(src, L"BLOCKED::", ARRAY_SIZE(L"BLOCKED::")-1)) {
            TRACE("returning BLOCKED::\n");
            *p = SysAllocString(L"BLOCKED::");
            if(!*p)
                hres = E_OUTOFMEMORY;
        }else {
            hres = nsuri_to_url(src, TRUE, p);
        }
    }else {
        ERR("GetSrc failed: %08lx\n", nsres);
        hres = E_FAIL;
    }

    nsAString_Finish(&src_str);
    return hres;
}

static HRESULT WINAPI HTMLImgElement_put_lowsrc(IHTMLImgElement *iface, BSTR v)
{
    HTMLImg *This = impl_from_IHTMLImgElement(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLImgElement_get_lowsrc(IHTMLImgElement *iface, BSTR *p)
{
    HTMLImg *This = impl_from_IHTMLImgElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLImgElement_put_vrml(IHTMLImgElement *iface, BSTR v)
{
    HTMLImg *This = impl_from_IHTMLImgElement(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLImgElement_get_vrml(IHTMLImgElement *iface, BSTR *p)
{
    HTMLImg *This = impl_from_IHTMLImgElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLImgElement_put_dynsrc(IHTMLImgElement *iface, BSTR v)
{
    HTMLImg *This = impl_from_IHTMLImgElement(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLImgElement_get_dynsrc(IHTMLImgElement *iface, BSTR *p)
{
    HTMLImg *This = impl_from_IHTMLImgElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLImgElement_get_readyState(IHTMLImgElement *iface, BSTR *p)
{
    HTMLImg *This = impl_from_IHTMLImgElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLImgElement_get_complete(IHTMLImgElement *iface, VARIANT_BOOL *p)
{
    HTMLImg *This = impl_from_IHTMLImgElement(iface);
    cpp_bool complete;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsres = nsIDOMHTMLImageElement_GetComplete(This->nsimg, &complete);
    if(NS_FAILED(nsres)) {
        ERR("GetComplete failed: %08lx\n", nsres);
        return E_FAIL;
    }

    *p = variant_bool(complete);
    return S_OK;
}

static HRESULT WINAPI HTMLImgElement_put_loop(IHTMLImgElement *iface, VARIANT v)
{
    HTMLImg *This = impl_from_IHTMLImgElement(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLImgElement_get_loop(IHTMLImgElement *iface, VARIANT *p)
{
    HTMLImg *This = impl_from_IHTMLImgElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLImgElement_put_align(IHTMLImgElement *iface, BSTR v)
{
    HTMLImg *This = impl_from_IHTMLImgElement(iface);
    nsAString str;
    nsresult nsres;

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    nsAString_InitDepend(&str, v);

    nsres = nsIDOMHTMLImageElement_SetAlign(This->nsimg, &str);
    nsAString_Finish(&str);
    if (NS_FAILED(nsres)){
        ERR("Set Align(%s) failed: %08lx\n", debugstr_w(v), nsres);
        return E_FAIL;
    }

    return S_OK;
}

static HRESULT WINAPI HTMLImgElement_get_align(IHTMLImgElement *iface, BSTR *p)
{
    HTMLImg *This = impl_from_IHTMLImgElement(iface);
    nsAString str;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsAString_Init(&str, NULL);
    nsres = nsIDOMHTMLImageElement_GetAlign(This->nsimg, &str);

    return return_nsstr(nsres, &str, p);
}

static HRESULT WINAPI HTMLImgElement_put_onload(IHTMLImgElement *iface, VARIANT v)
{
    HTMLImg *This = impl_from_IHTMLImgElement(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_node_event(&This->element.node, EVENTID_LOAD, &v);
}

static HRESULT WINAPI HTMLImgElement_get_onload(IHTMLImgElement *iface, VARIANT *p)
{
    HTMLImg *This = impl_from_IHTMLImgElement(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_node_event(&This->element.node, EVENTID_LOAD, p);
}

static HRESULT WINAPI HTMLImgElement_put_onerror(IHTMLImgElement *iface, VARIANT v)
{
    HTMLImg *This = impl_from_IHTMLImgElement(iface);

    TRACE("(%p)->()\n", This);

    return set_node_event(&This->element.node, EVENTID_ERROR, &v);
}

static HRESULT WINAPI HTMLImgElement_get_onerror(IHTMLImgElement *iface, VARIANT *p)
{
    HTMLImg *This = impl_from_IHTMLImgElement(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_node_event(&This->element.node, EVENTID_ERROR, p);
}

static HRESULT WINAPI HTMLImgElement_put_onabort(IHTMLImgElement *iface, VARIANT v)
{
    HTMLImg *This = impl_from_IHTMLImgElement(iface);

    TRACE("(%p)->()\n", This);

    return set_node_event(&This->element.node, EVENTID_ABORT, &v);
}

static HRESULT WINAPI HTMLImgElement_get_onabort(IHTMLImgElement *iface, VARIANT *p)
{
    HTMLImg *This = impl_from_IHTMLImgElement(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_node_event(&This->element.node, EVENTID_ABORT, p);
}

static HRESULT WINAPI HTMLImgElement_put_name(IHTMLImgElement *iface, BSTR v)
{
    HTMLImg *This = impl_from_IHTMLImgElement(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLImgElement_get_name(IHTMLImgElement *iface, BSTR *p)
{
    HTMLImg *This = impl_from_IHTMLImgElement(iface);
    nsAString name;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsAString_Init(&name, NULL);
    nsres = nsIDOMHTMLImageElement_GetName(This->nsimg, &name);
    return return_nsstr(nsres, &name, p);
}

static HRESULT WINAPI HTMLImgElement_put_width(IHTMLImgElement *iface, LONG v)
{
    HTMLImg *This = impl_from_IHTMLImgElement(iface);
    nsresult nsres;

    TRACE("(%p)->(%ld)\n", This, v);

    nsres = nsIDOMHTMLImageElement_SetWidth(This->nsimg, v);
    if(NS_FAILED(nsres)) {
        ERR("SetWidth failed: %08lx\n", nsres);
        return E_FAIL;
    }

    return S_OK;
}

static HRESULT WINAPI HTMLImgElement_get_width(IHTMLImgElement *iface, LONG *p)
{
    HTMLImg *This = impl_from_IHTMLImgElement(iface);
    UINT32 width;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsres = nsIDOMHTMLImageElement_GetWidth(This->nsimg, &width);
    if(NS_FAILED(nsres)) {
        ERR("GetWidth failed: %08lx\n", nsres);
        return E_FAIL;
    }

    *p = width;
    return S_OK;
}

static HRESULT WINAPI HTMLImgElement_put_height(IHTMLImgElement *iface, LONG v)
{
    HTMLImg *This = impl_from_IHTMLImgElement(iface);
    nsresult nsres;

    TRACE("(%p)->(%ld)\n", This, v);

    nsres = nsIDOMHTMLImageElement_SetHeight(This->nsimg, v);
    if(NS_FAILED(nsres)) {
        ERR("SetHeight failed: %08lx\n", nsres);
        return E_FAIL;
    }

    return S_OK;
}

static HRESULT WINAPI HTMLImgElement_get_height(IHTMLImgElement *iface, LONG *p)
{
    HTMLImg *This = impl_from_IHTMLImgElement(iface);
    UINT32 height;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsres = nsIDOMHTMLImageElement_GetHeight(This->nsimg, &height);
    if(NS_FAILED(nsres)) {
        ERR("GetHeight failed: %08lx\n", nsres);
        return E_FAIL;
    }

    *p = height;
    return S_OK;
}

static HRESULT WINAPI HTMLImgElement_put_start(IHTMLImgElement *iface, BSTR v)
{
    HTMLImg *This = impl_from_IHTMLImgElement(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLImgElement_get_start(IHTMLImgElement *iface, BSTR *p)
{
    HTMLImg *This = impl_from_IHTMLImgElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

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

static inline HTMLImg *impl_from_HTMLDOMNode(HTMLDOMNode *iface)
{
    return CONTAINING_RECORD(iface, HTMLImg, element.node);
}

static HRESULT HTMLImgElement_get_readystate(HTMLDOMNode *iface, BSTR *p)
{
    HTMLImg *This = impl_from_HTMLDOMNode(iface);

    return IHTMLImgElement_get_readyState(&This->IHTMLImgElement_iface, p);
}

static inline HTMLImg *HTMLImg_from_DispatchEx(DispatchEx *iface)
{
    return CONTAINING_RECORD(iface, HTMLImg, element.node.event_target.dispex);
}

static void *HTMLImgElement_query_interface(DispatchEx *dispex, REFIID riid)
{
    HTMLImg *This = HTMLImg_from_DispatchEx(dispex);

    if(IsEqualGUID(&IID_IHTMLImgElement, riid))
        return &This->IHTMLImgElement_iface;

    return HTMLElement_query_interface(&This->element.node.event_target.dispex, riid);
}

static void HTMLImgElement_traverse(DispatchEx *dispex, nsCycleCollectionTraversalCallback *cb)
{
    HTMLImg *This = HTMLImg_from_DispatchEx(dispex);
    HTMLElement_traverse(dispex, cb);

    if(This->nsimg)
        note_cc_edge((nsISupports*)This->nsimg, "nsimg", cb);
}

static void HTMLImgElement_unlink(DispatchEx *dispex)
{
    HTMLImg *This = HTMLImg_from_DispatchEx(dispex);
    HTMLElement_unlink(dispex);
    unlink_ref(&This->nsimg);
}

static const NodeImplVtbl HTMLImgElementImplVtbl = {
    .clsid                 = &CLSID_HTMLImg,
    .cpc_entries           = HTMLElement_cpc,
    .clone                 = HTMLElement_clone,
    .get_attr_col          = HTMLElement_get_attr_col,
    .get_readystate        = HTMLImgElement_get_readystate,
};

static const event_target_vtbl_t HTMLImgElement_event_target_vtbl = {
    {
        HTMLELEMENT_DISPEX_VTBL_ENTRIES,
        .query_interface= HTMLImgElement_query_interface,
        .destructor     = HTMLElement_destructor,
        .traverse       = HTMLImgElement_traverse,
        .unlink         = HTMLImgElement_unlink
    },
    HTMLELEMENT_EVENT_TARGET_VTBL_ENTRIES,
    .handle_event       = HTMLElement_handle_event
};

static void HTMLImgElement_init_dispex_info(dispex_data_t *info, compat_mode_t mode)
{
    static const dispex_hook_t img_ie11_hooks[] = {
        {DISPID_IHTMLIMGELEMENT_FILESIZE, NULL},
        {DISPID_UNKNOWN}
    };

    HTMLElement_init_dispex_info(info, mode);

    dispex_info_add_interface(info, IHTMLImgElement_tid, mode >= COMPAT_MODE_IE11 ? img_ie11_hooks : NULL);
}

dispex_static_data_t HTMLImageElement_dispex = {
    .id           = PROT_HTMLImageElement,
    .prototype_id = PROT_HTMLElement,
    .vtbl         = &HTMLImgElement_event_target_vtbl.dispex_vtbl,
    .disp_tid     = DispHTMLImg_tid,
    .init_info    = HTMLImgElement_init_dispex_info,
};

HRESULT HTMLImgElement_Create(HTMLDocumentNode *doc, nsIDOMElement *nselem, HTMLElement **elem)
{
    HTMLImg *ret;
    nsresult nsres;

    ret = calloc(1, sizeof(HTMLImg));
    if(!ret)
        return E_OUTOFMEMORY;

    ret->IHTMLImgElement_iface.lpVtbl = &HTMLImgElementVtbl;
    ret->element.node.vtbl = &HTMLImgElementImplVtbl;

    HTMLElement_Init(&ret->element, doc, nselem, &HTMLImageElement_dispex);

    nsres = nsIDOMElement_QueryInterface(nselem, &IID_nsIDOMHTMLImageElement, (void**)&ret->nsimg);
    assert(nsres == NS_OK);

    *elem = &ret->element;
    return S_OK;
}

static inline HTMLImageElementFactory *impl_from_IHTMLImageElementFactory(IHTMLImageElementFactory *iface)
{
    return CONTAINING_RECORD(iface, HTMLImageElementFactory, IHTMLImageElementFactory_iface);
}

DISPEX_IDISPATCH_IMPL(HTMLImageElementFactory, IHTMLImageElementFactory,
                      impl_from_IHTMLImageElementFactory(iface)->dispex)

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
            FIXME("VarI4FromStr failed: %08lx\n", hres);
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
    HTMLImageElementFactory *This = impl_from_IHTMLImageElementFactory(iface);
    HTMLDocumentNode *doc = This->window->doc;
    IHTMLImgElement *img;
    HTMLElement *elem;
    nsIDOMElement *nselem;
    LONG l;
    HRESULT hres;

    TRACE("(%p)->(%s %s %p)\n", This, debugstr_variant(&width),
            debugstr_variant(&height), img_elem);

    *img_elem = NULL;

    hres = create_nselem(doc, L"IMG", &nselem);
    if(FAILED(hres))
        return hres;

    hres = HTMLElement_Create(doc, (nsIDOMNode*)nselem, FALSE, &elem);
    nsIDOMElement_Release(nselem);
    if(FAILED(hres)) {
        ERR("HTMLElement_Create failed\n");
        return hres;
    }

    hres = IHTMLElement_QueryInterface(&elem->IHTMLElement_iface, &IID_IHTMLImgElement,
            (void**)&img);
    IHTMLElement_Release(&elem->IHTMLElement_iface);
    if(FAILED(hres)) {
        ERR("IHTMLElement_QueryInterface failed: 0x%08lx\n", hres);
        return hres;
    }

    l = var_to_size(&width);
    if(l)
        IHTMLImgElement_put_width(img, l);
    l = var_to_size(&height);
    if(l)
        IHTMLImgElement_put_height(img, l);

    *img_elem = img;
    return S_OK;
}

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

static inline HTMLImageElementFactory *impl_from_DispatchEx(DispatchEx *iface)
{
    return CONTAINING_RECORD(iface, HTMLImageElementFactory, dispex);
}

static void *HTMLImageElementFactory_query_interface(DispatchEx *dispex, REFIID riid)
{
    HTMLImageElementFactory *This = impl_from_DispatchEx(dispex);

    if(IsEqualGUID(&IID_IHTMLImageElementFactory, riid))
        return &This->IHTMLImageElementFactory_iface;

    return NULL;
}

static void HTMLImageElementFactory_traverse(DispatchEx *dispex, nsCycleCollectionTraversalCallback *cb)
{
    HTMLImageElementFactory *This = impl_from_DispatchEx(dispex);

    if(This->window)
        note_cc_edge((nsISupports*)&This->window->base.IHTMLWindow2_iface, "window", cb);
}

static void HTMLImageElementFactory_unlink(DispatchEx *dispex)
{
    HTMLImageElementFactory *This = impl_from_DispatchEx(dispex);

    if(This->window) {
        HTMLInnerWindow *window = This->window;
        This->window = NULL;
        IHTMLWindow2_Release(&window->base.IHTMLWindow2_iface);
    }
}

static void HTMLImageElementFactory_destructor(DispatchEx *dispex)
{
    HTMLImageElementFactory *This = impl_from_DispatchEx(dispex);
    free(This);
}

static HRESULT HTMLImageElementFactory_value(DispatchEx *dispex, LCID lcid,
        WORD flags, DISPPARAMS *params, VARIANT *res, EXCEPINFO *ei,
        IServiceProvider *caller)
{
    HTMLImageElementFactory *This = impl_from_DispatchEx(dispex);
    IHTMLImgElement *img;
    VARIANT empty, *width, *height;
    HRESULT hres;
    int argc = params->cArgs - params->cNamedArgs;

    V_VT(res) = VT_NULL;

    V_VT(&empty) = VT_EMPTY;

    width = argc >= 1 ? params->rgvarg + (params->cArgs - 1) : &empty;
    height = argc >= 2 ? params->rgvarg + (params->cArgs - 2) : &empty;

    hres = IHTMLImageElementFactory_create(&This->IHTMLImageElementFactory_iface, *width, *height,
            &img);
    if(FAILED(hres))
        return hres;

    V_VT(res) = VT_DISPATCH;
    V_DISPATCH(res) = (IDispatch*)img;

    return S_OK;
}

static const tid_t HTMLImageElementFactory_iface_tids[] = {
    IHTMLImageElementFactory_tid,
    0
};

static const dispex_static_data_vtbl_t HTMLImageElementFactory_dispex_vtbl = {
    .query_interface  = HTMLImageElementFactory_query_interface,
    .destructor       = HTMLImageElementFactory_destructor,
    .traverse         = HTMLImageElementFactory_traverse,
    .unlink           = HTMLImageElementFactory_unlink,
    .value            = HTMLImageElementFactory_value,
};

static dispex_static_data_t HTMLImageElementFactory_dispex = {
    .name           = "Function",
    .constructor_id = PROT_HTMLImageElement,
    .vtbl           = &HTMLImageElementFactory_dispex_vtbl,
    .disp_tid       = IHTMLImageElementFactory_tid,
    .iface_tids     = HTMLImageElementFactory_iface_tids,
};

HRESULT HTMLImageElementFactory_Create(HTMLInnerWindow *window, HTMLImageElementFactory **ret_val)
{
    HTMLImageElementFactory *ret;

    ret = malloc(sizeof(HTMLImageElementFactory));
    if(!ret)
        return E_OUTOFMEMORY;

    ret->IHTMLImageElementFactory_iface.lpVtbl = &HTMLImageElementFactoryVtbl;
    ret->window = window;
    IHTMLWindow2_AddRef(&window->base.IHTMLWindow2_iface);

    init_dispatch(&ret->dispex, &HTMLImageElementFactory_dispex, window,
                  dispex_compat_mode(&window->event_target.dispex));

    *ret_val = ret;
    return S_OK;
}
