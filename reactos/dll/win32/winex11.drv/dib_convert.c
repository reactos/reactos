/*
 * DIB conversion routinues for cases where the source and destination
 * have the same byte order.
 *
 * Copyright (C) 2001 Francois Gouget
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

#include "config.h"

#include <stdlib.h>

#include "windef.h"
#include "x11drv.h"


/***********************************************************************
 *           X11DRV_DIB_Convert_*
 *
 * All X11DRV_DIB_Convert_Xxx functions take at least the following
 * parameters:
 * - width
 *   This is the width in pixel of the surface to copy. This may be less
 *   than the full width of the image.
 * - height
 *   The number of lines to copy. This may be less than the full height
 *   of the image. This is always >0.
 * - srcbits
 *   Points to the first byte containing data to be copied. If the source
 *   surface starts at coordinates (x,y) then this is:
 *   image_ptr+x*bytes_per_pixel+y*bytes_per_line
 *   (with further adjustments for top-down/bottom-up images)
 * - srclinebytes
 *   This is the number of bytes per line. It may be >0 or <0 depending on
 *   whether this is a top-down or bottom-up image.
 * - dstbits
 *   Same as srcbits but for the destination
 * - dstlinebytes
 *   Same as srclinebytes but for the destination.
 *
 * Notes:
 * - The supported Dib formats are: pal1, pal4, pal8, rgb555, bgr555,
 *   rgb565, bgr565, rgb888 and any 32bit (0888) format.
 *   The supported XImage (Bmp) formats are: pal1, pal4, pal8,
 *   rgb555, bgr555, rgb565, bgr565, rgb888, bgr888, rgb0888, bgr0888.
 * - Rgb formats are those for which the masks are such that:
 *   red_mask > green_mask > blue_mask
 * - Bgr formats are those for which the masks sort in the other direction.
 * - Many conversion functions handle both rgb->bgr and bgr->rgb conversions
 *   so the comments use h, g, l to mean respectively the source color in the
 *   high bits, the green, and the source color in the low bits.
 */


/*
 * 15 bit conversions
 */

static void convert_5x5_asis(int width, int height,
                             const void* srcbits, int srclinebytes,
                             void* dstbits, int dstlinebytes)
{
    int y;

    width *= 2;

    if (srclinebytes == dstlinebytes && srclinebytes == width)
    {
        memcpy(dstbits, srcbits, height * width);
        return;
    }

    for (y=0; y<height; y++) {
        memcpy(dstbits, srcbits, width);
        srcbits = (const char*)srcbits + srclinebytes;
        dstbits = (char*)dstbits + dstlinebytes;
    }
}


static void convert_555_reverse(int width, int height,
                                const void* srcbits, int srclinebytes,
                                void* dstbits, int dstlinebytes)
{
    const DWORD* srcpixel;
    DWORD* dstpixel;
    int x,y;

    for (y=0; y<height; y++) {
        srcpixel=srcbits;
        dstpixel=dstbits;
        for (x=0; x<width/2; x++) {
            /* Do 2 pixels at a time */
            DWORD srcval;
            srcval=*srcpixel++;
            *dstpixel++=((srcval << 10) & 0x7c007c00) | /* h */
                        ( srcval        & 0x03e003e0) | /* g */
                        ((srcval >> 10) & 0x001f001f);  /* l */
        }
        if (width&1) {
            /* And then the odd pixel */
            WORD srcval;
            srcval=*((const WORD*)srcpixel);
            *((WORD*)dstpixel)=((srcval << 10) & 0x7c00) | /* h */
                               ( srcval        & 0x03e0) | /* g */
                               ((srcval >> 10) & 0x001f);  /* l */
        }
        srcbits = (const char*)srcbits + srclinebytes;
        dstbits = (char*)dstbits + dstlinebytes;
    }
}

static void convert_555_to_565_asis(int width, int height,
                                    const void* srcbits, int srclinebytes,
                                    void* dstbits, int dstlinebytes)
{
    const DWORD* srcpixel;
    DWORD* dstpixel;
    int x,y;

    for (y=0; y<height; y++) {
        srcpixel=srcbits;
        dstpixel=dstbits;
        for (x=0; x<width/2; x++) {
            /* Do 2 pixels at a time */
            DWORD srcval;
            srcval=*srcpixel++;
            *dstpixel++=((srcval << 1) & 0xffc0ffc0) | /* h, g */
                        ((srcval >> 4) & 0x00200020) | /* g - 1 bit */
                        ( srcval       & 0x001f001f);  /* l */
        }
        if (width&1) {
            /* And then the odd pixel */
            WORD srcval;
            srcval=*((const WORD*)srcpixel);
            *((WORD*)dstpixel)=((srcval << 1) & 0xffc0) | /* h, g */
                               ((srcval >> 4) & 0x0020) | /* g - 1 bit */
                                (srcval       & 0x001f);  /* l */
        }
        srcbits = (const char*)srcbits + srclinebytes;
        dstbits = (char*)dstbits + dstlinebytes;
    }
}

static void convert_555_to_565_reverse(int width, int height,
                                       const void* srcbits, int srclinebytes,
                                       void* dstbits, int dstlinebytes)
{
    const DWORD* srcpixel;
    DWORD* dstpixel;
    int x,y;

    for (y=0; y<height; y++) {
        srcpixel=srcbits;
        dstpixel=dstbits;
        for (x=0; x<width/2; x++) {
            /* Do 2 pixels at a time */
            DWORD srcval;
            srcval=*srcpixel++;
            *dstpixel++=((srcval >> 10) & 0x001f001f) | /* h */
                        ((srcval <<  1) & 0x07c007c0) | /* g */
                        ((srcval >>  4) & 0x00200020) | /* g - 1 bit */
                        ((srcval << 11) & 0xf800f800);  /* l */
        }
        if (width&1) {
            /* And then the odd pixel */
            WORD srcval;
            srcval=*((const WORD*)srcpixel);
            *((WORD*)dstpixel)=((srcval >> 10) & 0x001f) | /* h */
                               ((srcval <<  1) & 0x07c0) | /* g */
                               ((srcval >>  4) & 0x0020) | /* g - 1 bit */
                               ((srcval << 11) & 0xf800);  /* l */
        }
        srcbits = (const char*)srcbits + srclinebytes;
        dstbits = (char*)dstbits + dstlinebytes;
    }
}

static void convert_555_to_888_asis(int width, int height,
                                    const void* srcbits, int srclinebytes,
                                    void* dstbits, int dstlinebytes)
{
    const WORD* srcpixel;
    BYTE* dstpixel;
    int x,y;

    for (y=0; y<height; y++) {
        srcpixel=srcbits;
        dstpixel=dstbits;
        for (x=0; x<width; x++) {
            WORD srcval;
            srcval=*srcpixel++;
            dstpixel[0]=((srcval <<  3) & 0xf8) | /* l */
                        ((srcval >>  2) & 0x07);  /* l - 3 bits */
            dstpixel[1]=((srcval >>  2) & 0xf8) | /* g */
                        ((srcval >>  7) & 0x07);  /* g - 3 bits */
            dstpixel[2]=((srcval >>  7) & 0xf8) | /* h */
                        ((srcval >> 12) & 0x07);  /* h - 3 bits */
            dstpixel+=3;
        }
        srcbits = (const char*)srcbits + srclinebytes;
        dstbits = (char*)dstbits + dstlinebytes;
    }
}

static void convert_555_to_888_reverse(int width, int height,
                                       const void* srcbits, int srclinebytes,
                                       void* dstbits, int dstlinebytes)
{
    const WORD* srcpixel;
    BYTE* dstpixel;
    int x,y;

    for (y=0; y<height; y++) {
        srcpixel=srcbits;
        dstpixel=dstbits;
        for (x=0; x<width; x++) {
            WORD srcval;
            srcval=*srcpixel++;
            dstpixel[0]=((srcval >>  7) & 0xf8) | /* h */
                        ((srcval >> 12) & 0x07);  /* h - 3 bits */
            dstpixel[1]=((srcval >>  2) & 0xf8) | /* g */
                        ((srcval >>  7) & 0x07);  /* g - 3 bits */
            dstpixel[2]=((srcval <<  3) & 0xf8) | /* l */
                        ((srcval >>  2) & 0x07);  /* l - 3 bits */
            dstpixel+=3;
        }
        srcbits = (const char*)srcbits + srclinebytes;
        dstbits = (char*)dstbits + dstlinebytes;
    }
}

static void convert_555_to_0888_asis(int width, int height,
                                     const void* srcbits, int srclinebytes,
                                     void* dstbits, int dstlinebytes)
{
    const WORD* srcpixel;
    DWORD* dstpixel;
    int x,y;

    for (y=0; y<height; y++) {
        srcpixel=srcbits;
        dstpixel=dstbits;
        for (x=0; x<width; x++) {
            WORD srcval;
            srcval=*srcpixel++;
            *dstpixel++=((srcval << 9) & 0xf80000) | /* h */
                        ((srcval << 4) & 0x070000) | /* h - 3 bits */
                        ((srcval << 6) & 0x00f800) | /* g */
                        ((srcval << 1) & 0x000700) | /* g - 3 bits */
                        ((srcval << 3) & 0x0000f8) | /* l */
                        ((srcval >> 2) & 0x000007);  /* l - 3 bits */
        }
        srcbits = (const char*)srcbits + srclinebytes;
        dstbits = (char*)dstbits + dstlinebytes;
    }
}

static void convert_555_to_0888_reverse(int width, int height,
                                        const void* srcbits, int srclinebytes,
                                        void* dstbits, int dstlinebytes)
{
    const WORD* srcpixel;
    DWORD* dstpixel;
    int x,y;

    for (y=0; y<height; y++) {
        srcpixel=srcbits;
        dstpixel=dstbits;
        for (x=0; x<width; x++) {
            WORD srcval;
            srcval=*srcpixel++;
            *dstpixel++=((srcval >>  7) & 0x0000f8) | /* h */
                        ((srcval >> 12) & 0x000007) | /* h - 3 bits */
                        ((srcval <<  6) & 0x00f800) | /* g */
                        ((srcval <<  1) & 0x000700) | /* g - 3 bits */
                        ((srcval << 19) & 0xf80000) | /* l */
                        ((srcval << 14) & 0x070000);  /* l - 3 bits */
        }
        srcbits = (const char*)srcbits + srclinebytes;
        dstbits = (char*)dstbits + dstlinebytes;
    }
}

static void convert_5x5_to_any0888(int width, int height,
                                   const void* srcbits, int srclinebytes,
                                   WORD rsrc, WORD gsrc, WORD bsrc,
                                   void* dstbits, int dstlinebytes,
                                   DWORD rdst, DWORD gdst, DWORD bdst)
{
    int rRightShift1,gRightShift1,bRightShift1;
    int rRightShift2,gRightShift2,bRightShift2;
    BYTE gMask1,gMask2;
    int rLeftShift,gLeftShift,bLeftShift;
    const WORD* srcpixel;
    DWORD* dstpixel;
    int x,y;

    /* Note, the source pixel value is shifted left by 16 bits so that
     * we know we will always have to shift right to extract the components.
     */
    rRightShift1=16+X11DRV_DIB_MaskToShift(rsrc)-3;
    gRightShift1=16+X11DRV_DIB_MaskToShift(gsrc)-3;
    bRightShift1=16+X11DRV_DIB_MaskToShift(bsrc)-3;
    rRightShift2=rRightShift1+5;
    gRightShift2=gRightShift1+5;
    bRightShift2=bRightShift1+5;
    if (gsrc==0x03e0) {
        /* Green has 5 bits, like the others */
        gMask1=0xf8;
        gMask2=0x07;
    } else {
        /* Green has 6 bits, not 5. Compensate. */
        gRightShift1++;
        gRightShift2+=2;
        gMask1=0xfc;
        gMask2=0x03;
    }

    rLeftShift=X11DRV_DIB_MaskToShift(rdst);
    gLeftShift=X11DRV_DIB_MaskToShift(gdst);
    bLeftShift=X11DRV_DIB_MaskToShift(bdst);

    for (y=0; y<height; y++) {
        srcpixel=srcbits;
        dstpixel=dstbits;
        for (x=0; x<width; x++) {
            DWORD srcval;
            BYTE red,green,blue;
            srcval=*srcpixel++ << 16;
            red=  ((srcval >> rRightShift1) & 0xf8) |
                  ((srcval >> rRightShift2) & 0x07);
            green=((srcval >> gRightShift1) & gMask1) |
                  ((srcval >> gRightShift2) & gMask2);
            blue= ((srcval >> bRightShift1) & 0xf8) |
                  ((srcval >> bRightShift2) & 0x07);
            *dstpixel++=(red   << rLeftShift) |
                        (green << gLeftShift) |
                        (blue  << bLeftShift);
        }
        srcbits = (const char*)srcbits + srclinebytes;
        dstbits = (char*)dstbits + dstlinebytes;
    }
}

/*
 * 16 bits conversions
 */

static void convert_565_reverse(int width, int height,
                                const void* srcbits, int srclinebytes,
                                void* dstbits, int dstlinebytes)
{
    const DWORD* srcpixel;
    DWORD* dstpixel;
    int x,y;

    for (y=0; y<height; y++) {
        srcpixel=srcbits;
        dstpixel=dstbits;
        for (x=0; x<width/2; x++) {
            /* Do 2 pixels at a time */
            DWORD srcval;
            srcval=*srcpixel++;
            *dstpixel++=((srcval << 11) & 0xf800f800) | /* h */
                        ( srcval        & 0x07e007e0) | /* g */
                        ((srcval >> 11) & 0x001f001f);  /* l */
        }
        if (width&1) {
            /* And then the odd pixel */
            WORD srcval;
            srcval=*((const WORD*)srcpixel);
            *((WORD*)dstpixel)=((srcval << 11) & 0xf800) | /* h */
                               ( srcval        & 0x07e0) | /* g */
                               ((srcval >> 11) & 0x001f);  /* l */
        }
        srcbits = (const char*)srcbits + srclinebytes;
        dstbits = (char*)dstbits + dstlinebytes;
    }
}

static void convert_565_to_555_asis(int width, int height,
                                    const void* srcbits, int srclinebytes,
                                    void* dstbits, int dstlinebytes)
{
    const DWORD* srcpixel;
    DWORD* dstpixel;
    int x,y;

    for (y=0; y<height; y++) {
        srcpixel=srcbits;
        dstpixel=dstbits;
        for (x=0; x<width/2; x++) {
            /* Do 2 pixels at a time */
            DWORD srcval;
            srcval=*srcpixel++;
            *dstpixel++=((srcval >> 1) & 0x7fe07fe0) | /* h, g */
                        ( srcval       & 0x001f001f);  /* l */
        }
        if (width&1) {
            /* And then the odd pixel */
            WORD srcval;
            srcval=*((const WORD*)srcpixel);
            *((WORD*)dstpixel)=((srcval >> 1) & 0x7fe0) | /* h, g */
                               ( srcval       & 0x001f);  /* l */
        }
        srcbits = (const char*)srcbits + srclinebytes;
        dstbits = (char*)dstbits + dstlinebytes;
    }
}

static void convert_565_to_555_reverse(int width, int height,
                                       const void* srcbits, int srclinebytes,
                                       void* dstbits, int dstlinebytes)
{
    const DWORD* srcpixel;
    DWORD* dstpixel;
    int x,y;

    for (y=0; y<height; y++) {
        srcpixel=srcbits;
        dstpixel=dstbits;
        for (x=0; x<width/2; x++) {
            /* Do 2 pixels at a time */
            DWORD srcval;
            srcval=*srcpixel++;
            *dstpixel++=((srcval >> 11) & 0x001f001f) | /* h */
                        ((srcval >>  1) & 0x03e003e0) | /* g */
                        ((srcval << 10) & 0x7c007c00);  /* l */
        }
        if (width&1) {
            /* And then the odd pixel */
            WORD srcval;
            srcval=*((const WORD*)srcpixel);
            *((WORD*)dstpixel)=((srcval >> 11) & 0x001f) | /* h */
                               ((srcval >>  1) & 0x03e0) | /* g */
                               ((srcval << 10) & 0x7c00);  /* l */
        }
        srcbits = (const char*)srcbits + srclinebytes;
        dstbits = (char*)dstbits + dstlinebytes;
    }
}

static void convert_565_to_888_asis(int width, int height,
                                    const void* srcbits, int srclinebytes,
                                    void* dstbits, int dstlinebytes)
{
    const WORD* srcpixel;
    BYTE* dstpixel;
    int x,y;

    for (y=0; y<height; y++) {
        srcpixel=srcbits;
        dstpixel=dstbits;
        for (x=0; x<width; x++) {
            WORD srcval;
            srcval=*srcpixel++;
            dstpixel[0]=((srcval <<  3) & 0xf8) | /* l */
                        ((srcval >>  2) & 0x07);  /* l - 3 bits */
            dstpixel[1]=((srcval >>  3) & 0xfc) | /* g */
                        ((srcval >>  9) & 0x03);  /* g - 2 bits */
            dstpixel[2]=((srcval >>  8) & 0xf8) | /* h */
                        ((srcval >> 13) & 0x07);  /* h - 3 bits */
            dstpixel+=3;
        }
        srcbits = (const char*)srcbits + srclinebytes;
        dstbits = (char*)dstbits + dstlinebytes;
    }
}

static void convert_565_to_888_reverse(int width, int height,
                                       const void* srcbits, int srclinebytes,
                                       void* dstbits, int dstlinebytes)
{
    const WORD* srcpixel;
    BYTE* dstpixel;
    int x,y;

    for (y=0; y<height; y++) {
        srcpixel=srcbits;
        dstpixel=dstbits;
        for (x=0; x<width; x++) {
            WORD srcval;
            srcval=*srcpixel++;
            dstpixel[0]=((srcval >>  8) & 0xf8) | /* h */
                        ((srcval >> 13) & 0x07);  /* h - 3 bits */
            dstpixel[1]=((srcval >>  3) & 0xfc) | /* g */
                        ((srcval >>  9) & 0x03);  /* g - 2 bits */
            dstpixel[2]=((srcval <<  3) & 0xf8) | /* l */
                        ((srcval >>  2) & 0x07);  /* l - 3 bits */
            dstpixel+=3;
        }
        srcbits = (const char*)srcbits + srclinebytes;
        dstbits = (char*)dstbits + dstlinebytes;
    }
}

static void convert_565_to_0888_asis(int width, int height,
                                     const void* srcbits, int srclinebytes,
                                     void* dstbits, int dstlinebytes)
{
    const WORD* srcpixel;
    DWORD* dstpixel;
    int x,y;

    for (y=0; y<height; y++) {
        srcpixel=srcbits;
        dstpixel=dstbits;
        for (x=0; x<width; x++) {
            WORD srcval;
            srcval=*srcpixel++;
            *dstpixel++=((srcval << 8) & 0xf80000) | /* h */
                        ((srcval << 3) & 0x070000) | /* h - 3 bits */
                        ((srcval << 5) & 0x00fc00) | /* g */
                        ((srcval >> 1) & 0x000300) | /* g - 2 bits */
                        ((srcval << 3) & 0x0000f8) | /* l */
                        ((srcval >> 2) & 0x000007);  /* l - 3 bits */
        }
        srcbits = (const char*)srcbits + srclinebytes;
        dstbits = (char*)dstbits + dstlinebytes;
    }
}

static void convert_565_to_0888_reverse(int width, int height,
                                        const void* srcbits, int srclinebytes,
                                        void* dstbits, int dstlinebytes)
{
    const WORD* srcpixel;
    DWORD* dstpixel;
    int x,y;

    for (y=0; y<height; y++) {
        srcpixel=srcbits;
        dstpixel=dstbits;
        for (x=0; x<width; x++) {
            WORD srcval;
            srcval=*srcpixel++;
            *dstpixel++=((srcval >>  8) & 0x0000f8) | /* h */
                        ((srcval >> 13) & 0x000007) | /* h - 3 bits */
                        ((srcval <<  5) & 0x00fc00) | /* g */
                        ((srcval >>  1) & 0x000300) | /* g - 2 bits */
                        ((srcval << 19) & 0xf80000) | /* l */
                        ((srcval << 14) & 0x070000);  /* l - 3 bits */
        }
        srcbits = (const char*)srcbits + srclinebytes;
        dstbits = (char*)dstbits + dstlinebytes;
    }
}


/*
 * 24 bit conversions
 */

static void convert_888_asis(int width, int height,
                             const void* srcbits, int srclinebytes,
                             void* dstbits, int dstlinebytes)
{
    int y;

    width *= 3;

    if (srclinebytes == dstlinebytes && srclinebytes == width)
    {
        memcpy(dstbits, srcbits, height * width);
        return;
    }

    for (y=0; y<height; y++) {
        memcpy(dstbits, srcbits, width);
        srcbits = (const char*)srcbits + srclinebytes;
        dstbits = (char*)dstbits + dstlinebytes;
    }
}


static void convert_888_reverse(int width, int height,
                                const void* srcbits, int srclinebytes,
                                void* dstbits, int dstlinebytes)
{
    const BYTE* srcpixel;
    BYTE* dstpixel;
    int x,y;

    for (y=0; y<height; y++) {
        srcpixel=srcbits;
        dstpixel=dstbits;
        for (x=0; x<width; x++) {
            dstpixel[0]=srcpixel[2];
            dstpixel[1]=srcpixel[1];
            dstpixel[2]=srcpixel[0];
            srcpixel+=3;
            dstpixel+=3;
        }
        srcbits = (const char*)srcbits + srclinebytes;
        dstbits = (char*)dstbits + dstlinebytes;
    }
}

static void convert_888_to_555_asis(int width, int height,
                                    const void* srcbits, int srclinebytes,
                                    void* dstbits, int dstlinebytes)
{
    const DWORD* srcpixel;
    const BYTE* srcbyte;
    WORD* dstpixel;
    int x,y;
    int oddwidth;

    oddwidth=width & 3;
    width=width/4;
    for (y=0; y<height; y++) {
        srcpixel=srcbits;
        dstpixel=dstbits;
        for (x=0; x<width; x++) {
            /* Do 4 pixels at a time: 3 dwords in and 4 words out */
            DWORD srcval1,srcval2;
            srcval1=srcpixel[0];
            dstpixel[0]=((srcval1 >>  3) & 0x001f) | /* l1 */
                        ((srcval1 >>  6) & 0x03e0) | /* g1 */
                        ((srcval1 >>  9) & 0x7c00);  /* h1 */
            srcval2=srcpixel[1];
            dstpixel[1]=((srcval1 >> 27) & 0x001f) | /* l2 */
                        ((srcval2 <<  2) & 0x03e0) | /* g2 */
                        ((srcval2 >>  1) & 0x7c00);  /* h2 */
            srcval1=srcpixel[2];
            dstpixel[2]=((srcval2 >> 19) & 0x001f) | /* l3 */
                        ((srcval2 >> 22) & 0x03e0) | /* g3 */
                        ((srcval1 <<  7) & 0x7c00);  /* h3 */
            dstpixel[3]=((srcval1 >> 11) & 0x001f) | /* l4 */
                        ((srcval1 >> 14) & 0x03e0) | /* g4 */
                        ((srcval1 >> 17) & 0x7c00);  /* h4 */
            srcpixel+=3;
            dstpixel+=4;
        }
        /* And now up to 3 odd pixels */
        srcbyte=(const BYTE*)srcpixel;
        for (x=0; x<oddwidth; x++) {
            WORD dstval;
            dstval =((srcbyte[0] >> 3) & 0x001f);    /* l */
            dstval|=((srcbyte[1] << 2) & 0x03e0);    /* g */
            dstval|=((srcbyte[2] << 7) & 0x7c00);    /* h */
            *dstpixel++=dstval;
            srcbyte+=3;
        }
        srcbits = (const char*)srcbits + srclinebytes;
        dstbits = (char*)dstbits + dstlinebytes;
    }
}

static void convert_888_to_555_reverse(int width, int height,
                                       const void* srcbits, int srclinebytes,
                                       void* dstbits, int dstlinebytes)
{
    const DWORD* srcpixel;
    const BYTE* srcbyte;
    WORD* dstpixel;
    int x,y;
    int oddwidth;

    oddwidth=width & 3;
    width=width/4;
    for (y=0; y<height; y++) {
        srcpixel=srcbits;
        dstpixel=dstbits;
        for (x=0; x<width; x++) {
            /* Do 4 pixels at a time: 3 dwords in and 4 words out */
            DWORD srcval1,srcval2;
            srcval1=srcpixel[0];
            dstpixel[0]=((srcval1 <<  7) & 0x7c00) | /* l1 */
                        ((srcval1 >>  6) & 0x03e0) | /* g1 */
                        ((srcval1 >> 19) & 0x001f);  /* h1 */
            srcval2=srcpixel[1];
            dstpixel[1]=((srcval1 >> 17) & 0x7c00) | /* l2 */
                        ((srcval2 <<  2) & 0x03e0) | /* g2 */
                        ((srcval2 >> 11) & 0x001f);  /* h2 */
            srcval1=srcpixel[2];
            dstpixel[2]=((srcval2 >>  9) & 0x7c00) | /* l3 */
                        ((srcval2 >> 22) & 0x03e0) | /* g3 */
                        ((srcval1 >>  3) & 0x001f);  /* h3 */
            dstpixel[3]=((srcval1 >>  1) & 0x7c00) | /* l4 */
                        ((srcval1 >> 14) & 0x03e0) | /* g4 */
                        ((srcval1 >> 27) & 0x001f);  /* h4 */
            srcpixel+=3;
            dstpixel+=4;
        }
        /* And now up to 3 odd pixels */
        srcbyte=(const BYTE*)srcpixel;
        for (x=0; x<oddwidth; x++) {
            WORD dstval;
            dstval =((srcbyte[0] << 7) & 0x7c00);    /* l */
            dstval|=((srcbyte[1] << 2) & 0x03e0);    /* g */
            dstval|=((srcbyte[2] >> 3) & 0x001f);    /* h */
            *dstpixel++=dstval;
            srcbyte+=3;
        }
        srcbits = (const char*)srcbits + srclinebytes;
        dstbits = (char*)dstbits + dstlinebytes;
    }
}

static void convert_888_to_565_asis(int width, int height,
                                    const void* srcbits, int srclinebytes,
                                    void* dstbits, int dstlinebytes)
{
    const DWORD* srcpixel;
    const BYTE* srcbyte;
    WORD* dstpixel;
    int x,y;
    int oddwidth;

    oddwidth=width & 3;
    width=width/4;
    for (y=0; y<height; y++) {
        srcpixel=srcbits;
        dstpixel=dstbits;
        for (x=0; x<width; x++) {
            /* Do 4 pixels at a time: 3 dwords in and 4 words out */
            DWORD srcval1,srcval2;
            srcval1=srcpixel[0];
            dstpixel[0]=((srcval1 >>  3) & 0x001f) | /* l1 */
                        ((srcval1 >>  5) & 0x07e0) | /* g1 */
                        ((srcval1 >>  8) & 0xf800);  /* h1 */
            srcval2=srcpixel[1];
            dstpixel[1]=((srcval1 >> 27) & 0x001f) | /* l2 */
                        ((srcval2 <<  3) & 0x07e0) | /* g2 */
                        ( srcval2        & 0xf800);  /* h2 */
            srcval1=srcpixel[2];
            dstpixel[2]=((srcval2 >> 19) & 0x001f) | /* l3 */
                        ((srcval2 >> 21) & 0x07e0) | /* g3 */
                        ((srcval1 <<  8) & 0xf800);  /* h3 */
            dstpixel[3]=((srcval1 >> 11) & 0x001f) | /* l4 */
                        ((srcval1 >> 13) & 0x07e0) | /* g4 */
                        ((srcval1 >> 16) & 0xf800);  /* h4 */
            srcpixel+=3;
            dstpixel+=4;
        }
        /* And now up to 3 odd pixels */
        srcbyte=(const BYTE*)srcpixel;
        for (x=0; x<oddwidth; x++) {
            WORD dstval;
            dstval =((srcbyte[0] >> 3) & 0x001f);    /* l */
            dstval|=((srcbyte[1] << 3) & 0x07e0);    /* g */
            dstval|=((srcbyte[2] << 8) & 0xf800);    /* h */
            *dstpixel++=dstval;
            srcbyte+=3;
        }
        srcbits = (const char*)srcbits + srclinebytes;
        dstbits = (char*)dstbits + dstlinebytes;
    }
}

static void convert_888_to_565_reverse(int width, int height,
                                       const void* srcbits, int srclinebytes,
                                       void* dstbits, int dstlinebytes)
{
    const DWORD* srcpixel;
    const BYTE* srcbyte;
    WORD* dstpixel;
    int x,y;
    int oddwidth;

    oddwidth=width & 3;
    width=width/4;
    for (y=0; y<height; y++) {
        srcpixel=srcbits;
        dstpixel=dstbits;
        for (x=0; x<width; x++) {
            /* Do 4 pixels at a time: 3 dwords in and 4 words out */
            DWORD srcval1,srcval2;
            srcval1=srcpixel[0];
            dstpixel[0]=((srcval1 <<  8) & 0xf800) | /* l1 */
                        ((srcval1 >>  5) & 0x07e0) | /* g1 */
                        ((srcval1 >> 19) & 0x001f);  /* h1 */
            srcval2=srcpixel[1];
            dstpixel[1]=((srcval1 >> 16) & 0xf800) | /* l2 */
                        ((srcval2 <<  3) & 0x07e0) | /* g2 */
                        ((srcval2 >> 11) & 0x001f);  /* h2 */
            srcval1=srcpixel[2];
            dstpixel[2]=((srcval2 >>  8) & 0xf800) | /* l3 */
                        ((srcval2 >> 21) & 0x07e0) | /* g3 */
                        ((srcval1 >>  3) & 0x001f);  /* h3 */
            dstpixel[3]=(srcval1         & 0xf800) | /* l4 */
                        ((srcval1 >> 13) & 0x07e0) | /* g4 */
                        ((srcval1 >> 27) & 0x001f);  /* h4 */
            srcpixel+=3;
            dstpixel+=4;
        }
        /* And now up to 3 odd pixels */
        srcbyte=(const BYTE*)srcpixel;
        for (x=0; x<oddwidth; x++) {
            WORD dstval;
            dstval =((srcbyte[0] << 8) & 0xf800);    /* l */
            dstval|=((srcbyte[1] << 3) & 0x07e0);    /* g */
            dstval|=((srcbyte[2] >> 3) & 0x001f);    /* h */
            *dstpixel++=dstval;
            srcbyte+=3;
        }
        srcbits = (const char*)srcbits + srclinebytes;
        dstbits = (char*)dstbits + dstlinebytes;
    }
}

static void convert_888_to_0888_asis(int width, int height,
                                     const void* srcbits, int srclinebytes,
                                     void* dstbits, int dstlinebytes)
{
    const DWORD* srcpixel;
    DWORD* dstpixel;
    int x,y;
    int w1, w2, w3;
    
    w1 = min( (INT_PTR)srcbits & 3, width);
    w2 = ( width - w1) / 4;
    w3 = ( width - w1) & 3;
    for (y=0; y<height; y++) {
        srcpixel=srcbits;
        dstpixel=dstbits;
        /* advance  w1 pixels to make srcpixel 32 bit aligned */
        srcpixel = (const DWORD*)((INT_PTR)srcpixel & ~3);
        srcpixel += w1;
        dstpixel += w1;
        /* and do the w1 pixels */
        x = w1;
        if( x) {
            dstpixel[ -1] = ( srcpixel[ -1] >>  8);               /* h4, g4, l4 */
            if( --x) {
                dstpixel[ -2] = ( srcpixel[ -2] >> 16) |              /* g3, l3 */
                                ((srcpixel[ -1] << 16) & 0x00ff0000); /* h3 */
                if( --x) {
                    dstpixel[ -3] = ( srcpixel[ -3] >> 24) |              /* l2 */
                                    ((srcpixel[ -2] <<  8) & 0x00ffff00); /* h2, g2 */
                }
            }
        }
        for (x=0; x < w2; x++) {
            /* Do 4 pixels at a time: 3 dwords in and 4 dwords out */
            DWORD srcval1,srcval2;
            srcval1=srcpixel[0];
            dstpixel[0]=( srcval1        & 0x00ffffff); /* h1, g1, l1 */
            srcval2=srcpixel[1];
            dstpixel[1]=( srcval1 >> 24) |              /* l2 */
                        ((srcval2 <<  8) & 0x00ffff00); /* h2, g2 */
            srcval1=srcpixel[2];
            dstpixel[2]=( srcval2 >> 16) |              /* g3, l3 */
                        ((srcval1 << 16) & 0x00ff0000); /* h3 */
            dstpixel[3]=( srcval1 >>  8);               /* h4, g4, l4 */
            srcpixel+=3;
            dstpixel+=4;
        }
        /* do last w3 pixels */
        x = w3;
        if( x) {
            dstpixel[0]=( srcpixel[0]       & 0x00ffffff); /* h1, g1, l1 */
            if( --x) {
                dstpixel[1]=( srcpixel[0]>> 24) |              /* l2 */
                            ((srcpixel[1]<<  8) & 0x00ffff00); /* h2, g2 */
                if( --x) {
                    dstpixel[2]=( srcpixel[1]>> 16) |              /* g3, l3 */
                                ((srcpixel[2]<< 16) & 0x00ff0000); /* h3 */
                }
            }
        }
        srcbits = (const char*)srcbits + srclinebytes;
        dstbits = (char*)dstbits + dstlinebytes;
    }
}

static void convert_888_to_0888_reverse(int width, int height,
                                        const void* srcbits, int srclinebytes,
                                        void* dstbits, int dstlinebytes)
{
    const DWORD* srcpixel;
    DWORD* dstpixel;
    int x,y;
    int w1, w2, w3;

    w1 = min( (INT_PTR)srcbits & 3, width);
    w2 = ( width - w1) / 4;
    w3 = ( width - w1) & 3;
    for (y=0; y<height; y++) {
        srcpixel=srcbits;
        dstpixel=dstbits;
        /* advance w1 pixels to make srcpixel 32 bit aligned */
        srcpixel =  (const DWORD*)((INT_PTR)srcpixel & ~3);
        srcpixel += w1;
        dstpixel += w1;
        /* and do the w1 pixels */
        x = w1;
        if( x) {
            dstpixel[ -1]=((srcpixel[ -1] >> 24) & 0x0000ff) | /* h4 */
                          ((srcpixel[ -1] >>  8) & 0x00ff00) | /* g4 */
                          ((srcpixel[ -1] <<  8) & 0xff0000);  /* l4 */
            if( --x) {
                dstpixel[ -2]=( srcpixel[ -2]        & 0xff0000) | /* l3 */
                              ((srcpixel[ -2] >> 16) & 0x00ff00) | /* g3 */
                              ( srcpixel[ -1]        & 0x0000ff);  /* h3 */
                if( --x) {
                    dstpixel[ -3]=((srcpixel[ -3] >>  8) & 0xff0000) | /* l2 */
                                  ((srcpixel[ -2] <<  8) & 0x00ff00) | /* g2 */
                                  ((srcpixel[ -2] >>  8) & 0x0000ff);  /* h2 */
                }
            }
        }
        for (x=0; x < w2; x++) {
            /* Do 4 pixels at a time: 3 dwords in and 4 dwords out */
            DWORD srcval1,srcval2;

            srcval1=srcpixel[0];
            dstpixel[0]=((srcval1 >> 16) & 0x0000ff) | /* h1 */
                        ( srcval1        & 0x00ff00) | /* g1 */
                        ((srcval1 << 16) & 0xff0000);  /* l1 */
            srcval2=srcpixel[1];
            dstpixel[1]=((srcval1 >>  8) & 0xff0000) | /* l2 */
                        ((srcval2 <<  8) & 0x00ff00) | /* g2 */
                        ((srcval2 >>  8) & 0x0000ff);  /* h2 */
            srcval1=srcpixel[2];
            dstpixel[2]=( srcval2        & 0xff0000) | /* l3 */
                        ((srcval2 >> 16) & 0x00ff00) | /* g3 */
                        ( srcval1        & 0x0000ff);  /* h3 */
            dstpixel[3]=((srcval1 >> 24) & 0x0000ff) | /* h4 */
                        ((srcval1 >>  8) & 0x00ff00) | /* g4 */
                        ((srcval1 <<  8) & 0xff0000);  /* l4 */
            srcpixel+=3;
            dstpixel+=4;
        }
        /* do last w3 pixels */
        x = w3;
        if( x) {
            dstpixel[0]=((srcpixel[0] >> 16) & 0x0000ff) | /* h1 */
                        ( srcpixel[0]        & 0x00ff00) | /* g1 */
                        ((srcpixel[0] << 16) & 0xff0000);  /* l1 */
            if( --x) {
                dstpixel[1]=((srcpixel[0] >>  8) & 0xff0000) | /* l2 */
                            ((srcpixel[1] <<  8) & 0x00ff00) | /* g2 */
                            ((srcpixel[1] >>  8) & 0x0000ff);  /* h2 */
                if( --x) {
                    dstpixel[2]=( srcpixel[1]        & 0xff0000) | /* l3 */
                                ((srcpixel[1] >> 16) & 0x00ff00) | /* g3 */
                                ( srcpixel[2]        & 0x0000ff);  /* h3 */
                }
            }
        }
        srcbits = (const char*)srcbits + srclinebytes;
        dstbits = (char*)dstbits + dstlinebytes;
    }
}

static void convert_rgb888_to_any0888(int width, int height,
                                      const void* srcbits, int srclinebytes,
                                      void* dstbits, int dstlinebytes,
                                      DWORD rdst, DWORD gdst, DWORD bdst)
{
    int rLeftShift,gLeftShift,bLeftShift;
    const BYTE* srcpixel;
    DWORD* dstpixel;
    int x,y;

    rLeftShift=X11DRV_DIB_MaskToShift(rdst);
    gLeftShift=X11DRV_DIB_MaskToShift(gdst);
    bLeftShift=X11DRV_DIB_MaskToShift(bdst);
    for (y=0; y<height; y++) {
        srcpixel=srcbits;
        dstpixel=dstbits;
        for (x=0; x<width; x++) {
            *dstpixel++=(srcpixel[0] << bLeftShift) | /* b */
                        (srcpixel[1] << gLeftShift) | /* g */
                        (srcpixel[2] << rLeftShift);  /* r */
            srcpixel+=3;
        }
        srcbits = (const char*)srcbits + srclinebytes;
        dstbits = (char*)dstbits + dstlinebytes;
    }
}

static void convert_bgr888_to_any0888(int width, int height,
                                      const void* srcbits, int srclinebytes,
                                      void* dstbits, int dstlinebytes,
                                      DWORD rdst, DWORD gdst, DWORD bdst)
{
    int rLeftShift,gLeftShift,bLeftShift;
    const BYTE* srcpixel;
    DWORD* dstpixel;
    int x,y;

    rLeftShift=X11DRV_DIB_MaskToShift(rdst);
    gLeftShift=X11DRV_DIB_MaskToShift(gdst);
    bLeftShift=X11DRV_DIB_MaskToShift(bdst);
    for (y=0; y<height; y++) {
        srcpixel=srcbits;
        dstpixel=dstbits;
        for (x=0; x<width; x++) {
            *dstpixel++=(srcpixel[0] << rLeftShift) | /* r */
                        (srcpixel[1] << gLeftShift) | /* g */
                        (srcpixel[2] << bLeftShift);  /* b */
            srcpixel+=3;
        }
        srcbits = (const char*)srcbits + srclinebytes;
        dstbits = (char*)dstbits + dstlinebytes;
    }
}

/*
 * 32 bit conversions
 */

static void convert_0888_asis(int width, int height,
                              const void* srcbits, int srclinebytes,
                              void* dstbits, int dstlinebytes)
{
    int y;

    width *= 4;

    if (srclinebytes == dstlinebytes && srclinebytes == width)
    {
        memcpy(dstbits, srcbits, height * width);
        return;
    }

    for (y=0; y<height; y++) {
        memcpy(dstbits, srcbits, width);
        srcbits = (const char*)srcbits + srclinebytes;
        dstbits = (char*)dstbits + dstlinebytes;
    }
}

static void convert_0888_reverse(int width, int height,
                                 const void* srcbits, int srclinebytes,
                                 void* dstbits, int dstlinebytes)
{
    const DWORD* srcpixel;
    DWORD* dstpixel;
    int x,y;

    for (y=0; y<height; y++) {
        srcpixel=srcbits;
        dstpixel=dstbits;
        for (x=0; x<width; x++) {
            DWORD srcval;
            srcval=*srcpixel++;
            *dstpixel++=((srcval << 16) & 0x00ff0000) | /* h */
                        ( srcval        & 0x0000ff00) | /* g */
                        ((srcval >> 16) & 0x000000ff);  /* l */
        }
        srcbits = (const char*)srcbits + srclinebytes;
        dstbits = (char*)dstbits + dstlinebytes;
    }
}

static void convert_0888_any(int width, int height,
                             const void* srcbits, int srclinebytes,
                             DWORD rsrc, DWORD gsrc, DWORD bsrc,
                             void* dstbits, int dstlinebytes,
                             DWORD rdst, DWORD gdst, DWORD bdst)
{
    int rRightShift,gRightShift,bRightShift;
    int rLeftShift,gLeftShift,bLeftShift;
    const DWORD* srcpixel;
    DWORD* dstpixel;
    int x,y;

    rRightShift=X11DRV_DIB_MaskToShift(rsrc);
    gRightShift=X11DRV_DIB_MaskToShift(gsrc);
    bRightShift=X11DRV_DIB_MaskToShift(bsrc);
    rLeftShift=X11DRV_DIB_MaskToShift(rdst);
    gLeftShift=X11DRV_DIB_MaskToShift(gdst);
    bLeftShift=X11DRV_DIB_MaskToShift(bdst);
    for (y=0; y<height; y++) {
        srcpixel=srcbits;
        dstpixel=dstbits;
        for (x=0; x<width; x++) {
            DWORD srcval;
            srcval=*srcpixel++;
            *dstpixel++=(((srcval >> rRightShift) & 0xff) << rLeftShift) |
                        (((srcval >> gRightShift) & 0xff) << gLeftShift) |
                        (((srcval >> bRightShift) & 0xff) << bLeftShift);
        }
        srcbits = (const char*)srcbits + srclinebytes;
        dstbits = (char*)dstbits + dstlinebytes;
    }
}

static void convert_0888_to_555_asis(int width, int height,
                                     const void* srcbits, int srclinebytes,
                                     void* dstbits, int dstlinebytes)
{
    const DWORD* srcpixel;
    WORD* dstpixel;
    int x,y;

    for (y=0; y<height; y++) {
        srcpixel=srcbits;
        dstpixel=dstbits;
        for (x=0; x<width; x++) {
            DWORD srcval;
            srcval=*srcpixel++;
            *dstpixel++=((srcval >> 9) & 0x7c00) | /* h */
                        ((srcval >> 6) & 0x03e0) | /* g */
                        ((srcval >> 3) & 0x001f);  /* l */
        }
        srcbits = (const char*)srcbits + srclinebytes;
        dstbits = (char*)dstbits + dstlinebytes;
    }
}

static void convert_0888_to_555_reverse(int width, int height,
                                        const void* srcbits, int srclinebytes,
                                        void* dstbits, int dstlinebytes)
{
    const DWORD* srcpixel;
    WORD* dstpixel;
    int x,y;

    for (y=0; y<height; y++) {
        srcpixel=srcbits;
        dstpixel=dstbits;
        for (x=0; x<width; x++) {
            DWORD srcval;
            srcval=*srcpixel++;
            *dstpixel++=((srcval >> 19) & 0x001f) | /* h */
                        ((srcval >>  6) & 0x03e0) | /* g */
                        ((srcval <<  7) & 0x7c00);  /* l */
        }
        srcbits = (const char*)srcbits + srclinebytes;
        dstbits = (char*)dstbits + dstlinebytes;
    }
}

static void convert_0888_to_565_asis(int width, int height,
                                     const void* srcbits, int srclinebytes,
                                     void* dstbits, int dstlinebytes)
{
    const DWORD* srcpixel;
    WORD* dstpixel;
    int x,y;

    for (y=0; y<height; y++) {
        srcpixel=srcbits;
        dstpixel=dstbits;
        for (x=0; x<width; x++) {
            DWORD srcval;
            srcval=*srcpixel++;
            *dstpixel++=((srcval >> 8) & 0xf800) | /* h */
                        ((srcval >> 5) & 0x07e0) | /* g */
                        ((srcval >> 3) & 0x001f);  /* l */
        }
        srcbits = (const char*)srcbits + srclinebytes;
        dstbits = (char*)dstbits + dstlinebytes;
    }
}

static void convert_0888_to_565_reverse(int width, int height,
                                        const void* srcbits, int srclinebytes,
                                        void* dstbits, int dstlinebytes)
{
    const DWORD* srcpixel;
    WORD* dstpixel;
    int x,y;

    for (y=0; y<height; y++) {
        srcpixel=srcbits;
        dstpixel=dstbits;
        for (x=0; x<width; x++) {
            DWORD srcval;
            srcval=*srcpixel++;
            *dstpixel++=((srcval >> 19) & 0x001f) | /* h */
                        ((srcval >>  5) & 0x07e0) | /* g */
                        ((srcval <<  8) & 0xf800);  /* l */
        }
        srcbits = (const char*)srcbits + srclinebytes;
        dstbits = (char*)dstbits + dstlinebytes;
    }
}

static void convert_any0888_to_5x5(int width, int height,
                                   const void* srcbits, int srclinebytes,
                                   DWORD rsrc, DWORD gsrc, DWORD bsrc,
                                   void* dstbits, int dstlinebytes,
                                   WORD rdst, WORD gdst, WORD bdst)
{
    int rRightShift,gRightShift,bRightShift;
    int rLeftShift,gLeftShift,bLeftShift;
    const DWORD* srcpixel;
    WORD* dstpixel;
    int x,y;

    /* Here is how we proceed. Assume we have rsrc=0x0000ff00 and our pixel
     * contains 0x11223344.
     * - first we shift 0x11223344 right by rRightShift to bring the most
     *   significant bits of the red components in the bottom 5 (or 6) bits
     *   -> 0x4488c
     * - then we remove non red bits by anding with the modified rdst (0x1f)
     *   -> 0x0c
     * - finally shift these bits left by rLeftShift so that they end up in
     *   the right place
     *   -> 0x3000
     */
    rRightShift=X11DRV_DIB_MaskToShift(rsrc)+3;
    gRightShift=X11DRV_DIB_MaskToShift(gsrc);
    gRightShift+=(gdst==0x07e0?2:3);
    bRightShift=X11DRV_DIB_MaskToShift(bsrc)+3;

    rLeftShift=X11DRV_DIB_MaskToShift(rdst);
    rdst=rdst >> rLeftShift;
    gLeftShift=X11DRV_DIB_MaskToShift(gdst);
    gdst=gdst >> gLeftShift;
    bLeftShift=X11DRV_DIB_MaskToShift(bdst);
    bdst=bdst >> bLeftShift;

    for (y=0; y<height; y++) {
        srcpixel=srcbits;
        dstpixel=dstbits;
        for (x=0; x<width; x++) {
            DWORD srcval;
            srcval=*srcpixel++;
            *dstpixel++=(((srcval >> rRightShift) & rdst) << rLeftShift) |
                        (((srcval >> gRightShift) & gdst) << gLeftShift) |
                        (((srcval >> bRightShift) & bdst) << bLeftShift);
        }
        srcbits = (const char*)srcbits + srclinebytes;
        dstbits = (char*)dstbits + dstlinebytes;
    }
}

static void convert_0888_to_888_asis(int width, int height,
                                     const void* srcbits, int srclinebytes,
                                     void* dstbits, int dstlinebytes)
{
    const DWORD* srcpixel;
    DWORD* dstpixel;
    BYTE* dstbyte;
    int x,y;
    int oddwidth;

    oddwidth=width & 3;
    width=width/4;
    for (y=0; y<height; y++) {
        srcpixel=srcbits;
        dstpixel=dstbits;
        for (x=0; x<width; x++) {
            /* Do 4 pixels at a time: 4 dwords in and 3 dwords out */
            DWORD srcval;
            srcval=((*srcpixel++)       & 0x00ffffff);  /* h1, g1, l1*/
            *dstpixel++=srcval | ((*srcpixel)   << 24); /* h2 */
            srcval=((*srcpixel++ >> 8 ) & 0x0000ffff);  /* g2, l2 */
            *dstpixel++=srcval | ((*srcpixel)   << 16); /* h3, g3 */
            srcval=((*srcpixel++ >> 16) & 0x000000ff);  /* l3 */
            *dstpixel++=srcval | ((*srcpixel++) << 8);  /* h4, g4, l4 */
        }
        /* And now up to 3 odd pixels */
        dstbyte=(BYTE*)dstpixel;
        for (x=0; x<oddwidth; x++) {
            DWORD srcval;
            srcval=*srcpixel++;
            *((WORD*)dstbyte) = srcval;                 /* h, g */
            dstbyte+=sizeof(WORD);
            *dstbyte++=srcval >> 16;                    /* l */
        }
        srcbits = (const char*)srcbits + srclinebytes;
        dstbits = (char*)dstbits + dstlinebytes;
    }
}

static void convert_0888_to_888_reverse(int width, int height,
                                        const void* srcbits, int srclinebytes,
                                        void* dstbits, int dstlinebytes)
{
    const DWORD* srcpixel;
    DWORD* dstpixel;
    BYTE* dstbyte;
    int x,y;
    int oddwidth;

    oddwidth=width & 3;
    width=width/4;
    for (y=0; y<height; y++) {
        srcpixel=srcbits;
        dstpixel=dstbits;
        for (x=0; x<width; x++) {
            /* Do 4 pixels at a time: 4 dwords in and 3 dwords out */
            DWORD srcval1,srcval2;
            srcval1=*srcpixel++;
            srcval2=    ((srcval1 >> 16) & 0x000000ff) | /* h1 */
                        ( srcval1        & 0x0000ff00) | /* g1 */
                        ((srcval1 << 16) & 0x00ff0000);  /* l1 */
            srcval1=*srcpixel++;
            *dstpixel++=srcval2 |
                        ((srcval1 <<  8) & 0xff000000);  /* h2 */
            srcval2=    ((srcval1 >>  8) & 0x000000ff) | /* g2 */
                        ((srcval1 <<  8) & 0x0000ff00);  /* l2 */
            srcval1=*srcpixel++;
            *dstpixel++=srcval2 |
                        ( srcval1        & 0x00ff0000) | /* h3 */
                        ((srcval1 << 16) & 0xff000000);  /* g3 */
            srcval2=    ( srcval1        & 0x000000ff);  /* l3 */
            srcval1=*srcpixel++;
            *dstpixel++=srcval2 |
                        ((srcval1 >>  8) & 0x0000ff00) | /* h4 */
                        ((srcval1 <<  8) & 0x00ff0000) | /* g4 */
                        ( srcval1 << 24);                /* l4 */
        }
        /* And now up to 3 odd pixels */
        dstbyte=(BYTE*)dstpixel;
        for (x=0; x<oddwidth; x++) {
            DWORD srcval;
            srcval=*srcpixel++;
            *((WORD*)dstbyte)=((srcval >> 16) & 0x00ff) | /* h */
                              (srcval         & 0xff00);  /* g */
            dstbyte += sizeof(WORD);
            *dstbyte++=srcval;                              /* l */
        }
        srcbits = (const char*)srcbits + srclinebytes;
        dstbits = (char*)dstbits + dstlinebytes;
    }
}

static void convert_any0888_to_rgb888(int width, int height,
                                      const void* srcbits, int srclinebytes,
                                      DWORD rsrc, DWORD gsrc, DWORD bsrc,
                                      void* dstbits, int dstlinebytes)
{
    int rRightShift,gRightShift,bRightShift;
    const DWORD* srcpixel;
    BYTE* dstpixel;
    int x,y;

    rRightShift=X11DRV_DIB_MaskToShift(rsrc);
    gRightShift=X11DRV_DIB_MaskToShift(gsrc);
    bRightShift=X11DRV_DIB_MaskToShift(bsrc);
    for (y=0; y<height; y++) {
        srcpixel=srcbits;
        dstpixel=dstbits;
        for (x=0; x<width; x++) {
            DWORD srcval;
            srcval=*srcpixel++;
            dstpixel[0]=(srcval >> bRightShift); /* b */
            dstpixel[1]=(srcval >> gRightShift); /* g */
            dstpixel[2]=(srcval >> rRightShift); /* r */
            dstpixel+=3;
        }
        srcbits = (const char*)srcbits + srclinebytes;
        dstbits = (char*)dstbits + dstlinebytes;
    }
}

static void convert_any0888_to_bgr888(int width, int height,
                                      const void* srcbits, int srclinebytes,
                                      DWORD rsrc, DWORD gsrc, DWORD bsrc,
                                      void* dstbits, int dstlinebytes)
{
    int rRightShift,gRightShift,bRightShift;
    const DWORD* srcpixel;
    BYTE* dstpixel;
    int x,y;

    rRightShift=X11DRV_DIB_MaskToShift(rsrc);
    gRightShift=X11DRV_DIB_MaskToShift(gsrc);
    bRightShift=X11DRV_DIB_MaskToShift(bsrc);
    for (y=0; y<height; y++) {
        srcpixel=srcbits;
        dstpixel=dstbits;
        for (x=0; x<width; x++) {
            DWORD srcval;
            srcval=*srcpixel++;
            dstpixel[0]=(srcval >> rRightShift); /* r */
            dstpixel[1]=(srcval >> gRightShift); /* g */
            dstpixel[2]=(srcval >> bRightShift); /* b */
            dstpixel+=3;
        }
        srcbits = (const char*)srcbits + srclinebytes;
        dstbits = (char*)dstbits + dstlinebytes;
    }
}

const dib_conversions dib_normal = {
    convert_5x5_asis,
    convert_555_reverse,
    convert_555_to_565_asis,
    convert_555_to_565_reverse,
    convert_555_to_888_asis,
    convert_555_to_888_reverse,
    convert_555_to_0888_asis,
    convert_555_to_0888_reverse,
    convert_5x5_to_any0888,
    convert_565_reverse,
    convert_565_to_555_asis,
    convert_565_to_555_reverse,
    convert_565_to_888_asis,
    convert_565_to_888_reverse,
    convert_565_to_0888_asis,
    convert_565_to_0888_reverse,
    convert_888_asis,
    convert_888_reverse,
    convert_888_to_555_asis,
    convert_888_to_555_reverse,
    convert_888_to_565_asis,
    convert_888_to_565_reverse,
    convert_888_to_0888_asis,
    convert_888_to_0888_reverse,
    convert_rgb888_to_any0888,
    convert_bgr888_to_any0888,
    convert_0888_asis,
    convert_0888_reverse,
    convert_0888_any,
    convert_0888_to_555_asis,
    convert_0888_to_555_reverse,
    convert_0888_to_565_asis,
    convert_0888_to_565_reverse,
    convert_any0888_to_5x5,
    convert_0888_to_888_asis,
    convert_0888_to_888_reverse,
    convert_any0888_to_rgb888,
    convert_any0888_to_bgr888
};
