/*
 * IDirect3D9 implementation
 *
 * Copyright 2002 Jason Edmeades
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

#include "d3d9_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d9);

static inline struct d3d9 *impl_from_IDirect3D9Ex(IDirect3D9Ex *iface)
{
    return CONTAINING_RECORD(iface, struct d3d9, IDirect3D9Ex_iface);
}

static HRESULT WINAPI d3d9_QueryInterface(IDirect3D9Ex *iface, REFIID riid, void **out)
{
    struct d3d9 *d3d9 = impl_from_IDirect3D9Ex(iface);

    TRACE("iface %p, riid %s, out %p.\n", iface, debugstr_guid(riid), out);

    if (IsEqualGUID(riid, &IID_IDirect3D9)
            || IsEqualGUID(riid, &IID_IUnknown))
    {
        IDirect3D9Ex_AddRef(&d3d9->IDirect3D9Ex_iface);
        *out = &d3d9->IDirect3D9Ex_iface;
        return S_OK;
    }

    if (IsEqualGUID(riid, &IID_IDirect3D9Ex))
    {
        if (!d3d9->extended)
        {
            WARN("Application asks for IDirect3D9Ex, but this instance wasn't created with Direct3DCreate9Ex.\n");
            *out = NULL;
            return E_NOINTERFACE;
        }

        IDirect3D9Ex_AddRef(&d3d9->IDirect3D9Ex_iface);
        *out = &d3d9->IDirect3D9Ex_iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(riid));

    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI d3d9_AddRef(IDirect3D9Ex *iface)
{
    struct d3d9 *d3d9 = impl_from_IDirect3D9Ex(iface);
    ULONG refcount = InterlockedIncrement(&d3d9->refcount);

    TRACE("%p increasing refcount to %lu.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI d3d9_Release(IDirect3D9Ex *iface)
{
    struct d3d9 *d3d9 = impl_from_IDirect3D9Ex(iface);
    ULONG refcount = InterlockedDecrement(&d3d9->refcount);

    TRACE("%p decreasing refcount to %lu.\n", iface, refcount);

    if (!refcount)
    {
        wined3d_decref(d3d9->wined3d);

        free(d3d9->wined3d_outputs);
        free(d3d9);
    }

    return refcount;
}

static HRESULT WINAPI d3d9_RegisterSoftwareDevice(IDirect3D9Ex *iface, void *init_function)
{
    struct d3d9 *d3d9 = impl_from_IDirect3D9Ex(iface);
    HRESULT hr;

    TRACE("iface %p, init_function %p.\n", iface, init_function);

    wined3d_mutex_lock();
    hr = wined3d_register_software_device(d3d9->wined3d, init_function);
    wined3d_mutex_unlock();

    return hr;
}

static UINT WINAPI d3d9_GetAdapterCount(IDirect3D9Ex *iface)
{
    struct d3d9 *d3d9 = impl_from_IDirect3D9Ex(iface);

    TRACE("iface %p.\n", iface);

    return d3d9->wined3d_output_count;
}

static HRESULT WINAPI d3d9_GetAdapterIdentifier(IDirect3D9Ex *iface, UINT adapter,
        DWORD flags, D3DADAPTER_IDENTIFIER9 *identifier)
{
    struct d3d9 *d3d9 = impl_from_IDirect3D9Ex(iface);
    struct wined3d_adapter_identifier adapter_id;
    struct wined3d_adapter *wined3d_adapter;
    struct wined3d_output_desc output_desc;
    unsigned int output_idx;
    HRESULT hr;

    TRACE("iface %p, adapter %u, flags %#lx, identifier %p.\n",
            iface, adapter, flags, identifier);

    output_idx = adapter;
    if (output_idx >= d3d9->wined3d_output_count)
        return D3DERR_INVALIDCALL;

    wined3d_mutex_lock();
    if (FAILED(hr = wined3d_output_get_desc(d3d9->wined3d_outputs[output_idx], &output_desc)))
    {
        wined3d_mutex_unlock();
        WARN("Failed to get output description, hr %#lx.\n", hr);
        return hr;
    }
    WideCharToMultiByte(CP_ACP, 0, output_desc.device_name, -1, identifier->DeviceName,
            sizeof(identifier->DeviceName), NULL, NULL);

    adapter_id.driver = identifier->Driver;
    adapter_id.driver_size = sizeof(identifier->Driver);
    adapter_id.description = identifier->Description;
    adapter_id.description_size = sizeof(identifier->Description);

    wined3d_adapter = wined3d_output_get_adapter(d3d9->wined3d_outputs[output_idx]);
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
    wined3d_mutex_unlock();

    return hr;
}

static UINT WINAPI d3d9_GetAdapterModeCount(IDirect3D9Ex *iface, UINT adapter, D3DFORMAT format)
{
    struct d3d9 *d3d9 = impl_from_IDirect3D9Ex(iface);
    unsigned int output_idx, count;

    TRACE("iface %p, adapter %u, format %#x.\n", iface, adapter, format);

    output_idx = adapter;
    if (output_idx >= d3d9->wined3d_output_count)
        return 0;

    /* Others than that not supported by d3d9, but reported by wined3d for ddraw. Filter them out. */
    if (format != D3DFMT_X8R8G8B8 && format != D3DFMT_R5G6B5)
        return 0;

    wined3d_mutex_lock();
    count = wined3d_output_get_mode_count(d3d9->wined3d_outputs[output_idx],
            wined3dformat_from_d3dformat(format), WINED3D_SCANLINE_ORDERING_UNKNOWN, true);
    wined3d_mutex_unlock();

    return count;
}

static HRESULT WINAPI d3d9_EnumAdapterModes(IDirect3D9Ex *iface, UINT adapter,
        D3DFORMAT format, UINT mode_idx, D3DDISPLAYMODE *mode)
{
    struct d3d9 *d3d9 = impl_from_IDirect3D9Ex(iface);
    struct wined3d_display_mode wined3d_mode;
    unsigned int output_idx;
    HRESULT hr;

    TRACE("iface %p, adapter %u, format %#x, mode_idx %u, mode %p.\n",
            iface, adapter, format, mode_idx, mode);

    output_idx = adapter;
    if (output_idx >= d3d9->wined3d_output_count)
        return D3DERR_INVALIDCALL;

    if (format != D3DFMT_X8R8G8B8 && format != D3DFMT_R5G6B5)
        return D3DERR_INVALIDCALL;

    wined3d_mutex_lock();
    hr = wined3d_output_get_mode(d3d9->wined3d_outputs[output_idx], wined3dformat_from_d3dformat(format),
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

static HRESULT WINAPI d3d9_GetAdapterDisplayMode(IDirect3D9Ex *iface, UINT adapter, D3DDISPLAYMODE *mode)
{
    struct d3d9 *d3d9 = impl_from_IDirect3D9Ex(iface);
    struct wined3d_display_mode wined3d_mode;
    unsigned int output_idx;
    HRESULT hr;

    TRACE("iface %p, adapter %u, mode %p.\n", iface, adapter, mode);

    output_idx = adapter;
    if (output_idx >= d3d9->wined3d_output_count)
        return D3DERR_INVALIDCALL;

    wined3d_mutex_lock();
    hr = wined3d_output_get_display_mode(d3d9->wined3d_outputs[output_idx], &wined3d_mode, NULL);
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

static HRESULT WINAPI d3d9_CheckDeviceType(IDirect3D9Ex *iface, UINT adapter, D3DDEVTYPE device_type,
        D3DFORMAT display_format, D3DFORMAT backbuffer_format, BOOL windowed)
{
    struct d3d9 *d3d9 = impl_from_IDirect3D9Ex(iface);
    unsigned int output_idx;
    HRESULT hr;

    TRACE("iface %p, adapter %u, device_type %#x, display_format %#x, backbuffer_format %#x, windowed %#x.\n",
            iface, adapter, device_type, display_format, backbuffer_format, windowed);

    output_idx = adapter;
    if (output_idx >= d3d9->wined3d_output_count)
        return D3DERR_INVALIDCALL;

    /* Others than that not supported by d3d9, but reported by wined3d for ddraw. Filter them out. */
    if (!windowed && display_format != D3DFMT_X8R8G8B8 && display_format != D3DFMT_R5G6B5)
        return WINED3DERR_NOTAVAILABLE;

    wined3d_mutex_lock();
    hr = wined3d_check_device_type(d3d9->wined3d, d3d9->wined3d_outputs[output_idx],
            wined3d_device_type_from_d3d(device_type), wined3dformat_from_d3dformat(display_format),
            wined3dformat_from_d3dformat(backbuffer_format), windowed);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI d3d9_CheckDeviceFormat(IDirect3D9Ex *iface, UINT adapter, D3DDEVTYPE device_type,
        D3DFORMAT adapter_format, DWORD usage, D3DRESOURCETYPE resource_type, D3DFORMAT format)
{
    struct d3d9 *d3d9 = impl_from_IDirect3D9Ex(iface);
    enum wined3d_resource_type wined3d_rtype;
    struct wined3d_adapter *wined3d_adapter;
    unsigned int bind_flags;
    unsigned int output_idx;
    HRESULT hr;

    TRACE("iface %p, adapter %u, device_type %#x, adapter_format %#x, usage %#lx, resource_type %#x, format %#x.\n",
            iface, adapter, device_type, adapter_format, usage, resource_type, format);

    output_idx = adapter;
    if (output_idx >= d3d9->wined3d_output_count)
        return D3DERR_INVALIDCALL;

    if (adapter_format != D3DFMT_X8R8G8B8 && adapter_format != D3DFMT_R5G6B5
            && adapter_format != D3DFMT_X1R5G5B5)
    {
        WARN("Invalid adapter format.\n");
        return adapter_format ? D3DERR_NOTAVAILABLE : D3DERR_INVALIDCALL;
    }

    bind_flags = wined3d_bind_flags_from_d3d9_usage(usage);
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
    wined3d_adapter = wined3d_output_get_adapter(d3d9->wined3d_outputs[output_idx]);
    if (format == D3DFMT_RESZ && resource_type == D3DRTYPE_SURFACE && usage == D3DUSAGE_RENDERTARGET)
    {
        unsigned int levels;

        hr = wined3d_check_device_multisample_type(wined3d_adapter, wined3d_device_type_from_d3d(device_type),
                WINED3DFMT_D24_UNORM_S8_UINT, FALSE, WINED3D_MULTISAMPLE_NON_MASKABLE, &levels);
        if (SUCCEEDED(hr) && !levels)
            hr = D3DERR_NOTAVAILABLE;
    }
    else
        hr = wined3d_check_device_format(d3d9->wined3d, wined3d_adapter, wined3d_device_type_from_d3d(device_type),
                wined3dformat_from_d3dformat(adapter_format), usage, bind_flags,
                wined3d_rtype, wined3dformat_from_d3dformat(format));
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI d3d9_CheckDeviceMultiSampleType(IDirect3D9Ex *iface, UINT adapter, D3DDEVTYPE device_type,
        D3DFORMAT format, BOOL windowed, D3DMULTISAMPLE_TYPE multisample_type, DWORD *levels)
{
    struct d3d9 *d3d9 = impl_from_IDirect3D9Ex(iface);
    struct wined3d_adapter *wined3d_adapter;
    unsigned int output_idx;
    HRESULT hr;

    TRACE("iface %p, adapter %u, device_type %#x, format %#x, windowed %#x, multisample_type %#x, levels %p.\n",
            iface, adapter, device_type, format, windowed, multisample_type, levels);

    output_idx = adapter;
    if (output_idx >= d3d9->wined3d_output_count)
        return D3DERR_INVALIDCALL;

    if (multisample_type > D3DMULTISAMPLE_16_SAMPLES)
        return D3DERR_INVALIDCALL;

    wined3d_mutex_lock();
    wined3d_adapter = wined3d_output_get_adapter(d3d9->wined3d_outputs[output_idx]);
    hr = wined3d_check_device_multisample_type(wined3d_adapter,
            wined3d_device_type_from_d3d(device_type), wined3dformat_from_d3dformat(format),
            windowed, wined3d_multisample_type_from_d3d(multisample_type), (unsigned int *)levels);
    wined3d_mutex_unlock();
    if (hr == WINED3DERR_NOTAVAILABLE && levels)
        *levels = 1;

    return hr;
}

static HRESULT WINAPI d3d9_CheckDepthStencilMatch(IDirect3D9Ex *iface, UINT adapter, D3DDEVTYPE device_type,
        D3DFORMAT adapter_format, D3DFORMAT rt_format, D3DFORMAT ds_format)
{
    struct d3d9 *d3d9 = impl_from_IDirect3D9Ex(iface);
    struct wined3d_adapter *wined3d_adapter;
    unsigned int output_idx;
    HRESULT hr;

    TRACE("iface %p, adapter %u, device_type %#x, adapter_format %#x, rt_format %#x, ds_format %#x.\n",
            iface, adapter, device_type, adapter_format, rt_format, ds_format);

    output_idx = adapter;
    if (output_idx >= d3d9->wined3d_output_count)
        return D3DERR_INVALIDCALL;

    wined3d_mutex_lock();
    wined3d_adapter = wined3d_output_get_adapter(d3d9->wined3d_outputs[output_idx]);
    hr = wined3d_check_depth_stencil_match(wined3d_adapter,
            wined3d_device_type_from_d3d(device_type), wined3dformat_from_d3dformat(adapter_format),
            wined3dformat_from_d3dformat(rt_format), wined3dformat_from_d3dformat(ds_format));
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI d3d9_CheckDeviceFormatConversion(IDirect3D9Ex *iface, UINT adapter,
        D3DDEVTYPE device_type, D3DFORMAT src_format, D3DFORMAT dst_format)
{
    struct d3d9 *d3d9 = impl_from_IDirect3D9Ex(iface);
    unsigned int output_idx;
    HRESULT hr;

    TRACE("iface %p, adapter %u, device_type %#x, src_format %#x, dst_format %#x.\n",
            iface, adapter, device_type, src_format, dst_format);

    output_idx = adapter;
    if (output_idx >= d3d9->wined3d_output_count)
        return D3DERR_INVALIDCALL;

    if (src_format == dst_format)
        return S_OK;

    wined3d_mutex_lock();
    hr = wined3d_check_device_format_conversion(d3d9->wined3d_outputs[output_idx],
            wined3d_device_type_from_d3d(device_type), wined3dformat_from_d3dformat(src_format),
            wined3dformat_from_d3dformat(dst_format));
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI d3d9_GetDeviceCaps(IDirect3D9Ex *iface, UINT adapter, D3DDEVTYPE device_type, D3DCAPS9 *caps)
{
    struct d3d9 *d3d9 = impl_from_IDirect3D9Ex(iface);
    struct wined3d_adapter *wined3d_adapter;
    struct wined3d_caps wined3d_caps;
    unsigned int output_idx;
    HRESULT hr;

    TRACE("iface %p, adapter %u, device_type %#x, caps %p.\n", iface, adapter, device_type, caps);

    output_idx = adapter;
    if (output_idx >= d3d9->wined3d_output_count)
        return D3DERR_INVALIDCALL;

    if (!caps)
        return D3DERR_INVALIDCALL;

    memset(caps, 0, sizeof(*caps));

    wined3d_mutex_lock();
    wined3d_adapter = wined3d_output_get_adapter(d3d9->wined3d_outputs[output_idx]);
    hr = wined3d_get_device_caps(wined3d_adapter, wined3d_device_type_from_d3d(device_type), &wined3d_caps);
    wined3d_mutex_unlock();

    d3d9_caps_from_wined3dcaps(d3d9, adapter, caps, &wined3d_caps);

    return hr;
}

static HMONITOR WINAPI d3d9_GetAdapterMonitor(IDirect3D9Ex *iface, UINT adapter)
{
    struct d3d9 *d3d9 = impl_from_IDirect3D9Ex(iface);
    struct wined3d_output_desc desc;
    unsigned int output_idx;
    HRESULT hr;

    TRACE("iface %p, adapter %u.\n", iface, adapter);

    output_idx = adapter;
    if (output_idx >= d3d9->wined3d_output_count)
        return NULL;

    wined3d_mutex_lock();
    hr = wined3d_output_get_desc(d3d9->wined3d_outputs[output_idx], &desc);
    wined3d_mutex_unlock();

    if (FAILED(hr))
    {
        WARN("Failed to get output desc, hr %#lx.\n", hr);
        return NULL;
    }

    return desc.monitor;
}

static HRESULT WINAPI DECLSPEC_HOTPATCH d3d9_CreateDevice(IDirect3D9Ex *iface, UINT adapter,
        D3DDEVTYPE device_type, HWND focus_window, DWORD flags, D3DPRESENT_PARAMETERS *parameters,
        IDirect3DDevice9 **device)
{
    struct d3d9 *d3d9 = impl_from_IDirect3D9Ex(iface);
    struct d3d9_device *object;
    HRESULT hr;

    TRACE("iface %p, adapter %u, device_type %#x, focus_window %p, flags %#lx, parameters %p, device %p.\n",
            iface, adapter, device_type, focus_window, flags, parameters, device);

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    hr = device_init(object, d3d9, d3d9->wined3d, adapter, device_type, focus_window, flags, parameters, NULL);
    if (FAILED(hr))
    {
        WARN("Failed to initialize device, hr %#lx.\n", hr);
        free(object);
        return hr;
    }

    TRACE("Created device %p.\n", object);
    *device = (IDirect3DDevice9 *)&object->IDirect3DDevice9Ex_iface;

    return D3D_OK;
}

static UINT WINAPI d3d9_GetAdapterModeCountEx(IDirect3D9Ex *iface,
        UINT adapter, const D3DDISPLAYMODEFILTER *filter)
{
    struct d3d9 *d3d9 = impl_from_IDirect3D9Ex(iface);
    unsigned int output_idx, count;

    TRACE("iface %p, adapter %u, filter %p.\n", iface, adapter, filter);

    output_idx = adapter;
    if (output_idx >= d3d9->wined3d_output_count)
        return 0;

    if (filter->Format != D3DFMT_X8R8G8B8 && filter->Format != D3DFMT_R5G6B5)
        return 0;

    wined3d_mutex_lock();
    count = wined3d_output_get_mode_count(d3d9->wined3d_outputs[output_idx],
            wined3dformat_from_d3dformat(filter->Format), wined3d_scanline_ordering_from_d3d(filter->ScanLineOrdering), true);
    wined3d_mutex_unlock();

    return count;
}

static HRESULT WINAPI d3d9_EnumAdapterModesEx(IDirect3D9Ex *iface,
        UINT adapter, const D3DDISPLAYMODEFILTER *filter, UINT mode_idx, D3DDISPLAYMODEEX *mode)
{
    struct d3d9 *d3d9 = impl_from_IDirect3D9Ex(iface);
    struct wined3d_display_mode wined3d_mode;
    unsigned int output_idx;
    HRESULT hr;

    TRACE("iface %p, adapter %u, filter %p, mode_idx %u, mode %p.\n",
            iface, adapter, filter, mode_idx, mode);

    output_idx = adapter;
    if (output_idx >= d3d9->wined3d_output_count)
        return D3DERR_INVALIDCALL;

    if (filter->Format != D3DFMT_X8R8G8B8 && filter->Format != D3DFMT_R5G6B5)
        return D3DERR_INVALIDCALL;

    wined3d_mutex_lock();
    hr = wined3d_output_get_mode(d3d9->wined3d_outputs[output_idx], wined3dformat_from_d3dformat(filter->Format),
            wined3d_scanline_ordering_from_d3d(filter->ScanLineOrdering), mode_idx, &wined3d_mode, true);
    wined3d_mutex_unlock();

    if (SUCCEEDED(hr))
    {
        mode->Width = wined3d_mode.width;
        mode->Height = wined3d_mode.height;
        mode->RefreshRate = wined3d_mode.refresh_rate;
        mode->Format = d3dformat_from_wined3dformat(wined3d_mode.format_id);
        mode->ScanLineOrdering = d3dscanlineordering_from_wined3d(wined3d_mode.scanline_ordering);
    }

    return hr;
}

static HRESULT WINAPI d3d9_GetAdapterDisplayModeEx(IDirect3D9Ex *iface,
        UINT adapter, D3DDISPLAYMODEEX *mode, D3DDISPLAYROTATION *rotation)
{
    struct d3d9 *d3d9 = impl_from_IDirect3D9Ex(iface);
    struct wined3d_display_mode wined3d_mode;
    unsigned int output_idx;
    HRESULT hr;

    TRACE("iface %p, adapter %u, mode %p, rotation %p.\n",
            iface, adapter, mode, rotation);

    output_idx = adapter;
    if (output_idx >= d3d9->wined3d_output_count)
        return D3DERR_INVALIDCALL;

    if (mode->Size != sizeof(*mode))
        return D3DERR_INVALIDCALL;

    wined3d_mutex_lock();
    hr = wined3d_output_get_display_mode(d3d9->wined3d_outputs[output_idx], &wined3d_mode,
            (enum wined3d_display_rotation *)rotation);
    wined3d_mutex_unlock();

    if (SUCCEEDED(hr))
    {
        mode->Width = wined3d_mode.width;
        mode->Height = wined3d_mode.height;
        mode->RefreshRate = wined3d_mode.refresh_rate;
        mode->Format = d3dformat_from_wined3dformat(wined3d_mode.format_id);
        mode->ScanLineOrdering = d3dscanlineordering_from_wined3d(wined3d_mode.scanline_ordering);
    }

    return hr;
}

static HRESULT WINAPI DECLSPEC_HOTPATCH d3d9_CreateDeviceEx(IDirect3D9Ex *iface,
        UINT adapter, D3DDEVTYPE device_type, HWND focus_window, DWORD flags,
        D3DPRESENT_PARAMETERS *parameters, D3DDISPLAYMODEEX *mode, IDirect3DDevice9Ex **device)
{
    struct d3d9 *d3d9 = impl_from_IDirect3D9Ex(iface);
    struct d3d9_device *object;
    HRESULT hr;

    TRACE("iface %p, adapter %u, device_type %#x, focus_window %p, flags %#lx, parameters %p, mode %p, device %p.\n",
            iface, adapter, device_type, focus_window, flags, parameters, mode, device);

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    hr = device_init(object, d3d9, d3d9->wined3d, adapter, device_type, focus_window, flags, parameters, mode);
    if (FAILED(hr))
    {
        WARN("Failed to initialize device, hr %#lx.\n", hr);
        free(object);
        return hr;
    }

    TRACE("Created device %p.\n", object);
    *device = &object->IDirect3DDevice9Ex_iface;

    return D3D_OK;
}

static HRESULT WINAPI d3d9_GetAdapterLUID(IDirect3D9Ex *iface, UINT adapter, LUID *luid)
{
    struct d3d9 *d3d9 = impl_from_IDirect3D9Ex(iface);
    struct wined3d_adapter_identifier adapter_id;
    struct wined3d_adapter *wined3d_adapter;
    unsigned int output_idx;
    HRESULT hr;

    TRACE("iface %p, adapter %u, luid %p.\n", iface, adapter, luid);

    output_idx = adapter;
    if (output_idx >= d3d9->wined3d_output_count)
        return D3DERR_INVALIDCALL;

    adapter_id.driver_size = 0;
    adapter_id.description_size = 0;

    wined3d_adapter = wined3d_output_get_adapter(d3d9->wined3d_outputs[output_idx]);
    if (SUCCEEDED(hr = wined3d_adapter_get_identifier(wined3d_adapter, 0, &adapter_id)))
        *luid = adapter_id.adapter_luid;

    return hr;
}

static const struct IDirect3D9ExVtbl d3d9_vtbl =
{
    /* IUnknown */
    d3d9_QueryInterface,
    d3d9_AddRef,
    d3d9_Release,
    /* IDirect3D9 */
    d3d9_RegisterSoftwareDevice,
    d3d9_GetAdapterCount,
    d3d9_GetAdapterIdentifier,
    d3d9_GetAdapterModeCount,
    d3d9_EnumAdapterModes,
    d3d9_GetAdapterDisplayMode,
    d3d9_CheckDeviceType,
    d3d9_CheckDeviceFormat,
    d3d9_CheckDeviceMultiSampleType,
    d3d9_CheckDepthStencilMatch,
    d3d9_CheckDeviceFormatConversion,
    d3d9_GetDeviceCaps,
    d3d9_GetAdapterMonitor,
    d3d9_CreateDevice,
    /* IDirect3D9Ex */
    d3d9_GetAdapterModeCountEx,
    d3d9_EnumAdapterModesEx,
    d3d9_GetAdapterDisplayModeEx,
    d3d9_CreateDeviceEx,
    d3d9_GetAdapterLUID,
};

BOOL d3d9_init(struct d3d9 *d3d9, BOOL extended, BOOL d3d9on12)
{
    DWORD flags = WINED3D_PRESENT_CONVERSION | WINED3D_HANDLE_RESTORE | WINED3D_PIXEL_CENTER_INTEGER
            | WINED3D_SRGB_READ_WRITE_CONTROL | WINED3D_LEGACY_UNBOUND_RESOURCE_COLOR
            | WINED3D_NO_PRIMITIVE_RESTART | WINED3D_LEGACY_CUBEMAP_FILTERING
            | WINED3D_NORMALIZED_DEPTH_BIAS | WINED3D_NO_DRAW_INDIRECT;
    unsigned int adapter_idx, output_idx, adapter_count, output_count = 0;
    struct wined3d_adapter *wined3d_adapter;

    if (extended)
        flags |= WINED3D_RESTORE_MODE_ON_ACTIVATE;

    d3d9->IDirect3D9Ex_iface.lpVtbl = &d3d9_vtbl;
    d3d9->refcount = 1;

    wined3d_mutex_lock();
    d3d9->wined3d = wined3d_create(flags);
    if (!d3d9->wined3d)
    {
        wined3d_mutex_unlock();
        return FALSE;
    }

    adapter_count = wined3d_get_adapter_count(d3d9->wined3d);
    for (adapter_idx = 0; adapter_idx < adapter_count; ++adapter_idx)
    {
        wined3d_adapter = wined3d_get_adapter(d3d9->wined3d, adapter_idx);
        output_count += wined3d_adapter_get_output_count(wined3d_adapter);
    }

    d3d9->wined3d_outputs = calloc(output_count, sizeof(*d3d9->wined3d_outputs));
    if (!d3d9->wined3d_outputs)
    {
        wined3d_decref(d3d9->wined3d);
        wined3d_mutex_unlock();
        return FALSE;
    }

    d3d9->wined3d_output_count = 0;
    for (adapter_idx = 0; adapter_idx < adapter_count; ++adapter_idx)
    {
        wined3d_adapter = wined3d_get_adapter(d3d9->wined3d, adapter_idx);
        output_count = wined3d_adapter_get_output_count(wined3d_adapter);
        for (output_idx = 0; output_idx < output_count; ++output_idx)
        {
            d3d9->wined3d_outputs[d3d9->wined3d_output_count] =
                    wined3d_adapter_get_output(wined3d_adapter, output_idx);
            ++d3d9->wined3d_output_count;
        }
    }

    wined3d_mutex_unlock();
    d3d9->extended = extended;
    d3d9->d3d9on12 = d3d9on12;

    return TRUE;
}
