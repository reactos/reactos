/*
 * Copyright 2007 Stephane Marchesin. All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sub license,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * VIA, S3 GRAPHICS, AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

/* Software TCL for NV04, NV05, NV06 */

#include <stdio.h>
#include <math.h>

#include "glheader.h"
#include "context.h"
#include "mtypes.h"
#include "macros.h"
#include "colormac.h"
#include "enums.h"

#include "swrast/swrast.h"
#include "swrast_setup/swrast_setup.h"
#include "tnl/t_context.h"
#include "tnl/t_pipeline.h"

#include "nouveau_swtcl.h"
#include "nv04_swtcl.h"
#include "nouveau_context.h"
#include "nouveau_span.h"
#include "nouveau_reg.h"
#include "nouveau_tex.h"
#include "nouveau_fifo.h"
#include "nouveau_msg.h"
#include "nouveau_object.h"

static void nv04RasterPrimitive( GLcontext *ctx, GLenum rprim, GLuint hwprim );
static void nv04RenderPrimitive( GLcontext *ctx, GLenum prim );
static void nv04ResetLineStipple( GLcontext *ctx );


static inline void nv04_2triangles(struct nouveau_context *nmesa,nouveauVertex* v0,nouveauVertex* v1,nouveauVertex* v2,nouveauVertex* v3,nouveauVertex* v4,nouveauVertex* v5)
{
	BEGIN_RING_SIZE(NvSub3D,NV04_DX5_TEXTURED_TRIANGLE_TLVERTEX_SX(0xA),49);
	OUT_RINGp(v0,8);
	OUT_RINGp(v1,8);
	OUT_RINGp(v2,8);
	OUT_RINGp(v3,8);
	OUT_RINGp(v4,8);
	OUT_RINGp(v5,8);
	OUT_RING(0xFEDCBA);
}

static inline void nv04_1triangle(struct nouveau_context *nmesa,nouveauVertex* v0,nouveauVertex* v1,nouveauVertex* v2)
{
	BEGIN_RING_SIZE(NvSub3D,NV04_DX5_TEXTURED_TRIANGLE_TLVERTEX_SX(0xD),25);
	OUT_RINGp(v0,8);
	OUT_RINGp(v1,8);
	OUT_RINGp(v2,8);
	OUT_RING(0xFED);
}

static inline void nv04_1quad(struct nouveau_context *nmesa,nouveauVertex* v0,nouveauVertex* v1,nouveauVertex* v2,nouveauVertex* v3)
{
	BEGIN_RING_SIZE(NvSub3D,NV04_DX5_TEXTURED_TRIANGLE_TLVERTEX_SX(0xC),33);
	OUT_RINGp(v0,8);
	OUT_RINGp(v1,8);
	OUT_RINGp(v2,8);
	OUT_RINGp(v3,8);
	OUT_RING(0xFECEDC);
}

static inline void nv04_render_points(GLcontext *ctx,GLuint first,GLuint last)
{
	WARN_ONCE("Unimplemented\n");
}

static inline void nv04_render_line(GLcontext *ctx,GLuint v1,GLuint v2)
{
	WARN_ONCE("Unimplemented\n");
}

static inline void nv04_render_triangle(GLcontext *ctx,GLuint v1,GLuint v2,GLuint v3)
{
	struct nouveau_context *nmesa = NOUVEAU_CONTEXT(ctx);
	GLubyte *vertptr = (GLubyte *)nmesa->verts;
	GLuint vertsize = nmesa->vertex_size;

	nv04_1triangle(nmesa,
			(nouveauVertex*)(vertptr+v1*vertsize),
			(nouveauVertex*)(vertptr+v2*vertsize),
			(nouveauVertex*)(vertptr+v3*vertsize)
		  );
}

static inline void nv04_render_quad(GLcontext *ctx,GLuint v1,GLuint v2,GLuint v3,GLuint v4)
{
	struct nouveau_context *nmesa = NOUVEAU_CONTEXT(ctx);
	GLubyte *vertptr = (GLubyte *)nmesa->verts;
	GLuint vertsize = nmesa->vertex_size;

	nv04_1quad(nmesa,
			(nouveauVertex*)(vertptr+v1*vertsize),
			(nouveauVertex*)(vertptr+v2*vertsize),
			(nouveauVertex*)(vertptr+v3*vertsize),
			(nouveauVertex*)(vertptr+v4*vertsize)
		  );
}

/**********************************************************************/
/*               Render unclipped begin/end objects                   */
/**********************************************************************/

static void nv04_render_points_verts(GLcontext *ctx,GLuint start,GLuint count,GLuint flags)
{
	// erm
}

static void nv04_render_lines_verts(GLcontext *ctx,GLuint start,GLuint count,GLuint flags)
{
	// umm
}

static void nv04_render_line_strip_verts(GLcontext *ctx,GLuint start,GLuint count,GLuint flags)
{
	// yeah
}

static void nv04_render_line_loop_verts(GLcontext *ctx,GLuint start,GLuint count,GLuint flags)
{
	// right
}

static void nv04_render_triangles_verts(GLcontext *ctx,GLuint start,GLuint count,GLuint flags)
{
	struct nouveau_context *nmesa = NOUVEAU_CONTEXT(ctx);
	GLubyte *vertptr = (GLubyte *)nmesa->verts;
	GLuint vertsize = nmesa->vertex_size;
	int i;

	for(i=start;i<count-5;i+=6)
		nv04_2triangles(nmesa,
				(nouveauVertex*)(vertptr+(i+0)*vertsize),
				(nouveauVertex*)(vertptr+(i+1)*vertsize),
				(nouveauVertex*)(vertptr+(i+2)*vertsize),
				(nouveauVertex*)(vertptr+(i+3)*vertsize),
				(nouveauVertex*)(vertptr+(i+4)*vertsize),
				(nouveauVertex*)(vertptr+(i+5)*vertsize)
			       );
	if (i!=count)
	{
		nv04_1triangle(nmesa,
				(nouveauVertex*)(vertptr+(i+0)*vertsize),
				(nouveauVertex*)(vertptr+(i+1)*vertsize),
				(nouveauVertex*)(vertptr+(i+2)*vertsize)
			       );
		i+=3;
	}
	if (i!=count)
		printf("oops\n");
}

static void nv04_render_tri_strip_verts(GLcontext *ctx,GLuint start,GLuint count,GLuint flags)
{
	struct nouveau_context *nmesa = NOUVEAU_CONTEXT(ctx);
	GLubyte *vertptr = (GLubyte *)nmesa->verts;
	GLuint vertsize = nmesa->vertex_size;
	uint32_t striptbl[]={0x321210,0x543432,0x765654,0x987876,0xBA9A98,0xDCBCBA,0xFEDEDC};
	int i,j;

	for(i=start;i<count;i+=14)
	{
		int numvert=MIN2(16,count-i);
		int numtri=numvert-2;
		if (numvert<3)
			break;

		BEGIN_RING_SIZE(NvSub3D,NV04_DX5_TEXTURED_TRIANGLE_TLVERTEX_SX(0x0),numvert*8);
		for(j=0;j<numvert;j++)
			OUT_RINGp((nouveauVertex*)(vertptr+(i+j)*vertsize),8);

		BEGIN_RING_SIZE(NvSub3D,NV04_DX5_TEXTURED_TRIANGLE_DRAW|NONINC_METHOD,(numtri+1)/2);
		for(j=0;j<numtri/2;j++)
			OUT_RING(striptbl[j]);
		if (numtri%2)
			OUT_RING(striptbl[numtri/2]&0xFFF);
	}
}

static void nv04_render_tri_fan_verts(GLcontext *ctx,GLuint start,GLuint count,GLuint flags)
{
	struct nouveau_context *nmesa = NOUVEAU_CONTEXT(ctx);
	GLubyte *vertptr = (GLubyte *)nmesa->verts;
	GLuint vertsize = nmesa->vertex_size;
	uint32_t fantbl[]={0x320210,0x540430,0x760650,0x980870,0xBA0A90,0xDC0CB0,0xFE0ED0};
	int i,j;

	BEGIN_RING_SIZE(NvSub3D,NV04_DX5_TEXTURED_TRIANGLE_TLVERTEX_SX(0x0),8);
	OUT_RINGp((nouveauVertex*)(vertptr+start*vertsize),8);

	for(i=start+1;i<count;i+=14)
	{
		int numvert=MIN2(15,count-i);
		int numtri=numvert-1;
		if (numvert<3)
			break;

		BEGIN_RING_SIZE(NvSub3D,NV04_DX5_TEXTURED_TRIANGLE_TLVERTEX_SX(0x1),numvert*8);

		for(j=0;j<numvert;j++)
			OUT_RINGp((nouveauVertex*)(vertptr+(i+j)*vertsize),8);

		BEGIN_RING_SIZE(NvSub3D,NV04_DX5_TEXTURED_TRIANGLE_DRAW|NONINC_METHOD,(numtri+1)/2);
		for(j=0;j<numtri/2;j++)
			OUT_RING(fantbl[j]);
		if (numtri%2)
			OUT_RING(fantbl[numtri/2]&0xFFF);
	}
}

static void nv04_render_quads_verts(GLcontext *ctx,GLuint start,GLuint count,GLuint flags)
{
	struct nouveau_context *nmesa = NOUVEAU_CONTEXT(ctx);
	GLubyte *vertptr = (GLubyte *)nmesa->verts;
	GLuint vertsize = nmesa->vertex_size;
	int i;

	for(i=start;i<count;i+=4)
		nv04_1quad(nmesa,
				(nouveauVertex*)(vertptr+(i+0)*vertsize),
				(nouveauVertex*)(vertptr+(i+1)*vertsize),
				(nouveauVertex*)(vertptr+(i+2)*vertsize),
				(nouveauVertex*)(vertptr+(i+3)*vertsize)
			       );
}

static void nv04_render_noop_verts(GLcontext *ctx,GLuint start,GLuint count,GLuint flags)
{
}

static void (*nv04_render_tab_verts[GL_POLYGON+2])(GLcontext *,
							   GLuint,
							   GLuint,
							   GLuint) =
{
   nv04_render_points_verts,
   nv04_render_lines_verts,
   nv04_render_line_loop_verts,
   nv04_render_line_strip_verts,
   nv04_render_triangles_verts,
   nv04_render_tri_strip_verts,
   nv04_render_tri_fan_verts,
   nv04_render_quads_verts,
   nv04_render_tri_strip_verts,  //nv04_render_quad_strip_verts
   nv04_render_tri_fan_verts,    //nv04_render_poly_verts
   nv04_render_noop_verts,
};


static void nv04_render_points_elts(GLcontext *ctx,GLuint start,GLuint count,GLuint flags)
{
	// erm
}

static void nv04_render_lines_elts(GLcontext *ctx,GLuint start,GLuint count,GLuint flags)
{
	// umm
}

static void nv04_render_line_strip_elts(GLcontext *ctx,GLuint start,GLuint count,GLuint flags)
{
	// yeah
}

static void nv04_render_line_loop_elts(GLcontext *ctx,GLuint start,GLuint count,GLuint flags)
{
	// right
}

static void nv04_render_triangles_elts(GLcontext *ctx,GLuint start,GLuint count,GLuint flags)
{
	struct nouveau_context *nmesa = NOUVEAU_CONTEXT(ctx);
	GLubyte *vertptr = (GLubyte *)nmesa->verts;
	GLuint vertsize = nmesa->vertex_size;
	const GLuint * const elt = TNL_CONTEXT(ctx)->vb.Elts;
	int i;

	for(i=start;i<count-5;i+=6)
		nv04_2triangles(nmesa,
				(nouveauVertex*)(vertptr+elt[i+0]*vertsize),
				(nouveauVertex*)(vertptr+elt[i+1]*vertsize),
				(nouveauVertex*)(vertptr+elt[i+2]*vertsize),
				(nouveauVertex*)(vertptr+elt[i+3]*vertsize),
				(nouveauVertex*)(vertptr+elt[i+4]*vertsize),
				(nouveauVertex*)(vertptr+elt[i+5]*vertsize)
			       );
	if (i!=count)
	{
		nv04_1triangle(nmesa,
				(nouveauVertex*)(vertptr+elt[i+0]*vertsize),
				(nouveauVertex*)(vertptr+elt[i+1]*vertsize),
				(nouveauVertex*)(vertptr+elt[i+2]*vertsize)
			       );
		i+=3;
	}
	if (i!=count)
		printf("oops\n");
}

static void nv04_render_tri_strip_elts(GLcontext *ctx,GLuint start,GLuint count,GLuint flags)
{
	struct nouveau_context *nmesa = NOUVEAU_CONTEXT(ctx);
	GLubyte *vertptr = (GLubyte *)nmesa->verts;
	GLuint vertsize = nmesa->vertex_size;
	uint32_t striptbl[]={0x321210,0x543432,0x765654,0x987876,0xBA9A98,0xDCBCBA,0xFEDEDC};
	const GLuint * const elt = TNL_CONTEXT(ctx)->vb.Elts;
	int i,j;

	for(i=start;i<count;i+=14)
	{
		int numvert=MIN2(16,count-i);
		int numtri=numvert-2;
		if (numvert<3)
			break;

		BEGIN_RING_SIZE(NvSub3D,NV04_DX5_TEXTURED_TRIANGLE_TLVERTEX_SX(0x0),numvert*8);
		for(j=0;j<numvert;j++)
			OUT_RINGp((nouveauVertex*)(vertptr+elt[i+j]*vertsize),8);

		BEGIN_RING_SIZE(NvSub3D,NV04_DX5_TEXTURED_TRIANGLE_DRAW|NONINC_METHOD,(numtri+1)/2);
		for(j=0;j<numtri/2;j++)
			OUT_RING(striptbl[j]);
		if (numtri%2)
			OUT_RING(striptbl[numtri/2]&0xFFF);
	}
}

static void nv04_render_tri_fan_elts(GLcontext *ctx,GLuint start,GLuint count,GLuint flags)
{
	struct nouveau_context *nmesa = NOUVEAU_CONTEXT(ctx);
	GLubyte *vertptr = (GLubyte *)nmesa->verts;
	GLuint vertsize = nmesa->vertex_size;
	uint32_t fantbl[]={0x320210,0x540430,0x760650,0x980870,0xBA0A90,0xDC0CB0,0xFE0ED0};
	const GLuint * const elt = TNL_CONTEXT(ctx)->vb.Elts;
	int i,j;

	BEGIN_RING_SIZE(NvSub3D,NV04_DX5_TEXTURED_TRIANGLE_TLVERTEX_SX(0x0),8);
	OUT_RINGp((nouveauVertex*)(vertptr+elt[start]*vertsize),8);

	for(i=start+1;i<count;i+=14)
	{
		int numvert=MIN2(15,count-i);
		int numtri=numvert-2;
		if (numvert<3)
			break;

		BEGIN_RING_SIZE(NvSub3D,NV04_DX5_TEXTURED_TRIANGLE_TLVERTEX_SX(0x1),numvert*8);

		for(j=0;j<numvert;j++)
			OUT_RINGp((nouveauVertex*)(vertptr+elt[i+j]*vertsize),8);

		BEGIN_RING_SIZE(NvSub3D,NV04_DX5_TEXTURED_TRIANGLE_DRAW|NONINC_METHOD,(numtri+1)/2);
		for(j=0;j<numtri/2;j++)
			OUT_RING(fantbl[j]);
		if (numtri%2)
			OUT_RING(fantbl[numtri/2]&0xFFF);
	}
}

static void nv04_render_quads_elts(GLcontext *ctx,GLuint start,GLuint count,GLuint flags)
{
	struct nouveau_context *nmesa = NOUVEAU_CONTEXT(ctx);
	GLubyte *vertptr = (GLubyte *)nmesa->verts;
	GLuint vertsize = nmesa->vertex_size;
	const GLuint * const elt = TNL_CONTEXT(ctx)->vb.Elts;
	int i;

	for(i=start;i<count;i+=4)
		nv04_1quad(nmesa,
				(nouveauVertex*)(vertptr+elt[i+0]*vertsize),
				(nouveauVertex*)(vertptr+elt[i+1]*vertsize),
				(nouveauVertex*)(vertptr+elt[i+2]*vertsize),
				(nouveauVertex*)(vertptr+elt[i+3]*vertsize)
			       );
}

static void nv04_render_noop_elts(GLcontext *ctx,GLuint start,GLuint count,GLuint flags)
{
}

static void (*nv04_render_tab_elts[GL_POLYGON+2])(GLcontext *,
							   GLuint,
							   GLuint,
							   GLuint) =
{
   nv04_render_points_elts,
   nv04_render_lines_elts,
   nv04_render_line_loop_elts,
   nv04_render_line_strip_elts,
   nv04_render_triangles_elts,
   nv04_render_tri_strip_elts,
   nv04_render_tri_fan_elts,
   nv04_render_quads_elts,
   nv04_render_tri_strip_elts,   // nv04_render_quad_strip_elts,
   nv04_render_tri_fan_elts,     // nv04_render_poly_elts,
   nv04_render_noop_elts,
};


/**********************************************************************/
/*                    Choose render functions                         */
/**********************************************************************/


#define EMIT_ATTR( ATTR, STYLE )					\
do {									\
   nmesa->vertex_attrs[nmesa->vertex_attr_count].attrib = (ATTR);	\
   nmesa->vertex_attrs[nmesa->vertex_attr_count].format = (STYLE);	\
   nmesa->vertex_attr_count++;						\
} while (0)

#define EMIT_PAD( N )							\
do {									\
   nmesa->vertex_attrs[nmesa->vertex_attr_count].attrib = 0;		\
   nmesa->vertex_attrs[nmesa->vertex_attr_count].format = EMIT_PAD;	\
   nmesa->vertex_attrs[nmesa->vertex_attr_count].offset = (N);		\
   nmesa->vertex_attr_count++;						\
} while (0)

static void nv04_render_clipped_line(GLcontext *ctx,GLuint ii,GLuint jj)
{
}

static void nv04_render_clipped_poly(GLcontext *ctx,const GLuint *elts,GLuint n)
{
}

static void nv04ChooseRenderState(GLcontext *ctx)
{
	TNLcontext *tnl = TNL_CONTEXT(ctx);

	tnl->Driver.Render.PrimTabVerts = nv04_render_tab_verts;
	tnl->Driver.Render.PrimTabElts = nv04_render_tab_elts;
	tnl->Driver.Render.ClippedLine = nv04_render_clipped_line;
	tnl->Driver.Render.ClippedPolygon = nv04_render_clipped_poly;
	tnl->Driver.Render.Points = nv04_render_points;
	tnl->Driver.Render.Line = nv04_render_line;
	tnl->Driver.Render.Triangle = nv04_render_triangle;
	tnl->Driver.Render.Quad = nv04_render_quad;
}



static inline void nv04OutputVertexFormat(struct nouveau_context* nmesa)
{
	GLcontext* ctx=nmesa->glCtx;
	DECLARE_RENDERINPUTS(index);

	/*
	 * Tell t_vertex about the vertex format
	 */
	nmesa->vertex_attr_count = 0;
	RENDERINPUTS_COPY(index, nmesa->render_inputs_bitset);

	// SX SY SZ INVW
	// FIXME : we use W instead of INVW, but since W=1 it doesn't matter
	if (RENDERINPUTS_TEST(index, _TNL_ATTRIB_POS))
		EMIT_ATTR(_TNL_ATTRIB_POS,EMIT_4F_VIEWPORT);
	else
		EMIT_PAD(4*sizeof(float));

	// COLOR
	if (RENDERINPUTS_TEST(index, _TNL_ATTRIB_COLOR0))
		EMIT_ATTR(_TNL_ATTRIB_COLOR0,EMIT_4UB_4F_ABGR);
	else
		EMIT_PAD(4);

	// SPECULAR
	if (RENDERINPUTS_TEST(index, _TNL_ATTRIB_COLOR1))
		EMIT_ATTR(_TNL_ATTRIB_COLOR1,EMIT_4UB_4F_ABGR);
	else
		EMIT_PAD(4);

	// TEXTURE
	if (RENDERINPUTS_TEST(index, _TNL_ATTRIB_TEX0))
		EMIT_ATTR(_TNL_ATTRIB_TEX0,EMIT_2F);
	else
		EMIT_PAD(2*sizeof(float));

	nmesa->vertex_size=_tnl_install_attrs( ctx,
			nmesa->vertex_attrs, 
			nmesa->vertex_attr_count,
			nmesa->viewport.m, 0 );
}


static void nv04ChooseVertexState( GLcontext *ctx )
{
	struct nouveau_context *nmesa = NOUVEAU_CONTEXT(ctx);
	TNLcontext *tnl = TNL_CONTEXT(ctx);
	DECLARE_RENDERINPUTS(index);

	RENDERINPUTS_COPY(index, tnl->render_inputs_bitset);
	if (!RENDERINPUTS_EQUAL(index, nmesa->render_inputs_bitset))
	{
		RENDERINPUTS_COPY(nmesa->render_inputs_bitset, index);
		nv04OutputVertexFormat(nmesa);
	}
}


/**********************************************************************/
/*                 High level hooks for t_vb_render.c                 */
/**********************************************************************/


static void nv04RenderStart(GLcontext *ctx)
{
	struct nouveau_context *nmesa = NOUVEAU_CONTEXT(ctx);

	if (nmesa->new_state) {
		nmesa->new_render_state |= nmesa->new_state;
	}

	if (nmesa->new_render_state) {
		nv04ChooseVertexState(ctx);
		nv04ChooseRenderState(ctx);
		nmesa->new_render_state = 0;
	}
}

static void nv04RenderFinish(GLcontext *ctx)
{
}


/* System to flush dma and emit state changes based on the rasterized
 * primitive.
 */
void nv04RasterPrimitive(GLcontext *ctx,
		GLenum glprim,
		GLuint hwprim)
{
	struct nouveau_context *nmesa = NOUVEAU_CONTEXT(ctx);

	assert (!nmesa->new_state);

	if (hwprim != nmesa->current_primitive)
	{
		nmesa->current_primitive=hwprim;
		
	}
}

static const GLuint hw_prim[GL_POLYGON+1] = {
	GL_POINTS+1,
	GL_LINES+1,
	GL_LINE_STRIP+1,
	GL_LINE_LOOP+1,
	GL_TRIANGLES+1,
	GL_TRIANGLE_STRIP+1,
	GL_TRIANGLE_FAN+1,
	GL_QUADS+1,
	GL_QUAD_STRIP+1,
	GL_POLYGON+1
};

/* Callback for mesa:
 */
static void nv04RenderPrimitive( GLcontext *ctx, GLuint prim )
{
	nv04RasterPrimitive( ctx, prim, hw_prim[prim] );
}

static void nv04ResetLineStipple( GLcontext *ctx )
{
	/* FIXME do something here */
	WARN_ONCE("Unimplemented nv04ResetLineStipple\n");
}


/**********************************************************************/
/*                            Initialization.                         */
/**********************************************************************/

void nv04TriInitFunctions(GLcontext *ctx)
{
	struct nouveau_context *nmesa = NOUVEAU_CONTEXT(ctx);
	TNLcontext *tnl = TNL_CONTEXT(ctx);

	tnl->Driver.RunPipeline = nouveauRunPipeline;
	tnl->Driver.Render.Start = nv04RenderStart;
	tnl->Driver.Render.Finish = nv04RenderFinish;
	tnl->Driver.Render.PrimitiveNotify = nv04RenderPrimitive;
	tnl->Driver.Render.ResetLineStipple = nv04ResetLineStipple;
	tnl->Driver.Render.BuildVertices = _tnl_build_vertices;
	tnl->Driver.Render.CopyPV = _tnl_copy_pv;
	tnl->Driver.Render.Interp = _tnl_interp;

	_tnl_init_vertices( ctx, ctx->Const.MaxArrayLockSize + 12, 32 );

	nmesa->verts = (GLubyte *)tnl->clipspace.vertex_buf;
}


