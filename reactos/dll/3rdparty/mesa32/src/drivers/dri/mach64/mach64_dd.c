/* $XFree86$ */ /* -*- mode: c; c-basic-offset: 3 -*- */
/*
 * Copyright 2000 Gareth Hughes
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * GARETH HUGHES BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/*
 * Authors:
 *	Gareth Hughes <gareth@valinux.com>
 *	Leif Delgass <ldelgass@retinalburn.net>
 *	José Fonseca <j_r_fonseca@yahoo.co.uk>
 */

#include "mach64_context.h"
#include "mach64_ioctl.h"
#include "mach64_state.h"
#include "mach64_vb.h"
#include "mach64_dd.h"

#include "context.h"
#include "utils.h"
#include "framebuffer.h"

#define DRIVER_DATE	"20051019"

/* Return the current color buffer size.
 */
static void mach64DDGetBufferSize( GLframebuffer *buffer,
				   GLuint *width, GLuint *height )
{
   GET_CURRENT_CONTEXT(ctx);
   mach64ContextPtr mmesa = MACH64_CONTEXT(ctx);

   LOCK_HARDWARE( mmesa );
   *width  = mmesa->driDrawable->w;
   *height = mmesa->driDrawable->h;
   UNLOCK_HARDWARE( mmesa );
}

/* Return various strings for glGetString().
 */
static const GLubyte *mach64DDGetString( GLcontext *ctx, GLenum name )
{
   mach64ContextPtr mmesa = MACH64_CONTEXT(ctx);
   static char buffer[128];
   unsigned   offset;
   const char * card_name = "Mach64 [Rage Pro]";
   GLuint agp_mode = mmesa->mach64Screen->IsPCI ? 0 :
      mmesa->mach64Screen->AGPMode;

   switch ( name ) {
   case GL_VENDOR:
      return (GLubyte*)"Gareth Hughes, Leif Delgass, José Fonseca";

   case GL_RENDERER:
 
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
static void mach64DDFlush( GLcontext *ctx )
{
   mach64ContextPtr mmesa = MACH64_CONTEXT(ctx);

   LOCK_HARDWARE( mmesa );
   FLUSH_DMA_LOCKED( mmesa );
   UNLOCK_HARDWARE( mmesa );

#if ENABLE_PERF_BOXES
   if ( mmesa->boxes ) {
      LOCK_HARDWARE( mmesa );
      mach64PerformanceBoxesLocked( mmesa );
      UNLOCK_HARDWARE( mmesa );
   }

   /* Log the performance counters if necessary */
   mach64PerformanceCounters( mmesa );
#endif
}

/* Make sure all commands have been sent to the hardware and have
 * completed processing.
 */
static void mach64DDFinish( GLcontext *ctx )
{
   mach64ContextPtr mmesa = MACH64_CONTEXT(ctx);

#if ENABLE_PERF_BOXES
   /* Bump the performance counter */
   mmesa->c_drawWaits++;
#endif

   mach64DDFlush( ctx );
   mach64WaitForIdle( mmesa );
}

/* Initialize the driver's misc functions.
 */
void mach64InitDriverFuncs( struct dd_function_table *functions )
{
   functions->GetBufferSize	= mach64DDGetBufferSize;
   functions->GetString	= mach64DDGetString;
   functions->Finish		= mach64DDFinish;
   functions->Flush		= mach64DDFlush;

}
