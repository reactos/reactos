/*
 * Copyright 2002 by Alan Hourihane, Sychdyn, North Wales, UK.
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
 * Authors:  Alan Hourihane, <alanh@fairlite.demon.co.uk>
 *
 * Trident CyberBladeXP driver.
 *
 */
#include "trident_context.h"
#include "trident_lock.h"
#if defined(USE_X86_ASM)
#include "x86/common_x86_asm.h"
#endif

#include "swrast/swrast.h"
#include "context.h"
#include "framebuffer.h"

#define TRIDENT_DATE	"20041223"

/* Return the width and height of the current color buffer.
 */
static void tridentDDGetBufferSize( GLframebuffer *framebuffer,
				 GLuint *width, GLuint *height )
{
   GET_CURRENT_CONTEXT(ctx);
   tridentContextPtr tmesa = TRIDENT_CONTEXT(ctx);

   LOCK_HARDWARE(tmesa);
   *width  = tmesa->driDrawable->w;
   *height = tmesa->driDrawable->h;
   UNLOCK_HARDWARE(tmesa);
}


/* Return various strings for glGetString().
 */
static const GLubyte *tridentDDGetString( GLcontext *ctx, GLenum name )
{
   static char buffer[128];

   switch ( name ) {
   case GL_VENDOR:
      return (GLubyte *)"Alan Hourihane";

   case GL_RENDERER:
      sprintf( buffer, "Mesa DRI Trident " TRIDENT_DATE );

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
void tridentDDInitExtensions( GLcontext *ctx )
{
   /* None... */
}

/* Initialize the driver's misc functions.
 */
void tridentDDInitDriverFuncs( GLcontext *ctx )
{
   ctx->Driver.GetBufferSize = tridentDDGetBufferSize;
   ctx->Driver.GetString = tridentDDGetString;
   ctx->Driver.Error = NULL;
}
