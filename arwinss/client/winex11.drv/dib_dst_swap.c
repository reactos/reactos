/*
 * DIB conversion routinues for cases where the destination
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

static void convert_5x5_asis_dst_byteswap(int width, int height,
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

static void convert_555_reverse_dst_byteswap(int width, int height,
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
            *dstpixel++=((srcval >>  2) & 0x1f001f00) | /* h */
                        ((srcval >>  8) & 0x00030003) | /* g - 2 bits */
                        ((srcval <<  8) & 0xe000e000) | /* g - 3 bits */
                        ((srcval <<  2) & 0x007c007c);  /* l */
        }
        if (width&1) {
            /* And then the odd pixel */
            WORD srcval;
            srcval=*((const WORD*)srcpixel);
            *((WORD*)dstpixel)=((srcval >>  2) & 0x1f00) | /* h */
                               ((srcval >>  8) & 0x0003) | /* g - 2 bits */
                               ((srcval <<  8) & 0xe000) | /* g - 3 bits */
                               ((srcval <<  2) & 0x007c);  /* l */
        }
        srcbits = (const char*)srcbits + srclinebytes;
        dstbits = (char*)dstbits + dstlinebytes;
    }
}

static void convert_555_to_565_asis_dst_byteswap(int width, int height,
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
            *dstpixel++=((srcval >> 7) & 0x00ff00ff) | /* h, g - 3 bits */
                        ((srcval << 9) & 0xc000c000) | /* g - 2 bits */
                        ((srcval << 4) & 0x20002000) | /* g - 1 bits */
                        ((srcval << 8) & 0x1f001f00);  /* l */
        }
        if (width&1) {
            /* And then the odd pixel */
            WORD srcval;
            srcval=*((const WORD*)srcpixel);
            *((WORD*)dstpixel)=((srcval >> 7) & 0x00ff) | /* h, g - 3 bits */
                               ((srcval << 9) & 0xc000) | /* g - 2 bits */
                               ((srcval << 4) & 0x2000) | /* g - 1 bit */
                               ((srcval << 8) & 0x1f00);  /* l */
        }
        srcbits = (const char*)srcbits + srclinebytes;
        dstbits = (char*)dstbits + dstlinebytes;
    }
}

static void convert_555_to_565_reverse_dst_byteswap(int width, int height,
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
            *dstpixel++=((srcval >>  2) & 0x1f001f00) | /* h */
                        ((srcval >>  7) & 0x00070007) | /* g - 3 bits */
                        ((srcval <<  9) & 0xc000c000) | /* g - 2 bits */
                        ((srcval <<  4) & 0x20002000) | /* g - 1 bit */
                        ((srcval <<  3) & 0x00f800f8);  /* l */
        }
        if (width&1) {
            /* And then the odd pixel */
            WORD srcval;
            srcval=*((const WORD*)srcpixel);
            *((WORD*)dstpixel)=((srcval >>  2) & 0x1f00) | /* h */
                               ((srcval >>  7) & 0x0007) | /* g - 3 bits */
                               ((srcval <<  9) & 0xc000) | /* g - 2 bits */
                               ((srcval <<  4) & 0x2000) | /* g - 1 bit */
                               ((srcval <<  3) & 0x00f8);  /* l */
        }
        srcbits = (const char*)srcbits + srclinebytes;
        dstbits = (char*)dstbits + dstlinebytes;
    }
}

static void convert_555_to_888_asis_dst_byteswap(int width, int height,
                                                 const void* srcbits, int srclinebytes,
                                                 void* dstbits, int dstlinebytes)
{
    const WORD* srcpixel;
    DWORD* dstpixel;
    int x,y;

    for (y=0; y<height; y++) {
        srcpixel=srcbits;
        dstpixel=dstbits;
        for (x=0; x<width/4; x++) {
            /* Do 4 pixels at a time.  4 words in 3 dwords out */
            DWORD srcval1, srcval2;
            srcval1=(DWORD)*srcpixel++;
            srcval2=(DWORD)*srcpixel++;
            *dstpixel++= ((srcval1 << 27) & 0xf8000000) | /* l1 */
                         ((srcval1 << 22) & 0x07000000) | /* l1 - 3 bits */
                         ((srcval1 << 14) & 0x00f80000) | /* g1 */
                         ((srcval1 <<  9) & 0x00070000) | /* g1 - 3 bits */
                         ((srcval1 <<  1) & 0x0000f800) | /* h1 */
                         ((srcval1 >>  4) & 0x00070000) | /* h1 - 3 bits */
                         ((srcval2 <<  3) & 0x000000f8) | /* l2 */
                         ((srcval2 >>  2) & 0x00000007);  /* l2 - 3 bits */
            srcval1=(DWORD)*srcpixel++;
            *dstpixel++= ((srcval2 << 22) & 0xf8000000) | /* g2 */
                         ((srcval2 << 17) & 0x07000000) | /* g2 - 3 bits */
                         ((srcval2 <<  9) & 0x00f80000) | /* h2 */
                         ((srcval2 <<  4) & 0x00070000) | /* h2 - 3 bits */
                         ((srcval1 << 11) & 0x0000f800) | /* l3 */
                         ((srcval1 <<  6) & 0x00000700) | /* l3 - 3 bits */
                         ((srcval1 >>  2) & 0x000000f8) | /* g3 */
                         ((srcval1 >>  7) & 0x00000007);  /* g3 - 3 bits */
            srcval2=(DWORD)*srcpixel++;
            *dstpixel++= ((srcval1 << 17) & 0xf8000000) | /* h3 */
                         ((srcval1 << 12) & 0x07000000) | /* h3 - 3 bits */
                         ((srcval2 << 19) & 0x00f80000) | /* l4 */
                         ((srcval2 << 14) & 0x00070000) | /* l4 - 3 bits */
                         ((srcval2 <<  6) & 0x0000f800) | /* g4 */
                         ((srcval2 <<  1) & 0x00000700) | /* g4 - 3 bits */
                         ((srcval2 >>  7) & 0x000000f8) | /* h4 */
                         ((srcval2 >> 12) & 0x00000007);  /* h4 - 3 bits */
        }
        if(width&3) {
            BYTE *dstbyte = (BYTE*)dstpixel;
            DWORD srcval;
            for(x = 0; x < (width&3); x++) {
                srcval = *srcpixel++;
                dstbyte[0] = ((srcval <<  3) & 0xf8) | ((srcval >>  2) & 0x07);
                dstbyte[1] = ((srcval >>  2) & 0xf8) | ((srcval >>  7) & 0x07);
                dstbyte[2] = ((srcval >>  7) & 0xf8) | ((srcval >> 12) & 0x07);
                dstbyte+=3;
                if(x > 0)
                    FLIP_DWORD(dstpixel + x - 1);
            }
            FLIP_DWORD(dstpixel + x - 1);
        }
        srcbits = (const char*)srcbits + srclinebytes;
        dstbits = (char*)dstbits + dstlinebytes;
    }
}

static void convert_555_to_888_reverse_dst_byteswap(int width, int height,
                                                    const void* srcbits, int srclinebytes,
                                                    void* dstbits, int dstlinebytes)
{
    const WORD* srcpixel;
    DWORD* dstpixel;
    int x,y;

    for (y=0; y<height; y++) {
        srcpixel=srcbits;
        dstpixel=dstbits;
        for (x=0; x<width/4; x++) {
            /* Do 4 pixels at a time.  4 words in 3 dwords out */
            DWORD srcval1, srcval2;
            srcval1=(DWORD)*srcpixel++;
            srcval2=(DWORD)*srcpixel++;
            *dstpixel++= ((srcval1 << 17) & 0xf8000000) | /* h1 */
                         ((srcval1 << 12) & 0x07000000) | /* h1 - 3 bits */
                         ((srcval1 << 14) & 0x00f80000) | /* g1 */
                         ((srcval1 <<  9) & 0x00070000) | /* g1 - 3 bits */
                         ((srcval1 << 11) & 0x0000f800) | /* l1 */
                         ((srcval1 <<  6) & 0x00070000) | /* l1 - 3 bits */
                         ((srcval2 >>  7) & 0x000000f8) | /* h2 */
                         ((srcval2 >> 12) & 0x00000007);  /* h2 - 3 bits */
            srcval1=(DWORD)*srcpixel++;
            *dstpixel++= ((srcval2 << 22) & 0xf8000000) | /* g2 */
                         ((srcval2 << 17) & 0x07000000) | /* g2 - 3 bits */
                         ((srcval2 << 19) & 0x00f80000) | /* l2 */
                         ((srcval2 << 14) & 0x00070000) | /* l2 - 3 bits */
                         ((srcval1 <<  1) & 0x0000f800) | /* h3 */
                         ((srcval1 >>  4) & 0x00000700) | /* h3 - 3 bits */
                         ((srcval1 >>  2) & 0x000000f8) | /* g3 */
                         ((srcval1 >>  7) & 0x00000007);  /* g3 - 3 bits */
            srcval2=(DWORD)*srcpixel++;
            *dstpixel++= ((srcval1 << 27) & 0xf8000000) | /* l3 */
                         ((srcval1 << 22) & 0x07000000) | /* l3 - 3 bits */
                         ((srcval2 <<  9) & 0x00f80000) | /* h4 */
                         ((srcval2 <<  4) & 0x00070000) | /* h4 - 3 bits */
                         ((srcval2 <<  6) & 0x0000f800) | /* g4 */
                         ((srcval2 <<  1) & 0x00000700) | /* g4 - 3 bits */
                         ((srcval2 <<  3) & 0x000000f8) | /* l4 */
                         ((srcval2 >>  2) & 0x00000007);  /* l4 - 3 bits */
        }
        if(width&3) {
            BYTE *dstbyte = (BYTE*)dstpixel;
            DWORD srcval;
            for(x = 0; x < (width&3); x++) {
                srcval = *srcpixel++;
                dstbyte[2] = ((srcval <<  3) & 0xf8) | ((srcval >>  2) & 0x07);
                dstbyte[1] = ((srcval >>  2) & 0xf8) | ((srcval >>  7) & 0x07);
                dstbyte[0] = ((srcval >>  7) & 0xf8) | ((srcval >> 12) & 0x07);
                dstbyte+=3;
                if(x > 0)
                    FLIP_DWORD(dstpixel + x - 1);
            }
            FLIP_DWORD(dstpixel + x - 1);
        }
        srcbits = (const char*)srcbits + srclinebytes;
        dstbits = (char*)dstbits + dstlinebytes;
    }
}

static void convert_555_to_0888_asis_dst_byteswap(int width, int height,
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
            *dstpixel++=((srcval <<  1) & 0x0000f800) | /* h */
                        ((srcval >>  4) & 0x00000700) | /* h - 3 bits */
                        ((srcval << 14) & 0x00f80000) | /* g */
                        ((srcval <<  9) & 0x00070000) | /* g - 3 bits */
                        ((srcval << 27) & 0xf8000000) | /* l */
                        ((srcval << 22) & 0x07000000);  /* l - 3 bits */
        }
        srcbits = (const char*)srcbits + srclinebytes;
        dstbits = (char*)dstbits + dstlinebytes;
    }
}

static void convert_555_to_0888_reverse_dst_byteswap(int width, int height,
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
            *dstpixel++=((srcval << 17) & 0xf8000000) | /* h */
                        ((srcval << 12) & 0x07000000) | /* h - 3 bits */
                        ((srcval << 14) & 0x00f80000) | /* g */
                        ((srcval <<  9) & 0x00070000) | /* g - 3 bits */
                        ((srcval << 11) & 0x0000f800) | /* l */
                        ((srcval <<  6) & 0x00000700);  /* l - 3 bits */
        }
        srcbits = (const char*)srcbits + srclinebytes;
        dstbits = (char*)dstbits + dstlinebytes;
    }
}

static void convert_5x5_to_any0888_dst_byteswap(int width, int height,
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
            *dstpixel  =(red   << rLeftShift) |
                        (green << gLeftShift) |
                        (blue  << bLeftShift);
            FLIP_DWORD(dstpixel);
            dstpixel++;
        }
        srcbits = (const char*)srcbits + srclinebytes;
        dstbits = (char*)dstbits + dstlinebytes;
    }
}

/*
 * 16 bits conversions
 */

static void convert_565_reverse_dst_byteswap(int width, int height,
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
            *dstpixel++=((srcval >>  3) & 0x1f001f00) | /* h */
                        ((srcval >>  8) & 0x00070007) | /* g - 3 bits */
                        ((srcval <<  8) & 0xe000e000) | /* g - 3 bits */
                        ((srcval <<  3) & 0x00f800f8);  /* l */
        }
        if (width&1) {
            /* And then the odd pixel */
            WORD srcval;
            srcval=*((const WORD*)srcpixel);
            *((WORD*)dstpixel)=((srcval >>  3) & 0x1f00) | /* h */
                               ((srcval >>  8) & 0x0007) | /* g - 3 bits */
                               ((srcval <<  8) & 0xe000) | /* g - 3 bits */
                               ((srcval <<  3) & 0x00f8);  /* l */
        }
        srcbits = (const char*)srcbits + srclinebytes;
        dstbits = (char*)dstbits + dstlinebytes;
    }
}

static void convert_565_to_555_asis_dst_byteswap(int width, int height,
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
            *dstpixel++=((srcval <<  7) & 0xe000e000) | /* g - 3 bits */
                        ((srcval <<  8) & 0x1f001f00) | /* l */
                        ((srcval >>  9) & 0x007f007f);  /* h, g - 2 bits */
        }
        if (width&1) {
            /* And then the odd pixel */
            WORD srcval;
            srcval=*((const WORD*)srcpixel);
            *((WORD*)dstpixel)=((srcval <<  7) & 0xe000) | /* g - 3 bits*/
                               ((srcval <<  8) & 0x1f00) | /* l */
                               ((srcval >>  9) & 0x007f);  /* h, g - 2 bits */
        }
        srcbits = (const char*)srcbits + srclinebytes;
        dstbits = (char*)dstbits + dstlinebytes;
    }
}

static void convert_565_to_555_reverse_dst_byteswap(int width, int height,
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
            *dstpixel++=((srcval <<  7) & 0xe000e000) | /* g - 3 bits */
                        ((srcval >>  3) & 0x1f001f00) | /* l */
                        ((srcval <<  2) & 0x007c007c) | /* h */
                        ((srcval >>  9) & 0x00030003);  /* g - 2 bits */
        }
        if (width&1) {
            /* And then the odd pixel */
            WORD srcval;
            srcval=*((const WORD*)srcpixel);
            *((WORD*)dstpixel)=((srcval <<  7) & 0xe000) | /* g - 3 bits */
                               ((srcval >>  3) & 0x1f00) | /* l */
                               ((srcval <<  2) & 0x007c) | /* h */
                               ((srcval >>  9) & 0x0003);  /* g - 2 bits */
        }
        srcbits = (const char*)srcbits + srclinebytes;
        dstbits = (char*)dstbits + dstlinebytes;
    }
}

static void convert_565_to_888_asis_dst_byteswap(int width, int height,
                                                 const void* srcbits, int srclinebytes,
                                                 void* dstbits, int dstlinebytes)
{
    const WORD* srcpixel;
    DWORD* dstpixel;
    int x,y;

    for (y=0; y<height; y++) {
        srcpixel=srcbits;
        dstpixel=dstbits;
        for (x=0; x<width/4; x++) {
            /* Do 4 pixels at a time.  4 words in 3 dwords out */
            DWORD srcval1, srcval2;
            srcval1=(DWORD)*srcpixel++;
            srcval2=(DWORD)*srcpixel++;
            *dstpixel++= ((srcval1 << 27) & 0xf8000000) | /* l1 */
                         ((srcval1 << 22) & 0x07000000) | /* l1 - 3 bits */
                         ((srcval1 << 13) & 0x00fc0000) | /* g1 */
                         ((srcval1 <<  7) & 0x00030000) | /* g1 - 2 bits */
                         ((srcval1 <<  0) & 0x0000f800) | /* h1 */
                         ((srcval1 >>  5) & 0x00070000) | /* h1 - 3 bits */
                         ((srcval2 <<  3) & 0x000000f8) | /* l2 */
                         ((srcval2 >>  2) & 0x00000007);  /* l2 - 3 bits */
            srcval1=(DWORD)*srcpixel++;
            *dstpixel++= ((srcval2 << 21) & 0xfc000000) | /* g2 */
                         ((srcval2 << 15) & 0x03000000) | /* g2 - 2 bits */
                         ((srcval2 <<  8) & 0x00f80000) | /* h2 */
                         ((srcval2 <<  3) & 0x00070000) | /* h2 - 3 bits */
                         ((srcval1 << 11) & 0x0000f800) | /* l3 */
                         ((srcval1 <<  6) & 0x00000700) | /* l3 - 3 bits */
                         ((srcval1 >>  3) & 0x000000fc) | /* g3 */
                         ((srcval1 >>  9) & 0x00000003);  /* g3 - 2 bits */
            srcval2=(DWORD)*srcpixel++;
            *dstpixel++= ((srcval1 << 16) & 0xf8000000) | /* h3 */
                         ((srcval1 << 11) & 0x07000000) | /* h3 - 3 bits */
                         ((srcval2 << 19) & 0x00f80000) | /* l4 */
                         ((srcval2 << 14) & 0x00070000) | /* l4 - 3 bits */
                         ((srcval2 <<  5) & 0x0000fc00) | /* g4 */
                         ((srcval2 >>  1) & 0x00000300) | /* g4 - 2 bits */
                         ((srcval2 >>  8) & 0x000000f8) | /* h4 */
                         ((srcval2 >> 13) & 0x00000007);  /* h4 - 3 bits */
        }
        if(width&3) {
            BYTE *dstbyte = (BYTE*)dstpixel;
            DWORD srcval;
            for(x = 0; x < (width&3); x++) {
                srcval = *srcpixel++;
                dstbyte[0] = ((srcval <<  3) & 0xf8) | ((srcval >>  2) & 0x07);
                dstbyte[1] = ((srcval >>  3) & 0xfc) | ((srcval >>  9) & 0x03);
                dstbyte[2] = ((srcval >>  8) & 0xf8) | ((srcval >> 13) & 0x07);
                dstbyte+=3;
                if(x > 0)
                    FLIP_DWORD(dstpixel + x - 1);
            }
            FLIP_DWORD(dstpixel + x - 1);
        }

        srcbits = (const char*)srcbits + srclinebytes;
        dstbits = (char*)dstbits + dstlinebytes;
    }
}

static void convert_565_to_888_reverse_dst_byteswap(int width, int height,
                                                    const void* srcbits, int srclinebytes,
                                                    void* dstbits, int dstlinebytes)
{
    const WORD* srcpixel;
    DWORD* dstpixel;
    int x,y;

    for (y=0; y<height; y++) {
        srcpixel=srcbits;
        dstpixel=dstbits;
        for (x=0; x<width/4; x++) {
            /* Do 4 pixels at a time.  4 words in 3 dwords out */
            DWORD srcval1, srcval2;
            srcval1=(DWORD)*srcpixel++;
            srcval2=(DWORD)*srcpixel++;
            *dstpixel++= ((srcval1 << 16) & 0xf8000000) | /* h1 */
                         ((srcval1 << 11) & 0x07000000) | /* h1 - 3 bits */
                         ((srcval1 << 13) & 0x00fc0000) | /* g1 */
                         ((srcval1 <<  7) & 0x00030000) | /* g1 - 2 bits */
                         ((srcval1 << 11) & 0x0000f800) | /* l1 */
                         ((srcval1 <<  6) & 0x00070000) | /* l1 - 3 bits */
                         ((srcval2 >>  8) & 0x000000f8) | /* h2 */
                         ((srcval2 >> 13) & 0x00000007);  /* h2 - 3 bits */
            srcval1=(DWORD)*srcpixel++;
            *dstpixel++= ((srcval2 << 21) & 0xfc000000) | /* g2 */
                         ((srcval2 << 15) & 0x03000000) | /* g2 - 2 bits */
                         ((srcval2 << 19) & 0x00f80000) | /* l2 */
                         ((srcval2 << 14) & 0x00070000) | /* l2 - 3 bits */
                         ((srcval1 <<  0) & 0x0000f800) | /* h3 */
                         ((srcval1 >>  5) & 0x00000700) | /* h3 - 3 bits */
                         ((srcval1 >>  3) & 0x000000fc) | /* g3 */
                         ((srcval1 >>  9) & 0x00000003);  /* g3 - 2 bits */
            srcval2=(DWORD)*srcpixel++;
            *dstpixel++= ((srcval1 << 27) & 0xf8000000) | /* l3 */
                         ((srcval1 << 22) & 0x07000000) | /* l3 - 3 bits */
                         ((srcval2 <<  8) & 0x00f80000) | /* h4 */
                         ((srcval2 <<  3) & 0x00070000) | /* h4 - 3 bits */
                         ((srcval2 <<  5) & 0x0000fc00) | /* g4 */
                         ((srcval2 >>  1) & 0x00000700) | /* g4 - 2 bits */
                         ((srcval2 <<  3) & 0x000000f8) | /* l4 */
                         ((srcval2 >>  2) & 0x00000007);  /* l4 - 3 bits */
        }
        if(width&3) {
            BYTE *dstbyte = (BYTE*)dstpixel;
            DWORD srcval;
            for(x = 0; x < (width&3); x++) {
                srcval = *srcpixel++;
                dstbyte[2] = ((srcval <<  3) & 0xf8) | ((srcval >>  2) & 0x07);
                dstbyte[1] = ((srcval >>  3) & 0xfc) | ((srcval >>  9) & 0x03);
                dstbyte[0] = ((srcval >>  8) & 0xf8) | ((srcval >> 13) & 0x07);
                dstbyte+=3;
                if(x > 0)
                    FLIP_DWORD(dstpixel + x - 1);
            }
            FLIP_DWORD(dstpixel + x - 1);
        }
        srcbits = (const char*)srcbits + srclinebytes;
        dstbits = (char*)dstbits + dstlinebytes;
    }
}

static void convert_565_to_0888_asis_dst_byteswap(int width, int height,
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
            *dstpixel++=((srcval <<  0) & 0x0000f800) | /* h */
                        ((srcval >>  5) & 0x00000700) | /* h - 3 bits */
                        ((srcval << 13) & 0x00fc0000) | /* g */
                        ((srcval <<  7) & 0x00030000) | /* g - 2 bits */
                        ((srcval << 27) & 0xf8000000) | /* l */
                        ((srcval << 22) & 0x07000000);  /* l - 3 bits */
        }
        srcbits = (const char*)srcbits + srclinebytes;
        dstbits = (char*)dstbits + dstlinebytes;
    }
}

static void convert_565_to_0888_reverse_dst_byteswap(int width, int height,
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
            *dstpixel++=((srcval << 16) & 0xf8000000) | /* h */
                        ((srcval << 11) & 0x07000000) | /* h - 3 bits */
                        ((srcval << 13) & 0x00fc0000) | /* g */
                        ((srcval <<  7) & 0x00030000) | /* g - 2 bits */
                        ((srcval << 11) & 0x0000f800) | /* l */
                        ((srcval <<  6) & 0x00000700);  /* l - 3 bits */
        }
        srcbits = (const char*)srcbits + srclinebytes;
        dstbits = (char*)dstbits + dstlinebytes;
    }
}

/*
 * 24 bit conversions
 */

static void convert_888_asis_dst_byteswap(int width, int height,
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

static void convert_888_reverse_dst_byteswap(int width, int height,
                                             const void* srcbits, int srclinebytes,
                                             void* dstbits, int dstlinebytes)
{
    const DWORD* srcpixel;
    DWORD* dstpixel;
    int x,y;

    for (y=0; y<height; y++) {
        srcpixel=srcbits;
        dstpixel=dstbits;
        for (x=0; x<width/4; x++) {
            /* Do 4 pixels at a time.  3 dwords in 3 dwords out */
            *dstpixel++=((srcpixel[0] <<  8) & 0xffffff00) | /* h1, g1, l1 */
                        ((srcpixel[1] >>  8) & 0x000000ff);  /* h2 */
            *dstpixel++=((srcpixel[1] << 24) & 0xff000000) | /* g2 */
                        ((srcpixel[0] >>  8) & 0x00ff0000) | /* l2 */
                        ((srcpixel[2] <<  8) & 0x0000ff00) | /* h3 */
                        ((srcpixel[0] >> 24) & 0x000000ff);  /* g3 */
            *dstpixel++=((srcpixel[1] <<  8) & 0xff000000) | /* l3 */
                        ((srcpixel[2] >>  8) & 0x00ffffff);  /* h4, g4, l4 */
            srcpixel+=3;
        }
        if(width&3) {
            BYTE *dstbyte = (BYTE*)dstpixel;
            const BYTE *srcbyte = (const BYTE*)srcpixel;
            for(x = 0; x < (width&3); x++) {
                dstbyte[2] = srcbyte[0];
                dstbyte[1] = srcbyte[1];
                dstbyte[0] = srcbyte[2];
                dstbyte+=3;
                srcbyte+=3;
                if(x > 0)
                    FLIP_DWORD(dstpixel + x - 1);
            }
            FLIP_DWORD(dstpixel + x - 1);
        }
        srcbits = (const char*)srcbits + srclinebytes;
        dstbits = (char*)dstbits + dstlinebytes;
    }
}

static void convert_888_to_555_asis_dst_byteswap(int width, int height,
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
            dstpixel[0]=((srcval1 <<  5) & 0x1f00) | /* l1 */
                        ((srcval1 >> 14) & 0x0003) | /* g1 - 2 bits */
                        ((srcval1 <<  2) & 0xe000) | /* g1 - 3 bits */
                        ((srcval1 >> 17) & 0x007c);  /* h1 */
            srcval2=srcpixel[1];
            dstpixel[1]=((srcval1 >> 19) & 0x1f00) | /* l2 */
                        ((srcval2 >>  6) & 0x0003) | /* g2 - 2 bits */
                        ((srcval2 << 10) & 0xe000) | /* g2 - 3 bits */
                        ((srcval2 >>  9) & 0x007c);  /* h2 */
            srcval1=srcpixel[2];
            dstpixel[2]=((srcval2 >> 11) & 0x1f00) | /* l3 */
                        ((srcval2 >> 30) & 0x0003) | /* g3 - 2 bits */
                        ((srcval2 >> 14) & 0xe000) | /* g3 - 3 bits */
                        ((srcval1 >>  1) & 0x007c);  /* h3 */
            dstpixel[3]=((srcval1 >>  3) & 0x1f00) | /* l4 */
                        ((srcval1 >> 22) & 0x0003) | /* g4 - 2 bits */
                        ((srcval1 >>  6) & 0xe000) | /* g4 - 3 bits */
                        ((srcval1 >> 17) & 0x007c);  /* h4 */
            srcpixel+=3;
            dstpixel+=4;
        }
        /* And now up to 3 odd pixels */
        srcbyte=(const BYTE*)srcpixel;
        for (x=0; x<oddwidth; x++) {
            WORD dstval;
            dstval =((srcbyte[0] <<  5) & 0x1f00);    /* l */
            dstval|=((srcbyte[1] >>  6) & 0x0003);    /* g - 2 bits */
            dstval|=((srcbyte[1] << 10) & 0xe000);    /* g - 3 bits */
            dstval|=((srcbyte[2] >>  1) & 0x007c);    /* h */
            *dstpixel++=dstval;
            srcbyte+=3;
        }
        srcbits = (const char*)srcbits + srclinebytes;
        dstbits = (char*)dstbits + dstlinebytes;
    }
}

static void convert_888_to_555_reverse_dst_byteswap(int width, int height,
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
            dstpixel[0]=((srcval1 >>  1) & 0x007c) | /* l1 */
                        ((srcval1 >> 14) & 0x0003) | /* g1 - 2 bits */
                        ((srcval1 <<  2) & 0xe000) | /* g1 - 3 bits */
                        ((srcval1 >> 11) & 0x1f00);  /* h1 */
            srcval2=srcpixel[1];
            dstpixel[1]=((srcval1 >> 25) & 0x007c) | /* l2 */
                        ((srcval2 >>  6) & 0x0003) | /* g2 - 2 bits */
                        ((srcval2 << 10) & 0xe000) | /* g2 - 3 bits */
                        ((srcval2 >>  3) & 0x1f00);  /* h2 */
            srcval1=srcpixel[2];
            dstpixel[2]=((srcval2 >> 17) & 0x007c) | /* l3 */
                        ((srcval2 >> 30) & 0x0003) | /* g3 - 2 bits */
                        ((srcval2 >> 14) & 0xe000) | /* g3 - 3 bits */
                        ((srcval1 <<  5) & 0x1f00);  /* h3 */
            dstpixel[3]=((srcval1 >>  9) & 0x007c) | /* l4 */
                        ((srcval1 >> 22) & 0x0003) | /* g4 - 2 bits */
                        ((srcval1 >>  6) & 0xe000) | /* g4 - 3 bits */
                        ((srcval1 >> 19) & 0x1f00);  /* h4 */
            srcpixel+=3;
            dstpixel+=4;
        }
        /* And now up to 3 odd pixels */
        srcbyte=(const BYTE*)srcpixel;
        for (x=0; x<oddwidth; x++) {
            WORD dstval;
            dstval =((srcbyte[0] >>  1) & 0x007c);    /* l */
            dstval|=((srcbyte[1] >>  6) & 0x0003);    /* g - 2 bits */
            dstval|=((srcbyte[1] << 10) & 0xe000);    /* g - 3 bits */
            dstval|=((srcbyte[2] <<  5) & 0x1f00);    /* h */
            FLIP_WORD(&dstval);
            *dstpixel++=dstval;
            srcbyte+=3;
        }
        srcbits = (const char*)srcbits + srclinebytes;
        dstbits = (char*)dstbits + dstlinebytes;
    }
}

static void convert_888_to_565_asis_dst_byteswap(int width, int height,
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
            dstpixel[0]=((srcval1 <<  5) & 0x1f00) | /* l1 */
                        ((srcval1 >> 13) & 0x0007) | /* g1 - 3 bits */
                        ((srcval1 <<  3) & 0xe000) | /* g1 - 3 bits */
                        ((srcval1 >> 16) & 0x00f8);  /* h1 */
            srcval2=srcpixel[1];
            dstpixel[1]=((srcval1 >> 19) & 0x1f00) | /* l2 */
                        ((srcval2 >>  5) & 0x0007) | /* g2 - 3 bits */
                        ((srcval2 << 11) & 0xe000) | /* g2 - 3 bits */
                        ((srcval2 >>  8) & 0x00f8);  /* h2 */
            srcval1=srcpixel[2];
            dstpixel[2]=((srcval2 >> 11) & 0x1f00) | /* l3 */
                        ((srcval2 >> 29) & 0x0007) | /* g3 - 3 bits */
                        ((srcval2 >> 13) & 0xe000) | /* g3 - 3 bits */
                        ((srcval1 <<  0) & 0x00f8);  /* h3 */
            dstpixel[3]=((srcval1 >>  3) & 0x1f00) | /* l4 */
                        ((srcval1 >> 21) & 0x0007) | /* g4 - 3 bits */
                        ((srcval1 >>  5) & 0xe000) | /* g4 - 3 bits */
                        ((srcval1 >> 24) & 0x00f8);  /* h4 */
            srcpixel+=3;
            dstpixel+=4;
        }
        /* And now up to 3 odd pixels */
        srcbyte=(const BYTE*)srcpixel;
        for (x=0; x<oddwidth; x++) {
            WORD dstval;
            dstval =((srcbyte[0] <<  5) & 0x1f00);    /* l */
            dstval|=((srcbyte[1] >>  5) & 0x0007);    /* g - 3 bits */
            dstval|=((srcbyte[1] << 11) & 0xe000);    /* g - 3 bits */
            dstval|=((srcbyte[2] <<  0) & 0x00f8);    /* h */
            *dstpixel++=dstval;
            srcbyte+=3;
        }
        srcbits = (const char*)srcbits + srclinebytes;
        dstbits = (char*)dstbits + dstlinebytes;
    }
}

static void convert_888_to_565_reverse_dst_byteswap(int width, int height,
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
            dstpixel[0]=((srcval1 >>  0) & 0x00f8) | /* l1 */
                        ((srcval1 >> 13) & 0x0007) | /* g1 - 3 bits */
                        ((srcval1 <<  3) & 0xe000) | /* g1 - 3 bits */
                        ((srcval1 >> 11) & 0x1f00);  /* h1 */
            srcval2=srcpixel[1];
            dstpixel[1]=((srcval1 >> 24) & 0x00f8) | /* l2 */
                        ((srcval2 >>  5) & 0x0007) | /* g2 - 3 bits */
                        ((srcval2 << 11) & 0xe000) | /* g2 - 3 bits */
                        ((srcval2 >>  3) & 0x1f00);  /* h2 */
            srcval1=srcpixel[2];
            dstpixel[2]=((srcval2 >> 16) & 0x00f8) | /* l3 */
                        ((srcval2 >> 29) & 0x0007) | /* g3 - 3 bits */
                        ((srcval2 >> 13) & 0xe000) | /* g3 - 3 bits */
                        ((srcval1 <<  5) & 0x1f00);  /* h3 */
            dstpixel[3]=((srcval1 >>  8) & 0x00f8) | /* l4 */
                        ((srcval1 >> 21) & 0x0007) | /* g4 - 3 bits */
                        ((srcval1 >>  5) & 0xe000) | /* g4 - 3 bits */
                        ((srcval1 >> 19) & 0x1f00);  /* h4 */
            srcpixel+=3;
            dstpixel+=4;
        }
        /* And now up to 3 odd pixels */
        srcbyte=(const BYTE*)srcpixel;
        for (x=0; x<oddwidth; x++) {
            WORD dstval;
            dstval =((srcbyte[0] <<  0) & 0x00f8);    /* l */
            dstval|=((srcbyte[1] >>  5) & 0x0007);    /* g - 3 bits */
            dstval|=((srcbyte[1] << 11) & 0xe000);    /* g - 3 bits */
            dstval|=((srcbyte[2] <<  5) & 0x1f00);    /* h */
            *dstpixel++=dstval;
            srcbyte+=3;
        }
        srcbits = (const char*)srcbits + srclinebytes;
        dstbits = (char*)dstbits + dstlinebytes;
    }
}

static void convert_888_to_0888_asis_dst_byteswap(int width, int height,
                                                  const void* srcbits, int srclinebytes,
                                                  void* dstbits, int dstlinebytes)
{
    const DWORD* srcpixel;
    DWORD* dstpixel;
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
            srcval1=*srcpixel++;
            *dstpixel++=((srcval1 << 24) & 0xff000000) | /* l1 */
                        ((srcval1 <<  8) & 0x00ff0000) | /* g1 */
                        ((srcval1 >>  8) & 0x0000ff00);  /* h1 */
            srcval2=*srcpixel++;
            *dstpixel++=((srcval1 <<  0) & 0xff000000) | /* l2 */
                        ((srcval2 << 16) & 0x00ff0000) | /* g2 */
                        ((srcval2 <<  0) & 0x0000ff00);  /* h2 */
            srcval1=*srcpixel++;
            *dstpixel++=((srcval2 <<  8) & 0xff000000) | /* l3 */
                        ((srcval2 >>  8) & 0x00ff0000) | /* g3 */
                        ((srcval1 <<  8) & 0x0000ff00);  /* h3 */
            *dstpixel++=((srcval1 << 16) & 0xff000000) | /* l4 */
                        ((srcval1 <<  0) & 0x00ff0000) | /* g4 */
                        ((srcval1 >> 16) & 0x0000ff00);  /* h4 */
        }
        /* And now up to 3 odd pixels */
        for (x=0; x<oddwidth; x++) {
            DWORD srcval;
            srcval=*srcpixel;
            srcpixel=(const DWORD*)(((const char*)srcpixel)+3);
            *dstpixel++=((srcval << 24) & 0xff000000) | /* l */
                        ((srcval <<  8) & 0x00ff0000) | /* g */
                        ((srcval >>  8) & 0x0000ff00);  /* h */
        }
        srcbits = (const char*)srcbits + srclinebytes;
        dstbits = (char*)dstbits + dstlinebytes;
    }
}

static void convert_888_to_0888_reverse_dst_byteswap(int width, int height,
                                                     const void* srcbits, int srclinebytes,
                                                     void* dstbits, int dstlinebytes)
{
    const DWORD* srcpixel;
    DWORD* dstpixel;
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

            srcval1=*srcpixel++;
            *dstpixel++=((srcval1 <<  8) & 0xffffff00);  /* h1, g1, l1 */
            srcval2=*srcpixel++;
            *dstpixel++=((srcval2 << 16) & 0xffff0000) | /* h2, g2 */
                        ((srcval1 >> 16) & 0x0000ff00); /* l2 */
            srcval1=*srcpixel++;
            *dstpixel++=((srcval1 << 24) & 0xff000000) | /* h3 */
                        ((srcval2 >>  8) & 0x00ffff00);  /* g3, l3 */
            *dstpixel++=((srcval1 >>  0) & 0xffffff00);  /* h4, g4, l4 */
        }
        /* And now up to 3 odd pixels */
        for (x=0; x<oddwidth; x++) {
            DWORD srcval;
            srcval=*srcpixel;
            srcpixel=(const DWORD*)(((const char*)srcpixel)+3);
            *dstpixel++=((srcval << 8) & 0xffffff00);
        }
        srcbits = (const char*)srcbits + srclinebytes;
        dstbits = (char*)dstbits + dstlinebytes;
    }
}

static void convert_rgb888_to_any0888_dst_byteswap(int width, int height,
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
            *dstpixel  =(srcpixel[0] << bLeftShift) | /* b */
                        (srcpixel[1] << gLeftShift) | /* g */
                        (srcpixel[2] << rLeftShift);  /* r */
            FLIP_DWORD(dstpixel);
            dstpixel++;
            srcpixel+=3;
        }
        srcbits = (const char*)srcbits + srclinebytes;
        dstbits = (char*)dstbits + dstlinebytes;
    }
}

static void convert_bgr888_to_any0888_dst_byteswap(int width, int height,
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
            *dstpixel  =(srcpixel[0] << rLeftShift) | /* r */
                        (srcpixel[1] << gLeftShift) | /* g */
                        (srcpixel[2] << bLeftShift);  /* b */
            FLIP_DWORD(dstpixel);
            dstpixel++;
            srcpixel+=3;
        }
        srcbits = (const char*)srcbits + srclinebytes;
        dstbits = (char*)dstbits + dstlinebytes;
    }
}

/*
 * 32 bit conversions
 */

static void convert_0888_asis_dst_byteswap(int width, int height,
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


static void convert_0888_reverse_dst_byteswap(int width, int height,
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
            *dstpixel++=((srcval << 8) & 0xffffff00);
        }
        srcbits = (const char*)srcbits + srclinebytes;
        dstbits = (char*)dstbits + dstlinebytes;
    }
}

static void convert_0888_any_dst_byteswap(int width, int height,
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
            *dstpixel  =(((srcval >> rRightShift) & 0xff) << rLeftShift) |
                        (((srcval >> gRightShift) & 0xff) << gLeftShift) |
                        (((srcval >> bRightShift) & 0xff) << bLeftShift);
            FLIP_DWORD(dstpixel);
            dstpixel++;
        }
        srcbits = (const char*)srcbits + srclinebytes;
        dstbits = (char*)dstbits + dstlinebytes;
    }
}

static void convert_0888_to_555_asis_dst_byteswap(int width, int height,
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
            *dstpixel  =((srcval >> 17) & 0x007c) | /* h */
                        ((srcval >> 14) & 0x0003) | /* g - 2 bits */
                        ((srcval <<  2) & 0xe000) | /* g - 3 bits */
                        ((srcval <<  5) & 0x1f00);  /* l */
            dstpixel++;
        }
        srcbits = (const char*)srcbits + srclinebytes;
        dstbits = (char*)dstbits + dstlinebytes;
    }
}

static void convert_0888_to_555_reverse_dst_byteswap(int width, int height,
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
            *dstpixel  =((srcval >> 11) & 0x1f00) | /* h */
                        ((srcval >>  6) & 0x0003) | /* g - 2 bits */
                        ((srcval <<  2) & 0xe000) | /* g - 3 bits */
                        ((srcval >>  1) & 0x7c00);  /* l */
            dstpixel++;
        }
        srcbits = (const char*)srcbits + srclinebytes;
        dstbits = (char*)dstbits + dstlinebytes;
    }
}

static void convert_0888_to_565_asis_dst_byteswap(int width, int height,
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
            *dstpixel++=((srcval >> 16) & 0x00f8) | /* h  */
                        ((srcval >> 13) & 0x0007) | /* g - 3 bits */
                        ((srcval <<  3) & 0xe000) | /* g - 3 bits */
                        ((srcval <<  5) & 0x1f00);  /* l */
        }
        srcbits = (const char*)srcbits + srclinebytes;
        dstbits = (char*)dstbits + dstlinebytes;
    }
}

static void convert_0888_to_565_reverse_dst_byteswap(int width, int height,
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
            *dstpixel++=((srcval >> 11) & 0x1f00) | /* h */
                        ((srcval >> 13) & 0x0007) | /* g - 3 bits */
                        ((srcval <<  3) & 0xe000) | /* g - 3 bits */
                        ((srcval <<  0) & 0x00f8);  /* l */
        }
        srcbits = (const char*)srcbits + srclinebytes;
        dstbits = (char*)dstbits + dstlinebytes;
    }
}

static void convert_any0888_to_5x5_dst_byteswap(int width, int height,
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
            *dstpixel  =(((srcval >> rRightShift) & rdst) << rLeftShift) |
                        (((srcval >> gRightShift) & gdst) << gLeftShift) |
                        (((srcval >> bRightShift) & bdst) << bLeftShift);
            FLIP_WORD(dstpixel);
            dstpixel++;
        }
        srcbits = (const char*)srcbits + srclinebytes;
        dstbits = (char*)dstbits + dstlinebytes;
    }
}

static void convert_0888_to_888_asis_dst_byteswap(int width, int height,
                                                  const void* srcbits, int srclinebytes,
                                                  void* dstbits, int dstlinebytes)
{
    const DWORD* srcpixel;
    DWORD* dstpixel;
    int x,y;

    width=width/4;
    for (y=0; y<height; y++) {
        srcpixel=srcbits;
        dstpixel=dstbits;
        for (x=0; x<width; x++) {
            /* Do 4 pixels at a time: 4 dwords in and 3 dwords out */
            DWORD srcval1, srcval2;
            srcval1=*srcpixel++;
            srcval2=*srcpixel++;
            *dstpixel++=((srcval1 << 24) & 0xff000000) | /* l1 */
                        ((srcval1 <<  8) & 0x00ff0000) | /* g1 */
                        ((srcval1 >>  8) & 0x0000ff00) | /* h1 */
                        ((srcval2 >>  0) & 0x000000ff);  /* l2 */
            srcval1=*srcpixel++;
            *dstpixel++=((srcval2 << 16) & 0xff000000) | /* g2 */
                        ((srcval2 <<  0) & 0x00ff0000) | /* h2 */
                        ((srcval1 <<  8) & 0x0000ff00) | /* l3 */
                        ((srcval1 >>  8) & 0x000000ff);  /* g3 */
            srcval2=*srcpixel++;
            *dstpixel++=((srcval1 <<  8) & 0xff000000) | /* h3 */
                        ((srcval2 << 16) & 0x00ff0000) | /* l4 */
                        ((srcval2 <<  0) & 0x0000ff00) | /* g4 */
                        ((srcval2 >> 16) & 0x000000ff);  /* h4 */
        }
        /* And now up to 3 odd pixels */
        if(width&3) {
            BYTE *dstbyte = (BYTE*)dstpixel;
            const BYTE *srcbyte = (const BYTE*)srcpixel;
            for(x = 0; x < (width&3); x++) {
                dstbyte[0] = srcbyte[0];
                dstbyte[1] = srcbyte[1];
                dstbyte[2] = srcbyte[2];
                dstbyte+=3;
                srcbyte+=4;
                if(x > 0)
                    FLIP_DWORD(dstpixel + x - 1);
            }
            FLIP_DWORD(dstpixel + x - 1);
        }
        srcbits = (const char*)srcbits + srclinebytes;
        dstbits = (char*)dstbits + dstlinebytes;
    }
}

static void convert_0888_to_888_reverse_dst_byteswap(int width, int height,
                                                     const void* srcbits, int srclinebytes,
                                                     void* dstbits, int dstlinebytes)
{
    const DWORD* srcpixel;
    DWORD* dstpixel;
    int x,y;

    width=width/4;
    for (y=0; y<height; y++) {
        srcpixel=srcbits;
        dstpixel=dstbits;
        for (x=0; x<width; x++) {
            /* Do 4 pixels at a time: 4 dwords in and 3 dwords out */
            DWORD srcval1,srcval2;
            srcval1=*srcpixel++;
            srcval2=*srcpixel++;
            *dstpixel++=((srcval1 <<  8) & 0xffffff00) | /* h1, g1, l1 */
                        ((srcval2 >> 16) & 0x000000ff);  /* h2 */
            srcval1=*srcpixel++;
            *dstpixel++=((srcval2 << 16) & 0xffff0000) | /* g2, l2 */
                        ((srcval1 >>  8) & 0x0000ffff);  /* h3, g3 */
            srcval2=*srcpixel++;
            *dstpixel++=((srcval1 << 24) & 0xff000000) | /* l3 */
                        ((srcval2 <<  0) & 0x00ffffff);  /* h4, g4, l4 */
        }
        /* And now up to 3 odd pixels */
        if(width&3) {
            BYTE *dstbyte = (BYTE*)dstpixel;
            const BYTE *srcbyte = (const BYTE*)srcpixel;
            for(x = 0; x < (width&3); x++) {
                dstbyte[2] = srcbyte[0];
                dstbyte[1] = srcbyte[1];
                dstbyte[0] = srcbyte[2];
                dstbyte+=3;
                srcbyte+=4;
                if(x > 0)
                    FLIP_DWORD(dstpixel + x - 1);
            }
            FLIP_DWORD(dstpixel + x - 1);
        }
        srcbits = (const char*)srcbits + srclinebytes;
        dstbits = (char*)dstbits + dstlinebytes;
    }
}

static void convert_any0888_to_rgb888_dst_byteswap(int width, int height,
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
            if(x&3)
                FLIP_DWORD((DWORD*)(dstpixel + x - 4));
            dstpixel+=3;
        }
        if(x&3)
            FLIP_DWORD((DWORD*)(dstpixel + x - 4));

        srcbits = (const char*)srcbits + srclinebytes;
        dstbits = (char*)dstbits + dstlinebytes;
    }
}

static void convert_any0888_to_bgr888_dst_byteswap(int width, int height,
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
            if(x&3)
                FLIP_DWORD((DWORD*)(dstpixel + x - 4));
            dstpixel+=3;
        }
        if(x&3)
            FLIP_DWORD((DWORD*)(dstpixel + x - 4));
        srcbits = (const char*)srcbits + srclinebytes;
        dstbits = (char*)dstbits + dstlinebytes;
    }
}

const dib_conversions dib_dst_byteswap = {
    convert_5x5_asis_dst_byteswap,
    convert_555_reverse_dst_byteswap,
    convert_555_to_565_asis_dst_byteswap,
    convert_555_to_565_reverse_dst_byteswap,
    convert_555_to_888_asis_dst_byteswap,
    convert_555_to_888_reverse_dst_byteswap,
    convert_555_to_0888_asis_dst_byteswap,
    convert_555_to_0888_reverse_dst_byteswap,
    convert_5x5_to_any0888_dst_byteswap,
    convert_565_reverse_dst_byteswap,
    convert_565_to_555_asis_dst_byteswap,
    convert_565_to_555_reverse_dst_byteswap,
    convert_565_to_888_asis_dst_byteswap,
    convert_565_to_888_reverse_dst_byteswap,
    convert_565_to_0888_asis_dst_byteswap,
    convert_565_to_0888_reverse_dst_byteswap,
    convert_888_asis_dst_byteswap,
    convert_888_reverse_dst_byteswap,
    convert_888_to_555_asis_dst_byteswap,
    convert_888_to_555_reverse_dst_byteswap,
    convert_888_to_565_asis_dst_byteswap,
    convert_888_to_565_reverse_dst_byteswap,
    convert_888_to_0888_asis_dst_byteswap,
    convert_888_to_0888_reverse_dst_byteswap,
    convert_rgb888_to_any0888_dst_byteswap,
    convert_bgr888_to_any0888_dst_byteswap,
    convert_0888_asis_dst_byteswap,
    convert_0888_reverse_dst_byteswap,
    convert_0888_any_dst_byteswap,
    convert_0888_to_555_asis_dst_byteswap,
    convert_0888_to_555_reverse_dst_byteswap,
    convert_0888_to_565_asis_dst_byteswap,
    convert_0888_to_565_reverse_dst_byteswap,
    convert_any0888_to_5x5_dst_byteswap,
    convert_0888_to_888_asis_dst_byteswap,
    convert_0888_to_888_reverse_dst_byteswap,
    convert_any0888_to_rgb888_dst_byteswap,
    convert_any0888_to_bgr888_dst_byteswap
};
