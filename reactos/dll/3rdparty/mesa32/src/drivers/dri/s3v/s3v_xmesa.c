/*
 * Author: Max Lingua <sunmax@libero.it>
 */

#include "s3v_context.h"
#include "s3v_vb.h"
#include "context.h"
#include "matrix.h"
#include "s3v_dri.h"
#include "framebuffer.h"
#include "renderbuffer.h"

#include "swrast/swrast.h"
#include "swrast_setup/swrast_setup.h"
#include "tnl/tnl.h"
#include "vbo/vbo.h"

/* #define DEBUG(str) printf str */

static GLboolean 
s3vInitDriver(__DRIscreenPrivate *sPriv)
{
    sPriv->private = (void *) s3vCreateScreen( sPriv );

    if (!sPriv->private) {
	s3vDestroyScreen( sPriv );
	return GL_FALSE;
    }

    return GL_TRUE;
}

static void 
s3vDestroyContext(__DRIcontextPrivate *driContextPriv)
{
    s3vContextPtr vmesa = (s3vContextPtr)driContextPriv->driverPrivate;

    if (vmesa) {
      _swsetup_DestroyContext( vmesa->glCtx );
      _tnl_DestroyContext( vmesa->glCtx );
      _vbo_DestroyContext( vmesa->glCtx );
      _swrast_DestroyContext( vmesa->glCtx );

      s3vFreeVB( vmesa->glCtx );

      /* free the Mesa context */
      vmesa->glCtx->DriverCtx = NULL;
      _mesa_destroy_context(vmesa->glCtx);

      _mesa_free(vmesa);
      driContextPriv->driverPrivate = NULL;
    }
}


static GLboolean
s3vCreateBuffer( __DRIscreenPrivate *driScrnPriv,
                   __DRIdrawablePrivate *driDrawPriv,
                   const __GLcontextModes *mesaVis,
                   GLboolean isPixmap )
{
   s3vScreenPtr screen = (s3vScreenPtr) driScrnPriv->private;

   if (isPixmap) {
      return GL_FALSE; /* not implemented */
   }
   else {
      struct gl_framebuffer *fb = _mesa_create_framebuffer(mesaVis);

      {
         driRenderbuffer *frontRb
            = driNewRenderbuffer(GL_RGBA, NULL, screen->cpp,
                                 screen->frontOffset, screen->frontPitch,
                                 driDrawPriv);
         s3vSetSpanFunctions(frontRb, mesaVis);
         _mesa_add_renderbuffer(fb, BUFFER_FRONT_LEFT, &frontRb->Base);
      }

      if (mesaVis->doubleBufferMode) {
         driRenderbuffer *backRb
            = driNewRenderbuffer(GL_RGBA, NULL, screen->cpp,
                                 screen->backOffset, screen->backPitch,
                                 driDrawPriv);
         s3vSetSpanFunctions(backRb, mesaVis);
         _mesa_add_renderbuffer(fb, BUFFER_BACK_LEFT, &backRb->Base);
         backRb->backBuffer = GL_TRUE;
      }

      if (mesaVis->depthBits == 16) {
         driRenderbuffer *depthRb
            = driNewRenderbuffer(GL_DEPTH_COMPONENT16, NULL, screen->cpp,
                                 screen->depthOffset, screen->depthPitch,
                                 driDrawPriv);
         s3vSetSpanFunctions(depthRb, mesaVis);
         _mesa_add_renderbuffer(fb, BUFFER_DEPTH, &depthRb->Base);
      }
      else if (mesaVis->depthBits == 24) {
         driRenderbuffer *depthRb
            = driNewRenderbuffer(GL_DEPTH_COMPONENT24, NULL, screen->cpp,
                                 screen->depthOffset, screen->depthPitch,
                                 driDrawPriv);
         s3vSetSpanFunctions(depthRb, mesaVis);
         _mesa_add_renderbuffer(fb, BUFFER_DEPTH, &depthRb->Base);
      }

      /* no h/w stencil yet?
      if (mesaVis->stencilBits > 0) {
         driRenderbuffer *stencilRb
            = driNewRenderbuffer(GL_STENCIL_INDEX8_EXT, NULL,
                                 screen->cpp, screen->depthOffset,
                                 screen->depthPitch, driDrawPriv);
         s3vSetSpanFunctions(stencilRb, mesaVis);
         _mesa_add_renderbuffer(fb, BUFFER_STENCIL, &stencilRb->Base);
      }
      */

      _mesa_add_soft_renderbuffers(fb,
                                   GL_FALSE, /* color */
                                   GL_FALSE, /* depth */
                                   mesaVis->stencilBits > 0,
                                   mesaVis->accumRedBits > 0,
                                   GL_FALSE, /* alpha */
                                   GL_FALSE /* aux */);
      driDrawPriv->driverPrivate = (void *) fb;

      return (driDrawPriv->driverPrivate != NULL);
   }
}


static void
s3vDestroyBuffer(__DRIdrawablePrivate *driDrawPriv)
{
   _mesa_unreference_framebuffer((GLframebuffer **)(&(driDrawPriv->driverPrivate)));
}

static void
s3vSwapBuffers(__DRIdrawablePrivate *drawablePrivate)
{
   __DRIdrawablePrivate *dPriv = (__DRIdrawablePrivate *) drawablePrivate;
   __DRIscreenPrivate *sPriv;
   GLcontext *ctx;
   s3vContextPtr vmesa;
   s3vScreenPtr s3vscrn;
   
   vmesa = (s3vContextPtr) dPriv->driContextPriv->driverPrivate;
   sPriv = vmesa->driScreen;
   s3vscrn = vmesa->s3vScreen;
   ctx = vmesa->glCtx;

   DEBUG(("*** s3vSwapBuffers ***\n"));

/* DMAFLUSH(); */

   _mesa_notifySwapBuffers( ctx );

   vmesa = (s3vContextPtr) dPriv->driContextPriv->driverPrivate;
/*    driScrnPriv = vmesa->driScreen; */

/*    if (vmesa->EnabledFlags & S3V_BACK_BUFFER) */

/*	_mesa_notifySwapBuffers( ctx );  */
#if 1
{	
	int x0, y0, x1, y1;
/*	
	int nRect = dPriv->numClipRects;
	XF86DRIClipRectPtr pRect = dPriv->pClipRects;

	__DRIscreenPrivate *driScrnPriv = vmesa->driScreen;
*/

/*	
	DEBUG(("s3vSwapBuffers: S3V_BACK_BUFFER = 1 - nClip = %i\n", nRect));
*/
/*	vmesa->drawOffset=vmesa->s3vScreen->backOffset; */

	x0 = dPriv->x;
	y0 = dPriv->y;

	x1 = x0 + dPriv->w - 1;
	y1 = y0 + dPriv->h - 1;

	DMAOUT_CHECK(BITBLT_SRC_BASE, 15);
		DMAOUT(vmesa->s3vScreen->backOffset);
		DMAOUT(0); /* 0xc0000000 */
		DMAOUT( ((x0 << 16) | x1) );
		DMAOUT( ((y0 << 16) | y1) );
		DMAOUT( (vmesa->DestStride << 16) | vmesa->SrcStride );
		DMAOUT( (~(0)) );
		DMAOUT( (~(0)) );
		DMAOUT(0);
		DMAOUT(0);
       /* FIXME */
		DMAOUT(0);
		DMAOUT(0);
		DMAOUT( (0x01 | /* Autoexecute */
			 0x02 | /* clip */
			 0x04 | /* 16 bit */
			 0x20 | /* draw */
			0x400 | /* word alignment (bit 10=1) */
			(0x2 << 11) | /*  offset = 1 byte */
			(0xCC << 17) |	/* rop #204 */
			(0x3 << 25)) ); /* l-r, t-b */
		DMAOUT(vmesa->ScissorWH);
		DMAOUT( /* 0 */ vmesa->SrcXY );
		DMAOUT( (dPriv->x << 16) | dPriv->y );
	DMAFINISH();

	DMAFLUSH();

	vmesa->restore_primitive = -1;

}
#endif
}

static GLboolean 
s3vMakeCurrent(__DRIcontextPrivate *driContextPriv,
		 __DRIdrawablePrivate *driDrawPriv,
		 __DRIdrawablePrivate *driReadPriv)
{
	int x1,x2,y1,y2;
	int cx, cy, cw, ch;
	unsigned int src_stride, dest_stride;
	int cl;

	s3vContextPtr vmesa;
	__DRIdrawablePrivate *dPriv = driDrawPriv;
	vmesa = (s3vContextPtr) dPriv->driContextPriv->driverPrivate;
	
	DEBUG(("s3vMakeCurrent\n"));

	DEBUG(("dPriv->x=%i y=%i w=%i h=%i\n", dPriv->x, dPriv->y,
		dPriv->w, dPriv->h));

	if (driContextPriv) {
	GET_CURRENT_CONTEXT(ctx);
	s3vContextPtr oldVirgeCtx = ctx ? S3V_CONTEXT(ctx) : NULL;
	s3vContextPtr newVirgeCtx = (s3vContextPtr) driContextPriv->driverPrivate;

	if ( newVirgeCtx != oldVirgeCtx ) {

		newVirgeCtx->dirty = ~0;
		cl = 1;
		DEBUG(("newVirgeCtx != oldVirgeCtx\n"));
/*		s3vUpdateClipping(newVirgeCtx->glCtx ); */
	}

	if (newVirgeCtx->driDrawable != driDrawPriv) {
	    newVirgeCtx->driDrawable = driDrawPriv;
		DEBUG(("driDrawable != driDrawPriv\n"));
		s3vUpdateWindow ( newVirgeCtx->glCtx );
		s3vUpdateViewportOffset( newVirgeCtx->glCtx );
/*		s3vUpdateClipping(newVirgeCtx->glCtx ); */
	}
/*
	s3vUpdateWindow ( newVirgeCtx->glCtx );
	s3vUpdateViewportOffset( newVirgeCtx->glCtx );
*/

/*
	_mesa_make_current( newVirgeCtx->glCtx,
                          (GLframebuffer *) driDrawPriv->driverPrivate,
                          (GLframebuffer *) driReadPriv->driverPrivate );

	_mesa_set_viewport(newVirgeCtx->glCtx, 0, 0,
	                  newVirgeCtx->driDrawable->w,
			  newVirgeCtx->driDrawable->h);
*/

#if 0
	newVirgeCtx->Window &= ~W_GIDMask;
	newVirgeCtx->Window |= (driDrawPriv->index << 5);
	CHECK_DMA_BUFFER(newVirgeCtx,1);
	WRITE(newVirgeCtx->buf, S3VWindow, newVirgeCtx->Window);
#endif

	newVirgeCtx->new_state |= S3V_NEW_WINDOW; /* FIXME */

	_mesa_make_current( newVirgeCtx->glCtx, 
                            (GLframebuffer *) driDrawPriv->driverPrivate,
                            (GLframebuffer *) driReadPriv->driverPrivate );

	if (!newVirgeCtx->glCtx->Viewport.Width) {
	    _mesa_set_viewport(newVirgeCtx->glCtx, 0, 0, 
					driDrawPriv->w, driDrawPriv->h);

/*		s3vUpdateClipping(newVirgeCtx->glCtx ); */
	}

/*
	if (cl) {
		s3vUpdateClipping(newVirgeCtx->glCtx );
		cl =0;
	}
*/

	newVirgeCtx->new_state |= S3V_NEW_CLIP;

        if (1) {
           cx = dPriv->x;
           cw = dPriv->w;
           cy = dPriv->y;
           ch = dPriv->h;
        }
        
        x1 = y1 = 0;
        x2 = cw-1;
        y2 = ch-1;

        /*  src_stride = vmesa->s3vScreen->w * vmesa->s3vScreen->cpp; 
            dest_stride = ((x2+31)&~31) * vmesa->s3vScreen->cpp; */
        src_stride = vmesa->driScreen->fbWidth * 2;
        dest_stride = ((x2+31)&~31) * 2;
    } else {
       _mesa_make_current( NULL, NULL, NULL );
    }

    return GL_TRUE;
}


static GLboolean 
s3vUnbindContext( __DRIcontextPrivate *driContextPriv )
{
   return GL_TRUE;
}


static struct __DriverAPIRec s3vAPI = {
   s3vInitDriver,
   s3vDestroyScreen,
   s3vCreateContext,
   s3vDestroyContext,
   s3vCreateBuffer,
   s3vDestroyBuffer,
   s3vSwapBuffers,
   s3vMakeCurrent,
   s3vUnbindContext,
};


#if 0
/*
 * This is the bootstrap function for the driver.
 * The __driCreateScreen name is the symbol that libGL.so fetches.
 * Return:  pointer to a __DRIscreenPrivate.
 */
void *__driCreateScreen(Display *dpy, int scrn, __DRIscreen *psc,
                        int numConfigs, __GLXvisualConfig *config)
{
   __DRIscreenPrivate *psp=NULL;

   DEBUG(("__driCreateScreen: psp = %p\n", psp));
   psp = __driUtilCreateScreen(dpy, scrn, psc, numConfigs, config, &s3vAPI);
   DEBUG(("__driCreateScreen: psp = %p\n", psp));
   return (void *) psp;
}
#endif
