/* $XFree86: xc/lib/GL/mesa/src/drv/ffb/ffb_lines.c,v 1.2 2002/02/22 21:32:58 dawes Exp $
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
#include "mm.h"
#include "ffb_dd.h"
#include "ffb_span.h"
#include "ffb_depth.h"
#include "ffb_context.h"
#include "ffb_vb.h"
#include "ffb_lines.h"
#include "ffb_tris.h"
#include "ffb_lock.h"
#include "extensions.h"

#undef FFB_LINE_TRACE

#define FFB_LINE_FLAT_BIT	0x01
#define FFB_LINE_ALPHA_BIT	0x02
#define MAX_FFB_LINE_FUNCS      0x04

static ffb_line_func ffb_line_tab[MAX_FFB_LINE_FUNCS];

/* If the line is not wide, we can support all of the line
 * patterning and smooth shading features of OpenGL fully.
 */

#define IND (0)
#define TAG(x) x
#include "ffb_linetmp.h"

#define IND (FFB_LINE_FLAT_BIT)
#define TAG(x) x##_flat
#include "ffb_linetmp.h"

#define IND (FFB_LINE_ALPHA_BIT)
#define TAG(x) x##_alpha
#include "ffb_linetmp.h"

#define IND (FFB_LINE_ALPHA_BIT|FFB_LINE_FLAT_BIT)
#define TAG(x) x##_alpha_flat
#include "ffb_linetmp.h"

void ffbDDLinefuncInit(void)
{
	init();
	init_flat();
	init_alpha();
	init_alpha_flat();
}

static void ffb_dd_line( GLcontext *ctx, GLuint e0, GLuint e1 )
{
	ffbContextPtr fmesa = FFB_CONTEXT(ctx);
	ffb_vertex *v0 = &fmesa->verts[e0];
	ffb_vertex *v1 = &fmesa->verts[e1];
	fmesa->draw_line( ctx, v0, v1 );
}

void ffbChooseLineState(GLcontext *ctx)
{
	ffbContextPtr fmesa = FFB_CONTEXT(ctx);
 	TNLcontext *tnl = TNL_CONTEXT(ctx);
	GLuint flags = ctx->_TriangleCaps;
	GLuint ind = 0;

	tnl->Driver.Render.Line = ffb_dd_line;

	if (flags & DD_FLATSHADE)
		ind |= FFB_LINE_FLAT_BIT;

	if ((flags & DD_LINE_STIPPLE) != 0 &&
	    fmesa->lpat == FFB_LPAT_BAD) {
		fmesa->draw_line = ffb_fallback_line;
		return;
	}

	/* If blending or the alpha test is enabled we need to
	 * provide alpha components to the chip, else we can
	 * do without it and thus feed vertex data to the chip
	 * more efficiently.
	 */
	if (ctx->Color.BlendEnabled || ctx->Color.AlphaEnabled)
		ind |= FFB_LINE_ALPHA_BIT;

	fmesa->draw_line = ffb_line_tab[ind];
}
