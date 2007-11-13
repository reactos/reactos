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

#include <stdlib.h>
#include <X11/Xmd.h>

#include "glxheader.h"
#include "xmesaP.h"

#ifdef XFree86Server

#ifdef ROUNDUP
#undef ROUNDUP
#endif

#define ROUNDUP(nbytes, pad) ((((nbytes) + ((pad)-1)) / (pad)) * ((pad)>>3))

XMesaImage *XMesaCreateImage(int bitsPerPixel, int width, int height, char *data)
{
    XMesaImage *image;

    image = (XMesaImage *)xalloc(sizeof(XMesaImage));

    if (image) {
	image->width = width;
	image->height = height;
	image->data = data;
	/* Always pad to 32 bits */
	image->bytes_per_line = ROUNDUP((bitsPerPixel * width), 32);
	image->bits_per_pixel = bitsPerPixel;
    }

    return image;
}

void XMesaDestroyImage(XMesaImage *image)
{
    if (image->data)
	free(image->data);
    xfree(image);
}

unsigned long XMesaGetPixel(XMesaImage *image, int x, int y)
{
    CARD8  *row = (CARD8 *)(image->data + y*image->bytes_per_line);
    CARD8  *i8;
    CARD16 *i16;
    CARD32 *i32;
    switch (image->bits_per_pixel) {
    case 8:
	i8 = (CARD8 *)row;
	return i8[x];
	break;
    case 15:
    case 16:
	i16 = (CARD16 *)row;
	return i16[x];
	break;
    case 24: /* WARNING: architecture specific code */
	i8 = (CARD8 *)row;
	return (((CARD32)i8[x*3]) |
		(((CARD32)i8[x*3+1])<<8) |
		(((CARD32)i8[x*3+2])<<16));
	break;
    case 32:
	i32 = (CARD32 *)row;
	return i32[x];
	break;
    }
    return 0;
}

#ifndef XMESA_USE_PUTPIXEL_MACRO
void XMesaPutPixel(XMesaImage *image, int x, int y, unsigned long pixel)
{
    CARD8  *row = (CARD8 *)(image->data + y*image->bytes_per_line);
    CARD8  *i8;
    CARD16 *i16;
    CARD32 *i32;
    switch (image->bits_per_pixel) {
    case 8:
	i8 = (CARD8 *)row;
	i8[x] = (CARD8)pixel;
	break;
    case 15:
    case 16:
	i16 = (CARD16 *)row;
	i16[x] = (CARD16)pixel;
	break;
    case 24: /* WARNING: architecture specific code */
	i8 = (CARD8 *)__row;
	i8[x*3]   = (CARD8)(p);
	i8[x*3+1] = (CARD8)(p>>8);
	i8[x*3+2] = (CARD8)(p>>16);
    case 32:
	i32 = (CARD32 *)row;
	i32[x] = (CARD32)pixel;
	break;
    }
}
#endif

#endif /* XFree86Server */
