/*
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

#include <stdarg.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "objbase.h"

#include "wincodecs_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(wincodecs);

static const PROPBAG2 encoder_option_properties[ENCODER_OPTION_END] = {
    { PROPBAG2_TYPE_DATA, VT_BOOL, 0, 0, (LPOLESTR)L"InterlaceOption" },
    { PROPBAG2_TYPE_DATA, VT_UI1,  0, 0, (LPOLESTR)L"FilterOption" },
    { PROPBAG2_TYPE_DATA, VT_UI1,  0, 0, (LPOLESTR)L"TiffCompressionMethod" },
    { PROPBAG2_TYPE_DATA, VT_R4,   0, 0, (LPOLESTR)L"CompressionQuality" },
    { PROPBAG2_TYPE_DATA, VT_R4,            0, 0, (LPOLESTR)L"ImageQuality" },
    { PROPBAG2_TYPE_DATA, VT_UI1,           0, 0, (LPOLESTR)L"BitmapTransform" },
    { PROPBAG2_TYPE_DATA, VT_I4 | VT_ARRAY, 0, 0, (LPOLESTR)L"Luminance" },
    { PROPBAG2_TYPE_DATA, VT_I4 | VT_ARRAY, 0, 0, (LPOLESTR)L"Chrominance" },
    { PROPBAG2_TYPE_DATA, VT_UI1,           0, 0, (LPOLESTR)L"JpegYCrCbSubsampling" },
    { PROPBAG2_TYPE_DATA, VT_BOOL,          0, 0, (LPOLESTR)L"SuppressApp0" }
};

typedef struct CommonEncoder {
    IWICBitmapEncoder IWICBitmapEncoder_iface;
    LONG ref;
    CRITICAL_SECTION lock; /* must be held when stream or encoder is accessed */
    IStream *stream;
    struct encoder *encoder;
    struct encoder_info encoder_info;
    UINT frame_count;
    BOOL uncommitted_frame;
    BOOL committed;
} CommonEncoder;

typedef struct CommonEncoderFrame {
    IWICBitmapFrameEncode IWICBitmapFrameEncode_iface;
    IWICMetadataBlockWriter IWICMetadataBlockWriter_iface;
    LONG ref;
    CommonEncoder *parent;
    struct encoder_frame encoder_frame;
    BOOL initialized;
    BOOL frame_created;
    UINT lines_written;
    BOOL committed;
} CommonEncoderFrame;

static inline CommonEncoder *impl_from_IWICBitmapEncoder(IWICBitmapEncoder *iface)
{
    return CONTAINING_RECORD(iface, CommonEncoder, IWICBitmapEncoder_iface);
}

static inline CommonEncoderFrame *impl_from_IWICBitmapFrameEncode(IWICBitmapFrameEncode *iface)
{
    return CONTAINING_RECORD(iface, CommonEncoderFrame, IWICBitmapFrameEncode_iface);
}

static inline CommonEncoderFrame *impl_from_IWICMetadataBlockWriter(IWICMetadataBlockWriter *iface)
{
    return CONTAINING_RECORD(iface, CommonEncoderFrame, IWICMetadataBlockWriter_iface);
}

static HRESULT WINAPI CommonEncoderFrame_QueryInterface(IWICBitmapFrameEncode *iface, REFIID iid,
    void **ppv)
{
    CommonEncoderFrame *object = impl_from_IWICBitmapFrameEncode(iface);
    TRACE("(%p,%s,%p)\n", iface, debugstr_guid(iid), ppv);

    if (!ppv) return E_INVALIDARG;

    if (IsEqualIID(&IID_IUnknown, iid) ||
        IsEqualIID(&IID_IWICBitmapFrameEncode, iid))
    {
        *ppv = &object->IWICBitmapFrameEncode_iface;
    }
    else if (object->parent->encoder_info.flags & ENCODER_FLAGS_SUPPORTS_METADATA
            && IsEqualIID(&IID_IWICMetadataBlockWriter, iid))
    {
        *ppv = &object->IWICMetadataBlockWriter_iface;
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI CommonEncoderFrame_AddRef(IWICBitmapFrameEncode *iface)
{
    CommonEncoderFrame *This = impl_from_IWICBitmapFrameEncode(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) refcount=%lu\n", iface, ref);

    return ref;
}

static ULONG WINAPI CommonEncoderFrame_Release(IWICBitmapFrameEncode *iface)
{
    CommonEncoderFrame *This = impl_from_IWICBitmapFrameEncode(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) refcount=%lu\n", iface, ref);

    if (ref == 0)
    {
        IWICBitmapEncoder_Release(&This->parent->IWICBitmapEncoder_iface);
        free(This);
    }

    return ref;
}

static HRESULT WINAPI CommonEncoderFrame_Initialize(IWICBitmapFrameEncode *iface,
    IPropertyBag2 *pIEncoderOptions)
{
    CommonEncoderFrame *This = impl_from_IWICBitmapFrameEncode(iface);
    HRESULT hr=S_OK;
    struct encoder_frame options = {{0}};
    PROPBAG2 opts[7]= {{0}};
    VARIANT opt_values[7];
    HRESULT opt_hres[7];
    DWORD num_opts, i;

    TRACE("(%p,%p)\n", iface, pIEncoderOptions);

    if (pIEncoderOptions)
    {
        for (i=0; This->parent->encoder_info.encoder_options[i] != ENCODER_OPTION_END; i++)
            opts[i] = encoder_option_properties[This->parent->encoder_info.encoder_options[i]];
        num_opts = i;

        hr = IPropertyBag2_Read(pIEncoderOptions, num_opts, opts, NULL, opt_values, opt_hres);

        if (FAILED(hr))
            return hr;

        for (i=0; This->parent->encoder_info.encoder_options[i] != ENCODER_OPTION_END; i++)
        {
            VARIANT *val = &opt_values[i];

            switch (This->parent->encoder_info.encoder_options[i])
            {
            case ENCODER_OPTION_INTERLACE:
                if (V_VT(val) == VT_EMPTY)
                    options.interlace = FALSE;
                else
                    options.interlace = (V_BOOL(val) != 0);
                break;
            case ENCODER_OPTION_FILTER:
                options.filter = V_UI1(val);
                if (options.filter > WICPngFilterAdaptive)
                {
                    WARN("Unrecognized filter option value %lu.\n", options.filter);
                    options.filter = WICPngFilterUnspecified;
                }
                break;
            default:
                break;
            }
        }
    }
    else
    {
        options.interlace = FALSE;
        options.filter = WICPngFilterUnspecified;
    }

    EnterCriticalSection(&This->parent->lock);

    if (This->initialized)
        hr = WINCODEC_ERR_WRONGSTATE;
    else
    {
        This->encoder_frame = options;
        This->initialized = TRUE;
    }

    LeaveCriticalSection(&This->parent->lock);

    return hr;
}

static HRESULT WINAPI CommonEncoderFrame_SetSize(IWICBitmapFrameEncode *iface,
    UINT uiWidth, UINT uiHeight)
{
    CommonEncoderFrame *This = impl_from_IWICBitmapFrameEncode(iface);
    HRESULT hr;

    TRACE("(%p,%u,%u)\n", iface, uiWidth, uiHeight);

    EnterCriticalSection(&This->parent->lock);

    if (This->parent->encoder_info.flags & ENCODER_FLAGS_ICNS_SIZE)
    {
        if (uiWidth != uiHeight)
        {
            WARN("cannot generate ICNS icon from %dx%d image\n", uiWidth, uiHeight);
            hr = E_INVALIDARG;
            goto end;
        }

        switch (uiWidth)
        {
            case 16:
            case 32:
            case 48:
            case 128:
            case 256:
            case 512:
                break;
            default:
                WARN("cannot generate ICNS icon from %dx%d image\n", uiWidth, uiHeight);
                hr = E_INVALIDARG;
                goto end;
        }
    }

    if (!This->initialized || This->frame_created)
    {
        hr = WINCODEC_ERR_WRONGSTATE;
    }
    else
    {
        This->encoder_frame.width = uiWidth;
        This->encoder_frame.height = uiHeight;
        hr = S_OK;
    }

end:
    LeaveCriticalSection(&This->parent->lock);

    return hr;
}

static HRESULT WINAPI CommonEncoderFrame_SetResolution(IWICBitmapFrameEncode *iface,
    double dpiX, double dpiY)
{
    CommonEncoderFrame *This = impl_from_IWICBitmapFrameEncode(iface);
    HRESULT hr;

    TRACE("(%p,%0.2f,%0.2f)\n", iface, dpiX, dpiY);

    EnterCriticalSection(&This->parent->lock);

    if (!This->initialized || This->frame_created)
    {
        hr = WINCODEC_ERR_WRONGSTATE;
    }
    else
    {
        This->encoder_frame.dpix = dpiX;
        This->encoder_frame.dpiy = dpiY;
        hr = S_OK;
    }

    LeaveCriticalSection(&This->parent->lock);

    return hr;
}

static HRESULT WINAPI CommonEncoderFrame_SetPixelFormat(IWICBitmapFrameEncode *iface,
    WICPixelFormatGUID *pPixelFormat)
{
    CommonEncoderFrame *This = impl_from_IWICBitmapFrameEncode(iface);
    HRESULT hr;
    GUID pixel_format;
    DWORD bpp;
    BOOL indexed;

    TRACE("(%p,%s)\n", iface, debugstr_guid(pPixelFormat));

    EnterCriticalSection(&This->parent->lock);

    if (!This->initialized || This->frame_created)
    {
        hr = WINCODEC_ERR_WRONGSTATE;
    }
    else
    {
        pixel_format = *pPixelFormat;
        hr = encoder_get_supported_format(This->parent->encoder, &pixel_format, &bpp, &indexed);
    }

    if (SUCCEEDED(hr))
    {
        TRACE("<-- %s bpp=%li indexed=%i\n", wine_dbgstr_guid(&pixel_format), bpp, indexed);
        *pPixelFormat = pixel_format;
        This->encoder_frame.pixel_format = pixel_format;
        This->encoder_frame.bpp = bpp;
        This->encoder_frame.indexed = indexed;
    }

    LeaveCriticalSection(&This->parent->lock);

    return hr;
}

static HRESULT WINAPI CommonEncoderFrame_SetColorContexts(IWICBitmapFrameEncode *iface,
    UINT cCount, IWICColorContext **ppIColorContext)
{
    FIXME("(%p,%u,%p): stub\n", iface, cCount, ppIColorContext);
    return E_NOTIMPL;
}

static HRESULT WINAPI CommonEncoderFrame_SetPalette(IWICBitmapFrameEncode *iface,
    IWICPalette *palette)
{
    CommonEncoderFrame *This = impl_from_IWICBitmapFrameEncode(iface);
    HRESULT hr;

    TRACE("(%p,%p)\n", iface, palette);

    if (!palette)
        return E_INVALIDARG;

    EnterCriticalSection(&This->parent->lock);

    if (!This->initialized)
        hr = WINCODEC_ERR_NOTINITIALIZED;
    else if (This->frame_created)
        hr = WINCODEC_ERR_WRONGSTATE;
    else
        hr = IWICPalette_GetColors(palette, 256, This->encoder_frame.palette,
            &This->encoder_frame.num_colors);

    LeaveCriticalSection(&This->parent->lock);

    return hr;
}

static HRESULT WINAPI CommonEncoderFrame_SetThumbnail(IWICBitmapFrameEncode *iface,
    IWICBitmapSource *pIThumbnail)
{
    FIXME("(%p,%p): stub\n", iface, pIThumbnail);
    return WINCODEC_ERR_UNSUPPORTEDOPERATION;
}

static HRESULT WINAPI CommonEncoderFrame_WritePixels(IWICBitmapFrameEncode *iface,
    UINT lineCount, UINT cbStride, UINT cbBufferSize, BYTE *pbPixels)
{
    CommonEncoderFrame *This = impl_from_IWICBitmapFrameEncode(iface);
    HRESULT hr=S_OK;
    DWORD required_stride;

    TRACE("(%p,%u,%u,%u,%p)\n", iface, lineCount, cbStride, cbBufferSize, pbPixels);

    EnterCriticalSection(&This->parent->lock);

    if (!This->initialized || !This->encoder_frame.height || !This->encoder_frame.width ||
        !This->encoder_frame.bpp)
    {
        LeaveCriticalSection(&This->parent->lock);
        return WINCODEC_ERR_WRONGSTATE;
    }

    required_stride = (This->encoder_frame.width * This->encoder_frame.bpp + 7)/8;

    if (lineCount == 0 || This->encoder_frame.height - This->lines_written < lineCount ||
        cbStride < required_stride || cbBufferSize < cbStride * (lineCount - 1) + required_stride ||
        !pbPixels)
    {
        LeaveCriticalSection(&This->parent->lock);
        return E_INVALIDARG;
    }

    if (!This->frame_created)
    {
        hr = encoder_create_frame(This->parent->encoder, &This->encoder_frame);
        if (SUCCEEDED(hr))
            This->frame_created = TRUE;
    }

    if (SUCCEEDED(hr))
    {
        hr = encoder_write_lines(This->parent->encoder, pbPixels, lineCount, cbStride);
        if (SUCCEEDED(hr))
            This->lines_written += lineCount;
    }

    LeaveCriticalSection(&This->parent->lock);

    return hr;
}

static HRESULT WINAPI CommonEncoderFrame_WriteSource(IWICBitmapFrameEncode *iface,
    IWICBitmapSource *pIBitmapSource, WICRect *prc)
{
    CommonEncoderFrame *This = impl_from_IWICBitmapFrameEncode(iface);
    HRESULT hr;
    TRACE("(%p,%p,%s)\n", iface, pIBitmapSource, debug_wic_rect(prc));

    if (!This->initialized)
        return WINCODEC_ERR_WRONGSTATE;

    hr = configure_write_source(iface, pIBitmapSource, prc,
        This->encoder_frame.bpp ? &This->encoder_frame.pixel_format : NULL,
        This->encoder_frame.width, This->encoder_frame.height,
        This->encoder_frame.dpix, This->encoder_frame.dpiy);

    if (SUCCEEDED(hr))
    {
        hr = write_source(iface, pIBitmapSource, prc,
            &This->encoder_frame.pixel_format, This->encoder_frame.bpp,
            !This->encoder_frame.num_colors && This->encoder_frame.indexed,
            This->encoder_frame.width, This->encoder_frame.height);
    }

    return hr;
}

static HRESULT WINAPI CommonEncoderFrame_Commit(IWICBitmapFrameEncode *iface)
{
    CommonEncoderFrame *This = impl_from_IWICBitmapFrameEncode(iface);
    HRESULT hr;

    TRACE("(%p)\n", iface);

    EnterCriticalSection(&This->parent->lock);

    if (!This->frame_created || This->lines_written != This->encoder_frame.height ||
        This->committed)
    {
        hr = WINCODEC_ERR_WRONGSTATE;
    }
    else
    {
        hr = encoder_commit_frame(This->parent->encoder);
        if (SUCCEEDED(hr))
        {
            This->committed = TRUE;
            This->parent->uncommitted_frame = FALSE;
        }
    }

    LeaveCriticalSection(&This->parent->lock);

    return hr;
}

static HRESULT WINAPI CommonEncoderFrame_GetMetadataQueryWriter(IWICBitmapFrameEncode *iface,
    IWICMetadataQueryWriter **ppIMetadataQueryWriter)
{
    CommonEncoderFrame *encoder = impl_from_IWICBitmapFrameEncode(iface);

    TRACE("iface, %p, ppIMetadataQueryWriter %p.\n", iface, ppIMetadataQueryWriter);

    if (!ppIMetadataQueryWriter)
        return E_INVALIDARG;

    if (!encoder->initialized)
        return WINCODEC_ERR_NOTINITIALIZED;

    if (!(encoder->parent->encoder_info.flags & ENCODER_FLAGS_SUPPORTS_METADATA))
        return WINCODEC_ERR_UNSUPPORTEDOPERATION;

    return MetadataQueryWriter_CreateInstance(&encoder->IWICMetadataBlockWriter_iface, NULL, ppIMetadataQueryWriter);
}

static const IWICBitmapFrameEncodeVtbl CommonEncoderFrame_Vtbl = {
    CommonEncoderFrame_QueryInterface,
    CommonEncoderFrame_AddRef,
    CommonEncoderFrame_Release,
    CommonEncoderFrame_Initialize,
    CommonEncoderFrame_SetSize,
    CommonEncoderFrame_SetResolution,
    CommonEncoderFrame_SetPixelFormat,
    CommonEncoderFrame_SetColorContexts,
    CommonEncoderFrame_SetPalette,
    CommonEncoderFrame_SetThumbnail,
    CommonEncoderFrame_WritePixels,
    CommonEncoderFrame_WriteSource,
    CommonEncoderFrame_Commit,
    CommonEncoderFrame_GetMetadataQueryWriter
};

static HRESULT WINAPI CommonEncoder_QueryInterface(IWICBitmapEncoder *iface, REFIID iid,
    void **ppv)
{
    CommonEncoder *This = impl_from_IWICBitmapEncoder(iface);
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

static ULONG WINAPI CommonEncoder_AddRef(IWICBitmapEncoder *iface)
{
    CommonEncoder *This = impl_from_IWICBitmapEncoder(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) refcount=%lu\n", iface, ref);

    return ref;
}

static ULONG WINAPI CommonEncoder_Release(IWICBitmapEncoder *iface)
{
    CommonEncoder *This = impl_from_IWICBitmapEncoder(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) refcount=%lu\n", iface, ref);

    if (ref == 0)
    {
        This->lock.DebugInfo->Spare[0] = 0;
        DeleteCriticalSection(&This->lock);
        if (This->stream)
            IStream_Release(This->stream);
        encoder_destroy(This->encoder);
        free(This);
    }

    return ref;
}

static HRESULT WINAPI CommonEncoder_Initialize(IWICBitmapEncoder *iface,
    IStream *pIStream, WICBitmapEncoderCacheOption cacheOption)
{
    CommonEncoder *This = impl_from_IWICBitmapEncoder(iface);
    HRESULT hr;

    TRACE("(%p,%p,%u)\n", iface, pIStream, cacheOption);

    if (!pIStream)
        return E_POINTER;

    EnterCriticalSection(&This->lock);

    if (This->stream)
    {
        LeaveCriticalSection(&This->lock);
        return WINCODEC_ERR_WRONGSTATE;
    }

    hr = encoder_initialize(This->encoder, pIStream);

    if (SUCCEEDED(hr))
    {
        This->stream = pIStream;
        IStream_AddRef(This->stream);
    }

    LeaveCriticalSection(&This->lock);

    return S_OK;
}

static HRESULT WINAPI CommonEncoder_GetContainerFormat(IWICBitmapEncoder *iface, GUID *format)
{
    CommonEncoder *This = impl_from_IWICBitmapEncoder(iface);
    TRACE("(%p,%p)\n", iface, format);

    if (!format)
        return E_INVALIDARG;

    memcpy(format, &This->encoder_info.container_format, sizeof(*format));
    return S_OK;
}

static HRESULT WINAPI CommonEncoder_GetEncoderInfo(IWICBitmapEncoder *iface, IWICBitmapEncoderInfo **info)
{
    CommonEncoder *This = impl_from_IWICBitmapEncoder(iface);
    IWICComponentInfo *comp_info;
    HRESULT hr;

    TRACE("%p,%p\n", iface, info);

    if (!info) return E_INVALIDARG;

    hr = CreateComponentInfo(&This->encoder_info.clsid, &comp_info);
    if (hr == S_OK)
    {
        hr = IWICComponentInfo_QueryInterface(comp_info, &IID_IWICBitmapEncoderInfo, (void **)info);
        IWICComponentInfo_Release(comp_info);
    }
    return hr;
}

static HRESULT WINAPI CommonEncoder_SetColorContexts(IWICBitmapEncoder *iface,
    UINT cCount, IWICColorContext **ppIColorContext)
{
    FIXME("(%p,%u,%p): stub\n", iface, cCount, ppIColorContext);
    return E_NOTIMPL;
}

static HRESULT WINAPI CommonEncoder_SetPalette(IWICBitmapEncoder *iface, IWICPalette *palette)
{
    CommonEncoder *This = impl_from_IWICBitmapEncoder(iface);
    HRESULT hr;

    TRACE("(%p,%p)\n", iface, palette);

    EnterCriticalSection(&This->lock);

    hr = This->stream ? WINCODEC_ERR_UNSUPPORTEDOPERATION : WINCODEC_ERR_NOTINITIALIZED;

    LeaveCriticalSection(&This->lock);

    return hr;
}

static HRESULT WINAPI CommonEncoder_SetThumbnail(IWICBitmapEncoder *iface, IWICBitmapSource *pIThumbnail)
{
    TRACE("(%p,%p)\n", iface, pIThumbnail);
    return WINCODEC_ERR_UNSUPPORTEDOPERATION;
}

static HRESULT WINAPI CommonEncoder_SetPreview(IWICBitmapEncoder *iface, IWICBitmapSource *pIPreview)
{
    TRACE("(%p,%p)\n", iface, pIPreview);
    return WINCODEC_ERR_UNSUPPORTEDOPERATION;
}

static HRESULT WINAPI CommonEncoderFrame_Block_QueryInterface(IWICMetadataBlockWriter *iface, REFIID iid, void **ppv)
{
    CommonEncoderFrame *encoder = impl_from_IWICMetadataBlockWriter(iface);

    return IWICBitmapFrameEncode_QueryInterface(&encoder->IWICBitmapFrameEncode_iface, iid, ppv);
}

static ULONG WINAPI CommonEncoderFrame_Block_AddRef(IWICMetadataBlockWriter *iface)
{
    CommonEncoderFrame *encoder = impl_from_IWICMetadataBlockWriter(iface);

    return IWICBitmapFrameEncode_AddRef(&encoder->IWICBitmapFrameEncode_iface);
}

static ULONG WINAPI CommonEncoderFrame_Block_Release(IWICMetadataBlockWriter *iface)
{
    CommonEncoderFrame *encoder = impl_from_IWICMetadataBlockWriter(iface);

    return IWICBitmapFrameEncode_Release(&encoder->IWICBitmapFrameEncode_iface);
}

static HRESULT WINAPI CommonEncoderFrame_Block_GetContainerFormat(IWICMetadataBlockWriter *iface, GUID *container_format)
{
    FIXME("iface %p, container_format %p stub.\n", iface, container_format);

    return E_NOTIMPL;
}

static HRESULT WINAPI CommonEncoderFrame_Block_GetCount(IWICMetadataBlockWriter *iface, UINT *count)
{
    FIXME("iface %p, count %p stub.\n", iface, count);

    return E_NOTIMPL;
}

static HRESULT WINAPI CommonEncoderFrame_Block_GetReaderByIndex(IWICMetadataBlockWriter *iface,
        UINT index, IWICMetadataReader **metadata_reader)
{
    FIXME("iface %p, index %d, metadata_reader %p stub.\n", iface, index, metadata_reader);

    return E_NOTIMPL;
}

static HRESULT WINAPI CommonEncoderFrame_Block_GetEnumerator(IWICMetadataBlockWriter *iface, IEnumUnknown **enum_metadata)
{
    FIXME("iface %p, ppIEnumMetadata %p stub.\n", iface, enum_metadata);

    return E_NOTIMPL;
}

static HRESULT WINAPI CommonEncoderFrame_Block_InitializeFromBlockReader(IWICMetadataBlockWriter *iface,
        IWICMetadataBlockReader *metadata_block_reader)
{
    FIXME("iface %p, metadata_block_reader %p stub.\n", iface, metadata_block_reader);

    return E_NOTIMPL;
}

static HRESULT WINAPI CommonEncoderFrame_Block_GetWriterByIndex(IWICMetadataBlockWriter *iface, UINT index,
        IWICMetadataWriter **metadata_writer)
{
    FIXME("iface %p, index %u, metadata_writer %p stub.\n", iface, index, metadata_writer);

    return E_NOTIMPL;
}

static HRESULT WINAPI CommonEncoderFrame_Block_AddWriter(IWICMetadataBlockWriter *iface, IWICMetadataWriter *metadata_writer)
{
    FIXME("iface %p, metadata_writer %p.\n", iface, metadata_writer);

    return E_NOTIMPL;
}

static HRESULT WINAPI CommonEncoderFrame_Block_SetWriterByIndex(IWICMetadataBlockWriter *iface, UINT index,
        IWICMetadataWriter *metadata_writer)
{
    FIXME("iface %p, index %u, metadata_writer %p stub.\n", iface, index, metadata_writer);

    return E_NOTIMPL;
}

static HRESULT WINAPI CommonEncoderFrame_Block_RemoveWriterByIndex(IWICMetadataBlockWriter *iface, UINT index)
{
    FIXME("iface %p, index %u.\n", iface, index);

    return E_NOTIMPL;
}

static const IWICMetadataBlockWriterVtbl CommonEncoderFrame_BlockVtbl =
{
    CommonEncoderFrame_Block_QueryInterface,
    CommonEncoderFrame_Block_AddRef,
    CommonEncoderFrame_Block_Release,
    CommonEncoderFrame_Block_GetContainerFormat,
    CommonEncoderFrame_Block_GetCount,
    CommonEncoderFrame_Block_GetReaderByIndex,
    CommonEncoderFrame_Block_GetEnumerator,
    CommonEncoderFrame_Block_InitializeFromBlockReader,
    CommonEncoderFrame_Block_GetWriterByIndex,
    CommonEncoderFrame_Block_AddWriter,
    CommonEncoderFrame_Block_SetWriterByIndex,
    CommonEncoderFrame_Block_RemoveWriterByIndex,
};

static HRESULT WINAPI CommonEncoder_CreateNewFrame(IWICBitmapEncoder *iface,
    IWICBitmapFrameEncode **ppIFrameEncode, IPropertyBag2 **ppIEncoderOptions)
{
    CommonEncoder *This = impl_from_IWICBitmapEncoder(iface);
    CommonEncoderFrame *result;
    HRESULT hr;
    DWORD opts_length;
    PROPBAG2 opts[6];

    TRACE("(%p,%p,%p)\n", iface, ppIFrameEncode, ppIEncoderOptions);

    EnterCriticalSection(&This->lock);

    if (This->frame_count != 0 && !(This->encoder_info.flags & ENCODER_FLAGS_MULTI_FRAME))
    {
        LeaveCriticalSection(&This->lock);
        return WINCODEC_ERR_UNSUPPORTEDOPERATION;
    }

    if (!This->stream || This->committed || This->uncommitted_frame)
    {
        LeaveCriticalSection(&This->lock);
        return WINCODEC_ERR_NOTINITIALIZED;
    }

    result = calloc(1, sizeof(*result));
    if (!result)
    {
        LeaveCriticalSection(&This->lock);
        return E_OUTOFMEMORY;
    }

    result->IWICBitmapFrameEncode_iface.lpVtbl = &CommonEncoderFrame_Vtbl;
    result->IWICMetadataBlockWriter_iface.lpVtbl = &CommonEncoderFrame_BlockVtbl;
    result->ref = 1;
    result->parent = This;

    if (ppIEncoderOptions)
    {
        for (opts_length = 0; This->encoder_info.encoder_options[opts_length] < ENCODER_OPTION_END; opts_length++)
        {
            opts[opts_length] = encoder_option_properties[This->encoder_info.encoder_options[opts_length]];
        }

        hr = CreatePropertyBag2(opts, opts_length, ppIEncoderOptions);
        if (FAILED(hr))
        {
            LeaveCriticalSection(&This->lock);
            free(result);
            return hr;
        }
    }

    IWICBitmapEncoder_AddRef(iface);
    This->frame_count++;
    This->uncommitted_frame = TRUE;

    LeaveCriticalSection(&This->lock);

    *ppIFrameEncode = &result->IWICBitmapFrameEncode_iface;

    return S_OK;
}

static HRESULT WINAPI CommonEncoder_Commit(IWICBitmapEncoder *iface)
{
    CommonEncoder *This = impl_from_IWICBitmapEncoder(iface);
    HRESULT hr;

    TRACE("(%p)\n", iface);

    EnterCriticalSection(&This->lock);

    if (This->committed || This->uncommitted_frame)
        hr = WINCODEC_ERR_WRONGSTATE;
    else
    {
        hr = encoder_commit_file(This->encoder);
        if (SUCCEEDED(hr))
            This->committed = TRUE;
    }

    LeaveCriticalSection(&This->lock);

    return hr;
}

static HRESULT WINAPI CommonEncoder_GetMetadataQueryWriter(IWICBitmapEncoder *iface,
    IWICMetadataQueryWriter **ppIMetadataQueryWriter)
{
    FIXME("(%p,%p): stub\n", iface, ppIMetadataQueryWriter);
    return E_NOTIMPL;
}

static const IWICBitmapEncoderVtbl CommonEncoder_Vtbl = {
    CommonEncoder_QueryInterface,
    CommonEncoder_AddRef,
    CommonEncoder_Release,
    CommonEncoder_Initialize,
    CommonEncoder_GetContainerFormat,
    CommonEncoder_GetEncoderInfo,
    CommonEncoder_SetColorContexts,
    CommonEncoder_SetPalette,
    CommonEncoder_SetThumbnail,
    CommonEncoder_SetPreview,
    CommonEncoder_CreateNewFrame,
    CommonEncoder_Commit,
    CommonEncoder_GetMetadataQueryWriter
};

HRESULT CommonEncoder_CreateInstance(struct encoder *encoder,
    const struct encoder_info *encoder_info, REFIID iid, void** ppv)
{
    CommonEncoder *This;
    HRESULT ret;

    TRACE("(%s,%p)\n", debugstr_guid(iid), ppv);

    *ppv = NULL;

    This = malloc(sizeof(CommonEncoder));
    if (!This)
    {
        encoder_destroy(encoder);
        return E_OUTOFMEMORY;
    }

    This->IWICBitmapEncoder_iface.lpVtbl = &CommonEncoder_Vtbl;
    This->ref = 1;
    This->stream = NULL;
    This->encoder = encoder;
    This->encoder_info = *encoder_info;
    This->frame_count = 0;
    This->uncommitted_frame = FALSE;
    This->committed = FALSE;
#ifdef __REACTOS__
    InitializeCriticalSection(&This->lock);
#else
    InitializeCriticalSectionEx(&This->lock, 0, RTL_CRITICAL_SECTION_FLAG_FORCE_DEBUG_INFO);
#endif
    This->lock.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": CommonEncoder.lock");

    ret = IWICBitmapEncoder_QueryInterface(&This->IWICBitmapEncoder_iface, iid, ppv);
    IWICBitmapEncoder_Release(&This->IWICBitmapEncoder_iface);

    return ret;
}
