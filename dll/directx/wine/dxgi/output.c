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

WINE_DEFAULT_DEBUG_CHANNEL(dxgi);

static inline DXGI_MODE_SCANLINE_ORDER dxgi_mode_scanline_order_from_wined3d(enum wined3d_scanline_ordering ordering)
{
    return (DXGI_MODE_SCANLINE_ORDER)ordering;
}

static inline DXGI_MODE_ROTATION dxgi_mode_rotation_from_wined3d(enum wined3d_display_rotation rotation)
{
    return (DXGI_MODE_ROTATION)rotation;
}

static void dxgi_mode_from_wined3d(DXGI_MODE_DESC *mode, const struct wined3d_display_mode *wined3d_mode)
{
    mode->Width = wined3d_mode->width;
    mode->Height = wined3d_mode->height;
    mode->RefreshRate.Numerator = wined3d_mode->refresh_rate;
    mode->RefreshRate.Denominator = 1;
    mode->Format = dxgi_format_from_wined3dformat(wined3d_mode->format_id);
    mode->ScanlineOrdering = dxgi_mode_scanline_order_from_wined3d(wined3d_mode->scanline_ordering);
    mode->Scaling = DXGI_MODE_SCALING_UNSPECIFIED; /* FIXME */
}

static void dxgi_mode1_from_wined3d(DXGI_MODE_DESC1 *mode, const struct wined3d_display_mode *wined3d_mode)
{
    mode->Width = wined3d_mode->width;
    mode->Height = wined3d_mode->height;
    mode->RefreshRate.Numerator = wined3d_mode->refresh_rate;
    mode->RefreshRate.Denominator = 1;
    mode->Format = dxgi_format_from_wined3dformat(wined3d_mode->format_id);
    mode->ScanlineOrdering = dxgi_mode_scanline_order_from_wined3d(wined3d_mode->scanline_ordering);
    mode->Scaling = DXGI_MODE_SCALING_UNSPECIFIED; /* FIXME */
    mode->Stereo = FALSE; /* FIXME */
}

static HRESULT dxgi_output_find_closest_matching_mode(struct dxgi_output *output,
        struct wined3d_display_mode *mode, IUnknown *device)
{
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
    hr = wined3d_output_find_closest_matching_mode(output->wined3d_output, mode);
    wined3d_mutex_unlock();

    return hr;
}

static int dxgi_mode_desc_compare(const void *l, const void *r)
{
    const DXGI_MODE_DESC *left = l, *right = r;
    int a, b;

    if (left->Width != right->Width)
        return left->Width - right->Width;

    if (left->Height != right->Height)
        return left->Height - right->Height;

    a = left->RefreshRate.Numerator * right->RefreshRate.Denominator;
    b = right->RefreshRate.Numerator * left->RefreshRate.Denominator;
    if (a != b)
        return a - b;

    return 0;
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
    max_count = wined3d_output_get_mode_count(output->wined3d_output,
            wined3d_format, WINED3D_SCANLINE_ORDERING_UNKNOWN, false);

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
        if (FAILED(hr = wined3d_output_get_mode(output->wined3d_output, wined3d_format,
                WINED3D_SCANLINE_ORDERING_UNKNOWN, i, &mode, true)))
        {
            WARN("Failed to get output mode %u, hr %#lx.\n", i, hr);
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

    switch (struct_version)
    {
        case DXGI_MODE_STRUCT_VERSION_0:
            qsort(modes, *mode_count, sizeof(DXGI_MODE_DESC), dxgi_mode_desc_compare);
            break;
        case DXGI_MODE_STRUCT_VERSION_1:
            qsort(modes, *mode_count, sizeof(DXGI_MODE_DESC1), dxgi_mode_desc_compare);
            break;
    }

    return S_OK;
}

static inline struct dxgi_output *impl_from_IDXGIOutput6(IDXGIOutput6 *iface)
{
    return CONTAINING_RECORD(iface, struct dxgi_output, IDXGIOutput6_iface);
}

/* IUnknown methods */

static HRESULT STDMETHODCALLTYPE dxgi_output_QueryInterface(IDXGIOutput6 *iface, REFIID iid, void **object)
{
    TRACE("iface %p, iid %s, object %p.\n", iface, debugstr_guid(iid), object);

    if (IsEqualGUID(iid, &IID_IDXGIOutput6)
            || IsEqualGUID(iid, &IID_IDXGIOutput5)
            || IsEqualGUID(iid, &IID_IDXGIOutput4)
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

static ULONG STDMETHODCALLTYPE dxgi_output_AddRef(IDXGIOutput6 *iface)
{
    struct dxgi_output *output = impl_from_IDXGIOutput6(iface);
    ULONG refcount = InterlockedIncrement(&output->refcount);

    TRACE("%p increasing refcount to %lu.\n", output, refcount);

    return refcount;
}

static ULONG STDMETHODCALLTYPE dxgi_output_Release(IDXGIOutput6 *iface)
{
    struct dxgi_output *output = impl_from_IDXGIOutput6(iface);
    ULONG refcount = InterlockedDecrement(&output->refcount);

    TRACE("%p decreasing refcount to %lu.\n", output, refcount);

    if (!refcount)
    {
        wined3d_private_store_cleanup(&output->private_store);
        IWineDXGIAdapter_Release(&output->adapter->IWineDXGIAdapter_iface);
        free(output);
    }

    return refcount;
}

/* IDXGIObject methods */

static HRESULT STDMETHODCALLTYPE dxgi_output_SetPrivateData(IDXGIOutput6 *iface,
        REFGUID guid, UINT data_size, const void *data)
{
    struct dxgi_output *output = impl_from_IDXGIOutput6(iface);

    TRACE("iface %p, guid %s, data_size %u, data %p.\n", iface, debugstr_guid(guid), data_size, data);

    return dxgi_set_private_data(&output->private_store, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE dxgi_output_SetPrivateDataInterface(IDXGIOutput6 *iface,
        REFGUID guid, const IUnknown *object)
{
    struct dxgi_output *output = impl_from_IDXGIOutput6(iface);

    TRACE("iface %p, guid %s, object %p.\n", iface, debugstr_guid(guid), object);

    return dxgi_set_private_data_interface(&output->private_store, guid, object);
}

static HRESULT STDMETHODCALLTYPE dxgi_output_GetPrivateData(IDXGIOutput6 *iface,
        REFGUID guid, UINT *data_size, void *data)
{
    struct dxgi_output *output = impl_from_IDXGIOutput6(iface);

    TRACE("iface %p, guid %s, data_size %p, data %p.\n", iface, debugstr_guid(guid), data_size, data);

    return dxgi_get_private_data(&output->private_store, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE dxgi_output_GetParent(IDXGIOutput6 *iface,
        REFIID riid, void **parent)
{
    struct dxgi_output *output = impl_from_IDXGIOutput6(iface);

    TRACE("iface %p, riid %s, parent %p.\n", iface, debugstr_guid(riid), parent);

    return IWineDXGIAdapter_QueryInterface(&output->adapter->IWineDXGIAdapter_iface, riid, parent);
}

/* IDXGIOutput methods */

static HRESULT STDMETHODCALLTYPE dxgi_output_GetDesc(IDXGIOutput6 *iface, DXGI_OUTPUT_DESC *desc)
{
    struct dxgi_output *output = impl_from_IDXGIOutput6(iface);
    struct wined3d_output_desc wined3d_desc;
    enum wined3d_display_rotation rotation;
    struct wined3d_display_mode mode;
    HRESULT hr;

    TRACE("iface %p, desc %p.\n", iface, desc);

    if (!desc)
        return E_INVALIDARG;

    wined3d_mutex_lock();
    hr = wined3d_output_get_desc(output->wined3d_output, &wined3d_desc);
    if (FAILED(hr))
    {
        WARN("Failed to get output desc, hr %#lx.\n", hr);
        wined3d_mutex_unlock();
        return hr;
    }

    hr = wined3d_output_get_display_mode(output->wined3d_output, &mode, &rotation);
    if (FAILED(hr))
    {
        WARN("Failed to get output display mode, hr %#lx.\n", hr);
        wined3d_mutex_unlock();
        return hr;
    }
    wined3d_mutex_unlock();

    memcpy(desc->DeviceName, wined3d_desc.device_name, sizeof(desc->DeviceName));
    desc->DesktopCoordinates = wined3d_desc.desktop_rect;
    desc->AttachedToDesktop = wined3d_desc.attached_to_desktop;
    desc->Rotation = dxgi_mode_rotation_from_wined3d(rotation);
    desc->Monitor = wined3d_desc.monitor;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE dxgi_output_GetDisplayModeList(IDXGIOutput6 *iface,
        DXGI_FORMAT format, UINT flags, UINT *mode_count, DXGI_MODE_DESC *modes)
{
    struct dxgi_output *output = impl_from_IDXGIOutput6(iface);

    FIXME("iface %p, format %s, flags %#x, mode_count %p, modes %p partial stub!\n",
            iface, debug_dxgi_format(format), flags, mode_count, modes);

    return dxgi_output_get_display_mode_list(output,
            format, mode_count, modes, DXGI_MODE_STRUCT_VERSION_0);
}

static HRESULT STDMETHODCALLTYPE dxgi_output_FindClosestMatchingMode(IDXGIOutput6 *iface,
        const DXGI_MODE_DESC *mode, DXGI_MODE_DESC *closest_match, IUnknown *device)
{
    struct dxgi_output *output = impl_from_IDXGIOutput6(iface);
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

static HRESULT STDMETHODCALLTYPE dxgi_output_WaitForVBlank(IDXGIOutput6 *iface)
{
    static BOOL once = FALSE;

    if (!once++)
        FIXME("iface %p stub!\n", iface);
    else
        TRACE("iface %p stub!\n", iface);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE dxgi_output_TakeOwnership(IDXGIOutput6 *iface, IUnknown *device, BOOL exclusive)
{
    struct dxgi_output *output = impl_from_IDXGIOutput6(iface);
    HRESULT hr;

    TRACE("iface %p, device %p, exclusive %d.\n", iface, device, exclusive);

    if (!device)
        return DXGI_ERROR_INVALID_CALL;

    wined3d_mutex_lock();
    hr = wined3d_output_take_ownership(output->wined3d_output, exclusive);
    wined3d_mutex_unlock();

    return hr;
}

static void STDMETHODCALLTYPE dxgi_output_ReleaseOwnership(IDXGIOutput6 *iface)
{
    struct dxgi_output *output = impl_from_IDXGIOutput6(iface);

    TRACE("iface %p.\n", iface);

    wined3d_mutex_lock();
    wined3d_output_release_ownership(output->wined3d_output);
    wined3d_mutex_unlock();
}

static HRESULT STDMETHODCALLTYPE dxgi_output_GetGammaControlCapabilities(IDXGIOutput6 *iface,
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

static WORD uint16_from_float(float f)
{
    f *= 65535.0f;
    if (f < 0.0f)
        f = 0.0f;
    else if (f > 65535.0f)
        f = 65535.0f;

    return f + 0.5f;
}

static HRESULT STDMETHODCALLTYPE dxgi_output_SetGammaControl(IDXGIOutput6 *iface,
        const DXGI_GAMMA_CONTROL *gamma_control)
{
    struct dxgi_output *output = impl_from_IDXGIOutput6(iface);
    struct wined3d_gamma_ramp ramp;
    const DXGI_RGB *p;
    unsigned int i;

    TRACE("iface %p, gamma_control %p.\n", iface, gamma_control);

    if (gamma_control->Scale.Red != 1.0f || gamma_control->Scale.Green != 1.0f || gamma_control->Scale.Blue != 1.0f)
        FIXME("Ignoring unhandled scale {%.8e, %.8e, %.8e}.\n", gamma_control->Scale.Red,
                gamma_control->Scale.Green, gamma_control->Scale.Blue);
    if (gamma_control->Offset.Red != 0.0f || gamma_control->Offset.Green != 0.0f || gamma_control->Offset.Blue != 0.0f)
        FIXME("Ignoring unhandled offset {%.8e, %.8e, %.8e}.\n", gamma_control->Offset.Red,
                gamma_control->Offset.Green, gamma_control->Offset.Blue);

    for (i = 0; i < 256; ++i)
    {
        p = &gamma_control->GammaCurve[i];
        ramp.red[i] = uint16_from_float(p->Red);
        ramp.green[i] = uint16_from_float(p->Green);
        ramp.blue[i] = uint16_from_float(p->Blue);
    }

    wined3d_mutex_lock();
    wined3d_output_set_gamma_ramp(output->wined3d_output, &ramp);
    wined3d_mutex_unlock();

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE dxgi_output_GetGammaControl(IDXGIOutput6 *iface,
        DXGI_GAMMA_CONTROL *gamma_control)
{
    FIXME("iface %p, gamma_control %p stub!\n", iface, gamma_control);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE dxgi_output_SetDisplaySurface(IDXGIOutput6 *iface, IDXGISurface *surface)
{
    FIXME("iface %p, surface %p stub!\n", iface, surface);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE dxgi_output_GetDisplaySurfaceData(IDXGIOutput6 *iface, IDXGISurface *surface)
{
    FIXME("iface %p, surface %p stub!\n", iface, surface);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE dxgi_output_GetFrameStatistics(IDXGIOutput6 *iface, DXGI_FRAME_STATISTICS *stats)
{
    FIXME("iface %p, stats %p stub!\n", iface, stats);

    return E_NOTIMPL;
}

/* IDXGIOutput1 methods */

static HRESULT STDMETHODCALLTYPE dxgi_output_GetDisplayModeList1(IDXGIOutput6 *iface,
        DXGI_FORMAT format, UINT flags, UINT *mode_count, DXGI_MODE_DESC1 *modes)
{
    struct dxgi_output *output = impl_from_IDXGIOutput6(iface);

    FIXME("iface %p, format %s, flags %#x, mode_count %p, modes %p partial stub!\n",
            iface, debug_dxgi_format(format), flags, mode_count, modes);

    return dxgi_output_get_display_mode_list(output,
            format, mode_count, modes, DXGI_MODE_STRUCT_VERSION_1);
}

static HRESULT STDMETHODCALLTYPE dxgi_output_FindClosestMatchingMode1(IDXGIOutput6 *iface,
        const DXGI_MODE_DESC1 *mode, DXGI_MODE_DESC1 *closest_match, IUnknown *device)
{
    struct dxgi_output *output = impl_from_IDXGIOutput6(iface);
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

static HRESULT STDMETHODCALLTYPE dxgi_output_GetDisplaySurfaceData1(IDXGIOutput6 *iface,
        IDXGIResource *resource)
{
    FIXME("iface %p, resource %p stub!\n", iface, resource);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE dxgi_output_DuplicateOutput(IDXGIOutput6 *iface,
        IUnknown *device, IDXGIOutputDuplication **output_duplication)
{
    FIXME("iface %p, device %p, output_duplication %p stub!\n", iface, device, output_duplication);

    return E_NOTIMPL;
}

/* IDXGIOutput2 methods */

static BOOL STDMETHODCALLTYPE dxgi_output_SupportsOverlays(IDXGIOutput6 *iface)
{
    FIXME("iface %p stub!\n", iface);

    return FALSE;
}

/* IDXGIOutput3 methods */

static HRESULT STDMETHODCALLTYPE dxgi_output_CheckOverlaySupport(IDXGIOutput6 *iface,
        DXGI_FORMAT format, IUnknown *device, UINT *flags)
{
    FIXME("iface %p, format %#x, device %p, flags %p stub!\n", iface, format, device, flags);

    return E_NOTIMPL;
}

/* IDXGIOutput4 methods */

static HRESULT STDMETHODCALLTYPE dxgi_output_CheckOverlayColorSpaceSupport(IDXGIOutput6 *iface,
        DXGI_FORMAT format, DXGI_COLOR_SPACE_TYPE color_space, IUnknown *device, UINT *flags)
{
    FIXME("iface %p, format %#x, color_space %#x, device %p, flags %p stub!\n",
            iface, format, color_space, device, flags);

    return E_NOTIMPL;
}

/* IDXGIOutput5 methods */

static HRESULT STDMETHODCALLTYPE dxgi_output_DuplicateOutput1(IDXGIOutput6 *iface,
        IUnknown *device, UINT flags, UINT format_count, const DXGI_FORMAT *formats,
        IDXGIOutputDuplication **output_duplication)
{
    FIXME("iface %p, device %p, flags %#x, format_count %u, formats %p, "
            "output_duplication %p stub!\n", iface, device, flags, format_count,
            formats, output_duplication);

    return E_NOTIMPL;
}

/* IDXGIOutput6 methods */

static HRESULT STDMETHODCALLTYPE dxgi_output_GetDesc1(IDXGIOutput6 *iface,
        DXGI_OUTPUT_DESC1 *desc)
{
    struct dxgi_output *output = impl_from_IDXGIOutput6(iface);
    struct wined3d_output_desc wined3d_desc;
    enum wined3d_display_rotation rotation;
    struct wined3d_display_mode mode;
    HRESULT hr;

    FIXME("iface %p, desc %p semi-stub!\n", iface, desc);

    if (!desc)
        return E_INVALIDARG;

    wined3d_mutex_lock();
    hr = wined3d_output_get_desc(output->wined3d_output, &wined3d_desc);
    if (FAILED(hr))
    {
        WARN("Failed to get output desc, hr %#lx.\n", hr);
        wined3d_mutex_unlock();
        return hr;
    }

    hr = wined3d_output_get_display_mode(output->wined3d_output, &mode, &rotation);
    if (FAILED(hr))
    {
        WARN("Failed to get output display mode, hr %#lx.\n", hr);
        wined3d_mutex_unlock();
        return hr;
    }
    wined3d_mutex_unlock();

    if (FAILED(hr))
    {
        WARN("Failed to get output desc, hr %#lx.\n", hr);
        return hr;
    }

    memcpy(desc->DeviceName, wined3d_desc.device_name, sizeof(desc->DeviceName));
    desc->DesktopCoordinates = wined3d_desc.desktop_rect;
    desc->AttachedToDesktop = wined3d_desc.attached_to_desktop;
    desc->Rotation = dxgi_mode_rotation_from_wined3d(rotation);
    desc->Monitor = wined3d_desc.monitor;

    /* FIXME: fill this from monitor EDID */
    desc->BitsPerColor = 0;
    desc->ColorSpace = DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709;
    desc->RedPrimary[0] = 0.f;
    desc->RedPrimary[1] = 0.f;
    desc->GreenPrimary[0] = 0.f;
    desc->GreenPrimary[1] = 0.f;
    desc->BluePrimary[0] = 0.f;
    desc->BluePrimary[1] = 0.f;
    desc->WhitePoint[0] = 0.f;
    desc->WhitePoint[1] = 0.f;
    desc->MinLuminance = 0.f;
    desc->MaxLuminance = 0.f;
    desc->MaxFullFrameLuminance = 0.f;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE dxgi_output_CheckHardwareCompositionSupport(IDXGIOutput6 *iface,
        UINT *flags)
{
    FIXME("iface %p, flags %p stub!\n", iface, flags);

    return E_NOTIMPL;
}

static const struct IDXGIOutput6Vtbl dxgi_output_vtbl =
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
    /* IDXGIOutput5 methods */
    dxgi_output_DuplicateOutput1,
    /* IDXGIOutput6 methods */
    dxgi_output_GetDesc1,
    dxgi_output_CheckHardwareCompositionSupport,
};

struct dxgi_output *unsafe_impl_from_IDXGIOutput(IDXGIOutput *iface)
{
    if (!iface)
        return NULL;
    assert(iface->lpVtbl == (IDXGIOutputVtbl *)&dxgi_output_vtbl);
    return CONTAINING_RECORD(iface, struct dxgi_output, IDXGIOutput6_iface);
}

static void dxgi_output_init(struct dxgi_output *output, unsigned int output_idx,
        struct dxgi_adapter *adapter)
{
    output->IDXGIOutput6_iface.lpVtbl = &dxgi_output_vtbl;
    output->refcount = 1;
    output->wined3d_output = wined3d_adapter_get_output(adapter->wined3d_adapter, output_idx);
    wined3d_private_store_init(&output->private_store);
    output->adapter = adapter;
    IWineDXGIAdapter_AddRef(&output->adapter->IWineDXGIAdapter_iface);
}

HRESULT dxgi_output_create(struct dxgi_adapter *adapter, unsigned int output_idx,
        struct dxgi_output **output)
{
    if (!(*output = calloc(1, sizeof(**output))))
        return E_OUTOFMEMORY;

    dxgi_output_init(*output, output_idx, adapter);
    return S_OK;
}
