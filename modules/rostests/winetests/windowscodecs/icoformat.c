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

#include "pshpack1.h"

struct ICONHEADER
{
    WORD idReserved;
    WORD idType;
    WORD idCount;
};

struct ICONDIRENTRY
{
    BYTE bWidth;
    BYTE bHeight;
    BYTE bColorCount;
    BYTE bReserved;
    WORD wPlanes;
    WORD wBitCount;
    DWORD dwDIBSize;
    DWORD dwDIBOffset;
};

struct test_ico
{
    struct ICONHEADER header;
    struct ICONDIRENTRY direntry;
    BITMAPINFOHEADER bmi;
    unsigned char data[512];
};

static const struct test_ico ico_1 =
{
    /* ICONHEADER */
    {
      0, /* reserved */
      1, /* type */
      1, /* count */
    },
    /* ICONDIRENTRY */
    {
      16, /* width */
      16, /* height */
      2, /* color count */
      0, /* reserved */
      1, /* planes */
      8, /* bitcount*/
      40 + 2*4 + 16 * 16 + 16 * 4, /* data size */
      22 /* data offset */
    },
    /* BITMAPINFOHEADER */
    {
      sizeof(BITMAPINFOHEADER), /* header size */
      16, /* width */
      2*16, /* height (XOR+AND rows) */
      1, /* planes */
      8, /* bit count */
      0, /* compression */
      0, /* sizeImage */
      0, /* x pels per meter */
      0, /* y pels per meter */
      2, /* clrUsed */
      0, /* clrImportant */
    },
    {
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
    }
};

#include "poppack.h"

#define test_ico_data(a, b, c) test_ico_data_(a, b, c,  0, __LINE__)
#define test_ico_data_todo(a, b, c) test_ico_data_(a, b, c, 1, __LINE__)
static void test_ico_data_(void *data, DWORD data_size, HRESULT init_hr, int todo, unsigned int line)
{
    IWICBitmapDecoder *decoder;
    IWICImagingFactory *factory;
    HRESULT hr;
    IWICStream *icostream;
    IWICBitmapFrameDecode *framedecode = NULL;

    hr = CoCreateInstance(&CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER,
        &IID_IWICImagingFactory, (void**)&factory);
    ok(hr == S_OK, "CoCreateInstance failed, hr=%lx\n", hr);
    if (FAILED(hr)) return;

    hr = IWICImagingFactory_CreateStream(factory, &icostream);
    ok(hr == S_OK, "CreateStream failed, hr=%lx\n", hr);
    if (SUCCEEDED(hr))
    {
        hr = IWICStream_InitializeFromMemory(icostream, data, data_size);
        ok(hr == S_OK, "InitializeFromMemory failed, hr=%lx\n", hr);

        if (SUCCEEDED(hr))
        {
            hr = CoCreateInstance(&CLSID_WICIcoDecoder, NULL, CLSCTX_INPROC_SERVER,
                &IID_IWICBitmapDecoder, (void**)&decoder);
            ok(hr == S_OK, "CoCreateInstance failed, hr=%lx\n", hr);
        }

        if (SUCCEEDED(hr))
        {
            hr = IWICBitmapDecoder_Initialize(decoder, (IStream*)icostream,
                WICDecodeMetadataCacheOnDemand);
            todo_wine_if(todo)
            ok_(__FILE__, line)(hr == init_hr, "Initialize failed, hr=%lx\n", hr);

            if (SUCCEEDED(hr))
            {
                hr = IWICBitmapDecoder_GetFrame(decoder, 0, &framedecode);
                ok(hr == S_OK, "GetFrame failed, hr=%lx\n", hr);
            }

            if (SUCCEEDED(hr))
            {
                UINT width, height;
                IWICBitmapSource *thumbnail;

                width = height = 0;
                hr = IWICBitmapFrameDecode_GetSize(framedecode, &width, &height);
                ok(hr == S_OK, "GetFrameSize failed, hr=%lx\n", hr);
                ok(width == 16 && height == 16, "framesize=%ux%u\n", width, height);

                hr = IWICBitmapFrameDecode_GetThumbnail(framedecode, &thumbnail);
                ok(hr == S_OK, "GetThumbnail failed, hr=%lx\n", hr);
                if (hr == S_OK)
                {
                    width = height = 0;
                    hr = IWICBitmapSource_GetSize(thumbnail, &width, &height);
                    ok(hr == S_OK, "GetFrameSize failed, hr=%lx\n", hr);
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

static void test_decoder(void)
{
    struct test_ico ico;

    /* Icon size specified in ICONDIRENTRY does not match bitmap header. */
    ico = ico_1;
    ico.direntry.bWidth = 2;
    ico.direntry.bHeight = 2;
    test_ico_data(&ico, sizeof(ico), S_OK);

    /* Invalid DIRENTRY data size/offset. */
    ico = ico_1;
    ico.direntry.dwDIBOffset = sizeof(ico);
    test_ico_data(&ico, sizeof(ico), WINCODEC_ERR_BADIMAGE);

    ico = ico_1;
    ico.direntry.dwDIBSize = sizeof(ico);
    test_ico_data(&ico, sizeof(ico), WINCODEC_ERR_BADIMAGE);

    /* Header fields validation. */
    ico = ico_1;
    ico.header.idReserved = 1;
    test_ico_data_todo(&ico, sizeof(ico), S_OK);
    ico.header.idReserved = 0;
    ico.header.idType = 100;
    test_ico_data_todo(&ico, sizeof(ico), S_OK);

    /* Premature end of data. */
    ico = ico_1;
    test_ico_data(&ico, sizeof(ico.header) - 1, WINCODEC_ERR_STREAMREAD);
    test_ico_data(&ico, sizeof(ico.header) + sizeof(ico.direntry) - 1, WINCODEC_ERR_BADIMAGE);
}

START_TEST(icoformat)
{
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    test_decoder();

    CoUninitialize();
}
