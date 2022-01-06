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


#define COBJMACROS
#include "config.h"

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "objbase.h"

#include "wincodecs_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(wincodecs);

extern BOOL WINAPI WIC_DllMain(HINSTANCE, DWORD, LPVOID) DECLSPEC_HIDDEN;

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{

    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hinstDLL);
            break;
        case DLL_PROCESS_DETACH:
            ReleaseComponentInfos();
            break;
    }

    return WIC_DllMain(hinstDLL, fdwReason, lpvReserved);
}

HRESULT WINAPI DllCanUnloadNow(void)
{
    return S_FALSE;
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

HRESULT configure_write_source(IWICBitmapFrameEncode *iface,
    IWICBitmapSource *source, const WICRect *prc,
    const WICPixelFormatGUID *format,
    INT width, INT height, double xres, double yres)
{
    HRESULT hr = S_OK;

    if (width == 0 || height == 0)
        return WINCODEC_ERR_WRONGSTATE;

    if (!format)
    {
        WICPixelFormatGUID src_format;

        hr = IWICBitmapSource_GetPixelFormat(source, &src_format);
        if (FAILED(hr)) return hr;

        hr = IWICBitmapFrameEncode_SetPixelFormat(iface, &src_format);
        if (FAILED(hr)) return hr;
    }

    if (xres == 0.0 || yres == 0.0)
    {
        hr = IWICBitmapSource_GetResolution(source, &xres, &yres);
        if (FAILED(hr)) return hr;
        hr = IWICBitmapFrameEncode_SetResolution(iface, xres, yres);
        if (FAILED(hr)) return hr;
    }

    return hr;
}

HRESULT write_source(IWICBitmapFrameEncode *iface,
    IWICBitmapSource *source, const WICRect *prc,
    const WICPixelFormatGUID *format, UINT bpp,
    INT width, INT height)
{
    IWICBitmapSource *converted_source;
    HRESULT hr=S_OK;
    WICRect rc;
    UINT stride;
    BYTE* pixeldata;

    if (!prc)
    {
        UINT src_width, src_height;
        hr = IWICBitmapSource_GetSize(source, &src_width, &src_height);
        if (FAILED(hr)) return hr;
        rc.X = 0;
        rc.Y = 0;
        rc.Width = src_width;
        rc.Height = src_height;
        prc = &rc;
    }

    if (prc->Width != width || prc->Height <= 0)
        return E_INVALIDARG;

    hr = WICConvertBitmapSource(format, source, &converted_source);
    if (FAILED(hr))
    {
        ERR("Failed to convert source, target format %s, %#x\n", debugstr_guid(format), hr);
        return E_NOTIMPL;
    }

    stride = (bpp * width + 7)/8;

    pixeldata = HeapAlloc(GetProcessHeap(), 0, stride * prc->Height);
    if (!pixeldata)
    {
        IWICBitmapSource_Release(converted_source);
        return E_OUTOFMEMORY;
    }

    hr = IWICBitmapSource_CopyPixels(converted_source, prc, stride,
        stride*prc->Height, pixeldata);

    if (SUCCEEDED(hr))
    {
        hr = IWICBitmapFrameEncode_WritePixels(iface, prc->Height, stride,
            stride*prc->Height, pixeldata);
    }

    HeapFree(GetProcessHeap(), 0, pixeldata);
    IWICBitmapSource_Release(converted_source);

    return hr;
}

void reverse_bgr8(UINT bytesperpixel, LPBYTE bits, UINT width, UINT height, INT stride)
{
    UINT x, y;
    BYTE *pixel, temp;

    for (y=0; y<height; y++)
    {
        pixel = bits + stride * y;

        for (x=0; x<width; x++)
        {
            temp = pixel[2];
            pixel[2] = pixel[0];
            pixel[0] = temp;
            pixel += bytesperpixel;
        }
    }
}

void convert_rgba_to_bgra(UINT bytesperpixel, LPBYTE bits, UINT width, UINT height, INT stride)
{
    UINT x, y;
    BYTE *pixel, temp;

    for (y=0; y<height; y++)
    {
        pixel = bits + stride * y;

        for (x=0; x<width; x++)
        {
            temp = pixel[3];
            pixel[3] = pixel[0];
            pixel[0] = pixel[1];
            pixel[1] = pixel[2];
            pixel[2] = temp;
            pixel += bytesperpixel;
        }
    }
}

HRESULT get_pixelformat_bpp(const GUID *pixelformat, UINT *bpp)
{
    HRESULT hr;
    IWICComponentInfo *info;
    IWICPixelFormatInfo *formatinfo;

    hr = CreateComponentInfo(pixelformat, &info);
    if (SUCCEEDED(hr))
    {
        hr = IWICComponentInfo_QueryInterface(info, &IID_IWICPixelFormatInfo, (void**)&formatinfo);

        if (SUCCEEDED(hr))
        {
            hr = IWICPixelFormatInfo_GetBitsPerPixel(formatinfo, bpp);

            IWICPixelFormatInfo_Release(formatinfo);
        }

        IWICComponentInfo_Release(info);
    }

    return hr;
}
