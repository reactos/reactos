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
#include <math.h>
#include <sys/types.h>
#include <tiffio.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winternl.h"
#include "winbase.h"
#include "objbase.h"

#include "wincodecs_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(wincodecs);
#ifdef __REACTOS__
#include "wine/library.h"
extern int isfinite(double x);
static void tiff_error_handler( const char *module, const char *format, va_list args )
{

}
static void tiff_warning_handler( const char *module, const char *format, va_list args )
{

}
#else
WINE_DECLARE_DEBUG_CHANNEL(tiff);

static void tiff_error_handler( const char *module, const char *format, va_list args )
{
    if (!ERR_ON(tiff)) return;
    if (__wine_dbg_vlog( __WINE_DBCL_ERR, &__wine_dbch_tiff, module, format, args ) != -1)
        __wine_dbg_output( "\n" );
}

static void tiff_warning_handler( const char *module, const char *format, va_list args )
{
    if (!WARN_ON(tiff)) return;
    if (__wine_dbg_vlog( __WINE_DBCL_WARN, &__wine_dbch_tiff, module, format, args ) != -1)
        __wine_dbg_output( "\n" );
}
#endif

static tsize_t tiff_stream_read(thandle_t client_data, tdata_t data, tsize_t size)
{
    IStream *stream = (IStream*)client_data;
    ULONG bytes_read;
    HRESULT hr;

    hr = stream_read(stream, data, size, &bytes_read);
    if (FAILED(hr)) bytes_read = 0;
    return bytes_read;
}

static tsize_t tiff_stream_write(thandle_t client_data, tdata_t data, tsize_t size)
{
    IStream *stream = (IStream*)client_data;
    ULONG bytes_written;
    HRESULT hr;

    hr = stream_write(stream, data, size, &bytes_written);
    if (FAILED(hr)) bytes_written = 0;
    return bytes_written;
}

static toff_t tiff_stream_seek(thandle_t client_data, toff_t offset, int whence)
{
    IStream *stream = (IStream*)client_data;
    DWORD origin;
    ULONGLONG new_position;
    HRESULT hr;

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

    hr = stream_seek(stream, offset, origin, &new_position);
    if (SUCCEEDED(hr)) return new_position;
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
    ULONGLONG size;
    HRESULT hr;

    hr = stream_getsize(stream, &size);

    if (SUCCEEDED(hr)) return size;
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
    stream_seek(stream, 0, STREAM_SEEK_SET, NULL);

    return TIFFClientOpen("<IStream object>", mode, stream, tiff_stream_read,
        tiff_stream_write, (void *)tiff_stream_seek, tiff_stream_close,
        (void *)tiff_stream_size, (void *)tiff_stream_map, (void *)tiff_stream_unmap);
}

typedef struct {
    struct decoder_frame frame;
    int bps;
    int samples;
    int source_bpp;
    int planar;
    int indexed;
    int reverse_bgr;
    int invert_grayscale;
    UINT tile_width, tile_height;
    UINT tile_stride;
    UINT tile_size;
    int tiled;
    UINT tiles_across;
} tiff_decode_info;

struct tiff_decoder
{
    struct decoder decoder;
    IStream *stream;
    TIFF *tiff;
    DWORD frame_count;
    DWORD cached_frame;
    tiff_decode_info cached_decode_info;
    INT cached_tile_x, cached_tile_y;
    BYTE *cached_tile;
};

static inline struct tiff_decoder *impl_from_decoder(struct decoder* iface)
{
    return CONTAINING_RECORD(iface, struct tiff_decoder, decoder);
}

static HRESULT tiff_get_decode_info(TIFF *tiff, tiff_decode_info *decode_info)
{
    uint16_t photometric, bps, samples, planar;
    uint16_t extra_sample_count, extra_sample, *extra_samples;
    uint16_t *red, *green, *blue;
    UINT resolution_unit;
    float xres=0.0, yres=0.0;
    int ret, i;
    const BYTE *profile;
    UINT len;

    decode_info->indexed = 0;
    decode_info->reverse_bgr = 0;
    decode_info->invert_grayscale = 0;
    decode_info->tiled = 0;
    decode_info->source_bpp = 0;

    ret = TIFFGetField(tiff, TIFFTAG_PHOTOMETRIC, &photometric);
    if (!ret)
    {
        WARN("missing PhotometricInterpretation tag\n");
        return E_FAIL;
    }

    ret = TIFFGetField(tiff, TIFFTAG_BITSPERSAMPLE, &bps);
    if (!ret) bps = 1;
    decode_info->bps = bps;

    ret = TIFFGetField(tiff, TIFFTAG_SAMPLESPERPIXEL, &samples);
    if (!ret) samples = 1;
    decode_info->samples = samples;

    if (samples == 1)
        planar = 1;
    else
    {
        ret = TIFFGetField(tiff, TIFFTAG_PLANARCONFIG, &planar);
        if (!ret) planar = 1;
        if (planar != 1)
        {
            FIXME("unhandled planar configuration %u\n", planar);
            return E_FAIL;
        }
    }
    decode_info->planar = planar;

    TRACE("planar %u, photometric %u, samples %u, bps %u\n", planar, photometric, samples, bps);

    switch(photometric)
    {
    case 0: /* WhiteIsZero */
        decode_info->invert_grayscale = 1;
        /* fall through */
    case 1: /* BlackIsZero */
        if (samples == 2)
        {
            ret = TIFFGetField(tiff, TIFFTAG_EXTRASAMPLES, &extra_sample_count, &extra_samples);
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

        decode_info->frame.bpp = bps * samples;
        decode_info->source_bpp = decode_info->frame.bpp;
        switch (bps)
        {
        case 1:
            if (samples != 1)
            {
                FIXME("unhandled 1bpp sample count %u\n", samples);
                return E_FAIL;
            }
            decode_info->frame.pixel_format = GUID_WICPixelFormatBlackWhite;
            break;
        case 4:
            if (samples != 1)
            {
                FIXME("unhandled 4bpp grayscale sample count %u\n", samples);
                return E_FAIL;
            }
            decode_info->frame.pixel_format = GUID_WICPixelFormat4bppGray;
            break;
        case 8:
            if (samples == 1)
                decode_info->frame.pixel_format = GUID_WICPixelFormat8bppGray;
            else
            {
                decode_info->frame.bpp = 32;

                switch(extra_samples[0])
                {
                case 1: /* Associated (pre-multiplied) alpha data */
                    decode_info->frame.pixel_format = GUID_WICPixelFormat32bppPBGRA;
                    break;
                case 0: /* Unspecified data */
                case 2: /* Unassociated alpha data */
                    decode_info->frame.pixel_format = GUID_WICPixelFormat32bppBGRA;
                    break;
                default:
                    FIXME("unhandled extra sample type %u\n", extra_samples[0]);
                    return E_FAIL;
                }
            }
            break;
        case 16:
            if (samples != 1)
            {
                FIXME("unhandled 16bpp grayscale sample count %u\n", samples);
                return WINCODEC_ERR_UNSUPPORTEDPIXELFORMAT;
            }
            decode_info->frame.pixel_format = GUID_WICPixelFormat16bppGray;
            break;
        case 32:
            if (samples != 1)
            {
                FIXME("unhandled 32bpp grayscale sample count %u\n", samples);
                return WINCODEC_ERR_UNSUPPORTEDPIXELFORMAT;
            }
            decode_info->frame.pixel_format = GUID_WICPixelFormat32bppGrayFloat;
            break;
        default:
            WARN("unhandled greyscale bit count %u\n", bps);
            return WINCODEC_ERR_UNSUPPORTEDPIXELFORMAT;
        }
        break;
    case 2: /* RGB */
        if (samples == 4)
        {
            ret = TIFFGetField(tiff, TIFFTAG_EXTRASAMPLES, &extra_sample_count, &extra_samples);
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

        decode_info->frame.bpp = max(bps, 8) * samples;
        decode_info->source_bpp = bps * samples;
        switch(bps)
        {
        case 1:
        case 4:
        case 8:
            decode_info->reverse_bgr = 1;
            if (samples == 3)
                decode_info->frame.pixel_format = GUID_WICPixelFormat24bppBGR;
            else
                switch(extra_samples[0])
                {
                case 1: /* Associated (pre-multiplied) alpha data */
                    decode_info->frame.pixel_format = GUID_WICPixelFormat32bppPBGRA;
                    break;
                case 0: /* Unspecified data */
                case 2: /* Unassociated alpha data */
                    decode_info->frame.pixel_format = GUID_WICPixelFormat32bppBGRA;
                    break;
                default:
                    FIXME("unhandled extra sample type %i\n", extra_samples[0]);
                    return E_FAIL;
                }
            break;
        case 16:
            if (samples == 3)
                decode_info->frame.pixel_format = GUID_WICPixelFormat48bppRGB;
            else
                switch(extra_samples[0])
                {
                case 1: /* Associated (pre-multiplied) alpha data */
                    decode_info->frame.pixel_format = GUID_WICPixelFormat64bppPRGBA;
                    break;
                case 0: /* Unspecified data */
                case 2: /* Unassociated alpha data */
                    decode_info->frame.pixel_format = GUID_WICPixelFormat64bppRGBA;
                    break;
                default:
                    FIXME("unhandled extra sample type %i\n", extra_samples[0]);
                    return E_FAIL;
                }
            break;
        case 32:
            if (samples == 3)
                decode_info->frame.pixel_format = GUID_WICPixelFormat96bppRGBFloat;
            else
                switch(extra_samples[0])
                {
                case 1: /* Associated (pre-multiplied) alpha data */
                    decode_info->frame.pixel_format = GUID_WICPixelFormat128bppPRGBAFloat;
                    break;
                case 0: /* Unspecified data */
                case 2: /* Unassociated alpha data */
                    decode_info->frame.pixel_format = GUID_WICPixelFormat128bppRGBAFloat;
                    break;
                default:
                    FIXME("unhandled extra sample type %i\n", extra_samples[0]);
                    return E_FAIL;
                }
            break;
        default:
            WARN("unhandled RGB bit count %u\n", bps);
            return WINCODEC_ERR_UNSUPPORTEDPIXELFORMAT;
        }
        break;
    case 3: /* RGB Palette */
        if (samples != 1)
        {
            FIXME("unhandled indexed sample count %u\n", samples);
            return E_FAIL;
        }

        decode_info->indexed = 1;
        decode_info->frame.bpp = bps;
        switch (bps)
        {
        case 1:
            decode_info->frame.pixel_format = GUID_WICPixelFormat1bppIndexed;
            break;
        case 2:
            decode_info->frame.pixel_format = GUID_WICPixelFormat2bppIndexed;
            break;
        case 4:
            decode_info->frame.pixel_format = GUID_WICPixelFormat4bppIndexed;
            break;
        case 8:
            decode_info->frame.pixel_format = GUID_WICPixelFormat8bppIndexed;
            break;
        default:
            FIXME("unhandled indexed bit count %u\n", bps);
            return E_NOTIMPL;
        }
        break;

    case 5: /* Separated */
        if (samples != 4)
        {
            FIXME("unhandled Separated sample count %u\n", samples);
            return E_FAIL;
        }

        decode_info->frame.bpp = bps * samples;
        switch(bps)
        {
        case 8:
            decode_info->frame.pixel_format = GUID_WICPixelFormat32bppCMYK;
            break;
        case 16:
            decode_info->frame.pixel_format = GUID_WICPixelFormat64bppCMYK;
            break;

        default:
            WARN("unhandled Separated bit count %u\n", bps);
            return WINCODEC_ERR_UNSUPPORTEDPIXELFORMAT;
        }
        break;

    case 4: /* Transparency mask */
    case 6: /* YCbCr */
    case 8: /* CIELab */
    default:
        FIXME("unhandled PhotometricInterpretation %u\n", photometric);
        return E_FAIL;
    }

    ret = TIFFGetField(tiff, TIFFTAG_IMAGEWIDTH, &decode_info->frame.width);
    if (!ret)
    {
        WARN("missing image width\n");
        return E_FAIL;
    }

    ret = TIFFGetField(tiff, TIFFTAG_IMAGELENGTH, &decode_info->frame.height);
    if (!ret)
    {
        WARN("missing image length\n");
        return E_FAIL;
    }

    if ((ret = TIFFGetField(tiff, TIFFTAG_TILEWIDTH, &decode_info->tile_width)))
    {
        decode_info->tiled = 1;

        ret = TIFFGetField(tiff, TIFFTAG_TILELENGTH, &decode_info->tile_height);
        if (!ret)
        {
            WARN("missing tile height\n");
            return E_FAIL;
        }

        decode_info->tile_stride = ((decode_info->frame.bpp * decode_info->tile_width + 7)/8);
        decode_info->tile_size = decode_info->tile_height * decode_info->tile_stride;
        decode_info->tiles_across = (decode_info->frame.width + decode_info->tile_width - 1) / decode_info->tile_width;
    }
    else if ((ret = TIFFGetField(tiff, TIFFTAG_ROWSPERSTRIP, &decode_info->tile_height)))
    {
        if (decode_info->tile_height > decode_info->frame.height)
            decode_info->tile_height = decode_info->frame.height;
        decode_info->tile_width = decode_info->frame.width;
        decode_info->tile_stride = ((decode_info->frame.bpp * decode_info->tile_width + 7)/8);
        decode_info->tile_size = decode_info->tile_height * decode_info->tile_stride;
    }
    else
    {
        /* Some broken TIFF files have a single strip and lack the RowsPerStrip tag */
        decode_info->tile_height = decode_info->frame.height;
        decode_info->tile_width = decode_info->frame.width;
        decode_info->tile_stride = ((decode_info->frame.bpp * decode_info->tile_width + 7)/8);
        decode_info->tile_size = decode_info->tile_height * decode_info->tile_stride;
    }

    resolution_unit = 0;
    TIFFGetField(tiff, TIFFTAG_RESOLUTIONUNIT, &resolution_unit);

    ret = TIFFGetField(tiff, TIFFTAG_XRESOLUTION, &xres);
    if (!ret)
    {
        WARN("missing X resolution\n");
    }
    /* Emulate the behavior of current libtiff versions (libtiff commit a39f6131)
     * yielding 0 instead of INFINITY for IFD_RATIONAL fields with denominator 0. */
    if (!isfinite(xres))
    {
        xres = 0.0;
    }

    ret = TIFFGetField(tiff, TIFFTAG_YRESOLUTION, &yres);
    if (!ret)
    {
        WARN("missing Y resolution\n");
    }
    if (!isfinite(yres))
    {
        yres = 0.0;
    }

    if (xres == 0.0 || yres == 0.0)
    {
        decode_info->frame.dpix = decode_info->frame.dpiy = 96.0;
    }
    else
    {
        switch (resolution_unit)
        {
        default:
            FIXME("unknown resolution unit %i\n", resolution_unit);
            /* fall through */
        case 0: /* Not set */
        case 1: /* Relative measurements */
        case 2: /* Inch */
            decode_info->frame.dpix = xres;
            decode_info->frame.dpiy = yres;
            break;
        case 3: /* Centimeter */
            decode_info->frame.dpix = xres * 2.54;
            decode_info->frame.dpiy = yres * 2.54;
            break;
        }
    }

    if (decode_info->indexed &&
        TIFFGetField(tiff, TIFFTAG_COLORMAP, &red, &green, &blue))
    {
        decode_info->frame.num_colors = 1 << decode_info->bps;
        for (i=0; i<decode_info->frame.num_colors; i++)
        {
            decode_info->frame.palette[i] = 0xff000000 |
                ((red[i]<<8) & 0xff0000) |
                (green[i] & 0xff00) |
                ((blue[i]>>8) & 0xff);
        }
    }
    else
    {
        decode_info->frame.num_colors = 0;
    }

    if (TIFFGetField(tiff, TIFFTAG_ICCPROFILE, &len, &profile))
        decode_info->frame.num_color_contexts = 1;
    else
        decode_info->frame.num_color_contexts = 0;

    return S_OK;
}

static HRESULT CDECL tiff_decoder_initialize(struct decoder* iface, IStream *stream, struct decoder_stat *st)
{
    struct tiff_decoder *This = impl_from_decoder(iface);
    HRESULT hr;

    This->tiff = tiff_open_stream(stream, "r");
    if (!This->tiff)
        return E_FAIL;

    This->frame_count = TIFFNumberOfDirectories(This->tiff);
    This->cached_frame = 0;
    hr = tiff_get_decode_info(This->tiff, &This->cached_decode_info);
    if (FAILED(hr))
        goto fail;

    st->frame_count = This->frame_count;
    st->flags = WICBitmapDecoderCapabilityCanDecodeAllImages |
                WICBitmapDecoderCapabilityCanDecodeSomeImages |
                WICBitmapDecoderCapabilityCanEnumerateMetadata;
    return S_OK;

fail:
    TIFFClose(This->tiff);
    This->tiff = NULL;
    return hr;
}

static HRESULT tiff_decoder_select_frame(struct tiff_decoder* This, DWORD frame)
{
    HRESULT hr;
    UINT prev_tile_size;
    int res;

    if (frame >= This->frame_count)
        return E_INVALIDARG;

    if (This->cached_frame == frame)
        return S_OK;

    prev_tile_size = This->cached_tile ? This->cached_decode_info.tile_size : 0;

    res = TIFFSetDirectory(This->tiff, frame);
    if (!res)
        return E_INVALIDARG;

    hr = tiff_get_decode_info(This->tiff, &This->cached_decode_info);

    This->cached_tile_x = -1;

    if (SUCCEEDED(hr))
    {
        This->cached_frame = frame;
        if (This->cached_decode_info.tile_size > prev_tile_size)
        {
            free(This->cached_tile);
            This->cached_tile = NULL;
        }
    }
    else
    {
        /* Set an invalid value to ensure we'll refresh cached_decode_info before using it. */
        This->cached_frame = This->frame_count;
        free(This->cached_tile);
        This->cached_tile = NULL;
    }

    return hr;
}

static HRESULT CDECL tiff_decoder_get_frame_info(struct decoder* iface, UINT frame, struct decoder_frame *info)
{
    struct tiff_decoder *This = impl_from_decoder(iface);
    HRESULT hr;

    hr = tiff_decoder_select_frame(This, frame);
    if (SUCCEEDED(hr))
    {
        *info = This->cached_decode_info.frame;
    }

    return hr;
}

static HRESULT tiff_decoder_read_tile(struct tiff_decoder *This, UINT tile_x, UINT tile_y)
{
    tsize_t ret;
    int swap_bytes;
    tiff_decode_info *info = &This->cached_decode_info;

    swap_bytes = TIFFIsByteSwapped(This->tiff);

    if (info->tiled)
        ret = TIFFReadEncodedTile(This->tiff, tile_x + tile_y * info->tiles_across, This->cached_tile, info->tile_size);
    else
        ret = TIFFReadEncodedStrip(This->tiff, tile_y, This->cached_tile, info->tile_size);

    if (ret == -1)
        return E_FAIL;

    /* 3bps RGB */
    if (info->source_bpp == 3 && info->samples == 3 && info->frame.bpp == 24)
    {
        BYTE *srcdata, *src, *dst;
        DWORD x, y, count, width_bytes = (info->tile_width * 3 + 7) / 8;

        count = width_bytes * info->tile_height;

        srcdata = malloc(count);
        if (!srcdata) return E_OUTOFMEMORY;
        memcpy(srcdata, This->cached_tile, count);

        for (y = 0; y < info->tile_height; y++)
        {
            src = srcdata + y * width_bytes;
            dst = This->cached_tile + y * info->tile_width * 3;

            for (x = 0; x < info->tile_width; x += 8)
            {
                dst[2] = (src[0] & 0x80) ? 0xff : 0; /* R */
                dst[1] = (src[0] & 0x40) ? 0xff : 0; /* G */
                dst[0] = (src[0] & 0x20) ? 0xff : 0; /* B */
                if (x + 1 < info->tile_width)
                {
                    dst[5] = (src[0] & 0x10) ? 0xff : 0; /* R */
                    dst[4] = (src[0] & 0x08) ? 0xff : 0; /* G */
                    dst[3] = (src[0] & 0x04) ? 0xff : 0; /* B */
                }
                if (x + 2 < info->tile_width)
                {
                    dst[8] = (src[0] & 0x02) ? 0xff : 0; /* R */
                    dst[7] = (src[0] & 0x01) ? 0xff : 0; /* G */
                    dst[6]  = (src[1] & 0x80) ? 0xff : 0; /* B */
                }
                if (x + 3 < info->tile_width)
                {
                    dst[11] = (src[1] & 0x40) ? 0xff : 0; /* R */
                    dst[10] = (src[1] & 0x20) ? 0xff : 0; /* G */
                    dst[9]  = (src[1] & 0x10) ? 0xff : 0; /* B */
                }
                if (x + 4 < info->tile_width)
                {
                    dst[14] = (src[1] & 0x08) ? 0xff : 0; /* R */
                    dst[13] = (src[1] & 0x04) ? 0xff : 0; /* G */
                    dst[12] = (src[1] & 0x02) ? 0xff : 0; /* B */
                }
                if (x + 5 < info->tile_width)
                {
                    dst[17] = (src[1] & 0x01) ? 0xff : 0; /* R */
                    dst[16] = (src[2] & 0x80) ? 0xff : 0; /* G */
                    dst[15] = (src[2] & 0x40) ? 0xff : 0; /* B */
                }
                if (x + 6 < info->tile_width)
                {
                    dst[20] = (src[2] & 0x20) ? 0xff : 0; /* R */
                    dst[19] = (src[2] & 0x10) ? 0xff : 0; /* G */
                    dst[18] = (src[2] & 0x08) ? 0xff : 0; /* B */
                }
                if (x + 7 < info->tile_width)
                {
                    dst[23] = (src[2] & 0x04) ? 0xff : 0; /* R */
                    dst[22] = (src[2] & 0x02) ? 0xff : 0; /* G */
                    dst[21] = (src[2] & 0x01) ? 0xff : 0; /* B */
                }
                src += 3;
                dst += 24;
            }
        }

        free(srcdata);
    }
    /* 12bps RGB */
    else if (info->source_bpp == 12 && info->samples == 3 && info->frame.bpp == 24)
    {
        BYTE *srcdata, *src, *dst;
        DWORD x, y, count, width_bytes = (info->tile_width * 12 + 7) / 8;

        count = width_bytes * info->tile_height;

        srcdata = malloc(count);
        if (!srcdata) return E_OUTOFMEMORY;
        memcpy(srcdata, This->cached_tile, count);

        for (y = 0; y < info->tile_height; y++)
        {
            src = srcdata + y * width_bytes;
            dst = This->cached_tile + y * info->tile_width * 3;

            for (x = 0; x < info->tile_width; x += 2)
            {
                dst[0] = ((src[1] & 0xf0) >> 4) * 17; /* B */
                dst[1] = (src[0] & 0x0f) * 17; /* G */
                dst[2] = ((src[0] & 0xf0) >> 4) * 17; /* R */
                if (x + 1 < info->tile_width)
                {
                    dst[5] = (src[1] & 0x0f) * 17; /* B */
                    dst[4] = ((src[2] & 0xf0) >> 4) * 17; /* G */
                    dst[3] = (src[2] & 0x0f) * 17; /* R */
                }
                src += 3;
                dst += 6;
            }
        }

        free(srcdata);
    }
    /* 4bps RGBA */
    else if (info->source_bpp == 4 && info->samples == 4 && info->frame.bpp == 32)
    {
        BYTE *srcdata, *src, *dst;
        DWORD x, y, count, width_bytes = (info->tile_width * 3 + 7) / 8;

        count = width_bytes * info->tile_height;

        srcdata = malloc(count);
        if (!srcdata) return E_OUTOFMEMORY;
        memcpy(srcdata, This->cached_tile, count);

        for (y = 0; y < info->tile_height; y++)
        {
            src = srcdata + y * width_bytes;
            dst = This->cached_tile + y * info->tile_width * 4;

            /* 1 source byte expands to 2 BGRA samples */

            for (x = 0; x < info->tile_width; x += 2)
            {
                dst[0] = (src[0] & 0x20) ? 0xff : 0; /* B */
                dst[1] = (src[0] & 0x40) ? 0xff : 0; /* G */
                dst[2] = (src[0] & 0x80) ? 0xff : 0; /* R */
                dst[3] = (src[0] & 0x10) ? 0xff : 0; /* A */
                if (x + 1 < info->tile_width)
                {
                    dst[4] = (src[0] & 0x02) ? 0xff : 0; /* B */
                    dst[5] = (src[0] & 0x04) ? 0xff : 0; /* G */
                    dst[6] = (src[0] & 0x08) ? 0xff : 0; /* R */
                    dst[7] = (src[0] & 0x01) ? 0xff : 0; /* A */
                }
                src++;
                dst += 8;
            }
        }

        free(srcdata);
    }
    /* 16bps RGBA */
    else if (info->source_bpp == 16 && info->samples == 4 && info->frame.bpp == 32)
    {
        BYTE *srcdata, *src, *dst;
        DWORD x, y, count, width_bytes = (info->tile_width * 12 + 7) / 8;

        count = width_bytes * info->tile_height;

        srcdata = malloc(count);
        if (!srcdata) return E_OUTOFMEMORY;
        memcpy(srcdata, This->cached_tile, count);

        for (y = 0; y < info->tile_height; y++)
        {
            src = srcdata + y * width_bytes;
            dst = This->cached_tile + y * info->tile_width * 4;

            for (x = 0; x < info->tile_width; x++)
            {
                dst[0] = ((src[1] & 0xf0) >> 4) * 17; /* B */
                dst[1] = (src[0] & 0x0f) * 17; /* G */
                dst[2] = ((src[0] & 0xf0) >> 4) * 17; /* R */
                dst[3] = (src[1] & 0x0f) * 17; /* A */
                src += 2;
                dst += 4;
            }
        }

        free(srcdata);
    }
    /* 8bpp grayscale with extra alpha */
    else if (info->source_bpp == 16 && info->samples == 2 && info->frame.bpp == 32)
    {
        BYTE *src;
        DWORD *dst, count = info->tile_width * info->tile_height;

        src = This->cached_tile + info->tile_width * info->tile_height * 2 - 2;
        dst = (DWORD *)(This->cached_tile + info->tile_size - 4);

        while (count--)
        {
            *dst-- = src[0] | (src[0] << 8) | (src[0] << 16) | (src[1] << 24);
            src -= 2;
        }
    }

    if (info->reverse_bgr)
    {
        if (info->bps == 8)
        {
            UINT sample_count = info->samples;

            reverse_bgr8(sample_count, This->cached_tile, info->tile_width,
                info->tile_height, info->tile_width * sample_count);
        }
    }

    if (swap_bytes && info->bps > 8)
    {
        UINT row, i, samples_per_row;
        BYTE *sample, temp;

        samples_per_row = info->tile_width * info->samples;

        switch(info->bps)
        {
        case 16:
            for (row=0; row<info->tile_height; row++)
            {
                sample = This->cached_tile + row * info->tile_stride;
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
            ERR("unhandled bps for byte swap %u\n", info->bps);
            return E_FAIL;
        }
    }

    if (info->invert_grayscale)
    {
        BYTE *byte, *end;

        if (info->samples != 1)
        {
            ERR("cannot invert grayscale image with %u samples\n", info->samples);
            return E_FAIL;
        }

        end = This->cached_tile+info->tile_size;

        for (byte = This->cached_tile; byte != end; byte++)
            *byte = ~(*byte);
    }

    This->cached_tile_x = tile_x;
    This->cached_tile_y = tile_y;

    return S_OK;
}

static HRESULT CDECL tiff_decoder_copy_pixels(struct decoder* iface, UINT frame,
    const WICRect *prc, UINT stride, UINT buffersize, BYTE *buffer)
{
    struct tiff_decoder *This = impl_from_decoder(iface);
    HRESULT hr;
    UINT min_tile_x, max_tile_x, min_tile_y, max_tile_y;
    UINT tile_x, tile_y;
    BYTE *dst_tilepos;
    WICRect rc;
    tiff_decode_info *info = &This->cached_decode_info;

    hr = tiff_decoder_select_frame(This, frame);
    if (FAILED(hr))
        return hr;

    if (!This->cached_tile)
    {
        This->cached_tile = malloc(info->tile_size);
        if (!This->cached_tile)
            return E_OUTOFMEMORY;
    }

    min_tile_x = prc->X / info->tile_width;
    min_tile_y = prc->Y / info->tile_height;
    max_tile_x = (prc->X+prc->Width-1) / info->tile_width;
    max_tile_y = (prc->Y+prc->Height-1) / info->tile_height;

    for (tile_x=min_tile_x; tile_x <= max_tile_x; tile_x++)
    {
        for (tile_y=min_tile_y; tile_y <= max_tile_y; tile_y++)
        {
            if (tile_x != This->cached_tile_x || tile_y != This->cached_tile_y)
            {
                hr = tiff_decoder_read_tile(This, tile_x, tile_y);
            }

            if (SUCCEEDED(hr))
            {
                if (prc->X < tile_x * info->tile_width)
                    rc.X = 0;
                else
                    rc.X = prc->X - tile_x * info->tile_width;

                if (prc->Y < tile_y * info->tile_height)
                    rc.Y = 0;
                else
                    rc.Y = prc->Y - tile_y * info->tile_height;

                if (prc->X+prc->Width > (tile_x+1) * info->tile_width)
                    rc.Width = info->tile_width - rc.X;
                else if (prc->X < tile_x * info->tile_width)
                    rc.Width = prc->Width + prc->X - tile_x * info->tile_width;
                else
                    rc.Width = prc->Width;

                if (prc->Y+prc->Height > (tile_y+1) * info->tile_height)
                    rc.Height = info->tile_height - rc.Y;
                else if (prc->Y < tile_y * info->tile_height)
                    rc.Height = prc->Height + prc->Y - tile_y * info->tile_height;
                else
                    rc.Height = prc->Height;

                dst_tilepos = buffer + (stride * ((rc.Y + tile_y * info->tile_height) - prc->Y)) +
                    ((info->frame.bpp * ((rc.X + tile_x * info->tile_width) - prc->X) + 7) / 8);

                hr = copy_pixels(info->frame.bpp, This->cached_tile,
                    info->tile_width, info->tile_height, info->tile_stride,
                    &rc, stride, buffersize, dst_tilepos);
            }

            if (FAILED(hr))
            {
                TRACE("<-- 0x%lx\n", hr);
                return hr;
            }
        }
    }

    return S_OK;
}

static HRESULT CDECL tiff_decoder_get_color_context(struct decoder *iface,
    UINT frame, UINT num, BYTE **data, DWORD *datasize)
{
    struct tiff_decoder *This = impl_from_decoder(iface);
    const BYTE *profile;
    UINT len;
    HRESULT hr;

    hr = tiff_decoder_select_frame(This, frame);
    if (FAILED(hr))
        return hr;

    if (!TIFFGetField(This->tiff, TIFFTAG_ICCPROFILE, &len, &profile))
    {
        return E_UNEXPECTED;
    }

    *datasize = len;
    *data = malloc(len);
    if (!*data)
        return E_OUTOFMEMORY;

    memcpy(*data, profile, len);

    return S_OK;
}

static HRESULT CDECL tiff_decoder_get_metadata_blocks(struct decoder *iface,
    UINT frame, UINT *count, struct decoder_block **blocks)
{
    struct tiff_decoder *This = impl_from_decoder(iface);
    HRESULT hr;
    BOOL byte_swapped;
    struct decoder_block result;

    hr = tiff_decoder_select_frame(This, frame);
    if (FAILED(hr))
        return hr;

    *count = 1;

    result.offset = TIFFCurrentDirOffset(This->tiff);
    result.length = 0;

    byte_swapped = TIFFIsByteSwapped(This->tiff);
#ifdef WORDS_BIGENDIAN
    result.options = byte_swapped ? WICPersistOptionLittleEndian : WICPersistOptionBigEndian;
#else
    result.options = byte_swapped ? WICPersistOptionBigEndian : WICPersistOptionLittleEndian;
#endif
    result.options |= WICPersistOptionNoCacheStream|DECODER_BLOCK_FULL_STREAM|DECODER_BLOCK_READER_CLSID;
    result.reader_clsid = CLSID_WICIfdMetadataReader;

    *blocks = malloc(sizeof(**blocks));
    **blocks = result;

    return S_OK;
}

static void CDECL tiff_decoder_destroy(struct decoder* iface)
{
    struct tiff_decoder *This = impl_from_decoder(iface);
    if (This->tiff) TIFFClose(This->tiff);
    free(This->cached_tile);
    free(This);
}

static const struct decoder_funcs tiff_decoder_vtable = {
    tiff_decoder_initialize,
    tiff_decoder_get_frame_info,
    tiff_decoder_copy_pixels,
    tiff_decoder_get_metadata_blocks,
    tiff_decoder_get_color_context,
    tiff_decoder_destroy
};

HRESULT CDECL tiff_decoder_create(struct decoder_info *info, struct decoder **result)
{
    struct tiff_decoder *This;

    This = malloc(sizeof(*This));
    if (!This) return E_OUTOFMEMORY;

    This->decoder.vtable = &tiff_decoder_vtable;
    This->tiff = NULL;
    This->cached_tile = NULL;
    This->cached_tile_x = -1;
    *result = &This->decoder;

    info->container_format = GUID_ContainerFormatTiff;
    info->block_format = GUID_ContainerFormatTiff;
    info->clsid = CLSID_WICTiffDecoder;

    TIFFSetErrorHandler( tiff_error_handler );
    TIFFSetWarningHandler( tiff_warning_handler );
    return S_OK;
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
    int indexed;
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
    {&GUID_WICPixelFormat1bppIndexed, 3, 1, 1, 1, 0, 0, 0, 1},
    {&GUID_WICPixelFormat4bppIndexed, 3, 4, 1, 4, 0, 0, 0, 1},
    {&GUID_WICPixelFormat8bppIndexed, 3, 8, 1, 8, 0, 0, 0, 1},
    {0}
};

typedef struct tiff_encoder {
    struct encoder encoder;
    TIFF *tiff;
    const struct tiff_encode_format *format;
    struct encoder_frame encoder_frame;
    DWORD num_frames;
    DWORD lines_written;
} tiff_encoder;

static inline struct tiff_encoder *impl_from_encoder(struct encoder* iface)
{
    return CONTAINING_RECORD(iface, struct tiff_encoder, encoder);
}

static HRESULT CDECL tiff_encoder_initialize(struct encoder* iface, IStream *stream)
{
    struct tiff_encoder* This = impl_from_encoder(iface);
    TIFF *tiff;

    tiff = tiff_open_stream(stream, "w");

    if (!tiff)
        return E_FAIL;

    This->tiff = tiff;

    return S_OK;
}

static HRESULT CDECL tiff_encoder_get_supported_format(struct encoder *iface,
    GUID *pixel_format, DWORD *bpp, BOOL *indexed)
{
    int i;

    if (IsEqualGUID(pixel_format, &GUID_WICPixelFormat2bppIndexed))
        *pixel_format = GUID_WICPixelFormat4bppIndexed;

    for (i=0; formats[i].guid; i++)
    {
        if (IsEqualGUID(formats[i].guid, pixel_format))
            break;
    }

    if (!formats[i].guid) i = 0;

    *pixel_format = *formats[i].guid;
    *bpp = formats[i].bpp;
    *indexed = formats[i].indexed;

    return S_OK;
}

static HRESULT CDECL tiff_encoder_create_frame(struct encoder* iface, const struct encoder_frame *frame)
{
    struct tiff_encoder* This = impl_from_encoder(iface);
    int i;

    if (This->num_frames != 0)
        TIFFWriteDirectory(This->tiff);

    This->num_frames++;
    This->lines_written = 0;
    This->encoder_frame = *frame;

    for (i=0; formats[i].guid; i++)
    {
        if (IsEqualGUID(formats[i].guid, &frame->pixel_format))
            break;
    }

    This->format = &formats[i];

    TIFFSetField(This->tiff, TIFFTAG_PHOTOMETRIC, (uint16_t)This->format->photometric);
    TIFFSetField(This->tiff, TIFFTAG_PLANARCONFIG, (uint16_t)1);
    TIFFSetField(This->tiff, TIFFTAG_BITSPERSAMPLE, (uint16_t)This->format->bps);
    TIFFSetField(This->tiff, TIFFTAG_SAMPLESPERPIXEL, (uint16_t)This->format->samples);

    if (This->format->extra_sample)
    {
        uint16_t extra_samples;
        extra_samples = This->format->extra_sample_type;

        TIFFSetField(This->tiff, TIFFTAG_EXTRASAMPLES, (uint16_t)1, &extra_samples);
    }

    TIFFSetField(This->tiff, TIFFTAG_IMAGEWIDTH, (uint32_t)frame->width);
    TIFFSetField(This->tiff, TIFFTAG_IMAGELENGTH, (uint32_t)frame->height);

    if (frame->dpix != 0.0 && frame->dpiy != 0.0)
    {
        TIFFSetField(This->tiff, TIFFTAG_RESOLUTIONUNIT, (uint16_t)2); /* Inch */
        TIFFSetField(This->tiff, TIFFTAG_XRESOLUTION, (float)frame->dpix);
        TIFFSetField(This->tiff, TIFFTAG_YRESOLUTION, (float)frame->dpiy);
    }

    if (This->format->bpp <= 8 && frame->num_colors && This->format->indexed)
    {
        uint16_t red[256], green[256], blue[256];
        UINT i;

        for (i = 0; i < frame->num_colors; i++)
        {
            red[i] = (frame->palette[i] >> 8) & 0xff00;
            green[i] = frame->palette[i] & 0xff00;
            blue[i] = (frame->palette[i] << 8) & 0xff00;
        }

        TIFFSetField(This->tiff, TIFFTAG_COLORMAP, red, green, blue);
    }

    return S_OK;
}

static HRESULT CDECL tiff_encoder_write_lines(struct encoder* iface,
    BYTE *data, DWORD line_count, DWORD stride)
{
    struct tiff_encoder* This = impl_from_encoder(iface);
    BYTE *row_data, *swapped_data = NULL;
    UINT i, j, line_size;

    line_size = ((This->encoder_frame.width * This->format->bpp)+7)/8;

    if (This->format->reverse_bgr)
    {
        swapped_data = malloc(line_size);
        if (!swapped_data)
            return E_OUTOFMEMORY;
    }

    for (i=0; i<line_count; i++)
    {
        row_data = data + i * stride;

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

        TIFFWriteScanline(This->tiff, (tdata_t)row_data, i+This->lines_written, 0);
    }

    This->lines_written += line_count;

    return S_OK;
}

static HRESULT CDECL tiff_encoder_commit_frame(struct encoder* iface)
{
    return S_OK;
}

static HRESULT CDECL tiff_encoder_commit_file(struct encoder* iface)
{
    struct tiff_encoder* This = impl_from_encoder(iface);

    TIFFClose(This->tiff);
    This->tiff = NULL;

    return S_OK;
}

static void CDECL tiff_encoder_destroy(struct encoder* iface)
{
    struct tiff_encoder *This = impl_from_encoder(iface);

    if (This->tiff) TIFFClose(This->tiff);
    free(This);
}

static const struct encoder_funcs tiff_encoder_vtable = {
    tiff_encoder_initialize,
    tiff_encoder_get_supported_format,
    tiff_encoder_create_frame,
    tiff_encoder_write_lines,
    tiff_encoder_commit_frame,
    tiff_encoder_commit_file,
    tiff_encoder_destroy
};

HRESULT CDECL tiff_encoder_create(struct encoder_info *info, struct encoder **result)
{
    struct tiff_encoder *This;

    This = malloc(sizeof(*This));
    if (!This) return E_OUTOFMEMORY;

    This->encoder.vtable = &tiff_encoder_vtable;
    This->tiff = NULL;
    This->num_frames = 0;

    info->flags = ENCODER_FLAGS_MULTI_FRAME | ENCODER_FLAGS_SUPPORTS_METADATA;
    info->container_format = GUID_ContainerFormatTiff;
    info->clsid = CLSID_WICTiffEncoder;
    info->encoder_options[0] = ENCODER_OPTION_COMPRESSION_METHOD;
    info->encoder_options[1] = ENCODER_OPTION_COMPRESSION_QUALITY;
    info->encoder_options[2] = ENCODER_OPTION_END;

    *result = &This->encoder;

    TIFFSetErrorHandler( tiff_error_handler );
    TIFFSetWarningHandler( tiff_warning_handler );
    return S_OK;
}
