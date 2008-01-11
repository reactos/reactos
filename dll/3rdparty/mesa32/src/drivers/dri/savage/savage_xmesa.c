/*
 * Copyright 1998-2003 VIA Technologies, Inc. All Rights Reserved.
 * Copyright 2001-2003 S3 Graphics, Inc. All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sub license,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * VIA, S3 GRAPHICS, AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */


#include <X11/Xlibint.h>
#include <stdio.h>

#include "savagecontext.h"
#include "context.h"
#include "matrix.h"
#include "framebuffer.h"
#include "renderbuffer.h"
#include "simple_list.h"

#include "utils.h"

#include "extensions.h"

#include "swrast/swrast.h"
#include "swrast_setup/swrast_setup.h"
#include "tnl/tnl.h"
#include "vbo/vbo.h"

#include "tnl/t_pipeline.h"

#include "drivers/common/driverfuncs.h"

#include "savagedd.h"
#include "savagestate.h"
#include "savagetex.h"
#include "savagespan.h"
#include "savagetris.h"
#include "savageioctl.h"
#include "savage_bci.h"

#include "savage_dri.h"

#include "drirenderbuffer.h"
#include "texmem.h"

#define need_GL_ARB_multisample
#define need_GL_ARB_texture_compression
#define need_GL_ARB_vertex_buffer_object
#define need_GL_EXT_secondary_color
#include "extension_helper.h"

#include "xmlpool.h"

/* Driver-specific options
 */
#define SAVAGE_ENABLE_VDMA(def) \
DRI_CONF_OPT_BEGIN(enable_vdma,bool,def) \
	DRI_CONF_DESC(en,"Use DMA for vertex transfers") \
	DRI_CONF_DESC(de,"Benutze DMA für Vertextransfers") \
DRI_CONF_OPT_END
#define SAVAGE_ENABLE_FASTPATH(def) \
DRI_CONF_OPT_BEGIN(enable_fastpath,bool,def) \
	DRI_CONF_DESC(en,"Use fast path for unclipped primitives") \
	DRI_CONF_DESC(de,"Schneller Codepfad für ungeschnittene Polygone") \
DRI_CONF_OPT_END
#define SAVAGE_SYNC_FRAMES(def) \
DRI_CONF_OPT_BEGIN(sync_frames,bool,def) \
	DRI_CONF_DESC(en,"Synchronize with graphics hardware after each frame") \
	DRI_CONF_DESC(de,"Synchronisiere nach jedem Frame mit Grafikhardware") \
DRI_CONF_OPT_END

/* Configuration
 */
PUBLIC const char __driConfigOptions[] =
DRI_CONF_BEGIN
    DRI_CONF_SECTION_QUALITY
        DRI_CONF_TEXTURE_DEPTH(DRI_CONF_TEXTURE_DEPTH_FB)
        DRI_CONF_COLOR_REDUCTION(DRI_CONF_COLOR_REDUCTION_DITHER)
        DRI_CONF_FLOAT_DEPTH(false)
    DRI_CONF_SECTION_END
    DRI_CONF_SECTION_PERFORMANCE
        SAVAGE_ENABLE_VDMA(true)
        SAVAGE_ENABLE_FASTPATH(true)
        SAVAGE_SYNC_FRAMES(false)
        DRI_CONF_MAX_TEXTURE_UNITS(2,1,2)
    	DRI_CONF_TEXTURE_HEAPS(DRI_CONF_TEXTURE_HEAPS_ALL)
        DRI_CONF_FORCE_S3TC_ENABLE(false)
    DRI_CONF_SECTION_END
    DRI_CONF_SECTION_DEBUG
        DRI_CONF_NO_RAST(false)
    DRI_CONF_SECTION_END
DRI_CONF_END;
static const GLuint __driNConfigOptions = 10;


static const struct dri_debug_control debug_control[] =
{
    { "fall",  DEBUG_FALLBACKS },
    { "api",   DEBUG_VERBOSE_API },
    { "tex",   DEBUG_VERBOSE_TEX },
    { "verb",  DEBUG_VERBOSE_MSG },
    { "dma",   DEBUG_DMA },
    { "state", DEBUG_STATE },
    { NULL,    0 }
};
#ifndef SAVAGE_DEBUG
int SAVAGE_DEBUG = 0;
#endif


/*For time caculating test*/
#if defined(DEBUG_TIME) && DEBUG_TIME
struct timeval tv_s,tv_f;
unsigned long time_sum=0;
struct timeval tv_s1,tv_f1;
#endif

static const struct dri_extension card_extensions[] =
{
    { "GL_ARB_multisample",                GL_ARB_multisample_functions },
    { "GL_ARB_multitexture",               NULL },
    { "GL_ARB_texture_compression",        GL_ARB_texture_compression_functions },
    { "GL_ARB_vertex_buffer_object",       GL_ARB_vertex_buffer_object_functions },
    { "GL_EXT_stencil_wrap",               NULL },
    { "GL_EXT_texture_lod_bias",           NULL },
    { "GL_EXT_secondary_color",            GL_EXT_secondary_color_functions },
    { NULL,                                NULL }
};

static const struct dri_extension s4_extensions[] =
{
    { "GL_ARB_texture_env_add",            NULL },
    { "GL_ARB_texture_mirrored_repeat",    NULL },
    { NULL,                                NULL }
};

extern struct tnl_pipeline_stage _savage_texnorm_stage;
extern struct tnl_pipeline_stage _savage_render_stage;

static const struct tnl_pipeline_stage *savage_pipeline[] = {

   &_tnl_vertex_transform_stage,
   &_tnl_normal_transform_stage,
   &_tnl_lighting_stage,
   &_tnl_fog_coordinate_stage,
   &_tnl_texgen_stage,
   &_tnl_texture_transform_stage,
   &_savage_texnorm_stage,
   &_savage_render_stage,
   &_tnl_render_stage,
   0,
};


/* this is first function called in dirver*/

static GLboolean
savageInitDriver(__DRIscreenPrivate *sPriv)
{
  savageScreenPrivate *savageScreen;
  SAVAGEDRIPtr         gDRIPriv = (SAVAGEDRIPtr)sPriv->pDevPriv;
   PFNGLXSCRENABLEEXTENSIONPROC glx_enable_extension =
     (PFNGLXSCRENABLEEXTENSIONPROC) (*dri_interface->getProcAddress("glxEnableExtension"));


   if (sPriv->devPrivSize != sizeof(SAVAGEDRIRec)) {
      fprintf(stderr,"\nERROR!  sizeof(SAVAGEDRIRec) does not match passed size from device driver\n");
      return GL_FALSE;
   }

   /* Allocate the private area */
   savageScreen = (savageScreenPrivate *)Xmalloc(sizeof(savageScreenPrivate));
   if (!savageScreen)
      return GL_FALSE;

   savageScreen->driScrnPriv = sPriv;
   sPriv->private = (void *)savageScreen;

   savageScreen->chipset=gDRIPriv->chipset; 
   savageScreen->width=gDRIPriv->width;
   savageScreen->height=gDRIPriv->height;
   savageScreen->mem=gDRIPriv->mem;
   savageScreen->cpp=gDRIPriv->cpp;
   savageScreen->zpp=gDRIPriv->zpp;

   savageScreen->agpMode=gDRIPriv->agpMode;

   savageScreen->bufferSize=gDRIPriv->bufferSize;

   if (gDRIPriv->cpp == 4) 
       savageScreen->frontFormat = DV_PF_8888;
   else
       savageScreen->frontFormat = DV_PF_565;
   savageScreen->frontOffset=gDRIPriv->frontOffset;
   savageScreen->backOffset = gDRIPriv->backOffset; 
   savageScreen->depthOffset=gDRIPriv->depthOffset;

   savageScreen->textureOffset[SAVAGE_CARD_HEAP] = 
                                   gDRIPriv->textureOffset;
   savageScreen->textureSize[SAVAGE_CARD_HEAP] = 
                                   gDRIPriv->textureSize;
   savageScreen->logTextureGranularity[SAVAGE_CARD_HEAP] = 
                                   gDRIPriv->logTextureGranularity;

   savageScreen->textureOffset[SAVAGE_AGP_HEAP] = 
                                   gDRIPriv->agpTextureHandle;
   savageScreen->textureSize[SAVAGE_AGP_HEAP] = 
                                   gDRIPriv->agpTextureSize;
   savageScreen->logTextureGranularity[SAVAGE_AGP_HEAP] =
                                   gDRIPriv->logAgpTextureGranularity;

   savageScreen->agpTextures.handle = gDRIPriv->agpTextureHandle;
   savageScreen->agpTextures.size   = gDRIPriv->agpTextureSize;
   if (gDRIPriv->agpTextureSize) {
       if (drmMap(sPriv->fd, 
		  savageScreen->agpTextures.handle,
		  savageScreen->agpTextures.size,
		  (drmAddress *)&(savageScreen->agpTextures.map)) != 0) {
	   Xfree(savageScreen);
	   sPriv->private = NULL;
	   return GL_FALSE;
       }
   } else
       savageScreen->agpTextures.map = NULL;

   savageScreen->texVirtual[SAVAGE_CARD_HEAP] = 
             (drmAddress)(((GLubyte *)sPriv->pFB)+gDRIPriv->textureOffset);
   savageScreen->texVirtual[SAVAGE_AGP_HEAP] = 
                        (drmAddress)(savageScreen->agpTextures.map);

   savageScreen->aperture.handle = gDRIPriv->apertureHandle;
   savageScreen->aperture.size   = gDRIPriv->apertureSize;
   savageScreen->aperturePitch   = gDRIPriv->aperturePitch;
   if (drmMap(sPriv->fd, 
	      savageScreen->aperture.handle, 
	      savageScreen->aperture.size, 
	      (drmAddress *)&savageScreen->aperture.map) != 0) 
   {
      Xfree(savageScreen);
      sPriv->private = NULL;
      return GL_FALSE;
   }

   savageScreen->bufs = drmMapBufs(sPriv->fd);

   savageScreen->sarea_priv_offset = gDRIPriv->sarea_priv_offset;

   /* parse information in __driConfigOptions */
   driParseOptionInfo (&savageScreen->optionCache,
		       __driConfigOptions, __driNConfigOptions);

   if (glx_enable_extension != NULL) {
      (*glx_enable_extension)(sPriv->psc->screenConfigs,
			      "GLX_SGI_make_current_read");
   }

#if 0
   savageDDFastPathInit();
   savageDDTrifuncInit();
   savageDDSetupInit();
#endif
   return GL_TRUE;
}

/* Accessed by dlsym from dri_mesa_init.c
 */
static void
savageDestroyScreen(__DRIscreenPrivate *sPriv)
{
   savageScreenPrivate *savageScreen = (savageScreenPrivate *)sPriv->private;

   if (savageScreen->bufs)
       drmUnmapBufs(savageScreen->bufs);

   /* free all option information */
   driDestroyOptionInfo (&savageScreen->optionCache);

   Xfree(savageScreen);
   sPriv->private = NULL;
}

#if 0
GLvisual *XMesaCreateVisual(Display *dpy,
                            __DRIscreenPrivate *driScrnPriv,
                            const XVisualInfo *visinfo,
                            const __GLXvisualConfig *config)
{
   /* Drivers may change the args to _mesa_create_visual() in order to
    * setup special visuals.
    */
   return _mesa_create_visual( config->rgba,
                               config->doubleBuffer,
                               config->stereo,
                               _mesa_bitcount(visinfo->red_mask),
                               _mesa_bitcount(visinfo->green_mask),
                               _mesa_bitcount(visinfo->blue_mask),
                               config->alphaSize,
                               0, /* index bits */
                               config->depthSize,
                               config->stencilSize,
                               config->accumRedSize,
                               config->accumGreenSize,
                               config->accumBlueSize,
                               config->accumAlphaSize,
                               0 /* num samples */ );
}
#endif


static GLboolean
savageCreateContext( const __GLcontextModes *mesaVis,
		     __DRIcontextPrivate *driContextPriv,
		     void *sharedContextPrivate )
{
   GLcontext *ctx, *shareCtx;
   savageContextPtr imesa;
   __DRIscreenPrivate *sPriv = driContextPriv->driScreenPriv;
   struct dd_function_table functions;
   savageScreenPrivate *savageScreen = (savageScreenPrivate *)sPriv->private;
   drm_savage_sarea_t *saPriv=(drm_savage_sarea_t *)(((char*)sPriv->pSAREA)+
						 savageScreen->sarea_priv_offset);
   int textureSize[SAVAGE_NR_TEX_HEAPS];
   int i;
   imesa = (savageContextPtr)Xcalloc(sizeof(savageContext), 1);
   if (!imesa) {
      return GL_FALSE;
   }

   /* Init default driver functions then plug in savage-specific texture
    * functions that are needed as early as during context creation. */
   _mesa_init_driver_functions( &functions );
   savageDDInitTextureFuncs( &functions );

   /* Allocate the Mesa context */
   if (sharedContextPrivate)
      shareCtx = ((savageContextPtr) sharedContextPrivate)->glCtx;
   else 
      shareCtx = NULL;
   ctx = _mesa_create_context(mesaVis, shareCtx, &functions, imesa);
   if (!ctx) {
      Xfree(imesa);
      return GL_FALSE;
   }
   driContextPriv->driverPrivate = imesa;

   imesa->cmdBuf.size = SAVAGE_CMDBUF_SIZE;
   imesa->cmdBuf.base = imesa->cmdBuf.write =
       malloc(SAVAGE_CMDBUF_SIZE * sizeof(drm_savage_cmd_header_t));
   if (!imesa->cmdBuf.base)
       return GL_FALSE;

   /* Parse configuration files */
   driParseConfigFiles (&imesa->optionCache, &savageScreen->optionCache,
                        sPriv->myNum, "savage");

   imesa->float_depth = driQueryOptionb(&imesa->optionCache, "float_depth") &&
       savageScreen->chipset >= S3_SAVAGE4;
   imesa->no_rast = driQueryOptionb(&imesa->optionCache, "no_rast");

#if 0
   ctx->Const.MinLineWidth = 1.0;
   ctx->Const.MinLineWidthAA = 1.0;
   ctx->Const.MaxLineWidth = 3.0;
   ctx->Const.MaxLineWidthAA = 3.0;
   ctx->Const.LineWidthGranularity = 1.0;
#endif
   
   /* Dri stuff
    */
   imesa->hHWContext = driContextPriv->hHWContext;
   imesa->driFd = sPriv->fd;
   imesa->driHwLock = &sPriv->pSAREA->lock;
   
   imesa->savageScreen = savageScreen;
   imesa->driScreen = sPriv;
   imesa->sarea = saPriv;
   imesa->glBuffer = NULL;
   
   /* DMA buffer */

   for(i=0;i<5;i++)
   {
       imesa->apertureBase[i] = (GLubyte *)savageScreen->aperture.map + 
	   0x01000000 * i;
   }
   
   imesa->aperturePitch = savageScreen->aperturePitch;

   /* change texHeap initialize to support two kind of texture heap*/
   /* here is some parts of initialization, others in InitDriver() */
    
   (void) memset( imesa->textureHeaps, 0, sizeof( imesa->textureHeaps ) );
   make_empty_list( & imesa->swapped );

   textureSize[SAVAGE_CARD_HEAP] = savageScreen->textureSize[SAVAGE_CARD_HEAP];
   textureSize[SAVAGE_AGP_HEAP] = savageScreen->textureSize[SAVAGE_AGP_HEAP];
   imesa->lastTexHeap = savageScreen->texVirtual[SAVAGE_AGP_HEAP] ? 2 : 1;
   switch(driQueryOptioni (&imesa->optionCache, "texture_heaps")) {
   case DRI_CONF_TEXTURE_HEAPS_CARD: /* only use card memory, if available */
       if (textureSize[SAVAGE_CARD_HEAP])
	   imesa->lastTexHeap = 1;
       break;
   case DRI_CONF_TEXTURE_HEAPS_GART: /* only use gart memory, if available */
       if (imesa->lastTexHeap == 2 && textureSize[SAVAGE_AGP_HEAP])
	   textureSize[SAVAGE_CARD_HEAP] = 0;
       break;
   /*default: Nothing to do, use all available memory. */
   }
   
   for (i = 0; i < imesa->lastTexHeap; i++) {
       imesa->textureHeaps[i] = driCreateTextureHeap(
	   i, imesa,
	   textureSize[i],
	   11,					/* 2^11 = 2k alignment */
	   SAVAGE_NR_TEX_REGIONS,
	   (drmTextureRegionPtr)imesa->sarea->texList[i],
	    &imesa->sarea->texAge[i],
	    &imesa->swapped,
	    sizeof( savageTexObj ),
	    (destroy_texture_object_t *) savageDestroyTexObj );
       /* If textureSize[i] == 0 textureHeaps[i] is NULL. This can happen
	* if there is not enough card memory for a card texture heap. */
       if (imesa->textureHeaps[i])
	   driSetTextureSwapCounterLocation( imesa->textureHeaps[i],
					     & imesa->c_textureSwaps );
   }
   imesa->texture_depth = driQueryOptioni (&imesa->optionCache,
					   "texture_depth");
   if (imesa->texture_depth == DRI_CONF_TEXTURE_DEPTH_FB)
       imesa->texture_depth = ( savageScreen->cpp == 4 ) ?
	   DRI_CONF_TEXTURE_DEPTH_32 : DRI_CONF_TEXTURE_DEPTH_16;

   if (savageScreen->chipset >= S3_SAVAGE4)
       ctx->Const.MaxTextureUnits = 2;
   else
       ctx->Const.MaxTextureUnits = 1;
   if (driQueryOptioni(&imesa->optionCache, "texture_units") <
       ctx->Const.MaxTextureUnits)
       ctx->Const.MaxTextureUnits =
	   driQueryOptioni(&imesa->optionCache, "texture_units");
   ctx->Const.MaxTextureImageUnits = ctx->Const.MaxTextureUnits;
   ctx->Const.MaxTextureCoordUnits = ctx->Const.MaxTextureUnits;

   driCalculateMaxTextureLevels( imesa->textureHeaps,
				 imesa->lastTexHeap,
				 & ctx->Const,
				 4,
				 11, /* max 2D texture size is 2048x2048 */
				 0,  /* 3D textures unsupported. */
				 0,  /* cube textures unsupported. */
				 0,  /* texture rectangles unsupported. */
				 12,
				 GL_FALSE,
				 0 );
   if (ctx->Const.MaxTextureLevels <= 6) { /*spec requires at least 64x64*/
       __driUtilMessage("Not enough texture memory. "
			"Falling back to indirect rendering.");
       Xfree(imesa);
       return GL_FALSE;
   }

   imesa->hw_stencil = mesaVis->stencilBits && mesaVis->depthBits == 24;
   imesa->depth_scale = (imesa->savageScreen->zpp == 2) ?
       (1.0F/0xffff):(1.0F/0xffffff);

   imesa->bufferSize = savageScreen->bufferSize;
   imesa->dmaVtxBuf.total = 0;
   imesa->dmaVtxBuf.used = 0;
   imesa->dmaVtxBuf.flushed = 0;

   imesa->clientVtxBuf.total = imesa->bufferSize / 4;
   imesa->clientVtxBuf.used = 0;
   imesa->clientVtxBuf.flushed = 0;
   imesa->clientVtxBuf.buf = (u_int32_t *)malloc(imesa->bufferSize);

   imesa->vtxBuf = &imesa->clientVtxBuf;

   imesa->firstElt = -1;

   /* Uninitialized vertex format. Force setting the vertex state in
    * savageRenderStart.
    */
   imesa->vertex_size = 0;

   /* Utah stuff
    */
   imesa->new_state = ~0;
   imesa->new_gl_state = ~0;
   imesa->RenderIndex = ~0;
   imesa->dirty = ~0;
   imesa->lostContext = GL_TRUE;
   imesa->CurrentTexObj[0] = 0;
   imesa->CurrentTexObj[1] = 0;

   /* Initialize the software rasterizer and helper modules.
    */
   _swrast_CreateContext( ctx );
   _vbo_CreateContext( ctx );
   _tnl_CreateContext( ctx );
   
   _swsetup_CreateContext( ctx );

   /* Install the customized pipeline:
    */
   _tnl_destroy_pipeline( ctx );
   _tnl_install_pipeline( ctx, savage_pipeline );

   imesa->enable_fastpath = driQueryOptionb(&imesa->optionCache,
					    "enable_fastpath");
   /* DRM versions before 2.1.3 would only render triangle lists. ELTS
    * support was added in 2.2.0. */
   if (imesa->enable_fastpath && sPriv->drmMinor < 2) {
      fprintf (stderr,
	       "*** Disabling fast path because your DRM version is buggy "
	       "or doesn't\n*** support ELTS. You need at least Savage DRM "
	       "version 2.2.\n");
      imesa->enable_fastpath = GL_FALSE;
   }

   if (!savageScreen->bufs || savageScreen->chipset == S3_SUPERSAVAGE)
       imesa->enable_vdma = GL_FALSE;
   else
       imesa->enable_vdma = driQueryOptionb(&imesa->optionCache, "enable_vdma");

   imesa->sync_frames = driQueryOptionb(&imesa->optionCache, "sync_frames");

   /* Configure swrast to match hardware characteristics:
    */
   _tnl_allow_pixel_fog( ctx, GL_FALSE );
   _tnl_allow_vertex_fog( ctx, GL_TRUE );
   _swrast_allow_pixel_fog( ctx, GL_FALSE );
   _swrast_allow_vertex_fog( ctx, GL_TRUE );

   ctx->DriverCtx = (void *) imesa;
   imesa->glCtx = ctx;

#ifndef SAVAGE_DEBUG
   SAVAGE_DEBUG = driParseDebugString( getenv( "SAVAGE_DEBUG" ),
				       debug_control );
#endif

   driInitExtensions( ctx, card_extensions, GL_TRUE );
   if (savageScreen->chipset >= S3_SAVAGE4)
       driInitExtensions( ctx, s4_extensions, GL_FALSE );
   if (ctx->Mesa_DXTn ||
       driQueryOptionb (&imesa->optionCache, "force_s3tc_enable")) {
       _mesa_enable_extension( ctx, "GL_S3_s3tc" );
       if (savageScreen->chipset >= S3_SAVAGE4)
	   /* This extension needs DXT3 and DTX5 support in hardware.
	    * Not available on Savage3D/MX/IX. */
	   _mesa_enable_extension( ctx, "GL_EXT_texture_compression_s3tc" );
   }

   savageDDInitStateFuncs( ctx );
   savageDDInitSpanFuncs( ctx );
   savageDDInitDriverFuncs( ctx );
   savageDDInitIoctlFuncs( ctx );
   savageInitTriFuncs( ctx );

   savageDDInitState( imesa );

   driContextPriv->driverPrivate = (void *) imesa;

   return GL_TRUE;
}

static void
savageDestroyContext(__DRIcontextPrivate *driContextPriv)
{
   savageContextPtr imesa = (savageContextPtr) driContextPriv->driverPrivate;
   GLuint i;

   assert (imesa); /* should never be NULL */
   if (imesa) {
      savageFlushVertices(imesa);
      savageReleaseIndexedVerts(imesa);
      savageFlushCmdBuf(imesa, GL_TRUE); /* release DMA buffer */
      WAIT_IDLE_EMPTY(imesa);

      for (i = 0; i < imesa->lastTexHeap; i++)
	 driDestroyTextureHeap(imesa->textureHeaps[i]);

      free(imesa->cmdBuf.base);
      free(imesa->clientVtxBuf.buf);

      _swsetup_DestroyContext(imesa->glCtx );
      _tnl_DestroyContext( imesa->glCtx );
      _vbo_DestroyContext( imesa->glCtx );
      _swrast_DestroyContext( imesa->glCtx );

      /* free the Mesa context */
      imesa->glCtx->DriverCtx = NULL;
      _mesa_destroy_context(imesa->glCtx);

      /* no longer use vertex_dma_buf*/
      Xfree(imesa);
   }
}


static GLboolean
savageCreateBuffer( __DRIscreenPrivate *driScrnPriv,
		    __DRIdrawablePrivate *driDrawPriv,
		    const __GLcontextModes *mesaVis,
		    GLboolean isPixmap)
{
   savageScreenPrivate *screen = (savageScreenPrivate *) driScrnPriv->private;

   if (isPixmap) {
      return GL_FALSE; /* not implemented */
   }
   else {
      GLboolean swStencil = mesaVis->stencilBits > 0 && mesaVis->depthBits != 24;
      struct gl_framebuffer *fb = _mesa_create_framebuffer(mesaVis);
      /*
       * XXX: this value needs to be set according to the config file
       * setting.  But we don't get that until we create a rendering
       * context!!!!
       */
      GLboolean float_depth = GL_FALSE;

      {
         driRenderbuffer *frontRb
            = driNewRenderbuffer(GL_RGBA,
                                 (GLubyte *) screen->aperture.map
                                 + 0x01000000 * TARGET_FRONT,
                                 screen->cpp,
                                 screen->frontOffset, screen->aperturePitch,
                                 driDrawPriv);
         savageSetSpanFunctions(frontRb, mesaVis, float_depth);
         assert(frontRb->Base.Data);
         _mesa_add_renderbuffer(fb, BUFFER_FRONT_LEFT, &frontRb->Base);
      }

      if (mesaVis->doubleBufferMode) {
         driRenderbuffer *backRb
            = driNewRenderbuffer(GL_RGBA,
                                 (GLubyte *) screen->aperture.map
                                 + 0x01000000 * TARGET_BACK,
                                 screen->cpp,
                                 screen->backOffset, screen->aperturePitch,
                                 driDrawPriv);
         savageSetSpanFunctions(backRb, mesaVis, float_depth);
         assert(backRb->Base.Data);
         _mesa_add_renderbuffer(fb, BUFFER_BACK_LEFT, &backRb->Base);
      }

      if (mesaVis->depthBits == 16) {
         driRenderbuffer *depthRb
            = driNewRenderbuffer(GL_DEPTH_COMPONENT16,
                                 (GLubyte *) screen->aperture.map
                                 + 0x01000000 * TARGET_DEPTH,
                                 screen->zpp,
                                 screen->depthOffset, screen->aperturePitch,
                                 driDrawPriv);
         savageSetSpanFunctions(depthRb, mesaVis, float_depth);
         _mesa_add_renderbuffer(fb, BUFFER_DEPTH, &depthRb->Base);
      }
      else if (mesaVis->depthBits == 24) {
         driRenderbuffer *depthRb
            = driNewRenderbuffer(GL_DEPTH_COMPONENT24,
                                 (GLubyte *) screen->aperture.map
                                 + 0x01000000 * TARGET_DEPTH,
                                 screen->zpp,
                                 screen->depthOffset, screen->aperturePitch,
                                 driDrawPriv);
         savageSetSpanFunctions(depthRb, mesaVis, float_depth);
         _mesa_add_renderbuffer(fb, BUFFER_DEPTH, &depthRb->Base);
      }

      if (mesaVis->stencilBits > 0 && !swStencil) {
         driRenderbuffer *stencilRb
            = driNewRenderbuffer(GL_STENCIL_INDEX8_EXT,
                                 (GLubyte *) screen->aperture.map
                                 + 0x01000000 * TARGET_DEPTH,
                                 screen->zpp,
                                 screen->depthOffset, screen->aperturePitch,
                                 driDrawPriv);
         savageSetSpanFunctions(stencilRb, mesaVis, float_depth);
         _mesa_add_renderbuffer(fb, BUFFER_STENCIL, &stencilRb->Base);
      }

      _mesa_add_soft_renderbuffers(fb,
                                   GL_FALSE, /* color */
                                   GL_FALSE, /* depth */
                                   swStencil,
                                   mesaVis->accumRedBits > 0,
                                   GL_FALSE, /* alpha */
                                   GL_FALSE /* aux */);
      driDrawPriv->driverPrivate = (void *) fb;

      return (driDrawPriv->driverPrivate != NULL);
   }
}

static void
savageDestroyBuffer(__DRIdrawablePrivate *driDrawPriv)
{
   _mesa_unreference_framebuffer((GLframebuffer **)(&(driDrawPriv->driverPrivate)));
}

#if 0
void XMesaSwapBuffers(__DRIdrawablePrivate *driDrawPriv)
{
   /* XXX should do swap according to the buffer, not the context! */
   savageContextPtr imesa = savageCtx; 

   FLUSH_VB( imesa->glCtx, "swap buffers" );
   savageSwapBuffers(imesa);
}
#endif


void savageXMesaSetClipRects(savageContextPtr imesa)
{
   __DRIdrawablePrivate *dPriv = imesa->driDrawable;

   if ((dPriv->numBackClipRects == 0)
       || (imesa->glCtx->DrawBuffer->_ColorDrawBufferMask[0] == BUFFER_BIT_FRONT_LEFT)) {
      imesa->numClipRects = dPriv->numClipRects;
      imesa->pClipRects = dPriv->pClipRects;
      imesa->drawX = dPriv->x;
      imesa->drawY = dPriv->y;
   } else {
      imesa->numClipRects = dPriv->numBackClipRects;
      imesa->pClipRects = dPriv->pBackClipRects;
      imesa->drawX = dPriv->backX;
      imesa->drawY = dPriv->backY;
   }

   savageCalcViewport( imesa->glCtx );
}


static void savageXMesaWindowMoved( savageContextPtr imesa ) 
{
   __DRIdrawablePrivate *const drawable = imesa->driDrawable;
   __DRIdrawablePrivate *const readable = imesa->driReadable;

   if (0)
      fprintf(stderr, "savageXMesaWindowMoved\n\n");

   savageXMesaSetClipRects(imesa);

   driUpdateFramebufferSize(imesa->glCtx, drawable);
   if (drawable != readable) {
      driUpdateFramebufferSize(imesa->glCtx, readable);
   }
}


static GLboolean
savageUnbindContext(__DRIcontextPrivate *driContextPriv)
{
   savageContextPtr savage = (savageContextPtr) driContextPriv->driverPrivate;
   if (savage)
      savage->dirty = ~0;

   return GL_TRUE;
}

#if 0
static GLboolean
savageOpenFullScreen(__DRIcontextPrivate *driContextPriv)
{
    
  
    
    if (driContextPriv) {
      savageContextPtr imesa = (savageContextPtr) driContextPriv->driverPrivate;
      imesa->IsFullScreen = GL_TRUE;
      imesa->backup_frontOffset = imesa->savageScreen->frontOffset;
      imesa->backup_backOffset = imesa->savageScreen->backOffset;
      imesa->backup_frontBitmapDesc = imesa->savageScreen->frontBitmapDesc;
      imesa->savageScreen->frontBitmapDesc = imesa->savageScreen->backBitmapDesc;      
      imesa->toggle = TARGET_BACK;
   }

    return GL_TRUE;
}

static GLboolean
savageCloseFullScreen(__DRIcontextPrivate *driContextPriv)
{
    
    if (driContextPriv) {
      savageContextPtr imesa = (savageContextPtr) driContextPriv->driverPrivate;
      WAIT_IDLE_EMPTY(imesa);
      imesa->IsFullScreen = GL_FALSE;   
      imesa->savageScreen->frontOffset = imesa->backup_frontOffset;
      imesa->savageScreen->backOffset = imesa->backup_backOffset;
      imesa->savageScreen->frontBitmapDesc = imesa->backup_frontBitmapDesc;
   }
    return GL_TRUE;
}
#endif

static GLboolean
savageMakeCurrent(__DRIcontextPrivate *driContextPriv,
		  __DRIdrawablePrivate *driDrawPriv,
		  __DRIdrawablePrivate *driReadPriv)
{
   if (driContextPriv) {
      savageContextPtr imesa
         = (savageContextPtr) driContextPriv->driverPrivate;
      struct gl_framebuffer *drawBuffer
         = (GLframebuffer *) driDrawPriv->driverPrivate;
      struct gl_framebuffer *readBuffer
         = (GLframebuffer *) driReadPriv->driverPrivate;
      driRenderbuffer *frontRb = (driRenderbuffer *)
         drawBuffer->Attachment[BUFFER_FRONT_LEFT].Renderbuffer;
      driRenderbuffer *backRb = (driRenderbuffer *)
         drawBuffer->Attachment[BUFFER_BACK_LEFT].Renderbuffer;

      assert(frontRb->Base.Data);
      if (imesa->glCtx->Visual.doubleBufferMode) {
         assert(backRb->Base.Data);
      }

      imesa->driReadable = driReadPriv;
      imesa->driDrawable = driDrawPriv;
      imesa->dirty = ~0;
      
      _mesa_make_current(imesa->glCtx, drawBuffer, readBuffer);
      
      savageXMesaWindowMoved( imesa );
   }
   else 
   {
      _mesa_make_current(NULL, NULL, NULL);
   }
   return GL_TRUE;
}


void savageGetLock( savageContextPtr imesa, GLuint flags ) 
{
   __DRIdrawablePrivate *const drawable = imesa->driDrawable;
   __DRIdrawablePrivate *const readable = imesa->driReadable;
   __DRIscreenPrivate *sPriv = imesa->driScreen;
   drm_savage_sarea_t *sarea = imesa->sarea;
   int me = imesa->hHWContext;
   int stamp = drawable->lastStamp; 
   int heap;
   unsigned int timestamp = 0;

  

   /* We know there has been contention.
    */
   drmGetLock(imesa->driFd, imesa->hHWContext, flags);	


   /* Note contention for throttling hint
    */
   imesa->any_contend = 1;

   /* If the window moved, may need to set a new cliprect now.
    *
    * NOTE: This releases and regains the hw lock, so all state
    * checking must be done *after* this call:
    */
   DRI_VALIDATE_DRAWABLE_INFO(sPriv, drawable);
   if (drawable != readable) {
      DRI_VALIDATE_DRAWABLE_INFO(sPriv, readable);
   }


   /* If we lost context, need to dump all registers to hardware.
    * Note that we don't care about 2d contexts, even if they perform
    * accelerated commands, so the DRI locking in the X server is even
    * more broken than usual.
    */
   if (sarea->ctxOwner != me) {
      imesa->dirty |= (SAVAGE_UPLOAD_LOCAL |
		       SAVAGE_UPLOAD_GLOBAL |
		       SAVAGE_UPLOAD_FOGTBL |
		       SAVAGE_UPLOAD_TEX0 |
		       SAVAGE_UPLOAD_TEX1 |
		       SAVAGE_UPLOAD_TEXGLOBAL);
      imesa->lostContext = GL_TRUE;
      sarea->ctxOwner = me;
   }

   for (heap = 0; heap < imesa->lastTexHeap; ++heap) {
      /* If a heap was changed, update its timestamp. Do this before
       * DRI_AGE_TEXTURES updates the local_age. */
      if (imesa->textureHeaps[heap] &&
	  imesa->textureHeaps[heap]->global_age[0] >
	  imesa->textureHeaps[heap]->local_age) {
	 if (timestamp == 0)
	    timestamp = savageEmitEventLocked(imesa, 0);
	 imesa->textureHeaps[heap]->timestamp = timestamp;
      }
      DRI_AGE_TEXTURES( imesa->textureHeaps[heap] );
   }

   if (drawable->lastStamp != stamp) {
      driUpdateFramebufferSize(imesa->glCtx, drawable);
      savageXMesaWindowMoved( imesa );
   }
}



static const struct __DriverAPIRec savageAPI = {
   savageInitDriver,
   savageDestroyScreen,
   savageCreateContext,
   savageDestroyContext,
   savageCreateBuffer,
   savageDestroyBuffer,
   savageSwapBuffers,
   savageMakeCurrent,
   savageUnbindContext
};


static __GLcontextModes *
savageFillInModes( unsigned pixel_bits, unsigned depth_bits,
		   unsigned stencil_bits, GLboolean have_back_buffer )
{
    __GLcontextModes * modes;
    __GLcontextModes * m;
    unsigned num_modes;
    unsigned depth_buffer_factor;
    unsigned back_buffer_factor;
    GLenum fb_format;
    GLenum fb_type;

    /* Right now GLX_SWAP_COPY_OML isn't supported, but it would be easy
     * enough to add support.  Basically, if a context is created with an
     * fbconfig where the swap method is GLX_SWAP_COPY_OML, pageflipping
     * will never be used.
     *
     * FK: What about drivers that don't use page flipping? Could they
     * just expose GLX_SWAP_COPY_OML?
     */
    static const GLenum back_buffer_modes[] = {
	GLX_NONE, GLX_SWAP_UNDEFINED_OML /*, GLX_SWAP_COPY_OML */
    };

    u_int8_t depth_bits_array[2];
    u_int8_t stencil_bits_array[2];


    depth_bits_array[0] = depth_bits;
    depth_bits_array[1] = depth_bits;
    
    /* Just like with the accumulation buffer, always provide some modes
     * with a stencil buffer.  It will be a sw fallback, but some apps won't
     * care about that.
     */
    stencil_bits_array[0] = 0;
    stencil_bits_array[1] = (stencil_bits == 0) ? 8 : stencil_bits;

    depth_buffer_factor = ((depth_bits != 0) || (stencil_bits != 0)) ? 2 : 1;
    back_buffer_factor  = (have_back_buffer) ? 2 : 1;

    num_modes = depth_buffer_factor * back_buffer_factor * 4;

    if ( pixel_bits == 16 ) {
        fb_format = GL_RGB;
        fb_type = GL_UNSIGNED_SHORT_5_6_5;
    }
    else {
        fb_format = GL_BGR;
        fb_type = GL_UNSIGNED_INT_8_8_8_8_REV;
    }

    modes = (*dri_interface->createContextModes)( num_modes, sizeof( __GLcontextModes ) );
    m = modes;
    if ( ! driFillInModes( & m, fb_format, fb_type,
			   depth_bits_array, stencil_bits_array, depth_buffer_factor,
			   back_buffer_modes, back_buffer_factor,
			   GLX_TRUE_COLOR ) ) {
	fprintf( stderr, "[%s:%u] Error creating FBConfig!\n",
		 __func__, __LINE__ );
	return NULL;
    }

    if ( ! driFillInModes( & m, fb_format, fb_type,
			   depth_bits_array, stencil_bits_array, depth_buffer_factor,
			   back_buffer_modes, back_buffer_factor,
			   GLX_DIRECT_COLOR ) ) {
	fprintf( stderr, "[%s:%u] Error creating FBConfig!\n",
		 __func__, __LINE__ );
	return NULL;
    }

    /* Mark the visual as slow if there are "fake" stencil bits.
     */
    for ( m = modes ; m != NULL ; m = m->next ) {
	if ( (m->stencilBits != 0) && (m->stencilBits != stencil_bits) ) {
	    m->visualRating = GLX_SLOW_CONFIG;
	}
    }

    return modes;
}


/**
 * This is the bootstrap function for the driver.  libGL supplies all of the
 * requisite information about the system, and the driver initializes itself.
 * This routine also fills in the linked list pointed to by \c driver_modes
 * with the \c __GLcontextModes that the driver can support for windows or
 * pbuffers.
 * 
 * \return A pointer to a \c __DRIscreenPrivate on success, or \c NULL on 
 *         failure.
 */
PUBLIC
void * __driCreateNewScreen_20050727( __DRInativeDisplay *dpy, int scrn, __DRIscreen *psc,
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
   static const __DRIversion ddx_expected = { 2, 0, 0 };
   static const __DRIversion dri_expected = { 4, 0, 0 };
   static const __DRIversion drm_expected = { 2, 1, 0 };

   dri_interface = interface;

   if ( ! driCheckDriDdxDrmVersions2( "Savage",
				      dri_version, & dri_expected,
				      ddx_version, & ddx_expected,
				      drm_version, & drm_expected ) ) {
      return NULL;
   }
      
   psp = __driUtilCreateNewScreen(dpy, scrn, psc, NULL,
				  ddx_version, dri_version, drm_version,
				  frame_buffer, pSAREA, fd,
				  internal_api_version, &savageAPI);
   if ( psp != NULL ) {
      SAVAGEDRIPtr dri_priv = (SAVAGEDRIPtr)psp->pDevPriv;
      *driver_modes = savageFillInModes( dri_priv->cpp*8,
					 (dri_priv->cpp == 2) ? 16 : 24,
					 (dri_priv->cpp == 2) ? 0  : 8,
					 (dri_priv->backOffset != dri_priv->depthOffset) );

      /* Calling driInitExtensions here, with a NULL context pointer, does not actually
       * enable the extensions.  It just makes sure that all the dispatch offsets for all
       * the extensions that *might* be enables are known.  This is needed because the
       * dispatch offsets need to be known when _mesa_context_create is called, but we can't
       * enable the extensions until we have a context pointer.
       *
       * Hello chicken.  Hello egg.  How are you two today?
       */
      driInitExtensions( NULL, card_extensions, GL_FALSE );
   }

   return (void *) psp;
}
