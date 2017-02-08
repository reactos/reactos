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

#include <precomp.h>

#if FEATURE_drawpix


/*
 * Execute glDrawPixels
 */
static void GLAPIENTRY
_mesa_DrawPixels( GLsizei width, GLsizei height,
                  GLenum format, GLenum type, const GLvoid *pixels )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);

   if (MESA_VERBOSE & VERBOSE_API)
      _mesa_debug(ctx, "glDrawPixels(%d, %d, %s, %s, %p) // to %s at %d, %d\n",
                  width, height,
                  _mesa_lookup_enum_by_nr(format),
                  _mesa_lookup_enum_by_nr(type),
                  pixels,
                  _mesa_lookup_enum_by_nr(ctx->DrawBuffer->ColorDrawBuffer),
                  IROUND(ctx->Current.RasterPos[0]),
                  IROUND(ctx->Current.RasterPos[1]));


   if (width < 0 || height < 0) {
      _mesa_error( ctx, GL_INVALID_VALUE, "glDrawPixels(width or height < 0)" );
      return;
   }

   /* Note: this call does state validation */
   if (!_mesa_valid_to_render(ctx, "glDrawPixels")) {
      return;      /* the error code was recorded */
   }

   /* GL 3.0 introduced a new restriction on glDrawPixels() over what was in
    * GL_EXT_texture_integer.  From section 3.7.4 ("Rasterization of Pixel
    * Rectangles) on page 151 of the GL 3.0 specification:
    *
    *     "If format contains integer components, as shown in table 3.6, an
    *      INVALID OPERATION error is generated."
    *
    * Since DrawPixels rendering would be merely undefined if not an error (due
    * to a lack of defined mapping from integer data to gl_Color fragment shader
    * input), NVIDIA's implementation also just returns this error despite
    * exposing GL_EXT_texture_integer, just return an error regardless.
    */
   if (_mesa_is_integer_format(format)) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glDrawPixels(integer format)");
      return;
   }

   if (_mesa_error_check_format_type(ctx, format, type, GL_TRUE)) {
      return;      /* the error code was recorded */
   }

   if (ctx->RasterDiscard) {
      return;
   }

   if (!ctx->Current.RasterPosValid) {
      return;  /* no-op, not an error */
   }

   if (ctx->RenderMode == GL_RENDER) {
      if (width > 0 && height > 0) {
         /* Round, to satisfy conformance tests (matches SGI's OpenGL) */
         GLint x = IROUND(ctx->Current.RasterPos[0]);
         GLint y = IROUND(ctx->Current.RasterPos[1]);

         ctx->Driver.DrawPixels(ctx, x, y, width, height, format, type,
                                &ctx->Unpack, pixels);
      }
   }
   else if (ctx->RenderMode == GL_FEEDBACK) {
      /* Feedback the current raster pos info */
      FLUSH_CURRENT( ctx, 0 );
      _mesa_feedback_token( ctx, (GLfloat) (GLint) GL_DRAW_PIXEL_TOKEN );
      _mesa_feedback_vertex( ctx,
                             ctx->Current.RasterPos,
                             ctx->Current.RasterColor,
                             ctx->Current.RasterTexCoords );
   }
   else {
      ASSERT(ctx->RenderMode == GL_SELECT);
      /* Do nothing.  See OpenGL Spec, Appendix B, Corollary 6. */
   }
}


static void GLAPIENTRY
_mesa_CopyPixels( GLint srcx, GLint srcy, GLsizei width, GLsizei height,
                  GLenum type )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);

   if (MESA_VERBOSE & VERBOSE_API)
      _mesa_debug(ctx,
                  "glCopyPixels(%d, %d, %d, %d, %s) // from %s to %s at %d, %d\n",
                  srcx, srcy, width, height,
                  _mesa_lookup_enum_by_nr(type),
                  _mesa_lookup_enum_by_nr(ctx->ReadBuffer->ColorReadBuffer),
                  _mesa_lookup_enum_by_nr(ctx->DrawBuffer->ColorDrawBuffer),
                  IROUND(ctx->Current.RasterPos[0]),
                  IROUND(ctx->Current.RasterPos[1]));

   if (width < 0 || height < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glCopyPixels(width or height < 0)");
      return;
   }

   /* Note: more detailed 'type' checking is done by the
    * _mesa_source/dest_buffer_exists() calls below.  That's where we
    * check if the stencil buffer exists, etc.
    */
   if (type != GL_COLOR &&
       type != GL_DEPTH &&
       type != GL_STENCIL) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glCopyPixels(type=%s)",
                  _mesa_lookup_enum_by_nr(type));
      return;
   }

   /* Note: this call does state validation */
   if (!_mesa_valid_to_render(ctx, "glCopyPixels")) {
      return;      /* the error code was recorded */
   }

   if (!_mesa_source_buffer_exists(ctx, type) ||
       !_mesa_dest_buffer_exists(ctx, type)) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glCopyPixels(missing source or dest buffer)");
      return;
   }

   if (ctx->RasterDiscard) {
      return;
   }

   if (!ctx->Current.RasterPosValid || width == 0 || height == 0) {
      return; /* no-op, not an error */
   }

   if (ctx->RenderMode == GL_RENDER) {
      /* Round to satisfy conformance tests (matches SGI's OpenGL) */
      if (width > 0 && height > 0) {
         GLint destx = IROUND(ctx->Current.RasterPos[0]);
         GLint desty = IROUND(ctx->Current.RasterPos[1]);
         ctx->Driver.CopyPixels( ctx, srcx, srcy, width, height, destx, desty,
                                 type );
      }
   }
   else if (ctx->RenderMode == GL_FEEDBACK) {
      FLUSH_CURRENT( ctx, 0 );
      _mesa_feedback_token( ctx, (GLfloat) (GLint) GL_COPY_PIXEL_TOKEN );
      _mesa_feedback_vertex( ctx, 
                             ctx->Current.RasterPos,
                             ctx->Current.RasterColor,
                             ctx->Current.RasterTexCoords );
   }
   else {
      ASSERT(ctx->RenderMode == GL_SELECT);
      /* Do nothing.  See OpenGL Spec, Appendix B, Corollary 6. */
   }
}


static void GLAPIENTRY
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

   /* Note: this call does state validation */
   if (!_mesa_valid_to_render(ctx, "glBitmap")) {
      /* the error code was recorded */
      return;
   }

   if (ctx->RasterDiscard)
      return;

   if (ctx->RenderMode == GL_RENDER) {
      /* Truncate, to satisfy conformance tests (matches SGI's OpenGL). */
      if (width > 0 && height > 0) {
         const GLfloat epsilon = 0.0001F;
         GLint x = IFLOOR(ctx->Current.RasterPos[0] + epsilon - xorig);
         GLint y = IFLOOR(ctx->Current.RasterPos[1] + epsilon - yorig);

         ctx->Driver.Bitmap( ctx, x, y, width, height, &ctx->Unpack, bitmap );
      }
   }
#if _HAVE_FULL_GL
   else if (ctx->RenderMode == GL_FEEDBACK) {
      FLUSH_CURRENT(ctx, 0);
      _mesa_feedback_token( ctx, (GLfloat) (GLint) GL_BITMAP_TOKEN );
      _mesa_feedback_vertex( ctx,
                             ctx->Current.RasterPos,
                             ctx->Current.RasterColor,
                             ctx->Current.RasterTexCoords );
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


void
_mesa_init_drawpix_dispatch(struct _glapi_table *disp)
{
   SET_Bitmap(disp, _mesa_Bitmap);
   SET_CopyPixels(disp, _mesa_CopyPixels);
   SET_DrawPixels(disp, _mesa_DrawPixels);
}


#endif /* FEATURE_drawpix */
