/* 
 * DirectPlay8 Peer
 * 
 * Copyright 2004 Raphael Junqueira
 * Copyright 2008 Alexander N. Sørnes <alex@thehandofagony.com>
 * Copyright 2011 Louis Lenders
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


typedef struct IDirectPlay8PeerImpl
{
    IDirectPlay8Peer IDirectPlay8Peer_iface;
    LONG ref;

    PFNDPNMESSAGEHANDLER msghandler;
    DWORD flags;
    void *usercontext;

    WCHAR *username;
    void  *data;
    DWORD datasize;

    DPN_SP_CAPS spcaps;
} IDirectPlay8PeerImpl;

static inline IDirectPlay8PeerImpl *impl_from_IDirectPlay8Peer(IDirectPlay8Peer *iface)
{
    return CONTAINING_RECORD(iface, IDirectPlay8PeerImpl, IDirectPlay8Peer_iface);
}

/* IUnknown interface follows */
static HRESULT WINAPI IDirectPlay8PeerImpl_QueryInterface(IDirectPlay8Peer *iface, REFIID riid,
        void **ret_iface)
{
    if(IsEqualGUID(riid, &IID_IUnknown) || IsEqualGUID(riid, &IID_IDirectPlay8Peer))
    {
        IDirectPlay8Peer_AddRef(iface);
        *ret_iface = iface;
        return S_OK;
    }

    WARN("(%p)->(%s,%p): not found\n", iface, debugstr_guid(riid), ret_iface);
    return E_NOINTERFACE;
}

static ULONG WINAPI IDirectPlay8PeerImpl_AddRef(IDirectPlay8Peer *iface)
{
    IDirectPlay8PeerImpl* This = impl_from_IDirectPlay8Peer(iface);
    ULONG RefCount = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, RefCount);

    return RefCount;
}

static ULONG WINAPI IDirectPlay8PeerImpl_Release(IDirectPlay8Peer *iface)
{
    IDirectPlay8PeerImpl* This = impl_from_IDirectPlay8Peer(iface);
    ULONG RefCount = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, RefCount);

    if(!RefCount)
    {
        free(This->username);
        free(This->data);

        free(This);
    }

    return RefCount;
}

/* IDirectPlay8Peer interface follows */

static HRESULT WINAPI IDirectPlay8PeerImpl_Initialize(IDirectPlay8Peer *iface,
        void * const pvUserContext, const PFNDPNMESSAGEHANDLER pfn, const DWORD dwFlags)
{
    IDirectPlay8PeerImpl* This = impl_from_IDirectPlay8Peer(iface);

    TRACE("(%p)->(%p,%p,%lx): stub\n", iface, pvUserContext, pfn, dwFlags);

    if(!pfn)
        return DPNERR_INVALIDPARAM;

    This->usercontext = pvUserContext;
    This->msghandler = pfn;
    This->flags = dwFlags;

    init_winsock();

    return DPN_OK;
}

HRESULT enum_services_providers(const GUID * const service, DPN_SERVICE_PROVIDER_INFO * const info_buffer,
        DWORD * const buf_size, DWORD * const returned)
{
    static const WCHAR serviceproviders[] = L"SOFTWARE\\Microsoft\\DirectPlay8\\Service Providers";
    static const WCHAR friendly[] = L"Friendly Name";
    static const WCHAR dp_adapterW[] = L"Local Area Connection - IPv4";
    static const GUID adapter_guid = {0x4ce725f6, 0xd3c0, 0xdade, {0xba, 0x6f, 0x11, 0xf9, 0x65, 0xbc, 0x42, 0x99}};
    DWORD req_size = 0;
    LONG res;
    HKEY key = NULL;
    LONG next_key;
    DWORD index = 0;
    WCHAR provider[MAX_PATH];
    DWORD size;

    if(!returned || !buf_size)
        return E_POINTER;

    if(!service)
    {
        *returned = 0;

        res = RegOpenKeyExW(HKEY_LOCAL_MACHINE, serviceproviders, 0, KEY_READ, &key);
        if(res == ERROR_FILE_NOT_FOUND)
            return DPNERR_DOESNOTEXIST;

        next_key = RegEnumKeyW( key, index, provider, MAX_PATH);
        while(next_key == ERROR_SUCCESS)
        {
            size = 0;
            res = RegGetValueW(key, provider, friendly, RRF_RT_REG_SZ, NULL, NULL, &size);
            if(res == ERROR_SUCCESS)
            {
                 req_size += sizeof(DPN_SERVICE_PROVIDER_INFO) + size;

                 (*returned)++;
            }

            index++;
            next_key = RegEnumKeyW( key, index, provider, MAX_PATH );
        }

    }
    else if(IsEqualGUID(service, &CLSID_DP8SP_TCPIP))
    {
        req_size = sizeof(DPN_SERVICE_PROVIDER_INFO) + sizeof(dp_adapterW);
    }
    else
    {
        FIXME("Application requested a provider we don't handle (yet)\n");
        return DPNERR_DOESNOTEXIST;
    }

    if(*buf_size < req_size)
    {
        RegCloseKey(key);

        *buf_size = req_size;
        return DPNERR_BUFFERTOOSMALL;
    }

    if(!service)
    {
        int offset = 0;
        int count = 0;
        char *infoend = ((char *)info_buffer + (sizeof(DPN_SERVICE_PROVIDER_INFO) * (*returned)));

        index = 0;
        next_key = RegEnumKeyW( key, index, provider, MAX_PATH);
        while(next_key == ERROR_SUCCESS)
        {
            size = 0;
            res = RegGetValueW(key, provider, friendly, RRF_RT_REG_SZ, NULL, NULL, &size);
            if(res == ERROR_SUCCESS)
            {
                info_buffer[count].guid = CLSID_DP8SP_TCPIP;
                info_buffer[count].pwszName = (LPWSTR)(infoend + offset);

                RegGetValueW(key, provider, friendly, RRF_RT_REG_SZ, NULL, info_buffer[count].pwszName, &size);

                offset += size;
                count++;
            }

            index++;
            next_key = RegEnumKeyW(key, index, provider, MAX_PATH);
        }
    }
    else
    {
        info_buffer->pwszName = (LPWSTR)(info_buffer + 1);
        lstrcpyW(info_buffer->pwszName, dp_adapterW);
        info_buffer->guid = adapter_guid;
        *returned = 1;
    }

    RegCloseKey(key);

    return DPN_OK;
}

static HRESULT WINAPI IDirectPlay8PeerImpl_EnumServiceProviders(IDirectPlay8Peer *iface,
        const GUID * const pguidServiceProvider, const GUID * const pguidApplication,
        DPN_SERVICE_PROVIDER_INFO * const pSPInfoBuffer, DWORD * const pcbEnumData,
        DWORD * const pcReturned, const DWORD dwFlags)
{
    TRACE("(%p)->(%s,%s,%p,%p,%p,%lx)\n", iface, debugstr_guid(pguidServiceProvider), debugstr_guid(pguidApplication),
                                             pSPInfoBuffer, pcbEnumData, pcReturned, dwFlags);

    if(dwFlags)
        FIXME("Unhandled flags %lx\n", dwFlags);

    if(pguidApplication)
        FIXME("Application guid %s is currently being ignored\n", debugstr_guid(pguidApplication));

    return enum_services_providers(pguidServiceProvider, pSPInfoBuffer, pcbEnumData, pcReturned);
}

static HRESULT WINAPI IDirectPlay8PeerImpl_CancelAsyncOperation(IDirectPlay8Peer *iface,
        const DPNHANDLE hAsyncHandle, const DWORD dwFlags)
{
    FIXME("(%p)->(%lx,%lx): stub\n", iface, hAsyncHandle, dwFlags);

    return DPNERR_GENERIC;
}

static HRESULT WINAPI IDirectPlay8PeerImpl_Connect(IDirectPlay8Peer *iface,
        const DPN_APPLICATION_DESC * const pdnAppDesc, IDirectPlay8Address * const pHostAddr,
        IDirectPlay8Address * const pDeviceInfo, const DPN_SECURITY_DESC * const pdnSecurity,
        const DPN_SECURITY_CREDENTIALS * const pdnCredentials, const void * const pvUserConnectData,
        const DWORD dwUserConnectDataSize, void * const pvPlayerContext,
        void * const pvAsyncContext, DPNHANDLE * const phAsyncHandle, const DWORD dwFlags)
{
    FIXME("(%p)->(%p,%p,%p,%p,%p,%p,%lx,%p,%p,%p,%lx): stub\n", iface, pdnAppDesc, pHostAddr, pDeviceInfo, pdnSecurity, pdnCredentials, pvUserConnectData, dwUserConnectDataSize, pvPlayerContext, pvAsyncContext, phAsyncHandle, dwFlags);

    return DPNERR_GENERIC;
}

static HRESULT WINAPI IDirectPlay8PeerImpl_SendTo(IDirectPlay8Peer *iface, const DPNID dpnId,
        const DPN_BUFFER_DESC *pBufferDesc, const DWORD cBufferDesc, const DWORD dwTimeOut,
        void * const pvAsyncContext, DPNHANDLE * const phAsyncHandle, const DWORD dwFlags)
{
    FIXME("(%p)->(%lx,%p,%lx,%lx,%p,%p,%lx): stub\n", iface, dpnId, pBufferDesc, cBufferDesc, dwTimeOut, pvAsyncContext, phAsyncHandle, dwFlags);

    return DPNERR_GENERIC;
}

static HRESULT WINAPI IDirectPlay8PeerImpl_GetSendQueueInfo(IDirectPlay8Peer *iface,
        const DPNID dpnid, DWORD * const pdwNumMsgs, DWORD * const pdwNumBytes, const DWORD dwFlags)
{
    FIXME("(%p)->(%lx,%p,%p,%lx): stub\n", iface, dpnid, pdwNumMsgs, pdwNumBytes, dwFlags);

    return DPNERR_GENERIC;
}

static HRESULT WINAPI IDirectPlay8PeerImpl_Host(IDirectPlay8Peer *iface,
        const DPN_APPLICATION_DESC * const pdnAppDesc, IDirectPlay8Address ** const prgpDeviceInfo,
        const DWORD cDeviceInfo, const DPN_SECURITY_DESC * const pdpSecurity,
        const DPN_SECURITY_CREDENTIALS * const pdpCredentials, void * const pvPlayerContext,
        const DWORD dwFlags)
{
    FIXME("(%p)->(%p,%p,%lx,%p,%p,%p,%lx): stub\n", iface, pdnAppDesc, prgpDeviceInfo, cDeviceInfo, pdpSecurity, pdpCredentials, pvPlayerContext, dwFlags);

    return DPNERR_GENERIC;
}

static HRESULT WINAPI IDirectPlay8PeerImpl_GetApplicationDesc(IDirectPlay8Peer *iface,
        DPN_APPLICATION_DESC * const pAppDescBuffer, DWORD * const pcbDataSize, const DWORD dwFlags)
{
    FIXME("(%p)->(%p,%p,%lx): stub\n", iface, pAppDescBuffer, pcbDataSize, dwFlags);

    return DPNERR_GENERIC;
}

static HRESULT WINAPI IDirectPlay8PeerImpl_SetApplicationDesc(IDirectPlay8Peer *iface,
        const DPN_APPLICATION_DESC * const pad, const DWORD dwFlags)
{
    FIXME("(%p)->(%p,%lx): stub\n", iface, pad, dwFlags);

    return DPNERR_GENERIC;
}

static HRESULT WINAPI IDirectPlay8PeerImpl_CreateGroup(IDirectPlay8Peer *iface,
        const DPN_GROUP_INFO * const pdpnGroupInfo, void * const pvGroupContext,
        void * const pvAsyncContext, DPNHANDLE * const phAsyncHandle, const DWORD dwFlags)
{
    FIXME("(%p)->(%p,%p,%p,%p,%lx): stub\n", iface, pdpnGroupInfo, pvGroupContext, pvAsyncContext, phAsyncHandle, dwFlags);

    return DPNERR_GENERIC;
}

static HRESULT WINAPI IDirectPlay8PeerImpl_DestroyGroup(IDirectPlay8Peer *iface,
        const DPNID idGroup, void * const pvAsyncContext, DPNHANDLE * const phAsyncHandle,
        const DWORD dwFlags)
{
    FIXME("(%p)->(%lx,%p,%p,%lx): stub\n", iface, idGroup, pvAsyncContext, phAsyncHandle, dwFlags);

    return DPNERR_GENERIC;
}

static HRESULT WINAPI IDirectPlay8PeerImpl_AddPlayerToGroup(IDirectPlay8Peer *iface,
        const DPNID idGroup, const DPNID idClient, void * const pvAsyncContext,
        DPNHANDLE * const phAsyncHandle, const DWORD dwFlags)
{
    FIXME("(%p)->(%lx,%lx,%p,%p,%lx): stub\n", iface, idGroup, idClient, pvAsyncContext, phAsyncHandle, dwFlags);

    return DPNERR_GENERIC;
}

static HRESULT WINAPI IDirectPlay8PeerImpl_RemovePlayerFromGroup(IDirectPlay8Peer *iface,
        const DPNID idGroup, const DPNID idClient, void * const pvAsyncContext,
        DPNHANDLE * const phAsyncHandle, const DWORD dwFlags)
{
    FIXME("(%p)->(%lx,%lx,%p,%p,%lx): stub\n", iface, idGroup, idClient, pvAsyncContext, phAsyncHandle, dwFlags);

    return DPNERR_GENERIC;
}

static HRESULT WINAPI IDirectPlay8PeerImpl_SetGroupInfo(IDirectPlay8Peer *iface, const DPNID dpnid,
        DPN_GROUP_INFO * const pdpnGroupInfo, void * const pvAsyncContext,
        DPNHANDLE * const phAsyncHandle, const DWORD dwFlags)
{
    FIXME("(%p)->(%lx,%p,%p,%p,%lx): stub\n", iface, dpnid, pdpnGroupInfo, pvAsyncContext, phAsyncHandle, dwFlags);

    return DPNERR_GENERIC;
}

static HRESULT WINAPI IDirectPlay8PeerImpl_GetGroupInfo(IDirectPlay8Peer *iface, const DPNID dpnid,
        DPN_GROUP_INFO * const pdpnGroupInfo, DWORD * const pdwSize, const DWORD dwFlags)
{
    FIXME("(%p)->(%lx,%p,%p,%lx): stub\n", iface, dpnid, pdpnGroupInfo, pdwSize, dwFlags);

    return DPNERR_GENERIC;
}

static HRESULT WINAPI IDirectPlay8PeerImpl_EnumPlayersAndGroups(IDirectPlay8Peer *iface,
        DPNID * const prgdpnid, DWORD * const pcdpnid, const DWORD dwFlags)
{
    FIXME("(%p)->(%p,%p,%lx): stub\n", iface, prgdpnid, pcdpnid, dwFlags);

    return DPNERR_GENERIC;
}

static HRESULT WINAPI IDirectPlay8PeerImpl_EnumGroupMembers(IDirectPlay8Peer *iface,
        const DPNID dpnid, DPNID * const prgdpnid, DWORD * const pcdpnid, const DWORD dwFlags)
{
    FIXME("(%p)->(%lx,%p,%p,%lx): stub\n", iface, dpnid, prgdpnid, pcdpnid, dwFlags);

    return DPNERR_GENERIC;
}

static HRESULT WINAPI IDirectPlay8PeerImpl_SetPeerInfo(IDirectPlay8Peer *iface,
        const DPN_PLAYER_INFO * const pdpnPlayerInfo, void * const pvAsyncContext,
        DPNHANDLE * const phAsyncHandle, const DWORD dwFlags)
{
    IDirectPlay8PeerImpl* This = impl_from_IDirectPlay8Peer(iface);

    FIXME("(%p)->(%p,%p,%p,%lx) Semi-stub.\n", This, pdpnPlayerInfo, pvAsyncContext, phAsyncHandle, dwFlags);

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

        This->datasize = pdpnPlayerInfo->dwDataSize;
        This->data = malloc(pdpnPlayerInfo->dwDataSize);
        if (!This->data)
            return E_OUTOFMEMORY;

        memcpy(This->data, pdpnPlayerInfo->pvData, pdpnPlayerInfo->dwDataSize);
    }

    return S_OK;
}

static HRESULT WINAPI IDirectPlay8PeerImpl_GetPeerInfo(IDirectPlay8Peer *iface, const DPNID dpnid,
        DPN_PLAYER_INFO * const pdpnPlayerInfo, DWORD * const pdwSize, const DWORD dwFlags)
{
    FIXME("(%p)->(%lx,%p,%p,%lx): stub\n", iface, dpnid, pdpnPlayerInfo, pdwSize, dwFlags);

    return DPNERR_GENERIC;
}

static HRESULT WINAPI IDirectPlay8PeerImpl_GetPeerAddress(IDirectPlay8Peer *iface,
        const DPNID dpnid, IDirectPlay8Address ** const pAddress, const DWORD dwFlags)
{
    FIXME("(%p)->(%lx,%p,%lx): stub\n", iface, dpnid, pAddress, dwFlags);

    return DPNERR_GENERIC;
}

static HRESULT WINAPI IDirectPlay8PeerImpl_GetLocalHostAddresses(IDirectPlay8Peer *iface,
        IDirectPlay8Address ** const prgpAddress, DWORD * const pcAddress, const DWORD dwFlags)
{
    FIXME("(%p)->(%p,%p,%lx): stub\n", iface, prgpAddress, pcAddress, dwFlags);

    return DPNERR_GENERIC;
}

static HRESULT WINAPI IDirectPlay8PeerImpl_Close(IDirectPlay8Peer *iface, const DWORD dwFlags)
{
    FIXME("(%p)->(%lx): stub\n", iface, dwFlags);

    return DPN_OK;
}

static HRESULT WINAPI IDirectPlay8PeerImpl_EnumHosts(IDirectPlay8Peer *iface,
        PDPN_APPLICATION_DESC const pApplicationDesc, IDirectPlay8Address * const pAddrHost,
        IDirectPlay8Address * const pDeviceInfo, void * const pUserEnumData,
        const DWORD dwUserEnumDataSize, const DWORD dwEnumCount, const DWORD dwRetryInterval,
        const DWORD dwTimeOut, void * const pvUserContext, DPNHANDLE * const pAsyncHandle, const DWORD dwFlags)
{
    IDirectPlay8PeerImpl* This = impl_from_IDirectPlay8Peer(iface);

    FIXME("(%p)->(%p,%p,%p,%p,%lx,%lx,%lx,%lx,%p,%p,%lx): stub\n",
            This, pApplicationDesc, pAddrHost, pDeviceInfo, pUserEnumData, dwUserEnumDataSize, dwEnumCount,
            dwRetryInterval, dwTimeOut, pvUserContext, pAsyncHandle, dwFlags);

    if(!This->msghandler)
        return DPNERR_UNINITIALIZED;

    if((dwFlags & DPNENUMHOSTS_SYNC) && pAsyncHandle)
        return DPNERR_INVALIDPARAM;

    if(dwUserEnumDataSize > This->spcaps.dwMaxEnumPayloadSize)
        return DPNERR_ENUMQUERYTOOLARGE;

    return (dwFlags & DPNENUMHOSTS_SYNC) ? DPN_OK : DPNSUCCESS_PENDING;
}

static HRESULT WINAPI IDirectPlay8PeerImpl_DestroyPeer(IDirectPlay8Peer *iface, const DPNID dpnidClient,
        const void * const pvDestroyData, const DWORD dwDestroyDataSize, const DWORD dwFlags)
{
    FIXME("(%p)->(%lx,%p,%lx,%lx): stub\n", iface, dpnidClient, pvDestroyData, dwDestroyDataSize, dwFlags);

    return DPNERR_GENERIC;
}

static HRESULT WINAPI IDirectPlay8PeerImpl_ReturnBuffer(IDirectPlay8Peer *iface, const DPNHANDLE hBufferHandle,
        const DWORD dwFlags)
{
    FIXME("(%p)->(%lx,%lx): stub\n", iface, hBufferHandle, dwFlags);

    return DPNERR_GENERIC;
}

static HRESULT WINAPI IDirectPlay8PeerImpl_GetPlayerContext(IDirectPlay8Peer *iface, const DPNID dpnid,
        void ** const ppvPlayerContext, const DWORD dwFlags)
{
    FIXME("(%p)->(%lx,%p,%lx): stub\n", iface, dpnid, ppvPlayerContext, dwFlags);

    return DPNERR_GENERIC;
}

static HRESULT WINAPI IDirectPlay8PeerImpl_GetGroupContext(IDirectPlay8Peer *iface, const DPNID dpnid,
        void ** const ppvGroupContext, const DWORD dwFlags)
{
    FIXME("(%p)->(%lx,%p,%lx): stub\n", iface, dpnid, ppvGroupContext, dwFlags);

    return DPNERR_GENERIC;
}

static HRESULT WINAPI IDirectPlay8PeerImpl_GetCaps(IDirectPlay8Peer *iface, DPN_CAPS * const pdpCaps,
        const DWORD dwFlags)
{
    FIXME("(%p)->(%p,%lx): stub\n", iface, pdpCaps, dwFlags);

    return DPNERR_GENERIC;
}

static HRESULT WINAPI IDirectPlay8PeerImpl_SetCaps(IDirectPlay8Peer *iface, const DPN_CAPS * const pdpCaps,
        const DWORD dwFlags)
{
    FIXME("(%p)->(%p,%lx): stub\n", iface, pdpCaps, dwFlags);

    return DPNERR_GENERIC;
}

static HRESULT WINAPI IDirectPlay8PeerImpl_SetSPCaps(IDirectPlay8Peer *iface, const GUID * const pguidSP,
        const DPN_SP_CAPS * const pdpspCaps, const DWORD dwFlags )
{
    IDirectPlay8PeerImpl* This = impl_from_IDirectPlay8Peer(iface);

    TRACE("(%p)->(%p,%p,%lx): stub\n", iface, pguidSP, pdpspCaps, dwFlags);

    if(!This->msghandler || pdpspCaps->dwSize != sizeof(DPN_SP_CAPS))
        return DPNERR_INVALIDPARAM;

    /* Only dwSystemBufferSize is set by this call. */
    This->spcaps.dwSystemBufferSize = pdpspCaps->dwSystemBufferSize;

    return DPN_OK;
}

static HRESULT WINAPI IDirectPlay8PeerImpl_GetSPCaps(IDirectPlay8Peer *iface, const GUID * const pguidSP,
        DPN_SP_CAPS * const pdpspCaps, const DWORD dwFlags)
{
    IDirectPlay8PeerImpl* This = impl_from_IDirectPlay8Peer(iface);

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

static HRESULT WINAPI IDirectPlay8PeerImpl_GetConnectionInfo(IDirectPlay8Peer *iface, const DPNID dpnid,
        DPN_CONNECTION_INFO * const pdpConnectionInfo, const DWORD dwFlags)
{
    FIXME("(%p)->(%lx,%p,%lx): stub\n", iface, dpnid, pdpConnectionInfo, dwFlags);

    return DPNERR_GENERIC;
}

static HRESULT WINAPI IDirectPlay8PeerImpl_RegisterLobby(IDirectPlay8Peer *iface, const DPNHANDLE dpnHandle,
        struct IDirectPlay8LobbiedApplication * const pIDP8LobbiedApplication, const DWORD dwFlags)
{
    FIXME("(%p)->(%lx,%p,%lx): stub\n", iface, dpnHandle, pIDP8LobbiedApplication, dwFlags);

    return DPNERR_GENERIC;
}

static HRESULT WINAPI IDirectPlay8PeerImpl_TerminateSession(IDirectPlay8Peer *iface, void * const pvTerminateData,
        const DWORD dwTerminateDataSize, const DWORD dwFlags)
{
    FIXME("(%p)->(%p,%lx,%lx): stub\n", iface, pvTerminateData, dwTerminateDataSize, dwFlags);

    return DPNERR_GENERIC;
}

static const IDirectPlay8PeerVtbl DirectPlay8Peer_Vtbl =
{
    IDirectPlay8PeerImpl_QueryInterface,
    IDirectPlay8PeerImpl_AddRef,
    IDirectPlay8PeerImpl_Release,
    IDirectPlay8PeerImpl_Initialize,
    IDirectPlay8PeerImpl_EnumServiceProviders,
    IDirectPlay8PeerImpl_CancelAsyncOperation,
    IDirectPlay8PeerImpl_Connect,
    IDirectPlay8PeerImpl_SendTo,
    IDirectPlay8PeerImpl_GetSendQueueInfo,
    IDirectPlay8PeerImpl_Host,
    IDirectPlay8PeerImpl_GetApplicationDesc,
    IDirectPlay8PeerImpl_SetApplicationDesc,
    IDirectPlay8PeerImpl_CreateGroup,
    IDirectPlay8PeerImpl_DestroyGroup,
    IDirectPlay8PeerImpl_AddPlayerToGroup,
    IDirectPlay8PeerImpl_RemovePlayerFromGroup,
    IDirectPlay8PeerImpl_SetGroupInfo,
    IDirectPlay8PeerImpl_GetGroupInfo,
    IDirectPlay8PeerImpl_EnumPlayersAndGroups,
    IDirectPlay8PeerImpl_EnumGroupMembers,
    IDirectPlay8PeerImpl_SetPeerInfo,
    IDirectPlay8PeerImpl_GetPeerInfo,
    IDirectPlay8PeerImpl_GetPeerAddress,
    IDirectPlay8PeerImpl_GetLocalHostAddresses,
    IDirectPlay8PeerImpl_Close,
    IDirectPlay8PeerImpl_EnumHosts,
    IDirectPlay8PeerImpl_DestroyPeer,
    IDirectPlay8PeerImpl_ReturnBuffer,
    IDirectPlay8PeerImpl_GetPlayerContext,
    IDirectPlay8PeerImpl_GetGroupContext,
    IDirectPlay8PeerImpl_GetCaps,
    IDirectPlay8PeerImpl_SetCaps,
    IDirectPlay8PeerImpl_SetSPCaps,
    IDirectPlay8PeerImpl_GetSPCaps,
    IDirectPlay8PeerImpl_GetConnectionInfo,
    IDirectPlay8PeerImpl_RegisterLobby,
    IDirectPlay8PeerImpl_TerminateSession
};

void init_dpn_sp_caps(DPN_SP_CAPS *dpnspcaps)
{
    dpnspcaps->dwSize = sizeof(DPN_SP_CAPS);
    dpnspcaps->dwFlags = DPNSPCAPS_SUPPORTSDPNSRV | DPNSPCAPS_SUPPORTSBROADCAST |
                         DPNSPCAPS_SUPPORTSALLADAPTERS | DPNSPCAPS_SUPPORTSTHREADPOOL;
    dpnspcaps->dwNumThreads = 3;
    dpnspcaps->dwDefaultEnumCount = 5;
    dpnspcaps->dwDefaultEnumRetryInterval = 1500;
    dpnspcaps->dwDefaultEnumTimeout = 1500;
    dpnspcaps->dwMaxEnumPayloadSize = 983;
    dpnspcaps->dwBuffersPerThread = 1;
    dpnspcaps->dwSystemBufferSize = 0x10000;
};

HRESULT DPNET_CreateDirectPlay8Peer(IClassFactory *iface, IUnknown *pUnkOuter, REFIID riid, LPVOID *ppobj)
{
    IDirectPlay8PeerImpl* Client;
    HRESULT ret;

    Client = calloc(1, sizeof(IDirectPlay8PeerImpl));

    *ppobj = NULL;

    if(Client == NULL)
    {
        WARN("Not enough memory\n");
        return E_OUTOFMEMORY;
    }

    Client->IDirectPlay8Peer_iface.lpVtbl = &DirectPlay8Peer_Vtbl;
    Client->ref = 1;
    Client->usercontext = NULL;
    Client->msghandler = NULL;
    Client->flags = 0;

    init_dpn_sp_caps(&Client->spcaps);

    ret = IDirectPlay8Peer_QueryInterface(&Client->IDirectPlay8Peer_iface, riid, ppobj);
    IDirectPlay8Peer_Release(&Client->IDirectPlay8Peer_iface);

    return ret;
}
