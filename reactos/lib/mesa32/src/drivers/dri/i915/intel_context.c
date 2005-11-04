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

#include "swrast/swrast.h"
#include "swrast_setup/swrast_setup.h"
#include "tnl/tnl.h"
#include "array_cache/acache.h"

#include "tnl/t_pipeline.h"
#include "tnl/t_vertex.h"

#include "drivers/common/driverfuncs.h"

#include "intel_screen.h"

#include "i830_dri.h"
#include "i830_common.h"

#include "intel_tex.h"
#include "intel_span.h"
#include "intel_tris.h"
#include "intel_ioctl.h"
#include "intel_batchbuffer.h"

#include "utils.h"
#ifndef INTEL_DEBUG
int INTEL_DEBUG = (0);
#endif

#define need_GL_ARB_multisample
#define need_GL_ARB_point_parameters
#define need_GL_ARB_texture_compression
#define need_GL_ARB_vertex_buffer_object
#define need_GL_ARB_vertex_program
#define need_GL_ARB_window_pos
#define need_GL_EXT_blend_color
#define need_GL_EXT_blend_equation_separate
#define need_GL_EXT_blend_func_separate
#define need_GL_EXT_blend_minmax
#define need_GL_EXT_cull_vertex
#define need_GL_EXT_fog_coord
#define need_GL_EXT_multi_draw_arrays
#define need_GL_EXT_secondary_color
#define need_GL_NV_vertex_program
#include "extension_helper.h"

#ifndef VERBOSE
int VERBOSE = 0;
#endif

#if DEBUG_LOCKING
char *prevLockFile;
int prevLockLine;
#endif

/***************************************
 * Mesa's Driver Functions
 ***************************************/

#define DRIVER_DATE                     "20050225"

const GLubyte *intelGetString( GLcontext *ctx, GLenum name )
{
   const char * chipset;
   static char buffer[128];

   switch (name) {
   case GL_VENDOR:
      return (GLubyte *)"Tungsten Graphics, Inc";
      break;
      
   case GL_RENDERER:
      switch (INTEL_CONTEXT(ctx)->intelScreen->deviceID) {
      case PCI_CHIP_845_G:
	 chipset = "Intel(R) 845G"; break;
      case PCI_CHIP_I830_M:
	 chipset = "Intel(R) 830M"; break;
      case PCI_CHIP_I855_GM:
	 chipset = "Intel(R) 852GM/855GM"; break;
      case PCI_CHIP_I865_G:
	 chipset = "Intel(R) 865G"; break;
      case PCI_CHIP_I915_G:
	 chipset = "Intel(R) 915G"; break;
      case PCI_CHIP_I915_GM:
	 chipset = "Intel(R) 915GM"; break;
      case PCI_CHIP_I945_G:
	 chipset = "Intel(R) 945G"; break;
      default:
	 chipset = "Unknown Intel Chipset"; break;
      }

      (void) driGetRendererString( buffer, chipset, DRIVER_DATE, 0 );
      return (GLubyte *) buffer;

   default:
      return NULL;
   }
}

static void intelBufferSize(GLframebuffer *buffer,
			   GLuint *width, GLuint *height)
{
   GET_CURRENT_CONTEXT(ctx);
   intelContextPtr intel = INTEL_CONTEXT(ctx);
   /* Need to lock to make sure the driDrawable is uptodate.  This
    * information is used to resize Mesa's software buffers, so it has
    * to be correct.
    */
   LOCK_HARDWARE(intel);
   *width = intel->driDrawable->w;
   *height = intel->driDrawable->h;
   UNLOCK_HARDWARE(intel);
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
    { "GL_ARB_texture_rectangle",          NULL },
    { "GL_ARB_vertex_buffer_object",       GL_ARB_vertex_buffer_object_functions },
    { "GL_ARB_vertex_program",             GL_ARB_vertex_program_functions },
    { "GL_ARB_window_pos",                 GL_ARB_window_pos_functions },
    { "GL_EXT_blend_color",                GL_EXT_blend_color_functions },
    { "GL_EXT_blend_equation_separate",    GL_EXT_blend_equation_separate_functions },
    { "GL_EXT_blend_func_separate",        GL_EXT_blend_func_separate_functions },
    { "GL_EXT_blend_minmax",               GL_EXT_blend_minmax_functions },
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
    { "GL_NV_vertex_program",              GL_NV_vertex_program_functions },
    { "GL_NV_vertex_program1_1",           NULL },
    { "GL_SGIS_generate_mipmap",           NULL },
    { NULL,                                NULL }
};

extern const struct tnl_pipeline_stage _intel_render_stage;

static const struct tnl_pipeline_stage *intel_pipeline[] = {
   &_tnl_vertex_transform_stage,
   &_tnl_vertex_cull_stage,
   &_tnl_normal_transform_stage,
   &_tnl_lighting_stage,
   &_tnl_fog_coordinate_stage,
   &_tnl_texgen_stage,
   &_tnl_texture_transform_stage,
   &_tnl_point_attenuation_stage,
   &_tnl_arb_vertex_program_stage,
   &_tnl_vertex_program_stage,
#if 1
   &_intel_render_stage,     /* ADD: unclipped rastersetup-to-dma */
#endif
   &_tnl_render_stage,
   0,
};


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
    { NULL,    0 }
};


static void intelInvalidateState( GLcontext *ctx, GLuint new_state )
{
   _swrast_InvalidateState( ctx, new_state );
   _swsetup_InvalidateState( ctx, new_state );
   _ac_InvalidateState( ctx, new_state );
   _tnl_InvalidateState( ctx, new_state );
   _tnl_invalidate_vertex_state( ctx, new_state );
   INTEL_CONTEXT(ctx)->NewGLState |= new_state;
}


void intelInitDriverFunctions( struct dd_function_table *functions )
{
   _mesa_init_driver_functions( functions );

   functions->Flush = intelFlush;
   functions->Clear = intelClear;
   functions->Finish = intelFinish;
   functions->GetBufferSize = intelBufferSize;
   functions->ResizeBuffers = _mesa_resize_framebuffer;
   functions->GetString = intelGetString;
   functions->UpdateState = intelInvalidateState;
   functions->CopyColorTable = _swrast_CopyColorTable;
   functions->CopyColorSubTable = _swrast_CopyColorSubTable;
   functions->CopyConvolutionFilter1D = _swrast_CopyConvolutionFilter1D;
   functions->CopyConvolutionFilter2D = _swrast_CopyConvolutionFilter2D;

   intelInitTextureFuncs( functions );
   intelInitPixelFuncs( functions );
   intelInitStateFuncs( functions );
}



GLboolean intelInitContext( intelContextPtr intel,
			    const __GLcontextModes *mesaVis,
			    __DRIcontextPrivate *driContextPriv,
			    void *sharedContextPrivate,
			    struct dd_function_table *functions )
{
   GLcontext *ctx = &intel->ctx;
   GLcontext *shareCtx = (GLcontext *) sharedContextPrivate;
   __DRIscreenPrivate *sPriv = driContextPriv->driScreenPriv;
   intelScreenPrivate *intelScreen = (intelScreenPrivate *)sPriv->private;
   drmI830Sarea *saPriv = (drmI830Sarea *)
      (((GLubyte *)sPriv->pSAREA)+intelScreen->sarea_priv_offset);

   if (!_mesa_initialize_context(&intel->ctx,
				 mesaVis, shareCtx, 
				 functions,
				 (void*) intel))
      return GL_FALSE;

   driContextPriv->driverPrivate = intel;
   intel->intelScreen = intelScreen;
   intel->driScreen = sPriv;
   intel->sarea = saPriv;


   (void) memset( intel->texture_heaps, 0, sizeof( intel->texture_heaps ) );
   make_empty_list( & intel->swapped );

   ctx->Const.MaxTextureMaxAnisotropy = 2.0;

   ctx->Const.MinLineWidth = 1.0;
   ctx->Const.MinLineWidthAA = 1.0;
   ctx->Const.MaxLineWidth = 3.0;
   ctx->Const.MaxLineWidthAA = 3.0;
   ctx->Const.LineWidthGranularity = 1.0;

   ctx->Const.MinPointSize = 1.0;
   ctx->Const.MinPointSizeAA = 1.0;
   ctx->Const.MaxPointSize = 255.0;
   ctx->Const.MaxPointSizeAA = 3.0;
   ctx->Const.PointSizeGranularity = 1.0;

   /* Initialize the software rasterizer and helper modules. */
   _swrast_CreateContext( ctx );
   _ac_CreateContext( ctx );
   _tnl_CreateContext( ctx );
   _swsetup_CreateContext( ctx );

   /* Install the customized pipeline: */
   _tnl_destroy_pipeline( ctx );
   _tnl_install_pipeline( ctx, intel_pipeline );

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
   intelInitTriFuncs( ctx );


   intel->RenderIndex = ~0;

   intel->do_irqs = (intel->intelScreen->irq_active &&
		     !getenv("INTEL_NO_IRQS"));

   _math_matrix_ctr (&intel->ViewportMatrix);

   driInitExtensions( ctx, card_extensions, GL_TRUE );

   if (intel->ctx.Mesa_DXTn) {
     _mesa_enable_extension( ctx, "GL_EXT_texture_compression_s3tc" );
     _mesa_enable_extension( ctx, "GL_S3_s3tc" );
   }
   else if (driQueryOptionb (&intelScreen->optionCache, "force_s3tc_enable")) {
     _mesa_enable_extension( ctx, "GL_EXT_texture_compression_s3tc" );
   }

/*    driInitTextureObjects( ctx, & intel->swapped, */
/* 			  DRI_TEXMGR_DO_TEXTURE_1D | */
/* 			  DRI_TEXMGR_DO_TEXTURE_2D |  */
/* 			  DRI_TEXMGR_DO_TEXTURE_RECT ); */


   intel->prim.flush = intelInitBatchBuffer;
   intel->prim.primitive = ~0;


#if DO_DEBUG
   INTEL_DEBUG  = driParseDebugString( getenv( "INTEL_DEBUG" ),
				       debug_control );
   INTEL_DEBUG |= driParseDebugString( getenv( "INTEL_DEBUG" ),
				       debug_control );
#endif

#ifndef VERBOSE
   if (getenv("INTEL_VERBOSE"))
      VERBOSE=1;
#endif

   if (getenv("INTEL_NO_RAST") || 
       getenv("INTEL_NO_RAST")) {
      fprintf(stderr, "disabling 3D rasterization\n");
      FALLBACK(intel, INTEL_FALLBACK_USER, 1); 
   }

   return GL_TRUE;
}

void intelDestroyContext(__DRIcontextPrivate *driContextPriv)
{
   intelContextPtr intel = (intelContextPtr) driContextPriv->driverPrivate;

   assert(intel); /* should never be null */
   if (intel) {
      GLboolean   release_texture_heaps;


      intel->vtbl.destroy( intel );

      release_texture_heaps = (intel->ctx.Shared->RefCount == 1);
      _swsetup_DestroyContext (&intel->ctx);
      _tnl_DestroyContext (&intel->ctx);
      _ac_DestroyContext (&intel->ctx);

      _swrast_DestroyContext (&intel->ctx);
      intel->Fallback = 0;	/* don't call _swrast_Flush later */

      intelDestroyBatchBuffer(&intel->ctx);
      

      if ( release_texture_heaps ) {
         /* This share group is about to go away, free our private
          * texture object data.
          */
         int i;

         for ( i = 0 ; i < intel->nr_heaps ; i++ ) {
	    driDestroyTextureHeap( intel->texture_heaps[ i ] );
	    intel->texture_heaps[ i ] = NULL;
         }

	 assert( is_empty_list( & intel->swapped ) );
      }

      /* free the Mesa context */
      _mesa_destroy_context(&intel->ctx);
   }
}

void intelSetFrontClipRects( intelContextPtr intel )
{
   __DRIdrawablePrivate *dPriv = intel->driDrawable;

   if (!dPriv) return;

   intel->numClipRects = dPriv->numClipRects;
   intel->pClipRects = dPriv->pClipRects;
   intel->drawX = dPriv->x;
   intel->drawY = dPriv->y;
}


void intelSetBackClipRects( intelContextPtr intel )
{
   __DRIdrawablePrivate *dPriv = intel->driDrawable;

   if (!dPriv) return;

   if (intel->sarea->pf_enabled == 0 && dPriv->numBackClipRects == 0) {
      intel->numClipRects = dPriv->numClipRects;
      intel->pClipRects = dPriv->pClipRects;
      intel->drawX = dPriv->x;
      intel->drawY = dPriv->y;
   } else {
      intel->numClipRects = dPriv->numBackClipRects;
      intel->pClipRects = dPriv->pBackClipRects;
      intel->drawX = dPriv->backX;
      intel->drawY = dPriv->backY;
      
      if (dPriv->numBackClipRects == 1 &&
	  dPriv->x == dPriv->backX &&
	  dPriv->y == dPriv->backY) {
      
	 /* Repeat the calculation of the back cliprect dimensions here
	  * as early versions of dri.a in the Xserver are incorrect.  Try
	  * very hard not to restrict future versions of dri.a which
	  * might eg. allocate truly private back buffers.
	  */
	 int x1, y1;
	 int x2, y2;
	 
	 x1 = dPriv->x;
	 y1 = dPriv->y;      
	 x2 = dPriv->x + dPriv->w;
	 y2 = dPriv->y + dPriv->h;
	 
	 if (x1 < 0) x1 = 0;
	 if (y1 < 0) y1 = 0;
	 if (x2 > intel->intelScreen->width) x2 = intel->intelScreen->width;
	 if (y2 > intel->intelScreen->height) y2 = intel->intelScreen->height;

	 if (x1 == dPriv->pBackClipRects[0].x1 &&
	     y1 == dPriv->pBackClipRects[0].y1) {

	    dPriv->pBackClipRects[0].x2 = x2;
	    dPriv->pBackClipRects[0].y2 = y2;
	 }
      }
   }
}


void intelWindowMoved( intelContextPtr intel )
{
   if (!intel->ctx.DrawBuffer) {
      intelSetFrontClipRects( intel );
   }
   else {
      switch (intel->ctx.DrawBuffer->_ColorDrawBufferMask[0]) {
      case BUFFER_BIT_FRONT_LEFT:
	 intelSetFrontClipRects( intel );
	 break;
      case BUFFER_BIT_BACK_LEFT:
	 intelSetBackClipRects( intel );
	 break;
      default:
	 /* glDrawBuffer(GL_NONE or GL_FRONT_AND_BACK): software fallback */
	 intelSetFrontClipRects( intel );
      }
   }
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
      intelContextPtr intel = (intelContextPtr) driContextPriv->driverPrivate;

      if ( intel->driDrawable != driDrawPriv ) {
	 /* Shouldn't the readbuffer be stored also? */
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

void intelGetLock( intelContextPtr intel, GLuint flags )
{
   __DRIdrawablePrivate *dPriv = intel->driDrawable;
   __DRIscreenPrivate *sPriv = intel->driScreen;
   drmI830Sarea * sarea = intel->sarea;
   int me = intel->hHWContext;
   unsigned   i;

   drmGetLock(intel->driFd, intel->hHWContext, flags);

   /* If the window moved, may need to set a new cliprect now.
    *
    * NOTE: This releases and regains the hw lock, so all state
    * checking must be done *after* this call:
    */
   if (dPriv)
      DRI_VALIDATE_DRAWABLE_INFO(sPriv, dPriv);

   /* If we lost context, need to dump all registers to hardware.
    * Note that we don't care about 2d contexts, even if they perform
    * accelerated commands, so the DRI locking in the X server is even
    * more broken than usual.
    */

   if (sarea->ctxOwner != me) {
      intel->perf_boxes |= I830_BOX_LOST_CONTEXT;
      sarea->ctxOwner = me;
   }

   /* Shared texture managment - if another client has played with
    * texture space, figure out which if any of our textures have been
    * ejected, and update our global LRU.
    */

   for ( i = 0 ; i < intel->nr_heaps ; i++ ) {
      DRI_AGE_TEXTURES( intel->texture_heaps[ i ] );
   }

   if (dPriv && intel->lastStamp != dPriv->lastStamp) {
      intelWindowMoved( intel );
      intel->lastStamp = dPriv->lastStamp;
   }
}

void intelSwapBuffers( __DRIdrawablePrivate *dPriv )
{
   if (dPriv->driContextPriv && dPriv->driContextPriv->driverPrivate) {
      intelContextPtr intel;
      GLcontext *ctx;
      intel = (intelContextPtr) dPriv->driContextPriv->driverPrivate;
      ctx = &intel->ctx;
      if (ctx->Visual.doubleBufferMode) {
	 _mesa_notifySwapBuffers( ctx );  /* flush pending rendering comands */
	 if ( 0 /*intel->doPageFlip*/ ) { /* doPageFlip is never set !!! */
	    intelPageFlip( dPriv );
	 } else {
	    intelCopyBuffer( dPriv );
	 }
      }
   } else {
      /* XXX this shouldn't be an error but we can't handle it for now */
      fprintf(stderr, "%s: drawable has no context!\n", __FUNCTION__);
   }
}


void intelInitState( GLcontext *ctx )
{
   /* Mesa should do this for us:
    */
   ctx->Driver.AlphaFunc( ctx, 
			  ctx->Color.AlphaFunc,
			  ctx->Color.AlphaRef);

   ctx->Driver.BlendColor( ctx,
			   ctx->Color.BlendColor );

   ctx->Driver.BlendEquationSeparate( ctx, 
				      ctx->Color.BlendEquationRGB,
				      ctx->Color.BlendEquationA);

   ctx->Driver.BlendFuncSeparate( ctx,
				  ctx->Color.BlendSrcRGB,
				  ctx->Color.BlendDstRGB,
				  ctx->Color.BlendSrcA,
				  ctx->Color.BlendDstA);

   ctx->Driver.ColorMask( ctx, 
			  ctx->Color.ColorMask[RCOMP],
			  ctx->Color.ColorMask[GCOMP],
			  ctx->Color.ColorMask[BCOMP],
			  ctx->Color.ColorMask[ACOMP]);

   ctx->Driver.CullFace( ctx, ctx->Polygon.CullFaceMode );
   ctx->Driver.DepthFunc( ctx, ctx->Depth.Func );
   ctx->Driver.DepthMask( ctx, ctx->Depth.Mask );

   ctx->Driver.Enable( ctx, GL_ALPHA_TEST, ctx->Color.AlphaEnabled );
   ctx->Driver.Enable( ctx, GL_BLEND, ctx->Color.BlendEnabled );
   ctx->Driver.Enable( ctx, GL_COLOR_LOGIC_OP, ctx->Color.ColorLogicOpEnabled );
   ctx->Driver.Enable( ctx, GL_COLOR_SUM, ctx->Fog.ColorSumEnabled );
   ctx->Driver.Enable( ctx, GL_CULL_FACE, ctx->Polygon.CullFlag );
   ctx->Driver.Enable( ctx, GL_DEPTH_TEST, ctx->Depth.Test );
   ctx->Driver.Enable( ctx, GL_DITHER, ctx->Color.DitherFlag );
   ctx->Driver.Enable( ctx, GL_FOG, ctx->Fog.Enabled );
   ctx->Driver.Enable( ctx, GL_LIGHTING, ctx->Light.Enabled );
   ctx->Driver.Enable( ctx, GL_LINE_SMOOTH, ctx->Line.SmoothFlag );
   ctx->Driver.Enable( ctx, GL_POLYGON_STIPPLE, ctx->Polygon.StippleFlag );
   ctx->Driver.Enable( ctx, GL_SCISSOR_TEST, ctx->Scissor.Enabled );
   ctx->Driver.Enable( ctx, GL_STENCIL_TEST, ctx->Stencil.Enabled );
   ctx->Driver.Enable( ctx, GL_TEXTURE_1D, GL_FALSE );
   ctx->Driver.Enable( ctx, GL_TEXTURE_2D, GL_FALSE );
   ctx->Driver.Enable( ctx, GL_TEXTURE_RECTANGLE_NV, GL_FALSE );
   ctx->Driver.Enable( ctx, GL_TEXTURE_3D, GL_FALSE );
   ctx->Driver.Enable( ctx, GL_TEXTURE_CUBE_MAP, GL_FALSE );

   ctx->Driver.Fogfv( ctx, GL_FOG_COLOR, ctx->Fog.Color );
   ctx->Driver.Fogfv( ctx, GL_FOG_MODE, 0 );
   ctx->Driver.Fogfv( ctx, GL_FOG_DENSITY, &ctx->Fog.Density );
   ctx->Driver.Fogfv( ctx, GL_FOG_START, &ctx->Fog.Start );
   ctx->Driver.Fogfv( ctx, GL_FOG_END, &ctx->Fog.End );

   ctx->Driver.FrontFace( ctx, ctx->Polygon.FrontFace );

   {
      GLfloat f = (GLfloat)ctx->Light.Model.ColorControl;
      ctx->Driver.LightModelfv( ctx, GL_LIGHT_MODEL_COLOR_CONTROL, &f );
   }

   ctx->Driver.LineWidth( ctx, ctx->Line.Width );
   ctx->Driver.LogicOpcode( ctx, ctx->Color.LogicOp );
   ctx->Driver.PointSize( ctx, ctx->Point.Size );
   ctx->Driver.PolygonStipple( ctx, (const GLubyte *)ctx->PolygonStipple );
   ctx->Driver.Scissor( ctx, ctx->Scissor.X, ctx->Scissor.Y,
			ctx->Scissor.Width, ctx->Scissor.Height );
   ctx->Driver.ShadeModel( ctx, ctx->Light.ShadeModel );
   ctx->Driver.StencilFunc( ctx, 
			    ctx->Stencil.Function[0],
			    ctx->Stencil.Ref[0],
			    ctx->Stencil.ValueMask[0] );
   ctx->Driver.StencilMask( ctx, ctx->Stencil.WriteMask[0] );
   ctx->Driver.StencilOp( ctx, 
			  ctx->Stencil.FailFunc[0],
			  ctx->Stencil.ZFailFunc[0],
			  ctx->Stencil.ZPassFunc[0]);


   ctx->Driver.DrawBuffer( ctx, ctx->Color.DrawBuffer[0] );
}
