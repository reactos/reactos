/*
 * Mesa 3-D graphics library
 * Version:  7.1
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


/**
 * Find the max index in the given element/index buffer
 */
static GLuint
max_buffer_index(GLcontext *ctx, GLuint count, GLenum type,
                 const void *indices,
                 struct gl_buffer_object *elementBuf)
{
   const GLubyte *map = NULL;
   GLuint max = 0;
   GLint i;

   if (elementBuf->Name) {
      /* elements are in a user-defined buffer object.  need to map it */
      map = ctx->Driver.MapBuffer(ctx,
                                  GL_ELEMENT_ARRAY_BUFFER_ARB,
                                  GL_READ_ONLY,
                                  elementBuf);
      /* Actual address is the sum of pointers */
      indices = (const GLvoid *) ADD_POINTERS(map, (const GLubyte *) indices);
   }

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

   if (map) {
      ctx->Driver.UnmapBuffer(ctx,
                              GL_ELEMENT_ARRAY_BUFFER_ARB,
                              ctx->Array.ElementArrayBufferObj);
   }

   return max;
}

static GLboolean
check_valid_to_render(GLcontext *ctx, char *function)
{
   if (ctx->DrawBuffer->_Status != GL_FRAMEBUFFER_COMPLETE_EXT) {
      _mesa_error(ctx, GL_INVALID_FRAMEBUFFER_OPERATION_EXT,
                  "glDraw%s(incomplete framebuffer)", function);
      return GL_FALSE;
   }

#if FEATURE_es2_glsl
   /* For ES2, we can draw if any vertex array is enabled (and we should
    * always have a vertex program/shader).
    */
   if (ctx->Array.ArrayObj->_Enabled == 0x0 || !ctx->VertexProgram._Current)
      return GL_FALSE;
#else
   /* For regular OpenGL, only draw if we have vertex positions (regardless
    * of whether or not we have a vertex program/shader).
    */
   if (!ctx->Array.ArrayObj->Vertex.Enabled &&
       !ctx->Array.ArrayObj->VertexAttrib[0].Enabled)
      return GL_FALSE;
#endif

   return GL_TRUE;
}

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

   if (!check_valid_to_render(ctx, "Elements"))
      return GL_FALSE;

   /* Vertex buffer object tests */
   if (ctx->Array.ElementArrayBufferObj->Name) {
      /* use indices in the buffer object */
      GLuint indexBytes;

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

      /* make sure count doesn't go outside buffer bounds */
      if (indexBytes > (GLuint) ctx->Array.ElementArrayBufferObj->Size) {
         _mesa_warning(ctx, "glDrawElements index out of buffer bounds");
         return GL_FALSE;
      }
   }
   else {
      /* not using a VBO */
      if (!indices)
         return GL_FALSE;
   }

   if (ctx->Const.CheckArrayBounds) {
      /* find max array index */
      GLuint max = max_buffer_index(ctx, count, type, indices,
                                    ctx->Array.ElementArrayBufferObj);
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

   if (!check_valid_to_render(ctx, "RangeElements"))
      return GL_FALSE;

   /* Vertex buffer object tests */
   if (ctx->Array.ElementArrayBufferObj->Name) {
      /* use indices in the buffer object */
      GLuint indexBytes;

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

      /* make sure count doesn't go outside buffer bounds */
      if (indexBytes > ctx->Array.ElementArrayBufferObj->Size) {
         _mesa_warning(ctx, "glDrawRangeElements index out of buffer bounds");
         return GL_FALSE;
      }
   }
   else {
      /* not using a VBO */
      if (!indices)
         return GL_FALSE;
   }

   if (ctx->Const.CheckArrayBounds) {
      GLuint max = max_buffer_index(ctx, count, type, indices,
                                    ctx->Array.ElementArrayBufferObj);
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

   if (count <= 0) {
      if (count < 0)
         _mesa_error(ctx, GL_INVALID_VALUE, "glDrawArrays(count)" );
      return GL_FALSE;
   }

   if (mode > GL_POLYGON) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glDrawArrays(mode)" );
      return GL_FALSE;
   }

   if (ctx->NewState)
      _mesa_update_state(ctx);

   if (!check_valid_to_render(ctx, "Arrays"))
      return GL_FALSE;

   if (ctx->Const.CheckArrayBounds) {
      if (start + count > (GLint) ctx->Array._MaxElement)
         return GL_FALSE;
   }

   return GL_TRUE;
}
