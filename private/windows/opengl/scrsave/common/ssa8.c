/******************************Module*Header*******************************\
* Module Name: ssa8.c
*
* Operations on .a8 files
*
* Copyright (c) 1995 Microsoft Corporation
*
\**************************************************************************/

#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "tk.h"
#include "sscommon.h"

#define ESCAPE 0
#define ESC_ENDLINE 0
#define ESC_ENDBITMAP 1
#define ESC_DELTA 2
#define ESC_RANDOM 3
#define RANDOM_COUNT(c) ((c)-(ESC_RANDOM-1))

#define COUNT_SIZE 256
#define MAXRUN (COUNT_SIZE-1)
#define MAXRND (RANDOM_COUNT(COUNT_SIZE)-1)

typedef unsigned char *HwMem;
typedef unsigned long HwMemSize;
#define HW_UNALIGNED UNALIGNED

static HwMemSize HwuRld(HwMem src, HwMem dest, HwMemSize stride)
{
    unsigned short code;
    unsigned char run, esc;
    size_t len;
    HwMem s;
    HwMem f;

    s = src;
    f = dest;
    for (;;)
    {
        code = *((unsigned short HW_UNALIGNED *)s)++;
        run = code & 0xff;
        esc = code >> 8;
        if (run == ESCAPE)
        {
            if (esc == ESC_ENDBITMAP)
            {
                break;
            }
            else if (esc == ESC_DELTA)
            {
                len = *((unsigned short HW_UNALIGNED *)s)++;
                while (len-- > 0)
                {
                    *f = 0;
                    f += stride;
                }
            }
            else if (esc >= ESC_RANDOM)
            {
                len = RANDOM_COUNT(esc);
                while (len-- > 0)
                {
                    *f = *s++;
                    f += stride;
                }
            }
        }
        else
        {
            while (run-- > 0)
            {
                *f = esc;
                f += stride;
            }
        }
    }
    
    return (HwMemSize)((ULONG_PTR)(s-src));
}

static HwMemSize HwuRldTo32(HwMem src, HwMem dest, HwMemSize stride,
                            DWORD *translate)
{
    unsigned short code;
    unsigned char run, esc;
    size_t len;
    HwMem s;
    DWORD *f, tran;

    s = src;
    f = (DWORD *)dest;
    for (;;)
    {
        code = *((unsigned short HW_UNALIGNED *)s)++;
        run = code & 0xff;
        esc = code >> 8;
        if (run == ESCAPE)
        {
            if (esc == ESC_ENDBITMAP)
            {
                break;
            }
            else if (esc == ESC_DELTA)
            {
                len = *((unsigned short HW_UNALIGNED *)s)++;
                while (len-- > 0)
                {
                    *f = translate[0];
                    f += stride;
                }
            }
            else if (esc >= ESC_RANDOM)
            {
                len = RANDOM_COUNT(esc);
                while (len-- > 0)
                {
                    *f = translate[*s++];
                    f += stride;
                }
            }
        }
        else
        {
            tran = translate[esc];
            while (run-- > 0)
            {
                *f = tran;
                f += stride;
            }
        }
    }
    
    return (HwMemSize)((ULONG_PTR)(s-src));
}

#define ALPHA_SIGNATURE 0xa0a1a2a3
#define COMPRESS_NONE 0
#define COMPRESS_RLE  1

BOOL ss_A8ImageLoad(void *pvResource, TEXTURE *ptex)
{
    DWORD compress;
    DWORD size;
    DWORD *pal;
    BYTE *buf;
    DWORD *pdwA8;

    pdwA8 = (DWORD *)pvResource;

    // Check data signature for alpha texture format
    if (*pdwA8 != ALPHA_SIGNATURE)
    {
        return FALSE;
    }
    pdwA8++;

    ptex->width = *pdwA8++;
    ptex->height = *pdwA8++;

    // Make sure depth is 8bpp
    if (*pdwA8 != 8)
    {
        return FALSE;
    }
    pdwA8++;
    
    size = ptex->width*ptex->height;

    // Compression type
    compress = *pdwA8++;
    // Compressed data size only if compressed, not used
    pdwA8++;

    // Remember pointer to palette data
    pal = pdwA8;
    pdwA8 += 256;

    if (ss_PalettedTextureEnabled())
    {
        // Allocate data for final image
        ptex->data = malloc(size);
        if (ptex->data == NULL)
        {
            return FALSE;
        }
    
        ptex->pal_size = 256;
        ptex->pal = malloc(ptex->pal_size*sizeof(RGBQUAD));
        if (ptex->pal == NULL)
        {
            free(ptex->data);
            return FALSE;
        }
        memcpy(ptex->pal, pal, ptex->pal_size*sizeof(RGBQUAD));

        // Unpack 8bpp data into final image
        if (compress == COMPRESS_NONE)
        {
            memcpy(ptex->data, pdwA8, size);
        }
        else
        {
            HwuRld((HwMem)pdwA8, ptex->data, 1);
        }

        ptex->format = GL_COLOR_INDEX;
        ptex->components = GL_COLOR_INDEX8_EXT;
    }
    else
    {
        // Allocate data for final image
        ptex->data = malloc(size*sizeof(DWORD));
        if (ptex->data == NULL)
        {
            return FALSE;
        }
    
        ptex->pal_size = 0;
        ptex->pal = NULL;

        // Unpack 8bpp data into final image
        if (compress == COMPRESS_NONE)
        {
            DWORD i;
            BYTE *src;
            DWORD *dst;

            src = (BYTE *)pdwA8;
            dst = (DWORD *)ptex->data;
            for (i = 0; i < size; i++)
            {
                *dst++ = pal[*src++];
            }
        }
        else
        {
            HwuRldTo32((HwMem)pdwA8, ptex->data, 1, pal);
        }

        ptex->format = GL_BGRA_EXT;
        ptex->components = 4;
    }
    
    return TRUE;
}
