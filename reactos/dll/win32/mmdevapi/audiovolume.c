/*
 * Copyright 2010 Maarten Lankhorst for CodeWeavers
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

#include "mmdevapi.h"

static const IAudioEndpointVolumeExVtbl AEVImpl_Vtbl;

typedef struct AEVImpl {
    IAudioEndpointVolumeEx IAudioEndpointVolumeEx_iface;
    LONG ref;
    float level;
    BOOL mute;
} AEVImpl;

static inline AEVImpl *impl_from_IAudioEndpointVolumeEx(IAudioEndpointVolumeEx *iface)
{
    return CONTAINING_RECORD(iface, AEVImpl, IAudioEndpointVolumeEx_iface);
}

HRESULT AudioEndpointVolume_Create(MMDevice *parent, IAudioEndpointVolume **ppv)
{
    AEVImpl *This;
    This = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*This));
    *ppv = (IAudioEndpointVolume*)This;
    if (!This)
        return E_OUTOFMEMORY;
    This->IAudioEndpointVolumeEx_iface.lpVtbl = &AEVImpl_Vtbl;
    This->ref = 1;
    This->level = 1.0f;
    This->mute = FALSE;
    return S_OK;
}

static void AudioEndpointVolume_Destroy(AEVImpl *This)
{
    HeapFree(GetProcessHeap(), 0, This);
}

static HRESULT WINAPI AEV_QueryInterface(IAudioEndpointVolumeEx *iface, REFIID riid, void **ppv)
{
    AEVImpl *This = impl_from_IAudioEndpointVolumeEx(iface);
    TRACE("(%p)->(%s,%p)\n", This, debugstr_guid(riid), ppv);
    if (!ppv)
        return E_POINTER;
    *ppv = NULL;
    if (IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_IAudioEndpointVolume) ||
        IsEqualIID(riid, &IID_IAudioEndpointVolumeEx)) {
        *ppv = This;
    }
    else
        return E_NOINTERFACE;
    IUnknown_AddRef((IUnknown *)*ppv);
    return S_OK;
}

static ULONG WINAPI AEV_AddRef(IAudioEndpointVolumeEx *iface)
{
    AEVImpl *This = impl_from_IAudioEndpointVolumeEx(iface);
    ULONG ref = InterlockedIncrement(&This->ref);
    TRACE("(%p) new ref %u\n", This, ref);
    return ref;
}

static ULONG WINAPI AEV_Release(IAudioEndpointVolumeEx *iface)
{
    AEVImpl *This = impl_from_IAudioEndpointVolumeEx(iface);
    ULONG ref = InterlockedDecrement(&This->ref);
    TRACE("(%p) new ref %u\n", This, ref);
    if (!ref)
        AudioEndpointVolume_Destroy(This);
    return ref;
}

static HRESULT WINAPI AEV_RegisterControlChangeNotify(IAudioEndpointVolumeEx *iface, IAudioEndpointVolumeCallback *notify)
{
    TRACE("(%p)->(%p)\n", iface, notify);
    if (!notify)
        return E_POINTER;
    FIXME("stub\n");
    return S_OK;
}

static HRESULT WINAPI AEV_UnregisterControlChangeNotify(IAudioEndpointVolumeEx *iface, IAudioEndpointVolumeCallback *notify)
{
    TRACE("(%p)->(%p)\n", iface, notify);
    if (!notify)
        return E_POINTER;
    FIXME("stub\n");
    return S_OK;
}

static HRESULT WINAPI AEV_GetChannelCount(IAudioEndpointVolumeEx *iface, UINT *count)
{
    TRACE("(%p)->(%p)\n", iface, count);
    if (!count)
        return E_POINTER;
    FIXME("stub\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI AEV_SetMasterVolumeLevel(IAudioEndpointVolumeEx *iface, float leveldb, const GUID *ctx)
{
    AEVImpl *This = impl_from_IAudioEndpointVolumeEx(iface);

    FIXME("(%p)->(%f,%s): stub\n", iface, leveldb, debugstr_guid(ctx));

    This->level = leveldb;

    return S_OK;
}

static HRESULT WINAPI AEV_SetMasterVolumeLevelScalar(IAudioEndpointVolumeEx *iface, float level, const GUID *ctx)
{
    TRACE("(%p)->(%f,%s)\n", iface, level, debugstr_guid(ctx));
    FIXME("stub\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI AEV_GetMasterVolumeLevel(IAudioEndpointVolumeEx *iface, float *leveldb)
{
    AEVImpl *This = impl_from_IAudioEndpointVolumeEx(iface);

    FIXME("(%p)->(%p): stub\n", iface, leveldb);

    if (!leveldb)
        return E_POINTER;

    *leveldb = This->level;

    return S_OK;
}

static HRESULT WINAPI AEV_GetMasterVolumeLevelScalar(IAudioEndpointVolumeEx *iface, float *level)
{
    TRACE("(%p)->(%p)\n", iface, level);
    if (!level)
        return E_POINTER;
    FIXME("stub\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI AEV_SetChannelVolumeLevel(IAudioEndpointVolumeEx *iface, UINT chan, float leveldb, const GUID *ctx)
{
    TRACE("(%p)->(%f,%s)\n", iface, leveldb, debugstr_guid(ctx));
    FIXME("stub\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI AEV_SetChannelVolumeLevelScalar(IAudioEndpointVolumeEx *iface, UINT chan, float level, const GUID *ctx)
{
    TRACE("(%p)->(%u,%f,%s)\n", iface, chan, level, debugstr_guid(ctx));
    FIXME("stub\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI AEV_GetChannelVolumeLevel(IAudioEndpointVolumeEx *iface, UINT chan, float *leveldb)
{
    TRACE("(%p)->(%u,%p)\n", iface, chan, leveldb);
    if (!leveldb)
        return E_POINTER;
    FIXME("stub\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI AEV_GetChannelVolumeLevelScalar(IAudioEndpointVolumeEx *iface, UINT chan, float *level)
{
    TRACE("(%p)->(%u,%p)\n", iface, chan, level);
    if (!level)
        return E_POINTER;
    FIXME("stub\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI AEV_SetMute(IAudioEndpointVolumeEx *iface, BOOL mute, const GUID *ctx)
{
    AEVImpl *This = impl_from_IAudioEndpointVolumeEx(iface);

    FIXME("(%p)->(%u,%s): stub\n", iface, mute, debugstr_guid(ctx));

    This->mute = mute;

    return S_OK;
}

static HRESULT WINAPI AEV_GetMute(IAudioEndpointVolumeEx *iface, BOOL *mute)
{
    AEVImpl *This = impl_from_IAudioEndpointVolumeEx(iface);

    FIXME("(%p)->(%p): stub\n", iface, mute);

    if (!mute)
        return E_POINTER;

    *mute = This->mute;

    return S_OK;
}

static HRESULT WINAPI AEV_GetVolumeStepInfo(IAudioEndpointVolumeEx *iface, UINT *stepsize, UINT *stepcount)
{
    TRACE("(%p)->(%p,%p)\n", iface, stepsize, stepcount);
    if (!stepsize && !stepcount)
        return E_POINTER;
    FIXME("stub\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI AEV_VolumeStepUp(IAudioEndpointVolumeEx *iface, const GUID *ctx)
{
    TRACE("(%p)->(%s)\n", iface, debugstr_guid(ctx));
    FIXME("stub\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI AEV_VolumeStepDown(IAudioEndpointVolumeEx *iface, const GUID *ctx)
{
    TRACE("(%p)->(%s)\n", iface, debugstr_guid(ctx));
    FIXME("stub\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI AEV_QueryHardwareSupport(IAudioEndpointVolumeEx *iface, DWORD *mask)
{
    TRACE("(%p)->(%p)\n", iface, mask);
    if (!mask)
        return E_POINTER;
    FIXME("stub\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI AEV_GetVolumeRange(IAudioEndpointVolumeEx *iface, float *mindb, float *maxdb, float *inc)
{
    FIXME("(%p)->(%p,%p,%p): stub\n", iface, mindb, maxdb, inc);

    if (!mindb || !maxdb || !inc)
        return E_POINTER;

    *mindb = 0.0f;
    *maxdb = 1.0f;
    *inc = 0.1f;

    return S_OK;
}

static HRESULT WINAPI AEV_GetVolumeRangeChannel(IAudioEndpointVolumeEx *iface, UINT chan, float *mindb, float *maxdb, float *inc)
{
    TRACE("(%p)->(%p,%p,%p)\n", iface, mindb, maxdb, inc);
    if (!mindb || !maxdb || !inc)
        return E_POINTER;
    FIXME("stub\n");
    return E_NOTIMPL;
}

static const IAudioEndpointVolumeExVtbl AEVImpl_Vtbl = {
    AEV_QueryInterface,
    AEV_AddRef,
    AEV_Release,
    AEV_RegisterControlChangeNotify,
    AEV_UnregisterControlChangeNotify,
    AEV_GetChannelCount,
    AEV_SetMasterVolumeLevel,
    AEV_SetMasterVolumeLevelScalar,
    AEV_GetMasterVolumeLevel,
    AEV_GetMasterVolumeLevelScalar,
    AEV_SetChannelVolumeLevel,
    AEV_SetChannelVolumeLevelScalar,
    AEV_GetChannelVolumeLevel,
    AEV_GetChannelVolumeLevelScalar,
    AEV_SetMute,
    AEV_GetMute,
    AEV_GetVolumeStepInfo,
    AEV_VolumeStepUp,
    AEV_VolumeStepDown,
    AEV_QueryHardwareSupport,
    AEV_GetVolumeRange,
    AEV_GetVolumeRangeChannel
};
