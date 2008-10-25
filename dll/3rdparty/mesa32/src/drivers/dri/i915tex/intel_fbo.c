/**************************************************************************
 * 
 * Copyright 2006 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 **************************************************************************/


#include "imports.h"
#include "mtypes.h"
#include "fbobject.h"
#include "framebuffer.h"
#include "renderbuffer.h"
#include "context.h"
#include "texformat.h"
#include "texrender.h"

#include "intel_context.h"
#include "intel_buffers.h"
#include "intel_depthstencil.h"
#include "intel_fbo.h"
#include "intel_mipmap_tree.h"
#include "intel_regions.h"
#include "intel_span.h"


#define FILE_DEBUG_FLAG DEBUG_FBO

#define INTEL_RB_CLASS 0x12345678


/* XXX FBO: move this to intel_context.h (inlined) */
/**
 * Return a gl_renderbuffer ptr casted to intel_renderbuffer.
 * NULL will be returned if the rb isn't really an intel_renderbuffer.
 * This is determiend by checking the ClassID.
 */
struct intel_renderbuffer *
intel_renderbuffer(struct gl_renderbuffer *rb)
{
   struct intel_renderbuffer *irb = (struct intel_renderbuffer *) rb;
   if (irb && irb->Base.ClassID == INTEL_RB_CLASS) {
      /*_mesa_warning(NULL, "Returning non-intel Rb\n");*/
      return irb;
   }
   else
      return NULL;
}


struct intel_renderbuffer *
intel_get_renderbuffer(struct gl_framebuffer *fb, GLuint attIndex)
{
   return intel_renderbuffer(fb->Attachment[attIndex].Renderbuffer);
}


void
intel_flip_renderbuffers(struct intel_framebuffer *intel_fb)
{
   int current_page = intel_fb->pf_current_page;
   int next_page = (current_page + 1) % intel_fb->pf_num_pages;
   struct gl_renderbuffer *tmp_rb;

   /* Exchange renderbuffers if necessary but make sure their reference counts
    * are preserved.
    */
   if (intel_fb->color_rb[current_page] &&
       intel_fb->Base.Attachment[BUFFER_FRONT_LEFT].Renderbuffer !=
       &intel_fb->color_rb[current_page]->Base) {
      tmp_rb = NULL;
      _mesa_reference_renderbuffer(&tmp_rb,
	 intel_fb->Base.Attachment[BUFFER_FRONT_LEFT].Renderbuffer);
      tmp_rb = &intel_fb->color_rb[current_page]->Base;
      _mesa_reference_renderbuffer(
	 &intel_fb->Base.Attachment[BUFFER_FRONT_LEFT].Renderbuffer, tmp_rb);
      _mesa_reference_renderbuffer(&tmp_rb, NULL);
   }

   if (intel_fb->color_rb[next_page] &&
       intel_fb->Base.Attachment[BUFFER_BACK_LEFT].Renderbuffer !=
       &intel_fb->color_rb[next_page]->Base) {
      tmp_rb = NULL;
      _mesa_reference_renderbuffer(&tmp_rb,
	 intel_fb->Base.Attachment[BUFFER_BACK_LEFT].Renderbuffer);
      tmp_rb = &intel_fb->color_rb[next_page]->Base;
      _mesa_reference_renderbuffer(
	 &intel_fb->Base.Attachment[BUFFER_BACK_LEFT].Renderbuffer, tmp_rb);
      _mesa_reference_renderbuffer(&tmp_rb, NULL);
   }
}


struct intel_region *
intel_get_rb_region(struct gl_framebuffer *fb, GLuint attIndex)
{
   struct intel_renderbuffer *irb = intel_get_renderbuffer(fb, attIndex);

   if (irb)
      return irb->region;
   else
      return NULL;
}



/**
 * Create a new framebuffer object.
 */
static struct gl_framebuffer *
intel_new_framebuffer(GLcontext * ctx, GLuint name)
{
   /* Only drawable state in intel_framebuffer at this time, just use Mesa's
    * class
    */
   return _mesa_new_framebuffer(ctx, name);
}


static void
intel_delete_renderbuffer(struct gl_renderbuffer *rb)
{
   GET_CURRENT_CONTEXT(ctx);
   struct intel_context *intel = intel_context(ctx);
   struct intel_renderbuffer *irb = intel_renderbuffer(rb);

   ASSERT(irb);

   if (irb->PairedStencil || irb->PairedDepth) {
      intel_unpair_depth_stencil(ctx, irb);
   }

   if (intel && irb->region) {
      intel_region_release(&irb->region);
   }

   _mesa_free(irb);
}



/**
 * Return a pointer to a specific pixel in a renderbuffer.
 */
static void *
intel_get_pointer(GLcontext * ctx, struct gl_renderbuffer *rb,
                  GLint x, GLint y)
{
   /* By returning NULL we force all software rendering to go through
    * the span routines.
    */
   return NULL;
}



/**
 * Called via glRenderbufferStorageEXT() to set the format and allocate
 * storage for a user-created renderbuffer.
 */
static GLboolean
intel_alloc_renderbuffer_storage(GLcontext * ctx, struct gl_renderbuffer *rb,
                                 GLenum internalFormat,
                                 GLuint width, GLuint height)
{
   struct intel_context *intel = intel_context(ctx);
   struct intel_renderbuffer *irb = intel_renderbuffer(rb);
   GLboolean softwareBuffer = GL_FALSE;
   int cpp;

   ASSERT(rb->Name != 0);

   switch (internalFormat) {
   case GL_R3_G3_B2:
   case GL_RGB4:
   case GL_RGB5:
      rb->_ActualFormat = GL_RGB5;
      rb->DataType = GL_UNSIGNED_BYTE;
      rb->RedBits = 5;
      rb->GreenBits = 6;
      rb->BlueBits = 5;
      cpp = 2;
      break;
   case GL_RGB:
   case GL_RGB8:
   case GL_RGB10:
   case GL_RGB12:
   case GL_RGB16:
   case GL_RGBA:
   case GL_RGBA2:
   case GL_RGBA4:
   case GL_RGB5_A1:
   case GL_RGBA8:
   case GL_RGB10_A2:
   case GL_RGBA12:
   case GL_RGBA16:
      rb->_ActualFormat = GL_RGBA8;
      rb->DataType = GL_UNSIGNED_BYTE;
      rb->RedBits = 8;
      rb->GreenBits = 8;
      rb->BlueBits = 8;
      rb->AlphaBits = 8;
      cpp = 4;
      break;
   case GL_STENCIL_INDEX:
   case GL_STENCIL_INDEX1_EXT:
   case GL_STENCIL_INDEX4_EXT:
   case GL_STENCIL_INDEX8_EXT:
   case GL_STENCIL_INDEX16_EXT:
      /* alloc a depth+stencil buffer */
      rb->_ActualFormat = GL_DEPTH24_STENCIL8_EXT;
      rb->DataType = GL_UNSIGNED_INT_24_8_EXT;
      rb->StencilBits = 8;
      cpp = 4;
      break;
   case GL_DEPTH_COMPONENT16:
      rb->_ActualFormat = GL_DEPTH_COMPONENT16;
      rb->DataType = GL_UNSIGNED_SHORT;
      rb->DepthBits = 16;
      cpp = 2;
      break;
   case GL_DEPTH_COMPONENT:
   case GL_DEPTH_COMPONENT24:
   case GL_DEPTH_COMPONENT32:
      rb->_ActualFormat = GL_DEPTH24_STENCIL8_EXT;
      rb->DataType = GL_UNSIGNED_INT_24_8_EXT;
      rb->DepthBits = 24;
      cpp = 4;
      break;
   case GL_DEPTH_STENCIL_EXT:
   case GL_DEPTH24_STENCIL8_EXT:
      rb->_ActualFormat = GL_DEPTH24_STENCIL8_EXT;
      rb->DataType = GL_UNSIGNED_INT_24_8_EXT;
      rb->DepthBits = 24;
      rb->StencilBits = 8;
      cpp = 4;
      break;
   default:
      _mesa_problem(ctx,
                    "Unexpected format in intel_alloc_renderbuffer_storage");
      return GL_FALSE;
   }

   intelFlush(ctx);

   /* free old region */
   if (irb->region) {
      intel_region_release(&irb->region);
   }

   /* allocate new memory region/renderbuffer */
   if (softwareBuffer) {
      return _mesa_soft_renderbuffer_storage(ctx, rb, internalFormat,
                                             width, height);
   }
   else {
      /* Choose a pitch to match hardware requirements:
       */
      GLuint pitch = ((cpp * width + 63) & ~63) / cpp;

      /* alloc hardware renderbuffer */
      DBG("Allocating %d x %d Intel RBO (pitch %d)\n", width,
	  height, pitch);

      irb->region = intel_region_alloc(intel->intelScreen, cpp, pitch, height);
      if (!irb->region)
         return GL_FALSE;       /* out of memory? */

      ASSERT(irb->region->buffer);

      rb->Width = width;
      rb->Height = height;

      /* This sets the Get/PutRow/Value functions */
      intel_set_span_functions(&irb->Base);

      return GL_TRUE;
   }
}



/**
 * Called for each hardware renderbuffer when a _window_ is resized.
 * Just update fields.
 * Not used for user-created renderbuffers!
 */
static GLboolean
intel_alloc_window_storage(GLcontext * ctx, struct gl_renderbuffer *rb,
                           GLenum internalFormat, GLuint width, GLuint height)
{
   ASSERT(rb->Name == 0);
   rb->Width = width;
   rb->Height = height;
   rb->_ActualFormat = internalFormat;

   return GL_TRUE;
}

static void
intel_resize_buffers(GLcontext *ctx, struct gl_framebuffer *fb,
		     GLuint width, GLuint height)
{
   struct intel_framebuffer *intel_fb = (struct intel_framebuffer*)fb;
   int i;

   _mesa_resize_framebuffer(ctx, fb, width, height);

   fb->Initialized = GL_TRUE; /* XXX remove someday */

   if (fb->Name != 0) {
      return;
   }

   /* Make sure all window system renderbuffers are up to date */
   for (i = 0; i < 3; i++) {
      struct gl_renderbuffer *rb = &intel_fb->color_rb[i]->Base;

      /* only resize if size is changing */
      if (rb && (rb->Width != width || rb->Height != height)) {
	 rb->AllocStorage(ctx, rb, rb->InternalFormat, width, height);
      }
   }
}

static GLboolean
intel_nop_alloc_storage(GLcontext * ctx, struct gl_renderbuffer *rb,
                        GLenum internalFormat, GLuint width, GLuint height)
{
   _mesa_problem(ctx, "intel_op_alloc_storage should never be called.");
   return GL_FALSE;
}



/**
 * Create a new intel_renderbuffer which corresponds to an on-screen window,
 * not a user-created renderbuffer.
 * \param width  the screen width
 * \param height  the screen height
 */
struct intel_renderbuffer *
intel_create_renderbuffer(GLenum intFormat, GLsizei width, GLsizei height,
                          int offset, int pitch, int cpp, void *map)
{
   GET_CURRENT_CONTEXT(ctx);

   struct intel_renderbuffer *irb;
   const GLuint name = 0;

   irb = CALLOC_STRUCT(intel_renderbuffer);
   if (!irb) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "creating renderbuffer");
      return NULL;
   }

   _mesa_init_renderbuffer(&irb->Base, name);
   irb->Base.ClassID = INTEL_RB_CLASS;

   switch (intFormat) {
   case GL_RGB5:
      irb->Base._ActualFormat = GL_RGB5;
      irb->Base._BaseFormat = GL_RGBA;
      irb->Base.RedBits = 5;
      irb->Base.GreenBits = 6;
      irb->Base.BlueBits = 5;
      irb->Base.DataType = GL_UNSIGNED_BYTE;
      cpp = 2;
      break;
   case GL_RGBA8:
      irb->Base._ActualFormat = GL_RGBA8;
      irb->Base._BaseFormat = GL_RGBA;
      irb->Base.RedBits = 8;
      irb->Base.GreenBits = 8;
      irb->Base.BlueBits = 8;
      irb->Base.AlphaBits = 8;
      irb->Base.DataType = GL_UNSIGNED_BYTE;
      cpp = 4;
      break;
   case GL_STENCIL_INDEX8_EXT:
      irb->Base._ActualFormat = GL_STENCIL_INDEX8_EXT;
      irb->Base._BaseFormat = GL_STENCIL_INDEX;
      irb->Base.StencilBits = 8;
      irb->Base.DataType = GL_UNSIGNED_BYTE;
      cpp = 1;
      break;
   case GL_DEPTH_COMPONENT16:
      irb->Base._ActualFormat = GL_DEPTH_COMPONENT16;
      irb->Base._BaseFormat = GL_DEPTH_COMPONENT;
      irb->Base.DepthBits = 16;
      irb->Base.DataType = GL_UNSIGNED_SHORT;
      cpp = 2;
      break;
   case GL_DEPTH_COMPONENT24:
      irb->Base._ActualFormat = GL_DEPTH24_STENCIL8_EXT;
      irb->Base._BaseFormat = GL_DEPTH_COMPONENT;
      irb->Base.DepthBits = 24;
      irb->Base.DataType = GL_UNSIGNED_INT;
      cpp = 4;
      break;
   case GL_DEPTH24_STENCIL8_EXT:
      irb->Base._ActualFormat = GL_DEPTH24_STENCIL8_EXT;
      irb->Base._BaseFormat = GL_DEPTH_STENCIL_EXT;
      irb->Base.DepthBits = 24;
      irb->Base.StencilBits = 8;
      irb->Base.DataType = GL_UNSIGNED_INT_24_8_EXT;
      cpp = 4;
      break;
   default:
      _mesa_problem(NULL,
                    "Unexpected intFormat in intel_create_renderbuffer");
      return NULL;
   }

   irb->Base.InternalFormat = intFormat;

   /* intel-specific methods */
   irb->Base.Delete = intel_delete_renderbuffer;
   irb->Base.AllocStorage = intel_alloc_window_storage;
   irb->Base.GetPointer = intel_get_pointer;
   /* This sets the Get/PutRow/Value functions */
   intel_set_span_functions(&irb->Base);

   irb->pfMap = map;
   irb->pfPitch = pitch / cpp;	/* in pixels */

#if 00
   irb->region = intel_region_create_static(intel,
                                            DRM_MM_TT,
                                            offset, map, cpp, width, height);
#endif

   return irb;
}


/**
 * Create a new renderbuffer object.
 * Typically called via glBindRenderbufferEXT().
 */
static struct gl_renderbuffer *
intel_new_renderbuffer(GLcontext * ctx, GLuint name)
{
   /*struct intel_context *intel = intel_context(ctx); */
   struct intel_renderbuffer *irb;

   irb = CALLOC_STRUCT(intel_renderbuffer);
   if (!irb) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "creating renderbuffer");
      return NULL;
   }

   _mesa_init_renderbuffer(&irb->Base, name);
   irb->Base.ClassID = INTEL_RB_CLASS;

   /* intel-specific methods */
   irb->Base.Delete = intel_delete_renderbuffer;
   irb->Base.AllocStorage = intel_alloc_renderbuffer_storage;
   irb->Base.GetPointer = intel_get_pointer;
   /* span routines set in alloc_storage function */

   return &irb->Base;
}


/**
 * Called via glBindFramebufferEXT().
 */
static void
intel_bind_framebuffer(GLcontext * ctx, GLenum target,
                       struct gl_framebuffer *fb)
{
   if (target == GL_FRAMEBUFFER_EXT || target == GL_DRAW_FRAMEBUFFER_EXT) {
      intel_draw_buffer(ctx, fb);
      /* Integer depth range depends on depth buffer bits */
      ctx->Driver.DepthRange(ctx, ctx->Viewport.Near, ctx->Viewport.Far);
   }
   else {
      /* don't need to do anything if target == GL_READ_FRAMEBUFFER_EXT */
   }
}


/**
 * Called via glFramebufferRenderbufferEXT().
 */
static void
intel_framebuffer_renderbuffer(GLcontext * ctx,
                               struct gl_framebuffer *fb,
                               GLenum attachment, struct gl_renderbuffer *rb)
{
   DBG("Intel FramebufferRenderbuffer %u %u\n", fb->Name, rb ? rb->Name : 0);

   intelFlush(ctx);

   _mesa_framebuffer_renderbuffer(ctx, fb, attachment, rb);
   intel_draw_buffer(ctx, fb);
}


/**
 * When glFramebufferTexture[123]D is called this function sets up the
 * gl_renderbuffer wrapper around the texture image.
 * This will have the region info needed for hardware rendering.
 */
static struct intel_renderbuffer *
intel_wrap_texture(GLcontext * ctx, struct gl_texture_image *texImage)
{
   const GLuint name = ~0;      /* not significant, but distinct for debugging */
   struct intel_renderbuffer *irb;

   /* make an intel_renderbuffer to wrap the texture image */
   irb = CALLOC_STRUCT(intel_renderbuffer);
   if (!irb) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glFramebufferTexture");
      return NULL;
   }

   _mesa_init_renderbuffer(&irb->Base, name);
   irb->Base.ClassID = INTEL_RB_CLASS;

   if (texImage->TexFormat == &_mesa_texformat_argb8888) {
      irb->Base._ActualFormat = GL_RGBA8;
      irb->Base._BaseFormat = GL_RGBA;
      DBG("Render to RGBA8 texture OK\n");
   }
   else if (texImage->TexFormat == &_mesa_texformat_rgb565) {
      irb->Base._ActualFormat = GL_RGB5;
      irb->Base._BaseFormat = GL_RGB;
      DBG("Render to RGB5 texture OK\n");
   }
   else if (texImage->TexFormat == &_mesa_texformat_z16) {
      irb->Base._ActualFormat = GL_DEPTH_COMPONENT16;
      irb->Base._BaseFormat = GL_DEPTH_COMPONENT;
      DBG("Render to DEPTH16 texture OK\n");
   }
   else {
      DBG("Render to texture BAD FORMAT %d\n",
	  texImage->TexFormat->MesaFormat);
      _mesa_free(irb);
      return NULL;
   }

   irb->Base.InternalFormat = irb->Base._ActualFormat;
   irb->Base.Width = texImage->Width;
   irb->Base.Height = texImage->Height;
   irb->Base.DataType = GL_UNSIGNED_BYTE;       /* FBO XXX fix */
   irb->Base.RedBits = texImage->TexFormat->RedBits;
   irb->Base.GreenBits = texImage->TexFormat->GreenBits;
   irb->Base.BlueBits = texImage->TexFormat->BlueBits;
   irb->Base.AlphaBits = texImage->TexFormat->AlphaBits;
   irb->Base.DepthBits = texImage->TexFormat->DepthBits;

   irb->Base.Delete = intel_delete_renderbuffer;
   irb->Base.AllocStorage = intel_nop_alloc_storage;
   intel_set_span_functions(&irb->Base);

   irb->RenderToTexture = GL_TRUE;

   return irb;
}


/**
 * Called by glFramebufferTexture[123]DEXT() (and other places) to
 * prepare for rendering into texture memory.  This might be called
 * many times to choose different texture levels, cube faces, etc
 * before intel_finish_render_texture() is ever called.
 */
static void
intel_render_texture(GLcontext * ctx,
                     struct gl_framebuffer *fb,
                     struct gl_renderbuffer_attachment *att)
{
   struct gl_texture_image *newImage
      = att->Texture->Image[att->CubeMapFace][att->TextureLevel];
   struct intel_renderbuffer *irb = intel_renderbuffer(att->Renderbuffer);
   struct intel_texture_image *intel_image;
   GLuint imageOffset;

   (void) fb;

   ASSERT(newImage);

   if (!irb) {
      irb = intel_wrap_texture(ctx, newImage);
      if (irb) {
         /* bind the wrapper to the attachment point */
         _mesa_reference_renderbuffer(&att->Renderbuffer, &irb->Base);
      }
      else {
         /* fallback to software rendering */
         _mesa_render_texture(ctx, fb, att);
         return;
      }
   }

   DBG("Begin render texture tid %x tex=%u w=%d h=%d refcount=%d\n",
       _glthread_GetID(),
       att->Texture->Name, newImage->Width, newImage->Height,
       irb->Base.RefCount);

   /* point the renderbufer's region to the texture image region */
   intel_image = intel_texture_image(newImage);
   if (irb->region != intel_image->mt->region) {
      if (irb->region)
	 intel_region_release(&irb->region);
      intel_region_reference(&irb->region, intel_image->mt->region);
   }

   /* compute offset of the particular 2D image within the texture region */
   imageOffset = intel_miptree_image_offset(intel_image->mt,
                                            att->CubeMapFace,
                                            att->TextureLevel);

   if (att->Texture->Target == GL_TEXTURE_3D) {
      const GLuint *offsets = intel_miptree_depth_offsets(intel_image->mt,
                                                          att->TextureLevel);
      imageOffset += offsets[att->Zoffset];
   }

   /* store that offset in the region */
   intel_image->mt->region->draw_offset = imageOffset;

   /* update drawing region, etc */
   intel_draw_buffer(ctx, fb);
}


/**
 * Called by Mesa when rendering to a texture is done.
 */
static void
intel_finish_render_texture(GLcontext * ctx,
                            struct gl_renderbuffer_attachment *att)
{
   struct intel_renderbuffer *irb = intel_renderbuffer(att->Renderbuffer);

   DBG("End render texture (tid %x) tex %u\n", _glthread_GetID(), att->Texture->Name);

   if (irb) {
      /* just release the region */
      intel_region_release(&irb->region);
   }
   else if (att->Renderbuffer) {
      /* software fallback */
      _mesa_finish_render_texture(ctx, att);
      /* XXX FBO: Need to unmap the buffer (or in intelSpanRenderStart???) */
   }
}


/**
 * Do one-time context initializations related to GL_EXT_framebuffer_object.
 * Hook in device driver functions.
 */
void
intel_fbo_init(struct intel_context *intel)
{
   intel->ctx.Driver.NewFramebuffer = intel_new_framebuffer;
   intel->ctx.Driver.NewRenderbuffer = intel_new_renderbuffer;
   intel->ctx.Driver.BindFramebuffer = intel_bind_framebuffer;
   intel->ctx.Driver.FramebufferRenderbuffer = intel_framebuffer_renderbuffer;
   intel->ctx.Driver.RenderTexture = intel_render_texture;
   intel->ctx.Driver.FinishRenderTexture = intel_finish_render_texture;
   intel->ctx.Driver.ResizeBuffers = intel_resize_buffers;
}
