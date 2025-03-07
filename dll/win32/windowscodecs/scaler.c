/*
 * Copyright 2010 Vincent Povirk for CodeWeavers
 * Copyright 2016 Dmitry Timoshkov
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

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "objbase.h"

#include "wincodecs_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(wincodecs);

typedef struct BitmapScaler {
    IWICBitmapScaler IWICBitmapScaler_iface;
    LONG ref;
    IMILBitmapScaler IMILBitmapScaler_iface;
    IWICBitmapSource *source;
    UINT width, height;
    UINT src_width, src_height;
    WICBitmapInterpolationMode mode;
    UINT bpp;
    void (*fn_get_required_source_rect)(struct BitmapScaler*,UINT,UINT,WICRect*);
    void (*fn_copy_scanline)(struct BitmapScaler*,UINT,UINT,UINT,BYTE**,UINT,UINT,BYTE*);
    CRITICAL_SECTION lock; /* must be held when initialized */
} BitmapScaler;

static inline BitmapScaler *impl_from_IWICBitmapScaler(IWICBitmapScaler *iface)
{
    return CONTAINING_RECORD(iface, BitmapScaler, IWICBitmapScaler_iface);
}

static inline BitmapScaler *impl_from_IMILBitmapScaler(IMILBitmapScaler *iface)
{
    return CONTAINING_RECORD(iface, BitmapScaler, IMILBitmapScaler_iface);
}

static HRESULT WINAPI BitmapScaler_QueryInterface(IWICBitmapScaler *iface, REFIID iid,
    void **ppv)
{
    BitmapScaler *This = impl_from_IWICBitmapScaler(iface);
    TRACE("(%p,%s,%p)\n", iface, debugstr_guid(iid), ppv);

    if (!ppv) return E_INVALIDARG;

    if (IsEqualIID(&IID_IUnknown, iid) ||
        IsEqualIID(&IID_IWICBitmapSource, iid) ||
        IsEqualIID(&IID_IWICBitmapScaler, iid))
    {
        *ppv = &This->IWICBitmapScaler_iface;
    }
    else if (IsEqualIID(&IID_IMILBitmapScaler, iid))
    {
        *ppv = &This->IMILBitmapScaler_iface;
    }
    else
    {
        FIXME("unknown interface %s\n", debugstr_guid(iid));
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI BitmapScaler_AddRef(IWICBitmapScaler *iface)
{
    BitmapScaler *This = impl_from_IWICBitmapScaler(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) refcount=%lu\n", iface, ref);

    return ref;
}

static ULONG WINAPI BitmapScaler_Release(IWICBitmapScaler *iface)
{
    BitmapScaler *This = impl_from_IWICBitmapScaler(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) refcount=%lu\n", iface, ref);

    if (ref == 0)
    {
        This->lock.DebugInfo->Spare[0] = 0;
        DeleteCriticalSection(&This->lock);
        if (This->source) IWICBitmapSource_Release(This->source);
        free(This);
    }

    return ref;
}

static HRESULT WINAPI BitmapScaler_GetSize(IWICBitmapScaler *iface,
    UINT *puiWidth, UINT *puiHeight)
{
    BitmapScaler *This = impl_from_IWICBitmapScaler(iface);
    TRACE("(%p,%p,%p)\n", iface, puiWidth, puiHeight);

    if (!This->source)
        return WINCODEC_ERR_NOTINITIALIZED;

    if (!puiWidth || !puiHeight)
        return E_INVALIDARG;

    *puiWidth = This->width;
    *puiHeight = This->height;

    return S_OK;
}

static HRESULT WINAPI BitmapScaler_GetPixelFormat(IWICBitmapScaler *iface,
    WICPixelFormatGUID *pPixelFormat)
{
    BitmapScaler *This = impl_from_IWICBitmapScaler(iface);
    TRACE("(%p,%p)\n", iface, pPixelFormat);

    if (!pPixelFormat)
        return E_INVALIDARG;

    if (!This->source)
    {
        memcpy(pPixelFormat, &GUID_WICPixelFormatDontCare, sizeof(*pPixelFormat));
        return S_OK;
    }

    return IWICBitmapSource_GetPixelFormat(This->source, pPixelFormat);
}

static HRESULT WINAPI BitmapScaler_GetResolution(IWICBitmapScaler *iface,
    double *pDpiX, double *pDpiY)
{
    BitmapScaler *This = impl_from_IWICBitmapScaler(iface);
    TRACE("(%p,%p,%p)\n", iface, pDpiX, pDpiY);

    if (!This->source)
        return WINCODEC_ERR_NOTINITIALIZED;

    if (!pDpiX || !pDpiY)
        return E_INVALIDARG;

    return IWICBitmapSource_GetResolution(This->source, pDpiX, pDpiY);
}

static HRESULT WINAPI BitmapScaler_CopyPalette(IWICBitmapScaler *iface,
    IWICPalette *pIPalette)
{
    BitmapScaler *This = impl_from_IWICBitmapScaler(iface);
    TRACE("(%p,%p)\n", iface, pIPalette);

    if (!pIPalette)
        return E_INVALIDARG;

    if (!This->source)
        return WINCODEC_ERR_PALETTEUNAVAILABLE;

    return IWICBitmapSource_CopyPalette(This->source, pIPalette);
}

static void NearestNeighbor_GetRequiredSourceRect(BitmapScaler *This,
    UINT x, UINT y, WICRect *src_rect)
{
    src_rect->X = x * This->src_width / This->width;
    src_rect->Y = y * This->src_height / This->height;
    src_rect->Width = src_rect->Height = 1;
}

static void NearestNeighbor_CopyScanline(BitmapScaler *This,
    UINT dst_x, UINT dst_y, UINT dst_width,
    BYTE **src_data, UINT src_data_x, UINT src_data_y, BYTE *pbBuffer)
{
    UINT i;
    UINT bytesperpixel = This->bpp/8;
    UINT src_x, src_y;

    src_y = dst_y * This->src_height / This->height - src_data_y;

    for (i=0; i<dst_width; i++)
    {
        src_x = (dst_x + i) * This->src_width / This->width - src_data_x;
        memcpy(pbBuffer + bytesperpixel * i, src_data[src_y] + bytesperpixel * src_x, bytesperpixel);
    }
}

static HRESULT WINAPI BitmapScaler_CopyPixels(IWICBitmapScaler *iface,
    const WICRect *prc, UINT cbStride, UINT cbBufferSize, BYTE *pbBuffer)
{
    BitmapScaler *This = impl_from_IWICBitmapScaler(iface);
    HRESULT hr;
    WICRect dest_rect;
    WICRect src_rect_ul, src_rect_br, src_rect;
    BYTE **src_rows;
    BYTE *src_bits;
    ULONG bytesperrow;
    ULONG src_bytesperrow;
    ULONG buffer_size;
    UINT y;

    TRACE("(%p,%s,%u,%u,%p)\n", iface, debug_wic_rect(prc), cbStride, cbBufferSize, pbBuffer);

    EnterCriticalSection(&This->lock);

    if (!This->source)
    {
        hr = WINCODEC_ERR_NOTINITIALIZED;
        goto end;
    }

    if (prc)
        dest_rect = *prc;
    else
    {
        dest_rect.X = dest_rect.Y = 0;
        dest_rect.Width = This->width;
        dest_rect.Height = This->height;
    }

    if (dest_rect.X < 0 || dest_rect.Y < 0 ||
        dest_rect.X+dest_rect.Width > This->width|| dest_rect.Y+dest_rect.Height > This->height)
    {
        hr = E_INVALIDARG;
        goto end;
    }

    bytesperrow = ((This->bpp * dest_rect.Width)+7)/8;

    if (cbStride < bytesperrow)
    {
        hr = E_INVALIDARG;
        goto end;
    }

    if (cbStride * (dest_rect.Height - 1) + bytesperrow > cbBufferSize)
    {
        hr = E_INVALIDARG;
        goto end;
    }

    /* MSDN recommends calling CopyPixels once for each scanline from top to
     * bottom, and claims codecs optimize for this. Ideally, when called in this
     * way, we should avoid requesting a scanline from the source more than
     * once, by saving the data that will be useful for the next scanline after
     * the call returns. The GetRequiredSourceRect/CopyScanline functions are
     * designed to make it possible to do this in a generic way, but for now we
     * just grab all the data we need in each call. */

    This->fn_get_required_source_rect(This, dest_rect.X, dest_rect.Y, &src_rect_ul);
    This->fn_get_required_source_rect(This, dest_rect.X+dest_rect.Width-1,
        dest_rect.Y+dest_rect.Height-1, &src_rect_br);

    src_rect.X = src_rect_ul.X;
    src_rect.Y = src_rect_ul.Y;
    src_rect.Width = src_rect_br.Width + src_rect_br.X - src_rect_ul.X;
    src_rect.Height = src_rect_br.Height + src_rect_br.Y - src_rect_ul.Y;

    src_bytesperrow = (src_rect.Width * This->bpp + 7)/8;
    buffer_size = src_bytesperrow * src_rect.Height;

    src_rows = malloc(sizeof(BYTE*) * src_rect.Height);
    src_bits = malloc(buffer_size);

    if (!src_rows || !src_bits)
    {
        free(src_rows);
        free(src_bits);
        hr = E_OUTOFMEMORY;
        goto end;
    }

    for (y=0; y<src_rect.Height; y++)
        src_rows[y] = src_bits + y * src_bytesperrow;

    hr = IWICBitmapSource_CopyPixels(This->source, &src_rect, src_bytesperrow,
        buffer_size, src_bits);

    if (SUCCEEDED(hr))
    {
        for (y=0; y < dest_rect.Height; y++)
        {
            This->fn_copy_scanline(This, dest_rect.X, dest_rect.Y+y, dest_rect.Width,
                src_rows, src_rect.X, src_rect.Y, pbBuffer + cbStride * y);
        }
    }

    free(src_rows);
    free(src_bits);

end:
    LeaveCriticalSection(&This->lock);

    return hr;
}

static HRESULT WINAPI BitmapScaler_Initialize(IWICBitmapScaler *iface,
    IWICBitmapSource *pISource, UINT uiWidth, UINT uiHeight,
    WICBitmapInterpolationMode mode)
{
    BitmapScaler *This = impl_from_IWICBitmapScaler(iface);
    HRESULT hr;
    GUID src_pixelformat;

    TRACE("(%p,%p,%u,%u,%u)\n", iface, pISource, uiWidth, uiHeight, mode);

    if (!pISource || !uiWidth || !uiHeight)
        return E_INVALIDARG;

    EnterCriticalSection(&This->lock);

    if (This->source)
    {
        hr = WINCODEC_ERR_WRONGSTATE;
        goto end;
    }

    This->width = uiWidth;
    This->height = uiHeight;
    This->mode = mode;

    hr = IWICBitmapSource_GetSize(pISource, &This->src_width, &This->src_height);

    if (SUCCEEDED(hr))
        hr = IWICBitmapSource_GetPixelFormat(pISource, &src_pixelformat);

    if (SUCCEEDED(hr))
    {
        hr = get_pixelformat_bpp(&src_pixelformat, &This->bpp);
    }

    if (SUCCEEDED(hr))
    {
        switch (mode)
        {
        default:
            FIXME("unsupported mode %i\n", mode);
            /* fall-through */
        case WICBitmapInterpolationModeNearestNeighbor:
            if ((This->bpp % 8) == 0)
            {
                IWICBitmapSource_AddRef(pISource);
                This->source = pISource;
            }
            else
            {
                hr = WICConvertBitmapSource(&GUID_WICPixelFormat32bppBGRA,
                    pISource, &This->source);
                This->bpp = 32;
            }
            This->fn_get_required_source_rect = NearestNeighbor_GetRequiredSourceRect;
            This->fn_copy_scanline = NearestNeighbor_CopyScanline;
            break;
        }
    }

end:
    LeaveCriticalSection(&This->lock);

    return hr;
}

static const IWICBitmapScalerVtbl BitmapScaler_Vtbl = {
    BitmapScaler_QueryInterface,
    BitmapScaler_AddRef,
    BitmapScaler_Release,
    BitmapScaler_GetSize,
    BitmapScaler_GetPixelFormat,
    BitmapScaler_GetResolution,
    BitmapScaler_CopyPalette,
    BitmapScaler_CopyPixels,
    BitmapScaler_Initialize
};

static HRESULT WINAPI IMILBitmapScaler_QueryInterface(IMILBitmapScaler *iface, REFIID iid,
    void **ppv)
{
    BitmapScaler *This = impl_from_IMILBitmapScaler(iface);
    TRACE("(%p,%s,%p)\n", iface, debugstr_guid(iid), ppv);
    return IWICBitmapScaler_QueryInterface(&This->IWICBitmapScaler_iface, iid, ppv);
}

static ULONG WINAPI IMILBitmapScaler_AddRef(IMILBitmapScaler *iface)
{
    BitmapScaler *This = impl_from_IMILBitmapScaler(iface);
    return IWICBitmapScaler_AddRef(&This->IWICBitmapScaler_iface);
}

static ULONG WINAPI IMILBitmapScaler_Release(IMILBitmapScaler *iface)
{
    BitmapScaler *This = impl_from_IMILBitmapScaler(iface);
    return IWICBitmapScaler_Release(&This->IWICBitmapScaler_iface);
}

static HRESULT WINAPI IMILBitmapScaler_GetSize(IMILBitmapScaler *iface,
    UINT *width, UINT *height)
{
    BitmapScaler *This = impl_from_IMILBitmapScaler(iface);
    TRACE("(%p,%p,%p)\n", iface, width, height);
    return IWICBitmapScaler_GetSize(&This->IWICBitmapScaler_iface, width, height);
}

static HRESULT WINAPI IMILBitmapScaler_GetPixelFormat(IMILBitmapScaler *iface,
    int *format)
{
    BitmapScaler *This = impl_from_IMILBitmapScaler(iface);
    IMILBitmapSource *source;
    HRESULT hr;

    TRACE("(%p,%p)\n", iface, format);

    if (!format) return E_INVALIDARG;

    if (!This->source)
        return WINCODEC_ERR_NOTINITIALIZED;

    hr = IWICBitmapSource_QueryInterface(This->source, &IID_IMILBitmapSource, (void **)&source);
    if (hr == S_OK)
    {
        hr = source->lpVtbl->GetPixelFormat(source, format);
        source->lpVtbl->Release(source);
    }
    return hr;
}

static HRESULT WINAPI IMILBitmapScaler_GetResolution(IMILBitmapScaler *iface,
    double *dpix, double *dpiy)
{
    BitmapScaler *This = impl_from_IMILBitmapScaler(iface);
    TRACE("(%p,%p,%p)\n", iface, dpix, dpiy);
    return IWICBitmapScaler_GetResolution(&This->IWICBitmapScaler_iface, dpix, dpiy);
}

static HRESULT WINAPI IMILBitmapScaler_CopyPalette(IMILBitmapScaler *iface,
    IWICPalette *palette)
{
    BitmapScaler *This = impl_from_IMILBitmapScaler(iface);

    TRACE("(%p,%p)\n", iface, palette);

    if (!This->source)
        return WINCODEC_ERR_NOTINITIALIZED;

    return IWICBitmapScaler_CopyPalette(&This->IWICBitmapScaler_iface, palette);
}

static HRESULT WINAPI IMILBitmapScaler_CopyPixels(IMILBitmapScaler *iface,
    const WICRect *rc, UINT stride, UINT size, BYTE *buffer)
{
    BitmapScaler *This = impl_from_IMILBitmapScaler(iface);
    TRACE("(%p,%p,%u,%u,%p)\n", iface, rc, stride, size, buffer);
    return IWICBitmapScaler_CopyPixels(&This->IWICBitmapScaler_iface, rc, stride, size, buffer);
}

static HRESULT WINAPI IMILBitmapScaler_unknown1(IMILBitmapScaler *iface, void **ppv)
{
    TRACE("(%p,%p)\n", iface, ppv);
    return E_NOINTERFACE;
}

static HRESULT WINAPI IMILBitmapScaler_Initialize(IMILBitmapScaler *iface,
    IMILBitmapSource *mil_source, UINT width, UINT height,
    WICBitmapInterpolationMode mode)
{
    BitmapScaler *This = impl_from_IMILBitmapScaler(iface);
    IWICBitmapSource *wic_source;
    HRESULT hr;

    TRACE("(%p,%p,%u,%u,%u)\n", iface, mil_source, width, height, mode);

    if (!mil_source) return E_INVALIDARG;

    hr = mil_source->lpVtbl->QueryInterface(mil_source, &IID_IWICBitmapSource, (void **)&wic_source);
    if (hr == S_OK)
    {
        hr = IWICBitmapScaler_Initialize(&This->IWICBitmapScaler_iface, wic_source, width, height, mode);
        IWICBitmapSource_Release(wic_source);
    }
    return hr;
}

static const IMILBitmapScalerVtbl IMILBitmapScaler_Vtbl = {
    IMILBitmapScaler_QueryInterface,
    IMILBitmapScaler_AddRef,
    IMILBitmapScaler_Release,
    IMILBitmapScaler_GetSize,
    IMILBitmapScaler_GetPixelFormat,
    IMILBitmapScaler_GetResolution,
    IMILBitmapScaler_CopyPalette,
    IMILBitmapScaler_CopyPixels,
    IMILBitmapScaler_unknown1,
    IMILBitmapScaler_Initialize
};

HRESULT BitmapScaler_Create(IWICBitmapScaler **scaler)
{
    BitmapScaler *This;

    This = malloc(sizeof(BitmapScaler));
    if (!This) return E_OUTOFMEMORY;

    This->IWICBitmapScaler_iface.lpVtbl = &BitmapScaler_Vtbl;
    This->IMILBitmapScaler_iface.lpVtbl = &IMILBitmapScaler_Vtbl;
    This->ref = 1;
    This->source = NULL;
    This->width = 0;
    This->height = 0;
    This->src_width = 0;
    This->src_height = 0;
    This->mode = 0;
    This->bpp = 0;
#ifdef __REACTOS__
    InitializeCriticalSection(&This->lock);
#else
    InitializeCriticalSectionEx(&This->lock, 0, RTL_CRITICAL_SECTION_FLAG_FORCE_DEBUG_INFO);
#endif
    This->lock.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": BitmapScaler.lock");

    *scaler = &This->IWICBitmapScaler_iface;

    return S_OK;
}
