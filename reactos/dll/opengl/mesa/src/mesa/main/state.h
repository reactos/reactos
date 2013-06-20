/*
 * Mesa 3-D graphics library
 * Version:  7.3
 *
 * Copyright (C) 1999-2008  Brian Paul   All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */


#ifndef STATE_H
#define STATE_H

#include "mtypes.h"

extern void
_mesa_update_state(struct gl_context *ctx);

/* As above but can only be called between _mesa_lock_context_textures() and 
 * _mesa_unlock_context_textures().
 */
extern void
_mesa_update_state_locked(struct gl_context *ctx);


extern void
_mesa_set_varying_vp_inputs(struct gl_context *ctx, GLbitfield64 varying_inputs);


extern void
_mesa_set_vp_override(struct gl_context *ctx, GLboolean flag);


/**
 * Is the secondary color needed?
 */
static inline GLboolean
_mesa_need_secondary_color(const struct gl_context *ctx)
{
   if (ctx->Light.Enabled &&
       ctx->Light.Model.ColorControl == GL_SEPARATE_SPECULAR_COLOR)
       return GL_TRUE;

   if (ctx->Fog.ColorSumEnabled)
      return GL_TRUE;

   if (ctx->VertexProgram._Current &&
       (ctx->VertexProgram._Current != ctx->VertexProgram._TnlProgram) &&
       (ctx->VertexProgram._Current->Base.InputsRead & VERT_BIT_COLOR1))
      return GL_TRUE;

   if (ctx->FragmentProgram._Current &&
       (ctx->FragmentProgram._Current != ctx->FragmentProgram._TexEnvProgram) &&
       (ctx->FragmentProgram._Current->Base.InputsRead & FRAG_BIT_COL1))
      return GL_TRUE;

   return GL_FALSE;
}

#endif
