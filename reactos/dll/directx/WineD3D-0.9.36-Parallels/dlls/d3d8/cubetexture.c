/*
 * IDirect3DCubeTexture8 implementation
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

/* IDirect3DCubeTexture8 IUnknown parts follow: */
static HRESULT WINAPI IDirect3DCubeTexture8Impl_QueryInterface(LPDIRECT3DCUBETEXTURE8 iface, REFIID riid, LPVOID *ppobj) {
    IDirect3DCubeTexture8Impl *This = (IDirect3DCubeTexture8Impl *)iface;
    if (IsEqualGUID(riid, &IID_IUnknown)
        || IsEqualGUID(riid, &IID_IDirect3DResource8)
        || IsEqualGUID(riid, &IID_IDirect3DBaseTexture8)
        || IsEqualGUID(riid, &IID_IDirect3DCubeTexture8)) {
        IUnknown_AddRef(iface);
        *ppobj = This;
        return S_OK;
    }

    WARN("(%p)->(%s,%p),not found\n", This, debugstr_guid(riid), ppobj);
    *ppobj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI IDirect3DCubeTexture8Impl_AddRef(LPDIRECT3DCUBETEXTURE8 iface) {
    IDirect3DCubeTexture8Impl *This = (IDirect3DCubeTexture8Impl *)iface;
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) : AddRef from %d\n", This, ref - 1);

    return ref;
}

static ULONG WINAPI IDirect3DCubeTexture8Impl_Release(LPDIRECT3DCUBETEXTURE8 iface) {
    IDirect3DCubeTexture8Impl *This = (IDirect3DCubeTexture8Impl *)iface;
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) : ReleaseRef to %d\n", This, ref);

    if (ref == 0) {
        TRACE("Releasing child %p\n", This->wineD3DCubeTexture);
        IWineD3DCubeTexture_Destroy(This->wineD3DCubeTexture, D3D8CB_DestroySurface);
        IUnknown_Release(This->parentDevice);
        HeapFree(GetProcessHeap(), 0, This);
    }
    return ref;
}

/* IDirect3DCubeTexture8 IDirect3DResource8 Interface follow: */
static HRESULT WINAPI IDirect3DCubeTexture8Impl_GetDevice(LPDIRECT3DCUBETEXTURE8 iface, IDirect3DDevice8 **ppDevice) {
    IDirect3DCubeTexture8Impl *This = (IDirect3DCubeTexture8Impl *)iface;
    TRACE("(%p) Relay\n" , This);
    return IDirect3DResource8Impl_GetDevice((LPDIRECT3DRESOURCE8) This, ppDevice);
}

static HRESULT WINAPI IDirect3DCubeTexture8Impl_SetPrivateData(LPDIRECT3DCUBETEXTURE8 iface, REFGUID refguid, CONST void* pData, DWORD SizeOfData, DWORD Flags) {
    IDirect3DCubeTexture8Impl *This = (IDirect3DCubeTexture8Impl *)iface;
    TRACE("(%p) Relay\n", This);
    return IWineD3DCubeTexture_SetPrivateData(This->wineD3DCubeTexture,refguid,pData,SizeOfData,Flags);
}

static HRESULT WINAPI IDirect3DCubeTexture8Impl_GetPrivateData(LPDIRECT3DCUBETEXTURE8 iface, REFGUID refguid, void *pData, DWORD *pSizeOfData) {
    IDirect3DCubeTexture8Impl *This = (IDirect3DCubeTexture8Impl *)iface;
    TRACE("(%p) Relay\n", This);
    return IWineD3DCubeTexture_GetPrivateData(This->wineD3DCubeTexture,refguid,pData,pSizeOfData);
}

static HRESULT WINAPI IDirect3DCubeTexture8Impl_FreePrivateData(LPDIRECT3DCUBETEXTURE8 iface, REFGUID refguid) {
    IDirect3DCubeTexture8Impl *This = (IDirect3DCubeTexture8Impl *)iface;
    TRACE("(%p) Relay\n", This);
    return IWineD3DCubeTexture_FreePrivateData(This->wineD3DCubeTexture,refguid);
}

static DWORD WINAPI IDirect3DCubeTexture8Impl_SetPriority(LPDIRECT3DCUBETEXTURE8 iface, DWORD PriorityNew) {
    IDirect3DCubeTexture8Impl *This = (IDirect3DCubeTexture8Impl *)iface;
    TRACE("(%p) Relay\n", This);
    return IWineD3DCubeTexture_SetPriority(This->wineD3DCubeTexture, PriorityNew);
}

static DWORD WINAPI IDirect3DCubeTexture8Impl_GetPriority(LPDIRECT3DCUBETEXTURE8 iface) {
    IDirect3DCubeTexture8Impl *This = (IDirect3DCubeTexture8Impl *)iface;
    TRACE("(%p) Relay\n", This);
    return IWineD3DCubeTexture_GetPriority(This->wineD3DCubeTexture);
}

static void WINAPI IDirect3DCubeTexture8Impl_PreLoad(LPDIRECT3DCUBETEXTURE8 iface) {
    IDirect3DCubeTexture8Impl *This = (IDirect3DCubeTexture8Impl *)iface;
    TRACE("(%p) Relay\n", This);
    IWineD3DCubeTexture_PreLoad(This->wineD3DCubeTexture);
}

static D3DRESOURCETYPE WINAPI IDirect3DCubeTexture8Impl_GetType(LPDIRECT3DCUBETEXTURE8 iface) {
    IDirect3DCubeTexture8Impl *This = (IDirect3DCubeTexture8Impl *)iface;
    TRACE("(%p) Relay\n", This);
    return IWineD3DCubeTexture_GetType(This->wineD3DCubeTexture);
}

/* IDirect3DCubeTexture8 IDirect3DBaseTexture8 Interface follow: */
static DWORD WINAPI IDirect3DCubeTexture8Impl_SetLOD(LPDIRECT3DCUBETEXTURE8 iface, DWORD LODNew) {
    IDirect3DCubeTexture8Impl *This = (IDirect3DCubeTexture8Impl *)iface;
    TRACE("(%p) Relay\n", This);
    return IWineD3DCubeTexture_SetLOD(This->wineD3DCubeTexture, LODNew);
}

static DWORD WINAPI IDirect3DCubeTexture8Impl_GetLOD(LPDIRECT3DCUBETEXTURE8 iface) {
    IDirect3DCubeTexture8Impl *This = (IDirect3DCubeTexture8Impl *)iface;
    TRACE("(%p) Relay\n", This);
    return IWineD3DCubeTexture_GetLOD((LPDIRECT3DBASETEXTURE8) This);
}

static DWORD WINAPI IDirect3DCubeTexture8Impl_GetLevelCount(LPDIRECT3DCUBETEXTURE8 iface) {
    IDirect3DCubeTexture8Impl *This = (IDirect3DCubeTexture8Impl *)iface;
    TRACE("(%p) Relay\n", This);
    return IWineD3DCubeTexture_GetLevelCount(This->wineD3DCubeTexture);
}

/* IDirect3DCubeTexture8 Interface follow: */
static HRESULT WINAPI IDirect3DCubeTexture8Impl_GetLevelDesc(LPDIRECT3DCUBETEXTURE8 iface, UINT Level, D3DSURFACE_DESC *pDesc) {
    IDirect3DCubeTexture8Impl *This = (IDirect3DCubeTexture8Impl *)iface;
    WINED3DSURFACE_DESC    wined3ddesc;

    TRACE("(%p) Relay\n", This);

    /* As d3d8 and d3d9 structures differ, pass in ptrs to where data needs to go */
    wined3ddesc.Format              = (WINED3DFORMAT *)&pDesc->Format;
    wined3ddesc.Type                = (WINED3DRESOURCETYPE *)&pDesc->Type;
    wined3ddesc.Usage               = &pDesc->Usage;
    wined3ddesc.Pool                = (WINED3DPOOL *) &pDesc->Pool;
    wined3ddesc.Size                = &pDesc->Size;
    wined3ddesc.MultiSampleType     = (WINED3DMULTISAMPLE_TYPE *) &pDesc->MultiSampleType;
    wined3ddesc.MultiSampleQuality  = NULL; /* DirectX9 only */
    wined3ddesc.Width               = &pDesc->Width;
    wined3ddesc.Height              = &pDesc->Height;

    return IWineD3DCubeTexture_GetLevelDesc(This->wineD3DCubeTexture, Level, &wined3ddesc);
}

static HRESULT WINAPI IDirect3DCubeTexture8Impl_GetCubeMapSurface(LPDIRECT3DCUBETEXTURE8 iface, D3DCUBEMAP_FACES FaceType, UINT Level, IDirect3DSurface8 **ppCubeMapSurface) {
    IDirect3DCubeTexture8Impl *This = (IDirect3DCubeTexture8Impl *)iface;
    HRESULT hrc = D3D_OK;
    IWineD3DSurface *mySurface = NULL;

    TRACE("(%p) Relay\n", This);

    hrc = IWineD3DCubeTexture_GetCubeMapSurface(This->wineD3DCubeTexture, (WINED3DCUBEMAP_FACES) FaceType, Level, &mySurface);
    if (hrc == D3D_OK && NULL != ppCubeMapSurface) {
       IWineD3DCubeTexture_GetParent(mySurface, (IUnknown **)ppCubeMapSurface);
       IWineD3DCubeTexture_Release(mySurface);
    }
    return hrc;
}

static HRESULT WINAPI IDirect3DCubeTexture8Impl_LockRect(LPDIRECT3DCUBETEXTURE8 iface, D3DCUBEMAP_FACES FaceType, UINT Level, D3DLOCKED_RECT* pLockedRect, CONST RECT *pRect, DWORD Flags) {
    IDirect3DCubeTexture8Impl *This = (IDirect3DCubeTexture8Impl *)iface;
    TRACE("(%p) Relay\n", This);
    return IWineD3DCubeTexture_LockRect(This->wineD3DCubeTexture, (WINED3DCUBEMAP_FACES) FaceType, Level, (WINED3DLOCKED_RECT *) pLockedRect, pRect, Flags);
}

static HRESULT WINAPI IDirect3DCubeTexture8Impl_UnlockRect(LPDIRECT3DCUBETEXTURE8 iface, D3DCUBEMAP_FACES FaceType, UINT Level) {
    IDirect3DCubeTexture8Impl *This = (IDirect3DCubeTexture8Impl *)iface;
    TRACE("(%p) Relay\n", This);
    return IWineD3DCubeTexture_UnlockRect(This->wineD3DCubeTexture, (WINED3DCUBEMAP_FACES) FaceType, Level);
}

static HRESULT WINAPI IDirect3DCubeTexture8Impl_AddDirtyRect(LPDIRECT3DCUBETEXTURE8 iface, D3DCUBEMAP_FACES FaceType, CONST RECT *pDirtyRect) {
    IDirect3DCubeTexture8Impl *This = (IDirect3DCubeTexture8Impl *)iface;
    TRACE("(%p) Relay\n", This);
    return IWineD3DCubeTexture_AddDirtyRect(This->wineD3DCubeTexture, (WINED3DCUBEMAP_FACES) FaceType, pDirtyRect);
}


const IDirect3DCubeTexture8Vtbl Direct3DCubeTexture8_Vtbl =
{
    /* IUnknown */
    IDirect3DCubeTexture8Impl_QueryInterface,
    IDirect3DCubeTexture8Impl_AddRef,
    IDirect3DCubeTexture8Impl_Release,
    /* IDirect3DResource8 */
    IDirect3DCubeTexture8Impl_GetDevice,
    IDirect3DCubeTexture8Impl_SetPrivateData,
    IDirect3DCubeTexture8Impl_GetPrivateData,
    IDirect3DCubeTexture8Impl_FreePrivateData,
    IDirect3DCubeTexture8Impl_SetPriority,
    IDirect3DCubeTexture8Impl_GetPriority,
    IDirect3DCubeTexture8Impl_PreLoad,
    IDirect3DCubeTexture8Impl_GetType,
    /* IDirect3DBaseTexture8 */
    IDirect3DCubeTexture8Impl_SetLOD,
    IDirect3DCubeTexture8Impl_GetLOD,
    IDirect3DCubeTexture8Impl_GetLevelCount,
    /* IDirect3DCubeTexture8 */
    IDirect3DCubeTexture8Impl_GetLevelDesc,
    IDirect3DCubeTexture8Impl_GetCubeMapSurface,
    IDirect3DCubeTexture8Impl_LockRect,
    IDirect3DCubeTexture8Impl_UnlockRect,
    IDirect3DCubeTexture8Impl_AddDirtyRect
};
