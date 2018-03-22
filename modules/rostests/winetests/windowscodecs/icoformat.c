/*
 * Copyright 2010 Damjan Jovanovic
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
#include "wine/test.h"

static unsigned char testico_bad_icondirentry_size[] = {
    /* ICONDIR */
    0, 0, /* reserved */
    1, 0, /* type */
    1, 0, /* count */
    /* ICONDIRENTRY */
    2, /* width */
    2, /* height */
    2, /* colorCount */
    0, /* reserved */
    1,0, /* planes */
    8,0, /* bitCount */
    (40+2*4+16*16+16*4) & 0xFF,((40+2*4+16*16+16*4) >> 8) & 0xFF,0,0, /* bytesInRes */
    22,0,0,0, /* imageOffset */
    /* BITMAPINFOHEADER */
    40,0,0,0, /* header size */
    16,0,0,0, /* width */
    2*16,0,0,0, /* height (XOR+AND rows) */
    1,0, /* planes */
    8,0, /* bit count */
    0,0,0,0, /* compression */
    0,0,0,0, /* sizeImage */
    0,0,0,0, /* x pels per meter */
    0,0,0,0, /* y pels per meter */
    2,0,0,0, /* clrUsed */
    0,0,0,0, /* clrImportant */
    /* palette */
    0,0,0,0,
    0xFF,0xFF,0xFF,0,
    /* XOR mask */
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,1,0,0,0,0,0,0,0,0,0,0,0,1,0,0,
    0,1,0,0,0,0,0,0,0,0,0,0,0,1,0,0,
    0,1,0,0,0,0,0,0,0,0,0,0,0,1,0,0,
    0,0,1,0,0,0,0,0,0,0,0,0,1,0,0,0,
    0,0,1,0,0,0,0,0,0,0,0,0,1,0,0,0,
    0,0,1,0,0,0,0,0,0,0,0,0,1,0,0,0,
    0,0,0,1,0,0,0,0,0,0,0,1,0,0,0,0,
    0,0,0,1,0,0,0,0,0,0,0,1,0,0,0,0,
    0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,0,
    0,0,0,0,1,0,1,0,1,0,1,0,0,0,0,0,
    0,0,0,0,0,1,0,0,0,1,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    /* AND mask */
    0,0,0,0,
    0,0,0,0,
    0,0,0,0,
    0,0,0,0,
    0,0,0,0,
    0,0,0,0,
    0,0,0,0,
    0,0,0,0,
    0,0,0,0,
    0,0,0,0,
    0,0,0,0,
    0,0,0,0,
    0,0,0,0,
    0,0,0,0,
    0,0,0,0,
    0,0,0,0
};

static void test_bad_icondirentry_size(void)
{
    IWICBitmapDecoder *decoder;
    IWICImagingFactory *factory;
    HRESULT hr;
    IWICStream *icostream;
    IWICBitmapFrameDecode *framedecode = NULL;

    hr = CoCreateInstance(&CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER,
        &IID_IWICImagingFactory, (void**)&factory);
    ok(hr == S_OK, "CoCreateInstance failed, hr=%x\n", hr);
    if (FAILED(hr)) return;

    hr = IWICImagingFactory_CreateStream(factory, &icostream);
    ok(hr == S_OK, "CreateStream failed, hr=%x\n", hr);
    if (SUCCEEDED(hr))
    {
        hr = IWICStream_InitializeFromMemory(icostream, testico_bad_icondirentry_size,
            sizeof(testico_bad_icondirentry_size));
        ok(hr == S_OK, "InitializeFromMemory failed, hr=%x\n", hr);

        if (SUCCEEDED(hr))
        {
            hr = CoCreateInstance(&CLSID_WICIcoDecoder, NULL, CLSCTX_INPROC_SERVER,
                &IID_IWICBitmapDecoder, (void**)&decoder);
            ok(hr == S_OK, "CoCreateInstance failed, hr=%x\n", hr);
        }

        if (SUCCEEDED(hr))
        {
            hr = IWICBitmapDecoder_Initialize(decoder, (IStream*)icostream,
                WICDecodeMetadataCacheOnDemand);
            ok(hr == S_OK, "Initialize failed, hr=%x\n", hr);

            if (SUCCEEDED(hr))
            {
                hr = IWICBitmapDecoder_GetFrame(decoder, 0, &framedecode);
                ok(hr == S_OK, "GetFrame failed, hr=%x\n", hr);
            }

            if (SUCCEEDED(hr))
            {
                UINT width, height;
                IWICBitmapSource *thumbnail;

                width = height = 0;
                hr = IWICBitmapFrameDecode_GetSize(framedecode, &width, &height);
                ok(hr == S_OK, "GetFrameSize failed, hr=%x\n", hr);
                ok(width == 16 && height == 16, "framesize=%ux%u\n", width, height);

                hr = IWICBitmapFrameDecode_GetThumbnail(framedecode, &thumbnail);
                ok(hr == S_OK, "GetThumbnail failed, hr=%x\n", hr);
                if (hr == S_OK)
                {
                    width = height = 0;
                    hr = IWICBitmapSource_GetSize(thumbnail, &width, &height);
                    ok(hr == S_OK, "GetFrameSize failed, hr=%x\n", hr);
                    ok(width == 16 && height == 16, "framesize=%ux%u\n", width, height);
                    IWICBitmapSource_Release(thumbnail);
                }
                IWICBitmapFrameDecode_Release(framedecode);
            }

            IWICBitmapDecoder_Release(decoder);
        }

        IWICStream_Release(icostream);
    }

    IWICImagingFactory_Release(factory);
}

START_TEST(icoformat)
{
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    test_bad_icondirentry_size();

    CoUninitialize();
}
