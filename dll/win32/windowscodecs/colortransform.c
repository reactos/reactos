/*
 * Copyright 2013 Hans Leidekker for CodeWeavers
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

typedef struct ColorTransform {
    IWICColorTransform IWICColorTransform_iface;
    LONG ref;
    IWICBitmapSource *dst;
} ColorTransform;

static inline ColorTransform *impl_from_IWICColorTransform(IWICColorTransform *iface)
{
    return CONTAINING_RECORD(iface, ColorTransform, IWICColorTransform_iface);
}

static HRESULT WINAPI ColorTransform_QueryInterface(IWICColorTransform *iface, REFIID iid,
    void **ppv)
{
    ColorTransform *This = impl_from_IWICColorTransform(iface);
    TRACE("(%p,%s,%p)\n", iface, debugstr_guid(iid), ppv);

    if (!ppv) return E_INVALIDARG;

    if (IsEqualIID(&IID_IUnknown, iid) ||
        IsEqualIID(&IID_IWICBitmapSource, iid) ||
        IsEqualIID(&IID_IWICColorTransform, iid))
    {
        *ppv = &This->IWICColorTransform_iface;
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI ColorTransform_AddRef(IWICColorTransform *iface)
{
    ColorTransform *This = impl_from_IWICColorTransform(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) refcount=%lu\n", iface, ref);

    return ref;
}

static ULONG WINAPI ColorTransform_Release(IWICColorTransform *iface)
{
    ColorTransform *This = impl_from_IWICColorTransform(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) refcount=%lu\n", iface, ref);

    if (ref == 0)
    {
        if (This->dst) IWICBitmapSource_Release(This->dst);
        free(This);
    }

    return ref;
}

static HRESULT WINAPI ColorTransform_GetSize(IWICColorTransform *iface,
    UINT *puiWidth, UINT *puiHeight)
{
    ColorTransform *This = impl_from_IWICColorTransform(iface);
    TRACE("(%p,%p,%p)\n", iface, puiWidth, puiHeight);

    return IWICBitmapSource_GetSize(This->dst, puiWidth, puiHeight);
}

static HRESULT WINAPI ColorTransform_GetPixelFormat(IWICColorTransform *iface,
    WICPixelFormatGUID *pPixelFormat)
{
    ColorTransform *This = impl_from_IWICColorTransform(iface);
    TRACE("(%p,%p)\n", iface, pPixelFormat);

    return IWICBitmapSource_GetPixelFormat(This->dst, pPixelFormat);
}

static HRESULT WINAPI ColorTransform_GetResolution(IWICColorTransform *iface,
    double *pDpiX, double *pDpiY)
{
    ColorTransform *This = impl_from_IWICColorTransform(iface);
    TRACE("(%p,%p,%p)\n", iface, pDpiX, pDpiY);

    return IWICBitmapSource_GetResolution(This->dst, pDpiX, pDpiY);
}

static HRESULT WINAPI ColorTransform_CopyPalette(IWICColorTransform *iface,
    IWICPalette *pIPalette)
{
    ColorTransform *This = impl_from_IWICColorTransform(iface);
    TRACE("(%p,%p)\n", iface, pIPalette);

    return IWICBitmapSource_CopyPalette(This->dst, pIPalette);
}

static HRESULT WINAPI ColorTransform_CopyPixels(IWICColorTransform *iface,
    const WICRect *prc, UINT cbStride, UINT cbBufferSize, BYTE *pbBuffer)
{
    ColorTransform *This = impl_from_IWICColorTransform(iface);
    TRACE("(%p,%s,%u,%u,%p)\n", iface, debug_wic_rect(prc), cbStride, cbBufferSize, pbBuffer);

    return IWICBitmapSource_CopyPixels(This->dst, prc, cbStride, cbBufferSize, pbBuffer);
}

static HRESULT WINAPI ColorTransform_Initialize(IWICColorTransform *iface,
    IWICBitmapSource *pIBitmapSource, IWICColorContext *pIContextSource,
    IWICColorContext *pIContextDest, REFWICPixelFormatGUID pixelFmtDest)
{
    ColorTransform *This = impl_from_IWICColorTransform(iface);
    IWICBitmapSource *dst;
    HRESULT hr;

    TRACE("(%p,%p,%p,%p,%s)\n", iface, pIBitmapSource, pIContextSource,
          pIContextDest, debugstr_guid(pixelFmtDest));

    FIXME("ignoring color contexts\n");

    hr = WICConvertBitmapSource(pixelFmtDest, pIBitmapSource, &dst);
    if (FAILED(hr)) return hr;

    if (This->dst) IWICBitmapSource_Release(This->dst);
    This->dst = dst;
    return S_OK;
}

static const IWICColorTransformVtbl ColorTransform_Vtbl = {
    ColorTransform_QueryInterface,
    ColorTransform_AddRef,
    ColorTransform_Release,
    ColorTransform_GetSize,
    ColorTransform_GetPixelFormat,
    ColorTransform_GetResolution,
    ColorTransform_CopyPalette,
    ColorTransform_CopyPixels,
    ColorTransform_Initialize
};

HRESULT ColorTransform_Create(IWICColorTransform **colortransform)
{
    ColorTransform *This;

    if (!colortransform) return E_INVALIDARG;

    This = malloc(sizeof(ColorTransform));
    if (!This) return E_OUTOFMEMORY;

    This->IWICColorTransform_iface.lpVtbl = &ColorTransform_Vtbl;
    This->ref = 1;
    This->dst = NULL;

    *colortransform = &This->IWICColorTransform_iface;

    return S_OK;
}
