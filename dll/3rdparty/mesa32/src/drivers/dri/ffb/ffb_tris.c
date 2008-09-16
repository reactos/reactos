/* $XFree86: xc/lib/GL/mesa/src/drv/ffb/ffb_tris.c,v 1.3 2002/10/30 12:51:28 alanh Exp $
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

#include "glheader.h"
#include "mtypes.h"
#include "macros.h"
#include "swrast/swrast.h"
#include "swrast_setup/swrast_setup.h"
#include "swrast/s_context.h"
#include "tnl/t_context.h"
#include "tnl/t_pipeline.h"

#include "ffb_context.h"
#include "ffb_tris.h"
#include "ffb_lines.h"
#include "ffb_lock.h"
#include "ffb_points.h"
#include "ffb_state.h"
#include "ffb_vb.h"

#undef TRI_DEBUG
#undef FFB_RENDER_TRACE
#undef STATE_TRACE

#ifdef TRI_DEBUG
static void ffb_print_vertex(const ffb_vertex *v)
{
	fprintf(stderr, "Vertex @(%p): "
		"X[%f] Y[%f] Z[%f]\n",
		v, v->x, v->y, v->z);
	fprintf(stderr, "Vertex @(%p): "
		"A[%f] R[%f] G[%f] B[%f]\n",
		v,
		v->color[0].alpha,
		v->color[0].red,
		v->color[0].green,
		v->color[0].blue);
}
#define FFB_DUMP_VERTEX(V)	ffb_print_vertex(V)
#else
#define FFB_DUMP_VERTEX(V)	do { } while(0)
#endif

#define FFB_ALPHA_BIT		0x01
#define FFB_FLAT_BIT		0x02
#define FFB_TRI_CULL_BIT	0x04
#define MAX_FFB_RENDER_FUNCS	0x08

/***********************************************************************
 *         Build low-level triangle/quad rasterize functions           *
 ***********************************************************************/

#define FFB_TRI_FLAT_BIT	0x01
#define FFB_TRI_ALPHA_BIT	0x02
/*#define FFB_TRI_CULL_BIT	0x04*/

static ffb_tri_func ffb_tri_tab[0x8];
static ffb_quad_func ffb_quad_tab[0x8];

#define IND (0)
#define TAG(x) x
#include "ffb_tritmp.h"

#define IND (FFB_TRI_FLAT_BIT)
#define TAG(x) x##_flat
#include "ffb_tritmp.h"

#define IND (FFB_TRI_CULL_BIT)
#define TAG(x) x##_cull
#include "ffb_tritmp.h"

#define IND (FFB_TRI_CULL_BIT|FFB_TRI_FLAT_BIT)
#define TAG(x) x##_cull_flat
#include "ffb_tritmp.h"

#define IND (FFB_TRI_ALPHA_BIT)
#define TAG(x) x##_alpha
#include "ffb_tritmp.h"

#define IND (FFB_TRI_ALPHA_BIT|FFB_TRI_FLAT_BIT)
#define TAG(x) x##_alpha_flat
#include "ffb_tritmp.h"

#define IND (FFB_TRI_ALPHA_BIT|FFB_TRI_CULL_BIT)
#define TAG(x) x##_alpha_cull
#include "ffb_tritmp.h"

#define IND (FFB_TRI_ALPHA_BIT|FFB_TRI_CULL_BIT|FFB_TRI_FLAT_BIT)
#define TAG(x) x##_alpha_cull_flat
#include "ffb_tritmp.h"

static void init_tri_tab(void)
{
	ffb_init();
	ffb_init_flat();
	ffb_init_cull();
	ffb_init_cull_flat();
	ffb_init_alpha();
	ffb_init_alpha_flat();
	ffb_init_alpha_cull();
	ffb_init_alpha_cull_flat();
}

/* Build a SWvertex from a hardware vertex. */
static void ffb_translate_vertex(GLcontext *ctx, const ffb_vertex *src,
				 SWvertex *dst)
{
	ffbContextPtr fmesa = FFB_CONTEXT(ctx);
	GLfloat *m = ctx->Viewport._WindowMap.m;
	const GLfloat sx = m[0];
	const GLfloat sy = m[5];
	const GLfloat sz = m[10];
	const GLfloat tx = m[12];
	const GLfloat ty = m[13];
	const GLfloat tz = m[14];

	dst->win[0] = sx * src->x + tx;
	dst->win[1] = sy * src->y + ty;
	dst->win[2] = sz * src->z + tz;
	dst->win[3] = 1.0;
      
	dst->color[0] = FFB_UBYTE_FROM_COLOR(src->color[0].red);
	dst->color[1] = FFB_UBYTE_FROM_COLOR(src->color[0].green);
	dst->color[2] = FFB_UBYTE_FROM_COLOR(src->color[0].blue);
	dst->color[3] = FFB_UBYTE_FROM_COLOR(src->color[0].alpha);
}

/***********************************************************************
 *          Build fallback triangle/quad rasterize functions           *
 ***********************************************************************/

static void ffb_fallback_triangle(GLcontext *ctx, ffb_vertex *v0,
				  ffb_vertex *v1, ffb_vertex *v2)
{
	SWvertex v[3];

	ffb_translate_vertex(ctx, v0, &v[0]);
	ffb_translate_vertex(ctx, v1, &v[1]);
	ffb_translate_vertex(ctx, v2, &v[2]);

	_swrast_Triangle(ctx, &v[0], &v[1], &v[2]);
}

static void ffb_fallback_quad(GLcontext *ctx,
			      ffb_vertex *v0, ffb_vertex *v1, 
			      ffb_vertex *v2, ffb_vertex *v3)
{
	SWvertex v[4];

	ffb_translate_vertex(ctx, v0, &v[0]);
	ffb_translate_vertex(ctx, v1, &v[1]);
	ffb_translate_vertex(ctx, v2, &v[2]);
	ffb_translate_vertex(ctx, v3, &v[3]);

	_swrast_Quad(ctx, &v[0], &v[1], &v[2], &v[3]);
}

void ffb_fallback_line(GLcontext *ctx, ffb_vertex *v0, ffb_vertex *v1)
{
	SWvertex v[2];

	ffb_translate_vertex(ctx, v0, &v[0]);
	ffb_translate_vertex(ctx, v1, &v[1]);

	_swrast_Line(ctx, &v[0], &v[1]);
}

void ffb_fallback_point(GLcontext *ctx, ffb_vertex *v0)
{
	SWvertex v[1];

	ffb_translate_vertex(ctx, v0, &v[0]);

	_swrast_Point(ctx, &v[0]);
}

/***********************************************************************
 *             Rasterization functions for culled tris/quads           *
 ***********************************************************************/

static void ffb_nodraw_triangle(GLcontext *ctx, ffb_vertex *v0,
				ffb_vertex *v1, ffb_vertex *v2)
{
	(void) (ctx && v0 && v1 && v2);
}

static void ffb_nodraw_quad(GLcontext *ctx,
			    ffb_vertex *v0, ffb_vertex *v1, 
			    ffb_vertex *v2, ffb_vertex *v3)
{
	(void) (ctx && v0 && v1 && v2 && v3);
}

static void ffb_update_cullsign(GLcontext *ctx)
{
	GLfloat backface_sign = 1;

	switch (ctx->Polygon.CullFaceMode) {
	case GL_BACK:
		if (ctx->Polygon.FrontFace==GL_CCW)
			backface_sign = -1;
		break;

	case GL_FRONT:
		if (ctx->Polygon.FrontFace!=GL_CCW)
			backface_sign = -1;
		break;

	default:
		break;
	};

	FFB_CONTEXT(ctx)->backface_sign = backface_sign;
}

/***********************************************************************
 *               Choose triangle/quad rasterize functions              *
 ***********************************************************************/

void ffbChooseTriangleState(GLcontext *ctx)
{
	ffbContextPtr fmesa = FFB_CONTEXT(ctx);
	GLuint flags = ctx->_TriangleCaps;
	GLuint ind = 0;

	if (flags & DD_TRI_SMOOTH) {
		fmesa->draw_tri = ffb_fallback_triangle;
		fmesa->draw_quad = ffb_fallback_quad;
		return;
	}

	if (flags & DD_FLATSHADE)
		ind |= FFB_TRI_FLAT_BIT;

	if (ctx->Polygon.CullFlag) {
		if (ctx->Polygon.CullFaceMode == GL_FRONT_AND_BACK) {
			fmesa->draw_tri = ffb_nodraw_triangle;
			fmesa->draw_quad = ffb_nodraw_quad;
			return;
		}

		ind |= FFB_TRI_CULL_BIT;
		ffb_update_cullsign(ctx);
	} else
		FFB_CONTEXT(ctx)->backface_sign = 0;
		
	/* If blending or the alpha test is enabled we need to
	 * provide alpha components to the chip, else we can
	 * do without it and thus feed vertex data to the chip
	 * more efficiently.
	 */
	if (ctx->Color.BlendEnabled || ctx->Color.AlphaEnabled)
		ind |= FFB_TRI_ALPHA_BIT;

	fmesa->draw_tri = ffb_tri_tab[ind];
	fmesa->draw_quad = ffb_quad_tab[ind];
}

static const GLenum reduced_prim[GL_POLYGON+1] = {
   GL_POINTS,
   GL_LINES,
   GL_LINES,
   GL_LINES,
   GL_TRIANGLES,
   GL_TRIANGLES,
   GL_TRIANGLES,
   GL_TRIANGLES,
   GL_TRIANGLES,
   GL_TRIANGLES
};

static void ffbRenderPrimitive(GLcontext *ctx, GLenum prim);
static void ffbRasterPrimitive(GLcontext *ctx, GLenum rprim);

/***********************************************************************
 *              Build render functions from dd templates               *
 ***********************************************************************/

#define FFB_OFFSET_BIT  	0x01
#define FFB_TWOSIDE_BIT 	0x02
#define FFB_UNFILLED_BIT	0x04
#define FFB_MAX_TRIFUNC 	0x08

static struct {
	tnl_triangle_func triangle;
	tnl_quad_func quad;
} rast_tab[FFB_MAX_TRIFUNC];

#define DO_OFFSET   (IND & FFB_OFFSET_BIT)
#define DO_UNFILLED (IND & FFB_UNFILLED_BIT)
#define DO_TWOSIDE  (IND & FFB_TWOSIDE_BIT)
#define DO_FLAT      0
#define DO_QUAD      1
#define DO_FULL_QUAD 1
#define DO_TRI       1
#define DO_LINE      0
#define DO_POINTS    0

#define QUAD( a, b, c, d ) fmesa->draw_quad( ctx, a, b, c, d )
#define TRI( a, b, c )     fmesa->draw_tri( ctx, a, b, c )
#define LINE( a, b )       fmesa->draw_line( ctx, a, b )
#define POINT( a )         fmesa->draw_point( ctx, a )

#define HAVE_BACK_COLORS  1
#define HAVE_RGBA         1
#define HAVE_SPEC         0
#define HAVE_HW_FLATSHADE 1
#define VERTEX            ffb_vertex
#define TAB               rast_tab

#define UNFILLED_TRI      unfilled_tri
#define UNFILLED_QUAD     unfilled_quad
#define DEPTH_SCALE       (fmesa->depth_scale)
#define VERT_X(_v)        (_v->x)
#define VERT_Y(_v)        (_v->y)
#define VERT_Z(_v)	  (_v->z)
#define AREA_IS_CCW( a )  (a < fmesa->ffb_zero)
#define GET_VERTEX(e)     (&fmesa->verts[e])
#define INSANE_VERTICES
#define VERT_SET_Z(v,val) ((v)->z = (val))
#define VERT_Z_ADD(v,val) ((v)->z += (val))

#define VERT_COPY_RGBA1( _v )     _v->color[0] = _v->color[1]
#define VERT_COPY_RGBA( v0, v1 )  v0->color[0] = v1->color[0] 
#define VERT_SAVE_RGBA( idx )     color[idx] = v[idx]->color[0]
#define VERT_RESTORE_RGBA( idx )  v[idx]->color[0] = color[idx]   

#define LOCAL_VARS(n)				\
   ffbContextPtr fmesa = FFB_CONTEXT(ctx);	\
   __DRIdrawablePrivate *dPriv = fmesa->driDrawable; \
   ffb_color color[n];				\
   (void) color; (void) dPriv;

/***********************************************************************
 *                Helpers for rendering unfilled primitives            *
 ***********************************************************************/

#define RASTERIZE(x) if (fmesa->raster_primitive != reduced_prim[x]) \
                        ffbRasterPrimitive( ctx, reduced_prim[x] )
#define RENDER_PRIMITIVE fmesa->render_primitive
#define TAG(x) x
#include "tnl_dd/t_dd_unfilled.h"

/***********************************************************************
 *                      Generate GL render functions                   *
 ***********************************************************************/

#define IND (0)
#define TAG(x) x
#include "tnl_dd/t_dd_tritmp.h"

#define IND (FFB_OFFSET_BIT)
#define TAG(x) x##_offset
#include "tnl_dd/t_dd_tritmp.h"

#define IND (FFB_TWOSIDE_BIT)
#define TAG(x) x##_twoside
#include "tnl_dd/t_dd_tritmp.h"

#define IND (FFB_TWOSIDE_BIT|FFB_OFFSET_BIT)
#define TAG(x) x##_twoside_offset
#include "tnl_dd/t_dd_tritmp.h"

#define IND (FFB_UNFILLED_BIT)
#define TAG(x) x##_unfilled
#include "tnl_dd/t_dd_tritmp.h"

#define IND (FFB_OFFSET_BIT|FFB_UNFILLED_BIT)
#define TAG(x) x##_offset_unfilled
#include "tnl_dd/t_dd_tritmp.h"

#define IND (FFB_TWOSIDE_BIT|FFB_UNFILLED_BIT)
#define TAG(x) x##_twoside_unfilled
#include "tnl_dd/t_dd_tritmp.h"

#define IND (FFB_TWOSIDE_BIT|FFB_OFFSET_BIT|FFB_UNFILLED_BIT)
#define TAG(x) x##_twoside_offset_unfilled
#include "tnl_dd/t_dd_tritmp.h"

static void init_rast_tab( void )
{
   init();
   init_offset();
   init_twoside();
   init_twoside_offset();
   init_unfilled();
   init_offset_unfilled();
   init_twoside_unfilled();
   init_twoside_offset_unfilled();
}

/**********************************************************************/
/*                   Render clipped primitives                        */
/**********************************************************************/

static void ffbRenderClippedPolygon(GLcontext *ctx, const GLuint *elts, GLuint n)
{
	ffbContextPtr fmesa = FFB_CONTEXT(ctx);
	TNLcontext *tnl = TNL_CONTEXT(ctx);
	struct vertex_buffer *VB = &tnl->vb;
	GLuint prim = fmesa->render_primitive;

	/* Render the new vertices as an unclipped polygon. */
	{
		GLuint *tmp = VB->Elts;
		VB->Elts = (GLuint *)elts;
		tnl->Driver.Render.PrimTabElts[GL_POLYGON](ctx, 0, n, PRIM_BEGIN|PRIM_END);
		VB->Elts = tmp;
	}

	/* Restore the render primitive. */
	if (prim != GL_POLYGON)
		tnl->Driver.Render.PrimitiveNotify(ctx, prim);
}

static void ffbRenderClippedLine(GLcontext *ctx, GLuint ii, GLuint jj)
{
	TNLcontext *tnl = TNL_CONTEXT(ctx);
	tnl->Driver.Render.Line(ctx, ii, jj);
}

/**********************************************************************/
/*               Render unclipped begin/end objects                   */
/**********************************************************************/

static void ffb_vb_noop(GLcontext *ctx, GLuint start, GLuint count, GLuint flags)
{
	(void)(ctx && start && count && flags);
}

#define ELT(x)	x

#define IND	0
#define TAG(x)	x
#include "ffb_rendertmp.h"

#define IND	(FFB_FLAT_BIT)
#define TAG(x)	x##_flat
#include "ffb_rendertmp.h"

#define IND	(FFB_ALPHA_BIT)
#define TAG(x)	x##_alpha
#include "ffb_rendertmp.h"

#define IND	(FFB_FLAT_BIT | FFB_ALPHA_BIT)
#define TAG(x)	x##_flat_alpha
#include "ffb_rendertmp.h"

#define IND	(FFB_TRI_CULL_BIT)
#define TAG(x)	x##_tricull
#include "ffb_rendertmp.h"

#define IND	(FFB_FLAT_BIT | FFB_TRI_CULL_BIT)
#define TAG(x)	x##_flat_tricull
#include "ffb_rendertmp.h"

#define IND	(FFB_ALPHA_BIT | FFB_TRI_CULL_BIT)
#define TAG(x)	x##_alpha_tricull
#include "ffb_rendertmp.h"

#define IND	(FFB_FLAT_BIT | FFB_ALPHA_BIT | FFB_TRI_CULL_BIT)
#define TAG(x)	x##_flat_alpha_tricull
#include "ffb_rendertmp.h"

#undef ELT
#define ELT(x)	elt[x]

#define IND	0
#define TAG(x)	x##_elt
#include "ffb_rendertmp.h"

#define IND	(FFB_FLAT_BIT)
#define TAG(x)	x##_flat_elt
#include "ffb_rendertmp.h"

#define IND	(FFB_ALPHA_BIT)
#define TAG(x)	x##_alpha_elt
#include "ffb_rendertmp.h"

#define IND	(FFB_FLAT_BIT | FFB_ALPHA_BIT)
#define TAG(x)	x##_flat_alpha_elt
#include "ffb_rendertmp.h"

#define IND	(FFB_TRI_CULL_BIT)
#define TAG(x)	x##_tricull_elt
#include "ffb_rendertmp.h"

#define IND	(FFB_FLAT_BIT | FFB_TRI_CULL_BIT)
#define TAG(x)	x##_flat_tricull_elt
#include "ffb_rendertmp.h"

#define IND	(FFB_ALPHA_BIT | FFB_TRI_CULL_BIT)
#define TAG(x)	x##_alpha_tricull_elt
#include "ffb_rendertmp.h"

#define IND	(FFB_FLAT_BIT | FFB_ALPHA_BIT | FFB_TRI_CULL_BIT)
#define TAG(x)	x##_flat_alpha_tricull_elt
#include "ffb_rendertmp.h"

static void *render_tabs[MAX_FFB_RENDER_FUNCS];
static void *render_tabs_elt[MAX_FFB_RENDER_FUNCS];

static void init_render_tab(void)
{
	int i;

	render_tabs[0] = render_tab;
	render_tabs[FFB_FLAT_BIT] = render_tab_flat;
	render_tabs[FFB_ALPHA_BIT] = render_tab_alpha;
	render_tabs[FFB_FLAT_BIT|FFB_ALPHA_BIT] = render_tab_flat_alpha;
	render_tabs[FFB_TRI_CULL_BIT] = render_tab_tricull;
	render_tabs[FFB_FLAT_BIT|FFB_TRI_CULL_BIT] = render_tab_flat_tricull;
	render_tabs[FFB_ALPHA_BIT|FFB_TRI_CULL_BIT] = render_tab_alpha_tricull;
	render_tabs[FFB_FLAT_BIT|FFB_ALPHA_BIT|FFB_TRI_CULL_BIT] =
		render_tab_flat_alpha_tricull;

	render_tabs_elt[0] = render_tab_elt;
	render_tabs_elt[FFB_FLAT_BIT] = render_tab_flat_elt;
	render_tabs_elt[FFB_ALPHA_BIT] = render_tab_alpha_elt;
	render_tabs_elt[FFB_FLAT_BIT|FFB_ALPHA_BIT] = render_tab_flat_alpha_elt;
	render_tabs_elt[FFB_TRI_CULL_BIT] = render_tab_tricull_elt;
	render_tabs_elt[FFB_FLAT_BIT|FFB_TRI_CULL_BIT] = render_tab_flat_tricull_elt;
	render_tabs_elt[FFB_ALPHA_BIT|FFB_TRI_CULL_BIT] = render_tab_alpha_tricull_elt;
	render_tabs_elt[FFB_FLAT_BIT|FFB_ALPHA_BIT|FFB_TRI_CULL_BIT] =
		render_tab_flat_alpha_tricull_elt;

	for (i = 0; i < MAX_FFB_RENDER_FUNCS; i++) {
		tnl_render_func *rf = render_tabs[i];
		tnl_render_func *rfe = render_tabs_elt[i];

		if (i & FFB_TRI_CULL_BIT) {
			int from_idx = (i & ~FFB_TRI_CULL_BIT);
			tnl_render_func *rf_from = render_tabs[from_idx];
			tnl_render_func *rfe_from = render_tabs_elt[from_idx];
			int j;

			for (j = GL_POINTS; j < GL_TRIANGLES; j++) {
				rf[j] = rf_from[j];
				rfe[j] = rfe_from[j];
			}
		}
	}
}

/**********************************************************************/
/*                    Choose render functions                         */
/**********************************************************************/

#ifdef FFB_RENDER_TRACE
static void ffbPrintRenderFlags(GLuint index, GLuint render_index)
{
	fprintf(stderr,
		"ffbChooseRenderState: "
		"index(%s%s%s) "
		"render_index(%s%s%s)\n",
		((index & FFB_TWOSIDE_BIT) ? "twoside " : ""),
		((index & FFB_OFFSET_BIT) ? "offset " : ""),
		((index & FFB_UNFILLED_BIT) ? "unfilled " : ""),
		((render_index & FFB_FLAT_BIT) ? "flat " : ""),
		((render_index & FFB_ALPHA_BIT) ? "alpha " : ""),
		((render_index & FFB_TRI_CULL_BIT) ? "tricull " : ""));
}
#endif

void ffbChooseRenderState(GLcontext *ctx)
{
	GLuint flags = ctx->_TriangleCaps;
 	TNLcontext *tnl = TNL_CONTEXT(ctx);
	GLuint index = 0;

	/* Per-primitive fallbacks and the selection of fmesa->draw_* are
	 * handled elsewhere.
	 */
	if (flags & DD_TRI_LIGHT_TWOSIDE)       
		index |= FFB_TWOSIDE_BIT;

	if (flags & DD_TRI_OFFSET)	      
		index |= FFB_OFFSET_BIT;

	if (flags & DD_TRI_UNFILLED)	     
		index |= FFB_UNFILLED_BIT;

	tnl->Driver.Render.Triangle = rast_tab[index].triangle;
	tnl->Driver.Render.Quad = rast_tab[index].quad;

	if (index == 0) {
		GLuint render_index = 0;

		if (flags & DD_FLATSHADE)
			render_index |= FFB_FLAT_BIT;

		if (ctx->Color.BlendEnabled || ctx->Color.AlphaEnabled)
			render_index |= FFB_ALPHA_BIT;

		if (ctx->Polygon.CullFlag)
			render_index |= FFB_TRI_CULL_BIT;

#ifdef FFB_RENDER_TRACE
		ffbPrintRenderFlags(index, render_index);
#endif
		tnl->Driver.Render.PrimTabVerts = render_tabs[render_index];
		tnl->Driver.Render.PrimTabElts = render_tabs_elt[render_index];
	} else {
#ifdef FFB_RENDER_TRACE
		ffbPrintRenderFlags(index, 0);
#endif
		tnl->Driver.Render.PrimTabVerts = _tnl_render_tab_verts;
		tnl->Driver.Render.PrimTabElts = _tnl_render_tab_elts;
	}

	tnl->Driver.Render.ClippedPolygon = ffbRenderClippedPolygon;
	tnl->Driver.Render.ClippedLine    = ffbRenderClippedLine;
}

static void ffbRunPipeline(GLcontext *ctx)
{
	ffbContextPtr fmesa = FFB_CONTEXT(ctx);

	if (fmesa->bad_fragment_attrs == 0 &&
	    fmesa->new_gl_state) {
		if (fmesa->new_gl_state & _FFB_NEW_TRIANGLE)
			ffbChooseTriangleState(ctx);
		if (fmesa->new_gl_state & _FFB_NEW_LINE)
			ffbChooseLineState(ctx);
		if (fmesa->new_gl_state & _FFB_NEW_POINT)
			ffbChoosePointState(ctx);
		if (fmesa->new_gl_state & _FFB_NEW_RENDER)
			ffbChooseRenderState(ctx);
		if (fmesa->new_gl_state & _FFB_NEW_VERTEX)
			ffbChooseVertexState(ctx);

		fmesa->new_gl_state = 0;
	}

	_tnl_run_pipeline(ctx);
}

static void ffbRenderStart(GLcontext *ctx)
{
	ffbContextPtr fmesa = FFB_CONTEXT(ctx);

	LOCK_HARDWARE(fmesa);
	fmesa->hw_locked = 1;

	if (fmesa->state_dirty != 0)
		ffbSyncHardware(fmesa);
}

static void ffbRenderFinish(GLcontext *ctx)
{
	ffbContextPtr fmesa = FFB_CONTEXT(ctx);

	UNLOCK_HARDWARE(fmesa);
	fmesa->hw_locked = 0;
}

/* Even when doing full software rendering we need to
 * wrap render{start,finish} so that the hardware is kept
 * in sync (because multipass rendering changes the write
 * buffer etc.)
 */
static void ffbSWRenderStart(GLcontext *ctx)
{
	ffbContextPtr fmesa = FFB_CONTEXT(ctx);

	LOCK_HARDWARE(fmesa);
	fmesa->hw_locked = 1;

	if (fmesa->state_dirty != 0)
		ffbSyncHardware(fmesa);
}

static void ffbSWRenderFinish(GLcontext *ctx)
{
	ffbContextPtr fmesa = FFB_CONTEXT(ctx);

	UNLOCK_HARDWARE(fmesa);
	fmesa->hw_locked = 0;
}

static void ffbRasterPrimitive(GLcontext *ctx, GLenum rprim)
{
	ffbContextPtr fmesa = FFB_CONTEXT(ctx);
	GLuint drawop, fbc, ppc;
	int do_sw = 0;

	fmesa->raster_primitive = rprim;

	drawop = fmesa->drawop;
	fbc = fmesa->fbc;
	ppc = fmesa->ppc & ~(FFB_PPC_ZS_MASK | FFB_PPC_CS_MASK);

#ifdef STATE_TRACE
	fprintf(stderr,
		"ffbReducedPrimitiveChange: rprim(%d) ", rprim);
#endif
	switch(rprim) {
	case GL_POINTS:
#ifdef STATE_TRACE
		fprintf(stderr, "GL_POINTS ");
#endif
		if (fmesa->draw_point == ffb_fallback_point) {
			do_sw = 1;
			break;
		}

		if (ctx->Point.SmoothFlag) {
			ppc |= (FFB_PPC_ZS_VAR | FFB_PPC_CS_CONST);
			drawop = FFB_DRAWOP_AADOT;
		} else {
			ppc |= (FFB_PPC_ZS_CONST | FFB_PPC_CS_CONST);
			drawop = FFB_DRAWOP_DOT;
		}
		break;

	case GL_LINES:
#ifdef STATE_TRACE
		fprintf(stderr, "GL_LINES ");
#endif
		if (fmesa->draw_line == ffb_fallback_line) {
			do_sw = 1;
			break;
		}

		if (ctx->_TriangleCaps & DD_FLATSHADE) {
			ppc |= FFB_PPC_ZS_VAR | FFB_PPC_CS_CONST;
		} else {
			ppc |= FFB_PPC_ZS_VAR | FFB_PPC_CS_VAR;
		}
		if (ctx->Line.SmoothFlag)
			drawop = FFB_DRAWOP_AALINE;
		else
			drawop = FFB_DRAWOP_DDLINE;
		break;

	case GL_TRIANGLES:
#ifdef STATE_TRACE
		fprintf(stderr, "GL_POLYGON ");
#endif
		if (fmesa->draw_tri == ffb_fallback_triangle) {
			do_sw = 1;
			break;
		}

		ppc &= ~FFB_PPC_APE_MASK;
		if (ctx->Polygon.StippleFlag)
			ppc |= FFB_PPC_APE_ENABLE;
		else
			ppc |= FFB_PPC_APE_DISABLE;

		if (ctx->_TriangleCaps & DD_FLATSHADE) {
			ppc |= FFB_PPC_ZS_VAR | FFB_PPC_CS_CONST;
		} else {
			ppc |= FFB_PPC_ZS_VAR | FFB_PPC_CS_VAR;
		}
		drawop = FFB_DRAWOP_TRIANGLE;
		break;

	default:
#ifdef STATE_TRACE
		fprintf(stderr, "unknown %d!\n", rprim);
#endif
		return;
	};

#ifdef STATE_TRACE
	fprintf(stderr, "do_sw(%d) ", do_sw);
#endif
	if (do_sw != 0) {
		fbc &= ~(FFB_FBC_WB_C);
		fbc &= ~(FFB_FBC_ZE_MASK | FFB_FBC_RGBE_MASK);
		fbc |=   FFB_FBC_ZE_OFF  | FFB_FBC_RGBE_MASK;
		ppc &= ~(FFB_PPC_XS_MASK | FFB_PPC_ABE_MASK |
			 FFB_PPC_DCE_MASK | FFB_PPC_APE_MASK);
		ppc |=  (FFB_PPC_ZS_VAR | FFB_PPC_CS_VAR | FFB_PPC_XS_WID |
			 FFB_PPC_ABE_DISABLE | FFB_PPC_DCE_DISABLE |
			 FFB_PPC_APE_DISABLE);
	} else {
		fbc |= FFB_FBC_WB_C;
		fbc &= ~(FFB_FBC_RGBE_MASK);
		fbc |=   FFB_FBC_RGBE_MASK;
		ppc &= ~(FFB_PPC_ABE_MASK | FFB_PPC_XS_MASK);
		if (ctx->Color.BlendEnabled) {
			if ((rprim == GL_POINTS && !ctx->Point.SmoothFlag) ||
			    (rprim != GL_POINTS && ctx->_TriangleCaps & DD_FLATSHADE))
				ppc |= FFB_PPC_ABE_ENABLE | FFB_PPC_XS_CONST;
			else
				ppc |= FFB_PPC_ABE_ENABLE | FFB_PPC_XS_VAR;
		} else {
			ppc |= FFB_PPC_ABE_DISABLE | FFB_PPC_XS_WID;
		}
	}
#ifdef STATE_TRACE
	fprintf(stderr, "fbc(%08x) ppc(%08x)\n", fbc, ppc);
#endif

	FFBFifo(fmesa, 4);
	if (fmesa->drawop != drawop)
		fmesa->regs->drawop = fmesa->drawop = drawop;
	if (fmesa->fbc != fbc)
		fmesa->regs->fbc = fmesa->fbc = fbc;
	if (fmesa->ppc != ppc)
		fmesa->regs->ppc = fmesa->ppc = ppc;
	if (do_sw != 0) {
		fmesa->regs->cmp =
			(fmesa->cmp & ~(0xff<<16)) | (0x80 << 16);
	} else
		fmesa->regs->cmp = fmesa->cmp;
}

static void ffbRenderPrimitive(GLcontext *ctx, GLenum prim)
{
	ffbContextPtr fmesa = FFB_CONTEXT(ctx);
	GLuint rprim = reduced_prim[prim];

	fmesa->render_primitive = prim;

	if (rprim == GL_TRIANGLES && (ctx->_TriangleCaps & DD_TRI_UNFILLED))
		return;

	if (fmesa->raster_primitive != rprim) {
		ffbRasterPrimitive( ctx, rprim );
	}
}




/**********************************************************************/
/*           Transition to/from hardware rasterization.               */
/**********************************************************************/

static char *fallbackStrings[] = {
	"Fog enabled",
	"Blend function",
	"Blend ROP",
	"Blend equation",
	"Stencil",
	"Texture",
	"LIBGL_SOFTWARE_RENDERING"
};

static char *getFallbackString(GLuint bit)
{
	int i = 0;

	while (bit > 1) {
		i++;
		bit >>= 1;
	}
	return fallbackStrings[i];
}

void ffbFallback( GLcontext *ctx, GLuint bit, GLboolean mode )
{
	ffbContextPtr fmesa = FFB_CONTEXT(ctx);
 	TNLcontext *tnl = TNL_CONTEXT(ctx);
	GLuint oldfallback = fmesa->bad_fragment_attrs;

	if (mode) {
		fmesa->bad_fragment_attrs |= bit;
		if (oldfallback == 0) {
/*  			FFB_FIREVERTICES(fmesa); */
  		        _swsetup_Wakeup( ctx );
			if (fmesa->debugFallbacks)
				fprintf(stderr, "FFB begin software fallback: 0x%x %s\n",
					bit, getFallbackString(bit));
		}
	} else {
		fmesa->bad_fragment_attrs &= ~bit;
		if (oldfallback == bit) {
			_swrast_flush( ctx );

			tnl->Driver.Render.Start = ffbRenderStart;
			tnl->Driver.Render.PrimitiveNotify = ffbRenderPrimitive;
			tnl->Driver.Render.Finish = ffbRenderFinish;
			fmesa->new_gl_state = ~0;

			/* Just re-choose everything:
			 */
			ffbChooseVertexState(ctx);
			ffbChooseRenderState(ctx);
			ffbChooseTriangleState(ctx);
			ffbChooseLineState(ctx);
			ffbChoosePointState(ctx);

			if (fmesa->debugFallbacks)
				fprintf(stderr, "FFB end software fallback: 0x%x %s\n",
					bit, getFallbackString(bit));
		}
	}
}

/**********************************************************************/
/*                            Initialization.                         */
/**********************************************************************/

void ffbDDInitRenderFuncs( GLcontext *ctx )
{
 	TNLcontext *tnl = TNL_CONTEXT(ctx);
	SWcontext *swrast = SWRAST_CONTEXT(ctx);
	static int firsttime = 1;

	if (firsttime) {
		init_rast_tab();
		init_tri_tab();
		init_render_tab();
		firsttime = 0;
	}

	tnl->Driver.RunPipeline = ffbRunPipeline;
	tnl->Driver.Render.Start = ffbRenderStart;
	tnl->Driver.Render.Finish = ffbRenderFinish; 
	tnl->Driver.Render.PrimitiveNotify = ffbRenderPrimitive;
	tnl->Driver.Render.ResetLineStipple = _swrast_ResetLineStipple;
	tnl->Driver.Render.PrimTabVerts = _tnl_render_tab_verts;
	tnl->Driver.Render.PrimTabElts = _tnl_render_tab_elts;

	swrast->Driver.SpanRenderStart = ffbSWRenderStart;
	swrast->Driver.SpanRenderFinish = ffbSWRenderFinish;
}
