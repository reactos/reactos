/* $XFree86$ */
/**************************************************************************

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
#include "api_arrayelt.h"
#include "enums.h"
#include "colormac.h"
#include "light.h"
#include "framebuffer.h"

#include "swrast/swrast.h"
#include "vbo/vbo.h"
#include "tnl/tnl.h"
#include "tnl/t_pipeline.h"
#include "swrast_setup/swrast_setup.h"

#include "r200_context.h"
#include "r200_ioctl.h"
#include "r200_state.h"
#include "r200_tcl.h"
#include "r200_tex.h"
#include "r200_swtcl.h"
#include "r200_vertprog.h"

#include "drirenderbuffer.h"


/* =============================================================
 * Alpha blending
 */

static void r200AlphaFunc( GLcontext *ctx, GLenum func, GLfloat ref )
{
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
   int pp_misc = rmesa->hw.ctx.cmd[CTX_PP_MISC];
   GLubyte refByte;

   CLAMPED_FLOAT_TO_UBYTE(refByte, ref);

   R200_STATECHANGE( rmesa, ctx );

   pp_misc &= ~(R200_ALPHA_TEST_OP_MASK | R200_REF_ALPHA_MASK);
   pp_misc |= (refByte & R200_REF_ALPHA_MASK);

   switch ( func ) {
   case GL_NEVER:
      pp_misc |= R200_ALPHA_TEST_FAIL; 
      break;
   case GL_LESS:
      pp_misc |= R200_ALPHA_TEST_LESS;
      break;
   case GL_EQUAL:
      pp_misc |= R200_ALPHA_TEST_EQUAL;
      break;
   case GL_LEQUAL:
      pp_misc |= R200_ALPHA_TEST_LEQUAL;
      break;
   case GL_GREATER:
      pp_misc |= R200_ALPHA_TEST_GREATER;
      break;
   case GL_NOTEQUAL:
      pp_misc |= R200_ALPHA_TEST_NEQUAL;
      break;
   case GL_GEQUAL:
      pp_misc |= R200_ALPHA_TEST_GEQUAL;
      break;
   case GL_ALWAYS:
      pp_misc |= R200_ALPHA_TEST_PASS;
      break;
   }

   rmesa->hw.ctx.cmd[CTX_PP_MISC] = pp_misc;
}

static void r200BlendColor( GLcontext *ctx, const GLfloat cf[4] )
{
   GLubyte color[4];
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
   R200_STATECHANGE( rmesa, ctx );
   CLAMPED_FLOAT_TO_UBYTE(color[0], cf[0]);
   CLAMPED_FLOAT_TO_UBYTE(color[1], cf[1]);
   CLAMPED_FLOAT_TO_UBYTE(color[2], cf[2]);
   CLAMPED_FLOAT_TO_UBYTE(color[3], cf[3]);
   if (rmesa->r200Screen->drmSupportsBlendColor)
      rmesa->hw.ctx.cmd[CTX_RB3D_BLENDCOLOR] = r200PackColor( 4, color[0], color[1], color[2], color[3] );
}

/**
 * Calculate the hardware blend factor setting.  This same function is used
 * for source and destination of both alpha and RGB.
 *
 * \returns
 * The hardware register value for the specified blend factor.  This value
 * will need to be shifted into the correct position for either source or
 * destination factor.
 *
 * \todo
 * Since the two cases where source and destination are handled differently
 * are essentially error cases, they should never happen.  Determine if these
 * cases can be removed.
 */
static int blend_factor( GLenum factor, GLboolean is_src )
{
   int func;

   switch ( factor ) {
   case GL_ZERO:
      func = R200_BLEND_GL_ZERO;
      break;
   case GL_ONE:
      func = R200_BLEND_GL_ONE;
      break;
   case GL_DST_COLOR:
      func = R200_BLEND_GL_DST_COLOR;
      break;
   case GL_ONE_MINUS_DST_COLOR:
      func = R200_BLEND_GL_ONE_MINUS_DST_COLOR;
      break;
   case GL_SRC_COLOR:
      func = R200_BLEND_GL_SRC_COLOR;
      break;
   case GL_ONE_MINUS_SRC_COLOR:
      func = R200_BLEND_GL_ONE_MINUS_SRC_COLOR;
      break;
   case GL_SRC_ALPHA:
      func = R200_BLEND_GL_SRC_ALPHA;
      break;
   case GL_ONE_MINUS_SRC_ALPHA:
      func = R200_BLEND_GL_ONE_MINUS_SRC_ALPHA;
      break;
   case GL_DST_ALPHA:
      func = R200_BLEND_GL_DST_ALPHA;
      break;
   case GL_ONE_MINUS_DST_ALPHA:
      func = R200_BLEND_GL_ONE_MINUS_DST_ALPHA;
      break;
   case GL_SRC_ALPHA_SATURATE:
      func = (is_src) ? R200_BLEND_GL_SRC_ALPHA_SATURATE : R200_BLEND_GL_ZERO;
      break;
   case GL_CONSTANT_COLOR:
      func = R200_BLEND_GL_CONST_COLOR;
      break;
   case GL_ONE_MINUS_CONSTANT_COLOR:
      func = R200_BLEND_GL_ONE_MINUS_CONST_COLOR;
      break;
   case GL_CONSTANT_ALPHA:
      func = R200_BLEND_GL_CONST_ALPHA;
      break;
   case GL_ONE_MINUS_CONSTANT_ALPHA:
      func = R200_BLEND_GL_ONE_MINUS_CONST_ALPHA;
      break;
   default:
      func = (is_src) ? R200_BLEND_GL_ONE : R200_BLEND_GL_ZERO;
   }
   return func;
}

/**
 * Sets both the blend equation and the blend function.
 * This is done in a single
 * function because some blend equations (i.e., \c GL_MIN and \c GL_MAX)
 * change the interpretation of the blend function.
 * Also, make sure that blend function and blend equation are set to their default
 * value if color blending is not enabled, since at least blend equations GL_MIN
 * and GL_FUNC_REVERSE_SUBTRACT will cause wrong results otherwise for
 * unknown reasons.
 */
static void r200_set_blend_state( GLcontext * ctx )
{
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
   GLuint cntl = rmesa->hw.ctx.cmd[CTX_RB3D_CNTL] &
      ~(R200_ROP_ENABLE | R200_ALPHA_BLEND_ENABLE | R200_SEPARATE_ALPHA_ENABLE);

   int func = (R200_BLEND_GL_ONE << R200_SRC_BLEND_SHIFT) |
      (R200_BLEND_GL_ZERO << R200_DST_BLEND_SHIFT);
   int eqn = R200_COMB_FCN_ADD_CLAMP;
   int funcA = (R200_BLEND_GL_ONE << R200_SRC_BLEND_SHIFT) |
      (R200_BLEND_GL_ZERO << R200_DST_BLEND_SHIFT);
   int eqnA = R200_COMB_FCN_ADD_CLAMP;

   R200_STATECHANGE( rmesa, ctx );

   if (rmesa->r200Screen->drmSupportsBlendColor) {
      if (ctx->Color.ColorLogicOpEnabled) {
         rmesa->hw.ctx.cmd[CTX_RB3D_CNTL] =  cntl | R200_ROP_ENABLE;
         rmesa->hw.ctx.cmd[CTX_RB3D_ABLENDCNTL] = eqn | func;
         rmesa->hw.ctx.cmd[CTX_RB3D_CBLENDCNTL] = eqn | func;
         return;
      } else if (ctx->Color.BlendEnabled) {
         rmesa->hw.ctx.cmd[CTX_RB3D_CNTL] =  cntl | R200_ALPHA_BLEND_ENABLE | R200_SEPARATE_ALPHA_ENABLE;
      }
      else {
         rmesa->hw.ctx.cmd[CTX_RB3D_CNTL] = cntl;
         rmesa->hw.ctx.cmd[CTX_RB3D_ABLENDCNTL] = eqn | func;
         rmesa->hw.ctx.cmd[CTX_RB3D_CBLENDCNTL] = eqn | func;
         return;
      }
   }
   else {
      if (ctx->Color.ColorLogicOpEnabled) {
         rmesa->hw.ctx.cmd[CTX_RB3D_CNTL] =  cntl | R200_ROP_ENABLE;
         rmesa->hw.ctx.cmd[CTX_RB3D_BLENDCNTL] = eqn | func;
         return;
      } else if (ctx->Color.BlendEnabled) {
         rmesa->hw.ctx.cmd[CTX_RB3D_CNTL] =  cntl | R200_ALPHA_BLEND_ENABLE;
      }
      else {
         rmesa->hw.ctx.cmd[CTX_RB3D_CNTL] = cntl;
         rmesa->hw.ctx.cmd[CTX_RB3D_BLENDCNTL] = eqn | func;
         return;
      }
   }

   func = (blend_factor( ctx->Color.BlendSrcRGB, GL_TRUE ) << R200_SRC_BLEND_SHIFT) |
      (blend_factor( ctx->Color.BlendDstRGB, GL_FALSE ) << R200_DST_BLEND_SHIFT);

   switch(ctx->Color.BlendEquationRGB) {
   case GL_FUNC_ADD:
      eqn = R200_COMB_FCN_ADD_CLAMP;
      break;

   case GL_FUNC_SUBTRACT:
      eqn = R200_COMB_FCN_SUB_CLAMP;
      break;

   case GL_FUNC_REVERSE_SUBTRACT:
      eqn = R200_COMB_FCN_RSUB_CLAMP;
      break;

   case GL_MIN:
      eqn = R200_COMB_FCN_MIN;
      func = (R200_BLEND_GL_ONE << R200_SRC_BLEND_SHIFT) |
         (R200_BLEND_GL_ONE << R200_DST_BLEND_SHIFT);
      break;

   case GL_MAX:
      eqn = R200_COMB_FCN_MAX;
      func = (R200_BLEND_GL_ONE << R200_SRC_BLEND_SHIFT) |
         (R200_BLEND_GL_ONE << R200_DST_BLEND_SHIFT);
      break;

   default:
      fprintf( stderr, "[%s:%u] Invalid RGB blend equation (0x%04x).\n",
         __FUNCTION__, __LINE__, ctx->Color.BlendEquationRGB );
      return;
   }

   if (!rmesa->r200Screen->drmSupportsBlendColor) {
      rmesa->hw.ctx.cmd[CTX_RB3D_BLENDCNTL] = eqn | func;
      return;
   }

   funcA = (blend_factor( ctx->Color.BlendSrcA, GL_TRUE ) << R200_SRC_BLEND_SHIFT) |
      (blend_factor( ctx->Color.BlendDstA, GL_FALSE ) << R200_DST_BLEND_SHIFT);

   switch(ctx->Color.BlendEquationA) {
   case GL_FUNC_ADD:
      eqnA = R200_COMB_FCN_ADD_CLAMP;
      break;

   case GL_FUNC_SUBTRACT:
      eqnA = R200_COMB_FCN_SUB_CLAMP;
      break;

   case GL_FUNC_REVERSE_SUBTRACT:
      eqnA = R200_COMB_FCN_RSUB_CLAMP;
      break;

   case GL_MIN:
      eqnA = R200_COMB_FCN_MIN;
      funcA = (R200_BLEND_GL_ONE << R200_SRC_BLEND_SHIFT) |
         (R200_BLEND_GL_ONE << R200_DST_BLEND_SHIFT);
      break;

   case GL_MAX:
      eqnA = R200_COMB_FCN_MAX;
      funcA = (R200_BLEND_GL_ONE << R200_SRC_BLEND_SHIFT) |
         (R200_BLEND_GL_ONE << R200_DST_BLEND_SHIFT);
      break;

   default:
      fprintf( stderr, "[%s:%u] Invalid A blend equation (0x%04x).\n",
         __FUNCTION__, __LINE__, ctx->Color.BlendEquationA );
      return;
   }

   rmesa->hw.ctx.cmd[CTX_RB3D_ABLENDCNTL] = eqnA | funcA;
   rmesa->hw.ctx.cmd[CTX_RB3D_CBLENDCNTL] = eqn | func;

}

static void r200BlendEquationSeparate( GLcontext *ctx,
				       GLenum modeRGB, GLenum modeA )
{
      r200_set_blend_state( ctx );
}

static void r200BlendFuncSeparate( GLcontext *ctx,
				     GLenum sfactorRGB, GLenum dfactorRGB,
				     GLenum sfactorA, GLenum dfactorA )
{
      r200_set_blend_state( ctx );
}


/* =============================================================
 * Depth testing
 */

static void r200DepthFunc( GLcontext *ctx, GLenum func )
{
   r200ContextPtr rmesa = R200_CONTEXT(ctx);

   R200_STATECHANGE( rmesa, ctx );
   rmesa->hw.ctx.cmd[CTX_RB3D_ZSTENCILCNTL] &= ~R200_Z_TEST_MASK;

   switch ( ctx->Depth.Func ) {
   case GL_NEVER:
      rmesa->hw.ctx.cmd[CTX_RB3D_ZSTENCILCNTL] |= R200_Z_TEST_NEVER;
      break;
   case GL_LESS:
      rmesa->hw.ctx.cmd[CTX_RB3D_ZSTENCILCNTL] |= R200_Z_TEST_LESS;
      break;
   case GL_EQUAL:
      rmesa->hw.ctx.cmd[CTX_RB3D_ZSTENCILCNTL] |= R200_Z_TEST_EQUAL;
      break;
   case GL_LEQUAL:
      rmesa->hw.ctx.cmd[CTX_RB3D_ZSTENCILCNTL] |= R200_Z_TEST_LEQUAL;
      break;
   case GL_GREATER:
      rmesa->hw.ctx.cmd[CTX_RB3D_ZSTENCILCNTL] |= R200_Z_TEST_GREATER;
      break;
   case GL_NOTEQUAL:
      rmesa->hw.ctx.cmd[CTX_RB3D_ZSTENCILCNTL] |= R200_Z_TEST_NEQUAL;
      break;
   case GL_GEQUAL:
      rmesa->hw.ctx.cmd[CTX_RB3D_ZSTENCILCNTL] |= R200_Z_TEST_GEQUAL;
      break;
   case GL_ALWAYS:
      rmesa->hw.ctx.cmd[CTX_RB3D_ZSTENCILCNTL] |= R200_Z_TEST_ALWAYS;
      break;
   }
}

static void r200ClearDepth( GLcontext *ctx, GLclampd d )
{
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
   GLuint format = (rmesa->hw.ctx.cmd[CTX_RB3D_ZSTENCILCNTL] &
		    R200_DEPTH_FORMAT_MASK);

   switch ( format ) {
   case R200_DEPTH_FORMAT_16BIT_INT_Z:
      rmesa->state.depth.clear = d * 0x0000ffff;
      break;
   case R200_DEPTH_FORMAT_24BIT_INT_Z:
      rmesa->state.depth.clear = d * 0x00ffffff;
      break;
   }
}

static void r200DepthMask( GLcontext *ctx, GLboolean flag )
{
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
   R200_STATECHANGE( rmesa, ctx );

   if ( ctx->Depth.Mask ) {
      rmesa->hw.ctx.cmd[CTX_RB3D_ZSTENCILCNTL] |=  R200_Z_WRITE_ENABLE;
   } else {
      rmesa->hw.ctx.cmd[CTX_RB3D_ZSTENCILCNTL] &= ~R200_Z_WRITE_ENABLE;
   }
}


/* =============================================================
 * Fog
 */


static void r200Fogfv( GLcontext *ctx, GLenum pname, const GLfloat *param )
{
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
   union { int i; float f; } c, d;
   GLchan col[4];
   GLuint i;

   c.i = rmesa->hw.fog.cmd[FOG_C];
   d.i = rmesa->hw.fog.cmd[FOG_D];

   switch (pname) {
   case GL_FOG_MODE:
      if (!ctx->Fog.Enabled)
	 return;
      R200_STATECHANGE(rmesa, tcl);
      rmesa->hw.tcl.cmd[TCL_UCP_VERT_BLEND_CTL] &= ~R200_TCL_FOG_MASK;
      switch (ctx->Fog.Mode) {
      case GL_LINEAR:
	 rmesa->hw.tcl.cmd[TCL_UCP_VERT_BLEND_CTL] |= R200_TCL_FOG_LINEAR;
	 if (ctx->Fog.Start == ctx->Fog.End) {
	    c.f = 1.0F;
	    d.f = 1.0F;
	 }
	 else {
	    c.f = ctx->Fog.End/(ctx->Fog.End-ctx->Fog.Start);
	    d.f = -1.0/(ctx->Fog.End-ctx->Fog.Start);
	 }
	 break;
      case GL_EXP:
	 rmesa->hw.tcl.cmd[TCL_UCP_VERT_BLEND_CTL] |= R200_TCL_FOG_EXP;
	 c.f = 0.0;
	 d.f = -ctx->Fog.Density;
	 break;
      case GL_EXP2:
	 rmesa->hw.tcl.cmd[TCL_UCP_VERT_BLEND_CTL] |= R200_TCL_FOG_EXP2;
	 c.f = 0.0;
	 d.f = -(ctx->Fog.Density * ctx->Fog.Density);
	 break;
      default:
	 return;
      }
      break;
   case GL_FOG_DENSITY:
      switch (ctx->Fog.Mode) {
      case GL_EXP:
	 c.f = 0.0;
	 d.f = -ctx->Fog.Density;
	 break;
      case GL_EXP2:
	 c.f = 0.0;
	 d.f = -(ctx->Fog.Density * ctx->Fog.Density);
	 break;
      default:
	 break;
      }
      break;
   case GL_FOG_START:
   case GL_FOG_END:
      if (ctx->Fog.Mode == GL_LINEAR) {
	 if (ctx->Fog.Start == ctx->Fog.End) {
	    c.f = 1.0F;
	    d.f = 1.0F;
	 } else {
	    c.f = ctx->Fog.End/(ctx->Fog.End-ctx->Fog.Start);
	    d.f = -1.0/(ctx->Fog.End-ctx->Fog.Start);
	 }
      }
      break;
   case GL_FOG_COLOR: 
      R200_STATECHANGE( rmesa, ctx );
      UNCLAMPED_FLOAT_TO_RGB_CHAN( col, ctx->Fog.Color );
      i = r200PackColor( 4, col[0], col[1], col[2], 0 );
      rmesa->hw.ctx.cmd[CTX_PP_FOG_COLOR] &= ~R200_FOG_COLOR_MASK;
      rmesa->hw.ctx.cmd[CTX_PP_FOG_COLOR] |= i;
      break;
   case GL_FOG_COORD_SRC: {
      GLuint out_0 = rmesa->hw.vtx.cmd[VTX_TCL_OUTPUT_VTXFMT_0];
      GLuint fog   = rmesa->hw.ctx.cmd[CTX_PP_FOG_COLOR];

      fog &= ~R200_FOG_USE_MASK;
      if ( ctx->Fog.FogCoordinateSource == GL_FOG_COORD || ctx->VertexProgram.Enabled) {
	 fog   |= R200_FOG_USE_VTX_FOG;
	 out_0 |= R200_VTX_DISCRETE_FOG;
      }
      else {
	 fog   |=  R200_FOG_USE_SPEC_ALPHA;
	 out_0 &= ~R200_VTX_DISCRETE_FOG;
      }

      if ( fog != rmesa->hw.ctx.cmd[CTX_PP_FOG_COLOR] ) {
	 R200_STATECHANGE( rmesa, ctx );
	 rmesa->hw.ctx.cmd[CTX_PP_FOG_COLOR] = fog;
      }

      if (out_0 != rmesa->hw.vtx.cmd[VTX_TCL_OUTPUT_VTXFMT_0]) {
	 R200_STATECHANGE( rmesa, vtx );
	 rmesa->hw.vtx.cmd[VTX_TCL_OUTPUT_VTXFMT_0] = out_0;	 
      }

      break;
   }
   default:
      return;
   }

   if (c.i != rmesa->hw.fog.cmd[FOG_C] || d.i != rmesa->hw.fog.cmd[FOG_D]) {
      R200_STATECHANGE( rmesa, fog );
      rmesa->hw.fog.cmd[FOG_C] = c.i;
      rmesa->hw.fog.cmd[FOG_D] = d.i;
   }
}


/* =============================================================
 * Scissoring
 */


static GLboolean intersect_rect( drm_clip_rect_t *out,
				 drm_clip_rect_t *a,
				 drm_clip_rect_t *b )
{
   *out = *a;
   if ( b->x1 > out->x1 ) out->x1 = b->x1;
   if ( b->y1 > out->y1 ) out->y1 = b->y1;
   if ( b->x2 < out->x2 ) out->x2 = b->x2;
   if ( b->y2 < out->y2 ) out->y2 = b->y2;
   if ( out->x1 >= out->x2 ) return GL_FALSE;
   if ( out->y1 >= out->y2 ) return GL_FALSE;
   return GL_TRUE;
}


void r200RecalcScissorRects( r200ContextPtr rmesa )
{
   drm_clip_rect_t *out;
   int i;

   /* Grow cliprect store?
    */
   if (rmesa->state.scissor.numAllocedClipRects < rmesa->numClipRects) {
      while (rmesa->state.scissor.numAllocedClipRects < rmesa->numClipRects) {
	 rmesa->state.scissor.numAllocedClipRects += 1;	/* zero case */
	 rmesa->state.scissor.numAllocedClipRects *= 2;
      }

      if (rmesa->state.scissor.pClipRects)
	 FREE(rmesa->state.scissor.pClipRects);

      rmesa->state.scissor.pClipRects = 
	 MALLOC( rmesa->state.scissor.numAllocedClipRects * 
		 sizeof(drm_clip_rect_t) );

      if ( rmesa->state.scissor.pClipRects == NULL ) {
	 rmesa->state.scissor.numAllocedClipRects = 0;
	 return;
      }
   }
   
   out = rmesa->state.scissor.pClipRects;
   rmesa->state.scissor.numClipRects = 0;

   for ( i = 0 ; i < rmesa->numClipRects ;  i++ ) {
      if ( intersect_rect( out, 
			   &rmesa->pClipRects[i], 
			   &rmesa->state.scissor.rect ) ) {
	 rmesa->state.scissor.numClipRects++;
	 out++;
      }
   }
}


static void r200UpdateScissor( GLcontext *ctx )
{
   r200ContextPtr rmesa = R200_CONTEXT(ctx);

   if ( rmesa->dri.drawable ) {
      __DRIdrawablePrivate *dPriv = rmesa->dri.drawable;

      int x = ctx->Scissor.X;
      int y = dPriv->h - ctx->Scissor.Y - ctx->Scissor.Height;
      int w = ctx->Scissor.X + ctx->Scissor.Width - 1;
      int h = dPriv->h - ctx->Scissor.Y - 1;

      rmesa->state.scissor.rect.x1 = x + dPriv->x;
      rmesa->state.scissor.rect.y1 = y + dPriv->y;
      rmesa->state.scissor.rect.x2 = w + dPriv->x + 1;
      rmesa->state.scissor.rect.y2 = h + dPriv->y + 1;

      r200RecalcScissorRects( rmesa );
   }
}


static void r200Scissor( GLcontext *ctx,
			   GLint x, GLint y, GLsizei w, GLsizei h )
{
   r200ContextPtr rmesa = R200_CONTEXT(ctx);

   if ( ctx->Scissor.Enabled ) {
      R200_FIREVERTICES( rmesa );	/* don't pipeline cliprect changes */
      r200UpdateScissor( ctx );
   }

}


/* =============================================================
 * Culling
 */

static void r200CullFace( GLcontext *ctx, GLenum unused )
{
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
   GLuint s = rmesa->hw.set.cmd[SET_SE_CNTL];
   GLuint t = rmesa->hw.tcl.cmd[TCL_UCP_VERT_BLEND_CTL];

   s |= R200_FFACE_SOLID | R200_BFACE_SOLID;
   t &= ~(R200_CULL_FRONT | R200_CULL_BACK);

   if ( ctx->Polygon.CullFlag ) {
      switch ( ctx->Polygon.CullFaceMode ) {
      case GL_FRONT:
	 s &= ~R200_FFACE_SOLID;
	 t |= R200_CULL_FRONT;
	 break;
      case GL_BACK:
	 s &= ~R200_BFACE_SOLID;
	 t |= R200_CULL_BACK;
	 break;
      case GL_FRONT_AND_BACK:
	 s &= ~(R200_FFACE_SOLID | R200_BFACE_SOLID);
	 t |= (R200_CULL_FRONT | R200_CULL_BACK);
	 break;
      }
   }

   if ( rmesa->hw.set.cmd[SET_SE_CNTL] != s ) {
      R200_STATECHANGE(rmesa, set );
      rmesa->hw.set.cmd[SET_SE_CNTL] = s;
   }

   if ( rmesa->hw.tcl.cmd[TCL_UCP_VERT_BLEND_CTL] != t ) {
      R200_STATECHANGE(rmesa, tcl );
      rmesa->hw.tcl.cmd[TCL_UCP_VERT_BLEND_CTL] = t;
   }
}

static void r200FrontFace( GLcontext *ctx, GLenum mode )
{
   r200ContextPtr rmesa = R200_CONTEXT(ctx);

   R200_STATECHANGE( rmesa, set );
   rmesa->hw.set.cmd[SET_SE_CNTL] &= ~R200_FFACE_CULL_DIR_MASK;

   R200_STATECHANGE( rmesa, tcl );
   rmesa->hw.tcl.cmd[TCL_UCP_VERT_BLEND_CTL] &= ~R200_CULL_FRONT_IS_CCW;

   switch ( mode ) {
   case GL_CW:
      rmesa->hw.set.cmd[SET_SE_CNTL] |= R200_FFACE_CULL_CW;
      break;
   case GL_CCW:
      rmesa->hw.set.cmd[SET_SE_CNTL] |= R200_FFACE_CULL_CCW;
      rmesa->hw.tcl.cmd[TCL_UCP_VERT_BLEND_CTL] |= R200_CULL_FRONT_IS_CCW;
      break;
   }
}

/* =============================================================
 * Point state
 */
static void r200PointSize( GLcontext *ctx, GLfloat size )
{
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
   GLfloat *fcmd = (GLfloat *)rmesa->hw.ptp.cmd;

   R200_STATECHANGE( rmesa, cst );
   R200_STATECHANGE( rmesa, ptp );
   rmesa->hw.cst.cmd[CST_RE_POINTSIZE] &= ~0xffff;
   rmesa->hw.cst.cmd[CST_RE_POINTSIZE] |= ((GLuint)(ctx->Point.Size * 16.0));
/* this is the size param of the point size calculation (point size reg value
   is not used when calculation is active). */
   fcmd[PTP_VPORT_SCALE_PTSIZE] = ctx->Point.Size;
}

static void r200PointParameter( GLcontext *ctx, GLenum pname, const GLfloat *params)
{
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
   GLfloat *fcmd = (GLfloat *)rmesa->hw.ptp.cmd;

   switch (pname) {
   case GL_POINT_SIZE_MIN:
   /* Can clamp both in tcl and setup - just set both (as does fglrx) */
      R200_STATECHANGE( rmesa, lin );
      R200_STATECHANGE( rmesa, ptp );
      rmesa->hw.lin.cmd[LIN_SE_LINE_WIDTH] &= 0xffff;
      rmesa->hw.lin.cmd[LIN_SE_LINE_WIDTH] |= (GLuint)(ctx->Point.MinSize * 16.0) << 16;
      fcmd[PTP_CLAMP_MIN] = ctx->Point.MinSize;
      break;
   case GL_POINT_SIZE_MAX:
      R200_STATECHANGE( rmesa, cst );
      R200_STATECHANGE( rmesa, ptp );
      rmesa->hw.cst.cmd[CST_RE_POINTSIZE] &= 0xffff;
      rmesa->hw.cst.cmd[CST_RE_POINTSIZE] |= (GLuint)(ctx->Point.MaxSize * 16.0) << 16;
      fcmd[PTP_CLAMP_MAX] = ctx->Point.MaxSize;
      break;
   case GL_POINT_DISTANCE_ATTENUATION:
      R200_STATECHANGE( rmesa, vtx );
      R200_STATECHANGE( rmesa, spr );
      R200_STATECHANGE( rmesa, ptp );
      GLfloat *fcmd = (GLfloat *)rmesa->hw.ptp.cmd;
      rmesa->hw.spr.cmd[SPR_POINT_SPRITE_CNTL] &=
	 ~(R200_PS_MULT_MASK | R200_PS_LIN_ATT_ZERO | R200_PS_SE_SEL_STATE);
      /* can't rely on ctx->Point._Attenuated here and test for NEW_POINT in
	 r200ValidateState looks like overkill */
      if (ctx->Point.Params[0] != 1.0 ||
	  ctx->Point.Params[1] != 0.0 ||
	  ctx->Point.Params[2] != 0.0 ||
	  (ctx->VertexProgram.Enabled && ctx->VertexProgram.PointSizeEnabled)) {
	 /* all we care for vp would be the ps_se_sel_state setting */
	 fcmd[PTP_ATT_CONST_QUAD] = ctx->Point.Params[2];
	 fcmd[PTP_ATT_CONST_LIN] = ctx->Point.Params[1];
	 fcmd[PTP_ATT_CONST_CON] = ctx->Point.Params[0];
	 rmesa->hw.spr.cmd[SPR_POINT_SPRITE_CNTL] |= R200_PS_MULT_ATTENCONST;
	 if (ctx->Point.Params[1] == 0.0)
	    rmesa->hw.spr.cmd[SPR_POINT_SPRITE_CNTL] |= R200_PS_LIN_ATT_ZERO;
/* FIXME: setting this here doesn't look quite ok - we only want to do
          that if we're actually drawing points probably */
	 rmesa->hw.vtx.cmd[VTX_TCL_OUTPUT_COMPSEL] |= R200_OUTPUT_PT_SIZE;
	 rmesa->hw.vtx.cmd[VTX_TCL_OUTPUT_VTXFMT_0] |= R200_VTX_POINT_SIZE;
      }
      else {
	 rmesa->hw.spr.cmd[SPR_POINT_SPRITE_CNTL] |=
	    R200_PS_SE_SEL_STATE | R200_PS_MULT_CONST;
	 rmesa->hw.vtx.cmd[VTX_TCL_OUTPUT_COMPSEL] &= ~R200_OUTPUT_PT_SIZE;
	 rmesa->hw.vtx.cmd[VTX_TCL_OUTPUT_VTXFMT_0] &= ~R200_VTX_POINT_SIZE;
      }
      break;
   case GL_POINT_FADE_THRESHOLD_SIZE:
      /* don't support multisampling, so doesn't matter. */
      break;
   /* can't do these but don't need them.
   case GL_POINT_SPRITE_R_MODE_NV:
   case GL_POINT_SPRITE_COORD_ORIGIN: */
   default:
      fprintf(stderr, "bad pname parameter in r200PointParameter\n");
      return;
   }
}

/* =============================================================
 * Line state
 */
static void r200LineWidth( GLcontext *ctx, GLfloat widthf )
{
   r200ContextPtr rmesa = R200_CONTEXT(ctx);

   R200_STATECHANGE( rmesa, lin );
   R200_STATECHANGE( rmesa, set );

   /* Line width is stored in U6.4 format.
    */
   rmesa->hw.lin.cmd[LIN_SE_LINE_WIDTH] &= ~0xffff;
   rmesa->hw.lin.cmd[LIN_SE_LINE_WIDTH] |= (GLuint)(ctx->Line._Width * 16.0);

   if ( widthf > 1.0 ) {
      rmesa->hw.set.cmd[SET_SE_CNTL] |=  R200_WIDELINE_ENABLE;
   } else {
      rmesa->hw.set.cmd[SET_SE_CNTL] &= ~R200_WIDELINE_ENABLE;
   }
}

static void r200LineStipple( GLcontext *ctx, GLint factor, GLushort pattern )
{
   r200ContextPtr rmesa = R200_CONTEXT(ctx);

   R200_STATECHANGE( rmesa, lin );
   rmesa->hw.lin.cmd[LIN_RE_LINE_PATTERN] = 
      ((((GLuint)factor & 0xff) << 16) | ((GLuint)pattern));
}


/* =============================================================
 * Masks
 */
static void r200ColorMask( GLcontext *ctx,
			   GLboolean r, GLboolean g,
			   GLboolean b, GLboolean a )
{
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
   GLuint mask = r200PackColor( rmesa->r200Screen->cpp,
				ctx->Color.ColorMask[RCOMP],
				ctx->Color.ColorMask[GCOMP],
				ctx->Color.ColorMask[BCOMP],
				ctx->Color.ColorMask[ACOMP] );

   GLuint flag = rmesa->hw.ctx.cmd[CTX_RB3D_CNTL] & ~R200_PLANE_MASK_ENABLE;

   if (!(r && g && b && a))
      flag |= R200_PLANE_MASK_ENABLE;

   if ( rmesa->hw.ctx.cmd[CTX_RB3D_CNTL] != flag ) { 
      R200_STATECHANGE( rmesa, ctx ); 
      rmesa->hw.ctx.cmd[CTX_RB3D_CNTL] = flag; 
   } 

   if ( rmesa->hw.msk.cmd[MSK_RB3D_PLANEMASK] != mask ) {
      R200_STATECHANGE( rmesa, msk );
      rmesa->hw.msk.cmd[MSK_RB3D_PLANEMASK] = mask;
   }
}


/* =============================================================
 * Polygon state
 */

static void r200PolygonOffset( GLcontext *ctx,
			       GLfloat factor, GLfloat units )
{
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
   float_ui32_type constant =  { units * rmesa->state.depth.scale };
   float_ui32_type factoru = { factor };

/*    factor *= 2; */
/*    constant *= 2; */

/*    fprintf(stderr, "%s f:%f u:%f\n", __FUNCTION__, factor, constant); */

   R200_STATECHANGE( rmesa, zbs );
   rmesa->hw.zbs.cmd[ZBS_SE_ZBIAS_FACTOR]   = factoru.ui32;
   rmesa->hw.zbs.cmd[ZBS_SE_ZBIAS_CONSTANT] = constant.ui32;
}

static void r200PolygonStipple( GLcontext *ctx, const GLubyte *mask )
{
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
   GLuint i;
   drm_radeon_stipple_t stipple;

   /* Must flip pattern upside down.
    */
   for ( i = 0 ; i < 32 ; i++ ) {
      rmesa->state.stipple.mask[31 - i] = ((GLuint *) mask)[i];
   }

   /* TODO: push this into cmd mechanism
    */
   R200_FIREVERTICES( rmesa );
   LOCK_HARDWARE( rmesa );

   /* FIXME: Use window x,y offsets into stipple RAM.
    */
   stipple.mask = rmesa->state.stipple.mask;
   drmCommandWrite( rmesa->dri.fd, DRM_RADEON_STIPPLE, 
                    &stipple, sizeof(stipple) );
   UNLOCK_HARDWARE( rmesa );
}

static void r200PolygonMode( GLcontext *ctx, GLenum face, GLenum mode )
{
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
   GLboolean flag = (ctx->_TriangleCaps & DD_TRI_UNFILLED) != 0;

   /* Can't generally do unfilled via tcl, but some good special
    * cases work. 
    */
   TCL_FALLBACK( ctx, R200_TCL_FALLBACK_UNFILLED, flag);
   if (rmesa->TclFallback) {
      r200ChooseRenderState( ctx );
      r200ChooseVertexState( ctx );
   }
}


/* =============================================================
 * Rendering attributes
 *
 * We really don't want to recalculate all this every time we bind a
 * texture.  These things shouldn't change all that often, so it makes
 * sense to break them out of the core texture state update routines.
 */

/* Examine lighting and texture state to determine if separate specular
 * should be enabled.
 */
static void r200UpdateSpecular( GLcontext *ctx )
{
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
   u_int32_t p = rmesa->hw.ctx.cmd[CTX_PP_CNTL];

   R200_STATECHANGE( rmesa, tcl );
   R200_STATECHANGE( rmesa, vtx );

   rmesa->hw.vtx.cmd[VTX_TCL_OUTPUT_VTXFMT_0] &= ~(3<<R200_VTX_COLOR_0_SHIFT);
   rmesa->hw.vtx.cmd[VTX_TCL_OUTPUT_VTXFMT_0] &= ~(3<<R200_VTX_COLOR_1_SHIFT);
   rmesa->hw.vtx.cmd[VTX_TCL_OUTPUT_COMPSEL] &= ~R200_OUTPUT_COLOR_0;
   rmesa->hw.vtx.cmd[VTX_TCL_OUTPUT_COMPSEL] &= ~R200_OUTPUT_COLOR_1;
   rmesa->hw.tcl.cmd[TCL_LIGHT_MODEL_CTL_0] &= ~R200_LIGHTING_ENABLE;

   p &= ~R200_SPECULAR_ENABLE;

   rmesa->hw.tcl.cmd[TCL_LIGHT_MODEL_CTL_0] |= R200_DIFFUSE_SPECULAR_COMBINE;


   if (ctx->Light.Enabled &&
       ctx->Light.Model.ColorControl == GL_SEPARATE_SPECULAR_COLOR) {
      rmesa->hw.vtx.cmd[VTX_TCL_OUTPUT_VTXFMT_0] |= 
	 ((R200_VTX_FP_RGBA << R200_VTX_COLOR_0_SHIFT) |
	  (R200_VTX_FP_RGBA << R200_VTX_COLOR_1_SHIFT));	
      rmesa->hw.vtx.cmd[VTX_TCL_OUTPUT_COMPSEL] |= R200_OUTPUT_COLOR_0;
      rmesa->hw.vtx.cmd[VTX_TCL_OUTPUT_COMPSEL] |= R200_OUTPUT_COLOR_1;
      rmesa->hw.tcl.cmd[TCL_LIGHT_MODEL_CTL_0] |= R200_LIGHTING_ENABLE;
      p |=  R200_SPECULAR_ENABLE;
      rmesa->hw.tcl.cmd[TCL_LIGHT_MODEL_CTL_0] &= 
	 ~R200_DIFFUSE_SPECULAR_COMBINE;
   }
   else if (ctx->Light.Enabled) {
      rmesa->hw.vtx.cmd[VTX_TCL_OUTPUT_VTXFMT_0] |= 
	 ((R200_VTX_FP_RGBA << R200_VTX_COLOR_0_SHIFT));	
      rmesa->hw.vtx.cmd[VTX_TCL_OUTPUT_COMPSEL] |= R200_OUTPUT_COLOR_0;
      rmesa->hw.tcl.cmd[TCL_LIGHT_MODEL_CTL_0] |= R200_LIGHTING_ENABLE;
   } else if (ctx->Fog.ColorSumEnabled ) {
      rmesa->hw.vtx.cmd[VTX_TCL_OUTPUT_VTXFMT_0] |= 
	 ((R200_VTX_FP_RGBA << R200_VTX_COLOR_0_SHIFT) |
	  (R200_VTX_FP_RGBA << R200_VTX_COLOR_1_SHIFT));	
      p |=  R200_SPECULAR_ENABLE;
   } else {
      rmesa->hw.vtx.cmd[VTX_TCL_OUTPUT_VTXFMT_0] |= 
	 ((R200_VTX_FP_RGBA << R200_VTX_COLOR_0_SHIFT));	
   }

   if (ctx->Fog.Enabled) {
      rmesa->hw.vtx.cmd[VTX_TCL_OUTPUT_VTXFMT_0] |= 
	 ((R200_VTX_FP_RGBA << R200_VTX_COLOR_1_SHIFT));	
      rmesa->hw.vtx.cmd[VTX_TCL_OUTPUT_COMPSEL] |= R200_OUTPUT_COLOR_1;
   }

   if ( rmesa->hw.ctx.cmd[CTX_PP_CNTL] != p ) {
      R200_STATECHANGE( rmesa, ctx );
      rmesa->hw.ctx.cmd[CTX_PP_CNTL] = p;
   }

   /* Update vertex/render formats
    */
   if (rmesa->TclFallback) { 
      r200ChooseRenderState( ctx );
      r200ChooseVertexState( ctx );
   }
}


/* =============================================================
 * Materials
 */


/* Update on colormaterial, material emmissive/ambient, 
 * lightmodel.globalambient
 */
static void update_global_ambient( GLcontext *ctx )
{
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
   float *fcmd = (float *)R200_DB_STATE( glt );

   /* Need to do more if both emmissive & ambient are PREMULT:
    * I believe this is not nessary when using source_material. This condition thus
    * will never happen currently, and the function has no dependencies on materials now
    */
   if ((rmesa->hw.tcl.cmd[TCL_LIGHT_MODEL_CTL_1] &
       ((3 << R200_FRONT_EMISSIVE_SOURCE_SHIFT) |
	(3 << R200_FRONT_AMBIENT_SOURCE_SHIFT))) == 0) 
   {
      COPY_3V( &fcmd[GLT_RED], 
	       ctx->Light.Material.Attrib[MAT_ATTRIB_FRONT_EMISSION]);
      ACC_SCALE_3V( &fcmd[GLT_RED],
		   ctx->Light.Model.Ambient,
		   ctx->Light.Material.Attrib[MAT_ATTRIB_FRONT_AMBIENT]);
   } 
   else
   {
      COPY_3V( &fcmd[GLT_RED], ctx->Light.Model.Ambient );
   }
   
   R200_DB_STATECHANGE(rmesa, &rmesa->hw.glt);
}

/* Update on change to 
 *    - light[p].colors
 *    - light[p].enabled
 */
static void update_light_colors( GLcontext *ctx, GLuint p )
{
   struct gl_light *l = &ctx->Light.Light[p];

/*     fprintf(stderr, "%s\n", __FUNCTION__); */

   if (l->Enabled) {
      r200ContextPtr rmesa = R200_CONTEXT(ctx);
      float *fcmd = (float *)R200_DB_STATE( lit[p] );

      COPY_4V( &fcmd[LIT_AMBIENT_RED], l->Ambient );	 
      COPY_4V( &fcmd[LIT_DIFFUSE_RED], l->Diffuse );
      COPY_4V( &fcmd[LIT_SPECULAR_RED], l->Specular );
      
      R200_DB_STATECHANGE( rmesa, &rmesa->hw.lit[p] );
   }
}

static void r200ColorMaterial( GLcontext *ctx, GLenum face, GLenum mode )
{
      r200ContextPtr rmesa = R200_CONTEXT(ctx);
      GLuint light_model_ctl1 = rmesa->hw.tcl.cmd[TCL_LIGHT_MODEL_CTL_1];
      light_model_ctl1 &= ~((0xf << R200_FRONT_EMISSIVE_SOURCE_SHIFT) |
			   (0xf << R200_FRONT_AMBIENT_SOURCE_SHIFT) |
			   (0xf << R200_FRONT_DIFFUSE_SOURCE_SHIFT) |
		   (0xf << R200_FRONT_SPECULAR_SOURCE_SHIFT) |
		   (0xf << R200_BACK_EMISSIVE_SOURCE_SHIFT) |
		   (0xf << R200_BACK_AMBIENT_SOURCE_SHIFT) |
		   (0xf << R200_BACK_DIFFUSE_SOURCE_SHIFT) |
		   (0xf << R200_BACK_SPECULAR_SOURCE_SHIFT));

   if (ctx->Light.ColorMaterialEnabled) {
      GLuint mask = ctx->Light.ColorMaterialBitmask;
   
      if (mask & MAT_BIT_FRONT_EMISSION) {
	 light_model_ctl1 |= (R200_LM1_SOURCE_VERTEX_COLOR_0 <<
			     R200_FRONT_EMISSIVE_SOURCE_SHIFT);
      }
      else
	 light_model_ctl1 |= (R200_LM1_SOURCE_MATERIAL_0 <<
			     R200_FRONT_EMISSIVE_SOURCE_SHIFT);

      if (mask & MAT_BIT_FRONT_AMBIENT) {
	 light_model_ctl1 |= (R200_LM1_SOURCE_VERTEX_COLOR_0 <<
			     R200_FRONT_AMBIENT_SOURCE_SHIFT);
      }
      else
         light_model_ctl1 |= (R200_LM1_SOURCE_MATERIAL_0 <<
			     R200_FRONT_AMBIENT_SOURCE_SHIFT);
	 
      if (mask & MAT_BIT_FRONT_DIFFUSE) {
	 light_model_ctl1 |= (R200_LM1_SOURCE_VERTEX_COLOR_0 <<
			     R200_FRONT_DIFFUSE_SOURCE_SHIFT);
      }
      else
         light_model_ctl1 |= (R200_LM1_SOURCE_MATERIAL_0 <<
			     R200_FRONT_DIFFUSE_SOURCE_SHIFT);
   
      if (mask & MAT_BIT_FRONT_SPECULAR) {
	 light_model_ctl1 |= (R200_LM1_SOURCE_VERTEX_COLOR_0 <<
			     R200_FRONT_SPECULAR_SOURCE_SHIFT);
      }
      else {
         light_model_ctl1 |= (R200_LM1_SOURCE_MATERIAL_0 <<
			     R200_FRONT_SPECULAR_SOURCE_SHIFT);
      }
   
      if (mask & MAT_BIT_BACK_EMISSION) {
	 light_model_ctl1 |= (R200_LM1_SOURCE_VERTEX_COLOR_0 <<
			     R200_BACK_EMISSIVE_SOURCE_SHIFT);
      }

      else light_model_ctl1 |= (R200_LM1_SOURCE_MATERIAL_1 <<
			     R200_BACK_EMISSIVE_SOURCE_SHIFT);

      if (mask & MAT_BIT_BACK_AMBIENT) {
	 light_model_ctl1 |= (R200_LM1_SOURCE_VERTEX_COLOR_0 <<
			     R200_BACK_AMBIENT_SOURCE_SHIFT);
      }
      else light_model_ctl1 |= (R200_LM1_SOURCE_MATERIAL_1 <<
			     R200_BACK_AMBIENT_SOURCE_SHIFT);

      if (mask & MAT_BIT_BACK_DIFFUSE) {
	 light_model_ctl1 |= (R200_LM1_SOURCE_VERTEX_COLOR_0 <<
			     R200_BACK_DIFFUSE_SOURCE_SHIFT);
   }
      else light_model_ctl1 |= (R200_LM1_SOURCE_MATERIAL_1 <<
			     R200_BACK_DIFFUSE_SOURCE_SHIFT);

      if (mask & MAT_BIT_BACK_SPECULAR) {
	 light_model_ctl1 |= (R200_LM1_SOURCE_VERTEX_COLOR_0 <<
			     R200_BACK_SPECULAR_SOURCE_SHIFT);
      }
      else {
         light_model_ctl1 |= (R200_LM1_SOURCE_MATERIAL_1 <<
			     R200_BACK_SPECULAR_SOURCE_SHIFT);
      }
      }
   else {
       /* Default to SOURCE_MATERIAL:
        */
     light_model_ctl1 |=
        (R200_LM1_SOURCE_MATERIAL_0 << R200_FRONT_EMISSIVE_SOURCE_SHIFT) |
        (R200_LM1_SOURCE_MATERIAL_0 << R200_FRONT_AMBIENT_SOURCE_SHIFT) |
        (R200_LM1_SOURCE_MATERIAL_0 << R200_FRONT_DIFFUSE_SOURCE_SHIFT) |
        (R200_LM1_SOURCE_MATERIAL_0 << R200_FRONT_SPECULAR_SOURCE_SHIFT) |
        (R200_LM1_SOURCE_MATERIAL_1 << R200_BACK_EMISSIVE_SOURCE_SHIFT) |
        (R200_LM1_SOURCE_MATERIAL_1 << R200_BACK_AMBIENT_SOURCE_SHIFT) |
        (R200_LM1_SOURCE_MATERIAL_1 << R200_BACK_DIFFUSE_SOURCE_SHIFT) |
        (R200_LM1_SOURCE_MATERIAL_1 << R200_BACK_SPECULAR_SOURCE_SHIFT);
   }

   if (light_model_ctl1 != rmesa->hw.tcl.cmd[TCL_LIGHT_MODEL_CTL_1]) {
      R200_STATECHANGE( rmesa, tcl );
      rmesa->hw.tcl.cmd[TCL_LIGHT_MODEL_CTL_1] = light_model_ctl1;
   }
   
   
}

void r200UpdateMaterial( GLcontext *ctx )
{
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
   GLfloat (*mat)[4] = ctx->Light.Material.Attrib;
   GLfloat *fcmd = (GLfloat *)R200_DB_STATE( mtl[0] );
   GLfloat *fcmd2 = (GLfloat *)R200_DB_STATE( mtl[1] );
   GLuint mask = ~0;
   
   /* Might be possible and faster to update everything unconditionally? */
   if (ctx->Light.ColorMaterialEnabled)
      mask &= ~ctx->Light.ColorMaterialBitmask;

   if (R200_DEBUG & DEBUG_STATE)
      fprintf(stderr, "%s\n", __FUNCTION__);

   if (mask & MAT_BIT_FRONT_EMISSION) {
      fcmd[MTL_EMMISSIVE_RED]   = mat[MAT_ATTRIB_FRONT_EMISSION][0];
      fcmd[MTL_EMMISSIVE_GREEN] = mat[MAT_ATTRIB_FRONT_EMISSION][1];
      fcmd[MTL_EMMISSIVE_BLUE]  = mat[MAT_ATTRIB_FRONT_EMISSION][2];
      fcmd[MTL_EMMISSIVE_ALPHA] = mat[MAT_ATTRIB_FRONT_EMISSION][3];
   }
   if (mask & MAT_BIT_FRONT_AMBIENT) {
      fcmd[MTL_AMBIENT_RED]     = mat[MAT_ATTRIB_FRONT_AMBIENT][0];
      fcmd[MTL_AMBIENT_GREEN]   = mat[MAT_ATTRIB_FRONT_AMBIENT][1];
      fcmd[MTL_AMBIENT_BLUE]    = mat[MAT_ATTRIB_FRONT_AMBIENT][2];
      fcmd[MTL_AMBIENT_ALPHA]   = mat[MAT_ATTRIB_FRONT_AMBIENT][3];
   }
   if (mask & MAT_BIT_FRONT_DIFFUSE) {
      fcmd[MTL_DIFFUSE_RED]     = mat[MAT_ATTRIB_FRONT_DIFFUSE][0];
      fcmd[MTL_DIFFUSE_GREEN]   = mat[MAT_ATTRIB_FRONT_DIFFUSE][1];
      fcmd[MTL_DIFFUSE_BLUE]    = mat[MAT_ATTRIB_FRONT_DIFFUSE][2];
      fcmd[MTL_DIFFUSE_ALPHA]   = mat[MAT_ATTRIB_FRONT_DIFFUSE][3];
   }
   if (mask & MAT_BIT_FRONT_SPECULAR) {
      fcmd[MTL_SPECULAR_RED]    = mat[MAT_ATTRIB_FRONT_SPECULAR][0];
      fcmd[MTL_SPECULAR_GREEN]  = mat[MAT_ATTRIB_FRONT_SPECULAR][1];
      fcmd[MTL_SPECULAR_BLUE]   = mat[MAT_ATTRIB_FRONT_SPECULAR][2];
      fcmd[MTL_SPECULAR_ALPHA]  = mat[MAT_ATTRIB_FRONT_SPECULAR][3];
   }
   if (mask & MAT_BIT_FRONT_SHININESS) {
      fcmd[MTL_SHININESS]       = mat[MAT_ATTRIB_FRONT_SHININESS][0];
   }

   if (mask & MAT_BIT_BACK_EMISSION) {
      fcmd2[MTL_EMMISSIVE_RED]   = mat[MAT_ATTRIB_BACK_EMISSION][0];
      fcmd2[MTL_EMMISSIVE_GREEN] = mat[MAT_ATTRIB_BACK_EMISSION][1];
      fcmd2[MTL_EMMISSIVE_BLUE]  = mat[MAT_ATTRIB_BACK_EMISSION][2];
      fcmd2[MTL_EMMISSIVE_ALPHA] = mat[MAT_ATTRIB_BACK_EMISSION][3];
   }
   if (mask & MAT_BIT_BACK_AMBIENT) {
      fcmd2[MTL_AMBIENT_RED]     = mat[MAT_ATTRIB_BACK_AMBIENT][0];
      fcmd2[MTL_AMBIENT_GREEN]   = mat[MAT_ATTRIB_BACK_AMBIENT][1];
      fcmd2[MTL_AMBIENT_BLUE]    = mat[MAT_ATTRIB_BACK_AMBIENT][2];
      fcmd2[MTL_AMBIENT_ALPHA]   = mat[MAT_ATTRIB_BACK_AMBIENT][3];
   }
   if (mask & MAT_BIT_BACK_DIFFUSE) {
      fcmd2[MTL_DIFFUSE_RED]     = mat[MAT_ATTRIB_BACK_DIFFUSE][0];
      fcmd2[MTL_DIFFUSE_GREEN]   = mat[MAT_ATTRIB_BACK_DIFFUSE][1];
      fcmd2[MTL_DIFFUSE_BLUE]    = mat[MAT_ATTRIB_BACK_DIFFUSE][2];
      fcmd2[MTL_DIFFUSE_ALPHA]   = mat[MAT_ATTRIB_BACK_DIFFUSE][3];
   }
   if (mask & MAT_BIT_BACK_SPECULAR) {
      fcmd2[MTL_SPECULAR_RED]    = mat[MAT_ATTRIB_BACK_SPECULAR][0];
      fcmd2[MTL_SPECULAR_GREEN]  = mat[MAT_ATTRIB_BACK_SPECULAR][1];
      fcmd2[MTL_SPECULAR_BLUE]   = mat[MAT_ATTRIB_BACK_SPECULAR][2];
      fcmd2[MTL_SPECULAR_ALPHA]  = mat[MAT_ATTRIB_BACK_SPECULAR][3];
   }
   if (mask & MAT_BIT_BACK_SHININESS) {
      fcmd2[MTL_SHININESS]       = mat[MAT_ATTRIB_BACK_SHININESS][0];
   }

   R200_DB_STATECHANGE( rmesa, &rmesa->hw.mtl[0] );
   R200_DB_STATECHANGE( rmesa, &rmesa->hw.mtl[1] );

   /* currently material changes cannot trigger a global ambient change, I believe this is correct
    update_global_ambient( ctx ); */
}

/* _NEW_LIGHT
 * _NEW_MODELVIEW
 * _MESA_NEW_NEED_EYE_COORDS
 *
 * Uses derived state from mesa:
 *       _VP_inf_norm
 *       _h_inf_norm
 *       _Position
 *       _NormDirection
 *       _ModelViewInvScale
 *       _NeedEyeCoords
 *       _EyeZDir
 *
 * which are calculated in light.c and are correct for the current
 * lighting space (model or eye), hence dependencies on _NEW_MODELVIEW
 * and _MESA_NEW_NEED_EYE_COORDS.  
 */
static void update_light( GLcontext *ctx )
{
   r200ContextPtr rmesa = R200_CONTEXT(ctx);

   /* Have to check these, or have an automatic shortcircuit mechanism
    * to remove noop statechanges. (Or just do a better job on the
    * front end).
    */
   {
      GLuint tmp = rmesa->hw.tcl.cmd[TCL_LIGHT_MODEL_CTL_0];

      if (ctx->_NeedEyeCoords)
	 tmp &= ~R200_LIGHT_IN_MODELSPACE;
      else
	 tmp |= R200_LIGHT_IN_MODELSPACE;
      
      if (tmp != rmesa->hw.tcl.cmd[TCL_LIGHT_MODEL_CTL_0]) 
      {
	 R200_STATECHANGE( rmesa, tcl );
	 rmesa->hw.tcl.cmd[TCL_LIGHT_MODEL_CTL_0] = tmp;
      }
   }

   {
      GLfloat *fcmd = (GLfloat *)R200_DB_STATE( eye );
      fcmd[EYE_X] = ctx->_EyeZDir[0];
      fcmd[EYE_Y] = ctx->_EyeZDir[1];
      fcmd[EYE_Z] = - ctx->_EyeZDir[2];
      fcmd[EYE_RESCALE_FACTOR] = ctx->_ModelViewInvScale;
      R200_DB_STATECHANGE( rmesa, &rmesa->hw.eye );
   }



   if (ctx->Light.Enabled) {
      GLint p;
      for (p = 0 ; p < MAX_LIGHTS; p++) {
	 if (ctx->Light.Light[p].Enabled) {
	    struct gl_light *l = &ctx->Light.Light[p];
	    GLfloat *fcmd = (GLfloat *)R200_DB_STATE( lit[p] );
	    
	    if (l->EyePosition[3] == 0.0) {
	       COPY_3FV( &fcmd[LIT_POSITION_X], l->_VP_inf_norm ); 
	       COPY_3FV( &fcmd[LIT_DIRECTION_X], l->_h_inf_norm ); 
	       fcmd[LIT_POSITION_W] = 0;
	       fcmd[LIT_DIRECTION_W] = 0;
	    } else {
	       COPY_4V( &fcmd[LIT_POSITION_X], l->_Position );
	       fcmd[LIT_DIRECTION_X] = -l->_NormDirection[0];
	       fcmd[LIT_DIRECTION_Y] = -l->_NormDirection[1];
	       fcmd[LIT_DIRECTION_Z] = -l->_NormDirection[2];
	       fcmd[LIT_DIRECTION_W] = 0;
	    }

	    R200_DB_STATECHANGE( rmesa, &rmesa->hw.lit[p] );
	 }
      }
   }
}

static void r200Lightfv( GLcontext *ctx, GLenum light,
			   GLenum pname, const GLfloat *params )
{
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
   GLint p = light - GL_LIGHT0;
   struct gl_light *l = &ctx->Light.Light[p];
   GLfloat *fcmd = (GLfloat *)rmesa->hw.lit[p].cmd;
   

   switch (pname) {
   case GL_AMBIENT:		
   case GL_DIFFUSE:
   case GL_SPECULAR:
      update_light_colors( ctx, p );
      break;

   case GL_SPOT_DIRECTION: 
      /* picked up in update_light */	
      break;

   case GL_POSITION: {
      /* positions picked up in update_light, but can do flag here */	
      GLuint flag = (p&1)? R200_LIGHT_1_IS_LOCAL : R200_LIGHT_0_IS_LOCAL;
      GLuint idx = TCL_PER_LIGHT_CTL_0 + p/2;

      R200_STATECHANGE(rmesa, tcl);
      if (l->EyePosition[3] != 0.0F)
	 rmesa->hw.tcl.cmd[idx] |= flag;
      else
	 rmesa->hw.tcl.cmd[idx] &= ~flag;
      break;
   }

   case GL_SPOT_EXPONENT:
      R200_STATECHANGE(rmesa, lit[p]);
      fcmd[LIT_SPOT_EXPONENT] = params[0];
      break;

   case GL_SPOT_CUTOFF: {
      GLuint flag = (p&1) ? R200_LIGHT_1_IS_SPOT : R200_LIGHT_0_IS_SPOT;
      GLuint idx = TCL_PER_LIGHT_CTL_0 + p/2;

      R200_STATECHANGE(rmesa, lit[p]);
      fcmd[LIT_SPOT_CUTOFF] = l->_CosCutoff;

      R200_STATECHANGE(rmesa, tcl);
      if (l->SpotCutoff != 180.0F)
	 rmesa->hw.tcl.cmd[idx] |= flag;
      else
	 rmesa->hw.tcl.cmd[idx] &= ~flag;

      break;
   }

   case GL_CONSTANT_ATTENUATION:
      R200_STATECHANGE(rmesa, lit[p]);
      fcmd[LIT_ATTEN_CONST] = params[0];
      if ( params[0] == 0.0 )
	 fcmd[LIT_ATTEN_CONST_INV] = FLT_MAX;
      else
	 fcmd[LIT_ATTEN_CONST_INV] = 1.0 / params[0];
      break;
   case GL_LINEAR_ATTENUATION:
      R200_STATECHANGE(rmesa, lit[p]);
      fcmd[LIT_ATTEN_LINEAR] = params[0];
      break;
   case GL_QUADRATIC_ATTENUATION:
      R200_STATECHANGE(rmesa, lit[p]);
      fcmd[LIT_ATTEN_QUADRATIC] = params[0];
      break;
   default:
      return;
   }

   /* Set RANGE_ATTEN only when needed */
   switch (pname) {
   case GL_POSITION:
   case GL_CONSTANT_ATTENUATION:
   case GL_LINEAR_ATTENUATION:
   case GL_QUADRATIC_ATTENUATION: {
      GLuint *icmd = (GLuint *)R200_DB_STATE( tcl );
      GLuint idx = TCL_PER_LIGHT_CTL_0 + p/2;
      GLuint atten_flag = ( p&1 ) ? R200_LIGHT_1_ENABLE_RANGE_ATTEN
				  : R200_LIGHT_0_ENABLE_RANGE_ATTEN;
      GLuint atten_const_flag = ( p&1 ) ? R200_LIGHT_1_CONSTANT_RANGE_ATTEN
				  : R200_LIGHT_0_CONSTANT_RANGE_ATTEN;

      if ( l->EyePosition[3] == 0.0F ||
	   ( ( fcmd[LIT_ATTEN_CONST] == 0.0 || fcmd[LIT_ATTEN_CONST] == 1.0 ) &&
	     fcmd[LIT_ATTEN_QUADRATIC] == 0.0 && fcmd[LIT_ATTEN_LINEAR] == 0.0 ) ) {
	 /* Disable attenuation */
	 icmd[idx] &= ~atten_flag;
      } else {
	 if ( fcmd[LIT_ATTEN_QUADRATIC] == 0.0 && fcmd[LIT_ATTEN_LINEAR] == 0.0 ) {
	    /* Enable only constant portion of attenuation calculation */
	    icmd[idx] |= ( atten_flag | atten_const_flag );
	 } else {
	    /* Enable full attenuation calculation */
	    icmd[idx] &= ~atten_const_flag;
	    icmd[idx] |= atten_flag;
	 }
      }

      R200_DB_STATECHANGE( rmesa, &rmesa->hw.tcl );
      break;
   }
   default:
     break;
   }
}

static void r200UpdateLocalViewer ( GLcontext *ctx )
{
/* It looks like for the texgen modes GL_SPHERE_MAP, GL_NORMAL_MAP and
   GL_REFLECTION_MAP we need R200_LOCAL_VIEWER set (fglrx does exactly that
   for these and only these modes). This means specular highlights may turn out
   wrong in some cases when lighting is enabled but GL_LIGHT_MODEL_LOCAL_VIEWER
   is not set, though it seems to happen rarely and the effect seems quite
   subtle. May need TCL fallback to fix it completely, though I'm not sure
   how you'd identify the cases where the specular highlights indeed will
   be wrong. Don't know if fglrx does something special in that case.
*/
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
   R200_STATECHANGE( rmesa, tcl );
   if (ctx->Light.Model.LocalViewer ||
       ctx->Texture._GenFlags & TEXGEN_NEED_NORMALS)
      rmesa->hw.tcl.cmd[TCL_LIGHT_MODEL_CTL_0] |= R200_LOCAL_VIEWER;
   else
      rmesa->hw.tcl.cmd[TCL_LIGHT_MODEL_CTL_0] &= ~R200_LOCAL_VIEWER;
}

static void r200LightModelfv( GLcontext *ctx, GLenum pname,
				const GLfloat *param )
{
   r200ContextPtr rmesa = R200_CONTEXT(ctx);

   switch (pname) {
      case GL_LIGHT_MODEL_AMBIENT: 
	 update_global_ambient( ctx );
	 break;

      case GL_LIGHT_MODEL_LOCAL_VIEWER:
	 r200UpdateLocalViewer( ctx );
         break;

      case GL_LIGHT_MODEL_TWO_SIDE:
	 R200_STATECHANGE( rmesa, tcl );
	 if (ctx->Light.Model.TwoSide)
	    rmesa->hw.tcl.cmd[TCL_LIGHT_MODEL_CTL_0] |= R200_LIGHT_TWOSIDE;
	 else
	    rmesa->hw.tcl.cmd[TCL_LIGHT_MODEL_CTL_0] &= ~(R200_LIGHT_TWOSIDE);
	 if (rmesa->TclFallback) {
	    r200ChooseRenderState( ctx );
	    r200ChooseVertexState( ctx );
	 }
         break;

      case GL_LIGHT_MODEL_COLOR_CONTROL:
	 r200UpdateSpecular(ctx);
         break;

      default:
         break;
   }
}

static void r200ShadeModel( GLcontext *ctx, GLenum mode )
{
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
   GLuint s = rmesa->hw.set.cmd[SET_SE_CNTL];

   s &= ~(R200_DIFFUSE_SHADE_MASK |
	  R200_ALPHA_SHADE_MASK |
	  R200_SPECULAR_SHADE_MASK |
	  R200_FOG_SHADE_MASK |
	  R200_DISC_FOG_SHADE_MASK);

   switch ( mode ) {
   case GL_FLAT:
      s |= (R200_DIFFUSE_SHADE_FLAT |
	    R200_ALPHA_SHADE_FLAT |
	    R200_SPECULAR_SHADE_FLAT |
	    R200_FOG_SHADE_FLAT |
	    R200_DISC_FOG_SHADE_FLAT);
      break;
   case GL_SMOOTH:
      s |= (R200_DIFFUSE_SHADE_GOURAUD |
	    R200_ALPHA_SHADE_GOURAUD |
	    R200_SPECULAR_SHADE_GOURAUD |
	    R200_FOG_SHADE_GOURAUD |
	    R200_DISC_FOG_SHADE_GOURAUD);
      break;
   default:
      return;
   }

   if ( rmesa->hw.set.cmd[SET_SE_CNTL] != s ) {
      R200_STATECHANGE( rmesa, set );
      rmesa->hw.set.cmd[SET_SE_CNTL] = s;
   }
}


/* =============================================================
 * User clip planes
 */

static void r200ClipPlane( GLcontext *ctx, GLenum plane, const GLfloat *eq )
{
   GLint p = (GLint) plane - (GLint) GL_CLIP_PLANE0;
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
   GLint *ip = (GLint *)ctx->Transform._ClipUserPlane[p];

   R200_STATECHANGE( rmesa, ucp[p] );
   rmesa->hw.ucp[p].cmd[UCP_X] = ip[0];
   rmesa->hw.ucp[p].cmd[UCP_Y] = ip[1];
   rmesa->hw.ucp[p].cmd[UCP_Z] = ip[2];
   rmesa->hw.ucp[p].cmd[UCP_W] = ip[3];
}

static void r200UpdateClipPlanes( GLcontext *ctx )
{
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
   GLuint p;

   for (p = 0; p < ctx->Const.MaxClipPlanes; p++) {
      if (ctx->Transform.ClipPlanesEnabled & (1 << p)) {
	 GLint *ip = (GLint *)ctx->Transform._ClipUserPlane[p];

	 R200_STATECHANGE( rmesa, ucp[p] );
	 rmesa->hw.ucp[p].cmd[UCP_X] = ip[0];
	 rmesa->hw.ucp[p].cmd[UCP_Y] = ip[1];
	 rmesa->hw.ucp[p].cmd[UCP_Z] = ip[2];
	 rmesa->hw.ucp[p].cmd[UCP_W] = ip[3];
      }
   }
}


/* =============================================================
 * Stencil
 */

static void
r200StencilFuncSeparate( GLcontext *ctx, GLenum face, GLenum func,
                         GLint ref, GLuint mask )
{
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
   GLuint refmask = (((ctx->Stencil.Ref[0] & 0xff) << R200_STENCIL_REF_SHIFT) |
		     ((ctx->Stencil.ValueMask[0] & 0xff) << R200_STENCIL_MASK_SHIFT));

   R200_STATECHANGE( rmesa, ctx );
   R200_STATECHANGE( rmesa, msk );

   rmesa->hw.ctx.cmd[CTX_RB3D_ZSTENCILCNTL] &= ~R200_STENCIL_TEST_MASK;
   rmesa->hw.msk.cmd[MSK_RB3D_STENCILREFMASK] &= ~(R200_STENCIL_REF_MASK|
						   R200_STENCIL_VALUE_MASK);

   switch ( ctx->Stencil.Function[0] ) {
   case GL_NEVER:
      rmesa->hw.ctx.cmd[CTX_RB3D_ZSTENCILCNTL] |= R200_STENCIL_TEST_NEVER;
      break;
   case GL_LESS:
      rmesa->hw.ctx.cmd[CTX_RB3D_ZSTENCILCNTL] |= R200_STENCIL_TEST_LESS;
      break;
   case GL_EQUAL:
      rmesa->hw.ctx.cmd[CTX_RB3D_ZSTENCILCNTL] |= R200_STENCIL_TEST_EQUAL;
      break;
   case GL_LEQUAL:
      rmesa->hw.ctx.cmd[CTX_RB3D_ZSTENCILCNTL] |= R200_STENCIL_TEST_LEQUAL;
      break;
   case GL_GREATER:
      rmesa->hw.ctx.cmd[CTX_RB3D_ZSTENCILCNTL] |= R200_STENCIL_TEST_GREATER;
      break;
   case GL_NOTEQUAL:
      rmesa->hw.ctx.cmd[CTX_RB3D_ZSTENCILCNTL] |= R200_STENCIL_TEST_NEQUAL;
      break;
   case GL_GEQUAL:
      rmesa->hw.ctx.cmd[CTX_RB3D_ZSTENCILCNTL] |= R200_STENCIL_TEST_GEQUAL;
      break;
   case GL_ALWAYS:
      rmesa->hw.ctx.cmd[CTX_RB3D_ZSTENCILCNTL] |= R200_STENCIL_TEST_ALWAYS;
      break;
   }

   rmesa->hw.msk.cmd[MSK_RB3D_STENCILREFMASK] |= refmask;
}

static void
r200StencilMaskSeparate( GLcontext *ctx, GLenum face, GLuint mask )
{
   r200ContextPtr rmesa = R200_CONTEXT(ctx);

   R200_STATECHANGE( rmesa, msk );
   rmesa->hw.msk.cmd[MSK_RB3D_STENCILREFMASK] &= ~R200_STENCIL_WRITE_MASK;
   rmesa->hw.msk.cmd[MSK_RB3D_STENCILREFMASK] |=
      ((ctx->Stencil.WriteMask[0] & 0xff) << R200_STENCIL_WRITEMASK_SHIFT);
}

static void
r200StencilOpSeparate( GLcontext *ctx, GLenum face, GLenum fail,
                       GLenum zfail, GLenum zpass )
{
   r200ContextPtr rmesa = R200_CONTEXT(ctx);

   R200_STATECHANGE( rmesa, ctx );
   rmesa->hw.ctx.cmd[CTX_RB3D_ZSTENCILCNTL] &= ~(R200_STENCIL_FAIL_MASK |
					       R200_STENCIL_ZFAIL_MASK |
					       R200_STENCIL_ZPASS_MASK);

   switch ( ctx->Stencil.FailFunc[0] ) {
   case GL_KEEP:
      rmesa->hw.ctx.cmd[CTX_RB3D_ZSTENCILCNTL] |= R200_STENCIL_FAIL_KEEP;
      break;
   case GL_ZERO:
      rmesa->hw.ctx.cmd[CTX_RB3D_ZSTENCILCNTL] |= R200_STENCIL_FAIL_ZERO;
      break;
   case GL_REPLACE:
      rmesa->hw.ctx.cmd[CTX_RB3D_ZSTENCILCNTL] |= R200_STENCIL_FAIL_REPLACE;
      break;
   case GL_INCR:
      rmesa->hw.ctx.cmd[CTX_RB3D_ZSTENCILCNTL] |= R200_STENCIL_FAIL_INC;
      break;
   case GL_DECR:
      rmesa->hw.ctx.cmd[CTX_RB3D_ZSTENCILCNTL] |= R200_STENCIL_FAIL_DEC;
      break;
   case GL_INCR_WRAP_EXT:
      rmesa->hw.ctx.cmd[CTX_RB3D_ZSTENCILCNTL] |= R200_STENCIL_FAIL_INC_WRAP;
      break;
   case GL_DECR_WRAP_EXT:
      rmesa->hw.ctx.cmd[CTX_RB3D_ZSTENCILCNTL] |= R200_STENCIL_FAIL_DEC_WRAP;
      break;
   case GL_INVERT:
      rmesa->hw.ctx.cmd[CTX_RB3D_ZSTENCILCNTL] |= R200_STENCIL_FAIL_INVERT;
      break;
   }

   switch ( ctx->Stencil.ZFailFunc[0] ) {
   case GL_KEEP:
      rmesa->hw.ctx.cmd[CTX_RB3D_ZSTENCILCNTL] |= R200_STENCIL_ZFAIL_KEEP;
      break;
   case GL_ZERO:
      rmesa->hw.ctx.cmd[CTX_RB3D_ZSTENCILCNTL] |= R200_STENCIL_ZFAIL_ZERO;
      break;
   case GL_REPLACE:
      rmesa->hw.ctx.cmd[CTX_RB3D_ZSTENCILCNTL] |= R200_STENCIL_ZFAIL_REPLACE;
      break;
   case GL_INCR:
      rmesa->hw.ctx.cmd[CTX_RB3D_ZSTENCILCNTL] |= R200_STENCIL_ZFAIL_INC;
      break;
   case GL_DECR:
      rmesa->hw.ctx.cmd[CTX_RB3D_ZSTENCILCNTL] |= R200_STENCIL_ZFAIL_DEC;
      break;
   case GL_INCR_WRAP_EXT:
      rmesa->hw.ctx.cmd[CTX_RB3D_ZSTENCILCNTL] |= R200_STENCIL_ZFAIL_INC_WRAP;
      break;
   case GL_DECR_WRAP_EXT:
      rmesa->hw.ctx.cmd[CTX_RB3D_ZSTENCILCNTL] |= R200_STENCIL_ZFAIL_DEC_WRAP;
      break;
   case GL_INVERT:
      rmesa->hw.ctx.cmd[CTX_RB3D_ZSTENCILCNTL] |= R200_STENCIL_ZFAIL_INVERT;
      break;
   }

   switch ( ctx->Stencil.ZPassFunc[0] ) {
   case GL_KEEP:
      rmesa->hw.ctx.cmd[CTX_RB3D_ZSTENCILCNTL] |= R200_STENCIL_ZPASS_KEEP;
      break;
   case GL_ZERO:
      rmesa->hw.ctx.cmd[CTX_RB3D_ZSTENCILCNTL] |= R200_STENCIL_ZPASS_ZERO;
      break;
   case GL_REPLACE:
      rmesa->hw.ctx.cmd[CTX_RB3D_ZSTENCILCNTL] |= R200_STENCIL_ZPASS_REPLACE;
      break;
   case GL_INCR:
      rmesa->hw.ctx.cmd[CTX_RB3D_ZSTENCILCNTL] |= R200_STENCIL_ZPASS_INC;
      break;
   case GL_DECR:
      rmesa->hw.ctx.cmd[CTX_RB3D_ZSTENCILCNTL] |= R200_STENCIL_ZPASS_DEC;
      break;
   case GL_INCR_WRAP_EXT:
      rmesa->hw.ctx.cmd[CTX_RB3D_ZSTENCILCNTL] |= R200_STENCIL_ZPASS_INC_WRAP;
      break;
   case GL_DECR_WRAP_EXT:
      rmesa->hw.ctx.cmd[CTX_RB3D_ZSTENCILCNTL] |= R200_STENCIL_ZPASS_DEC_WRAP;
      break;
   case GL_INVERT:
      rmesa->hw.ctx.cmd[CTX_RB3D_ZSTENCILCNTL] |= R200_STENCIL_ZPASS_INVERT;
      break;
   }
}

static void r200ClearStencil( GLcontext *ctx, GLint s )
{
   r200ContextPtr rmesa = R200_CONTEXT(ctx);

   rmesa->state.stencil.clear = 
      ((GLuint) (ctx->Stencil.Clear & 0xff) |
       (0xff << R200_STENCIL_MASK_SHIFT) |
       ((ctx->Stencil.WriteMask[0] & 0xff) << R200_STENCIL_WRITEMASK_SHIFT));
}


/* =============================================================
 * Window position and viewport transformation
 */

/*
 * To correctly position primitives:
 */
#define SUBPIXEL_X 0.125
#define SUBPIXEL_Y 0.125


/**
 * Called when window size or position changes or viewport or depth range
 * state is changed.  We update the hardware viewport state here.
 */
void r200UpdateWindow( GLcontext *ctx )
{
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
   __DRIdrawablePrivate *dPriv = rmesa->dri.drawable;
   GLfloat xoffset = (GLfloat)dPriv->x;
   GLfloat yoffset = (GLfloat)dPriv->y + dPriv->h;
   const GLfloat *v = ctx->Viewport._WindowMap.m;

   float_ui32_type sx = { v[MAT_SX] };
   float_ui32_type tx = { v[MAT_TX] + xoffset + SUBPIXEL_X };
   float_ui32_type sy = { - v[MAT_SY] };
   float_ui32_type ty = { (- v[MAT_TY]) + yoffset + SUBPIXEL_Y };
   float_ui32_type sz = { v[MAT_SZ] * rmesa->state.depth.scale };
   float_ui32_type tz = { v[MAT_TZ] * rmesa->state.depth.scale };

   R200_FIREVERTICES( rmesa );
   R200_STATECHANGE( rmesa, vpt );

   rmesa->hw.vpt.cmd[VPT_SE_VPORT_XSCALE]  = sx.ui32;
   rmesa->hw.vpt.cmd[VPT_SE_VPORT_XOFFSET] = tx.ui32;
   rmesa->hw.vpt.cmd[VPT_SE_VPORT_YSCALE]  = sy.ui32;
   rmesa->hw.vpt.cmd[VPT_SE_VPORT_YOFFSET] = ty.ui32;
   rmesa->hw.vpt.cmd[VPT_SE_VPORT_ZSCALE]  = sz.ui32;
   rmesa->hw.vpt.cmd[VPT_SE_VPORT_ZOFFSET] = tz.ui32;
}



static void r200Viewport( GLcontext *ctx, GLint x, GLint y,
			    GLsizei width, GLsizei height )
{
   /* Don't pipeline viewport changes, conflict with window offset
    * setting below.  Could apply deltas to rescue pipelined viewport
    * values, or keep the originals hanging around.
    */
   r200UpdateWindow( ctx );
}

static void r200DepthRange( GLcontext *ctx, GLclampd nearval,
			      GLclampd farval )
{
   r200UpdateWindow( ctx );
}

void r200UpdateViewportOffset( GLcontext *ctx )
{
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
   __DRIdrawablePrivate *dPriv = rmesa->dri.drawable;
   GLfloat xoffset = (GLfloat)dPriv->x;
   GLfloat yoffset = (GLfloat)dPriv->y + dPriv->h;
   const GLfloat *v = ctx->Viewport._WindowMap.m;

   float_ui32_type tx;
   float_ui32_type ty;

   tx.f = v[MAT_TX] + xoffset + SUBPIXEL_X;
   ty.f = (- v[MAT_TY]) + yoffset + SUBPIXEL_Y;

   if ( rmesa->hw.vpt.cmd[VPT_SE_VPORT_XOFFSET] != tx.ui32 ||
	rmesa->hw.vpt.cmd[VPT_SE_VPORT_YOFFSET] != ty.ui32 )
   {
      /* Note: this should also modify whatever data the context reset
       * code uses...
       */
      R200_STATECHANGE( rmesa, vpt );
      rmesa->hw.vpt.cmd[VPT_SE_VPORT_XOFFSET] = tx.ui32;
      rmesa->hw.vpt.cmd[VPT_SE_VPORT_YOFFSET] = ty.ui32;

      /* update polygon stipple x/y screen offset */
      {
         GLuint stx, sty;
         GLuint m = rmesa->hw.msc.cmd[MSC_RE_MISC];

         m &= ~(R200_STIPPLE_X_OFFSET_MASK |
                R200_STIPPLE_Y_OFFSET_MASK);

         /* add magic offsets, then invert */
         stx = 31 - ((rmesa->dri.drawable->x - 1) & R200_STIPPLE_COORD_MASK);
         sty = 31 - ((rmesa->dri.drawable->y + rmesa->dri.drawable->h - 1)
                     & R200_STIPPLE_COORD_MASK);

         m |= ((stx << R200_STIPPLE_X_OFFSET_SHIFT) |
               (sty << R200_STIPPLE_Y_OFFSET_SHIFT));

         if ( rmesa->hw.msc.cmd[MSC_RE_MISC] != m ) {
            R200_STATECHANGE( rmesa, msc );
	    rmesa->hw.msc.cmd[MSC_RE_MISC] = m;
         }
      }
   }

   r200UpdateScissor( ctx );
}



/* =============================================================
 * Miscellaneous
 */

static void r200ClearColor( GLcontext *ctx, const GLfloat c[4] )
{
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
   GLubyte color[4];
   CLAMPED_FLOAT_TO_UBYTE(color[0], c[0]);
   CLAMPED_FLOAT_TO_UBYTE(color[1], c[1]);
   CLAMPED_FLOAT_TO_UBYTE(color[2], c[2]);
   CLAMPED_FLOAT_TO_UBYTE(color[3], c[3]);
   rmesa->state.color.clear = r200PackColor( rmesa->r200Screen->cpp,
                                             color[0], color[1],
                                             color[2], color[3] );
}


static void r200RenderMode( GLcontext *ctx, GLenum mode )
{
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
   FALLBACK( rmesa, R200_FALLBACK_RENDER_MODE, (mode != GL_RENDER) );
}


static GLuint r200_rop_tab[] = {
   R200_ROP_CLEAR,
   R200_ROP_AND,
   R200_ROP_AND_REVERSE,
   R200_ROP_COPY,
   R200_ROP_AND_INVERTED,
   R200_ROP_NOOP,
   R200_ROP_XOR,
   R200_ROP_OR,
   R200_ROP_NOR,
   R200_ROP_EQUIV,
   R200_ROP_INVERT,
   R200_ROP_OR_REVERSE,
   R200_ROP_COPY_INVERTED,
   R200_ROP_OR_INVERTED,
   R200_ROP_NAND,
   R200_ROP_SET,
};

static void r200LogicOpCode( GLcontext *ctx, GLenum opcode )
{
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
   GLuint rop = (GLuint)opcode - GL_CLEAR;

   ASSERT( rop < 16 );

   R200_STATECHANGE( rmesa, msk );
   rmesa->hw.msk.cmd[MSK_RB3D_ROPCNTL] = r200_rop_tab[rop];
}


/*
 * Set up the cliprects for either front or back-buffer drawing.
 */
void r200SetCliprects( r200ContextPtr rmesa )
{
   __DRIdrawablePrivate *const drawable = rmesa->dri.drawable;
   __DRIdrawablePrivate *const readable = rmesa->dri.readable;
   GLframebuffer *const draw_fb = (GLframebuffer*) drawable->driverPrivate;
   GLframebuffer *const read_fb = (GLframebuffer*) readable->driverPrivate;

   if (draw_fb->_ColorDrawBufferMask[0]
       == BUFFER_BIT_BACK_LEFT) {
      /* Can't ignore 2d windows if we are page flipping.
       */
      if ( drawable->numBackClipRects == 0 || rmesa->doPageFlip ) {
         rmesa->numClipRects = drawable->numClipRects;
         rmesa->pClipRects = drawable->pClipRects;
      }
      else {
         rmesa->numClipRects = drawable->numBackClipRects;
         rmesa->pClipRects = drawable->pBackClipRects;
      }
   }
   else {
     /* front buffer (or none, or multiple buffers) */
     rmesa->numClipRects = drawable->numClipRects;
     rmesa->pClipRects = drawable->pClipRects;
  }

   if ((draw_fb->Width != drawable->w) || (draw_fb->Height != drawable->h)) {
      _mesa_resize_framebuffer(rmesa->glCtx, draw_fb,
			       drawable->w, drawable->h);
      draw_fb->Initialized = GL_TRUE;
   }

   if (drawable != readable) {
      if ((read_fb->Width != readable->w) ||
	  (read_fb->Height != readable->h)) {
	 _mesa_resize_framebuffer(rmesa->glCtx, read_fb,
				  readable->w, readable->h);
	 read_fb->Initialized = GL_TRUE;
      }
   }

   if (rmesa->state.scissor.enabled)
      r200RecalcScissorRects( rmesa );

   rmesa->lastStamp = drawable->lastStamp;
}


static void r200DrawBuffer( GLcontext *ctx, GLenum mode )
{
   r200ContextPtr rmesa = R200_CONTEXT(ctx);

   if (R200_DEBUG & DEBUG_DRI)
      fprintf(stderr, "%s %s\n", __FUNCTION__,
	      _mesa_lookup_enum_by_nr( mode ));

   R200_FIREVERTICES(rmesa);	/* don't pipeline cliprect changes */

   /*
    * _ColorDrawBufferMask is easier to cope with than <mode>.
    * Check for software fallback, update cliprects.
    */
   switch ( ctx->DrawBuffer->_ColorDrawBufferMask[0] ) {
   case BUFFER_BIT_FRONT_LEFT:
   case BUFFER_BIT_BACK_LEFT:
      FALLBACK( rmesa, R200_FALLBACK_DRAW_BUFFER, GL_FALSE );
      break;
   default:
      /* 0 (GL_NONE) buffers or multiple color drawing buffers */
      FALLBACK( rmesa, R200_FALLBACK_DRAW_BUFFER, GL_TRUE );
      return;
   }

   r200SetCliprects( rmesa );

   /* We'll set the drawing engine's offset/pitch parameters later
    * when we update other state.
    */
}


static void r200ReadBuffer( GLcontext *ctx, GLenum mode )
{
   /* nothing, until we implement h/w glRead/CopyPixels or CopyTexImage */
}

/* =============================================================
 * State enable/disable
 */

static void r200Enable( GLcontext *ctx, GLenum cap, GLboolean state )
{
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
   GLuint p, flag;

   if ( R200_DEBUG & DEBUG_STATE )
      fprintf( stderr, "%s( %s = %s )\n", __FUNCTION__,
	       _mesa_lookup_enum_by_nr( cap ),
	       state ? "GL_TRUE" : "GL_FALSE" );

   switch ( cap ) {
      /* Fast track this one...
       */
   case GL_TEXTURE_1D:
   case GL_TEXTURE_2D:
   case GL_TEXTURE_3D:
      break;

   case GL_ALPHA_TEST:
      R200_STATECHANGE( rmesa, ctx );
      if (state) {
	 rmesa->hw.ctx.cmd[CTX_PP_CNTL] |= R200_ALPHA_TEST_ENABLE;
      } else {
	 rmesa->hw.ctx.cmd[CTX_PP_CNTL] &= ~R200_ALPHA_TEST_ENABLE;
      }
      break;

   case GL_BLEND:
   case GL_COLOR_LOGIC_OP:
      r200_set_blend_state( ctx );
      break;

   case GL_CLIP_PLANE0:
   case GL_CLIP_PLANE1:
   case GL_CLIP_PLANE2:
   case GL_CLIP_PLANE3:
   case GL_CLIP_PLANE4:
   case GL_CLIP_PLANE5: 
      p = cap-GL_CLIP_PLANE0;
      R200_STATECHANGE( rmesa, tcl );
      if (state) {
	 rmesa->hw.tcl.cmd[TCL_UCP_VERT_BLEND_CTL] |= (R200_UCP_ENABLE_0<<p);
	 r200ClipPlane( ctx, cap, NULL );
      }
      else {
	 rmesa->hw.tcl.cmd[TCL_UCP_VERT_BLEND_CTL] &= ~(R200_UCP_ENABLE_0<<p);
      }
      break;

   case GL_COLOR_MATERIAL:
      r200ColorMaterial( ctx, 0, 0 );
      r200UpdateMaterial( ctx );
      break;

   case GL_CULL_FACE:
      r200CullFace( ctx, 0 );
      break;

   case GL_DEPTH_TEST:
      R200_STATECHANGE(rmesa, ctx );
      if ( state ) {
	 rmesa->hw.ctx.cmd[CTX_RB3D_CNTL] |=  R200_Z_ENABLE;
      } else {
	 rmesa->hw.ctx.cmd[CTX_RB3D_CNTL] &= ~R200_Z_ENABLE;
      }
      break;

   case GL_DITHER:
      R200_STATECHANGE(rmesa, ctx );
      if ( state ) {
	 rmesa->hw.ctx.cmd[CTX_RB3D_CNTL] |=  R200_DITHER_ENABLE;
	 rmesa->hw.ctx.cmd[CTX_RB3D_CNTL] &= ~rmesa->state.color.roundEnable;
      } else {
	 rmesa->hw.ctx.cmd[CTX_RB3D_CNTL] &= ~R200_DITHER_ENABLE;
	 rmesa->hw.ctx.cmd[CTX_RB3D_CNTL] |=  rmesa->state.color.roundEnable;
      }
      break;

   case GL_FOG:
      R200_STATECHANGE(rmesa, ctx );
      if ( state ) {
	 rmesa->hw.ctx.cmd[CTX_PP_CNTL] |= R200_FOG_ENABLE;
	 r200Fogfv( ctx, GL_FOG_MODE, NULL );
      } else {
	 rmesa->hw.ctx.cmd[CTX_PP_CNTL] &= ~R200_FOG_ENABLE;
	 R200_STATECHANGE(rmesa, tcl);
	 rmesa->hw.tcl.cmd[TCL_UCP_VERT_BLEND_CTL] &= ~R200_TCL_FOG_MASK;
      }
      r200UpdateSpecular( ctx ); /* for PK_SPEC */
      if (rmesa->TclFallback) 
	 r200ChooseVertexState( ctx );
      _mesa_allow_light_in_model( ctx, !state );
      break;

   case GL_LIGHT0:
   case GL_LIGHT1:
   case GL_LIGHT2:
   case GL_LIGHT3:
   case GL_LIGHT4:
   case GL_LIGHT5:
   case GL_LIGHT6:
   case GL_LIGHT7:
      R200_STATECHANGE(rmesa, tcl);
      p = cap - GL_LIGHT0;
      if (p&1) 
	 flag = (R200_LIGHT_1_ENABLE |
		 R200_LIGHT_1_ENABLE_AMBIENT | 
		 R200_LIGHT_1_ENABLE_SPECULAR);
      else
	 flag = (R200_LIGHT_0_ENABLE |
		 R200_LIGHT_0_ENABLE_AMBIENT | 
		 R200_LIGHT_0_ENABLE_SPECULAR);

      if (state)
	 rmesa->hw.tcl.cmd[p/2 + TCL_PER_LIGHT_CTL_0] |= flag;
      else
	 rmesa->hw.tcl.cmd[p/2 + TCL_PER_LIGHT_CTL_0] &= ~flag;

      /* 
       */
      update_light_colors( ctx, p );
      break;

   case GL_LIGHTING:
      r200UpdateSpecular(ctx);
      /* for reflection map fixup - might set recheck_texgen for all units too */
      rmesa->NewGLState |= _NEW_TEXTURE;
      break;

   case GL_LINE_SMOOTH:
      R200_STATECHANGE( rmesa, ctx );
      if ( state ) {
	 rmesa->hw.ctx.cmd[CTX_PP_CNTL] |=  R200_ANTI_ALIAS_LINE;
      } else {
	 rmesa->hw.ctx.cmd[CTX_PP_CNTL] &= ~R200_ANTI_ALIAS_LINE;
      }
      break;

   case GL_LINE_STIPPLE:
      R200_STATECHANGE( rmesa, set );
      if ( state ) {
	 rmesa->hw.set.cmd[SET_RE_CNTL] |=  R200_PATTERN_ENABLE;
      } else {
	 rmesa->hw.set.cmd[SET_RE_CNTL] &= ~R200_PATTERN_ENABLE;
      }
      break;

   case GL_NORMALIZE:
      R200_STATECHANGE( rmesa, tcl );
      if ( state ) {
	 rmesa->hw.tcl.cmd[TCL_LIGHT_MODEL_CTL_0] |=  R200_NORMALIZE_NORMALS;
      } else {
	 rmesa->hw.tcl.cmd[TCL_LIGHT_MODEL_CTL_0] &= ~R200_NORMALIZE_NORMALS;
      }
      break;

      /* Pointsize registers on r200 only work for point sprites, and point smooth
       * doesn't work for point sprites (and isn't needed for 1.0 sized aa points).
       * In any case, setting pointmin == pointsizemax == 1.0 for aa points
       * is enough to satisfy conform.
       */
   case GL_POINT_SMOOTH:
      break;

      /* These don't really do anything, as we don't use the 3vtx
       * primitives yet.
       */
#if 0
   case GL_POLYGON_OFFSET_POINT:
      R200_STATECHANGE( rmesa, set );
      if ( state ) {
	 rmesa->hw.set.cmd[SET_SE_CNTL] |=  R200_ZBIAS_ENABLE_POINT;
      } else {
	 rmesa->hw.set.cmd[SET_SE_CNTL] &= ~R200_ZBIAS_ENABLE_POINT;
      }
      break;

   case GL_POLYGON_OFFSET_LINE:
      R200_STATECHANGE( rmesa, set );
      if ( state ) {
	 rmesa->hw.set.cmd[SET_SE_CNTL] |=  R200_ZBIAS_ENABLE_LINE;
      } else {
	 rmesa->hw.set.cmd[SET_SE_CNTL] &= ~R200_ZBIAS_ENABLE_LINE;
      }
      break;
#endif

   case GL_POINT_SPRITE_ARB:
      R200_STATECHANGE( rmesa, spr );
      if ( state ) {
	 int i;
	 for (i = 0; i < 6; i++) {
	    rmesa->hw.spr.cmd[SPR_POINT_SPRITE_CNTL] |=
		ctx->Point.CoordReplace[i] << (R200_PS_GEN_TEX_0_SHIFT + i);
	 }
      } else {
	 rmesa->hw.spr.cmd[SPR_POINT_SPRITE_CNTL] &= ~R200_PS_GEN_TEX_MASK;
      }
      break;

   case GL_POLYGON_OFFSET_FILL:
      R200_STATECHANGE( rmesa, set );
      if ( state ) {
	 rmesa->hw.set.cmd[SET_SE_CNTL] |=  R200_ZBIAS_ENABLE_TRI;
      } else {
	 rmesa->hw.set.cmd[SET_SE_CNTL] &= ~R200_ZBIAS_ENABLE_TRI;
      }
      break;

   case GL_POLYGON_SMOOTH:
      R200_STATECHANGE( rmesa, ctx );
      if ( state ) {
	 rmesa->hw.ctx.cmd[CTX_PP_CNTL] |=  R200_ANTI_ALIAS_POLY;
      } else {
	 rmesa->hw.ctx.cmd[CTX_PP_CNTL] &= ~R200_ANTI_ALIAS_POLY;
      }
      break;

   case GL_POLYGON_STIPPLE:
      R200_STATECHANGE(rmesa, set );
      if ( state ) {
	 rmesa->hw.set.cmd[SET_RE_CNTL] |=  R200_STIPPLE_ENABLE;
      } else {
	 rmesa->hw.set.cmd[SET_RE_CNTL] &= ~R200_STIPPLE_ENABLE;
      }
      break;

   case GL_RESCALE_NORMAL_EXT: {
      GLboolean tmp = ctx->_NeedEyeCoords ? state : !state;
      R200_STATECHANGE( rmesa, tcl );
      if ( tmp ) {
	 rmesa->hw.tcl.cmd[TCL_LIGHT_MODEL_CTL_0] |=  R200_RESCALE_NORMALS;
      } else {
	 rmesa->hw.tcl.cmd[TCL_LIGHT_MODEL_CTL_0] &= ~R200_RESCALE_NORMALS;
      }
      break;
   }

   case GL_SCISSOR_TEST:
      R200_FIREVERTICES( rmesa );
      rmesa->state.scissor.enabled = state;
      r200UpdateScissor( ctx );
      break;

   case GL_STENCIL_TEST:
      if ( rmesa->state.stencil.hwBuffer ) {
	 R200_STATECHANGE( rmesa, ctx );
	 if ( state ) {
	    rmesa->hw.ctx.cmd[CTX_RB3D_CNTL] |=  R200_STENCIL_ENABLE;
	 } else {
	    rmesa->hw.ctx.cmd[CTX_RB3D_CNTL] &= ~R200_STENCIL_ENABLE;
	 }
      } else {
	 FALLBACK( rmesa, R200_FALLBACK_STENCIL, state );
      }
      break;

   case GL_TEXTURE_GEN_Q:
   case GL_TEXTURE_GEN_R:
   case GL_TEXTURE_GEN_S:
   case GL_TEXTURE_GEN_T:
      /* Picked up in r200UpdateTextureState.
       */
      rmesa->recheck_texgen[ctx->Texture.CurrentUnit] = GL_TRUE; 
      break;

   case GL_COLOR_SUM_EXT:
      r200UpdateSpecular ( ctx );
      break;

   case GL_VERTEX_PROGRAM_ARB:
      if (!state) {
	 GLuint i;
	 rmesa->curr_vp_hw = NULL;
	 R200_STATECHANGE( rmesa, vap );
	 rmesa->hw.vap.cmd[VAP_SE_VAP_CNTL] &= ~R200_VAP_PROG_VTX_SHADER_ENABLE;
	 /* mark all tcl atoms (tcl vector state got overwritten) dirty
	    not sure about tcl scalar state - we need at least grd
	    with vert progs too.
	    ucp looks like it doesn't get overwritten (may even work
	    with vp for pos-invariant progs if we're lucky) */
	 R200_STATECHANGE( rmesa, mtl[0] );
	 R200_STATECHANGE( rmesa, mtl[1] );
	 R200_STATECHANGE( rmesa, fog );
	 R200_STATECHANGE( rmesa, glt );
	 R200_STATECHANGE( rmesa, eye );
	 for (i = R200_MTX_MV; i <= R200_MTX_TEX5; i++) {
	    R200_STATECHANGE( rmesa, mat[i] );
	 }
	 for (i = 0 ; i < 8; i++) {
	    R200_STATECHANGE( rmesa, lit[i] );
	 }
	 R200_STATECHANGE( rmesa, tcl );
	 for (i = 0; i <= ctx->Const.MaxClipPlanes; i++) {
	    if (ctx->Transform.ClipPlanesEnabled & (1 << i)) {
	       rmesa->hw.tcl.cmd[TCL_UCP_VERT_BLEND_CTL] |= (R200_UCP_ENABLE_0 << i);
	    }
/*	    else {
	       rmesa->hw.tcl.cmd[TCL_UCP_VERT_BLEND_CTL] &= ~(R200_UCP_ENABLE_0 << i);
	    }*/
	 }
	 /* ugly. Need to call everything which might change compsel. */
	 r200UpdateSpecular( ctx );
#if 0
	/* shouldn't be necessary, as it's picked up anyway in r200ValidateState (_NEW_PROGRAM),
	   but without it doom3 locks up at always the same places. Why? */
	/* FIXME: This can (and should) be replaced by a call to the TCL_STATE_FLUSH reg before
	   accessing VAP_SE_VAP_CNTL. Requires drm changes (done). Remove after some time... */
	 r200UpdateTextureState( ctx );
	 /* if we call r200UpdateTextureState we need the code below because we are calling it with
	    non-current derived enabled values which may revert the state atoms for frag progs even when
	    they already got disabled... ugh
	    Should really figure out why we need to call r200UpdateTextureState in the first place */
	 GLuint unit;
	 for (unit = 0; unit < R200_MAX_TEXTURE_UNITS; unit++) {
	    R200_STATECHANGE( rmesa, pix[unit] );
	    R200_STATECHANGE( rmesa, tex[unit] );
	    rmesa->hw.tex[unit].cmd[TEX_PP_TXFORMAT] &=
		~(R200_TXFORMAT_ST_ROUTE_MASK | R200_TXFORMAT_LOOKUP_DISABLE);
	    rmesa->hw.tex[unit].cmd[TEX_PP_TXFORMAT] |= unit << R200_TXFORMAT_ST_ROUTE_SHIFT;
	    /* need to guard this with drmSupportsFragmentShader? Should never get here if
	       we don't announce ATI_fs, right? */
	    rmesa->hw.tex[unit].cmd[TEX_PP_TXMULTI_CTL] = 0;
         }
	 R200_STATECHANGE( rmesa, cst );
	 R200_STATECHANGE( rmesa, tf );
	 rmesa->hw.cst.cmd[CST_PP_CNTL_X] = 0;
#endif
      }
      else {
	 /* picked up later */
      }
      /* call functions which change hw state based on ARB_vp enabled or not. */
      r200PointParameter( ctx, GL_POINT_DISTANCE_ATTENUATION, NULL );
      r200Fogfv( ctx, GL_FOG_COORD_SRC, NULL );
      break;

   case GL_VERTEX_PROGRAM_POINT_SIZE_ARB:
      r200PointParameter( ctx, GL_POINT_DISTANCE_ATTENUATION, NULL );
      break;

   case GL_FRAGMENT_SHADER_ATI:
      if ( !state ) {
	 /* restore normal tex env colors and make sure tex env combine will get updated
	    mark env atoms dirty (as their data was overwritten by afs even
	    if they didn't change) and restore tex coord routing */
	 GLuint unit;
	 for (unit = 0; unit < R200_MAX_TEXTURE_UNITS; unit++) {
	    R200_STATECHANGE( rmesa, pix[unit] );
	    R200_STATECHANGE( rmesa, tex[unit] );
	    rmesa->hw.tex[unit].cmd[TEX_PP_TXFORMAT] &=
		~(R200_TXFORMAT_ST_ROUTE_MASK | R200_TXFORMAT_LOOKUP_DISABLE);
	    rmesa->hw.tex[unit].cmd[TEX_PP_TXFORMAT] |= unit << R200_TXFORMAT_ST_ROUTE_SHIFT;
	    /* need to guard this with drmSupportsFragmentShader? Should never get here if
	       we don't announce ATI_fs, right? */
	    rmesa->hw.tex[unit].cmd[TEX_PP_TXMULTI_CTL] = 0;
         }
	 R200_STATECHANGE( rmesa, cst );
	 R200_STATECHANGE( rmesa, tf );
	 rmesa->hw.cst.cmd[CST_PP_CNTL_X] = 0;
      }
      else {
	 /* need to mark this dirty as pix/tf atoms have overwritten the data
	    even if the data in the atoms didn't change */
	 R200_STATECHANGE( rmesa, atf );
	 R200_STATECHANGE( rmesa, afs[1] );
	 /* everything else picked up in r200UpdateTextureState hopefully */
      }
      break;
   default:
      return;
   }
}


void r200LightingSpaceChange( GLcontext *ctx )
{
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
   GLboolean tmp;

   if (R200_DEBUG & DEBUG_STATE) 
      fprintf(stderr, "%s %d BEFORE %x\n", __FUNCTION__, ctx->_NeedEyeCoords,
	      rmesa->hw.tcl.cmd[TCL_LIGHT_MODEL_CTL_0]);

   if (ctx->_NeedEyeCoords)
      tmp = ctx->Transform.RescaleNormals;
   else
      tmp = !ctx->Transform.RescaleNormals;

   R200_STATECHANGE( rmesa, tcl );
   if ( tmp ) {
      rmesa->hw.tcl.cmd[TCL_LIGHT_MODEL_CTL_0] |=  R200_RESCALE_NORMALS;
   } else {
      rmesa->hw.tcl.cmd[TCL_LIGHT_MODEL_CTL_0] &= ~R200_RESCALE_NORMALS;
   }

   if (R200_DEBUG & DEBUG_STATE) 
      fprintf(stderr, "%s %d AFTER %x\n", __FUNCTION__, ctx->_NeedEyeCoords,
	      rmesa->hw.tcl.cmd[TCL_LIGHT_MODEL_CTL_0]);
}

/* =============================================================
 * Deferred state management - matrices, textures, other?
 */




static void upload_matrix( r200ContextPtr rmesa, GLfloat *src, int idx )
{
   float *dest = ((float *)R200_DB_STATE( mat[idx] ))+MAT_ELT_0;
   int i;


   for (i = 0 ; i < 4 ; i++) {
      *dest++ = src[i];
      *dest++ = src[i+4];
      *dest++ = src[i+8];
      *dest++ = src[i+12];
   }

   R200_DB_STATECHANGE( rmesa, &rmesa->hw.mat[idx] );
}

static void upload_matrix_t( r200ContextPtr rmesa, const GLfloat *src, int idx )
{
   float *dest = ((float *)R200_DB_STATE( mat[idx] ))+MAT_ELT_0;
   memcpy(dest, src, 16*sizeof(float));
   R200_DB_STATECHANGE( rmesa, &rmesa->hw.mat[idx] );
}


static void update_texturematrix( GLcontext *ctx )
{
   r200ContextPtr rmesa = R200_CONTEXT( ctx );
   GLuint tpc = rmesa->hw.tcg.cmd[TCG_TEX_PROC_CTL_0];
   GLuint compsel = rmesa->hw.vtx.cmd[VTX_TCL_OUTPUT_COMPSEL];
   int unit;

   if (R200_DEBUG & DEBUG_STATE) 
      fprintf(stderr, "%s before COMPSEL: %x\n", __FUNCTION__,
	      rmesa->hw.vtx.cmd[VTX_TCL_OUTPUT_COMPSEL]);

   rmesa->TexMatEnabled = 0;
   rmesa->TexMatCompSel = 0;

   for (unit = 0 ; unit < ctx->Const.MaxTextureUnits; unit++) {
      if (!ctx->Texture.Unit[unit]._ReallyEnabled) 
	 continue;

      if (ctx->TextureMatrixStack[unit].Top->type != MATRIX_IDENTITY) {
	 rmesa->TexMatEnabled |= (R200_TEXGEN_TEXMAT_0_ENABLE|
				  R200_TEXMAT_0_ENABLE) << unit;

	 rmesa->TexMatCompSel |= R200_OUTPUT_TEX_0 << unit;

	 if (rmesa->TexGenEnabled & (R200_TEXMAT_0_ENABLE << unit)) {
	    /* Need to preconcatenate any active texgen 
	     * obj/eyeplane matrices:
	     */
	    _math_matrix_mul_matrix( &rmesa->tmpmat,
				     ctx->TextureMatrixStack[unit].Top, 
				     &rmesa->TexGenMatrix[unit] );
	    upload_matrix( rmesa, rmesa->tmpmat.m, R200_MTX_TEX0+unit );
	 } 
	 else {
	    upload_matrix( rmesa, ctx->TextureMatrixStack[unit].Top->m, 
			   R200_MTX_TEX0+unit );
	 }
      }
      else if (rmesa->TexGenEnabled & (R200_TEXMAT_0_ENABLE << unit)) {
	 upload_matrix( rmesa, rmesa->TexGenMatrix[unit].m, 
			R200_MTX_TEX0+unit );
      }
   }

   tpc = (rmesa->TexMatEnabled | rmesa->TexGenEnabled);
   if (tpc != rmesa->hw.tcg.cmd[TCG_TEX_PROC_CTL_0]) {
      R200_STATECHANGE(rmesa, tcg);
      rmesa->hw.tcg.cmd[TCG_TEX_PROC_CTL_0] = tpc;
   }

   compsel &= ~R200_OUTPUT_TEX_MASK;
   compsel |= rmesa->TexMatCompSel | rmesa->TexGenCompSel;
   if (compsel != rmesa->hw.vtx.cmd[VTX_TCL_OUTPUT_COMPSEL]) {
      R200_STATECHANGE(rmesa, vtx);
      rmesa->hw.vtx.cmd[VTX_TCL_OUTPUT_COMPSEL] = compsel;
   }
}



/**
 * Tell the card where to render (offset, pitch).
 * Effected by glDrawBuffer, etc
 */
void
r200UpdateDrawBuffer(GLcontext *ctx)
{
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
   struct gl_framebuffer *fb = ctx->DrawBuffer;
   driRenderbuffer *drb;

   if (fb->_ColorDrawBufferMask[0] == BUFFER_BIT_FRONT_LEFT) {
      /* draw to front */
      drb = (driRenderbuffer *) fb->Attachment[BUFFER_FRONT_LEFT].Renderbuffer;
   }
   else if (fb->_ColorDrawBufferMask[0] == BUFFER_BIT_BACK_LEFT) {
      /* draw to back */
      drb = (driRenderbuffer *) fb->Attachment[BUFFER_BACK_LEFT].Renderbuffer;
   }
   else {
      /* drawing to multiple buffers, or none */
      return;
   }

   assert(drb);
   assert(drb->flippedPitch);

   R200_STATECHANGE( rmesa, ctx );

   /* Note: we used the (possibly) page-flipped values */
   rmesa->hw.ctx.cmd[CTX_RB3D_COLOROFFSET]
     = ((drb->flippedOffset + rmesa->r200Screen->fbLocation)
	& R200_COLOROFFSET_MASK);
   rmesa->hw.ctx.cmd[CTX_RB3D_COLORPITCH] = drb->flippedPitch;
   if (rmesa->sarea->tiling_enabled) {
      rmesa->hw.ctx.cmd[CTX_RB3D_COLORPITCH] |= R200_COLOR_TILE_ENABLE;
   }
}



void r200ValidateState( GLcontext *ctx )
{
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
   GLuint new_state = rmesa->NewGLState;

   if (new_state & (_NEW_BUFFERS | _NEW_COLOR | _NEW_PIXEL)) {
     r200UpdateDrawBuffer(ctx);
   }

   if (new_state & (_NEW_TEXTURE | _NEW_PROGRAM)) {
      r200UpdateTextureState( ctx );
      new_state |= rmesa->NewGLState; /* may add TEXTURE_MATRIX */
      r200UpdateLocalViewer( ctx );
   }

/* FIXME: don't really need most of these when vertex progs are enabled */

   /* Need an event driven matrix update?
    */
   if (new_state & (_NEW_MODELVIEW|_NEW_PROJECTION)) 
      upload_matrix( rmesa, ctx->_ModelProjectMatrix.m, R200_MTX_MVP );

   /* Need these for lighting (shouldn't upload otherwise)
    */
   if (new_state & (_NEW_MODELVIEW)) {
      upload_matrix( rmesa, ctx->ModelviewMatrixStack.Top->m, R200_MTX_MV );
      upload_matrix_t( rmesa, ctx->ModelviewMatrixStack.Top->inv, R200_MTX_IMV );
   }

   /* Does this need to be triggered on eg. modelview for
    * texgen-derived objplane/eyeplane matrices?
    */
   if (new_state & (_NEW_TEXTURE|_NEW_TEXTURE_MATRIX)) {
      update_texturematrix( ctx );
   }

   if (new_state & (_NEW_LIGHT|_NEW_MODELVIEW|_MESA_NEW_NEED_EYE_COORDS)) {
      update_light( ctx );
   }

   /* emit all active clip planes if projection matrix changes.
    */
   if (new_state & (_NEW_PROJECTION)) {
      if (ctx->Transform.ClipPlanesEnabled) 
	 r200UpdateClipPlanes( ctx );
   }

   if (new_state & (_NEW_PROGRAM|
   /* need to test for pretty much anything due to possible parameter bindings */
	_NEW_MODELVIEW|_NEW_PROJECTION|_NEW_TRANSFORM|
	_NEW_LIGHT|_NEW_TEXTURE|_NEW_TEXTURE_MATRIX|
	_NEW_FOG|_NEW_POINT|_NEW_TRACK_MATRIX)) {
      if (ctx->VertexProgram._Enabled) {
	 r200SetupVertexProg( ctx );
      }
      else TCL_FALLBACK(ctx, R200_TCL_FALLBACK_VERTEX_PROGRAM, 0);
   }

   rmesa->NewGLState = 0;
}


static void r200InvalidateState( GLcontext *ctx, GLuint new_state )
{
   _swrast_InvalidateState( ctx, new_state );
   _swsetup_InvalidateState( ctx, new_state );
   _vbo_InvalidateState( ctx, new_state );
   _tnl_InvalidateState( ctx, new_state );
   _ae_invalidate_state( ctx, new_state );
   R200_CONTEXT(ctx)->NewGLState |= new_state;
}

/* A hack.  The r200 can actually cope just fine with materials
 * between begin/ends, so fix this.
 * Should map to inputs just like the generic vertex arrays for vertex progs.
 * In theory there could still be too many and we'd still need a fallback.
 */
static GLboolean check_material( GLcontext *ctx )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   GLint i;

   for (i = _TNL_ATTRIB_MAT_FRONT_AMBIENT;
	i < _TNL_ATTRIB_MAT_BACK_INDEXES;
	i++)
      if (tnl->vb.AttribPtr[i] &&
	  tnl->vb.AttribPtr[i]->stride)
	 return GL_TRUE;

   return GL_FALSE;
}

static void r200WrapRunPipeline( GLcontext *ctx )
{
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
   GLboolean has_material;

   if (0)
      fprintf(stderr, "%s, newstate: %x\n", __FUNCTION__, rmesa->NewGLState);

   /* Validate state:
    */
   if (rmesa->NewGLState)
      r200ValidateState( ctx );

   has_material = !ctx->VertexProgram._Enabled && ctx->Light.Enabled && check_material( ctx );

   if (has_material) {
      TCL_FALLBACK( ctx, R200_TCL_FALLBACK_MATERIAL, GL_TRUE );
   }

   /* Run the pipeline.
    */ 
   _tnl_run_pipeline( ctx );

   if (has_material) {
      TCL_FALLBACK( ctx, R200_TCL_FALLBACK_MATERIAL, GL_FALSE );
   }
}


/* Initialize the driver's state functions.
 */
void r200InitStateFuncs( struct dd_function_table *functions )
{
   functions->UpdateState		= r200InvalidateState;
   functions->LightingSpaceChange	= r200LightingSpaceChange;

   functions->DrawBuffer		= r200DrawBuffer;
   functions->ReadBuffer		= r200ReadBuffer;

   functions->AlphaFunc			= r200AlphaFunc;
   functions->BlendColor		= r200BlendColor;
   functions->BlendEquationSeparate	= r200BlendEquationSeparate;
   functions->BlendFuncSeparate		= r200BlendFuncSeparate;
   functions->ClearColor		= r200ClearColor;
   functions->ClearDepth		= r200ClearDepth;
   functions->ClearIndex		= NULL;
   functions->ClearStencil		= r200ClearStencil;
   functions->ClipPlane			= r200ClipPlane;
   functions->ColorMask			= r200ColorMask;
   functions->CullFace			= r200CullFace;
   functions->DepthFunc			= r200DepthFunc;
   functions->DepthMask			= r200DepthMask;
   functions->DepthRange		= r200DepthRange;
   functions->Enable			= r200Enable;
   functions->Fogfv			= r200Fogfv;
   functions->FrontFace			= r200FrontFace;
   functions->Hint			= NULL;
   functions->IndexMask			= NULL;
   functions->LightModelfv		= r200LightModelfv;
   functions->Lightfv			= r200Lightfv;
   functions->LineStipple		= r200LineStipple;
   functions->LineWidth			= r200LineWidth;
   functions->LogicOpcode		= r200LogicOpCode;
   functions->PolygonMode		= r200PolygonMode;
   functions->PolygonOffset		= r200PolygonOffset;
   functions->PolygonStipple		= r200PolygonStipple;
   functions->PointParameterfv		= r200PointParameter;
   functions->PointSize			= r200PointSize;
   functions->RenderMode		= r200RenderMode;
   functions->Scissor			= r200Scissor;
   functions->ShadeModel		= r200ShadeModel;
   functions->StencilFuncSeparate	= r200StencilFuncSeparate;
   functions->StencilMaskSeparate	= r200StencilMaskSeparate;
   functions->StencilOpSeparate		= r200StencilOpSeparate;
   functions->Viewport			= r200Viewport;
}


void r200InitTnlFuncs( GLcontext *ctx )
{
   TNL_CONTEXT(ctx)->Driver.NotifyMaterialChange = r200UpdateMaterial;
   TNL_CONTEXT(ctx)->Driver.RunPipeline = r200WrapRunPipeline;
}
