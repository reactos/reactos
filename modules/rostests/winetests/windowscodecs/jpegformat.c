/*
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

#define COBJMACROS

#include "objbase.h"
#include "wincodec.h"
#include "wine/test.h"

static const char jpeg_adobe_cmyk_1x5[] =
    "\xff\xd8\xff\xe0\x00\x10\x4a\x46\x49\x46\x00\x01\x01\x01\x01\x2c"
    "\x01\x2c\x00\x00\xff\xee\x00\x0e\x41\x64\x6f\x62\x65\x00\x64\x00"
    "\x00\x00\x00\x02\xff\xfe\x00\x13\x43\x72\x65\x61\x74\x65\x64\x20"
    "\x77\x69\x74\x68\x20\x47\x49\x4d\x50\xff\xdb\x00\x43\x00\x01\x01"
    "\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01"
    "\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01"
    "\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01"
    "\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\xff\xdb"
    "\x00\x43\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01"
    "\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01"
    "\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01"
    "\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01\x01"
    "\x01\x01\x01\xff\xc0\x00\x14\x08\x00\x05\x00\x01\x04\x01\x11\x00"
    "\x02\x11\x01\x03\x11\x01\x04\x11\x00\xff\xc4\x00\x15\x00\x01\x01"
    "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x06\x08"
    "\xff\xc4\x00\x14\x10\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
    "\x00\x00\x00\x00\x00\x00\xff\xc4\x00\x14\x01\x01\x00\x00\x00\x00"
    "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x0a\xff\xc4\x00\x14"
    "\x11\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
    "\x00\x00\xff\xda\x00\x0e\x04\x01\x00\x02\x11\x03\x11\x04\x00\x00"
    "\x3f\x00\x40\x44\x02\x1e\xa4\x1f\xff\xd9";

static void test_decode_adobe_cmyk(void)
{
    IWICBitmapDecoder *decoder;
    IWICBitmapFrameDecode *framedecode;
    IWICImagingFactory *factory;
    IWICPalette *palette;
    HRESULT hr;
    HGLOBAL hjpegdata;
    char *jpegdata;
    IStream *jpegstream;
    GUID guidresult;
    UINT count=0, width=0, height=0;
    BYTE imagedata[5 * 4] = {1};
    UINT i;

    const BYTE expected_imagedata[5 * 4] = {
        0x00, 0xb0, 0xfc, 0x6d,
        0x00, 0xb0, 0xfc, 0x6d,
        0x00, 0xb0, 0xfc, 0x6d,
        0x00, 0xb0, 0xfc, 0x6d,
        0x00, 0xb0, 0xfc, 0x6d,
    };

    const BYTE expected_imagedata_24bpp[5 * 4] = {
        0x0d, 0x4b, 0x94, 0x00,
        0x0d, 0x4b, 0x94, 0x00,
        0x0d, 0x4b, 0x94, 0x00,
        0x0d, 0x4b, 0x94, 0x00,
        0x0d, 0x4b, 0x94, 0x00,
    };

    hr = CoCreateInstance(&CLSID_WICJpegDecoder, NULL, CLSCTX_INPROC_SERVER,
        &IID_IWICBitmapDecoder, (void**)&decoder);
    ok(SUCCEEDED(hr), "CoCreateInstance failed, hr=%lx\n", hr);
    if (FAILED(hr)) return;

    hr = CoCreateInstance(&CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER,
        &IID_IWICImagingFactory, (void **)&factory);
    ok(SUCCEEDED(hr), "CoCreateInstance failed, hr=%lx\n", hr);

    hjpegdata = GlobalAlloc(GMEM_MOVEABLE, sizeof(jpeg_adobe_cmyk_1x5));
    ok(hjpegdata != 0, "GlobalAlloc failed\n");
    if (hjpegdata)
    {
        jpegdata = GlobalLock(hjpegdata);
        memcpy(jpegdata, jpeg_adobe_cmyk_1x5, sizeof(jpeg_adobe_cmyk_1x5));
        GlobalUnlock(hjpegdata);

        hr = CreateStreamOnHGlobal(hjpegdata, FALSE, &jpegstream);
        ok(SUCCEEDED(hr), "CreateStreamOnHGlobal failed, hr=%lx\n", hr);
        if (SUCCEEDED(hr))
        {
            hr = IWICBitmapDecoder_Initialize(decoder, jpegstream, WICDecodeMetadataCacheOnLoad);
            ok(hr == S_OK, "Initialize failed, hr=%lx\n", hr);

            hr = IWICBitmapDecoder_GetContainerFormat(decoder, &guidresult);
            ok(SUCCEEDED(hr), "GetContainerFormat failed, hr=%lx\n", hr);
            ok(IsEqualGUID(&guidresult, &GUID_ContainerFormatJpeg), "unexpected container format\n");

            hr = IWICBitmapDecoder_GetFrameCount(decoder, &count);
            ok(SUCCEEDED(hr), "GetFrameCount failed, hr=%lx\n", hr);
            ok(count == 1, "unexpected count %u\n", count);

            hr = IWICBitmapDecoder_GetFrame(decoder, 0, &framedecode);
            ok(SUCCEEDED(hr), "GetFrame failed, hr=%lx\n", hr);
            if (SUCCEEDED(hr))
            {
                hr = IWICBitmapFrameDecode_GetSize(framedecode, &width, &height);
                ok(SUCCEEDED(hr), "GetSize failed, hr=%lx\n", hr);
                ok(width == 1, "expected width=1, got %u\n", width);
                ok(height == 5, "expected height=5, got %u\n", height);

                hr = IWICBitmapFrameDecode_GetPixelFormat(framedecode, &guidresult);
                ok(SUCCEEDED(hr), "GetPixelFormat failed, hr=%lx\n", hr);
                ok(IsEqualGUID(&guidresult, &GUID_WICPixelFormat32bppCMYK) ||
                    broken(IsEqualGUID(&guidresult, &GUID_WICPixelFormat24bppBGR)), /* xp/2003 */
                    "unexpected pixel format: %s\n", wine_dbgstr_guid(&guidresult));

                /* We want to be sure our state tracking will not impact output
                 * data on subsequent calls */
                for(i=2; i>0; --i)
                {
                    hr = IWICBitmapFrameDecode_CopyPixels(framedecode, NULL, 4, sizeof(imagedata), imagedata);
                    ok(SUCCEEDED(hr), "CopyPixels failed, hr=%lx\n", hr);
                    ok(!memcmp(imagedata, expected_imagedata, sizeof(imagedata)) ||
                            broken(!memcmp(imagedata, expected_imagedata_24bpp, sizeof(expected_imagedata))), /* xp/2003 */
                            "unexpected image data\n");
                }

                hr = IWICImagingFactory_CreatePalette(factory, &palette);
                ok(SUCCEEDED(hr), "CreatePalette failed, hr=%lx\n", hr);

                hr = IWICBitmapDecoder_CopyPalette(decoder, palette);
                ok(hr == WINCODEC_ERR_PALETTEUNAVAILABLE, "Unexpected hr %#lx.\n", hr);

                hr = IWICBitmapFrameDecode_CopyPalette(framedecode, palette);
                ok(hr == WINCODEC_ERR_PALETTEUNAVAILABLE, "Unexpected hr %#lx.\n", hr);

                IWICPalette_Release(palette);

                IWICBitmapFrameDecode_Release(framedecode);
            }
            IStream_Release(jpegstream);
        }
        GlobalFree(hjpegdata);
    }

    IWICBitmapDecoder_Release(decoder);
    IWICImagingFactory_Release(factory);
}


START_TEST(jpegformat)
{
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    test_decode_adobe_cmyk();

    CoUninitialize();
}
