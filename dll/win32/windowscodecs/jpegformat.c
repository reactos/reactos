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

#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <setjmp.h>

#ifdef SONAME_LIBJPEG
/* This is a hack, so jpeglib.h does not redefine INT32 and the like*/
#define XMD_H
#define UINT8 JPEG_UINT8
#define UINT16 JPEG_UINT16
#define boolean jpeg_boolean
#undef HAVE_STDLIB_H
# include <jpeglib.h>
#undef HAVE_STDLIB_H
#define HAVE_STDLIB_H 1
#undef UINT8
#undef UINT16
#undef boolean
#endif

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "objbase.h"

#include "wincodecs_private.h"

#include "wine/heap.h"
#include "wine/debug.h"
#include "wine/library.h"

WINE_DEFAULT_DEBUG_CHANNEL(wincodecs);

#ifdef SONAME_LIBJPEG
WINE_DECLARE_DEBUG_CHANNEL(jpeg);

static void *libjpeg_handle;

static const WCHAR wszImageQuality[] = {'I','m','a','g','e','Q','u','a','l','i','t','y',0};
static const WCHAR wszBitmapTransform[] = {'B','i','t','m','a','p','T','r','a','n','s','f','o','r','m',0};
static const WCHAR wszLuminance[] = {'L','u','m','i','n','a','n','c','e',0};
static const WCHAR wszChrominance[] = {'C','h','r','o','m','i','n','a','n','c','e',0};
static const WCHAR wszJpegYCrCbSubsampling[] = {'J','p','e','g','Y','C','r','C','b','S','u','b','s','a','m','p','l','i','n','g',0};
static const WCHAR wszSuppressApp0[] = {'S','u','p','p','r','e','s','s','A','p','p','0',0};

#define MAKE_FUNCPTR(f) static typeof(f) * p##f
MAKE_FUNCPTR(jpeg_CreateCompress);
MAKE_FUNCPTR(jpeg_CreateDecompress);
MAKE_FUNCPTR(jpeg_destroy_compress);
MAKE_FUNCPTR(jpeg_destroy_decompress);
MAKE_FUNCPTR(jpeg_finish_compress);
MAKE_FUNCPTR(jpeg_read_header);
MAKE_FUNCPTR(jpeg_read_scanlines);
MAKE_FUNCPTR(jpeg_resync_to_restart);
MAKE_FUNCPTR(jpeg_set_defaults);
MAKE_FUNCPTR(jpeg_start_compress);
MAKE_FUNCPTR(jpeg_start_decompress);
MAKE_FUNCPTR(jpeg_std_error);
MAKE_FUNCPTR(jpeg_write_scanlines);
#undef MAKE_FUNCPTR

static void *load_libjpeg(void)
{
    if((libjpeg_handle = wine_dlopen(SONAME_LIBJPEG, RTLD_NOW, NULL, 0)) != NULL) {

#define LOAD_FUNCPTR(f) \
    if((p##f = wine_dlsym(libjpeg_handle, #f, NULL, 0)) == NULL) { \
        libjpeg_handle = NULL; \
        return NULL; \
    }

        LOAD_FUNCPTR(jpeg_CreateCompress);
        LOAD_FUNCPTR(jpeg_CreateDecompress);
        LOAD_FUNCPTR(jpeg_destroy_compress);
        LOAD_FUNCPTR(jpeg_destroy_decompress);
        LOAD_FUNCPTR(jpeg_finish_compress);
        LOAD_FUNCPTR(jpeg_read_header);
        LOAD_FUNCPTR(jpeg_read_scanlines);
        LOAD_FUNCPTR(jpeg_resync_to_restart);
        LOAD_FUNCPTR(jpeg_set_defaults);
        LOAD_FUNCPTR(jpeg_start_compress);
        LOAD_FUNCPTR(jpeg_start_decompress);
        LOAD_FUNCPTR(jpeg_std_error);
        LOAD_FUNCPTR(jpeg_write_scanlines);
#undef LOAD_FUNCPTR
    }
    return libjpeg_handle;
}

static void error_exit_fn(j_common_ptr cinfo)
{
    char message[JMSG_LENGTH_MAX];
    if (ERR_ON(jpeg))
    {
        cinfo->err->format_message(cinfo, message);
        ERR_(jpeg)("%s\n", message);
    }
    longjmp(*(jmp_buf*)cinfo->client_data, 1);
}

static void emit_message_fn(j_common_ptr cinfo, int msg_level)
{
    char message[JMSG_LENGTH_MAX];

    if (msg_level < 0 && ERR_ON(jpeg))
    {
        cinfo->err->format_message(cinfo, message);
        ERR_(jpeg)("%s\n", message);
    }
    else if (msg_level == 0 && WARN_ON(jpeg))
    {
        cinfo->err->format_message(cinfo, message);
        WARN_(jpeg)("%s\n", message);
    }
    else if (msg_level > 0 && TRACE_ON(jpeg))
    {
        cinfo->err->format_message(cinfo, message);
        TRACE_(jpeg)("%s\n", message);
    }
}

typedef struct {
    IWICBitmapDecoder IWICBitmapDecoder_iface;
    IWICBitmapFrameDecode IWICBitmapFrameDecode_iface;
    IWICMetadataBlockReader IWICMetadataBlockReader_iface;
    LONG ref;
    BOOL initialized;
    BOOL cinfo_initialized;
    IStream *stream;
    struct jpeg_decompress_struct cinfo;
    struct jpeg_error_mgr jerr;
    struct jpeg_source_mgr source_mgr;
    BYTE source_buffer[1024];
    UINT bpp, stride;
    BYTE *image_data;
    CRITICAL_SECTION lock;
} JpegDecoder;

static inline JpegDecoder *impl_from_IWICBitmapDecoder(IWICBitmapDecoder *iface)
{
    return CONTAINING_RECORD(iface, JpegDecoder, IWICBitmapDecoder_iface);
}

static inline JpegDecoder *impl_from_IWICBitmapFrameDecode(IWICBitmapFrameDecode *iface)
{
    return CONTAINING_RECORD(iface, JpegDecoder, IWICBitmapFrameDecode_iface);
}

static inline JpegDecoder *decoder_from_decompress(j_decompress_ptr decompress)
{
    return CONTAINING_RECORD(decompress, JpegDecoder, cinfo);
}

static inline JpegDecoder *impl_from_IWICMetadataBlockReader(IWICMetadataBlockReader *iface)
{
    return CONTAINING_RECORD(iface, JpegDecoder, IWICMetadataBlockReader_iface);
}

static HRESULT WINAPI JpegDecoder_QueryInterface(IWICBitmapDecoder *iface, REFIID iid,
    void **ppv)
{
    JpegDecoder *This = impl_from_IWICBitmapDecoder(iface);
    TRACE("(%p,%s,%p)\n", iface, debugstr_guid(iid), ppv);

    if (!ppv) return E_INVALIDARG;

    if (IsEqualIID(&IID_IUnknown, iid) ||
        IsEqualIID(&IID_IWICBitmapDecoder, iid))
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

static ULONG WINAPI JpegDecoder_AddRef(IWICBitmapDecoder *iface)
{
    JpegDecoder *This = impl_from_IWICBitmapDecoder(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) refcount=%u\n", iface, ref);

    return ref;
}

static ULONG WINAPI JpegDecoder_Release(IWICBitmapDecoder *iface)
{
    JpegDecoder *This = impl_from_IWICBitmapDecoder(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) refcount=%u\n", iface, ref);

    if (ref == 0)
    {
        This->lock.DebugInfo->Spare[0] = 0;
        DeleteCriticalSection(&This->lock);
        if (This->cinfo_initialized) pjpeg_destroy_decompress(&This->cinfo);
        if (This->stream) IStream_Release(This->stream);
        HeapFree(GetProcessHeap(), 0, This->image_data);
        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}

static HRESULT WINAPI JpegDecoder_QueryCapability(IWICBitmapDecoder *iface, IStream *stream,
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

static void source_mgr_init_source(j_decompress_ptr cinfo)
{
}

static jpeg_boolean source_mgr_fill_input_buffer(j_decompress_ptr cinfo)
{
    JpegDecoder *This = decoder_from_decompress(cinfo);
    HRESULT hr;
    ULONG bytesread;

    hr = IStream_Read(This->stream, This->source_buffer, 1024, &bytesread);

    if (FAILED(hr) || bytesread == 0)
    {
        return FALSE;
    }
    else
    {
        This->source_mgr.next_input_byte = This->source_buffer;
        This->source_mgr.bytes_in_buffer = bytesread;
        return TRUE;
    }
}

static void source_mgr_skip_input_data(j_decompress_ptr cinfo, long num_bytes)
{
    JpegDecoder *This = decoder_from_decompress(cinfo);
    LARGE_INTEGER seek;

    if (num_bytes > This->source_mgr.bytes_in_buffer)
    {
        seek.QuadPart = num_bytes - This->source_mgr.bytes_in_buffer;
        IStream_Seek(This->stream, seek, STREAM_SEEK_CUR, NULL);
        This->source_mgr.bytes_in_buffer = 0;
    }
    else if (num_bytes > 0)
    {
        This->source_mgr.next_input_byte += num_bytes;
        This->source_mgr.bytes_in_buffer -= num_bytes;
    }
}

static void source_mgr_term_source(j_decompress_ptr cinfo)
{
}

static HRESULT WINAPI JpegDecoder_Initialize(IWICBitmapDecoder *iface, IStream *pIStream,
    WICDecodeOptions cacheOptions)
{
    JpegDecoder *This = impl_from_IWICBitmapDecoder(iface);
    int ret;
    LARGE_INTEGER seek;
    jmp_buf jmpbuf;
    UINT data_size, i;

    TRACE("(%p,%p,%u)\n", iface, pIStream, cacheOptions);

    EnterCriticalSection(&This->lock);

    if (This->cinfo_initialized)
    {
        LeaveCriticalSection(&This->lock);
        return WINCODEC_ERR_WRONGSTATE;
    }

    pjpeg_std_error(&This->jerr);

    This->jerr.error_exit = error_exit_fn;
    This->jerr.emit_message = emit_message_fn;

    This->cinfo.err = &This->jerr;

    This->cinfo.client_data = jmpbuf;

    if (setjmp(jmpbuf))
    {
        LeaveCriticalSection(&This->lock);
        return E_FAIL;
    }

    pjpeg_CreateDecompress(&This->cinfo, JPEG_LIB_VERSION, sizeof(struct jpeg_decompress_struct));

    This->cinfo_initialized = TRUE;

    This->stream = pIStream;
    IStream_AddRef(pIStream);

    seek.QuadPart = 0;
    IStream_Seek(This->stream, seek, STREAM_SEEK_SET, NULL);

    This->source_mgr.bytes_in_buffer = 0;
    This->source_mgr.init_source = source_mgr_init_source;
    This->source_mgr.fill_input_buffer = source_mgr_fill_input_buffer;
    This->source_mgr.skip_input_data = source_mgr_skip_input_data;
    This->source_mgr.resync_to_restart = pjpeg_resync_to_restart;
    This->source_mgr.term_source = source_mgr_term_source;

    This->cinfo.src = &This->source_mgr;

    ret = pjpeg_read_header(&This->cinfo, TRUE);

    if (ret != JPEG_HEADER_OK) {
        WARN("Jpeg image in stream has bad format, read header returned %d.\n",ret);
        LeaveCriticalSection(&This->lock);
        return E_FAIL;
    }

    switch (This->cinfo.jpeg_color_space)
    {
    case JCS_GRAYSCALE:
        This->cinfo.out_color_space = JCS_GRAYSCALE;
        break;
    case JCS_RGB:
    case JCS_YCbCr:
        This->cinfo.out_color_space = JCS_RGB;
        break;
    case JCS_CMYK:
    case JCS_YCCK:
        This->cinfo.out_color_space = JCS_CMYK;
        break;
    default:
        ERR("Unknown JPEG color space %i\n", This->cinfo.jpeg_color_space);
        LeaveCriticalSection(&This->lock);
        return E_FAIL;
    }

    if (!pjpeg_start_decompress(&This->cinfo))
    {
        ERR("jpeg_start_decompress failed\n");
        LeaveCriticalSection(&This->lock);
        return E_FAIL;
    }

    if (This->cinfo.out_color_space == JCS_GRAYSCALE) This->bpp = 8;
    else if (This->cinfo.out_color_space == JCS_CMYK) This->bpp = 32;
    else This->bpp = 24;

    This->stride = (This->bpp * This->cinfo.output_width + 7) / 8;
    data_size = This->stride * This->cinfo.output_height;

    This->image_data = heap_alloc(data_size);
    if (!This->image_data)
    {
        LeaveCriticalSection(&This->lock);
        return E_OUTOFMEMORY;
    }

    while (This->cinfo.output_scanline < This->cinfo.output_height)
    {
        UINT first_scanline = This->cinfo.output_scanline;
        UINT max_rows;
        JSAMPROW out_rows[4];
        JDIMENSION ret;

        max_rows = min(This->cinfo.output_height-first_scanline, 4);
        for (i=0; i<max_rows; i++)
            out_rows[i] = This->image_data + This->stride * (first_scanline+i);

        ret = pjpeg_read_scanlines(&This->cinfo, out_rows, max_rows);
        if (ret == 0)
        {
            ERR("read_scanlines failed\n");
            LeaveCriticalSection(&This->lock);
            return E_FAIL;
        }
    }

    if (This->bpp == 24)
    {
        /* libjpeg gives us RGB data and we want BGR, so byteswap the data */
        reverse_bgr8(3, This->image_data,
            This->cinfo.output_width, This->cinfo.output_height,
            This->stride);
    }

    if (This->cinfo.out_color_space == JCS_CMYK && This->cinfo.saw_Adobe_marker)
    {
        /* Adobe JPEG's have inverted CMYK data. */
        for (i=0; i<data_size; i++)
            This->image_data[i] ^= 0xff;
    }

    This->initialized = TRUE;

    LeaveCriticalSection(&This->lock);

    return S_OK;
}

static HRESULT WINAPI JpegDecoder_GetContainerFormat(IWICBitmapDecoder *iface,
    GUID *pguidContainerFormat)
{
    memcpy(pguidContainerFormat, &GUID_ContainerFormatJpeg, sizeof(GUID));
    return S_OK;
}

static HRESULT WINAPI JpegDecoder_GetDecoderInfo(IWICBitmapDecoder *iface,
    IWICBitmapDecoderInfo **ppIDecoderInfo)
{
    TRACE("(%p,%p)\n", iface, ppIDecoderInfo);

    return get_decoder_info(&CLSID_WICJpegDecoder, ppIDecoderInfo);
}

static HRESULT WINAPI JpegDecoder_CopyPalette(IWICBitmapDecoder *iface,
    IWICPalette *pIPalette)
{
    TRACE("(%p,%p)\n", iface, pIPalette);

    return WINCODEC_ERR_PALETTEUNAVAILABLE;
}

static HRESULT WINAPI JpegDecoder_GetMetadataQueryReader(IWICBitmapDecoder *iface,
    IWICMetadataQueryReader **reader)
{
    TRACE("(%p,%p)\n", iface, reader);

    if (!reader) return E_INVALIDARG;

    *reader = NULL;
    return WINCODEC_ERR_UNSUPPORTEDOPERATION;
}

static HRESULT WINAPI JpegDecoder_GetPreview(IWICBitmapDecoder *iface,
    IWICBitmapSource **ppIBitmapSource)
{
    FIXME("(%p,%p): stub\n", iface, ppIBitmapSource);
    return WINCODEC_ERR_UNSUPPORTEDOPERATION;
}

static HRESULT WINAPI JpegDecoder_GetColorContexts(IWICBitmapDecoder *iface,
    UINT cCount, IWICColorContext **ppIColorContexts, UINT *pcActualCount)
{
    FIXME("(%p,%u,%p,%p): stub\n", iface, cCount, ppIColorContexts, pcActualCount);
    return WINCODEC_ERR_UNSUPPORTEDOPERATION;
}

static HRESULT WINAPI JpegDecoder_GetThumbnail(IWICBitmapDecoder *iface,
    IWICBitmapSource **ppIThumbnail)
{
    FIXME("(%p,%p): stub\n", iface, ppIThumbnail);
    return WINCODEC_ERR_CODECNOTHUMBNAIL;
}

static HRESULT WINAPI JpegDecoder_GetFrameCount(IWICBitmapDecoder *iface,
    UINT *pCount)
{
    if (!pCount) return E_INVALIDARG;

    *pCount = 1;
    return S_OK;
}

static HRESULT WINAPI JpegDecoder_GetFrame(IWICBitmapDecoder *iface,
    UINT index, IWICBitmapFrameDecode **ppIBitmapFrame)
{
    JpegDecoder *This = impl_from_IWICBitmapDecoder(iface);
    TRACE("(%p,%u,%p)\n", iface, index, ppIBitmapFrame);

    if (!This->initialized) return WINCODEC_ERR_FRAMEMISSING;

    if (index != 0) return E_INVALIDARG;

    IWICBitmapDecoder_AddRef(iface);
    *ppIBitmapFrame = &This->IWICBitmapFrameDecode_iface;

    return S_OK;
}

static const IWICBitmapDecoderVtbl JpegDecoder_Vtbl = {
    JpegDecoder_QueryInterface,
    JpegDecoder_AddRef,
    JpegDecoder_Release,
    JpegDecoder_QueryCapability,
    JpegDecoder_Initialize,
    JpegDecoder_GetContainerFormat,
    JpegDecoder_GetDecoderInfo,
    JpegDecoder_CopyPalette,
    JpegDecoder_GetMetadataQueryReader,
    JpegDecoder_GetPreview,
    JpegDecoder_GetColorContexts,
    JpegDecoder_GetThumbnail,
    JpegDecoder_GetFrameCount,
    JpegDecoder_GetFrame
};

static HRESULT WINAPI JpegDecoder_Frame_QueryInterface(IWICBitmapFrameDecode *iface, REFIID iid,
    void **ppv)
{
    JpegDecoder *This = impl_from_IWICBitmapFrameDecode(iface);

    TRACE("(%p,%s,%p)\n", iface, debugstr_guid(iid), ppv);

    if (!ppv) return E_INVALIDARG;

    if (IsEqualIID(&IID_IUnknown, iid) ||
        IsEqualIID(&IID_IWICBitmapSource, iid) ||
        IsEqualIID(&IID_IWICBitmapFrameDecode, iid))
    {
        *ppv = &This->IWICBitmapFrameDecode_iface;
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI JpegDecoder_Frame_AddRef(IWICBitmapFrameDecode *iface)
{
    JpegDecoder *This = impl_from_IWICBitmapFrameDecode(iface);
    return IWICBitmapDecoder_AddRef(&This->IWICBitmapDecoder_iface);
}

static ULONG WINAPI JpegDecoder_Frame_Release(IWICBitmapFrameDecode *iface)
{
    JpegDecoder *This = impl_from_IWICBitmapFrameDecode(iface);
    return IWICBitmapDecoder_Release(&This->IWICBitmapDecoder_iface);
}

static HRESULT WINAPI JpegDecoder_Frame_GetSize(IWICBitmapFrameDecode *iface,
    UINT *puiWidth, UINT *puiHeight)
{
    JpegDecoder *This = impl_from_IWICBitmapFrameDecode(iface);
    *puiWidth = This->cinfo.output_width;
    *puiHeight = This->cinfo.output_height;
    TRACE("(%p)->(%u,%u)\n", iface, *puiWidth, *puiHeight);
    return S_OK;
}

static HRESULT WINAPI JpegDecoder_Frame_GetPixelFormat(IWICBitmapFrameDecode *iface,
    WICPixelFormatGUID *pPixelFormat)
{
    JpegDecoder *This = impl_from_IWICBitmapFrameDecode(iface);
    TRACE("(%p,%p)\n", iface, pPixelFormat);
    if (This->cinfo.out_color_space == JCS_RGB)
        memcpy(pPixelFormat, &GUID_WICPixelFormat24bppBGR, sizeof(GUID));
    else if (This->cinfo.out_color_space == JCS_CMYK)
        memcpy(pPixelFormat, &GUID_WICPixelFormat32bppCMYK, sizeof(GUID));
    else /* This->cinfo.out_color_space == JCS_GRAYSCALE */
        memcpy(pPixelFormat, &GUID_WICPixelFormat8bppGray, sizeof(GUID));
    return S_OK;
}

static HRESULT WINAPI JpegDecoder_Frame_GetResolution(IWICBitmapFrameDecode *iface,
    double *pDpiX, double *pDpiY)
{
    JpegDecoder *This = impl_from_IWICBitmapFrameDecode(iface);

    EnterCriticalSection(&This->lock);

    switch (This->cinfo.density_unit)
    {
    case 2: /* pixels per centimeter */
        *pDpiX = This->cinfo.X_density * 2.54;
        *pDpiY = This->cinfo.Y_density * 2.54;
        break;

    case 1: /* pixels per inch */
        *pDpiX = This->cinfo.X_density;
        *pDpiY = This->cinfo.Y_density;
        break;

    case 0: /* unknown */
    default:
        *pDpiX = 96.0;
        *pDpiY = 96.0;
        break;
    }

    LeaveCriticalSection(&This->lock);

    TRACE("(%p)->(%0.2f,%0.2f)\n", iface, *pDpiX, *pDpiY);

    return S_OK;
}

static HRESULT WINAPI JpegDecoder_Frame_CopyPalette(IWICBitmapFrameDecode *iface,
    IWICPalette *pIPalette)
{
    FIXME("(%p,%p): stub\n", iface, pIPalette);
    return E_NOTIMPL;
}

static HRESULT WINAPI JpegDecoder_Frame_CopyPixels(IWICBitmapFrameDecode *iface,
    const WICRect *prc, UINT cbStride, UINT cbBufferSize, BYTE *pbBuffer)
{
    JpegDecoder *This = impl_from_IWICBitmapFrameDecode(iface);

    TRACE("(%p,%s,%u,%u,%p)\n", iface, debug_wic_rect(prc), cbStride, cbBufferSize, pbBuffer);

    return copy_pixels(This->bpp, This->image_data,
        This->cinfo.output_width, This->cinfo.output_height, This->stride,
        prc, cbStride, cbBufferSize, pbBuffer);
}

static HRESULT WINAPI JpegDecoder_Frame_GetMetadataQueryReader(IWICBitmapFrameDecode *iface,
    IWICMetadataQueryReader **ppIMetadataQueryReader)
{
    JpegDecoder *This = impl_from_IWICBitmapFrameDecode(iface);

    TRACE("(%p,%p)\n", iface, ppIMetadataQueryReader);

    if (!ppIMetadataQueryReader)
        return E_INVALIDARG;

    return MetadataQueryReader_CreateInstance(&This->IWICMetadataBlockReader_iface, NULL, ppIMetadataQueryReader);
}

static HRESULT WINAPI JpegDecoder_Frame_GetColorContexts(IWICBitmapFrameDecode *iface,
    UINT cCount, IWICColorContext **ppIColorContexts, UINT *pcActualCount)
{
    FIXME("(%p,%u,%p,%p): stub\n", iface, cCount, ppIColorContexts, pcActualCount);
    return WINCODEC_ERR_UNSUPPORTEDOPERATION;
}

static HRESULT WINAPI JpegDecoder_Frame_GetThumbnail(IWICBitmapFrameDecode *iface,
    IWICBitmapSource **ppIThumbnail)
{
    FIXME("(%p,%p): stub\n", iface, ppIThumbnail);
    return WINCODEC_ERR_CODECNOTHUMBNAIL;
}

static const IWICBitmapFrameDecodeVtbl JpegDecoder_Frame_Vtbl = {
    JpegDecoder_Frame_QueryInterface,
    JpegDecoder_Frame_AddRef,
    JpegDecoder_Frame_Release,
    JpegDecoder_Frame_GetSize,
    JpegDecoder_Frame_GetPixelFormat,
    JpegDecoder_Frame_GetResolution,
    JpegDecoder_Frame_CopyPalette,
    JpegDecoder_Frame_CopyPixels,
    JpegDecoder_Frame_GetMetadataQueryReader,
    JpegDecoder_Frame_GetColorContexts,
    JpegDecoder_Frame_GetThumbnail
};

static HRESULT WINAPI JpegDecoder_Block_QueryInterface(IWICMetadataBlockReader *iface, REFIID iid,
    void **ppv)
{
    JpegDecoder *This = impl_from_IWICMetadataBlockReader(iface);
    return IWICBitmapFrameDecode_QueryInterface(&This->IWICBitmapFrameDecode_iface, iid, ppv);
}

static ULONG WINAPI JpegDecoder_Block_AddRef(IWICMetadataBlockReader *iface)
{
    JpegDecoder *This = impl_from_IWICMetadataBlockReader(iface);
    return IWICBitmapDecoder_AddRef(&This->IWICBitmapDecoder_iface);
}

static ULONG WINAPI JpegDecoder_Block_Release(IWICMetadataBlockReader *iface)
{
    JpegDecoder *This = impl_from_IWICMetadataBlockReader(iface);
    return IWICBitmapDecoder_Release(&This->IWICBitmapDecoder_iface);
}

static HRESULT WINAPI JpegDecoder_Block_GetContainerFormat(IWICMetadataBlockReader *iface,
    GUID *pguidContainerFormat)
{
    TRACE("%p,%p\n", iface, pguidContainerFormat);

    if (!pguidContainerFormat) return E_INVALIDARG;

    memcpy(pguidContainerFormat, &GUID_ContainerFormatJpeg, sizeof(GUID));

    return S_OK;
}

static HRESULT WINAPI JpegDecoder_Block_GetCount(IWICMetadataBlockReader *iface,
    UINT *pcCount)
{
    FIXME("%p,%p\n", iface, pcCount);

    if (!pcCount) return E_INVALIDARG;

    *pcCount = 0;

    return S_OK;
}

static HRESULT WINAPI JpegDecoder_Block_GetReaderByIndex(IWICMetadataBlockReader *iface,
    UINT nIndex, IWICMetadataReader **ppIMetadataReader)
{
    FIXME("%p,%d,%p\n", iface, nIndex, ppIMetadataReader);
    return E_INVALIDARG;
}

static HRESULT WINAPI JpegDecoder_Block_GetEnumerator(IWICMetadataBlockReader *iface,
    IEnumUnknown **ppIEnumMetadata)
{
    FIXME("%p,%p\n", iface, ppIEnumMetadata);
    return E_NOTIMPL;
}

static const IWICMetadataBlockReaderVtbl JpegDecoder_Block_Vtbl = {
    JpegDecoder_Block_QueryInterface,
    JpegDecoder_Block_AddRef,
    JpegDecoder_Block_Release,
    JpegDecoder_Block_GetContainerFormat,
    JpegDecoder_Block_GetCount,
    JpegDecoder_Block_GetReaderByIndex,
    JpegDecoder_Block_GetEnumerator,
};

HRESULT JpegDecoder_CreateInstance(REFIID iid, void** ppv)
{
    JpegDecoder *This;
    HRESULT ret;

    TRACE("(%s,%p)\n", debugstr_guid(iid), ppv);

    if (!libjpeg_handle && !load_libjpeg())
    {
        ERR("Failed reading JPEG because unable to find %s\n", SONAME_LIBJPEG);
        return E_FAIL;
    }

    *ppv = NULL;

    This = HeapAlloc(GetProcessHeap(), 0, sizeof(JpegDecoder));
    if (!This) return E_OUTOFMEMORY;

    This->IWICBitmapDecoder_iface.lpVtbl = &JpegDecoder_Vtbl;
    This->IWICBitmapFrameDecode_iface.lpVtbl = &JpegDecoder_Frame_Vtbl;
    This->IWICMetadataBlockReader_iface.lpVtbl = &JpegDecoder_Block_Vtbl;
    This->ref = 1;
    This->initialized = FALSE;
    This->cinfo_initialized = FALSE;
    This->stream = NULL;
    This->image_data = NULL;
    InitializeCriticalSection(&This->lock);
    This->lock.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": JpegDecoder.lock");

    ret = IWICBitmapDecoder_QueryInterface(&This->IWICBitmapDecoder_iface, iid, ppv);
    IWICBitmapDecoder_Release(&This->IWICBitmapDecoder_iface);

    return ret;
}

typedef struct jpeg_compress_format {
    const WICPixelFormatGUID *guid;
    int bpp;
    int num_components;
    J_COLOR_SPACE color_space;
    int swap_rgb;
} jpeg_compress_format;

static const jpeg_compress_format compress_formats[] = {
    { &GUID_WICPixelFormat24bppBGR, 24, 3, JCS_RGB, 1 },
    { &GUID_WICPixelFormat32bppCMYK, 32, 4, JCS_CMYK },
    { &GUID_WICPixelFormat8bppGray, 8, 1, JCS_GRAYSCALE },
    { 0 }
};

typedef struct JpegEncoder {
    IWICBitmapEncoder IWICBitmapEncoder_iface;
    IWICBitmapFrameEncode IWICBitmapFrameEncode_iface;
    LONG ref;
    struct jpeg_compress_struct cinfo;
    struct jpeg_error_mgr jerr;
    struct jpeg_destination_mgr dest_mgr;
    BOOL initialized;
    int frame_count;
    BOOL frame_initialized;
    BOOL started_compress;
    int lines_written;
    BOOL frame_committed;
    BOOL committed;
    UINT width, height;
    double xres, yres;
    const jpeg_compress_format *format;
    IStream *stream;
    WICColor palette[256];
    UINT colors;
    CRITICAL_SECTION lock;
    BYTE dest_buffer[1024];
} JpegEncoder;

static inline JpegEncoder *impl_from_IWICBitmapEncoder(IWICBitmapEncoder *iface)
{
    return CONTAINING_RECORD(iface, JpegEncoder, IWICBitmapEncoder_iface);
}

static inline JpegEncoder *impl_from_IWICBitmapFrameEncode(IWICBitmapFrameEncode *iface)
{
    return CONTAINING_RECORD(iface, JpegEncoder, IWICBitmapFrameEncode_iface);
}

static inline JpegEncoder *encoder_from_compress(j_compress_ptr compress)
{
    return CONTAINING_RECORD(compress, JpegEncoder, cinfo);
}

static void dest_mgr_init_destination(j_compress_ptr cinfo)
{
    JpegEncoder *This = encoder_from_compress(cinfo);

    This->dest_mgr.next_output_byte = This->dest_buffer;
    This->dest_mgr.free_in_buffer = sizeof(This->dest_buffer);
}

static jpeg_boolean dest_mgr_empty_output_buffer(j_compress_ptr cinfo)
{
    JpegEncoder *This = encoder_from_compress(cinfo);
    HRESULT hr;
    ULONG byteswritten;

    hr = IStream_Write(This->stream, This->dest_buffer,
        sizeof(This->dest_buffer), &byteswritten);

    if (hr != S_OK || byteswritten == 0)
    {
        ERR("Failed writing data, hr=%x\n", hr);
        return FALSE;
    }

    This->dest_mgr.next_output_byte = This->dest_buffer;
    This->dest_mgr.free_in_buffer = sizeof(This->dest_buffer);
    return TRUE;
}

static void dest_mgr_term_destination(j_compress_ptr cinfo)
{
    JpegEncoder *This = encoder_from_compress(cinfo);
    ULONG byteswritten;
    HRESULT hr;

    if (This->dest_mgr.free_in_buffer != sizeof(This->dest_buffer))
    {
        hr = IStream_Write(This->stream, This->dest_buffer,
            sizeof(This->dest_buffer) - This->dest_mgr.free_in_buffer, &byteswritten);

        if (hr != S_OK || byteswritten == 0)
            ERR("Failed writing data, hr=%x\n", hr);
    }
}

static HRESULT WINAPI JpegEncoder_Frame_QueryInterface(IWICBitmapFrameEncode *iface, REFIID iid,
    void **ppv)
{
    JpegEncoder *This = impl_from_IWICBitmapFrameEncode(iface);
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

static ULONG WINAPI JpegEncoder_Frame_AddRef(IWICBitmapFrameEncode *iface)
{
    JpegEncoder *This = impl_from_IWICBitmapFrameEncode(iface);
    return IWICBitmapEncoder_AddRef(&This->IWICBitmapEncoder_iface);
}

static ULONG WINAPI JpegEncoder_Frame_Release(IWICBitmapFrameEncode *iface)
{
    JpegEncoder *This = impl_from_IWICBitmapFrameEncode(iface);
    return IWICBitmapEncoder_Release(&This->IWICBitmapEncoder_iface);
}

static HRESULT WINAPI JpegEncoder_Frame_Initialize(IWICBitmapFrameEncode *iface,
    IPropertyBag2 *pIEncoderOptions)
{
    JpegEncoder *This = impl_from_IWICBitmapFrameEncode(iface);
    TRACE("(%p,%p)\n", iface, pIEncoderOptions);

    EnterCriticalSection(&This->lock);

    if (This->frame_initialized)
    {
        LeaveCriticalSection(&This->lock);
        return WINCODEC_ERR_WRONGSTATE;
    }

    This->frame_initialized = TRUE;

    LeaveCriticalSection(&This->lock);

    return S_OK;
}

static HRESULT WINAPI JpegEncoder_Frame_SetSize(IWICBitmapFrameEncode *iface,
    UINT uiWidth, UINT uiHeight)
{
    JpegEncoder *This = impl_from_IWICBitmapFrameEncode(iface);
    TRACE("(%p,%u,%u)\n", iface, uiWidth, uiHeight);

    EnterCriticalSection(&This->lock);

    if (!This->frame_initialized || This->started_compress)
    {
        LeaveCriticalSection(&This->lock);
        return WINCODEC_ERR_WRONGSTATE;
    }

    This->width = uiWidth;
    This->height = uiHeight;

    LeaveCriticalSection(&This->lock);

    return S_OK;
}

static HRESULT WINAPI JpegEncoder_Frame_SetResolution(IWICBitmapFrameEncode *iface,
    double dpiX, double dpiY)
{
    JpegEncoder *This = impl_from_IWICBitmapFrameEncode(iface);
    TRACE("(%p,%0.2f,%0.2f)\n", iface, dpiX, dpiY);

    EnterCriticalSection(&This->lock);

    if (!This->frame_initialized || This->started_compress)
    {
        LeaveCriticalSection(&This->lock);
        return WINCODEC_ERR_WRONGSTATE;
    }

    This->xres = dpiX;
    This->yres = dpiY;

    LeaveCriticalSection(&This->lock);

    return S_OK;
}

static HRESULT WINAPI JpegEncoder_Frame_SetPixelFormat(IWICBitmapFrameEncode *iface,
    WICPixelFormatGUID *pPixelFormat)
{
    JpegEncoder *This = impl_from_IWICBitmapFrameEncode(iface);
    int i;
    TRACE("(%p,%s)\n", iface, debugstr_guid(pPixelFormat));

    EnterCriticalSection(&This->lock);

    if (!This->frame_initialized || This->started_compress)
    {
        LeaveCriticalSection(&This->lock);
        return WINCODEC_ERR_WRONGSTATE;
    }

    for (i=0; compress_formats[i].guid; i++)
    {
        if (memcmp(compress_formats[i].guid, pPixelFormat, sizeof(GUID)) == 0)
            break;
    }

    if (!compress_formats[i].guid) i = 0;

    This->format = &compress_formats[i];
    memcpy(pPixelFormat, This->format->guid, sizeof(GUID));

    LeaveCriticalSection(&This->lock);

    return S_OK;
}

static HRESULT WINAPI JpegEncoder_Frame_SetColorContexts(IWICBitmapFrameEncode *iface,
    UINT cCount, IWICColorContext **ppIColorContext)
{
    FIXME("(%p,%u,%p): stub\n", iface, cCount, ppIColorContext);
    return E_NOTIMPL;
}

static HRESULT WINAPI JpegEncoder_Frame_SetPalette(IWICBitmapFrameEncode *iface,
    IWICPalette *palette)
{
    JpegEncoder *This = impl_from_IWICBitmapFrameEncode(iface);
    HRESULT hr;

    TRACE("(%p,%p)\n", iface, palette);

    if (!palette) return E_INVALIDARG;

    EnterCriticalSection(&This->lock);

    if (This->frame_initialized)
        hr = IWICPalette_GetColors(palette, 256, This->palette, &This->colors);
    else
        hr = WINCODEC_ERR_NOTINITIALIZED;

    LeaveCriticalSection(&This->lock);
    return hr;
}

static HRESULT WINAPI JpegEncoder_Frame_SetThumbnail(IWICBitmapFrameEncode *iface,
    IWICBitmapSource *pIThumbnail)
{
    FIXME("(%p,%p): stub\n", iface, pIThumbnail);
    return WINCODEC_ERR_UNSUPPORTEDOPERATION;
}

static HRESULT WINAPI JpegEncoder_Frame_WritePixels(IWICBitmapFrameEncode *iface,
    UINT lineCount, UINT cbStride, UINT cbBufferSize, BYTE *pbPixels)
{
    JpegEncoder *This = impl_from_IWICBitmapFrameEncode(iface);
    jmp_buf jmpbuf;
    BYTE *swapped_data = NULL, *current_row;
    UINT line;
    int row_size;
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
        HeapFree(GetProcessHeap(), 0, swapped_data);
        return E_FAIL;
    }
    This->cinfo.client_data = jmpbuf;

    if (!This->started_compress)
    {
        This->cinfo.image_width = This->width;
        This->cinfo.image_height = This->height;
        This->cinfo.input_components = This->format->num_components;
        This->cinfo.in_color_space = This->format->color_space;

        pjpeg_set_defaults(&This->cinfo);

        if (This->xres != 0.0 && This->yres != 0.0)
        {
            This->cinfo.density_unit = 1; /* dots per inch */
            This->cinfo.X_density = This->xres;
            This->cinfo.Y_density = This->yres;
        }

        pjpeg_start_compress(&This->cinfo, TRUE);

        This->started_compress = TRUE;
    }

    row_size = This->format->bpp / 8 * This->width;

    if (This->format->swap_rgb)
    {
        swapped_data = HeapAlloc(GetProcessHeap(), 0, row_size);
        if (!swapped_data)
        {
            LeaveCriticalSection(&This->lock);
            return E_OUTOFMEMORY;
        }
    }

    for (line=0; line < lineCount; line++)
    {
        if (This->format->swap_rgb)
        {
            UINT x;

            memcpy(swapped_data, pbPixels + (cbStride * line), row_size);

            for (x=0; x < This->width; x++)
            {
                BYTE b;

                b = swapped_data[x*3];
                swapped_data[x*3] = swapped_data[x*3+2];
                swapped_data[x*3+2] = b;
            }

            current_row = swapped_data;
        }
        else
            current_row = pbPixels + (cbStride * line);

        if (!pjpeg_write_scanlines(&This->cinfo, &current_row, 1))
        {
            ERR("failed writing scanlines\n");
            LeaveCriticalSection(&This->lock);
            HeapFree(GetProcessHeap(), 0, swapped_data);
            return E_FAIL;
        }

        This->lines_written++;
    }

    LeaveCriticalSection(&This->lock);
    HeapFree(GetProcessHeap(), 0, swapped_data);

    return S_OK;
}

static HRESULT WINAPI JpegEncoder_Frame_WriteSource(IWICBitmapFrameEncode *iface,
    IWICBitmapSource *pIBitmapSource, WICRect *prc)
{
    JpegEncoder *This = impl_from_IWICBitmapFrameEncode(iface);
    HRESULT hr;
    TRACE("(%p,%p,%s)\n", iface, pIBitmapSource, debug_wic_rect(prc));

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

static HRESULT WINAPI JpegEncoder_Frame_Commit(IWICBitmapFrameEncode *iface)
{
    JpegEncoder *This = impl_from_IWICBitmapFrameEncode(iface);
    jmp_buf jmpbuf;
    TRACE("(%p)\n", iface);

    EnterCriticalSection(&This->lock);

    if (!This->started_compress || This->lines_written != This->height || This->frame_committed)
    {
        LeaveCriticalSection(&This->lock);
        return WINCODEC_ERR_WRONGSTATE;
    }

    /* set up setjmp/longjmp error handling */
    if (setjmp(jmpbuf))
    {
        LeaveCriticalSection(&This->lock);
        return E_FAIL;
    }
    This->cinfo.client_data = jmpbuf;

    pjpeg_finish_compress(&This->cinfo);

    This->frame_committed = TRUE;

    LeaveCriticalSection(&This->lock);

    return S_OK;
}

static HRESULT WINAPI JpegEncoder_Frame_GetMetadataQueryWriter(IWICBitmapFrameEncode *iface,
    IWICMetadataQueryWriter **ppIMetadataQueryWriter)
{
    FIXME("(%p, %p): stub\n", iface, ppIMetadataQueryWriter);
    return E_NOTIMPL;
}

static const IWICBitmapFrameEncodeVtbl JpegEncoder_FrameVtbl = {
    JpegEncoder_Frame_QueryInterface,
    JpegEncoder_Frame_AddRef,
    JpegEncoder_Frame_Release,
    JpegEncoder_Frame_Initialize,
    JpegEncoder_Frame_SetSize,
    JpegEncoder_Frame_SetResolution,
    JpegEncoder_Frame_SetPixelFormat,
    JpegEncoder_Frame_SetColorContexts,
    JpegEncoder_Frame_SetPalette,
    JpegEncoder_Frame_SetThumbnail,
    JpegEncoder_Frame_WritePixels,
    JpegEncoder_Frame_WriteSource,
    JpegEncoder_Frame_Commit,
    JpegEncoder_Frame_GetMetadataQueryWriter
};

static HRESULT WINAPI JpegEncoder_QueryInterface(IWICBitmapEncoder *iface, REFIID iid,
    void **ppv)
{
    JpegEncoder *This = impl_from_IWICBitmapEncoder(iface);
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

static ULONG WINAPI JpegEncoder_AddRef(IWICBitmapEncoder *iface)
{
    JpegEncoder *This = impl_from_IWICBitmapEncoder(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) refcount=%u\n", iface, ref);

    return ref;
}

static ULONG WINAPI JpegEncoder_Release(IWICBitmapEncoder *iface)
{
    JpegEncoder *This = impl_from_IWICBitmapEncoder(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) refcount=%u\n", iface, ref);

    if (ref == 0)
    {
        This->lock.DebugInfo->Spare[0] = 0;
        DeleteCriticalSection(&This->lock);
        if (This->initialized) pjpeg_destroy_compress(&This->cinfo);
        if (This->stream) IStream_Release(This->stream);
        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}

static HRESULT WINAPI JpegEncoder_Initialize(IWICBitmapEncoder *iface,
    IStream *pIStream, WICBitmapEncoderCacheOption cacheOption)
{
    JpegEncoder *This = impl_from_IWICBitmapEncoder(iface);
    jmp_buf jmpbuf;

    TRACE("(%p,%p,%u)\n", iface, pIStream, cacheOption);

    EnterCriticalSection(&This->lock);

    if (This->initialized)
    {
        LeaveCriticalSection(&This->lock);
        return WINCODEC_ERR_WRONGSTATE;
    }

    pjpeg_std_error(&This->jerr);

    This->jerr.error_exit = error_exit_fn;
    This->jerr.emit_message = emit_message_fn;

    This->cinfo.err = &This->jerr;

    This->cinfo.client_data = jmpbuf;

    if (setjmp(jmpbuf))
    {
        LeaveCriticalSection(&This->lock);
        return E_FAIL;
    }

    pjpeg_CreateCompress(&This->cinfo, JPEG_LIB_VERSION, sizeof(struct jpeg_compress_struct));

    This->stream = pIStream;
    IStream_AddRef(pIStream);

    This->dest_mgr.next_output_byte = This->dest_buffer;
    This->dest_mgr.free_in_buffer = sizeof(This->dest_buffer);

    This->dest_mgr.init_destination = dest_mgr_init_destination;
    This->dest_mgr.empty_output_buffer = dest_mgr_empty_output_buffer;
    This->dest_mgr.term_destination = dest_mgr_term_destination;

    This->cinfo.dest = &This->dest_mgr;

    This->initialized = TRUE;

    LeaveCriticalSection(&This->lock);

    return S_OK;
}

static HRESULT WINAPI JpegEncoder_GetContainerFormat(IWICBitmapEncoder *iface, GUID *format)
{
    TRACE("(%p,%p)\n", iface, format);

    if (!format)
        return E_INVALIDARG;

    memcpy(format, &GUID_ContainerFormatJpeg, sizeof(*format));
    return S_OK;
}

static HRESULT WINAPI JpegEncoder_GetEncoderInfo(IWICBitmapEncoder *iface, IWICBitmapEncoderInfo **info)
{
    IWICComponentInfo *comp_info;
    HRESULT hr;

    TRACE("%p,%p\n", iface, info);

    if (!info) return E_INVALIDARG;

    hr = CreateComponentInfo(&CLSID_WICJpegEncoder, &comp_info);
    if (hr == S_OK)
    {
        hr = IWICComponentInfo_QueryInterface(comp_info, &IID_IWICBitmapEncoderInfo, (void **)info);
        IWICComponentInfo_Release(comp_info);
    }
    return hr;
}

static HRESULT WINAPI JpegEncoder_SetColorContexts(IWICBitmapEncoder *iface,
    UINT cCount, IWICColorContext **ppIColorContext)
{
    FIXME("(%p,%u,%p): stub\n", iface, cCount, ppIColorContext);
    return E_NOTIMPL;
}

static HRESULT WINAPI JpegEncoder_SetPalette(IWICBitmapEncoder *iface, IWICPalette *pIPalette)
{
    JpegEncoder *This = impl_from_IWICBitmapEncoder(iface);
    HRESULT hr;

    TRACE("(%p,%p)\n", iface, pIPalette);

    EnterCriticalSection(&This->lock);

    hr = This->initialized ? WINCODEC_ERR_UNSUPPORTEDOPERATION : WINCODEC_ERR_NOTINITIALIZED;

    LeaveCriticalSection(&This->lock);

    return hr;
}

static HRESULT WINAPI JpegEncoder_SetThumbnail(IWICBitmapEncoder *iface, IWICBitmapSource *pIThumbnail)
{
    TRACE("(%p,%p)\n", iface, pIThumbnail);
    return WINCODEC_ERR_UNSUPPORTEDOPERATION;
}

static HRESULT WINAPI JpegEncoder_SetPreview(IWICBitmapEncoder *iface, IWICBitmapSource *pIPreview)
{
    TRACE("(%p,%p)\n", iface, pIPreview);
    return WINCODEC_ERR_UNSUPPORTEDOPERATION;
}

static HRESULT WINAPI JpegEncoder_CreateNewFrame(IWICBitmapEncoder *iface,
    IWICBitmapFrameEncode **ppIFrameEncode, IPropertyBag2 **ppIEncoderOptions)
{
    JpegEncoder *This = impl_from_IWICBitmapEncoder(iface);
    HRESULT hr;
    static const PROPBAG2 opts[6] =
    {
        { PROPBAG2_TYPE_DATA, VT_R4,            0, 0, (LPOLESTR)wszImageQuality },
        { PROPBAG2_TYPE_DATA, VT_UI1,           0, 0, (LPOLESTR)wszBitmapTransform },
        { PROPBAG2_TYPE_DATA, VT_I4 | VT_ARRAY, 0, 0, (LPOLESTR)wszLuminance },
        { PROPBAG2_TYPE_DATA, VT_I4 | VT_ARRAY, 0, 0, (LPOLESTR)wszChrominance },
        { PROPBAG2_TYPE_DATA, VT_UI1,           0, 0, (LPOLESTR)wszJpegYCrCbSubsampling },
        { PROPBAG2_TYPE_DATA, VT_BOOL,          0, 0, (LPOLESTR)wszSuppressApp0 },
    };

    TRACE("(%p,%p,%p)\n", iface, ppIFrameEncode, ppIEncoderOptions);

    EnterCriticalSection(&This->lock);

    if (This->frame_count != 0)
    {
        LeaveCriticalSection(&This->lock);
        return WINCODEC_ERR_UNSUPPORTEDOPERATION;
    }

    if (!This->initialized)
    {
        LeaveCriticalSection(&This->lock);
        return WINCODEC_ERR_NOTINITIALIZED;
    }

    if (ppIEncoderOptions)
    {
        hr = CreatePropertyBag2(opts, ARRAY_SIZE(opts), ppIEncoderOptions);
        if (FAILED(hr))
        {
            LeaveCriticalSection(&This->lock);
            return hr;
        }
    }

    This->frame_count = 1;

    LeaveCriticalSection(&This->lock);

    IWICBitmapEncoder_AddRef(iface);
    *ppIFrameEncode = &This->IWICBitmapFrameEncode_iface;

    return S_OK;
}

static HRESULT WINAPI JpegEncoder_Commit(IWICBitmapEncoder *iface)
{
    JpegEncoder *This = impl_from_IWICBitmapEncoder(iface);
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

static HRESULT WINAPI JpegEncoder_GetMetadataQueryWriter(IWICBitmapEncoder *iface,
    IWICMetadataQueryWriter **ppIMetadataQueryWriter)
{
    FIXME("(%p,%p): stub\n", iface, ppIMetadataQueryWriter);
    return E_NOTIMPL;
}

static const IWICBitmapEncoderVtbl JpegEncoder_Vtbl = {
    JpegEncoder_QueryInterface,
    JpegEncoder_AddRef,
    JpegEncoder_Release,
    JpegEncoder_Initialize,
    JpegEncoder_GetContainerFormat,
    JpegEncoder_GetEncoderInfo,
    JpegEncoder_SetColorContexts,
    JpegEncoder_SetPalette,
    JpegEncoder_SetThumbnail,
    JpegEncoder_SetPreview,
    JpegEncoder_CreateNewFrame,
    JpegEncoder_Commit,
    JpegEncoder_GetMetadataQueryWriter
};

HRESULT JpegEncoder_CreateInstance(REFIID iid, void** ppv)
{
    JpegEncoder *This;
    HRESULT ret;

    TRACE("(%s,%p)\n", debugstr_guid(iid), ppv);

    *ppv = NULL;

    if (!libjpeg_handle && !load_libjpeg())
    {
        ERR("Failed writing JPEG because unable to find %s\n",SONAME_LIBJPEG);
        return E_FAIL;
    }

    This = HeapAlloc(GetProcessHeap(), 0, sizeof(JpegEncoder));
    if (!This) return E_OUTOFMEMORY;

    This->IWICBitmapEncoder_iface.lpVtbl = &JpegEncoder_Vtbl;
    This->IWICBitmapFrameEncode_iface.lpVtbl = &JpegEncoder_FrameVtbl;
    This->ref = 1;
    This->initialized = FALSE;
    This->frame_count = 0;
    This->frame_initialized = FALSE;
    This->started_compress = FALSE;
    This->lines_written = 0;
    This->frame_committed = FALSE;
    This->committed = FALSE;
    This->width = This->height = 0;
    This->xres = This->yres = 0.0;
    This->format = NULL;
    This->stream = NULL;
    This->colors = 0;
    InitializeCriticalSection(&This->lock);
    This->lock.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": JpegEncoder.lock");

    ret = IWICBitmapEncoder_QueryInterface(&This->IWICBitmapEncoder_iface, iid, ppv);
    IWICBitmapEncoder_Release(&This->IWICBitmapEncoder_iface);

    return ret;
}

#else /* !defined(SONAME_LIBJPEG) */

HRESULT JpegDecoder_CreateInstance(REFIID iid, void** ppv)
{
    ERR("Trying to load JPEG picture, but JPEG support is not compiled in.\n");
    return E_FAIL;
}

HRESULT JpegEncoder_CreateInstance(REFIID iid, void** ppv)
{
    ERR("Trying to save JPEG picture, but JPEG support is not compiled in.\n");
    return E_FAIL;
}

#endif
