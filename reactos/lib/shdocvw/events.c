/*
 * Implementation of event-related interfaces for WebBrowser control:
 *
 *  - IConnectionPointContainer
 *  - IConnectionPoint
 *
 * Copyright 2001 John R. Sheets (for CodeWeavers)
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <string.h>
#include "wine/debug.h"
#include "shdocvw.h"

WINE_DEFAULT_DEBUG_CHANNEL(shdocvw);

typedef struct {
    const IConnectionPointVtbl *lpConnectionPointVtbl;

    WebBrowser *webbrowser;

    IID iid;
} ConnectionPoint;

#define CONPOINT(x)  ((IConnectionPoint*) &(x)->lpConnectionPointVtbl)

/**********************************************************************
 * Implement the IConnectionPointContainer interface
 */

#define CONPTCONT_THIS(iface) DEFINE_THIS(WebBrowser, ConnectionPointContainer, iface)

static HRESULT WINAPI ConnectionPointContainer_QueryInterface(IConnectionPointContainer *iface,
        REFIID riid, LPVOID *ppobj)
{
    WebBrowser *This = CONPTCONT_THIS(iface);
    return IWebBrowser_QueryInterface(WEBBROWSER(This), riid, ppobj);
}

static ULONG WINAPI ConnectionPointContainer_AddRef(IConnectionPointContainer *iface)
{
    WebBrowser *This = CONPTCONT_THIS(iface);
    return IWebBrowser_AddRef(WEBBROWSER(This));
}

static ULONG WINAPI ConnectionPointContainer_Release(IConnectionPointContainer *iface)
{
    WebBrowser *This = CONPTCONT_THIS(iface);
    return IWebBrowser_Release(WEBBROWSER(This));
}

static HRESULT WINAPI ConnectionPointContainer_EnumConnectionPoints(IConnectionPointContainer *iface,
        LPENUMCONNECTIONPOINTS *ppEnum)
{
    WebBrowser *This = CONPTCONT_THIS(iface);
    FIXME("(%p)->(%p)\n", This, ppEnum);
    return E_NOTIMPL;
}

static HRESULT WINAPI ConnectionPointContainer_FindConnectionPoint(IConnectionPointContainer *iface,
        REFIID riid, LPCONNECTIONPOINT *ppCP)
{
    WebBrowser *This = CONPTCONT_THIS(iface);

    if(!ppCP) {
        WARN("ppCP == NULL\n");
        return E_POINTER;
    }

    *ppCP = NULL;

    if(IsEqualGUID(&DIID_DWebBrowserEvents2, riid)) {
        TRACE("(%p)->(DIID_DWebBrowserEvents2 %p)\n", This, ppCP);
        *ppCP = This->cp_wbe2;
    }else if(IsEqualGUID(&DIID_DWebBrowserEvents, riid)) {
        TRACE("(%p)->(DIID_DWebBrowserEvents %p)\n", This, ppCP);
        *ppCP = This->cp_wbe;
    }else if(IsEqualGUID(&IID_IPropertyNotifySink, riid)) {
        TRACE("(%p)->(IID_IPropertyNotifySink %p)\n", This, ppCP);
        *ppCP = This->cp_pns;
    }

    if(*ppCP) {
        IConnectionPoint_AddRef(*ppCP);
        return S_OK;
    }

    WARN("Unsupported IID %s\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

#undef CONPTCONT_THIS

static const IConnectionPointContainerVtbl ConnectionPointContainerVtbl =
{
    ConnectionPointContainer_QueryInterface,
    ConnectionPointContainer_AddRef,
    ConnectionPointContainer_Release,
    ConnectionPointContainer_EnumConnectionPoints,
    ConnectionPointContainer_FindConnectionPoint
};


/**********************************************************************
 * Implement the IConnectionPoint interface
 */

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
        IWebBrowser2_AddRef(WEBBROWSER(This->webbrowser));
        return S_OK;
    }

    WARN("Unsupported interface %s\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI ConnectionPoint_AddRef(IConnectionPoint *iface)
{
    ConnectionPoint *This = CONPOINT_THIS(iface);
    return IWebBrowser2_AddRef(WEBBROWSER(This->webbrowser));
}

static ULONG WINAPI ConnectionPoint_Release(IConnectionPoint *iface)
{
    ConnectionPoint *This = CONPOINT_THIS(iface);
    return IWebBrowser2_Release(WEBBROWSER(This->webbrowser));
}

static HRESULT WINAPI ConnectionPoint_GetConnectionInterface(IConnectionPoint *iface, IID *pIID)
{
    ConnectionPoint *This = CONPOINT_THIS(iface);

    TRACE("(%p)->(%p)\n", This, pIID);

    memcpy(pIID, &This->iid, sizeof(IID));
    return S_OK;
}

static HRESULT WINAPI ConnectionPoint_GetConnectionPointContainer(IConnectionPoint *iface,
        IConnectionPointContainer **ppCPC)
{
    ConnectionPoint *This = CONPOINT_THIS(iface);

    TRACE("(%p)->(%p)\n", This, ppCPC);

    *ppCPC = CONPTCONT(This->webbrowser);
    return S_OK;
}

static HRESULT WINAPI ConnectionPoint_Advise(IConnectionPoint *iface, IUnknown *pUnkSink,
                                             DWORD *pdwCookie)
{
    ConnectionPoint *This = CONPOINT_THIS(iface);
    FIXME("(%p)->(%p %p)\n", This, pUnkSink, pdwCookie);
    return E_NOTIMPL;
}

static HRESULT WINAPI ConnectionPoint_Unadvise(IConnectionPoint *iface, DWORD dwCookie)
{
    ConnectionPoint *This = CONPOINT_THIS(iface);
    FIXME("(%p)->(%ld)\n", This, dwCookie);
    return E_NOTIMPL;
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

static void ConnectionPoint_Create(WebBrowser *wb, REFIID riid, IConnectionPoint **cp)
{
    ConnectionPoint *ret = HeapAlloc(GetProcessHeap(), 0, sizeof(ConnectionPoint));

    ret->lpConnectionPointVtbl = &ConnectionPointVtbl;
    ret->webbrowser = wb;
    memcpy(&ret->iid, riid, sizeof(IID));

    *cp = CONPOINT(ret);
}

void WebBrowser_Events_Init(WebBrowser *This)
{
    This->lpConnectionPointContainerVtbl = &ConnectionPointContainerVtbl;

    ConnectionPoint_Create(This, &DIID_DWebBrowserEvents2, &This->cp_wbe2);
    ConnectionPoint_Create(This, &DIID_DWebBrowserEvents, &This->cp_wbe);
    ConnectionPoint_Create(This, &IID_IPropertyNotifySink, &This->cp_pns);
}

void WebBrowser_Events_Destroy(WebBrowser *This)
{
    HeapFree(GetProcessHeap(), 0, This->cp_wbe2);
    HeapFree(GetProcessHeap(), 0, This->cp_wbe);
    HeapFree(GetProcessHeap(), 0, This->cp_pns);
}
