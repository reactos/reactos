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

#include <assert.h>
#include <stdarg.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "wingdi.h"
#include "objbase.h"

#include "wincodecs_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(wincodecs);

typedef struct {
    DWORD bc2Size;
    DWORD bc2Width;
    DWORD bc2Height;
    WORD  bc2Planes;
    WORD  bc2BitCount;
    DWORD bc2Compression;
    DWORD bc2SizeImage;
    DWORD bc2XRes;
    DWORD bc2YRes;
    DWORD bc2ClrUsed;
    DWORD bc2ClrImportant;
    /* same as BITMAPINFOHEADER until this point */
    WORD  bc2ResUnit;
    WORD  bc2Reserved;
    WORD  bc2Orientation;
    WORD  bc2Halftoning;
    DWORD bc2HalftoneSize1;
    DWORD bc2HalftoneSize2;
    DWORD bc2ColorSpace;
    DWORD bc2AppData;
} BITMAPCOREHEADER2;

typedef HRESULT (*ReadDataFunc)(BmpDecoder* This);

struct BmpDecoder {
    IWICBitmapDecoder IWICBitmapDecoder_iface;
    IWICBitmapFrameDecode IWICBitmapFrameDecode_iface;
    LONG ref;
    BOOL initialized;
    IStream *stream;
    ULONG palette_offset;
    ULONG image_offset;
    BITMAPV5HEADER bih;
    const WICPixelFormatGUID *pixelformat;
    int bitsperpixel;
    ReadDataFunc read_data_func;
    INT stride;
    BYTE *imagedata;
    BYTE *imagedatastart;
    CRITICAL_SECTION lock; /* must be held when initialized/imagedata is set or stream is accessed */
    int packed; /* If TRUE, don't look for a file header and assume a packed DIB. */
    int icoframe; /* If TRUE, this is a frame of a .ico file. */
};

static inline BmpDecoder *impl_from_IWICBitmapDecoder(IWICBitmapDecoder *iface)
{
    return CONTAINING_RECORD(iface, BmpDecoder, IWICBitmapDecoder_iface);
}

static inline BmpDecoder *impl_from_IWICBitmapFrameDecode(IWICBitmapFrameDecode *iface)
{
    return CONTAINING_RECORD(iface, BmpDecoder, IWICBitmapFrameDecode_iface);
}

static HRESULT WINAPI BmpFrameDecode_QueryInterface(IWICBitmapFrameDecode *iface, REFIID iid,
    void **ppv)
{
    BmpDecoder *This = impl_from_IWICBitmapFrameDecode(iface);

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

static ULONG WINAPI BmpFrameDecode_AddRef(IWICBitmapFrameDecode *iface)
{
    BmpDecoder *This = impl_from_IWICBitmapFrameDecode(iface);

    return IWICBitmapDecoder_AddRef(&This->IWICBitmapDecoder_iface);
}

static ULONG WINAPI BmpFrameDecode_Release(IWICBitmapFrameDecode *iface)
{
    BmpDecoder *This = impl_from_IWICBitmapFrameDecode(iface);

    return IWICBitmapDecoder_Release(&This->IWICBitmapDecoder_iface);
}

static HRESULT WINAPI BmpFrameDecode_GetSize(IWICBitmapFrameDecode *iface,
    UINT *puiWidth, UINT *puiHeight)
{
    BmpDecoder *This = impl_from_IWICBitmapFrameDecode(iface);
    TRACE("(%p,%p,%p)\n", iface, puiWidth, puiHeight);

    if (This->bih.bV5Size == sizeof(BITMAPCOREHEADER))
    {
        BITMAPCOREHEADER *bch = (BITMAPCOREHEADER*)&This->bih;
        *puiWidth = bch->bcWidth;
        *puiHeight = bch->bcHeight;
    }
    else
    {
        *puiWidth = This->bih.bV5Width;
        *puiHeight = abs(This->bih.bV5Height);
    }
    return S_OK;
}

static HRESULT WINAPI BmpFrameDecode_GetPixelFormat(IWICBitmapFrameDecode *iface,
    WICPixelFormatGUID *pPixelFormat)
{
    BmpDecoder *This = impl_from_IWICBitmapFrameDecode(iface);
    TRACE("(%p,%p)\n", iface, pPixelFormat);

    memcpy(pPixelFormat, This->pixelformat, sizeof(GUID));

    return S_OK;
}

static HRESULT BmpHeader_GetResolution(BITMAPV5HEADER *bih, double *pDpiX, double *pDpiY)
{
    LONG resx = 0, resy = 0;

    switch (bih->bV5Size)
    {
    default:
    case sizeof(BITMAPCOREHEADER):
        break;

    case sizeof(BITMAPCOREHEADER2):
    case sizeof(BITMAPINFOHEADER):
    case sizeof(BITMAPV4HEADER):
    case sizeof(BITMAPV5HEADER):
        resx = bih->bV5XPelsPerMeter;
        resy = bih->bV5YPelsPerMeter;
        break;
    }

    if (!resx || !resy)
    {
        *pDpiX = 96.0;
        *pDpiY = 96.0;
    }
    else
    {
        *pDpiX = resx * 0.0254;
        *pDpiY = resy * 0.0254;
    }

    return S_OK;
}

static HRESULT WINAPI BmpFrameDecode_GetResolution(IWICBitmapFrameDecode *iface,
    double *pDpiX, double *pDpiY)
{
    BmpDecoder *This = impl_from_IWICBitmapFrameDecode(iface);
    TRACE("(%p,%p,%p)\n", iface, pDpiX, pDpiY);

    return BmpHeader_GetResolution(&This->bih, pDpiX, pDpiY);
}

static HRESULT WINAPI BmpFrameDecode_CopyPalette(IWICBitmapFrameDecode *iface,
    IWICPalette *pIPalette)
{
    HRESULT hr;
    BmpDecoder *This = impl_from_IWICBitmapFrameDecode(iface);
    int count;
    WICColor *wiccolors=NULL;
    RGBTRIPLE *bgrcolors=NULL;

    TRACE("(%p,%p)\n", iface, pIPalette);

    EnterCriticalSection(&This->lock);

    if (This->bih.bV5Size == sizeof(BITMAPCOREHEADER))
    {
        BITMAPCOREHEADER *bch = (BITMAPCOREHEADER*)&This->bih;
        if (bch->bcBitCount <= 8)
        {
            /* 2**n colors in BGR format after the header */
            ULONG tablesize, bytesread;
            LARGE_INTEGER offset;
            int i;

            count = 1 << bch->bcBitCount;
            wiccolors = malloc(sizeof(WICColor) * count);
            tablesize = sizeof(RGBTRIPLE) * count;
            bgrcolors = malloc(tablesize);
            if (!wiccolors || !bgrcolors)
            {
                hr = E_OUTOFMEMORY;
                goto end;
            }

            offset.QuadPart = This->palette_offset;
            hr = IStream_Seek(This->stream, offset, STREAM_SEEK_SET, NULL);
            if (FAILED(hr)) goto end;

            hr = IStream_Read(This->stream, bgrcolors, tablesize, &bytesread);
            if (FAILED(hr)) goto end;
            if (bytesread != tablesize) {
                hr = E_FAIL;
                goto end;
            }

            for (i=0; i<count; i++)
            {
                wiccolors[i] = 0xff000000|
                               (bgrcolors[i].rgbtRed<<16)|
                               (bgrcolors[i].rgbtGreen<<8)|
                               bgrcolors[i].rgbtBlue;
            }
        }
        else
        {
            hr = WINCODEC_ERR_PALETTEUNAVAILABLE;
            goto end;
        }
    }
    else
    {
        if (This->bih.bV5BitCount <= 8)
        {
            ULONG tablesize, bytesread;
            LARGE_INTEGER offset;
            int i;

            if (This->bih.bV5ClrUsed == 0)
                count = 1 << This->bih.bV5BitCount;
            else
                count = min(This->bih.bV5ClrUsed, 1 << This->bih.bV5BitCount);

            tablesize = sizeof(WICColor) * count;
            wiccolors = malloc(tablesize);
            if (!wiccolors)
            {
                hr = E_OUTOFMEMORY;
                goto end;
            }

            offset.QuadPart = This->palette_offset;
            hr = IStream_Seek(This->stream, offset, STREAM_SEEK_SET, NULL);
            if (FAILED(hr)) goto end;

            hr = IStream_Read(This->stream, wiccolors, tablesize, &bytesread);
            if (FAILED(hr)) goto end;
            if (bytesread != tablesize) {
                hr = E_FAIL;
                goto end;
            }

            /* convert from BGR to BGRA by setting alpha to 100% */
            for (i=0; i<count; i++)
                wiccolors[i] |= 0xff000000;
        }
        else
        {
            hr = WINCODEC_ERR_PALETTEUNAVAILABLE;
            goto end;
        }
    }

end:

    LeaveCriticalSection(&This->lock);

    if (SUCCEEDED(hr))
        hr = IWICPalette_InitializeCustom(pIPalette, wiccolors, count);

    free(wiccolors);
    free(bgrcolors);
    return hr;
}

static HRESULT WINAPI BmpFrameDecode_CopyPixels(IWICBitmapFrameDecode *iface,
    const WICRect *prc, UINT cbStride, UINT cbBufferSize, BYTE *pbBuffer)
{
    BmpDecoder *This = impl_from_IWICBitmapFrameDecode(iface);
    HRESULT hr=S_OK;
    UINT width, height;
    TRACE("(%p,%s,%u,%u,%p)\n", iface, debug_wic_rect(prc), cbStride, cbBufferSize, pbBuffer);

    EnterCriticalSection(&This->lock);
    if (!This->imagedata)
    {
        hr = This->read_data_func(This);
    }
    LeaveCriticalSection(&This->lock);
    if (FAILED(hr)) return hr;

    hr = BmpFrameDecode_GetSize(iface, &width, &height);
    if (FAILED(hr)) return hr;

    return copy_pixels(This->bitsperpixel, This->imagedatastart,
        width, height, This->stride,
        prc, cbStride, cbBufferSize, pbBuffer);
}

static HRESULT WINAPI BmpFrameDecode_GetMetadataQueryReader(IWICBitmapFrameDecode *iface,
    IWICMetadataQueryReader **ppIMetadataQueryReader)
{
    TRACE("(%p,%p)\n", iface, ppIMetadataQueryReader);
    return WINCODEC_ERR_UNSUPPORTEDOPERATION;
}

static HRESULT WINAPI BmpFrameDecode_GetColorContexts(IWICBitmapFrameDecode *iface,
    UINT cCount, IWICColorContext **ppIColorContexts, UINT *pcActualCount)
{
    TRACE("(%p,%u,%p,%p)\n", iface, cCount, ppIColorContexts, pcActualCount);
    return WINCODEC_ERR_UNSUPPORTEDOPERATION;
}

static HRESULT WINAPI BmpFrameDecode_GetThumbnail(IWICBitmapFrameDecode *iface,
    IWICBitmapSource **ppIThumbnail)
{
    TRACE("(%p,%p)\n", iface, ppIThumbnail);
    return WINCODEC_ERR_CODECNOTHUMBNAIL;
}

static HRESULT BmpFrameDecode_ReadUncompressed(BmpDecoder* This)
{
    UINT bytesperrow;
    UINT width, height;
    UINT datasize;
    int bottomup;
    HRESULT hr;
    LARGE_INTEGER offbits;
    ULONG bytesread;

    if (This->bih.bV5Size == sizeof(BITMAPCOREHEADER))
    {
        BITMAPCOREHEADER *bch = (BITMAPCOREHEADER*)&This->bih;
        width = bch->bcWidth;
        height = bch->bcHeight;
        bottomup = 1;
    }
    else
    {
        width = This->bih.bV5Width;
        height = abs(This->bih.bV5Height);
        bottomup = (This->bih.bV5Height > 0);
    }

    /* row sizes in BMP files must be divisible by 4 bytes */
    bytesperrow = (((width * This->bitsperpixel)+31)/32)*4;
    datasize = bytesperrow * height;

    This->imagedata = malloc(datasize);
    if (!This->imagedata) return E_OUTOFMEMORY;

    offbits.QuadPart = This->image_offset;
    hr = IStream_Seek(This->stream, offbits, STREAM_SEEK_SET, NULL);
    if (FAILED(hr)) goto fail;

    hr = IStream_Read(This->stream, This->imagedata, datasize, &bytesread);
    if (FAILED(hr) || bytesread != datasize) goto fail;

    if (bottomup)
    {
        This->imagedatastart = This->imagedata + (height-1) * bytesperrow;
        This->stride = -bytesperrow;
    }
    else
    {
        This->imagedatastart = This->imagedata;
        This->stride = bytesperrow;
    }
    return S_OK;

fail:
    free(This->imagedata);
    This->imagedata = NULL;
    if (SUCCEEDED(hr)) hr = E_FAIL;
    return hr;
}

static HRESULT BmpFrameDecode_ReadABGRasBGR(BmpDecoder* This)
{
    UINT x, y, width, height;
    BYTE *pixel;
    HRESULT hr;

    hr = IWICBitmapFrameDecode_GetSize(&This->IWICBitmapFrameDecode_iface, &width, &height);

    if (SUCCEEDED(hr))
    {
        hr = BmpFrameDecode_ReadUncompressed(This);
    }

    if (SUCCEEDED(hr))
    {
        for (y = 0; y < height; y++)
        {
            pixel = This->imagedatastart + This->stride * (INT)y;

            for (x = 0; x < width; x++)
            {
                pixel[0] = pixel[1];
                pixel[1] = pixel[2];
                pixel[2] = pixel[3];
                pixel[3] = 0;
                pixel += 4;
            }
        }
    }

    return hr;
}

static HRESULT BmpFrameDecode_ReadRGB8(BmpDecoder* This)
{
    HRESULT hr;
    UINT width, height;

    hr = IWICBitmapFrameDecode_GetSize(&This->IWICBitmapFrameDecode_iface, &width, &height);

    if (SUCCEEDED(hr))
    {
        hr = BmpFrameDecode_ReadUncompressed(This);
    }

    if (SUCCEEDED(hr))
    {
        reverse_bgr8(This->bitsperpixel/8, This->imagedatastart,
            width, height, This->stride);
    }

    return hr;
}

static HRESULT ReadByte(IStream *stream, BYTE *buffer, ULONG buffer_size,
    ULONG *cursor, ULONG *bytesread, BYTE *result)
{
    HRESULT hr=S_OK;

    if (*bytesread == 0 || *cursor == *bytesread)
    {
        hr = IStream_Read(stream, buffer, buffer_size, bytesread);
        *cursor = 0;
    }

    if (SUCCEEDED(hr))
    {
        if (*cursor < *bytesread)
            *result = buffer[(*cursor)++];
        else
            hr = E_FAIL;
    }

    return hr;
}

static HRESULT BmpFrameDecode_ReadRLE8(BmpDecoder* This)
{
    UINT bytesperrow;
    UINT width, height;
    BYTE rledata[4096];
    UINT datasize, palettesize;
    DWORD palette[256];
    UINT x, y;
    DWORD *bgrdata;
    HRESULT hr;
    LARGE_INTEGER offbits;
    ULONG cursor=0, bytesread=0;

    width = This->bih.bV5Width;
    height = abs(This->bih.bV5Height);
    bytesperrow = width * 4;
    datasize = bytesperrow * height;
    if (This->bih.bV5ClrUsed && This->bih.bV5ClrUsed < 256)
        palettesize = 4 * This->bih.bV5ClrUsed;
    else
        palettesize = 4 * 256;

    This->imagedata = malloc(datasize);
    if (!This->imagedata)
    {
        hr = E_OUTOFMEMORY;
        goto fail;
    }

    /* read palette */
    offbits.QuadPart = This->palette_offset;
    hr = IStream_Seek(This->stream, offbits, STREAM_SEEK_SET, NULL);
    if (FAILED(hr)) goto fail;

    hr = IStream_Read(This->stream, palette, palettesize, &bytesread);
    if (FAILED(hr) || bytesread != palettesize) goto fail;

    /* read RLE data */
    offbits.QuadPart = This->image_offset;
    hr = IStream_Seek(This->stream, offbits, STREAM_SEEK_SET, NULL);
    if (FAILED(hr)) goto fail;

    /* decode RLE */
    bgrdata = (DWORD*)This->imagedata;
    x = 0;
    y = 0;
    cursor = 0;
    bytesread = 0;
    while (y < height)
    {
        BYTE length;
        hr = ReadByte(This->stream, rledata, 4096, &cursor, &bytesread, &length);

        if (FAILED(hr))
            goto fail;
        else if (length == 0)
        {
            /* escape code */
            BYTE escape;
            hr = ReadByte(This->stream, rledata, 4096, &cursor, &bytesread, &escape);
            if (FAILED(hr))
                goto fail;
            switch(escape)
            {
            case 0: /* end of line */
                x = 0;
                y++;
                break;
            case 1: /* end of bitmap */
                goto end;
            case 2: /* delta */
            {
                BYTE dx, dy;
                hr = ReadByte(This->stream, rledata, 4096, &cursor, &bytesread, &dx);
                if (SUCCEEDED(hr))
                    hr = ReadByte(This->stream, rledata, 4096, &cursor, &bytesread, &dy);
                if (FAILED(hr))
                    goto fail;
                x += dx;
                y += dy;
                break;
            }
            default: /* absolute mode */
                length = escape;
                while (length-- && x < width)
                {
                    BYTE index;
                    hr = ReadByte(This->stream, rledata, 4096, &cursor, &bytesread, &index);
                    if (FAILED(hr))
                        goto fail;
                    bgrdata[y*width + x++] = palette[index];
                }
                if (escape & 1)
                    hr = ReadByte(This->stream, rledata, 4096, &cursor, &bytesread, &length); /* skip pad byte */
                if (FAILED(hr))
                    goto fail;
            }
        }
        else
        {
            BYTE index;
            DWORD color;
            hr = ReadByte(This->stream, rledata, 4096, &cursor, &bytesread, &index);
            if (FAILED(hr))
                goto fail;
            color = palette[index];
            while (length-- && x < width)
                bgrdata[y*width + x++] = color;
        }
    }

end:
    This->imagedatastart = This->imagedata + (height-1) * bytesperrow;
    This->stride = -bytesperrow;

    return S_OK;

fail:
    free(This->imagedata);
    This->imagedata = NULL;
    if (SUCCEEDED(hr)) hr = E_FAIL;
    return hr;
}

static HRESULT BmpFrameDecode_ReadRLE4(BmpDecoder* This)
{
    UINT bytesperrow;
    UINT width, height;
    BYTE rledata[4096];
    UINT datasize, palettesize;
    DWORD palette[16];
    UINT x, y;
    DWORD *bgrdata;
    HRESULT hr;
    LARGE_INTEGER offbits;
    ULONG cursor=0, bytesread=0;

    width = This->bih.bV5Width;
    height = abs(This->bih.bV5Height);
    bytesperrow = width * 4;
    datasize = bytesperrow * height;
    if (This->bih.bV5ClrUsed && This->bih.bV5ClrUsed < 16)
        palettesize = 4 * This->bih.bV5ClrUsed;
    else
        palettesize = 4 * 16;

    This->imagedata = malloc(datasize);
    if (!This->imagedata)
    {
        hr = E_OUTOFMEMORY;
        goto fail;
    }

    /* read palette */
    offbits.QuadPart = This->palette_offset;
    hr = IStream_Seek(This->stream, offbits, STREAM_SEEK_SET, NULL);
    if (FAILED(hr)) goto fail;

    hr = IStream_Read(This->stream, palette, palettesize, &bytesread);
    if (FAILED(hr) || bytesread != palettesize) goto fail;

    /* read RLE data */
    offbits.QuadPart = This->image_offset;
    hr = IStream_Seek(This->stream, offbits, STREAM_SEEK_SET, NULL);
    if (FAILED(hr)) goto fail;

    /* decode RLE */
    bgrdata = (DWORD*)This->imagedata;
    x = 0;
    y = 0;
    cursor = 0;
    bytesread = 0;
    while (y < height)
    {
        BYTE length;
        hr = ReadByte(This->stream, rledata, 4096, &cursor, &bytesread, &length);

        if (FAILED(hr))
            goto fail;
        else if (length == 0)
        {
            /* escape code */
            BYTE escape;
            hr = ReadByte(This->stream, rledata, 4096, &cursor, &bytesread, &escape);
            if (FAILED(hr))
                goto fail;
            switch(escape)
            {
            case 0: /* end of line */
                x = 0;
                y++;
                break;
            case 1: /* end of bitmap */
                goto end;
            case 2: /* delta */
            {
                BYTE dx, dy;
                hr = ReadByte(This->stream, rledata, 4096, &cursor, &bytesread, &dx);
                if (SUCCEEDED(hr))
                    hr = ReadByte(This->stream, rledata, 4096, &cursor, &bytesread, &dy);
                if (FAILED(hr))
                    goto fail;
                x += dx;
                y += dy;
                break;
            }
            default: /* absolute mode */
            {
                BYTE realsize=0;
                length = escape;
                while (length-- && x < width)
                {
                    BYTE colors;
                    hr = ReadByte(This->stream, rledata, 4096, &cursor, &bytesread, &colors);
                    realsize++;
                    if (FAILED(hr))
                        goto fail;
                    bgrdata[y*width + x++] = palette[colors>>4];
                    if (length-- && x < width)
                        bgrdata[y*width + x++] = palette[colors&0xf];
                    else
                        break;
                }
                if (realsize & 1)
                    hr = ReadByte(This->stream, rledata, 4096, &cursor, &bytesread, &length); /* skip pad byte */
                if (FAILED(hr))
                    goto fail;
            }
            }
        }
        else
        {
            BYTE colors;
            DWORD color1;
            DWORD color2;
            hr = ReadByte(This->stream, rledata, 4096, &cursor, &bytesread, &colors);
            if (FAILED(hr))
                goto fail;
            color1 = palette[colors>>4];
            color2 = palette[colors&0xf];
            while (length-- && x < width)
            {
                bgrdata[y*width + x++] = color1;
                if (length-- && x < width)
                    bgrdata[y*width + x++] = color2;
                else
                    break;
            }
        }
    }

end:
    This->imagedatastart = This->imagedata + (height-1) * bytesperrow;
    This->stride = -bytesperrow;

    return S_OK;

fail:
    free(This->imagedata);
    This->imagedata = NULL;
    if (SUCCEEDED(hr)) hr = E_FAIL;
    return hr;
}

static HRESULT BmpFrameDecode_ReadUnsupported(BmpDecoder* This)
{
    return E_FAIL;
}

struct bitfields_format {
    WORD bitcount; /* 0 for end of list */
    DWORD redmask;
    DWORD greenmask;
    DWORD bluemask;
    DWORD alphamask;
    const WICPixelFormatGUID *pixelformat;
    ReadDataFunc read_data_func;
};

static const struct bitfields_format bitfields_formats[] = {
    {16,0x7c00,0x3e0,0x1f,0,&GUID_WICPixelFormat16bppBGR555,BmpFrameDecode_ReadUncompressed},
    {16,0xf800,0x7e0,0x1f,0,&GUID_WICPixelFormat16bppBGR565,BmpFrameDecode_ReadUncompressed},
    {32,0xff0000,0xff00,0xff,0,&GUID_WICPixelFormat32bppBGR,BmpFrameDecode_ReadUncompressed},
    {32,0xff0000,0xff00,0xff,0xff000000,&GUID_WICPixelFormat32bppBGRA,BmpFrameDecode_ReadUncompressed},
    {32,0xff000000,0xff0000,0xff00,0xff,&GUID_WICPixelFormat32bppBGR,BmpFrameDecode_ReadABGRasBGR},
    {32,0xff,0xff00,0xff0000,0,&GUID_WICPixelFormat32bppBGR,BmpFrameDecode_ReadRGB8},
    {0}
};

static const IWICBitmapFrameDecodeVtbl BmpDecoder_FrameVtbl = {
    BmpFrameDecode_QueryInterface,
    BmpFrameDecode_AddRef,
    BmpFrameDecode_Release,
    BmpFrameDecode_GetSize,
    BmpFrameDecode_GetPixelFormat,
    BmpFrameDecode_GetResolution,
    BmpFrameDecode_CopyPalette,
    BmpFrameDecode_CopyPixels,
    BmpFrameDecode_GetMetadataQueryReader,
    BmpFrameDecode_GetColorContexts,
    BmpFrameDecode_GetThumbnail
};

static HRESULT BmpDecoder_ReadHeaders(BmpDecoder* This, IStream *stream)
{
    HRESULT hr;
    ULONG bytestoread, bytesread;
    LARGE_INTEGER seek;

    if (This->initialized) return WINCODEC_ERR_WRONGSTATE;

    seek.QuadPart = 0;
    hr = IStream_Seek(stream, seek, STREAM_SEEK_SET, NULL);
    if (FAILED(hr)) return hr;

    if (!This->packed)
    {
        BITMAPFILEHEADER bfh;
        hr = IStream_Read(stream, &bfh, sizeof(BITMAPFILEHEADER), &bytesread);
        if (FAILED(hr)) return hr;
        if (bytesread != sizeof(BITMAPFILEHEADER) ||
            bfh.bfType != 0x4d42 /* "BM" */) return E_FAIL;
        This->image_offset = bfh.bfOffBits;
    }

    hr = IStream_Read(stream, &This->bih.bV5Size, sizeof(DWORD), &bytesread);
    if (FAILED(hr)) return hr;
    if (bytesread != sizeof(DWORD) ||
        (This->bih.bV5Size != sizeof(BITMAPCOREHEADER) &&
         This->bih.bV5Size != sizeof(BITMAPCOREHEADER2) &&
         This->bih.bV5Size != sizeof(BITMAPINFOHEADER) &&
         This->bih.bV5Size != sizeof(BITMAPV4HEADER) &&
         This->bih.bV5Size != sizeof(BITMAPV5HEADER))) return E_FAIL;

    bytestoread = This->bih.bV5Size-sizeof(DWORD);
    hr = IStream_Read(stream, &This->bih.bV5Width, bytestoread, &bytesread);
    if (FAILED(hr)) return hr;
    if (bytestoread != bytesread) return E_FAIL;

    if (This->packed)
        This->palette_offset = This->bih.bV5Size;
    else
        This->palette_offset = sizeof(BITMAPFILEHEADER) + This->bih.bV5Size;

    if (This->icoframe)
    {
        if (This->bih.bV5Size == sizeof(BITMAPCOREHEADER))
        {
            BITMAPCOREHEADER *bch = (BITMAPCOREHEADER*)&This->bih;
            bch->bcHeight /= 2;
        }
        else
        {
            This->bih.bV5Height /= 2;
        }
    }

    /* if this is a BITMAPINFOHEADER with BI_BITFIELDS compression, we need to
        read the extra fields */
    if (This->bih.bV5Size == sizeof(BITMAPINFOHEADER) &&
        This->bih.bV5Compression == BI_BITFIELDS)
    {
        hr = IStream_Read(stream, &This->bih.bV5RedMask, 12, &bytesread);
        if (FAILED(hr)) return hr;
        if (bytesread != 12) return E_FAIL;
        This->bih.bV5AlphaMask = 0;
        This->palette_offset += 12;
    }

    /* decide what kind of bitmap this is and how/if we can read it */
    if (This->bih.bV5Size == sizeof(BITMAPCOREHEADER))
    {
        BITMAPCOREHEADER *bch = (BITMAPCOREHEADER*)&This->bih;
        TRACE("BITMAPCOREHEADER with depth=%i\n", bch->bcBitCount);
        This->bitsperpixel = bch->bcBitCount;
        This->read_data_func = BmpFrameDecode_ReadUncompressed;
        switch(bch->bcBitCount)
        {
        case 1:
            This->pixelformat = &GUID_WICPixelFormat1bppIndexed;
            break;
        case 2:
            This->pixelformat = &GUID_WICPixelFormat2bppIndexed;
            break;
        case 4:
            This->pixelformat = &GUID_WICPixelFormat4bppIndexed;
            break;
        case 8:
            This->pixelformat = &GUID_WICPixelFormat8bppIndexed;
            break;
        case 24:
            This->pixelformat = &GUID_WICPixelFormat24bppBGR;
            break;
        default:
            This->pixelformat = &GUID_WICPixelFormatUndefined;
            WARN("unsupported bit depth %i for BITMAPCOREHEADER\n", bch->bcBitCount);
            break;
        }
    }
    else /* struct is compatible with BITMAPINFOHEADER */
    {
        TRACE("bitmap header=%li compression=%li depth=%i\n", This->bih.bV5Size, This->bih.bV5Compression, This->bih.bV5BitCount);
        switch(This->bih.bV5Compression)
        {
        case BI_RGB:
            This->bitsperpixel = This->bih.bV5BitCount;
            This->read_data_func = BmpFrameDecode_ReadUncompressed;
            switch(This->bih.bV5BitCount)
            {
            case 1:
                This->pixelformat = &GUID_WICPixelFormat1bppIndexed;
                break;
            case 2:
                This->pixelformat = &GUID_WICPixelFormat2bppIndexed;
                break;
            case 4:
                This->pixelformat = &GUID_WICPixelFormat4bppIndexed;
                break;
            case 8:
                This->pixelformat = &GUID_WICPixelFormat8bppIndexed;
                break;
            case 16:
                This->pixelformat = &GUID_WICPixelFormat16bppBGR555;
                break;
            case 24:
                This->pixelformat = &GUID_WICPixelFormat24bppBGR;
                break;
            case 32:
                This->pixelformat = &GUID_WICPixelFormat32bppBGR;
                break;
            default:
                This->pixelformat = &GUID_WICPixelFormatUndefined;
                FIXME("unsupported bit depth %i for uncompressed RGB\n", This->bih.bV5BitCount);
            }
            break;
        case BI_RLE8:
            This->bitsperpixel = 32;
            This->read_data_func = BmpFrameDecode_ReadRLE8;
            This->pixelformat = &GUID_WICPixelFormat32bppBGR;
            break;
        case BI_RLE4:
            This->bitsperpixel = 32;
            This->read_data_func = BmpFrameDecode_ReadRLE4;
            This->pixelformat = &GUID_WICPixelFormat32bppBGR;
            break;
        case BI_BITFIELDS:
        {
            const struct bitfields_format *format;
            if (This->bih.bV5Size == sizeof(BITMAPCOREHEADER2))
            {
                /* BCH2 doesn't support bitfields; this is Huffman 1D compression */
                This->bitsperpixel = 0;
                This->read_data_func = BmpFrameDecode_ReadUnsupported;
                This->pixelformat = &GUID_WICPixelFormatUndefined;
                FIXME("Huffman 1D compression is unsupported\n");
                break;
            }
            This->bitsperpixel = This->bih.bV5BitCount;
            for (format = bitfields_formats; format->bitcount; format++)
            {
                if ((format->bitcount == This->bih.bV5BitCount) &&
                    (format->redmask == This->bih.bV5RedMask) &&
                    (format->greenmask == This->bih.bV5GreenMask) &&
                    (format->bluemask == This->bih.bV5BlueMask) &&
                    (format->alphamask == This->bih.bV5AlphaMask))
                {
                    This->read_data_func = format->read_data_func;
                    This->pixelformat = format->pixelformat;
                    break;
                }
            }
            if (!format->bitcount)
            {
                This->read_data_func = BmpFrameDecode_ReadUncompressed;
                This->pixelformat = &GUID_WICPixelFormatUndefined;
                FIXME("unsupported bitfields type depth=%i red=%lx green=%lx blue=%lx alpha=%lx\n",
                    This->bih.bV5BitCount, This->bih.bV5RedMask, This->bih.bV5GreenMask, This->bih.bV5BlueMask, This->bih.bV5AlphaMask);
            }
            break;
        }
        default:
            This->bitsperpixel = 0;
            This->read_data_func = BmpFrameDecode_ReadUnsupported;
            This->pixelformat = &GUID_WICPixelFormatUndefined;
            FIXME("unsupported bitmap type header=%li compression=%li depth=%i\n", This->bih.bV5Size, This->bih.bV5Compression, This->bih.bV5BitCount);
            break;
        }
    }

    if (This->packed)
    {
        /* In a packed DIB, the image follows the palette. */
        ULONG palette_count, palette_size;
        if (This->bih.bV5ClrUsed)
            palette_count = This->bih.bV5ClrUsed;
        else if (This->bih.bV5BitCount <= 8)
            palette_count = 1 << This->bih.bV5BitCount;
        else
            palette_count = 0;
        if (This->bih.bV5Size == sizeof(BITMAPCOREHEADER))
            palette_size = sizeof(RGBTRIPLE) * palette_count;
        else
            palette_size = sizeof(RGBQUAD) * palette_count;
        This->image_offset = This->palette_offset + palette_size;
    }

    This->initialized = TRUE;

    return S_OK;
}

static HRESULT WINAPI BmpDecoder_QueryInterface(IWICBitmapDecoder *iface, REFIID iid,
    void **ppv)
{
    BmpDecoder *This = impl_from_IWICBitmapDecoder(iface);
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

static ULONG WINAPI BmpDecoder_AddRef(IWICBitmapDecoder *iface)
{
    BmpDecoder *This = impl_from_IWICBitmapDecoder(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) refcount=%lu\n", iface, ref);

    return ref;
}

static ULONG WINAPI BmpDecoder_Release(IWICBitmapDecoder *iface)
{
    BmpDecoder *This = impl_from_IWICBitmapDecoder(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) refcount=%lu\n", iface, ref);

    if (ref == 0)
    {
        if (This->stream) IStream_Release(This->stream);
        free(This->imagedata);
        This->lock.DebugInfo->Spare[0] = 0;
        DeleteCriticalSection(&This->lock);
        free(This);
    }

    return ref;
}

static HRESULT WINAPI BmpDecoder_QueryCapability(IWICBitmapDecoder *iface, IStream *stream,
    DWORD *capability)
{
    HRESULT hr;
    BmpDecoder *This = impl_from_IWICBitmapDecoder(iface);

    TRACE("(%p,%p,%p)\n", iface, stream, capability);

    if (!stream || !capability) return E_INVALIDARG;

    hr = IWICBitmapDecoder_Initialize(iface, stream, WICDecodeMetadataCacheOnDemand);
    if (hr != S_OK) return hr;

    *capability = This->read_data_func == BmpFrameDecode_ReadUnsupported ? 0 : WICBitmapDecoderCapabilityCanDecodeAllImages;
    return S_OK;
}

static HRESULT WINAPI BmpDecoder_Initialize(IWICBitmapDecoder *iface, IStream *pIStream,
    WICDecodeOptions cacheOptions)
{
    HRESULT hr;
    BmpDecoder *This = impl_from_IWICBitmapDecoder(iface);

    EnterCriticalSection(&This->lock);
    hr = BmpDecoder_ReadHeaders(This, pIStream);

    if (SUCCEEDED(hr))
    {
        This->stream = pIStream;
        IStream_AddRef(pIStream);
    }
    LeaveCriticalSection(&This->lock);

    return hr;
}

static HRESULT WINAPI BmpDecoder_GetContainerFormat(IWICBitmapDecoder *iface,
    GUID *pguidContainerFormat)
{
    memcpy(pguidContainerFormat, &GUID_ContainerFormatBmp, sizeof(GUID));
    return S_OK;
}

static HRESULT WINAPI BmpDecoder_GetDecoderInfo(IWICBitmapDecoder *iface,
    IWICBitmapDecoderInfo **ppIDecoderInfo)
{
    TRACE("(%p,%p)\n", iface, ppIDecoderInfo);

    return get_decoder_info(&CLSID_WICBmpDecoder, ppIDecoderInfo);
}

static HRESULT WINAPI BmpDecoder_CopyPalette(IWICBitmapDecoder *iface,
    IWICPalette *pIPalette)
{
    TRACE("(%p,%p)\n", iface, pIPalette);

    return WINCODEC_ERR_PALETTEUNAVAILABLE;
}

static HRESULT WINAPI BmpDecoder_GetMetadataQueryReader(IWICBitmapDecoder *iface,
    IWICMetadataQueryReader **ppIMetadataQueryReader)
{
    TRACE("(%p,%p)\n", iface, ppIMetadataQueryReader);
    return WINCODEC_ERR_UNSUPPORTEDOPERATION;
}

static HRESULT WINAPI BmpDecoder_GetPreview(IWICBitmapDecoder *iface,
    IWICBitmapSource **ppIBitmapSource)
{
    TRACE("(%p,%p)\n", iface, ppIBitmapSource);
    return WINCODEC_ERR_UNSUPPORTEDOPERATION;
}

static HRESULT WINAPI BmpDecoder_GetColorContexts(IWICBitmapDecoder *iface,
    UINT cCount, IWICColorContext **ppIColorContexts, UINT *pcActualCount)
{
    TRACE("(%p,%u,%p,%p)\n", iface, cCount, ppIColorContexts, pcActualCount);
    return WINCODEC_ERR_UNSUPPORTEDOPERATION;
}

static HRESULT WINAPI BmpDecoder_GetThumbnail(IWICBitmapDecoder *iface,
    IWICBitmapSource **ppIThumbnail)
{
    TRACE("(%p,%p)\n", iface, ppIThumbnail);
    return WINCODEC_ERR_CODECNOTHUMBNAIL;
}

static HRESULT WINAPI BmpDecoder_GetFrameCount(IWICBitmapDecoder *iface,
    UINT *pCount)
{
    if (!pCount) return E_INVALIDARG;

    *pCount = 1;
    return S_OK;
}

static HRESULT WINAPI BmpDecoder_GetFrame(IWICBitmapDecoder *iface,
    UINT index, IWICBitmapFrameDecode **ppIBitmapFrame)
{
    BmpDecoder *This = impl_from_IWICBitmapDecoder(iface);

    if (index != 0) return E_INVALIDARG;

    if (!This->stream) return WINCODEC_ERR_FRAMEMISSING;

    *ppIBitmapFrame = &This->IWICBitmapFrameDecode_iface;
    IWICBitmapDecoder_AddRef(iface);

    return S_OK;
}

static const IWICBitmapDecoderVtbl BmpDecoder_Vtbl = {
    BmpDecoder_QueryInterface,
    BmpDecoder_AddRef,
    BmpDecoder_Release,
    BmpDecoder_QueryCapability,
    BmpDecoder_Initialize,
    BmpDecoder_GetContainerFormat,
    BmpDecoder_GetDecoderInfo,
    BmpDecoder_CopyPalette,
    BmpDecoder_GetMetadataQueryReader,
    BmpDecoder_GetPreview,
    BmpDecoder_GetColorContexts,
    BmpDecoder_GetThumbnail,
    BmpDecoder_GetFrameCount,
    BmpDecoder_GetFrame
};

static HRESULT BmpDecoder_Create(int packed, int icoframe, BmpDecoder **ppDecoder)
{
    BmpDecoder *This;

    This = malloc(sizeof(BmpDecoder));
    if (!This) return E_OUTOFMEMORY;

    This->IWICBitmapDecoder_iface.lpVtbl = &BmpDecoder_Vtbl;
    This->IWICBitmapFrameDecode_iface.lpVtbl = &BmpDecoder_FrameVtbl;
    This->ref = 1;
    This->initialized = FALSE;
    This->stream = NULL;
    This->imagedata = NULL;
#ifdef __REACTOS__
    InitializeCriticalSection(&This->lock);
#else
    InitializeCriticalSectionEx(&This->lock, 0, RTL_CRITICAL_SECTION_FLAG_FORCE_DEBUG_INFO);
#endif
    This->lock.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": BmpDecoder.lock");
    This->packed = packed;
    This->icoframe = icoframe;

    *ppDecoder = This;

    return S_OK;
}

static HRESULT BmpDecoder_Construct(int packed, int icoframe, REFIID iid, void** ppv)
{
    BmpDecoder *This;
    HRESULT ret;

    TRACE("(%s,%p)\n", debugstr_guid(iid), ppv);

    *ppv = NULL;

    ret = BmpDecoder_Create(packed, icoframe, &This);
    if (FAILED(ret)) return ret;

    ret = IWICBitmapDecoder_QueryInterface(&This->IWICBitmapDecoder_iface, iid, ppv);
    IWICBitmapDecoder_Release(&This->IWICBitmapDecoder_iface);

    return ret;
}

HRESULT BmpDecoder_CreateInstance(REFIID iid, void** ppv)
{
    return BmpDecoder_Construct(FALSE, FALSE, iid, ppv);
}

HRESULT DibDecoder_CreateInstance(REFIID iid, void** ppv)
{
    return BmpDecoder_Construct(TRUE, FALSE, iid, ppv);
}

HRESULT IcoDibDecoder_CreateInstance(BmpDecoder **ppDecoder)
{
    return BmpDecoder_Create(TRUE, TRUE, ppDecoder);
}

void BmpDecoder_GetWICDecoder(BmpDecoder *This, IWICBitmapDecoder **ppDecoder)
{
    *ppDecoder = &This->IWICBitmapDecoder_iface;
}

/* Return the offset where the mask of an icon might be, or 0 for failure. */
void BmpDecoder_FindIconMask(BmpDecoder *This, ULONG *mask_offset, int *topdown)
{
    assert(This->stream != NULL);

    if (This->read_data_func == BmpFrameDecode_ReadUncompressed)
    {
        /* RGB or BITFIELDS data */
        UINT width, height;
        ULONG bytesperrow, datasize;
        IWICBitmapFrameDecode_GetSize(&This->IWICBitmapFrameDecode_iface, &width, &height);
        bytesperrow = (((width * This->bitsperpixel)+31)/32)*4;
        datasize = bytesperrow * height;
        *mask_offset = This->image_offset + datasize;
    }
    else
        *mask_offset = 0;

    *topdown = This->stride > 0;
}
