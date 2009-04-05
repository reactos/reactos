/*
 * IDirect3DVolume8 implementation
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

/* IDirect3DVolume8 IUnknown parts follow: */
static HRESULT WINAPI IDirect3DVolume8Impl_QueryInterface(LPDIRECT3DVOLUME8 iface, REFIID riid, LPVOID *ppobj) {
    IDirect3DVolume8Impl *This = (IDirect3DVolume8Impl *)iface;

    if (IsEqualGUID(riid, &IID_IUnknown)
        || IsEqualGUID(riid, &IID_IDirect3DVolume8)) {
        IUnknown_AddRef(iface);
        *ppobj = This;
        return S_OK;
    }

    WARN("(%p)->(%s,%p),not found\n", This, debugstr_guid(riid), ppobj);
    *ppobj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI IDirect3DVolume8Impl_AddRef(LPDIRECT3DVOLUME8 iface) {
    IDirect3DVolume8Impl *This = (IDirect3DVolume8Impl *)iface;

    TRACE("(%p)\n", This);

    if (This->forwardReference) {
        /* Forward to the containerParent */
        TRACE("(%p) : Forwarding to %p\n", This, This->forwardReference);
        return IUnknown_AddRef(This->forwardReference);
    } else {
        /* No container, handle our own refcounting */
        ULONG ref = InterlockedIncrement(&This->ref);
        TRACE("(%p) : AddRef from %d\n", This, ref - 1);
        return ref;
    }
}

static ULONG WINAPI IDirect3DVolume8Impl_Release(LPDIRECT3DVOLUME8 iface) {
    IDirect3DVolume8Impl *This = (IDirect3DVolume8Impl *)iface;

    TRACE("(%p)\n", This);

    if (This->forwardReference) {
        /* Forward to the containerParent */
        TRACE("(%p) : Forwarding to %p\n", This, This->forwardReference);
        return IUnknown_Release(This->forwardReference);
    }
    else {
        /* No container, handle our own refcounting */
        ULONG ref = InterlockedDecrement(&This->ref);
        TRACE("(%p) : ReleaseRef to %d\n", This, ref);

        if (ref == 0) {
            EnterCriticalSection(&d3d8_cs);
            IWineD3DVolume_Release(This->wineD3DVolume);
            LeaveCriticalSection(&d3d8_cs);
            HeapFree(GetProcessHeap(), 0, This);
        }

        return ref;
    }
}

/* IDirect3DVolume8 Interface follow: */
static HRESULT WINAPI IDirect3DVolume8Impl_GetDevice(LPDIRECT3DVOLUME8 iface, IDirect3DDevice8 **ppDevice) {
    IDirect3DVolume8Impl *This = (IDirect3DVolume8Impl *)iface;
    IWineD3DDevice       *myDevice = NULL;

    EnterCriticalSection(&d3d8_cs);
    IWineD3DVolume_GetDevice(This->wineD3DVolume, &myDevice);
    IWineD3DDevice_GetParent(myDevice, (IUnknown **)ppDevice);
    IWineD3DDevice_Release(myDevice);
    LeaveCriticalSection(&d3d8_cs);
    return D3D_OK;
}

static HRESULT WINAPI IDirect3DVolume8Impl_SetPrivateData(LPDIRECT3DVOLUME8 iface, REFGUID refguid, CONST void *pData, DWORD SizeOfData, DWORD Flags) {
    IDirect3DVolume8Impl *This = (IDirect3DVolume8Impl *)iface;
    HRESULT hr;
    TRACE("(%p) Relay\n", This);

    EnterCriticalSection(&d3d8_cs);
    hr = IWineD3DVolume_SetPrivateData(This->wineD3DVolume, refguid, pData, SizeOfData, Flags);
    LeaveCriticalSection(&d3d8_cs);
    return hr;
}

static HRESULT WINAPI IDirect3DVolume8Impl_GetPrivateData(LPDIRECT3DVOLUME8 iface, REFGUID  refguid, void *pData, DWORD* pSizeOfData) {
    IDirect3DVolume8Impl *This = (IDirect3DVolume8Impl *)iface;
    HRESULT hr;
    TRACE("(%p) Relay\n", This);

    EnterCriticalSection(&d3d8_cs);
    hr = IWineD3DVolume_GetPrivateData(This->wineD3DVolume, refguid, pData, pSizeOfData);
    LeaveCriticalSection(&d3d8_cs);
    return hr;
}

static HRESULT WINAPI IDirect3DVolume8Impl_FreePrivateData(LPDIRECT3DVOLUME8 iface, REFGUID refguid) {
    IDirect3DVolume8Impl *This = (IDirect3DVolume8Impl *)iface;
    HRESULT hr;
    TRACE("(%p) Relay\n", This);

    EnterCriticalSection(&d3d8_cs);
    hr = IWineD3DVolume_FreePrivateData(This->wineD3DVolume, refguid);
    LeaveCriticalSection(&d3d8_cs);
    return hr;
}

static HRESULT WINAPI IDirect3DVolume8Impl_GetContainer(LPDIRECT3DVOLUME8 iface, REFIID riid, void **ppContainer) {
    IDirect3DVolume8Impl *This = (IDirect3DVolume8Impl *)iface;
    HRESULT res;

    TRACE("(This %p, riid %s, ppContainer %p)\n", This, debugstr_guid(riid), ppContainer);

    if (!This->container) return E_NOINTERFACE;

    if (!ppContainer) {
        ERR("Called without a valid ppContainer.\n");
    }

    res = IUnknown_QueryInterface(This->container, riid, ppContainer);

    TRACE("Returning ppContainer %p, *ppContainer %p\n", ppContainer, *ppContainer);

    return res;
}

static HRESULT WINAPI IDirect3DVolume8Impl_GetDesc(LPDIRECT3DVOLUME8 iface, D3DVOLUME_DESC *pDesc) {
    IDirect3DVolume8Impl *This = (IDirect3DVolume8Impl *)iface;
    HRESULT hr;
    WINED3DVOLUME_DESC     wined3ddesc;

    TRACE("(%p) Relay\n", This);

    /* As d3d8 and d3d9 structures differ, pass in ptrs to where data needs to go */
    wined3ddesc.Format              = (WINED3DFORMAT *)&pDesc->Format;
    wined3ddesc.Type                = (WINED3DRESOURCETYPE *)&pDesc->Type;
    wined3ddesc.Usage               = &pDesc->Usage;
    wined3ddesc.Pool                = (WINED3DPOOL *) &pDesc->Pool;
    wined3ddesc.Size                = &pDesc->Size;
    wined3ddesc.Width               = &pDesc->Width;
    wined3ddesc.Height              = &pDesc->Height;
    wined3ddesc.Depth               = &pDesc->Depth;

    EnterCriticalSection(&d3d8_cs);
    hr = IWineD3DVolume_GetDesc(This->wineD3DVolume, &wined3ddesc);
    LeaveCriticalSection(&d3d8_cs);

    if (SUCCEEDED(hr)) pDesc->Format = d3dformat_from_wined3dformat(pDesc->Format);

    return hr;
}

static HRESULT WINAPI IDirect3DVolume8Impl_LockBox(LPDIRECT3DVOLUME8 iface, D3DLOCKED_BOX *pLockedVolume, CONST D3DBOX *pBox, DWORD Flags) {
    IDirect3DVolume8Impl *This = (IDirect3DVolume8Impl *)iface;
    HRESULT hr;
    TRACE("(%p) relay %p %p %p %d\n", This, This->wineD3DVolume, pLockedVolume, pBox, Flags);

    EnterCriticalSection(&d3d8_cs);
    hr = IWineD3DVolume_LockBox(This->wineD3DVolume, (WINED3DLOCKED_BOX *) pLockedVolume, (CONST WINED3DBOX *) pBox, Flags);
    LeaveCriticalSection(&d3d8_cs);
    return hr;
}

static HRESULT WINAPI IDirect3DVolume8Impl_UnlockBox(LPDIRECT3DVOLUME8 iface) {
    IDirect3DVolume8Impl *This = (IDirect3DVolume8Impl *)iface;
    HRESULT hr;
    TRACE("(%p) relay %p\n", This, This->wineD3DVolume);

    EnterCriticalSection(&d3d8_cs);
    hr = IWineD3DVolume_UnlockBox(This->wineD3DVolume);
    LeaveCriticalSection(&d3d8_cs);
    return hr;
}

const IDirect3DVolume8Vtbl Direct3DVolume8_Vtbl =
{
    /* IUnknown */
    IDirect3DVolume8Impl_QueryInterface,
    IDirect3DVolume8Impl_AddRef,
    IDirect3DVolume8Impl_Release,
    /* IDirect3DVolume8 */
    IDirect3DVolume8Impl_GetDevice,
    IDirect3DVolume8Impl_SetPrivateData,
    IDirect3DVolume8Impl_GetPrivateData,
    IDirect3DVolume8Impl_FreePrivateData,
    IDirect3DVolume8Impl_GetContainer,
    IDirect3DVolume8Impl_GetDesc,
    IDirect3DVolume8Impl_LockBox,
    IDirect3DVolume8Impl_UnlockBox
};

ULONG WINAPI D3D8CB_DestroyVolume(IWineD3DVolume *pVolume) {
    IDirect3DVolume8Impl* volumeParent;

    IWineD3DVolume_GetParent(pVolume, (IUnknown **) &volumeParent);
    /* GetParent's AddRef was forwarded to an object in destruction.
     * Releasing it here again would cause an endless recursion. */
    volumeParent->forwardReference = NULL;
    return IDirect3DVolume8_Release((IDirect3DVolume8*) volumeParent);
}
