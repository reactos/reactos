/*
 * IDirect3DVolumeTexture8 implementation
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

/* IDirect3DVolumeTexture8 IUnknown parts follow: */
static HRESULT WINAPI IDirect3DVolumeTexture8Impl_QueryInterface(LPDIRECT3DVOLUMETEXTURE8 iface, REFIID riid, LPVOID *ppobj) {
    IDirect3DVolumeTexture8Impl *This = (IDirect3DVolumeTexture8Impl *)iface;

    if (IsEqualGUID(riid, &IID_IUnknown)
    || IsEqualGUID(riid, &IID_IDirect3DResource8)
    || IsEqualGUID(riid, &IID_IDirect3DBaseTexture8)
    || IsEqualGUID(riid, &IID_IDirect3DVolumeTexture8)) {
        IUnknown_AddRef(iface);
        *ppobj = This;
        return S_OK;
    }

    WARN("(%p)->(%s,%p),not found\n", This, debugstr_guid(riid), ppobj);
    *ppobj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI IDirect3DVolumeTexture8Impl_AddRef(LPDIRECT3DVOLUMETEXTURE8 iface) {
    IDirect3DVolumeTexture8Impl *This = (IDirect3DVolumeTexture8Impl *)iface;
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) : AddRef from %d\n", This, ref - 1);

    return ref;
}

static ULONG WINAPI IDirect3DVolumeTexture8Impl_Release(LPDIRECT3DVOLUMETEXTURE8 iface) {
    IDirect3DVolumeTexture8Impl *This = (IDirect3DVolumeTexture8Impl *)iface;
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) : ReleaseRef to %d\n", This, ref);

    if (ref == 0) {
        IWineD3DVolumeTexture_Destroy(This->wineD3DVolumeTexture, D3D8CB_DestroyVolume);
        IUnknown_Release(This->parentDevice);
        HeapFree(GetProcessHeap(), 0, This);
    }
    return ref;
}

/* IDirect3DVolumeTexture8 IDirect3DResource8 Interface follow: */
static HRESULT WINAPI IDirect3DVolumeTexture8Impl_GetDevice(LPDIRECT3DVOLUMETEXTURE8 iface, IDirect3DDevice8 **ppDevice) {
    IDirect3DVolumeTexture8Impl *This = (IDirect3DVolumeTexture8Impl *)iface;
    TRACE("(%p) Relay\n", This);
    return IDirect3DResource8Impl_GetDevice((LPDIRECT3DRESOURCE8) This, ppDevice);
}

static HRESULT WINAPI IDirect3DVolumeTexture8Impl_SetPrivateData(LPDIRECT3DVOLUMETEXTURE8 iface, REFGUID refguid, CONST void* pData, DWORD SizeOfData, DWORD Flags) {
    IDirect3DVolumeTexture8Impl *This = (IDirect3DVolumeTexture8Impl *)iface;
    TRACE("(%p) Relay\n", This);
    return IWineD3DVolumeTexture_SetPrivateData(This->wineD3DVolumeTexture, refguid, pData, SizeOfData, Flags);
}

static HRESULT WINAPI IDirect3DVolumeTexture8Impl_GetPrivateData(LPDIRECT3DVOLUMETEXTURE8 iface, REFGUID refguid, void *pData, DWORD *pSizeOfData) {
    IDirect3DVolumeTexture8Impl *This = (IDirect3DVolumeTexture8Impl *)iface;
    TRACE("(%p) Relay\n", This);
    return IWineD3DVolumeTexture_GetPrivateData(This->wineD3DVolumeTexture, refguid, pData, pSizeOfData);
}

static HRESULT WINAPI IDirect3DVolumeTexture8Impl_FreePrivateData(LPDIRECT3DVOLUMETEXTURE8 iface, REFGUID refguid) {
    IDirect3DVolumeTexture8Impl *This = (IDirect3DVolumeTexture8Impl *)iface;
    TRACE("(%p) Relay\n", This);
    return IWineD3DVolumeTexture_FreePrivateData(This->wineD3DVolumeTexture, refguid);
}

static DWORD WINAPI IDirect3DVolumeTexture8Impl_SetPriority(LPDIRECT3DVOLUMETEXTURE8 iface, DWORD PriorityNew) {
    IDirect3DVolumeTexture8Impl *This = (IDirect3DVolumeTexture8Impl *)iface;
    TRACE("(%p) Relay\n", This);
    return IWineD3DVolumeTexture_SetPriority(This->wineD3DVolumeTexture, PriorityNew);
}

static DWORD WINAPI IDirect3DVolumeTexture8Impl_GetPriority(LPDIRECT3DVOLUMETEXTURE8 iface) {
    IDirect3DVolumeTexture8Impl *This = (IDirect3DVolumeTexture8Impl *)iface;
    TRACE("(%p) Relay\n", This);
    return IWineD3DVolumeTexture_GetPriority(This->wineD3DVolumeTexture);
}

static void WINAPI IDirect3DVolumeTexture8Impl_PreLoad(LPDIRECT3DVOLUMETEXTURE8 iface) {
    IDirect3DVolumeTexture8Impl *This = (IDirect3DVolumeTexture8Impl *)iface;
    TRACE("(%p) Relay\n", This);
    IWineD3DVolumeTexture_PreLoad(This->wineD3DVolumeTexture);
}

static D3DRESOURCETYPE WINAPI IDirect3DVolumeTexture8Impl_GetType(LPDIRECT3DVOLUMETEXTURE8 iface) {
    IDirect3DVolumeTexture8Impl *This = (IDirect3DVolumeTexture8Impl *)iface;
    TRACE("(%p) Relay\n", This);
    return IWineD3DVolumeTexture_GetType(This->wineD3DVolumeTexture);
}

/* IDirect3DVolumeTexture8 IDirect3DBaseTexture8 Interface follow: */
static DWORD WINAPI IDirect3DVolumeTexture8Impl_SetLOD(LPDIRECT3DVOLUMETEXTURE8 iface, DWORD LODNew) {
    IDirect3DVolumeTexture8Impl *This = (IDirect3DVolumeTexture8Impl *)iface;
    TRACE("(%p) Relay\n", This);
    return IWineD3DVolumeTexture_SetLOD(This->wineD3DVolumeTexture, LODNew);
}

static DWORD WINAPI IDirect3DVolumeTexture8Impl_GetLOD(LPDIRECT3DVOLUMETEXTURE8 iface) {
    IDirect3DVolumeTexture8Impl *This = (IDirect3DVolumeTexture8Impl *)iface;
    TRACE("(%p) Relay\n", This);
    return IWineD3DVolumeTexture_GetLOD(This->wineD3DVolumeTexture);
}

static DWORD WINAPI IDirect3DVolumeTexture8Impl_GetLevelCount(LPDIRECT3DVOLUMETEXTURE8 iface) {
    IDirect3DVolumeTexture8Impl *This = (IDirect3DVolumeTexture8Impl *)iface;
    TRACE("(%p) Relay\n", This);
    return IWineD3DVolumeTexture_GetLevelCount(This->wineD3DVolumeTexture);
}

/* IDirect3DVolumeTexture8 Interface follow: */
static HRESULT WINAPI IDirect3DVolumeTexture8Impl_GetLevelDesc(LPDIRECT3DVOLUMETEXTURE8 iface, UINT Level, D3DVOLUME_DESC* pDesc) {
    IDirect3DVolumeTexture8Impl *This = (IDirect3DVolumeTexture8Impl *)iface;
    WINED3DVOLUME_DESC     wined3ddesc;
    UINT                   tmpInt = -1;

    TRACE("(%p) Relay\n", This);

    /* As d3d8 and d3d8 structures differ, pass in ptrs to where data needs to go */
    wined3ddesc.Format              = (WINED3DFORMAT *)&pDesc->Format;
    wined3ddesc.Type                = (WINED3DRESOURCETYPE *)&pDesc->Type;
    wined3ddesc.Usage               = &pDesc->Usage;
    wined3ddesc.Pool                = (WINED3DPOOL *) &pDesc->Pool;
    wined3ddesc.Size                = &tmpInt;
    wined3ddesc.Width               = &pDesc->Width;
    wined3ddesc.Height              = &pDesc->Height;
    wined3ddesc.Depth               = &pDesc->Depth;

    return IWineD3DVolumeTexture_GetLevelDesc(This->wineD3DVolumeTexture, Level, &wined3ddesc);
}

static HRESULT WINAPI IDirect3DVolumeTexture8Impl_GetVolumeLevel(LPDIRECT3DVOLUMETEXTURE8 iface, UINT Level, IDirect3DVolume8 **ppVolumeLevel) {
    IDirect3DVolumeTexture8Impl *This = (IDirect3DVolumeTexture8Impl *)iface;
    HRESULT hrc = D3D_OK;
    IWineD3DVolume *myVolume = NULL;

    TRACE("(%p) Relay\n", This);

    hrc = IWineD3DVolumeTexture_GetVolumeLevel(This->wineD3DVolumeTexture, Level, &myVolume);
    if (hrc == D3D_OK && NULL != ppVolumeLevel) {
       IWineD3DVolumeTexture_GetParent(myVolume, (IUnknown **)ppVolumeLevel);
       IWineD3DVolumeTexture_Release(myVolume);
    }
    return hrc;
}

static HRESULT WINAPI IDirect3DVolumeTexture8Impl_LockBox(LPDIRECT3DVOLUMETEXTURE8 iface, UINT Level, D3DLOCKED_BOX *pLockedVolume, CONST D3DBOX *pBox, DWORD Flags) {
    IDirect3DVolumeTexture8Impl *This = (IDirect3DVolumeTexture8Impl *)iface;
    TRACE("(%p) Relay %p %p %p %d\n", This, This->wineD3DVolumeTexture, pLockedVolume, pBox,Flags);
    return IWineD3DVolumeTexture_LockBox(This->wineD3DVolumeTexture, Level, (WINED3DLOCKED_BOX *) pLockedVolume, (CONST WINED3DBOX *) pBox, Flags);
}

static HRESULT WINAPI IDirect3DVolumeTexture8Impl_UnlockBox(LPDIRECT3DVOLUMETEXTURE8 iface, UINT Level) {
    IDirect3DVolumeTexture8Impl *This = (IDirect3DVolumeTexture8Impl *)iface;
    TRACE("(%p) Relay %p %d\n", This, This->wineD3DVolumeTexture, Level);
    return IWineD3DVolumeTexture_UnlockBox(This->wineD3DVolumeTexture, Level);
}

static HRESULT WINAPI IDirect3DVolumeTexture8Impl_AddDirtyBox(LPDIRECT3DVOLUMETEXTURE8 iface, CONST D3DBOX *pDirtyBox) {
    IDirect3DVolumeTexture8Impl *This = (IDirect3DVolumeTexture8Impl *)iface;
    TRACE("(%p) Relay\n", This);
    return IWineD3DVolumeTexture_AddDirtyBox(This->wineD3DVolumeTexture, (CONST WINED3DBOX *) pDirtyBox);
}


const IDirect3DVolumeTexture8Vtbl Direct3DVolumeTexture8_Vtbl =
{
    /* IUnknown */
    IDirect3DVolumeTexture8Impl_QueryInterface,
    IDirect3DVolumeTexture8Impl_AddRef,
    IDirect3DVolumeTexture8Impl_Release,
    /* IDirect3DResource8 */
    IDirect3DVolumeTexture8Impl_GetDevice,
    IDirect3DVolumeTexture8Impl_SetPrivateData,
    IDirect3DVolumeTexture8Impl_GetPrivateData,
    IDirect3DVolumeTexture8Impl_FreePrivateData,
    IDirect3DVolumeTexture8Impl_SetPriority,
    IDirect3DVolumeTexture8Impl_GetPriority,
    IDirect3DVolumeTexture8Impl_PreLoad,
    IDirect3DVolumeTexture8Impl_GetType,
    /* IDirect3DBaseTexture8 */
    IDirect3DVolumeTexture8Impl_SetLOD,
    IDirect3DVolumeTexture8Impl_GetLOD,
    IDirect3DVolumeTexture8Impl_GetLevelCount,
    /* IDirect3DVolumeTexture8 */
    IDirect3DVolumeTexture8Impl_GetLevelDesc,
    IDirect3DVolumeTexture8Impl_GetVolumeLevel,
    IDirect3DVolumeTexture8Impl_LockBox,
    IDirect3DVolumeTexture8Impl_UnlockBox,
    IDirect3DVolumeTexture8Impl_AddDirtyBox
};
