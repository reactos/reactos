/*
 * Copyright 2009 Henri Verbeet for CodeWeavers
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

#include "dxgi_private.h"

static inline struct dxgi_output *impl_from_IDXGIOutput(IDXGIOutput *iface)
{
    return CONTAINING_RECORD(iface, struct dxgi_output, IDXGIOutput_iface);
}

/* IUnknown methods */

static HRESULT STDMETHODCALLTYPE dxgi_output_QueryInterface(IDXGIOutput *iface, REFIID riid, void **object)
{
    TRACE("iface %p, riid %s, object %p.\n", iface, debugstr_guid(riid), object);

    if (IsEqualGUID(riid, &IID_IDXGIOutput)
            || IsEqualGUID(riid, &IID_IDXGIObject)
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

static ULONG STDMETHODCALLTYPE dxgi_output_AddRef(IDXGIOutput *iface)
{
    struct dxgi_output *This = impl_from_IDXGIOutput(iface);
    ULONG refcount = InterlockedIncrement(&This->refcount);

    TRACE("%p increasing refcount to %u.\n", This, refcount);

    return refcount;
}

static ULONG STDMETHODCALLTYPE dxgi_output_Release(IDXGIOutput *iface)
{
    struct dxgi_output *This = impl_from_IDXGIOutput(iface);
    ULONG refcount = InterlockedDecrement(&This->refcount);

    TRACE("%p decreasing refcount to %u.\n", This, refcount);

    if (!refcount)
    {
        wined3d_private_store_cleanup(&This->private_store);
        HeapFree(GetProcessHeap(), 0, This);
    }

    return refcount;
}

/* IDXGIObject methods */

static HRESULT STDMETHODCALLTYPE dxgi_output_SetPrivateData(IDXGIOutput *iface,
        REFGUID guid, UINT data_size, const void *data)
{
    struct dxgi_output *output = impl_from_IDXGIOutput(iface);

    TRACE("iface %p, guid %s, data_size %u, data %p.\n", iface, debugstr_guid(guid), data_size, data);

    return dxgi_set_private_data(&output->private_store, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE dxgi_output_SetPrivateDataInterface(IDXGIOutput *iface,
        REFGUID guid, const IUnknown *object)
{
    struct dxgi_output *output = impl_from_IDXGIOutput(iface);

    TRACE("iface %p, guid %s, object %p.\n", iface, debugstr_guid(guid), object);

    return dxgi_set_private_data_interface(&output->private_store, guid, object);
}

static HRESULT STDMETHODCALLTYPE dxgi_output_GetPrivateData(IDXGIOutput *iface,
        REFGUID guid, UINT *data_size, void *data)
{
    struct dxgi_output *output = impl_from_IDXGIOutput(iface);

    TRACE("iface %p, guid %s, data_size %p, data %p.\n", iface, debugstr_guid(guid), data_size, data);

    return dxgi_get_private_data(&output->private_store, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE dxgi_output_GetParent(IDXGIOutput *iface,
        REFIID riid, void **parent)
{
    struct dxgi_output *This = impl_from_IDXGIOutput(iface);

    TRACE("iface %p, riid %s, parent %p.\n", iface, debugstr_guid(riid), parent);

    return IDXGIAdapter_QueryInterface((IDXGIAdapter *)This->adapter, riid, parent);
}

/* IDXGIOutput methods */

static HRESULT STDMETHODCALLTYPE dxgi_output_GetDesc(IDXGIOutput *iface, DXGI_OUTPUT_DESC *desc)
{
    struct dxgi_output *This = impl_from_IDXGIOutput(iface);
    struct wined3d *wined3d;
    MONITORINFOEXW monitor_info;

    FIXME("iface %p, desc %p semi-stub!\n", iface, desc);

    if (!desc)
        return DXGI_ERROR_INVALID_CALL;

    wined3d = This->adapter->parent->wined3d;

    EnterCriticalSection(&dxgi_cs);
    desc->Monitor = wined3d_get_adapter_monitor(wined3d, This->adapter->ordinal);
    LeaveCriticalSection(&dxgi_cs);

    if (!desc->Monitor)
        return DXGI_ERROR_INVALID_CALL;

    monitor_info.cbSize = sizeof(monitor_info);
    if (!GetMonitorInfoW(desc->Monitor, (MONITORINFO *)&monitor_info))
        return DXGI_ERROR_INVALID_CALL;

    memcpy(&desc->DeviceName, &monitor_info.szDevice, sizeof(desc->DeviceName));
    memcpy(&desc->DesktopCoordinates, &monitor_info.rcMonitor, sizeof(RECT));
    desc->AttachedToDesktop = TRUE;
    desc->Rotation = DXGI_MODE_ROTATION_IDENTITY;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE dxgi_output_GetDisplayModeList(IDXGIOutput *iface,
        DXGI_FORMAT format, UINT flags, UINT *mode_count, DXGI_MODE_DESC *desc)
{
    struct dxgi_output *This = impl_from_IDXGIOutput(iface);
    enum wined3d_format_id wined3d_format;
    struct wined3d *wined3d;
    UINT i;
    UINT max_count;

    FIXME("iface %p, format %s, flags %#x, mode_count %p, desc %p partial stub!\n",
            iface, debug_dxgi_format(format), flags, mode_count, desc);

    if (!mode_count)
        return DXGI_ERROR_INVALID_CALL;

    if (format == DXGI_FORMAT_UNKNOWN)
    {
        *mode_count = 0;
        return S_OK;
    }

    wined3d = This->adapter->parent->wined3d;
    wined3d_format = wined3dformat_from_dxgi_format(format);

    EnterCriticalSection(&dxgi_cs);
    max_count = wined3d_get_adapter_mode_count(wined3d, This->adapter->ordinal,
            wined3d_format, WINED3D_SCANLINE_ORDERING_UNKNOWN);

    if (!desc)
    {
        LeaveCriticalSection(&dxgi_cs);
        *mode_count = max_count;
        return S_OK;
    }

    if (max_count > *mode_count)
    {
        LeaveCriticalSection(&dxgi_cs);
        return DXGI_ERROR_MORE_DATA;
    }

    *mode_count = max_count;

    for (i = 0; i < *mode_count; ++i)
    {
        struct wined3d_display_mode mode;
        HRESULT hr;

        hr = wined3d_enum_adapter_modes(wined3d, This->adapter->ordinal, wined3d_format,
                WINED3D_SCANLINE_ORDERING_UNKNOWN, i, &mode);
        if (FAILED(hr))
        {
            WARN("EnumAdapterModes failed, hr %#x.\n", hr);
            LeaveCriticalSection(&dxgi_cs);
            return hr;
        }

        desc[i].Width = mode.width;
        desc[i].Height = mode.height;
        desc[i].RefreshRate.Numerator = mode.refresh_rate;
        desc[i].RefreshRate.Denominator = 1;
        desc[i].Format = format;
        desc[i].ScanlineOrdering = mode.scanline_ordering;
        desc[i].Scaling = DXGI_MODE_SCALING_UNSPECIFIED; /* FIXME */
    }
    LeaveCriticalSection(&dxgi_cs);

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE dxgi_output_FindClosestMatchingMode(IDXGIOutput *iface,
        const DXGI_MODE_DESC *mode, DXGI_MODE_DESC *closest_match, IUnknown *device)
{
    FIXME("iface %p, mode %p, closest_match %p, device %p stub!\n", iface, mode, closest_match, device);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE dxgi_output_WaitForVBlank(IDXGIOutput *iface)
{
    FIXME("iface %p stub!\n", iface);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE dxgi_output_TakeOwnership(IDXGIOutput *iface, IUnknown *device, BOOL exclusive)
{
    FIXME("iface %p, device %p, exclusive %d stub!\n", iface, device, exclusive);

    return E_NOTIMPL;
}

static void STDMETHODCALLTYPE dxgi_output_ReleaseOwnership(IDXGIOutput *iface)
{
    FIXME("iface %p stub!\n", iface);
}

static HRESULT STDMETHODCALLTYPE dxgi_output_GetGammaControlCapabilities(IDXGIOutput *iface,
        DXGI_GAMMA_CONTROL_CAPABILITIES *gamma_caps)
{
    FIXME("iface %p, gamma_caps %p stub!\n", iface, gamma_caps);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE dxgi_output_SetGammaControl(IDXGIOutput *iface,
        const DXGI_GAMMA_CONTROL *gamma_control)
{
    FIXME("iface %p, gamma_control %p stub!\n", iface, gamma_control);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE dxgi_output_GetGammaControl(IDXGIOutput *iface, DXGI_GAMMA_CONTROL *gamma_control)
{
    FIXME("iface %p, gamma_control %p stub!\n", iface, gamma_control);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE dxgi_output_SetDisplaySurface(IDXGIOutput *iface, IDXGISurface *surface)
{
    FIXME("iface %p, surface %p stub!\n", iface, surface);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE dxgi_output_GetDisplaySurfaceData(IDXGIOutput *iface, IDXGISurface *surface)
{
    FIXME("iface %p, surface %p stub!\n", iface, surface);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE dxgi_output_GetFrameStatistics(IDXGIOutput *iface, DXGI_FRAME_STATISTICS *stats)
{
    FIXME("iface %p, stats %p stub!\n", iface, stats);

    return E_NOTIMPL;
}

static const struct IDXGIOutputVtbl dxgi_output_vtbl =
{
    dxgi_output_QueryInterface,
    dxgi_output_AddRef,
    dxgi_output_Release,
    /* IDXGIObject methods */
    dxgi_output_SetPrivateData,
    dxgi_output_SetPrivateDataInterface,
    dxgi_output_GetPrivateData,
    dxgi_output_GetParent,
    /* IDXGIOutput methods */
    dxgi_output_GetDesc,
    dxgi_output_GetDisplayModeList,
    dxgi_output_FindClosestMatchingMode,
    dxgi_output_WaitForVBlank,
    dxgi_output_TakeOwnership,
    dxgi_output_ReleaseOwnership,
    dxgi_output_GetGammaControlCapabilities,
    dxgi_output_SetGammaControl,
    dxgi_output_GetGammaControl,
    dxgi_output_SetDisplaySurface,
    dxgi_output_GetDisplaySurfaceData,
    dxgi_output_GetFrameStatistics,
};

void dxgi_output_init(struct dxgi_output *output, struct dxgi_adapter *adapter)
{
    output->IDXGIOutput_iface.lpVtbl = &dxgi_output_vtbl;
    output->refcount = 1;
    wined3d_private_store_init(&output->private_store);
    output->adapter = adapter;
}
