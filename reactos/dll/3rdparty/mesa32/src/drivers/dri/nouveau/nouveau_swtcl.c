/**************************************************************************

Copyright 2006 Stephane Marchesin
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
ERIC ANHOLT OR SILICON INTEGRATED SYSTEMS CORP BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/

/* Common software TCL code */

#include "nouveau_context.h"
#include "nouveau_swtcl.h"
#include "nv10_swtcl.h"
#include "nouveau_span.h"
#include "swrast/swrast.h"
#include "swrast_setup/swrast_setup.h"
#include "tnl/tnl.h"
#include "tnl/t_pipeline.h"

/* Common tri functions */

/* The fallbacks */
void nouveau_fallback_tri(struct nouveau_context *nmesa,
		nouveauVertex *v0,
		nouveauVertex *v1,
		nouveauVertex *v2)
{    
	GLcontext *ctx = nmesa->glCtx;
	SWvertex v[3];
	_swsetup_Translate(ctx, v0, &v[0]);
	_swsetup_Translate(ctx, v1, &v[1]);
	_swsetup_Translate(ctx, v2, &v[2]);
	_swrast_Triangle(ctx, &v[0], &v[1], &v[2]);
}


void nouveau_fallback_line(struct nouveau_context *nmesa,
		nouveauVertex *v0,
		nouveauVertex *v1)
{
	GLcontext *ctx = nmesa->glCtx;
	SWvertex v[2];
	_swsetup_Translate(ctx, v0, &v[0]);
	_swsetup_Translate(ctx, v1, &v[1]);
	_swrast_Line(ctx, &v[0], &v[1]);
}


void nouveau_fallback_point(struct nouveau_context *nmesa,
		nouveauVertex *v0)
{
	GLcontext *ctx = nmesa->glCtx;
	SWvertex v[1];
	_swsetup_Translate(ctx, v0, &v[0]);
	_swrast_Point(ctx, &v[0]);
}

void nouveauFallback(struct nouveau_context *nmesa, GLuint bit, GLboolean mode)
{
	GLcontext *ctx = nmesa->glCtx;
	GLuint oldfallback = nmesa->Fallback;

	if (mode) {
		nmesa->Fallback |= bit;
		if (oldfallback == 0) {
			if (nmesa->screen->card->type<NV_10) {
				//nv04FinishPrimitive(nmesa);
			} else {
				//nv10FinishPrimitive(nmesa);
			}

			_swsetup_Wakeup(ctx);
			nmesa->render_index = ~0;
		}
	}
	else {
		nmesa->Fallback &= ~bit;
		if (oldfallback == bit) {
			_swrast_flush( ctx );

			if (nmesa->screen->card->type<NV_10) {
				nv04TriInitFunctions(ctx);
			} else {
				nv10TriInitFunctions(ctx);
			}

			_tnl_invalidate_vertex_state( ctx, ~0 );
			_tnl_invalidate_vertices( ctx, ~0 );
			_tnl_install_attrs( ctx, 
					nmesa->vertex_attrs, 
					nmesa->vertex_attr_count,
					nmesa->viewport.m, 0 ); 
		}
	}    
}


void nouveauRunPipeline( GLcontext *ctx )
{
	struct nouveau_context *nmesa = NOUVEAU_CONTEXT(ctx);

	if (nmesa->new_state) {
		nmesa->new_render_state |= nmesa->new_state;
	}

	_tnl_run_pipeline( ctx );
}


