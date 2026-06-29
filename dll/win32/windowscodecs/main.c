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

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "objbase.h"

#include "wincodecs_private.h"

#include "wine/debug.h"

extern BOOL WINAPI WIC_DllMain(HINSTANCE, DWORD, LPVOID);

HMODULE windowscodecs_module = 0;

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{

    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hinstDLL);
            windowscodecs_module = hinstDLL;
            break;
        case DLL_PROCESS_DETACH:
            if (lpvReserved) break;
            ReleaseComponentInfos();
            break;
    }

    return WIC_DllMain(hinstDLL, fdwReason, lpvReserved);
}

#ifdef __REACTOS__
HRESULT WINAPI DllCanUnloadNow(void)
{
    return S_FALSE;
}
#endif

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

HRESULT TiffDecoder_CreateInstance(REFIID iid, void** ppv)
{
    HRESULT hr;
    struct decoder *decoder;
    struct decoder_info decoder_info;

    hr = tiff_decoder_create(&decoder_info, &decoder);

    if (SUCCEEDED(hr))
        hr = CommonDecoder_CreateInstance(decoder, &decoder_info, iid, ppv);

    return hr;
}

HRESULT TiffEncoder_CreateInstance(REFIID iid, void** ppv)
{
    HRESULT hr;
    struct encoder *encoder;
    struct encoder_info encoder_info;

    hr = tiff_encoder_create(&encoder_info, &encoder);

    if (SUCCEEDED(hr))
        hr = CommonEncoder_CreateInstance(encoder, &encoder_info, iid, ppv);

    return hr;
}

HRESULT JpegDecoder_CreateInstance(REFIID iid, void** ppv)
{
    HRESULT hr;
    struct decoder *decoder;
    struct decoder_info decoder_info;

    hr = jpeg_decoder_create(&decoder_info, &decoder);

    if (SUCCEEDED(hr))
        hr = CommonDecoder_CreateInstance(decoder, &decoder_info, iid, ppv);

    return hr;
}

HRESULT JpegEncoder_CreateInstance(REFIID iid, void** ppv)
{
    HRESULT hr;
    struct encoder *encoder;
    struct encoder_info encoder_info;

    hr = jpeg_encoder_create(&encoder_info, &encoder);

    if (SUCCEEDED(hr))
        hr = CommonEncoder_CreateInstance(encoder, &encoder_info, iid, ppv);

    return hr;
}
