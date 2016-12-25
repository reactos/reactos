/*
 * Copyright 2012 Dmitry Timoshkov
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

//#include <stdarg.h>
#include <stdio.h>

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#define COBJMACROS

#include <windef.h>
#include <winbase.h>
#include <ole2.h>
#include <wincodec.h>
#include <wine/test.h>

#define IFD_BYTE 1
#define IFD_ASCII 2
#define IFD_SHORT 3
#define IFD_LONG 4
#define IFD_RATIONAL 5
#define IFD_SBYTE 6
#define IFD_UNDEFINED 7
#define IFD_SSHORT 8
#define IFD_SLONG 9
#define IFD_SRATIONAL 10
#define IFD_FLOAT 11
#define IFD_DOUBLE 12

#include <pshpack2.h>
struct IFD_entry
{
    SHORT id;
    SHORT type;
    ULONG count;
    LONG  value;
};

struct IFD_rational
{
    LONG numerator;
    LONG denominator;
};

static const struct tiff_1bpp_data
{
    USHORT byte_order;
    USHORT version;
    ULONG  dir_offset;
    USHORT number_of_entries;
    struct IFD_entry entry[13];
    ULONG next_IFD;
    struct IFD_rational res;
    BYTE pixel_data[4];
} tiff_1bpp_data =
{
#ifdef WORDS_BIGENDIAN
    'M' | 'M' << 8,
#else
    'I' | 'I' << 8,
#endif
    42,
    FIELD_OFFSET(struct tiff_1bpp_data, number_of_entries),
    13,
    {
        { 0xff, IFD_SHORT, 1, 0 }, /* SUBFILETYPE */
        { 0x100, IFD_LONG, 1, 1 }, /* IMAGEWIDTH */
        { 0x101, IFD_LONG, 1, 1 }, /* IMAGELENGTH */
        { 0x102, IFD_SHORT, 1, 1 }, /* BITSPERSAMPLE */
        { 0x103, IFD_SHORT, 1, 1 }, /* COMPRESSION: XP doesn't accept IFD_LONG here */
        { 0x106, IFD_SHORT, 1, 1 }, /* PHOTOMETRIC */
        { 0x111, IFD_LONG, 1, FIELD_OFFSET(struct tiff_1bpp_data, pixel_data) }, /* STRIPOFFSETS */
        { 0x115, IFD_SHORT, 1, 1 }, /* SAMPLESPERPIXEL */
        { 0x116, IFD_LONG, 1, 1 }, /* ROWSPERSTRIP */
        { 0x117, IFD_LONG, 1, 1 }, /* STRIPBYTECOUNT */
        { 0x11a, IFD_RATIONAL, 1, FIELD_OFFSET(struct tiff_1bpp_data, res) },
        { 0x11b, IFD_RATIONAL, 1, FIELD_OFFSET(struct tiff_1bpp_data, res) },
        { 0x128, IFD_SHORT, 1, 2 }, /* RESOLUTIONUNIT */
    },
    0,
    { 900, 3 },
    { 0x11, 0x22, 0x33, 0 }
};

static const struct tiff_8bpp_alpha
{
    USHORT byte_order;
    USHORT version;
    ULONG  dir_offset;
    USHORT number_of_entries;
    struct IFD_entry entry[15];
    ULONG next_IFD;
    struct IFD_rational res;
    BYTE pixel_data[8];
} tiff_8bpp_alpha =
{
#ifdef WORDS_BIGENDIAN
    'M' | 'M' << 8,
#else
    'I' | 'I' << 8,
#endif
    42,
    FIELD_OFFSET(struct tiff_8bpp_alpha, number_of_entries),
    15,
    {
        { 0xff, IFD_SHORT, 1, 0 }, /* SUBFILETYPE */
        { 0x100, IFD_LONG, 1, 2 }, /* IMAGEWIDTH */
        { 0x101, IFD_LONG, 1, 2 }, /* IMAGELENGTH */
        { 0x102, IFD_SHORT, 2, MAKELONG(8, 8) }, /* BITSPERSAMPLE */
        { 0x103, IFD_SHORT, 1, 1 }, /* COMPRESSION: XP doesn't accept IFD_LONG here */
        { 0x106, IFD_SHORT, 1, 1 }, /* PHOTOMETRIC */
        { 0x111, IFD_LONG, 1, FIELD_OFFSET(struct tiff_8bpp_alpha, pixel_data) }, /* STRIPOFFSETS */
        { 0x115, IFD_SHORT, 1, 2 }, /* SAMPLESPERPIXEL */
        { 0x116, IFD_LONG, 1, 2 }, /* ROWSPERSTRIP */
        { 0x117, IFD_LONG, 1, 8 }, /* STRIPBYTECOUNT */
        { 0x11a, IFD_RATIONAL, 1, FIELD_OFFSET(struct tiff_8bpp_alpha, res) },
        { 0x11b, IFD_RATIONAL, 1, FIELD_OFFSET(struct tiff_8bpp_alpha, res) },
        { 0x11c, IFD_SHORT, 1, 1 }, /* PLANARCONFIGURATION */
        { 0x128, IFD_SHORT, 1, 2 }, /* RESOLUTIONUNIT */
        { 0x152, IFD_SHORT, 1, 1 } /* EXTRASAMPLES: 1 - Associated alpha with pre-multiplied color */
    },
    0,
    { 96, 1 },
    { 0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88 }
};
#include "poppack.h"

static IWICImagingFactory *factory;

static IStream *create_stream(const void *data, int data_size)
{
    HRESULT hr;
    IStream *stream;
    HGLOBAL hdata;
    void *locked_data;

    hdata = GlobalAlloc(GMEM_MOVEABLE, data_size);
    ok(hdata != 0, "GlobalAlloc failed\n");
    if (!hdata) return NULL;

    locked_data = GlobalLock(hdata);
    memcpy(locked_data, data, data_size);
    GlobalUnlock(hdata);

    hr = CreateStreamOnHGlobal(hdata, TRUE, &stream);
    ok(hr == S_OK, "CreateStreamOnHGlobal failed, hr=%x\n", hr);

    return stream;
}

static IWICBitmapDecoder *create_decoder(const void *image_data, UINT image_size)
{
    HRESULT hr;
    IStream *stream;
    IWICBitmapDecoder *decoder = NULL;
    GUID guid;

    stream = create_stream(image_data, image_size);

    hr = IWICImagingFactory_CreateDecoderFromStream(factory, stream, NULL, 0, &decoder);
    ok(hr == S_OK, "CreateDecoderFromStream error %#x\n", hr);
    if (FAILED(hr)) return NULL;

    hr = IWICBitmapDecoder_GetContainerFormat(decoder, &guid);
    ok(hr == S_OK, "GetContainerFormat error %#x\n", hr);
    ok(IsEqualGUID(&guid, &GUID_ContainerFormatTiff), "container format is not TIFF\n");

    IStream_Release(stream);

    return decoder;
}

static void test_tiff_palette(void)
{
    HRESULT hr;
    IWICBitmapDecoder *decoder;
    IWICBitmapFrameDecode *frame;
    IWICPalette *palette;
    GUID format;

    decoder = create_decoder(&tiff_1bpp_data, sizeof(tiff_1bpp_data));
    ok(decoder != 0, "Failed to load TIFF image data\n");
    if (!decoder) return;

    hr = IWICBitmapDecoder_GetFrame(decoder, 0, &frame);
    ok(hr == S_OK, "GetFrame error %#x\n", hr);

    hr = IWICBitmapFrameDecode_GetPixelFormat(frame, &format);
    ok(hr == S_OK, "GetPixelFormat error %#x\n", hr);
    ok(IsEqualGUID(&format, &GUID_WICPixelFormatBlackWhite),
       "got wrong format %s\n", wine_dbgstr_guid(&format));

    hr = IWICImagingFactory_CreatePalette(factory, &palette);
    ok(hr == S_OK, "CreatePalette error %#x\n", hr);
    hr = IWICBitmapFrameDecode_CopyPalette(frame, palette);
    ok(hr == WINCODEC_ERR_PALETTEUNAVAILABLE,
       "expected WINCODEC_ERR_PALETTEUNAVAILABLE, got %#x\n", hr);

    IWICPalette_Release(palette);
    IWICBitmapFrameDecode_Release(frame);
    IWICBitmapDecoder_Release(decoder);
}

static void test_QueryCapability(void)
{
    HRESULT hr;
    IStream *stream;
    IWICBitmapDecoder *decoder;
    IWICBitmapFrameDecode *frame;
    static const DWORD exp_caps = WICBitmapDecoderCapabilityCanDecodeAllImages |
                                  WICBitmapDecoderCapabilityCanDecodeSomeImages |
                                  WICBitmapDecoderCapabilityCanEnumerateMetadata;
    static const DWORD exp_caps_xp = WICBitmapDecoderCapabilityCanDecodeAllImages |
                                     WICBitmapDecoderCapabilityCanDecodeSomeImages;
    DWORD capability;
    LARGE_INTEGER pos;
    ULARGE_INTEGER cur_pos;
    UINT frame_count;

    stream = create_stream(&tiff_1bpp_data, sizeof(tiff_1bpp_data));
    if (!stream) return;

    hr = IWICImagingFactory_CreateDecoder(factory, &GUID_ContainerFormatTiff, NULL, &decoder);
    ok(hr == S_OK, "CreateDecoder error %#x\n", hr);
    if (FAILED(hr)) return;

    frame_count = 0xdeadbeef;
    hr = IWICBitmapDecoder_GetFrameCount(decoder, &frame_count);
    ok(hr == S_OK || broken(hr == E_POINTER) /* XP */, "GetFrameCount error %#x\n", hr);
    ok(frame_count == 0, "expected 0, got %u\n", frame_count);

    hr = IWICBitmapDecoder_GetFrame(decoder, 0, &frame);
    ok(hr == WINCODEC_ERR_FRAMEMISSING || broken(hr == E_POINTER) /* XP */, "expected WINCODEC_ERR_FRAMEMISSING, got %#x\n", hr);

    pos.QuadPart = 4;
    hr = IStream_Seek(stream, pos, SEEK_SET, NULL);
    ok(hr == S_OK, "IStream_Seek error %#x\n", hr);

    capability = 0xdeadbeef;
    hr = IWICBitmapDecoder_QueryCapability(decoder, stream, &capability);
    ok(hr == S_OK, "QueryCapability error %#x\n", hr);
    ok(capability == exp_caps || capability == exp_caps_xp,
       "expected %#x, got %#x\n", exp_caps, capability);

    frame_count = 0xdeadbeef;
    hr = IWICBitmapDecoder_GetFrameCount(decoder, &frame_count);
    ok(hr == S_OK, "GetFrameCount error %#x\n", hr);
    ok(frame_count == 1, "expected 1, got %u\n", frame_count);

    hr = IWICBitmapDecoder_GetFrame(decoder, 0, &frame);
    ok(hr == S_OK, "GetFrame error %#x\n", hr);
    IWICBitmapFrameDecode_Release(frame);

    pos.QuadPart = 0;
    hr = IStream_Seek(stream, pos, SEEK_CUR, &cur_pos);
    ok(hr == S_OK, "IStream_Seek error %#x\n", hr);
    ok(cur_pos.QuadPart > 4 && cur_pos.QuadPart < sizeof(tiff_1bpp_data),
       "current stream pos is at %x/%x\n", cur_pos.u.LowPart, cur_pos.u.HighPart);

    hr = IWICBitmapDecoder_QueryCapability(decoder, stream, &capability);
    ok(hr == WINCODEC_ERR_WRONGSTATE, "expected WINCODEC_ERR_WRONGSTATE, got %#x\n", hr);

    hr = IWICBitmapDecoder_Initialize(decoder, stream, WICDecodeMetadataCacheOnDemand);
    ok(hr == WINCODEC_ERR_WRONGSTATE, "expected WINCODEC_ERR_WRONGSTATE, got %#x\n", hr);

    IWICBitmapDecoder_Release(decoder);

    hr = IWICImagingFactory_CreateDecoderFromStream(factory, stream, NULL, 0, &decoder);
todo_wine
    ok(hr == WINCODEC_ERR_COMPONENTNOTFOUND, "expected WINCODEC_ERR_COMPONENTNOTFOUND, got %#x\n", hr);

    if (SUCCEEDED(hr))
        IWICBitmapDecoder_Release(decoder);

    pos.QuadPart = 0;
    hr = IStream_Seek(stream, pos, SEEK_SET, NULL);
    ok(hr == S_OK, "IStream_Seek error %#x\n", hr);

    hr = IWICImagingFactory_CreateDecoderFromStream(factory, stream, NULL, 0, &decoder);
    ok(hr == S_OK, "CreateDecoderFromStream error %#x\n", hr);

    frame_count = 0xdeadbeef;
    hr = IWICBitmapDecoder_GetFrameCount(decoder, &frame_count);
    ok(hr == S_OK, "GetFrameCount error %#x\n", hr);
    ok(frame_count == 1, "expected 1, got %u\n", frame_count);

    hr = IWICBitmapDecoder_GetFrame(decoder, 0, &frame);
    ok(hr == S_OK, "GetFrame error %#x\n", hr);
    IWICBitmapFrameDecode_Release(frame);

    hr = IWICBitmapDecoder_Initialize(decoder, stream, WICDecodeMetadataCacheOnDemand);
    ok(hr == WINCODEC_ERR_WRONGSTATE, "expected WINCODEC_ERR_WRONGSTATE, got %#x\n", hr);

    hr = IWICBitmapDecoder_QueryCapability(decoder, stream, &capability);
    ok(hr == WINCODEC_ERR_WRONGSTATE, "expected WINCODEC_ERR_WRONGSTATE, got %#x\n", hr);

    IWICBitmapDecoder_Release(decoder);
    IStream_Release(stream);
}

static void test_tiff_8bpp_alpha(void)
{
    HRESULT hr;
    IWICBitmapDecoder *decoder;
    IWICBitmapFrameDecode *frame;
    UINT frame_count, width, height, i;
    double dpi_x, dpi_y;
    IWICPalette *palette;
    GUID format;
    WICRect rc;
    BYTE data[16];
    static const BYTE expected_data[16] = { 0x11,0x11,0x11,0x22,0x33,0x33,0x33,0x44,
                                            0x55,0x55,0x55,0x66,0x77,0x77,0x77,0x88 };

    decoder = create_decoder(&tiff_8bpp_alpha, sizeof(tiff_8bpp_alpha));
    ok(decoder != 0, "Failed to load TIFF image data\n");
    if (!decoder) return;

    hr = IWICBitmapDecoder_GetFrameCount(decoder, &frame_count);
    ok(hr == S_OK, "GetFrameCount error %#x\n", hr);
    ok(frame_count == 1, "expected 1, got %u\n", frame_count);

    hr = IWICBitmapDecoder_GetFrame(decoder, 0, &frame);
    ok(hr == S_OK, "GetFrame error %#x\n", hr);

    hr = IWICBitmapFrameDecode_GetSize(frame, &width, &height);
    ok(hr == S_OK, "GetSize error %#x\n", hr);
    ok(width == 2, "expected 2, got %u\n", width);
    ok(height == 2, "expected 2, got %u\n", height);

    hr = IWICBitmapFrameDecode_GetResolution(frame, &dpi_x, &dpi_y);
    ok(hr == S_OK, "GetResolution error %#x\n", hr);
    ok(dpi_x == 96.0, "expected 96.0, got %f\n", dpi_x);
    ok(dpi_y == 96.0, "expected 96.0, got %f\n", dpi_y);

    hr = IWICBitmapFrameDecode_GetPixelFormat(frame, &format);
    ok(hr == S_OK, "GetPixelFormat error %#x\n", hr);
    ok(IsEqualGUID(&format, &GUID_WICPixelFormat32bppPBGRA),
       "got wrong format %s\n", wine_dbgstr_guid(&format));

    hr = IWICImagingFactory_CreatePalette(factory, &palette);
    ok(hr == S_OK, "CreatePalette error %#x\n", hr);
    hr = IWICBitmapFrameDecode_CopyPalette(frame, palette);
    ok(hr == WINCODEC_ERR_PALETTEUNAVAILABLE,
       "expected WINCODEC_ERR_PALETTEUNAVAILABLE, got %#x\n", hr);
    IWICPalette_Release(palette);

    rc.X = 0;
    rc.Y = 0;
    rc.Width = 2;
    rc.Height = 2;
    hr = IWICBitmapFrameDecode_CopyPixels(frame, &rc, 8, sizeof(data), data);
    ok(hr == S_OK, "CopyPixels error %#x\n", hr);

    for (i = 0; i < sizeof(data); i++)
        ok(data[i] == expected_data[i], "%u: expected %02x, got %02x\n", i, expected_data[i], data[i]);

    IWICBitmapFrameDecode_Release(frame);
    IWICBitmapDecoder_Release(decoder);
}

START_TEST(tiffformat)
{
    HRESULT hr;

    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    hr = CoCreateInstance(&CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER,
                          &IID_IWICImagingFactory, (void **)&factory);
    ok(hr == S_OK, "CoCreateInstance error %#x\n", hr);
    if (FAILED(hr)) return;

    test_tiff_palette();
    test_QueryCapability();
    test_tiff_8bpp_alpha();

    IWICImagingFactory_Release(factory);
    CoUninitialize();
}
