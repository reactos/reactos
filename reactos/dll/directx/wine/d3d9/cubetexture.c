/*
 * IDirect3DCubeTexture9 implementation
 *
 * Copyright 2002-2005 Jason Edmeades
 * Copyright 2002-2005 Raphael Junqueira
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
#include "d3d9_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d9);


/* IDirect3DCubeTexture9 IUnknown parts follow: */
static HRESULT WINAPI IDirect3DCubeTexture9Impl_QueryInterface(LPDIRECT3DCUBETEXTURE9 iface, REFIID riid, LPVOID* ppobj) {
    IDirect3DCubeTexture9Impl *This = (IDirect3DCubeTexture9Impl *)iface;

    TRACE("iface %p, riid %s, object %p.\n", iface, debugstr_guid(riid), ppobj);

    if (IsEqualGUID(riid, &IID_IUnknown)
        || IsEqualGUID(riid, &IID_IDirect3DResource9)
        || IsEqualGUID(riid, &IID_IDirect3DBaseTexture9)
        || IsEqualGUID(riid, &IID_IDirect3DCubeTexture9)) {
        IDirect3DCubeTexture9_AddRef(iface);
        *ppobj = This;
        return S_OK;
    }

    WARN("(%p)->(%s,%p),not found\n", This, debugstr_guid(riid), ppobj);
    *ppobj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI IDirect3DCubeTexture9Impl_AddRef(LPDIRECT3DCUBETEXTURE9 iface) {
    IDirect3DCubeTexture9Impl *This = (IDirect3DCubeTexture9Impl *)iface;
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("%p increasing refcount to %u.\n", iface, ref);

    if (ref == 1)
    {
        IDirect3DDevice9Ex_AddRef(This->parentDevice);
        wined3d_mutex_lock();
        IWineD3DCubeTexture_AddRef(This->wineD3DCubeTexture);
        wined3d_mutex_unlock();
    }

    return ref;
}

static ULONG WINAPI IDirect3DCubeTexture9Impl_Release(LPDIRECT3DCUBETEXTURE9 iface) {
    IDirect3DCubeTexture9Impl *This = (IDirect3DCubeTexture9Impl *)iface;
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("%p decreasing refcount to %u.\n", iface, ref);

    if (ref == 0) {
        IDirect3DDevice9Ex *parentDevice = This->parentDevice;

        TRACE("Releasing child %p\n", This->wineD3DCubeTexture);

        wined3d_mutex_lock();
        IWineD3DCubeTexture_Release(This->wineD3DCubeTexture);
        wined3d_mutex_unlock();

        /* Release the device last, as it may cause the device to be destroyed. */
        IDirect3DDevice9Ex_Release(parentDevice);
    }
    return ref;
}

/* IDirect3DCubeTexture9 IDirect3DResource9 Interface follow: */
static HRESULT WINAPI IDirect3DCubeTexture9Impl_GetDevice(IDirect3DCubeTexture9 *iface, IDirect3DDevice9 **device)
{
    IDirect3DCubeTexture9Impl *This = (IDirect3DCubeTexture9Impl *)iface;

    TRACE("iface %p, device %p.\n", iface, device);

    *device = (IDirect3DDevice9 *)This->parentDevice;
    IDirect3DDevice9_AddRef(*device);

    TRACE("Returning device %p.\n", *device);

    return D3D_OK;
}

static HRESULT WINAPI IDirect3DCubeTexture9Impl_SetPrivateData(LPDIRECT3DCUBETEXTURE9 iface, REFGUID refguid, CONST void* pData, DWORD SizeOfData, DWORD Flags) {
    IDirect3DCubeTexture9Impl *This = (IDirect3DCubeTexture9Impl *)iface;
    HRESULT hr;

    TRACE("iface %p, guid %s, data %p, data_size %u, flags %#x.\n",
            iface, debugstr_guid(refguid), pData, SizeOfData, Flags);

    wined3d_mutex_lock();
    hr = IWineD3DCubeTexture_SetPrivateData(This->wineD3DCubeTexture,refguid,pData,SizeOfData,Flags);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DCubeTexture9Impl_GetPrivateData(LPDIRECT3DCUBETEXTURE9 iface, REFGUID refguid, void* pData, DWORD* pSizeOfData) {
    IDirect3DCubeTexture9Impl *This = (IDirect3DCubeTexture9Impl *)iface;
    HRESULT hr;

    TRACE("iface %p, guid %s, data %p, data_size %p.\n",
            iface, debugstr_guid(refguid), pData, pSizeOfData);

    wined3d_mutex_lock();
    hr = IWineD3DCubeTexture_GetPrivateData(This->wineD3DCubeTexture,refguid,pData,pSizeOfData);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DCubeTexture9Impl_FreePrivateData(LPDIRECT3DCUBETEXTURE9 iface, REFGUID refguid) {
    IDirect3DCubeTexture9Impl *This = (IDirect3DCubeTexture9Impl *)iface;
    HRESULT hr;

    TRACE("iface %p, guid %s.\n", iface, debugstr_guid(refguid));

    wined3d_mutex_lock();
    hr = IWineD3DCubeTexture_FreePrivateData(This->wineD3DCubeTexture,refguid);
    wined3d_mutex_unlock();

    return hr;
}

static DWORD WINAPI IDirect3DCubeTexture9Impl_SetPriority(LPDIRECT3DCUBETEXTURE9 iface, DWORD PriorityNew) {
    IDirect3DCubeTexture9Impl *This = (IDirect3DCubeTexture9Impl *)iface;
    DWORD ret;

    TRACE("iface %p, priority %u.\n", iface, PriorityNew);

    wined3d_mutex_lock();
    ret = IWineD3DCubeTexture_SetPriority(This->wineD3DCubeTexture, PriorityNew);
    wined3d_mutex_unlock();

    return ret;
}

static DWORD WINAPI IDirect3DCubeTexture9Impl_GetPriority(LPDIRECT3DCUBETEXTURE9 iface) {
    IDirect3DCubeTexture9Impl *This = (IDirect3DCubeTexture9Impl *)iface;
    DWORD ret;

    TRACE("iface %p.\n", iface);

    wined3d_mutex_lock();
    ret = IWineD3DCubeTexture_GetPriority(This->wineD3DCubeTexture);
    wined3d_mutex_unlock();

    return ret;
}

static void WINAPI IDirect3DCubeTexture9Impl_PreLoad(LPDIRECT3DCUBETEXTURE9 iface) {
    IDirect3DCubeTexture9Impl *This = (IDirect3DCubeTexture9Impl *)iface;

    TRACE("iface %p.\n", iface);

    wined3d_mutex_lock();
    IWineD3DCubeTexture_PreLoad(This->wineD3DCubeTexture);
    wined3d_mutex_unlock();
}

static D3DRESOURCETYPE WINAPI IDirect3DCubeTexture9Impl_GetType(LPDIRECT3DCUBETEXTURE9 iface) {
    IDirect3DCubeTexture9Impl *This = (IDirect3DCubeTexture9Impl *)iface;
    D3DRESOURCETYPE ret;

    TRACE("iface %p.\n", iface);

    wined3d_mutex_lock();
    ret = IWineD3DCubeTexture_GetType(This->wineD3DCubeTexture);
    wined3d_mutex_unlock();

    return ret;
}

/* IDirect3DCubeTexture9 IDirect3DBaseTexture9 Interface follow: */
static DWORD WINAPI IDirect3DCubeTexture9Impl_SetLOD(LPDIRECT3DCUBETEXTURE9 iface, DWORD LODNew) {
    IDirect3DCubeTexture9Impl *This = (IDirect3DCubeTexture9Impl *)iface;
    DWORD ret;

    TRACE("iface %p, lod %u.\n", iface, LODNew);

    wined3d_mutex_lock();
    ret = IWineD3DCubeTexture_SetLOD(This->wineD3DCubeTexture, LODNew);
    wined3d_mutex_unlock();

    return ret;
}

static DWORD WINAPI IDirect3DCubeTexture9Impl_GetLOD(LPDIRECT3DCUBETEXTURE9 iface) {
    IDirect3DCubeTexture9Impl *This = (IDirect3DCubeTexture9Impl *)iface;
    DWORD ret;

    TRACE("iface %p.\n", iface);

    wined3d_mutex_lock();
    ret = IWineD3DCubeTexture_GetLOD(This->wineD3DCubeTexture);
    wined3d_mutex_unlock();

    return ret;
}

static DWORD WINAPI IDirect3DCubeTexture9Impl_GetLevelCount(LPDIRECT3DCUBETEXTURE9 iface) {
    IDirect3DCubeTexture9Impl *This = (IDirect3DCubeTexture9Impl *)iface;
    DWORD ret;

    TRACE("iface %p.\n", iface);

    wined3d_mutex_lock();
    ret = IWineD3DCubeTexture_GetLevelCount(This->wineD3DCubeTexture);
    wined3d_mutex_unlock();

    return ret;
}

static HRESULT WINAPI IDirect3DCubeTexture9Impl_SetAutoGenFilterType(LPDIRECT3DCUBETEXTURE9 iface, D3DTEXTUREFILTERTYPE FilterType) {
    IDirect3DCubeTexture9Impl *This = (IDirect3DCubeTexture9Impl *)iface;
    HRESULT hr;

    TRACE("iface %p, filter_type %#x.\n", iface, FilterType);

    wined3d_mutex_lock();
    hr = IWineD3DCubeTexture_SetAutoGenFilterType(This->wineD3DCubeTexture, (WINED3DTEXTUREFILTERTYPE) FilterType);
    wined3d_mutex_unlock();

    return hr;
}

static D3DTEXTUREFILTERTYPE WINAPI IDirect3DCubeTexture9Impl_GetAutoGenFilterType(LPDIRECT3DCUBETEXTURE9 iface) {
    IDirect3DCubeTexture9Impl *This = (IDirect3DCubeTexture9Impl *)iface;
    D3DTEXTUREFILTERTYPE ret;

    TRACE("iface %p.\n", iface);

    wined3d_mutex_lock();
    ret = (D3DTEXTUREFILTERTYPE) IWineD3DCubeTexture_GetAutoGenFilterType(This->wineD3DCubeTexture);
    wined3d_mutex_unlock();

    return ret;
}

static void WINAPI IDirect3DCubeTexture9Impl_GenerateMipSubLevels(LPDIRECT3DCUBETEXTURE9 iface) {
    IDirect3DCubeTexture9Impl *This = (IDirect3DCubeTexture9Impl *)iface;

    TRACE("iface %p.\n", iface);

    wined3d_mutex_lock();
    IWineD3DCubeTexture_GenerateMipSubLevels(This->wineD3DCubeTexture);
    wined3d_mutex_unlock();
}

/* IDirect3DCubeTexture9 Interface follow: */
static HRESULT WINAPI IDirect3DCubeTexture9Impl_GetLevelDesc(LPDIRECT3DCUBETEXTURE9 iface, UINT Level, D3DSURFACE_DESC* pDesc) {
    IDirect3DCubeTexture9Impl *This = (IDirect3DCubeTexture9Impl *)iface;
    WINED3DSURFACE_DESC wined3ddesc;
    HRESULT hr;

    TRACE("iface %p, level %u, desc %p.\n", iface, Level, pDesc);

    wined3d_mutex_lock();
    hr = IWineD3DCubeTexture_GetLevelDesc(This->wineD3DCubeTexture, Level, &wined3ddesc);
    wined3d_mutex_unlock();

    if (SUCCEEDED(hr))
    {
        pDesc->Format = d3dformat_from_wined3dformat(wined3ddesc.format);
        pDesc->Type = wined3ddesc.resource_type;
        pDesc->Usage = wined3ddesc.usage;
        pDesc->Pool = wined3ddesc.pool;
        pDesc->MultiSampleType = wined3ddesc.multisample_type;
        pDesc->MultiSampleQuality = wined3ddesc.multisample_quality;
        pDesc->Width = wined3ddesc.width;
        pDesc->Height = wined3ddesc.height;
    }

    return hr;
}

static HRESULT WINAPI IDirect3DCubeTexture9Impl_GetCubeMapSurface(IDirect3DCubeTexture9 *iface,
        D3DCUBEMAP_FACES FaceType, UINT Level, IDirect3DSurface9 **ppCubeMapSurface)
{
    IDirect3DCubeTexture9Impl *This = (IDirect3DCubeTexture9Impl *)iface;
    IWineD3DSurface *mySurface = NULL;
    HRESULT hr;

    TRACE("iface %p, face %#x, level %u, surface %p.\n", iface, FaceType, Level, ppCubeMapSurface);

    wined3d_mutex_lock();
    hr = IWineD3DCubeTexture_GetCubeMapSurface(This->wineD3DCubeTexture,
            (WINED3DCUBEMAP_FACES)FaceType, Level, &mySurface);
    if (SUCCEEDED(hr) && ppCubeMapSurface)
    {
        *ppCubeMapSurface = IWineD3DCubeTexture_GetParent(mySurface);
        IDirect3DSurface9_AddRef(*ppCubeMapSurface);
        IWineD3DCubeTexture_Release(mySurface);
    }
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DCubeTexture9Impl_LockRect(LPDIRECT3DCUBETEXTURE9 iface, D3DCUBEMAP_FACES FaceType, UINT Level, D3DLOCKED_RECT* pLockedRect, CONST RECT* pRect, DWORD Flags) {
    IDirect3DCubeTexture9Impl *This = (IDirect3DCubeTexture9Impl *)iface;
    HRESULT hr;

    TRACE("iface %p, face %#x, level %u, locked_rect %p, rect %p, flags %#x.\n",
            iface, FaceType, Level, pLockedRect, pRect, Flags);

    wined3d_mutex_lock();
    hr = IWineD3DCubeTexture_LockRect(This->wineD3DCubeTexture, (WINED3DCUBEMAP_FACES) FaceType, Level, (WINED3DLOCKED_RECT *) pLockedRect, pRect, Flags);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DCubeTexture9Impl_UnlockRect(LPDIRECT3DCUBETEXTURE9 iface, D3DCUBEMAP_FACES FaceType, UINT Level) {
    IDirect3DCubeTexture9Impl *This = (IDirect3DCubeTexture9Impl *)iface;
    HRESULT hr;

    TRACE("iface %p, face %#x, level %u.\n", iface, FaceType, Level);

    wined3d_mutex_lock();
    hr = IWineD3DCubeTexture_UnlockRect(This->wineD3DCubeTexture, (WINED3DCUBEMAP_FACES) FaceType, Level);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT  WINAPI IDirect3DCubeTexture9Impl_AddDirtyRect(LPDIRECT3DCUBETEXTURE9 iface, D3DCUBEMAP_FACES FaceType, CONST RECT* pDirtyRect) {
    IDirect3DCubeTexture9Impl *This = (IDirect3DCubeTexture9Impl *)iface;
    HRESULT hr;

    TRACE("iface %p, face %#x, dirty_rect %p.\n", iface, FaceType, pDirtyRect);

    wined3d_mutex_lock();
    hr = IWineD3DCubeTexture_AddDirtyRect(This->wineD3DCubeTexture, (WINED3DCUBEMAP_FACES) FaceType, pDirtyRect);
    wined3d_mutex_unlock();

    return hr;
}


static const IDirect3DCubeTexture9Vtbl Direct3DCubeTexture9_Vtbl =
{
    /* IUnknown */
    IDirect3DCubeTexture9Impl_QueryInterface,
    IDirect3DCubeTexture9Impl_AddRef,
    IDirect3DCubeTexture9Impl_Release,
    /* IDirect3DResource9 */
    IDirect3DCubeTexture9Impl_GetDevice,
    IDirect3DCubeTexture9Impl_SetPrivateData,
    IDirect3DCubeTexture9Impl_GetPrivateData,
    IDirect3DCubeTexture9Impl_FreePrivateData,
    IDirect3DCubeTexture9Impl_SetPriority,
    IDirect3DCubeTexture9Impl_GetPriority,
    IDirect3DCubeTexture9Impl_PreLoad,
    IDirect3DCubeTexture9Impl_GetType,
    /* IDirect3DBaseTexture9 */
    IDirect3DCubeTexture9Impl_SetLOD,
    IDirect3DCubeTexture9Impl_GetLOD,
    IDirect3DCubeTexture9Impl_GetLevelCount,
    IDirect3DCubeTexture9Impl_SetAutoGenFilterType,
    IDirect3DCubeTexture9Impl_GetAutoGenFilterType,
    IDirect3DCubeTexture9Impl_GenerateMipSubLevels,
    IDirect3DCubeTexture9Impl_GetLevelDesc,
    IDirect3DCubeTexture9Impl_GetCubeMapSurface,
    IDirect3DCubeTexture9Impl_LockRect,
    IDirect3DCubeTexture9Impl_UnlockRect,
    IDirect3DCubeTexture9Impl_AddDirtyRect
};

static void STDMETHODCALLTYPE cubetexture_wined3d_object_destroyed(void *parent)
{
    HeapFree(GetProcessHeap(), 0, parent);
}

static const struct wined3d_parent_ops d3d9_cubetexture_wined3d_parent_ops =
{
    cubetexture_wined3d_object_destroyed,
};

HRESULT cubetexture_init(IDirect3DCubeTexture9Impl *texture, IDirect3DDevice9Impl *device,
        UINT edge_length, UINT levels, DWORD usage, D3DFORMAT format, D3DPOOL pool)
{
    HRESULT hr;

    texture->lpVtbl = &Direct3DCubeTexture9_Vtbl;
    texture->ref = 1;

    wined3d_mutex_lock();
    hr = IWineD3DDevice_CreateCubeTexture(device->WineD3DDevice, edge_length,
            levels, usage, wined3dformat_from_d3dformat(format), pool, texture,
            &d3d9_cubetexture_wined3d_parent_ops, &texture->wineD3DCubeTexture);
    wined3d_mutex_unlock();
    if (FAILED(hr))
    {
        WARN("Failed to create wined3d cube texture, hr %#x.\n", hr);
        return hr;
    }

    texture->parentDevice = (IDirect3DDevice9Ex *)device;
    IDirect3DDevice9Ex_AddRef(texture->parentDevice);

    return D3D_OK;
}
