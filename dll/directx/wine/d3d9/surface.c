/*
 * IDirect3DSurface9 implementation
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

/* IDirect3DSurface9 IUnknown parts follow: */
static HRESULT WINAPI IDirect3DSurface9Impl_QueryInterface(LPDIRECT3DSURFACE9 iface, REFIID riid, LPVOID* ppobj) {
    IDirect3DSurface9Impl *This = (IDirect3DSurface9Impl *)iface;

    if (IsEqualGUID(riid, &IID_IUnknown)
        || IsEqualGUID(riid, &IID_IDirect3DResource9)
        || IsEqualGUID(riid, &IID_IDirect3DSurface9)) {
        IUnknown_AddRef(iface);
        *ppobj = This;
        return S_OK;
    }

    WARN("(%p)->(%s,%p),not found\n", This, debugstr_guid(riid), ppobj);
    *ppobj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI IDirect3DSurface9Impl_AddRef(LPDIRECT3DSURFACE9 iface) {
    IDirect3DSurface9Impl *This = (IDirect3DSurface9Impl *)iface;

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

static ULONG WINAPI IDirect3DSurface9Impl_Release(LPDIRECT3DSURFACE9 iface) {
    IDirect3DSurface9Impl *This = (IDirect3DSurface9Impl *)iface;

    TRACE("(%p)\n", This);

    if (This->forwardReference) {
        /* Forward to the containerParent */
        TRACE("(%p) : Forwarding to %p\n", This, This->forwardReference);
        return IUnknown_Release(This->forwardReference);
    } else {
        /* No container, handle our own refcounting */
        ULONG ref = InterlockedDecrement(&This->ref);
        TRACE("(%p) : ReleaseRef to %d\n", This, ref);

        if (ref == 0) {
            if (This->parentDevice) IUnknown_Release(This->parentDevice);
            if (!This->isImplicit) {
                EnterCriticalSection(&d3d9_cs);
                IWineD3DSurface_Release(This->wineD3DSurface);
                LeaveCriticalSection(&d3d9_cs);
                HeapFree(GetProcessHeap(), 0, This);
            }
        }

        return ref;
    }
}

/* IDirect3DSurface9 IDirect3DResource9 Interface follow: */
static HRESULT WINAPI IDirect3DSurface9Impl_GetDevice(LPDIRECT3DSURFACE9 iface, IDirect3DDevice9** ppDevice) {
    IDirect3DSurface9Impl *This = (IDirect3DSurface9Impl *)iface;
    HRESULT hr;
    TRACE("(%p)->(%p)\n", This, ppDevice);

    EnterCriticalSection(&d3d9_cs);
    hr = IDirect3DResource9Impl_GetDevice((LPDIRECT3DRESOURCE9) This, ppDevice);
    LeaveCriticalSection(&d3d9_cs);
    return hr;
}

static HRESULT WINAPI IDirect3DSurface9Impl_SetPrivateData(LPDIRECT3DSURFACE9 iface, REFGUID refguid, CONST void* pData, DWORD SizeOfData, DWORD Flags) {
    IDirect3DSurface9Impl *This = (IDirect3DSurface9Impl *)iface;
    HRESULT hr;
    TRACE("(%p) Relay\n", This);

    EnterCriticalSection(&d3d9_cs);
    hr = IWineD3DSurface_SetPrivateData(This->wineD3DSurface, refguid, pData, SizeOfData, Flags);
    LeaveCriticalSection(&d3d9_cs);
    return hr;
}

static HRESULT WINAPI IDirect3DSurface9Impl_GetPrivateData(LPDIRECT3DSURFACE9 iface, REFGUID refguid, void* pData, DWORD* pSizeOfData) {
    IDirect3DSurface9Impl *This = (IDirect3DSurface9Impl *)iface;
    HRESULT hr;
    TRACE("(%p) Relay\n", This);

    EnterCriticalSection(&d3d9_cs);
    hr = IWineD3DSurface_GetPrivateData(This->wineD3DSurface, refguid, pData, pSizeOfData);
    LeaveCriticalSection(&d3d9_cs);
    return hr;
}

static HRESULT WINAPI IDirect3DSurface9Impl_FreePrivateData(LPDIRECT3DSURFACE9 iface, REFGUID refguid) {
    IDirect3DSurface9Impl *This = (IDirect3DSurface9Impl *)iface;
    HRESULT hr;
    TRACE("(%p) Relay\n", This);

    EnterCriticalSection(&d3d9_cs);
    hr = IWineD3DSurface_FreePrivateData(This->wineD3DSurface, refguid);
    LeaveCriticalSection(&d3d9_cs);
    return hr;
}

static DWORD WINAPI IDirect3DSurface9Impl_SetPriority(LPDIRECT3DSURFACE9 iface, DWORD PriorityNew) {
    IDirect3DSurface9Impl *This = (IDirect3DSurface9Impl *)iface;
    HRESULT hr;
    TRACE("(%p) Relay\n", This);

    EnterCriticalSection(&d3d9_cs);
    hr = IWineD3DSurface_SetPriority(This->wineD3DSurface, PriorityNew);
    LeaveCriticalSection(&d3d9_cs);
    return hr;
}

static DWORD WINAPI IDirect3DSurface9Impl_GetPriority(LPDIRECT3DSURFACE9 iface) {
    IDirect3DSurface9Impl *This = (IDirect3DSurface9Impl *)iface;
    HRESULT hr;
    TRACE("(%p) Relay\n", This);

    EnterCriticalSection(&d3d9_cs);
    hr = IWineD3DSurface_GetPriority(This->wineD3DSurface);
    LeaveCriticalSection(&d3d9_cs);
    return hr;
}

static void WINAPI IDirect3DSurface9Impl_PreLoad(LPDIRECT3DSURFACE9 iface) {
    IDirect3DSurface9Impl *This = (IDirect3DSurface9Impl *)iface;
    TRACE("(%p) Relay\n", This);

    EnterCriticalSection(&d3d9_cs);
    IWineD3DSurface_PreLoad(This->wineD3DSurface);
    LeaveCriticalSection(&d3d9_cs);
    return ;
}

static D3DRESOURCETYPE WINAPI IDirect3DSurface9Impl_GetType(LPDIRECT3DSURFACE9 iface) {
    IDirect3DSurface9Impl *This = (IDirect3DSurface9Impl *)iface;
    D3DRESOURCETYPE ret;
    TRACE("(%p) Relay\n", This);

    EnterCriticalSection(&d3d9_cs);
    ret = IWineD3DSurface_GetType(This->wineD3DSurface);
    LeaveCriticalSection(&d3d9_cs);
    return ret;
}

/* IDirect3DSurface9 Interface follow: */
static HRESULT WINAPI IDirect3DSurface9Impl_GetContainer(LPDIRECT3DSURFACE9 iface, REFIID riid, void** ppContainer) {
    IDirect3DSurface9Impl *This = (IDirect3DSurface9Impl *)iface;
    HRESULT res;

    TRACE("(This %p, riid %s, ppContainer %p)\n", This, debugstr_guid(riid), ppContainer);

    if (!This->container) return E_NOINTERFACE;

    if (!ppContainer) {
        ERR("Called without a valid ppContainer\n");
    }

    res = IUnknown_QueryInterface(This->container, riid, ppContainer);

    TRACE("Returning ppContainer %p, *ppContainer %p\n", ppContainer, *ppContainer);

    return res;
}

static HRESULT WINAPI IDirect3DSurface9Impl_GetDesc(LPDIRECT3DSURFACE9 iface, D3DSURFACE_DESC* pDesc) {
    IDirect3DSurface9Impl *This = (IDirect3DSurface9Impl *)iface;
    WINED3DSURFACE_DESC    wined3ddesc;
    UINT                   tmpInt = -1;
    HRESULT hr;
    TRACE("(%p) Relay\n", This);

    /* As d3d8 and d3d9 structures differ, pass in ptrs to where data needs to go */
    wined3ddesc.Format              = (WINED3DFORMAT *)&pDesc->Format;
    wined3ddesc.Type                = (WINED3DRESOURCETYPE *)&pDesc->Type;
    wined3ddesc.Usage               = &pDesc->Usage;
    wined3ddesc.Pool                = (WINED3DPOOL *) &pDesc->Pool;
    wined3ddesc.Size                = &tmpInt;
    wined3ddesc.MultiSampleType     = (WINED3DMULTISAMPLE_TYPE *) &pDesc->MultiSampleType;
    wined3ddesc.MultiSampleQuality  = &pDesc->MultiSampleQuality;
    wined3ddesc.Width               = &pDesc->Width;
    wined3ddesc.Height              = &pDesc->Height;

    EnterCriticalSection(&d3d9_cs);
    hr = IWineD3DSurface_GetDesc(This->wineD3DSurface, &wined3ddesc);
    LeaveCriticalSection(&d3d9_cs);
    return hr;
}

static HRESULT WINAPI IDirect3DSurface9Impl_LockRect(LPDIRECT3DSURFACE9 iface, D3DLOCKED_RECT* pLockedRect, CONST RECT* pRect, DWORD Flags) {
    IDirect3DSurface9Impl *This = (IDirect3DSurface9Impl *)iface;
    HRESULT hr;
    TRACE("(%p) Relay\n", This);

    EnterCriticalSection(&d3d9_cs);
    TRACE("(%p) calling IWineD3DSurface_LockRect %p %p %p %d\n", This, This->wineD3DSurface, pLockedRect, pRect, Flags);
    hr = IWineD3DSurface_LockRect(This->wineD3DSurface, (WINED3DLOCKED_RECT *) pLockedRect, pRect, Flags);
    LeaveCriticalSection(&d3d9_cs);
    return hr;
}

static HRESULT WINAPI IDirect3DSurface9Impl_UnlockRect(LPDIRECT3DSURFACE9 iface) {
    IDirect3DSurface9Impl *This = (IDirect3DSurface9Impl *)iface;
    HRESULT hr;
    TRACE("(%p) Relay\n", This);

    EnterCriticalSection(&d3d9_cs);
    hr = IWineD3DSurface_UnlockRect(This->wineD3DSurface);
    LeaveCriticalSection(&d3d9_cs);
    return hr;
}

static HRESULT WINAPI IDirect3DSurface9Impl_GetDC(LPDIRECT3DSURFACE9 iface, HDC* phdc) {
    IDirect3DSurface9Impl *This = (IDirect3DSurface9Impl *)iface;
    HRESULT hr;
    TRACE("(%p) Relay\n", This);

    EnterCriticalSection(&d3d9_cs);
    hr = IWineD3DSurface_GetDC(This->wineD3DSurface, phdc);
    LeaveCriticalSection(&d3d9_cs);
    return hr;
}

static HRESULT WINAPI IDirect3DSurface9Impl_ReleaseDC(LPDIRECT3DSURFACE9 iface, HDC hdc) {
    IDirect3DSurface9Impl *This = (IDirect3DSurface9Impl *)iface;
    HRESULT hr;
    TRACE("(%p) Relay\n", This);

    EnterCriticalSection(&d3d9_cs);
    hr = IWineD3DSurface_ReleaseDC(This->wineD3DSurface, hdc);
    LeaveCriticalSection(&d3d9_cs);
    return hr;
}


const IDirect3DSurface9Vtbl Direct3DSurface9_Vtbl =
{
    /* IUnknown */
    IDirect3DSurface9Impl_QueryInterface,
    IDirect3DSurface9Impl_AddRef,
    IDirect3DSurface9Impl_Release,
    /* IDirect3DResource9 */
    IDirect3DSurface9Impl_GetDevice,
    IDirect3DSurface9Impl_SetPrivateData,
    IDirect3DSurface9Impl_GetPrivateData,
    IDirect3DSurface9Impl_FreePrivateData,
    IDirect3DSurface9Impl_SetPriority,
    IDirect3DSurface9Impl_GetPriority,
    IDirect3DSurface9Impl_PreLoad,
    IDirect3DSurface9Impl_GetType,
    /* IDirect3DSurface9 */
    IDirect3DSurface9Impl_GetContainer,
    IDirect3DSurface9Impl_GetDesc,
    IDirect3DSurface9Impl_LockRect,
    IDirect3DSurface9Impl_UnlockRect,
    IDirect3DSurface9Impl_GetDC,
    IDirect3DSurface9Impl_ReleaseDC
};
