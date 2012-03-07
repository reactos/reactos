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

static inline IDirect3D8Impl *impl_from_IDirect3D8(IDirect3D8 *iface)
{
    return CONTAINING_RECORD(iface, IDirect3D8Impl, IDirect3D8_iface);
}

static HRESULT WINAPI IDirect3D8Impl_QueryInterface(LPDIRECT3D8 iface, REFIID riid,LPVOID *ppobj)
{
    IDirect3D8Impl *This = impl_from_IDirect3D8(iface);

    TRACE("iface %p, riid %s, object %p.\n", iface, debugstr_guid(riid), ppobj);

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

static ULONG WINAPI IDirect3D8Impl_AddRef(LPDIRECT3D8 iface)
{
    IDirect3D8Impl *This = impl_from_IDirect3D8(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("%p increasing refcount to %u.\n", iface, ref);

    return ref;
}

static ULONG WINAPI IDirect3D8Impl_Release(LPDIRECT3D8 iface)
{
    IDirect3D8Impl *This = impl_from_IDirect3D8(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("%p decreasing refcount to %u.\n", iface, ref);

    if (ref == 0) {
        TRACE("Releasing wined3d %p\n", This->WineD3D);

        wined3d_mutex_lock();
        wined3d_decref(This->WineD3D);
        wined3d_mutex_unlock();

        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}

static HRESULT WINAPI IDirect3D8Impl_RegisterSoftwareDevice(LPDIRECT3D8 iface,
        void* pInitializeFunction)
{
    IDirect3D8Impl *This = impl_from_IDirect3D8(iface);
    HRESULT hr;

    TRACE("iface %p, init_function %p.\n", iface, pInitializeFunction);

    wined3d_mutex_lock();
    hr = wined3d_register_software_device(This->WineD3D, pInitializeFunction);
    wined3d_mutex_unlock();

    return hr;
}

static UINT WINAPI IDirect3D8Impl_GetAdapterCount(LPDIRECT3D8 iface)
{
    IDirect3D8Impl *This = impl_from_IDirect3D8(iface);
    HRESULT hr;

    TRACE("iface %p.\n", iface);

    wined3d_mutex_lock();
    hr = wined3d_get_adapter_count(This->WineD3D);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3D8Impl_GetAdapterIdentifier(LPDIRECT3D8 iface, UINT Adapter,
        DWORD Flags, D3DADAPTER_IDENTIFIER8 *pIdentifier)
{
    IDirect3D8Impl *This = impl_from_IDirect3D8(iface);
    struct wined3d_adapter_identifier adapter_id;
    HRESULT hr;

    TRACE("iface %p, adapter %u, flags %#x, identifier %p.\n",
            iface, Adapter, Flags, pIdentifier);

    adapter_id.driver = pIdentifier->Driver;
    adapter_id.driver_size = sizeof(pIdentifier->Driver);
    adapter_id.description = pIdentifier->Description;
    adapter_id.description_size = sizeof(pIdentifier->Description);
    adapter_id.device_name = NULL; /* d3d9 only */
    adapter_id.device_name_size = 0; /* d3d9 only */

    wined3d_mutex_lock();
    hr = wined3d_get_adapter_identifier(This->WineD3D, Adapter, Flags, &adapter_id);
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

static UINT WINAPI IDirect3D8Impl_GetAdapterModeCount(LPDIRECT3D8 iface,UINT Adapter)
{
    IDirect3D8Impl *This = impl_from_IDirect3D8(iface);
    HRESULT hr;

    TRACE("iface %p, adapter %u.\n", iface, Adapter);

    wined3d_mutex_lock();
    hr = wined3d_get_adapter_mode_count(This->WineD3D, Adapter, 0);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3D8Impl_EnumAdapterModes(LPDIRECT3D8 iface, UINT Adapter, UINT Mode,
        D3DDISPLAYMODE* pMode)
{
    IDirect3D8Impl *This = impl_from_IDirect3D8(iface);
    HRESULT hr;

    TRACE("iface %p, adapter %u, mode_idx %u, mode %p.\n",
            iface, Adapter, Mode, pMode);

    wined3d_mutex_lock();
    hr = wined3d_enum_adapter_modes(This->WineD3D, Adapter, WINED3DFMT_UNKNOWN,
            Mode, (struct wined3d_display_mode *)pMode);
    wined3d_mutex_unlock();

    if (SUCCEEDED(hr)) pMode->Format = d3dformat_from_wined3dformat(pMode->Format);

    return hr;
}

static HRESULT WINAPI IDirect3D8Impl_GetAdapterDisplayMode(LPDIRECT3D8 iface, UINT Adapter,
        D3DDISPLAYMODE* pMode)
{
    IDirect3D8Impl *This = impl_from_IDirect3D8(iface);
    HRESULT hr;

    TRACE("iface %p, adapter %u, mode %p.\n",
            iface, Adapter, pMode);

    wined3d_mutex_lock();
    hr = wined3d_get_adapter_display_mode(This->WineD3D, Adapter, (struct wined3d_display_mode *)pMode);
    wined3d_mutex_unlock();

    if (SUCCEEDED(hr)) pMode->Format = d3dformat_from_wined3dformat(pMode->Format);

    return hr;
}

static HRESULT WINAPI IDirect3D8Impl_CheckDeviceType(LPDIRECT3D8 iface, UINT Adapter,
        D3DDEVTYPE CheckType, D3DFORMAT DisplayFormat, D3DFORMAT BackBufferFormat, BOOL Windowed)
{
    IDirect3D8Impl *This = impl_from_IDirect3D8(iface);
    HRESULT hr;

    TRACE("iface %p, adapter %u, device_type %#x, display_format %#x, backbuffer_format %#x, windowed %#x.\n",
            iface, Adapter, CheckType, DisplayFormat, BackBufferFormat, Windowed);

    wined3d_mutex_lock();
    hr = wined3d_check_device_type(This->WineD3D, Adapter, CheckType, wined3dformat_from_d3dformat(DisplayFormat),
            wined3dformat_from_d3dformat(BackBufferFormat), Windowed);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3D8Impl_CheckDeviceFormat(LPDIRECT3D8 iface, UINT Adapter,
        D3DDEVTYPE DeviceType, D3DFORMAT AdapterFormat, DWORD Usage, D3DRESOURCETYPE RType,
        D3DFORMAT CheckFormat)
{
    IDirect3D8Impl *This = impl_from_IDirect3D8(iface);
    enum wined3d_resource_type wined3d_rtype;
    HRESULT hr;

    TRACE("iface %p, adapter %u, device_type %#x, adapter_format %#x, usage %#x, resource_type %#x, format %#x.\n",
            iface, Adapter, DeviceType, AdapterFormat, Usage, RType, CheckFormat);

    switch(RType) {
        case D3DRTYPE_VERTEXBUFFER:
        case D3DRTYPE_INDEXBUFFER:
            wined3d_rtype = WINED3D_RTYPE_BUFFER;
            break;

        default:
            wined3d_rtype = RType;
            break;
    }

    wined3d_mutex_lock();
    hr = wined3d_check_device_format(This->WineD3D, Adapter, DeviceType, wined3dformat_from_d3dformat(AdapterFormat),
            Usage, wined3d_rtype, wined3dformat_from_d3dformat(CheckFormat), SURFACE_OPENGL);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3D8Impl_CheckDeviceMultiSampleType(IDirect3D8 *iface, UINT Adapter,
        D3DDEVTYPE DeviceType, D3DFORMAT SurfaceFormat, BOOL Windowed,
        D3DMULTISAMPLE_TYPE MultiSampleType)
{
    IDirect3D8Impl *This = impl_from_IDirect3D8(iface);
    HRESULT hr;

    TRACE("iface %p, adapter %u, device_type %#x, format %#x, windowed %#x, multisample_type %#x.\n",
            iface, Adapter, DeviceType, SurfaceFormat, Windowed, MultiSampleType);

    wined3d_mutex_lock();
    hr = wined3d_check_device_multisample_type(This->WineD3D, Adapter, DeviceType,
            wined3dformat_from_d3dformat(SurfaceFormat), Windowed,
            (enum wined3d_multisample_type)MultiSampleType, NULL);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3D8Impl_CheckDepthStencilMatch(IDirect3D8 *iface, UINT Adapter,
        D3DDEVTYPE DeviceType, D3DFORMAT AdapterFormat, D3DFORMAT RenderTargetFormat,
        D3DFORMAT DepthStencilFormat)
{
    IDirect3D8Impl *This = impl_from_IDirect3D8(iface);
    HRESULT hr;

    TRACE("iface %p, adapter %u, device_type %#x, adapter_format %#x, rt_format %#x, ds_format %#x.\n",
            iface, Adapter, DeviceType, AdapterFormat, RenderTargetFormat, DepthStencilFormat);

    wined3d_mutex_lock();
    hr = wined3d_check_depth_stencil_match(This->WineD3D, Adapter, DeviceType,
            wined3dformat_from_d3dformat(AdapterFormat), wined3dformat_from_d3dformat(RenderTargetFormat),
            wined3dformat_from_d3dformat(DepthStencilFormat));
    wined3d_mutex_unlock();

    return hr;
}

void fixup_caps(WINED3DCAPS *caps)
{
    /* D3D8 doesn't support SM 2.0 or higher, so clamp to 1.x */
    if (caps->PixelShaderVersion)
        caps->PixelShaderVersion = D3DPS_VERSION(1,4);
    else
        caps->PixelShaderVersion = D3DPS_VERSION(0,0);
    if (caps->VertexShaderVersion)
        caps->VertexShaderVersion = D3DVS_VERSION(1,1);
    else
        caps->VertexShaderVersion = D3DVS_VERSION(0,0);
    caps->MaxVertexShaderConst = min(D3D8_MAX_VERTEX_SHADER_CONSTANTF, caps->MaxVertexShaderConst);

    caps->StencilCaps &= ~WINED3DSTENCILCAPS_TWOSIDED;
}

static HRESULT  WINAPI  IDirect3D8Impl_GetDeviceCaps(LPDIRECT3D8 iface, UINT Adapter,
        D3DDEVTYPE DeviceType, D3DCAPS8* pCaps)
{
    IDirect3D8Impl *This = impl_from_IDirect3D8(iface);
    HRESULT hrc = D3D_OK;
    WINED3DCAPS *pWineCaps;

    TRACE("iface %p, adapter %u, device_type %#x, caps %p.\n", iface, Adapter, DeviceType, pCaps);

    if(NULL == pCaps){
        return D3DERR_INVALIDCALL;
    }
    pWineCaps = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(WINED3DCAPS));
    if(pWineCaps == NULL){
        return D3DERR_INVALIDCALL; /*well this is what MSDN says to return*/
    }

    wined3d_mutex_lock();
    hrc = wined3d_get_device_caps(This->WineD3D, Adapter, DeviceType, pWineCaps);
    wined3d_mutex_unlock();

    fixup_caps(pWineCaps);
    WINECAPSTOD3D8CAPS(pCaps, pWineCaps)
    HeapFree(GetProcessHeap(), 0, pWineCaps);

    TRACE("(%p) returning %p\n", This, pCaps);
    return hrc;
}

static HMONITOR WINAPI  IDirect3D8Impl_GetAdapterMonitor(LPDIRECT3D8 iface, UINT Adapter)
{
    IDirect3D8Impl *This = impl_from_IDirect3D8(iface);
    HMONITOR ret;

    TRACE("iface %p, adapter %u.\n", iface, Adapter);

    wined3d_mutex_lock();
    ret = wined3d_get_adapter_monitor(This->WineD3D, Adapter);
    wined3d_mutex_unlock();

    return ret;
}

static HRESULT WINAPI IDirect3D8Impl_CreateDevice(IDirect3D8 *iface, UINT adapter,
        D3DDEVTYPE device_type, HWND focus_window, DWORD flags, D3DPRESENT_PARAMETERS *parameters,
        IDirect3DDevice8 **device)
{
    IDirect3D8Impl *This = impl_from_IDirect3D8(iface);
    IDirect3DDevice8Impl *object;
    HRESULT hr;

    TRACE("iface %p, adapter %u, device_type %#x, focus_window %p, flags %#x, parameters %p, device %p.\n",
            iface, adapter, device_type, focus_window, flags, parameters, device);

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object));
    if (!object)
    {
        ERR("Failed to allocate device memory.\n");
        return E_OUTOFMEMORY;
    }

    hr = device_init(object, This, This->WineD3D, adapter, device_type, focus_window, flags, parameters);
    if (FAILED(hr))
    {
        WARN("Failed to initialize device, hr %#x.\n", hr);
        HeapFree(GetProcessHeap(), 0, object);
        return hr;
    }

    TRACE("Created device %p.\n", object);
    *device = &object->IDirect3DDevice8_iface;

    return D3D_OK;
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
