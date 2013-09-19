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

#ifndef WINCODECS_PRIVATE_H
#define WINCODECS_PRIVATE_H

DEFINE_GUID(CLSID_WineTgaDecoder, 0xb11fc79a,0x67cc,0x43e6,0xa9,0xce,0xe3,0xd5,0x49,0x45,0xd3,0x04);

DEFINE_GUID(CLSID_WICIcnsEncoder, 0x312fb6f1,0xb767,0x409d,0x8a,0x6d,0x0f,0xc1,0x54,0xd4,0xf0,0x5c);

DEFINE_GUID(GUID_WineContainerFormatTga, 0x0c44fda1,0xa5c5,0x4298,0x96,0x85,0x47,0x3f,0xc1,0x7c,0xd3,0x22);

DEFINE_GUID(GUID_VendorWine, 0xddf46da1,0x7dc1,0x404e,0x98,0xf2,0xef,0xa4,0x8d,0xfc,0x95,0x0a);

extern HRESULT FormatConverter_CreateInstance(IUnknown *pUnkOuter, REFIID riid, void** ppv) DECLSPEC_HIDDEN;
extern HRESULT ComponentFactory_CreateInstance(IUnknown *pUnkOuter, REFIID riid, void** ppv) DECLSPEC_HIDDEN;
extern HRESULT BmpDecoder_CreateInstance(IUnknown *pUnkOuter, REFIID riid, void** ppv) DECLSPEC_HIDDEN;
extern HRESULT PngDecoder_CreateInstance(IUnknown *pUnkOuter, REFIID iid, void** ppv) DECLSPEC_HIDDEN;
extern HRESULT PngEncoder_CreateInstance(IUnknown *pUnkOuter, REFIID iid, void** ppv) DECLSPEC_HIDDEN;
extern HRESULT BmpEncoder_CreateInstance(IUnknown *pUnkOuter, REFIID iid, void** ppv) DECLSPEC_HIDDEN;
extern HRESULT DibDecoder_CreateInstance(IUnknown *pUnkOuter, REFIID iid, void** ppv) DECLSPEC_HIDDEN;
extern HRESULT GifDecoder_CreateInstance(IUnknown *pUnkOuter, REFIID riid, void** ppv) DECLSPEC_HIDDEN;
extern HRESULT IcoDecoder_CreateInstance(IUnknown *pUnkOuter, REFIID iid, void** ppv) DECLSPEC_HIDDEN;
extern HRESULT JpegDecoder_CreateInstance(IUnknown *pUnkOuter, REFIID iid, void** ppv) DECLSPEC_HIDDEN;
extern HRESULT JpegEncoder_CreateInstance(IUnknown *pUnkOuter, REFIID iid, void** ppv) DECLSPEC_HIDDEN;
extern HRESULT TiffDecoder_CreateInstance(IUnknown *pUnkOuter, REFIID iid, void** ppv) DECLSPEC_HIDDEN;
extern HRESULT TiffEncoder_CreateInstance(IUnknown *pUnkOuter, REFIID iid, void** ppv) DECLSPEC_HIDDEN;
extern HRESULT IcnsEncoder_CreateInstance(IUnknown *pUnkOuter, REFIID iid, void** ppv) DECLSPEC_HIDDEN;

extern HRESULT TgaDecoder_CreateInstance(IUnknown *pUnkOuter, REFIID iid, void** ppv) DECLSPEC_HIDDEN;

extern HRESULT BitmapImpl_Create(UINT uiWidth, UINT uiHeight,
    UINT stride, UINT datasize, BYTE *bits,
    REFWICPixelFormatGUID pixelFormat, WICBitmapCreateCacheOption option,
    IWICBitmap **ppIBitmap) DECLSPEC_HIDDEN;
extern HRESULT BitmapScaler_Create(IWICBitmapScaler **scaler) DECLSPEC_HIDDEN;
extern HRESULT FlipRotator_Create(IWICBitmapFlipRotator **fliprotator) DECLSPEC_HIDDEN;
extern HRESULT PaletteImpl_Create(IWICPalette **palette) DECLSPEC_HIDDEN;
extern HRESULT StreamImpl_Create(IWICStream **stream) DECLSPEC_HIDDEN;
extern HRESULT ColorContext_Create(IWICColorContext **context) DECLSPEC_HIDDEN;
extern HRESULT ColorTransform_Create(IWICColorTransform **transform) DECLSPEC_HIDDEN;
extern HRESULT BitmapClipper_Create(IWICBitmapClipper **clipper) DECLSPEC_HIDDEN;

extern HRESULT copy_pixels(UINT bpp, const BYTE *srcbuffer,
    UINT srcwidth, UINT srcheight, INT srcstride,
    const WICRect *rc, UINT dststride, UINT dstbuffersize, BYTE *dstbuffer) DECLSPEC_HIDDEN;

extern void reverse_bgr8(UINT bytesperpixel, LPBYTE bits, UINT width, UINT height, INT stride) DECLSPEC_HIDDEN;

extern HRESULT get_pixelformat_bpp(const GUID *pixelformat, UINT *bpp) DECLSPEC_HIDDEN;

extern HRESULT CreatePropertyBag2(PROPBAG2 *options, UINT count,
                                  IPropertyBag2 **property) DECLSPEC_HIDDEN;

extern HRESULT CreateComponentInfo(REFCLSID clsid, IWICComponentInfo **ppIInfo) DECLSPEC_HIDDEN;
extern HRESULT CreateComponentEnumerator(DWORD componentTypes, DWORD options, IEnumUnknown **ppIEnumUnknown) DECLSPEC_HIDDEN;

typedef struct BmpDecoder BmpDecoder;

extern HRESULT IcoDibDecoder_CreateInstance(BmpDecoder **ppDecoder) DECLSPEC_HIDDEN;
extern void BmpDecoder_GetWICDecoder(BmpDecoder *This, IWICBitmapDecoder **ppDecoder) DECLSPEC_HIDDEN;
extern void BmpDecoder_FindIconMask(BmpDecoder *This, ULONG *mask_offset, int *topdown) DECLSPEC_HIDDEN;

typedef struct _MetadataItem
{
    PROPVARIANT schema;
    PROPVARIANT id;
    PROPVARIANT value;
} MetadataItem;

typedef struct _MetadataHandlerVtbl
{
    int is_writer;
    const CLSID *clsid;
    HRESULT (*fnLoad)(IStream *stream, const GUID *preferred_vendor,
        DWORD persist_options, MetadataItem **items, DWORD *item_count);
    HRESULT (*fnSave)(IStream *stream, DWORD persist_options,
        const MetadataItem *items, DWORD item_count);
    HRESULT (*fnGetSizeMax)(const MetadataItem *items, DWORD item_count,
        ULARGE_INTEGER *size);
} MetadataHandlerVtbl;

extern HRESULT MetadataReader_Create(const MetadataHandlerVtbl *vtable, IUnknown *pUnkOuter, REFIID iid, void** ppv) DECLSPEC_HIDDEN;

extern HRESULT UnknownMetadataReader_CreateInstance(IUnknown *pUnkOuter, REFIID iid, void** ppv) DECLSPEC_HIDDEN;
extern HRESULT IfdMetadataReader_CreateInstance(IUnknown *pUnkOuter, REFIID iid, void **ppv) DECLSPEC_HIDDEN;
extern HRESULT PngTextReader_CreateInstance(IUnknown *pUnkOuter, REFIID iid, void** ppv) DECLSPEC_HIDDEN;
extern HRESULT LSDReader_CreateInstance(IUnknown *pUnkOuter, REFIID iid, void **ppv) DECLSPEC_HIDDEN;
extern HRESULT IMDReader_CreateInstance(IUnknown *pUnkOuter, REFIID iid, void **ppv) DECLSPEC_HIDDEN;
extern HRESULT GCEReader_CreateInstance(IUnknown *pUnkOuter, REFIID iid, void **ppv) DECLSPEC_HIDDEN;
extern HRESULT APEReader_CreateInstance(IUnknown *pUnkOuter, REFIID iid, void **ppv) DECLSPEC_HIDDEN;
extern HRESULT GifCommentReader_CreateInstance(IUnknown *pUnkOuter, REFIID iid, void **ppv) DECLSPEC_HIDDEN;

extern HRESULT stream_initialize_from_filehandle(IWICStream *iface, HANDLE hfile) DECLSPEC_HIDDEN;

#endif /* WINCODECS_PRIVATE_H */
