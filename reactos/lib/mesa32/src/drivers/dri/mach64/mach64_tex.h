/* $XFree86$ */ /* -*- mode: c; c-basic-offset: 3 -*- */
/*
 * Copyright 2000 Gareth Hughes
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * GARETH HUGHES BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/*
 * Authors:
 *	Gareth Hughes <gareth@valinux.com>
 *	Leif Delgass <ldelgass@retinalburn.net>
 *	José Fonseca <j_r_fonseca@yahoo.co.uk>
 */

#ifndef __MACH64_TEX_H__
#define __MACH64_TEX_H__

extern void mach64UpdateTextureState( GLcontext *ctx );

extern void mach64SwapOutTexObj( mach64ContextPtr mach64ctx,
				 mach64TexObjPtr t );

extern void mach64UploadTexImages( mach64ContextPtr mach64ctx,
				   mach64TexObjPtr t );

extern void mach64UploadMultiTexImages( mach64ContextPtr mach64ctx,
					mach64TexObjPtr t0, mach64TexObjPtr t1 );

extern void mach64AgeTextures( mach64ContextPtr mach64ctx, int heap );
extern void mach64DestroyTexObj( mach64ContextPtr mach64ctx,
				 mach64TexObjPtr t );

extern void mach64UpdateTexLRU( mach64ContextPtr mach64ctx,
				mach64TexObjPtr t );

extern void mach64PrintLocalLRU( mach64ContextPtr mach64ctx, int heap );
extern void mach64PrintGlobalLRU( mach64ContextPtr mach64ctx, int heap );

extern void mach64EmitTexStateLocked( mach64ContextPtr mmesa,
				      mach64TexObjPtr t0,
				      mach64TexObjPtr t1 );

extern void mach64InitTextureFuncs( struct dd_function_table *functions );

/* ================================================================
 * Color conversion macros:
 */

#define MACH64PACKCOLOR332(r, g, b)					\
   (((r) & 0xe0) | (((g) & 0xe0) >> 3) | (((b) & 0xc0) >> 6))

#define MACH64PACKCOLOR1555(r, g, b, a)					\
   ((((r) & 0xf8) << 7) | (((g) & 0xf8) << 2) | (((b) & 0xf8) >> 3) |	\
    ((a) ? 0x8000 : 0))

#define MACH64PACKCOLOR565(r, g, b)					\
   ((((r) & 0xf8) << 8) | (((g) & 0xfc) << 3) | (((b) & 0xf8) >> 3))

#define MACH64PACKCOLOR888(r, g, b)					\
   (((r) << 16) | ((g) << 8) | (b))

#define MACH64PACKCOLOR8888(r, g, b, a)					\
   (((a) << 24) | ((r) << 16) | ((g) << 8) | (b))

#define MACH64PACKCOLOR4444(r, g, b, a)					\
   ((((a) & 0xf0) << 8) | (((r) & 0xf0) << 4) | ((g) & 0xf0) | ((b) >> 4))

static __inline__ GLuint mach64PackColor( GLuint cpp,
					  GLubyte r, GLubyte g,
					  GLubyte b, GLubyte a )
{
   switch ( cpp ) {
   case 2:
      return MACH64PACKCOLOR565( r, g, b );
   case 4:
      return MACH64PACKCOLOR8888( r, g, b, a );
   default:
      return 0;
   }
}

#endif
