/* $XFree86$ */ /* -*- mode: c; c-basic-offset: 3 -*- */
/*
 * Copyright 2000 Gareth Hughes
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
 * GARETH HUGHES BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/*
 * Authors:
 *	Gareth Hughes <gareth@valinux.com>
 *	Leif Delgass <ldelgass@retinalburn.net>
 *	Jos�Fonseca <j_r_fonseca@yahoo.co.uk>
 */

#include "glheader.h"
#include "context.h"
#include "simple_list.h"
#include "imports.h"
#include "matrix.h"
#include "extensions.h"

#include "swrast/swrast.h"
#include "swrast_setup/swrast_setup.h"
#include "vbo/vbo.h"

#include "tnl/tnl.h"
#include "tnl/t_pipeline.h"

#include "drivers/common/driverfuncs.h"

#include "mach64_context.h"
#include "mach64_ioctl.h"
#include "mach64_dd.h"
#include "mach64_span.h"
#include "mach64_state.h"
#include "mach64_tex.h"
#include "mach64_tris.h"
#include "mach64_vb.h"

#include "utils.h"
#include "vblank.h"

#define need_GL_ARB_multisample
#define need_GL_ARB_vertex_buffer_object
#include "extension_helper.h"

#ifndef MACH64_DEBUG
int MACH64_DEBUG = (0);
#endif

static const struct dri_debug_control debug_control[] =
{
    { "sync",   DEBUG_ALWAYS_SYNC },
    { "api",    DEBUG_VERBOSE_API },
    { "msg",    DEBUG_VERBOSE_MSG },
    { "lru",    DEBUG_VERBOSE_LRU },
    { "dri",    DEBUG_VERBOSE_DRI },
    { "ioctl",  DEBUG_VERBOSE_IOCTL },
    { "prims",  DEBUG_VERBOSE_PRIMS },
    { "count",  DEBUG_VERBOSE_COUNT },
    { "nowait", DEBUG_NOWAIT },
    { "fall",   DEBUG_VERBOSE_FALLBACK },
    { NULL,    0 }
};

const struct dri_extension card_extensions[] =
{
    { "GL_ARB_multisample",                GL_ARB_multisample_functions },
    { "GL_ARB_multitexture",               NULL },
    { "GL_ARB_vertex_buffer_object",       GL_ARB_vertex_buffer_object_functions },
    { "GL_EXT_texture_edge_clamp",         NULL },
    { "GL_MESA_ycbcr_texture",             NULL },
    { "GL_SGIS_generate_mipmap",           NULL },
    { NULL,                                NULL }
};


/* Create the device specific context.
  */
GLboolean mach64CreateContext( const __GLcontextModes *glVisual,
			       __DRIcontextPrivate *driContextPriv,
                               void *sharedContextPrivate )
{
   GLcontext *ctx, *shareCtx;
   __DRIscreenPrivate *driScreen = driContextPriv->driScreenPriv;
   struct dd_function_table functions;
   mach64ContextPtr mmesa;
   mach64ScreenPtr mach64Screen;
   int i, heap;
   GLuint *c_textureSwapsPtr = NULL;

#if DO_DEBUG
   MACH64_DEBUG = driParseDebugString(getenv("MACH64_DEBUG"), debug_control);
#endif

   /* Allocate the mach64 context */
   mmesa = (mach64ContextPtr) CALLOC( sizeof(*mmesa) );
   if ( !mmesa ) 
      return GL_FALSE;

   /* Init default driver functions then plug in our Mach64-specific functions
    * (the texture functions are especially important)
    */
   _mesa_init_driver_functions( &functions );
   mach64InitDriverFuncs( &functions );
   mach64InitIoctlFuncs( &functions );
   mach64InitTextureFuncs( &functions );

   /* Allocate the Mesa context */
   if (sharedContextPrivate)
      shareCtx = ((mach64ContextPtr) sharedContextPrivate)->glCtx;
   else 
      shareCtx = NULL;
   mmesa->glCtx = _mesa_create_context(glVisual, shareCtx, 
					&functions, (void *)mmesa);
   if (!mmesa->glCtx) {
      FREE(mmesa);
      return GL_FALSE;
   }
   driContextPriv->driverPrivate = mmesa;
   ctx = mmesa->glCtx;

   mmesa->driContext = driContextPriv;
   mmesa->driScreen = driScreen;
   mmesa->driDrawable = NULL;
   mmesa->hHWContext = driContextPriv->hHWContext;
   mmesa->driHwLock = &driScreen->pSAREA->lock;
   mmesa->driFd = driScreen->fd;

   mach64Screen = mmesa->mach64Screen = (mach64ScreenPtr)driScreen->private;

   /* Parse configuration files */
   driParseConfigFiles (&mmesa->optionCache, &mach64Screen->optionCache,
                        mach64Screen->driScreen->myNum, "mach64");

   mmesa->sarea = (drm_mach64_sarea_t *)((char *)driScreen->pSAREA +
				    sizeof(drm_sarea_t));

   mmesa->CurrentTexObj[0] = NULL;
   mmesa->CurrentTexObj[1] = NULL;

   (void) memset( mmesa->texture_heaps, 0, sizeof( mmesa->texture_heaps ) );
   make_empty_list( &mmesa->swapped );

   mmesa->firstTexHeap = mach64Screen->firstTexHeap;
   mmesa->lastTexHeap = mach64Screen->firstTexHeap + mach64Screen->numTexHeaps;

   for ( i = mmesa->firstTexHeap ; i < mmesa->lastTexHeap ; i++ ) {
      mmesa->texture_heaps[i] = driCreateTextureHeap( i, mmesa,
	    mach64Screen->texSize[i],
	    6, /* align to 64-byte boundary, use 12 for page-size boundary */
	    MACH64_NR_TEX_REGIONS,
	    (drmTextureRegionPtr)mmesa->sarea->tex_list[i],
	    &mmesa->sarea->tex_age[i],
	    &mmesa->swapped,
	    sizeof( mach64TexObj ),
	    (destroy_texture_object_t *) mach64DestroyTexObj );

#if ENABLE_PERF_BOXES
      c_textureSwapsPtr = & mmesa->c_textureSwaps;
#endif
      driSetTextureSwapCounterLocation( mmesa->texture_heaps[i],
					c_textureSwapsPtr );
   }

   mmesa->RenderIndex = -1;		/* Impossible value */
   mmesa->vert_buf = NULL;
   mmesa->num_verts = 0;
   mmesa->new_state = MACH64_NEW_ALL;
   mmesa->dirty = MACH64_UPLOAD_ALL;

   /* Set the maximum texture size small enough that we can
    * guarentee that both texture units can bind a maximal texture
    * and have them both in memory (on-card or AGP) at once.
    * Test for 2 textures * bytes/texel * size * size.  There's no
    * need to account for mipmaps since we only upload one level.
    */

   ctx->Const.MaxTextureUnits = 2;
   ctx->Const.MaxTextureImageUnits = 2;
   ctx->Const.MaxTextureCoordUnits = 2;

   heap = mach64Screen->IsPCI ? MACH64_CARD_HEAP : MACH64_AGP_HEAP;

   driCalculateMaxTextureLevels( & mmesa->texture_heaps[heap],
				 1,
				 & ctx->Const,
				 mach64Screen->cpp,
				 10, /* max 2D texture size is 1024x1024 */
				 0,  /* 3D textures unsupported. */
				 0,  /* cube textures unsupported. */
				 0,  /* texture rectangles unsupported. */
				 1,  /* mipmapping unsupported. */
				 GL_TRUE, /* need to have both textures in
					     either local or AGP memory */
				 0 );

#if ENABLE_PERF_BOXES
   mmesa->boxes = ( getenv( "LIBGL_PERFORMANCE_BOXES" ) != NULL );
#endif

   /* Allocate the vertex buffer
    */
   mmesa->vert_buf = ALIGN_MALLOC(MACH64_BUFFER_SIZE, 32);
   if ( !mmesa->vert_buf )
      return GL_FALSE;
   mmesa->vert_used = 0;
   mmesa->vert_total = MACH64_BUFFER_SIZE;
   
   /* Initialize the software rasterizer and helper modules.
    */
   _swrast_CreateContext( ctx );
   _vbo_CreateContext( ctx );
   _tnl_CreateContext( ctx );
   _swsetup_CreateContext( ctx );

   /* Install the customized pipeline:
    */
/*     _tnl_destroy_pipeline( ctx ); */
/*     _tnl_install_pipeline( ctx, mach64_pipeline ); */

   /* Configure swrast and T&L to match hardware characteristics:
    */
   _swrast_allow_pixel_fog( ctx, GL_FALSE );
   _swrast_allow_vertex_fog( ctx, GL_TRUE );
   _tnl_allow_pixel_fog( ctx, GL_FALSE );
   _tnl_allow_vertex_fog( ctx, GL_TRUE );

   driInitExtensions( ctx, card_extensions, GL_TRUE );

   mach64InitVB( ctx );
   mach64InitTriFuncs( ctx );
   mach64DDInitStateFuncs( ctx );
   mach64DDInitSpanFuncs( ctx );
   mach64DDInitState( mmesa );

   mmesa->do_irqs = (mmesa->mach64Screen->irq && !getenv("MACH64_NO_IRQS"));

   mmesa->vblank_flags = (mmesa->do_irqs)
      ? driGetDefaultVBlankFlags(&mmesa->optionCache) : VBLANK_FLAG_NO_IRQ;

   driContextPriv->driverPrivate = (void *)mmesa;

   if (driQueryOptionb(&mmesa->optionCache, "no_rast")) {
      fprintf(stderr, "disabling 3D acceleration\n");
      FALLBACK(mmesa, MACH64_FALLBACK_DISABLE, 1);
   }

   return GL_TRUE;
}

/* Destroy the device specific context.
 */
void mach64DestroyContext( __DRIcontextPrivate *driContextPriv  )
{
   mach64ContextPtr mmesa = (mach64ContextPtr) driContextPriv->driverPrivate;

   assert(mmesa);  /* should never be null */
   if ( mmesa ) {
      GLboolean   release_texture_heaps;

      release_texture_heaps = (mmesa->glCtx->Shared->RefCount == 1);

      _swsetup_DestroyContext( mmesa->glCtx );
      _tnl_DestroyContext( mmesa->glCtx );
      _vbo_DestroyContext( mmesa->glCtx );
      _swrast_DestroyContext( mmesa->glCtx );

      if (release_texture_heaps) {
         /* This share group is about to go away, free our private
          * texture object data.
          */
         int i;

         for ( i = mmesa->firstTexHeap ; i < mmesa->lastTexHeap ; i++ ) {
	    driDestroyTextureHeap( mmesa->texture_heaps[i] );
	    mmesa->texture_heaps[i] = NULL;
         }

	 assert( is_empty_list( & mmesa->swapped ) );
      }

      mach64FreeVB( mmesa->glCtx );

      /* Free the vertex buffer */
      if ( mmesa->vert_buf )
	 ALIGN_FREE( mmesa->vert_buf );
      
      /* free the Mesa context */
      mmesa->glCtx->DriverCtx = NULL;
      _mesa_destroy_context(mmesa->glCtx);

      FREE( mmesa );
   }
}

/* Force the context `c' to be the current context and associate with it
 * buffer `b'.
 */
GLboolean
mach64MakeCurrent( __DRIcontextPrivate *driContextPriv,
                 __DRIdrawablePrivate *driDrawPriv,
                 __DRIdrawablePrivate *driReadPriv )
{
   if ( driContextPriv ) {
      GET_CURRENT_CONTEXT(ctx);
      mach64ContextPtr oldMach64Ctx = ctx ? MACH64_CONTEXT(ctx) : NULL;
      mach64ContextPtr newMach64Ctx = (mach64ContextPtr) driContextPriv->driverPrivate;

      if ( newMach64Ctx != oldMach64Ctx ) {
	 newMach64Ctx->new_state |= MACH64_NEW_CONTEXT;
	 newMach64Ctx->dirty = MACH64_UPLOAD_ALL;
      }

      
      driDrawableInitVBlank( driDrawPriv, newMach64Ctx->vblank_flags,
			     &newMach64Ctx->vbl_seq );

      if ( newMach64Ctx->driDrawable != driDrawPriv ) {
	 newMach64Ctx->driDrawable = driDrawPriv;
	 mach64CalcViewport( newMach64Ctx->glCtx );
      }

      _mesa_make_current( newMach64Ctx->glCtx,
                          (GLframebuffer *) driDrawPriv->driverPrivate,
                          (GLframebuffer *) driReadPriv->driverPrivate );


      newMach64Ctx->new_state |=  MACH64_NEW_CLIP;
   } else {
      _mesa_make_current( NULL, NULL, NULL );
   }

   return GL_TRUE;
}


/* Force the context `c' to be unbound from its buffer.
 */
GLboolean
mach64UnbindContext( __DRIcontextPrivate *driContextPriv )
{
   return GL_TRUE;
}
