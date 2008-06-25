/* $XFree86: xc/lib/GL/mesa/src/drv/ffb/ffb_xmesa.c,v 1.4 2002/02/22 21:32:59 dawes Exp $
 *
 * GLX Hardware Device Driver for Sun Creator/Creator3D
 * Copyright (C) 2000, 2001 David S. Miller
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * DAVID MILLER, OR ANY OTHER CONTRIBUTORS BE LIABLE FOR ANY CLAIM, 
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR 
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE 
 * OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 *
 *    David S. Miller <davem@redhat.com>
 */

#include "ffb_xmesa.h"
#include "context.h"
#include "framebuffer.h"
#include "matrix.h"
#include "renderbuffer.h"
#include "simple_list.h"
#include "imports.h"
#include "utils.h"

#include "swrast/swrast.h"
#include "swrast_setup/swrast_setup.h"
#include "tnl/tnl.h"
#include "tnl/t_pipeline.h"
#include "vbo/vbo.h"
#include "drivers/common/driverfuncs.h"

#include "ffb_context.h"
#include "ffb_dd.h"
#include "ffb_span.h"
#include "ffb_depth.h"
#include "ffb_stencil.h"
#include "ffb_clear.h"
#include "ffb_vb.h"
#include "ffb_tris.h"
#include "ffb_lines.h"
#include "ffb_points.h"
#include "ffb_state.h"
#include "ffb_tex.h"
#include "ffb_lock.h"
#include "ffb_vtxfmt.h"
#include "ffb_bitmap.h"

#include "drm_sarea.h"

#include "drirenderbuffer.h"

static GLboolean
ffbInitDriver(__DRIscreenPrivate *sPriv)
{
	ffbScreenPrivate *ffbScreen;
	FFBDRIPtr gDRIPriv = (FFBDRIPtr) sPriv->pDevPriv;
	drmAddress map;

	if (getenv("LIBGL_FORCE_XSERVER"))
		return GL_FALSE;


   	if (sPriv->devPrivSize != sizeof(FFBDRIRec)) {
      		fprintf(stderr,"\nERROR!  sizeof(FFBDRIRec) does not match passed size from device driver\n");
      		return GL_FALSE;
   	}

	/* Allocate the private area. */
	ffbScreen = (ffbScreenPrivate *) MALLOC(sizeof(ffbScreenPrivate));
	if (!ffbScreen)
		return GL_FALSE;

	/* Map FBC registers. */
	if (drmMap(sPriv->fd,
		   gDRIPriv->hFbcRegs,
		   gDRIPriv->sFbcRegs,
		   &map)) {
	        FREE(ffbScreen);
		return GL_FALSE;
	}
	ffbScreen->regs = (ffb_fbcPtr) map;

	/* Map ramdac registers. */
	if (drmMap(sPriv->fd,
		   gDRIPriv->hDacRegs,
		   gDRIPriv->sDacRegs,
		   &map)) {
		drmUnmap((drmAddress)ffbScreen->regs, gDRIPriv->sFbcRegs);
		FREE(ffbScreen);
		return GL_FALSE;
	}
	ffbScreen->dac = (ffb_dacPtr) map;

	/* Map "Smart" framebuffer views. */
	if (drmMap(sPriv->fd,
		   gDRIPriv->hSfb8r,
		   gDRIPriv->sSfb8r,
		   &map)) {
		drmUnmap((drmAddress)ffbScreen->regs, gDRIPriv->sFbcRegs);
		drmUnmap((drmAddress)ffbScreen->dac, gDRIPriv->sDacRegs);
		FREE(ffbScreen);
		return GL_FALSE;
	}
	ffbScreen->sfb8r = (volatile char *) map;

	if (drmMap(sPriv->fd,
		   gDRIPriv->hSfb32,
		   gDRIPriv->sSfb32,
		   &map)) {
		drmUnmap((drmAddress)ffbScreen->regs, gDRIPriv->sFbcRegs);
		drmUnmap((drmAddress)ffbScreen->dac, gDRIPriv->sDacRegs);
		drmUnmap((drmAddress)ffbScreen->sfb8r, gDRIPriv->sSfb8r);
		FREE(ffbScreen);
		return GL_FALSE;
	}
	ffbScreen->sfb32 = (volatile char *) map;

	if (drmMap(sPriv->fd,
		   gDRIPriv->hSfb64,
		   gDRIPriv->sSfb64,
		   &map)) {
		drmUnmap((drmAddress)ffbScreen->regs, gDRIPriv->sFbcRegs);
		drmUnmap((drmAddress)ffbScreen->dac, gDRIPriv->sDacRegs);
		drmUnmap((drmAddress)ffbScreen->sfb8r, gDRIPriv->sSfb8r);
		drmUnmap((drmAddress)ffbScreen->sfb32, gDRIPriv->sSfb32);
		FREE(ffbScreen);
		return GL_FALSE;
	}
	ffbScreen->sfb64 = (volatile char *) map;

	ffbScreen->fifo_cache = 0;
	ffbScreen->rp_active = 0;

	ffbScreen->sPriv = sPriv;
	sPriv->private = (void *) ffbScreen;

	ffbDDLinefuncInit();
	ffbDDPointfuncInit();

	return GL_TRUE;
}


static void
ffbDestroyScreen(__DRIscreenPrivate *sPriv)
{
	ffbScreenPrivate *ffbScreen = sPriv->private;
	FFBDRIPtr gDRIPriv = (FFBDRIPtr) sPriv->pDevPriv;

	drmUnmap((drmAddress)ffbScreen->regs, gDRIPriv->sFbcRegs);
	drmUnmap((drmAddress)ffbScreen->dac, gDRIPriv->sDacRegs);
	drmUnmap((drmAddress)ffbScreen->sfb8r, gDRIPriv->sSfb8r);
	drmUnmap((drmAddress)ffbScreen->sfb32, gDRIPriv->sSfb32);
	drmUnmap((drmAddress)ffbScreen->sfb64, gDRIPriv->sSfb64);

	FREE(ffbScreen);
}

static const struct tnl_pipeline_stage *ffb_pipeline[] = {
   &_tnl_vertex_transform_stage, 
   &_tnl_normal_transform_stage, 
   &_tnl_lighting_stage,
				/* REMOVE: fog coord stage */
   &_tnl_texgen_stage, 
   &_tnl_texture_transform_stage, 
				/* REMOVE: point attenuation stage */
   &_tnl_render_stage,		
   0,
};

/* Create and initialize the Mesa and driver specific context data */
static GLboolean
ffbCreateContext(const __GLcontextModes *mesaVis,
                 __DRIcontextPrivate *driContextPriv,
                 void *sharedContextPrivate)
{
	ffbContextPtr fmesa;
	GLcontext *ctx, *shareCtx;
	__DRIscreenPrivate *sPriv;
	ffbScreenPrivate *ffbScreen;
	char *debug;
	struct dd_function_table functions;

        /* Allocate ffb context */
	fmesa = (ffbContextPtr) CALLOC(sizeof(ffbContextRec));
	if (!fmesa)
		return GL_FALSE;

	_mesa_init_driver_functions(&functions);

        /* Allocate Mesa context */
        if (sharedContextPrivate)
           shareCtx = ((ffbContextPtr) sharedContextPrivate)->glCtx;
        else 
           shareCtx = NULL;
        fmesa->glCtx = _mesa_create_context(mesaVis, shareCtx,
                                            &functions, fmesa);
        if (!fmesa->glCtx) {
           FREE(fmesa);
           return GL_FALSE;
        }
        driContextPriv->driverPrivate = fmesa;
        ctx = fmesa->glCtx;

	sPriv = driContextPriv->driScreenPriv;
	ffbScreen = (ffbScreenPrivate *) sPriv->private;

	/* Dri stuff. */
	fmesa->hHWContext = driContextPriv->hHWContext;
	fmesa->driFd = sPriv->fd;
	fmesa->driHwLock = &sPriv->pSAREA->lock;

	fmesa->ffbScreen = ffbScreen;
	fmesa->driScreen = sPriv;
	fmesa->ffb_sarea = FFB_DRISHARE(sPriv->pSAREA);

	/* Register and framebuffer hw pointers. */
	fmesa->regs = ffbScreen->regs;
	fmesa->sfb32 = ffbScreen->sfb32;

	ffbDDInitContextHwState(ctx);

	/* Default clear and depth colors. */
	{
		GLubyte r = (GLint) (ctx->Color.ClearColor[0] * 255.0F);
		GLubyte g = (GLint) (ctx->Color.ClearColor[1] * 255.0F);
		GLubyte b = (GLint) (ctx->Color.ClearColor[2] * 255.0F);

		fmesa->clear_pixel = ((r << 0) |
				      (g << 8) |
				      (b << 16));
	}
	fmesa->clear_depth = Z_FROM_MESA(ctx->Depth.Clear * 4294967295.0f);
	fmesa->clear_stencil = ctx->Stencil.Clear & 0xf;

	/* No wide points. */
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

	/* Instead of having GCC emit these constants a zillion times
	 * everywhere in the driver, put them here.
	 */
	fmesa->ffb_2_30_fixed_scale           = __FFB_2_30_FIXED_SCALE;
	fmesa->ffb_one_over_2_30_fixed_scale  = (1.0 / __FFB_2_30_FIXED_SCALE);
	fmesa->ffb_16_16_fixed_scale          = __FFB_16_16_FIXED_SCALE;
	fmesa->ffb_one_over_16_16_fixed_scale = (1.0 / __FFB_16_16_FIXED_SCALE);
	fmesa->ffb_ubyte_color_scale          = 255.0f;
	fmesa->ffb_zero			      = 0.0f;

	fmesa->debugFallbacks = GL_FALSE;
	debug = getenv("LIBGL_DEBUG");
	if (debug && strstr(debug, "fallbacks"))
		fmesa->debugFallbacks = GL_TRUE;

	/* Initialize the software rasterizer and helper modules. */
	_swrast_CreateContext( ctx );
	_vbo_CreateContext( ctx );
	_tnl_CreateContext( ctx );
	_swsetup_CreateContext( ctx );

	/* All of this need only be done once for a new context. */
	/* XXX these should be moved right after the
	 *  _mesa_init_driver_functions() call above.
	 */
	ffbDDExtensionsInit(ctx);
	ffbDDInitDriverFuncs(ctx);
	ffbDDInitStateFuncs(ctx);
	ffbDDInitRenderFuncs(ctx);
	/*ffbDDInitTexFuncs(ctx); not needed */
	ffbDDInitBitmapFuncs(ctx);
	ffbInitVB(ctx);

#if 0
	ffbInitTnlModule(ctx);
#endif

	_tnl_destroy_pipeline(ctx);
	_tnl_install_pipeline(ctx, ffb_pipeline);

	return GL_TRUE;
}

static void
ffbDestroyContext(__DRIcontextPrivate *driContextPriv)
{
	ffbContextPtr fmesa = (ffbContextPtr) driContextPriv->driverPrivate;

	if (fmesa) {
		ffbFreeVB(fmesa->glCtx);

		_swsetup_DestroyContext( fmesa->glCtx );
		_tnl_DestroyContext( fmesa->glCtx );
		_vbo_DestroyContext( fmesa->glCtx );
		_swrast_DestroyContext( fmesa->glCtx );

                /* free the Mesa context */
                fmesa->glCtx->DriverCtx = NULL;
                _mesa_destroy_context(fmesa->glCtx);

		FREE(fmesa);
	}
}

/* Create and initialize the Mesa and driver specific pixmap buffer data */
static GLboolean
ffbCreateBuffer(__DRIscreenPrivate *driScrnPriv,
                __DRIdrawablePrivate *driDrawPriv,
                const __GLcontextModes *mesaVis,
                GLboolean isPixmap )
{
   /* Mesa checks for pitch > 0, but ffb doesn't use pitches */
   int bogusPitch = 1;
   int bpp = 4; /* we've always got a 32bpp framebuffer */
   int offset = 0; /* always at 0 for offset */

   if (isPixmap) {
      return GL_FALSE; /* not implemented */
   } else {
      GLboolean swStencil = (mesaVis->stencilBits > 0 && 
			     mesaVis->depthBits != 24);
      struct gl_framebuffer *fb = _mesa_create_framebuffer(mesaVis);

      {
         driRenderbuffer *frontRb
            = driNewRenderbuffer(GL_RGBA, NULL, bpp, offset, bogusPitch,
                                 driDrawPriv);
         ffbSetSpanFunctions(frontRb, mesaVis);
         _mesa_add_renderbuffer(fb, BUFFER_FRONT_LEFT, &frontRb->Base);
      }

      if (mesaVis->doubleBufferMode) {
         driRenderbuffer *backRb
            = driNewRenderbuffer(GL_RGBA, NULL, bpp, offset, bogusPitch,
                                 driDrawPriv);
         ffbSetSpanFunctions(backRb, mesaVis);
         _mesa_add_renderbuffer(fb, BUFFER_BACK_LEFT, &backRb->Base);
      }

      if (mesaVis->depthBits == 16) {
         driRenderbuffer *depthRb
            = driNewRenderbuffer(GL_DEPTH_COMPONENT16, NULL, bpp, offset,
                                 bogusPitch, driDrawPriv);
         ffbSetDepthFunctions(depthRb, mesaVis);
         _mesa_add_renderbuffer(fb, BUFFER_DEPTH, &depthRb->Base);
      }

      if (mesaVis->stencilBits > 0 && !swStencil) {
         driRenderbuffer *stencilRb
            = driNewRenderbuffer(GL_STENCIL_INDEX8_EXT, NULL, bpp, offset,
                                 bogusPitch, driDrawPriv);
         ffbSetStencilFunctions(stencilRb, mesaVis);
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
ffbDestroyBuffer(__DRIdrawablePrivate *driDrawPriv)
{
   _mesa_unreference_framebuffer((GLframebuffer **)(&(driDrawPriv->driverPrivate)));
}


#define USE_FAST_SWAP

static void
ffbSwapBuffers( __DRIdrawablePrivate *dPriv )
{
	ffbContextPtr fmesa = (ffbContextPtr) dPriv->driContextPriv->driverPrivate;
	unsigned int fbc, wid, wid_reg_val, dac_db_bit;
	unsigned int shadow_dac_addr, active_dac_addr;
	ffb_fbcPtr ffb;
	ffb_dacPtr dac;

	if (fmesa == NULL ||
	    fmesa->glCtx->Visual.doubleBufferMode == 0)
		return;

	/* Flush pending rendering commands */
	_mesa_notifySwapBuffers(fmesa->glCtx);

	ffb = fmesa->regs;
	dac = fmesa->ffbScreen->dac;

	fbc = fmesa->fbc;
	wid = fmesa->wid;

	/* Swap the buffer we render into and read pixels from. */
	fmesa->back_buffer ^= 1;

	/* If we are writing into both buffers, don't mess with
	 * the WB setting.
	 */
	if ((fbc & FFB_FBC_WB_AB) != FFB_FBC_WB_AB) {
		if ((fbc & FFB_FBC_WB_A) != 0)
			fbc = (fbc & ~FFB_FBC_WB_A) | FFB_FBC_WB_B;
		else
			fbc = (fbc & ~FFB_FBC_WB_B) | FFB_FBC_WB_A;
	}

	/* But either way, we must flip the read buffer setting. */
	if ((fbc & FFB_FBC_RB_A) != 0)
		fbc = (fbc & ~FFB_FBC_RB_A) | FFB_FBC_RB_B;
	else
		fbc = (fbc & ~FFB_FBC_RB_B) | FFB_FBC_RB_A;

	LOCK_HARDWARE(fmesa);

	if (fmesa->fbc != fbc) {
		FFBFifo(fmesa, 1);
		ffb->fbc = fmesa->fbc = fbc;
		fmesa->ffbScreen->rp_active = 1;
	}

	/* And swap the buffer displayed in the WID. */
	if (fmesa->ffb_sarea->flags & FFB_DRI_PAC1) {
		shadow_dac_addr = FFBDAC_PAC1_SPWLUT(wid);
		active_dac_addr = FFBDAC_PAC1_APWLUT(wid);
		dac_db_bit = FFBDAC_PAC1_WLUT_DB;
	} else {
		shadow_dac_addr = FFBDAC_PAC2_SPWLUT(wid);
		active_dac_addr = FFBDAC_PAC2_APWLUT(wid);
		dac_db_bit = FFBDAC_PAC2_WLUT_DB;
	}

	FFBWait(fmesa, ffb);

	wid_reg_val = DACCFG_READ(dac, active_dac_addr);
	if (fmesa->back_buffer == 0)
		wid_reg_val |=  dac_db_bit;
	else
		wid_reg_val &= ~dac_db_bit;
#ifdef USE_FAST_SWAP
	DACCFG_WRITE(dac, active_dac_addr, wid_reg_val);
#else
	DACCFG_WRITE(dac, shadow_dac_addr, wid_reg_val);

	/* Schedule the window transfer. */
	DACCFG_WRITE(dac, FFBDAC_CFG_WTCTRL, 
		     (FFBDAC_CFG_WTCTRL_TCMD | FFBDAC_CFG_WTCTRL_TE));

	{
		int limit = 1000000;
		while (limit--) {
			unsigned int wtctrl = DACCFG_READ(dac, FFBDAC_CFG_WTCTRL);

			if ((wtctrl & FFBDAC_CFG_WTCTRL_DS) == 0)
				break;
		}
	}
#endif

	UNLOCK_HARDWARE(fmesa);
}

static void ffb_init_wid(ffbContextPtr fmesa, unsigned int wid)
{
	ffb_dacPtr dac = fmesa->ffbScreen->dac;
	unsigned int wid_reg_val, dac_db_bit, active_dac_addr;
	unsigned int shadow_dac_addr;

	if (fmesa->ffb_sarea->flags & FFB_DRI_PAC1) {
		shadow_dac_addr = FFBDAC_PAC1_SPWLUT(wid);
		active_dac_addr = FFBDAC_PAC1_APWLUT(wid);
		dac_db_bit = FFBDAC_PAC1_WLUT_DB;
	} else {
		shadow_dac_addr = FFBDAC_PAC2_SPWLUT(wid);
		active_dac_addr = FFBDAC_PAC2_APWLUT(wid);
		dac_db_bit = FFBDAC_PAC2_WLUT_DB;
	}

	wid_reg_val = DACCFG_READ(dac, active_dac_addr);
	wid_reg_val &= ~dac_db_bit;
#ifdef USE_FAST_SWAP
	DACCFG_WRITE(dac, active_dac_addr, wid_reg_val);
#else
	DACCFG_WRITE(dac, shadow_dac_addr, wid_reg_val);

	/* Schedule the window transfer. */
	DACCFG_WRITE(dac, FFBDAC_CFG_WTCTRL, 
		     (FFBDAC_CFG_WTCTRL_TCMD | FFBDAC_CFG_WTCTRL_TE));

	{
		int limit = 1000000;
		while (limit--) {
			unsigned int wtctrl = DACCFG_READ(dac, FFBDAC_CFG_WTCTRL);

			if ((wtctrl & FFBDAC_CFG_WTCTRL_DS) == 0)
				break;
		}
	}
#endif
}

/* Force the context `c' to be the current context and associate with it
   buffer `b' */
static GLboolean
ffbMakeCurrent(__DRIcontextPrivate *driContextPriv,
               __DRIdrawablePrivate *driDrawPriv,
               __DRIdrawablePrivate *driReadPriv)
{
	if (driContextPriv) {
		ffbContextPtr fmesa = (ffbContextPtr) driContextPriv->driverPrivate;
		int first_time;

		fmesa->driDrawable = driDrawPriv;

		_mesa_make_current(fmesa->glCtx, 
			    (GLframebuffer *) driDrawPriv->driverPrivate, 
			    (GLframebuffer *) driReadPriv->driverPrivate);

		first_time = 0;
		if (fmesa->wid == ~0) {
			first_time = 1;
			if (getenv("LIBGL_SOFTWARE_RENDERING"))
				FALLBACK( fmesa->glCtx, FFB_BADATTR_SWONLY, GL_TRUE );
		}

		LOCK_HARDWARE(fmesa);
		if (first_time) {
			fmesa->wid = fmesa->ffb_sarea->wid_table[driDrawPriv->index];
			ffb_init_wid(fmesa, fmesa->wid);
		}

		fmesa->state_dirty |= FFB_STATE_ALL;
		fmesa->state_fifo_ents = fmesa->state_all_fifo_ents;
		ffbSyncHardware(fmesa);
		UNLOCK_HARDWARE(fmesa);

		if (first_time) {
			/* Also, at the first switch to a new context,
			 * we need to clear all the hw buffers.
			 */
			ffbDDClear(fmesa->glCtx,
				   (BUFFER_BIT_FRONT_LEFT | BUFFER_BIT_BACK_LEFT |
				    BUFFER_BIT_DEPTH | BUFFER_BIT_STENCIL));
		}
	} else {
		_mesa_make_current(NULL, NULL, NULL);
	}

	return GL_TRUE;
}

/* Force the context `c' to be unbound from its buffer */
static GLboolean
ffbUnbindContext(__DRIcontextPrivate *driContextPriv)
{
	return GL_TRUE;
}

void ffbXMesaUpdateState(ffbContextPtr fmesa)
{
	__DRIdrawablePrivate *dPriv = fmesa->driDrawable;
	__DRIscreenPrivate *sPriv = fmesa->driScreen;
	int stamp = dPriv->lastStamp;

	DRI_VALIDATE_DRAWABLE_INFO(sPriv, dPriv);

	if (dPriv->lastStamp != stamp) {
		GLcontext *ctx = fmesa->glCtx;

		ffbCalcViewport(ctx);
		driUpdateFramebufferSize(ctx, dPriv);
		if (ctx->Polygon.StippleFlag) {
			ffbXformAreaPattern(fmesa,
					    (const GLubyte *)ctx->PolygonStipple);
		}
	}
}

static const struct __DriverAPIRec ffbAPI = {
   .InitDriver      = ffbInitDriver,
   .DestroyScreen   = ffbDestroyScreen,
   .CreateContext   = ffbCreateContext,
   .DestroyContext  = ffbDestroyContext,
   .CreateBuffer    = ffbCreateBuffer,
   .DestroyBuffer   = ffbDestroyBuffer,
   .SwapBuffers     = ffbSwapBuffers,
   .MakeCurrent     = ffbMakeCurrent,
   .UnbindContext   = ffbUnbindContext,
   .GetSwapInfo     = NULL,
   .GetMSC          = NULL,
   .WaitForMSC      = NULL,
   .WaitForSBC      = NULL,
   .SwapBuffersMSC  = NULL
};


static __GLcontextModes *
ffbFillInModes( unsigned pixel_bits, unsigned depth_bits,
		 unsigned stencil_bits, GLboolean have_back_buffer )
{
   __GLcontextModes * modes;
   __GLcontextModes * m;
   unsigned num_modes;
   unsigned depth_buffer_factor;
   unsigned back_buffer_factor;
   GLenum fb_format;
   GLenum fb_type;

   /* GLX_SWAP_COPY_OML is only supported because the FFB driver doesn't
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
   back_buffer_factor  = (have_back_buffer) ? 3 : 1;

   num_modes = depth_buffer_factor * back_buffer_factor * 4;

    if ( pixel_bits == 16 ) {
        fb_format = GL_RGB;
        fb_type = GL_UNSIGNED_SHORT_5_6_5;
    }
    else {
        fb_format = GL_BGRA;
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
   static const __DRIversion ddx_expected = { 0, 1, 1 };
   static const __DRIversion dri_expected = { 4, 0, 0 };
   static const __DRIversion drm_expected = { 0, 0, 1 };

   dri_interface = interface;

   if ( ! driCheckDriDdxDrmVersions2( "ffb",
				      dri_version, & dri_expected,
				      ddx_version, & ddx_expected,
				      drm_version, & drm_expected ) ) {
      return NULL;
   }

   psp = __driUtilCreateNewScreen(dpy, scrn, psc, NULL,
				  ddx_version, dri_version, drm_version,
				  frame_buffer, pSAREA, fd,
				  internal_api_version, &ffbAPI);
   if ( psp != NULL ) {
      *driver_modes = ffbFillInModes( 32, 16, 0, GL_TRUE );
   }

   return (void *) psp;
}
