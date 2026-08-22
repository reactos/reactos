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

    TRACE("%p increasing refcount to %lu.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI d3d8_Release(IDirect3D8 *iface)
{
    struct d3d8 *d3d8 = impl_from_IDirect3D8(iface);
    ULONG refcount = InterlockedDecrement(&d3d8->refcount);

    TRACE("%p decreasing refcount to %lu.\n", iface, refcount);

    if (!refcount)
    {
        wined3d_mutex_lock();
        wined3d_decref(d3d8->wined3d);
        wined3d_mutex_unlock();

        free(d3d8->wined3d_outputs);
        free(d3d8);
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

    TRACE("iface %p.\n", iface);

    return d3d8->wined3d_output_count;
}

static HRESULT WINAPI d3d8_GetAdapterIdentifier(IDirect3D8 *iface, UINT adapter,
        DWORD flags, D3DADAPTER_IDENTIFIER8 *identifier)
{
    struct d3d8 *d3d8 = impl_from_IDirect3D8(iface);
    struct wined3d_adapter_identifier adapter_id;
    struct wined3d_adapter *wined3d_adapter;
    unsigned int output_idx;
    HRESULT hr;

    TRACE("iface %p, adapter %u, flags %#lx, identifier %p.\n",
            iface, adapter, flags, identifier);

    output_idx = adapter;
    if (output_idx >= d3d8->wined3d_output_count)
        return D3DERR_INVALIDCALL;

    adapter_id.driver = identifier->Driver;
    adapter_id.driver_size = sizeof(identifier->Driver);
    adapter_id.description = identifier->Description;
    adapter_id.description_size = sizeof(identifier->Description);

    /* D3DENUM_NO_WHQL_LEVEL -> WINED3DENUM_WHQL_LEVEL */
    flags ^= D3DENUM_NO_WHQL_LEVEL;

    wined3d_adapter = wined3d_output_get_adapter(d3d8->wined3d_outputs[output_idx]);
    if (SUCCEEDED(hr = wined3d_adapter_get_identifier(wined3d_adapter, flags, &adapter_id)))
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
    unsigned int output_idx, count;

    TRACE("iface %p, adapter %u.\n", iface, adapter);

    output_idx = adapter;
    if (output_idx >= d3d8->wined3d_output_count)
        return 0;

    wined3d_mutex_lock();
    count = wined3d_output_get_mode_count(d3d8->wined3d_outputs[output_idx],
            WINED3DFMT_UNKNOWN, WINED3D_SCANLINE_ORDERING_UNKNOWN, true);
    wined3d_mutex_unlock();

    return count;
}

static HRESULT WINAPI d3d8_EnumAdapterModes(IDirect3D8 *iface, UINT adapter, UINT mode_idx, D3DDISPLAYMODE *mode)
{
    struct d3d8 *d3d8 = impl_from_IDirect3D8(iface);
    struct wined3d_display_mode wined3d_mode;
    unsigned int output_idx;
    HRESULT hr;

    TRACE("iface %p, adapter %u, mode_idx %u, mode %p.\n",
            iface, adapter, mode_idx, mode);

    output_idx = adapter;
    if (output_idx >= d3d8->wined3d_output_count)
        return D3DERR_INVALIDCALL;

    wined3d_mutex_lock();
    hr = wined3d_output_get_mode(d3d8->wined3d_outputs[output_idx], WINED3DFMT_UNKNOWN,
            WINED3D_SCANLINE_ORDERING_UNKNOWN, mode_idx, &wined3d_mode, true);
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
    unsigned int output_idx;
    HRESULT hr;

    TRACE("iface %p, adapter %u, mode %p.\n",
            iface, adapter, mode);

    output_idx = adapter;
    if (output_idx >= d3d8->wined3d_output_count)
        return D3DERR_INVALIDCALL;

    wined3d_mutex_lock();
    hr = wined3d_output_get_display_mode(d3d8->wined3d_outputs[output_idx], &wined3d_mode, NULL);
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
    unsigned int output_idx;
    HRESULT hr;

    TRACE("iface %p, adapter %u, device_type %#x, display_format %#x, backbuffer_format %#x, windowed %#x.\n",
            iface, adapter, device_type, display_format, backbuffer_format, windowed);

    output_idx = adapter;
    if (output_idx >= d3d8->wined3d_output_count)
        return D3DERR_INVALIDCALL;

    if (!windowed && display_format != D3DFMT_X8R8G8B8 && display_format != D3DFMT_R5G6B5)
        return WINED3DERR_NOTAVAILABLE;

    wined3d_mutex_lock();
    hr = wined3d_check_device_type(d3d8->wined3d, d3d8->wined3d_outputs[output_idx],
            wined3d_device_type_from_d3d(device_type), wined3dformat_from_d3dformat(display_format),
            wined3dformat_from_d3dformat(backbuffer_format), windowed);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI d3d8_CheckDeviceFormat(IDirect3D8 *iface, UINT adapter, D3DDEVTYPE device_type,
        D3DFORMAT adapter_format, DWORD usage, D3DRESOURCETYPE resource_type, D3DFORMAT format)
{
    struct d3d8 *d3d8 = impl_from_IDirect3D8(iface);
    enum wined3d_resource_type wined3d_rtype;
    struct wined3d_adapter *wined3d_adapter;
    unsigned int bind_flags;
    unsigned int output_idx;
    HRESULT hr;

    TRACE("iface %p, adapter %u, device_type %#x, adapter_format %#x, usage %#lx, resource_type %#x, format %#x.\n",
            iface, adapter, device_type, adapter_format, usage, resource_type, format);

    output_idx = adapter;
    if (output_idx >= d3d8->wined3d_output_count)
        return D3DERR_INVALIDCALL;

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
    wined3d_adapter = wined3d_output_get_adapter(d3d8->wined3d_outputs[output_idx]);
    if (format == D3DFMT_RESZ && resource_type == D3DRTYPE_SURFACE && usage == D3DUSAGE_RENDERTARGET)
    {
        unsigned int levels;

        hr = wined3d_check_device_multisample_type(wined3d_adapter, wined3d_device_type_from_d3d(device_type),
                WINED3DFMT_D24_UNORM_S8_UINT, FALSE, WINED3D_MULTISAMPLE_NON_MASKABLE, &levels);
        if (SUCCEEDED(hr) && !levels)
            hr = D3DERR_NOTAVAILABLE;
    }
    else
        hr = wined3d_check_device_format(d3d8->wined3d, wined3d_adapter, wined3d_device_type_from_d3d(device_type),
                wined3dformat_from_d3dformat(adapter_format), usage, bind_flags, wined3d_rtype,
                wined3dformat_from_d3dformat(format));
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI d3d8_CheckDeviceMultiSampleType(IDirect3D8 *iface, UINT adapter, D3DDEVTYPE device_type,
        D3DFORMAT format, BOOL windowed, D3DMULTISAMPLE_TYPE multisample_type)
{
    struct d3d8 *d3d8 = impl_from_IDirect3D8(iface);
    struct wined3d_adapter *wined3d_adapter;
    unsigned int output_idx;
    HRESULT hr;

    TRACE("iface %p, adapter %u, device_type %#x, format %#x, windowed %#x, multisample_type %#x.\n",
            iface, adapter, device_type, format, windowed, multisample_type);

    output_idx = adapter;
    if (output_idx >= d3d8->wined3d_output_count)
        return D3DERR_INVALIDCALL;

    if (multisample_type > D3DMULTISAMPLE_16_SAMPLES)
        return D3DERR_INVALIDCALL;

    wined3d_mutex_lock();
    wined3d_adapter = wined3d_output_get_adapter(d3d8->wined3d_outputs[output_idx]);
    hr = wined3d_check_device_multisample_type(wined3d_adapter, wined3d_device_type_from_d3d(device_type),
            wined3dformat_from_d3dformat(format), windowed,
            (enum wined3d_multisample_type)multisample_type, NULL);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI d3d8_CheckDepthStencilMatch(IDirect3D8 *iface, UINT adapter, D3DDEVTYPE device_type,
        D3DFORMAT adapter_format, D3DFORMAT rt_format, D3DFORMAT ds_format)
{
    struct d3d8 *d3d8 = impl_from_IDirect3D8(iface);
    struct wined3d_adapter *wined3d_adapter;
    unsigned int output_idx;
    HRESULT hr;

    TRACE("iface %p, adapter %u, device_type %#x, adapter_format %#x, rt_format %#x, ds_format %#x.\n",
            iface, adapter, device_type, adapter_format, rt_format, ds_format);

    output_idx = adapter;
    if (output_idx >= d3d8->wined3d_output_count)
        return D3DERR_INVALIDCALL;

    wined3d_mutex_lock();
    wined3d_adapter = wined3d_output_get_adapter(d3d8->wined3d_outputs[output_idx]);
    hr = wined3d_check_depth_stencil_match(wined3d_adapter, wined3d_device_type_from_d3d(device_type),
            wined3dformat_from_d3dformat(adapter_format), wined3dformat_from_d3dformat(rt_format),
            wined3dformat_from_d3dformat(ds_format));
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI d3d8_GetDeviceCaps(IDirect3D8 *iface, UINT adapter, D3DDEVTYPE device_type, D3DCAPS8 *caps)
{
    struct d3d8 *d3d8 = impl_from_IDirect3D8(iface);
    struct wined3d_adapter *wined3d_adapter;
    struct wined3d_caps wined3d_caps;
    unsigned int output_idx;
    HRESULT hr;

    TRACE("iface %p, adapter %u, device_type %#x, caps %p.\n", iface, adapter, device_type, caps);

    output_idx = adapter;
    if (output_idx >= d3d8->wined3d_output_count)
        return D3DERR_INVALIDCALL;

    if (!caps)
        return D3DERR_INVALIDCALL;

    wined3d_mutex_lock();
    wined3d_adapter = wined3d_output_get_adapter(d3d8->wined3d_outputs[output_idx]);
    hr = wined3d_get_device_caps(wined3d_adapter, wined3d_device_type_from_d3d(device_type), &wined3d_caps);
    wined3d_mutex_unlock();

    d3dcaps_from_wined3dcaps(caps, &wined3d_caps, adapter);

    return hr;
}

static HMONITOR WINAPI d3d8_GetAdapterMonitor(IDirect3D8 *iface, UINT adapter)
{
    struct d3d8 *d3d8 = impl_from_IDirect3D8(iface);
    struct wined3d_output_desc desc;
    unsigned int output_idx;
    HRESULT hr;

    TRACE("iface %p, adapter %u.\n", iface, adapter);

    output_idx = adapter;
    if (output_idx >= d3d8->wined3d_output_count)
        return NULL;

    wined3d_mutex_lock();
    hr = wined3d_output_get_desc(d3d8->wined3d_outputs[output_idx], &desc);
    wined3d_mutex_unlock();

    if (FAILED(hr))
    {
        WARN("Failed to get output desc, hr %#lx.\n", hr);
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

    TRACE("iface %p, adapter %u, device_type %#x, focus_window %p, flags %#lx, parameters %p, device %p.\n",
            iface, adapter, device_type, focus_window, flags, parameters, device);

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    hr = device_init(object, d3d8, d3d8->wined3d, adapter, device_type, focus_window, flags, parameters);
    if (FAILED(hr))
    {
        WARN("Failed to initialize device, hr %#lx.\n", hr);
        free(object);
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
    DWORD flags = WINED3D_LEGACY_DEPTH_BIAS | WINED3D_HANDLE_RESTORE
            | WINED3D_PIXEL_CENTER_INTEGER  | WINED3D_LEGACY_UNBOUND_RESOURCE_COLOR
            | WINED3D_NO_PRIMITIVE_RESTART  | WINED3D_LEGACY_CUBEMAP_FILTERING
            | WINED3D_NO_DRAW_INDIRECT;
    unsigned int adapter_idx, output_idx, adapter_count, output_count = 0;
    struct wined3d_adapter *wined3d_adapter;

    d3d8->IDirect3D8_iface.lpVtbl = &d3d8_vtbl;
    d3d8->refcount = 1;

    wined3d_mutex_lock();
    d3d8->wined3d = wined3d_create(flags);
    if (!d3d8->wined3d)
    {
        wined3d_mutex_unlock();
        return FALSE;
    }

    adapter_count = wined3d_get_adapter_count(d3d8->wined3d);
    for (adapter_idx = 0; adapter_idx < adapter_count; ++adapter_idx)
    {
        wined3d_adapter = wined3d_get_adapter(d3d8->wined3d, adapter_idx);
        output_count += wined3d_adapter_get_output_count(wined3d_adapter);
    }

    d3d8->wined3d_outputs = calloc(output_count, sizeof(*d3d8->wined3d_outputs));
    if (!d3d8->wined3d_outputs)
    {
        wined3d_decref(d3d8->wined3d);
        wined3d_mutex_unlock();
        return FALSE;
    }

    d3d8->wined3d_output_count = 0;
    for (adapter_idx = 0; adapter_idx < adapter_count; ++adapter_idx)
    {
        wined3d_adapter = wined3d_get_adapter(d3d8->wined3d, adapter_idx);
        output_count = wined3d_adapter_get_output_count(wined3d_adapter);
        for (output_idx = 0; output_idx < output_count; ++output_idx)
        {
            d3d8->wined3d_outputs[d3d8->wined3d_output_count] =
                    wined3d_adapter_get_output(wined3d_adapter, output_idx);
            ++d3d8->wined3d_output_count;
        }
    }

    wined3d_mutex_unlock();
    return TRUE;
}
