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

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "wine/debug.h"

#include "d3d8_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d8);

static inline struct d3d8 *impl_from_IDirect3D8(IDirect3D8 *iface)
{
    return CONTAINING_RECORD(iface, struct d3d8, IDirect3D8_iface);
}

static HRESULT WINAPI d3d8_QueryInterface(IDirect3D8 *iface, REFIID riid, void **out)
{
    TRACE("iface %p, riid %s, out %p.\n", iface, debugstr_guid(riid), out);

    if (IsEqualGUID(riid, &IID_IDirect3D8)
            || IsEqualGUID(riid, &IID_IUnknown))
    {
        IDirect3D8_AddRef(iface);
        *out = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(riid));

    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI d3d8_AddRef(IDirect3D8 *iface)
{
    struct d3d8 *d3d8 = impl_from_IDirect3D8(iface);
    ULONG refcount = InterlockedIncrement(&d3d8->refcount);

    TRACE("%p increasing refcount to %u.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI d3d8_Release(IDirect3D8 *iface)
{
    struct d3d8 *d3d8 = impl_from_IDirect3D8(iface);
    ULONG refcount = InterlockedDecrement(&d3d8->refcount);

    TRACE("%p decreasing refcount to %u.\n", iface, refcount);

    if (!refcount)
    {
        wined3d_mutex_lock();
        wined3d_decref(d3d8->wined3d);
        wined3d_mutex_unlock();

        heap_free(d3d8);
    }

    return refcount;
}

static HRESULT WINAPI d3d8_RegisterSoftwareDevice(IDirect3D8 *iface, void *init_function)
{
    struct d3d8 *d3d8 = impl_from_IDirect3D8(iface);
    HRESULT hr;

    TRACE("iface %p, init_function %p.\n", iface, init_function);

    wined3d_mutex_lock();
    hr = wined3d_register_software_device(d3d8->wined3d, init_function);
    wined3d_mutex_unlock();

    return hr;
}

static UINT WINAPI d3d8_GetAdapterCount(IDirect3D8 *iface)
{
    struct d3d8 *d3d8 = impl_from_IDirect3D8(iface);
    HRESULT hr;

    TRACE("iface %p.\n", iface);

    wined3d_mutex_lock();
    hr = wined3d_get_adapter_count(d3d8->wined3d);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI d3d8_GetAdapterIdentifier(IDirect3D8 *iface, UINT adapter,
        DWORD flags, D3DADAPTER_IDENTIFIER8 *identifier)
{
    struct d3d8 *d3d8 = impl_from_IDirect3D8(iface);
    struct wined3d_adapter_identifier adapter_id;
    HRESULT hr;

    TRACE("iface %p, adapter %u, flags %#x, identifier %p.\n",
            iface, adapter, flags, identifier);

    adapter_id.driver = identifier->Driver;
    adapter_id.driver_size = sizeof(identifier->Driver);
    adapter_id.description = identifier->Description;
    adapter_id.description_size = sizeof(identifier->Description);
    adapter_id.device_name = NULL; /* d3d9 only */
    adapter_id.device_name_size = 0; /* d3d9 only */

    if (SUCCEEDED(hr = wined3d_get_adapter_identifier(d3d8->wined3d, adapter, flags, &adapter_id)))
    {
        identifier->DriverVersion = adapter_id.driver_version;
        identifier->VendorId = adapter_id.vendor_id;
        identifier->DeviceId = adapter_id.device_id;
        identifier->SubSysId = adapter_id.subsystem_id;
        identifier->Revision = adapter_id.revision;
        memcpy(&identifier->DeviceIdentifier, &adapter_id.device_identifier, sizeof(identifier->DeviceIdentifier));
        identifier->WHQLLevel = adapter_id.whql_level;
    }

    return hr;
}

static UINT WINAPI d3d8_GetAdapterModeCount(IDirect3D8 *iface, UINT adapter)
{
    struct d3d8 *d3d8 = impl_from_IDirect3D8(iface);
    HRESULT hr;

    TRACE("iface %p, adapter %u.\n", iface, adapter);

    wined3d_mutex_lock();
    hr = wined3d_get_adapter_mode_count(d3d8->wined3d, adapter,
            WINED3DFMT_UNKNOWN, WINED3D_SCANLINE_ORDERING_UNKNOWN);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI d3d8_EnumAdapterModes(IDirect3D8 *iface, UINT adapter, UINT mode_idx, D3DDISPLAYMODE *mode)
{
    struct d3d8 *d3d8 = impl_from_IDirect3D8(iface);
    struct wined3d_display_mode wined3d_mode;
    HRESULT hr;

    TRACE("iface %p, adapter %u, mode_idx %u, mode %p.\n",
            iface, adapter, mode_idx, mode);

    wined3d_mutex_lock();
    hr = wined3d_enum_adapter_modes(d3d8->wined3d, adapter, WINED3DFMT_UNKNOWN,
            WINED3D_SCANLINE_ORDERING_UNKNOWN, mode_idx, &wined3d_mode);
    wined3d_mutex_unlock();

    if (SUCCEEDED(hr))
    {
        mode->Width = wined3d_mode.width;
        mode->Height = wined3d_mode.height;
        mode->RefreshRate = wined3d_mode.refresh_rate;
        mode->Format = d3dformat_from_wined3dformat(wined3d_mode.format_id);
    }

    return hr;
}

static HRESULT WINAPI d3d8_GetAdapterDisplayMode(IDirect3D8 *iface, UINT adapter, D3DDISPLAYMODE *mode)
{
    struct d3d8 *d3d8 = impl_from_IDirect3D8(iface);
    struct wined3d_display_mode wined3d_mode;
    HRESULT hr;

    TRACE("iface %p, adapter %u, mode %p.\n",
            iface, adapter, mode);

    wined3d_mutex_lock();
    hr = wined3d_get_adapter_display_mode(d3d8->wined3d, adapter, &wined3d_mode, NULL);
    wined3d_mutex_unlock();

    if (SUCCEEDED(hr))
    {
        mode->Width = wined3d_mode.width;
        mode->Height = wined3d_mode.height;
        mode->RefreshRate = wined3d_mode.refresh_rate;
        mode->Format = d3dformat_from_wined3dformat(wined3d_mode.format_id);
    }

    return hr;
}

static HRESULT WINAPI d3d8_CheckDeviceType(IDirect3D8 *iface, UINT adapter, D3DDEVTYPE device_type,
        D3DFORMAT display_format, D3DFORMAT backbuffer_format, BOOL windowed)
{
    struct d3d8 *d3d8 = impl_from_IDirect3D8(iface);
    HRESULT hr;

    TRACE("iface %p, adapter %u, device_type %#x, display_format %#x, backbuffer_format %#x, windowed %#x.\n",
            iface, adapter, device_type, display_format, backbuffer_format, windowed);

    if (!windowed && display_format != D3DFMT_X8R8G8B8 && display_format != D3DFMT_R5G6B5)
        return WINED3DERR_NOTAVAILABLE;

    wined3d_mutex_lock();
    hr = wined3d_check_device_type(d3d8->wined3d, adapter, device_type, wined3dformat_from_d3dformat(display_format),
            wined3dformat_from_d3dformat(backbuffer_format), windowed);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI d3d8_CheckDeviceFormat(IDirect3D8 *iface, UINT adapter, D3DDEVTYPE device_type,
        D3DFORMAT adapter_format, DWORD usage, D3DRESOURCETYPE resource_type, D3DFORMAT format)
{
    struct d3d8 *d3d8 = impl_from_IDirect3D8(iface);
    enum wined3d_resource_type wined3d_rtype;
    unsigned int bind_flags;
    HRESULT hr;

    TRACE("iface %p, adapter %u, device_type %#x, adapter_format %#x, usage %#x, resource_type %#x, format %#x.\n",
            iface, adapter, device_type, adapter_format, usage, resource_type, format);

    if (adapter_format != D3DFMT_X8R8G8B8 && adapter_format != D3DFMT_R5G6B5
            && adapter_format != D3DFMT_X1R5G5B5)
    {
        WARN("Invalid adapter format.\n");
        return adapter_format ? D3DERR_NOTAVAILABLE : D3DERR_INVALIDCALL;
    }

    bind_flags = wined3d_bind_flags_from_d3d8_usage(usage);
    usage = usage & (WINED3DUSAGE_MASK | WINED3DUSAGE_QUERY_MASK);
    switch (resource_type)
    {
        case D3DRTYPE_CUBETEXTURE:
            usage |= WINED3DUSAGE_LEGACY_CUBEMAP;
        case D3DRTYPE_TEXTURE:
            bind_flags |= WINED3D_BIND_SHADER_RESOURCE;
        case D3DRTYPE_SURFACE:
            wined3d_rtype = WINED3D_RTYPE_TEXTURE_2D;
            break;

        case D3DRTYPE_VOLUMETEXTURE:
        case D3DRTYPE_VOLUME:
            bind_flags |= WINED3D_BIND_SHADER_RESOURCE;
            wined3d_rtype = WINED3D_RTYPE_TEXTURE_3D;
            break;

        case D3DRTYPE_VERTEXBUFFER:
        case D3DRTYPE_INDEXBUFFER:
            wined3d_rtype = WINED3D_RTYPE_BUFFER;
            break;

        default:
            FIXME("Unhandled resource type %#x.\n", resource_type);
            return WINED3DERR_INVALIDCALL;
    }

    wined3d_mutex_lock();
    hr = wined3d_check_device_format(d3d8->wined3d, adapter, device_type, wined3dformat_from_d3dformat(adapter_format),
            usage, bind_flags, wined3d_rtype, wined3dformat_from_d3dformat(format));
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI d3d8_CheckDeviceMultiSampleType(IDirect3D8 *iface, UINT adapter, D3DDEVTYPE device_type,
        D3DFORMAT format, BOOL windowed, D3DMULTISAMPLE_TYPE multisample_type)
{
    struct d3d8 *d3d8 = impl_from_IDirect3D8(iface);
    HRESULT hr;

    TRACE("iface %p, adapter %u, device_type %#x, format %#x, windowed %#x, multisample_type %#x.\n",
            iface, adapter, device_type, format, windowed, multisample_type);

    if (multisample_type > D3DMULTISAMPLE_16_SAMPLES)
        return D3DERR_INVALIDCALL;

    wined3d_mutex_lock();
    hr = wined3d_check_device_multisample_type(d3d8->wined3d, adapter, device_type,
            wined3dformat_from_d3dformat(format), windowed,
            (enum wined3d_multisample_type)multisample_type, NULL);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI d3d8_CheckDepthStencilMatch(IDirect3D8 *iface, UINT adapter, D3DDEVTYPE device_type,
        D3DFORMAT adapter_format, D3DFORMAT rt_format, D3DFORMAT ds_format)
{
    struct d3d8 *d3d8 = impl_from_IDirect3D8(iface);
    HRESULT hr;

    TRACE("iface %p, adapter %u, device_type %#x, adapter_format %#x, rt_format %#x, ds_format %#x.\n",
            iface, adapter, device_type, adapter_format, rt_format, ds_format);

    wined3d_mutex_lock();
    hr = wined3d_check_depth_stencil_match(d3d8->wined3d, adapter, device_type,
            wined3dformat_from_d3dformat(adapter_format), wined3dformat_from_d3dformat(rt_format),
            wined3dformat_from_d3dformat(ds_format));
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI d3d8_GetDeviceCaps(IDirect3D8 *iface, UINT adapter, D3DDEVTYPE device_type, D3DCAPS8 *caps)
{
    struct d3d8 *d3d8 = impl_from_IDirect3D8(iface);
    struct wined3d_caps wined3d_caps;
    HRESULT hr;

    TRACE("iface %p, adapter %u, device_type %#x, caps %p.\n", iface, adapter, device_type, caps);

    if (!caps)
        return D3DERR_INVALIDCALL;

    wined3d_mutex_lock();
    hr = wined3d_get_device_caps(d3d8->wined3d, adapter, device_type, &wined3d_caps);
    wined3d_mutex_unlock();

    d3dcaps_from_wined3dcaps(caps, &wined3d_caps);

    return hr;
}

static HMONITOR WINAPI d3d8_GetAdapterMonitor(IDirect3D8 *iface, UINT adapter)
{
    struct d3d8 *d3d8 = impl_from_IDirect3D8(iface);
    struct wined3d_output_desc desc;
    HRESULT hr;

    TRACE("iface %p, adapter %u.\n", iface, adapter);

    wined3d_mutex_lock();
    hr = wined3d_get_output_desc(d3d8->wined3d, adapter, &desc);
    wined3d_mutex_unlock();

    if (FAILED(hr))
    {
        WARN("Failed to get output desc, hr %#x.\n", hr);
        return NULL;
    }

    return desc.monitor;
}

static HRESULT WINAPI d3d8_CreateDevice(IDirect3D8 *iface, UINT adapter,
        D3DDEVTYPE device_type, HWND focus_window, DWORD flags, D3DPRESENT_PARAMETERS *parameters,
        IDirect3DDevice8 **device)
{
    struct d3d8 *d3d8 = impl_from_IDirect3D8(iface);
    struct d3d8_device *object;
    HRESULT hr;

    TRACE("iface %p, adapter %u, device_type %#x, focus_window %p, flags %#x, parameters %p, device %p.\n",
            iface, adapter, device_type, focus_window, flags, parameters, device);

    if (!(object = heap_alloc_zero(sizeof(*object))))
        return E_OUTOFMEMORY;

    hr = device_init(object, d3d8, d3d8->wined3d, adapter, device_type, focus_window, flags, parameters);
    if (FAILED(hr))
    {
        WARN("Failed to initialize device, hr %#x.\n", hr);
        heap_free(object);
        return hr;
    }

    TRACE("Created device %p.\n", object);
    *device = &object->IDirect3DDevice8_iface;

    return D3D_OK;
}

static const struct IDirect3D8Vtbl d3d8_vtbl =
{
    /* IUnknown */
    d3d8_QueryInterface,
    d3d8_AddRef,
    d3d8_Release,
    /* IDirect3D8 */
    d3d8_RegisterSoftwareDevice,
    d3d8_GetAdapterCount,
    d3d8_GetAdapterIdentifier,
    d3d8_GetAdapterModeCount,
    d3d8_EnumAdapterModes,
    d3d8_GetAdapterDisplayMode,
    d3d8_CheckDeviceType,
    d3d8_CheckDeviceFormat,
    d3d8_CheckDeviceMultiSampleType,
    d3d8_CheckDepthStencilMatch,
    d3d8_GetDeviceCaps,
    d3d8_GetAdapterMonitor,
    d3d8_CreateDevice,
};

BOOL d3d8_init(struct d3d8 *d3d8)
{
    DWORD flags = WINED3D_LEGACY_DEPTH_BIAS | WINED3D_VIDMEM_ACCOUNTING
            | WINED3D_HANDLE_RESTORE | WINED3D_PIXEL_CENTER_INTEGER
            | WINED3D_LEGACY_UNBOUND_RESOURCE_COLOR | WINED3D_NO_PRIMITIVE_RESTART
            | WINED3D_LEGACY_CUBEMAP_FILTERING;

    d3d8->IDirect3D8_iface.lpVtbl = &d3d8_vtbl;
    d3d8->refcount = 1;

    wined3d_mutex_lock();
    d3d8->wined3d = wined3d_create(flags);
    wined3d_mutex_unlock();
    if (!d3d8->wined3d)
        return FALSE;

    return TRUE;
}
