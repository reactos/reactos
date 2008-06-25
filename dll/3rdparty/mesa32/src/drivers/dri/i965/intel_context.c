/**************************************************************************
 * 
 * Copyright 2003 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 **************************************************************************/


#include "glheader.h"
#include "context.h"
#include "matrix.h"
#include "simple_list.h"
#include "extensions.h"
#include "framebuffer.h"
#include "imports.h"
#include "points.h"

#include "swrast/swrast.h"
#include "swrast_setup/swrast_setup.h"
#include "tnl/tnl.h"
#include "vbo/vbo.h"

#include "tnl/t_pipeline.h"
#include "tnl/t_vertex.h"

#include "drivers/common/driverfuncs.h"

#include "intel_screen.h"

#include "i830_dri.h"
#include "i830_common.h"

#include "intel_tex.h"
#include "intel_span.h"
#include "intel_ioctl.h"
#include "intel_batchbuffer.h"
#include "intel_blit.h"
#include "intel_regions.h"
#include "intel_buffer_objects.h"

#include "bufmgr.h"

#include "utils.h"
#include "vblank.h"
#ifndef INTEL_DEBUG
int INTEL_DEBUG = (0);
#endif

#define need_GL_ARB_multisample
#define need_GL_ARB_point_parameters
#define need_GL_ARB_texture_compression
#define need_GL_ARB_vertex_buffer_object
#define need_GL_ARB_vertex_program
#define need_GL_ARB_window_pos
#define need_GL_ARB_occlusion_query
#define need_GL_EXT_blend_color
#define need_GL_EXT_blend_equation_separate
#define need_GL_EXT_blend_func_separate
#define need_GL_EXT_blend_minmax
#define need_GL_EXT_cull_vertex
#define need_GL_EXT_fog_coord
#define need_GL_EXT_multi_draw_arrays
#define need_GL_EXT_secondary_color
#include "extension_helper.h"

#ifndef VERBOSE
int VERBOSE = 0;
#endif

/***************************************
 * Mesa's Driver Functions
 ***************************************/

#define DRIVER_VERSION                     "4.1.3002"

static const GLubyte *intelGetString( GLcontext *ctx, GLenum name )
{
   const char * chipset;
   static char buffer[128];

   switch (name) {
   case GL_VENDOR:
      return (GLubyte *)"Tungsten Graphics, Inc";
      break;
      
   case GL_RENDERER:
      switch (intel_context(ctx)->intelScreen->deviceID) {
      case PCI_CHIP_I965_Q:
	 chipset = "Intel(R) 965Q"; break;
         break;
      case PCI_CHIP_I965_G:
      case PCI_CHIP_I965_G_1:
	 chipset = "Intel(R) 965G"; break;
         break;
      case PCI_CHIP_I946_GZ:
	 chipset = "Intel(R) 946GZ"; break;
         break;
      case PCI_CHIP_I965_GM:
	 chipset = "Intel(R) 965GM"; break;
         break;
      default:
	 chipset = "Unknown Intel Chipset"; break;
      }

      (void) driGetRendererString( buffer, chipset, DRIVER_VERSION, 0 );
      return (GLubyte *) buffer;

   default:
      return NULL;
   }
}


/**
 * Extension strings exported by the intel driver.
 *
 * \note
 * It appears that ARB_texture_env_crossbar has "disappeared" compared to the
 * old i830-specific driver.
 */
const struct dri_extension card_extensions[] =
{
    { "GL_ARB_multisample",                GL_ARB_multisample_functions },
    { "GL_ARB_multitexture",               NULL },
    { "GL_ARB_point_parameters",           GL_ARB_point_parameters_functions },
    { "GL_ARB_texture_border_clamp",       NULL },
    { "GL_ARB_texture_compression",        GL_ARB_texture_compression_functions },
    { "GL_ARB_texture_cube_map",           NULL },
    { "GL_ARB_texture_env_add",            NULL },
    { "GL_ARB_texture_env_combine",        NULL },
    { "GL_ARB_texture_env_dot3",           NULL },
    { "GL_ARB_texture_mirrored_repeat",    NULL },
    { "GL_ARB_texture_non_power_of_two",   NULL },
    { "GL_ARB_texture_rectangle",          NULL },
    { "GL_NV_texture_rectangle",           NULL },
    { "GL_EXT_texture_rectangle",          NULL },
    { "GL_ARB_texture_rectangle",          NULL },
    { "GL_ARB_vertex_buffer_object",       GL_ARB_vertex_buffer_object_functions },
    { "GL_ARB_vertex_program",             GL_ARB_vertex_program_functions },
    { "GL_ARB_window_pos",                 GL_ARB_window_pos_functions },
    { "GL_EXT_blend_color",                GL_EXT_blend_color_functions },
    { "GL_EXT_blend_equation_separate",    GL_EXT_blend_equation_separate_functions },
    { "GL_EXT_blend_func_separate",        GL_EXT_blend_func_separate_functions },
    { "GL_EXT_blend_minmax",               GL_EXT_blend_minmax_functions },
    { "GL_EXT_blend_logic_op",             NULL },
    { "GL_EXT_blend_subtract",             NULL },
    { "GL_EXT_cull_vertex",                GL_EXT_cull_vertex_functions },
    { "GL_EXT_fog_coord",                  GL_EXT_fog_coord_functions },
    { "GL_EXT_multi_draw_arrays",          GL_EXT_multi_draw_arrays_functions },
    { "GL_EXT_secondary_color",            GL_EXT_secondary_color_functions },
    { "GL_EXT_stencil_wrap",               NULL },
    { "GL_EXT_texture_edge_clamp",         NULL },
    { "GL_EXT_texture_env_combine",        NULL },
    { "GL_EXT_texture_env_dot3",           NULL },
    { "GL_EXT_texture_filter_anisotropic", NULL },
    { "GL_EXT_texture_lod_bias",           NULL },
    { "GL_3DFX_texture_compression_FXT1",  NULL },
    { "GL_APPLE_client_storage",           NULL },
    { "GL_MESA_pack_invert",               NULL },
    { "GL_MESA_ycbcr_texture",             NULL },
    { "GL_NV_blend_square",                NULL },
    { "GL_SGIS_generate_mipmap",           NULL },
    { NULL,                                NULL }
};

const struct dri_extension arb_oc_extension = 
    { "GL_ARB_occlusion_query",            GL_ARB_occlusion_query_functions};

void intelInitExtensions(GLcontext *ctx, GLboolean enable_imaging)
{	     
	struct intel_context *intel = ctx?intel_context(ctx):NULL;
	driInitExtensions(ctx, card_extensions, enable_imaging);
	if (!ctx || intel->intelScreen->drmMinor >= 8)
		driInitSingleExtension (ctx, &arb_oc_extension);
}

static const struct dri_debug_control debug_control[] =
{
    { "fall",  DEBUG_FALLBACKS },
    { "tex",   DEBUG_TEXTURE },
    { "ioctl", DEBUG_IOCTL },
    { "prim",  DEBUG_PRIMS },
    { "vert",  DEBUG_VERTS },
    { "state", DEBUG_STATE },
    { "verb",  DEBUG_VERBOSE },
    { "dri",   DEBUG_DRI },
    { "dma",   DEBUG_DMA },
    { "san",   DEBUG_SANITY },
    { "sync",  DEBUG_SYNC },
    { "sleep", DEBUG_SLEEP },
    { "pix",   DEBUG_PIXEL },
    { "buf",   DEBUG_BUFMGR },
    { "stats", DEBUG_STATS },
    { "tile",  DEBUG_TILE },
    { "sing",  DEBUG_SINGLE_THREAD },
    { "thre",  DEBUG_SINGLE_THREAD },
    { "wm",    DEBUG_WM },
    { "vs",    DEBUG_VS },
    { NULL,    0 }
};


static void intelInvalidateState( GLcontext *ctx, GLuint new_state )
{
   struct intel_context *intel = intel_context(ctx);

   _swrast_InvalidateState( ctx, new_state );
   _swsetup_InvalidateState( ctx, new_state );
   _vbo_InvalidateState( ctx, new_state );
   _tnl_InvalidateState( ctx, new_state );
   _tnl_invalidate_vertex_state( ctx, new_state );
   
   intel->NewGLState |= new_state;

   if (intel->vtbl.invalidate_state)
      intel->vtbl.invalidate_state( intel, new_state );
}


void intelFlush( GLcontext *ctx )
{
   struct intel_context *intel = intel_context( ctx );

   bmLockAndFence(intel);
}

void intelFinish( GLcontext *ctx ) 
{
   struct intel_context *intel = intel_context( ctx );

   bmFinishFence(intel, bmLockAndFence(intel));
}

static void
intelBeginQuery(GLcontext *ctx, GLenum target, struct gl_query_object *q)
{
	struct intel_context *intel = intel_context( ctx );
	drmI830MMIO io = {
		.read_write = MMIO_READ,
		.reg = MMIO_REGS_PS_DEPTH_COUNT,
		.data = &q->Result 
	};
	intel->stats_wm++;
	intelFinish(&intel->ctx);
	drmCommandWrite(intel->driFd, DRM_I830_MMIO, &io, sizeof(io));
}

static void
intelEndQuery(GLcontext *ctx, GLenum target, struct gl_query_object *q)
{
	struct intel_context *intel = intel_context( ctx );
	GLuint64EXT tmp;	
	drmI830MMIO io = {
		.read_write = MMIO_READ,
		.reg = MMIO_REGS_PS_DEPTH_COUNT,
		.data = &tmp
	};
	intelFinish(&intel->ctx);
	drmCommandWrite(intel->driFd, DRM_I830_MMIO, &io, sizeof(io));
	q->Result = tmp - q->Result;
	q->Ready = GL_TRUE;
	intel->stats_wm--;
}


void intelInitDriverFunctions( struct dd_function_table *functions )
{
   _mesa_init_driver_functions( functions );

   functions->Flush = intelFlush;
   functions->Finish = intelFinish;
   functions->GetString = intelGetString;
   functions->UpdateState = intelInvalidateState;
   functions->BeginQuery = intelBeginQuery;
   functions->EndQuery = intelEndQuery;

   /* CopyPixels can be accelerated even with the current memory
    * manager:
    */
   if (!getenv("INTEL_NO_BLIT")) {
      functions->CopyPixels = intelCopyPixels;
      functions->Bitmap = intelBitmap;
   }

   intelInitTextureFuncs( functions );
   intelInitStateFuncs( functions );
   intelInitBufferFuncs( functions );
}



GLboolean intelInitContext( struct intel_context *intel,
			    const __GLcontextModes *mesaVis,
			    __DRIcontextPrivate *driContextPriv,
			    void *sharedContextPrivate,
			    struct dd_function_table *functions )
{
   GLcontext *ctx = &intel->ctx;
   GLcontext *shareCtx = (GLcontext *) sharedContextPrivate;
   __DRIscreenPrivate *sPriv = driContextPriv->driScreenPriv;
   intelScreenPrivate *intelScreen = (intelScreenPrivate *)sPriv->private;
   volatile drmI830Sarea *saPriv = (volatile drmI830Sarea *)
      (((GLubyte *)sPriv->pSAREA)+intelScreen->sarea_priv_offset);

   if (!_mesa_initialize_context(&intel->ctx,
				 mesaVis, shareCtx, 
				 functions,
				 (void*) intel)) {
      _mesa_printf("%s: failed to init mesa context\n", __FUNCTION__);
      return GL_FALSE;
   }

   driContextPriv->driverPrivate = intel;
   intel->intelScreen = intelScreen;
   intel->driScreen = sPriv;
   intel->sarea = saPriv;

   driParseConfigFiles (&intel->optionCache, &intelScreen->optionCache,
		   intel->driScreen->myNum, "i965");

   intel->vblank_flags = (intel->intelScreen->irq_active != 0)
	   ? driGetDefaultVBlankFlags(&intel->optionCache) : VBLANK_FLAG_NO_IRQ;

   ctx->Const.MaxTextureMaxAnisotropy = 2.0;

   if (getenv("INTEL_STRICT_CONFORMANCE")) {
      intel->strict_conformance = 1;
   }

   if (intel->strict_conformance) {
      ctx->Const.MinLineWidth = 1.0;
      ctx->Const.MinLineWidthAA = 1.0;
      ctx->Const.MaxLineWidth = 1.0;
      ctx->Const.MaxLineWidthAA = 1.0;
      ctx->Const.LineWidthGranularity = 1.0;
   }
   else {
      ctx->Const.MinLineWidth = 1.0;
      ctx->Const.MinLineWidthAA = 1.0;
      ctx->Const.MaxLineWidth = 5.0;
      ctx->Const.MaxLineWidthAA = 5.0;
      ctx->Const.LineWidthGranularity = 0.5;
   }

   ctx->Const.MinPointSize = 1.0;
   ctx->Const.MinPointSizeAA = 1.0;
   ctx->Const.MaxPointSize = 255.0;
   ctx->Const.MaxPointSizeAA = 3.0;
   ctx->Const.PointSizeGranularity = 1.0;

   /* reinitialize the context point state.
    * It depend on constants in __GLcontextRec::Const
    */
   _mesa_init_point(ctx);

   /* Initialize the software rasterizer and helper modules. */
   _swrast_CreateContext( ctx );
   _vbo_CreateContext( ctx );
   _tnl_CreateContext( ctx );
   _swsetup_CreateContext( ctx );

   TNL_CONTEXT(ctx)->Driver.RunPipeline = _tnl_run_pipeline;

   /* Configure swrast to match hardware characteristics: */
   _swrast_allow_pixel_fog( ctx, GL_FALSE );
   _swrast_allow_vertex_fog( ctx, GL_TRUE );

   /* Dri stuff */
   intel->hHWContext = driContextPriv->hHWContext;
   intel->driFd = sPriv->fd;
   intel->driHwLock = (drmLock *) &sPriv->pSAREA->lock;

   intel->hw_stencil = mesaVis->stencilBits && mesaVis->depthBits == 24;
   intel->hw_stipple = 1;

   switch(mesaVis->depthBits) {
   case 0:			/* what to do in this case? */
   case 16:
      intel->depth_scale = 1.0/0xffff;
      intel->polygon_offset_scale = 1.0/0xffff;
      intel->depth_clear_mask = ~0;
      intel->ClearDepth = 0xffff;
      break;
   case 24:
      intel->depth_scale = 1.0/0xffffff;
      intel->polygon_offset_scale = 2.0/0xffffff; /* req'd to pass glean */
      intel->depth_clear_mask = 0x00ffffff;
      intel->stencil_clear_mask = 0xff000000;
      intel->ClearDepth = 0x00ffffff;
      break;
   default:
      assert(0); 
      break;
   }

   /* Initialize swrast, tnl driver tables: */
   intelInitSpanFuncs( ctx );

   intel->no_hw = getenv("INTEL_NO_HW") != NULL;

   if (!intel->intelScreen->irq_active) {
      _mesa_printf("IRQs not active.  Exiting\n");
      exit(1);
   }
   intelInitExtensions(ctx, GL_TRUE); 

   INTEL_DEBUG  = driParseDebugString( getenv( "INTEL_DEBUG" ),
				       debug_control );


   /* Buffer manager: 
    */
   intel->bm = bm_fake_intel_Attach( intel );


   bmInitPool(intel,
	      intel->intelScreen->tex.offset, /* low offset */
	      intel->intelScreen->tex.map, /* low virtual */
	      intel->intelScreen->tex.size,
	      BM_MEM_AGP);

   /* These are still static, but create regions for them.  
    */
   intel->front_region = 
      intel_region_create_static(intel,
				 BM_MEM_AGP,
				 intelScreen->front.offset,
				 intelScreen->front.map,
				 intelScreen->cpp,
				 intelScreen->front.pitch / intelScreen->cpp,
				 intelScreen->height,
				 intelScreen->front.size,
				 intelScreen->front.tiled != 0);

   intel->back_region = 
      intel_region_create_static(intel,
				 BM_MEM_AGP,
				 intelScreen->back.offset,
				 intelScreen->back.map,
				 intelScreen->cpp,
				 intelScreen->back.pitch / intelScreen->cpp,
				 intelScreen->height,
				 intelScreen->back.size,
                                 intelScreen->back.tiled != 0);

   /* Still assuming front.cpp == depth.cpp
    *
    * XXX: Setting tiling to false because Depth tiling only supports
    * YMAJOR but the blitter only supports XMAJOR tiling.  Have to
    * resolve later.
    */
   intel->depth_region = 
      intel_region_create_static(intel,
				 BM_MEM_AGP,
				 intelScreen->depth.offset,
				 intelScreen->depth.map,
				 intelScreen->cpp,
				 intelScreen->depth.pitch / intelScreen->cpp,
				 intelScreen->height,
				 intelScreen->depth.size,
                                 intelScreen->depth.tiled != 0);
   
   intel_bufferobj_init( intel );
   intel->batch = intel_batchbuffer_alloc( intel );

   if (intel->ctx.Mesa_DXTn) {
      _mesa_enable_extension( ctx, "GL_EXT_texture_compression_s3tc" );
      _mesa_enable_extension( ctx, "GL_S3_s3tc" );
   }
   else if (driQueryOptionb (&intel->optionCache, "force_s3tc_enable")) {
      _mesa_enable_extension( ctx, "GL_EXT_texture_compression_s3tc" );
   }

/*    driInitTextureObjects( ctx, & intel->swapped, */
/* 			  DRI_TEXMGR_DO_TEXTURE_1D | */
/* 			  DRI_TEXMGR_DO_TEXTURE_2D |  */
/* 			  DRI_TEXMGR_DO_TEXTURE_RECT ); */


   if (getenv("INTEL_NO_RAST")) {
      fprintf(stderr, "disabling 3D rasterization\n");
      intel->no_rast = 1;
   }


   return GL_TRUE;
}

void intelDestroyContext(__DRIcontextPrivate *driContextPriv)
{
   struct intel_context *intel = (struct intel_context *) driContextPriv->driverPrivate;

   assert(intel); /* should never be null */
   if (intel) {
      GLboolean   release_texture_heaps;


      intel->vtbl.destroy( intel );

      release_texture_heaps = (intel->ctx.Shared->RefCount == 1);
      _swsetup_DestroyContext (&intel->ctx);
      _tnl_DestroyContext (&intel->ctx);
      _vbo_DestroyContext (&intel->ctx);

      _swrast_DestroyContext (&intel->ctx);
      intel->Fallback = 0;	/* don't call _swrast_Flush later */
      intel_batchbuffer_free(intel->batch);
      intel->batch = NULL;
      

      if ( release_texture_heaps ) {
         /* This share group is about to go away, free our private
          * texture object data.
          */

	 /* XXX: destroy the shared bufmgr struct here?
	  */
      }

      /* Free the regions created to describe front/back/depth
       * buffers:
       */
#if 0
      intel_region_release(intel, &intel->front_region);
      intel_region_release(intel, &intel->back_region);
      intel_region_release(intel, &intel->depth_region);
      intel_region_release(intel, &intel->draw_region);
#endif

      /* free the Mesa context */
      _mesa_destroy_context(&intel->ctx);
   }

   driContextPriv->driverPrivate = NULL;
}

GLboolean intelUnbindContext(__DRIcontextPrivate *driContextPriv)
{
   return GL_TRUE;
}

GLboolean intelMakeCurrent(__DRIcontextPrivate *driContextPriv,
			  __DRIdrawablePrivate *driDrawPriv,
			  __DRIdrawablePrivate *driReadPriv)
{

   if (driContextPriv) {
      struct intel_context *intel = (struct intel_context *) driContextPriv->driverPrivate;

      if (intel->driReadDrawable != driReadPriv) {
          intel->driReadDrawable = driReadPriv;
      }

      if ( intel->driDrawable != driDrawPriv ) {
	 /* Shouldn't the readbuffer be stored also? */
	 driDrawableInitVBlank( driDrawPriv, intel->vblank_flags,
		      &intel->vbl_seq );

	 intel->driDrawable = driDrawPriv;
	 intelWindowMoved( intel );
      }

      _mesa_make_current(&intel->ctx,
			 (GLframebuffer *) driDrawPriv->driverPrivate,
			 (GLframebuffer *) driReadPriv->driverPrivate);

      intel->ctx.Driver.DrawBuffer( &intel->ctx, intel->ctx.Color.DrawBuffer[0] );
   } else {
      _mesa_make_current(NULL, NULL, NULL);
   }

   return GL_TRUE;
}


static void intelContendedLock( struct intel_context *intel, GLuint flags )
{
   __DRIdrawablePrivate *dPriv = intel->driDrawable;
   __DRIscreenPrivate *sPriv = intel->driScreen;
   volatile drmI830Sarea * sarea = intel->sarea;
   int me = intel->hHWContext;
   int my_bufmgr = bmCtxId(intel);

   drmGetLock(intel->driFd, intel->hHWContext, flags);

   /* If the window moved, may need to set a new cliprect now.
    *
    * NOTE: This releases and regains the hw lock, so all state
    * checking must be done *after* this call:
    */
   if (dPriv)
      DRI_VALIDATE_DRAWABLE_INFO(sPriv, dPriv);


   intel->locked = 1;
   intel->need_flush = 1;

   /* Lost context?
    */
   if (sarea->ctxOwner != me) {
      DBG("Lost Context: sarea->ctxOwner %x me %x\n", sarea->ctxOwner, me);
      sarea->ctxOwner = me;
      intel->vtbl.lost_hardware( intel );
   }

   /* As above, but don't evict the texture data on transitions
    * between contexts which all share a local buffer manager.
    */
   if (sarea->texAge != my_bufmgr) {
      DBG("Lost Textures: sarea->texAge %x my_bufmgr %x\n", sarea->ctxOwner, my_bufmgr);
      sarea->texAge = my_bufmgr;
      bm_fake_NotifyContendedLockTake( intel ); 
   }

   /* Drawable changed?
    */
   if (dPriv && intel->lastStamp != dPriv->lastStamp) {
      intelWindowMoved( intel );
      intel->lastStamp = dPriv->lastStamp;
   }
}

_glthread_DECLARE_STATIC_MUTEX(lockMutex);

/* Lock the hardware and validate our state.  
 */
void LOCK_HARDWARE( struct intel_context *intel )
{
    char __ret=0;

    _glthread_LOCK_MUTEX(lockMutex);
    assert(!intel->locked);


    DRM_CAS(intel->driHwLock, intel->hHWContext,
	    (DRM_LOCK_HELD|intel->hHWContext), __ret);
    if (__ret)
        intelContendedLock( intel, 0 );

   intel->locked = 1;

   if (intel->aub_wrap) {
      bm_fake_NotifyContendedLockTake( intel ); 
      intel->vtbl.lost_hardware( intel );
      intel->vtbl.aub_wrap(intel);
      intel->aub_wrap = 0;
   }

   if (bmError(intel)) {
      bmEvictAll(intel);
      intel->vtbl.lost_hardware( intel );
   }

   /* Make sure nothing has been emitted prior to getting the lock: 
    */
   assert(intel->batch->map == 0);

   /* XXX: postpone, may not be needed:
    */
   if (!intel_batchbuffer_map(intel->batch)) {
      bmEvictAll(intel);
      intel->vtbl.lost_hardware( intel );

      /* This could only fail if the batchbuffer was greater in size
       * than the available texture memory:
       */
      if (!intel_batchbuffer_map(intel->batch)) {
	 _mesa_printf("double failure to map batchbuffer\n");
	 assert(0);
      }
   }
}
 
  
/* Unlock the hardware using the global current context 
 */
void UNLOCK_HARDWARE( struct intel_context *intel )
{
   /* Make sure everything has been released: 
    */
   assert(intel->batch->ptr == intel->batch->map + intel->batch->offset);

   intel_batchbuffer_unmap(intel->batch);
   intel->vtbl.note_unlock( intel );
   intel->locked = 0;



   DRM_UNLOCK(intel->driFd, intel->driHwLock, intel->hHWContext);
   _glthread_UNLOCK_MUTEX(lockMutex); 
}


