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
#include "winreg.h"
#include "objbase.h"
#include "shellapi.h"
#include "wincodec.h"

#include "wincodecs_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(wincodecs);

typedef struct {
    const IWICImagingFactoryVtbl    *lpIWICImagingFactoryVtbl;
    LONG ref;
} ImagingFactory;

static HRESULT WINAPI ImagingFactory_QueryInterface(IWICImagingFactory *iface, REFIID iid,
    void **ppv)
{
    ImagingFactory *This = (ImagingFactory*)iface;
    TRACE("(%p,%s,%p)\n", iface, debugstr_guid(iid), ppv);

    if (!ppv) return E_INVALIDARG;

    if (IsEqualIID(&IID_IUnknown, iid) || IsEqualIID(&IID_IWICImagingFactory, iid))
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

static ULONG WINAPI ImagingFactory_AddRef(IWICImagingFactory *iface)
{
    ImagingFactory *This = (ImagingFactory*)iface;
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) refcount=%u\n", iface, ref);

    return ref;
}

static ULONG WINAPI ImagingFactory_Release(IWICImagingFactory *iface)
{
    ImagingFactory *This = (ImagingFactory*)iface;
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) refcount=%u\n", iface, ref);

    if (ref == 0)
        HeapFree(GetProcessHeap(), 0, This);

    return ref;
}

static HRESULT WINAPI ImagingFactory_CreateDecoderFromFilename(
    IWICImagingFactory *iface, LPCWSTR wzFilename, const GUID *pguidVendor,
    DWORD dwDesiredAccess, WICDecodeOptions metadataOptions,
    IWICBitmapDecoder **ppIDecoder)
{
    IWICStream *stream;
    HRESULT hr;

    TRACE("(%p,%s,%s,%u,%u,%p)\n", iface, debugstr_w(wzFilename),
        debugstr_guid(pguidVendor), dwDesiredAccess, metadataOptions, ppIDecoder);

    hr = StreamImpl_Create(&stream);
    if (SUCCEEDED(hr))
    {
        hr = IWICStream_InitializeFromFilename(stream, wzFilename, dwDesiredAccess);

        if (SUCCEEDED(hr))
        {
            hr = IWICImagingFactory_CreateDecoderFromStream(iface, (IStream*)stream,
                pguidVendor, metadataOptions, ppIDecoder);
        }

        IWICStream_Release(stream);
    }

    return hr;
}

static HRESULT WINAPI ImagingFactory_CreateDecoderFromStream(
    IWICImagingFactory *iface, IStream *pIStream, const GUID *pguidVendor,
    WICDecodeOptions metadataOptions, IWICBitmapDecoder **ppIDecoder)
{
    static int fixme=0;
    IEnumUnknown *enumdecoders;
    IUnknown *unkdecoderinfo;
    IWICBitmapDecoderInfo *decoderinfo;
    IWICBitmapDecoder *decoder=NULL;
    HRESULT res=S_OK;
    ULONG num_fetched;
    BOOL matches;

    TRACE("(%p,%p,%s,%u,%p)\n", iface, pIStream, debugstr_guid(pguidVendor),
        metadataOptions, ppIDecoder);

    if (pguidVendor && !fixme++)
        FIXME("ignoring vendor GUID\n");

    res = CreateComponentEnumerator(WICDecoder, WICComponentEnumerateDefault, &enumdecoders);
    if (FAILED(res)) return res;

    while (!decoder)
    {
        res = IEnumUnknown_Next(enumdecoders, 1, &unkdecoderinfo, &num_fetched);

        if (res == S_OK)
        {
            res = IUnknown_QueryInterface(unkdecoderinfo, &IID_IWICBitmapDecoderInfo, (void**)&decoderinfo);

            if (SUCCEEDED(res))
            {
                res = IWICBitmapDecoderInfo_MatchesPattern(decoderinfo, pIStream, &matches);

                if (SUCCEEDED(res) && matches)
                {
                    res = IWICBitmapDecoderInfo_CreateInstance(decoderinfo, &decoder);

                    /* FIXME: should use QueryCapability to choose a decoder */

                    if (SUCCEEDED(res))
                    {
                        res = IWICBitmapDecoder_Initialize(decoder, pIStream, metadataOptions);

                        if (FAILED(res))
                        {
                            IWICBitmapDecoder_Release(decoder);
                            decoder = NULL;
                        }
                    }
                }

                IWICBitmapDecoderInfo_Release(decoderinfo);
            }

            IUnknown_Release(unkdecoderinfo);
        }
        else
            break;
    }

    IEnumUnknown_Release(enumdecoders);

    if (decoder)
    {
        *ppIDecoder = decoder;
        return S_OK;
    }
    else
    {
        if (WARN_ON(wincodecs))
        {
            LARGE_INTEGER seek;
            BYTE data[4];
            ULONG bytesread;

            WARN("failed to load from a stream\n");

            seek.QuadPart = 0;
            res = IStream_Seek(pIStream, seek, STREAM_SEEK_SET, NULL);
            if (SUCCEEDED(res))
                res = IStream_Read(pIStream, data, 4, &bytesread);
            if (SUCCEEDED(res))
                WARN("first %i bytes of stream=%x %x %x %x\n", bytesread, data[0], data[1], data[2], data[3]);
        }
        *ppIDecoder = NULL;
        return WINCODEC_ERR_COMPONENTNOTFOUND;
    }
}

static HRESULT WINAPI ImagingFactory_CreateDecoderFromFileHandle(
    IWICImagingFactory *iface, ULONG_PTR hFile, const GUID *pguidVendor,
    WICDecodeOptions metadataOptions, IWICBitmapDecoder **ppIDecoder)
{
    FIXME("(%p,%lx,%s,%u,%p): stub\n", iface, hFile, debugstr_guid(pguidVendor),
        metadataOptions, ppIDecoder);
    return E_NOTIMPL;
}

static HRESULT WINAPI ImagingFactory_CreateComponentInfo(IWICImagingFactory *iface,
    REFCLSID clsidComponent, IWICComponentInfo **ppIInfo)
{
    TRACE("(%p,%s,%p)\n", iface, debugstr_guid(clsidComponent), ppIInfo);
    return CreateComponentInfo(clsidComponent, ppIInfo);
}

static HRESULT WINAPI ImagingFactory_CreateDecoder(IWICImagingFactory *iface,
    REFGUID guidContainerFormat, const GUID *pguidVendor,
    IWICBitmapDecoder **ppIDecoder)
{
    FIXME("(%p,%s,%s,%p): stub\n", iface, debugstr_guid(guidContainerFormat),
        debugstr_guid(pguidVendor), ppIDecoder);
    return E_NOTIMPL;
}

static HRESULT WINAPI ImagingFactory_CreateEncoder(IWICImagingFactory *iface,
    REFGUID guidContainerFormat, const GUID *pguidVendor,
    IWICBitmapEncoder **ppIEncoder)
{
    FIXME("(%p,%s,%s,%p): stub\n", iface, debugstr_guid(guidContainerFormat),
        debugstr_guid(pguidVendor), ppIEncoder);
    return E_NOTIMPL;
}

static HRESULT WINAPI ImagingFactory_CreatePalette(IWICImagingFactory *iface,
    IWICPalette **ppIPalette)
{
    TRACE("(%p,%p)\n", iface, ppIPalette);
    return PaletteImpl_Create(ppIPalette);
}

static HRESULT WINAPI ImagingFactory_CreateFormatConverter(IWICImagingFactory *iface,
    IWICFormatConverter **ppIFormatConverter)
{
    return FormatConverter_CreateInstance(NULL, &IID_IWICFormatConverter, (void**)ppIFormatConverter);
}

static HRESULT WINAPI ImagingFactory_CreateBitmapScaler(IWICImagingFactory *iface,
    IWICBitmapScaler **ppIBitmapScaler)
{
    FIXME("(%p,%p): stub\n", iface, ppIBitmapScaler);
    return E_NOTIMPL;
}

static HRESULT WINAPI ImagingFactory_CreateBitmapClipper(IWICImagingFactory *iface,
    IWICBitmapClipper **ppIBitmapClipper)
{
    FIXME("(%p,%p): stub\n", iface, ppIBitmapClipper);
    return E_NOTIMPL;
}

static HRESULT WINAPI ImagingFactory_CreateBitmapFlipRotator(IWICImagingFactory *iface,
    IWICBitmapFlipRotator **ppIBitmapFlipRotator)
{
    TRACE("(%p,%p)\n", iface, ppIBitmapFlipRotator);
    return FlipRotator_Create(ppIBitmapFlipRotator);
}

static HRESULT WINAPI ImagingFactory_CreateStream(IWICImagingFactory *iface,
    IWICStream **ppIWICStream)
{
    TRACE("(%p,%p)\n", iface, ppIWICStream);
    return StreamImpl_Create(ppIWICStream);
}

static HRESULT WINAPI ImagingFactory_CreateColorContext(IWICImagingFactory *iface,
    IWICColorContext **ppIColorContext)
{
    FIXME("(%p,%p): stub\n", iface, ppIColorContext);
    return E_NOTIMPL;
}

static HRESULT WINAPI ImagingFactory_CreateColorTransformer(IWICImagingFactory *iface,
    IWICColorTransform **ppIColorTransform)
{
    FIXME("(%p,%p): stub\n", iface, ppIColorTransform);
    return E_NOTIMPL;
}

static HRESULT WINAPI ImagingFactory_CreateBitmap(IWICImagingFactory *iface,
    UINT uiWidth, UINT uiHeight, REFWICPixelFormatGUID pixelFormat,
    WICBitmapCreateCacheOption option, IWICBitmap **ppIBitmap)
{
    FIXME("(%p,%u,%u,%s,%u,%p): stub\n", iface, uiWidth, uiHeight,
        debugstr_guid(pixelFormat), option, ppIBitmap);
    return E_NOTIMPL;
}

static HRESULT WINAPI ImagingFactory_CreateBitmapFromSource(IWICImagingFactory *iface,
    IWICBitmapSource *piBitmapSource, WICBitmapCreateCacheOption option,
    IWICBitmap **ppIBitmap)
{
    FIXME("(%p,%p,%u,%p): stub\n", iface, piBitmapSource, option, ppIBitmap);
    return E_NOTIMPL;
}

static HRESULT WINAPI ImagingFactory_CreateBitmapFromSourceRect(IWICImagingFactory *iface,
    IWICBitmapSource *piBitmapSource, UINT x, UINT y, UINT width, UINT height,
    IWICBitmap **ppIBitmap)
{
    FIXME("(%p,%p,%u,%u,%u,%u,%p): stub\n", iface, piBitmapSource, x, y, width,
        height, ppIBitmap);
    return E_NOTIMPL;
}

static HRESULT WINAPI ImagingFactory_CreateBitmapFromMemory(IWICImagingFactory *iface,
    UINT uiWidth, UINT uiHeight, REFWICPixelFormatGUID pixelFormat, UINT cbStride,
    UINT cbBufferSize, BYTE *pbBuffer, IWICBitmap **ppIBitmap)
{
    FIXME("(%p,%u,%u,%s,%u,%u,%p,%p): stub\n", iface, uiWidth, uiHeight,
        debugstr_guid(pixelFormat), cbStride, cbBufferSize, pbBuffer, ppIBitmap);
    return E_NOTIMPL;
}

static HRESULT WINAPI ImagingFactory_CreateBitmapFromHBITMAP(IWICImagingFactory *iface,
    HBITMAP hBitmap, HPALETTE hPalette, WICBitmapAlphaChannelOption options,
    IWICBitmap **ppIBitmap)
{
    FIXME("(%p,%p,%p,%u,%p): stub\n", iface, hBitmap, hPalette, options, ppIBitmap);
    return E_NOTIMPL;
}

static HRESULT WINAPI ImagingFactory_CreateBitmapFromHICON(IWICImagingFactory *iface,
    HICON hIcon, IWICBitmap **ppIBitmap)
{
    FIXME("(%p,%p,%p): stub\n", iface, hIcon, ppIBitmap);
    return E_NOTIMPL;
}

static HRESULT WINAPI ImagingFactory_CreateComponentEnumerator(IWICImagingFactory *iface,
    DWORD componentTypes, DWORD options, IEnumUnknown **ppIEnumUnknown)
{
    TRACE("(%p,%u,%u,%p)\n", iface, componentTypes, options, ppIEnumUnknown);
    return CreateComponentEnumerator(componentTypes, options, ppIEnumUnknown);
}

static HRESULT WINAPI ImagingFactory_CreateFastMetadataEncoderFromDecoder(
    IWICImagingFactory *iface, IWICBitmapDecoder *pIDecoder,
    IWICFastMetadataEncoder **ppIFastEncoder)
{
    FIXME("(%p,%p,%p): stub\n", iface, pIDecoder, ppIFastEncoder);
    return E_NOTIMPL;
}

static HRESULT WINAPI ImagingFactory_CreateFastMetadataEncoderFromFrameDecode(
    IWICImagingFactory *iface, IWICBitmapFrameDecode *pIFrameDecoder,
    IWICFastMetadataEncoder **ppIFastEncoder)
{
    FIXME("(%p,%p,%p): stub\n", iface, pIFrameDecoder, ppIFastEncoder);
    return E_NOTIMPL;
}

static HRESULT WINAPI ImagingFactory_CreateQueryWriter(IWICImagingFactory *iface,
    REFGUID guidMetadataFormat, const GUID *pguidVendor,
    IWICMetadataQueryWriter **ppIQueryWriter)
{
    FIXME("(%p,%s,%s,%p): stub\n", iface, debugstr_guid(guidMetadataFormat),
        debugstr_guid(pguidVendor), ppIQueryWriter);
    return E_NOTIMPL;
}

static HRESULT WINAPI ImagingFactory_CreateQueryWriterFromReader(IWICImagingFactory *iface,
    IWICMetadataQueryReader *pIQueryReader, const GUID *pguidVendor,
    IWICMetadataQueryWriter **ppIQueryWriter)
{
    FIXME("(%p,%p,%s,%p): stub\n", iface, pIQueryReader, debugstr_guid(pguidVendor),
        ppIQueryWriter);
    return E_NOTIMPL;
}

static const IWICImagingFactoryVtbl ImagingFactory_Vtbl = {
    ImagingFactory_QueryInterface,
    ImagingFactory_AddRef,
    ImagingFactory_Release,
    ImagingFactory_CreateDecoderFromFilename,
    ImagingFactory_CreateDecoderFromStream,
    ImagingFactory_CreateDecoderFromFileHandle,
    ImagingFactory_CreateComponentInfo,
    ImagingFactory_CreateDecoder,
    ImagingFactory_CreateEncoder,
    ImagingFactory_CreatePalette,
    ImagingFactory_CreateFormatConverter,
    ImagingFactory_CreateBitmapScaler,
    ImagingFactory_CreateBitmapClipper,
    ImagingFactory_CreateBitmapFlipRotator,
    ImagingFactory_CreateStream,
    ImagingFactory_CreateColorContext,
    ImagingFactory_CreateColorTransformer,
    ImagingFactory_CreateBitmap,
    ImagingFactory_CreateBitmapFromSource,
    ImagingFactory_CreateBitmapFromSourceRect,
    ImagingFactory_CreateBitmapFromMemory,
    ImagingFactory_CreateBitmapFromHBITMAP,
    ImagingFactory_CreateBitmapFromHICON,
    ImagingFactory_CreateComponentEnumerator,
    ImagingFactory_CreateFastMetadataEncoderFromDecoder,
    ImagingFactory_CreateFastMetadataEncoderFromFrameDecode,
    ImagingFactory_CreateQueryWriter,
    ImagingFactory_CreateQueryWriterFromReader
};

HRESULT ImagingFactory_CreateInstance(IUnknown *pUnkOuter, REFIID iid, void** ppv)
{
    ImagingFactory *This;
    HRESULT ret;

    TRACE("(%p,%s,%p)\n", pUnkOuter, debugstr_guid(iid), ppv);

    *ppv = NULL;

    if (pUnkOuter) return CLASS_E_NOAGGREGATION;

    This = HeapAlloc(GetProcessHeap(), 0, sizeof(ImagingFactory));
    if (!This) return E_OUTOFMEMORY;

    This->lpIWICImagingFactoryVtbl = &ImagingFactory_Vtbl;
    This->ref = 1;

    ret = IUnknown_QueryInterface((IUnknown*)This, iid, ppv);
    IUnknown_Release((IUnknown*)This);

    return ret;
}
