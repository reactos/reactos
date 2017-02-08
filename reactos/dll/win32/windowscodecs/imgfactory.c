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

#include "wincodecs_private.h"

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
    IWICStream *stream;
    HRESULT hr;

    TRACE("(%p,%lx,%s,%u,%p)\n", iface, hFile, debugstr_guid(pguidVendor),
        metadataOptions, ppIDecoder);

    hr = StreamImpl_Create(&stream);
    if (SUCCEEDED(hr))
    {
        hr = stream_initialize_from_filehandle(stream, (HANDLE)hFile);
        if (SUCCEEDED(hr))
        {
            hr = IWICComponentFactory_CreateDecoderFromStream(iface, (IStream*)stream,
                pguidVendor, metadataOptions, ppIDecoder);
        }
        IWICStream_Release(stream);
    }
    return hr;
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
    return FormatConverter_CreateInstance(&IID_IWICFormatConverter, (void**)ppIFormatConverter);
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
    TRACE("(%p,%p)\n", iface, ppIBitmapClipper);
    return BitmapClipper_Create(ppIBitmapClipper);
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
    TRACE("(%p,%p)\n", iface, ppIColorTransform);
    return ColorTransform_Create(ppIColorTransform);
}

static HRESULT WINAPI ComponentFactory_CreateBitmap(IWICComponentFactory *iface,
    UINT uiWidth, UINT uiHeight, REFWICPixelFormatGUID pixelFormat,
    WICBitmapCreateCacheOption option, IWICBitmap **ppIBitmap)
{
    TRACE("(%p,%u,%u,%s,%u,%p)\n", iface, uiWidth, uiHeight,
        debugstr_guid(pixelFormat), option, ppIBitmap);
    return BitmapImpl_Create(uiWidth, uiHeight, 0, 0, NULL, pixelFormat, option, ppIBitmap);
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
        hr = BitmapImpl_Create(width, height, 0, 0, NULL, &pixelformat, option, &result);

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

static HRESULT WINAPI ComponentFactory_CreateBitmapFromSourceRect(IWICComponentFactory *iface,
    IWICBitmapSource *piBitmapSource, UINT x, UINT y, UINT width, UINT height,
    IWICBitmap **ppIBitmap)
{
    FIXME("(%p,%p,%u,%u,%u,%u,%p): stub\n", iface, piBitmapSource, x, y, width,
        height, ppIBitmap);
    return E_NOTIMPL;
}

static HRESULT WINAPI ComponentFactory_CreateBitmapFromMemory(IWICComponentFactory *iface,
    UINT width, UINT height, REFWICPixelFormatGUID format, UINT stride,
    UINT size, BYTE *buffer, IWICBitmap **bitmap)
{
    TRACE("(%p,%u,%u,%s,%u,%u,%p,%p\n", iface, width, height,
        debugstr_guid(format), stride, size, buffer, bitmap);

    if (!stride || !size || !buffer || !bitmap) return E_INVALIDARG;

    return BitmapImpl_Create(width, height, stride, size, buffer, format, WICBitmapCacheOnLoad, bitmap);
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
        FIXME("unrecognized bitfields %x,%x,%x\n", bmh.bV4RedMask,
            bmh.bV4GreenMask, bmh.bV4BlueMask);
        ret = FALSE;
    }

    DeleteDC(hdc);
    return ret;
}

static HRESULT WINAPI ComponentFactory_CreateBitmapFromHBITMAP(IWICComponentFactory *iface,
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

    hr = BitmapImpl_Create(bm.bmWidth, bm.bmHeight, bm.bmWidthBytes, 0, NULL, &format, WICBitmapCacheOnLoad, bitmap);
    if (hr != S_OK) return hr;

    hr = IWICBitmap_Lock(*bitmap, NULL, WICBitmapLockWrite, &lock);
    if (hr == S_OK)
    {
        BYTE *buffer;
        HDC hdc;
        char bmibuf[FIELD_OFFSET(BITMAPINFO, bmiColors) + 256 * sizeof(RGBQUAD)];
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

static HRESULT WINAPI ComponentFactory_CreateBitmapFromHICON(IWICComponentFactory *iface,
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

    hr = BitmapImpl_Create(width, height, stride, size, NULL,
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
            bits = (DWORD *)buffer;
            for (x = 0; x < width && !has_alpha; x++, bits++)
            {
                for (y = 0; y < height; y++)
                {
                    if (*bits & 0xff000000)
                    {
                        has_alpha = TRUE;
                        break;
                    }
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

            mask = HeapAlloc(GetProcessHeap(), 0, size);
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

            HeapFree(GetProcessHeap(), 0, mask);
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
    HRESULT hr;
    IEnumUnknown *enumreaders;
    IUnknown *unkreaderinfo;
    IWICMetadataReaderInfo *readerinfo;
    IWICPersistStream *wicpersiststream;
    ULONG num_fetched;
    GUID decoder_vendor;
    BOOL matches;
    LARGE_INTEGER zero;

    TRACE("%p,%s,%s,%x,%p,%p\n", iface, debugstr_guid(format), debugstr_guid(vendor),
        options, stream, reader);

    if (!format || !stream || !reader)
        return E_INVALIDARG;

    zero.QuadPart = 0;

    hr = CreateComponentEnumerator(WICMetadataReader, WICComponentEnumerateDefault, &enumreaders);
    if (FAILED(hr)) return hr;

    *reader = NULL;

start:
    while (!*reader)
    {
        hr = IEnumUnknown_Next(enumreaders, 1, &unkreaderinfo, &num_fetched);

        if (hr == S_OK)
        {
            hr = IUnknown_QueryInterface(unkreaderinfo, &IID_IWICMetadataReaderInfo, (void**)&readerinfo);

            if (SUCCEEDED(hr))
            {
                if (vendor)
                {
                    hr = IWICMetadataReaderInfo_GetVendorGUID(readerinfo, &decoder_vendor);

                    if (FAILED(hr) || !IsEqualIID(vendor, &decoder_vendor))
                    {
                        IWICMetadataReaderInfo_Release(readerinfo);
                        IUnknown_Release(unkreaderinfo);
                        continue;
                    }
                }

                hr = IWICMetadataReaderInfo_MatchesPattern(readerinfo, format, stream, &matches);

                if (SUCCEEDED(hr) && matches)
                {
                    hr = IStream_Seek(stream, zero, STREAM_SEEK_SET, NULL);

                    if (SUCCEEDED(hr))
                        hr = IWICMetadataReaderInfo_CreateInstance(readerinfo, reader);

                    if (SUCCEEDED(hr))
                    {
                        hr = IWICMetadataReader_QueryInterface(*reader, &IID_IWICPersistStream, (void**)&wicpersiststream);

                        if (SUCCEEDED(hr))
                        {
                            hr = IWICPersistStream_LoadEx(wicpersiststream,
                                stream, vendor, options & WICPersistOptionsMask);

                            IWICPersistStream_Release(wicpersiststream);
                        }

                        if (FAILED(hr))
                        {
                            IWICMetadataReader_Release(*reader);
                            *reader = NULL;
                        }
                    }
                }

                IUnknown_Release(readerinfo);
            }

            IUnknown_Release(unkreaderinfo);
        }
        else
            break;
    }

    if (!*reader && vendor)
    {
        vendor = NULL;
        IEnumUnknown_Reset(enumreaders);
        goto start;
    }

    IEnumUnknown_Release(enumreaders);

    if (!*reader && !(options & WICMetadataCreationFailUnknown))
    {
        hr = IStream_Seek(stream, zero, STREAM_SEEK_SET, NULL);

        if (SUCCEEDED(hr))
            hr = UnknownMetadataReader_CreateInstance(&IID_IWICMetadataReader, (void**)reader);

        if (SUCCEEDED(hr))
        {
            hr = IWICMetadataReader_QueryInterface(*reader, &IID_IWICPersistStream, (void**)&wicpersiststream);

            if (SUCCEEDED(hr))
            {
                hr = IWICPersistStream_LoadEx(wicpersiststream, stream, NULL, options & WICPersistOptionsMask);

                IWICPersistStream_Release(wicpersiststream);
            }

            if (FAILED(hr))
            {
                IWICMetadataReader_Release(*reader);
                *reader = NULL;
            }
        }
    }

    if (*reader)
        return S_OK;
    else
        return WINCODEC_ERR_COMPONENTNOTFOUND;
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

HRESULT ComponentFactory_CreateInstance(REFIID iid, void** ppv)
{
    ComponentFactory *This;
    HRESULT ret;

    TRACE("(%s,%p)\n", debugstr_guid(iid), ppv);

    *ppv = NULL;

    This = HeapAlloc(GetProcessHeap(), 0, sizeof(ComponentFactory));
    if (!This) return E_OUTOFMEMORY;

    This->IWICComponentFactory_iface.lpVtbl = &ComponentFactory_Vtbl;
    This->ref = 1;

    ret = IWICComponentFactory_QueryInterface(&This->IWICComponentFactory_iface, iid, ppv);
    IWICComponentFactory_Release(&This->IWICComponentFactory_iface);

    return ret;
}
