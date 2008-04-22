/*
 * EGL driver for radeon_dri.so
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#include "eglconfig.h"
#include "eglcontext.h"
#include "egldisplay.h"
#include "egldriver.h"
#include "eglglobals.h"
#include "egllog.h"
#include "eglmode.h"
#include "eglscreen.h"
#include "eglsurface.h"
#include "egldri.h"

#include "mtypes.h"
#include "memops.h"
#include "drm.h"
#include "drm_sarea.h"
#include "radeon_drm.h"
#include "radeon_dri.h"
#include "radeon.h"

static size_t radeon_drm_page_size;

/**
 * radeon driver-specific driver class derived from _EGLDriver
 */
typedef struct radeon_driver
{
   _EGLDriver Base;  /* base class/object */
   GLuint radeonStuff;
} radeonDriver;

static int
RADEONSetParam(driDisplay  *disp, int param, int value)
{
   drm_radeon_setparam_t sp;
   int ret;

   memset(&sp, 0, sizeof(sp));
   sp.param = param;
   sp.value = value;

   if ((ret=drmCommandWrite(disp->drmFD, DRM_RADEON_SETPARAM, &sp, sizeof(sp)))) {
     fprintf(stderr,"Set param failed\n", ret);
      return -1;
   }

   return 0;
}

static int
RADEONCheckDRMVersion(driDisplay *disp, RADEONInfoPtr info)
{
   drmVersionPtr  version;

   version = drmGetVersion(disp->drmFD);
   if (version) {
      int req_minor, req_patch;

      /* Need 1.21.x for card type detection getparam
       */
      req_minor = 21;
      req_patch = 0;

      if (version->version_major != 1 ||
          version->version_minor < req_minor ||
          (version->version_minor == req_minor &&
           version->version_patchlevel < req_patch)) {
         /* Incompatible drm version */
         fprintf(stderr,
                 "[dri] RADEONDRIScreenInit failed because of a version "
                 "mismatch.\n"
                 "[dri] radeon.o kernel module version is %d.%d.%d "
                 "but version 1.%d.%d or newer is needed.\n"
                 "[dri] Disabling DRI.\n",
                 version->version_major,
                 version->version_minor,
                 version->version_patchlevel,
                 req_minor,
                 req_patch);
         drmFreeVersion(version);
         return 0;
      }

      info->drmMinor = version->version_minor;
      drmFreeVersion(version);
   }

   return 1;
}


/**
 * \brief Compute base 2 logarithm.
 *
 * \param val value.
 *
 * \return base 2 logarithm of \p val.
 */
static int RADEONMinBits(int val)
{
   int  bits;

   if (!val) return 1;
   for (bits = 0; val; val >>= 1, ++bits);
   return bits;
}


/* Initialize the PCI GART state.  Request memory for use in PCI space,
 * and initialize the Radeon registers to point to that memory.
 */
static int RADEONDRIPciInit(driDisplay *disp, RADEONInfoPtr info)
{
    int  ret;
    int  flags = DRM_READ_ONLY | DRM_LOCKED | DRM_KERNEL;
    int            s, l;

    ret = drmScatterGatherAlloc(disp->drmFD, info->gartSize*1024*1024,
                                &info->gartMemHandle);
    if (ret < 0) {
        fprintf(stderr, "[pci] Out of memory (%d)\n", ret);
        return 0;
    }
    fprintf(stderr,
               "[pci] %d kB allocated with handle 0x%04lx\n",
            info->gartSize*1024, (long) info->gartMemHandle);

   info->gartOffset = 0;

   /* Initialize the CP ring buffer data */
   info->ringStart       = info->gartOffset;
   info->ringMapSize     = info->ringSize*1024*1024 + radeon_drm_page_size;

   info->ringReadOffset  = info->ringStart + info->ringMapSize;
   info->ringReadMapSize = radeon_drm_page_size;

   /* Reserve space for vertex/indirect buffers */
   info->bufStart        = info->ringReadOffset + info->ringReadMapSize;
   info->bufMapSize      = info->bufSize*1024*1024;

   /* Reserve the rest for AGP textures */
   info->gartTexStart     = info->bufStart + info->bufMapSize;
   s = (info->gartSize*1024*1024 - info->gartTexStart);
   l = RADEONMinBits((s-1) / RADEON_NR_TEX_REGIONS);
   if (l < RADEON_LOG_TEX_GRANULARITY) l = RADEON_LOG_TEX_GRANULARITY;
   info->gartTexMapSize   = (s >> l) << l;
   info->log2GARTTexGran  = l;

    if (drmAddMap(disp->drmFD, info->ringStart, info->ringMapSize,
                  DRM_SCATTER_GATHER, flags, &info->ringHandle) < 0) {
        fprintf(stderr,
                   "[pci] Could not add ring mapping\n");
        return 0;
    }
    fprintf(stderr,
               "[pci] ring handle = 0x%08lx\n", info->ringHandle);

    if (drmAddMap(disp->drmFD, info->ringReadOffset, info->ringReadMapSize,
                  DRM_SCATTER_GATHER, flags, &info->ringReadPtrHandle) < 0) {
        fprintf(stderr,
                   "[pci] Could not add ring read ptr mapping\n");
        return 0;
    }
    fprintf(stderr,
               "[pci] ring read ptr handle = 0x%08lx\n",
               info->ringReadPtrHandle);

    if (drmAddMap(disp->drmFD, info->bufStart, info->bufMapSize,
                  DRM_SCATTER_GATHER, 0, &info->bufHandle) < 0) {
        fprintf(stderr,
                   "[pci] Could not add vertex/indirect buffers mapping\n");
        return 0;
    }
    fprintf(stderr,
               "[pci] vertex/indirect buffers handle = 0x%08lx\n",
               info->bufHandle);

    if (drmAddMap(disp->drmFD, info->gartTexStart, info->gartTexMapSize,
                  DRM_SCATTER_GATHER, 0, &info->gartTexHandle) < 0) {
        fprintf(stderr,
                   "[pci] Could not add GART texture map mapping\n");
        return 0;
    }
    fprintf(stderr,
               "[pci] GART texture map handle = 0x%08lx\n",
               info->gartTexHandle);

    return 1;
}


/**
 * \brief Initialize the AGP state
 *
 * \param ctx display handle.
 * \param info driver private data.
 *
 * \return one on success, or zero on failure.
 *
 * Acquires and enables the AGP device. Reserves memory in the AGP space for
 * the ring buffer, vertex buffers and textures. Initialize the Radeon
 * registers to point to that memory and add client mappings.
 */
static int RADEONDRIAgpInit( driDisplay *disp, RADEONInfoPtr info)
{
   int            mode, ret;
   int            s, l;
   int agpmode = 1;

   if (drmAgpAcquire(disp->drmFD) < 0) {
      fprintf(stderr, "[gart] AGP not available\n");
      return 0;
   }

   mode = drmAgpGetMode(disp->drmFD);	/* Default mode */
   /* Disable fast write entirely - too many lockups.
    */
   mode &= ~RADEON_AGP_MODE_MASK;
   switch (agpmode) {
   case 4:          mode |= RADEON_AGP_4X_MODE;
   case 2:          mode |= RADEON_AGP_2X_MODE;
   case 1: default: mode |= RADEON_AGP_1X_MODE;
   }

   if (drmAgpEnable(disp->drmFD, mode) < 0) {
      fprintf(stderr, "[gart] AGP not enabled\n");
      drmAgpRelease(disp->drmFD);
      return 0;
   }

#if 0
   /* Workaround for some hardware bugs */
   if (info->ChipFamily < CHIP_FAMILY_R200)
      OUTREG(RADEON_AGP_CNTL, INREG(RADEON_AGP_CNTL) | 0x000e0000);
#endif
   info->gartOffset = 0;

   if ((ret = drmAgpAlloc(disp->drmFD, info->gartSize*1024*1024, 0, NULL,
                          &info->gartMemHandle)) < 0) {
      fprintf(stderr, "[gart] Out of memory (%d)\n", ret);
      drmAgpRelease(disp->drmFD);
      return 0;
   }
   fprintf(stderr,
           "[gart] %d kB allocated with handle 0x%08x\n",
           info->gartSize*1024, (unsigned)info->gartMemHandle);

   if (drmAgpBind(disp->drmFD,
                  info->gartMemHandle, info->gartOffset) < 0) {
      fprintf(stderr, "[gart] Could not bind\n");
      drmAgpFree(disp->drmFD, info->gartMemHandle);
      drmAgpRelease(disp->drmFD);
      return 0;
   }

   /* Initialize the CP ring buffer data */
   info->ringStart       = info->gartOffset;
   info->ringMapSize     = info->ringSize*1024*1024 + radeon_drm_page_size;

   info->ringReadOffset  = info->ringStart + info->ringMapSize;
   info->ringReadMapSize = radeon_drm_page_size;

   /* Reserve space for vertex/indirect buffers */
   info->bufStart        = info->ringReadOffset + info->ringReadMapSize;
   info->bufMapSize      = info->bufSize*1024*1024;

   /* Reserve the rest for AGP textures */
   info->gartTexStart     = info->bufStart + info->bufMapSize;
   s = (info->gartSize*1024*1024 - info->gartTexStart);
   l = RADEONMinBits((s-1) / RADEON_NR_TEX_REGIONS);
   if (l < RADEON_LOG_TEX_GRANULARITY) l = RADEON_LOG_TEX_GRANULARITY;
   info->gartTexMapSize   = (s >> l) << l;
   info->log2GARTTexGran  = l;

   if (drmAddMap(disp->drmFD, info->ringStart, info->ringMapSize,
                 DRM_AGP, DRM_READ_ONLY, &info->ringHandle) < 0) {
      fprintf(stderr, "[gart] Could not add ring mapping\n");
      return 0;
   }
   fprintf(stderr, "[gart] ring handle = 0x%08lx\n", info->ringHandle);


   if (drmAddMap(disp->drmFD, info->ringReadOffset, info->ringReadMapSize,
                 DRM_AGP, DRM_READ_ONLY, &info->ringReadPtrHandle) < 0) {
      fprintf(stderr,
              "[gart] Could not add ring read ptr mapping\n");
      return 0;
   }

   fprintf(stderr,
           "[gart] ring read ptr handle = 0x%08lx\n",
           info->ringReadPtrHandle);

   if (drmAddMap(disp->drmFD, info->bufStart, info->bufMapSize,
                 DRM_AGP, 0, &info->bufHandle) < 0) {
      fprintf(stderr,
              "[gart] Could not add vertex/indirect buffers mapping\n");
      return 0;
   }
   fprintf(stderr,
           "[gart] vertex/indirect buffers handle = 0x%08lx\n",
           info->bufHandle);

   if (drmAddMap(disp->drmFD, info->gartTexStart, info->gartTexMapSize,
                 DRM_AGP, 0, &info->gartTexHandle) < 0) {
      fprintf(stderr,
              "[gart] Could not add AGP texture map mapping\n");
      return 0;
   }
   fprintf(stderr,
           "[gart] AGP texture map handle = 0x%08lx\n",
           info->gartTexHandle);

   return 1;
}


/**
 * Initialize all the memory-related fields of the RADEONInfo object.
 * This includes the various 'offset' and 'size' fields.
 */
static int
RADEONMemoryInit(driDisplay *disp, RADEONInfoPtr info)
{
   int        width_bytes = disp->virtualWidth * disp->cpp;
   int        cpp         = disp->cpp;
   int        bufferSize  = ((disp->virtualHeight * width_bytes
                              + RADEON_BUFFER_ALIGN)
                             & ~RADEON_BUFFER_ALIGN);
   int        depthSize   = ((((disp->virtualHeight+15) & ~15) * width_bytes
                              + RADEON_BUFFER_ALIGN)
                             & ~RADEON_BUFFER_ALIGN);
   int        l;
   int        pcie_gart_table_size = 0;

   info->frontOffset = 0;
   info->frontPitch = disp->virtualWidth;

   if (disp->card_type==RADEON_CARD_PCIE)
     pcie_gart_table_size  = RADEON_PCIGART_TABLE_SIZE;

   /* Front, back and depth buffers - everything else texture??
    */
   info->textureSize = disp->fbSize - pcie_gart_table_size - 2 * bufferSize - depthSize;

   if (info->textureSize < 0)
      return 0;

   l = RADEONMinBits((info->textureSize-1) / RADEON_NR_TEX_REGIONS);
   if (l < RADEON_LOG_TEX_GRANULARITY) l = RADEON_LOG_TEX_GRANULARITY;

   /* Round the texture size up to the nearest whole number of
    * texture regions.  Again, be greedy about this, don't
    * round down.
    */
   info->log2TexGran = l;
   info->textureSize = (info->textureSize >> l) << l;

   /* Set a minimum usable local texture heap size.  This will fit
    * two 256x256x32bpp textures.
    */
   if (info->textureSize < 512 * 1024) {
      info->textureOffset = 0;
      info->textureSize = 0;
   }

   /* Reserve space for textures */
   info->textureOffset = ((disp->fbSize - pcie_gart_table_size - info->textureSize +
                           RADEON_BUFFER_ALIGN) &
                          ~RADEON_BUFFER_ALIGN);

   /* Reserve space for the shared depth
    * buffer.
    */
   info->depthOffset = ((info->textureOffset - depthSize +
                         RADEON_BUFFER_ALIGN) &
                        ~RADEON_BUFFER_ALIGN);
   info->depthPitch = disp->virtualWidth;

   info->backOffset = ((info->depthOffset - bufferSize +
                        RADEON_BUFFER_ALIGN) &
                       ~RADEON_BUFFER_ALIGN);
   info->backPitch = disp->virtualWidth;

   if (pcie_gart_table_size)
     info->pcieGartTableOffset = disp->fbSize - pcie_gart_table_size;

   fprintf(stderr,
           "Will use back buffer at offset 0x%x, pitch %d\n",
           info->backOffset, info->backPitch);
   fprintf(stderr,
           "Will use depth buffer at offset 0x%x, pitch %d\n",
           info->depthOffset, info->depthPitch);
   fprintf(stderr,
           "Will use %d kb for textures at offset 0x%x\n",
           info->textureSize/1024, info->textureOffset);
   if (pcie_gart_table_size)
   { 
     fprintf(stderr,
	     "Will use %d kb for PCIE GART Table at offset 0x%x\n",
	     pcie_gart_table_size/1024, info->pcieGartTableOffset);
   }

   /* XXX I don't think these are needed. */
#if 0
   info->frontPitchOffset = (((info->frontPitch * cpp / 64) << 22) |
                             (info->frontOffset >> 10));

   info->backPitchOffset = (((info->backPitch * cpp / 64) << 22) |
                            (info->backOffset >> 10));

   info->depthPitchOffset = (((info->depthPitch * cpp / 64) << 22) |
                             (info->depthOffset >> 10));
#endif

   if (pcie_gart_table_size)
     RADEONSetParam(disp, RADEON_SETPARAM_PCIGART_LOCATION, info->pcieGartTableOffset);

   return 1;
}


/**
 * \brief Initialize the kernel data structures and enable the CP engine.
 *
 * \param ctx display handle.
 * \param info driver private data.
 *
 * \return non-zero on success, or zero on failure.
 *
 * This function is a wrapper around the DRM_RADEON_CP_INIT command, passing
 * all the parameters in a drm_radeon_init_t structure.
 */
static int RADEONDRIKernelInit( driDisplay *disp,
                               RADEONInfoPtr info)
{
   int cpp = disp->bpp / 8;
   drm_radeon_init_t  drmInfo;
   int ret;

   memset(&drmInfo, 0, sizeof(drmInfo));

   if ( (info->ChipFamily >= CHIP_FAMILY_R300) )
      drmInfo.func            = RADEON_INIT_R300_CP;
   else if ( (info->ChipFamily == CHIP_FAMILY_R200) ||
        (info->ChipFamily == CHIP_FAMILY_RV250) ||
        (info->ChipFamily == CHIP_FAMILY_M9) ||
        (info->ChipFamily == CHIP_FAMILY_RV280) )
      drmInfo.func             = RADEON_INIT_R200_CP;
   else
      drmInfo.func             = RADEON_INIT_CP;

   /* This is the struct passed to the kernel module for its initialization */
   /* XXX problem here:
    * The front/back/depth_offset/pitch fields may change depending upon
    * which drawing surface we're using!!!  They can't be set just once
    * during initialization.
    * Looks like we'll need a new ioctl to update these fields for drawing
    * to other surfaces...
    */
   drmInfo.sarea_priv_offset   = sizeof(drm_sarea_t);
   drmInfo.cp_mode             = RADEON_DEFAULT_CP_BM_MODE;
   drmInfo.gart_size            = info->gartSize*1024*1024;
   drmInfo.ring_size           = info->ringSize*1024*1024;
   drmInfo.usec_timeout        = 1000;
   drmInfo.fb_bpp              = disp->bpp;
   drmInfo.depth_bpp           = disp->bpp;
   drmInfo.front_offset        = info->frontOffset;
   drmInfo.front_pitch         = info->frontPitch * cpp;
   drmInfo.back_offset         = info->backOffset;
   drmInfo.back_pitch          = info->backPitch * cpp;
   drmInfo.depth_offset        = info->depthOffset;
   drmInfo.depth_pitch         = info->depthPitch * cpp;
   drmInfo.ring_offset         = info->ringHandle;
   drmInfo.ring_rptr_offset    = info->ringReadPtrHandle;
   drmInfo.buffers_offset      = info->bufHandle;
   drmInfo.gart_textures_offset = info->gartTexHandle;

   ret = drmCommandWrite(disp->drmFD, DRM_RADEON_CP_INIT, &drmInfo,
                         sizeof(drm_radeon_init_t));

   return ret >= 0;
}


/**
 * \brief Add a map for the vertex buffers that will be accessed by any
 * DRI-based clients.
 *
 * \param ctx display handle.
 * \param info driver private data.
 *
 * \return one on success, or zero on failure.
 *
 * Calls drmAddBufs() with the previously allocated vertex buffers.
 */
static int RADEONDRIBufInit( driDisplay *disp, RADEONInfoPtr info )
{
   /* Initialize vertex buffers */
   info->bufNumBufs = drmAddBufs(disp->drmFD,
                                 info->bufMapSize / RADEON_BUFFER_SIZE,
                                 RADEON_BUFFER_SIZE,
				 (disp->card_type!=RADEON_CARD_AGP) ? DRM_SG_BUFFER : DRM_AGP_BUFFER,
                                 info->bufStart);

   if (info->bufNumBufs <= 0) {
      fprintf(stderr,
              "[drm] Could not create vertex/indirect buffers list\n");
      return 0;
   }
   fprintf(stderr,
           "[drm] Added %d %d byte vertex/indirect buffers\n",
           info->bufNumBufs, RADEON_BUFFER_SIZE);

   return 1;
}


/**
 * \brief Install an IRQ handler.
 *
 * \param disp display handle.
 * \param info driver private data.
 *
 * Attempts to install an IRQ handler via drmCtlInstHandler(), falling back to
 * IRQ-free operation on failure.
 */
static void RADEONDRIIrqInit(driDisplay *disp, RADEONInfoPtr info)
{
   if ((drmCtlInstHandler(disp->drmFD, 0)) != 0)
      fprintf(stderr, "[drm] failure adding irq handler, "
                 "there is a device already using that irq\n"
                 "[drm] falling back to irq-free operation\n");
}


/**
 * \brief Initialize the AGP heap.
 *
 * \param disp display handle.
 * \param info driver private data.
 *
 * This function is a wrapper around the DRM_RADEON_INIT_HEAP command, passing
 * all the parameters in a drm_radeon_mem_init_heap structure.
 */
static void RADEONDRIAgpHeapInit(driDisplay *disp,
                                 RADEONInfoPtr info)
{
   drm_radeon_mem_init_heap_t drmHeap;

   /* Start up the simple memory manager for gart space */
   drmHeap.region = RADEON_MEM_REGION_GART;
   drmHeap.start  = 0;
   drmHeap.size   = info->gartTexMapSize;

   if (drmCommandWrite(disp->drmFD, DRM_RADEON_INIT_HEAP,
                       &drmHeap, sizeof(drmHeap))) {
      fprintf(stderr,
              "[drm] Failed to initialized gart heap manager\n");
   } else {
      fprintf(stderr,
              "[drm] Initialized kernel gart heap manager, %d\n",
              info->gartTexMapSize);
   }
}

static int RADEONGetCardType(driDisplay *disp, RADEONInfoPtr info)
{
   drm_radeon_getparam_t gp;  
   int ret;
 
   gp.param = RADEON_PARAM_CARD_TYPE;
   gp.value = &disp->card_type;

   ret=drmCommandWriteRead(disp->drmFD, DRM_RADEON_GETPARAM, &gp, sizeof(gp));
   if (ret) {
     fprintf(stderr, "drm_radeon_getparam_t (RADEON_PARAM_CARD_TYPE) : %d\n", ret);
     return -1;
   }

   return disp->card_type;
}

/**
 * Called at the start of each server generation.
 *
 * \param disp display handle.
 * \param info driver private data.
 *
 * \return non-zero on success, or zero on failure.
 *
 * Performs static frame buffer allocation. Opens the DRM device and add maps
 * to the SAREA, framebuffer and MMIO regions. Fills in \p info with more
 * information. Creates a \e server context to grab the lock for the
 * initialization ioctls and calls the other initilization functions in this
 * file. Starts the CP engine via the DRM_RADEON_CP_START command.
 *
 * Setups a RADEONDRIRec structure to be passed to radeon_dri.so for its
 * initialization.
 */
static int
RADEONScreenInit( driDisplay *disp, RADEONInfoPtr info,
                  RADEONDRIPtr pRADEONDRI)
{
   int i, err;

   /* XXX this probably isn't needed here */
   {
      int  width_bytes = (disp->virtualWidth * disp->cpp);
      int  maxy        = disp->fbSize / width_bytes;

      if (maxy <= disp->virtualHeight * 3) {
         _eglLog(_EGL_WARNING,
                 "Static buffer allocation failed -- "
                 "need at least %d kB video memory (have %d kB)\n",
                 (disp->virtualWidth * disp->virtualHeight *
                  disp->cpp * 3 + 1023) / 1024,
                 disp->fbSize / 1024);
         return 0;
      }
   }

   /* Memory manager setup */
   if (!RADEONMemoryInit(disp, info)) {
      return 0;
   }

   /* Create a 'server' context so we can grab the lock for
    * initialization ioctls.
    */
   if ((err = drmCreateContext(disp->drmFD, &disp->serverContext)) != 0) {
      _eglLog(_EGL_WARNING, "%s: drmCreateContext failed %d\n",
              __FUNCTION__, err);
      return 0;
   }

   DRM_LOCK(disp->drmFD, disp->pSAREA, disp->serverContext, 0);

   /* Initialize the kernel data structures */
   if (!RADEONDRIKernelInit(disp, info)) {
      _eglLog(_EGL_WARNING, "RADEONDRIKernelInit failed\n");
      DRM_UNLOCK(disp->drmFD, disp->pSAREA, disp->serverContext);
      return 0;
   }

   /* Initialize the vertex buffers list */
   if (!RADEONDRIBufInit(disp, info)) {
      fprintf(stderr, "RADEONDRIBufInit failed\n");
      DRM_UNLOCK(disp->drmFD, disp->pSAREA, disp->serverContext);
      return 0;
   }

   /* Initialize IRQ */
   RADEONDRIIrqInit(disp, info);

   /* Initialize kernel gart memory manager */
   RADEONDRIAgpHeapInit(disp, info);

   /* Initialize the SAREA private data structure */
   {
      drm_radeon_sarea_t *pSAREAPriv;
      pSAREAPriv = (drm_radeon_sarea_t *)(((char*)disp->pSAREA) +
                                        sizeof(drm_sarea_t));
      memset(pSAREAPriv, 0, sizeof(*pSAREAPriv));
      pSAREAPriv->pfState = info->page_flip_enable;
   }

   for ( i = 0;; i++ ) {
      drmMapType type;
      drmMapFlags flags;
      drm_handle_t handle, offset;
      drmSize size;
      int rc, mtrr;

      if ( ( rc = drmGetMap( disp->drmFD, i, &offset, &size, &type, &flags, &handle, &mtrr ) ) != 0 )
         break;
      if ( type == DRM_REGISTERS ) {
         pRADEONDRI->registerHandle = offset;
         pRADEONDRI->registerSize = size;
         break;
      }
   }
   /* Quick hack to clear the front & back buffers.  Could also use
    * the clear ioctl to do this, but would need to setup hw state
    * first.
    */
   drimemsetio((char *)disp->pFB + info->frontOffset,
          0xEE,
          info->frontPitch * disp->cpp * disp->virtualHeight );

   drimemsetio((char *)disp->pFB + info->backOffset,
          0x30,
          info->backPitch * disp->cpp * disp->virtualHeight );


   /* This is the struct passed to radeon_dri.so for its initialization */
   pRADEONDRI->deviceID          = info->Chipset;
   pRADEONDRI->width             = disp->virtualWidth;
   pRADEONDRI->height            = disp->virtualHeight;
   pRADEONDRI->depth             = disp->bpp; /* XXX: depth */
   pRADEONDRI->bpp               = disp->bpp;
   pRADEONDRI->IsPCI             = (disp->card_type != RADEON_CARD_AGP);;
   pRADEONDRI->frontOffset       = info->frontOffset;
   pRADEONDRI->frontPitch        = info->frontPitch;
   pRADEONDRI->backOffset        = info->backOffset;
   pRADEONDRI->backPitch         = info->backPitch;
   pRADEONDRI->depthOffset       = info->depthOffset;
   pRADEONDRI->depthPitch        = info->depthPitch;
   pRADEONDRI->textureOffset     = info->textureOffset;
   pRADEONDRI->textureSize       = info->textureSize;
   pRADEONDRI->log2TexGran       = info->log2TexGran;
   pRADEONDRI->statusHandle      = info->ringReadPtrHandle;
   pRADEONDRI->statusSize        = info->ringReadMapSize;
   pRADEONDRI->gartTexHandle      = info->gartTexHandle;
   pRADEONDRI->gartTexMapSize     = info->gartTexMapSize;
   pRADEONDRI->log2GARTTexGran    = info->log2GARTTexGran;
   pRADEONDRI->gartTexOffset      = info->gartTexStart;
   pRADEONDRI->sarea_priv_offset = sizeof(drm_sarea_t);

   /* Don't release the lock now - let the VT switch handler do it. */

   return 1;
}


/**
 * \brief Get Radeon chip family from chipset number.
 *
 * \param info driver private data.
 *
 * \return non-zero on success, or zero on failure.
 *
 * Called by radeonInitFBDev() to set RADEONInfoRec::ChipFamily
 * according to the value of RADEONInfoRec::Chipset.  Fails if the
 * chipset is unrecognized or not appropriate for this driver (i.e., not
 * an r100 style radeon)
 */
static int get_chipfamily_from_chipset( RADEONInfoPtr info )
{
    switch (info->Chipset) {
    case PCI_CHIP_RADEON_LY:
    case PCI_CHIP_RADEON_LZ:
        info->ChipFamily = CHIP_FAMILY_M6;
        break;

    case PCI_CHIP_RADEON_QY:
    case PCI_CHIP_RADEON_QZ:
        info->ChipFamily = CHIP_FAMILY_VE;
        break;

    case PCI_CHIP_R200_QL:
    case PCI_CHIP_R200_QN:
    case PCI_CHIP_R200_QO:
    case PCI_CHIP_R200_Ql:
    case PCI_CHIP_R200_BB:
        info->ChipFamily = CHIP_FAMILY_R200;
        break;

    case PCI_CHIP_RV200_QW: /* RV200 desktop */
    case PCI_CHIP_RV200_QX:
        info->ChipFamily = CHIP_FAMILY_RV200;
        break;

    case PCI_CHIP_RADEON_LW:
    case PCI_CHIP_RADEON_LX:
        info->ChipFamily = CHIP_FAMILY_M7;
        break;

    case PCI_CHIP_RV250_Id:
    case PCI_CHIP_RV250_Ie:
    case PCI_CHIP_RV250_If:
    case PCI_CHIP_RV250_Ig:
        info->ChipFamily = CHIP_FAMILY_RV250;
        break;

    case PCI_CHIP_RV250_Ld:
    case PCI_CHIP_RV250_Le:
    case PCI_CHIP_RV250_Lf:
    case PCI_CHIP_RV250_Lg:
        info->ChipFamily = CHIP_FAMILY_M9;
        break;

    case PCI_CHIP_RV280_Y_:
    case PCI_CHIP_RV280_Ya:
    case PCI_CHIP_RV280_Yb:
    case PCI_CHIP_RV280_Yc:
        info->ChipFamily = CHIP_FAMILY_RV280;
        break;

    case PCI_CHIP_R300_ND:
    case PCI_CHIP_R300_NE:
    case PCI_CHIP_R300_NF:
    case PCI_CHIP_R300_NG:
        info->ChipFamily = CHIP_FAMILY_R300;
        break;

    case PCI_CHIP_RV370_5460:
        info->ChipFamily = CHIP_FAMILY_RV380;
	break;

    default:
        /* Original Radeon/7200 */
        info->ChipFamily = CHIP_FAMILY_RADEON;
    }

    return 1;
}


/**
 * \brief Initialize the framebuffer device mode
 *
 * \param disp display handle.
 *
 * \return one on success, or zero on failure.
 *
 * Fills in \p info with some default values and some information from \p disp
 * and then calls RADEONScreenInit() for the screen initialization.
 *
 * Before exiting clears the framebuffer memory accessing it directly.
 */
static int radeonInitFBDev( driDisplay *disp, RADEONDRIPtr pRADEONDRI )
{
   int err;
   RADEONInfoPtr info = calloc(1, sizeof(*info));

   disp->driverPrivate = (void *)info;

   info->gartFastWrite  = RADEON_DEFAULT_AGP_FAST_WRITE;
   info->gartSize       = RADEON_DEFAULT_AGP_SIZE;
   info->gartTexSize    = RADEON_DEFAULT_AGP_TEX_SIZE;
   info->bufSize       = RADEON_DEFAULT_BUFFER_SIZE;
   info->ringSize      = RADEON_DEFAULT_RING_SIZE;
   info->page_flip_enable = RADEON_DEFAULT_PAGE_FLIP;

   fprintf(stderr,
           "Using %d MB AGP aperture\n", info->gartSize);
   fprintf(stderr,
           "Using %d MB for the ring buffer\n", info->ringSize);
   fprintf(stderr,
           "Using %d MB for vertex/indirect buffers\n", info->bufSize);
   fprintf(stderr,
           "Using %d MB for AGP textures\n", info->gartTexSize);
   fprintf(stderr,
           "page flipping %sabled\n", info->page_flip_enable?"en":"dis");

   info->Chipset = disp->chipset;

   if (!get_chipfamily_from_chipset( info )) {
      fprintf(stderr, "Unknown or non-radeon chipset -- cannot continue\n");
      fprintf(stderr, "==> Verify PCI BusID is correct in miniglx.conf\n");
      return 0;
   }
#if 0
   if (info->ChipFamily >= CHIP_FAMILY_R300) {
      fprintf(stderr,
              "Direct rendering not yet supported on "
              "Radeon 9700 and newer cards\n");
      return 0;
   }
#endif

#if 00
   /* don't seem to need this here */
   info->frontPitch = disp->virtualWidth;
#endif

   /* Check the radeon DRM version */
   if (!RADEONCheckDRMVersion(disp, info)) {
      return 0;
   }

   if (RADEONGetCardType(disp, info)<0)
      return 0;

   if (disp->card_type!=RADEON_CARD_AGP) {
      /* Initialize PCI */
      if (!RADEONDRIPciInit(disp, info))
         return 0;
   }
   else {
      /* Initialize AGP */
      if (!RADEONDRIAgpInit(disp, info))
         return 0;
   }

   if (!RADEONScreenInit( disp, info, pRADEONDRI))
      return 0;

   /* Initialize and start the CP if required */
   if ((err = drmCommandNone(disp->drmFD, DRM_RADEON_CP_START)) != 0) {
      fprintf(stderr, "%s: CP start %d\n", __FUNCTION__, err);
      return 0;
   }

   return 1;
}


/**
 * Create list of all supported surface configs, attach list to the display.
 */
static EGLBoolean
radeonFillInConfigs(_EGLDisplay *disp, unsigned pixel_bits,
                    unsigned depth_bits,
                    unsigned stencil_bits, GLboolean have_back_buffer)
{
   _EGLConfig *configs;
   _EGLConfig *c;
   unsigned int i, num_configs;
   unsigned int depth_buffer_factor;
   unsigned int back_buffer_factor;
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
   back_buffer_factor = (have_back_buffer) ? 2 : 1;

   num_configs = depth_buffer_factor * back_buffer_factor * 2;

   if (pixel_bits == 16) {
      fb_format = GL_RGB;
      fb_type = GL_UNSIGNED_SHORT_5_6_5;
   } else {
      fb_format = GL_RGBA;
      fb_type = GL_UNSIGNED_INT_8_8_8_8_REV;
   }

   configs = calloc(sizeof(*configs), num_configs);
   c = configs;
   if (!_eglFillInConfigs(c, fb_format, fb_type,
                          depth_bits_array, stencil_bits_array,
                          depth_buffer_factor,
                          back_buffer_modes, back_buffer_factor,
                          GLX_TRUE_COLOR)) {
      fprintf(stderr, "[%s:%u] Error creating FBConfig!\n",
               __func__, __LINE__);
      return EGL_FALSE;
   }

   /* Mark the visual as slow if there are "fake" stencil bits.
   */
   for (i = 0, c = configs; i < num_configs; i++, c++) {
      int stencil = GET_CONFIG_ATTRIB(c, EGL_STENCIL_SIZE);
      if ((stencil != 0)  && (stencil != stencil_bits)) {
         SET_CONFIG_ATTRIB(c, EGL_CONFIG_CAVEAT, EGL_SLOW_CONFIG);
      }
   }

   for (i = 0, c = configs; i < num_configs; i++, c++)
      _eglAddConfig(disp, c);

   free(configs);

   return EGL_TRUE;
}


/**
 * Show the given surface on the named screen.
 * If surface is EGL_NO_SURFACE, disable the screen's output.
 */
static EGLBoolean
radeonShowScreenSurfaceMESA(_EGLDriver *drv, EGLDisplay dpy, EGLScreenMESA screen,
                      EGLSurface surface, EGLModeMESA m)
{
   EGLBoolean b = _eglDRIShowScreenSurfaceMESA(drv, dpy, screen, surface, m);
   return b;
}


/**
 * Called via eglInitialize() by user.
 */
static EGLBoolean
radeonInitialize(_EGLDriver *drv, EGLDisplay dpy, EGLint *major, EGLint *minor)
{
   __DRIframebuffer framebuffer;
   driDisplay *display;

   /* one-time init */
   radeon_drm_page_size = getpagesize();

   if (!_eglDRIInitialize(drv, dpy, major, minor))
      return EGL_FALSE;

   display = Lookup_driDisplay(dpy);

   framebuffer.dev_priv_size = sizeof(RADEONDRIRec);
   framebuffer.dev_priv = malloc(sizeof(RADEONDRIRec));

   /* XXX we shouldn't hard-code values here! */
   /* we won't know the screen surface size until the user calls
    * eglCreateScreenSurfaceMESA().
    */
#if 0
   display->virtualWidth = 1024;
   display->virtualHeight = 768;
#else
   display->virtualWidth = 1280;
   display->virtualHeight = 1024;
#endif
   display->bpp = 32;
   display->cpp = 4;

   if (!_eglDRIGetDisplayInfo(display))
      return EGL_FALSE;

   framebuffer.base = display->pFB;
   framebuffer.width = display->virtualWidth;
   framebuffer.height = display->virtualHeight;
   framebuffer.stride = display->virtualWidth;
   framebuffer.size = display->fbSize;
   radeonInitFBDev( display, framebuffer.dev_priv );

   if (!_eglDRICreateDisplay(display, &framebuffer))
      return EGL_FALSE;

   if (!_eglDRICreateScreens(display))
      return EGL_FALSE;

   /* create a variety of both 32 and 16-bit configurations */
   radeonFillInConfigs(&display->Base, 32, 24, 8, GL_TRUE);
   radeonFillInConfigs(&display->Base, 16, 16, 0, GL_TRUE);

   drv->Initialized = EGL_TRUE;
   return EGL_TRUE;
}


/**
 * The bootstrap function.  Return a new radeonDriver object and
 * plug in API functions.
 */
_EGLDriver *
_eglMain(_EGLDisplay *dpy)
{
   radeonDriver *radeon;

   radeon = (radeonDriver *) calloc(1, sizeof(*radeon));
   if (!radeon) {
      return NULL;
   }

   /* First fill in the dispatch table with defaults */
   _eglDRIInitDriverFallbacks(&radeon->Base);

   /* then plug in our radeon-specific functions */
   radeon->Base.API.Initialize = radeonInitialize;
   radeon->Base.API.ShowScreenSurfaceMESA = radeonShowScreenSurfaceMESA;

   return &radeon->Base;
}
