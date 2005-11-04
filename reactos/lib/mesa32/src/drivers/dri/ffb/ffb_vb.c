/* $XFree86: xc/lib/GL/mesa/src/drv/ffb/ffb_vb.c,v 1.4 2002/02/22 21:32:59 dawes Exp $
 *
 * GLX Hardware Device Driver for Sun Creator/Creator3D
 * Copyright (C) 2000, 2001 David S. Miller
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

#include "ffb_xmesa.h"
#include "ffb_context.h"
#include "ffb_vb.h"
#include "imports.h"
#include "tnl/t_context.h"
#include "swrast_setup/swrast_setup.h"
#include "math/m_translate.h"

#undef VB_DEBUG

static void ffb_copy_pv_oneside(GLcontext *ctx, GLuint edst, GLuint esrc)
{
	ffbContextPtr fmesa = FFB_CONTEXT(ctx);
	ffb_vertex *dst = &fmesa->verts[edst];
	ffb_vertex *src = &fmesa->verts[esrc];

#ifdef VB_DEBUG
	fprintf(stderr, "ffb_copy_pv_oneside: edst(%d) esrc(%d)\n", edst, esrc);
#endif
	dst->color[0].alpha = src->color[0].alpha;
	dst->color[0].red   = src->color[0].red;
	dst->color[0].green = src->color[0].green;
	dst->color[0].blue  = src->color[0].blue;
}

static void ffb_copy_pv_twoside(GLcontext *ctx, GLuint edst, GLuint esrc)
{
	ffbContextPtr fmesa = FFB_CONTEXT(ctx);
	ffb_vertex *dst = &fmesa->verts[edst];
	ffb_vertex *src = &fmesa->verts[esrc];

#ifdef VB_DEBUG
	fprintf(stderr, "ffb_copy_pv_twoside: edst(%d) esrc(%d)\n", edst, esrc);
#endif
	dst->color[0].alpha = src->color[0].alpha;
	dst->color[0].red   = src->color[0].red;
	dst->color[0].green = src->color[0].green;
	dst->color[0].blue  = src->color[0].blue;
	dst->color[1].alpha = src->color[1].alpha;
	dst->color[1].red   = src->color[1].red;
	dst->color[1].green = src->color[1].green;
	dst->color[1].blue  = src->color[1].blue;
}

#define FFB_VB_RGBA_BIT		0x01
#define FFB_VB_XYZ_BIT		0x02
#define FFB_VB_TWOSIDE_BIT	0x04
#define FFB_VB_MAX		0x08

typedef void (*ffb_emit_func)(GLcontext *, GLuint, GLuint);

static struct {
 	ffb_emit_func	emit;
	tnl_interp_func	interp;
} setup_tab[FFB_VB_MAX];


#define IND	(FFB_VB_XYZ_BIT)
#define TAG(x)	x##_w
#include "ffb_vbtmp.h"

#define IND	(FFB_VB_RGBA_BIT)
#define TAG(x)	x##_g
#include "ffb_vbtmp.h"

#define IND	(FFB_VB_XYZ_BIT | FFB_VB_RGBA_BIT)
#define TAG(x)	x##_wg
#include "ffb_vbtmp.h"

#define IND	(FFB_VB_TWOSIDE_BIT)
#define TAG(x)	x##_t
#include "ffb_vbtmp.h"

#define IND	(FFB_VB_XYZ_BIT | FFB_VB_TWOSIDE_BIT)
#define TAG(x)	x##_wt
#include "ffb_vbtmp.h"

#define IND	(FFB_VB_RGBA_BIT | FFB_VB_TWOSIDE_BIT)
#define TAG(x)	x##_gt
#include "ffb_vbtmp.h"

#define IND	(FFB_VB_XYZ_BIT | FFB_VB_RGBA_BIT | FFB_VB_TWOSIDE_BIT)
#define TAG(x)	x##_wgt
#include "ffb_vbtmp.h"

static void init_setup_tab( void )
{
	init_w();
	init_g();
	init_wg();
	init_t();
	init_wt();
	init_gt();
	init_wgt();
}

#ifdef VB_DEBUG
static void ffbPrintSetupFlags(char *msg, GLuint flags)
{
   fprintf(stderr, "%s(%x): %s%s%s\n",
	   msg,
	   (int)flags,
	   (flags & FFB_VB_XYZ_BIT)     ? " xyz," : "", 
	   (flags & FFB_VB_RGBA_BIT)    ? " rgba," : "",
	   (flags & FFB_VB_TWOSIDE_BIT) ? " twoside," : "");
}
#endif

static void ffbDDBuildVertices(GLcontext *ctx, GLuint start, GLuint count, 
			       GLuint newinputs)
{
	ffbContextPtr fmesa = FFB_CONTEXT(ctx);

	newinputs |= fmesa->setupnewinputs;
	fmesa->setupnewinputs = 0;

	if (!newinputs)
		return;

	if (newinputs & VERT_BIT_POS) {
		setup_tab[fmesa->setupindex].emit(ctx, start, count);
	} else {
		GLuint ind = 0;

		if (newinputs & VERT_BIT_COLOR0)
			ind |= (FFB_VB_RGBA_BIT | FFB_VB_TWOSIDE_BIT);

		ind &= fmesa->setupindex;

		if (ind)
			setup_tab[ind].emit(ctx, start, count);
	}
}

void ffbChooseVertexState( GLcontext *ctx )
{
 	TNLcontext *tnl = TNL_CONTEXT(ctx);
	ffbContextPtr fmesa = FFB_CONTEXT(ctx);
	int ind = FFB_VB_XYZ_BIT | FFB_VB_RGBA_BIT;

	if (ctx->_TriangleCaps & DD_TRI_LIGHT_TWOSIDE)
		ind |= FFB_VB_TWOSIDE_BIT;

#ifdef VB_DEBUG
	ffbPrintSetupFlags("ffb: full setup function", ind);
#endif

	fmesa->setupindex = ind;

	tnl->Driver.Render.BuildVertices = ffbDDBuildVertices;
	tnl->Driver.Render.Interp = setup_tab[ind].interp;
	if (ind & FFB_VB_TWOSIDE_BIT)
		tnl->Driver.Render.CopyPV = ffb_copy_pv_twoside;
	else
		tnl->Driver.Render.CopyPV = ffb_copy_pv_oneside;
}

void ffbInitVB( GLcontext *ctx )
{
	ffbContextPtr fmesa = FFB_CONTEXT(ctx);
	GLuint size = TNL_CONTEXT(ctx)->vb.Size;

	fmesa->verts = (ffb_vertex *)ALIGN_MALLOC(size * sizeof(ffb_vertex), 32);

	{
		static int firsttime = 1;
		if (firsttime) {
			init_setup_tab();
			firsttime = 0;
		}
	}
}


void ffbFreeVB( GLcontext *ctx )
{
	ffbContextPtr fmesa = FFB_CONTEXT(ctx);
	if (fmesa->verts) {
		ALIGN_FREE(fmesa->verts);
		fmesa->verts = 0;
	}
}
