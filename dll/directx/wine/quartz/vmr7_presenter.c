/*
 * Default allocator-presenter for the VMR7
 *
 * Copyright 2023 Zebediah Figura for CodeWeavers
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

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(quartz);

struct vmr7_presenter
{
    IVMRImagePresenter IVMRImagePresenter_iface;
    IVMRSurfaceAllocator IVMRSurfaceAllocator_iface;
    IVMRWindowlessControl IVMRWindowlessControl_iface;
    LONG refcount;

    IDirectDraw7 *ddraw;
    IDirectDrawSurface7 *frontbuffer;
    IDirectDrawSurface7 *primary;
    HWND window;

    SIZE native_size, aspect_ratio;
};

static struct vmr7_presenter *impl_from_IVMRImagePresenter(IVMRImagePresenter *iface)
{
    return CONTAINING_RECORD(iface, struct vmr7_presenter, IVMRImagePresenter_iface);
}

static HRESULT WINAPI image_presenter_QueryInterface(IVMRImagePresenter *iface, REFIID iid, void **out)
{
    struct vmr7_presenter *presenter = impl_from_IVMRImagePresenter(iface);

    TRACE("presenter %p, iid %s, out %p.\n", presenter, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_IUnknown) || IsEqualGUID(iid, &IID_IVMRImagePresenter))
        *out = iface;
    else if (IsEqualGUID(iid, &IID_IVMRSurfaceAllocator))
        *out = &presenter->IVMRSurfaceAllocator_iface;
    else if (IsEqualGUID(iid, &IID_IVMRWindowlessControl))
        *out = &presenter->IVMRWindowlessControl_iface;
    else
    {
        *out = NULL;
        WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown *)*out);
    return S_OK;
}

static ULONG WINAPI image_presenter_AddRef(IVMRImagePresenter *iface)
{
    struct vmr7_presenter *presenter = impl_from_IVMRImagePresenter(iface);
    ULONG refcount = InterlockedIncrement(&presenter->refcount);

    TRACE("%p increasing refcount to %lu.\n", presenter, refcount);
    return refcount;
}

static ULONG WINAPI image_presenter_Release(IVMRImagePresenter *iface)
{
    struct vmr7_presenter *presenter = impl_from_IVMRImagePresenter(iface);
    ULONG refcount = InterlockedDecrement(&presenter->refcount);

    TRACE("%p decreasing refcount to %lu.\n", presenter, refcount);
    if (!refcount)
    {
        if (presenter->frontbuffer)
            IDirectDrawSurface7_Release(presenter->frontbuffer);
        IDirectDrawSurface7_Release(presenter->primary);
        IDirectDraw7_Release(presenter->ddraw);
        free(presenter);
    }
    return refcount;
}

static HRESULT WINAPI image_presenter_StartPresenting(IVMRImagePresenter *iface, DWORD_PTR cookie)
{
    FIXME("iface %p, cookie %#Ix, stub!\n", iface, cookie);
    return E_NOTIMPL;
}

static HRESULT WINAPI image_presenter_StopPresenting(IVMRImagePresenter *iface, DWORD_PTR cookie)
{
    FIXME("iface %p, cookie %#Ix, stub!\n", iface, cookie);
    return E_NOTIMPL;
}

static HRESULT WINAPI image_presenter_PresentImage(IVMRImagePresenter *iface,
        DWORD_PTR cookie, VMRPRESENTATIONINFO *info)
{
    struct vmr7_presenter *presenter = impl_from_IVMRImagePresenter(iface);
    POINT point;
    HRESULT hr;
    RECT rect;

    TRACE("iface %p, cookie %#Ix, info %p.\n", iface, cookie, info);

    TRACE("flags %#lx, surface %p, start %s, end %s, aspect ratio %ldx%ld,\n",
            info->dwFlags, info->lpSurf, debugstr_time(info->rtStart),
            debugstr_time(info->rtEnd), info->szAspectRatio.cx, info->szAspectRatio.cy);
    TRACE("src %s, dst %s, type-specific flags %#lx, interlace flags %#lx.\n",
            wine_dbgstr_rect(&info->rcSrc), wine_dbgstr_rect(&info->rcDst),
            info->dwTypeSpecificFlags, info->dwInterlaceFlags);

    if (info->dwFlags & VMRSample_SrcDstRectsValid)
        FIXME("Ignoring src/dst rects.\n");

    GetClientRect(presenter->window, &rect);
    point.x = point.y = 0;
    ClientToScreen(presenter->window, &point);
    OffsetRect(&rect, point.x, point.y);
    if (FAILED(hr = IDirectDrawSurface7_Blt(presenter->primary, &rect, info->lpSurf, NULL, DDBLT_WAIT, NULL)))
        ERR("Failed to blit, hr %#lx.\n", hr);

    return S_OK;
}

static const IVMRImagePresenterVtbl image_presenter_vtbl =
{
    image_presenter_QueryInterface,
    image_presenter_AddRef,
    image_presenter_Release,
    image_presenter_StartPresenting,
    image_presenter_StopPresenting,
    image_presenter_PresentImage,
};

static struct vmr7_presenter *impl_from_IVMRSurfaceAllocator(IVMRSurfaceAllocator *iface)
{
    return CONTAINING_RECORD(iface, struct vmr7_presenter, IVMRSurfaceAllocator_iface);
}

static HRESULT WINAPI surface_allocator_QueryInterface(IVMRSurfaceAllocator *iface, REFIID iid, void **out)
{
    struct vmr7_presenter *presenter = impl_from_IVMRSurfaceAllocator(iface);

    return IVMRImagePresenter_QueryInterface(&presenter->IVMRImagePresenter_iface, iid, out);
}

static ULONG WINAPI surface_allocator_AddRef(IVMRSurfaceAllocator *iface)
{
    struct vmr7_presenter *presenter = impl_from_IVMRSurfaceAllocator(iface);

    return IVMRImagePresenter_AddRef(&presenter->IVMRImagePresenter_iface);
}

static ULONG WINAPI surface_allocator_Release(IVMRSurfaceAllocator *iface)
{
    struct vmr7_presenter *presenter = impl_from_IVMRSurfaceAllocator(iface);

    return IVMRImagePresenter_Release(&presenter->IVMRImagePresenter_iface);
}

static HRESULT WINAPI surface_allocator_AllocateSurface(IVMRSurfaceAllocator *iface,
        DWORD_PTR id, VMRALLOCATIONINFO *info, DWORD *count, IDirectDrawSurface7 **surface)
{
    struct vmr7_presenter *presenter = impl_from_IVMRSurfaceAllocator(iface);
    DDSURFACEDESC2 surface_desc = {.dwSize = sizeof(surface_desc)};
    HRESULT hr;

    TRACE("presenter %p, id %#Ix, info %p, count %p, surface %p.\n", presenter, id, info, count, surface);

    surface_desc.dwFlags = DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT | DDSD_CAPS | DDSD_BACKBUFFERCOUNT;
    surface_desc.dwWidth = info->lpHdr->biWidth;
    surface_desc.dwHeight = info->lpHdr->biHeight;
    surface_desc.ddpfPixelFormat.dwSize = sizeof(surface_desc.ddpfPixelFormat);
    surface_desc.ddsCaps.dwCaps = DDSCAPS_FLIP | DDSCAPS_COMPLEX | DDSCAPS_OFFSCREENPLAIN;
    surface_desc.dwBackBufferCount = *count;

    if (info->lpHdr->biCompression == BI_RGB)
    {
        if (info->lpHdr->biBitCount != 32)
        {
            FIXME("Unhandled bit depth %u.\n", info->lpHdr->biBitCount);
            return E_NOTIMPL;
        }

        surface_desc.ddpfPixelFormat.dwFlags = DDPF_RGB;
        surface_desc.ddpfPixelFormat.dwRGBBitCount = 32;
        surface_desc.ddpfPixelFormat.dwRBitMask = 0x00ff0000;
        surface_desc.ddpfPixelFormat.dwGBitMask = 0x0000ff00;
        surface_desc.ddpfPixelFormat.dwBBitMask = 0x000000ff;
    }
    else
    {
        surface_desc.ddpfPixelFormat.dwFlags = DDPF_FOURCC;
        surface_desc.ddpfPixelFormat.dwFourCC = info->lpHdr->biCompression;
    }

    if (FAILED(hr = IDirectDraw7_CreateSurface(presenter->ddraw,
            &surface_desc, &presenter->frontbuffer, NULL)))
    {
        WARN("Failed to create surface, hr %#lx.\n", hr);
        return hr;
    }
    *surface = presenter->frontbuffer;
    ++*count;

    presenter->native_size = info->szNativeSize;
    presenter->aspect_ratio = info->szAspectRatio;

    return S_OK;
}

static HRESULT WINAPI surface_allocator_FreeSurface(IVMRSurfaceAllocator *iface, DWORD_PTR id)
{
    struct vmr7_presenter *presenter = impl_from_IVMRSurfaceAllocator(iface);
    DDSURFACEDESC2 surface_desc = {.dwSize = sizeof(surface_desc)};

    TRACE("presenter %p, id %#Ix.\n", presenter, id);

    if (presenter->frontbuffer)
    {
        IDirectDrawSurface7_Release(presenter->frontbuffer);
        presenter->frontbuffer = NULL;
    }

    return S_OK;
}

static HRESULT WINAPI surface_allocator_PrepareSurface(IVMRSurfaceAllocator *iface,
        DWORD_PTR id, IDirectDrawSurface7 *surface, DWORD flags)
{
    TRACE("iface %p, id %#Ix, surface %p, flags %#lx.\n", iface, id, surface, flags);
    return S_OK;
}

static HRESULT WINAPI surface_allocator_AdviseNotify(IVMRSurfaceAllocator *iface,
        IVMRSurfaceAllocatorNotify *notify)
{
    FIXME("iface %p, notify %p, stub!\n", iface, notify);
    return S_OK;
}

static const IVMRSurfaceAllocatorVtbl surface_allocator_vtbl =
{
    surface_allocator_QueryInterface,
    surface_allocator_AddRef,
    surface_allocator_Release,
    surface_allocator_AllocateSurface,
    surface_allocator_FreeSurface,
    surface_allocator_PrepareSurface,
    surface_allocator_AdviseNotify,
};

static struct vmr7_presenter *impl_from_IVMRWindowlessControl(IVMRWindowlessControl *iface)
{
    return CONTAINING_RECORD(iface, struct vmr7_presenter, IVMRWindowlessControl_iface);
}

static HRESULT WINAPI windowless_control_QueryInterface(IVMRWindowlessControl *iface, REFIID iid, void **out)
{
    struct vmr7_presenter *presenter = impl_from_IVMRWindowlessControl(iface);

    return IVMRImagePresenter_QueryInterface(&presenter->IVMRImagePresenter_iface, iid, out);
}

static ULONG WINAPI windowless_control_AddRef(IVMRWindowlessControl *iface)
{
    struct vmr7_presenter *presenter = impl_from_IVMRWindowlessControl(iface);

    return IVMRImagePresenter_AddRef(&presenter->IVMRImagePresenter_iface);
}

static ULONG WINAPI windowless_control_Release(IVMRWindowlessControl *iface)
{
    struct vmr7_presenter *presenter = impl_from_IVMRWindowlessControl(iface);

    return IVMRImagePresenter_Release(&presenter->IVMRImagePresenter_iface);
}

static HRESULT WINAPI windowless_control_GetNativeVideoSize(IVMRWindowlessControl *iface,
        LONG *width, LONG *height, LONG *aspect_width, LONG *aspect_height)
{
    struct vmr7_presenter *presenter = impl_from_IVMRWindowlessControl(iface);

    TRACE("iface %p, width %p, height %p, aspect_width %p, aspect_height %p.\n",
            iface, width, height, aspect_width, aspect_height);

    if (width)
        *width = presenter->native_size.cx;
    if (height)
        *height = presenter->native_size.cy;
    if (aspect_width)
        *aspect_width = presenter->aspect_ratio.cx;
    if (aspect_height)
        *aspect_height = presenter->aspect_ratio.cy;

    TRACE("Returning size (%ld, %ld), aspect ratio (%ld, %ld).\n",
            presenter->native_size.cx, presenter->native_size.cy,
            presenter->aspect_ratio.cx, presenter->aspect_ratio.cy);
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
    FIXME("iface %p, source %s, dest %s, stub!.\n", iface, wine_dbgstr_rect(source), wine_dbgstr_rect(dest));
    return E_NOTIMPL;
}

static HRESULT WINAPI windowless_control_GetVideoPosition(
        IVMRWindowlessControl *iface, RECT *source, RECT *dest)
{
    FIXME("iface %p, source %p, dest %p.\n", iface, source, dest);
    return E_NOTIMPL;
}

static HRESULT WINAPI windowless_control_GetAspectRatioMode(
        IVMRWindowlessControl *iface, DWORD *mode)
{
    FIXME("iface %p, mode %p, stub!\n", iface, mode);
    return E_NOTIMPL;
}

static HRESULT WINAPI windowless_control_SetAspectRatioMode(
        IVMRWindowlessControl *iface, DWORD mode)
{
    FIXME("iface %p, mode %#lx, stub!\n", iface, mode);
    return E_NOTIMPL;
}

static HRESULT WINAPI windowless_control_SetVideoClippingWindow(
        IVMRWindowlessControl *iface, HWND window)
{
    struct vmr7_presenter *presenter = impl_from_IVMRWindowlessControl(iface);
    IDirectDrawClipper *clipper;
    HRESULT hr;

    TRACE("iface %p, window %p.\n", iface, window);

    if (FAILED(hr = IDirectDraw7_CreateClipper(presenter->ddraw, 0, &clipper, NULL)))
        ERR("Failed to create clipper, hr %#lx.\n", hr);
    if (FAILED(hr = IDirectDrawClipper_SetHWnd(clipper, 0, window)))
        ERR("Failed to set clip window, hr %#lx.\n", hr);
    if (FAILED(hr = IDirectDrawSurface7_SetClipper(presenter->primary, clipper)))
        ERR("Failed to set clipper, hr %#lx.\n", hr);
    IDirectDrawClipper_Release(clipper);

    presenter->window = window;

    return S_OK;
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

HRESULT vmr7_presenter_create(IUnknown *outer, IUnknown **out)
{
    struct vmr7_presenter *object;
    HRESULT hr;

    DDSURFACEDESC2 primary_desc =
    {
        .dwSize = sizeof(DDSURFACEDESC2),
        .dwFlags = DDSD_CAPS,
        .ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE,
    };

    TRACE("outer %p, out %p.\n", outer, out);

    if (outer)
        FIXME("Ignoring outer %p.\n", outer);

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;
    object->IVMRImagePresenter_iface.lpVtbl = &image_presenter_vtbl;
    object->IVMRSurfaceAllocator_iface.lpVtbl = &surface_allocator_vtbl;
    object->IVMRWindowlessControl_iface.lpVtbl = &windowless_control_vtbl;
    object->refcount = 1;

    if (FAILED(hr = DirectDrawCreateEx(NULL, (void **)&object->ddraw, &IID_IDirectDraw7, NULL)))
    {
        ERR("Failed to create ddraw object, hr %#lx.\n", hr);
        free(object);
        return hr;
    }

    if (FAILED(hr = IDirectDraw7_SetCooperativeLevel(object->ddraw, NULL, DDSCL_NORMAL)))
        ERR("Failed to set cooperative level, hr %#lx.\n", hr);

    if (FAILED(hr = IDirectDraw7_CreateSurface(object->ddraw, &primary_desc, &object->primary, NULL)))
        ERR("Failed to create primary surface, hr %#lx.\n", hr);

    TRACE("Created VMR7 default presenter %p.\n", object);
    *out = (IUnknown *)&object->IVMRSurfaceAllocator_iface;
    return S_OK;
}
