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

#include "config.h"
#include "wine/port.h"

#include <stdarg.h>

#ifdef HAVE_PNG_H
#include <png.h>
#endif

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "objbase.h"
#include "wincodec.h"

#include "wincodecs_private.h"

#include "wine/debug.h"
#include "wine/library.h"

WINE_DEFAULT_DEBUG_CHANNEL(wincodecs);

#ifdef SONAME_LIBPNG

static void *libpng_handle;
#define MAKE_FUNCPTR(f) static typeof(f) * p##f
MAKE_FUNCPTR(png_create_read_struct);
MAKE_FUNCPTR(png_create_info_struct);
MAKE_FUNCPTR(png_create_write_struct);
MAKE_FUNCPTR(png_destroy_read_struct);
MAKE_FUNCPTR(png_destroy_write_struct);
MAKE_FUNCPTR(png_error);
MAKE_FUNCPTR(png_get_bit_depth);
MAKE_FUNCPTR(png_get_color_type);
MAKE_FUNCPTR(png_get_image_height);
MAKE_FUNCPTR(png_get_image_width);
MAKE_FUNCPTR(png_get_io_ptr);
MAKE_FUNCPTR(png_get_pHYs);
MAKE_FUNCPTR(png_get_PLTE);
MAKE_FUNCPTR(png_get_tRNS);
MAKE_FUNCPTR(png_set_bgr);
MAKE_FUNCPTR(png_set_filler);
MAKE_FUNCPTR(png_set_gray_1_2_4_to_8);
MAKE_FUNCPTR(png_set_gray_to_rgb);
MAKE_FUNCPTR(png_set_IHDR);
MAKE_FUNCPTR(png_set_pHYs);
MAKE_FUNCPTR(png_set_read_fn);
MAKE_FUNCPTR(png_set_strip_16);
MAKE_FUNCPTR(png_set_tRNS_to_alpha);
MAKE_FUNCPTR(png_set_write_fn);
MAKE_FUNCPTR(png_read_end);
MAKE_FUNCPTR(png_read_image);
MAKE_FUNCPTR(png_read_info);
MAKE_FUNCPTR(png_write_end);
MAKE_FUNCPTR(png_write_info);
MAKE_FUNCPTR(png_write_rows);
#undef MAKE_FUNCPTR

static void *load_libpng(void)
{
    if((libpng_handle = wine_dlopen(SONAME_LIBPNG, RTLD_NOW, NULL, 0)) != NULL) {

#define LOAD_FUNCPTR(f) \
    if((p##f = wine_dlsym(libpng_handle, #f, NULL, 0)) == NULL) { \
        libpng_handle = NULL; \
        return NULL; \
    }
        LOAD_FUNCPTR(png_create_read_struct);
        LOAD_FUNCPTR(png_create_info_struct);
        LOAD_FUNCPTR(png_create_write_struct);
        LOAD_FUNCPTR(png_destroy_read_struct);
        LOAD_FUNCPTR(png_destroy_write_struct);
        LOAD_FUNCPTR(png_error);
        LOAD_FUNCPTR(png_get_bit_depth);
        LOAD_FUNCPTR(png_get_color_type);
        LOAD_FUNCPTR(png_get_image_height);
        LOAD_FUNCPTR(png_get_image_width);
        LOAD_FUNCPTR(png_get_io_ptr);
        LOAD_FUNCPTR(png_get_pHYs);
        LOAD_FUNCPTR(png_get_PLTE);
        LOAD_FUNCPTR(png_get_tRNS);
        LOAD_FUNCPTR(png_set_bgr);
        LOAD_FUNCPTR(png_set_filler);
        LOAD_FUNCPTR(png_set_gray_1_2_4_to_8);
        LOAD_FUNCPTR(png_set_gray_to_rgb);
        LOAD_FUNCPTR(png_set_IHDR);
        LOAD_FUNCPTR(png_set_pHYs);
        LOAD_FUNCPTR(png_set_read_fn);
        LOAD_FUNCPTR(png_set_strip_16);
        LOAD_FUNCPTR(png_set_tRNS_to_alpha);
        LOAD_FUNCPTR(png_set_write_fn);
        LOAD_FUNCPTR(png_read_end);
        LOAD_FUNCPTR(png_read_image);
        LOAD_FUNCPTR(png_read_info);
        LOAD_FUNCPTR(png_write_end);
        LOAD_FUNCPTR(png_write_info);
        LOAD_FUNCPTR(png_write_rows);

#undef LOAD_FUNCPTR
    }
    return libpng_handle;
}

typedef struct {
    const IWICBitmapDecoderVtbl *lpVtbl;
    const IWICBitmapFrameDecodeVtbl *lpFrameVtbl;
    LONG ref;
    png_structp png_ptr;
    png_infop info_ptr;
    png_infop end_info;
    BOOL initialized;
    int bpp;
    int width, height;
    UINT stride;
    const WICPixelFormatGUID *format;
    BYTE *image_bits;
} PngDecoder;

static inline PngDecoder *impl_from_frame(IWICBitmapFrameDecode *iface)
{
    return CONTAINING_RECORD(iface, PngDecoder, lpFrameVtbl);
}

static const IWICBitmapFrameDecodeVtbl PngDecoder_FrameVtbl;

static HRESULT WINAPI PngDecoder_QueryInterface(IWICBitmapDecoder *iface, REFIID iid,
    void **ppv)
{
    PngDecoder *This = (PngDecoder*)iface;
    TRACE("(%p,%s,%p)\n", iface, debugstr_guid(iid), ppv);

    if (!ppv) return E_INVALIDARG;

    if (IsEqualIID(&IID_IUnknown, iid) || IsEqualIID(&IID_IWICBitmapDecoder, iid))
    {
        *ppv = This;
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI PngDecoder_AddRef(IWICBitmapDecoder *iface)
{
    PngDecoder *This = (PngDecoder*)iface;
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) refcount=%u\n", iface, ref);

    return ref;
}

static ULONG WINAPI PngDecoder_Release(IWICBitmapDecoder *iface)
{
    PngDecoder *This = (PngDecoder*)iface;
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) refcount=%u\n", iface, ref);

    if (ref == 0)
    {
        if (This->png_ptr)
            ppng_destroy_read_struct(&This->png_ptr, &This->info_ptr, &This->end_info);
        HeapFree(GetProcessHeap(), 0, This->image_bits);
        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}

static HRESULT WINAPI PngDecoder_QueryCapability(IWICBitmapDecoder *iface, IStream *pIStream,
    DWORD *pdwCapability)
{
    FIXME("(%p,%p,%p): stub\n", iface, pIStream, pdwCapability);
    return E_NOTIMPL;
}

static void user_read_data(png_structp png_ptr, png_bytep data, png_size_t length)
{
    IStream *stream = ppng_get_io_ptr(png_ptr);
    HRESULT hr;
    ULONG bytesread;

    hr = IStream_Read(stream, data, length, &bytesread);
    if (FAILED(hr) || bytesread != length)
    {
        ppng_error(png_ptr, "failed reading data");
    }
}

static HRESULT WINAPI PngDecoder_Initialize(IWICBitmapDecoder *iface, IStream *pIStream,
    WICDecodeOptions cacheOptions)
{
    PngDecoder *This = (PngDecoder*)iface;
    LARGE_INTEGER seek;
    HRESULT hr;
    png_bytep *row_pointers=NULL;
    UINT image_size;
    UINT i;
    int color_type, bit_depth;
    png_bytep trans;
    int num_trans;
    png_uint_32 transparency;
    png_color_16p trans_values;
    TRACE("(%p,%p,%x)\n", iface, pIStream, cacheOptions);

    /* initialize libpng */
    This->png_ptr = ppng_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!This->png_ptr) return E_FAIL;

    This->info_ptr = ppng_create_info_struct(This->png_ptr);
    if (!This->info_ptr)
    {
        ppng_destroy_read_struct(&This->png_ptr, NULL, NULL);
        This->png_ptr = NULL;
        return E_FAIL;
    }

    This->end_info = ppng_create_info_struct(This->png_ptr);
    if (!This->info_ptr)
    {
        ppng_destroy_read_struct(&This->png_ptr, &This->info_ptr, NULL);
        This->png_ptr = NULL;
        return E_FAIL;
    }

    /* set up setjmp/longjmp error handling */
    if (setjmp(png_jmpbuf(This->png_ptr)))
    {
        ppng_destroy_read_struct(&This->png_ptr, &This->info_ptr, &This->end_info);
        HeapFree(GetProcessHeap(), 0, row_pointers);
        This->png_ptr = NULL;
        return E_FAIL;
    }

    /* seek to the start of the stream */
    seek.QuadPart = 0;
    hr = IStream_Seek(pIStream, seek, STREAM_SEEK_SET, NULL);
    if (FAILED(hr)) return hr;

    /* set up custom i/o handling */
    ppng_set_read_fn(This->png_ptr, pIStream, user_read_data);

    /* read the header */
    ppng_read_info(This->png_ptr, This->info_ptr);

    /* choose a pixel format */
    color_type = ppng_get_color_type(This->png_ptr, This->info_ptr);
    bit_depth = ppng_get_bit_depth(This->png_ptr, This->info_ptr);

    /* check for color-keyed alpha */
    transparency = ppng_get_tRNS(This->png_ptr, This->info_ptr, &trans, &num_trans, &trans_values);

    if (transparency && color_type != PNG_COLOR_TYPE_PALETTE)
    {
        /* expand to RGBA */
        if (color_type == PNG_COLOR_TYPE_GRAY)
        {
            if (bit_depth < 8)
            {
                ppng_set_gray_1_2_4_to_8(This->png_ptr);
                bit_depth = 8;
            }
            ppng_set_gray_to_rgb(This->png_ptr);
        }
        ppng_set_tRNS_to_alpha(This->png_ptr);
        color_type = PNG_COLOR_TYPE_RGB_ALPHA;
    }

    switch (color_type)
    {
    case PNG_COLOR_TYPE_GRAY:
        This->bpp = bit_depth;
        switch (bit_depth)
        {
        case 1: This->format = &GUID_WICPixelFormatBlackWhite; break;
        case 2: This->format = &GUID_WICPixelFormat2bppGray; break;
        case 4: This->format = &GUID_WICPixelFormat4bppGray; break;
        case 8: This->format = &GUID_WICPixelFormat8bppGray; break;
        case 16: This->format = &GUID_WICPixelFormat16bppGray; break;
        default:
            ERR("invalid grayscale bit depth: %i\n", bit_depth);
            return E_FAIL;
        }
        break;
    case PNG_COLOR_TYPE_GRAY_ALPHA:
        /* WIC does not support grayscale alpha formats so use RGBA */
        ppng_set_gray_to_rgb(This->png_ptr);
    case PNG_COLOR_TYPE_RGB_ALPHA:
        This->bpp = bit_depth * 4;
        switch (bit_depth)
        {
        case 8:
            ppng_set_bgr(This->png_ptr);
            This->format = &GUID_WICPixelFormat32bppBGRA;
            break;
        case 16: This->format = &GUID_WICPixelFormat64bppRGBA; break;
        default:
            ERR("invalid RGBA bit depth: %i\n", bit_depth);
            return E_FAIL;
        }
        break;
    case PNG_COLOR_TYPE_PALETTE:
        This->bpp = bit_depth;
        switch (bit_depth)
        {
        case 1: This->format = &GUID_WICPixelFormat1bppIndexed; break;
        case 2: This->format = &GUID_WICPixelFormat2bppIndexed; break;
        case 4: This->format = &GUID_WICPixelFormat4bppIndexed; break;
        case 8: This->format = &GUID_WICPixelFormat8bppIndexed; break;
        default:
            ERR("invalid indexed color bit depth: %i\n", bit_depth);
            return E_FAIL;
        }
        break;
    case PNG_COLOR_TYPE_RGB:
        This->bpp = bit_depth * 3;
        switch (bit_depth)
        {
        case 8:
            ppng_set_bgr(This->png_ptr);
            This->format = &GUID_WICPixelFormat24bppBGR;
            break;
        case 16: This->format = &GUID_WICPixelFormat48bppRGB; break;
        default:
            ERR("invalid RGB color bit depth: %i\n", bit_depth);
            return E_FAIL;
        }
        break;
    default:
        ERR("invalid color type %i\n", color_type);
        return E_FAIL;
    }

    /* read the image data */
    This->width = ppng_get_image_width(This->png_ptr, This->info_ptr);
    This->height = ppng_get_image_height(This->png_ptr, This->info_ptr);
    This->stride = This->width * This->bpp;
    image_size = This->stride * This->height;

    This->image_bits = HeapAlloc(GetProcessHeap(), 0, image_size);
    if (!This->image_bits) return E_OUTOFMEMORY;

    row_pointers = HeapAlloc(GetProcessHeap(), 0, sizeof(png_bytep)*This->height);
    if (!row_pointers) return E_OUTOFMEMORY;

    for (i=0; i<This->height; i++)
        row_pointers[i] = This->image_bits + i * This->stride;

    ppng_read_image(This->png_ptr, row_pointers);

    HeapFree(GetProcessHeap(), 0, row_pointers);
    row_pointers = NULL;

    ppng_read_end(This->png_ptr, This->end_info);

    This->initialized = TRUE;

    return S_OK;
}

static HRESULT WINAPI PngDecoder_GetContainerFormat(IWICBitmapDecoder *iface,
    GUID *pguidContainerFormat)
{
    memcpy(pguidContainerFormat, &GUID_ContainerFormatPng, sizeof(GUID));
    return S_OK;
}

static HRESULT WINAPI PngDecoder_GetDecoderInfo(IWICBitmapDecoder *iface,
    IWICBitmapDecoderInfo **ppIDecoderInfo)
{
    FIXME("(%p,%p): stub\n", iface, ppIDecoderInfo);
    return E_NOTIMPL;
}

static HRESULT WINAPI PngDecoder_CopyPalette(IWICBitmapDecoder *iface,
    IWICPalette *pIPalette)
{
    FIXME("(%p,%p): stub\n", iface, pIPalette);
    return E_NOTIMPL;
}

static HRESULT WINAPI PngDecoder_GetMetadataQueryReader(IWICBitmapDecoder *iface,
    IWICMetadataQueryReader **ppIMetadataQueryReader)
{
    FIXME("(%p,%p): stub\n", iface, ppIMetadataQueryReader);
    return E_NOTIMPL;
}

static HRESULT WINAPI PngDecoder_GetPreview(IWICBitmapDecoder *iface,
    IWICBitmapSource **ppIBitmapSource)
{
    FIXME("(%p,%p): stub\n", iface, ppIBitmapSource);
    return E_NOTIMPL;
}

static HRESULT WINAPI PngDecoder_GetColorContexts(IWICBitmapDecoder *iface,
    UINT cCount, IWICColorContext **ppIColorContexts, UINT *pcActualCount)
{
    FIXME("(%p,%u,%p,%p)\n", iface, cCount, ppIColorContexts, pcActualCount);
    return E_NOTIMPL;
}

static HRESULT WINAPI PngDecoder_GetThumbnail(IWICBitmapDecoder *iface,
    IWICBitmapSource **ppIThumbnail)
{
    TRACE("(%p,%p)\n", iface, ppIThumbnail);
    return WINCODEC_ERR_CODECNOTHUMBNAIL;
}

static HRESULT WINAPI PngDecoder_GetFrameCount(IWICBitmapDecoder *iface,
    UINT *pCount)
{
    *pCount = 1;
    return S_OK;
}

static HRESULT WINAPI PngDecoder_GetFrame(IWICBitmapDecoder *iface,
    UINT index, IWICBitmapFrameDecode **ppIBitmapFrame)
{
    PngDecoder *This = (PngDecoder*)iface;
    TRACE("(%p,%u,%p)\n", iface, index, ppIBitmapFrame);

    if (!This->initialized) return WINCODEC_ERR_NOTINITIALIZED;

    if (index != 0) return E_INVALIDARG;

    IWICBitmapDecoder_AddRef(iface);

    *ppIBitmapFrame = (void*)(&This->lpFrameVtbl);

    return S_OK;
}

static const IWICBitmapDecoderVtbl PngDecoder_Vtbl = {
    PngDecoder_QueryInterface,
    PngDecoder_AddRef,
    PngDecoder_Release,
    PngDecoder_QueryCapability,
    PngDecoder_Initialize,
    PngDecoder_GetContainerFormat,
    PngDecoder_GetDecoderInfo,
    PngDecoder_CopyPalette,
    PngDecoder_GetMetadataQueryReader,
    PngDecoder_GetPreview,
    PngDecoder_GetColorContexts,
    PngDecoder_GetThumbnail,
    PngDecoder_GetFrameCount,
    PngDecoder_GetFrame
};

static HRESULT WINAPI PngDecoder_Frame_QueryInterface(IWICBitmapFrameDecode *iface, REFIID iid,
    void **ppv)
{
    if (!ppv) return E_INVALIDARG;

    if (IsEqualIID(&IID_IUnknown, iid) ||
        IsEqualIID(&IID_IWICBitmapSource, iid) ||
        IsEqualIID(&IID_IWICBitmapFrameDecode, iid))
    {
        *ppv = iface;
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI PngDecoder_Frame_AddRef(IWICBitmapFrameDecode *iface)
{
    PngDecoder *This = impl_from_frame(iface);
    return IUnknown_AddRef((IUnknown*)This);
}

static ULONG WINAPI PngDecoder_Frame_Release(IWICBitmapFrameDecode *iface)
{
    PngDecoder *This = impl_from_frame(iface);
    return IUnknown_Release((IUnknown*)This);
}

static HRESULT WINAPI PngDecoder_Frame_GetSize(IWICBitmapFrameDecode *iface,
    UINT *puiWidth, UINT *puiHeight)
{
    PngDecoder *This = impl_from_frame(iface);
    *puiWidth = This->width;
    *puiHeight = This->height;
    TRACE("(%p)->(%u,%u)\n", iface, *puiWidth, *puiHeight);
    return S_OK;
}

static HRESULT WINAPI PngDecoder_Frame_GetPixelFormat(IWICBitmapFrameDecode *iface,
    WICPixelFormatGUID *pPixelFormat)
{
    PngDecoder *This = impl_from_frame(iface);
    TRACE("(%p,%p)\n", iface, pPixelFormat);

    memcpy(pPixelFormat, This->format, sizeof(GUID));

    return S_OK;
}

static HRESULT WINAPI PngDecoder_Frame_GetResolution(IWICBitmapFrameDecode *iface,
    double *pDpiX, double *pDpiY)
{
    PngDecoder *This = impl_from_frame(iface);
    png_uint_32 ret, xres, yres;
    int unit_type;

    ret = ppng_get_pHYs(This->png_ptr, This->info_ptr, &xres, &yres, &unit_type);

    if (ret && unit_type == PNG_RESOLUTION_METER)
    {
        *pDpiX = xres * 0.0254;
        *pDpiY = yres * 0.0254;
    }
    else
    {
        WARN("no pHYs block present\n");
        *pDpiX = *pDpiY = 96.0;
    }

    TRACE("(%p)->(%0.2f,%0.2f)\n", iface, *pDpiX, *pDpiY);

    return S_OK;
}

static HRESULT WINAPI PngDecoder_Frame_CopyPalette(IWICBitmapFrameDecode *iface,
    IWICPalette *pIPalette)
{
    PngDecoder *This = impl_from_frame(iface);
    png_uint_32 ret;
    png_colorp png_palette;
    int num_palette;
    WICColor palette[256];
    png_bytep trans;
    int num_trans;
    png_color_16p trans_values;
    int i;

    TRACE("(%p,%p)\n", iface, pIPalette);

    ret = ppng_get_PLTE(This->png_ptr, This->info_ptr, &png_palette, &num_palette);
    if (!ret) return WINCODEC_ERR_PALETTEUNAVAILABLE;

    if (num_palette > 256)
    {
        ERR("palette has %i colors?!\n", num_palette);
        return E_FAIL;
    }

    for (i=0; i<num_palette; i++)
    {
        palette[i] = (0xff000000|
                      png_palette[i].red << 16|
                      png_palette[i].green << 8|
                      png_palette[i].blue);
    }

    ret = ppng_get_tRNS(This->png_ptr, This->info_ptr, &trans, &num_trans, &trans_values);
    if (ret)
    {
        for (i=0; i<num_trans; i++)
        {
            palette[trans[i]] = 0x00000000;
        }
    }

    return IWICPalette_InitializeCustom(pIPalette, palette, num_palette);
}

static HRESULT WINAPI PngDecoder_Frame_CopyPixels(IWICBitmapFrameDecode *iface,
    const WICRect *prc, UINT cbStride, UINT cbBufferSize, BYTE *pbBuffer)
{
    PngDecoder *This = impl_from_frame(iface);
    TRACE("(%p,%p,%u,%u,%p)\n", iface, prc, cbStride, cbBufferSize, pbBuffer);

    return copy_pixels(This->bpp, This->image_bits,
        This->width, This->height, This->stride,
        prc, cbStride, cbBufferSize, pbBuffer);
}

static HRESULT WINAPI PngDecoder_Frame_GetMetadataQueryReader(IWICBitmapFrameDecode *iface,
    IWICMetadataQueryReader **ppIMetadataQueryReader)
{
    FIXME("(%p,%p): stub\n", iface, ppIMetadataQueryReader);
    return E_NOTIMPL;
}

static HRESULT WINAPI PngDecoder_Frame_GetColorContexts(IWICBitmapFrameDecode *iface,
    UINT cCount, IWICColorContext **ppIColorContexts, UINT *pcActualCount)
{
    FIXME("(%p,%u,%p,%p): stub\n", iface, cCount, ppIColorContexts, pcActualCount);
    return E_NOTIMPL;
}

static HRESULT WINAPI PngDecoder_Frame_GetThumbnail(IWICBitmapFrameDecode *iface,
    IWICBitmapSource **ppIThumbnail)
{
    FIXME("(%p,%p): stub\n", iface, ppIThumbnail);
    return E_NOTIMPL;
}

static const IWICBitmapFrameDecodeVtbl PngDecoder_FrameVtbl = {
    PngDecoder_Frame_QueryInterface,
    PngDecoder_Frame_AddRef,
    PngDecoder_Frame_Release,
    PngDecoder_Frame_GetSize,
    PngDecoder_Frame_GetPixelFormat,
    PngDecoder_Frame_GetResolution,
    PngDecoder_Frame_CopyPalette,
    PngDecoder_Frame_CopyPixels,
    PngDecoder_Frame_GetMetadataQueryReader,
    PngDecoder_Frame_GetColorContexts,
    PngDecoder_Frame_GetThumbnail
};

HRESULT PngDecoder_CreateInstance(IUnknown *pUnkOuter, REFIID iid, void** ppv)
{
    PngDecoder *This;
    HRESULT ret;

    TRACE("(%p,%s,%p)\n", pUnkOuter, debugstr_guid(iid), ppv);

    *ppv = NULL;

    if (pUnkOuter) return CLASS_E_NOAGGREGATION;

    if (!libpng_handle && !load_libpng())
    {
        ERR("Failed reading PNG because unable to find %s\n",SONAME_LIBPNG);
        return E_FAIL;
    }

    This = HeapAlloc(GetProcessHeap(), 0, sizeof(PngDecoder));
    if (!This) return E_OUTOFMEMORY;

    This->lpVtbl = &PngDecoder_Vtbl;
    This->lpFrameVtbl = &PngDecoder_FrameVtbl;
    This->ref = 1;
    This->png_ptr = NULL;
    This->info_ptr = NULL;
    This->end_info = NULL;
    This->initialized = FALSE;
    This->image_bits = NULL;

    ret = IUnknown_QueryInterface((IUnknown*)This, iid, ppv);
    IUnknown_Release((IUnknown*)This);

    return ret;
}

struct png_pixelformat {
    const WICPixelFormatGUID *guid;
    UINT bpp;
    int bit_depth;
    int color_type;
    BOOL remove_filler;
    BOOL swap_rgb;
};

static const struct png_pixelformat formats[] = {
    {&GUID_WICPixelFormat24bppBGR, 24, 8, PNG_COLOR_TYPE_RGB, 0, 1},
    {&GUID_WICPixelFormatBlackWhite, 1, 1, PNG_COLOR_TYPE_GRAY, 0, 0},
    {&GUID_WICPixelFormat2bppGray, 2, 2, PNG_COLOR_TYPE_GRAY, 0, 0},
    {&GUID_WICPixelFormat4bppGray, 4, 4, PNG_COLOR_TYPE_GRAY, 0, 0},
    {&GUID_WICPixelFormat8bppGray, 8, 8, PNG_COLOR_TYPE_GRAY, 0, 0},
    {&GUID_WICPixelFormat16bppGray, 16, 16, PNG_COLOR_TYPE_GRAY, 0, 0},
    {&GUID_WICPixelFormat32bppBGR, 32, 8, PNG_COLOR_TYPE_RGB, 1, 1},
    {&GUID_WICPixelFormat32bppBGRA, 32, 8, PNG_COLOR_TYPE_RGB_ALPHA, 0, 1},
    {&GUID_WICPixelFormat48bppRGB, 48, 16, PNG_COLOR_TYPE_RGB, 0, 0},
    {&GUID_WICPixelFormat64bppRGBA, 64, 16, PNG_COLOR_TYPE_RGB_ALPHA, 0, 0},
    {NULL},
};

typedef struct PngEncoder {
    const IWICBitmapEncoderVtbl *lpVtbl;
    const IWICBitmapFrameEncodeVtbl *lpFrameVtbl;
    LONG ref;
    IStream *stream;
    png_structp png_ptr;
    png_infop info_ptr;
    UINT frame_count;
    BOOL frame_initialized;
    const struct png_pixelformat *format;
    BOOL info_written;
    UINT width, height;
    double xres, yres;
    UINT lines_written;
    BOOL frame_committed;
    BOOL committed;
} PngEncoder;

static inline PngEncoder *encoder_from_frame(IWICBitmapFrameEncode *iface)
{
    return CONTAINING_RECORD(iface, PngEncoder, lpFrameVtbl);
}

static HRESULT WINAPI PngFrameEncode_QueryInterface(IWICBitmapFrameEncode *iface, REFIID iid,
    void **ppv)
{
    PngEncoder *This = encoder_from_frame(iface);
    TRACE("(%p,%s,%p)\n", iface, debugstr_guid(iid), ppv);

    if (!ppv) return E_INVALIDARG;

    if (IsEqualIID(&IID_IUnknown, iid) ||
        IsEqualIID(&IID_IWICBitmapFrameEncode, iid))
    {
        *ppv = &This->lpFrameVtbl;
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI PngFrameEncode_AddRef(IWICBitmapFrameEncode *iface)
{
    PngEncoder *This = encoder_from_frame(iface);
    return IUnknown_AddRef((IUnknown*)This);
}

static ULONG WINAPI PngFrameEncode_Release(IWICBitmapFrameEncode *iface)
{
    PngEncoder *This = encoder_from_frame(iface);
    return IUnknown_Release((IUnknown*)This);
}

static HRESULT WINAPI PngFrameEncode_Initialize(IWICBitmapFrameEncode *iface,
    IPropertyBag2 *pIEncoderOptions)
{
    PngEncoder *This = encoder_from_frame(iface);
    TRACE("(%p,%p)\n", iface, pIEncoderOptions);

    if (This->frame_initialized) return WINCODEC_ERR_WRONGSTATE;

    This->frame_initialized = TRUE;

    return S_OK;
}

static HRESULT WINAPI PngFrameEncode_SetSize(IWICBitmapFrameEncode *iface,
    UINT uiWidth, UINT uiHeight)
{
    PngEncoder *This = encoder_from_frame(iface);
    TRACE("(%p,%u,%u)\n", iface, uiWidth, uiHeight);

    if (!This->frame_initialized || This->info_written) return WINCODEC_ERR_WRONGSTATE;

    This->width = uiWidth;
    This->height = uiHeight;

    return S_OK;
}

static HRESULT WINAPI PngFrameEncode_SetResolution(IWICBitmapFrameEncode *iface,
    double dpiX, double dpiY)
{
    PngEncoder *This = encoder_from_frame(iface);
    TRACE("(%p,%0.2f,%0.2f)\n", iface, dpiX, dpiY);

    if (!This->frame_initialized || This->info_written) return WINCODEC_ERR_WRONGSTATE;

    This->xres = dpiX;
    This->yres = dpiY;

    return S_OK;
}

static HRESULT WINAPI PngFrameEncode_SetPixelFormat(IWICBitmapFrameEncode *iface,
    WICPixelFormatGUID *pPixelFormat)
{
    PngEncoder *This = encoder_from_frame(iface);
    int i;
    TRACE("(%p,%s)\n", iface, debugstr_guid(pPixelFormat));

    if (!This->frame_initialized || This->info_written) return WINCODEC_ERR_WRONGSTATE;

    for (i=0; formats[i].guid; i++)
    {
        if (memcmp(formats[i].guid, pPixelFormat, sizeof(GUID)) == 0)
            break;
    }

    if (!formats[i].guid) i = 0;

    This->format = &formats[i];
    memcpy(pPixelFormat, This->format->guid, sizeof(GUID));

    return S_OK;
}

static HRESULT WINAPI PngFrameEncode_SetColorContexts(IWICBitmapFrameEncode *iface,
    UINT cCount, IWICColorContext **ppIColorContext)
{
    FIXME("(%p,%u,%p): stub\n", iface, cCount, ppIColorContext);
    return E_NOTIMPL;
}

static HRESULT WINAPI PngFrameEncode_SetPalette(IWICBitmapFrameEncode *iface,
    IWICPalette *pIPalette)
{
    FIXME("(%p,%p): stub\n", iface, pIPalette);
    return WINCODEC_ERR_UNSUPPORTEDOPERATION;
}

static HRESULT WINAPI PngFrameEncode_SetThumbnail(IWICBitmapFrameEncode *iface,
    IWICBitmapSource *pIThumbnail)
{
    FIXME("(%p,%p): stub\n", iface, pIThumbnail);
    return WINCODEC_ERR_UNSUPPORTEDOPERATION;
}

static HRESULT WINAPI PngFrameEncode_WritePixels(IWICBitmapFrameEncode *iface,
    UINT lineCount, UINT cbStride, UINT cbBufferSize, BYTE *pbPixels)
{
    PngEncoder *This = encoder_from_frame(iface);
    png_byte **row_pointers=NULL;
    UINT i;
    TRACE("(%p,%u,%u,%u,%p)\n", iface, lineCount, cbStride, cbBufferSize, pbPixels);

    if (!This->frame_initialized || !This->width || !This->height || !This->format)
        return WINCODEC_ERR_WRONGSTATE;

    if (lineCount == 0 || lineCount + This->lines_written > This->height)
        return E_INVALIDARG;

    /* set up setjmp/longjmp error handling */
    if (setjmp(png_jmpbuf(This->png_ptr)))
    {
        HeapFree(GetProcessHeap(), 0, row_pointers);
        return E_FAIL;
    }

    if (!This->info_written)
    {
        ppng_set_IHDR(This->png_ptr, This->info_ptr, This->width, This->height,
            This->format->bit_depth, This->format->color_type, PNG_INTERLACE_NONE,
            PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

        if (This->xres != 0.0 && This->yres != 0.0)
        {
            ppng_set_pHYs(This->png_ptr, This->info_ptr, (This->xres+0.0127) / 0.0254,
                (This->yres+0.0127) / 0.0254, PNG_RESOLUTION_METER);
        }

        ppng_write_info(This->png_ptr, This->info_ptr);

        if (This->format->remove_filler)
            ppng_set_filler(This->png_ptr, 0, PNG_FILLER_AFTER);

        if (This->format->swap_rgb)
            ppng_set_bgr(This->png_ptr);

        This->info_written = TRUE;
    }

    row_pointers = HeapAlloc(GetProcessHeap(), 0, lineCount * sizeof(png_byte*));
    if (!row_pointers)
        return E_OUTOFMEMORY;

    for (i=0; i<lineCount; i++)
        row_pointers[i] = pbPixels + cbStride * i;

    ppng_write_rows(This->png_ptr, row_pointers, lineCount);
    This->lines_written += lineCount;

    HeapFree(GetProcessHeap(), 0, row_pointers);

    return S_OK;
}

static HRESULT WINAPI PngFrameEncode_WriteSource(IWICBitmapFrameEncode *iface,
    IWICBitmapSource *pIBitmapSource, WICRect *prc)
{
    PngEncoder *This = encoder_from_frame(iface);
    HRESULT hr;
    WICRect rc;
    WICPixelFormatGUID guid;
    UINT stride;
    BYTE *pixeldata;
    TRACE("(%p,%p,%p)\n", iface, pIBitmapSource, prc);

    if (!This->frame_initialized || !This->width || !This->height)
        return WINCODEC_ERR_WRONGSTATE;

    if (!This->format)
    {
        hr = IWICBitmapSource_GetPixelFormat(pIBitmapSource, &guid);
        if (FAILED(hr)) return hr;
        hr = IWICBitmapFrameEncode_SetPixelFormat(iface, &guid);
        if (FAILED(hr)) return hr;
    }

    hr = IWICBitmapSource_GetPixelFormat(pIBitmapSource, &guid);
    if (FAILED(hr)) return hr;
    if (memcmp(&guid, This->format->guid, sizeof(GUID)) != 0)
    {
        /* FIXME: should use WICConvertBitmapSource to convert */
        ERR("format %s unsupported\n", debugstr_guid(&guid));
        return E_FAIL;
    }

    if (This->xres == 0.0 || This->yres == 0.0)
    {
        double xres, yres;
        hr = IWICBitmapSource_GetResolution(pIBitmapSource, &xres, &yres);
        if (FAILED(hr)) return hr;
        hr = IWICBitmapFrameEncode_SetResolution(iface, xres, yres);
        if (FAILED(hr)) return hr;
    }

    if (!prc)
    {
        UINT width, height;
        hr = IWICBitmapSource_GetSize(pIBitmapSource, &width, &height);
        if (FAILED(hr)) return hr;
        rc.X = 0;
        rc.Y = 0;
        rc.Width = width;
        rc.Height = height;
        prc = &rc;
    }

    if (prc->Width != This->width) return E_INVALIDARG;

    stride = (This->format->bpp * This->width + 7)/8;

    pixeldata = HeapAlloc(GetProcessHeap(), 0, stride * prc->Height);
    if (!pixeldata) return E_OUTOFMEMORY;

    hr = IWICBitmapSource_CopyPixels(pIBitmapSource, prc, stride,
        stride*prc->Height, pixeldata);

    if (SUCCEEDED(hr))
    {
        hr = IWICBitmapFrameEncode_WritePixels(iface, prc->Height, stride,
            stride*prc->Height, pixeldata);
    }

    HeapFree(GetProcessHeap(), 0, pixeldata);

    return hr;
}

static HRESULT WINAPI PngFrameEncode_Commit(IWICBitmapFrameEncode *iface)
{
    PngEncoder *This = encoder_from_frame(iface);
    TRACE("(%p)\n", iface);

    if (!This->info_written || This->lines_written != This->height || This->frame_committed)
        return WINCODEC_ERR_WRONGSTATE;

    /* set up setjmp/longjmp error handling */
    if (setjmp(png_jmpbuf(This->png_ptr)))
    {
        return E_FAIL;
    }

    ppng_write_end(This->png_ptr, This->info_ptr);

    This->frame_committed = TRUE;

    return S_OK;
}

static HRESULT WINAPI PngFrameEncode_GetMetadataQueryWriter(IWICBitmapFrameEncode *iface,
    IWICMetadataQueryWriter **ppIMetadataQueryWriter)
{
    FIXME("(%p, %p): stub\n", iface, ppIMetadataQueryWriter);
    return E_NOTIMPL;
}

static const IWICBitmapFrameEncodeVtbl PngEncoder_FrameVtbl = {
    PngFrameEncode_QueryInterface,
    PngFrameEncode_AddRef,
    PngFrameEncode_Release,
    PngFrameEncode_Initialize,
    PngFrameEncode_SetSize,
    PngFrameEncode_SetResolution,
    PngFrameEncode_SetPixelFormat,
    PngFrameEncode_SetColorContexts,
    PngFrameEncode_SetPalette,
    PngFrameEncode_SetThumbnail,
    PngFrameEncode_WritePixels,
    PngFrameEncode_WriteSource,
    PngFrameEncode_Commit,
    PngFrameEncode_GetMetadataQueryWriter
};

static HRESULT WINAPI PngEncoder_QueryInterface(IWICBitmapEncoder *iface, REFIID iid,
    void **ppv)
{
    PngEncoder *This = (PngEncoder*)iface;
    TRACE("(%p,%s,%p)\n", iface, debugstr_guid(iid), ppv);

    if (!ppv) return E_INVALIDARG;

    if (IsEqualIID(&IID_IUnknown, iid) ||
        IsEqualIID(&IID_IWICBitmapEncoder, iid))
    {
        *ppv = This;
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI PngEncoder_AddRef(IWICBitmapEncoder *iface)
{
    PngEncoder *This = (PngEncoder*)iface;
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) refcount=%u\n", iface, ref);

    return ref;
}

static ULONG WINAPI PngEncoder_Release(IWICBitmapEncoder *iface)
{
    PngEncoder *This = (PngEncoder*)iface;
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) refcount=%u\n", iface, ref);

    if (ref == 0)
    {
        if (This->png_ptr)
            ppng_destroy_write_struct(&This->png_ptr, &This->info_ptr);
        if (This->stream)
            IStream_Release(This->stream);
        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}

static void user_write_data(png_structp png_ptr, png_bytep data, png_size_t length)
{
    PngEncoder *This = ppng_get_io_ptr(png_ptr);
    HRESULT hr;
    ULONG byteswritten;

    hr = IStream_Write(This->stream, data, length, &byteswritten);
    if (FAILED(hr) || byteswritten != length)
    {
        ppng_error(png_ptr, "failed writing data");
    }
}

static void user_flush(png_structp png_ptr)
{
}

static HRESULT WINAPI PngEncoder_Initialize(IWICBitmapEncoder *iface,
    IStream *pIStream, WICBitmapEncoderCacheOption cacheOption)
{
    PngEncoder *This = (PngEncoder*)iface;

    TRACE("(%p,%p,%u)\n", iface, pIStream, cacheOption);

    if (This->png_ptr)
        return WINCODEC_ERR_WRONGSTATE;

    /* initialize libpng */
    This->png_ptr = ppng_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!This->png_ptr)
        return E_FAIL;

    This->info_ptr = ppng_create_info_struct(This->png_ptr);
    if (!This->info_ptr)
    {
        ppng_destroy_write_struct(&This->png_ptr, NULL);
        This->png_ptr = NULL;
        return E_FAIL;
    }

    IStream_AddRef(pIStream);
    This->stream = pIStream;

    /* set up setjmp/longjmp error handling */
    if (setjmp(png_jmpbuf(This->png_ptr)))
    {
        ppng_destroy_write_struct(&This->png_ptr, &This->info_ptr);
        This->png_ptr = NULL;
        IStream_Release(This->stream);
        This->stream = NULL;
        return E_FAIL;
    }

    /* set up custom i/o handling */
    ppng_set_write_fn(This->png_ptr, This, user_write_data, user_flush);

    return S_OK;
}

static HRESULT WINAPI PngEncoder_GetContainerFormat(IWICBitmapEncoder *iface,
    GUID *pguidContainerFormat)
{
    FIXME("(%p,%s): stub\n", iface, debugstr_guid(pguidContainerFormat));
    return E_NOTIMPL;
}

static HRESULT WINAPI PngEncoder_GetEncoderInfo(IWICBitmapEncoder *iface,
    IWICBitmapEncoderInfo **ppIEncoderInfo)
{
    FIXME("(%p,%p): stub\n", iface, ppIEncoderInfo);
    return E_NOTIMPL;
}

static HRESULT WINAPI PngEncoder_SetColorContexts(IWICBitmapEncoder *iface,
    UINT cCount, IWICColorContext **ppIColorContext)
{
    FIXME("(%p,%u,%p): stub\n", iface, cCount, ppIColorContext);
    return E_NOTIMPL;
}

static HRESULT WINAPI PngEncoder_SetPalette(IWICBitmapEncoder *iface, IWICPalette *pIPalette)
{
    TRACE("(%p,%p)\n", iface, pIPalette);
    return WINCODEC_ERR_UNSUPPORTEDOPERATION;
}

static HRESULT WINAPI PngEncoder_SetThumbnail(IWICBitmapEncoder *iface, IWICBitmapSource *pIThumbnail)
{
    TRACE("(%p,%p)\n", iface, pIThumbnail);
    return WINCODEC_ERR_UNSUPPORTEDOPERATION;
}

static HRESULT WINAPI PngEncoder_SetPreview(IWICBitmapEncoder *iface, IWICBitmapSource *pIPreview)
{
    TRACE("(%p,%p)\n", iface, pIPreview);
    return WINCODEC_ERR_UNSUPPORTEDOPERATION;
}

static HRESULT WINAPI PngEncoder_CreateNewFrame(IWICBitmapEncoder *iface,
    IWICBitmapFrameEncode **ppIFrameEncode, IPropertyBag2 **ppIEncoderOptions)
{
    PngEncoder *This = (PngEncoder*)iface;
    HRESULT hr;
    TRACE("(%p,%p,%p)\n", iface, ppIFrameEncode, ppIEncoderOptions);

    if (This->frame_count != 0)
        return WINCODEC_ERR_UNSUPPORTEDOPERATION;

    if (!This->stream)
        return WINCODEC_ERR_NOTINITIALIZED;

    hr = CreatePropertyBag2(ppIEncoderOptions);
    if (FAILED(hr)) return hr;

    This->frame_count = 1;

    IWICBitmapEncoder_AddRef(iface);
    *ppIFrameEncode = (IWICBitmapFrameEncode*)&This->lpFrameVtbl;

    return S_OK;
}

static HRESULT WINAPI PngEncoder_Commit(IWICBitmapEncoder *iface)
{
    PngEncoder *This = (PngEncoder*)iface;
    TRACE("(%p)\n", iface);

    if (!This->frame_committed || This->committed)
        return WINCODEC_ERR_WRONGSTATE;

    This->committed = TRUE;

    return S_OK;
}

static HRESULT WINAPI PngEncoder_GetMetadataQueryWriter(IWICBitmapEncoder *iface,
    IWICMetadataQueryWriter **ppIMetadataQueryWriter)
{
    FIXME("(%p,%p): stub\n", iface, ppIMetadataQueryWriter);
    return E_NOTIMPL;
}

static const IWICBitmapEncoderVtbl PngEncoder_Vtbl = {
    PngEncoder_QueryInterface,
    PngEncoder_AddRef,
    PngEncoder_Release,
    PngEncoder_Initialize,
    PngEncoder_GetContainerFormat,
    PngEncoder_GetEncoderInfo,
    PngEncoder_SetColorContexts,
    PngEncoder_SetPalette,
    PngEncoder_SetThumbnail,
    PngEncoder_SetPreview,
    PngEncoder_CreateNewFrame,
    PngEncoder_Commit,
    PngEncoder_GetMetadataQueryWriter
};

HRESULT PngEncoder_CreateInstance(IUnknown *pUnkOuter, REFIID iid, void** ppv)
{
    PngEncoder *This;
    HRESULT ret;

    TRACE("(%p,%s,%p)\n", pUnkOuter, debugstr_guid(iid), ppv);

    *ppv = NULL;

    if (pUnkOuter) return CLASS_E_NOAGGREGATION;

    if (!libpng_handle && !load_libpng())
    {
        ERR("Failed writing PNG because unable to find %s\n",SONAME_LIBPNG);
        return E_FAIL;
    }

    This = HeapAlloc(GetProcessHeap(), 0, sizeof(PngEncoder));
    if (!This) return E_OUTOFMEMORY;

    This->lpVtbl = &PngEncoder_Vtbl;
    This->lpFrameVtbl = &PngEncoder_FrameVtbl;
    This->ref = 1;
    This->png_ptr = NULL;
    This->info_ptr = NULL;
    This->stream = NULL;
    This->frame_count = 0;
    This->frame_initialized = FALSE;
    This->format = NULL;
    This->info_written = FALSE;
    This->width = 0;
    This->height = 0;
    This->xres = 0.0;
    This->yres = 0.0;
    This->lines_written = 0;
    This->frame_committed = FALSE;
    This->committed = FALSE;

    ret = IUnknown_QueryInterface((IUnknown*)This, iid, ppv);
    IUnknown_Release((IUnknown*)This);

    return ret;
}

#else /* !HAVE_PNG_H */

HRESULT PngDecoder_CreateInstance(IUnknown *pUnkOuter, REFIID iid, void** ppv)
{
    ERR("Trying to load PNG picture, but PNG supported not compiled in.\n");
    return E_FAIL;
}

HRESULT PngEncoder_CreateInstance(IUnknown *pUnkOuter, REFIID iid, void** ppv)
{
    ERR("Trying to save PNG picture, but PNG supported not compiled in.\n");
    return E_FAIL;
}

#endif
