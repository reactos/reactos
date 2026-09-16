/*
 * Copyright 2015 Alex Henrie
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

struct HTMLAreaElement {
    HTMLElement element;

    IHTMLAreaElement IHTMLAreaElement_iface;

    nsIDOMHTMLAreaElement *nsarea;
};

static inline HTMLAreaElement *impl_from_IHTMLAreaElement(IHTMLAreaElement *iface)
{
    return CONTAINING_RECORD(iface, HTMLAreaElement, IHTMLAreaElement_iface);
}

DISPEX_IDISPATCH_IMPL(HTMLAreaElement, IHTMLAreaElement,
                      impl_from_IHTMLAreaElement(iface)->element.node.event_target.dispex)

static HRESULT WINAPI HTMLAreaElement_put_shape(IHTMLAreaElement *iface, BSTR v)
{
    HTMLAreaElement *This = impl_from_IHTMLAreaElement(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLAreaElement_get_shape(IHTMLAreaElement *iface, BSTR *p)
{
    HTMLAreaElement *This = impl_from_IHTMLAreaElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLAreaElement_put_coords(IHTMLAreaElement *iface, BSTR v)
{
    HTMLAreaElement *This = impl_from_IHTMLAreaElement(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLAreaElement_get_coords(IHTMLAreaElement *iface, BSTR *p)
{
    HTMLAreaElement *This = impl_from_IHTMLAreaElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLAreaElement_put_href(IHTMLAreaElement *iface, BSTR v)
{
    HTMLAreaElement *This = impl_from_IHTMLAreaElement(iface);
    nsAString nsstr;
    nsresult nsres;

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    nsAString_InitDepend(&nsstr, v);
    nsres = nsIDOMHTMLAreaElement_SetHref(This->nsarea, &nsstr);
    nsAString_Finish(&nsstr);
    if(NS_FAILED(nsres))
        return E_FAIL;

    return S_OK;
}

static HRESULT WINAPI HTMLAreaElement_get_href(IHTMLAreaElement *iface, BSTR *p)
{
    HTMLAreaElement *This = impl_from_IHTMLAreaElement(iface);
    nsAString href_str;
    nsresult nsres;
    HRESULT hres;

    TRACE("(%p)->(%p)\n", This, p);

    nsAString_Init(&href_str, NULL);
    nsres = nsIDOMHTMLAreaElement_GetHref(This->nsarea, &href_str);
    if(NS_SUCCEEDED(nsres)) {
        const PRUnichar *href;

        nsAString_GetData(&href_str, &href);
        hres = nsuri_to_url(href, TRUE, p);
    }else {
        ERR("GetHref failed: %08lx\n", nsres);
        hres = E_FAIL;
    }

    nsAString_Finish(&href_str);
    return hres;
}

static HRESULT WINAPI HTMLAreaElement_put_target(IHTMLAreaElement *iface, BSTR v)
{
    HTMLAreaElement *This = impl_from_IHTMLAreaElement(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLAreaElement_get_target(IHTMLAreaElement *iface, BSTR *p)
{
    HTMLAreaElement *This = impl_from_IHTMLAreaElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLAreaElement_put_alt(IHTMLAreaElement *iface, BSTR v)
{
    HTMLAreaElement *This = impl_from_IHTMLAreaElement(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLAreaElement_get_alt(IHTMLAreaElement *iface, BSTR *p)
{
    HTMLAreaElement *This = impl_from_IHTMLAreaElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLAreaElement_put_noHref(IHTMLAreaElement *iface, VARIANT_BOOL v)
{
    HTMLAreaElement *This = impl_from_IHTMLAreaElement(iface);
    FIXME("(%p)->(%i)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLAreaElement_get_noHref(IHTMLAreaElement *iface, VARIANT_BOOL *p)
{
    HTMLAreaElement *This = impl_from_IHTMLAreaElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLAreaElement_put_host(IHTMLAreaElement *iface, BSTR v)
{
    HTMLAreaElement *This = impl_from_IHTMLAreaElement(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLAreaElement_get_host(IHTMLAreaElement *iface, BSTR *p)
{
    HTMLAreaElement *This = impl_from_IHTMLAreaElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLAreaElement_put_hostname(IHTMLAreaElement *iface, BSTR v)
{
    HTMLAreaElement *This = impl_from_IHTMLAreaElement(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLAreaElement_get_hostname(IHTMLAreaElement *iface, BSTR *p)
{
    HTMLAreaElement *This = impl_from_IHTMLAreaElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLAreaElement_put_pathname(IHTMLAreaElement *iface, BSTR v)
{
    HTMLAreaElement *This = impl_from_IHTMLAreaElement(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLAreaElement_get_pathname(IHTMLAreaElement *iface, BSTR *p)
{
    HTMLAreaElement *This = impl_from_IHTMLAreaElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLAreaElement_put_port(IHTMLAreaElement *iface, BSTR v)
{
    HTMLAreaElement *This = impl_from_IHTMLAreaElement(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLAreaElement_get_port(IHTMLAreaElement *iface, BSTR *p)
{
    HTMLAreaElement *This = impl_from_IHTMLAreaElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLAreaElement_put_protocol(IHTMLAreaElement *iface, BSTR v)
{
    HTMLAreaElement *This = impl_from_IHTMLAreaElement(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLAreaElement_get_protocol(IHTMLAreaElement *iface, BSTR *p)
{
    HTMLAreaElement *This = impl_from_IHTMLAreaElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLAreaElement_put_search(IHTMLAreaElement *iface, BSTR v)
{
    HTMLAreaElement *This = impl_from_IHTMLAreaElement(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLAreaElement_get_search(IHTMLAreaElement *iface, BSTR *p)
{
    HTMLAreaElement *This = impl_from_IHTMLAreaElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLAreaElement_put_hash(IHTMLAreaElement *iface, BSTR v)
{
    HTMLAreaElement *This = impl_from_IHTMLAreaElement(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLAreaElement_get_hash(IHTMLAreaElement *iface, BSTR *p)
{
    HTMLAreaElement *This = impl_from_IHTMLAreaElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLAreaElement_put_onblur(IHTMLAreaElement *iface, VARIANT v)
{
    HTMLAreaElement *This = impl_from_IHTMLAreaElement(iface);
    FIXME("(%p)->(%p)\n", This, &v);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLAreaElement_get_onblur(IHTMLAreaElement *iface, VARIANT *p)
{
    HTMLAreaElement *This = impl_from_IHTMLAreaElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLAreaElement_put_onfocus(IHTMLAreaElement *iface, VARIANT v)
{
    HTMLAreaElement *This = impl_from_IHTMLAreaElement(iface);
    FIXME("(%p)->(%p)\n", This, &v);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLAreaElement_get_onfocus(IHTMLAreaElement *iface, VARIANT *p)
{
    HTMLAreaElement *This = impl_from_IHTMLAreaElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLAreaElement_put_tabIndex(IHTMLAreaElement *iface, short v)
{
    HTMLAreaElement *This = impl_from_IHTMLAreaElement(iface);
    FIXME("(%p)->(%i)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLAreaElement_get_tabIndex(IHTMLAreaElement *iface, short *p)
{
    HTMLAreaElement *This = impl_from_IHTMLAreaElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLAreaElement_focus(IHTMLAreaElement *iface)
{
    HTMLAreaElement *This = impl_from_IHTMLAreaElement(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLAreaElement_blur(IHTMLAreaElement *iface)
{
    HTMLAreaElement *This = impl_from_IHTMLAreaElement(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static const IHTMLAreaElementVtbl HTMLAreaElementVtbl = {
    HTMLAreaElement_QueryInterface,
    HTMLAreaElement_AddRef,
    HTMLAreaElement_Release,
    HTMLAreaElement_GetTypeInfoCount,
    HTMLAreaElement_GetTypeInfo,
    HTMLAreaElement_GetIDsOfNames,
    HTMLAreaElement_Invoke,
    HTMLAreaElement_put_shape,
    HTMLAreaElement_get_shape,
    HTMLAreaElement_put_coords,
    HTMLAreaElement_get_coords,
    HTMLAreaElement_put_href,
    HTMLAreaElement_get_href,
    HTMLAreaElement_put_target,
    HTMLAreaElement_get_target,
    HTMLAreaElement_put_alt,
    HTMLAreaElement_get_alt,
    HTMLAreaElement_put_noHref,
    HTMLAreaElement_get_noHref,
    HTMLAreaElement_put_host,
    HTMLAreaElement_get_host,
    HTMLAreaElement_put_hostname,
    HTMLAreaElement_get_hostname,
    HTMLAreaElement_put_pathname,
    HTMLAreaElement_get_pathname,
    HTMLAreaElement_put_port,
    HTMLAreaElement_get_port,
    HTMLAreaElement_put_protocol,
    HTMLAreaElement_get_protocol,
    HTMLAreaElement_put_search,
    HTMLAreaElement_get_search,
    HTMLAreaElement_put_hash,
    HTMLAreaElement_get_hash,
    HTMLAreaElement_put_onblur,
    HTMLAreaElement_get_onblur,
    HTMLAreaElement_put_onfocus,
    HTMLAreaElement_get_onfocus,
    HTMLAreaElement_put_tabIndex,
    HTMLAreaElement_get_tabIndex,
    HTMLAreaElement_focus,
    HTMLAreaElement_blur
};

static inline HTMLAreaElement *impl_from_DispatchEx(DispatchEx *iface)
{
    return CONTAINING_RECORD(iface, HTMLAreaElement, element.node.event_target.dispex);
}

static void *HTMLAreaElement_query_interface(DispatchEx *dispex, REFIID riid)
{
    HTMLAreaElement *This = impl_from_DispatchEx(dispex);

    if(IsEqualGUID(&IID_IHTMLAreaElement, riid))
        return &This->IHTMLAreaElement_iface;

    return HTMLElement_query_interface(&This->element.node.event_target.dispex, riid);
}

static void HTMLAreaElement_traverse(DispatchEx *dispex, nsCycleCollectionTraversalCallback *cb)
{
    HTMLAreaElement *This = impl_from_DispatchEx(dispex);
    HTMLElement_traverse(dispex, cb);

    if(This->nsarea)
        note_cc_edge((nsISupports*)This->nsarea, "nsarea", cb);
}

static void HTMLAreaElement_unlink(DispatchEx *dispex)
{
    HTMLAreaElement *This = impl_from_DispatchEx(dispex);
    HTMLElement_unlink(dispex);
    unlink_ref(&This->nsarea);
}

static HRESULT HTMLAreaElement_handle_event(DispatchEx *dispex, DOMEvent *event, BOOL *prevent_default)
{
    HTMLAreaElement *This = impl_from_DispatchEx(dispex);
    nsAString href_str, target_str;
    nsresult nsres;

    if(event->event_id == EVENTID_CLICK) {
        nsAString_Init(&href_str, NULL);
        nsres = nsIDOMHTMLAreaElement_GetHref(This->nsarea, &href_str);
        if (NS_FAILED(nsres)) {
            ERR("Could not get area href: %08lx\n", nsres);
            goto fallback;
        }

        nsAString_Init(&target_str, NULL);
        nsres = nsIDOMHTMLAreaElement_GetTarget(This->nsarea, &target_str);
        if (NS_FAILED(nsres)) {
            ERR("Could not get area target: %08lx\n", nsres);
            goto fallback;
        }

        return handle_link_click_event(&This->element, &href_str, &target_str, event->nsevent, prevent_default);

fallback:
        nsAString_Finish(&href_str);
        nsAString_Finish(&target_str);
    }

    return HTMLElement_handle_event(&This->element.node.event_target.dispex, event, prevent_default);
}

static const NodeImplVtbl HTMLAreaElementImplVtbl = {
    .clsid                 = &CLSID_HTMLAreaElement,
    .cpc_entries           = HTMLElement_cpc,
    .clone                 = HTMLElement_clone,
    .get_attr_col          = HTMLElement_get_attr_col,
};

static const event_target_vtbl_t HTMLAreaElement_event_target_vtbl = {
    {
        HTMLELEMENT_DISPEX_VTBL_ENTRIES,
        .query_interface= HTMLAreaElement_query_interface,
        .destructor     = HTMLElement_destructor,
        .traverse       = HTMLAreaElement_traverse,
        .unlink         = HTMLAreaElement_unlink
    },
    HTMLELEMENT_EVENT_TARGET_VTBL_ENTRIES,
    .handle_event       = HTMLAreaElement_handle_event
};

static void HTMLAreaElement_init_dispex_info(dispex_data_t *info, compat_mode_t mode)
{
    static const DISPID elem_dispids[] = {
        DISPID_IHTMLELEMENT_TOSTRING,
        DISPID_UNKNOWN
    };
    HTMLElement_init_dispex_info(info, mode);
    if(mode >= COMPAT_MODE_IE9)
        dispex_info_add_dispids(info, IHTMLElement_tid, elem_dispids);
}

static const tid_t HTMLAreaElement_iface_tids[] = {
    IHTMLAreaElement_tid,
    0
};
dispex_static_data_t HTMLAreaElement_dispex = {
    .id           = PROT_HTMLAreaElement,
    .prototype_id = PROT_HTMLElement,
    .vtbl         = &HTMLAreaElement_event_target_vtbl.dispex_vtbl,
    .disp_tid     = DispHTMLAreaElement_tid,
    .iface_tids   = HTMLAreaElement_iface_tids,
    .init_info    = HTMLAreaElement_init_dispex_info,
};

HRESULT HTMLAreaElement_Create(HTMLDocumentNode *doc, nsIDOMElement *nselem, HTMLElement **elem)
{
    HTMLAreaElement *ret;
    nsresult nsres;

    ret = calloc(1, sizeof(HTMLAreaElement));
    if(!ret)
        return E_OUTOFMEMORY;

    ret->IHTMLAreaElement_iface.lpVtbl = &HTMLAreaElementVtbl;
    ret->element.node.vtbl = &HTMLAreaElementImplVtbl;

    HTMLElement_Init(&ret->element, doc, nselem, &HTMLAreaElement_dispex);

    nsres = nsIDOMElement_QueryInterface(nselem, &IID_nsIDOMHTMLAreaElement, (void**)&ret->nsarea);
    assert(nsres == NS_OK);

    *elem = &ret->element;
    return S_OK;
}
