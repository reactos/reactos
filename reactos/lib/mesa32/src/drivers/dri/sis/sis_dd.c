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
#include "sis_state.h"
#include "sis_tris.h"

#include "swrast/swrast.h"
#include "framebuffer.h"

#include "utils.h"

#define DRIVER_DATE	"20051019"

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

void
sisUpdateBufferSize( sisContextPtr smesa )
{
   __GLSiSHardware *current = &smesa->current;
   __GLSiSHardware *prev = &smesa->prev;
   GLuint z_depth;

   /* XXX Should get the base offset of the frontbuffer from the X Server */
   smesa->frontOffset = smesa->driDrawable->x * smesa->bytesPerPixel +
			smesa->driDrawable->y * smesa->frontPitch;

   if ( smesa->width == smesa->driDrawable->w &&
      smesa->height == smesa->driDrawable->h )
   {
      return;
   }

   smesa->width = smesa->driDrawable->w;
   smesa->height = smesa->driDrawable->h;
   smesa->bottom = smesa->height - 1;

   if ( smesa->backbuffer )
      sisFreeBackbuffer( smesa );
   if ( smesa->depthbuffer )
      sisFreeZStencilBuffer( smesa );
	
   if ( smesa->glCtx->Visual.depthBits > 0 )
      sisAllocZStencilBuffer( smesa );
   if ( smesa->glCtx->Visual.doubleBufferMode )
      sisAllocBackbuffer( smesa );

   switch (smesa->zFormat)
   {
   case SiS_ZFORMAT_Z16:
      z_depth = 2;
      break;
   case SiS_ZFORMAT_Z32:
   case SiS_ZFORMAT_S8Z24:
      z_depth = 4;
      break;
   default:
      sis_fatal_error("Bad Z format\n");
   }

   current->hwZ &= ~MASK_ZBufferPitch;
   current->hwZ |= smesa->width * z_depth >> 2;
   current->hwOffsetZ = smesa->depthOffset >> 2;

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
   functions->ResizeBuffers = _mesa_resize_framebuffer;
   functions->GetString     = sisGetString;
   functions->Finish        = sisFinish;
   functions->Flush         = sisFlush;
}
