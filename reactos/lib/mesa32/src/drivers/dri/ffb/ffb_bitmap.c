/* $XFree86: xc/lib/GL/mesa/src/drv/ffb/ffb_bitmap.c,v 1.1 2002/02/22 21:32:58 dawes Exp $
 *
 * GLX Hardware Device Driver for Sun Creator/Creator3D
 * Copyright (C) 2001 David S. Miller
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
 * DAVID MILLER, OR ANY OTHER CONTRIBUTORS BE LIABLE FOR ANY CLAIM, 
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR 
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE 
 * OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 *
 *    David S. Miller <davem@redhat.com>
 */

#include "ffb_context.h"
#include "ffb_state.h"
#include "ffb_lock.h"
#include "ffb_bitmap.h"
#include "swrast/swrast.h"
#include "image.h"
#include "macros.h"

/* Compute ceiling of integer quotient of A divided by B: */
#define CEILING( A, B )  ( (A) % (B) == 0 ? (A)/(B) : (A)/(B)+1 )

#undef FFB_BITMAP_TRACE

static void
ffb_bitmap(GLcontext *ctx, GLint px, GLint py,
	   GLsizei width, GLsizei height,
	   const struct gl_pixelstore_attrib *unpack,
	   const GLubyte *bitmap)
{
	ffbContextPtr fmesa = FFB_CONTEXT(ctx);
	ffb_fbcPtr ffb = fmesa->regs;
	__DRIdrawablePrivate *dPriv = fmesa->driDrawable;
	unsigned int ppc, pixel;
	GLint row, col, row_stride;
	const GLubyte *src;
	char *buf;

	if (fmesa->bad_fragment_attrs != 0)
		_swrast_Bitmap(ctx, px, py, width,
			       height, unpack, bitmap);

	pixel = (((((GLuint)(ctx->Current.RasterColor[0] * 255.0f)) & 0xff) <<  0) |
		 ((((GLuint)(ctx->Current.RasterColor[1] * 255.0f)) & 0xff) <<  8) |
		 ((((GLuint)(ctx->Current.RasterColor[2] * 255.0f)) & 0xff) << 16) |
		 ((((GLuint)(ctx->Current.RasterColor[3] * 255.0f)) & 0xff) << 24));

#ifdef FFB_BITMAP_TRACE
	fprintf(stderr, "ffb_bitmap: ppc(%08x) fbc(%08x) cmp(%08x) pixel(%08x)\n",
		fmesa->ppc, fmesa->fbc, fmesa->cmp, pixel);
#endif

	LOCK_HARDWARE(fmesa);
	fmesa->hw_locked = 1;

	if (fmesa->state_dirty)
		ffbSyncHardware(fmesa);

	ppc = fmesa->ppc;

	FFBFifo(fmesa, 4);
	ffb->ppc = ((ppc &
		     ~(FFB_PPC_TBE_MASK | FFB_PPC_ZS_MASK | FFB_PPC_CS_MASK | FFB_PPC_XS_MASK))
		    | (FFB_PPC_TBE_TRANSPARENT | FFB_PPC_ZS_CONST | FFB_PPC_CS_CONST |
		       (ctx->Color.BlendEnabled ? FFB_PPC_XS_CONST : FFB_PPC_XS_WID)));
	ffb->constz = ((GLuint) (ctx->Current.RasterPos[2] * 0x0fffffff));
	ffb->fg = pixel;
	ffb->fontinc = (0 << 16) | 32;

	buf = (char *)(fmesa->sfb32 + (dPriv->x << 2) + (dPriv->y << 13));

	row_stride = (unpack->Alignment * CEILING(width, 8 * unpack->Alignment));
	src = (const GLubyte *) (bitmap +
				 (unpack->SkipRows * row_stride) +
				 (unpack->SkipPixels / 8));
	if (unpack->LsbFirst == GL_TRUE) {
		for (row = 0; row < height; row++, src += row_stride) {
			const GLubyte *row_src = src;
			GLuint base_x, base_y;

			base_x = dPriv->x + px;
			base_y = dPriv->y + (dPriv->h - (py + row));

			FFBFifo(fmesa, 1);
			ffb->fontxy = (base_y << 16) | base_x;

			for (col = 0; col < width; col += 32, row_src += 4) {
				GLint bitnum, font_w = (width - col);
				GLuint font_data;

				if (font_w > 32)
					font_w = 32;
				font_data = 0;
				for (bitnum = 0; bitnum < 32; bitnum++) {
					const GLubyte val = row_src[bitnum >> 3];

					if (val & (1 << (bitnum & (8 - 1))))
						font_data |= (1 << (31 - bitnum));
				}

				FFBFifo(fmesa, 2);
				ffb->fontw = font_w;
				ffb->font = font_data;
			}
		}
	} else {
		for (row = 0; row < height; row++, src += row_stride) {
			const GLubyte *row_src = src;
			GLuint base_x, base_y;

			base_x = dPriv->x + px;
			base_y = dPriv->y + (dPriv->h - (py + row));

			FFBFifo(fmesa, 1);
			ffb->fontxy = (base_y << 16) | base_x;

			for (col = 0; col < width; col += 32, row_src += 4) {
				GLint font_w = (width - col);

				if (font_w > 32)
					font_w = 32;
				FFBFifo(fmesa, 2);
				ffb->fontw = font_w;
				ffb->font = (((unsigned int)row_src[0]) << 24 |
					     ((unsigned int)row_src[1]) << 16 |
					     ((unsigned int)row_src[2]) <<  8 |
					     ((unsigned int)row_src[3]) <<  0);
			}
		}
	}

	FFBFifo(fmesa, 1);
	ffb->ppc = ppc;
	fmesa->ffbScreen->rp_active = 1;

	UNLOCK_HARDWARE(fmesa);
	fmesa->hw_locked = 0;
}

void ffbDDInitBitmapFuncs(GLcontext *ctx)
{
	ctx->Driver.Bitmap = ffb_bitmap;
}
