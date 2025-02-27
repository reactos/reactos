/*
 * Copyright 2014 Henri Verbeet for CodeWeavers
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

#include "d2d1_private.h"
#include "initguid.h"
#include "wincodec.h"

WINE_DEFAULT_DEBUG_CHANNEL(d2d);

static inline struct d2d_wic_render_target *impl_from_IUnknown(IUnknown *iface)
{
    return CONTAINING_RECORD(iface, struct d2d_wic_render_target, IUnknown_iface);
}

static HRESULT d2d_wic_render_target_present(IUnknown *outer_unknown)
{
    struct d2d_wic_render_target *render_target = impl_from_IUnknown(outer_unknown);
    D3D10_MAPPED_TEXTURE2D mapped_texture;
    ID3D10Resource *src_resource;
    IWICBitmapLock *bitmap_lock;
    UINT dst_size, dst_pitch;
    ID3D10Device *device;
    WICRect dst_rect;
    BYTE *src, *dst;
    unsigned int i;
    HRESULT hr;

    if (FAILED(hr = IDXGISurface_QueryInterface(render_target->dxgi_surface,
            &IID_ID3D10Resource, (void **)&src_resource)))
    {
        ERR("Failed to get source resource interface, hr %#lx.\n", hr);
        goto end;
    }

    ID3D10Texture2D_GetDevice(render_target->readback_texture, &device);
    ID3D10Device_CopyResource(device, (ID3D10Resource *)render_target->readback_texture, src_resource);
    ID3D10Device_Release(device);
    ID3D10Resource_Release(src_resource);

    dst_rect.X = 0;
    dst_rect.Y = 0;
    dst_rect.Width = render_target->width;
    dst_rect.Height = render_target->height;
    if (FAILED(hr = IWICBitmap_Lock(render_target->bitmap, &dst_rect, WICBitmapLockWrite, &bitmap_lock)))
    {
        ERR("Failed to lock destination bitmap, hr %#lx.\n", hr);
        goto end;
    }

    if (FAILED(hr = IWICBitmapLock_GetDataPointer(bitmap_lock, &dst_size, &dst)))
    {
        ERR("Failed to get data pointer, hr %#lx.\n", hr);
        IWICBitmapLock_Release(bitmap_lock);
        goto end;
    }

    if (FAILED(hr = IWICBitmapLock_GetStride(bitmap_lock, &dst_pitch)))
    {
        ERR("Failed to get stride, hr %#lx.\n", hr);
        IWICBitmapLock_Release(bitmap_lock);
        goto end;
    }

    if (FAILED(hr = ID3D10Texture2D_Map(render_target->readback_texture, 0, D3D10_MAP_READ, 0, &mapped_texture)))
    {
        ERR("Failed to map readback texture, hr %#lx.\n", hr);
        IWICBitmapLock_Release(bitmap_lock);
        goto end;
    }

    src = mapped_texture.pData;

    for (i = 0; i < render_target->height; ++i)
    {
        memcpy(dst, src, render_target->bpp * render_target->width);
        src += mapped_texture.RowPitch;
        dst += dst_pitch;
    }

    ID3D10Texture2D_Unmap(render_target->readback_texture, 0);
    IWICBitmapLock_Release(bitmap_lock);

end:
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE d2d_wic_render_target_QueryInterface(IUnknown *iface, REFIID iid, void **out)
{
    struct d2d_wic_render_target *render_target = impl_from_IUnknown(iface);

    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    return IUnknown_QueryInterface(render_target->dxgi_inner, iid, out);
}

static ULONG STDMETHODCALLTYPE d2d_wic_render_target_AddRef(IUnknown *iface)
{
    struct d2d_wic_render_target *render_target = impl_from_IUnknown(iface);
    ULONG refcount = InterlockedIncrement(&render_target->refcount);

    TRACE("%p increasing refcount to %lu.\n", iface, refcount);

    return refcount;
}

static ULONG STDMETHODCALLTYPE d2d_wic_render_target_Release(IUnknown *iface)
{
    struct d2d_wic_render_target *render_target = impl_from_IUnknown(iface);
    ULONG refcount = InterlockedDecrement(&render_target->refcount);

    TRACE("%p decreasing refcount to %lu.\n", iface, refcount);

    if (!refcount)
    {
        IWICBitmap_Release(render_target->bitmap);
        ID3D10Texture2D_Release(render_target->readback_texture);
        IUnknown_Release(render_target->dxgi_inner);
        IDXGISurface_Release(render_target->dxgi_surface);
        free(render_target);
    }

    return refcount;
}

static const struct IUnknownVtbl d2d_wic_render_target_vtbl =
{
    d2d_wic_render_target_QueryInterface,
    d2d_wic_render_target_AddRef,
    d2d_wic_render_target_Release,
};

static const struct d2d_device_context_ops d2d_wic_render_target_ops =
{
    d2d_wic_render_target_present,
};

HRESULT d2d_wic_render_target_init(struct d2d_wic_render_target *render_target, ID2D1Factory1 *factory,
        ID3D10Device1 *d3d_device, IWICBitmap *bitmap, const D2D1_RENDER_TARGET_PROPERTIES *desc)
{
    D3D10_TEXTURE2D_DESC texture_desc;
    ID3D10Texture2D *texture;
    IDXGIDevice *dxgi_device;
    ID2D1Device *device;
    HRESULT hr;

    render_target->IUnknown_iface.lpVtbl = &d2d_wic_render_target_vtbl;

    if (FAILED(hr = IWICBitmap_GetSize(bitmap, &render_target->width, &render_target->height)))
    {
        WARN("Failed to get bitmap dimensions, hr %#lx.\n", hr);
        return hr;
    }

    texture_desc.Width = render_target->width;
    texture_desc.Height = render_target->height;
    texture_desc.MipLevels = 1;
    texture_desc.ArraySize = 1;

    texture_desc.Format = desc->pixelFormat.format;
    if (texture_desc.Format == DXGI_FORMAT_UNKNOWN)
    {
        WICPixelFormatGUID bitmap_format;

        if (FAILED(hr = IWICBitmap_GetPixelFormat(bitmap, &bitmap_format)))
        {
            WARN("Failed to get bitmap format, hr %#lx.\n", hr);
            return hr;
        }

        if (IsEqualGUID(&bitmap_format, &GUID_WICPixelFormat32bppPBGRA)
                || IsEqualGUID(&bitmap_format, &GUID_WICPixelFormat32bppBGR))
        {
            texture_desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
        }
        else if (IsEqualGUID(&bitmap_format, &GUID_WICPixelFormat32bppPRGBA)
                || IsEqualGUID(&bitmap_format, &GUID_WICPixelFormat32bppRGB))
        {
            texture_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        }
        else
        {
            WARN("Unsupported WIC bitmap format %s.\n", debugstr_guid(&bitmap_format));
            return D2DERR_UNSUPPORTED_PIXEL_FORMAT;
        }
    }

    switch (texture_desc.Format)
    {
        case DXGI_FORMAT_B8G8R8A8_UNORM:
        case DXGI_FORMAT_R8G8B8A8_UNORM:
            render_target->bpp = 4;
            break;

        default:
            FIXME("Unhandled format %#x.\n", texture_desc.Format);
            return D2DERR_UNSUPPORTED_PIXEL_FORMAT;
    }

    texture_desc.SampleDesc.Count = 1;
    texture_desc.SampleDesc.Quality = 0;
    texture_desc.Usage = D3D10_USAGE_DEFAULT;
    texture_desc.BindFlags = D3D10_BIND_RENDER_TARGET | D3D10_BIND_SHADER_RESOURCE;
    texture_desc.CPUAccessFlags = 0;
    texture_desc.MiscFlags = desc->usage & D2D1_RENDER_TARGET_USAGE_GDI_COMPATIBLE ?
            D3D10_RESOURCE_MISC_GDI_COMPATIBLE : 0;

    if (FAILED(hr = ID3D10Device1_CreateTexture2D(d3d_device, &texture_desc, NULL, &texture)))
    {
        WARN("Failed to create texture, hr %#lx.\n", hr);
        return hr;
    }

    hr = ID3D10Texture2D_QueryInterface(texture, &IID_IDXGISurface, (void **)&render_target->dxgi_surface);
    ID3D10Texture2D_Release(texture);
    if (FAILED(hr))
    {
        WARN("Failed to get DXGI surface interface, hr %#lx.\n", hr);
        return hr;
    }

    texture_desc.Usage = D3D10_USAGE_STAGING;
    texture_desc.BindFlags = 0;
    texture_desc.CPUAccessFlags = D3D10_CPU_ACCESS_READ;
    texture_desc.MiscFlags = 0;

    if (FAILED(hr = ID3D10Device1_CreateTexture2D(d3d_device, &texture_desc, NULL, &render_target->readback_texture)))
    {
        WARN("Failed to create readback texture, hr %#lx.\n", hr);
        IDXGISurface_Release(render_target->dxgi_surface);
        return hr;
    }

    if (FAILED(hr = ID3D10Device1_QueryInterface(d3d_device, &IID_IDXGIDevice, (void **)&dxgi_device)))
    {
        WARN("Failed to get DXGI device, hr %#lx.\n", hr);
        IDXGISurface_Release(render_target->dxgi_surface);
        return hr;
    }

    hr = d2d_factory_create_device(factory, dxgi_device, false, &IID_ID2D1Device, (void **)&device);
    IDXGIDevice_Release(dxgi_device);
    if (FAILED(hr))
    {
        WARN("Failed to create D2D device, hr %#lx.\n", hr);
        IDXGISurface_Release(render_target->dxgi_surface);
        return hr;
    }

    hr = d2d_d3d_create_render_target(unsafe_impl_from_ID2D1Device((ID2D1Device1 *)device),
            render_target->dxgi_surface, &render_target->IUnknown_iface,
            &d2d_wic_render_target_ops, desc, (void **)&render_target->dxgi_inner);
    ID2D1Device_Release(device);
    if (FAILED(hr))
    {
        WARN("Failed to create DXGI surface render target, hr %#lx.\n", hr);
        ID3D10Texture2D_Release(render_target->readback_texture);
        IDXGISurface_Release(render_target->dxgi_surface);
        return hr;
    }

    if (FAILED(hr = IUnknown_QueryInterface(render_target->dxgi_inner,
            &IID_ID2D1RenderTarget, (void **)&render_target->dxgi_target)))
    {
        WARN("Failed to retrieve ID2D1RenderTarget interface, hr %#lx.\n", hr);
        IUnknown_Release(render_target->dxgi_inner);
        ID3D10Texture2D_Release(render_target->readback_texture);
        IDXGISurface_Release(render_target->dxgi_surface);
        return hr;
    }

    render_target->bitmap = bitmap;
    IWICBitmap_AddRef(bitmap);

    return S_OK;
}
