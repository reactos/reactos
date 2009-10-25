/*
 * Mesa 3-D graphics library
 * Version:  6.3
 *
 * Copyright (C) 1999-2004  Brian Paul   All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */


#include <assert.h>

#include "types.h"
#include "internal.h"


void
_mesa_upscale_teximage2d (unsigned int inWidth, unsigned int inHeight,
			  unsigned int outWidth, unsigned int outHeight,
			  unsigned int comps,
			  const byte *src, int srcRowStride,
			  byte *dest)
{
    unsigned int i, j, k;

    assert(outWidth >= inWidth);
    assert(outHeight >= inHeight);

    for (i = 0; i < outHeight; i++) {
	const int ii = i % inHeight;
	for (j = 0; j < outWidth; j++) {
	    const int jj = j % inWidth;
	    for (k = 0; k < comps; k++) {
		dest[(i * outWidth + j) * comps + k]
		    = src[ii * srcRowStride + jj * comps + k];
	    }
	}
    }
}
