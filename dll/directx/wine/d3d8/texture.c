/*
 * IDirect3DTexture8 implementation
 *
 * Copyright 2005 Oliver Stieber
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

#include "config.h"
#include "d3d8_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d8);

/* IDirect3DTexture8 IUnknown parts follow: */
static HRESULT WINAPI IDirect3DTexture8Impl_QueryInterface(LPDIRECT3DTEXTURE8 iface, REFIID riid, LPVOID *ppobj) {
    IDirect3DTexture8Impl *This = (IDirect3DTexture8Impl *)iface;

    TRACE("iface %p, riid %s, object %p.\n", iface, debugstr_guid(riid), ppobj);

    if (IsEqualGUID(riid, &IID_IUnknown)
        || IsEqualGUID(riid, &IID_IDirect3DResource8)
        || IsEqualGUID(riid, &IID_IDirect3DBaseTexture8)
        || IsEqualGUID(riid, &IID_IDirect3DTexture8)) {
        IUnknown_AddRef(iface);
        *ppobj = This;
        return S_OK;
    }

    WARN("(%p)->(%s,%p) not found\n", This, debugstr_guid(riid), ppobj);
    *ppobj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI IDirect3DTexture8Impl_AddRef(LPDIRECT3DTEXTURE8 iface) {
    IDirect3DTexture8Impl *This = (IDirect3DTexture8Impl *)iface;
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("%p increasing refcount to %u.\n", iface, ref);

    if (ref == 1)
    {
        IDirect3DDevice8_AddRef(This->parentDevice);
        wined3d_mutex_lock();
        IWineD3DTexture_AddRef(This->wineD3DTexture);
        wined3d_mutex_unlock();
    }

    return ref;
}

static ULONG WINAPI IDirect3DTexture8Impl_Release(LPDIRECT3DTEXTURE8 iface) {
    IDirect3DTexture8Impl *This = (IDirect3DTexture8Impl *)iface;
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("%p decreasing refcount to %u.\n", iface, ref);

    if (ref == 0) {
        IDirect3DDevice8 *parentDevice = This->parentDevice;

        wined3d_mutex_lock();
        IWineD3DTexture_Release(This->wineD3DTexture);
        wined3d_mutex_unlock();

        /* Release the device last, as it may cause the device to be destroyed. */
        IDirect3DDevice8_Release(parentDevice);
    }
    return ref;
}

/* IDirect3DTexture8 IDirect3DResource8 Interface follow: */
static HRESULT WINAPI IDirect3DTexture8Impl_GetDevice(IDirect3DTexture8 *iface, IDirect3DDevice8 **device)
{
    IDirect3DTexture8Impl *This = (IDirect3DTexture8Impl *)iface;

    TRACE("iface %p, device %p.\n", iface, device);

    *device = (IDirect3DDevice8 *)This->parentDevice;
    IDirect3DDevice8_AddRef(*device);

    TRACE("Returning device %p.\n", *device);

    return D3D_OK;
}

static HRESULT WINAPI IDirect3DTexture8Impl_SetPrivateData(LPDIRECT3DTEXTURE8 iface, REFGUID refguid, CONST void *pData, DWORD SizeOfData, DWORD Flags) {
    IDirect3DTexture8Impl *This = (IDirect3DTexture8Impl *)iface;
    HRESULT hr;

    TRACE("iface %p, guid %s, data %p, data_size %u, flags %#x.\n",
            iface, debugstr_guid(refguid), pData, SizeOfData, Flags);

    wined3d_mutex_lock();
    hr = IWineD3DTexture_SetPrivateData(This->wineD3DTexture, refguid, pData, SizeOfData, Flags);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DTexture8Impl_GetPrivateData(LPDIRECT3DTEXTURE8 iface, REFGUID refguid, void *pData, DWORD* pSizeOfData) {
    IDirect3DTexture8Impl *This = (IDirect3DTexture8Impl *)iface;
    HRESULT hr;

    TRACE("iface %p, guid %s, data %p, data_size %p.\n",
            iface, debugstr_guid(refguid), pData, pSizeOfData);

    wined3d_mutex_lock();
    hr = IWineD3DTexture_GetPrivateData(This->wineD3DTexture, refguid, pData, pSizeOfData);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DTexture8Impl_FreePrivateData(LPDIRECT3DTEXTURE8 iface, REFGUID refguid) {
    IDirect3DTexture8Impl *This = (IDirect3DTexture8Impl *)iface;
    HRESULT hr;

    TRACE("iface %p, guid %s.\n", iface, debugstr_guid(refguid));

    wined3d_mutex_lock();
    hr = IWineD3DTexture_FreePrivateData(This->wineD3DTexture, refguid);
    wined3d_mutex_unlock();

    return hr;
}

static DWORD WINAPI IDirect3DTexture8Impl_SetPriority(LPDIRECT3DTEXTURE8 iface, DWORD PriorityNew) {
    IDirect3DTexture8Impl *This = (IDirect3DTexture8Impl *)iface;
    DWORD ret;

    TRACE("iface %p, priority %u.\n", iface, PriorityNew);

    wined3d_mutex_lock();
    ret = IWineD3DTexture_SetPriority(This->wineD3DTexture, PriorityNew);
    wined3d_mutex_unlock();

    return ret;
}

static DWORD WINAPI IDirect3DTexture8Impl_GetPriority(LPDIRECT3DTEXTURE8 iface) {
    IDirect3DTexture8Impl *This = (IDirect3DTexture8Impl *)iface;
    DWORD ret;

    TRACE("iface %p.\n", iface);

    wined3d_mutex_lock();
    ret = IWineD3DTexture_GetPriority(This->wineD3DTexture);
    wined3d_mutex_unlock();

    return ret;
}

static void WINAPI IDirect3DTexture8Impl_PreLoad(LPDIRECT3DTEXTURE8 iface) {
    IDirect3DTexture8Impl *This = (IDirect3DTexture8Impl *)iface;

    TRACE("iface %p.\n", iface);

    wined3d_mutex_lock();
    IWineD3DTexture_PreLoad(This->wineD3DTexture);
    wined3d_mutex_unlock();
}

static D3DRESOURCETYPE WINAPI IDirect3DTexture8Impl_GetType(LPDIRECT3DTEXTURE8 iface) {
    IDirect3DTexture8Impl *This = (IDirect3DTexture8Impl *)iface;
    D3DRESOURCETYPE type;

    TRACE("iface %p.\n", iface);

    wined3d_mutex_lock();
    type = IWineD3DTexture_GetType(This->wineD3DTexture);
    wined3d_mutex_unlock();

    return type;
}

/* IDirect3DTexture8 IDirect3DBaseTexture8 Interface follow: */
static DWORD WINAPI IDirect3DTexture8Impl_SetLOD(LPDIRECT3DTEXTURE8 iface, DWORD LODNew) {
    IDirect3DTexture8Impl *This = (IDirect3DTexture8Impl *)iface;
    DWORD ret;

    TRACE("iface %p, lod %u.\n", iface, LODNew);

    wined3d_mutex_lock();
    ret = IWineD3DTexture_SetLOD(This->wineD3DTexture, LODNew);
    wined3d_mutex_unlock();

    return ret;
}

static DWORD WINAPI IDirect3DTexture8Impl_GetLOD(LPDIRECT3DTEXTURE8 iface) {
    IDirect3DTexture8Impl *This = (IDirect3DTexture8Impl *)iface;
    DWORD ret;

    TRACE("iface %p.\n", iface);

    wined3d_mutex_lock();
    ret = IWineD3DTexture_GetLOD(This->wineD3DTexture);
    wined3d_mutex_unlock();

    return ret;
}

static DWORD WINAPI IDirect3DTexture8Impl_GetLevelCount(LPDIRECT3DTEXTURE8 iface) {
    IDirect3DTexture8Impl *This = (IDirect3DTexture8Impl *)iface;
    DWORD ret;

    TRACE("iface %p.\n", iface);

    wined3d_mutex_lock();
    ret = IWineD3DTexture_GetLevelCount(This->wineD3DTexture);
    wined3d_mutex_unlock();

    return ret;
}

/* IDirect3DTexture8 Interface follow: */
static HRESULT WINAPI IDirect3DTexture8Impl_GetLevelDesc(LPDIRECT3DTEXTURE8 iface, UINT Level, D3DSURFACE_DESC *pDesc) {
    IDirect3DTexture8Impl *This = (IDirect3DTexture8Impl *)iface;
    WINED3DSURFACE_DESC wined3ddesc;
    HRESULT hr;

    TRACE("iface %p, level %u, desc %p.\n", iface, Level, pDesc);

    wined3d_mutex_lock();
    hr = IWineD3DTexture_GetLevelDesc(This->wineD3DTexture, Level, &wined3ddesc);
    wined3d_mutex_unlock();

    if (SUCCEEDED(hr))
    {
        pDesc->Format = d3dformat_from_wined3dformat(wined3ddesc.format);
        pDesc->Type = wined3ddesc.resource_type;
        pDesc->Usage = wined3ddesc.usage;
        pDesc->Pool = wined3ddesc.pool;
        pDesc->Size = wined3ddesc.size;
        pDesc->MultiSampleType = wined3ddesc.multisample_type;
        pDesc->Width = wined3ddesc.width;
        pDesc->Height = wined3ddesc.height;
    }

    return hr;
}

static HRESULT WINAPI IDirect3DTexture8Impl_GetSurfaceLevel(IDirect3DTexture8 *iface,
        UINT Level, IDirect3DSurface8 **ppSurfaceLevel)
{
    IDirect3DTexture8Impl *This = (IDirect3DTexture8Impl *)iface;
    IWineD3DSurface *mySurface = NULL;
    HRESULT hr;

    TRACE("iface %p, level %u, surface %p.\n", iface, Level, ppSurfaceLevel);

    wined3d_mutex_lock();
    hr = IWineD3DTexture_GetSurfaceLevel(This->wineD3DTexture, Level, &mySurface);
    if (SUCCEEDED(hr) && ppSurfaceLevel)
    {
       *ppSurfaceLevel = IWineD3DSurface_GetParent(mySurface);
       IDirect3DSurface8_AddRef(*ppSurfaceLevel);
       IWineD3DSurface_Release(mySurface);
    }
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DTexture8Impl_LockRect(LPDIRECT3DTEXTURE8 iface, UINT Level, D3DLOCKED_RECT *pLockedRect, CONST RECT *pRect, DWORD Flags) {
    IDirect3DTexture8Impl *This = (IDirect3DTexture8Impl *)iface;
    HRESULT hr;

    TRACE("iface %p, level %u, locked_rect %p, rect %p, flags %#x.\n",
            iface, Level, pLockedRect, pRect, Flags);

    wined3d_mutex_lock();
    hr = IWineD3DTexture_LockRect(This->wineD3DTexture, Level, (WINED3DLOCKED_RECT *) pLockedRect, pRect, Flags);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DTexture8Impl_UnlockRect(LPDIRECT3DTEXTURE8 iface, UINT Level) {
    IDirect3DTexture8Impl *This = (IDirect3DTexture8Impl *)iface;
    HRESULT hr;

    TRACE("iface %p, level %u.\n", iface, Level);

    wined3d_mutex_lock();
    hr = IWineD3DTexture_UnlockRect(This->wineD3DTexture, Level);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DTexture8Impl_AddDirtyRect(LPDIRECT3DTEXTURE8 iface, CONST RECT *pDirtyRect) {
    IDirect3DTexture8Impl *This = (IDirect3DTexture8Impl *)iface;
    HRESULT hr;

    TRACE("iface %p, dirty_rect %p.\n", iface, pDirtyRect);

    wined3d_mutex_lock();
    hr = IWineD3DTexture_AddDirtyRect(This->wineD3DTexture, pDirtyRect);
    wined3d_mutex_unlock();

    return hr;
}

static const IDirect3DTexture8Vtbl Direct3DTexture8_Vtbl =
{
    /* IUnknown */
    IDirect3DTexture8Impl_QueryInterface,
    IDirect3DTexture8Impl_AddRef,
    IDirect3DTexture8Impl_Release,
     /* IDirect3DResource8 */
    IDirect3DTexture8Impl_GetDevice,
    IDirect3DTexture8Impl_SetPrivateData,
    IDirect3DTexture8Impl_GetPrivateData,
    IDirect3DTexture8Impl_FreePrivateData,
    IDirect3DTexture8Impl_SetPriority,
    IDirect3DTexture8Impl_GetPriority,
    IDirect3DTexture8Impl_PreLoad,
    IDirect3DTexture8Impl_GetType,
    /* IDirect3dBaseTexture8 */
    IDirect3DTexture8Impl_SetLOD,
    IDirect3DTexture8Impl_GetLOD,
    IDirect3DTexture8Impl_GetLevelCount,
    /* IDirect3DTexture8 */
    IDirect3DTexture8Impl_GetLevelDesc,
    IDirect3DTexture8Impl_GetSurfaceLevel,
    IDirect3DTexture8Impl_LockRect,
    IDirect3DTexture8Impl_UnlockRect,
    IDirect3DTexture8Impl_AddDirtyRect
};

static void STDMETHODCALLTYPE d3d8_texture_wined3d_object_destroyed(void *parent)
{
    HeapFree(GetProcessHeap(), 0, parent);
}

static const struct wined3d_parent_ops d3d8_texture_wined3d_parent_ops =
{
    d3d8_texture_wined3d_object_destroyed,
};

HRESULT texture_init(IDirect3DTexture8Impl *texture, IDirect3DDevice8Impl *device,
        UINT width, UINT height, UINT levels, DWORD usage, D3DFORMAT format, D3DPOOL pool)
{
    HRESULT hr;

    texture->lpVtbl = &Direct3DTexture8_Vtbl;
    texture->ref = 1;

    wined3d_mutex_lock();
    hr = IWineD3DDevice_CreateTexture(device->WineD3DDevice, width, height, levels,
            usage & WINED3DUSAGE_MASK, wined3dformat_from_d3dformat(format), pool,
            texture, &d3d8_texture_wined3d_parent_ops, &texture->wineD3DTexture);
    wined3d_mutex_unlock();
    if (FAILED(hr))
    {
        WARN("Failed to create wined3d texture, hr %#x.\n", hr);
        return hr;
    }

    texture->parentDevice = (IDirect3DDevice8 *)device;
    IDirect3DDevice8_AddRef(texture->parentDevice);

    return D3D_OK;
}
