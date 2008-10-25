/**************************************************************************
 * 
 * Copyright 2003 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 **************************************************************************/


#include "glheader.h"
#include "context.h"
#include "macros.h"
#include "enums.h"
#include "dd.h"

#include "intel_screen.h"
#include "intel_context.h"
#include "swrast/swrast.h"

int intel_translate_compare_func( GLenum func )
{
   switch(func) {
   case GL_NEVER: 
      return COMPAREFUNC_NEVER; 
   case GL_LESS: 
      return COMPAREFUNC_LESS; 
   case GL_LEQUAL: 
      return COMPAREFUNC_LEQUAL; 
   case GL_GREATER: 
      return COMPAREFUNC_GREATER; 
   case GL_GEQUAL: 
      return COMPAREFUNC_GEQUAL; 
   case GL_NOTEQUAL: 
      return COMPAREFUNC_NOTEQUAL; 
   case GL_EQUAL: 
      return COMPAREFUNC_EQUAL; 
   case GL_ALWAYS: 
      return COMPAREFUNC_ALWAYS; 
   }

   fprintf(stderr, "Unknown value in %s: %x\n", __FUNCTION__, func);
   return COMPAREFUNC_ALWAYS; 
}

int intel_translate_stencil_op( GLenum op )
{
   switch(op) {
   case GL_KEEP: 
      return STENCILOP_KEEP; 
   case GL_ZERO: 
      return STENCILOP_ZERO; 
   case GL_REPLACE: 
      return STENCILOP_REPLACE; 
   case GL_INCR: 
      return STENCILOP_INCRSAT;
   case GL_DECR: 
      return STENCILOP_DECRSAT;
   case GL_INCR_WRAP:
      return STENCILOP_INCR; 
   case GL_DECR_WRAP:
      return STENCILOP_DECR; 
   case GL_INVERT: 
      return STENCILOP_INVERT; 
   default: 
      return STENCILOP_ZERO;
   }
}

int intel_translate_blend_factor( GLenum factor )
{
   switch(factor) {
   case GL_ZERO: 
      return BLENDFACT_ZERO; 
   case GL_SRC_ALPHA: 
      return BLENDFACT_SRC_ALPHA; 
   case GL_ONE: 
      return BLENDFACT_ONE; 
   case GL_SRC_COLOR: 
      return BLENDFACT_SRC_COLR; 
   case GL_ONE_MINUS_SRC_COLOR: 
      return BLENDFACT_INV_SRC_COLR; 
   case GL_DST_COLOR: 
      return BLENDFACT_DST_COLR; 
   case GL_ONE_MINUS_DST_COLOR: 
      return BLENDFACT_INV_DST_COLR; 
   case GL_ONE_MINUS_SRC_ALPHA:
      return BLENDFACT_INV_SRC_ALPHA; 
   case GL_DST_ALPHA: 
      return BLENDFACT_DST_ALPHA; 
   case GL_ONE_MINUS_DST_ALPHA:
      return BLENDFACT_INV_DST_ALPHA; 
   case GL_SRC_ALPHA_SATURATE: 
      return BLENDFACT_SRC_ALPHA_SATURATE;
   case GL_CONSTANT_COLOR:
      return BLENDFACT_CONST_COLOR; 
   case GL_ONE_MINUS_CONSTANT_COLOR:
      return BLENDFACT_INV_CONST_COLOR;
   case GL_CONSTANT_ALPHA:
      return BLENDFACT_CONST_ALPHA; 
   case GL_ONE_MINUS_CONSTANT_ALPHA:
      return BLENDFACT_INV_CONST_ALPHA;
   }
   
   fprintf(stderr, "Unknown value in %s: %x\n", __FUNCTION__, factor);
   return BLENDFACT_ZERO;
}

int intel_translate_logic_op( GLenum opcode )
{
   switch(opcode) {
   case GL_CLEAR: 
      return LOGICOP_CLEAR; 
   case GL_AND: 
      return LOGICOP_AND; 
   case GL_AND_REVERSE: 
      return LOGICOP_AND_RVRSE; 
   case GL_COPY: 
      return LOGICOP_COPY; 
   case GL_COPY_INVERTED: 
      return LOGICOP_COPY_INV; 
   case GL_AND_INVERTED: 
      return LOGICOP_AND_INV; 
   case GL_NOOP: 
      return LOGICOP_NOOP; 
   case GL_XOR: 
      return LOGICOP_XOR; 
   case GL_OR: 
      return LOGICOP_OR; 
   case GL_OR_INVERTED: 
      return LOGICOP_OR_INV; 
   case GL_NOR: 
      return LOGICOP_NOR; 
   case GL_EQUIV: 
      return LOGICOP_EQUIV; 
   case GL_INVERT: 
      return LOGICOP_INV; 
   case GL_OR_REVERSE: 
      return LOGICOP_OR_RVRSE; 
   case GL_NAND: 
      return LOGICOP_NAND; 
   case GL_SET: 
      return LOGICOP_SET; 
   default:
      return LOGICOP_SET;
   }
}

static void intelDrawBuffer(GLcontext *ctx, GLenum mode )
{
   intelContextPtr intel = INTEL_CONTEXT(ctx);
   int front = 0;
 
   if (!ctx->DrawBuffer)
      return;

   switch ( ctx->DrawBuffer->_ColorDrawBufferMask[0] ) {
   case BUFFER_BIT_FRONT_LEFT:
      front = 1;
      FALLBACK( intel, INTEL_FALLBACK_DRAW_BUFFER, GL_FALSE );
      break;
   case BUFFER_BIT_BACK_LEFT:
      front = 0;
      FALLBACK( intel, INTEL_FALLBACK_DRAW_BUFFER, GL_FALSE );
      break;
   default:
      FALLBACK( intel, INTEL_FALLBACK_DRAW_BUFFER, GL_TRUE );
      return;
   }

   if ( intel->sarea->pf_current_page == 1 ) 
      front ^= 1;
   
   intelSetFrontClipRects( intel );

   if (front) {
      intel->drawRegion = &intel->intelScreen->front;
      intel->readRegion = &intel->intelScreen->front;
   } else {
      intel->drawRegion = &intel->intelScreen->back;
      intel->readRegion = &intel->intelScreen->back;
   }

   intel->vtbl.set_color_region( intel, intel->drawRegion );
}

static void intelReadBuffer( GLcontext *ctx, GLenum mode )
{
   /* nothing, until we implement h/w glRead/CopyPixels or CopyTexImage */
}


static void intelClearColor(GLcontext *ctx, const GLfloat color[4])
{
   intelContextPtr intel = INTEL_CONTEXT(ctx);
   intelScreenPrivate *screen = intel->intelScreen;

   CLAMPED_FLOAT_TO_UBYTE(intel->clear_red, color[0]);
   CLAMPED_FLOAT_TO_UBYTE(intel->clear_green, color[1]);
   CLAMPED_FLOAT_TO_UBYTE(intel->clear_blue, color[2]);
   CLAMPED_FLOAT_TO_UBYTE(intel->clear_alpha, color[3]);

   intel->ClearColor = INTEL_PACKCOLOR(screen->fbFormat,
				       intel->clear_red, 
				       intel->clear_green, 
				       intel->clear_blue, 
				       intel->clear_alpha);
}


static void intelCalcViewport( GLcontext *ctx )
{
   intelContextPtr intel = INTEL_CONTEXT(ctx);
   const GLfloat *v = ctx->Viewport._WindowMap.m;
   GLfloat *m = intel->ViewportMatrix.m;
   GLint h = 0;

   if (intel->driDrawable) 
      h = intel->driDrawable->h + SUBPIXEL_Y;

   /* See also intel_translate_vertex.  SUBPIXEL adjustments can be done
    * via state vars, too.
    */
   m[MAT_SX] =   v[MAT_SX];
   m[MAT_TX] =   v[MAT_TX] + SUBPIXEL_X;
   m[MAT_SY] = - v[MAT_SY];
   m[MAT_TY] = - v[MAT_TY] + h;
   m[MAT_SZ] =   v[MAT_SZ] * intel->depth_scale;
   m[MAT_TZ] =   v[MAT_TZ] * intel->depth_scale;
}

static void intelViewport( GLcontext *ctx,
			  GLint x, GLint y,
			  GLsizei width, GLsizei height )
{
   intelCalcViewport( ctx );
}

static void intelDepthRange( GLcontext *ctx,
			    GLclampd nearval, GLclampd farval )
{
   intelCalcViewport( ctx );
}

/* Fallback to swrast for select and feedback.
 */
static void intelRenderMode( GLcontext *ctx, GLenum mode )
{
   intelContextPtr intel = INTEL_CONTEXT(ctx);
   FALLBACK( intel, INTEL_FALLBACK_RENDERMODE, (mode != GL_RENDER) );
}


void intelInitStateFuncs( struct dd_function_table *functions )
{
   functions->DrawBuffer = intelDrawBuffer;
   functions->ReadBuffer = intelReadBuffer;
   functions->RenderMode = intelRenderMode;
   functions->Viewport = intelViewport;
   functions->DepthRange = intelDepthRange;
   functions->ClearColor = intelClearColor;
}

