/* 
 * DirectPlay8 Client
 * 
 * Copyright 2004 Raphael Junqueira
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
 *
 */


#include <stdarg.h>

#define COBJMACROS
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "objbase.h"
#include "wine/debug.h"

#include "dpnet_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(dpnet);

typedef struct IDirectPlay8LobbyClientImpl
{
    IDirectPlay8LobbyClient IDirectPlay8LobbyClient_iface;
    LONG ref;

    PFNDPNMESSAGEHANDLER msghandler;
    DWORD flags;
    void *usercontext;
} IDirectPlay8LobbyClientImpl;

static inline IDirectPlay8LobbyClientImpl *impl_from_IDirectPlay8LobbyClient(IDirectPlay8LobbyClient *iface)
{
    return CONTAINING_RECORD(iface, IDirectPlay8LobbyClientImpl, IDirectPlay8LobbyClient_iface);
}

static inline IDirectPlay8ClientImpl *impl_from_IDirectPlay8Client(IDirectPlay8Client *iface)
{
    return CONTAINING_RECORD(iface, IDirectPlay8ClientImpl, IDirectPlay8Client_iface);
}

/* IDirectPlay8Client IUnknown parts follow: */
static HRESULT WINAPI IDirectPlay8ClientImpl_QueryInterface(IDirectPlay8Client *iface, REFIID riid,
        void **ret_iface)
{
    if (IsEqualGUID(riid, &IID_IUnknown) || IsEqualGUID(riid, &IID_IDirectPlay8Client)) {
        IDirectPlay8Client_AddRef(iface);
        *ret_iface = iface;
        return S_OK;
    }

    WARN("(%p)->(%s,%p): not found\n", iface, debugstr_guid(riid), ret_iface);
    return E_NOINTERFACE;
}

static ULONG WINAPI IDirectPlay8ClientImpl_AddRef(IDirectPlay8Client *iface)
{
    IDirectPlay8ClientImpl *This = impl_from_IDirectPlay8Client(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%lu\n", This, ref);

    return ref;
}

static ULONG WINAPI IDirectPlay8ClientImpl_Release(IDirectPlay8Client *iface)
{
    IDirectPlay8ClientImpl *This = impl_from_IDirectPlay8Client(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%lu\n", This, ref);

    if (!ref)
    {
        free(This->username);
        free(This->data);
        free(This);
    }
    return ref;
}

/* IDirectPlay8Client Interface follow: */

static HRESULT WINAPI IDirectPlay8ClientImpl_Initialize(IDirectPlay8Client *iface,
        void * const pvUserContext, const PFNDPNMESSAGEHANDLER pfn, const DWORD dwFlags)
{
    IDirectPlay8ClientImpl *This = impl_from_IDirectPlay8Client(iface);

    TRACE("(%p):(%p,%p,%lx)\n", This, pvUserContext, pfn, dwFlags);

    if(!pfn)
        return DPNERR_INVALIDPARAM;

    This->usercontext = pvUserContext;
    This->msghandler = pfn;
    This->flags = dwFlags;

    init_winsock();

    return DPN_OK;
}

static HRESULT WINAPI IDirectPlay8ClientImpl_EnumServiceProviders(IDirectPlay8Client *iface,
        const GUID * const pguidServiceProvider, const GUID * const pguidApplication,
        DPN_SERVICE_PROVIDER_INFO * const pSPInfoBuffer, PDWORD const pcbEnumData,
        PDWORD const pcReturned, const DWORD dwFlags)
{
    IDirectPlay8ClientImpl *This = impl_from_IDirectPlay8Client(iface);
    TRACE("(%p)->(%s,%s,%p,%p,%p,%lx)\n", This, debugstr_guid(pguidServiceProvider), debugstr_guid(pguidApplication),
                                             pSPInfoBuffer, pcbEnumData, pcReturned, dwFlags);

    if(dwFlags)
        FIXME("Unhandled flags %lx\n", dwFlags);

    if(pguidApplication)
        FIXME("Application guid %s is currently being ignored\n", debugstr_guid(pguidApplication));

    return enum_services_providers(pguidServiceProvider, pSPInfoBuffer, pcbEnumData, pcReturned);
}

static HRESULT WINAPI IDirectPlay8ClientImpl_EnumHosts(IDirectPlay8Client *iface,
        PDPN_APPLICATION_DESC const pApplicationDesc, IDirectPlay8Address * const pAddrHost,
        IDirectPlay8Address * const pDeviceInfo, void * const pUserEnumData,
        const DWORD dwUserEnumDataSize, const DWORD dwEnumCount, const DWORD dwRetryInterval,
        const DWORD dwTimeOut, void * const pvUserContext, DPNHANDLE * const pAsyncHandle,
        const DWORD dwFlags)
{
    IDirectPlay8ClientImpl *This = impl_from_IDirectPlay8Client(iface);

    FIXME("(%p):(%p,%p,%p,%p,%lu,%lu,%lu,%lu,%p,%p,%lx)\n", This, pApplicationDesc, pAddrHost, pDeviceInfo, pUserEnumData,
        dwUserEnumDataSize, dwEnumCount, dwRetryInterval, dwTimeOut, pvUserContext, pAsyncHandle, dwFlags);

    if(!This->msghandler)
        return DPNERR_UNINITIALIZED;

    if((dwFlags & DPNENUMHOSTS_SYNC) && pAsyncHandle)
        return DPNERR_INVALIDPARAM;

    if(dwUserEnumDataSize > This->spcaps.dwMaxEnumPayloadSize)
        return DPNERR_ENUMQUERYTOOLARGE;

    return (dwFlags & DPNENUMHOSTS_SYNC) ? DPN_OK : DPNSUCCESS_PENDING;
}

static HRESULT WINAPI IDirectPlay8ClientImpl_CancelAsyncOperation(IDirectPlay8Client *iface,
        const DPNHANDLE hAsyncHandle, const DWORD dwFlags)
{
  IDirectPlay8ClientImpl *This = impl_from_IDirectPlay8Client(iface);
  FIXME("(%p):(%lu,%lx): Stub\n", This, hAsyncHandle, dwFlags);
  return DPN_OK; 
}

static HRESULT WINAPI IDirectPlay8ClientImpl_Connect(IDirectPlay8Client *iface,
        const DPN_APPLICATION_DESC * const pdnAppDesc, IDirectPlay8Address * const pHostAddr,
        IDirectPlay8Address * const pDeviceInfo, const DPN_SECURITY_DESC * const pdnSecurity,
        const DPN_SECURITY_CREDENTIALS * const pdnCredentials, const void * const pvUserConnectData,
        const DWORD dwUserConnectDataSize, void * const pvAsyncContext,
        DPNHANDLE * const phAsyncHandle, const DWORD dwFlags)
{
  IDirectPlay8ClientImpl *This = impl_from_IDirectPlay8Client(iface);
  FIXME("(%p):(%p,%p,%lx): Stub\n", This, pvAsyncContext, phAsyncHandle, dwFlags);
  return DPN_OK; 
}

static HRESULT WINAPI IDirectPlay8ClientImpl_Send(IDirectPlay8Client *iface,
        const DPN_BUFFER_DESC * const prgBufferDesc, const DWORD cBufferDesc, const DWORD dwTimeOut,
        void * const pvAsyncContext, DPNHANDLE * const phAsyncHandle, const DWORD dwFlags)
{
  IDirectPlay8ClientImpl *This = impl_from_IDirectPlay8Client(iface);
  FIXME("(%p):(%p,%p,%lx): Stub\n", This, pvAsyncContext, phAsyncHandle, dwFlags);
  return DPN_OK; 
}

static HRESULT WINAPI IDirectPlay8ClientImpl_GetSendQueueInfo(IDirectPlay8Client *iface,
        DWORD * const pdwNumMsgs, DWORD * const pdwNumBytes, const DWORD dwFlags)
{
  IDirectPlay8ClientImpl *This = impl_from_IDirectPlay8Client(iface);
  FIXME("(%p):(%lx): Stub\n", This, dwFlags);
  return DPN_OK; 
}

static HRESULT WINAPI IDirectPlay8ClientImpl_GetApplicationDesc(IDirectPlay8Client *iface,
        DPN_APPLICATION_DESC * const pAppDescBuffer, DWORD * const pcbDataSize, const DWORD dwFlags)
{
  IDirectPlay8ClientImpl *This = impl_from_IDirectPlay8Client(iface);
  FIXME("(%p):(%lx): Stub\n", This, dwFlags);
  return DPN_OK; 
}

static HRESULT WINAPI IDirectPlay8ClientImpl_SetClientInfo(IDirectPlay8Client *iface,
        const DPN_PLAYER_INFO * const pdpnPlayerInfo, void * const pvAsyncContext,
        DPNHANDLE * const phAsyncHandle, const DWORD dwFlags)
{
    IDirectPlay8ClientImpl *This = impl_from_IDirectPlay8Client(iface);
    FIXME("(%p):(%p,%p,%lx): Semi-stub.\n", This, pvAsyncContext, phAsyncHandle, dwFlags);

    if(!pdpnPlayerInfo)
       return E_POINTER;

    if(phAsyncHandle)
        FIXME("Async handle currently not supported.\n");

    if (pdpnPlayerInfo->dwInfoFlags & DPNINFO_NAME)
    {
        free(This->username);
        This->username = NULL;

        if(pdpnPlayerInfo->pwszName)
        {
            This->username = wcsdup(pdpnPlayerInfo->pwszName);
            if (!This->username)
                return E_OUTOFMEMORY;
        }
    }

    if (pdpnPlayerInfo->dwInfoFlags & DPNINFO_DATA)
    {
        free(This->data);
        This->data = NULL;
        This->datasize = 0;

        if(!pdpnPlayerInfo->pvData && pdpnPlayerInfo->dwDataSize)
            return E_POINTER;

        if(pdpnPlayerInfo->dwDataSize && pdpnPlayerInfo->pvData)
        {
            This->data = malloc(pdpnPlayerInfo->dwDataSize);
            if (!This->data)
                return E_OUTOFMEMORY;

            This->datasize = pdpnPlayerInfo->dwDataSize;

            memcpy(This->data, pdpnPlayerInfo->pvData, pdpnPlayerInfo->dwDataSize);
        }
    }

    return DPN_OK;
}

static HRESULT WINAPI IDirectPlay8ClientImpl_GetServerInfo(IDirectPlay8Client *iface,
        DPN_PLAYER_INFO * const pdpnPlayerInfo, DWORD * const pdwSize, const DWORD dwFlags)
{
  IDirectPlay8ClientImpl *This = impl_from_IDirectPlay8Client(iface);
  FIXME("(%p):(%lx): Stub\n", This, dwFlags);
  return DPN_OK; 
}

static HRESULT WINAPI IDirectPlay8ClientImpl_GetServerAddress(IDirectPlay8Client *iface,
        IDirectPlay8Address ** const pAddress, const DWORD dwFlags)
{
  IDirectPlay8ClientImpl *This = impl_from_IDirectPlay8Client(iface);
  FIXME("(%p):(%lx): Stub\n", This, dwFlags);
  return DPN_OK; 
}

static HRESULT WINAPI IDirectPlay8ClientImpl_Close(IDirectPlay8Client *iface, const DWORD dwFlags)
{
    IDirectPlay8ClientImpl *This = impl_from_IDirectPlay8Client(iface);
    FIXME("(%p):(%lx): Stub\n", This, dwFlags);

    This->msghandler = NULL;

    return DPN_OK;
}

static HRESULT WINAPI IDirectPlay8ClientImpl_ReturnBuffer(IDirectPlay8Client *iface,
        const DPNHANDLE hBufferHandle, const DWORD dwFlags)
{
  IDirectPlay8ClientImpl *This = impl_from_IDirectPlay8Client(iface);
  FIXME("(%p):(%lx): Stub\n", This, dwFlags);
  return DPN_OK; 
}

static HRESULT WINAPI IDirectPlay8ClientImpl_GetCaps(IDirectPlay8Client *iface,
        DPN_CAPS * const pdpCaps, const DWORD dwFlags)
{
  IDirectPlay8ClientImpl *This = impl_from_IDirectPlay8Client(iface);
  FIXME("(%p):(%lx): Stub\n", This, dwFlags);
  return DPN_OK; 
}

static HRESULT WINAPI IDirectPlay8ClientImpl_SetCaps(IDirectPlay8Client *iface,
        const DPN_CAPS * const pdpCaps, const DWORD dwFlags)
{
  IDirectPlay8ClientImpl *This = impl_from_IDirectPlay8Client(iface);
  FIXME("(%p):(%lx): Stub\n", This, dwFlags);
  return DPN_OK; 
}

static HRESULT WINAPI IDirectPlay8ClientImpl_SetSPCaps(IDirectPlay8Client *iface,
        const GUID * const pguidSP, const DPN_SP_CAPS * const pdpspCaps, const DWORD dwFlags)
{
    IDirectPlay8ClientImpl *This = impl_from_IDirectPlay8Client(iface);

    TRACE("(%p)->(%p,%p,%lx): stub\n", iface, pguidSP, pdpspCaps, dwFlags);

    if(!This->msghandler || pdpspCaps->dwSize != sizeof(DPN_SP_CAPS))
        return DPNERR_INVALIDPARAM;

    /* Only dwSystemBufferSize is set by this call. */
    This->spcaps.dwSystemBufferSize = pdpspCaps->dwSystemBufferSize;

    return DPN_OK;
}

static HRESULT WINAPI IDirectPlay8ClientImpl_GetSPCaps(IDirectPlay8Client *iface,
        const GUID * const pguidSP, DPN_SP_CAPS * const pdpspCaps, const DWORD dwFlags)
{
    IDirectPlay8ClientImpl *This = impl_from_IDirectPlay8Client(iface);

    TRACE("(%p)->(%p,%p,%lx)\n", This, pguidSP, pdpspCaps, dwFlags);

    if(!This->msghandler)
        return DPNERR_UNINITIALIZED;

    if(pdpspCaps->dwSize != sizeof(DPN_SP_CAPS))
    {
        return DPNERR_INVALIDPARAM;
    }

    *pdpspCaps = This->spcaps;

    return DPN_OK;
}

static HRESULT WINAPI IDirectPlay8ClientImpl_GetConnectionInfo(IDirectPlay8Client *iface,
        DPN_CONNECTION_INFO * const pdpConnectionInfo, const DWORD dwFlags)
{
  IDirectPlay8ClientImpl *This = impl_from_IDirectPlay8Client(iface);
  FIXME("(%p):(%lx): Stub\n", This, dwFlags);
  return DPN_OK; 
}

static HRESULT WINAPI IDirectPlay8ClientImpl_RegisterLobby(IDirectPlay8Client *iface,
        const DPNHANDLE dpnHandle,
        struct IDirectPlay8LobbiedApplication * const pIDP8LobbiedApplication, const DWORD dwFlags)
{
  IDirectPlay8ClientImpl *This = impl_from_IDirectPlay8Client(iface);
  FIXME("(%p):(%lx): Stub\n", This, dwFlags);
  return DPN_OK; 
}

static const IDirectPlay8ClientVtbl DirectPlay8Client_Vtbl =
{
    IDirectPlay8ClientImpl_QueryInterface,
    IDirectPlay8ClientImpl_AddRef,
    IDirectPlay8ClientImpl_Release,
    IDirectPlay8ClientImpl_Initialize,
    IDirectPlay8ClientImpl_EnumServiceProviders,
    IDirectPlay8ClientImpl_EnumHosts,
    IDirectPlay8ClientImpl_CancelAsyncOperation,
    IDirectPlay8ClientImpl_Connect,
    IDirectPlay8ClientImpl_Send,
    IDirectPlay8ClientImpl_GetSendQueueInfo,
    IDirectPlay8ClientImpl_GetApplicationDesc,
    IDirectPlay8ClientImpl_SetClientInfo,
    IDirectPlay8ClientImpl_GetServerInfo,
    IDirectPlay8ClientImpl_GetServerAddress,
    IDirectPlay8ClientImpl_Close,
    IDirectPlay8ClientImpl_ReturnBuffer,
    IDirectPlay8ClientImpl_GetCaps,
    IDirectPlay8ClientImpl_SetCaps,
    IDirectPlay8ClientImpl_SetSPCaps,
    IDirectPlay8ClientImpl_GetSPCaps,
    IDirectPlay8ClientImpl_GetConnectionInfo,
    IDirectPlay8ClientImpl_RegisterLobby
};

HRESULT DPNET_CreateDirectPlay8Client(IClassFactory *iface, IUnknown *pUnkOuter, REFIID riid, void **ppv)
{
    IDirectPlay8ClientImpl* client;
    HRESULT hr;

    TRACE("(%p, %s, %p)\n", pUnkOuter, debugstr_guid(riid), ppv);

    *ppv = NULL;

    if(pUnkOuter)
        return CLASS_E_NOAGGREGATION;

    client = calloc(1, sizeof(IDirectPlay8ClientImpl));
    if (!client)
        return E_OUTOFMEMORY;

    client->IDirectPlay8Client_iface.lpVtbl = &DirectPlay8Client_Vtbl;
    client->ref = 1;

    init_dpn_sp_caps(&client->spcaps);

    hr = IDirectPlay8ClientImpl_QueryInterface(&client->IDirectPlay8Client_iface, riid, ppv);
    IDirectPlay8ClientImpl_Release(&client->IDirectPlay8Client_iface);

    return hr;
}

static HRESULT WINAPI lobbyclient_QueryInterface(IDirectPlay8LobbyClient *iface, REFIID riid, void **obj)
{
    IDirectPlay8LobbyClientImpl *This = impl_from_IDirectPlay8LobbyClient(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), obj);

    *obj = NULL;

    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_IDirectPlay8LobbyClient))
    {
        *obj = &This->IDirectPlay8LobbyClient_iface;
        IUnknown_AddRef( (IUnknown*)*obj);

        return DPN_OK;
    }

    WARN("(%p)->(%s,%p),not found\n",This,debugstr_guid(riid),obj);
    return E_NOINTERFACE;
}

static ULONG WINAPI lobbyclient_AddRef(IDirectPlay8LobbyClient *iface)
{
    IDirectPlay8LobbyClientImpl *This = impl_from_IDirectPlay8LobbyClient(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%lu\n", This, ref);

    return ref;
}

static ULONG WINAPI lobbyclient_Release(IDirectPlay8LobbyClient *iface)
{
    IDirectPlay8LobbyClientImpl *This = impl_from_IDirectPlay8LobbyClient(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%lu\n", This, ref);

    if (!ref)
    {
        free(This);
    }

    return ref;
}

static HRESULT WINAPI lobbyclient_Initialize(IDirectPlay8LobbyClient *iface, void *context,
                                             PFNDPNMESSAGEHANDLER msghandler, DWORD flags)
{
    IDirectPlay8LobbyClientImpl *This = impl_from_IDirectPlay8LobbyClient(iface);

    TRACE("(%p):(%p,%p,%lx)\n", This, context, msghandler, flags);

    if(!msghandler)
        return E_POINTER;

    This->usercontext = context;
    This->msghandler = msghandler;
    This->flags = flags;

    init_winsock();

    return DPN_OK;
}

static HRESULT WINAPI lobbyclient_EnumLocalPrograms(IDirectPlay8LobbyClient *iface, GUID* guidapplication,
                                                    BYTE *enumdata, DWORD *enumDataSize, DWORD *items, DWORD flags)
{
    IDirectPlay8LobbyClientImpl *This = impl_from_IDirectPlay8LobbyClient(iface);

    FIXME("(%p)->(%p %p %p %p 0x%08lx)\n", This, guidapplication, enumdata, enumDataSize, items, flags);

    if(enumDataSize)
        *enumDataSize = 0;

    return S_OK;
}

static HRESULT WINAPI lobbyclient_ConnectApplication(IDirectPlay8LobbyClient *iface, DPL_CONNECT_INFO *connectioninfo,
                                                     void *connectioncontext, DPNHANDLE *application, DWORD timeout,
                                                     DWORD flags)
{
    IDirectPlay8LobbyClientImpl *This = impl_from_IDirectPlay8LobbyClient(iface);

    FIXME("(%p)->(%p %p %p %lu 0x%08lx)\n", This, connectioninfo, connectioncontext, application, timeout, flags);

    return E_NOTIMPL;
}

static HRESULT WINAPI lobbyclient_Send(IDirectPlay8LobbyClient *iface, DPNHANDLE connection, BYTE *buffer, DWORD buffersize, DWORD flags)
{
    IDirectPlay8LobbyClientImpl *This = impl_from_IDirectPlay8LobbyClient(iface);

    FIXME("(%p)->(%lu %p %lu 0x%08lx)\n", This, connection, buffer, buffersize, flags);

    return E_NOTIMPL;
}

static HRESULT WINAPI lobbyclient_ReleaseApplication(IDirectPlay8LobbyClient *iface, DPNHANDLE connection, DWORD flags)
{
    IDirectPlay8LobbyClientImpl *This = impl_from_IDirectPlay8LobbyClient(iface);

    FIXME("(%p)->(%lu 0x%08lx)\n", This, connection, flags);

    return E_NOTIMPL;
}

static HRESULT WINAPI lobbyclient_Close(IDirectPlay8LobbyClient *iface, DWORD flags)
{
    IDirectPlay8LobbyClientImpl *This = impl_from_IDirectPlay8LobbyClient(iface);

    FIXME("(%p)->(0x%08lx)\n", This, flags);

    return E_NOTIMPL;
}

static HRESULT WINAPI lobbyclient_GetConnectionSettings(IDirectPlay8LobbyClient *iface, DPNHANDLE connection, DPL_CONNECTION_SETTINGS *sessioninfo, DWORD *infosize, DWORD flags)
{
    IDirectPlay8LobbyClientImpl *This = impl_from_IDirectPlay8LobbyClient(iface);

    FIXME("(%p)->(%lu %p %p 0x%08lx)\n", This, connection, sessioninfo, infosize, flags);

    return E_NOTIMPL;
}

static HRESULT WINAPI lobbyclient_SetConnectionSettings(IDirectPlay8LobbyClient *iface, DPNHANDLE connection, const DPL_CONNECTION_SETTINGS *sessioninfo, DWORD flags)
{
    IDirectPlay8LobbyClientImpl *This = impl_from_IDirectPlay8LobbyClient(iface);

    FIXME("(%p)->(%lu %p 0x%08lx)\n", This, connection, sessioninfo, flags);

    return E_NOTIMPL;
}

static const IDirectPlay8LobbyClientVtbl DirectPlay8LobbiedClient_Vtbl =
{
    lobbyclient_QueryInterface,
    lobbyclient_AddRef,
    lobbyclient_Release,
    lobbyclient_Initialize,
    lobbyclient_EnumLocalPrograms,
    lobbyclient_ConnectApplication,
    lobbyclient_Send,
    lobbyclient_ReleaseApplication,
    lobbyclient_Close,
    lobbyclient_GetConnectionSettings,
    lobbyclient_SetConnectionSettings
};

HRESULT DPNET_CreateDirectPlay8LobbyClient(IClassFactory *iface, IUnknown *outer, REFIID riid, void **obj)
{
    IDirectPlay8LobbyClientImpl *client;
    HRESULT ret;

    TRACE("%p (%p, %s, %p)\n", iface, outer, debugstr_guid(riid), obj);

    client = calloc(1, sizeof(*client));
    if (!client)
    {
        *obj = NULL;
        return E_OUTOFMEMORY;
    }

    client->IDirectPlay8LobbyClient_iface.lpVtbl = &DirectPlay8LobbiedClient_Vtbl;
    client->ref = 1;

    ret = lobbyclient_QueryInterface(&client->IDirectPlay8LobbyClient_iface, riid, obj);
    lobbyclient_Release(&client->IDirectPlay8LobbyClient_iface);

    return ret;
}
