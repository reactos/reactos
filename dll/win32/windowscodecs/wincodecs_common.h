/*
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

HRESULT CDECL decoder_initialize(struct decoder *decoder, IStream *stream, struct decoder_stat *st)
{
    return decoder->vtable->initialize(decoder, stream, st);
}

HRESULT CDECL decoder_get_frame_info(struct decoder *decoder, UINT frame, struct decoder_frame *info)
{
    return decoder->vtable->get_frame_info(decoder, frame, info);
}

HRESULT CDECL decoder_copy_pixels(struct decoder *decoder, UINT frame,
    const WICRect *prc, UINT stride, UINT buffersize, BYTE *buffer)
{
    return decoder->vtable->copy_pixels(decoder, frame, prc, stride, buffersize, buffer);
}

HRESULT CDECL decoder_get_metadata_blocks(struct decoder *decoder, UINT frame, UINT *count, struct decoder_block **blocks)
{
    return decoder->vtable->get_metadata_blocks(decoder, frame, count, blocks);
}

HRESULT CDECL decoder_get_color_context(struct decoder *decoder, UINT frame,
    UINT num, BYTE **data, DWORD *datasize)
{
    return decoder->vtable->get_color_context(decoder, frame, num, data, datasize);
}

void CDECL decoder_destroy(struct decoder *decoder)
{
    decoder->vtable->destroy(decoder);
}

HRESULT CDECL encoder_initialize(struct encoder *encoder, IStream *stream)
{
    return encoder->vtable->initialize(encoder, stream);
}

HRESULT CDECL encoder_get_supported_format(struct encoder* encoder, GUID *pixel_format, DWORD *bpp, BOOL *indexed)
{
    return encoder->vtable->get_supported_format(encoder, pixel_format, bpp, indexed);
}

HRESULT CDECL encoder_create_frame(struct encoder* encoder, const struct encoder_frame *frame)
{
    return encoder->vtable->create_frame(encoder, frame);
}

HRESULT CDECL encoder_write_lines(struct encoder* encoder, BYTE *data, DWORD line_count, DWORD stride)
{
    return encoder->vtable->write_lines(encoder, data, line_count, stride);
}

HRESULT CDECL encoder_commit_frame(struct encoder* encoder)
{
    return encoder->vtable->commit_frame(encoder);
}

HRESULT CDECL encoder_commit_file(struct encoder* encoder)
{
    return encoder->vtable->commit_file(encoder);
}

void CDECL encoder_destroy(struct encoder *encoder)
{
    encoder->vtable->destroy(encoder);
}

HRESULT copy_pixels(UINT bpp, const BYTE *srcbuffer,
    UINT srcwidth, UINT srcheight, INT srcstride,
    const WICRect *rc, UINT dststride, UINT dstbuffersize, BYTE *dstbuffer)
{
    UINT bytesperrow;
    UINT row_offset; /* number of bits into the source rows where the data starts */
    WICRect rect;

    if (!rc)
    {
        rect.X = 0;
        rect.Y = 0;
        rect.Width = srcwidth;
        rect.Height = srcheight;
        rc = &rect;
    }
    else
    {
        if (rc->X < 0 || rc->Y < 0 || rc->X+rc->Width > srcwidth || rc->Y+rc->Height > srcheight)
            return E_INVALIDARG;
    }

    bytesperrow = ((bpp * rc->Width)+7)/8;

    if (dststride < bytesperrow)
        return E_INVALIDARG;

    if ((dststride * (rc->Height-1)) + bytesperrow > dstbuffersize)
        return E_INVALIDARG;

    /* if the whole bitmap is copied and the buffer format matches then it's a matter of a single memcpy */
    if (rc->X == 0 && rc->Y == 0 && rc->Width == srcwidth && rc->Height == srcheight &&
        srcstride == dststride && srcstride == bytesperrow)
    {
        memcpy(dstbuffer, srcbuffer, srcstride * srcheight);
        return S_OK;
    }

    row_offset = rc->X * bpp;

    if (row_offset % 8 == 0)
    {
        /* everything lines up on a byte boundary */
        INT row;
        const BYTE *src;
        BYTE *dst;

        src = srcbuffer + (row_offset / 8) + srcstride * rc->Y;
        dst = dstbuffer;
        for (row=0; row < rc->Height; row++)
        {
            memcpy(dst, src, bytesperrow);
            src += srcstride;
            dst += dststride;
        }
        return S_OK;
    }
    else
    {
        /* we have to do a weird bitwise copy. eww. */
        FIXME("cannot reliably copy bitmap data if bpp < 8\n");
        return E_FAIL;
    }
}

static inline ULONG read_ulong_be(BYTE* data)
{
    return data[0] << 24 | data[1] << 16 | data[2] << 8 | data[3];
}

HRESULT read_png_chunk(IStream *stream, BYTE *type, BYTE **data, ULONG *data_size)
{
    BYTE header[8];
    HRESULT hr;
    ULONG bytesread;

    hr = stream_read(stream, header, 8, &bytesread);
    if (FAILED(hr) || bytesread < 8)
    {
        if (SUCCEEDED(hr))
            hr = E_FAIL;
        return hr;
    }

    *data_size = read_ulong_be(&header[0]);

    memcpy(type, &header[4], 4);

    if (data)
    {
        *data = malloc(*data_size);
        if (!*data)
            return E_OUTOFMEMORY;

        hr = stream_read(stream, *data, *data_size, &bytesread);

        if (FAILED(hr) || bytesread < *data_size)
        {
            if (SUCCEEDED(hr))
                hr = E_FAIL;
            free(*data);
            *data = NULL;
            return hr;
        }

        /* Windows ignores CRC of the chunk */
    }

    return S_OK;
}

void reverse_bgr8(UINT bytesperpixel, LPBYTE bits, UINT width, UINT height, INT stride)
{
    UINT x, y;
    BYTE *pixel, temp;

    for (y=0; y<height; y++)
    {
        pixel = bits + stride * (INT)y;

        for (x=0; x<width; x++)
        {
            temp = pixel[2];
            pixel[2] = pixel[0];
            pixel[0] = temp;
            pixel += bytesperpixel;
        }
    }
}
