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
#include <math.h>

#define COBJMACROS

#include "windef.h"
#include "objbase.h"
#include "wincodec.h"
#include "wincodecsdk.h"
#include "wine/test.h"

static const char testbmp_24bpp[] = {
    /* BITMAPFILEHEADER */
    66,77, /* "BM" */
    78,0,0,0, /* file size */
    0,0,0,0, /* reserved */
    54,0,0,0, /* offset to bits */
    /* BITMAPINFOHEADER */
    40,0,0,0, /* header size */
    2,0,0,0, /* width */
    3,0,0,0, /* height */
    1,0, /* planes */
    24,0, /* bit count */
    0,0,0,0, /* compression */
    0,0,0,0, /* image size */
    0x74,0x12,0,0, /* X pels per meter => 120 dpi */
    0,0,0,0, /* Y pels per meter */
    0,0,0,0, /* colors used */
    0,0,0,0, /* colors important */
    /* bits */
    0,0,0,     0,255,0,     0,0,
    255,0,0,   255,255,0,   0,0,
    255,0,255, 255,255,255, 0,0
};

static void test_decode_24bpp(void)
{
    IWICBitmapDecoder *decoder, *decoder2;
    IWICBitmapFrameDecode *framedecode;
    IWICMetadataQueryReader *queryreader;
    IWICColorContext *colorcontext;
    IWICBitmapSource *thumbnail;
    HRESULT hr;
    HGLOBAL hbmpdata;
    char *bmpdata;
    IStream *bmpstream;
    DWORD capability=0;
    GUID guidresult;
    UINT count=0, width=0, height=0;
    double dpiX, dpiY;
    BYTE imagedata[36] = {1};
    const BYTE expected_imagedata[36] = {
        255,0,255, 255,255,255,
        255,0,0,   255,255,0,
        0,0,0,     0,255,0};
    WICRect rc;

    hr = CoCreateInstance(&CLSID_WICBmpDecoder, NULL, CLSCTX_INPROC_SERVER,
        &IID_IWICBitmapDecoder, (void**)&decoder);
    ok(SUCCEEDED(hr), "CoCreateInstance failed, hr=%lx\n", hr);
    if (FAILED(hr)) return;

    hbmpdata = GlobalAlloc(GMEM_MOVEABLE, sizeof(testbmp_24bpp));
    ok(hbmpdata != 0, "GlobalAlloc failed\n");
    if (hbmpdata)
    {
        bmpdata = GlobalLock(hbmpdata);
        memcpy(bmpdata, testbmp_24bpp, sizeof(testbmp_24bpp));
        GlobalUnlock(hbmpdata);

        hr = CreateStreamOnHGlobal(hbmpdata, FALSE, &bmpstream);
        ok(SUCCEEDED(hr), "CreateStreamOnHGlobal failed, hr=%lx\n", hr);
        if (SUCCEEDED(hr))
        {
            hr = IWICBitmapDecoder_Initialize(decoder, bmpstream, WICDecodeMetadataCacheOnLoad);
            ok(hr == S_OK || broken(hr == WINCODEC_ERR_BADIMAGE) /* XP */, "Initialize failed, hr=%lx\n", hr);
            if (FAILED(hr))
            {
                win_skip("BMP decoder failed to initialize\n");
                GlobalFree(hbmpdata);
                IWICBitmapDecoder_Release(decoder);
                return;
            }

            hr = IWICBitmapDecoder_GetContainerFormat(decoder, &guidresult);
            ok(SUCCEEDED(hr), "GetContainerFormat failed, hr=%lx\n", hr);
            ok(IsEqualGUID(&guidresult, &GUID_ContainerFormatBmp), "unexpected container format\n");

            hr = IWICBitmapDecoder_GetMetadataQueryReader(decoder, &queryreader);
            ok(hr == WINCODEC_ERR_UNSUPPORTEDOPERATION, "expected WINCODEC_ERR_UNSUPPORTEDOPERATION, got %lx\n", hr);

            hr = IWICBitmapDecoder_GetColorContexts(decoder, 1, &colorcontext, &count);
            ok(hr == WINCODEC_ERR_UNSUPPORTEDOPERATION, "expected WINCODEC_ERR_UNSUPPORTEDOPERATION, got %lx\n", hr);

            hr = IWICBitmapDecoder_GetThumbnail(decoder, &thumbnail);
            ok(hr == WINCODEC_ERR_CODECNOTHUMBNAIL, "expected WINCODEC_ERR_CODECNOTHUMBNAIL, got %lx\n", hr);

            hr = IWICBitmapDecoder_GetPreview(decoder, &thumbnail);
            ok(hr == WINCODEC_ERR_UNSUPPORTEDOPERATION, "expected WINCODEC_ERR_UNSUPPORTEDOPERATION, got %lx\n", hr);

            hr = IWICBitmapDecoder_GetFrameCount(decoder, &count);
            ok(SUCCEEDED(hr), "GetFrameCount failed, hr=%lx\n", hr);
            ok(count == 1, "unexpected count %u\n", count);

            hr = IWICBitmapDecoder_GetFrame(decoder, 1, &framedecode);
            ok(hr == E_INVALIDARG || hr == WINCODEC_ERR_FRAMEMISSING, "GetFrame returned %lx\n", hr);

            hr = IWICBitmapDecoder_GetFrame(decoder, 0, &framedecode);
            ok(SUCCEEDED(hr), "GetFrame failed, hr=%lx\n", hr);
            if (SUCCEEDED(hr))
            {
                IWICImagingFactory *factory;
                IWICPalette *palette;

                hr = IWICBitmapFrameDecode_GetSize(framedecode, &width, &height);
                ok(SUCCEEDED(hr), "GetSize failed, hr=%lx\n", hr);
                ok(width == 2, "expected width=2, got %u\n", width);
                ok(height == 3, "expected height=2, got %u\n", height);

                hr = IWICBitmapFrameDecode_GetResolution(framedecode, &dpiX, &dpiY);
                ok(SUCCEEDED(hr), "GetResolution failed, hr=%lx\n", hr);
                ok(dpiX == 96.0, "expected dpiX=96.0, got %f\n", dpiX);
                ok(dpiY == 96.0, "expected dpiY=96.0, got %f\n", dpiY);

                hr = IWICBitmapFrameDecode_GetPixelFormat(framedecode, &guidresult);
                ok(SUCCEEDED(hr), "GetPixelFormat failed, hr=%lx\n", hr);
                ok(IsEqualGUID(&guidresult, &GUID_WICPixelFormat24bppBGR), "unexpected pixel format\n");

                hr = IWICBitmapFrameDecode_GetMetadataQueryReader(framedecode, &queryreader);
                ok(hr == WINCODEC_ERR_UNSUPPORTEDOPERATION, "expected WINCODEC_ERR_UNSUPPORTEDOPERATION, got %lx\n", hr);

                hr = IWICBitmapFrameDecode_GetColorContexts(framedecode, 1, &colorcontext, &count);
                ok(hr == WINCODEC_ERR_UNSUPPORTEDOPERATION, "expected WINCODEC_ERR_UNSUPPORTEDOPERATION, got %lx\n", hr);

                hr = IWICBitmapFrameDecode_GetThumbnail(framedecode, &thumbnail);
                ok(hr == WINCODEC_ERR_CODECNOTHUMBNAIL, "expected WINCODEC_ERR_CODECNOTHUMBNAIL, got %lx\n", hr);

                hr = CoCreateInstance(&CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER,
                    &IID_IWICImagingFactory, (void**)&factory);
                ok(SUCCEEDED(hr), "CoCreateInstance failed, hr=%lx\n", hr);
                if (SUCCEEDED(hr))
                {
                    hr = IWICImagingFactory_CreatePalette(factory, &palette);
                    ok(SUCCEEDED(hr), "CreatePalette failed, hr=%lx\n", hr);
                    if (SUCCEEDED(hr))
                    {
                        hr = IWICBitmapDecoder_CopyPalette(decoder, palette);
                        ok(hr == WINCODEC_ERR_PALETTEUNAVAILABLE, "expected WINCODEC_ERR_PALETTEUNAVAILABLE, got %lx\n", hr);

                        hr = IWICBitmapFrameDecode_CopyPalette(framedecode, palette);
                        ok(hr == WINCODEC_ERR_PALETTEUNAVAILABLE, "expected WINCODEC_ERR_PALETTEUNAVAILABLE, got %lx\n", hr);

                        IWICPalette_Release(palette);
                    }

                    IWICImagingFactory_Release(factory);
                }

                rc.X = 0;
                rc.Y = 0;
                rc.Width = 3;
                rc.Height = 3;
                hr = IWICBitmapFrameDecode_CopyPixels(framedecode, &rc, 6, sizeof(imagedata), imagedata);
                ok(hr == E_INVALIDARG, "expected E_INVALIDARG, got %lx\n", hr);

                rc.X = -1;
                rc.Y = 0;
                rc.Width = 2;
                rc.Height = 3;
                hr = IWICBitmapFrameDecode_CopyPixels(framedecode, &rc, 6, sizeof(imagedata), imagedata);
                ok(hr == E_INVALIDARG, "expected E_INVALIDARG, got %lx\n", hr);

                rc.X = 0;
                rc.Y = 0;
                rc.Width = 2;
                rc.Height = 3;
                hr = IWICBitmapFrameDecode_CopyPixels(framedecode, &rc, 4, sizeof(imagedata), imagedata);
                ok(hr == E_INVALIDARG, "expected E_INVALIDARG, got %lx\n", hr);

                rc.X = 0;
                rc.Y = 0;
                rc.Width = 2;
                rc.Height = 3;
                hr = IWICBitmapFrameDecode_CopyPixels(framedecode, &rc, 4, 5, imagedata);
                ok(hr == E_INVALIDARG, "expected E_INVALIDARG, got %lx\n", hr);

                rc.X = 0;
                rc.Y = 0;
                rc.Width = 2;
                rc.Height = 3;
                hr = IWICBitmapFrameDecode_CopyPixels(framedecode, &rc, 6, sizeof(imagedata), imagedata);
                ok(SUCCEEDED(hr), "CopyPixels failed, hr=%lx\n", hr);
                ok(!memcmp(imagedata, expected_imagedata, sizeof(imagedata)), "unexpected image data\n");

                hr = IWICBitmapFrameDecode_CopyPixels(framedecode, NULL, 6, sizeof(imagedata), imagedata);
                ok(SUCCEEDED(hr), "CopyPixels(rect=NULL) failed, hr=%lx\n", hr);
                ok(!memcmp(imagedata, expected_imagedata, sizeof(imagedata)), "unexpected image data\n");

                IWICBitmapFrameDecode_Release(framedecode);
            }

            /* cannot initialize twice */
            hr = IWICBitmapDecoder_Initialize(decoder, bmpstream, WICDecodeMetadataCacheOnLoad);
            ok(hr == WINCODEC_ERR_WRONGSTATE, "expected WINCODEC_ERR_WRONGSTATE, hr=%lx\n", hr);

            /* cannot querycapability after initialize */
            hr = IWICBitmapDecoder_QueryCapability(decoder, bmpstream, &capability);
            ok(hr == WINCODEC_ERR_WRONGSTATE, "expected WINCODEC_ERR_WRONGSTATE, hr=%lx\n", hr);

            hr = CoCreateInstance(&CLSID_WICBmpDecoder, NULL, CLSCTX_INPROC_SERVER,
                &IID_IWICBitmapDecoder, (void**)&decoder2);
            ok(SUCCEEDED(hr), "CoCreateInstance failed, hr=%lx\n", hr);
            if (SUCCEEDED(hr))
            {
                hr = IWICBitmapDecoder_QueryCapability(decoder2, bmpstream, &capability);
                ok(hr == S_OK, "QueryCapability failed, hr=%lx\n", hr);
                ok(capability == (WICBitmapDecoderCapabilityCanDecodeAllImages),
                    "unexpected capabilities: %lx\n", capability);

                /* cannot initialize after querycapability */
                hr = IWICBitmapDecoder_Initialize(decoder2, bmpstream, WICDecodeMetadataCacheOnLoad);
                ok(hr == WINCODEC_ERR_WRONGSTATE, "expected WINCODEC_ERR_WRONGSTATE, hr=%lx\n", hr);

                /* cannot querycapability twice */
                hr = IWICBitmapDecoder_QueryCapability(decoder2, bmpstream, &capability);
                ok(hr == WINCODEC_ERR_WRONGSTATE, "expected WINCODEC_ERR_WRONGSTATE, hr=%lx\n", hr);

                IWICBitmapDecoder_Release(decoder2);
            }

            IStream_Release(bmpstream);
        }

        GlobalFree(hbmpdata);
    }

    IWICBitmapDecoder_Release(decoder);
}

static const char testbmp_1bpp[] = {
    /* BITMAPFILEHEADER */
    66,77, /* "BM" */
    40,0,0,0, /* file size */
    0,0,0,0, /* reserved */
    32,0,0,0, /* offset to bits */
    /* BITMAPCOREHEADER */
    12,0,0,0, /* header size */
    2,0, /* width */
    2,0, /* height */
    1,0, /* planes */
    1,0, /* bit count */
    /* color table */
    255,0,0,
    0,255,0,
    /* bits */
    0xc0,0,0,0,
    0x80,0,0,0
};

static void test_decode_1bpp(void)
{
    IWICBitmapDecoder *decoder, *decoder2;
    IWICBitmapFrameDecode *framedecode;
    HRESULT hr;
    HGLOBAL hbmpdata;
    char *bmpdata;
    IStream *bmpstream;
    DWORD capability=0;
    GUID guidresult;
    UINT count=0, width=0, height=0;
    double dpiX, dpiY;
    BYTE imagedata[2] = {1};
    const BYTE expected_imagedata[2] = {0x80,0xc0};
    WICColor palettedata[2] = {1};
    const WICColor expected_palettedata[2] = {0xff0000ff,0xff00ff00};
    WICRect rc;

    hr = CoCreateInstance(&CLSID_WICBmpDecoder, NULL, CLSCTX_INPROC_SERVER,
        &IID_IWICBitmapDecoder, (void**)&decoder);
    ok(SUCCEEDED(hr), "CoCreateInstance failed, hr=%lx\n", hr);
    if (FAILED(hr)) return;

    hbmpdata = GlobalAlloc(GMEM_MOVEABLE, sizeof(testbmp_1bpp));
    ok(hbmpdata != 0, "GlobalAlloc failed\n");
    if (hbmpdata)
    {
        bmpdata = GlobalLock(hbmpdata);
        memcpy(bmpdata, testbmp_1bpp, sizeof(testbmp_1bpp));
        GlobalUnlock(hbmpdata);

        hr = CreateStreamOnHGlobal(hbmpdata, FALSE, &bmpstream);
        ok(SUCCEEDED(hr), "CreateStreamOnHGlobal failed, hr=%lx\n", hr);
        if (SUCCEEDED(hr))
        {
            hr = IWICBitmapDecoder_Initialize(decoder, bmpstream, WICDecodeMetadataCacheOnLoad);
            ok(hr == S_OK, "Initialize failed, hr=%lx\n", hr);

            hr = IWICBitmapDecoder_GetContainerFormat(decoder, &guidresult);
            ok(SUCCEEDED(hr), "GetContainerFormat failed, hr=%lx\n", hr);
            ok(IsEqualGUID(&guidresult, &GUID_ContainerFormatBmp), "unexpected container format\n");

            hr = IWICBitmapDecoder_GetFrameCount(decoder, &count);
            ok(SUCCEEDED(hr), "GetFrameCount failed, hr=%lx\n", hr);
            ok(count == 1, "unexpected count %u\n", count);

            hr = IWICBitmapDecoder_GetFrame(decoder, 0, &framedecode);
            ok(SUCCEEDED(hr), "GetFrame failed, hr=%lx\n", hr);
            if (SUCCEEDED(hr))
            {
                IWICImagingFactory *factory;
                IWICPalette *palette;

                hr = IWICBitmapFrameDecode_GetSize(framedecode, &width, &height);
                ok(SUCCEEDED(hr), "GetSize failed, hr=%lx\n", hr);
                ok(width == 2, "expected width=2, got %u\n", width);
                ok(height == 2, "expected height=2, got %u\n", height);

                hr = IWICBitmapFrameDecode_GetResolution(framedecode, &dpiX, &dpiY);
                ok(SUCCEEDED(hr), "GetResolution failed, hr=%lx\n", hr);
                ok(dpiX == 96.0, "expected dpiX=96.0, got %f\n", dpiX);
                ok(dpiY == 96.0, "expected dpiY=96.0, got %f\n", dpiY);

                hr = IWICBitmapFrameDecode_GetPixelFormat(framedecode, &guidresult);
                ok(SUCCEEDED(hr), "GetPixelFormat failed, hr=%lx\n", hr);
                ok(IsEqualGUID(&guidresult, &GUID_WICPixelFormat1bppIndexed), "unexpected pixel format\n");

                hr = CoCreateInstance(&CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER,
                    &IID_IWICImagingFactory, (void**)&factory);
                ok(SUCCEEDED(hr), "CoCreateInstance failed, hr=%lx\n", hr);
                if (SUCCEEDED(hr))
                {
                    hr = IWICImagingFactory_CreatePalette(factory, &palette);
                    ok(SUCCEEDED(hr), "CreatePalette failed, hr=%lx\n", hr);
                    if (SUCCEEDED(hr))
                    {
                        hr = IWICBitmapDecoder_CopyPalette(decoder, palette);
                        ok(hr == WINCODEC_ERR_PALETTEUNAVAILABLE, "expected WINCODEC_ERR_PALETTEUNAVAILABLE, got %lx\n", hr);

                        hr = IWICBitmapFrameDecode_CopyPalette(framedecode, palette);
                        ok(SUCCEEDED(hr), "CopyPalette failed, hr=%lx\n", hr);

                        hr = IWICPalette_GetColorCount(palette, &count);
                        ok(SUCCEEDED(hr), "GetColorCount failed, hr=%lx\n", hr);
                        ok(count == 2, "expected count=2, got %u\n", count);

                        hr = IWICPalette_GetColors(palette, 2, palettedata, &count);
                        ok(SUCCEEDED(hr), "GetColorCount failed, hr=%lx\n", hr);
                        ok(count == 2, "expected count=2, got %u\n", count);
                        ok(!memcmp(palettedata, expected_palettedata, sizeof(palettedata)), "unexpected palette data\n");

                        IWICPalette_Release(palette);
                    }

                    IWICImagingFactory_Release(factory);
                }

                rc.X = 0;
                rc.Y = 0;
                rc.Width = 2;
                rc.Height = 2;
                hr = IWICBitmapFrameDecode_CopyPixels(framedecode, &rc, 1, sizeof(imagedata), imagedata);
                ok(SUCCEEDED(hr), "CopyPixels failed, hr=%lx\n", hr);
                ok(!memcmp(imagedata, expected_imagedata, sizeof(imagedata)), "unexpected image data\n");

                IWICBitmapFrameDecode_Release(framedecode);
            }

            hr = CoCreateInstance(&CLSID_WICBmpDecoder, NULL, CLSCTX_INPROC_SERVER,
                &IID_IWICBitmapDecoder, (void**)&decoder2);
            ok(SUCCEEDED(hr), "CoCreateInstance failed, hr=%lx\n", hr);
            if (SUCCEEDED(hr))
            {
                hr = IWICBitmapDecoder_QueryCapability(decoder2, bmpstream, &capability);
                ok(hr == S_OK, "QueryCapability failed, hr=%lx\n", hr);
                ok(capability == (WICBitmapDecoderCapabilityCanDecodeAllImages),
                    "unexpected capabilities: %lx\n", capability);
                IWICBitmapDecoder_Release(decoder2);
            }

            IStream_Release(bmpstream);
        }

        GlobalFree(hbmpdata);
    }

    IWICBitmapDecoder_Release(decoder);
}

static const char testbmp_4bpp[] = {
    /* BITMAPFILEHEADER */
    66,77, /* "BM" */
    82,0,0,0, /* file size */
    0,0,0,0, /* reserved */
    74,0,0,0, /* offset to bits */
    /* BITMAPINFOHEADER */
    40,0,0,0, /* header size */
    2,0,0,0, /* width */
    254,255,255,255, /* height = -2 */
    1,0, /* planes */
    4,0, /* bit count */
    0,0,0,0, /* compression = BI_RGB */
    0,0,0,0, /* image size = 0 */
    16,39,0,0, /* X pixels per meter = 10000 */
    32,78,0,0, /* Y pixels per meter = 20000 */
    5,0,0,0, /* colors used */
    5,0,0,0, /* colors important */
    /* color table */
    255,0,0,0,
    0,255,0,255,
    0,0,255,23,
    128,0,128,1,
    255,255,255,0,
    /* bits */
    0x01,0,0,0,
    0x23,0,0,0,
};

static void test_decode_4bpp(void)
{
    IWICBitmapDecoder *decoder, *decoder2;
    IWICBitmapFrameDecode *framedecode;
    HRESULT hr;
    HGLOBAL hbmpdata;
    char *bmpdata;
    IStream *bmpstream;
    DWORD capability=0;
    GUID guidresult;
    UINT count=0, width=0, height=0;
    double dpiX, dpiY;
    BYTE imagedata[2] = {1};
    const BYTE expected_imagedata[2] = {0x01,0x23};
    WICColor palettedata[5] = {1};
    const WICColor expected_palettedata[5] =
        {0xff0000ff,0xff00ff00,0xffff0000,0xff800080,0xffffffff};
    WICRect rc;

    hr = CoCreateInstance(&CLSID_WICBmpDecoder, NULL, CLSCTX_INPROC_SERVER,
        &IID_IWICBitmapDecoder, (void**)&decoder);
    ok(SUCCEEDED(hr), "CoCreateInstance failed, hr=%lx\n", hr);
    if (FAILED(hr)) return;

    hbmpdata = GlobalAlloc(GMEM_MOVEABLE, sizeof(testbmp_4bpp));
    ok(hbmpdata != 0, "GlobalAlloc failed\n");
    if (hbmpdata)
    {
        bmpdata = GlobalLock(hbmpdata);
        memcpy(bmpdata, testbmp_4bpp, sizeof(testbmp_4bpp));
        GlobalUnlock(hbmpdata);

        hr = CreateStreamOnHGlobal(hbmpdata, FALSE, &bmpstream);
        ok(SUCCEEDED(hr), "CreateStreamOnHGlobal failed, hr=%lx\n", hr);
        if (SUCCEEDED(hr))
        {
            hr = IWICBitmapDecoder_Initialize(decoder, bmpstream, WICDecodeMetadataCacheOnLoad);
            ok(hr == S_OK, "Initialize failed, hr=%lx\n", hr);

            hr = IWICBitmapDecoder_GetContainerFormat(decoder, &guidresult);
            ok(SUCCEEDED(hr), "GetContainerFormat failed, hr=%lx\n", hr);
            ok(IsEqualGUID(&guidresult, &GUID_ContainerFormatBmp), "unexpected container format\n");

            hr = IWICBitmapDecoder_GetFrameCount(decoder, &count);
            ok(SUCCEEDED(hr), "GetFrameCount failed, hr=%lx\n", hr);
            ok(count == 1, "unexpected count %u\n", count);

            hr = IWICBitmapDecoder_GetFrame(decoder, 0, &framedecode);
            ok(SUCCEEDED(hr), "GetFrame failed, hr=%lx\n", hr);
            if (SUCCEEDED(hr))
            {
                IWICImagingFactory *factory;
                IWICPalette *palette;

                hr = IWICBitmapFrameDecode_GetSize(framedecode, &width, &height);
                ok(SUCCEEDED(hr), "GetSize failed, hr=%lx\n", hr);
                ok(width == 2, "expected width=2, got %u\n", width);
                ok(height == 2, "expected height=2, got %u\n", height);

                hr = IWICBitmapFrameDecode_GetResolution(framedecode, &dpiX, &dpiY);
                ok(SUCCEEDED(hr), "GetResolution failed, hr=%lx\n", hr);
                ok(fabs(dpiX - 254.0) < 0.01, "expected dpiX=96.0, got %f\n", dpiX);
                ok(fabs(dpiY - 508.0) < 0.01, "expected dpiY=96.0, got %f\n", dpiY);

                hr = IWICBitmapFrameDecode_GetPixelFormat(framedecode, &guidresult);
                ok(SUCCEEDED(hr), "GetPixelFormat failed, hr=%lx\n", hr);
                ok(IsEqualGUID(&guidresult, &GUID_WICPixelFormat4bppIndexed), "unexpected pixel format\n");

                hr = CoCreateInstance(&CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER,
                    &IID_IWICImagingFactory, (void**)&factory);
                ok(SUCCEEDED(hr), "CoCreateInstance failed, hr=%lx\n", hr);
                if (SUCCEEDED(hr))
                {
                    hr = IWICImagingFactory_CreatePalette(factory, &palette);
                    ok(SUCCEEDED(hr), "CreatePalette failed, hr=%lx\n", hr);
                    if (SUCCEEDED(hr))
                    {
                        hr = IWICBitmapDecoder_CopyPalette(decoder, palette);
                        ok(hr == WINCODEC_ERR_PALETTEUNAVAILABLE, "expected WINCODEC_ERR_PALETTEUNAVAILABLE, got %lx\n", hr);

                        hr = IWICBitmapFrameDecode_CopyPalette(framedecode, palette);
                        ok(SUCCEEDED(hr), "CopyPalette failed, hr=%lx\n", hr);

                        hr = IWICPalette_GetColorCount(palette, &count);
                        ok(SUCCEEDED(hr), "GetColorCount failed, hr=%lx\n", hr);
                        ok(count == 5, "expected count=5, got %u\n", count);

                        hr = IWICPalette_GetColors(palette, 5, palettedata, &count);
                        ok(SUCCEEDED(hr), "GetColorCount failed, hr=%lx\n", hr);
                        ok(count == 5, "expected count=5, got %u\n", count);
                        ok(!memcmp(palettedata, expected_palettedata, sizeof(palettedata)), "unexpected palette data\n");

                        IWICPalette_Release(palette);
                    }

                    IWICImagingFactory_Release(factory);
                }

                rc.X = 0;
                rc.Y = 0;
                rc.Width = 2;
                rc.Height = 2;
                hr = IWICBitmapFrameDecode_CopyPixels(framedecode, &rc, 1, sizeof(imagedata), imagedata);
                ok(SUCCEEDED(hr), "CopyPixels failed, hr=%lx\n", hr);
                ok(!memcmp(imagedata, expected_imagedata, sizeof(imagedata)), "unexpected image data\n");

                IWICBitmapFrameDecode_Release(framedecode);
            }

            hr = CoCreateInstance(&CLSID_WICBmpDecoder, NULL, CLSCTX_INPROC_SERVER,
                &IID_IWICBitmapDecoder, (void**)&decoder2);
            ok(SUCCEEDED(hr), "CoCreateInstance failed, hr=%lx\n", hr);
            if (SUCCEEDED(hr))
            {
                hr = IWICBitmapDecoder_QueryCapability(decoder2, bmpstream, &capability);
                ok(hr == S_OK, "QueryCapability failed, hr=%lx\n", hr);
                ok(capability == (WICBitmapDecoderCapabilityCanDecodeAllImages),
                    "unexpected capabilities: %lx\n", capability);
                IWICBitmapDecoder_Release(decoder2);
            }

            IStream_Release(bmpstream);
        }

        GlobalFree(hbmpdata);
    }

    IWICBitmapDecoder_Release(decoder);
}

static const char testbmp_rle8[] = {
    /* BITMAPFILEHEADER */
    66,77, /* "BM" */
    202,0,0,0, /* file size */
    0,0,0,0, /* reserved */
    122,0,0,0, /* offset to bits */
    /* BITMAPINFOHEADER */
    40,0,0,0, /* header size */
    8,0,0,0, /* width */
    8,0,0,0, /* height */
    1,0, /* planes */
    8,0, /* bit count */
    1,0,0,0, /* compression = BI_RLE8 */
    80,0,0,0, /* image size */
    19,11,0,0, /* X pixels per meter */
    19,11,0,0, /* Y pixels per meter */
    17,0,0,0, /* colors used */
    17,0,0,0, /* colors important */
    /* color table */
    0,0,0,0,
    17,17,17,0,
    255,0,0,0,
    34,34,34,0,
    0,0,204,0,
    0,0,221,0,
    0,0,238,0,
    51,51,51,0,
    0,0,255,0,
    68,68,68,0,
    255,0,255,0,
    85,85,85,0,
    0,204,0,0,
    0,221,0,0,
    0,238,0,0,
    0,255,0,0,
    255,255,255,0,
    /* bits */
    4,15,0,4,11,9,9,0,0,0,4,14,0,4,3,10,10,7,0,0,4,13,0,4,3,10,10,7,0,0,4,12,0,4,0,1,1,11,0,0,0,4,16,2,16,2,4,4,0,0,0,4,2,16,2,16,4,5,0,0,0,4,16,2,16,2,4,6,0,0,0,4,2,16,2,16,4,8,0,1
};

static void test_decode_rle8(void)
{
    IWICBitmapDecoder *decoder, *decoder2;
    IWICBitmapFrameDecode *framedecode;
    HRESULT hr;
    HGLOBAL hbmpdata;
    char *bmpdata;
    IStream *bmpstream;
    DWORD capability=0;
    GUID guidresult;
    UINT count=0, width=0, height=0;
    double dpiX, dpiY;
    DWORD imagedata[64] = {1};
    const DWORD expected_imagedata[64] = {
        0x0000ff,0xffffff,0x0000ff,0xffffff,0xff0000,0xff0000,0xff0000,0xff0000,
        0xffffff,0x0000ff,0xffffff,0x0000ff,0xee0000,0xee0000,0xee0000,0xee0000,
        0x0000ff,0xffffff,0x0000ff,0xffffff,0xdd0000,0xdd0000,0xdd0000,0xdd0000,
        0xffffff,0x0000ff,0xffffff,0x0000ff,0xcc0000,0xcc0000,0xcc0000,0xcc0000,
        0x00cc00,0x00cc00,0x00cc00,0x00cc00,0x000000,0x111111,0x111111,0x555555,
        0x00dd00,0x00dd00,0x00dd00,0x00dd00,0x222222,0xff00ff,0xff00ff,0x333333,
        0x00ee00,0x00ee00,0x00ee00,0x00ee00,0x222222,0xff00ff,0xff00ff,0x333333,
        0x00ff00,0x00ff00,0x00ff00,0x00ff00,0x555555,0x444444,0x444444,0x000000};
    WICColor palettedata[17] = {1};
    const WICColor expected_palettedata[17] = {
        0xff000000,0xff111111,0xff0000ff,0xff222222,0xffcc0000,0xffdd0000,
        0xffee0000,0xff333333,0xffff0000,0xff444444,0xffff00ff,0xff555555,
        0xff00cc00,0xff00dd00,0xff00ee00,0xff00ff00,0xffffffff};
    WICRect rc;

    hr = CoCreateInstance(&CLSID_WICBmpDecoder, NULL, CLSCTX_INPROC_SERVER,
        &IID_IWICBitmapDecoder, (void**)&decoder);
    ok(SUCCEEDED(hr), "CoCreateInstance failed, hr=%lx\n", hr);
    if (FAILED(hr)) return;

    hbmpdata = GlobalAlloc(GMEM_MOVEABLE, sizeof(testbmp_rle8));
    ok(hbmpdata != 0, "GlobalAlloc failed\n");
    if (hbmpdata)
    {
        bmpdata = GlobalLock(hbmpdata);
        memcpy(bmpdata, testbmp_rle8, sizeof(testbmp_rle8));
        GlobalUnlock(hbmpdata);

        hr = CreateStreamOnHGlobal(hbmpdata, FALSE, &bmpstream);
        ok(SUCCEEDED(hr), "CreateStreamOnHGlobal failed, hr=%lx\n", hr);
        if (SUCCEEDED(hr))
        {
            hr = IWICBitmapDecoder_Initialize(decoder, bmpstream, WICDecodeMetadataCacheOnLoad);
            ok(hr == S_OK, "Initialize failed, hr=%lx\n", hr);

            hr = IWICBitmapDecoder_GetContainerFormat(decoder, &guidresult);
            ok(SUCCEEDED(hr), "GetContainerFormat failed, hr=%lx\n", hr);
            ok(IsEqualGUID(&guidresult, &GUID_ContainerFormatBmp), "unexpected container format\n");

            hr = IWICBitmapDecoder_GetFrameCount(decoder, &count);
            ok(SUCCEEDED(hr), "GetFrameCount failed, hr=%lx\n", hr);
            ok(count == 1, "unexpected count %u\n", count);

            hr = IWICBitmapDecoder_GetFrame(decoder, 0, &framedecode);
            ok(SUCCEEDED(hr), "GetFrame failed, hr=%lx\n", hr);
            if (SUCCEEDED(hr))
            {
                IWICImagingFactory *factory;
                IWICPalette *palette;

                hr = IWICBitmapFrameDecode_GetSize(framedecode, &width, &height);
                ok(SUCCEEDED(hr), "GetSize failed, hr=%lx\n", hr);
                ok(width == 8, "expected width=8, got %u\n", width);
                ok(height == 8, "expected height=8, got %u\n", height);

                hr = IWICBitmapFrameDecode_GetResolution(framedecode, &dpiX, &dpiY);
                ok(SUCCEEDED(hr), "GetResolution failed, hr=%lx\n", hr);
                ok(fabs(dpiX - 72.0) < 0.01, "expected dpiX=96.0, got %f\n", dpiX);
                ok(fabs(dpiY - 72.0) < 0.01, "expected dpiY=96.0, got %f\n", dpiY);

                hr = IWICBitmapFrameDecode_GetPixelFormat(framedecode, &guidresult);
                ok(SUCCEEDED(hr), "GetPixelFormat failed, hr=%lx\n", hr);
                ok(IsEqualGUID(&guidresult, &GUID_WICPixelFormat32bppBGR), "unexpected pixel format\n");

                hr = CoCreateInstance(&CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER,
                    &IID_IWICImagingFactory, (void**)&factory);
                ok(SUCCEEDED(hr), "CoCreateInstance failed, hr=%lx\n", hr);
                if (SUCCEEDED(hr))
                {
                    hr = IWICImagingFactory_CreatePalette(factory, &palette);
                    ok(SUCCEEDED(hr), "CreatePalette failed, hr=%lx\n", hr);
                    if (SUCCEEDED(hr))
                    {
                        hr = IWICBitmapDecoder_CopyPalette(decoder, palette);
                        ok(hr == WINCODEC_ERR_PALETTEUNAVAILABLE, "expected WINCODEC_ERR_PALETTEUNAVAILABLE, got %lx\n", hr);

                        hr = IWICBitmapFrameDecode_CopyPalette(framedecode, palette);
                        ok(SUCCEEDED(hr), "CopyPalette failed, hr=%lx\n", hr);

                        hr = IWICPalette_GetColorCount(palette, &count);
                        ok(SUCCEEDED(hr), "GetColorCount failed, hr=%lx\n", hr);
                        ok(count == 17, "expected count=17, got %u\n", count);

                        hr = IWICPalette_GetColors(palette, 17, palettedata, &count);
                        ok(SUCCEEDED(hr), "GetColorCount failed, hr=%lx\n", hr);
                        ok(count == 17, "expected count=17, got %u\n", count);
                        ok(!memcmp(palettedata, expected_palettedata, sizeof(palettedata)), "unexpected palette data\n");

                        IWICPalette_Release(palette);
                    }

                    IWICImagingFactory_Release(factory);
                }

                rc.X = 0;
                rc.Y = 0;
                rc.Width = 8;
                rc.Height = 8;
                hr = IWICBitmapFrameDecode_CopyPixels(framedecode, &rc, 32, sizeof(imagedata), (BYTE*)imagedata);
                ok(SUCCEEDED(hr), "CopyPixels failed, hr=%lx\n", hr);
                ok(!memcmp(imagedata, expected_imagedata, sizeof(imagedata)), "unexpected image data\n");

                IWICBitmapFrameDecode_Release(framedecode);
            }

            hr = CoCreateInstance(&CLSID_WICBmpDecoder, NULL, CLSCTX_INPROC_SERVER,
                &IID_IWICBitmapDecoder, (void**)&decoder2);
            ok(SUCCEEDED(hr), "CoCreateInstance failed, hr=%lx\n", hr);
            if (SUCCEEDED(hr))
            {
                hr = IWICBitmapDecoder_QueryCapability(decoder2, bmpstream, &capability);
                ok(hr == S_OK, "QueryCapability failed, hr=%lx\n", hr);
                ok(capability == (WICBitmapDecoderCapabilityCanDecodeAllImages),
                    "unexpected capabilities: %lx\n", capability);
                IWICBitmapDecoder_Release(decoder2);
            }

            IStream_Release(bmpstream);
        }

        GlobalFree(hbmpdata);
    }

    IWICBitmapDecoder_Release(decoder);
}

static const char testbmp_rle4[] = {
    /* BITMAPFILEHEADER */
    66,77, /* "BM" */
    142,0,0,0, /* file size */
    0,0,0,0, /* reserved */
    78,0,0,0, /* offset to bits */
    /* BITMAPINFOHEADER */
    40,0,0,0, /* header size */
    8,0,0,0, /* width */
    8,0,0,0, /* height */
    1,0, /* planes */
    4,0, /* bit count */
    2,0,0,0, /* compression = BI_RLE4 */
    64,0,0,0, /* image size */
    19,11,0,0, /* X pixels per meter */
    19,11,0,0, /* Y pixels per meter */
    6,0,0,0, /* colors used */
    6,0,0,0, /* colors important */
    /* color table */
    0,0,0,0,
    255,0,0,0,
    0,0,255,0,
    255,0,255,0,
    0,255,0,0,
    255,255,255,0,
    /* bits */
    0,8,68,68,0,0,0,0,0,8,68,68,3,48,0,0,0,8,68,68,3,48,0,0,0,8,68,68,0,0,0,0,0,8,81,81,34,34,0,0,0,8,21,21,34,34,0,0,0,8,81,81,34,34,0,0,0,8,21,21,34,34,0,1
};

static void test_decode_rle4(void)
{
    IWICBitmapDecoder *decoder, *decoder2;
    IWICBitmapFrameDecode *framedecode;
    HRESULT hr;
    HGLOBAL hbmpdata;
    char *bmpdata;
    IStream *bmpstream;
    DWORD capability=0;
    GUID guidresult;
    UINT count=0, width=0, height=0;
    double dpiX, dpiY;
    DWORD imagedata[64] = {1};
    const DWORD expected_imagedata[64] = {
        0x0000ff,0xffffff,0x0000ff,0xffffff,0xff0000,0xff0000,0xff0000,0xff0000,
        0xffffff,0x0000ff,0xffffff,0x0000ff,0xff0000,0xff0000,0xff0000,0xff0000,
        0x0000ff,0xffffff,0x0000ff,0xffffff,0xff0000,0xff0000,0xff0000,0xff0000,
        0xffffff,0x0000ff,0xffffff,0x0000ff,0xff0000,0xff0000,0xff0000,0xff0000,
        0x00ff00,0x00ff00,0x00ff00,0x00ff00,0x000000,0x000000,0x000000,0x000000,
        0x00ff00,0x00ff00,0x00ff00,0x00ff00,0x000000,0xff00ff,0xff00ff,0x000000,
        0x00ff00,0x00ff00,0x00ff00,0x00ff00,0x000000,0xff00ff,0xff00ff,0x000000,
        0x00ff00,0x00ff00,0x00ff00,0x00ff00,0x000000,0x000000,0x000000,0x000000};
    WICColor palettedata[6] = {1};
    const WICColor expected_palettedata[6] = {
        0xff000000,0xff0000ff,0xffff0000,0xffff00ff,0xff00ff00,0xffffffff};
    WICRect rc;

    hr = CoCreateInstance(&CLSID_WICBmpDecoder, NULL, CLSCTX_INPROC_SERVER,
        &IID_IWICBitmapDecoder, (void**)&decoder);
    ok(SUCCEEDED(hr), "CoCreateInstance failed, hr=%lx\n", hr);
    if (FAILED(hr)) return;

    hbmpdata = GlobalAlloc(GMEM_MOVEABLE, sizeof(testbmp_rle4));
    ok(hbmpdata != 0, "GlobalAlloc failed\n");
    if (hbmpdata)
    {
        bmpdata = GlobalLock(hbmpdata);
        memcpy(bmpdata, testbmp_rle4, sizeof(testbmp_rle4));
        GlobalUnlock(hbmpdata);

        hr = CreateStreamOnHGlobal(hbmpdata, FALSE, &bmpstream);
        ok(SUCCEEDED(hr), "CreateStreamOnHGlobal failed, hr=%lx\n", hr);
        if (SUCCEEDED(hr))
        {
            hr = IWICBitmapDecoder_Initialize(decoder, bmpstream, WICDecodeMetadataCacheOnLoad);
            ok(hr == S_OK, "Initialize failed, hr=%lx\n", hr);

            hr = IWICBitmapDecoder_GetContainerFormat(decoder, &guidresult);
            ok(SUCCEEDED(hr), "GetContainerFormat failed, hr=%lx\n", hr);
            ok(IsEqualGUID(&guidresult, &GUID_ContainerFormatBmp), "unexpected container format\n");

            hr = IWICBitmapDecoder_GetFrameCount(decoder, &count);
            ok(SUCCEEDED(hr), "GetFrameCount failed, hr=%lx\n", hr);
            ok(count == 1, "unexpected count %u\n", count);

            hr = IWICBitmapDecoder_GetFrame(decoder, 0, &framedecode);
            ok(SUCCEEDED(hr), "GetFrame failed, hr=%lx\n", hr);
            if (SUCCEEDED(hr))
            {
                IWICImagingFactory *factory;
                IWICPalette *palette;

                hr = IWICBitmapFrameDecode_GetSize(framedecode, &width, &height);
                ok(SUCCEEDED(hr), "GetSize failed, hr=%lx\n", hr);
                ok(width == 8, "expected width=8, got %u\n", width);
                ok(height == 8, "expected height=8, got %u\n", height);

                hr = IWICBitmapFrameDecode_GetResolution(framedecode, &dpiX, &dpiY);
                ok(SUCCEEDED(hr), "GetResolution failed, hr=%lx\n", hr);
                ok(fabs(dpiX - 72.0) < 0.01, "expected dpiX=96.0, got %f\n", dpiX);
                ok(fabs(dpiY - 72.0) < 0.01, "expected dpiY=96.0, got %f\n", dpiY);

                hr = IWICBitmapFrameDecode_GetPixelFormat(framedecode, &guidresult);
                ok(SUCCEEDED(hr), "GetPixelFormat failed, hr=%lx\n", hr);
                ok(IsEqualGUID(&guidresult, &GUID_WICPixelFormat32bppBGR), "unexpected pixel format\n");

                hr = CoCreateInstance(&CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER,
                    &IID_IWICImagingFactory, (void**)&factory);
                ok(SUCCEEDED(hr), "CoCreateInstance failed, hr=%lx\n", hr);
                if (SUCCEEDED(hr))
                {
                    hr = IWICImagingFactory_CreatePalette(factory, &palette);
                    ok(SUCCEEDED(hr), "CreatePalette failed, hr=%lx\n", hr);
                    if (SUCCEEDED(hr))
                    {
                        hr = IWICBitmapDecoder_CopyPalette(decoder, palette);
                        ok(hr == WINCODEC_ERR_PALETTEUNAVAILABLE, "expected WINCODEC_ERR_PALETTEUNAVAILABLE, got %lx\n", hr);

                        hr = IWICBitmapFrameDecode_CopyPalette(framedecode, palette);
                        ok(SUCCEEDED(hr), "CopyPalette failed, hr=%lx\n", hr);

                        hr = IWICPalette_GetColorCount(palette, &count);
                        ok(SUCCEEDED(hr), "GetColorCount failed, hr=%lx\n", hr);
                        ok(count == 6, "expected count=6, got %u\n", count);

                        hr = IWICPalette_GetColors(palette, 6, palettedata, &count);
                        ok(SUCCEEDED(hr), "GetColorCount failed, hr=%lx\n", hr);
                        ok(count == 6, "expected count=6, got %u\n", count);
                        ok(!memcmp(palettedata, expected_palettedata, sizeof(palettedata)), "unexpected palette data\n");

                        IWICPalette_Release(palette);
                    }

                    IWICImagingFactory_Release(factory);
                }

                rc.X = 0;
                rc.Y = 0;
                rc.Width = 8;
                rc.Height = 8;
                hr = IWICBitmapFrameDecode_CopyPixels(framedecode, &rc, 32, sizeof(imagedata), (BYTE*)imagedata);
                ok(SUCCEEDED(hr), "CopyPixels failed, hr=%lx\n", hr);
                ok(!memcmp(imagedata, expected_imagedata, sizeof(imagedata)), "unexpected image data\n");

                IWICBitmapFrameDecode_Release(framedecode);
            }

            hr = CoCreateInstance(&CLSID_WICBmpDecoder, NULL, CLSCTX_INPROC_SERVER,
                &IID_IWICBitmapDecoder, (void**)&decoder2);
            ok(SUCCEEDED(hr), "CoCreateInstance failed, hr=%lx\n", hr);
            if (SUCCEEDED(hr))
            {
                hr = IWICBitmapDecoder_QueryCapability(decoder2, bmpstream, &capability);
                ok(hr == S_OK, "QueryCapability failed, hr=%lx\n", hr);
                ok(capability == (WICBitmapDecoderCapabilityCanDecodeAllImages),
                    "unexpected capabilities: %lx\n", capability);
                IWICBitmapDecoder_Release(decoder2);
            }

            IStream_Release(bmpstream);
        }

        GlobalFree(hbmpdata);
    }

    IWICBitmapDecoder_Release(decoder);
}

static const char testbmp_bitfields_abgr[] = {
    /* BITMAPFILEHEADER */
    'B','M', /* "BM" */
    sizeof(BITMAPFILEHEADER)+sizeof(BITMAPV5HEADER)+8,0,0,0, /* file size */
    0,0,0,0, /* reserved */
    sizeof(BITMAPFILEHEADER)+sizeof(BITMAPV5HEADER),0,0,0, /* offset to bits */
    /* BITMAPV5HEADER */
    sizeof(BITMAPV5HEADER),0,0,0, /* size */
    2,0,0,0, /* width */
    1,0,0,0, /* height */
    1,0, /* planes */
    32,0, /* bit count */
    BI_BITFIELDS,0,0,0, /* compression */
    8,0,0,0, /* image size */
    19,11,0,0, /* X pixels per meter */
    19,11,0,0, /* Y pixels per meter */
    0,0,0,0, /* colors used */
    0,0,0,0, /* colors important */
    0,0,0,255, /* red mask */
    0,0,255,0, /* green mask */
    0,255,0,0, /* blue mask */
    255,0,0,0, /* alpha mask */
    'B','G','R','s', /* color space */
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0, /* gamma */
    LCS_GM_GRAPHICS,0,0,0, /* intent */
    0,0,0,0,0,0,0,0,0,0,0,0,
    /* bits */
    0,255,0,0,255,0,255,0
};

static void test_decode_bitfields(void)
{
    IWICBitmapDecoder *decoder;
    IWICBitmapFrameDecode *framedecode;
    HRESULT hr;
    HGLOBAL hbmpdata;
    char *bmpdata;
    IStream *bmpstream;
    GUID guidresult;
    UINT count=0, width=0, height=0;
    double dpiX, dpiY;
    DWORD imagedata[2] = {1};
    const DWORD expected_imagedata[2] = { 0xff, 0xff00 };
    WICRect rc;

    hr = CoCreateInstance(&CLSID_WICBmpDecoder, NULL, CLSCTX_INPROC_SERVER,
        &IID_IWICBitmapDecoder, (void**)&decoder);
    ok(SUCCEEDED(hr), "CoCreateInstance failed, hr=%lx\n", hr);
    if (FAILED(hr)) return;

    hbmpdata = GlobalAlloc(GMEM_MOVEABLE, sizeof(testbmp_bitfields_abgr));
    ok(hbmpdata != 0, "GlobalAlloc failed\n");
    if (hbmpdata)
    {
        bmpdata = GlobalLock(hbmpdata);
        memcpy(bmpdata, testbmp_bitfields_abgr, sizeof(testbmp_bitfields_abgr));
        GlobalUnlock(hbmpdata);

        hr = CreateStreamOnHGlobal(hbmpdata, FALSE, &bmpstream);
        ok(SUCCEEDED(hr), "CreateStreamOnHGlobal failed, hr=%lx\n", hr);
        if (SUCCEEDED(hr))
        {
            hr = IWICBitmapDecoder_Initialize(decoder, bmpstream, WICDecodeMetadataCacheOnLoad);
            ok(hr == S_OK, "Initialize failed, hr=%lx\n", hr);

            hr = IWICBitmapDecoder_GetContainerFormat(decoder, &guidresult);
            ok(SUCCEEDED(hr), "GetContainerFormat failed, hr=%lx\n", hr);
            ok(IsEqualGUID(&guidresult, &GUID_ContainerFormatBmp), "unexpected container format\n");

            hr = IWICBitmapDecoder_GetFrameCount(decoder, &count);
            ok(SUCCEEDED(hr), "GetFrameCount failed, hr=%lx\n", hr);
            ok(count == 1, "unexpected count %u\n", count);

            hr = IWICBitmapDecoder_GetFrame(decoder, 0, &framedecode);
            ok(SUCCEEDED(hr), "GetFrame failed, hr=%lx\n", hr);
            if (SUCCEEDED(hr))
            {
                IWICImagingFactory *factory;
                IWICPalette *palette;

                hr = IWICBitmapFrameDecode_GetSize(framedecode, &width, &height);
                ok(SUCCEEDED(hr), "GetSize failed, hr=%lx\n", hr);
                ok(width == 2, "expected width=2, got %u\n", width);
                ok(height == 1, "expected height=1, got %u\n", height);

                hr = IWICBitmapFrameDecode_GetResolution(framedecode, &dpiX, &dpiY);
                ok(SUCCEEDED(hr), "GetResolution failed, hr=%lx\n", hr);
                ok(fabs(dpiX - 72.0) < 0.01, "expected dpiX=72.0, got %f\n", dpiX);
                ok(fabs(dpiY - 72.0) < 0.01, "expected dpiY=72.0, got %f\n", dpiY);

                hr = IWICBitmapFrameDecode_GetPixelFormat(framedecode, &guidresult);
                ok(SUCCEEDED(hr), "GetPixelFormat failed, hr=%lx\n", hr);
                ok(IsEqualGUID(&guidresult, &GUID_WICPixelFormat32bppBGR), "unexpected pixel format\n");

                hr = CoCreateInstance(&CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER,
                    &IID_IWICImagingFactory, (void**)&factory);
                ok(SUCCEEDED(hr), "CoCreateInstance failed, hr=%lx\n", hr);
                if (SUCCEEDED(hr))
                {
                    hr = IWICImagingFactory_CreatePalette(factory, &palette);
                    ok(SUCCEEDED(hr), "CreatePalette failed, hr=%lx\n", hr);
                    if (SUCCEEDED(hr))
                    {
                        hr = IWICBitmapDecoder_CopyPalette(decoder, palette);
                        ok(hr == WINCODEC_ERR_PALETTEUNAVAILABLE, "expected WINCODEC_ERR_PALETTEUNAVAILABLE, got %lx\n", hr);

                        hr = IWICBitmapFrameDecode_CopyPalette(framedecode, palette);
                        ok(hr == WINCODEC_ERR_PALETTEUNAVAILABLE, "expected WINCODEC_ERR_PALETTEUNAVAILABLE, got %lx\n", hr);

                        IWICPalette_Release(palette);
                    }

                    IWICImagingFactory_Release(factory);
                }

                rc.X = 0;
                rc.Y = 0;
                rc.Width = 2;
                rc.Height = 1;
                hr = IWICBitmapFrameDecode_CopyPixels(framedecode, &rc, 32, sizeof(imagedata), (BYTE*)imagedata);
                ok(SUCCEEDED(hr), "CopyPixels failed, hr=%lx\n", hr);
                ok(!memcmp(imagedata, expected_imagedata, sizeof(imagedata)), "unexpected image data\n");

                IWICBitmapFrameDecode_Release(framedecode);
            }

            IStream_Release(bmpstream);
        }

        GlobalFree(hbmpdata);
    }

    IWICBitmapDecoder_Release(decoder);
}

static void test_componentinfo(void)
{
    IWICImagingFactory *factory;
    IWICComponentInfo *info;
    IWICBitmapDecoderInfo *decoderinfo;
    IWICBitmapDecoder *decoder;
    HRESULT hr;
    WICBitmapPattern *patterns;
    UINT pattern_count, pattern_size;
    WICComponentType type;
    GUID guidresult;
    HGLOBAL hbmpdata;
    char *bmpdata;
    IStream *bmpstream;
    BOOL boolresult;

    hr = CoCreateInstance(&CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER,
        &IID_IWICImagingFactory, (void**)&factory);
    ok(SUCCEEDED(hr), "CoCreateInstance failed, hr=%lx\n", hr);
    if (SUCCEEDED(hr))
    {
        hr = IWICImagingFactory_CreateComponentInfo(factory, &CLSID_WICBmpDecoder, &info);
        ok(SUCCEEDED(hr), "CreateComponentInfo failed, hr=%lx\n", hr);
        if (SUCCEEDED(hr))
        {
            hr = IWICComponentInfo_GetComponentType(info, &type);
            ok(SUCCEEDED(hr), "GetComponentType failed, hr=%lx\n", hr);
            ok(type == WICDecoder, "got %i, expected WICDecoder\n", type);

            hr = IWICComponentInfo_QueryInterface(info, &IID_IWICBitmapDecoderInfo, (void**)&decoderinfo);
            ok(SUCCEEDED(hr), "QueryInterface failed, hr=%lx\n", hr);
            if (SUCCEEDED(hr))
            {
                pattern_count = 0;
                pattern_size = 0;
                hr = IWICBitmapDecoderInfo_GetPatterns(decoderinfo, 0, NULL, &pattern_count, &pattern_size);
                ok(SUCCEEDED(hr), "GetPatterns failed, hr=%lx\n", hr);
                ok(pattern_count != 0, "pattern count is 0\n");
                ok(pattern_size > pattern_count * sizeof(WICBitmapPattern), "size=%i, count=%i\n", pattern_size, pattern_count);

                patterns = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, pattern_size);
                hr = IWICBitmapDecoderInfo_GetPatterns(decoderinfo, pattern_size, patterns, &pattern_count, &pattern_size);
                ok(SUCCEEDED(hr), "GetPatterns failed, hr=%lx\n", hr);
                ok(pattern_count != 0, "pattern count is 0\n");
                ok(pattern_size > pattern_count * sizeof(WICBitmapPattern), "size=%i, count=%i\n", pattern_size, pattern_count);
                ok(patterns[0].Length != 0, "pattern length is 0\n");
                ok(patterns[0].Pattern != NULL, "pattern is NULL\n");
                ok(patterns[0].Mask != NULL, "mask is NULL\n");

                pattern_size -= 1;
                hr = IWICBitmapDecoderInfo_GetPatterns(decoderinfo, pattern_size, patterns, &pattern_count, &pattern_size);
                ok(hr == WINCODEC_ERR_INSUFFICIENTBUFFER, "GetPatterns returned %lx, expected WINCODEC_ERR_INSUFFICIENTBUFFER\n", hr);

                HeapFree(GetProcessHeap(), 0, patterns);

                hr = IWICBitmapDecoderInfo_CreateInstance(decoderinfo, &decoder);
                ok(SUCCEEDED(hr), "CreateInstance failed, hr=%lx\n", hr);
                if (SUCCEEDED(hr))
                {
                    hr = IWICBitmapDecoder_GetContainerFormat(decoder, &guidresult);
                    ok(SUCCEEDED(hr), "GetContainerFormat failed, hr=%lx\n", hr);
                    ok(IsEqualGUID(&guidresult, &GUID_ContainerFormatBmp), "unexpected container format\n");

                    IWICBitmapDecoder_Release(decoder);
                }

                hbmpdata = GlobalAlloc(GMEM_MOVEABLE, sizeof(testbmp_rle4));
                ok(hbmpdata != 0, "GlobalAlloc failed\n");
                if (hbmpdata)
                {
                    bmpdata = GlobalLock(hbmpdata);
                    memcpy(bmpdata, testbmp_rle4, sizeof(testbmp_rle4));
                    GlobalUnlock(hbmpdata);

                    hr = CreateStreamOnHGlobal(hbmpdata, FALSE, &bmpstream);
                    ok(SUCCEEDED(hr), "CreateStreamOnHGlobal failed, hr=%lx\n", hr);
                    if (SUCCEEDED(hr))
                    {
                        boolresult = 0;
                        hr = IWICBitmapDecoderInfo_MatchesPattern(decoderinfo, bmpstream, &boolresult);
                        ok(SUCCEEDED(hr), "MatchesPattern failed, hr=%lx\n", hr);
                        ok(boolresult, "pattern not matched\n");

                        IStream_Release(bmpstream);
                    }

                    GlobalFree(hbmpdata);
                }

                IWICBitmapDecoderInfo_Release(decoderinfo);
            }

            IWICComponentInfo_Release(info);
        }

        IWICImagingFactory_Release(factory);
    }
}

static void test_createfromstream(void)
{
    IWICBitmapDecoder *decoder;
    IWICImagingFactory *factory;
    HRESULT hr;
    HGLOBAL hbmpdata;
    char *bmpdata;
    IStream *bmpstream;
    GUID guidresult;

    hr = CoCreateInstance(&CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER,
        &IID_IWICImagingFactory, (void**)&factory);
    ok(SUCCEEDED(hr), "CoCreateInstance failed, hr=%lx\n", hr);
    if (FAILED(hr)) return;

    hbmpdata = GlobalAlloc(GMEM_MOVEABLE, sizeof(testbmp_1bpp));
    ok(hbmpdata != 0, "GlobalAlloc failed\n");
    if (hbmpdata)
    {
        bmpdata = GlobalLock(hbmpdata);
        memcpy(bmpdata, testbmp_1bpp, sizeof(testbmp_1bpp));
        GlobalUnlock(hbmpdata);

        hr = CreateStreamOnHGlobal(hbmpdata, FALSE, &bmpstream);
        ok(SUCCEEDED(hr), "CreateStreamOnHGlobal failed, hr=%lx\n", hr);
        if (SUCCEEDED(hr))
        {
            hr = IWICImagingFactory_CreateDecoderFromStream(factory, bmpstream,
                NULL, WICDecodeMetadataCacheOnDemand, &decoder);
            ok(SUCCEEDED(hr), "CreateDecoderFromStream failed, hr=%lx\n", hr);
            if (SUCCEEDED(hr))
            {
                hr = IWICBitmapDecoder_GetContainerFormat(decoder, &guidresult);
                ok(SUCCEEDED(hr), "GetContainerFormat failed, hr=%lx\n", hr);
                ok(IsEqualGUID(&guidresult, &GUID_ContainerFormatBmp), "unexpected container format\n");

                IWICBitmapDecoder_Release(decoder);
            }

            IStream_Release(bmpstream);
        }

        GlobalFree(hbmpdata);
    }

    IWICImagingFactory_Release(factory);
}

static void test_create_decoder(void)
{
    IWICBitmapDecoder *decoder;
    IWICImagingFactory *factory;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER,
        &IID_IWICImagingFactory, (void **)&factory);
    ok(hr == S_OK, "CoCreateInstance error %#lx\n", hr);

    hr = IWICImagingFactory_CreateDecoder(factory, NULL, NULL, NULL);
    ok(hr == E_INVALIDARG, "expected E_INVALIDARG, got %#lx\n", hr);

    hr = IWICImagingFactory_CreateDecoder(factory, NULL, NULL, &decoder);
    ok(hr == E_INVALIDARG, "expected E_INVALIDARG, got %#lx\n", hr);

    hr = IWICImagingFactory_CreateDecoder(factory, &GUID_ContainerFormatBmp, NULL, &decoder);
    ok(hr == S_OK, "CreateDecoder error %#lx\n", hr);
    IWICBitmapDecoder_Release(decoder);

    hr = IWICImagingFactory_CreateDecoder(factory, &GUID_ContainerFormatBmp, &GUID_VendorMicrosoft, &decoder);
    ok(hr == S_OK, "CreateDecoder error %#lx\n", hr);
    IWICBitmapDecoder_Release(decoder);

    IWICImagingFactory_Release(factory);
}

static void test_writesource_palette(void)
{
    IWICImagingFactory *factory;
    HRESULT hr;
    WICColor encode_palette[2] = {0xff111111, 0xffcccccc};
    WICColor source_palette[2] = {0xff555555, 0xffaaaaaa};
    WICColor result_palette[2];
    UINT result_colors;
    IWICBitmap *bitmap;
    IWICPalette *palette;
    IStream *stream;
    IWICBitmapEncoder *encoder;
    IWICBitmapFrameEncode *frame_encode;
    IPropertyBag2 *encode_options;
    GUID pixelformat;
    IWICBitmapDecoder *decoder;
    IWICBitmapFrameDecode *frame_decode;

    hr = CoCreateInstance(&CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER,
        &IID_IWICImagingFactory, (void **)&factory);
    ok(hr == S_OK, "CoCreateInstance error %#lx\n", hr);

    /* Encoder with palette set */
    hr = IWICImagingFactory_CreateBitmap(factory, 1, 1, &GUID_WICPixelFormat1bppIndexed,
        WICBitmapCacheOnDemand, &bitmap);
    ok(hr == S_OK, "CreateBitmap error %#lx\n", hr);

    hr = IWICImagingFactory_CreatePalette(factory, &palette);
    ok(hr == S_OK, "CreatePalette error %#lx\n", hr);

    hr = CreateStreamOnHGlobal(NULL, TRUE, &stream);
    ok(hr == S_OK, "CreateStream error %#lx\n", hr);

    hr = IWICImagingFactory_CreateEncoder(factory, &GUID_ContainerFormatBmp, &GUID_VendorMicrosoft, &encoder);
    ok(hr == S_OK, "CreateDecoder error %#lx\n", hr);

    hr = IWICBitmapEncoder_Initialize(encoder, stream, WICBitmapEncoderNoCache);
    ok(hr == S_OK, "IWICBitmapEncoder_Initialize error %#lx\n", hr);

    hr = IWICBitmapEncoder_CreateNewFrame(encoder, &frame_encode, &encode_options);
    ok(hr == S_OK, "CreateNewFrame error %#lx\n", hr);

    hr = IWICBitmapFrameEncode_Initialize(frame_encode, encode_options);
    ok(hr == S_OK, "IWICBitmapFrameEncode_Initialize error %#lx\n", hr);

    IPropertyBag2_Release(encode_options);

    hr = IWICBitmapFrameEncode_SetSize(frame_encode, 1, 1);
    ok(hr == S_OK, "SetSize error %#lx\n", hr);

    pixelformat = GUID_WICPixelFormat1bppIndexed;
    hr = IWICBitmapFrameEncode_SetPixelFormat(frame_encode, &pixelformat);
    ok(hr == S_OK, "SetPixelFormat error %#lx\n", hr);
    ok(!memcmp(&pixelformat, &GUID_WICPixelFormat1bppIndexed, sizeof(pixelformat)), "pixel format changed\n");

    hr = IWICPalette_InitializeCustom(palette, encode_palette, 2);
    ok(hr == S_OK, "InitializeCustom error %#lx\n", hr);

    hr = IWICBitmapFrameEncode_SetPalette(frame_encode, palette);
    ok(hr == S_OK, "SetPalette error %#lx\n", hr);

    hr = IWICPalette_InitializeCustom(palette, source_palette, 2);
    ok(hr == S_OK, "InitializeCustom error %#lx\n", hr);

    hr = IWICBitmap_SetPalette(bitmap, palette);
    ok(hr == S_OK, "SetPalette error %#lx\n", hr);

    hr = IWICBitmapFrameEncode_WriteSource(frame_encode, (IWICBitmapSource*)bitmap, NULL);
    ok(hr == S_OK, "WriteSource error %#lx\n", hr);

    hr = IWICBitmapFrameEncode_Commit(frame_encode);
    ok(hr == S_OK, "Commit error %#lx\n", hr);

    IWICBitmapFrameEncode_Release(frame_encode);

    hr = IWICBitmapEncoder_Commit(encoder);
    ok(hr == S_OK, "Commit error %#lx\n", hr);

    IWICBitmapEncoder_Release(encoder);

    hr = IWICImagingFactory_CreateDecoderFromStream(factory, stream, NULL, WICDecodeMetadataCacheOnLoad, &decoder);
    ok(hr == S_OK, "CreateDecoderFromStream error %#lx\n", hr);

    hr = IWICBitmapDecoder_GetFrame(decoder, 0, &frame_decode);
    ok(hr == S_OK, "GetFrame error %#lx\n", hr);

    hr = IWICBitmapFrameDecode_CopyPalette(frame_decode, palette);
    ok(hr == S_OK, "CopyPalette error %#lx\n", hr);

    hr = IWICPalette_GetColors(palette, 2, result_palette, &result_colors);
    ok(hr == S_OK, "GetColors error %#lx\n", hr);
    ok(result_colors == 2, "Got %i colors\n", result_colors);
    ok(result_palette[0] == encode_palette[0], "Unexpected palette entry: %x\n", result_palette[0]);
    ok(result_palette[1] == encode_palette[1], "Unexpected palette entry: %x\n", result_palette[0]);

    IWICBitmapFrameDecode_Release(frame_decode);
    IWICBitmapDecoder_Release(decoder);
    IStream_Release(stream);

    /* Encoder with no palette set */
    hr = IWICImagingFactory_CreateEncoder(factory, &GUID_ContainerFormatBmp, &GUID_VendorMicrosoft, &encoder);
    ok(hr == S_OK, "CreateDecoder error %#lx\n", hr);

    hr = CreateStreamOnHGlobal(NULL, TRUE, &stream);
    ok(hr == S_OK, "CreateStream error %#lx\n", hr);

    hr = IWICBitmapEncoder_Initialize(encoder, stream, WICBitmapEncoderNoCache);
    ok(hr == S_OK, "IWICBitmapEncoder_Initialize error %#lx\n", hr);

    hr = IWICBitmapEncoder_CreateNewFrame(encoder, &frame_encode, &encode_options);
    ok(hr == S_OK, "CreateNewFrame error %#lx\n", hr);

    hr = IWICBitmapFrameEncode_Initialize(frame_encode, encode_options);
    ok(hr == S_OK, "IWICBitmapFrameEncode_Initialize error %#lx\n", hr);

    IPropertyBag2_Release(encode_options);

    hr = IWICBitmapFrameEncode_SetSize(frame_encode, 1, 1);
    ok(hr == S_OK, "SetSize error %#lx\n", hr);

    pixelformat = GUID_WICPixelFormat1bppIndexed;
    hr = IWICBitmapFrameEncode_SetPixelFormat(frame_encode, &pixelformat);
    ok(hr == S_OK, "SetPixelFormat error %#lx\n", hr);
    ok(!memcmp(&pixelformat, &GUID_WICPixelFormat1bppIndexed, sizeof(pixelformat)), "pixel format changed\n");

    hr = IWICPalette_InitializeCustom(palette, source_palette, 2);
    ok(hr == S_OK, "InitializeCustom error %#lx\n", hr);

    hr = IWICBitmap_SetPalette(bitmap, palette);
    ok(hr == S_OK, "SetPalette error %#lx\n", hr);

    hr = IWICBitmapFrameEncode_WriteSource(frame_encode, (IWICBitmapSource*)bitmap, NULL);
    if (hr == WINCODEC_ERR_PALETTEUNAVAILABLE)
    {
        win_skip("old WriteSource palette behavior\n"); /* winxp */
        IWICBitmapFrameEncode_Release(frame_encode);
        IWICBitmapEncoder_Release(encoder);
        IStream_Release(stream);
        IWICPalette_Release(palette);
        IWICBitmap_Release(bitmap);
        IWICImagingFactory_Release(factory);
        return;
    }
    ok(hr == S_OK, "WriteSource error %#lx\n", hr);

    hr = IWICBitmapFrameEncode_Commit(frame_encode);
    ok(hr == S_OK, "Commit error %#lx\n", hr);

    IWICBitmapFrameEncode_Release(frame_encode);

    hr = IWICBitmapEncoder_Commit(encoder);
    ok(hr == S_OK, "Commit error %#lx\n", hr);

    IWICBitmapEncoder_Release(encoder);

    hr = IWICImagingFactory_CreateDecoderFromStream(factory, stream, NULL, WICDecodeMetadataCacheOnLoad, &decoder);
    ok(hr == S_OK, "CreateDecoderFromStream error %#lx\n", hr);

    hr = IWICBitmapDecoder_GetFrame(decoder, 0, &frame_decode);
    ok(hr == S_OK, "GetFrame error %#lx\n", hr);

    hr = IWICBitmapFrameDecode_CopyPalette(frame_decode, palette);
    ok(hr == S_OK, "CopyPalette error %#lx\n", hr);

    hr = IWICPalette_GetColors(palette, 2, result_palette, &result_colors);
    ok(hr == S_OK, "GetColors error %#lx\n", hr);
    ok(result_colors == 2, "Got %i colors\n", result_colors);
    ok(result_palette[0] == source_palette[0], "Unexpected palette entry: %x\n", result_palette[0]);
    ok(result_palette[1] == source_palette[1], "Unexpected palette entry: %x\n", result_palette[0]);

    IWICBitmapFrameDecode_Release(frame_decode);
    IWICBitmapDecoder_Release(decoder);
    IStream_Release(stream);

    IWICPalette_Release(palette);
    IWICBitmap_Release(bitmap);
    IWICImagingFactory_Release(factory);
}

static void test_encoder_formats(void)
{
    static const struct
    {
        const GUID *format;
        BOOL supported;
        const char *name;
    }
    tests[] =
    {
        {&GUID_WICPixelFormat24bppBGR, TRUE, "WICPixelFormat24bppBGR"},
        {&GUID_WICPixelFormatBlackWhite, FALSE, "WICPixelFormatBlackWhite"},
        {&GUID_WICPixelFormat1bppIndexed, TRUE, "WICPixelFormat1bppIndexed"},
        {&GUID_WICPixelFormat2bppIndexed, FALSE, "WICPixelFormat2bppIndexed"},
        {&GUID_WICPixelFormat4bppIndexed, TRUE, "WICPixelFormat4bppIndexed"},
        {&GUID_WICPixelFormat8bppIndexed, TRUE, "WICPixelFormat8bppIndexed"},
        {&GUID_WICPixelFormat16bppBGR555, TRUE, "WICPixelFormat16bppBGR555"},
        {&GUID_WICPixelFormat16bppBGR565, TRUE, "WICPixelFormat16bppBGR565"},
        {&GUID_WICPixelFormat32bppBGR, TRUE, "WICPixelFormat32bppBGR"},
        {&GUID_WICPixelFormat32bppBGRA, TRUE, "WICPixelFormat32bppBGRA"},
        {&GUID_WICPixelFormat8bppGray, FALSE, "WICPixelFormat8bppGray"},
    };

    IWICImagingFactory *factory;
    HRESULT hr;
    IStream *stream;
    IWICBitmapEncoder *encoder;
    IWICBitmapFrameEncode *frame_encode;
    GUID pixelformat;
    LONG refcount;
    unsigned int i;
    BOOL supported;

    hr = CoCreateInstance(&CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER,
        &IID_IWICImagingFactory, (void **)&factory);
    ok(hr == S_OK, "got %#lx.\n", hr);

    hr = CreateStreamOnHGlobal(NULL, TRUE, &stream);
    ok(hr == S_OK, "got %#lx.\n", hr);

    hr = IWICImagingFactory_CreateEncoder(factory, &GUID_ContainerFormatBmp, &GUID_VendorMicrosoft, &encoder);
    ok(hr == S_OK, "got %#lx.\n", hr);

    hr = IWICBitmapEncoder_Initialize(encoder, stream, WICBitmapEncoderNoCache);
    ok(hr == S_OK, "got %#lx.\n", hr);
    hr = IWICBitmapEncoder_CreateNewFrame(encoder, &frame_encode, NULL);
    ok(hr == S_OK, "got %#lx.\n", hr);
    hr = IWICBitmapFrameEncode_Initialize(frame_encode, NULL);
    ok(hr == S_OK, "got %#lx.\n", hr);

    for (i = 0; i < ARRAY_SIZE(tests); ++i)
    {
        winetest_push_context("%s", tests[i].name);
        pixelformat = *tests[i].format;
        hr = IWICBitmapFrameEncode_SetPixelFormat(frame_encode, &pixelformat);
        ok(hr == S_OK, "got %#lx.\n", hr);
        supported = !memcmp(&pixelformat, tests[i].format, sizeof(pixelformat));
        ok(supported == tests[i].supported, "got %d.\n", supported);
        winetest_pop_context();
    }

    IWICBitmapFrameEncode_Release(frame_encode);
    refcount = IWICBitmapEncoder_Release(encoder);
    ok(!refcount, "got %ld.\n", refcount);
    IStream_Release(stream);
    IWICImagingFactory_Release(factory);
}

START_TEST(bmpformat)
{
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    test_decode_24bpp();
    test_decode_1bpp();
    test_decode_4bpp();
    test_decode_rle8();
    test_decode_rle4();
    test_decode_bitfields();
    test_componentinfo();
    test_createfromstream();
    test_create_decoder();
    test_writesource_palette();
    test_encoder_formats();

    CoUninitialize();
}
