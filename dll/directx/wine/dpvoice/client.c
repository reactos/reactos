 /*
 * DirectPlay Voice Client Interface
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
#include "dvoice_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(dpvoice);

typedef struct IDirectPlayVoiceClientImpl
{
    IDirectPlayVoiceClient IDirectPlayVoiceClient_iface;
    LONG ref;
} IDirectPlayVoiceClientImpl;

typedef struct IDirectPlayVoiceTestImpl
{
    IDirectPlayVoiceTest IDirectPlayVoiceTest_iface;
    LONG ref;
} IDirectPlayVoiceTestImpl;

static inline IDirectPlayVoiceClientImpl *impl_from_IDirectPlayVoiceClient(IDirectPlayVoiceClient *iface)
{
    return CONTAINING_RECORD(iface, IDirectPlayVoiceClientImpl, IDirectPlayVoiceClient_iface);
}

static inline IDirectPlayVoiceTestImpl *impl_from_IDirectPlayVoiceTest(IDirectPlayVoiceTest *iface)
{
    return CONTAINING_RECORD(iface, IDirectPlayVoiceTestImpl, IDirectPlayVoiceTest_iface);
}

static HRESULT WINAPI dpvclient_QueryInterface(IDirectPlayVoiceClient *iface, REFIID riid, void **ppv)
{
    if (IsEqualGUID(riid, &IID_IUnknown) || IsEqualGUID(riid, &IID_IDirectPlayVoiceClient))
    {
        IUnknown_AddRef(iface);
        *ppv = iface;
        return S_OK;
    }

    WARN("(%p)->(%s,%p),not found\n", iface, debugstr_guid(riid), ppv);
    return E_NOINTERFACE;
}

static ULONG WINAPI dpvclient_AddRef(IDirectPlayVoiceClient *iface)
{
    IDirectPlayVoiceClientImpl *This = impl_from_IDirectPlayVoiceClient(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%lu\n", This, ref);

    return ref;
}

static ULONG WINAPI dpvclient_Release(IDirectPlayVoiceClient *iface)
{
    IDirectPlayVoiceClientImpl *This = impl_from_IDirectPlayVoiceClient(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%lu\n", This, ref);

    if (!ref)
    {
        HeapFree(GetProcessHeap(), 0, This);
    }
    return ref;
}

static HRESULT WINAPI dpvclient_Initialize(IDirectPlayVoiceClient *iface,LPUNKNOWN pVoid, PDVMESSAGEHANDLER pMessageHandler,
                                void *pUserContext, DWORD *pdwMessageMask, DWORD dwMessageMaskElements)
{
    IDirectPlayVoiceClientImpl *This = impl_from_IDirectPlayVoiceClient(iface);
    FIXME("%p %p %p %p %p %ld\n", This, pVoid, pMessageHandler, pUserContext,pdwMessageMask, dwMessageMaskElements);
    return E_NOTIMPL;
}

static HRESULT WINAPI dpvclient_Connect(IDirectPlayVoiceClient *iface, PDVSOUNDDEVICECONFIG pSoundDeviceConfig,
                                PDVCLIENTCONFIG pdvClientConfig, DWORD dwFlags)
{
    IDirectPlayVoiceClientImpl *This = impl_from_IDirectPlayVoiceClient(iface);
    FIXME("%p %p %p %ld\n", This, pSoundDeviceConfig, pdvClientConfig, dwFlags);
    return E_NOTIMPL;
}

static HRESULT WINAPI dpvclient_Disconnect(IDirectPlayVoiceClient *iface, DWORD dwFlags)
{
    IDirectPlayVoiceClientImpl *This = impl_from_IDirectPlayVoiceClient(iface);
    FIXME("%p %ld\n", This, dwFlags);
    return E_NOTIMPL;
}

static HRESULT WINAPI dpvclient_GetSessionDesc(IDirectPlayVoiceClient *iface, PDVSESSIONDESC pvSessionDesc)
{
    IDirectPlayVoiceClientImpl *This = impl_from_IDirectPlayVoiceClient(iface);
    FIXME("%p %p\n", This, pvSessionDesc);
    return E_NOTIMPL;
}

static HRESULT WINAPI dpvclient_GetClientConfig(IDirectPlayVoiceClient *iface, PDVCLIENTCONFIG pClientConfig)
{
    IDirectPlayVoiceClientImpl *This = impl_from_IDirectPlayVoiceClient(iface);
    FIXME("%p %p\n", This, pClientConfig);
    return E_NOTIMPL;
}

static HRESULT WINAPI dpvclient_SetClientConfig(IDirectPlayVoiceClient *iface, PDVCLIENTCONFIG pClientConfig)
{
    IDirectPlayVoiceClientImpl *This = impl_from_IDirectPlayVoiceClient(iface);
    FIXME("%p %p\n", This, pClientConfig);
    return E_NOTIMPL;
}

static HRESULT WINAPI dpvclient_GetCaps(IDirectPlayVoiceClient *iface, PDVCAPS pCaps)
{
    IDirectPlayVoiceClientImpl *This = impl_from_IDirectPlayVoiceClient(iface);
    FIXME("%p %p\n", This, pCaps);
    return E_NOTIMPL;
}

static HRESULT WINAPI dpvclient_GetCompressionTypes(IDirectPlayVoiceClient *iface, void *pData,
                                DWORD *pdwDataSize, DWORD *pdwNumElements, DWORD dwFlags)
{
    IDirectPlayVoiceClientImpl *This = impl_from_IDirectPlayVoiceClient(iface);
    FIXME("%p %p %p %p %ld semi-stub\n", This, pData, pdwDataSize, pdwNumElements, dwFlags);
    return DPVOICE_GetCompressionTypes(pData, pdwDataSize, pdwNumElements, dwFlags);
}

static HRESULT WINAPI dpvclient_SetTransmitTargets(IDirectPlayVoiceClient *iface, PDVID pdvIDTargets,
                                DWORD dwNumTargets, DWORD dwFlags)
{
    IDirectPlayVoiceClientImpl *This = impl_from_IDirectPlayVoiceClient(iface);
    FIXME("%p %p %ld %ld\n", This, pdvIDTargets, dwNumTargets, dwFlags);
    return E_NOTIMPL;
}

static HRESULT WINAPI dpvclient_GetTransmitTargets(IDirectPlayVoiceClient *iface, PDVID pdvIDTargets,
                                DWORD *pdwNumTargets, DWORD dwFlags)
{
    IDirectPlayVoiceClientImpl *This = impl_from_IDirectPlayVoiceClient(iface);
    FIXME("%p %p %ld\n", This, pdwNumTargets, dwFlags);
    return E_NOTIMPL;
}

static HRESULT WINAPI dpvclient_Create3DSoundBuffer(IDirectPlayVoiceClient *iface, DVID dvID,
                                LPDIRECTSOUNDBUFFER lpdsSourceBuffer, DWORD dwPriority, DWORD dwFlags,
                                LPDIRECTSOUND3DBUFFER *lpUserBuffer)
{
    IDirectPlayVoiceClientImpl *This = impl_from_IDirectPlayVoiceClient(iface);
    FIXME("%p %ld %p %ld %ld %p\n", This, dvID, lpdsSourceBuffer, dwPriority, dwFlags, lpUserBuffer);
    return E_NOTIMPL;
}

static HRESULT WINAPI dpvclient_Delete3DSoundBuffer(IDirectPlayVoiceClient *iface, DVID dvID, LPDIRECTSOUND3DBUFFER *lpUserBuffer)
{
    IDirectPlayVoiceClientImpl *This = impl_from_IDirectPlayVoiceClient(iface);
    FIXME("%p %ld %p\n", This, dvID, lpUserBuffer);
    return E_NOTIMPL;
}

static HRESULT WINAPI dpvclient_SetNotifyMask(IDirectPlayVoiceClient *iface, DWORD *pdwMessageMask, DWORD dwMessageMaskElements)
{
    IDirectPlayVoiceClientImpl *This = impl_from_IDirectPlayVoiceClient(iface);
    FIXME("%p %p %ld\n", This, pdwMessageMask, dwMessageMaskElements);
    return E_NOTIMPL;
}

static HRESULT WINAPI dpvclient_GetSoundDeviceConfig(IDirectPlayVoiceClient *iface, PDVSOUNDDEVICECONFIG pSoundDeviceConfig,
                                DWORD *pdwSize)
{
    IDirectPlayVoiceClientImpl *This = impl_from_IDirectPlayVoiceClient(iface);
    FIXME("%p %p %p\n", This, pSoundDeviceConfig, pdwSize);
    return E_NOTIMPL;
}


static const IDirectPlayVoiceClientVtbl DirectPlayVoiceClient_Vtbl =
{
    dpvclient_QueryInterface,
    dpvclient_AddRef,
    dpvclient_Release,
    dpvclient_Initialize,
    dpvclient_Connect,
    dpvclient_Disconnect,
    dpvclient_GetSessionDesc,
    dpvclient_GetClientConfig,
    dpvclient_SetClientConfig,
    dpvclient_GetCaps,
    dpvclient_GetCompressionTypes,
    dpvclient_SetTransmitTargets,
    dpvclient_GetTransmitTargets,
    dpvclient_Create3DSoundBuffer,
    dpvclient_Delete3DSoundBuffer,
    dpvclient_SetNotifyMask,
    dpvclient_GetSoundDeviceConfig
};

HRESULT DPVOICE_CreateDirectPlayVoiceClient(IClassFactory *iface, IUnknown *pUnkOuter, REFIID riid, void **ppobj)
{
    IDirectPlayVoiceClientImpl* client;
    HRESULT ret;

    TRACE("(%p, %s, %p)\n", pUnkOuter, debugstr_guid(riid), ppobj);

    *ppobj = NULL;

    client = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirectPlayVoiceClientImpl));
    if (!client)
        return E_OUTOFMEMORY;

    client->IDirectPlayVoiceClient_iface.lpVtbl = &DirectPlayVoiceClient_Vtbl;
    client->ref = 1;

    ret = IDirectPlayVoiceClient_QueryInterface(&client->IDirectPlayVoiceClient_iface, riid, ppobj);
    IDirectPlayVoiceClient_Release(&client->IDirectPlayVoiceClient_iface);

    return ret;
}

static HRESULT WINAPI dpvtest_QueryInterface(IDirectPlayVoiceTest *iface, REFIID riid, void **ppv)
{
    if (IsEqualGUID(riid, &IID_IUnknown) || IsEqualGUID(riid, &IID_IDirectPlayVoiceTest))
    {
        IUnknown_AddRef(iface);
        *ppv = iface;
        return S_OK;
    }

    WARN("(%p)->(%s,%p),not found\n", iface, debugstr_guid(riid), ppv);
    return E_NOINTERFACE;
}

static ULONG WINAPI dpvtest_AddRef(IDirectPlayVoiceTest *iface)
{
    IDirectPlayVoiceTestImpl *This = impl_from_IDirectPlayVoiceTest(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%lu\n", This, ref);

    return ref;
}

static ULONG WINAPI dpvtest_Release(IDirectPlayVoiceTest *iface)
{
    IDirectPlayVoiceTestImpl *This = impl_from_IDirectPlayVoiceTest(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%lu\n", This, ref);

    if (!ref)
    {
        HeapFree(GetProcessHeap(), 0, This);
    }
    return ref;
}

static HRESULT WINAPI dpvtest_CheckAudioSetup(IDirectPlayVoiceTest *iface, const GUID *pguidPlaybackDevice, const GUID *pguidCaptureDevice,
                     HWND hwndParent, DWORD dwFlags)
{
    IDirectPlayVoiceTestImpl *This = impl_from_IDirectPlayVoiceTest(iface);
    FIXME("%p %s %s %p %ld\n", This, debugstr_guid(pguidPlaybackDevice),
            debugstr_guid(pguidCaptureDevice), hwndParent, dwFlags);
    return E_NOTIMPL;
}

static const IDirectPlayVoiceTestVtbl DirectPlayVoiceTest_Vtbl =
{
    dpvtest_QueryInterface,
    dpvtest_AddRef,
    dpvtest_Release,
    dpvtest_CheckAudioSetup
};

HRESULT DPVOICE_CreateDirectPlayVoiceTest(IClassFactory *iface, IUnknown *pUnkOuter, REFIID riid, void **ppobj)
{
    IDirectPlayVoiceTestImpl* test;
    HRESULT ret;

    TRACE("(%p, %s, %p)\n", pUnkOuter, debugstr_guid(riid), ppobj);

    *ppobj = NULL;

    test = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirectPlayVoiceTestImpl));
    if (!test)
        return E_OUTOFMEMORY;

    test->IDirectPlayVoiceTest_iface.lpVtbl = &DirectPlayVoiceTest_Vtbl;
    test->ref = 1;

    ret = IDirectPlayVoiceTest_QueryInterface(&test->IDirectPlayVoiceTest_iface, riid, ppobj);
    IDirectPlayVoiceTest_Release(&test->IDirectPlayVoiceTest_iface);

    return ret;
}
