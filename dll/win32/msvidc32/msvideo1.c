/*
 * Microsoft Video-1 Decoder
 * Copyright (C) 2003 the ffmpeg project
 *
 * Portions Copyright (C) 2004 Mike McCormack for CodeWeavers
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
 *
 */

/**
 * @file msvideo1.c
 * Microsoft Video-1 Decoder by Mike Melanson (melanson@pcisys.net)
 * For more information about the MS Video-1 format, visit:
 *   http://www.pcisys.net/~melanson/codecs/
 *
 * This decoder outputs either PAL8 or RGB555 data, depending on the
 * whether a RGB palette was passed through palctrl;
 * if it's present, then the data is PAL8; RGB555 otherwise.
 */

#include <stdarg.h>
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h" 
#include "commdlg.h"
#include "vfw.h"
#include "mmsystem.h"
#include "msvidc32_private.h"
 
#include "wine/debug.h"
 
WINE_DEFAULT_DEBUG_CHANNEL(msvidc32); 

static HINSTANCE MSVIDC32_hModule;

#define CRAM_MAGIC mmioFOURCC('C', 'R', 'A', 'M')
#define MSVC_MAGIC mmioFOURCC('M', 'S', 'V', 'C')
#define WHAM_MAGIC mmioFOURCC('W', 'H', 'A', 'M')
#define compare_fourcc(fcc1, fcc2) (((fcc1)^(fcc2))&~0x20202020)

#define PALETTE_COUNT 256
#define LE_16(x)  ((((const uint8_t *)(x))[1] << 8) | ((const uint8_t *)(x))[0])

/* FIXME - check the stream size */
#define CHECK_STREAM_PTR(n) \
  if ((stream_ptr + n) > buf_size ) { \
    WARN("stream_ptr out of bounds (%d >= %d)\n", \
      stream_ptr + n, buf_size); \
    return; \
  }

typedef BYTE uint8_t;

typedef struct Msvideo1Context {
    DWORD dwMagic;
    int depth;
} Msvideo1Context;

static inline int get_stride(int width, int depth)
{
    return ((depth * width + 31) >> 3) & ~3;
}

static void 
msvideo1_decode_8bit( int width, int height, const unsigned char *buf, int buf_size,
                      unsigned char *pixels, int stride)
{
    int block_ptr, pixel_ptr;
    int total_blocks;
    int pixel_x, pixel_y;  /* pixel width and height iterators */
    int block_x, block_y;  /* block width and height iterators */
    int blocks_wide, blocks_high;  /* width and height in 4x4 blocks */
    int block_inc;
    int row_dec;

    /* decoding parameters */
    int stream_ptr;
    unsigned char byte_a, byte_b;
    unsigned short flags;
    int skip_blocks;
    unsigned char colors[8];

    stream_ptr = 0;
    skip_blocks = 0;
    blocks_wide = width / 4;
    blocks_high = height / 4;
    total_blocks = blocks_wide * blocks_high;
    block_inc = 4;
#ifdef ORIGINAL
    row_dec = stride + 4;
#else
    row_dec = - (stride - 4); /* such that -row_dec > 0 */
#endif

    for (block_y = blocks_high; block_y > 0; block_y--) {
#ifdef ORIGINAL
        block_ptr = ((block_y * 4) - 1) * stride;
#else
        block_ptr = ((blocks_high - block_y) * 4) * stride;
#endif
        for (block_x = blocks_wide; block_x > 0; block_x--) {
            /* check if this block should be skipped */
            if (skip_blocks) {
                block_ptr += block_inc;
                skip_blocks--;
                total_blocks--;
                continue;
            }

            pixel_ptr = block_ptr;

            /* get the next two bytes in the encoded data stream */
            CHECK_STREAM_PTR(2);
            byte_a = buf[stream_ptr++];
            byte_b = buf[stream_ptr++];

            /* check if the decode is finished */
            if ((byte_a == 0) && (byte_b == 0) && (total_blocks == 0))
                return;
            else if ((byte_b & 0xFC) == 0x84) {
                /* skip code, but don't count the current block */
                skip_blocks = ((byte_b - 0x84) << 8) + byte_a - 1;
            } else if (byte_b < 0x80) {
                /* 2-color encoding */
                flags = (byte_b << 8) | byte_a;

                CHECK_STREAM_PTR(2);
                colors[0] = buf[stream_ptr++];
                colors[1] = buf[stream_ptr++];

                for (pixel_y = 0; pixel_y < 4; pixel_y++) {
                    for (pixel_x = 0; pixel_x < 4; pixel_x++, flags >>= 1)
                        pixels[pixel_ptr++] = colors[(flags & 0x1) ^ 1];
                    pixel_ptr -= row_dec;
                }
            } else if (byte_b >= 0x90) {
                /* 8-color encoding */
                flags = (byte_b << 8) | byte_a;

                CHECK_STREAM_PTR(8);
                memcpy(colors, &buf[stream_ptr], 8);
                stream_ptr += 8;

                for (pixel_y = 0; pixel_y < 4; pixel_y++) {
                    for (pixel_x = 0; pixel_x < 4; pixel_x++, flags >>= 1)
                        pixels[pixel_ptr++] =
                            colors[((pixel_y & 0x2) << 1) +
                                (pixel_x & 0x2) + ((flags & 0x1) ^ 1)];
                    pixel_ptr -= row_dec;
                }
            } else {
                /* 1-color encoding */
                colors[0] = byte_a;

                for (pixel_y = 0; pixel_y < 4; pixel_y++) {
                    for (pixel_x = 0; pixel_x < 4; pixel_x++)
                        pixels[pixel_ptr++] = colors[0];
                    pixel_ptr -= row_dec;
                }
            }

            block_ptr += block_inc;
            total_blocks--;
        }
    }
}

static void
msvideo1_decode_16bit( int width, int height, const unsigned char *buf, int buf_size,
                       unsigned short *pixels, int stride)
{
    int block_ptr, pixel_ptr;
    int total_blocks;
    int pixel_x, pixel_y;  /* pixel width and height iterators */
    int block_x, block_y;  /* block width and height iterators */
    int blocks_wide, blocks_high;  /* width and height in 4x4 blocks */
    int block_inc;
    int row_dec;

    /* decoding parameters */
    int stream_ptr;
    unsigned char byte_a, byte_b;
    unsigned short flags;
    int skip_blocks;
    unsigned short colors[8];

    stream_ptr = 0;
    skip_blocks = 0;
    blocks_wide = width / 4;
    blocks_high = height / 4;
    total_blocks = blocks_wide * blocks_high;
    block_inc = 4;
#ifdef ORIGINAL
    row_dec = stride + 4;
#else
    row_dec = - (stride - 4); /* such that -row_dec > 0 */
#endif

    for (block_y = blocks_high; block_y > 0; block_y--) {
#ifdef ORIGINAL
        block_ptr = ((block_y * 4) - 1) * stride;
#else
        block_ptr = ((blocks_high - block_y) * 4) * stride;
#endif
        for (block_x = blocks_wide; block_x > 0; block_x--) {
            /* check if this block should be skipped */
            if (skip_blocks) {
                block_ptr += block_inc;
                skip_blocks--;
                total_blocks--;
                continue;
            }

            pixel_ptr = block_ptr;

            /* get the next two bytes in the encoded data stream */
            CHECK_STREAM_PTR(2);
            byte_a = buf[stream_ptr++];
            byte_b = buf[stream_ptr++];

            /* check if the decode is finished */
            if ((byte_a == 0) && (byte_b == 0) && (total_blocks == 0)) {
                return;
            } else if ((byte_b & 0xFC) == 0x84) {
                /* skip code, but don't count the current block */
                skip_blocks = ((byte_b - 0x84) << 8) + byte_a - 1;
            } else if (byte_b < 0x80) {
                /* 2- or 8-color encoding modes */
                flags = (byte_b << 8) | byte_a;

                CHECK_STREAM_PTR(4);
                colors[0] = LE_16(&buf[stream_ptr]);
                stream_ptr += 2;
                colors[1] = LE_16(&buf[stream_ptr]);
                stream_ptr += 2;

                if (colors[0] & 0x8000) {
                    /* 8-color encoding */
                    CHECK_STREAM_PTR(12);
                    colors[2] = LE_16(&buf[stream_ptr]);
                    stream_ptr += 2;
                    colors[3] = LE_16(&buf[stream_ptr]);
                    stream_ptr += 2;
                    colors[4] = LE_16(&buf[stream_ptr]);
                    stream_ptr += 2;
                    colors[5] = LE_16(&buf[stream_ptr]);
                    stream_ptr += 2;
                    colors[6] = LE_16(&buf[stream_ptr]);
                    stream_ptr += 2;
                    colors[7] = LE_16(&buf[stream_ptr]);
                    stream_ptr += 2;

                    for (pixel_y = 0; pixel_y < 4; pixel_y++) {
                        for (pixel_x = 0; pixel_x < 4; pixel_x++, flags >>= 1)
                            pixels[pixel_ptr++] =
                                colors[((pixel_y & 0x2) << 1) +
                                    (pixel_x & 0x2) + ((flags & 0x1) ^ 1)];
                        pixel_ptr -= row_dec;
                    }
                } else {
                    /* 2-color encoding */
                    for (pixel_y = 0; pixel_y < 4; pixel_y++) {
                        for (pixel_x = 0; pixel_x < 4; pixel_x++, flags >>= 1)
                            pixels[pixel_ptr++] = colors[(flags & 0x1) ^ 1];
                        pixel_ptr -= row_dec;
                    }
                }
            } else {
                /* otherwise, it's a 1-color block */
                colors[0] = (byte_b << 8) | byte_a;

                for (pixel_y = 0; pixel_y < 4; pixel_y++) {
                    for (pixel_x = 0; pixel_x < 4; pixel_x++)
                        pixels[pixel_ptr++] = colors[0];
                    pixel_ptr -= row_dec;
                }
            }

            block_ptr += block_inc;
            total_blocks--;
        }
    }
}

static LRESULT
CRAM_DecompressQuery( Msvideo1Context *info, LPBITMAPINFO in, LPBITMAPINFO out )
{
    TRACE("ICM_DECOMPRESS_QUERY %p %p %p\n", info, in, out);

    if( (info==NULL) || (info->dwMagic!=CRAM_MAGIC) )
        return ICERR_BADPARAM;

    TRACE("in->planes  = %d\n", in->bmiHeader.biPlanes );
    TRACE("in->bpp     = %d\n", in->bmiHeader.biBitCount );
    TRACE("in->height  = %d\n", in->bmiHeader.biHeight );
    TRACE("in->width   = %d\n", in->bmiHeader.biWidth );
    TRACE("in->compr   = 0x%x\n", in->bmiHeader.biCompression );

    if( ( in->bmiHeader.biCompression != CRAM_MAGIC ) &&
        ( in->bmiHeader.biCompression != MSVC_MAGIC ) &&
        ( in->bmiHeader.biCompression != WHAM_MAGIC ) )
    {
        TRACE("can't do 0x%x compression\n", in->bmiHeader.biCompression);
        return ICERR_BADFORMAT;
    }

    if( ( in->bmiHeader.biBitCount != 16 ) &&
        ( in->bmiHeader.biBitCount != 8 ) )
    {
        TRACE("can't do %d bpp\n", in->bmiHeader.biBitCount );
        return ICERR_BADFORMAT;
    }

    /* output must be same dimensions as input */
    if( out )
    {
        TRACE("out->planes = %d\n", out->bmiHeader.biPlanes );
        TRACE("out->bpp    = %d\n", out->bmiHeader.biBitCount );
        TRACE("out->height = %d\n", out->bmiHeader.biHeight );
        TRACE("out->width  = %d\n", out->bmiHeader.biWidth );

        if ((in->bmiHeader.biBitCount != out->bmiHeader.biBitCount) &&
            (in->bmiHeader.biBitCount != 16 || out->bmiHeader.biBitCount != 24))
        {
            TRACE("incompatible depth requested\n");
            return ICERR_BADFORMAT;
        }

        if(( in->bmiHeader.biPlanes != out->bmiHeader.biPlanes ) ||
          ( in->bmiHeader.biHeight != out->bmiHeader.biHeight ) ||
          ( in->bmiHeader.biWidth != out->bmiHeader.biWidth ))
        {
            TRACE("incompatible output requested\n");
            return ICERR_BADFORMAT;
        }
    }

    TRACE("OK!\n");
    return ICERR_OK;
}

static LRESULT 
CRAM_DecompressGetFormat( Msvideo1Context *info, LPBITMAPINFO in, LPBITMAPINFO out )
{
    DWORD size;

    TRACE("ICM_DECOMPRESS_GETFORMAT %p %p %p\n", info, in, out);

    if( (info==NULL) || (info->dwMagic!=CRAM_MAGIC) )
        return ICERR_BADPARAM;

    size = in->bmiHeader.biSize;
    if (in->bmiHeader.biBitCount <= 8)
        size += in->bmiHeader.biClrUsed * sizeof(RGBQUAD);

    if (in->bmiHeader.biBitCount != 8 && in->bmiHeader.biBitCount != 16)
        return ICERR_BADFORMAT;

    if( out )
    {
        memcpy( out, in, size );
        out->bmiHeader.biWidth = in->bmiHeader.biWidth & ~1;
        out->bmiHeader.biHeight = in->bmiHeader.biHeight & ~1;
        out->bmiHeader.biCompression = BI_RGB;
        out->bmiHeader.biSizeImage = in->bmiHeader.biHeight *
                                     get_stride(out->bmiHeader.biWidth, out->bmiHeader.biBitCount);
        return ICERR_OK;
    }

    return size;
}

static LRESULT CRAM_DecompressBegin( Msvideo1Context *info, LPBITMAPINFO in, LPBITMAPINFO out )
{
    TRACE("ICM_DECOMPRESS_BEGIN %p %p %p\n", info, in, out);

    if( (info==NULL) || (info->dwMagic!=CRAM_MAGIC) )
        return ICERR_BADPARAM;

    TRACE("bitmap is %d bpp\n", in->bmiHeader.biBitCount);
    if( in->bmiHeader.biBitCount == 8 )
        info->depth = 8;
    else if( in->bmiHeader.biBitCount == 16 )
        info->depth = 16;
    else
    {
        info->depth = 0;
        FIXME("Unsupported output format %i\n", in->bmiHeader.biBitCount);
    }

    return ICERR_OK;
}

static void convert_depth(char *input, int depth_in, char *output, BITMAPINFOHEADER *out_hdr)
{
    int x, y;
    int stride_in  = get_stride(out_hdr->biWidth, depth_in);
    int stride_out = get_stride(out_hdr->biWidth, out_hdr->biBitCount);

    if (depth_in == 16 && out_hdr->biBitCount == 24)
    {
        static const unsigned char convert_5to8[] =
        {
            0x00, 0x08, 0x10, 0x19, 0x21, 0x29, 0x31, 0x3a,
            0x42, 0x4a, 0x52, 0x5a, 0x63, 0x6b, 0x73, 0x7b,
            0x84, 0x8c, 0x94, 0x9c, 0xa5, 0xad, 0xb5, 0xbd,
            0xc5, 0xce, 0xd6, 0xde, 0xe6, 0xef, 0xf7, 0xff,
        };

        for (y = 0; y < out_hdr->biHeight; y++)
        {
            WORD *src_row = (WORD *)(input + y * stride_in);
            char *out_row = output + y * stride_out;

            for (x = 0; x < out_hdr->biWidth; x++)
            {
                WORD pixel = *src_row++;
                *out_row++ = convert_5to8[(pixel & 0x7c00u) >> 10];
                *out_row++ = convert_5to8[(pixel & 0x03e0u) >> 5];
                *out_row++ = convert_5to8[(pixel & 0x001fu)];
            }
        }
    }
    else
        FIXME("Conversion from %d to %d bit unimplemented\n", depth_in, out_hdr->biBitCount);
}

static LRESULT CRAM_Decompress( Msvideo1Context *info, ICDECOMPRESS *icd, DWORD size )
{
    LONG width, height, stride, sz;
    void *output;

    TRACE("ICM_DECOMPRESS %p %p %d\n", info, icd, size);

    if( (info==NULL) || (info->dwMagic!=CRAM_MAGIC) )
        return ICERR_BADPARAM;

    /* FIXME: flags are ignored */

    width  = icd->lpbiInput->biWidth;
    height = icd->lpbiInput->biHeight;
    sz = icd->lpbiInput->biSizeImage;

    output = icd->lpOutput;

    if (icd->lpbiOutput->biBitCount != info->depth)
    {
        output = HeapAlloc(GetProcessHeap(), 0, icd->lpbiOutput->biWidth * icd->lpbiOutput->biHeight * info->depth / 8);
        if (!output) return ICERR_MEMORY;
    }

    if (info->depth == 8)
    {
        stride = get_stride(width, 8);
        msvideo1_decode_8bit( width, height, icd->lpInput, sz,
                              output, stride );
    }
    else
    {
        stride = get_stride(width, 16) / 2;
        msvideo1_decode_16bit( width, height, icd->lpInput, sz,
                               output, stride );
    }

    if (icd->lpbiOutput->biBitCount != info->depth)
    {
        convert_depth(output, info->depth, icd->lpOutput, icd->lpbiOutput);
        HeapFree(GetProcessHeap(), 0, output);
    }

    return ICERR_OK;
}

static LRESULT CRAM_DecompressEx( Msvideo1Context *info, ICDECOMPRESSEX *icd, DWORD size )
{
    LONG width, height, stride, sz;
    void *output;

    TRACE("ICM_DECOMPRESSEX %p %p %d\n", info, icd, size);

    if( (info==NULL) || (info->dwMagic!=CRAM_MAGIC) )
        return ICERR_BADPARAM;

    /* FIXME: flags are ignored */

    width  = icd->lpbiSrc->biWidth;
    height = icd->lpbiSrc->biHeight;
    sz = icd->lpbiSrc->biSizeImage;

    output = icd->lpDst;

    if (icd->lpbiDst->biBitCount != info->depth)
    {
        output = HeapAlloc(GetProcessHeap(), 0, icd->lpbiDst->biWidth * icd->lpbiDst->biHeight * info->depth / 8);
        if (!output) return ICERR_MEMORY;
    }

    if (info->depth == 8)
    {
        stride = get_stride(width, 8);
        msvideo1_decode_8bit( width, height, icd->lpSrc, sz, 
                              output, stride );
    }
    else
    {
        stride = get_stride(width, 16) / 2;
        msvideo1_decode_16bit( width, height, icd->lpSrc, sz,
                               output, stride );
    }

    if (icd->lpbiDst->biBitCount != info->depth)
    {
        convert_depth(output, info->depth, icd->lpDst, icd->lpbiDst);
        HeapFree(GetProcessHeap(), 0, output);
    }

    return ICERR_OK;
}

static LRESULT CRAM_GetInfo( const Msvideo1Context *info, ICINFO *icinfo, DWORD dwSize )
{
    if (!icinfo) return sizeof(ICINFO);
    if (dwSize < sizeof(ICINFO)) return 0;

    icinfo->dwSize = sizeof(ICINFO);
    icinfo->fccType = ICTYPE_VIDEO;
    icinfo->fccHandler = info ? info->dwMagic : CRAM_MAGIC;
    icinfo->dwFlags = 0;
    icinfo->dwVersion = ICVERSION;
    icinfo->dwVersionICM = ICVERSION;

    LoadStringW(MSVIDC32_hModule, IDS_NAME, icinfo->szName, sizeof(icinfo->szName)/sizeof(WCHAR));
    LoadStringW(MSVIDC32_hModule, IDS_DESCRIPTION, icinfo->szDescription, sizeof(icinfo->szDescription)/sizeof(WCHAR));
    /* msvfw32 will fill icinfo->szDriver for us */

    return sizeof(ICINFO);
}

/***********************************************************************
 *		DriverProc (MSVIDC32.@)
 */
LRESULT WINAPI CRAM_DriverProc( DWORD_PTR dwDriverId, HDRVR hdrvr, UINT msg,
                                LPARAM lParam1, LPARAM lParam2 )
{
    Msvideo1Context *info = (Msvideo1Context *) dwDriverId;
    LRESULT r = ICERR_UNSUPPORTED;

    TRACE("%ld %p %04x %08lx %08lx\n", dwDriverId, hdrvr, msg, lParam1, lParam2);

    switch( msg )
    {
    case DRV_LOAD:
        TRACE("Loaded\n");
        r = 1;
        break;

    case DRV_ENABLE:
        break;

    case DRV_OPEN:
    {
        ICINFO *icinfo = (ICINFO *)lParam2;

        TRACE("Opened\n");

        if (icinfo && compare_fourcc(icinfo->fccType, ICTYPE_VIDEO)) return 0;

        info = HeapAlloc( GetProcessHeap(), 0, sizeof (Msvideo1Context) );
        if( info )
        {
            memset( info, 0, sizeof *info );
            info->dwMagic = CRAM_MAGIC;
        }
        r = (LRESULT) info;
        break;
    }

    case DRV_CLOSE:
        HeapFree( GetProcessHeap(), 0, info );
        break;

    case DRV_DISABLE:
        break;

    case DRV_FREE:
        break;

    case ICM_GETINFO:
        r = CRAM_GetInfo( info, (ICINFO *)lParam1, (DWORD)lParam2 );
        break;

    case ICM_DECOMPRESS_QUERY:
        r = CRAM_DecompressQuery( info, (LPBITMAPINFO) lParam1,
                                       (LPBITMAPINFO) lParam2 );
        break;

    case ICM_DECOMPRESS_GET_FORMAT:
        r = CRAM_DecompressGetFormat( info, (LPBITMAPINFO) lParam1,
                                       (LPBITMAPINFO) lParam2 );
        break;

    case ICM_DECOMPRESS_GET_PALETTE:
        FIXME("ICM_DECOMPRESS_GET_PALETTE\n");
        break;

    case ICM_DECOMPRESSEX_QUERY:
        FIXME("ICM_DECOMPRESSEX_QUERY\n");
        break;

    case ICM_DECOMPRESS:
        r = CRAM_Decompress( info, (ICDECOMPRESS*) lParam1,
                                  (DWORD) lParam2 );
        break;

    case ICM_DECOMPRESS_BEGIN:
        r = CRAM_DecompressBegin( info, (LPBITMAPINFO) lParam1,
                                       (LPBITMAPINFO) lParam2 );
        break;

    case ICM_DECOMPRESSEX:
        r = CRAM_DecompressEx( info, (ICDECOMPRESSEX*) lParam1,
                                  (DWORD) lParam2 );
        break;

    case ICM_DECOMPRESS_END:
        r = ICERR_OK;
        break;

    case ICM_COMPRESS_QUERY:
        r = ICERR_BADFORMAT;
        /* fall through */
    case ICM_COMPRESS_GET_FORMAT:
    case ICM_COMPRESS_END:
    case ICM_COMPRESS:
        FIXME("compression not implemented\n");
        break;

    case ICM_CONFIGURE:
        break;

    default:
        FIXME("Unknown message: %04x %ld %ld\n", msg, lParam1, lParam2);
    }

    return r;
}

/***********************************************************************
 *		DllMain
 */
BOOL WINAPI DllMain(HINSTANCE hModule, DWORD dwReason, LPVOID lpReserved)
{
    TRACE("(%p,%d,%p)\n", hModule, dwReason, lpReserved);

    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hModule);
        MSVIDC32_hModule = hModule;
        break;
    }
    return TRUE;
}
