/*
 * IDirect3DVolumeTexture9 implementation
 *
 * Copyright 2002-2005 Jason Edmeades
 *                     Raphael Junqueira
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
#include "d3d9_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d9);

/* IDirect3DVolumeTexture9 IUnknown parts follow: */
static HRESULT WINAPI IDirect3DVolumeTexture9Impl_QueryInterface(LPDIRECT3DVOLUMETEXTURE9 iface, REFIID riid, LPVOID* ppobj) {
    IDirect3DVolumeTexture9Impl *This = (IDirect3DVolumeTexture9Impl *)iface;

    TRACE("iface %p, riid %s, object %p.\n", iface, debugstr_guid(riid), ppobj);

    if (IsEqualGUID(riid, &IID_IUnknown)
    || IsEqualGUID(riid, &IID_IDirect3DResource9)
    || IsEqualGUID(riid, &IID_IDirect3DBaseTexture9)
    || IsEqualGUID(riid, &IID_IDirect3DVolumeTexture9)) {
        IDirect3DVolumeTexture9_AddRef(iface);
        *ppobj = This;
        return S_OK;
    }

    WARN("(%p)->(%s,%p),not found\n", This, debugstr_guid(riid), ppobj);
    *ppobj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI IDirect3DVolumeTexture9Impl_AddRef(LPDIRECT3DVOLUMETEXTURE9 iface) {
    IDirect3DVolumeTexture9Impl *This = (IDirect3DVolumeTexture9Impl *)iface;
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("%p increasing refcount to %u.\n", iface, ref);

    if (ref == 1)
    {
        IDirect3DDevice9Ex_AddRef(This->parentDevice);
        wined3d_mutex_lock();
        IWineD3DVolumeTexture_AddRef(This->wineD3DVolumeTexture);
        wined3d_mutex_unlock();
    }

    return ref;
}

static ULONG WINAPI IDirect3DVolumeTexture9Impl_Release(LPDIRECT3DVOLUMETEXTURE9 iface) {
    IDirect3DVolumeTexture9Impl *This = (IDirect3DVolumeTexture9Impl *)iface;
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("%p decreasing refcount to %u.\n", iface, ref);

    if (ref == 0) {
        IDirect3DDevice9Ex *parentDevice = This->parentDevice;

        wined3d_mutex_lock();
        IWineD3DVolumeTexture_Release(This->wineD3DVolumeTexture);
        wined3d_mutex_unlock();

        /* Release the device last, as it may cause the device to be destroyed. */
        IDirect3DDevice9Ex_Release(parentDevice);
    }
    return ref;
}

/* IDirect3DVolumeTexture9 IDirect3DResource9 Interface follow: */
static HRESULT WINAPI IDirect3DVolumeTexture9Impl_GetDevice(IDirect3DVolumeTexture9 *iface, IDirect3DDevice9 **device)
{
    IDirect3DVolumeTexture9Impl *This = (IDirect3DVolumeTexture9Impl *)iface;

    TRACE("iface %p, device %p.\n", iface, device);

    *device = (IDirect3DDevice9 *)This->parentDevice;
    IDirect3DDevice9_AddRef(*device);

    TRACE("Returning device %p.\n", *device);

    return D3D_OK;
}

static HRESULT WINAPI IDirect3DVolumeTexture9Impl_SetPrivateData(LPDIRECT3DVOLUMETEXTURE9 iface, REFGUID refguid, CONST void* pData, DWORD SizeOfData, DWORD Flags) {
    IDirect3DVolumeTexture9Impl *This = (IDirect3DVolumeTexture9Impl *)iface;
    HRESULT hr;

    TRACE("iface %p, guid %s, data %p, data_size %u, flags %#x.\n",
            iface, debugstr_guid(refguid), pData, SizeOfData, Flags);

    wined3d_mutex_lock();

    hr = IWineD3DVolumeTexture_SetPrivateData(This->wineD3DVolumeTexture, refguid, pData, SizeOfData, Flags);

    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DVolumeTexture9Impl_GetPrivateData(LPDIRECT3DVOLUMETEXTURE9 iface, REFGUID refguid, void* pData, DWORD* pSizeOfData) {
    IDirect3DVolumeTexture9Impl *This = (IDirect3DVolumeTexture9Impl *)iface;
    HRESULT hr;

    TRACE("iface %p, guid %s, data %p, data_size %p.\n",
            iface, debugstr_guid(refguid), pData, pSizeOfData);

    wined3d_mutex_lock();

    hr = IWineD3DVolumeTexture_GetPrivateData(This->wineD3DVolumeTexture, refguid, pData, pSizeOfData);

    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DVolumeTexture9Impl_FreePrivateData(LPDIRECT3DVOLUMETEXTURE9 iface, REFGUID refguid) {
    IDirect3DVolumeTexture9Impl *This = (IDirect3DVolumeTexture9Impl *)iface;
    HRESULT hr;

    TRACE("iface %p, guid %s.\n", iface, debugstr_guid(refguid));

    wined3d_mutex_lock();

    hr = IWineD3DVolumeTexture_FreePrivateData(This->wineD3DVolumeTexture, refguid);

    wined3d_mutex_unlock();

    return hr;
}

static DWORD WINAPI IDirect3DVolumeTexture9Impl_SetPriority(LPDIRECT3DVOLUMETEXTURE9 iface, DWORD PriorityNew) {
    IDirect3DVolumeTexture9Impl *This = (IDirect3DVolumeTexture9Impl *)iface;
    DWORD priority;

    TRACE("iface %p, priority %u.\n", iface, PriorityNew);

    wined3d_mutex_lock();

    priority = IWineD3DVolumeTexture_SetPriority(This->wineD3DVolumeTexture, PriorityNew);

    wined3d_mutex_unlock();

    return priority;
}

static DWORD WINAPI IDirect3DVolumeTexture9Impl_GetPriority(LPDIRECT3DVOLUMETEXTURE9 iface) {
    IDirect3DVolumeTexture9Impl *This = (IDirect3DVolumeTexture9Impl *)iface;
    DWORD priority;

    TRACE("iface %p.\n", iface);

    wined3d_mutex_lock();

    priority = IWineD3DVolumeTexture_GetPriority(This->wineD3DVolumeTexture);

    wined3d_mutex_unlock();

    return priority;
}

static void WINAPI IDirect3DVolumeTexture9Impl_PreLoad(LPDIRECT3DVOLUMETEXTURE9 iface) {
    IDirect3DVolumeTexture9Impl *This = (IDirect3DVolumeTexture9Impl *)iface;

    TRACE("iface %p.\n", iface);

    wined3d_mutex_lock();

    IWineD3DVolumeTexture_PreLoad(This->wineD3DVolumeTexture);

    wined3d_mutex_unlock();
}

static D3DRESOURCETYPE WINAPI IDirect3DVolumeTexture9Impl_GetType(LPDIRECT3DVOLUMETEXTURE9 iface) {
    IDirect3DVolumeTexture9Impl *This = (IDirect3DVolumeTexture9Impl *)iface;
    D3DRESOURCETYPE type;

    TRACE("iface %p.\n", iface);

    wined3d_mutex_lock();

    type = IWineD3DVolumeTexture_GetType(This->wineD3DVolumeTexture);

    wined3d_mutex_unlock();

    return type;
}

/* IDirect3DVolumeTexture9 IDirect3DBaseTexture9 Interface follow: */
static DWORD WINAPI IDirect3DVolumeTexture9Impl_SetLOD(LPDIRECT3DVOLUMETEXTURE9 iface, DWORD LODNew) {
    IDirect3DVolumeTexture9Impl *This = (IDirect3DVolumeTexture9Impl *)iface;
    DWORD lod;

    TRACE("iface %p, lod %u.\n", iface, LODNew);

    wined3d_mutex_lock();

    lod = IWineD3DVolumeTexture_SetLOD(This->wineD3DVolumeTexture, LODNew);

    wined3d_mutex_unlock();

    return lod;
}

static DWORD WINAPI IDirect3DVolumeTexture9Impl_GetLOD(LPDIRECT3DVOLUMETEXTURE9 iface) {
    IDirect3DVolumeTexture9Impl *This = (IDirect3DVolumeTexture9Impl *)iface;
    DWORD lod;

    TRACE("iface %p.\n", iface);

    wined3d_mutex_lock();

    lod = IWineD3DVolumeTexture_GetLOD(This->wineD3DVolumeTexture);

    wined3d_mutex_unlock();

    return lod;
}

static DWORD WINAPI IDirect3DVolumeTexture9Impl_GetLevelCount(LPDIRECT3DVOLUMETEXTURE9 iface) {
    IDirect3DVolumeTexture9Impl *This = (IDirect3DVolumeTexture9Impl *)iface;
    DWORD level_count;

    TRACE("iface %p.\n", iface);

    wined3d_mutex_lock();

    level_count = IWineD3DVolumeTexture_GetLevelCount(This->wineD3DVolumeTexture);

    wined3d_mutex_unlock();

    return level_count;
}

static HRESULT WINAPI IDirect3DVolumeTexture9Impl_SetAutoGenFilterType(LPDIRECT3DVOLUMETEXTURE9 iface, D3DTEXTUREFILTERTYPE FilterType) {
    IDirect3DVolumeTexture9Impl *This = (IDirect3DVolumeTexture9Impl *)iface;
    HRESULT hr;

    TRACE("iface %p, filter_type %#x.\n", iface, FilterType);

    wined3d_mutex_lock();

    hr = IWineD3DVolumeTexture_SetAutoGenFilterType(This->wineD3DVolumeTexture, (WINED3DTEXTUREFILTERTYPE) FilterType);

    wined3d_mutex_unlock();

    return hr;
}

static D3DTEXTUREFILTERTYPE WINAPI IDirect3DVolumeTexture9Impl_GetAutoGenFilterType(LPDIRECT3DVOLUMETEXTURE9 iface) {
    IDirect3DVolumeTexture9Impl *This = (IDirect3DVolumeTexture9Impl *)iface;
    D3DTEXTUREFILTERTYPE filter_type;

    TRACE("iface %p.\n", iface);

    wined3d_mutex_lock();

    filter_type = (D3DTEXTUREFILTERTYPE)IWineD3DVolumeTexture_GetAutoGenFilterType(This->wineD3DVolumeTexture);

    wined3d_mutex_unlock();

    return filter_type;
}

static void WINAPI IDirect3DVolumeTexture9Impl_GenerateMipSubLevels(LPDIRECT3DVOLUMETEXTURE9 iface) {
    IDirect3DVolumeTexture9Impl *This = (IDirect3DVolumeTexture9Impl *)iface;

    TRACE("iface %p.\n", iface);

    wined3d_mutex_lock();

    IWineD3DVolumeTexture_GenerateMipSubLevels(This->wineD3DVolumeTexture);

    wined3d_mutex_unlock();
}

/* IDirect3DVolumeTexture9 Interface follow: */
static HRESULT WINAPI IDirect3DVolumeTexture9Impl_GetLevelDesc(LPDIRECT3DVOLUMETEXTURE9 iface, UINT Level, D3DVOLUME_DESC* pDesc) {
    IDirect3DVolumeTexture9Impl *This = (IDirect3DVolumeTexture9Impl *)iface;
    WINED3DVOLUME_DESC     wined3ddesc;
    HRESULT hr;

    TRACE("iface %p, level %u, desc %p.\n", iface, Level, pDesc);

    wined3d_mutex_lock();

    hr = IWineD3DVolumeTexture_GetLevelDesc(This->wineD3DVolumeTexture, Level, &wined3ddesc);

    wined3d_mutex_unlock();

    if (SUCCEEDED(hr))
    {
        pDesc->Format = d3dformat_from_wined3dformat(wined3ddesc.Format);
        pDesc->Type = wined3ddesc.Type;
        pDesc->Usage = wined3ddesc.Usage;
        pDesc->Pool = wined3ddesc.Pool;
        pDesc->Width = wined3ddesc.Width;
        pDesc->Height = wined3ddesc.Height;
        pDesc->Depth = wined3ddesc.Depth;
    }

    return hr;
}

static HRESULT WINAPI IDirect3DVolumeTexture9Impl_GetVolumeLevel(IDirect3DVolumeTexture9 *iface,
        UINT Level, IDirect3DVolume9 **ppVolumeLevel)
{
    IDirect3DVolumeTexture9Impl *This = (IDirect3DVolumeTexture9Impl *)iface;
    IWineD3DVolume *myVolume = NULL;
    HRESULT hr;

    TRACE("iface %p, level %u, volume %p.\n", iface, Level, ppVolumeLevel);

    wined3d_mutex_lock();

    hr = IWineD3DVolumeTexture_GetVolumeLevel(This->wineD3DVolumeTexture, Level, &myVolume);
    if (SUCCEEDED(hr) && ppVolumeLevel)
    {
       *ppVolumeLevel = IWineD3DVolumeTexture_GetParent(myVolume);
       IDirect3DVolumeTexture9_AddRef(*ppVolumeLevel);
       IWineD3DVolumeTexture_Release(myVolume);
    }

    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DVolumeTexture9Impl_LockBox(LPDIRECT3DVOLUMETEXTURE9 iface, UINT Level, D3DLOCKED_BOX* pLockedVolume, CONST D3DBOX* pBox, DWORD Flags) {
    IDirect3DVolumeTexture9Impl *This = (IDirect3DVolumeTexture9Impl *)iface;
    HRESULT hr;

    TRACE("iface %p, level %u, locked_box %p, box %p, flags %#x.\n",
            iface, Level, pLockedVolume, pBox, Flags);

    wined3d_mutex_lock();

    hr = IWineD3DVolumeTexture_LockBox(This->wineD3DVolumeTexture, Level, (WINED3DLOCKED_BOX *)pLockedVolume,
            (const WINED3DBOX *)pBox, Flags);

    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DVolumeTexture9Impl_UnlockBox(LPDIRECT3DVOLUMETEXTURE9 iface, UINT Level) {
    IDirect3DVolumeTexture9Impl *This = (IDirect3DVolumeTexture9Impl *)iface;
    HRESULT hr;

    TRACE("iface %p, level %u.\n", iface, Level);

    wined3d_mutex_lock();

    hr = IWineD3DVolumeTexture_UnlockBox(This->wineD3DVolumeTexture, Level);

    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DVolumeTexture9Impl_AddDirtyBox(LPDIRECT3DVOLUMETEXTURE9 iface, CONST D3DBOX* pDirtyBox) {
    IDirect3DVolumeTexture9Impl *This = (IDirect3DVolumeTexture9Impl *)iface;
    HRESULT hr;

    TRACE("iface %p, dirty_box %p.\n", iface, pDirtyBox);

    wined3d_mutex_lock();

    hr = IWineD3DVolumeTexture_AddDirtyBox(This->wineD3DVolumeTexture, (CONST WINED3DBOX *)pDirtyBox);

    wined3d_mutex_unlock();

    return hr;
}


static const IDirect3DVolumeTexture9Vtbl Direct3DVolumeTexture9_Vtbl =
{
    /* IUnknown */
    IDirect3DVolumeTexture9Impl_QueryInterface,
    IDirect3DVolumeTexture9Impl_AddRef,
    IDirect3DVolumeTexture9Impl_Release,
    /* IDirect3DResource9 */
    IDirect3DVolumeTexture9Impl_GetDevice,
    IDirect3DVolumeTexture9Impl_SetPrivateData,
    IDirect3DVolumeTexture9Impl_GetPrivateData,
    IDirect3DVolumeTexture9Impl_FreePrivateData,
    IDirect3DVolumeTexture9Impl_SetPriority,
    IDirect3DVolumeTexture9Impl_GetPriority,
    IDirect3DVolumeTexture9Impl_PreLoad,
    IDirect3DVolumeTexture9Impl_GetType,
    /* IDirect3DBaseTexture9 */
    IDirect3DVolumeTexture9Impl_SetLOD,
    IDirect3DVolumeTexture9Impl_GetLOD,
    IDirect3DVolumeTexture9Impl_GetLevelCount,
    IDirect3DVolumeTexture9Impl_SetAutoGenFilterType,
    IDirect3DVolumeTexture9Impl_GetAutoGenFilterType,
    IDirect3DVolumeTexture9Impl_GenerateMipSubLevels,
    /* IDirect3DVolumeTexture9 */
    IDirect3DVolumeTexture9Impl_GetLevelDesc,
    IDirect3DVolumeTexture9Impl_GetVolumeLevel,
    IDirect3DVolumeTexture9Impl_LockBox,
    IDirect3DVolumeTexture9Impl_UnlockBox,
    IDirect3DVolumeTexture9Impl_AddDirtyBox
};

static void STDMETHODCALLTYPE volumetexture_wined3d_object_destroyed(void *parent)
{
    HeapFree(GetProcessHeap(), 0, parent);
}

static const struct wined3d_parent_ops d3d9_volumetexture_wined3d_parent_ops =
{
    volumetexture_wined3d_object_destroyed,
};

HRESULT volumetexture_init(IDirect3DVolumeTexture9Impl *texture, IDirect3DDevice9Impl *device,
        UINT width, UINT height, UINT depth, UINT levels, DWORD usage, D3DFORMAT format, D3DPOOL pool)
{
    HRESULT hr;

    texture->lpVtbl = &Direct3DVolumeTexture9_Vtbl;
    texture->ref = 1;

    wined3d_mutex_lock();
    hr = IWineD3DDevice_CreateVolumeTexture(device->WineD3DDevice, width, height, depth, levels,
            usage & WINED3DUSAGE_MASK, wined3dformat_from_d3dformat(format), pool, texture,
            &d3d9_volumetexture_wined3d_parent_ops, &texture->wineD3DVolumeTexture);
    wined3d_mutex_unlock();
    if (FAILED(hr))
    {
        WARN("Failed to create wined3d volume texture, hr %#x.\n", hr);
        return hr;
    }

    texture->parentDevice = (IDirect3DDevice9Ex *)device;
    IDirect3DDevice9Ex_AddRef(texture->parentDevice);

    return D3D_OK;
}
