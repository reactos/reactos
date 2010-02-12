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
MAKE_FUNCPTR(png_destroy_read_struct);
MAKE_FUNCPTR(png_error);
MAKE_FUNCPTR(png_get_bit_depth);
MAKE_FUNCPTR(png_get_color_type);
MAKE_FUNCPTR(png_get_image_height);
MAKE_FUNCPTR(png_get_image_width);
MAKE_FUNCPTR(png_get_io_ptr);
MAKE_FUNCPTR(png_get_PLTE);
MAKE_FUNCPTR(png_get_tRNS);
MAKE_FUNCPTR(png_set_bgr);
MAKE_FUNCPTR(png_set_gray_1_2_4_to_8);
MAKE_FUNCPTR(png_set_gray_to_rgb);
MAKE_FUNCPTR(png_set_read_fn);
MAKE_FUNCPTR(png_set_strip_16);
MAKE_FUNCPTR(png_set_tRNS_to_alpha);
MAKE_FUNCPTR(png_read_end);
MAKE_FUNCPTR(png_read_image);
MAKE_FUNCPTR(png_read_info);
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
        LOAD_FUNCPTR(png_destroy_read_struct);
        LOAD_FUNCPTR(png_error);
        LOAD_FUNCPTR(png_get_bit_depth);
        LOAD_FUNCPTR(png_get_color_type);
        LOAD_FUNCPTR(png_get_image_height);
        LOAD_FUNCPTR(png_get_image_width);
        LOAD_FUNCPTR(png_get_io_ptr);
        LOAD_FUNCPTR(png_get_PLTE);
        LOAD_FUNCPTR(png_get_tRNS);
        LOAD_FUNCPTR(png_set_bgr);
        LOAD_FUNCPTR(png_set_gray_1_2_4_to_8);
        LOAD_FUNCPTR(png_set_gray_to_rgb);
        LOAD_FUNCPTR(png_set_read_fn);
        LOAD_FUNCPTR(png_set_strip_16);
        LOAD_FUNCPTR(png_set_tRNS_to_alpha);
        LOAD_FUNCPTR(png_read_end);
        LOAD_FUNCPTR(png_read_image);
        LOAD_FUNCPTR(png_read_info);

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
        ppng_destroy_read_struct(&This->png_ptr, (png_infopp)NULL, (png_infopp)NULL);
        This->png_ptr = NULL;
        return E_FAIL;
    }

    This->end_info = ppng_create_info_struct(This->png_ptr);
    if (!This->info_ptr)
    {
        ppng_destroy_read_struct(&This->png_ptr, &This->info_ptr, (png_infopp)NULL);
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
    FIXME("(%p,%p,%p): stub\n", iface, pDpiX, pDpiY);
    return E_NOTIMPL;
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

#else /* !HAVE_PNG_H */

HRESULT PngDecoder_CreateInstance(IUnknown *pUnkOuter, REFIID iid, void** ppv)
{
    ERR("Trying to load PNG picture, but PNG supported not compiled in.\n");
    return E_FAIL;
}

#endif
