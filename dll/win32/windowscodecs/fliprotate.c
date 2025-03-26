/*
 * Copyright 2010 Vincent Povirk for CodeWeavers
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

typedef struct FlipRotator {
    IWICBitmapFlipRotator IWICBitmapFlipRotator_iface;
    LONG ref;
    IWICBitmapSource *source;
    int flip_x;
    int flip_y;
    int swap_xy;
    CRITICAL_SECTION lock; /* must be held when initialized */
} FlipRotator;

static inline FlipRotator *impl_from_IWICBitmapFlipRotator(IWICBitmapFlipRotator *iface)
{
    return CONTAINING_RECORD(iface, FlipRotator, IWICBitmapFlipRotator_iface);
}

static HRESULT WINAPI FlipRotator_QueryInterface(IWICBitmapFlipRotator *iface, REFIID iid,
    void **ppv)
{
    FlipRotator *This = impl_from_IWICBitmapFlipRotator(iface);
    TRACE("(%p,%s,%p)\n", iface, debugstr_guid(iid), ppv);

    if (!ppv) return E_INVALIDARG;

    if (IsEqualIID(&IID_IUnknown, iid) ||
        IsEqualIID(&IID_IWICBitmapSource, iid) ||
        IsEqualIID(&IID_IWICBitmapFlipRotator, iid))
    {
        *ppv = &This->IWICBitmapFlipRotator_iface;
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI FlipRotator_AddRef(IWICBitmapFlipRotator *iface)
{
    FlipRotator *This = impl_from_IWICBitmapFlipRotator(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) refcount=%lu\n", iface, ref);

    return ref;
}

static ULONG WINAPI FlipRotator_Release(IWICBitmapFlipRotator *iface)
{
    FlipRotator *This = impl_from_IWICBitmapFlipRotator(iface);
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

static HRESULT WINAPI FlipRotator_GetSize(IWICBitmapFlipRotator *iface,
    UINT *puiWidth, UINT *puiHeight)
{
    FlipRotator *This = impl_from_IWICBitmapFlipRotator(iface);
    TRACE("(%p,%p,%p)\n", iface, puiWidth, puiHeight);

    if (!This->source)
        return WINCODEC_ERR_WRONGSTATE;
    else if (This->swap_xy)
        return IWICBitmapSource_GetSize(This->source, puiHeight, puiWidth);
    else
        return IWICBitmapSource_GetSize(This->source, puiWidth, puiHeight);
}

static HRESULT WINAPI FlipRotator_GetPixelFormat(IWICBitmapFlipRotator *iface,
    WICPixelFormatGUID *pPixelFormat)
{
    FlipRotator *This = impl_from_IWICBitmapFlipRotator(iface);
    TRACE("(%p,%p)\n", iface, pPixelFormat);

    if (!This->source)
        return WINCODEC_ERR_WRONGSTATE;
    else
        return IWICBitmapSource_GetPixelFormat(This->source, pPixelFormat);
}

static HRESULT WINAPI FlipRotator_GetResolution(IWICBitmapFlipRotator *iface,
    double *pDpiX, double *pDpiY)
{
    FlipRotator *This = impl_from_IWICBitmapFlipRotator(iface);
    TRACE("(%p,%p,%p)\n", iface, pDpiX, pDpiY);

    if (!This->source)
        return WINCODEC_ERR_WRONGSTATE;
    else if (This->swap_xy)
        return IWICBitmapSource_GetResolution(This->source, pDpiY, pDpiX);
    else
        return IWICBitmapSource_GetResolution(This->source, pDpiX, pDpiY);
}

static HRESULT WINAPI FlipRotator_CopyPalette(IWICBitmapFlipRotator *iface,
    IWICPalette *pIPalette)
{
    FlipRotator *This = impl_from_IWICBitmapFlipRotator(iface);
    TRACE("(%p,%p)\n", iface, pIPalette);

    if (!This->source)
        return WINCODEC_ERR_WRONGSTATE;
    else
        return IWICBitmapSource_CopyPalette(This->source, pIPalette);
}

static HRESULT WINAPI FlipRotator_CopyPixels(IWICBitmapFlipRotator *iface,
    const WICRect *prc, UINT cbStride, UINT cbBufferSize, BYTE *pbBuffer)
{
    FlipRotator *This = impl_from_IWICBitmapFlipRotator(iface);
    HRESULT hr;
    UINT y;
    UINT srcy, srcwidth, srcheight;
    WICRect rc;
    WICRect rect;

    TRACE("(%p,%s,%u,%u,%p)\n", iface, debug_wic_rect(prc), cbStride, cbBufferSize, pbBuffer);

    if (!This->source) return WINCODEC_ERR_WRONGSTATE;

    if (This->swap_xy || This->flip_x)
    {
        /* This requires knowledge of the pixel format. */
        FIXME("flipping x and rotating are not implemented\n");
        return E_NOTIMPL;
    }

    hr = IWICBitmapSource_GetSize(This->source, &srcwidth, &srcheight);
    if (FAILED(hr)) return hr;

    if (!prc)
    {
        UINT width, height;
        hr = IWICBitmapFlipRotator_GetSize(iface, &width, &height);
        if (FAILED(hr)) return hr;
        rect.X = 0;
        rect.Y = 0;
        rect.Width = width;
        rect.Height = height;
        prc = &rect;
    }

    for (y=prc->Y; y - prc->Y < prc->Height; y++)
    {
        if (This->flip_y)
            srcy = srcheight - 1 - y;
        else
            srcy = y;

        rc.X = prc->X;
        rc.Y = srcy;
        rc.Width = prc->Width;
        rc.Height = 1;

        hr = IWICBitmapSource_CopyPixels(This->source, &rc, cbStride, cbStride,
            pbBuffer);

        if (FAILED(hr)) break;

        pbBuffer += cbStride;
    }

    return hr;
}

static HRESULT WINAPI FlipRotator_Initialize(IWICBitmapFlipRotator *iface,
    IWICBitmapSource *pISource, WICBitmapTransformOptions options)
{
    FlipRotator *This = impl_from_IWICBitmapFlipRotator(iface);
    HRESULT hr=S_OK;

    TRACE("(%p,%p,%u)\n", iface, pISource, options);

    EnterCriticalSection(&This->lock);

    if (This->source)
    {
        hr = WINCODEC_ERR_WRONGSTATE;
        goto end;
    }

    if (options&WICBitmapTransformRotate90)
    {
        This->swap_xy = 1;
        This->flip_x = !This->flip_x;
    }

    if (options&WICBitmapTransformRotate180)
    {
        This->flip_x = !This->flip_x;
        This->flip_y = !This->flip_y;
    }

    if (options&WICBitmapTransformFlipHorizontal)
        This->flip_x = !This->flip_x;

    if (options&WICBitmapTransformFlipVertical)
        This->flip_y = !This->flip_y;

    IWICBitmapSource_AddRef(pISource);
    This->source = pISource;

end:
    LeaveCriticalSection(&This->lock);

    return hr;
}

static const IWICBitmapFlipRotatorVtbl FlipRotator_Vtbl = {
    FlipRotator_QueryInterface,
    FlipRotator_AddRef,
    FlipRotator_Release,
    FlipRotator_GetSize,
    FlipRotator_GetPixelFormat,
    FlipRotator_GetResolution,
    FlipRotator_CopyPalette,
    FlipRotator_CopyPixels,
    FlipRotator_Initialize
};

HRESULT FlipRotator_Create(IWICBitmapFlipRotator **fliprotator)
{
    FlipRotator *This;

    This = malloc(sizeof(FlipRotator));
    if (!This) return E_OUTOFMEMORY;

    This->IWICBitmapFlipRotator_iface.lpVtbl = &FlipRotator_Vtbl;
    This->ref = 1;
    This->source = NULL;
    This->flip_x = 0;
    This->flip_y = 0;
    This->swap_xy = 0;
#ifdef __REACTOS__
    InitializeCriticalSection(&This->lock);
#else
    InitializeCriticalSectionEx(&This->lock, 0, RTL_CRITICAL_SECTION_FLAG_FORCE_DEBUG_INFO);
#endif
    This->lock.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": FlipRotator.lock");

    *fliprotator = &This->IWICBitmapFlipRotator_iface;

    return S_OK;
}
