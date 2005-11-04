/*
 * Copyright 2001 by Alan Hourihane.
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
 * Authors:  Alan Hourihane, <alanh@tungstengraphics.com>
 *
 * 3DLabs Gamma driver.
 *
 */
#include "gamma_context.h"

#include "swrast/swrast.h"
#include "swrast_setup/swrast_setup.h"
#include "array_cache/acache.h"

#include "tnl/tnl.h"
#include "tnl/t_pipeline.h"

#include "drivers/common/driverfuncs.h"

#include "context.h"
#include "simple_list.h"
#include "imports.h"
#include "matrix.h"
#include "extensions.h"
#if defined(USE_X86_ASM)
#include "x86/common_x86_asm.h"
#endif
#include "simple_list.h"
#include "mm.h"


#include "gamma_vb.h"
#include "gamma_tris.h"

extern const struct tnl_pipeline_stage _gamma_render_stage;

static const struct tnl_pipeline_stage *gamma_pipeline[] = {
   &_tnl_vertex_transform_stage,
   &_tnl_normal_transform_stage,
   &_tnl_lighting_stage,
   &_tnl_fog_coordinate_stage,
   &_tnl_texgen_stage,
   &_tnl_texture_transform_stage,
				/* REMOVE: point attenuation stage */
#if 1
   &_gamma_render_stage,	/* ADD: unclipped rastersetup-to-dma */
#endif
   &_tnl_render_stage,
   0,
};

GLboolean gammaCreateContext( const __GLcontextModes *glVisual,
			     __DRIcontextPrivate *driContextPriv,
                     	     void *sharedContextPrivate)
{
   GLcontext *ctx, *shareCtx;
   __DRIscreenPrivate *sPriv = driContextPriv->driScreenPriv;
   gammaContextPtr gmesa;
   gammaScreenPtr gammascrn;
   GLINTSAREADRIPtr saPriv=(GLINTSAREADRIPtr)(((char*)sPriv->pSAREA)+
						 sizeof(drm_sarea_t));
   struct dd_function_table functions;

   gmesa = (gammaContextPtr) CALLOC( sizeof(*gmesa) );
   if (!gmesa)
      return GL_FALSE;

   /* Init default driver functions then plug in our gamma-specific functions
    * (the texture functions are especially important)
    */
   _mesa_init_driver_functions( &functions );
   gammaDDInitTextureFuncs( &functions );

   /* Allocate the Mesa context */
   if (sharedContextPrivate)
      shareCtx = ((gammaContextPtr) sharedContextPrivate)->glCtx;
   else
      shareCtx = NULL;

   gmesa->glCtx = _mesa_create_context(glVisual, shareCtx,
                                       &functions, (void *) gmesa);
   if (!gmesa->glCtx) {
      FREE(gmesa);
      return GL_FALSE;
   }

   gmesa->driContext = driContextPriv;
   gmesa->driScreen = sPriv;
   gmesa->driDrawable = NULL; /* Set by XMesaMakeCurrent */

   gmesa->hHWContext = driContextPriv->hHWContext;
   gmesa->driHwLock = &sPriv->pSAREA->lock;
   gmesa->driFd = sPriv->fd;
   gmesa->sarea = saPriv;

   gammascrn = gmesa->gammaScreen = (gammaScreenPtr)(sPriv->private);

   ctx = gmesa->glCtx;

   ctx->Const.MaxTextureLevels = GAMMA_TEX_MAXLEVELS;
   ctx->Const.MaxTextureUnits = 1; /* Permedia 3 */
   ctx->Const.MaxTextureImageUnits = 1;
   ctx->Const.MaxTextureCoordUnits = 1;

   ctx->Const.MinLineWidth = 0.0;
   ctx->Const.MaxLineWidth = 255.0;

   ctx->Const.MinLineWidthAA = 0.0;
   ctx->Const.MaxLineWidthAA = 65536.0;

   ctx->Const.MinPointSize = 0.0;
   ctx->Const.MaxPointSize = 255.0;

   ctx->Const.MinPointSizeAA = 0.5; /* 4x4 quality mode */
   ctx->Const.MaxPointSizeAA = 16.0; 
   ctx->Const.PointSizeGranularity = 0.25;

   gmesa->texHeap = mmInit( 0, gmesa->gammaScreen->textureSize );

   make_empty_list(&gmesa->TexObjList);
   make_empty_list(&gmesa->SwappedOut);

   gmesa->CurrentTexObj[0] = 0;
   gmesa->CurrentTexObj[1] = 0; /* Permedia 3, second texture */

   gmesa->RenderIndex = ~0;


   /* Initialize the software rasterizer and helper modules.
    */
   _swrast_CreateContext( ctx );
   _ac_CreateContext( ctx );
   _tnl_CreateContext( ctx );
   _swsetup_CreateContext( ctx );

   /* Install the customized pipeline:
    */
   _tnl_destroy_pipeline( ctx );
   _tnl_install_pipeline( ctx, gamma_pipeline );

   /* Configure swrast & TNL to match hardware characteristics:
    */
   _swrast_allow_pixel_fog( ctx, GL_FALSE );
   _swrast_allow_vertex_fog( ctx, GL_TRUE );
   _tnl_allow_pixel_fog( ctx, GL_FALSE );
   _tnl_allow_vertex_fog( ctx, GL_TRUE );

   gammaInitVB( ctx );
   gammaDDInitExtensions( ctx );
   /* XXX these should really go right after _mesa_init_driver_functions() */
   gammaDDInitDriverFuncs( ctx );
   gammaDDInitStateFuncs( ctx );
   gammaDDInitSpanFuncs( ctx );
   gammaDDInitTriFuncs( ctx );
   gammaDDInitState( gmesa );

   gammaInitTextureObjects( ctx );

   driContextPriv->driverPrivate = (void *)gmesa;

   GET_FIRST_DMA(gmesa->driFd, gmesa->hHWContext,
		  1, &gmesa->bufIndex, &gmesa->bufSize,
		  &gmesa->buf, &gmesa->bufCount, gammascrn);

#ifdef DO_VALIDATE
    GET_FIRST_DMA(gmesa->driFd, gmesa->hHWContext,
		  1, &gmesa->WCbufIndex, &gmesa->WCbufSize,
		  &gmesa->WCbuf, &gmesa->WCbufCount, gammascrn);
#endif

    switch (glVisual->depthBits) {
    case 16:
	gmesa->DeltaMode = DM_Depth16;
	gmesa->depth_scale = 1.0f / 0xffff;
	break;
    case 24:
	gmesa->DeltaMode = DM_Depth24;
	gmesa->depth_scale = 1.0f / 0xffffff;
	break;
    case 32:
	gmesa->DeltaMode = DM_Depth32;
	gmesa->depth_scale = 1.0f / 0xffffffff;
	break;
    default:
	break;
    }

    gmesa->DepthSize = glVisual->depthBits;
    gmesa->Flags  = GAMMA_FRONT_BUFFER;
    gmesa->Flags |= (glVisual->doubleBufferMode ? GAMMA_BACK_BUFFER : 0);
    gmesa->Flags |= (gmesa->DepthSize > 0 ? GAMMA_DEPTH_BUFFER : 0);

    gmesa->EnabledFlags = GAMMA_FRONT_BUFFER;
    gmesa->EnabledFlags |= (glVisual->doubleBufferMode ? GAMMA_BACK_BUFFER : 0);


    if (gmesa->Flags & GAMMA_BACK_BUFFER) {
        gmesa->readOffset = gmesa->drawOffset = gmesa->driScreen->fbHeight * gmesa->driScreen->fbWidth * gmesa->gammaScreen->cpp;
    } else {
    	gmesa->readOffset = gmesa->drawOffset = 0;
    }

    gammaInitHW( gmesa );

    driContextPriv->driverPrivate = (void *)gmesa;

    return GL_TRUE;
}
