/*
 * Mesa 3-D graphics library
 * Version: 0.1
 *
 * Copyright (C) 1999-2002  Brian Paul   All Rights Reserved.
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


/* Minimal swrast-based DRI-loadable driver.
 *
 * Derived from fb_dri.c, the difference being that one works for
 * framebuffers without X, whereas this points Mesa at an X surface
 * to draw on.
 *
 * This is basically just a wrapper around src/mesa/drivers/x11 to make it
 * look like a DRI driver.
 */

#define GLX_DIRECT_RENDERING

#include <assert.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <linux/kd.h>
#include <linux/vt.h>

#include "dri_util.h"

#include "GL/xmesa.h"
#include "xmesaP.h"

#include "mtypes.h"
#include "context.h"
#include "extensions.h"
#include "imports.h"
#include "matrix.h"
#include "texformat.h"
#include "texstore.h"
#include "teximage.h"
#include "array_cache/acache.h"
#include "swrast/swrast.h"
#include "swrast_setup/swrast_setup.h"
#include "tnl/tnl.h"
#include "tnl/t_context.h"
#include "tnl/t_pipeline.h"
#include "drivers/common/driverfuncs.h"

#include "x11_dri.h"

typedef struct {
   GLcontext *glCtx;		/* Mesa context */

   struct {
      __DRIcontextPrivate *context;	
      __DRIscreenPrivate *screen;	
      __DRIdrawablePrivate *drawable; /* drawable bound to this ctx */
   } dri;
} x11Context, *x11ContextPtr;

#define X11_CONTEXT(ctx)		((x11ContextPtr)(ctx->DriverCtx))

static const GLubyte *
get_string(GLcontext *ctx, GLenum pname)
{
   (void) ctx;
   switch (pname) {
      case GL_RENDERER:
         return (const GLubyte *) "Mesa X11 hack";
      default:
         return NULL;
   }
}

static void
update_state(GLcontext *ctx, GLuint new_state)
{
   /* not much to do here - pass it on */
   _swrast_InvalidateState(ctx, new_state);
   _swsetup_InvalidateState(ctx, new_state);
   _ac_InvalidateState(ctx, new_state);
   _tnl_InvalidateState(ctx, new_state);
}

/**
 * Called by ctx->Driver.GetBufferSize from in core Mesa to query the
 * current framebuffer size.
 */
static void
get_buffer_size(GLframebuffer *buffer, GLuint *width, GLuint *height)
{
   GET_CURRENT_CONTEXT(ctx);
   x11ContextPtr x11mesa = X11_CONTEXT(ctx);

   *width  = x11mesa->dri.drawable->w;
   *height = x11mesa->dri.drawable->h;
}

static void
init_core_functions(struct dd_function_table *functions)
{
   functions->GetString = get_string;
   functions->UpdateState = update_state;
   functions->ResizeBuffers = _swrast_alloc_buffers;
   functions->GetBufferSize = get_buffer_size;

   functions->Clear = _swrast_Clear;  /* could accelerate with blits */
}

/* Initialize the driver specific screen private data.
 */
static GLboolean
x11InitDriver(__DRIscreenPrivate *sPriv)
{
   sPriv->private = NULL;
   return GL_TRUE;
}

static void
x11DestroyScreen(__DRIscreenPrivate *sPriv)
{
}

/* placeholders, disables rendering */
static void
nullwrite(void *a, int b, int c, int d, void *e, void *f)
{
}

static void
set_buffer( GLcontext *ctx, GLframebuffer *buffer, GLuint bufferBit )
{
}

/* Create the device specific context. */
static GLboolean
x11CreateContext(const __GLcontextModes *glVisual,
		 __DRIcontextPrivate *driContextPriv,
		 void *sharedContextPrivate)
{
   x11ContextPtr x11mesa;
   GLcontext *ctx, *shareCtx;
   struct dd_function_table functions;

   assert(glVisual);
   assert(driContextPriv);

   /* Allocate the Fb context */
   x11mesa = (x11ContextPtr) CALLOC(sizeof(*x11mesa));
   if (!x11mesa)
      return GL_FALSE;

   /* Init default driver functions then plug in our own functions */
   _mesa_init_driver_functions(&functions);
   init_core_functions(&functions);

   /* Allocate the Mesa context */
   if (sharedContextPrivate)
      shareCtx = ((x11ContextPtr) sharedContextPrivate)->glCtx;
   else
      shareCtx = NULL;

   ctx = x11mesa->glCtx = _mesa_create_context(glVisual, shareCtx, 
					      &functions, (void *) x11mesa);
   if (!x11mesa->glCtx) {
      FREE(x11mesa);
      return GL_FALSE;
   }
   driContextPriv->driverPrivate = x11mesa;

   /* Create module contexts */
   _swrast_CreateContext(ctx);
   _ac_CreateContext(ctx);
   _tnl_CreateContext(ctx);
   _swsetup_CreateContext(ctx);
   _swsetup_Wakeup(ctx);

   /* swrast init */
   {
      struct swrast_device_driver *swdd;
      swdd = _swrast_GetDeviceDriverReference(ctx);
      swdd->SetBuffer = set_buffer;
      if (!glVisual->rgbMode) {
         swdd->WriteCI32Span = 
         swdd->WriteCI32Span = 
         swdd->WriteCI8Span = 
         swdd->WriteMonoCISpan = 
         swdd->WriteCI32Pixels = 
         swdd->WriteMonoCIPixels = 
         swdd->ReadCI32Span = 
         swdd->ReadCI32Pixels = nullwrite;
      }
      else if (glVisual->rgbBits == 24 &&
	       glVisual->alphaBits == 0) {
         swdd->WriteRGBASpan = 
         swdd->WriteRGBSpan = 
         swdd->WriteMonoRGBASpan = 
         swdd->WriteRGBAPixels = 
         swdd->WriteMonoRGBAPixels = 
         swdd->ReadRGBASpan = 
         swdd->ReadRGBAPixels = nullwrite;
      }
      else if (glVisual->rgbBits == 32 &&
	       glVisual->alphaBits == 8) {
         swdd->WriteRGBASpan = 
         swdd->WriteRGBSpan = 
         swdd->WriteMonoRGBASpan = 
         swdd->WriteRGBAPixels = 
         swdd->WriteMonoRGBAPixels = 
         swdd->ReadRGBASpan = 
         swdd->ReadRGBAPixels = nullwrite;
      }
      else if (glVisual->rgbBits == 16 &&
	       glVisual->alphaBits == 0) {
         swdd->WriteRGBASpan = 
         swdd->WriteRGBSpan = 
         swdd->WriteMonoRGBASpan = 
         swdd->WriteRGBAPixels = 
         swdd->WriteMonoRGBAPixels = 
         swdd->ReadRGBASpan = 
         swdd->ReadRGBAPixels = nullwrite;
      }
      else if (glVisual->rgbBits == 15 &&
	       glVisual->alphaBits == 0) {
         swdd->WriteRGBASpan = 
         swdd->WriteRGBSpan = 
         swdd->WriteMonoRGBASpan = 
         swdd->WriteRGBAPixels = 
         swdd->WriteMonoRGBAPixels = 
         swdd->ReadRGBASpan = 
         swdd->ReadRGBAPixels = nullwrite;
      }
      else {
         _mesa_printf("bad pixelformat rgb %d alpha %d\n",
		      glVisual->rgbBits, 
		      glVisual->alphaBits );
      }
   }

   /* use default TCL pipeline */
   {
      TNLcontext *tnl = TNL_CONTEXT(ctx);
      tnl->Driver.RunPipeline = _tnl_run_pipeline;
   }

   _mesa_enable_sw_extensions(ctx);

   return GL_TRUE;
}


static void
x11DestroyContext(__DRIcontextPrivate *driContextPriv)
{
   GET_CURRENT_CONTEXT(ctx);
   x11ContextPtr x11mesa = (x11ContextPtr) driContextPriv->driverPrivate;
   x11ContextPtr current = ctx ? X11_CONTEXT(ctx) : NULL;

   /* check if we're deleting the currently bound context */
   if (x11mesa == current) {
      _mesa_make_current2(NULL, NULL, NULL);
   }

   /* Free x11 context resources */
   if (x11mesa) {
      _swsetup_DestroyContext(x11mesa->glCtx);
      _tnl_DestroyContext(x11mesa->glCtx);
      _ac_DestroyContext(x11mesa->glCtx);
      _swrast_DestroyContext(x11mesa->glCtx);

      /* free the Mesa context */
      x11mesa->glCtx->DriverCtx = NULL;
      _mesa_destroy_context(x11mesa->glCtx);

      FREE(x11mesa);
   }
}


/* Create and initialize the Mesa and driver specific pixmap buffer
 * data.
 */
static GLboolean
x11CreateBuffer(__DRIscreenPrivate *driScrnPriv,
		__DRIdrawablePrivate *driDrawPriv,
		const __GLcontextModes *mesaVis,
		GLboolean isPixmap)
{
   if (isPixmap) {
      return GL_FALSE; /* not implemented */
   }
   else {
      const GLboolean swDepth = mesaVis->depthBits > 0;
      const GLboolean swAlpha = mesaVis->alphaBits > 0;
      const GLboolean swAccum = mesaVis->accumRedBits > 0;
      const GLboolean swStencil = mesaVis->stencilBits > 0;
      driDrawPriv->driverPrivate = (void *)
         _mesa_create_framebuffer(mesaVis,
                                  swDepth,
                                  swStencil,
                                  swAccum,
                                  swAlpha);

      if (!driDrawPriv->driverPrivate)
	 return 0;
      
      /* Replace the framebuffer back buffer with a malloc'ed one --
       * big speedup.
       */
/*
      if (driDrawPriv->backBuffer)
	 driDrawPriv->backBuffer = malloc(driDrawPriv->currentPitch * driDrawPriv->h);
*/

      return 1;
   }
}


static void
x11DestroyBuffer(__DRIdrawablePrivate *driDrawPriv)
{
   _mesa_destroy_framebuffer((GLframebuffer *) (driDrawPriv->driverPrivate));
/*   free(driDrawPriv->backBuffer); */
}



/* If the backbuffer is on a videocard, this is extraordinarily slow!
 */
static void
x11SwapBuffers(__DRIdrawablePrivate *dPriv)
{

   if (dPriv->driContextPriv && dPriv->driContextPriv->driverPrivate) {
      x11ContextPtr x11mesa;
      GLcontext *ctx;
      x11mesa = (x11ContextPtr) dPriv->driContextPriv->driverPrivate;
      ctx = x11mesa->glCtx;
      if (ctx->Visual.doubleBufferMode) {
	 int i;
	 int offset = 0;
	 char *tmp /*= malloc(dPriv->currentPitch) */ ;

         _mesa_notifySwapBuffers(ctx);  /* flush pending rendering comands */

/*
	 ASSERT(dPriv->frontBuffer);
	 ASSERT(dPriv->backBuffer);

	 for (i = 0 ; i < dPriv->h ; i++ ) {
	    memcpy(tmp, (char *)dPriv->frontBuffer + offset, dPriv->currentPitch);
	    memcpy((char *)dPriv->backBuffer + offset, tmp, dPriv->currentPitch);
	    offset += dPriv->currentPitch;
	 }
	    
	 free(tmp);
*/
      }
   }
   else {
      /* XXX this shouldn't be an error but we can't handle it for now */
      _mesa_problem(NULL, "x11SwapBuffers: drawable has no context!\n");
   }
}


/* Force the context `c' to be the current context and associate with it
 * buffer `b'.
 */
static GLboolean
x11MakeCurrent(__DRIcontextPrivate *driContextPriv,
	       __DRIdrawablePrivate *driDrawPriv,
	       __DRIdrawablePrivate *driReadPriv)
{
   if (driContextPriv) {
      x11ContextPtr newFbCtx = 
	 (x11ContextPtr) driContextPriv->driverPrivate;

      newFbCtx->dri.drawable = driDrawPriv;

      _mesa_make_current2(newFbCtx->glCtx,
			  (GLframebuffer *) driDrawPriv->driverPrivate,
			  (GLframebuffer *) driReadPriv->driverPrivate);
   } else {
      _mesa_make_current(0, 0);
   }

   return GL_TRUE;
}


/* Force the context `c' to be unbound from its buffer.
 */
static GLboolean
x11UnbindContext(__DRIcontextPrivate *driContextPriv)
{
   return GL_TRUE;
}

static struct __DriverAPIRec x11API = {
   x11InitDriver,
   x11DestroyScreen,
   x11CreateContext,
   x11DestroyContext,
   x11CreateBuffer,
   x11DestroyBuffer,
   x11SwapBuffers,
   x11MakeCurrent,
   x11UnbindContext
};

/*
 * This is the bootstrap function for the driver.
 * The __driCreateScreen name is the symbol that libGL.so fetches.
 * Return:  pointer to a __DRIscreenPrivate.
 */
void *
__driCreateScreen(Display *dpy, int scrn, __DRIscreen *psc,
		  int numConfigs, __GLXvisualConfig *config)
{
   __DRIscreenPrivate *psp;
   psp = __driUtilCreateScreen(dpy, scrn, psc, numConfigs, config, &x11API);
   return (void *) psp;
}

/**
 * \brief Establish the set of modes available for the display.
 *
 * \param ctx display handle.
 * \param numModes will receive the number of supported modes.
 * \param modes will point to the list of supported modes.
 *
 * \return one on success, or zero on failure.
 * 
 * Allocates a single visual and fills it with information according to the
 * display bit depth. Supports only 16 and 32 bpp bit depths, aborting
 * otherwise.
 */
const __GLcontextModes __glModes[] = {
    /* 32 bit, RGBA Depth=24 Stencil=8 */
    {.rgbMode = GL_TRUE, .colorIndexMode = GL_FALSE, .doubleBufferMode = GL_TRUE, .stereoMode = GL_FALSE,
     .haveAccumBuffer = GL_FALSE, .haveDepthBuffer = GL_TRUE, .haveStencilBuffer = GL_TRUE,
     .redBits = 8, .greenBits = 8, .blueBits = 8, .alphaBits = 8,
     .redMask = 0xff0000, .greenMask = 0xff00, .blueMask = 0xff, .alphaMask = 0xff000000,
     .rgbBits = 32, .indexBits = 0,
     .accumRedBits = 0, .accumGreenBits = 0, .accumBlueBits = 0, .accumAlphaBits = 0,
     .depthBits = 24, .stencilBits = 8,
     .numAuxBuffers= 0, .level = 0, .pixmapMode = GL_FALSE, },

    /* 16 bit, RGB Depth=16 */
    {.rgbMode = GL_TRUE, .colorIndexMode = GL_FALSE, .doubleBufferMode = GL_TRUE, .stereoMode = GL_FALSE,
     .haveAccumBuffer = GL_FALSE, .haveDepthBuffer = GL_TRUE, .haveStencilBuffer = GL_FALSE,
     .redBits = 5, .greenBits = 6, .blueBits = 5, .alphaBits = 0,
     .redMask = 0xf800, .greenMask = 0x07e0, .blueMask = 0x001f, .alphaMask = 0x0,
     .rgbBits = 16, .indexBits = 0,
     .accumRedBits = 0, .accumGreenBits = 0, .accumBlueBits = 0, .accumAlphaBits = 0,
     .depthBits = 16, .stencilBits = 0,
     .numAuxBuffers= 0, .level = 0, .pixmapMode = GL_FALSE, },
};
