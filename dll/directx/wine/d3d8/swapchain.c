/*
 * IDirect3DSwapChain8 implementation
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

/* IDirect3DSwapChain IUnknown parts follow: */
static HRESULT WINAPI IDirect3DSwapChain8Impl_QueryInterface(LPDIRECT3DSWAPCHAIN8 iface, REFIID riid, LPVOID* ppobj)
{
    IDirect3DSwapChain8Impl *This = (IDirect3DSwapChain8Impl *)iface;

    TRACE("iface %p, riid %s, object %p.\n", iface, debugstr_guid(riid), ppobj);

    if (IsEqualGUID(riid, &IID_IUnknown)
        || IsEqualGUID(riid, &IID_IDirect3DSwapChain8)) {
        IUnknown_AddRef(iface);
        *ppobj = This;
        return S_OK;
    }

    WARN("(%p)->(%s,%p),not found\n", This, debugstr_guid(riid), ppobj);
    *ppobj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI IDirect3DSwapChain8Impl_AddRef(LPDIRECT3DSWAPCHAIN8 iface) {
    IDirect3DSwapChain8Impl *This = (IDirect3DSwapChain8Impl *)iface;
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("%p increasing refcount to %u.\n", iface, ref);

    return ref;
}

static ULONG WINAPI IDirect3DSwapChain8Impl_Release(LPDIRECT3DSWAPCHAIN8 iface) {
    IDirect3DSwapChain8Impl *This = (IDirect3DSwapChain8Impl *)iface;
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("%p decreasing refcount to %u.\n", iface, ref);

    if (ref == 0) {
        wined3d_mutex_lock();
        IWineD3DSwapChain_Destroy(This->wineD3DSwapChain);
        wined3d_mutex_unlock();

        if (This->parentDevice) IUnknown_Release(This->parentDevice);
        HeapFree(GetProcessHeap(), 0, This);
    }
    return ref;
}

/* IDirect3DSwapChain8 parts follow: */
static HRESULT WINAPI IDirect3DSwapChain8Impl_Present(LPDIRECT3DSWAPCHAIN8 iface, CONST RECT *pSourceRect, CONST RECT *pDestRect, HWND hDestWindowOverride, CONST RGNDATA *pDirtyRegion) {
    IDirect3DSwapChain8Impl *This = (IDirect3DSwapChain8Impl *)iface;
    HRESULT hr;

    TRACE("iface %p, src_rect %p, dst_rect %p, dst_window_override %p, dirty_region %p.\n",
            iface, pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion);

    wined3d_mutex_lock();
    hr = IWineD3DSwapChain_Present(This->wineD3DSwapChain, pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion, 0);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DSwapChain8Impl_GetBackBuffer(IDirect3DSwapChain8 *iface,
        UINT iBackBuffer, D3DBACKBUFFER_TYPE Type, IDirect3DSurface8 **ppBackBuffer)
{
    IDirect3DSwapChain8Impl *This = (IDirect3DSwapChain8Impl *)iface;
    IWineD3DSurface *mySurface = NULL;
    HRESULT hr;

    TRACE("iface %p, backbuffer_idx %u, backbuffer_type %#x, backbuffer %p.\n",
            iface, iBackBuffer, Type, ppBackBuffer);

    wined3d_mutex_lock();
    hr = IWineD3DSwapChain_GetBackBuffer(This->wineD3DSwapChain, iBackBuffer,
            (WINED3DBACKBUFFER_TYPE)Type, &mySurface);
    if (SUCCEEDED(hr) && mySurface)
    {
        *ppBackBuffer = IWineD3DSurface_GetParent(mySurface);
        IDirect3DSurface8_AddRef(*ppBackBuffer);
        IWineD3DSurface_Release(mySurface);
    }
    wined3d_mutex_unlock();

    return hr;
}

static const IDirect3DSwapChain8Vtbl Direct3DSwapChain8_Vtbl =
{
    IDirect3DSwapChain8Impl_QueryInterface,
    IDirect3DSwapChain8Impl_AddRef,
    IDirect3DSwapChain8Impl_Release,
    IDirect3DSwapChain8Impl_Present,
    IDirect3DSwapChain8Impl_GetBackBuffer
};

HRESULT swapchain_init(IDirect3DSwapChain8Impl *swapchain, IDirect3DDevice8Impl *device,
        D3DPRESENT_PARAMETERS *present_parameters)
{
    WINED3DPRESENT_PARAMETERS wined3d_parameters;
    HRESULT hr;

    swapchain->ref = 1;
    swapchain->lpVtbl = &Direct3DSwapChain8_Vtbl;

    wined3d_parameters.BackBufferWidth = present_parameters->BackBufferWidth;
    wined3d_parameters.BackBufferHeight = present_parameters->BackBufferHeight;
    wined3d_parameters.BackBufferFormat = wined3dformat_from_d3dformat(present_parameters->BackBufferFormat);
    wined3d_parameters.BackBufferCount = max(1, present_parameters->BackBufferCount);
    wined3d_parameters.MultiSampleType = present_parameters->MultiSampleType;
    wined3d_parameters.MultiSampleQuality = 0; /* d3d9 only */
    wined3d_parameters.SwapEffect = present_parameters->SwapEffect;
    wined3d_parameters.hDeviceWindow = present_parameters->hDeviceWindow;
    wined3d_parameters.Windowed = present_parameters->Windowed;
    wined3d_parameters.EnableAutoDepthStencil = present_parameters->EnableAutoDepthStencil;
    wined3d_parameters.AutoDepthStencilFormat = wined3dformat_from_d3dformat(present_parameters->AutoDepthStencilFormat);
    wined3d_parameters.Flags = present_parameters->Flags;
    wined3d_parameters.FullScreen_RefreshRateInHz = present_parameters->FullScreen_RefreshRateInHz;
    wined3d_parameters.PresentationInterval = present_parameters->FullScreen_PresentationInterval;
    wined3d_parameters.AutoRestoreDisplayMode = TRUE;

    wined3d_mutex_lock();
    hr = IWineD3DDevice_CreateSwapChain(device->WineD3DDevice, &wined3d_parameters,
            SURFACE_OPENGL, swapchain, &swapchain->wineD3DSwapChain);
    wined3d_mutex_unlock();

    present_parameters->BackBufferWidth = wined3d_parameters.BackBufferWidth;
    present_parameters->BackBufferHeight = wined3d_parameters.BackBufferHeight;
    present_parameters->BackBufferFormat = d3dformat_from_wined3dformat(wined3d_parameters.BackBufferFormat);
    present_parameters->BackBufferCount = wined3d_parameters.BackBufferCount;
    present_parameters->MultiSampleType = wined3d_parameters.MultiSampleType;
    present_parameters->SwapEffect = wined3d_parameters.SwapEffect;
    present_parameters->hDeviceWindow = wined3d_parameters.hDeviceWindow;
    present_parameters->Windowed = wined3d_parameters.Windowed;
    present_parameters->EnableAutoDepthStencil = wined3d_parameters.EnableAutoDepthStencil;
    present_parameters->AutoDepthStencilFormat = d3dformat_from_wined3dformat(wined3d_parameters.AutoDepthStencilFormat);
    present_parameters->Flags = wined3d_parameters.Flags;
    present_parameters->FullScreen_RefreshRateInHz = wined3d_parameters.FullScreen_RefreshRateInHz;
    present_parameters->FullScreen_PresentationInterval = wined3d_parameters.PresentationInterval;

    if (FAILED(hr))
    {
        WARN("Failed to create wined3d swapchain, hr %#x.\n", hr);
        return hr;
    }

    swapchain->parentDevice = (IDirect3DDevice8 *)device;
    IDirect3DDevice8_AddRef(swapchain->parentDevice);

    return D3D_OK;
}
