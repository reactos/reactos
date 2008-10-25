/*
 * Copyright 2005 Eric Anholt
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Authors:
 *    Eric Anholt <anholt@FreeBSD.org>
 *
 */

#include "sis_context.h"
#include "sis_state.h"
#include "sis_tris.h"
#include "sis_lock.h"
#include "sis_tex.h"
#include "sis_reg.h"

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
sis6326DDAlphaFunc( GLcontext *ctx, GLenum func, GLfloat ref )
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
      current->hwAlpha |= S_ASET_PASS_NEVER;
      break;
   case GL_LESS:
      current->hwAlpha |= S_ASET_PASS_LESS;
      break;
   case GL_EQUAL:
      current->hwAlpha |= S_ASET_PASS_EQUAL;
      break;
   case GL_LEQUAL:
      current->hwAlpha |= S_ASET_PASS_LEQUAL;
      break;
   case GL_GREATER:
      current->hwAlpha |= S_ASET_PASS_GREATER;
      break;
   case GL_NOTEQUAL:
      current->hwAlpha |= S_ASET_PASS_NOTEQUAL;
      break;
   case GL_GEQUAL:
      current->hwAlpha |= S_ASET_PASS_GEQUAL;
      break;
   case GL_ALWAYS:
      current->hwAlpha |= S_ASET_PASS_ALWAYS;
      break;
   }

   prev->hwAlpha = current->hwAlpha;
   smesa->GlobalFlag |= GFLAG_ALPHASETTING;
}

static void
sis6326DDBlendFuncSeparate( GLcontext *ctx, 
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
      current->hwDstSrcBlend |= S_DBLEND_ZERO;
      break;
   case GL_ONE:
      current->hwDstSrcBlend |= S_DBLEND_ONE;
      break;
   case GL_SRC_COLOR:
      current->hwDstSrcBlend |= S_DBLEND_SRC_COLOR;
      break;
   case GL_ONE_MINUS_SRC_COLOR:
      current->hwDstSrcBlend |= S_DBLEND_INV_SRC_COLOR;
      break;
   case GL_SRC_ALPHA:
      current->hwDstSrcBlend |= S_DBLEND_SRC_ALPHA;
      break;
   case GL_ONE_MINUS_SRC_ALPHA:
      current->hwDstSrcBlend |= S_DBLEND_INV_SRC_ALPHA;
      break;
   case GL_DST_ALPHA:
      current->hwDstSrcBlend |= S_DBLEND_DST_ALPHA;
      break;
   case GL_ONE_MINUS_DST_ALPHA:
      current->hwDstSrcBlend |= S_DBLEND_INV_DST_ALPHA;
      break;
   }

   switch (sfactorRGB)
   {
   case GL_ZERO:
      current->hwDstSrcBlend |= S_SBLEND_ZERO;
      break;
   case GL_ONE:
      current->hwDstSrcBlend |= S_SBLEND_ONE;
      break;
   case GL_SRC_ALPHA:
      current->hwDstSrcBlend |= S_SBLEND_SRC_ALPHA;
      break;
   case GL_ONE_MINUS_SRC_ALPHA:
      current->hwDstSrcBlend |= S_SBLEND_INV_SRC_ALPHA;
      break;
   case GL_DST_ALPHA:
      current->hwDstSrcBlend |= S_SBLEND_DST_ALPHA;
      break;
   case GL_ONE_MINUS_DST_ALPHA:
      current->hwDstSrcBlend |= S_SBLEND_INV_DST_ALPHA;
      break;
   case GL_DST_COLOR:
      current->hwDstSrcBlend |= S_SBLEND_DST_COLOR;
      break;
   case GL_ONE_MINUS_DST_COLOR:
      current->hwDstSrcBlend |= S_SBLEND_INV_DST_COLOR;
      break;
   case GL_SRC_ALPHA_SATURATE:
      current->hwDstSrcBlend |= S_SBLEND_SRC_ALPHA_SAT;
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
sis6326DDDepthFunc( GLcontext *ctx, GLenum func )
{
   sisContextPtr smesa = SIS_CONTEXT(ctx);
   __GLSiSHardware *prev = &smesa->prev;
   __GLSiSHardware *current = &smesa->current;

   current->hwZ &= ~MASK_6326_ZTestMode;
   switch (func)
   {
   case GL_LESS:
      current->hwZ |= S_ZSET_PASS_LESS;
      break;
   case GL_GEQUAL:
      current->hwZ |= S_ZSET_PASS_GEQUAL;
      break;
   case GL_LEQUAL:
      current->hwZ |= S_ZSET_PASS_LEQUAL;
      break;
   case GL_GREATER:
      current->hwZ |= S_ZSET_PASS_GREATER;
      break;
   case GL_NOTEQUAL:
      current->hwZ |= S_ZSET_PASS_NOTEQUAL;
      break;
   case GL_EQUAL:
      current->hwZ |= S_ZSET_PASS_EQUAL;
      break;
   case GL_ALWAYS:
      current->hwZ |= S_ZSET_PASS_ALWAYS;
      break;
   case GL_NEVER:
      current->hwZ |= S_ZSET_PASS_NEVER;
      break;
   }

   if (current->hwZ != prev->hwZ) {
      prev->hwZ = current->hwZ;
      smesa->GlobalFlag |= GFLAG_ZSETTING;
   }
}

static void
sis6326DDDepthMask( GLcontext *ctx, GLboolean flag )
{
   sisContextPtr smesa = SIS_CONTEXT(ctx);
   __GLSiSHardware *current = &smesa->current;

   if (ctx->Depth.Test)
      current->hwCapEnable |= S_ENABLE_ZWrite;
   else
      current->hwCapEnable &= ~S_ENABLE_ZWrite;
}

/* =============================================================
 * Fog
 */

static void
sis6326DDFogfv( GLcontext *ctx, GLenum pname, const GLfloat *params )
{
   sisContextPtr smesa = SIS_CONTEXT(ctx);
   __GLSiSHardware *current = &smesa->current;
   __GLSiSHardware *prev = &smesa->prev;

   GLint fogColor;

   switch(pname)
   {
   case GL_FOG_COLOR:
      fogColor  = FLOAT_TO_UBYTE( ctx->Fog.Color[0] ) << 16;
      fogColor |= FLOAT_TO_UBYTE( ctx->Fog.Color[1] ) << 8;
      fogColor |= FLOAT_TO_UBYTE( ctx->Fog.Color[2] );
      current->hwFog = 0x01000000 | fogColor;
      if (current->hwFog != prev->hwFog) {
	 prev->hwFog = current->hwFog;
	 smesa->GlobalFlag |= GFLAG_FOGSETTING;
      }
      break;
   }
}

/* =============================================================
 * Clipping
 */

void
sis6326UpdateClipping(GLcontext *ctx)
{
   sisContextPtr smesa = SIS_CONTEXT(ctx);

   __GLSiSHardware *prev = &smesa->prev;
   __GLSiSHardware *current = &smesa->current;

   GLint x1, y1, x2, y2;

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

   /*current->clipTopBottom = (y2 << 13) | y1;
   current->clipLeftRight = (x1 << 13) | x2;*/ /* XXX */
   current->clipTopBottom = (0 << 13) | smesa->height;
   current->clipLeftRight = (0 << 13) | smesa->width;

   if ((current->clipTopBottom != prev->clipTopBottom) ||
       (current->clipLeftRight != prev->clipLeftRight)) {
      prev->clipTopBottom = current->clipTopBottom;
      prev->clipLeftRight = current->clipLeftRight;
      smesa->GlobalFlag |= GFLAG_CLIPPING;
   }
}

static void
sis6326DDScissor( GLcontext *ctx, GLint x, GLint y, GLsizei w, GLsizei h )
{
   if (ctx->Scissor.Enabled)
      sis6326UpdateClipping( ctx );
}

/* =============================================================
 * Culling
 */

static void
sis6326UpdateCull( GLcontext *ctx )
{
   /* XXX culling */
}


static void
sis6326DDCullFace( GLcontext *ctx, GLenum mode )
{
   sis6326UpdateCull( ctx );
}

static void
sis6326DDFrontFace( GLcontext *ctx, GLenum mode )
{
   sis6326UpdateCull( ctx );
}

/* =============================================================
 * Masks
 */

static void sis6326DDColorMask( GLcontext *ctx,
				GLboolean r, GLboolean g,
				GLboolean b, GLboolean a )
{
   sisContextPtr smesa = SIS_CONTEXT(ctx);
	
   if (r && g && b && ((ctx->Visual.alphaBits == 0) || a)) {
      FALLBACK(smesa, SIS_FALLBACK_WRITEMASK, 0);
   } else {
      FALLBACK(smesa, SIS_FALLBACK_WRITEMASK, 1);
   }
}

/* =============================================================
 * Rendering attributes
 */

static void sis6326UpdateSpecular(GLcontext *ctx)
{
   sisContextPtr smesa = SIS_CONTEXT(ctx);
   __GLSiSHardware *current = &smesa->current;

   if (NEED_SECONDARY_COLOR(ctx))
      current->hwCapEnable |= S_ENABLE_Specular;
   else
      current->hwCapEnable &= ~S_ENABLE_Specular;
}

static void sis6326DDLightModelfv(GLcontext *ctx, GLenum pname,
			      const GLfloat *param)
{
   if (pname == GL_LIGHT_MODEL_COLOR_CONTROL) {
      sis6326UpdateSpecular(ctx);
   }
}
static void sis6326DDShadeModel( GLcontext *ctx, GLenum mode )
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

static void sis6326CalcViewport( GLcontext *ctx )
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

static void sis6326DDViewport( GLcontext *ctx,
			   GLint x, GLint y,
			   GLsizei width, GLsizei height )
{
   sis6326CalcViewport( ctx );
}

static void sis6326DDDepthRange( GLcontext *ctx,
			     GLclampd nearval, GLclampd farval )
{
   sis6326CalcViewport( ctx );
}

/* =============================================================
 * Miscellaneous
 */

static void
sis6326DDLogicOpCode( GLcontext *ctx, GLenum opcode )
{
   sisContextPtr smesa = SIS_CONTEXT(ctx);

   __GLSiSHardware *prev = &smesa->prev;
   __GLSiSHardware *current = &smesa->current;

   if (!ctx->Color.ColorLogicOpEnabled)
      return;

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

   if (current->hwDstSet != prev->hwDstSet) {
      prev->hwDstSet = current->hwDstSet;
      smesa->GlobalFlag |= GFLAG_DESTSETTING;
   }
}

void sis6326DDDrawBuffer( GLcontext *ctx, GLenum mode )
{
   sisContextPtr smesa = SIS_CONTEXT(ctx);

   __GLSiSHardware *prev = &smesa->prev;
   __GLSiSHardware *current = &smesa->current;

   if(getenv("SIS_DRAW_FRONT"))
      ctx->DrawBuffer->_ColorDrawBufferMask[0] = GL_FRONT_LEFT;

   /*
    * _DrawDestMask is easier to cope with than <mode>.
    */
   current->hwDstSet &= ~MASK_DstBufferPitch;
   switch ( ctx->DrawBuffer->_ColorDrawBufferMask[0] ) {
   case BUFFER_BIT_FRONT_LEFT:
      current->hwOffsetDest = smesa->front.offset;
      current->hwDstSet |= smesa->front.pitch;
      FALLBACK( smesa, SIS_FALLBACK_DRAW_BUFFER, GL_FALSE );
      break;
   case BUFFER_BIT_BACK_LEFT:
      current->hwOffsetDest = smesa->back.offset;
      current->hwDstSet |= smesa->back.pitch;
      FALLBACK( smesa, SIS_FALLBACK_DRAW_BUFFER, GL_FALSE );
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
sis6326DDEnable( GLcontext *ctx, GLenum cap, GLboolean state )
{
   sisContextPtr smesa = SIS_CONTEXT(ctx);

   __GLSiSHardware *current = &smesa->current;

   switch (cap)
   {
   case GL_ALPHA_TEST:
      if (state)
         current->hwCapEnable |= S_ENABLE_AlphaTest;
      else
         current->hwCapEnable &= ~S_ENABLE_AlphaTest;
      break;
   case GL_BLEND:
      /* TODO: */
      if (state)
      /* if (state & !ctx->Color.ColorLogicOpEnabled) */
         current->hwCapEnable |= S_ENABLE_Blend;
      else
         current->hwCapEnable &= ~S_ENABLE_Blend;
      break;
   case GL_CULL_FACE:
      /* XXX culling */
      break;
   case GL_DEPTH_TEST:
      if (state && smesa->depth.offset != 0)
         current->hwCapEnable |= S_ENABLE_ZTest;
      else
         current->hwCapEnable &= ~S_ENABLE_ZTest;
      sis6326DDDepthMask( ctx, ctx->Depth.Mask );
      break;
   case GL_DITHER:
      if (state)
         current->hwCapEnable |= S_ENABLE_Dither;
      else
         current->hwCapEnable &= ~S_ENABLE_Dither;
      break;
   case GL_FOG:
      if (state)
         current->hwCapEnable |= S_ENABLE_Fog;
      else
         current->hwCapEnable &= ~S_ENABLE_Fog;
      break;
   case GL_COLOR_LOGIC_OP:
      if (state)
         sis6326DDLogicOpCode( ctx, ctx->Color.LogicOp );
      else
         sis6326DDLogicOpCode( ctx, GL_COPY );
      break;
   case GL_SCISSOR_TEST:
      sis6326UpdateClipping( ctx );
      break;
   case GL_STENCIL_TEST:
      if (state) {
         FALLBACK(smesa, SIS_FALLBACK_STENCIL, 1);
      } else {
         FALLBACK(smesa, SIS_FALLBACK_STENCIL, 0);
      }
      break;
   case GL_LIGHTING:
   case GL_COLOR_SUM_EXT:
      sis6326UpdateSpecular(ctx);
      break;
    }
}

/* =============================================================
 * State initialization, management
 */

/* Called before beginning of rendering. */
void
sis6326UpdateHWState( GLcontext *ctx )
{
   sisContextPtr smesa = SIS_CONTEXT(ctx);
   __GLSiSHardware *prev = &smesa->prev;
   __GLSiSHardware *current = &smesa->current;

   if (smesa->NewGLState & _NEW_TEXTURE)
      sisUpdateTextureState( ctx );

   if (current->hwCapEnable ^ prev->hwCapEnable) {
      prev->hwCapEnable = current->hwCapEnable;
      smesa->GlobalFlag |= GFLAG_ENABLESETTING;
   }

   if (smesa->GlobalFlag & GFLAG_RENDER_STATES)
      sis_update_render_state( smesa );

   if (smesa->GlobalFlag & GFLAG_TEXTURE_STATES)
      sis_update_texture_state( smesa );
}

static void
sis6326DDInvalidateState( GLcontext *ctx, GLuint new_state )
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
void sis6326DDInitState( sisContextPtr smesa )
{
   __GLSiSHardware *prev = &smesa->prev;
   __GLSiSHardware *current = &smesa->current;
   GLcontext *ctx = smesa->glCtx;

   /* add Texture Perspective Enable */
   current->hwCapEnable = S_ENABLE_TextureCache |
       S_ENABLE_TexturePerspective | S_ENABLE_Dither;

   /* Z test mode is LESS */
   current->hwZ = S_ZSET_PASS_LESS | S_ZSET_FORMAT_16;
   if (ctx->Visual.depthBits > 0)
      current->hwCapEnable |= S_ENABLE_ZWrite;

   /* Alpha test mode is ALWAYS, alpha ref value is 0 */
   current->hwAlpha = S_ASET_PASS_ALWAYS;

   /* ROP2 is COPYPEN */
   current->hwDstSet = LOP_COPY;

   /* LinePattern is 0, Repeat Factor is 0 */
   current->hwLinePattern = 0x00008000;

   /* Src blend is BLEND_ONE, Dst blend is D3DBLEND_ZERO */
   current->hwDstSrcBlend = S_SBLEND_ONE | S_DBLEND_ZERO;
   
   switch (smesa->bytesPerPixel)
   {
   case 2:
      current->hwDstSet |= DST_FORMAT_RGB_565;
      break;
   case 4:
      current->hwDstSet |= DST_FORMAT_ARGB_8888;
      break;
   }

   smesa->depth_scale = 1.0 / (GLfloat)0xffff;

   smesa->clearTexCache = GL_TRUE;

   smesa->clearColorPattern = 0;

   sis6326UpdateZPattern(smesa, 1.0);
   sis6326UpdateCull(ctx);

   /* Set initial fog settings. Start and end are the same case.  */
   sis6326DDFogfv( ctx, GL_FOG_DENSITY, &ctx->Fog.Density );
   sis6326DDFogfv( ctx, GL_FOG_END, &ctx->Fog.End );
   sis6326DDFogfv( ctx, GL_FOG_MODE, NULL );

   memcpy(prev, current, sizeof(__GLSiSHardware));
}

/* Initialize the driver's state functions.
 */
void sis6326DDInitStateFuncs( GLcontext *ctx )
{
   ctx->Driver.UpdateState		= sis6326DDInvalidateState;

   ctx->Driver.Clear			= sis6326DDClear;
   ctx->Driver.ClearColor		= sis6326DDClearColor;
   ctx->Driver.ClearDepth		= sis6326DDClearDepth;

   ctx->Driver.AlphaFunc		= sis6326DDAlphaFunc;
   ctx->Driver.BlendFuncSeparate	= sis6326DDBlendFuncSeparate;
   ctx->Driver.ColorMask		= sis6326DDColorMask;
   ctx->Driver.CullFace			= sis6326DDCullFace;
   ctx->Driver.DepthMask		= sis6326DDDepthMask;
   ctx->Driver.DepthFunc		= sis6326DDDepthFunc;
   ctx->Driver.DepthRange		= sis6326DDDepthRange;
   ctx->Driver.DrawBuffer		= sis6326DDDrawBuffer;
   ctx->Driver.Enable			= sis6326DDEnable;
   ctx->Driver.FrontFace	 	= sis6326DDFrontFace;
   ctx->Driver.Fogfv			= sis6326DDFogfv;
   ctx->Driver.LogicOpcode	 	= sis6326DDLogicOpCode;
   ctx->Driver.Scissor			= sis6326DDScissor;
   ctx->Driver.ShadeModel		= sis6326DDShadeModel;
   ctx->Driver.LightModelfv		= sis6326DDLightModelfv;
   ctx->Driver.Viewport			= sis6326DDViewport;
}
