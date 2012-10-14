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


/**
 * \file clear.c
 * glClearColor, glClearIndex, glClear() functions.
 */



#include "glheader.h"
#include "clear.h"
#include "context.h"
#include "colormac.h"
#include "enums.h"
#include "macros.h"
#include "mtypes.h"
#include "state.h"



#if _HAVE_FULL_GL
void GLAPIENTRY
_mesa_ClearIndex( GLfloat c )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (ctx->Color.ClearIndex == (GLuint) c)
      return;

   FLUSH_VERTICES(ctx, _NEW_COLOR);
   ctx->Color.ClearIndex = (GLuint) c;
}
#endif


/**
 * Specify the clear values for the color buffers.
 *
 * \param red red color component.
 * \param green green color component.
 * \param blue blue color component.
 * \param alpha alpha component.
 *
 * \sa glClearColor().
 *
 * Clamps the parameters and updates gl_colorbuffer_attrib::ClearColor.  On a
 * change, flushes the vertices and notifies the driver via the
 * dd_function_table::ClearColor callback.
 */
void GLAPIENTRY
_mesa_ClearColor( GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha )
{
   GLfloat tmp[4];
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   tmp[0] = red;
   tmp[1] = green;
   tmp[2] = blue;
   tmp[3] = alpha;

   if (TEST_EQ_4V(tmp, ctx->Color.ClearColor.f))
      return; /* no change */

   FLUSH_VERTICES(ctx, _NEW_COLOR);
   COPY_4V(ctx->Color.ClearColor.f, tmp);

   if (ctx->Driver.ClearColor) {
      /* it's OK to call glClearColor in CI mode but it should be a NOP */
      /* we pass the clamped color, since all drivers that need this don't
       * support GL_ARB_color_buffer_float
       */
      (*ctx->Driver.ClearColor)(ctx, ctx->Color.ClearColor);
   }
}


/**
 * GL_EXT_texture_integer
 */
void GLAPIENTRY
_mesa_ClearColorIiEXT(GLint r, GLint g, GLint b, GLint a)
{
   GLint tmp[4];
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   tmp[0] = r;
   tmp[1] = g;
   tmp[2] = b;
   tmp[3] = a;

   if (TEST_EQ_4V(tmp, ctx->Color.ClearColor.i))
      return; /* no change */

   FLUSH_VERTICES(ctx, _NEW_COLOR);
   COPY_4V(ctx->Color.ClearColor.i, tmp);

   /* these should be NOP calls for drivers supporting EXT_texture_integer */
   if (ctx->Driver.ClearColor) {
      ctx->Driver.ClearColor(ctx, ctx->Color.ClearColor);
   }
}


/**
 * GL_EXT_texture_integer
 */
void GLAPIENTRY
_mesa_ClearColorIuiEXT(GLuint r, GLuint g, GLuint b, GLuint a)
{
   GLuint tmp[4];
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   tmp[0] = r;
   tmp[1] = g;
   tmp[2] = b;
   tmp[3] = a;

   if (TEST_EQ_4V(tmp, ctx->Color.ClearColor.ui))
      return; /* no change */

   FLUSH_VERTICES(ctx, _NEW_COLOR);
   COPY_4V(ctx->Color.ClearColor.ui, tmp);

   /* these should be NOP calls for drivers supporting EXT_texture_integer */
   if (ctx->Driver.ClearColor) {
      ctx->Driver.ClearColor(ctx, ctx->Color.ClearColor);
   }
}


/**
 * Clear buffers.
 * 
 * \param mask bit-mask indicating the buffers to be cleared.
 *
 * Flushes the vertices and verifies the parameter. If __struct gl_contextRec::NewState
 * is set then calls _mesa_update_state() to update gl_frame_buffer::_Xmin,
 * etc. If the rasterization mode is set to GL_RENDER then requests the driver
 * to clear the buffers, via the dd_function_table::Clear callback.
 */ 
void GLAPIENTRY
_mesa_Clear( GLbitfield mask )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);

   FLUSH_CURRENT(ctx, 0);

   if (MESA_VERBOSE & VERBOSE_API)
      _mesa_debug(ctx, "glClear 0x%x\n", mask);

   if (mask & ~(GL_COLOR_BUFFER_BIT |
                GL_DEPTH_BUFFER_BIT |
                GL_STENCIL_BUFFER_BIT |
                GL_ACCUM_BUFFER_BIT)) {
      /* invalid bit set */
      _mesa_error( ctx, GL_INVALID_VALUE, "glClear(0x%x)", mask);
      return;
   }

   if (ctx->NewState) {
      _mesa_update_state( ctx );	/* update _Xmin, etc */
   }

   if (ctx->DrawBuffer->_Status != GL_FRAMEBUFFER_COMPLETE_EXT) {
      _mesa_error(ctx, GL_INVALID_FRAMEBUFFER_OPERATION_EXT,
                  "glClear(incomplete framebuffer)");
      return;
   }

   if (ctx->DrawBuffer->Width == 0 || ctx->DrawBuffer->Height == 0 ||
       ctx->DrawBuffer->_Xmin >= ctx->DrawBuffer->_Xmax ||
       ctx->DrawBuffer->_Ymin >= ctx->DrawBuffer->_Ymax)
      return;

   if (ctx->RasterDiscard)
      return;

   if (ctx->RenderMode == GL_RENDER) {
      GLbitfield bufferMask;

      /* don't clear depth buffer if depth writing disabled */
      if (!ctx->Depth.Mask)
         mask &= ~GL_DEPTH_BUFFER_BIT;

      /* Build the bitmask to send to device driver's Clear function.
       * Note that the GL_COLOR_BUFFER_BIT flag will expand to 0, 1, 2 or 4
       * of the BUFFER_BIT_FRONT/BACK_LEFT/RIGHT flags, or one of the
       * BUFFER_BIT_COLORn flags.
       */
      bufferMask = 0;
      if (mask & GL_COLOR_BUFFER_BIT) {
         GLuint i;
         for (i = 0; i < ctx->DrawBuffer->_NumColorDrawBuffers; i++) {
            bufferMask |= (1 << ctx->DrawBuffer->_ColorDrawBufferIndexes[i]);
         }
      }

      if ((mask & GL_DEPTH_BUFFER_BIT)
          && ctx->DrawBuffer->Visual.haveDepthBuffer) {
         bufferMask |= BUFFER_BIT_DEPTH;
      }

      if ((mask & GL_STENCIL_BUFFER_BIT)
          && ctx->DrawBuffer->Visual.haveStencilBuffer) {
         bufferMask |= BUFFER_BIT_STENCIL;
      }

      if ((mask & GL_ACCUM_BUFFER_BIT)
          && ctx->DrawBuffer->Visual.haveAccumBuffer) {
         bufferMask |= BUFFER_BIT_ACCUM;
      }

      ASSERT(ctx->Driver.Clear);
      ctx->Driver.Clear(ctx, bufferMask);
   }
}


/** Returned by make_color_buffer_mask() for errors */
#define INVALID_MASK ~0x0


/**
 * Convert the glClearBuffer 'drawbuffer' parameter into a bitmask of
 * BUFFER_BIT_x values.
 * Return INVALID_MASK if the drawbuffer value is invalid.
 */
static GLbitfield
make_color_buffer_mask(struct gl_context *ctx, GLint drawbuffer)
{
   const struct gl_renderbuffer_attachment *att = ctx->DrawBuffer->Attachment;
   GLbitfield mask = 0x0;

   switch (drawbuffer) {
   case GL_FRONT:
      if (att[BUFFER_FRONT_LEFT].Renderbuffer)
         mask |= BUFFER_BIT_FRONT_LEFT;
      if (att[BUFFER_FRONT_RIGHT].Renderbuffer)
         mask |= BUFFER_BIT_FRONT_RIGHT;
      break;
   case GL_BACK:
      if (att[BUFFER_BACK_LEFT].Renderbuffer)
         mask |= BUFFER_BIT_BACK_LEFT;
      if (att[BUFFER_BACK_RIGHT].Renderbuffer)
         mask |= BUFFER_BIT_BACK_RIGHT;
      break;
   case GL_LEFT:
      if (att[BUFFER_FRONT_LEFT].Renderbuffer)
         mask |= BUFFER_BIT_FRONT_LEFT;
      if (att[BUFFER_BACK_LEFT].Renderbuffer)
         mask |= BUFFER_BIT_BACK_LEFT;
      break;
   case GL_RIGHT:
      if (att[BUFFER_FRONT_RIGHT].Renderbuffer)
         mask |= BUFFER_BIT_FRONT_RIGHT;
      if (att[BUFFER_BACK_RIGHT].Renderbuffer)
         mask |= BUFFER_BIT_BACK_RIGHT;
      break;
   case GL_FRONT_AND_BACK:
      if (att[BUFFER_FRONT_LEFT].Renderbuffer)
         mask |= BUFFER_BIT_FRONT_LEFT;
      if (att[BUFFER_BACK_LEFT].Renderbuffer)
         mask |= BUFFER_BIT_BACK_LEFT;
      if (att[BUFFER_FRONT_RIGHT].Renderbuffer)
         mask |= BUFFER_BIT_FRONT_RIGHT;
      if (att[BUFFER_BACK_RIGHT].Renderbuffer)
         mask |= BUFFER_BIT_BACK_RIGHT;
      break;
   default:
      if (drawbuffer < 0 || drawbuffer >= (GLint)ctx->Const.MaxDrawBuffers) {
         mask = INVALID_MASK;
      }
      else if (att[BUFFER_COLOR0 + drawbuffer].Renderbuffer) {
         mask |= (BUFFER_BIT_COLOR0 << drawbuffer);
      }
   }

   return mask;
}



/**
 * New in GL 3.0
 * Clear signed integer color buffer or stencil buffer (not depth).
 */
void GLAPIENTRY
_mesa_ClearBufferiv(GLenum buffer, GLint drawbuffer, const GLint *value)
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);

   FLUSH_CURRENT(ctx, 0);

   if (ctx->NewState) {
      _mesa_update_state( ctx );
   }

   switch (buffer) {
   case GL_STENCIL:
      /* Page 264 (page 280 of the PDF) of the OpenGL 3.0 spec says:
       *
       *     "ClearBuffer generates an INVALID VALUE error if buffer is
       *     COLOR and drawbuffer is less than zero, or greater than the
       *     value of MAX DRAW BUFFERS minus one; or if buffer is DEPTH,
       *     STENCIL, or DEPTH STENCIL and drawbuffer is not zero."
       */
      if (drawbuffer != 0) {
         _mesa_error(ctx, GL_INVALID_VALUE, "glClearBufferiv(drawbuffer=%d)",
                     drawbuffer);
         return;
      }
      else if (ctx->DrawBuffer->Attachment[BUFFER_STENCIL].Renderbuffer && !ctx->RasterDiscard) {
         /* Save current stencil clear value, set to 'value', do the
          * stencil clear and restore the clear value.
          * XXX in the future we may have a new ctx->Driver.ClearBuffer()
          * hook instead.
          */
         const GLuint clearSave = ctx->Stencil.Clear;
         ctx->Stencil.Clear = *value;
         if (ctx->Driver.ClearStencil)
            ctx->Driver.ClearStencil(ctx, *value);
         ctx->Driver.Clear(ctx, BUFFER_BIT_STENCIL);
         ctx->Stencil.Clear = clearSave;
         if (ctx->Driver.ClearStencil)
            ctx->Driver.ClearStencil(ctx, clearSave);
      }
      break;
   case GL_COLOR:
      {
         const GLbitfield mask = make_color_buffer_mask(ctx, drawbuffer);
         if (mask == INVALID_MASK) {
            _mesa_error(ctx, GL_INVALID_VALUE, "glClearBufferiv(drawbuffer=%d)",
                        drawbuffer);
            return;
         }
         else if (mask && !ctx->RasterDiscard) {
            union gl_color_union clearSave;

            /* save color */
            clearSave = ctx->Color.ClearColor;
            /* set color */
            COPY_4V(ctx->Color.ClearColor.i, value);
            if (ctx->Driver.ClearColor)
               ctx->Driver.ClearColor(ctx, ctx->Color.ClearColor);
            /* clear buffer(s) */
            ctx->Driver.Clear(ctx, mask);
            /* restore color */
            ctx->Color.ClearColor = clearSave;
            if (ctx->Driver.ClearColor)
               ctx->Driver.ClearColor(ctx, clearSave);
         }
      }
      break;
   case GL_DEPTH:
      /* Page 264 (page 280 of the PDF) of the OpenGL 3.0 spec says:
       *
       *     "The result of ClearBuffer is undefined if no conversion between
       *     the type of the specified value and the type of the buffer being
       *     cleared is defined (for example, if ClearBufferiv is called for a
       *     fixed- or floating-point buffer, or if ClearBufferfv is called
       *     for a signed or unsigned integer buffer). This is not an error."
       *
       * In this case we take "undefined" and "not an error" to mean "ignore."
       * Note that we still need to generate an error for the invalid
       * drawbuffer case (see the GL_STENCIL case above).
       */
      if (drawbuffer != 0) {
         _mesa_error(ctx, GL_INVALID_VALUE, "glClearBufferiv(drawbuffer=%d)",
                     drawbuffer);
         return;
      }
      return;
   default:
      _mesa_error(ctx, GL_INVALID_ENUM, "glClearBufferiv(buffer=%s)",
                  _mesa_lookup_enum_by_nr(buffer));
      return;
   }
}


/**
 * New in GL 3.0
 * Clear unsigned integer color buffer (not depth, not stencil).
 */
void GLAPIENTRY
_mesa_ClearBufferuiv(GLenum buffer, GLint drawbuffer, const GLuint *value)
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);

   FLUSH_CURRENT(ctx, 0);

   if (ctx->NewState) {
      _mesa_update_state( ctx );
   }

   switch (buffer) {
   case GL_COLOR:
      {
         const GLbitfield mask = make_color_buffer_mask(ctx, drawbuffer);
         if (mask == INVALID_MASK) {
            _mesa_error(ctx, GL_INVALID_VALUE, "glClearBufferuiv(drawbuffer=%d)",
                        drawbuffer);
            return;
         }
         else if (mask && !ctx->RasterDiscard) {
            union gl_color_union clearSave;

            /* save color */
            clearSave = ctx->Color.ClearColor;
            /* set color */
            COPY_4V(ctx->Color.ClearColor.ui, value);
            if (ctx->Driver.ClearColor)
               ctx->Driver.ClearColor(ctx, ctx->Color.ClearColor);
            /* clear buffer(s) */
            ctx->Driver.Clear(ctx, mask);
            /* restore color */
            ctx->Color.ClearColor = clearSave;
            if (ctx->Driver.ClearColor)
               ctx->Driver.ClearColor(ctx, clearSave);
         }
      }
      break;
   case GL_DEPTH:
   case GL_STENCIL:
      /* Page 264 (page 280 of the PDF) of the OpenGL 3.0 spec says:
       *
       *     "The result of ClearBuffer is undefined if no conversion between
       *     the type of the specified value and the type of the buffer being
       *     cleared is defined (for example, if ClearBufferiv is called for a
       *     fixed- or floating-point buffer, or if ClearBufferfv is called
       *     for a signed or unsigned integer buffer). This is not an error."
       *
       * In this case we take "undefined" and "not an error" to mean "ignore."
       * Even though we could do something sensible for GL_STENCIL, page 263
       * (page 279 of the PDF) says:
       *
       *     "Only ClearBufferiv should be used to clear stencil buffers."
       *
       * Note that we still need to generate an error for the invalid
       * drawbuffer case (see the GL_STENCIL case in _mesa_ClearBufferiv).
       */
      if (drawbuffer != 0) {
         _mesa_error(ctx, GL_INVALID_VALUE, "glClearBufferuiv(drawbuffer=%d)",
                     drawbuffer);
         return;
      }
      return;
   default:
      _mesa_error(ctx, GL_INVALID_ENUM, "glClearBufferuiv(buffer=%s)",
                  _mesa_lookup_enum_by_nr(buffer));
      return;
   }
}


/**
 * New in GL 3.0
 * Clear fixed-pt or float color buffer or depth buffer (not stencil).
 */
void GLAPIENTRY
_mesa_ClearBufferfv(GLenum buffer, GLint drawbuffer, const GLfloat *value)
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);

   FLUSH_CURRENT(ctx, 0);

   if (ctx->NewState) {
      _mesa_update_state( ctx );
   }

   switch (buffer) {
   case GL_DEPTH:
      /* Page 264 (page 280 of the PDF) of the OpenGL 3.0 spec says:
       *
       *     "ClearBuffer generates an INVALID VALUE error if buffer is
       *     COLOR and drawbuffer is less than zero, or greater than the
       *     value of MAX DRAW BUFFERS minus one; or if buffer is DEPTH,
       *     STENCIL, or DEPTH STENCIL and drawbuffer is not zero."
       */
      if (drawbuffer != 0) {
         _mesa_error(ctx, GL_INVALID_VALUE, "glClearBufferfv(drawbuffer=%d)",
                     drawbuffer);
         return;
      }
      else if (ctx->DrawBuffer->Attachment[BUFFER_DEPTH].Renderbuffer && !ctx->RasterDiscard) {
         /* Save current depth clear value, set to 'value', do the
          * depth clear and restore the clear value.
          * XXX in the future we may have a new ctx->Driver.ClearBuffer()
          * hook instead.
          */
         const GLclampd clearSave = ctx->Depth.Clear;
         ctx->Depth.Clear = *value;
         if (ctx->Driver.ClearDepth)
            ctx->Driver.ClearDepth(ctx, *value);
         ctx->Driver.Clear(ctx, BUFFER_BIT_DEPTH);
         ctx->Depth.Clear = clearSave;
         if (ctx->Driver.ClearDepth)
            ctx->Driver.ClearDepth(ctx, clearSave);
      }
      /* clear depth buffer to value */
      break;
   case GL_COLOR:
      {
         const GLbitfield mask = make_color_buffer_mask(ctx, drawbuffer);
         if (mask == INVALID_MASK) {
            _mesa_error(ctx, GL_INVALID_VALUE, "glClearBufferfv(drawbuffer=%d)",
                        drawbuffer);
            return;
         }
         else if (mask && !ctx->RasterDiscard) {
            union gl_color_union clearSave;

            /* save color */
            clearSave = ctx->Color.ClearColor;
            /* set color */
            COPY_4V_CAST(ctx->Color.ClearColor.f, value, GLclampf);
            if (ctx->Driver.ClearColor)
               ctx->Driver.ClearColor(ctx, ctx->Color.ClearColor);
            /* clear buffer(s) */
            ctx->Driver.Clear(ctx, mask);
            /* restore color */
            ctx->Color.ClearColor = clearSave;
            if (ctx->Driver.ClearColor)
               ctx->Driver.ClearColor(ctx, clearSave);
         }
      }
      break;
   case GL_STENCIL:
      /* Page 264 (page 280 of the PDF) of the OpenGL 3.0 spec says:
       *
       *     "The result of ClearBuffer is undefined if no conversion between
       *     the type of the specified value and the type of the buffer being
       *     cleared is defined (for example, if ClearBufferiv is called for a
       *     fixed- or floating-point buffer, or if ClearBufferfv is called
       *     for a signed or unsigned integer buffer). This is not an error."
       *
       * In this case we take "undefined" and "not an error" to mean "ignore."
       * Note that we still need to generate an error for the invalid
       * drawbuffer case (see the GL_DEPTH case above).
       */
      if (drawbuffer != 0) {
         _mesa_error(ctx, GL_INVALID_VALUE, "glClearBufferfv(drawbuffer=%d)",
                     drawbuffer);
         return;
      }
      return;
   default:
      _mesa_error(ctx, GL_INVALID_ENUM, "glClearBufferfv(buffer=%s)",
                  _mesa_lookup_enum_by_nr(buffer));
      return;
   }
}


/**
 * New in GL 3.0
 * Clear depth/stencil buffer only.
 */
void GLAPIENTRY
_mesa_ClearBufferfi(GLenum buffer, GLint drawbuffer,
                    GLfloat depth, GLint stencil)
{
   GET_CURRENT_CONTEXT(ctx);
   GLbitfield mask = 0;

   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);

   FLUSH_CURRENT(ctx, 0);

   if (buffer != GL_DEPTH_STENCIL) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glClearBufferfi(buffer=%s)",
                  _mesa_lookup_enum_by_nr(buffer));
      return;
   }

   /* Page 264 (page 280 of the PDF) of the OpenGL 3.0 spec says:
    *
    *     "ClearBuffer generates an INVALID VALUE error if buffer is
    *     COLOR and drawbuffer is less than zero, or greater than the
    *     value of MAX DRAW BUFFERS minus one; or if buffer is DEPTH,
    *     STENCIL, or DEPTH STENCIL and drawbuffer is not zero."
    */
   if (drawbuffer != 0) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glClearBufferfi(drawbuffer=%d)",
                  drawbuffer);
      return;
   }

   if (ctx->RasterDiscard)
      return;

   if (ctx->NewState) {
      _mesa_update_state( ctx );
   }

   if (ctx->DrawBuffer->Attachment[BUFFER_DEPTH].Renderbuffer)
      mask |= BUFFER_BIT_DEPTH;
   if (ctx->DrawBuffer->Attachment[BUFFER_STENCIL].Renderbuffer)
      mask |= BUFFER_BIT_STENCIL;

   if (mask) {
      /* save current clear values */
      const GLclampd clearDepthSave = ctx->Depth.Clear;
      const GLuint clearStencilSave = ctx->Stencil.Clear;

      /* set new clear values */
      ctx->Depth.Clear = depth;
      ctx->Stencil.Clear = stencil;
      if (ctx->Driver.ClearDepth)
         ctx->Driver.ClearDepth(ctx, depth);
      if (ctx->Driver.ClearStencil)
         ctx->Driver.ClearStencil(ctx, stencil);

      /* clear buffers */
      ctx->Driver.Clear(ctx, mask);

      /* restore */
      ctx->Depth.Clear = clearDepthSave;
      ctx->Stencil.Clear = clearStencilSave;
      if (ctx->Driver.ClearDepth)
         ctx->Driver.ClearDepth(ctx, clearDepthSave);
      if (ctx->Driver.ClearStencil)
         ctx->Driver.ClearStencil(ctx, clearStencilSave);
   }
}
