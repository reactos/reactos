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

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#include <config.h>

#include <stdarg.h>

#define COBJMACROS

#include <windef.h>
#include <winbase.h>
//#include "winreg.h"
#include <objbase.h>
//#include "shellapi.h"
//#include "wincodec.h"
#include <wincodecsdk.h>

#include "wincodecs_private.h"

#include <wine/debug.h>

WINE_DEFAULT_DEBUG_CHANNEL(wincodecs);

typedef struct {
    IWICComponentFactory IWICComponentFactory_iface;
    LONG ref;
} ComponentFactory;

static inline ComponentFactory *impl_from_IWICComponentFactory(IWICComponentFactory *iface)
{
    return CONTAINING_RECORD(iface, ComponentFactory, IWICComponentFactory_iface);
}

static HRESULT WINAPI ComponentFactory_QueryInterface(IWICComponentFactory *iface, REFIID iid,
    void **ppv)
{
    ComponentFactory *This = impl_from_IWICComponentFactory(iface);
    TRACE("(%p,%s,%p)\n", iface, debugstr_guid(iid), ppv);

    if (!ppv) return E_INVALIDARG;

    if (IsEqualIID(&IID_IUnknown, iid) ||
        IsEqualIID(&IID_IWICImagingFactory, iid) ||
        IsEqualIID(&IID_IWICComponentFactory, iid))
    {
        *ppv = &This->IWICComponentFactory_iface;
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI ComponentFactory_AddRef(IWICComponentFactory *iface)
{
    ComponentFactory *This = impl_from_IWICComponentFactory(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) refcount=%u\n", iface, ref);

    return ref;
}

static ULONG WINAPI ComponentFactory_Release(IWICComponentFactory *iface)
{
    ComponentFactory *This = impl_from_IWICComponentFactory(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) refcount=%u\n", iface, ref);

    if (ref == 0)
        HeapFree(GetProcessHeap(), 0, This);

    return ref;
}

static HRESULT WINAPI ComponentFactory_CreateDecoderFromFilename(
    IWICComponentFactory *iface, LPCWSTR wzFilename, const GUID *pguidVendor,
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
            hr = IWICComponentFactory_CreateDecoderFromStream(iface, (IStream*)stream,
                pguidVendor, metadataOptions, ppIDecoder);
        }

        IWICStream_Release(stream);
    }

    return hr;
}

static IWICBitmapDecoder *find_decoder(IStream *pIStream, const GUID *pguidVendor,
                                       WICDecodeOptions metadataOptions)
{
    IEnumUnknown *enumdecoders;
    IUnknown *unkdecoderinfo;
    IWICBitmapDecoderInfo *decoderinfo;
    IWICBitmapDecoder *decoder = NULL;
    GUID vendor;
    HRESULT res;
    ULONG num_fetched;
    BOOL matches;

    res = CreateComponentEnumerator(WICDecoder, WICComponentEnumerateDefault, &enumdecoders);
    if (FAILED(res)) return NULL;

    while (!decoder)
    {
        res = IEnumUnknown_Next(enumdecoders, 1, &unkdecoderinfo, &num_fetched);

        if (res == S_OK)
        {
            res = IUnknown_QueryInterface(unkdecoderinfo, &IID_IWICBitmapDecoderInfo, (void**)&decoderinfo);

            if (SUCCEEDED(res))
            {
                if (pguidVendor)
                {
                    res = IWICBitmapDecoderInfo_GetVendorGUID(decoderinfo, &vendor);
                    if (FAILED(res) || !IsEqualIID(&vendor, pguidVendor))
                    {
                        IWICBitmapDecoderInfo_Release(decoderinfo);
                        IUnknown_Release(unkdecoderinfo);
                        continue;
                    }
                }

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

    return decoder;
}

static HRESULT WINAPI ComponentFactory_CreateDecoderFromStream(
    IWICComponentFactory *iface, IStream *pIStream, const GUID *pguidVendor,
    WICDecodeOptions metadataOptions, IWICBitmapDecoder **ppIDecoder)
{
    HRESULT res;
    IWICBitmapDecoder *decoder = NULL;

    TRACE("(%p,%p,%s,%u,%p)\n", iface, pIStream, debugstr_guid(pguidVendor),
        metadataOptions, ppIDecoder);

    if (pguidVendor)
        decoder = find_decoder(pIStream, pguidVendor, metadataOptions);
    if (!decoder)
        decoder = find_decoder(pIStream, NULL, metadataOptions);

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

static HRESULT WINAPI ComponentFactory_CreateDecoderFromFileHandle(
    IWICComponentFactory *iface, ULONG_PTR hFile, const GUID *pguidVendor,
    WICDecodeOptions metadataOptions, IWICBitmapDecoder **ppIDecoder)
{
    FIXME("(%p,%lx,%s,%u,%p): stub\n", iface, hFile, debugstr_guid(pguidVendor),
        metadataOptions, ppIDecoder);
    return E_NOTIMPL;
}

static HRESULT WINAPI ComponentFactory_CreateComponentInfo(IWICComponentFactory *iface,
    REFCLSID clsidComponent, IWICComponentInfo **ppIInfo)
{
    TRACE("(%p,%s,%p)\n", iface, debugstr_guid(clsidComponent), ppIInfo);
    return CreateComponentInfo(clsidComponent, ppIInfo);
}

static HRESULT WINAPI ComponentFactory_CreateDecoder(IWICComponentFactory *iface,
    REFGUID guidContainerFormat, const GUID *pguidVendor,
    IWICBitmapDecoder **ppIDecoder)
{
    IEnumUnknown *enumdecoders;
    IUnknown *unkdecoderinfo;
    IWICBitmapDecoderInfo *decoderinfo;
    IWICBitmapDecoder *decoder = NULL, *preferred_decoder = NULL;
    GUID vendor;
    HRESULT res;
    ULONG num_fetched;

    TRACE("(%p,%s,%s,%p)\n", iface, debugstr_guid(guidContainerFormat),
        debugstr_guid(pguidVendor), ppIDecoder);

    if (!guidContainerFormat || !ppIDecoder) return E_INVALIDARG;

    res = CreateComponentEnumerator(WICDecoder, WICComponentEnumerateDefault, &enumdecoders);
    if (FAILED(res)) return res;

    while (!preferred_decoder)
    {
        res = IEnumUnknown_Next(enumdecoders, 1, &unkdecoderinfo, &num_fetched);
        if (res != S_OK) break;

        res = IUnknown_QueryInterface(unkdecoderinfo, &IID_IWICBitmapDecoderInfo, (void **)&decoderinfo);
        if (SUCCEEDED(res))
        {
            GUID container_guid;

            res = IWICBitmapDecoderInfo_GetContainerFormat(decoderinfo, &container_guid);
            if (SUCCEEDED(res) && IsEqualIID(&container_guid, guidContainerFormat))
            {
                IWICBitmapDecoder *new_decoder;

                res = IWICBitmapDecoderInfo_CreateInstance(decoderinfo, &new_decoder);
                if (SUCCEEDED(res))
                {
                    if (pguidVendor)
                    {
                        res = IWICBitmapDecoderInfo_GetVendorGUID(decoderinfo, &vendor);
                        if (SUCCEEDED(res) && IsEqualIID(&vendor, pguidVendor))
                        {
                            preferred_decoder = new_decoder;
                            new_decoder = NULL;
                        }
                    }

                    if (new_decoder && !decoder)
                    {
                        decoder = new_decoder;
                        new_decoder = NULL;
                    }

                    if (new_decoder) IWICBitmapDecoder_Release(new_decoder);
                }
            }

            IWICBitmapDecoderInfo_Release(decoderinfo);
        }

        IUnknown_Release(unkdecoderinfo);
    }

    IEnumUnknown_Release(enumdecoders);

    if (preferred_decoder)
    {
        *ppIDecoder = preferred_decoder;
        if (decoder) IWICBitmapDecoder_Release(decoder);
        return S_OK;
    }

    if (decoder)
    {
        *ppIDecoder = decoder;
        return S_OK;
    }

    *ppIDecoder = NULL;
    return WINCODEC_ERR_COMPONENTNOTFOUND;
}

static HRESULT WINAPI ComponentFactory_CreateEncoder(IWICComponentFactory *iface,
    REFGUID guidContainerFormat, const GUID *pguidVendor,
    IWICBitmapEncoder **ppIEncoder)
{
    static int fixme=0;
    IEnumUnknown *enumencoders;
    IUnknown *unkencoderinfo;
    IWICBitmapEncoderInfo *encoderinfo;
    IWICBitmapEncoder *encoder=NULL;
    HRESULT res=S_OK;
    ULONG num_fetched;
    GUID actual_containerformat;

    TRACE("(%p,%s,%s,%p)\n", iface, debugstr_guid(guidContainerFormat),
        debugstr_guid(pguidVendor), ppIEncoder);

    if (pguidVendor && !fixme++)
        FIXME("ignoring vendor GUID\n");

    res = CreateComponentEnumerator(WICEncoder, WICComponentEnumerateDefault, &enumencoders);
    if (FAILED(res)) return res;

    while (!encoder)
    {
        res = IEnumUnknown_Next(enumencoders, 1, &unkencoderinfo, &num_fetched);

        if (res == S_OK)
        {
            res = IUnknown_QueryInterface(unkencoderinfo, &IID_IWICBitmapEncoderInfo, (void**)&encoderinfo);

            if (SUCCEEDED(res))
            {
                res = IWICBitmapEncoderInfo_GetContainerFormat(encoderinfo, &actual_containerformat);

                if (SUCCEEDED(res) && IsEqualGUID(guidContainerFormat, &actual_containerformat))
                {
                    res = IWICBitmapEncoderInfo_CreateInstance(encoderinfo, &encoder);
                    if (FAILED(res))
                        encoder = NULL;
                }

                IWICBitmapEncoderInfo_Release(encoderinfo);
            }

            IUnknown_Release(unkencoderinfo);
        }
        else
            break;
    }

    IEnumUnknown_Release(enumencoders);

    if (encoder)
    {
        *ppIEncoder = encoder;
        return S_OK;
    }
    else
    {
        WARN("failed to create encoder\n");
        *ppIEncoder = NULL;
        return WINCODEC_ERR_COMPONENTNOTFOUND;
    }
}

static HRESULT WINAPI ComponentFactory_CreatePalette(IWICComponentFactory *iface,
    IWICPalette **ppIPalette)
{
    TRACE("(%p,%p)\n", iface, ppIPalette);
    return PaletteImpl_Create(ppIPalette);
}

static HRESULT WINAPI ComponentFactory_CreateFormatConverter(IWICComponentFactory *iface,
    IWICFormatConverter **ppIFormatConverter)
{
    return FormatConverter_CreateInstance(NULL, &IID_IWICFormatConverter, (void**)ppIFormatConverter);
}

static HRESULT WINAPI ComponentFactory_CreateBitmapScaler(IWICComponentFactory *iface,
    IWICBitmapScaler **ppIBitmapScaler)
{
    TRACE("(%p,%p)\n", iface, ppIBitmapScaler);

    return BitmapScaler_Create(ppIBitmapScaler);
}

static HRESULT WINAPI ComponentFactory_CreateBitmapClipper(IWICComponentFactory *iface,
    IWICBitmapClipper **ppIBitmapClipper)
{
    FIXME("(%p,%p): stub\n", iface, ppIBitmapClipper);
    return E_NOTIMPL;
}

static HRESULT WINAPI ComponentFactory_CreateBitmapFlipRotator(IWICComponentFactory *iface,
    IWICBitmapFlipRotator **ppIBitmapFlipRotator)
{
    TRACE("(%p,%p)\n", iface, ppIBitmapFlipRotator);
    return FlipRotator_Create(ppIBitmapFlipRotator);
}

static HRESULT WINAPI ComponentFactory_CreateStream(IWICComponentFactory *iface,
    IWICStream **ppIWICStream)
{
    TRACE("(%p,%p)\n", iface, ppIWICStream);
    return StreamImpl_Create(ppIWICStream);
}

static HRESULT WINAPI ComponentFactory_CreateColorContext(IWICComponentFactory *iface,
    IWICColorContext **ppIColorContext)
{
    TRACE("(%p,%p)\n", iface, ppIColorContext);
    return ColorContext_Create(ppIColorContext);
}

static HRESULT WINAPI ComponentFactory_CreateColorTransformer(IWICComponentFactory *iface,
    IWICColorTransform **ppIColorTransform)
{
    FIXME("(%p,%p): stub\n", iface, ppIColorTransform);
    return E_NOTIMPL;
}

static HRESULT WINAPI ComponentFactory_CreateBitmap(IWICComponentFactory *iface,
    UINT uiWidth, UINT uiHeight, REFWICPixelFormatGUID pixelFormat,
    WICBitmapCreateCacheOption option, IWICBitmap **ppIBitmap)
{
    TRACE("(%p,%u,%u,%s,%u,%p)\n", iface, uiWidth, uiHeight,
        debugstr_guid(pixelFormat), option, ppIBitmap);
    return BitmapImpl_Create(uiWidth, uiHeight, pixelFormat, option, ppIBitmap);
}

static HRESULT WINAPI ComponentFactory_CreateBitmapFromSource(IWICComponentFactory *iface,
    IWICBitmapSource *piBitmapSource, WICBitmapCreateCacheOption option,
    IWICBitmap **ppIBitmap)
{
    IWICBitmap *result;
    IWICBitmapLock *lock;
    IWICPalette *palette;
    UINT width, height;
    WICPixelFormatGUID pixelformat = {0};
    HRESULT hr;
    WICRect rc;
    double dpix, dpiy;
    IWICComponentInfo *info;
    IWICPixelFormatInfo2 *formatinfo;
    WICPixelFormatNumericRepresentation format_type;

    TRACE("(%p,%p,%u,%p)\n", iface, piBitmapSource, option, ppIBitmap);

    if (!piBitmapSource || !ppIBitmap)
        return E_INVALIDARG;

    hr = IWICBitmapSource_GetSize(piBitmapSource, &width, &height);

    if (SUCCEEDED(hr))
        hr = IWICBitmapSource_GetPixelFormat(piBitmapSource, &pixelformat);

    if (SUCCEEDED(hr))
        hr = CreateComponentInfo(&pixelformat, &info);

    if (SUCCEEDED(hr))
    {
        hr = IWICComponentInfo_QueryInterface(info, &IID_IWICPixelFormatInfo2, (void**)&formatinfo);

        if (SUCCEEDED(hr))
        {
            hr = IWICPixelFormatInfo2_GetNumericRepresentation(formatinfo, &format_type);

            IWICPixelFormatInfo2_Release(formatinfo);
        }

        IWICComponentInfo_Release(info);
    }

    if (SUCCEEDED(hr))
        hr = BitmapImpl_Create(width, height, &pixelformat, option, &result);

    if (SUCCEEDED(hr))
    {
        hr = IWICBitmap_Lock(result, NULL, WICBitmapLockWrite, &lock);
        if (SUCCEEDED(hr))
        {
            UINT stride, buffersize;
            BYTE *buffer;
            rc.X = rc.Y = 0;
            rc.Width = width;
            rc.Height = height;

            hr = IWICBitmapLock_GetStride(lock, &stride);

            if (SUCCEEDED(hr))
                hr = IWICBitmapLock_GetDataPointer(lock, &buffersize, &buffer);

            if (SUCCEEDED(hr))
                hr = IWICBitmapSource_CopyPixels(piBitmapSource, &rc, stride,
                    buffersize, buffer);

            IWICBitmapLock_Release(lock);
        }

        if (SUCCEEDED(hr))
            hr = PaletteImpl_Create(&palette);

        if (SUCCEEDED(hr) && (format_type == WICPixelFormatNumericRepresentationUnspecified ||
                              format_type == WICPixelFormatNumericRepresentationIndexed))
        {
            hr = IWICBitmapSource_CopyPalette(piBitmapSource, palette);

            if (SUCCEEDED(hr))
                hr = IWICBitmap_SetPalette(result, palette);
            else
                hr = S_OK;

            IWICPalette_Release(palette);
        }

        if (SUCCEEDED(hr))
        {
            hr = IWICBitmapSource_GetResolution(piBitmapSource, &dpix, &dpiy);

            if (SUCCEEDED(hr))
                hr = IWICBitmap_SetResolution(result, dpix, dpiy);
            else
                hr = S_OK;
        }

        if (SUCCEEDED(hr))
            *ppIBitmap = result;
        else
            IWICBitmap_Release(result);
    }

    return hr;
}

static HRESULT WINAPI ComponentFactory_CreateBitmapFromSourceRect(IWICComponentFactory *iface,
    IWICBitmapSource *piBitmapSource, UINT x, UINT y, UINT width, UINT height,
    IWICBitmap **ppIBitmap)
{
    FIXME("(%p,%p,%u,%u,%u,%u,%p): stub\n", iface, piBitmapSource, x, y, width,
        height, ppIBitmap);
    return E_NOTIMPL;
}

static HRESULT WINAPI ComponentFactory_CreateBitmapFromMemory(IWICComponentFactory *iface,
    UINT uiWidth, UINT uiHeight, REFWICPixelFormatGUID pixelFormat, UINT cbStride,
    UINT cbBufferSize, BYTE *pbBuffer, IWICBitmap **ppIBitmap)
{
    FIXME("(%p,%u,%u,%s,%u,%u,%p,%p): stub\n", iface, uiWidth, uiHeight,
        debugstr_guid(pixelFormat), cbStride, cbBufferSize, pbBuffer, ppIBitmap);
    return E_NOTIMPL;
}

static HRESULT WINAPI ComponentFactory_CreateBitmapFromHBITMAP(IWICComponentFactory *iface,
    HBITMAP hBitmap, HPALETTE hPalette, WICBitmapAlphaChannelOption options,
    IWICBitmap **ppIBitmap)
{
    FIXME("(%p,%p,%p,%u,%p): stub\n", iface, hBitmap, hPalette, options, ppIBitmap);
    return E_NOTIMPL;
}

static HRESULT WINAPI ComponentFactory_CreateBitmapFromHICON(IWICComponentFactory *iface,
    HICON hIcon, IWICBitmap **ppIBitmap)
{
    FIXME("(%p,%p,%p): stub\n", iface, hIcon, ppIBitmap);
    return E_NOTIMPL;
}

static HRESULT WINAPI ComponentFactory_CreateComponentEnumerator(IWICComponentFactory *iface,
    DWORD componentTypes, DWORD options, IEnumUnknown **ppIEnumUnknown)
{
    TRACE("(%p,%u,%u,%p)\n", iface, componentTypes, options, ppIEnumUnknown);
    return CreateComponentEnumerator(componentTypes, options, ppIEnumUnknown);
}

static HRESULT WINAPI ComponentFactory_CreateFastMetadataEncoderFromDecoder(
    IWICComponentFactory *iface, IWICBitmapDecoder *pIDecoder,
    IWICFastMetadataEncoder **ppIFastEncoder)
{
    FIXME("(%p,%p,%p): stub\n", iface, pIDecoder, ppIFastEncoder);
    return E_NOTIMPL;
}

static HRESULT WINAPI ComponentFactory_CreateFastMetadataEncoderFromFrameDecode(
    IWICComponentFactory *iface, IWICBitmapFrameDecode *pIFrameDecoder,
    IWICFastMetadataEncoder **ppIFastEncoder)
{
    FIXME("(%p,%p,%p): stub\n", iface, pIFrameDecoder, ppIFastEncoder);
    return E_NOTIMPL;
}

static HRESULT WINAPI ComponentFactory_CreateQueryWriter(IWICComponentFactory *iface,
    REFGUID guidMetadataFormat, const GUID *pguidVendor,
    IWICMetadataQueryWriter **ppIQueryWriter)
{
    FIXME("(%p,%s,%s,%p): stub\n", iface, debugstr_guid(guidMetadataFormat),
        debugstr_guid(pguidVendor), ppIQueryWriter);
    return E_NOTIMPL;
}

static HRESULT WINAPI ComponentFactory_CreateQueryWriterFromReader(IWICComponentFactory *iface,
    IWICMetadataQueryReader *pIQueryReader, const GUID *pguidVendor,
    IWICMetadataQueryWriter **ppIQueryWriter)
{
    FIXME("(%p,%p,%s,%p): stub\n", iface, pIQueryReader, debugstr_guid(pguidVendor),
        ppIQueryWriter);
    return E_NOTIMPL;
}

static HRESULT WINAPI ComponentFactory_CreateMetadataReader(IWICComponentFactory *iface,
        REFGUID format, const GUID *vendor, DWORD options, IStream *stream, IWICMetadataReader **reader)
{
    FIXME("%p,%s,%s,%x,%p,%p: stub\n", iface, debugstr_guid(format), debugstr_guid(vendor),
        options, stream, reader);
    return E_NOTIMPL;
}

static HRESULT WINAPI ComponentFactory_CreateMetadataReaderFromContainer(IWICComponentFactory *iface,
        REFGUID format, const GUID *vendor, DWORD options, IStream *stream, IWICMetadataReader **reader)
{
    FIXME("%p,%s,%s,%x,%p,%p: stub\n", iface, debugstr_guid(format), debugstr_guid(vendor),
        options, stream, reader);
    return E_NOTIMPL;
}

static HRESULT WINAPI ComponentFactory_CreateMetadataWriter(IWICComponentFactory *iface,
        REFGUID format, const GUID *vendor, DWORD options, IWICMetadataWriter **writer)
{
    FIXME("%p,%s,%s,%x,%p: stub\n", iface, debugstr_guid(format), debugstr_guid(vendor), options, writer);
    return E_NOTIMPL;
}

static HRESULT WINAPI ComponentFactory_CreateMetadataWriterFromReader(IWICComponentFactory *iface,
        IWICMetadataReader *reader, const GUID *vendor, IWICMetadataWriter **writer)
{
    FIXME("%p,%p,%s,%p: stub\n", iface, reader, debugstr_guid(vendor), writer);
    return E_NOTIMPL;
}

static HRESULT WINAPI ComponentFactory_CreateQueryReaderFromBlockReader(IWICComponentFactory *iface,
        IWICMetadataBlockReader *block_reader, IWICMetadataQueryReader **query_reader)
{
    FIXME("%p,%p,%p: stub\n", iface, block_reader, query_reader);
    return E_NOTIMPL;
}

static HRESULT WINAPI ComponentFactory_CreateQueryWriterFromBlockWriter(IWICComponentFactory *iface,
        IWICMetadataBlockWriter *block_writer, IWICMetadataQueryWriter **query_writer)
{
    FIXME("%p,%p,%p: stub\n", iface, block_writer, query_writer);
    return E_NOTIMPL;
}

static HRESULT WINAPI ComponentFactory_CreateEncoderPropertyBag(IWICComponentFactory *iface,
        PROPBAG2 *options, UINT count, IPropertyBag2 **property)
{
    FIXME("%p,%p,%u,%p: stub\n", iface, options, count, property);
    return E_NOTIMPL;
}

static const IWICComponentFactoryVtbl ComponentFactory_Vtbl = {
    ComponentFactory_QueryInterface,
    ComponentFactory_AddRef,
    ComponentFactory_Release,
    ComponentFactory_CreateDecoderFromFilename,
    ComponentFactory_CreateDecoderFromStream,
    ComponentFactory_CreateDecoderFromFileHandle,
    ComponentFactory_CreateComponentInfo,
    ComponentFactory_CreateDecoder,
    ComponentFactory_CreateEncoder,
    ComponentFactory_CreatePalette,
    ComponentFactory_CreateFormatConverter,
    ComponentFactory_CreateBitmapScaler,
    ComponentFactory_CreateBitmapClipper,
    ComponentFactory_CreateBitmapFlipRotator,
    ComponentFactory_CreateStream,
    ComponentFactory_CreateColorContext,
    ComponentFactory_CreateColorTransformer,
    ComponentFactory_CreateBitmap,
    ComponentFactory_CreateBitmapFromSource,
    ComponentFactory_CreateBitmapFromSourceRect,
    ComponentFactory_CreateBitmapFromMemory,
    ComponentFactory_CreateBitmapFromHBITMAP,
    ComponentFactory_CreateBitmapFromHICON,
    ComponentFactory_CreateComponentEnumerator,
    ComponentFactory_CreateFastMetadataEncoderFromDecoder,
    ComponentFactory_CreateFastMetadataEncoderFromFrameDecode,
    ComponentFactory_CreateQueryWriter,
    ComponentFactory_CreateQueryWriterFromReader,
    ComponentFactory_CreateMetadataReader,
    ComponentFactory_CreateMetadataReaderFromContainer,
    ComponentFactory_CreateMetadataWriter,
    ComponentFactory_CreateMetadataWriterFromReader,
    ComponentFactory_CreateQueryReaderFromBlockReader,
    ComponentFactory_CreateQueryWriterFromBlockWriter,
    ComponentFactory_CreateEncoderPropertyBag
};

HRESULT ComponentFactory_CreateInstance(IUnknown *pUnkOuter, REFIID iid, void** ppv)
{
    ComponentFactory *This;
    HRESULT ret;

    TRACE("(%p,%s,%p)\n", pUnkOuter, debugstr_guid(iid), ppv);

    *ppv = NULL;

    if (pUnkOuter) return CLASS_E_NOAGGREGATION;

    This = HeapAlloc(GetProcessHeap(), 0, sizeof(ComponentFactory));
    if (!This) return E_OUTOFMEMORY;

    This->IWICComponentFactory_iface.lpVtbl = &ComponentFactory_Vtbl;
    This->ref = 1;

    ret = IWICComponentFactory_QueryInterface(&This->IWICComponentFactory_iface, iid, ppv);
    IWICComponentFactory_Release(&This->IWICComponentFactory_iface);

    return ret;
}
