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

    TRACE("(%p) : AddRef from %d\n", This, ref - 1);

    return ref;
}

static ULONG WINAPI IDirect3DVolumeTexture9Impl_Release(LPDIRECT3DVOLUMETEXTURE9 iface) {
    IDirect3DVolumeTexture9Impl *This = (IDirect3DVolumeTexture9Impl *)iface;
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) : ReleaseRef to %d\n", This, ref);

    if (ref == 0) {
        EnterCriticalSection(&d3d9_cs);
        IWineD3DVolumeTexture_Destroy(This->wineD3DVolumeTexture, D3D9CB_DestroyVolume);
        LeaveCriticalSection(&d3d9_cs);
        IDirect3DDevice9Ex_Release(This->parentDevice);
        HeapFree(GetProcessHeap(), 0, This);
    }
    return ref;
}

/* IDirect3DVolumeTexture9 IDirect3DResource9 Interface follow: */
static HRESULT WINAPI IDirect3DVolumeTexture9Impl_GetDevice(LPDIRECT3DVOLUMETEXTURE9 iface, IDirect3DDevice9** ppDevice) {
    IDirect3DVolumeTexture9Impl *This = (IDirect3DVolumeTexture9Impl *)iface;
    IWineD3DDevice *wined3d_device;
    HRESULT hr;

    TRACE("(%p) Relay\n", This);

    EnterCriticalSection(&d3d9_cs);
    hr = IWineD3DStateBlock_GetDevice(This->wineD3DVolumeTexture, &wined3d_device);
    if (SUCCEEDED(hr))
    {
        IWineD3DDevice_GetParent(wined3d_device, (IUnknown **)ppDevice);
        IWineD3DDevice_Release(wined3d_device);
    }
    LeaveCriticalSection(&d3d9_cs);

    return hr;
}

static HRESULT WINAPI IDirect3DVolumeTexture9Impl_SetPrivateData(LPDIRECT3DVOLUMETEXTURE9 iface, REFGUID refguid, CONST void* pData, DWORD SizeOfData, DWORD Flags) {
    IDirect3DVolumeTexture9Impl *This = (IDirect3DVolumeTexture9Impl *)iface;
    HRESULT hr;

    TRACE("(%p) Relay\n", This);

    EnterCriticalSection(&d3d9_cs);

    hr = IWineD3DVolumeTexture_SetPrivateData(This->wineD3DVolumeTexture, refguid, pData, SizeOfData, Flags);

    LeaveCriticalSection(&d3d9_cs);

    return hr;
}

static HRESULT WINAPI IDirect3DVolumeTexture9Impl_GetPrivateData(LPDIRECT3DVOLUMETEXTURE9 iface, REFGUID refguid, void* pData, DWORD* pSizeOfData) {
    IDirect3DVolumeTexture9Impl *This = (IDirect3DVolumeTexture9Impl *)iface;
    HRESULT hr;

    TRACE("(%p) Relay\n", This);

    EnterCriticalSection(&d3d9_cs);

    hr = IWineD3DVolumeTexture_GetPrivateData(This->wineD3DVolumeTexture, refguid, pData, pSizeOfData);

    LeaveCriticalSection(&d3d9_cs);

    return hr;
}

static HRESULT WINAPI IDirect3DVolumeTexture9Impl_FreePrivateData(LPDIRECT3DVOLUMETEXTURE9 iface, REFGUID refguid) {
    IDirect3DVolumeTexture9Impl *This = (IDirect3DVolumeTexture9Impl *)iface;
    HRESULT hr;

    TRACE("(%p) Relay\n", This);

    EnterCriticalSection(&d3d9_cs);

    hr = IWineD3DVolumeTexture_FreePrivateData(This->wineD3DVolumeTexture, refguid);

    LeaveCriticalSection(&d3d9_cs);

    return hr;
}

static DWORD WINAPI IDirect3DVolumeTexture9Impl_SetPriority(LPDIRECT3DVOLUMETEXTURE9 iface, DWORD PriorityNew) {
    IDirect3DVolumeTexture9Impl *This = (IDirect3DVolumeTexture9Impl *)iface;
    DWORD priority;

    TRACE("(%p) Relay\n", This);

    EnterCriticalSection(&d3d9_cs);

    priority = IWineD3DVolumeTexture_SetPriority(This->wineD3DVolumeTexture, PriorityNew);

    LeaveCriticalSection(&d3d9_cs);

    return priority;
}

static DWORD WINAPI IDirect3DVolumeTexture9Impl_GetPriority(LPDIRECT3DVOLUMETEXTURE9 iface) {
    IDirect3DVolumeTexture9Impl *This = (IDirect3DVolumeTexture9Impl *)iface;
    DWORD priority;

    TRACE("(%p) Relay\n", This);

    EnterCriticalSection(&d3d9_cs);

    priority = IWineD3DVolumeTexture_GetPriority(This->wineD3DVolumeTexture);

    LeaveCriticalSection(&d3d9_cs);

    return priority;
}

static void WINAPI IDirect3DVolumeTexture9Impl_PreLoad(LPDIRECT3DVOLUMETEXTURE9 iface) {
    IDirect3DVolumeTexture9Impl *This = (IDirect3DVolumeTexture9Impl *)iface;

    TRACE("(%p) Relay\n", This);

    EnterCriticalSection(&d3d9_cs);

    IWineD3DVolumeTexture_PreLoad(This->wineD3DVolumeTexture);

    LeaveCriticalSection(&d3d9_cs);
}

static D3DRESOURCETYPE WINAPI IDirect3DVolumeTexture9Impl_GetType(LPDIRECT3DVOLUMETEXTURE9 iface) {
    IDirect3DVolumeTexture9Impl *This = (IDirect3DVolumeTexture9Impl *)iface;
    D3DRESOURCETYPE type;

    TRACE("(%p) Relay\n", This);

    EnterCriticalSection(&d3d9_cs);

    type = IWineD3DVolumeTexture_GetType(This->wineD3DVolumeTexture);

    LeaveCriticalSection(&d3d9_cs);

    return type;
}

/* IDirect3DVolumeTexture9 IDirect3DBaseTexture9 Interface follow: */
static DWORD WINAPI IDirect3DVolumeTexture9Impl_SetLOD(LPDIRECT3DVOLUMETEXTURE9 iface, DWORD LODNew) {
    IDirect3DVolumeTexture9Impl *This = (IDirect3DVolumeTexture9Impl *)iface;
    DWORD lod;

    TRACE("(%p) Relay\n", This);

    EnterCriticalSection(&d3d9_cs);

    lod = IWineD3DVolumeTexture_SetLOD(This->wineD3DVolumeTexture, LODNew);

    LeaveCriticalSection(&d3d9_cs);

    return lod;
}

static DWORD WINAPI IDirect3DVolumeTexture9Impl_GetLOD(LPDIRECT3DVOLUMETEXTURE9 iface) {
    IDirect3DVolumeTexture9Impl *This = (IDirect3DVolumeTexture9Impl *)iface;
    DWORD lod;

    TRACE("(%p) Relay\n", This);

    EnterCriticalSection(&d3d9_cs);

    lod = IWineD3DVolumeTexture_GetLOD(This->wineD3DVolumeTexture);

    LeaveCriticalSection(&d3d9_cs);

    return lod;
}

static DWORD WINAPI IDirect3DVolumeTexture9Impl_GetLevelCount(LPDIRECT3DVOLUMETEXTURE9 iface) {
    IDirect3DVolumeTexture9Impl *This = (IDirect3DVolumeTexture9Impl *)iface;
    DWORD level_count;

    TRACE("(%p) Relay\n", This);

    EnterCriticalSection(&d3d9_cs);

    level_count = IWineD3DVolumeTexture_GetLevelCount(This->wineD3DVolumeTexture);

    LeaveCriticalSection(&d3d9_cs);

    return level_count;
}

static HRESULT WINAPI IDirect3DVolumeTexture9Impl_SetAutoGenFilterType(LPDIRECT3DVOLUMETEXTURE9 iface, D3DTEXTUREFILTERTYPE FilterType) {
    IDirect3DVolumeTexture9Impl *This = (IDirect3DVolumeTexture9Impl *)iface;
    HRESULT hr;

    TRACE("(%p) Relay\n", This);

    EnterCriticalSection(&d3d9_cs);

    hr = IWineD3DVolumeTexture_SetAutoGenFilterType(This->wineD3DVolumeTexture, (WINED3DTEXTUREFILTERTYPE) FilterType);

    LeaveCriticalSection(&d3d9_cs);

    return hr;
}

static D3DTEXTUREFILTERTYPE WINAPI IDirect3DVolumeTexture9Impl_GetAutoGenFilterType(LPDIRECT3DVOLUMETEXTURE9 iface) {
    IDirect3DVolumeTexture9Impl *This = (IDirect3DVolumeTexture9Impl *)iface;
    D3DTEXTUREFILTERTYPE filter_type;

    TRACE("(%p) Relay\n", This);

    EnterCriticalSection(&d3d9_cs);

    filter_type = (D3DTEXTUREFILTERTYPE)IWineD3DVolumeTexture_GetAutoGenFilterType(This->wineD3DVolumeTexture);

    LeaveCriticalSection(&d3d9_cs);

    return filter_type;
}

static void WINAPI IDirect3DVolumeTexture9Impl_GenerateMipSubLevels(LPDIRECT3DVOLUMETEXTURE9 iface) {
    IDirect3DVolumeTexture9Impl *This = (IDirect3DVolumeTexture9Impl *)iface;

    TRACE("(%p) Relay\n", This);

    EnterCriticalSection(&d3d9_cs);

    IWineD3DVolumeTexture_GenerateMipSubLevels(This->wineD3DVolumeTexture);

    LeaveCriticalSection(&d3d9_cs);
}

/* IDirect3DVolumeTexture9 Interface follow: */
static HRESULT WINAPI IDirect3DVolumeTexture9Impl_GetLevelDesc(LPDIRECT3DVOLUMETEXTURE9 iface, UINT Level, D3DVOLUME_DESC* pDesc) {
    IDirect3DVolumeTexture9Impl *This = (IDirect3DVolumeTexture9Impl *)iface;
    WINED3DVOLUME_DESC     wined3ddesc;
    UINT                   tmpInt = -1;
    WINED3DFORMAT format;
    HRESULT hr;

    TRACE("(%p) Relay\n", This);

    /* As d3d8 and d3d9 structures differ, pass in ptrs to where data needs to go */
    wined3ddesc.Format              = &format;
    wined3ddesc.Type                = (WINED3DRESOURCETYPE *)&pDesc->Type;
    wined3ddesc.Usage               = &pDesc->Usage;
    wined3ddesc.Pool                = (WINED3DPOOL *) &pDesc->Pool;
    wined3ddesc.Size                = &tmpInt;
    wined3ddesc.Width               = &pDesc->Width;
    wined3ddesc.Height              = &pDesc->Height;
    wined3ddesc.Depth               = &pDesc->Depth;

    EnterCriticalSection(&d3d9_cs);

    hr = IWineD3DVolumeTexture_GetLevelDesc(This->wineD3DVolumeTexture, Level, &wined3ddesc);

    LeaveCriticalSection(&d3d9_cs);

    if (SUCCEEDED(hr)) pDesc->Format = d3dformat_from_wined3dformat(format);

    return hr;
}

static HRESULT WINAPI IDirect3DVolumeTexture9Impl_GetVolumeLevel(LPDIRECT3DVOLUMETEXTURE9 iface, UINT Level, IDirect3DVolume9** ppVolumeLevel) {
    IDirect3DVolumeTexture9Impl *This = (IDirect3DVolumeTexture9Impl *)iface;
    HRESULT hrc = D3D_OK;
    IWineD3DVolume *myVolume = NULL;

    TRACE("(%p) Relay\n", This);

    EnterCriticalSection(&d3d9_cs);

    hrc = IWineD3DVolumeTexture_GetVolumeLevel(This->wineD3DVolumeTexture, Level, &myVolume);
    if (hrc == D3D_OK && NULL != ppVolumeLevel) {
       IWineD3DVolumeTexture_GetParent(myVolume, (IUnknown **)ppVolumeLevel);
       IWineD3DVolumeTexture_Release(myVolume);
    }

    LeaveCriticalSection(&d3d9_cs);

    return hrc;
}

static HRESULT WINAPI IDirect3DVolumeTexture9Impl_LockBox(LPDIRECT3DVOLUMETEXTURE9 iface, UINT Level, D3DLOCKED_BOX* pLockedVolume, CONST D3DBOX* pBox, DWORD Flags) {
    IDirect3DVolumeTexture9Impl *This = (IDirect3DVolumeTexture9Impl *)iface;
    HRESULT hr;

    TRACE("(%p) Relay %p %p %p %d\n", This, This->wineD3DVolumeTexture, pLockedVolume, pBox,Flags);

    EnterCriticalSection(&d3d9_cs);

    hr = IWineD3DVolumeTexture_LockBox(This->wineD3DVolumeTexture, Level, (WINED3DLOCKED_BOX *)pLockedVolume,
            (const WINED3DBOX *)pBox, Flags);

    LeaveCriticalSection(&d3d9_cs);

    return hr;
}

static HRESULT WINAPI IDirect3DVolumeTexture9Impl_UnlockBox(LPDIRECT3DVOLUMETEXTURE9 iface, UINT Level) {
    IDirect3DVolumeTexture9Impl *This = (IDirect3DVolumeTexture9Impl *)iface;
    HRESULT hr;

    TRACE("(%p) Relay %p %d\n", This, This->wineD3DVolumeTexture, Level);

    EnterCriticalSection(&d3d9_cs);

    hr = IWineD3DVolumeTexture_UnlockBox(This->wineD3DVolumeTexture, Level);

    LeaveCriticalSection(&d3d9_cs);

    return hr;
}

static HRESULT WINAPI IDirect3DVolumeTexture9Impl_AddDirtyBox(LPDIRECT3DVOLUMETEXTURE9 iface, CONST D3DBOX* pDirtyBox) {
    IDirect3DVolumeTexture9Impl *This = (IDirect3DVolumeTexture9Impl *)iface;
    HRESULT hr;

    TRACE("(%p) Relay\n", This);

    EnterCriticalSection(&d3d9_cs);

    hr = IWineD3DVolumeTexture_AddDirtyBox(This->wineD3DVolumeTexture, (CONST WINED3DBOX *)pDirtyBox);

    LeaveCriticalSection(&d3d9_cs);

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


/* IDirect3DDevice9 IDirect3DVolumeTexture9 Methods follow: */
HRESULT  WINAPI  IDirect3DDevice9Impl_CreateVolumeTexture(LPDIRECT3DDEVICE9EX iface,
                                                          UINT Width, UINT Height, UINT Depth, UINT Levels, DWORD Usage, 
                                                          D3DFORMAT Format, D3DPOOL Pool, 
                                                          IDirect3DVolumeTexture9** ppVolumeTexture, HANDLE* pSharedHandle) {

    IDirect3DVolumeTexture9Impl *object;
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    HRESULT hrc = D3D_OK;

    TRACE("(%p) Relay\n", This);

    /* Allocate the storage for the device */
    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirect3DVolumeTexture9Impl));
    if (NULL == object) {
        ERR("(%p) allocation of memory failed, returning D3DERR_OUTOFVIDEOMEMORY\n", This);
        return D3DERR_OUTOFVIDEOMEMORY;
    }

    object->lpVtbl = &Direct3DVolumeTexture9_Vtbl;
    object->ref = 1;

    EnterCriticalSection(&d3d9_cs);

    hrc = IWineD3DDevice_CreateVolumeTexture(This->WineD3DDevice, Width, Height, Depth, Levels,
            Usage & WINED3DUSAGE_MASK, wined3dformat_from_d3dformat(Format), Pool,
            &object->wineD3DVolumeTexture, pSharedHandle, (IUnknown *)object);

    LeaveCriticalSection(&d3d9_cs);

    if (hrc != D3D_OK) {

        /* free up object */
        WARN("(%p) call to IWineD3DDevice_CreateVolumeTexture failed\n", This);
        HeapFree(GetProcessHeap(), 0, object);
    } else {
        IDirect3DDevice9Ex_AddRef(iface);
        object->parentDevice = iface;
        *ppVolumeTexture = (LPDIRECT3DVOLUMETEXTURE9) object;
        TRACE("(%p) : Created volume texture %p\n", This, object);
    }
    TRACE("(%p)  returning %p\n", This , *ppVolumeTexture);
    return hrc;
}
