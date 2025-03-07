/*
 * Copyright 2012,2016 Dmitry Timoshkov
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

#include <math.h>
#include <stdarg.h>
#include <stdio.h>

#define COBJMACROS

#include "windef.h"
#include "wincodec.h"
#include "wine/test.h"

#define EXPECT_REF(obj,ref) _expect_ref((IUnknown*)obj, ref, __LINE__)
static void _expect_ref(IUnknown* obj, ULONG ref, int line)
{
    ULONG rc;
    IUnknown_AddRef(obj);
    rc = IUnknown_Release(obj);
    ok_(__FILE__,line)(rc == ref, "expected refcount %ld, got %ld\n", ref, rc);
}

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

#include "pshpack2.h"
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
    'I' | 'I' << 8,
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
        { 0x11a, IFD_RATIONAL, 1, FIELD_OFFSET(struct tiff_1bpp_data, res) }, /* XRESOLUTION */
        { 0x11b, IFD_RATIONAL, 1, FIELD_OFFSET(struct tiff_1bpp_data, res) }, /* YRESOLUTION */
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
    'I' | 'I' << 8,
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
        { 0x11a, IFD_RATIONAL, 1, FIELD_OFFSET(struct tiff_8bpp_alpha, res) }, /* XRESOLUTION */
        { 0x11b, IFD_RATIONAL, 1, FIELD_OFFSET(struct tiff_8bpp_alpha, res) }, /* YRESOLUTION */
        { 0x11c, IFD_SHORT, 1, 1 }, /* PLANARCONFIGURATION */
        { 0x128, IFD_SHORT, 1, 2 }, /* RESOLUTIONUNIT */
        { 0x152, IFD_SHORT, 1, 1 } /* EXTRASAMPLES: 1 - Associated alpha with pre-multiplied color */
    },
    0,
    { 96, 1 },
    { 0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88 }
};

static const struct tiff_8bpp_data
{
    USHORT byte_order;
    USHORT version;
    ULONG  dir_offset;
    USHORT number_of_entries;
    struct IFD_entry entry[14];
    ULONG next_IFD;
    struct IFD_rational res;
    short palette_data[3][256];
    BYTE pixel_data[4];
} tiff_8bpp_data =
{
    'I' | 'I' << 8,
    42,
    FIELD_OFFSET(struct tiff_8bpp_data, number_of_entries),
    14,
    {
        { 0xff, IFD_SHORT, 1, 0 }, /* SUBFILETYPE */
        { 0x100, IFD_LONG, 1, 4 }, /* IMAGEWIDTH */
        { 0x101, IFD_LONG, 1, 1 }, /* IMAGELENGTH */
        { 0x102, IFD_SHORT, 1, 8 }, /* BITSPERSAMPLE: XP doesn't accept IFD_LONG here */
        { 0x103, IFD_SHORT, 1, 1 }, /* COMPRESSION: XP doesn't accept IFD_LONG here */
        { 0x106, IFD_SHORT, 1, 3 }, /* PHOTOMETRIC */
        { 0x111, IFD_LONG, 1, FIELD_OFFSET(struct tiff_8bpp_data, pixel_data) }, /* STRIPOFFSETS */
        { 0x115, IFD_SHORT, 1, 1 }, /* SAMPLESPERPIXEL */
        { 0x116, IFD_LONG, 1, 1 }, /* ROWSPERSTRIP */
        { 0x117, IFD_LONG, 1, 1 }, /* STRIPBYTECOUNT */
        { 0x11a, IFD_RATIONAL, 1, FIELD_OFFSET(struct tiff_8bpp_data, res) },
        { 0x11b, IFD_RATIONAL, 1, FIELD_OFFSET(struct tiff_8bpp_data, res) },
        { 0x128, IFD_SHORT, 1, 2 }, /* RESOLUTIONUNIT */
        { 0x140, IFD_SHORT, 256*3, FIELD_OFFSET(struct tiff_8bpp_data, palette_data) } /* COLORMAP */
    },
    0,
    { 96, 1 },
    { { 0 } },
    { 0,1,2,3 }
};

static const struct tiff_resolution_test_data
{
    struct IFD_rational resx;
    struct IFD_rational resy;
    LONG resolution_unit;
    double expected_dpi_x;
    double expected_dpi_y;
    /* if != 0: values for different behavior of some Windows versions */
    double broken_dpi_x;
    double broken_dpi_y;
} tiff_resolution_test_data[] =
{
    { { 100, 1 }, {  50, 1 }, 0, 100.0,  50.0,     0,      0  }, /* invalid resolution unit */
    { {  50, 1 }, { 100, 1 }, 0,  50.0, 100.0,     0,      0  },

    { { 100, 1 }, {  50, 1 }, 1, 100.0,  50.0,     0,      0  }, /* RESUNIT_NONE */
    { {  50, 1 }, { 100, 1 }, 1,  50.0, 100.0,     0,      0  },

    { {  49, 1 }, {  49, 1 }, 2,  49.0,  49.0,     0,      0  }, /* same resolution for both X and Y */
    { {  33, 1 }, {  55, 1 }, 2,  33.0,  55.0,     0,      0  }, /* different resolutions for X and Y */
    { {  50, 2 }, {  66, 3 }, 2,  25.0,  22.0,     0,      0  }, /* denominator != 1 */

    { { 100, 1 }, { 200, 1 }, 3, 254.0, 508.0,     0,      0  }, /* unit = centimeters */

    /* XP and Server 2003 do not discard both resolution values if only one of them is invalid */
    { {   0, 1 }, {  29, 1 }, 2,  96.0,  96.0,     0,   29.0  }, /* resolution 0 */
    { {  58, 1 }, {  29, 0 }, 2,  96.0,  96.0,  58.0,      0  }, /* denominator 0 (division by zero) */

    /* XP and Server 2003 return 96 dots per centimeter (= 243.84 dpi) as fallback value */
    { {   0, 1 }, { 100, 1 }, 3,  96.0,  96.0, 243.84, 254.0  }, /* resolution 0 and unit = centimeters */
    { {  50, 1 }, {  72, 0 }, 3,  96.0,  96.0, 127.0,  243.84 }  /* denominator 0 and unit = centimeters */
};

static struct tiff_resolution_image_data
{
    USHORT byte_order;
    USHORT version;
    ULONG dir_offset;
    USHORT number_of_entries;
    struct IFD_entry entry[13];
    ULONG next_IFD;
    struct IFD_rational resx;
    struct IFD_rational resy;
    BYTE pixel_data[4];
} tiff_resolution_image_data =
{
    'I' | 'I' << 8,
    42,
    FIELD_OFFSET(struct tiff_resolution_image_data, number_of_entries),
    13,
    {
        { 0xff, IFD_SHORT, 1, 0 }, /* SUBFILETYPE */
        { 0x100, IFD_LONG, 1, 1 }, /* IMAGEWIDTH */
        { 0x101, IFD_LONG, 1, 1 }, /* IMAGELENGTH */
        { 0x102, IFD_SHORT, 1, 1 }, /* BITSPERSAMPLE */
        { 0x103, IFD_SHORT, 1, 1 }, /* COMPRESSION: XP doesn't accept IFD_LONG here */
        { 0x106, IFD_SHORT, 1, 1 }, /* PHOTOMETRIC */
        { 0x111, IFD_LONG, 1, FIELD_OFFSET(struct tiff_resolution_image_data, pixel_data) }, /* STRIPOFFSETS */
        { 0x115, IFD_SHORT, 1, 1 }, /* SAMPLESPERPIXEL */
        { 0x116, IFD_LONG, 1, 1 }, /* ROWSPERSTRIP */
        { 0x117, IFD_LONG, 1, 1 }, /* STRIPBYTECOUNT */
        { 0x11a, IFD_RATIONAL, 1, FIELD_OFFSET(struct tiff_resolution_image_data, resx) }, /* XRESOLUTION */
        { 0x11b, IFD_RATIONAL, 1, FIELD_OFFSET(struct tiff_resolution_image_data, resy) }, /* YRESOLUTION */
        { 0x128, IFD_SHORT, 1, 1 }, /* RESOLUTIONUNIT -- value will be filled with test data */
    },
    0,
    { 72, 1 }, /* value will be filled with test data */
    { 72, 1 }, /* value will be filled with test data */
    { 0x11, 0x22, 0x33, 0 }
};

static const struct tiff_24bpp_data
{
    USHORT byte_order;
    USHORT version;
    ULONG  dir_offset;
    USHORT number_of_entries;
    struct IFD_entry entry[13];
    ULONG next_IFD;
    struct IFD_rational res;
    BYTE pixel_data[3];
} tiff_24bpp_data =
{
    'I' | 'I' << 8,
    42,
    FIELD_OFFSET(struct tiff_1bpp_data, number_of_entries),
    13,
    {
        { 0xff, IFD_SHORT, 1, 0 }, /* SUBFILETYPE */
        { 0x100, IFD_LONG, 1, 1 }, /* IMAGEWIDTH */
        { 0x101, IFD_LONG, 1, 1 }, /* IMAGELENGTH */
        { 0x102, IFD_SHORT, 1, 8 }, /* BITSPERSAMPLE */
        { 0x103, IFD_SHORT, 1, 1 }, /* COMPRESSION: XP doesn't accept IFD_LONG here */
        { 0x106, IFD_SHORT, 1, 2 }, /* PHOTOMETRIC */
        { 0x111, IFD_LONG, 1, FIELD_OFFSET(struct tiff_24bpp_data, pixel_data) }, /* STRIPOFFSETS */
        { 0x115, IFD_SHORT, 1, 3 }, /* SAMPLESPERPIXEL */
        { 0x116, IFD_LONG, 1, 1 }, /* ROWSPERSTRIP */
        { 0x117, IFD_LONG, 1, 3 }, /* STRIPBYTECOUNT */
        { 0x11a, IFD_RATIONAL, 1, FIELD_OFFSET(struct tiff_24bpp_data, res) }, /* XRESOLUTION */
        { 0x11b, IFD_RATIONAL, 1, FIELD_OFFSET(struct tiff_24bpp_data, res) }, /* YRESOLUTION */
        { 0x128, IFD_SHORT, 1, 2 }, /* RESOLUTIONUNIT */
    },
    0,
    { 900, 3 },
    { 0x11, 0x22, 0x33 }
};

static const struct tiff_4bps_bgra
{
    USHORT byte_order;
    USHORT version;
    ULONG  dir_offset;
    USHORT number_of_entries;
    struct IFD_entry entry[14];
    ULONG next_IFD;
    struct IFD_rational res;
    BYTE pixel_data[4];
} tiff_4bps_bgra =
{
    'I' | 'I' << 8,
    42,
    FIELD_OFFSET(struct tiff_4bps_bgra, number_of_entries),
    14,
    {
        { 0xff, IFD_SHORT, 1, 0 }, /* SUBFILETYPE */
        { 0x100, IFD_LONG, 1, 3 }, /* IMAGEWIDTH */
        { 0x101, IFD_LONG, 1, 2 }, /* IMAGELENGTH */
        { 0x102, IFD_SHORT, 1, 1 }, /* BITSPERSAMPLE */
        { 0x103, IFD_SHORT, 1, 1 }, /* COMPRESSION */
        { 0x106, IFD_SHORT, 1, 2 }, /* PHOTOMETRIC */
        { 0x111, IFD_LONG, 1, FIELD_OFFSET(struct tiff_4bps_bgra, pixel_data) }, /* STRIPOFFSETS */
        { 0x115, IFD_SHORT, 1, 4 }, /* SAMPLESPERPIXEL */
        { 0x116, IFD_LONG, 1, 2 }, /* ROWSPERSTRIP */
        { 0x117, IFD_LONG, 1, 4 }, /* STRIPBYTECOUNT */
        { 0x11a, IFD_RATIONAL, 1, FIELD_OFFSET(struct tiff_4bps_bgra, res) }, /* XRESOLUTION */
        { 0x11b, IFD_RATIONAL, 1, FIELD_OFFSET(struct tiff_4bps_bgra, res) }, /* YRESOLUTION */
        { 0x11c, IFD_SHORT, 1, 1 }, /* PLANARCONFIGURATION */
        { 0x128, IFD_SHORT, 1, 2 } /* RESOLUTIONUNIT */
    },
    0,
    { 96, 1 },
    { 0x12,0x30, 0x47,0xe0 }
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
    ok(hr == S_OK, "CreateStreamOnHGlobal failed, hr=%lx\n", hr);

    return stream;
}

static HRESULT create_decoder(const void *image_data, UINT image_size, IWICBitmapDecoder **decoder)
{
    HGLOBAL hmem;
    BYTE *data;
    HRESULT hr;
    IStream *stream;
    GUID format;
    LONG refcount;

    *decoder = NULL;

    hmem = GlobalAlloc(0, image_size);
    data = GlobalLock(hmem);
    memcpy(data, image_data, image_size);
    GlobalUnlock(hmem);

    hr = CreateStreamOnHGlobal(hmem, TRUE, &stream);
    ok(hr == S_OK, "CreateStreamOnHGlobal error %#lx\n", hr);

    hr = IWICImagingFactory_CreateDecoderFromStream(factory, stream, NULL, 0, decoder);
    if (hr == S_OK)
    {
        hr = IWICBitmapDecoder_GetContainerFormat(*decoder, &format);
        ok(hr == S_OK, "GetContainerFormat error %#lx\n", hr);
        ok(IsEqualGUID(&format, &GUID_ContainerFormatTiff),
           "wrong container format %s\n", wine_dbgstr_guid(&format));

        refcount = IStream_Release(stream);
        ok(refcount > 0, "expected stream refcount > 0\n");
    }

    return hr;
}

static HRESULT get_pixelformat_info(const GUID *format, UINT *bpp, UINT *channels, BOOL *transparency)
{
    HRESULT hr;
    IWICComponentInfo *info;
    IWICPixelFormatInfo2 *formatinfo;

    hr = IWICImagingFactory_CreateComponentInfo(factory, format, &info);
    ok(hr == S_OK, "CreateComponentInfo(%s) error %#lx\n", wine_dbgstr_guid(format), hr);
    if (hr == S_OK)
    {
        hr = IWICComponentInfo_QueryInterface(info, &IID_IWICPixelFormatInfo2, (void **)&formatinfo);
        if (hr == S_OK)
        {
            hr = IWICPixelFormatInfo2_SupportsTransparency(formatinfo, transparency);
            ok(hr == S_OK, "SupportsTransparency error %#lx\n", hr);
            IWICPixelFormatInfo2_Release(formatinfo);
        }
        hr = IWICComponentInfo_QueryInterface(info, &IID_IWICPixelFormatInfo, (void **)&formatinfo);
        if (hr == S_OK)
        {
            hr = IWICPixelFormatInfo2_GetBitsPerPixel(formatinfo, bpp);
            ok(hr == S_OK, "GetBitsPerPixel error %#lx\n", hr);
            hr = IWICPixelFormatInfo2_GetChannelCount(formatinfo, channels);
            ok(hr == S_OK, "GetChannelCount error %#lx\n", hr);
            IWICPixelFormatInfo2_Release(formatinfo);
        }
        IWICComponentInfo_Release(info);
    }
    return hr;
}

static void dump_tiff(void *buf)
{
    UINT count, i;
    struct tiff_1bpp_data *tiff;
    struct IFD_entry *tag;

    tiff = buf;
    count = *(short *)((char *)tiff + tiff->dir_offset);
    tag = (struct IFD_entry *)((char *)tiff + tiff->dir_offset + sizeof(short));

    for (i = 0; i < count; i++)
    {
        printf("tag %u: id %04x, type %04x, count %lu, value %ld",
                i, tag[i].id, tag[i].type, tag[i].count, tag[i].value);
        if (tag[i].id == 0x102 && tag[i].count > 2)
        {
            short *bps = (short *)((char *)tiff + tag[i].value);
            printf(" (%d,%d,%d,%d)\n", bps[0], bps[1], bps[2], bps[3]);
        }
        else
            printf("\n");
    }
}

static void test_tiff_1bpp_palette(void)
{
    HRESULT hr;
    IWICBitmapDecoder *decoder;
    IWICBitmapFrameDecode *frame;
    IWICPalette *palette;
    GUID format;

    hr = create_decoder(&tiff_1bpp_data, sizeof(tiff_1bpp_data), &decoder);
    ok(hr == S_OK, "Failed to load TIFF image data %#lx\n", hr);
    if (hr != S_OK) return;

    hr = IWICBitmapDecoder_GetFrame(decoder, 0, &frame);
    ok(hr == S_OK, "GetFrame error %#lx\n", hr);

    hr = IWICBitmapFrameDecode_GetPixelFormat(frame, &format);
    ok(hr == S_OK, "GetPixelFormat error %#lx\n", hr);
    ok(IsEqualGUID(&format, &GUID_WICPixelFormatBlackWhite),
       "got wrong format %s\n", wine_dbgstr_guid(&format));

    hr = IWICImagingFactory_CreatePalette(factory, &palette);
    ok(hr == S_OK, "CreatePalette error %#lx\n", hr);
    hr = IWICBitmapFrameDecode_CopyPalette(frame, palette);
    ok(hr == WINCODEC_ERR_PALETTEUNAVAILABLE,
       "expected WINCODEC_ERR_PALETTEUNAVAILABLE, got %#lx\n", hr);

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
    UINT frame_count;

    stream = create_stream(&tiff_1bpp_data, sizeof(tiff_1bpp_data));
    if (!stream) return;

    hr = IWICImagingFactory_CreateDecoder(factory, &GUID_ContainerFormatTiff, NULL, &decoder);
    ok(hr == S_OK, "CreateDecoder error %#lx\n", hr);
    if (FAILED(hr)) return;

    frame_count = 0xdeadbeef;
    hr = IWICBitmapDecoder_GetFrameCount(decoder, &frame_count);
    ok(hr == S_OK || broken(hr == E_POINTER) /* XP */, "GetFrameCount error %#lx\n", hr);
    ok(frame_count == 0, "expected 0, got %u\n", frame_count);

    hr = IWICBitmapDecoder_GetFrame(decoder, 0, &frame);
    ok(hr == WINCODEC_ERR_FRAMEMISSING || broken(hr == E_POINTER) /* XP */, "expected WINCODEC_ERR_FRAMEMISSING, got %#lx\n", hr);

    pos.QuadPart = 4;
    hr = IStream_Seek(stream, pos, SEEK_SET, NULL);
    ok(hr == S_OK, "IStream_Seek error %#lx\n", hr);

    capability = 0xdeadbeef;
    hr = IWICBitmapDecoder_QueryCapability(decoder, stream, &capability);
    ok(hr == S_OK, "QueryCapability error %#lx\n", hr);
    ok(capability == exp_caps || capability == exp_caps_xp,
       "expected %#lx, got %#lx\n", exp_caps, capability);

    frame_count = 0xdeadbeef;
    hr = IWICBitmapDecoder_GetFrameCount(decoder, &frame_count);
    ok(hr == S_OK, "GetFrameCount error %#lx\n", hr);
    ok(frame_count == 1, "expected 1, got %u\n", frame_count);

    hr = IWICBitmapDecoder_GetFrame(decoder, 0, &frame);
    ok(hr == S_OK, "GetFrame error %#lx\n", hr);
    IWICBitmapFrameDecode_Release(frame);

    pos.QuadPart = 5;
    hr = IStream_Seek(stream, pos, SEEK_SET, NULL);
    ok(hr == S_OK, "IStream_Seek error %#lx\n", hr);

    hr = IWICBitmapDecoder_QueryCapability(decoder, stream, &capability);
    ok(hr == WINCODEC_ERR_WRONGSTATE, "expected WINCODEC_ERR_WRONGSTATE, got %#lx\n", hr);

    hr = IWICBitmapDecoder_Initialize(decoder, stream, WICDecodeMetadataCacheOnDemand);
    ok(hr == WINCODEC_ERR_WRONGSTATE, "expected WINCODEC_ERR_WRONGSTATE, got %#lx\n", hr);

    IWICBitmapDecoder_Release(decoder);

    /* CreateDecoderFromStream fails if seeked past the start */
    hr = IWICImagingFactory_CreateDecoderFromStream(factory, stream, NULL, 0, &decoder);
    todo_wine
    ok(hr == WINCODEC_ERR_COMPONENTNOTFOUND, "expected WINCODEC_ERR_COMPONENTNOTFOUND, got %#lx\n", hr);

    if (SUCCEEDED(hr))
        IWICBitmapDecoder_Release(decoder);

    pos.QuadPart = 0;
    hr = IStream_Seek(stream, pos, SEEK_SET, NULL);
    ok(hr == S_OK, "IStream_Seek error %#lx\n", hr);

    hr = IWICImagingFactory_CreateDecoderFromStream(factory, stream, NULL, 0, &decoder);
    ok(hr == S_OK, "CreateDecoderFromStream error %#lx\n", hr);

    frame_count = 0xdeadbeef;
    hr = IWICBitmapDecoder_GetFrameCount(decoder, &frame_count);
    ok(hr == S_OK, "GetFrameCount error %#lx\n", hr);
    ok(frame_count == 1, "expected 1, got %u\n", frame_count);

    hr = IWICBitmapDecoder_GetFrame(decoder, 0, &frame);
    ok(hr == S_OK, "GetFrame error %#lx\n", hr);
    IWICBitmapFrameDecode_Release(frame);

    hr = IWICBitmapDecoder_Initialize(decoder, stream, WICDecodeMetadataCacheOnDemand);
    ok(hr == WINCODEC_ERR_WRONGSTATE, "expected WINCODEC_ERR_WRONGSTATE, got %#lx\n", hr);

    hr = IWICBitmapDecoder_QueryCapability(decoder, stream, &capability);
    ok(hr == WINCODEC_ERR_WRONGSTATE, "expected WINCODEC_ERR_WRONGSTATE, got %#lx\n", hr);

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

    hr = create_decoder(&tiff_8bpp_alpha, sizeof(tiff_8bpp_alpha), &decoder);
    ok(hr == S_OK, "Failed to load TIFF image data %#lx\n", hr);
    if (hr != S_OK) return;

    hr = IWICBitmapDecoder_GetFrameCount(decoder, &frame_count);
    ok(hr == S_OK, "GetFrameCount error %#lx\n", hr);
    ok(frame_count == 1, "expected 1, got %u\n", frame_count);

    EXPECT_REF(decoder, 1);
    hr = IWICBitmapDecoder_GetFrame(decoder, 0, &frame);
    ok(hr == S_OK, "GetFrame error %#lx\n", hr);
    EXPECT_REF(decoder, 2);
    IWICBitmapDecoder_Release(decoder);

    hr = IWICBitmapFrameDecode_GetSize(frame, &width, &height);
    ok(hr == S_OK, "GetSize error %#lx\n", hr);
    ok(width == 2, "expected 2, got %u\n", width);
    ok(height == 2, "expected 2, got %u\n", height);

    hr = IWICBitmapFrameDecode_GetResolution(frame, &dpi_x, &dpi_y);
    ok(hr == S_OK, "GetResolution error %#lx\n", hr);
    ok(dpi_x == 96.0, "expected 96.0, got %f\n", dpi_x);
    ok(dpi_y == 96.0, "expected 96.0, got %f\n", dpi_y);

    hr = IWICBitmapFrameDecode_GetPixelFormat(frame, &format);
    ok(hr == S_OK, "GetPixelFormat error %#lx\n", hr);
    ok(IsEqualGUID(&format, &GUID_WICPixelFormat32bppPBGRA),
       "got wrong format %s\n", wine_dbgstr_guid(&format));

    hr = IWICImagingFactory_CreatePalette(factory, &palette);
    ok(hr == S_OK, "CreatePalette error %#lx\n", hr);
    hr = IWICBitmapFrameDecode_CopyPalette(frame, palette);
    ok(hr == WINCODEC_ERR_PALETTEUNAVAILABLE,
       "expected WINCODEC_ERR_PALETTEUNAVAILABLE, got %#lx\n", hr);
    IWICPalette_Release(palette);

    rc.X = 0;
    rc.Y = 0;
    rc.Width = 2;
    rc.Height = 2;
    hr = IWICBitmapFrameDecode_CopyPixels(frame, &rc, 8, sizeof(data), data);
    ok(hr == S_OK, "CopyPixels error %#lx\n", hr);

    for (i = 0; i < sizeof(data); i++)
        ok(data[i] == expected_data[i], "%u: expected %02x, got %02x\n", i, expected_data[i], data[i]);

    IWICBitmapFrameDecode_Release(frame);
}

static void generate_tiff_palette(void *buf, unsigned count)
{
    unsigned short *r, *g, *b;
    unsigned i;

    r = buf;
    g = r + count;
    b = g + count;

    r[0] = 0x11 * 257;
    g[0] = 0x22 * 257;
    b[0] = 0x33 * 257;
    r[1] = 0x44 * 257;
    g[1] = 0x55 * 257;
    b[1] = 0x66 * 257;
    r[2] = 0x77 * 257;
    g[2] = 0x88 * 257;
    b[2] = 0x99 * 257;
    r[3] = 0xa1 * 257;
    g[3] = 0xb5 * 257;
    b[3] = 0xff * 257;

    for (i = 4; i < count; i++)
    {
        r[i] = i * 257;
        g[i] = (i | 0x40) * 257;
        b[i] = (i | 0x80) * 257;
    }
}

static void test_tiff_8bpp_palette(void)
{
    char buf[sizeof(tiff_8bpp_data)];
    HRESULT hr;
    IWICBitmapDecoder *decoder;
    IWICBitmapFrameDecode *frame;
    IWICPalette *palette;
    GUID format;
    UINT count, ret;
    WICColor color[256];

    memcpy(buf, &tiff_8bpp_data, sizeof(tiff_8bpp_data));
    generate_tiff_palette(buf + FIELD_OFFSET(struct tiff_8bpp_data, palette_data), 256);

    hr = create_decoder(buf, sizeof(buf), &decoder);
    ok(hr == S_OK, "Failed to load TIFF image data %#lx\n", hr);
    if (hr != S_OK) return;

    hr = IWICBitmapDecoder_GetFrame(decoder, 0, &frame);
    ok(hr == S_OK, "GetFrame error %#lx\n", hr);

    hr = IWICBitmapFrameDecode_GetPixelFormat(frame, &format);
    ok(hr == S_OK, "GetPixelFormat error %#lx\n", hr);
    ok(IsEqualGUID(&format, &GUID_WICPixelFormat8bppIndexed),
       "expected GUID_WICPixelFormat8bppIndexed, got %s\n", wine_dbgstr_guid(&format));

    hr = IWICImagingFactory_CreatePalette(factory, &palette);
    ok(hr == S_OK, "CreatePalette error %#lx\n", hr);
    hr = IWICBitmapFrameDecode_CopyPalette(frame, palette);
    ok(hr == S_OK, "CopyPalette error %#lx\n", hr);

    hr = IWICPalette_GetColorCount(palette, &count);
    ok(hr == S_OK, "GetColorCount error %#lx\n", hr);
    ok(count == 256, "expected 256, got %u\n", count);

    hr = IWICPalette_GetColors(palette, 256, color, &ret);
    ok(hr == S_OK, "GetColors error %#lx\n", hr);
    ok(ret == count, "expected %u, got %u\n", count, ret);
    ok(color[0] == 0xff112233, "got %#x\n", color[0]);
    ok(color[1] == 0xff445566, "got %#x\n", color[1]);
    ok(color[2] == 0xff778899, "got %#x\n", color[2]);
    ok(color[3] == 0xffa1b5ff, "got %#x\n", color[3]);

    IWICPalette_Release(palette);
    IWICBitmapFrameDecode_Release(frame);
    IWICBitmapDecoder_Release(decoder);
}

static void test_tiff_resolution(void)
{
    HRESULT hr;
    IWICBitmapDecoder *decoder;
    IWICBitmapFrameDecode *frame;
    double dpi_x, dpi_y;
    int i;

    for (i = 0; i < ARRAY_SIZE(tiff_resolution_test_data); i++)
    {
        const struct tiff_resolution_test_data *test_data = &tiff_resolution_test_data[i];
        tiff_resolution_image_data.resx = test_data->resx;
        tiff_resolution_image_data.resy = test_data->resy;
        tiff_resolution_image_data.entry[12].value = test_data->resolution_unit;

        hr = create_decoder(&tiff_resolution_image_data, sizeof(tiff_resolution_image_data), &decoder);
        ok(hr == S_OK, "Failed to load TIFF image data %#lx\n", hr);
        if (hr != S_OK) return;

        hr = IWICBitmapDecoder_GetFrame(decoder, 0, &frame);
        ok(hr == S_OK, "%d: GetFrame error %#lx\n", i, hr);

        hr = IWICBitmapFrameDecode_GetResolution(frame, &dpi_x, &dpi_y);
        ok(hr == S_OK, "%d: GetResolution error %#lx\n", i, hr);

        if (test_data->broken_dpi_x != 0)
        {
            ok(fabs(dpi_x - test_data->expected_dpi_x) < 0.01 || broken(fabs(dpi_x - test_data->broken_dpi_x) < 0.01),
                "%d: x: expected %f or %f, got %f\n", i, test_data->expected_dpi_x, test_data->broken_dpi_x, dpi_x);
        }
        else
        {
            ok(fabs(dpi_x - test_data->expected_dpi_x) < 0.01,
                "%d: x: expected %f, got %f\n", i, test_data->expected_dpi_x, dpi_x);
        }

        if (test_data->broken_dpi_y != 0)
        {
            ok(fabs(dpi_y - test_data->expected_dpi_y) < 0.01 || broken(fabs(dpi_y - test_data->broken_dpi_y) < 0.01),
                "%d: y: expected %f or %f, got %f\n", i, test_data->expected_dpi_y, test_data->broken_dpi_y, dpi_y);
        }
        else
        {
            ok(fabs(dpi_y - test_data->expected_dpi_y) < 0.01,
                "%d: y: expected %f, got %f\n", i, test_data->expected_dpi_y, dpi_y);
        }

        IWICBitmapFrameDecode_Release(frame);
        IWICBitmapDecoder_Release(decoder);
    }
}

static void test_tiff_24bpp(void)
{
    HRESULT hr;
    IWICBitmapDecoder *decoder;
    IWICBitmapFrameDecode *frame;
    UINT count, width, height, i, stride;
    double dpi_x, dpi_y;
    GUID format;
    WICRect rc;
    BYTE data[3];
    static const BYTE expected_data[] = { 0x33,0x22,0x11 };

    hr = create_decoder(&tiff_24bpp_data, sizeof(tiff_24bpp_data), &decoder);
    ok(hr == S_OK, "Failed to load TIFF image data %#lx\n", hr);
    if (hr != S_OK) return;
    ok(decoder != NULL, "Failed to load TIFF image data\n");

    hr = IWICBitmapDecoder_GetFrameCount(decoder, &count);
    ok(hr == S_OK, "GetFrameCount error %#lx\n", hr);
    ok(count == 1, "got %u\n", count);

    hr = IWICBitmapDecoder_GetFrame(decoder, 0, &frame);
    ok(hr == S_OK, "GetFrame error %#lx\n", hr);

    hr = IWICBitmapFrameDecode_GetSize(frame, &width, &height);
    ok(hr == S_OK, "GetSize error %#lx\n", hr);
    ok(width == 1, "got %u\n", width);
    ok(height == 1, "got %u\n", height);

    hr = IWICBitmapFrameDecode_GetResolution(frame, &dpi_x, &dpi_y);
    ok(hr == S_OK, "GetResolution error %#lx\n", hr);
    ok(dpi_x == 300.0, "got %f\n", dpi_x);
    ok(dpi_y == 300.0, "got %f\n", dpi_y);

    hr = IWICBitmapFrameDecode_GetPixelFormat(frame, &format);
    ok(hr == S_OK, "GetPixelFormat error %#lx\n", hr);
    ok(IsEqualGUID(&format, &GUID_WICPixelFormat24bppBGR),
       "got wrong format %s\n", wine_dbgstr_guid(&format));

    for (stride = 0; stride <= 32; stride++)
    {
        memset(data, 0, sizeof(data));
        rc.X = 0;
        rc.Y = 0;
        rc.Width = 1;
        rc.Height = 1;
        hr = IWICBitmapFrameDecode_CopyPixels(frame, &rc, stride, sizeof(data), data);
        if (stride < 3)
            ok(hr == E_INVALIDARG, "CopyPixels(%u) should fail: %#lx\n", stride, hr);
        else
        {
            ok(hr == S_OK, "CopyPixels(%u) error %#lx\n", stride, hr);

            for (i = 0; i < sizeof(data); i++)
                ok(data[i] == expected_data[i], "%u: expected %02x, got %02x\n", i, expected_data[i], data[i]);
        }
    }

    IWICBitmapFrameDecode_Release(frame);
    IWICBitmapDecoder_Release(decoder);
}

#include "pshpack2.h"
static const struct tiff_1x1_data
{
    USHORT byte_order;
    USHORT version;
    ULONG  dir_offset;
    USHORT number_of_entries;
    struct IFD_entry entry[13];
    ULONG next_IFD;
    struct IFD_rational res;
    short palette_data[3][256];
    short bps_data[4];
    BYTE pixel_data[32];
} tiff_1x1_data =
{
    'I' | 'I' << 8,
    42,
    FIELD_OFFSET(struct tiff_1x1_data, number_of_entries),
    13,
    {
        { 0xff, IFD_SHORT, 1, 0 }, /* SUBFILETYPE */
        { 0x100, IFD_LONG, 1, 1 }, /* IMAGEWIDTH */
        { 0x101, IFD_LONG, 1, 1 }, /* IMAGELENGTH */
        { 0x102, IFD_SHORT, 3, FIELD_OFFSET(struct tiff_1x1_data, bps_data) }, /* BITSPERSAMPLE */
        { 0x103, IFD_SHORT, 1, 1 }, /* COMPRESSION: XP doesn't accept IFD_LONG here */
        { 0x106, IFD_SHORT, 1, 2 }, /* PHOTOMETRIC */
        { 0x111, IFD_LONG, 1, FIELD_OFFSET(struct tiff_1x1_data, pixel_data) }, /* STRIPOFFSETS */
        { 0x115, IFD_SHORT, 1, 3 }, /* SAMPLESPERPIXEL */
        { 0x11a, IFD_RATIONAL, 1, FIELD_OFFSET(struct tiff_1x1_data, res) },
        { 0x11b, IFD_RATIONAL, 1, FIELD_OFFSET(struct tiff_1x1_data, res) },
        { 0x128, IFD_SHORT, 1, 2 }, /* RESOLUTIONUNIT */
        { 0x140, IFD_SHORT, 256*3, FIELD_OFFSET(struct tiff_1x1_data, palette_data) }, /* COLORMAP */
        { 0x152, IFD_SHORT, 1, 1 } /* EXTRASAMPLES: 1 - Associated alpha with pre-multiplied color */
    },
    0,
    { 96, 1 },
    { { 0 } },
    { 8,8,8,0 },
    { 1,0,2,3,4,5,6,7,8,9,0,1,2,3,4,5 }
};
#include "poppack.h"

static UINT width_bytes(UINT width, UINT bpp)
{
    return (width * bpp + 7) / 8;
}

static void test_color_formats(void)
{
    struct bitmap_data
    {
        UINT bpp;
        UINT width;
        UINT height;
        const WICPixelFormatGUID *format;
        const BYTE *bits;
    };
    static const BYTE bits_1bpsBGR[] = { 0,255,0,255,0,255,255,255,0,0,0,255,255,0,0,0,255,255,255,255,255,0,0,0,0,255,0,255,0,255 };
    static const struct bitmap_data data_1bpsBGR =
    {
        24, 10, 2, &GUID_WICPixelFormat24bppBGR, bits_1bpsBGR
    };
    static const BYTE bits_4bpsBGR[] = { 204,85,85,136,187,51,0,85,85,85,0,68,0,102,0,136,0,119,0,153,0 };
    static const struct bitmap_data data_4bpsBGR =
    {
        24, 5, 2, &GUID_WICPixelFormat24bppBGR, bits_4bpsBGR
    };
    static const BYTE bits_8bpsBGR[] = { 2,0,1,5,4,3,8,7,6 };
    static const struct bitmap_data data_8bpsBGR =
    {
        24, 3, 1, &GUID_WICPixelFormat24bppBGR, bits_8bpsBGR
    };
    static const BYTE bits_48bppRGB[] = { 1,0,2,3,4,5,6,7,8,9,0,1 };
    static const struct bitmap_data data_48bppRGB =
    {
        48, 2, 1, &GUID_WICPixelFormat48bppRGB, bits_48bppRGB
    };
    static const BYTE bits_1bpsBGRA[] = { 0,255,0,255,0,255,0,255,0,255,255,0,255,0,0,255,255,0,255,255,0,0,255,0,0,255,0,255,0,255,0,255,0,0,0,0,0,255,0,0 };
    static const struct bitmap_data data_1bpsBGRA =
    {
        32, 5, 2, &GUID_WICPixelFormat32bppBGRA, bits_1bpsBGRA
    };
    static const struct bitmap_data data_1bpsPBGRA =
    {
        32, 5, 2, &GUID_WICPixelFormat32bppPBGRA, bits_1bpsBGRA
    };
    static const BYTE bits_4bpsBGRA[] = { 204,85,85,51,85,136,187,85,0,68,0,85,0,102,0,119,0,136,0,153,0,0,0,17,0,34,0,51 };
    static const struct bitmap_data data_4bpsBGRA =
    {
        32, 5, 2, &GUID_WICPixelFormat32bppBGRA, bits_4bpsBGRA
    };
    static const struct bitmap_data data_4bpsPBGRA =
    {
        32, 5, 2, &GUID_WICPixelFormat32bppPBGRA, bits_4bpsBGRA
    };
    static const BYTE bits_8bpsBGRA[] = { 2,0,1,3,6,5,4,7,0,9,8,1,4,3,2,5 };
    static const struct bitmap_data data_8bpsBGRA =
    {
        32, 4, 1, &GUID_WICPixelFormat32bppBGRA, bits_8bpsBGRA
    };
    static const struct bitmap_data data_8bpsPBGRA =
    {
        32, 4, 1, &GUID_WICPixelFormat32bppPBGRA, bits_8bpsBGRA
    };
    static const BYTE bits_64bppRGBA[] = { 1,0,2,3,4,5,6,7,8,9,0,1,2,3,4,5 };
    static const struct bitmap_data data_64bppRGBA =
    {
        64, 2, 1, &GUID_WICPixelFormat64bppRGBA, bits_64bppRGBA
    };
    static const struct bitmap_data data_64bppPRGBA =
    {
        64, 2, 1, &GUID_WICPixelFormat64bppPRGBA, bits_64bppRGBA
    };
    static const BYTE bits_BlackWhite[] = { 85,195,184,85 };
    static const struct bitmap_data data_BlackWhite =
    {
        1, 30, 1, &GUID_WICPixelFormatBlackWhite, bits_BlackWhite
    };
    static const BYTE bits_BlackWhite_xp[] = { 85,195,184,84 };
    static const struct bitmap_data data_BlackWhite_xp =
    {
        1, 30, 1, &GUID_WICPixelFormatBlackWhite, bits_BlackWhite_xp
    };
    static const BYTE bits_4bppGray[] = { 85,195,184,85 };
    static const struct bitmap_data data_4bppGray =
    {
        4, 7, 1, &GUID_WICPixelFormat4bppGray, bits_4bppGray
    };
    static const BYTE bits_4bppGray_xp[] = { 85,195,184,80 };
    static const struct bitmap_data data_4bppGray_xp =
    {
        4, 7, 1, &GUID_WICPixelFormat4bppGray, bits_4bppGray_xp
    };
    static const BYTE bits_8bppGray[] = { 1,0,2,3,4,5,6,7,8,9 };
    static const struct bitmap_data data_8bppGray =
    {
        8, 10, 1, &GUID_WICPixelFormat8bppGray, bits_8bppGray
    };
    static const BYTE bits_16bppGray[] = { 1,0,2,3,4,5 };
    static const struct bitmap_data data_16bppGray =
    {
        16, 3, 1, &GUID_WICPixelFormat16bppGray, bits_16bppGray
    };
    static const BYTE bits_32bppGrayFloat[] = { 1,0,2,3,4,5,6,7,8,9,0,1 };
    static const struct bitmap_data data_32bppGrayFloat =
    {
        32, 3, 1, &GUID_WICPixelFormat32bppGrayFloat, bits_32bppGrayFloat
    };
    static const BYTE bits_96bppRGBFloat[] = { 1,0,2,3,4,5,6,7,8,9,0,1 };
    static const struct bitmap_data data_96bppRGBFloat =
    {
        96, 1, 1, &GUID_WICPixelFormat96bppRGBFloat, bits_96bppRGBFloat
    };
    static const BYTE bits_128bppRGBAFloat[] = { 1,0,2,3,4,5,6,7,8,9,0,1,2,3,4,5 };
    static const struct bitmap_data data_128bppRGBAFloat =
    {
        128, 1, 1, &GUID_WICPixelFormat128bppRGBAFloat, bits_128bppRGBAFloat
    };
    static const struct bitmap_data data_128bppPRGBAFloat =
    {
        128, 1, 1, &GUID_WICPixelFormat128bppPRGBAFloat, bits_128bppRGBAFloat
    };
    static const BYTE bits_1bppIndexed[] = { 85,195,184,85 };
    static const struct bitmap_data data_1bppIndexed =
    {
        1, 32, 1, &GUID_WICPixelFormat1bppIndexed, bits_1bppIndexed
    };
    static const BYTE bits_4bppIndexed[] = { 85,195,184,85 };
    static const struct bitmap_data data_4bppIndexed =
    {
        4, 7, 1, &GUID_WICPixelFormat4bppIndexed, bits_4bppIndexed
    };
    static const BYTE bits_4bppIndexed_xp[] = { 85,195,184,80 };
    static const struct bitmap_data data_4bppIndexed_xp =
    {
        4, 7, 1, &GUID_WICPixelFormat4bppIndexed, bits_4bppIndexed_xp
    };
    static const BYTE bits_8bppIndexed[] = { 1,0,2,3,4,5,6,7,8,9 };
    static const struct bitmap_data data_8bppIndexed =
    {
        8, 3, 1, &GUID_WICPixelFormat8bppIndexed, bits_8bppIndexed
    };
    static const BYTE bits_32bppCMYK[] = { 1,0,2,3,4,5,6,7,8,9,0,1 };
    static const struct bitmap_data data_32bppCMYK =
    {
        32, 3, 1, &GUID_WICPixelFormat32bppCMYK, bits_32bppCMYK
    };
    static const BYTE bits_64bppCMYK[] = { 1,0,2,3,4,5,6,7,8,9,0,1,2,3,4,5 };
    static const struct bitmap_data data_64bppCMYK =
    {
        64, 2, 1, &GUID_WICPixelFormat64bppCMYK, bits_64bppCMYK
    };
    static const struct
    {
        int photometric; /* PhotometricInterpretation */
        int samples; /* SamplesPerPixel */
        int bps; /* BitsPerSample */
        int extra_samples; /* ExtraSamples */
        const struct bitmap_data *data;
        const struct bitmap_data *alt_data;
    } td[] =
    {
        /* 2 - RGB */
        { 2, 3, 1, 0, &data_1bpsBGR },
        { 2, 3, 4, 0, &data_4bpsBGR },
        { 2, 3, 8, 0, &data_8bpsBGR },
        { 2, 3, 16, 0, &data_48bppRGB },
        { 2, 3, 24, 0, NULL },
        { 2, 3, 32, 0, &data_96bppRGBFloat },
        { 2, 4, 1, 0, &data_1bpsBGRA },
        { 2, 4, 1, 1, &data_1bpsPBGRA },
        { 2, 4, 4, 0, &data_4bpsBGRA },
        { 2, 4, 4, 1, &data_4bpsPBGRA },
        { 2, 4, 8, 0, &data_8bpsBGRA },
        { 2, 4, 8, 1, &data_8bpsPBGRA },
        { 2, 4, 16, 0, &data_64bppRGBA },
        { 2, 4, 16, 1, &data_64bppPRGBA },
        { 2, 4, 24, 0, NULL },
        { 2, 4, 32, 0, &data_128bppRGBAFloat },
        { 2, 4, 32, 1, &data_128bppPRGBAFloat },
        /* 1 - BlackIsZero (Bilevel) */
        { 1, 1, 1, 0, &data_BlackWhite, &data_BlackWhite_xp },
        { 1, 1, 4, 0, &data_4bppGray, &data_4bppGray_xp },
        { 1, 1, 8, 0, &data_8bppGray },
        { 1, 1, 16, 0, &data_16bppGray },
        { 1, 1, 24, 0, NULL },
        { 1, 1, 32, 0, &data_32bppGrayFloat },
        /* 3 - Palette Color */
        { 3, 1, 1, 0, &data_1bppIndexed },
        { 3, 1, 4, 0, &data_4bppIndexed, &data_4bppIndexed_xp },
        { 3, 1, 8, 0, &data_8bppIndexed },
#if 0 /* FIXME: for some reason libtiff replaces photometric 3 by 1 for bps > 8 */
        { 3, 1, 16, 0, &data_8bppIndexed },
        { 3, 1, 24, 0, &data_8bppIndexed },
        { 3, 1, 32, 0, &data_8bppIndexed },
#endif
        /* 5 - Separated */
        { 5, 4, 1, 0, NULL },
        { 5, 4, 4, 0, NULL },
        { 5, 4, 8, 0, &data_32bppCMYK },
        { 5, 4, 16, 0, &data_64bppCMYK },
        { 5, 4, 24, 0, NULL },
        { 5, 4, 32, 0, NULL },
    };
    BYTE buf[sizeof(tiff_1x1_data)];
    BYTE pixels[256];
    HRESULT hr;
    IWICBitmapDecoder *decoder;
    IWICBitmapFrameDecode *frame;
    GUID format;
    UINT count, i, bpp, channels, ret;
    BOOL transparency;
    struct IFD_entry *tag, *tag_photo = NULL, *tag_bps = NULL, *tag_samples = NULL, *tag_colormap = NULL;
    struct IFD_entry *tag_width = NULL, *tag_height = NULL, *tag_extra_samples = NULL;
    short *bps;

    memcpy(buf, &tiff_1x1_data, sizeof(tiff_1x1_data));
    generate_tiff_palette(buf + FIELD_OFFSET(struct tiff_1x1_data, palette_data), 256);

    count = *(short *)(buf + tiff_1x1_data.dir_offset);
    tag = (struct IFD_entry *)(buf + tiff_1x1_data.dir_offset + sizeof(short));

    /* verify the TIFF structure */
    for (i = 0; i < count; i++)
    {
        if (tag[i].id == 0x100) /* ImageWidth */
            tag_width = &tag[i];
        else if (tag[i].id == 0x101) /* ImageLength */
            tag_height = &tag[i];
        else if (tag[i].id == 0x102) /* BitsPerSample */
            tag_bps = &tag[i];
        else if (tag[i].id == 0x106) /* PhotometricInterpretation */
            tag_photo = &tag[i];
        else if (tag[i].id == 0x115) /* SamplesPerPixel */
            tag_samples = &tag[i];
        else if (tag[i].id == 0x140) /* ColorMap */
            tag_colormap = &tag[i];
        else if (tag[i].id == 0x152) /* ExtraSamples */
            tag_extra_samples = &tag[i];
    }

    ok(tag_bps && tag_photo && tag_samples && tag_colormap && tag_extra_samples, "tag 0x102,0x106,0x115,0x140 or 0x152 is missing\n");
    if (!tag_bps || !tag_photo || !tag_samples || !tag_colormap || !tag_extra_samples) return;

    ok(tag_bps->type == IFD_SHORT, "tag 0x102 should have type IFD_SHORT\n");
    bps = (short *)(buf + tag_bps->value);
    ok(bps[0] == 8 && bps[1] == 8 && bps[2] == 8 && bps[3] == 0,
       "expected bps 8,8,8,0 got %d,%d,%d,%d\n", bps[0], bps[1], bps[2], bps[3]);

    for (i = 0; i < ARRAY_SIZE(td); i++)
    {
        if (td[i].data)
        {
            bpp = td[i].samples * td[i].bps;
            if (winetest_debug > 1)
                trace("photometric %d, samples %d, extra samples %d, bps %d, bpp %u, width %u => width_bytes %u\n",
                      td[i].photometric, td[i].samples, td[i].extra_samples, td[i].bps, bpp,
                      td[i].data->width, width_bytes(td[i].data->width, bpp));
            tag_width->value = td[i].data->width;
            tag_height->value = td[i].data->height;
        }
        else
        {
            tag_width->value = 1;
            tag_height->value = 1;
        }

        tag_colormap->count = (1 << td[i].bps) * 3;

        if (td[i].bps < 8)
        {
            buf[FIELD_OFFSET(struct tiff_1x1_data, pixel_data)] = 0x55;
            buf[FIELD_OFFSET(struct tiff_1x1_data, pixel_data) + 1] = 0xc3;
            buf[FIELD_OFFSET(struct tiff_1x1_data, pixel_data) + 2] = 0xb8;
            buf[FIELD_OFFSET(struct tiff_1x1_data, pixel_data) + 3] = 0x55;
        }
        else
        {
            buf[FIELD_OFFSET(struct tiff_1x1_data, pixel_data)] = 1;
            buf[FIELD_OFFSET(struct tiff_1x1_data, pixel_data) + 1] = 0;
            buf[FIELD_OFFSET(struct tiff_1x1_data, pixel_data) + 2] = 2;
            buf[FIELD_OFFSET(struct tiff_1x1_data, pixel_data) + 3] = 3;
        }

        tag_photo->value = td[i].photometric;
        tag_bps->count = td[i].samples;
        tag_samples->value = td[i].samples;

        if (td[i].samples == 1)
            tag_bps->value = td[i].bps;
        else if (td[i].samples == 2)
            tag_bps->value = MAKELONG(td[i].bps, td[i].bps);
        else if (td[i].samples == 3)
        {
            tag_bps->value = (BYTE *)bps - buf;
            bps[0] = bps[1] = bps[2] = td[i].bps;
        }
        else if (td[i].samples == 4)
        {
            tag_bps->value = (BYTE *)bps - buf;
            bps[0] = bps[1] = bps[2] = bps[3] = td[i].bps;
        }
        else
        {
            ok(0, "%u: unsupported samples count %d\n", i, td[i].samples);
            continue;
        }

        if (td[i].extra_samples)
        {
            tag_extra_samples->id = 0x152; /* ExtraSamples */
            tag_extra_samples->value = td[i].extra_samples;
        }
        else /* remove ExtraSamples tag */
        {
            tag_extra_samples->id = 0x14e; /* NumberOfInks */
            tag_extra_samples->value = td[i].samples;
        }

        hr = create_decoder(buf, sizeof(buf), &decoder);
        if (!td[i].data)
        {
            ok(hr == WINCODEC_ERR_UNSUPPORTEDPIXELFORMAT || hr == WINCODEC_ERR_COMPONENTNOTFOUND /* win8+ */ || hr == WINCODEC_ERR_BADIMAGE /* XP */,
               "%u: (%d,%d,%d,%d) wrong error %#lx\n", i, td[i].photometric, td[i].samples, td[i].extra_samples, td[i].bps, hr);
            if (hr == S_OK)
            {
                IWICBitmapDecoder_Release(decoder);
                dump_tiff(buf);
            }
            continue;
        }
        else
            ok(hr == S_OK || broken(hr == WINCODEC_ERR_UNSUPPORTEDPIXELFORMAT || hr == WINCODEC_ERR_BADIMAGE) /* XP */,
               "%u: failed to load TIFF image data (%d,%d,%d,%d) %#lx\n",
               i, td[i].photometric, td[i].samples, td[i].extra_samples, td[i].bps, hr);
        if (hr != S_OK) continue;

        hr = IWICBitmapDecoder_GetFrame(decoder, 0, &frame);
        ok(hr == S_OK, "%u: GetFrame error %#lx\n", i, hr);

        hr = IWICBitmapFrameDecode_GetPixelFormat(frame, &format);
        ok(hr == S_OK, "%u: GetPixelFormat error %#lx\n", i, hr);
        if (IsEqualGUID(td[i].data->format, &GUID_WICPixelFormat96bppRGBFloat) && IsEqualGUID(&format, &GUID_WICPixelFormat128bppRGBFloat))
        {
            win_skip("Windows Server 2008 misinterprets 96bppRGBFloat as 128bppRGBFloat, skipping the tests\n");
            IWICBitmapFrameDecode_Release(frame);
            IWICBitmapDecoder_Release(decoder);
            continue;
        }
        ok(IsEqualGUID(&format, td[i].data->format),
           "%u (%d,%d,%d,%d): expected %s, got %s\n",
            i, td[i].photometric, td[i].samples, td[i].extra_samples, td[i].bps,
            wine_dbgstr_guid(td[i].data->format), wine_dbgstr_guid(&format));

        transparency = (td[i].photometric == 2 && td[i].samples == 4); /* for XP */
        hr = get_pixelformat_info(&format, &bpp, &channels, &transparency);
        ok(hr == S_OK, "%u: get_pixelformat_bpp error %#lx\n", i, hr);
        ok(bpp == td[i].data->bpp, "%u: expected %u, got %u\n", i, td[i].data->bpp, bpp);
        ok(channels == td[i].samples, "%u: expected %u, got %u\n", i, td[i].samples, channels);
        ok(transparency == (td[i].photometric == 2 && td[i].samples == 4), "%u: got %u\n", i, transparency);

        memset(pixels, 0, sizeof(pixels));
        hr = IWICBitmapFrameDecode_CopyPixels(frame, NULL, width_bytes(td[i].data->width, bpp), sizeof(pixels), pixels);
        ok(hr == S_OK, "%u: CopyPixels error %#lx\n", i, hr);
        ret = memcmp(pixels, td[i].data->bits, width_bytes(td[i].data->width, bpp));
        if (ret && td[i].alt_data)
            ret = memcmp(pixels, td[i].alt_data->bits, width_bytes(td[i].data->width, bpp));
        ok(ret == 0, "%u: (%d,%d,%d,%d) wrong pixel data\n", i, td[i].photometric, td[i].samples, td[i].extra_samples, td[i].bps);
        if (ret)
        {
            UINT j, n = width_bytes(td[i].data->width, bpp);
            for (j = 0; j < n; j++)
                printf("%u%s", pixels[j], (j + 1) < n ? "," : "\n");
        }

        IWICBitmapFrameDecode_Release(frame);
        IWICBitmapDecoder_Release(decoder);
    }
}

static void test_tiff_4bps_bgra(void)
{
    HRESULT hr;
    IWICBitmapDecoder *decoder;
    IWICBitmapFrameDecode *frame;
    UINT frame_count, width, height, i;
    double dpi_x, dpi_y;
    IWICPalette *palette;
    GUID format;
    WICRect rc;
    BYTE data[24];
    static const BYTE expected_data[24] = { 0,0,0,0xff, 0xff,0,0,0, 0xff,0,0,0xff,
                                            0,0xff,0,0, 0xff,0xff,0,0xff, 0xff,0xff,0xff,0 };

    hr = create_decoder(&tiff_4bps_bgra, sizeof(tiff_4bps_bgra), &decoder);
    ok(hr == S_OK, "Failed to load TIFF image data %#lx\n", hr);
    if (hr != S_OK) return;

    hr = IWICBitmapDecoder_GetFrameCount(decoder, &frame_count);
    ok(hr == S_OK, "GetFrameCount error %#lx\n", hr);
    ok(frame_count == 1, "expected 1, got %u\n", frame_count);

    hr = IWICBitmapDecoder_GetFrame(decoder, 0, &frame);
    ok(hr == S_OK, "GetFrame error %#lx\n", hr);

    hr = IWICBitmapFrameDecode_GetSize(frame, &width, &height);
    ok(hr == S_OK, "GetSize error %#lx\n", hr);
    ok(width == 3, "got %u\n", width);
    ok(height == 2, "got %u\n", height);

    hr = IWICBitmapFrameDecode_GetResolution(frame, &dpi_x, &dpi_y);
    ok(hr == S_OK, "GetResolution error %#lx\n", hr);
    ok(dpi_x == 96.0, "expected 96.0, got %f\n", dpi_x);
    ok(dpi_y == 96.0, "expected 96.0, got %f\n", dpi_y);

    hr = IWICBitmapFrameDecode_GetPixelFormat(frame, &format);
    ok(hr == S_OK, "GetPixelFormat error %#lx\n", hr);
    ok(IsEqualGUID(&format, &GUID_WICPixelFormat32bppBGRA),
       "got wrong format %s\n", wine_dbgstr_guid(&format));

    hr = IWICImagingFactory_CreatePalette(factory, &palette);
    ok(hr == S_OK, "CreatePalette error %#lx\n", hr);
    hr = IWICBitmapFrameDecode_CopyPalette(frame, palette);
    ok(hr == WINCODEC_ERR_PALETTEUNAVAILABLE,
       "expected WINCODEC_ERR_PALETTEUNAVAILABLE, got %#lx\n", hr);
    IWICPalette_Release(palette);

    memset(data, 0xaa, sizeof(data));
    rc.X = 0;
    rc.Y = 0;
    rc.Width = 3;
    rc.Height = 2;
    hr = IWICBitmapFrameDecode_CopyPixels(frame, &rc, 12, sizeof(data), data);
    ok(hr == S_OK, "CopyPixels error %#lx\n", hr);

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
    ok(hr == S_OK, "CoCreateInstance error %#lx\n", hr);
    if (FAILED(hr)) return;

    test_tiff_4bps_bgra();
    test_color_formats();
    test_tiff_1bpp_palette();
    test_tiff_8bpp_palette();
    test_QueryCapability();
    test_tiff_8bpp_alpha();
    test_tiff_resolution();
    test_tiff_24bpp();

    IWICImagingFactory_Release(factory);
    CoUninitialize();
}
