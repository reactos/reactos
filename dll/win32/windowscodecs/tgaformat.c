/*
 * Copyright 2010 Vincent Povirk for CodeWeavers
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

#include "pshpack1.h"

typedef struct {
    BYTE id_length;
    BYTE colormap_type;
    BYTE image_type;
    /* Colormap Specification */
    WORD colormap_firstentry;
    WORD colormap_length;
    BYTE colormap_entrysize;
    /* Image Specification */
    WORD xorigin;
    WORD yorigin;
    WORD width;
    WORD height;
    BYTE depth;
    BYTE image_descriptor;
} tga_header;

#define IMAGETYPE_COLORMAPPED 1
#define IMAGETYPE_TRUECOLOR 2
#define IMAGETYPE_GRAYSCALE 3
#define IMAGETYPE_RLE 8

#define IMAGE_ATTRIBUTE_BITCOUNT_MASK 0xf
#define IMAGE_RIGHTTOLEFT 0x10
#define IMAGE_TOPTOBOTTOM 0x20

typedef struct {
    DWORD extension_area_offset;
    DWORD developer_directory_offset;
    char magic[18];
} tga_footer;

static const BYTE tga_footer_magic[18] = "TRUEVISION-XFILE.";

typedef struct {
    WORD size;
    char author_name[41];
    char author_comments[324];
    WORD timestamp[6];
    char job_name[41];
    WORD job_timestamp[6];
    char software_id[41];
    WORD software_version;
    char software_version_letter;
    DWORD key_color;
    WORD pixel_width;
    WORD pixel_height;
    WORD gamma_numerator;
    WORD gamma_denominator;
    DWORD color_correction_offset;
    DWORD thumbnail_offset;
    DWORD scanline_offset;
    BYTE attributes_type;
} tga_extension_area;

#define ATTRIBUTE_NO_ALPHA 0
#define ATTRIBUTE_UNDEFINED 1
#define ATTRIBUTE_UNDEFINED_PRESERVE 2
#define ATTRIBUTE_ALPHA 3
#define ATTRIBUTE_PALPHA 4

#include "poppack.h"

typedef struct {
    IWICBitmapDecoder IWICBitmapDecoder_iface;
    IWICBitmapFrameDecode IWICBitmapFrameDecode_iface;
    LONG ref;
    BOOL initialized;
    IStream *stream;
    tga_header header;
    tga_extension_area extension_area;
    BYTE *imagebits;
    BYTE *origin;
    int stride;
    ULONG id_offset;
    ULONG colormap_length;
    ULONG colormap_offset;
    ULONG image_offset;
    ULONG extension_area_offset;
    ULONG developer_directory_offset;
    CRITICAL_SECTION lock;
} TgaDecoder;

static inline TgaDecoder *impl_from_IWICBitmapDecoder(IWICBitmapDecoder *iface)
{
    return CONTAINING_RECORD(iface, TgaDecoder, IWICBitmapDecoder_iface);
}

static inline TgaDecoder *impl_from_IWICBitmapFrameDecode(IWICBitmapFrameDecode *iface)
{
    return CONTAINING_RECORD(iface, TgaDecoder, IWICBitmapFrameDecode_iface);
}

static HRESULT WINAPI TgaDecoder_QueryInterface(IWICBitmapDecoder *iface, REFIID iid,
    void **ppv)
{
    TgaDecoder *This = impl_from_IWICBitmapDecoder(iface);
    TRACE("(%p,%s,%p)\n", iface, debugstr_guid(iid), ppv);

    if (!ppv) return E_INVALIDARG;

    if (IsEqualIID(&IID_IUnknown, iid) || IsEqualIID(&IID_IWICBitmapDecoder, iid))
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

static ULONG WINAPI TgaDecoder_AddRef(IWICBitmapDecoder *iface)
{
    TgaDecoder *This = impl_from_IWICBitmapDecoder(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) refcount=%lu\n", iface, ref);

    return ref;
}

static ULONG WINAPI TgaDecoder_Release(IWICBitmapDecoder *iface)
{
    TgaDecoder *This = impl_from_IWICBitmapDecoder(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) refcount=%lu\n", iface, ref);

    if (ref == 0)
    {
        This->lock.DebugInfo->Spare[0] = 0;
        DeleteCriticalSection(&This->lock);
        if (This->stream)
            IStream_Release(This->stream);
        free(This->imagebits);
        free(This);
    }

    return ref;
}

static HRESULT WINAPI TgaDecoder_QueryCapability(IWICBitmapDecoder *iface, IStream *stream,
    DWORD *capability)
{
    HRESULT hr;

    TRACE("(%p,%p,%p)\n", iface, stream, capability);

    if (!stream || !capability) return E_INVALIDARG;

    hr = IWICBitmapDecoder_Initialize(iface, stream, WICDecodeMetadataCacheOnDemand);
    if (hr != S_OK) return hr;

    *capability = WICBitmapDecoderCapabilityCanDecodeAllImages |
                  WICBitmapDecoderCapabilityCanDecodeSomeImages;
    return S_OK;
}

static HRESULT WINAPI TgaDecoder_Initialize(IWICBitmapDecoder *iface, IStream *pIStream,
    WICDecodeOptions cacheOptions)
{
    TgaDecoder *This = impl_from_IWICBitmapDecoder(iface);
    HRESULT hr=S_OK;
    DWORD bytesread;
    LARGE_INTEGER seek;
    tga_footer footer;
    int attribute_bitcount;
    int mapped_depth=0;

    TRACE("(%p,%p,%u)\n", iface, pIStream, cacheOptions);

    EnterCriticalSection(&This->lock);

    if (This->initialized)
    {
        hr = WINCODEC_ERR_WRONGSTATE;
        goto end;
    }

    seek.QuadPart = 0;
    hr = IStream_Seek(pIStream, seek, STREAM_SEEK_SET, NULL);
    if (FAILED(hr)) goto end;

    hr = IStream_Read(pIStream, &This->header, sizeof(tga_header), &bytesread);
    if (SUCCEEDED(hr) && bytesread != sizeof(tga_header))
    {
        TRACE("got only %lu bytes\n", bytesread);
        hr = E_FAIL;
    }
    if (FAILED(hr)) goto end;

    TRACE("imagetype=%u, colormap type=%u, depth=%u, image descriptor=0x%x\n",
        This->header.image_type, This->header.colormap_type,
        This->header.depth, This->header.image_descriptor);

    /* Sanity checking. Since TGA has no clear identifying markers, we need
     * to be careful to not load a non-TGA image. */
    switch (This->header.image_type)
    {
    case IMAGETYPE_COLORMAPPED:
    case IMAGETYPE_COLORMAPPED|IMAGETYPE_RLE:
        if (This->header.colormap_type != 1)
            hr = E_FAIL;
        mapped_depth = This->header.colormap_entrysize;
        break;
    case IMAGETYPE_TRUECOLOR:
    case IMAGETYPE_TRUECOLOR|IMAGETYPE_RLE:
        if (This->header.colormap_type != 0 && This->header.colormap_type != 1)
            hr = E_FAIL;
        mapped_depth = This->header.depth;
        break;
    case IMAGETYPE_GRAYSCALE:
    case IMAGETYPE_GRAYSCALE|IMAGETYPE_RLE:
        if (This->header.colormap_type != 0)
            hr = E_FAIL;
        mapped_depth = 0;
        break;
    default:
        hr = E_FAIL;
    }

    if (This->header.depth != 8 && This->header.depth != 16 &&
        This->header.depth != 24 && This->header.depth != 32)
        hr = E_FAIL;

    if ((This->header.image_descriptor & 0xc0) != 0)
        hr = E_FAIL;

    attribute_bitcount = This->header.image_descriptor & IMAGE_ATTRIBUTE_BITCOUNT_MASK;

    if (attribute_bitcount &&
        !((mapped_depth == 32 && attribute_bitcount == 8) ||
          (mapped_depth == 16 && attribute_bitcount == 1)))
        hr = E_FAIL;

    if (FAILED(hr))
    {
        WARN("bad tga header\n");
        goto end;
    }

    /* Locate data in the file based on the header. */
    This->id_offset = sizeof(tga_header);
    This->colormap_offset = This->id_offset + This->header.id_length;
    if (This->header.colormap_type == 1)
        This->colormap_length = ((This->header.colormap_entrysize+7)/8) * This->header.colormap_length;
    else
        This->colormap_length = 0;
    This->image_offset = This->colormap_offset + This->colormap_length;

    /* Read footer if there is one */
    seek.QuadPart = -(LONGLONG)sizeof(tga_footer);
    hr = IStream_Seek(pIStream, seek, STREAM_SEEK_END, NULL);

    if (SUCCEEDED(hr)) {
        hr = IStream_Read(pIStream, &footer, sizeof(tga_footer), &bytesread);
        if (SUCCEEDED(hr) && bytesread != sizeof(tga_footer))
        {
            TRACE("got only %lu footer bytes\n", bytesread);
            hr = E_FAIL;
        }

        if (memcmp(footer.magic, tga_footer_magic, sizeof(tga_footer_magic)) == 0)
        {
            This->extension_area_offset = footer.extension_area_offset;
            This->developer_directory_offset = footer.developer_directory_offset;
        }
        else
        {
            This->extension_area_offset = 0;
            This->developer_directory_offset = 0;
        }
    }
    else
    {
        /* File is too small to have a footer. */
        This->extension_area_offset = 0;
        This->developer_directory_offset = 0;
        hr = S_OK;
    }

    if (This->extension_area_offset)
    {
        seek.QuadPart = This->extension_area_offset;
        hr = IStream_Seek(pIStream, seek, STREAM_SEEK_SET, NULL);
        if (FAILED(hr)) goto end;

        hr = IStream_Read(pIStream, &This->extension_area, sizeof(tga_extension_area), &bytesread);
        if (SUCCEEDED(hr) && bytesread != sizeof(tga_extension_area))
        {
            TRACE("got only %lu extension area bytes\n", bytesread);
            hr = E_FAIL;
        }
        if (SUCCEEDED(hr) && This->extension_area.size < 495)
        {
            TRACE("extension area is only %u bytes long\n", This->extension_area.size);
            hr = E_FAIL;
        }
        if (FAILED(hr)) goto end;
    }

    IStream_AddRef(pIStream);
    This->stream = pIStream;
    This->initialized = TRUE;

end:
    LeaveCriticalSection(&This->lock);
    return hr;
}

static HRESULT WINAPI TgaDecoder_GetContainerFormat(IWICBitmapDecoder *iface,
    GUID *pguidContainerFormat)
{
    memcpy(pguidContainerFormat, &GUID_WineContainerFormatTga, sizeof(GUID));
    return S_OK;
}

static HRESULT WINAPI TgaDecoder_GetDecoderInfo(IWICBitmapDecoder *iface,
    IWICBitmapDecoderInfo **ppIDecoderInfo)
{
    TRACE("(%p,%p)\n", iface, ppIDecoderInfo);

    return get_decoder_info(&CLSID_WineTgaDecoder, ppIDecoderInfo);
}

static HRESULT WINAPI TgaDecoder_CopyPalette(IWICBitmapDecoder *iface,
    IWICPalette *pIPalette)
{
    FIXME("(%p,%p): stub\n", iface, pIPalette);
    return E_NOTIMPL;
}

static HRESULT WINAPI TgaDecoder_GetMetadataQueryReader(IWICBitmapDecoder *iface,
    IWICMetadataQueryReader **ppIMetadataQueryReader)
{
    FIXME("(%p,%p): stub\n", iface, ppIMetadataQueryReader);
    return E_NOTIMPL;
}

static HRESULT WINAPI TgaDecoder_GetPreview(IWICBitmapDecoder *iface,
    IWICBitmapSource **ppIBitmapSource)
{
    FIXME("(%p,%p): stub\n", iface, ppIBitmapSource);
    return WINCODEC_ERR_UNSUPPORTEDOPERATION;
}

static HRESULT WINAPI TgaDecoder_GetColorContexts(IWICBitmapDecoder *iface,
    UINT cCount, IWICColorContext **ppIColorContexts, UINT *pcActualCount)
{
    FIXME("(%p,%u,%p,%p): stub\n", iface, cCount, ppIColorContexts, pcActualCount);
    return WINCODEC_ERR_UNSUPPORTEDOPERATION;
}

static HRESULT WINAPI TgaDecoder_GetThumbnail(IWICBitmapDecoder *iface,
    IWICBitmapSource **ppIThumbnail)
{
    FIXME("(%p,%p): stub\n", iface, ppIThumbnail);
    return WINCODEC_ERR_CODECNOTHUMBNAIL;
}

static HRESULT WINAPI TgaDecoder_GetFrameCount(IWICBitmapDecoder *iface,
    UINT *pCount)
{
    if (!pCount) return E_INVALIDARG;

    *pCount = 1;
    return S_OK;
}

static HRESULT WINAPI TgaDecoder_GetFrame(IWICBitmapDecoder *iface,
    UINT index, IWICBitmapFrameDecode **ppIBitmapFrame)
{
    TgaDecoder *This = impl_from_IWICBitmapDecoder(iface);
    TRACE("(%p,%p)\n", iface, ppIBitmapFrame);

    if (!This->initialized) return WINCODEC_ERR_FRAMEMISSING;

    if (index != 0) return E_INVALIDARG;

    IWICBitmapDecoder_AddRef(iface);
    *ppIBitmapFrame = &This->IWICBitmapFrameDecode_iface;

    return S_OK;
}

static const IWICBitmapDecoderVtbl TgaDecoder_Vtbl = {
    TgaDecoder_QueryInterface,
    TgaDecoder_AddRef,
    TgaDecoder_Release,
    TgaDecoder_QueryCapability,
    TgaDecoder_Initialize,
    TgaDecoder_GetContainerFormat,
    TgaDecoder_GetDecoderInfo,
    TgaDecoder_CopyPalette,
    TgaDecoder_GetMetadataQueryReader,
    TgaDecoder_GetPreview,
    TgaDecoder_GetColorContexts,
    TgaDecoder_GetThumbnail,
    TgaDecoder_GetFrameCount,
    TgaDecoder_GetFrame
};

static HRESULT WINAPI TgaDecoder_Frame_QueryInterface(IWICBitmapFrameDecode *iface, REFIID iid,
    void **ppv)
{
    TRACE("(%p,%s,%p)\n", iface, debugstr_guid(iid), ppv);

    if (!ppv) return E_INVALIDARG;

    if (IsEqualIID(&IID_IUnknown, iid) ||
        IsEqualIID(&IID_IWICBitmapSource, iid) ||
        IsEqualIID(&IID_IWICBitmapFrameDecode, iid))
    {
        *ppv = iface;
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI TgaDecoder_Frame_AddRef(IWICBitmapFrameDecode *iface)
{
    TgaDecoder *This = impl_from_IWICBitmapFrameDecode(iface);
    return IWICBitmapDecoder_AddRef(&This->IWICBitmapDecoder_iface);
}

static ULONG WINAPI TgaDecoder_Frame_Release(IWICBitmapFrameDecode *iface)
{
    TgaDecoder *This = impl_from_IWICBitmapFrameDecode(iface);
    return IWICBitmapDecoder_Release(&This->IWICBitmapDecoder_iface);
}

static HRESULT WINAPI TgaDecoder_Frame_GetSize(IWICBitmapFrameDecode *iface,
    UINT *puiWidth, UINT *puiHeight)
{
    TgaDecoder *This = impl_from_IWICBitmapFrameDecode(iface);

    *puiWidth = This->header.width;
    *puiHeight = This->header.height;

    TRACE("(%p)->(%u,%u)\n", iface, *puiWidth, *puiHeight);

    return S_OK;
}

static HRESULT WINAPI TgaDecoder_Frame_GetPixelFormat(IWICBitmapFrameDecode *iface,
    WICPixelFormatGUID *pPixelFormat)
{
    TgaDecoder *This = impl_from_IWICBitmapFrameDecode(iface);
    int attribute_bitcount;
    byte attribute_type;

    TRACE("(%p,%p)\n", iface, pPixelFormat);

    attribute_bitcount = This->header.image_descriptor & IMAGE_ATTRIBUTE_BITCOUNT_MASK;

    if (attribute_bitcount && This->extension_area_offset)
        attribute_type = This->extension_area.attributes_type;
    else if (attribute_bitcount)
        attribute_type = ATTRIBUTE_ALPHA;
    else
        attribute_type = ATTRIBUTE_NO_ALPHA;

    switch (This->header.image_type & ~IMAGETYPE_RLE)
    {
    case IMAGETYPE_COLORMAPPED:
        switch (This->header.depth)
        {
        case 8:
            memcpy(pPixelFormat, &GUID_WICPixelFormat8bppIndexed, sizeof(GUID));
            break;
        default:
            FIXME("Unhandled indexed color depth %u\n", This->header.depth);
            return E_NOTIMPL;
        }
        break;
    case IMAGETYPE_TRUECOLOR:
        switch (This->header.depth)
        {
        case 16:
            switch (attribute_type)
            {
            case ATTRIBUTE_NO_ALPHA:
            case ATTRIBUTE_UNDEFINED:
            case ATTRIBUTE_UNDEFINED_PRESERVE:
                memcpy(pPixelFormat, &GUID_WICPixelFormat16bppBGR555, sizeof(GUID));
                break;
            case ATTRIBUTE_ALPHA:
            case ATTRIBUTE_PALPHA:
                memcpy(pPixelFormat, &GUID_WICPixelFormat16bppBGRA5551, sizeof(GUID));
                break;
            default:
                FIXME("Unhandled 16-bit attribute type %u\n", attribute_type);
                return E_NOTIMPL;
            }
            break;
        case 24:
            memcpy(pPixelFormat, &GUID_WICPixelFormat24bppBGR, sizeof(GUID));
            break;
        case 32:
            switch (attribute_type)
            {
            case ATTRIBUTE_NO_ALPHA:
            case ATTRIBUTE_UNDEFINED:
            case ATTRIBUTE_UNDEFINED_PRESERVE:
                memcpy(pPixelFormat, &GUID_WICPixelFormat32bppBGR, sizeof(GUID));
                break;
            case ATTRIBUTE_ALPHA:
                memcpy(pPixelFormat, &GUID_WICPixelFormat32bppBGRA, sizeof(GUID));
                break;
            case ATTRIBUTE_PALPHA:
                memcpy(pPixelFormat, &GUID_WICPixelFormat32bppPBGRA, sizeof(GUID));
                break;
            default:
                FIXME("Unhandled 32-bit attribute type %u\n", attribute_type);
                return E_NOTIMPL;
            }
            break;
        default:
            FIXME("Unhandled truecolor depth %u\n", This->header.depth);
            return E_NOTIMPL;
        }
        break;
    case IMAGETYPE_GRAYSCALE:
        switch (This->header.depth)
        {
        case 8:
            memcpy(pPixelFormat, &GUID_WICPixelFormat8bppGray, sizeof(GUID));
            break;
        case 16:
            memcpy(pPixelFormat, &GUID_WICPixelFormat16bppGray, sizeof(GUID));
            break;
        default:
            FIXME("Unhandled grayscale depth %u\n", This->header.depth);
            return E_NOTIMPL;
        }
        break;
    default:
        ERR("Unknown image type %u\n", This->header.image_type);
        return E_FAIL;
    }

    return S_OK;
}

static HRESULT WINAPI TgaDecoder_Frame_GetResolution(IWICBitmapFrameDecode *iface,
    double *pDpiX, double *pDpiY)
{
    FIXME("(%p,%p,%p): stub\n", iface, pDpiX, pDpiY);
    return E_NOTIMPL;
}

static HRESULT WINAPI TgaDecoder_Frame_CopyPalette(IWICBitmapFrameDecode *iface,
    IWICPalette *pIPalette)
{
    TgaDecoder *This = impl_from_IWICBitmapFrameDecode(iface);
    HRESULT hr=S_OK;
    WICColor colors[256], *color;
    BYTE *colormap_data;
    WORD *wcolormap_data;
    DWORD *dwcolormap_data;
    LARGE_INTEGER seek;
    ULONG bytesread;
    int depth, attribute_bitcount, attribute_type;
    int i;

    TRACE("(%p,%p)\n", iface, pIPalette);

    if (!This->colormap_length)
    {
        WARN("no colormap present in this file\n");
        return WINCODEC_ERR_PALETTEUNAVAILABLE;
    }

    if (This->header.colormap_firstentry + This->header.colormap_length > 256)
    {
        FIXME("cannot read colormap with %i entries starting at %i\n",
            This->header.colormap_firstentry + This->header.colormap_length,
            This->header.colormap_firstentry);
        return E_FAIL;
    }

    colormap_data = malloc(This->colormap_length);
    if (!colormap_data) return E_OUTOFMEMORY;

    wcolormap_data = (WORD*)colormap_data;
    dwcolormap_data = (DWORD*)colormap_data;

    EnterCriticalSection(&This->lock);

    seek.QuadPart = This->colormap_offset;
    hr = IStream_Seek(This->stream, seek, STREAM_SEEK_SET, NULL);

    if (SUCCEEDED(hr))
    {
        hr = IStream_Read(This->stream, colormap_data, This->colormap_length, &bytesread);
        if (SUCCEEDED(hr) && bytesread != This->colormap_length)
        {
            WARN("expected %li bytes in colormap, got %li\n", This->colormap_length, bytesread);
            hr = E_FAIL;
        }
    }

    LeaveCriticalSection(&This->lock);

    if (SUCCEEDED(hr))
    {
        attribute_bitcount = This->header.image_descriptor & IMAGE_ATTRIBUTE_BITCOUNT_MASK;

        if (attribute_bitcount && This->extension_area_offset)
            attribute_type = This->extension_area.attributes_type;
        else if (attribute_bitcount)
            attribute_type = ATTRIBUTE_ALPHA;
        else
            attribute_type = ATTRIBUTE_NO_ALPHA;

        depth = This->header.colormap_entrysize;
        if (depth == 15)
        {
            depth = 16;
            attribute_type = ATTRIBUTE_NO_ALPHA;
        }

        memset(colors, 0, sizeof(colors));

        color = &colors[This->header.colormap_firstentry];

        /* Colormap entries can be in any truecolor format, and we have to convert them. */
        switch (depth)
        {
        case 16:
            switch (attribute_type)
            {
            case ATTRIBUTE_NO_ALPHA:
            case ATTRIBUTE_UNDEFINED:
            case ATTRIBUTE_UNDEFINED_PRESERVE:
                for (i=0; i<This->header.colormap_length; i++)
                {
                    WORD srcval = wcolormap_data[i];
                    *color++=0xff000000 | /* constant 255 alpha */
                             ((srcval << 9) & 0xf80000) | /* r */
                             ((srcval << 4) & 0x070000) | /* r - 3 bits */
                             ((srcval << 6) & 0x00f800) | /* g */
                             ((srcval << 1) & 0x000700) | /* g - 3 bits */
                             ((srcval << 3) & 0x0000f8) | /* b */
                             ((srcval >> 2) & 0x000007);  /* b - 3 bits */
                }
                break;
            case ATTRIBUTE_ALPHA:
            case ATTRIBUTE_PALPHA:
                for (i=0; i<This->header.colormap_length; i++)
                {
                    WORD srcval = wcolormap_data[i];
                    *color++=((srcval & 0x8000) ? 0xff000000 : 0) | /* alpha */
                             ((srcval << 9) & 0xf80000) | /* r */
                             ((srcval << 4) & 0x070000) | /* r - 3 bits */
                             ((srcval << 6) & 0x00f800) | /* g */
                             ((srcval << 1) & 0x000700) | /* g - 3 bits */
                             ((srcval << 3) & 0x0000f8) | /* b */
                             ((srcval >> 2) & 0x000007);  /* b - 3 bits */
                }
                break;
            default:
                FIXME("Unhandled 16-bit attribute type %u\n", attribute_type);
                hr = E_NOTIMPL;
            }
            break;
        case 24:
            for (i=0; i<This->header.colormap_length; i++)
            {
                *color++=0xff000000 | /* alpha */
                         colormap_data[i*3+2] | /* red */
                         colormap_data[i*3+1] | /* green */
                         colormap_data[i*3]; /* blue */
            }
            break;
        case 32:
            switch (attribute_type)
            {
            case ATTRIBUTE_NO_ALPHA:
            case ATTRIBUTE_UNDEFINED:
            case ATTRIBUTE_UNDEFINED_PRESERVE:
                for (i=0; i<This->header.colormap_length; i++)
                    *color++=dwcolormap_data[i]|0xff000000;
                break;
            case ATTRIBUTE_ALPHA:
                for (i=0; i<This->header.colormap_length; i++)
                    *color++=dwcolormap_data[i];
                break;
            case ATTRIBUTE_PALPHA:
                /* FIXME: Unpremultiply alpha */
            default:
                FIXME("Unhandled 16-bit attribute type %u\n", attribute_type);
                hr = E_NOTIMPL;
            }
            break;
        default:
            FIXME("Unhandled truecolor depth %u\n", This->header.depth);
            hr = E_NOTIMPL;
        }
    }

    free(colormap_data);

    if (SUCCEEDED(hr))
        hr = IWICPalette_InitializeCustom(pIPalette, colors, 256);

    return hr;
}

static HRESULT TgaDecoder_ReadRLE(TgaDecoder *This, BYTE *imagebits, int datasize)
{
    int i=0, j, bytesperpixel;
    ULONG bytesread;
    HRESULT hr=S_OK;

    bytesperpixel = This->header.depth / 8;

    while (i<datasize)
    {
        BYTE rc;
        int count, size;
        BYTE pixeldata[4];

        hr = IStream_Read(This->stream, &rc, 1, &bytesread);
        if (bytesread != 1) hr = E_FAIL;
        if (FAILED(hr)) break;

        count = (rc&0x7f)+1;
        size = count * bytesperpixel;

        if (size + i > datasize)
        {
            WARN("RLE packet too large\n");
            hr = E_FAIL;
            break;
        }

        if (rc&0x80)
        {
            /* Run-length packet */
            hr = IStream_Read(This->stream, pixeldata, bytesperpixel, &bytesread);
            if (bytesread != bytesperpixel) hr = E_FAIL;
            if (FAILED(hr)) break;

            if (bytesperpixel == 1)
                memset(&imagebits[i], pixeldata[0], count);
            else
            {
                for (j=0; j<count; j++)
                    memcpy(&imagebits[i+j*bytesperpixel], pixeldata, bytesperpixel);
            }
        }
        else
        {
            /* Raw packet */
            hr = IStream_Read(This->stream, &imagebits[i], size, &bytesread);
            if (bytesread != size) hr = E_FAIL;
            if (FAILED(hr)) break;
        }

        i += size;
    }

    return hr;
}

static HRESULT TgaDecoder_ReadImage(TgaDecoder *This)
{
    HRESULT hr=S_OK;
    int datasize;
    LARGE_INTEGER seek;
    ULONG bytesread;

    if (This->imagebits)
        return S_OK;

    EnterCriticalSection(&This->lock);

    if (!This->imagebits)
    {
        if (This->header.image_descriptor & IMAGE_RIGHTTOLEFT)
        {
            FIXME("Right to left image reading not implemented\n");
            hr = E_NOTIMPL;
        }

        if (SUCCEEDED(hr))
        {
            datasize = This->header.width * This->header.height * (This->header.depth / 8);
            This->imagebits = malloc(datasize);
            if (!This->imagebits) hr = E_OUTOFMEMORY;
        }

        if (SUCCEEDED(hr))
        {
            seek.QuadPart = This->image_offset;
            hr = IStream_Seek(This->stream, seek, STREAM_SEEK_SET, NULL);
        }

        if (SUCCEEDED(hr))
        {
            if (This->header.image_type & IMAGETYPE_RLE)
            {
                hr = TgaDecoder_ReadRLE(This, This->imagebits, datasize);
            }
            else
            {
                hr = IStream_Read(This->stream, This->imagebits, datasize, &bytesread);
                if (SUCCEEDED(hr) && bytesread != datasize)
                    hr = E_FAIL;
            }
        }

        if (SUCCEEDED(hr))
        {
            if (This->header.image_descriptor & IMAGE_TOPTOBOTTOM)
            {
                This->origin = This->imagebits;
                This->stride = This->header.width * (This->header.depth / 8);
            }
            else
            {
                This->stride = -This->header.width * (This->header.depth / 8);
                This->origin = This->imagebits + This->header.width * (This->header.height - 1) * (This->header.depth / 8);
            }
        }
        else
        {
            free(This->imagebits);
            This->imagebits = NULL;
        }
    }

    LeaveCriticalSection(&This->lock);

    return hr;
}

static HRESULT WINAPI TgaDecoder_Frame_CopyPixels(IWICBitmapFrameDecode *iface,
    const WICRect *prc, UINT cbStride, UINT cbBufferSize, BYTE *pbBuffer)
{
    TgaDecoder *This = impl_from_IWICBitmapFrameDecode(iface);
    HRESULT hr;

    TRACE("(%p,%s,%u,%u,%p)\n", iface, debug_wic_rect(prc), cbStride, cbBufferSize, pbBuffer);

    hr = TgaDecoder_ReadImage(This);

    if (SUCCEEDED(hr))
    {
        hr = copy_pixels(This->header.depth, This->origin,
            This->header.width, This->header.height, This->stride,
            prc, cbStride, cbBufferSize, pbBuffer);
    }

    return hr;
}

static HRESULT WINAPI TgaDecoder_Frame_GetMetadataQueryReader(IWICBitmapFrameDecode *iface,
    IWICMetadataQueryReader **ppIMetadataQueryReader)
{
    FIXME("(%p,%p): stub\n", iface, ppIMetadataQueryReader);
    return WINCODEC_ERR_UNSUPPORTEDOPERATION;
}

static HRESULT WINAPI TgaDecoder_Frame_GetColorContexts(IWICBitmapFrameDecode *iface,
    UINT cCount, IWICColorContext **ppIColorContexts, UINT *pcActualCount)
{
    FIXME("(%p,%u,%p,%p): stub\n", iface, cCount, ppIColorContexts, pcActualCount);
    return WINCODEC_ERR_UNSUPPORTEDOPERATION;
}

static HRESULT WINAPI TgaDecoder_Frame_GetThumbnail(IWICBitmapFrameDecode *iface,
    IWICBitmapSource **ppIThumbnail)
{
    FIXME("(%p,%p): stub\n", iface, ppIThumbnail);
    return WINCODEC_ERR_CODECNOTHUMBNAIL;
}

static const IWICBitmapFrameDecodeVtbl TgaDecoder_Frame_Vtbl = {
    TgaDecoder_Frame_QueryInterface,
    TgaDecoder_Frame_AddRef,
    TgaDecoder_Frame_Release,
    TgaDecoder_Frame_GetSize,
    TgaDecoder_Frame_GetPixelFormat,
    TgaDecoder_Frame_GetResolution,
    TgaDecoder_Frame_CopyPalette,
    TgaDecoder_Frame_CopyPixels,
    TgaDecoder_Frame_GetMetadataQueryReader,
    TgaDecoder_Frame_GetColorContexts,
    TgaDecoder_Frame_GetThumbnail
};

HRESULT TgaDecoder_CreateInstance(REFIID iid, void** ppv)
{
    TgaDecoder *This;
    HRESULT ret;

    TRACE("(%s,%p)\n", debugstr_guid(iid), ppv);

    *ppv = NULL;

    This = malloc(sizeof(TgaDecoder));
    if (!This) return E_OUTOFMEMORY;

    This->IWICBitmapDecoder_iface.lpVtbl = &TgaDecoder_Vtbl;
    This->IWICBitmapFrameDecode_iface.lpVtbl = &TgaDecoder_Frame_Vtbl;
    This->ref = 1;
    This->initialized = FALSE;
    This->stream = NULL;
    This->imagebits = NULL;
#ifdef __REACTOS__
    InitializeCriticalSection(&This->lock);
#else
    InitializeCriticalSectionEx(&This->lock, 0, RTL_CRITICAL_SECTION_FLAG_FORCE_DEBUG_INFO);
#endif
    This->lock.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": TgaDecoder.lock");

    ret = IWICBitmapDecoder_QueryInterface(&This->IWICBitmapDecoder_iface, iid, ppv);
    IWICBitmapDecoder_Release(&This->IWICBitmapDecoder_iface);

    return ret;
}
