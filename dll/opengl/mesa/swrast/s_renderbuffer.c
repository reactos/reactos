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

/**
 * Functions for allocating/managing software-based renderbuffers.
 * Also, routines for reading/writing software-based renderbuffer data as
 * ubytes, ushorts, uints, etc.
 */

#include <precomp.h>

#include <main/renderbuffer.h>
#include "s_renderbuffer.h"

static
GLenum
_mesa_base_fb_format(struct gl_context *ctx, GLenum internalFormat)
{
   /*
    * Notes: some formats such as alpha, luminance, etc. were added
    * with GL_ARB_framebuffer_object.
    */
   switch (internalFormat) {
   case GL_RGB:
   case GL_R3_G3_B2:
   case GL_RGB4:
   case GL_RGB5:
   case GL_RGB8:
   case GL_RGB10:
   case GL_RGB12:
   case GL_RGB16:
      return GL_RGB;
   case GL_RGBA:
   case GL_RGBA2:
   case GL_RGBA4:
   case GL_RGB5_A1:
   case GL_RGBA8:
   case GL_RGB10_A2:
   case GL_RGBA12:
   case GL_RGBA16:
      return GL_RGBA;
   case GL_STENCIL_INDEX:
   case GL_STENCIL_INDEX1_EXT:
   case GL_STENCIL_INDEX4_EXT:
   case GL_STENCIL_INDEX8_EXT:
   case GL_STENCIL_INDEX16_EXT:
      return GL_STENCIL_INDEX;
   case GL_DEPTH_COMPONENT:
   case GL_DEPTH_COMPONENT16:
   case GL_DEPTH_COMPONENT24:
   case GL_DEPTH_COMPONENT32:
      return GL_DEPTH_COMPONENT;

   default:
      return 0;
   }
}


/**
 * This is a software fallback for the gl_renderbuffer->AllocStorage
 * function.
 * Device drivers will typically override this function for the buffers
 * which it manages (typically color buffers, Z and stencil).
 * Other buffers (like software accumulation and aux buffers) which the driver
 * doesn't manage can be handled with this function.
 *
 * This one multi-purpose function can allocate stencil, depth, accum, color
 * or color-index buffers!
 */
static GLboolean
soft_renderbuffer_storage(struct gl_context *ctx, struct gl_renderbuffer *rb,
                          GLenum internalFormat,
                          GLuint width, GLuint height)
{
   struct swrast_renderbuffer *srb = swrast_renderbuffer(rb);
   GLuint bpp;

   switch (internalFormat) {
   case GL_RGB:
   case GL_R3_G3_B2:
   case GL_RGB4:
   case GL_RGB5:
   case GL_RGB8:
   case GL_RGB10:
   case GL_RGB12:
   case GL_RGB16:
      rb->Format = MESA_FORMAT_RGB888;
      break;
   case GL_RGBA:
   case GL_RGBA2:
   case GL_RGBA4:
   case GL_RGB5_A1:
   case GL_RGBA8:
#if 1
   case GL_RGB10_A2:
   case GL_RGBA12:
#endif
      if (_mesa_little_endian())
         rb->Format = MESA_FORMAT_RGBA8888_REV;
      else
         rb->Format = MESA_FORMAT_RGBA8888;
      break;
   case GL_RGBA16:
   case GL_RGBA16_SNORM:
      /* for accum buffer */
      rb->Format = MESA_FORMAT_SIGNED_RGBA_16;
      break;
   case GL_STENCIL_INDEX:
   case GL_STENCIL_INDEX1_EXT:
   case GL_STENCIL_INDEX4_EXT:
   case GL_STENCIL_INDEX8_EXT:
   case GL_STENCIL_INDEX16_EXT:
      rb->Format = MESA_FORMAT_S8;
      break;
   case GL_DEPTH_COMPONENT:
   case GL_DEPTH_COMPONENT16:
      rb->Format = MESA_FORMAT_Z16;
      break;
   case GL_DEPTH_COMPONENT24:
      rb->Format = MESA_FORMAT_X8_Z24;
      break;
   case GL_DEPTH_COMPONENT32:
      rb->Format = MESA_FORMAT_Z32;
      break;
   default:
      /* unsupported format */
      return GL_FALSE;
   }

   bpp = _mesa_get_format_bytes(rb->Format);

   /* free old buffer storage */
   if (srb->Buffer) {
      free(srb->Buffer);
      srb->Buffer = NULL;
   }

   srb->RowStride = width * bpp;

   if (width > 0 && height > 0) {
      /* allocate new buffer storage */
      srb->Buffer = malloc(srb->RowStride * height);

      if (srb->Buffer == NULL) {
         rb->Width = 0;
         rb->Height = 0;
         _mesa_error(ctx, GL_OUT_OF_MEMORY,
                     "software renderbuffer allocation (%d x %d x %d)",
                     width, height, bpp);
         return GL_FALSE;
      }
   }

   rb->Width = width;
   rb->Height = height;
   rb->_BaseFormat = _mesa_base_fb_format(ctx, internalFormat);

   /* the internalFormat should have been error checked long ago */
   ASSERT(rb->_BaseFormat);

   return GL_TRUE;
}


/**
 * Called via gl_renderbuffer::Delete()
 */
static void
soft_renderbuffer_delete(struct gl_renderbuffer *rb)
{
   struct swrast_renderbuffer *srb = swrast_renderbuffer(rb);

   if (srb->Buffer) {
      free(srb->Buffer);
      srb->Buffer = NULL;
   }
   free(srb);
}


void
_swrast_map_soft_renderbuffer(struct gl_context *ctx,
                              struct gl_renderbuffer *rb,
                              GLuint x, GLuint y, GLuint w, GLuint h,
                              GLbitfield mode,
                              GLubyte **out_map,
                              GLint *out_stride)
{
   struct swrast_renderbuffer *srb = swrast_renderbuffer(rb);
   GLubyte *map = srb->Buffer;
   int cpp = _mesa_get_format_bytes(rb->Format);
   int stride = rb->Width * cpp;

   if (!map) {
      *out_map = NULL;
      *out_stride = 0;
   }

   map += y * stride;
   map += x * cpp;

   *out_map = map;
   *out_stride = stride;
}


void
_swrast_unmap_soft_renderbuffer(struct gl_context *ctx,
                                struct gl_renderbuffer *rb)
{
}



/**
 * Allocate a software-based renderbuffer.  This is called via the
 * ctx->Driver.NewRenderbuffer() function when the user creates a new
 * renderbuffer.
 * This would not be used for hardware-based renderbuffers.
 */
static
struct gl_renderbuffer *
_swrast_new_soft_renderbuffer(struct gl_context *ctx, GLuint name)
{
   struct swrast_renderbuffer *srb = CALLOC_STRUCT(swrast_renderbuffer);
   if (srb) {
      _mesa_init_renderbuffer(&srb->Base, name);
      srb->Base.AllocStorage = soft_renderbuffer_storage;
      srb->Base.Delete = soft_renderbuffer_delete;
   }
   return &srb->Base;
}

/**
 * Add a software-based depth renderbuffer to the given framebuffer.
 * This is a helper routine for device drivers when creating a
 * window system framebuffer (not a user-created render/framebuffer).
 * Once this function is called, you can basically forget about this
 * renderbuffer; core Mesa will handle all the buffer management and
 * rendering!
 */
static GLboolean
add_depth_renderbuffer(struct gl_context *ctx, struct gl_framebuffer *fb,
                       GLuint depthBits)
{
   struct gl_renderbuffer *rb;

   if (depthBits > 32) {
      _mesa_problem(ctx,
                    "Unsupported depthBits in add_depth_renderbuffer");
      return GL_FALSE;
   }

   assert(fb->Attachment[BUFFER_DEPTH].Renderbuffer == NULL);

   rb = _swrast_new_soft_renderbuffer(ctx, 0);
   if (!rb) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "Allocating depth buffer");
      return GL_FALSE;
   }

   if (depthBits <= 16) {
      rb->InternalFormat = GL_DEPTH_COMPONENT16;
   }
   else if (depthBits <= 24) {
      rb->InternalFormat = GL_DEPTH_COMPONENT24;
   }
   else {
      rb->InternalFormat = GL_DEPTH_COMPONENT32;
   }

   rb->AllocStorage = soft_renderbuffer_storage;
   _mesa_add_renderbuffer(fb, BUFFER_DEPTH, rb);

   return GL_TRUE;
}


/**
 * Add a software-based stencil renderbuffer to the given framebuffer.
 * This is a helper routine for device drivers when creating a
 * window system framebuffer (not a user-created render/framebuffer).
 * Once this function is called, you can basically forget about this
 * renderbuffer; core Mesa will handle all the buffer management and
 * rendering!
 */
static GLboolean
add_stencil_renderbuffer(struct gl_context *ctx, struct gl_framebuffer *fb,
                         GLuint stencilBits)
{
   struct gl_renderbuffer *rb;

   if (stencilBits > 16) {
      _mesa_problem(ctx,
                  "Unsupported stencilBits in add_stencil_renderbuffer");
      return GL_FALSE;
   }

   assert(fb->Attachment[BUFFER_STENCIL].Renderbuffer == NULL);

   rb = _swrast_new_soft_renderbuffer(ctx, 0);
   if (!rb) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "Allocating stencil buffer");
      return GL_FALSE;
   }

   assert(stencilBits <= 8);
   rb->InternalFormat = GL_STENCIL_INDEX8;

   rb->AllocStorage = soft_renderbuffer_storage;
   _mesa_add_renderbuffer(fb, BUFFER_STENCIL, rb);

   return GL_TRUE;
}


/**
 * Add a software-based accumulation renderbuffer to the given framebuffer.
 * This is a helper routine for device drivers when creating a
 * window system framebuffer (not a user-created render/framebuffer).
 * Once this function is called, you can basically forget about this
 * renderbuffer; core Mesa will handle all the buffer management and
 * rendering!
 */
static GLboolean
add_accum_renderbuffer(struct gl_context *ctx, struct gl_framebuffer *fb,
                       GLuint redBits, GLuint greenBits,
                       GLuint blueBits, GLuint alphaBits)
{
   struct gl_renderbuffer *rb;

   if (redBits > 16 || greenBits > 16 || blueBits > 16 || alphaBits > 16) {
      _mesa_problem(ctx,
                    "Unsupported accumBits in add_accum_renderbuffer");
      return GL_FALSE;
   }

   assert(fb->Attachment[BUFFER_ACCUM].Renderbuffer == NULL);

   rb = _swrast_new_soft_renderbuffer(ctx, 0);
   if (!rb) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "Allocating accum buffer");
      return GL_FALSE;
   }

   rb->InternalFormat = GL_RGBA16_SNORM;
   rb->AllocStorage = soft_renderbuffer_storage;
   _mesa_add_renderbuffer(fb, BUFFER_ACCUM, rb);

   return GL_TRUE;
}



/**
 * Add a software-based aux renderbuffer to the given framebuffer.
 * This is a helper routine for device drivers when creating a
 * window system framebuffer (not a user-created render/framebuffer).
 * Once this function is called, you can basically forget about this
 * renderbuffer; core Mesa will handle all the buffer management and
 * rendering!
 *
 * NOTE: color-index aux buffers not supported.
 */
static GLboolean
add_aux_renderbuffers(struct gl_context *ctx, struct gl_framebuffer *fb,
                      GLuint colorBits, GLuint numBuffers)
{
   GLuint i;

   if (colorBits > 16) {
      _mesa_problem(ctx,
                    "Unsupported colorBits in add_aux_renderbuffers");
      return GL_FALSE;
   }

   assert(numBuffers <= MAX_AUX_BUFFERS);

   for (i = 0; i < numBuffers; i++) {
      struct gl_renderbuffer *rb = _swrast_new_soft_renderbuffer(ctx, 0);

      assert(fb->Attachment[BUFFER_AUX0 + i].Renderbuffer == NULL);

      if (!rb) {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "Allocating aux buffer");
         return GL_FALSE;
      }

      assert (colorBits <= 8);
      rb->InternalFormat = GL_RGBA;

      rb->AllocStorage = soft_renderbuffer_storage;
      _mesa_add_renderbuffer(fb, BUFFER_AUX0 + i, rb);
   }
   return GL_TRUE;
}


/**
 * Create/attach software-based renderbuffers to the given framebuffer.
 * This is a helper routine for device drivers.  Drivers can just as well
 * call the individual _mesa_add_*_renderbuffer() routines directly.
 */
void
_swrast_add_soft_renderbuffers(struct gl_framebuffer *fb,
                               GLboolean color,
                               GLboolean depth,
                               GLboolean stencil,
                               GLboolean accum,
                               GLboolean alpha,
                               GLboolean aux)
{
   (void)color;
   
   if (depth) {
      assert(fb->Visual.depthBits > 0);
      add_depth_renderbuffer(NULL, fb, fb->Visual.depthBits);
   }

   if (stencil) {
      assert(fb->Visual.stencilBits > 0);
      add_stencil_renderbuffer(NULL, fb, fb->Visual.stencilBits);
   }

   if (accum) {
      assert(fb->Visual.accumRedBits > 0);
      assert(fb->Visual.accumGreenBits > 0);
      assert(fb->Visual.accumBlueBits > 0);
      add_accum_renderbuffer(NULL, fb,
                             fb->Visual.accumRedBits,
                             fb->Visual.accumGreenBits,
                             fb->Visual.accumBlueBits,
                             fb->Visual.accumAlphaBits);
   }

   if (aux) {
      assert(fb->Visual.numAuxBuffers > 0);
      add_aux_renderbuffers(NULL, fb, fb->Visual.redBits,
                            fb->Visual.numAuxBuffers);
   }

#if 0
   if (multisample) {
      /* maybe someday */
   }
#endif
}



static void
map_attachment(struct gl_context *ctx,
                 struct gl_framebuffer *fb,
                 gl_buffer_index buffer)
{
   struct gl_renderbuffer *rb = fb->Attachment[buffer].Renderbuffer;
   struct swrast_renderbuffer *srb = swrast_renderbuffer(rb);

   if (rb) {
      /* Map ordinary renderbuffer */
      ctx->Driver.MapRenderbuffer(ctx, rb,
                                  0, 0, rb->Width, rb->Height,
                                  GL_MAP_READ_BIT | GL_MAP_WRITE_BIT,
                                  &srb->Map, &srb->RowStride);
   }

   assert(srb->Map);
}
 

static void
unmap_attachment(struct gl_context *ctx,
                   struct gl_framebuffer *fb,
                   gl_buffer_index buffer)
{
   struct gl_renderbuffer *rb = fb->Attachment[buffer].Renderbuffer;
   struct swrast_renderbuffer *srb = swrast_renderbuffer(rb);

   if (rb) {
      /* unmap ordinary renderbuffer */
      ctx->Driver.UnmapRenderbuffer(ctx, rb);
   }

   srb->Map = NULL;
}


/**
 * Determine what type to use (ubyte vs. float) for span colors for the
 * given renderbuffer.
 * See also _swrast_write_rgba_span().
 */
static void
find_renderbuffer_colortype(struct gl_renderbuffer *rb)
{
   struct swrast_renderbuffer *srb = swrast_renderbuffer(rb);
   GLuint rbMaxBits = _mesa_get_format_max_bits(rb->Format);
   GLenum rbDatatype = _mesa_get_format_datatype(rb->Format);

   if (rbDatatype == GL_UNSIGNED_NORMALIZED && rbMaxBits <= 8) {
      /* the buffer's values fit in GLubyte values */
      srb->ColorType = GL_UNSIGNED_BYTE;
   }
   else {
      /* use floats otherwise */
      srb->ColorType = GL_FLOAT;
   }
}


/**
 * Map the renderbuffers we'll use for tri/line/point rendering.
 */
void
_swrast_map_renderbuffers(struct gl_context *ctx)
{
   struct gl_framebuffer *fb = ctx->DrawBuffer;
   struct gl_renderbuffer *depthRb, *stencilRb;

   depthRb = fb->Attachment[BUFFER_DEPTH].Renderbuffer;
   if (depthRb) {
      /* map depth buffer */
      map_attachment(ctx, fb, BUFFER_DEPTH);
   }

   stencilRb = fb->Attachment[BUFFER_STENCIL].Renderbuffer;
   if (stencilRb && stencilRb != depthRb) {
      /* map stencil buffer */
      map_attachment(ctx, fb, BUFFER_STENCIL);
   }

   map_attachment(ctx, fb, fb->_ColorDrawBufferIndex);
   find_renderbuffer_colortype(fb->_ColorDrawBuffer);
}
 
 
/**
 * Unmap renderbuffers after rendering.
 */
void
_swrast_unmap_renderbuffers(struct gl_context *ctx)
{
   struct gl_framebuffer *fb = ctx->DrawBuffer;
   struct gl_renderbuffer *depthRb, *stencilRb;

   depthRb = fb->Attachment[BUFFER_DEPTH].Renderbuffer;
   if (depthRb) {
      /* map depth buffer */
      unmap_attachment(ctx, fb, BUFFER_DEPTH);
   }

   stencilRb = fb->Attachment[BUFFER_STENCIL].Renderbuffer;
   if (stencilRb && stencilRb != depthRb) {
      /* map stencil buffer */
      unmap_attachment(ctx, fb, BUFFER_STENCIL);
   }

   unmap_attachment(ctx, fb, fb->_ColorDrawBufferIndex);
}
