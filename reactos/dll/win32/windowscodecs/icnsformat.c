/*
 * Copyright 2010 Damjan Jovanovic
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

#ifdef HAVE_APPLICATIONSERVICES_APPLICATIONSERVICES_H
#define GetCurrentProcess GetCurrentProcess_Mac
#define GetCurrentThread GetCurrentThread_Mac
#define LoadResource LoadResource_Mac
#define AnimatePalette AnimatePalette_Mac
#define EqualRgn EqualRgn_Mac
#define FillRgn FillRgn_Mac
#define FrameRgn FrameRgn_Mac
#define GetPixel GetPixel_Mac
#define InvertRgn InvertRgn_Mac
#define LineTo LineTo_Mac
#define OffsetRgn OffsetRgn_Mac
#define PaintRgn PaintRgn_Mac
#define Polygon Polygon_Mac
#define ResizePalette ResizePalette_Mac
#define SetRectRgn SetRectRgn_Mac
#define EqualRect EqualRect_Mac
#define FillRect FillRect_Mac
#define FrameRect FrameRect_Mac
#define GetCursor GetCursor_Mac
#define InvertRect InvertRect_Mac
#define OffsetRect OffsetRect_Mac
#define PtInRect PtInRect_Mac
#define SetCursor SetCursor_Mac
#define SetRect SetRect_Mac
#define ShowCursor ShowCursor_Mac
#define UnionRect UnionRect_Mac
#include <ApplicationServices/ApplicationServices.h>
#undef GetCurrentProcess
#undef GetCurrentThread
#undef LoadResource
#undef AnimatePalette
#undef EqualRgn
#undef FillRgn
#undef FrameRgn
#undef GetPixel
#undef InvertRgn
#undef LineTo
#undef OffsetRgn
#undef PaintRgn
#undef Polygon
#undef ResizePalette
#undef SetRectRgn
#undef EqualRect
#undef FillRect
#undef FrameRect
#undef GetCursor
#undef InvertRect
#undef OffsetRect
#undef PtInRect
#undef SetCursor
#undef SetRect
#undef ShowCursor
#undef UnionRect
#undef DPRINTF
#endif

#include "wincodecs_private.h"

#if defined(HAVE_APPLICATIONSERVICES_APPLICATIONSERVICES_H) && \
    MAC_OS_X_VERSION_MAX_ALLOWED > MAC_OS_X_VERSION_10_4

typedef struct IcnsEncoder {
    IWICBitmapEncoder IWICBitmapEncoder_iface;
    LONG ref;
    IStream *stream;
    IconFamilyHandle icns_family;
    BOOL any_frame_committed;
    int outstanding_commits;
    BOOL committed;
    CRITICAL_SECTION lock;
} IcnsEncoder;

static inline IcnsEncoder *impl_from_IWICBitmapEncoder(IWICBitmapEncoder *iface)
{
    return CONTAINING_RECORD(iface, IcnsEncoder, IWICBitmapEncoder_iface);
}

typedef struct IcnsFrameEncode {
    IWICBitmapFrameEncode IWICBitmapFrameEncode_iface;
    IcnsEncoder *encoder;
    LONG ref;
    BOOL initialized;
    UINT size;
    OSType icns_type;
    BYTE* icns_image;
    int lines_written;
    BOOL committed;
} IcnsFrameEncode;

static inline IcnsFrameEncode *impl_from_IWICBitmapFrameEncode(IWICBitmapFrameEncode *iface)
{
    return CONTAINING_RECORD(iface, IcnsFrameEncode, IWICBitmapFrameEncode_iface);
}

static HRESULT WINAPI IcnsFrameEncode_QueryInterface(IWICBitmapFrameEncode *iface, REFIID iid,
    void **ppv)
{
    IcnsFrameEncode *This = impl_from_IWICBitmapFrameEncode(iface);
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

static ULONG WINAPI IcnsFrameEncode_AddRef(IWICBitmapFrameEncode *iface)
{
    IcnsFrameEncode *This = impl_from_IWICBitmapFrameEncode(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) refcount=%u\n", iface, ref);

    return ref;
}

static ULONG WINAPI IcnsFrameEncode_Release(IWICBitmapFrameEncode *iface)
{
    IcnsFrameEncode *This = impl_from_IWICBitmapFrameEncode(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) refcount=%u\n", iface, ref);

    if (ref == 0)
    {
        if (!This->committed)
        {
            EnterCriticalSection(&This->encoder->lock);
            This->encoder->outstanding_commits--;
            LeaveCriticalSection(&This->encoder->lock);
        }
        if (This->icns_image != NULL)
            HeapFree(GetProcessHeap(), 0, This->icns_image);

        IWICBitmapEncoder_Release(&This->encoder->IWICBitmapEncoder_iface);
        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}

static HRESULT WINAPI IcnsFrameEncode_Initialize(IWICBitmapFrameEncode *iface,
    IPropertyBag2 *pIEncoderOptions)
{
    IcnsFrameEncode *This = impl_from_IWICBitmapFrameEncode(iface);
    HRESULT hr = S_OK;

    TRACE("(%p,%p)\n", iface, pIEncoderOptions);

    EnterCriticalSection(&This->encoder->lock);

    if (This->initialized)
    {
        hr = WINCODEC_ERR_WRONGSTATE;
        goto end;
    }
    This->initialized = TRUE;

end:
    LeaveCriticalSection(&This->encoder->lock);
    return hr;
}

static HRESULT WINAPI IcnsFrameEncode_SetSize(IWICBitmapFrameEncode *iface,
    UINT uiWidth, UINT uiHeight)
{
    IcnsFrameEncode *This = impl_from_IWICBitmapFrameEncode(iface);
    HRESULT hr = S_OK;

    TRACE("(%p,%u,%u)\n", iface, uiWidth, uiHeight);

    EnterCriticalSection(&This->encoder->lock);

    if (!This->initialized || This->icns_image)
    {
        hr = WINCODEC_ERR_WRONGSTATE;
        goto end;
    }

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
            WARN("cannot generate ICNS icon from %dx%d image\n", This->size, This->size);
            hr = E_INVALIDARG;
            goto end;
    }

    This->size = uiWidth;

end:
    LeaveCriticalSection(&This->encoder->lock);
    return hr;
}

static HRESULT WINAPI IcnsFrameEncode_SetResolution(IWICBitmapFrameEncode *iface,
    double dpiX, double dpiY)
{
    IcnsFrameEncode *This = impl_from_IWICBitmapFrameEncode(iface);
    HRESULT hr = S_OK;

    TRACE("(%p,%0.2f,%0.2f)\n", iface, dpiX, dpiY);

    EnterCriticalSection(&This->encoder->lock);

    if (!This->initialized || This->icns_image)
    {
        hr = WINCODEC_ERR_WRONGSTATE;
        goto end;
    }

end:
    LeaveCriticalSection(&This->encoder->lock);
    return S_OK;
}

static HRESULT WINAPI IcnsFrameEncode_SetPixelFormat(IWICBitmapFrameEncode *iface,
    WICPixelFormatGUID *pPixelFormat)
{
    IcnsFrameEncode *This = impl_from_IWICBitmapFrameEncode(iface);
    HRESULT hr = S_OK;

    TRACE("(%p,%s)\n", iface, debugstr_guid(pPixelFormat));

    EnterCriticalSection(&This->encoder->lock);

    if (!This->initialized || This->icns_image)
    {
        hr = WINCODEC_ERR_WRONGSTATE;
        goto end;
    }

    memcpy(pPixelFormat, &GUID_WICPixelFormat32bppBGRA, sizeof(GUID));

end:
    LeaveCriticalSection(&This->encoder->lock);
    return S_OK;
}

static HRESULT WINAPI IcnsFrameEncode_SetColorContexts(IWICBitmapFrameEncode *iface,
    UINT cCount, IWICColorContext **ppIColorContext)
{
    FIXME("(%p,%u,%p): stub\n", iface, cCount, ppIColorContext);
    return E_NOTIMPL;
}

static HRESULT WINAPI IcnsFrameEncode_SetPalette(IWICBitmapFrameEncode *iface,
    IWICPalette *pIPalette)
{
    FIXME("(%p,%p): stub\n", iface, pIPalette);
    return WINCODEC_ERR_UNSUPPORTEDOPERATION;
}

static HRESULT WINAPI IcnsFrameEncode_SetThumbnail(IWICBitmapFrameEncode *iface,
    IWICBitmapSource *pIThumbnail)
{
    FIXME("(%p,%p): stub\n", iface, pIThumbnail);
    return WINCODEC_ERR_UNSUPPORTEDOPERATION;
}

static HRESULT WINAPI IcnsFrameEncode_WritePixels(IWICBitmapFrameEncode *iface,
    UINT lineCount, UINT cbStride, UINT cbBufferSize, BYTE *pbPixels)
{
    IcnsFrameEncode *This = impl_from_IWICBitmapFrameEncode(iface);
    HRESULT hr = S_OK;
    UINT i;

    TRACE("(%p,%u,%u,%u,%p)\n", iface, lineCount, cbStride, cbBufferSize, pbPixels);

    EnterCriticalSection(&This->encoder->lock);

    if (!This->initialized || !This->size)
    {
        hr = WINCODEC_ERR_WRONGSTATE;
        goto end;
    }
    if (lineCount == 0 || lineCount + This->lines_written > This->size)
    {
        hr = E_INVALIDARG;
        goto end;
    }

    if (!This->icns_image)
    {
        switch (This->size)
        {
            case 16:  This->icns_type = kIconServices16PixelDataARGB;  break;
            case 32:  This->icns_type = kIconServices32PixelDataARGB;  break;
            case 48:  This->icns_type = kIconServices48PixelDataARGB;  break;
            case 128: This->icns_type = kIconServices128PixelDataARGB; break;
            case 256: This->icns_type = kIconServices256PixelDataARGB; break;
            case 512: This->icns_type = kIconServices512PixelDataARGB; break;
            default:
                WARN("cannot generate ICNS icon from %dx%d image\n", This->size, This->size);
                hr = E_INVALIDARG;
                goto end;
        }
        This->icns_image = HeapAlloc(GetProcessHeap(), 0, This->size * This->size * 4);
        if (!This->icns_image)
        {
            WARN("failed to allocate image buffer\n");
            hr = E_FAIL;
            goto end;
        }
    }

    for (i = 0; i < lineCount; i++)
    {
        BYTE *src_row, *dst_row;
        UINT j;
        src_row = pbPixels + cbStride * i;
        dst_row = This->icns_image + (This->lines_written + i)*(This->size*4);
        /* swap bgr -> rgb */
        for (j = 0; j < This->size*4; j += 4)
        {
            dst_row[j] = src_row[j+3];
            dst_row[j+1] = src_row[j+2];
            dst_row[j+2] = src_row[j+1];
            dst_row[j+3] = src_row[j];
        }
    }
    This->lines_written += lineCount;

end:
    LeaveCriticalSection(&This->encoder->lock);
    return hr;
}

static HRESULT WINAPI IcnsFrameEncode_WriteSource(IWICBitmapFrameEncode *iface,
    IWICBitmapSource *pIBitmapSource, WICRect *prc)
{
    IcnsFrameEncode *This = impl_from_IWICBitmapFrameEncode(iface);
    HRESULT hr;

    TRACE("(%p,%p,%p)\n", iface, pIBitmapSource, prc);

    if (!This->initialized)
        return WINCODEC_ERR_WRONGSTATE;

    hr = configure_write_source(iface, pIBitmapSource, prc,
        &GUID_WICPixelFormat32bppBGRA, This->size, This->size,
        1.0, 1.0);

    if (SUCCEEDED(hr))
    {
        hr = write_source(iface, pIBitmapSource, prc,
            &GUID_WICPixelFormat32bppBGRA, 32, This->size, This->size);
    }

    return hr;
}

static HRESULT WINAPI IcnsFrameEncode_Commit(IWICBitmapFrameEncode *iface)
{
    IcnsFrameEncode *This = impl_from_IWICBitmapFrameEncode(iface);
    Handle handle;
    OSErr ret;
    HRESULT hr = S_OK;

    TRACE("(%p): stub\n", iface);

    EnterCriticalSection(&This->encoder->lock);

    if (!This->icns_image || This->lines_written != This->size || This->committed)
    {
        hr = WINCODEC_ERR_WRONGSTATE;
        goto end;
    }

    ret = PtrToHand(This->icns_image, &handle, This->size * This->size * 4);
    if (ret != noErr || !handle)
    {
        WARN("PtrToHand failed with error %d\n", ret);
        hr = E_FAIL;
        goto end;
    }

    ret = SetIconFamilyData(This->encoder->icns_family, This->icns_type, handle);
    DisposeHandle(handle);

    if (ret != noErr)
	{
        WARN("SetIconFamilyData failed for image with error %d\n", ret);
        hr = E_FAIL;
        goto end;
	}

    This->committed = TRUE;
    This->encoder->any_frame_committed = TRUE;
    This->encoder->outstanding_commits--;

end:
    LeaveCriticalSection(&This->encoder->lock);
    return hr;
}

static HRESULT WINAPI IcnsFrameEncode_GetMetadataQueryWriter(IWICBitmapFrameEncode *iface,
    IWICMetadataQueryWriter **ppIMetadataQueryWriter)
{
    FIXME("(%p, %p): stub\n", iface, ppIMetadataQueryWriter);
    return E_NOTIMPL;
}

static const IWICBitmapFrameEncodeVtbl IcnsEncoder_FrameVtbl = {
    IcnsFrameEncode_QueryInterface,
    IcnsFrameEncode_AddRef,
    IcnsFrameEncode_Release,
    IcnsFrameEncode_Initialize,
    IcnsFrameEncode_SetSize,
    IcnsFrameEncode_SetResolution,
    IcnsFrameEncode_SetPixelFormat,
    IcnsFrameEncode_SetColorContexts,
    IcnsFrameEncode_SetPalette,
    IcnsFrameEncode_SetThumbnail,
    IcnsFrameEncode_WritePixels,
    IcnsFrameEncode_WriteSource,
    IcnsFrameEncode_Commit,
    IcnsFrameEncode_GetMetadataQueryWriter
};

static HRESULT WINAPI IcnsEncoder_QueryInterface(IWICBitmapEncoder *iface, REFIID iid,
    void **ppv)
{
    IcnsEncoder *This = impl_from_IWICBitmapEncoder(iface);
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

static ULONG WINAPI IcnsEncoder_AddRef(IWICBitmapEncoder *iface)
{
    IcnsEncoder *This = impl_from_IWICBitmapEncoder(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) refcount=%u\n", iface, ref);

    return ref;
}

static ULONG WINAPI IcnsEncoder_Release(IWICBitmapEncoder *iface)
{
    IcnsEncoder *This = impl_from_IWICBitmapEncoder(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) refcount=%u\n", iface, ref);

    if (ref == 0)
    {
        This->lock.DebugInfo->Spare[0] = 0;
        DeleteCriticalSection(&This->lock);
        if (This->icns_family)
            DisposeHandle((Handle)This->icns_family);
        if (This->stream)
            IStream_Release(This->stream);
        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}

static HRESULT WINAPI IcnsEncoder_Initialize(IWICBitmapEncoder *iface,
    IStream *pIStream, WICBitmapEncoderCacheOption cacheOption)
{
    IcnsEncoder *This = impl_from_IWICBitmapEncoder(iface);
    HRESULT hr = S_OK;

    TRACE("(%p,%p,%u)\n", iface, pIStream, cacheOption);

    EnterCriticalSection(&This->lock);

    if (This->icns_family)
    {
        hr = WINCODEC_ERR_WRONGSTATE;
        goto end;
    }
    This->icns_family = (IconFamilyHandle)NewHandle(0);
    if (!This->icns_family)
    {
        WARN("error creating icns family\n");
        hr = E_FAIL;
        goto end;
    }
    IStream_AddRef(pIStream);
    This->stream = pIStream;

end:
    LeaveCriticalSection(&This->lock);

    return hr;
}

static HRESULT WINAPI IcnsEncoder_GetContainerFormat(IWICBitmapEncoder *iface,
    GUID *pguidContainerFormat)
{
    FIXME("(%p,%s): stub\n", iface, debugstr_guid(pguidContainerFormat));
    return E_NOTIMPL;
}

static HRESULT WINAPI IcnsEncoder_GetEncoderInfo(IWICBitmapEncoder *iface,
    IWICBitmapEncoderInfo **ppIEncoderInfo)
{
    FIXME("(%p,%p): stub\n", iface, ppIEncoderInfo);
    return E_NOTIMPL;
}

static HRESULT WINAPI IcnsEncoder_SetColorContexts(IWICBitmapEncoder *iface,
    UINT cCount, IWICColorContext **ppIColorContext)
{
    FIXME("(%p,%u,%p): stub\n", iface, cCount, ppIColorContext);
    return E_NOTIMPL;
}

static HRESULT WINAPI IcnsEncoder_SetPalette(IWICBitmapEncoder *iface, IWICPalette *pIPalette)
{
    TRACE("(%p,%p)\n", iface, pIPalette);
    return WINCODEC_ERR_UNSUPPORTEDOPERATION;
}

static HRESULT WINAPI IcnsEncoder_SetThumbnail(IWICBitmapEncoder *iface, IWICBitmapSource *pIThumbnail)
{
    TRACE("(%p,%p)\n", iface, pIThumbnail);
    return WINCODEC_ERR_UNSUPPORTEDOPERATION;
}

static HRESULT WINAPI IcnsEncoder_SetPreview(IWICBitmapEncoder *iface, IWICBitmapSource *pIPreview)
{
    TRACE("(%p,%p)\n", iface, pIPreview);
    return WINCODEC_ERR_UNSUPPORTEDOPERATION;
}

static HRESULT WINAPI IcnsEncoder_CreateNewFrame(IWICBitmapEncoder *iface,
    IWICBitmapFrameEncode **ppIFrameEncode, IPropertyBag2 **ppIEncoderOptions)
{
    IcnsEncoder *This = impl_from_IWICBitmapEncoder(iface);
    HRESULT hr = S_OK;
    IcnsFrameEncode *frameEncode = NULL;

    TRACE("(%p,%p,%p)\n", iface, ppIFrameEncode, ppIEncoderOptions);

    EnterCriticalSection(&This->lock);

    if (!This->icns_family)
    {
        hr = WINCODEC_ERR_NOTINITIALIZED;
        goto end;
    }

    hr = CreatePropertyBag2(NULL, 0, ppIEncoderOptions);
    if (FAILED(hr))
        goto end;

    frameEncode = HeapAlloc(GetProcessHeap(), 0, sizeof(IcnsFrameEncode));
    if (frameEncode == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto end;
    }
    frameEncode->IWICBitmapFrameEncode_iface.lpVtbl = &IcnsEncoder_FrameVtbl;
    frameEncode->encoder = This;
    frameEncode->ref = 1;
    frameEncode->initialized = FALSE;
    frameEncode->size = 0;
    frameEncode->icns_image = NULL;
    frameEncode->lines_written = 0;
    frameEncode->committed = FALSE;
    *ppIFrameEncode = &frameEncode->IWICBitmapFrameEncode_iface;
    This->outstanding_commits++;
    IWICBitmapEncoder_AddRef(&This->IWICBitmapEncoder_iface);

end:
    LeaveCriticalSection(&This->lock);

    return hr;
}

static HRESULT WINAPI IcnsEncoder_Commit(IWICBitmapEncoder *iface)
{
    IcnsEncoder *This = impl_from_IWICBitmapEncoder(iface);
    size_t buffer_size;
    HRESULT hr = S_OK;
    ULONG byteswritten;

    TRACE("(%p)\n", iface);

    EnterCriticalSection(&This->lock);

    if (!This->any_frame_committed || This->outstanding_commits > 0 || This->committed)
    {
        hr = WINCODEC_ERR_WRONGSTATE;
        goto end;
    }

    buffer_size = GetHandleSize((Handle)This->icns_family);
    hr = IStream_Write(This->stream, *This->icns_family, buffer_size, &byteswritten);
    if (FAILED(hr) || byteswritten != buffer_size)
    {
        WARN("writing file failed, hr = 0x%08X\n", hr);
        hr = E_FAIL;
        goto end;
    }

    This->committed = TRUE;

end:
    LeaveCriticalSection(&This->lock);
    return hr;
}

static HRESULT WINAPI IcnsEncoder_GetMetadataQueryWriter(IWICBitmapEncoder *iface,
    IWICMetadataQueryWriter **ppIMetadataQueryWriter)
{
    FIXME("(%p,%p): stub\n", iface, ppIMetadataQueryWriter);
    return E_NOTIMPL;
}

static const IWICBitmapEncoderVtbl IcnsEncoder_Vtbl = {
    IcnsEncoder_QueryInterface,
    IcnsEncoder_AddRef,
    IcnsEncoder_Release,
    IcnsEncoder_Initialize,
    IcnsEncoder_GetContainerFormat,
    IcnsEncoder_GetEncoderInfo,
    IcnsEncoder_SetColorContexts,
    IcnsEncoder_SetPalette,
    IcnsEncoder_SetThumbnail,
    IcnsEncoder_SetPreview,
    IcnsEncoder_CreateNewFrame,
    IcnsEncoder_Commit,
    IcnsEncoder_GetMetadataQueryWriter
};

HRESULT IcnsEncoder_CreateInstance(REFIID iid, void** ppv)
{
    IcnsEncoder *This;
    HRESULT ret;

    TRACE("(%s,%p)\n", debugstr_guid(iid), ppv);

    *ppv = NULL;

    This = HeapAlloc(GetProcessHeap(), 0, sizeof(IcnsEncoder));
    if (!This) return E_OUTOFMEMORY;

    This->IWICBitmapEncoder_iface.lpVtbl = &IcnsEncoder_Vtbl;
    This->ref = 1;
    This->stream = NULL;
    This->icns_family = NULL;
    This->any_frame_committed = FALSE;
    This->outstanding_commits = 0;
    This->committed = FALSE;
    InitializeCriticalSection(&This->lock);
    This->lock.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": IcnsEncoder.lock");

    ret = IWICBitmapEncoder_QueryInterface(&This->IWICBitmapEncoder_iface, iid, ppv);
    IWICBitmapEncoder_Release(&This->IWICBitmapEncoder_iface);

    return ret;
}

#else /* !defined(HAVE_APPLICATIONSERVICES_APPLICATIONSERVICES_H) ||
         MAC_OS_X_VERSION_MAX_ALLOWED <= MAC_OS_X_VERSION_10_4 */

HRESULT IcnsEncoder_CreateInstance(REFIID iid, void** ppv)
{
    ERR("Trying to save ICNS picture, but ICNS support is not compiled in.\n");
    return E_FAIL;
}

#endif
