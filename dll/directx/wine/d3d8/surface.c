/*
 * IDirect3DSurface8 implementation
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

/* IDirect3DSurface8 IUnknown parts follow: */
static HRESULT WINAPI IDirect3DSurface8Impl_QueryInterface(LPDIRECT3DSURFACE8 iface, REFIID riid, LPVOID *ppobj) {
    IDirect3DSurface8Impl *This = (IDirect3DSurface8Impl *)iface;

    if (IsEqualGUID(riid, &IID_IUnknown)
        || IsEqualGUID(riid, &IID_IDirect3DResource8)
        || IsEqualGUID(riid, &IID_IDirect3DSurface8)) {
        IUnknown_AddRef(iface);
        *ppobj = This;
        return S_OK;
    }

    WARN("(%p)->(%s,%p),not found\n", This, debugstr_guid(riid), ppobj);
    *ppobj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI IDirect3DSurface8Impl_AddRef(LPDIRECT3DSURFACE8 iface) {
    IDirect3DSurface8Impl *This = (IDirect3DSurface8Impl *)iface;

    TRACE("(%p)\n", This);

    if (This->forwardReference) {
        /* Forward refcounting */
        TRACE("(%p) : Forwarding to %p\n", This, This->forwardReference);
        return IUnknown_AddRef(This->forwardReference);
    } else {
        /* No container, handle our own refcounting */
        ULONG ref = InterlockedIncrement(&This->ref);
        if(ref == 1 && This->parentDevice) IUnknown_AddRef(This->parentDevice);
        TRACE("(%p) : AddRef from %d\n", This, ref - 1);
        return ref;
    }
}

static ULONG WINAPI IDirect3DSurface8Impl_Release(LPDIRECT3DSURFACE8 iface) {
    IDirect3DSurface8Impl *This = (IDirect3DSurface8Impl *)iface;

    TRACE("(%p)\n", This);

    if (This->forwardReference) {
        /* Forward refcounting */
        TRACE("(%p) : Forwarding to %p\n", This, This->forwardReference);
        return IUnknown_Release(This->forwardReference);
    } else {
        /* No container, handle our own refcounting */
        ULONG ref = InterlockedDecrement(&This->ref);
        TRACE("(%p) : ReleaseRef to %d\n", This, ref);

        if (ref == 0) {
            if (This->parentDevice) IUnknown_Release(This->parentDevice);
            /* Implicit surfaces are destroyed with the device, not if refcount reaches 0. */
            if (!This->isImplicit) {
                EnterCriticalSection(&d3d8_cs);
                IWineD3DSurface_Release(This->wineD3DSurface);
                LeaveCriticalSection(&d3d8_cs);
                HeapFree(GetProcessHeap(), 0, This);
            }
        }

        return ref;
    }
}

/* IDirect3DSurface8 IDirect3DResource8 Interface follow: */
static HRESULT WINAPI IDirect3DSurface8Impl_GetDevice(LPDIRECT3DSURFACE8 iface, IDirect3DDevice8 **ppDevice) {
    IDirect3DSurface8Impl *This = (IDirect3DSurface8Impl *)iface;
    HRESULT hr;
    TRACE("(%p)->(%p)\n", This, ppDevice);

    EnterCriticalSection(&d3d8_cs);
    hr = IDirect3DResource8Impl_GetDevice((LPDIRECT3DRESOURCE8) This, ppDevice);
    LeaveCriticalSection(&d3d8_cs);
    return hr;
}

static HRESULT WINAPI IDirect3DSurface8Impl_SetPrivateData(LPDIRECT3DSURFACE8 iface, REFGUID refguid, CONST void *pData, DWORD SizeOfData, DWORD Flags) {
    IDirect3DSurface8Impl *This = (IDirect3DSurface8Impl *)iface;
    HRESULT hr;
    TRACE("(%p) Relay\n", This);

    EnterCriticalSection(&d3d8_cs);
    hr = IWineD3DSurface_SetPrivateData(This->wineD3DSurface, refguid, pData, SizeOfData, Flags);
    LeaveCriticalSection(&d3d8_cs);
    return hr;
}

static HRESULT WINAPI IDirect3DSurface8Impl_GetPrivateData(LPDIRECT3DSURFACE8 iface, REFGUID refguid, void *pData, DWORD *pSizeOfData) {
    IDirect3DSurface8Impl *This = (IDirect3DSurface8Impl *)iface;
    HRESULT hr;
    TRACE("(%p) Relay\n", This);

    EnterCriticalSection(&d3d8_cs);
    hr = IWineD3DSurface_GetPrivateData(This->wineD3DSurface, refguid, pData, pSizeOfData);
    LeaveCriticalSection(&d3d8_cs);
    return hr;
}

static HRESULT WINAPI IDirect3DSurface8Impl_FreePrivateData(LPDIRECT3DSURFACE8 iface, REFGUID refguid) {
    IDirect3DSurface8Impl *This = (IDirect3DSurface8Impl *)iface;
    HRESULT hr;
    TRACE("(%p) Relay\n", This);

    EnterCriticalSection(&d3d8_cs);
    hr = IWineD3DSurface_FreePrivateData(This->wineD3DSurface, refguid);
    LeaveCriticalSection(&d3d8_cs);
    return hr;
}

/* IDirect3DSurface8 Interface follow: */
static HRESULT WINAPI IDirect3DSurface8Impl_GetContainer(LPDIRECT3DSURFACE8 iface, REFIID riid, void **ppContainer) {
    IDirect3DSurface8Impl *This = (IDirect3DSurface8Impl *)iface;
    HRESULT res;

    TRACE("(%p) Relay\n", This);

    if (!This->container) return E_NOINTERFACE;

    res = IUnknown_QueryInterface(This->container, riid, ppContainer);

    TRACE("(%p) : returning %p\n", This, *ppContainer);
    return res;
}

static HRESULT WINAPI IDirect3DSurface8Impl_GetDesc(LPDIRECT3DSURFACE8 iface, D3DSURFACE_DESC *pDesc) {
    IDirect3DSurface8Impl *This = (IDirect3DSurface8Impl *)iface;
    WINED3DSURFACE_DESC    wined3ddesc;
    HRESULT hr;
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

    EnterCriticalSection(&d3d8_cs);
    hr = IWineD3DSurface_GetDesc(This->wineD3DSurface, &wined3ddesc);
    LeaveCriticalSection(&d3d8_cs);
    return hr;
}

static HRESULT WINAPI IDirect3DSurface8Impl_LockRect(LPDIRECT3DSURFACE8 iface, D3DLOCKED_RECT *pLockedRect, CONST RECT *pRect, DWORD Flags) {
    IDirect3DSurface8Impl *This = (IDirect3DSurface8Impl *)iface;
    HRESULT hr;
    TRACE("(%p) Relay\n", This);
    TRACE("(%p) calling IWineD3DSurface_LockRect %p %p %p %d\n", This, This->wineD3DSurface, pLockedRect, pRect, Flags);

    EnterCriticalSection(&d3d8_cs);
    if (pRect) {
        D3DSURFACE_DESC desc;
        IDirect3DSurface8_GetDesc(iface, &desc);

        if ((pRect->left < 0)
                || (pRect->top < 0)
                || (pRect->left >= pRect->right)
                || (pRect->top >= pRect->bottom)
                || (pRect->right > desc.Width)
                || (pRect->bottom > desc.Height)) {
            WARN("Trying to lock an invalid rectangle, returning D3DERR_INVALIDCALL\n");
            LeaveCriticalSection(&d3d8_cs);
            return D3DERR_INVALIDCALL;
        }
    }

    hr = IWineD3DSurface_LockRect(This->wineD3DSurface, (WINED3DLOCKED_RECT *) pLockedRect, pRect, Flags);
    LeaveCriticalSection(&d3d8_cs);
    return hr;
}

static HRESULT WINAPI IDirect3DSurface8Impl_UnlockRect(LPDIRECT3DSURFACE8 iface) {
    IDirect3DSurface8Impl *This = (IDirect3DSurface8Impl *)iface;
    HRESULT hr;
    TRACE("(%p) Relay\n", This);

    EnterCriticalSection(&d3d8_cs);
    hr = IWineD3DSurface_UnlockRect(This->wineD3DSurface);
    LeaveCriticalSection(&d3d8_cs);
    return hr;
}

const IDirect3DSurface8Vtbl Direct3DSurface8_Vtbl =
{
    /* IUnknown */
    IDirect3DSurface8Impl_QueryInterface,
    IDirect3DSurface8Impl_AddRef,
    IDirect3DSurface8Impl_Release,
    /* IDirect3DResource8 */
    IDirect3DSurface8Impl_GetDevice,
    IDirect3DSurface8Impl_SetPrivateData,
    IDirect3DSurface8Impl_GetPrivateData,
    IDirect3DSurface8Impl_FreePrivateData,
    /* IDirect3DSurface8 */
    IDirect3DSurface8Impl_GetContainer,
    IDirect3DSurface8Impl_GetDesc,
    IDirect3DSurface8Impl_LockRect,
    IDirect3DSurface8Impl_UnlockRect
};
