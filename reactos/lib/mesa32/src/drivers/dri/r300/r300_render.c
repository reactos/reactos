/**************************************************************************

Copyright (C) 2004 Nicolai Haehnle.

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
ATI, VA LINUX SYSTEMS AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/

/*
 * Authors:
 *   Nicolai Haehnle <prefect_@gmx.net>
 */

#include "glheader.h"
#include "state.h"
#include "imports.h"
#include "enums.h"
#include "macros.h"
#include "context.h"
#include "dd.h"
#include "simple_list.h"

#include "api_arrayelt.h"
#include "swrast/swrast.h"
#include "swrast_setup/swrast_setup.h"
#include "array_cache/acache.h"
#include "tnl/tnl.h"
#include "tnl/t_vp_build.h"

#include "radeon_reg.h"
#include "radeon_macros.h"
#include "radeon_ioctl.h"
#include "radeon_state.h"
#include "r300_context.h"
#include "r300_ioctl.h"
#include "r300_state.h"
#include "r300_reg.h"
#include "r300_program.h"
#include "r300_tex.h"
#include "r300_maos.h"
#include "r300_emit.h"

extern int future_hw_tcl_on;

/**********************************************************************
*                     Hardware rasterization
*
* When we fell back to software TCL, we still try to use the
* rasterization hardware for rendering.
**********************************************************************/

static int r300_get_primitive_type(r300ContextPtr rmesa, GLcontext *ctx, int prim)
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   struct vertex_buffer *VB = &tnl->vb;
   GLuint i;
   int type=-1;

	switch (prim & PRIM_MODE_MASK) {
	case GL_POINTS:
	        type=R300_VAP_VF_CNTL__PRIM_POINTS;
      		break;
	case GL_LINES:
	        type=R300_VAP_VF_CNTL__PRIM_LINES;
      		break;
	case GL_LINE_STRIP:
	        type=R300_VAP_VF_CNTL__PRIM_LINE_STRIP;
      		break;
	case GL_LINE_LOOP:
		type=R300_VAP_VF_CNTL__PRIM_LINE_LOOP;
      		break;
    	case GL_TRIANGLES:
	        type=R300_VAP_VF_CNTL__PRIM_TRIANGLES;
      		break;
   	case GL_TRIANGLE_STRIP:
	        type=R300_VAP_VF_CNTL__PRIM_TRIANGLE_STRIP;
      		break;
   	case GL_TRIANGLE_FAN:
	        type=R300_VAP_VF_CNTL__PRIM_TRIANGLE_FAN;
      		break;
	case GL_QUADS:
	        type=R300_VAP_VF_CNTL__PRIM_QUADS;
      		break;
	case GL_QUAD_STRIP:
	        type=R300_VAP_VF_CNTL__PRIM_QUAD_STRIP;
      		break;
	case GL_POLYGON:
		type=R300_VAP_VF_CNTL__PRIM_POLYGON;
		break;
   	default:
 		fprintf(stderr, "%s:%s Do not know how to handle primitive %02x - help me !\n",
			__FILE__, __FUNCTION__,
			prim & PRIM_MODE_MASK);
		return -1;
         	break;
   	}
   return type;
}

static int r300_get_num_verts(r300ContextPtr rmesa,
	GLcontext *ctx,
	int num_verts,
	int prim)
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   struct vertex_buffer *VB = &tnl->vb;
   GLuint i;
   int type=-1, verts_off=0;
   char *name="UNKNOWN";

	switch (prim & PRIM_MODE_MASK) {
	case GL_POINTS:
   		name="P";
		verts_off = 0;
      		break;
	case GL_LINES:
   		name="L";
		verts_off = num_verts % 2;
      		break;
	case GL_LINE_STRIP:
   		name="LS";
		if(num_verts < 2)
			verts_off = num_verts;
      		break;
	case GL_LINE_LOOP:
   		name="LL";
		if(num_verts < 2)
			verts_off = num_verts;
      		break;
    	case GL_TRIANGLES:
   		name="T";
		verts_off = num_verts % 3;
      		break;
   	case GL_TRIANGLE_STRIP:
   		name="TS";
		if(num_verts < 3)
			verts_off = num_verts;
      		break;
   	case GL_TRIANGLE_FAN:
   		name="TF";
		if(num_verts < 3)
			verts_off = num_verts;
      		break;
	case GL_QUADS:
   		name="Q";
		verts_off = num_verts % 4;
      		break;
	case GL_QUAD_STRIP:
   		name="QS";
		if(num_verts < 4)
			verts_off = num_verts;
		else
			verts_off = num_verts % 2;
      		break;
	case GL_POLYGON:
		name="P";
		if(num_verts < 3)
			verts_off = num_verts;
		break;
   	default:
 		fprintf(stderr, "%s:%s Do not know how to handle primitive %02x - help me !\n",
			__FILE__, __FUNCTION__,
			prim & PRIM_MODE_MASK);
		return -1;
         	break;
   	}

	if(num_verts - verts_off == 0){
		WARN_ONCE("user error: Need more than %d vertices to draw primitive %s !\n", num_verts, name);
		return 0;
	}

	if(verts_off > 0){
		WARN_ONCE("user error: %d is not a valid number of vertices for primitive %s !\n", num_verts, name);
	}

	return num_verts - verts_off;
}

/* This function compiles GL context into state registers that
   describe data routing inside of R300 pipeline.

   In particular, it programs input_route, output_vtx_fmt, texture
   unit configuration and gb_output_vtx_fmt

   This function encompasses setup_AOS() from r300_lib.c
*/




/* Immediate implementation - vertex data is sent via command stream */

static GLfloat default_vector[4]={0.0, 0.0, 0.0, 1.0};

#define output_vector(v, i) { \
	int _i; \
	for(_i=0;_i<v->size;_i++){ \
		if(VB->Elts){ \
			efloat(VEC_ELT(v, GLfloat, VB->Elts[i])[_i]); \
		}else{ \
			efloat(VEC_ELT(v, GLfloat, i)[_i]); \
		} \
	} \
	for(_i=v->size;_i<4;_i++){ \
		efloat(default_vector[_i]); \
	} \
}

/* Immediate implementation - vertex data is sent via command stream */

static void r300_render_immediate_primitive(r300ContextPtr rmesa,
	GLcontext *ctx,
	int start,
	int end,
	int prim)
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   struct vertex_buffer *VB = &tnl->vb;
   GLuint i, render_inputs;
   int k, type, num_verts;
   LOCAL_VARS

   type=r300_get_primitive_type(rmesa, ctx, prim);
   num_verts=r300_get_num_verts(rmesa, ctx, end-start, prim);

#if 0
		fprintf(stderr,"ObjPtr: size=%d stride=%d\n",
			VB->ObjPtr->size, VB->ObjPtr->stride);
		fprintf(stderr,"ColorPtr[0]: size=%d stride=%d\n",
			VB->ColorPtr[0]->size, VB->ColorPtr[0]->stride);
		fprintf(stderr,"TexCoordPtr[0]: size=%d stride=%d\n",
			VB->TexCoordPtr[0]->size, VB->TexCoordPtr[0]->stride);
#endif

   if(type<0 || num_verts <= 0)return;

   if(!VB->ObjPtr){
   	WARN_ONCE("FIXME: Don't know how to handle GL_ARB_vertex_buffer_object correctly\n");
   	return;
   }
   /* A packet cannot have more than 16383 data words.. */
   if((num_verts*4*rmesa->state.aos_count)>16380){
   	WARN_ONCE("Too many vertices to paint. Fix me !\n");
	return;
	}

   //fprintf(stderr, "aos_count=%d start=%d end=%d\n", rmesa->state.aos_count, start, end);

   if(rmesa->state.aos_count==0){
   	WARN_ONCE("Aeiee ! aos_count==0, while it shouldn't. Skipping rendering\n");
	return;
   	}

   render_inputs = rmesa->state.render_inputs;

   if(!render_inputs){
   	WARN_ONCE("Aeiee ! render_inputs==0. Skipping rendering.\n");
	return;
   	}


   start_immediate_packet(num_verts, type, 4*rmesa->state.aos_count);

	for(i=start;i<start+num_verts;i++){
#if 0
		fprintf(stderr, "* (%f %f %f %f) (%f %f %f %f)\n",
			VEC_ELT(VB->ObjPtr, GLfloat, i)[0],
			VEC_ELT(VB->ObjPtr, GLfloat, i)[1],
			VEC_ELT(VB->ObjPtr, GLfloat, i)[2],
			VEC_ELT(VB->ObjPtr, GLfloat, i)[3],

			VEC_ELT(VB->ColorPtr[0], GLfloat, i)[0],
			VEC_ELT(VB->ColorPtr[0], GLfloat, i)[1],
			VEC_ELT(VB->ColorPtr[0], GLfloat, i)[2],
			VEC_ELT(VB->ColorPtr[0], GLfloat, i)[3]
			);
#endif


		/* coordinates */
		if(render_inputs & _TNL_BIT_POS)
			output_vector(VB->ObjPtr, i);
		if(render_inputs & _TNL_BIT_NORMAL)
			output_vector(VB->NormalPtr, i);

		/* color components */
		if(render_inputs & _TNL_BIT_COLOR0)
			output_vector(VB->ColorPtr[0], i);
		if(render_inputs & _TNL_BIT_COLOR1)
			output_vector(VB->SecondaryColorPtr[0], i);

/*		if(render_inputs & _TNL_BIT_FOG) // Causes lock ups when immediate mode is on
			output_vector(VB->FogCoordPtr, i);*/

		/* texture coordinates */
		for(k=0;k < ctx->Const.MaxTextureUnits;k++)
			if(render_inputs & (_TNL_BIT_TEX0<<k))
				output_vector(VB->TexCoordPtr[k], i);

		if(render_inputs & _TNL_BIT_INDEX)
			output_vector(VB->IndexPtr[0], i);
		if(render_inputs & _TNL_BIT_POINTSIZE)
			output_vector(VB->PointSizePtr, i);
		}

}


static GLboolean r300_run_immediate_render(GLcontext *ctx,
				 struct tnl_pipeline_stage *stage)
{
   r300ContextPtr rmesa = R300_CONTEXT(ctx);
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   struct vertex_buffer *VB = &tnl->vb;
   GLuint i;
   /* Only do 2d textures */
   struct gl_texture_object *to=ctx->Texture.Unit[0].Current2D;
   r300TexObjPtr t=to->DriverData;
   LOCAL_VARS


   /* Update texture state - needs to be done only when actually changed..
      All the time for now.. */


	if (RADEON_DEBUG == DEBUG_PRIMS)
		fprintf(stderr, "%s\n", __FUNCTION__);

#if 1 /* we need this, somehow */
   /* Flush state - make sure command buffer is nice and large */
   r300Flush(ctx);
   /* Make sure we have enough space */
#else
   /* Count is very imprecize, but should be good upper bound */
   r300EnsureCmdBufSpace(rmesa, rmesa->hw.max_state_size + 4+2+30
   	+VB->PrimitiveCount*(1+8)+VB->Count*4*rmesa->state.texture.tc_count+4, __FUNCTION__);
#endif

   /* needed before starting 3d operation .. */
   reg_start(R300_RB3D_DSTCACHE_CTLSTAT,0);
	e32(0x0000000a);

   reg_start(0x4f18,0);
	e32(0x00000003);


#if 0 /* looks like the Z offset issue got fixed */
   rmesa->hw.vte.cmd[1] = R300_VPORT_X_SCALE_ENA
				| R300_VPORT_X_OFFSET_ENA
				| R300_VPORT_Y_SCALE_ENA
				| R300_VPORT_Y_OFFSET_ENA
				| R300_VTX_W0_FMT;
   R300_STATECHANGE(rmesa, vte);
#endif



   /* Magic register - note it is right after 20b0 */


   if(rmesa->state.texture.tc_count>0){
   	reg_start(0x20b4,0);
		e32(0x0000000c);

	}

   r300EmitState(rmesa);

/* Setup INPUT_ROUTE and INPUT_CNTL */
	r300EmitArrays(ctx, GL_TRUE);

/* Why do we need this for immediate mode?? Vertex processor needs it to know proper regs */
//	r300EmitLOAD_VBPNTR(rmesa, 0);
/* Okay, it seems I misunderstood something, EmitAOS does the same thing */
	r300EmitAOS(rmesa, rmesa->state.aos_count, 0);

   for(i=0; i < VB->PrimitiveCount; i++){
       GLuint prim = VB->Primitive[i].mode;
       GLuint start = VB->Primitive[i].start;
       GLuint length = VB->Primitive[i].count;

	r300_render_immediate_primitive(rmesa, ctx, start, start + length, prim);
   	}

    /* This sequence is required after any 3d drawing packet
      I suspect it work arounds a bug (or deficiency) in hardware */

   reg_start(R300_RB3D_DSTCACHE_CTLSTAT,0);
	e32(0x0000000a);

   reg_start(0x4f18,0);
	e32(0x00000003);

   return GL_FALSE;
}


/* vertex buffer implementation */

static void inline fire_EB(PREFIX unsigned long addr, int vertex_count, int type)
{
	LOCAL_VARS
	unsigned long addr_a;
	
	if(addr & 1){
		WARN_ONCE("Badly aligned buffer\n");
		return ;
	}
	addr_a = 0; /*addr & 0x1c;*/
	
	check_space(6);
	
	start_packet3(RADEON_CP_PACKET3_3D_DRAW_INDX_2, 0);
	/* TODO: R300_VAP_VF_CNTL__INDEX_SIZE_32bit . */
	e32(R300_VAP_VF_CNTL__PRIM_WALK_INDICES | (vertex_count<<16) | type);

	start_packet3(RADEON_CP_PACKET3_INDX_BUFFER, 2);
	e32(R300_EB_UNK1 | (addr_a << 16) | R300_EB_UNK2);
	e32(addr /*& 0xffffffe3*/);
	e32((vertex_count+1)/2 /*+ addr_a/4*/); /* Total number of dwords needed? */
}

static void r300_render_vb_primitive(r300ContextPtr rmesa,
	GLcontext *ctx,
	int start,
	int end,
	int prim)
{
   int type, num_verts;
   radeonScreenPtr rsp=rmesa->radeon.radeonScreen;
   LOCAL_VARS
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   struct vertex_buffer *VB = &tnl->vb;
   int i;

   type=r300_get_primitive_type(rmesa, ctx, prim);
   num_verts=r300_get_num_verts(rmesa, ctx, end-start, prim);

   if(type<0 || num_verts <= 0)return;

   if(rmesa->state.Elts){
	r300EmitAOS(rmesa, rmesa->state.aos_count, 0);
#if 0
	start_index32_packet(num_verts, type);
	for(i=0; i < num_verts; i++)
		e32(rmesa->state.Elts[start+i]); /* start ? */
#else
	WARN_ONCE("Rendering with elt buffers\n");
	if(num_verts == 1){
		start_index32_packet(num_verts, type);
		e32(rmesa->state.Elts[start]);
		return;
	}
	
	if(num_verts > 65535){ /* not implemented yet */
		WARN_ONCE("Too many elts\n");
		return;
	}
	r300EmitElts(ctx, rmesa->state.Elts+start, num_verts);
	fire_EB(PASS_PREFIX GET_START(&(rmesa->state.elt_dma)), num_verts, type);
#endif
   }else{
	   r300EmitAOS(rmesa, rmesa->state.aos_count, start);
	   fire_AOS(PASS_PREFIX num_verts, type);
   }
}

static GLboolean r300_run_vb_render(GLcontext *ctx,
				 struct tnl_pipeline_stage *stage)
{
   r300ContextPtr rmesa = R300_CONTEXT(ctx);
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   struct vertex_buffer *VB = &tnl->vb;
   int i, j;
   LOCAL_VARS
   
	if (RADEON_DEBUG & DEBUG_PRIMS)
		fprintf(stderr, "%s\n", __FUNCTION__);
	

   	r300ReleaseArrays(ctx);
	r300EmitArrays(ctx, GL_FALSE);

//	LOCK_HARDWARE(&(rmesa->radeon));

	reg_start(R300_RB3D_DSTCACHE_CTLSTAT,0);
	e32(0x0000000a);

	reg_start(0x4f18,0);
	e32(0x00000003);
	r300EmitState(rmesa);
	
	rmesa->state.Elts = VB->Elts;

	for(i=0; i < VB->PrimitiveCount; i++){
		GLuint prim = VB->Primitive[i].mode;
		GLuint start = VB->Primitive[i].start;
		GLuint length = VB->Primitive[i].count;
		
		r300_render_vb_primitive(rmesa, ctx, start, start + length, prim);
	}

	reg_start(R300_RB3D_DSTCACHE_CTLSTAT,0);
	e32(0x0000000a);

	reg_start(0x4f18,0);
	e32(0x00000003);

//	end_3d(PASS_PREFIX_VOID);

   /* Flush state - we are done drawing.. */
//	r300FlushCmdBufLocked(rmesa, __FUNCTION__);
//	radeonWaitForIdleLocked(&(rmesa->radeon));

//	UNLOCK_HARDWARE(&(rmesa->radeon));
	return GL_FALSE;
}

/**
 * Called by the pipeline manager to render a batch of primitives.
 * We can return true to pass on to the next stage (i.e. software
 * rasterization) or false to indicate that the pipeline has finished
 * after we render something.
 */
static GLboolean r300_run_render(GLcontext *ctx,
				 struct tnl_pipeline_stage *stage)
{
   r300ContextPtr rmesa = R300_CONTEXT(ctx);
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   struct vertex_buffer *VB = &tnl->vb;
   GLuint i;

	if (RADEON_DEBUG & DEBUG_PRIMS)
		fprintf(stderr, "%s\n", __FUNCTION__);


#if 1

#if 0
		return r300_run_immediate_render(ctx, stage);
#else
		return r300_run_vb_render(ctx, stage);
#endif
#else
	return GL_TRUE;
#endif
}


/**
 * Called by the pipeline manager once before rendering.
 * We check the GL state here to
 *  a) decide whether we can do the current state in hardware and
 *  b) update hardware registers
 */
#define FALLBACK_IF(expr) \
do {										\
	if (expr) {								\
		if (1 || RADEON_DEBUG & DEBUG_FALLBACKS)				\
			fprintf(stderr, "%s: fallback:%s\n",			\
				__FUNCTION__, #expr);				\
		/*stage->active = GL_FALSE*/;					\
		return;								\
	}									\
} while(0)

static void r300_check_render(GLcontext *ctx, struct tnl_pipeline_stage *stage)
{
	r300ContextPtr r300 = R300_CONTEXT(ctx);
	int i;

	if (RADEON_DEBUG & DEBUG_STATE)
		fprintf(stderr, "%s\n", __FUNCTION__);

	/* We only support rendering in hardware for now */
	if (ctx->RenderMode != GL_RENDER) {
		//stage->active = GL_FALSE;
		return;
	}
		

	/* I'm almost certain I forgot something here */
#if 0 /* These should work now.. */
	FALLBACK_IF(ctx->Color.DitherFlag);
	FALLBACK_IF(ctx->Color.AlphaEnabled); // GL_ALPHA_TEST
	FALLBACK_IF(ctx->Color.BlendEnabled); // GL_BLEND
	FALLBACK_IF(ctx->Polygon.OffsetFill); // GL_POLYGON_OFFSET_FILL
#endif
	//FALLBACK_IF(ctx->Polygon.OffsetPoint); // GL_POLYGON_OFFSET_POINT
	//FALLBACK_IF(ctx->Polygon.OffsetLine); // GL_POLYGON_OFFSET_LINE
	//FALLBACK_IF(ctx->Stencil.Enabled); // GL_STENCIL_TEST
	
	//FALLBACK_IF(ctx->Fog.Enabled); // GL_FOG disable as swtcl doesnt seem to support this
	//FALLBACK_IF(ctx->Polygon.SmoothFlag); // GL_POLYGON_SMOOTH disabling to get blender going
	FALLBACK_IF(ctx->Polygon.StippleFlag); // GL_POLYGON_STIPPLE
	FALLBACK_IF(ctx->Multisample.Enabled); // GL_MULTISAMPLE_ARB
	
	FALLBACK_IF(ctx->RenderMode != GL_RENDER);  // We do not do SELECT or FEEDBACK (yet ?)

#if 0 /* ut2k3 fails to start if this is on */
	/* One step at a time - let one texture pass.. */
	for (i = 1; i < ctx->Const.MaxTextureUnits; i++)
		FALLBACK_IF(ctx->Texture.Unit[i].Enabled);
#endif	
	
	/* Assumed factor reg is found but pattern is still missing */
	//FALLBACK_IF(ctx->Line.StippleFlag); // GL_LINE_STIPPLE disabling to get blender going
	
	/* HW doesnt appear to directly support these */
	//FALLBACK_IF(ctx->Line.SmoothFlag); // GL_LINE_SMOOTH disabling to get blender going
	FALLBACK_IF(ctx->Point.SmoothFlag); // GL_POINT_SMOOTH
	/* Rest could be done with vertex fragments */
	if (ctx->Extensions.NV_point_sprite || ctx->Extensions.ARB_point_sprite)
		FALLBACK_IF(ctx->Point.PointSprite); // GL_POINT_SPRITE_NV
	//GL_POINT_DISTANCE_ATTENUATION_ARB
	//GL_POINT_FADE_THRESHOLD_SIZE_ARB
	
	/* let r300_run_render do its job */
#if 0
	stage->active = GL_FALSE;
#endif
}


static void dtr(struct tnl_pipeline_stage *stage)
{
	(void)stage;
}

static GLboolean r300_create_render(GLcontext *ctx,
				    struct tnl_pipeline_stage *stage)
{
	return GL_TRUE;	
}


const struct tnl_pipeline_stage _r300_render_stage = {
	"r300 hw rasterize",
	NULL,
	r300_create_render,
	dtr,			/* destructor */
	r300_check_render,	/* check */
	r300_run_render		/* run */
};

static GLboolean r300_run_tcl_render(GLcontext *ctx,
				 struct tnl_pipeline_stage *stage)
{
   r300ContextPtr rmesa = R300_CONTEXT(ctx);
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   struct vertex_buffer *VB = &tnl->vb;
   GLuint i;
   struct r300_vertex_program *vp;
   
   	hw_tcl_on=future_hw_tcl_on;
   
	if (RADEON_DEBUG & DEBUG_PRIMS)
		fprintf(stderr, "%s\n", __FUNCTION__);
	if(hw_tcl_on == GL_FALSE)
		return GL_TRUE;
	if(ctx->VertexProgram._Enabled == GL_FALSE){
		_tnl_UpdateFixedFunctionProgram(ctx);
	}
	vp = (struct r300_vertex_program *)CURRENT_VERTEX_SHADER(ctx);
	if(vp->translated == GL_FALSE)
		translate_vertex_shader(vp);
	if(vp->translated == GL_FALSE){
		fprintf(stderr, "Failing back to sw-tcl\n");
		debug_vp(ctx, &vp->mesa_program);
		hw_tcl_on=future_hw_tcl_on=0;
		r300ResetHwState(rmesa);
		return GL_TRUE;
	}
		
	r300_setup_textures(ctx);
	r300_setup_rs_unit(ctx);

	r300SetupVertexShader(rmesa);
	r300SetupPixelShader(rmesa);

	return r300_run_vb_render(ctx, stage);
}

static void r300_check_tcl_render(GLcontext *ctx, struct tnl_pipeline_stage *stage)
{
	r300ContextPtr r300 = R300_CONTEXT(ctx);
	int i;

	if (RADEON_DEBUG & DEBUG_STATE)
		fprintf(stderr, "%s\n", __FUNCTION__);

	/* We only support rendering in hardware for now */
	if (ctx->RenderMode != GL_RENDER) {
		//stage->active = GL_FALSE;
		return;
	}
}

const struct tnl_pipeline_stage _r300_tcl_stage = {
	"r300 tcl",
	NULL,
	r300_create_render,
	dtr,			/* destructor */
	r300_check_tcl_render,	/* check */
	r300_run_tcl_render	/* run */
};
