/**************************************************************************
 * 
 * Copyright 2003 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 **************************************************************************/

#include "glheader.h"
#include "context.h"
#include "framebuffer.h"
#include "matrix.h"
#include "renderbuffer.h"
#include "simple_list.h"
#include "utils.h"
#include "vblank.h"
#include "xmlpool.h"


#include "intel_screen.h"

#include "intel_buffers.h"
#include "intel_tex.h"
#include "intel_span.h"
#include "intel_tris.h"
#include "intel_ioctl.h"
#include "intel_fbo.h"

#include "i830_dri.h"
#include "dri_bufpool.h"
#include "intel_regions.h"
#include "intel_batchbuffer.h"

PUBLIC const char __driConfigOptions[] =
   DRI_CONF_BEGIN DRI_CONF_SECTION_PERFORMANCE
   DRI_CONF_FTHROTTLE_MODE(DRI_CONF_FTHROTTLE_IRQS)
   DRI_CONF_VBLANK_MODE(DRI_CONF_VBLANK_DEF_INTERVAL_0)
   DRI_CONF_SECTION_END DRI_CONF_SECTION_QUALITY
   DRI_CONF_FORCE_S3TC_ENABLE(false)
   DRI_CONF_ALLOW_LARGE_TEXTURES(1)
   DRI_CONF_SECTION_END DRI_CONF_END;
     const GLuint __driNConfigOptions = 4;

#ifdef USE_NEW_INTERFACE
     static PFNGLXCREATECONTEXTMODES create_context_modes = NULL;
#endif /*USE_NEW_INTERFACE */

     extern const struct dri_extension card_extensions[];

/**
 * Map all the memory regions described by the screen.
 * \return GL_TRUE if success, GL_FALSE if error.
 */
GLboolean
intelMapScreenRegions(__DRIscreenPrivate * sPriv)
{
   intelScreenPrivate *intelScreen = (intelScreenPrivate *) sPriv->private;

   if (intelScreen->front.handle) {
      if (drmMap(sPriv->fd,
                 intelScreen->front.handle,
                 intelScreen->front.size,
                 (drmAddress *) & intelScreen->front.map) != 0) {
         _mesa_problem(NULL, "drmMap(frontbuffer) failed!");
         return GL_FALSE;
      }
   }
   else {
      _mesa_warning(NULL, "no front buffer handle in intelMapScreenRegions!");
   }

   if (0)
      _mesa_printf("Back 0x%08x ", intelScreen->back.handle);
   if (drmMap(sPriv->fd,
              intelScreen->back.handle,
              intelScreen->back.size,
              (drmAddress *) & intelScreen->back.map) != 0) {
      intelUnmapScreenRegions(intelScreen);
      return GL_FALSE;
   }

   if (intelScreen->third.handle) {
      if (0)
	 _mesa_printf("Third 0x%08x ", intelScreen->third.handle);
      if (drmMap(sPriv->fd,
		 intelScreen->third.handle,
		 intelScreen->third.size,
		 (drmAddress *) & intelScreen->third.map) != 0) {
	 intelUnmapScreenRegions(intelScreen);
	 return GL_FALSE;
      }
   }

   if (0)
      _mesa_printf("Depth 0x%08x ", intelScreen->depth.handle);
   if (drmMap(sPriv->fd,
              intelScreen->depth.handle,
              intelScreen->depth.size,
              (drmAddress *) & intelScreen->depth.map) != 0) {
      intelUnmapScreenRegions(intelScreen);
      return GL_FALSE;
   }

#if 0
   _mesa_printf("TEX 0x%08x ", intelScreen->tex.handle);
   if (drmMap(sPriv->fd,
              intelScreen->tex.handle,
              intelScreen->tex.size,
              (drmAddress *) & intelScreen->tex.map) != 0) {
      intelUnmapScreenRegions(intelScreen);
      return GL_FALSE;
   }
#endif
   if (0)
      printf("Mappings:  front: %p  back: %p  third: %p  depth: %p  tex: %p\n",
             intelScreen->front.map,
             intelScreen->back.map, intelScreen->third.map,
             intelScreen->depth.map, intelScreen->tex.map);
   return GL_TRUE;
}


static struct intel_region *
intel_recreate_static(intelScreenPrivate *intelScreen,
		      struct intel_region *region,
		      GLuint mem_type,
		      GLuint offset,
		      void *virtual,
		      GLuint cpp, GLuint pitch, GLuint height)
{
  if (region) {
    intel_region_update_static(intelScreen, region, mem_type, offset,
			       virtual, cpp, pitch, height);
  } else {
    region = intel_region_create_static(intelScreen, mem_type, offset,
					virtual, cpp, pitch, height);
  }
  return region;
}
    

/* Create intel_region structs to describe the static front,back,depth
 * buffers created by the xserver. 
 *
 * Although FBO's mean we now no longer use these as render targets in
 * all circumstances, they won't go away until the back and depth
 * buffers become private, and the front and rotated buffers will
 * remain even then.
 *
 * Note that these don't allocate video memory, just describe
 * allocations alread made by the X server.
 */
static void
intel_recreate_static_regions(intelScreenPrivate *intelScreen)
{
   intelScreen->front_region =
      intel_recreate_static(intelScreen,
			    intelScreen->front_region,
			    DRM_BO_FLAG_MEM_TT,
			    intelScreen->front.offset,
			    intelScreen->front.map,
			    intelScreen->cpp,
			    intelScreen->front.pitch / intelScreen->cpp,
			    intelScreen->height);

   intelScreen->rotated_region =
      intel_recreate_static(intelScreen,
			    intelScreen->rotated_region,
			    DRM_BO_FLAG_MEM_TT,
			    intelScreen->rotated.offset,
			    intelScreen->rotated.map,
			    intelScreen->cpp,
			    intelScreen->rotated.pitch /
			    intelScreen->cpp, intelScreen->height);


   intelScreen->back_region =
      intel_recreate_static(intelScreen,
			    intelScreen->back_region,
			    DRM_BO_FLAG_MEM_TT,
			    intelScreen->back.offset,
			    intelScreen->back.map,
			    intelScreen->cpp,
			    intelScreen->back.pitch / intelScreen->cpp,
			    intelScreen->height);

   if (intelScreen->third.handle) {
      intelScreen->third_region =
	 intel_recreate_static(intelScreen,
			       intelScreen->third_region,
			       DRM_BO_FLAG_MEM_TT,
			       intelScreen->third.offset,
			       intelScreen->third.map,
			       intelScreen->cpp,
			       intelScreen->third.pitch / intelScreen->cpp,
			       intelScreen->height);
   }

   /* Still assuming front.cpp == depth.cpp
    */
   intelScreen->depth_region =
      intel_recreate_static(intelScreen,
			    intelScreen->depth_region,
			    DRM_BO_FLAG_MEM_TT,
			    intelScreen->depth.offset,
			    intelScreen->depth.map,
			    intelScreen->cpp,
			    intelScreen->depth.pitch / intelScreen->cpp,
			    intelScreen->height);
}

/**
 * Use the information in the sarea to update the screen parameters
 * related to screen rotation. Needs to be called locked.
 */
void
intelUpdateScreenRotation(__DRIscreenPrivate * sPriv, drmI830Sarea * sarea)
{
   intelScreenPrivate *intelScreen = (intelScreenPrivate *) sPriv->private;

   intelUnmapScreenRegions(intelScreen);
   intelUpdateScreenFromSAREA(intelScreen, sarea);
   if (!intelMapScreenRegions(sPriv)) {
      fprintf(stderr, "ERROR Remapping screen regions!!!\n");
   }
   intel_recreate_static_regions(intelScreen);
}


void
intelUnmapScreenRegions(intelScreenPrivate * intelScreen)
{
#define REALLY_UNMAP 1
   if (intelScreen->front.map) {
#if REALLY_UNMAP
      if (drmUnmap(intelScreen->front.map, intelScreen->front.size) != 0)
         printf("drmUnmap front failed!\n");
#endif
      intelScreen->front.map = NULL;
   }
   if (intelScreen->back.map) {
#if REALLY_UNMAP
      if (drmUnmap(intelScreen->back.map, intelScreen->back.size) != 0)
         printf("drmUnmap back failed!\n");
#endif
      intelScreen->back.map = NULL;
   }
   if (intelScreen->third.map) {
#if REALLY_UNMAP
      if (drmUnmap(intelScreen->third.map, intelScreen->third.size) != 0)
         printf("drmUnmap third failed!\n");
#endif
      intelScreen->third.map = NULL;
   }
   if (intelScreen->depth.map) {
#if REALLY_UNMAP
      drmUnmap(intelScreen->depth.map, intelScreen->depth.size);
      intelScreen->depth.map = NULL;
#endif
   }
   if (intelScreen->tex.map) {
#if REALLY_UNMAP
      drmUnmap(intelScreen->tex.map, intelScreen->tex.size);
      intelScreen->tex.map = NULL;
#endif
   }
}


static void
intelPrintDRIInfo(intelScreenPrivate * intelScreen,
                  __DRIscreenPrivate * sPriv, I830DRIPtr gDRIPriv)
{
   fprintf(stderr, "*** Front size:   0x%x  offset: 0x%x  pitch: %d\n",
           intelScreen->front.size, intelScreen->front.offset,
           intelScreen->front.pitch);
   fprintf(stderr, "*** Back size:    0x%x  offset: 0x%x  pitch: %d\n",
           intelScreen->back.size, intelScreen->back.offset,
           intelScreen->back.pitch);
   fprintf(stderr, "*** Depth size:   0x%x  offset: 0x%x  pitch: %d\n",
           intelScreen->depth.size, intelScreen->depth.offset,
           intelScreen->depth.pitch);
   fprintf(stderr, "*** Rotated size: 0x%x  offset: 0x%x  pitch: %d\n",
           intelScreen->rotated.size, intelScreen->rotated.offset,
           intelScreen->rotated.pitch);
   fprintf(stderr, "*** Texture size: 0x%x  offset: 0x%x\n",
           intelScreen->tex.size, intelScreen->tex.offset);
   fprintf(stderr, "*** Memory : 0x%x\n", gDRIPriv->mem);
}


static void
intelPrintSAREA(const drmI830Sarea * sarea)
{
   fprintf(stderr, "SAREA: sarea width %d  height %d\n", sarea->width,
           sarea->height);
   fprintf(stderr, "SAREA: pitch: %d\n", sarea->pitch);
   fprintf(stderr,
           "SAREA: front offset: 0x%08x  size: 0x%x  handle: 0x%x\n",
           sarea->front_offset, sarea->front_size,
           (unsigned) sarea->front_handle);
   fprintf(stderr,
           "SAREA: back  offset: 0x%08x  size: 0x%x  handle: 0x%x\n",
           sarea->back_offset, sarea->back_size,
           (unsigned) sarea->back_handle);
   fprintf(stderr, "SAREA: depth offset: 0x%08x  size: 0x%x  handle: 0x%x\n",
           sarea->depth_offset, sarea->depth_size,
           (unsigned) sarea->depth_handle);
   fprintf(stderr, "SAREA: tex   offset: 0x%08x  size: 0x%x  handle: 0x%x\n",
           sarea->tex_offset, sarea->tex_size, (unsigned) sarea->tex_handle);
   fprintf(stderr, "SAREA: rotation: %d\n", sarea->rotation);
   fprintf(stderr,
           "SAREA: rotated offset: 0x%08x  size: 0x%x\n",
           sarea->rotated_offset, sarea->rotated_size);
   fprintf(stderr, "SAREA: rotated pitch: %d\n", sarea->rotated_pitch);
}


/**
 * A number of the screen parameters are obtained/computed from
 * information in the SAREA.  This function updates those parameters.
 */
void
intelUpdateScreenFromSAREA(intelScreenPrivate * intelScreen,
                           drmI830Sarea * sarea)
{
   intelScreen->width = sarea->width;
   intelScreen->height = sarea->height;

   intelScreen->front.offset = sarea->front_offset;
   intelScreen->front.pitch = sarea->pitch * intelScreen->cpp;
   intelScreen->front.handle = sarea->front_handle;
   intelScreen->front.size = sarea->front_size;

   intelScreen->back.offset = sarea->back_offset;
   intelScreen->back.pitch = sarea->pitch * intelScreen->cpp;
   intelScreen->back.handle = sarea->back_handle;
   intelScreen->back.size = sarea->back_size;

   if (intelScreen->driScrnPriv->ddxMinor >= 8) {
      intelScreen->third.offset = sarea->third_offset;
      intelScreen->third.pitch = sarea->pitch * intelScreen->cpp;
      intelScreen->third.handle = sarea->third_handle;
      intelScreen->third.size = sarea->third_size;
   }

   intelScreen->depth.offset = sarea->depth_offset;
   intelScreen->depth.pitch = sarea->pitch * intelScreen->cpp;
   intelScreen->depth.handle = sarea->depth_handle;
   intelScreen->depth.size = sarea->depth_size;

   intelScreen->tex.offset = sarea->tex_offset;
   intelScreen->logTextureGranularity = sarea->log_tex_granularity;
   intelScreen->tex.handle = sarea->tex_handle;
   intelScreen->tex.size = sarea->tex_size;

   intelScreen->rotated.offset = sarea->rotated_offset;
   intelScreen->rotated.pitch = sarea->rotated_pitch * intelScreen->cpp;
   intelScreen->rotated.size = sarea->rotated_size;
   intelScreen->current_rotation = sarea->rotation;
   matrix23Rotate(&intelScreen->rotMatrix,
                  sarea->width, sarea->height, sarea->rotation);
   intelScreen->rotatedWidth = sarea->virtualX;
   intelScreen->rotatedHeight = sarea->virtualY;

   if (0)
      intelPrintSAREA(sarea);
}

GLboolean
intelCreatePools(intelScreenPrivate *intelScreen)
{
   unsigned batchPoolSize = 1024*1024;
   __DRIscreenPrivate * sPriv = intelScreen->driScrnPriv;

   if (intelScreen->havePools)
      return GL_TRUE;

   batchPoolSize /= intelScreen->maxBatchSize;
   intelScreen->regionPool = driDRMPoolInit(sPriv->fd);

   if (!intelScreen->regionPool)
      return GL_FALSE;

   intelScreen->staticPool = driDRMStaticPoolInit(sPriv->fd);

   if (!intelScreen->staticPool)
      return GL_FALSE;

   intelScreen->texPool = intelScreen->regionPool;

   intelScreen->batchPool = driBatchPoolInit(sPriv->fd,
                                             DRM_BO_FLAG_EXE |
                                             DRM_BO_FLAG_MEM_TT |
                                             DRM_BO_FLAG_MEM_LOCAL,
                                             intelScreen->maxBatchSize, 
					     batchPoolSize, 5);
   if (!intelScreen->batchPool) {
      fprintf(stderr, "Failed to initialize batch pool - possible incorrect agpgart installed\n");
      return GL_FALSE;
   }
   
   intel_recreate_static_regions(intelScreen);
   intelScreen->havePools = GL_TRUE;

   return GL_TRUE;
}


static GLboolean
intelInitDriver(__DRIscreenPrivate * sPriv)
{
   intelScreenPrivate *intelScreen;
   I830DRIPtr gDRIPriv = (I830DRIPtr) sPriv->pDevPriv;
   drmI830Sarea *sarea;

   PFNGLXSCRENABLEEXTENSIONPROC glx_enable_extension =
      (PFNGLXSCRENABLEEXTENSIONPROC) (*dri_interface->
                                      getProcAddress("glxEnableExtension"));
   void *const psc = sPriv->psc->screenConfigs;

   if (sPriv->devPrivSize != sizeof(I830DRIRec)) {
      fprintf(stderr,
              "\nERROR!  sizeof(I830DRIRec) does not match passed size from device driver\n");
      return GL_FALSE;
   }

   /* Allocate the private area */
   intelScreen = (intelScreenPrivate *) CALLOC(sizeof(intelScreenPrivate));
   if (!intelScreen) {
      fprintf(stderr, "\nERROR!  Allocating private area failed\n");
      return GL_FALSE;
   }
   /* parse information in __driConfigOptions */
   driParseOptionInfo(&intelScreen->optionCache,
                      __driConfigOptions, __driNConfigOptions);

   intelScreen->driScrnPriv = sPriv;
   sPriv->private = (void *) intelScreen;
   intelScreen->sarea_priv_offset = gDRIPriv->sarea_priv_offset;
   sarea = (drmI830Sarea *)
      (((GLubyte *) sPriv->pSAREA) + intelScreen->sarea_priv_offset);

   intelScreen->maxBatchSize = BATCH_SZ;
   intelScreen->deviceID = gDRIPriv->deviceID;
   if (intelScreen->deviceID == PCI_CHIP_I865_G)
      intelScreen->maxBatchSize = 4096;

   intelScreen->mem = gDRIPriv->mem;
   intelScreen->cpp = gDRIPriv->cpp;

   switch (gDRIPriv->bitsPerPixel) {
   case 16:
      intelScreen->fbFormat = DV_PF_565;
      break;
   case 32:
      intelScreen->fbFormat = DV_PF_8888;
      break;
   default:
      exit(1);
      break;
   }

   intelUpdateScreenFromSAREA(intelScreen, sarea);

   if (!intelMapScreenRegions(sPriv)) {
      fprintf(stderr, "\nERROR!  mapping regions\n");
      _mesa_free(intelScreen);
      sPriv->private = NULL;
      return GL_FALSE;
   }

#if 0

   /*
    * FIXME: Remove this code and its references.
    */

   intelScreen->tex.offset = gDRIPriv->textureOffset;
   intelScreen->logTextureGranularity = gDRIPriv->logTextureGranularity;
   intelScreen->tex.handle = gDRIPriv->textures;
   intelScreen->tex.size = gDRIPriv->textureSize;

#else
   intelScreen->tex.offset = 0;
   intelScreen->logTextureGranularity = 0;
   intelScreen->tex.handle = 0;
   intelScreen->tex.size = 0;
#endif

   intelScreen->sarea_priv_offset = gDRIPriv->sarea_priv_offset;

   if (0)
      intelPrintDRIInfo(intelScreen, sPriv, gDRIPriv);

   intelScreen->drmMinor = sPriv->drmMinor;

   /* Determine if IRQs are active? */
   {
      int ret;
      drmI830GetParam gp;

      gp.param = I830_PARAM_IRQ_ACTIVE;
      gp.value = &intelScreen->irq_active;

      ret = drmCommandWriteRead(sPriv->fd, DRM_I830_GETPARAM,
                                &gp, sizeof(gp));
      if (ret) {
         fprintf(stderr, "drmI830GetParam: %d\n", ret);
         return GL_FALSE;
      }
   }

   /* Determine if batchbuffers are allowed */
   {
      int ret;
      drmI830GetParam gp;

      gp.param = I830_PARAM_ALLOW_BATCHBUFFER;
      gp.value = &intelScreen->allow_batchbuffer;

      ret = drmCommandWriteRead(sPriv->fd, DRM_I830_GETPARAM,
                                &gp, sizeof(gp));
      if (ret) {
         fprintf(stderr, "drmI830GetParam: (%d) %d\n", gp.param, ret);
         return GL_FALSE;
      }
   }

   if (glx_enable_extension != NULL) {
      (*glx_enable_extension) (psc, "GLX_SGI_swap_control");
      (*glx_enable_extension) (psc, "GLX_SGI_video_sync");
      (*glx_enable_extension) (psc, "GLX_MESA_swap_control");
      (*glx_enable_extension) (psc, "GLX_MESA_swap_frame_usage");
      (*glx_enable_extension) (psc, "GLX_SGI_make_current_read");
   }

   return GL_TRUE;
}


static void
intelDestroyScreen(__DRIscreenPrivate * sPriv)
{
   intelScreenPrivate *intelScreen = (intelScreenPrivate *) sPriv->private;

   intelUnmapScreenRegions(intelScreen);

   if (intelScreen->havePools) {
      driPoolTakeDown(intelScreen->regionPool);
      driPoolTakeDown(intelScreen->staticPool);
      driPoolTakeDown(intelScreen->batchPool);
   }
   FREE(intelScreen);
   sPriv->private = NULL;
}


/**
 * This is called when we need to set up GL rendering to a new X window.
 */
static GLboolean
intelCreateBuffer(__DRIscreenPrivate * driScrnPriv,
                  __DRIdrawablePrivate * driDrawPriv,
                  const __GLcontextModes * mesaVis, GLboolean isPixmap)
{
   intelScreenPrivate *screen = (intelScreenPrivate *) driScrnPriv->private;

   if (isPixmap) {
      return GL_FALSE;          /* not implemented */
   }
   else {
      GLboolean swStencil = (mesaVis->stencilBits > 0 &&
                             mesaVis->depthBits != 24);
      GLenum rgbFormat = (mesaVis->redBits == 5 ? GL_RGB5 : GL_RGBA8);

      struct intel_framebuffer *intel_fb = CALLOC_STRUCT(intel_framebuffer);

      if (!intel_fb)
	 return GL_FALSE;

      _mesa_initialize_framebuffer(&intel_fb->Base, mesaVis);

      /* setup the hardware-based renderbuffers */
      {
         intel_fb->color_rb[0]
            = intel_create_renderbuffer(rgbFormat,
                                        screen->width, screen->height,
                                        screen->front.offset,
                                        screen->front.pitch,
                                        screen->cpp,
                                        screen->front.map);
         intel_set_span_functions(&intel_fb->color_rb[0]->Base);
         _mesa_add_renderbuffer(&intel_fb->Base, BUFFER_FRONT_LEFT,
				&intel_fb->color_rb[0]->Base);
      }

      if (mesaVis->doubleBufferMode) {
         intel_fb->color_rb[1]
            = intel_create_renderbuffer(rgbFormat,
                                        screen->width, screen->height,
                                        screen->back.offset,
                                        screen->back.pitch,
                                        screen->cpp,
                                        screen->back.map);
         intel_set_span_functions(&intel_fb->color_rb[1]->Base);
         _mesa_add_renderbuffer(&intel_fb->Base, BUFFER_BACK_LEFT,
				&intel_fb->color_rb[1]->Base);

	 if (screen->third.handle) {
	    struct gl_renderbuffer *tmp_rb = NULL;

	    intel_fb->color_rb[2]
	       = intel_create_renderbuffer(rgbFormat,
					   screen->width, screen->height,
					   screen->third.offset,
					   screen->third.pitch,
					   screen->cpp,
					   screen->third.map);
	    intel_set_span_functions(&intel_fb->color_rb[2]->Base);
	    _mesa_reference_renderbuffer(&tmp_rb, &intel_fb->color_rb[2]->Base);
	 }
      }

      if (mesaVis->depthBits == 24 && mesaVis->stencilBits == 8) {
         /* combined depth/stencil buffer */
         struct intel_renderbuffer *depthStencilRb
            = intel_create_renderbuffer(GL_DEPTH24_STENCIL8_EXT,
                                        screen->width, screen->height,
                                        screen->depth.offset,
                                        screen->depth.pitch,
                                        screen->cpp,    /* 4! */
                                        screen->depth.map);
         intel_set_span_functions(&depthStencilRb->Base);
         /* note: bind RB to two attachment points */
         _mesa_add_renderbuffer(&intel_fb->Base, BUFFER_DEPTH,
				&depthStencilRb->Base);
         _mesa_add_renderbuffer(&intel_fb->Base, BUFFER_STENCIL,
				&depthStencilRb->Base);
      }
      else if (mesaVis->depthBits == 16) {
         /* just 16-bit depth buffer, no hw stencil */
         struct intel_renderbuffer *depthRb
            = intel_create_renderbuffer(GL_DEPTH_COMPONENT16,
                                        screen->width, screen->height,
                                        screen->depth.offset,
                                        screen->depth.pitch,
                                        screen->cpp,    /* 2! */
                                        screen->depth.map);
         intel_set_span_functions(&depthRb->Base);
         _mesa_add_renderbuffer(&intel_fb->Base, BUFFER_DEPTH, &depthRb->Base);
      }

      /* now add any/all software-based renderbuffers we may need */
      _mesa_add_soft_renderbuffers(&intel_fb->Base,
                                   GL_FALSE, /* never sw color */
                                   GL_FALSE, /* never sw depth */
                                   swStencil, mesaVis->accumRedBits > 0,
                                   GL_FALSE, /* never sw alpha */
                                   GL_FALSE  /* never sw aux */ );
      driDrawPriv->driverPrivate = (void *) intel_fb;

      return GL_TRUE;
   }
}

static void
intelDestroyBuffer(__DRIdrawablePrivate * driDrawPriv)
{
   _mesa_unreference_framebuffer((GLframebuffer **)(&(driDrawPriv->driverPrivate)));
}


/**
 * Get information about previous buffer swaps.
 */
static int
intelGetSwapInfo(__DRIdrawablePrivate * dPriv, __DRIswapInfo * sInfo)
{
   struct intel_framebuffer *intel_fb;

   if ((dPriv == NULL) || (dPriv->driverPrivate == NULL)
       || (sInfo == NULL)) {
      return -1;
   }

   intel_fb = dPriv->driverPrivate;
   sInfo->swap_count = intel_fb->swap_count;
   sInfo->swap_ust = intel_fb->swap_ust;
   sInfo->swap_missed_count = intel_fb->swap_missed_count;

   sInfo->swap_missed_usage = (sInfo->swap_missed_count != 0)
      ? driCalculateSwapUsage(dPriv, 0, intel_fb->swap_missed_ust)
      : 0.0;

   return 0;
}


/* There are probably better ways to do this, such as an
 * init-designated function to register chipids and createcontext
 * functions.
 */
extern GLboolean i830CreateContext(const __GLcontextModes * mesaVis,
                                   __DRIcontextPrivate * driContextPriv,
                                   void *sharedContextPrivate);

extern GLboolean i915CreateContext(const __GLcontextModes * mesaVis,
                                   __DRIcontextPrivate * driContextPriv,
                                   void *sharedContextPrivate);




static GLboolean
intelCreateContext(const __GLcontextModes * mesaVis,
                   __DRIcontextPrivate * driContextPriv,
                   void *sharedContextPrivate)
{
   __DRIscreenPrivate *sPriv = driContextPriv->driScreenPriv;
   intelScreenPrivate *intelScreen = (intelScreenPrivate *) sPriv->private;

   switch (intelScreen->deviceID) {
      /* Don't deal with i830 until texture work complete:
       */
   case PCI_CHIP_845_G:
   case PCI_CHIP_I830_M:
   case PCI_CHIP_I855_GM:
   case PCI_CHIP_I865_G:
      return i830CreateContext(mesaVis, driContextPriv, sharedContextPrivate);

   case PCI_CHIP_I915_G:
   case PCI_CHIP_I915_GM:
   case PCI_CHIP_I945_G:
   case PCI_CHIP_I945_GM:
   case PCI_CHIP_I945_GME:
   case PCI_CHIP_G33_G:
   case PCI_CHIP_Q35_G:
   case PCI_CHIP_Q33_G:
      return i915CreateContext(mesaVis, driContextPriv, sharedContextPrivate);

   default:
      fprintf(stderr, "Unrecognized deviceID %x\n", intelScreen->deviceID);
      return GL_FALSE;
   }
}


static const struct __DriverAPIRec intelAPI = {
   .InitDriver = intelInitDriver,
   .DestroyScreen = intelDestroyScreen,
   .CreateContext = intelCreateContext,
   .DestroyContext = intelDestroyContext,
   .CreateBuffer = intelCreateBuffer,
   .DestroyBuffer = intelDestroyBuffer,
   .SwapBuffers = intelSwapBuffers,
   .MakeCurrent = intelMakeCurrent,
   .UnbindContext = intelUnbindContext,
   .GetSwapInfo = intelGetSwapInfo,
   .GetMSC = driGetMSC32,
   .WaitForMSC = driWaitForMSC32,
   .WaitForSBC = NULL,
   .SwapBuffersMSC = NULL,
   .CopySubBuffer = intelCopySubBuffer,
   .setTexOffset = intelSetTexOffset,
};


static __GLcontextModes *
intelFillInModes(unsigned pixel_bits, unsigned depth_bits,
                 unsigned stencil_bits, GLboolean have_back_buffer)
{
   __GLcontextModes *modes;
   __GLcontextModes *m;
   unsigned num_modes;
   unsigned depth_buffer_factor;
   unsigned back_buffer_factor;
   GLenum fb_format;
   GLenum fb_type;

   /* GLX_SWAP_COPY_OML is only supported because the Intel driver doesn't
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
   if (depth_bits == 24)
      stencil_bits_array[1] = (stencil_bits == 0) ? 8 : stencil_bits;

   stencil_bits_array[2] = (stencil_bits == 0) ? 8 : stencil_bits;

   depth_buffer_factor = ((depth_bits != 0) || (stencil_bits != 0)) ? 3 : 1;
   back_buffer_factor = (have_back_buffer) ? 3 : 1;

   num_modes = depth_buffer_factor * back_buffer_factor * 4;

   if (pixel_bits == 16) {
      fb_format = GL_RGB;
      fb_type = GL_UNSIGNED_SHORT_5_6_5;
   }
   else {
      fb_format = GL_BGRA;
      fb_type = GL_UNSIGNED_INT_8_8_8_8_REV;
   }

   modes =
      (*dri_interface->createContextModes) (num_modes,
                                            sizeof(__GLcontextModes));
   m = modes;
   if (!driFillInModes(&m, fb_format, fb_type,
                       depth_bits_array, stencil_bits_array,
                       depth_buffer_factor, back_buffer_modes,
                       back_buffer_factor, GLX_TRUE_COLOR)) {
      fprintf(stderr, "[%s:%u] Error creating FBConfig!\n", __func__,
              __LINE__);
      return NULL;
   }
   if (!driFillInModes(&m, fb_format, fb_type,
                       depth_bits_array, stencil_bits_array,
                       depth_buffer_factor, back_buffer_modes,
                       back_buffer_factor, GLX_DIRECT_COLOR)) {
      fprintf(stderr, "[%s:%u] Error creating FBConfig!\n", __func__,
              __LINE__);
      return NULL;
   }

   /* Mark the visual as slow if there are "fake" stencil bits.
    */
   for (m = modes; m != NULL; m = m->next) {
      if ((m->stencilBits != 0) && (m->stencilBits != stencil_bits)) {
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
PUBLIC void *
__driCreateNewScreen_20050727(__DRInativeDisplay * dpy, int scrn,
                              __DRIscreen * psc,
                              const __GLcontextModes * modes,
                              const __DRIversion * ddx_version,
                              const __DRIversion * dri_version,
                              const __DRIversion * drm_version,
                              const __DRIframebuffer * frame_buffer,
                              drmAddress pSAREA, int fd,
                              int internal_api_version,
                              const __DRIinterfaceMethods * interface,
                              __GLcontextModes ** driver_modes)
{
   __DRIscreenPrivate *psp;
   static const __DRIversion ddx_expected = { 1, 5, 0 };
   static const __DRIversion dri_expected = { 4, 0, 0 };
   static const __DRIversion drm_expected = { 1, 7, 0 };

   dri_interface = interface;

   if (!driCheckDriDdxDrmVersions2("i915",
                                   dri_version, &dri_expected,
                                   ddx_version, &ddx_expected,
                                   drm_version, &drm_expected)) {
      return NULL;
   }

   psp = __driUtilCreateNewScreen(dpy, scrn, psc, NULL,
                                  ddx_version, dri_version, drm_version,
                                  frame_buffer, pSAREA, fd,
                                  internal_api_version, &intelAPI);

   if (psp != NULL) {
      I830DRIPtr dri_priv = (I830DRIPtr) psp->pDevPriv;
      *driver_modes = intelFillInModes(dri_priv->cpp * 8,
                                       (dri_priv->cpp == 2) ? 16 : 24,
                                       (dri_priv->cpp == 2) ? 0 : 8, 1);

      /* Calling driInitExtensions here, with a NULL context pointer, does not actually
       * enable the extensions.  It just makes sure that all the dispatch offsets for all
       * the extensions that *might* be enables are known.  This is needed because the
       * dispatch offsets need to be known when _mesa_context_create is called, but we can't
       * enable the extensions until we have a context pointer.
       *
       * Hello chicken.  Hello egg.  How are you two today?
       */
      driInitExtensions(NULL, card_extensions, GL_FALSE);
   }

   return (void *) psp;
}

struct intel_context *intelScreenContext(intelScreenPrivate *intelScreen)
{
  /*
   * This should probably change to have the screen allocate a dummy
   * context at screen creation. For now just use the current context.
   */

  GET_CURRENT_CONTEXT(ctx);
  if (ctx == NULL) {
     _mesa_problem(NULL, "No current context in intelScreenContext\n");
     return NULL;
  }
  return intel_context(ctx);
}

