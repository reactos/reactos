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

#include <precomp.h>

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

   /* Draw if we have vertex positions (GL_VERTEX_ARRAY or generic
    * array [0]).
    */
   return ctx->Array.VertexAttrib[VERT_ATTRIB_POS].Enabled;
}


/**
 * Do bounds checking on array element indexes.  Check that the vertices
 * pointed to by the indices don't lie outside buffer object bounds.
 * \return GL_TRUE if OK, GL_FALSE if any indexed vertex goes is out of bounds
 */
static GLboolean
check_index_bounds(struct gl_context *ctx, GLsizei count, GLenum type,
		   const GLvoid *indices)
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
   ib.obj = ctx->Array.ElementArrayBufferObj;

   vbo_get_minmax_index(ctx, &prim, &ib, &min, &max);

   if (max >= ctx->Array._MaxElement) {
      /* the max element is out of bounds of one or more enabled arrays */
      _mesa_warning(ctx, "glDrawElements() index=%u is out of bounds (max=%u)",
                    max, ctx->Array._MaxElement);
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
   if (mode > GL_POLYGON) {
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
			    const GLvoid *indices)
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
   if (_mesa_is_bufferobj(ctx->Array.ElementArrayBufferObj)) {
      /* use indices in the buffer object */
      /* make sure count doesn't go outside buffer bounds */
      if (index_bytes(type, count) > ctx->Array.ElementArrayBufferObj->Size) {
         _mesa_warning(ctx, "glDrawElements index out of buffer bounds");
         return GL_FALSE;
      }
   }
   else {
      /* not using a VBO */
      if (!indices)
         return GL_FALSE;
   }

   if (!check_index_bounds(ctx, count, type, indices))
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
      if (start + count > (GLint) ctx->Array._MaxElement)
         return GL_FALSE;
   }

   return GL_TRUE;
}
