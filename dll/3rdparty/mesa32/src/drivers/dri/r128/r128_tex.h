/* $XFree86: xc/lib/GL/mesa/src/drv/r128/r128_tex.h,v 1.7 2002/02/22 21:44:58 dawes Exp $ */
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
 *   Gareth Hughes <gareth@valinux.com>
 *   Kevin E. Martin <martin@valinux.com>
 *
 */

#ifndef __R128_TEX_H__
#define __R128_TEX_H__

extern void r128UpdateTextureState( GLcontext *ctx );

extern void r128UploadTexImages( r128ContextPtr rmesa, r128TexObjPtr t );

extern void r128DestroyTexObj( r128ContextPtr rmesa, r128TexObjPtr t );

extern void r128InitTextureFuncs( struct dd_function_table *functions );


/* ================================================================
 * Color conversion macros:
 */

#define R128PACKCOLOR332( r, g, b )					\
   (((r) & 0xe0) | (((g) & 0xe0) >> 3) | (((b) & 0xc0) >> 6))

#define R128PACKCOLOR1555( r, g, b, a )					\
   ((((r) & 0xf8) << 7) | (((g) & 0xf8) << 2) | (((b) & 0xf8) >> 3) |	\
    ((a) ? 0x8000 : 0))

#define R128PACKCOLOR565( r, g, b )					\
   ((((r) & 0xf8) << 8) | (((g) & 0xfc) << 3) | (((b) & 0xf8) >> 3))

#define R128PACKCOLOR888( r, g, b )					\
   (((r) << 16) | ((g) << 8) | (b))

#define R128PACKCOLOR8888( r, g, b, a )					\
   (((a) << 24) | ((r) << 16) | ((g) << 8) | (b))

#define R128PACKCOLOR4444( r, g, b, a )					\
   ((((a) & 0xf0) << 8) | (((r) & 0xf0) << 4) | ((g) & 0xf0) | ((b) >> 4))

static __inline__ u_int32_t r128PackColor( GLuint cpp,
					GLubyte r, GLubyte g,
					GLubyte b, GLubyte a )
{
    switch ( cpp ) {
    case 2:
       return R128PACKCOLOR565( r, g, b );
    case 4:
       return R128PACKCOLOR8888( r, g, b, a );
    default:
       return 0;
    }
}

#endif /* __R128_TEX_H__ */
