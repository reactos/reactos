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
#include "bufferobj.h"
#include "context.h"
#include "imports.h"
#include "mfeatures.h"
#include "mtypes.h"
#include "vbo/vbo.h"


/**
 * \return  number of bytes in array [count] of type.
 */
static GLsizei
index_bytes(GLenum type, GLsizei count)
{
   if (type == GL_UNSIGNED_INT) {
      return count * sizeof(GLuint);
   }
   else if (type == GL_UNSIGNED_BYTE) {
      return count * sizeof(GLubyte);
   }
   else {
      ASSERT(type == GL_UNSIGNED_SHORT);
      return count * sizeof(GLushort);
   }
}


/**
 * Find the max index in the given element/index buffer
 */
GLuint
_mesa_max_buffer_index(struct gl_context *ctx, GLuint count, GLenum type,
                       const void *indices,
                       struct gl_buffer_object *elementBuf)
{
   const GLubyte *map = NULL;
   GLuint max = 0;
   GLuint i;

   if (_mesa_is_bufferobj(elementBuf)) {
      /* elements are in a user-defined buffer object.  need to map it */
      map = ctx->Driver.MapBufferRange(ctx, 0, elementBuf->Size,
				       GL_MAP_READ_BIT, elementBuf);
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
      ctx->Driver.UnmapBuffer(ctx, elementBuf);
   }

   return max;
}


/**
 * Check if OK to draw arrays/elements.
 */
static GLboolean
check_valid_to_render(struct gl_context *ctx, const char *function)
{
   if (!_mesa_valid_to_render(ctx, function)) {
      return GL_FALSE;
   }

   switch (ctx->API) {
#if FEATURE_es2_glsl
   case API_OPENGLES2:
      /* For ES2, we can draw if any vertex array is enabled (and we
       * should always have a vertex program/shader). */
      if (ctx->Array.ArrayObj->_Enabled == 0x0 || !ctx->VertexProgram._Current)
	 return GL_FALSE;
      break;
#endif

#if FEATURE_ES1
   case API_OPENGLES:
      /* For OpenGL ES, only draw if we have vertex positions
       */
      if (!ctx->Array.ArrayObj->VertexAttrib[VERT_ATTRIB_POS].Enabled)
	 return GL_FALSE;
      break;
#endif

#if FEATURE_GL
   case API_OPENGL:
      {
         const struct gl_shader_program *vsProg =
            ctx->Shader.CurrentVertexProgram;
         GLboolean haveVertexShader = (vsProg && vsProg->LinkStatus);
         GLboolean haveVertexProgram = ctx->VertexProgram._Enabled;
         if (haveVertexShader || haveVertexProgram) {
            /* Draw regardless of whether or not we have any vertex arrays.
             * (Ex: could draw a point using a constant vertex pos)
             */
            return GL_TRUE;
         }
         else {
            /* Draw if we have vertex positions (GL_VERTEX_ARRAY or generic
             * array [0]).
             */
            return (ctx->Array.ArrayObj->VertexAttrib[VERT_ATTRIB_POS].Enabled ||
                    ctx->Array.ArrayObj->VertexAttrib[VERT_ATTRIB_GENERIC0].Enabled);
         }
      }
      break;
#endif

   default:
      ASSERT_NO_FEATURE();
   }

   return GL_TRUE;
}


/**
 * Do bounds checking on array element indexes.  Check that the vertices
 * pointed to by the indices don't lie outside buffer object bounds.
 * \return GL_TRUE if OK, GL_FALSE if any indexed vertex goes is out of bounds
 */
static GLboolean
check_index_bounds(struct gl_context *ctx, GLsizei count, GLenum type,
		   const GLvoid *indices, GLint basevertex)
{
   struct _mesa_prim prim;
   struct _mesa_index_buffer ib;
   GLuint min, max;

   /* Only the X Server needs to do this -- otherwise, accessing outside
    * array/BO bounds allows application termination.
    */
   if (!ctx->Const.CheckArrayBounds)
      return GL_TRUE;

   memset(&prim, 0, sizeof(prim));
   prim.count = count;

   memset(&ib, 0, sizeof(ib));
   ib.type = type;
   ib.ptr = indices;
   ib.obj = ctx->Array.ArrayObj->ElementArrayBufferObj;

   vbo_get_minmax_index(ctx, &prim, &ib, &min, &max);

   if ((int)(min + basevertex) < 0 ||
       max + basevertex >= ctx->Array.ArrayObj->_MaxElement) {
      /* the max element is out of bounds of one or more enabled arrays */
      _mesa_warning(ctx, "glDrawElements() index=%u is out of bounds (max=%u)",
                    max, ctx->Array.ArrayObj->_MaxElement);
      return GL_FALSE;
   }

   return GL_TRUE;
}


/**
 * Is 'mode' a valid value for glBegin(), glDrawArrays(), glDrawElements(),
 * etc?  The set of legal values depends on whether geometry shaders/programs
 * are supported.
 */
GLboolean
_mesa_valid_prim_mode(const struct gl_context *ctx, GLenum mode)
{
   if (ctx->Extensions.ARB_geometry_shader4 &&
       mode > GL_TRIANGLE_STRIP_ADJACENCY_ARB) {
      return GL_FALSE;
   }
   else if (mode > GL_POLYGON) {
      return GL_FALSE;
   }
   else {
      return GL_TRUE;
   }
}


/**
 * Error checking for glDrawElements().  Includes parameter checking
 * and VBO bounds checking.
 * \return GL_TRUE if OK to render, GL_FALSE if error found
 */
GLboolean
_mesa_validate_DrawElements(struct gl_context *ctx,
			    GLenum mode, GLsizei count, GLenum type,
			    const GLvoid *indices, GLint basevertex)
{
   ASSERT_OUTSIDE_BEGIN_END_WITH_RETVAL(ctx, GL_FALSE);

   if (count <= 0) {
      if (count < 0)
	 _mesa_error(ctx, GL_INVALID_VALUE, "glDrawElements(count)" );
      return GL_FALSE;
   }

   if (!_mesa_valid_prim_mode(ctx, mode)) {
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

   if (!check_valid_to_render(ctx, "glDrawElements"))
      return GL_FALSE;

   /* Vertex buffer object tests */
   if (_mesa_is_bufferobj(ctx->Array.ArrayObj->ElementArrayBufferObj)) {
      /* use indices in the buffer object */
      /* make sure count doesn't go outside buffer bounds */
      if (index_bytes(type, count) > ctx->Array.ArrayObj->ElementArrayBufferObj->Size) {
         _mesa_warning(ctx, "glDrawElements index out of buffer bounds");
         return GL_FALSE;
      }
   }
   else {
      /* not using a VBO */
      if (!indices)
         return GL_FALSE;
   }

   if (!check_index_bounds(ctx, count, type, indices, basevertex))
      return GL_FALSE;

   return GL_TRUE;
}


/**
 * Error checking for glDrawRangeElements().  Includes parameter checking
 * and VBO bounds checking.
 * \return GL_TRUE if OK to render, GL_FALSE if error found
 */
GLboolean
_mesa_validate_DrawRangeElements(struct gl_context *ctx, GLenum mode,
				 GLuint start, GLuint end,
				 GLsizei count, GLenum type,
				 const GLvoid *indices, GLint basevertex)
{
   ASSERT_OUTSIDE_BEGIN_END_WITH_RETVAL(ctx, GL_FALSE);

   if (count <= 0) {
      if (count < 0)
	 _mesa_error(ctx, GL_INVALID_VALUE, "glDrawRangeElements(count)" );
      return GL_FALSE;
   }

   if (!_mesa_valid_prim_mode(ctx, mode)) {
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

   if (!check_valid_to_render(ctx, "glDrawRangeElements"))
      return GL_FALSE;

   /* Vertex buffer object tests */
   if (_mesa_is_bufferobj(ctx->Array.ArrayObj->ElementArrayBufferObj)) {
      /* use indices in the buffer object */
      /* make sure count doesn't go outside buffer bounds */
      if (index_bytes(type, count) > ctx->Array.ArrayObj->ElementArrayBufferObj->Size) {
         _mesa_warning(ctx, "glDrawRangeElements index out of buffer bounds");
         return GL_FALSE;
      }
   }
   else {
      /* not using a VBO */
      if (!indices)
         return GL_FALSE;
   }

   if (!check_index_bounds(ctx, count, type, indices, basevertex))
      return GL_FALSE;

   return GL_TRUE;
}


/**
 * Called from the tnl module to error check the function parameters and
 * verify that we really can draw something.
 * \return GL_TRUE if OK to render, GL_FALSE if error found
 */
GLboolean
_mesa_validate_DrawArrays(struct gl_context *ctx,
			  GLenum mode, GLint start, GLsizei count)
{
   ASSERT_OUTSIDE_BEGIN_END_WITH_RETVAL(ctx, GL_FALSE);

   if (count <= 0) {
      if (count < 0)
         _mesa_error(ctx, GL_INVALID_VALUE, "glDrawArrays(count)" );
      return GL_FALSE;
   }

   if (!_mesa_valid_prim_mode(ctx, mode)) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glDrawArrays(mode)" );
      return GL_FALSE;
   }

   if (!check_valid_to_render(ctx, "glDrawArrays"))
      return GL_FALSE;

   if (ctx->Const.CheckArrayBounds) {
      if (start + count > (GLint) ctx->Array.ArrayObj->_MaxElement)
         return GL_FALSE;
   }

   return GL_TRUE;
}


GLboolean
_mesa_validate_DrawArraysInstanced(struct gl_context *ctx, GLenum mode, GLint first,
                                   GLsizei count, GLsizei numInstances)
{
   ASSERT_OUTSIDE_BEGIN_END_WITH_RETVAL(ctx, GL_FALSE);

   if (count <= 0) {
      if (count < 0)
         _mesa_error(ctx, GL_INVALID_VALUE,
                     "glDrawArraysInstanced(count=%d)", count);
      return GL_FALSE;
   }

   if (first < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE,
		  "glDrawArraysInstanced(start=%d)", first);
      return GL_FALSE;
   }

   if (!_mesa_valid_prim_mode(ctx, mode)) {
      _mesa_error(ctx, GL_INVALID_ENUM,
                  "glDrawArraysInstanced(mode=0x%x)", mode);
      return GL_FALSE;
   }

   if (numInstances <= 0) {
      if (numInstances < 0)
         _mesa_error(ctx, GL_INVALID_VALUE,
                     "glDrawArraysInstanced(numInstances=%d)", numInstances);
      return GL_FALSE;
   }

   if (!check_valid_to_render(ctx, "glDrawArraysInstanced(invalid to render)"))
      return GL_FALSE;

   if (ctx->Const.CheckArrayBounds) {
      if (first + count > (GLint) ctx->Array.ArrayObj->_MaxElement)
         return GL_FALSE;
   }

   return GL_TRUE;
}


GLboolean
_mesa_validate_DrawElementsInstanced(struct gl_context *ctx,
                                     GLenum mode, GLsizei count, GLenum type,
                                     const GLvoid *indices, GLsizei numInstances,
                                     GLint basevertex)
{
   ASSERT_OUTSIDE_BEGIN_END_WITH_RETVAL(ctx, GL_FALSE);

   if (count <= 0) {
      if (count < 0)
	 _mesa_error(ctx, GL_INVALID_VALUE,
                     "glDrawElementsInstanced(count=%d)", count);
      return GL_FALSE;
   }

   if (!_mesa_valid_prim_mode(ctx, mode)) {
      _mesa_error(ctx, GL_INVALID_ENUM,
                  "glDrawElementsInstanced(mode = 0x%x)", mode);
      return GL_FALSE;
   }

   if (type != GL_UNSIGNED_INT &&
       type != GL_UNSIGNED_BYTE &&
       type != GL_UNSIGNED_SHORT) {
      _mesa_error(ctx, GL_INVALID_ENUM,
                  "glDrawElementsInstanced(type=0x%x)", type);
      return GL_FALSE;
   }

   if (numInstances <= 0) {
      if (numInstances < 0)
         _mesa_error(ctx, GL_INVALID_VALUE,
                     "glDrawElementsInstanced(numInstances=%d)", numInstances);
      return GL_FALSE;
   }

   if (!check_valid_to_render(ctx, "glDrawElementsInstanced"))
      return GL_FALSE;

   /* Vertex buffer object tests */
   if (_mesa_is_bufferobj(ctx->Array.ArrayObj->ElementArrayBufferObj)) {
      /* use indices in the buffer object */
      /* make sure count doesn't go outside buffer bounds */
      if (index_bytes(type, count) > ctx->Array.ArrayObj->ElementArrayBufferObj->Size) {
         _mesa_warning(ctx,
                       "glDrawElementsInstanced index out of buffer bounds");
         return GL_FALSE;
      }
   }
   else {
      /* not using a VBO */
      if (!indices)
         return GL_FALSE;
   }

   if (!check_index_bounds(ctx, count, type, indices, basevertex))
      return GL_FALSE;

   return GL_TRUE;
}


#if FEATURE_EXT_transform_feedback

GLboolean
_mesa_validate_DrawTransformFeedback(struct gl_context *ctx,
                                     GLenum mode,
                                     struct gl_transform_feedback_object *obj)
{
   ASSERT_OUTSIDE_BEGIN_END_WITH_RETVAL(ctx, GL_FALSE);

   if (!_mesa_valid_prim_mode(ctx, mode)) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glDrawTransformFeedback(mode)");
      return GL_FALSE;
   }

   if (!obj) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glDrawTransformFeedback(name)");
      return GL_FALSE;
   }

   if (!obj->EndedAnytime) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glDrawTransformFeedback");
      return GL_FALSE;
   }

   if (!check_valid_to_render(ctx, "glDrawTransformFeedback")) {
      return GL_FALSE;
   }

   return GL_TRUE;
}

#endif
