/* $XFree86: xc/lib/GL/mesa/src/drv/radeon/radeon_context.c,v 1.9 2003/09/24 02:43:12 dawes Exp $ */
/**************************************************************************

Copyright 2000, 2001 ATI Technologies Inc., Ontario, Canada, and
                     VA Linux Systems Inc., Fremont, California.

All Rights Reserved.

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
 *   Kevin E. Martin <martin@valinux.com>
 *   Gareth Hughes <gareth@valinux.com>
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

#include "swrast/swrast.h"
#include "swrast_setup/swrast_setup.h"
#include "vbo/vbo.h"

#include "tnl/tnl.h"
#include "tnl/t_pipeline.h"

#include "drivers/common/driverfuncs.h"

#include "radeon_context.h"
#include "radeon_ioctl.h"
#include "radeon_state.h"
#include "radeon_span.h"
#include "radeon_tex.h"
#include "radeon_swtcl.h"
#include "radeon_tcl.h"
#include "radeon_maos.h"

#define need_GL_ARB_multisample
#define need_GL_ARB_texture_compression
#define need_GL_ARB_vertex_buffer_object
#define need_GL_EXT_blend_minmax
#define need_GL_EXT_fog_coord
#define need_GL_EXT_secondary_color
#include "extension_helper.h"

#define DRIVER_DATE	"20061018"

#include "vblank.h"
#include "utils.h"
#include "xmlpool.h" /* for symbolic values of enum-type options */
#ifndef RADEON_DEBUG
int RADEON_DEBUG = (0);
#endif


/* Return various strings for glGetString().
 */
static const GLubyte *radeonGetString( GLcontext *ctx, GLenum name )
{
   radeonContextPtr rmesa = RADEON_CONTEXT(ctx);
   static char buffer[128];
   unsigned   offset;
   GLuint agp_mode = (rmesa->radeonScreen->card_type==RADEON_CARD_PCI) ? 0 :
      rmesa->radeonScreen->AGPMode;

   switch ( name ) {
   case GL_VENDOR:
      return (GLubyte *)"Tungsten Graphics, Inc.";

   case GL_RENDERER:
      offset = driGetRendererString( buffer, "Radeon", DRIVER_DATE,
				     agp_mode );

      sprintf( & buffer[ offset ], " %sTCL",
	       !(rmesa->TclFallback & RADEON_TCL_FALLBACK_TCL_DISABLE)
	       ? "" : "NO-" );

      return (GLubyte *)buffer;

   default:
      return NULL;
   }
}


/* Extension strings exported by the R100 driver.
 */
const struct dri_extension card_extensions[] =
{
    { "GL_ARB_multisample",                GL_ARB_multisample_functions },
    { "GL_ARB_multitexture",               NULL },
    { "GL_ARB_texture_border_clamp",       NULL },
    { "GL_ARB_texture_compression",        GL_ARB_texture_compression_functions },
    { "GL_ARB_texture_env_add",            NULL },
    { "GL_ARB_texture_env_combine",        NULL },
    { "GL_ARB_texture_env_crossbar",       NULL },
    { "GL_ARB_texture_env_dot3",           NULL },
    { "GL_ARB_texture_mirrored_repeat",    NULL },
    { "GL_ARB_vertex_buffer_object",       GL_ARB_vertex_buffer_object_functions },
    { "GL_EXT_blend_logic_op",             NULL },
    { "GL_EXT_blend_subtract",             GL_EXT_blend_minmax_functions },
    { "GL_EXT_fog_coord",                  GL_EXT_fog_coord_functions },
    { "GL_EXT_secondary_color",            GL_EXT_secondary_color_functions },
    { "GL_EXT_stencil_wrap",               NULL },
    { "GL_EXT_texture_edge_clamp",         NULL },
    { "GL_EXT_texture_env_combine",        NULL },
    { "GL_EXT_texture_env_dot3",           NULL },
    { "GL_EXT_texture_filter_anisotropic", NULL },
    { "GL_EXT_texture_lod_bias",           NULL },
    { "GL_EXT_texture_mirror_clamp",       NULL },
    { "GL_ATI_texture_env_combine3",       NULL },
    { "GL_ATI_texture_mirror_once",        NULL },
    { "GL_MESA_ycbcr_texture",             NULL },
    { "GL_NV_blend_square",                NULL },
    { "GL_SGIS_generate_mipmap",           NULL },
    { NULL,                                NULL }
};

extern const struct tnl_pipeline_stage _radeon_render_stage;
extern const struct tnl_pipeline_stage _radeon_tcl_stage;

static const struct tnl_pipeline_stage *radeon_pipeline[] = {

   /* Try and go straight to t&l
    */
   &_radeon_tcl_stage,  

   /* Catch any t&l fallbacks
    */
   &_tnl_vertex_transform_stage,
   &_tnl_normal_transform_stage,
   &_tnl_lighting_stage,
   &_tnl_fog_coordinate_stage,
   &_tnl_texgen_stage,
   &_tnl_texture_transform_stage,

   &_radeon_render_stage,
   &_tnl_render_stage,		/* FALLBACK:  */
   NULL,
};



/* Initialize the driver's misc functions.
 */
static void radeonInitDriverFuncs( struct dd_function_table *functions )
{
    functions->GetString	= radeonGetString;
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
    { NULL,    0 }
};


/* Create the device specific context.
 */
GLboolean
radeonCreateContext( const __GLcontextModes *glVisual,
                     __DRIcontextPrivate *driContextPriv,
                     void *sharedContextPrivate)
{
   __DRIscreenPrivate *sPriv = driContextPriv->driScreenPriv;
   radeonScreenPtr screen = (radeonScreenPtr)(sPriv->private);
   struct dd_function_table functions;
   radeonContextPtr rmesa;
   GLcontext *ctx, *shareCtx;
   int i;
   int tcl_mode, fthrottle_mode;

   assert(glVisual);
   assert(driContextPriv);
   assert(screen);

   /* Allocate the Radeon context */
   rmesa = (radeonContextPtr) CALLOC( sizeof(*rmesa) );
   if ( !rmesa )
      return GL_FALSE;

   /* init exp fog table data */
   radeonInitStaticFogData();
   
   /* Parse configuration files.
    * Do this here so that initialMaxAnisotropy is set before we create
    * the default textures.
    */
   driParseConfigFiles (&rmesa->optionCache, &screen->optionCache,
			screen->driScreen->myNum, "radeon");
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

   /* Init default driver functions then plug in our Radeon-specific functions
    * (the texture functions are especially important)
    */
   _mesa_init_driver_functions( &functions );
   radeonInitDriverFuncs( &functions );
   radeonInitTextureFuncs( &functions );

   /* Allocate the Mesa context */
   if (sharedContextPrivate)
      shareCtx = ((radeonContextPtr) sharedContextPrivate)->glCtx;
   else
      shareCtx = NULL;
   rmesa->glCtx = _mesa_create_context(glVisual, shareCtx,
                                       &functions, (void *) rmesa);
   if (!rmesa->glCtx) {
      FREE(rmesa);
      return GL_FALSE;
   }
   driContextPriv->driverPrivate = rmesa;

   /* Init radeon context data */
   rmesa->dri.context = driContextPriv;
   rmesa->dri.screen = sPriv;
   rmesa->dri.drawable = NULL;
   rmesa->dri.readable = NULL;
   rmesa->dri.hwContext = driContextPriv->hHWContext;
   rmesa->dri.hwLock = &sPriv->pSAREA->lock;
   rmesa->dri.fd = sPriv->fd;
   rmesa->dri.drmMinor = sPriv->drmMinor;

   rmesa->radeonScreen = screen;
   rmesa->sarea = (drm_radeon_sarea_t *)((GLubyte *)sPriv->pSAREA +
				       screen->sarea_priv_offset);


   rmesa->dma.buf0_address = rmesa->radeonScreen->buffers->list[0].address;

   (void) memset( rmesa->texture_heaps, 0, sizeof( rmesa->texture_heaps ) );
   make_empty_list( & rmesa->swapped );

   rmesa->nr_heaps = screen->numTexHeaps;
   for ( i = 0 ; i < rmesa->nr_heaps ; i++ ) {
      rmesa->texture_heaps[i] = driCreateTextureHeap( i, rmesa,
	    screen->texSize[i],
	    12,
	    RADEON_NR_TEX_REGIONS,
	    (drmTextureRegionPtr)rmesa->sarea->tex_list[i],
	    & rmesa->sarea->tex_age[i],
	    & rmesa->swapped,
	    sizeof( radeonTexObj ),
	    (destroy_texture_object_t *) radeonDestroyTexObj );

      driSetTextureSwapCounterLocation( rmesa->texture_heaps[i],
					& rmesa->c_textureSwaps );
   }
   rmesa->texture_depth = driQueryOptioni (&rmesa->optionCache,
					   "texture_depth");
   if (rmesa->texture_depth == DRI_CONF_TEXTURE_DEPTH_FB)
      rmesa->texture_depth = ( screen->cpp == 4 ) ?
	 DRI_CONF_TEXTURE_DEPTH_32 : DRI_CONF_TEXTURE_DEPTH_16;

   rmesa->swtcl.RenderIndex = ~0;
   rmesa->hw.all_dirty = GL_TRUE;

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
				 8,  /* 256^3 */
				 9,  /* \todo: max cube texture size seems to be 512x512(x6) */
				 11, /* max rect texture size is 2048x2048. */
				 12,
				 GL_FALSE,
				 i );


   ctx->Const.MaxTextureMaxAnisotropy = 16.0;

   /* No wide points.
    */
   ctx->Const.MinPointSize = 1.0;
   ctx->Const.MinPointSizeAA = 1.0;
   ctx->Const.MaxPointSize = 1.0;
   ctx->Const.MaxPointSizeAA = 1.0;

   ctx->Const.MinLineWidth = 1.0;
   ctx->Const.MinLineWidthAA = 1.0;
   ctx->Const.MaxLineWidth = 10.0;
   ctx->Const.MaxLineWidthAA = 10.0;
   ctx->Const.LineWidthGranularity = 0.0625;

   /* Set maxlocksize (and hence vb size) small enough to avoid
    * fallbacks in radeon_tcl.c.  ie. guarentee that all vertices can
    * fit in a single dma buffer for indexed rendering of quad strips,
    * etc.
    */
   ctx->Const.MaxArrayLockSize = 
      MIN2( ctx->Const.MaxArrayLockSize, 
 	    RADEON_BUFFER_SIZE / RADEON_MAX_TCL_VERTSIZE ); 

   rmesa->boxes = 0;

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
   _tnl_install_pipeline( ctx, radeon_pipeline );

   /* Try and keep materials and vertices separate:
    */
/*    _tnl_isolate_materials( ctx, GL_TRUE ); */

   /* Configure swrast and T&L to match hardware characteristics:
    */
   _swrast_allow_pixel_fog( ctx, GL_FALSE );
   _swrast_allow_vertex_fog( ctx, GL_TRUE );
   _tnl_allow_pixel_fog( ctx, GL_FALSE );
   _tnl_allow_vertex_fog( ctx, GL_TRUE );


   for ( i = 0 ; i < RADEON_MAX_TEXTURE_UNITS ; i++ ) {
      _math_matrix_ctr( &rmesa->TexGenMatrix[i] );
      _math_matrix_ctr( &rmesa->tmpmat[i] );
      _math_matrix_set_identity( &rmesa->TexGenMatrix[i] );
      _math_matrix_set_identity( &rmesa->tmpmat[i] );
   }

   driInitExtensions( ctx, card_extensions, GL_TRUE );
   if (rmesa->radeonScreen->drmSupportsCubeMapsR100)
      _mesa_enable_extension( ctx, "GL_ARB_texture_cube_map" );
   if (rmesa->glCtx->Mesa_DXTn) {
      _mesa_enable_extension( ctx, "GL_EXT_texture_compression_s3tc" );
      _mesa_enable_extension( ctx, "GL_S3_s3tc" );
   }
   else if (driQueryOptionb (&rmesa->optionCache, "force_s3tc_enable")) {
      _mesa_enable_extension( ctx, "GL_EXT_texture_compression_s3tc" );
   }

   if (rmesa->dri.drmMinor >= 9)
      _mesa_enable_extension( ctx, "GL_NV_texture_rectangle");

   /* XXX these should really go right after _mesa_init_driver_functions() */
   radeonInitIoctlFuncs( ctx );
   radeonInitStateFuncs( ctx );
   radeonInitSpanFuncs( ctx );
   radeonInitState( rmesa );
   radeonInitSwtcl( ctx );

   _mesa_vector4f_alloc( &rmesa->tcl.ObjClean, 0, 
			 ctx->Const.MaxArrayLockSize, 32 );

   fthrottle_mode = driQueryOptioni(&rmesa->optionCache, "fthrottle_mode");
   rmesa->iw.irq_seq = -1;
   rmesa->irqsEmitted = 0;
   rmesa->do_irqs = (rmesa->radeonScreen->irq != 0 &&
		     fthrottle_mode == DRI_CONF_FTHROTTLE_IRQS);

   rmesa->do_usleeps = (fthrottle_mode == DRI_CONF_FTHROTTLE_USLEEPS);

   rmesa->vblank_flags = (rmesa->radeonScreen->irq != 0)
       ? driGetDefaultVBlankFlags(&rmesa->optionCache) : VBLANK_FLAG_NO_IRQ;

   (*dri_interface->getUST)( & rmesa->swap_ust );


#if DO_DEBUG
   RADEON_DEBUG = driParseDebugString( getenv( "RADEON_DEBUG" ),
				       debug_control );
#endif

   tcl_mode = driQueryOptioni(&rmesa->optionCache, "tcl_mode");
   if (driQueryOptionb(&rmesa->optionCache, "no_rast")) {
      fprintf(stderr, "disabling 3D acceleration\n");
      FALLBACK(rmesa, RADEON_FALLBACK_DISABLE, 1);
   } else if (tcl_mode == DRI_CONF_TCL_SW ||
	      !(rmesa->radeonScreen->chip_flags & RADEON_CHIPSET_TCL)) {
      if (rmesa->radeonScreen->chip_flags & RADEON_CHIPSET_TCL) {
	 rmesa->radeonScreen->chip_flags &= ~RADEON_CHIPSET_TCL;
	 fprintf(stderr, "Disabling HW TCL support\n");
      }
      TCL_FALLBACK(rmesa->glCtx, RADEON_TCL_FALLBACK_TCL_DISABLE, 1);
   }

   if (rmesa->radeonScreen->chip_flags & RADEON_CHIPSET_TCL) {
/*       _tnl_need_dlist_norm_lengths( ctx, GL_FALSE ); */
   }
   return GL_TRUE;
}


/* Destroy the device specific context.
 */
/* Destroy the Mesa and driver specific context data.
 */
void radeonDestroyContext( __DRIcontextPrivate *driContextPriv )
{
   GET_CURRENT_CONTEXT(ctx);
   radeonContextPtr rmesa = (radeonContextPtr) driContextPriv->driverPrivate;
   radeonContextPtr current = ctx ? RADEON_CONTEXT(ctx) : NULL;

   /* check if we're deleting the currently bound context */
   if (rmesa == current) {
      RADEON_FIREVERTICES( rmesa );
      _mesa_make_current(NULL, NULL, NULL);
   }

   /* Free radeon context resources */
   assert(rmesa); /* should never be null */
   if ( rmesa ) {
      GLboolean   release_texture_heaps;


      release_texture_heaps = (rmesa->glCtx->Shared->RefCount == 1);
      _swsetup_DestroyContext( rmesa->glCtx );
      _tnl_DestroyContext( rmesa->glCtx );
      _vbo_DestroyContext( rmesa->glCtx );
      _swrast_DestroyContext( rmesa->glCtx );

      radeonDestroySwtcl( rmesa->glCtx );
      radeonReleaseArrays( rmesa->glCtx, ~0 );
      if (rmesa->dma.current.buf) {
	 radeonReleaseDmaRegion( rmesa, &rmesa->dma.current, __FUNCTION__ );
	 radeonFlushCmdBuf( rmesa, __FUNCTION__ );
      }

      _mesa_vector4f_free( &rmesa->tcl.ObjClean );

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
radeonSwapBuffers( __DRIdrawablePrivate *dPriv )
{

   if (dPriv->driContextPriv && dPriv->driContextPriv->driverPrivate) {
      radeonContextPtr rmesa;
      GLcontext *ctx;
      rmesa = (radeonContextPtr) dPriv->driContextPriv->driverPrivate;
      ctx = rmesa->glCtx;
      if (ctx->Visual.doubleBufferMode) {
         _mesa_notifySwapBuffers( ctx );  /* flush pending rendering comands */

         if ( rmesa->doPageFlip ) {
            radeonPageFlip( dPriv );
         }
         else {
	     radeonCopyBuffer( dPriv, NULL );
         }
      }
   }
   else {
      /* XXX this shouldn't be an error but we can't handle it for now */
      _mesa_problem(NULL, "%s: drawable has no context!", __FUNCTION__);
   }
}

void radeonCopySubBuffer(__DRIdrawablePrivate * dPriv,
			 int x, int y, int w, int h )
{
    if (dPriv->driContextPriv && dPriv->driContextPriv->driverPrivate) {
	radeonContextPtr radeon;
	GLcontext *ctx;

	radeon = (radeonContextPtr) dPriv->driContextPriv->driverPrivate;
	ctx = radeon->glCtx;

	if (ctx->Visual.doubleBufferMode) {
	    drm_clip_rect_t rect;
	    rect.x1 = x + dPriv->x;
	    rect.y1 = (dPriv->h - y - h) + dPriv->y;
	    rect.x2 = rect.x1 + w;
	    rect.y2 = rect.y1 + h;
	    _mesa_notifySwapBuffers(ctx);	/* flush pending rendering comands */
	    radeonCopyBuffer(dPriv, &rect);
	}
    } else {
	/* XXX this shouldn't be an error but we can't handle it for now */
	_mesa_problem(NULL, "%s: drawable has no context!",
		      __FUNCTION__);
    }
}

/* Make context `c' the current context and bind it to the given
 * drawing and reading surfaces.
 */
GLboolean
radeonMakeCurrent( __DRIcontextPrivate *driContextPriv,
                   __DRIdrawablePrivate *driDrawPriv,
                   __DRIdrawablePrivate *driReadPriv )
{
   if ( driContextPriv ) {
      radeonContextPtr newCtx = 
	 (radeonContextPtr) driContextPriv->driverPrivate;

      if (RADEON_DEBUG & DEBUG_DRI)
	 fprintf(stderr, "%s ctx %p\n", __FUNCTION__, (void *) newCtx->glCtx);

      if ( newCtx->dri.drawable != driDrawPriv ) {
         /* XXX we may need to validate the drawable here!!! */
	 driDrawableInitVBlank( driDrawPriv, newCtx->vblank_flags,
				&newCtx->vbl_seq );
      }

      newCtx->dri.readable = driReadPriv;

      if ( (newCtx->dri.drawable != driDrawPriv) ||
           newCtx->lastStamp != driDrawPriv->lastStamp ) {
	 newCtx->dri.drawable = driDrawPriv;

	 radeonSetCliprects(newCtx);
	 radeonUpdateViewportOffset( newCtx->glCtx );
      }

      _mesa_make_current( newCtx->glCtx,
			  (GLframebuffer *) driDrawPriv->driverPrivate,
			  (GLframebuffer *) driReadPriv->driverPrivate );

      _mesa_update_state( newCtx->glCtx );
   } else {
      if (RADEON_DEBUG & DEBUG_DRI)
	 fprintf(stderr, "%s ctx is null\n", __FUNCTION__);
      _mesa_make_current( NULL, NULL, NULL );
   }

   if (RADEON_DEBUG & DEBUG_DRI)
      fprintf(stderr, "End %s\n", __FUNCTION__);
   return GL_TRUE;
}

/* Force the context `c' to be unbound from its buffer.
 */
GLboolean
radeonUnbindContext( __DRIcontextPrivate *driContextPriv )
{
   radeonContextPtr rmesa = (radeonContextPtr) driContextPriv->driverPrivate;

   if (RADEON_DEBUG & DEBUG_DRI)
      fprintf(stderr, "%s ctx %p\n", __FUNCTION__, (void *) rmesa->glCtx);

   return GL_TRUE;
}
