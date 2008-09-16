/*
 * Copyright 1998-2003 VIA Technologies, Inc. All Rights Reserved.
 * Copyright 2001-2003 S3 Graphics, Inc. All Rights Reserved.
 * Copyright 2006 Stephane Marchesin. All Rights Reserved.
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

/* Software TCL for NV10, NV20, NV30, NV40, NV50 */

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
#include "nv10_swtcl.h"
#include "nouveau_context.h"
#include "nouveau_span.h"
#include "nouveau_reg.h"
#include "nouveau_tex.h"
#include "nouveau_fifo.h"
#include "nouveau_msg.h"
#include "nouveau_object.h"

static void nv10RasterPrimitive( GLcontext *ctx, GLenum rprim, GLuint hwprim );
static void nv10RenderPrimitive( GLcontext *ctx, GLenum prim );
static void nv10ResetLineStipple( GLcontext *ctx );



static inline void nv10StartPrimitive(struct nouveau_context* nmesa,uint32_t primitive,uint32_t size)
{
	if (nmesa->screen->card->type==NV_10)
		BEGIN_RING_SIZE(NvSub3D,NV10_TCL_PRIMITIVE_3D_BEGIN_END,1);
	else if (nmesa->screen->card->type==NV_20)
		BEGIN_RING_SIZE(NvSub3D,NV20_TCL_PRIMITIVE_3D_BEGIN_END,1);
	else
		BEGIN_RING_SIZE(NvSub3D,NV30_TCL_PRIMITIVE_3D_BEGIN_END,1);
	OUT_RING(primitive);

	if (nmesa->screen->card->type==NV_10)
		BEGIN_RING_SIZE(NvSub3D,NV10_TCL_PRIMITIVE_3D_VERTEX_ARRAY_DATA|NONINC_METHOD,size);
	else if (nmesa->screen->card->type==NV_20)
		BEGIN_RING_SIZE(NvSub3D,NV20_TCL_PRIMITIVE_3D_VERTEX_DATA|NONINC_METHOD,size);
	else
		BEGIN_RING_SIZE(NvSub3D,NV30_TCL_PRIMITIVE_3D_VERTEX_DATA|NONINC_METHOD,size);
}

inline void nv10FinishPrimitive(struct nouveau_context *nmesa)
{
	if (nmesa->screen->card->type==NV_10)
		BEGIN_RING_SIZE(NvSub3D,NV10_TCL_PRIMITIVE_3D_BEGIN_END,1);
	else if (nmesa->screen->card->type==NV_20)
		BEGIN_RING_SIZE(NvSub3D,NV20_TCL_PRIMITIVE_3D_BEGIN_END,1);
	else
		BEGIN_RING_SIZE(NvSub3D,NV30_TCL_PRIMITIVE_3D_BEGIN_END,1);
	OUT_RING(0x0);
	FIRE_RING();
}


static inline void nv10ExtendPrimitive(struct nouveau_context* nmesa, int size)
{
	/* make sure there's enough room. if not, wait */
	if (RING_AVAILABLE()<size)
	{
		WAIT_RING(nmesa,size);
	}
}

/**********************************************************************/
/*               Render unclipped begin/end objects                   */
/**********************************************************************/

static inline void nv10_render_generic_primitive_verts(GLcontext *ctx,GLuint start,GLuint count,GLuint flags,GLuint prim)
{
	struct nouveau_context *nmesa = NOUVEAU_CONTEXT(ctx);
	GLubyte *vertptr = (GLubyte *)nmesa->verts;
	GLuint vertsize = nmesa->vertex_size;
	GLuint size_dword = vertsize*(count-start)/4;

	nv10ExtendPrimitive(nmesa, size_dword);
	nv10StartPrimitive(nmesa,prim+1,size_dword);
	OUT_RINGp((nouveauVertex*)(vertptr+(start*vertsize)),size_dword);
	nv10FinishPrimitive(nmesa);
}

static void nv10_render_points_verts(GLcontext *ctx,GLuint start,GLuint count,GLuint flags)
{
	nv10_render_generic_primitive_verts(ctx,start,count,flags,GL_POINTS);
}

static void nv10_render_lines_verts(GLcontext *ctx,GLuint start,GLuint count,GLuint flags)
{
	nv10_render_generic_primitive_verts(ctx,start,count,flags,GL_LINES);
}

static void nv10_render_line_strip_verts(GLcontext *ctx,GLuint start,GLuint count,GLuint flags)
{
	nv10_render_generic_primitive_verts(ctx,start,count,flags,GL_LINE_STRIP);
}

static void nv10_render_line_loop_verts(GLcontext *ctx,GLuint start,GLuint count,GLuint flags)
{
	nv10_render_generic_primitive_verts(ctx,start,count,flags,GL_LINE_LOOP);
}

static void nv10_render_triangles_verts(GLcontext *ctx,GLuint start,GLuint count,GLuint flags)
{
	nv10_render_generic_primitive_verts(ctx,start,count,flags,GL_TRIANGLES);
}

static void nv10_render_tri_strip_verts(GLcontext *ctx,GLuint start,GLuint count,GLuint flags)
{
	nv10_render_generic_primitive_verts(ctx,start,count,flags,GL_TRIANGLE_STRIP);
}

static void nv10_render_tri_fan_verts(GLcontext *ctx,GLuint start,GLuint count,GLuint flags)
{
	nv10_render_generic_primitive_verts(ctx,start,count,flags,GL_TRIANGLE_FAN);
}

static void nv10_render_quads_verts(GLcontext *ctx,GLuint start,GLuint count,GLuint flags)
{
	nv10_render_generic_primitive_verts(ctx,start,count,flags,GL_QUADS);
}

static void nv10_render_quad_strip_verts(GLcontext *ctx,GLuint start,GLuint count,GLuint flags)
{
	nv10_render_generic_primitive_verts(ctx,start,count,flags,GL_QUAD_STRIP);
}

static void nv10_render_poly_verts(GLcontext *ctx,GLuint start,GLuint count,GLuint flags)
{
	nv10_render_generic_primitive_verts(ctx,start,count,flags,GL_POLYGON);
}

static void nv10_render_noop_verts(GLcontext *ctx,GLuint start,GLuint count,GLuint flags)
{
}

static void (*nv10_render_tab_verts[GL_POLYGON+2])(GLcontext *,
							   GLuint,
							   GLuint,
							   GLuint) =
{
   nv10_render_points_verts,
   nv10_render_lines_verts,
   nv10_render_line_loop_verts,
   nv10_render_line_strip_verts,
   nv10_render_triangles_verts,
   nv10_render_tri_strip_verts,
   nv10_render_tri_fan_verts,
   nv10_render_quads_verts,
   nv10_render_quad_strip_verts,
   nv10_render_poly_verts,
   nv10_render_noop_verts,
};


static inline void nv10_render_generic_primitive_elts(GLcontext *ctx,GLuint start,GLuint count,GLuint flags,GLuint prim)
{
	struct nouveau_context *nmesa = NOUVEAU_CONTEXT(ctx);
	GLubyte *vertptr = (GLubyte *)nmesa->verts;
	GLuint vertsize = nmesa->vertex_size;
	GLuint size_dword = vertsize*(count-start)/4;
	const GLuint * const elt = TNL_CONTEXT(ctx)->vb.Elts;
	GLuint j;

	nv10ExtendPrimitive(nmesa, size_dword);
	nv10StartPrimitive(nmesa,prim+1,size_dword);
	for (j=start; j<count; j++ ) {
		OUT_RINGp((nouveauVertex*)(vertptr+(elt[j]*vertsize)),vertsize/4);
	}
	nv10FinishPrimitive(nmesa);
}

static void nv10_render_points_elts(GLcontext *ctx,GLuint start,GLuint count,GLuint flags)
{
	nv10_render_generic_primitive_elts(ctx,start,count,flags,GL_POINTS);
}

static void nv10_render_lines_elts(GLcontext *ctx,GLuint start,GLuint count,GLuint flags)
{
	nv10_render_generic_primitive_elts(ctx,start,count,flags,GL_LINES);
}

static void nv10_render_line_strip_elts(GLcontext *ctx,GLuint start,GLuint count,GLuint flags)
{
	nv10_render_generic_primitive_elts(ctx,start,count,flags,GL_LINE_STRIP);
}

static void nv10_render_line_loop_elts(GLcontext *ctx,GLuint start,GLuint count,GLuint flags)
{
	nv10_render_generic_primitive_elts(ctx,start,count,flags,GL_LINE_LOOP);
}

static void nv10_render_triangles_elts(GLcontext *ctx,GLuint start,GLuint count,GLuint flags)
{
	nv10_render_generic_primitive_elts(ctx,start,count,flags,GL_TRIANGLES);
}

static void nv10_render_tri_strip_elts(GLcontext *ctx,GLuint start,GLuint count,GLuint flags)
{
	nv10_render_generic_primitive_elts(ctx,start,count,flags,GL_TRIANGLE_STRIP);
}

static void nv10_render_tri_fan_elts(GLcontext *ctx,GLuint start,GLuint count,GLuint flags)
{
	nv10_render_generic_primitive_elts(ctx,start,count,flags,GL_TRIANGLE_FAN);
}

static void nv10_render_quads_elts(GLcontext *ctx,GLuint start,GLuint count,GLuint flags)
{
	nv10_render_generic_primitive_elts(ctx,start,count,flags,GL_QUADS);
}

static void nv10_render_quad_strip_elts(GLcontext *ctx,GLuint start,GLuint count,GLuint flags)
{
	nv10_render_generic_primitive_elts(ctx,start,count,flags,GL_QUAD_STRIP);
}

static void nv10_render_poly_elts(GLcontext *ctx,GLuint start,GLuint count,GLuint flags)
{
	nv10_render_generic_primitive_elts(ctx,start,count,flags,GL_POLYGON);
}

static void nv10_render_noop_elts(GLcontext *ctx,GLuint start,GLuint count,GLuint flags)
{
}

static void (*nv10_render_tab_elts[GL_POLYGON+2])(GLcontext *,
							   GLuint,
							   GLuint,
							   GLuint) =
{
   nv10_render_points_elts,
   nv10_render_lines_elts,
   nv10_render_line_loop_elts,
   nv10_render_line_strip_elts,
   nv10_render_triangles_elts,
   nv10_render_tri_strip_elts,
   nv10_render_tri_fan_elts,
   nv10_render_quads_elts,
   nv10_render_quad_strip_elts,
   nv10_render_poly_elts,
   nv10_render_noop_elts,
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

static void nv10_render_clipped_line(GLcontext *ctx,GLuint ii,GLuint jj)
{

}

static void nv10_render_clipped_poly(GLcontext *ctx,const GLuint *elts,GLuint n)
{
	TNLcontext *tnl = TNL_CONTEXT(ctx);
	struct vertex_buffer *VB = &tnl->vb;
	GLuint *tmp = VB->Elts;
	VB->Elts = (GLuint *)elts;
	nv10_render_generic_primitive_elts( ctx, 0, n, PRIM_BEGIN|PRIM_END,GL_POLYGON );
	VB->Elts = tmp;
}

static inline void nv10_render_points(GLcontext *ctx,GLuint first,GLuint last)
{
	WARN_ONCE("Unimplemented\n");
}

static inline void nv10_render_line(GLcontext *ctx,GLuint v1,GLuint v2)
{
	struct nouveau_context *nmesa = NOUVEAU_CONTEXT(ctx);
	GLubyte *vertptr = (GLubyte *)nmesa->verts;
	GLuint vertsize = nmesa->vertex_size;
	GLuint size_dword = vertsize*(2)/4;

	/* OUT_RINGp wants size in DWORDS */
	vertsize >>= 2;

	nv10ExtendPrimitive(nmesa, size_dword);
	nv10StartPrimitive(nmesa,GL_LINES+1,size_dword);
	OUT_RINGp((nouveauVertex*)(vertptr+(v1*vertsize)),vertsize);
	OUT_RINGp((nouveauVertex*)(vertptr+(v2*vertsize)),vertsize);
	nv10FinishPrimitive(nmesa);
}

static inline void nv10_render_triangle(GLcontext *ctx,GLuint v1,GLuint v2,GLuint v3)
{
	struct nouveau_context *nmesa = NOUVEAU_CONTEXT(ctx);
	GLubyte *vertptr = (GLubyte *)nmesa->verts;
	GLuint vertsize = nmesa->vertex_size;
	GLuint size_dword = vertsize*(3)/4;

	/* OUT_RINGp wants size in DWORDS */
	vertsize >>= 2;

	nv10ExtendPrimitive(nmesa, size_dword);
	nv10StartPrimitive(nmesa,GL_TRIANGLES+1,size_dword);
	OUT_RINGp((nouveauVertex*)(vertptr+(v1*vertsize)),vertsize);
	OUT_RINGp((nouveauVertex*)(vertptr+(v2*vertsize)),vertsize);
	OUT_RINGp((nouveauVertex*)(vertptr+(v3*vertsize)),vertsize);
	nv10FinishPrimitive(nmesa);
}

static inline void nv10_render_quad(GLcontext *ctx,GLuint v1,GLuint v2,GLuint v3,GLuint v4)
{
	struct nouveau_context *nmesa = NOUVEAU_CONTEXT(ctx);
	GLubyte *vertptr = (GLubyte *)nmesa->verts;
	GLuint vertsize = nmesa->vertex_size;
	GLuint size_dword = vertsize*(4)/4;

	/* OUT_RINGp wants size in DWORDS */
	vertsize >>= 2;

	nv10ExtendPrimitive(nmesa, size_dword);
	nv10StartPrimitive(nmesa,GL_QUADS+1,size_dword);
	OUT_RINGp((nouveauVertex*)(vertptr+(v1*vertsize)),vertsize);
	OUT_RINGp((nouveauVertex*)(vertptr+(v2*vertsize)),vertsize);
	OUT_RINGp((nouveauVertex*)(vertptr+(v3*vertsize)),vertsize);
	OUT_RINGp((nouveauVertex*)(vertptr+(v4*vertsize)),vertsize);
	nv10FinishPrimitive(nmesa);
}



static void nv10ChooseRenderState(GLcontext *ctx)
{
	TNLcontext *tnl = TNL_CONTEXT(ctx);
	struct nouveau_context *nmesa = NOUVEAU_CONTEXT(ctx);

	tnl->Driver.Render.PrimTabVerts = nv10_render_tab_verts;
	tnl->Driver.Render.PrimTabElts = nv10_render_tab_elts;
	tnl->Driver.Render.ClippedLine = nv10_render_clipped_line;
	tnl->Driver.Render.ClippedPolygon = nv10_render_clipped_poly;
	tnl->Driver.Render.Points = nv10_render_points;
	tnl->Driver.Render.Line = nv10_render_line;
	tnl->Driver.Render.Triangle = nv10_render_triangle;
	tnl->Driver.Render.Quad = nv10_render_quad;
}



static inline void nv10OutputVertexFormat(struct nouveau_context* nmesa)
{
	GLcontext* ctx=nmesa->glCtx;
	TNLcontext *tnl = TNL_CONTEXT(ctx);
	DECLARE_RENDERINPUTS(index);
	struct vertex_buffer *VB = &tnl->vb;
	int attr_size[16];
	int default_attr_size[8]={3,3,3,4,3,1,4,4};
	int i;
	int slots=0;
	int total_size=0;
	/* t_vertex_generic dereferences a NULL pointer if we
	 * pass NULL as the vp transform...
	 */
	const GLfloat ident_vp[16] = {
	   1.0, 0.0, 0.0, 0.0,
	   0.0, 1.0, 0.0, 0.0,
	   0.0, 0.0, 1.0, 0.0,
	   0.0, 0.0, 0.0, 1.0
	};

	nmesa->vertex_attr_count = 0;
	RENDERINPUTS_COPY(index, nmesa->render_inputs_bitset);

	/*
	 * Determine attribute sizes
	 */
	for(i=0;i<8;i++)
	{
		if (RENDERINPUTS_TEST(index, i))
			attr_size[i]=default_attr_size[i];
		else
			attr_size[i]=0;
	}
	for(i=8;i<16;i++)
	{
		if (RENDERINPUTS_TEST(index, i))
			attr_size[i]=VB->TexCoordPtr[i-8]->size;
		else
			attr_size[i]=0;
	}

	/*
	 * Tell t_vertex about the vertex format
	 */
	for(i=0;i<16;i++)
	{
		if (RENDERINPUTS_TEST(index, i))
		{
			slots=i+1;
			if (i==_TNL_ATTRIB_POS)
			{
				/* special-case POS */
				EMIT_ATTR(_TNL_ATTRIB_POS,EMIT_3F_VIEWPORT);
			}
			else
			{
				switch(attr_size[i])
				{
					case 1:
						EMIT_ATTR(i,EMIT_1F);
						break;
					case 2:
						EMIT_ATTR(i,EMIT_2F);
						break;
					case 3:
						EMIT_ATTR(i,EMIT_3F);
						break;
					case 4:
						EMIT_ATTR(i,EMIT_4F);
						break;
				}
			}
			if (i==_TNL_ATTRIB_COLOR0)
				nmesa->color_offset=total_size;
			if (i==_TNL_ATTRIB_COLOR1)
				nmesa->specular_offset=total_size;
			total_size+=attr_size[i];
		}
	}

	nmesa->vertex_size=_tnl_install_attrs( ctx,
			nmesa->vertex_attrs, 
			nmesa->vertex_attr_count,
			ident_vp, 0 );
	assert(nmesa->vertex_size==total_size*4);

	/* 
	 * Tell the hardware about the vertex format
	 */
	if (nmesa->screen->card->type==NV_10) {
		int size;

#define NV_VERTEX_ATTRIBUTE_TYPE_FLOAT 2

#define NV10_SET_VERTEX_ATTRIB(i,j) \
	do {	\
		size = attr_size[j] << 4;	\
		size |= (attr_size[j]*4) << 8;	\
		size |= NV_VERTEX_ATTRIBUTE_TYPE_FLOAT;	\
		BEGIN_RING_CACHE(NvSub3D, NV10_TCL_PRIMITIVE_3D_VERTEX_ATTR(i),1);	\
		OUT_RING_CACHE(size);	\
	} while (0)

		NV10_SET_VERTEX_ATTRIB(0, _TNL_ATTRIB_POS);
		NV10_SET_VERTEX_ATTRIB(1, _TNL_ATTRIB_COLOR0);
		NV10_SET_VERTEX_ATTRIB(2, _TNL_ATTRIB_COLOR1);
		NV10_SET_VERTEX_ATTRIB(3, _TNL_ATTRIB_TEX0);
		NV10_SET_VERTEX_ATTRIB(4, _TNL_ATTRIB_TEX1);
		NV10_SET_VERTEX_ATTRIB(5, _TNL_ATTRIB_NORMAL);
		NV10_SET_VERTEX_ATTRIB(6, _TNL_ATTRIB_WEIGHT);
		NV10_SET_VERTEX_ATTRIB(7, _TNL_ATTRIB_FOG);

		BEGIN_RING_CACHE(NvSub3D, NV10_TCL_PRIMITIVE_3D_VERTEX_ARRAY_VALIDATE,1);
		OUT_RING_CACHE(0);
	} else if (nmesa->screen->card->type==NV_20) {
		for(i=0;i<16;i++)
		{
			int size=attr_size[i];
			BEGIN_RING_CACHE(NvSub3D,NV20_TCL_PRIMITIVE_3D_VERTEX_ATTR(i),1);
			OUT_RING_CACHE(NV_VERTEX_ATTRIBUTE_TYPE_FLOAT|(size*0x10));
		}
	} else {
		BEGIN_RING_SIZE(NvSub3D, NV30_TCL_PRIMITIVE_3D_DO_VERTICES, 1);
		OUT_RING(0);
		BEGIN_RING_CACHE(NvSub3D,NV30_TCL_PRIMITIVE_3D_VERTEX_ATTR0_POS,slots);
		for(i=0;i<slots;i++)
		{
			int size=attr_size[i];
			OUT_RING_CACHE(NV_VERTEX_ATTRIBUTE_TYPE_FLOAT|(size*0x10));
		}
		// FIXME this is probably not needed
		BEGIN_RING_SIZE(NvSub3D,NV30_TCL_PRIMITIVE_3D_VERTEX_UNK_0,1);
		OUT_RING(0);
		BEGIN_RING_SIZE(NvSub3D,NV30_TCL_PRIMITIVE_3D_VERTEX_UNK_0,1);
		OUT_RING(0);
		BEGIN_RING_SIZE(NvSub3D,NV30_TCL_PRIMITIVE_3D_VERTEX_UNK_0,1);
		OUT_RING(0);
	}
}


static void nv10ChooseVertexState( GLcontext *ctx )
{
	struct nouveau_context *nmesa = NOUVEAU_CONTEXT(ctx);
	TNLcontext *tnl = TNL_CONTEXT(ctx);
	DECLARE_RENDERINPUTS(index);
	
	RENDERINPUTS_COPY(index, tnl->render_inputs_bitset);
	if (!RENDERINPUTS_EQUAL(index, nmesa->render_inputs_bitset))
	{
		RENDERINPUTS_COPY(nmesa->render_inputs_bitset, index);
		nv10OutputVertexFormat(nmesa);
	}

	if (nmesa->screen->card->type == NV_30) {
		nouveauShader *fp;
		
		if (ctx->FragmentProgram.Enabled) {
			fp = (nouveauShader *) ctx->FragmentProgram.Current;
			nvsUpdateShader(ctx, fp);
		} else
			nvsUpdateShader(ctx, nmesa->passthrough_fp);
	}

	if (nmesa->screen->card->type >= NV_40) {
		/* Ensure passthrough shader is being used, and mvp matrix
		 * is up to date
		 */
		nvsUpdateShader(ctx, nmesa->passthrough_vp);

		/* Update texenv shader / user fragprog */
		nvsUpdateShader(ctx, (nouveauShader*)ctx->FragmentProgram._Current);
	}
}


/**********************************************************************/
/*                 High level hooks for t_vb_render.c                 */
/**********************************************************************/


static void nv10RenderStart(GLcontext *ctx)
{
	TNLcontext *tnl = TNL_CONTEXT(ctx);
	struct nouveau_context *nmesa = NOUVEAU_CONTEXT(ctx);

	if (nmesa->new_state) {
		nmesa->new_render_state |= nmesa->new_state;
	}

	if (nmesa->new_render_state) {
		nv10ChooseVertexState(ctx);
		nv10ChooseRenderState(ctx);
		nmesa->new_render_state = 0;
	}
}

static void nv10RenderFinish(GLcontext *ctx)
{
}


/* System to flush dma and emit state changes based on the rasterized
 * primitive.
 */
void nv10RasterPrimitive(GLcontext *ctx,
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
static void nv10RenderPrimitive( GLcontext *ctx, GLuint prim )
{
	nv10RasterPrimitive( ctx, prim, hw_prim[prim] );
}

static void nv10ResetLineStipple( GLcontext *ctx )
{
	/* FIXME do something here */
	WARN_ONCE("Unimplemented nv10ResetLineStipple\n");
}


/**********************************************************************/
/*                            Initialization.                         */
/**********************************************************************/

void nv10TriInitFunctions(GLcontext *ctx)
{
	struct nouveau_context *nmesa = NOUVEAU_CONTEXT(ctx);
	TNLcontext *tnl = TNL_CONTEXT(ctx);

	tnl->Driver.RunPipeline = nouveauRunPipeline;
	tnl->Driver.Render.Start = nv10RenderStart;
	tnl->Driver.Render.Finish = nv10RenderFinish;
	tnl->Driver.Render.PrimitiveNotify = nv10RenderPrimitive;
	tnl->Driver.Render.ResetLineStipple = nv10ResetLineStipple;
	tnl->Driver.Render.BuildVertices = _tnl_build_vertices;
	tnl->Driver.Render.CopyPV = _tnl_copy_pv;
	tnl->Driver.Render.Interp = _tnl_interp;

	_tnl_init_vertices( ctx, ctx->Const.MaxArrayLockSize + 12, 
			64 * sizeof(GLfloat) );

	nmesa->verts = (GLubyte *)tnl->clipspace.vertex_buf;
}


