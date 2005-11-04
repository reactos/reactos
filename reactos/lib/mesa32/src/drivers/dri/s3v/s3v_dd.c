/*
 * Author: Max Lingua <sunmax@libero.it>
 */

#include "s3v_context.h"
#include "s3v_vb.h"
#include "s3v_lock.h"
#if defined(USE_X86_ASM)
#include "x86/common_x86_asm.h"
#endif

#include "context.h"
#include "framebuffer.h"
#include "swrast/swrast.h"

#define S3V_DATE "20020207"


/* Return the width and height of the current color buffer.
 */
static void s3vDDGetBufferSize( GLframebuffer *buffer,
				 GLuint *width, GLuint *height )
{
   GET_CURRENT_CONTEXT(ctx);
   s3vContextPtr vmesa = S3V_CONTEXT(ctx);

/* S3VHW_LOCK( vmesa ); */
   *width  = vmesa->driDrawable->w;
   *height = vmesa->driDrawable->h;
/* S3VHW_UNLOCK( vmesa ); */
}


/* Return various strings for glGetString().
 */
static const GLubyte *s3vDDGetString( GLcontext *ctx, GLenum name )
{
   static char buffer[128];

   switch ( name ) {
   case GL_VENDOR:
      return (GLubyte *)"Max Lingua (ladybug)";

   case GL_RENDERER:
      sprintf( buffer, "Mesa DRI S3 Virge " S3V_DATE );

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
void s3vInitExtensions( GLcontext *ctx )
{
   /* None... */
}

/* Initialize the driver's misc functions.
 */
void s3vInitDriverFuncs( GLcontext *ctx )
{
   ctx->Driver.GetBufferSize	= s3vDDGetBufferSize;
   ctx->Driver.GetString		= s3vDDGetString;

   ctx->Driver.Error			= NULL;

   /* Pixel path fallbacks
    */
   ctx->Driver.Accum                    = _swrast_Accum;
   ctx->Driver.Bitmap                   = _swrast_Bitmap;
   ctx->Driver.CopyPixels               = _swrast_CopyPixels;
   ctx->Driver.DrawPixels               = _swrast_DrawPixels;
   ctx->Driver.ReadPixels               = _swrast_ReadPixels;
   ctx->Driver.ResizeBuffers            = _mesa_resize_framebuffer;

   /* Swrast hooks for imaging extensions:
    */
   ctx->Driver.CopyColorTable			= _swrast_CopyColorTable;
   ctx->Driver.CopyColorSubTable		= _swrast_CopyColorSubTable;
   ctx->Driver.CopyConvolutionFilter1D	= _swrast_CopyConvolutionFilter1D;
   ctx->Driver.CopyConvolutionFilter2D	= _swrast_CopyConvolutionFilter2D;
}
