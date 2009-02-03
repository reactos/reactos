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
#include "depthstencil.h"
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
   (void) ctx;
   assert(name != 0);
   fb = CALLOC_STRUCT(gl_framebuffer);
   if (fb) {
      fb->Name = name;
      fb->RefCount = 1;
      fb->_NumColorDrawBuffers = 1;
      fb->ColorDrawBuffer[0] = GL_COLOR_ATTACHMENT0_EXT;
      fb->_ColorDrawBufferIndexes[0] = BUFFER_COLOR0;
      fb->ColorReadBuffer = GL_COLOR_ATTACHMENT0_EXT;
      fb->_ColorReadBufferIndex = BUFFER_COLOR0;
      fb->Delete = _mesa_destroy_framebuffer;
   }
   return fb;
}


/**
 * Initialize a gl_framebuffer object.  Typically used to initialize
 * window system-created framebuffers, not user-created framebuffers.
 * \sa _mesa_create_framebuffer
 */
void
_mesa_initialize_framebuffer(struct gl_framebuffer *fb, const GLvisual *visual)
{
   assert(fb);
   assert(visual);

   _mesa_bzero(fb, sizeof(struct gl_framebuffer));

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
 * Deallocate buffer and everything attached to it.
 * Typically called via the gl_framebuffer->Delete() method.
 */
void
_mesa_destroy_framebuffer(struct gl_framebuffer *fb)
{
   if (fb) {
      _mesa_free_framebuffer_data(fb);
      _mesa_free(fb);
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

   /* unbind _Depth/_StencilBuffer to decr ref counts */
   _mesa_reference_renderbuffer(&fb->_DepthBuffer, NULL);
   _mesa_reference_renderbuffer(&fb->_StencilBuffer, NULL);
}


/**
 * Set *ptr to point to fb, with refcounting and locking.
 */
void
_mesa_reference_framebuffer(struct gl_framebuffer **ptr,
                            struct gl_framebuffer *fb)
{
   assert(ptr);
   if (*ptr == fb) {
      /* no change */
      return;
   }
   if (*ptr) {
      _mesa_unreference_framebuffer(ptr);
   }
   assert(!*ptr);
   assert(fb);
   _glthread_LOCK_MUTEX(fb->Mutex);
   fb->RefCount++;
   _glthread_UNLOCK_MUTEX(fb->Mutex);
   *ptr = fb;
}


/**
 * Undo/remove a reference to a framebuffer object.
 * Decrement the framebuffer object's reference count and delete it when
 * the refcount hits zero.
 * Note: we pass the address of a pointer and set it to NULL.
 */
void
_mesa_unreference_framebuffer(struct gl_framebuffer **fb)
{
   assert(fb);
   if (*fb) {
      GLboolean deleteFlag = GL_FALSE;

      _glthread_LOCK_MUTEX((*fb)->Mutex);
      ASSERT((*fb)->RefCount > 0);
      (*fb)->RefCount--;
      deleteFlag = ((*fb)->RefCount == 0);
      _glthread_UNLOCK_MUTEX((*fb)->Mutex);
      
      if (deleteFlag)
         (*fb)->Delete(*fb);

      *fb = NULL;
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
_mesa_resize_framebuffer(GLcontext *ctx, struct gl_framebuffer *fb,
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
            /* could just as well pass rb->_ActualFormat here */
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

   if (fb->_DepthBuffer) {
      struct gl_renderbuffer *rb = fb->_DepthBuffer;
      if (rb->Width != width || rb->Height != height) {
         if (!rb->AllocStorage(ctx, rb, rb->InternalFormat, width, height)) {
            _mesa_error(ctx, GL_OUT_OF_MEMORY, "Resizing framebuffer");
         }
      }
   }

   if (fb->_StencilBuffer) {
      struct gl_renderbuffer *rb = fb->_StencilBuffer;
      if (rb->Width != width || rb->Height != height) {
         if (!rb->AllocStorage(ctx, rb, rb->InternalFormat, width, height)) {
            _mesa_error(ctx, GL_OUT_OF_MEMORY, "Resizing framebuffer");
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
_mesa_resizebuffers( GLcontext *ctx )
{
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH( ctx );

   if (MESA_VERBOSE & VERBOSE_API)
      _mesa_debug(ctx, "glResizeBuffersMESA\n");

   if (!ctx->Driver.GetBufferSize) {
      return;
   }

   if (ctx->WinSysDrawBuffer) {
      GLuint newWidth, newHeight;
      GLframebuffer *buffer = ctx->WinSysDrawBuffer;

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
      GLframebuffer *buffer = ctx->WinSysReadBuffer;

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

   if (!buffer)
      return;

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
_mesa_update_framebuffer_visual(struct gl_framebuffer *fb)
{
   GLuint i;

   _mesa_bzero(&fb->Visual, sizeof(fb->Visual));
   fb->Visual.rgbMode = GL_TRUE; /* assume this */

#if 0 /* this _might_ be needed */
   if (fb->_Status != GL_FRAMEBUFFER_COMPLETE_EXT) {
      /* leave visual fields zero'd */
      return;
   }
#endif

   /* find first RGB or CI renderbuffer */
   for (i = 0; i < BUFFER_COUNT; i++) {
      if (fb->Attachment[i].Renderbuffer) {
         const struct gl_renderbuffer *rb = fb->Attachment[i].Renderbuffer;
         if (rb->_BaseFormat == GL_RGBA || rb->_BaseFormat == GL_RGB) {
            fb->Visual.redBits = rb->RedBits;
            fb->Visual.greenBits = rb->GreenBits;
            fb->Visual.blueBits = rb->BlueBits;
            fb->Visual.alphaBits = rb->AlphaBits;
            fb->Visual.rgbBits = fb->Visual.redBits
               + fb->Visual.greenBits + fb->Visual.blueBits;
            fb->Visual.floatMode = GL_FALSE;
            break;
         }
         else if (rb->_BaseFormat == GL_COLOR_INDEX) {
            fb->Visual.indexBits = rb->IndexBits;
            fb->Visual.rgbMode = GL_FALSE;
            break;
         }
      }
   }

   if (fb->Attachment[BUFFER_DEPTH].Renderbuffer) {
      fb->Visual.haveDepthBuffer = GL_TRUE;
      fb->Visual.depthBits
         = fb->Attachment[BUFFER_DEPTH].Renderbuffer->DepthBits;
   }

   if (fb->Attachment[BUFFER_STENCIL].Renderbuffer) {
      fb->Visual.haveStencilBuffer = GL_TRUE;
      fb->Visual.stencilBits
         = fb->Attachment[BUFFER_STENCIL].Renderbuffer->StencilBits;
   }

   if (fb->Attachment[BUFFER_ACCUM].Renderbuffer) {
      fb->Visual.haveAccumBuffer = GL_TRUE;
      fb->Visual.accumRedBits
         = fb->Attachment[BUFFER_ACCUM].Renderbuffer->RedBits;
      fb->Visual.accumGreenBits
         = fb->Attachment[BUFFER_ACCUM].Renderbuffer->GreenBits;
      fb->Visual.accumBlueBits
         = fb->Attachment[BUFFER_ACCUM].Renderbuffer->BlueBits;
      fb->Visual.accumAlphaBits
         = fb->Attachment[BUFFER_ACCUM].Renderbuffer->AlphaBits;
   }

   compute_depth_max(fb);
}


/**
 * Update the framebuffer's _DepthBuffer field using the renderbuffer
 * found at the given attachment index.
 *
 * If that attachment points to a combined GL_DEPTH_STENCIL renderbuffer,
 * create and install a depth wrapper/adaptor.
 *
 * \param fb  the framebuffer whose _DepthBuffer field to update
 * \param attIndex  indicates the renderbuffer to possibly wrap
 */
void
_mesa_update_depth_buffer(GLcontext *ctx,
                          struct gl_framebuffer *fb,
                          GLuint attIndex)
{
   struct gl_renderbuffer *depthRb;

   /* only one possiblity for now */
   ASSERT(attIndex == BUFFER_DEPTH);

   depthRb = fb->Attachment[attIndex].Renderbuffer;

   if (depthRb && depthRb->_ActualFormat == GL_DEPTH24_STENCIL8_EXT) {
      /* The attached depth buffer is a GL_DEPTH_STENCIL renderbuffer */
      if (!fb->_DepthBuffer
          || fb->_DepthBuffer->Wrapped != depthRb
          || fb->_DepthBuffer->_BaseFormat != GL_DEPTH_COMPONENT) {
         /* need to update wrapper */
         struct gl_renderbuffer *wrapper
            = _mesa_new_z24_renderbuffer_wrapper(ctx, depthRb);
         _mesa_reference_renderbuffer(&fb->_DepthBuffer, wrapper);
         ASSERT(fb->_DepthBuffer->Wrapped == depthRb);
      }
   }
   else {
      /* depthRb may be null */
      _mesa_reference_renderbuffer(&fb->_DepthBuffer, depthRb);
   }
}


/**
 * Update the framebuffer's _StencilBuffer field using the renderbuffer
 * found at the given attachment index.
 *
 * If that attachment points to a combined GL_DEPTH_STENCIL renderbuffer,
 * create and install a stencil wrapper/adaptor.
 *
 * \param fb  the framebuffer whose _StencilBuffer field to update
 * \param attIndex  indicates the renderbuffer to possibly wrap
 */
void
_mesa_update_stencil_buffer(GLcontext *ctx,
                            struct gl_framebuffer *fb,
                            GLuint attIndex)
{
   struct gl_renderbuffer *stencilRb;

   ASSERT(attIndex == BUFFER_DEPTH ||
          attIndex == BUFFER_STENCIL);

   stencilRb = fb->Attachment[attIndex].Renderbuffer;

   if (stencilRb && stencilRb->_ActualFormat == GL_DEPTH24_STENCIL8_EXT) {
      /* The attached stencil buffer is a GL_DEPTH_STENCIL renderbuffer */
      if (!fb->_StencilBuffer
          || fb->_StencilBuffer->Wrapped != stencilRb
          || fb->_StencilBuffer->_BaseFormat != GL_STENCIL_INDEX) {
         /* need to update wrapper */
         struct gl_renderbuffer *wrapper
            = _mesa_new_s8_renderbuffer_wrapper(ctx, stencilRb);
         _mesa_reference_renderbuffer(&fb->_StencilBuffer, wrapper);
         ASSERT(fb->_StencilBuffer->Wrapped == stencilRb);
      }
   }
   else {
      /* stencilRb may be null */
      _mesa_reference_renderbuffer(&fb->_StencilBuffer, stencilRb);
   }
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
update_color_draw_buffers(GLcontext *ctx, struct gl_framebuffer *fb)
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
update_color_read_buffer(GLcontext *ctx, struct gl_framebuffer *fb)
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
 *    _DepthBuffer
 *    _StencilBuffer
 *
 * If the framebuffer is user-created, make sure it's complete.
 *
 * The following functions (at least) can effect framebuffer state:
 * glReadBuffer, glDrawBuffer, glDrawBuffersARB, glFramebufferRenderbufferEXT,
 * glRenderbufferStorageEXT.
 */
static void
update_framebuffer(GLcontext *ctx, struct gl_framebuffer *fb)
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
      if (fb->ColorReadBuffer != ctx->Pixel.ReadBuffer) {
         
      }
   }
   else {
      /* This is a user-created framebuffer.
       * Completeness only matters for user-created framebuffers.
       */
      _mesa_test_framebuffer_completeness(ctx, fb);
      _mesa_update_framebuffer_visual(fb);
   }

   /* Strictly speaking, we don't need to update the draw-state
    * if this FB is bound as ctx->ReadBuffer (and conversely, the
    * read-state if this FB is bound as ctx->DrawBuffer), but no
    * harm.
    */
   update_color_draw_buffers(ctx, fb);
   update_color_read_buffer(ctx, fb);
   _mesa_update_depth_buffer(ctx, fb, BUFFER_DEPTH);
   _mesa_update_stencil_buffer(ctx, fb, BUFFER_STENCIL);

   compute_depth_max(fb);
}


/**
 * Update state related to the current draw/read framebuffers.
 */
void
_mesa_update_framebuffer(GLcontext *ctx)
{
   struct gl_framebuffer *drawFb = ctx->DrawBuffer;
   struct gl_framebuffer *readFb = ctx->ReadBuffer;

   update_framebuffer(ctx, drawFb);
   if (readFb != drawFb)
      update_framebuffer(ctx, readFb);
}


/**
 * Check if the renderbuffer for a read operation (glReadPixels, glCopyPixels,
 * glCopyTex[Sub]Image, etc. exists.
 * \param format  a basic image format such as GL_RGB, GL_RGBA, GL_ALPHA,
 *                GL_DEPTH_COMPONENT, etc. or GL_COLOR, GL_DEPTH, GL_STENCIL.
 * \return GL_TRUE if buffer exists, GL_FALSE otherwise
 */
GLboolean
_mesa_source_buffer_exists(GLcontext *ctx, GLenum format)
{
   const struct gl_renderbuffer_attachment *att
      = ctx->ReadBuffer->Attachment;

   if (ctx->ReadBuffer->_Status != GL_FRAMEBUFFER_COMPLETE_EXT) {
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
   case GL_RGB:
   case GL_BGR:
   case GL_RGBA:
   case GL_BGRA:
   case GL_ABGR_EXT:
   case GL_COLOR_INDEX:
      if (ctx->ReadBuffer->_ColorReadBuffer == NULL) {
         return GL_FALSE;
      }
      /* XXX enable this post 6.5 release:
      ASSERT(ctx->ReadBuffer->_ColorReadBuffer->RedBits > 0 ||
             ctx->ReadBuffer->_ColorReadBuffer->IndexBits > 0);
      */
      break;
   case GL_DEPTH:
   case GL_DEPTH_COMPONENT:
      if (!att[BUFFER_DEPTH].Renderbuffer) {
         return GL_FALSE;
      }
      ASSERT(att[BUFFER_DEPTH].Renderbuffer->DepthBits > 0);
      break;
   case GL_STENCIL:
   case GL_STENCIL_INDEX:
      if (!att[BUFFER_STENCIL].Renderbuffer) {
         return GL_FALSE;
      }
      ASSERT(att[BUFFER_STENCIL].Renderbuffer->StencilBits > 0);
      break;
   case GL_DEPTH_STENCIL_EXT:
      if (!att[BUFFER_DEPTH].Renderbuffer ||
          !att[BUFFER_STENCIL].Renderbuffer) {
         return GL_FALSE;
      }
      ASSERT(att[BUFFER_DEPTH].Renderbuffer->DepthBits > 0);
      ASSERT(att[BUFFER_STENCIL].Renderbuffer->StencilBits > 0);
      break;
   default:
      _mesa_problem(ctx,
                    "Unexpected format 0x%x in _mesa_source_buffer_exists",
                    format);
      return GL_FALSE;
   }

   /* OK */
   return GL_TRUE;
}


/**
 * As above, but for drawing operations.
 * XXX code do some code merging w/ above function.
 */
GLboolean
_mesa_dest_buffer_exists(GLcontext *ctx, GLenum format)
{
   const struct gl_renderbuffer_attachment *att
      = ctx->ReadBuffer->Attachment;

   if (ctx->DrawBuffer->_Status != GL_FRAMEBUFFER_COMPLETE_EXT) {
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
   case GL_RGB:
   case GL_BGR:
   case GL_RGBA:
   case GL_BGRA:
   case GL_ABGR_EXT:
   case GL_COLOR_INDEX:
      /* nothing special */
      /* Could assert that colorbuffer has RedBits > 0 */
      break;
   case GL_DEPTH:
   case GL_DEPTH_COMPONENT:
      if (!att[BUFFER_DEPTH].Renderbuffer) {
         return GL_FALSE;
      }
      ASSERT(att[BUFFER_DEPTH].Renderbuffer->DepthBits > 0);
      break;
   case GL_STENCIL:
   case GL_STENCIL_INDEX:
      if (!att[BUFFER_STENCIL].Renderbuffer) {
         return GL_FALSE;
      }
      ASSERT(att[BUFFER_STENCIL].Renderbuffer->StencilBits > 0);
      break;
   case GL_DEPTH_STENCIL_EXT:
      if (!att[BUFFER_DEPTH].Renderbuffer ||
          !att[BUFFER_STENCIL].Renderbuffer) {
         return GL_FALSE;
      }
      ASSERT(att[BUFFER_DEPTH].Renderbuffer->DepthBits > 0);
      ASSERT(att[BUFFER_STENCIL].Renderbuffer->StencilBits > 0);
      break;
   default:
      _mesa_problem(ctx,
                    "Unexpected format 0x%x in _mesa_source_buffer_exists",
                    format);
      return GL_FALSE;
   }

   /* OK */
   return GL_TRUE;
}
