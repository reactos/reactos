/* $XFree86: xc/lib/GL/mesa/src/drv/ffb/ffb_clear.c,v 1.2 2002/02/22 21:32:58 dawes Exp $
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
#include "extensions.h"

#include "mm.h"
#include "ffb_dd.h"
#include "ffb_span.h"
#include "ffb_depth.h"
#include "ffb_context.h"
#include "ffb_vb.h"
#include "ffb_tris.h"
#include "ffb_clear.h"
#include "ffb_lock.h"

#undef CLEAR_TRACE

#define BOX_AREA(__w, __h)	((int)(__w) * (int)(__h))

/* Compute the page aligned box for a page mode fast fill.
 * In 'ework' this returns greater than zero if there are some odd
 * edges to take care of which are outside of the page aligned area.
 * It will place less than zero there if the box is too small,
 * indicating that a different method must be used to fill it.
 */
#define CreatorPageFillParms(ffp, x, y, w, h, px, py, pw, ph, ework) \
do {	int xdiff, ydiff; \
	int pf_bh = ffp->pagefill_height; \
	int pf_bw = ffp->pagefill_width; \
	py = ((y + (pf_bh - 1)) & ~(pf_bh - 1)); \
	ydiff = py - y; \
	px = ffp->Pf_AlignTab[x + (pf_bw - 1)]; \
	xdiff = px - x; \
	ph = ((h - ydiff) & ~(pf_bh - 1)); \
	if(ph <= 0) \
		ework = -1; \
	else { \
		pw = ffp->Pf_AlignTab[w - xdiff]; \
		if(pw <= 0) { \
			ework = -1; \
		} else { \
			ework = (((xdiff > 0)		|| \
				  (ydiff > 0)		|| \
				  ((w - pw) > 0)	|| \
				  ((h - ph) > 0))) ? 1 : 0; \
		} \
	} \
} while(0);

struct ff_fixups {
	int x, y, width, height;
};

/* Compute fixups of non-page aligned areas after a page fill.
 * Return the number of fixups needed.
 */
static __inline__ int
CreatorComputePageFillFixups(struct ff_fixups *fixups,
			     int x, int y, int w, int h,
			     int paligned_x, int paligned_y,
			     int paligned_w, int paligned_h)
{
	int nfixups = 0;

	/* FastFill Left */
	if(paligned_x != x) {
		fixups[nfixups].x = x;
		fixups[nfixups].y = paligned_y;
		fixups[nfixups].width = paligned_x - x;
		fixups[nfixups].height = paligned_h;
		nfixups++;
	}
	/* FastFill Top */
	if(paligned_y != y) {
		fixups[nfixups].x = x;
		fixups[nfixups].y = y;
		fixups[nfixups].width = w;
		fixups[nfixups].height = paligned_y - y;
		nfixups++;
	}
	/* FastFill Right */
	if((x+w) != (paligned_x+paligned_w)) {
		fixups[nfixups].x = (paligned_x+paligned_w);
		fixups[nfixups].y = paligned_y;
		fixups[nfixups].width = (x+w) - fixups[nfixups].x;
		fixups[nfixups].height = paligned_h;
		nfixups++;
	}
	/* FastFill Bottom */
	if((y+h) != (paligned_y+paligned_h)) {
		fixups[nfixups].x = x;
		fixups[nfixups].y = (paligned_y+paligned_h);
		fixups[nfixups].width = w;
		fixups[nfixups].height = (y+h) - fixups[nfixups].y;
		nfixups++;
	}
	return nfixups;
}

static void
ffb_do_clear(GLcontext *ctx, __DRIdrawablePrivate *dPriv)
{
	ffbContextPtr fmesa = FFB_CONTEXT(ctx);
	FFBDRIPtr gDRIPriv = (FFBDRIPtr) fmesa->driScreen->pDevPriv;
	ffb_fbcPtr ffb = fmesa->regs;
	drm_clip_rect_t *box = dPriv->pClipRects;
	int nc = dPriv->numClipRects;
	GLint cx, cy, cw, ch;

	/* compute region after locking: */
	cx = ctx->DrawBuffer->_Xmin;
	cy = ctx->DrawBuffer->_Ymin;
	cw = ctx->DrawBuffer->_Xmax - cx;
	ch = ctx->DrawBuffer->_Ymax - cy;

	cy  = dPriv->h - cy - ch;
	cx += dPriv->x;
	cy += dPriv->y;

	while (nc--) {
		GLint x = box[nc].x1;
		GLint y = box[nc].y1;
		GLint width = box[nc].x2 - x;
		GLint height = box[nc].y2 - y;
		int paligned_y, paligned_x;
		int paligned_h, paligned_w = 0;
		int extra_work;

		if (BOX_AREA(width, height) < gDRIPriv->fastfill_small_area) {
			FFBFifo(fmesa, 5);
			ffb->drawop = FFB_DRAWOP_RECTANGLE;
			ffb->by = y;
			ffb->bx = x;
			ffb->bh = height;
			ffb->bw = width;
			continue;
		}

		FFBFifo(fmesa, 1);
		ffb->drawop = FFB_DRAWOP_FASTFILL;

		if (gDRIPriv->disable_pagefill ||
		    (width < (gDRIPriv->pagefill_width<<1)) ||
		    (height < (gDRIPriv->pagefill_height<<1)))
			goto do_fastfill;

		CreatorPageFillParms(gDRIPriv,
				     x, y, width, height,
				     paligned_x, paligned_y,
				     paligned_w, paligned_h, extra_work);

		if (extra_work < 0 ||
		    BOX_AREA(paligned_w, paligned_h) < gDRIPriv->pagefill_small_area) {
		do_fastfill:
			FFBFifo(fmesa, 10);
			ffb->by = FFB_FASTFILL_COLOR_BLK;
			ffb->dy = 0;
			ffb->dx = 0;
			ffb->bh = gDRIPriv->fastfill_height;
			ffb->bw = (gDRIPriv->fastfill_width * 4);
			ffb->by = FFB_FASTFILL_BLOCK;
			ffb->dy = y;
			ffb->dx = x;
			ffb->bh = (height + (y & (gDRIPriv->fastfill_height - 1)));
			ffb->bx = (width + (x & (gDRIPriv->fastfill_width - 1)));
			continue;
		}

		/* Ok, page fill is possible and worth it. */
		FFBFifo(fmesa, 15);
		ffb->by = FFB_FASTFILL_COLOR_BLK;
		ffb->dy = 0;
		ffb->dx = 0;
		ffb->bh = gDRIPriv->fastfill_height;
		ffb->bw = gDRIPriv->fastfill_width * 4;
		ffb->by = FFB_FASTFILL_BLOCK_X;
		ffb->dy = 0;
		ffb->dx = 0;
		ffb->bh = gDRIPriv->pagefill_height;
		ffb->bw = gDRIPriv->pagefill_width * 4;
		ffb->by = FFB_FASTFILL_PAGE;
		ffb->dy = paligned_y;
		ffb->dx = paligned_x;
		ffb->bh = paligned_h;
		ffb->bx = paligned_w;

		if (extra_work) {
			struct ff_fixups local_fixups[4];
			int nfixups;

			nfixups = CreatorComputePageFillFixups(local_fixups,
							       x, y, width, height,
							       paligned_x, paligned_y,
							       paligned_w, paligned_h);
			FFBFifo(fmesa, 5 + (nfixups * 5));
			ffb->by = FFB_FASTFILL_COLOR_BLK;
			ffb->dy = 0;
			ffb->dx = 0;
			ffb->bh = gDRIPriv->fastfill_height;
			ffb->bw = gDRIPriv->fastfill_width * 4;

			while (--nfixups >= 0) {
				int xx, yy, ww, hh;

				xx = local_fixups[nfixups].x;
				yy = local_fixups[nfixups].y;
				ffb->dy = yy;
				ffb->dx = xx;
				ww = (local_fixups[nfixups].width +
				      (xx & (gDRIPriv->fastfill_width - 1)));
				hh = (local_fixups[nfixups].height +
				      (yy & (gDRIPriv->fastfill_height - 1)));
				if (nfixups != 0) {
					ffb->by = FFB_FASTFILL_BLOCK;
					ffb->bh = hh;
					ffb->bw = ww;
				} else {
					ffb->bh = hh;
					ffb->by = FFB_FASTFILL_BLOCK;
					ffb->bx = ww;
				}
			}
		}
	}
}

void ffbDDClear(GLcontext *ctx, GLbitfield mask)
{
	ffbContextPtr fmesa = FFB_CONTEXT(ctx);
	__DRIdrawablePrivate *dPriv = fmesa->driDrawable;
	unsigned int stcmask = BUFFER_BIT_STENCIL;

#ifdef CLEAR_TRACE
	fprintf(stderr, "ffbDDClear: mask(%08x) \n", mask);
#endif
	if (!(fmesa->ffb_sarea->flags & FFB_DRI_FFB2PLUS))
		stcmask = 0;

	if (mask & (BUFFER_BIT_FRONT_LEFT | BUFFER_BIT_BACK_LEFT | BUFFER_BIT_DEPTH | stcmask)) {
		ffb_fbcPtr ffb = fmesa->regs;
		unsigned int fbc, ppc;

		fbc = (FFB_FBC_XE_ON);
		ppc = (FFB_PPC_ACE_DISABLE | FFB_PPC_DCE_DISABLE |
		       FFB_PPC_ABE_DISABLE | FFB_PPC_VCE_DISABLE |
		       FFB_PPC_APE_DISABLE | FFB_PPC_XS_WID |
		       FFB_PPC_ZS_CONST | FFB_PPC_CS_CONST);

		/* Y/X enables must be both on or both off. */
		if (mask & (BUFFER_BIT_DEPTH | stcmask)) {
			fbc |= (FFB_FBC_ZE_ON | FFB_FBC_YE_ON | FFB_FBC_WB_C);
		} else
			fbc |= FFB_FBC_ZE_OFF | FFB_FBC_YE_OFF;

		/* All RGB enables must be both on or both off. */
		if (mask & (BUFFER_BIT_FRONT_LEFT | BUFFER_BIT_BACK_LEFT)) {
			if (mask & BUFFER_BIT_FRONT_LEFT) {
				if (fmesa->back_buffer == 0)
					fbc |= FFB_FBC_WB_B;
				else
					fbc |= FFB_FBC_WB_A;
			}
			if (mask & BUFFER_BIT_BACK_LEFT) {
				if (fmesa->back_buffer == 0)
					fbc |= FFB_FBC_WB_A;
				else
					fbc |= FFB_FBC_WB_B;
			}
			fbc |= FFB_FBC_RGBE_ON;
		} else
			fbc |= FFB_FBC_RGBE_OFF;

		LOCK_HARDWARE(fmesa);

		if (dPriv->numClipRects) {
			FFBFifo(fmesa, 8);
			ffb->fbc = fbc;
			ffb->ppc = ppc;
			ffb->xclip = FFB_XCLIP_TEST_ALWAYS;
			ffb->cmp = 0x80808080;
			ffb->rop = FFB_ROP_NEW;

			if (mask & (BUFFER_BIT_FRONT_LEFT | BUFFER_BIT_BACK_LEFT))
				ffb->fg = fmesa->clear_pixel;
			if (mask & BUFFER_BIT_DEPTH)
				ffb->constz = fmesa->clear_depth;
			if (mask & stcmask)
				ffb->consty = fmesa->clear_stencil;

			ffb_do_clear(ctx, dPriv);

			FFBFifo(fmesa, 6);
			ffb->ppc = fmesa->ppc;
			ffb->fbc = fmesa->fbc;
			ffb->xclip = fmesa->xclip;
			ffb->cmp = fmesa->cmp;
			ffb->rop = fmesa->rop;
			ffb->drawop = fmesa->drawop;
			if (mask & stcmask)
				ffb->consty = fmesa->consty;
			fmesa->ffbScreen->rp_active = 1;
		}

		UNLOCK_HARDWARE(fmesa);

		mask &= ~(BUFFER_BIT_FRONT_LEFT | BUFFER_BIT_BACK_LEFT |
			  BUFFER_BIT_DEPTH | stcmask);
	}

	if (mask) 
		_swrast_Clear(ctx, mask);
}

