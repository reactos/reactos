/**************************************************************************

Copyright 2000 Silicon Integrated Systems Corp, Inc., HsinChu, Taiwan.
Copyright 2003 Eric Anholt
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
/* $XFree86: xc/lib/GL/mesa/src/drv/sis/sis_ctx.c,v 1.3 2000/09/26 15:56:48 tsi Exp $ */

/*
 * Authors:
 *    Sung-Ching Lin <sclin@sis.com.tw>
 *    Eric Anholt <anholt@FreeBSD.org>
 */

#include "sis_context.h"
#include "sis_state.h"
#include "sis_tris.h"
#include "sis_lock.h"
#include "sis_tex.h"

#include "context.h"
#include "enums.h"
#include "colormac.h"
#include "swrast/swrast.h"
#include "vbo/vbo.h"
#include "tnl/tnl.h"
#include "swrast_setup/swrast_setup.h"

#include "tnl/t_pipeline.h"

/* =============================================================
 * Alpha blending
 */

static void
sisDDAlphaFunc( GLcontext * ctx, GLenum func, GLfloat ref )
{
   sisContextPtr smesa = SIS_CONTEXT(ctx);
   GLubyte refbyte;

   __GLSiSHardware *prev = &smesa->prev;
   __GLSiSHardware *current = &smesa->current;

   CLAMPED_FLOAT_TO_UBYTE(refbyte, ref);
   current->hwAlpha = refbyte << 16;

   /* Alpha Test function */
   switch (func)
   {
   case GL_NEVER:
      current->hwAlpha |= SiS_ALPHA_NEVER;
      break;
   case GL_LESS:
      current->hwAlpha |= SiS_ALPHA_LESS;
      break;
   case GL_EQUAL:
      current->hwAlpha |= SiS_ALPHA_EQUAL;
      break;
   case GL_LEQUAL:
      current->hwAlpha |= SiS_ALPHA_LEQUAL;
      break;
   case GL_GREATER:
      current->hwAlpha |= SiS_ALPHA_GREATER;
      break;
   case GL_NOTEQUAL:
      current->hwAlpha |= SiS_ALPHA_NOTEQUAL;
      break;
   case GL_GEQUAL:
      current->hwAlpha |= SiS_ALPHA_GEQUAL;
      break;
   case GL_ALWAYS:
      current->hwAlpha |= SiS_ALPHA_ALWAYS;
      break;
   }

   prev->hwAlpha = current->hwAlpha;
   smesa->GlobalFlag |= GFLAG_ALPHASETTING;
}

static void
sisDDBlendFuncSeparate( GLcontext *ctx, 
			GLenum sfactorRGB, GLenum dfactorRGB,
			GLenum sfactorA,   GLenum dfactorA )
{
   sisContextPtr smesa = SIS_CONTEXT(ctx);

   __GLSiSHardware *prev = &smesa->prev;
   __GLSiSHardware *current = &smesa->current;

   current->hwDstSrcBlend = 0;

   switch (dfactorRGB)
   {
   case GL_ZERO:
      current->hwDstSrcBlend |= SiS_D_ZERO;
      break;
   case GL_ONE:
      current->hwDstSrcBlend |= SiS_D_ONE;
      break;
   case GL_SRC_COLOR:
      current->hwDstSrcBlend |= SiS_D_SRC_COLOR;
      break;
   case GL_ONE_MINUS_SRC_COLOR:
      current->hwDstSrcBlend |= SiS_D_ONE_MINUS_SRC_COLOR;
      break;
   case GL_SRC_ALPHA:
      current->hwDstSrcBlend |= SiS_D_SRC_ALPHA;
      break;
   case GL_ONE_MINUS_SRC_ALPHA:
      current->hwDstSrcBlend |= SiS_D_ONE_MINUS_SRC_ALPHA;
      break;
   case GL_DST_COLOR:
      current->hwDstSrcBlend |= SiS_D_DST_COLOR;
      break;
   case GL_ONE_MINUS_DST_COLOR:
      current->hwDstSrcBlend |= SiS_D_ONE_MINUS_DST_COLOR;
      break;
   case GL_DST_ALPHA:
      current->hwDstSrcBlend |= SiS_D_DST_ALPHA;
      break;
   case GL_ONE_MINUS_DST_ALPHA:
      current->hwDstSrcBlend |= SiS_D_ONE_MINUS_DST_ALPHA;
      break;
   default:
      fprintf(stderr, "Unknown dst blend function 0x%x\n", dfactorRGB);
      break;
   }

   switch (sfactorRGB)
   {
   case GL_ZERO:
      current->hwDstSrcBlend |= SiS_S_ZERO;
      break;
   case GL_ONE:
      current->hwDstSrcBlend |= SiS_S_ONE;
      break;
   case GL_SRC_COLOR:
      current->hwDstSrcBlend |= SiS_S_SRC_COLOR;
      break;
   case GL_ONE_MINUS_SRC_COLOR:
      current->hwDstSrcBlend |= SiS_S_ONE_MINUS_SRC_COLOR;
      break;
   case GL_SRC_ALPHA:
      current->hwDstSrcBlend |= SiS_S_SRC_ALPHA;
      break;
   case GL_ONE_MINUS_SRC_ALPHA:
      current->hwDstSrcBlend |= SiS_S_ONE_MINUS_SRC_ALPHA;
      break;
   case GL_DST_COLOR:
      current->hwDstSrcBlend |= SiS_S_DST_COLOR;
      break;
   case GL_ONE_MINUS_DST_COLOR:
      current->hwDstSrcBlend |= SiS_S_ONE_MINUS_DST_COLOR;
      break;
   case GL_DST_ALPHA:
      current->hwDstSrcBlend |= SiS_S_DST_ALPHA;
      break;
   case GL_ONE_MINUS_DST_ALPHA:
      current->hwDstSrcBlend |= SiS_S_ONE_MINUS_DST_ALPHA;
      break;
   case GL_SRC_ALPHA_SATURATE:
      current->hwDstSrcBlend |= SiS_S_SRC_ALPHA_SATURATE;
      break;
   default:
      fprintf(stderr, "Unknown src blend function 0x%x\n", sfactorRGB);
      break;
   }

   if (current->hwDstSrcBlend != prev->hwDstSrcBlend) {
      prev->hwDstSrcBlend = current->hwDstSrcBlend;
      smesa->GlobalFlag |= GFLAG_DSTBLEND;
   }
}

/* =============================================================
 * Depth testing
 */

static void
sisDDDepthFunc( GLcontext * ctx, GLenum func )
{
   sisContextPtr smesa = SIS_CONTEXT(ctx);
   __GLSiSHardware *prev = &smesa->prev;
   __GLSiSHardware *current = &smesa->current;

   current->hwZ &= ~MASK_ZTestMode;
   switch (func)
   {
   case GL_LESS:
      current->hwZ |= SiS_Z_COMP_S_LT_B;
      break;
   case GL_GEQUAL:
      current->hwZ |= SiS_Z_COMP_S_GE_B;
      break;
   case GL_LEQUAL:
      current->hwZ |= SiS_Z_COMP_S_LE_B;
      break;
   case GL_GREATER:
      current->hwZ |= SiS_Z_COMP_S_GT_B;
      break;
   case GL_NOTEQUAL:
      current->hwZ |= SiS_Z_COMP_S_NE_B;
      break;
   case GL_EQUAL:
      current->hwZ |= SiS_Z_COMP_S_EQ_B;
      break;
   case GL_ALWAYS:
      current->hwZ |= SiS_Z_COMP_ALWAYS;
      break;
   case GL_NEVER:
      current->hwZ |= SiS_Z_COMP_NEVER;
      break;
   }

   if (current->hwZ != prev->hwZ) {
      prev->hwZ = current->hwZ;
      smesa->GlobalFlag |= GFLAG_ZSETTING;
   }
}

void
sisDDDepthMask( GLcontext * ctx, GLboolean flag )
{
   sisContextPtr smesa = SIS_CONTEXT(ctx);
   __GLSiSHardware *prev = &smesa->prev;
   __GLSiSHardware *current = &smesa->current;

   if (!ctx->Depth.Test)
      flag = GL_FALSE;

   if (ctx->Visual.stencilBits) {
      if (flag || (ctx->Stencil.WriteMask[0] != 0)) {
         current->hwCapEnable |= MASK_ZWriteEnable;
         if (flag && ((ctx->Stencil.WriteMask[0] & 0xff) == 0xff)) {
	      current->hwCapEnable2 &= ~MASK_ZMaskWriteEnable;
         } else {
            current->hwCapEnable2 |= MASK_ZMaskWriteEnable;
            current->hwZMask = (ctx->Stencil.WriteMask[0] << 24) |
               ((flag) ? 0x00ffffff : 0);

            if (current->hwZMask ^ prev->hwZMask) {
               prev->hwZMask = current->hwZMask;
               smesa->GlobalFlag |= GFLAG_ZSETTING;
            }
         }
      } else {
         current->hwCapEnable &= ~MASK_ZWriteEnable;
      }
   } else {
      if (flag) {
         current->hwCapEnable |= MASK_ZWriteEnable;
         current->hwCapEnable2 &= ~MASK_ZMaskWriteEnable;
      } else {
         current->hwCapEnable &= ~MASK_ZWriteEnable;
      }
   }
}

/* =============================================================
 * Clipping
 */

void
sisUpdateClipping( GLcontext *ctx )
{
   sisContextPtr smesa = SIS_CONTEXT(ctx);

   __GLSiSHardware *prev = &smesa->prev;
   __GLSiSHardware *current = &smesa->current;

   GLint x1, y1, x2, y2;

   if (smesa->is6326) {
      /* XXX: 6326 has its own clipping for now. Should be fixed */
      sis6326UpdateClipping(ctx);
      return;
   }

   x1 = 0;
   y1 = 0;
   x2 = smesa->width - 1;
   y2 = smesa->height - 1;

   if (ctx->Scissor.Enabled) {
      if (ctx->Scissor.X > x1)
         x1 = ctx->Scissor.X;
      if (ctx->Scissor.Y > y1)
         y1 = ctx->Scissor.Y;
      if (ctx->Scissor.X + ctx->Scissor.Width - 1 < x2)
         x2 = ctx->Scissor.X + ctx->Scissor.Width - 1;
      if (ctx->Scissor.Y + ctx->Scissor.Height - 1 < y2)
         y2 = ctx->Scissor.Y + ctx->Scissor.Height - 1;
   }

   y1 = Y_FLIP(y1);
   y2 = Y_FLIP(y2);

   current->clipTopBottom = (y2 << 13) | y1;
   current->clipLeftRight = (x1 << 13) | x2;

   if ((current->clipTopBottom ^ prev->clipTopBottom) ||
       (current->clipLeftRight ^ prev->clipLeftRight))
   {
      prev->clipTopBottom = current->clipTopBottom;
      prev->clipLeftRight = current->clipLeftRight;
      smesa->GlobalFlag |= GFLAG_CLIPPING;
   }
}

static void
sisDDScissor( GLcontext *ctx, GLint x, GLint y, GLsizei w, GLsizei h )
{
   if (ctx->Scissor.Enabled)
      sisUpdateClipping( ctx );
}

/* =============================================================
 * Culling
 */

static void
sisUpdateCull( GLcontext *ctx )
{
   sisContextPtr smesa = SIS_CONTEXT(ctx);
   GLint cullflag, frontface;

   cullflag = ctx->Polygon.CullFaceMode;
   frontface = ctx->Polygon.FrontFace;

   smesa->AGPParseSet &= ~(MASK_PsCullDirection_CCW);
   smesa->dwPrimitiveSet &= ~(MASK_CullDirection);

   if((cullflag == GL_FRONT && frontface == GL_CCW) ||
      (cullflag == GL_BACK && frontface == GL_CW))
   {
      smesa->AGPParseSet |= MASK_PsCullDirection_CCW;
      smesa->dwPrimitiveSet |= OP_3D_CullDirection_CCW;
   }
}


static void
sisDDCullFace( GLcontext *ctx, GLenum mode )
{
   sisUpdateCull( ctx );
}

static void
sisDDFrontFace( GLcontext *ctx, GLenum mode )
{
   sisUpdateCull( ctx );
}

/* =============================================================
 * Masks
 */

static void sisDDColorMask( GLcontext *ctx,
			    GLboolean r, GLboolean g,
			    GLboolean b, GLboolean a )
{
   sisContextPtr smesa = SIS_CONTEXT(ctx);
   __GLSiSHardware *prev = &smesa->prev;
   __GLSiSHardware *current = &smesa->current;

   if (r && g && b && ((ctx->Visual.alphaBits == 0) || a)) {
      current->hwCapEnable2 &= ~(MASK_AlphaMaskWriteEnable |
				 MASK_ColorMaskWriteEnable);
   } else {
      current->hwCapEnable2 |= (MASK_AlphaMaskWriteEnable |
                             MASK_ColorMaskWriteEnable);

      current->hwDstMask = (r) ? smesa->redMask : 0 |
			   (g) ? smesa->greenMask : 0 |
			   (b) ? smesa->blueMask : 0 |
			   (a) ? smesa->alphaMask : 0;
   }
   
   if (current->hwDstMask != prev->hwDstMask) {
      prev->hwDstMask = current->hwDstMask;
      smesa->GlobalFlag |= GFLAG_DESTSETTING;
   }
}

/* =============================================================
 * Rendering attributes
 */

static void sisUpdateSpecular(GLcontext *ctx)
{
   sisContextPtr smesa = SIS_CONTEXT(ctx);
   __GLSiSHardware *current = &smesa->current;

   if (NEED_SECONDARY_COLOR(ctx))
      current->hwCapEnable |= MASK_SpecularEnable;
   else
      current->hwCapEnable &= ~MASK_SpecularEnable;
}

static void sisDDLightModelfv(GLcontext *ctx, GLenum pname,
			      const GLfloat *param)
{
   if (pname == GL_LIGHT_MODEL_COLOR_CONTROL) {
      sisUpdateSpecular(ctx);
   }
}

static void sisDDShadeModel( GLcontext *ctx, GLenum mode )
{
   sisContextPtr smesa = SIS_CONTEXT(ctx);

   /* Signal to sisRasterPrimitive to recalculate dwPrimitiveSet */
   smesa->hw_primitive = -1;
}

/* =============================================================
 * Window position
 */

/* =============================================================
 * Viewport
 */

static void sisCalcViewport( GLcontext *ctx )
{
   sisContextPtr smesa = SIS_CONTEXT(ctx);
   const GLfloat *v = ctx->Viewport._WindowMap.m;
   GLfloat *m = smesa->hw_viewport;

   /* See also sis_translate_vertex.
    */
   m[MAT_SX] =   v[MAT_SX];
   m[MAT_TX] =   v[MAT_TX] + SUBPIXEL_X;
   m[MAT_SY] = - v[MAT_SY];
   m[MAT_TY] = - v[MAT_TY] + smesa->driDrawable->h + SUBPIXEL_Y;
   m[MAT_SZ] =   v[MAT_SZ] * smesa->depth_scale;
   m[MAT_TZ] =   v[MAT_TZ] * smesa->depth_scale;
}

static void sisDDViewport( GLcontext *ctx,
			   GLint x, GLint y,
			   GLsizei width, GLsizei height )
{
   sisCalcViewport( ctx );
}

static void sisDDDepthRange( GLcontext *ctx,
			     GLclampd nearval, GLclampd farval )
{
   sisCalcViewport( ctx );
}

/* =============================================================
 * Miscellaneous
 */

static void
sisDDLogicOpCode( GLcontext *ctx, GLenum opcode )
{
   sisContextPtr smesa = SIS_CONTEXT(ctx);

   __GLSiSHardware *prev = &smesa->prev;
   __GLSiSHardware *current = &smesa->current;

   current->hwDstSet &= ~MASK_ROP2;
   switch (opcode)
   {
   case GL_CLEAR:
      current->hwDstSet |= LOP_CLEAR;
      break;
   case GL_SET:
      current->hwDstSet |= LOP_SET;
      break;
   case GL_COPY:
      current->hwDstSet |= LOP_COPY;
      break;
   case GL_COPY_INVERTED:
      current->hwDstSet |= LOP_COPY_INVERTED;
      break;
   case GL_NOOP:
      current->hwDstSet |= LOP_NOOP;
      break;
   case GL_INVERT:
      current->hwDstSet |= LOP_INVERT;
      break;
   case GL_AND:
      current->hwDstSet |= LOP_AND;
      break;
   case GL_NAND:
      current->hwDstSet |= LOP_NAND;
      break;
   case GL_OR:
      current->hwDstSet |= LOP_OR;
      break;
   case GL_NOR:
      current->hwDstSet |= LOP_NOR;
      break;
   case GL_XOR:
      current->hwDstSet |= LOP_XOR;
      break;
   case GL_EQUIV:
      current->hwDstSet |= LOP_EQUIV;
      break;
   case GL_AND_REVERSE:
      current->hwDstSet |= LOP_AND_REVERSE;
      break;
   case GL_AND_INVERTED:
      current->hwDstSet |= LOP_AND_INVERTED;
      break;
   case GL_OR_REVERSE:
      current->hwDstSet |= LOP_OR_REVERSE;
      break;
   case GL_OR_INVERTED:
      current->hwDstSet |= LOP_OR_INVERTED;
      break;
   }

   if (current->hwDstSet ^ prev->hwDstSet) {
      prev->hwDstSet = current->hwDstSet;
      smesa->GlobalFlag |= GFLAG_DESTSETTING;
   }
}

void sisDDDrawBuffer( GLcontext *ctx, GLenum mode )
{
   sisContextPtr smesa = SIS_CONTEXT(ctx);
   __GLSiSHardware *prev = &smesa->prev;
   __GLSiSHardware *current = &smesa->current;

   /*
    * _DrawDestMask is easier to cope with than <mode>.
    */
   current->hwDstSet &= ~MASK_DstBufferPitch;
   switch ( ctx->DrawBuffer->_ColorDrawBufferMask[0] ) {
   case BUFFER_BIT_FRONT_LEFT:
      FALLBACK( smesa, SIS_FALLBACK_DRAW_BUFFER, GL_FALSE );
      current->hwOffsetDest = smesa->front.offset >> 1;
      current->hwDstSet |= smesa->front.pitch >> 2;
      break;
   case BUFFER_BIT_BACK_LEFT:
      FALLBACK( smesa, SIS_FALLBACK_DRAW_BUFFER, GL_FALSE );
      current->hwOffsetDest = smesa->back.offset >> 1;
      current->hwDstSet |= smesa->back.pitch >> 2;
      break;
   default:
      /* GL_NONE or GL_FRONT_AND_BACK or stereo left&right, etc */
      FALLBACK( smesa, SIS_FALLBACK_DRAW_BUFFER, GL_TRUE );
      return;
   }

   if (current->hwDstSet != prev->hwDstSet) {
      prev->hwDstSet = current->hwDstSet;
      smesa->GlobalFlag |= GFLAG_DESTSETTING;
   }

   if (current->hwOffsetDest != prev->hwOffsetDest) {
      prev->hwOffsetDest = current->hwOffsetDest;
      smesa->GlobalFlag |= GFLAG_DESTSETTING;
   }
}

/* =============================================================
 * Polygon stipple
 */

/* =============================================================
 * Render mode
 */

/* =============================================================
 * State enable/disable
 */

static void
sisDDEnable( GLcontext * ctx, GLenum cap, GLboolean state )
{
   sisContextPtr smesa = SIS_CONTEXT(ctx);

   __GLSiSHardware *current = &smesa->current;

   switch (cap)
   {
   case GL_ALPHA_TEST:
      if (state)
         current->hwCapEnable |= MASK_AlphaTestEnable;
      else
         current->hwCapEnable &= ~MASK_AlphaTestEnable;
      break;
   case GL_BLEND:
      /* TODO: */
      if (state)
      /* if (state & !ctx->Color.ColorLogicOpEnabled) */
         current->hwCapEnable |= MASK_BlendEnable;
      else
         current->hwCapEnable &= ~MASK_BlendEnable;
      break;
   case GL_CULL_FACE:
      if (state)
         current->hwCapEnable |= MASK_CullEnable;
      else
         current->hwCapEnable &= ~MASK_CullEnable;
      break;
   case GL_DEPTH_TEST:
      if (state && smesa->depth.offset != 0)
         current->hwCapEnable |= MASK_ZTestEnable;
      else
         current->hwCapEnable &= ~MASK_ZTestEnable;
      sisDDDepthMask( ctx, ctx->Depth.Mask );
      break;
   case GL_DITHER:
      if (state)
         current->hwCapEnable |= MASK_DitherEnable;
      else
         current->hwCapEnable &= ~MASK_DitherEnable;
      break;
   case GL_FOG:
      if (state)
         current->hwCapEnable |= MASK_FogEnable;
      else
         current->hwCapEnable &= ~MASK_FogEnable;
      break;
   case GL_COLOR_LOGIC_OP:
      if (state)
         sisDDLogicOpCode( ctx, ctx->Color.LogicOp );
      else
         sisDDLogicOpCode( ctx, GL_COPY );
      break;
   case GL_SCISSOR_TEST:
      sisUpdateClipping( ctx );
      break;
   case GL_STENCIL_TEST:
      if (state) {
         if (smesa->zFormat != SiS_ZFORMAT_S8Z24)
            FALLBACK(smesa, SIS_FALLBACK_STENCIL, 1);
         else
            current->hwCapEnable |= (MASK_StencilTestEnable |
				     MASK_StencilWriteEnable);
      } else {
         FALLBACK(smesa, SIS_FALLBACK_STENCIL, 0);
         current->hwCapEnable &= ~(MASK_StencilTestEnable |
				   MASK_StencilWriteEnable);
      }
      break;
   case GL_LIGHTING:
   case GL_COLOR_SUM_EXT:
      sisUpdateSpecular(ctx);
      break;
   }
}


/* =============================================================
 * State initialization, management
 */

/* Called before beginning of rendering. */
void
sisUpdateHWState( GLcontext *ctx )
{
   sisContextPtr smesa = SIS_CONTEXT(ctx);
   __GLSiSHardware *prev = &smesa->prev;
   __GLSiSHardware *current = &smesa->current;

   /* enable setting 1 */
   if (current->hwCapEnable ^ prev->hwCapEnable) {
      prev->hwCapEnable = current->hwCapEnable;
      smesa->GlobalFlag |= GFLAG_ENABLESETTING;
   }

  /* enable setting 2 */
   if (current->hwCapEnable2 ^ prev->hwCapEnable2) {
      prev->hwCapEnable2 = current->hwCapEnable2;
      smesa->GlobalFlag |= GFLAG_ENABLESETTING2;
   }

   if (smesa->GlobalFlag & GFLAG_RENDER_STATES)
      sis_update_render_state( smesa );

   if (smesa->GlobalFlag & GFLAG_TEXTURE_STATES)
      sis_update_texture_state( smesa );
}

static void
sisDDInvalidateState( GLcontext *ctx, GLuint new_state )
{
   sisContextPtr smesa = SIS_CONTEXT(ctx);

   _swrast_InvalidateState( ctx, new_state );
   _swsetup_InvalidateState( ctx, new_state );
   _vbo_InvalidateState( ctx, new_state );
   _tnl_InvalidateState( ctx, new_state );
   smesa->NewGLState |= new_state;
}

/* Initialize the context's hardware state.
 */
void sisDDInitState( sisContextPtr smesa )
{
   __GLSiSHardware *current = &smesa->current;
   __GLSiSHardware *prev = &(smesa->prev);
   GLcontext *ctx = smesa->glCtx;

   /* add Texture Perspective Enable */
   prev->hwCapEnable = MASK_FogPerspectiveEnable | MASK_TextureCacheEnable |
      MASK_TexturePerspectiveEnable | MASK_DitherEnable;

   /*
   prev->hwCapEnable2 = 0x00aa0080;
   */
   /* if multi-texture enabled, disable Z pre-test */
   prev->hwCapEnable2 = MASK_TextureMipmapBiasEnable;

   /* Z test mode is LESS */
   prev->hwZ = SiS_Z_COMP_S_LT_B;

   /* Depth mask */
   prev->hwZMask = 0xffffffff;

   /* Alpha test mode is ALWAYS, alpha ref value is 0 */
   prev->hwAlpha = SiS_ALPHA_ALWAYS;

   /* ROP2 is COPYPEN */
   prev->hwDstSet = LOP_COPY;

   /* color mask */
   prev->hwDstMask = 0xffffffff;

   /* LinePattern is 0, Repeat Factor is 0 */
   prev->hwLinePattern = 0x00008000;

   /* Src blend is BLEND_ONE, Dst blend is D3DBLEND_ZERO */
   prev->hwDstSrcBlend = SiS_S_ONE | SiS_D_ZERO;

   /* Stenciling disabled, function ALWAYS, ref value zero, mask all ones */
   prev->hwStSetting = STENCIL_FORMAT_8 | SiS_STENCIL_ALWAYS | 0xff;
   /* Op is KEEP for all three operations */
   prev->hwStSetting2 = SiS_SFAIL_KEEP | SiS_SPASS_ZFAIL_KEEP | 
      SiS_SPASS_ZPASS_KEEP;

   /* Texture mapping mode is Tile */
#if 0
   prev->texture[0].hwTextureSet = 0x00030000;
#endif
   /* Magnified & minified texture filter is NEAREST */
#if 0
   prev->texture[0].hwTextureMip = 0;
#endif

   /* Texture Blending setting -- use fragment color/alpha*/
   prev->hwTexBlendColor0 = STAGE0_C_CF;
   prev->hwTexBlendColor1 = STAGE1_C_CF;
   prev->hwTexBlendAlpha0 = STAGE0_A_AF;
   prev->hwTexBlendAlpha1 = STAGE1_A_AF;
   
   switch (smesa->bytesPerPixel)
   {
   case 2:
      prev->hwDstSet |= DST_FORMAT_RGB_565;
      break;
   case 4:
      prev->hwDstSet |= DST_FORMAT_ARGB_8888;
      break;
   }

   switch (ctx->Visual.depthBits)
   {
   case 0:
      prev->hwCapEnable &= ~MASK_ZWriteEnable;
   case 16:
      smesa->zFormat = SiS_ZFORMAT_Z16;
      prev->hwCapEnable |= MASK_ZWriteEnable;
      smesa->depth_scale = 1.0 / (GLfloat)0xffff;
      break;
   case 32:
      smesa->zFormat = SiS_ZFORMAT_Z32;
      prev->hwCapEnable |= MASK_ZWriteEnable;
      smesa->depth_scale = 1.0 / (GLfloat)0xffffffff;
      break;
   case 24:
      assert (ctx->Visual.stencilBits);
      smesa->zFormat = SiS_ZFORMAT_S8Z24;
      prev->hwCapEnable |= MASK_StencilBufferEnable;
      prev->hwCapEnable |= MASK_ZWriteEnable;
      smesa->depth_scale = 1.0 / (GLfloat)0xffffff;
      break;
   }

   prev->hwZ |= smesa->zFormat;

   /* TODO: need to clear cache? */
   smesa->clearTexCache = GL_TRUE;

   smesa->clearColorPattern = 0;

   smesa->AGPParseSet = MASK_PsTexture1FromB | MASK_PsBumpTextureFromC;
   smesa->dwPrimitiveSet = OP_3D_Texture1FromB | OP_3D_TextureBumpFromC;

   sisUpdateZStencilPattern( smesa, 1.0, 0 );
   sisUpdateCull( ctx );

   memcpy( current, prev, sizeof (__GLSiSHardware) );

   /* Set initial fog settings. Start and end are the same case.  */
   sisDDFogfv( ctx, GL_FOG_DENSITY, &ctx->Fog.Density );
   sisDDFogfv( ctx, GL_FOG_END, &ctx->Fog.End );
   sisDDFogfv( ctx, GL_FOG_COORDINATE_SOURCE_EXT, NULL );
   sisDDFogfv( ctx, GL_FOG_MODE, NULL );
}

/* Initialize the driver's state functions.
 */
void sisDDInitStateFuncs( GLcontext *ctx )
{
   ctx->Driver.UpdateState	 = sisDDInvalidateState;

   ctx->Driver.Clear		 = sisDDClear;
   ctx->Driver.ClearColor	 = sisDDClearColor;
   ctx->Driver.ClearDepth	 = sisDDClearDepth;
   ctx->Driver.ClearStencil	 = sisDDClearStencil;

   ctx->Driver.AlphaFunc	 = sisDDAlphaFunc;
   ctx->Driver.BlendFuncSeparate = sisDDBlendFuncSeparate;
   ctx->Driver.ColorMask	 = sisDDColorMask;
   ctx->Driver.CullFace		 = sisDDCullFace;
   ctx->Driver.DepthMask	 = sisDDDepthMask;
   ctx->Driver.DepthFunc	 = sisDDDepthFunc;
   ctx->Driver.DepthRange	 = sisDDDepthRange;
   ctx->Driver.DrawBuffer	 = sisDDDrawBuffer;
   ctx->Driver.Enable		 = sisDDEnable;
   ctx->Driver.FrontFace	 = sisDDFrontFace;
   ctx->Driver.Fogfv		 = sisDDFogfv;
   ctx->Driver.Hint		 = NULL;
   ctx->Driver.Lightfv		 = NULL;
   ctx->Driver.LogicOpcode	 = sisDDLogicOpCode;
   ctx->Driver.PolygonMode	 = NULL;
   ctx->Driver.PolygonStipple	 = NULL;
   ctx->Driver.ReadBuffer	 = NULL;
   ctx->Driver.RenderMode	 = NULL;
   ctx->Driver.Scissor		 = sisDDScissor;
   ctx->Driver.ShadeModel	 = sisDDShadeModel;
   ctx->Driver.LightModelfv	 = sisDDLightModelfv;
   ctx->Driver.Viewport		 = sisDDViewport;

   /* XXX this should go away */
   ctx->Driver.ResizeBuffers	 = sisReAllocateBuffers;
}
