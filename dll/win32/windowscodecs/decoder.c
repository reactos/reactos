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

typedef struct {
    IWICBitmapDecoder IWICBitmapDecoder_iface;
    LONG ref;
    CRITICAL_SECTION lock; /* must be held when stream or decoder is accessed */
    IStream *stream;
    struct decoder *decoder;
    struct decoder_info decoder_info;
    struct decoder_stat file_info;
    WICDecodeOptions cache_options;
} CommonDecoder;

static inline CommonDecoder *impl_from_IWICBitmapDecoder(IWICBitmapDecoder *iface)
{
    return CONTAINING_RECORD(iface, CommonDecoder, IWICBitmapDecoder_iface);
}

static HRESULT WINAPI CommonDecoder_QueryInterface(IWICBitmapDecoder *iface, REFIID iid,
    void **ppv)
{
    CommonDecoder *This = impl_from_IWICBitmapDecoder(iface);
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

static ULONG WINAPI CommonDecoder_AddRef(IWICBitmapDecoder *iface)
{
    CommonDecoder *This = impl_from_IWICBitmapDecoder(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) refcount=%lu\n", iface, ref);

    return ref;
}

static ULONG WINAPI CommonDecoder_Release(IWICBitmapDecoder *iface)
{
    CommonDecoder *This = impl_from_IWICBitmapDecoder(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) refcount=%lu\n", iface, ref);

    if (ref == 0)
    {
        if (This->stream)
            IStream_Release(This->stream);
        This->lock.DebugInfo->Spare[0] = 0;
        DeleteCriticalSection(&This->lock);
        decoder_destroy(This->decoder);
        free(This);
    }

    return ref;
}

static HRESULT WINAPI CommonDecoder_QueryCapability(IWICBitmapDecoder *iface, IStream *stream,
    DWORD *capability)
{
    CommonDecoder *This = impl_from_IWICBitmapDecoder(iface);
    HRESULT hr;

    TRACE("(%p,%p,%p)\n", iface, stream, capability);

    if (!stream || !capability) return E_INVALIDARG;

    hr = IWICBitmapDecoder_Initialize(iface, stream, WICDecodeMetadataCacheOnDemand);
    if (hr != S_OK) return hr;

    *capability = (This->file_info.flags & DECODER_FLAGS_CAPABILITY_MASK);
    return S_OK;
}

static HRESULT WINAPI CommonDecoder_Initialize(IWICBitmapDecoder *iface, IStream *pIStream,
    WICDecodeOptions cacheOptions)
{
    CommonDecoder *This = impl_from_IWICBitmapDecoder(iface);
    HRESULT hr=S_OK;

    TRACE("(%p,%p,%x)\n", iface, pIStream, cacheOptions);

    EnterCriticalSection(&This->lock);

    if (This->stream)
        hr = WINCODEC_ERR_WRONGSTATE;

    if (SUCCEEDED(hr))
        hr = decoder_initialize(This->decoder, pIStream, &This->file_info);

    if (SUCCEEDED(hr))
    {
        This->cache_options = cacheOptions;
        This->stream = pIStream;
        IStream_AddRef(This->stream);
    }

    LeaveCriticalSection(&This->lock);

    return hr;
}

static HRESULT WINAPI CommonDecoder_GetContainerFormat(IWICBitmapDecoder *iface,
    GUID *pguidContainerFormat)
{
    CommonDecoder *This = impl_from_IWICBitmapDecoder(iface);
    memcpy(pguidContainerFormat, &This->decoder_info.container_format, sizeof(GUID));
    return S_OK;
}

static HRESULT WINAPI CommonDecoder_GetDecoderInfo(IWICBitmapDecoder *iface,
    IWICBitmapDecoderInfo **ppIDecoderInfo)
{
    CommonDecoder *This = impl_from_IWICBitmapDecoder(iface);
    TRACE("(%p,%p)\n", iface, ppIDecoderInfo);

    return get_decoder_info(&This->decoder_info.clsid, ppIDecoderInfo);
}

static HRESULT WINAPI CommonDecoder_CopyPalette(IWICBitmapDecoder *iface,
    IWICPalette *palette)
{
    TRACE("(%p,%p)\n", iface, palette);
    return WINCODEC_ERR_PALETTEUNAVAILABLE;
}

static HRESULT WINAPI CommonDecoder_GetMetadataQueryReader(IWICBitmapDecoder *iface,
    IWICMetadataQueryReader **reader)
{
    TRACE("(%p,%p)\n", iface, reader);

    if (!reader) return E_INVALIDARG;

    *reader = NULL;
    return WINCODEC_ERR_UNSUPPORTEDOPERATION;
}

static HRESULT WINAPI CommonDecoder_GetPreview(IWICBitmapDecoder *iface,
    IWICBitmapSource **ppIBitmapSource)
{
    TRACE("(%p,%p)\n", iface, ppIBitmapSource);

    if (!ppIBitmapSource) return E_INVALIDARG;

    *ppIBitmapSource = NULL;
    return WINCODEC_ERR_UNSUPPORTEDOPERATION;
}

static HRESULT WINAPI CommonDecoder_GetColorContexts(IWICBitmapDecoder *iface,
    UINT cCount, IWICColorContext **ppIColorContexts, UINT *pcActualCount)
{
    TRACE("(%p,%u,%p,%p)\n", iface, cCount, ppIColorContexts, pcActualCount);
    return WINCODEC_ERR_UNSUPPORTEDOPERATION;
}

static HRESULT WINAPI CommonDecoder_GetThumbnail(IWICBitmapDecoder *iface,
    IWICBitmapSource **ppIThumbnail)
{
    TRACE("(%p,%p)\n", iface, ppIThumbnail);

    if (!ppIThumbnail) return E_INVALIDARG;

    *ppIThumbnail = NULL;
    return WINCODEC_ERR_CODECNOTHUMBNAIL;
}

static HRESULT WINAPI CommonDecoder_GetFrameCount(IWICBitmapDecoder *iface,
    UINT *pCount)
{
    CommonDecoder *This = impl_from_IWICBitmapDecoder(iface);
    if (!pCount) return E_INVALIDARG;

    if (This->stream)
        *pCount = This->file_info.frame_count;
    else
        *pCount = 0;

    return S_OK;
}

static HRESULT WINAPI CommonDecoder_GetFrame(IWICBitmapDecoder *iface,
    UINT index, IWICBitmapFrameDecode **ppIBitmapFrame);

static const IWICBitmapDecoderVtbl CommonDecoder_Vtbl = {
    CommonDecoder_QueryInterface,
    CommonDecoder_AddRef,
    CommonDecoder_Release,
    CommonDecoder_QueryCapability,
    CommonDecoder_Initialize,
    CommonDecoder_GetContainerFormat,
    CommonDecoder_GetDecoderInfo,
    CommonDecoder_CopyPalette,
    CommonDecoder_GetMetadataQueryReader,
    CommonDecoder_GetPreview,
    CommonDecoder_GetColorContexts,
    CommonDecoder_GetThumbnail,
    CommonDecoder_GetFrameCount,
    CommonDecoder_GetFrame
};

typedef struct {
    IWICBitmapFrameDecode IWICBitmapFrameDecode_iface;
    IWICMetadataBlockReader IWICMetadataBlockReader_iface;
    LONG ref;
    CommonDecoder *parent;
    DWORD frame;
    struct decoder_frame decoder_frame;
    BOOL metadata_initialized;
    UINT metadata_count;
    struct decoder_block* metadata_blocks;
} CommonDecoderFrame;

static inline CommonDecoderFrame *impl_from_IWICBitmapFrameDecode(IWICBitmapFrameDecode *iface)
{
    return CONTAINING_RECORD(iface, CommonDecoderFrame, IWICBitmapFrameDecode_iface);
}

static inline CommonDecoderFrame *impl_from_IWICMetadataBlockReader(IWICMetadataBlockReader *iface)
{
    return CONTAINING_RECORD(iface, CommonDecoderFrame, IWICMetadataBlockReader_iface);
}

static HRESULT WINAPI CommonDecoderFrame_QueryInterface(IWICBitmapFrameDecode *iface, REFIID iid,
    void **ppv)
{
    CommonDecoderFrame *This = impl_from_IWICBitmapFrameDecode(iface);
    if (!ppv) return E_INVALIDARG;

    if (IsEqualIID(&IID_IUnknown, iid) ||
        IsEqualIID(&IID_IWICBitmapSource, iid) ||
        IsEqualIID(&IID_IWICBitmapFrameDecode, iid))
    {
        *ppv = &This->IWICBitmapFrameDecode_iface;
    }
    else if (IsEqualIID(&IID_IWICMetadataBlockReader, iid) &&
             (This->parent->file_info.flags & WICBitmapDecoderCapabilityCanEnumerateMetadata))
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

static ULONG WINAPI CommonDecoderFrame_AddRef(IWICBitmapFrameDecode *iface)
{
    CommonDecoderFrame *This = impl_from_IWICBitmapFrameDecode(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) refcount=%lu\n", iface, ref);

    return ref;
}

static ULONG WINAPI CommonDecoderFrame_Release(IWICBitmapFrameDecode *iface)
{
    CommonDecoderFrame *This = impl_from_IWICBitmapFrameDecode(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) refcount=%lu\n", iface, ref);

    if (ref == 0)
    {
        IWICBitmapDecoder_Release(&This->parent->IWICBitmapDecoder_iface);
        free(This->metadata_blocks);
        free(This);
    }

    return ref;
}

static HRESULT WINAPI CommonDecoderFrame_GetSize(IWICBitmapFrameDecode *iface,
    UINT *puiWidth, UINT *puiHeight)
{
    CommonDecoderFrame *This = impl_from_IWICBitmapFrameDecode(iface);

    TRACE("(%p,%p,%p)\n", This, puiWidth, puiHeight);

    if (!puiWidth || !puiHeight)
        return E_POINTER;

    *puiWidth = This->decoder_frame.width;
    *puiHeight = This->decoder_frame.height;
    return S_OK;
}

static HRESULT WINAPI CommonDecoderFrame_GetPixelFormat(IWICBitmapFrameDecode *iface,
    WICPixelFormatGUID *pPixelFormat)
{
    CommonDecoderFrame *This = impl_from_IWICBitmapFrameDecode(iface);

    TRACE("(%p,%p)\n", This, pPixelFormat);

    if (!pPixelFormat)
        return E_POINTER;

    *pPixelFormat = This->decoder_frame.pixel_format;
    return S_OK;
}

static HRESULT WINAPI CommonDecoderFrame_GetResolution(IWICBitmapFrameDecode *iface,
    double *pDpiX, double *pDpiY)
{
    CommonDecoderFrame *This = impl_from_IWICBitmapFrameDecode(iface);

    TRACE("(%p,%p,%p)\n", This, pDpiX, pDpiY);

    if (!pDpiX || !pDpiY)
        return E_POINTER;

    *pDpiX = This->decoder_frame.dpix;
    *pDpiY = This->decoder_frame.dpiy;
    return S_OK;
}

static HRESULT WINAPI CommonDecoderFrame_CopyPalette(IWICBitmapFrameDecode *iface,
    IWICPalette *pIPalette)
{
    CommonDecoderFrame *This = impl_from_IWICBitmapFrameDecode(iface);
    HRESULT hr=S_OK;

    TRACE("(%p,%p)\n", iface, pIPalette);

    if (This->decoder_frame.num_colors)
    {
        hr = IWICPalette_InitializeCustom(pIPalette, This->decoder_frame.palette, This->decoder_frame.num_colors);
    }
    else
    {
        hr = WINCODEC_ERR_PALETTEUNAVAILABLE;
    }

    return hr;
}

static HRESULT WINAPI CommonDecoderFrame_CopyPixels(IWICBitmapFrameDecode *iface,
    const WICRect *prc, UINT cbStride, UINT cbBufferSize, BYTE *pbBuffer)
{
    CommonDecoderFrame *This = impl_from_IWICBitmapFrameDecode(iface);
    HRESULT hr;
    UINT bytesperrow;
    WICRect rect;

    TRACE("(%p,%s,%u,%u,%p)\n", iface, debug_wic_rect(prc), cbStride, cbBufferSize, pbBuffer);

    if (!pbBuffer)
        return E_POINTER;

    if (!prc)
    {
        rect.X = 0;
        rect.Y = 0;
        rect.Width = This->decoder_frame.width;
        rect.Height = This->decoder_frame.height;
        prc = &rect;
    }
    else
    {
        if (prc->X < 0 || prc->Y < 0 ||
            prc->X+prc->Width > This->decoder_frame.width ||
            prc->Y+prc->Height > This->decoder_frame.height)
            return E_INVALIDARG;
    }

    bytesperrow = ((This->decoder_frame.bpp * prc->Width)+7)/8;

    if (cbStride < bytesperrow)
        return E_INVALIDARG;

    if ((cbStride * (prc->Height-1)) + bytesperrow > cbBufferSize)
        return E_INVALIDARG;

    EnterCriticalSection(&This->parent->lock);

    hr = decoder_copy_pixels(This->parent->decoder, This->frame,
        prc, cbStride, cbBufferSize, pbBuffer);

    LeaveCriticalSection(&This->parent->lock);

    return hr;
}

static HRESULT WINAPI CommonDecoderFrame_GetMetadataQueryReader(IWICBitmapFrameDecode *iface,
    IWICMetadataQueryReader **ppIMetadataQueryReader)
{
    CommonDecoderFrame *This = impl_from_IWICBitmapFrameDecode(iface);
    IWICComponentFactory* factory;
    HRESULT hr;

    TRACE("(%p,%p)\n", iface, ppIMetadataQueryReader);

    if (!ppIMetadataQueryReader)
        return E_INVALIDARG;

    if (!(This->parent->file_info.flags & WICBitmapDecoderCapabilityCanEnumerateMetadata))
        return WINCODEC_ERR_UNSUPPORTEDOPERATION;

    hr = create_instance(&CLSID_WICImagingFactory, &IID_IWICComponentFactory, (void**)&factory);

    if (SUCCEEDED(hr))
    {
        hr = IWICComponentFactory_CreateQueryReaderFromBlockReader(factory, &This->IWICMetadataBlockReader_iface, ppIMetadataQueryReader);
        IWICComponentFactory_Release(factory);
    }

    if (FAILED(hr))
        *ppIMetadataQueryReader = NULL;

    return hr;
}

static HRESULT WINAPI CommonDecoderFrame_GetColorContexts(IWICBitmapFrameDecode *iface,
    UINT cCount, IWICColorContext **ppIColorContexts, UINT *pcActualCount)
{
    CommonDecoderFrame *This = impl_from_IWICBitmapFrameDecode(iface);
    HRESULT hr=S_OK;
    UINT i;
    BYTE *profile;
    DWORD profile_len;

    TRACE("(%p,%u,%p,%p)\n", iface, cCount, ppIColorContexts, pcActualCount);

    if (!pcActualCount) return E_INVALIDARG;

    if (This->parent->file_info.flags & DECODER_FLAGS_UNSUPPORTED_COLOR_CONTEXT)
    {
        FIXME("not supported for %s\n", wine_dbgstr_guid(&This->parent->decoder_info.clsid));
        *pcActualCount = 0;
        return WINCODEC_ERR_UNSUPPORTEDOPERATION;
    }

    *pcActualCount = This->decoder_frame.num_color_contexts;

    if (This->decoder_frame.num_color_contexts && cCount && ppIColorContexts)
    {
        if (cCount >= This->decoder_frame.num_color_contexts)
        {
            EnterCriticalSection(&This->parent->lock);

            for (i=0; i<This->decoder_frame.num_color_contexts; i++)
            {
                hr = decoder_get_color_context(This->parent->decoder, This->frame, i,
                    &profile, &profile_len);
                if (SUCCEEDED(hr))
                {
                    hr = IWICColorContext_InitializeFromMemory(ppIColorContexts[i], profile, profile_len);

                    free(profile);
                }

                if (FAILED(hr))
                    break;
            }

            LeaveCriticalSection(&This->parent->lock);
        }
        else
        {
            hr = E_INVALIDARG;
        }
    }

    return hr;
}

static HRESULT WINAPI CommonDecoderFrame_GetThumbnail(IWICBitmapFrameDecode *iface,
    IWICBitmapSource **ppIThumbnail)
{
    TRACE("(%p,%p)\n", iface, ppIThumbnail);

    if (!ppIThumbnail) return E_INVALIDARG;

    *ppIThumbnail = NULL;
    return WINCODEC_ERR_CODECNOTHUMBNAIL;
}

static const IWICBitmapFrameDecodeVtbl CommonDecoderFrameVtbl = {
    CommonDecoderFrame_QueryInterface,
    CommonDecoderFrame_AddRef,
    CommonDecoderFrame_Release,
    CommonDecoderFrame_GetSize,
    CommonDecoderFrame_GetPixelFormat,
    CommonDecoderFrame_GetResolution,
    CommonDecoderFrame_CopyPalette,
    CommonDecoderFrame_CopyPixels,
    CommonDecoderFrame_GetMetadataQueryReader,
    CommonDecoderFrame_GetColorContexts,
    CommonDecoderFrame_GetThumbnail
};

static HRESULT WINAPI CommonDecoderFrame_Block_QueryInterface(IWICMetadataBlockReader *iface, REFIID iid,
    void **ppv)
{
    CommonDecoderFrame *This = impl_from_IWICMetadataBlockReader(iface);
    return IWICBitmapFrameDecode_QueryInterface(&This->IWICBitmapFrameDecode_iface, iid, ppv);
}

static ULONG WINAPI CommonDecoderFrame_Block_AddRef(IWICMetadataBlockReader *iface)
{
    CommonDecoderFrame *This = impl_from_IWICMetadataBlockReader(iface);
    return IWICBitmapFrameDecode_AddRef(&This->IWICBitmapFrameDecode_iface);
}

static ULONG WINAPI CommonDecoderFrame_Block_Release(IWICMetadataBlockReader *iface)
{
    CommonDecoderFrame *This = impl_from_IWICMetadataBlockReader(iface);
    return IWICBitmapFrameDecode_Release(&This->IWICBitmapFrameDecode_iface);
}

static HRESULT WINAPI CommonDecoderFrame_Block_GetContainerFormat(IWICMetadataBlockReader *iface,
    GUID *pguidContainerFormat)
{
    CommonDecoderFrame *This = impl_from_IWICMetadataBlockReader(iface);
    if (!pguidContainerFormat) return E_INVALIDARG;
    *pguidContainerFormat = This->parent->decoder_info.block_format;
    return S_OK;
}

static HRESULT CommonDecoderFrame_InitializeMetadata(CommonDecoderFrame *This)
{
    HRESULT hr=S_OK;

    if (This->metadata_initialized)
        return S_OK;

    EnterCriticalSection(&This->parent->lock);

    if (!This->metadata_initialized)
    {
        hr = decoder_get_metadata_blocks(This->parent->decoder, This->frame, &This->metadata_count, &This->metadata_blocks);
        if (SUCCEEDED(hr))
            This->metadata_initialized = TRUE;
    }

    LeaveCriticalSection(&This->parent->lock);

    return hr;
}

static HRESULT WINAPI CommonDecoderFrame_Block_GetCount(IWICMetadataBlockReader *iface,
    UINT *pcCount)
{
    CommonDecoderFrame *This = impl_from_IWICMetadataBlockReader(iface);
    HRESULT hr;

    TRACE("%p,%p\n", iface, pcCount);

    if (!pcCount) return E_INVALIDARG;

    hr = CommonDecoderFrame_InitializeMetadata(This);
    if (SUCCEEDED(hr))
        *pcCount = This->metadata_count;

    return hr;
}

static HRESULT WINAPI CommonDecoderFrame_Block_GetReaderByIndex(IWICMetadataBlockReader *iface,
    UINT nIndex, IWICMetadataReader **ppIMetadataReader)
{
    CommonDecoderFrame *This = impl_from_IWICMetadataBlockReader(iface);
    HRESULT hr;
    IWICComponentFactory* factory = NULL;
    IWICStream* stream;

    TRACE("%p,%d,%p\n", iface, nIndex, ppIMetadataReader);

    if (!ppIMetadataReader)
        return E_INVALIDARG;

    hr = CommonDecoderFrame_InitializeMetadata(This);

    if (SUCCEEDED(hr) && nIndex >= This->metadata_count)
        hr = E_INVALIDARG;

    if (SUCCEEDED(hr))
        hr = create_instance(&CLSID_WICImagingFactory, &IID_IWICComponentFactory, (void**)&factory);

    if (SUCCEEDED(hr))
        hr = IWICComponentFactory_CreateStream(factory, &stream);

    if (SUCCEEDED(hr))
    {
        if (This->metadata_blocks[nIndex].options & DECODER_BLOCK_FULL_STREAM)
        {
            LARGE_INTEGER offset;
            offset.QuadPart = This->metadata_blocks[nIndex].offset;

            hr = IWICStream_InitializeFromIStream(stream, This->parent->stream);

            if (SUCCEEDED(hr))
                hr = IWICStream_Seek(stream, offset, STREAM_SEEK_SET, NULL);
        }
        else
        {
            ULARGE_INTEGER offset, length;

            offset.QuadPart = This->metadata_blocks[nIndex].offset;
            length.QuadPart = This->metadata_blocks[nIndex].length;

            hr = IWICStream_InitializeFromIStreamRegion(stream, This->parent->stream,
                offset, length);
        }

        if (This->metadata_blocks[nIndex].options & DECODER_BLOCK_READER_CLSID)
        {
            IWICMetadataReader *reader;
            IWICPersistStream *persist;
            if (SUCCEEDED(hr))
            {
                hr = create_instance(&This->metadata_blocks[nIndex].reader_clsid,
                    &IID_IWICMetadataReader, (void**)&reader);
            }

            if (SUCCEEDED(hr))
            {
                hr = IWICMetadataReader_QueryInterface(reader, &IID_IWICPersistStream, (void**)&persist);

                if (SUCCEEDED(hr))
                {
                    hr = IWICPersistStream_LoadEx(persist, (IStream*)stream, NULL,
                        This->metadata_blocks[nIndex].options & DECODER_BLOCK_OPTION_MASK);

                    IWICPersistStream_Release(persist);
                }

                if (SUCCEEDED(hr))
                    *ppIMetadataReader = reader;
                else
                    IWICMetadataReader_Release(reader);
            }
        }
        else
        {
            hr = IWICComponentFactory_CreateMetadataReaderFromContainer(factory,
                &This->parent->decoder_info.block_format, NULL,
                This->metadata_blocks[nIndex].options & DECODER_BLOCK_OPTION_MASK,
                (IStream*)stream, ppIMetadataReader);
        }

        IWICStream_Release(stream);
    }

    if (factory) IWICComponentFactory_Release(factory);

    if (FAILED(hr))
        *ppIMetadataReader = NULL;

    return S_OK;
}

static HRESULT WINAPI CommonDecoderFrame_Block_GetEnumerator(IWICMetadataBlockReader *iface,
    IEnumUnknown **ppIEnumMetadata)
{
    FIXME("%p,%p\n", iface, ppIEnumMetadata);
    return E_NOTIMPL;
}

static const IWICMetadataBlockReaderVtbl CommonDecoderFrame_BlockVtbl = {
    CommonDecoderFrame_Block_QueryInterface,
    CommonDecoderFrame_Block_AddRef,
    CommonDecoderFrame_Block_Release,
    CommonDecoderFrame_Block_GetContainerFormat,
    CommonDecoderFrame_Block_GetCount,
    CommonDecoderFrame_Block_GetReaderByIndex,
    CommonDecoderFrame_Block_GetEnumerator,
};

static HRESULT WINAPI CommonDecoder_GetFrame(IWICBitmapDecoder *iface,
    UINT index, IWICBitmapFrameDecode **ppIBitmapFrame)
{
    CommonDecoder *This = impl_from_IWICBitmapDecoder(iface);
    HRESULT hr=S_OK;
    CommonDecoderFrame *result;

    TRACE("(%p,%u,%p)\n", iface, index, ppIBitmapFrame);

    if (!ppIBitmapFrame)
        return E_INVALIDARG;

    EnterCriticalSection(&This->lock);

    if (!This->stream || index >= This->file_info.frame_count)
        hr = WINCODEC_ERR_FRAMEMISSING;

    if (SUCCEEDED(hr))
    {
        result = malloc(sizeof(*result));
        if (!result)
            hr = E_OUTOFMEMORY;
    }

    if (SUCCEEDED(hr))
    {
        result->IWICBitmapFrameDecode_iface.lpVtbl = &CommonDecoderFrameVtbl;
        result->IWICMetadataBlockReader_iface.lpVtbl = &CommonDecoderFrame_BlockVtbl;
        result->ref = 1;
        result->parent = This;
        result->frame = index;
        result->metadata_initialized = FALSE;
        result->metadata_count = 0;
        result->metadata_blocks = NULL;

        hr = decoder_get_frame_info(This->decoder, index, &result->decoder_frame);

        if (SUCCEEDED(hr) && This->cache_options == WICDecodeMetadataCacheOnLoad)
            hr = CommonDecoderFrame_InitializeMetadata(result);

        if (FAILED(hr))
            free(result);
    }

    LeaveCriticalSection(&This->lock);

    if (SUCCEEDED(hr))
    {
        TRACE("-> %ux%u, %u-bit pixelformat=%s res=%f,%f colors=%lu contexts=%lu\n",
            result->decoder_frame.width, result->decoder_frame.height,
            result->decoder_frame.bpp, wine_dbgstr_guid(&result->decoder_frame.pixel_format),
            result->decoder_frame.dpix, result->decoder_frame.dpiy,
            result->decoder_frame.num_colors, result->decoder_frame.num_color_contexts);
        IWICBitmapDecoder_AddRef(&This->IWICBitmapDecoder_iface);
        *ppIBitmapFrame = &result->IWICBitmapFrameDecode_iface;
    }
    else
    {
        *ppIBitmapFrame = NULL;
    }

    return hr;
}

HRESULT CommonDecoder_CreateInstance(struct decoder *decoder,
    const struct decoder_info *decoder_info, REFIID iid, void** ppv)
{
    CommonDecoder *This;
    HRESULT hr;

    TRACE("(%s,%s,%p)\n", debugstr_guid(&decoder_info->clsid), debugstr_guid(iid), ppv);

    This = malloc(sizeof(*This));
    if (!This)
    {
        decoder_destroy(decoder);
        return E_OUTOFMEMORY;
    }

    This->IWICBitmapDecoder_iface.lpVtbl = &CommonDecoder_Vtbl;
    This->ref = 1;
    This->stream = NULL;
    This->decoder = decoder;
    This->decoder_info = *decoder_info;
#ifdef __REACTOS__
    InitializeCriticalSection(&This->lock);
#else
    InitializeCriticalSectionEx(&This->lock, 0, RTL_CRITICAL_SECTION_FLAG_FORCE_DEBUG_INFO);
#endif
    This->lock.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": CommonDecoder.lock");

    hr = IWICBitmapDecoder_QueryInterface(&This->IWICBitmapDecoder_iface, iid, ppv);
    IWICBitmapDecoder_Release(&This->IWICBitmapDecoder_iface);

    return hr;
}
