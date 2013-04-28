/**************************************************************************
 * 
 * Copyright 2007 Tungsten Graphics, Inc., Cedar Park, Texas.
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

 /*
  * Authors:
  *   Keith Whitwell <keith@tungstengraphics.com>
  *   Brian Paul
  *   Zack  Rusin
  */
 

#include <assert.h>

#include "st_context.h"
#include "st_atom.h"
#include "pipe/p_context.h"
#include "pipe/p_defines.h"
#include "cso_cache/cso_context.h"


/**
 * Convert an OpenGL compare mode to a pipe tokens.
 */
GLuint
st_compare_func_to_pipe(GLenum func)
{
   /* Same values, just biased */
   assert(PIPE_FUNC_NEVER == GL_NEVER - GL_NEVER);
   assert(PIPE_FUNC_LESS == GL_LESS - GL_NEVER);
   assert(PIPE_FUNC_EQUAL == GL_EQUAL - GL_NEVER);
   assert(PIPE_FUNC_LEQUAL == GL_LEQUAL - GL_NEVER);
   assert(PIPE_FUNC_GREATER == GL_GREATER - GL_NEVER);
   assert(PIPE_FUNC_NOTEQUAL == GL_NOTEQUAL - GL_NEVER);
   assert(PIPE_FUNC_GEQUAL == GL_GEQUAL - GL_NEVER);
   assert(PIPE_FUNC_ALWAYS == GL_ALWAYS - GL_NEVER);
   assert(func >= GL_NEVER);
   assert(func <= GL_ALWAYS);
   return func - GL_NEVER;
}


/**
 * Convert GLenum stencil op tokens to pipe tokens.
 */
static GLuint
gl_stencil_op_to_pipe(GLenum func)
{
   switch (func) {
   case GL_KEEP:
      return PIPE_STENCIL_OP_KEEP;
   case GL_ZERO:
      return PIPE_STENCIL_OP_ZERO;
   case GL_REPLACE:
      return PIPE_STENCIL_OP_REPLACE;
   case GL_INCR:
      return PIPE_STENCIL_OP_INCR;
   case GL_DECR:
      return PIPE_STENCIL_OP_DECR;
   case GL_INCR_WRAP:
      return PIPE_STENCIL_OP_INCR_WRAP;
   case GL_DECR_WRAP:
      return PIPE_STENCIL_OP_DECR_WRAP;
   case GL_INVERT:
      return PIPE_STENCIL_OP_INVERT;
   default:
      assert("invalid GL token in gl_stencil_op_to_pipe()" == NULL);
      return 0;
   }
}

static void
update_depth_stencil_alpha(struct st_context *st)
{
   struct pipe_depth_stencil_alpha_state *dsa = &st->state.depth_stencil;
   struct pipe_stencil_ref sr;
   struct gl_context *ctx = st->ctx;

   memset(dsa, 0, sizeof(*dsa));
   memset(&sr, 0, sizeof(sr));

   if (ctx->Depth.Test && ctx->DrawBuffer->Visual.depthBits > 0) {
      dsa->depth.enabled = 1;
      dsa->depth.writemask = ctx->Depth.Mask;
      dsa->depth.func = st_compare_func_to_pipe(ctx->Depth.Func);
   }

   if (ctx->Stencil.Enabled && ctx->DrawBuffer->Visual.stencilBits > 0) {
      dsa->stencil[0].enabled = 1;
      dsa->stencil[0].func = st_compare_func_to_pipe(ctx->Stencil.Function[0]);
      dsa->stencil[0].fail_op = gl_stencil_op_to_pipe(ctx->Stencil.FailFunc[0]);
      dsa->stencil[0].zfail_op = gl_stencil_op_to_pipe(ctx->Stencil.ZFailFunc[0]);
      dsa->stencil[0].zpass_op = gl_stencil_op_to_pipe(ctx->Stencil.ZPassFunc[0]);
      dsa->stencil[0].valuemask = ctx->Stencil.ValueMask[0] & 0xff;
      dsa->stencil[0].writemask = ctx->Stencil.WriteMask[0] & 0xff;
      sr.ref_value[0] = ctx->Stencil.Ref[0] & 0xff;

      if (ctx->Stencil._TestTwoSide) {
         const GLuint back = ctx->Stencil._BackFace;
         dsa->stencil[1].enabled = 1;
         dsa->stencil[1].func = st_compare_func_to_pipe(ctx->Stencil.Function[back]);
         dsa->stencil[1].fail_op = gl_stencil_op_to_pipe(ctx->Stencil.FailFunc[back]);
         dsa->stencil[1].zfail_op = gl_stencil_op_to_pipe(ctx->Stencil.ZFailFunc[back]);
         dsa->stencil[1].zpass_op = gl_stencil_op_to_pipe(ctx->Stencil.ZPassFunc[back]);
         dsa->stencil[1].valuemask = ctx->Stencil.ValueMask[back] & 0xff;
         dsa->stencil[1].writemask = ctx->Stencil.WriteMask[back] & 0xff;
         sr.ref_value[1] = ctx->Stencil.Ref[back] & 0xff;
      }
      else {
         /* This should be unnecessary. Drivers must not expect this to
          * contain valid data, except the enabled bit
          */
         dsa->stencil[1] = dsa->stencil[0];
         dsa->stencil[1].enabled = 0;
         sr.ref_value[1] = sr.ref_value[0];
      }
   }

   if (ctx->Color.AlphaEnabled) {
      dsa->alpha.enabled = 1;
      dsa->alpha.func = st_compare_func_to_pipe(ctx->Color.AlphaFunc);
      dsa->alpha.ref_value = ctx->Color.AlphaRefUnclamped;
   }

   cso_set_depth_stencil_alpha(st->cso_context, dsa);
   cso_set_stencil_ref(st->cso_context, &sr);
}


const struct st_tracked_state st_update_depth_stencil_alpha = {
   "st_update_depth_stencil",				/* name */
   {							/* dirty */
      (_NEW_DEPTH|_NEW_STENCIL|_NEW_COLOR),		/* mesa */
      0,						/* st */
   },
   update_depth_stencil_alpha				/* update */
};
