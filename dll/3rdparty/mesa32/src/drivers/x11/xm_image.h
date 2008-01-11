/**************************************************************************

Copyright 1998-1999 Precision Insight, Inc., Cedar Park, Texas.
All Rights Reserved.

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sub license, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice (including the
next paragraph) shall be included in all copies or substantial portions
of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
IN NO EVENT SHALL PRECISION INSIGHT AND/OR ITS SUPPLIERS BE LIABLE FOR
ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/

/*
 * Authors:
 *   Kevin E. Martin <kevin@precisioninsight.com>
 *   Brian Paul <brian@precisioninsight.com>
 */

#ifndef _XM_IMAGE_H_
#define _XM_IMAGE_H_

#define XMESA_USE_PUTPIXEL_MACRO

struct _XMesaImageRec {
    int width, height;
    char *data;
    int bytes_per_line; /* Padded to 32 bits */
    int bits_per_pixel;
};

extern XMesaImage *XMesaCreateImage(int bitsPerPixel, int width, int height,
				    char *data);
extern void XMesaDestroyImage(XMesaImage *image);
extern unsigned long XMesaGetPixel(XMesaImage *image, int x, int y);
#ifdef XMESA_USE_PUTPIXEL_MACRO
#define XMesaPutPixel(__i,__x,__y,__p) \
{ \
    CARD8  *__row = (CARD8 *)(__i->data + __y*__i->bytes_per_line); \
    CARD8  *__i8; \
    CARD16 *__i16; \
    CARD32 *__i32; \
    switch (__i->bits_per_pixel) { \
    case 8: \
	__i8 = (CARD8 *)__row; \
	__i8[__x] = (CARD8)__p; \
	break; \
    case 15: \
    case 16: \
	__i16 = (CARD16 *)__row; \
	__i16[__x] = (CARD16)__p; \
	break; \
    case 24: /* WARNING: architecture specific code */ \
	__i8 = (CARD8 *)__row; \
	__i8[__x*3]   = (CARD8)(__p); \
	__i8[__x*3+1] = (CARD8)(__p>>8); \
	__i8[__x*3+2] = (CARD8)(__p>>16); \
	break; \
    case 32: \
	__i32 = (CARD32 *)__row; \
	__i32[__x] = (CARD32)__p; \
	break; \
    } \
}
#else
extern void XMesaPutPixel(XMesaImage *image, int x, int y,
			  unsigned long pixel);
#endif

#endif /* _XM_IMAGE_H_ */
