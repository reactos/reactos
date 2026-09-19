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
#include "htmlevent.h"

WINE_DEFAULT_DEBUG_CHANNEL(mshtml);

struct HTMLTextAreaElement {
    HTMLElement element;

    IHTMLTextAreaElement IHTMLTextAreaElement_iface;

    nsIDOMHTMLTextAreaElement *nstextarea;
};

static inline HTMLTextAreaElement *impl_from_IHTMLTextAreaElement(IHTMLTextAreaElement *iface)
{
    return CONTAINING_RECORD(iface, HTMLTextAreaElement, IHTMLTextAreaElement_iface);
}

DISPEX_IDISPATCH_IMPL(HTMLTextAreaElement, IHTMLTextAreaElement,
                      impl_from_IHTMLTextAreaElement(iface)->element.node.event_target.dispex)

static HRESULT WINAPI HTMLTextAreaElement_get_type(IHTMLTextAreaElement *iface, BSTR *p)
{
    HTMLTextAreaElement *This = impl_from_IHTMLTextAreaElement(iface);

    TRACE("(%p)->(%p)\n", This, p);

    *p = SysAllocString(L"textarea");
    if(!*p)
        return E_OUTOFMEMORY;
    return S_OK;
}

static HRESULT WINAPI HTMLTextAreaElement_put_value(IHTMLTextAreaElement *iface, BSTR v)
{
    HTMLTextAreaElement *This = impl_from_IHTMLTextAreaElement(iface);
    nsAString value_str;
    nsresult nsres;

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    nsAString_InitDepend(&value_str, v);
    nsres = nsIDOMHTMLTextAreaElement_SetValue(This->nstextarea, &value_str);
    nsAString_Finish(&value_str);
    if(NS_FAILED(nsres)) {
        ERR("SetValue failed: %08lx\n", nsres);
        return E_FAIL;
    }

    return S_OK;
}

static HRESULT WINAPI HTMLTextAreaElement_get_value(IHTMLTextAreaElement *iface, BSTR *p)
{
    HTMLTextAreaElement *This = impl_from_IHTMLTextAreaElement(iface);
    nsAString value_str;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsAString_Init(&value_str, NULL);
    nsres = nsIDOMHTMLTextAreaElement_GetValue(This->nstextarea, &value_str);
    return return_nsstr(nsres, &value_str, p);
}

static HRESULT WINAPI HTMLTextAreaElement_put_name(IHTMLTextAreaElement *iface, BSTR v)
{
    HTMLTextAreaElement *This = impl_from_IHTMLTextAreaElement(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTextAreaElement_get_name(IHTMLTextAreaElement *iface, BSTR *p)
{
    HTMLTextAreaElement *This = impl_from_IHTMLTextAreaElement(iface);
    nsAString name_str;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsAString_Init(&name_str, NULL);
    nsres = nsIDOMHTMLTextAreaElement_GetName(This->nstextarea, &name_str);
    return return_nsstr(nsres, &name_str, p);
}

static HRESULT WINAPI HTMLTextAreaElement_put_status(IHTMLTextAreaElement *iface, VARIANT v)
{
    HTMLTextAreaElement *This = impl_from_IHTMLTextAreaElement(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTextAreaElement_get_status(IHTMLTextAreaElement *iface, VARIANT *p)
{
    HTMLTextAreaElement *This = impl_from_IHTMLTextAreaElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTextAreaElement_put_disabled(IHTMLTextAreaElement *iface, VARIANT_BOOL v)
{
    HTMLTextAreaElement *This = impl_from_IHTMLTextAreaElement(iface);
    FIXME("(%p)->(%x)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTextAreaElement_get_disabled(IHTMLTextAreaElement *iface, VARIANT_BOOL *p)
{
    HTMLTextAreaElement *This = impl_from_IHTMLTextAreaElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTextAreaElement_get_form(IHTMLTextAreaElement *iface, IHTMLFormElement **p)
{
    HTMLTextAreaElement *This = impl_from_IHTMLTextAreaElement(iface);
    nsIDOMHTMLFormElement *nsform;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsres = nsIDOMHTMLTextAreaElement_GetForm(This->nstextarea, &nsform);
    return return_nsform(nsres, nsform, p);
}

static HRESULT WINAPI HTMLTextAreaElement_put_defaultValue(IHTMLTextAreaElement *iface, BSTR v)
{
    HTMLTextAreaElement *This = impl_from_IHTMLTextAreaElement(iface);
    nsAString nsstr;
    nsresult nsres;

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    nsAString_InitDepend(&nsstr, v);
    nsres = nsIDOMHTMLTextAreaElement_SetDefaultValue(This->nstextarea, &nsstr);
    nsAString_Finish(&nsstr);
    return NS_SUCCEEDED(nsres) ? S_OK : E_FAIL;
}

static HRESULT WINAPI HTMLTextAreaElement_get_defaultValue(IHTMLTextAreaElement *iface, BSTR *p)
{
    HTMLTextAreaElement *This = impl_from_IHTMLTextAreaElement(iface);
    nsAString nsstr;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsAString_Init(&nsstr, NULL);
    nsres = nsIDOMHTMLTextAreaElement_GetDefaultValue(This->nstextarea, &nsstr);
    return return_nsstr(nsres, &nsstr, p);
}

static HRESULT WINAPI HTMLTextAreaElement_select(IHTMLTextAreaElement *iface)
{
    HTMLTextAreaElement *This = impl_from_IHTMLTextAreaElement(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTextAreaElement_put_onchange(IHTMLTextAreaElement *iface, VARIANT v)
{
    HTMLTextAreaElement *This = impl_from_IHTMLTextAreaElement(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTextAreaElement_get_onchange(IHTMLTextAreaElement *iface, VARIANT *p)
{
    HTMLTextAreaElement *This = impl_from_IHTMLTextAreaElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTextAreaElement_put_onselect(IHTMLTextAreaElement *iface, VARIANT v)
{
    HTMLTextAreaElement *This = impl_from_IHTMLTextAreaElement(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTextAreaElement_get_onselect(IHTMLTextAreaElement *iface, VARIANT *p)
{
    HTMLTextAreaElement *This = impl_from_IHTMLTextAreaElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTextAreaElement_put_readOnly(IHTMLTextAreaElement *iface, VARIANT_BOOL v)
{
    HTMLTextAreaElement *This = impl_from_IHTMLTextAreaElement(iface);
    nsresult nsres;

    TRACE("(%p)->(%x)\n", This, v);

    nsres = nsIDOMHTMLTextAreaElement_SetReadOnly(This->nstextarea, v != VARIANT_FALSE);
    if(NS_FAILED(nsres)) {
        ERR("SetReadOnly failed: %08lx\n", nsres);
        return E_FAIL;
    }

    return S_OK;
}

static HRESULT WINAPI HTMLTextAreaElement_get_readOnly(IHTMLTextAreaElement *iface, VARIANT_BOOL *p)
{
    HTMLTextAreaElement *This = impl_from_IHTMLTextAreaElement(iface);
    cpp_bool b;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsres = nsIDOMHTMLTextAreaElement_GetReadOnly(This->nstextarea, &b);
    if(NS_FAILED(nsres)) {
        ERR("GetReadOnly failed: %08lx\n", nsres);
        return E_FAIL;
    }

    *p = variant_bool(b);
    return S_OK;
}

static HRESULT WINAPI HTMLTextAreaElement_put_rows(IHTMLTextAreaElement *iface, LONG v)
{
    HTMLTextAreaElement *This = impl_from_IHTMLTextAreaElement(iface);
    FIXME("(%p)->(%ld)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTextAreaElement_get_rows(IHTMLTextAreaElement *iface, LONG *p)
{
    HTMLTextAreaElement *This = impl_from_IHTMLTextAreaElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTextAreaElement_put_cols(IHTMLTextAreaElement *iface, LONG v)
{
    HTMLTextAreaElement *This = impl_from_IHTMLTextAreaElement(iface);
    FIXME("(%p)->(%ld)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTextAreaElement_get_cols(IHTMLTextAreaElement *iface, LONG *p)
{
    HTMLTextAreaElement *This = impl_from_IHTMLTextAreaElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTextAreaElement_put_wrap(IHTMLTextAreaElement *iface, BSTR v)
{
    HTMLTextAreaElement *This = impl_from_IHTMLTextAreaElement(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTextAreaElement_get_wrap(IHTMLTextAreaElement *iface, BSTR *p)
{
    HTMLTextAreaElement *This = impl_from_IHTMLTextAreaElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLTextAreaElement_createTextRange(IHTMLTextAreaElement *iface,
                                                          IHTMLTxtRange **range)
{
    HTMLTextAreaElement *This = impl_from_IHTMLTextAreaElement(iface);
    FIXME("(%p)->(%p)\n", This, range);
    return E_NOTIMPL;
}

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

static inline HTMLTextAreaElement *impl_from_HTMLDOMNode(HTMLDOMNode *iface)
{
    return CONTAINING_RECORD(iface, HTMLTextAreaElement, element.node);
}

static HRESULT HTMLTextAreaElementImpl_put_disabled(HTMLDOMNode *iface, VARIANT_BOOL v)
{
    HTMLTextAreaElement *This = impl_from_HTMLDOMNode(iface);
    return IHTMLTextAreaElement_put_disabled(&This->IHTMLTextAreaElement_iface, v);
}

static HRESULT HTMLTextAreaElementImpl_get_disabled(HTMLDOMNode *iface, VARIANT_BOOL *p)
{
    HTMLTextAreaElement *This = impl_from_HTMLDOMNode(iface);
    return IHTMLTextAreaElement_get_disabled(&This->IHTMLTextAreaElement_iface, p);
}

static BOOL HTMLTextAreaElement_is_text_edit(HTMLDOMNode *iface)
{
    return TRUE;
}

static inline HTMLTextAreaElement *impl_from_DispatchEx(DispatchEx *iface)
{
    return CONTAINING_RECORD(iface, HTMLTextAreaElement, element.node.event_target.dispex);
}

static void *HTMLTextAreaElement_query_interface(DispatchEx *dispex, REFIID riid)
{
    HTMLTextAreaElement *This = impl_from_DispatchEx(dispex);

    if(IsEqualGUID(&IID_IHTMLTextAreaElement, riid))
        return &This->IHTMLTextAreaElement_iface;

    return HTMLElement_query_interface(&This->element.node.event_target.dispex, riid);
}

static void HTMLTextAreaElement_traverse(DispatchEx *dispex, nsCycleCollectionTraversalCallback *cb)
{
    HTMLTextAreaElement *This = impl_from_DispatchEx(dispex);
    HTMLElement_traverse(dispex, cb);

    if(This->nstextarea)
        note_cc_edge((nsISupports*)This->nstextarea, "nstextarea", cb);
}

static void HTMLTextAreaElement_unlink(DispatchEx *dispex)
{
    HTMLTextAreaElement *This = impl_from_DispatchEx(dispex);
    HTMLElement_unlink(dispex);
    unlink_ref(&This->nstextarea);
}

static const NodeImplVtbl HTMLTextAreaElementImplVtbl = {
    .clsid                 = &CLSID_HTMLTextAreaElement,
    .cpc_entries           = HTMLElement_cpc,
    .clone                 = HTMLElement_clone,
    .get_attr_col          = HTMLElement_get_attr_col,
    .put_disabled          = HTMLTextAreaElementImpl_put_disabled,
    .get_disabled          = HTMLTextAreaElementImpl_get_disabled,
    .is_text_edit          = HTMLTextAreaElement_is_text_edit
};

static const event_target_vtbl_t HTMLTextAreaElement_event_target_vtbl = {
    {
        HTMLELEMENT_DISPEX_VTBL_ENTRIES,
        .query_interface= HTMLTextAreaElement_query_interface,
        .destructor     = HTMLElement_destructor,
        .traverse       = HTMLTextAreaElement_traverse,
        .unlink         = HTMLTextAreaElement_unlink
    },
    HTMLELEMENT_EVENT_TARGET_VTBL_ENTRIES,
    .handle_event       = HTMLElement_handle_event
};

static const tid_t HTMLTextAreaElement_iface_tids[] = {
    IHTMLTextAreaElement_tid,
    0
};

dispex_static_data_t HTMLTextAreaElement_dispex = {
    .id           = PROT_HTMLTextAreaElement,
    .prototype_id = PROT_HTMLElement,
    .vtbl         = &HTMLTextAreaElement_event_target_vtbl.dispex_vtbl,
    .disp_tid     = DispHTMLTextAreaElement_tid,
    .iface_tids   = HTMLTextAreaElement_iface_tids,
    .init_info    = HTMLElement_init_dispex_info,
};

HRESULT HTMLTextAreaElement_Create(HTMLDocumentNode *doc, nsIDOMElement *nselem, HTMLElement **elem)
{
    HTMLTextAreaElement *ret;
    nsresult nsres;

    ret = calloc(1, sizeof(HTMLTextAreaElement));
    if(!ret)
        return E_OUTOFMEMORY;

    ret->IHTMLTextAreaElement_iface.lpVtbl = &HTMLTextAreaElementVtbl;
    ret->element.node.vtbl = &HTMLTextAreaElementImplVtbl;

    HTMLElement_Init(&ret->element, doc, nselem, &HTMLTextAreaElement_dispex);

    nsres = nsIDOMElement_QueryInterface(nselem, &IID_nsIDOMHTMLTextAreaElement, (void**)&ret->nstextarea);
    assert(nsres == NS_OK);

    *elem = &ret->element;
    return S_OK;
}
