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

#include "wincodecs_private.h"

extern BOOL WINAPI WIC_DllMain(HINSTANCE, DWORD, LPVOID) DECLSPEC_HIDDEN;

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{

    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hinstDLL);
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

    if ((dststride * (rc->Height-1)) + ((rc->Width * bpp) + 7)/8 > dstbuffersize)
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
