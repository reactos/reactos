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

    TRACE("(%p) : AddRef from %d\n", This, ref - 1);

    return ref;
}

static ULONG WINAPI IDirect3DTexture8Impl_Release(LPDIRECT3DTEXTURE8 iface) {
    IDirect3DTexture8Impl *This = (IDirect3DTexture8Impl *)iface;
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) : ReleaseRef to %d\n", This, ref);

    if (ref == 0) {
        IWineD3DTexture_Destroy(This->wineD3DTexture, D3D8CB_DestroySurface);
        IUnknown_Release(This->parentDevice);
        HeapFree(GetProcessHeap(), 0, This);
    }
    return ref;
}

/* IDirect3DTexture8 IDirect3DResource8 Interface follow: */
static HRESULT WINAPI IDirect3DTexture8Impl_GetDevice(LPDIRECT3DTEXTURE8 iface, IDirect3DDevice8 **ppDevice) {
    IDirect3DTexture8Impl *This = (IDirect3DTexture8Impl *)iface;
    TRACE("(%p) Relay\n", This);
    return IDirect3DResource8Impl_GetDevice((LPDIRECT3DRESOURCE8) This, ppDevice);
}

static HRESULT WINAPI IDirect3DTexture8Impl_SetPrivateData(LPDIRECT3DTEXTURE8 iface, REFGUID refguid, CONST void *pData, DWORD SizeOfData, DWORD Flags) {
    IDirect3DTexture8Impl *This = (IDirect3DTexture8Impl *)iface;
    TRACE("(%p) Relay\n", This);
    return IWineD3DTexture_SetPrivateData(This->wineD3DTexture, refguid, pData, SizeOfData, Flags);
}

static HRESULT WINAPI IDirect3DTexture8Impl_GetPrivateData(LPDIRECT3DTEXTURE8 iface, REFGUID refguid, void *pData, DWORD* pSizeOfData) {
    IDirect3DTexture8Impl *This = (IDirect3DTexture8Impl *)iface;
    TRACE("(%p) Relay\n", This);
    return IWineD3DTexture_GetPrivateData(This->wineD3DTexture, refguid, pData, pSizeOfData);
}

static HRESULT WINAPI IDirect3DTexture8Impl_FreePrivateData(LPDIRECT3DTEXTURE8 iface, REFGUID refguid) {
    IDirect3DTexture8Impl *This = (IDirect3DTexture8Impl *)iface;
    TRACE("(%p) Relay\n", This);
    return IWineD3DTexture_FreePrivateData(This->wineD3DTexture, refguid);
}

static DWORD WINAPI IDirect3DTexture8Impl_SetPriority(LPDIRECT3DTEXTURE8 iface, DWORD PriorityNew) {
    IDirect3DTexture8Impl *This = (IDirect3DTexture8Impl *)iface;
    TRACE("(%p) Relay\n", This);
    return IWineD3DTexture_SetPriority(This->wineD3DTexture, PriorityNew);
}

static DWORD WINAPI IDirect3DTexture8Impl_GetPriority(LPDIRECT3DTEXTURE8 iface) {
    IDirect3DTexture8Impl *This = (IDirect3DTexture8Impl *)iface;
    TRACE("(%p) Relay\n", This);
    return IWineD3DTexture_GetPriority(This->wineD3DTexture);
}

static void WINAPI IDirect3DTexture8Impl_PreLoad(LPDIRECT3DTEXTURE8 iface) {
    IDirect3DTexture8Impl *This = (IDirect3DTexture8Impl *)iface;
    TRACE("(%p) Relay\n", This);
    IWineD3DTexture_PreLoad(This->wineD3DTexture);
}

static D3DRESOURCETYPE WINAPI IDirect3DTexture8Impl_GetType(LPDIRECT3DTEXTURE8 iface) {
    IDirect3DTexture8Impl *This = (IDirect3DTexture8Impl *)iface;
    TRACE("(%p) Relay\n", This);
    return IWineD3DTexture_GetType(This->wineD3DTexture);
}

/* IDirect3DTexture8 IDirect3DBaseTexture8 Interface follow: */
static DWORD WINAPI IDirect3DTexture8Impl_SetLOD(LPDIRECT3DTEXTURE8 iface, DWORD LODNew) {
    IDirect3DTexture8Impl *This = (IDirect3DTexture8Impl *)iface;
    TRACE("(%p) Relay\n", This);
    return IWineD3DTexture_SetLOD(This->wineD3DTexture, LODNew);
}

static DWORD WINAPI IDirect3DTexture8Impl_GetLOD(LPDIRECT3DTEXTURE8 iface) {
    IDirect3DTexture8Impl *This = (IDirect3DTexture8Impl *)iface;
    TRACE("(%p) Relay\n", This);
    return IWineD3DTexture_GetLOD(This->wineD3DTexture);
}

static DWORD WINAPI IDirect3DTexture8Impl_GetLevelCount(LPDIRECT3DTEXTURE8 iface) {
    IDirect3DTexture8Impl *This = (IDirect3DTexture8Impl *)iface;
    TRACE("(%p) Relay\n", This);
    return IWineD3DTexture_GetLevelCount(This->wineD3DTexture);
}

/* IDirect3DTexture8 Interface follow: */
static HRESULT WINAPI IDirect3DTexture8Impl_GetLevelDesc(LPDIRECT3DTEXTURE8 iface, UINT Level, D3DSURFACE_DESC *pDesc) {
    IDirect3DTexture8Impl *This = (IDirect3DTexture8Impl *)iface;

    WINED3DSURFACE_DESC    wined3ddesc;
    TRACE("(%p) Relay\n", This);

    /* As d3d8 and d3d9 structures differ, pass in ptrs to where data needs to go */
    memset(&wined3ddesc, 0, sizeof(wined3ddesc));
    wined3ddesc.Format              = (WINED3DFORMAT *)&pDesc->Format;
    wined3ddesc.Type                = (WINED3DRESOURCETYPE *)&pDesc->Type;
    wined3ddesc.Usage               = &pDesc->Usage;
    wined3ddesc.Pool                = (WINED3DPOOL *) &pDesc->Pool;
    wined3ddesc.Size                = &pDesc->Size;
    wined3ddesc.MultiSampleType     = (WINED3DMULTISAMPLE_TYPE *) &pDesc->MultiSampleType;
    wined3ddesc.Width               = &pDesc->Width;
    wined3ddesc.Height              = &pDesc->Height;

    return IWineD3DTexture_GetLevelDesc(This->wineD3DTexture, Level, &wined3ddesc);
}

static HRESULT WINAPI IDirect3DTexture8Impl_GetSurfaceLevel(LPDIRECT3DTEXTURE8 iface, UINT Level, IDirect3DSurface8 **ppSurfaceLevel) {
    IDirect3DTexture8Impl *This = (IDirect3DTexture8Impl *)iface;
    HRESULT hrc = D3D_OK;
    IWineD3DSurface *mySurface = NULL;

    TRACE("(%p) Relay\n", This);
    hrc = IWineD3DTexture_GetSurfaceLevel(This->wineD3DTexture, Level, &mySurface);
    if (hrc == D3D_OK && NULL != ppSurfaceLevel) {
       IWineD3DSurface_GetParent(mySurface, (IUnknown **)ppSurfaceLevel);
       IWineD3DSurface_Release(mySurface);
    }
    return hrc;
}

static HRESULT WINAPI IDirect3DTexture8Impl_LockRect(LPDIRECT3DTEXTURE8 iface, UINT Level, D3DLOCKED_RECT *pLockedRect, CONST RECT *pRect, DWORD Flags) {
    IDirect3DTexture8Impl *This = (IDirect3DTexture8Impl *)iface;
    TRACE("(%p) Relay\n", This);
    return IWineD3DTexture_LockRect(This->wineD3DTexture, Level, (WINED3DLOCKED_RECT *) pLockedRect, pRect, Flags);
}

static HRESULT WINAPI IDirect3DTexture8Impl_UnlockRect(LPDIRECT3DTEXTURE8 iface, UINT Level) {
    IDirect3DTexture8Impl *This = (IDirect3DTexture8Impl *)iface;
    TRACE("(%p) Relay\n", This);
    return IWineD3DTexture_UnlockRect(This->wineD3DTexture, Level);
}

static HRESULT WINAPI IDirect3DTexture8Impl_AddDirtyRect(LPDIRECT3DTEXTURE8 iface, CONST RECT *pDirtyRect) {
    IDirect3DTexture8Impl *This = (IDirect3DTexture8Impl *)iface;
    TRACE("(%p) Relay\n", This);
    return IWineD3DTexture_AddDirtyRect(This->wineD3DTexture, pDirtyRect);
}

const IDirect3DTexture8Vtbl Direct3DTexture8_Vtbl =
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
