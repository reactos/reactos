/*
 * Mesa 3-D graphics library
 * Version:  6.5
 *
 * Copyright (C) 1999-2006  Brian Paul   All Rights Reserved.
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
#include "context.h"
#include "drawpix.h"
#include "feedback.h"
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
static GLboolean
error_check_format_type(GLcontext *ctx, GLenum format, GLenum type,
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
      if ((drawing && !_mesa_dest_buffer_exists(ctx, format)) ||
          (!drawing && !_mesa_source_buffer_exists(ctx, format))) {
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
      


#if _HAVE_FULL_GL

/*
 * Execute glDrawPixels
 */
void GLAPIENTRY
_mesa_DrawPixels( GLsizei width, GLsizei height,
                  GLenum format, GLenum type, const GLvoid *pixels )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);

   if (width < 0 || height < 0) {
      _mesa_error( ctx, GL_INVALID_VALUE, "glDrawPixels(width or height < 0" );
      return;
   }

   if (ctx->NewState) {
      _mesa_update_state(ctx);
   }

   if (ctx->FragmentProgram.Enabled && !ctx->FragmentProgram._Enabled) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glDrawPixels (invalid fragment program)");
      return;
   }

   if (error_check_format_type(ctx, format, type, GL_TRUE)) {
      /* found an error */
      return;
   }

   if (ctx->DrawBuffer->_Status != GL_FRAMEBUFFER_COMPLETE_EXT) {
      _mesa_error(ctx, GL_INVALID_FRAMEBUFFER_OPERATION_EXT,
                  "glDrawPixels(incomplete framebuffer)" );
      return;
   }

   if (!ctx->Current.RasterPosValid) {
      return;
   }

   if (ctx->RenderMode == GL_RENDER) {
      /* Round, to satisfy conformance tests (matches SGI's OpenGL) */
      GLint x = IROUND(ctx->Current.RasterPos[0]);
      GLint y = IROUND(ctx->Current.RasterPos[1]);
      ctx->Driver.DrawPixels(ctx, x, y, width, height, format, type,
			     &ctx->Unpack, pixels);
   }
   else if (ctx->RenderMode == GL_FEEDBACK) {
      /* Feedback the current raster pos info */
      FLUSH_CURRENT( ctx, 0 );
      FEEDBACK_TOKEN( ctx, (GLfloat) (GLint) GL_DRAW_PIXEL_TOKEN );
      _mesa_feedback_vertex( ctx,
                             ctx->Current.RasterPos,
                             ctx->Current.RasterColor,
                             ctx->Current.RasterIndex,
                             ctx->Current.RasterTexCoords[0] );
   }
   else {
      ASSERT(ctx->RenderMode == GL_SELECT);
      /* Do nothing.  See OpenGL Spec, Appendix B, Corollary 6. */
   }
}


void GLAPIENTRY
_mesa_CopyPixels( GLint srcx, GLint srcy, GLsizei width, GLsizei height,
                  GLenum type )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);

   if (ctx->NewState) {
      _mesa_update_state(ctx);
   }

   if (ctx->FragmentProgram.Enabled && !ctx->FragmentProgram._Enabled) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glCopyPixels (invalid fragment program)");
      return;
   }

   if (width < 0 || height < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glCopyPixels(width or height < 0)");
      return;
   }

   if (ctx->DrawBuffer->_Status != GL_FRAMEBUFFER_COMPLETE_EXT ||
       ctx->ReadBuffer->_Status != GL_FRAMEBUFFER_COMPLETE_EXT) {
      _mesa_error(ctx, GL_INVALID_FRAMEBUFFER_OPERATION_EXT,
                  "glCopyPixels(incomplete framebuffer)" );
      return;
   }

   if (!_mesa_source_buffer_exists(ctx, type) ||
       !_mesa_dest_buffer_exists(ctx, type)) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glCopyPixels(missing source or dest buffer)");
      return;
   }

   if (!ctx->Current.RasterPosValid) {
      return;
   }

   if (ctx->RenderMode == GL_RENDER) {
      /* Round to satisfy conformance tests (matches SGI's OpenGL) */
      GLint destx = IROUND(ctx->Current.RasterPos[0]);
      GLint desty = IROUND(ctx->Current.RasterPos[1]);
      ctx->Driver.CopyPixels( ctx, srcx, srcy, width, height, destx, desty,
			      type );
   }
   else if (ctx->RenderMode == GL_FEEDBACK) {
      FLUSH_CURRENT( ctx, 0 );
      FEEDBACK_TOKEN( ctx, (GLfloat) (GLint) GL_COPY_PIXEL_TOKEN );
      _mesa_feedback_vertex( ctx, 
                             ctx->Current.RasterPos,
                             ctx->Current.RasterColor,
                             ctx->Current.RasterIndex,
                             ctx->Current.RasterTexCoords[0] );
   }
   else {
      ASSERT(ctx->RenderMode == GL_SELECT);
      /* Do nothing.  See OpenGL Spec, Appendix B, Corollary 6. */
   }
}

#endif /* _HAVE_FULL_GL */



void GLAPIENTRY
_mesa_ReadPixels( GLint x, GLint y, GLsizei width, GLsizei height,
		  GLenum format, GLenum type, GLvoid *pixels )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);

   if (width < 0 || height < 0) {
      _mesa_error( ctx, GL_INVALID_VALUE,
                   "glReadPixels(width=%d height=%d)", width, height );
      return;
   }

   if (ctx->NewState)
      _mesa_update_state(ctx);

   if (error_check_format_type(ctx, format, type, GL_FALSE)) {
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

   ctx->Driver.ReadPixels(ctx, x, y, width, height,
			  format, type, &ctx->Pack, pixels);
}



void GLAPIENTRY
_mesa_Bitmap( GLsizei width, GLsizei height,
              GLfloat xorig, GLfloat yorig, GLfloat xmove, GLfloat ymove,
              const GLubyte *bitmap )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);

   if (width < 0 || height < 0) {
      _mesa_error( ctx, GL_INVALID_VALUE, "glBitmap(width or height < 0)" );
      return;
   }

   if (!ctx->Current.RasterPosValid) {
      return;    /* do nothing */
   }

   if (ctx->NewState) {
      _mesa_update_state(ctx);
   }

   if (ctx->FragmentProgram.Enabled && !ctx->FragmentProgram._Enabled) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glBitmap (invalid fragment program)");
      return;
   }

   if (ctx->DrawBuffer->_Status != GL_FRAMEBUFFER_COMPLETE_EXT) {
      _mesa_error(ctx, GL_INVALID_FRAMEBUFFER_OPERATION_EXT,
                  "glBitmap(incomplete framebuffer)");
      return;
   }

   if (ctx->RenderMode == GL_RENDER) {
      if (bitmap) {
         /* Truncate, to satisfy conformance tests (matches SGI's OpenGL). */
         GLint x = IFLOOR(ctx->Current.RasterPos[0] - xorig);
         GLint y = IFLOOR(ctx->Current.RasterPos[1] - yorig);
         ctx->Driver.Bitmap( ctx, x, y, width, height, &ctx->Unpack, bitmap );
      }
   }
#if _HAVE_FULL_GL
   else if (ctx->RenderMode == GL_FEEDBACK) {
      FLUSH_CURRENT(ctx, 0);
      FEEDBACK_TOKEN( ctx, (GLfloat) (GLint) GL_BITMAP_TOKEN );
      _mesa_feedback_vertex( ctx,
                             ctx->Current.RasterPos,
                             ctx->Current.RasterColor,
                             ctx->Current.RasterIndex, 
                             ctx->Current.RasterTexCoords[0] );
   }
   else {
      ASSERT(ctx->RenderMode == GL_SELECT);
      /* Do nothing.  See OpenGL Spec, Appendix B, Corollary 6. */
   }
#endif

   /* update raster position */
   ctx->Current.RasterPos[0] += xmove;
   ctx->Current.RasterPos[1] += ymove;
}



#if 0  /* experimental */
/*
 * Execute glDrawDepthPixelsMESA().  This function accepts both a color
 * image and depth (Z) image.  Rasterization produces fragments with
 * color and Z taken from these images.  This function is intended for
 * Z-compositing.  Normally, this operation requires two glDrawPixels
 * calls with stencil testing.
 */
void GLAPIENTRY
_mesa_DrawDepthPixelsMESA( GLsizei width, GLsizei height,
                           GLenum colorFormat, GLenum colorType,
                           const GLvoid *colors,
                           GLenum depthType, const GLvoid *depths )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);

   if (width < 0 || height < 0) {
      _mesa_error( ctx, GL_INVALID_VALUE,
                   "glDrawDepthPixelsMESA(width or height < 0" );
      return;
   }

   if (!ctx->Current.RasterPosValid) {
      return;
   }

   if (ctx->NewState) {
      _mesa_update_state(ctx);
   }

   if (ctx->DrawBuffer->_Status != GL_FRAMEBUFFER_COMPLETE_EXT) {
      _mesa_error(ctx, GL_INVALID_FRAMEBUFFER_OPERATION_EXT,
                  "glDrawDepthPixelsMESA(incomplete framebuffer)");
      return;
   }

   if (ctx->RenderMode == GL_RENDER) {
      /* Round, to satisfy conformance tests (matches SGI's OpenGL) */
      GLint x = IROUND(ctx->Current.RasterPos[0]);
      GLint y = IROUND(ctx->Current.RasterPos[1]);
      ctx->Driver.DrawDepthPixelsMESA(ctx, x, y, width, height,
                                      colorFormat, colorType, colors,
                                      depthType, depths, &ctx->Unpack);
   }
   else if (ctx->RenderMode == GL_FEEDBACK) {
      /* Feedback the current raster pos info */
      FLUSH_CURRENT( ctx, 0 );
      FEEDBACK_TOKEN( ctx, (GLfloat) (GLint) GL_DRAW_PIXEL_TOKEN );
      _mesa_feedback_vertex( ctx,
                             ctx->Current.RasterPos,
                             ctx->Current.RasterColor,
                             ctx->Current.RasterIndex,
                             ctx->Current.RasterTexCoords[0] );
   }
   else {
      ASSERT(ctx->RenderMode == GL_SELECT);
      /* Do nothing.  See OpenGL Spec, Appendix B, Corollary 6. */
   }
}

#endif
