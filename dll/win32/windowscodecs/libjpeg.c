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

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <setjmp.h>
#include <basetsd.h>
#include <jpeglib.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winternl.h"
#include "winbase.h"
#include "objbase.h"

#include "wincodecs_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(wincodecs);
WINE_DECLARE_DEBUG_CHANNEL(jpeg);

static void error_exit_fn(j_common_ptr cinfo)
{
    char message[JMSG_LENGTH_MAX];
    if (ERR_ON(jpeg))
    {
        cinfo->err->format_message(cinfo, message);
        ERR_(jpeg)("%s\n", message);
    }
    longjmp(*(jmp_buf*)cinfo->client_data, 1);
}

static void emit_message_fn(j_common_ptr cinfo, int msg_level)
{
    char message[JMSG_LENGTH_MAX];

    if (msg_level < 0 && ERR_ON(jpeg))
    {
        cinfo->err->format_message(cinfo, message);
        ERR_(jpeg)("%s\n", message);
    }
    else if (msg_level == 0 && WARN_ON(jpeg))
    {
        cinfo->err->format_message(cinfo, message);
        WARN_(jpeg)("%s\n", message);
    }
    else if (msg_level > 0 && TRACE_ON(jpeg))
    {
        cinfo->err->format_message(cinfo, message);
        TRACE_(jpeg)("%s\n", message);
    }
}

struct jpeg_decoder {
    struct decoder decoder;
    struct decoder_frame frame;
    BOOL cinfo_initialized;
    IStream *stream;
    struct jpeg_decompress_struct cinfo;
    struct jpeg_error_mgr jerr;
    struct jpeg_source_mgr source_mgr;
    BYTE source_buffer[1024];
    UINT stride;
    BYTE *image_data;
};

static inline struct jpeg_decoder *impl_from_decoder(struct decoder* iface)
{
    return CONTAINING_RECORD(iface, struct jpeg_decoder, decoder);
}

static inline struct jpeg_decoder *decoder_from_decompress(j_decompress_ptr decompress)
{
    return CONTAINING_RECORD(decompress, struct jpeg_decoder, cinfo);
}

static void CDECL jpeg_decoder_destroy(struct decoder* iface)
{
    struct jpeg_decoder *This = impl_from_decoder(iface);

    if (This->cinfo_initialized) jpeg_destroy_decompress(&This->cinfo);
    free(This->image_data);
    free(This);
}

static void source_mgr_init_source(j_decompress_ptr cinfo)
{
}

static boolean source_mgr_fill_input_buffer(j_decompress_ptr cinfo)
{
    struct jpeg_decoder *This = decoder_from_decompress(cinfo);
    HRESULT hr;
    ULONG bytesread;

    hr = stream_read(This->stream, This->source_buffer, 1024, &bytesread);

    if (FAILED(hr) || bytesread == 0)
    {
        return FALSE;
    }
    else
    {
        This->source_mgr.next_input_byte = This->source_buffer;
        This->source_mgr.bytes_in_buffer = bytesread;
        return TRUE;
    }
}

static void source_mgr_skip_input_data(j_decompress_ptr cinfo, long num_bytes)
{
    struct jpeg_decoder *This = decoder_from_decompress(cinfo);

    if (num_bytes > This->source_mgr.bytes_in_buffer)
    {
        stream_seek(This->stream, num_bytes - This->source_mgr.bytes_in_buffer, STREAM_SEEK_CUR, NULL);
        This->source_mgr.bytes_in_buffer = 0;
    }
    else if (num_bytes > 0)
    {
        This->source_mgr.next_input_byte += num_bytes;
        This->source_mgr.bytes_in_buffer -= num_bytes;
    }
}

static void source_mgr_term_source(j_decompress_ptr cinfo)
{
}

static HRESULT CDECL jpeg_decoder_initialize(struct decoder* iface, IStream *stream, struct decoder_stat *st)
{
    struct jpeg_decoder *This = impl_from_decoder(iface);
    int ret;
    jmp_buf jmpbuf;
    UINT data_size, i;

    if (This->cinfo_initialized)
        return WINCODEC_ERR_WRONGSTATE;

    jpeg_std_error(&This->jerr);

    This->jerr.error_exit = error_exit_fn;
    This->jerr.emit_message = emit_message_fn;

    This->cinfo.err = &This->jerr;

    This->cinfo.client_data = jmpbuf;

    if (setjmp(jmpbuf))
        return E_FAIL;

    jpeg_CreateDecompress(&This->cinfo, JPEG_LIB_VERSION, sizeof(struct jpeg_decompress_struct));

    This->cinfo_initialized = TRUE;

    This->stream = stream;

    stream_seek(This->stream, 0, STREAM_SEEK_SET, NULL);

    This->source_mgr.bytes_in_buffer = 0;
    This->source_mgr.init_source = source_mgr_init_source;
    This->source_mgr.fill_input_buffer = source_mgr_fill_input_buffer;
    This->source_mgr.skip_input_data = source_mgr_skip_input_data;
    This->source_mgr.resync_to_restart = jpeg_resync_to_restart;
    This->source_mgr.term_source = source_mgr_term_source;

    This->cinfo.src = &This->source_mgr;

    ret = jpeg_read_header(&This->cinfo, TRUE);

    if (ret != JPEG_HEADER_OK) {
        WARN("Jpeg image in stream has bad format, read header returned %d.\n",ret);
        return E_FAIL;
    }

    switch (This->cinfo.jpeg_color_space)
    {
    case JCS_GRAYSCALE:
        This->cinfo.out_color_space = JCS_GRAYSCALE;
        This->frame.bpp = 8;
        This->frame.pixel_format = GUID_WICPixelFormat8bppGray;
        break;
    case JCS_RGB:
    case JCS_YCbCr:
        This->cinfo.out_color_space = JCS_RGB;
        This->frame.bpp = 24;
        This->frame.pixel_format = GUID_WICPixelFormat24bppBGR;
        break;
    case JCS_CMYK:
    case JCS_YCCK:
        This->cinfo.out_color_space = JCS_CMYK;
        This->frame.bpp = 32;
        This->frame.pixel_format = GUID_WICPixelFormat32bppCMYK;
        break;
    default:
        ERR("Unknown JPEG color space %i\n", This->cinfo.jpeg_color_space);
        return E_FAIL;
    }

    if (!jpeg_start_decompress(&This->cinfo))
    {
        ERR("jpeg_start_decompress failed\n");
        return E_FAIL;
    }

    This->frame.width = This->cinfo.output_width;
    This->frame.height = This->cinfo.output_height;

    switch (This->cinfo.density_unit)
    {
    case 2: /* pixels per centimeter */
        This->frame.dpix = This->cinfo.X_density * 2.54;
        This->frame.dpiy = This->cinfo.Y_density * 2.54;
        break;

    case 1: /* pixels per inch */
        This->frame.dpix = This->cinfo.X_density;
        This->frame.dpiy = This->cinfo.Y_density;
        break;

    case 0: /* unknown */
    default:
        This->frame.dpix = This->frame.dpiy = 96.0;
        break;
    }

    This->frame.num_color_contexts = 0;
    This->frame.num_colors = 0;

    This->stride = (This->frame.bpp * This->cinfo.output_width + 7) / 8;
    data_size = This->stride * This->cinfo.output_height;

    if (data_size / This->stride < This->cinfo.output_height)
        /* overflow in multiplication */
        return E_OUTOFMEMORY;

    This->image_data = malloc(data_size);
    if (!This->image_data)
        return E_OUTOFMEMORY;

    while (This->cinfo.output_scanline < This->cinfo.output_height)
    {
        UINT first_scanline = This->cinfo.output_scanline;
        UINT max_rows;
        JSAMPROW out_rows[4];
        JDIMENSION ret;

        max_rows = min(This->cinfo.output_height-first_scanline, 4);
        for (i=0; i<max_rows; i++)
            out_rows[i] = This->image_data + This->stride * (first_scanline+i);

        ret = jpeg_read_scanlines(&This->cinfo, out_rows, max_rows);
        if (ret == 0)
        {
            ERR("read_scanlines failed\n");
            return E_FAIL;
        }
    }

    if (This->frame.bpp == 24)
    {
        /* libjpeg gives us RGB data and we want BGR, so byteswap the data */
        reverse_bgr8(3, This->image_data,
            This->cinfo.output_width, This->cinfo.output_height,
            This->stride);
    }

    if (This->cinfo.out_color_space == JCS_CMYK && This->cinfo.saw_Adobe_marker)
    {
        /* Adobe JPEG's have inverted CMYK data. */
        for (i=0; i<data_size; i++)
            This->image_data[i] ^= 0xff;
    }

    st->frame_count = 1;
    st->flags = WICBitmapDecoderCapabilityCanDecodeAllImages |
                WICBitmapDecoderCapabilityCanDecodeSomeImages |
                WICBitmapDecoderCapabilityCanEnumerateMetadata |
                DECODER_FLAGS_UNSUPPORTED_COLOR_CONTEXT;
    return S_OK;
}

static HRESULT CDECL jpeg_decoder_get_frame_info(struct decoder* iface, UINT frame, struct decoder_frame *info)
{
    struct jpeg_decoder *This = impl_from_decoder(iface);
    *info = This->frame;
    return S_OK;
}

static HRESULT CDECL jpeg_decoder_copy_pixels(struct decoder* iface, UINT frame,
    const WICRect *prc, UINT stride, UINT buffersize, BYTE *buffer)
{
    struct jpeg_decoder *This = impl_from_decoder(iface);
    return copy_pixels(This->frame.bpp, This->image_data,
        This->frame.width, This->frame.height, This->stride,
        prc, stride, buffersize, buffer);
}

static HRESULT CDECL jpeg_decoder_get_metadata_blocks(struct decoder* iface, UINT frame,
    UINT *count, struct decoder_block **blocks)
{
    FIXME("stub\n");
    *count = 0;
    *blocks = NULL;
    return S_OK;
}

static HRESULT CDECL jpeg_decoder_get_color_context(struct decoder* This, UINT frame, UINT num,
    BYTE **data, DWORD *datasize)
{
    /* This should never be called because we report 0 color contexts and the unsupported flag. */
    FIXME("stub\n");
    return E_NOTIMPL;
}

static const struct decoder_funcs jpeg_decoder_vtable = {
    jpeg_decoder_initialize,
    jpeg_decoder_get_frame_info,
    jpeg_decoder_copy_pixels,
    jpeg_decoder_get_metadata_blocks,
    jpeg_decoder_get_color_context,
    jpeg_decoder_destroy
};

HRESULT CDECL jpeg_decoder_create(struct decoder_info *info, struct decoder **result)
{
    struct jpeg_decoder *This;

    This = malloc(sizeof(struct jpeg_decoder));
    if (!This) return E_OUTOFMEMORY;

    This->decoder.vtable = &jpeg_decoder_vtable;
    This->cinfo_initialized = FALSE;
    This->stream = NULL;
    This->image_data = NULL;
    *result = &This->decoder;

    info->container_format = GUID_ContainerFormatJpeg;
    info->block_format = GUID_ContainerFormatJpeg;
    info->clsid = CLSID_WICJpegDecoder;

    return S_OK;
}

typedef struct jpeg_compress_format {
    const WICPixelFormatGUID *guid;
    int bpp;
    int num_components;
    J_COLOR_SPACE color_space;
    int swap_rgb;
} jpeg_compress_format;

static const jpeg_compress_format compress_formats[] = {
    { &GUID_WICPixelFormat24bppBGR, 24, 3, JCS_RGB, 1 },
    { &GUID_WICPixelFormat32bppCMYK, 32, 4, JCS_CMYK },
    { &GUID_WICPixelFormat8bppGray, 8, 1, JCS_GRAYSCALE },
    { 0 }
};

struct jpeg_encoder
{
    struct encoder encoder;
    IStream *stream;
    BOOL cinfo_initialized;
    struct jpeg_compress_struct cinfo;
    struct jpeg_error_mgr jerr;
    struct jpeg_destination_mgr dest_mgr;
    struct encoder_frame encoder_frame;
    const jpeg_compress_format *format;
    BYTE dest_buffer[1024];
};

static inline struct jpeg_encoder *impl_from_encoder(struct encoder* iface)
{
    return CONTAINING_RECORD(iface, struct jpeg_encoder, encoder);
}

static inline struct jpeg_encoder *encoder_from_compress(j_compress_ptr compress)
{
    return CONTAINING_RECORD(compress, struct jpeg_encoder, cinfo);
}

static void dest_mgr_init_destination(j_compress_ptr cinfo)
{
    struct jpeg_encoder *This = encoder_from_compress(cinfo);

    This->dest_mgr.next_output_byte = This->dest_buffer;
    This->dest_mgr.free_in_buffer = sizeof(This->dest_buffer);
}

static boolean dest_mgr_empty_output_buffer(j_compress_ptr cinfo)
{
    struct jpeg_encoder *This = encoder_from_compress(cinfo);
    HRESULT hr;
    ULONG byteswritten;

    hr = stream_write(This->stream, This->dest_buffer,
        sizeof(This->dest_buffer), &byteswritten);

    if (hr != S_OK || byteswritten == 0)
    {
        ERR("Failed writing data, hr=%lx\n", hr);
        return FALSE;
    }

    This->dest_mgr.next_output_byte = This->dest_buffer;
    This->dest_mgr.free_in_buffer = sizeof(This->dest_buffer);
    return TRUE;
}

static void dest_mgr_term_destination(j_compress_ptr cinfo)
{
    struct jpeg_encoder *This = encoder_from_compress(cinfo);
    ULONG byteswritten;
    HRESULT hr;

    if (This->dest_mgr.free_in_buffer != sizeof(This->dest_buffer))
    {
        hr = stream_write(This->stream, This->dest_buffer,
            sizeof(This->dest_buffer) - This->dest_mgr.free_in_buffer, &byteswritten);

        if (hr != S_OK || byteswritten == 0)
            ERR("Failed writing data, hr=%lx\n", hr);
    }
}

static HRESULT CDECL jpeg_encoder_initialize(struct encoder* iface, IStream *stream)
{
    struct jpeg_encoder *This = impl_from_encoder(iface);
    jmp_buf jmpbuf;

    jpeg_std_error(&This->jerr);

    This->jerr.error_exit = error_exit_fn;
    This->jerr.emit_message = emit_message_fn;

    This->cinfo.err = &This->jerr;

    This->cinfo.client_data = jmpbuf;

    if (setjmp(jmpbuf))
        return E_FAIL;

    jpeg_CreateCompress(&This->cinfo, JPEG_LIB_VERSION, sizeof(struct jpeg_compress_struct));

    This->stream = stream;

    This->dest_mgr.next_output_byte = This->dest_buffer;
    This->dest_mgr.free_in_buffer = sizeof(This->dest_buffer);

    This->dest_mgr.init_destination = dest_mgr_init_destination;
    This->dest_mgr.empty_output_buffer = dest_mgr_empty_output_buffer;
    This->dest_mgr.term_destination = dest_mgr_term_destination;

    This->cinfo.dest = &This->dest_mgr;

    This->cinfo_initialized = TRUE;

    return S_OK;
}

static HRESULT CDECL jpeg_encoder_get_supported_format(struct encoder* iface, GUID *pixel_format,
    DWORD *bpp, BOOL *indexed)
{
    int i;

    for (i=0; compress_formats[i].guid; i++)
    {
        if (memcmp(compress_formats[i].guid, pixel_format, sizeof(GUID)) == 0)
            break;
    }

    if (!compress_formats[i].guid) i = 0;

    *pixel_format = *compress_formats[i].guid;
    *bpp = compress_formats[i].bpp;
    *indexed = FALSE;

    return S_OK;
}

static HRESULT CDECL jpeg_encoder_create_frame(struct encoder* iface, const struct encoder_frame *frame)
{
    struct jpeg_encoder *This = impl_from_encoder(iface);
    jmp_buf jmpbuf;
    int i;

    This->encoder_frame = *frame;

    if (setjmp(jmpbuf))
        return E_FAIL;

    This->cinfo.client_data = jmpbuf;

    for (i=0; compress_formats[i].guid; i++)
    {
        if (memcmp(compress_formats[i].guid, &frame->pixel_format, sizeof(GUID)) == 0)
            break;
    }
    This->format = &compress_formats[i];

    This->cinfo.image_width = frame->width;
    This->cinfo.image_height = frame->height;
    This->cinfo.input_components = This->format->num_components;
    This->cinfo.in_color_space = This->format->color_space;

    jpeg_set_defaults(&This->cinfo);

    if (frame->dpix != 0.0 && frame->dpiy != 0.0)
    {
        This->cinfo.density_unit = 1; /* dots per inch */
        This->cinfo.X_density = frame->dpix;
        This->cinfo.Y_density = frame->dpiy;
    }

    jpeg_start_compress(&This->cinfo, TRUE);

    return S_OK;
}

static HRESULT CDECL jpeg_encoder_write_lines(struct encoder* iface, BYTE *data,
    DWORD line_count, DWORD stride)
{
    struct jpeg_encoder *This = impl_from_encoder(iface);
    jmp_buf jmpbuf;
    BYTE *swapped_data = NULL, *current_row;
    UINT line;
    int row_size;

    if (setjmp(jmpbuf))
    {
        free(swapped_data);
        return E_FAIL;
    }

    This->cinfo.client_data = jmpbuf;

    row_size = This->format->bpp / 8 * This->encoder_frame.width;

    if (This->format->swap_rgb)
    {
        swapped_data = malloc(row_size);
        if (!swapped_data)
            return E_OUTOFMEMORY;
    }

    for (line=0; line < line_count; line++)
    {
        if (This->format->swap_rgb)
        {
            UINT x;

            memcpy(swapped_data, data + (stride * line), row_size);

            for (x=0; x < This->encoder_frame.width; x++)
            {
                BYTE b;

                b = swapped_data[x*3];
                swapped_data[x*3] = swapped_data[x*3+2];
                swapped_data[x*3+2] = b;
            }

            current_row = swapped_data;
        }
        else
            current_row = data + (stride * line);

        if (!jpeg_write_scanlines(&This->cinfo, &current_row, 1))
        {
            ERR("failed writing scanlines\n");
            free(swapped_data);
            return E_FAIL;
        }
    }

    free(swapped_data);

    return S_OK;
}

static HRESULT CDECL jpeg_encoder_commit_frame(struct encoder* iface)
{
    struct jpeg_encoder *This = impl_from_encoder(iface);
    jmp_buf jmpbuf;

    if (setjmp(jmpbuf))
        return E_FAIL;

    This->cinfo.client_data = jmpbuf;

    jpeg_finish_compress(&This->cinfo);

    return S_OK;
}

static HRESULT CDECL jpeg_encoder_commit_file(struct encoder* iface)
{
    return S_OK;
}

static void CDECL jpeg_encoder_destroy(struct encoder* iface)
{
    struct jpeg_encoder *This = impl_from_encoder(iface);
    if (This->cinfo_initialized)
        jpeg_destroy_compress(&This->cinfo);
    free(This);
};

static const struct encoder_funcs jpeg_encoder_vtable = {
    jpeg_encoder_initialize,
    jpeg_encoder_get_supported_format,
    jpeg_encoder_create_frame,
    jpeg_encoder_write_lines,
    jpeg_encoder_commit_frame,
    jpeg_encoder_commit_file,
    jpeg_encoder_destroy
};

HRESULT CDECL jpeg_encoder_create(struct encoder_info *info, struct encoder **result)
{
    struct jpeg_encoder *This;

    This = malloc(sizeof(struct jpeg_encoder));
    if (!This) return E_OUTOFMEMORY;

    This->encoder.vtable = &jpeg_encoder_vtable;
    This->stream = NULL;
    This->cinfo_initialized = FALSE;
    *result = &This->encoder;

    info->flags = ENCODER_FLAGS_SUPPORTS_METADATA;
    info->container_format = GUID_ContainerFormatJpeg;
    info->clsid = CLSID_WICJpegEncoder;
    info->encoder_options[0] = ENCODER_OPTION_IMAGE_QUALITY;
    info->encoder_options[1] = ENCODER_OPTION_BITMAP_TRANSFORM;
    info->encoder_options[2] = ENCODER_OPTION_LUMINANCE;
    info->encoder_options[3] = ENCODER_OPTION_CHROMINANCE;
    info->encoder_options[4] = ENCODER_OPTION_YCRCB_SUBSAMPLING;
    info->encoder_options[5] = ENCODER_OPTION_SUPPRESS_APP0;
    info->encoder_options[6] = ENCODER_OPTION_END;

    return S_OK;
}
