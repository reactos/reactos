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

#include "wincodec.h"
#include "wincodecsdk.h"

#include "wine/debug.h"

#ifdef __REACTOS__
#include <wine/library.h>
#endif

DEFINE_GUID(CLSID_WineTgaDecoder, 0xb11fc79a,0x67cc,0x43e6,0xa9,0xce,0xe3,0xd5,0x49,0x45,0xd3,0x04);

DEFINE_GUID(GUID_WineContainerFormatTga, 0x0c44fda1,0xa5c5,0x4298,0x96,0x85,0x47,0x3f,0xc1,0x7c,0xd3,0x22);

DEFINE_GUID(GUID_VendorWine, 0xddf46da1,0x7dc1,0x404e,0x98,0xf2,0xef,0xa4,0x8d,0xfc,0x95,0x0a);

#ifdef __REACTOS__
extern const GUID IID_IMILBitmap;
extern const GUID IID_IMILBitmapSource;
extern const GUID IID_IMILBitmapLock;
extern const GUID IID_IMILBitmapScaler;
extern const GUID IID_IMILFormatConverter;
extern const GUID IID_IMILPalette;
#else
extern IID IID_IMILBitmap;
extern IID IID_IMILBitmapSource;
extern IID IID_IMILBitmapLock;
extern IID IID_IMILBitmapScaler;
extern IID IID_IMILFormatConverter;
extern IID IID_IMILPalette;
#endif

#define INTERFACE IMILBitmapSource
DECLARE_INTERFACE_(IMILBitmapSource,IUnknown)
{
    /*** IUnknown methods ***/
    STDMETHOD_(HRESULT,QueryInterface)(THIS_ REFIID,void **) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;
    /*** IWICBitmapSource methods ***/
    STDMETHOD_(HRESULT,GetSize)(THIS_ UINT *,UINT *) PURE;
    STDMETHOD_(HRESULT,GetPixelFormat)(THIS_ int *) PURE;
    STDMETHOD_(HRESULT,GetResolution)(THIS_ double *,double *) PURE;
    STDMETHOD_(HRESULT,CopyPalette)(THIS_ IWICPalette *) PURE;
    STDMETHOD_(HRESULT,CopyPixels)(THIS_ const WICRect *,UINT,UINT,BYTE *) PURE;
};
#undef INTERFACE

#define INTERFACE IMILBitmap
DECLARE_INTERFACE_(IMILBitmap,IMILBitmapSource)
{
    /*** IUnknown methods ***/
    STDMETHOD_(HRESULT,QueryInterface)(THIS_ REFIID,void **) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;
    /*** IWICBitmapSource methods ***/
    STDMETHOD_(HRESULT,GetSize)(THIS_ UINT *,UINT *) PURE;
    STDMETHOD_(HRESULT,GetPixelFormat)(THIS_ int *) PURE;
    STDMETHOD_(HRESULT,GetResolution)(THIS_ double *,double *) PURE;
    STDMETHOD_(HRESULT,CopyPalette)(THIS_ IWICPalette *) PURE;
    STDMETHOD_(HRESULT,CopyPixels)(THIS_ const WICRect *,UINT,UINT,BYTE *) PURE;
    /*** IMILBitmap methods ***/
    STDMETHOD_(HRESULT,unknown1)(THIS_ void **) PURE;
    STDMETHOD_(HRESULT,Lock)(THIS_ const WICRect *,DWORD,IWICBitmapLock **) PURE;
    STDMETHOD_(HRESULT,Unlock)(THIS_ IWICBitmapLock *) PURE;
    STDMETHOD_(HRESULT,SetPalette)(THIS_ IWICPalette *) PURE;
    STDMETHOD_(HRESULT,SetResolution)(THIS_ double,double) PURE;
    STDMETHOD_(HRESULT,AddDirtyRect)(THIS_ const WICRect *) PURE;
};
#undef INTERFACE

#define INTERFACE IMILBitmapScaler
DECLARE_INTERFACE_(IMILBitmapScaler,IMILBitmapSource)
{
    /*** IUnknown methods ***/
    STDMETHOD_(HRESULT,QueryInterface)(THIS_ REFIID,void **) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;
    /*** IWICBitmapSource methods ***/
    STDMETHOD_(HRESULT,GetSize)(THIS_ UINT *,UINT *) PURE;
    STDMETHOD_(HRESULT,GetPixelFormat)(THIS_ int *) PURE;
    STDMETHOD_(HRESULT,GetResolution)(THIS_ double *,double *) PURE;
    STDMETHOD_(HRESULT,CopyPalette)(THIS_ IWICPalette *) PURE;
    STDMETHOD_(HRESULT,CopyPixels)(THIS_ const WICRect *,UINT,UINT,BYTE *) PURE;
    /*** IMILBitmapScaler methods ***/
    STDMETHOD_(HRESULT,unknown1)(THIS_ void **) PURE;
    STDMETHOD_(HRESULT,Initialize)(THIS_ IMILBitmapSource *,UINT,UINT,WICBitmapInterpolationMode) PURE;
};
#undef INTERFACE

#define THISCALLMETHOD_(type,method)  type (__thiscall *method)

#define INTERFACE IMILUnknown1
DECLARE_INTERFACE_(IMILUnknown1,IUnknown)
{
    /*** IUnknown methods ***/
    STDMETHOD_(HRESULT,QueryInterface)(THIS_ REFIID,void **) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;
    /*** thiscall method ***/
    #ifdef __REACTOS__
    STDMETHOD_(void,unknown1)(THIS_ void*) PURE;
    #else
    THISCALLMETHOD_(void,unknown1)(THIS_ void*) PURE;
    #endif
    STDMETHOD_(HRESULT,unknown2)(THIS_ void*, void*) PURE;
    #ifdef __REACTOS__
    STDMETHOD_(HRESULT,unknown3)(THIS_ void*) PURE;
    #else
    THISCALLMETHOD_(HRESULT,unknown3)(THIS_ void*) PURE;
    #endif
    STDMETHOD_(HRESULT,unknown4)(THIS_ void*) PURE;
    STDMETHOD_(HRESULT,unknown5)(THIS_ void*) PURE;
    STDMETHOD_(HRESULT,unknown6)(THIS_ DWORD64) PURE;
    STDMETHOD_(HRESULT,unknown7)(THIS_ void*) PURE;
    #ifdef __REACTOS__
    /*** thiscall method ***/
    STDMETHOD_(HRESULT,unknown8)(THIS) PURE;
    #else
    THISCALLMETHOD_(HRESULT,unknown8)(THIS) PURE;
    #endif
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
    STDMETHOD_(HRESULT,unknown1)(THIS_ void *,void **) PURE;
    STDMETHOD_(HRESULT,unknown2)(THIS_ void *,void *) PURE;
    STDMETHOD_(HRESULT,unknown3)(THIS_ void *) PURE;
};
#undef INTERFACE

HRESULT create_instance(const CLSID *clsid, const IID *iid, void **ppv);

typedef HRESULT(*class_constructor)(REFIID,void**);
extern HRESULT FormatConverter_CreateInstance(REFIID riid, void** ppv);
extern HRESULT ImagingFactory_CreateInstance(REFIID riid, void** ppv);
extern HRESULT BmpDecoder_CreateInstance(REFIID riid, void** ppv);
extern HRESULT PngDecoder_CreateInstance(REFIID iid, void** ppv);
extern HRESULT PngEncoder_CreateInstance(REFIID iid, void** ppv);
extern HRESULT BmpEncoder_CreateInstance(REFIID iid, void** ppv);
extern HRESULT DibDecoder_CreateInstance(REFIID iid, void** ppv);
extern HRESULT GifDecoder_CreateInstance(REFIID riid, void** ppv);
extern HRESULT GifEncoder_CreateInstance(REFIID iid, void** ppv);
extern HRESULT IcoDecoder_CreateInstance(REFIID iid, void** ppv);
extern HRESULT JpegDecoder_CreateInstance(REFIID iid, void** ppv);
extern HRESULT JpegEncoder_CreateInstance(REFIID iid, void** ppv);
extern HRESULT TiffDecoder_CreateInstance(REFIID iid, void** ppv);
extern HRESULT TiffEncoder_CreateInstance(REFIID iid, void** ppv);
extern HRESULT TgaDecoder_CreateInstance(REFIID iid, void** ppv);
extern HRESULT DdsDecoder_CreateInstance(REFIID iid, void** ppv);
extern HRESULT DdsEncoder_CreateInstance(REFIID iid, void** ppv);

extern HRESULT BitmapImpl_Create(UINT uiWidth, UINT uiHeight,
    UINT stride, UINT datasize, void *view, UINT offset,
    REFWICPixelFormatGUID pixelFormat, WICBitmapCreateCacheOption option,
    IWICBitmap **ppIBitmap);
extern HRESULT BitmapScaler_Create(IWICBitmapScaler **scaler);
extern HRESULT FlipRotator_Create(IWICBitmapFlipRotator **fliprotator);
extern HRESULT PaletteImpl_Create(IWICPalette **palette);
extern HRESULT StreamImpl_Create(IWICStream **stream);
extern HRESULT ColorContext_Create(IWICColorContext **context);
extern HRESULT ColorTransform_Create(IWICColorTransform **transform);
extern HRESULT BitmapClipper_Create(IWICBitmapClipper **clipper);

extern HRESULT copy_pixels(UINT bpp, const BYTE *srcbuffer,
    UINT srcwidth, UINT srcheight, INT srcstride,
    const WICRect *rc, UINT dststride, UINT dstbuffersize, BYTE *dstbuffer);

extern HRESULT configure_write_source(IWICBitmapFrameEncode *iface,
    IWICBitmapSource *source, const WICRect *prc,
    const WICPixelFormatGUID *format,
    INT width, INT height, double xres, double yres);

extern HRESULT write_source(IWICBitmapFrameEncode *iface,
    IWICBitmapSource *source, const WICRect *prc,
    const WICPixelFormatGUID *format, UINT bpp, BOOL need_palette,
    INT width, INT height);

extern void reverse_bgr8(UINT bytesperpixel, LPBYTE bits, UINT width, UINT height, INT stride);

extern HRESULT get_pixelformat_bpp(const GUID *pixelformat, UINT *bpp);

extern HRESULT CreatePropertyBag2(const PROPBAG2 *options, UINT count,
                                  IPropertyBag2 **property);

extern HRESULT CreateComponentInfo(REFCLSID clsid, IWICComponentInfo **ppIInfo);
extern void ReleaseComponentInfos(void);
extern HRESULT CreateComponentEnumerator(DWORD componentTypes, DWORD options, IEnumUnknown **ppIEnumUnknown);
extern HRESULT get_decoder_info(REFCLSID clsid, IWICBitmapDecoderInfo **info);

typedef struct BmpDecoder BmpDecoder;

extern HRESULT IcoDibDecoder_CreateInstance(BmpDecoder **ppDecoder);
extern void BmpDecoder_GetWICDecoder(BmpDecoder *This, IWICBitmapDecoder **ppDecoder);
extern void BmpDecoder_FindIconMask(BmpDecoder *This, ULONG *mask_offset, int *topdown);

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

extern HRESULT MetadataReader_Create(const MetadataHandlerVtbl *vtable, REFIID iid, void** ppv);

extern HRESULT UnknownMetadataReader_CreateInstance(REFIID iid, void** ppv);
extern HRESULT IfdMetadataReader_CreateInstance(REFIID iid, void **ppv);
extern HRESULT PngChrmReader_CreateInstance(REFIID iid, void** ppv);
extern HRESULT PngGamaReader_CreateInstance(REFIID iid, void** ppv);
extern HRESULT PngHistReader_CreateInstance(REFIID iid, void** ppv);
extern HRESULT PngTextReader_CreateInstance(REFIID iid, void** ppv);
extern HRESULT PngTimeReader_CreateInstance(REFIID iid, void** ppv);
extern HRESULT LSDReader_CreateInstance(REFIID iid, void **ppv);
extern HRESULT IMDReader_CreateInstance(REFIID iid, void **ppv);
extern HRESULT GCEReader_CreateInstance(REFIID iid, void **ppv);
extern HRESULT APEReader_CreateInstance(REFIID iid, void **ppv);
extern HRESULT GifCommentReader_CreateInstance(REFIID iid, void **ppv);
extern HRESULT MetadataQueryReader_CreateInstance(IWICMetadataBlockReader *, const WCHAR *, IWICMetadataQueryReader **);
extern HRESULT MetadataQueryWriter_CreateInstance(IWICMetadataBlockWriter *, const WCHAR *, IWICMetadataQueryWriter **);
extern HRESULT stream_initialize_from_filehandle(IWICStream *iface, HANDLE hfile);

static inline const char *debug_wic_rect(const WICRect *rect)
{
    if (!rect) return "(null)";
    return wine_dbg_sprintf("(%u,%u)-(%u,%u)", rect->X, rect->Y, rect->Width, rect->Height);
}

extern HMODULE windowscodecs_module;

HRESULT read_png_chunk(IStream *stream, BYTE *type, BYTE **data, ULONG *data_size);

struct decoder_funcs;

struct decoder_info
{
    GUID container_format;
    GUID block_format;
    CLSID clsid;
};

#define DECODER_FLAGS_CAPABILITY_MASK 0x1f
#define DECODER_FLAGS_UNSUPPORTED_COLOR_CONTEXT 0x80000000

struct decoder_stat
{
    DWORD flags;
    UINT frame_count;
};

struct decoder_frame
{
    CLSID pixel_format;
    UINT width, height;
    UINT bpp;
    DOUBLE dpix, dpiy;
    DWORD num_color_contexts;
    DWORD num_colors;
    WICColor palette[256];
};

#define DECODER_BLOCK_OPTION_MASK 0x0001000F
#define DECODER_BLOCK_FULL_STREAM 0x80000000
#define DECODER_BLOCK_READER_CLSID 0x40000000
struct decoder_block
{
    ULONGLONG offset;
    ULONGLONG length;
    DWORD options;
    GUID reader_clsid;
};

struct decoder
{
    const struct decoder_funcs *vtable;
};

struct decoder_funcs
{
    HRESULT (CDECL *initialize)(struct decoder* This, IStream *stream, struct decoder_stat *st);
    HRESULT (CDECL *get_frame_info)(struct decoder* This, UINT frame, struct decoder_frame *info);
    HRESULT (CDECL *copy_pixels)(struct decoder* This, UINT frame, const WICRect *prc,
        UINT stride, UINT buffersize, BYTE *buffer);
    HRESULT (CDECL *get_metadata_blocks)(struct decoder* This, UINT frame, UINT *count,
        struct decoder_block **blocks);
    HRESULT (CDECL *get_color_context)(struct decoder* This, UINT frame, UINT num,
        BYTE **data, DWORD *datasize);
    void (CDECL *destroy)(struct decoder* This);
};

HRESULT CDECL stream_getsize(IStream *stream, ULONGLONG *size);
HRESULT CDECL stream_read(IStream *stream, void *buffer, ULONG read, ULONG *bytes_read);
HRESULT CDECL stream_seek(IStream *stream, LONGLONG ofs, DWORD origin, ULONGLONG *new_position);
HRESULT CDECL stream_write(IStream *stream, const void *buffer, ULONG write, ULONG *bytes_written);

HRESULT CDECL decoder_create(const CLSID *decoder_clsid, struct decoder_info *info, struct decoder **result);
HRESULT CDECL decoder_initialize(struct decoder *This, IStream *stream, struct decoder_stat *st);
HRESULT CDECL decoder_get_frame_info(struct decoder* This, UINT frame, struct decoder_frame *info);
HRESULT CDECL decoder_copy_pixels(struct decoder* This, UINT frame, const WICRect *prc,
    UINT stride, UINT buffersize, BYTE *buffer);
HRESULT CDECL decoder_get_metadata_blocks(struct decoder* This, UINT frame, UINT *count,
    struct decoder_block **blocks);
HRESULT CDECL decoder_get_color_context(struct decoder* This, UINT frame, UINT num,
    BYTE **data, DWORD *datasize);
void CDECL decoder_destroy(struct decoder *This);

struct encoder_funcs;

/* sync with encoder_option_properties */
enum encoder_option
{
    ENCODER_OPTION_INTERLACE,
    ENCODER_OPTION_FILTER,
    ENCODER_OPTION_COMPRESSION_METHOD,
    ENCODER_OPTION_COMPRESSION_QUALITY,
    ENCODER_OPTION_IMAGE_QUALITY,
    ENCODER_OPTION_BITMAP_TRANSFORM,
    ENCODER_OPTION_LUMINANCE,
    ENCODER_OPTION_CHROMINANCE,
    ENCODER_OPTION_YCRCB_SUBSAMPLING,
    ENCODER_OPTION_SUPPRESS_APP0,
    ENCODER_OPTION_END
};

#define ENCODER_FLAGS_MULTI_FRAME 0x1
#define ENCODER_FLAGS_ICNS_SIZE 0x2
#define ENCODER_FLAGS_SUPPORTS_METADATA 0x4

struct encoder_info
{
    DWORD flags;
    GUID container_format;
    CLSID clsid;
    DWORD encoder_options[7];
};

struct encoder_frame
{
    GUID pixel_format;
    UINT width, height;
    UINT bpp;
    BOOL indexed;
    DOUBLE dpix, dpiy;
    UINT num_colors;
    WICColor palette[256];
    /* encoder options */
    BOOL interlace;
    DWORD filter;
};

struct encoder
{
    const struct encoder_funcs *vtable;
};

struct encoder_funcs
{
    HRESULT (CDECL *initialize)(struct encoder* This, IStream *stream);
    HRESULT (CDECL *get_supported_format)(struct encoder* This, GUID *pixel_format, DWORD *bpp, BOOL *indexed);
    HRESULT (CDECL *create_frame)(struct encoder* This, const struct encoder_frame *frame);
    HRESULT (CDECL *write_lines)(struct encoder* This, BYTE *data, DWORD line_count, DWORD stride);
    HRESULT (CDECL *commit_frame)(struct encoder* This);
    HRESULT (CDECL *commit_file)(struct encoder* This);
    void (CDECL *destroy)(struct encoder* This);
};

HRESULT CDECL encoder_initialize(struct encoder* This, IStream *stream);
HRESULT CDECL encoder_get_supported_format(struct encoder* This, GUID *pixel_format, DWORD *bpp, BOOL *indexed);
HRESULT CDECL encoder_create_frame(struct encoder* This, const struct encoder_frame *frame);
HRESULT CDECL encoder_write_lines(struct encoder* This, BYTE *data, DWORD line_count, DWORD stride);
HRESULT CDECL encoder_commit_frame(struct encoder* This);
HRESULT CDECL encoder_commit_file(struct encoder* This);
void CDECL encoder_destroy(struct encoder* This);

HRESULT CDECL png_decoder_create(struct decoder_info *info, struct decoder **result);
HRESULT CDECL tiff_decoder_create(struct decoder_info *info, struct decoder **result);
HRESULT CDECL jpeg_decoder_create(struct decoder_info *info, struct decoder **result);

HRESULT CDECL png_encoder_create(struct encoder_info *info, struct encoder **result);
HRESULT CDECL tiff_encoder_create(struct encoder_info *info, struct encoder **result);
HRESULT CDECL jpeg_encoder_create(struct encoder_info *info, struct encoder **result);
HRESULT CDECL icns_encoder_create(struct encoder_info *info, struct encoder **result);

extern HRESULT CommonDecoder_CreateInstance(struct decoder *decoder,
    const struct decoder_info *decoder_info, REFIID iid, void** ppv);

extern HRESULT CommonEncoder_CreateInstance(struct encoder *encoder,
    const struct encoder_info *encoder_info, REFIID iid, void** ppv);

#endif /* WINCODECS_PRIVATE_H */
