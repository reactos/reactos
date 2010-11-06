/*
 * Mesa 3-D graphics library
 * Version:  7.1
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

#include "glheader.h"
#include "imports.h"
#include "bufferobj.h"
#include "context.h"
#include "readpix.h"
#include "framebuffer.h"
#include "image.h"
#include "state.h"


/**
 * Do error checking of the format/type parameters to glReadPixels and
 * glDrawPixels.
 * \param drawing if GL_TRUE do checking for DrawPixels, else do checking
 *                for ReadPixels.
 * \return GL_TRUE if error detected, GL_FALSE if no errors
 */
GLboolean
_mesa_error_check_format_type(GLcontext *ctx, GLenum format, GLenum type,
                              GLboolean drawing)
{
   const char *readDraw = drawing ? "Draw" : "Read";

   if (ctx->Extensions.EXT_packed_depth_stencil
       && type == GL_UNSIGNED_INT_24_8_EXT
       && format != GL_DEPTH_STENCIL_EXT) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "gl%sPixels(format is not GL_DEPTH_STENCIL_EXT)", readDraw);
      return GL_TRUE;
   }

   /* basic combinations test */
   if (!_mesa_is_legal_format_and_type(ctx, format, type)) {
      _mesa_error(ctx, GL_INVALID_ENUM,
                  "gl%sPixels(format or type)", readDraw);
      return GL_TRUE;
   }

   /* additional checks */
   switch (format) {
   case GL_RED:
   case GL_GREEN:
   case GL_BLUE:
   case GL_ALPHA:
   case GL_LUMINANCE:
   case GL_LUMINANCE_ALPHA:
   case GL_RGB:
   case GL_BGR:
   case GL_RGBA:
   case GL_BGRA:
   case GL_ABGR_EXT:
      if (drawing && !ctx->Visual.rgbMode) {
         _mesa_error(ctx, GL_INVALID_OPERATION,
                   "glDrawPixels(drawing RGB pixels into color index buffer)");
         return GL_TRUE;
      }
      if (!drawing && !_mesa_dest_buffer_exists(ctx, GL_COLOR)) {
         _mesa_error(ctx, GL_INVALID_OPERATION,
                     "glReadPixels(no color buffer)");
         return GL_TRUE;
      }
      break;
   case GL_COLOR_INDEX:
      if (!drawing && ctx->Visual.rgbMode) {
         _mesa_error(ctx, GL_INVALID_OPERATION,
                    "glReadPixels(reading color index format from RGB buffer)");
         return GL_TRUE;
      }
      if (!drawing && !_mesa_dest_buffer_exists(ctx, GL_COLOR)) {
         _mesa_error(ctx, GL_INVALID_OPERATION,
                     "glReadPixels(no color buffer)");
         return GL_TRUE;
      }
      break;
   case GL_STENCIL_INDEX:
      if ((drawing && !_mesa_dest_buffer_exists(ctx, format)) ||
          (!drawing && !_mesa_source_buffer_exists(ctx, format))) {
         _mesa_error(ctx, GL_INVALID_OPERATION,
                     "gl%sPixels(no stencil buffer)", readDraw);
         return GL_TRUE;
      }
      break;
   case GL_DEPTH_COMPONENT:
      if ((drawing && !_mesa_dest_buffer_exists(ctx, format))) {
         _mesa_error(ctx, GL_INVALID_OPERATION,
                     "gl%sPixels(no depth buffer)", readDraw);
         return GL_TRUE;
      }
      break;
   case GL_DEPTH_STENCIL_EXT:
      if (!ctx->Extensions.EXT_packed_depth_stencil ||
          type != GL_UNSIGNED_INT_24_8_EXT) {
         _mesa_error(ctx, GL_INVALID_ENUM, "gl%sPixels(type)", readDraw);
         return GL_TRUE;
      }
      if ((drawing && !_mesa_dest_buffer_exists(ctx, format)) ||
          (!drawing && !_mesa_source_buffer_exists(ctx, format))) {
         _mesa_error(ctx, GL_INVALID_OPERATION,
                     "gl%sPixels(no depth or stencil buffer)", readDraw);
         return GL_TRUE;
      }
      break;
   default:
      /* this should have been caught in _mesa_is_legal_format_type() */
      _mesa_problem(ctx, "unexpected format in _mesa_%sPixels", readDraw);
      return GL_TRUE;
   }

   /* no errors */
   return GL_FALSE;
}
      


void GLAPIENTRY
_mesa_ReadPixels( GLint x, GLint y, GLsizei width, GLsizei height,
		  GLenum format, GLenum type, GLvoid *pixels )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);

   FLUSH_CURRENT(ctx, 0);

   if (width < 0 || height < 0) {
      _mesa_error( ctx, GL_INVALID_VALUE,
                   "glReadPixels(width=%d height=%d)", width, height );
      return;
   }

   if (ctx->NewState)
      _mesa_update_state(ctx);

   if (_mesa_error_check_format_type(ctx, format, type, GL_FALSE)) {
      /* found an error */
      return;
   }

   if (ctx->ReadBuffer->_Status != GL_FRAMEBUFFER_COMPLETE_EXT) {
      _mesa_error(ctx, GL_INVALID_FRAMEBUFFER_OPERATION_EXT,
                  "glReadPixels(incomplete framebuffer)" );
      return;
   }

   if (!_mesa_source_buffer_exists(ctx, format)) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glReadPixels(no readbuffer)");
      return;
   }

   if (ctx->Pack.BufferObj->Name) {
      if (!_mesa_validate_pbo_access(2, &ctx->Pack, width, height, 1,
                                     format, type, pixels)) {
         _mesa_error(ctx, GL_INVALID_OPERATION,
                     "glReadPixels(invalid PBO access)");
         return;
      }

      if (ctx->Pack.BufferObj->Pointer) {
         /* buffer is mapped - that's an error */
         _mesa_error(ctx, GL_INVALID_OPERATION, "glReadPixels(PBO is mapped)");
         return;
      }
   }

   ctx->Driver.ReadPixels(ctx, x, y, width, height,
			  format, type, &ctx->Pack, pixels);
}
