/**************************************************************************

Copyright 2006 Stephane Marchesin
All Rights Reserved.

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
on the rights to use, copy, modify, merge, publish, distribute, sub
license, and/or sell copies of the Software, and to permit persons to whom
the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice (including the next
paragraph) shall be included in all copies or substantial portions of the
Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
ERIC ANHOLT OR SILICON INTEGRATED SYSTEMS CORP BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/

#include "glheader.h"
#include "context.h"
#include "simple_list.h"
#include "imports.h"
#include "matrix.h"
#include "swrast/swrast.h"
#include "swrast_setup/swrast_setup.h"
#include "framebuffer.h"

#include "tnl/tnl.h"
#include "tnl/t_pipeline.h"
#include "tnl/t_vp_build.h"

#include "drivers/common/driverfuncs.h"

#include "nouveau_context.h"
#include "nouveau_driver.h"
//#include "nouveau_state.h"
#include "nouveau_span.h"
#include "nouveau_object.h"
#include "nouveau_fifo.h"
#include "nouveau_tex.h"
#include "nouveau_msg.h"
#include "nouveau_reg.h"
#include "nouveau_lock.h"
#include "nouveau_query.h"
#include "nv04_swtcl.h"
#include "nv10_swtcl.h"

#include "vblank.h"
#include "utils.h"
#include "texmem.h"
#include "xmlpool.h" /* for symbolic values of enum-type options */

#ifndef NOUVEAU_DEBUG
int NOUVEAU_DEBUG = 0;
#endif

static const struct dri_debug_control debug_control[] =
{
	{ "shaders"   , DEBUG_SHADERS    },
	{ "mem"       , DEBUG_MEM        },
	{ "bufferobj" , DEBUG_BUFFEROBJ  },
	{ NULL        , 0                }
};

#define need_GL_ARB_vertex_program
#define need_GL_ARB_occlusion_query
#include "extension_helper.h"

const struct dri_extension common_extensions[] =
{
	{ NULL,    0 }
};

const struct dri_extension nv10_extensions[] =
{
	{ NULL,    0 }
};

const struct dri_extension nv20_extensions[] =
{
	{ NULL,    0 }
};

const struct dri_extension nv30_extensions[] =
{
	{ "GL_ARB_fragment_program",	NULL                            },
	{ NULL,    0 }
};

const struct dri_extension nv40_extensions[] =
{
   /* ARB_vp can be moved to nv20/30 once the shader backend has been
    * written for those cards.
    */
	{ "GL_ARB_vertex_program",	GL_ARB_vertex_program_functions },
	{ "GL_ARB_occlusion_query",	GL_ARB_occlusion_query_functions},
	{ NULL, 0 }
};

const struct dri_extension nv50_extensions[] =
{
	{ NULL,    0 }
};

/* Create the device specific context.
 */
GLboolean nouveauCreateContext( const __GLcontextModes *glVisual,
		__DRIcontextPrivate *driContextPriv,
		void *sharedContextPrivate )
{
	GLcontext *ctx, *shareCtx;
	__DRIscreenPrivate *sPriv = driContextPriv->driScreenPriv;
	struct dd_function_table functions;
	nouveauContextPtr nmesa;
	nouveauScreenPtr screen;

	/* Allocate the context */
	nmesa = (nouveauContextPtr) CALLOC( sizeof(*nmesa) );
	if ( !nmesa )
		return GL_FALSE;

	nmesa->driContext = driContextPriv;
	nmesa->driScreen = sPriv;
	nmesa->driDrawable = NULL;
	nmesa->hHWContext = driContextPriv->hHWContext;
	nmesa->driHwLock = &sPriv->pSAREA->lock;
	nmesa->driFd = sPriv->fd;

	nmesa->screen = (nouveauScreenPtr)(sPriv->private);
	screen=nmesa->screen;

	/* Create the hardware context */
	if (!nouveauDRMGetParam(nmesa, NOUVEAU_GETPARAM_FB_PHYSICAL,
		 		&nmesa->vram_phys))
	   return GL_FALSE;
	if (!nouveauDRMGetParam(nmesa, NOUVEAU_GETPARAM_FB_SIZE,
		 		&nmesa->vram_size))
	   return GL_FALSE;
	if (!nouveauDRMGetParam(nmesa, NOUVEAU_GETPARAM_AGP_PHYSICAL,
		 		&nmesa->agp_phys))
	   return GL_FALSE;
	if (!nouveauDRMGetParam(nmesa, NOUVEAU_GETPARAM_AGP_SIZE,
		 		&nmesa->agp_size))
	   return GL_FALSE;
	if (!nouveauFifoInit(nmesa))
	   return GL_FALSE;
	nouveauObjectInit(nmesa);


	/* Init default driver functions then plug in our nouveau-specific functions
	 * (the texture functions are especially important)
	 */
	_mesa_init_driver_functions( &functions );
	nouveauDriverInitFunctions( &functions );
	nouveauTexInitFunctions( &functions );

	/* Allocate the Mesa context */
	if (sharedContextPrivate)
		shareCtx = ((nouveauContextPtr) sharedContextPrivate)->glCtx;
	else 
		shareCtx = NULL;
	nmesa->glCtx = _mesa_create_context(glVisual, shareCtx,
			&functions, (void *) nmesa);
	if (!nmesa->glCtx) {
		FREE(nmesa);
		return GL_FALSE;
	}
	driContextPriv->driverPrivate = nmesa;
	ctx = nmesa->glCtx;

	/* Parse configuration files */
	driParseConfigFiles (&nmesa->optionCache, &screen->optionCache,
			screen->driScreen->myNum, "nouveau");

	nmesa->sarea = (drm_nouveau_sarea_t *)((char *)sPriv->pSAREA +
			screen->sarea_priv_offset);

	/* Enable any supported extensions */
	driInitExtensions(ctx, common_extensions, GL_TRUE);
	if (nmesa->screen->card->type >= NV_10)
		driInitExtensions(ctx, nv10_extensions, GL_FALSE);
	if (nmesa->screen->card->type >= NV_20)
		driInitExtensions(ctx, nv20_extensions, GL_FALSE);
	if (nmesa->screen->card->type >= NV_30)
		driInitExtensions(ctx, nv30_extensions, GL_FALSE);
	if (nmesa->screen->card->type >= NV_40)
		driInitExtensions(ctx, nv40_extensions, GL_FALSE);
	if (nmesa->screen->card->type >= NV_50)
		driInitExtensions(ctx, nv50_extensions, GL_FALSE);

	nmesa->current_primitive = -1;

	nouveauShaderInitFuncs(ctx);
	/* Install Mesa's fixed-function texenv shader support */
	if (nmesa->screen->card->type >= NV_40)
		ctx->FragmentProgram._MaintainTexEnvProgram = GL_TRUE;

	/* Initialize the swrast */
	_swrast_CreateContext( ctx );
	_vbo_CreateContext( ctx );
	_tnl_CreateContext( ctx );
	_swsetup_CreateContext( ctx );

	_math_matrix_ctr(&nmesa->viewport);

	nouveauDDInitStateFuncs( ctx );
	nouveauSpanInitFunctions( ctx );
	nouveauDDInitState( nmesa );
	switch(nmesa->screen->card->type)
	{
		case NV_03:
			//nv03TriInitFunctions( ctx );
			break;
		case NV_04:
		case NV_05:
			nv04TriInitFunctions( ctx );
			break;
		case NV_10:
		case NV_20:
		case NV_30:
		case NV_40:
		case NV_44:
		case NV_50:
		default:
			nv10TriInitFunctions( ctx );
			break;
	}

	nouveauInitBufferObjects(ctx);
	if (!nouveauSyncInitFuncs(ctx))
	   return GL_FALSE;
	nouveauQueryInitFuncs(ctx);
	nmesa->hw_func.InitCard(nmesa);
        nouveauInitState(ctx);

	driContextPriv->driverPrivate = (void *)nmesa;

	NOUVEAU_DEBUG = driParseDebugString( getenv( "NOUVEAU_DEBUG" ),
			debug_control );

	if (driQueryOptionb(&nmesa->optionCache, "no_rast")) {
		fprintf(stderr, "disabling 3D acceleration\n");
		FALLBACK(nmesa, NOUVEAU_FALLBACK_DISABLE, 1);
	}

	return GL_TRUE;
}

/* Destroy the device specific context. */
void nouveauDestroyContext( __DRIcontextPrivate *driContextPriv  )
{
	nouveauContextPtr nmesa = (nouveauContextPtr) driContextPriv->driverPrivate;

	assert(nmesa);
	if ( nmesa ) {
		/* free the option cache */
		driDestroyOptionCache (&nmesa->optionCache);

		FREE( nmesa );
	}

}


/* Force the context `c' to be the current context and associate with it
 * buffer `b'.
 */
GLboolean nouveauMakeCurrent( __DRIcontextPrivate *driContextPriv,
		__DRIdrawablePrivate *driDrawPriv,
		__DRIdrawablePrivate *driReadPriv )
{
	if ( driContextPriv ) {
		nouveauContextPtr nmesa = (nouveauContextPtr) driContextPriv->driverPrivate;
		struct gl_framebuffer *draw_fb =
			(struct gl_framebuffer*)driDrawPriv->driverPrivate;
		struct gl_framebuffer *read_fb =
			(struct gl_framebuffer*)driReadPriv->driverPrivate;

		driDrawableInitVBlank(driDrawPriv, nmesa->vblank_flags, &nmesa->vblank_seq );
		nmesa->driDrawable = driDrawPriv;

		_mesa_resize_framebuffer(nmesa->glCtx, draw_fb,
					 driDrawPriv->w, driDrawPriv->h);
		if (draw_fb != read_fb) {
			_mesa_resize_framebuffer(nmesa->glCtx, draw_fb,
						 driReadPriv->w,
						 driReadPriv->h);
		}
		_mesa_make_current(nmesa->glCtx, draw_fb, read_fb);

		nouveau_build_framebuffer(nmesa->glCtx,
		      			  driDrawPriv->driverPrivate);
	} else {
		_mesa_make_current( NULL, NULL, NULL );
	}

	return GL_TRUE;
}


/* Force the context `c' to be unbound from its buffer.
 */
GLboolean nouveauUnbindContext( __DRIcontextPrivate *driContextPriv )
{
	return GL_TRUE;
}

static void nouveauDoSwapBuffers(nouveauContextPtr nmesa,
				 __DRIdrawablePrivate *dPriv)
{
	struct gl_framebuffer *fb;
	nouveau_renderbuffer *src, *dst;
	drm_clip_rect_t *box;
	int nbox, i;

	fb = (struct gl_framebuffer *)dPriv->driverPrivate;
	dst = (nouveau_renderbuffer*)
		fb->Attachment[BUFFER_FRONT_LEFT].Renderbuffer;
	src = (nouveau_renderbuffer*)
		fb->Attachment[BUFFER_BACK_LEFT].Renderbuffer;

#ifdef ALLOW_MULTI_SUBCHANNEL
	LOCK_HARDWARE(nmesa);
	nbox = dPriv->numClipRects;
	box  = dPriv->pClipRects;

	if (nbox) {
		BEGIN_RING_SIZE(NvSubCtxSurf2D,
				NV10_CONTEXT_SURFACES_2D_FORMAT, 4);
		if (src->mesa._ActualFormat == GL_RGBA8)
			OUT_RING       (6); /* X8R8G8B8 */
		else
			OUT_RING       (4); /* R5G6B5 */
		OUT_RING       ((dst->pitch << 16) | src->pitch);
		OUT_RING       (src->offset);
		OUT_RING       (dst->offset);
	}

	for (i=0; i<nbox; i++, box++) {
		BEGIN_RING_SIZE(NvSubImageBlit, NV10_IMAGE_BLIT_SET_POINT, 3);
		OUT_RING       (((box->y1 - dPriv->y) << 16) |
				(box->x1 - dPriv->x));
		OUT_RING       ((box->y1 << 16) | box->x1);
		OUT_RING       (((box->y2 - box->y1) << 16) |
				(box->x2 - box->x1));
	}
	FIRE_RING();

	UNLOCK_HARDWARE(nmesa);
#endif
}

void nouveauSwapBuffers(__DRIdrawablePrivate *dPriv)
{
	if (dPriv->driContextPriv && dPriv->driContextPriv->driverPrivate) {
		nouveauContextPtr nmesa = dPriv->driContextPriv->driverPrivate;

		if (nmesa->glCtx->Visual.doubleBufferMode) {
			_mesa_notifySwapBuffers(nmesa->glCtx);
			nouveauDoSwapBuffers(nmesa, dPriv);
		}

	}
}

void nouveauCopySubBuffer(__DRIdrawablePrivate *dPriv,
			  int x, int y, int w, int h)
{
}

