/*
 * Misleadingly named convenience functions for accessing WIC.
 *
 * Copyright 2012 Vincent Povirk for CodeWeavers
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

HRESULT WINAPI IPropertyBag2_Write_Proxy(IPropertyBag2 *iface,
    ULONG cProperties, PROPBAG2 *ppropbag, VARIANT *pvarValue)
{
    return IPropertyBag2_Write(iface, cProperties, ppropbag, pvarValue);
}

HRESULT WINAPI IWICBitmapClipper_Initialize_Proxy_W(IWICBitmapClipper *iface,
    IWICBitmapSource *pISource, const WICRect *prc)
{
    return IWICBitmapClipper_Initialize(iface, pISource, prc);
}

HRESULT WINAPI IWICBitmapCodecInfo_GetContainerFormat_Proxy_W(IWICBitmapCodecInfo *iface,
    GUID *pguidContainerFormat)
{
    return IWICBitmapCodecInfo_GetContainerFormat(iface, pguidContainerFormat);
}

HRESULT WINAPI IWICBitmapCodecInfo_GetDeviceManufacturer_Proxy_W(IWICBitmapCodecInfo *iface,
    UINT cchDeviceManufacturer, WCHAR *wzDeviceManufacturer, UINT *pcchActual)
{
    return IWICBitmapCodecInfo_GetDeviceManufacturer(iface, cchDeviceManufacturer, wzDeviceManufacturer, pcchActual);
}

HRESULT WINAPI IWICBitmapCodecInfo_GetDeviceModels_Proxy_W(IWICBitmapCodecInfo *iface,
    UINT cchDeviceModels, WCHAR *wzDeviceModels, UINT *pcchActual)
{
    return IWICBitmapCodecInfo_GetDeviceModels(iface, cchDeviceModels, wzDeviceModels, pcchActual);
}

HRESULT WINAPI IWICBitmapCodecInfo_GetMimeTypes_Proxy_W(IWICBitmapCodecInfo *iface,
    UINT cchMimeTypes, WCHAR *wzMimeTypes, UINT *pcchActual)
{
    return IWICBitmapCodecInfo_GetMimeTypes(iface, cchMimeTypes, wzMimeTypes, pcchActual);
}

HRESULT WINAPI IWICBitmapCodecInfo_GetFileExtensions_Proxy_W(IWICBitmapCodecInfo *iface,
    UINT cchFileExtensions, WCHAR *wzFileExtensions, UINT *pcchActual)
{
    return IWICBitmapCodecInfo_GetFileExtensions(iface, cchFileExtensions, wzFileExtensions, pcchActual);
}

HRESULT WINAPI IWICBitmapCodecInfo_DoesSupportAnimation_Proxy_W(IWICBitmapCodecInfo *iface,
    BOOL *pfSupportAnimation)
{
    return IWICBitmapCodecInfo_DoesSupportAnimation(iface, pfSupportAnimation);
}

HRESULT WINAPI IWICBitmapCodecInfo_DoesSupportLossless_Proxy_W(IWICBitmapCodecInfo *iface,
    BOOL *pfSupportLossless)
{
    return IWICBitmapCodecInfo_DoesSupportLossless(iface, pfSupportLossless);
}

HRESULT WINAPI IWICBitmapCodecInfo_DoesSupportMultiframe_Proxy_W(IWICBitmapCodecInfo *iface,
    BOOL *pfSupportMultiframe)
{
    return IWICBitmapCodecInfo_DoesSupportMultiframe(iface, pfSupportMultiframe);
}

HRESULT WINAPI IWICBitmapDecoder_GetDecoderInfo_Proxy_W(IWICBitmapDecoder *iface,
    IWICBitmapDecoderInfo **ppIDecoderInfo)
{
    return IWICBitmapDecoder_GetDecoderInfo(iface, ppIDecoderInfo);
}

HRESULT WINAPI IWICBitmapDecoder_CopyPalette_Proxy_W(IWICBitmapDecoder *iface,
    IWICPalette *pIPalette)
{
    return IWICBitmapDecoder_CopyPalette(iface, pIPalette);
}

HRESULT WINAPI IWICBitmapDecoder_GetMetadataQueryReader_Proxy_W(IWICBitmapDecoder *iface,
    IWICMetadataQueryReader **ppIMetadataQueryReader)
{
    return IWICBitmapDecoder_GetMetadataQueryReader(iface, ppIMetadataQueryReader);
}

HRESULT WINAPI IWICBitmapDecoder_GetPreview_Proxy_W(IWICBitmapDecoder *iface,
    IWICBitmapSource **ppIBitmapSource)
{
    return IWICBitmapDecoder_GetPreview(iface, ppIBitmapSource);
}

HRESULT WINAPI IWICBitmapDecoder_GetColorContexts_Proxy_W(IWICBitmapDecoder *iface,
    UINT cCount, IWICColorContext **ppIColorContexts, UINT *pcActualCount)
{
    return IWICBitmapDecoder_GetColorContexts(iface, cCount, ppIColorContexts, pcActualCount);
}

HRESULT WINAPI IWICBitmapDecoder_GetThumbnail_Proxy_W(IWICBitmapDecoder *iface,
    IWICBitmapSource **ppIThumbnail)
{
    return IWICBitmapDecoder_GetThumbnail(iface, ppIThumbnail);
}

HRESULT WINAPI IWICBitmapDecoder_GetFrameCount_Proxy_W(IWICBitmapDecoder *iface,
    UINT *pCount)
{
    return IWICBitmapDecoder_GetFrameCount(iface, pCount);
}

HRESULT WINAPI IWICBitmapDecoder_GetFrame_Proxy_W(IWICBitmapDecoder *iface,
    UINT index, IWICBitmapFrameDecode **ppIBitmapFrame)
{
    return IWICBitmapDecoder_GetFrame(iface, index, ppIBitmapFrame);
}

HRESULT WINAPI IWICBitmapEncoder_Initialize_Proxy_W(IWICBitmapEncoder *iface,
    IStream *pIStream, WICBitmapEncoderCacheOption cacheOption)
{
    return IWICBitmapEncoder_Initialize(iface, pIStream, cacheOption);
}

HRESULT WINAPI IWICBitmapEncoder_GetEncoderInfo_Proxy_W(IWICBitmapEncoder *iface,
    IWICBitmapEncoderInfo **ppIEncoderInfo)
{
    return IWICBitmapEncoder_GetEncoderInfo(iface, ppIEncoderInfo);
}

HRESULT WINAPI IWICBitmapEncoder_SetPalette_Proxy_W(IWICBitmapEncoder *iface,
    IWICPalette *pIPalette)
{
    return IWICBitmapEncoder_SetPalette(iface, pIPalette);
}

HRESULT WINAPI IWICBitmapEncoder_SetThumbnail_Proxy_W(IWICBitmapEncoder *iface,
    IWICBitmapSource *pIThumbnail)
{
    return IWICBitmapEncoder_SetThumbnail(iface, pIThumbnail);
}

HRESULT WINAPI IWICBitmapEncoder_CreateNewFrame_Proxy_W(IWICBitmapEncoder *iface,
    IWICBitmapFrameEncode **ppIFrameEncode, IPropertyBag2 **ppIEncoderOptions)
{
    return IWICBitmapEncoder_CreateNewFrame(iface, ppIFrameEncode, ppIEncoderOptions);
}

HRESULT WINAPI IWICBitmapEncoder_Commit_Proxy_W(IWICBitmapEncoder *iface)
{
    return IWICBitmapEncoder_Commit(iface);
}

HRESULT WINAPI IWICBitmapEncoder_GetMetadataQueryWriter_Proxy_W(IWICBitmapEncoder *iface,
    IWICMetadataQueryWriter **ppIMetadataQueryWriter)
{
    return IWICBitmapEncoder_GetMetadataQueryWriter(iface, ppIMetadataQueryWriter);
}

HRESULT WINAPI IWICBitmapFlipRotator_Initialize_Proxy_W(IWICBitmapFlipRotator *iface,
    IWICBitmapSource *pISource, WICBitmapTransformOptions options)
{
    return IWICBitmapFlipRotator_Initialize(iface, pISource, options);
}

HRESULT WINAPI IWICBitmapFrameEncode_Initialize_Proxy_W(IWICBitmapFrameEncode *iface,
    IPropertyBag2 *pIEncoderOptions)
{
    return IWICBitmapFrameEncode_Initialize(iface, pIEncoderOptions);
}

HRESULT WINAPI IWICBitmapFrameEncode_SetSize_Proxy_W(IWICBitmapFrameEncode *iface,
    UINT uiWidth, UINT uiHeight)
{
    return IWICBitmapFrameEncode_SetSize(iface, uiWidth, uiHeight);
}

HRESULT WINAPI IWICBitmapFrameEncode_SetResolution_Proxy_W(IWICBitmapFrameEncode *iface,
    double dpiX, double dpiY)
{
    return IWICBitmapFrameEncode_SetResolution(iface, dpiX, dpiY);
}

HRESULT WINAPI IWICBitmapFrameEncode_SetColorContexts_Proxy_W(IWICBitmapFrameEncode *iface,
    UINT cCount, IWICColorContext **ppIColorContext)
{
    return IWICBitmapFrameEncode_SetColorContexts(iface, cCount, ppIColorContext);
}

HRESULT WINAPI IWICBitmapFrameEncode_SetThumbnail_Proxy_W(IWICBitmapFrameEncode *iface,
    IWICBitmapSource *pIThumbnail)
{
    return IWICBitmapFrameEncode_SetThumbnail(iface, pIThumbnail);
}

HRESULT WINAPI IWICBitmapFrameEncode_WriteSource_Proxy_W(IWICBitmapFrameEncode *iface,
    IWICBitmapSource *pIBitmapSource, WICRect *prc)
{
    if (prc && (prc->Width <= 0 || prc->Height <= 0))
        prc = NULL;

    return IWICBitmapFrameEncode_WriteSource(iface, pIBitmapSource, prc);
}

HRESULT WINAPI IWICBitmapFrameEncode_Commit_Proxy_W(IWICBitmapFrameEncode *iface)
{
    return IWICBitmapFrameEncode_Commit(iface);
}

HRESULT WINAPI IWICBitmapFrameEncode_GetMetadataQueryWriter_Proxy_W(IWICBitmapFrameEncode *iface,
    IWICMetadataQueryWriter **ppIMetadataQueryWriter)
{
    return IWICBitmapFrameEncode_GetMetadataQueryWriter(iface, ppIMetadataQueryWriter);
}

HRESULT WINAPI IWICBitmapLock_GetDataPointer_Proxy_W(IWICBitmapLock *iface,
    UINT *pcbBufferSize, BYTE **ppbData)
{
    return IWICBitmapLock_GetDataPointer(iface, pcbBufferSize, ppbData);
}

HRESULT WINAPI IWICBitmapLock_GetStride_Proxy_W(IWICBitmapLock *iface,
    UINT *pcbStride)
{
    return IWICBitmapLock_GetStride(iface, pcbStride);
}

HRESULT WINAPI IWICBitmapScaler_Initialize_Proxy_W(IWICBitmapScaler *iface,
    IWICBitmapSource *pISource, UINT uiWidth, UINT uiHeight, WICBitmapInterpolationMode mode)
{
    return IWICBitmapScaler_Initialize(iface, pISource, uiWidth, uiHeight, mode);
}

HRESULT WINAPI IWICBitmapSource_GetSize_Proxy_W(IWICBitmapSource *iface,
    UINT *puiWidth, UINT *puiHeight)
{
    return IWICBitmapSource_GetSize(iface, puiWidth, puiHeight);
}

HRESULT WINAPI IWICBitmapSource_GetPixelFormat_Proxy_W(IWICBitmapSource *iface,
    WICPixelFormatGUID *pPixelFormat)
{
    return IWICBitmapSource_GetPixelFormat(iface, pPixelFormat);
}

HRESULT WINAPI IWICBitmapSource_GetResolution_Proxy_W(IWICBitmapSource *iface,
    double *pDpiX, double *pDpiY)
{
    return IWICBitmapSource_GetResolution(iface, pDpiX, pDpiY);
}

HRESULT WINAPI IWICBitmapSource_CopyPalette_Proxy_W(IWICBitmapSource *iface,
    IWICPalette *pIPalette)
{
    return IWICBitmapSource_CopyPalette(iface, pIPalette);
}

HRESULT WINAPI IWICBitmapSource_CopyPixels_Proxy_W(IWICBitmapSource *iface,
    const WICRect *prc, UINT cbStride, UINT cbBufferSize, BYTE *pbBuffer)
{
    return IWICBitmapSource_CopyPixels(iface, prc, cbStride, cbBufferSize, pbBuffer);
}

HRESULT WINAPI IWICBitmap_Lock_Proxy_W(IWICBitmap *iface,
    const WICRect *prcLock, DWORD flags, IWICBitmapLock **ppILock)
{
    return IWICBitmap_Lock(iface, prcLock, flags, ppILock);
}

HRESULT WINAPI IWICBitmap_SetPalette_Proxy_W(IWICBitmap *iface,
    IWICPalette *pIPalette)
{
    return IWICBitmap_SetPalette(iface, pIPalette);
}

HRESULT WINAPI IWICBitmap_SetResolution_Proxy_W(IWICBitmap *iface,
    double dpiX, double dpiY)
{
    return IWICBitmap_SetResolution(iface, dpiX, dpiY);
}

HRESULT WINAPI IWICColorContext_InitializeFromMemory_Proxy_W(IWICColorContext *iface,
    const BYTE *pbBuffer, UINT cbBufferSize)
{
    return IWICColorContext_InitializeFromMemory(iface, pbBuffer, cbBufferSize);
}

HRESULT WINAPI IWICComponentFactory_CreateMetadataWriterFromReader_Proxy_W(IWICComponentFactory *iface,
    IWICMetadataReader *pIReader, const GUID *pguidVendor, IWICMetadataWriter **ppIWriter)
{
    return IWICComponentFactory_CreateMetadataWriterFromReader(iface, pIReader, pguidVendor, ppIWriter);
}

HRESULT WINAPI IWICComponentFactory_CreateQueryWriterFromBlockWriter_Proxy_W(IWICComponentFactory *iface,
    IWICMetadataBlockWriter *pIBlockWriter, IWICMetadataQueryWriter **ppIQueryWriter)
{
    return IWICComponentFactory_CreateQueryWriterFromBlockWriter(iface, pIBlockWriter, ppIQueryWriter);
}

HRESULT WINAPI IWICComponentInfo_GetCLSID_Proxy_W(IWICComponentInfo *iface,
    CLSID *pclsid)
{
    return IWICComponentInfo_GetCLSID(iface, pclsid);
}

HRESULT WINAPI IWICComponentInfo_GetAuthor_Proxy_W(IWICComponentInfo *iface,
    UINT cchAuthor, WCHAR *wzAuthor, UINT *pcchActual)
{
    return IWICComponentInfo_GetAuthor(iface, cchAuthor, wzAuthor, pcchActual);
}

HRESULT WINAPI IWICComponentInfo_GetVersion_Proxy_W(IWICComponentInfo *iface,
    UINT cchVersion, WCHAR *wzVersion, UINT *pcchActual)
{
    return IWICComponentInfo_GetVersion(iface, cchVersion, wzVersion, pcchActual);
}

HRESULT WINAPI IWICComponentInfo_GetSpecVersion_Proxy_W(IWICComponentInfo *iface,
    UINT cchSpecVersion, WCHAR *wzSpecVersion, UINT *pcchActual)
{
    return IWICComponentInfo_GetSpecVersion(iface, cchSpecVersion, wzSpecVersion, pcchActual);
}

HRESULT WINAPI IWICComponentInfo_GetFriendlyName_Proxy_W(IWICComponentInfo *iface,
    UINT cchFriendlyName, WCHAR *wzFriendlyName, UINT *pcchActual)
{
    return IWICComponentInfo_GetFriendlyName(iface, cchFriendlyName, wzFriendlyName, pcchActual);
}

HRESULT WINAPI IWICFastMetadataEncoder_Commit_Proxy_W(IWICFastMetadataEncoder *iface)
{
    return IWICFastMetadataEncoder_Commit(iface);
}

HRESULT WINAPI IWICFastMetadataEncoder_GetMetadataQueryWriter_Proxy_W(IWICFastMetadataEncoder *iface,
    IWICMetadataQueryWriter **ppIMetadataQueryWriter)
{
    return IWICFastMetadataEncoder_GetMetadataQueryWriter(iface, ppIMetadataQueryWriter);
}

HRESULT WINAPI IWICBitmapFrameDecode_GetMetadataQueryReader_Proxy_W(IWICBitmapFrameDecode *iface,
    IWICMetadataQueryReader **ppIMetadataQueryReader)
{
    return IWICBitmapFrameDecode_GetMetadataQueryReader(iface, ppIMetadataQueryReader);
}

HRESULT WINAPI IWICBitmapFrameDecode_GetColorContexts_Proxy_W(IWICBitmapFrameDecode *iface,
    UINT cCount, IWICColorContext **ppIColorContexts, UINT *pcActualCount)
{
    return IWICBitmapFrameDecode_GetColorContexts(iface, cCount, ppIColorContexts, pcActualCount);
}

HRESULT WINAPI IWICBitmapFrameDecode_GetThumbnail_Proxy_W(IWICBitmapFrameDecode *iface,
    IWICBitmapSource **ppIThumbnail)
{
    return IWICBitmapFrameDecode_GetThumbnail(iface, ppIThumbnail);
}

HRESULT WINAPI IWICFormatConverter_Initialize_Proxy_W(IWICFormatConverter *iface,
    IWICBitmapSource *pISource, REFWICPixelFormatGUID dstFormat, WICBitmapDitherType dither,
    IWICPalette *pIPalette, double alphaThresholdPercent, WICBitmapPaletteType paletteTranslate)
{
    return IWICFormatConverter_Initialize(iface, pISource, dstFormat, dither,
        pIPalette, alphaThresholdPercent, paletteTranslate);
}

HRESULT WINAPI IWICImagingFactory_CreateBitmapClipper_Proxy_W(IWICImagingFactory *pFactory,
    IWICBitmapClipper **ppIBitmapClipper)
{
    return IWICImagingFactory_CreateBitmapClipper(pFactory, ppIBitmapClipper);
}

HRESULT WINAPI IWICImagingFactory_CreateBitmapFlipRotator_Proxy_W(IWICImagingFactory *pFactory,
    IWICBitmapFlipRotator **ppIBitmapFlipRotator)
{
    return IWICImagingFactory_CreateBitmapFlipRotator(pFactory, ppIBitmapFlipRotator);
}

HRESULT WINAPI IWICImagingFactory_CreateBitmapFromHBITMAP_Proxy_W(IWICImagingFactory *pFactory,
    HBITMAP hBitmap, HPALETTE hPalette, WICBitmapAlphaChannelOption options, IWICBitmap **ppIBitmap)
{
    return IWICImagingFactory_CreateBitmapFromHBITMAP(pFactory, hBitmap, hPalette, options, ppIBitmap);
}

HRESULT WINAPI IWICImagingFactory_CreateBitmapFromHICON_Proxy_W(IWICImagingFactory *pFactory,
    HICON hIcon, IWICBitmap **ppIBitmap)
{
    return IWICImagingFactory_CreateBitmapFromHICON(pFactory, hIcon, ppIBitmap);
}

HRESULT WINAPI IWICImagingFactory_CreateBitmapFromMemory_Proxy_W(IWICImagingFactory *pFactory,
    UINT uiWidth, UINT uiHeight, REFWICPixelFormatGUID pixelFormat, UINT cbStride,
    UINT cbBufferSize, BYTE *pbBuffer, IWICBitmap **ppIBitmap)
{
    return IWICImagingFactory_CreateBitmapFromMemory(pFactory, uiWidth, uiHeight,
        pixelFormat, cbStride, cbBufferSize, pbBuffer, ppIBitmap);
}

HRESULT WINAPI IWICImagingFactory_CreateBitmapFromSource_Proxy_W(IWICImagingFactory *pFactory,
    IWICBitmapSource *piBitmapSource, WICBitmapCreateCacheOption option, IWICBitmap **ppIBitmap)
{
    return IWICImagingFactory_CreateBitmapFromSource(pFactory, piBitmapSource, option, ppIBitmap);
}

HRESULT WINAPI IWICImagingFactory_CreateBitmapScaler_Proxy_W(IWICImagingFactory *pFactory,
    IWICBitmapScaler **ppIBitmapScaler)
{
    return IWICImagingFactory_CreateBitmapScaler(pFactory, ppIBitmapScaler);
}

HRESULT WINAPI IWICImagingFactory_CreateBitmap_Proxy_W(IWICImagingFactory *pFactory,
    UINT uiWidth, UINT uiHeight, REFWICPixelFormatGUID pixelFormat,
    WICBitmapCreateCacheOption option, IWICBitmap **ppIBitmap)
{
    return IWICImagingFactory_CreateBitmap(pFactory, uiWidth, uiHeight,
        pixelFormat, option, ppIBitmap);
}

HRESULT WINAPI IWICImagingFactory_CreateComponentInfo_Proxy_W(IWICImagingFactory *pFactory,
    REFCLSID clsidComponent, IWICComponentInfo **ppIInfo)
{
    return IWICImagingFactory_CreateComponentInfo(pFactory, clsidComponent, ppIInfo);
}

HRESULT WINAPI IWICImagingFactory_CreateDecoderFromFileHandle_Proxy_W(IWICImagingFactory *pFactory,
    ULONG_PTR hFile, const GUID *pguidVendor, WICDecodeOptions metadataOptions, IWICBitmapDecoder **ppIDecoder)
{
    return IWICImagingFactory_CreateDecoderFromFileHandle(pFactory, hFile, pguidVendor, metadataOptions, ppIDecoder);
}

HRESULT WINAPI IWICImagingFactory_CreateDecoderFromFilename_Proxy_W(IWICImagingFactory *pFactory,
    LPCWSTR wzFilename, const GUID *pguidVendor, DWORD dwDesiredAccess,
    WICDecodeOptions metadataOptions, IWICBitmapDecoder **ppIDecoder)
{
    return IWICImagingFactory_CreateDecoderFromFilename(pFactory, wzFilename,
        pguidVendor, dwDesiredAccess, metadataOptions, ppIDecoder);
}

HRESULT WINAPI IWICImagingFactory_CreateDecoderFromStream_Proxy_W(IWICImagingFactory *pFactory,
    IStream *pIStream, const GUID *pguidVendor,
    WICDecodeOptions metadataOptions, IWICBitmapDecoder **ppIDecoder)
{
    return IWICImagingFactory_CreateDecoderFromStream(pFactory, pIStream, pguidVendor, metadataOptions, ppIDecoder);
}

HRESULT WINAPI IWICImagingFactory_CreateEncoder_Proxy_W(IWICImagingFactory *pFactory,
    REFGUID guidContainerFormat, const GUID *pguidVendor, IWICBitmapEncoder **ppIEncoder)
{
    return IWICImagingFactory_CreateEncoder(pFactory, guidContainerFormat, pguidVendor, ppIEncoder);
}

HRESULT WINAPI IWICImagingFactory_CreateFastMetadataEncoderFromDecoder_Proxy_W(IWICImagingFactory *pFactory,
    IWICBitmapDecoder *pIDecoder, IWICFastMetadataEncoder **ppIFastEncoder)
{
    return IWICImagingFactory_CreateFastMetadataEncoderFromDecoder(pFactory, pIDecoder, ppIFastEncoder);
}

HRESULT WINAPI IWICImagingFactory_CreateFastMetadataEncoderFromFrameDecode_Proxy_W(IWICImagingFactory *pFactory,
    IWICBitmapFrameDecode *pIFrameDecoder, IWICFastMetadataEncoder **ppIFastEncoder)
{
    return IWICImagingFactory_CreateFastMetadataEncoderFromFrameDecode(pFactory, pIFrameDecoder, ppIFastEncoder);
}

HRESULT WINAPI IWICImagingFactory_CreateFormatConverter_Proxy_W(IWICImagingFactory *pFactory,
    IWICFormatConverter **ppIFormatConverter)
{
    return IWICImagingFactory_CreateFormatConverter(pFactory, ppIFormatConverter);
}

HRESULT WINAPI IWICImagingFactory_CreatePalette_Proxy_W(IWICImagingFactory *pFactory,
    IWICPalette **ppIPalette)
{
    return IWICImagingFactory_CreatePalette(pFactory, ppIPalette);
}

HRESULT WINAPI IWICImagingFactory_CreateQueryWriterFromReader_Proxy_W(IWICImagingFactory *pFactory,
    IWICMetadataQueryReader *pIQueryReader, const GUID *pguidVendor,
    IWICMetadataQueryWriter **ppIQueryWriter)
{
    return IWICImagingFactory_CreateQueryWriterFromReader(pFactory, pIQueryReader, pguidVendor, ppIQueryWriter);
}

HRESULT WINAPI IWICImagingFactory_CreateQueryWriter_Proxy_W(IWICImagingFactory *pFactory,
    REFGUID guidMetadataFormat, const GUID *pguidVendor, IWICMetadataQueryWriter **ppIQueryWriter)
{
    return IWICImagingFactory_CreateQueryWriter(pFactory, guidMetadataFormat, pguidVendor, ppIQueryWriter);
}

HRESULT WINAPI IWICImagingFactory_CreateStream_Proxy_W(IWICImagingFactory *pFactory,
    IWICStream **ppIWICStream)
{
    return IWICImagingFactory_CreateStream(pFactory, ppIWICStream);
}

HRESULT WINAPI IWICMetadataBlockReader_GetCount_Proxy_W(IWICMetadataBlockReader *iface,
    UINT *pcCount)
{
    return IWICMetadataBlockReader_GetCount(iface, pcCount);
}

HRESULT WINAPI IWICMetadataBlockReader_GetReaderByIndex_Proxy_W(IWICMetadataBlockReader *iface,
    UINT nIndex, IWICMetadataReader **ppIMetadataReader)
{
    return IWICMetadataBlockReader_GetReaderByIndex(iface, nIndex, ppIMetadataReader);
}

HRESULT WINAPI IWICMetadataQueryReader_GetContainerFormat_Proxy_W(IWICMetadataQueryReader *iface,
    GUID *pguidContainerFormat)
{
    return IWICMetadataQueryReader_GetContainerFormat(iface, pguidContainerFormat);
}

HRESULT WINAPI IWICMetadataQueryReader_GetLocation_Proxy_W(IWICMetadataQueryReader *iface,
    UINT cchMaxLength, WCHAR *wzNamespace, UINT *pcchActualLength)
{
    return IWICMetadataQueryReader_GetLocation(iface, cchMaxLength, wzNamespace, pcchActualLength);
}

HRESULT WINAPI IWICMetadataQueryReader_GetMetadataByName_Proxy_W(IWICMetadataQueryReader *iface,
    LPCWSTR wzName, PROPVARIANT *pvarValue)
{
    return IWICMetadataQueryReader_GetMetadataByName(iface, wzName, pvarValue);
}

HRESULT WINAPI IWICMetadataQueryReader_GetEnumerator_Proxy_W(IWICMetadataQueryReader *iface,
    IEnumString **ppIEnumString)
{
    return IWICMetadataQueryReader_GetEnumerator(iface, ppIEnumString);
}

HRESULT WINAPI IWICMetadataQueryWriter_SetMetadataByName_Proxy_W(IWICMetadataQueryWriter *iface,
    LPCWSTR wzName, const PROPVARIANT *pvarValue)
{
    return IWICMetadataQueryWriter_SetMetadataByName(iface, wzName, pvarValue);
}

HRESULT WINAPI IWICMetadataQueryWriter_RemoveMetadataByName_Proxy_W(IWICMetadataQueryWriter *iface,
    LPCWSTR wzName)
{
    return IWICMetadataQueryWriter_RemoveMetadataByName(iface, wzName);
}

HRESULT WINAPI IWICPalette_InitializePredefined_Proxy_W(IWICPalette *iface,
    WICBitmapPaletteType ePaletteType, BOOL fAddTransparentColor)
{
    return IWICPalette_InitializePredefined(iface, ePaletteType, fAddTransparentColor);
}

HRESULT WINAPI IWICPalette_InitializeCustom_Proxy_W(IWICPalette *iface,
    WICColor *pColors, UINT colorCount)
{
    return IWICPalette_InitializeCustom(iface, pColors, colorCount);
}

HRESULT WINAPI IWICPalette_InitializeFromBitmap_Proxy_W(IWICPalette *iface,
    IWICBitmapSource *pISurface, UINT colorCount, BOOL fAddTransparentColor)
{
    return IWICPalette_InitializeFromBitmap(iface, pISurface, colorCount, fAddTransparentColor);
}

HRESULT WINAPI IWICPalette_InitializeFromPalette_Proxy_W(IWICPalette *iface,
    IWICPalette *pIPalette)
{
    return IWICPalette_InitializeFromPalette(iface, pIPalette);
}

HRESULT WINAPI IWICPalette_GetType_Proxy_W(IWICPalette *iface,
    WICBitmapPaletteType *pePaletteType)
{
    return IWICPalette_GetType(iface, pePaletteType);
}

HRESULT WINAPI IWICPalette_GetColorCount_Proxy_W(IWICPalette *iface,
    UINT *pcCount)
{
    return IWICPalette_GetColorCount(iface, pcCount);
}

HRESULT WINAPI IWICPalette_GetColors_Proxy_W(IWICPalette *iface,
    UINT colorCount, WICColor *pColors, UINT *pcActualColors)
{
    return IWICPalette_GetColors(iface, colorCount, pColors, pcActualColors);
}

HRESULT WINAPI IWICPalette_HasAlpha_Proxy_W(IWICPalette *iface,
    BOOL *pfHasAlpha)
{
    return IWICPalette_HasAlpha(iface, pfHasAlpha);
}

HRESULT WINAPI IWICStream_InitializeFromIStream_Proxy_W(IWICStream *iface,
    IStream *pIStream)
{
    return IWICStream_InitializeFromIStream(iface, pIStream);
}

HRESULT WINAPI IWICStream_InitializeFromMemory_Proxy_W(IWICStream *iface,
    BYTE *pbBuffer, DWORD cbBufferSize)
{
    return IWICStream_InitializeFromMemory(iface, pbBuffer, cbBufferSize);
}

HRESULT WINAPI IWICPixelFormatInfo_GetBitsPerPixel_Proxy_W(IWICPixelFormatInfo *iface, UINT *bpp)
{
    return IWICPixelFormatInfo_GetBitsPerPixel(iface, bpp);
}

HRESULT WINAPI IWICPixelFormatInfo_GetChannelCount_Proxy_W(IWICPixelFormatInfo *iface, UINT *count)
{
    return IWICPixelFormatInfo_GetChannelCount(iface, count);
}

HRESULT WINAPI IWICPixelFormatInfo_GetChannelMask_Proxy_W(IWICPixelFormatInfo *iface, UINT channel, UINT buffer_size, BYTE *buffer, UINT *actual)
{
    return IWICPixelFormatInfo_GetChannelMask(iface, channel, buffer_size, buffer, actual);
}

HRESULT WINAPI WICCreateColorContext_Proxy(IWICImagingFactory *iface, IWICColorContext **ppIWICColorContext)
{
    TRACE("%p, %p\n", iface, ppIWICColorContext);

    return IWICImagingFactory_CreateColorContext(iface, ppIWICColorContext);
}

HRESULT WINAPI WICCreateImagingFactory_Proxy(UINT SDKVersion, IWICImagingFactory **ppIImagingFactory)
{
    TRACE("%x, %p\n", SDKVersion, ppIImagingFactory);

    return ImagingFactory_CreateInstance(&IID_IWICImagingFactory, (void**)ppIImagingFactory);
}

HRESULT WINAPI WICSetEncoderFormat_Proxy(IWICBitmapSource *pSourceIn,
    IWICPalette *pIPalette, IWICBitmapFrameEncode *pIFrameEncode,
    IWICBitmapSource **ppSourceOut)
{
    HRESULT hr;
    WICPixelFormatGUID pixelformat, framepixelformat;

    TRACE("%p,%p,%p,%p\n", pSourceIn, pIPalette, pIFrameEncode, ppSourceOut);

    if (pIPalette) FIXME("ignoring palette\n");

    if (!pSourceIn || !pIFrameEncode || !ppSourceOut)
        return E_INVALIDARG;

    *ppSourceOut = NULL;

    hr = IWICBitmapSource_GetPixelFormat(pSourceIn, &pixelformat);

    if (SUCCEEDED(hr))
    {
        framepixelformat = pixelformat;
        hr = IWICBitmapFrameEncode_SetPixelFormat(pIFrameEncode, &framepixelformat);
    }

    if (SUCCEEDED(hr))
    {
        if (IsEqualGUID(&pixelformat, &framepixelformat))
        {
            *ppSourceOut = pSourceIn;
            IWICBitmapSource_AddRef(pSourceIn);
        }
        else
        {
            hr = WICConvertBitmapSource(&framepixelformat, pSourceIn, ppSourceOut);
        }
    }

    return hr;
}
