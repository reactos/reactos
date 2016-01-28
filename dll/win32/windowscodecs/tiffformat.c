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

#include "wincodecs_private.h"

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_TIFFIO_H
#include <tiffio.h>
#endif

#ifdef SONAME_LIBTIFF

/* Workaround for broken libtiff 4.x headers on some 64-bit hosts which
 * define TIFF_UINT64_T/toff_t as 32-bit for 32-bit builds, while they
 * are supposed to be always 64-bit.
 * TIFF_UINT64_T doesn't exist in libtiff 3.x, it was introduced in 4.x.
 */
#ifdef TIFF_UINT64_T
# undef toff_t
# define toff_t UINT64
#endif

static CRITICAL_SECTION init_tiff_cs;
static CRITICAL_SECTION_DEBUG init_tiff_cs_debug =
{
    0, 0, &init_tiff_cs,
    { &init_tiff_cs_debug.ProcessLocksList,
      &init_tiff_cs_debug.ProcessLocksList },
    0, 0, { (DWORD_PTR)(__FILE__ ": init_tiff_cs") }
};
static CRITICAL_SECTION init_tiff_cs = { &init_tiff_cs_debug, -1, 0, 0, 0, 0 };

static const WCHAR wszTiffCompressionMethod[] = {'T','i','f','f','C','o','m','p','r','e','s','s','i','o','n','M','e','t','h','o','d',0};
static const WCHAR wszCompressionQuality[] = {'C','o','m','p','r','e','s','s','i','o','n','Q','u','a','l','i','t','y',0};

static void *libtiff_handle;
#define MAKE_FUNCPTR(f) static typeof(f) * p##f
MAKE_FUNCPTR(TIFFClientOpen);
MAKE_FUNCPTR(TIFFClose);
MAKE_FUNCPTR(TIFFCurrentDirOffset);
MAKE_FUNCPTR(TIFFGetField);
MAKE_FUNCPTR(TIFFIsByteSwapped);
MAKE_FUNCPTR(TIFFNumberOfDirectories);
MAKE_FUNCPTR(TIFFReadDirectory);
MAKE_FUNCPTR(TIFFReadEncodedStrip);
MAKE_FUNCPTR(TIFFReadEncodedTile);
MAKE_FUNCPTR(TIFFSetDirectory);
MAKE_FUNCPTR(TIFFSetField);
MAKE_FUNCPTR(TIFFWriteDirectory);
MAKE_FUNCPTR(TIFFWriteScanline);
#undef MAKE_FUNCPTR

static void *load_libtiff(void)
{
    void *result;

    EnterCriticalSection(&init_tiff_cs);

    if (!libtiff_handle &&
        (libtiff_handle = wine_dlopen(SONAME_LIBTIFF, RTLD_NOW, NULL, 0)) != NULL)
    {
        void * (*pTIFFSetWarningHandler)(void *);
        void * (*pTIFFSetWarningHandlerExt)(void *);

#define LOAD_FUNCPTR(f) \
    if((p##f = wine_dlsym(libtiff_handle, #f, NULL, 0)) == NULL) { \
        ERR("failed to load symbol %s\n", #f); \
        libtiff_handle = NULL; \
        LeaveCriticalSection(&init_tiff_cs); \
        return NULL; \
    }
        LOAD_FUNCPTR(TIFFClientOpen);
        LOAD_FUNCPTR(TIFFClose);
        LOAD_FUNCPTR(TIFFCurrentDirOffset);
        LOAD_FUNCPTR(TIFFGetField);
        LOAD_FUNCPTR(TIFFIsByteSwapped);
        LOAD_FUNCPTR(TIFFNumberOfDirectories);
        LOAD_FUNCPTR(TIFFReadDirectory);
        LOAD_FUNCPTR(TIFFReadEncodedStrip);
        LOAD_FUNCPTR(TIFFReadEncodedTile);
        LOAD_FUNCPTR(TIFFSetDirectory);
        LOAD_FUNCPTR(TIFFSetField);
        LOAD_FUNCPTR(TIFFWriteDirectory);
        LOAD_FUNCPTR(TIFFWriteScanline);
#undef LOAD_FUNCPTR

        if ((pTIFFSetWarningHandler = wine_dlsym(libtiff_handle, "TIFFSetWarningHandler", NULL, 0)))
            pTIFFSetWarningHandler(NULL);
        if ((pTIFFSetWarningHandlerExt = wine_dlsym(libtiff_handle, "TIFFSetWarningHandlerExt", NULL, 0)))
            pTIFFSetWarningHandlerExt(NULL);
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
    LARGE_INTEGER zero;

    zero.QuadPart = 0;
    IStream_Seek(stream, zero, STREAM_SEEK_SET, NULL);

    return pTIFFClientOpen("<IStream object>", mode, stream, tiff_stream_read,
        tiff_stream_write, (void *)tiff_stream_seek, tiff_stream_close,
        (void *)tiff_stream_size, (void *)tiff_stream_map, (void *)tiff_stream_unmap);
}

typedef struct {
    IWICBitmapDecoder IWICBitmapDecoder_iface;
    LONG ref;
    IStream *stream;
    CRITICAL_SECTION lock; /* Must be held when tiff is used or initialized is set */
    TIFF *tiff;
    BOOL initialized;
} TiffDecoder;

typedef struct {
    const WICPixelFormatGUID *format;
    int bps;
    int samples;
    int bpp, source_bpp;
    int planar;
    int indexed;
    int reverse_bgr;
    int invert_grayscale;
    UINT width, height;
    UINT tile_width, tile_height;
    UINT tile_stride;
    UINT tile_size;
    int tiled;
    UINT tiles_across;
    UINT resolution_unit;
    float xres, yres;
} tiff_decode_info;

typedef struct {
    IWICBitmapFrameDecode IWICBitmapFrameDecode_iface;
    IWICMetadataBlockReader IWICMetadataBlockReader_iface;
    LONG ref;
    TiffDecoder *parent;
    UINT index;
    tiff_decode_info decode_info;
    INT cached_tile_x, cached_tile_y;
    BYTE *cached_tile;
} TiffFrameDecode;

static const IWICBitmapFrameDecodeVtbl TiffFrameDecode_Vtbl;
static const IWICMetadataBlockReaderVtbl TiffFrameDecode_BlockVtbl;

static inline TiffDecoder *impl_from_IWICBitmapDecoder(IWICBitmapDecoder *iface)
{
    return CONTAINING_RECORD(iface, TiffDecoder, IWICBitmapDecoder_iface);
}

static inline TiffFrameDecode *impl_from_IWICBitmapFrameDecode(IWICBitmapFrameDecode *iface)
{
    return CONTAINING_RECORD(iface, TiffFrameDecode, IWICBitmapFrameDecode_iface);
}

static inline TiffFrameDecode *impl_from_IWICMetadataBlockReader(IWICMetadataBlockReader *iface)
{
    return CONTAINING_RECORD(iface, TiffFrameDecode, IWICMetadataBlockReader_iface);
}

static HRESULT tiff_get_decode_info(TIFF *tiff, tiff_decode_info *decode_info)
{
    uint16 photometric, bps, samples, planar;
    uint16 extra_sample_count, extra_sample, *extra_samples;
    int ret;

    decode_info->indexed = 0;
    decode_info->reverse_bgr = 0;
    decode_info->invert_grayscale = 0;
    decode_info->tiled = 0;

    ret = pTIFFGetField(tiff, TIFFTAG_PHOTOMETRIC, &photometric);
    if (!ret)
    {
        WARN("missing PhotometricInterpretation tag\n");
        return E_FAIL;
    }

    ret = pTIFFGetField(tiff, TIFFTAG_BITSPERSAMPLE, &bps);
    if (!ret) bps = 1;
    decode_info->bps = bps;

    ret = pTIFFGetField(tiff, TIFFTAG_SAMPLESPERPIXEL, &samples);
    if (!ret) samples = 1;
    decode_info->samples = samples;

    if (samples == 1)
        planar = 1;
    else
    {
        ret = pTIFFGetField(tiff, TIFFTAG_PLANARCONFIG, &planar);
        if (!ret) planar = 1;
        if (planar != 1)
        {
            FIXME("unhandled planar configuration %u\n", planar);
            return E_FAIL;
        }
    }
    decode_info->planar = planar;

    switch(photometric)
    {
    case 0: /* WhiteIsZero */
        decode_info->invert_grayscale = 1;
        /* fall through */
    case 1: /* BlackIsZero */
        if (samples == 2)
        {
            ret = pTIFFGetField(tiff, TIFFTAG_EXTRASAMPLES, &extra_sample_count, &extra_samples);
            if (!ret)
            {
                extra_sample_count = 1;
                extra_sample = 0;
                extra_samples = &extra_sample;
            }
        }
        else if (samples != 1)
        {
            FIXME("unhandled %dbpp sample count %u\n", bps, samples);
            return E_FAIL;
        }

        decode_info->bpp = bps * samples;
        decode_info->source_bpp = decode_info->bpp;
        switch (bps)
        {
        case 1:
            if (samples != 1)
            {
                FIXME("unhandled 1bpp sample count %u\n", samples);
                return E_FAIL;
            }
            decode_info->format = &GUID_WICPixelFormatBlackWhite;
            break;
        case 4:
            if (samples != 1)
            {
                FIXME("unhandled 4bpp grayscale sample count %u\n", samples);
                return E_FAIL;
            }
            decode_info->format = &GUID_WICPixelFormat4bppGray;
            break;
        case 8:
            if (samples == 1)
                decode_info->format = &GUID_WICPixelFormat8bppGray;
            else
            {
                decode_info->bpp = 32;

                switch(extra_samples[0])
                {
                case 1: /* Associated (pre-multiplied) alpha data */
                    decode_info->format = &GUID_WICPixelFormat32bppPBGRA;
                    break;
                case 0: /* Unspecified data */
                case 2: /* Unassociated alpha data */
                    decode_info->format = &GUID_WICPixelFormat32bppBGRA;
                    break;
                default:
                    FIXME("unhandled extra sample type %u\n", extra_samples[0]);
                    return E_FAIL;
                }
            }
            break;
        default:
            FIXME("unhandled greyscale bit count %u\n", bps);
            return E_FAIL;
        }
        break;
    case 2: /* RGB */
        decode_info->bpp = bps * samples;

        if (samples == 4)
        {
            ret = pTIFFGetField(tiff, TIFFTAG_EXTRASAMPLES, &extra_sample_count, &extra_samples);
            if (!ret)
            {
                extra_sample_count = 1;
                extra_sample = 0;
                extra_samples = &extra_sample;
            }
        }
        else if (samples != 3)
        {
            FIXME("unhandled RGB sample count %u\n", samples);
            return E_FAIL;
        }

        switch(bps)
        {
        case 8:
            decode_info->reverse_bgr = 1;
            if (samples == 3)
                decode_info->format = &GUID_WICPixelFormat24bppBGR;
            else
                switch(extra_samples[0])
                {
                case 1: /* Associated (pre-multiplied) alpha data */
                    decode_info->format = &GUID_WICPixelFormat32bppPBGRA;
                    break;
                case 0: /* Unspecified data */
                case 2: /* Unassociated alpha data */
                    decode_info->format = &GUID_WICPixelFormat32bppBGRA;
                    break;
                default:
                    FIXME("unhandled extra sample type %i\n", extra_samples[0]);
                    return E_FAIL;
                }
            break;
        case 16:
            if (samples == 3)
                decode_info->format = &GUID_WICPixelFormat48bppRGB;
            else
                switch(extra_samples[0])
                {
                case 1: /* Associated (pre-multiplied) alpha data */
                    decode_info->format = &GUID_WICPixelFormat64bppPRGBA;
                    break;
                case 0: /* Unspecified data */
                case 2: /* Unassociated alpha data */
                    decode_info->format = &GUID_WICPixelFormat64bppRGBA;
                    break;
                default:
                    FIXME("unhandled extra sample type %i\n", extra_samples[0]);
                    return E_FAIL;
                }
            break;
        default:
            FIXME("unhandled RGB bit count %u\n", bps);
            return E_FAIL;
        }
        break;
    case 3: /* RGB Palette */
        if (samples != 1)
        {
            FIXME("unhandled indexed sample count %u\n", samples);
            return E_FAIL;
        }

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

    if ((ret = pTIFFGetField(tiff, TIFFTAG_TILEWIDTH, &decode_info->tile_width)))
    {
        decode_info->tiled = 1;

        ret = pTIFFGetField(tiff, TIFFTAG_TILELENGTH, &decode_info->tile_height);
        if (!ret)
        {
            WARN("missing tile height\n");
            return E_FAIL;
        }

        decode_info->tile_stride = ((decode_info->bpp * decode_info->tile_width + 7)/8);
        decode_info->tile_size = decode_info->tile_height * decode_info->tile_stride;
        decode_info->tiles_across = (decode_info->width + decode_info->tile_width - 1) / decode_info->tile_width;
    }
    else if ((ret = pTIFFGetField(tiff, TIFFTAG_ROWSPERSTRIP, &decode_info->tile_height)))
    {
        if (decode_info->tile_height > decode_info->height)
            decode_info->tile_height = decode_info->height;
        decode_info->tile_width = decode_info->width;
        decode_info->tile_stride = ((decode_info->bpp * decode_info->tile_width + 7)/8);
        decode_info->tile_size = decode_info->tile_height * decode_info->tile_stride;
    }
    else
    {
        /* Some broken TIFF files have a single strip and lack the RowsPerStrip tag */
        decode_info->tile_height = decode_info->height;
        decode_info->tile_width = decode_info->width;
        decode_info->tile_stride = ((decode_info->bpp * decode_info->tile_width + 7)/8);
        decode_info->tile_size = decode_info->tile_height * decode_info->tile_stride;
    }

    decode_info->resolution_unit = 0;
    pTIFFGetField(tiff, TIFFTAG_RESOLUTIONUNIT, &decode_info->resolution_unit);
    if (decode_info->resolution_unit != 0)
    {
        ret = pTIFFGetField(tiff, TIFFTAG_XRESOLUTION, &decode_info->xres);
        if (!ret)
        {
            WARN("missing X resolution\n");
            decode_info->resolution_unit = 0;
        }

        ret = pTIFFGetField(tiff, TIFFTAG_YRESOLUTION, &decode_info->yres);
        if (!ret)
        {
            WARN("missing Y resolution\n");
            decode_info->resolution_unit = 0;
        }
    }

    return S_OK;
}

static HRESULT WINAPI TiffDecoder_QueryInterface(IWICBitmapDecoder *iface, REFIID iid,
    void **ppv)
{
    TiffDecoder *This = impl_from_IWICBitmapDecoder(iface);
    TRACE("(%p,%s,%p)\n", iface, debugstr_guid(iid), ppv);

    if (!ppv) return E_INVALIDARG;

    if (IsEqualIID(&IID_IUnknown, iid) ||
        IsEqualIID(&IID_IWICBitmapDecoder, iid))
    {
        *ppv = &This->IWICBitmapDecoder_iface;
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
    TiffDecoder *This = impl_from_IWICBitmapDecoder(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) refcount=%u\n", iface, ref);

    return ref;
}

static ULONG WINAPI TiffDecoder_Release(IWICBitmapDecoder *iface)
{
    TiffDecoder *This = impl_from_IWICBitmapDecoder(iface);
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

static HRESULT WINAPI TiffDecoder_QueryCapability(IWICBitmapDecoder *iface, IStream *stream,
    DWORD *capability)
{
    HRESULT hr;

    TRACE("(%p,%p,%p)\n", iface, stream, capability);

    if (!stream || !capability) return E_INVALIDARG;

    hr = IWICBitmapDecoder_Initialize(iface, stream, WICDecodeMetadataCacheOnDemand);
    if (hr != S_OK) return hr;

    *capability = WICBitmapDecoderCapabilityCanDecodeAllImages |
                  WICBitmapDecoderCapabilityCanDecodeSomeImages |
                  WICBitmapDecoderCapabilityCanEnumerateMetadata;
    return S_OK;
}

static HRESULT WINAPI TiffDecoder_Initialize(IWICBitmapDecoder *iface, IStream *pIStream,
    WICDecodeOptions cacheOptions)
{
    TiffDecoder *This = impl_from_IWICBitmapDecoder(iface);
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
    if (!pguidContainerFormat) return E_INVALIDARG;

    memcpy(pguidContainerFormat, &GUID_ContainerFormatTiff, sizeof(GUID));
    return S_OK;
}

static HRESULT WINAPI TiffDecoder_GetDecoderInfo(IWICBitmapDecoder *iface,
    IWICBitmapDecoderInfo **ppIDecoderInfo)
{
    HRESULT hr;
    IWICComponentInfo *compinfo;

    TRACE("(%p,%p)\n", iface, ppIDecoderInfo);

    hr = CreateComponentInfo(&CLSID_WICTiffDecoder, &compinfo);
    if (FAILED(hr)) return hr;

    hr = IWICComponentInfo_QueryInterface(compinfo, &IID_IWICBitmapDecoderInfo,
        (void**)ppIDecoderInfo);

    IWICComponentInfo_Release(compinfo);

    return hr;
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
    TRACE("(%p,%p)\n", iface, ppIBitmapSource);

    if (!ppIBitmapSource) return E_INVALIDARG;

    *ppIBitmapSource = NULL;
    return WINCODEC_ERR_UNSUPPORTEDOPERATION;
}

static HRESULT WINAPI TiffDecoder_GetColorContexts(IWICBitmapDecoder *iface,
    UINT cCount, IWICColorContext **ppIColorContexts, UINT *pcActualCount)
{
    FIXME("(%p,%u,%p,%p)\n", iface, cCount, ppIColorContexts, pcActualCount);
    return WINCODEC_ERR_UNSUPPORTEDOPERATION;
}

static HRESULT WINAPI TiffDecoder_GetThumbnail(IWICBitmapDecoder *iface,
    IWICBitmapSource **ppIThumbnail)
{
    TRACE("(%p,%p)\n", iface, ppIThumbnail);

    if (!ppIThumbnail) return E_INVALIDARG;

    *ppIThumbnail = NULL;
    return WINCODEC_ERR_CODECNOTHUMBNAIL;
}

static HRESULT WINAPI TiffDecoder_GetFrameCount(IWICBitmapDecoder *iface,
    UINT *pCount)
{
    TiffDecoder *This = impl_from_IWICBitmapDecoder(iface);

    if (!pCount) return E_INVALIDARG;

    EnterCriticalSection(&This->lock);
    *pCount = This->tiff ? pTIFFNumberOfDirectories(This->tiff) : 0;
    LeaveCriticalSection(&This->lock);

    TRACE("(%p) <-- %i\n", iface, *pCount);

    return S_OK;
}

static HRESULT WINAPI TiffDecoder_GetFrame(IWICBitmapDecoder *iface,
    UINT index, IWICBitmapFrameDecode **ppIBitmapFrame)
{
    TiffDecoder *This = impl_from_IWICBitmapDecoder(iface);
    TiffFrameDecode *result;
    int res;
    tiff_decode_info decode_info;
    HRESULT hr;

    TRACE("(%p,%u,%p)\n", iface, index, ppIBitmapFrame);

    if (!This->tiff)
        return WINCODEC_ERR_FRAMEMISSING;

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
            result->IWICBitmapFrameDecode_iface.lpVtbl = &TiffFrameDecode_Vtbl;
            result->IWICMetadataBlockReader_iface.lpVtbl = &TiffFrameDecode_BlockVtbl;
            result->ref = 1;
            result->parent = This;
            result->index = index;
            result->decode_info = decode_info;
            result->cached_tile_x = -1;
            result->cached_tile = HeapAlloc(GetProcessHeap(), 0, decode_info.tile_size);

            if (result->cached_tile)
                *ppIBitmapFrame = &result->IWICBitmapFrameDecode_iface;
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
    TiffFrameDecode *This = impl_from_IWICBitmapFrameDecode(iface);
    TRACE("(%p,%s,%p)\n", iface, debugstr_guid(iid), ppv);

    if (!ppv) return E_INVALIDARG;

    if (IsEqualIID(&IID_IUnknown, iid) ||
        IsEqualIID(&IID_IWICBitmapSource, iid) ||
        IsEqualIID(&IID_IWICBitmapFrameDecode, iid))
    {
        *ppv = &This->IWICBitmapFrameDecode_iface;
    }
    else if (IsEqualIID(&IID_IWICMetadataBlockReader, iid))
    {
        *ppv = &This->IWICMetadataBlockReader_iface;
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
    TiffFrameDecode *This = impl_from_IWICBitmapFrameDecode(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) refcount=%u\n", iface, ref);

    return ref;
}

static ULONG WINAPI TiffFrameDecode_Release(IWICBitmapFrameDecode *iface)
{
    TiffFrameDecode *This = impl_from_IWICBitmapFrameDecode(iface);
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
    TiffFrameDecode *This = impl_from_IWICBitmapFrameDecode(iface);

    *puiWidth = This->decode_info.width;
    *puiHeight = This->decode_info.height;

    TRACE("(%p) <-- %ux%u\n", iface, *puiWidth, *puiHeight);

    return S_OK;
}

static HRESULT WINAPI TiffFrameDecode_GetPixelFormat(IWICBitmapFrameDecode *iface,
    WICPixelFormatGUID *pPixelFormat)
{
    TiffFrameDecode *This = impl_from_IWICBitmapFrameDecode(iface);

    memcpy(pPixelFormat, This->decode_info.format, sizeof(GUID));

    TRACE("(%p) <-- %s\n", This, debugstr_guid(This->decode_info.format));

    return S_OK;
}

static HRESULT WINAPI TiffFrameDecode_GetResolution(IWICBitmapFrameDecode *iface,
    double *pDpiX, double *pDpiY)
{
    TiffFrameDecode *This = impl_from_IWICBitmapFrameDecode(iface);

    switch (This->decode_info.resolution_unit)
    {
    default:
        FIXME("unknown resolution unit %i\n", This->decode_info.resolution_unit);
        /* fall through */
    case 0: /* Not set */
        *pDpiX = *pDpiY = 96.0;
        break;
    case 1: /* Relative measurements */
        *pDpiX = 96.0;
        *pDpiY = 96.0 * This->decode_info.yres / This->decode_info.xres;
        break;
    case 2: /* Inch */
        *pDpiX = This->decode_info.xres;
        *pDpiY = This->decode_info.yres;
        break;
    case 3: /* Centimeter */
        *pDpiX = This->decode_info.xres / 2.54;
        *pDpiY = This->decode_info.yres / 2.54;
        break;
    }

    TRACE("(%p) <-- %f,%f unit=%i\n", iface, *pDpiX, *pDpiY, This->decode_info.resolution_unit);

    return S_OK;
}

static HRESULT WINAPI TiffFrameDecode_CopyPalette(IWICBitmapFrameDecode *iface,
    IWICPalette *pIPalette)
{
    TiffFrameDecode *This = impl_from_IWICBitmapFrameDecode(iface);
    uint16 *red, *green, *blue;
    WICColor colors[256];
    int color_count, ret, i;

    TRACE("(%p,%p)\n", iface, pIPalette);

    color_count = 1<<This->decode_info.bps;

    EnterCriticalSection(&This->parent->lock);
    ret = pTIFFGetField(This->parent->tiff, TIFFTAG_COLORMAP, &red, &green, &blue);
    LeaveCriticalSection(&This->parent->lock);

    if (!ret)
    {
        WARN("Couldn't read color map\n");
        return WINCODEC_ERR_PALETTEUNAVAILABLE;
    }

    for (i=0; i<color_count; i++)
    {
        colors[i] = 0xff000000 |
            ((red[i]<<8) & 0xff0000) |
            (green[i] & 0xff00) |
            ((blue[i]>>8) & 0xff);
    }

    return IWICPalette_InitializeCustom(pIPalette, colors, color_count);
}

static HRESULT TiffFrameDecode_ReadTile(TiffFrameDecode *This, UINT tile_x, UINT tile_y)
{
    HRESULT hr=S_OK;
    tsize_t ret;
    int swap_bytes;

    swap_bytes = pTIFFIsByteSwapped(This->parent->tiff);

    ret = pTIFFSetDirectory(This->parent->tiff, This->index);

    if (ret == -1)
        hr = E_FAIL;

    if (hr == S_OK)
    {
        if (This->decode_info.tiled)
        {
            ret = pTIFFReadEncodedTile(This->parent->tiff, tile_x + tile_y * This->decode_info.tiles_across, This->cached_tile, This->decode_info.tile_size);
        }
        else
        {
            ret = pTIFFReadEncodedStrip(This->parent->tiff, tile_y, This->cached_tile, This->decode_info.tile_size);
        }

        if (ret == -1)
            hr = E_FAIL;
    }

    /* 8bpp grayscale with extra alpha */
    if (hr == S_OK && This->decode_info.source_bpp == 16 && This->decode_info.samples == 2 && This->decode_info.bpp == 32)
    {
        BYTE *src;
        DWORD *dst, count = This->decode_info.tile_width * This->decode_info.tile_height;

        src = This->cached_tile + This->decode_info.tile_width * This->decode_info.tile_height * 2 - 2;
        dst = (DWORD *)(This->cached_tile + This->decode_info.tile_size - 4);

        while (count--)
        {
            *dst-- = src[0] | (src[0] << 8) | (src[0] << 16) | (src[1] << 24);
            src -= 2;
        }
    }

    if (hr == S_OK && This->decode_info.reverse_bgr)
    {
        if (This->decode_info.bps == 8)
        {
            UINT sample_count = This->decode_info.samples;

            reverse_bgr8(sample_count, This->cached_tile, This->decode_info.tile_width,
                This->decode_info.tile_height, This->decode_info.tile_width * sample_count);
        }
    }

    if (hr == S_OK && swap_bytes && This->decode_info.bps > 8)
    {
        UINT row, i, samples_per_row;
        BYTE *sample, temp;

        samples_per_row = This->decode_info.tile_width * This->decode_info.samples;

        switch(This->decode_info.bps)
        {
        case 16:
            for (row=0; row<This->decode_info.tile_height; row++)
            {
                sample = This->cached_tile + row * This->decode_info.tile_stride;
                for (i=0; i<samples_per_row; i++)
                {
                    temp = sample[1];
                    sample[1] = sample[0];
                    sample[0] = temp;
                    sample += 2;
                }
            }
            break;
        default:
            ERR("unhandled bps for byte swap %u\n", This->decode_info.bps);
            return E_FAIL;
        }
    }

    if (hr == S_OK && This->decode_info.invert_grayscale)
    {
        BYTE *byte, *end;

        if (This->decode_info.samples != 1)
        {
            ERR("cannot invert grayscale image with %u samples\n", This->decode_info.samples);
            return E_FAIL;
        }

        end = This->cached_tile+This->decode_info.tile_size;

        for (byte = This->cached_tile; byte != end; byte++)
            *byte = ~(*byte);
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
    TiffFrameDecode *This = impl_from_IWICBitmapFrameDecode(iface);
    UINT min_tile_x, max_tile_x, min_tile_y, max_tile_y;
    UINT tile_x, tile_y;
    WICRect rc;
    HRESULT hr=S_OK;
    BYTE *dst_tilepos;
    UINT bytesperrow;
    WICRect rect;

    TRACE("(%p,%p,%u,%u,%p)\n", iface, prc, cbStride, cbBufferSize, pbBuffer);

    if (!prc)
    {
        rect.X = 0;
        rect.Y = 0;
        rect.Width = This->decode_info.width;
        rect.Height = This->decode_info.height;
        prc = &rect;
    }
    else
    {
        if (prc->X < 0 || prc->Y < 0 || prc->X+prc->Width > This->decode_info.width ||
            prc->Y+prc->Height > This->decode_info.height)
            return E_INVALIDARG;
    }

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
                else if (prc->X < tile_x * This->decode_info.tile_width)
                    rc.Width = prc->Width + prc->X - tile_x * This->decode_info.tile_width;
                else
                    rc.Width = prc->Width;

                if (prc->Y+prc->Height > (tile_y+1) * This->decode_info.tile_height)
                    rc.Height = This->decode_info.tile_height - rc.Y;
                else if (prc->Y < tile_y * This->decode_info.tile_height)
                    rc.Height = prc->Height + prc->Y - tile_y * This->decode_info.tile_height;
                else
                    rc.Height = prc->Height;

                dst_tilepos = pbBuffer + (cbStride * ((rc.Y + tile_y * This->decode_info.tile_height) - prc->Y)) +
                    ((This->decode_info.bpp * ((rc.X + tile_x * This->decode_info.tile_width) - prc->X) + 7) / 8);

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
    TiffFrameDecode *This = impl_from_IWICBitmapFrameDecode(iface);
    const BYTE *profile;
    UINT len;
    HRESULT hr;

    TRACE("(%p,%u,%p,%p)\n", iface, cCount, ppIColorContexts, pcActualCount);

    EnterCriticalSection(&This->parent->lock);

    if (pTIFFGetField(This->parent->tiff, TIFFTAG_ICCPROFILE, &len, &profile))
    {
        if (cCount && ppIColorContexts)
        {
            hr = IWICColorContext_InitializeFromMemory(*ppIColorContexts, profile, len);
            if (FAILED(hr))
            {
                LeaveCriticalSection(&This->parent->lock);
                return hr;
            }
        }
        *pcActualCount = 1;
    }
    else
        *pcActualCount = 0;

    LeaveCriticalSection(&This->parent->lock);

    return S_OK;
}

static HRESULT WINAPI TiffFrameDecode_GetThumbnail(IWICBitmapFrameDecode *iface,
    IWICBitmapSource **ppIThumbnail)
{
    TRACE("(%p,%p)\n", iface, ppIThumbnail);

    if (!ppIThumbnail) return E_INVALIDARG;

    *ppIThumbnail = NULL;
    return WINCODEC_ERR_CODECNOTHUMBNAIL;
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

static HRESULT WINAPI TiffFrameDecode_Block_QueryInterface(IWICMetadataBlockReader *iface,
    REFIID iid, void **ppv)
{
    TiffFrameDecode *This = impl_from_IWICMetadataBlockReader(iface);
    return IWICBitmapFrameDecode_QueryInterface(&This->IWICBitmapFrameDecode_iface, iid, ppv);
}

static ULONG WINAPI TiffFrameDecode_Block_AddRef(IWICMetadataBlockReader *iface)
{
    TiffFrameDecode *This = impl_from_IWICMetadataBlockReader(iface);
    return IWICBitmapFrameDecode_AddRef(&This->IWICBitmapFrameDecode_iface);
}

static ULONG WINAPI TiffFrameDecode_Block_Release(IWICMetadataBlockReader *iface)
{
    TiffFrameDecode *This = impl_from_IWICMetadataBlockReader(iface);
    return IWICBitmapFrameDecode_Release(&This->IWICBitmapFrameDecode_iface);
}

static HRESULT WINAPI TiffFrameDecode_Block_GetContainerFormat(IWICMetadataBlockReader *iface,
    GUID *guid)
{
    TRACE("(%p,%p)\n", iface, guid);

    if (!guid) return E_INVALIDARG;

    *guid = GUID_ContainerFormatTiff;
    return S_OK;
}

static HRESULT WINAPI TiffFrameDecode_Block_GetCount(IWICMetadataBlockReader *iface,
    UINT *count)
{
    TRACE("%p,%p\n", iface, count);

    if (!count) return E_INVALIDARG;

    *count = 1;
    return S_OK;
}

static HRESULT create_metadata_reader(TiffFrameDecode *This, IWICMetadataReader **reader)
{
    HRESULT hr;
    LARGE_INTEGER dir_offset;
    IWICMetadataReader *metadata_reader;
    IWICPersistStream *persist;

    /* FIXME: Use IWICComponentFactory_CreateMetadataReader once it's implemented */

    hr = IfdMetadataReader_CreateInstance(&IID_IWICMetadataReader, (void **)&metadata_reader);
    if (FAILED(hr)) return hr;

    hr = IWICMetadataReader_QueryInterface(metadata_reader, &IID_IWICPersistStream, (void **)&persist);
    if (FAILED(hr))
    {
        IWICMetadataReader_Release(metadata_reader);
        return hr;
    }

    EnterCriticalSection(&This->parent->lock);

    dir_offset.QuadPart = pTIFFCurrentDirOffset(This->parent->tiff);
    hr = IStream_Seek(This->parent->stream, dir_offset, STREAM_SEEK_SET, NULL);
    if (SUCCEEDED(hr))
    {
        BOOL byte_swapped = pTIFFIsByteSwapped(This->parent->tiff);
#ifdef WORDS_BIGENDIAN
        DWORD persist_options = byte_swapped ? WICPersistOptionsLittleEndian : WICPersistOptionsBigEndian;
#else
        DWORD persist_options = byte_swapped ? WICPersistOptionsBigEndian : WICPersistOptionsLittleEndian;
#endif
        persist_options |= WICPersistOptionsNoCacheStream;
        hr = IWICPersistStream_LoadEx(persist, This->parent->stream, NULL, persist_options);
        if (FAILED(hr))
            ERR("IWICPersistStream_LoadEx error %#x\n", hr);
    }

    LeaveCriticalSection(&This->parent->lock);

    IWICPersistStream_Release(persist);

    if (FAILED(hr))
    {
        IWICMetadataReader_Release(metadata_reader);
        return hr;
    }

    *reader = metadata_reader;
    return S_OK;
}

static HRESULT WINAPI TiffFrameDecode_Block_GetReaderByIndex(IWICMetadataBlockReader *iface,
    UINT index, IWICMetadataReader **reader)
{
    TiffFrameDecode *This = impl_from_IWICMetadataBlockReader(iface);

    TRACE("(%p,%u,%p)\n", iface, index, reader);

    if (!reader || index != 0) return E_INVALIDARG;

    return create_metadata_reader(This, reader);
}

static HRESULT WINAPI TiffFrameDecode_Block_GetEnumerator(IWICMetadataBlockReader *iface,
    IEnumUnknown **enum_metadata)
{
    FIXME("(%p,%p): stub\n", iface, enum_metadata);
    return E_NOTIMPL;
}

static const IWICMetadataBlockReaderVtbl TiffFrameDecode_BlockVtbl =
{
    TiffFrameDecode_Block_QueryInterface,
    TiffFrameDecode_Block_AddRef,
    TiffFrameDecode_Block_Release,
    TiffFrameDecode_Block_GetContainerFormat,
    TiffFrameDecode_Block_GetCount,
    TiffFrameDecode_Block_GetReaderByIndex,
    TiffFrameDecode_Block_GetEnumerator
};

HRESULT TiffDecoder_CreateInstance(REFIID iid, void** ppv)
{
    HRESULT ret;
    TiffDecoder *This;

    TRACE("(%s,%p)\n", debugstr_guid(iid), ppv);

    *ppv = NULL;

    if (!load_libtiff())
    {
        ERR("Failed reading TIFF because unable to load %s\n",SONAME_LIBTIFF);
        return E_FAIL;
    }

    This = HeapAlloc(GetProcessHeap(), 0, sizeof(TiffDecoder));
    if (!This) return E_OUTOFMEMORY;

    This->IWICBitmapDecoder_iface.lpVtbl = &TiffDecoder_Vtbl;
    This->ref = 1;
    This->stream = NULL;
    InitializeCriticalSection(&This->lock);
    This->lock.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": TiffDecoder.lock");
    This->tiff = NULL;
    This->initialized = FALSE;

    ret = IWICBitmapDecoder_QueryInterface(&This->IWICBitmapDecoder_iface, iid, ppv);
    IWICBitmapDecoder_Release(&This->IWICBitmapDecoder_iface);

    return ret;
}

struct tiff_encode_format {
    const WICPixelFormatGUID *guid;
    int photometric;
    int bps;
    int samples;
    int bpp;
    int extra_sample;
    int extra_sample_type;
    int reverse_bgr;
};

static const struct tiff_encode_format formats[] = {
    {&GUID_WICPixelFormat24bppBGR, 2, 8, 3, 24, 0, 0, 1},
    {&GUID_WICPixelFormat24bppRGB, 2, 8, 3, 24, 0, 0, 0},
    {&GUID_WICPixelFormatBlackWhite, 1, 1, 1, 1, 0, 0, 0},
    {&GUID_WICPixelFormat4bppGray, 1, 4, 1, 4, 0, 0, 0},
    {&GUID_WICPixelFormat8bppGray, 1, 8, 1, 8, 0, 0, 0},
    {&GUID_WICPixelFormat32bppBGRA, 2, 8, 4, 32, 1, 2, 1},
    {&GUID_WICPixelFormat32bppPBGRA, 2, 8, 4, 32, 1, 1, 1},
    {&GUID_WICPixelFormat48bppRGB, 2, 16, 3, 48, 0, 0, 0},
    {&GUID_WICPixelFormat64bppRGBA, 2, 16, 4, 64, 1, 2, 0},
    {&GUID_WICPixelFormat64bppPRGBA, 2, 16, 4, 64, 1, 1, 0},
    {0}
};

typedef struct TiffEncoder {
    IWICBitmapEncoder IWICBitmapEncoder_iface;
    LONG ref;
    IStream *stream;
    CRITICAL_SECTION lock; /* Must be held when tiff is used or fields below are set */
    TIFF *tiff;
    BOOL initialized;
    BOOL committed;
    ULONG num_frames;
    ULONG num_frames_committed;
} TiffEncoder;

static inline TiffEncoder *impl_from_IWICBitmapEncoder(IWICBitmapEncoder *iface)
{
    return CONTAINING_RECORD(iface, TiffEncoder, IWICBitmapEncoder_iface);
}

typedef struct TiffFrameEncode {
    IWICBitmapFrameEncode IWICBitmapFrameEncode_iface;
    LONG ref;
    TiffEncoder *parent;
    /* fields below are protected by parent->lock */
    BOOL initialized;
    BOOL info_written;
    BOOL committed;
    const struct tiff_encode_format *format;
    UINT width, height;
    double xres, yres;
    UINT lines_written;
} TiffFrameEncode;

static inline TiffFrameEncode *impl_from_IWICBitmapFrameEncode(IWICBitmapFrameEncode *iface)
{
    return CONTAINING_RECORD(iface, TiffFrameEncode, IWICBitmapFrameEncode_iface);
}

static HRESULT WINAPI TiffFrameEncode_QueryInterface(IWICBitmapFrameEncode *iface, REFIID iid,
    void **ppv)
{
    TiffFrameEncode *This = impl_from_IWICBitmapFrameEncode(iface);
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

static ULONG WINAPI TiffFrameEncode_AddRef(IWICBitmapFrameEncode *iface)
{
    TiffFrameEncode *This = impl_from_IWICBitmapFrameEncode(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) refcount=%u\n", iface, ref);

    return ref;
}

static ULONG WINAPI TiffFrameEncode_Release(IWICBitmapFrameEncode *iface)
{
    TiffFrameEncode *This = impl_from_IWICBitmapFrameEncode(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) refcount=%u\n", iface, ref);

    if (ref == 0)
    {
        IWICBitmapEncoder_Release(&This->parent->IWICBitmapEncoder_iface);
        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}

static HRESULT WINAPI TiffFrameEncode_Initialize(IWICBitmapFrameEncode *iface,
    IPropertyBag2 *pIEncoderOptions)
{
    TiffFrameEncode *This = impl_from_IWICBitmapFrameEncode(iface);
    TRACE("(%p,%p)\n", iface, pIEncoderOptions);

    EnterCriticalSection(&This->parent->lock);

    if (This->initialized)
    {
        LeaveCriticalSection(&This->parent->lock);
        return WINCODEC_ERR_WRONGSTATE;
    }

    This->initialized = TRUE;

    LeaveCriticalSection(&This->parent->lock);

    return S_OK;
}

static HRESULT WINAPI TiffFrameEncode_SetSize(IWICBitmapFrameEncode *iface,
    UINT uiWidth, UINT uiHeight)
{
    TiffFrameEncode *This = impl_from_IWICBitmapFrameEncode(iface);
    TRACE("(%p,%u,%u)\n", iface, uiWidth, uiHeight);

    EnterCriticalSection(&This->parent->lock);

    if (!This->initialized || This->info_written)
    {
        LeaveCriticalSection(&This->parent->lock);
        return WINCODEC_ERR_WRONGSTATE;
    }

    This->width = uiWidth;
    This->height = uiHeight;

    LeaveCriticalSection(&This->parent->lock);

    return S_OK;
}

static HRESULT WINAPI TiffFrameEncode_SetResolution(IWICBitmapFrameEncode *iface,
    double dpiX, double dpiY)
{
    TiffFrameEncode *This = impl_from_IWICBitmapFrameEncode(iface);
    TRACE("(%p,%0.2f,%0.2f)\n", iface, dpiX, dpiY);

    EnterCriticalSection(&This->parent->lock);

    if (!This->initialized || This->info_written)
    {
        LeaveCriticalSection(&This->parent->lock);
        return WINCODEC_ERR_WRONGSTATE;
    }

    This->xres = dpiX;
    This->yres = dpiY;

    LeaveCriticalSection(&This->parent->lock);

    return S_OK;
}

static HRESULT WINAPI TiffFrameEncode_SetPixelFormat(IWICBitmapFrameEncode *iface,
    WICPixelFormatGUID *pPixelFormat)
{
    TiffFrameEncode *This = impl_from_IWICBitmapFrameEncode(iface);
    int i;

    TRACE("(%p,%s)\n", iface, debugstr_guid(pPixelFormat));

    EnterCriticalSection(&This->parent->lock);

    if (!This->initialized || This->info_written)
    {
        LeaveCriticalSection(&This->parent->lock);
        return WINCODEC_ERR_WRONGSTATE;
    }

    for (i=0; formats[i].guid; i++)
    {
        if (memcmp(formats[i].guid, pPixelFormat, sizeof(GUID)) == 0)
            break;
    }

    if (!formats[i].guid) i = 0;

    This->format = &formats[i];
    memcpy(pPixelFormat, This->format->guid, sizeof(GUID));

    LeaveCriticalSection(&This->parent->lock);

    return S_OK;
}

static HRESULT WINAPI TiffFrameEncode_SetColorContexts(IWICBitmapFrameEncode *iface,
    UINT cCount, IWICColorContext **ppIColorContext)
{
    FIXME("(%p,%u,%p): stub\n", iface, cCount, ppIColorContext);
    return E_NOTIMPL;
}

static HRESULT WINAPI TiffFrameEncode_SetPalette(IWICBitmapFrameEncode *iface,
    IWICPalette *pIPalette)
{
    FIXME("(%p,%p): stub\n", iface, pIPalette);
    return WINCODEC_ERR_UNSUPPORTEDOPERATION;
}

static HRESULT WINAPI TiffFrameEncode_SetThumbnail(IWICBitmapFrameEncode *iface,
    IWICBitmapSource *pIThumbnail)
{
    FIXME("(%p,%p): stub\n", iface, pIThumbnail);
    return WINCODEC_ERR_UNSUPPORTEDOPERATION;
}

static HRESULT WINAPI TiffFrameEncode_WritePixels(IWICBitmapFrameEncode *iface,
    UINT lineCount, UINT cbStride, UINT cbBufferSize, BYTE *pbPixels)
{
    TiffFrameEncode *This = impl_from_IWICBitmapFrameEncode(iface);
    BYTE *row_data, *swapped_data = NULL;
    UINT i, j, line_size;

    TRACE("(%p,%u,%u,%u,%p)\n", iface, lineCount, cbStride, cbBufferSize, pbPixels);

    EnterCriticalSection(&This->parent->lock);

    if (!This->initialized || !This->width || !This->height || !This->format)
    {
        LeaveCriticalSection(&This->parent->lock);
        return WINCODEC_ERR_WRONGSTATE;
    }

    if (lineCount == 0 || lineCount + This->lines_written > This->height)
    {
        LeaveCriticalSection(&This->parent->lock);
        return E_INVALIDARG;
    }

    line_size = ((This->width * This->format->bpp)+7)/8;

    if (This->format->reverse_bgr)
    {
        swapped_data = HeapAlloc(GetProcessHeap(), 0, line_size);
        if (!swapped_data)
        {
            LeaveCriticalSection(&This->parent->lock);
            return E_OUTOFMEMORY;
        }
    }

    if (!This->info_written)
    {
        pTIFFSetField(This->parent->tiff, TIFFTAG_PHOTOMETRIC, (uint16)This->format->photometric);
        pTIFFSetField(This->parent->tiff, TIFFTAG_PLANARCONFIG, (uint16)1);
        pTIFFSetField(This->parent->tiff, TIFFTAG_BITSPERSAMPLE, (uint16)This->format->bps);
        pTIFFSetField(This->parent->tiff, TIFFTAG_SAMPLESPERPIXEL, (uint16)This->format->samples);

        if (This->format->extra_sample)
        {
            uint16 extra_samples;
            extra_samples = This->format->extra_sample_type;

            pTIFFSetField(This->parent->tiff, TIFFTAG_EXTRASAMPLES, (uint16)1, &extra_samples);
        }

        pTIFFSetField(This->parent->tiff, TIFFTAG_IMAGEWIDTH, (uint32)This->width);
        pTIFFSetField(This->parent->tiff, TIFFTAG_IMAGELENGTH, (uint32)This->height);

        if (This->xres != 0.0 && This->yres != 0.0)
        {
            pTIFFSetField(This->parent->tiff, TIFFTAG_RESOLUTIONUNIT, (uint16)2); /* Inch */
            pTIFFSetField(This->parent->tiff, TIFFTAG_XRESOLUTION, (float)This->xres);
            pTIFFSetField(This->parent->tiff, TIFFTAG_YRESOLUTION, (float)This->yres);
        }

        This->info_written = TRUE;
    }

    for (i=0; i<lineCount; i++)
    {
        row_data = pbPixels + i * cbStride;

        if (This->format->reverse_bgr && This->format->bps == 8)
        {
            memcpy(swapped_data, row_data, line_size);
            for (j=0; j<line_size; j += This->format->samples)
            {
                BYTE temp;
                temp = swapped_data[j];
                swapped_data[j] = swapped_data[j+2];
                swapped_data[j+2] = temp;
            }
            row_data = swapped_data;
        }

        pTIFFWriteScanline(This->parent->tiff, (tdata_t)row_data, i+This->lines_written, 0);
    }

    This->lines_written += lineCount;

    LeaveCriticalSection(&This->parent->lock);

    HeapFree(GetProcessHeap(), 0, swapped_data);

    return S_OK;
}

static HRESULT WINAPI TiffFrameEncode_WriteSource(IWICBitmapFrameEncode *iface,
    IWICBitmapSource *pIBitmapSource, WICRect *prc)
{
    TiffFrameEncode *This = impl_from_IWICBitmapFrameEncode(iface);
    HRESULT hr;

    TRACE("(%p,%p,%p)\n", iface, pIBitmapSource, prc);

    if (!This->initialized)
        return WINCODEC_ERR_WRONGSTATE;

    hr = configure_write_source(iface, pIBitmapSource, prc,
        This->format ? This->format->guid : NULL, This->width, This->height,
        This->xres, This->yres);

    if (SUCCEEDED(hr))
    {
        hr = write_source(iface, pIBitmapSource, prc,
            This->format->guid, This->format->bpp, This->width, This->height);
    }

    return hr;
}

static HRESULT WINAPI TiffFrameEncode_Commit(IWICBitmapFrameEncode *iface)
{
    TiffFrameEncode *This = impl_from_IWICBitmapFrameEncode(iface);

    TRACE("(%p)\n", iface);

    EnterCriticalSection(&This->parent->lock);

    if (!This->info_written || This->lines_written != This->height || This->committed)
    {
        LeaveCriticalSection(&This->parent->lock);
        return WINCODEC_ERR_WRONGSTATE;
    }

    /* libtiff will commit the data when creating a new frame or closing the file */

    This->committed = TRUE;
    This->parent->num_frames_committed++;

    LeaveCriticalSection(&This->parent->lock);

    return S_OK;
}

static HRESULT WINAPI TiffFrameEncode_GetMetadataQueryWriter(IWICBitmapFrameEncode *iface,
    IWICMetadataQueryWriter **ppIMetadataQueryWriter)
{
    FIXME("(%p, %p): stub\n", iface, ppIMetadataQueryWriter);
    return E_NOTIMPL;
}

static const IWICBitmapFrameEncodeVtbl TiffFrameEncode_Vtbl = {
    TiffFrameEncode_QueryInterface,
    TiffFrameEncode_AddRef,
    TiffFrameEncode_Release,
    TiffFrameEncode_Initialize,
    TiffFrameEncode_SetSize,
    TiffFrameEncode_SetResolution,
    TiffFrameEncode_SetPixelFormat,
    TiffFrameEncode_SetColorContexts,
    TiffFrameEncode_SetPalette,
    TiffFrameEncode_SetThumbnail,
    TiffFrameEncode_WritePixels,
    TiffFrameEncode_WriteSource,
    TiffFrameEncode_Commit,
    TiffFrameEncode_GetMetadataQueryWriter
};

static HRESULT WINAPI TiffEncoder_QueryInterface(IWICBitmapEncoder *iface, REFIID iid,
    void **ppv)
{
    TiffEncoder *This = impl_from_IWICBitmapEncoder(iface);
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

static ULONG WINAPI TiffEncoder_AddRef(IWICBitmapEncoder *iface)
{
    TiffEncoder *This = impl_from_IWICBitmapEncoder(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) refcount=%u\n", iface, ref);

    return ref;
}

static ULONG WINAPI TiffEncoder_Release(IWICBitmapEncoder *iface)
{
    TiffEncoder *This = impl_from_IWICBitmapEncoder(iface);
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

static HRESULT WINAPI TiffEncoder_Initialize(IWICBitmapEncoder *iface,
    IStream *pIStream, WICBitmapEncoderCacheOption cacheOption)
{
    TiffEncoder *This = impl_from_IWICBitmapEncoder(iface);
    TIFF *tiff;
    HRESULT hr=S_OK;

    TRACE("(%p,%p,%u)\n", iface, pIStream, cacheOption);

    EnterCriticalSection(&This->lock);

    if (This->initialized || This->committed)
    {
        hr = WINCODEC_ERR_WRONGSTATE;
        goto exit;
    }

    tiff = tiff_open_stream(pIStream, "w");

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

static HRESULT WINAPI TiffEncoder_GetContainerFormat(IWICBitmapEncoder *iface,
    GUID *pguidContainerFormat)
{
    memcpy(pguidContainerFormat, &GUID_ContainerFormatTiff, sizeof(GUID));
    return S_OK;
}

static HRESULT WINAPI TiffEncoder_GetEncoderInfo(IWICBitmapEncoder *iface,
    IWICBitmapEncoderInfo **ppIEncoderInfo)
{
    FIXME("(%p,%p): stub\n", iface, ppIEncoderInfo);
    return E_NOTIMPL;
}

static HRESULT WINAPI TiffEncoder_SetColorContexts(IWICBitmapEncoder *iface,
    UINT cCount, IWICColorContext **ppIColorContext)
{
    FIXME("(%p,%u,%p): stub\n", iface, cCount, ppIColorContext);
    return E_NOTIMPL;
}

static HRESULT WINAPI TiffEncoder_SetPalette(IWICBitmapEncoder *iface, IWICPalette *pIPalette)
{
    TRACE("(%p,%p)\n", iface, pIPalette);
    return WINCODEC_ERR_UNSUPPORTEDOPERATION;
}

static HRESULT WINAPI TiffEncoder_SetThumbnail(IWICBitmapEncoder *iface, IWICBitmapSource *pIThumbnail)
{
    TRACE("(%p,%p)\n", iface, pIThumbnail);
    return WINCODEC_ERR_UNSUPPORTEDOPERATION;
}

static HRESULT WINAPI TiffEncoder_SetPreview(IWICBitmapEncoder *iface, IWICBitmapSource *pIPreview)
{
    TRACE("(%p,%p)\n", iface, pIPreview);
    return WINCODEC_ERR_UNSUPPORTEDOPERATION;
}

static HRESULT WINAPI TiffEncoder_CreateNewFrame(IWICBitmapEncoder *iface,
    IWICBitmapFrameEncode **ppIFrameEncode, IPropertyBag2 **ppIEncoderOptions)
{
    TiffEncoder *This = impl_from_IWICBitmapEncoder(iface);
    TiffFrameEncode *result;

    HRESULT hr=S_OK;

    TRACE("(%p,%p,%p)\n", iface, ppIFrameEncode, ppIEncoderOptions);

    EnterCriticalSection(&This->lock);

    if (!This->initialized || This->committed)
    {
        hr = WINCODEC_ERR_WRONGSTATE;
    }
    else if (This->num_frames != This->num_frames_committed)
    {
        FIXME("New frame created before previous frame was committed\n");
        hr = E_FAIL;
    }

    if (SUCCEEDED(hr))
    {
        PROPBAG2 opts[2]= {{0}};
        opts[0].pstrName = (LPOLESTR)wszTiffCompressionMethod;
        opts[0].vt = VT_UI1;
        opts[0].dwType = PROPBAG2_TYPE_DATA;

        opts[1].pstrName = (LPOLESTR)wszCompressionQuality;
        opts[1].vt = VT_R4;
        opts[1].dwType = PROPBAG2_TYPE_DATA;

        hr = CreatePropertyBag2(opts, 2, ppIEncoderOptions);

        if (SUCCEEDED(hr))
        {
            VARIANT v;
            VariantInit(&v);
            V_VT(&v) = VT_UI1;
            V_UNION(&v, bVal) = WICTiffCompressionDontCare;
            hr = IPropertyBag2_Write(*ppIEncoderOptions, 1, opts, &v);
            VariantClear(&v);
            if (FAILED(hr))
            {
                IPropertyBag2_Release(*ppIEncoderOptions);
                *ppIEncoderOptions = NULL;
            }
        }
    }

    if (SUCCEEDED(hr))
    {
        result = HeapAlloc(GetProcessHeap(), 0, sizeof(*result));

        if (result)
        {
            result->IWICBitmapFrameEncode_iface.lpVtbl = &TiffFrameEncode_Vtbl;
            result->ref = 1;
            result->parent = This;
            result->initialized = FALSE;
            result->info_written = FALSE;
            result->committed = FALSE;
            result->format = NULL;
            result->width = 0;
            result->height = 0;
            result->xres = 0.0;
            result->yres = 0.0;
            result->lines_written = 0;

            IWICBitmapEncoder_AddRef(iface);
            *ppIFrameEncode = &result->IWICBitmapFrameEncode_iface;

            if (This->num_frames != 0)
                pTIFFWriteDirectory(This->tiff);

            This->num_frames++;
        }
        else
            hr = E_OUTOFMEMORY;

        if (FAILED(hr))
        {
            IPropertyBag2_Release(*ppIEncoderOptions);
            *ppIEncoderOptions = NULL;
        }
    }

    LeaveCriticalSection(&This->lock);

    return hr;
}

static HRESULT WINAPI TiffEncoder_Commit(IWICBitmapEncoder *iface)
{
    TiffEncoder *This = impl_from_IWICBitmapEncoder(iface);

    TRACE("(%p)\n", iface);

    EnterCriticalSection(&This->lock);

    if (!This->initialized || This->committed)
    {
        LeaveCriticalSection(&This->lock);
        return WINCODEC_ERR_WRONGSTATE;
    }

    pTIFFClose(This->tiff);
    IStream_Release(This->stream);
    This->stream = NULL;
    This->tiff = NULL;

    This->committed = TRUE;

    LeaveCriticalSection(&This->lock);

    return S_OK;
}

static HRESULT WINAPI TiffEncoder_GetMetadataQueryWriter(IWICBitmapEncoder *iface,
    IWICMetadataQueryWriter **ppIMetadataQueryWriter)
{
    FIXME("(%p,%p): stub\n", iface, ppIMetadataQueryWriter);
    return E_NOTIMPL;
}

static const IWICBitmapEncoderVtbl TiffEncoder_Vtbl = {
    TiffEncoder_QueryInterface,
    TiffEncoder_AddRef,
    TiffEncoder_Release,
    TiffEncoder_Initialize,
    TiffEncoder_GetContainerFormat,
    TiffEncoder_GetEncoderInfo,
    TiffEncoder_SetColorContexts,
    TiffEncoder_SetPalette,
    TiffEncoder_SetThumbnail,
    TiffEncoder_SetPreview,
    TiffEncoder_CreateNewFrame,
    TiffEncoder_Commit,
    TiffEncoder_GetMetadataQueryWriter
};

HRESULT TiffEncoder_CreateInstance(REFIID iid, void** ppv)
{
    TiffEncoder *This;
    HRESULT ret;

    TRACE("(%s,%p)\n", debugstr_guid(iid), ppv);

    *ppv = NULL;

    if (!load_libtiff())
    {
        ERR("Failed writing TIFF because unable to load %s\n",SONAME_LIBTIFF);
        return E_FAIL;
    }

    This = HeapAlloc(GetProcessHeap(), 0, sizeof(TiffEncoder));
    if (!This) return E_OUTOFMEMORY;

    This->IWICBitmapEncoder_iface.lpVtbl = &TiffEncoder_Vtbl;
    This->ref = 1;
    This->stream = NULL;
    InitializeCriticalSection(&This->lock);
    This->lock.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": TiffEncoder.lock");
    This->tiff = NULL;
    This->initialized = FALSE;
    This->num_frames = 0;
    This->num_frames_committed = 0;
    This->committed = FALSE;

    ret = IWICBitmapEncoder_QueryInterface(&This->IWICBitmapEncoder_iface, iid, ppv);
    IWICBitmapEncoder_Release(&This->IWICBitmapEncoder_iface);

    return ret;
}

#else /* !SONAME_LIBTIFF */

HRESULT TiffDecoder_CreateInstance(REFIID iid, void** ppv)
{
    ERR("Trying to load TIFF picture, but Wine was compiled without TIFF support.\n");
    return E_FAIL;
}

HRESULT TiffEncoder_CreateInstance(REFIID iid, void** ppv)
{
    ERR("Trying to save TIFF picture, but Wine was compiled without TIFF support.\n");
    return E_FAIL;
}

#endif
