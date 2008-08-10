/* $XFree86: xc/lib/GL/mesa/src/drv/r128/r128_texobj.h,v 1.5 2002/02/22 21:44:58 dawes Exp $ */
/**************************************************************************

Copyright 1999, 2000 ATI Technologies Inc. and Precision Insight, Inc.,
                                               Cedar Park, Texas.
All Rights Reserved.

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
on the rights to use, copy, modify, merge, publish, distribute, sub
license, and/or sell copies of the Software, and to permit persons to whom
the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice (including the next
paragraph) shall be included in all copies or substantial portions of the
Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
ATI, PRECISION INSIGHT AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/

/*
 * Authors:
 *   Kevin E. Martin <martin@valinux.com>
 *   Gareth Hughes <gareth@valinux.com>
 *
 */

#ifndef _R128_TEXOBJ_H_
#define _R128_TEXOBJ_H_

#include "mm.h"

/* Individual texture image information.
 */
typedef struct {
    GLuint offset;			/* Relative to local texture space */
    GLuint width;
    GLuint height;
} r128TexImage;

typedef struct r128_tex_obj r128TexObj, *r128TexObjPtr;

/* Texture object in locally shared texture space.
 */
struct r128_tex_obj {
   driTextureObject   base;

   u_int32_t bufAddr;			/* Offset to start of locally
					   shared texture block */

   GLuint age;
   r128TexImage image[R128_MAX_TEXTURE_LEVELS]; /* Image data for all
						   mipmap levels */

   u_int32_t textureFormat;		/* Actual hardware format */

   drm_r128_texture_regs_t setup;		/* Setup regs for texture */
};

#endif /* _R128_TEXOBJ_H_ */
