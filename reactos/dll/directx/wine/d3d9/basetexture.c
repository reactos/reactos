/*
 * IDirect3DBaseTexture9 implementation
 *
 * Copyright 2002-2004 Jason Edmeades
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

/* IDirect3DBaseTexture9 IUnknown parts follow: */
static HRESULT WINAPI IDirect3DBaseTexture9Impl_QueryInterface(LPDIRECT3DBASETEXTURE9 iface, REFIID riid, LPVOID* ppobj) {
    IDirect3DBaseTexture9Impl *This = (IDirect3DBaseTexture9Impl *)iface;
    TRACE("(%p) Relay\n" , This);
    if (IsEqualGUID(riid, &IID_IUnknown)
        || IsEqualGUID(riid, &IID_IDirect3DResource9)
        || IsEqualGUID(riid, &IID_IDirect3DBaseTexture9)) {
        IDirect3DBaseTexture9_AddRef(iface);
        *ppobj = This;
        return S_OK;
    }

    WARN("(%p)->(%s,%p),not found\n", This, debugstr_guid(riid), ppobj);
    *ppobj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI IDirect3DBaseTexture9Impl_AddRef(LPDIRECT3DBASETEXTURE9 iface) {
    IDirect3DBaseTexture9Impl *This = (IDirect3DBaseTexture9Impl *)iface;
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) : AddRef from %d\n", This, ref - 1);

    return ref;
}

static ULONG WINAPI IDirect3DBaseTexture9Impl_Release(LPDIRECT3DBASETEXTURE9 iface) {
    IDirect3DBaseTexture9Impl *This = (IDirect3DBaseTexture9Impl *)iface;
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) : ReleaseRef to %d\n", This, ref);

    if (ref == 0) {
        IWineD3DBaseTexture_Release(This->wineD3DBaseTexture);
        HeapFree(GetProcessHeap(), 0, This);
    }
    return ref;
}

/* IDirect3DBaseTexture9 IDirect3DResource9 Interface follow: */
static HRESULT WINAPI IDirect3DBaseTexture9Impl_GetDevice(LPDIRECT3DBASETEXTURE9 iface, IDirect3DDevice9** ppDevice) {
    IDirect3DBaseTexture9Impl *This = (IDirect3DBaseTexture9Impl *)iface;
    TRACE("(%p) Relay\n" , This);
    return IDirect3DResource9Impl_GetDevice((LPDIRECT3DRESOURCE9) This, ppDevice);
}

static HRESULT WINAPI IDirect3DBaseTexture9Impl_SetPrivateData(LPDIRECT3DBASETEXTURE9 iface, REFGUID refguid, CONST void* pData, DWORD SizeOfData, DWORD Flags) {
    IDirect3DBaseTexture9Impl *This = (IDirect3DBaseTexture9Impl *)iface;
    TRACE("(%p) Relay\n" , This);
    return IWineD3DBaseTexture_SetPrivateData(This->wineD3DBaseTexture, refguid, pData, SizeOfData, Flags);
}

static HRESULT WINAPI IDirect3DBaseTexture9Impl_GetPrivateData(LPDIRECT3DBASETEXTURE9 iface, REFGUID refguid, void* pData, DWORD* pSizeOfData) {
    IDirect3DBaseTexture9Impl *This = (IDirect3DBaseTexture9Impl *)iface;
    TRACE("(%p) Relay\n" , This);
    return IWineD3DBaseTexture_GetPrivateData(This->wineD3DBaseTexture, refguid, pData, pSizeOfData);
}

static HRESULT WINAPI IDirect3DBaseTexture9Impl_FreePrivateData(LPDIRECT3DBASETEXTURE9 iface, REFGUID refguid) {
    IDirect3DBaseTexture9Impl *This = (IDirect3DBaseTexture9Impl *)iface;
    TRACE("(%p) Relay\n" , This);
    return IWineD3DBaseTexture_FreePrivateData(This->wineD3DBaseTexture, refguid);
}

static DWORD WINAPI IDirect3DBaseTexture9Impl_SetPriority(LPDIRECT3DBASETEXTURE9 iface, DWORD PriorityNew) {
    IDirect3DBaseTexture9Impl *This = (IDirect3DBaseTexture9Impl *)iface;
    TRACE("(%p) Relay\n" , This);
    return IWineD3DBaseTexture_SetPriority(This->wineD3DBaseTexture, PriorityNew);
}

static DWORD WINAPI IDirect3DBaseTexture9Impl_GetPriority(LPDIRECT3DBASETEXTURE9 iface) {
    IDirect3DBaseTexture9Impl *This = (IDirect3DBaseTexture9Impl *)iface;
    TRACE("(%p) Relay\n" , This);
    return IWineD3DBaseTexture_GetPriority(This->wineD3DBaseTexture);
}

static void WINAPI IDirect3DBaseTexture9Impl_PreLoad(LPDIRECT3DBASETEXTURE9 iface) {
    IDirect3DBaseTexture9Impl *This = (IDirect3DBaseTexture9Impl *)iface;
    TRACE("(%p) Relay\n" , This);
    IWineD3DBaseTexture_PreLoad(This->wineD3DBaseTexture);
    return ;
}

static D3DRESOURCETYPE WINAPI IDirect3DBaseTexture9Impl_GetType(LPDIRECT3DBASETEXTURE9 iface) {
    IDirect3DBaseTexture9Impl *This = (IDirect3DBaseTexture9Impl *)iface;
    TRACE("(%p) Relay\n" , This);
    return IWineD3DBaseTexture_GetType(This->wineD3DBaseTexture);
}

/* IDirect3DBaseTexture9 Interface follow: */
static DWORD  WINAPI IDirect3DBaseTexture9Impl_SetLOD(LPDIRECT3DBASETEXTURE9 iface, DWORD LODNew) {
    IDirect3DBaseTexture9Impl *This = (IDirect3DBaseTexture9Impl *)iface;
    TRACE("(%p) Relay\n" , This);
    return IWineD3DBaseTexture_SetLOD(This->wineD3DBaseTexture, LODNew);
}

DWORD WINAPI IDirect3DBaseTexture9Impl_GetLOD(LPDIRECT3DBASETEXTURE9 iface) {
    IDirect3DBaseTexture9Impl *This = (IDirect3DBaseTexture9Impl *)iface;
    TRACE("(%p) Relay\n" , This);
    return IWineD3DBaseTexture_GetLOD(This->wineD3DBaseTexture);
}

static DWORD WINAPI IDirect3DBaseTexture9Impl_GetLevelCount(LPDIRECT3DBASETEXTURE9 iface) {
    IDirect3DBaseTexture9Impl *This = (IDirect3DBaseTexture9Impl *)iface;
    TRACE("(%p) Relay\n" , This);
    return IWineD3DBaseTexture_GetLevelCount(This->wineD3DBaseTexture);
}

static HRESULT WINAPI IDirect3DBaseTexture9Impl_SetAutoGenFilterType(LPDIRECT3DBASETEXTURE9 iface, D3DTEXTUREFILTERTYPE FilterType) {
    IDirect3DBaseTexture9Impl *This = (IDirect3DBaseTexture9Impl *)iface;
    TRACE("(%p) Relay\n" , This);
    return IWineD3DBaseTexture_SetAutoGenFilterType(This->wineD3DBaseTexture, (WINED3DTEXTUREFILTERTYPE) FilterType);
}

static D3DTEXTUREFILTERTYPE WINAPI IDirect3DBaseTexture9Impl_GetAutoGenFilterType(LPDIRECT3DBASETEXTURE9 iface) {
    IDirect3DBaseTexture9Impl *This = (IDirect3DBaseTexture9Impl *)iface;
    TRACE("(%p) Relay\n" , This);
    return (D3DTEXTUREFILTERTYPE) IWineD3DBaseTexture_GetAutoGenFilterType(This->wineD3DBaseTexture);
}

static void WINAPI IDirect3DBaseTexture9Impl_GenerateMipSubLevels(LPDIRECT3DBASETEXTURE9 iface) {
    IDirect3DBaseTexture9Impl *This = (IDirect3DBaseTexture9Impl *)iface;
    TRACE("(%p) Relay\n" , This);
    IWineD3DBaseTexture_GenerateMipSubLevels(This->wineD3DBaseTexture);
}

const IDirect3DBaseTexture9Vtbl Direct3DBaseTexture9_Vtbl =
{
    IDirect3DBaseTexture9Impl_QueryInterface,
    IDirect3DBaseTexture9Impl_AddRef,
    IDirect3DBaseTexture9Impl_Release,
    IDirect3DBaseTexture9Impl_GetDevice,
    IDirect3DBaseTexture9Impl_SetPrivateData,
    IDirect3DBaseTexture9Impl_GetPrivateData,
    IDirect3DBaseTexture9Impl_FreePrivateData,
    IDirect3DBaseTexture9Impl_SetPriority,
    IDirect3DBaseTexture9Impl_GetPriority,
    IDirect3DBaseTexture9Impl_PreLoad,
    IDirect3DBaseTexture9Impl_GetType,
    IDirect3DBaseTexture9Impl_SetLOD,
    IDirect3DBaseTexture9Impl_GetLOD,
    IDirect3DBaseTexture9Impl_GetLevelCount,
    IDirect3DBaseTexture9Impl_SetAutoGenFilterType,
    IDirect3DBaseTexture9Impl_GetAutoGenFilterType,
    IDirect3DBaseTexture9Impl_GenerateMipSubLevels   
};
