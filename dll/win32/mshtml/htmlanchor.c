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
#include <stdio.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "ole2.h"
#include "mshtmdid.h"

#include "mshtml_private.h"
#include "htmlevent.h"
#include "binding.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(mshtml);

struct HTMLAnchorElement {
    HTMLElement element;

    IHTMLAnchorElement IHTMLAnchorElement_iface;

    nsIDOMHTMLAnchorElement *nsanchor;
};

static HRESULT navigate_href_new_window(HTMLElement *element, nsAString *href_str, const WCHAR *target)
{
    const PRUnichar *href;
    IUri *uri;
    HRESULT hres;

    if(!element->node.doc->window->base.outer_window)
        return S_OK;

    nsAString_GetData(href_str, &href);
    hres = create_relative_uri(element->node.doc->window->base.outer_window, href, &uri);
    if(FAILED(hres))
        return hres;

    hres = navigate_new_window(element->node.doc->window->base.outer_window, uri, target, NULL, NULL);
    IUri_Release(uri);
    return hres;
}

HTMLOuterWindow *get_target_window(HTMLOuterWindow *window, nsAString *target_str, BOOL *use_new_window)
{
    HTMLOuterWindow *top_window, *ret_window;
    const PRUnichar *target;
    HRESULT hres;

    *use_new_window = FALSE;

    nsAString_GetData(target_str, &target);
    TRACE("%s\n", debugstr_w(target));

    if(!*target || !wcsicmp(target, L"_self")) {
        IHTMLWindow2_AddRef(&window->base.IHTMLWindow2_iface);
        return window;
    }

    if(!wcsicmp(target, L"_top")) {
        get_top_window(window, &top_window);
        IHTMLWindow2_AddRef(&top_window->base.IHTMLWindow2_iface);
        return top_window;
    }

    if(!wcsicmp(target, L"_parent")) {
        if(!window->parent) {
            WARN("Window has no parent, treat as self\n");
            IHTMLWindow2_AddRef(&window->base.IHTMLWindow2_iface);
            return window;
        }

        IHTMLWindow2_AddRef(&window->parent->base.IHTMLWindow2_iface);
        return window->parent;
    }

    get_top_window(window, &top_window);

    hres = get_frame_by_name(top_window, target, TRUE, &ret_window);
    if(FAILED(hres) || !ret_window) {
        *use_new_window = TRUE;
        return NULL;
    }

    IHTMLWindow2_AddRef(&ret_window->base.IHTMLWindow2_iface);
    return ret_window;
}

static HRESULT navigate_href(HTMLElement *element, nsAString *href_str, nsAString *target_str)
{
    HTMLOuterWindow *window;
    BOOL use_new_window;
    const PRUnichar *href;
    HRESULT hres;

    if(!element->node.doc->window->base.outer_window)
        return S_OK;

    window = get_target_window(element->node.doc->window->base.outer_window, target_str, &use_new_window);
    if(!window) {
        if(use_new_window) {
            const PRUnichar *target;
            nsAString_GetData(target_str, &target);
            return navigate_href_new_window(element, href_str, target);
        }else {
            return S_OK;
        }
    }

    nsAString_GetData(href_str, &href);
    if(*href) {
        hres = navigate_url(window, href, window->uri_nofrag, BINDING_NAVIGATED);
    }else {
        TRACE("empty href\n");
        hres = S_OK;
    }
    IHTMLWindow2_Release(&window->base.IHTMLWindow2_iface);
    return hres;
}

HRESULT handle_link_click_event(HTMLElement *element, nsAString *href_str, nsAString *target_str,
                                nsIDOMEvent *event, BOOL *prevent_default)
{
    nsIDOMMouseEvent *mouse_event;
    INT16 button;
    nsresult nsres;
    HRESULT hres;

    TRACE("CLICK\n");

    nsres = nsIDOMEvent_QueryInterface(event, &IID_nsIDOMMouseEvent, (void**)&mouse_event);
    assert(nsres == NS_OK);

    nsres = nsIDOMMouseEvent_GetButton(mouse_event, &button);
    assert(nsres == NS_OK);

    nsIDOMMouseEvent_Release(mouse_event);

    switch(button) {
    case 0:
        *prevent_default = TRUE;
        hres = navigate_href(element, href_str, target_str);
        break;
    case 1:
        *prevent_default = TRUE;
        hres = navigate_href_new_window(element, href_str, NULL);
        break;
    default:
        *prevent_default = FALSE;
        hres = S_OK;
    }

    nsAString_Finish(href_str);
    nsAString_Finish(target_str);
    return hres;
}

static IUri *get_anchor_uri(HTMLAnchorElement *anchor)
{
    nsAString href_str;
    IUri *uri = NULL;
    nsresult nsres;

    nsAString_Init(&href_str, NULL);
    nsres = nsIDOMHTMLAnchorElement_GetHref(anchor->nsanchor, &href_str);
    if(NS_SUCCEEDED(nsres)) {
        const PRUnichar *href;

        nsAString_GetData(&href_str, &href);
        create_uri(href, 0, &uri);
    }else {
        ERR("GetHref failed: %08lx\n", nsres);
    }

    nsAString_Finish(&href_str);
    return uri;
}

static inline HTMLAnchorElement *impl_from_IHTMLAnchorElement(IHTMLAnchorElement *iface)
{
    return CONTAINING_RECORD(iface, HTMLAnchorElement, IHTMLAnchorElement_iface);
}

DISPEX_IDISPATCH_IMPL(HTMLAnchorElement, IHTMLAnchorElement,
                      impl_from_IHTMLAnchorElement(iface)->element.node.event_target.dispex)

static HRESULT WINAPI HTMLAnchorElement_put_href(IHTMLAnchorElement *iface, BSTR v)
{
    HTMLAnchorElement *This = impl_from_IHTMLAnchorElement(iface);
    nsAString nsstr;
    nsresult nsres;

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    nsAString_InitDepend(&nsstr, v);
    nsres = nsIDOMHTMLAnchorElement_SetHref(This->nsanchor, &nsstr);
    nsAString_Finish(&nsstr);
    if(NS_FAILED(nsres))
        return E_FAIL;

    return S_OK;
}

static HRESULT WINAPI HTMLAnchorElement_get_href(IHTMLAnchorElement *iface, BSTR *p)
{
    HTMLAnchorElement *This = impl_from_IHTMLAnchorElement(iface);
    nsAString href_str;
    nsresult nsres;
    HRESULT hres;

    TRACE("(%p)->(%p)\n", This, p);

    nsAString_Init(&href_str, NULL);
    nsres = nsIDOMHTMLAnchorElement_GetHref(This->nsanchor, &href_str);
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

static HRESULT WINAPI HTMLAnchorElement_put_target(IHTMLAnchorElement *iface, BSTR v)
{
    HTMLAnchorElement *This = impl_from_IHTMLAnchorElement(iface);
    nsAString nsstr;
    nsresult nsres;

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    nsAString_InitDepend(&nsstr, v);
    nsres = nsIDOMHTMLAnchorElement_SetTarget(This->nsanchor, &nsstr);
    nsAString_Finish(&nsstr);
    if(NS_FAILED(nsres))
        return E_FAIL;

    return S_OK;
}

static HRESULT WINAPI HTMLAnchorElement_get_target(IHTMLAnchorElement *iface, BSTR *p)
{
    HTMLAnchorElement *This = impl_from_IHTMLAnchorElement(iface);
    nsAString target_str;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsAString_Init(&target_str, NULL);
    nsres = nsIDOMHTMLAnchorElement_GetTarget(This->nsanchor, &target_str);

    return return_nsstr(nsres, &target_str, p);
}

static HRESULT WINAPI HTMLAnchorElement_put_rel(IHTMLAnchorElement *iface, BSTR v)
{
    HTMLAnchorElement *This = impl_from_IHTMLAnchorElement(iface);
    nsAString nsstr;
    nsresult nsres;

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    nsAString_InitDepend(&nsstr, v);
    nsres = nsIDOMHTMLAnchorElement_SetRel(This->nsanchor, &nsstr);
    nsAString_Finish(&nsstr);
    if(NS_FAILED(nsres))
        return E_FAIL;

    return S_OK;
}

static HRESULT WINAPI HTMLAnchorElement_get_rel(IHTMLAnchorElement *iface, BSTR *p)
{
    HTMLAnchorElement *This = impl_from_IHTMLAnchorElement(iface);
    nsAString nsstr;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsAString_Init(&nsstr, NULL);
    nsres = nsIDOMHTMLAnchorElement_GetRel(This->nsanchor, &nsstr);
    return return_nsstr(nsres, &nsstr, p);
}

static HRESULT WINAPI HTMLAnchorElement_put_rev(IHTMLAnchorElement *iface, BSTR v)
{
    HTMLAnchorElement *This = impl_from_IHTMLAnchorElement(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLAnchorElement_get_rev(IHTMLAnchorElement *iface, BSTR *p)
{
    HTMLAnchorElement *This = impl_from_IHTMLAnchorElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLAnchorElement_put_urn(IHTMLAnchorElement *iface, BSTR v)
{
    HTMLAnchorElement *This = impl_from_IHTMLAnchorElement(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLAnchorElement_get_urn(IHTMLAnchorElement *iface, BSTR *p)
{
    HTMLAnchorElement *This = impl_from_IHTMLAnchorElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLAnchorElement_put_Methods(IHTMLAnchorElement *iface, BSTR v)
{
    HTMLAnchorElement *This = impl_from_IHTMLAnchorElement(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLAnchorElement_get_Methods(IHTMLAnchorElement *iface, BSTR *p)
{
    HTMLAnchorElement *This = impl_from_IHTMLAnchorElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLAnchorElement_put_name(IHTMLAnchorElement *iface, BSTR v)
{
    HTMLAnchorElement *This = impl_from_IHTMLAnchorElement(iface);
    nsAString nsstr;
    nsresult nsres;

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    nsAString_InitDepend(&nsstr, v);
    nsres = nsIDOMHTMLAnchorElement_SetName(This->nsanchor, &nsstr);
    nsAString_Finish(&nsstr);
    if(NS_FAILED(nsres))
        return E_FAIL;

    return S_OK;
}

static HRESULT WINAPI HTMLAnchorElement_get_name(IHTMLAnchorElement *iface, BSTR *p)
{
    HTMLAnchorElement *This = impl_from_IHTMLAnchorElement(iface);
    nsAString name_str;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsAString_Init(&name_str, NULL);
    nsres = nsIDOMHTMLAnchorElement_GetName(This->nsanchor, &name_str);

    return return_nsstr(nsres, &name_str, p);
}

static HRESULT WINAPI HTMLAnchorElement_put_host(IHTMLAnchorElement *iface, BSTR v)
{
    HTMLAnchorElement *This = impl_from_IHTMLAnchorElement(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLAnchorElement_get_host(IHTMLAnchorElement *iface, BSTR *p)
{
    HTMLAnchorElement *This = impl_from_IHTMLAnchorElement(iface);
    nsAString str;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    /* FIXME: IE always appends port number, even if it's implicit default number */
    nsAString_InitDepend(&str, NULL);
    nsres = nsIDOMHTMLAnchorElement_GetHost(This->nsanchor, &str);
    return return_nsstr(nsres, &str, p);
}

static HRESULT WINAPI HTMLAnchorElement_put_hostname(IHTMLAnchorElement *iface, BSTR v)
{
    HTMLAnchorElement *This = impl_from_IHTMLAnchorElement(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLAnchorElement_get_hostname(IHTMLAnchorElement *iface, BSTR *p)
{
    HTMLAnchorElement *This = impl_from_IHTMLAnchorElement(iface);
    nsAString hostname_str;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsAString_Init(&hostname_str, NULL);
    nsres = nsIDOMHTMLAnchorElement_GetHostname(This->nsanchor, &hostname_str);
    return return_nsstr(nsres, &hostname_str, p);
}

static HRESULT WINAPI HTMLAnchorElement_put_pathname(IHTMLAnchorElement *iface, BSTR v)
{
    HTMLAnchorElement *This = impl_from_IHTMLAnchorElement(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLAnchorElement_get_pathname(IHTMLAnchorElement *iface, BSTR *p)
{
    HTMLAnchorElement *This = impl_from_IHTMLAnchorElement(iface);
    nsAString pathname_str;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    /* FIXME: IE prepends a slash for some protocols */
    nsAString_Init(&pathname_str, NULL);
    nsres = nsIDOMHTMLAnchorElement_GetPathname(This->nsanchor, &pathname_str);
    return return_nsstr(nsres, &pathname_str, p);
}

static HRESULT WINAPI HTMLAnchorElement_put_port(IHTMLAnchorElement *iface, BSTR v)
{
    HTMLAnchorElement *This = impl_from_IHTMLAnchorElement(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLAnchorElement_get_port(IHTMLAnchorElement *iface, BSTR *p)
{
    HTMLAnchorElement *This = impl_from_IHTMLAnchorElement(iface);
    IUri *uri;
    HRESULT hres;
    DWORD port;
    WCHAR buf[11];
    int len;
    BSTR str;

    TRACE("(%p)->(%p)\n", This, p);

    uri = get_anchor_uri(This);
    if(!uri) {
        WARN("Could not create IUri\n");
        *p = NULL;
        return S_OK;
    }

    hres = IUri_GetPort(uri, &port);
    IUri_Release(uri);
    if(FAILED(hres))
        return hres;
    if(hres != S_OK) {
        *p = NULL;
        return S_OK;
    }

    len = swprintf(buf, ARRAY_SIZE(buf), L"%u", port);
    str = SysAllocStringLen(buf, len);
    if (str)
        *p = str;
    else
        hres = E_OUTOFMEMORY;

    return hres;
}

static HRESULT WINAPI HTMLAnchorElement_put_protocol(IHTMLAnchorElement *iface, BSTR v)
{
    HTMLAnchorElement *This = impl_from_IHTMLAnchorElement(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLAnchorElement_get_protocol(IHTMLAnchorElement *iface, BSTR *p)
{
    HTMLAnchorElement *This = impl_from_IHTMLAnchorElement(iface);
    BSTR scheme;
    size_t len;
    IUri *uri;
    HRESULT hres;

    TRACE("(%p)->(%p)\n", This, p);

    uri = get_anchor_uri(This);
    if(!uri) {
        WARN("Could not create IUri\n");
        *p = NULL;
        return S_OK;
    }

    hres = IUri_GetSchemeName(uri, &scheme);
    IUri_Release(uri);
    if(FAILED(hres))
        return hres;
    if(hres != S_OK) {
        SysFreeString(scheme);
        *p = NULL;
        return S_OK;
    }

    len = SysStringLen(scheme);
    if(len) {
        *p = SysAllocStringLen(scheme, len + 1);
        if(*p)
            (*p)[len] = ':';
        else
            hres = E_OUTOFMEMORY;
    }else {
        *p = NULL;
    }
    SysFreeString(scheme);
    return hres;
}

static HRESULT WINAPI HTMLAnchorElement_put_search(IHTMLAnchorElement *iface, BSTR v)
{
    HTMLAnchorElement *This = impl_from_IHTMLAnchorElement(iface);
    nsAString nsstr;
    nsresult nsres;

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    nsAString_InitDepend(&nsstr, v);
    nsres = nsIDOMHTMLAnchorElement_SetSearch(This->nsanchor, &nsstr);
    nsAString_Finish(&nsstr);
    if(NS_FAILED(nsres))
        return E_FAIL;

    return S_OK;
}

static HRESULT WINAPI HTMLAnchorElement_get_search(IHTMLAnchorElement *iface, BSTR *p)
{
    HTMLAnchorElement *This = impl_from_IHTMLAnchorElement(iface);
    nsAString search_str;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsAString_Init(&search_str, NULL);
    nsres = nsIDOMHTMLAnchorElement_GetSearch(This->nsanchor, &search_str);
    return return_nsstr(nsres, &search_str, p);
}

static HRESULT WINAPI HTMLAnchorElement_put_hash(IHTMLAnchorElement *iface, BSTR v)
{
    HTMLAnchorElement *This = impl_from_IHTMLAnchorElement(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLAnchorElement_get_hash(IHTMLAnchorElement *iface, BSTR *p)
{
    HTMLAnchorElement *This = impl_from_IHTMLAnchorElement(iface);
    nsAString hash_str;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsAString_Init(&hash_str, NULL);
    nsres = nsIDOMHTMLAnchorElement_GetHash(This->nsanchor, &hash_str);
    return return_nsstr(nsres, &hash_str, p);
}

static HRESULT WINAPI HTMLAnchorElement_put_onblur(IHTMLAnchorElement *iface, VARIANT v)
{
    HTMLAnchorElement *This = impl_from_IHTMLAnchorElement(iface);

    TRACE("(%p)->()\n", This);

    return IHTMLElement2_put_onblur(&This->element.IHTMLElement2_iface, v);
}

static HRESULT WINAPI HTMLAnchorElement_get_onblur(IHTMLAnchorElement *iface, VARIANT *p)
{
    HTMLAnchorElement *This = impl_from_IHTMLAnchorElement(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return IHTMLElement2_get_onblur(&This->element.IHTMLElement2_iface, p);
}

static HRESULT WINAPI HTMLAnchorElement_put_onfocus(IHTMLAnchorElement *iface, VARIANT v)
{
    HTMLAnchorElement *This = impl_from_IHTMLAnchorElement(iface);

    TRACE("(%p)->()\n", This);

    return IHTMLElement2_put_onfocus(&This->element.IHTMLElement2_iface, v);
}

static HRESULT WINAPI HTMLAnchorElement_get_onfocus(IHTMLAnchorElement *iface, VARIANT *p)
{
    HTMLAnchorElement *This = impl_from_IHTMLAnchorElement(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return IHTMLElement2_get_onfocus(&This->element.IHTMLElement2_iface, p);
}

static HRESULT WINAPI HTMLAnchorElement_put_accessKey(IHTMLAnchorElement *iface, BSTR v)
{
    HTMLAnchorElement *This = impl_from_IHTMLAnchorElement(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    return IHTMLElement2_put_accessKey(&This->element.IHTMLElement2_iface, v);
}

static HRESULT WINAPI HTMLAnchorElement_get_accessKey(IHTMLAnchorElement *iface, BSTR *p)
{
    HTMLAnchorElement *This = impl_from_IHTMLAnchorElement(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return IHTMLElement2_get_accessKey(&This->element.IHTMLElement2_iface, p);
}

static HRESULT WINAPI HTMLAnchorElement_get_protocolLong(IHTMLAnchorElement *iface, BSTR *p)
{
    HTMLAnchorElement *This = impl_from_IHTMLAnchorElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLAnchorElement_get_mimeType(IHTMLAnchorElement *iface, BSTR *p)
{
    HTMLAnchorElement *This = impl_from_IHTMLAnchorElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLAnchorElement_get_nameProp(IHTMLAnchorElement *iface, BSTR *p)
{
    HTMLAnchorElement *This = impl_from_IHTMLAnchorElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLAnchorElement_put_tabIndex(IHTMLAnchorElement *iface, short v)
{
    HTMLAnchorElement *This = impl_from_IHTMLAnchorElement(iface);

    TRACE("(%p)->()\n", This);

    return IHTMLElement2_put_tabIndex(&This->element.IHTMLElement2_iface, v);
}

static HRESULT WINAPI HTMLAnchorElement_get_tabIndex(IHTMLAnchorElement *iface, short *p)
{
    HTMLAnchorElement *This = impl_from_IHTMLAnchorElement(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return IHTMLElement2_get_tabIndex(&This->element.IHTMLElement2_iface, p);
}

static HRESULT WINAPI HTMLAnchorElement_focus(IHTMLAnchorElement *iface)
{
    HTMLAnchorElement *This = impl_from_IHTMLAnchorElement(iface);

    TRACE("(%p)\n", This);

    return IHTMLElement2_focus(&This->element.IHTMLElement2_iface);
}

static HRESULT WINAPI HTMLAnchorElement_blur(IHTMLAnchorElement *iface)
{
    HTMLAnchorElement *This = impl_from_IHTMLAnchorElement(iface);

    TRACE("(%p)\n", This);

    return IHTMLElement2_blur(&This->element.IHTMLElement2_iface);
}

static const IHTMLAnchorElementVtbl HTMLAnchorElementVtbl = {
    HTMLAnchorElement_QueryInterface,
    HTMLAnchorElement_AddRef,
    HTMLAnchorElement_Release,
    HTMLAnchorElement_GetTypeInfoCount,
    HTMLAnchorElement_GetTypeInfo,
    HTMLAnchorElement_GetIDsOfNames,
    HTMLAnchorElement_Invoke,
    HTMLAnchorElement_put_href,
    HTMLAnchorElement_get_href,
    HTMLAnchorElement_put_target,
    HTMLAnchorElement_get_target,
    HTMLAnchorElement_put_rel,
    HTMLAnchorElement_get_rel,
    HTMLAnchorElement_put_rev,
    HTMLAnchorElement_get_rev,
    HTMLAnchorElement_put_urn,
    HTMLAnchorElement_get_urn,
    HTMLAnchorElement_put_Methods,
    HTMLAnchorElement_get_Methods,
    HTMLAnchorElement_put_name,
    HTMLAnchorElement_get_name,
    HTMLAnchorElement_put_host,
    HTMLAnchorElement_get_host,
    HTMLAnchorElement_put_hostname,
    HTMLAnchorElement_get_hostname,
    HTMLAnchorElement_put_pathname,
    HTMLAnchorElement_get_pathname,
    HTMLAnchorElement_put_port,
    HTMLAnchorElement_get_port,
    HTMLAnchorElement_put_protocol,
    HTMLAnchorElement_get_protocol,
    HTMLAnchorElement_put_search,
    HTMLAnchorElement_get_search,
    HTMLAnchorElement_put_hash,
    HTMLAnchorElement_get_hash,
    HTMLAnchorElement_put_onblur,
    HTMLAnchorElement_get_onblur,
    HTMLAnchorElement_put_onfocus,
    HTMLAnchorElement_get_onfocus,
    HTMLAnchorElement_put_accessKey,
    HTMLAnchorElement_get_accessKey,
    HTMLAnchorElement_get_protocolLong,
    HTMLAnchorElement_get_mimeType,
    HTMLAnchorElement_get_nameProp,
    HTMLAnchorElement_put_tabIndex,
    HTMLAnchorElement_get_tabIndex,
    HTMLAnchorElement_focus,
    HTMLAnchorElement_blur
};

static inline HTMLAnchorElement *impl_from_DispatchEx(DispatchEx *iface)
{
    return CONTAINING_RECORD(iface, HTMLAnchorElement, element.node.event_target.dispex);
}

static void *HTMLAnchorElement_query_interface(DispatchEx *dispex, REFIID riid)
{
    HTMLAnchorElement *This = impl_from_DispatchEx(dispex);

    if(IsEqualGUID(&IID_IHTMLAnchorElement, riid))
        return &This->IHTMLAnchorElement_iface;

    return HTMLElement_query_interface(&This->element.node.event_target.dispex, riid);
}

static void HTMLAnchorElement_traverse(DispatchEx *dispex, nsCycleCollectionTraversalCallback *cb)
{
    HTMLAnchorElement *This = impl_from_DispatchEx(dispex);
    HTMLElement_traverse(dispex, cb);

    if(This->nsanchor)
        note_cc_edge((nsISupports*)This->nsanchor, "nsanchor", cb);
}

static void HTMLAnchorElement_unlink(DispatchEx *dispex)
{
    HTMLAnchorElement *This = impl_from_DispatchEx(dispex);
    HTMLElement_unlink(dispex);
    unlink_ref(&This->nsanchor);
}

static HRESULT HTMLAnchorElement_handle_event(DispatchEx *dispex, DOMEvent *event, BOOL *prevent_default)
{
    HTMLAnchorElement *This = impl_from_DispatchEx(dispex);
    nsAString href_str, target_str;
    nsresult nsres;

    if(event->event_id == EVENTID_CLICK) {
        nsAString_Init(&href_str, NULL);
        nsres = nsIDOMHTMLAnchorElement_GetHref(This->nsanchor, &href_str);
        if (NS_FAILED(nsres)) {
            ERR("Could not get anchor href: %08lx\n", nsres);
            goto fallback;
        }

        nsAString_Init(&target_str, NULL);
        nsres = nsIDOMHTMLAnchorElement_GetTarget(This->nsanchor, &target_str);
        if (NS_FAILED(nsres)) {
            ERR("Could not get anchor target: %08lx\n", nsres);
            goto fallback;
        }

        return handle_link_click_event(&This->element, &href_str, &target_str, event->nsevent, prevent_default);

fallback:
        nsAString_Finish(&href_str);
        nsAString_Finish(&target_str);
    }

    return HTMLElement_handle_event(&This->element.node.event_target.dispex, event, prevent_default);
}

static const NodeImplVtbl HTMLAnchorElementImplVtbl = {
    .clsid                 = &CLSID_HTMLAnchorElement,
    .cpc_entries           = HTMLElement_cpc,
    .clone                 = HTMLElement_clone,
    .get_attr_col          = HTMLElement_get_attr_col,
};

static const event_target_vtbl_t HTMLAnchorElement_event_target_vtbl = {
    {
        HTMLELEMENT_DISPEX_VTBL_ENTRIES,
        .query_interface= HTMLAnchorElement_query_interface,
        .destructor     = HTMLElement_destructor,
        .traverse       = HTMLAnchorElement_traverse,
        .unlink         = HTMLAnchorElement_unlink
    },
    HTMLELEMENT_EVENT_TARGET_VTBL_ENTRIES,
    .handle_event       = HTMLAnchorElement_handle_event
};

static void HTMLAnchorElement_init_dispex_info(dispex_data_t *info, compat_mode_t mode)
{
    static const DISPID elem_dispids[] = {
        DISPID_IHTMLELEMENT_TOSTRING,
        DISPID_UNKNOWN
    };
    HTMLElement_init_dispex_info(info, mode);
    if(mode >= COMPAT_MODE_IE9)
        dispex_info_add_dispids(info, IHTMLElement_tid, elem_dispids);
}

static const tid_t HTMLAnchorElement_iface_tids[] = {
    IHTMLAnchorElement_tid,
    0
};

dispex_static_data_t HTMLAnchorElement_dispex = {
    .id           = PROT_HTMLAnchorElement,
    .prototype_id = PROT_HTMLElement,
    .vtbl         = &HTMLAnchorElement_event_target_vtbl.dispex_vtbl,
    .disp_tid     = DispHTMLAnchorElement_tid,
    .iface_tids   = HTMLAnchorElement_iface_tids,
    .init_info    = HTMLAnchorElement_init_dispex_info,
};

HRESULT HTMLAnchorElement_Create(HTMLDocumentNode *doc, nsIDOMElement *nselem, HTMLElement **elem)
{
    HTMLAnchorElement *ret;
    nsresult nsres;

    ret = calloc(1, sizeof(HTMLAnchorElement));
    if(!ret)
        return E_OUTOFMEMORY;

    ret->IHTMLAnchorElement_iface.lpVtbl = &HTMLAnchorElementVtbl;
    ret->element.node.vtbl = &HTMLAnchorElementImplVtbl;

    HTMLElement_Init(&ret->element, doc, nselem, &HTMLAnchorElement_dispex);

    nsres = nsIDOMElement_QueryInterface(nselem, &IID_nsIDOMHTMLAnchorElement, (void**)&ret->nsanchor);
    assert(nsres == NS_OK);

    *elem = &ret->element;
    return S_OK;
}
