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
 * \file tdfx_context.c
 * Context management functions for 3Dfx hardware.
 *
 * \author Gareth Hughes <gareth@valinux.com> (original rewrite 29 Sep - 1 Oct 2000)
 * \author Brian Paul <brianp@valinux.com>
 * \author Daniel Borca <dborca@users.sourceforge.net> (new fixes 19 Jul 2004)
 */

#include <dlfcn.h>
#include "tdfx_context.h"
#include "tdfx_dd.h"
#include "tdfx_state.h"
#include "tdfx_vb.h"
#include "tdfx_tex.h"
#include "tdfx_tris.h"
#include "tdfx_render.h"
#include "tdfx_span.h"
#include "tdfx_texman.h"
#include "extensions.h"
#include "hash.h"
#include "texobj.h"

#include "swrast/swrast.h"
#include "swrast_setup/swrast_setup.h"
#include "vbo/vbo.h"

#include "tnl/tnl.h"
#include "tnl/t_pipeline.h"

#include "drivers/common/driverfuncs.h"

#include "utils.h"

#define need_GL_ARB_multisample
/* #define need_GL_ARB_point_parameters */
#define need_GL_ARB_occlusion_query
#define need_GL_ARB_texture_compression
#define need_GL_ARB_vertex_buffer_object
/* #define need_GL_ARB_vertex_program */
#define need_GL_EXT_blend_equation_separate
#define need_GL_EXT_blend_func_separate
#define need_GL_EXT_blend_minmax
#define need_GL_EXT_fog_coord
#define need_GL_EXT_multi_draw_arrays
#define need_GL_EXT_paletted_texture
/* #define need_GL_EXT_secondary_color */
#define need_GL_IBM_multimode_draw_arrays
/* #define need_GL_MESA_program_debug */
/* #define need_GL_NV_vertex_program */
#include "extension_helper.h"


/**
 * Common extension strings exported by all cards
 */
const struct dri_extension card_extensions[] =
{
    { "GL_ARB_multisample",                GL_ARB_multisample_functions },
    { "GL_ARB_occlusion_query",            GL_ARB_occlusion_query_functions },
    { "GL_ARB_texture_mirrored_repeat",    NULL },
    { "GL_ARB_vertex_buffer_object",       GL_ARB_vertex_buffer_object_functions },

    { "GL_EXT_blend_func_separate",        GL_EXT_blend_func_separate_functions },
    { "GL_EXT_fog_coord",                  GL_EXT_fog_coord_functions },
    { "GL_EXT_multi_draw_arrays",          GL_EXT_multi_draw_arrays_functions },
    { "GL_EXT_paletted_texture",           GL_EXT_paletted_texture_functions },
    { "GL_EXT_shared_texture_palette",     NULL },
    { "GL_EXT_stencil_wrap",               NULL },
    { "GL_EXT_texture_env_add",            NULL },
    { "GL_EXT_texture_lod_bias",           NULL },
    { "GL_IBM_multimode_draw_arrays",      GL_IBM_multimode_draw_arrays_functions },

#ifdef need_GL_ARB_point_parameters
    { "GL_ARB_point_parameters",           GL_ARB_point_parameters_functions },
    { "GL_ARB_point_sprite",               NULL },
#endif
#ifdef need_GL_EXT_secondary_color
    { "GL_EXT_secondary_color",            GL_EXT_secondary_color_functions },
#endif
#ifdef need_GL_ARB_vertex_program
    { "GL_ARB_vertex_program",             GL_ARB_vertex_program_functions }
#endif
#ifdef need_GL_NV_vertex_program
    { "GL_NV_vertex_program",              GL_NV_vertex_program_functions }
    { "GL_NV_vertex_program1_1",           NULL },
#endif
#ifdef need_GL_MESA_program_debug
    { "GL_MESA_program_debug",             GL_MESA_program_debug_functions },
#endif
    { NULL,                                NULL }
};

/**
 * Extension strings exported only by Naplam (e.g., Voodoo4 & Voodoo5) cards.
 */
const struct dri_extension napalm_extensions[] =
{
    { "GL_ARB_texture_compression",        GL_ARB_texture_compression_functions },
    { "GL_ARB_texture_env_combine",        NULL },
    { "GL_EXT_blend_equation_separate",    GL_EXT_blend_equation_separate_functions },
    { "GL_EXT_blend_subtract",             GL_EXT_blend_minmax_functions },
    { "GL_EXT_texture_compression_s3tc",   NULL },
    { "GL_EXT_texture_env_combine",        NULL },

    { "GL_3DFX_texture_compression_FXT1",  NULL },
    { "GL_NV_blend_square",                NULL },
    { "GL_S3_s3tc",                        NULL },
    { NULL,                                NULL }
};

/*
 * Enable/Disable the extensions for this context.
 */
static void tdfxDDInitExtensions( GLcontext *ctx )
{
   tdfxContextPtr fxMesa = TDFX_CONTEXT(ctx);

   driInitExtensions( ctx, card_extensions, GL_FALSE );

   if ( fxMesa->haveTwoTMUs ) {
      _mesa_enable_extension( ctx, "GL_ARB_multitexture" );
   }

   if ( TDFX_IS_NAPALM( fxMesa ) ) {
      driInitExtensions( ctx, napalm_extensions, GL_FALSE );
   } else {
      _mesa_enable_extension( ctx, "GL_SGIS_generate_mipmap" );
   }
}



static const struct tnl_pipeline_stage *tdfx_pipeline[] = {
   &_tnl_vertex_transform_stage, 
   &_tnl_normal_transform_stage, 
   &_tnl_lighting_stage,
   &_tnl_fog_coordinate_stage, 
   &_tnl_texgen_stage, 
   &_tnl_texture_transform_stage, 
   &_tnl_point_attenuation_stage,
   &_tnl_render_stage,		
   0,
};

static const struct dri_debug_control debug_control[] =
{
    { "dri",   DEBUG_VERBOSE_DRI },
    { "sync",  DEBUG_ALWAYS_SYNC },
    { "api",   DEBUG_VERBOSE_API },
    { "fall",  DEBUG_VERBOSE_FALL },
    { NULL,    0 }
};

GLboolean tdfxCreateContext( const __GLcontextModes *mesaVis,
			     __DRIcontextPrivate *driContextPriv,
                             void *sharedContextPrivate )
{
   tdfxContextPtr fxMesa;
   GLcontext *ctx, *shareCtx;
   __DRIscreenPrivate *sPriv = driContextPriv->driScreenPriv;
   tdfxScreenPrivate *fxScreen = (tdfxScreenPrivate *) sPriv->private;
   TDFXSAREAPriv *saPriv = (TDFXSAREAPriv *) ((char *) sPriv->pSAREA +
					      sizeof(drm_sarea_t));
   struct dd_function_table functions;

   /* Allocate tdfx context */
   fxMesa = (tdfxContextPtr) CALLOC( sizeof(tdfxContextRec) );
   if (!fxMesa)
      return GL_FALSE;

   /* Init default driver functions then plug in our tdfx-specific functions
    * (the texture functions are especially important)
    */
   _mesa_init_driver_functions(&functions);
   tdfxDDInitDriverFuncs(mesaVis, &functions);
   tdfxInitTextureFuncs(&functions);
   tdfxInitRenderFuncs(&functions);

   /* Allocate the Mesa context */
   if (sharedContextPrivate)
      shareCtx = ((tdfxContextPtr) sharedContextPrivate)->glCtx;
   else 
      shareCtx = NULL;

   fxMesa->glCtx = _mesa_create_context(mesaVis, shareCtx,
                                        &functions, (void *) fxMesa);
   if (!fxMesa->glCtx) {
      FREE(fxMesa);
      return GL_FALSE;
   }
   driContextPriv->driverPrivate = fxMesa;

   /* Mirror some important DRI state
    */
   fxMesa->hHWContext = driContextPriv->hHWContext;
   fxMesa->driHwLock = &sPriv->pSAREA->lock;
   fxMesa->driFd = sPriv->fd;

   fxMesa->driScreen = sPriv;
   fxMesa->driContext = driContextPriv;
   fxMesa->fxScreen = fxScreen;
   fxMesa->sarea = saPriv;

   /*JJJ - really?*/
   fxMesa->haveHwAlpha = ( mesaVis->alphaBits &&
                           ((mesaVis->greenBits == 8) ||
                            (mesaVis->depthBits == 0)) );
   fxMesa->haveHwStencil = ( TDFX_IS_NAPALM( fxMesa ) &&
			     mesaVis->stencilBits &&
			     mesaVis->depthBits == 24 );

   fxMesa->screen_width = fxScreen->width;
   fxMesa->screen_height = fxScreen->height;

   fxMesa->new_gl_state = ~0;
   fxMesa->new_state = ~0;
   fxMesa->dirty = ~0;

   /* Parse configuration files */
   driParseConfigFiles (&fxMesa->optionCache, &fxScreen->optionCache,
                        fxMesa->driScreen->myNum, "tdfx");

   /* NOTE: This must be here before any Glide calls! */
   if (!tdfxInitGlide( fxMesa )) {
      FREE(fxMesa);
      return GL_FALSE;
   }

   fxMesa->Glide.grDRIOpen( (char*) sPriv->pFB, fxScreen->regs.map, fxScreen->deviceID,
	      fxScreen->width, fxScreen->height, fxScreen->mem, fxScreen->cpp,
	      fxScreen->stride, fxScreen->fifoOffset, fxScreen->fifoSize,
	      fxScreen->fbOffset, fxScreen->backOffset, fxScreen->depthOffset,
	      fxScreen->textureOffset, fxScreen->textureSize, &saPriv->fifoPtr,
	      &saPriv->fifoRead );

   if ( getenv( "FX_GLIDE_SWAPINTERVAL" ) ) {
      fxMesa->Glide.SwapInterval = atoi( getenv( "FX_GLIDE_SWAPINTERVAL" ) );
   } else {
      fxMesa->Glide.SwapInterval = 0;
   }
   if ( getenv( "FX_MAX_PENDING_SWAPS" ) ) {
      fxMesa->Glide.MaxPendingSwaps = atoi( getenv( "FX_MAX_PENDING_SWAPS" ) );
   } else {
      fxMesa->Glide.MaxPendingSwaps = 2;
   }

   fxMesa->Glide.Initialized = GL_FALSE;
   fxMesa->Glide.Board = 0;


   if (getenv("FX_EMULATE_SINGLE_TMU")) {
      fxMesa->haveTwoTMUs = GL_FALSE;
   }
   else {
      if ( TDFX_IS_BANSHEE( fxMesa ) ) {
         fxMesa->haveTwoTMUs = GL_FALSE;
      } else {
         fxMesa->haveTwoTMUs = GL_TRUE;
      }
   }

   fxMesa->stats.swapBuffer = 0;
   fxMesa->stats.reqTexUpload = 0;
   fxMesa->stats.texUpload = 0;
   fxMesa->stats.memTexUpload = 0;

   fxMesa->tmuSrc = TDFX_TMU_NONE;

   ctx = fxMesa->glCtx;
   if ( TDFX_IS_NAPALM( fxMesa ) ) {
      ctx->Const.MaxTextureLevels = 12;
   } else {
      ctx->Const.MaxTextureLevels = 9;
   }
   ctx->Const.MaxTextureUnits = TDFX_IS_BANSHEE( fxMesa ) ? 1 : 2;
   ctx->Const.MaxTextureImageUnits = ctx->Const.MaxTextureUnits;
   ctx->Const.MaxTextureCoordUnits = ctx->Const.MaxTextureUnits;

   /* No wide points.
    */
   ctx->Const.MinPointSize = 1.0;
   ctx->Const.MinPointSizeAA = 1.0;
   ctx->Const.MaxPointSize = 1.0;
   ctx->Const.MaxPointSizeAA = 1.0;

   /* Disable wide lines as we can't antialias them correctly in
    * hardware.
    */
   ctx->Const.MinLineWidth = 1.0;
   ctx->Const.MinLineWidthAA = 1.0;
   ctx->Const.MaxLineWidth = 1.0;
   ctx->Const.MaxLineWidthAA = 1.0;
   ctx->Const.LineWidthGranularity = 1.0;

   /* Initialize the software rasterizer and helper modules.
    */
   _swrast_CreateContext( ctx );
   _vbo_CreateContext( ctx );
   _tnl_CreateContext( ctx );
   _swsetup_CreateContext( ctx );

   /* Install the customized pipeline:
    */
   _tnl_destroy_pipeline( ctx );
   _tnl_install_pipeline( ctx, tdfx_pipeline );

   /* Configure swrast and T&L to match hardware characteristics:
    */
   _swrast_allow_pixel_fog( ctx, GL_TRUE );
   _swrast_allow_vertex_fog( ctx, GL_FALSE );
   _tnl_allow_pixel_fog( ctx, GL_TRUE );
   _tnl_allow_vertex_fog( ctx, GL_FALSE );

   tdfxDDInitExtensions( ctx );
   /* XXX these should really go right after _mesa_init_driver_functions() */
   tdfxDDInitSpanFuncs( ctx ); 
   tdfxDDInitStateFuncs( ctx );
   tdfxDDInitTriFuncs( ctx );
   tdfxInitVB( ctx );
   tdfxInitState( fxMesa );

#if DO_DEBUG
   TDFX_DEBUG = driParseDebugString( getenv( "TDFX_DEBUG" ), debug_control );
#endif

   if (driQueryOptionb(&fxMesa->optionCache, "no_rast")) {
      fprintf(stderr, "disabling 3D acceleration\n");
      FALLBACK(fxMesa, TDFX_FALLBACK_DISABLE, 1);
   }

   return GL_TRUE;
}


static GLboolean tdfxInitVertexFormats( tdfxContextPtr fxMesa )
{
   FxI32 result;
   int i;

   LOCK_HARDWARE( fxMesa );

   fxMesa->Glide.grGet( GR_GLIDE_VERTEXLAYOUT_SIZE, sizeof(FxI32), &result );
   for ( i = 0 ; i < TDFX_NUM_LAYOUTS ; i++ ) {
      fxMesa->layout[i] = MALLOC( result );
      if ( !fxMesa->layout[i] ) {
	 UNLOCK_HARDWARE( fxMesa );
	 return GL_FALSE;
      }
   }

   /* Tiny vertex format - 16 bytes.
    */
   fxMesa->Glide.grReset( GR_VERTEX_PARAMETER );
   fxMesa->Glide.grCoordinateSpace( GR_WINDOW_COORDS );
   fxMesa->Glide.grVertexLayout( GR_PARAM_XY,	TDFX_XY_OFFSET, GR_PARAM_ENABLE );
   fxMesa->Glide.grVertexLayout( GR_PARAM_Z, TDFX_Z_OFFSET, GR_PARAM_ENABLE );
   fxMesa->Glide.grVertexLayout( GR_PARAM_PARGB, TDFX_ARGB_OFFSET, GR_PARAM_ENABLE );
   fxMesa->Glide.grGlideGetVertexLayout( fxMesa->layout[TDFX_LAYOUT_TINY] );

   /* Non textured vertex format - 24 bytes (Need w for table fog)
    */
   fxMesa->Glide.grReset( GR_VERTEX_PARAMETER );
   fxMesa->Glide.grCoordinateSpace( GR_WINDOW_COORDS );
   fxMesa->Glide.grVertexLayout( GR_PARAM_XY,	TDFX_XY_OFFSET, GR_PARAM_ENABLE );
   fxMesa->Glide.grVertexLayout( GR_PARAM_Z, TDFX_Z_OFFSET, GR_PARAM_ENABLE );
   fxMesa->Glide.grVertexLayout( GR_PARAM_Q, TDFX_Q_OFFSET, GR_PARAM_ENABLE );
   fxMesa->Glide.grVertexLayout( GR_PARAM_PARGB, TDFX_ARGB_OFFSET, GR_PARAM_ENABLE );
   fxMesa->Glide.grGlideGetVertexLayout( fxMesa->layout[TDFX_LAYOUT_NOTEX] );

   /* Single textured vertex format - 32 bytes.
    */
   fxMesa->Glide.grReset( GR_VERTEX_PARAMETER );
   fxMesa->Glide.grCoordinateSpace( GR_WINDOW_COORDS );
   fxMesa->Glide.grVertexLayout( GR_PARAM_XY,	TDFX_XY_OFFSET, GR_PARAM_ENABLE );
   fxMesa->Glide.grVertexLayout( GR_PARAM_Z, TDFX_Z_OFFSET, GR_PARAM_ENABLE );
   fxMesa->Glide.grVertexLayout( GR_PARAM_Q, TDFX_Q_OFFSET, GR_PARAM_ENABLE );
   fxMesa->Glide.grVertexLayout( GR_PARAM_PARGB, TDFX_ARGB_OFFSET, GR_PARAM_ENABLE );
   fxMesa->Glide.grVertexLayout( GR_PARAM_ST0, TDFX_ST0_OFFSET, GR_PARAM_ENABLE );
   fxMesa->Glide.grGlideGetVertexLayout( fxMesa->layout[TDFX_LAYOUT_SINGLE] );

   /* Multitextured vertex format - 40 bytes.
    */
   fxMesa->Glide.grReset( GR_VERTEX_PARAMETER );
   fxMesa->Glide.grCoordinateSpace( GR_WINDOW_COORDS );
   fxMesa->Glide.grVertexLayout( GR_PARAM_XY, TDFX_XY_OFFSET, GR_PARAM_ENABLE );
   fxMesa->Glide.grVertexLayout( GR_PARAM_Z, TDFX_Z_OFFSET, GR_PARAM_ENABLE );
   fxMesa->Glide.grVertexLayout( GR_PARAM_Q, TDFX_Q_OFFSET, GR_PARAM_ENABLE );
   fxMesa->Glide.grVertexLayout( GR_PARAM_PARGB, TDFX_ARGB_OFFSET, GR_PARAM_ENABLE );
   fxMesa->Glide.grVertexLayout( GR_PARAM_ST0, TDFX_ST0_OFFSET, GR_PARAM_ENABLE );
   fxMesa->Glide.grVertexLayout( GR_PARAM_ST1, TDFX_ST1_OFFSET, GR_PARAM_ENABLE );
   fxMesa->Glide.grGlideGetVertexLayout( fxMesa->layout[TDFX_LAYOUT_MULTI] );

   /* Projected texture vertex format - 36 bytes.
    */
   fxMesa->Glide.grReset( GR_VERTEX_PARAMETER );
   fxMesa->Glide.grCoordinateSpace( GR_WINDOW_COORDS );
   fxMesa->Glide.grVertexLayout( GR_PARAM_XY, TDFX_XY_OFFSET, GR_PARAM_ENABLE );
   fxMesa->Glide.grVertexLayout( GR_PARAM_Z, TDFX_Z_OFFSET, GR_PARAM_ENABLE );
   fxMesa->Glide.grVertexLayout( GR_PARAM_Q, TDFX_Q_OFFSET, GR_PARAM_ENABLE );
   fxMesa->Glide.grVertexLayout( GR_PARAM_PARGB, TDFX_ARGB_OFFSET, GR_PARAM_ENABLE );
   fxMesa->Glide.grVertexLayout( GR_PARAM_ST0, TDFX_ST0_OFFSET, GR_PARAM_ENABLE );
   fxMesa->Glide.grVertexLayout( GR_PARAM_Q0, TDFX_Q0_OFFSET, GR_PARAM_ENABLE );
   fxMesa->Glide.grGlideGetVertexLayout( fxMesa->layout[TDFX_LAYOUT_PROJ1] );

   /* Projected multitexture vertex format - 48 bytes.
    */
   fxMesa->Glide.grReset( GR_VERTEX_PARAMETER );
   fxMesa->Glide.grCoordinateSpace( GR_WINDOW_COORDS );
   fxMesa->Glide.grVertexLayout( GR_PARAM_XY, TDFX_XY_OFFSET, GR_PARAM_ENABLE );
   fxMesa->Glide.grVertexLayout( GR_PARAM_Z, TDFX_Z_OFFSET, GR_PARAM_ENABLE );
   fxMesa->Glide.grVertexLayout( GR_PARAM_Q, TDFX_Q_OFFSET, GR_PARAM_ENABLE );
   fxMesa->Glide.grVertexLayout( GR_PARAM_PARGB, TDFX_ARGB_OFFSET, GR_PARAM_ENABLE );
   fxMesa->Glide.grVertexLayout( GR_PARAM_ST0, TDFX_ST0_OFFSET, GR_PARAM_ENABLE );
   fxMesa->Glide.grVertexLayout( GR_PARAM_Q0, TDFX_Q0_OFFSET, GR_PARAM_ENABLE );
   fxMesa->Glide.grVertexLayout( GR_PARAM_ST1, TDFX_ST1_OFFSET, GR_PARAM_ENABLE );
   fxMesa->Glide.grVertexLayout( GR_PARAM_Q1, TDFX_Q1_OFFSET, GR_PARAM_ENABLE );
   fxMesa->Glide.grGlideGetVertexLayout( fxMesa->layout[TDFX_LAYOUT_PROJ2] );

   UNLOCK_HARDWARE( fxMesa );

   return GL_TRUE;
}


/*
 * Initialize the state in an tdfxContextPtr struct.
 */
static GLboolean
tdfxInitContext( __DRIdrawablePrivate *driDrawPriv, tdfxContextPtr fxMesa )
{
   /* KW: Would be nice to make one of these a member of the other.
    */
   FxI32 result[2];
   const char *gext;

   if ( TDFX_DEBUG & DEBUG_VERBOSE_DRI ) {
      fprintf( stderr, "%s( %p )\n", __FUNCTION__, (void *)fxMesa );
   }

#if DEBUG_LOCKING
   fprintf(stderr, "Debug locking enabled\n");
#endif

   if ( fxMesa->Glide.Initialized )
      return GL_TRUE;

   fxMesa->width = driDrawPriv->w;
   fxMesa->height = driDrawPriv->h;

   /* We have to use a light lock here, because we can't do any glide
    * operations yet. No use of FX_* functions in this function.
    */
   DRM_LIGHT_LOCK( fxMesa->driFd, fxMesa->driHwLock, fxMesa->hHWContext );

   fxMesa->Glide.grGlideInit();
   fxMesa->Glide.grSstSelect( fxMesa->Glide.Board );

   fxMesa->Glide.Context = fxMesa->Glide.grSstWinOpen( (FxU32) -1,
					 GR_RESOLUTION_NONE,
					 GR_REFRESH_NONE,
					 fxMesa->Glide.ColorFormat,
					 fxMesa->Glide.Origin,
					 2, 1 );

   fxMesa->Glide.grDRIResetSAREA();

   DRM_UNLOCK( fxMesa->driFd, fxMesa->driHwLock, fxMesa->hHWContext );

   if ( !fxMesa->Glide.Context )
      return GL_FALSE;


   /* Perform the Glide-dependant part of the context initialization.
    */
   FX_grColorMaskv( fxMesa->glCtx, true4 );

   tdfxTMInit( fxMesa );

   LOCK_HARDWARE( fxMesa );

   /* JJJ - COMMAND_TRANSPORT, PALETTE6666 */
   gext = fxMesa->Glide.grGetString( GR_EXTENSION );
   fxMesa->Glide.HaveCombineExt = strstr(gext, "COMBINE") && !getenv("MESA_FX_IGNORE_CMBEXT");
   fxMesa->Glide.HaveCommandTransportExt = GL_FALSE;
   fxMesa->Glide.HaveFogCoordExt = GL_TRUE;
   fxMesa->Glide.HavePixelExt = strstr(gext, "PIXEXT") && !getenv("MESA_FX_IGNORE_PIXEXT");
   fxMesa->Glide.HaveTextureBufferExt = GL_TRUE;
   fxMesa->Glide.HaveTexFmtExt = strstr(gext, "TEXFMT") && !getenv("MESA_FX_IGNORE_TEXFMT");
   fxMesa->Glide.HaveTexUMAExt = strstr(gext, "TEXUMA") && !getenv("MESA_FX_IGNORE_TEXUMA");
   fxMesa->Glide.HaveMirrorExt = strstr(gext, "TEXMIRROR") && !getenv("MESA_FX_IGNORE_MIREXT");
   fxMesa->Glide.HaveTexus2 = GL_FALSE;

   if ( fxMesa->glCtx->Visual.depthBits > 0 ) {
      fxMesa->Glide.grDepthBufferMode(GR_DEPTHBUFFER_ZBUFFER);
   } else {
      fxMesa->Glide.grDepthBufferMode(GR_DEPTHBUFFER_DISABLE);
   }

   fxMesa->Glide.grLfbWriteColorFormat( GR_COLORFORMAT_ABGR );

   fxMesa->Glide.grGet( GR_TEXTURE_ALIGN, sizeof(FxI32), result );
   fxMesa->Glide.TextureAlign = result[0];

   fxMesa->Glide.State = NULL;
   fxMesa->Glide.grGet( GR_GLIDE_STATE_SIZE, sizeof(FxI32), result );
   fxMesa->Glide.State = MALLOC( result[0] );

   fxMesa->Fog.Table = NULL;
   fxMesa->Glide.grGet( GR_FOG_TABLE_ENTRIES, sizeof(FxI32), result );
   fxMesa->Fog.Table = MALLOC( result[0] * sizeof(GrFog_t) );

   UNLOCK_HARDWARE( fxMesa );

   if ( !fxMesa->Glide.State || !fxMesa->Fog.Table ) {
      if ( fxMesa->Glide.State )
	 FREE( fxMesa->Glide.State );
      if ( fxMesa->Fog.Table )
	 FREE( fxMesa->Fog.Table );
      return GL_FALSE;
   }

   if ( !tdfxInitVertexFormats( fxMesa ) ) {
      return GL_FALSE;
   }

   LOCK_HARDWARE( fxMesa );

   fxMesa->Glide.grGlideGetState( fxMesa->Glide.State );

   if ( getenv( "FX_GLIDE_INFO" ) ) {
      printf( "GR_RENDERER  = %s\n", (char *) fxMesa->Glide.grGetString( GR_RENDERER ) );
      printf( "GR_VERSION   = %s\n", (char *) fxMesa->Glide.grGetString( GR_VERSION ) );
      printf( "GR_VENDOR    = %s\n", (char *) fxMesa->Glide.grGetString( GR_VENDOR ) );
      printf( "GR_HARDWARE  = %s\n", (char *) fxMesa->Glide.grGetString( GR_HARDWARE ) );
      printf( "GR_EXTENSION = %s\n", (char *) gext );
   }

   UNLOCK_HARDWARE( fxMesa );

   fxMesa->numClipRects = 0;
   fxMesa->pClipRects = NULL;
   fxMesa->scissoredClipRects = GL_FALSE;

   fxMesa->Glide.Initialized = GL_TRUE;

   return GL_TRUE;
}


void
tdfxDestroyContext( __DRIcontextPrivate *driContextPriv )
{
   tdfxContextPtr fxMesa = (tdfxContextPtr) driContextPriv->driverPrivate;

   if ( TDFX_DEBUG & DEBUG_VERBOSE_DRI ) {
      fprintf( stderr, "%s( %p )\n", __FUNCTION__, (void *)fxMesa );
   }

   if ( fxMesa ) {
      if (fxMesa->glCtx->Shared->RefCount == 1 && fxMesa->driDrawable) {
         /* This share group is about to go away, free our private
          * texture object data.
          */
         struct _mesa_HashTable *textures = fxMesa->glCtx->Shared->TexObjects;
         GLuint id;
         for (id = _mesa_HashFirstEntry(textures);
              id;
              id = _mesa_HashNextEntry(textures, id)) {
            struct gl_texture_object *tObj
               = _mesa_lookup_texture(fxMesa->glCtx, id);
            tdfxTMFreeTexture(fxMesa, tObj);
         }
      }

      tdfxTMClose(fxMesa);  /* free texture memory */

      _swsetup_DestroyContext( fxMesa->glCtx );
      _tnl_DestroyContext( fxMesa->glCtx );
      _vbo_DestroyContext( fxMesa->glCtx );
      _swrast_DestroyContext( fxMesa->glCtx );

      tdfxFreeVB( fxMesa->glCtx );

      /* Free Mesa context */
      fxMesa->glCtx->DriverCtx = NULL;
      _mesa_destroy_context(fxMesa->glCtx);

      /* free the tdfx context */
      FREE( fxMesa );
   }
}


GLboolean
tdfxUnbindContext( __DRIcontextPrivate *driContextPriv )
{
   GET_CURRENT_CONTEXT(ctx);
   tdfxContextPtr fxMesa = TDFX_CONTEXT(ctx);

   if ( TDFX_DEBUG & DEBUG_VERBOSE_DRI ) {
      fprintf( stderr, "%s( %p )\n", __FUNCTION__, (void *)driContextPriv );
   }

   if ( driContextPriv && (tdfxContextPtr) driContextPriv == fxMesa ) {
      LOCK_HARDWARE(fxMesa);
      fxMesa->Glide.grGlideGetState(fxMesa->Glide.State);
      UNLOCK_HARDWARE(fxMesa);
   }
   return GL_TRUE;
}


GLboolean
tdfxMakeCurrent( __DRIcontextPrivate *driContextPriv,
                 __DRIdrawablePrivate *driDrawPriv,
                 __DRIdrawablePrivate *driReadPriv )
{
   if ( TDFX_DEBUG & DEBUG_VERBOSE_DRI ) {
      fprintf( stderr, "%s( %p )\n", __FUNCTION__, (void *)driContextPriv );
   }

   if ( driContextPriv ) {
      tdfxContextPtr newFx = (tdfxContextPtr) driContextPriv->driverPrivate;
      GLcontext *newCtx = newFx->glCtx;
      GET_CURRENT_CONTEXT(curCtx);

      if ((newFx->driDrawable != driDrawPriv)
	  || (newFx->driReadable != driReadPriv)) {
	 newFx->driDrawable = driDrawPriv;
	 newFx->driReadable = driReadPriv;
	 newFx->dirty = ~0;
      }
      else {
         if (curCtx == newCtx) {
            /* same drawable, same context -> no-op */
            /* Need to call _mesa_make_current2() in order to make sure API
             * dispatch is set correctly.
             */
            _mesa_make_current( newCtx,
                                (GLframebuffer *) driDrawPriv->driverPrivate,
                                (GLframebuffer *) driReadPriv->driverPrivate );
            return GL_TRUE;
	 }
	 /* [dBorca] tunnel2 requires this */
	 newFx->dirty = ~0;
      }

      driUpdateFramebufferSize(newCtx, driDrawPriv);
      if (driDrawPriv != driReadPriv) {
	 driUpdateFramebufferSize(newCtx, driReadPriv);
      }

      if ( !newFx->Glide.Initialized ) {
	 if ( !tdfxInitContext( driDrawPriv, newFx ) )
	    return GL_FALSE;

	 LOCK_HARDWARE( newFx );

	 /* FIXME: Force loading of window information */
	 newFx->width = 0;
         tdfxUpdateClipping(newCtx);
         tdfxUploadClipping(newFx);

	 UNLOCK_HARDWARE( newFx );
      } else {
	 LOCK_HARDWARE( newFx );

	 newFx->Glide.grSstSelect( newFx->Glide.Board );
	 newFx->Glide.grGlideSetState( newFx->Glide.State );

         tdfxUpdateClipping(newCtx);
         tdfxUploadClipping(newFx);

	 UNLOCK_HARDWARE( newFx );
      }

      _mesa_make_current( newCtx,
                          (GLframebuffer *) driDrawPriv->driverPrivate,
                          (GLframebuffer *) driReadPriv->driverPrivate );
   } else {
      _mesa_make_current( NULL, NULL, NULL );
   }

   return GL_TRUE;
}


/*
 * Enable this to trace calls to various Glide functions.
 */
/*#define DEBUG_TRAP*/
#ifdef DEBUG_TRAP
static void (*real_grDrawTriangle)( const void *a, const void *b, const void *c );
static void (*real_grDrawPoint)( const void *a );
static void (*real_grDrawVertexArray)(FxU32 mode, FxU32 Count, void *pointers);
static void (*real_grDrawVertexArrayContiguous)(FxU32 mode, FxU32 Count,
                                       void *pointers, FxU32 stride);
static void (*real_grClipWindow)( FxU32 minx, FxU32 miny, FxU32 maxx, FxU32 maxy );

static void (*real_grVertexLayout)(FxU32 param, FxI32 offset, FxU32 mode);
static void (*real_grGlideGetVertexLayout)( void *layout );
static void (*real_grGlideSetVertexLayout)( const void *layout );

static void (*real_grTexDownloadMipMapLevel)( GrChipID_t        tmu,
                                     FxU32             startAddress,
                                     GrLOD_t           thisLod,
                                     GrLOD_t           largeLod,
                                     GrAspectRatio_t   aspectRatio,
                                     GrTextureFormat_t format,
                                     FxU32             evenOdd,
                                              void              *data );


static void debug_grDrawTriangle( const void *a, const void *b, const void *c )
{
   printf("%s\n", __FUNCTION__);
   (*real_grDrawTriangle)(a, b, c);
}

static void debug_grDrawPoint( const void *a )
{
   const float *f = (const float *) a;
   printf("%s %g %g\n", __FUNCTION__, f[0], f[1]);
   (*real_grDrawPoint)(a);
}

static void debug_grDrawVertexArray(FxU32 mode, FxU32 Count, void *pointers)
{
   printf("%s count=%d\n", __FUNCTION__, (int) Count);
   (*real_grDrawVertexArray)(mode, Count, pointers);
}

static void debug_grDrawVertexArrayContiguous(FxU32 mode, FxU32 Count,
                                       void *pointers, FxU32 stride)
{
   printf("%s mode=0x%x count=%d\n", __FUNCTION__, (int) mode, (int) Count);
   (*real_grDrawVertexArrayContiguous)(mode, Count, pointers, stride);
}

static void debug_grClipWindow( FxU32 minx, FxU32 miny, FxU32 maxx, FxU32 maxy )
{
   printf("%s %d,%d .. %d,%d\n", __FUNCTION__,
          (int) minx, (int) miny, (int) maxx, (int) maxy);
   (*real_grClipWindow)(minx, miny, maxx, maxy);
}

static void debug_grVertexLayout(FxU32 param, FxI32 offset, FxU32 mode)
{
   (*real_grVertexLayout)(param, offset, mode);
}

static void debug_grGlideGetVertexLayout( void *layout )
{
   (*real_grGlideGetVertexLayout)(layout);
}

static void debug_grGlideSetVertexLayout( const void *layout )
{
   (*real_grGlideSetVertexLayout)(layout);
}

static void debug_grTexDownloadMipMapLevel( GrChipID_t        tmu,
                                     FxU32             startAddress,
                                     GrLOD_t           thisLod,
                                     GrLOD_t           largeLod,
                                     GrAspectRatio_t   aspectRatio,
                                     GrTextureFormat_t format,
                                     FxU32             evenOdd,
                                     void              *data )
{
   (*real_grTexDownloadMipMapLevel)(tmu, startAddress, thisLod, largeLod,
                                    aspectRatio, format, evenOdd, data);
}

#endif


/*
 * Examine the context's deviceID to determine what kind of 3dfx hardware
 * is installed.  dlopen() the appropriate Glide library and initialize
 * this context's Glide function pointers.
 * Return:  true/false = success/failure
 */
GLboolean tdfxInitGlide(tdfxContextPtr tmesa)
{
   static const char *defaultGlide = "libglide3.so";
   const char *libName;
   void *libHandle;

   /*
    * XXX this code which selects a Glide library filename given the
    * deviceID may need to be cleaned up a bit.
    * Non-Linux systems may have different filenames, for example.
    */
   switch (tmesa->fxScreen->deviceID) {
   case PCI_CHIP_BANSHEE:
   case PCI_CHIP_VOODOO3:
      libName = "libglide3-v3.so";
      break;
   case PCI_CHIP_VOODOO5:   /* same as PCI_CHIP_VOODOO4 */
      libName = "libglide3-v5.so";
      break;
   default:
      {
         __driUtilMessage("unrecognized 3dfx deviceID: 0x%x",
                 tmesa->fxScreen->deviceID);
      }
      return GL_FALSE;
   }

   libHandle = dlopen(libName, RTLD_NOW);
   if (!libHandle) {
      /* The device-specific Glide library filename didn't work, try the
       * old, generic libglide3.so library.
       */
      libHandle = dlopen(defaultGlide, RTLD_NOW); 
      if (!libHandle) {
         __driUtilMessage(
            "can't find Glide library, dlopen(%s) and dlopen(%s) both failed.",
            libName, defaultGlide);
         __driUtilMessage("dlerror() message: %s", dlerror());
         return GL_FALSE;
      }
      libName = defaultGlide;
   }

   {
      const char *env = getenv("LIBGL_DEBUG");
      if (env && strstr(env, "verbose")) {
         fprintf(stderr, "libGL: using Glide library %s\n", libName);
      }
   }         

#define GET_FUNCTION(PTR, NAME)						\
   tmesa->Glide.PTR = dlsym(libHandle, NAME);				\
   if (!tmesa->Glide.PTR) {						\
      __driUtilMessage("couldn't find Glide function %s in %s.",	\
              NAME, libName);						\
   }

   GET_FUNCTION(grDrawPoint, "grDrawPoint");
   GET_FUNCTION(grDrawLine, "grDrawLine");
   GET_FUNCTION(grDrawTriangle, "grDrawTriangle");
   GET_FUNCTION(grVertexLayout, "grVertexLayout");
   GET_FUNCTION(grDrawVertexArray, "grDrawVertexArray");
   GET_FUNCTION(grDrawVertexArrayContiguous, "grDrawVertexArrayContiguous");
   GET_FUNCTION(grBufferClear, "grBufferClear");
   /*GET_FUNCTION(grBufferSwap, "grBufferSwap");*/
   GET_FUNCTION(grRenderBuffer, "grRenderBuffer");
   GET_FUNCTION(grErrorSetCallback, "grErrorSetCallback");
   GET_FUNCTION(grFinish, "grFinish");
   GET_FUNCTION(grFlush, "grFlush");
   GET_FUNCTION(grSstWinOpen, "grSstWinOpen");
   GET_FUNCTION(grSstWinClose, "grSstWinClose");
#if 0
   /* Not in V3 lib, and not used anyway. */
   GET_FUNCTION(grSetNumPendingBuffers, "grSetNumPendingBuffers");
#endif
   GET_FUNCTION(grSelectContext, "grSelectContext");
   GET_FUNCTION(grSstOrigin, "grSstOrigin");
   GET_FUNCTION(grSstSelect, "grSstSelect");
   GET_FUNCTION(grAlphaBlendFunction, "grAlphaBlendFunction");
   GET_FUNCTION(grAlphaCombine, "grAlphaCombine");
   GET_FUNCTION(grAlphaControlsITRGBLighting, "grAlphaControlsITRGBLighting");
   GET_FUNCTION(grAlphaTestFunction, "grAlphaTestFunction");
   GET_FUNCTION(grAlphaTestReferenceValue, "grAlphaTestReferenceValue");
   GET_FUNCTION(grChromakeyMode, "grChromakeyMode");
   GET_FUNCTION(grChromakeyValue, "grChromakeyValue");
   GET_FUNCTION(grClipWindow, "grClipWindow");
   GET_FUNCTION(grColorCombine, "grColorCombine");
   GET_FUNCTION(grColorMask, "grColorMask");
   GET_FUNCTION(grCullMode, "grCullMode");
   GET_FUNCTION(grConstantColorValue, "grConstantColorValue");
   GET_FUNCTION(grDepthBiasLevel, "grDepthBiasLevel");
   GET_FUNCTION(grDepthBufferFunction, "grDepthBufferFunction");
   GET_FUNCTION(grDepthBufferMode, "grDepthBufferMode");
   GET_FUNCTION(grDepthMask, "grDepthMask");
   GET_FUNCTION(grDisableAllEffects, "grDisableAllEffects");
   GET_FUNCTION(grDitherMode, "grDitherMode");
   GET_FUNCTION(grFogColorValue, "grFogColorValue");
   GET_FUNCTION(grFogMode, "grFogMode");
   GET_FUNCTION(grFogTable, "grFogTable");
   GET_FUNCTION(grLoadGammaTable, "grLoadGammaTable");
   GET_FUNCTION(grSplash, "grSplash");
   GET_FUNCTION(grGet, "grGet");
   GET_FUNCTION(grGetString, "grGetString");
   GET_FUNCTION(grQueryResolutions, "grQueryResolutions");
   GET_FUNCTION(grReset, "grReset");
   GET_FUNCTION(grGetProcAddress, "grGetProcAddress");
   GET_FUNCTION(grEnable, "grEnable");
   GET_FUNCTION(grDisable, "grDisable");
   GET_FUNCTION(grCoordinateSpace, "grCoordinateSpace");
   GET_FUNCTION(grDepthRange, "grDepthRange");
   GET_FUNCTION(grStippleMode, "grStippleMode");
   GET_FUNCTION(grStipplePattern, "grStipplePattern");
   GET_FUNCTION(grViewport, "grViewport");
   GET_FUNCTION(grTexCalcMemRequired, "grTexCalcMemRequired");
   GET_FUNCTION(grTexTextureMemRequired, "grTexTextureMemRequired");
   GET_FUNCTION(grTexMinAddress, "grTexMinAddress");
   GET_FUNCTION(grTexMaxAddress, "grTexMaxAddress");
   GET_FUNCTION(grTexNCCTable, "grTexNCCTable");
   GET_FUNCTION(grTexSource, "grTexSource");
   GET_FUNCTION(grTexClampMode, "grTexClampMode");
   GET_FUNCTION(grTexCombine, "grTexCombine");
   GET_FUNCTION(grTexDetailControl, "grTexDetailControl");
   GET_FUNCTION(grTexFilterMode, "grTexFilterMode");
   GET_FUNCTION(grTexLodBiasValue, "grTexLodBiasValue");
   GET_FUNCTION(grTexDownloadMipMap, "grTexDownloadMipMap");
   GET_FUNCTION(grTexDownloadMipMapLevel, "grTexDownloadMipMapLevel");
   GET_FUNCTION(grTexDownloadMipMapLevelPartial, "grTexDownloadMipMapLevelPartial");
   GET_FUNCTION(grTexDownloadTable, "grTexDownloadTable");
   GET_FUNCTION(grTexDownloadTablePartial, "grTexDownloadTablePartial");
   GET_FUNCTION(grTexMipMapMode, "grTexMipMapMode");
   GET_FUNCTION(grTexMultibase, "grTexMultibase");
   GET_FUNCTION(grTexMultibaseAddress, "grTexMultibaseAddress");
   GET_FUNCTION(grLfbLock, "grLfbLock");
   GET_FUNCTION(grLfbUnlock, "grLfbUnlock");
   GET_FUNCTION(grLfbConstantAlpha, "grLfbConstantAlpha");
   GET_FUNCTION(grLfbConstantDepth, "grLfbConstantDepth");
   GET_FUNCTION(grLfbWriteColorSwizzle, "grLfbWriteColorSwizzle");
   GET_FUNCTION(grLfbWriteColorFormat, "grLfbWriteColorFormat");
   GET_FUNCTION(grLfbWriteRegion, "grLfbWriteRegion");
   GET_FUNCTION(grLfbReadRegion, "grLfbReadRegion");
   GET_FUNCTION(grGlideInit, "grGlideInit");
   GET_FUNCTION(grGlideShutdown, "grGlideShutdown");
   GET_FUNCTION(grGlideGetState, "grGlideGetState");
   GET_FUNCTION(grGlideSetState, "grGlideSetState");
   GET_FUNCTION(grGlideGetVertexLayout, "grGlideGetVertexLayout");
   GET_FUNCTION(grGlideSetVertexLayout, "grGlideSetVertexLayout");

   /* Glide utility functions */
   GET_FUNCTION(guFogGenerateExp, "guFogGenerateExp");
   GET_FUNCTION(guFogGenerateExp2, "guFogGenerateExp2");
   GET_FUNCTION(guFogGenerateLinear, "guFogGenerateLinear");

   /* DRI functions */
   GET_FUNCTION(grDRIOpen, "grDRIOpen");
   GET_FUNCTION(grDRIPosition, "grDRIPosition");
   /*GET_FUNCTION(grDRILostContext, "grDRILostContext");*/
   GET_FUNCTION(grDRIImportFifo, "grDRIImportFifo");
   GET_FUNCTION(grDRIInvalidateAll, "grDRIInvalidateAll");
   GET_FUNCTION(grDRIResetSAREA, "grDRIResetSAREA");
   GET_FUNCTION(grDRIBufferSwap, "grDRIBufferSwap");

   /*
    * Extension functions:
    * Just use dlysm() because we want a NULL pointer if the function is
    * not found.
    */
   /* PIXEXT extension */
   tmesa->Glide.grStencilFunc = dlsym(libHandle, "grStencilFunc");
   tmesa->Glide.grStencilMask = dlsym(libHandle, "grStencilMask");
   tmesa->Glide.grStencilOp = dlsym(libHandle, "grStencilOp");
   tmesa->Glide.grBufferClearExt = dlsym(libHandle, "grBufferClearExt");
   tmesa->Glide.grColorMaskExt = dlsym(libHandle, "grColorMaskExt");
   /* COMBINE extension */
   tmesa->Glide.grColorCombineExt = dlsym(libHandle, "grColorCombineExt");
   tmesa->Glide.grTexColorCombineExt = dlsym(libHandle, "grTexColorCombineExt");
   tmesa->Glide.grAlphaCombineExt = dlsym(libHandle, "grAlphaCombineExt");
   tmesa->Glide.grTexAlphaCombineExt = dlsym(libHandle, "grTexAlphaCombineExt");
   tmesa->Glide.grAlphaBlendFunctionExt = dlsym(libHandle, "grAlphaBlendFunctionExt");
   tmesa->Glide.grConstantColorValueExt = dlsym(libHandle, "grConstantColorValueExt");
   /* Texus 2 */
   tmesa->Glide.txImgQuantize = dlsym(libHandle, "txImgQuantize");
   tmesa->Glide.txImgDequantizeFXT1 = dlsym(libHandle, "_txImgDequantizeFXT1");
   tmesa->Glide.txErrorSetCallback = dlsym(libHandle, "txErrorSetCallback");
   
#ifdef DEBUG_TRAP
   /* wrap the drawing functions so we can trap them */
   real_grDrawTriangle = tmesa->Glide.grDrawTriangle;
   tmesa->Glide.grDrawTriangle = debug_grDrawTriangle;

   real_grDrawPoint = tmesa->Glide.grDrawPoint;
   tmesa->Glide.grDrawPoint = debug_grDrawPoint;

   real_grDrawVertexArray = tmesa->Glide.grDrawVertexArray;
   tmesa->Glide.grDrawVertexArray = debug_grDrawVertexArray;

   real_grDrawVertexArrayContiguous = tmesa->Glide.grDrawVertexArrayContiguous;
   tmesa->Glide.grDrawVertexArrayContiguous = debug_grDrawVertexArrayContiguous;

   real_grClipWindow = tmesa->Glide.grClipWindow;
   tmesa->Glide.grClipWindow = debug_grClipWindow;

   real_grVertexLayout = tmesa->Glide.grVertexLayout;
   tmesa->Glide.grVertexLayout = debug_grVertexLayout;

   real_grGlideGetVertexLayout = tmesa->Glide.grGlideGetVertexLayout;
   tmesa->Glide.grGlideGetVertexLayout = debug_grGlideGetVertexLayout;

   real_grGlideSetVertexLayout = tmesa->Glide.grGlideSetVertexLayout;
   tmesa->Glide.grGlideSetVertexLayout = debug_grGlideSetVertexLayout;

   real_grTexDownloadMipMapLevel = tmesa->Glide.grTexDownloadMipMapLevel;
   tmesa->Glide.grTexDownloadMipMapLevel = debug_grTexDownloadMipMapLevel;

#endif
   return GL_TRUE;
}
