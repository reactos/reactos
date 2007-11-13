/*
 * Mesa 3-D graphics library
 * Version:  6.3
 *
 * Copyright (C) 1999-2005  Brian Paul   All Rights Reserved.
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
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/* Minimal swrast-based dri loadable driver.
 *
 * Todo:
 *   -- Use malloced (rather than framebuffer) memory for backbuffer
 *   -- 32bpp is hardwared -- fix
 *
 * NOTES:
 *   -- No mechanism for cliprects or resize notification --
 *      assumes this is a fullscreen device.  
 *   -- No locking -- assumes this is the only driver accessing this 
 *      device.
 *   -- Doesn't (yet) make use of any acceleration or other interfaces
 *      provided by fb.  Would be entirely happy working against any 
 *	fullscreen interface.
 *   -- HOWEVER: only a small number of pixelformats are supported, and
 *      the mechanism for choosing between them makes some assumptions
 *      that may not be valid everywhere.
 */

#include "driver.h"
#include "drm.h"
#include "utils.h"
#include "drirenderbuffer.h"

#include "buffers.h"
#include "extensions.h"
#include "framebuffer.h"
#include "renderbuffer.h"
#include "vbo/vbo.h"
#include "swrast/swrast.h"
#include "swrast_setup/swrast_setup.h"
#include "tnl/tnl.h"
#include "tnl/t_context.h"
#include "tnl/t_pipeline.h"
#include "drivers/common/driverfuncs.h"

void fbSetSpanFunctions(driRenderbuffer *drb, const GLvisual *vis);

typedef struct {
   GLcontext *glCtx;		/* Mesa context */

   struct {
      __DRIcontextPrivate *context;	
      __DRIscreenPrivate *screen;	
      __DRIdrawablePrivate *drawable; /* drawable bound to this ctx */
   } dri;
   
} fbContext, *fbContextPtr;

#define FB_CONTEXT(ctx)		((fbContextPtr)(ctx->DriverCtx))


static const GLubyte *
get_string(GLcontext *ctx, GLenum pname)
{
   (void) ctx;
   switch (pname) {
      case GL_RENDERER:
         return (const GLubyte *) "Mesa dumb framebuffer";
      default:
         return NULL;
   }
}


static void
update_state( GLcontext *ctx, GLuint new_state )
{
   /* not much to do here - pass it on */
   _swrast_InvalidateState( ctx, new_state );
   _swsetup_InvalidateState( ctx, new_state );
   _vbo_InvalidateState( ctx, new_state );
   _tnl_InvalidateState( ctx, new_state );
}


/**
 * Called by ctx->Driver.GetBufferSize from in core Mesa to query the
 * current framebuffer size.
 */
static void
get_buffer_size( GLframebuffer *buffer, GLuint *width, GLuint *height )
{
   GET_CURRENT_CONTEXT(ctx);
   fbContextPtr fbmesa = FB_CONTEXT(ctx);

   *width  = fbmesa->dri.drawable->w;
   *height = fbmesa->dri.drawable->h;
}


static void
updateFramebufferSize(GLcontext *ctx)
{
   fbContextPtr fbmesa = FB_CONTEXT(ctx);
   struct gl_framebuffer *fb = ctx->WinSysDrawBuffer;
   if (fbmesa->dri.drawable->w != fb->Width ||
       fbmesa->dri.drawable->h != fb->Height) {
      driUpdateFramebufferSize(ctx, fbmesa->dri.drawable);
   }
}

static void
viewport(GLcontext *ctx, GLint x, GLint y, GLsizei w, GLsizei h)
{
   /* XXX this should be called after we acquire the DRI lock, not here */
   updateFramebufferSize(ctx);
}


static void
init_core_functions( struct dd_function_table *functions )
{
   functions->GetString = get_string;
   functions->UpdateState = update_state;
   functions->GetBufferSize = get_buffer_size;
   functions->Viewport = viewport;

   functions->Clear = _swrast_Clear;  /* could accelerate with blits */
}


/*
 * Generate code for span functions.
 */

/* 24-bit BGR */
#define NAME(PREFIX) PREFIX##_B8G8R8
#define FORMAT GL_RGBA8
#define SPAN_VARS \
   driRenderbuffer *drb = (driRenderbuffer *) rb;
#define INIT_PIXEL_PTR(P, X, Y) \
   GLubyte *P = (GLubyte *)drb->Base.Data + (drb->Base.Height - (Y)) * drb->pitch + (X) * 3;
#define INC_PIXEL_PTR(P) P += 3
#define STORE_PIXEL(DST, X, Y, VALUE) \
   DST[0] = VALUE[BCOMP]; \
   DST[1] = VALUE[GCOMP]; \
   DST[2] = VALUE[RCOMP]
#define FETCH_PIXEL(DST, SRC) \
   DST[RCOMP] = SRC[2]; \
   DST[GCOMP] = SRC[1]; \
   DST[BCOMP] = SRC[0]; \
   DST[ACOMP] = 0xff

#include "swrast/s_spantemp.h"


/* 32-bit BGRA */
#define NAME(PREFIX) PREFIX##_B8G8R8A8
#define FORMAT GL_RGBA8
#define SPAN_VARS \
   driRenderbuffer *drb = (driRenderbuffer *) rb;
#define INIT_PIXEL_PTR(P, X, Y) \
   GLubyte *P = (GLubyte *)drb->Base.Data + (drb->Base.Height - (Y)) * drb->pitch + (X) * 4;
#define INC_PIXEL_PTR(P) P += 4
#define STORE_PIXEL(DST, X, Y, VALUE) \
   DST[0] = VALUE[BCOMP]; \
   DST[1] = VALUE[GCOMP]; \
   DST[2] = VALUE[RCOMP]; \
   DST[3] = VALUE[ACOMP]
#define STORE_PIXEL_RGB(DST, X, Y, VALUE) \
   DST[0] = VALUE[BCOMP]; \
   DST[1] = VALUE[GCOMP]; \
   DST[2] = VALUE[RCOMP]; \
   DST[3] = 0xff
#define FETCH_PIXEL(DST, SRC) \
   DST[RCOMP] = SRC[2]; \
   DST[GCOMP] = SRC[1]; \
   DST[BCOMP] = SRC[0]; \
   DST[ACOMP] = SRC[3]

#include "swrast/s_spantemp.h"


/* 16-bit BGR (XXX implement dithering someday) */
#define NAME(PREFIX) PREFIX##_B5G6R5
#define FORMAT GL_RGBA8
#define SPAN_VARS \
   driRenderbuffer *drb = (driRenderbuffer *) rb;
#define INIT_PIXEL_PTR(P, X, Y) \
   GLushort *P = (GLushort *)drb->Base.Data + (drb->Base.Height - (Y)) * drb->pitch + (X) * 2;
#define INC_PIXEL_PTR(P) P += 1
#define STORE_PIXEL(DST, X, Y, VALUE) \
   DST[0] = ( (((VALUE[RCOMP]) & 0xf8) << 8) | (((VALUE[GCOMP]) & 0xfc) << 3) | ((VALUE[BCOMP]) >> 3) )
#define FETCH_PIXEL(DST, SRC) \
   DST[RCOMP] = ( (((SRC[0]) >> 8) & 0xf8) | (((SRC[0]) >> 11) & 0x7) ); \
   DST[GCOMP] = ( (((SRC[0]) >> 3) & 0xfc) | (((SRC[0]) >>  5) & 0x3) ); \
   DST[BCOMP] = ( (((SRC[0]) << 3) & 0xf8) | (((SRC[0])      ) & 0x7) ); \
   DST[ACOMP] = 0xff

#include "swrast/s_spantemp.h"


/* 15-bit BGR (XXX implement dithering someday) */
#define NAME(PREFIX) PREFIX##_B5G5R5
#define FORMAT GL_RGBA8
#define SPAN_VARS \
   driRenderbuffer *drb = (driRenderbuffer *) rb;
#define INIT_PIXEL_PTR(P, X, Y) \
   GLushort *P = (GLushort *)drb->Base.Data + (drb->Base.Height - (Y)) * drb->pitch + (X) * 2;
#define INC_PIXEL_PTR(P) P += 1
#define STORE_PIXEL(DST, X, Y, VALUE) \
   DST[0] = ( (((VALUE[RCOMP]) & 0xf8) << 7) | (((VALUE[GCOMP]) & 0xf8) << 2) | ((VALUE[BCOMP]) >> 3) )
#define FETCH_PIXEL(DST, SRC) \
   DST[RCOMP] = ( (((SRC[0]) >> 7) & 0xf8) | (((SRC[0]) >> 10) & 0x7) ); \
   DST[GCOMP] = ( (((SRC[0]) >> 2) & 0xf8) | (((SRC[0]) >>  5) & 0x7) ); \
   DST[BCOMP] = ( (((SRC[0]) << 3) & 0xf8) | (((SRC[0])      ) & 0x7) ); \
   DST[ACOMP] = 0xff

#include "swrast/s_spantemp.h"


/* 8-bit color index */
#define NAME(PREFIX) PREFIX##_CI8
#define FORMAT GL_COLOR_INDEX8_EXT
#define SPAN_VARS \
   driRenderbuffer *drb = (driRenderbuffer *) rb;
#define INIT_PIXEL_PTR(P, X, Y) \
   GLubyte *P = (GLubyte *)drb->Base.Data + (drb->Base.Height - (Y)) * drb->pitch + (X);
#define INC_PIXEL_PTR(P) P += 1
#define STORE_PIXEL(DST, X, Y, VALUE) \
   *DST = VALUE[0]
#define FETCH_PIXEL(DST, SRC) \
   DST = SRC[0]

#include "swrast/s_spantemp.h"



void
fbSetSpanFunctions(driRenderbuffer *drb, const GLvisual *vis)
{
   ASSERT(drb->Base.InternalFormat == GL_RGBA);
   if (drb->Base.InternalFormat == GL_RGBA) {
      if (vis->redBits == 5 && vis->greenBits == 6 && vis->blueBits == 5) {
         drb->Base.GetRow = get_row_B5G6R5;
         drb->Base.GetValues = get_values_B5G6R5;
         drb->Base.PutRow = put_row_B5G6R5;
         drb->Base.PutMonoRow = put_mono_row_B5G6R5;
         drb->Base.PutRowRGB = put_row_rgb_B5G6R5;
         drb->Base.PutValues = put_values_B5G6R5;
         drb->Base.PutMonoValues = put_mono_values_B5G6R5;
      }
      else if (vis->redBits == 5 && vis->greenBits == 5 && vis->blueBits == 5) {
         drb->Base.GetRow = get_row_B5G5R5;
         drb->Base.GetValues = get_values_B5G5R5;
         drb->Base.PutRow = put_row_B5G5R5;
         drb->Base.PutMonoRow = put_mono_row_B5G5R5;
         drb->Base.PutRowRGB = put_row_rgb_B5G5R5;
         drb->Base.PutValues = put_values_B5G5R5;
         drb->Base.PutMonoValues = put_mono_values_B5G5R5;
      }
      else if (vis->redBits == 8 && vis->greenBits == 8 && vis->blueBits == 8
               && vis->alphaBits == 8) {
         drb->Base.GetRow = get_row_B8G8R8A8;
         drb->Base.GetValues = get_values_B8G8R8A8;
         drb->Base.PutRow = put_row_B8G8R8A8;
         drb->Base.PutMonoRow = put_mono_row_B8G8R8A8;
         drb->Base.PutRowRGB = put_row_rgb_B8G8R8A8;
         drb->Base.PutValues = put_values_B8G8R8A8;
         drb->Base.PutMonoValues = put_mono_values_B8G8R8A8;
      }
      else if (vis->redBits == 8 && vis->greenBits == 8 && vis->blueBits == 8
               && vis->alphaBits == 0) {
         drb->Base.GetRow = get_row_B8G8R8;
         drb->Base.GetValues = get_values_B8G8R8;
         drb->Base.PutRow = put_row_B8G8R8;
         drb->Base.PutMonoRow = put_mono_row_B8G8R8;
         drb->Base.PutRowRGB = put_row_rgb_B8G8R8;
         drb->Base.PutValues = put_values_B8G8R8;
         drb->Base.PutMonoValues = put_mono_values_B8G8R8;
      }
      else if (vis->indexBits == 8) {
         drb->Base.GetRow = get_row_CI8;
         drb->Base.GetValues = get_values_CI8;
         drb->Base.PutRow = put_row_CI8;
         drb->Base.PutMonoRow = put_mono_row_CI8;
         drb->Base.PutValues = put_values_CI8;
         drb->Base.PutMonoValues = put_mono_values_CI8;
      }
   }
   else {
      /* hardware z/stencil/etc someday */
   }
}



/* Initialize the driver specific screen private data.
 */
static GLboolean
fbInitDriver( __DRIscreenPrivate *sPriv )
{
   sPriv->private = NULL;
   return GL_TRUE;
}

static void
fbDestroyScreen( __DRIscreenPrivate *sPriv )
{
}


/* Create the device specific context.
 */
static GLboolean
fbCreateContext( const __GLcontextModes *glVisual,
		 __DRIcontextPrivate *driContextPriv,
		 void *sharedContextPrivate)
{
   fbContextPtr fbmesa;
   GLcontext *ctx, *shareCtx;
   struct dd_function_table functions;

   assert(glVisual);
   assert(driContextPriv);

   /* Allocate the Fb context */
   fbmesa = (fbContextPtr) _mesa_calloc( sizeof(*fbmesa) );
   if ( !fbmesa )
      return GL_FALSE;

   /* Init default driver functions then plug in our FBdev-specific functions
    */
   _mesa_init_driver_functions(&functions);
   init_core_functions(&functions);

   /* Allocate the Mesa context */
   if (sharedContextPrivate)
      shareCtx = ((fbContextPtr) sharedContextPrivate)->glCtx;
   else
      shareCtx = NULL;

   ctx = fbmesa->glCtx = _mesa_create_context(glVisual, shareCtx, 
					      &functions, (void *) fbmesa);
   if (!fbmesa->glCtx) {
      _mesa_free(fbmesa);
      return GL_FALSE;
   }
   driContextPriv->driverPrivate = fbmesa;

   /* Create module contexts */
   _swrast_CreateContext( ctx );
   _vbo_CreateContext( ctx );
   _tnl_CreateContext( ctx );
   _swsetup_CreateContext( ctx );
   _swsetup_Wakeup( ctx );


   /* use default TCL pipeline */
   {
      TNLcontext *tnl = TNL_CONTEXT(ctx);
      tnl->Driver.RunPipeline = _tnl_run_pipeline;
   }

   _mesa_enable_sw_extensions(ctx);

   return GL_TRUE;
}


static void
fbDestroyContext( __DRIcontextPrivate *driContextPriv )
{
   GET_CURRENT_CONTEXT(ctx);
   fbContextPtr fbmesa = (fbContextPtr) driContextPriv->driverPrivate;
   fbContextPtr current = ctx ? FB_CONTEXT(ctx) : NULL;

   /* check if we're deleting the currently bound context */
   if (fbmesa == current) {
      _mesa_make_current(NULL, NULL, NULL);
   }

   /* Free fb context resources */
   if ( fbmesa ) {
      _swsetup_DestroyContext( fbmesa->glCtx );
      _tnl_DestroyContext( fbmesa->glCtx );
      _vbo_DestroyContext( fbmesa->glCtx );
      _swrast_DestroyContext( fbmesa->glCtx );

      /* free the Mesa context */
      fbmesa->glCtx->DriverCtx = NULL;
      _mesa_destroy_context( fbmesa->glCtx );

      _mesa_free( fbmesa );
   }
}


/* Create and initialize the Mesa and driver specific pixmap buffer
 * data.
 */
static GLboolean
fbCreateBuffer( __DRIscreenPrivate *driScrnPriv,
		__DRIdrawablePrivate *driDrawPriv,
		const __GLcontextModes *mesaVis,
		GLboolean isPixmap )
{
   struct gl_framebuffer *mesa_framebuffer;
   
   if (isPixmap) {
      return GL_FALSE; /* not implemented */
   }
   else {
      const GLboolean swDepth = mesaVis->depthBits > 0;
      const GLboolean swAlpha = mesaVis->alphaBits > 0;
      const GLboolean swAccum = mesaVis->accumRedBits > 0;
      const GLboolean swStencil = mesaVis->stencilBits > 0;
      
      mesa_framebuffer = _mesa_create_framebuffer(mesaVis);
      if (!mesa_framebuffer)
         return 0;

      /* XXX double-check these parameters (bpp vs cpp, etc) */
      {
         driRenderbuffer *drb = driNewRenderbuffer(GL_RGBA,
                                                   driScrnPriv->pFB,
                                                   driScrnPriv->fbBPP / 8,
                                                   driScrnPriv->fbOrigin,
                                                   driScrnPriv->fbStride,
                                                   driDrawPriv);
         fbSetSpanFunctions(drb, mesaVis);
         _mesa_add_renderbuffer(mesa_framebuffer,
                                BUFFER_FRONT_LEFT, &drb->Base);
      }
      if (mesaVis->doubleBufferMode) {
         /* XXX what are the correct origin/stride values? */
         GLvoid *backBuf = _mesa_malloc(driScrnPriv->fbStride
                                        * driScrnPriv->fbHeight);
         driRenderbuffer *drb = driNewRenderbuffer(GL_RGBA,
                                                   backBuf,
                                                   driScrnPriv->fbBPP /8,
                                                   driScrnPriv->fbOrigin,
                                                   driScrnPriv->fbStride,
                                                   driDrawPriv);
         fbSetSpanFunctions(drb, mesaVis);
         _mesa_add_renderbuffer(mesa_framebuffer,
                                BUFFER_BACK_LEFT, &drb->Base);
      }

      _mesa_add_soft_renderbuffers(mesa_framebuffer,
                                   GL_FALSE, /* color */
                                   swDepth,
                                   swStencil,
                                   swAccum,
                                   swAlpha, /* or always zero? */
                                   GL_FALSE /* aux */);
      
      driDrawPriv->driverPrivate = mesa_framebuffer;

      return 1;
   }
}


static void
fbDestroyBuffer(__DRIdrawablePrivate *driDrawPriv)
{
   _mesa_unreference_framebuffer((GLframebuffer **)(&(driDrawPriv->driverPrivate)));
}



/* If the backbuffer is on a videocard, this is extraordinarily slow!
 */
static void
fbSwapBuffers( __DRIdrawablePrivate *dPriv )
{
   struct gl_framebuffer *mesa_framebuffer = (struct gl_framebuffer *)dPriv->driverPrivate;
   struct gl_renderbuffer * front_renderbuffer = mesa_framebuffer->Attachment[BUFFER_FRONT_LEFT].Renderbuffer;
   void *frontBuffer = front_renderbuffer->Data;
   int currentPitch = ((driRenderbuffer *)front_renderbuffer)->pitch;
   void *backBuffer = mesa_framebuffer->Attachment[BUFFER_BACK_LEFT].Renderbuffer->Data;

   if (dPriv->driContextPriv && dPriv->driContextPriv->driverPrivate) {
      fbContextPtr fbmesa = (fbContextPtr) dPriv->driContextPriv->driverPrivate;
      GLcontext *ctx = fbmesa->glCtx;
      
      if (ctx->Visual.doubleBufferMode) {
	 int i;
	 int offset = 0;
         char *tmp = _mesa_malloc(currentPitch);

         _mesa_notifySwapBuffers( ctx );  /* flush pending rendering comands */

         ASSERT(frontBuffer);
         ASSERT(backBuffer);

	 for (i = 0; i < dPriv->h; i++) {
            _mesa_memcpy(tmp, (char *) backBuffer + offset,
                         currentPitch);
            _mesa_memcpy((char *) frontBuffer + offset, tmp,
                          currentPitch);
            offset += currentPitch;
	 }
	    
	 _mesa_free(tmp);
      }
   }
   else {
      /* XXX this shouldn't be an error but we can't handle it for now */
      _mesa_problem(NULL, "fbSwapBuffers: drawable has no context!\n");
   }
}


/* Force the context `c' to be the current context and associate with it
 * buffer `b'.
 */
static GLboolean
fbMakeCurrent( __DRIcontextPrivate *driContextPriv,
	       __DRIdrawablePrivate *driDrawPriv,
	       __DRIdrawablePrivate *driReadPriv )
{
   if ( driContextPriv ) {
      fbContextPtr newFbCtx = 
            (fbContextPtr) driContextPriv->driverPrivate;

      newFbCtx->dri.drawable = driDrawPriv;

      _mesa_make_current( newFbCtx->glCtx, 
                           driDrawPriv->driverPrivate,
                           driReadPriv->driverPrivate);
   } else {
      _mesa_make_current( NULL, NULL, NULL );
   }

   return GL_TRUE;
}


/* Force the context `c' to be unbound from its buffer.
 */
static GLboolean
fbUnbindContext( __DRIcontextPrivate *driContextPriv )
{
   return GL_TRUE;
}

static struct __DriverAPIRec fbAPI = {
   .InitDriver      = fbInitDriver,
   .DestroyScreen   = fbDestroyScreen,
   .CreateContext   = fbCreateContext,
   .DestroyContext  = fbDestroyContext,
   .CreateBuffer    = fbCreateBuffer,
   .DestroyBuffer   = fbDestroyBuffer,
   .SwapBuffers     = fbSwapBuffers,
   .MakeCurrent     = fbMakeCurrent,
   .UnbindContext   = fbUnbindContext,
};



static int
__driValidateMode(const DRIDriverContext *ctx )
{
   return 1;
}

static int
__driInitFBDev( struct DRIDriverContextRec *ctx )
{
   /* Note that drmOpen will try to load the kernel module, if needed. */
   /* we need a fbdev drm driver - it will only track maps */
   ctx->drmFD = drmOpen("radeon", NULL );
   if (ctx->drmFD < 0) {
      fprintf(stderr, "[drm] drmOpen failed\n");
      return 0;
   }

   ctx->shared.SAREASize = SAREA_MAX;

   if (drmAddMap( ctx->drmFD,
       0,
       ctx->shared.SAREASize,
       DRM_SHM,
       DRM_CONTAINS_LOCK,
       &ctx->shared.hSAREA) < 0)
   {
      fprintf(stderr, "[drm] drmAddMap failed\n");
      return 0;
   }
   fprintf(stderr, "[drm] added %d byte SAREA at 0x%08lx\n",
           ctx->shared.SAREASize,
           (unsigned long) ctx->shared.hSAREA);

   if (drmMap( ctx->drmFD,
       ctx->shared.hSAREA,
       ctx->shared.SAREASize,
       (drmAddressPtr)(&ctx->pSAREA)) < 0)
   {
      fprintf(stderr, "[drm] drmMap failed\n");
      return 0;
   }
   memset(ctx->pSAREA, 0, ctx->shared.SAREASize);
   fprintf(stderr, "[drm] mapped SAREA 0x%08lx to %p, size %d\n",
           (unsigned long) ctx->shared.hSAREA, ctx->pSAREA,
           ctx->shared.SAREASize);
   
   /* Need to AddMap the framebuffer and mmio regions here:
   */
   if (drmAddMap( ctx->drmFD,
       (drm_handle_t)ctx->FBStart,
       ctx->FBSize,
       DRM_FRAME_BUFFER,
#ifndef _EMBEDDED
		  0,
#else
		  DRM_READ_ONLY,
#endif
		  &ctx->shared.hFrameBuffer) < 0)
   {
      fprintf(stderr, "[drm] drmAddMap framebuffer failed\n");
      return 0;
   }

   fprintf(stderr, "[drm] framebuffer handle = 0x%08lx\n",
           (unsigned long) ctx->shared.hFrameBuffer);

   return 1;
}

static void
__driHaltFBDev( struct DRIDriverContextRec *ctx )
{
}

struct DRIDriverRec __driDriver = {
   __driValidateMode,
   __driValidateMode,
   __driInitFBDev,
   __driHaltFBDev
};

static __GLcontextModes *
fbFillInModes( unsigned pixel_bits, unsigned depth_bits,
                 unsigned stencil_bits, GLboolean have_back_buffer )
{
   __GLcontextModes * modes;
   __GLcontextModes * m;
   unsigned num_modes;
   unsigned depth_buffer_factor;
   unsigned back_buffer_factor;
   GLenum fb_format;
   GLenum fb_type;

    /* Right now GLX_SWAP_COPY_OML isn't supported, but it would be easy
   * enough to add support.  Basically, if a context is created with an
   * fbconfig where the swap method is GLX_SWAP_COPY_OML, pageflipping
   * will never be used.
    */
   static const GLenum back_buffer_modes[] = {
      GLX_NONE, GLX_SWAP_UNDEFINED_OML /*, GLX_SWAP_COPY_OML */
   };

   u_int8_t depth_bits_array[2];
   u_int8_t stencil_bits_array[2];


   depth_bits_array[0] = depth_bits;
   depth_bits_array[1] = depth_bits;
    
    /* Just like with the accumulation buffer, always provide some modes
   * with a stencil buffer.  It will be a sw fallback, but some apps won't
   * care about that.
    */
   stencil_bits_array[0] = 0;
   stencil_bits_array[1] = (stencil_bits == 0) ? 8 : stencil_bits;

   depth_buffer_factor = ((depth_bits != 0) || (stencil_bits != 0)) ? 2 : 1;
   back_buffer_factor  = (have_back_buffer) ? 2 : 1;

   num_modes = depth_buffer_factor * back_buffer_factor * 4;

   if ( pixel_bits == 16 ) {
      fb_format = GL_RGB;
      fb_type = GL_UNSIGNED_SHORT_5_6_5;
   }
   else {
      fb_format = GL_RGBA;
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
void * __driCreateNewScreen( __DRInativeDisplay *dpy, int scrn, __DRIscreen *psc,
                                   const __GLcontextModes * modes,
                                   const __DRIversion * ddx_version,
                                   const __DRIversion * dri_version,
                                   const __DRIversion * drm_version,
                                   const __DRIframebuffer * frame_buffer,
                                   drmAddress pSAREA, int fd, 
                                   int internal_api_version,
                                   __GLcontextModes ** driver_modes )
{
   __DRIscreenPrivate *psp;
   static const __DRIversion ddx_expected = { 4, 0, 0 };
   static const __DRIversion dri_expected = { 4, 0, 0 };
   static const __DRIversion drm_expected = { 1, 5, 0 };


   if ( ! driCheckDriDdxDrmVersions2( "fb",
          dri_version, & dri_expected,
          ddx_version, & ddx_expected,
          drm_version, & drm_expected ) ) {
             return NULL;
          }
      
          psp = __driUtilCreateNewScreen(dpy, scrn, psc, NULL,
                                         ddx_version, dri_version, drm_version,
                                         frame_buffer, pSAREA, fd,
                                         internal_api_version, &fbAPI);
          if ( psp != NULL ) {
	     *driver_modes = fbFillInModes( psp->fbBPP,
					    (psp->fbBPP == 16) ? 16 : 24,
					    (psp->fbBPP == 16) ? 0  : 8,
					    1);
          }

          return (void *) psp;
}
