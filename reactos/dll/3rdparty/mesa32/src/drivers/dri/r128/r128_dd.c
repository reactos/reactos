/* $XFree86: xc/lib/GL/mesa/src/drv/r128/r128_dd.c,v 1.15 2002/10/30 12:51:38 alanh Exp $ */
/**************************************************************************

Copyright 1999, 2000 ATI Technologies Inc. and Precision Insight, Inc.,
                                               Cedar Park, Texas.
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
ATI, PRECISION INSIGHT AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/

/*
 * Authors:
 *   Gareth Hughes <gareth@valinux.com>
 *   Kevin E. Martin <martin@valinux.com>
 *
 */

#include "r128_context.h"
#include "r128_ioctl.h"
#include "r128_state.h"
#include "r128_dd.h"
#include "swrast/swrast.h"

#include "context.h"
#include "framebuffer.h"

#include "utils.h"

#define DRIVER_DATE	"20051027"


/* Return the width and height of the current color buffer.
 */
static void r128GetBufferSize( GLframebuffer *buffer,
				 GLuint *width, GLuint *height )
{
   GET_CURRENT_CONTEXT(ctx);
   r128ContextPtr rmesa = R128_CONTEXT(ctx);

   LOCK_HARDWARE( rmesa );
   *width  = rmesa->driDrawable->w;
   *height = rmesa->driDrawable->h;
   UNLOCK_HARDWARE( rmesa );
}

/* Return various strings for glGetString().
 */
static const GLubyte *r128GetString( GLcontext *ctx, GLenum name )
{
   r128ContextPtr rmesa = R128_CONTEXT(ctx);
   static char buffer[128];
   unsigned   offset;
   const char * card_name = "Rage 128";
   GLuint agp_mode = rmesa->r128Screen->IsPCI ? 0 :
      rmesa->r128Screen->AGPMode;

   switch ( name ) {
   case GL_VENDOR:
      return (GLubyte *)"VA Linux Systems, Inc.";

   case GL_RENDERER:
      /* Select the spefic chipset.
       */
      if ( R128_IS_PRO( rmesa ) ) {
	 card_name = "Rage 128 Pro";
      }
      else if ( R128_IS_MOBILITY( rmesa ) ) {
	 card_name = "Rage 128 Mobility";
      }

      offset = driGetRendererString( buffer, card_name, DRIVER_DATE,
				     agp_mode );

      return (GLubyte *)buffer;

   default:
      return NULL;
   }
}

/* Send all commands to the hardware.  If vertex buffers or indirect
 * buffers are in use, then we need to make sure they are sent to the
 * hardware.  All commands that are normally sent to the ring are
 * already considered `flushed'.
 */
static void r128Flush( GLcontext *ctx )
{
   r128ContextPtr rmesa = R128_CONTEXT(ctx);

   FLUSH_BATCH( rmesa );

#if ENABLE_PERF_BOXES
   if ( rmesa->boxes ) {
      LOCK_HARDWARE( rmesa );
      r128PerformanceBoxesLocked( rmesa );
      UNLOCK_HARDWARE( rmesa );
   }

   /* Log the performance counters if necessary */
   r128PerformanceCounters( rmesa );
#endif
}

/* Make sure all commands have been sent to the hardware and have
 * completed processing.
 */
static void r128Finish( GLcontext *ctx )
{
   r128ContextPtr rmesa = R128_CONTEXT(ctx);

#if ENABLE_PERF_BOXES
   /* Bump the performance counter */
   rmesa->c_drawWaits++;
#endif

   r128Flush( ctx );
   r128WaitForIdle( rmesa );
}


/* Initialize the driver's misc functions.
 */
void r128InitDriverFuncs( struct dd_function_table *functions )
{
   functions->GetBufferSize	= r128GetBufferSize;
   functions->GetString		= r128GetString;
   functions->Finish		= r128Finish;
   functions->Flush		= r128Flush;
}
