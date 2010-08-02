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

#include <stdarg.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "objbase.h"
#include "wincodec.h"

#include "ungif.h"

#include "wincodecs_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(wincodecs);

typedef struct {
    const IWICBitmapDecoderVtbl *lpVtbl;
    LONG ref;
    BOOL initialized;
    GifFileType *gif;
    CRITICAL_SECTION lock;
} GifDecoder;

typedef struct {
    const IWICBitmapFrameDecodeVtbl *lpVtbl;
    LONG ref;
    SavedImage *frame;
    GifDecoder *parent;
} GifFrameDecode;

static HRESULT WINAPI GifFrameDecode_QueryInterface(IWICBitmapFrameDecode *iface, REFIID iid,
    void **ppv)
{
    GifFrameDecode *This = (GifFrameDecode*)iface;
    TRACE("(%p,%s,%p)\n", iface, debugstr_guid(iid), ppv);

    if (!ppv) return E_INVALIDARG;

    if (IsEqualIID(&IID_IUnknown, iid) ||
        IsEqualIID(&IID_IWICBitmapSource, iid) ||
        IsEqualIID(&IID_IWICBitmapFrameDecode, iid))
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

static ULONG WINAPI GifFrameDecode_AddRef(IWICBitmapFrameDecode *iface)
{
    GifFrameDecode *This = (GifFrameDecode*)iface;
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) refcount=%u\n", iface, ref);

    return ref;
}

static ULONG WINAPI GifFrameDecode_Release(IWICBitmapFrameDecode *iface)
{
    GifFrameDecode *This = (GifFrameDecode*)iface;
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) refcount=%u\n", iface, ref);

    if (ref == 0)
    {
        IUnknown_Release((IUnknown*)This->parent);
        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}

static HRESULT WINAPI GifFrameDecode_GetSize(IWICBitmapFrameDecode *iface,
    UINT *puiWidth, UINT *puiHeight)
{
    GifFrameDecode *This = (GifFrameDecode*)iface;
    TRACE("(%p,%p,%p)\n", iface, puiWidth, puiHeight);

    *puiWidth = This->frame->ImageDesc.Width;
    *puiHeight = This->frame->ImageDesc.Height;

    return S_OK;
}

static HRESULT WINAPI GifFrameDecode_GetPixelFormat(IWICBitmapFrameDecode *iface,
    WICPixelFormatGUID *pPixelFormat)
{
    memcpy(pPixelFormat, &GUID_WICPixelFormat8bppIndexed, sizeof(GUID));

    return S_OK;
}

static HRESULT WINAPI GifFrameDecode_GetResolution(IWICBitmapFrameDecode *iface,
    double *pDpiX, double *pDpiY)
{
    FIXME("(%p,%p,%p): stub\n", iface, pDpiX, pDpiY);
    return E_NOTIMPL;
}

static HRESULT WINAPI GifFrameDecode_CopyPalette(IWICBitmapFrameDecode *iface,
    IWICPalette *pIPalette)
{
    GifFrameDecode *This = (GifFrameDecode*)iface;
    WICColor colors[256];
    ColorMapObject *cm = This->frame->ImageDesc.ColorMap;
    int i, trans;
    ExtensionBlock *eb;
    TRACE("(%p,%p)\n", iface, pIPalette);

    if (!cm) cm = This->parent->gif->SColorMap;

    if (cm->ColorCount > 256)
    {
        ERR("GIF contains %i colors???\n", cm->ColorCount);
        return E_FAIL;
    }

    for (i = 0; i < cm->ColorCount; i++) {
        colors[i] = 0xff000000| /* alpha */
                    cm->Colors[i].Red << 16|
                    cm->Colors[i].Green << 8|
                    cm->Colors[i].Blue;
    }

    /* look for the transparent color extension */
    for (i = 0; i < This->frame->ExtensionBlockCount; ++i) {
	eb = This->frame->ExtensionBlocks + i;
	if (eb->Function == 0xF9 && eb->ByteCount == 4) {
	    if ((eb->Bytes[0] & 1) == 1) {
	        trans = (unsigned char)eb->Bytes[3];
	        colors[trans] &= 0xffffff; /* set alpha to 0 */
	        break;
	    }
	}
    }

    IWICPalette_InitializeCustom(pIPalette, colors, cm->ColorCount);

    return S_OK;
}

static HRESULT copy_interlaced_pixels(const BYTE *srcbuffer,
    UINT srcwidth, UINT srcheight, INT srcstride, const WICRect *rc,
    UINT dststride, UINT dstbuffersize, BYTE *dstbuffer)
{
    UINT row_offset; /* number of bytes into the source rows where the data starts */
    const BYTE *src;
    BYTE *dst;
    UINT y;

    if (rc->X < 0 || rc->Y < 0 || rc->X+rc->Width > srcwidth || rc->Y+rc->Height > srcheight)
        return E_INVALIDARG;

    if (dststride < rc->Width)
        return E_INVALIDARG;

    if ((dststride * rc->Height) > dstbuffersize)
        return E_INVALIDARG;

    row_offset = rc->X;

    dst = dstbuffer;
    for (y=rc->Y; y-rc->Y < rc->Height; y++)
    {
        if (y%8 == 0)
            src = srcbuffer + srcstride * (y/8);
        else if (y%4 == 0)
            src = srcbuffer + srcstride * ((srcheight+7)/8 + y/8);
        else if (y%2 == 0)
            src = srcbuffer + srcstride * ((srcheight+3)/4 + y/4);
        else /* y%2 == 1 */
            src = srcbuffer + srcstride * ((srcheight+1)/2 + y/2);
        src += row_offset;
        memcpy(dst, src, rc->Width);
        dst += dststride;
    }
    return S_OK;
}

static HRESULT WINAPI GifFrameDecode_CopyPixels(IWICBitmapFrameDecode *iface,
    const WICRect *prc, UINT cbStride, UINT cbBufferSize, BYTE *pbBuffer)
{
    GifFrameDecode *This = (GifFrameDecode*)iface;
    TRACE("(%p,%p,%u,%u,%p)\n", iface, prc, cbStride, cbBufferSize, pbBuffer);

    if (This->frame->ImageDesc.Interlace)
    {
        return copy_interlaced_pixels(This->frame->RasterBits, This->frame->ImageDesc.Width,
            This->frame->ImageDesc.Height, This->frame->ImageDesc.Width,
            prc, cbStride, cbBufferSize, pbBuffer);
    }
    else
    {
        return copy_pixels(8, This->frame->RasterBits, This->frame->ImageDesc.Width,
            This->frame->ImageDesc.Height, This->frame->ImageDesc.Width,
            prc, cbStride, cbBufferSize, pbBuffer);
    }
}

static HRESULT WINAPI GifFrameDecode_GetMetadataQueryReader(IWICBitmapFrameDecode *iface,
    IWICMetadataQueryReader **ppIMetadataQueryReader)
{
    TRACE("(%p,%p)\n", iface, ppIMetadataQueryReader);
    return WINCODEC_ERR_UNSUPPORTEDOPERATION;
}

static HRESULT WINAPI GifFrameDecode_GetColorContexts(IWICBitmapFrameDecode *iface,
    UINT cCount, IWICColorContext **ppIColorContexts, UINT *pcActualCount)
{
    TRACE("(%p,%u,%p,%p)\n", iface, cCount, ppIColorContexts, pcActualCount);
    return WINCODEC_ERR_UNSUPPORTEDOPERATION;
}

static HRESULT WINAPI GifFrameDecode_GetThumbnail(IWICBitmapFrameDecode *iface,
    IWICBitmapSource **ppIThumbnail)
{
    TRACE("(%p,%p)\n", iface, ppIThumbnail);
    return WINCODEC_ERR_CODECNOTHUMBNAIL;
}

static const IWICBitmapFrameDecodeVtbl GifFrameDecode_Vtbl = {
    GifFrameDecode_QueryInterface,
    GifFrameDecode_AddRef,
    GifFrameDecode_Release,
    GifFrameDecode_GetSize,
    GifFrameDecode_GetPixelFormat,
    GifFrameDecode_GetResolution,
    GifFrameDecode_CopyPalette,
    GifFrameDecode_CopyPixels,
    GifFrameDecode_GetMetadataQueryReader,
    GifFrameDecode_GetColorContexts,
    GifFrameDecode_GetThumbnail
};

static HRESULT WINAPI GifDecoder_QueryInterface(IWICBitmapDecoder *iface, REFIID iid,
    void **ppv)
{
    GifDecoder *This = (GifDecoder*)iface;
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

static ULONG WINAPI GifDecoder_AddRef(IWICBitmapDecoder *iface)
{
    GifDecoder *This = (GifDecoder*)iface;
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) refcount=%u\n", iface, ref);

    return ref;
}

static ULONG WINAPI GifDecoder_Release(IWICBitmapDecoder *iface)
{
    GifDecoder *This = (GifDecoder*)iface;
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) refcount=%u\n", iface, ref);

    if (ref == 0)
    {
        This->lock.DebugInfo->Spare[0] = 0;
        DeleteCriticalSection(&This->lock);
        DGifCloseFile(This->gif);
        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}

static HRESULT WINAPI GifDecoder_QueryCapability(IWICBitmapDecoder *iface, IStream *pIStream,
    DWORD *pdwCapability)
{
    FIXME("(%p,%p,%p): stub\n", iface, pIStream, pdwCapability);
    return E_NOTIMPL;
}

static int _gif_inputfunc(GifFileType *gif, GifByteType *data, int len) {
    IStream *stream = gif->UserData;
    ULONG bytesread;
    HRESULT hr;

    if (!stream)
    {
        ERR("attempting to read file after initialization\n");
        return 0;
    }

    hr = IStream_Read(stream, data, len, &bytesread);
    if (hr != S_OK) bytesread = 0;
    return bytesread;
}

static HRESULT WINAPI GifDecoder_Initialize(IWICBitmapDecoder *iface, IStream *pIStream,
    WICDecodeOptions cacheOptions)
{
    GifDecoder *This = (GifDecoder*)iface;
    LARGE_INTEGER seek;
    int ret;

    TRACE("(%p,%p,%x)\n", iface, pIStream, cacheOptions);

    EnterCriticalSection(&This->lock);

    if (This->initialized || This->gif)
    {
        WARN("already initialized\n");
        LeaveCriticalSection(&This->lock);
        return WINCODEC_ERR_WRONGSTATE;
    }

    /* seek to start of stream */
    seek.QuadPart = 0;
    IStream_Seek(pIStream, seek, STREAM_SEEK_SET, NULL);

    /* read all data from the stream */
    This->gif = DGifOpen((void*)pIStream, _gif_inputfunc);
    if (!This->gif)
    {
        LeaveCriticalSection(&This->lock);
        return E_FAIL;
    }

    ret = DGifSlurp(This->gif);
    if (ret == GIF_ERROR)
    {
        LeaveCriticalSection(&This->lock);
        return E_FAIL;
    }

    /* make sure we don't use the stream after this method returns */
    This->gif->UserData = NULL;

    This->initialized = TRUE;

    LeaveCriticalSection(&This->lock);

    return S_OK;
}

static HRESULT WINAPI GifDecoder_GetContainerFormat(IWICBitmapDecoder *iface,
    GUID *pguidContainerFormat)
{
    memcpy(pguidContainerFormat, &GUID_ContainerFormatGif, sizeof(GUID));
    return S_OK;
}

static HRESULT WINAPI GifDecoder_GetDecoderInfo(IWICBitmapDecoder *iface,
    IWICBitmapDecoderInfo **ppIDecoderInfo)
{
    HRESULT hr;
    IWICComponentInfo *compinfo;

    TRACE("(%p,%p)\n", iface, ppIDecoderInfo);

    hr = CreateComponentInfo(&CLSID_WICGifDecoder, &compinfo);
    if (FAILED(hr)) return hr;

    hr = IWICComponentInfo_QueryInterface(compinfo, &IID_IWICBitmapDecoderInfo,
        (void**)ppIDecoderInfo);

    IWICComponentInfo_Release(compinfo);

    return hr;
}

static HRESULT WINAPI GifDecoder_CopyPalette(IWICBitmapDecoder *iface,
    IWICPalette *pIPalette)
{
    TRACE("(%p,%p)\n", iface, pIPalette);
    return WINCODEC_ERR_PALETTEUNAVAILABLE;
}

static HRESULT WINAPI GifDecoder_GetMetadataQueryReader(IWICBitmapDecoder *iface,
    IWICMetadataQueryReader **ppIMetadataQueryReader)
{
    TRACE("(%p,%p)\n", iface, ppIMetadataQueryReader);
    return WINCODEC_ERR_UNSUPPORTEDOPERATION;
}

static HRESULT WINAPI GifDecoder_GetPreview(IWICBitmapDecoder *iface,
    IWICBitmapSource **ppIBitmapSource)
{
    TRACE("(%p,%p)\n", iface, ppIBitmapSource);
    return WINCODEC_ERR_UNSUPPORTEDOPERATION;
}

static HRESULT WINAPI GifDecoder_GetColorContexts(IWICBitmapDecoder *iface,
    UINT cCount, IWICColorContext **ppIColorContexts, UINT *pcActualCount)
{
    TRACE("(%p,%u,%p,%p)\n", iface, cCount, ppIColorContexts, pcActualCount);
    return WINCODEC_ERR_UNSUPPORTEDOPERATION;
}

static HRESULT WINAPI GifDecoder_GetThumbnail(IWICBitmapDecoder *iface,
    IWICBitmapSource **ppIThumbnail)
{
    TRACE("(%p,%p)\n", iface, ppIThumbnail);
    return WINCODEC_ERR_CODECNOTHUMBNAIL;
}

static HRESULT WINAPI GifDecoder_GetFrameCount(IWICBitmapDecoder *iface,
    UINT *pCount)
{
    GifDecoder *This = (GifDecoder*)iface;
    TRACE("(%p,%p)\n", iface, pCount);

    if (!This->initialized) return WINCODEC_ERR_NOTINITIALIZED;

    *pCount = This->gif->ImageCount;

    TRACE("<- %u\n", *pCount);

    return S_OK;
}

static HRESULT WINAPI GifDecoder_GetFrame(IWICBitmapDecoder *iface,
    UINT index, IWICBitmapFrameDecode **ppIBitmapFrame)
{
    GifDecoder *This = (GifDecoder*)iface;
    GifFrameDecode *result;
    TRACE("(%p,%u,%p)\n", iface, index, ppIBitmapFrame);

    if (!This->initialized) return WINCODEC_ERR_NOTINITIALIZED;

    if (index >= This->gif->ImageCount) return E_INVALIDARG;

    result = HeapAlloc(GetProcessHeap(), 0, sizeof(GifFrameDecode));
    if (!result) return E_OUTOFMEMORY;

    result->lpVtbl = &GifFrameDecode_Vtbl;
    result->ref = 1;
    result->frame = &This->gif->SavedImages[index];
    IWICBitmapDecoder_AddRef(iface);
    result->parent = This;

    *ppIBitmapFrame = (IWICBitmapFrameDecode*)result;

    return S_OK;
}

static const IWICBitmapDecoderVtbl GifDecoder_Vtbl = {
    GifDecoder_QueryInterface,
    GifDecoder_AddRef,
    GifDecoder_Release,
    GifDecoder_QueryCapability,
    GifDecoder_Initialize,
    GifDecoder_GetContainerFormat,
    GifDecoder_GetDecoderInfo,
    GifDecoder_CopyPalette,
    GifDecoder_GetMetadataQueryReader,
    GifDecoder_GetPreview,
    GifDecoder_GetColorContexts,
    GifDecoder_GetThumbnail,
    GifDecoder_GetFrameCount,
    GifDecoder_GetFrame
};

HRESULT GifDecoder_CreateInstance(IUnknown *pUnkOuter, REFIID iid, void** ppv)
{
    GifDecoder *This;
    HRESULT ret;

    TRACE("(%p,%s,%p)\n", pUnkOuter, debugstr_guid(iid), ppv);

    *ppv = NULL;

    if (pUnkOuter) return CLASS_E_NOAGGREGATION;

    This = HeapAlloc(GetProcessHeap(), 0, sizeof(GifDecoder));
    if (!This) return E_OUTOFMEMORY;

    This->lpVtbl = &GifDecoder_Vtbl;
    This->ref = 1;
    This->initialized = FALSE;
    This->gif = NULL;
    InitializeCriticalSection(&This->lock);
    This->lock.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": GifDecoder.lock");

    ret = IUnknown_QueryInterface((IUnknown*)This, iid, ppv);
    IUnknown_Release((IUnknown*)This);

    return ret;
}
