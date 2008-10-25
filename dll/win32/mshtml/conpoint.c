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

WINE_DEFAULT_DEBUG_CHANNEL(mshtml);

#define CONPOINT(x) ((IConnectionPoint*) &(x)->lpConnectionPointVtbl);

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

void call_property_onchanged(ConnectionPoint *This, DISPID dispid)
{
    DWORD i;

    for(i=0; i<This->sinks_size; i++) {
        if(This->sinks[i].propnotif)
            IPropertyNotifySink_OnChanged(This->sinks[i].propnotif, dispid);
    }
}

#define CONPOINT_THIS(iface) DEFINE_THIS(ConnectionPoint, ConnectionPoint, iface)

static HRESULT WINAPI ConnectionPoint_QueryInterface(IConnectionPoint *iface,
                                                     REFIID riid, LPVOID *ppv)
{
    ConnectionPoint *This = CONPOINT_THIS(iface);

    *ppv = NULL;

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = CONPOINT(This);
    }else if(IsEqualGUID(&IID_IConnectionPoint, riid)) {
        TRACE("(%p)->(IID_IConnectionPoint %p)\n", This, ppv);
        *ppv = CONPOINT(This);
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
    ConnectionPoint *This = CONPOINT_THIS(iface);
    return IConnectionPointContainer_AddRef(This->container);
}

static ULONG WINAPI ConnectionPoint_Release(IConnectionPoint *iface)
{
    ConnectionPoint *This = CONPOINT_THIS(iface);
    return IConnectionPointContainer_Release(This->container);
}

static HRESULT WINAPI ConnectionPoint_GetConnectionInterface(IConnectionPoint *iface, IID *pIID)
{
    ConnectionPoint *This = CONPOINT_THIS(iface);

    TRACE("(%p)->(%p)\n", This, pIID);

    if(!pIID)
        return E_POINTER;

    *pIID = *This->iid;
    return S_OK;
}

static HRESULT WINAPI ConnectionPoint_GetConnectionPointContainer(IConnectionPoint *iface,
        IConnectionPointContainer **ppCPC)
{
    ConnectionPoint *This = CONPOINT_THIS(iface);

    TRACE("(%p)->(%p)\n", This, ppCPC);

    if(!ppCPC)
        return E_POINTER;

    *ppCPC = This->container;
    IConnectionPointContainer_AddRef(*ppCPC);
    return S_OK;
}

static HRESULT WINAPI ConnectionPoint_Advise(IConnectionPoint *iface, IUnknown *pUnkSink,
                                             DWORD *pdwCookie)
{
    ConnectionPoint *This = CONPOINT_THIS(iface);
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
    *pdwCookie = i+1;

    return S_OK;
}

static HRESULT WINAPI ConnectionPoint_Unadvise(IConnectionPoint *iface, DWORD dwCookie)
{
    ConnectionPoint *This = CONPOINT_THIS(iface);
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
    ConnectionPoint *This = CONPOINT_THIS(iface);
    FIXME("(%p)->(%p)\n", This, ppEnum);
    return E_NOTIMPL;
}

#undef CONPOINT_THIS

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

void ConnectionPoint_Init(ConnectionPoint *cp, ConnectionPointContainer *container, REFIID riid)
{
    cp->lpConnectionPointVtbl = &ConnectionPointVtbl;
    cp->container = CONPTCONT(container);
    cp->sinks = NULL;
    cp->sinks_size = 0;
    cp->iid = riid;
    cp->next = NULL;

    cp->next = container->cp_list;
    container->cp_list = cp;
}

static void ConnectionPoint_Destroy(ConnectionPoint *This)
{
    int i;

    for(i=0; i<This->sinks_size; i++) {
        if(This->sinks[i].unk)
            IUnknown_Release(This->sinks[i].unk);
    }

    heap_free(This->sinks);
}

#define CONPTCONT_THIS(iface) DEFINE_THIS(ConnectionPointContainer, ConnectionPointContainer, iface)

static HRESULT WINAPI ConnectionPointContainer_QueryInterface(IConnectionPointContainer *iface,
                                                              REFIID riid, void **ppv)
{
    ConnectionPointContainer *This = CONPTCONT_THIS(iface);
    return IUnknown_QueryInterface(This->outer, riid, ppv);
}

static ULONG WINAPI ConnectionPointContainer_AddRef(IConnectionPointContainer *iface)
{
    ConnectionPointContainer *This = CONPTCONT_THIS(iface);
    return IUnknown_AddRef(This->outer);
}

static ULONG WINAPI ConnectionPointContainer_Release(IConnectionPointContainer *iface)
{
    ConnectionPointContainer *This = CONPTCONT_THIS(iface);
    return IUnknown_Release(This->outer);
}

static HRESULT WINAPI ConnectionPointContainer_EnumConnectionPoints(IConnectionPointContainer *iface,
        IEnumConnectionPoints **ppEnum)
{
    ConnectionPointContainer *This = CONPTCONT_THIS(iface);
    FIXME("(%p)->(%p)\n", This, ppEnum);
    return E_NOTIMPL;
}

static HRESULT WINAPI ConnectionPointContainer_FindConnectionPoint(IConnectionPointContainer *iface,
        REFIID riid, IConnectionPoint **ppCP)
{
    ConnectionPointContainer *This = CONPTCONT_THIS(iface);
    ConnectionPoint *iter;

    TRACE("(%p)->(%s %p)\n", This, debugstr_cp_guid(riid), ppCP);

    *ppCP = NULL;

    for(iter = This->cp_list; iter; iter = iter->next) {
        if(IsEqualGUID(iter->iid, riid))
            *ppCP = CONPOINT(iter);
    }

    if(*ppCP) {
        IConnectionPoint_AddRef(*ppCP);
        return S_OK;
    }

    FIXME("unsupported riid %s\n", debugstr_cp_guid(riid));
    return CONNECT_E_NOCONNECTION;
}

static const IConnectionPointContainerVtbl ConnectionPointContainerVtbl = {
    ConnectionPointContainer_QueryInterface,
    ConnectionPointContainer_AddRef,
    ConnectionPointContainer_Release,
    ConnectionPointContainer_EnumConnectionPoints,
    ConnectionPointContainer_FindConnectionPoint
};

#undef CONPTCONT_THIS

void ConnectionPointContainer_Init(ConnectionPointContainer *This, IUnknown *outer)
{
    This->lpConnectionPointContainerVtbl = &ConnectionPointContainerVtbl;
    This->cp_list = NULL;
    This->outer = outer;
}

void ConnectionPointContainer_Destroy(ConnectionPointContainer *This)
{
    ConnectionPoint *iter = This->cp_list;

    while(iter) {
        ConnectionPoint_Destroy(iter);
        iter = iter->next;
    }
}
