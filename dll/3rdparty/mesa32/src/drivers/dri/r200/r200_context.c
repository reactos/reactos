/* $XFree86: xc/lib/GL/mesa/src/drv/r200/r200_context.c,v 1.3 2003/05/06 23:52:08 daenzer Exp $ */
/*
Copyright (C) The Weather Channel, Inc.  2002.  All Rights Reserved.

The Weather Channel (TM) funded Tungsten Graphics to develop the
initial release of the Radeon 8500 driver under the XFree86 license.
This notice must be preserved.

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice (including the
next paragraph) shall be included in all copies or substantial
portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/

/*
 * Authors:
 *   Keith Whitwell <keith@tungstengraphics.com>
 */

#include "glheader.h"
#include "api_arrayelt.h"
#include "context.h"
#include "simple_list.h"
#include "imports.h"
#include "matrix.h"
#include "extensions.h"
#include "framebuffer.h"
#include "state.h"

#include "swrast/swrast.h"
#include "swrast_setup/swrast_setup.h"
#include "vbo/vbo.h"

#include "tnl/tnl.h"
#include "tnl/t_pipeline.h"

#include "drivers/common/driverfuncs.h"

#include "r200_context.h"
#include "r200_ioctl.h"
#include "r200_state.h"
#include "r200_span.h"
#include "r200_pixel.h"
#include "r200_tex.h"
#include "r200_swtcl.h"
#include "r200_tcl.h"
#include "r200_maos.h"
#include "r200_vertprog.h"

#define need_GL_ARB_multisample
#define need_GL_ARB_texture_compression
#define need_GL_ARB_vertex_buffer_object
#define need_GL_ARB_vertex_program
#define need_GL_ATI_fragment_shader
#define need_GL_EXT_blend_minmax
#define need_GL_EXT_fog_coord
#define need_GL_EXT_secondary_color
#define need_GL_EXT_blend_equation_separate
#define need_GL_EXT_blend_func_separate
#define need_GL_NV_vertex_program
#define need_GL_ARB_point_parameters
#include "extension_helper.h"

#define DRIVER_DATE	"20060602"

#include "vblank.h"
#include "utils.h"
#include "xmlpool.h" /* for symbolic values of enum-type options */
#ifndef R200_DEBUG
int R200_DEBUG = (0);
#endif

/* Return various strings for glGetString().
 */
static const GLubyte *r200GetString( GLcontext *ctx, GLenum name )
{
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
   static char buffer[128];
   unsigned   offset;
   GLuint agp_mode = (rmesa->r200Screen->card_type == RADEON_CARD_PCI)? 0 :
      rmesa->r200Screen->AGPMode;

   switch ( name ) {
   case GL_VENDOR:
      return (GLubyte *)"Tungsten Graphics, Inc.";

   case GL_RENDERER:
      offset = driGetRendererString( buffer, "R200", DRIVER_DATE,
				     agp_mode );

      sprintf( & buffer[ offset ], " %sTCL",
	       !(rmesa->TclFallback & R200_TCL_FALLBACK_TCL_DISABLE)
	       ? "" : "NO-" );

      return (GLubyte *)buffer;

   default:
      return NULL;
   }
}


/* Extension strings exported by the R200 driver.
 */
const struct dri_extension card_extensions[] =
{
    { "GL_ARB_multisample",                GL_ARB_multisample_functions },
    { "GL_ARB_multitexture",               NULL },
    { "GL_ARB_texture_border_clamp",       NULL },
    { "GL_ARB_texture_compression",        GL_ARB_texture_compression_functions },
    { "GL_ARB_texture_env_add",            NULL },
    { "GL_ARB_texture_env_combine",        NULL },
    { "GL_ARB_texture_env_dot3",           NULL },
    { "GL_ARB_texture_env_crossbar",       NULL },
    { "GL_ARB_texture_mirrored_repeat",    NULL },
    { "GL_ARB_vertex_buffer_object",       GL_ARB_vertex_buffer_object_functions },
    { "GL_EXT_blend_minmax",               GL_EXT_blend_minmax_functions },
    { "GL_EXT_blend_subtract",             NULL },
    { "GL_EXT_fog_coord",                  GL_EXT_fog_coord_functions },
    { "GL_EXT_secondary_color",            GL_EXT_secondary_color_functions },
    { "GL_EXT_stencil_wrap",               NULL },
    { "GL_EXT_texture_edge_clamp",         NULL },
    { "GL_EXT_texture_env_combine",        NULL },
    { "GL_EXT_texture_env_dot3",           NULL },
    { "GL_EXT_texture_filter_anisotropic", NULL },
    { "GL_EXT_texture_lod_bias",           NULL },
    { "GL_EXT_texture_mirror_clamp",       NULL },
    { "GL_EXT_texture_rectangle",          NULL },
    { "GL_ATI_texture_env_combine3",       NULL },
    { "GL_ATI_texture_mirror_once",        NULL },
    { "GL_MESA_pack_invert",               NULL },
    { "GL_NV_blend_square",                NULL },
    { "GL_SGIS_generate_mipmap",           NULL },
    { NULL,                                NULL }
};

const struct dri_extension blend_extensions[] = {
    { "GL_EXT_blend_equation_separate",    GL_EXT_blend_equation_separate_functions },
    { "GL_EXT_blend_func_separate",        GL_EXT_blend_func_separate_functions },
    { NULL,                                NULL }
};

const struct dri_extension ARB_vp_extension[] = {
    { "GL_ARB_vertex_program",             GL_ARB_vertex_program_functions }
};

const struct dri_extension NV_vp_extension[] = {
    { "GL_NV_vertex_program",              GL_NV_vertex_program_functions }
};

const struct dri_extension ATI_fs_extension[] = {
    { "GL_ATI_fragment_shader",            GL_ATI_fragment_shader_functions }
};

const struct dri_extension point_extensions[] = {
    { "GL_ARB_point_sprite",               NULL },
    { "GL_ARB_point_parameters",           GL_ARB_point_parameters_functions },
    { NULL,                                NULL }
};

extern const struct tnl_pipeline_stage _r200_render_stage;
extern const struct tnl_pipeline_stage _r200_tcl_stage;

static const struct tnl_pipeline_stage *r200_pipeline[] = {

   /* Try and go straight to t&l
    */
   &_r200_tcl_stage,  

   /* Catch any t&l fallbacks
    */
   &_tnl_vertex_transform_stage,
   &_tnl_normal_transform_stage,
   &_tnl_lighting_stage,
   &_tnl_fog_coordinate_stage,
   &_tnl_texgen_stage,
   &_tnl_texture_transform_stage,
   &_tnl_point_attenuation_stage,
   &_tnl_vertex_program_stage,
   /* Try again to go to tcl? 
    *     - no good for asymmetric-twoside (do with multipass)
    *     - no good for asymmetric-unfilled (do with multipass)
    *     - good for material
    *     - good for texgen
    *     - need to manipulate a bit of state
    *
    * - worth it/not worth it?
    */
			
   /* Else do them here.
    */
/*    &_r200_render_stage,  */ /* FIXME: bugs with ut2003 */
   &_tnl_render_stage,		/* FALLBACK:  */
   NULL,
};



/* Initialize the driver's misc functions.
 */
static void r200InitDriverFuncs( struct dd_function_table *functions )
{
    functions->GetBufferSize		= NULL; /* OBSOLETE */
    functions->GetString		= r200GetString;
}

static const struct dri_debug_control debug_control[] =
{
    { "fall",  DEBUG_FALLBACKS },
    { "tex",   DEBUG_TEXTURE },
    { "ioctl", DEBUG_IOCTL },
    { "prim",  DEBUG_PRIMS },
    { "vert",  DEBUG_VERTS },
    { "state", DEBUG_STATE },
    { "code",  DEBUG_CODEGEN },
    { "vfmt",  DEBUG_VFMT },
    { "vtxf",  DEBUG_VFMT },
    { "verb",  DEBUG_VERBOSE },
    { "dri",   DEBUG_DRI },
    { "dma",   DEBUG_DMA },
    { "san",   DEBUG_SANITY },
    { "sync",  DEBUG_SYNC },
    { "pix",   DEBUG_PIXEL },
    { "mem",   DEBUG_MEMORY },
    { NULL,    0 }
};


/* Create the device specific rendering context.
 */
GLboolean r200CreateContext( const __GLcontextModes *glVisual,
			     __DRIcontextPrivate *driContextPriv,
			     void *sharedContextPrivate)
{
   __DRIscreenPrivate *sPriv = driContextPriv->driScreenPriv;
   radeonScreenPtr screen = (radeonScreenPtr)(sPriv->private);
   struct dd_function_table functions;
   r200ContextPtr rmesa;
   GLcontext *ctx, *shareCtx;
   int i;
   int tcl_mode, fthrottle_mode;

   assert(glVisual);
   assert(driContextPriv);
   assert(screen);

   /* Allocate the R200 context */
   rmesa = (r200ContextPtr) CALLOC( sizeof(*rmesa) );
   if ( !rmesa )
      return GL_FALSE;
      
   /* init exp fog table data */
   r200InitStaticFogData();

   /* Parse configuration files.
    * Do this here so that initialMaxAnisotropy is set before we create
    * the default textures.
    */
   driParseConfigFiles (&rmesa->optionCache, &screen->optionCache,
			screen->driScreen->myNum, "r200");
   rmesa->initialMaxAnisotropy = driQueryOptionf(&rmesa->optionCache,
                                                 "def_max_anisotropy");

   if ( driQueryOptionb( &rmesa->optionCache, "hyperz" ) ) {
      if ( sPriv->drmMinor < 13 )
	 fprintf( stderr, "DRM version 1.%d too old to support HyperZ, "
			  "disabling.\n",sPriv->drmMinor );
      else
	 rmesa->using_hyperz = GL_TRUE;
   }
 
   if ( sPriv->drmMinor >= 15 )
      rmesa->texmicrotile = GL_TRUE;

   /* Init default driver functions then plug in our R200-specific functions
    * (the texture functions are especially important)
    */
   _mesa_init_driver_functions(&functions);
   r200InitDriverFuncs(&functions);
   r200InitIoctlFuncs(&functions);
   r200InitStateFuncs(&functions);
   r200InitTextureFuncs(&functions);
   r200InitShaderFuncs(&functions); 

   /* Allocate and initialize the Mesa context */
   if (sharedContextPrivate)
      shareCtx = ((r200ContextPtr) sharedContextPrivate)->glCtx;
   else
      shareCtx = NULL;
   rmesa->glCtx = _mesa_create_context(glVisual, shareCtx,
                                       &functions, (void *) rmesa);
   if (!rmesa->glCtx) {
      FREE(rmesa);
      return GL_FALSE;
   }
   driContextPriv->driverPrivate = rmesa;

   /* Init r200 context data */
   rmesa->dri.context = driContextPriv;
   rmesa->dri.screen = sPriv;
   rmesa->dri.drawable = NULL; /* Set by XMesaMakeCurrent */
   rmesa->dri.hwContext = driContextPriv->hHWContext;
   rmesa->dri.hwLock = &sPriv->pSAREA->lock;
   rmesa->dri.fd = sPriv->fd;
   rmesa->dri.drmMinor = sPriv->drmMinor;

   rmesa->r200Screen = screen;
   rmesa->sarea = (drm_radeon_sarea_t *)((GLubyte *)sPriv->pSAREA +
				       screen->sarea_priv_offset);


   rmesa->dma.buf0_address = rmesa->r200Screen->buffers->list[0].address;

   (void) memset( rmesa->texture_heaps, 0, sizeof( rmesa->texture_heaps ) );
   make_empty_list( & rmesa->swapped );

   rmesa->nr_heaps = 1 /* screen->numTexHeaps */ ;
   assert(rmesa->nr_heaps < RADEON_NR_TEX_HEAPS);
   for ( i = 0 ; i < rmesa->nr_heaps ; i++ ) {
      rmesa->texture_heaps[i] = driCreateTextureHeap( i, rmesa,
	    screen->texSize[i],
	    12,
	    RADEON_NR_TEX_REGIONS,
	    (drmTextureRegionPtr)rmesa->sarea->tex_list[i],
	    & rmesa->sarea->tex_age[i],
	    & rmesa->swapped,
	    sizeof( r200TexObj ),
	    (destroy_texture_object_t *) r200DestroyTexObj );
   }
   rmesa->texture_depth = driQueryOptioni (&rmesa->optionCache,
					   "texture_depth");
   if (rmesa->texture_depth == DRI_CONF_TEXTURE_DEPTH_FB)
      rmesa->texture_depth = ( screen->cpp == 4 ) ?
	 DRI_CONF_TEXTURE_DEPTH_32 : DRI_CONF_TEXTURE_DEPTH_16;

   rmesa->swtcl.RenderIndex = ~0;
   rmesa->hw.all_dirty = 1;

   /* Set the maximum texture size small enough that we can guarentee that
    * all texture units can bind a maximal texture and have all of them in
    * texturable memory at once. Depending on the allow_large_textures driconf
    * setting allow larger textures.
    */

   ctx = rmesa->glCtx;
   ctx->Const.MaxTextureUnits = driQueryOptioni (&rmesa->optionCache,
						 "texture_units");
   ctx->Const.MaxTextureImageUnits = ctx->Const.MaxTextureUnits;
   ctx->Const.MaxTextureCoordUnits = ctx->Const.MaxTextureUnits;

   i = driQueryOptioni( &rmesa->optionCache, "allow_large_textures");

   driCalculateMaxTextureLevels( rmesa->texture_heaps,
				 rmesa->nr_heaps,
				 & ctx->Const,
				 4,
				 11, /* max 2D texture size is 2048x2048 */
#if ENABLE_HW_3D_TEXTURE
				 8,  /* max 3D texture size is 256^3 */
#else
				 0,  /* 3D textures unsupported */
#endif
				 11, /* max cube texture size is 2048x2048 */
				 11, /* max texture rectangle size is 2048x2048 */
				 12,
				 GL_FALSE,
				 i );

   ctx->Const.MaxTextureMaxAnisotropy = 16.0;

   /* No wide AA points.
    */
   ctx->Const.MinPointSize = 1.0;
   ctx->Const.MinPointSizeAA = 1.0;
   ctx->Const.MaxPointSizeAA = 1.0;
   ctx->Const.PointSizeGranularity = 0.0625;
   if (rmesa->r200Screen->drmSupportsPointSprites)
      ctx->Const.MaxPointSize = 2047.0;
   else
      ctx->Const.MaxPointSize = 1.0;

   /* mesa initialization problem - _mesa_init_point was already called */
   ctx->Point.MaxSize = ctx->Const.MaxPointSize;

   ctx->Const.MinLineWidth = 1.0;
   ctx->Const.MinLineWidthAA = 1.0;
   ctx->Const.MaxLineWidth = 10.0;
   ctx->Const.MaxLineWidthAA = 10.0;
   ctx->Const.LineWidthGranularity = 0.0625;

   ctx->Const.VertexProgram.MaxNativeInstructions = R200_VSF_MAX_INST;
   ctx->Const.VertexProgram.MaxNativeAttribs = 12;
   ctx->Const.VertexProgram.MaxNativeTemps = R200_VSF_MAX_TEMPS;
   ctx->Const.VertexProgram.MaxNativeParameters = R200_VSF_MAX_PARAM;
   ctx->Const.VertexProgram.MaxNativeAddressRegs = 1;

   /* Initialize the software rasterizer and helper modules.
    */
   _swrast_CreateContext( ctx );
   _vbo_CreateContext( ctx );
   _tnl_CreateContext( ctx );
   _swsetup_CreateContext( ctx );
   _ae_create_context( ctx );

   /* Install the customized pipeline:
    */
   _tnl_destroy_pipeline( ctx );
   _tnl_install_pipeline( ctx, r200_pipeline );

   /* Try and keep materials and vertices separate:
    */
/*    _tnl_isolate_materials( ctx, GL_TRUE ); */


   /* Configure swrast and TNL to match hardware characteristics:
    */
   _swrast_allow_pixel_fog( ctx, GL_FALSE );
   _swrast_allow_vertex_fog( ctx, GL_TRUE );
   _tnl_allow_pixel_fog( ctx, GL_FALSE );
   _tnl_allow_vertex_fog( ctx, GL_TRUE );


   for ( i = 0 ; i < R200_MAX_TEXTURE_UNITS ; i++ ) {
      _math_matrix_ctr( &rmesa->TexGenMatrix[i] );
      _math_matrix_set_identity( &rmesa->TexGenMatrix[i] );
   }
   _math_matrix_ctr( &rmesa->tmpmat );
   _math_matrix_set_identity( &rmesa->tmpmat );

   driInitExtensions( ctx, card_extensions, GL_TRUE );
   if (!(rmesa->r200Screen->chip_flags & R200_CHIPSET_YCBCR_BROKEN)) {
     /* yuv textures don't work with some chips - R200 / rv280 okay so far
	others get the bit ordering right but don't actually do YUV-RGB conversion */
      _mesa_enable_extension( ctx, "GL_MESA_ycbcr_texture" );
   }
   if (rmesa->glCtx->Mesa_DXTn) {
      _mesa_enable_extension( ctx, "GL_EXT_texture_compression_s3tc" );
      _mesa_enable_extension( ctx, "GL_S3_s3tc" );
   }
   else if (driQueryOptionb (&rmesa->optionCache, "force_s3tc_enable")) {
      _mesa_enable_extension( ctx, "GL_EXT_texture_compression_s3tc" );
   }

   if (rmesa->r200Screen->drmSupportsCubeMapsR200)
      _mesa_enable_extension( ctx, "GL_ARB_texture_cube_map" );
   if (rmesa->r200Screen->drmSupportsBlendColor) {
       driInitExtensions( ctx, blend_extensions, GL_FALSE );
   }
   if(rmesa->r200Screen->drmSupportsVertexProgram)
      driInitSingleExtension( ctx, ARB_vp_extension );
   if(driQueryOptionb(&rmesa->optionCache, "nv_vertex_program"))
      driInitSingleExtension( ctx, NV_vp_extension );

   if ((ctx->Const.MaxTextureUnits == 6) && rmesa->r200Screen->drmSupportsFragShader)
      driInitSingleExtension( ctx, ATI_fs_extension );
   if (rmesa->r200Screen->drmSupportsPointSprites)
      driInitExtensions( ctx, point_extensions, GL_FALSE );
#if 0
   r200InitDriverFuncs( ctx );
   r200InitIoctlFuncs( ctx );
   r200InitStateFuncs( ctx );
   r200InitTextureFuncs( ctx );
#endif
   /* plug in a few more device driver functions */
   /* XXX these should really go right after _mesa_init_driver_functions() */
   r200InitPixelFuncs( ctx );
   r200InitSpanFuncs( ctx );
   r200InitTnlFuncs( ctx );
   r200InitState( rmesa );
   r200InitSwtcl( ctx );

   fthrottle_mode = driQueryOptioni(&rmesa->optionCache, "fthrottle_mode");
   rmesa->iw.irq_seq = -1;
   rmesa->irqsEmitted = 0;
   rmesa->do_irqs = (fthrottle_mode == DRI_CONF_FTHROTTLE_IRQS &&
		     rmesa->r200Screen->irq);

   rmesa->do_usleeps = (fthrottle_mode == DRI_CONF_FTHROTTLE_USLEEPS);

   if (!rmesa->do_irqs)
      fprintf(stderr,
	      "IRQ's not enabled, falling back to %s: %d %d\n",
	      rmesa->do_usleeps ? "usleeps" : "busy waits",
	      fthrottle_mode,
	      rmesa->r200Screen->irq);

   rmesa->vblank_flags = (rmesa->r200Screen->irq != 0)
       ? driGetDefaultVBlankFlags(&rmesa->optionCache) : VBLANK_FLAG_NO_IRQ;

   rmesa->prefer_gart_client_texturing = 
      (getenv("R200_GART_CLIENT_TEXTURES") != 0);

   (*dri_interface->getUST)( & rmesa->swap_ust );


#if DO_DEBUG
   R200_DEBUG  = driParseDebugString( getenv( "R200_DEBUG" ),
				      debug_control );
   R200_DEBUG |= driParseDebugString( getenv( "RADEON_DEBUG" ),
				      debug_control );
#endif

   tcl_mode = driQueryOptioni(&rmesa->optionCache, "tcl_mode");
   if (driQueryOptionb(&rmesa->optionCache, "no_rast")) {
      fprintf(stderr, "disabling 3D acceleration\n");
      FALLBACK(rmesa, R200_FALLBACK_DISABLE, 1);
   }
   else if (tcl_mode == DRI_CONF_TCL_SW || getenv("R200_NO_TCL") ||
	    !(rmesa->r200Screen->chip_flags & RADEON_CHIPSET_TCL)) {
      if (rmesa->r200Screen->chip_flags & RADEON_CHIPSET_TCL) {
	 rmesa->r200Screen->chip_flags &= ~RADEON_CHIPSET_TCL;
	 fprintf(stderr, "Disabling HW TCL support\n");
      }
      TCL_FALLBACK(rmesa->glCtx, R200_TCL_FALLBACK_TCL_DISABLE, 1);
   }

   return GL_TRUE;
}


/* Destroy the device specific context.
 */
/* Destroy the Mesa and driver specific context data.
 */
void r200DestroyContext( __DRIcontextPrivate *driContextPriv )
{
   GET_CURRENT_CONTEXT(ctx);
   r200ContextPtr rmesa = (r200ContextPtr) driContextPriv->driverPrivate;
   r200ContextPtr current = ctx ? R200_CONTEXT(ctx) : NULL;

   /* check if we're deleting the currently bound context */
   if (rmesa == current) {
      R200_FIREVERTICES( rmesa );
      _mesa_make_current(NULL, NULL, NULL);
   }

   /* Free r200 context resources */
   assert(rmesa); /* should never be null */
   if ( rmesa ) {
      GLboolean   release_texture_heaps;


      release_texture_heaps = (rmesa->glCtx->Shared->RefCount == 1);
      _swsetup_DestroyContext( rmesa->glCtx );
      _tnl_DestroyContext( rmesa->glCtx );
      _vbo_DestroyContext( rmesa->glCtx );
      _swrast_DestroyContext( rmesa->glCtx );

      r200DestroySwtcl( rmesa->glCtx );
      r200ReleaseArrays( rmesa->glCtx, ~0 );

      if (rmesa->dma.current.buf) {
	 r200ReleaseDmaRegion( rmesa, &rmesa->dma.current, __FUNCTION__ );
	 r200FlushCmdBuf( rmesa, __FUNCTION__ );
      }

      if (rmesa->state.scissor.pClipRects) {
	 FREE(rmesa->state.scissor.pClipRects);
	 rmesa->state.scissor.pClipRects = NULL;
      }

      if ( release_texture_heaps ) {
         /* This share group is about to go away, free our private
          * texture object data.
          */
         int i;

         for ( i = 0 ; i < rmesa->nr_heaps ; i++ ) {
	    driDestroyTextureHeap( rmesa->texture_heaps[ i ] );
	    rmesa->texture_heaps[ i ] = NULL;
         }

	 assert( is_empty_list( & rmesa->swapped ) );
      }

      /* free the Mesa context */
      rmesa->glCtx->DriverCtx = NULL;
      _mesa_destroy_context( rmesa->glCtx );

      /* free the option cache */
      driDestroyOptionCache (&rmesa->optionCache);

      FREE( rmesa );
   }
}




void
r200SwapBuffers( __DRIdrawablePrivate *dPriv )
{
   if (dPriv->driContextPriv && dPriv->driContextPriv->driverPrivate) {
      r200ContextPtr rmesa;
      GLcontext *ctx;
      rmesa = (r200ContextPtr) dPriv->driContextPriv->driverPrivate;
      ctx = rmesa->glCtx;
      if (ctx->Visual.doubleBufferMode) {
         _mesa_notifySwapBuffers( ctx );  /* flush pending rendering comands */
         if ( rmesa->doPageFlip ) {
            r200PageFlip( dPriv );
         }
         else {
	     r200CopyBuffer( dPriv, NULL );
         }
      }
   }
   else {
      /* XXX this shouldn't be an error but we can't handle it for now */
      _mesa_problem(NULL, "%s: drawable has no context!", __FUNCTION__);
   }
}

void
r200CopySubBuffer( __DRIdrawablePrivate *dPriv,
		   int x, int y, int w, int h )
{
   if (dPriv->driContextPriv && dPriv->driContextPriv->driverPrivate) {
      r200ContextPtr rmesa;
      GLcontext *ctx;
      rmesa = (r200ContextPtr) dPriv->driContextPriv->driverPrivate;
      ctx = rmesa->glCtx;
      if (ctx->Visual.doubleBufferMode) {
	 drm_clip_rect_t rect;
	 rect.x1 = x + dPriv->x;
	 rect.y1 = (dPriv->h - y - h) + dPriv->y;
	 rect.x2 = rect.x1 + w;
	 rect.y2 = rect.y1 + h;
         _mesa_notifySwapBuffers( ctx );  /* flush pending rendering comands */
	 r200CopyBuffer( dPriv, &rect );
      }
   }
   else {
      /* XXX this shouldn't be an error but we can't handle it for now */
      _mesa_problem(NULL, "%s: drawable has no context!", __FUNCTION__);
   }
}

/* Force the context `c' to be the current context and associate with it
 * buffer `b'.
 */
GLboolean
r200MakeCurrent( __DRIcontextPrivate *driContextPriv,
                   __DRIdrawablePrivate *driDrawPriv,
                   __DRIdrawablePrivate *driReadPriv )
{
   if ( driContextPriv ) {
      r200ContextPtr newCtx = 
	 (r200ContextPtr) driContextPriv->driverPrivate;

      if (R200_DEBUG & DEBUG_DRI)
	 fprintf(stderr, "%s ctx %p\n", __FUNCTION__, (void *)newCtx->glCtx);

      if ( newCtx->dri.drawable != driDrawPriv ) {
	 driDrawableInitVBlank( driDrawPriv, newCtx->vblank_flags,
				&newCtx->vbl_seq );
      }

      newCtx->dri.readable = driReadPriv;

      if ( newCtx->dri.drawable != driDrawPriv ||
           newCtx->lastStamp != driDrawPriv->lastStamp ) {
	 newCtx->dri.drawable = driDrawPriv;

	 r200SetCliprects(newCtx);
	 r200UpdateViewportOffset( newCtx->glCtx );
      }

      _mesa_make_current( newCtx->glCtx,
			  (GLframebuffer *) driDrawPriv->driverPrivate,
			  (GLframebuffer *) driReadPriv->driverPrivate );

      _mesa_update_state( newCtx->glCtx );
      r200ValidateState( newCtx->glCtx );

   } else {
      if (R200_DEBUG & DEBUG_DRI)
	 fprintf(stderr, "%s ctx is null\n", __FUNCTION__);
      _mesa_make_current( NULL, NULL, NULL );
   }

   if (R200_DEBUG & DEBUG_DRI)
      fprintf(stderr, "End %s\n", __FUNCTION__);
   return GL_TRUE;
}

/* Force the context `c' to be unbound from its buffer.
 */
GLboolean
r200UnbindContext( __DRIcontextPrivate *driContextPriv )
{
   r200ContextPtr rmesa = (r200ContextPtr) driContextPriv->driverPrivate;

   if (R200_DEBUG & DEBUG_DRI)
      fprintf(stderr, "%s ctx %p\n", __FUNCTION__, (void *)rmesa->glCtx);

   return GL_TRUE;
}
