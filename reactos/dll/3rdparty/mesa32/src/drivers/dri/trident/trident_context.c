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
#include "trident_dri.h"
#include "trident_context.h"
#include "trident_lock.h"

#include "swrast/swrast.h"
#include "swrast_setup/swrast_setup.h"
#include "vbo/vbo.h"

#include "tnl/tnl.h"
#include "tnl/t_pipeline.h"

#include "context.h"
#include "simple_list.h"
#include "matrix.h"
#include "extensions.h"
#include "framebuffer.h"
#include "renderbuffer.h"
#if defined(USE_X86_ASM)
#include "x86/common_x86_asm.h"
#endif
#include "simple_list.h"
#include "mm.h"
#include "drirenderbuffer.h"

#include "drivers/common/driverfuncs.h"
#include "dri_util.h"
#include "utils.h"

static const struct tnl_pipeline_stage *trident_pipeline[] = {
   &_tnl_vertex_transform_stage, 
   &_tnl_normal_transform_stage, 
   &_tnl_lighting_stage,
   &_tnl_texgen_stage, 
   &_tnl_texture_transform_stage, 
   &_tnl_render_stage,		
   0,
};


static GLboolean
tridentCreateContext( const __GLcontextModes *glVisual,
                      __DRIcontextPrivate *driContextPriv,
                      void *sharedContextPrivate)
{
   GLcontext *ctx, *shareCtx;
   __DRIscreenPrivate *sPriv = driContextPriv->driScreenPriv;
   tridentContextPtr tmesa;
   tridentScreenPtr tridentscrn;
   struct dd_function_table functions;
#if 0
   drm_trident_sarea_t *saPriv=(drm_trident_sarea_t *)(((char*)sPriv->pSAREA)+
						 sizeof(XF86DRISAREARec));
#endif

   tmesa = (tridentContextPtr) CALLOC( sizeof(*tmesa) );
   if ( !tmesa ) return GL_FALSE;

   /* Allocate the Mesa context */
   if (sharedContextPrivate)
      shareCtx = ((tridentContextPtr) sharedContextPrivate)->glCtx;
   else
      shareCtx = NULL;

   _mesa_init_driver_functions(&functions);

   tmesa->glCtx =
      _mesa_create_context(glVisual, shareCtx, &functions, (void *)tmesa);

   if (!tmesa->glCtx) {
      FREE(tmesa);
      return GL_FALSE;
   }

   tmesa->driContext = driContextPriv;
   tmesa->driScreen = sPriv;
   tmesa->driDrawable = NULL; /* Set by XMesaMakeCurrent */

   tmesa->hHWContext = driContextPriv->hHWContext;
   tmesa->driHwLock = (drmLock *)&sPriv->pSAREA->lock;
   tmesa->driFd = sPriv->fd;
#if 0
   tmesa->sarea = saPriv;
#endif

   tridentscrn = tmesa->tridentScreen = (tridentScreenPtr)(sPriv->private);

   ctx = tmesa->glCtx;

   ctx->Const.MaxTextureLevels = 13;  /* 4K by 4K?  Is that right? */
   ctx->Const.MaxTextureUnits = 1; /* Permedia 3 */

   ctx->Const.MinLineWidth = 0.0;
   ctx->Const.MaxLineWidth = 255.0;

   ctx->Const.MinLineWidthAA = 0.0;
   ctx->Const.MaxLineWidthAA = 65536.0;

   ctx->Const.MinPointSize = 0.0;
   ctx->Const.MaxPointSize = 255.0;

   ctx->Const.MinPointSizeAA = 0.5; /* 4x4 quality mode */
   ctx->Const.MaxPointSizeAA = 16.0; 
   ctx->Const.PointSizeGranularity = 0.25;

#if 0
   tmesa->texHeap = mmInit( 0, tmesa->tridentScreen->textureSize );

   make_empty_list(&tmesa->TexObjList);
   make_empty_list(&tmesa->SwappedOut);

   tmesa->CurrentTexObj[0] = 0;
   tmesa->CurrentTexObj[1] = 0; /* Permedia 3, second texture */

   tmesa->RenderIndex = ~0;
#endif

   /* Initialize the software rasterizer and helper modules.
    */
   _swrast_CreateContext( ctx );
   _vbo_CreateContext( ctx );
   _tnl_CreateContext( ctx );
   _swsetup_CreateContext( ctx );

   /* Install the customized pipeline:
    */
   _tnl_destroy_pipeline( ctx );
   _tnl_install_pipeline( ctx, trident_pipeline );

   /* Configure swrast to match hardware characteristics:
    */
   _swrast_allow_pixel_fog( ctx, GL_FALSE );
   _swrast_allow_vertex_fog( ctx, GL_TRUE );

   tridentInitVB( ctx );
   tridentDDInitExtensions( ctx );
   tridentDDInitDriverFuncs( ctx );
   tridentDDInitStateFuncs( ctx );
#if 0
   tridentDDInitSpanFuncs( ctx );
   tridentDDInitTextureFuncs( ctx );
#endif
   tridentDDInitTriFuncs( ctx );
   tridentDDInitState( tmesa );

   driContextPriv->driverPrivate = (void *)tmesa;

   UNLOCK_HARDWARE(tmesa);

   return GL_TRUE;
}

static void 
tridentDestroyContext(__DRIcontextPrivate *driContextPriv)
{
    tridentContextPtr tmesa = (tridentContextPtr)driContextPriv->driverPrivate;

    if (tmesa) {
      _swsetup_DestroyContext( tmesa->glCtx );
      _tnl_DestroyContext( tmesa->glCtx );
      _vbo_DestroyContext( tmesa->glCtx );
      _swrast_DestroyContext( tmesa->glCtx );

      /* free the Mesa context */
      tmesa->glCtx->DriverCtx = NULL;
      _mesa_destroy_context(tmesa->glCtx);

      _mesa_free(tmesa);
      driContextPriv->driverPrivate = NULL;
    }
}


static GLboolean
tridentCreateBuffer( __DRIscreenPrivate *driScrnPriv,
                   __DRIdrawablePrivate *driDrawPriv,
                   const __GLcontextModes *mesaVis,
                   GLboolean isPixmap )
{
   tridentScreenPtr screen = (tridentScreenPtr) driScrnPriv->private;

   if (isPixmap) {
      return GL_FALSE; /* not implemented */
   }
   else {
      struct gl_framebuffer *fb = _mesa_create_framebuffer(mesaVis);

      {
         driRenderbuffer *frontRb
            = driNewRenderbuffer(GL_RGBA, NULL, screen->cpp,
                                 screen->frontOffset, screen->frontPitch,
                                 driDrawPriv);
         /*
         tridentSetSpanFunctions(frontRb, mesaVis);
         */
         _mesa_add_renderbuffer(fb, BUFFER_FRONT_LEFT, &frontRb->Base);
      }

      if (mesaVis->doubleBufferMode) {
         driRenderbuffer *backRb
            = driNewRenderbuffer(GL_RGBA, NULL, screen->cpp,
                                 screen->backOffset, screen->backPitch,
                                 driDrawPriv);
         /*
         tridentSetSpanFunctions(backRb, mesaVis);
         */
         _mesa_add_renderbuffer(fb, BUFFER_BACK_LEFT, &backRb->Base);
      }

      if (mesaVis->depthBits == 16) {
         driRenderbuffer *depthRb
            = driNewRenderbuffer(GL_DEPTH_COMPONENT16, NULL, screen->cpp,
                                 screen->depthOffset, screen->depthPitch,
                                 driDrawPriv);
         /*
         tridentSetSpanFunctions(depthRb, mesaVis);
         */
         _mesa_add_renderbuffer(fb, BUFFER_DEPTH, &depthRb->Base);
      }
      else if (mesaVis->depthBits == 24) {
         driRenderbuffer *depthRb
            = driNewRenderbuffer(GL_DEPTH_COMPONENT24, NULL, screen->cpp,
                                 screen->depthOffset, screen->depthPitch,
                                 driDrawPriv);
         /*
         tridentSetSpanFunctions(depthRb, mesaVis);
         */
         _mesa_add_renderbuffer(fb, BUFFER_DEPTH, &depthRb->Base);
      }

      /* no h/w stencil?
      if (mesaVis->stencilBits > 0 && !swStencil) {
         driRenderbuffer *stencilRb
            = driNewRenderbuffer(GL_STENCIL_INDEX8_EXT);
         tridentSetSpanFunctions(stencilRb, mesaVis);
         _mesa_add_renderbuffer(fb, BUFFER_STENCIL, &stencilRb->Base);
      }
      */

      _mesa_add_soft_renderbuffers(fb,
                                   GL_FALSE, /* color */
                                   GL_FALSE, /* depth */
                                   mesaVis->stencilBits > 0,
                                   mesaVis->accumRedBits > 0,
                                   GL_FALSE, /* alpha */
                                   GL_FALSE /* aux */);
      driDrawPriv->driverPrivate = (void *) fb;

      return (driDrawPriv->driverPrivate != NULL);
   }
}


static void
tridentDestroyBuffer(__DRIdrawablePrivate *driDrawPriv)
{
   _mesa_unreference_framebuffer((GLframebuffer **)(&(driDrawPriv->driverPrivate)));
}

static void
tridentSwapBuffers(__DRIdrawablePrivate *drawablePrivate)
{
   __DRIdrawablePrivate *dPriv = (__DRIdrawablePrivate *) drawablePrivate;

   if (dPriv->driContextPriv && dPriv->driContextPriv->driverPrivate) {
      tridentContextPtr tmesa;
      GLcontext *ctx;
      tmesa = (tridentContextPtr) dPriv->driContextPriv->driverPrivate;
      ctx = tmesa->glCtx;
      if (ctx->Visual.doubleBufferMode) {
         _mesa_notifySwapBuffers( ctx );  /* flush pending rendering comands */
         tridentCopyBuffer( dPriv );
      }
   }
   else {
      /* XXX this shouldn't be an error but we can't handle it for now */
      _mesa_problem(NULL, "tridentSwapBuffers: drawable has no context!\n");
   }
}

static GLboolean 
tridentMakeCurrent(__DRIcontextPrivate *driContextPriv,
		 __DRIdrawablePrivate *driDrawPriv,
		 __DRIdrawablePrivate *driReadPriv)
{
    if (driContextPriv) {
	GET_CURRENT_CONTEXT(ctx);
	tridentContextPtr oldCtx = ctx ? TRIDENT_CONTEXT(ctx) : NULL;
	tridentContextPtr newCtx = (tridentContextPtr) driContextPriv->driverPrivate;

	if ( newCtx != oldCtx ) {
	    newCtx->dirty = ~0;
	}

	if (newCtx->driDrawable != driDrawPriv) {
	    newCtx->driDrawable = driDrawPriv;
#if 0
	    tridentUpdateWindow ( newCtx->glCtx );
	    tridentUpdateViewportOffset( newCtx->glCtx );
#endif
	}

   newCtx->drawOffset = newCtx->tridentScreen->backOffset;
   newCtx->drawPitch = newCtx->tridentScreen->backPitch;

	_mesa_make_current( newCtx->glCtx, 
                            (GLframebuffer *) driDrawPriv->driverPrivate,
                            (GLframebuffer *) driReadPriv->driverPrivate );

	if (!newCtx->glCtx->Viewport.Width) {
	    _mesa_set_viewport(newCtx->glCtx, 0, 0, 
					driDrawPriv->w, driDrawPriv->h);
	}
    } else {
	_mesa_make_current( NULL, NULL, NULL );
    }
    return GL_TRUE;
}


static GLboolean 
tridentUnbindContext( __DRIcontextPrivate *driContextPriv )
{
   return GL_TRUE;
}


static tridentScreenPtr
tridentCreateScreen( __DRIscreenPrivate *sPriv )
{
   TRIDENTDRIPtr tDRIPriv = (TRIDENTDRIPtr)sPriv->pDevPriv;
   tridentScreenPtr tridentScreen;

   if (sPriv->devPrivSize != sizeof(TRIDENTDRIRec)) {
      fprintf(stderr,"\nERROR!  sizeof(TRIDENTDRIRec) does not match passed size from device driver\n");
      return GL_FALSE;
   }

    /* Allocate the private area */
    tridentScreen = (tridentScreenPtr) CALLOC( sizeof(*tridentScreen) );
    if ( !tridentScreen ) return NULL;

   tridentScreen->driScreen = sPriv;

   tridentScreen->frontOffset = tDRIPriv->frontOffset;
   tridentScreen->backOffset = tDRIPriv->backOffset;
   tridentScreen->depthOffset = tDRIPriv->depthOffset;
   tridentScreen->frontPitch = tDRIPriv->frontPitch;
   tridentScreen->backPitch = tDRIPriv->backPitch;
   tridentScreen->depthPitch = tDRIPriv->depthPitch;
   tridentScreen->width = tDRIPriv->width;
   tridentScreen->height = tDRIPriv->height;

printf("%d %d\n",tridentScreen->width,tridentScreen->height);
printf("%d %d\n",tridentScreen->frontPitch,tridentScreen->backPitch);
printf("offset 0x%x 0x%x\n",tridentScreen->backOffset,tridentScreen->depthOffset);

   tridentScreen->mmio.handle = tDRIPriv->regs;
   tridentScreen->mmio.size = 0x20000;
    
   if (drmMap(sPriv->fd,
	 	tridentScreen->mmio.handle, tridentScreen->mmio.size,
		(drmAddressPtr)&tridentScreen->mmio.map)) {
	    FREE(tridentScreen);
	    return GL_FALSE;
    }
printf("MAPPED at %p\n", tridentScreen->mmio.map);

   return tridentScreen;
}

/* Destroy the device specific screen private data struct.
 */
static void
tridentDestroyScreen( __DRIscreenPrivate *sPriv )
{
    tridentScreenPtr tridentScreen = (tridentScreenPtr)sPriv->private;

    FREE(tridentScreen);
}

static GLboolean 
tridentInitDriver(__DRIscreenPrivate *sPriv)
{
    sPriv->private = (void *) tridentCreateScreen( sPriv );

    if (!sPriv->private) {
	tridentDestroyScreen( sPriv );
	return GL_FALSE;
    }

    return GL_TRUE;
}

static struct __DriverAPIRec tridentAPI = {
   tridentInitDriver,
   tridentDestroyScreen,
   tridentCreateContext,
   tridentDestroyContext,
   tridentCreateBuffer,
   tridentDestroyBuffer,
   tridentSwapBuffers,
   tridentMakeCurrent,
   tridentUnbindContext,
};


PUBLIC void *__driCreateNewScreen_20050727( __DRInativeDisplay *dpy, int scrn,
                                   __DRIscreen *psc,
                                   const __GLcontextModes * modes,
                                   const __DRIversion * ddx_version,
                                   const __DRIversion * dri_version,
                                   const __DRIversion * drm_version,
                                   const __DRIframebuffer * frame_buffer,
                                   drmAddress pSAREA, int fd,
                                   int internal_api_version,
				   const __DRIinterfaceMethods * interface,
                                   __GLcontextModes ** driver_modes )
{
    __DRIscreenPrivate *psp;
   static const __DRIversion ddx_expected = { 4, 0, 0 };
   static const __DRIversion dri_expected = { 3, 1, 0 };
   static const __DRIversion drm_expected = { 1, 0, 0 };

   dri_interface = interface;

   if ( ! driCheckDriDdxDrmVersions2( "Trident",
				      dri_version, & dri_expected,
				      ddx_version, & ddx_expected,
				      drm_version, & drm_expected ) ) {
      return NULL;
   }

    psp = __driUtilCreateNewScreen(dpy, scrn, psc, NULL,
                                   ddx_version, dri_version, drm_version,
                                   frame_buffer, pSAREA, fd,
                                   internal_api_version, &tridentAPI);

    if ( psp != NULL ) {
#if 0
       TRIDENTDRIPtr dri_priv = (TRIDENTDRIPtr) psp->pDevPriv;
       *driver_modes = tridentFillInModes( dri_priv->bytesPerPixel * 8,
					   GL_TRUE );
#endif
    }
    return (void *) psp;
}
