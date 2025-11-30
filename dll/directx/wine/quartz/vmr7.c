/*
 * Video Mixing Renderer for DirectDraw 7
 *
 * Copyright 2004 Christian Costa
 * Copyright 2008 Maarten Lankhorst
 * Copyright 2012 Aric Stewart
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

#include "quartz_private.h"

#include "uuids.h"
#include "vfwmsgs.h"
#include "amvideo.h"
#include "windef.h"
#include "winbase.h"
#include "dshow.h"
#include "evcode.h"
#include "strmif.h"
#include "ddraw.h"
#include "dvdmedia.h"
#include "d3d9.h"
#include "videoacc.h"
#include "vmr9.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(quartz);

static const BITMAPINFOHEADER *get_bitmap_header(const AM_MEDIA_TYPE *mt)
{
    if (IsEqualGUID(&mt->formattype, &FORMAT_VideoInfo))
        return &((VIDEOINFOHEADER *)mt->pbFormat)->bmiHeader;
    else
        return &((VIDEOINFOHEADER2 *)mt->pbFormat)->bmiHeader;
}

struct vmr7
{
    struct strmbase_renderer renderer;
    struct video_window window;

    IAMCertifiedOutputProtection IAMCertifiedOutputProtection_iface;
    IAMFilterMiscFlags IAMFilterMiscFlags_iface;
    IVMRFilterConfig IVMRFilterConfig_iface;
    IVMRMonitorConfig IVMRMonitorConfig_iface;
    IVMRSurfaceAllocatorNotify IVMRSurfaceAllocatorNotify_iface;
    IVMRWindowlessControl IVMRWindowlessControl_iface;

    IAMVideoAccelerator IAMVideoAccelerator_iface;
    IOverlay IOverlay_iface;

    DWORD stream_count;
    DWORD mixing_prefs;

    VMRMode mode;

    IVMRSurfaceAllocator *allocator;
    IVMRImagePresenter *presenter;
    IDirectDrawSurface7 **surfaces;
    DWORD surface_count;
    DWORD surface_index;
    DWORD_PTR cookie;

    HWND clipping_window;

    VMR9AspectRatioMode aspect_mode;
};

static const BITMAPINFOHEADER *get_filter_bitmap_header(const struct vmr7 *filter)
{
    return get_bitmap_header(&filter->renderer.sink.pin.mt);
}

static struct vmr7 *impl_from_video_window(struct video_window *iface)
{
    return CONTAINING_RECORD(iface, struct vmr7, window);
}

static struct vmr7 *impl_from_IAMCertifiedOutputProtection(IAMCertifiedOutputProtection *iface)
{
    return CONTAINING_RECORD(iface, struct vmr7, IAMCertifiedOutputProtection_iface);
}

static struct vmr7 *impl_from_IAMFilterMiscFlags(IAMFilterMiscFlags *iface)
{
    return CONTAINING_RECORD(iface, struct vmr7, IAMFilterMiscFlags_iface);
}

static struct vmr7 *impl_from_IVMRFilterConfig(IVMRFilterConfig *iface)
{
    return CONTAINING_RECORD(iface, struct vmr7, IVMRFilterConfig_iface);
}

static struct vmr7 *impl_from_IVMRMonitorConfig(IVMRMonitorConfig *iface)
{
    return CONTAINING_RECORD(iface, struct vmr7, IVMRMonitorConfig_iface);
}

static struct vmr7 *impl_from_IVMRSurfaceAllocatorNotify(IVMRSurfaceAllocatorNotify *iface)
{
    return CONTAINING_RECORD(iface, struct vmr7, IVMRSurfaceAllocatorNotify_iface);
}

static struct vmr7 *impl_from_IVMRWindowlessControl(IVMRWindowlessControl *iface)
{
    return CONTAINING_RECORD(iface, struct vmr7, IVMRWindowlessControl_iface);
}

static struct vmr7 *impl_from_IBaseFilter(IBaseFilter *iface)
{
    return CONTAINING_RECORD(iface, struct vmr7, renderer.filter.IBaseFilter_iface);
}

static void copy_plane(BYTE **dstp, unsigned int dst_pitch, unsigned int dst_height,
                       const BYTE **srcp, unsigned int src_pitch, int src_height)
{
    size_t copy_size = min(src_pitch, dst_pitch);
    const BYTE *src = *srcp;
    BYTE *dst = *dstp;
    unsigned int i;

    if (src_height < 0)
    {
        TRACE("Inverting image.\n");

        src_height = -src_height;
        src += src_height * src_pitch;

        for (i = 0; i < src_height; ++i)
        {
            src -= src_pitch;
            memcpy(dst, src, copy_size);
            dst += dst_pitch;
        }
    }
    else
    {
        for (i = 0; i < src_height; ++i)
        {
            memcpy(dst, src, copy_size);
            dst += dst_pitch;
            src += src_pitch;
        }
    }

    *srcp += src_pitch * src_height;
    *dstp += dst_pitch * dst_height;
}

static HRESULT vmr_render(struct strmbase_renderer *iface, IMediaSample *sample)
{
    struct vmr7 *filter = impl_from_IBaseFilter(&iface->filter.IBaseFilter_iface);
    unsigned int data_size, width, depth, src_pitch;
    const BITMAPINFOHEADER *bitmap_header;
    REFERENCE_TIME start_time, end_time;
    VMRPRESENTATIONINFO info = {0};
    DDSURFACEDESC2 surface_desc;
    BYTE *data = NULL;
    HRESULT hr;
    int height;

    TRACE("filter %p, sample %p.\n", filter, sample);

    if (!filter->allocator || !filter->presenter)
    {
        ERR("No presenter.\n");
        return S_FALSE;
    }

    info.dwFlags = VMR9Sample_SrcDstRectsValid;

    if (SUCCEEDED(hr = IMediaSample_GetTime(sample, &start_time, &end_time)))
        info.dwFlags |= VMR9Sample_TimeValid;

    if (IMediaSample_IsDiscontinuity(sample) == S_OK)
        info.dwFlags |= VMR9Sample_Discontinuity;

    if (IMediaSample_IsPreroll(sample) == S_OK)
        info.dwFlags |= VMR9Sample_Preroll;

    if (IMediaSample_IsSyncPoint(sample) == S_OK)
        info.dwFlags |= VMR9Sample_SyncPoint;

    if (FAILED(hr = IMediaSample_GetPointer(sample, &data)))
    {
        ERR("Failed to get pointer to sample data, hr %#lx.\n", hr);
        return hr;
    }
    data_size = IMediaSample_GetActualDataLength(sample);
    TRACE("Sample size %u.\n", data_size);

    bitmap_header = get_filter_bitmap_header(filter);
    width = bitmap_header->biWidth;
    height = bitmap_header->biHeight;
    depth = bitmap_header->biBitCount;
    if (bitmap_header->biCompression == mmioFOURCC('N','V','1','2')
            || bitmap_header->biCompression == mmioFOURCC('Y','V','1','2'))
        src_pitch = (width + 3) & ~3;
    else /* packed YUV (UYVY or YUY2) or RGB */
        src_pitch = ((width * depth / 8) + 3) & ~3;

    info.dwFlags = VMRSample_TimeValid;
    info.rtStart = start_time;
    info.rtEnd = end_time;
    info.szAspectRatio.cx = width;
    info.szAspectRatio.cy = height;
    info.lpSurf = filter->surfaces[(--filter->surface_index) % filter->surface_count];

    hr = IVMRSurfaceAllocator_PrepareSurface(filter->allocator, filter->cookie, info.lpSurf, 0);
    if (hr != S_OK)
        ERR("PrepareSurface() returned %#lx.\n", hr);

    surface_desc.dwSize = sizeof(surface_desc);
    if (FAILED(hr = IDirectDrawSurface7_Lock(info.lpSurf, NULL,
            &surface_desc, DDLOCK_DISCARDCONTENTS | DDLOCK_WAIT, NULL)))
    {
        ERR("Failed to lock surface, hr %#lx.\n", hr);
        return hr;
    }

    if (width > surface_desc.dwWidth || abs(height)  > surface_desc.dwHeight)
    {
        FIXME("src surface (%ux%u) larger than rendering surface (%lux%lu).\n", width, height,
                surface_desc.dwWidth, surface_desc.dwHeight);
    }

    if (bitmap_header->biCompression == mmioFOURCC('N','V','1','2'))
    {
        BYTE *dst = surface_desc.lpSurface;
        const BYTE *src = data;

        copy_plane(&dst, surface_desc.lPitch, surface_desc.dwHeight, &src, src_pitch, height);
        copy_plane(&dst, surface_desc.lPitch, surface_desc.dwHeight / 2, &src, src_pitch, height / 2);
    }
    else if (bitmap_header->biCompression == mmioFOURCC('Y','V','1','2'))
    {
        BYTE *dst = surface_desc.lpSurface;
        const BYTE *src = data;

        copy_plane(&dst, surface_desc.lPitch, surface_desc.dwHeight, &src, src_pitch, height);
        copy_plane(&dst, surface_desc.lPitch / 2, surface_desc.dwHeight / 2, &src, src_pitch / 2, height / 2);
        copy_plane(&dst, surface_desc.lPitch / 2, surface_desc.dwHeight / 2, &src, src_pitch / 2, height / 2);
    }
    else if (height > 0 && bitmap_header->biCompression == BI_RGB)
    {
        BYTE *dst = surface_desc.lpSurface;
        const BYTE *src = data;

        copy_plane(&dst, surface_desc.lPitch, surface_desc.dwHeight, &src, src_pitch, -height);
    }
    else if (surface_desc.lPitch != src_pitch)
    {
        BYTE *dst = surface_desc.lpSurface;
        const BYTE *src = data;

        TRACE("Source pitch %u does not match dest pitch %lu; copying manually.\n",
                src_pitch, surface_desc.lPitch);

        height = abs(height);
        copy_plane(&dst, surface_desc.lPitch, surface_desc.dwHeight, &src, src_pitch, height);
    }
    else
    {
        memcpy(surface_desc.lpSurface, data, data_size);
    }

    IDirectDrawSurface7_Unlock(info.lpSurf, NULL);

    return IVMRImagePresenter_PresentImage(filter->presenter, filter->cookie, &info);
}

static HRESULT vmr_query_accept(struct strmbase_renderer *iface, const AM_MEDIA_TYPE *mt)
{
    if (!IsEqualIID(&mt->majortype, &MEDIATYPE_Video) || !mt->pbFormat)
        return S_FALSE;

    if (!IsEqualGUID(&mt->formattype, &FORMAT_VideoInfo)
            && !IsEqualGUID(&mt->formattype, &FORMAT_VideoInfo2))
        return S_FALSE;

    return S_OK;
}

static HRESULT initialize_device(struct vmr7 *filter, VMRALLOCATIONINFO *info, DWORD count)
{
    IDirectDrawSurface7 *frontbuffer, *prev;
    IVMRWindowlessControl *control;
    HRESULT hr;

    if (FAILED(hr = IVMRSurfaceAllocator_AllocateSurface(filter->allocator,
            filter->cookie, info, &count, &frontbuffer)))
    {
        WARN("Failed to allocate surface, hr %#lx.\n", hr);
        return hr;
    }

    if (filter->mode != VMRMode_Renderless)
    {
        IVMRSurfaceAllocator_QueryInterface(filter->allocator, &IID_IVMRWindowlessControl, (void **)&control);
        IVMRWindowlessControl_SetVideoClippingWindow(control,
                filter->mode == VMRMode_Windowed ? filter->window.hwnd : filter->clipping_window);
        IVMRWindowlessControl_Release(control);
    }

    if (!(filter->surfaces = calloc(count, sizeof(IDirectDrawSurface7 *))))
        return E_OUTOFMEMORY;
    filter->surface_count = count;
    filter->surface_index = 0;

    prev = frontbuffer;
    for (DWORD i = 0; i < count; ++i)
    {
        DDSCAPS2 caps = {.dwCaps = DDSCAPS_FLIP};

        if (FAILED(hr = IDirectDrawSurface7_GetAttachedSurface(prev, &caps, &filter->surfaces[i])))
        {
            ERR("Failed to get surface %lu, hr %#lx.\n", i, hr);
            while (i--)
                IDirectDrawSurface7_Release(filter->surfaces[i]);
            IVMRSurfaceAllocator_FreeSurface(filter->allocator, filter->cookie);
            return hr;
        }
    }

    return hr;
}

static HRESULT allocate_surfaces(struct vmr7 *filter, const AM_MEDIA_TYPE *mt)
{
    BITMAPINFOHEADER bitmap_header = *get_bitmap_header(mt);
    VMRALLOCATIONINFO info = {0};
    HRESULT hr = E_FAIL;
    DWORD count = 1;

    TRACE("Initializing in mode %u, our window %p, clipping window %p.\n",
            filter->mode, filter->window.hwnd, filter->clipping_window);

    if (filter->mode == VMRMode_Windowless && !filter->clipping_window)
        return S_OK;

    info.dwFlags = AMAP_DIRECTED_FLIP | AMAP_ALLOW_SYSMEM;
    info.lpHdr = &bitmap_header;
    info.dwMinBuffers = info.dwMaxBuffers = count;
    info.szAspectRatio.cx = info.szNativeSize.cx = bitmap_header.biWidth;
    info.szAspectRatio.cy = info.szNativeSize.cy = bitmap_header.biHeight;

    if (FAILED(hr = initialize_device(filter, &info, count)))
        free(filter->surfaces);
    return hr;
}

static void vmr_init_stream(struct strmbase_renderer *iface)
{
    struct vmr7 *filter = impl_from_IBaseFilter(&iface->filter.IBaseFilter_iface);

    if (filter->window.hwnd && filter->window.AutoShow)
        ShowWindow(filter->window.hwnd, SW_SHOW);
}

static void vmr_start_stream(struct strmbase_renderer *iface)
{
    struct vmr7 *filter = impl_from_IBaseFilter(&iface->filter.IBaseFilter_iface);

    IVMRImagePresenter_StartPresenting(filter->presenter, filter->cookie);
}

static void vmr_stop_stream(struct strmbase_renderer *iface)
{
    struct vmr7 *filter = impl_from_IBaseFilter(&iface->filter.IBaseFilter_iface);

    if (filter->renderer.filter.state == State_Running)
        IVMRImagePresenter_StopPresenting(filter->presenter, filter->cookie);
}

static HRESULT vmr_connect(struct strmbase_renderer *iface, const AM_MEDIA_TYPE *mt)
{
    struct vmr7 *filter = impl_from_IBaseFilter(&iface->filter.IBaseFilter_iface);
    const BITMAPINFOHEADER *bitmap_header = get_bitmap_header(mt);
    HWND window = filter->window.hwnd;
    HRESULT hr;
    RECT rect;

    SetRect(&rect, 0, 0, bitmap_header->biWidth, bitmap_header->biHeight);
    filter->window.src = rect;

    AdjustWindowRectEx(&rect, GetWindowLongW(window, GWL_STYLE), FALSE,
            GetWindowLongW(window, GWL_EXSTYLE));
    SetWindowPos(window, NULL, 0, 0, rect.right - rect.left, rect.bottom - rect.top,
            SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
    GetClientRect(window, &filter->window.dst);

    if (filter->mode
            || SUCCEEDED(hr = IVMRFilterConfig_SetRenderingMode(&filter->IVMRFilterConfig_iface, VMRMode_Windowed)))
        hr = allocate_surfaces(filter, mt);

    return hr;
}

static void deallocate_surfaces(struct vmr7 *filter)
{
    if (filter->mode && filter->allocator && filter->presenter)
    {
        for (DWORD i = 0; i < filter->surface_count; ++i)
            IDirectDrawSurface7_Release(filter->surfaces[i]);
        free(filter->surfaces);

        IVMRSurfaceAllocator_FreeSurface(filter->allocator, filter->cookie);
        filter->surface_count = 0;
    }
}

static void vmr_disconnect(struct strmbase_renderer *iface)
{
    struct vmr7 *filter = impl_from_IBaseFilter(&iface->filter.IBaseFilter_iface);

    deallocate_surfaces(filter);
}

static void vmr_destroy(struct strmbase_renderer *iface)
{
    struct vmr7 *filter = impl_from_IBaseFilter(&iface->filter.IBaseFilter_iface);

    video_window_cleanup(&filter->window);

    if (filter->allocator)
    {
        if (filter->surface_count)
            IVMRSurfaceAllocator_FreeSurface(filter->allocator, filter->cookie);
        IVMRSurfaceAllocator_Release(filter->allocator);
    }
    if (filter->presenter)
        IVMRImagePresenter_Release(filter->presenter);

    filter->surface_count = 0;

    strmbase_renderer_cleanup(&filter->renderer);
    free(filter);
}

static HRESULT vmr_query_interface(struct strmbase_renderer *iface, REFIID iid, void **out)
{
    struct vmr7 *filter = impl_from_IBaseFilter(&iface->filter.IBaseFilter_iface);

    if (IsEqualGUID(iid, &IID_IVideoWindow))
        *out = &filter->window.IVideoWindow_iface;
    else if (IsEqualGUID(iid, &IID_IBasicVideo))
        *out = &filter->window.IBasicVideo_iface;
    else if (IsEqualGUID(iid, &IID_IAMCertifiedOutputProtection))
        *out = &filter->IAMCertifiedOutputProtection_iface;
    else if (IsEqualGUID(iid, &IID_IAMFilterMiscFlags))
        *out = &filter->IAMFilterMiscFlags_iface;
    else if (IsEqualGUID(iid, &IID_IVMRFilterConfig))
        *out = &filter->IVMRFilterConfig_iface;
    else if (IsEqualGUID(iid, &IID_IVMRMonitorConfig))
        *out = &filter->IVMRMonitorConfig_iface;
    else if (IsEqualGUID(iid, &IID_IVMRSurfaceAllocatorNotify) && filter->mode == VMRMode_Renderless)
        *out = &filter->IVMRSurfaceAllocatorNotify_iface;
    else if (IsEqualGUID(iid, &IID_IVMRWindowlessControl) && filter->mode == VMRMode_Windowless)
        *out = &filter->IVMRWindowlessControl_iface;
    else
        return E_NOINTERFACE;

    IUnknown_AddRef((IUnknown *)*out);
    return S_OK;
}

static HRESULT vmr_pin_query_interface(struct strmbase_renderer *iface, REFIID iid, void **out)
{
    struct vmr7 *filter = impl_from_IBaseFilter(&iface->filter.IBaseFilter_iface);

    if (IsEqualGUID(iid, &IID_IAMVideoAccelerator))
        *out = &filter->IAMVideoAccelerator_iface;
    else if (IsEqualGUID(iid, &IID_IOverlay))
        *out = &filter->IOverlay_iface;
    else
        return E_NOINTERFACE;

    IUnknown_AddRef((IUnknown *)*out);
    return S_OK;
}

static const struct strmbase_renderer_ops renderer_ops =
{
    .renderer_query_accept = vmr_query_accept,
    .renderer_render = vmr_render,
    .renderer_init_stream = vmr_init_stream,
    .renderer_start_stream = vmr_start_stream,
    .renderer_stop_stream = vmr_stop_stream,
    .renderer_connect = vmr_connect,
    .renderer_disconnect = vmr_disconnect,
    .renderer_destroy = vmr_destroy,
    .renderer_query_interface = vmr_query_interface,
    .renderer_pin_query_interface = vmr_pin_query_interface,
};

static void vmr_get_default_rect(struct video_window *iface, RECT *rect)
{
    struct vmr7 *filter = impl_from_video_window(iface);
    const BITMAPINFOHEADER *bitmap_header = get_filter_bitmap_header(filter);

    SetRect(rect, 0, 0, bitmap_header->biWidth, bitmap_header->biHeight);
}

static HRESULT vmr_get_current_image(struct video_window *iface, LONG *size, LONG *image)
{
    FIXME("stub!\n");
    return E_NOTIMPL;
}

static const struct video_window_ops window_ops =
{
    .get_default_rect = vmr_get_default_rect,
    .get_current_image = vmr_get_current_image,
};

static const IVideoWindowVtbl IVideoWindow_VTable =
{
    BaseControlWindowImpl_QueryInterface,
    BaseControlWindowImpl_AddRef,
    BaseControlWindowImpl_Release,
    BaseControlWindowImpl_GetTypeInfoCount,
    BaseControlWindowImpl_GetTypeInfo,
    BaseControlWindowImpl_GetIDsOfNames,
    BaseControlWindowImpl_Invoke,
    BaseControlWindowImpl_put_Caption,
    BaseControlWindowImpl_get_Caption,
    BaseControlWindowImpl_put_WindowStyle,
    BaseControlWindowImpl_get_WindowStyle,
    BaseControlWindowImpl_put_WindowStyleEx,
    BaseControlWindowImpl_get_WindowStyleEx,
    BaseControlWindowImpl_put_AutoShow,
    BaseControlWindowImpl_get_AutoShow,
    BaseControlWindowImpl_put_WindowState,
    BaseControlWindowImpl_get_WindowState,
    BaseControlWindowImpl_put_BackgroundPalette,
    BaseControlWindowImpl_get_BackgroundPalette,
    BaseControlWindowImpl_put_Visible,
    BaseControlWindowImpl_get_Visible,
    BaseControlWindowImpl_put_Left,
    BaseControlWindowImpl_get_Left,
    BaseControlWindowImpl_put_Width,
    BaseControlWindowImpl_get_Width,
    BaseControlWindowImpl_put_Top,
    BaseControlWindowImpl_get_Top,
    BaseControlWindowImpl_put_Height,
    BaseControlWindowImpl_get_Height,
    BaseControlWindowImpl_put_Owner,
    BaseControlWindowImpl_get_Owner,
    BaseControlWindowImpl_put_MessageDrain,
    BaseControlWindowImpl_get_MessageDrain,
    BaseControlWindowImpl_get_BorderColor,
    BaseControlWindowImpl_put_BorderColor,
    BaseControlWindowImpl_get_FullScreenMode,
    BaseControlWindowImpl_put_FullScreenMode,
    BaseControlWindowImpl_SetWindowForeground,
    BaseControlWindowImpl_NotifyOwnerMessage,
    BaseControlWindowImpl_SetWindowPosition,
    BaseControlWindowImpl_GetWindowPosition,
    BaseControlWindowImpl_GetMinIdealImageSize,
    BaseControlWindowImpl_GetMaxIdealImageSize,
    BaseControlWindowImpl_GetRestorePosition,
    BaseControlWindowImpl_HideCursor,
    BaseControlWindowImpl_IsCursorHidden,
};

static HRESULT WINAPI certified_output_protection_QueryInterface(
        IAMCertifiedOutputProtection *iface, REFIID iid, void **out)
{
    struct vmr7 *filter = impl_from_IAMCertifiedOutputProtection(iface);

    return IUnknown_QueryInterface(filter->renderer.filter.outer_unk, iid, out);
}

static ULONG WINAPI certified_output_protection_AddRef(IAMCertifiedOutputProtection *iface)
{
    struct vmr7 *filter = impl_from_IAMCertifiedOutputProtection(iface);

    return IUnknown_AddRef(filter->renderer.filter.outer_unk);
}

static ULONG WINAPI certified_output_protection_Release(IAMCertifiedOutputProtection *iface)
{
    struct vmr7 *filter = impl_from_IAMCertifiedOutputProtection(iface);

    return IUnknown_Release(filter->renderer.filter.outer_unk);
}

static HRESULT WINAPI certified_output_protection_KeyExchange(
        IAMCertifiedOutputProtection *iface, GUID *random, BYTE **certificate, DWORD *size)
{
    FIXME("iface %p, random %p, certificate %p, size %p, stub!\n", iface, random, certificate, size);
    return VFW_E_NO_COPP_HW;
}

static HRESULT WINAPI certified_output_protection_SessionSequenceStart(
        IAMCertifiedOutputProtection *iface, AMCOPPSignature *signature)
{
    FIXME("iface %p, signature %p, stub!\n", iface, signature);
    return VFW_E_NO_COPP_HW;
}

static HRESULT WINAPI certified_output_protection_ProtectionCommand(
        IAMCertifiedOutputProtection *iface, const AMCOPPCommand *cmd)
{
    FIXME("iface %p, cmd %p, stub!\n", iface, cmd);
    return VFW_E_NO_COPP_HW;
}

static HRESULT WINAPI certified_output_protection_ProtectionStatus(
        IAMCertifiedOutputProtection *iface, const AMCOPPStatusInput *input, AMCOPPStatusOutput *output)
{
    FIXME("iface %p, input %p, output %p, stub!\n", iface, input, output);
    return VFW_E_NO_COPP_HW;
}

static const IAMCertifiedOutputProtectionVtbl certified_output_protection_vtbl =
{
    certified_output_protection_QueryInterface,
    certified_output_protection_AddRef,
    certified_output_protection_Release,
    certified_output_protection_KeyExchange,
    certified_output_protection_SessionSequenceStart,
    certified_output_protection_ProtectionCommand,
    certified_output_protection_ProtectionStatus,
};

static HRESULT WINAPI misc_flags_QueryInterface(IAMFilterMiscFlags *iface, REFIID iid, void **out)
{
    struct vmr7 *filter = impl_from_IAMFilterMiscFlags(iface);

    return IUnknown_QueryInterface(filter->renderer.filter.outer_unk, iid, out);
}

static ULONG WINAPI misc_flags_AddRef(IAMFilterMiscFlags *iface)
{
    struct vmr7 *filter = impl_from_IAMFilterMiscFlags(iface);

    return IUnknown_AddRef(filter->renderer.filter.outer_unk);
}

static ULONG WINAPI misc_flags_Release(IAMFilterMiscFlags *iface)
{
    struct vmr7 *filter = impl_from_IAMFilterMiscFlags(iface);

    return IUnknown_Release(filter->renderer.filter.outer_unk);
}

static ULONG WINAPI misc_flags_GetMiscFlags(IAMFilterMiscFlags *iface)
{
    return AM_FILTER_MISC_FLAGS_IS_RENDERER;
}

static const IAMFilterMiscFlagsVtbl misc_flags_vtbl =
{
    misc_flags_QueryInterface,
    misc_flags_AddRef,
    misc_flags_Release,
    misc_flags_GetMiscFlags,
};

static HRESULT WINAPI filter_config_QueryInterface(IVMRFilterConfig *iface, REFIID iid, void** out)
{
    struct vmr7 *filter = impl_from_IVMRFilterConfig(iface);

    return IUnknown_QueryInterface(filter->renderer.filter.outer_unk, iid, out);
}

static ULONG WINAPI filter_config_AddRef(IVMRFilterConfig *iface)
{
    struct vmr7 *filter = impl_from_IVMRFilterConfig(iface);

    return IUnknown_AddRef(filter->renderer.filter.outer_unk);
}

static ULONG WINAPI filter_config_Release(IVMRFilterConfig *iface)
{
    struct vmr7 *filter = impl_from_IVMRFilterConfig(iface);

    return IUnknown_Release(filter->renderer.filter.outer_unk);
}

static HRESULT WINAPI filter_config_SetImageCompositor(
        IVMRFilterConfig *iface, IVMRImageCompositor *compositor)
{
    FIXME("iface %p, compositor %p, stub!\n", iface, compositor);
    return E_NOTIMPL;
}

static HRESULT WINAPI filter_config_SetNumberOfStreams(IVMRFilterConfig *iface, DWORD count)
{
    struct vmr7 *filter = impl_from_IVMRFilterConfig(iface);

    FIXME("iface %p, count %lu, stub!\n", iface, count);

    if (!count)
    {
        WARN("Application requested zero streams; returning E_INVALIDARG.\n");
        return E_INVALIDARG;
    }

    EnterCriticalSection(&filter->renderer.filter.filter_cs);

    if (filter->stream_count)
    {
        LeaveCriticalSection(&filter->renderer.filter.filter_cs);
        WARN("Stream count is already set; returning VFW_E_WRONG_STATE.\n");
        return VFW_E_WRONG_STATE;
    }

    filter->stream_count = count;

    LeaveCriticalSection(&filter->renderer.filter.filter_cs);
    return S_OK;
}

static HRESULT WINAPI filter_config_GetNumberOfStreams(IVMRFilterConfig *iface, DWORD *count)
{
    FIXME("iface %p, count %p, stub!\n", iface, count);
    return E_NOTIMPL;
}

static HRESULT WINAPI filter_config_SetRenderingPrefs(IVMRFilterConfig *iface, DWORD flags)
{
    FIXME("iface %p, flags %#lx, stub!\n", iface, flags);
    return E_NOTIMPL;
}

static HRESULT WINAPI filter_config_GetRenderingPrefs(IVMRFilterConfig *iface, DWORD *flags)
{
    FIXME("iface %p, flags %p, stub!\n", iface, flags);
    return E_NOTIMPL;
}

static HRESULT WINAPI filter_config_SetRenderingMode(IVMRFilterConfig *iface, DWORD mode)
{
    struct vmr7 *filter = impl_from_IVMRFilterConfig(iface);
    IUnknown *default_presenter;
    HRESULT hr = S_OK;

    TRACE("filter %p, mode %lu.\n", filter, mode);

    EnterCriticalSection(&filter->renderer.filter.filter_cs);

    if (filter->mode)
    {
        LeaveCriticalSection(&filter->renderer.filter.filter_cs);
        return VFW_E_WRONG_STATE;
    }

    switch (mode)
    {
        case VMRMode_Windowed:
        case VMRMode_Windowless:
            if (FAILED(hr = vmr7_presenter_create(NULL, &default_presenter)))
            {
                ERR("Failed to create default presenter, hr %#lx.\n", hr);
                LeaveCriticalSection(&filter->renderer.filter.filter_cs);
                return hr;
            }
            IUnknown_QueryInterface(default_presenter, &IID_IVMRSurfaceAllocator, (void **)&filter->allocator);
            IUnknown_QueryInterface(default_presenter, &IID_IVMRImagePresenter, (void **)&filter->presenter);
            IUnknown_Release(default_presenter);
            break;

        case VMRMode_Renderless:
            break;

        default:
            LeaveCriticalSection(&filter->renderer.filter.filter_cs);
            return E_INVALIDARG;
    }

    if (mode != VMRMode_Windowed)
        video_window_cleanup(&filter->window);

    filter->mode = mode;

    LeaveCriticalSection(&filter->renderer.filter.filter_cs);
    return hr;
}

static HRESULT WINAPI filter_config_GetRenderingMode(IVMRFilterConfig *iface, DWORD *mode)
{
    struct vmr7 *filter = impl_from_IVMRFilterConfig(iface);

    TRACE("filter %p, mode %p.\n", filter, mode);

    if (!mode)
        return E_POINTER;

    if (filter->mode)
        *mode = filter->mode;
    else
        *mode = VMRMode_Windowed;

    return S_OK;
}

static const IVMRFilterConfigVtbl filter_config_vtbl =
{
    filter_config_QueryInterface,
    filter_config_AddRef,
    filter_config_Release,
    filter_config_SetImageCompositor,
    filter_config_SetNumberOfStreams,
    filter_config_GetNumberOfStreams,
    filter_config_SetRenderingPrefs,
    filter_config_GetRenderingPrefs,
    filter_config_SetRenderingMode,
    filter_config_GetRenderingMode,
};

struct get_available_monitors_args
{
    VMRMONITORINFO *info7;
    VMR9MonitorInfo *info9;
    DWORD capacity;
    DWORD count;
};

static BOOL CALLBACK get_available_monitors_proc(HMONITOR monitor, HDC dc, RECT *rect, LPARAM ctx)
{
    struct get_available_monitors_args *args = (struct get_available_monitors_args *)ctx;
    MONITORINFOEXW mi;

    if (args->info7 || args->info9)
    {

        if (!args->capacity)
            return FALSE;

        mi.cbSize = sizeof(mi);
        if (!GetMonitorInfoW(monitor, (MONITORINFO*)&mi))
            return TRUE;

        /* fill VMRMONITORINFO struct */
        if (args->info7)
        {
            VMRMONITORINFO *info = args->info7++;
            memset(info, 0, sizeof(*info));

            if (args->count > 0)
            {
                info->guid.pGUID = &info->guid.GUID;
                info->guid.GUID.Data4[7] = args->count;
            }
            else
                info->guid.pGUID = NULL;

            info->rcMonitor     = mi.rcMonitor;
            info->hMon          = monitor;
            info->dwFlags       = mi.dwFlags;

            lstrcpynW(info->szDevice, mi.szDevice, ARRAY_SIZE(info->szDevice));

            /* FIXME: how to get these values? */
            info->szDescription[0] = 0;
        }

        /* fill VMR9MonitorInfo struct */
        if (args->info9)
        {
            VMR9MonitorInfo *info = args->info9++;
            memset(info, 0, sizeof(*info));

            info->uDevID        = 0; /* FIXME */
            info->rcMonitor     = mi.rcMonitor;
            info->hMon          = monitor;
            info->dwFlags       = mi.dwFlags;

            lstrcpynW(info->szDevice, mi.szDevice, ARRAY_SIZE(info->szDevice));

            /* FIXME: how to get these values? */
            info->szDescription[0] = 0;
            info->dwVendorId    = 0;
            info->dwDeviceId    = 0;
            info->dwSubSysId    = 0;
            info->dwRevision    = 0;
        }

        args->capacity--;
    }

    args->count++;
    return TRUE;
}

static HRESULT WINAPI monitor_config_QueryInterface(IVMRMonitorConfig *iface, REFIID iid, void **out)
{
    struct vmr7 *filter = impl_from_IVMRMonitorConfig(iface);

    return IUnknown_QueryInterface(filter->renderer.filter.outer_unk, iid, out);
}

static ULONG WINAPI monitor_config_AddRef(IVMRMonitorConfig *iface)
{
    struct vmr7 *filter = impl_from_IVMRMonitorConfig(iface);

    return IUnknown_AddRef(filter->renderer.filter.outer_unk);
}

static ULONG WINAPI monitor_config_Release(IVMRMonitorConfig *iface)
{
    struct vmr7 *filter = impl_from_IVMRMonitorConfig(iface);

    return IUnknown_Release(filter->renderer.filter.outer_unk);
}

static HRESULT WINAPI monitor_config_SetMonitor(IVMRMonitorConfig *iface, const VMRGUID *guid)
{
    FIXME("iface %p, guid %p, stub!\n", iface, guid);

    if (!guid)
        return E_POINTER;

    return S_OK;
}

static HRESULT WINAPI monitor_config_GetMonitor(IVMRMonitorConfig *iface, VMRGUID *guid)
{
    FIXME("iface %p, guid %p, stub!\n", iface, guid);

    if (!guid)
        return E_POINTER;

    guid->pGUID = NULL; /* default DirectDraw device */
    return S_OK;
}

static HRESULT WINAPI monitor_config_SetDefaultMonitor(IVMRMonitorConfig *iface, const VMRGUID *guid)
{
    FIXME("iface %p, guid %p, stub!\n", iface, guid);

    if (!guid)
        return E_POINTER;

    return S_OK;
}

static HRESULT WINAPI monitor_config_GetDefaultMonitor(IVMRMonitorConfig *iface, VMRGUID *guid)
{
    FIXME("iface %p, guid %p, stub!\n", iface, guid);

    if (!guid)
        return E_POINTER;

    guid->pGUID = NULL; /* default DirectDraw device */
    return S_OK;
}

static HRESULT WINAPI monitor_config_GetAvailableMonitors(IVMRMonitorConfig *iface,
        VMRMONITORINFO *info, DWORD capacity, DWORD *count)
{
    struct vmr7 *filter = impl_from_IVMRMonitorConfig(iface);
    struct get_available_monitors_args args;

    TRACE("filter %p, info %p, capacity %lu, count %p.\n", filter, info, capacity, count);

    if (!count)
        return E_POINTER;

    if (info && !capacity)
        return E_INVALIDARG;

    args.info7 = info;
    args.info9 = NULL;
    args.capacity = capacity;
    args.count = 0;
    EnumDisplayMonitors(NULL, NULL, get_available_monitors_proc, (LPARAM)&args);

    *count = args.count;
    return S_OK;
}

static const IVMRMonitorConfigVtbl monitor_config_vtbl =
{
    monitor_config_QueryInterface,
    monitor_config_AddRef,
    monitor_config_Release,
    monitor_config_SetMonitor,
    monitor_config_GetMonitor,
    monitor_config_SetDefaultMonitor,
    monitor_config_GetDefaultMonitor,
    monitor_config_GetAvailableMonitors,
};

static HRESULT WINAPI windowless_control_QueryInterface(IVMRWindowlessControl *iface, REFIID iid, void **out)
{
    struct vmr7 *filter = impl_from_IVMRWindowlessControl(iface);

    return IUnknown_QueryInterface(filter->renderer.filter.outer_unk, iid, out);
}

static ULONG WINAPI windowless_control_AddRef(IVMRWindowlessControl *iface)
{
    struct vmr7 *filter = impl_from_IVMRWindowlessControl(iface);

    return IUnknown_AddRef(filter->renderer.filter.outer_unk);
}

static ULONG WINAPI windowless_control_Release(IVMRWindowlessControl *iface)
{
    struct vmr7 *filter = impl_from_IVMRWindowlessControl(iface);

    return IUnknown_Release(filter->renderer.filter.outer_unk);
}

static HRESULT WINAPI windowless_control_GetNativeVideoSize(IVMRWindowlessControl *iface,
        LONG *width, LONG *height, LONG *aspect_width, LONG *aspect_height)
{
    struct vmr7 *filter = impl_from_IVMRWindowlessControl(iface);
    const BITMAPINFOHEADER *bitmap_header = get_filter_bitmap_header(filter);

    TRACE("filter %p, width %p, height %p, aspect_width %p, aspect_height %p.\n",
            filter, width, height, aspect_width, aspect_height);

    if (!width || !height)
        return E_POINTER;

    *width = bitmap_header->biWidth;
    *height = bitmap_header->biHeight;
    if (aspect_width)
        *aspect_width = bitmap_header->biWidth;
    if (aspect_height)
        *aspect_height = bitmap_header->biHeight;

    return S_OK;
}

static HRESULT WINAPI windowless_control_GetMinIdealVideoSize(
        IVMRWindowlessControl *iface, LONG *width, LONG *height)
{
    FIXME("iface %p, width %p, height %p, stub!\n", iface, width, height);
    return E_NOTIMPL;
}

static HRESULT WINAPI windowless_control_GetMaxIdealVideoSize(
        IVMRWindowlessControl *iface, LONG *width, LONG *height)
{
    FIXME("iface %p, width %p, height %p, stub!\n", iface, width, height);
    return E_NOTIMPL;
}

static HRESULT WINAPI windowless_control_SetVideoPosition(
        IVMRWindowlessControl *iface, const RECT *source, const RECT *dest)
{
    struct vmr7 *filter = impl_from_IVMRWindowlessControl(iface);

    TRACE("filter %p, source %s, dest %s.\n", filter, wine_dbgstr_rect(source), wine_dbgstr_rect(dest));

    EnterCriticalSection(&filter->renderer.filter.filter_cs);

    if (source)
        filter->window.src = *source;
    if (dest)
        filter->window.dst = *dest;

    LeaveCriticalSection(&filter->renderer.filter.filter_cs);

    return S_OK;
}

static HRESULT WINAPI windowless_control_GetVideoPosition(
        IVMRWindowlessControl *iface, RECT *source, RECT *dest)
{
    struct vmr7 *filter = impl_from_IVMRWindowlessControl(iface);

    TRACE("filter %p, source %p, dest %p.\n", filter, source, dest);

    if (source)
        *source = filter->window.src;

    if (dest)
        *dest = filter->window.dst;

    return S_OK;
}

static HRESULT WINAPI windowless_control_GetAspectRatioMode(IVMRWindowlessControl *iface, DWORD *mode)
{
    FIXME("iface %p, mode %p, stub!\n", iface, mode);
    return E_NOTIMPL;
}

static HRESULT WINAPI windowless_control_SetAspectRatioMode(IVMRWindowlessControl *iface, DWORD mode)
{
    FIXME("iface %p, mode %#lx, stub!\n", iface, mode);
    return E_NOTIMPL;
}

static HRESULT WINAPI windowless_control_SetVideoClippingWindow(IVMRWindowlessControl *iface, HWND window)
{
    struct vmr7 *filter = impl_from_IVMRWindowlessControl(iface);
    HRESULT hr;

    TRACE("filter %p, window %p.\n", filter, window);

    if (!IsWindow(window))
    {
        WARN("Invalid window %p, returning E_INVALIDARG.\n", window);
        return E_INVALIDARG;
    }

    EnterCriticalSection(&filter->renderer.filter.filter_cs);

    if (filter->renderer.sink.pin.peer)
    {
        LeaveCriticalSection(&filter->renderer.filter.filter_cs);
        WARN("Attempt to set the clipping window while connected; returning VFW_E_WRONG_STATE.\n");
        return VFW_E_WRONG_STATE;
    }

    filter->clipping_window = window;

    hr = IVMRFilterConfig_SetNumberOfStreams(&filter->IVMRFilterConfig_iface, 4);

    LeaveCriticalSection(&filter->renderer.filter.filter_cs);
    return hr;
}

static HRESULT WINAPI windowless_control_RepaintVideo(
        IVMRWindowlessControl *iface, HWND window, HDC dc)
{
    FIXME("iface %p, window %p, dc %p, stub!\n", iface, window, dc);
    return E_NOTIMPL;
}

static HRESULT WINAPI windowless_control_DisplayModeChanged(IVMRWindowlessControl *iface)
{
    FIXME("iface %p, stub!\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI windowless_control_GetCurrentImage(IVMRWindowlessControl *iface, BYTE **image)
{
    FIXME("iface %p, image %p, stub!\n", iface, image);
    return E_NOTIMPL;
}

static HRESULT WINAPI windowless_control_SetBorderColor(IVMRWindowlessControl *iface, COLORREF color)
{
    FIXME("iface %p, color %#08lx, stub!\n", iface, color);
    return E_NOTIMPL;
}

static HRESULT WINAPI windowless_control_GetBorderColor(IVMRWindowlessControl *iface, COLORREF *color)
{
    FIXME("iface %p, color %p, stub!\n", iface, color);
    return E_NOTIMPL;
}

static HRESULT WINAPI windowless_control_SetColorKey(IVMRWindowlessControl *iface, COLORREF color)
{
    FIXME("iface %p, color %#08lx, stub!\n", iface, color);
    return E_NOTIMPL;
}

static HRESULT WINAPI windowless_control_GetColorKey(IVMRWindowlessControl *iface, COLORREF *color)
{
    FIXME("iface %p, color %p, stub!\n", iface, color);
    return E_NOTIMPL;
}

static const IVMRWindowlessControlVtbl windowless_control_vtbl =
{
    windowless_control_QueryInterface,
    windowless_control_AddRef,
    windowless_control_Release,
    windowless_control_GetNativeVideoSize,
    windowless_control_GetMinIdealVideoSize,
    windowless_control_GetMaxIdealVideoSize,
    windowless_control_SetVideoPosition,
    windowless_control_GetVideoPosition,
    windowless_control_GetAspectRatioMode,
    windowless_control_SetAspectRatioMode,
    windowless_control_SetVideoClippingWindow,
    windowless_control_RepaintVideo,
    windowless_control_DisplayModeChanged,
    windowless_control_GetCurrentImage,
    windowless_control_SetBorderColor,
    windowless_control_GetBorderColor,
    windowless_control_SetColorKey,
    windowless_control_GetColorKey,
};

static HRESULT WINAPI surface_allocator_notify_QueryInterface(
        IVMRSurfaceAllocatorNotify *iface, REFIID iid, void **out)
{
    struct vmr7 *filter = impl_from_IVMRSurfaceAllocatorNotify(iface);

    return IUnknown_QueryInterface(filter->renderer.filter.outer_unk, iid, out);
}

static ULONG WINAPI surface_allocator_notify_AddRef(IVMRSurfaceAllocatorNotify *iface)
{
    struct vmr7 *filter = impl_from_IVMRSurfaceAllocatorNotify(iface);

    return IUnknown_AddRef(filter->renderer.filter.outer_unk);
}

static ULONG WINAPI surface_allocator_notify_Release(IVMRSurfaceAllocatorNotify *iface)
{
    struct vmr7 *filter = impl_from_IVMRSurfaceAllocatorNotify(iface);

    return IUnknown_Release(filter->renderer.filter.outer_unk);
}

static HRESULT WINAPI surface_allocator_notify_AdviseSurfaceAllocator(
        IVMRSurfaceAllocatorNotify *iface, DWORD_PTR cookie, IVMRSurfaceAllocator *allocator)
{
    struct vmr7 *filter = impl_from_IVMRSurfaceAllocatorNotify(iface);
    IVMRImagePresenter *presenter;

    TRACE("filter %p, cookie %#Ix, allocator %p.\n", filter, cookie, allocator);

    EnterCriticalSection(&filter->renderer.filter.filter_cs);

    filter->cookie = cookie;

    if (filter->renderer.sink.pin.peer)
    {
        LeaveCriticalSection(&filter->renderer.filter.filter_cs);
        WARN("Attempt to set allocator while connected; returning VFW_E_WRONG_STATE.\n");
        return VFW_E_WRONG_STATE;
    }

    if (FAILED(IVMRSurfaceAllocator_QueryInterface(allocator, &IID_IVMRImagePresenter, (void **)&presenter)))
    {
        LeaveCriticalSection(&filter->renderer.filter.filter_cs);
        return E_NOINTERFACE;
    }

    if (filter->allocator)
    {
        IVMRImagePresenter_Release(filter->presenter);
        IVMRSurfaceAllocator_Release(filter->allocator);
    }
    filter->allocator = allocator;
    filter->presenter = presenter;
    IVMRSurfaceAllocator_AddRef(allocator);

    LeaveCriticalSection(&filter->renderer.filter.filter_cs);
    return S_OK;
}

static HRESULT WINAPI surface_allocator_notify_SetDDrawDevice(
        IVMRSurfaceAllocatorNotify *iface, IDirectDraw7 *device, HMONITOR monitor)
{
    FIXME("iface %p, device %p, monitor %p, stub!\n", iface, device, monitor);
    return E_NOTIMPL;
}

static HRESULT WINAPI surface_allocator_notify_ChangeDDrawDevice(
        IVMRSurfaceAllocatorNotify *iface, IDirectDraw7 *device, HMONITOR monitor)
{
    FIXME("iface %p, device %p, monitor %p, stub!\n", iface, device, monitor);
    return E_NOTIMPL;
}

static HRESULT WINAPI surface_allocator_notify_RestoreDDrawSurfaces(IVMRSurfaceAllocatorNotify *iface)
{
    FIXME("iface %p, stub!\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI surface_allocator_notify_NotifyEvent(
        IVMRSurfaceAllocatorNotify *iface, LONG code, LONG_PTR param1, LONG_PTR param2)
{
    FIXME("iface %p, code %#lx, param1 %#Ix, param2 %#Ix, stub!\n", iface, code, param1, param2);
    return E_NOTIMPL;
}

static HRESULT WINAPI surface_allocator_notify_SetBorderColor(
        IVMRSurfaceAllocatorNotify *iface, COLORREF color)
{
    FIXME("iface %p, color %#08lx, stub!\n", iface, color);
    return E_NOTIMPL;
}

static const IVMRSurfaceAllocatorNotifyVtbl surface_allocator_notify_vtbl =
{
    surface_allocator_notify_QueryInterface,
    surface_allocator_notify_AddRef,
    surface_allocator_notify_Release,
    surface_allocator_notify_AdviseSurfaceAllocator,
    surface_allocator_notify_SetDDrawDevice,
    surface_allocator_notify_ChangeDDrawDevice,
    surface_allocator_notify_RestoreDDrawSurfaces,
    surface_allocator_notify_NotifyEvent,
    surface_allocator_notify_SetBorderColor,
};

static struct vmr7 *impl_from_IAMVideoAccelerator(IAMVideoAccelerator *iface)
{
    return CONTAINING_RECORD(iface, struct vmr7, IAMVideoAccelerator_iface);
}

static HRESULT WINAPI video_accelerator_QueryInterface(IAMVideoAccelerator *iface, REFIID iid, void **out)
{
    struct vmr7 *filter = impl_from_IAMVideoAccelerator(iface);

    return IPin_QueryInterface(&filter->renderer.sink.pin.IPin_iface, iid, out);
}

static ULONG WINAPI video_accelerator_AddRef(IAMVideoAccelerator *iface)
{
    struct vmr7 *filter = impl_from_IAMVideoAccelerator(iface);

    return IPin_AddRef(&filter->renderer.sink.pin.IPin_iface);
}

static ULONG WINAPI video_accelerator_Release(IAMVideoAccelerator *iface)
{
    struct vmr7 *filter = impl_from_IAMVideoAccelerator(iface);

    return IPin_Release(&filter->renderer.sink.pin.IPin_iface);
}

static HRESULT WINAPI video_accelerator_GetVideoAcceleratorGUIDs(
        IAMVideoAccelerator *iface, DWORD *count, GUID *accelerators)
{
    FIXME("iface %p, count %p, accelerators %p, stub!\n", iface, count, accelerators);
    return E_NOTIMPL;
}

static HRESULT WINAPI video_accelerator_GetUncompFormatsSupported(IAMVideoAccelerator *iface,
        const GUID *accelerator, DWORD *count, DDPIXELFORMAT *formats)
{
    FIXME("iface %p, accelerator %s, count %p, formats %p, stub!\n",
            iface, debugstr_guid(accelerator), count, formats);
    return E_NOTIMPL;
}

static HRESULT WINAPI video_accelerator_GetInternalMemInfo(IAMVideoAccelerator *iface,
        const GUID *accelerator, const AMVAUncompDataInfo *format_info, AMVAInternalMemInfo *mem_info)
{
    FIXME("iface %p, accelerator %s, format_info %p, mem_info %p, stub!\n",
            iface, debugstr_guid(accelerator), format_info, mem_info);
    return E_NOTIMPL;
}

static HRESULT WINAPI video_accelerator_GetCompBufferInfo(IAMVideoAccelerator *iface,
        const GUID *accelerator, const AMVAUncompDataInfo *uncompressed_info,
        DWORD *compressed_info_count, AMVACompBufferInfo *compressed_infos)
{
    FIXME("iface %p, accelerator %s, uncompressed_info %p, compressed_info_count %p, compressed_infos %p, stub!\n",
            iface, debugstr_guid(accelerator), uncompressed_info, compressed_info_count, compressed_infos);
    return E_NOTIMPL;
}

static HRESULT WINAPI video_accelerator_GetInternalCompBufferInfo(
        IAMVideoAccelerator *iface, DWORD *count, AMVACompBufferInfo *infos)
{
    FIXME("iface %p, count %p, infos %p, stub!\n", iface, count, infos);
    return E_NOTIMPL;
}

static HRESULT WINAPI video_accelerator_BeginFrame(IAMVideoAccelerator *iface, const AMVABeginFrameInfo *info)
{
    FIXME("iface %p, info %p, stub!\n", iface, info);
    return E_NOTIMPL;
}

static HRESULT WINAPI video_accelerator_EndFrame(IAMVideoAccelerator *iface, const AMVAEndFrameInfo *info)
{
    FIXME("iface %p, info %p, stub!\n", iface, info);
    return E_NOTIMPL;
}

static HRESULT WINAPI video_accelerator_GetBuffer(IAMVideoAccelerator *iface,
        DWORD type_index, DWORD buffer_index, BOOL read_only, void **buffer, LONG *stride)
{
    FIXME("iface %p, type_index %lu, buffer_index %lu, read_only %d, buffer %p, stride %p, stub!\n",
            iface, type_index, buffer_index, read_only, buffer, stride);
    return E_NOTIMPL;
}

static HRESULT WINAPI video_accelerator_ReleaseBuffer(
        IAMVideoAccelerator *iface, DWORD type_index, DWORD buffer_index)
{
    FIXME("iface %p, type_index %lu, buffer_index %lu, stub!\n", iface, type_index, buffer_index);
    return E_NOTIMPL;
}

static HRESULT WINAPI video_accelerator_Execute(IAMVideoAccelerator *iface,
        DWORD function, void *in_data, DWORD in_size, void *out_data,
        DWORD out_size, DWORD buffer_count, const AMVABUFFERINFO *buffers)
{
    FIXME("iface %p, function %#lx, in_data %p, in_size %lu,"
            " out_data %p, out_size %lu, buffer_count %lu, buffers %p, stub!\n",
            iface, function, in_data, in_size, out_data, out_size, buffer_count, buffers);
    return E_NOTIMPL;
}

static HRESULT WINAPI video_accelerator_QueryRenderStatus(IAMVideoAccelerator *iface,
        DWORD type_index, DWORD buffer_index, DWORD flags)
{
    FIXME("iface %p, type_index %lu, buffer_index %lu, flags %#lx, stub!\n",
            iface, type_index, buffer_index, flags);
    return E_NOTIMPL;
}

static HRESULT WINAPI video_accelerator_DisplayFrame(
        IAMVideoAccelerator *iface, DWORD index, IMediaSample *sample)
{
    FIXME("iface %p, index %lu, sample %p, stub!\n", iface, index, sample);
    return E_NOTIMPL;
}

static const IAMVideoAcceleratorVtbl video_accelerator_vtbl =
{
    video_accelerator_QueryInterface,
    video_accelerator_AddRef,
    video_accelerator_Release,
    video_accelerator_GetVideoAcceleratorGUIDs,
    video_accelerator_GetUncompFormatsSupported,
    video_accelerator_GetInternalMemInfo,
    video_accelerator_GetCompBufferInfo,
    video_accelerator_GetInternalCompBufferInfo,
    video_accelerator_BeginFrame,
    video_accelerator_EndFrame,
    video_accelerator_GetBuffer,
    video_accelerator_ReleaseBuffer,
    video_accelerator_Execute,
    video_accelerator_QueryRenderStatus,
    video_accelerator_DisplayFrame,
};

static struct vmr7 *impl_from_IOverlay(IOverlay *iface)
{
    return CONTAINING_RECORD(iface, struct vmr7, IOverlay_iface);
}

static HRESULT WINAPI overlay_QueryInterface(IOverlay *iface, REFIID iid, void **out)
{
    struct vmr7 *filter = impl_from_IOverlay(iface);

    return IPin_QueryInterface(&filter->renderer.sink.pin.IPin_iface, iid, out);
}

static ULONG WINAPI overlay_AddRef(IOverlay *iface)
{
    struct vmr7 *filter = impl_from_IOverlay(iface);

    return IPin_AddRef(&filter->renderer.sink.pin.IPin_iface);
}

static ULONG WINAPI overlay_Release(IOverlay *iface)
{
    struct vmr7 *filter = impl_from_IOverlay(iface);

    return IPin_Release(&filter->renderer.sink.pin.IPin_iface);
}

static HRESULT WINAPI overlay_GetPalette(IOverlay *iface, DWORD *count, PALETTEENTRY **palette)
{
    FIXME("iface %p, count %p, palette %p, stub!\n", iface, count, palette);
    return E_NOTIMPL;
}

static HRESULT WINAPI overlay_SetPalette(IOverlay *iface, DWORD count, PALETTEENTRY *palette)
{
    FIXME("iface %p, count %lu, palette %p, stub!\n", iface, count, palette);
    return E_NOTIMPL;
}

static HRESULT WINAPI overlay_GetDefaultColorKey(IOverlay *iface, COLORKEY *key)
{
    FIXME("iface %p, key %p, stub!\n", iface, key);
    return E_NOTIMPL;
}

static HRESULT WINAPI overlay_GetColorKey(IOverlay *iface, COLORKEY *key)
{
    FIXME("iface %p, key %p, stub!\n", iface, key);
    return E_NOTIMPL;
}

static HRESULT WINAPI overlay_SetColorKey(IOverlay *iface, COLORKEY *key)
{
    FIXME("iface %p, key %p, stub!\n", iface, key);
    return E_NOTIMPL;
}

static HRESULT WINAPI overlay_GetWindowHandle(IOverlay *iface, HWND *window)
{
    struct vmr7 *filter = impl_from_IOverlay(iface);

    TRACE("filter %p, window %p.\n", filter, window);

    if (!filter->window.hwnd)
        return VFW_E_WRONG_STATE;

    *window = filter->window.hwnd;
    return S_OK;
}

static HRESULT WINAPI overlay_GetClipList(IOverlay *iface, RECT *source, RECT *dest, RGNDATA **region)
{
    FIXME("iface %p, source %p, dest %p, region %p, stub!\n", iface, source, dest, region);
    return E_NOTIMPL;
}

static HRESULT WINAPI overlay_GetVideoPosition(IOverlay *iface, RECT *source, RECT *dest)
{
    FIXME("iface %p, source %p, dest %p, stub!\n", iface, source, dest);
    return E_NOTIMPL;
}

static HRESULT WINAPI overlay_Advise(IOverlay *iface, IOverlayNotify *sink, DWORD flags)
{
    FIXME("iface %p, sink %p, flags %#lx, stub!\n", iface, sink, flags);
    return E_NOTIMPL;
}

static HRESULT WINAPI overlay_Unadvise(IOverlay *iface)
{
    FIXME("iface %p, stub!\n", iface);
    return E_NOTIMPL;
}

static const IOverlayVtbl overlay_vtbl =
{
    overlay_QueryInterface,
    overlay_AddRef,
    overlay_Release,
    overlay_GetPalette,
    overlay_SetPalette,
    overlay_GetDefaultColorKey,
    overlay_GetColorKey,
    overlay_SetColorKey,
    overlay_GetWindowHandle,
    overlay_GetClipList,
    overlay_GetVideoPosition,
    overlay_Advise,
    overlay_Unadvise,
};

HRESULT vmr7_create(IUnknown *outer, IUnknown **out)
{
    struct vmr7 *object;
    HRESULT hr;

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    strmbase_renderer_init(&object->renderer, outer, &CLSID_VideoMixingRenderer, L"VMR Input0", &renderer_ops);
    object->IAMCertifiedOutputProtection_iface.lpVtbl = &certified_output_protection_vtbl;
    object->IAMFilterMiscFlags_iface.lpVtbl = &misc_flags_vtbl;
    object->IVMRFilterConfig_iface.lpVtbl = &filter_config_vtbl;
    object->IVMRMonitorConfig_iface.lpVtbl = &monitor_config_vtbl;
    object->IVMRSurfaceAllocatorNotify_iface.lpVtbl = &surface_allocator_notify_vtbl;
    object->IVMRWindowlessControl_iface.lpVtbl = &windowless_control_vtbl;

    object->IAMVideoAccelerator_iface.lpVtbl = &video_accelerator_vtbl;
    object->IOverlay_iface.lpVtbl = &overlay_vtbl;

    video_window_init(&object->window, &IVideoWindow_VTable,
            &object->renderer.filter, &object->renderer.sink.pin, &window_ops);

    if (FAILED(hr = video_window_create_window(&object->window)))
    {
        video_window_cleanup(&object->window);
        strmbase_renderer_cleanup(&object->renderer);
        free(object);
        return hr;
    }

    object->mixing_prefs = MixerPref9_NoDecimation | MixerPref9_ARAdjustXorY
            | MixerPref9_BiLinearFiltering | MixerPref9_RenderTargetRGB;

    TRACE("Created VMR %p.\n", object);
    *out = &object->renderer.filter.IUnknown_inner;
    return S_OK;
}
