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

#define WIN32_NO_STATUS
#define _INC_WINDOWS

#include <stdarg.h>

#define COBJMACROS

#include <windef.h>
#include <winbase.h>
//#include "winuser.h"
#include <ole2.h>

#include <wine/debug.h>

#include "mshtml_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(mshtml);

static const char *debugstr_cp_guid(REFIID riid)
{
#define X(x) \
    if(IsEqualGUID(riid, &x)) \
        return #x

    X(IID_IPropertyNotifySink);
    X(DIID_HTMLDocumentEvents);
    X(DIID_HTMLDocumentEvents2);
    X(DIID_HTMLTableEvents);
    X(DIID_HTMLTextContainerEvents);

#undef X

    return debugstr_guid(riid);
}

static inline ConnectionPoint *impl_from_IConnectionPoint(IConnectionPoint *iface)
{
    return CONTAINING_RECORD(iface, ConnectionPoint, IConnectionPoint_iface);
}

static HRESULT WINAPI ConnectionPoint_QueryInterface(IConnectionPoint *iface,
                                                     REFIID riid, LPVOID *ppv)
{
    ConnectionPoint *This = impl_from_IConnectionPoint(iface);

    *ppv = NULL;

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = &This->IConnectionPoint_iface;
    }else if(IsEqualGUID(&IID_IConnectionPoint, riid)) {
        TRACE("(%p)->(IID_IConnectionPoint %p)\n", This, ppv);
        *ppv = &This->IConnectionPoint_iface;
    }

    if(*ppv) {
        IUnknown_AddRef((IUnknown*)*ppv);
        return S_OK;
    }

    WARN("Unsupported interface %s\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI ConnectionPoint_AddRef(IConnectionPoint *iface)
{
    ConnectionPoint *This = impl_from_IConnectionPoint(iface);
    return IConnectionPointContainer_AddRef(&This->container->IConnectionPointContainer_iface);
}

static ULONG WINAPI ConnectionPoint_Release(IConnectionPoint *iface)
{
    ConnectionPoint *This = impl_from_IConnectionPoint(iface);
    return IConnectionPointContainer_Release(&This->container->IConnectionPointContainer_iface);
}

static HRESULT WINAPI ConnectionPoint_GetConnectionInterface(IConnectionPoint *iface, IID *pIID)
{
    ConnectionPoint *This = impl_from_IConnectionPoint(iface);

    TRACE("(%p)->(%p)\n", This, pIID);

    if(!pIID)
        return E_POINTER;

    *pIID = *This->iid;
    return S_OK;
}

static HRESULT WINAPI ConnectionPoint_GetConnectionPointContainer(IConnectionPoint *iface,
        IConnectionPointContainer **ppCPC)
{
    ConnectionPoint *This = impl_from_IConnectionPoint(iface);

    TRACE("(%p)->(%p)\n", This, ppCPC);

    if(!ppCPC)
        return E_POINTER;

    *ppCPC = &This->container->IConnectionPointContainer_iface;
    IConnectionPointContainer_AddRef(*ppCPC);
    return S_OK;
}

static HRESULT WINAPI ConnectionPoint_Advise(IConnectionPoint *iface, IUnknown *pUnkSink,
                                             DWORD *pdwCookie)
{
    ConnectionPoint *This = impl_from_IConnectionPoint(iface);
    IUnknown *sink;
    DWORD i;
    HRESULT hres;

    TRACE("(%p)->(%p %p)\n", This, pUnkSink, pdwCookie);

    hres = IUnknown_QueryInterface(pUnkSink, This->iid, (void**)&sink);
    if(FAILED(hres) && !IsEqualGUID(&IID_IPropertyNotifySink, This->iid))
        hres = IUnknown_QueryInterface(pUnkSink, &IID_IDispatch, (void**)&sink);
    if(FAILED(hres))
        return CONNECT_E_CANNOTCONNECT;

    if(This->sinks) {
        for(i=0; i<This->sinks_size; i++) {
            if(!This->sinks[i].unk)
                break;
        }

        if(i == This->sinks_size)
            This->sinks = heap_realloc(This->sinks,(++This->sinks_size)*sizeof(*This->sinks));
    }else {
        This->sinks = heap_alloc(sizeof(*This->sinks));
        This->sinks_size = 1;
        i = 0;
    }

    This->sinks[i].unk = sink;
    if(pdwCookie)
        *pdwCookie = i+1;

    if(!i && This->data && This->data->on_advise)
        This->data->on_advise(This->container->outer, This->data);

    return S_OK;
}

static HRESULT WINAPI ConnectionPoint_Unadvise(IConnectionPoint *iface, DWORD dwCookie)
{
    ConnectionPoint *This = impl_from_IConnectionPoint(iface);
    TRACE("(%p)->(%d)\n", This, dwCookie);

    if(!dwCookie || dwCookie > This->sinks_size || !This->sinks[dwCookie-1].unk)
        return CONNECT_E_NOCONNECTION;

    IUnknown_Release(This->sinks[dwCookie-1].unk);
    This->sinks[dwCookie-1].unk = NULL;

    return S_OK;
}

static HRESULT WINAPI ConnectionPoint_EnumConnections(IConnectionPoint *iface,
                                                      IEnumConnections **ppEnum)
{
    ConnectionPoint *This = impl_from_IConnectionPoint(iface);
    FIXME("(%p)->(%p)\n", This, ppEnum);
    return E_NOTIMPL;
}

static const IConnectionPointVtbl ConnectionPointVtbl =
{
    ConnectionPoint_QueryInterface,
    ConnectionPoint_AddRef,
    ConnectionPoint_Release,
    ConnectionPoint_GetConnectionInterface,
    ConnectionPoint_GetConnectionPointContainer,
    ConnectionPoint_Advise,
    ConnectionPoint_Unadvise,
    ConnectionPoint_EnumConnections
};

static void ConnectionPoint_Init(ConnectionPoint *cp, ConnectionPointContainer *container, REFIID riid, cp_static_data_t *data)
{
    cp->IConnectionPoint_iface.lpVtbl = &ConnectionPointVtbl;
    cp->container = container;
    cp->sinks = NULL;
    cp->sinks_size = 0;
    cp->iid = riid;
    cp->data = data;
}

static void ConnectionPoint_Destroy(ConnectionPoint *This)
{
    DWORD i;

    for(i=0; i<This->sinks_size; i++) {
        if(This->sinks[i].unk)
            IUnknown_Release(This->sinks[i].unk);
    }

    heap_free(This->sinks);
}

static ConnectionPoint *get_cp(ConnectionPointContainer *container, REFIID riid, BOOL do_create)
{
    const cpc_entry_t *iter;
    unsigned idx, i;

    for(iter = container->cp_entries; iter->riid; iter++) {
        if(IsEqualGUID(iter->riid, riid))
            break;
    }
    if(!iter->riid)
        return NULL;
    idx = iter - container->cp_entries;

    if(!container->cps) {
        if(!do_create)
            return NULL;

        while(iter->riid)
            iter++;
        container->cps = heap_alloc((iter - container->cp_entries) * sizeof(*container->cps));
        if(!container->cps)
            return NULL;

        for(i=0; container->cp_entries[i].riid; i++)
            ConnectionPoint_Init(container->cps+i, container, container->cp_entries[i].riid, container->cp_entries[i].desc);
    }

    return container->cps+idx;
}

void call_property_onchanged(ConnectionPointContainer *container, DISPID dispid)
{
    ConnectionPoint *cp;
    DWORD i;

    cp = get_cp(container, &IID_IPropertyNotifySink, FALSE);
    if(!cp)
        return;

    for(i=0; i<cp->sinks_size; i++) {
        if(cp->sinks[i].propnotif)
            IPropertyNotifySink_OnChanged(cp->sinks[i].propnotif, dispid);
    }
}

static inline ConnectionPointContainer *impl_from_IConnectionPointContainer(IConnectionPointContainer *iface)
{
    return CONTAINING_RECORD(iface, ConnectionPointContainer, IConnectionPointContainer_iface);
}

static HRESULT WINAPI ConnectionPointContainer_QueryInterface(IConnectionPointContainer *iface,
                                                              REFIID riid, void **ppv)
{
    ConnectionPointContainer *This = impl_from_IConnectionPointContainer(iface);
    return IUnknown_QueryInterface(This->outer, riid, ppv);
}

static ULONG WINAPI ConnectionPointContainer_AddRef(IConnectionPointContainer *iface)
{
    ConnectionPointContainer *This = impl_from_IConnectionPointContainer(iface);
    return IUnknown_AddRef(This->outer);
}

static ULONG WINAPI ConnectionPointContainer_Release(IConnectionPointContainer *iface)
{
    ConnectionPointContainer *This = impl_from_IConnectionPointContainer(iface);
    return IUnknown_Release(This->outer);
}

static HRESULT WINAPI ConnectionPointContainer_EnumConnectionPoints(IConnectionPointContainer *iface,
        IEnumConnectionPoints **ppEnum)
{
    ConnectionPointContainer *This = impl_from_IConnectionPointContainer(iface);
    FIXME("(%p)->(%p)\n", This, ppEnum);
    return E_NOTIMPL;
}

static HRESULT WINAPI ConnectionPointContainer_FindConnectionPoint(IConnectionPointContainer *iface,
        REFIID riid, IConnectionPoint **ppCP)
{
    ConnectionPointContainer *This = impl_from_IConnectionPointContainer(iface);
    ConnectionPoint *cp;

    TRACE("(%p)->(%s %p)\n", This, debugstr_cp_guid(riid), ppCP);

    if(This->forward_container)
        return IConnectionPointContainer_FindConnectionPoint(&This->forward_container->IConnectionPointContainer_iface,
                riid, ppCP);

    cp = get_cp(This, riid, TRUE);
    if(!cp) {
        FIXME("unsupported riid %s\n", debugstr_cp_guid(riid));
        *ppCP = NULL;
        return CONNECT_E_NOCONNECTION;
    }

    *ppCP = &cp->IConnectionPoint_iface;
    IConnectionPoint_AddRef(*ppCP);
    return S_OK;
}

static const IConnectionPointContainerVtbl ConnectionPointContainerVtbl = {
    ConnectionPointContainer_QueryInterface,
    ConnectionPointContainer_AddRef,
    ConnectionPointContainer_Release,
    ConnectionPointContainer_EnumConnectionPoints,
    ConnectionPointContainer_FindConnectionPoint
};

void ConnectionPointContainer_Init(ConnectionPointContainer *This, IUnknown *outer, const cpc_entry_t *cp_entries)
{
    This->IConnectionPointContainer_iface.lpVtbl = &ConnectionPointContainerVtbl;
    This->cp_entries = cp_entries;
    This->cps = NULL;
    This->outer = outer;
    This->forward_container = NULL;
}

void ConnectionPointContainer_Destroy(ConnectionPointContainer *This)
{
    unsigned i;

    if(!This->cps)
        return;

    for(i=0; This->cp_entries[i].riid; i++)
        ConnectionPoint_Destroy(This->cps+i);
    heap_free(This->cps);
}
