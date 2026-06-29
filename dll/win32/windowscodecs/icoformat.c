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

#include <stdarg.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "objbase.h"

#include "wincodecs_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(wincodecs);

#include "pshpack1.h"

typedef struct {
    BYTE bWidth;
    BYTE bHeight;
    BYTE bColorCount;
    BYTE bReserved;
    WORD wPlanes;
    WORD wBitCount;
    DWORD dwDIBSize;
    DWORD dwDIBOffset;
} ICONDIRENTRY;

typedef struct
{
    WORD idReserved;
    WORD idType;
    WORD idCount;
} ICONHEADER;

#include "poppack.h"

typedef struct {
    IWICBitmapDecoder IWICBitmapDecoder_iface;
    LONG ref;
    BOOL initialized;
    IStream *stream;
    ICONHEADER header;
    CRITICAL_SECTION lock; /* must be held when accessing stream */
} IcoDecoder;

typedef struct {
    IWICBitmapFrameDecode IWICBitmapFrameDecode_iface;
    LONG ref;
    UINT width, height;
    double dpiX, dpiY;
    BYTE *bits;
} IcoFrameDecode;

static inline IcoDecoder *impl_from_IWICBitmapDecoder(IWICBitmapDecoder *iface)
{
    return CONTAINING_RECORD(iface, IcoDecoder, IWICBitmapDecoder_iface);
}

static inline IcoFrameDecode *impl_from_IWICBitmapFrameDecode(IWICBitmapFrameDecode *iface)
{
    return CONTAINING_RECORD(iface, IcoFrameDecode, IWICBitmapFrameDecode_iface);
}

static HRESULT WINAPI IcoFrameDecode_QueryInterface(IWICBitmapFrameDecode *iface, REFIID iid,
    void **ppv)
{
    IcoFrameDecode *This = impl_from_IWICBitmapFrameDecode(iface);
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

static ULONG WINAPI IcoFrameDecode_AddRef(IWICBitmapFrameDecode *iface)
{
    IcoFrameDecode *This = impl_from_IWICBitmapFrameDecode(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) refcount=%lu\n", iface, ref);

    return ref;
}

static ULONG WINAPI IcoFrameDecode_Release(IWICBitmapFrameDecode *iface)
{
    IcoFrameDecode *This = impl_from_IWICBitmapFrameDecode(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) refcount=%lu\n", iface, ref);

    if (ref == 0)
    {
        free(This->bits);
        free(This);
    }

    return ref;
}

static HRESULT WINAPI IcoFrameDecode_GetSize(IWICBitmapFrameDecode *iface,
    UINT *puiWidth, UINT *puiHeight)
{
    IcoFrameDecode *This = impl_from_IWICBitmapFrameDecode(iface);

    *puiWidth = This->width;
    *puiHeight = This->height;

    TRACE("(%p) -> (%i,%i)\n", iface, *puiWidth, *puiHeight);

    return S_OK;
}

static HRESULT WINAPI IcoFrameDecode_GetPixelFormat(IWICBitmapFrameDecode *iface,
    WICPixelFormatGUID *pPixelFormat)
{
    memcpy(pPixelFormat, &GUID_WICPixelFormat32bppBGRA, sizeof(GUID));
    return S_OK;
}

static HRESULT WINAPI IcoFrameDecode_GetResolution(IWICBitmapFrameDecode *iface,
    double *pDpiX, double *pDpiY)
{
    IcoFrameDecode *This = impl_from_IWICBitmapFrameDecode(iface);

    *pDpiX = This->dpiX;
    *pDpiY = This->dpiY;

    TRACE("(%p) -> (%f,%f)\n", iface, *pDpiX, *pDpiY);

    return S_OK;
}

static HRESULT WINAPI IcoFrameDecode_CopyPalette(IWICBitmapFrameDecode *iface,
    IWICPalette *pIPalette)
{
    TRACE("(%p,%p)\n", iface, pIPalette);
    return WINCODEC_ERR_PALETTEUNAVAILABLE;
}

static HRESULT WINAPI IcoFrameDecode_CopyPixels(IWICBitmapFrameDecode *iface,
    const WICRect *prc, UINT cbStride, UINT cbBufferSize, BYTE *pbBuffer)
{
    IcoFrameDecode *This = impl_from_IWICBitmapFrameDecode(iface);
    TRACE("(%p,%s,%u,%u,%p)\n", iface, debug_wic_rect(prc), cbStride, cbBufferSize, pbBuffer);

    return copy_pixels(32, This->bits, This->width, This->height, This->width * 4,
        prc, cbStride, cbBufferSize, pbBuffer);
}

static HRESULT WINAPI IcoFrameDecode_GetMetadataQueryReader(IWICBitmapFrameDecode *iface,
    IWICMetadataQueryReader **ppIMetadataQueryReader)
{
    TRACE("(%p,%p)\n", iface, ppIMetadataQueryReader);
    return WINCODEC_ERR_UNSUPPORTEDOPERATION;
}

static HRESULT WINAPI IcoFrameDecode_GetColorContexts(IWICBitmapFrameDecode *iface,
    UINT cCount, IWICColorContext **ppIColorContexts, UINT *pcActualCount)
{
    TRACE("(%p,%u,%p,%p)\n", iface, cCount, ppIColorContexts, pcActualCount);
    return WINCODEC_ERR_UNSUPPORTEDOPERATION;
}

static HRESULT WINAPI IcoFrameDecode_GetThumbnail(IWICBitmapFrameDecode *iface,
    IWICBitmapSource **ppIThumbnail)
{
    TRACE("(%p,%p)\n", iface, ppIThumbnail);
    return IWICBitmapFrameDecode_QueryInterface(iface, &IID_IWICBitmapSource, (void **)ppIThumbnail);
}

static const IWICBitmapFrameDecodeVtbl IcoFrameDecode_Vtbl = {
    IcoFrameDecode_QueryInterface,
    IcoFrameDecode_AddRef,
    IcoFrameDecode_Release,
    IcoFrameDecode_GetSize,
    IcoFrameDecode_GetPixelFormat,
    IcoFrameDecode_GetResolution,
    IcoFrameDecode_CopyPalette,
    IcoFrameDecode_CopyPixels,
    IcoFrameDecode_GetMetadataQueryReader,
    IcoFrameDecode_GetColorContexts,
    IcoFrameDecode_GetThumbnail
};

static inline void pixel_set_trans(DWORD* pixel, BOOL transparent)
{
    if (transparent) *pixel = 0;
    else *pixel |= 0xff000000;
}

static HRESULT ReadIcoDib(IStream *stream, IcoFrameDecode *result)
{
    HRESULT hr;
    BmpDecoder *bmp_decoder;
    IWICBitmapDecoder *decoder;
    IWICBitmapFrameDecode *framedecode;
    WICPixelFormatGUID pixelformat;
    IWICBitmapSource *source;
    BOOL has_alpha=FALSE; /* if TRUE, alpha data might be in the image data */
    WICRect rc;

    hr = IcoDibDecoder_CreateInstance(&bmp_decoder);
    if (SUCCEEDED(hr))
    {
        BmpDecoder_GetWICDecoder(bmp_decoder, &decoder);
        hr = IWICBitmapDecoder_Initialize(decoder, stream, WICDecodeMetadataCacheOnLoad);

        if (SUCCEEDED(hr))
            hr = IWICBitmapDecoder_GetFrame(decoder, 0, &framedecode);

        if (SUCCEEDED(hr))
        {
            hr = IWICBitmapFrameDecode_GetSize(framedecode, &result->width, &result->height);

            if (SUCCEEDED(hr))
            {
                result->bits = malloc(result->width * result->height * 4);
                if (!result->bits) hr = E_OUTOFMEMORY;
            }

            if (SUCCEEDED(hr))
                hr = IWICBitmapFrameDecode_GetPixelFormat(framedecode, &pixelformat);

            if (IsEqualGUID(&pixelformat, &GUID_WICPixelFormat32bppBGR) ||
                IsEqualGUID(&pixelformat, &GUID_WICPixelFormat32bppBGRA))
            {
                source = (IWICBitmapSource*)framedecode;
                IWICBitmapSource_AddRef(source);
                has_alpha = TRUE;
            }
            else
            {
                hr = WICConvertBitmapSource(&GUID_WICPixelFormat32bppBGRA,
                    (IWICBitmapSource*)framedecode, &source);
                has_alpha = FALSE;
            }

            if (SUCCEEDED(hr))
            {
                rc.X = 0;
                rc.Y = 0;
                rc.Width = result->width;
                rc.Height = result->height;
                hr = IWICBitmapSource_CopyPixels(source, &rc, result->width * 4,
                    result->width * result->height * 4, result->bits);

                IWICBitmapSource_Release(source);
            }

            if (SUCCEEDED(hr))
                hr = IWICBitmapFrameDecode_GetResolution(framedecode, &result->dpiX, &result->dpiY);

            IWICBitmapFrameDecode_Release(framedecode);
        }

        if (SUCCEEDED(hr) && has_alpha)
        {
            /* If the alpha channel is fully transparent, we should ignore it. */
            int nonzero_alpha = 0;
            UINT i;

            for (i=0; i<(result->height*result->width); i++)
            {
                if (result->bits[i*4+3] != 0)
                {
                    nonzero_alpha = 1;
                    break;
                }
            }

            if (!nonzero_alpha)
            {
                for (i=0; i<(result->height*result->width); i++)
                    result->bits[i*4+3] = 0xff;

                has_alpha = FALSE;
            }
        }

        if (SUCCEEDED(hr) && !has_alpha)
        {
            /* set alpha data based on the AND mask */
            UINT andBytesPerRow = (result->width+31)/32*4;
            UINT andBytes = andBytesPerRow * result->height;
            INT andStride;
            BYTE *tempdata=NULL;
            BYTE *andRow;
            BYTE *bitsRow;
            UINT bitsStride = result->width * 4;
            UINT x, y;
            ULONG offset;
            ULONG bytesread;
            LARGE_INTEGER seek;
            int topdown;

            BmpDecoder_FindIconMask(bmp_decoder, &offset, &topdown);

            if (offset)
            {
                seek.QuadPart = offset;

                hr = IStream_Seek(stream, seek, STREAM_SEEK_SET, 0);

                if (SUCCEEDED(hr))
                {
                    tempdata = malloc(andBytes);
                    if (!tempdata) hr = E_OUTOFMEMORY;
                }

                if (SUCCEEDED(hr))
                    hr = IStream_Read(stream, tempdata, andBytes, &bytesread);

                if (SUCCEEDED(hr) && bytesread == andBytes)
                {
                    if (topdown)
                    {
                        andStride = andBytesPerRow;
                        andRow = tempdata;
                    }
                    else
                    {
                        andStride = -andBytesPerRow;
                        andRow = tempdata + (result->height-1)*andBytesPerRow;
                    }

                    bitsRow = result->bits;
                    for (y=0; y<result->height; y++) {
                        BYTE *andByte=andRow;
                        DWORD *bitsPixel=(DWORD*)bitsRow;
                        for (x=0; x<result->width; x+=8) {
                            BYTE andVal=*andByte++;
                            pixel_set_trans(bitsPixel++, andVal>>7&1);
                            if (x+1 < result->width) pixel_set_trans(bitsPixel++, andVal>>6&1);
                            if (x+2 < result->width) pixel_set_trans(bitsPixel++, andVal>>5&1);
                            if (x+3 < result->width) pixel_set_trans(bitsPixel++, andVal>>4&1);
                            if (x+4 < result->width) pixel_set_trans(bitsPixel++, andVal>>3&1);
                            if (x+5 < result->width) pixel_set_trans(bitsPixel++, andVal>>2&1);
                            if (x+6 < result->width) pixel_set_trans(bitsPixel++, andVal>>1&1);
                            if (x+7 < result->width) pixel_set_trans(bitsPixel++, andVal&1);
                        }
                        andRow += andStride;
                        bitsRow += bitsStride;
                    }
                }

                free(tempdata);
            }
        }

        IWICBitmapDecoder_Release(decoder);
    }

    return hr;
}

static HRESULT ReadIcoPng(IStream *stream, IcoFrameDecode *result)
{
    IWICBitmapDecoder *decoder = NULL;
    IWICBitmapFrameDecode *sourceFrame = NULL;
    IWICBitmapSource *sourceBitmap = NULL;
    WICRect rect;
    HRESULT hr;

    hr = PngDecoder_CreateInstance(&IID_IWICBitmapDecoder, (void**)&decoder);
    if (FAILED(hr))
        goto end;
    hr = IWICBitmapDecoder_Initialize(decoder, stream, WICDecodeMetadataCacheOnLoad);
    if (FAILED(hr))
        goto end;
    hr = IWICBitmapDecoder_GetFrame(decoder, 0, &sourceFrame);
    if (FAILED(hr))
        goto end;
    hr = WICConvertBitmapSource(&GUID_WICPixelFormat32bppBGRA, (IWICBitmapSource*)sourceFrame, &sourceBitmap);
    if (FAILED(hr))
        goto end;
    hr = IWICBitmapFrameDecode_GetSize(sourceFrame, &result->width, &result->height);
    if (FAILED(hr))
        goto end;
    hr = IWICBitmapFrameDecode_GetResolution(sourceFrame, &result->dpiX, &result->dpiY);
    if (FAILED(hr))
        goto end;
    result->bits = malloc(4 * result->width * result->height);
    if (result->bits == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto end;
    }
    rect.X = 0;
    rect.Y = 0;
    rect.Width = result->width;
    rect.Height = result->height;
    hr = IWICBitmapSource_CopyPixels(sourceBitmap, &rect, 4*result->width,
                                     4*result->width*result->height, result->bits);

end:
    if (decoder != NULL)
        IWICBitmapDecoder_Release(decoder);
    if (sourceFrame != NULL)
        IWICBitmapFrameDecode_Release(sourceFrame);
    if (sourceBitmap != NULL)
        IWICBitmapSource_Release(sourceBitmap);
    return hr;
}

static HRESULT WINAPI IcoDecoder_QueryInterface(IWICBitmapDecoder *iface, REFIID iid,
    void **ppv)
{
    IcoDecoder *This = impl_from_IWICBitmapDecoder(iface);
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

static ULONG WINAPI IcoDecoder_AddRef(IWICBitmapDecoder *iface)
{
    IcoDecoder *This = impl_from_IWICBitmapDecoder(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) refcount=%lu\n", iface, ref);

    return ref;
}

static ULONG WINAPI IcoDecoder_Release(IWICBitmapDecoder *iface)
{
    IcoDecoder *This = impl_from_IWICBitmapDecoder(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) refcount=%lu\n", iface, ref);

    if (ref == 0)
    {
        This->lock.DebugInfo->Spare[0] = 0;
        DeleteCriticalSection(&This->lock);
        if (This->stream) IStream_Release(This->stream);
        free(This);
    }

    return ref;
}

static HRESULT WINAPI IcoDecoder_QueryCapability(IWICBitmapDecoder *iface, IStream *stream,
    DWORD *capability)
{
    HRESULT hr;

    TRACE("(%p,%p,%p)\n", iface, stream, capability);

    if (!stream || !capability) return E_INVALIDARG;

    hr = IWICBitmapDecoder_Initialize(iface, stream, WICDecodeMetadataCacheOnDemand);
    if (hr != S_OK) return hr;

    *capability = WICBitmapDecoderCapabilityCanDecodeAllImages;
    return S_OK;
}

static HRESULT WINAPI IcoDecoder_Initialize(IWICBitmapDecoder *iface, IStream *pIStream,
    WICDecodeOptions cacheOptions)
{
    IcoDecoder *This = impl_from_IWICBitmapDecoder(iface);
    LARGE_INTEGER seek;
    HRESULT hr;
    ULONG bytesread;
    STATSTG statstg;
    unsigned int i;

    TRACE("(%p,%p,%x)\n", iface, pIStream, cacheOptions);

    EnterCriticalSection(&This->lock);

    if (This->initialized)
    {
        hr = WINCODEC_ERR_WRONGSTATE;
        goto end;
    }

    seek.QuadPart = 0;
    hr = IStream_Seek(pIStream, seek, STREAM_SEEK_SET, NULL);
    if (FAILED(hr)) goto end;

    hr = IStream_Read(pIStream, &This->header, sizeof(ICONHEADER), &bytesread);
    if (FAILED(hr)) goto end;

    if (bytesread != sizeof(ICONHEADER))
    {
        hr = WINCODEC_ERR_STREAMREAD;
        goto end;
    }

    if (This->header.idReserved != 0 ||
        This->header.idType != 1)
    {
        hr = E_FAIL;
        goto end;
    }

    hr = IStream_Stat(pIStream, &statstg, STATFLAG_NONAME);
    if (FAILED(hr))
    {
        WARN("Stat() failed, hr %#lx.\n", hr);
        goto end;
    }

    for (i = 0; i < This->header.idCount; i++)
    {
        ICONDIRENTRY direntry;

        hr = IStream_Read(pIStream, &direntry, sizeof(direntry), &bytesread);
        if (FAILED(hr)) goto end;

        if (bytesread != sizeof(direntry) || (direntry.dwDIBSize + direntry.dwDIBOffset > statstg.cbSize.QuadPart))
        {
            hr = WINCODEC_ERR_BADIMAGE;
            goto end;
        }
    }

    This->initialized = TRUE;
    This->stream = pIStream;
    IStream_AddRef(pIStream);

end:

    LeaveCriticalSection(&This->lock);

    return hr;
}

static HRESULT WINAPI IcoDecoder_GetContainerFormat(IWICBitmapDecoder *iface,
    GUID *pguidContainerFormat)
{
    memcpy(pguidContainerFormat, &GUID_ContainerFormatIco, sizeof(GUID));
    return S_OK;
}

static HRESULT WINAPI IcoDecoder_GetDecoderInfo(IWICBitmapDecoder *iface,
    IWICBitmapDecoderInfo **ppIDecoderInfo)
{
    TRACE("(%p,%p)\n", iface, ppIDecoderInfo);

    return get_decoder_info(&CLSID_WICIcoDecoder, ppIDecoderInfo);
}

static HRESULT WINAPI IcoDecoder_CopyPalette(IWICBitmapDecoder *iface,
    IWICPalette *pIPalette)
{
    TRACE("(%p,%p)\n", iface, pIPalette);
    return WINCODEC_ERR_PALETTEUNAVAILABLE;
}

static HRESULT WINAPI IcoDecoder_GetMetadataQueryReader(IWICBitmapDecoder *iface,
    IWICMetadataQueryReader **ppIMetadataQueryReader)
{
    TRACE("(%p,%p)\n", iface, ppIMetadataQueryReader);
    return WINCODEC_ERR_UNSUPPORTEDOPERATION;
}

static HRESULT WINAPI IcoDecoder_GetPreview(IWICBitmapDecoder *iface,
    IWICBitmapSource **ppIBitmapSource)
{
    TRACE("(%p,%p)\n", iface, ppIBitmapSource);
    return WINCODEC_ERR_UNSUPPORTEDOPERATION;
}

static HRESULT WINAPI IcoDecoder_GetColorContexts(IWICBitmapDecoder *iface,
    UINT cCount, IWICColorContext **ppIColorContexts, UINT *pcActualCount)
{
    TRACE("(%p,%u,%p,%p)\n", iface, cCount, ppIColorContexts, pcActualCount);
    return WINCODEC_ERR_UNSUPPORTEDOPERATION;
}

static HRESULT WINAPI IcoDecoder_GetThumbnail(IWICBitmapDecoder *iface,
    IWICBitmapSource **ppIThumbnail)
{
    TRACE("(%p,%p)\n", iface, ppIThumbnail);
    return WINCODEC_ERR_CODECNOTHUMBNAIL;
}

static HRESULT WINAPI IcoDecoder_GetFrameCount(IWICBitmapDecoder *iface,
    UINT *pCount)
{
    IcoDecoder *This = impl_from_IWICBitmapDecoder(iface);

    if (!pCount) return E_INVALIDARG;

    EnterCriticalSection(&This->lock);
    *pCount = This->initialized ? This->header.idCount : 0;
    LeaveCriticalSection(&This->lock);

    TRACE("(%p) <-- %d\n", iface, *pCount);

    return S_OK;
}

static HRESULT WINAPI IcoDecoder_GetFrame(IWICBitmapDecoder *iface,
    UINT index, IWICBitmapFrameDecode **ppIBitmapFrame)
{
    IcoDecoder *This = impl_from_IWICBitmapDecoder(iface);
    IcoFrameDecode *result=NULL;
    LARGE_INTEGER seek;
    ULARGE_INTEGER offset, length;
    HRESULT hr;
    ULONG bytesread;
    ICONDIRENTRY entry;
    IWICStream *substream=NULL;
    DWORD magic;
    TRACE("(%p,%u,%p)\n", iface, index, ppIBitmapFrame);

    EnterCriticalSection(&This->lock);

    if (!This->initialized)
    {
        hr = WINCODEC_ERR_FRAMEMISSING;
        goto fail;
    }

    if (This->header.idCount < index)
    {
        hr = E_INVALIDARG;
        goto fail;
    }

    result = malloc(sizeof(IcoFrameDecode));
    if (!result)
    {
        hr = E_OUTOFMEMORY;
        goto fail;
    }

    result->IWICBitmapFrameDecode_iface.lpVtbl = &IcoFrameDecode_Vtbl;
    result->ref = 1;
    result->bits = NULL;

    /* read the icon entry */
    seek.QuadPart = sizeof(ICONHEADER) + sizeof(ICONDIRENTRY) * index;
    hr = IStream_Seek(This->stream, seek, STREAM_SEEK_SET, 0);
    if (FAILED(hr)) goto fail;

    hr = IStream_Read(This->stream, &entry, sizeof(ICONDIRENTRY), &bytesread);
    if (FAILED(hr) || bytesread != sizeof(ICONDIRENTRY)) goto fail;

    /* create a stream object for this icon */
    hr = StreamImpl_Create(&substream);
    if (FAILED(hr)) goto fail;

    offset.QuadPart = entry.dwDIBOffset;
    length.QuadPart = entry.dwDIBSize;
    hr = IWICStream_InitializeFromIStreamRegion(substream, This->stream, offset, length);
    if (FAILED(hr)) goto fail;

    /* read the bitmapinfo size or magic number */
    hr = IWICStream_Read(substream, &magic, sizeof(magic), &bytesread);
    if (FAILED(hr) || bytesread != sizeof(magic)) goto fail;

    /* forward to the appropriate decoding function based on the magic number */
    switch (magic)
    {
    case sizeof(BITMAPCOREHEADER):
    case 64: /* sizeof(BITMAPCOREHEADER2) */
    case sizeof(BITMAPINFOHEADER):
    case sizeof(BITMAPV4HEADER):
    case sizeof(BITMAPV5HEADER):
        hr = ReadIcoDib((IStream*)substream, result);
        break;
    case 0x474e5089:
        hr = ReadIcoPng((IStream*)substream, result);
        break;
    default:
        FIXME("Unrecognized ICO frame magic: %lx\n", magic);
        hr = E_FAIL;
        break;
    }
    if (FAILED(hr)) goto fail;

    *ppIBitmapFrame = &result->IWICBitmapFrameDecode_iface;

    LeaveCriticalSection(&This->lock);

    IWICStream_Release(substream);

    return S_OK;

fail:
    LeaveCriticalSection(&This->lock);
    free(result);
    if (substream) IWICStream_Release(substream);
    if (SUCCEEDED(hr)) hr = E_FAIL;
    TRACE("<-- %lx\n", hr);
    return hr;
}

static const IWICBitmapDecoderVtbl IcoDecoder_Vtbl = {
    IcoDecoder_QueryInterface,
    IcoDecoder_AddRef,
    IcoDecoder_Release,
    IcoDecoder_QueryCapability,
    IcoDecoder_Initialize,
    IcoDecoder_GetContainerFormat,
    IcoDecoder_GetDecoderInfo,
    IcoDecoder_CopyPalette,
    IcoDecoder_GetMetadataQueryReader,
    IcoDecoder_GetPreview,
    IcoDecoder_GetColorContexts,
    IcoDecoder_GetThumbnail,
    IcoDecoder_GetFrameCount,
    IcoDecoder_GetFrame
};

HRESULT IcoDecoder_CreateInstance(REFIID iid, void** ppv)
{
    IcoDecoder *This;
    HRESULT ret;

    TRACE("(%s,%p)\n", debugstr_guid(iid), ppv);

    *ppv = NULL;

    This = malloc(sizeof(IcoDecoder));
    if (!This) return E_OUTOFMEMORY;

    This->IWICBitmapDecoder_iface.lpVtbl = &IcoDecoder_Vtbl;
    This->ref = 1;
    This->stream = NULL;
    This->initialized = FALSE;
#ifdef __REACTOS__
    InitializeCriticalSection(&This->lock);
#else
    InitializeCriticalSectionEx(&This->lock, 0, RTL_CRITICAL_SECTION_FLAG_FORCE_DEBUG_INFO);
#endif
    This->lock.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": IcoDecoder.lock");

    ret = IWICBitmapDecoder_QueryInterface(&This->IWICBitmapDecoder_iface, iid, ppv);
    IWICBitmapDecoder_Release(&This->IWICBitmapDecoder_iface);

    return ret;
}
