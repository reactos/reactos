/* $XFree86: xc/lib/GL/mesa/src/drv/ffb/ffb_stencil.c,v 1.2 2002/02/22 21:32:59 dawes Exp $
 *
 * GLX Hardware Device Driver for Sun Creator/Creator3D
 * Copyright (C) 2000 David S. Miller
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

#include "mtypes.h"
#include "ffb_dd.h"
#include "ffb_span.h"
#include "ffb_context.h"
#include "ffb_stencil.h"
#include "ffb_lock.h"

#include "swrast/swrast.h"

#undef STENCIL_TRACE

static void FFBWriteStencilSpan( GLcontext *ctx,
                                   struct gl_renderbuffer *rb,
				   GLuint n, GLint x, GLint y,
				   const void *values, const GLubyte mask[] )
{
        const GLubyte *stencil = (const GLubyte *) values;
#ifdef STENCIL_TRACE
	fprintf(stderr, "FFBWriteStencilSpan: n(%d) x(%d) y(%d)\n",
		(int) n, x, y);
#endif
	if (ctx->Depth.Mask) {
		ffbContextPtr fmesa = FFB_CONTEXT(ctx);
		__DRIdrawablePrivate *dPriv = fmesa->driDrawable;
		GLuint *zptr;
		GLuint i;

		if (!fmesa->hw_locked)
			LOCK_HARDWARE(fmesa);
		FFBFifo(fmesa, 2);
		fmesa->regs->fbc = (FFB_FBC_WB_C | FFB_FBC_ZE_OFF |
				    FFB_FBC_YE_ON | FFB_FBC_RGBE_OFF);
		fmesa->regs->ppc = FFB_PPC_YS_VAR;
		FFBWait(fmesa, fmesa->regs);

		y = (dPriv->h - y);
		zptr = (GLuint *)
			((char *)fmesa->sfb32 +
			 ((dPriv->x + x) << 2) +
			 ((dPriv->y + y) << 13));
		
		for (i = 0; i < n; i++) {
			if (mask[i])
				*zptr = (stencil[i] & 0xf) << 28;
			zptr++;
		}

		FFBFifo(fmesa, 2);
		fmesa->regs->fbc = fmesa->fbc;
		fmesa->regs->ppc = fmesa->ppc;
		fmesa->ffbScreen->rp_active = 1;
		if (!fmesa->hw_locked)
			UNLOCK_HARDWARE(fmesa);
	}
}

static void FFBWriteStencilPixels( GLcontext *ctx,
                                     struct gl_renderbuffer *rb,
				     GLuint n,
				     const GLint x[], const GLint y[],
				     const void *values, const GLubyte mask[] )
{
        const GLubyte *stencil = (const GLubyte *) values;
#ifdef STENCIL_TRACE
	fprintf(stderr, "FFBWriteStencilPixels: n(%d)\n", (int) n);
#endif
	if (ctx->Depth.Mask) {
		ffbContextPtr fmesa = FFB_CONTEXT(ctx);
		__DRIdrawablePrivate *dPriv = fmesa->driDrawable;
		char *zbase;
		GLuint i;

		if (!fmesa->hw_locked)
			LOCK_HARDWARE(fmesa);
		FFBFifo(fmesa, 2);
		fmesa->regs->fbc = (FFB_FBC_WB_C | FFB_FBC_ZE_OFF |
				    FFB_FBC_YE_ON | FFB_FBC_RGBE_OFF);
		fmesa->regs->ppc = FFB_PPC_YS_VAR;
		fmesa->ffbScreen->rp_active = 1;
		FFBWait(fmesa, fmesa->regs);

		zbase = ((char *)fmesa->sfb32 +
			 (dPriv->x << 2) + (dPriv->y << 13));
		
		for (i = 0; i < n; i++) {
			GLint y1 = (dPriv->h - y[i]);
			GLint x1 = x[i];
			GLuint *zptr;

			zptr = (GLuint *)
				(zbase + (x1 << 2) + (y1 << 13));
			if (mask[i])
				*zptr = (stencil[i] & 0xf) << 28;
		}

		FFBFifo(fmesa, 2);
		fmesa->regs->fbc = fmesa->fbc;
		fmesa->regs->ppc = fmesa->ppc;
		fmesa->ffbScreen->rp_active = 1;
		if (!fmesa->hw_locked)
			UNLOCK_HARDWARE(fmesa);
	}
}

static void FFBReadStencilSpan( GLcontext *ctx,
                                  struct gl_renderbuffer *rb,
				  GLuint n, GLint x, GLint y,
				  void *values)
{
        GLubyte *stencil = (GLubyte *) values;
	ffbContextPtr fmesa = FFB_CONTEXT(ctx);
	__DRIdrawablePrivate *dPriv = fmesa->driDrawable;
	GLuint *zptr;
	GLuint i;

#ifdef STENCIL_TRACE
	fprintf(stderr, "FFBReadStencilSpan: n(%d) x(%d) y(%d)\n",
		(int) n, x, y);
#endif
	if (!fmesa->hw_locked)
		LOCK_HARDWARE(fmesa);
	FFBFifo(fmesa, 1);
	fmesa->regs->fbc = FFB_FBC_RB_C;
	fmesa->ffbScreen->rp_active = 1;
	FFBWait(fmesa, fmesa->regs);

	y = (dPriv->h - y);
	zptr = (GLuint *)
		((char *)fmesa->sfb32 +
		 ((dPriv->x + x) << 2) +
		 ((dPriv->y + y) << 13));
		
	for (i = 0; i < n; i++) {
		stencil[i] = (*zptr >> 28) & 0xf;
		zptr++;
	}

	FFBFifo(fmesa, 1);
	fmesa->regs->fbc = fmesa->fbc;
	fmesa->ffbScreen->rp_active = 1;
	if (!fmesa->hw_locked)
		UNLOCK_HARDWARE(fmesa);
}

static void FFBReadStencilPixels( GLcontext *ctx,
                                    struct gl_renderbuffer *rb,
                                    GLuint n, const GLint x[], const GLint y[],
				    void *values )
{
        GLubyte *stencil = (GLubyte *) values;
	ffbContextPtr fmesa = FFB_CONTEXT(ctx);
	__DRIdrawablePrivate *dPriv = fmesa->driDrawable;
	char *zbase;
	GLuint i;

#ifdef STENCIL_TRACE
	fprintf(stderr, "FFBReadStencilPixels: n(%d)\n", (int) n);
#endif
	if (!fmesa->hw_locked)
		LOCK_HARDWARE(fmesa);
	FFBFifo(fmesa, 1);
	fmesa->regs->fbc = FFB_FBC_RB_C;
	fmesa->ffbScreen->rp_active = 1;
	FFBWait(fmesa, fmesa->regs);

	zbase = ((char *)fmesa->sfb32 +
		 (dPriv->x << 2) + (dPriv->y << 13));
		
	for (i = 0; i < n; i++) {
		GLint y1 = (dPriv->h - y[i]);
		GLint x1 = x[i];
		GLuint *zptr;

		zptr = (GLuint *)
			(zbase + (x1 << 2) + (y1 << 13));
		stencil[i] = (*zptr >> 28) & 0xf;
	}

	FFBFifo(fmesa, 1);
	fmesa->regs->fbc = fmesa->fbc;
	fmesa->ffbScreen->rp_active = 1;
	if (!fmesa->hw_locked)
		UNLOCK_HARDWARE(fmesa);
}

/**
 * Plug in the Get/Put routines for the given driRenderbuffer.
 */
void
ffbSetStencilFunctions(driRenderbuffer *drb, const GLvisual *vis)
{
	assert(drb->Base.InternalFormat == GL_STENCIL_INDEX8_EXT);
      	drb->Base.GetRow        = FFBReadStencilSpan;
      	drb->Base.GetValues     = FFBReadStencilPixels;
      	drb->Base.PutRow        = FFBWriteStencilSpan;
      	/*drb->Base.PutMonoRow    = FFBWriteMonoStencilSpan;*/
      	drb->Base.PutValues     = FFBWriteStencilPixels;
      	drb->Base.PutMonoValues = NULL;
}
