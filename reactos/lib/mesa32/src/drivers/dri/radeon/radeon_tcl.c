/* $XFree86$ */
/**************************************************************************

Copyright 2000, 2001 ATI Technologies Inc., Ontario, Canada, and
                     Tungsten Graphics Inc., Austin, Texas.

All Rights Reserved.

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
#include "light.h"
#include "mtypes.h"
#include "enums.h"

#include "array_cache/acache.h"
#include "tnl/tnl.h"
#include "tnl/t_pipeline.h"

#include "radeon_context.h"
#include "radeon_state.h"
#include "radeon_ioctl.h"
#include "radeon_tex.h"
#include "radeon_tcl.h"
#include "radeon_swtcl.h"
#include "radeon_maos.h"



/*
 * Render unclipped vertex buffers by emitting vertices directly to
 * dma buffers.  Use strip/fan hardware primitives where possible.
 * Try to simulate missing primitives with indexed vertices.
 */
#define HAVE_POINTS      1
#define HAVE_LINES       1
#define HAVE_LINE_LOOP   0
#define HAVE_LINE_STRIPS 1
#define HAVE_TRIANGLES   1
#define HAVE_TRI_STRIPS  1
#define HAVE_TRI_STRIP_1 0
#define HAVE_TRI_FANS    1
#define HAVE_QUADS       0
#define HAVE_QUAD_STRIPS 0
#define HAVE_POLYGONS    1
#define HAVE_ELTS        1


#define HW_POINTS           RADEON_CP_VC_CNTL_PRIM_TYPE_POINT
#define HW_LINES            RADEON_CP_VC_CNTL_PRIM_TYPE_LINE
#define HW_LINE_LOOP        0
#define HW_LINE_STRIP       RADEON_CP_VC_CNTL_PRIM_TYPE_LINE_STRIP
#define HW_TRIANGLES        RADEON_CP_VC_CNTL_PRIM_TYPE_TRI_LIST
#define HW_TRIANGLE_STRIP_0 RADEON_CP_VC_CNTL_PRIM_TYPE_TRI_STRIP
#define HW_TRIANGLE_STRIP_1 0
#define HW_TRIANGLE_FAN     RADEON_CP_VC_CNTL_PRIM_TYPE_TRI_FAN
#define HW_QUADS            0
#define HW_QUAD_STRIP       0
#define HW_POLYGON          RADEON_CP_VC_CNTL_PRIM_TYPE_TRI_FAN


static GLboolean discrete_prim[0x10] = {
   0,				/* 0 none */
   1,				/* 1 points */
   1,				/* 2 lines */
   0,				/* 3 line_strip */
   1,				/* 4 tri_list */
   0,				/* 5 tri_fan */
   0,				/* 6 tri_type2 */
   1,				/* 7 rect list (unused) */
   1,				/* 8 3vert point */
   1,				/* 9 3vert line */
   0,
   0,
   0,
   0,
   0,
   0,
};
   

#define LOCAL_VARS radeonContextPtr rmesa = RADEON_CONTEXT(ctx)
#define ELT_TYPE  GLushort

#define ELT_INIT(prim, hw_prim) \
   radeonTclPrimitive( ctx, prim, hw_prim | RADEON_CP_VC_CNTL_PRIM_WALK_IND )

#define GET_MESA_ELTS() rmesa->tcl.Elts


/* Don't really know how many elts will fit in what's left of cmdbuf,
 * as there is state to emit, etc:
 */

/* Testing on isosurf shows a maximum around here.  Don't know if it's
 * the card or driver or kernel module that is causing the behaviour.
 */
#define GET_MAX_HW_ELTS() 300


#define RESET_STIPPLE() do {			\
   RADEON_STATECHANGE( rmesa, lin );		\
   radeonEmitState( rmesa );			\
} while (0)

#define AUTO_STIPPLE( mode )  do {		\
   RADEON_STATECHANGE( rmesa, lin );		\
   if (mode)					\
      rmesa->hw.lin.cmd[LIN_RE_LINE_PATTERN] |=	\
	 RADEON_LINE_PATTERN_AUTO_RESET;	\
   else						\
      rmesa->hw.lin.cmd[LIN_RE_LINE_PATTERN] &=	\
	 ~RADEON_LINE_PATTERN_AUTO_RESET;	\
   radeonEmitState( rmesa );			\
} while (0)



#define ALLOC_ELTS(nr)	radeonAllocElts( rmesa, nr )

static GLushort *radeonAllocElts( radeonContextPtr rmesa, GLuint nr ) 
{
   if (rmesa->dma.flush)
      rmesa->dma.flush( rmesa );

   radeonEnsureCmdBufSpace(rmesa, AOS_BUFSZ(rmesa->tcl.nr_aos_components) +
			   rmesa->hw.max_state_size + ELTS_BUFSZ(nr));

   radeonEmitAOS( rmesa,
		rmesa->tcl.aos_components,
		rmesa->tcl.nr_aos_components, 0 );

   return radeonAllocEltsOpenEnded( rmesa,
				    rmesa->tcl.vertex_format, 
				    rmesa->tcl.hw_primitive, nr );
}

#define CLOSE_ELTS()  RADEON_NEWPRIM( rmesa )



/* TODO: Try to extend existing primitive if both are identical,
 * discrete and there are no intervening state changes.  (Somewhat
 * duplicates changes to DrawArrays code)
 */
static void radeonEmitPrim( GLcontext *ctx, 
		       GLenum prim, 
		       GLuint hwprim, 
		       GLuint start, 
		       GLuint count)	
{
   radeonContextPtr rmesa = RADEON_CONTEXT( ctx );
   radeonTclPrimitive( ctx, prim, hwprim );
   
   radeonEnsureCmdBufSpace( rmesa, AOS_BUFSZ(rmesa->tcl.nr_aos_components) +
			    rmesa->hw.max_state_size + VBUF_BUFSZ );

   radeonEmitAOS( rmesa,
		  rmesa->tcl.aos_components,
		  rmesa->tcl.nr_aos_components,
		  start );
   
   /* Why couldn't this packet have taken an offset param?
    */
   radeonEmitVbufPrim( rmesa,
		       rmesa->tcl.vertex_format,
		       rmesa->tcl.hw_primitive,
		       count - start );
}

#define EMIT_PRIM( ctx, prim, hwprim, start, count ) do {       \
   radeonEmitPrim( ctx, prim, hwprim, start, count );           \
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
			    RADEON_CP_VC_CNTL_PRIM_WALK_IND|	\
			    RADEON_CP_VC_CNTL_TCL_ENABLE)))
#endif

#ifdef MESA_BIG_ENDIAN
/* We could do without (most of) this ugliness if dest was always 32 bit word aligned... */
#define EMIT_ELT(dest, offset, x) do {				\
	int off = offset + ( ( (GLuint)dest & 0x2 ) >> 1 );	\
	GLushort *des = (GLushort *)( (GLuint)dest & ~0x2 );	\
	(des)[ off + 1 - 2 * ( off & 1 ) ] = (GLushort)(x); 	\
	(void)rmesa; } while (0)
#else
#define EMIT_ELT(dest, offset, x) do {				\
	(dest)[offset] = (GLushort) (x);			\
	(void)rmesa; } while (0)
#endif

#define EMIT_TWO_ELTS(dest, offset, x, y)  *(GLuint *)(dest+offset) = ((y)<<16)|(x);



#define TAG(x) tcl_##x
#include "tnl_dd/t_dd_dmatmp2.h"

/**********************************************************************/
/*                          External entrypoints                     */
/**********************************************************************/

void radeonEmitPrimitive( GLcontext *ctx, 
			  GLuint first,
			  GLuint last,
			  GLuint flags )
{
   tcl_render_tab_verts[flags&PRIM_MODE_MASK]( ctx, first, last, flags );
}

void radeonEmitEltPrimitive( GLcontext *ctx, 
			     GLuint first,
			     GLuint last,
			     GLuint flags )
{
   tcl_render_tab_elts[flags&PRIM_MODE_MASK]( ctx, first, last, flags );
}

void radeonTclPrimitive( GLcontext *ctx, 
			 GLenum prim,
			 int hw_prim )
{
   radeonContextPtr rmesa = RADEON_CONTEXT(ctx);
   GLuint se_cntl;
   GLuint newprim = hw_prim | RADEON_CP_VC_CNTL_TCL_ENABLE;

   if (newprim != rmesa->tcl.hw_primitive ||
       !discrete_prim[hw_prim&0xf]) {
      RADEON_NEWPRIM( rmesa );
      rmesa->tcl.hw_primitive = newprim;
   }

   se_cntl = rmesa->hw.set.cmd[SET_SE_CNTL];
   se_cntl &= ~RADEON_FLAT_SHADE_VTX_LAST;

   if (prim == GL_POLYGON && (ctx->_TriangleCaps & DD_FLATSHADE)) 
      se_cntl |= RADEON_FLAT_SHADE_VTX_0;
   else
      se_cntl |= RADEON_FLAT_SHADE_VTX_LAST;

   if (se_cntl != rmesa->hw.set.cmd[SET_SE_CNTL]) {
      RADEON_STATECHANGE( rmesa, set );
      rmesa->hw.set.cmd[SET_SE_CNTL] = se_cntl;
   }
}


/**********************************************************************/
/*                          Render pipeline stage                     */
/**********************************************************************/


/* TCL render.
 */
static GLboolean radeon_run_tcl_render( GLcontext *ctx,
					struct tnl_pipeline_stage *stage )
{
   radeonContextPtr rmesa = RADEON_CONTEXT(ctx);
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   struct vertex_buffer *VB = &tnl->vb;
   GLuint inputs = VERT_BIT_POS | VERT_BIT_COLOR0;
   GLuint i;

   /* TODO: separate this from the swtnl pipeline 
    */
   if (rmesa->TclFallback)
      return GL_TRUE;	/* fallback to software t&l */

   if (VB->Count == 0)
      return GL_FALSE;

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

   radeonReleaseArrays( ctx, ~0 );
   radeonEmitArrays( ctx, inputs );

   rmesa->tcl.Elts = VB->Elts;

   for (i = 0 ; i < VB->PrimitiveCount ; i++)
   {
      GLuint prim = VB->Primitive[i].mode;
      GLuint start = VB->Primitive[i].start;
      GLuint length = VB->Primitive[i].count;

      if (!length)
	 continue;

      if (rmesa->tcl.Elts)
	 radeonEmitEltPrimitive( ctx, start, start+length, prim );
      else
	 radeonEmitPrimitive( ctx, start, start+length, prim );
   }

   return GL_FALSE;		/* finished the pipe */
}



/* Initial state for tcl stage.  
 */
const struct tnl_pipeline_stage _radeon_tcl_stage =
{
   "radeon render",
   NULL,
   NULL,
   NULL,
   NULL,
   radeon_run_tcl_render	/* run */
};



/**********************************************************************/
/*                 Validate state at pipeline start                   */
/**********************************************************************/


/*-----------------------------------------------------------------------
 * Manage TCL fallbacks
 */


static void transition_to_swtnl( GLcontext *ctx )
{
   radeonContextPtr rmesa = RADEON_CONTEXT(ctx);
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   GLuint se_cntl;

   RADEON_NEWPRIM( rmesa );
   rmesa->swtcl.vertex_format = 0;

   radeonChooseVertexState( ctx );
   radeonChooseRenderState( ctx );

   _mesa_validate_all_lighting_tables( ctx ); 

   tnl->Driver.NotifyMaterialChange = 
      _mesa_validate_all_lighting_tables;

   radeonReleaseArrays( ctx, ~0 );

   se_cntl = rmesa->hw.set.cmd[SET_SE_CNTL];
   se_cntl |= RADEON_FLAT_SHADE_VTX_LAST;
	 
   if (se_cntl != rmesa->hw.set.cmd[SET_SE_CNTL]) {
      RADEON_STATECHANGE( rmesa, set );
      rmesa->hw.set.cmd[SET_SE_CNTL] = se_cntl;
   }
}


static void transition_to_hwtnl( GLcontext *ctx )
{
   radeonContextPtr rmesa = RADEON_CONTEXT(ctx);
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   GLuint se_coord_fmt = (RADEON_VTX_W0_IS_NOT_1_OVER_W0 |
			  RADEON_TEX1_W_ROUTING_USE_Q1);

   if ( se_coord_fmt != rmesa->hw.set.cmd[SET_SE_COORDFMT] ) {
      RADEON_STATECHANGE( rmesa, set );
      rmesa->hw.set.cmd[SET_SE_COORDFMT] = se_coord_fmt;
      _tnl_need_projected_coords( ctx, GL_FALSE );
   }

   radeonUpdateMaterial( ctx );

   tnl->Driver.NotifyMaterialChange = radeonUpdateMaterial;

   if ( rmesa->dma.flush )			
      rmesa->dma.flush( rmesa );	

   rmesa->dma.flush = NULL;
   rmesa->swtcl.vertex_format = 0;
   
   if (rmesa->swtcl.indexed_verts.buf) 
      radeonReleaseDmaRegion( rmesa, &rmesa->swtcl.indexed_verts, 
			      __FUNCTION__ );

   if (RADEON_DEBUG & DEBUG_FALLBACKS) 
      fprintf(stderr, "Radeon end tcl fallback\n");
}

static char *fallbackStrings[] = {
   "Rasterization fallback",
   "Unfilled triangles",
   "Twosided lighting, differing materials",
   "Materials in VB (maybe between begin/end)",
   "Texgen unit 0",
   "Texgen unit 1",
   "Texgen unit 2",
   "User disable",
   "texture rectangle unit 0",
   "texture rectangle unit 1",
   "texture rectangle unit 2"
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



void radeonTclFallback( GLcontext *ctx, GLuint bit, GLboolean mode )
{
   radeonContextPtr rmesa = RADEON_CONTEXT(ctx);
   GLuint oldfallback = rmesa->TclFallback;

   if (mode) {
      rmesa->TclFallback |= bit;
      if (oldfallback == 0) {
	 if (RADEON_DEBUG & DEBUG_FALLBACKS) 
	    fprintf(stderr, "Radeon begin tcl fallback %s\n",
		    getFallbackString( bit ));
	 transition_to_swtnl( ctx );
      }
   }
   else {
      rmesa->TclFallback &= ~bit;
      if (oldfallback == bit) {
	 if (RADEON_DEBUG & DEBUG_FALLBACKS) 
	    fprintf(stderr, "Radeon end tcl fallback %s\n",
		    getFallbackString( bit ));
	 transition_to_hwtnl( ctx );
      }
   }
}
