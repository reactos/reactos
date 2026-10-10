/* 
 * DirectPlay8 Server
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
#include "ocidl.h"
#include "wine/debug.h"

#include "dpnet_private.h"

typedef struct IDirectPlay8ServerImpl
{
    IDirectPlay8Server IDirectPlay8Server_iface;
    LONG ref;

    PFNDPNMESSAGEHANDLER msghandler;
    DWORD flags;
    void *usercontext;

    WCHAR *servername;
    void  *data;
    DWORD datasize;
} IDirectPlay8ServerImpl;

WINE_DEFAULT_DEBUG_CHANNEL(dpnet);

static inline IDirectPlay8ServerImpl *impl_from_IDirectPlay8Server(IDirectPlay8Server *iface)
{
    return CONTAINING_RECORD(iface, IDirectPlay8ServerImpl, IDirectPlay8Server_iface);
}

static HRESULT WINAPI IDirectPlay8ServerImpl_QueryInterface(IDirectPlay8Server *iface, REFIID riid, void **ppv)
{
    IDirectPlay8ServerImpl *This = impl_from_IDirectPlay8Server(iface);

    TRACE("%p %s %p\n", This, debugstr_guid(riid), ppv);

    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_IDirectPlay8Server))
    {
        TRACE("(%p)->(IID_IDirectPlay8Server %p)\n", This, ppv);
        *ppv = &This->IDirectPlay8Server_iface;
    }
    else {
        WARN("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppv);
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI IDirectPlay8ServerImpl_AddRef(IDirectPlay8Server *iface)
{
    IDirectPlay8ServerImpl *This = impl_from_IDirectPlay8Server(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    return ref;
}

static ULONG WINAPI IDirectPlay8ServerImpl_Release(IDirectPlay8Server *iface)
{
    IDirectPlay8ServerImpl *This = impl_from_IDirectPlay8Server(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    if (!ref)
    {
        free(This->servername);
        free(This->data);
        free(This);
    }

    return ref;
}

static HRESULT WINAPI IDirectPlay8ServerImpl_Initialize(IDirectPlay8Server *iface, PVOID pvUserContext,
                                            PFNDPNMESSAGEHANDLER pfn, DWORD dwFlags)
{
    IDirectPlay8ServerImpl *This = impl_from_IDirectPlay8Server(iface);
    TRACE("(%p)->(%p %p %ld)\n", This, pvUserContext, pfn, dwFlags);

    if(!pfn)
        return DPNERR_INVALIDPARAM;

    This->usercontext = pvUserContext;
    This->msghandler = pfn;
    This->flags = dwFlags;

    init_winsock();

    return DPN_OK;
}

static HRESULT WINAPI IDirectPlay8ServerImpl_EnumServiceProviders(IDirectPlay8Server *iface, const GUID *pguidServiceProvider,
                                            const GUID *pguidApplication, DPN_SERVICE_PROVIDER_INFO *pSPInfoBuffer, PDWORD pcbEnumData,
                                            PDWORD pcReturned, DWORD dwFlags)
{
    IDirectPlay8ServerImpl *This = impl_from_IDirectPlay8Server(iface);
    TRACE("(%p)->(%s %s %p %p %p %ld)\n", This, debugstr_guid(pguidServiceProvider), debugstr_guid(pguidApplication),
                pSPInfoBuffer, pcbEnumData, pcReturned, dwFlags);

    if(!This->msghandler)
       return DPNERR_UNINITIALIZED;

    if(dwFlags)
        FIXME("Unhandled flags %lx\n", dwFlags);

    if(pguidApplication)
        FIXME("Application guid %s is currently being ignored\n", debugstr_guid(pguidApplication));

    return enum_services_providers(pguidServiceProvider, pSPInfoBuffer, pcbEnumData, pcReturned);
}

static HRESULT WINAPI IDirectPlay8ServerImpl_CancelAsyncOperation(IDirectPlay8Server *iface, DPNHANDLE hAsyncHandle, DWORD dwFlags)
{
    IDirectPlay8ServerImpl *This = impl_from_IDirectPlay8Server(iface);
    FIXME("(%p)->(%ld %ld)\n", This, hAsyncHandle, dwFlags);
    return E_NOTIMPL;
}

static HRESULT WINAPI IDirectPlay8ServerImpl_GetSendQueueInfo(IDirectPlay8Server *iface, DPNID dpnid, DWORD *pdwNumMsgs,
                                            DWORD *pdwNumBytes, DWORD dwFlags)
{
    IDirectPlay8ServerImpl *This = impl_from_IDirectPlay8Server(iface);
    FIXME("(%p)->(%ld %p %p %ld)\n", This, dpnid, pdwNumMsgs, pdwNumBytes, dwFlags);
    return E_NOTIMPL;
}

static HRESULT WINAPI IDirectPlay8ServerImpl_GetApplicationDesc(IDirectPlay8Server *iface, DPN_APPLICATION_DESC *pAppDescBuffer,
                                            DWORD *pcbDataSize, DWORD dwFlags)
{
    IDirectPlay8ServerImpl *This = impl_from_IDirectPlay8Server(iface);
    FIXME("(%p)->(%p %p %ld)\n", This, pAppDescBuffer, pcbDataSize, dwFlags);
    return E_NOTIMPL;
}

static HRESULT WINAPI IDirectPlay8ServerImpl_SetServerInfo(IDirectPlay8Server *iface, const DPN_PLAYER_INFO *pdpnPlayerInfo,
                                            PVOID pvAsyncContext, DPNHANDLE *phAsyncHandle, DWORD dwFlags)
{
    IDirectPlay8ServerImpl *This = impl_from_IDirectPlay8Server(iface);

    FIXME("(%p)->(%p %p %p %lx)  Semi-stub\n", This, pdpnPlayerInfo, pvAsyncContext, phAsyncHandle, dwFlags);

    if(!pdpnPlayerInfo)
       return E_POINTER;

    if(!This->msghandler)
       return DPNERR_UNINITIALIZED;

    if(phAsyncHandle)
        FIXME("Async handle currently not supported.\n");

    if (pdpnPlayerInfo->dwInfoFlags & DPNINFO_NAME)
    {
        free(This->servername);
        This->servername = NULL;

        if(pdpnPlayerInfo->pwszName)
        {
            This->servername = wcsdup(pdpnPlayerInfo->pwszName);
            if (!This->servername)
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

    /* TODO: Send DPN_MSGID_SERVER_INFO message to all players. */
    return S_OK;
}

static HRESULT WINAPI IDirectPlay8ServerImpl_GetClientInfo(IDirectPlay8Server *iface, DPNID dpnid, DPN_PLAYER_INFO *pdpnPlayerInfo,
                                            DWORD *pdwSize, DWORD dwFlags)
{
    IDirectPlay8ServerImpl *This = impl_from_IDirectPlay8Server(iface);
    FIXME("(%p)->(%ld %p %p %ld)\n", This, dpnid, pdpnPlayerInfo, pdwSize, dwFlags);
    return E_NOTIMPL;
}

static HRESULT WINAPI IDirectPlay8ServerImpl_GetClientAddress(IDirectPlay8Server *iface, DPNID dpnid, IDirectPlay8Address **pAddress,
                                            DWORD dwFlags)
{
    IDirectPlay8ServerImpl *This = impl_from_IDirectPlay8Server(iface);
    FIXME("(%p)->(%ld %p %ld)\n", This, dpnid, pAddress, dwFlags);
    return E_NOTIMPL;
}

static HRESULT WINAPI IDirectPlay8ServerImpl_GetLocalHostAddresses(IDirectPlay8Server *iface, IDirectPlay8Address **prgpAddress,
                                            DWORD *pcAddress, DWORD dwFlags)
{
    IDirectPlay8ServerImpl *This = impl_from_IDirectPlay8Server(iface);
    FIXME("(%p)->(%p %p %ld)\n", This, prgpAddress, pcAddress, dwFlags);
    return E_NOTIMPL;
}

static HRESULT WINAPI IDirectPlay8ServerImpl_SetApplicationDesc(IDirectPlay8Server *iface, const DPN_APPLICATION_DESC *pad, DWORD dwFlags)
{
    IDirectPlay8ServerImpl *This = impl_from_IDirectPlay8Server(iface);
    FIXME("(%p)->(%p %ld)\n", This, pad, dwFlags);
    return E_NOTIMPL;
}

static HRESULT WINAPI IDirectPlay8ServerImpl_Host(IDirectPlay8Server *iface, const DPN_APPLICATION_DESC *pdnAppDesc,
                                            IDirectPlay8Address **prgpDeviceInfo, DWORD cDeviceInfo, const DPN_SECURITY_DESC *pdnSecurity,
                                            const DPN_SECURITY_CREDENTIALS *pdnCredentials, void *pvPlayerContext, DWORD dwFlags)
{
    IDirectPlay8ServerImpl *This = impl_from_IDirectPlay8Server(iface);
    FIXME("(%p)->(%p %p %ld %p %p %p %ld)\n", This, pdnAppDesc, prgpDeviceInfo, cDeviceInfo, pdnSecurity, pdnCredentials,
                    pvPlayerContext, dwFlags);
    return E_NOTIMPL;
}

static HRESULT WINAPI IDirectPlay8ServerImpl_SendTo(IDirectPlay8Server *iface, DPNID dpnid, const DPN_BUFFER_DESC *prgBufferDesc,
                                             DWORD cBufferDesc, DWORD dwTimeOut, void *pvAsyncContext, DPNHANDLE *phAsyncHandle, DWORD dwFlags)
{
    IDirectPlay8ServerImpl *This = impl_from_IDirectPlay8Server(iface);
    FIXME("(%p)->(%ld %p %ld %ld %p %p %ld)\n", This, dpnid, prgBufferDesc, cBufferDesc, dwTimeOut, pvAsyncContext, phAsyncHandle,
                    dwFlags);
    return E_NOTIMPL;
}

static HRESULT WINAPI IDirectPlay8ServerImpl_CreateGroup(IDirectPlay8Server *iface, const DPN_GROUP_INFO *pdpnGroupInfo,
                                             void *pvGroupContext, void *pvAsyncContext, DPNHANDLE *phAsyncHandle, DWORD dwFlags)
{
    IDirectPlay8ServerImpl *This = impl_from_IDirectPlay8Server(iface);
    FIXME("(%p)->(%p %p %p %p %ld)\n", This, pdpnGroupInfo, pvGroupContext, pvAsyncContext, phAsyncHandle, dwFlags);
    return E_NOTIMPL;
}

static HRESULT WINAPI IDirectPlay8ServerImpl_DestroyGroup(IDirectPlay8Server *iface, DPNID idGroup, PVOID pvAsyncContext,
                                            DPNHANDLE *phAsyncHandle, DWORD dwFlags)
{
    IDirectPlay8ServerImpl *This = impl_from_IDirectPlay8Server(iface);
    FIXME("(%p)->(%ld %p %p %ld)\n", This, idGroup, pvAsyncContext, phAsyncHandle, dwFlags);
    return E_NOTIMPL;
}

static HRESULT WINAPI IDirectPlay8ServerImpl_AddPlayerToGroup(IDirectPlay8Server *iface, DPNID idGroup, DPNID idClient,
                                            PVOID pvAsyncContext, DPNHANDLE *phAsyncHandle, DWORD dwFlags)
{
    IDirectPlay8ServerImpl *This = impl_from_IDirectPlay8Server(iface);
    FIXME("(%p)->(%ld %ld %p %p %ld)\n", This, idGroup, idClient, pvAsyncContext, phAsyncHandle, dwFlags);
    return E_NOTIMPL;
}

static HRESULT WINAPI IDirectPlay8ServerImpl_RemovePlayerFromGroup(IDirectPlay8Server *iface, DPNID idGroup, DPNID idClient,
                                            PVOID pvAsyncContext, DPNHANDLE *phAsyncHandle, DWORD dwFlags)
{
    IDirectPlay8ServerImpl *This = impl_from_IDirectPlay8Server(iface);
    FIXME("(%p)->(%ld %ld %p %p %ld)\n", This, idGroup, idClient, pvAsyncContext, phAsyncHandle, dwFlags);
    return E_NOTIMPL;
}

static HRESULT WINAPI IDirectPlay8ServerImpl_SetGroupInfo(IDirectPlay8Server *iface, DPNID dpnid, DPN_GROUP_INFO *pdpnGroupInfo,
                                            PVOID pvAsyncContext, DPNHANDLE *phAsyncHandle, DWORD dwFlags)
{
    IDirectPlay8ServerImpl *This = impl_from_IDirectPlay8Server(iface);
    FIXME("(%p)->(%ld %p %p %p %ld)\n", This, dpnid, pdpnGroupInfo, pvAsyncContext, phAsyncHandle, dwFlags);
    return E_NOTIMPL;
}

static HRESULT WINAPI IDirectPlay8ServerImpl_GetGroupInfo(IDirectPlay8Server *iface, DPNID dpnid, DPN_GROUP_INFO *pdpnGroupInfo,
                                            DWORD *pdwSize, DWORD dwFlags)
{
    IDirectPlay8ServerImpl *This = impl_from_IDirectPlay8Server(iface);
    FIXME("(%p)->(%ld %p %p %ld)\n", This, dpnid, pdpnGroupInfo, pdwSize, dwFlags);
    return E_NOTIMPL;
}

static HRESULT WINAPI IDirectPlay8ServerImpl_EnumPlayersAndGroups(IDirectPlay8Server *iface, DPNID *prgdpnid, DWORD *pcdpnid, DWORD dwFlags)
{
    IDirectPlay8ServerImpl *This = impl_from_IDirectPlay8Server(iface);
    FIXME("(%p)->(%p %p %ld)\n", This, prgdpnid, pcdpnid, dwFlags);
    return E_NOTIMPL;
}

static HRESULT WINAPI IDirectPlay8ServerImpl_EnumGroupMembers(IDirectPlay8Server *iface, DPNID dpnid, DPNID *prgdpnid, DWORD *pcdpnid, DWORD dwFlags)
{
    IDirectPlay8ServerImpl *This = impl_from_IDirectPlay8Server(iface);
    FIXME("(%p)->(%ld %p %p %ld)\n", This, dpnid, prgdpnid, pcdpnid, dwFlags);
    return E_NOTIMPL;
}

static HRESULT WINAPI IDirectPlay8ServerImpl_Close(IDirectPlay8Server *iface, DWORD dwFlags)
{
    IDirectPlay8ServerImpl *This = impl_from_IDirectPlay8Server(iface);
    FIXME("(%p)->(%ld)\n", This, dwFlags);
    return E_NOTIMPL;
}

static HRESULT WINAPI IDirectPlay8ServerImpl_DestroyClient(IDirectPlay8Server *iface, DPNID dpnidClient, const void *pvDestroyData,
                                                    DWORD dwDestroyDataSize, DWORD dwFlags)
{
    IDirectPlay8ServerImpl *This = impl_from_IDirectPlay8Server(iface);
    FIXME("(%p)->(%ld %p %ld %ld)\n", This, dpnidClient, pvDestroyData, dwDestroyDataSize, dwFlags);
    return E_NOTIMPL;
}

static HRESULT WINAPI IDirectPlay8ServerImpl_ReturnBuffer(IDirectPlay8Server *iface, DPNHANDLE hBufferHandle, DWORD dwFlags)
{
    IDirectPlay8ServerImpl *This = impl_from_IDirectPlay8Server(iface);
    FIXME("(%p)->(%ld %ld)\n", This, hBufferHandle, dwFlags);
    return E_NOTIMPL;
}

static HRESULT WINAPI IDirectPlay8ServerImpl_GetPlayerContext(IDirectPlay8Server *iface, DPNID dpnid, PVOID *ppvPlayerContext, DWORD dwFlags)
{
    IDirectPlay8ServerImpl *This = impl_from_IDirectPlay8Server(iface);
    FIXME("(%p)->(%ld %p %ld)\n", This, dpnid, ppvPlayerContext, dwFlags);
    return E_NOTIMPL;
}

static HRESULT WINAPI IDirectPlay8ServerImpl_GetGroupContext(IDirectPlay8Server *iface, DPNID dpnid, PVOID *ppvGroupContext, DWORD dwFlags)
{
    IDirectPlay8ServerImpl *This = impl_from_IDirectPlay8Server(iface);
    FIXME("(%p)->(%ld %p %ld)\n", This, dpnid, ppvGroupContext, dwFlags);
    return E_NOTIMPL;
}

static HRESULT WINAPI IDirectPlay8ServerImpl_GetCaps(IDirectPlay8Server *iface, DPN_CAPS *pdpCaps, DWORD dwFlags)
{
    IDirectPlay8ServerImpl *This = impl_from_IDirectPlay8Server(iface);
    FIXME("(%p)->(%p %ld)\n", This, pdpCaps, dwFlags);
    return E_NOTIMPL;
}

static HRESULT WINAPI IDirectPlay8ServerImpl_SetCaps(IDirectPlay8Server *iface, const DPN_CAPS *pdpCaps, DWORD dwFlags)
{
    IDirectPlay8ServerImpl *This = impl_from_IDirectPlay8Server(iface);
    FIXME("(%p)->(%p %ld)\n", This, pdpCaps, dwFlags);
    return E_NOTIMPL;
}

static HRESULT WINAPI IDirectPlay8ServerImpl_SetSPCaps(IDirectPlay8Server *iface, const GUID *pguidSP, const DPN_SP_CAPS *pdpspCaps, DWORD dwFlags )
{
    IDirectPlay8ServerImpl *This = impl_from_IDirectPlay8Server(iface);
    FIXME("(%p)->(%p %p %ld)\n", This, pguidSP, pdpspCaps, dwFlags);
    return E_NOTIMPL;
}

static HRESULT WINAPI IDirectPlay8ServerImpl_GetSPCaps(IDirectPlay8Server *iface, const GUID *pguidSP, DPN_SP_CAPS *pdpspCaps, DWORD dwFlags)
{
    IDirectPlay8ServerImpl *This = impl_from_IDirectPlay8Server(iface);
    FIXME("(%p)->(%p %p %ld)\n", This, pguidSP, pdpspCaps, dwFlags);
    return E_NOTIMPL;
}

static HRESULT WINAPI IDirectPlay8ServerImpl_GetConnectionInfo(IDirectPlay8Server *iface, DPNID dpnid,
                                            DPN_CONNECTION_INFO *pdpConnectionInfo, DWORD dwFlags)
{
    IDirectPlay8ServerImpl *This = impl_from_IDirectPlay8Server(iface);
    FIXME("(%p)->(%ld %p %ld)\n", This, dpnid, pdpConnectionInfo, dwFlags);
    return E_NOTIMPL;
}

static HRESULT WINAPI IDirectPlay8ServerImpl_RegisterLobby(IDirectPlay8Server *iface, DPNHANDLE dpnHandle,
                                            struct IDirectPlay8LobbiedApplication *pIDP8LobbiedApplication, DWORD dwFlags)
{
    IDirectPlay8ServerImpl *This = impl_from_IDirectPlay8Server(iface);
    FIXME("(%p)->(%ld %p %ld)\n", This, dpnHandle, pIDP8LobbiedApplication, dwFlags);
    return E_NOTIMPL;
}

static const IDirectPlay8ServerVtbl DirectPlay8ServerVtbl =
{
    IDirectPlay8ServerImpl_QueryInterface,
    IDirectPlay8ServerImpl_AddRef,
    IDirectPlay8ServerImpl_Release,
    IDirectPlay8ServerImpl_Initialize,
    IDirectPlay8ServerImpl_EnumServiceProviders,
    IDirectPlay8ServerImpl_CancelAsyncOperation,
    IDirectPlay8ServerImpl_GetSendQueueInfo,
    IDirectPlay8ServerImpl_GetApplicationDesc,
    IDirectPlay8ServerImpl_SetServerInfo,
    IDirectPlay8ServerImpl_GetClientInfo,
    IDirectPlay8ServerImpl_GetClientAddress,
    IDirectPlay8ServerImpl_GetLocalHostAddresses,
    IDirectPlay8ServerImpl_SetApplicationDesc,
    IDirectPlay8ServerImpl_Host,
    IDirectPlay8ServerImpl_SendTo,
    IDirectPlay8ServerImpl_CreateGroup,
    IDirectPlay8ServerImpl_DestroyGroup,
    IDirectPlay8ServerImpl_AddPlayerToGroup,
    IDirectPlay8ServerImpl_RemovePlayerFromGroup,
    IDirectPlay8ServerImpl_SetGroupInfo,
    IDirectPlay8ServerImpl_GetGroupInfo,
    IDirectPlay8ServerImpl_EnumPlayersAndGroups,
    IDirectPlay8ServerImpl_EnumGroupMembers,
    IDirectPlay8ServerImpl_Close,
    IDirectPlay8ServerImpl_DestroyClient,
    IDirectPlay8ServerImpl_ReturnBuffer,
    IDirectPlay8ServerImpl_GetPlayerContext,
    IDirectPlay8ServerImpl_GetGroupContext,
    IDirectPlay8ServerImpl_GetCaps,
    IDirectPlay8ServerImpl_SetCaps,
    IDirectPlay8ServerImpl_SetSPCaps,
    IDirectPlay8ServerImpl_GetSPCaps,
    IDirectPlay8ServerImpl_GetConnectionInfo,
    IDirectPlay8ServerImpl_RegisterLobby
};

HRESULT DPNET_CreateDirectPlay8Server(IClassFactory *iface, IUnknown *pUnkOuter, REFIID riid, void **ppv)
{
    IDirectPlay8ServerImpl *server;
    HRESULT hr;

    TRACE("(%p, %s, %p)\n", pUnkOuter, debugstr_guid(riid), ppv);

    *ppv = NULL;

    if(pUnkOuter)
        return CLASS_E_NOAGGREGATION;

    server = calloc(1, sizeof(IDirectPlay8ServerImpl));
    if (!server)
        return E_OUTOFMEMORY;

    server->IDirectPlay8Server_iface.lpVtbl = &DirectPlay8ServerVtbl;
    server->ref = 1;
    server->usercontext = NULL;
    server->msghandler = NULL;
    server->flags = 0;

    hr = IDirectPlay8Server_QueryInterface(&server->IDirectPlay8Server_iface, riid, ppv);
    IDirectPlay8Server_Release(&server->IDirectPlay8Server_iface);

    return hr;
}
