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

struct HTMLGenericElement {
    HTMLElement element;

    IHTMLGenericElement IHTMLGenericElement_iface;
};

static inline HTMLGenericElement *impl_from_IHTMLGenericElement(IHTMLGenericElement *iface)
{
    return CONTAINING_RECORD(iface, HTMLGenericElement, IHTMLGenericElement_iface);
}

DISPEX_IDISPATCH_IMPL(HTMLGenericElement, IHTMLGenericElement,
                      impl_from_IHTMLGenericElement(iface)->element.node.event_target.dispex)

static HRESULT WINAPI HTMLGenericElement_get_recordset(IHTMLGenericElement *iface, IDispatch **p)
{
    HTMLGenericElement *This = impl_from_IHTMLGenericElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLGenericElement_namedRecordset(IHTMLGenericElement *iface,
        BSTR dataMember, VARIANT *hierarchy, IDispatch **ppRecordset)
{
    HTMLGenericElement *This = impl_from_IHTMLGenericElement(iface);
    FIXME("(%p)->(%s %p %p)\n", This, debugstr_w(dataMember), hierarchy, ppRecordset);
    return E_NOTIMPL;
}

static const IHTMLGenericElementVtbl HTMLGenericElementVtbl = {
    HTMLGenericElement_QueryInterface,
    HTMLGenericElement_AddRef,
    HTMLGenericElement_Release,
    HTMLGenericElement_GetTypeInfoCount,
    HTMLGenericElement_GetTypeInfo,
    HTMLGenericElement_GetIDsOfNames,
    HTMLGenericElement_Invoke,
    HTMLGenericElement_get_recordset,
    HTMLGenericElement_namedRecordset
};

static inline HTMLGenericElement *impl_from_DispatchEx(DispatchEx *iface)
{
    return CONTAINING_RECORD(iface, HTMLGenericElement, element.node.event_target.dispex);
}

static void *HTMLGenericElement_query_interface(DispatchEx *dispex, REFIID riid)
{
    HTMLGenericElement *This = impl_from_DispatchEx(dispex);

    if(IsEqualGUID(&IID_IHTMLGenericElement, riid))
        return &This->IHTMLGenericElement_iface;

    return HTMLElement_query_interface(&This->element.node.event_target.dispex, riid);
}

static const NodeImplVtbl HTMLGenericElementImplVtbl = {
    .clsid                 = &CLSID_HTMLGenericElement,
    .cpc_entries           = HTMLElement_cpc,
    .clone                 = HTMLElement_clone,
    .get_attr_col          = HTMLElement_get_attr_col
};

static const event_target_vtbl_t HTMLGenericElement_event_target_vtbl = {
    {
        HTMLELEMENT_DISPEX_VTBL_ENTRIES,
        .query_interface= HTMLGenericElement_query_interface,
        .destructor     = HTMLElement_destructor,
        .traverse       = HTMLElement_traverse,
        .unlink         = HTMLElement_unlink
    },
    HTMLELEMENT_EVENT_TARGET_VTBL_ENTRIES,
    .handle_event       = HTMLElement_handle_event
};

static const tid_t HTMLUnknownElement_iface_tids[] = {
    IHTMLGenericElement_tid,
    0
};

dispex_static_data_t HTMLUnknownElement_dispex = {
    .id           = PROT_HTMLUnknownElement,
    .prototype_id = PROT_HTMLElement,
    .vtbl         = &HTMLGenericElement_event_target_vtbl.dispex_vtbl,
    .disp_tid     = DispHTMLGenericElement_tid,
    .iface_tids   = HTMLUnknownElement_iface_tids,
    .init_info    = HTMLElement_init_dispex_info,
};

HRESULT HTMLGenericElement_Create(HTMLDocumentNode *doc, nsIDOMElement *nselem, HTMLElement **elem)
{
    HTMLGenericElement *ret;

    ret = calloc(1, sizeof(HTMLGenericElement));
    if(!ret)
        return E_OUTOFMEMORY;

    ret->IHTMLGenericElement_iface.lpVtbl = &HTMLGenericElementVtbl;
    ret->element.node.vtbl = &HTMLGenericElementImplVtbl;

    HTMLElement_Init(&ret->element, doc, nselem, &HTMLUnknownElement_dispex);

    *elem = &ret->element;
    return S_OK;
}
