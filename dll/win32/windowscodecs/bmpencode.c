/*
 * Copyright 2009 Vincent Povirk for CodeWeavers
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

#include <stdarg.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "wingdi.h"
#include "objbase.h"
#include "wincodec.h"

#include "wincodecs_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(wincodecs);

struct bmp_pixelformat {
    const WICPixelFormatGUID *guid;
    UINT bpp;
    DWORD compression;
    DWORD redmask;
    DWORD greenmask;
    DWORD bluemask;
    DWORD alphamask;
};

static const struct bmp_pixelformat formats[] = {
    {&GUID_WICPixelFormat24bppBGR, 24, BI_RGB},
    {&GUID_WICPixelFormat16bppBGR555, 16, BI_RGB},
    {&GUID_WICPixelFormat16bppBGR565, 16, BI_BITFIELDS, 0xf800, 0x7e0, 0x1f, 0},
    {&GUID_WICPixelFormat32bppBGR, 32, BI_RGB},
#if 0
    /* Windows doesn't seem to support this one. */
    {&GUID_WICPixelFormat32bppBGRA, 32, BI_BITFIELDS, 0xff0000, 0xff00, 0xff, 0xff000000},
#endif
    {NULL}
};

typedef struct BmpFrameEncode {
    const IWICBitmapFrameEncodeVtbl *lpVtbl;
    LONG ref;
    IStream *stream;
    BOOL initialized;
    UINT width, height;
    BYTE *bits;
    const struct bmp_pixelformat *format;
    double xres, yres;
    UINT lineswritten;
    UINT stride;
    BOOL committed;
} BmpFrameEncode;

static HRESULT WINAPI BmpFrameEncode_QueryInterface(IWICBitmapFrameEncode *iface, REFIID iid,
    void **ppv)
{
    BmpFrameEncode *This = (BmpFrameEncode*)iface;
    TRACE("(%p,%s,%p)\n", iface, debugstr_guid(iid), ppv);

    if (!ppv) return E_INVALIDARG;

    if (IsEqualIID(&IID_IUnknown, iid) ||
        IsEqualIID(&IID_IWICBitmapFrameEncode, iid))
    {
        *ppv = This;
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI BmpFrameEncode_AddRef(IWICBitmapFrameEncode *iface)
{
    BmpFrameEncode *This = (BmpFrameEncode*)iface;
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) refcount=%u\n", iface, ref);

    return ref;
}

static ULONG WINAPI BmpFrameEncode_Release(IWICBitmapFrameEncode *iface)
{
    BmpFrameEncode *This = (BmpFrameEncode*)iface;
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) refcount=%u\n", iface, ref);

    if (ref == 0)
    {
        if (This->stream) IStream_Release(This->stream);
        HeapFree(GetProcessHeap(), 0, This->bits);
        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}

static HRESULT WINAPI BmpFrameEncode_Initialize(IWICBitmapFrameEncode *iface,
    IPropertyBag2 *pIEncoderOptions)
{
    BmpFrameEncode *This = (BmpFrameEncode*)iface;
    TRACE("(%p,%p)\n", iface, pIEncoderOptions);

    if (This->initialized) return WINCODEC_ERR_WRONGSTATE;

    This->initialized = TRUE;

    return S_OK;
}

static HRESULT WINAPI BmpFrameEncode_SetSize(IWICBitmapFrameEncode *iface,
    UINT uiWidth, UINT uiHeight)
{
    BmpFrameEncode *This = (BmpFrameEncode*)iface;
    TRACE("(%p,%u,%u)\n", iface, uiWidth, uiHeight);

    if (!This->initialized || This->bits) return WINCODEC_ERR_WRONGSTATE;

    This->width = uiWidth;
    This->height = uiHeight;

    return S_OK;
}

static HRESULT WINAPI BmpFrameEncode_SetResolution(IWICBitmapFrameEncode *iface,
    double dpiX, double dpiY)
{
    BmpFrameEncode *This = (BmpFrameEncode*)iface;
    TRACE("(%p,%0.2f,%0.2f)\n", iface, dpiX, dpiY);

    if (!This->initialized || This->bits) return WINCODEC_ERR_WRONGSTATE;

    This->xres = dpiX;
    This->yres = dpiY;

    return S_OK;
}

static HRESULT WINAPI BmpFrameEncode_SetPixelFormat(IWICBitmapFrameEncode *iface,
    WICPixelFormatGUID *pPixelFormat)
{
    BmpFrameEncode *This = (BmpFrameEncode*)iface;
    int i;
    TRACE("(%p,%s)\n", iface, debugstr_guid(pPixelFormat));

    if (!This->initialized || This->bits) return WINCODEC_ERR_WRONGSTATE;

    for (i=0; formats[i].guid; i++)
    {
        if (memcmp(formats[i].guid, pPixelFormat, sizeof(GUID)) == 0)
            break;
    }

    if (!formats[i].guid) i = 0;

    This->format = &formats[i];
    memcpy(pPixelFormat, This->format->guid, sizeof(GUID));

    return S_OK;
}

static HRESULT WINAPI BmpFrameEncode_SetColorContexts(IWICBitmapFrameEncode *iface,
    UINT cCount, IWICColorContext **ppIColorContext)
{
    FIXME("(%p,%u,%p): stub\n", iface, cCount, ppIColorContext);
    return E_NOTIMPL;
}

static HRESULT WINAPI BmpFrameEncode_SetPalette(IWICBitmapFrameEncode *iface,
    IWICPalette *pIPalette)
{
    FIXME("(%p,%p): stub\n", iface, pIPalette);
    return WINCODEC_ERR_UNSUPPORTEDOPERATION;
}

static HRESULT WINAPI BmpFrameEncode_SetThumbnail(IWICBitmapFrameEncode *iface,
    IWICBitmapSource *pIThumbnail)
{
    FIXME("(%p,%p): stub\n", iface, pIThumbnail);
    return WINCODEC_ERR_UNSUPPORTEDOPERATION;
}

static HRESULT BmpFrameEncode_AllocateBits(BmpFrameEncode *This)
{
    if (!This->bits)
    {
        if (!This->initialized || !This->width || !This->height || !This->format)
            return WINCODEC_ERR_WRONGSTATE;

        This->stride = (((This->width * This->format->bpp)+31)/32)*4;
        This->bits = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, This->stride * This->height);
        if (!This->bits) return E_OUTOFMEMORY;
    }

    return S_OK;
}

static HRESULT WINAPI BmpFrameEncode_WritePixels(IWICBitmapFrameEncode *iface,
    UINT lineCount, UINT cbStride, UINT cbBufferSize, BYTE *pbPixels)
{
    BmpFrameEncode *This = (BmpFrameEncode*)iface;
    HRESULT hr;
    WICRect rc;
    TRACE("(%p,%u,%u,%u,%p)\n", iface, lineCount, cbStride, cbBufferSize, pbPixels);

    if (!This->initialized || !This->width || !This->height || !This->format)
        return WINCODEC_ERR_WRONGSTATE;

    hr = BmpFrameEncode_AllocateBits(This);
    if (FAILED(hr)) return hr;

    rc.X = 0;
    rc.Y = 0;
    rc.Width = This->width;
    rc.Height = lineCount;

    hr = copy_pixels(This->format->bpp, pbPixels, This->width, lineCount, cbStride,
        &rc, This->stride, This->stride*(This->height-This->lineswritten),
        This->bits + This->stride*This->lineswritten);

    if (SUCCEEDED(hr))
        This->lineswritten += lineCount;

    return hr;
}

static HRESULT WINAPI BmpFrameEncode_WriteSource(IWICBitmapFrameEncode *iface,
    IWICBitmapSource *pIBitmapSource, WICRect *prc)
{
    BmpFrameEncode *This = (BmpFrameEncode*)iface;
    HRESULT hr;
    WICRect rc;
    WICPixelFormatGUID guid;
    TRACE("(%p,%p,%p)\n", iface, pIBitmapSource, prc);

    if (!This->initialized || !This->width || !This->height)
        return WINCODEC_ERR_WRONGSTATE;

    if (!This->format)
    {
        hr = IWICBitmapSource_GetPixelFormat(pIBitmapSource, &guid);
        if (FAILED(hr)) return hr;
        hr = BmpFrameEncode_SetPixelFormat(iface, &guid);
        if (FAILED(hr)) return hr;
    }

    hr = IWICBitmapSource_GetPixelFormat(pIBitmapSource, &guid);
    if (FAILED(hr)) return hr;
    if (memcmp(&guid, This->format->guid, sizeof(GUID)) != 0)
    {
        /* should use WICConvertBitmapSource to convert, but that's unimplemented */
        ERR("format %s unsupported\n", debugstr_guid(&guid));
        return E_FAIL;
    }

    if (This->xres == 0.0 || This->yres == 0.0)
    {
        double xres, yres;
        hr = IWICBitmapSource_GetResolution(pIBitmapSource, &xres, &yres);
        if (FAILED(hr)) return hr;
        hr = BmpFrameEncode_SetResolution(iface, xres, yres);
        if (FAILED(hr)) return hr;
    }

    if (!prc)
    {
        UINT width, height;
        hr = IWICBitmapSource_GetSize(pIBitmapSource, &width, &height);
        if (FAILED(hr)) return hr;
        rc.X = 0;
        rc.Y = 0;
        rc.Width = width;
        rc.Height = height;
        prc = &rc;
    }

    if (prc->Width != This->width) return E_INVALIDARG;

    hr = BmpFrameEncode_AllocateBits(This);
    if (FAILED(hr)) return hr;

    hr = IWICBitmapSource_CopyPixels(pIBitmapSource, prc, This->stride,
        This->stride*(This->height-This->lineswritten),
        This->bits + This->stride*This->lineswritten);

    This->lineswritten += rc.Height;

    return S_OK;
}

static HRESULT WINAPI BmpFrameEncode_Commit(IWICBitmapFrameEncode *iface)
{
    BmpFrameEncode *This = (BmpFrameEncode*)iface;
    BITMAPFILEHEADER bfh;
    BITMAPV5HEADER bih;
    UINT info_size;
    LARGE_INTEGER pos;
    ULONG byteswritten;
    HRESULT hr;

    TRACE("(%p)\n", iface);

    if (!This->bits || This->committed || This->height != This->lineswritten)
        return WINCODEC_ERR_WRONGSTATE;

    bfh.bfType = 0x4d42; /* "BM" */
    bfh.bfReserved1 = 0;
    bfh.bfReserved2 = 0;

    bih.bV5Size = info_size = sizeof(BITMAPINFOHEADER);
    bih.bV5Width = This->width;
    bih.bV5Height = -This->height; /* top-down bitmap */
    bih.bV5Planes = 1;
    bih.bV5BitCount = This->format->bpp;
    bih.bV5Compression = This->format->compression;
    bih.bV5SizeImage = This->stride*This->height;
    bih.bV5XPelsPerMeter = (This->xres+0.0127) / 0.0254;
    bih.bV5YPelsPerMeter = (This->yres+0.0127) / 0.0254;
    bih.bV5ClrUsed = 0;
    bih.bV5ClrImportant = 0;

    if (This->format->compression == BI_BITFIELDS)
    {
        if (This->format->alphamask)
            bih.bV5Size = info_size = sizeof(BITMAPV4HEADER);
        else
            info_size = sizeof(BITMAPINFOHEADER)+12;
        bih.bV5RedMask = This->format->redmask;
        bih.bV5GreenMask = This->format->greenmask;
        bih.bV5BlueMask = This->format->bluemask;
        bih.bV5AlphaMask = This->format->alphamask;
        bih.bV5AlphaMask = LCS_DEVICE_RGB;
    }

    bfh.bfSize = sizeof(BITMAPFILEHEADER) + info_size + bih.bV5SizeImage;
    bfh.bfOffBits = sizeof(BITMAPFILEHEADER) + info_size;

    pos.QuadPart = 0;
    hr = IStream_Seek(This->stream, pos, STREAM_SEEK_SET, NULL);
    if (FAILED(hr)) return hr;

    hr = IStream_Write(This->stream, &bfh, sizeof(BITMAPFILEHEADER), &byteswritten);
    if (FAILED(hr)) return hr;
    if (byteswritten != sizeof(BITMAPFILEHEADER)) return E_FAIL;

    hr = IStream_Write(This->stream, &bih, info_size, &byteswritten);
    if (FAILED(hr)) return hr;
    if (byteswritten != info_size) return E_FAIL;

    hr = IStream_Write(This->stream, This->bits, bih.bV5SizeImage, &byteswritten);
    if (FAILED(hr)) return hr;
    if (byteswritten != bih.bV5SizeImage) return E_FAIL;

    This->committed = TRUE;

    return S_OK;
}

static HRESULT WINAPI BmpFrameEncode_GetMetadataQueryWriter(IWICBitmapFrameEncode *iface,
    IWICMetadataQueryWriter **ppIMetadataQueryWriter)
{
    FIXME("(%p, %p): stub\n", iface, ppIMetadataQueryWriter);
    return E_NOTIMPL;
}

static const IWICBitmapFrameEncodeVtbl BmpFrameEncode_Vtbl = {
    BmpFrameEncode_QueryInterface,
    BmpFrameEncode_AddRef,
    BmpFrameEncode_Release,
    BmpFrameEncode_Initialize,
    BmpFrameEncode_SetSize,
    BmpFrameEncode_SetResolution,
    BmpFrameEncode_SetPixelFormat,
    BmpFrameEncode_SetColorContexts,
    BmpFrameEncode_SetPalette,
    BmpFrameEncode_SetThumbnail,
    BmpFrameEncode_WritePixels,
    BmpFrameEncode_WriteSource,
    BmpFrameEncode_Commit,
    BmpFrameEncode_GetMetadataQueryWriter
};

typedef struct BmpEncoder {
    const IWICBitmapEncoderVtbl *lpVtbl;
    LONG ref;
    IStream *stream;
    IWICBitmapFrameEncode *frame;
} BmpEncoder;

static HRESULT WINAPI BmpEncoder_QueryInterface(IWICBitmapEncoder *iface, REFIID iid,
    void **ppv)
{
    BmpEncoder *This = (BmpEncoder*)iface;
    TRACE("(%p,%s,%p)\n", iface, debugstr_guid(iid), ppv);

    if (!ppv) return E_INVALIDARG;

    if (IsEqualIID(&IID_IUnknown, iid) ||
        IsEqualIID(&IID_IWICBitmapEncoder, iid))
    {
        *ppv = This;
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI BmpEncoder_AddRef(IWICBitmapEncoder *iface)
{
    BmpEncoder *This = (BmpEncoder*)iface;
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) refcount=%u\n", iface, ref);

    return ref;
}

static ULONG WINAPI BmpEncoder_Release(IWICBitmapEncoder *iface)
{
    BmpEncoder *This = (BmpEncoder*)iface;
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) refcount=%u\n", iface, ref);

    if (ref == 0)
    {
        if (This->stream) IStream_Release(This->stream);
        if (This->frame) IWICBitmapFrameEncode_Release(This->frame);
        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}

static HRESULT WINAPI BmpEncoder_Initialize(IWICBitmapEncoder *iface,
    IStream *pIStream, WICBitmapEncoderCacheOption cacheOption)
{
    BmpEncoder *This = (BmpEncoder*)iface;

    TRACE("(%p,%p,%u)\n", iface, pIStream, cacheOption);

    IStream_AddRef(pIStream);
    This->stream = pIStream;

    return S_OK;
}

static HRESULT WINAPI BmpEncoder_GetContainerFormat(IWICBitmapEncoder *iface,
    GUID *pguidContainerFormat)
{
    FIXME("(%p,%s): stub\n", iface, debugstr_guid(pguidContainerFormat));
    return E_NOTIMPL;
}

static HRESULT WINAPI BmpEncoder_GetEncoderInfo(IWICBitmapEncoder *iface,
    IWICBitmapEncoderInfo **ppIEncoderInfo)
{
    FIXME("(%p,%p): stub\n", iface, ppIEncoderInfo);
    return E_NOTIMPL;
}

static HRESULT WINAPI BmpEncoder_SetColorContexts(IWICBitmapEncoder *iface,
    UINT cCount, IWICColorContext **ppIColorContext)
{
    FIXME("(%p,%u,%p): stub\n", iface, cCount, ppIColorContext);
    return E_NOTIMPL;
}

static HRESULT WINAPI BmpEncoder_SetPalette(IWICBitmapEncoder *iface, IWICPalette *pIPalette)
{
    TRACE("(%p,%p)\n", iface, pIPalette);
    return WINCODEC_ERR_UNSUPPORTEDOPERATION;
}

static HRESULT WINAPI BmpEncoder_SetThumbnail(IWICBitmapEncoder *iface, IWICBitmapSource *pIThumbnail)
{
    TRACE("(%p,%p)\n", iface, pIThumbnail);
    return WINCODEC_ERR_UNSUPPORTEDOPERATION;
}

static HRESULT WINAPI BmpEncoder_SetPreview(IWICBitmapEncoder *iface, IWICBitmapSource *pIPreview)
{
    TRACE("(%p,%p)\n", iface, pIPreview);
    return WINCODEC_ERR_UNSUPPORTEDOPERATION;
}

static HRESULT WINAPI BmpEncoder_CreateNewFrame(IWICBitmapEncoder *iface,
    IWICBitmapFrameEncode **ppIFrameEncode, IPropertyBag2 **ppIEncoderOptions)
{
    BmpEncoder *This = (BmpEncoder*)iface;
    BmpFrameEncode *encode;
    HRESULT hr;

    TRACE("(%p,%p,%p)\n", iface, ppIFrameEncode, ppIEncoderOptions);

    if (This->frame) return WINCODEC_ERR_UNSUPPORTEDOPERATION;

    if (!This->stream) return WINCODEC_ERR_NOTINITIALIZED;

    hr = CreatePropertyBag2(ppIEncoderOptions);
    if (FAILED(hr)) return hr;

    encode = HeapAlloc(GetProcessHeap(), 0, sizeof(BmpFrameEncode));
    if (!encode)
    {
        IPropertyBag2_Release(*ppIEncoderOptions);
        *ppIEncoderOptions = NULL;
        return E_OUTOFMEMORY;
    }
    encode->lpVtbl = &BmpFrameEncode_Vtbl;
    encode->ref = 2;
    IStream_AddRef(This->stream);
    encode->stream = This->stream;
    encode->initialized = FALSE;
    encode->width = 0;
    encode->height = 0;
    encode->bits = NULL;
    encode->format = NULL;
    encode->xres = 0.0;
    encode->yres = 0.0;
    encode->lineswritten = 0;
    encode->committed = FALSE;

    *ppIFrameEncode = (IWICBitmapFrameEncode*)encode;
    This->frame = (IWICBitmapFrameEncode*)encode;

    return S_OK;
}

static HRESULT WINAPI BmpEncoder_Commit(IWICBitmapEncoder *iface)
{
    BmpEncoder *This = (BmpEncoder*)iface;
    BmpFrameEncode *frame = (BmpFrameEncode*)This->frame;
    TRACE("(%p)\n", iface);

    if (!frame || !frame->committed) return WINCODEC_ERR_WRONGSTATE;

    return S_OK;
}

static HRESULT WINAPI BmpEncoder_GetMetadataQueryWriter(IWICBitmapEncoder *iface,
    IWICMetadataQueryWriter **ppIMetadataQueryWriter)
{
    FIXME("(%p,%p): stub\n", iface, ppIMetadataQueryWriter);
    return E_NOTIMPL;
}

static const IWICBitmapEncoderVtbl BmpEncoder_Vtbl = {
    BmpEncoder_QueryInterface,
    BmpEncoder_AddRef,
    BmpEncoder_Release,
    BmpEncoder_Initialize,
    BmpEncoder_GetContainerFormat,
    BmpEncoder_GetEncoderInfo,
    BmpEncoder_SetColorContexts,
    BmpEncoder_SetPalette,
    BmpEncoder_SetThumbnail,
    BmpEncoder_SetPreview,
    BmpEncoder_CreateNewFrame,
    BmpEncoder_Commit,
    BmpEncoder_GetMetadataQueryWriter
};

HRESULT BmpEncoder_CreateInstance(IUnknown *pUnkOuter, REFIID iid, void** ppv)
{
    BmpEncoder *This;
    HRESULT ret;

    TRACE("(%p,%s,%p)\n", pUnkOuter, debugstr_guid(iid), ppv);

    *ppv = NULL;

    if (pUnkOuter) return CLASS_E_NOAGGREGATION;

    This = HeapAlloc(GetProcessHeap(), 0, sizeof(BmpEncoder));
    if (!This) return E_OUTOFMEMORY;

    This->lpVtbl = &BmpEncoder_Vtbl;
    This->ref = 1;
    This->stream = NULL;
    This->frame = NULL;

    ret = IUnknown_QueryInterface((IUnknown*)This, iid, ppv);
    IUnknown_Release((IUnknown*)This);

    return ret;
}
