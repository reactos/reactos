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

/**
 * \file tdfx_dd.c
 * Device driver interface functions for 3Dfx based cards.
 * 
 * \author Gareth Hughes <gareth@valinux.com> (Original rewrite 29 Sep - 1 Oct 2000)
 * \author Brian Paul <brianp@valinux.com>
 */

#include "tdfx_context.h"
#include "tdfx_dd.h"
#include "tdfx_lock.h"
#include "tdfx_vb.h"
#include "tdfx_pixels.h"

#include "utils.h"
#include "context.h"
#include "enums.h"
#include "framebuffer.h"
#include "swrast/swrast.h"
#if defined(USE_X86_ASM)
#include "x86/common_x86_asm.h"
#endif


#define DRIVER_DATE	"20061113"


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

   switch (name) {
   case GL_RENDERER:
   {
      /* The renderer string must be per-context state to handle
       * multihead correctly.
       */
      char *const buffer = fxMesa->rendererString;
      char hardware[64];

      LOCK_HARDWARE(fxMesa);
      strncpy(hardware, fxMesa->Glide.grGetString(GR_HARDWARE),
	      sizeof(hardware));
      hardware[sizeof(hardware) - 1] = '\0';
      UNLOCK_HARDWARE(fxMesa);

      if ((strncmp(hardware, "Voodoo3", 7) == 0)
	  || (strncmp(hardware, "Voodoo4", 7) == 0)
	  || (strncmp(hardware, "Voodoo5", 7) == 0)) {
	 hardware[7] = '\0';
      }
      else if (strncmp(hardware, "Voodoo Banshee", 14) == 0) {
	 strcpy(&hardware[6], "Banshee");
      }
      else {
	 /* unexpected result: replace spaces with hyphens */
	 int i;
	 for (i = 0; hardware[i] && (i < sizeof(hardware)); i++) {
	    if (hardware[i] == ' ' || hardware[i] == '\t') {
	       hardware[i] = '-';
	    }
	 }
      }

      (void) driGetRendererString(buffer, hardware, DRIVER_DATE, 0);
      return (const GLubyte *) buffer;
   }
   case GL_VENDOR:
      return (const GLubyte *)"VA Linux Systems, Inc.";
   default:
      return NULL;
   }
}


static void
tdfxBeginQuery(GLcontext *ctx, GLenum target, struct gl_query_object *q)
{
   tdfxContextPtr fxMesa = TDFX_CONTEXT(ctx);

   (void) q;

   if (target == GL_SAMPLES_PASSED_ARB) {
      LOCK_HARDWARE(fxMesa);
      fxMesa->Glide.grFinish();
      fxMesa->Glide.grReset(GR_STATS_PIXELS);
      UNLOCK_HARDWARE(fxMesa);
   }
}


static void
tdfxEndQuery(GLcontext *ctx, GLenum target, struct gl_query_object *q)
{
   tdfxContextPtr fxMesa = TDFX_CONTEXT(ctx);
   FxI32 total_pixels;
   FxI32 z_fail_pixels;


   if (target == GL_SAMPLES_PASSED_ARB) {
      LOCK_HARDWARE(fxMesa);
      fxMesa->Glide.grFinish();

      fxMesa->Glide.grGet(GR_STATS_PIXELS_DEPTHFUNC_FAIL, sizeof(FxI32),
			  &z_fail_pixels);
      fxMesa->Glide.grGet(GR_STATS_PIXELS_IN, sizeof(FxI32), &total_pixels);

      q->Result = total_pixels - z_fail_pixels;
      
      /* Apparently, people have seen z_fail_pixels > total_pixels under
       * some conditions on some 3Dfx hardware.  The occlusion query spec
       * requires that we clamp to 0.
       */
      if (q->Result < 0) {
	 q->Result = 0;
      }

      q->Ready = GL_TRUE;

      UNLOCK_HARDWARE(fxMesa);
   }
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

   functions->GetString         = tdfxDDGetString;
   functions->BeginQuery        = tdfxBeginQuery;
   functions->EndQuery          = tdfxEndQuery;

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
