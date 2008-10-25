/* $XFree86: xc/lib/GL/mesa/src/drv/ffb/ffb_span.c,v 1.2 2002/02/22 21:32:59 dawes Exp $
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
#include "ffb_lock.h"

#include "swrast/swrast.h"

#define DBG 0

#define HW_LOCK()						\
	ffbContextPtr fmesa = FFB_CONTEXT(ctx);			\
	if (!fmesa->hw_locked)					\
		LOCK_HARDWARE(fmesa);

#define HW_UNLOCK()			\
	if (!fmesa->hw_locked)		\
		UNLOCK_HARDWARE(fmesa); \

#define LOCAL_VARS						\
	__DRIdrawablePrivate *dPriv = fmesa->driDrawable;	\
	GLuint height = dPriv->h;				\
        GLuint p;						\
	char *buf; 						\
        (void) p

#define INIT_MONO_PIXEL(p, color)		\
        p = ((color[0] <<  0) |			\
	     (color[1] << 8) |			\
	     (color[2] << 16))

/* We use WID clipping, so this test always passes. */
#define CLIPPIXEL(__x, __y)	(1)

/* And also, due to WID clipping, we need not do anything
 * special here.
 */
#define CLIPSPAN(__x,__y,__n,__x1,__n1,__i)		\
	__n1 = __n;					\
	__x1 = __x;					\

#define HW_CLIPLOOP()							\
do {	unsigned int fbc, ppc, cmp;					\
	FFBWait(fmesa, fmesa->regs);					\
	fbc = fmesa->regs->fbc; ppc = fmesa->regs->ppc; cmp = fmesa->regs->cmp; \
	fmesa->regs->fbc = ((fbc &					\
			     ~(FFB_FBC_WB_C | FFB_FBC_ZE_MASK | FFB_FBC_RGBE_MASK)) \
			    | (FFB_FBC_ZE_OFF | FFB_FBC_RGBE_MASK));	\
	fmesa->regs->ppc = ((ppc &					\
			     ~(FFB_PPC_XS_MASK | FFB_PPC_ABE_MASK | FFB_PPC_DCE_MASK | \
			       FFB_PPC_APE_MASK | FFB_PPC_CS_MASK))	\
			    | (FFB_PPC_XS_WID | FFB_PPC_ABE_DISABLE |	\
			       FFB_PPC_DCE_DISABLE | FFB_PPC_APE_DISABLE | \
			       FFB_PPC_CS_VAR));			\
	fmesa->regs->cmp = ((cmp & ~(0xff << 16)) | (0x80 << 16));	\
	fmesa->ffbScreen->rp_active = 1;				\
	FFBWait(fmesa, fmesa->regs);					\
	buf = (char *)(fmesa->sfb32 + (dPriv->x << 2) + (dPriv->y << 13));\
	if (dPriv->numClipRects) {

#define HW_ENDCLIPLOOP()	\
	}			\
	fmesa->regs->fbc = fbc;	\
	fmesa->regs->ppc = ppc;	\
	fmesa->regs->cmp = cmp;	\
	fmesa->ffbScreen->rp_active = 1; \
} while(0)

#define Y_FLIP(__y)		(height - __y - 1)

#define READ_RGBA(rgba,__x,__y)					\
do {	GLuint p = *(GLuint *)(buf + ((__x)<<2) + ((__y)<<13));	\
	rgba[0] = (p >>  0) & 0xff;				\
	rgba[1] = (p >>  8) & 0xff;				\
	rgba[2] = (p >> 16) & 0xff;				\
	rgba[3] = 0xff;						\
} while(0)

#define WRITE_RGBA(__x, __y, __r, __g, __b, __a)		\
	*(GLuint *)(buf + ((__x)<<2) + ((__y)<<13)) =		\
		((((__r) & 0xff) <<  0) |			\
		 (((__g) & 0xff) <<  8) |			\
		 (((__b) & 0xff) << 16))

#define WRITE_PIXEL(__x, __y, __p) \
	*(GLuint *)(buf + ((__x)<<2) + ((__y)<<13)) = (__p)

#define TAG(x) ffb##x##_888

#include "spantmp.h"

/**
 * Plug in the Get/Put routines for the given driRenderbuffer.
 */
void
ffbSetSpanFunctions(driRenderbuffer *drb, const GLvisual *vis)
{
	assert(vis->redBits == 8);
        assert(vis->greenBits == 8);
        assert(vis->blueBits == 8);
        ffbInitPointers_888(&drb->Base);
}
