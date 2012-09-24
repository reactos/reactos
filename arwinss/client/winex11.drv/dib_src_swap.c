/*
 * DIB conversion routinues for cases where the source
 * has non-native byte order.
 *
 * Copyright (C) 2003 Huw Davies
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


#define FLIP_WORD(x) \
 ( *(x)  = ( (*(x) & 0xff) << 8) | \
   ( (*(x) & 0xff00) >> 8) )

#define FLIP_TWO_WORDS(x) \
 ( *(x)  = ( (*(x) & 0x00ff00ff) << 8) | \
   ( (*(x) & 0xff00ff00) >> 8) )

#define FLIP_DWORD(x) \
 ( *(x)  = ( (*(x) & 0xff) << 24) | \
   ( (*(x) & 0xff00) << 8) | \
   ( (*(x) & 0xff0000) >> 8) | \
   ( (*(x) & 0xff000000) >> 24) )



/*
 * 15 bit conversions
 */

static void convert_5x5_asis_src_byteswap(int width, int height,
                                          const void* srcbits, int srclinebytes,
                                          void* dstbits, int dstlinebytes)
{
    int x, y;
    const DWORD *srcpixel;
    DWORD *dstpixel;

    for (y=0; y<height; y++) {
        srcpixel=srcbits;
        dstpixel=dstbits;
        for(x = 0; x < width/2; x++) {
            /* Do 2 pixels at a time */
            DWORD srcval = *srcpixel++;
            *dstpixel++=((srcval << 8) & 0xff00ff00) |
                        ((srcval >> 8) & 0x00ff00ff);
        }
        if(width&1) {
            /* And the odd pixel */
            WORD srcval = *(const WORD*)srcpixel;
            *(WORD*)dstpixel = ((srcval << 8) & 0xff00) |
                               ((srcval >> 8) & 0x00ff);
        }
        srcbits = (const char*)srcbits + srclinebytes;
        dstbits = (char*)dstbits + dstlinebytes;
    }
}

static void convert_555_reverse_src_byteswap(int width, int height,
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
            *dstpixel++=((srcval >>  2) & 0x001f001f) | /* h */
                        ((srcval <<  8) & 0x03000300) | /* g - 2 bits */
                        ((srcval >>  8) & 0x00e000e0) | /* g - 3 bits */
                        ((srcval <<  2) & 0x7c007c00);  /* l */
        }
        if (width&1) {
            /* And then the odd pixel */
            WORD srcval;
            srcval=*((const WORD*)srcpixel);
            *((WORD*)dstpixel)=((srcval >>  2) & 0x001f) | /* h */
                               ((srcval <<  8) & 0x0300) | /* g - 2 bits */
                               ((srcval >>  8) & 0x00e0) | /* g - 3 bits */
                               ((srcval <<  2) & 0x7c00);  /* l */
        }
        srcbits = (const char*)srcbits + srclinebytes;
        dstbits = (char*)dstbits + dstlinebytes;
    }
}

static void convert_555_to_565_asis_src_byteswap(int width, int height,
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
            *dstpixel++=((srcval << 9) & 0xfe00fe00) | /* h, g - 2 bits*/
                        ((srcval >> 7) & 0x01c001c0) | /* g - 3 bits */
                        ((srcval << 4) & 0x00200020) | /* g - 1 bit */
                        ((srcval >> 8) & 0x001f001f);  /* l */
        }
        if (width&1) {
            /* And then the odd pixel */
            WORD srcval;
            srcval=*((const WORD*)srcpixel);
            *((WORD*)dstpixel)=((srcval << 9) & 0xfe00) | /* h, g - 2bits*/
                               ((srcval >> 7) & 0x01c0) | /* g - 3 bits */
                               ((srcval << 4) & 0x0020) | /* g - 1 bit */
                               ((srcval >> 8) & 0x001f);  /* l */
        }
        srcbits = (const char*)srcbits + srclinebytes;
        dstbits = (char*)dstbits + dstlinebytes;
    }
}

static void convert_555_to_565_reverse_src_byteswap(int width, int height,
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
             *dstpixel++=((srcval >>  2) & 0x001f001f) | /* h */
                         ((srcval <<  9) & 0x06000600) | /* g - 2 bits*/
                         ((srcval >>  7) & 0x01c001c0) | /* g - 3 bits */
                         ((srcval <<  4) & 0x00200020) | /* g - 1 bits */
                         ((srcval <<  3) & 0xf800f800);  /* l */
        }
        if (width&1) {
            /* And then the odd pixel */
            WORD srcval;
            srcval=*((const WORD*)srcpixel);
            *((WORD*)dstpixel)=((srcval >>  2) & 0x001f) | /* h */
                               ((srcval <<  9) & 0x0600) | /* g - 2 bits  */
                               ((srcval >>  7) & 0x01c0) | /* g - 3 bits */
                               ((srcval <<  4) & 0x0020) | /* g - 1 bit */
                               ((srcval <<  3) & 0xf800);  /* l */
        }
        srcbits = (const char*)srcbits + srclinebytes;
        dstbits = (char*)dstbits + dstlinebytes;
    }
}

static void convert_555_to_888_asis_src_byteswap(int width, int height,
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
            dstpixel[0]=((srcval >>  5) & 0xf8) | /* l */
                        ((srcval >> 10) & 0x07);  /* l - 3 bits */
            dstpixel[1]=((srcval <<  6) & 0xc0) | /* g - 2 bits */
                        ((srcval >> 10) & 0x38) | /* g - 3 bits */
                        ((srcval <<  1) & 0x06) | /* g - 2 bits */
                        ((srcval >> 15) & 0x01);  /* g - 1 bit */
            dstpixel[2]=((srcval <<  1) & 0xf8) | /* h */
                        ((srcval >>  4) & 0x07);  /* h - 3 bits */
            dstpixel+=3;
        }
        srcbits = (const char*)srcbits + srclinebytes;
        dstbits = (char*)dstbits + dstlinebytes;
    }
}

static void convert_555_to_888_reverse_src_byteswap(int width, int height,
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
            dstpixel[0]=((srcval <<  1) & 0xf8) | /* h */
                        ((srcval >>  4) & 0x07);  /* h - 3 bits */
            dstpixel[1]=((srcval <<  6) & 0xc0) | /* g - 2 bits */
                        ((srcval >> 10) & 0x38) | /* g - 3 bits */
                        ((srcval <<  1) & 0x06) | /* g - 2 bits */
                        ((srcval >> 15) & 0x01);  /* g - 1 bits */
            dstpixel[2]=((srcval >>  5) & 0xf8) | /* l */
                        ((srcval >> 10) & 0x07);  /* l - 3 bits */
            dstpixel+=3;
        }
        srcbits = (const char*)srcbits + srclinebytes;
        dstbits = (char*)dstbits + dstlinebytes;
    }
}

static void convert_555_to_0888_asis_src_byteswap(int width, int height,
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
            *dstpixel++=((srcval << 17) & 0xf80000) | /* h */
                        ((srcval << 12) & 0x070000) | /* h - 3 bits */
                        ((srcval << 14) & 0x00c000) | /* g - 2 bits */
                        ((srcval >>  2) & 0x003800) | /* g - 3 bits */
                        ((srcval <<  9) & 0x000600) | /* g - 2 bits */
                        ((srcval >>  7) & 0x000100) | /* g - 1 bit */
                        ((srcval >>  5) & 0x0000f8) | /* l */
                        ((srcval >> 10) & 0x000007);  /* l - 3 bits */
        }
        srcbits = (const char*)srcbits + srclinebytes;
        dstbits = (char*)dstbits + dstlinebytes;
    }
}

static void convert_555_to_0888_reverse_src_byteswap(int width, int height,
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
            *dstpixel++=((srcval <<  1) & 0x0000f8) | /* h */
                        ((srcval >>  4) & 0x000007) | /* h - 3 bits */
                        ((srcval << 14) & 0x00c000) | /* g - 2 bits */
                        ((srcval >>  2) & 0x003800) | /* g - 3 bits */
                        ((srcval <<  9) & 0x000600) | /* g - 2 bits */
                        ((srcval >>  7) & 0x000100) | /* g - 1 bit */
                        ((srcval << 11) & 0xf80000) | /* l */
                        ((srcval <<  6) & 0x070000);  /* l - 3 bits */
        }
        srcbits = (const char*)srcbits + srclinebytes;
        dstbits = (char*)dstbits + dstlinebytes;
    }
}

static void convert_5x5_to_any0888_src_byteswap(int width, int height,
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
            FLIP_TWO_WORDS(&srcval);

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

static void convert_565_reverse_src_byteswap(int width, int height,
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
            *dstpixel++=((srcval <<  3) & 0xf800f800) | /* l */
                        ((srcval <<  8) & 0x07000700) | /* g - 3 bits */
                        ((srcval >>  8) & 0x00e000e0) | /* g - 3 bits */
                        ((srcval >>  3) & 0x001f001f);  /* h */
        }
        if (width&1) {
            /* And then the odd pixel */
            WORD srcval;
            srcval=*((const WORD*)srcpixel);
            *((WORD*)dstpixel)=((srcval <<  3) & 0xf800) | /* l */
                               ((srcval <<  8) & 0x0700) | /* g - 3 bits */
                               ((srcval >>  8) & 0x00e0) | /* g - 3 bits */
                               ((srcval >>  3) & 0x001f);  /* h */
        }
        srcbits = (const char*)srcbits + srclinebytes;
        dstbits = (char*)dstbits + dstlinebytes;
    }
}

static void convert_565_to_555_asis_src_byteswap(int width, int height,
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
            *dstpixel++=((srcval << 7) & 0x7f807f80) | /* h, g - 3 bits */
                        ((srcval >> 9) & 0x00600060) | /* g - 2 bits */
                        ((srcval >> 8) & 0x001f001f);  /* l */
        }
        if (width&1) {
            /* And then the odd pixel */
            WORD srcval;
            srcval=*((const WORD*)srcpixel);
            *((WORD*)dstpixel)=((srcval << 7) & 0x7f80) | /* h, g - 3 bits */
                               ((srcval >> 9) & 0x0060) | /* g - 2 bits */
                               ((srcval >> 8) & 0x001f);  /* l */
        }
        srcbits = (const char*)srcbits + srclinebytes;
        dstbits = (char*)dstbits + dstlinebytes;
    }
}

static void convert_565_to_555_reverse_src_byteswap(int width, int height,
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
            *dstpixel++=((srcval >>  3) & 0x001f001f) | /* h */
                        ((srcval >>  9) & 0x00600060) | /* g - 2 bits */
                        ((srcval <<  7) & 0x03800380) | /* g - 3 bits */
                        ((srcval <<  2) & 0x7c007c00);  /* l */
        }
        if (width&1) {
            /* And then the odd pixel */
            WORD srcval;
            srcval=*((const WORD*)srcpixel);
            *((WORD*)dstpixel)=((srcval >>  3) & 0x001f) | /* h */
                               ((srcval >>  9) & 0x0060) | /* g - 2 bits */
                               ((srcval <<  7) & 0x0380) | /* g - 3 bits */
                               ((srcval <<  2) & 0x7c00);  /* l */
        }
        srcbits = (const char*)srcbits + srclinebytes;
        dstbits = (char*)dstbits + dstlinebytes;
    }
}

static void convert_565_to_888_asis_src_byteswap(int width, int height,
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
            dstpixel[0]=((srcval >>  5) & 0xf8) | /* l */
                        ((srcval >> 10) & 0x07);  /* l - 3 bits */
            dstpixel[1]=((srcval <<  5) & 0xe0) | /* g - 3 bits */
                        ((srcval >> 11) & 0x1c) | /* g - 3 bits */
                        ((srcval >>  1) & 0x03);  /* g - 2 bits */
            dstpixel[2]=((srcval >>  0) & 0xf8) | /* h */
                        ((srcval >>  5) & 0x07);  /* h - 3 bits */
            dstpixel+=3;
        }
        srcbits = (const char*)srcbits + srclinebytes;
        dstbits = (char*)dstbits + dstlinebytes;
    }
}

static void convert_565_to_888_reverse_src_byteswap(int width, int height,
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
            dstpixel[0]=((srcval >>  0) & 0xf8) | /* h */
                        ((srcval >>  5) & 0x07);  /* h - 3 bits */
            dstpixel[1]=((srcval <<  5) & 0xe0) | /* g - 3 bits */
                        ((srcval >> 11) & 0x1c) | /* g - 3 bits */
                        ((srcval >>  1) & 0x03);  /* g - 2 bits */
            dstpixel[2]=((srcval >>  5) & 0xf8) | /* l */
                        ((srcval >> 10) & 0x07);  /* l - 3 bits */
            dstpixel+=3;
        }
        srcbits = (const char*)srcbits + srclinebytes;
        dstbits = (char*)dstbits + dstlinebytes;
    }
}

static void convert_565_to_0888_asis_src_byteswap(int width, int height,
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
            *dstpixel++=((srcval << 16) & 0xf80000) | /* h */
                        ((srcval << 11) & 0x070000) | /* h - 3 bits */
                        ((srcval << 13) & 0x00e000) | /* g - 3 bits */
                        ((srcval >>  3) & 0x001c00) | /* g - 3 bits */
                        ((srcval <<  7) & 0x000300) | /* g - 2 bits */
                        ((srcval >>  5) & 0x0000f8) | /* l */
                        ((srcval >> 10) & 0x000007);  /* l - 3 bits */
        }
        srcbits = (const char*)srcbits + srclinebytes;
        dstbits = (char*)dstbits + dstlinebytes;
    }
}

static void convert_565_to_0888_reverse_src_byteswap(int width, int height,
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
            *dstpixel++=((srcval >>  0) & 0x0000f8) | /* h */
                        ((srcval >>  5) & 0x000007) | /* h - 3 bits */
                        ((srcval << 13) & 0x00e000) | /* g - 3 bits */
                        ((srcval >>  3) & 0x001c00) | /* g - 3 bits */
                        ((srcval <<  7) & 0x000300) | /* g - 2 bits */
                        ((srcval << 11) & 0xf80000) | /* l */
                        ((srcval <<  6) & 0x070000);  /* l - 3 bits */
        }
        srcbits = (const char*)srcbits + srclinebytes;
        dstbits = (char*)dstbits + dstlinebytes;
    }
}

/*
 * 24 bit conversions
 */

static void convert_888_asis_src_byteswap(int width, int height,
                                          const void* srcbits, int srclinebytes,
                                          void* dstbits, int dstlinebytes)
{
    int x, y;

    for (y=0; y<height; y++) {
        for(x = 0; x < ((width+1)*3/4); x++) {
            DWORD srcval = *((const DWORD*)srcbits + x);
            *((DWORD*)dstbits + x) = ((srcval << 24) & 0xff000000) |
                                     ((srcval <<  8) & 0x00ff0000) |
                                     ((srcval >>  8) & 0x0000ff00) |
                                     ((srcval >> 24) & 0x000000ff);
        }
        srcbits = (const char*)srcbits + srclinebytes;
        dstbits = (char*)dstbits + dstlinebytes;
    }
}

static void convert_888_reverse_src_byteswap(int width, int height,
                                             const void* srcbits, int srclinebytes,
                                             void* dstbits, int dstlinebytes)
{
    const DWORD* srcpixel;
    DWORD* dstpixel;
    DWORD srcarray[3];
    int x,y;
    int oddwidth = width & 3;

    width = width/4;

    for (y=0; y<height; y++) {
        srcpixel=srcbits;
        dstpixel=dstbits;
        for (x=0; x<width; x++) {
            /* Do 4 pixels at a time: 3 dwords in and 3 dwords out */
            *dstpixel++= ((srcpixel[0] >>  8) & 0x00ffffff) | /* l1, g1, h1 */
                         ((srcpixel[1] <<  8) & 0xff000000);  /* h2 */
            *dstpixel++= ((srcpixel[1] >> 24) & 0x000000ff) | /* g2 */
                         ((srcpixel[0] <<  8) & 0x0000ff00) | /* l2 */
                         ((srcpixel[2] >>  8) & 0x00ff0000) | /* h3 */
                         ((srcpixel[1] << 24) & 0xff000000);  /* g3 */
            *dstpixel++= ((srcpixel[1] >>  8) & 0x000000ff) | /* l3 */
                         ((srcpixel[2] <<  8) & 0xffffff00);  /* l4, g4, h4 */
            srcpixel+=3;
        }
        /* And now up to 3 odd pixels */
        if(oddwidth) {
            BYTE *dstbyte, *srcbyte;
            memcpy(srcarray,srcpixel,oddwidth*sizeof(DWORD));
            dstbyte = (LPBYTE)dstpixel;
            srcbyte = (LPBYTE)srcarray;
            for (x=0; x<oddwidth; x++) {
                FLIP_DWORD(srcarray+x);
                dstbyte[0] = srcbyte[2];
                dstbyte[1] = srcbyte[1];
                dstbyte[2] = srcbyte[0];
                srcbyte+=3;
                dstbyte+=3;
            }
        }

        srcbits = (const char*)srcbits + srclinebytes;
        dstbits = (char*)dstbits + dstlinebytes;
    }
}

static void convert_888_to_555_asis_src_byteswap(int width, int height,
                                                 const void* srcbits, int srclinebytes,
                                                 void* dstbits, int dstlinebytes)
{
    const DWORD* srcpixel;
    const BYTE* srcbyte;
    WORD* dstpixel;
    DWORD srcarray[3];
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
            FLIP_DWORD(&srcval1);
            dstpixel[0]=((srcval1 >>  3) & 0x001f) | /* l1 */
                        ((srcval1 >>  6) & 0x03e0) | /* g1 */
                        ((srcval1 >>  9) & 0x7c00);  /* h1 */
            srcval2=srcpixel[1];
            FLIP_DWORD(&srcval2);
            dstpixel[1]=((srcval1 >> 27) & 0x001f) | /* l2 */
                        ((srcval2 <<  2) & 0x03e0) | /* g2 */
                        ((srcval2 >>  1) & 0x7c00);  /* h2 */
            srcval1=srcpixel[2];
            FLIP_DWORD(&srcval1);
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
        if(oddwidth) {
            memcpy(srcarray,srcpixel,oddwidth*sizeof(DWORD));
            srcbyte = (LPBYTE)srcarray;
            for (x=0; x<oddwidth; x++) {
                WORD dstval;
                FLIP_DWORD(srcarray+x);

                dstval =((srcbyte[0] >> 3) & 0x001f);    /* l */
                dstval|=((srcbyte[1] << 2) & 0x03e0);    /* g */
                dstval|=((srcbyte[2] << 7) & 0x7c00);    /* h */
                *dstpixel++=dstval;
                srcbyte+=3;
            }
        }
        srcbits = (const char*)srcbits + srclinebytes;
        dstbits = (char*)dstbits + dstlinebytes;
    }
}

static void convert_888_to_555_reverse_src_byteswap(int width, int height,
                                                    const void* srcbits, int srclinebytes,
                                                    void* dstbits, int dstlinebytes)
{
    const DWORD* srcpixel;
    const BYTE* srcbyte;
    WORD* dstpixel;
    DWORD srcarray[3];
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
            FLIP_DWORD(&srcval1);
            dstpixel[0]=((srcval1 <<  7) & 0x7c00) | /* l1 */
                        ((srcval1 >>  6) & 0x03e0) | /* g1 */
                        ((srcval1 >> 19) & 0x001f);  /* h1 */
            srcval2=srcpixel[1];
            FLIP_DWORD(&srcval2);
            dstpixel[1]=((srcval1 >> 17) & 0x7c00) | /* l2 */
                        ((srcval2 <<  2) & 0x03e0) | /* g2 */
                        ((srcval2 >> 11) & 0x001f);  /* h2 */
            srcval1=srcpixel[2];
            FLIP_DWORD(&srcval1);
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
        if(oddwidth) {
            memcpy(srcarray,srcpixel,oddwidth*sizeof(DWORD));
            srcbyte = (LPBYTE)srcarray;
            for (x=0; x<oddwidth; x++) {
                WORD dstval;
                FLIP_DWORD(srcarray+x);
                dstval =((srcbyte[0] << 7) & 0x7c00);    /* l */
                dstval|=((srcbyte[1] << 2) & 0x03e0);    /* g */
                dstval|=((srcbyte[2] >> 3) & 0x001f);    /* h */
                *dstpixel++=dstval;
                srcbyte+=3;
            }
        }
        srcbits = (const char*)srcbits + srclinebytes;
        dstbits = (char*)dstbits + dstlinebytes;
    }
}

static void convert_888_to_565_asis_src_byteswap(int width, int height,
                                                 const void* srcbits, int srclinebytes,
                                                 void* dstbits, int dstlinebytes)
{
    const DWORD* srcpixel;
    const BYTE* srcbyte;
    WORD* dstpixel;
    DWORD srcarray[3];
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
            FLIP_DWORD(&srcval1);
            dstpixel[0]=((srcval1 >>  3) & 0x001f) | /* l1 */
                        ((srcval1 >>  5) & 0x07e0) | /* g1 */
                        ((srcval1 >>  8) & 0xf800);  /* h1 */
            srcval2=srcpixel[1];
            FLIP_DWORD(&srcval2);
            dstpixel[1]=((srcval1 >> 27) & 0x001f) | /* l2 */
                        ((srcval2 <<  3) & 0x07e0) | /* g2 */
                        ( srcval2        & 0xf800);  /* h2 */
            srcval1=srcpixel[2];
            FLIP_DWORD(&srcval1);
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
        if(oddwidth) {
            memcpy(srcarray,srcpixel,oddwidth*sizeof(DWORD));
            srcbyte = (LPBYTE)srcarray;
            for (x=0; x<oddwidth; x++) {
                WORD dstval;
                FLIP_DWORD(srcarray+x);
                dstval =((srcbyte[0] >> 3) & 0x001f);    /* l */
                dstval|=((srcbyte[1] << 3) & 0x07e0);    /* g */
                dstval|=((srcbyte[2] << 8) & 0xf800);    /* h */
                *dstpixel++=dstval;
                srcbyte+=3;
            }
        }
        srcbits = (const char*)srcbits + srclinebytes;
        dstbits = (char*)dstbits + dstlinebytes;
    }
}

static void convert_888_to_565_reverse_src_byteswap(int width, int height,
                                                    const void* srcbits, int srclinebytes,
                                                    void* dstbits, int dstlinebytes)
{
    const DWORD* srcpixel;
    const BYTE* srcbyte;
    WORD* dstpixel;
    DWORD srcarray[3];
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
            FLIP_DWORD(&srcval1);
            dstpixel[0]=((srcval1 <<  8) & 0xf800) | /* l1 */
                        ((srcval1 >>  5) & 0x07e0) | /* g1 */
                        ((srcval1 >> 19) & 0x001f);  /* h1 */
            srcval2=srcpixel[1];
            FLIP_DWORD(&srcval2);
            dstpixel[1]=((srcval1 >> 16) & 0xf800) | /* l2 */
                        ((srcval2 <<  3) & 0x07e0) | /* g2 */
                        ((srcval2 >> 11) & 0x001f);  /* h2 */
            srcval1=srcpixel[2];
            FLIP_DWORD(&srcval1);
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
        if(oddwidth) {
            memcpy(srcarray,srcpixel,oddwidth*sizeof(DWORD));
            srcbyte = (LPBYTE)srcarray;
            for (x=0; x<oddwidth; x++) {
                WORD dstval;
                FLIP_DWORD(srcarray+x);
                dstval =((srcbyte[0] << 8) & 0xf800);    /* l */
                dstval|=((srcbyte[1] << 3) & 0x07e0);    /* g */
                dstval|=((srcbyte[2] >> 3) & 0x001f);    /* h */
                *dstpixel++=dstval;
                srcbyte+=3;
            }
        }
        srcbits = (const char*)srcbits + srclinebytes;
        dstbits = (char*)dstbits + dstlinebytes;
    }
}

static void convert_888_to_0888_asis_src_byteswap(int width, int height,
                                                  const void* srcbits, int srclinebytes,
                                                  void* dstbits, int dstlinebytes)
{
    const DWORD* srcpixel;
    DWORD* dstpixel;
    DWORD srcarray[3];
    int x,y;
    int oddwidth;

    oddwidth=width & 3;
    width=width/4;
    for (y=0; y<height; y++) {
        srcpixel=srcbits;
        dstpixel=dstbits;
        for (x=0; x<width; x++) {
            /* Do 4 pixels at a time: 3 dwords in and 4 dwords out */
            DWORD srcval1,srcval2;
            srcval1=srcpixel[0];
            FLIP_DWORD(&srcval1);
            dstpixel[0]=( srcval1        & 0x00ffffff);  /* h1, g1, l1 */
            srcval2=srcpixel[1];
            FLIP_DWORD(&srcval2);
            dstpixel[1]=( srcval1 >> 24) |              /* l2 */
                        ((srcval2 <<  8) & 0x00ffff00); /* h2, g2 */
            srcval1=srcpixel[2];
            FLIP_DWORD(&srcval1);
            dstpixel[2]=( srcval2 >> 16) |              /* g3, l3 */
                        ((srcval1 << 16) & 0x00ff0000); /* h3 */
            dstpixel[3]=( srcval1 >>  8);               /* h4, g4, l4 */
            srcpixel+=3;
            dstpixel+=4;
        }
        /* And now up to 3 odd pixels */
        if(oddwidth) {
            memcpy(srcarray,srcpixel,oddwidth*sizeof(DWORD));
            srcpixel = srcarray;
            for (x=0; x<oddwidth; x++) {
                DWORD srcval;
                FLIP_DWORD(srcarray+x);
                srcval=*srcpixel;
                srcpixel=(const DWORD*)(((const char*)srcpixel)+3);
                *dstpixel++=( srcval         & 0x00ffffff); /* h, g, l */
            }
        }
        srcbits = (const char*)srcbits + srclinebytes;
        dstbits = (char*)dstbits + dstlinebytes;
    }
}

static void convert_888_to_0888_reverse_src_byteswap(int width, int height,
                                                     const void* srcbits, int srclinebytes,
                                                     void* dstbits, int dstlinebytes)
{
    const DWORD* srcpixel;
    DWORD* dstpixel;
    DWORD srcarray[3];
    int x,y;
    int oddwidth;

    oddwidth=width & 3;
    width=width/4;
    for (y=0; y<height; y++) {
        srcpixel=srcbits;
        dstpixel=dstbits;
        for (x=0; x<width; x++) {
            /* Do 4 pixels at a time: 3 dwords in and 4 dwords out */
            DWORD srcval1,srcval2;

            srcval1=srcpixel[0];
            FLIP_DWORD(&srcval1);
            dstpixel[0]=((srcval1 >> 16) & 0x0000ff) | /* h1 */
                        ( srcval1        & 0x00ff00) | /* g1 */
                        ((srcval1 << 16) & 0xff0000);  /* l1 */
            srcval2=srcpixel[1];
            FLIP_DWORD(&srcval2);
            dstpixel[1]=((srcval1 >>  8) & 0xff0000) | /* l2 */
                        ((srcval2 <<  8) & 0x00ff00) | /* g2 */
                        ((srcval2 >>  8) & 0x0000ff);  /* h2 */
            srcval1=srcpixel[2];
            FLIP_DWORD(&srcval1);
            dstpixel[2]=( srcval2        & 0xff0000) | /* l3 */
                        ((srcval2 >> 16) & 0x00ff00) | /* g3 */
                        ( srcval1        & 0x0000ff);  /* h3 */
            dstpixel[3]=((srcval1 >> 24) & 0x0000ff) | /* h4 */
                        ((srcval1 >>  8) & 0x00ff00) | /* g4 */
                        ((srcval1 <<  8) & 0xff0000);  /* l4 */
            srcpixel+=3;
            dstpixel+=4;
        }
        /* And now up to 3 odd pixels */
        if(oddwidth) {
            memcpy(srcarray,srcpixel,oddwidth*sizeof(DWORD));
            srcpixel = srcarray;
            for (x=0; x<oddwidth; x++) {
                DWORD srcval;
                FLIP_DWORD(srcarray+x);
                srcval=*srcpixel;
                srcpixel=(const DWORD*)(((const char*)srcpixel)+3);
                *dstpixel++=((srcval  >> 16) & 0x0000ff) | /* h */
                            ( srcval         & 0x00ff00) | /* g */
                            ((srcval  << 16) & 0xff0000);  /* l */
            }
        }
        srcbits = (const char*)srcbits + srclinebytes;
        dstbits = (char*)dstbits + dstlinebytes;
    }
}

static void convert_rgb888_to_any0888_src_byteswap(int width, int height,
                                                   const void* srcbits, int srclinebytes,
                                                   void* dstbits, int dstlinebytes,
                                                   DWORD rdst, DWORD gdst, DWORD bdst)
{
    int rLeftShift,gLeftShift,bLeftShift;
    const DWORD* srcpixel;
    DWORD* dstpixel;
    int x,y;
    DWORD srcarray[3];

    rLeftShift=X11DRV_DIB_MaskToShift(rdst);
    gLeftShift=X11DRV_DIB_MaskToShift(gdst);
    bLeftShift=X11DRV_DIB_MaskToShift(bdst);
    for (y=0; y<height; y++) {
        srcpixel=srcbits;
        dstpixel=dstbits;
        for (x=0; x<width/4; x++) {
            /* Do 4 pixels at a time: 3 dwords in and 4 dwords out */
            DWORD srcval1, srcval2;
            srcval1=*srcpixel++;
            *dstpixel++=(((srcval1 >> 24) & 0xff) << bLeftShift) | /* b1 */
                        (((srcval1 >> 16) & 0xff) << gLeftShift) | /* g1 */
                        (((srcval1 >>  8) & 0xff) << rLeftShift);  /* r1 */
            srcval2=*srcpixel++;
            *dstpixel++=(((srcval1 >>  0) & 0xff) << bLeftShift) | /* b2 */
                        (((srcval2 >> 24) & 0xff) << gLeftShift) | /* g2 */
                        (((srcval2 >> 16) & 0xff) << rLeftShift);  /* r2 */
            srcval1=*srcpixel++;
            *dstpixel++=(((srcval2 >>  8) & 0xff) << bLeftShift) | /* b3 */
                        (((srcval2 >>  0) & 0xff) << gLeftShift) | /* g3 */
                        (((srcval1 >> 24) & 0xff) << rLeftShift);  /* r3 */
            *dstpixel++=(((srcval1 >> 16) & 0xff) << bLeftShift) | /* b4 */
                        (((srcval1 >>  8) & 0xff) << gLeftShift) | /* g4 */
                        (((srcval1 >>  0) & 0xff) << rLeftShift);  /* r4 */
        }
        /* And now up to 3 odd pixels */
        if(width&3) {
            memcpy(srcarray,srcpixel,width&3*sizeof(DWORD));
            srcpixel = srcarray;
            for (x=0; x < (width&3); x++) {
                DWORD srcval;
                FLIP_DWORD(srcarray+x);
                srcval=*srcpixel;
                srcpixel=(const DWORD*)(((const char*)srcpixel)+3);
                *dstpixel++=(((srcval >>  0) & 0xff) << bLeftShift) | /* b */
                            (((srcval >>  8) & 0xff) << gLeftShift) | /* g */
                            (((srcval >> 16) & 0xff) << rLeftShift);  /* r */
            }
        }
        srcbits = (const char*)srcbits + srclinebytes;
        dstbits = (char*)dstbits + dstlinebytes;
    }
}

static void convert_bgr888_to_any0888_src_byteswap(int width, int height,
                                                   const void* srcbits, int srclinebytes,
                                                   void* dstbits, int dstlinebytes,
                                                   DWORD rdst, DWORD gdst, DWORD bdst)
{
    int rLeftShift,gLeftShift,bLeftShift;
    const DWORD* srcpixel;
    DWORD* dstpixel;
    int x,y;
    DWORD srcarray[3];

    rLeftShift=X11DRV_DIB_MaskToShift(rdst);
    gLeftShift=X11DRV_DIB_MaskToShift(gdst);
    bLeftShift=X11DRV_DIB_MaskToShift(bdst);
    for (y=0; y<height; y++) {
        srcpixel=srcbits;
        dstpixel=dstbits;
        for (x=0; x<width/4; x++) {
            /* Do 4 pixels at a time: 3 dwords in and 4 dwords out */
            DWORD srcval1, srcval2;
            srcval1=*srcpixel++;
            *dstpixel++=(((srcval1 >> 24) & 0xff) << rLeftShift) | /* r1 */
                        (((srcval1 >> 16) & 0xff) << gLeftShift) | /* g1 */
                        (((srcval1 >>  8) & 0xff) << bLeftShift);  /* b1 */
            srcval2=*srcpixel++;
            *dstpixel++=(((srcval1 >>  0) & 0xff) << rLeftShift) | /* r2 */
                        (((srcval2 >> 24) & 0xff) << gLeftShift) | /* g2 */
                        (((srcval2 >> 16) & 0xff) << bLeftShift);  /* b2 */
            srcval1=*srcpixel++;
            *dstpixel++=(((srcval2 >>  8) & 0xff) << rLeftShift) | /* r3 */
                        (((srcval2 >>  0) & 0xff) << gLeftShift) | /* g3 */
                        (((srcval1 >> 24) & 0xff) << bLeftShift);  /* b3 */
            *dstpixel++=(((srcval1 >> 16) & 0xff) << rLeftShift) | /* r4 */
                        (((srcval1 >>  8) & 0xff) << gLeftShift) | /* g4 */
                        (((srcval1 >>  0) & 0xff) << bLeftShift);  /* b4 */
        }
        /* And now up to 3 odd pixels */
        if(width&3) {
            memcpy(srcarray,srcpixel,width&3*sizeof(DWORD));
            srcpixel = srcarray;
            for (x=0; x < (width&3); x++) {
                DWORD srcval;
                FLIP_DWORD(srcarray+x);
                srcval=*srcpixel;
                srcpixel=(const DWORD*)(((const char*)srcpixel)+3);
                *dstpixel++=(((srcval >>  0) & 0xff) << rLeftShift) | /* r */
                            (((srcval >>  8) & 0xff) << gLeftShift) | /* g */
                            (((srcval >> 16) & 0xff) << bLeftShift);  /* b */
            }
        }
        srcbits = (const char*)srcbits + srclinebytes;
        dstbits = (char*)dstbits + dstlinebytes;
    }
}


/*
 * 32 bit conversions
 */

static void convert_0888_asis_src_byteswap(int width, int height,
                                           const void* srcbits, int srclinebytes,
                                           void* dstbits, int dstlinebytes)
{
    int x, y;

    for (y=0; y<height; y++) {
        for(x = 0; x < width; x++) {
            DWORD srcval = *((const DWORD*)srcbits + x);
            *((DWORD*)dstbits + x) = ((srcval << 24) & 0xff000000) |
                                     ((srcval <<  8) & 0x00ff0000) |
                                     ((srcval >>  8) & 0x0000ff00) |
                                     ((srcval >> 24) & 0x000000ff);
        }
        srcbits = (const char*)srcbits + srclinebytes;
        dstbits = (char*)dstbits + dstlinebytes;
    }
}

static void convert_0888_reverse_src_byteswap(int width, int height,
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
            *dstpixel++=((srcval >> 8) & 0x00ffffff);
        }
        srcbits = (const char*)srcbits + srclinebytes;
        dstbits = (char*)dstbits + dstlinebytes;
    }
}

static void convert_0888_any_src_byteswap(int width, int height,
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
            FLIP_DWORD(&srcval);
            *dstpixel++=(((srcval >> rRightShift) & 0xff) << rLeftShift) |
                        (((srcval >> gRightShift) & 0xff) << gLeftShift) |
                        (((srcval >> bRightShift) & 0xff) << bLeftShift);
        }
        srcbits = (const char*)srcbits + srclinebytes;
        dstbits = (char*)dstbits + dstlinebytes;
    }
}

static void convert_0888_to_555_asis_src_byteswap(int width, int height,
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
            *dstpixel++=((srcval >>  1) & 0x7c00) | /* h */
                        ((srcval >> 14) & 0x03e0) | /* g */
                        ((srcval >> 27) & 0x001f);  /* l */
        }
        srcbits = (const char*)srcbits + srclinebytes;
        dstbits = (char*)dstbits + dstlinebytes;
    }
}

static void convert_0888_to_555_reverse_src_byteswap(int width, int height,
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
            *dstpixel++=((srcval >> 11) & 0x001f) | /* h */
                        ((srcval >> 14) & 0x03e0) | /* g */
                        ((srcval >> 17) & 0x7c00);  /* l */
        }
        srcbits = (const char*)srcbits + srclinebytes;
        dstbits = (char*)dstbits + dstlinebytes;
    }
}

static void convert_0888_to_565_asis_src_byteswap(int width, int height,
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
            *dstpixel++=((srcval >>  0) & 0xf800) | /* h */
                        ((srcval >> 13) & 0x07e0) | /* g */
                        ((srcval >> 27) & 0x001f);  /* l */
        }
        srcbits = (const char*)srcbits + srclinebytes;
        dstbits = (char*)dstbits + dstlinebytes;
    }
}

static void convert_0888_to_565_reverse_src_byteswap(int width, int height,
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
            *dstpixel++=((srcval >> 11) & 0x001f) | /* h */
                        ((srcval >> 13) & 0x07e0) | /* g */
                        ((srcval >> 16) & 0xf800);  /* l */
        }
        srcbits = (const char*)srcbits + srclinebytes;
        dstbits = (char*)dstbits + dstlinebytes;
    }
}

static void convert_any0888_to_5x5_src_byteswap(int width, int height,
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
            FLIP_DWORD(&srcval);
            *dstpixel++=(((srcval >> rRightShift) & rdst) << rLeftShift) |
                        (((srcval >> gRightShift) & gdst) << gLeftShift) |
                        (((srcval >> bRightShift) & bdst) << bLeftShift);
        }
        srcbits = (const char*)srcbits + srclinebytes;
        dstbits = (char*)dstbits + dstlinebytes;
    }
}

static void convert_0888_to_888_asis_src_byteswap(int width, int height,
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
            DWORD srcval1, srcval2;
            srcval1 = *srcpixel++;
            srcval2 = *srcpixel++;
            *dstpixel++= ((srcval1 >> 24) & 0x000000ff) | /* l1 */
                         ((srcval1 >>  8) & 0x0000ff00) | /* g1 */
                         ((srcval1 <<  8) & 0x00ff0000) | /* h1 */
                         ( srcval2        & 0xff000000);  /* l2 */
            srcval1 = *srcpixel++;
            *dstpixel++= ((srcval2 >> 16) & 0x000000ff) | /* g2 */
                         ( srcval2        & 0x0000ff00) | /* h2 */
                         ((srcval1 >>  8) & 0x00ff0000) | /* l3 */
                         ((srcval1 <<  8) & 0xff000000);  /* g3 */
            srcval2 = *srcpixel++;
            *dstpixel++= ((srcval1 >>  8) & 0x000000ff) | /* h3 */
                         ((srcval2 >> 16) & 0x0000ff00) | /* l4 */
                         ( srcval2        & 0x00ff0000) | /* g4 */
                         ((srcval2 << 16) & 0xff000000);  /* h4 */
        }
        /* And now up to 3 odd pixels */
        dstbyte=(BYTE*)dstpixel;
        for (x=0; x<oddwidth; x++) {
            DWORD srcval;
            srcval=*srcpixel++;
            FLIP_DWORD(&srcval);
            *((WORD*)dstbyte)=srcval;                   /* h, g */
            dstbyte+=sizeof(WORD);
            *dstbyte++=srcval >> 16;                    /* l */
        }
        srcbits = (const char*)srcbits + srclinebytes;
        dstbits = (char*)dstbits + dstlinebytes;
    }
}

static void convert_0888_to_888_reverse_src_byteswap(int width, int height,
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
            srcval2=    ((srcval1 >> 8 ) & 0x00ffffff); /* l1, g1, h1 */
            srcval1=*srcpixel++;
            *dstpixel++=srcval2 |
                        ((srcval1 << 16) & 0xff000000);  /* h2 */
            srcval2=    ((srcval1 >> 16) & 0x0000ffff);  /* l2, g2 */
            srcval1=*srcpixel++;
            *dstpixel++=srcval2 |
                        ((srcval1 <<  8) & 0xffff0000);  /* g3, h3 */
            srcval2=    ((srcval1 >> 24) & 0x000000ff);  /* l3 */
            srcval1=*srcpixel++;
            *dstpixel++=srcval2 |
                        srcval1;                         /* l4, g4, h4 */
        }
        /* And now up to 3 odd pixels */
        dstbyte=(BYTE*)dstpixel;
        for (x=0; x<oddwidth; x++) {
            DWORD srcval;
            srcval=*srcpixel++;
            *((WORD*)dstbyte)=((srcval >> 8) & 0xffff); /* g, h */
            dstbyte+=sizeof(WORD);
            *dstbyte++= srcval >> 24;                     /* l */
        }
        srcbits = (const char*)srcbits + srclinebytes;
        dstbits = (char*)dstbits + dstlinebytes;
    }
}

static void convert_any0888_to_rgb888_src_byteswap(int width, int height,
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
            FLIP_DWORD(&srcval);
            dstpixel[0]=(srcval >> bRightShift); /* b */
            dstpixel[1]=(srcval >> gRightShift); /* g */
            dstpixel[2]=(srcval >> rRightShift); /* r */
            dstpixel+=3;
        }
        srcbits = (const char*)srcbits + srclinebytes;
        dstbits = (char*)dstbits + dstlinebytes;
    }
}

static void convert_any0888_to_bgr888_src_byteswap(int width, int height,
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
            FLIP_DWORD(&srcval);
            dstpixel[0]=(srcval >> rRightShift); /* r */
            dstpixel[1]=(srcval >> gRightShift); /* g */
            dstpixel[2]=(srcval >> bRightShift); /* b */
            dstpixel+=3;
        }
        srcbits = (const char*)srcbits + srclinebytes;
        dstbits = (char*)dstbits + dstlinebytes;
    }
}


const dib_conversions dib_src_byteswap = {
    convert_5x5_asis_src_byteswap,
    convert_555_reverse_src_byteswap,
    convert_555_to_565_asis_src_byteswap,
    convert_555_to_565_reverse_src_byteswap,
    convert_555_to_888_asis_src_byteswap,
    convert_555_to_888_reverse_src_byteswap,
    convert_555_to_0888_asis_src_byteswap,
    convert_555_to_0888_reverse_src_byteswap,
    convert_5x5_to_any0888_src_byteswap,
    convert_565_reverse_src_byteswap,
    convert_565_to_555_asis_src_byteswap,
    convert_565_to_555_reverse_src_byteswap,
    convert_565_to_888_asis_src_byteswap,
    convert_565_to_888_reverse_src_byteswap,
    convert_565_to_0888_asis_src_byteswap,
    convert_565_to_0888_reverse_src_byteswap,
    convert_888_asis_src_byteswap,
    convert_888_reverse_src_byteswap,
    convert_888_to_555_asis_src_byteswap,
    convert_888_to_555_reverse_src_byteswap,
    convert_888_to_565_asis_src_byteswap,
    convert_888_to_565_reverse_src_byteswap,
    convert_888_to_0888_asis_src_byteswap,
    convert_888_to_0888_reverse_src_byteswap,
    convert_rgb888_to_any0888_src_byteswap,
    convert_bgr888_to_any0888_src_byteswap,
    convert_0888_asis_src_byteswap,
    convert_0888_reverse_src_byteswap,
    convert_0888_any_src_byteswap,
    convert_0888_to_555_asis_src_byteswap,
    convert_0888_to_555_reverse_src_byteswap,
    convert_0888_to_565_asis_src_byteswap,
    convert_0888_to_565_reverse_src_byteswap,
    convert_any0888_to_5x5_src_byteswap,
    convert_0888_to_888_asis_src_byteswap,
    convert_0888_to_888_reverse_src_byteswap,
    convert_any0888_to_rgb888_src_byteswap,
    convert_any0888_to_bgr888_src_byteswap
};
