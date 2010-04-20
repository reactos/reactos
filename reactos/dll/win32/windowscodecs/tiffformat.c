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

#include "config.h"
#include "wine/port.h"

#include <stdarg.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_TIFFIO_H
#include <tiffio.h>
#endif

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "objbase.h"
#include "wincodec.h"

#include "wincodecs_private.h"

#include "wine/debug.h"
#include "wine/library.h"

WINE_DEFAULT_DEBUG_CHANNEL(wincodecs);

#ifdef SONAME_LIBTIFF

static CRITICAL_SECTION init_tiff_cs;
static CRITICAL_SECTION_DEBUG init_tiff_cs_debug =
{
    0, 0, &init_tiff_cs,
    { &init_tiff_cs_debug.ProcessLocksList,
      &init_tiff_cs_debug.ProcessLocksList },
    0, 0, { (DWORD_PTR)(__FILE__ ": init_tiff_cs") }
};
static CRITICAL_SECTION init_tiff_cs = { &init_tiff_cs_debug, -1, 0, 0, 0, 0 };

static void *libtiff_handle;
#define MAKE_FUNCPTR(f) static typeof(f) * p##f
MAKE_FUNCPTR(TIFFClientOpen);
MAKE_FUNCPTR(TIFFClose);
MAKE_FUNCPTR(TIFFCurrentDirectory);
MAKE_FUNCPTR(TIFFGetField);
MAKE_FUNCPTR(TIFFReadDirectory);
MAKE_FUNCPTR(TIFFReadEncodedStrip);
MAKE_FUNCPTR(TIFFSetDirectory);
#undef MAKE_FUNCPTR

static void *load_libtiff(void)
{
    void *result;

    EnterCriticalSection(&init_tiff_cs);

    if (!libtiff_handle &&
        (libtiff_handle = wine_dlopen(SONAME_LIBTIFF, RTLD_NOW, NULL, 0)) != NULL)
    {

#define LOAD_FUNCPTR(f) \
    if((p##f = wine_dlsym(libtiff_handle, #f, NULL, 0)) == NULL) { \
        ERR("failed to load symbol %s\n", #f); \
        libtiff_handle = NULL; \
        LeaveCriticalSection(&init_tiff_cs); \
        return NULL; \
    }
        LOAD_FUNCPTR(TIFFClientOpen);
        LOAD_FUNCPTR(TIFFClose);
        LOAD_FUNCPTR(TIFFCurrentDirectory);
        LOAD_FUNCPTR(TIFFGetField);
        LOAD_FUNCPTR(TIFFReadDirectory);
        LOAD_FUNCPTR(TIFFReadEncodedStrip);
        LOAD_FUNCPTR(TIFFSetDirectory);
#undef LOAD_FUNCPTR

    }

    result = libtiff_handle;

    LeaveCriticalSection(&init_tiff_cs);
    return result;
}

static tsize_t tiff_stream_read(thandle_t client_data, tdata_t data, tsize_t size)
{
    IStream *stream = (IStream*)client_data;
    ULONG bytes_read;
    HRESULT hr;

    hr = IStream_Read(stream, data, size, &bytes_read);
    if (FAILED(hr)) bytes_read = 0;
    return bytes_read;
}

static tsize_t tiff_stream_write(thandle_t client_data, tdata_t data, tsize_t size)
{
    IStream *stream = (IStream*)client_data;
    ULONG bytes_written;
    HRESULT hr;

    hr = IStream_Write(stream, data, size, &bytes_written);
    if (FAILED(hr)) bytes_written = 0;
    return bytes_written;
}

static toff_t tiff_stream_seek(thandle_t client_data, toff_t offset, int whence)
{
    IStream *stream = (IStream*)client_data;
    LARGE_INTEGER move;
    DWORD origin;
    ULARGE_INTEGER new_position;
    HRESULT hr;

    move.QuadPart = offset;
    switch (whence)
    {
        case SEEK_SET:
            origin = STREAM_SEEK_SET;
            break;
        case SEEK_CUR:
            origin = STREAM_SEEK_CUR;
            break;
        case SEEK_END:
            origin = STREAM_SEEK_END;
            break;
        default:
            ERR("unknown whence value %i\n", whence);
            return -1;
    }

    hr = IStream_Seek(stream, move, origin, &new_position);
    if (SUCCEEDED(hr)) return new_position.QuadPart;
    else return -1;
}

static int tiff_stream_close(thandle_t client_data)
{
    /* Caller is responsible for releasing the stream object. */
    return 0;
}

static toff_t tiff_stream_size(thandle_t client_data)
{
    IStream *stream = (IStream*)client_data;
    STATSTG statstg;
    HRESULT hr;

    hr = IStream_Stat(stream, &statstg, STATFLAG_NONAME);

    if (SUCCEEDED(hr)) return statstg.cbSize.QuadPart;
    else return -1;
}

static int tiff_stream_map(thandle_t client_data, tdata_t *addr, toff_t *size)
{
    /* Cannot mmap streams */
    return 0;
}

static void tiff_stream_unmap(thandle_t client_data, tdata_t addr, toff_t size)
{
    /* No need to ever do this, since we can't map things. */
}

static TIFF* tiff_open_stream(IStream *stream, const char *mode)
{
    return pTIFFClientOpen("<IStream object>", mode, stream, tiff_stream_read,
        tiff_stream_write, tiff_stream_seek, tiff_stream_close,
        tiff_stream_size, tiff_stream_map, tiff_stream_unmap);
}

typedef struct {
    const IWICBitmapDecoderVtbl *lpVtbl;
    LONG ref;
    IStream *stream;
    CRITICAL_SECTION lock; /* Must be held when tiff is used or initiailzed is set */
    TIFF *tiff;
    BOOL initialized;
} TiffDecoder;

typedef struct {
    const WICPixelFormatGUID *format;
    int bpp;
    int indexed;
    int reverse_bgr;
    UINT width, height;
    UINT tile_width, tile_height;
    UINT tile_stride;
    UINT tile_size;
} tiff_decode_info;

typedef struct {
    const IWICBitmapFrameDecodeVtbl *lpVtbl;
    LONG ref;
    TiffDecoder *parent;
    UINT index;
    tiff_decode_info decode_info;
    INT cached_tile_x, cached_tile_y;
    BYTE *cached_tile;
} TiffFrameDecode;

static const IWICBitmapFrameDecodeVtbl TiffFrameDecode_Vtbl;

static HRESULT tiff_get_decode_info(TIFF *tiff, tiff_decode_info *decode_info)
{
    uint16 photometric, bps, samples, planar;
    int ret;

    decode_info->indexed = 0;
    decode_info->reverse_bgr = 0;

    ret = pTIFFGetField(tiff, TIFFTAG_PHOTOMETRIC, &photometric);
    if (!ret)
    {
        WARN("missing PhotometricInterpretation tag\n");
        return E_FAIL;
    }

    ret = pTIFFGetField(tiff, TIFFTAG_BITSPERSAMPLE, &bps);
    if (!ret) bps = 1;

    switch(photometric)
    {
    case 1: /* BlackIsZero */
        decode_info->bpp = bps;
        switch (bps)
        {
        case 1:
            decode_info->format = &GUID_WICPixelFormatBlackWhite;
            break;
        case 4:
            decode_info->format = &GUID_WICPixelFormat4bppGray;
            break;
        case 8:
            decode_info->format = &GUID_WICPixelFormat8bppGray;
            break;
        default:
            FIXME("unhandled greyscale bit count %u\n", bps);
            return E_FAIL;
        }
        break;
    case 2: /* RGB */
        if (bps != 8)
        {
            FIXME("unhandled RGB bit count %u\n", bps);
            return E_FAIL;
        }
        ret = pTIFFGetField(tiff, TIFFTAG_SAMPLESPERPIXEL, &samples);
        if (samples != 3)
        {
            FIXME("unhandled RGB sample count %u\n", samples);
            return E_FAIL;
        }
        ret = pTIFFGetField(tiff, TIFFTAG_PLANARCONFIG, &planar);
        if (!ret) planar = 1;
        if (planar != 1)
        {
            FIXME("unhandled planar configuration %u\n", planar);
            return E_FAIL;
        }
        decode_info->bpp = bps * samples;
        decode_info->reverse_bgr = 1;
        decode_info->format = &GUID_WICPixelFormat24bppBGR;
        break;
    case 3: /* RGB Palette */
        decode_info->indexed = 1;
        decode_info->bpp = bps;
        switch (bps)
        {
        case 4:
            decode_info->format = &GUID_WICPixelFormat4bppIndexed;
            break;
        case 8:
            decode_info->format = &GUID_WICPixelFormat8bppIndexed;
            break;
        default:
            FIXME("unhandled indexed bit count %u\n", bps);
            return E_FAIL;
        }
        break;
    case 0: /* WhiteIsZero */
    case 4: /* Transparency mask */
    case 5: /* CMYK */
    case 6: /* YCbCr */
    case 8: /* CIELab */
    default:
        FIXME("unhandled PhotometricInterpretation %u\n", photometric);
        return E_FAIL;
    }

    ret = pTIFFGetField(tiff, TIFFTAG_IMAGEWIDTH, &decode_info->width);
    if (!ret)
    {
        WARN("missing image width\n");
        return E_FAIL;
    }

    ret = pTIFFGetField(tiff, TIFFTAG_IMAGELENGTH, &decode_info->height);
    if (!ret)
    {
        WARN("missing image length\n");
        return E_FAIL;
    }

    ret = pTIFFGetField(tiff, TIFFTAG_ROWSPERSTRIP, &decode_info->tile_height);
    if (ret)
    {
        decode_info->tile_width = decode_info->width;
        decode_info->tile_stride = ((decode_info->bpp * decode_info->tile_width + 7)/8);
        decode_info->tile_size = decode_info->tile_height * decode_info->tile_stride;
    }
    else
    {
        /* Probably a tiled image */
        FIXME("missing RowsPerStrip value\n");
        return E_FAIL;
    }

    return S_OK;
}

static HRESULT WINAPI TiffDecoder_QueryInterface(IWICBitmapDecoder *iface, REFIID iid,
    void **ppv)
{
    TiffDecoder *This = (TiffDecoder*)iface;
    TRACE("(%p,%s,%p)\n", iface, debugstr_guid(iid), ppv);

    if (!ppv) return E_INVALIDARG;

    if (IsEqualIID(&IID_IUnknown, iid) || IsEqualIID(&IID_IWICBitmapDecoder, iid))
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

static ULONG WINAPI TiffDecoder_AddRef(IWICBitmapDecoder *iface)
{
    TiffDecoder *This = (TiffDecoder*)iface;
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) refcount=%u\n", iface, ref);

    return ref;
}

static ULONG WINAPI TiffDecoder_Release(IWICBitmapDecoder *iface)
{
    TiffDecoder *This = (TiffDecoder*)iface;
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) refcount=%u\n", iface, ref);

    if (ref == 0)
    {
        if (This->tiff) pTIFFClose(This->tiff);
        if (This->stream) IStream_Release(This->stream);
        This->lock.DebugInfo->Spare[0] = 0;
        DeleteCriticalSection(&This->lock);
        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}

static HRESULT WINAPI TiffDecoder_QueryCapability(IWICBitmapDecoder *iface, IStream *pIStream,
    DWORD *pdwCapability)
{
    FIXME("(%p,%p,%p): stub\n", iface, pIStream, pdwCapability);
    return E_NOTIMPL;
}

static HRESULT WINAPI TiffDecoder_Initialize(IWICBitmapDecoder *iface, IStream *pIStream,
    WICDecodeOptions cacheOptions)
{
    TiffDecoder *This = (TiffDecoder*)iface;
    TIFF *tiff;
    HRESULT hr=S_OK;

    TRACE("(%p,%p,%x): stub\n", iface, pIStream, cacheOptions);

    EnterCriticalSection(&This->lock);

    if (This->initialized)
    {
        hr = WINCODEC_ERR_WRONGSTATE;
        goto exit;
    }

    tiff = tiff_open_stream(pIStream, "r");

    if (!tiff)
    {
        hr = E_FAIL;
        goto exit;
    }

    This->tiff = tiff;
    This->stream = pIStream;
    IStream_AddRef(pIStream);
    This->initialized = TRUE;

exit:
    LeaveCriticalSection(&This->lock);
    return hr;
}

static HRESULT WINAPI TiffDecoder_GetContainerFormat(IWICBitmapDecoder *iface,
    GUID *pguidContainerFormat)
{
    memcpy(pguidContainerFormat, &GUID_ContainerFormatTiff, sizeof(GUID));
    return S_OK;
}

static HRESULT WINAPI TiffDecoder_GetDecoderInfo(IWICBitmapDecoder *iface,
    IWICBitmapDecoderInfo **ppIDecoderInfo)
{
    FIXME("(%p,%p): stub\n", iface, ppIDecoderInfo);
    return E_NOTIMPL;
}

static HRESULT WINAPI TiffDecoder_CopyPalette(IWICBitmapDecoder *iface,
    IWICPalette *pIPalette)
{
    FIXME("(%p,%p): stub\n", iface, pIPalette);
    return E_NOTIMPL;
}

static HRESULT WINAPI TiffDecoder_GetMetadataQueryReader(IWICBitmapDecoder *iface,
    IWICMetadataQueryReader **ppIMetadataQueryReader)
{
    FIXME("(%p,%p): stub\n", iface, ppIMetadataQueryReader);
    return E_NOTIMPL;
}

static HRESULT WINAPI TiffDecoder_GetPreview(IWICBitmapDecoder *iface,
    IWICBitmapSource **ppIBitmapSource)
{
    FIXME("(%p,%p): stub\n", iface, ppIBitmapSource);
    return E_NOTIMPL;
}

static HRESULT WINAPI TiffDecoder_GetColorContexts(IWICBitmapDecoder *iface,
    UINT cCount, IWICColorContext **ppIColorContexts, UINT *pcActualCount)
{
    FIXME("(%p,%u,%p,%p)\n", iface, cCount, ppIColorContexts, pcActualCount);
    return E_NOTIMPL;
}

static HRESULT WINAPI TiffDecoder_GetThumbnail(IWICBitmapDecoder *iface,
    IWICBitmapSource **ppIThumbnail)
{
    TRACE("(%p,%p)\n", iface, ppIThumbnail);
    return WINCODEC_ERR_CODECNOTHUMBNAIL;
}

static HRESULT WINAPI TiffDecoder_GetFrameCount(IWICBitmapDecoder *iface,
    UINT *pCount)
{
    TiffDecoder *This = (TiffDecoder*)iface;

    if (!This->tiff)
    {
        WARN("(%p) <-- WINCODEC_ERR_WRONGSTATE\n", iface);
        return WINCODEC_ERR_WRONGSTATE;
    }

    EnterCriticalSection(&This->lock);
    while (pTIFFReadDirectory(This->tiff)) { }
    *pCount = pTIFFCurrentDirectory(This->tiff)+1;
    LeaveCriticalSection(&This->lock);

    TRACE("(%p) <-- %i\n", iface, *pCount);

    return S_OK;
}

static HRESULT WINAPI TiffDecoder_GetFrame(IWICBitmapDecoder *iface,
    UINT index, IWICBitmapFrameDecode **ppIBitmapFrame)
{
    TiffDecoder *This = (TiffDecoder*)iface;
    TiffFrameDecode *result;
    int res;
    tiff_decode_info decode_info;
    HRESULT hr;

    TRACE("(%p,%u,%p)\n", iface, index, ppIBitmapFrame);

    if (!This->tiff)
        return WINCODEC_ERR_WRONGSTATE;

    EnterCriticalSection(&This->lock);
    res = pTIFFSetDirectory(This->tiff, index);
    if (!res) hr = E_INVALIDARG;
    else hr = tiff_get_decode_info(This->tiff, &decode_info);
    LeaveCriticalSection(&This->lock);

    if (SUCCEEDED(hr))
    {
        result = HeapAlloc(GetProcessHeap(), 0, sizeof(TiffFrameDecode));

        if (result)
        {
            result->lpVtbl = &TiffFrameDecode_Vtbl;
            result->ref = 1;
            result->parent = This;
            result->index = index;
            result->decode_info = decode_info;
            result->cached_tile_x = -1;
            result->cached_tile = HeapAlloc(GetProcessHeap(), 0, decode_info.tile_size);

            if (result->cached_tile)
                *ppIBitmapFrame = (IWICBitmapFrameDecode*)result;
            else
            {
                hr = E_OUTOFMEMORY;
                HeapFree(GetProcessHeap(), 0, result);
            }
        }
        else hr = E_OUTOFMEMORY;
    }

    if (FAILED(hr)) *ppIBitmapFrame = NULL;

    return hr;
}

static const IWICBitmapDecoderVtbl TiffDecoder_Vtbl = {
    TiffDecoder_QueryInterface,
    TiffDecoder_AddRef,
    TiffDecoder_Release,
    TiffDecoder_QueryCapability,
    TiffDecoder_Initialize,
    TiffDecoder_GetContainerFormat,
    TiffDecoder_GetDecoderInfo,
    TiffDecoder_CopyPalette,
    TiffDecoder_GetMetadataQueryReader,
    TiffDecoder_GetPreview,
    TiffDecoder_GetColorContexts,
    TiffDecoder_GetThumbnail,
    TiffDecoder_GetFrameCount,
    TiffDecoder_GetFrame
};

static HRESULT WINAPI TiffFrameDecode_QueryInterface(IWICBitmapFrameDecode *iface, REFIID iid,
    void **ppv)
{
    TiffFrameDecode *This = (TiffFrameDecode*)iface;
    TRACE("(%p,%s,%p)\n", iface, debugstr_guid(iid), ppv);

    if (!ppv) return E_INVALIDARG;

    if (IsEqualIID(&IID_IUnknown, iid) ||
        IsEqualIID(&IID_IWICBitmapSource, iid) ||
        IsEqualIID(&IID_IWICBitmapFrameDecode, iid))
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

static ULONG WINAPI TiffFrameDecode_AddRef(IWICBitmapFrameDecode *iface)
{
    TiffFrameDecode *This = (TiffFrameDecode*)iface;
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) refcount=%u\n", iface, ref);

    return ref;
}

static ULONG WINAPI TiffFrameDecode_Release(IWICBitmapFrameDecode *iface)
{
    TiffFrameDecode *This = (TiffFrameDecode*)iface;
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) refcount=%u\n", iface, ref);

    if (ref == 0)
    {
        HeapFree(GetProcessHeap(), 0, This->cached_tile);
        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}

static HRESULT WINAPI TiffFrameDecode_GetSize(IWICBitmapFrameDecode *iface,
    UINT *puiWidth, UINT *puiHeight)
{
    TiffFrameDecode *This = (TiffFrameDecode*)iface;

    *puiWidth = This->decode_info.width;
    *puiHeight = This->decode_info.height;

    TRACE("(%p) <-- %ux%u\n", iface, *puiWidth, *puiHeight);

    return S_OK;
}

static HRESULT WINAPI TiffFrameDecode_GetPixelFormat(IWICBitmapFrameDecode *iface,
    WICPixelFormatGUID *pPixelFormat)
{
    TiffFrameDecode *This = (TiffFrameDecode*)iface;

    memcpy(pPixelFormat, This->decode_info.format, sizeof(GUID));

    TRACE("(%p) <-- %s\n", This, debugstr_guid(This->decode_info.format));

    return S_OK;
}

static HRESULT WINAPI TiffFrameDecode_GetResolution(IWICBitmapFrameDecode *iface,
    double *pDpiX, double *pDpiY)
{
    FIXME("(%p,%p,%p)\n", iface, pDpiX, pDpiY);
    return E_NOTIMPL;
}

static HRESULT WINAPI TiffFrameDecode_CopyPalette(IWICBitmapFrameDecode *iface,
    IWICPalette *pIPalette)
{
    FIXME("(%p,%p)\n", iface, pIPalette);
    return E_NOTIMPL;
}

static HRESULT TiffFrameDecode_ReadTile(TiffFrameDecode *This, UINT tile_x, UINT tile_y)
{
    HRESULT hr=S_OK;
    tsize_t ret;

    ret = pTIFFSetDirectory(This->parent->tiff, This->index);

    if (ret == -1)
        hr = E_FAIL;

    if (hr == S_OK)
    {
        ret = pTIFFReadEncodedStrip(This->parent->tiff, tile_y, This->cached_tile, This->decode_info.tile_size);

        if (ret == -1)
            hr = E_FAIL;
    }

    if (hr == S_OK && This->decode_info.reverse_bgr)
    {
        if (This->decode_info.format == &GUID_WICPixelFormat24bppBGR)
        {
            UINT i, total_pixels;
            BYTE *pixel, temp;

            total_pixels = This->decode_info.tile_width * This->decode_info.tile_height;
            pixel = This->cached_tile;
            for (i=0; i<total_pixels; i++)
            {
                temp = pixel[2];
                pixel[2] = pixel[0];
                pixel[0] = temp;
                pixel += 3;
            }
        }
    }

    if (hr == S_OK)
    {
        This->cached_tile_x = tile_x;
        This->cached_tile_y = tile_y;
    }

    return hr;
}

static HRESULT WINAPI TiffFrameDecode_CopyPixels(IWICBitmapFrameDecode *iface,
    const WICRect *prc, UINT cbStride, UINT cbBufferSize, BYTE *pbBuffer)
{
    TiffFrameDecode *This = (TiffFrameDecode*)iface;
    UINT min_tile_x, max_tile_x, min_tile_y, max_tile_y;
    UINT tile_x, tile_y;
    WICRect rc;
    HRESULT hr=S_OK;
    BYTE *dst_tilepos;
    UINT bytesperrow;

    TRACE("(%p,%p,%u,%u,%p)\n", iface, prc, cbStride, cbBufferSize, pbBuffer);

    if (prc->X < 0 || prc->Y < 0 || prc->X+prc->Width > This->decode_info.width ||
        prc->Y+prc->Height > This->decode_info.height)
        return E_INVALIDARG;

    bytesperrow = ((This->decode_info.bpp * prc->Width)+7)/8;

    if (cbStride < bytesperrow)
        return E_INVALIDARG;

    if ((cbStride * prc->Height) > cbBufferSize)
        return E_INVALIDARG;

    min_tile_x = prc->X / This->decode_info.tile_width;
    min_tile_y = prc->Y / This->decode_info.tile_height;
    max_tile_x = (prc->X+prc->Width-1) / This->decode_info.tile_width;
    max_tile_y = (prc->Y+prc->Height-1) / This->decode_info.tile_height;

    EnterCriticalSection(&This->parent->lock);

    for (tile_x=min_tile_x; tile_x <= max_tile_x; tile_x++)
    {
        for (tile_y=min_tile_y; tile_y <= max_tile_y; tile_y++)
        {
            if (tile_x != This->cached_tile_x || tile_y != This->cached_tile_y)
            {
                hr = TiffFrameDecode_ReadTile(This, tile_x, tile_y);
            }

            if (SUCCEEDED(hr))
            {
                if (prc->X < tile_x * This->decode_info.tile_width)
                    rc.X = 0;
                else
                    rc.X = prc->X - tile_x * This->decode_info.tile_width;

                if (prc->Y < tile_y * This->decode_info.tile_height)
                    rc.Y = 0;
                else
                    rc.Y = prc->Y - tile_y * This->decode_info.tile_height;

                if (prc->X+prc->Width > (tile_x+1) * This->decode_info.tile_width)
                    rc.Width = This->decode_info.tile_width - rc.X;
                else
                    rc.Width = prc->Width + rc.X - prc->X;

                if (prc->Y+prc->Height > (tile_y+1) * This->decode_info.tile_height)
                    rc.Height = This->decode_info.tile_height - rc.Y;
                else
                    rc.Height = prc->Height + rc.Y - prc->Y;

                dst_tilepos = pbBuffer + (cbStride * (rc.Y - prc->Y)) +
                    ((This->decode_info.bpp * (rc.X - prc->X) + 7) / 8);

                hr = copy_pixels(This->decode_info.bpp, This->cached_tile,
                    This->decode_info.tile_width, This->decode_info.tile_height, This->decode_info.tile_stride,
                    &rc, cbStride, cbBufferSize, dst_tilepos);
            }

            if (FAILED(hr))
            {
                LeaveCriticalSection(&This->parent->lock);
                TRACE("<-- 0x%x\n", hr);
                return hr;
            }
        }
    }

    LeaveCriticalSection(&This->parent->lock);

    return S_OK;
}

static HRESULT WINAPI TiffFrameDecode_GetMetadataQueryReader(IWICBitmapFrameDecode *iface,
    IWICMetadataQueryReader **ppIMetadataQueryReader)
{
    FIXME("(%p,%p): stub\n", iface, ppIMetadataQueryReader);
    return E_NOTIMPL;
}

static HRESULT WINAPI TiffFrameDecode_GetColorContexts(IWICBitmapFrameDecode *iface,
    UINT cCount, IWICColorContext **ppIColorContexts, UINT *pcActualCount)
{
    FIXME("(%p,%u,%p,%p): stub\n", iface, cCount, ppIColorContexts, pcActualCount);
    return E_NOTIMPL;
}

static HRESULT WINAPI TiffFrameDecode_GetThumbnail(IWICBitmapFrameDecode *iface,
    IWICBitmapSource **ppIThumbnail)
{
    FIXME("(%p,%p): stub\n", iface, ppIThumbnail);
    return E_NOTIMPL;
}

static const IWICBitmapFrameDecodeVtbl TiffFrameDecode_Vtbl = {
    TiffFrameDecode_QueryInterface,
    TiffFrameDecode_AddRef,
    TiffFrameDecode_Release,
    TiffFrameDecode_GetSize,
    TiffFrameDecode_GetPixelFormat,
    TiffFrameDecode_GetResolution,
    TiffFrameDecode_CopyPalette,
    TiffFrameDecode_CopyPixels,
    TiffFrameDecode_GetMetadataQueryReader,
    TiffFrameDecode_GetColorContexts,
    TiffFrameDecode_GetThumbnail
};

HRESULT TiffDecoder_CreateInstance(IUnknown *pUnkOuter, REFIID iid, void** ppv)
{
    HRESULT ret;
    TiffDecoder *This;

    TRACE("(%p,%s,%p)\n", pUnkOuter, debugstr_guid(iid), ppv);

    *ppv = NULL;

    if (pUnkOuter) return CLASS_E_NOAGGREGATION;

    if (!load_libtiff())
    {
        ERR("Failed reading TIFF because unable to load %s\n",SONAME_LIBTIFF);
        return E_FAIL;
    }

    This = HeapAlloc(GetProcessHeap(), 0, sizeof(TiffDecoder));
    if (!This) return E_OUTOFMEMORY;

    This->lpVtbl = &TiffDecoder_Vtbl;
    This->ref = 1;
    This->stream = NULL;
    InitializeCriticalSection(&This->lock);
    This->lock.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": TiffDecoder.lock");
    This->tiff = NULL;
    This->initialized = FALSE;

    ret = IUnknown_QueryInterface((IUnknown*)This, iid, ppv);
    IUnknown_Release((IUnknown*)This);

    return ret;
}

#else /* !SONAME_LIBTIFF */

HRESULT TiffDecoder_CreateInstance(IUnknown *pUnkOuter, REFIID iid, void** ppv)
{
    ERR("Trying to load TIFF picture, but Wine was compiled without TIFF support.\n");
    return E_FAIL;
}

#endif
