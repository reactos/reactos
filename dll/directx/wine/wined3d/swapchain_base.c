/*
 *IDirect3DSwapChain9 implementation
 *
 *Copyright 2002-2003 Jason Edmeades
 *Copyright 2002-2003 Raphael Junqueira
 *Copyright 2005 Oliver Stieber
 *Copyright 2007-2008 Stefan Dösinger for CodeWeavers
 *
 *This library is free software; you can redistribute it and/or
 *modify it under the terms of the GNU Lesser General Public
 *License as published by the Free Software Foundation; either
 *version 2.1 of the License, or (at your option) any later version.
 *
 *This library is distributed in the hope that it will be useful,
 *but WITHOUT ANY WARRANTY; without even the implied warranty of
 *MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *Lesser General Public License for more details.
 *
 *You should have received a copy of the GNU Lesser General Public
 *License along with this library; if not, write to the Free Software
 *Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "config.h"
#include "wined3d_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d);

/* IDirect3DSwapChain IUnknown parts follow: */
HRESULT WINAPI IWineD3DBaseSwapChainImpl_QueryInterface(IWineD3DSwapChain *iface, REFIID riid, void **object)
{
    TRACE("iface %p, riid %s, object %p.\n", iface, debugstr_guid(riid), object);

    if (IsEqualGUID(riid, &IID_IWineD3DSwapChain)
            || IsEqualGUID(riid, &IID_IWineD3DBase)
            || IsEqualGUID(riid, &IID_IUnknown))
    {
        IUnknown_AddRef(iface);
        *object = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(riid));

    *object = NULL;
    return E_NOINTERFACE;
}

ULONG WINAPI IWineD3DBaseSwapChainImpl_AddRef(IWineD3DSwapChain *iface) {
    IWineD3DSwapChainImpl *This = (IWineD3DSwapChainImpl *)iface;
    DWORD refCount = InterlockedIncrement(&This->ref);
    TRACE("(%p) : AddRef increasing from %d\n", This, refCount - 1);
    return refCount;
}

/* Do not call while under the GL lock. */
ULONG WINAPI IWineD3DBaseSwapChainImpl_Release(IWineD3DSwapChain *iface)
{
    IWineD3DSwapChainImpl *This = (IWineD3DSwapChainImpl *)iface;
    DWORD refCount;
    refCount = InterlockedDecrement(&This->ref);
    TRACE("(%p) : ReleaseRef to %d\n", This, refCount);

    if (!refCount) IWineD3DSwapChain_Destroy(iface);

    return refCount;
}

void * WINAPI IWineD3DBaseSwapChainImpl_GetParent(IWineD3DSwapChain *iface)
{
    TRACE("iface %p.\n", iface);

    return ((IWineD3DSwapChainImpl *)iface)->parent;
}

HRESULT WINAPI IWineD3DBaseSwapChainImpl_GetFrontBufferData(IWineD3DSwapChain *iface, IWineD3DSurface *dst_surface)
{
    IWineD3DSwapChainImpl *This = (IWineD3DSwapChainImpl *)iface;
    POINT start;

    TRACE("iface %p, dst_surface %p.\n", iface, dst_surface);

    start.x = 0;
    start.y = 0;

    if (This->presentParms.Windowed) {
        MapWindowPoints(This->win_handle, NULL, &start, 1);
    }

    IWineD3DSurface_BltFast(dst_surface, start.x, start.y, (IWineD3DSurface *)This->front_buffer, NULL, 0);
    return WINED3D_OK;
}

HRESULT WINAPI IWineD3DBaseSwapChainImpl_GetBackBuffer(IWineD3DSwapChain *iface,
        UINT back_buffer_idx, WINED3DBACKBUFFER_TYPE type, IWineD3DSurface **back_buffer)
{
    IWineD3DSwapChainImpl *swapchain = (IWineD3DSwapChainImpl *)iface;

    TRACE("iface %p, back_buffer_idx %u, type %#x, back_buffer %p.\n",
            iface, back_buffer_idx, type, back_buffer);

    /* Return invalid if there is no backbuffer array, otherwise it will
     * crash when ddraw is used (there swapchain->back_buffers is always NULL).
     * We need this because this function is called from
     * IWineD3DStateBlockImpl_InitStartupStateBlock() to get the default
     * scissorrect dimensions. */
    if (!swapchain->back_buffers || back_buffer_idx >= swapchain->presentParms.BackBufferCount)
    {
        WARN("Invalid back buffer index.\n");
        /* Native d3d9 doesn't set NULL here, just as wine's d3d9. But set it
         * here in wined3d to avoid problems in other libs. */
        *back_buffer = NULL;
        return WINED3DERR_INVALIDCALL;
    }

    *back_buffer = (IWineD3DSurface *)swapchain->back_buffers[back_buffer_idx];
    if (*back_buffer) IWineD3DSurface_AddRef(*back_buffer);

    TRACE("Returning back buffer %p.\n", *back_buffer);

    return WINED3D_OK;
}

HRESULT WINAPI IWineD3DBaseSwapChainImpl_GetRasterStatus(IWineD3DSwapChain *iface, WINED3DRASTER_STATUS *pRasterStatus) {
    static BOOL warned;
    /* No OpenGL equivalent */
    if (!warned)
    {
        FIXME("iface %p, raster_status %p stub!\n", iface, pRasterStatus);
        warned = TRUE;
    }
    /* Obtaining the raster status is a widely implemented but optional feature.
     * When this method returns OK then the application Starcraft 2 expects that
     * the pRasterStatus->InVBlank value differs over time. To prevent Starcraft 2
     * from running in an infinite loop at startup this method returns INVALIDCALL.
     */
    return WINED3DERR_INVALIDCALL;
}

HRESULT WINAPI IWineD3DBaseSwapChainImpl_GetDisplayMode(IWineD3DSwapChain *iface, WINED3DDISPLAYMODE*pMode) {
    IWineD3DSwapChainImpl *This = (IWineD3DSwapChainImpl *)iface;
    HRESULT hr;

    TRACE("(%p)->(%p): Calling GetAdapterDisplayMode\n", This, pMode);
    hr = IWineD3D_GetAdapterDisplayMode(This->device->wined3d, This->device->adapter->ordinal, pMode);

    TRACE("(%p) : returning w(%d) h(%d) rr(%d) fmt(%u,%s)\n", This, pMode->Width, pMode->Height, pMode->RefreshRate,
          pMode->Format, debug_d3dformat(pMode->Format));
    return hr;
}

HRESULT WINAPI IWineD3DBaseSwapChainImpl_GetDevice(IWineD3DSwapChain *iface, IWineD3DDevice **device)
{
    IWineD3DSwapChainImpl *This = (IWineD3DSwapChainImpl *)iface;

    *device = (IWineD3DDevice *)This->device;
    IWineD3DDevice_AddRef(*device);

    TRACE("(%p) : returning %p\n", This, *device);
    return WINED3D_OK;
}

HRESULT WINAPI IWineD3DBaseSwapChainImpl_GetPresentParameters(IWineD3DSwapChain *iface, WINED3DPRESENT_PARAMETERS *pPresentationParameters) {
    IWineD3DSwapChainImpl *This = (IWineD3DSwapChainImpl *)iface;
    TRACE("(%p)\n", This);

    *pPresentationParameters = This->presentParms;

    return WINED3D_OK;
}

HRESULT WINAPI IWineD3DBaseSwapChainImpl_SetGammaRamp(IWineD3DSwapChain *iface, DWORD Flags, CONST WINED3DGAMMARAMP *pRamp){

    IWineD3DSwapChainImpl *This = (IWineD3DSwapChainImpl *)iface;
    HDC hDC;
    TRACE("(%p) : pRamp@%p flags(%d)\n", This, pRamp, Flags);
    hDC = GetDC(This->device_window);
    SetDeviceGammaRamp(hDC, (LPVOID)pRamp);
    ReleaseDC(This->device_window, hDC);
    return WINED3D_OK;

}

HRESULT WINAPI IWineD3DBaseSwapChainImpl_GetGammaRamp(IWineD3DSwapChain *iface, WINED3DGAMMARAMP *pRamp){

    IWineD3DSwapChainImpl *This = (IWineD3DSwapChainImpl *)iface;
    HDC hDC;
    TRACE("(%p) : pRamp@%p\n", This, pRamp);
    hDC = GetDC(This->device_window);
    GetDeviceGammaRamp(hDC, pRamp);
    ReleaseDC(This->device_window, hDC);
    return WINED3D_OK;

}
