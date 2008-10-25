/* $XFree86: xc/lib/GL/mesa/src/drv/mga/mga_xmesa.c,v 1.19 2003/03/26 20:43:49 tsi Exp $ */
/*
 * Copyright 2000-2001 VA Linux Systems, Inc.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * on the rights to use, copy, modify, merge, publish, distribute, sub
 * license, and/or sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.  IN NO EVENT SHALL
 * VA LINUX SYSTEMS AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

/**
 * \file mga_xmesa.c
 * MGA screen and context initialization / creation code.
 *
 * \author Keith Whitwell <keith@tungstengraphics.com>
 */

#include <stdlib.h>
#include <stdint.h>
#include "drm.h"
#include "mga_drm.h"
#include "mga_xmesa.h"
#include "context.h"
#include "matrix.h"
#include "simple_list.h"
#include "imports.h"
#include "framebuffer.h"
#include "renderbuffer.h"

#include "swrast/swrast.h"
#include "swrast_setup/swrast_setup.h"
#include "tnl/tnl.h"
#include "vbo/vbo.h"

#include "tnl/t_pipeline.h"

#include "drivers/common/driverfuncs.h"

#include "mgadd.h"
#include "mgastate.h"
#include "mgatex.h"
#include "mgaspan.h"
#include "mgaioctl.h"
#include "mgatris.h"
#include "mgavb.h"
#include "mgapixel.h"
#include "mga_xmesa.h"
#include "mga_dri.h"

#include "utils.h"
#include "vblank.h"

#include "extensions.h"
#include "drirenderbuffer.h"

#include "GL/internal/dri_interface.h"

#define need_GL_ARB_multisample
#define need_GL_ARB_texture_compression
#define need_GL_ARB_vertex_buffer_object
#define need_GL_ARB_vertex_program
#define need_GL_EXT_fog_coord
#define need_GL_EXT_multi_draw_arrays
#define need_GL_EXT_secondary_color
#if 0
#define need_GL_EXT_paletted_texture
#endif
#define need_GL_NV_vertex_program
#include "extension_helper.h"

/* MGA configuration
 */
#include "xmlpool.h"

PUBLIC const char __driConfigOptions[] =
DRI_CONF_BEGIN
    DRI_CONF_SECTION_PERFORMANCE
        DRI_CONF_VBLANK_MODE(DRI_CONF_VBLANK_DEF_INTERVAL_0)
    DRI_CONF_SECTION_END
    DRI_CONF_SECTION_QUALITY
        DRI_CONF_TEXTURE_DEPTH(DRI_CONF_TEXTURE_DEPTH_FB)
        DRI_CONF_COLOR_REDUCTION(DRI_CONF_COLOR_REDUCTION_DITHER)
    DRI_CONF_SECTION_END
    DRI_CONF_SECTION_SOFTWARE
        DRI_CONF_ARB_VERTEX_PROGRAM(true)
        DRI_CONF_NV_VERTEX_PROGRAM(true)
    DRI_CONF_SECTION_END
    DRI_CONF_SECTION_DEBUG
        DRI_CONF_NO_RAST(false)
    DRI_CONF_SECTION_END
DRI_CONF_END;
static const GLuint __driNConfigOptions = 6;

#ifndef MGA_DEBUG
int MGA_DEBUG = 0;
#endif

static int getSwapInfo( __DRIdrawablePrivate *dPriv, __DRIswapInfo * sInfo );

static __GLcontextModes *
mgaFillInModes( unsigned pixel_bits, unsigned depth_bits,
		unsigned stencil_bits, GLboolean have_back_buffer )
{
    __GLcontextModes * modes;
    __GLcontextModes * m;
    unsigned num_modes;
    unsigned depth_buffer_factor;
    unsigned back_buffer_factor;
    GLenum fb_format;
    GLenum fb_type;

    /* GLX_SWAP_COPY_OML is only supported because the MGA driver doesn't
     * support pageflipping at all.
     */
    static const GLenum back_buffer_modes[] = {
	GLX_NONE, GLX_SWAP_UNDEFINED_OML, GLX_SWAP_COPY_OML
    };

    u_int8_t depth_bits_array[3];
    u_int8_t stencil_bits_array[3];


    depth_bits_array[0] = 0;
    depth_bits_array[1] = depth_bits;
    depth_bits_array[2] = depth_bits;
    
    /* Just like with the accumulation buffer, always provide some modes
     * with a stencil buffer.  It will be a sw fallback, but some apps won't
     * care about that.
     */
    stencil_bits_array[0] = 0;
    stencil_bits_array[1] = 0;
    stencil_bits_array[2] = (stencil_bits == 0) ? 8 : stencil_bits;

    depth_buffer_factor = ((depth_bits != 0) || (stencil_bits != 0)) ? 3 : 1;
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


static GLboolean
mgaInitDriver(__DRIscreenPrivate *sPriv)
{
   mgaScreenPrivate *mgaScreen;
   MGADRIPtr         serverInfo = (MGADRIPtr)sPriv->pDevPriv;
   PFNGLXSCRENABLEEXTENSIONPROC glx_enable_extension =
       (PFNGLXSCRENABLEEXTENSIONPROC) (*dri_interface->getProcAddress("glxEnableExtension"));
   void * const psc = sPriv->psc->screenConfigs;

   if (sPriv->devPrivSize != sizeof(MGADRIRec)) {
      fprintf(stderr,"\nERROR!  sizeof(MGADRIRec) does not match passed size from device driver\n");
      return GL_FALSE;
   }

   /* Allocate the private area */
   mgaScreen = (mgaScreenPrivate *)MALLOC(sizeof(mgaScreenPrivate));
   if (!mgaScreen) {
      __driUtilMessage("Couldn't malloc screen struct");
      return GL_FALSE;
   }

   mgaScreen->sPriv = sPriv;
   sPriv->private = (void *)mgaScreen;

   if (sPriv->drmMinor >= 1) {
      int ret;
      drm_mga_getparam_t gp;

      gp.param = MGA_PARAM_IRQ_NR;
      gp.value = &mgaScreen->irq;
      mgaScreen->irq = 0;

      ret = drmCommandWriteRead( sPriv->fd, DRM_MGA_GETPARAM,
				    &gp, sizeof(gp));
      if (ret) {
	    fprintf(stderr, "drmMgaGetParam (MGA_PARAM_IRQ_NR): %d\n", ret);
	    FREE(mgaScreen);
	    sPriv->private = NULL;
	    return GL_FALSE;
      }
   }

   if ( glx_enable_extension != NULL ) {
      (*glx_enable_extension)( psc, "GLX_MESA_swap_control" );
      (*glx_enable_extension)( psc, "GLX_MESA_swap_frame_usage" );
      (*glx_enable_extension)( psc, "GLX_SGI_make_current_read" );
      (*glx_enable_extension)( psc, "GLX_SGI_swap_control" );
      (*glx_enable_extension)( psc, "GLX_SGI_video_sync" );
   }

   if (serverInfo->chipset != MGA_CARD_TYPE_G200 &&
       serverInfo->chipset != MGA_CARD_TYPE_G400) {
      FREE(mgaScreen);
      sPriv->private = NULL;
      __driUtilMessage("Unrecognized chipset");
      return GL_FALSE;
   }


   mgaScreen->chipset = serverInfo->chipset;
   mgaScreen->cpp = serverInfo->cpp;

   mgaScreen->agpMode = serverInfo->agpMode;

   mgaScreen->frontPitch = serverInfo->frontPitch;
   mgaScreen->frontOffset = serverInfo->frontOffset;
   mgaScreen->backOffset = serverInfo->backOffset;
   mgaScreen->backPitch  =  serverInfo->backPitch;
   mgaScreen->depthOffset = serverInfo->depthOffset;
   mgaScreen->depthPitch  =  serverInfo->depthPitch;


   /* The only reason that the MMIO region needs to be accessable and the
    * primary DMA region base address needs to be known is so that the driver
    * can busy wait for certain DMA operations to complete (see
    * mgaWaitForFrameCompletion in mgaioctl.c).
    *
    * Starting with MGA DRM version 3.2, these are completely unneeded as
    * there is a new, in-kernel mechanism for handling the wait.
    */

   if (mgaScreen->sPriv->drmMinor < 2) {
      mgaScreen->mmio.handle = serverInfo->registers.handle;
      mgaScreen->mmio.size = serverInfo->registers.size;
      if ( drmMap( sPriv->fd,
		   mgaScreen->mmio.handle, mgaScreen->mmio.size,
		   &mgaScreen->mmio.map ) < 0 ) {
	 FREE( mgaScreen );
	 sPriv->private = NULL;
	 __driUtilMessage( "Couldn't map MMIO registers" );
	 return GL_FALSE;
      }

      mgaScreen->primary.handle = serverInfo->primary.handle;
      mgaScreen->primary.size = serverInfo->primary.size;
   }
   else {
      (void) memset( & mgaScreen->primary, 0, sizeof( mgaScreen->primary ) );
      (void) memset( & mgaScreen->mmio, 0, sizeof( mgaScreen->mmio ) );
   }

   mgaScreen->textureOffset[MGA_CARD_HEAP] = serverInfo->textureOffset;
   mgaScreen->textureOffset[MGA_AGP_HEAP] = (serverInfo->agpTextureOffset |
					     PDEA_pagpxfer_enable | 1);

   mgaScreen->textureSize[MGA_CARD_HEAP] = serverInfo->textureSize;
   mgaScreen->textureSize[MGA_AGP_HEAP] = serverInfo->agpTextureSize;

   
   /* The texVirtual array stores the base addresses in the CPU's address
    * space of the texture memory pools.  The base address of the on-card
    * memory pool is calculated as an offset of the base of video memory.  The
    * AGP texture pool has to be mapped into the processes address space by
    * the DRM. 
    */

   mgaScreen->texVirtual[MGA_CARD_HEAP] = (char *)(mgaScreen->sPriv->pFB +
					   serverInfo->textureOffset);

   if ( serverInfo->agpTextureSize > 0 ) {
      if (drmMap(sPriv->fd, serverInfo->agpTextureOffset,
		 serverInfo->agpTextureSize,
		 (drmAddress *)&mgaScreen->texVirtual[MGA_AGP_HEAP]) != 0) {
	 FREE(mgaScreen);
	 sPriv->private = NULL;
	 __driUtilMessage("Couldn't map agptexture region");
	 return GL_FALSE;
      }
   }


   /* For calculating setupdma addresses.
    */

   mgaScreen->bufs = drmMapBufs(sPriv->fd);
   if (!mgaScreen->bufs) {
      FREE(mgaScreen);
      sPriv->private = NULL;
      __driUtilMessage("Couldn't map dma buffers");
      return GL_FALSE;
   }
   mgaScreen->sarea_priv_offset = serverInfo->sarea_priv_offset;

   /* parse information in __driConfigOptions */
   driParseOptionInfo (&mgaScreen->optionCache,
		       __driConfigOptions, __driNConfigOptions);

   return GL_TRUE;
}


static void
mgaDestroyScreen(__DRIscreenPrivate *sPriv)
{
   mgaScreenPrivate *mgaScreen = (mgaScreenPrivate *) sPriv->private;

   if (MGA_DEBUG&DEBUG_VERBOSE_DRI)
      fprintf(stderr, "mgaDestroyScreen\n");

   drmUnmapBufs(mgaScreen->bufs);


   /* free all option information */
   driDestroyOptionInfo (&mgaScreen->optionCache);

   FREE(mgaScreen);
   sPriv->private = NULL;
}


extern const struct tnl_pipeline_stage _mga_render_stage;

static const struct tnl_pipeline_stage *mga_pipeline[] = {
   &_tnl_vertex_transform_stage, 
   &_tnl_normal_transform_stage, 
   &_tnl_lighting_stage,	
   &_tnl_fog_coordinate_stage,
   &_tnl_texgen_stage, 
   &_tnl_texture_transform_stage, 
   &_tnl_vertex_program_stage,

				/* REMOVE: point attenuation stage */
#if 0
   &_mga_render_stage,		/* ADD: unclipped rastersetup-to-dma */
                                /* Need new ioctl for wacceptseq */
#endif
   &_tnl_render_stage,		
   0,
};


static const struct dri_extension g400_extensions[] =
{
   { "GL_ARB_multitexture",           NULL },
   { "GL_ARB_texture_env_add",        NULL },
   { "GL_ARB_texture_env_combine",    NULL },
   { "GL_ARB_texture_env_crossbar",   NULL },
   { "GL_EXT_texture_env_combine",    NULL },
   { "GL_EXT_texture_edge_clamp",     NULL },
   { "GL_ATI_texture_env_combine3",   NULL },
   { NULL,                            NULL }
};

static const struct dri_extension card_extensions[] =
{
   { "GL_ARB_multisample",            GL_ARB_multisample_functions },
   { "GL_ARB_texture_compression",    GL_ARB_texture_compression_functions },
   { "GL_ARB_texture_rectangle",      NULL },
   { "GL_ARB_vertex_buffer_object",   GL_ARB_vertex_buffer_object_functions },
   { "GL_EXT_blend_logic_op",         NULL },
   { "GL_EXT_fog_coord",              GL_EXT_fog_coord_functions },
   { "GL_EXT_multi_draw_arrays",      GL_EXT_multi_draw_arrays_functions },
   /* paletted_textures currently doesn't work, but we could fix them later */
#if defined( need_GL_EXT_paletted_texture )
   { "GL_EXT_shared_texture_palette", NULL },
   { "GL_EXT_paletted_texture",       GL_EXT_paletted_texture_functions },
#endif
   { "GL_EXT_secondary_color",        GL_EXT_secondary_color_functions },
   { "GL_EXT_stencil_wrap",           NULL },
   { "GL_MESA_ycbcr_texture",         NULL },
   { "GL_SGIS_generate_mipmap",       NULL },
   { NULL,                            NULL }
};

static const struct dri_extension ARB_vp_extension[] = {
   { "GL_ARB_vertex_program",         GL_ARB_vertex_program_functions },
   { NULL,                            NULL }
};

static const struct dri_extension NV_vp_extensions[] = {
   { "GL_NV_vertex_program",          GL_NV_vertex_program_functions },
   { "GL_NV_vertex_program1_1",       NULL },
   { NULL,                            NULL }
};

static const struct dri_debug_control debug_control[] =
{
    { "fall",  DEBUG_VERBOSE_FALLBACK },
    { "tex",   DEBUG_VERBOSE_TEXTURE },
    { "ioctl", DEBUG_VERBOSE_IOCTL },
    { "verb",  DEBUG_VERBOSE_MSG },
    { "dri",   DEBUG_VERBOSE_DRI },
    { NULL,    0 }
};


static GLboolean
mgaCreateContext( const __GLcontextModes *mesaVis,
                  __DRIcontextPrivate *driContextPriv,
                  void *sharedContextPrivate )
{
   int i;
   unsigned   maxlevels;
   GLcontext *ctx, *shareCtx;
   mgaContextPtr mmesa;
   __DRIscreenPrivate *sPriv = driContextPriv->driScreenPriv;
   mgaScreenPrivate *mgaScreen = (mgaScreenPrivate *)sPriv->private;
   drm_mga_sarea_t *saPriv = (drm_mga_sarea_t *)(((char*)sPriv->pSAREA)+
					      mgaScreen->sarea_priv_offset);
   struct dd_function_table functions;

   if (MGA_DEBUG&DEBUG_VERBOSE_DRI)
      fprintf(stderr, "mgaCreateContext\n");

   /* allocate mga context */
   mmesa = (mgaContextPtr) CALLOC(sizeof(mgaContext));
   if (!mmesa) {
      return GL_FALSE;
   }

   /* Init default driver functions then plug in our Radeon-specific functions
    * (the texture functions are especially important)
    */
   _mesa_init_driver_functions( &functions );
   mgaInitDriverFuncs( &functions );
   mgaInitTextureFuncs( &functions );
   mgaInitIoctlFuncs( &functions );

   /* Allocate the Mesa context */
   if (sharedContextPrivate)
      shareCtx = ((mgaContextPtr) sharedContextPrivate)->glCtx;
   else 
      shareCtx = NULL;
   mmesa->glCtx = _mesa_create_context(mesaVis, shareCtx,
                                       &functions, (void *) mmesa);
   if (!mmesa->glCtx) {
      FREE(mmesa);
      return GL_FALSE;
   }
   driContextPriv->driverPrivate = mmesa;

   /* Init mga state */
   mmesa->hHWContext = driContextPriv->hHWContext;
   mmesa->driFd = sPriv->fd;
   mmesa->driHwLock = &sPriv->pSAREA->lock;

   mmesa->mgaScreen = mgaScreen;
   mmesa->driScreen = sPriv;
   mmesa->sarea = (void *)saPriv;

   /* Parse configuration files */
   driParseConfigFiles (&mmesa->optionCache, &mgaScreen->optionCache,
                        sPriv->myNum, "mga");

   (void) memset( mmesa->texture_heaps, 0, sizeof( mmesa->texture_heaps ) );
   make_empty_list( & mmesa->swapped );

   mmesa->nr_heaps = mgaScreen->texVirtual[MGA_AGP_HEAP] ? 2 : 1;
   for ( i = 0 ; i < mmesa->nr_heaps ; i++ ) {
      mmesa->texture_heaps[i] = driCreateTextureHeap( i, mmesa,
	    mgaScreen->textureSize[i],
	    6,
	    MGA_NR_TEX_REGIONS,
	    (drmTextureRegionPtr)mmesa->sarea->texList[i],
	    &mmesa->sarea->texAge[i],
	    &mmesa->swapped,
	    sizeof( mgaTextureObject_t ),
	    (destroy_texture_object_t *) mgaDestroyTexObj );
   }

   /* Set the maximum texture size small enough that we can guarentee
    * that both texture units can bind a maximal texture and have them
    * on the card at once.
    */
   ctx = mmesa->glCtx;
   if ( mgaScreen->chipset == MGA_CARD_TYPE_G200 ) {
      ctx->Const.MaxTextureUnits = 1;
      ctx->Const.MaxTextureImageUnits = 1;
      ctx->Const.MaxTextureCoordUnits = 1;
      maxlevels = G200_TEX_MAXLEVELS;

   }
   else {
      ctx->Const.MaxTextureUnits = 2;
      ctx->Const.MaxTextureImageUnits = 2;
      ctx->Const.MaxTextureCoordUnits = 2;
      maxlevels = G400_TEX_MAXLEVELS;
   }

   driCalculateMaxTextureLevels( mmesa->texture_heaps,
				 mmesa->nr_heaps,
				 & ctx->Const,
				 4,
				 11, /* max 2D texture size is 2048x2048 */
				 0,  /* 3D textures unsupported. */
				 0,  /* cube textures unsupported. */
				 11, /* max texture rect size is 2048x2048 */
				 maxlevels,
				 GL_FALSE,
				 0 );

   ctx->Const.MinLineWidth = 1.0;
   ctx->Const.MinLineWidthAA = 1.0;
   ctx->Const.MaxLineWidth = 10.0;
   ctx->Const.MaxLineWidthAA = 10.0;
   ctx->Const.LineWidthGranularity = 1.0;

   mmesa->texture_depth = driQueryOptioni (&mmesa->optionCache,
					   "texture_depth");
   if (mmesa->texture_depth == DRI_CONF_TEXTURE_DEPTH_FB)
      mmesa->texture_depth = ( mesaVis->rgbBits >= 24 ) ?
	 DRI_CONF_TEXTURE_DEPTH_32 : DRI_CONF_TEXTURE_DEPTH_16;
   mmesa->hw_stencil = mesaVis->stencilBits && mesaVis->depthBits == 24;

   switch (mesaVis->depthBits) {
   case 16: 
      mmesa->depth_scale = 1.0/(GLdouble)0xffff; 
      mmesa->depth_clear_mask = ~0;
      mmesa->ClearDepth = 0xffff;
      break;
   case 24:
      mmesa->depth_scale = 1.0/(GLdouble)0xffffff;
      if (mmesa->hw_stencil) {
	 mmesa->depth_clear_mask = 0xffffff00;
	 mmesa->stencil_clear_mask = 0x000000ff;
      } else
	 mmesa->depth_clear_mask = ~0;
      mmesa->ClearDepth = 0xffffff00;
      break;
   case 32:
      mmesa->depth_scale = 1.0/(GLdouble)0xffffffff;
      mmesa->depth_clear_mask = ~0;
      mmesa->ClearDepth = 0xffffffff;
      break;
   };

   mmesa->haveHwStipple = GL_FALSE;
   mmesa->RenderIndex = -1;		/* impossible value */
   mmesa->dirty = ~0;
   mmesa->vertex_format = 0;   
   mmesa->CurrentTexObj[0] = 0;
   mmesa->CurrentTexObj[1] = 0;
   mmesa->tmu_source[0] = 0;
   mmesa->tmu_source[1] = 1;

   mmesa->texAge[0] = 0;
   mmesa->texAge[1] = 0;
   
   /* Initialize the software rasterizer and helper modules.
    */
   _swrast_CreateContext( ctx );
   _vbo_CreateContext( ctx );
   _tnl_CreateContext( ctx );
   
   _swsetup_CreateContext( ctx );

   /* Install the customized pipeline:
    */
   _tnl_destroy_pipeline( ctx );
   _tnl_install_pipeline( ctx, mga_pipeline );

   /* Configure swrast and T&L to match hardware characteristics:
    */
   _swrast_allow_pixel_fog( ctx, GL_FALSE );
   _swrast_allow_vertex_fog( ctx, GL_TRUE );
   _tnl_allow_pixel_fog( ctx, GL_FALSE );
   _tnl_allow_vertex_fog( ctx, GL_TRUE );

   mmesa->primary_offset = mmesa->mgaScreen->primary.handle;

   ctx->DriverCtx = (void *) mmesa;
   mmesa->glCtx = ctx;

   driInitExtensions( ctx, card_extensions, GL_FALSE );

   if (MGA_IS_G400(MGA_CONTEXT(ctx))) {
      driInitExtensions( ctx, g400_extensions, GL_FALSE );
   }

   if ( driQueryOptionb( &mmesa->optionCache, "arb_vertex_program" ) ) {
      driInitSingleExtension( ctx, ARB_vp_extension );
   }
   
   if ( driQueryOptionb( &mmesa->optionCache, "nv_vertex_program" ) ) {
      driInitExtensions( ctx, NV_vp_extensions, GL_FALSE );
   }

	
   /* XXX these should really go right after _mesa_init_driver_functions() */
   mgaDDInitStateFuncs( ctx );
   mgaDDInitSpanFuncs( ctx );
   mgaDDInitPixelFuncs( ctx );
   mgaDDInitTriFuncs( ctx );

   mgaInitVB( ctx );
   mgaInitState( mmesa );

   driContextPriv->driverPrivate = (void *) mmesa;

#if DO_DEBUG
   MGA_DEBUG = driParseDebugString( getenv( "MGA_DEBUG" ),
				    debug_control );
#endif

   mmesa->vblank_flags = (mmesa->mgaScreen->irq == 0)
       ? VBLANK_FLAG_NO_IRQ : driGetDefaultVBlankFlags(&mmesa->optionCache);

   (*dri_interface->getUST)( & mmesa->swap_ust );

   if (driQueryOptionb(&mmesa->optionCache, "no_rast")) {
      fprintf(stderr, "disabling 3D acceleration\n");
      FALLBACK(mmesa->glCtx, MGA_FALLBACK_DISABLE, 1);
   }

   return GL_TRUE;
}

static void
mgaDestroyContext(__DRIcontextPrivate *driContextPriv)
{
   mgaContextPtr mmesa = (mgaContextPtr) driContextPriv->driverPrivate;

   if (MGA_DEBUG&DEBUG_VERBOSE_DRI)
      fprintf( stderr, "[%s:%d] mgaDestroyContext start\n",
	       __FILE__, __LINE__ );

   assert(mmesa); /* should never be null */
   if (mmesa) {
      GLboolean   release_texture_heaps;


      release_texture_heaps = (mmesa->glCtx->Shared->RefCount == 1);
      _swsetup_DestroyContext( mmesa->glCtx );
      _tnl_DestroyContext( mmesa->glCtx );
      _vbo_DestroyContext( mmesa->glCtx );
      _swrast_DestroyContext( mmesa->glCtx );

      mgaFreeVB( mmesa->glCtx );

      /* free the Mesa context */
      mmesa->glCtx->DriverCtx = NULL;
      _mesa_destroy_context(mmesa->glCtx);
       
      if ( release_texture_heaps ) {
         /* This share group is about to go away, free our private
          * texture object data.
          */
         int i;

         for ( i = 0 ; i < mmesa->nr_heaps ; i++ ) {
	    driDestroyTextureHeap( mmesa->texture_heaps[ i ] );
	    mmesa->texture_heaps[ i ] = NULL;
         }

	 assert( is_empty_list( & mmesa->swapped ) );
      }

      /* free the option cache */
      driDestroyOptionCache (&mmesa->optionCache);

      FREE(mmesa);
   }

   if (MGA_DEBUG&DEBUG_VERBOSE_DRI)
      fprintf( stderr, "[%s:%d] mgaDestroyContext done\n",
	       __FILE__, __LINE__ );
}


static GLboolean
mgaCreateBuffer( __DRIscreenPrivate *driScrnPriv,
                 __DRIdrawablePrivate *driDrawPriv,
                 const __GLcontextModes *mesaVis,
                 GLboolean isPixmap )
{
   mgaScreenPrivate *screen = (mgaScreenPrivate *) driScrnPriv->private;

   if (isPixmap) {
      return GL_FALSE; /* not implemented */
   }
   else {
      GLboolean swStencil = (mesaVis->stencilBits > 0 && 
			     mesaVis->depthBits != 24);

#if 0
      driDrawPriv->driverPrivate = (void *) 
         _mesa_create_framebuffer(mesaVis,
                                  GL_FALSE,  /* software depth buffer? */
                                  swStencil,
                                  mesaVis->accumRedBits > 0,
                                  mesaVis->alphaBits > 0 );
#else
      struct gl_framebuffer *fb = _mesa_create_framebuffer(mesaVis);

      {
         driRenderbuffer *frontRb
            = driNewRenderbuffer(GL_RGBA,
                                 NULL,
                                 screen->cpp,
                                 screen->frontOffset, screen->frontPitch,
                                 driDrawPriv);
         mgaSetSpanFunctions(frontRb, mesaVis);
         _mesa_add_renderbuffer(fb, BUFFER_FRONT_LEFT, &frontRb->Base);
      }

      if (mesaVis->doubleBufferMode) {
         driRenderbuffer *backRb
            = driNewRenderbuffer(GL_RGBA,
                                 NULL,
                                 screen->cpp,
                                 screen->backOffset, screen->backPitch,
                                 driDrawPriv);
         mgaSetSpanFunctions(backRb, mesaVis);
         _mesa_add_renderbuffer(fb, BUFFER_BACK_LEFT, &backRb->Base);
      }

      if (mesaVis->depthBits == 16) {
         driRenderbuffer *depthRb
            = driNewRenderbuffer(GL_DEPTH_COMPONENT16,
                                 NULL,
                                 screen->cpp,
                                 screen->depthOffset, screen->depthPitch,
                                 driDrawPriv);
         mgaSetSpanFunctions(depthRb, mesaVis);
         _mesa_add_renderbuffer(fb, BUFFER_DEPTH, &depthRb->Base);
      }
      else if (mesaVis->depthBits == 24) {
         /* XXX is this right? */
         if (mesaVis->stencilBits) {
            driRenderbuffer *depthRb
               = driNewRenderbuffer(GL_DEPTH_COMPONENT24,
                                    NULL,
                                    screen->cpp,
                                    screen->depthOffset, screen->depthPitch,
                                    driDrawPriv);
            mgaSetSpanFunctions(depthRb, mesaVis);
            _mesa_add_renderbuffer(fb, BUFFER_DEPTH, &depthRb->Base);
         }
         else {
            driRenderbuffer *depthRb
               = driNewRenderbuffer(GL_DEPTH_COMPONENT32,
                                    NULL,
                                    screen->cpp,
                                    screen->depthOffset, screen->depthPitch,
                                    driDrawPriv);
            mgaSetSpanFunctions(depthRb, mesaVis);
            _mesa_add_renderbuffer(fb, BUFFER_DEPTH, &depthRb->Base);
         }
      }
      else if (mesaVis->depthBits == 32) {
         driRenderbuffer *depthRb
            = driNewRenderbuffer(GL_DEPTH_COMPONENT32,
                                 NULL,
                                 screen->cpp,
                                 screen->depthOffset, screen->depthPitch,
                                 driDrawPriv);
         mgaSetSpanFunctions(depthRb, mesaVis);
         _mesa_add_renderbuffer(fb, BUFFER_DEPTH, &depthRb->Base);
      }

      if (mesaVis->stencilBits > 0 && !swStencil) {
         driRenderbuffer *stencilRb
            = driNewRenderbuffer(GL_STENCIL_INDEX8_EXT,
                                 NULL,
                                 screen->cpp,
                                 screen->depthOffset, screen->depthPitch,
                                 driDrawPriv);
         mgaSetSpanFunctions(stencilRb, mesaVis);
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
#endif

      return (driDrawPriv->driverPrivate != NULL);
   }
}


static void
mgaDestroyBuffer(__DRIdrawablePrivate *driDrawPriv)
{
   _mesa_unreference_framebuffer((GLframebuffer **)(&(driDrawPriv->driverPrivate)));
}

static void
mgaSwapBuffers(__DRIdrawablePrivate *dPriv)
{
   if (dPriv->driContextPriv && dPriv->driContextPriv->driverPrivate) {
      mgaContextPtr mmesa;
      GLcontext *ctx;
      mmesa = (mgaContextPtr) dPriv->driContextPriv->driverPrivate;
      ctx = mmesa->glCtx;

      if (ctx->Visual.doubleBufferMode) {
         _mesa_notifySwapBuffers( ctx );
         mgaCopyBuffer( dPriv );
      }
   } else {
      /* XXX this shouldn't be an error but we can't handle it for now */
      _mesa_problem(NULL, "%s: drawable has no context!\n", __FUNCTION__);
   }
}

static GLboolean
mgaUnbindContext(__DRIcontextPrivate *driContextPriv)
{
   mgaContextPtr mmesa = (mgaContextPtr) driContextPriv->driverPrivate;
   if (mmesa)
      mmesa->dirty = ~0;

   return GL_TRUE;
}

/* This looks buggy to me - the 'b' variable isn't used anywhere...
 * Hmm - It seems that the drawable is already hooked in to
 * driDrawablePriv.
 *
 * But why are we doing context initialization here???
 */
static GLboolean
mgaMakeCurrent(__DRIcontextPrivate *driContextPriv,
               __DRIdrawablePrivate *driDrawPriv,
               __DRIdrawablePrivate *driReadPriv)
{
   if (driContextPriv) {
      mgaContextPtr mmesa = (mgaContextPtr) driContextPriv->driverPrivate;

      if (mmesa->driDrawable != driDrawPriv) {
	 driDrawableInitVBlank( driDrawPriv, mmesa->vblank_flags,
				&mmesa->vbl_seq );
	 mmesa->driDrawable = driDrawPriv;
	 mmesa->dirty = ~0; 
	 mmesa->dirty_cliprects = (MGA_FRONT|MGA_BACK); 
      }

      mmesa->driReadable = driReadPriv;

      _mesa_make_current(mmesa->glCtx,
                         (GLframebuffer *) driDrawPriv->driverPrivate,
                         (GLframebuffer *) driReadPriv->driverPrivate);
   }
   else {
      _mesa_make_current(NULL, NULL, NULL);
   }

   return GL_TRUE;
}


void mgaGetLock( mgaContextPtr mmesa, GLuint flags )
{
   __DRIdrawablePrivate *dPriv = mmesa->driDrawable;
   drm_mga_sarea_t *sarea = mmesa->sarea;
   int me = mmesa->hHWContext;
   int i;

   drmGetLock(mmesa->driFd, mmesa->hHWContext, flags);

   DRI_VALIDATE_DRAWABLE_INFO( mmesa->driScreen, dPriv );
   if (*(dPriv->pStamp) != mmesa->lastStamp) {
      mmesa->lastStamp = *(dPriv->pStamp);
      mmesa->SetupNewInputs |= VERT_BIT_POS;
      mmesa->dirty_cliprects = (MGA_FRONT|MGA_BACK);
      mgaUpdateRects( mmesa, (MGA_FRONT|MGA_BACK) );
      driUpdateFramebufferSize(mmesa->glCtx, dPriv);
   }

   mmesa->dirty |= MGA_UPLOAD_CONTEXT | MGA_UPLOAD_CLIPRECTS;

   mmesa->sarea->dirty |= MGA_UPLOAD_CONTEXT;

   if (sarea->ctxOwner != me) {
      mmesa->dirty |= (MGA_UPLOAD_CONTEXT | MGA_UPLOAD_TEX0 |
		       MGA_UPLOAD_TEX1 | MGA_UPLOAD_PIPE);
      sarea->ctxOwner=me;
   }

   for ( i = 0 ; i < mmesa->nr_heaps ; i++ ) {
      DRI_AGE_TEXTURES( mmesa->texture_heaps[ i ] );
   }
}


static const struct __DriverAPIRec mgaAPI = {
   .InitDriver      = mgaInitDriver,
   .DestroyScreen   = mgaDestroyScreen,
   .CreateContext   = mgaCreateContext,
   .DestroyContext  = mgaDestroyContext,
   .CreateBuffer    = mgaCreateBuffer,
   .DestroyBuffer   = mgaDestroyBuffer,
   .SwapBuffers     = mgaSwapBuffers,
   .MakeCurrent     = mgaMakeCurrent,
   .UnbindContext   = mgaUnbindContext,
   .GetSwapInfo     = getSwapInfo,
   .GetMSC          = driGetMSC32,
   .WaitForMSC      = driWaitForMSC32,
   .WaitForSBC      = NULL,
   .SwapBuffersMSC  = NULL
};


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
   static const __DRIversion ddx_expected = { 1, 2, 0 };
   static const __DRIversion dri_expected = { 4, 0, 0 };
   static const __DRIversion drm_expected = { 3, 0, 0 };

   dri_interface = interface;

   if ( ! driCheckDriDdxDrmVersions2( "MGA",
				      dri_version, & dri_expected,
				      ddx_version, & ddx_expected,
				      drm_version, & drm_expected ) ) {
      return NULL;
   }

   psp = __driUtilCreateNewScreen(dpy, scrn, psc, NULL,
				  ddx_version, dri_version, drm_version,
				  frame_buffer, pSAREA, fd,
				  internal_api_version, &mgaAPI);
   if ( psp != NULL ) {
      MGADRIPtr dri_priv = (MGADRIPtr) psp->pDevPriv;
      *driver_modes = mgaFillInModes( dri_priv->cpp * 8,
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
      driInitExtensions( NULL, g400_extensions, GL_FALSE );
      driInitSingleExtension( NULL, ARB_vp_extension );
      driInitExtensions( NULL, NV_vp_extensions, GL_FALSE );

   }

   return (void *) psp;
}


/**
 * Get information about previous buffer swaps.
 */
static int
getSwapInfo( __DRIdrawablePrivate *dPriv, __DRIswapInfo * sInfo )
{
   mgaContextPtr  mmesa;

   if ( (dPriv == NULL) || (dPriv->driContextPriv == NULL)
	|| (dPriv->driContextPriv->driverPrivate == NULL)
	|| (sInfo == NULL) ) {
      return -1;
   }

   mmesa = (mgaContextPtr) dPriv->driContextPriv->driverPrivate;
   sInfo->swap_count = mmesa->swap_count;
   sInfo->swap_ust = mmesa->swap_ust;
   sInfo->swap_missed_count = mmesa->swap_missed_count;

   sInfo->swap_missed_usage = (sInfo->swap_missed_count != 0)
       ? driCalculateSwapUsage( dPriv, 0, mmesa->swap_missed_ust )
       : 0.0;

   return 0;
}
