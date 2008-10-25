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


/*
 * Authors:
 *   Brian Paul
 */


#include "buffers.h"
#include "context.h"
#include "fbobject.h"
#include "framebuffer.h"
#include "hash.h"
#include "mipmap.h"
#include "renderbuffer.h"
#include "state.h"
#include "teximage.h"
#include "texobj.h"
#include "texstore.h"


/**
 * Notes:
 *
 * None of the GL_EXT_framebuffer_object functions are compiled into
 * display lists.
 */



/*
 * When glGenRender/FramebuffersEXT() is called we insert pointers to
 * these placeholder objects into the hash table.
 * Later, when the object ID is first bound, we replace the placeholder
 * with the real frame/renderbuffer.
 */
static struct gl_framebuffer DummyFramebuffer;
static struct gl_renderbuffer DummyRenderbuffer;


#define IS_CUBE_FACE(TARGET) \
   ((TARGET) >= GL_TEXTURE_CUBE_MAP_POSITIVE_X && \
    (TARGET) <= GL_TEXTURE_CUBE_MAP_NEGATIVE_Z)


static void
delete_dummy_renderbuffer(struct gl_renderbuffer *rb)
{
   /* no op */
}

static void
delete_dummy_framebuffer(struct gl_framebuffer *fb)
{
   /* no op */
}


void
_mesa_init_fbobjects(GLcontext *ctx)
{
   DummyFramebuffer.Delete = delete_dummy_framebuffer;
   DummyRenderbuffer.Delete = delete_dummy_renderbuffer;
}


/**
 * Helper routine for getting a gl_renderbuffer.
 */
struct gl_renderbuffer *
_mesa_lookup_renderbuffer(GLcontext *ctx, GLuint id)
{
   struct gl_renderbuffer *rb;

   if (id == 0)
      return NULL;

   rb = (struct gl_renderbuffer *)
      _mesa_HashLookup(ctx->Shared->RenderBuffers, id);
   return rb;
}


/**
 * Helper routine for getting a gl_framebuffer.
 */
struct gl_framebuffer *
_mesa_lookup_framebuffer(GLcontext *ctx, GLuint id)
{
   struct gl_framebuffer *fb;

   if (id == 0)
      return NULL;

   fb = (struct gl_framebuffer *)
      _mesa_HashLookup(ctx->Shared->FrameBuffers, id);
   return fb;
}


/**
 * Given a GL_*_ATTACHMENTn token, return a pointer to the corresponding
 * gl_renderbuffer_attachment object.
 */
struct gl_renderbuffer_attachment *
_mesa_get_attachment(GLcontext *ctx, struct gl_framebuffer *fb,
                     GLenum attachment)
{
   GLuint i;

   switch (attachment) {
   case GL_COLOR_ATTACHMENT0_EXT:
   case GL_COLOR_ATTACHMENT1_EXT:
   case GL_COLOR_ATTACHMENT2_EXT:
   case GL_COLOR_ATTACHMENT3_EXT:
   case GL_COLOR_ATTACHMENT4_EXT:
   case GL_COLOR_ATTACHMENT5_EXT:
   case GL_COLOR_ATTACHMENT6_EXT:
   case GL_COLOR_ATTACHMENT7_EXT:
   case GL_COLOR_ATTACHMENT8_EXT:
   case GL_COLOR_ATTACHMENT9_EXT:
   case GL_COLOR_ATTACHMENT10_EXT:
   case GL_COLOR_ATTACHMENT11_EXT:
   case GL_COLOR_ATTACHMENT12_EXT:
   case GL_COLOR_ATTACHMENT13_EXT:
   case GL_COLOR_ATTACHMENT14_EXT:
   case GL_COLOR_ATTACHMENT15_EXT:
      i = attachment - GL_COLOR_ATTACHMENT0_EXT;
      if (i >= ctx->Const.MaxColorAttachments) {
	 return NULL;
      }
      return &fb->Attachment[BUFFER_COLOR0 + i];
   case GL_DEPTH_ATTACHMENT_EXT:
      return &fb->Attachment[BUFFER_DEPTH];
   case GL_STENCIL_ATTACHMENT_EXT:
      return &fb->Attachment[BUFFER_STENCIL];
   default:
      return NULL;
   }
}


/**
 * Remove any texture or renderbuffer attached to the given attachment
 * point.  Update reference counts, etc.
 */
void
_mesa_remove_attachment(GLcontext *ctx, struct gl_renderbuffer_attachment *att)
{
   if (att->Type == GL_TEXTURE) {
      ASSERT(att->Texture);
      if (ctx->Driver.FinishRenderTexture) {
         /* tell driver we're done rendering to this texobj */
         ctx->Driver.FinishRenderTexture(ctx, att);
      }
      _mesa_reference_texobj(&att->Texture, NULL); /* unbind */
      ASSERT(!att->Texture);
   }
   if (att->Type == GL_TEXTURE || att->Type == GL_RENDERBUFFER_EXT) {
      ASSERT(!att->Texture);
      _mesa_reference_renderbuffer(&att->Renderbuffer, NULL); /* unbind */
      ASSERT(!att->Renderbuffer);
   }
   att->Type = GL_NONE;
   att->Complete = GL_TRUE;
}


/**
 * Bind a texture object to an attachment point.
 * The previous binding, if any, will be removed first.
 */
void
_mesa_set_texture_attachment(GLcontext *ctx,
                             struct gl_framebuffer *fb,
                             struct gl_renderbuffer_attachment *att,
                             struct gl_texture_object *texObj,
                             GLenum texTarget, GLuint level, GLuint zoffset)
{
   if (att->Texture == texObj) {
      /* re-attaching same texture */
      ASSERT(att->Type == GL_TEXTURE);
   }
   else {
      /* new attachment */
      _mesa_remove_attachment(ctx, att);
      att->Type = GL_TEXTURE;
      assert(!att->Texture);
      _mesa_reference_texobj(&att->Texture, texObj);
   }

   /* always update these fields */
   att->TextureLevel = level;
   if (IS_CUBE_FACE(texTarget)) {
      att->CubeMapFace = texTarget - GL_TEXTURE_CUBE_MAP_POSITIVE_X;
   }
   else {
      att->CubeMapFace = 0;
   }
   att->Zoffset = zoffset;
   att->Complete = GL_FALSE;

   if (att->Texture->Image[att->CubeMapFace][att->TextureLevel]) {
      ctx->Driver.RenderTexture(ctx, fb, att);
   }
}


/**
 * Bind a renderbuffer to an attachment point.
 * The previous binding, if any, will be removed first.
 */
void
_mesa_set_renderbuffer_attachment(GLcontext *ctx,
                                  struct gl_renderbuffer_attachment *att,
                                  struct gl_renderbuffer *rb)
{
   /* XXX check if re-doing same attachment, exit early */
   _mesa_remove_attachment(ctx, att);
   att->Type = GL_RENDERBUFFER_EXT;
   att->Texture = NULL; /* just to be safe */
   att->Complete = GL_FALSE;
   _mesa_reference_renderbuffer(&att->Renderbuffer, rb);
}


/**
 * Fallback for ctx->Driver.FramebufferRenderbuffer()
 * Attach a renderbuffer object to a framebuffer object.
 */
void
_mesa_framebuffer_renderbuffer(GLcontext *ctx, struct gl_framebuffer *fb,
                               GLenum attachment, struct gl_renderbuffer *rb)
{
   struct gl_renderbuffer_attachment *att;

   _glthread_LOCK_MUTEX(fb->Mutex);

   att = _mesa_get_attachment(ctx, fb, attachment);
   ASSERT(att);
   if (rb) {
      _mesa_set_renderbuffer_attachment(ctx, att, rb);
   }
   else {
      _mesa_remove_attachment(ctx, att);
   }

   _glthread_UNLOCK_MUTEX(fb->Mutex);
}


/**
 * Test if an attachment point is complete and update its Complete field.
 * \param format if GL_COLOR, this is a color attachment point,
 *               if GL_DEPTH, this is a depth component attachment point,
 *               if GL_STENCIL, this is a stencil component attachment point.
 */
static void
test_attachment_completeness(const GLcontext *ctx, GLenum format,
                             struct gl_renderbuffer_attachment *att)
{
   assert(format == GL_COLOR || format == GL_DEPTH || format == GL_STENCIL);

   /* assume complete */
   att->Complete = GL_TRUE;

   /* Look for reasons why the attachment might be incomplete */
   if (att->Type == GL_TEXTURE) {
      const struct gl_texture_object *texObj = att->Texture;
      struct gl_texture_image *texImage;

      if (!texObj) {
         att->Complete = GL_FALSE;
         return;
      }

      texImage = texObj->Image[att->CubeMapFace][att->TextureLevel];
      if (!texImage) {
         att->Complete = GL_FALSE;
         return;
      }
      if (texImage->Width < 1 || texImage->Height < 1) {
         att->Complete = GL_FALSE;
         return;
      }
      if (texObj->Target == GL_TEXTURE_3D && att->Zoffset >= texImage->Depth) {
         att->Complete = GL_FALSE;
         return;
      }

      if (format == GL_COLOR) {
         if (texImage->TexFormat->BaseFormat != GL_RGB &&
             texImage->TexFormat->BaseFormat != GL_RGBA) {
            att->Complete = GL_FALSE;
            return;
         }
      }
      else if (format == GL_DEPTH) {
         if (texImage->TexFormat->BaseFormat == GL_DEPTH_COMPONENT) {
            /* OK */
         }
         else if (ctx->Extensions.EXT_packed_depth_stencil &&
                  texImage->TexFormat->BaseFormat == GL_DEPTH_STENCIL_EXT) {
            /* OK */
         }
         else {
            att->Complete = GL_FALSE;
            return;
         }
      }
      else {
         /* no such thing as stencil textures */
         att->Complete = GL_FALSE;
         return;
      }
   }
   else if (att->Type == GL_RENDERBUFFER_EXT) {
      ASSERT(att->Renderbuffer);
      if (!att->Renderbuffer->InternalFormat ||
          att->Renderbuffer->Width < 1 ||
          att->Renderbuffer->Height < 1) {
         att->Complete = GL_FALSE;
         return;
      }
      if (format == GL_COLOR) {
         if (att->Renderbuffer->_BaseFormat != GL_RGB &&
             att->Renderbuffer->_BaseFormat != GL_RGBA) {
            ASSERT(att->Renderbuffer->RedBits);
            ASSERT(att->Renderbuffer->GreenBits);
            ASSERT(att->Renderbuffer->BlueBits);
            att->Complete = GL_FALSE;
            return;
         }
      }
      else if (format == GL_DEPTH) {
         ASSERT(att->Renderbuffer->DepthBits);
         if (att->Renderbuffer->_BaseFormat == GL_DEPTH_COMPONENT) {
            /* OK */
         }
         else if (ctx->Extensions.EXT_packed_depth_stencil &&
                  att->Renderbuffer->_BaseFormat == GL_DEPTH_STENCIL_EXT) {
            /* OK */
         }
         else {
            att->Complete = GL_FALSE;
            return;
         }
      }
      else {
         assert(format == GL_STENCIL);
         ASSERT(att->Renderbuffer->StencilBits);
         if (att->Renderbuffer->_BaseFormat == GL_STENCIL_INDEX) {
            /* OK */
         }
         else if (ctx->Extensions.EXT_packed_depth_stencil &&
                  att->Renderbuffer->_BaseFormat == GL_DEPTH_STENCIL_EXT) {
            /* OK */
         }
         else {
            att->Complete = GL_FALSE;
            return;
         }
      }
   }
   else {
      ASSERT(att->Type == GL_NONE);
      /* complete */
      return;
   }
}


/**
 * Helpful for debugging
 */
static void
fbo_incomplete(const char *msg, int index)
{
   (void) msg;
   (void) index;
   /*
   _mesa_debug(NULL, "FBO Incomplete: %s [%d]\n", msg, index);
   */
}


/**
 * Test if the given framebuffer object is complete and update its
 * Status field with the results.
 * Also update the framebuffer's Width and Height fields if the
 * framebuffer is complete.
 */
void
_mesa_test_framebuffer_completeness(GLcontext *ctx, struct gl_framebuffer *fb)
{
   GLuint numImages, width = 0, height = 0;
   GLenum intFormat = GL_NONE;
   GLuint w = 0, h = 0;
   GLint i;
   GLuint j;

   assert(fb->Name != 0);

   numImages = 0;
   fb->Width = 0;
   fb->Height = 0;

   /* Start at -2 to more easily loop over all attachment points */
   for (i = -2; i < (GLint) ctx->Const.MaxColorAttachments; i++) {
      struct gl_renderbuffer_attachment *att;
      GLenum f;

      if (i == -2) {
         att = &fb->Attachment[BUFFER_DEPTH];
         test_attachment_completeness(ctx, GL_DEPTH, att);
         if (!att->Complete) {
            fb->_Status = GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_EXT;
            fbo_incomplete("depth attachment incomplete", -1);
            return;
         }
      }
      else if (i == -1) {
         att = &fb->Attachment[BUFFER_STENCIL];
         test_attachment_completeness(ctx, GL_STENCIL, att);
         if (!att->Complete) {
            fb->_Status = GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_EXT;
            fbo_incomplete("stencil attachment incomplete", -1);
            return;
         }
      }
      else {
         att = &fb->Attachment[BUFFER_COLOR0 + i];
         test_attachment_completeness(ctx, GL_COLOR, att);
         if (!att->Complete) {
            fb->_Status = GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_EXT;
            fbo_incomplete("color attachment incomplete", i);
            return;
         }
      }

      if (att->Type == GL_TEXTURE) {
         const struct gl_texture_image *texImg
            = att->Texture->Image[att->CubeMapFace][att->TextureLevel];
         w = texImg->Width;
         h = texImg->Height;
         f = texImg->_BaseFormat;
         numImages++;
         if (f != GL_RGB && f != GL_RGBA && f != GL_DEPTH_COMPONENT
             && f != GL_DEPTH_STENCIL_EXT) {
            fb->_Status = GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT;
            fbo_incomplete("texture attachment incomplete", -1);
            return;
         }
      }
      else if (att->Type == GL_RENDERBUFFER_EXT) {
         w = att->Renderbuffer->Width;
         h = att->Renderbuffer->Height;
         f = att->Renderbuffer->InternalFormat;
         numImages++;
      }
      else {
         assert(att->Type == GL_NONE);
         continue;
      }

      if (numImages == 1) {
         /* set required width, height and format */
         width = w;
         height = h;
         if (i >= 0)
            intFormat = f;
      }
      else {
         /* check that width, height, format are same */
         if (w != width || h != height) {
            fb->_Status = GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT;
            fbo_incomplete("width or height mismatch", -1);
            return;
         }
         if (intFormat != GL_NONE && f != intFormat) {
            fb->_Status = GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT;
            fbo_incomplete("format mismatch", -1);
            return;
         }
      }
   }

   /* Check that all DrawBuffers are present */
   for (j = 0; j < ctx->Const.MaxDrawBuffers; j++) {
      if (fb->ColorDrawBuffer[j] != GL_NONE) {
         const struct gl_renderbuffer_attachment *att
            = _mesa_get_attachment(ctx, fb, fb->ColorDrawBuffer[j]);
         assert(att);
         if (att->Type == GL_NONE) {
            fb->_Status = GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER_EXT;
            fbo_incomplete("missing drawbuffer", j);
            return;
         }
      }
   }

   /* Check that the ReadBuffer is present */
   if (fb->ColorReadBuffer != GL_NONE) {
      const struct gl_renderbuffer_attachment *att
         = _mesa_get_attachment(ctx, fb, fb->ColorReadBuffer);
      assert(att);
      if (att->Type == GL_NONE) {
         fb->_Status = GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER_EXT;
            fbo_incomplete("missing readbuffer", -1);
         return;
      }
   }

   if (numImages == 0) {
      fb->_Status = GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_EXT;
      fbo_incomplete("no attachments", -1);
      return;
   }

   /*
    * If we get here, the framebuffer is complete!
    */
   fb->_Status = GL_FRAMEBUFFER_COMPLETE_EXT;
   fb->Width = w;
   fb->Height = h;
}


GLboolean GLAPIENTRY
_mesa_IsRenderbufferEXT(GLuint renderbuffer)
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_WITH_RETVAL(ctx, GL_FALSE);
   if (renderbuffer) {
      struct gl_renderbuffer *rb = _mesa_lookup_renderbuffer(ctx, renderbuffer);
      if (rb != NULL && rb != &DummyRenderbuffer)
         return GL_TRUE;
   }
   return GL_FALSE;
}


void GLAPIENTRY
_mesa_BindRenderbufferEXT(GLenum target, GLuint renderbuffer)
{
   struct gl_renderbuffer *newRb;
   GET_CURRENT_CONTEXT(ctx);

   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (target != GL_RENDERBUFFER_EXT) {
         _mesa_error(ctx, GL_INVALID_ENUM,
                  "glBindRenderbufferEXT(target)");
      return;
   }

   FLUSH_VERTICES(ctx, _NEW_BUFFERS);
   /* The above doesn't fully flush the drivers in the way that a
    * glFlush does, but that is required here:
    */
   if (ctx->Driver.Flush)
      ctx->Driver.Flush(ctx);


   if (renderbuffer) {
      newRb = _mesa_lookup_renderbuffer(ctx, renderbuffer);
      if (newRb == &DummyRenderbuffer) {
         /* ID was reserved, but no real renderbuffer object made yet */
         newRb = NULL;
      }
      if (!newRb) {
	 /* create new renderbuffer object */
	 newRb = ctx->Driver.NewRenderbuffer(ctx, renderbuffer);
	 if (!newRb) {
	    _mesa_error(ctx, GL_OUT_OF_MEMORY, "glBindRenderbufferEXT");
	    return;
	 }
         ASSERT(newRb->AllocStorage);
         _mesa_HashInsert(ctx->Shared->RenderBuffers, renderbuffer, newRb);
         newRb->RefCount = 1; /* referenced by hash table */
      }
   }
   else {
      newRb = NULL;
   }

   ASSERT(newRb != &DummyRenderbuffer);

   _mesa_reference_renderbuffer(&ctx->CurrentRenderbuffer, newRb);
}


void GLAPIENTRY
_mesa_DeleteRenderbuffersEXT(GLsizei n, const GLuint *renderbuffers)
{
   GLint i;
   GET_CURRENT_CONTEXT(ctx);

   ASSERT_OUTSIDE_BEGIN_END(ctx);
   FLUSH_VERTICES(ctx, _NEW_BUFFERS);

   for (i = 0; i < n; i++) {
      if (renderbuffers[i] > 0) {
	 struct gl_renderbuffer *rb;
	 rb = _mesa_lookup_renderbuffer(ctx, renderbuffers[i]);
	 if (rb) {
            /* check if deleting currently bound renderbuffer object */
            if (rb == ctx->CurrentRenderbuffer) {
               /* bind default */
               ASSERT(rb->RefCount >= 2);
               _mesa_BindRenderbufferEXT(GL_RENDERBUFFER_EXT, 0);
            }

	    /* Remove from hash table immediately, to free the ID.
             * But the object will not be freed until it's no longer
             * referenced anywhere else.
             */
	    _mesa_HashRemove(ctx->Shared->RenderBuffers, renderbuffers[i]);

            if (rb != &DummyRenderbuffer) {
               /* no longer referenced by hash table */
               _mesa_reference_renderbuffer(&rb, NULL);
	    }
	 }
      }
   }
}


void GLAPIENTRY
_mesa_GenRenderbuffersEXT(GLsizei n, GLuint *renderbuffers)
{
   GET_CURRENT_CONTEXT(ctx);
   GLuint first;
   GLint i;

   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (n < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glGenRenderbuffersEXT(n)");
      return;
   }

   if (!renderbuffers)
      return;

   first = _mesa_HashFindFreeKeyBlock(ctx->Shared->RenderBuffers, n);

   for (i = 0; i < n; i++) {
      GLuint name = first + i;
      renderbuffers[i] = name;
      /* insert dummy placeholder into hash table */
      _glthread_LOCK_MUTEX(ctx->Shared->Mutex);
      _mesa_HashInsert(ctx->Shared->RenderBuffers, name, &DummyRenderbuffer);
      _glthread_UNLOCK_MUTEX(ctx->Shared->Mutex);
   }
}


/**
 * Given an internal format token for a render buffer, return the
 * corresponding base format.
 * This is very similar to _mesa_base_tex_format() but the set of valid
 * internal formats is somewhat different.
 *
 * \return one of GL_RGB, GL_RGBA, GL_STENCIL_INDEX, GL_DEPTH_COMPONENT
 *  GL_DEPTH_STENCIL_EXT or zero if error.
 */
GLenum
_mesa_base_fbo_format(GLcontext *ctx, GLenum internalFormat)
{
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
   case GL_DEPTH_STENCIL_EXT:
   case GL_DEPTH24_STENCIL8_EXT:
      if (ctx->Extensions.EXT_packed_depth_stencil)
         return GL_DEPTH_STENCIL_EXT;
      else
         return 0;
   /* XXX add floating point formats eventually */
   default:
      return 0;
   }
}


void GLAPIENTRY
_mesa_RenderbufferStorageEXT(GLenum target, GLenum internalFormat,
                             GLsizei width, GLsizei height)
{
   struct gl_renderbuffer *rb;
   GLenum baseFormat;
   GET_CURRENT_CONTEXT(ctx);

   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (target != GL_RENDERBUFFER_EXT) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glRenderbufferStorageEXT(target)");
      return;
   }

   baseFormat = _mesa_base_fbo_format(ctx, internalFormat);
   if (baseFormat == 0) {
      _mesa_error(ctx, GL_INVALID_ENUM,
                  "glRenderbufferStorageEXT(internalFormat)");
      return;
   }

   if (width < 1 || width > (GLsizei) ctx->Const.MaxRenderbufferSize) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glRenderbufferStorageEXT(width)");
      return;
   }

   if (height < 1 || height > (GLsizei) ctx->Const.MaxRenderbufferSize) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glRenderbufferStorageEXT(height)");
      return;
   }

   rb = ctx->CurrentRenderbuffer;

   if (!rb) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glRenderbufferStorageEXT");
      return;
   }

   FLUSH_VERTICES(ctx, _NEW_BUFFERS);

   if (rb->InternalFormat == internalFormat &&
       rb->Width == (GLuint) width &&
       rb->Height == (GLuint) height) {
      /* no change in allocation needed */
      return;
   }

   /* These MUST get set by the AllocStorage func */
   rb->_ActualFormat = 0;
   rb->RedBits =
   rb->GreenBits =
   rb->BlueBits =
   rb->AlphaBits =
   rb->IndexBits =
   rb->DepthBits =
   rb->StencilBits = 0;

   /* Now allocate the storage */
   ASSERT(rb->AllocStorage);
   if (rb->AllocStorage(ctx, rb, internalFormat, width, height)) {
      /* No error - check/set fields now */
      assert(rb->_ActualFormat);
      assert(rb->Width == (GLuint) width);
      assert(rb->Height == (GLuint) height);
      assert(rb->RedBits || rb->GreenBits || rb->BlueBits || rb->AlphaBits ||
             rb->DepthBits || rb->StencilBits || rb->IndexBits);
      rb->InternalFormat = internalFormat;
      rb->_BaseFormat = baseFormat;
   }
   else {
      /* Probably ran out of memory - clear the fields */
      rb->Width = 0;
      rb->Height = 0;
      rb->InternalFormat = GL_NONE;
      rb->_ActualFormat = GL_NONE;
      rb->_BaseFormat = GL_NONE;
      rb->RedBits =
      rb->GreenBits =
      rb->BlueBits =
      rb->AlphaBits =
      rb->IndexBits =
      rb->DepthBits =
      rb->StencilBits = 0;
   }

   /*
   test_framebuffer_completeness(ctx, fb);
   */
   /* XXX if this renderbuffer is attached anywhere, invalidate attachment
    * points???
    */
}


void GLAPIENTRY
_mesa_GetRenderbufferParameterivEXT(GLenum target, GLenum pname, GLint *params)
{
   GET_CURRENT_CONTEXT(ctx);

   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (target != GL_RENDERBUFFER_EXT) {
      _mesa_error(ctx, GL_INVALID_ENUM,
                  "glGetRenderbufferParameterivEXT(target)");
      return;
   }

   if (!ctx->CurrentRenderbuffer) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glGetRenderbufferParameterivEXT");
      return;
   }

   FLUSH_VERTICES(ctx, _NEW_BUFFERS);

   switch (pname) {
   case GL_RENDERBUFFER_WIDTH_EXT:
      *params = ctx->CurrentRenderbuffer->Width;
      return;
   case GL_RENDERBUFFER_HEIGHT_EXT:
      *params = ctx->CurrentRenderbuffer->Height;
      return;
   case GL_RENDERBUFFER_INTERNAL_FORMAT_EXT:
      *params = ctx->CurrentRenderbuffer->InternalFormat;
      return;
   case GL_RENDERBUFFER_RED_SIZE_EXT:
      *params = ctx->CurrentRenderbuffer->RedBits;
      break;
   case GL_RENDERBUFFER_GREEN_SIZE_EXT:
      *params = ctx->CurrentRenderbuffer->GreenBits;
      break;
   case GL_RENDERBUFFER_BLUE_SIZE_EXT:
      *params = ctx->CurrentRenderbuffer->BlueBits;
      break;
   case GL_RENDERBUFFER_ALPHA_SIZE_EXT:
      *params = ctx->CurrentRenderbuffer->AlphaBits;
      break;
   case GL_RENDERBUFFER_DEPTH_SIZE_EXT:
      *params = ctx->CurrentRenderbuffer->DepthBits;
      break;
   case GL_RENDERBUFFER_STENCIL_SIZE_EXT:
      *params = ctx->CurrentRenderbuffer->StencilBits;
      break;
   default:
      _mesa_error(ctx, GL_INVALID_ENUM,
                  "glGetRenderbufferParameterivEXT(target)");
      return;
   }
}


GLboolean GLAPIENTRY
_mesa_IsFramebufferEXT(GLuint framebuffer)
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_WITH_RETVAL(ctx, GL_FALSE);
   if (framebuffer) {
      struct gl_framebuffer *rb = _mesa_lookup_framebuffer(ctx, framebuffer);
      if (rb != NULL && rb != &DummyFramebuffer)
         return GL_TRUE;
   }
   return GL_FALSE;
}


static void
check_begin_texture_render(GLcontext *ctx, struct gl_framebuffer *fb)
{
   GLuint i;
   ASSERT(ctx->Driver.RenderTexture);
   for (i = 0; i < BUFFER_COUNT; i++) {
      struct gl_renderbuffer_attachment *att = fb->Attachment + i;
      struct gl_texture_object *texObj = att->Texture;
      if (texObj
          && att->Texture->Image[att->CubeMapFace][att->TextureLevel]) {
         ctx->Driver.RenderTexture(ctx, fb, att);
      }
   }
}


/**
 * Examine all the framebuffer's attachments to see if any are textures.
 * If so, call ctx->Driver.FinishRenderTexture() for each texture to
 * notify the device driver that the texture image may have changed.
 */
static void
check_end_texture_render(GLcontext *ctx, struct gl_framebuffer *fb)
{
   if (ctx->Driver.FinishRenderTexture) {
      GLuint i;
      for (i = 0; i < BUFFER_COUNT; i++) {
         struct gl_renderbuffer_attachment *att = fb->Attachment + i;
         if (att->Texture && att->Renderbuffer) {
            ctx->Driver.FinishRenderTexture(ctx, att);
         }
      }
   }
}


void GLAPIENTRY
_mesa_BindFramebufferEXT(GLenum target, GLuint framebuffer)
{
   struct gl_framebuffer *newFb, *newFbread;
   GLboolean bindReadBuf, bindDrawBuf;
   GET_CURRENT_CONTEXT(ctx);

   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (!ctx->Extensions.EXT_framebuffer_object) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glBindFramebufferEXT(unsupported)");
      return;
   }

   switch (target) {
#if FEATURE_EXT_framebuffer_blit
   case GL_DRAW_FRAMEBUFFER_EXT:
      if (!ctx->Extensions.EXT_framebuffer_blit) {
         _mesa_error(ctx, GL_INVALID_ENUM, "glBindFramebufferEXT(target)");
         return;
      }
      bindDrawBuf = GL_TRUE;
      bindReadBuf = GL_FALSE;
      break;
   case GL_READ_FRAMEBUFFER_EXT:
      if (!ctx->Extensions.EXT_framebuffer_blit) {
         _mesa_error(ctx, GL_INVALID_ENUM, "glBindFramebufferEXT(target)");
         return;
      }
      bindDrawBuf = GL_FALSE;
      bindReadBuf = GL_TRUE;
      break;
#endif
   case GL_FRAMEBUFFER_EXT:
      bindDrawBuf = GL_TRUE;
      bindReadBuf = GL_TRUE;
      break;
   default:
      _mesa_error(ctx, GL_INVALID_ENUM, "glBindFramebufferEXT(target)");
      return;
   }

   FLUSH_VERTICES(ctx, _NEW_BUFFERS);

   if (ctx->Driver.Flush) {  
      ctx->Driver.Flush(ctx);
   }

   if (framebuffer) {
      /* Binding a user-created framebuffer object */
      newFb = _mesa_lookup_framebuffer(ctx, framebuffer);
      if (newFb == &DummyFramebuffer) {
         /* ID was reserved, but no real framebuffer object made yet */
         newFb = NULL;
      }
      if (!newFb) {
	 /* create new framebuffer object */
	 newFb = ctx->Driver.NewFramebuffer(ctx, framebuffer);
	 if (!newFb) {
	    _mesa_error(ctx, GL_OUT_OF_MEMORY, "glBindFramebufferEXT");
	    return;
	 }
         _mesa_HashInsert(ctx->Shared->FrameBuffers, framebuffer, newFb);
      }
      newFbread = newFb;
   }
   else {
      /* Binding the window system framebuffer (which was originally set
       * with MakeCurrent).
       */
      newFb = ctx->WinSysDrawBuffer;
      newFbread = ctx->WinSysReadBuffer;
   }

   ASSERT(newFb);
   ASSERT(newFb != &DummyFramebuffer);

   /*
    * XXX check if re-binding same buffer and skip some of this code.
    */

   if (bindReadBuf) {
      _mesa_reference_framebuffer(&ctx->ReadBuffer, newFbread);
   }

   if (bindDrawBuf) {
      /* check if old FB had any texture attachments */
      check_end_texture_render(ctx, ctx->DrawBuffer);

      /* check if time to delete this framebuffer */
      _mesa_reference_framebuffer(&ctx->DrawBuffer, newFb);

      if (newFb->Name != 0) {
         /* check if newly bound framebuffer has any texture attachments */
         check_begin_texture_render(ctx, newFb);
      }
   }

   if (ctx->Driver.BindFramebuffer) {
      ctx->Driver.BindFramebuffer(ctx, target, newFb, newFbread);
   }
}


void GLAPIENTRY
_mesa_DeleteFramebuffersEXT(GLsizei n, const GLuint *framebuffers)
{
   GLint i;
   GET_CURRENT_CONTEXT(ctx);

   ASSERT_OUTSIDE_BEGIN_END(ctx);
   FLUSH_VERTICES(ctx, _NEW_BUFFERS);
   /* The above doesn't fully flush the drivers in the way that a
    * glFlush does, but that is required here:
    */
   if (ctx->Driver.Flush)
      ctx->Driver.Flush(ctx);

   for (i = 0; i < n; i++) {
      if (framebuffers[i] > 0) {
	 struct gl_framebuffer *fb;
	 fb = _mesa_lookup_framebuffer(ctx, framebuffers[i]);
	 if (fb) {
            ASSERT(fb == &DummyFramebuffer || fb->Name == framebuffers[i]);

            /* check if deleting currently bound framebuffer object */
            if (fb == ctx->DrawBuffer) {
               /* bind default */
               ASSERT(fb->RefCount >= 2);
               _mesa_BindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
            }

	    /* remove from hash table immediately, to free the ID */
	    _mesa_HashRemove(ctx->Shared->FrameBuffers, framebuffers[i]);

            if (fb != &DummyFramebuffer) {
               /* But the object will not be freed until it's no longer
                * bound in any context.
                */
               _mesa_unreference_framebuffer(&fb);
	    }
	 }
      }
   }
}


void GLAPIENTRY
_mesa_GenFramebuffersEXT(GLsizei n, GLuint *framebuffers)
{
   GET_CURRENT_CONTEXT(ctx);
   GLuint first;
   GLint i;

   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (n < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glGenFramebuffersEXT(n)");
      return;
   }

   if (!framebuffers)
      return;

   first = _mesa_HashFindFreeKeyBlock(ctx->Shared->FrameBuffers, n);

   for (i = 0; i < n; i++) {
      GLuint name = first + i;
      framebuffers[i] = name;
      /* insert dummy placeholder into hash table */
      _glthread_LOCK_MUTEX(ctx->Shared->Mutex);
      _mesa_HashInsert(ctx->Shared->FrameBuffers, name, &DummyFramebuffer);
      _glthread_UNLOCK_MUTEX(ctx->Shared->Mutex);
   }
}



GLenum GLAPIENTRY
_mesa_CheckFramebufferStatusEXT(GLenum target)
{
   struct gl_framebuffer *buffer;
   GET_CURRENT_CONTEXT(ctx);

   ASSERT_OUTSIDE_BEGIN_END_WITH_RETVAL(ctx, 0);

   switch (target) {
#if FEATURE_EXT_framebuffer_blit
   case GL_DRAW_FRAMEBUFFER_EXT:
      if (!ctx->Extensions.EXT_framebuffer_blit) {
         _mesa_error(ctx, GL_INVALID_ENUM, "glCheckFramebufferStatus(target)");
         return 0;
      }
      buffer = ctx->DrawBuffer;
      break;
   case GL_READ_FRAMEBUFFER_EXT:
      if (!ctx->Extensions.EXT_framebuffer_blit) {
         _mesa_error(ctx, GL_INVALID_ENUM, "glCheckFramebufferStatus(target)");
         return 0;
      }
      buffer = ctx->ReadBuffer;
      break;
#endif
   case GL_FRAMEBUFFER_EXT:
      buffer = ctx->DrawBuffer;
      break;
   default:
      _mesa_error(ctx, GL_INVALID_ENUM, "glCheckFramebufferStatus(target)");
      return 0; /* formerly GL_FRAMEBUFFER_STATUS_ERROR_EXT */
   }

   if (buffer->Name == 0) {
      /* The window system / default framebuffer is always complete */
      return GL_FRAMEBUFFER_COMPLETE_EXT;
   }

   FLUSH_VERTICES(ctx, _NEW_BUFFERS);

   _mesa_test_framebuffer_completeness(ctx, buffer);
   return buffer->_Status;
}



/**
 * Common code called by glFramebufferTexture1D/2D/3DEXT().
 */
static void
framebuffer_texture(GLcontext *ctx, const char *caller, GLenum target, 
                    GLenum attachment, GLenum textarget, GLuint texture,
                    GLint level, GLint zoffset)
{
   struct gl_renderbuffer_attachment *att;
   struct gl_texture_object *texObj = NULL;
   struct gl_framebuffer *fb;

   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (target != GL_FRAMEBUFFER_EXT) {
      _mesa_error(ctx, GL_INVALID_ENUM,
                  "glFramebufferTexture%sEXT(target)", caller);
      return;
   }

   fb = ctx->DrawBuffer;
   ASSERT(fb);

   /* check framebuffer binding */
   if (fb->Name == 0) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glFramebufferTexture%sEXT", caller);
      return;
   }


   /* The textarget, level, and zoffset parameters are only validated if
    * texture is non-zero.
    */
   if (texture) {
      GLboolean err = GL_TRUE;

      texObj = _mesa_lookup_texture(ctx, texture);
      if (texObj != NULL) {
         if (textarget == 0) {
            err = (texObj->Target != GL_TEXTURE_3D) &&
                (texObj->Target != GL_TEXTURE_1D_ARRAY_EXT) &&
                (texObj->Target != GL_TEXTURE_2D_ARRAY_EXT);
         }
         else {
            err = (texObj->Target == GL_TEXTURE_CUBE_MAP)
                ? !IS_CUBE_FACE(textarget)
                : (texObj->Target != textarget);
         }
      }

      if (err) {
         _mesa_error(ctx, GL_INVALID_OPERATION,
                     "glFramebufferTexture%sEXT(texture target mismatch)",
                     caller);
         return;
      }

      if (texObj->Target == GL_TEXTURE_3D) {
         const GLint maxSize = 1 << (ctx->Const.Max3DTextureLevels - 1);
         if (zoffset < 0 || zoffset >= maxSize) {
            _mesa_error(ctx, GL_INVALID_VALUE,
                        "glFramebufferTexture%sEXT(zoffset)", caller);
            return;
         }
      }
      else if ((texObj->Target == GL_TEXTURE_1D_ARRAY_EXT) ||
               (texObj->Target == GL_TEXTURE_2D_ARRAY_EXT)) {
         if (zoffset < 0 || zoffset >= ctx->Const.MaxArrayTextureLayers) {
            _mesa_error(ctx, GL_INVALID_VALUE,
                        "glFramebufferTexture%sEXT(layer)", caller);
            return;
         }
      }


      if ((level < 0) || 
          (level >= _mesa_max_texture_levels(ctx, texObj->Target))) {
         _mesa_error(ctx, GL_INVALID_VALUE,
                     "glFramebufferTexture%sEXT(level)", caller);
         return;
      }
   }

   att = _mesa_get_attachment(ctx, fb, attachment);
   if (att == NULL) {
      _mesa_error(ctx, GL_INVALID_ENUM,
                  "glFramebufferTexture%sEXT(attachment)", caller);
      return;
   }

   FLUSH_VERTICES(ctx, _NEW_BUFFERS);
   /* The above doesn't fully flush the drivers in the way that a
    * glFlush does, but that is required here:
    */
   if (ctx->Driver.Flush)
      ctx->Driver.Flush(ctx);

   _glthread_LOCK_MUTEX(fb->Mutex);
   if (texObj) {
      _mesa_set_texture_attachment(ctx, fb, att, texObj, textarget,
                                   level, zoffset);
   }
   else {
      _mesa_remove_attachment(ctx, att);
   }
   _glthread_UNLOCK_MUTEX(fb->Mutex);
}



void GLAPIENTRY
_mesa_FramebufferTexture1DEXT(GLenum target, GLenum attachment,
                              GLenum textarget, GLuint texture, GLint level)
{
   GET_CURRENT_CONTEXT(ctx);

   if ((texture != 0) && (textarget != GL_TEXTURE_1D)) {
      _mesa_error(ctx, GL_INVALID_ENUM,
                  "glFramebufferTexture1DEXT(textarget)");
      return;
   }

   framebuffer_texture(ctx, "1D", target, attachment, textarget, texture,
                       level, 0);
}


void GLAPIENTRY
_mesa_FramebufferTexture2DEXT(GLenum target, GLenum attachment,
                              GLenum textarget, GLuint texture, GLint level)
{
   GET_CURRENT_CONTEXT(ctx);

   if ((texture != 0) &&
       (textarget != GL_TEXTURE_2D) &&
       (textarget != GL_TEXTURE_RECTANGLE_ARB) &&
       (!IS_CUBE_FACE(textarget))) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glFramebufferTexture2DEXT(textarget)");
      return;
   }

   framebuffer_texture(ctx, "2D", target, attachment, textarget, texture,
                       level, 0);
}


void GLAPIENTRY
_mesa_FramebufferTexture3DEXT(GLenum target, GLenum attachment,
                              GLenum textarget, GLuint texture,
                              GLint level, GLint zoffset)
{
   GET_CURRENT_CONTEXT(ctx);

   if ((texture != 0) && (textarget != GL_TEXTURE_3D)) {
      _mesa_error(ctx, GL_INVALID_ENUM,
                  "glFramebufferTexture3DEXT(textarget)");
      return;
   }

   framebuffer_texture(ctx, "3D", target, attachment, textarget, texture,
                       level, zoffset);
}


void GLAPIENTRY
_mesa_FramebufferTextureLayerEXT(GLenum target, GLenum attachment,
                                 GLuint texture, GLint level, GLint layer)
{
   GET_CURRENT_CONTEXT(ctx);

   framebuffer_texture(ctx, "Layer", target, attachment, 0, texture,
                       level, layer);
}


void GLAPIENTRY
_mesa_FramebufferRenderbufferEXT(GLenum target, GLenum attachment,
                                 GLenum renderbufferTarget,
                                 GLuint renderbuffer)
{
   struct gl_renderbuffer_attachment *att;
   struct gl_framebuffer *fb;
   struct gl_renderbuffer *rb;
   GET_CURRENT_CONTEXT(ctx);

   ASSERT_OUTSIDE_BEGIN_END(ctx);

   switch (target) {
#if FEATURE_EXT_framebuffer_blit
   case GL_DRAW_FRAMEBUFFER_EXT:
      if (!ctx->Extensions.EXT_framebuffer_blit) {
         _mesa_error(ctx, GL_INVALID_ENUM,
                     "glFramebufferRenderbufferEXT(target)");
         return;
      }
      fb = ctx->DrawBuffer;
      break;
   case GL_READ_FRAMEBUFFER_EXT:
      if (!ctx->Extensions.EXT_framebuffer_blit) {
         _mesa_error(ctx, GL_INVALID_ENUM,
                     "glFramebufferRenderbufferEXT(target)");
         return;
      }
      fb = ctx->ReadBuffer;
      break;
#endif
   case GL_FRAMEBUFFER_EXT:
      fb = ctx->DrawBuffer;
      break;
   default:
      _mesa_error(ctx, GL_INVALID_ENUM,
                  "glFramebufferRenderbufferEXT(target)");
      return;
   }

   if (renderbufferTarget != GL_RENDERBUFFER_EXT) {
      _mesa_error(ctx, GL_INVALID_ENUM,
                  "glFramebufferRenderbufferEXT(renderbufferTarget)");
      return;
   }

   if (fb->Name == 0) {
      /* Can't attach new renderbuffers to a window system framebuffer */
      _mesa_error(ctx, GL_INVALID_OPERATION, "glFramebufferRenderbufferEXT");
      return;
   }

   att = _mesa_get_attachment(ctx, fb, attachment);
   if (att == NULL) {
      _mesa_error(ctx, GL_INVALID_ENUM,
                 "glFramebufferRenderbufferEXT(attachment)");
      return;
   }

   if (renderbuffer) {
      rb = _mesa_lookup_renderbuffer(ctx, renderbuffer);
      if (!rb) {
	 _mesa_error(ctx, GL_INVALID_OPERATION,
		     "glFramebufferRenderbufferEXT(renderbuffer)");
	 return;
      }
   }
   else {
      /* remove renderbuffer attachment */
      rb = NULL;
   }

   FLUSH_VERTICES(ctx, _NEW_BUFFERS);
   /* The above doesn't fully flush the drivers in the way that a
    * glFlush does, but that is required here:
    */
   if (ctx->Driver.Flush)
      ctx->Driver.Flush(ctx);

   assert(ctx->Driver.FramebufferRenderbuffer);
   ctx->Driver.FramebufferRenderbuffer(ctx, fb, attachment, rb);

   /* Some subsequent GL commands may depend on the framebuffer's visual
    * after the binding is updated.  Update visual info now.
    */
   _mesa_update_framebuffer_visual(fb);
}


void GLAPIENTRY
_mesa_GetFramebufferAttachmentParameterivEXT(GLenum target, GLenum attachment,
                                             GLenum pname, GLint *params)
{
   const struct gl_renderbuffer_attachment *att;
   struct gl_framebuffer *buffer;
   GET_CURRENT_CONTEXT(ctx);

   ASSERT_OUTSIDE_BEGIN_END(ctx);

   switch (target) {
#if FEATURE_EXT_framebuffer_blit
   case GL_DRAW_FRAMEBUFFER_EXT:
      if (!ctx->Extensions.EXT_framebuffer_blit) {
         _mesa_error(ctx, GL_INVALID_ENUM,
                     "glGetFramebufferAttachmentParameterivEXT(target)");
         return;
      }
      buffer = ctx->DrawBuffer;
      break;
   case GL_READ_FRAMEBUFFER_EXT:
      if (!ctx->Extensions.EXT_framebuffer_blit) {
         _mesa_error(ctx, GL_INVALID_ENUM,
                     "glGetFramebufferAttachmentParameterivEXT(target)");
         return;
      }
      buffer = ctx->ReadBuffer;
      break;
#endif
   case GL_FRAMEBUFFER_EXT:
      buffer = ctx->DrawBuffer;
      break;
   default:
      _mesa_error(ctx, GL_INVALID_ENUM,
                  "glGetFramebufferAttachmentParameterivEXT(target)");
      return;
   }

   if (buffer->Name == 0) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glGetFramebufferAttachmentParameterivEXT");
      return;
   }

   att = _mesa_get_attachment(ctx, buffer, attachment);
   if (att == NULL) {
      _mesa_error(ctx, GL_INVALID_ENUM,
                  "glGetFramebufferAttachmentParameterivEXT(attachment)");
      return;
   }

   FLUSH_VERTICES(ctx, _NEW_BUFFERS);
   /* The above doesn't fully flush the drivers in the way that a
    * glFlush does, but that is required here:
    */
   if (ctx->Driver.Flush)
      ctx->Driver.Flush(ctx);

   switch (pname) {
   case GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE_EXT:
      *params = att->Type;
      return;
   case GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME_EXT:
      if (att->Type == GL_RENDERBUFFER_EXT) {
	 *params = att->Renderbuffer->Name;
      }
      else if (att->Type == GL_TEXTURE) {
	 *params = att->Texture->Name;
      }
      else {
	 _mesa_error(ctx, GL_INVALID_ENUM,
		     "glGetFramebufferAttachmentParameterivEXT(pname)");
      }
      return;
   case GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LEVEL_EXT:
      if (att->Type == GL_TEXTURE) {
	 *params = att->TextureLevel;
      }
      else {
	 _mesa_error(ctx, GL_INVALID_ENUM,
		     "glGetFramebufferAttachmentParameterivEXT(pname)");
      }
      return;
   case GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_CUBE_MAP_FACE_EXT:
      if (att->Type == GL_TEXTURE) {
         if (att->Texture && att->Texture->Target == GL_TEXTURE_CUBE_MAP) {
            *params = GL_TEXTURE_CUBE_MAP_POSITIVE_X + att->CubeMapFace;
         }
         else {
            *params = 0;
         }
      }
      else {
	 _mesa_error(ctx, GL_INVALID_ENUM,
		     "glGetFramebufferAttachmentParameterivEXT(pname)");
      }
      return;
   case GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_3D_ZOFFSET_EXT:
      if (att->Type == GL_TEXTURE) {
         if (att->Texture && att->Texture->Target == GL_TEXTURE_3D) {
            *params = att->Zoffset;
         }
         else {
            *params = 0;
         }
      }
      else {
	 _mesa_error(ctx, GL_INVALID_ENUM,
		     "glGetFramebufferAttachmentParameterivEXT(pname)");
      }
      return;
   default:
      _mesa_error(ctx, GL_INVALID_ENUM,
                  "glGetFramebufferAttachmentParameterivEXT(pname)");
      return;
   }
}


void GLAPIENTRY
_mesa_GenerateMipmapEXT(GLenum target)
{
   struct gl_texture_unit *texUnit;
   struct gl_texture_object *texObj;
   GET_CURRENT_CONTEXT(ctx);

   ASSERT_OUTSIDE_BEGIN_END(ctx);
   FLUSH_VERTICES(ctx, _NEW_BUFFERS);

   switch (target) {
   case GL_TEXTURE_1D:
   case GL_TEXTURE_2D:
   case GL_TEXTURE_3D:
   case GL_TEXTURE_CUBE_MAP:
      /* OK, legal value */
      break;
   default:
      _mesa_error(ctx, GL_INVALID_ENUM, "glGenerateMipmapEXT(target)");
      return;
   }

   texUnit = &ctx->Texture.Unit[ctx->Texture.CurrentUnit];
   texObj = _mesa_select_tex_object(ctx, texUnit, target);

   /* XXX this might not handle cube maps correctly */
   _mesa_lock_texture(ctx, texObj);
   ctx->Driver.GenerateMipmap(ctx, target, texObj);
   _mesa_unlock_texture(ctx, texObj);
}


#if FEATURE_EXT_framebuffer_blit
void GLAPIENTRY
_mesa_BlitFramebufferEXT(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1,
                         GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1,
                         GLbitfield mask, GLenum filter)
{
   GET_CURRENT_CONTEXT(ctx);

   ASSERT_OUTSIDE_BEGIN_END(ctx);
   FLUSH_VERTICES(ctx, _NEW_BUFFERS);

   if (ctx->NewState) {
      _mesa_update_state(ctx);
   }

   if (!ctx->ReadBuffer) {
      /* XXX */
   }

   /* check for complete framebuffers */
   if (ctx->DrawBuffer->_Status != GL_FRAMEBUFFER_COMPLETE_EXT ||
       ctx->ReadBuffer->_Status != GL_FRAMEBUFFER_COMPLETE_EXT) {
      _mesa_error(ctx, GL_INVALID_FRAMEBUFFER_OPERATION_EXT,
                  "glBlitFramebufferEXT(incomplete draw/read buffers)");
      return;
   }

   if (filter != GL_NEAREST && filter != GL_LINEAR) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glBlitFramebufferEXT(filter)");
      return;
   }

   if (mask & ~(GL_COLOR_BUFFER_BIT |
                GL_DEPTH_BUFFER_BIT |
                GL_STENCIL_BUFFER_BIT)) {
      _mesa_error( ctx, GL_INVALID_VALUE, "glBlitFramebufferEXT(mask)");
      return;
   }

   /* depth/stencil must be blitted with nearest filtering */
   if ((mask & (GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT))
        && filter != GL_NEAREST) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
             "glBlitFramebufferEXT(depth/stencil requires GL_NEAREST filter");
      return;
   }

   if (mask & GL_STENCIL_BUFFER_BIT) {
      struct gl_renderbuffer *readRb = ctx->ReadBuffer->_StencilBuffer;
      struct gl_renderbuffer *drawRb = ctx->DrawBuffer->_StencilBuffer;
      if (readRb->StencilBits != drawRb->StencilBits) {
         _mesa_error(ctx, GL_INVALID_OPERATION,
                     "glBlitFramebufferEXT(stencil buffer size mismatch");
         return;
      }
   }

   if (mask & GL_DEPTH_BUFFER_BIT) {
      struct gl_renderbuffer *readRb = ctx->ReadBuffer->_DepthBuffer;
      struct gl_renderbuffer *drawRb = ctx->DrawBuffer->_DepthBuffer;
      if (readRb->DepthBits != drawRb->DepthBits) {
         _mesa_error(ctx, GL_INVALID_OPERATION,
                     "glBlitFramebufferEXT(depth buffer size mismatch");
         return;
      }
   }

   if (!ctx->Extensions.EXT_framebuffer_blit) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glBlitFramebufferEXT");
      return;
   }

   ASSERT(ctx->Driver.BlitFramebuffer);
   ctx->Driver.BlitFramebuffer(ctx,
                               srcX0, srcY0, srcX1, srcY1,
                               dstX0, dstY0, dstX1, dstY1,
                               mask, filter);
}
#endif /* FEATURE_EXT_framebuffer_blit */
