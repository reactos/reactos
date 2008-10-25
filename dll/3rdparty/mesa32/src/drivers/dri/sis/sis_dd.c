/**************************************************************************

Copyright 2000 Silicon Integrated Systems Corp, Inc., HsinChu, Taiwan.
Copyright 2003 Eric Anholt
All Rights Reserved.

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
on the rights to use, copy, modify, merge, publish, distribute, sub
license, and/or sell copies of the Software, and to permit persons to whom
the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice (including the next
paragraph) shall be included in all copies or substantial portions of the
Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
ERIC ANHOLT OR SILICON INTEGRATED SYSTEMS CORP BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/
/* $XFree86: xc/lib/GL/mesa/src/drv/sis/sis_ctx.c,v 1.3 2000/09/26 15:56:48 tsi Exp $ */

/*
 * Authors:
 *    Sung-Ching Lin <sclin@sis.com.tw>
 *    Eric Anholt <anholt@FreeBSD.org>
 *
 */

#include "sis_context.h"
#include "sis_dd.h"
#include "sis_lock.h"
#include "sis_alloc.h"
#include "sis_span.h"
#include "sis_state.h"
#include "sis_tris.h"

#include "swrast/swrast.h"
#include "framebuffer.h"
#include "renderbuffer.h"

#include "utils.h"

#define DRIVER_DATE	"20060710"

/* Return the width and height of the given buffer.
 */
static void
sisGetBufferSize( GLframebuffer *buffer,
			      GLuint *width, GLuint *height )
{
   GET_CURRENT_CONTEXT(ctx);
   sisContextPtr smesa = SIS_CONTEXT(ctx);

   LOCK_HARDWARE();
   *width  = smesa->driDrawable->w;
   *height = smesa->driDrawable->h;
   UNLOCK_HARDWARE();
}

/* Return various strings for glGetString().
 */
static const GLubyte *
sisGetString( GLcontext *ctx, GLenum name )
{
   sisContextPtr smesa = SIS_CONTEXT(ctx);
   static char buffer[128];
   unsigned   offset;
   GLuint agp_mode = (smesa->AGPSize > 0);

   switch ( name )
   {
   case GL_VENDOR:
      return (GLubyte *)"Eric Anholt";

   case GL_RENDERER:
      offset = driGetRendererString( buffer, "SiS", DRIVER_DATE, agp_mode );

      return (GLubyte *)buffer;

   default:
      return NULL;
   }
}

/* Send all commands to the hardware.
 */
static void
sisFlush( GLcontext *ctx )
{
   sisContextPtr smesa = SIS_CONTEXT(ctx);

   SIS_FIREVERTICES(smesa);
}

/* Make sure all commands have been sent to the hardware and have
 * completed processing.
 */
static void
sisFinish( GLcontext *ctx )
{
   sisContextPtr smesa = SIS_CONTEXT(ctx);

   SIS_FIREVERTICES(smesa);
   LOCK_HARDWARE();
   WaitEngIdle( smesa );
   UNLOCK_HARDWARE();
}

static void
sisDeleteRenderbuffer(struct gl_renderbuffer *rb)
{
   /* Don't free() since we're contained in sis_context struct. */
}

static GLboolean
sisRenderbufferStorage(GLcontext *ctx, struct gl_renderbuffer *rb,
                       GLenum internalFormat, GLuint width, GLuint height)
{
   rb->Width = width;
   rb->Height = height;
   rb->InternalFormat = internalFormat;
   return GL_TRUE;
}

static void
sisInitRenderbuffer(struct gl_renderbuffer *rb, GLenum format)
{
   const GLuint name = 0;

   _mesa_init_renderbuffer(rb, name);

   /* Make sure we're using a null-valued GetPointer routine */
   assert(rb->GetPointer(NULL, rb, 0, 0) == NULL);

   rb->InternalFormat = format;

   if (format == GL_RGBA) {
      /* Color */
      rb->_BaseFormat = GL_RGBA;
      rb->DataType = GL_UNSIGNED_BYTE;
   }
   else if (format == GL_DEPTH_COMPONENT16) {
      /* Depth */
      rb->_BaseFormat = GL_DEPTH_COMPONENT;
      /* we always Get/Put 32-bit Z values */
      rb->DataType = GL_UNSIGNED_INT;
   }
   else if (format == GL_DEPTH_COMPONENT24) {
      /* Depth */
      rb->_BaseFormat = GL_DEPTH_COMPONENT;
      /* we always Get/Put 32-bit Z values */
      rb->DataType = GL_UNSIGNED_INT;
   }
   else {
      /* Stencil */
      ASSERT(format == GL_STENCIL_INDEX8_EXT);
      rb->_BaseFormat = GL_STENCIL_INDEX;
      rb->DataType = GL_UNSIGNED_BYTE;
   }

   rb->Delete = sisDeleteRenderbuffer;
   rb->AllocStorage = sisRenderbufferStorage;
}

void
sisUpdateBufferSize(sisContextPtr smesa)
{
   __GLSiSHardware *current = &smesa->current;
   __GLSiSHardware *prev = &smesa->prev;
   struct gl_framebuffer *fb = smesa->glCtx->DrawBuffer;

   if (!smesa->front.Base.InternalFormat) {
      /* do one-time init for the renderbuffers */
      sisInitRenderbuffer(&smesa->front.Base, GL_RGBA);
      sisSetSpanFunctions(&smesa->front, &fb->Visual);
      _mesa_add_renderbuffer(fb, BUFFER_FRONT_LEFT, &smesa->front.Base);

      if (fb->Visual.doubleBufferMode) {
         sisInitRenderbuffer(&smesa->back.Base, GL_RGBA);
         sisSetSpanFunctions(&smesa->back, &fb->Visual);
         _mesa_add_renderbuffer(fb, BUFFER_BACK_LEFT, &smesa->back.Base);
      }

      if (smesa->glCtx->Visual.depthBits > 0) {
         sisInitRenderbuffer(&smesa->depth.Base, 
                             (smesa->glCtx->Visual.depthBits == 16
                              ? GL_DEPTH_COMPONENT16 : GL_DEPTH_COMPONENT24));
         sisSetSpanFunctions(&smesa->depth, &fb->Visual);
         _mesa_add_renderbuffer(fb, BUFFER_DEPTH, &smesa->depth.Base);
      }

      if (smesa->glCtx->Visual.stencilBits > 0) {
         sisInitRenderbuffer(&smesa->stencil.Base, GL_STENCIL_INDEX8_EXT);
         sisSetSpanFunctions(&smesa->stencil, &fb->Visual);
         _mesa_add_renderbuffer(fb, BUFFER_STENCIL, &smesa->stencil.Base);
      }
   }

   /* Make sure initialization did what we think it should */
   assert(smesa->front.Base.InternalFormat);
   assert(smesa->front.Base.AllocStorage);
   if (fb->Visual.doubleBufferMode) {
      assert(fb->Attachment[BUFFER_BACK_LEFT].Renderbuffer);
      assert(smesa->front.Base.AllocStorage);
   }
   if (fb->Visual.depthBits) {
      assert(fb->Attachment[BUFFER_DEPTH].Renderbuffer);
      assert(smesa->depth.Base.AllocStorage);
   }

   /* XXX Should get the base offset of the frontbuffer from the X Server */
   smesa->front.offset = smesa->driDrawable->x * smesa->bytesPerPixel +
			 smesa->driDrawable->y * smesa->front.pitch;
   smesa->front.map = (char *) smesa->driScreen->pFB + smesa->front.offset;

   if ( smesa->width == smesa->driDrawable->w &&
	smesa->height == smesa->driDrawable->h )
   {
      return;
   }

   smesa->front.bpp = smesa->bytesPerPixel * 8;
   /* Front pitch set on context create */
   smesa->front.size = smesa->front.pitch * smesa->driDrawable->h;

   smesa->width = smesa->driDrawable->w;
   smesa->height = smesa->driDrawable->h;
   smesa->bottom = smesa->height - 1;

   if (smesa->back.offset)
      sisFreeBackbuffer( smesa );
   if (smesa->depth.offset)
      sisFreeZStencilBuffer( smesa );

   if ( smesa->glCtx->Visual.depthBits > 0 )
      sisAllocZStencilBuffer( smesa );
   if ( smesa->glCtx->Visual.doubleBufferMode )
      sisAllocBackbuffer( smesa );

   current->hwZ &= ~MASK_ZBufferPitch;
   current->hwZ |= smesa->depth.pitch >> 2;
   current->hwOffsetZ = smesa->depth.offset >> 2;

   if ((current->hwOffsetZ != prev->hwOffsetZ) || (current->hwZ != prev->hwZ)) {
      prev->hwOffsetZ = current->hwOffsetZ;
      prev->hwZ = current->hwZ;
      smesa->GlobalFlag |= GFLAG_ZSETTING;
   }
  
   sisUpdateClipping( smesa->glCtx );
}

/* Initialize the driver's misc functions.
 */
void
sisInitDriverFuncs( struct dd_function_table *functions )
{
   functions->GetBufferSize = sisGetBufferSize;
   functions->GetString     = sisGetString;
   functions->Finish        = sisFinish;
   functions->Flush         = sisFlush;
}
