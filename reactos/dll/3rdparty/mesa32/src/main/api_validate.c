/*
 * Mesa 3-D graphics library
 * Version:  7.0.1
 *
 * Copyright (C) 1999-2007  Brian Paul   All Rights Reserved.
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
#include "api_validate.h"
#include "context.h"
#include "imports.h"
#include "mtypes.h"
#include "state.h"


GLboolean
_mesa_validate_DrawElements(GLcontext *ctx,
			    GLenum mode, GLsizei count, GLenum type,
			    const GLvoid *indices)
{
   ASSERT_OUTSIDE_BEGIN_END_WITH_RETVAL(ctx,  GL_FALSE);

   if (count <= 0) {
      if (count < 0)
	 _mesa_error(ctx, GL_INVALID_VALUE, "glDrawElements(count)" );
      return GL_FALSE;
   }

   if (mode > GL_POLYGON) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glDrawElements(mode)" );
      return GL_FALSE;
   }

   if (type != GL_UNSIGNED_INT &&
       type != GL_UNSIGNED_BYTE &&
       type != GL_UNSIGNED_SHORT)
   {
      _mesa_error(ctx, GL_INVALID_ENUM, "glDrawElements(type)" );
      return GL_FALSE;
   }

   if (ctx->NewState)
      _mesa_update_state(ctx);

   /* Always need vertex positions */
   if (!ctx->Array.ArrayObj->Vertex.Enabled
       && !(ctx->VertexProgram._Enabled && ctx->Array.ArrayObj->VertexAttrib[0].Enabled))
      return GL_FALSE;

   /* Vertex buffer object tests */
   if (ctx->Array.ElementArrayBufferObj->Name) {
      GLuint indexBytes;

      /* use indices in the buffer object */
      if (!ctx->Array.ElementArrayBufferObj->Data) {
         _mesa_warning(ctx, "DrawElements with empty vertex elements buffer!");
         return GL_FALSE;
      }

      /* make sure count doesn't go outside buffer bounds */
      if (type == GL_UNSIGNED_INT) {
         indexBytes = count * sizeof(GLuint);
      }
      else if (type == GL_UNSIGNED_BYTE) {
         indexBytes = count * sizeof(GLubyte);
      }
      else {
         ASSERT(type == GL_UNSIGNED_SHORT);
         indexBytes = count * sizeof(GLushort);
      }

      if ((GLubyte *) indices + indexBytes >
          ctx->Array.ElementArrayBufferObj->Data +
          ctx->Array.ElementArrayBufferObj->Size) {
         _mesa_warning(ctx, "glDrawElements index out of buffer bounds");
         return GL_FALSE;
      }

      /* Actual address is the sum of pointers.  Indices may be used below. */
      if (ctx->Const.CheckArrayBounds) {
         indices = (const GLvoid *)
            ADD_POINTERS(ctx->Array.ElementArrayBufferObj->Data,
                         (const GLubyte *) indices);
      }
   }
   else {
      /* not using a VBO */
      if (!indices)
         return GL_FALSE;
   }

   if (ctx->Const.CheckArrayBounds) {
      /* find max array index */
      GLuint max = 0;
      GLint i;
      if (type == GL_UNSIGNED_INT) {
         for (i = 0; i < count; i++)
            if (((GLuint *) indices)[i] > max)
               max = ((GLuint *) indices)[i];
      }
      else if (type == GL_UNSIGNED_SHORT) {
         for (i = 0; i < count; i++)
            if (((GLushort *) indices)[i] > max)
               max = ((GLushort *) indices)[i];
      }
      else {
         ASSERT(type == GL_UNSIGNED_BYTE);
         for (i = 0; i < count; i++)
            if (((GLubyte *) indices)[i] > max)
               max = ((GLubyte *) indices)[i];
      }
      if (max >= ctx->Array._MaxElement) {
         /* the max element is out of bounds of one or more enabled arrays */
         return GL_FALSE;
      }
   }

   return GL_TRUE;
}


GLboolean
_mesa_validate_DrawRangeElements(GLcontext *ctx, GLenum mode,
				 GLuint start, GLuint end,
				 GLsizei count, GLenum type,
				 const GLvoid *indices)
{
   ASSERT_OUTSIDE_BEGIN_END_WITH_RETVAL(ctx, GL_FALSE);

   if (count <= 0) {
      if (count < 0)
	 _mesa_error(ctx, GL_INVALID_VALUE, "glDrawRangeElements(count)" );
      return GL_FALSE;
   }

   if (mode > GL_POLYGON) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glDrawRangeElements(mode)" );
      return GL_FALSE;
   }

   if (end < start) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glDrawRangeElements(end<start)");
      return GL_FALSE;
   }

   if (type != GL_UNSIGNED_INT &&
       type != GL_UNSIGNED_BYTE &&
       type != GL_UNSIGNED_SHORT) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glDrawRangeElements(type)" );
      return GL_FALSE;
   }

   if (ctx->NewState)
      _mesa_update_state(ctx);

   /* Always need vertex positions */
   if (!ctx->Array.ArrayObj->Vertex.Enabled
       && !(ctx->VertexProgram._Enabled && ctx->Array.ArrayObj->VertexAttrib[0].Enabled))
      return GL_FALSE;

   /* Vertex buffer object tests */
   if (ctx->Array.ElementArrayBufferObj->Name) {
      /* XXX re-use code from above? */
   }
   else {
      /* not using VBO */
      if (!indices)
         return GL_FALSE;
   }

   if (ctx->Const.CheckArrayBounds) {
      /* Find max array index.
       * We don't trust the user's start and end values.
       */
      GLuint max = 0;
      GLint i;
      if (type == GL_UNSIGNED_INT) {
         for (i = 0; i < count; i++)
            if (((GLuint *) indices)[i] > max)
               max = ((GLuint *) indices)[i];
      }
      else if (type == GL_UNSIGNED_SHORT) {
         for (i = 0; i < count; i++)
            if (((GLushort *) indices)[i] > max)
               max = ((GLushort *) indices)[i];
      }
      else {
         ASSERT(type == GL_UNSIGNED_BYTE);
         for (i = 0; i < count; i++)
            if (((GLubyte *) indices)[i] > max)
               max = ((GLubyte *) indices)[i];
      }
      if (max >= ctx->Array._MaxElement) {
         /* the max element is out of bounds of one or more enabled arrays */
         return GL_FALSE;
      }
   }

   return GL_TRUE;
}


/**
 * Called from the tnl module to error check the function parameters and
 * verify that we really can draw something.
 */
GLboolean
_mesa_validate_DrawArrays(GLcontext *ctx,
			  GLenum mode, GLint start, GLsizei count)
{
   ASSERT_OUTSIDE_BEGIN_END_WITH_RETVAL(ctx, GL_FALSE);

   if (count < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glDrawArrays(count)" );
      return GL_FALSE;
   }

   if (mode > GL_POLYGON) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glDrawArrays(mode)" );
      return GL_FALSE;
   }

   if (ctx->NewState)
      _mesa_update_state(ctx);

   /* Always need vertex positions */
   if (!ctx->Array.ArrayObj->Vertex.Enabled && !ctx->Array.ArrayObj->VertexAttrib[0].Enabled)
      return GL_FALSE;

   if (ctx->Const.CheckArrayBounds) {
      if (start + count > (GLint) ctx->Array._MaxElement)
         return GL_FALSE;
   }

   return GL_TRUE;
}
