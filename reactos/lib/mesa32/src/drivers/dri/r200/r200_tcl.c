/* $XFree86: xc/lib/GL/mesa/src/drv/r200/r200_tcl.c,v 1.2 2002/12/16 16:18:55 dawes Exp $ */
/*
Copyright (C) The Weather Channel, Inc.  2002.  All Rights Reserved.

The Weather Channel (TM) funded Tungsten Graphics to develop the
initial release of the Radeon 8500 driver under the XFree86 license.
This notice must be preserved.

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice (including the
next paragraph) shall be included in all copies or substantial
portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/

/*
 * Authors:
 *   Keith Whitwell <keith@tungstengraphics.com>
 */

#include "glheader.h"
#include "imports.h"
#include "mtypes.h"
#include "enums.h"
#include "colormac.h"
#include "light.h"

#include "array_cache/acache.h"
#include "tnl/tnl.h"
#include "tnl/t_pipeline.h"

#include "r200_context.h"
#include "r200_state.h"
#include "r200_ioctl.h"
#include "r200_tex.h"
#include "r200_tcl.h"
#include "r200_swtcl.h"
#include "r200_maos.h"



#define HAVE_POINTS      1
#define HAVE_LINES       1
#define HAVE_LINE_LOOP   0
#define HAVE_LINE_STRIPS 1
#define HAVE_TRIANGLES   1
#define HAVE_TRI_STRIPS  1
#define HAVE_TRI_STRIP_1 0
#define HAVE_TRI_FANS    1
#define HAVE_QUADS       1
#define HAVE_QUAD_STRIPS 1
#define HAVE_POLYGONS    1
#define HAVE_ELTS        1


#define HW_POINTS           R200_VF_PRIM_POINTS
#define HW_LINES            R200_VF_PRIM_LINES
#define HW_LINE_LOOP        0
#define HW_LINE_STRIP       R200_VF_PRIM_LINE_STRIP
#define HW_TRIANGLES        R200_VF_PRIM_TRIANGLES
#define HW_TRIANGLE_STRIP_0 R200_VF_PRIM_TRIANGLE_STRIP
#define HW_TRIANGLE_STRIP_1 0
#define HW_TRIANGLE_FAN     R200_VF_PRIM_TRIANGLE_FAN
#define HW_QUADS            R200_VF_PRIM_QUADS
#define HW_QUAD_STRIP       R200_VF_PRIM_QUAD_STRIP
#define HW_POLYGON          R200_VF_PRIM_POLYGON


static GLboolean discrete_prim[0x10] = {
   0,				/* 0 none */
   1,				/* 1 points */
   1,				/* 2 lines */
   0,				/* 3 line_strip */
   1,				/* 4 tri_list */
   0,				/* 5 tri_fan */
   0,				/* 6 tri_strip */
   0,				/* 7 tri_w_flags */
   1,				/* 8 rect list (unused) */
   1,				/* 9 3vert point */
   1,				/* a 3vert line */
   0,				/* b point sprite */
   0,				/* c line loop */
   1,				/* d quads */
   0,				/* e quad strip */
   0,				/* f polygon */
};
   

#define LOCAL_VARS r200ContextPtr rmesa = R200_CONTEXT(ctx)
#define ELT_TYPE  GLushort

#define ELT_INIT(prim, hw_prim) \
   r200TclPrimitive( ctx, prim, hw_prim | R200_VF_PRIM_WALK_IND )

#define GET_MESA_ELTS() rmesa->tcl.Elts


/* Don't really know how many elts will fit in what's left of cmdbuf,
 * as there is state to emit, etc:
 */

/* Testing on isosurf shows a maximum around here.  Don't know if it's
 * the card or driver or kernel module that is causing the behaviour.
 */
#define GET_MAX_HW_ELTS() 300

#define RESET_STIPPLE() do {			\
   R200_STATECHANGE( rmesa, lin );		\
   r200EmitState( rmesa );			\
} while (0)

#define AUTO_STIPPLE( mode )  do {		\
   R200_STATECHANGE( rmesa, lin );		\
   if (mode)					\
      rmesa->hw.lin.cmd[LIN_RE_LINE_PATTERN] |=	\
	 R200_LINE_PATTERN_AUTO_RESET;	\
   else						\
      rmesa->hw.lin.cmd[LIN_RE_LINE_PATTERN] &=	\
	 ~R200_LINE_PATTERN_AUTO_RESET;	\
   r200EmitState( rmesa );			\
} while (0)


#define ALLOC_ELTS(nr)	r200AllocElts( rmesa, nr )

static GLushort *r200AllocElts( r200ContextPtr rmesa, GLuint nr ) 
{
   if (rmesa->dma.flush == r200FlushElts &&
       rmesa->store.cmd_used + nr*2 < R200_CMD_BUF_SZ) {

      GLushort *dest = (GLushort *)(rmesa->store.cmd_buf +
				    rmesa->store.cmd_used);

      rmesa->store.cmd_used += nr*2;

      return dest;
   }
   else {
      if (rmesa->dma.flush)
	 rmesa->dma.flush( rmesa );

      r200EnsureCmdBufSpace( rmesa, AOS_BUFSZ(rmesa->tcl.nr_aos_components) +
			     rmesa->hw.max_state_size + ELTS_BUFSZ(nr) );

      r200EmitAOS( rmesa,
		   rmesa->tcl.aos_components,
		   rmesa->tcl.nr_aos_components, 0 );

      return r200AllocEltsOpenEnded( rmesa, rmesa->tcl.hw_primitive, nr );
   }
}


#define CLOSE_ELTS() 				\
do {						\
   if (0) R200_NEWPRIM( rmesa );		\
}						\
while (0)


/* TODO: Try to extend existing primitive if both are identical,
 * discrete and there are no intervening state changes.  (Somewhat
 * duplicates changes to DrawArrays code)
 */
static void r200EmitPrim( GLcontext *ctx, 
		          GLenum prim, 
		          GLuint hwprim, 
		          GLuint start, 
		          GLuint count)	
{
   r200ContextPtr rmesa = R200_CONTEXT( ctx );
   r200TclPrimitive( ctx, prim, hwprim );
   
   r200EnsureCmdBufSpace( rmesa, AOS_BUFSZ(rmesa->tcl.nr_aos_components) +
			  rmesa->hw.max_state_size + VBUF_BUFSZ );

   r200EmitAOS( rmesa,
		  rmesa->tcl.aos_components,
		  rmesa->tcl.nr_aos_components,
		  start );
   
   /* Why couldn't this packet have taken an offset param?
    */
   r200EmitVbufPrim( rmesa,
		     rmesa->tcl.hw_primitive,
		     count - start );
}

#define EMIT_PRIM(ctx, prim, hwprim, start, count) do {         \
   r200EmitPrim( ctx, prim, hwprim, start, count );             \
   (void) rmesa; } while (0)

/* Try & join small primitives
 */
#if 0
#define PREFER_DISCRETE_ELT_PRIM( NR, PRIM ) 0
#else
#define PREFER_DISCRETE_ELT_PRIM( NR, PRIM )			\
  ((NR) < 20 ||							\
   ((NR) < 40 &&						\
    rmesa->tcl.hw_primitive == (PRIM|				\
			    R200_VF_TCL_OUTPUT_VTX_ENABLE|	\
			        R200_VF_PRIM_WALK_IND)))
#endif

#ifdef MESA_BIG_ENDIAN
/* We could do without (most of) this ugliness if dest was always 32 bit word aligned... */
#define EMIT_ELT(dest, offset, x) do {                          \
        int off = offset + ( ( (GLuint)dest & 0x2 ) >> 1 );     \
        GLushort *des = (GLushort *)( (GLuint)dest & ~0x2 );    \
        (des)[ off + 1 - 2 * ( off & 1 ) ] = (GLushort)(x);	\
	(void)rmesa; } while (0)
#else
#define EMIT_ELT(dest, offset, x) do {				\
	(dest)[offset] = (GLushort) (x);			\
	(void)rmesa; } while (0)
#endif

#define EMIT_TWO_ELTS(dest, offset, x, y)  *(GLuint *)((dest)+offset) = ((y)<<16)|(x);



#define TAG(x) tcl_##x
#include "tnl_dd/t_dd_dmatmp2.h"

/**********************************************************************/
/*                          External entrypoints                     */
/**********************************************************************/

void r200EmitPrimitive( GLcontext *ctx, 
			  GLuint first,
			  GLuint last,
			  GLuint flags )
{
   tcl_render_tab_verts[flags&PRIM_MODE_MASK]( ctx, first, last, flags );
}

void r200EmitEltPrimitive( GLcontext *ctx, 
			     GLuint first,
			     GLuint last,
			     GLuint flags )
{
   tcl_render_tab_elts[flags&PRIM_MODE_MASK]( ctx, first, last, flags );
}

void r200TclPrimitive( GLcontext *ctx, 
			 GLenum prim,
			 int hw_prim )
{
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
   GLuint newprim = hw_prim | R200_VF_TCL_OUTPUT_VTX_ENABLE;

   if (newprim != rmesa->tcl.hw_primitive ||
       !discrete_prim[hw_prim&0xf]) {
      R200_NEWPRIM( rmesa );
      rmesa->tcl.hw_primitive = newprim;
   }
}


/**********************************************************************/
/*             Fog blend factor computation for hw tcl                */
/*             same calculation used as in t_vb_fog.c                 */
/**********************************************************************/

#define FOG_EXP_TABLE_SIZE 256
#define FOG_MAX (10.0)
#define EXP_FOG_MAX .0006595
#define FOG_INCR (FOG_MAX/FOG_EXP_TABLE_SIZE)
static GLfloat exp_table[FOG_EXP_TABLE_SIZE];

#if 1
#define NEG_EXP( result, narg )						\
do {									\
   GLfloat f = (GLfloat) (narg * (1.0/FOG_INCR));			\
   GLint k = (GLint) f;							\
   if (k > FOG_EXP_TABLE_SIZE-2) 					\
      result = (GLfloat) EXP_FOG_MAX;					\
   else									\
      result = exp_table[k] + (f-k)*(exp_table[k+1]-exp_table[k]);	\
} while (0)
#else
#define NEG_EXP( result, narg )					\
do {								\
   result = exp(-narg);						\
} while (0)
#endif


/**
 * Initialize the exp_table[] lookup table for approximating exp().
 */
void
r200InitStaticFogData( void )
{
   GLfloat f = 0.0F;
   GLint i = 0;
   for ( ; i < FOG_EXP_TABLE_SIZE ; i++, f += FOG_INCR) {
      exp_table[i] = (GLfloat) exp(-f);
   }
}


/**
 * Compute per-vertex fog blend factors from fog coordinates by
 * evaluating the GL_LINEAR, GL_EXP or GL_EXP2 fog function.
 * Fog coordinates are distances from the eye (typically between the
 * near and far clip plane distances).
 * Note the fog (eye Z) coords may be negative so we use ABS(z) below.
 * Fog blend factors are in the range [0,1].
 */
float
r200ComputeFogBlendFactor( GLcontext *ctx, GLfloat fogcoord )
{
   GLfloat end  = ctx->Fog.End;
   GLfloat d, temp;
   const GLfloat z = FABSF(fogcoord);

   switch (ctx->Fog.Mode) {
   case GL_LINEAR:
      if (ctx->Fog.Start == ctx->Fog.End)
         d = 1.0F;
      else
         d = 1.0F / (ctx->Fog.End - ctx->Fog.Start);
      temp = (end - z) * d;
      return CLAMP(temp, 0.0F, 1.0F);
      break;
   case GL_EXP:
      d = ctx->Fog.Density;
      NEG_EXP( temp, d * z );
      return temp;
      break;
   case GL_EXP2:
      d = ctx->Fog.Density*ctx->Fog.Density;
      NEG_EXP( temp, d * z * z );
      return temp;
      break;
   default:
      _mesa_problem(ctx, "Bad fog mode in make_fog_coord");
      return 0;
   }
}


/**********************************************************************/
/*                          Render pipeline stage                     */
/**********************************************************************/


/* TCL render.
 */
static GLboolean r200_run_tcl_render( GLcontext *ctx,
				      struct tnl_pipeline_stage *stage )
{
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   struct vertex_buffer *VB = &tnl->vb;
   GLuint inputs = VERT_BIT_POS | VERT_BIT_COLOR0;
   GLuint i;

   /* TODO: separate this from the swtnl pipeline 
    */
   if (rmesa->TclFallback)
      return GL_TRUE;	/* fallback to software t&l */

   if (R200_DEBUG & DEBUG_PRIMS)
      fprintf(stderr, "%s\n", __FUNCTION__);

   if (VB->Count == 0)
      return GL_FALSE;

   /* Validate state:
    */
   if (rmesa->NewGLState)
      r200ValidateState( ctx );

   /* NOTE: inputs != tnl->render_inputs - these are the untransformed
    * inputs.
    */
   if (ctx->Light.Enabled) {
      inputs |= VERT_BIT_NORMAL;
      if (ctx->_TriangleCaps & DD_SEPARATE_SPECULAR) {
	 inputs |= VERT_BIT_COLOR1;
      }
   }

   if ( ctx->Fog.FogCoordinateSource == GL_FOG_COORD ) {
      inputs |= VERT_BIT_FOG;
   }

   for (i = 0 ; i < ctx->Const.MaxTextureUnits; i++) {
      if (ctx->Texture.Unit[i]._ReallyEnabled) {
	 if (rmesa->TexGenNeedNormals[i]) {
	    inputs |= VERT_BIT_NORMAL;
	 }
	 inputs |= VERT_BIT_TEX(i);
      }
   }

   /* Do the actual work:
    */
   r200ReleaseArrays( ctx, ~0 /* stage->changed_inputs */ );
   r200EmitArrays( ctx, inputs );

   rmesa->tcl.Elts = VB->Elts;

   for (i = 0 ; i < VB->PrimitiveCount ; i++)
   {
      GLuint prim = VB->Primitive[i].mode;
      GLuint start = VB->Primitive[i].start;
      GLuint length = VB->Primitive[i].count;

      if (!length)
	 continue;

      if (rmesa->tcl.Elts)
	 r200EmitEltPrimitive( ctx, start, start+length, prim );
      else
	 r200EmitPrimitive( ctx, start, start+length, prim );
   }

   return GL_FALSE;		/* finished the pipe */
}



/* Initial state for tcl stage.  
 */
const struct tnl_pipeline_stage _r200_tcl_stage =
{
   "r200 render",
   NULL,			/*  private */
   NULL,
   NULL,
   NULL,
   r200_run_tcl_render	/* run */
};



/**********************************************************************/
/*                 Validate state at pipeline start                   */
/**********************************************************************/


/*-----------------------------------------------------------------------
 * Manage TCL fallbacks
 */


static void transition_to_swtnl( GLcontext *ctx )
{
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
   TNLcontext *tnl = TNL_CONTEXT(ctx);

   R200_NEWPRIM( rmesa );

   r200ChooseVertexState( ctx );
   r200ChooseRenderState( ctx );

   _mesa_validate_all_lighting_tables( ctx ); 

   tnl->Driver.NotifyMaterialChange = 
      _mesa_validate_all_lighting_tables;

   r200ReleaseArrays( ctx, ~0 );

   /* Still using the D3D based hardware-rasterizer from the radeon;
    * need to put the card into D3D mode to make it work:
    */
   R200_STATECHANGE( rmesa, vap );
   rmesa->hw.vap.cmd[VAP_SE_VAP_CNTL] &= ~R200_VAP_TCL_ENABLE;
}

static void transition_to_hwtnl( GLcontext *ctx )
{
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
   TNLcontext *tnl = TNL_CONTEXT(ctx);

   _tnl_need_projected_coords( ctx, GL_FALSE );

   r200UpdateMaterial( ctx );

   tnl->Driver.NotifyMaterialChange = r200UpdateMaterial;

   if ( rmesa->dma.flush )			
      rmesa->dma.flush( rmesa );	

   rmesa->dma.flush = NULL;
   
   if (rmesa->swtcl.indexed_verts.buf) 
      r200ReleaseDmaRegion( rmesa, &rmesa->swtcl.indexed_verts, 
			      __FUNCTION__ );

   R200_STATECHANGE( rmesa, vap );
   rmesa->hw.vap.cmd[VAP_SE_VAP_CNTL] |= R200_VAP_TCL_ENABLE;
   rmesa->hw.vap.cmd[VAP_SE_VAP_CNTL] &= ~R200_VAP_FORCE_W_TO_ONE;

   if ( ((rmesa->hw.ctx.cmd[CTX_PP_FOG_COLOR] & R200_FOG_USE_MASK)
      == R200_FOG_USE_SPEC_ALPHA) &&
      (ctx->Fog.FogCoordinateSource == GL_FOG_COORD )) {
      R200_STATECHANGE( rmesa, ctx );
      rmesa->hw.ctx.cmd[CTX_PP_FOG_COLOR] &= ~R200_FOG_USE_MASK;
      rmesa->hw.ctx.cmd[CTX_PP_FOG_COLOR] |= R200_FOG_USE_VTX_FOG;
   }
   
   R200_STATECHANGE( rmesa, vte );
   rmesa->hw.vte.cmd[VTE_SE_VTE_CNTL] &= ~(R200_VTX_XY_FMT|R200_VTX_Z_FMT);
   rmesa->hw.vte.cmd[VTE_SE_VTE_CNTL] |= R200_VTX_W0_FMT;

   if (R200_DEBUG & DEBUG_FALLBACKS) 
      fprintf(stderr, "R200 end tcl fallback\n");
}


static char *fallbackStrings[] = {
   "Rasterization fallback",
   "Unfilled triangles",
   "Twosided lighting, differing materials",
   "Materials in VB (maybe between begin/end)",
   "Texgen unit 0",
   "Texgen unit 1",
   "Texgen unit 2",
   "Texgen unit 3",
   "Texgen unit 4",
   "Texgen unit 5",
   "User disable",
   "Bitmap as points",
   "Vertex program"
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



void r200TclFallback( GLcontext *ctx, GLuint bit, GLboolean mode )
{
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
   GLuint oldfallback = rmesa->TclFallback;

   if (mode) {
      rmesa->TclFallback |= bit;
      if (oldfallback == 0) {
	 if (R200_DEBUG & DEBUG_FALLBACKS) 
	    fprintf(stderr, "R200 begin tcl fallback %s\n",
		    getFallbackString( bit ));
	 transition_to_swtnl( ctx );
      }
   }
   else {
      rmesa->TclFallback &= ~bit;
      if (oldfallback == bit) {
	 if (R200_DEBUG & DEBUG_FALLBACKS) 
	    fprintf(stderr, "R200 end tcl fallback %s\n",
		    getFallbackString( bit ));
	 transition_to_hwtnl( ctx );
      }
   }
}
