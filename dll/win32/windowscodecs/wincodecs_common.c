/*
 * wincodecs_common.c - Functions shared with other WIC libraries.
 *
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

#define COBJMACROS

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "objbase.h"

#include "wincodecs_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(wincodecs);

#include "wincodecs_common.h"

HRESULT configure_write_source(IWICBitmapFrameEncode *iface,
    IWICBitmapSource *source, const WICRect *prc,
    const WICPixelFormatGUID *format,
    INT width, INT height, double xres, double yres)
{
    UINT src_width, src_height;
    HRESULT hr = S_OK;

    if (width == 0 && height == 0)
    {
        if (prc)
        {
            if (prc->Width <= 0 || prc->Height <= 0) return E_INVALIDARG;
            width = prc->Width;
            height = prc->Height;
        }
        else
        {
            hr = IWICBitmapSource_GetSize(source, &src_width, &src_height);
            if (FAILED(hr)) return hr;
            if (src_width == 0 || src_height == 0) return E_INVALIDARG;
            width = src_width;
            height = src_height;
        }
        hr = IWICBitmapFrameEncode_SetSize(iface, (UINT)width, (UINT)height);
        if (FAILED(hr)) return hr;
    }
    if (width == 0 || height == 0) return E_INVALIDARG;

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
    const WICPixelFormatGUID *format, UINT bpp, BOOL need_palette,
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
        ERR("Failed to convert source, target format %s, %#lx\n", debugstr_guid(format), hr);
        return E_NOTIMPL;
    }

    if (need_palette)
    {
        IWICImagingFactory *factory;
        IWICPalette *palette;

        hr = create_instance(&CLSID_WICImagingFactory, &IID_IWICImagingFactory, (void**)&factory);

        if (SUCCEEDED(hr))
        {
            hr = IWICImagingFactory_CreatePalette(factory, &palette);
            IWICImagingFactory_Release(factory);
        }

        if (SUCCEEDED(hr))
        {
            hr = IWICBitmapSource_CopyPalette(converted_source, palette);

            if (SUCCEEDED(hr))
                hr = IWICBitmapFrameEncode_SetPalette(iface, palette);

            IWICPalette_Release(palette);
        }

        if (FAILED(hr))
        {
            IWICBitmapSource_Release(converted_source);
            return hr;
        }
    }

    stride = (bpp * width + 7)/8;

    pixeldata = malloc(stride * prc->Height);
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

    free(pixeldata);
    IWICBitmapSource_Release(converted_source);

    return hr;
}

HRESULT CDECL stream_getsize(IStream *stream, ULONGLONG *size)
{
    STATSTG statstg;
    HRESULT hr;

    hr = IStream_Stat(stream, &statstg, STATFLAG_NONAME);

    if (SUCCEEDED(hr))
        *size = statstg.cbSize.QuadPart;

    return hr;
}

HRESULT CDECL stream_read(IStream *stream, void *buffer, ULONG read, ULONG *bytes_read)
{
    return IStream_Read(stream, buffer, read, bytes_read);
}

HRESULT CDECL stream_seek(IStream *stream, LONGLONG ofs, DWORD origin, ULONGLONG *new_position)
{
    HRESULT hr;
    LARGE_INTEGER ofs_large;
    ULARGE_INTEGER pos_large;

    ofs_large.QuadPart = ofs;
    hr = IStream_Seek(stream, ofs_large, origin, &pos_large);
    if (new_position)
        *new_position = pos_large.QuadPart;

    return hr;
}

HRESULT CDECL stream_write(IStream *stream, const void *buffer, ULONG write, ULONG *bytes_written)
{
    return IStream_Write(stream, buffer, write, bytes_written);
}
