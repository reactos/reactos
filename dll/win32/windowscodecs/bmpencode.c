/*
 * Copyright 2009 Vincent Povirk for CodeWeavers
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
#include "winreg.h"
#include "wingdi.h"
#include "objbase.h"

#include "wincodecs_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(wincodecs);

struct bmp_pixelformat {
    const WICPixelFormatGUID *guid;
    UINT bpp;
    UINT colors; /* palette size */
    DWORD compression;
    DWORD redmask;
    DWORD greenmask;
    DWORD bluemask;
    DWORD alphamask;
};

static const struct bmp_pixelformat formats[] = {
    {&GUID_WICPixelFormat24bppBGR, 24, 0, BI_RGB},
    {&GUID_WICPixelFormatBlackWhite, 1, 2, BI_RGB},
    {&GUID_WICPixelFormat1bppIndexed, 1, 2, BI_RGB},
    {&GUID_WICPixelFormat2bppIndexed, 2, 4, BI_RGB},
    {&GUID_WICPixelFormat4bppIndexed, 4, 16, BI_RGB},
    {&GUID_WICPixelFormat8bppIndexed, 8, 256, BI_RGB},
    {&GUID_WICPixelFormat16bppBGR555, 16, 0, BI_RGB},
    {&GUID_WICPixelFormat16bppBGR565, 16, 0, BI_BITFIELDS, 0xf800, 0x7e0, 0x1f, 0},
    {&GUID_WICPixelFormat32bppBGR, 32, 0, BI_RGB},
    {&GUID_WICPixelFormat32bppBGRA, 32, 0, BI_BITFIELDS, 0xff0000, 0xff00, 0xff, 0xff000000},
    {NULL}
};

typedef struct BmpFrameEncode {
    IWICBitmapFrameEncode IWICBitmapFrameEncode_iface;
    LONG ref;
    IStream *stream;
    BOOL initialized;
    UINT width, height;
    BYTE *bits;
    const struct bmp_pixelformat *format;
    double xres, yres;
    UINT lineswritten;
    UINT stride;
    WICColor palette[256];
    UINT colors;
    BOOL committed;
} BmpFrameEncode;

static inline BmpFrameEncode *impl_from_IWICBitmapFrameEncode(IWICBitmapFrameEncode *iface)
{
    return CONTAINING_RECORD(iface, BmpFrameEncode, IWICBitmapFrameEncode_iface);
}

static HRESULT WINAPI BmpFrameEncode_QueryInterface(IWICBitmapFrameEncode *iface, REFIID iid,
    void **ppv)
{
    BmpFrameEncode *This = impl_from_IWICBitmapFrameEncode(iface);
    TRACE("(%p,%s,%p)\n", iface, debugstr_guid(iid), ppv);

    if (!ppv) return E_INVALIDARG;

    if (IsEqualIID(&IID_IUnknown, iid) ||
        IsEqualIID(&IID_IWICBitmapFrameEncode, iid))
    {
        *ppv = &This->IWICBitmapFrameEncode_iface;
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
    BmpFrameEncode *This = impl_from_IWICBitmapFrameEncode(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) refcount=%lu\n", iface, ref);

    return ref;
}

static ULONG WINAPI BmpFrameEncode_Release(IWICBitmapFrameEncode *iface)
{
    BmpFrameEncode *This = impl_from_IWICBitmapFrameEncode(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) refcount=%lu\n", iface, ref);

    if (ref == 0)
    {
        if (This->stream) IStream_Release(This->stream);
        free(This->bits);
        free(This);
    }

    return ref;
}

static HRESULT WINAPI BmpFrameEncode_Initialize(IWICBitmapFrameEncode *iface,
    IPropertyBag2 *pIEncoderOptions)
{
    BmpFrameEncode *This = impl_from_IWICBitmapFrameEncode(iface);
    TRACE("(%p,%p)\n", iface, pIEncoderOptions);

    if (This->initialized) return WINCODEC_ERR_WRONGSTATE;

    if (pIEncoderOptions)
        WARN("ignoring encoder options.\n");

    This->initialized = TRUE;

    return S_OK;
}

static HRESULT WINAPI BmpFrameEncode_SetSize(IWICBitmapFrameEncode *iface,
    UINT uiWidth, UINT uiHeight)
{
    BmpFrameEncode *This = impl_from_IWICBitmapFrameEncode(iface);
    TRACE("(%p,%u,%u)\n", iface, uiWidth, uiHeight);

    if (!This->initialized || This->bits) return WINCODEC_ERR_WRONGSTATE;

    This->width = uiWidth;
    This->height = uiHeight;

    return S_OK;
}

static HRESULT WINAPI BmpFrameEncode_SetResolution(IWICBitmapFrameEncode *iface,
    double dpiX, double dpiY)
{
    BmpFrameEncode *This = impl_from_IWICBitmapFrameEncode(iface);
    TRACE("(%p,%0.2f,%0.2f)\n", iface, dpiX, dpiY);

    if (!This->initialized || This->bits) return WINCODEC_ERR_WRONGSTATE;

    This->xres = dpiX;
    This->yres = dpiY;

    return S_OK;
}

static HRESULT WINAPI BmpFrameEncode_SetPixelFormat(IWICBitmapFrameEncode *iface,
    WICPixelFormatGUID *pPixelFormat)
{
    BmpFrameEncode *This = impl_from_IWICBitmapFrameEncode(iface);
    int i;
    TRACE("(%p,%s)\n", iface, debugstr_guid(pPixelFormat));

    if (!This->initialized || This->bits) return WINCODEC_ERR_WRONGSTATE;

    if (IsEqualGUID(pPixelFormat, &GUID_WICPixelFormatBlackWhite))
        *pPixelFormat = GUID_WICPixelFormat1bppIndexed;
    else if (IsEqualGUID(pPixelFormat, &GUID_WICPixelFormat2bppIndexed))
        *pPixelFormat = GUID_WICPixelFormat4bppIndexed;

    for (i=0; formats[i].guid; i++)
    {
        if (IsEqualGUID(formats[i].guid, pPixelFormat))
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
    IWICPalette *palette)
{
    BmpFrameEncode *This = impl_from_IWICBitmapFrameEncode(iface);
    HRESULT hr;

    TRACE("(%p,%p)\n", iface, palette);

    if (!palette) return E_INVALIDARG;

    if (!This->initialized)
        return WINCODEC_ERR_NOTINITIALIZED;

    hr = IWICPalette_GetColors(palette, 256, This->palette, &This->colors);
    if (hr == S_OK)
    {
        UINT i;
        for (i = 0; i < This->colors; i++)
            This->palette[i] |= 0xff000000; /* BMP palette has no alpha */
    }
    return hr;
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
        This->bits = calloc(This->stride, This->height);
        if (!This->bits) return E_OUTOFMEMORY;
    }

    return S_OK;
}

static HRESULT WINAPI BmpFrameEncode_WritePixels(IWICBitmapFrameEncode *iface,
    UINT lineCount, UINT cbStride, UINT cbBufferSize, BYTE *pbPixels)
{
    BmpFrameEncode *This = impl_from_IWICBitmapFrameEncode(iface);
    UINT dstbuffersize, bytesperrow, row;
    BYTE *dst, *src;
    HRESULT hr;

    TRACE("(%p,%u,%u,%u,%p)\n", iface, lineCount, cbStride, cbBufferSize, pbPixels);

    if (!This->initialized || !This->width || !This->height || !This->format)
        return WINCODEC_ERR_WRONGSTATE;

    hr = BmpFrameEncode_AllocateBits(This);
    if (FAILED(hr)) return hr;

    bytesperrow = ((This->format->bpp * This->width) + 7) / 8;

    if (This->stride < bytesperrow)
        return E_INVALIDARG;

    dstbuffersize = This->stride * (This->height - This->lineswritten);
    if ((This->stride * (lineCount - 1)) + bytesperrow > dstbuffersize)
        return E_INVALIDARG;

    src = pbPixels;
    dst = This->bits + This->stride * (This->height - This->lineswritten - 1);
    for (row = 0; row < lineCount; row++)
    {
        memcpy(dst, src, bytesperrow);
        src += cbStride;
        dst -= This->stride;
    }

    This->lineswritten += lineCount;

    return S_OK;
}

static HRESULT WINAPI BmpFrameEncode_WriteSource(IWICBitmapFrameEncode *iface,
    IWICBitmapSource *pIBitmapSource, WICRect *prc)
{
    BmpFrameEncode *This = impl_from_IWICBitmapFrameEncode(iface);
    HRESULT hr;
    TRACE("(%p,%p,%s)\n", iface, pIBitmapSource, debug_wic_rect(prc));

    if (!This->initialized)
        return WINCODEC_ERR_WRONGSTATE;

    hr = configure_write_source(iface, pIBitmapSource, prc,
        This->format ? This->format->guid : NULL, This->width, This->height,
        This->xres, This->yres);

    if (SUCCEEDED(hr))
    {
        hr = write_source(iface, pIBitmapSource, prc,
            This->format->guid, This->format->bpp, !This->colors && This->format->colors,
            This->width, This->height);
    }

    return hr;
}

static HRESULT WINAPI BmpFrameEncode_Commit(IWICBitmapFrameEncode *iface)
{
    BmpFrameEncode *This = impl_from_IWICBitmapFrameEncode(iface);
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
    bih.bV5Height = This->height;
    bih.bV5Planes = 1;
    bih.bV5BitCount = This->format->bpp;
    bih.bV5Compression = This->format->compression;
    bih.bV5SizeImage = This->stride*This->height;
    bih.bV5XPelsPerMeter = (This->xres+0.0127) / 0.0254;
    bih.bV5YPelsPerMeter = (This->yres+0.0127) / 0.0254;
    bih.bV5ClrUsed = (This->format->bpp <= 8) ? This->colors : 0;
    bih.bV5ClrImportant = bih.bV5ClrUsed;

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
        bih.bV5CSType = LCS_DEVICE_RGB;
    }

    bfh.bfSize = sizeof(BITMAPFILEHEADER) + info_size + bih.bV5SizeImage;
    bfh.bfOffBits = sizeof(BITMAPFILEHEADER) + info_size;
    bfh.bfOffBits += bih.bV5ClrUsed * sizeof(WICColor);

    pos.QuadPart = 0;
    hr = IStream_Seek(This->stream, pos, STREAM_SEEK_SET, NULL);
    if (FAILED(hr)) return hr;

    hr = IStream_Write(This->stream, &bfh, sizeof(BITMAPFILEHEADER), &byteswritten);
    if (FAILED(hr)) return hr;
    if (byteswritten != sizeof(BITMAPFILEHEADER)) return E_FAIL;

    hr = IStream_Write(This->stream, &bih, info_size, &byteswritten);
    if (FAILED(hr)) return hr;
    if (byteswritten != info_size) return E_FAIL;

    /* write the palette */
    if (This->format->colors)
    {
        hr = IStream_Write(This->stream, This->palette, This->colors * sizeof(WICColor), &byteswritten);
        if (FAILED(hr)) return hr;
        if (byteswritten != This->colors * sizeof(WICColor)) return E_FAIL;
    }

    hr = IStream_Write(This->stream, This->bits, bih.bV5SizeImage, &byteswritten);
    if (FAILED(hr)) return hr;
    if (byteswritten != bih.bV5SizeImage) return E_FAIL;

    This->committed = TRUE;

    return S_OK;
}

static HRESULT WINAPI BmpFrameEncode_GetMetadataQueryWriter(IWICBitmapFrameEncode *iface,
        IWICMetadataQueryWriter **query_writer)
{
    BmpFrameEncode *encoder = impl_from_IWICBitmapFrameEncode(iface);

    TRACE("iface %p, query_writer %p.\n", iface, query_writer);

    if (!encoder->initialized)
        return WINCODEC_ERR_NOTINITIALIZED;

    return WINCODEC_ERR_UNSUPPORTEDOPERATION;
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
    IWICBitmapEncoder IWICBitmapEncoder_iface;
    LONG ref;
    IStream *stream;
    BmpFrameEncode *frame;
} BmpEncoder;

static inline BmpEncoder *impl_from_IWICBitmapEncoder(IWICBitmapEncoder *iface)
{
    return CONTAINING_RECORD(iface, BmpEncoder, IWICBitmapEncoder_iface);
}

static HRESULT WINAPI BmpEncoder_QueryInterface(IWICBitmapEncoder *iface, REFIID iid,
    void **ppv)
{
    BmpEncoder *This = impl_from_IWICBitmapEncoder(iface);
    TRACE("(%p,%s,%p)\n", iface, debugstr_guid(iid), ppv);

    if (!ppv) return E_INVALIDARG;

    if (IsEqualIID(&IID_IUnknown, iid) ||
        IsEqualIID(&IID_IWICBitmapEncoder, iid))
    {
        *ppv = &This->IWICBitmapEncoder_iface;
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
    BmpEncoder *This = impl_from_IWICBitmapEncoder(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) refcount=%lu\n", iface, ref);

    return ref;
}

static ULONG WINAPI BmpEncoder_Release(IWICBitmapEncoder *iface)
{
    BmpEncoder *This = impl_from_IWICBitmapEncoder(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) refcount=%lu\n", iface, ref);

    if (ref == 0)
    {
        if (This->stream) IStream_Release(This->stream);
        if (This->frame) IWICBitmapFrameEncode_Release(&This->frame->IWICBitmapFrameEncode_iface);
        free(This);
    }

    return ref;
}

static HRESULT WINAPI BmpEncoder_Initialize(IWICBitmapEncoder *iface,
    IStream *pIStream, WICBitmapEncoderCacheOption cacheOption)
{
    BmpEncoder *This = impl_from_IWICBitmapEncoder(iface);

    TRACE("(%p,%p,%u)\n", iface, pIStream, cacheOption);

    IStream_AddRef(pIStream);
    This->stream = pIStream;

    return S_OK;
}

static HRESULT WINAPI BmpEncoder_GetContainerFormat(IWICBitmapEncoder *iface,
    GUID *pguidContainerFormat)
{
    TRACE("(%p,%p)\n", iface, pguidContainerFormat);

    if (!pguidContainerFormat)
        return E_INVALIDARG;

    memcpy(pguidContainerFormat, &GUID_ContainerFormatBmp, sizeof(GUID));
    return S_OK;
}

static HRESULT WINAPI BmpEncoder_GetEncoderInfo(IWICBitmapEncoder *iface, IWICBitmapEncoderInfo **info)
{
    IWICComponentInfo *comp_info;
    HRESULT hr;

    TRACE("%p,%p\n", iface, info);

    if (!info) return E_INVALIDARG;

    hr = CreateComponentInfo(&CLSID_WICBmpEncoder, &comp_info);
    if (hr == S_OK)
    {
        hr = IWICComponentInfo_QueryInterface(comp_info, &IID_IWICBitmapEncoderInfo, (void **)info);
        IWICComponentInfo_Release(comp_info);
    }
    return hr;
}

static HRESULT WINAPI BmpEncoder_SetColorContexts(IWICBitmapEncoder *iface,
    UINT cCount, IWICColorContext **ppIColorContext)
{
    FIXME("(%p,%u,%p): stub\n", iface, cCount, ppIColorContext);
    return E_NOTIMPL;
}

static HRESULT WINAPI BmpEncoder_SetPalette(IWICBitmapEncoder *iface, IWICPalette *palette)
{
    BmpEncoder *This = impl_from_IWICBitmapEncoder(iface);

    TRACE("(%p,%p)\n", iface, palette);
    return This->stream ? WINCODEC_ERR_UNSUPPORTEDOPERATION : WINCODEC_ERR_NOTINITIALIZED;
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
    BmpEncoder *This = impl_from_IWICBitmapEncoder(iface);
    BmpFrameEncode *encode;
    HRESULT hr;
    static const PROPBAG2 opts[1] =
    {
        { PROPBAG2_TYPE_DATA, VT_BOOL, 0, 0, (LPOLESTR)L"EnableV5Header32bppBGRA" },
    };

    TRACE("(%p,%p,%p)\n", iface, ppIFrameEncode, ppIEncoderOptions);

    if (This->frame) return WINCODEC_ERR_UNSUPPORTEDOPERATION;

    if (!This->stream) return WINCODEC_ERR_NOTINITIALIZED;

    if (ppIEncoderOptions)
    {
        hr = CreatePropertyBag2(opts, ARRAY_SIZE(opts), ppIEncoderOptions);
        if (FAILED(hr)) return hr;
    }

    encode = malloc(sizeof(BmpFrameEncode));
    if (!encode)
    {
        IPropertyBag2_Release(*ppIEncoderOptions);
        *ppIEncoderOptions = NULL;
        return E_OUTOFMEMORY;
    }
    encode->IWICBitmapFrameEncode_iface.lpVtbl = &BmpFrameEncode_Vtbl;
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
    encode->colors = 0;
    encode->committed = FALSE;

    *ppIFrameEncode = &encode->IWICBitmapFrameEncode_iface;
    This->frame = encode;

    return S_OK;
}

static HRESULT WINAPI BmpEncoder_Commit(IWICBitmapEncoder *iface)
{
    BmpEncoder *This = impl_from_IWICBitmapEncoder(iface);
    TRACE("(%p)\n", iface);

    if (!This->frame || !This->frame->committed) return WINCODEC_ERR_WRONGSTATE;

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

HRESULT BmpEncoder_CreateInstance(REFIID iid, void** ppv)
{
    BmpEncoder *This;
    HRESULT ret;

    TRACE("(%s,%p)\n", debugstr_guid(iid), ppv);

    *ppv = NULL;

    This = malloc(sizeof(BmpEncoder));
    if (!This) return E_OUTOFMEMORY;

    This->IWICBitmapEncoder_iface.lpVtbl = &BmpEncoder_Vtbl;
    This->ref = 1;
    This->stream = NULL;
    This->frame = NULL;

    ret = IWICBitmapEncoder_QueryInterface(&This->IWICBitmapEncoder_iface, iid, ppv);
    IWICBitmapEncoder_Release(&This->IWICBitmapEncoder_iface);

    return ret;
}
