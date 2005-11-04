/* -*- mode: c; c-basic-offset: 3 -*-
 *
 * Copyright 2000 VA Linux Systems Inc., Fremont, California.
 *
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
 * VA LINUX SYSTEMS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
/* $XFree86: xc/lib/GL/mesa/src/drv/tdfx/tdfx_dd.c,v 1.10 2002/10/30 12:52:00 alanh Exp $ */

/*
 * Original rewrite:
 *	Gareth Hughes <gareth@valinux.com>, 29 Sep - 1 Oct 2000
 *
 * Authors:
 *	Gareth Hughes <gareth@valinux.com>
 *	Brian Paul <brianp@valinux.com>
 *
 */

#include "tdfx_context.h"
#include "tdfx_dd.h"
#include "tdfx_lock.h"
#include "tdfx_vb.h"
#include "tdfx_pixels.h"

#include "context.h"
#include "enums.h"
#include "framebuffer.h"
#include "swrast/swrast.h"
#if defined(USE_X86_ASM)
#include "x86/common_x86_asm.h"
#endif


#define TDFX_DATE	"20040719"


/* These are used in calls to FX_grColorMaskv() */
const GLboolean false4[4] = { GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE };
const GLboolean true4[4] = { GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE };



/* KW: Put the word Mesa in the render string because quakeworld
 * checks for this rather than doing a glGet(GL_MAX_TEXTURE_SIZE).
 * Why?
 */
static const GLubyte *tdfxDDGetString( GLcontext *ctx, GLenum name )
{
   tdfxContextPtr fxMesa = (tdfxContextPtr) ctx->DriverCtx;

   switch ( name ) {
   case GL_RENDERER:
   {
      /* The renderer string must be per-context state to handle
       * multihead correctly.
       */
      char *buffer = fxMesa->rendererString;
      char hardware[100];

      LOCK_HARDWARE(fxMesa);
      strcpy( hardware, fxMesa->Glide.grGetString(GR_HARDWARE) );
      UNLOCK_HARDWARE(fxMesa);

      strcpy( buffer, "Mesa DRI " );
      strcat( buffer, TDFX_DATE );
      strcat( buffer, " " );

      if ( strcmp( hardware, "Voodoo3 (tm)" ) == 0 ) {
	 strcat( buffer, "Voodoo3" );
      }
      else if ( strcmp( hardware, "Voodoo Banshee (tm)" ) == 0 ) {
	 strcat( buffer, "VoodooBanshee" );
      }
      else if ( strcmp( hardware, "Voodoo4 (tm)" ) == 0 ) {
	 strcat( buffer, "Voodoo4" );
      }
      else if ( strcmp( hardware, "Voodoo5 (tm)" ) == 0 ) {
	 strcat( buffer, "Voodoo5" );
      }
      else {
	 /* unexpected result: replace spaces with hyphens */
	 int i;
	 for ( i = 0 ; hardware[i] && i < 60 ; i++ ) {
	    if ( hardware[i] == ' ' || hardware[i] == '\t' )
	       hardware[i] = '-';
	 }
         strcat( buffer, hardware );
      }

      /* Append any CPU-specific information.
       */
#ifdef USE_X86_ASM
      if ( _mesa_x86_cpu_features ) {
	 strncat( buffer, " x86", 4 );
      }
#endif
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
      return (const GLubyte *) buffer;
   }
   case GL_VENDOR:
      return (const GLubyte *)"VA Linux Systems, Inc.";
   default:
      return NULL;
   }
}


/* Return uptodate buffer size information.
 */
static void tdfxDDGetBufferSize( GLframebuffer *buffer,
				 GLuint *width, GLuint *height )
{
   GET_CURRENT_CONTEXT(ctx);
   tdfxContextPtr fxMesa = TDFX_CONTEXT(ctx);

   LOCK_HARDWARE( fxMesa );
   *width = fxMesa->width;
   *height = fxMesa->height;
   UNLOCK_HARDWARE( fxMesa );
}



/*
 * Return the current value of the occlusion test flag and
 * reset the flag (hardware counters) to false.
 */
static GLboolean get_occlusion_result( GLcontext *ctx )
{
   tdfxContextPtr fxMesa = TDFX_CONTEXT(ctx);
   GLboolean result;

   LOCK_HARDWARE( fxMesa );
   fxMesa->Glide.grFinish(); /* required to flush the FIFO - FB 21-01-2002 */ 

   if (ctx->Depth.OcclusionTest) {
      if (ctx->OcclusionResult) {
	 result = GL_TRUE;  /* result of software rendering */
      }
      else {
	 FxI32 zfail, in;
         fxMesa->Glide.grGet(GR_STATS_PIXELS_DEPTHFUNC_FAIL, 4, &zfail);
         fxMesa->Glide.grGet(GR_STATS_PIXELS_IN, 4, &in);
         /* Geometry is occluded if there is no input (in == 0) */
         /* or if all pixels failed the depth test (zfail == in) */
         /* The < 1 is there because I have empirically seen cases where */
         /* zfail > in.... go figure.  FB - 21-01-2002. */
         result = ((in - zfail) < 1 || in == 0) ? GL_FALSE : GL_TRUE;
      }
   }
   else {
      result = ctx->OcclusionResultSaved;
   }

   /* reset results now */
   fxMesa->Glide.grReset(GR_STATS_PIXELS);
   ctx->OcclusionResult = GL_FALSE;
   ctx->OcclusionResultSaved = GL_FALSE;

   UNLOCK_HARDWARE( fxMesa );

   return result;
}


/*
 * We're only implementing this function to handle the
 * GL_OCCLUSTION_TEST_RESULT_HP case.  It's special because it
 * has a side-effect: resetting the occlustion result flag.
 */
static GLboolean tdfxDDGetBooleanv( GLcontext *ctx, GLenum pname,
				    GLboolean *result )
{
   if ( pname == GL_OCCLUSION_TEST_RESULT_HP ) {
      *result = get_occlusion_result( ctx );
      return GL_TRUE;
   }
   return GL_FALSE;
}

static GLboolean tdfxDDGetDoublev( GLcontext *ctx, GLenum pname,
				   GLdouble *result )
{
   if ( pname == GL_OCCLUSION_TEST_RESULT_HP ) {
      *result = (GLdouble) get_occlusion_result( ctx );
      return GL_TRUE;
   }
   return GL_FALSE;
}

static GLboolean tdfxDDGetFloatv( GLcontext *ctx, GLenum pname,
				  GLfloat *result )
{
   if ( pname == GL_OCCLUSION_TEST_RESULT_HP ) {
      *result = (GLfloat) get_occlusion_result( ctx );
      return GL_TRUE;
   }
   return GL_FALSE;
}

static GLboolean tdfxDDGetIntegerv( GLcontext *ctx, GLenum pname,
				    GLint *result )
{
   if ( pname == GL_OCCLUSION_TEST_RESULT_HP ) {
      *result = (GLint) get_occlusion_result( ctx );
      return GL_TRUE;
   }
   return GL_FALSE;
}



#define VISUAL_EQUALS_RGBA(vis, r, g, b, a)        \
   ((vis->redBits == r) &&                         \
    (vis->greenBits == g) &&                       \
    (vis->blueBits == b) &&                        \
    (vis->alphaBits == a))

void tdfxDDInitDriverFuncs( const __GLcontextModes *visual,
                            struct dd_function_table *functions )
{
   if ( MESA_VERBOSE & VERBOSE_DRIVER ) {
      fprintf( stderr, "tdfx: %s()\n", __FUNCTION__ );
   }

   functions->GetString		= tdfxDDGetString;
   functions->GetBufferSize	= tdfxDDGetBufferSize;
   functions->ResizeBuffers     = _mesa_resize_framebuffer;

   /* Accelerated paths
    */
   if ( VISUAL_EQUALS_RGBA(visual, 8, 8, 8, 8) )
   {
      functions->DrawPixels	= tdfx_drawpixels_R8G8B8A8;
      functions->ReadPixels	= tdfx_readpixels_R8G8B8A8;
   }
   else if ( VISUAL_EQUALS_RGBA(visual, 5, 6, 5, 0) )
   {
      functions->ReadPixels	= tdfx_readpixels_R5G6B5;
   }

   functions->GetBooleanv	= tdfxDDGetBooleanv;
   functions->GetDoublev	= tdfxDDGetDoublev;
   functions->GetFloatv		= tdfxDDGetFloatv;
   functions->GetIntegerv	= tdfxDDGetIntegerv;
}


/*
 * These are here for lack of a better place.
 */

void
FX_grColorMaskv(GLcontext *ctx, const GLboolean rgba[4])
{
   tdfxContextPtr fxMesa = TDFX_CONTEXT(ctx);
   LOCK_HARDWARE(fxMesa);
   if (ctx->Visual.redBits == 8) {
      /* 32bpp mode */
      ASSERT( fxMesa->Glide.grColorMaskExt );
      fxMesa->Glide.grColorMaskExt(rgba[RCOMP], rgba[GCOMP],
                                   rgba[BCOMP], rgba[ACOMP]);
   }
   else {
      /* 16 bpp mode */
      /* we never have an alpha buffer */
      fxMesa->Glide.grColorMask(rgba[RCOMP] || rgba[GCOMP] || rgba[BCOMP],
                                GL_FALSE);
   }
   UNLOCK_HARDWARE(fxMesa);
}

void
FX_grColorMaskv_NoLock(GLcontext *ctx, const GLboolean rgba[4])
{
   tdfxContextPtr fxMesa = TDFX_CONTEXT(ctx);
   if (ctx->Visual.redBits == 8) {
      /* 32bpp mode */
      ASSERT( fxMesa->Glide.grColorMaskExt );
      fxMesa->Glide.grColorMaskExt(rgba[RCOMP], rgba[GCOMP],
                                   rgba[BCOMP], rgba[ACOMP]);
   }
   else {
      /* 16 bpp mode */
      /* we never have an alpha buffer */
      fxMesa->Glide.grColorMask(rgba[RCOMP] || rgba[GCOMP] || rgba[BCOMP],
                                GL_FALSE);
   }
}
