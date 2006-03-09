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

static IConnectionPointImpl SHDOCVW_ConnectionPoint;

static const GUID IID_INotifyDBEvents =
    { 0xdb526cc0, 0xd188, 0x11cd, { 0xad, 0x48, 0x00, 0xaa, 0x00, 0x3c, 0x9c, 0xb6 } };

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

    FIXME("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppCP);

    /* For now, return the same IConnectionPoint object for both
     * event interface requests.
     */
    if (IsEqualGUID (&IID_INotifyDBEvents, riid))
    {
        TRACE("Returning connection point %p for IID_INotifyDBEvents\n",
              &SHDOCVW_ConnectionPoint);
        *ppCP = (LPCONNECTIONPOINT)&SHDOCVW_ConnectionPoint;
        return S_OK;
    }
    else if (IsEqualGUID (&IID_IPropertyNotifySink, riid))
    {
        TRACE("Returning connection point %p for IID_IPropertyNotifySink\n",
              &SHDOCVW_ConnectionPoint);
        *ppCP = (LPCONNECTIONPOINT)&SHDOCVW_ConnectionPoint;
        return S_OK;
    }

    return E_FAIL;
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

static HRESULT WINAPI WBCP_QueryInterface(LPCONNECTIONPOINT iface,
                                          REFIID riid, LPVOID *ppobj)
{
    FIXME("- no interface\n\tIID:\t%s\n", debugstr_guid(riid));

    if (ppobj == NULL) return E_POINTER;
    
    return E_NOINTERFACE;
}

static ULONG WINAPI WBCP_AddRef(LPCONNECTIONPOINT iface)
{
    SHDOCVW_LockModule();

    return 2; /* non-heap based object */
}

static ULONG WINAPI WBCP_Release(LPCONNECTIONPOINT iface)
{
    SHDOCVW_UnlockModule();

    return 1; /* non-heap based object */
}

static HRESULT WINAPI WBCP_GetConnectionInterface(LPCONNECTIONPOINT iface, IID* pIId)
{
    FIXME("stub: %s\n", debugstr_guid(pIId));
    return S_OK;
}

/* Get this connection point's owning container */
static HRESULT WINAPI
WBCP_GetConnectionPointContainer(LPCONNECTIONPOINT iface,
                                 LPCONNECTIONPOINTCONTAINER *ppCPC)
{
    FIXME("stub: IConnectionPointContainer = %p\n", *ppCPC);
    return S_OK;
}

/* Connect the pUnkSink event-handling implementation (in the control site)
 * to this connection point.  Return a handle to this connection in
 * pdwCookie (for later use in Unadvise()).
 */
static HRESULT WINAPI WBCP_Advise(LPCONNECTIONPOINT iface,
                                  LPUNKNOWN pUnkSink, DWORD *pdwCookie)
{
    static int new_cookie;

    FIXME("stub: IUnknown = %p, connection cookie = %ld\n", pUnkSink, *pdwCookie);

    *pdwCookie = ++new_cookie;
    TRACE ("Returning cookie = %ld\n", *pdwCookie);

    return S_OK;
}

/* Disconnect this implementation from the connection point. */
static HRESULT WINAPI WBCP_Unadvise(LPCONNECTIONPOINT iface,
                                    DWORD dwCookie)
{
    FIXME("stub: cookie to disconnect = %lx\n", dwCookie);
    return S_OK;
}

/* Get a list of connections in this connection point. */
static HRESULT WINAPI WBCP_EnumConnections(LPCONNECTIONPOINT iface,
                                           LPENUMCONNECTIONS *ppEnum)
{
    FIXME("stub: IEnumConnections = %p\n", *ppEnum);
    return S_OK;
}

/**********************************************************************
 * IConnectionPoint virtual function table for IE Web Browser component
 */

static const IConnectionPointVtbl WBCP_Vtbl =
{
    WBCP_QueryInterface,
    WBCP_AddRef,
    WBCP_Release,
    WBCP_GetConnectionInterface,
    WBCP_GetConnectionPointContainer,
    WBCP_Advise,
    WBCP_Unadvise,
    WBCP_EnumConnections
};

static IConnectionPointImpl SHDOCVW_ConnectionPoint = {&WBCP_Vtbl};

void WebBrowser_Events_Init(WebBrowser *This)
{
    This->lpConnectionPointContainerVtbl = &ConnectionPointContainerVtbl;
}
