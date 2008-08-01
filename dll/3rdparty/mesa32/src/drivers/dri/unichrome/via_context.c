/*
 * Copyright 1998-2003 VIA Technologies, Inc. All Rights Reserved.
 * Copyright 2001-2003 S3 Graphics, Inc. All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sub license,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * VIA, S3 GRAPHICS, AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

/**
 * \file via_context.c
 * 
 * \author John Sheng (presumably of either VIA Technologies or S3 Graphics)
 * \author Others at VIA Technologies?
 * \author Others at S3 Graphics?
 */

#include "glheader.h"
#include "context.h"
#include "matrix.h"
#include "state.h"
#include "simple_list.h"
#include "extensions.h"
#include "framebuffer.h"
#include "renderbuffer.h"

#include "swrast/swrast.h"
#include "swrast_setup/swrast_setup.h"
#include "tnl/tnl.h"
#include "vbo/vbo.h"

#include "tnl/t_pipeline.h"

#include "drivers/common/driverfuncs.h"

#include "via_screen.h"
#include "via_dri.h"

#include "via_state.h"
#include "via_tex.h"
#include "via_span.h"
#include "via_tris.h"
#include "via_ioctl.h"
#include "via_fb.h"

#include <stdio.h>
#include "macros.h"
#include "drirenderbuffer.h"

#define need_GL_ARB_multisample
#define need_GL_ARB_point_parameters
#define need_GL_ARB_vertex_buffer_object
#define need_GL_EXT_fog_coord
#define need_GL_EXT_secondary_color
#include "extension_helper.h"

#define DRIVER_DATE	"20060710"

#include "vblank.h"
#include "utils.h"

GLuint VIA_DEBUG = 0;

/**
 * Return various strings for \c glGetString.
 *
 * \sa glGetString
 */
static const GLubyte *viaGetString(GLcontext *ctx, GLenum name)
{
   static char buffer[128];
   unsigned   offset;


   switch (name) {
   case GL_VENDOR:
      return (GLubyte *)"VIA Technology";

   case GL_RENDERER: {
      static const char * const chipset_names[] = {
	 "UniChrome",
	 "CastleRock (CLE266)",
	 "UniChrome (KM400)",
	 "UniChrome (K8M800)",
	 "UniChrome (PM8x0/CN400)",
      };
      struct via_context *vmesa = VIA_CONTEXT(ctx);
      unsigned id = vmesa->viaScreen->deviceID;

      offset = driGetRendererString( buffer, 
				     chipset_names[(id > VIA_PM800) ? 0 : id],
				     DRIVER_DATE, 0 );
      return (GLubyte *)buffer;
   }

   default:
      return NULL;
   }
}


/**
 * Calculate a width that satisfies the hardware's alignment requirements.
 * On the Unichrome hardware, each scanline must be aligned to a multiple of
 * 16 pixels.
 *
 * \param width  Minimum buffer width, in pixels.
 * 
 * \returns A pixel width that meets the alignment requirements.
 */
static __inline__ unsigned
buffer_align( unsigned width )
{
    return (width + 0x0f) & ~0x0f;
}


static void
viaDeleteRenderbuffer(struct gl_renderbuffer *rb)
{
   /* Don't free() since we're contained in via_context struct. */
}

static GLboolean
viaRenderbufferStorage(GLcontext *ctx, struct gl_renderbuffer *rb,
                       GLenum internalFormat, GLuint width, GLuint height)
{
   rb->Width = width;
   rb->Height = height;
   rb->InternalFormat = internalFormat;
   return GL_TRUE;
}


static void
viaInitRenderbuffer(struct via_renderbuffer *vrb, GLenum format,
		    __DRIdrawablePrivate *dPriv)
{
   const GLuint name = 0;
   struct gl_renderbuffer *rb = & vrb->Base;

   vrb->dPriv = dPriv;
   _mesa_init_renderbuffer(rb, name);

   /* Make sure we're using a null-valued GetPointer routine */
   assert(rb->GetPointer(NULL, rb, 0, 0) == NULL);

   rb->InternalFormat = format;

   if (format == GL_RGBA) {
      /* Color */
      rb->_BaseFormat = GL_RGBA;
      rb->DataType = GL_UNSIGNED_BYTE;
   }
   else if (format == GL_DEPTH_COMPONENT16) {
      /* Depth */
      rb->_BaseFormat = GL_DEPTH_COMPONENT;
      /* we always Get/Put 32-bit Z values */
      rb->DataType = GL_UNSIGNED_INT;
   }
   else if (format == GL_DEPTH_COMPONENT24) {
      /* Depth */
      rb->_BaseFormat = GL_DEPTH_COMPONENT;
      /* we always Get/Put 32-bit Z values */
      rb->DataType = GL_UNSIGNED_INT;
   }
   else {
      /* Stencil */
      ASSERT(format == GL_STENCIL_INDEX8_EXT);
      rb->_BaseFormat = GL_STENCIL_INDEX;
      rb->DataType = GL_UNSIGNED_BYTE;
   }

   rb->Delete = viaDeleteRenderbuffer;
   rb->AllocStorage = viaRenderbufferStorage;
}


/**
 * Calculate the framebuffer parameters for all buffers (front, back, depth,
 * and stencil) associated with the specified context.
 * 
 * \warning
 * This function also calls \c AllocateBuffer to actually allocate the
 * buffers.
 * 
 * \sa AllocateBuffer
 */
static GLboolean
calculate_buffer_parameters(struct via_context *vmesa,
			    struct gl_framebuffer *fb,
			    __DRIdrawablePrivate *dPriv)
{
   const unsigned shift = vmesa->viaScreen->bitsPerPixel / 16;
   const unsigned extra = 32;
   unsigned w;
   unsigned h;

   /* Normally, the renderbuffer would be added to the framebuffer just once
    * when the framebuffer was created.  The VIA driver is a bit funny
    * though in that the front/back/depth renderbuffers are in the per-context
    * state!
    * That should be fixed someday.
    */

   if (!vmesa->front.Base.InternalFormat) {
      /* do one-time init for the renderbuffers */
      viaInitRenderbuffer(&vmesa->front, GL_RGBA, dPriv);
      viaSetSpanFunctions(&vmesa->front, &fb->Visual);
      _mesa_add_renderbuffer(fb, BUFFER_FRONT_LEFT, &vmesa->front.Base);

      if (fb->Visual.doubleBufferMode) {
         viaInitRenderbuffer(&vmesa->back, GL_RGBA, dPriv);
         viaSetSpanFunctions(&vmesa->back, &fb->Visual);
         _mesa_add_renderbuffer(fb, BUFFER_BACK_LEFT, &vmesa->back.Base);
      }

      if (vmesa->glCtx->Visual.depthBits > 0) {
         viaInitRenderbuffer(&vmesa->depth,
                             (vmesa->glCtx->Visual.depthBits == 16
                              ? GL_DEPTH_COMPONENT16 : GL_DEPTH_COMPONENT24),
			     dPriv);
         viaSetSpanFunctions(&vmesa->depth, &fb->Visual);
         _mesa_add_renderbuffer(fb, BUFFER_DEPTH, &vmesa->depth.Base);
      }

      if (vmesa->glCtx->Visual.stencilBits > 0) {
         viaInitRenderbuffer(&vmesa->stencil, GL_STENCIL_INDEX8_EXT,
			     dPriv);
         viaSetSpanFunctions(&vmesa->stencil, &fb->Visual);
         _mesa_add_renderbuffer(fb, BUFFER_STENCIL, &vmesa->stencil.Base);
      }
   }

   assert(vmesa->front.Base.InternalFormat);
   assert(vmesa->front.Base.AllocStorage);
   if (fb->Visual.doubleBufferMode) {
      assert(vmesa->back.Base.AllocStorage);
   }
   if (fb->Visual.depthBits) {
      assert(vmesa->depth.Base.AllocStorage);
   }


   /* Allocate front-buffer */
   if (vmesa->drawType == GLX_PBUFFER_BIT) {
      w = vmesa->driDrawable->w;
      h = vmesa->driDrawable->h;

      vmesa->front.bpp = vmesa->viaScreen->bitsPerPixel;
      vmesa->front.pitch = buffer_align( w ) << shift; /* bytes, not pixels */
      vmesa->front.size = vmesa->front.pitch * h;

      if (vmesa->front.map)
	 via_free_draw_buffer(vmesa, &vmesa->front);
      if (!via_alloc_draw_buffer(vmesa, &vmesa->front))
	 return GL_FALSE;

   } else {
      w = vmesa->viaScreen->width;
      h = vmesa->viaScreen->height;

      vmesa->front.bpp = vmesa->viaScreen->bitsPerPixel;
      vmesa->front.pitch = buffer_align( w ) << shift; /* bytes, not pixels */
      vmesa->front.size = vmesa->front.pitch * h;
      if (getenv("ALTERNATE_SCREEN")) 
        vmesa->front.offset = vmesa->front.size;
      else
      	vmesa->front.offset = 0;
      vmesa->front.map = (char *) vmesa->driScreen->pFB;
   }


   /* Allocate back-buffer */
   if (vmesa->hasBack) {
      vmesa->back.bpp = vmesa->viaScreen->bitsPerPixel;
      vmesa->back.pitch = (buffer_align( vmesa->driDrawable->w ) << shift);
      vmesa->back.pitch += extra;
      vmesa->back.pitch = MIN2(vmesa->back.pitch, vmesa->front.pitch);
      vmesa->back.size = vmesa->back.pitch * vmesa->driDrawable->h;
      if (vmesa->back.map)
	 via_free_draw_buffer(vmesa, &vmesa->back);
      if (!via_alloc_draw_buffer(vmesa, &vmesa->back))
	 return GL_FALSE;
   }
   else {
      if (vmesa->back.map)
	 via_free_draw_buffer(vmesa, &vmesa->back);
      (void) memset( &vmesa->back, 0, sizeof( vmesa->back ) );
   }


   /* Allocate depth-buffer */
   if ( vmesa->hasStencil || vmesa->hasDepth ) {
      vmesa->depth.bpp = vmesa->depthBits;
      if (vmesa->depth.bpp == 24)
	 vmesa->depth.bpp = 32;

      vmesa->depth.pitch = (buffer_align( vmesa->driDrawable->w ) * 
			    (vmesa->depth.bpp/8)) + extra;
      vmesa->depth.size = vmesa->depth.pitch * vmesa->driDrawable->h;

      if (vmesa->depth.map)
	 via_free_draw_buffer(vmesa, &vmesa->depth);
      if (!via_alloc_draw_buffer(vmesa, &vmesa->depth)) {
	 return GL_FALSE;
      }
   }
   else {
      if (vmesa->depth.map)
   	 via_free_draw_buffer(vmesa, &vmesa->depth);
      (void) memset( & vmesa->depth, 0, sizeof( vmesa->depth ) );
   }

   /* stencil buffer is same as depth buffer */
   vmesa->stencil.handle = vmesa->depth.handle;
   vmesa->stencil.size = vmesa->depth.size;
   vmesa->stencil.offset = vmesa->depth.offset;
   vmesa->stencil.index = vmesa->depth.index;
   vmesa->stencil.pitch = vmesa->depth.pitch;
   vmesa->stencil.bpp = vmesa->depth.bpp;
   vmesa->stencil.map = vmesa->depth.map;
   vmesa->stencil.orig = vmesa->depth.orig;
   vmesa->stencil.origMap = vmesa->depth.origMap;

   if( vmesa->viaScreen->width == vmesa->driDrawable->w && 
       vmesa->viaScreen->height == vmesa->driDrawable->h ) {
      vmesa->doPageFlip = vmesa->allowPageFlip;
      if (vmesa->hasBack) {
         assert(vmesa->back.pitch == vmesa->front.pitch);
      }
   }
   else
      vmesa->doPageFlip = GL_FALSE;

   return GL_TRUE;
}


void viaReAllocateBuffers(GLcontext *ctx, GLframebuffer *drawbuffer,
                          GLuint width, GLuint height)
{
    struct via_context *vmesa = VIA_CONTEXT(ctx);

    calculate_buffer_parameters(vmesa, drawbuffer, vmesa->driDrawable);

    _mesa_resize_framebuffer(ctx, drawbuffer, width, height);
}

/* Extension strings exported by the Unichrome driver.
 */
const struct dri_extension card_extensions[] =
{
    { "GL_ARB_multisample",                GL_ARB_multisample_functions },
    { "GL_ARB_multitexture",               NULL },
    { "GL_ARB_point_parameters",           GL_ARB_point_parameters_functions },
    { "GL_ARB_texture_env_add",            NULL },
    { "GL_ARB_texture_env_combine",        NULL },
/*    { "GL_ARB_texture_env_dot3",           NULL }, */
    { "GL_ARB_texture_mirrored_repeat",    NULL },
    { "GL_ARB_vertex_buffer_object",       GL_ARB_vertex_buffer_object_functions },
    { "GL_EXT_fog_coord",                  GL_EXT_fog_coord_functions },
    { "GL_EXT_secondary_color",            GL_EXT_secondary_color_functions },
    { "GL_EXT_stencil_wrap",               NULL },
    { "GL_EXT_texture_env_combine",        NULL },
/*    { "GL_EXT_texture_env_dot3",           NULL }, */
    { "GL_EXT_texture_lod_bias",           NULL },
    { "GL_NV_blend_square",                NULL },
    { NULL,                                NULL }
};

extern const struct tnl_pipeline_stage _via_fastrender_stage;
extern const struct tnl_pipeline_stage _via_render_stage;

static const struct tnl_pipeline_stage *via_pipeline[] = {
    &_tnl_vertex_transform_stage,
    &_tnl_normal_transform_stage,
    &_tnl_lighting_stage,
    &_tnl_fog_coordinate_stage,
    &_tnl_texgen_stage,
    &_tnl_texture_transform_stage,
    /* REMOVE: point attenuation stage */
#if 1
    &_via_fastrender_stage,     /* ADD: unclipped rastersetup-to-dma */
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
    { "2d",    DEBUG_2D },
    { NULL,    0 }
};


static GLboolean
AllocateDmaBuffer(struct via_context *vmesa)
{
    if (vmesa->dma)
        via_free_dma_buffer(vmesa);
    
    if (!via_alloc_dma_buffer(vmesa))
        return GL_FALSE;

    vmesa->dmaLow = 0;
    vmesa->dmaCliprectAddr = ~0;
    return GL_TRUE;
}

static void
FreeBuffer(struct via_context *vmesa)
{
    if (vmesa->front.map && vmesa->drawType == GLX_PBUFFER_BIT)
	via_free_draw_buffer(vmesa, &vmesa->front);

    if (vmesa->back.map)
        via_free_draw_buffer(vmesa, &vmesa->back);

    if (vmesa->depth.map)
        via_free_draw_buffer(vmesa, &vmesa->depth);

    if (vmesa->breadcrumb.map)
        via_free_draw_buffer(vmesa, &vmesa->breadcrumb);

    if (vmesa->dma)
        via_free_dma_buffer(vmesa);
}


GLboolean
viaCreateContext(const __GLcontextModes *visual,
                 __DRIcontextPrivate *driContextPriv,
                 void *sharedContextPrivate)
{
    GLcontext *ctx, *shareCtx;
    struct via_context *vmesa;
    __DRIscreenPrivate *sPriv = driContextPriv->driScreenPriv;
    viaScreenPrivate *viaScreen = (viaScreenPrivate *)sPriv->private;
    drm_via_sarea_t *saPriv = (drm_via_sarea_t *)
        (((GLubyte *)sPriv->pSAREA) + viaScreen->sareaPrivOffset);
    struct dd_function_table functions;

    /* Allocate via context */
    vmesa = (struct via_context *) CALLOC_STRUCT(via_context);
    if (!vmesa) {
        return GL_FALSE;
    }

    /* Parse configuration files.
     */
    driParseConfigFiles (&vmesa->optionCache, &viaScreen->optionCache,
			 sPriv->myNum, "unichrome");

    /* pick back buffer */
    vmesa->hasBack = visual->doubleBufferMode;

    switch(visual->depthBits) {
    case 0:			
       vmesa->hasDepth = GL_FALSE;
       vmesa->depthBits = 0; 
       vmesa->depth_max = 1.0;
       break;
    case 16:
       vmesa->hasDepth = GL_TRUE;
       vmesa->depthBits = visual->depthBits;
       vmesa->have_hw_stencil = GL_FALSE;
       vmesa->depth_max = (GLfloat)0xffff;
       vmesa->depth_clear_mask = 0xf << 28;
       vmesa->ClearDepth = 0xffff;
       vmesa->polygon_offset_scale = 1.0 / vmesa->depth_max;
       break;
    case 24:
       vmesa->hasDepth = GL_TRUE;
       vmesa->depthBits = visual->depthBits;
       vmesa->depth_max = (GLfloat) 0xffffff;
       vmesa->depth_clear_mask = 0xe << 28;
       vmesa->ClearDepth = 0xffffff00;

       assert(visual->haveStencilBuffer);
       assert(visual->stencilBits == 8);

       vmesa->have_hw_stencil = GL_TRUE;
       vmesa->stencilBits = visual->stencilBits;
       vmesa->stencil_clear_mask = 0x1 << 28;
       vmesa->polygon_offset_scale = 2.0 / vmesa->depth_max;
       break;
    case 32:
       vmesa->hasDepth = GL_TRUE;
       vmesa->depthBits = visual->depthBits;
       assert(!visual->haveStencilBuffer);
       vmesa->have_hw_stencil = GL_FALSE;
       vmesa->depth_max = (GLfloat)0xffffffff;
       vmesa->depth_clear_mask = 0xf << 28;
       vmesa->ClearDepth = 0xffffffff;
       vmesa->polygon_offset_scale = 2.0 / vmesa->depth_max;
       break;
    default:
       assert(0); 
       break;
    }

    make_empty_list(&vmesa->freed_tex_buffers);
    make_empty_list(&vmesa->tex_image_list[VIA_MEM_VIDEO]);
    make_empty_list(&vmesa->tex_image_list[VIA_MEM_AGP]);
    make_empty_list(&vmesa->tex_image_list[VIA_MEM_SYSTEM]);

    _mesa_init_driver_functions(&functions);
    viaInitTextureFuncs(&functions);

    /* Allocate the Mesa context */
    if (sharedContextPrivate)
        shareCtx = ((struct via_context *) sharedContextPrivate)->glCtx;
    else
        shareCtx = NULL;

    vmesa->glCtx = _mesa_create_context(visual, shareCtx, &functions,
					(void*) vmesa);
    
    vmesa->shareCtx = shareCtx;
    
    if (!vmesa->glCtx) {
        FREE(vmesa);
        return GL_FALSE;
    }
    driContextPriv->driverPrivate = vmesa;

    ctx = vmesa->glCtx;

    if (driQueryOptionb(&vmesa->optionCache, "excess_mipmap"))
        ctx->Const.MaxTextureLevels = 11;
    else
        ctx->Const.MaxTextureLevels = 10;

    ctx->Const.MaxTextureUnits = 2;
    ctx->Const.MaxTextureImageUnits = ctx->Const.MaxTextureUnits;
    ctx->Const.MaxTextureCoordUnits = ctx->Const.MaxTextureUnits;

    ctx->Const.MinLineWidth = 1.0;
    ctx->Const.MinLineWidthAA = 1.0;
    ctx->Const.MaxLineWidth = 1.0;
    ctx->Const.MaxLineWidthAA = 1.0;
    ctx->Const.LineWidthGranularity = 1.0;

    ctx->Const.MinPointSize = 1.0;
    ctx->Const.MinPointSizeAA = 1.0;
    ctx->Const.MaxPointSize = 1.0;
    ctx->Const.MaxPointSizeAA = 1.0;
    ctx->Const.PointSizeGranularity = 1.0;

    ctx->Driver.GetString = viaGetString;

    ctx->DriverCtx = (void *)vmesa;
    vmesa->glCtx = ctx;

    /* Initialize the software rasterizer and helper modules.
     */
    _swrast_CreateContext(ctx);
    _vbo_CreateContext(ctx);
    _tnl_CreateContext(ctx);
    _swsetup_CreateContext(ctx);

    /* Install the customized pipeline:
     */
    _tnl_destroy_pipeline(ctx);
    _tnl_install_pipeline(ctx, via_pipeline);

    /* Configure swrast and T&L to match hardware characteristics:
     */
    _swrast_allow_pixel_fog(ctx, GL_FALSE);
    _swrast_allow_vertex_fog(ctx, GL_TRUE);
    _tnl_allow_pixel_fog(ctx, GL_FALSE);
    _tnl_allow_vertex_fog(ctx, GL_TRUE);

/*     vmesa->display = dpy; */
    vmesa->display = sPriv->display;
    
    vmesa->hHWContext = driContextPriv->hHWContext;
    vmesa->driFd = sPriv->fd;
    vmesa->driHwLock = &sPriv->pSAREA->lock;

    vmesa->viaScreen = viaScreen;
    vmesa->driScreen = sPriv;
    vmesa->sarea = saPriv;

    vmesa->renderIndex = ~0;
    vmesa->setupIndex = ~0;
    vmesa->hwPrimitive = GL_POLYGON+1;

    /* KW: Hardwire this.  Was previously set bogusly in
     * viaCreateBuffer.  Needs work before PBUFFER can be used:
     */
    vmesa->drawType = GLX_WINDOW_BIT;


    _math_matrix_ctr(&vmesa->ViewportMatrix);

    /* Do this early, before VIA_FLUSH_DMA can be called:
     */
    if (!AllocateDmaBuffer(vmesa)) {
	fprintf(stderr ,"AllocateDmaBuffer fail\n");
	FreeBuffer(vmesa);
        FREE(vmesa);
        return GL_FALSE;
    }

    /* Allocate a small piece of fb memory for synchronization:
     */
    vmesa->breadcrumb.bpp = 32;
    vmesa->breadcrumb.pitch = buffer_align( 64 ) << 2;
    vmesa->breadcrumb.size = vmesa->breadcrumb.pitch;

    if (!via_alloc_draw_buffer(vmesa, &vmesa->breadcrumb)) {
        fprintf(stderr ,"AllocateDmaBuffer fail\n");
        FreeBuffer(vmesa);
        FREE(vmesa);
        return GL_FALSE;
    }

    driInitExtensions( ctx, card_extensions, GL_TRUE );
    viaInitStateFuncs(ctx);
    viaInitTriFuncs(ctx);
    viaInitSpanFuncs(ctx);
    viaInitIoctlFuncs(ctx);
    viaInitState(ctx);
        
    if (getenv("VIA_DEBUG"))
       VIA_DEBUG = driParseDebugString( getenv( "VIA_DEBUG" ),
					debug_control );

    if (getenv("VIA_NO_RAST") ||
        driQueryOptionb(&vmesa->optionCache, "no_rast"))
       FALLBACK(vmesa, VIA_FALLBACK_USER_DISABLE, 1);

    vmesa->vblank_flags =
       vmesa->viaScreen->irqEnabled ?
        driGetDefaultVBlankFlags(&vmesa->optionCache) : VBLANK_FLAG_NO_IRQ;

    if (getenv("VIA_PAGEFLIP"))
       vmesa->allowPageFlip = 1;

    (*dri_interface->getUST)( &vmesa->swap_ust );


    vmesa->regMMIOBase = (GLuint *)((unsigned long)viaScreen->reg);
    vmesa->pnGEMode = (GLuint *)((unsigned long)viaScreen->reg + 0x4);
    vmesa->regEngineStatus = (GLuint *)((unsigned long)viaScreen->reg + 0x400);
    vmesa->regTranSet = (GLuint *)((unsigned long)viaScreen->reg + 0x43C);
    vmesa->regTranSpace = (GLuint *)((unsigned long)viaScreen->reg + 0x440);
    vmesa->agpBase = viaScreen->agpBase;


    return GL_TRUE;
}

void
viaDestroyContext(__DRIcontextPrivate *driContextPriv)
{
    GET_CURRENT_CONTEXT(ctx);
    struct via_context *vmesa = 
       (struct via_context *)driContextPriv->driverPrivate;
    struct via_context *current = ctx ? VIA_CONTEXT(ctx) : NULL;
    assert(vmesa); /* should never be null */

    /* check if we're deleting the currently bound context */
    if (vmesa == current) {
      VIA_FLUSH_DMA(vmesa);
      _mesa_make_current(NULL, NULL, NULL);
    }

    if (vmesa) {
        viaWaitIdle(vmesa, GL_FALSE);
	if (vmesa->doPageFlip) {
	   LOCK_HARDWARE(vmesa);
	   if (vmesa->pfCurrentOffset != 0) {
	      fprintf(stderr, "%s - reset pf\n", __FUNCTION__);
	      viaResetPageFlippingLocked(vmesa);
	   }
	   UNLOCK_HARDWARE(vmesa);
	}
	
	_swsetup_DestroyContext(vmesa->glCtx);
        _tnl_DestroyContext(vmesa->glCtx);
        _vbo_DestroyContext(vmesa->glCtx);
        _swrast_DestroyContext(vmesa->glCtx);
        /* free the Mesa context */
	_mesa_destroy_context(vmesa->glCtx);
	/* release our data */
	FreeBuffer(vmesa);

	assert (is_empty_list(&vmesa->tex_image_list[VIA_MEM_AGP]));
	assert (is_empty_list(&vmesa->tex_image_list[VIA_MEM_VIDEO]));
	assert (is_empty_list(&vmesa->tex_image_list[VIA_MEM_SYSTEM]));
	assert (is_empty_list(&vmesa->freed_tex_buffers));

	driDestroyOptionCache(&vmesa->optionCache);

	FREE(vmesa);
    }
}


void viaXMesaWindowMoved(struct via_context *vmesa)
{
   __DRIdrawablePrivate *const drawable = vmesa->driDrawable;
   __DRIdrawablePrivate *const readable = vmesa->driReadable;
   struct via_renderbuffer * draw_buffer;
   struct via_renderbuffer * read_buffer;
   GLuint bytePerPixel = vmesa->viaScreen->bitsPerPixel >> 3;

   if (!drawable)
      return;

   draw_buffer =  (struct via_renderbuffer *) drawable->driverPrivate;
   read_buffer =  (struct via_renderbuffer *) readable->driverPrivate;
   
   switch (vmesa->glCtx->DrawBuffer->_ColorDrawBufferMask[0]) {
   case BUFFER_BIT_BACK_LEFT: 
      if (drawable->numBackClipRects == 0) {
	 vmesa->numClipRects = drawable->numClipRects;
	 vmesa->pClipRects = drawable->pClipRects;
      } 
      else {
	 vmesa->numClipRects = drawable->numBackClipRects;
	 vmesa->pClipRects = drawable->pBackClipRects;
      }
      break;
   case BUFFER_BIT_FRONT_LEFT:
      vmesa->numClipRects = drawable->numClipRects;
      vmesa->pClipRects = drawable->pClipRects;
      break;
   default:
      vmesa->numClipRects = 0;
      break;
   }

   if ((draw_buffer->drawW != drawable->w) 
       || (draw_buffer->drawH != drawable->h)) {
      calculate_buffer_parameters(vmesa, vmesa->glCtx->DrawBuffer,
				  drawable);
   }

   draw_buffer->drawX = drawable->x;
   draw_buffer->drawY = drawable->y;
   draw_buffer->drawW = drawable->w;
   draw_buffer->drawH = drawable->h;

   if (drawable != readable) {
      if ((read_buffer->drawW != readable->w) 
	  || (read_buffer->drawH != readable->h)) {
	 calculate_buffer_parameters(vmesa, vmesa->glCtx->ReadBuffer,
				     readable);
      }

      read_buffer->drawX = readable->x;
      read_buffer->drawY = readable->y;
      read_buffer->drawW = readable->w;
      read_buffer->drawH = readable->h;
   }

   vmesa->front.orig = (vmesa->front.offset + 
			draw_buffer->drawY * vmesa->front.pitch + 
			draw_buffer->drawX * bytePerPixel);

   vmesa->front.origMap = (vmesa->front.map + 
			draw_buffer->drawY * vmesa->front.pitch + 
			draw_buffer->drawX * bytePerPixel);

   vmesa->back.orig = (vmesa->back.offset +
			draw_buffer->drawY * vmesa->back.pitch +
			draw_buffer->drawX * bytePerPixel);

   vmesa->back.origMap = (vmesa->back.map +
			draw_buffer->drawY * vmesa->back.pitch +
			draw_buffer->drawX * bytePerPixel);

   vmesa->depth.orig = (vmesa->depth.offset +
			draw_buffer->drawY * vmesa->depth.pitch +
			draw_buffer->drawX * bytePerPixel);   

   vmesa->depth.origMap = (vmesa->depth.map +
			draw_buffer->drawY * vmesa->depth.pitch +
			draw_buffer->drawX * bytePerPixel);

   viaCalcViewport(vmesa->glCtx);
}

GLboolean
viaUnbindContext(__DRIcontextPrivate *driContextPriv)
{
    return GL_TRUE;
}

GLboolean
viaMakeCurrent(__DRIcontextPrivate *driContextPriv,
               __DRIdrawablePrivate *driDrawPriv,
               __DRIdrawablePrivate *driReadPriv)
{
    if (VIA_DEBUG & DEBUG_DRI) {
	fprintf(stderr, "driContextPriv = %016lx\n", (unsigned long)driContextPriv);
	fprintf(stderr, "driDrawPriv = %016lx\n", (unsigned long)driDrawPriv);    
	fprintf(stderr, "driReadPriv = %016lx\n", (unsigned long)driReadPriv);
    }	

    if (driContextPriv) {
        struct via_context *vmesa = 
	   (struct via_context *)driContextPriv->driverPrivate;
	GLcontext *ctx = vmesa->glCtx;
        struct gl_framebuffer *drawBuffer, *readBuffer;

        drawBuffer = (GLframebuffer *)driDrawPriv->driverPrivate;
        readBuffer = (GLframebuffer *)driReadPriv->driverPrivate;

	if (vmesa->driDrawable != driDrawPriv) {
	   driDrawableInitVBlank(driDrawPriv, vmesa->vblank_flags,
				 &vmesa->vbl_seq);
	}

       if ((vmesa->driDrawable != driDrawPriv)
	   || (vmesa->driReadable != driReadPriv)) {
	  vmesa->driDrawable = driDrawPriv;
	  vmesa->driReadable = driReadPriv;

	  if ((drawBuffer->Width != driDrawPriv->w) 
	      || (drawBuffer->Height != driDrawPriv->h)) {
	     _mesa_resize_framebuffer(ctx, drawBuffer,
				      driDrawPriv->w, driDrawPriv->h);
	     drawBuffer->Initialized = GL_TRUE;
	  }

	  if (!calculate_buffer_parameters(vmesa, drawBuffer, driDrawPriv)) {
	     return GL_FALSE;
	  }

	  if (driDrawPriv != driReadPriv) {
	     if ((readBuffer->Width != driReadPriv->w)
		 || (readBuffer->Height != driReadPriv->h)) {
		_mesa_resize_framebuffer(ctx, readBuffer,
					 driReadPriv->w, driReadPriv->h);
		readBuffer->Initialized = GL_TRUE;
	     }

	     if (!calculate_buffer_parameters(vmesa, readBuffer, driReadPriv)) {
		return GL_FALSE;
	     }
	  }
       }

        _mesa_make_current(vmesa->glCtx, drawBuffer, readBuffer);

	ctx->Driver.DrawBuffer( ctx, ctx->Color.DrawBuffer[0] );
	   
        viaXMesaWindowMoved(vmesa);
	ctx->Driver.Scissor(vmesa->glCtx,
			    vmesa->glCtx->Scissor.X,
			    vmesa->glCtx->Scissor.Y,
			    vmesa->glCtx->Scissor.Width,
			    vmesa->glCtx->Scissor.Height);
    }
    else {
        _mesa_make_current(NULL, NULL, NULL);
    }
        
    return GL_TRUE;
}

void viaGetLock(struct via_context *vmesa, GLuint flags)
{
    __DRIdrawablePrivate *dPriv = vmesa->driDrawable;
    __DRIscreenPrivate *sPriv = vmesa->driScreen;

    drmGetLock(vmesa->driFd, vmesa->hHWContext, flags);

    DRI_VALIDATE_DRAWABLE_INFO(sPriv, dPriv);
    if (dPriv != vmesa->driReadable) {
	DRI_VALIDATE_DRAWABLE_INFO(sPriv, vmesa->driReadable);
    }

    if (vmesa->sarea->ctxOwner != vmesa->hHWContext) {
       vmesa->sarea->ctxOwner = vmesa->hHWContext;
       vmesa->newEmitState = ~0;
    }

    if (vmesa->lastStamp != dPriv->lastStamp) {
       viaXMesaWindowMoved(vmesa);
       driUpdateFramebufferSize(vmesa->glCtx, dPriv);
       vmesa->newEmitState = ~0;
       vmesa->lastStamp = dPriv->lastStamp;
    }

    if (vmesa->doPageFlip &&
	vmesa->pfCurrentOffset != vmesa->sarea->pfCurrentOffset) {
       fprintf(stderr, "%s - reset pf\n", __FUNCTION__);
       viaResetPageFlippingLocked(vmesa);
    }
}


void
viaSwapBuffers(__DRIdrawablePrivate *drawablePrivate)
{
    __DRIdrawablePrivate *dPriv = (__DRIdrawablePrivate *)drawablePrivate;

    if (dPriv && 
	dPriv->driContextPriv && 
	dPriv->driContextPriv->driverPrivate) {
        struct via_context *vmesa = 
	   (struct via_context *)dPriv->driContextPriv->driverPrivate;
        GLcontext *ctx = vmesa->glCtx;

	_mesa_notifySwapBuffers(ctx);

        if (ctx->Visual.doubleBufferMode) {
            if (vmesa->doPageFlip) {
                viaPageFlip(dPriv);
            }
            else {
                viaCopyBuffer(dPriv);
            }
        }
	else
	    VIA_FLUSH_DMA(vmesa);
    }
    else {
        _mesa_problem(NULL, "viaSwapBuffers: drawable has no context!\n");
    }
}
