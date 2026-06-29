/*
 * Copyright 2009 Vincent Povirk for CodeWeavers
 * Copyright 2012 Dmitry Timoshkov
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

#include <assert.h>
#include <stdarg.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "objbase.h"
#include "shellapi.h"

#include "wincodecs_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(wincodecs);

typedef struct {
    IWICImagingFactory2 IWICImagingFactory2_iface;
    IWICComponentFactory IWICComponentFactory_iface;
    LONG ref;
} ImagingFactory;

static inline ImagingFactory *impl_from_IWICComponentFactory(IWICComponentFactory *iface)
{
    return CONTAINING_RECORD(iface, ImagingFactory, IWICComponentFactory_iface);
}

static inline ImagingFactory *impl_from_IWICImagingFactory2(IWICImagingFactory2 *iface)
{
    return CONTAINING_RECORD(iface, ImagingFactory, IWICImagingFactory2_iface);
}

static HRESULT WINAPI ImagingFactory_QueryInterface(IWICImagingFactory2 *iface, REFIID iid,
    void **ppv)
{
    ImagingFactory *This = impl_from_IWICImagingFactory2(iface);
    TRACE("(%p,%s,%p)\n", iface, debugstr_guid(iid), ppv);

    if (!ppv) return E_INVALIDARG;

    if (IsEqualIID(&IID_IUnknown, iid) ||
        IsEqualIID(&IID_IWICImagingFactory, iid) ||
        IsEqualIID(&IID_IWICComponentFactory, iid))
    {
        *ppv = &This->IWICComponentFactory_iface;
    }
    else if (IsEqualIID(&IID_IWICImagingFactory2, iid))
    {
        *ppv = &This->IWICImagingFactory2_iface;
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI ImagingFactory_AddRef(IWICImagingFactory2 *iface)
{
    ImagingFactory *This = impl_from_IWICImagingFactory2(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) refcount=%lu\n", iface, ref);

    return ref;
}

static ULONG WINAPI ImagingFactory_Release(IWICImagingFactory2 *iface)
{
    ImagingFactory *This = impl_from_IWICImagingFactory2(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) refcount=%lu\n", iface, ref);

    if (ref == 0)
        free(This);

    return ref;
}

static HRESULT WINAPI ImagingFactory_CreateDecoderFromFilename(
    IWICImagingFactory2 *iface, LPCWSTR wzFilename, const GUID *pguidVendor,
    DWORD dwDesiredAccess, WICDecodeOptions metadataOptions,
    IWICBitmapDecoder **ppIDecoder)
{
    IWICStream *stream;
    HRESULT hr;

    TRACE("(%p,%s,%s,%lu,%u,%p)\n", iface, debugstr_w(wzFilename),
        debugstr_guid(pguidVendor), dwDesiredAccess, metadataOptions, ppIDecoder);

    hr = StreamImpl_Create(&stream);
    if (SUCCEEDED(hr))
    {
        hr = IWICStream_InitializeFromFilename(stream, wzFilename, dwDesiredAccess);

        if (SUCCEEDED(hr))
        {
            hr = IWICImagingFactory2_CreateDecoderFromStream(iface, (IStream*)stream,
                pguidVendor, metadataOptions, ppIDecoder);
        }

        IWICStream_Release(stream);
    }

    return hr;
}

static HRESULT find_decoder(IStream *pIStream, const GUID *pguidVendor,
                            WICDecodeOptions metadataOptions, IWICBitmapDecoder **decoder)
{
    IEnumUnknown *enumdecoders = NULL;
    IUnknown *unkdecoderinfo = NULL;
    GUID vendor;
    HRESULT res, res_wine;
    ULONG num_fetched;
    BOOL matches, found;

    *decoder = NULL;

    res = CreateComponentEnumerator(WICDecoder, WICComponentEnumerateDefault, &enumdecoders);
    if (FAILED(res)) return res;

    found = FALSE;
    while (IEnumUnknown_Next(enumdecoders, 1, &unkdecoderinfo, &num_fetched) == S_OK)
    {
        IWICBitmapDecoderInfo *decoderinfo = NULL;
        IWICWineDecoder *wine_decoder = NULL;

        res = IUnknown_QueryInterface(unkdecoderinfo, &IID_IWICBitmapDecoderInfo, (void**)&decoderinfo);
        if (FAILED(res)) goto next;

        if (pguidVendor)
        {
            res = IWICBitmapDecoderInfo_GetVendorGUID(decoderinfo, &vendor);
            if (FAILED(res) || !IsEqualIID(&vendor, pguidVendor)) goto next;
        }

        res = IWICBitmapDecoderInfo_MatchesPattern(decoderinfo, pIStream, &matches);
        if (FAILED(res) || !matches) goto next;

        res = IWICBitmapDecoderInfo_CreateInstance(decoderinfo, decoder);
        if (FAILED(res)) goto next;

        /* FIXME: should use QueryCapability to choose a decoder */

        found = TRUE;
        res = IWICBitmapDecoder_Initialize(*decoder, pIStream, metadataOptions);
        if (FAILED(res))
        {
            res_wine = IWICBitmapDecoder_QueryInterface(*decoder, &IID_IWICWineDecoder, (void **)&wine_decoder);
            if (FAILED(res_wine))
            {
                IWICBitmapDecoder_Release(*decoder);
                *decoder = NULL;
                goto next;
            }

            res_wine = IWICWineDecoder_Initialize(wine_decoder, pIStream, metadataOptions);
            if (FAILED(res_wine))
            {
                IWICBitmapDecoder_Release(*decoder);
                *decoder = NULL;
                goto next;
            }

            res = res_wine;
        }

    next:
        if (wine_decoder) IWICWineDecoder_Release(wine_decoder);
        if (decoderinfo) IWICBitmapDecoderInfo_Release(decoderinfo);
        IUnknown_Release(unkdecoderinfo);
        if (found) break;
    }

    IEnumUnknown_Release(enumdecoders);
    if (!found) res = WINCODEC_ERR_COMPONENTNOTFOUND;
    return res;
}

static HRESULT WINAPI ImagingFactory_CreateDecoderFromStream(
    IWICImagingFactory2 *iface, IStream *pIStream, const GUID *pguidVendor,
    WICDecodeOptions metadataOptions, IWICBitmapDecoder **ppIDecoder)
{
    HRESULT res;
    IWICBitmapDecoder *decoder = NULL;

    TRACE("(%p,%p,%s,%u,%p)\n", iface, pIStream, debugstr_guid(pguidVendor),
        metadataOptions, ppIDecoder);

    if (pguidVendor)
        res = find_decoder(pIStream, pguidVendor, metadataOptions, &decoder);
    if (!decoder)
        res = find_decoder(pIStream, NULL, metadataOptions, &decoder);

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

            WARN("failed to load from a stream %#lx\n", res);

            seek.QuadPart = 0;
            if (IStream_Seek(pIStream, seek, STREAM_SEEK_SET, NULL) == S_OK)
            {
                if (IStream_Read(pIStream, data, 4, &bytesread) == S_OK)
                    WARN("first %li bytes of stream=%x %x %x %x\n", bytesread, data[0], data[1], data[2], data[3]);
            }
        }
        *ppIDecoder = NULL;
        return res;
    }
}

static HRESULT WINAPI ImagingFactory_CreateDecoderFromFileHandle(
    IWICImagingFactory2 *iface, ULONG_PTR hFile, const GUID *pguidVendor,
    WICDecodeOptions metadataOptions, IWICBitmapDecoder **ppIDecoder)
{
    IWICStream *stream;
    HRESULT hr;

    TRACE("(%p,%Ix,%s,%u,%p)\n", iface, hFile, debugstr_guid(pguidVendor),
        metadataOptions, ppIDecoder);

    hr = StreamImpl_Create(&stream);
    if (SUCCEEDED(hr))
    {
        hr = stream_initialize_from_filehandle(stream, (HANDLE)hFile);
        if (SUCCEEDED(hr))
        {
            hr = IWICImagingFactory2_CreateDecoderFromStream(iface, (IStream*)stream,
                pguidVendor, metadataOptions, ppIDecoder);
        }
        IWICStream_Release(stream);
    }
    return hr;
}

static HRESULT WINAPI ImagingFactory_CreateComponentInfo(IWICImagingFactory2 *iface,
    REFCLSID clsidComponent, IWICComponentInfo **ppIInfo)
{
    TRACE("(%p,%s,%p)\n", iface, debugstr_guid(clsidComponent), ppIInfo);
    return CreateComponentInfo(clsidComponent, ppIInfo);
}

static HRESULT WINAPI ImagingFactory_CreateDecoder(IWICImagingFactory2 *iface,
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

static HRESULT WINAPI ImagingFactory_CreateEncoder(IWICImagingFactory2 *iface,
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

static HRESULT WINAPI ImagingFactory_CreatePalette(IWICImagingFactory2 *iface,
    IWICPalette **ppIPalette)
{
    TRACE("(%p,%p)\n", iface, ppIPalette);
    return PaletteImpl_Create(ppIPalette);
}

static HRESULT WINAPI ImagingFactory_CreateFormatConverter(IWICImagingFactory2 *iface,
    IWICFormatConverter **ppIFormatConverter)
{
    return FormatConverter_CreateInstance(&IID_IWICFormatConverter, (void**)ppIFormatConverter);
}

static HRESULT WINAPI ImagingFactory_CreateBitmapScaler(IWICImagingFactory2 *iface,
    IWICBitmapScaler **ppIBitmapScaler)
{
    TRACE("(%p,%p)\n", iface, ppIBitmapScaler);

    return BitmapScaler_Create(ppIBitmapScaler);
}

static HRESULT WINAPI ImagingFactory_CreateBitmapClipper(IWICImagingFactory2 *iface,
    IWICBitmapClipper **ppIBitmapClipper)
{
    TRACE("(%p,%p)\n", iface, ppIBitmapClipper);
    return BitmapClipper_Create(ppIBitmapClipper);
}

static HRESULT WINAPI ImagingFactory_CreateBitmapFlipRotator(IWICImagingFactory2 *iface,
    IWICBitmapFlipRotator **ppIBitmapFlipRotator)
{
    TRACE("(%p,%p)\n", iface, ppIBitmapFlipRotator);
    return FlipRotator_Create(ppIBitmapFlipRotator);
}

static HRESULT WINAPI ImagingFactory_CreateStream(IWICImagingFactory2 *iface,
    IWICStream **ppIWICStream)
{
    TRACE("(%p,%p)\n", iface, ppIWICStream);
    return StreamImpl_Create(ppIWICStream);
}

static HRESULT WINAPI ImagingFactory_CreateColorContext(IWICImagingFactory2 *iface,
    IWICColorContext **ppIColorContext)
{
    TRACE("(%p,%p)\n", iface, ppIColorContext);
    return ColorContext_Create(ppIColorContext);
}

static HRESULT WINAPI ImagingFactory_CreateColorTransformer(IWICImagingFactory2 *iface,
    IWICColorTransform **ppIColorTransform)
{
    TRACE("(%p,%p)\n", iface, ppIColorTransform);
    return ColorTransform_Create(ppIColorTransform);
}

static HRESULT WINAPI ImagingFactory_CreateBitmap(IWICImagingFactory2 *iface,
    UINT uiWidth, UINT uiHeight, REFWICPixelFormatGUID pixelFormat,
    WICBitmapCreateCacheOption option, IWICBitmap **ppIBitmap)
{
    TRACE("(%p,%u,%u,%s,%u,%p)\n", iface, uiWidth, uiHeight,
        debugstr_guid(pixelFormat), option, ppIBitmap);
    return BitmapImpl_Create(uiWidth, uiHeight, 0, 0, NULL, 0, pixelFormat, option, ppIBitmap);
}

static HRESULT create_bitmap_from_source_rect(IWICBitmapSource *piBitmapSource, const WICRect *rect,
    WICBitmapCreateCacheOption option, IWICBitmap **ppIBitmap)
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

    assert(!rect || option == WICBitmapCacheOnLoad);

    if (!piBitmapSource || !ppIBitmap)
        return E_INVALIDARG;

    if (option == WICBitmapNoCache && SUCCEEDED(IWICBitmapSource_QueryInterface(piBitmapSource,
            &IID_IWICBitmap, (void **)&result)))
    {
        *ppIBitmap = result;
        return S_OK;
    }

    hr = IWICBitmapSource_GetSize(piBitmapSource, &width, &height);

    if (SUCCEEDED(hr) && rect)
    {
        if (rect->X >= width || rect->Y >= height || rect->Width == 0 || rect->Height == 0)
            return E_INVALIDARG;

        width = min(width - rect->X, rect->Width);
        height = min(height - rect->Y, rect->Height);
    }

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
        hr = BitmapImpl_Create(width, height, 0, 0, NULL, 0, &pixelformat, option, &result);

    if (SUCCEEDED(hr))
    {
        hr = IWICBitmap_Lock(result, NULL, WICBitmapLockWrite, &lock);
        if (SUCCEEDED(hr))
        {
            UINT stride, buffersize;
            BYTE *buffer;

            if (rect)
            {
                rc.X = rect->X;
                rc.Y = rect->Y;
            }
            else
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

        if (SUCCEEDED(hr) && (format_type == WICPixelFormatNumericRepresentationUnspecified ||
                              format_type == WICPixelFormatNumericRepresentationIndexed))
        {
            hr = PaletteImpl_Create(&palette);

            if (SUCCEEDED(hr))
            {
                hr = IWICBitmapSource_CopyPalette(piBitmapSource, palette);

                if (SUCCEEDED(hr))
                    hr = IWICBitmap_SetPalette(result, palette);
                else
                    hr = S_OK;

                IWICPalette_Release(palette);
            }
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

static HRESULT WINAPI ImagingFactory_CreateBitmapFromSource(IWICImagingFactory2 *iface,
    IWICBitmapSource *piBitmapSource, WICBitmapCreateCacheOption option,
    IWICBitmap **ppIBitmap)
{
    TRACE("(%p,%p,%u,%p)\n", iface, piBitmapSource, option, ppIBitmap);

    return create_bitmap_from_source_rect(piBitmapSource, NULL, option, ppIBitmap);
}

static HRESULT WINAPI ImagingFactory_CreateBitmapFromSourceRect(IWICImagingFactory2 *iface,
    IWICBitmapSource *piBitmapSource, UINT x, UINT y, UINT width, UINT height,
    IWICBitmap **ppIBitmap)
{
    WICRect rect;

    TRACE("(%p,%p,%u,%u,%u,%u,%p)\n", iface, piBitmapSource, x, y, width,
        height, ppIBitmap);

    rect.X = x;
    rect.Y = y;
    rect.Width = width;
    rect.Height = height;

    return create_bitmap_from_source_rect(piBitmapSource, &rect, WICBitmapCacheOnLoad, ppIBitmap);
}

static HRESULT WINAPI ImagingFactory_CreateBitmapFromMemory(IWICImagingFactory2 *iface,
    UINT width, UINT height, REFWICPixelFormatGUID format, UINT stride,
    UINT size, BYTE *buffer, IWICBitmap **bitmap)
{
    HRESULT hr;

    TRACE("(%p,%u,%u,%s,%u,%u,%p,%p\n", iface, width, height,
        debugstr_guid(format), stride, size, buffer, bitmap);

    if (!stride || !size || !buffer || !bitmap) return E_INVALIDARG;

    hr = BitmapImpl_Create(width, height, stride, size, NULL, 0, format, WICBitmapCacheOnLoad, bitmap);
    if (SUCCEEDED(hr))
    {
        IWICBitmapLock *lock;

        hr = IWICBitmap_Lock(*bitmap, NULL, WICBitmapLockWrite, &lock);
        if (SUCCEEDED(hr))
        {
            UINT buffersize;
            BYTE *data;

            IWICBitmapLock_GetDataPointer(lock, &buffersize, &data);
            memcpy(data, buffer, buffersize);

            IWICBitmapLock_Release(lock);
        }
        else
        {
            IWICBitmap_Release(*bitmap);
            *bitmap = NULL;
        }
    }
    return hr;
}

static BOOL get_16bpp_format(HBITMAP hbm, WICPixelFormatGUID *format)
{
    BOOL ret = TRUE;
    BITMAPV4HEADER bmh;
    HDC hdc;

    hdc = CreateCompatibleDC(0);

    memset(&bmh, 0, sizeof(bmh));
    bmh.bV4Size = sizeof(bmh);
    bmh.bV4Width = 1;
    bmh.bV4Height = 1;
    bmh.bV4V4Compression = BI_BITFIELDS;
    bmh.bV4BitCount = 16;

    GetDIBits(hdc, hbm, 0, 0, NULL, (BITMAPINFO *)&bmh, DIB_RGB_COLORS);

    if (bmh.bV4RedMask == 0x7c00 &&
        bmh.bV4GreenMask == 0x3e0 &&
        bmh.bV4BlueMask == 0x1f)
    {
        *format = GUID_WICPixelFormat16bppBGR555;
    }
    else if (bmh.bV4RedMask == 0xf800 &&
        bmh.bV4GreenMask == 0x7e0 &&
        bmh.bV4BlueMask == 0x1f)
    {
        *format = GUID_WICPixelFormat16bppBGR565;
    }
    else
    {
        FIXME("unrecognized bitfields %lx,%lx,%lx\n", bmh.bV4RedMask,
            bmh.bV4GreenMask, bmh.bV4BlueMask);
        ret = FALSE;
    }

    DeleteDC(hdc);
    return ret;
}

static HRESULT WINAPI ImagingFactory_CreateBitmapFromHBITMAP(IWICImagingFactory2 *iface,
    HBITMAP hbm, HPALETTE hpal, WICBitmapAlphaChannelOption option, IWICBitmap **bitmap)
{
    BITMAP bm;
    HRESULT hr;
    WICPixelFormatGUID format;
    IWICBitmapLock *lock;
    UINT size, num_palette_entries = 0;
    PALETTEENTRY entry[256];

    TRACE("(%p,%p,%p,%u,%p)\n", iface, hbm, hpal, option, bitmap);

    if (!bitmap) return E_INVALIDARG;

    if (GetObjectW(hbm, sizeof(bm), &bm) != sizeof(bm))
        return WINCODEC_ERR_WIN32ERROR;

    if (hpal)
    {
        num_palette_entries = GetPaletteEntries(hpal, 0, 256, entry);
        if (!num_palette_entries)
            return WINCODEC_ERR_WIN32ERROR;
    }

    /* TODO: Figure out the correct format for 16, 32, 64 bpp */
    switch(bm.bmBitsPixel)
    {
    case 1:
        format = GUID_WICPixelFormat1bppIndexed;
        break;
    case 4:
        format = GUID_WICPixelFormat4bppIndexed;
        break;
    case 8:
        format = GUID_WICPixelFormat8bppIndexed;
        break;
    case 16:
        if (!get_16bpp_format(hbm, &format))
            return E_INVALIDARG;
        break;
    case 24:
        format = GUID_WICPixelFormat24bppBGR;
        break;
    case 32:
        switch (option)
        {
        case WICBitmapUseAlpha:
            format = GUID_WICPixelFormat32bppBGRA;
            break;
        case WICBitmapUsePremultipliedAlpha:
            format = GUID_WICPixelFormat32bppPBGRA;
            break;
        case WICBitmapIgnoreAlpha:
            format = GUID_WICPixelFormat32bppBGR;
            break;
        default:
            return E_INVALIDARG;
        }
        break;
    case 48:
        format = GUID_WICPixelFormat48bppRGB;
        break;
    default:
        FIXME("unsupported %d bpp\n", bm.bmBitsPixel);
        return E_INVALIDARG;
    }

    hr = BitmapImpl_Create(bm.bmWidth, bm.bmHeight, bm.bmWidthBytes, 0, NULL, 0, &format,
                           WICBitmapCacheOnLoad, bitmap);
    if (hr != S_OK) return hr;

    hr = IWICBitmap_Lock(*bitmap, NULL, WICBitmapLockWrite, &lock);
    if (hr == S_OK)
    {
        BYTE *buffer;
        HDC hdc;
#ifdef __REACTOS__
        char bmibuf[FIELD_OFFSET(BITMAPINFO, bmiColors) + 256 * sizeof(RGBQUAD)];
#else
        char bmibuf[FIELD_OFFSET(BITMAPINFO, bmiColors[256])];
#endif
        BITMAPINFO *bmi = (BITMAPINFO *)bmibuf;

        IWICBitmapLock_GetDataPointer(lock, &size, &buffer);

        hdc = CreateCompatibleDC(0);

        bmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bmi->bmiHeader.biBitCount = 0;
        GetDIBits(hdc, hbm, 0, 0, NULL, bmi, DIB_RGB_COLORS);
        bmi->bmiHeader.biHeight = -bm.bmHeight;
        GetDIBits(hdc, hbm, 0, bm.bmHeight, buffer, bmi, DIB_RGB_COLORS);

        DeleteDC(hdc);
        IWICBitmapLock_Release(lock);

        if (num_palette_entries)
        {
            IWICPalette *palette;
            WICColor colors[256];
            UINT i;

            hr = PaletteImpl_Create(&palette);
            if (hr == S_OK)
            {
                for (i = 0; i < num_palette_entries; i++)
                    colors[i] = 0xff000000 | entry[i].peRed << 16 |
                                entry[i].peGreen << 8 | entry[i].peBlue;

                hr = IWICPalette_InitializeCustom(palette, colors, num_palette_entries);
                if (hr == S_OK)
                    hr = IWICBitmap_SetPalette(*bitmap, palette);

                IWICPalette_Release(palette);
            }
        }
    }

    if (hr != S_OK)
    {
        IWICBitmap_Release(*bitmap);
        *bitmap = NULL;
    }

    return hr;
}

static HRESULT WINAPI ImagingFactory_CreateBitmapFromHICON(IWICImagingFactory2 *iface,
    HICON hicon, IWICBitmap **bitmap)
{
    IWICBitmapLock *lock;
    ICONINFO info;
    BITMAP bm;
    int width, height, x, y;
    UINT stride, size;
    BYTE *buffer;
    DWORD *bits;
    BITMAPINFO bi;
    HDC hdc;
    BOOL has_alpha;
    HRESULT hr;

    TRACE("(%p,%p,%p)\n", iface, hicon, bitmap);

    if (!bitmap) return E_INVALIDARG;

    if (!GetIconInfo(hicon, &info))
        return HRESULT_FROM_WIN32(GetLastError());

    GetObjectW(info.hbmColor ? info.hbmColor : info.hbmMask, sizeof(bm), &bm);

    width = bm.bmWidth;
    height = info.hbmColor ? abs(bm.bmHeight) : abs(bm.bmHeight) / 2;
    stride = width * 4;
    size = stride * height;

    hr = BitmapImpl_Create(width, height, stride, size, NULL, 0,
                           &GUID_WICPixelFormat32bppBGRA, WICBitmapCacheOnLoad, bitmap);
    if (hr != S_OK) goto failed;

    hr = IWICBitmap_Lock(*bitmap, NULL, WICBitmapLockWrite, &lock);
    if (hr != S_OK)
    {
        IWICBitmap_Release(*bitmap);
        goto failed;
    }
    IWICBitmapLock_GetDataPointer(lock, &size, &buffer);

    hdc = CreateCompatibleDC(0);

    memset(&bi, 0, sizeof(bi));
    bi.bmiHeader.biSize = sizeof(bi.bmiHeader);
    bi.bmiHeader.biWidth = width;
    bi.bmiHeader.biHeight = info.hbmColor ? -height: -height * 2;
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 32;
    bi.bmiHeader.biCompression = BI_RGB;

    has_alpha = FALSE;

    if (info.hbmColor)
    {
        GetDIBits(hdc, info.hbmColor, 0, height, buffer, &bi, DIB_RGB_COLORS);

        if (bm.bmBitsPixel == 32)
        {
            /* If any pixel has a non-zero alpha, ignore hbmMask */
            DWORD *ptr = (DWORD *)buffer;
            DWORD *end = ptr + width * height;
            while (ptr != end)
            {
                if (*ptr++ & 0xff000000)
                {
                    has_alpha = TRUE;
                    break;
                }
            }
        }
    }
    else
        GetDIBits(hdc, info.hbmMask, 0, height, buffer, &bi, DIB_RGB_COLORS);

    if (!has_alpha)
    {
        DWORD *rgba;

        if (info.hbmMask)
        {
            BYTE *mask;

            mask = malloc(size);
            if (!mask)
            {
                IWICBitmapLock_Release(lock);
                IWICBitmap_Release(*bitmap);
                DeleteDC(hdc);
                hr = E_OUTOFMEMORY;
                goto failed;
            }

            /* read alpha data from the mask */
            GetDIBits(hdc, info.hbmMask, info.hbmColor ? 0 : height, height, mask, &bi, DIB_RGB_COLORS);

            for (y = 0; y < height; y++)
            {
                rgba = (DWORD *)(buffer + y * stride);
                bits = (DWORD *)(mask + y * stride);

                for (x = 0; x < width; x++, rgba++, bits++)
                {
                    if (*bits)
                        *rgba = 0;
                    else
                        *rgba |= 0xff000000;
                }
            }

            free(mask);
        }
        else
        {
            /* set constant alpha of 255 */
            for (y = 0; y < height; y++)
            {
                rgba = (DWORD *)(buffer + y * stride);
                for (x = 0; x < width; x++, rgba++)
                    *rgba |= 0xff000000;
            }
        }

    }

    IWICBitmapLock_Release(lock);
    DeleteDC(hdc);

failed:
    DeleteObject(info.hbmColor);
    DeleteObject(info.hbmMask);

    return hr;
}

static HRESULT WINAPI ImagingFactory_CreateComponentEnumerator(IWICImagingFactory2 *iface,
    DWORD componentTypes, DWORD options, IEnumUnknown **ppIEnumUnknown)
{
    TRACE("(%p,%lu,%lu,%p)\n", iface, componentTypes, options, ppIEnumUnknown);
    return CreateComponentEnumerator(componentTypes, options, ppIEnumUnknown);
}

static HRESULT WINAPI ImagingFactory_CreateFastMetadataEncoderFromDecoder(
    IWICImagingFactory2 *iface, IWICBitmapDecoder *pIDecoder,
    IWICFastMetadataEncoder **ppIFastEncoder)
{
    FIXME("(%p,%p,%p): stub\n", iface, pIDecoder, ppIFastEncoder);
    return E_NOTIMPL;
}

static HRESULT WINAPI ImagingFactory_CreateFastMetadataEncoderFromFrameDecode(
    IWICImagingFactory2 *iface, IWICBitmapFrameDecode *pIFrameDecoder,
    IWICFastMetadataEncoder **ppIFastEncoder)
{
    FIXME("(%p,%p,%p): stub\n", iface, pIFrameDecoder, ppIFastEncoder);
    return E_NOTIMPL;
}

static HRESULT WINAPI ImagingFactory_CreateQueryWriter(IWICImagingFactory2 *iface,
    REFGUID guidMetadataFormat, const GUID *pguidVendor,
    IWICMetadataQueryWriter **ppIQueryWriter)
{
    FIXME("(%p,%s,%s,%p): stub\n", iface, debugstr_guid(guidMetadataFormat),
        debugstr_guid(pguidVendor), ppIQueryWriter);
    return E_NOTIMPL;
}

static HRESULT WINAPI ImagingFactory_CreateQueryWriterFromReader(IWICImagingFactory2 *iface,
    IWICMetadataQueryReader *pIQueryReader, const GUID *pguidVendor,
    IWICMetadataQueryWriter **ppIQueryWriter)
{
    FIXME("(%p,%p,%s,%p): stub\n", iface, pIQueryReader, debugstr_guid(pguidVendor),
        ppIQueryWriter);
    return E_NOTIMPL;
}

#ifndef __REACTOS__
static HRESULT WINAPI ImagingFactory_CreateImageEncoder(IWICImagingFactory2 *iface, ID2D1Device *device, IWICImageEncoder **encoder)
{
    FIXME("%p,%p,%p stub.\n", iface, device, encoder);
    return E_NOTIMPL;
}
#endif

static const IWICImagingFactory2Vtbl ImagingFactory_Vtbl = {
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
    ImagingFactory_CreateQueryWriterFromReader,
#ifndef __REACTOS__
    ImagingFactory_CreateImageEncoder,
#endif
};

static HRESULT WINAPI ComponentFactory_QueryInterface(IWICComponentFactory *iface, REFIID iid, void **ppv)
{
    ImagingFactory *This = impl_from_IWICComponentFactory(iface);
    return IWICImagingFactory2_QueryInterface(&This->IWICImagingFactory2_iface, iid, ppv);
}

static ULONG WINAPI ComponentFactory_AddRef(IWICComponentFactory *iface)
{
    ImagingFactory *This = impl_from_IWICComponentFactory(iface);
    return IWICImagingFactory2_AddRef(&This->IWICImagingFactory2_iface);
}

static ULONG WINAPI ComponentFactory_Release(IWICComponentFactory *iface)
{
    ImagingFactory *This = impl_from_IWICComponentFactory(iface);
    return IWICImagingFactory2_Release(&This->IWICImagingFactory2_iface);
}

static HRESULT WINAPI ComponentFactory_CreateDecoderFromFilename(IWICComponentFactory *iface, LPCWSTR filename,
    const GUID *vendor, DWORD desired_access, WICDecodeOptions options, IWICBitmapDecoder **decoder)
{
    ImagingFactory *This = impl_from_IWICComponentFactory(iface);
    return IWICImagingFactory2_CreateDecoderFromFilename(&This->IWICImagingFactory2_iface, filename, vendor,
        desired_access, options, decoder);
}

static HRESULT WINAPI ComponentFactory_CreateDecoderFromStream(IWICComponentFactory *iface, IStream *stream,
    const GUID *vendor, WICDecodeOptions options, IWICBitmapDecoder **decoder)
{
    ImagingFactory *This = impl_from_IWICComponentFactory(iface);
    return IWICImagingFactory2_CreateDecoderFromStream(&This->IWICImagingFactory2_iface, stream, vendor,
        options, decoder);
}

static HRESULT WINAPI ComponentFactory_CreateDecoderFromFileHandle(IWICComponentFactory *iface, ULONG_PTR hFile,
    const GUID *vendor, WICDecodeOptions options, IWICBitmapDecoder **decoder)
{
    ImagingFactory *This = impl_from_IWICComponentFactory(iface);
    return IWICImagingFactory2_CreateDecoderFromFileHandle(&This->IWICImagingFactory2_iface, hFile, vendor,
        options, decoder);
}

static HRESULT WINAPI ComponentFactory_CreateComponentInfo(IWICComponentFactory *iface, REFCLSID component,
    IWICComponentInfo **info)
{
    ImagingFactory *This = impl_from_IWICComponentFactory(iface);
    return IWICImagingFactory2_CreateComponentInfo(&This->IWICImagingFactory2_iface, component, info);
}

static HRESULT WINAPI ComponentFactory_CreateDecoder(IWICComponentFactory *iface, REFGUID format, const GUID *vendor,
    IWICBitmapDecoder **decoder)
{
    ImagingFactory *This = impl_from_IWICComponentFactory(iface);
    return IWICImagingFactory2_CreateDecoder(&This->IWICImagingFactory2_iface, format, vendor, decoder);
}

static HRESULT WINAPI ComponentFactory_CreateEncoder(IWICComponentFactory *iface, REFGUID format, const GUID *vendor,
    IWICBitmapEncoder **encoder)
{
    ImagingFactory *This = impl_from_IWICComponentFactory(iface);
    return IWICImagingFactory2_CreateEncoder(&This->IWICImagingFactory2_iface, format, vendor, encoder);
}

static HRESULT WINAPI ComponentFactory_CreatePalette(IWICComponentFactory *iface, IWICPalette **palette)
{
    ImagingFactory *This = impl_from_IWICComponentFactory(iface);
    return IWICImagingFactory2_CreatePalette(&This->IWICImagingFactory2_iface, palette);
}

static HRESULT WINAPI ComponentFactory_CreateFormatConverter(IWICComponentFactory *iface, IWICFormatConverter **converter)
{
    ImagingFactory *This = impl_from_IWICComponentFactory(iface);
    return IWICImagingFactory2_CreateFormatConverter(&This->IWICImagingFactory2_iface, converter);
}

static HRESULT WINAPI ComponentFactory_CreateBitmapScaler(IWICComponentFactory *iface, IWICBitmapScaler **scaler)
{
    ImagingFactory *This = impl_from_IWICComponentFactory(iface);
    return IWICImagingFactory2_CreateBitmapScaler(&This->IWICImagingFactory2_iface, scaler);
}

static HRESULT WINAPI ComponentFactory_CreateBitmapClipper(IWICComponentFactory *iface, IWICBitmapClipper **clipper)
{
    ImagingFactory *This = impl_from_IWICComponentFactory(iface);
    return IWICImagingFactory2_CreateBitmapClipper(&This->IWICImagingFactory2_iface, clipper);
}

static HRESULT WINAPI ComponentFactory_CreateBitmapFlipRotator(IWICComponentFactory *iface, IWICBitmapFlipRotator **fliprotator)
{
    ImagingFactory *This = impl_from_IWICComponentFactory(iface);
    return IWICImagingFactory2_CreateBitmapFlipRotator(&This->IWICImagingFactory2_iface, fliprotator);
}

static HRESULT WINAPI ComponentFactory_CreateStream(IWICComponentFactory *iface, IWICStream **stream)
{
    ImagingFactory *This = impl_from_IWICComponentFactory(iface);
    return IWICImagingFactory2_CreateStream(&This->IWICImagingFactory2_iface, stream);
}

static HRESULT WINAPI ComponentFactory_CreateColorContext(IWICComponentFactory *iface, IWICColorContext **context)
{
    ImagingFactory *This = impl_from_IWICComponentFactory(iface);
    return IWICImagingFactory2_CreateColorContext(&This->IWICImagingFactory2_iface, context);
}

static HRESULT WINAPI ComponentFactory_CreateColorTransformer(IWICComponentFactory *iface, IWICColorTransform **transformer)
{
    ImagingFactory *This = impl_from_IWICComponentFactory(iface);
    return IWICImagingFactory2_CreateColorTransformer(&This->IWICImagingFactory2_iface, transformer);
}

static HRESULT WINAPI ComponentFactory_CreateBitmap(IWICComponentFactory *iface, UINT width, UINT height, REFWICPixelFormatGUID pixel_format,
    WICBitmapCreateCacheOption option, IWICBitmap **bitmap)
{
    ImagingFactory *This = impl_from_IWICComponentFactory(iface);
    return IWICImagingFactory2_CreateBitmap(&This->IWICImagingFactory2_iface, width, height, pixel_format, option, bitmap);
}

static HRESULT WINAPI ComponentFactory_CreateBitmapFromSource(IWICComponentFactory *iface, IWICBitmapSource *source,
    WICBitmapCreateCacheOption option, IWICBitmap **bitmap)
{
    ImagingFactory *This = impl_from_IWICComponentFactory(iface);
    return IWICImagingFactory2_CreateBitmapFromSource(&This->IWICImagingFactory2_iface, source, option, bitmap);
}

static HRESULT WINAPI ComponentFactory_CreateBitmapFromSourceRect(IWICComponentFactory *iface, IWICBitmapSource *source,
    UINT x, UINT y, UINT width, UINT height, IWICBitmap **bitmap)
{
    ImagingFactory *This = impl_from_IWICComponentFactory(iface);
    return IWICImagingFactory2_CreateBitmapFromSourceRect(&This->IWICImagingFactory2_iface, source, x, y, width, height, bitmap);
}

static HRESULT WINAPI ComponentFactory_CreateBitmapFromMemory(IWICComponentFactory *iface, UINT width, UINT height,
    REFWICPixelFormatGUID format, UINT stride, UINT size, BYTE *buffer, IWICBitmap **bitmap)
{
    ImagingFactory *This = impl_from_IWICComponentFactory(iface);
    return IWICImagingFactory2_CreateBitmapFromMemory(&This->IWICImagingFactory2_iface, width, height, format, stride,
        size, buffer, bitmap);
}

static HRESULT WINAPI ComponentFactory_CreateBitmapFromHBITMAP(IWICComponentFactory *iface, HBITMAP hbm, HPALETTE hpal,
    WICBitmapAlphaChannelOption option, IWICBitmap **bitmap)
{
    ImagingFactory *This = impl_from_IWICComponentFactory(iface);
    return IWICImagingFactory2_CreateBitmapFromHBITMAP(&This->IWICImagingFactory2_iface, hbm, hpal, option, bitmap);
}

static HRESULT WINAPI ComponentFactory_CreateBitmapFromHICON(IWICComponentFactory *iface, HICON hicon, IWICBitmap **bitmap)
{
    ImagingFactory *This = impl_from_IWICComponentFactory(iface);
    return IWICImagingFactory2_CreateBitmapFromHICON(&This->IWICImagingFactory2_iface, hicon, bitmap);
}

static HRESULT WINAPI ComponentFactory_CreateComponentEnumerator(IWICComponentFactory *iface, DWORD component_types,
    DWORD options, IEnumUnknown **enumerator)
{
    ImagingFactory *This = impl_from_IWICComponentFactory(iface);
    return IWICImagingFactory2_CreateComponentEnumerator(&This->IWICImagingFactory2_iface, component_types,
        options, enumerator);
}

static HRESULT WINAPI ComponentFactory_CreateFastMetadataEncoderFromDecoder(IWICComponentFactory *iface, IWICBitmapDecoder *decoder,
    IWICFastMetadataEncoder **encoder)
{
    ImagingFactory *This = impl_from_IWICComponentFactory(iface);
    return IWICImagingFactory2_CreateFastMetadataEncoderFromDecoder(&This->IWICImagingFactory2_iface, decoder, encoder);
}

static HRESULT WINAPI ComponentFactory_CreateFastMetadataEncoderFromFrameDecode(IWICComponentFactory *iface,
    IWICBitmapFrameDecode *frame_decode, IWICFastMetadataEncoder **encoder)
{
    ImagingFactory *This = impl_from_IWICComponentFactory(iface);
    return IWICImagingFactory2_CreateFastMetadataEncoderFromFrameDecode(&This->IWICImagingFactory2_iface, frame_decode, encoder);
}

static HRESULT WINAPI ComponentFactory_CreateQueryWriter(IWICComponentFactory *iface, REFGUID format, const GUID *vendor,
    IWICMetadataQueryWriter **writer)
{
    ImagingFactory *This = impl_from_IWICComponentFactory(iface);
    return IWICImagingFactory2_CreateQueryWriter(&This->IWICImagingFactory2_iface, format, vendor, writer);
}

static HRESULT WINAPI ComponentFactory_CreateQueryWriterFromReader(IWICComponentFactory *iface, IWICMetadataQueryReader *reader,
    const GUID *vendor, IWICMetadataQueryWriter **writer)
{
    ImagingFactory *This = impl_from_IWICComponentFactory(iface);
    return IWICImagingFactory2_CreateQueryWriterFromReader(&This->IWICImagingFactory2_iface, reader, vendor, writer);
}

enum iterator_result
{
    ITER_SKIP,
    ITER_DONE,
};

struct iterator_context
{
    const GUID *format;
    const GUID *vendor;
    DWORD options;
    IStream *stream;

    void **result;
};

typedef enum iterator_result (*iterator_func)(IUnknown *item, struct iterator_context *context);

static enum iterator_result create_metadata_reader_from_container_iterator(IUnknown *item,
        struct iterator_context *context)
{
    IWICPersistStream *persist_stream = NULL;
    IWICMetadataReaderInfo *readerinfo;
    IWICMetadataReader *reader = NULL;
    LARGE_INTEGER zero = {{0}};
    BOOL matches;
    HRESULT hr;
    GUID guid;

    if (FAILED(IUnknown_QueryInterface(item, &IID_IWICMetadataReaderInfo, (void **)&readerinfo)))
        return ITER_SKIP;

    if (context->vendor)
    {
        hr = IWICMetadataReaderInfo_GetVendorGUID(readerinfo, &guid);

        if (FAILED(hr) || !IsEqualIID(context->vendor, &guid))
        {
            IWICMetadataReaderInfo_Release(readerinfo);
            return ITER_SKIP;
        }
    }

    hr = IWICMetadataReaderInfo_MatchesPattern(readerinfo, context->format, context->stream, &matches);

    if (SUCCEEDED(hr) && matches)
    {
        hr = IStream_Seek(context->stream, zero, STREAM_SEEK_SET, NULL);

        if (SUCCEEDED(hr))
            hr = IWICMetadataReaderInfo_CreateInstance(readerinfo, &reader);

        if (SUCCEEDED(hr))
            hr = IWICMetadataReader_QueryInterface(reader, &IID_IWICPersistStream, (void **)&persist_stream);

        if (SUCCEEDED(hr))
            hr = IWICPersistStream_LoadEx(persist_stream, context->stream, context->vendor,
                    context->options & WICPersistOptionMask);

        if (persist_stream)
            IWICPersistStream_Release(persist_stream);

        if (SUCCEEDED(hr))
        {
            *context->result = reader;
            IWICMetadataReaderInfo_Release(readerinfo);
            return ITER_DONE;
        }

        if (reader)
            IWICMetadataReader_Release(reader);
    }

    IWICMetadataReaderInfo_Release(readerinfo);

    return ITER_SKIP;
}

static enum iterator_result create_metadata_reader_iterator(IUnknown *item,
        struct iterator_context *context)
{
    IWICPersistStream *persist_stream = NULL;
    IWICMetadataReaderInfo *readerinfo;
    IWICMetadataReader *reader = NULL;
    LARGE_INTEGER zero = {{0}};
    HRESULT hr;
    GUID guid;

    if (FAILED(IUnknown_QueryInterface(item, &IID_IWICMetadataReaderInfo, (void **)&readerinfo)))
        return ITER_SKIP;

    if (context->vendor)
    {
        hr = IWICMetadataReaderInfo_GetVendorGUID(readerinfo, &guid);

        if (FAILED(hr) || !IsEqualIID(context->vendor, &guid))
        {
            IWICMetadataReaderInfo_Release(readerinfo);
            return ITER_SKIP;
        }
    }

    hr = IWICMetadataReaderInfo_GetMetadataFormat(readerinfo, &guid);

    if (FAILED(hr) || !IsEqualIID(context->format, &guid))
    {
        IWICMetadataReaderInfo_Release(readerinfo);
        return ITER_SKIP;
    }

    if (SUCCEEDED(hr))
        hr = IWICMetadataReaderInfo_CreateInstance(readerinfo, &reader);

    IWICMetadataReaderInfo_Release(readerinfo);

    if (context->stream)
    {
        if (SUCCEEDED(hr))
            hr = IStream_Seek(context->stream, zero, STREAM_SEEK_SET, NULL);

        if (SUCCEEDED(hr))
            hr = IWICMetadataReader_QueryInterface(reader, &IID_IWICPersistStream, (void **)&persist_stream);

        if (SUCCEEDED(hr))
            hr = IWICPersistStream_LoadEx(persist_stream, context->stream, context->vendor,
                    context->options & WICPersistOptionMask);

        if (persist_stream)
            IWICPersistStream_Release(persist_stream);
    }

    if (SUCCEEDED(hr))
    {
        *context->result = reader;
        return ITER_DONE;
    }

    if (reader)
        IWICMetadataReader_Release(reader);

    return ITER_SKIP;
}

static HRESULT foreach_component(DWORD mask, iterator_func func, struct iterator_context *context)
{
    enum iterator_result ret;
    IEnumUnknown *enumerator;
    IUnknown *item;
    HRESULT hr;

    if (FAILED(hr = CreateComponentEnumerator(mask, WICComponentEnumerateDefault, &enumerator)))
        return hr;

    while (IEnumUnknown_Next(enumerator, 1, &item, NULL) == S_OK)
    {
        *context->result = NULL;

        ret = func(item, context);
        IUnknown_Release(item);

        if (ret == ITER_SKIP)
            continue;

        break;
    }

    IEnumUnknown_Release(enumerator);

    return *context->result ? S_OK : WINCODEC_ERR_COMPONENTNOTFOUND;
}

static HRESULT create_unknown_metadata_reader(IStream *stream, DWORD options, IWICMetadataReader **reader)
{
    IWICPersistStream *persist_stream = NULL;
    LARGE_INTEGER zero = {{0}};
    HRESULT hr;

    hr = IStream_Seek(stream, zero, STREAM_SEEK_SET, NULL);

    if (SUCCEEDED(hr))
        hr = UnknownMetadataReader_CreateInstance(&IID_IWICMetadataReader, (void **)reader);

    if (SUCCEEDED(hr))
        hr = IWICMetadataReader_QueryInterface(*reader, &IID_IWICPersistStream, (void **)&persist_stream);

    if (SUCCEEDED(hr))
        hr = IWICPersistStream_LoadEx(persist_stream, stream, NULL, options & WICPersistOptionMask);

    if (persist_stream)
        IWICPersistStream_Release(persist_stream);

    if (FAILED(hr))
    {
        IWICMetadataReader_Release(*reader);
        *reader = NULL;
    }

    return hr;
}

static HRESULT WINAPI ComponentFactory_CreateMetadataReader(IWICComponentFactory *iface,
        REFGUID format, const GUID *vendor, DWORD options, IStream *stream, IWICMetadataReader **reader)
{
    struct iterator_context context = { 0 };
    HRESULT hr;

    TRACE("%p,%s,%s,%lx,%p,%p\n", iface, debugstr_guid(format), debugstr_guid(vendor),
        options, stream, reader);

    if (!format || !reader)
        return E_INVALIDARG;

    context.format = format;
    context.vendor = vendor;
    context.options = options;
    context.stream = stream;
    context.result = (void **)reader;

    hr = foreach_component(WICMetadataReader, create_metadata_reader_iterator, &context);

    if (FAILED(hr) && vendor)
    {
        context.vendor = NULL;
        hr = foreach_component(WICMetadataReader, create_metadata_reader_iterator, &context);
    }

    if (FAILED(hr))
        WARN("Failed to create a metadata reader instance, hr %#lx.\n", hr);

    if (!*reader && !(options & WICMetadataCreationFailUnknown))
        hr = create_unknown_metadata_reader(stream, options, reader);

    return *reader ? S_OK : WINCODEC_ERR_COMPONENTNOTFOUND;
}

static HRESULT WINAPI ComponentFactory_CreateMetadataReaderFromContainer(IWICComponentFactory *iface,
        REFGUID format, const GUID *vendor, DWORD options, IStream *stream, IWICMetadataReader **reader)
{
    struct iterator_context context = { 0 };
    HRESULT hr;

    TRACE("%p,%s,%s,%lx,%p,%p\n", iface, debugstr_guid(format), debugstr_guid(vendor),
        options, stream, reader);

    if (!format || !stream || !reader)
        return E_INVALIDARG;

    context.format = format;
    context.vendor = vendor;
    context.options = options;
    context.stream = stream;
    context.result = (void **)reader;

    hr = foreach_component(WICMetadataReader, create_metadata_reader_from_container_iterator, &context);

    if (FAILED(hr) && vendor)
    {
        context.vendor = NULL;
        hr = foreach_component(WICMetadataReader, create_metadata_reader_from_container_iterator, &context);
    }

    if (FAILED(hr))
        WARN("Failed to create a metadata reader instance, hr %#lx.\n", hr);

    if (!*reader && !(options & WICMetadataCreationFailUnknown))
        hr = create_unknown_metadata_reader(stream, options, reader);

    return *reader ? S_OK : WINCODEC_ERR_COMPONENTNOTFOUND;
}

static HRESULT WINAPI ComponentFactory_CreateMetadataWriter(IWICComponentFactory *iface,
        REFGUID format, const GUID *vendor, DWORD options, IWICMetadataWriter **writer)
{
    FIXME("%p,%s,%s,%lx,%p: stub\n", iface, debugstr_guid(format), debugstr_guid(vendor), options, writer);
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
    TRACE("%p,%p,%p\n", iface, block_reader, query_reader);

    if (!block_reader || !query_reader)
        return E_INVALIDARG;

    return MetadataQueryReader_CreateInstance(block_reader, NULL, query_reader);
}

static HRESULT WINAPI ComponentFactory_CreateQueryWriterFromBlockWriter(IWICComponentFactory *iface,
        IWICMetadataBlockWriter *block_writer, IWICMetadataQueryWriter **query_writer)
{
    TRACE("%p,%p,%p\n", iface, block_writer, query_writer);

    if (!block_writer || !query_writer)
        return E_INVALIDARG;

    return MetadataQueryWriter_CreateInstance(block_writer, NULL, query_writer);
}

static HRESULT WINAPI ComponentFactory_CreateEncoderPropertyBag(IWICComponentFactory *iface,
        PROPBAG2 *options, UINT count, IPropertyBag2 **property)
{
    TRACE("(%p,%p,%u,%p)\n", iface, options, count, property);
    return CreatePropertyBag2(options, count, property);
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

HRESULT ImagingFactory_CreateInstance(REFIID iid, void** ppv)
{
    ImagingFactory *This;
    HRESULT ret;

    TRACE("(%s,%p)\n", debugstr_guid(iid), ppv);

    *ppv = NULL;

    This = malloc(sizeof(*This));
    if (!This) return E_OUTOFMEMORY;

    This->IWICImagingFactory2_iface.lpVtbl = &ImagingFactory_Vtbl;
    This->IWICComponentFactory_iface.lpVtbl = &ComponentFactory_Vtbl;
    This->ref = 1;

    ret = IWICImagingFactory2_QueryInterface(&This->IWICImagingFactory2_iface, iid, ppv);
    IWICImagingFactory2_Release(&This->IWICImagingFactory2_iface);

    return ret;
}

HRESULT WINAPI WICCreateBitmapFromSectionEx(UINT width, UINT height,
        REFWICPixelFormatGUID format, HANDLE section, UINT stride,
        UINT offset, WICSectionAccessLevel wicaccess, IWICBitmap **bitmap)
{
    SYSTEM_INFO sysinfo;
    UINT bpp, access, size, view_offset, view_size;
    void *view;
    HRESULT hr;

    TRACE("%u,%u,%s,%p,%u,%u,%#x,%p\n", width, height, debugstr_guid(format),
          section, stride, offset, wicaccess, bitmap);

    if (!width || !height || !section || !bitmap) return E_INVALIDARG;

    hr = get_pixelformat_bpp(format, &bpp);
    if (FAILED(hr)) return hr;

    switch (wicaccess)
    {
    case WICSectionAccessLevelReadWrite:
        access = FILE_MAP_READ | FILE_MAP_WRITE;
        break;

    case WICSectionAccessLevelRead:
        access = FILE_MAP_READ;
        break;

    default:
        FIXME("unsupported access %#x\n", wicaccess);
        return E_INVALIDARG;
    }

    if (!stride) stride = (((bpp * width) + 31) / 32) * 4;
    size = stride * height;
    if (size / height != stride) return E_INVALIDARG;

    GetSystemInfo(&sysinfo);
    view_offset = offset - (offset % sysinfo.dwAllocationGranularity);
    view_size = size + (offset - view_offset);

    view = MapViewOfFile(section, access, 0, view_offset, view_size);
    if (!view) return HRESULT_FROM_WIN32(GetLastError());

    offset -= view_offset;
    hr = BitmapImpl_Create(width, height, stride, 0, view, offset, format, WICBitmapCacheOnLoad, bitmap);
    if (FAILED(hr)) UnmapViewOfFile(view);
    return hr;
}

HRESULT WINAPI WICCreateBitmapFromSection(UINT width, UINT height,
        REFWICPixelFormatGUID format, HANDLE section,
        UINT stride, UINT offset, IWICBitmap **bitmap)
{
    TRACE("%u,%u,%s,%p,%u,%u,%p\n", width, height, debugstr_guid(format),
        section, stride, offset, bitmap);

    return WICCreateBitmapFromSectionEx(width, height, format, section,
        stride, offset, WICSectionAccessLevelRead, bitmap);
}
