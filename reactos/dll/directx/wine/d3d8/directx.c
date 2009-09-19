/*
 * IDirect3D8 implementation
 *
 * Copyright 2002-2004 Jason Edmeades
 * Copyright 2003-2004 Raphael Junqueira
 * Copyright 2004 Christian Costa
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

#include <stdarg.h>

#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "wine/debug.h"
#include "wine/unicode.h"

#include "d3d8_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d8);

/* IDirect3D IUnknown parts follow: */
static HRESULT WINAPI IDirect3D8Impl_QueryInterface(LPDIRECT3D8 iface, REFIID riid,LPVOID *ppobj)
{
    IDirect3D8Impl *This = (IDirect3D8Impl *)iface;

    if (IsEqualGUID(riid, &IID_IUnknown)
        || IsEqualGUID(riid, &IID_IDirect3D8)) {
        IUnknown_AddRef(iface);
        *ppobj = This;
        return S_OK;
    }

    WARN("(%p)->(%s,%p),not found\n", This, debugstr_guid(riid),ppobj);
    *ppobj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI IDirect3D8Impl_AddRef(LPDIRECT3D8 iface) {
    IDirect3D8Impl *This = (IDirect3D8Impl *)iface;
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) : AddRef from %d\n", This, ref - 1);

    return ref;
}

static ULONG WINAPI IDirect3D8Impl_Release(LPDIRECT3D8 iface) {
    IDirect3D8Impl *This = (IDirect3D8Impl *)iface;
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) : ReleaseRef to %d\n", This, ref);

    if (ref == 0) {
        TRACE("Releasing wined3d %p\n", This->WineD3D);

        wined3d_mutex_lock();
        IWineD3D_Release(This->WineD3D);
        wined3d_mutex_unlock();

        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}

/* IDirect3D8 Interface follow: */
static HRESULT WINAPI IDirect3D8Impl_RegisterSoftwareDevice (LPDIRECT3D8 iface, void* pInitializeFunction) {
    IDirect3D8Impl *This = (IDirect3D8Impl *)iface;
    HRESULT hr;
    TRACE("(%p)->(%p)\n", This, pInitializeFunction);

    wined3d_mutex_lock();
    hr = IWineD3D_RegisterSoftwareDevice(This->WineD3D, pInitializeFunction);
    wined3d_mutex_unlock();

    return hr;
}

static UINT WINAPI IDirect3D8Impl_GetAdapterCount (LPDIRECT3D8 iface) {
    IDirect3D8Impl *This = (IDirect3D8Impl *)iface;
    HRESULT hr;
    TRACE("(%p)\n", This);

    wined3d_mutex_lock();
    hr = IWineD3D_GetAdapterCount(This->WineD3D);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3D8Impl_GetAdapterIdentifier(LPDIRECT3D8 iface,
        UINT Adapter, DWORD Flags, D3DADAPTER_IDENTIFIER8 *pIdentifier)
{
    IDirect3D8Impl *This = (IDirect3D8Impl *)iface;
    WINED3DADAPTER_IDENTIFIER adapter_id;
    HRESULT hr;

    TRACE("(%p)->(%d,%08x, %p\n", This, Adapter, Flags, pIdentifier);

    adapter_id.driver = pIdentifier->Driver;
    adapter_id.driver_size = sizeof(pIdentifier->Driver);
    adapter_id.description = pIdentifier->Description;
    adapter_id.description_size = sizeof(pIdentifier->Description);
    adapter_id.device_name = NULL; /* d3d9 only */
    adapter_id.device_name_size = 0; /* d3d9 only */

    wined3d_mutex_lock();
    hr = IWineD3D_GetAdapterIdentifier(This->WineD3D, Adapter, Flags, &adapter_id);
    wined3d_mutex_unlock();

    pIdentifier->DriverVersion = adapter_id.driver_version;
    pIdentifier->VendorId = adapter_id.vendor_id;
    pIdentifier->DeviceId = adapter_id.device_id;
    pIdentifier->SubSysId = adapter_id.subsystem_id;
    pIdentifier->Revision = adapter_id.revision;
    memcpy(&pIdentifier->DeviceIdentifier, &adapter_id.device_identifier, sizeof(pIdentifier->DeviceIdentifier));
    pIdentifier->WHQLLevel = adapter_id.whql_level;

    return hr;
}

static UINT WINAPI IDirect3D8Impl_GetAdapterModeCount (LPDIRECT3D8 iface,UINT Adapter) {
    IDirect3D8Impl *This = (IDirect3D8Impl *)iface;
    HRESULT hr;
    TRACE("(%p)->(%d)\n", This, Adapter);

    wined3d_mutex_lock();
    hr = IWineD3D_GetAdapterModeCount(This->WineD3D, Adapter, 0 /* format */);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3D8Impl_EnumAdapterModes (LPDIRECT3D8 iface, UINT Adapter, UINT Mode, D3DDISPLAYMODE* pMode) {
    IDirect3D8Impl *This = (IDirect3D8Impl *)iface;
    HRESULT hr;
    TRACE("(%p)->(%d, %d, %p)\n", This, Adapter, Mode, pMode);

    wined3d_mutex_lock();
    hr = IWineD3D_EnumAdapterModes(This->WineD3D, Adapter, WINED3DFMT_UNKNOWN, Mode, (WINED3DDISPLAYMODE *) pMode);
    wined3d_mutex_unlock();

    if (SUCCEEDED(hr)) pMode->Format = d3dformat_from_wined3dformat(pMode->Format);

    return hr;
}

static HRESULT WINAPI IDirect3D8Impl_GetAdapterDisplayMode (LPDIRECT3D8 iface, UINT Adapter, D3DDISPLAYMODE* pMode) {
    IDirect3D8Impl *This = (IDirect3D8Impl *)iface;
    HRESULT hr;
    TRACE("(%p)->(%d,%p)\n", This, Adapter, pMode);

    wined3d_mutex_lock();
    hr = IWineD3D_GetAdapterDisplayMode(This->WineD3D, Adapter, (WINED3DDISPLAYMODE *) pMode);
    wined3d_mutex_unlock();

    if (SUCCEEDED(hr)) pMode->Format = d3dformat_from_wined3dformat(pMode->Format);

    return hr;
}

static HRESULT  WINAPI  IDirect3D8Impl_CheckDeviceType            (LPDIRECT3D8 iface,
                                                            UINT Adapter, D3DDEVTYPE CheckType, D3DFORMAT DisplayFormat,
                                                            D3DFORMAT BackBufferFormat, BOOL Windowed) {
    IDirect3D8Impl *This = (IDirect3D8Impl *)iface;
    HRESULT hr;
    TRACE("(%p)->(%d, %d, %d, %d, %s)\n", This, Adapter, CheckType, DisplayFormat, BackBufferFormat, Windowed ? "true" : "false");

    wined3d_mutex_lock();
    hr = IWineD3D_CheckDeviceType(This->WineD3D, Adapter, CheckType, wined3dformat_from_d3dformat(DisplayFormat),
            wined3dformat_from_d3dformat(BackBufferFormat), Windowed);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT  WINAPI  IDirect3D8Impl_CheckDeviceFormat          (LPDIRECT3D8 iface,
                                                            UINT Adapter, D3DDEVTYPE DeviceType, D3DFORMAT AdapterFormat,
                                                            DWORD Usage, D3DRESOURCETYPE RType, D3DFORMAT CheckFormat) {
    IDirect3D8Impl *This = (IDirect3D8Impl *)iface;
    HRESULT hr;
    WINED3DRESOURCETYPE WineD3DRType;
    TRACE("(%p)->(%d, %d, %d, %08x, %d, %d)\n", This, Adapter, DeviceType, AdapterFormat, Usage, RType, CheckFormat);

    if(CheckFormat == D3DFMT_R8G8B8)
    {
        /* See comment in dlls/d3d9/directx.c, IDirect3D9Impl_CheckDeviceFormat for details */
        WARN("D3DFMT_R8G8B8 is not available on windows, returning D3DERR_NOTAVAILABLE\n");
        return D3DERR_NOTAVAILABLE;
    }


    switch(RType) {
        case D3DRTYPE_VERTEXBUFFER:
        case D3DRTYPE_INDEXBUFFER:
            WineD3DRType = WINED3DRTYPE_BUFFER;
            break;

        default:
            WineD3DRType = RType;
            break;
    }

    wined3d_mutex_lock();
    hr = IWineD3D_CheckDeviceFormat(This->WineD3D, Adapter, DeviceType, wined3dformat_from_d3dformat(AdapterFormat),
            Usage, WineD3DRType, wined3dformat_from_d3dformat(CheckFormat), SURFACE_OPENGL);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3D8Impl_CheckDeviceMultiSampleType(IDirect3D8 *iface, UINT Adapter,
        D3DDEVTYPE DeviceType, D3DFORMAT SurfaceFormat, BOOL Windowed, D3DMULTISAMPLE_TYPE MultiSampleType)
{
    IDirect3D8Impl *This = (IDirect3D8Impl *)iface;
    HRESULT hr;
    TRACE("(%p)-<(%d, %d, %d, %s, %d)\n", This, Adapter, DeviceType, SurfaceFormat, Windowed ? "true" : "false", MultiSampleType);

    wined3d_mutex_lock();
    hr = IWineD3D_CheckDeviceMultiSampleType(This->WineD3D, Adapter, DeviceType,
            wined3dformat_from_d3dformat(SurfaceFormat), Windowed, (WINED3DMULTISAMPLE_TYPE) MultiSampleType, NULL);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3D8Impl_CheckDepthStencilMatch(IDirect3D8 *iface, UINT Adapter, D3DDEVTYPE DeviceType,
        D3DFORMAT AdapterFormat, D3DFORMAT RenderTargetFormat, D3DFORMAT DepthStencilFormat)
{
    IDirect3D8Impl *This = (IDirect3D8Impl *)iface;
    HRESULT hr;
    TRACE("(%p)-<(%d, %d, %d, %d, %d)\n", This, Adapter, DeviceType, AdapterFormat, RenderTargetFormat, DepthStencilFormat);

    wined3d_mutex_lock();
    hr = IWineD3D_CheckDepthStencilMatch(This->WineD3D, Adapter, DeviceType,
            wined3dformat_from_d3dformat(AdapterFormat), wined3dformat_from_d3dformat(RenderTargetFormat),
            wined3dformat_from_d3dformat(DepthStencilFormat));
    wined3d_mutex_unlock();

    return hr;
}

void fixup_caps(WINED3DCAPS *caps)
{
    /* D3D8 doesn't support SM 2.0 or higher, so clamp to 1.x */
    if (caps->PixelShaderVersion > D3DPS_VERSION(1,4)) {
        caps->PixelShaderVersion = D3DPS_VERSION(1,4);
    }
    if (caps->VertexShaderVersion > D3DVS_VERSION(1,1)) {
        caps->VertexShaderVersion = D3DVS_VERSION(1,1);
    }
    caps->MaxVertexShaderConst = min(D3D8_MAX_VERTEX_SHADER_CONSTANTF, caps->MaxVertexShaderConst);

    caps->StencilCaps &= ~WINED3DSTENCILCAPS_TWOSIDED;
}

static HRESULT  WINAPI  IDirect3D8Impl_GetDeviceCaps(LPDIRECT3D8 iface, UINT Adapter, D3DDEVTYPE DeviceType, D3DCAPS8* pCaps) {
    IDirect3D8Impl *This = (IDirect3D8Impl *)iface;
    HRESULT hrc = D3D_OK;
    WINED3DCAPS *pWineCaps;

    TRACE("(%p) Relay %d %u %p\n", This, Adapter, DeviceType, pCaps);

    if(NULL == pCaps){
        return D3DERR_INVALIDCALL;
    }
    pWineCaps = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(WINED3DCAPS));
    if(pWineCaps == NULL){
        return D3DERR_INVALIDCALL; /*well this is what MSDN says to return*/
    }

    wined3d_mutex_lock();
    hrc = IWineD3D_GetDeviceCaps(This->WineD3D, Adapter, DeviceType, pWineCaps);
    wined3d_mutex_unlock();

    fixup_caps(pWineCaps);
    WINECAPSTOD3D8CAPS(pCaps, pWineCaps)
    HeapFree(GetProcessHeap(), 0, pWineCaps);

    TRACE("(%p) returning %p\n", This, pCaps);
    return hrc;
}

static HMONITOR WINAPI  IDirect3D8Impl_GetAdapterMonitor(LPDIRECT3D8 iface, UINT Adapter) {
    IDirect3D8Impl *This = (IDirect3D8Impl *)iface;
    HMONITOR ret;
    TRACE("(%p)->(%d)\n", This, Adapter);

    wined3d_mutex_lock();
    ret = IWineD3D_GetAdapterMonitor(This->WineD3D, Adapter);
    wined3d_mutex_unlock();

    return ret;
}

ULONG WINAPI D3D8CB_DestroyRenderTarget(IWineD3DSurface *pSurface) {
    IDirect3DSurface8Impl* surfaceParent;
    TRACE("(%p) call back\n", pSurface);

    IWineD3DSurface_GetParent(pSurface, (IUnknown **) &surfaceParent);
    surfaceParent->isImplicit = FALSE;
    /* Surface had refcount of 0 GetParent addrefed to 1, so 1 Release is enough */
    return IDirect3DSurface8_Release((IDirect3DSurface8*) surfaceParent);
}

ULONG WINAPI D3D8CB_DestroySwapChain(IWineD3DSwapChain *pSwapChain) {
    IUnknown* swapChainParent;
    TRACE("(%p) call back\n", pSwapChain);

    IWineD3DSwapChain_GetParent(pSwapChain, &swapChainParent);
    IUnknown_Release(swapChainParent);
    return IUnknown_Release(swapChainParent);
}

ULONG WINAPI D3D8CB_DestroyDepthStencilSurface(IWineD3DSurface *pSurface) {
    IDirect3DSurface8Impl* surfaceParent;
    TRACE("(%p) call back\n", pSurface);

    IWineD3DSurface_GetParent(pSurface, (IUnknown **) &surfaceParent);
    surfaceParent->isImplicit = FALSE;
    /* Surface had refcount of 0 GetParent addrefed to 1, so 1 Release is enough */
    return IDirect3DSurface8_Release((IDirect3DSurface8*) surfaceParent);
}

static HRESULT WINAPI IDirect3D8Impl_CreateDevice(LPDIRECT3D8 iface, UINT Adapter, D3DDEVTYPE DeviceType, HWND hFocusWindow,
                                            DWORD BehaviourFlags, D3DPRESENT_PARAMETERS* pPresentationParameters,
                                            IDirect3DDevice8** ppReturnedDeviceInterface) {

    IDirect3D8Impl       *This   = (IDirect3D8Impl *)iface;
    IDirect3DDevice8Impl *object = NULL;
    WINED3DPRESENT_PARAMETERS localParameters;
    HRESULT hr;
    TRACE("(%p) Relay\n", This);

    /* Check the validity range of the adapter parameter */
    if (Adapter >= IDirect3D8Impl_GetAdapterCount(iface)) {
        *ppReturnedDeviceInterface = NULL;
        return D3DERR_INVALIDCALL;
    }

    /* Allocate the storage for the device object */
    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirect3DDevice8Impl));
    if (NULL == object) {
        FIXME("Allocation of memory failed\n");
        *ppReturnedDeviceInterface = NULL;
        return D3DERR_OUTOFVIDEOMEMORY;
    }

    object->lpVtbl = &Direct3DDevice8_Vtbl;
    object->device_parent_vtbl = &d3d8_wined3d_device_parent_vtbl;
    object->ref = 1;
    object->handle_table.entries = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
            D3D8_INITIAL_HANDLE_TABLE_SIZE * sizeof(*object->handle_table.entries));
    object->handle_table.table_size = D3D8_INITIAL_HANDLE_TABLE_SIZE;
    *ppReturnedDeviceInterface = (IDirect3DDevice8 *)object;

    /* Allocate an associated WineD3DDevice object */
    wined3d_mutex_lock();
    hr = IWineD3D_CreateDevice(This->WineD3D, Adapter, DeviceType, hFocusWindow, BehaviourFlags,
            (IUnknown *)object, (IWineD3DDeviceParent *)&object->device_parent_vtbl, &object->WineD3DDevice);

    if (hr != D3D_OK) {
        HeapFree(GetProcessHeap(), 0, object);
        *ppReturnedDeviceInterface = NULL;
        wined3d_mutex_unlock();

        return hr;
    }

    TRACE("(%p) : Created Device %p\n", This, object);

    localParameters.BackBufferWidth                             = pPresentationParameters->BackBufferWidth;
    localParameters.BackBufferHeight                            = pPresentationParameters->BackBufferHeight;
    localParameters.BackBufferFormat                            = wined3dformat_from_d3dformat(pPresentationParameters->BackBufferFormat);
    localParameters.BackBufferCount                             = pPresentationParameters->BackBufferCount;
    localParameters.MultiSampleType                             = pPresentationParameters->MultiSampleType;
    localParameters.MultiSampleQuality                          = 0; /* d3d9 only */
    localParameters.SwapEffect                                  = pPresentationParameters->SwapEffect;
    localParameters.hDeviceWindow                               = pPresentationParameters->hDeviceWindow;
    localParameters.Windowed                                    = pPresentationParameters->Windowed;
    localParameters.EnableAutoDepthStencil                      = pPresentationParameters->EnableAutoDepthStencil;
    localParameters.AutoDepthStencilFormat                      = wined3dformat_from_d3dformat(pPresentationParameters->AutoDepthStencilFormat);
    localParameters.Flags                                       = pPresentationParameters->Flags;
    localParameters.FullScreen_RefreshRateInHz                  = pPresentationParameters->FullScreen_RefreshRateInHz;
    localParameters.PresentationInterval                        = pPresentationParameters->FullScreen_PresentationInterval;
    localParameters.AutoRestoreDisplayMode                      = TRUE;

    if(BehaviourFlags & D3DCREATE_MULTITHREADED) {
        IWineD3DDevice_SetMultithreaded(object->WineD3DDevice);
    }

    hr = IWineD3DDevice_Init3D(object->WineD3DDevice, &localParameters);
    wined3d_mutex_unlock();

    pPresentationParameters->BackBufferWidth                    = localParameters.BackBufferWidth;
    pPresentationParameters->BackBufferHeight                   = localParameters.BackBufferHeight;
    pPresentationParameters->BackBufferFormat                   = d3dformat_from_wined3dformat(localParameters.BackBufferFormat);
    pPresentationParameters->BackBufferCount                    = localParameters.BackBufferCount;
    pPresentationParameters->MultiSampleType                    = localParameters.MultiSampleType;
    pPresentationParameters->SwapEffect                         = localParameters.SwapEffect;
    pPresentationParameters->hDeviceWindow                      = localParameters.hDeviceWindow;
    pPresentationParameters->Windowed                           = localParameters.Windowed;
    pPresentationParameters->EnableAutoDepthStencil             = localParameters.EnableAutoDepthStencil;
    pPresentationParameters->AutoDepthStencilFormat             = d3dformat_from_wined3dformat(localParameters.AutoDepthStencilFormat);
    pPresentationParameters->Flags                              = localParameters.Flags;
    pPresentationParameters->FullScreen_RefreshRateInHz         = localParameters.FullScreen_RefreshRateInHz;
    pPresentationParameters->FullScreen_PresentationInterval    = localParameters.PresentationInterval;

    if (hr != D3D_OK) {
        FIXME("(%p) D3D Initialization failed for WineD3DDevice %p\n", This, object->WineD3DDevice);
        HeapFree(GetProcessHeap(), 0, object);
        *ppReturnedDeviceInterface = NULL;
    }

    object->declArraySize = 16;
    object->decls = HeapAlloc(GetProcessHeap(), 0, object->declArraySize * sizeof(*object->decls));
    if(!object->decls) {
        ERR("Out of memory\n");

        wined3d_mutex_lock();
        IWineD3DDevice_Release(object->WineD3DDevice);
        wined3d_mutex_unlock();

        HeapFree(GetProcessHeap(), 0, object);
        *ppReturnedDeviceInterface = NULL;
        hr = E_OUTOFMEMORY;
    }
    return hr;
}

const IDirect3D8Vtbl Direct3D8_Vtbl =
{
    /* IUnknown */
    IDirect3D8Impl_QueryInterface,
    IDirect3D8Impl_AddRef,
    IDirect3D8Impl_Release,
    /* IDirect3D8 */
    IDirect3D8Impl_RegisterSoftwareDevice,
    IDirect3D8Impl_GetAdapterCount,
    IDirect3D8Impl_GetAdapterIdentifier,
    IDirect3D8Impl_GetAdapterModeCount,
    IDirect3D8Impl_EnumAdapterModes,
    IDirect3D8Impl_GetAdapterDisplayMode,
    IDirect3D8Impl_CheckDeviceType,
    IDirect3D8Impl_CheckDeviceFormat,
    IDirect3D8Impl_CheckDeviceMultiSampleType,
    IDirect3D8Impl_CheckDepthStencilMatch,
    IDirect3D8Impl_GetDeviceCaps,
    IDirect3D8Impl_GetAdapterMonitor,
    IDirect3D8Impl_CreateDevice
};
