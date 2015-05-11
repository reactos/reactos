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

#ifdef HAVE_PNG_H
#include <png.h>
#endif

static const WCHAR wszPngInterlaceOption[] = {'I','n','t','e','r','l','a','c','e','O','p','t','i','o','n',0};

static HRESULT read_png_chunk(IStream *stream, BYTE *type, BYTE **data, ULONG *data_size)
{
    BYTE header[8];
    HRESULT hr;
    ULONG bytesread;

    hr = IStream_Read(stream, header, 8, &bytesread);
    if (FAILED(hr) || bytesread < 8)
    {
        if (SUCCEEDED(hr))
            hr = E_FAIL;
        return hr;
    }

    *data_size = header[0] << 24 | header[1] << 16 | header[2] << 8 | header[3];

    memcpy(type, &header[4], 4);

    if (data)
    {
        *data = HeapAlloc(GetProcessHeap(), 0, *data_size);
        if (!*data)
            return E_OUTOFMEMORY;

        hr = IStream_Read(stream, *data, *data_size, &bytesread);

        if (FAILED(hr) || bytesread < *data_size)
        {
            if (SUCCEEDED(hr))
                hr = E_FAIL;
            HeapFree(GetProcessHeap(), 0, *data);
            *data = NULL;
            return hr;
        }

        /* Windows ignores CRC of the chunk */
    }

    return S_OK;
}

static HRESULT LoadTextMetadata(IStream *stream, const GUID *preferred_vendor,
    DWORD persist_options, MetadataItem **items, DWORD *item_count)
{
    HRESULT hr;
    BYTE type[4];
    BYTE *data;
    ULONG data_size;
    ULONG name_len, value_len;
    BYTE *name_end_ptr;
    LPSTR name, value;
    MetadataItem *result;

    hr = read_png_chunk(stream, type, &data, &data_size);
    if (FAILED(hr)) return hr;

    name_end_ptr = memchr(data, 0, data_size);

    name_len = name_end_ptr - data;

    if (!name_end_ptr || name_len > 79)
    {
        HeapFree(GetProcessHeap(), 0, data);
        return E_FAIL;
    }

    value_len = data_size - name_len - 1;

    result = HeapAlloc(GetProcessHeap(), 0, sizeof(MetadataItem));
    name = HeapAlloc(GetProcessHeap(), 0, name_len + 1);
    value = HeapAlloc(GetProcessHeap(), 0, value_len + 1);
    if (!result || !name || !value)
    {
        HeapFree(GetProcessHeap(), 0, data);
        HeapFree(GetProcessHeap(), 0, result);
        HeapFree(GetProcessHeap(), 0, name);
        HeapFree(GetProcessHeap(), 0, value);
        return E_OUTOFMEMORY;
    }

    PropVariantInit(&result[0].schema);
    PropVariantInit(&result[0].id);
    PropVariantInit(&result[0].value);

    memcpy(name, data, name_len + 1);
    memcpy(value, name_end_ptr + 1, value_len);
    value[value_len] = 0;

    result[0].id.vt = VT_LPSTR;
    result[0].id.u.pszVal = name;
    result[0].value.vt = VT_LPSTR;
    result[0].value.u.pszVal = value;

    *items = result;
    *item_count = 1;

    HeapFree(GetProcessHeap(), 0, data);

    return S_OK;
}

static const MetadataHandlerVtbl TextReader_Vtbl = {
    0,
    &CLSID_WICPngTextMetadataReader,
    LoadTextMetadata
};

HRESULT PngTextReader_CreateInstance(REFIID iid, void** ppv)
{
    return MetadataReader_Create(&TextReader_Vtbl, iid, ppv);
}

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
MAKE_FUNCPTR(png_get_error_ptr);
MAKE_FUNCPTR(png_get_iCCP);
MAKE_FUNCPTR(png_get_image_height);
MAKE_FUNCPTR(png_get_image_width);
MAKE_FUNCPTR(png_get_io_ptr);
MAKE_FUNCPTR(png_get_pHYs);
MAKE_FUNCPTR(png_get_PLTE);
MAKE_FUNCPTR(png_get_tRNS);
MAKE_FUNCPTR(png_set_bgr);
MAKE_FUNCPTR(png_set_crc_action);
MAKE_FUNCPTR(png_set_error_fn);
#ifdef HAVE_PNG_SET_EXPAND_GRAY_1_2_4_TO_8
MAKE_FUNCPTR(png_set_expand_gray_1_2_4_to_8);
#else
MAKE_FUNCPTR(png_set_gray_1_2_4_to_8);
#endif
MAKE_FUNCPTR(png_set_filler);
MAKE_FUNCPTR(png_set_gray_to_rgb);
MAKE_FUNCPTR(png_set_interlace_handling);
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

static CRITICAL_SECTION init_png_cs;
static CRITICAL_SECTION_DEBUG init_png_cs_debug =
{
    0, 0, &init_png_cs,
    { &init_png_cs_debug.ProcessLocksList,
      &init_png_cs_debug.ProcessLocksList },
    0, 0, { (DWORD_PTR)(__FILE__ ": init_png_cs") }
};
static CRITICAL_SECTION init_png_cs = { &init_png_cs_debug, -1, 0, 0, 0, 0 };

static void *load_libpng(void)
{
    void *result;

    EnterCriticalSection(&init_png_cs);

    if(!libpng_handle && (libpng_handle = wine_dlopen(SONAME_LIBPNG, RTLD_NOW, NULL, 0)) != NULL) {

#define LOAD_FUNCPTR(f) \
    if((p##f = wine_dlsym(libpng_handle, #f, NULL, 0)) == NULL) { \
        libpng_handle = NULL; \
        LeaveCriticalSection(&init_png_cs); \
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
        LOAD_FUNCPTR(png_get_error_ptr);
        LOAD_FUNCPTR(png_get_iCCP);
        LOAD_FUNCPTR(png_get_image_height);
        LOAD_FUNCPTR(png_get_image_width);
        LOAD_FUNCPTR(png_get_io_ptr);
        LOAD_FUNCPTR(png_get_pHYs);
        LOAD_FUNCPTR(png_get_PLTE);
        LOAD_FUNCPTR(png_get_tRNS);
        LOAD_FUNCPTR(png_set_bgr);
        LOAD_FUNCPTR(png_set_crc_action);
        LOAD_FUNCPTR(png_set_error_fn);
#ifdef HAVE_PNG_SET_EXPAND_GRAY_1_2_4_TO_8
        LOAD_FUNCPTR(png_set_expand_gray_1_2_4_to_8);
#else
        LOAD_FUNCPTR(png_set_gray_1_2_4_to_8);
#endif
        LOAD_FUNCPTR(png_set_filler);
        LOAD_FUNCPTR(png_set_gray_to_rgb);
        LOAD_FUNCPTR(png_set_interlace_handling);
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

    result = libpng_handle;

    LeaveCriticalSection(&init_png_cs);

    return result;
}

static void user_error_fn(png_structp png_ptr, png_const_charp error_message)
{
    jmp_buf *pjmpbuf;

    /* This uses setjmp/longjmp just like the default. We can't use the
     * default because there's no way to access the jmp buffer in the png_struct
     * that works in 1.2 and 1.4 and allows us to dynamically load libpng. */
    WARN("PNG error: %s\n", debugstr_a(error_message));
    pjmpbuf = ppng_get_error_ptr(png_ptr);
    longjmp(*pjmpbuf, 1);
}

static void user_warning_fn(png_structp png_ptr, png_const_charp warning_message)
{
    WARN("PNG warning: %s\n", debugstr_a(warning_message));
}

typedef struct {
    IWICBitmapDecoder IWICBitmapDecoder_iface;
    IWICBitmapFrameDecode IWICBitmapFrameDecode_iface;
    IWICMetadataBlockReader IWICMetadataBlockReader_iface;
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
    CRITICAL_SECTION lock; /* must be held when png structures are accessed or initialized is set */
} PngDecoder;

static inline PngDecoder *impl_from_IWICBitmapDecoder(IWICBitmapDecoder *iface)
{
    return CONTAINING_RECORD(iface, PngDecoder, IWICBitmapDecoder_iface);
}

static inline PngDecoder *impl_from_IWICBitmapFrameDecode(IWICBitmapFrameDecode *iface)
{
    return CONTAINING_RECORD(iface, PngDecoder, IWICBitmapFrameDecode_iface);
}

static inline PngDecoder *impl_from_IWICMetadataBlockReader(IWICMetadataBlockReader *iface)
{
    return CONTAINING_RECORD(iface, PngDecoder, IWICMetadataBlockReader_iface);
}

static const IWICBitmapFrameDecodeVtbl PngDecoder_FrameVtbl;

static HRESULT WINAPI PngDecoder_QueryInterface(IWICBitmapDecoder *iface, REFIID iid,
    void **ppv)
{
    PngDecoder *This = impl_from_IWICBitmapDecoder(iface);
    TRACE("(%p,%s,%p)\n", iface, debugstr_guid(iid), ppv);

    if (!ppv) return E_INVALIDARG;

    if (IsEqualIID(&IID_IUnknown, iid) || IsEqualIID(&IID_IWICBitmapDecoder, iid))
    {
        *ppv = &This->IWICBitmapDecoder_iface;
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
    PngDecoder *This = impl_from_IWICBitmapDecoder(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) refcount=%u\n", iface, ref);

    return ref;
}

static ULONG WINAPI PngDecoder_Release(IWICBitmapDecoder *iface)
{
    PngDecoder *This = impl_from_IWICBitmapDecoder(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) refcount=%u\n", iface, ref);

    if (ref == 0)
    {
        if (This->png_ptr)
            ppng_destroy_read_struct(&This->png_ptr, &This->info_ptr, &This->end_info);
        This->lock.DebugInfo->Spare[0] = 0;
        DeleteCriticalSection(&This->lock);
        HeapFree(GetProcessHeap(), 0, This->image_bits);
        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}

static HRESULT WINAPI PngDecoder_QueryCapability(IWICBitmapDecoder *iface, IStream *stream,
    DWORD *capability)
{
    HRESULT hr;

    TRACE("(%p,%p,%p)\n", iface, stream, capability);

    if (!stream || !capability) return E_INVALIDARG;

    hr = IWICBitmapDecoder_Initialize(iface, stream, WICDecodeMetadataCacheOnDemand);
    if (hr != S_OK) return hr;

    *capability = WICBitmapDecoderCapabilityCanDecodeAllImages |
                  WICBitmapDecoderCapabilityCanDecodeSomeImages;
    /* FIXME: WICBitmapDecoderCapabilityCanEnumerateMetadata */
    return S_OK;
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
    PngDecoder *This = impl_from_IWICBitmapDecoder(iface);
    LARGE_INTEGER seek;
    HRESULT hr=S_OK;
    png_bytep *row_pointers=NULL;
    UINT image_size;
    UINT i;
    int color_type, bit_depth;
    png_bytep trans;
    int num_trans;
    png_uint_32 transparency;
    png_color_16p trans_values;
    jmp_buf jmpbuf;

    TRACE("(%p,%p,%x)\n", iface, pIStream, cacheOptions);

    EnterCriticalSection(&This->lock);

    /* initialize libpng */
    This->png_ptr = ppng_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!This->png_ptr)
    {
        hr = E_FAIL;
        goto end;
    }

    This->info_ptr = ppng_create_info_struct(This->png_ptr);
    if (!This->info_ptr)
    {
        ppng_destroy_read_struct(&This->png_ptr, NULL, NULL);
        This->png_ptr = NULL;
        hr = E_FAIL;
        goto end;
    }

    This->end_info = ppng_create_info_struct(This->png_ptr);
    if (!This->info_ptr)
    {
        ppng_destroy_read_struct(&This->png_ptr, &This->info_ptr, NULL);
        This->png_ptr = NULL;
        hr = E_FAIL;
        goto end;
    }

    /* set up setjmp/longjmp error handling */
    if (setjmp(jmpbuf))
    {
        ppng_destroy_read_struct(&This->png_ptr, &This->info_ptr, &This->end_info);
        HeapFree(GetProcessHeap(), 0, row_pointers);
        This->png_ptr = NULL;
        hr = E_FAIL;
        goto end;
    }
    ppng_set_error_fn(This->png_ptr, jmpbuf, user_error_fn, user_warning_fn);
    ppng_set_crc_action(This->png_ptr, PNG_CRC_QUIET_USE, PNG_CRC_QUIET_USE);

    /* seek to the start of the stream */
    seek.QuadPart = 0;
    hr = IStream_Seek(pIStream, seek, STREAM_SEEK_SET, NULL);
    if (FAILED(hr)) goto end;

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
#ifdef HAVE_PNG_SET_EXPAND_GRAY_1_2_4_TO_8
                ppng_set_expand_gray_1_2_4_to_8(This->png_ptr);
#else
                ppng_set_gray_1_2_4_to_8(This->png_ptr);
#endif
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
            hr = E_FAIL;
            goto end;
        }
        break;
    case PNG_COLOR_TYPE_GRAY_ALPHA:
        /* WIC does not support grayscale alpha formats so use RGBA */
        ppng_set_gray_to_rgb(This->png_ptr);
        /* fall through */
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
            hr = E_FAIL;
            goto end;
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
            hr = E_FAIL;
            goto end;
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
            hr = E_FAIL;
            goto end;
        }
        break;
    default:
        ERR("invalid color type %i\n", color_type);
        hr = E_FAIL;
        goto end;
    }

    /* read the image data */
    This->width = ppng_get_image_width(This->png_ptr, This->info_ptr);
    This->height = ppng_get_image_height(This->png_ptr, This->info_ptr);
    This->stride = This->width * This->bpp;
    image_size = This->stride * This->height;

    This->image_bits = HeapAlloc(GetProcessHeap(), 0, image_size);
    if (!This->image_bits)
    {
        hr = E_OUTOFMEMORY;
        goto end;
    }

    row_pointers = HeapAlloc(GetProcessHeap(), 0, sizeof(png_bytep)*This->height);
    if (!row_pointers)
    {
        hr = E_OUTOFMEMORY;
        goto end;
    }

    for (i=0; i<This->height; i++)
        row_pointers[i] = This->image_bits + i * This->stride;

    ppng_read_image(This->png_ptr, row_pointers);

    HeapFree(GetProcessHeap(), 0, row_pointers);
    row_pointers = NULL;

    ppng_read_end(This->png_ptr, This->end_info);

    This->initialized = TRUE;

end:

    LeaveCriticalSection(&This->lock);

    return hr;
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
    HRESULT hr;
    IWICComponentInfo *compinfo;

    TRACE("(%p,%p)\n", iface, ppIDecoderInfo);

    hr = CreateComponentInfo(&CLSID_WICPngDecoder, &compinfo);
    if (FAILED(hr)) return hr;

    hr = IWICComponentInfo_QueryInterface(compinfo, &IID_IWICBitmapDecoderInfo,
        (void**)ppIDecoderInfo);

    IWICComponentInfo_Release(compinfo);

    return hr;
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
    TRACE("(%p,%p)\n", iface, ppIBitmapSource);

    if (!ppIBitmapSource) return E_INVALIDARG;

    *ppIBitmapSource = NULL;
    return WINCODEC_ERR_UNSUPPORTEDOPERATION;
}

static HRESULT WINAPI PngDecoder_GetColorContexts(IWICBitmapDecoder *iface,
    UINT cCount, IWICColorContext **ppIColorContexts, UINT *pcActualCount)
{
    TRACE("(%p,%u,%p,%p)\n", iface, cCount, ppIColorContexts, pcActualCount);
    return WINCODEC_ERR_UNSUPPORTEDOPERATION;
}

static HRESULT WINAPI PngDecoder_GetThumbnail(IWICBitmapDecoder *iface,
    IWICBitmapSource **ppIThumbnail)
{
    TRACE("(%p,%p)\n", iface, ppIThumbnail);

    if (!ppIThumbnail) return E_INVALIDARG;

    *ppIThumbnail = NULL;
    return WINCODEC_ERR_CODECNOTHUMBNAIL;
}

static HRESULT WINAPI PngDecoder_GetFrameCount(IWICBitmapDecoder *iface,
    UINT *pCount)
{
    if (!pCount) return E_INVALIDARG;

    *pCount = 1;
    return S_OK;
}

static HRESULT WINAPI PngDecoder_GetFrame(IWICBitmapDecoder *iface,
    UINT index, IWICBitmapFrameDecode **ppIBitmapFrame)
{
    PngDecoder *This = impl_from_IWICBitmapDecoder(iface);
    TRACE("(%p,%u,%p)\n", iface, index, ppIBitmapFrame);

    if (!This->initialized) return WINCODEC_ERR_FRAMEMISSING;

    if (index != 0) return E_INVALIDARG;

    IWICBitmapDecoder_AddRef(iface);

    *ppIBitmapFrame = &This->IWICBitmapFrameDecode_iface;

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
    PngDecoder *This = impl_from_IWICBitmapFrameDecode(iface);
    if (!ppv) return E_INVALIDARG;

    if (IsEqualIID(&IID_IUnknown, iid) ||
        IsEqualIID(&IID_IWICBitmapSource, iid) ||
        IsEqualIID(&IID_IWICBitmapFrameDecode, iid))
    {
        *ppv = &This->IWICBitmapFrameDecode_iface;
    }
    else if (IsEqualIID(&IID_IWICMetadataBlockReader, iid))
    {
        *ppv = &This->IWICMetadataBlockReader_iface;
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
    PngDecoder *This = impl_from_IWICBitmapFrameDecode(iface);
    return IWICBitmapDecoder_AddRef(&This->IWICBitmapDecoder_iface);
}

static ULONG WINAPI PngDecoder_Frame_Release(IWICBitmapFrameDecode *iface)
{
    PngDecoder *This = impl_from_IWICBitmapFrameDecode(iface);
    return IWICBitmapDecoder_Release(&This->IWICBitmapDecoder_iface);
}

static HRESULT WINAPI PngDecoder_Frame_GetSize(IWICBitmapFrameDecode *iface,
    UINT *puiWidth, UINT *puiHeight)
{
    PngDecoder *This = impl_from_IWICBitmapFrameDecode(iface);
    *puiWidth = This->width;
    *puiHeight = This->height;
    TRACE("(%p)->(%u,%u)\n", iface, *puiWidth, *puiHeight);
    return S_OK;
}

static HRESULT WINAPI PngDecoder_Frame_GetPixelFormat(IWICBitmapFrameDecode *iface,
    WICPixelFormatGUID *pPixelFormat)
{
    PngDecoder *This = impl_from_IWICBitmapFrameDecode(iface);
    TRACE("(%p,%p)\n", iface, pPixelFormat);

    memcpy(pPixelFormat, This->format, sizeof(GUID));

    return S_OK;
}

static HRESULT WINAPI PngDecoder_Frame_GetResolution(IWICBitmapFrameDecode *iface,
    double *pDpiX, double *pDpiY)
{
    PngDecoder *This = impl_from_IWICBitmapFrameDecode(iface);
    png_uint_32 ret, xres, yres;
    int unit_type;

    EnterCriticalSection(&This->lock);

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

    LeaveCriticalSection(&This->lock);

    TRACE("(%p)->(%0.2f,%0.2f)\n", iface, *pDpiX, *pDpiY);

    return S_OK;
}

static HRESULT WINAPI PngDecoder_Frame_CopyPalette(IWICBitmapFrameDecode *iface,
    IWICPalette *pIPalette)
{
    PngDecoder *This = impl_from_IWICBitmapFrameDecode(iface);
    png_uint_32 ret;
    png_colorp png_palette;
    int num_palette;
    WICColor palette[256];
    png_bytep trans_alpha;
    int num_trans;
    png_color_16p trans_values;
    int i;
    HRESULT hr=S_OK;

    TRACE("(%p,%p)\n", iface, pIPalette);

    EnterCriticalSection(&This->lock);

    ret = ppng_get_PLTE(This->png_ptr, This->info_ptr, &png_palette, &num_palette);
    if (!ret)
    {
        hr = WINCODEC_ERR_PALETTEUNAVAILABLE;
        goto end;
    }

    if (num_palette > 256)
    {
        ERR("palette has %i colors?!\n", num_palette);
        hr = E_FAIL;
        goto end;
    }

    ret = ppng_get_tRNS(This->png_ptr, This->info_ptr, &trans_alpha, &num_trans, &trans_values);
    if (!ret) num_trans = 0;

    for (i=0; i<num_palette; i++)
    {
        BYTE alpha = (i < num_trans) ? trans_alpha[i] : 0xff;
        palette[i] = (alpha << 24 |
                      png_palette[i].red << 16|
                      png_palette[i].green << 8|
                      png_palette[i].blue);
    }

end:

    LeaveCriticalSection(&This->lock);

    if (SUCCEEDED(hr))
        hr = IWICPalette_InitializeCustom(pIPalette, palette, num_palette);

    return hr;
}

static HRESULT WINAPI PngDecoder_Frame_CopyPixels(IWICBitmapFrameDecode *iface,
    const WICRect *prc, UINT cbStride, UINT cbBufferSize, BYTE *pbBuffer)
{
    PngDecoder *This = impl_from_IWICBitmapFrameDecode(iface);
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
    PngDecoder *This = impl_from_IWICBitmapFrameDecode(iface);
    png_charp name;
    BYTE *profile;
    png_uint_32 len;
    int compression_type;
    HRESULT hr;

    TRACE("(%p,%u,%p,%p)\n", iface, cCount, ppIColorContexts, pcActualCount);

    if (!pcActualCount) return E_INVALIDARG;

    EnterCriticalSection(&This->lock);

    if (ppng_get_iCCP(This->png_ptr, This->info_ptr, &name, &compression_type, (void *)&profile, &len))
    {
        if (cCount && ppIColorContexts)
        {
            hr = IWICColorContext_InitializeFromMemory(*ppIColorContexts, profile, len);
            if (FAILED(hr))
            {
                LeaveCriticalSection(&This->lock);
                return hr;
            }
        }
        *pcActualCount = 1;
    }
    else
        *pcActualCount = 0;

    LeaveCriticalSection(&This->lock);

    return S_OK;
}

static HRESULT WINAPI PngDecoder_Frame_GetThumbnail(IWICBitmapFrameDecode *iface,
    IWICBitmapSource **ppIThumbnail)
{
    TRACE("(%p,%p)\n", iface, ppIThumbnail);

    if (!ppIThumbnail) return E_INVALIDARG;

    *ppIThumbnail = NULL;
    return WINCODEC_ERR_CODECNOTHUMBNAIL;
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

static HRESULT WINAPI PngDecoder_Block_QueryInterface(IWICMetadataBlockReader *iface, REFIID iid,
    void **ppv)
{
    PngDecoder *This = impl_from_IWICMetadataBlockReader(iface);
    return IWICBitmapFrameDecode_QueryInterface(&This->IWICBitmapFrameDecode_iface, iid, ppv);
}

static ULONG WINAPI PngDecoder_Block_AddRef(IWICMetadataBlockReader *iface)
{
    PngDecoder *This = impl_from_IWICMetadataBlockReader(iface);
    return IWICBitmapDecoder_AddRef(&This->IWICBitmapDecoder_iface);
}

static ULONG WINAPI PngDecoder_Block_Release(IWICMetadataBlockReader *iface)
{
    PngDecoder *This = impl_from_IWICMetadataBlockReader(iface);
    return IWICBitmapDecoder_Release(&This->IWICBitmapDecoder_iface);
}

static HRESULT WINAPI PngDecoder_Block_GetContainerFormat(IWICMetadataBlockReader *iface,
    GUID *pguidContainerFormat)
{
    if (!pguidContainerFormat) return E_INVALIDARG;
    memcpy(pguidContainerFormat, &GUID_ContainerFormatPng, sizeof(GUID));
    return S_OK;
}

static HRESULT WINAPI PngDecoder_Block_GetCount(IWICMetadataBlockReader *iface,
    UINT *pcCount)
{
    static int once;
    TRACE("%p,%p\n", iface, pcCount);
    if (!once++) FIXME("stub\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI PngDecoder_Block_GetReaderByIndex(IWICMetadataBlockReader *iface,
    UINT nIndex, IWICMetadataReader **ppIMetadataReader)
{
    FIXME("%p,%d,%p\n", iface, nIndex, ppIMetadataReader);
    return E_NOTIMPL;
}

static HRESULT WINAPI PngDecoder_Block_GetEnumerator(IWICMetadataBlockReader *iface,
    IEnumUnknown **ppIEnumMetadata)
{
    FIXME("%p,%p\n", iface, ppIEnumMetadata);
    return E_NOTIMPL;
}

static const IWICMetadataBlockReaderVtbl PngDecoder_BlockVtbl = {
    PngDecoder_Block_QueryInterface,
    PngDecoder_Block_AddRef,
    PngDecoder_Block_Release,
    PngDecoder_Block_GetContainerFormat,
    PngDecoder_Block_GetCount,
    PngDecoder_Block_GetReaderByIndex,
    PngDecoder_Block_GetEnumerator,
};

HRESULT PngDecoder_CreateInstance(REFIID iid, void** ppv)
{
    PngDecoder *This;
    HRESULT ret;

    TRACE("(%s,%p)\n", debugstr_guid(iid), ppv);

    *ppv = NULL;

    if (!load_libpng())
    {
        ERR("Failed reading PNG because unable to find %s\n",SONAME_LIBPNG);
        return E_FAIL;
    }

    This = HeapAlloc(GetProcessHeap(), 0, sizeof(PngDecoder));
    if (!This) return E_OUTOFMEMORY;

    This->IWICBitmapDecoder_iface.lpVtbl = &PngDecoder_Vtbl;
    This->IWICBitmapFrameDecode_iface.lpVtbl = &PngDecoder_FrameVtbl;
    This->IWICMetadataBlockReader_iface.lpVtbl = &PngDecoder_BlockVtbl;
    This->ref = 1;
    This->png_ptr = NULL;
    This->info_ptr = NULL;
    This->end_info = NULL;
    This->initialized = FALSE;
    This->image_bits = NULL;
    InitializeCriticalSection(&This->lock);
    This->lock.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": PngDecoder.lock");

    ret = IWICBitmapDecoder_QueryInterface(&This->IWICBitmapDecoder_iface, iid, ppv);
    IWICBitmapDecoder_Release(&This->IWICBitmapDecoder_iface);

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
    IWICBitmapEncoder IWICBitmapEncoder_iface;
    IWICBitmapFrameEncode IWICBitmapFrameEncode_iface;
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
    CRITICAL_SECTION lock;
    BOOL interlace;
    BYTE *data;
    UINT stride;
    UINT passes;
} PngEncoder;

static inline PngEncoder *impl_from_IWICBitmapEncoder(IWICBitmapEncoder *iface)
{
    return CONTAINING_RECORD(iface, PngEncoder, IWICBitmapEncoder_iface);
}

static inline PngEncoder *impl_from_IWICBitmapFrameEncode(IWICBitmapFrameEncode *iface)
{
    return CONTAINING_RECORD(iface, PngEncoder, IWICBitmapFrameEncode_iface);
}

static HRESULT WINAPI PngFrameEncode_QueryInterface(IWICBitmapFrameEncode *iface, REFIID iid,
    void **ppv)
{
    PngEncoder *This = impl_from_IWICBitmapFrameEncode(iface);
    TRACE("(%p,%s,%p)\n", iface, debugstr_guid(iid), ppv);

    if (!ppv) return E_INVALIDARG;

    if (IsEqualIID(&IID_IUnknown, iid) ||
        IsEqualIID(&IID_IWICBitmapFrameEncode, iid))
    {
        *ppv = &This->IWICBitmapFrameEncode_iface;
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
    PngEncoder *This = impl_from_IWICBitmapFrameEncode(iface);
    return IWICBitmapEncoder_AddRef(&This->IWICBitmapEncoder_iface);
}

static ULONG WINAPI PngFrameEncode_Release(IWICBitmapFrameEncode *iface)
{
    PngEncoder *This = impl_from_IWICBitmapFrameEncode(iface);
    return IWICBitmapEncoder_Release(&This->IWICBitmapEncoder_iface);
}

static HRESULT WINAPI PngFrameEncode_Initialize(IWICBitmapFrameEncode *iface,
    IPropertyBag2 *pIEncoderOptions)
{
    PngEncoder *This = impl_from_IWICBitmapFrameEncode(iface);
    BOOL interlace;
    PROPBAG2 opts[1]= {{0}};
    VARIANT opt_values[1];
    HRESULT opt_hres[1];
    HRESULT hr;

    TRACE("(%p,%p)\n", iface, pIEncoderOptions);

    opts[0].pstrName = (LPOLESTR)wszPngInterlaceOption;
    opts[0].vt = VT_BOOL;

    if (pIEncoderOptions)
    {
        hr = IPropertyBag2_Read(pIEncoderOptions, 1, opts, NULL, opt_values, opt_hres);

        if (FAILED(hr))
            return hr;
    }
    else
        memset(opt_values, 0, sizeof(opt_values));

    if (V_VT(&opt_values[0]) == VT_EMPTY)
        interlace = FALSE;
    else
        interlace = (V_BOOL(&opt_values[0]) != 0);

    EnterCriticalSection(&This->lock);

    if (This->frame_initialized)
    {
        LeaveCriticalSection(&This->lock);
        return WINCODEC_ERR_WRONGSTATE;
    }

    This->interlace = interlace;

    This->frame_initialized = TRUE;

    LeaveCriticalSection(&This->lock);

    return S_OK;
}

static HRESULT WINAPI PngFrameEncode_SetSize(IWICBitmapFrameEncode *iface,
    UINT uiWidth, UINT uiHeight)
{
    PngEncoder *This = impl_from_IWICBitmapFrameEncode(iface);
    TRACE("(%p,%u,%u)\n", iface, uiWidth, uiHeight);

    EnterCriticalSection(&This->lock);

    if (!This->frame_initialized || This->info_written)
    {
        LeaveCriticalSection(&This->lock);
        return WINCODEC_ERR_WRONGSTATE;
    }

    This->width = uiWidth;
    This->height = uiHeight;

    LeaveCriticalSection(&This->lock);

    return S_OK;
}

static HRESULT WINAPI PngFrameEncode_SetResolution(IWICBitmapFrameEncode *iface,
    double dpiX, double dpiY)
{
    PngEncoder *This = impl_from_IWICBitmapFrameEncode(iface);
    TRACE("(%p,%0.2f,%0.2f)\n", iface, dpiX, dpiY);

    EnterCriticalSection(&This->lock);

    if (!This->frame_initialized || This->info_written)
    {
        LeaveCriticalSection(&This->lock);
        return WINCODEC_ERR_WRONGSTATE;
    }

    This->xres = dpiX;
    This->yres = dpiY;

    LeaveCriticalSection(&This->lock);

    return S_OK;
}

static HRESULT WINAPI PngFrameEncode_SetPixelFormat(IWICBitmapFrameEncode *iface,
    WICPixelFormatGUID *pPixelFormat)
{
    PngEncoder *This = impl_from_IWICBitmapFrameEncode(iface);
    int i;
    TRACE("(%p,%s)\n", iface, debugstr_guid(pPixelFormat));

    EnterCriticalSection(&This->lock);

    if (!This->frame_initialized || This->info_written)
    {
        LeaveCriticalSection(&This->lock);
        return WINCODEC_ERR_WRONGSTATE;
    }

    for (i=0; formats[i].guid; i++)
    {
        if (memcmp(formats[i].guid, pPixelFormat, sizeof(GUID)) == 0)
            break;
    }

    if (!formats[i].guid) i = 0;

    This->format = &formats[i];
    memcpy(pPixelFormat, This->format->guid, sizeof(GUID));

    LeaveCriticalSection(&This->lock);

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
    PngEncoder *This = impl_from_IWICBitmapFrameEncode(iface);
    png_byte **row_pointers=NULL;
    UINT i;
    jmp_buf jmpbuf;
    TRACE("(%p,%u,%u,%u,%p)\n", iface, lineCount, cbStride, cbBufferSize, pbPixels);

    EnterCriticalSection(&This->lock);

    if (!This->frame_initialized || !This->width || !This->height || !This->format)
    {
        LeaveCriticalSection(&This->lock);
        return WINCODEC_ERR_WRONGSTATE;
    }

    if (lineCount == 0 || lineCount + This->lines_written > This->height)
    {
        LeaveCriticalSection(&This->lock);
        return E_INVALIDARG;
    }

    /* set up setjmp/longjmp error handling */
    if (setjmp(jmpbuf))
    {
        LeaveCriticalSection(&This->lock);
        HeapFree(GetProcessHeap(), 0, row_pointers);
        return E_FAIL;
    }
    ppng_set_error_fn(This->png_ptr, jmpbuf, user_error_fn, user_warning_fn);

    if (!This->info_written)
    {
        if (This->interlace)
        {
            /* libpng requires us to write all data multiple times in this case. */
            This->stride = (This->format->bpp * This->width + 7)/8;
            This->data = HeapAlloc(GetProcessHeap(), 0, This->height * This->stride);
            if (!This->data)
            {
                LeaveCriticalSection(&This->lock);
                return E_OUTOFMEMORY;
            }
        }

        ppng_set_IHDR(This->png_ptr, This->info_ptr, This->width, This->height,
            This->format->bit_depth, This->format->color_type,
            This->interlace ? PNG_INTERLACE_ADAM7 : PNG_INTERLACE_NONE,
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

        if (This->interlace)
            This->passes = ppng_set_interlace_handling(This->png_ptr);

        This->info_written = TRUE;
    }

    if (This->interlace)
    {
        /* Just store the data so we can write it in multiple passes in Commit. */
        for (i=0; i<lineCount; i++)
            memcpy(This->data + This->stride * (This->lines_written + i),
                   pbPixels + cbStride * i,
                   This->stride);

        This->lines_written += lineCount;

        LeaveCriticalSection(&This->lock);
        return S_OK;
    }

    row_pointers = HeapAlloc(GetProcessHeap(), 0, lineCount * sizeof(png_byte*));
    if (!row_pointers)
    {
        LeaveCriticalSection(&This->lock);
        return E_OUTOFMEMORY;
    }

    for (i=0; i<lineCount; i++)
        row_pointers[i] = pbPixels + cbStride * i;

    ppng_write_rows(This->png_ptr, row_pointers, lineCount);
    This->lines_written += lineCount;

    LeaveCriticalSection(&This->lock);

    HeapFree(GetProcessHeap(), 0, row_pointers);

    return S_OK;
}

static HRESULT WINAPI PngFrameEncode_WriteSource(IWICBitmapFrameEncode *iface,
    IWICBitmapSource *pIBitmapSource, WICRect *prc)
{
    PngEncoder *This = impl_from_IWICBitmapFrameEncode(iface);
    HRESULT hr;
    TRACE("(%p,%p,%p)\n", iface, pIBitmapSource, prc);

    if (!This->frame_initialized)
        return WINCODEC_ERR_WRONGSTATE;

    hr = configure_write_source(iface, pIBitmapSource, prc,
        This->format ? This->format->guid : NULL, This->width, This->height,
        This->xres, This->yres);

    if (SUCCEEDED(hr))
    {
        hr = write_source(iface, pIBitmapSource, prc,
            This->format->guid, This->format->bpp, This->width, This->height);
    }

    return hr;
}

static HRESULT WINAPI PngFrameEncode_Commit(IWICBitmapFrameEncode *iface)
{
    PngEncoder *This = impl_from_IWICBitmapFrameEncode(iface);
    png_byte **row_pointers=NULL;
    jmp_buf jmpbuf;
    TRACE("(%p)\n", iface);

    EnterCriticalSection(&This->lock);

    if (!This->info_written || This->lines_written != This->height || This->frame_committed)
    {
        LeaveCriticalSection(&This->lock);
        return WINCODEC_ERR_WRONGSTATE;
    }

    /* set up setjmp/longjmp error handling */
    if (setjmp(jmpbuf))
    {
        LeaveCriticalSection(&This->lock);
        HeapFree(GetProcessHeap(), 0, row_pointers);
        return E_FAIL;
    }
    ppng_set_error_fn(This->png_ptr, jmpbuf, user_error_fn, user_warning_fn);

    if (This->interlace)
    {
        int i;

        row_pointers = HeapAlloc(GetProcessHeap(), 0, This->height * sizeof(png_byte*));
        if (!row_pointers)
        {
            LeaveCriticalSection(&This->lock);
            return E_OUTOFMEMORY;
        }

        for (i=0; i<This->height; i++)
            row_pointers[i] = This->data + This->stride * i;

        for (i=0; i<This->passes; i++)
            ppng_write_rows(This->png_ptr, row_pointers, This->height);
    }

    ppng_write_end(This->png_ptr, This->info_ptr);

    This->frame_committed = TRUE;

    HeapFree(GetProcessHeap(), 0, row_pointers);

    LeaveCriticalSection(&This->lock);

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
    PngEncoder *This = impl_from_IWICBitmapEncoder(iface);
    TRACE("(%p,%s,%p)\n", iface, debugstr_guid(iid), ppv);

    if (!ppv) return E_INVALIDARG;

    if (IsEqualIID(&IID_IUnknown, iid) ||
        IsEqualIID(&IID_IWICBitmapEncoder, iid))
    {
        *ppv = &This->IWICBitmapEncoder_iface;
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
    PngEncoder *This = impl_from_IWICBitmapEncoder(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) refcount=%u\n", iface, ref);

    return ref;
}

static ULONG WINAPI PngEncoder_Release(IWICBitmapEncoder *iface)
{
    PngEncoder *This = impl_from_IWICBitmapEncoder(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) refcount=%u\n", iface, ref);

    if (ref == 0)
    {
        This->lock.DebugInfo->Spare[0] = 0;
        DeleteCriticalSection(&This->lock);
        if (This->png_ptr)
            ppng_destroy_write_struct(&This->png_ptr, &This->info_ptr);
        if (This->stream)
            IStream_Release(This->stream);
        HeapFree(GetProcessHeap(), 0, This->data);
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
    PngEncoder *This = impl_from_IWICBitmapEncoder(iface);
    jmp_buf jmpbuf;

    TRACE("(%p,%p,%u)\n", iface, pIStream, cacheOption);

    EnterCriticalSection(&This->lock);

    if (This->png_ptr)
    {
        LeaveCriticalSection(&This->lock);
        return WINCODEC_ERR_WRONGSTATE;
    }

    /* initialize libpng */
    This->png_ptr = ppng_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!This->png_ptr)
    {
        LeaveCriticalSection(&This->lock);
        return E_FAIL;
    }

    This->info_ptr = ppng_create_info_struct(This->png_ptr);
    if (!This->info_ptr)
    {
        ppng_destroy_write_struct(&This->png_ptr, NULL);
        This->png_ptr = NULL;
        LeaveCriticalSection(&This->lock);
        return E_FAIL;
    }

    IStream_AddRef(pIStream);
    This->stream = pIStream;

    /* set up setjmp/longjmp error handling */
    if (setjmp(jmpbuf))
    {
        ppng_destroy_write_struct(&This->png_ptr, &This->info_ptr);
        This->png_ptr = NULL;
        IStream_Release(This->stream);
        This->stream = NULL;
        LeaveCriticalSection(&This->lock);
        return E_FAIL;
    }
    ppng_set_error_fn(This->png_ptr, jmpbuf, user_error_fn, user_warning_fn);

    /* set up custom i/o handling */
    ppng_set_write_fn(This->png_ptr, This, user_write_data, user_flush);

    LeaveCriticalSection(&This->lock);

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
    PngEncoder *This = impl_from_IWICBitmapEncoder(iface);
    HRESULT hr;
    PROPBAG2 opts[1]= {{0}};

    TRACE("(%p,%p,%p)\n", iface, ppIFrameEncode, ppIEncoderOptions);

    EnterCriticalSection(&This->lock);

    if (This->frame_count != 0)
    {
        LeaveCriticalSection(&This->lock);
        return WINCODEC_ERR_UNSUPPORTEDOPERATION;
    }

    if (!This->stream)
    {
        LeaveCriticalSection(&This->lock);
        return WINCODEC_ERR_NOTINITIALIZED;
    }

    opts[0].pstrName = (LPOLESTR)wszPngInterlaceOption;
    opts[0].vt = VT_BOOL;
    opts[0].dwType = PROPBAG2_TYPE_DATA;

    hr = CreatePropertyBag2(opts, 1, ppIEncoderOptions);
    if (FAILED(hr))
    {
        LeaveCriticalSection(&This->lock);
        return hr;
    }

    This->frame_count = 1;

    LeaveCriticalSection(&This->lock);

    IWICBitmapEncoder_AddRef(iface);
    *ppIFrameEncode = &This->IWICBitmapFrameEncode_iface;

    return S_OK;
}

static HRESULT WINAPI PngEncoder_Commit(IWICBitmapEncoder *iface)
{
    PngEncoder *This = impl_from_IWICBitmapEncoder(iface);
    TRACE("(%p)\n", iface);

    EnterCriticalSection(&This->lock);

    if (!This->frame_committed || This->committed)
    {
        LeaveCriticalSection(&This->lock);
        return WINCODEC_ERR_WRONGSTATE;
    }

    This->committed = TRUE;

    LeaveCriticalSection(&This->lock);

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

HRESULT PngEncoder_CreateInstance(REFIID iid, void** ppv)
{
    PngEncoder *This;
    HRESULT ret;

    TRACE("(%s,%p)\n", debugstr_guid(iid), ppv);

    *ppv = NULL;

    if (!load_libpng())
    {
        ERR("Failed writing PNG because unable to find %s\n",SONAME_LIBPNG);
        return E_FAIL;
    }

    This = HeapAlloc(GetProcessHeap(), 0, sizeof(PngEncoder));
    if (!This) return E_OUTOFMEMORY;

    This->IWICBitmapEncoder_iface.lpVtbl = &PngEncoder_Vtbl;
    This->IWICBitmapFrameEncode_iface.lpVtbl = &PngEncoder_FrameVtbl;
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
    This->data = NULL;
    InitializeCriticalSection(&This->lock);
    This->lock.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": PngEncoder.lock");

    ret = IWICBitmapEncoder_QueryInterface(&This->IWICBitmapEncoder_iface, iid, ppv);
    IWICBitmapEncoder_Release(&This->IWICBitmapEncoder_iface);

    return ret;
}

#else /* !HAVE_PNG_H */

HRESULT PngDecoder_CreateInstance(REFIID iid, void** ppv)
{
    ERR("Trying to load PNG picture, but PNG support is not compiled in.\n");
    return E_FAIL;
}

HRESULT PngEncoder_CreateInstance(REFIID iid, void** ppv)
{
    ERR("Trying to save PNG picture, but PNG support is not compiled in.\n");
    return E_FAIL;
}

#endif
