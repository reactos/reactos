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

#include "config.h"
#include "wine/port.h"

#include "dxgi_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(dxgi);

static void dxgi_mode_from_wined3d(DXGI_MODE_DESC *mode, const struct wined3d_display_mode *wined3d_mode)
{
    mode->Width = wined3d_mode->width;
    mode->Height = wined3d_mode->height;
    mode->RefreshRate.Numerator = wined3d_mode->refresh_rate;
    mode->RefreshRate.Denominator = 1;
    mode->Format = dxgi_format_from_wined3dformat(wined3d_mode->format_id);
    mode->ScanlineOrdering = wined3d_mode->scanline_ordering;
    mode->Scaling = DXGI_MODE_SCALING_UNSPECIFIED; /* FIXME */
}

static void dxgi_mode1_from_wined3d(DXGI_MODE_DESC1 *mode, const struct wined3d_display_mode *wined3d_mode)
{
    mode->Width = wined3d_mode->width;
    mode->Height = wined3d_mode->height;
    mode->RefreshRate.Numerator = wined3d_mode->refresh_rate;
    mode->RefreshRate.Denominator = 1;
    mode->Format = dxgi_format_from_wined3dformat(wined3d_mode->format_id);
    mode->ScanlineOrdering = wined3d_mode->scanline_ordering;
    mode->Scaling = DXGI_MODE_SCALING_UNSPECIFIED; /* FIXME */
    mode->Stereo = FALSE; /* FIXME */
}

static HRESULT dxgi_output_find_closest_matching_mode(struct dxgi_output *output,
        struct wined3d_display_mode *mode, IUnknown *device)
{
    struct dxgi_adapter *adapter;
    struct wined3d *wined3d;
    HRESULT hr;

    if (!mode->width != !mode->height)
        return DXGI_ERROR_INVALID_CALL;

    if (mode->format_id == WINED3DFMT_UNKNOWN && !device)
        return DXGI_ERROR_INVALID_CALL;

    if (mode->format_id == WINED3DFMT_UNKNOWN)
    {
        FIXME("Matching formats to device not implemented.\n");
        return E_NOTIMPL;
    }

    wined3d_mutex_lock();
    adapter = output->adapter;
    wined3d = adapter->factory->wined3d;

    hr = wined3d_find_closest_matching_adapter_mode(wined3d, adapter->ordinal, mode);
    wined3d_mutex_unlock();

    return hr;
}

enum dxgi_mode_struct_version
{
    DXGI_MODE_STRUCT_VERSION_0,
    DXGI_MODE_STRUCT_VERSION_1,
};

static HRESULT dxgi_output_get_display_mode_list(struct dxgi_output *output,
        DXGI_FORMAT format, unsigned int *mode_count, void *modes,
        enum dxgi_mode_struct_version struct_version)
{
    enum wined3d_format_id wined3d_format;
    struct wined3d_display_mode mode;
    unsigned int i, max_count;
    struct wined3d *wined3d;
    HRESULT hr;

    if (!mode_count)
        return DXGI_ERROR_INVALID_CALL;

    if (format == DXGI_FORMAT_UNKNOWN)
    {
        *mode_count = 0;
        return S_OK;
    }

    wined3d_format = wined3dformat_from_dxgi_format(format);

    wined3d_mutex_lock();
    wined3d = output->adapter->factory->wined3d;
    max_count = wined3d_get_adapter_mode_count(wined3d, output->adapter->ordinal,
            wined3d_format, WINED3D_SCANLINE_ORDERING_UNKNOWN);

    if (!modes)
    {
        wined3d_mutex_unlock();
        *mode_count = max_count;
        return S_OK;
    }

    if (max_count > *mode_count)
    {
        wined3d_mutex_unlock();
        return DXGI_ERROR_MORE_DATA;
    }

    *mode_count = max_count;

    for (i = 0; i < *mode_count; ++i)
    {
        if (FAILED(hr = wined3d_enum_adapter_modes(wined3d, output->adapter->ordinal,
                wined3d_format, WINED3D_SCANLINE_ORDERING_UNKNOWN, i, &mode)))
        {
            WARN("Failed to enum adapter mode %u, hr %#x.\n", i, hr);
            wined3d_mutex_unlock();
            return hr;
        }

        switch (struct_version)
        {
            case DXGI_MODE_STRUCT_VERSION_0:
            {
                DXGI_MODE_DESC *desc = modes;
                dxgi_mode_from_wined3d(&desc[i], &mode);
                break;
            }

            case DXGI_MODE_STRUCT_VERSION_1:
            {
                DXGI_MODE_DESC1 *desc = modes;
                dxgi_mode1_from_wined3d(&desc[i], &mode);
                break;
            }
        }
    }
    wined3d_mutex_unlock();

    return S_OK;
}

static inline struct dxgi_output *impl_from_IDXGIOutput4(IDXGIOutput4 *iface)
{
    return CONTAINING_RECORD(iface, struct dxgi_output, IDXGIOutput4_iface);
}

/* IUnknown methods */

static HRESULT STDMETHODCALLTYPE dxgi_output_QueryInterface(IDXGIOutput4 *iface, REFIID iid, void **object)
{
    TRACE("iface %p, iid %s, object %p.\n", iface, debugstr_guid(iid), object);

    if (IsEqualGUID(iid, &IID_IDXGIOutput4)
            || IsEqualGUID(iid, &IID_IDXGIOutput3)
            || IsEqualGUID(iid, &IID_IDXGIOutput2)
            || IsEqualGUID(iid, &IID_IDXGIOutput1)
            || IsEqualGUID(iid, &IID_IDXGIOutput)
            || IsEqualGUID(iid, &IID_IDXGIObject)
            || IsEqualGUID(iid, &IID_IUnknown))
    {
        IUnknown_AddRef(iface);
        *object = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));

    *object = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE dxgi_output_AddRef(IDXGIOutput4 *iface)
{
    struct dxgi_output *output = impl_from_IDXGIOutput4(iface);
    ULONG refcount = InterlockedIncrement(&output->refcount);

    TRACE("%p increasing refcount to %u.\n", output, refcount);

    return refcount;
}

static ULONG STDMETHODCALLTYPE dxgi_output_Release(IDXGIOutput4 *iface)
{
    struct dxgi_output *output = impl_from_IDXGIOutput4(iface);
    ULONG refcount = InterlockedDecrement(&output->refcount);

    TRACE("%p decreasing refcount to %u.\n", output, refcount);

    if (!refcount)
    {
        wined3d_private_store_cleanup(&output->private_store);
        IWineDXGIAdapter_Release(&output->adapter->IWineDXGIAdapter_iface);
        heap_free(output);
    }

    return refcount;
}

/* IDXGIObject methods */

static HRESULT STDMETHODCALLTYPE dxgi_output_SetPrivateData(IDXGIOutput4 *iface,
        REFGUID guid, UINT data_size, const void *data)
{
    struct dxgi_output *output = impl_from_IDXGIOutput4(iface);

    TRACE("iface %p, guid %s, data_size %u, data %p.\n", iface, debugstr_guid(guid), data_size, data);

    return dxgi_set_private_data(&output->private_store, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE dxgi_output_SetPrivateDataInterface(IDXGIOutput4 *iface,
        REFGUID guid, const IUnknown *object)
{
    struct dxgi_output *output = impl_from_IDXGIOutput4(iface);

    TRACE("iface %p, guid %s, object %p.\n", iface, debugstr_guid(guid), object);

    return dxgi_set_private_data_interface(&output->private_store, guid, object);
}

static HRESULT STDMETHODCALLTYPE dxgi_output_GetPrivateData(IDXGIOutput4 *iface,
        REFGUID guid, UINT *data_size, void *data)
{
    struct dxgi_output *output = impl_from_IDXGIOutput4(iface);

    TRACE("iface %p, guid %s, data_size %p, data %p.\n", iface, debugstr_guid(guid), data_size, data);

    return dxgi_get_private_data(&output->private_store, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE dxgi_output_GetParent(IDXGIOutput4 *iface,
        REFIID riid, void **parent)
{
    struct dxgi_output *output = impl_from_IDXGIOutput4(iface);

    TRACE("iface %p, riid %s, parent %p.\n", iface, debugstr_guid(riid), parent);

    return IWineDXGIAdapter_QueryInterface(&output->adapter->IWineDXGIAdapter_iface, riid, parent);
}

/* IDXGIOutput methods */

static HRESULT STDMETHODCALLTYPE dxgi_output_GetDesc(IDXGIOutput4 *iface, DXGI_OUTPUT_DESC *desc)
{
    struct dxgi_output *output = impl_from_IDXGIOutput4(iface);
    struct wined3d_output_desc wined3d_desc;
    HRESULT hr;

    TRACE("iface %p, desc %p.\n", iface, desc);

    if (!desc)
        return E_INVALIDARG;

    wined3d_mutex_lock();
    hr = wined3d_get_output_desc(output->adapter->factory->wined3d,
            output->adapter->ordinal, &wined3d_desc);
    wined3d_mutex_unlock();

    if (FAILED(hr))
    {
        WARN("Failed to get output desc, hr %#x.\n", hr);
        return hr;
    }

    memcpy(desc->DeviceName, wined3d_desc.device_name, sizeof(desc->DeviceName));
    desc->DesktopCoordinates = wined3d_desc.desktop_rect;
    desc->AttachedToDesktop = wined3d_desc.attached_to_desktop;
    desc->Rotation = wined3d_desc.rotation;
    desc->Monitor = wined3d_desc.monitor;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE dxgi_output_GetDisplayModeList(IDXGIOutput4 *iface,
        DXGI_FORMAT format, UINT flags, UINT *mode_count, DXGI_MODE_DESC *modes)
{
    struct dxgi_output *output = impl_from_IDXGIOutput4(iface);

    FIXME("iface %p, format %s, flags %#x, mode_count %p, modes %p partial stub!\n",
            iface, debug_dxgi_format(format), flags, mode_count, modes);

    return dxgi_output_get_display_mode_list(output,
            format, mode_count, modes, DXGI_MODE_STRUCT_VERSION_0);
}

static HRESULT STDMETHODCALLTYPE dxgi_output_FindClosestMatchingMode(IDXGIOutput4 *iface,
        const DXGI_MODE_DESC *mode, DXGI_MODE_DESC *closest_match, IUnknown *device)
{
    struct dxgi_output *output = impl_from_IDXGIOutput4(iface);
    struct wined3d_display_mode wined3d_mode;
    HRESULT hr;

    TRACE("iface %p, mode %p, closest_match %p, device %p.\n",
            iface, mode, closest_match, device);

    TRACE("Mode: %s.\n", debug_dxgi_mode(mode));

    wined3d_display_mode_from_dxgi(&wined3d_mode, mode);
    hr = dxgi_output_find_closest_matching_mode(output, &wined3d_mode, device);
    if (SUCCEEDED(hr))
    {
        dxgi_mode_from_wined3d(closest_match, &wined3d_mode);
        TRACE("Returning %s.\n", debug_dxgi_mode(closest_match));
    }

    return hr;
}

static HRESULT STDMETHODCALLTYPE dxgi_output_WaitForVBlank(IDXGIOutput4 *iface)
{
    static BOOL once = FALSE;

    if (!once++)
        FIXME("iface %p stub!\n", iface);
    else
        TRACE("iface %p stub!\n", iface);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE dxgi_output_TakeOwnership(IDXGIOutput4 *iface, IUnknown *device, BOOL exclusive)
{
    FIXME("iface %p, device %p, exclusive %d stub!\n", iface, device, exclusive);

    return E_NOTIMPL;
}

static void STDMETHODCALLTYPE dxgi_output_ReleaseOwnership(IDXGIOutput4 *iface)
{
    FIXME("iface %p stub!\n", iface);
}

static HRESULT STDMETHODCALLTYPE dxgi_output_GetGammaControlCapabilities(IDXGIOutput4 *iface,
        DXGI_GAMMA_CONTROL_CAPABILITIES *gamma_caps)
{
    unsigned int i;

    TRACE("iface %p, gamma_caps %p.\n", iface, gamma_caps);

    if (!gamma_caps)
        return E_INVALIDARG;

    gamma_caps->ScaleAndOffsetSupported = FALSE;
    gamma_caps->MaxConvertedValue = 1.0f;
    gamma_caps->MinConvertedValue = 0.0f;
    gamma_caps->NumGammaControlPoints = 256;

    for (i = 0; i < gamma_caps->NumGammaControlPoints; ++i)
        gamma_caps->ControlPointPositions[i] = i / 255.0f;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE dxgi_output_SetGammaControl(IDXGIOutput4 *iface,
        const DXGI_GAMMA_CONTROL *gamma_control)
{
    FIXME("iface %p, gamma_control %p stub!\n", iface, gamma_control);

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE dxgi_output_GetGammaControl(IDXGIOutput4 *iface,
        DXGI_GAMMA_CONTROL *gamma_control)
{
    FIXME("iface %p, gamma_control %p stub!\n", iface, gamma_control);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE dxgi_output_SetDisplaySurface(IDXGIOutput4 *iface, IDXGISurface *surface)
{
    FIXME("iface %p, surface %p stub!\n", iface, surface);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE dxgi_output_GetDisplaySurfaceData(IDXGIOutput4 *iface, IDXGISurface *surface)
{
    FIXME("iface %p, surface %p stub!\n", iface, surface);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE dxgi_output_GetFrameStatistics(IDXGIOutput4 *iface, DXGI_FRAME_STATISTICS *stats)
{
    FIXME("iface %p, stats %p stub!\n", iface, stats);

    return E_NOTIMPL;
}

/* IDXGIOutput1 methods */

static HRESULT STDMETHODCALLTYPE dxgi_output_GetDisplayModeList1(IDXGIOutput4 *iface,
        DXGI_FORMAT format, UINT flags, UINT *mode_count, DXGI_MODE_DESC1 *modes)
{
    struct dxgi_output *output = impl_from_IDXGIOutput4(iface);

    FIXME("iface %p, format %s, flags %#x, mode_count %p, modes %p partial stub!\n",
            iface, debug_dxgi_format(format), flags, mode_count, modes);

    return dxgi_output_get_display_mode_list(output,
            format, mode_count, modes, DXGI_MODE_STRUCT_VERSION_1);
}

static HRESULT STDMETHODCALLTYPE dxgi_output_FindClosestMatchingMode1(IDXGIOutput4 *iface,
        const DXGI_MODE_DESC1 *mode, DXGI_MODE_DESC1 *closest_match, IUnknown *device)
{
    struct dxgi_output *output = impl_from_IDXGIOutput4(iface);
    struct wined3d_display_mode wined3d_mode;
    HRESULT hr;

    TRACE("iface %p, mode %p, closest_match %p, device %p.\n",
            iface, mode, closest_match, device);

    TRACE("Mode: %s.\n", debug_dxgi_mode1(mode));

    wined3d_display_mode_from_dxgi1(&wined3d_mode, mode);
    hr = dxgi_output_find_closest_matching_mode(output, &wined3d_mode, device);
    if (SUCCEEDED(hr))
    {
        dxgi_mode1_from_wined3d(closest_match, &wined3d_mode);
        TRACE("Returning %s.\n", debug_dxgi_mode1(closest_match));
    }

    return hr;
}

static HRESULT STDMETHODCALLTYPE dxgi_output_GetDisplaySurfaceData1(IDXGIOutput4 *iface,
        IDXGIResource *resource)
{
    FIXME("iface %p, resource %p stub!\n", iface, resource);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE dxgi_output_DuplicateOutput(IDXGIOutput4 *iface,
        IUnknown *device, IDXGIOutputDuplication **output_duplication)
{
    FIXME("iface %p, device %p, output_duplication %p stub!\n", iface, device, output_duplication);

    return E_NOTIMPL;
}

/* IDXGIOutput2 methods */

static BOOL STDMETHODCALLTYPE dxgi_output_SupportsOverlays(IDXGIOutput4 *iface)
{
    FIXME("iface %p stub!\n", iface);

    return FALSE;
}

/* IDXGIOutput3 methods */

static HRESULT STDMETHODCALLTYPE dxgi_output_CheckOverlaySupport(IDXGIOutput4 *iface,
        DXGI_FORMAT format, IUnknown *device, UINT *flags)
{
    FIXME("iface %p, format %#x, device %p, flags %p stub!\n", iface, format, device, flags);

    return E_NOTIMPL;
}

/* IDXGIOutput4 methods */

static HRESULT STDMETHODCALLTYPE dxgi_output_CheckOverlayColorSpaceSupport(IDXGIOutput4 *iface,
        DXGI_FORMAT format, DXGI_COLOR_SPACE_TYPE color_space, IUnknown *device, UINT *flags)
{
    FIXME("iface %p, format %#x, color_space %#x, device %p, flags %p stub!\n",
            iface, format, color_space, device, flags);

    return E_NOTIMPL;
}

static const struct IDXGIOutput4Vtbl dxgi_output_vtbl =
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
    /* IDXGIOutput1 methods */
    dxgi_output_GetDisplayModeList1,
    dxgi_output_FindClosestMatchingMode1,
    dxgi_output_GetDisplaySurfaceData1,
    dxgi_output_DuplicateOutput,
    /* IDXGIOutput2 methods */
    dxgi_output_SupportsOverlays,
    /* IDXGIOutput3 methods */
    dxgi_output_CheckOverlaySupport,
    /* IDXGIOutput4 methods */
    dxgi_output_CheckOverlayColorSpaceSupport,
};

struct dxgi_output *unsafe_impl_from_IDXGIOutput(IDXGIOutput *iface)
{
    if (!iface)
        return NULL;
    assert(iface->lpVtbl == (IDXGIOutputVtbl *)&dxgi_output_vtbl);
    return CONTAINING_RECORD(iface, struct dxgi_output, IDXGIOutput4_iface);
}

static void dxgi_output_init(struct dxgi_output *output, struct dxgi_adapter *adapter)
{
    output->IDXGIOutput4_iface.lpVtbl = &dxgi_output_vtbl;
    output->refcount = 1;
    wined3d_private_store_init(&output->private_store);
    output->adapter = adapter;
    IWineDXGIAdapter_AddRef(&output->adapter->IWineDXGIAdapter_iface);
}

HRESULT dxgi_output_create(struct dxgi_adapter *adapter, struct dxgi_output **output)
{
    if (!(*output = heap_alloc_zero(sizeof(**output))))
        return E_OUTOFMEMORY;

    dxgi_output_init(*output, adapter);
    return S_OK;
}
