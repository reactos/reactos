 /*
 * DirectPlay Voice Server Interface
 *
 * Copyright (C) 2014 Alistair Leslie-Hughes
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
#include "wingdi.h"
#include "winuser.h"
#include "objbase.h"

#include "wine/debug.h"

#include "dvoice.h"

WINE_DEFAULT_DEBUG_CHANNEL(dpvoice);

typedef struct IDirectPlayVoiceServerImpl
{
    IDirectPlayVoiceServer IDirectPlayVoiceServer_iface;
    LONG ref;
} IDirectPlayVoiceServerImpl;

HRESULT DPVOICE_GetCompressionTypes(DVCOMPRESSIONINFO *pData, DWORD *pdwDataSize, DWORD *pdwNumElements, DWORD dwFlags)
{
    static const DVCOMPRESSIONINFO pcm_type =
        {80, {0x8de12fd4,0x7cb3,0x48ce,{0xa7,0xe8,0x9c,0x47,0xa2,0x2e,0x8a,0xc5}}, NULL, NULL, 0, 64000};
    static const WCHAR pcm_name[] = L"MS-PCM 64 kbit/s";

    HRESULT ret;
    LPWSTR string_loc;

    if (!pdwDataSize || !pdwNumElements)
        return DVERR_INVALIDPOINTER;

    if (dwFlags)
        return DVERR_INVALIDFLAGS;

    *pdwNumElements = 1;

    if (*pdwDataSize < sizeof(pcm_type) + sizeof(pcm_name))
    {
        ret = DVERR_BUFFERTOOSMALL;
    }
    else if (!pData)
    {
        ret = DVERR_INVALIDPOINTER;
    }
    else
    {
        string_loc = (LPWSTR)((char*)pData + sizeof(pcm_type));
        memcpy(pData, &pcm_type, sizeof(pcm_type));
        memcpy(string_loc, pcm_name, sizeof(pcm_name));
        pData->lpszName = string_loc;
        ret = DV_OK;
    }

    *pdwDataSize = sizeof(pcm_type) + sizeof(pcm_name);
    return ret;
}

static inline IDirectPlayVoiceServerImpl *impl_from_IDirectPlayVoiceServer(IDirectPlayVoiceServer *iface)
{
    return CONTAINING_RECORD(iface, IDirectPlayVoiceServerImpl, IDirectPlayVoiceServer_iface);
}

static HRESULT WINAPI dpvserver_QueryInterface(IDirectPlayVoiceServer *iface, REFIID riid, void **ppv)
{
    if (IsEqualGUID(riid, &IID_IUnknown) || IsEqualGUID(riid, &IID_IDirectPlayVoiceServer))
    {
        IUnknown_AddRef(iface);
        *ppv = iface;
        return S_OK;
    }

    WARN("(%p)->(%s,%p),not found\n", iface, debugstr_guid(riid), ppv);
    return E_NOINTERFACE;
}

static ULONG WINAPI dpvserver_AddRef(IDirectPlayVoiceServer *iface)
{
    IDirectPlayVoiceServerImpl *This = impl_from_IDirectPlayVoiceServer(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%lu\n", This, ref);

    return ref;
}

static ULONG WINAPI dpvserver_Release(IDirectPlayVoiceServer *iface)
{
    IDirectPlayVoiceServerImpl *This = impl_from_IDirectPlayVoiceServer(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%lu\n", This, ref);

    if (!ref)
    {
        HeapFree(GetProcessHeap(), 0, This);
    }
    return ref;
}

static HRESULT WINAPI dpvserver_Initialize(IDirectPlayVoiceServer *iface, IUnknown *lpVoid, PDVMESSAGEHANDLER pMessageHandler,
                                void *pUserContext, DWORD *lpdwMessageMask, DWORD dwMessageMaskElements)
{
    IDirectPlayVoiceServerImpl *This = impl_from_IDirectPlayVoiceServer(iface);
    FIXME("%p %p %p %p %p %ld\n", This, lpVoid, pMessageHandler, pUserContext,lpdwMessageMask, dwMessageMaskElements);
    return E_NOTIMPL;
}

static HRESULT WINAPI dpvserver_StartSession(IDirectPlayVoiceServer *iface, PDVSESSIONDESC pSessionDesc, DWORD dwFlags)
{
    IDirectPlayVoiceServerImpl *This = impl_from_IDirectPlayVoiceServer(iface);
    FIXME("%p %p %ld\n", This, pSessionDesc, dwFlags);
    return E_NOTIMPL;
}

static HRESULT WINAPI dpvserver_StopSession(IDirectPlayVoiceServer *iface, DWORD dwFlags)
{
    IDirectPlayVoiceServerImpl *This = impl_from_IDirectPlayVoiceServer(iface);
    FIXME("%p %ld\n", This, dwFlags);
    return E_NOTIMPL;
}

static HRESULT WINAPI dpvserver_GetSessionDesc(IDirectPlayVoiceServer *iface, PDVSESSIONDESC pvSessionDesc)
{
    IDirectPlayVoiceServerImpl *This = impl_from_IDirectPlayVoiceServer(iface);
    FIXME("%p %p\n", This, pvSessionDesc);
    return E_NOTIMPL;
}

static HRESULT WINAPI dpvserver_SetSessionDesc(IDirectPlayVoiceServer *iface, PDVSESSIONDESC pSessionDesc)
{
    IDirectPlayVoiceServerImpl *This = impl_from_IDirectPlayVoiceServer(iface);
    FIXME("%p %p\n", This, pSessionDesc);
    return E_NOTIMPL;
}

static HRESULT WINAPI dpvserver_GetCaps(IDirectPlayVoiceServer *iface, PDVCAPS pDVCaps)
{
    IDirectPlayVoiceServerImpl *This = impl_from_IDirectPlayVoiceServer(iface);
    FIXME("%p %p\n", This, pDVCaps);
    return E_NOTIMPL;
}

static HRESULT WINAPI dpvserver_GetCompressionTypes(IDirectPlayVoiceServer *iface, void *pData, DWORD *pdwDataSize,
                                DWORD *pdwNumElements, DWORD dwFlags)
{
    IDirectPlayVoiceServerImpl *This = impl_from_IDirectPlayVoiceServer(iface);
    FIXME("%p %p %p %p %ld semi-stub\n", This, pData, pdwDataSize, pdwNumElements, dwFlags);
    return DPVOICE_GetCompressionTypes(pData, pdwDataSize, pdwNumElements, dwFlags);
}

static HRESULT WINAPI dpvserver_SetTransmitTargets(IDirectPlayVoiceServer *iface, DVID dvSource, PDVID pdvIDTargets,
                                DWORD dwNumTargets, DWORD dwFlags)
{
    IDirectPlayVoiceServerImpl *This = impl_from_IDirectPlayVoiceServer(iface);
    FIXME("%p %ld %p %ld %ld\n", This, dvSource, pdvIDTargets, dwNumTargets, dwFlags);
    return E_NOTIMPL;
}

static HRESULT WINAPI dpvserver_GetTransmitTargets(IDirectPlayVoiceServer *iface, DVID dvSource, PDVID pdvIDTargets,
                                DWORD *pdwNumTargets, DWORD dwFlags)
{
    IDirectPlayVoiceServerImpl *This = impl_from_IDirectPlayVoiceServer(iface);
    FIXME("%p %ld %p %p %ld\n", This, dvSource, pdvIDTargets, pdwNumTargets, dwFlags);
    return E_NOTIMPL;
}

static HRESULT WINAPI dpvserver_SetNotifyMask(IDirectPlayVoiceServer *iface, DWORD *pdwMessageMask, DWORD dwMessageMaskElements)
{
    IDirectPlayVoiceServerImpl *This = impl_from_IDirectPlayVoiceServer(iface);
    FIXME("%p %p %ld\n", This, pdwMessageMask, dwMessageMaskElements);
    return E_NOTIMPL;
}

static const IDirectPlayVoiceServerVtbl DirectPlayVoiceServer_Vtbl =
{
    dpvserver_QueryInterface,
    dpvserver_AddRef,
    dpvserver_Release,
    dpvserver_Initialize,
    dpvserver_StartSession,
    dpvserver_StopSession,
    dpvserver_GetSessionDesc,
    dpvserver_SetSessionDesc,
    dpvserver_GetCaps,
    dpvserver_GetCompressionTypes,
    dpvserver_SetTransmitTargets,
    dpvserver_GetTransmitTargets,
    dpvserver_SetNotifyMask
};

HRESULT DPVOICE_CreateDirectPlayVoiceServer(IClassFactory *iface, IUnknown *pUnkOuter, REFIID riid, void **ppobj)
{
    IDirectPlayVoiceServerImpl* server;
    HRESULT ret;

    TRACE("(%p, %s, %p)\n", pUnkOuter, debugstr_guid(riid), ppobj);

    *ppobj = NULL;

    server = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirectPlayVoiceServerImpl));
    if (!server)
        return E_OUTOFMEMORY;

    server->IDirectPlayVoiceServer_iface.lpVtbl = &DirectPlayVoiceServer_Vtbl;
    server->ref = 1;

    ret = IDirectPlayVoiceServer_QueryInterface(&server->IDirectPlayVoiceServer_iface, riid, ppobj);
    IDirectPlayVoiceServer_Release(&server->IDirectPlayVoiceServer_iface);

    return ret;
}
