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

#include <stdarg.h>
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

static const char gif_global_palette[] = {
/* LSD */'G','I','F','8','7','a',0x01,0x00,0x01,0x00,0xa1,0x02,0x00,
/* palette */0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,
/* GCE */0x21,0xf9,0x04,0x01,0x05,0x00,0x01,0x00, /* index 1 */
/* IMD */0x2c,0x00,0x00,0x00,0x00,0x01,0x00,0x01,0x00,0x00,
0x02,0x02,0x44,0x01,0x00,0x3b
};

/* frame 0, GCE transparent index 1
 * frame 1, GCE transparent index 2
 */
static const char gif_global_palette_2frames[] = {
/* LSD */'G','I','F','8','9','a',0x01,0x00,0x01,0x00,0xa1,0x02,0x00,
/* palette */0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,
/* GCE */0x21,0xf9,0x04,0x01,0x05,0x00,0x01,0x00, /* index 1 */
/* IMD */0x2c,0x00,0x00,0x00,0x00,0x01,0x00,0x01,0x00,0x00,
0x02,0x02,0x44,0x01,0x00,
/* GCE */0x21,0xf9,0x04,0x01,0x05,0x00,0x02,0x00, /* index 2 */
/* IMD */0x2c,0x00,0x00,0x00,0x00,0x01,0x00,0x01,0x00,0x00,
0x02,0x02,0x44,0x01,0x00,0x3b
};

static const char gif_local_palette[] = {
/* LSD */'G','I','F','8','7','a',0x01,0x00,0x01,0x00,0x27,0x02,0x00,
/* GCE */0x21,0xf9,0x04,0x01,0x05,0x00,0x01,0x00, /* index 1 */
/* IMD */0x2c,0x00,0x00,0x00,0x00,0x01,0x00,0x01,0x00,0x81,
/* palette */0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,
0x02,0x02,0x44,0x01,0x00,0x3b
};

static IWICImagingFactory *factory;

static const char *debugstr_guid(const GUID *guid)
{
    static char buf[50];

    if (!guid) return "(null)";
    sprintf(buf, "{%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}",
            guid->Data1, guid->Data2, guid->Data3, guid->Data4[0],
            guid->Data4[1], guid->Data4[2], guid->Data4[3], guid->Data4[4],
            guid->Data4[5], guid->Data4[6], guid->Data4[7]);
    return buf;
}

static IWICBitmapDecoder *create_decoder(const void *image_data, UINT image_size)
{
    HGLOBAL hmem;
    BYTE *data;
    HRESULT hr;
    IWICBitmapDecoder *decoder = NULL;
    IStream *stream;
    GUID format;

    hmem = GlobalAlloc(0, image_size);
    data = GlobalLock(hmem);
    memcpy(data, image_data, image_size);
    GlobalUnlock(hmem);

    hr = CreateStreamOnHGlobal(hmem, TRUE, &stream);
    ok(hr == S_OK, "CreateStreamOnHGlobal error %#x\n", hr);

    hr = IWICImagingFactory_CreateDecoderFromStream(factory, stream, NULL, 0, &decoder);
    ok(hr == S_OK, "CreateDecoderFromStream error %#x\n", hr);

    hr = IWICBitmapDecoder_GetContainerFormat(decoder, &format);
    ok(hr == S_OK, "GetContainerFormat error %#x\n", hr);
    ok(IsEqualGUID(&format, &GUID_ContainerFormatGif),
       "wrong container format %s\n", debugstr_guid(&format));

    IStream_Release(stream);

    return decoder;
}

static void test_global_gif_palette(void)
{
    HRESULT hr;
    IWICBitmapDecoder *decoder;
    IWICBitmapFrameDecode *frame;
    IWICPalette *palette;
    GUID format;
    UINT count, ret;
    WICColor color[256];

    decoder = create_decoder(gif_global_palette, sizeof(gif_global_palette));
    ok(decoder != 0, "Failed to load GIF image data\n");

    hr = IWICImagingFactory_CreatePalette(factory, &palette);
    ok(hr == S_OK, "CreatePalette error %#x\n", hr);

    /* global palette */
    hr = IWICBitmapDecoder_CopyPalette(decoder, palette);
    ok(hr == S_OK, "CopyPalette error %#x\n", hr);

    hr = IWICPalette_GetColorCount(palette, &count);
    ok(hr == S_OK, "GetColorCount error %#x\n", hr);
    ok(count == 4, "expected 4, got %u\n", count);

    hr = IWICPalette_GetColors(palette, count, color, &ret);
    ok(hr == S_OK, "GetColors error %#x\n", hr);
    ok(ret == count, "expected %u, got %u\n", count, ret);
    ok(color[0] == 0xff010203, "expected 0xff010203, got %#x\n", color[0]);
    ok(color[1] == 0x00040506, "expected 0x00040506, got %#x\n", color[1]);
    ok(color[2] == 0xff070809, "expected 0xff070809, got %#x\n", color[2]);
    ok(color[3] == 0xff0a0b0c, "expected 0xff0a0b0c, got %#x\n", color[3]);

    /* frame palette */
    hr = IWICBitmapDecoder_GetFrame(decoder, 0, &frame);
    ok(hr == S_OK, "GetFrame error %#x\n", hr);

    hr = IWICBitmapFrameDecode_GetPixelFormat(frame, &format);
    ok(hr == S_OK, "GetPixelFormat error %#x\n", hr);
    ok(IsEqualGUID(&format, &GUID_WICPixelFormat8bppIndexed),
       "wrong pixel format %s\n", debugstr_guid(&format));

    hr = IWICBitmapFrameDecode_CopyPalette(frame, palette);
    ok(hr == S_OK, "CopyPalette error %#x\n", hr);

    hr = IWICPalette_GetColorCount(palette, &count);
    ok(hr == S_OK, "GetColorCount error %#x\n", hr);
    ok(count == 4, "expected 4, got %u\n", count);

    hr = IWICPalette_GetColors(palette, count, color, &ret);
    ok(hr == S_OK, "GetColors error %#x\n", hr);
    ok(ret == count, "expected %u, got %u\n", count, ret);
    ok(color[0] == 0xff010203, "expected 0xff010203, got %#x\n", color[0]);
    ok(color[1] == 0x00040506, "expected 0x00040506, got %#x\n", color[1]);
    ok(color[2] == 0xff070809, "expected 0xff070809, got %#x\n", color[2]);
    ok(color[3] == 0xff0a0b0c, "expected 0xff0a0b0c, got %#x\n", color[3]);

    IWICPalette_Release(palette);
    IWICBitmapFrameDecode_Release(frame);
    IWICBitmapDecoder_Release(decoder);
}

static void test_global_gif_palette_2frames(void)
{
    HRESULT hr;
    IWICBitmapDecoder *decoder;
    IWICBitmapFrameDecode *frame;
    IWICPalette *palette;
    GUID format;
    UINT count, ret;
    WICColor color[256];

    decoder = create_decoder(gif_global_palette_2frames, sizeof(gif_global_palette_2frames));
    ok(decoder != 0, "Failed to load GIF image data\n");

    /* active frame 0, GCE transparent index 1 */
    hr = IWICBitmapDecoder_GetFrame(decoder, 0, &frame);
    ok(hr == S_OK, "GetFrame error %#x\n", hr);

    hr = IWICImagingFactory_CreatePalette(factory, &palette);
    ok(hr == S_OK, "CreatePalette error %#x\n", hr);

    /* global palette */
    hr = IWICBitmapDecoder_CopyPalette(decoder, palette);
    ok(hr == S_OK, "CopyPalette error %#x\n", hr);

    hr = IWICPalette_GetColorCount(palette, &count);
    ok(hr == S_OK, "GetColorCount error %#x\n", hr);
    ok(count == 4, "expected 4, got %u\n", count);

    hr = IWICPalette_GetColors(palette, count, color, &ret);
    ok(hr == S_OK, "GetColors error %#x\n", hr);
    ok(ret == count, "expected %u, got %u\n", count, ret);
    ok(color[0] == 0xff010203, "expected 0xff010203, got %#x\n", color[0]);
    ok(color[1] == 0x00040506, "expected 0x00040506, got %#x\n", color[1]);
    ok(color[2] == 0xff070809, "expected 0xff070809, got %#x\n", color[2]);
    ok(color[3] == 0xff0a0b0c, "expected 0xff0a0b0c, got %#x\n", color[3]);

    /* frame 0 palette */
    hr = IWICBitmapFrameDecode_GetPixelFormat(frame, &format);
    ok(hr == S_OK, "GetPixelFormat error %#x\n", hr);
    ok(IsEqualGUID(&format, &GUID_WICPixelFormat8bppIndexed),
       "wrong pixel format %s\n", debugstr_guid(&format));

    hr = IWICBitmapFrameDecode_CopyPalette(frame, palette);
    ok(hr == S_OK, "CopyPalette error %#x\n", hr);

    hr = IWICPalette_GetColorCount(palette, &count);
    ok(hr == S_OK, "GetColorCount error %#x\n", hr);
    ok(count == 4, "expected 4, got %u\n", count);

    hr = IWICPalette_GetColors(palette, count, color, &ret);
    ok(hr == S_OK, "GetColors error %#x\n", hr);
    ok(ret == count, "expected %u, got %u\n", count, ret);
    ok(color[0] == 0xff010203, "expected 0xff010203, got %#x\n", color[0]);
    ok(color[1] == 0x00040506, "expected 0x00040506, got %#x\n", color[1]);
    ok(color[2] == 0xff070809, "expected 0xff070809, got %#x\n", color[2]);
    ok(color[3] == 0xff0a0b0c, "expected 0xff0a0b0c, got %#x\n", color[3]);

    IWICBitmapFrameDecode_Release(frame);

    /* active frame 1, GCE transparent index 2 */
    hr = IWICBitmapDecoder_GetFrame(decoder, 1, &frame);
    ok(hr == S_OK, "GetFrame error %#x\n", hr);

    /* global palette */
    hr = IWICBitmapDecoder_CopyPalette(decoder, palette);
    ok(hr == S_OK, "CopyPalette error %#x\n", hr);

    hr = IWICPalette_GetColorCount(palette, &count);
    ok(hr == S_OK, "GetColorCount error %#x\n", hr);
    ok(count == 4, "expected 4, got %u\n", count);

    hr = IWICPalette_GetColors(palette, count, color, &ret);
    ok(hr == S_OK, "GetColors error %#x\n", hr);
    ok(ret == count, "expected %u, got %u\n", count, ret);
    ok(color[0] == 0xff010203, "expected 0xff010203, got %#x\n", color[0]);
    ok(color[1] == 0xff040506 || broken(color[1] == 0x00040506) /* XP */, "expected 0xff040506, got %#x\n", color[1]);
    ok(color[2] == 0x00070809 || broken(color[2] == 0xff070809) /* XP */, "expected 0x00070809, got %#x\n", color[2]);
    ok(color[3] == 0xff0a0b0c, "expected 0xff0a0b0c, got %#x\n", color[3]);

    /* frame 1 palette */
    hr = IWICBitmapFrameDecode_GetPixelFormat(frame, &format);
    ok(hr == S_OK, "GetPixelFormat error %#x\n", hr);
    ok(IsEqualGUID(&format, &GUID_WICPixelFormat8bppIndexed),
       "wrong pixel format %s\n", debugstr_guid(&format));

    hr = IWICBitmapFrameDecode_CopyPalette(frame, palette);
    ok(hr == S_OK, "CopyPalette error %#x\n", hr);

    hr = IWICPalette_GetColorCount(palette, &count);
    ok(hr == S_OK, "GetColorCount error %#x\n", hr);
    ok(count == 4, "expected 4, got %u\n", count);

    hr = IWICPalette_GetColors(palette, count, color, &ret);
    ok(hr == S_OK, "GetColors error %#x\n", hr);
    ok(ret == count, "expected %u, got %u\n", count, ret);
    ok(color[0] == 0xff010203, "expected 0xff010203, got %#x\n", color[0]);
    ok(color[1] == 0xff040506, "expected 0xff040506, got %#x\n", color[1]);
    ok(color[2] == 0x00070809, "expected 0x00070809, got %#x\n", color[2]);
    ok(color[3] == 0xff0a0b0c, "expected 0xff0a0b0c, got %#x\n", color[3]);

    IWICPalette_Release(palette);
    IWICBitmapFrameDecode_Release(frame);
    IWICBitmapDecoder_Release(decoder);
}

static void test_local_gif_palette(void)
{
    HRESULT hr;
    IWICBitmapDecoder *decoder;
    IWICBitmapFrameDecode *frame;
    IWICPalette *palette;
    GUID format;
    UINT count, ret;
    WICColor color[256];

    decoder = create_decoder(gif_local_palette, sizeof(gif_local_palette));
    ok(decoder != 0, "Failed to load GIF image data\n");

    hr = IWICImagingFactory_CreatePalette(factory, &palette);
    ok(hr == S_OK, "CreatePalette error %#x\n", hr);

    /* global palette */
    hr = IWICBitmapDecoder_CopyPalette(decoder, palette);
    ok(hr == WINCODEC_ERR_FRAMEMISSING,
       "expected WINCODEC_ERR_FRAMEMISSING, got %#x\n", hr);

    /* frame palette */
    hr = IWICBitmapDecoder_GetFrame(decoder, 0, &frame);
    ok(hr == S_OK, "GetFrame error %#x\n", hr);

    hr = IWICBitmapFrameDecode_GetPixelFormat(frame, &format);
    ok(hr == S_OK, "GetPixelFormat error %#x\n", hr);
    ok(IsEqualGUID(&format, &GUID_WICPixelFormat8bppIndexed),
       "wrong pixel format %s\n", debugstr_guid(&format));

    hr = IWICBitmapFrameDecode_CopyPalette(frame, palette);
    ok(hr == S_OK, "CopyPalette error %#x\n", hr);

    hr = IWICPalette_GetColorCount(palette, &count);
    ok(hr == S_OK, "GetColorCount error %#x\n", hr);
    ok(count == 4, "expected 4, got %u\n", count);

    hr = IWICPalette_GetColors(palette, count, color, &ret);
    ok(hr == S_OK, "GetColors error %#x\n", hr);
    ok(ret == count, "expected %u, got %u\n", count, ret);
    ok(color[0] == 0xff010203, "expected 0xff010203, got %#x\n", color[0]);
    ok(color[1] == 0x00040506, "expected 0x00040506, got %#x\n", color[1]);
    ok(color[2] == 0xff070809, "expected 0xff070809, got %#x\n", color[2]);
    ok(color[3] == 0xff0a0b0c, "expected 0xff0a0b0c, got %#x\n", color[3]);

    IWICPalette_Release(palette);
    IWICBitmapFrameDecode_Release(frame);
    IWICBitmapDecoder_Release(decoder);
}

START_TEST(gifformat)
{
    HRESULT hr;

    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    hr = CoCreateInstance(&CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER,
                          &IID_IWICImagingFactory, (void **)&factory);
    ok(hr == S_OK, "CoCreateInstance error %#x\n", hr);
    if (FAILED(hr)) return;

    test_global_gif_palette();
    test_global_gif_palette_2frames();
    test_local_gif_palette();

    IWICImagingFactory_Release(factory);
    CoUninitialize();
}
