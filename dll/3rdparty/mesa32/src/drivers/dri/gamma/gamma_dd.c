/*
 * Copyright 2001 by Alan Hourihane.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Alan Hourihane not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Alan Hourihane makes no representations
 * about the suitability of this software for any purpose.  It is provided
 * "as is" without express or implied warranty.
 *
 * ALAN HOURIHANE DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL ALAN HOURIHANE BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 * Authors:  Alan Hourihane, <alanh@tungstengraphics.com>
 *
 */

#include "gamma_context.h"
#include "gamma_vb.h"
#include "gamma_lock.h"
#if defined(USE_X86_ASM)
#include "x86/common_x86_asm.h"
#endif

#include "context.h"
#include "swrast/swrast.h"

#define GAMMA_DATE	"20021125"


/* Return the width and height of the current color buffer.
 */
static void gammaDDGetBufferSize( GLframebuffer *buffer,
				 GLuint *width, GLuint *height )
{
   GET_CURRENT_CONTEXT(ctx);
   gammaContextPtr gmesa = GAMMA_CONTEXT(ctx);

   GAMMAHW_LOCK( gmesa );
   *width  = gmesa->driDrawable->w;
   *height = gmesa->driDrawable->h;
   GAMMAHW_UNLOCK( gmesa );
}


/* Return various strings for glGetString().
 */
static const GLubyte *gammaDDGetString( GLcontext *ctx, GLenum name )
{
   static char buffer[128];

   switch ( name ) {
   case GL_VENDOR:
      return (GLubyte *)"VA Linux Systems, Inc.";

   case GL_RENDERER:
      sprintf( buffer, "Mesa DRI Gamma " GAMMA_DATE );

      /* Append any CPU-specific information.
       */
#ifdef USE_X86_ASM
      if ( _mesa_x86_cpu_features ) {
	 strncat( buffer, " x86", 4 );
      }
#ifdef USE_MMX_ASM
      if ( cpu_has_mmx ) {
	 strncat( buffer, "/MMX", 4 );
      }
#endif
#ifdef USE_3DNOW_ASM
      if ( cpu_has_3dnow ) {
	 strncat( buffer, "/3DNow!", 7 );
      }
#endif
#ifdef USE_SSE_ASM
      if ( cpu_has_xmm ) {
	 strncat( buffer, "/SSE", 4 );
      }
#endif
#endif
      return (GLubyte *)buffer;

   default:
      return NULL;
   }
}

/* Enable the extensions supported by this driver.
 */
void gammaDDInitExtensions( GLcontext *ctx )
{
   /* None... */
}

/* Initialize the driver's misc functions.
 */
void gammaDDInitDriverFuncs( GLcontext *ctx )
{
   ctx->Driver.GetBufferSize = gammaDDGetBufferSize;
   ctx->Driver.GetString = gammaDDGetString;
}
