/*
 * Mesa 3-D graphics library
 * Version:  6.4
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


/**
 * Functions for allocating/managing framebuffers and renderbuffers.
 * Also, routines for reading/writing renderbuffer data as ubytes,
 * ushorts, uints, etc.
 */


#include "glheader.h"
#include "imports.h"
#include "context.h"
#include "mtypes.h"
#include "fbobject.h"
#include "framebuffer.h"
#include "renderbuffer.h"



/**
 * Compute/set the _DepthMax field for the given framebuffer.
 * This value depends on the Z buffer resolution.
 */
static void
compute_depth_max(struct gl_framebuffer *fb)
{
   if (fb->Visual.depthBits == 0) {
      /* Special case.  Even if we don't have a depth buffer we need
       * good values for DepthMax for Z vertex transformation purposes
       * and for per-fragment fog computation.
       */
      fb->_DepthMax = (1 << 16) - 1;
   }
   else if (fb->Visual.depthBits < 32) {
      fb->_DepthMax = (1 << fb->Visual.depthBits) - 1;
   }
   else {
      /* Special case since shift values greater than or equal to the
       * number of bits in the left hand expression's type are undefined.
       */
      fb->_DepthMax = 0xffffffff;
   }
   fb->_DepthMaxF = (GLfloat) fb->_DepthMax;
   fb->_MRD = 1.0;  /* Minimum resolvable depth value, for polygon offset */
}


/**
 * Create and initialize a gl_framebuffer object.
 * This is intended for creating _window_system_ framebuffers, not generic
 * framebuffer objects ala GL_EXT_framebuffer_object.
 *
 * \sa _mesa_new_framebuffer
 */
struct gl_framebuffer *
_mesa_create_framebuffer(const GLvisual *visual)
{
   struct gl_framebuffer *fb = CALLOC_STRUCT(gl_framebuffer);
   assert(visual);
   if (fb) {
      _mesa_initialize_framebuffer(fb, visual);
   }
   return fb;
}


/**
 * Allocate a new gl_framebuffer object.
 * This is the default function for ctx->Driver.NewFramebuffer().
 * This is for allocating user-created framebuffers, not window-system
 * framebuffers!
 * \sa _mesa_create_framebuffer
 */
struct gl_framebuffer *
_mesa_new_framebuffer(GLcontext *ctx, GLuint name)
{
   struct gl_framebuffer *fb;
   assert(name != 0);
   fb = CALLOC_STRUCT(gl_framebuffer);
   if (fb) {
      fb->Name = name;
      fb->RefCount = 1;
      fb->Delete = _mesa_destroy_framebuffer;
      fb->ColorDrawBuffer[0] = GL_COLOR_ATTACHMENT0_EXT;
      fb->_ColorDrawBufferMask[0] = BUFFER_BIT_COLOR0;
      fb->ColorReadBuffer = GL_COLOR_ATTACHMENT0_EXT;
      fb->_ColorReadBufferMask = BUFFER_BIT_COLOR0;
      fb->Delete = _mesa_destroy_framebuffer;
   }
   return fb;
}


/**
 * Initialize a gl_framebuffer object.
 * \sa _mesa_create_framebuffer
 */
void
_mesa_initialize_framebuffer(struct gl_framebuffer *fb, const GLvisual *visual)
{
   assert(fb);
   assert(visual);

   _mesa_bzero(fb, sizeof(struct gl_framebuffer));

   /* save the visual */
   fb->Visual = *visual;

   /* Init glRead/DrawBuffer state */
   if (visual->doubleBufferMode) {
      fb->ColorDrawBuffer[0] = GL_BACK;
      fb->_ColorDrawBufferMask[0] = BUFFER_BIT_BACK_LEFT;
      fb->ColorReadBuffer = GL_BACK;
      fb->_ColorReadBufferMask = BUFFER_BIT_BACK_LEFT;
   }
   else {
      fb->ColorDrawBuffer[0] = GL_FRONT;
      fb->_ColorDrawBufferMask[0] = BUFFER_BIT_FRONT_LEFT;
      fb->ColorReadBuffer = GL_FRONT;
      fb->_ColorReadBufferMask = BUFFER_BIT_FRONT_LEFT;
   }

   fb->Delete = _mesa_destroy_framebuffer;
   fb->_Status = GL_FRAMEBUFFER_COMPLETE_EXT;

   compute_depth_max(fb);
}


/**
 * Create/attach software-based renderbuffers to the given framebuffer.
 * This is a helper routine for device drivers.  Drivers can just as well
 * call the individual _mesa_add_*_renderbuffer() routines directly.
 */
void
_mesa_add_soft_renderbuffers(struct gl_framebuffer *fb,
                             GLboolean color,
                             GLboolean depth,
                             GLboolean stencil,
                             GLboolean accum,
                             GLboolean alpha,
                             GLboolean aux)
{
   GLboolean frontLeft = GL_TRUE;
   GLboolean backLeft = fb->Visual.doubleBufferMode;
   GLboolean frontRight = fb->Visual.stereoMode;
   GLboolean backRight = fb->Visual.stereoMode && fb->Visual.doubleBufferMode;

   if (color) {
      if (fb->Visual.rgbMode) {
         assert(fb->Visual.redBits == fb->Visual.greenBits);
         assert(fb->Visual.redBits == fb->Visual.blueBits);
         _mesa_add_color_renderbuffers(NULL, fb,
                                       fb->Visual.redBits,
                                       fb->Visual.alphaBits,
                                       frontLeft, backLeft,
                                       frontRight, backRight);
      }
      else {
         _mesa_add_color_index_renderbuffers(NULL, fb,
                                             fb->Visual.indexBits,
                                             frontLeft, backLeft,
                                             frontRight, backRight);
      }
   }

   if (depth) {
      assert(fb->Visual.depthBits > 0);
      _mesa_add_depth_renderbuffer(NULL, fb, fb->Visual.depthBits);
   }

   if (stencil) {
      assert(fb->Visual.stencilBits > 0);
      _mesa_add_stencil_renderbuffer(NULL, fb, fb->Visual.stencilBits);
   }

   if (accum) {
      assert(fb->Visual.rgbMode);
      assert(fb->Visual.accumRedBits > 0);
      assert(fb->Visual.accumGreenBits > 0);
      assert(fb->Visual.accumBlueBits > 0);
      _mesa_add_accum_renderbuffer(NULL, fb,
                                   fb->Visual.accumRedBits,
                                   fb->Visual.accumGreenBits,
                                   fb->Visual.accumBlueBits,
                                   fb->Visual.accumAlphaBits);
   }

   if (aux) {
      assert(fb->Visual.rgbMode);
      assert(fb->Visual.numAuxBuffers > 0);
      _mesa_add_aux_renderbuffers(NULL, fb, fb->Visual.redBits,
                                  fb->Visual.numAuxBuffers);
   }

#if 1
   if (alpha) {
      assert(fb->Visual.rgbMode);
      assert(fb->Visual.alphaBits > 0);
      _mesa_add_alpha_renderbuffers(NULL, fb, fb->Visual.alphaBits,
                                    frontLeft, backLeft,
                                    frontRight, backRight);
   }
#endif

#if 0
   if (multisample) {
      /* maybe someday */
   }
#endif
}


/**
 * Deallocate buffer and everything attached to it.
 */
void
_mesa_destroy_framebuffer(struct gl_framebuffer *buffer)
{
   if (buffer) {
      _mesa_free_framebuffer_data(buffer);
      FREE(buffer);
   }
}


/**
 * Free all the data hanging off the given gl_framebuffer, but don't free
 * the gl_framebuffer object itself.
 */
void
_mesa_free_framebuffer_data(struct gl_framebuffer *fb)
{
   GLuint i;

   assert(fb);

   for (i = 0; i < BUFFER_COUNT; i++) {
      struct gl_renderbuffer_attachment *att = &fb->Attachment[i];
      if (att->Type == GL_RENDERBUFFER_EXT && att->Renderbuffer) {
         struct gl_renderbuffer *rb = att->Renderbuffer;
         rb->RefCount--;
         if (rb->RefCount == 0) {
            rb->Delete(rb);
         }
      }
      att->Type = GL_NONE;
      att->Renderbuffer = NULL;
   }
}


/**
 * Resize the given framebuffer's renderbuffers to the new width and height.
 * This should only be used for window-system framebuffers, not
 * user-created renderbuffers (i.e. made with GL_EXT_framebuffer_object).
 * This will typically be called via ctx->Driver.ResizeBuffers()
 */
void
_mesa_resize_framebuffer(GLcontext *ctx, struct gl_framebuffer *fb,
                         GLuint width, GLuint height)
{
   GLuint i;

   /* For window system framebuffers, Name is zero */
   assert(fb->Name == 0);

   for (i = 0; i < BUFFER_COUNT; i++) {
      struct gl_renderbuffer_attachment *att = &fb->Attachment[i];
      if (att->Type == GL_RENDERBUFFER_EXT && att->Renderbuffer) {
         struct gl_renderbuffer *rb = att->Renderbuffer;
         /* only resize if size is changing */
         if (rb->Width != width || rb->Height != height) {
            if (rb->AllocStorage(ctx, rb, rb->InternalFormat, width, height)) {
               rb->Width = width;
               rb->Height = height;
            }
            else {
               _mesa_error(ctx, GL_OUT_OF_MEMORY, "Resizing framebuffer");
            }
         }
      }
   }

   fb->Width = width;
   fb->Height = height;

   /* to update scissor / window bounds */
   ctx->NewState |= _NEW_BUFFERS;
}


/**
 * Examine all the framebuffer's renderbuffers to update the Width/Height
 * fields of the framebuffer.  If we have renderbuffers with different
 * sizes, set the framebuffer's width and height to zero.
 * Note: this is only intended for user-created framebuffers, not
 * window-system framebuffes.
 */
static void
update_framebuffer_size(struct gl_framebuffer *fb)
{
   GLboolean haveSize = GL_FALSE;
   GLuint i;

   /* user-created framebuffers only */
   assert(fb->Name);

   for (i = 0; i < BUFFER_COUNT; i++) {
      struct gl_renderbuffer_attachment *att = &fb->Attachment[i];
      const struct gl_renderbuffer *rb = att->Renderbuffer;
      if (rb) {
         if (haveSize) {
            if (rb->Width != fb->Width && rb->Height != fb->Height) {
               /* size mismatch! */
               fb->Width = 0;
               fb->Height = 0;
               return;
            }
         }
         else {
            fb->Width = rb->Width;
            fb->Height = rb->Height;
            haveSize = GL_TRUE;
         }
      }
   }
}


/**
 * Update the context's current drawing buffer's Xmin, Xmax, Ymin, Ymax fields.
 * These values are computed from the buffer's width and height and
 * the scissor box, if it's enabled.
 * \param ctx  the GL context.
 */
void
_mesa_update_draw_buffer_bounds(GLcontext *ctx)
{
   struct gl_framebuffer *buffer = ctx->DrawBuffer;

   if (buffer->Name) {
      /* user-created framebuffer size depends on the renderbuffers */
      update_framebuffer_size(buffer);
   }

   buffer->_Xmin = 0;
   buffer->_Ymin = 0;
   buffer->_Xmax = buffer->Width;
   buffer->_Ymax = buffer->Height;

   if (ctx->Scissor.Enabled) {
      if (ctx->Scissor.X > buffer->_Xmin) {
	 buffer->_Xmin = ctx->Scissor.X;
      }
      if (ctx->Scissor.Y > buffer->_Ymin) {
	 buffer->_Ymin = ctx->Scissor.Y;
      }
      if (ctx->Scissor.X + ctx->Scissor.Width < buffer->_Xmax) {
	 buffer->_Xmax = ctx->Scissor.X + ctx->Scissor.Width;
      }
      if (ctx->Scissor.Y + ctx->Scissor.Height < buffer->_Ymax) {
	 buffer->_Ymax = ctx->Scissor.Y + ctx->Scissor.Height;
      }
      /* finally, check for empty region */
      if (buffer->_Xmin > buffer->_Xmax) {
         buffer->_Xmin = buffer->_Xmax;
      }
      if (buffer->_Ymin > buffer->_Ymax) {
         buffer->_Ymin = buffer->_Ymax;
      }
   }

   ASSERT(buffer->_Xmin <= buffer->_Xmax);
   ASSERT(buffer->_Ymin <= buffer->_Ymax);
}


/**
 * The glGet queries of the framebuffer red/green/blue size, stencil size,
 * etc. are satisfied by the fields of ctx->DrawBuffer->Visual.  These can
 * change depending on the renderbuffer bindings.  This function update's
 * the given framebuffer's Visual from the current renderbuffer bindings.
 * This is only intended for user-created framebuffers.
 */
void
_mesa_update_framebuffer_visual(struct gl_framebuffer *fb)
{
   GLuint i;

   assert(fb->Name != 0);

   _mesa_bzero(&fb->Visual, sizeof(fb->Visual));
   fb->Visual.rgbMode = GL_TRUE;

   /* find first RGB or CI renderbuffer */
   for (i = 0; i < BUFFER_COUNT; i++) {
      if (fb->Attachment[i].Renderbuffer) {
         const struct gl_renderbuffer *rb = fb->Attachment[i].Renderbuffer;
         if (rb->_BaseFormat == GL_RGBA || rb->_BaseFormat == GL_RGB) {
            fb->Visual.redBits = rb->ComponentSizes[0];
            fb->Visual.greenBits = rb->ComponentSizes[1];
            fb->Visual.blueBits = rb->ComponentSizes[2];
            fb->Visual.alphaBits = rb->ComponentSizes[3];
            fb->Visual.floatMode = GL_FALSE;
            break;
         }
         else if (rb->_BaseFormat == GL_COLOR_INDEX) {
            fb->Visual.indexBits = rb->ComponentSizes[0];
            fb->Visual.rgbMode = GL_FALSE;
            break;
         }
      }
   }

   if (fb->Attachment[BUFFER_DEPTH].Renderbuffer) {
      fb->Visual.haveDepthBuffer = GL_TRUE;
      fb->Visual.depthBits
         = fb->Attachment[BUFFER_DEPTH].Renderbuffer->ComponentSizes[0];
   }

   if (fb->Attachment[BUFFER_STENCIL].Renderbuffer) {
      fb->Visual.haveStencilBuffer = GL_TRUE;
      fb->Visual.stencilBits
         = fb->Attachment[BUFFER_STENCIL].Renderbuffer->ComponentSizes[0];
   }

   compute_depth_max(fb);
}


/**
 * Given a framebuffer and a buffer bit (like BUFFER_BIT_FRONT_LEFT), return
 * the corresponding renderbuffer.
 */
static struct gl_renderbuffer *
get_renderbuffer(struct gl_framebuffer *fb, GLuint bufferBit)
{
   GLuint index;
   for (index = 0; index < BUFFER_COUNT; index++) {
      if ((1 << index) == bufferBit) {
         return fb->Attachment[index].Renderbuffer;
      }
   }
   _mesa_problem(NULL, "Bad bufferBit in get_renderbuffer");
   return NULL;
}


/**
 * Update state related to the current draw/read framebuffers.
 * If the current framebuffer is user-created, make sure it's complete.
 */
void
_mesa_update_framebuffer(GLcontext *ctx)
{
   struct gl_framebuffer *fb = ctx->DrawBuffer;
   GLuint output;

   /* Completeness only matters for user-created framebuffers */
   if (fb->Name != 0) {
      _mesa_test_framebuffer_completeness(ctx, fb);
      _mesa_update_framebuffer_visual(fb);
   }

   /*
    * Update the list of drawing renderbuffer pointers.
    * Later, when we're rendering we'll loop from 0 to _NumColorDrawBuffers
    * writing colors.  We have a loop because glDrawBuffer(GL_FRONT_AND_BACK)
    * can specify writing to two or four color buffers.
    */
   for (output = 0; output < ctx->Const.MaxDrawBuffers; output++) {
      GLuint bufferMask = fb->_ColorDrawBufferMask[output];
      GLuint count = 0;
      GLuint bufferBit;
      /* for each bit that's set in the bufferMask... */
      for (bufferBit = 1; bufferMask; bufferBit <<= 1) {
         if (bufferBit & bufferMask) {
            struct gl_renderbuffer *rb = get_renderbuffer(fb, bufferBit);
            if (rb) {
               fb->_ColorDrawBuffers[output][count] = rb;
               fb->_ColorDrawBit[output][count] = bufferBit;
               count++;
            }
            else {
               /*_mesa_warning(ctx, "DrawBuffer names a missing buffer!");*/
            }
            bufferMask &= ~bufferBit;
         }
      }
      fb->_NumColorDrawBuffers[output] = count;
   }

   /*
    * Update the read renderbuffer pointer.
    * Unlike the DrawBuffer, we can only read from one (or zero) color buffers.
    */
   if (fb->_ColorReadBufferMask == 0x0)
      fb->_ColorReadBuffer = NULL; /* legal! */
   else
      fb->_ColorReadBuffer = get_renderbuffer(fb, fb->_ColorReadBufferMask);

   compute_depth_max(fb);
}
