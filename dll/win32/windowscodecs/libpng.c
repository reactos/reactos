/*
 * Copyright 2016 Dmitry Timoshkov
 * Copyright 2020 Esme Povirk
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
#include <png.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winternl.h"
#include "winbase.h"
#include "objbase.h"

#include "wincodecs_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(wincodecs);

struct png_decoder
{
    struct decoder decoder;
    IStream *stream;
    struct decoder_frame decoder_frame;
    UINT stride;
    BYTE *image_bits;
    BYTE *color_profile;
    DWORD color_profile_len;
};

static inline struct png_decoder *impl_from_decoder(struct decoder* iface)
{
    return CONTAINING_RECORD(iface, struct png_decoder, decoder);
}

static void user_read_data(png_structp png_ptr, png_bytep data, png_size_t length)
{
    IStream *stream = png_get_io_ptr(png_ptr);
    HRESULT hr;
    ULONG bytesread;

    hr = stream_read(stream, data, length, &bytesread);
    if (FAILED(hr) || bytesread != length)
    {
        png_error(png_ptr, "failed reading data");
    }
}

static HRESULT CDECL png_decoder_initialize(struct decoder *iface, IStream *stream, struct decoder_stat *st)
{
    struct png_decoder *This = impl_from_decoder(iface);
    png_structp png_ptr;
    png_infop info_ptr;
    HRESULT hr = E_FAIL;
    int color_type, bit_depth;
    png_bytep trans;
    int num_trans;
    png_uint_32 transparency;
    png_color_16p trans_values;
    png_uint_32 ret, xres, yres;
    int unit_type;
    png_colorp png_palette;
    int num_palette;
    int i;
    UINT image_size;
    png_bytep *row_pointers=NULL;
    png_charp cp_name;
    png_bytep cp_profile;
    png_uint_32 cp_len;
    int cp_compression;

    png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png_ptr)
    {
        return E_FAIL;
    }

    info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr)
    {
        png_destroy_read_struct(&png_ptr, NULL, NULL);
        return E_FAIL;
    }

    /* set up setjmp/longjmp error handling */
    if (setjmp(png_jmpbuf(png_ptr)))
    {
        hr = WINCODEC_ERR_UNKNOWNIMAGEFORMAT;
        goto end;
    }
    png_set_crc_action(png_ptr, PNG_CRC_QUIET_USE, PNG_CRC_QUIET_USE);
    png_set_chunk_malloc_max(png_ptr, 0);

    /* seek to the start of the stream */
    hr = stream_seek(stream, 0, STREAM_SEEK_SET, NULL);
    if (FAILED(hr))
    {
        goto end;
    }

    /* set up custom i/o handling */
    png_set_read_fn(png_ptr, stream, user_read_data);

    /* read the header */
    png_read_info(png_ptr, info_ptr);

    /* choose a pixel format */
    color_type = png_get_color_type(png_ptr, info_ptr);
    bit_depth = png_get_bit_depth(png_ptr, info_ptr);

    /* PNGs with bit-depth greater than 8 are network byte order. Windows does not expect this. */
    if (bit_depth > 8)
        png_set_swap(png_ptr);

    /* check for color-keyed alpha */
    transparency = png_get_tRNS(png_ptr, info_ptr, &trans, &num_trans, &trans_values);
    if (!transparency)
        num_trans = 0;

    if (transparency && (color_type == PNG_COLOR_TYPE_RGB ||
        (color_type == PNG_COLOR_TYPE_GRAY && bit_depth == 16)))
    {
        /* expand to RGBA */
        if (color_type == PNG_COLOR_TYPE_GRAY)
            png_set_gray_to_rgb(png_ptr);
        png_set_tRNS_to_alpha(png_ptr);
        color_type = PNG_COLOR_TYPE_RGB_ALPHA;
    }

    switch (color_type)
    {
    case PNG_COLOR_TYPE_GRAY_ALPHA:
        /* WIC does not support grayscale alpha formats so use RGBA */
        png_set_gray_to_rgb(png_ptr);
        /* fall through */
    case PNG_COLOR_TYPE_RGB_ALPHA:
        This->decoder_frame.bpp = bit_depth * 4;
        switch (bit_depth)
        {
        case 8:
            png_set_bgr(png_ptr);
            This->decoder_frame.pixel_format = GUID_WICPixelFormat32bppBGRA;
            break;
        case 16: This->decoder_frame.pixel_format = GUID_WICPixelFormat64bppRGBA; break;
        default:
            ERR("invalid RGBA bit depth: %i\n", bit_depth);
            hr = E_FAIL;
            goto end;
        }
        break;
    case PNG_COLOR_TYPE_GRAY:
        This->decoder_frame.bpp = bit_depth;
        if (!transparency)
        {
            switch (bit_depth)
            {
            case 1: This->decoder_frame.pixel_format = GUID_WICPixelFormatBlackWhite; break;
            case 2: This->decoder_frame.pixel_format = GUID_WICPixelFormat2bppGray; break;
            case 4: This->decoder_frame.pixel_format = GUID_WICPixelFormat4bppGray; break;
            case 8: This->decoder_frame.pixel_format = GUID_WICPixelFormat8bppGray; break;
            case 16: This->decoder_frame.pixel_format = GUID_WICPixelFormat16bppGray; break;
            default:
                ERR("invalid grayscale bit depth: %i\n", bit_depth);
                hr = E_FAIL;
                goto end;
            }
            break;
        }
        /* else fall through */
    case PNG_COLOR_TYPE_PALETTE:
        This->decoder_frame.bpp = bit_depth;
        switch (bit_depth)
        {
        case 1: This->decoder_frame.pixel_format = GUID_WICPixelFormat1bppIndexed; break;
        case 2: This->decoder_frame.pixel_format = GUID_WICPixelFormat2bppIndexed; break;
        case 4: This->decoder_frame.pixel_format = GUID_WICPixelFormat4bppIndexed; break;
        case 8: This->decoder_frame.pixel_format = GUID_WICPixelFormat8bppIndexed; break;
        default:
            ERR("invalid indexed color bit depth: %i\n", bit_depth);
            hr = E_FAIL;
            goto end;
        }
        break;
    case PNG_COLOR_TYPE_RGB:
        This->decoder_frame.bpp = bit_depth * 3;
        switch (bit_depth)
        {
        case 8:
            png_set_bgr(png_ptr);
            This->decoder_frame.pixel_format = GUID_WICPixelFormat24bppBGR;
            break;
        case 16: This->decoder_frame.pixel_format = GUID_WICPixelFormat48bppRGB; break;
        default:
            ERR("invalid RGB color bit depth: %i\n", bit_depth);
            hr = E_FAIL;
            goto end;
        }
        break;
    default:
        ERR("invalid color type %i\n", color_type);
        hr = E_FAIL;
        goto end;
    }

    This->decoder_frame.width = png_get_image_width(png_ptr, info_ptr);
    This->decoder_frame.height = png_get_image_height(png_ptr, info_ptr);

    ret = png_get_pHYs(png_ptr, info_ptr, &xres, &yres, &unit_type);

    if (ret && unit_type == PNG_RESOLUTION_METER)
    {
        This->decoder_frame.dpix = xres * 0.0254;
        This->decoder_frame.dpiy = yres * 0.0254;
    }
    else
    {
        WARN("no pHYs block present\n");
        This->decoder_frame.dpix = This->decoder_frame.dpiy = 96.0;
    }

    ret = png_get_iCCP(png_ptr, info_ptr, &cp_name, &cp_compression, &cp_profile, &cp_len);
    if (ret)
    {
        This->decoder_frame.num_color_contexts = 1;
        This->color_profile_len = cp_len;
        This->color_profile = malloc(cp_len);
        if (!This->color_profile)
        {
            hr = E_OUTOFMEMORY;
            goto end;
        }
        memcpy(This->color_profile, cp_profile, cp_len);
    }
    else
        This->decoder_frame.num_color_contexts = 0;

    if (color_type == PNG_COLOR_TYPE_PALETTE)
    {
        ret = png_get_PLTE(png_ptr, info_ptr, &png_palette, &num_palette);
        if (!ret)
        {
            ERR("paletted image with no PLTE chunk\n");
            hr = E_FAIL;
            goto end;
        }

        if (num_palette > 256)
        {
            ERR("palette has %i colors?!\n", num_palette);
            hr = E_FAIL;
            goto end;
        }

        This->decoder_frame.num_colors = num_palette;
        for (i=0; i<num_palette; i++)
        {
            BYTE alpha = (i < num_trans) ? trans[i] : 0xff;
            This->decoder_frame.palette[i] = (alpha << 24 |
                                              png_palette[i].red << 16|
                                              png_palette[i].green << 8|
                                              png_palette[i].blue);
        }
    }
    else if (color_type == PNG_COLOR_TYPE_GRAY && transparency && bit_depth <= 8) {
        num_palette = 1 << bit_depth;

        This->decoder_frame.num_colors = num_palette;
        for (i=0; i<num_palette; i++)
        {
            BYTE alpha = (i == trans_values[0].gray) ? 0 : 0xff;
            BYTE val = i * 255 / (num_palette - 1);
            This->decoder_frame.palette[i] = (alpha << 24 | val << 16 | val << 8 | val);
        }
    }
    else
    {
        This->decoder_frame.num_colors = 0;
    }

    This->stride = (This->decoder_frame.width * This->decoder_frame.bpp + 7) / 8;
    image_size = This->stride * This->decoder_frame.height;

    This->image_bits = malloc(image_size);
    if (!This->image_bits)
    {
        hr = E_OUTOFMEMORY;
        goto end;
    }

    row_pointers = malloc(sizeof(png_bytep)*This->decoder_frame.height);
    if (!row_pointers)
    {
        hr = E_OUTOFMEMORY;
        goto end;
    }

    for (i=0; i<This->decoder_frame.height; i++)
        row_pointers[i] = This->image_bits + i * This->stride;

    png_read_image(png_ptr, row_pointers);

    free(row_pointers);
    row_pointers = NULL;

    /* png_read_end intentionally not called to not seek to the end of the file */

    st->flags = WICBitmapDecoderCapabilityCanDecodeAllImages |
                WICBitmapDecoderCapabilityCanDecodeSomeImages |
                WICBitmapDecoderCapabilityCanEnumerateMetadata;
    st->frame_count = 1;

    This->stream = stream;

    hr = S_OK;

end:
    png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
    free(row_pointers);
    if (FAILED(hr))
    {
        free(This->image_bits);
        This->image_bits = NULL;
        free(This->color_profile);
        This->color_profile = NULL;
    }
    return hr;
}

static HRESULT CDECL png_decoder_get_frame_info(struct decoder *iface, UINT frame, struct decoder_frame *info)
{
    struct png_decoder *This = impl_from_decoder(iface);
    *info = This->decoder_frame;
    return S_OK;
}

static HRESULT CDECL png_decoder_copy_pixels(struct decoder *iface, UINT frame,
    const WICRect *prc, UINT stride, UINT buffersize, BYTE *buffer)
{
    struct png_decoder *This = impl_from_decoder(iface);

    return copy_pixels(This->decoder_frame.bpp, This->image_bits,
        This->decoder_frame.width, This->decoder_frame.height, This->stride,
        prc, stride, buffersize, buffer);
}

static HRESULT CDECL png_decoder_get_metadata_blocks(struct decoder* iface,
    UINT frame, UINT *count, struct decoder_block **blocks)
{
    struct png_decoder *This = impl_from_decoder(iface);
    HRESULT hr;
    struct decoder_block *result = NULL;
    ULONGLONG seek;
    BYTE chunk_type[4];
    ULONG chunk_size;
    ULONGLONG chunk_start;
    ULONG metadata_blocks_size = 0;

    seek = 8;
    *count = 0;

    do
    {
        hr = stream_seek(This->stream, seek, STREAM_SEEK_SET, &chunk_start);
        if (FAILED(hr)) goto end;

        hr = read_png_chunk(This->stream, chunk_type, NULL, &chunk_size);
        if (FAILED(hr)) goto end;

        if (chunk_type[0] >= 'a' && chunk_type[0] <= 'z' &&
            memcmp(chunk_type, "tRNS", 4) && memcmp(chunk_type, "pHYs", 4))
        {
            /* This chunk is considered metadata. */
            if (*count == metadata_blocks_size)
            {
                struct decoder_block *new_metadata_blocks;
                ULONG new_metadata_blocks_size;

                new_metadata_blocks_size = 4 + metadata_blocks_size * 2;
                new_metadata_blocks = malloc(new_metadata_blocks_size * sizeof(*new_metadata_blocks));

                if (!new_metadata_blocks)
                {
                    hr = E_OUTOFMEMORY;
                    goto end;
                }

                memcpy(new_metadata_blocks, result,
                    *count * sizeof(*new_metadata_blocks));

                free(result);
                result = new_metadata_blocks;
                metadata_blocks_size = new_metadata_blocks_size;
            }

            result[*count].offset = chunk_start;
            result[*count].length = chunk_size + 12;
            result[*count].options = WICMetadataCreationAllowUnknown;
            (*count)++;
        }

        seek = chunk_start + chunk_size + 12; /* skip data and CRC */
    } while (memcmp(chunk_type, "IEND", 4));

end:
    if (SUCCEEDED(hr))
    {
        *blocks = result;
    }
    else
    {
        *count = 0;
        *blocks = NULL;
        free(result);
    }
    return hr;
}

static HRESULT CDECL png_decoder_get_color_context(struct decoder* iface, UINT frame, UINT num,
    BYTE **data, DWORD *datasize)
{
    struct png_decoder *This = impl_from_decoder(iface);

    *data = malloc(This->color_profile_len);
    *datasize = This->color_profile_len;

    if (!*data)
        return E_OUTOFMEMORY;

    memcpy(*data, This->color_profile, This->color_profile_len);

    return S_OK;
}

static void CDECL png_decoder_destroy(struct decoder* iface)
{
    struct png_decoder *This = impl_from_decoder(iface);

    free(This->image_bits);
    free(This->color_profile);
    free(This);
}

static const struct decoder_funcs png_decoder_vtable = {
    png_decoder_initialize,
    png_decoder_get_frame_info,
    png_decoder_copy_pixels,
    png_decoder_get_metadata_blocks,
    png_decoder_get_color_context,
    png_decoder_destroy
};

HRESULT CDECL png_decoder_create(struct decoder_info *info, struct decoder **result)
{
    struct png_decoder *This;

    This = malloc(sizeof(*This));

    if (!This)
    {
        return E_OUTOFMEMORY;
    }

    This->decoder.vtable = &png_decoder_vtable;
    This->image_bits = NULL;
    This->color_profile = NULL;
    *result = &This->decoder;

    info->container_format = GUID_ContainerFormatPng;
    info->block_format = GUID_ContainerFormatPng;
    info->clsid = CLSID_WICPngDecoder;

    return S_OK;
}

struct png_pixelformat {
    const WICPixelFormatGUID *guid;
    UINT bpp;
    int bit_depth;
    int color_type;
    BOOL remove_filler;
    BOOL swap_rgb;
};

static const struct png_pixelformat formats[] = {
    {&GUID_WICPixelFormat32bppBGRA, 32, 8, PNG_COLOR_TYPE_RGB_ALPHA, 0, 1},
    {&GUID_WICPixelFormat24bppBGR, 24, 8, PNG_COLOR_TYPE_RGB, 0, 1},
    {&GUID_WICPixelFormatBlackWhite, 1, 1, PNG_COLOR_TYPE_GRAY, 0, 0},
    {&GUID_WICPixelFormat2bppGray, 2, 2, PNG_COLOR_TYPE_GRAY, 0, 0},
    {&GUID_WICPixelFormat4bppGray, 4, 4, PNG_COLOR_TYPE_GRAY, 0, 0},
    {&GUID_WICPixelFormat8bppGray, 8, 8, PNG_COLOR_TYPE_GRAY, 0, 0},
    {&GUID_WICPixelFormat16bppGray, 16, 16, PNG_COLOR_TYPE_GRAY, 0, 0},
    {&GUID_WICPixelFormat32bppBGR, 32, 8, PNG_COLOR_TYPE_RGB, 1, 1},
    {&GUID_WICPixelFormat48bppRGB, 48, 16, PNG_COLOR_TYPE_RGB, 0, 0},
    {&GUID_WICPixelFormat64bppRGBA, 64, 16, PNG_COLOR_TYPE_RGB_ALPHA, 0, 0},
    {&GUID_WICPixelFormat1bppIndexed, 1, 1, PNG_COLOR_TYPE_PALETTE, 0, 0},
    {&GUID_WICPixelFormat2bppIndexed, 2, 2, PNG_COLOR_TYPE_PALETTE, 0, 0},
    {&GUID_WICPixelFormat4bppIndexed, 4, 4, PNG_COLOR_TYPE_PALETTE, 0, 0},
    {&GUID_WICPixelFormat8bppIndexed, 8, 8, PNG_COLOR_TYPE_PALETTE, 0, 0},
    {NULL},
};

struct png_encoder
{
    struct encoder encoder;
    IStream *stream;
    png_structp png_ptr;
    png_infop info_ptr;
    struct encoder_frame encoder_frame;
    const struct png_pixelformat *format;
    BYTE *data;
    UINT stride;
    UINT passes;
    UINT lines_written;
};

static inline struct png_encoder *impl_from_encoder(struct encoder* iface)
{
    return CONTAINING_RECORD(iface, struct png_encoder, encoder);
}

static void user_write_data(png_structp png_ptr, png_bytep data, png_size_t length)
{
    struct png_encoder *This = png_get_io_ptr(png_ptr);
    HRESULT hr;
    ULONG byteswritten;

    hr = stream_write(This->stream, data, length, &byteswritten);
    if (FAILED(hr) || byteswritten != length)
    {
        png_error(png_ptr, "failed writing data");
    }
}

static void user_flush(png_structp png_ptr)
{
}

static HRESULT CDECL png_encoder_initialize(struct encoder *encoder, IStream *stream)
{
    struct png_encoder *This = impl_from_encoder(encoder);

    TRACE("(%p,%p)\n", encoder, stream);

    /* initialize libpng */
    This->png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!This->png_ptr)
        return E_FAIL;

    This->info_ptr = png_create_info_struct(This->png_ptr);
    if (!This->info_ptr)
    {
        png_destroy_write_struct(&This->png_ptr, NULL);
        This->png_ptr = NULL;
        return E_FAIL;
    }

    This->stream = stream;

    /* set up setjmp/longjmp error handling */
    if (setjmp(png_jmpbuf(This->png_ptr)))
    {
        png_destroy_write_struct(&This->png_ptr, &This->info_ptr);
        This->png_ptr = NULL;
        This->stream = NULL;
        return E_FAIL;
    }

    /* set up custom i/o handling */
    png_set_write_fn(This->png_ptr, This, user_write_data, user_flush);

    return S_OK;
}

static HRESULT CDECL png_encoder_get_supported_format(struct encoder* iface, GUID *pixel_format, DWORD *bpp, BOOL *indexed)
{
    int i;

    for (i=0; formats[i].guid; i++)
    {
        if (memcmp(formats[i].guid, pixel_format, sizeof(GUID)) == 0)
            break;
    }

    if (!formats[i].guid)
        i = 0;

    *pixel_format = *formats[i].guid;
    *bpp = formats[i].bpp;
    *indexed = (formats[i].color_type == PNG_COLOR_TYPE_PALETTE);

    return S_OK;
}

static HRESULT CDECL png_encoder_create_frame(struct encoder *encoder, const struct encoder_frame *encoder_frame)
{
    struct png_encoder *This = impl_from_encoder(encoder);
    int i;

    for (i=0; formats[i].guid; i++)
    {
        if (memcmp(formats[i].guid, &encoder_frame->pixel_format, sizeof(GUID)) == 0)
        {
            This->format = &formats[i];
            break;
        }
    }

    if (!formats[i].guid)
    {
        ERR("invalid pixel format %s\n", wine_dbgstr_guid(&encoder_frame->pixel_format));
        return E_FAIL;
    }

    /* set up setjmp/longjmp error handling */
    if (setjmp(png_jmpbuf(This->png_ptr)))
        return E_FAIL;

    This->encoder_frame = *encoder_frame;
    This->lines_written = 0;

    if (encoder_frame->interlace)
    {
        /* libpng requires us to write all data multiple times in this case. */
        This->stride = (This->format->bpp * encoder_frame->width + 7)/8;
        This->data = malloc(encoder_frame->height * This->stride);
        if (!This->data)
            return E_OUTOFMEMORY;
    }

    /* Tell PNG we need to byte swap if writing a >8-bpp image */
    if (This->format->bit_depth > 8)
        png_set_swap(This->png_ptr);

    png_set_IHDR(This->png_ptr, This->info_ptr, encoder_frame->width, encoder_frame->height,
        This->format->bit_depth, This->format->color_type,
        encoder_frame->interlace ? PNG_INTERLACE_ADAM7 : PNG_INTERLACE_NONE,
        PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

    if (encoder_frame->dpix != 0.0 && encoder_frame->dpiy != 0.0)
    {
        png_set_pHYs(This->png_ptr, This->info_ptr, (encoder_frame->dpix+0.0127) / 0.0254,
            (encoder_frame->dpiy+0.0127) / 0.0254, PNG_RESOLUTION_METER);
    }

    if (This->format->color_type == PNG_COLOR_TYPE_PALETTE && encoder_frame->num_colors)
    {
        png_color png_palette[256];
        png_byte trans[256];
        UINT i, num_trans = 0, colors;

        /* Newer libpng versions don't accept larger palettes than the declared
         * bit depth, so we need to generate the palette of the correct length.
         */
        colors = min(encoder_frame->num_colors, 1 << This->format->bit_depth);

        for (i = 0; i < colors; i++)
        {
            png_palette[i].red = (encoder_frame->palette[i] >> 16) & 0xff;
            png_palette[i].green = (encoder_frame->palette[i] >> 8) & 0xff;
            png_palette[i].blue = encoder_frame->palette[i] & 0xff;
            trans[i] = (encoder_frame->palette[i] >> 24) & 0xff;
            if (trans[i] != 0xff)
                num_trans = i+1;
        }

        png_set_PLTE(This->png_ptr, This->info_ptr, png_palette, colors);

        if (num_trans)
            png_set_tRNS(This->png_ptr, This->info_ptr, trans, num_trans, NULL);
    }

    png_write_info(This->png_ptr, This->info_ptr);

    if (This->format->remove_filler)
        png_set_filler(This->png_ptr, 0, PNG_FILLER_AFTER);

    if (This->format->swap_rgb)
        png_set_bgr(This->png_ptr);

    if (encoder_frame->interlace)
        This->passes = png_set_interlace_handling(This->png_ptr);

    if (encoder_frame->filter != WICPngFilterUnspecified)
    {
        static const int png_filter_map[] =
        {
            /* WICPngFilterUnspecified */ PNG_NO_FILTERS,
            /* WICPngFilterNone */        PNG_FILTER_NONE,
            /* WICPngFilterSub */         PNG_FILTER_SUB,
            /* WICPngFilterUp */          PNG_FILTER_UP,
            /* WICPngFilterAverage */     PNG_FILTER_AVG,
            /* WICPngFilterPaeth */       PNG_FILTER_PAETH,
            /* WICPngFilterAdaptive */    PNG_ALL_FILTERS,
        };

        png_set_filter(This->png_ptr, 0, png_filter_map[encoder_frame->filter]);
    }

    return S_OK;
}

static HRESULT CDECL png_encoder_write_lines(struct encoder* encoder, BYTE *data, DWORD line_count, DWORD stride)
{
    struct png_encoder *This = impl_from_encoder(encoder);
    png_byte **row_pointers=NULL;
    UINT i;

    if (This->encoder_frame.interlace)
    {
        /* Just store the data so we can write it in multiple passes in Commit. */
        for (i=0; i<line_count; i++)
            memcpy(This->data + This->stride * (This->lines_written + i),
                   data + stride * i,
                   This->stride);

        This->lines_written += line_count;

        return S_OK;
    }

    /* set up setjmp/longjmp error handling */
    if (setjmp(png_jmpbuf(This->png_ptr)))
    {
        free(row_pointers);
        return E_FAIL;
    }

    row_pointers = malloc(line_count * sizeof(png_byte*));
    if (!row_pointers)
        return E_OUTOFMEMORY;

    for (i=0; i<line_count; i++)
        row_pointers[i] = data + stride * i;

    png_write_rows(This->png_ptr, row_pointers, line_count);
    This->lines_written += line_count;

    free(row_pointers);

    return S_OK;
}

static HRESULT CDECL png_encoder_commit_frame(struct encoder *encoder)
{
    struct png_encoder *This = impl_from_encoder(encoder);
    png_byte **row_pointers=NULL;

    /* set up setjmp/longjmp error handling */
    if (setjmp(png_jmpbuf(This->png_ptr)))
    {
        free(row_pointers);
        return E_FAIL;
    }

    if (This->encoder_frame.interlace)
    {
        int i;

        row_pointers = malloc(This->encoder_frame.height * sizeof(png_byte*));
        if (!row_pointers)
            return E_OUTOFMEMORY;

        for (i=0; i<This->encoder_frame.height; i++)
            row_pointers[i] = This->data + This->stride * i;

        for (i=0; i<This->passes; i++)
            png_write_rows(This->png_ptr, row_pointers, This->encoder_frame.height);
    }

    png_write_end(This->png_ptr, This->info_ptr);

    free(row_pointers);

    return S_OK;
}

static HRESULT CDECL png_encoder_commit_file(struct encoder *encoder)
{
    return S_OK;
}

static void CDECL png_encoder_destroy(struct encoder *encoder)
{
    struct png_encoder *This = impl_from_encoder(encoder);
    if (This->png_ptr)
        png_destroy_write_struct(&This->png_ptr, &This->info_ptr);
    free(This->data);
    free(This);
}

static const struct encoder_funcs png_encoder_vtable = {
    png_encoder_initialize,
    png_encoder_get_supported_format,
    png_encoder_create_frame,
    png_encoder_write_lines,
    png_encoder_commit_frame,
    png_encoder_commit_file,
    png_encoder_destroy
};

HRESULT CDECL png_encoder_create(struct encoder_info *info, struct encoder **result)
{
    struct png_encoder *This;

    This = malloc(sizeof(*This));

    if (!This)
    {
        return E_OUTOFMEMORY;
    }

    This->encoder.vtable = &png_encoder_vtable;
    This->png_ptr = NULL;
    This->info_ptr = NULL;
    This->data = NULL;
    *result = &This->encoder;

    info->flags = ENCODER_FLAGS_SUPPORTS_METADATA;
    info->container_format = GUID_ContainerFormatPng;
    info->clsid = CLSID_WICPngEncoder;
    info->encoder_options[0] = ENCODER_OPTION_INTERLACE;
    info->encoder_options[1] = ENCODER_OPTION_FILTER;
    info->encoder_options[2] = ENCODER_OPTION_END;

    return S_OK;
}
