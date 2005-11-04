/* $XFree86: xc/lib/GL/mesa/src/drv/ffb/ffb_points.c,v 1.2 2002/02/22 21:32:59 dawes Exp $
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

#include "mtypes.h"
#include "ffb_dd.h"
#include "ffb_context.h"
#include "ffb_vb.h"
#include "ffb_points.h"
#include "ffb_tris.h"
#include "ffb_lock.h"


#undef FFB_POINT_TRACE

#define FFB_POINT_AA_BIT	0x01

static ffb_point_func ffb_point_tab[0x08];

#define IND (0)
#define TAG(x) x
#include "ffb_pointtmp.h"

#define IND (FFB_POINT_AA_BIT)
#define TAG(x) x##_aa
#include "ffb_pointtmp.h"

void ffbDDPointfuncInit(void)
{
	init();
	init_aa();
}

static void ffb_dd_points( GLcontext *ctx, GLuint first, GLuint last )
{
	struct vertex_buffer *VB = &TNL_CONTEXT( ctx )->vb;
	ffbContextPtr fmesa = FFB_CONTEXT(ctx);
	ffb_vertex *fverts = fmesa->verts;
	int i;

	if (VB->Elts == 0) {
		for ( i = first ; i < last ; i++ ) {
			if ( VB->ClipMask[i] == 0 ) {
				fmesa->draw_point( ctx, &fverts[i] );
			}
		}
	} else {
		for ( i = first ; i < last ; i++ ) {
			GLuint e = VB->Elts[i];
			if ( VB->ClipMask[e] == 0 ) {
				fmesa->draw_point( ctx, &fverts[e] );
			}
		}
	}
}

void ffbChoosePointState(GLcontext *ctx)
{
	ffbContextPtr fmesa = FFB_CONTEXT(ctx);
 	TNLcontext *tnl = TNL_CONTEXT(ctx);
	GLuint flags = ctx->_TriangleCaps;
	GLuint ind = 0;

	tnl->Driver.Render.Points = ffb_dd_points;

	if (flags & DD_POINT_SMOOTH)
		ind |= FFB_POINT_AA_BIT;

	fmesa->draw_point = ffb_point_tab[ind];
}
