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

#include <wine/config.h>
#include <wine/port.h>

#include <stdarg.h>

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#define COBJMACROS
#define NONAMELESSUNION
#define NONAMELESSSTRUCT

#include <windef.h>
#include <winbase.h>
#include <wingdi.h>
#include <winreg.h>
#include <objbase.h>
#include <oleauto.h>
#include <wincodecsdk.h>

#include <wine/debug.h>
#include <wine/library.h>
#include <wine/unicode.h>

WINE_DEFAULT_DEBUG_CHANNEL(wincodecs);

DEFINE_GUID(CLSID_WineTgaDecoder, 0xb11fc79a,0x67cc,0x43e6,0xa9,0xce,0xe3,0xd5,0x49,0x45,0xd3,0x04);

DEFINE_GUID(CLSID_WICIcnsEncoder, 0x312fb6f1,0xb767,0x409d,0x8a,0x6d,0x0f,0xc1,0x54,0xd4,0xf0,0x5c);

DEFINE_GUID(GUID_WineContainerFormatTga, 0x0c44fda1,0xa5c5,0x4298,0x96,0x85,0x47,0x3f,0xc1,0x7c,0xd3,0x22);

DEFINE_GUID(GUID_VendorWine, 0xddf46da1,0x7dc1,0x404e,0x98,0xf2,0xef,0xa4,0x8d,0xfc,0x95,0x0a);

DEFINE_GUID(IID_IMILBitmapSource,0x7543696a,0xbc8d,0x46b0,0x5f,0x81,0x8d,0x95,0x72,0x89,0x72,0xbe);
#define INTERFACE IMILBitmapSource
DECLARE_INTERFACE_(IMILBitmapSource,IUnknown)
{
    /*** IUnknown methods ***/
    STDMETHOD_(HRESULT,QueryInterface)(THIS_ REFIID,void **) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;
    /*** IMILBitmapSource methods ***/
    STDMETHOD_(HRESULT,GetSize)(THIS_ UINT *,UINT *);
    STDMETHOD_(HRESULT,GetPixelFormat)(THIS_ int *);
    STDMETHOD_(HRESULT,GetResolution)(THIS_ double *,double *);
    STDMETHOD_(HRESULT,CopyPalette)(THIS_ IWICPalette *);
    STDMETHOD_(HRESULT,CopyPixels)(THIS_ const WICRect *,UINT,UINT,BYTE *);
    STDMETHOD_(HRESULT,UnknownMethod1)(THIS_ void **);
};
#undef INTERFACE

#define INTERFACE IMILUnknown1
DECLARE_INTERFACE_(IMILUnknown1,IUnknown)
{
    /*** IUnknown methods ***/
    STDMETHOD_(HRESULT,QueryInterface)(THIS_ REFIID,void **) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;
};
#undef INTERFACE

#define INTERFACE IMILUnknown2
DECLARE_INTERFACE_(IMILUnknown2,IUnknown)
{
    /*** IUnknown methods ***/
    STDMETHOD_(HRESULT,QueryInterface)(THIS_ REFIID,void **) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;
    /*** unknown methods ***/
    STDMETHOD_(HRESULT,UnknownMethod1)(THIS_ void *, void *) PURE;
};
#undef INTERFACE

HRESULT create_instance(CLSID *clsid, const IID *iid, void **ppv) DECLSPEC_HIDDEN;

typedef HRESULT(*class_constructor)(REFIID,void**);
extern HRESULT FormatConverter_CreateInstance(REFIID riid, void** ppv) DECLSPEC_HIDDEN;
extern HRESULT ComponentFactory_CreateInstance(REFIID riid, void** ppv) DECLSPEC_HIDDEN;
extern HRESULT BmpDecoder_CreateInstance(REFIID riid, void** ppv) DECLSPEC_HIDDEN;
extern HRESULT PngDecoder_CreateInstance(REFIID iid, void** ppv) DECLSPEC_HIDDEN;
extern HRESULT PngEncoder_CreateInstance(REFIID iid, void** ppv) DECLSPEC_HIDDEN;
extern HRESULT BmpEncoder_CreateInstance(REFIID iid, void** ppv) DECLSPEC_HIDDEN;
extern HRESULT DibDecoder_CreateInstance(REFIID iid, void** ppv) DECLSPEC_HIDDEN;
extern HRESULT GifDecoder_CreateInstance(REFIID riid, void** ppv) DECLSPEC_HIDDEN;
extern HRESULT IcoDecoder_CreateInstance(REFIID iid, void** ppv) DECLSPEC_HIDDEN;
extern HRESULT JpegDecoder_CreateInstance(REFIID iid, void** ppv) DECLSPEC_HIDDEN;
extern HRESULT JpegEncoder_CreateInstance(REFIID iid, void** ppv) DECLSPEC_HIDDEN;
extern HRESULT TiffDecoder_CreateInstance(REFIID iid, void** ppv) DECLSPEC_HIDDEN;
extern HRESULT TiffEncoder_CreateInstance(REFIID iid, void** ppv) DECLSPEC_HIDDEN;
extern HRESULT IcnsEncoder_CreateInstance(REFIID iid, void** ppv) DECLSPEC_HIDDEN;
extern HRESULT TgaDecoder_CreateInstance(REFIID iid, void** ppv) DECLSPEC_HIDDEN;

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

extern HRESULT configure_write_source(IWICBitmapFrameEncode *iface,
    IWICBitmapSource *source, const WICRect *prc,
    const WICPixelFormatGUID *format,
    INT width, INT height, double xres, double yres) DECLSPEC_HIDDEN;

extern HRESULT write_source(IWICBitmapFrameEncode *iface,
    IWICBitmapSource *source, const WICRect *prc,
    const WICPixelFormatGUID *format, UINT bpp,
    INT width, INT height) DECLSPEC_HIDDEN;

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

extern HRESULT MetadataReader_Create(const MetadataHandlerVtbl *vtable, REFIID iid, void** ppv) DECLSPEC_HIDDEN;

extern HRESULT UnknownMetadataReader_CreateInstance(REFIID iid, void** ppv) DECLSPEC_HIDDEN;
extern HRESULT IfdMetadataReader_CreateInstance(REFIID iid, void **ppv) DECLSPEC_HIDDEN;
extern HRESULT PngGamaReader_CreateInstance(REFIID iid, void** ppv) DECLSPEC_HIDDEN;
extern HRESULT PngTextReader_CreateInstance(REFIID iid, void** ppv) DECLSPEC_HIDDEN;
extern HRESULT LSDReader_CreateInstance(REFIID iid, void **ppv) DECLSPEC_HIDDEN;
extern HRESULT IMDReader_CreateInstance(REFIID iid, void **ppv) DECLSPEC_HIDDEN;
extern HRESULT GCEReader_CreateInstance(REFIID iid, void **ppv) DECLSPEC_HIDDEN;
extern HRESULT APEReader_CreateInstance(REFIID iid, void **ppv) DECLSPEC_HIDDEN;
extern HRESULT GifCommentReader_CreateInstance(REFIID iid, void **ppv) DECLSPEC_HIDDEN;

extern HRESULT stream_initialize_from_filehandle(IWICStream *iface, HANDLE hfile) DECLSPEC_HIDDEN;

#endif /* WINCODECS_PRIVATE_H */
