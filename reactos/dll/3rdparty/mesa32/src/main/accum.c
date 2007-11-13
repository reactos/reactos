/*
 * Mesa 3-D graphics library
 * Version:  6.5
 *
 * Copyright (C) 1999-2005  Brian Paul   All Rights Reserved.
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

#include "glheader.h"
#include "accum.h"
#include "context.h"
#include "imports.h"
#include "macros.h"
#include "state.h"
#include "mtypes.h"


void GLAPIENTRY
_mesa_ClearAccum( GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha )
{
   GLfloat tmp[4];
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   tmp[0] = CLAMP( red,   -1.0F, 1.0F );
   tmp[1] = CLAMP( green, -1.0F, 1.0F );
   tmp[2] = CLAMP( blue,  -1.0F, 1.0F );
   tmp[3] = CLAMP( alpha, -1.0F, 1.0F );

   if (TEST_EQ_4V(tmp, ctx->Accum.ClearColor))
      return;

   FLUSH_VERTICES(ctx, _NEW_ACCUM);
   COPY_4FV( ctx->Accum.ClearColor, tmp );
}


void GLAPIENTRY
_mesa_Accum( GLenum op, GLfloat value )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);

   switch (op) {
   case GL_ADD:
   case GL_MULT:
   case GL_ACCUM:
   case GL_LOAD:
   case GL_RETURN:
      /* OK */
      break;
   default:
      _mesa_error(ctx, GL_INVALID_ENUM, "glAccum(op)");
      return;
   }

   if (ctx->DrawBuffer->Visual.haveAccumBuffer == 0) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glAccum(no accum buffer)");
      return;
   }

   if (ctx->DrawBuffer != ctx->ReadBuffer) {
      /* See GLX_SGI_make_current_read or WGL_ARB_make_current_read,
       * or GL_EXT_framebuffer_blit.
       */
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glAccum(different read/draw buffers)");
      return;
   }

   if (ctx->NewState)
      _mesa_update_state(ctx);

   if (ctx->DrawBuffer->_Status != GL_FRAMEBUFFER_COMPLETE_EXT) {
      _mesa_error(ctx, GL_INVALID_FRAMEBUFFER_OPERATION_EXT,
                  "glAccum(incomplete framebuffer)");
      return;
   }

   if (ctx->RenderMode == GL_RENDER) {
      ctx->Driver.Accum(ctx, op, value);
   }
}



void 
_mesa_init_accum( GLcontext *ctx )
{
   /* Accumulate buffer group */
   ASSIGN_4V( ctx->Accum.ClearColor, 0.0, 0.0, 0.0, 0.0 );
}
