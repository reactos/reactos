/*
 * Mesa 3-D graphics library
 * Version:  7.2
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


/**
 * Functions for allocating/managing framebuffers and renderbuffers.
 * Also, routines for reading/writing renderbuffer data as ubytes,
 * ushorts, uints, etc.
 */


#include "glheader.h"
#include "imports.h"
#include "buffers.h"
#include "context.h"
#include "enums.h"
#include "formats.h"
#include "macros.h"
#include "mtypes.h"
#include "fbobject.h"
#include "framebuffer.h"
#include "renderbuffer.h"
#include "texobj.h"



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

   /* Minimum resolvable depth value, for polygon offset */
   fb->_MRD = (GLfloat)1.0 / fb->_DepthMaxF;
}

/**
 * Create and initialize a gl_framebuffer object.
 * This is intended for creating _window_system_ framebuffers, not generic
 * framebuffer objects ala GL_EXT_framebuffer_object.
 *
 * \sa _mesa_new_framebuffer
 */
struct gl_framebuffer *
_mesa_create_framebuffer(const struct gl_config *visual)
{
   struct gl_framebuffer *fb = CALLOC_STRUCT(gl_framebuffer);
   assert(visual);
   if (fb) {
      _mesa_initialize_window_framebuffer(fb, visual);
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
_mesa_new_framebuffer(struct gl_context *ctx, GLuint name)
{
   struct gl_framebuffer *fb;
   (void) ctx;
   assert(name != 0);
   fb = CALLOC_STRUCT(gl_framebuffer);
   if (fb) {
      _mesa_initialize_user_framebuffer(fb, name);
   }
   return fb;
}


/**
 * Initialize a gl_framebuffer object.  Typically used to initialize
 * window system-created framebuffers, not user-created framebuffers.
 * \sa _mesa_initialize_user_framebuffer
 */
void
_mesa_initialize_window_framebuffer(struct gl_framebuffer *fb,
				     const struct gl_config *visual)
{
   assert(fb);
   assert(visual);

   memset(fb, 0, sizeof(struct gl_framebuffer));

   _glthread_INIT_MUTEX(fb->Mutex);

   fb->RefCount = 1;

   /* save the visual */
   fb->Visual = *visual;

   /* Init read/draw renderbuffer state */
   if (visual->doubleBufferMode) {
      fb->_NumColorDrawBuffers = 1;
      fb->ColorDrawBuffer[0] = GL_BACK;
      fb->_ColorDrawBufferIndexes[0] = BUFFER_BACK_LEFT;
      fb->ColorReadBuffer = GL_BACK;
      fb->_ColorReadBufferIndex = BUFFER_BACK_LEFT;
   }
   else {
      fb->_NumColorDrawBuffers = 1;
      fb->ColorDrawBuffer[0] = GL_FRONT;
      fb->_ColorDrawBufferIndexes[0] = BUFFER_FRONT_LEFT;
      fb->ColorReadBuffer = GL_FRONT;
      fb->_ColorReadBufferIndex = BUFFER_FRONT_LEFT;
   }

   fb->Delete = _mesa_destroy_framebuffer;
   fb->_Status = GL_FRAMEBUFFER_COMPLETE_EXT;

   compute_depth_max(fb);
}


/**
 * Initialize a user-created gl_framebuffer object.
 * \sa _mesa_initialize_window_framebuffer
 */
void
_mesa_initialize_user_framebuffer(struct gl_framebuffer *fb, GLuint name)
{
   assert(fb);
   assert(name);

   memset(fb, 0, sizeof(struct gl_framebuffer));

   fb->Name = name;
   fb->RefCount = 1;
   fb->_NumColorDrawBuffers = 1;
   fb->ColorDrawBuffer[0] = GL_COLOR_ATTACHMENT0_EXT;
   fb->_ColorDrawBufferIndexes[0] = BUFFER_COLOR0;
   fb->ColorReadBuffer = GL_COLOR_ATTACHMENT0_EXT;
   fb->_ColorReadBufferIndex = BUFFER_COLOR0;
   fb->Delete = _mesa_destroy_framebuffer;
   _glthread_INIT_MUTEX(fb->Mutex);
}


/**
 * Deallocate buffer and everything attached to it.
 * Typically called via the gl_framebuffer->Delete() method.
 */
void
_mesa_destroy_framebuffer(struct gl_framebuffer *fb)
{
   if (fb) {
      _mesa_free_framebuffer_data(fb);
      free(fb);
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
   assert(fb->RefCount == 0);

   _glthread_DESTROY_MUTEX(fb->Mutex);

   for (i = 0; i < BUFFER_COUNT; i++) {
      struct gl_renderbuffer_attachment *att = &fb->Attachment[i];
      if (att->Renderbuffer) {
         _mesa_reference_renderbuffer(&att->Renderbuffer, NULL);
      }
      if (att->Texture) {
         _mesa_reference_texobj(&att->Texture, NULL);
      }
      ASSERT(!att->Renderbuffer);
      ASSERT(!att->Texture);
      att->Type = GL_NONE;
   }
}


/**
 * Set *ptr to point to fb, with refcounting and locking.
 * This is normally only called from the _mesa_reference_framebuffer() macro
 * when there's a real pointer change.
 */
void
_mesa_reference_framebuffer_(struct gl_framebuffer **ptr,
                             struct gl_framebuffer *fb)
{
   if (*ptr) {
      /* unreference old renderbuffer */
      GLboolean deleteFlag = GL_FALSE;
      struct gl_framebuffer *oldFb = *ptr;

      _glthread_LOCK_MUTEX(oldFb->Mutex);
      ASSERT(oldFb->RefCount > 0);
      oldFb->RefCount--;
      deleteFlag = (oldFb->RefCount == 0);
      _glthread_UNLOCK_MUTEX(oldFb->Mutex);
      
      if (deleteFlag)
         oldFb->Delete(oldFb);

      *ptr = NULL;
   }
   assert(!*ptr);

   if (fb) {
      _glthread_LOCK_MUTEX(fb->Mutex);
      fb->RefCount++;
      _glthread_UNLOCK_MUTEX(fb->Mutex);
      *ptr = fb;
   }
}


/**
 * Resize the given framebuffer's renderbuffers to the new width and height.
 * This should only be used for window-system framebuffers, not
 * user-created renderbuffers (i.e. made with GL_EXT_framebuffer_object).
 * This will typically be called via ctx->Driver.ResizeBuffers() or directly
 * from a device driver.
 *
 * \note it's possible for ctx to be null since a window can be resized
 * without a currently bound rendering context.
 */
void
_mesa_resize_framebuffer(struct gl_context *ctx, struct gl_framebuffer *fb,
                         GLuint width, GLuint height)
{
   GLuint i;

   /* XXX I think we could check if the size is not changing
    * and return early.
    */

   /* For window system framebuffers, Name is zero */
   assert(fb->Name == 0);

   for (i = 0; i < BUFFER_COUNT; i++) {
      struct gl_renderbuffer_attachment *att = &fb->Attachment[i];
      if (att->Type == GL_RENDERBUFFER_EXT && att->Renderbuffer) {
         struct gl_renderbuffer *rb = att->Renderbuffer;
         /* only resize if size is changing */
         if (rb->Width != width || rb->Height != height) {
            if (rb->AllocStorage(ctx, rb, rb->InternalFormat, width, height)) {
               ASSERT(rb->Width == width);
               ASSERT(rb->Height == height);
            }
            else {
               _mesa_error(ctx, GL_OUT_OF_MEMORY, "Resizing framebuffer");
               /* no return */
            }
         }
      }
   }

   fb->Width = width;
   fb->Height = height;

   if (ctx) {
      /* update scissor / window bounds */
      _mesa_update_draw_buffer_bounds(ctx);
      /* Signal new buffer state so that swrast will update its clipping
       * info (the CLIP_BIT flag).
       */
      ctx->NewState |= _NEW_BUFFERS;
   }
}



/**
 * XXX THIS IS OBSOLETE - drivers should take care of detecting window
 * size changes and act accordingly, likely calling _mesa_resize_framebuffer().
 *
 * GL_MESA_resize_buffers extension.
 *
 * When this function is called, we'll ask the window system how large
 * the current window is.  If it's a new size, we'll call the driver's
 * ResizeBuffers function.  The driver will then resize its color buffers
 * as needed, and maybe call the swrast's routine for reallocating
 * swrast-managed depth/stencil/accum/etc buffers.
 * \note This function should only be called through the GL API, not
 * from device drivers (as was done in the past).
 */
void
_mesa_resizebuffers( struct gl_context *ctx )
{
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH( ctx );

   if (MESA_VERBOSE & VERBOSE_API)
      _mesa_debug(ctx, "glResizeBuffersMESA\n");

   if (!ctx->Driver.GetBufferSize) {
      return;
   }

   if (ctx->WinSysDrawBuffer) {
      GLuint newWidth, newHeight;
      struct gl_framebuffer *buffer = ctx->WinSysDrawBuffer;

      assert(buffer->Name == 0);

      /* ask device driver for size of output buffer */
      ctx->Driver.GetBufferSize( buffer, &newWidth, &newHeight );

      /* see if size of device driver's color buffer (window) has changed */
      if (buffer->Width != newWidth || buffer->Height != newHeight) {
         if (ctx->Driver.ResizeBuffers)
            ctx->Driver.ResizeBuffers(ctx, buffer, newWidth, newHeight );
      }
   }

   if (ctx->WinSysReadBuffer
       && ctx->WinSysReadBuffer != ctx->WinSysDrawBuffer) {
      GLuint newWidth, newHeight;
      struct gl_framebuffer *buffer = ctx->WinSysReadBuffer;

      assert(buffer->Name == 0);

      /* ask device driver for size of read buffer */
      ctx->Driver.GetBufferSize( buffer, &newWidth, &newHeight );

      /* see if size of device driver's color buffer (window) has changed */
      if (buffer->Width != newWidth || buffer->Height != newHeight) {
         if (ctx->Driver.ResizeBuffers)
            ctx->Driver.ResizeBuffers(ctx, buffer, newWidth, newHeight );
      }
   }

   ctx->NewState |= _NEW_BUFFERS;  /* to update scissor / window bounds */
}


/*
 * XXX THIS IS OBSOLETE
 */
void GLAPIENTRY
_mesa_ResizeBuffersMESA( void )
{
   GET_CURRENT_CONTEXT(ctx);

   if (ctx->Extensions.MESA_resize_buffers)
      _mesa_resizebuffers( ctx );
}



/**
 * Examine all the framebuffer's renderbuffers to update the Width/Height
 * fields of the framebuffer.  If we have renderbuffers with different
 * sizes, set the framebuffer's width and height to the min size.
 * Note: this is only intended for user-created framebuffers, not
 * window-system framebuffes.
 */
static void
update_framebuffer_size(struct gl_context *ctx, struct gl_framebuffer *fb)
{
   GLuint minWidth = ~0, minHeight = ~0;
   GLuint i;

   /* user-created framebuffers only */
   assert(fb->Name);

   for (i = 0; i < BUFFER_COUNT; i++) {
      struct gl_renderbuffer_attachment *att = &fb->Attachment[i];
      const struct gl_renderbuffer *rb = att->Renderbuffer;
      if (rb) {
         minWidth = MIN2(minWidth, rb->Width);
         minHeight = MIN2(minHeight, rb->Height);
      }
   }

   if (minWidth != ~0) {
      fb->Width = minWidth;
      fb->Height = minHeight;
   }
   else {
      fb->Width = 0;
      fb->Height = 0;
   }
}


/**
 * Update the context's current drawing buffer's Xmin, Xmax, Ymin, Ymax fields.
 * These values are computed from the buffer's width and height and
 * the scissor box, if it's enabled.
 * \param ctx  the GL context.
 */
void
_mesa_update_draw_buffer_bounds(struct gl_context *ctx)
{
   struct gl_framebuffer *buffer = ctx->DrawBuffer;

   if (!buffer)
      return;

   if (buffer->Name) {
      /* user-created framebuffer size depends on the renderbuffers */
      update_framebuffer_size(ctx, buffer);
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
 * change depending on the renderbuffer bindings.  This function updates
 * the given framebuffer's Visual from the current renderbuffer bindings.
 *
 * This may apply to user-created framebuffers or window system framebuffers.
 *
 * Also note: ctx->DrawBuffer->Visual.depthBits might not equal
 * ctx->DrawBuffer->Attachment[BUFFER_DEPTH].Renderbuffer.DepthBits.
 * The former one is used to convert floating point depth values into
 * integer Z values.
 */
void
_mesa_update_framebuffer_visual(struct gl_context *ctx,
				struct gl_framebuffer *fb)
{
   GLuint i;

   memset(&fb->Visual, 0, sizeof(fb->Visual));
   fb->Visual.rgbMode = GL_TRUE; /* assume this */

#if 0 /* this _might_ be needed */
   if (fb->_Status != GL_FRAMEBUFFER_COMPLETE_EXT) {
      /* leave visual fields zero'd */
      return;
   }
#endif

   /* find first RGB renderbuffer */
   for (i = 0; i < BUFFER_COUNT; i++) {
      if (fb->Attachment[i].Renderbuffer) {
         const struct gl_renderbuffer *rb = fb->Attachment[i].Renderbuffer;
         const GLenum baseFormat = _mesa_get_format_base_format(rb->Format);
         const gl_format fmt = rb->Format;

         if (_mesa_is_legal_color_format(ctx, baseFormat)) {
            fb->Visual.redBits = _mesa_get_format_bits(fmt, GL_RED_BITS);
            fb->Visual.greenBits = _mesa_get_format_bits(fmt, GL_GREEN_BITS);
            fb->Visual.blueBits = _mesa_get_format_bits(fmt, GL_BLUE_BITS);
            fb->Visual.alphaBits = _mesa_get_format_bits(fmt, GL_ALPHA_BITS);
            fb->Visual.rgbBits = fb->Visual.redBits
               + fb->Visual.greenBits + fb->Visual.blueBits;
            fb->Visual.samples = rb->NumSamples;
            fb->Visual.sampleBuffers = rb->NumSamples > 0 ? 1 : 0;
            if (_mesa_get_format_color_encoding(fmt) == GL_SRGB)
                fb->Visual.sRGBCapable = ctx->Const.sRGBCapable;
            break;
         }
      }
   }

   fb->Visual.floatMode = GL_FALSE;
   for (i = 0; i < BUFFER_COUNT; i++) {
      if (fb->Attachment[i].Renderbuffer) {
         const struct gl_renderbuffer *rb = fb->Attachment[i].Renderbuffer;
         const gl_format fmt = rb->Format;

         if (_mesa_get_format_datatype(fmt) == GL_FLOAT) {
            fb->Visual.floatMode = GL_TRUE;
            break;
         }
      }
   }

   if (fb->Attachment[BUFFER_DEPTH].Renderbuffer) {
      const struct gl_renderbuffer *rb =
         fb->Attachment[BUFFER_DEPTH].Renderbuffer;
      const gl_format fmt = rb->Format;
      fb->Visual.haveDepthBuffer = GL_TRUE;
      fb->Visual.depthBits = _mesa_get_format_bits(fmt, GL_DEPTH_BITS);
   }

   if (fb->Attachment[BUFFER_STENCIL].Renderbuffer) {
      const struct gl_renderbuffer *rb =
         fb->Attachment[BUFFER_STENCIL].Renderbuffer;
      const gl_format fmt = rb->Format;
      fb->Visual.haveStencilBuffer = GL_TRUE;
      fb->Visual.stencilBits = _mesa_get_format_bits(fmt, GL_STENCIL_BITS);
   }

   if (fb->Attachment[BUFFER_ACCUM].Renderbuffer) {
      const struct gl_renderbuffer *rb =
         fb->Attachment[BUFFER_ACCUM].Renderbuffer;
      const gl_format fmt = rb->Format;
      fb->Visual.haveAccumBuffer = GL_TRUE;
      fb->Visual.accumRedBits = _mesa_get_format_bits(fmt, GL_RED_BITS);
      fb->Visual.accumGreenBits = _mesa_get_format_bits(fmt, GL_GREEN_BITS);
      fb->Visual.accumBlueBits = _mesa_get_format_bits(fmt, GL_BLUE_BITS);
      fb->Visual.accumAlphaBits = _mesa_get_format_bits(fmt, GL_ALPHA_BITS);
   }

   compute_depth_max(fb);
}


/*
 * Example DrawBuffers scenarios:
 *
 * 1. glDrawBuffer(GL_FRONT_AND_BACK), fixed-func or shader writes to
 * "gl_FragColor" or program writes to the "result.color" register:
 *
 *   fragment color output   renderbuffer
 *   ---------------------   ---------------
 *   color[0]                Front, Back
 *
 *
 * 2. glDrawBuffers(3, [GL_FRONT, GL_AUX0, GL_AUX1]), shader writes to
 * gl_FragData[i] or program writes to result.color[i] registers:
 *
 *   fragment color output   renderbuffer
 *   ---------------------   ---------------
 *   color[0]                Front
 *   color[1]                Aux0
 *   color[3]                Aux1
 *
 *
 * 3. glDrawBuffers(3, [GL_FRONT, GL_AUX0, GL_AUX1]) and shader writes to
 * gl_FragColor, or fixed function:
 *
 *   fragment color output   renderbuffer
 *   ---------------------   ---------------
 *   color[0]                Front, Aux0, Aux1
 *
 *
 * In either case, the list of renderbuffers is stored in the
 * framebuffer->_ColorDrawBuffers[] array and
 * framebuffer->_NumColorDrawBuffers indicates the number of buffers.
 * The renderer (like swrast) has to look at the current fragment shader
 * to see if it writes to gl_FragColor vs. gl_FragData[i] to determine
 * how to map color outputs to renderbuffers.
 *
 * Note that these two calls are equivalent (for fixed function fragment
 * shading anyway):
 *   a)  glDrawBuffer(GL_FRONT_AND_BACK);  (assuming non-stereo framebuffer)
 *   b)  glDrawBuffers(2, [GL_FRONT_LEFT, GL_BACK_LEFT]);
 */




/**
 * Update the (derived) list of color drawing renderbuffer pointers.
 * Later, when we're rendering we'll loop from 0 to _NumColorDrawBuffers
 * writing colors.
 */
static void
update_color_draw_buffers(struct gl_context *ctx, struct gl_framebuffer *fb)
{
   GLuint output;

   /* set 0th buffer to NULL now in case _NumColorDrawBuffers is zero */
   fb->_ColorDrawBuffers[0] = NULL;

   for (output = 0; output < fb->_NumColorDrawBuffers; output++) {
      GLint buf = fb->_ColorDrawBufferIndexes[output];
      if (buf >= 0) {
         fb->_ColorDrawBuffers[output] = fb->Attachment[buf].Renderbuffer;
      }
      else {
         fb->_ColorDrawBuffers[output] = NULL;
      }
   }
}


/**
 * Update the (derived) color read renderbuffer pointer.
 * Unlike the DrawBuffer, we can only read from one (or zero) color buffers.
 */
static void
update_color_read_buffer(struct gl_context *ctx, struct gl_framebuffer *fb)
{
   (void) ctx;
   if (fb->_ColorReadBufferIndex == -1 ||
       fb->DeletePending ||
       fb->Width == 0 ||
       fb->Height == 0) {
      fb->_ColorReadBuffer = NULL; /* legal! */
   }
   else {
      ASSERT(fb->_ColorReadBufferIndex >= 0);
      ASSERT(fb->_ColorReadBufferIndex < BUFFER_COUNT);
      fb->_ColorReadBuffer
         = fb->Attachment[fb->_ColorReadBufferIndex].Renderbuffer;
   }
}


/**
 * Update a gl_framebuffer's derived state.
 *
 * Specifically, update these framebuffer fields:
 *    _ColorDrawBuffers
 *    _NumColorDrawBuffers
 *    _ColorReadBuffer
 *
 * If the framebuffer is user-created, make sure it's complete.
 *
 * The following functions (at least) can effect framebuffer state:
 * glReadBuffer, glDrawBuffer, glDrawBuffersARB, glFramebufferRenderbufferEXT,
 * glRenderbufferStorageEXT.
 */
static void
update_framebuffer(struct gl_context *ctx, struct gl_framebuffer *fb)
{
   if (fb->Name == 0) {
      /* This is a window-system framebuffer */
      /* Need to update the FB's GL_DRAW_BUFFER state to match the
       * context state (GL_READ_BUFFER too).
       */
      if (fb->ColorDrawBuffer[0] != ctx->Color.DrawBuffer[0]) {
         _mesa_drawbuffers(ctx, ctx->Const.MaxDrawBuffers,
                           ctx->Color.DrawBuffer, NULL);
      }
   }
   else {
      /* This is a user-created framebuffer.
       * Completeness only matters for user-created framebuffers.
       */
      if (fb->_Status != GL_FRAMEBUFFER_COMPLETE) {
         _mesa_test_framebuffer_completeness(ctx, fb);
      }
   }

   /* Strictly speaking, we don't need to update the draw-state
    * if this FB is bound as ctx->ReadBuffer (and conversely, the
    * read-state if this FB is bound as ctx->DrawBuffer), but no
    * harm.
    */
   update_color_draw_buffers(ctx, fb);
   update_color_read_buffer(ctx, fb);

   compute_depth_max(fb);
}


/**
 * Update state related to the current draw/read framebuffers.
 */
void
_mesa_update_framebuffer(struct gl_context *ctx)
{
   struct gl_framebuffer *drawFb;
   struct gl_framebuffer *readFb;

   assert(ctx);
   drawFb = ctx->DrawBuffer;
   readFb = ctx->ReadBuffer;

   update_framebuffer(ctx, drawFb);
   if (readFb != drawFb)
      update_framebuffer(ctx, readFb);
}


/**
 * Check if the renderbuffer for a read/draw operation exists.
 * \param format  a basic image format such as GL_RGB, GL_RGBA, GL_ALPHA,
 *                GL_DEPTH_COMPONENT, etc. or GL_COLOR, GL_DEPTH, GL_STENCIL.
 * \param reading  if TRUE, we're going to read from the buffer,
                   if FALSE, we're going to write to the buffer.
 * \return GL_TRUE if buffer exists, GL_FALSE otherwise
 */
static GLboolean
renderbuffer_exists(struct gl_context *ctx,
                    struct gl_framebuffer *fb,
                    GLenum format,
                    GLboolean reading)
{
   const struct gl_renderbuffer_attachment *att = fb->Attachment;

   /* If we don't know the framebuffer status, update it now */
   if (fb->_Status == 0) {
      _mesa_test_framebuffer_completeness(ctx, fb);
   }

   if (fb->_Status != GL_FRAMEBUFFER_COMPLETE_EXT) {
      return GL_FALSE;
   }

   switch (format) {
   case GL_COLOR:
   case GL_RED:
   case GL_GREEN:
   case GL_BLUE:
   case GL_ALPHA:
   case GL_LUMINANCE:
   case GL_LUMINANCE_ALPHA:
   case GL_INTENSITY:
   case GL_RG:
   case GL_RGB:
   case GL_BGR:
   case GL_RGBA:
   case GL_BGRA:
   case GL_ABGR_EXT:
   case GL_RED_INTEGER_EXT:
   case GL_RG_INTEGER:
   case GL_GREEN_INTEGER_EXT:
   case GL_BLUE_INTEGER_EXT:
   case GL_ALPHA_INTEGER_EXT:
   case GL_RGB_INTEGER_EXT:
   case GL_RGBA_INTEGER_EXT:
   case GL_BGR_INTEGER_EXT:
   case GL_BGRA_INTEGER_EXT:
   case GL_LUMINANCE_INTEGER_EXT:
   case GL_LUMINANCE_ALPHA_INTEGER_EXT:
      if (reading) {
         /* about to read from a color buffer */
         const struct gl_renderbuffer *readBuf = fb->_ColorReadBuffer;
         if (!readBuf) {
            return GL_FALSE;
         }
         ASSERT(_mesa_get_format_bits(readBuf->Format, GL_RED_BITS) > 0 ||
                _mesa_get_format_bits(readBuf->Format, GL_ALPHA_BITS) > 0 ||
                _mesa_get_format_bits(readBuf->Format, GL_TEXTURE_LUMINANCE_SIZE) > 0 ||
                _mesa_get_format_bits(readBuf->Format, GL_TEXTURE_INTENSITY_SIZE) > 0 ||
                _mesa_get_format_bits(readBuf->Format, GL_INDEX_BITS) > 0);
      }
      else {
         /* about to draw to zero or more color buffers (none is OK) */
         return GL_TRUE;
      }
      break;
   case GL_DEPTH:
   case GL_DEPTH_COMPONENT:
      if (att[BUFFER_DEPTH].Type == GL_NONE) {
         return GL_FALSE;
      }
      break;
   case GL_STENCIL:
   case GL_STENCIL_INDEX:
      if (att[BUFFER_STENCIL].Type == GL_NONE) {
         return GL_FALSE;
      }
      break;
   case GL_DEPTH_STENCIL_EXT:
      if (att[BUFFER_DEPTH].Type == GL_NONE ||
          att[BUFFER_STENCIL].Type == GL_NONE) {
         return GL_FALSE;
      }
      break;
   default:
      _mesa_problem(ctx,
                    "Unexpected format 0x%x in renderbuffer_exists",
                    format);
      return GL_FALSE;
   }

   /* OK */
   return GL_TRUE;
}


/**
 * Check if the renderbuffer for a read operation (glReadPixels, glCopyPixels,
 * glCopyTex[Sub]Image, etc) exists.
 * \param format  a basic image format such as GL_RGB, GL_RGBA, GL_ALPHA,
 *                GL_DEPTH_COMPONENT, etc. or GL_COLOR, GL_DEPTH, GL_STENCIL.
 * \return GL_TRUE if buffer exists, GL_FALSE otherwise
 */
GLboolean
_mesa_source_buffer_exists(struct gl_context *ctx, GLenum format)
{
   return renderbuffer_exists(ctx, ctx->ReadBuffer, format, GL_TRUE);
}


/**
 * As above, but for drawing operations.
 * XXX could do some code merging w/ above function.
 */
GLboolean
_mesa_dest_buffer_exists(struct gl_context *ctx, GLenum format)
{
   return renderbuffer_exists(ctx, ctx->DrawBuffer, format, GL_FALSE);
}


/**
 * Used to answer the GL_IMPLEMENTATION_COLOR_READ_FORMAT_OES query.
 */
GLenum
_mesa_get_color_read_format(struct gl_context *ctx)
{
   switch (ctx->ReadBuffer->_ColorReadBuffer->Format) {
   case MESA_FORMAT_ARGB8888:
      return GL_BGRA;
   case MESA_FORMAT_RGB565:
      return GL_BGR;
   default:
      return GL_RGBA;
   }
}


/**
 * Used to answer the GL_IMPLEMENTATION_COLOR_READ_TYPE_OES query.
 */
GLenum
_mesa_get_color_read_type(struct gl_context *ctx)
{
   switch (ctx->ReadBuffer->_ColorReadBuffer->Format) {
   case MESA_FORMAT_ARGB8888:
      return GL_UNSIGNED_BYTE;
   case MESA_FORMAT_RGB565:
      return GL_UNSIGNED_SHORT_5_6_5_REV;
   default:
      return GL_UNSIGNED_BYTE;
   }
}


/**
 * Print framebuffer info to stderr, for debugging.
 */
void
_mesa_print_framebuffer(const struct gl_framebuffer *fb)
{
   GLuint i;

   fprintf(stderr, "Mesa Framebuffer %u at %p\n", fb->Name, (void *) fb);
   fprintf(stderr, "  Size: %u x %u  Status: %s\n", fb->Width, fb->Height,
           _mesa_lookup_enum_by_nr(fb->_Status));
   fprintf(stderr, "  Attachments:\n");

   for (i = 0; i < BUFFER_COUNT; i++) {
      const struct gl_renderbuffer_attachment *att = &fb->Attachment[i];
      if (att->Type == GL_TEXTURE) {
         const struct gl_texture_image *texImage =
            _mesa_get_attachment_teximage_const(att);
         fprintf(stderr,
                 "  %2d: Texture %u, level %u, face %u, slice %u, complete %d\n",
                 i, att->Texture->Name, att->TextureLevel, att->CubeMapFace,
                 att->Zoffset, att->Complete);
         fprintf(stderr, "       Size: %u x %u x %u  Format %s\n",
                 texImage->Width, texImage->Height, texImage->Depth,
                 _mesa_get_format_name(texImage->TexFormat));
      }
      else if (att->Type == GL_RENDERBUFFER) {
         fprintf(stderr, "  %2d: Renderbuffer %u, complete %d\n",
                 i, att->Renderbuffer->Name, att->Complete);
         fprintf(stderr, "       Size: %u x %u  Format %s\n",
                 att->Renderbuffer->Width, att->Renderbuffer->Height,
                 _mesa_get_format_name(att->Renderbuffer->Format));
      }
      else {
         fprintf(stderr, "  %2d: none\n", i);
      }
   }
}
