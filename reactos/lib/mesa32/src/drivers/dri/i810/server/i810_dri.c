/**
 * \file server/i810_dri.c
 * \brief File to perform the device-specific initialization tasks typically
 * done in the X server.
 *
 * Here they are converted to run in the client (or perhaps a standalone
 * process), and to work with the frame buffer device rather than the X
 * server infrastructure.
 * 
 * Copyright (C) 2004 Dave Airlie (airlied@linux.ie)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include "driver.h"
#include "drm.h"

#include "i810.h"
#include "i810_dri.h"
#include "i810_reg.h"


static int i810_pitches[] = {
   512,
   1024,
   2048,
   4096,
   0
};

static int i810_pitch_flags[] = {
   0x0,
   0x1,
   0x2,
   0x3,
   0
};

static unsigned int i810_drm_version = 0;

static int
I810AllocLow(I810MemRange * result, I810MemRange * pool, int size)
{
   if (size > pool->Size)
      return 0;

   pool->Size -= size;
   result->Size = size;
   result->Start = pool->Start;
   result->End = pool->Start += size;

   return 1;
}

static int
I810AllocHigh(I810MemRange * result, I810MemRange * pool, int size)
{
   if (size > pool->Size)
      return 0;

   pool->Size -= size;
   result->Size = size;
   result->End = pool->End;
   result->Start = pool->End -= size;

   return 1;
}


/**
 * \brief Wait for free FIFO entries.
 *
 * \param ctx display handle.
 * \param entries number of free entries to wait.
 *
 * It polls the free entries from the chip until it reaches the requested value
 * or a timeout (3000 tries) occurs. Aborts the program if the FIFO times out.
 */
static void I810WaitForFifo( const DRIDriverContext *ctx,
			       int entries )
{
}

/**
 * \brief Reset graphics card to known state.
 *
 * \param ctx display handle.
 *
 * Resets the values of several I810 registers.
 */
static void I810EngineReset( const DRIDriverContext *ctx )
{
   unsigned char *I810MMIO = ctx->MMIOAddress;
}

/**
 * \brief Restore the drawing engine.
 *
 * \param ctx display handle
 *
 * Resets the graphics card and sets initial values for several registers of
 * the card's drawing engine.
 *
 * Turns on the i810 command processor engine (i.e., the ringbuffer).
 */
static int I810EngineRestore( const DRIDriverContext *ctx )
{
   I810Ptr info = ctx->driverPrivate;
   unsigned char *I810MMIO = ctx->MMIOAddress;

   fprintf(stderr, "%s\n", __FUNCTION__);

   return 1;
}


/**
 * \brief Shutdown the drawing engine.
 *
 * \param ctx display handle
 *
 * Turns off the command processor engine & restores the graphics card
 * to a state that fbdev understands.
 */
static int I810EngineShutdown( const DRIDriverContext *ctx )
{
  drmI810Init info;
  int ret;

  memset(&info, 0, sizeof(drmI810Init));
  info.func = I810_CLEANUP_DMA;
  
  ret = drmCommandWrite(ctx->drmFD, DRM_I810_INIT, &info, sizeof(drmI810Init));
  if (ret>0)
  {
    fprintf(stderr,"[dri] I810 DMA Cleanup failed\n");
    return -errno;
  }
  return 0;
}

/**
 * \brief Compute base 2 logarithm.
 *
 * \param val value.
 * 
 * \return base 2 logarithm of \p val.
 */
static int I810MinBits(int val)
{
   int  bits;

   if (!val) return 1;
   for (bits = 0; val; val >>= 1, ++bits);
   return bits;
}

static int I810DRIAgpPreInit( const DRIDriverContext *ctx, I810Ptr info)
{

  if (drmAgpAcquire(ctx->drmFD) < 0) {
    fprintf(stderr, "[gart] AGP not available\n");
    return 0;
  }
  
  
  if (drmAgpEnable(ctx->drmFD, 0) < 0) {
    fprintf(stderr, "[gart] AGP not enabled\n");
    drmAgpRelease(ctx->drmFD);
    return 0;
  }
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
 * the ring buffer, vertex buffers and textures. Initialize the I810
 * registers to point to that memory and add client mappings.
 */
static int I810DRIAgpInit( const DRIDriverContext *ctx, I810Ptr info)
{
   unsigned char *I810MMIO = ctx->MMIOAddress;
   int            ret;
   int            s, l;
   unsigned long dcacheHandle;
   unsigned long agpHandle;
   int pitch_idx = 0;
   int back_size = 0;
   int sysmem_size = 0;
   int width = ctx->shared.virtualWidth * ctx->cpp;


   info->backHandle = DRM_AGP_NO_HANDLE;
   info->zHandle = DRM_AGP_NO_HANDLE;
   info->sysmemHandle = DRM_AGP_NO_HANDLE;
   info->dcacheHandle = DRM_AGP_NO_HANDLE;

   memset(&info->DcacheMem, 0, sizeof(I810MemRange));
   memset(&info->BackBuffer, 0, sizeof(I810MemRange));
   memset(&info->DepthBuffer, 0, sizeof(I810MemRange));
   
   drmAgpAlloc(ctx->drmFD, 4096 * 1024, 1, NULL, &dcacheHandle);
   info->dcacheHandle = dcacheHandle;
   
   fprintf(stderr, "[agp] dcacheHandle : 0x%x\n", dcacheHandle);

#define Elements(x) sizeof(x)/sizeof(*x)
   for (pitch_idx = 0; pitch_idx < Elements(i810_pitches); pitch_idx++)
     if (width <= i810_pitches[pitch_idx])
       break;
   
   if (pitch_idx == Elements(i810_pitches)) {
     fprintf(stderr,"[dri] Couldn't find depth/back buffer pitch\n");
     exit(-1);
   }
   else
   {
     int lines = (ctx->shared.virtualWidth + 15) / 16 * 16;
     back_size = i810_pitches[pitch_idx] * lines;
     back_size = ((back_size + 4096 - 1) / 4096) * 4096;
   }

   sysmem_size = ctx->shared.fbSize;
   fprintf(stderr,"sysmem_size is %lu back_size is %lu\n", sysmem_size, back_size);
   if (dcacheHandle != DRM_AGP_NO_HANDLE) {
     if (back_size > 4 * 1024 * 1024) {
       fprintf(stderr,"[dri] Backsize is larger then 4 meg\n");
       sysmem_size = sysmem_size - 2 * back_size;
       drmAgpFree(ctx->drmFD, dcacheHandle);
       info->dcacheHandle = dcacheHandle = DRM_AGP_NO_HANDLE;
     } else {
       sysmem_size = sysmem_size - back_size;
     }
   } else {
     sysmem_size = sysmem_size - 2 * back_size;
   }
   
   info->SysMem.Start=0;
   info->SysMem.Size = sysmem_size;
   info->SysMem.End = sysmem_size;
   
   if (dcacheHandle != DRM_AGP_NO_HANDLE) {
      if (drmAgpBind(ctx->drmFD, dcacheHandle, info->DepthOffset) == 0) {
	memset(&info->DcacheMem, 0, sizeof(I810MemRange));
	fprintf(stderr,"[agp] GART: Found 4096K Z buffer memory\n");
	info->DcacheMem.Start = info->DepthOffset;
	 info->DcacheMem.Size = 1024 * 4096;
	 info->DcacheMem.End =  info->DcacheMem.Start + info->DcacheMem.Size;
      } else {
	fprintf(stderr, "[agp] GART: dcache bind failed\n");
	drmAgpFree(ctx->drmFD, dcacheHandle);
	info->dcacheHandle = dcacheHandle = DRM_AGP_NO_HANDLE;
      }
   } else {
     fprintf(stderr, "[agp] GART: no dcache memory found\n");
   }
   
   drmAgpAlloc(ctx->drmFD, back_size, 0, NULL, &agpHandle);
   info->backHandle = agpHandle;

   if (agpHandle != DRM_AGP_NO_HANDLE) {
      if (drmAgpBind(ctx->drmFD, agpHandle, info->BackOffset) == 0) {
	fprintf(stderr, "[agp] Bound backbuffer memory\n");

	info->BackBuffer.Start = info->BackOffset;
	info->BackBuffer.Size = back_size;
	info->BackBuffer.End = (info->BackBuffer.Start +
				 info->BackBuffer.Size);
      } else {
	fprintf(stderr,"[agp] Unable to bind backbuffer.  Disabling DRI.\n");
	return 0;
      }
   } else {
     fprintf(stderr, "[dri] Unable to allocate backbuffer memory.  Disabling DRI.\n");
     return 0;
   }

   if (dcacheHandle == DRM_AGP_NO_HANDLE) {
     drmAgpAlloc(ctx->drmFD, back_size, 0, NULL, &agpHandle);

     info->zHandle = agpHandle;

     if (agpHandle != DRM_AGP_NO_HANDLE) {
       if (drmAgpBind(ctx->drmFD, agpHandle, info->DepthOffset) == 0) {
	 fprintf(stderr,"[agp] Bound depthbuffer memory\n");
	 info->DepthBuffer.Start = info->DepthOffset;
	 info->DepthBuffer.Size = back_size;
	 info->DepthBuffer.End = (info->DepthBuffer.Start +
				   info->DepthBuffer.Size);
       } else {
	 fprintf(stderr,"[agp] Unable to bind depthbuffer.  Disabling DRI.\n");
	 return 0;
       }
     } else {
       fprintf(stderr,"[agp] Unable to allocate depthbuffer memory.  Disabling DRI.\n");
       return 0;
     }
   }

   /* Now allocate and bind the agp space.  This memory will include the
    * regular framebuffer as well as texture memory.
    */
   drmAgpAlloc(ctx->drmFD, sysmem_size, 0, NULL, &agpHandle);
   info->sysmemHandle = agpHandle;
   
   if (agpHandle != DRM_AGP_NO_HANDLE) {
     if (drmAgpBind(ctx->drmFD, agpHandle, 0) == 0) {
       fprintf(stderr, "[agp] Bound System Texture Memory\n");
     } else {
       fprintf(stderr, "[agp] Unable to bind system texture memory. Disabling DRI.\n");
       return 0;
     }
   } else {
     fprintf(stderr, "[agp] Unable to allocate system texture memory. Disabling DRI.\n");
     return 0;
   }
   
   info->auxPitch = i810_pitches[pitch_idx];
   info->auxPitchBits = i810_pitch_flags[pitch_idx];
   
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
 * This function is a wrapper around the DRM_I810_CP_INIT command, passing
 * all the parameters in a drmI810Init structure.
 */
static int I810DRIKernelInit( const DRIDriverContext *ctx,
			       I810Ptr info)
{
   int cpp = ctx->bpp / 8;
   drmI810Init  drmInfo;
   int ret;
   I810RingBuffer *ring = &(info->LpRing);

   /* This is the struct passed to the kernel module for its initialization */
   memset(&drmInfo, 0, sizeof(drmI810Init));
   
   /* make sure we have at least 1.4 */
   drmInfo.func             = I810_INIT_DMA_1_4;

   drmInfo.ring_start = ring->mem.Start;
   drmInfo.ring_end = ring->mem.End;
   drmInfo.ring_size = ring->mem.Size;

   drmInfo.mmio_offset         = (unsigned int)info->regs;
   drmInfo.buffers_offset      = (unsigned int)info->buffer_map;
   drmInfo.sarea_priv_offset   = sizeof(drm_sarea_t);

   drmInfo.front_offset        = 0;
   drmInfo.back_offset         = info->BackBuffer.Start;
   drmInfo.depth_offset        = info->DepthBuffer.Start;

   drmInfo.w                   = ctx->shared.virtualWidth;
   drmInfo.h                   = ctx->shared.virtualHeight;
   drmInfo.pitch               = info->auxPitch;
   drmInfo.pitch_bits          = info->auxPitchBits;
   

   ret = drmCommandWrite(ctx->drmFD, DRM_I810_INIT, &drmInfo, 
			 sizeof(drmI810Init));

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
static int I810DRIBufInit( const DRIDriverContext *ctx, I810Ptr info )
{
   /* Initialize vertex buffers */
   info->bufNumBufs = drmAddBufs(ctx->drmFD,
				 I810_DMA_BUF_NR,
				 I810_DMA_BUF_SZ,
				 DRM_AGP_BUFFER,
				 info->BufferMem.Start);

   if (info->bufNumBufs <= 0) {
      fprintf(stderr,
	      "[drm] Could not create vertex/indirect buffers list\n");
      return 0;
   }
   fprintf(stderr,
	   "[drm] Added %d %d byte vertex/indirect buffers\n",
	   info->bufNumBufs, I810_DMA_BUF_SZ);
   
   return 1;
}

/**
 * \brief Install an IRQ handler.
 * 
 * \param ctx display handle.
 * \param info driver private data.
 *
 * Attempts to install an IRQ handler via drmCtlInstHandler(), falling back to
 * IRQ-free operation on failure.
 */
static void I810DRIIrqInit(const DRIDriverContext *ctx,
			     I810Ptr info)
{
   if (!info->irq) {
      info->irq = drmGetInterruptFromBusID(ctx->drmFD,
					   ctx->pciBus,
					   ctx->pciDevice,
					   ctx->pciFunc);

      if ((drmCtlInstHandler(ctx->drmFD, info->irq)) != 0) {
	 fprintf(stderr,
		 "[drm] failure adding irq handler, "
		 "there is a device already using that irq\n"
		 "[drm] falling back to irq-free operation\n");
	 info->irq = 0;
      }
   }

   if (info->irq)
      fprintf(stderr,
	      "[drm] dma control initialized, using IRQ %d\n",
	      info->irq);
}

static int I810CheckDRMVersion( const DRIDriverContext *ctx,
				  I810Ptr info )
{
   drmVersionPtr  version;

   version = drmGetVersion(ctx->drmFD);
   if (version) {
      int req_minor, req_patch;

      req_minor = 4;
      req_patch = 0;	

      i810_drm_version = (version->version_major<<16) | version->version_minor;
      if (version->version_major != 1 ||
	  version->version_minor < req_minor ||
	  (version->version_minor == req_minor && 
	   version->version_patchlevel < req_patch)) {
	 /* Incompatible drm version */
	 fprintf(stderr,
		 "[dri] I810DRIScreenInit failed because of a version "
		 "mismatch.\n"
		 "[dri] i810.o kernel module version is %d.%d.%d "
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

static int I810MemoryInit( const DRIDriverContext *ctx, I810Ptr info )
{
   int        width_bytes = ctx->shared.virtualWidth * ctx->cpp;
   int        cpp         = ctx->cpp;
   int        bufferSize  = (ctx->shared.virtualHeight * width_bytes);
   int        depthSize   = (((ctx->shared.virtualHeight+15) & ~15) * width_bytes);
   int        l;

   if (drmAddMap(ctx->drmFD, (drm_handle_t) info->BackBuffer.Start,
		 info->BackBuffer.Size, DRM_AGP, 0,
		 &info->backbuffer) < 0) {
     fprintf(stderr, "[drm] drmAddMap(backbuffer) failed.  Disabling DRI\n");
     return 0;
   }
   
   if (drmAddMap(ctx->drmFD, (drm_handle_t) info->DepthBuffer.Start,
		 info->DepthBuffer.Size, DRM_AGP, 0,
		 &info->depthbuffer) < 0) {
     fprintf(stderr, "[drm] drmAddMap(depthbuffer) failed.  Disabling DRI.\n");
      return 0;
   }

   if (!I810AllocLow(&(info->FrontBuffer), &(info->SysMem), (((ctx->shared.virtualHeight * width_bytes) + 4095) & ~4095)))
   {
     fprintf(stderr,"Framebuffer allocation failed\n");
     return 0;
   }
   else
     fprintf(stderr,"Frame buffer at 0x%.8x (%luk, %lu bytes)\n",
	     info->FrontBuffer.Start,
	     info->FrontBuffer.Size / 1024, info->FrontBuffer.Size);
   
   memset(&(info->LpRing), 0, sizeof(I810RingBuffer));
   if (I810AllocLow(&(info->LpRing.mem), &(info->SysMem), 16 * 4096)) {
     fprintf(stderr,
	    "Ring buffer at 0x%.8x (%luk, %lu bytes)\n",
	     info->LpRing.mem.Start,
	     info->LpRing.mem.Size / 1024, info->LpRing.mem.Size);
     
     info->LpRing.tail_mask = info->LpRing.mem.Size - 1;
     info->LpRing.virtual_start = info->LpRing.mem.Start;
     info->LpRing.head = 0;
     info->LpRing.tail = 0;
     info->LpRing.space = 0;
   } else {
     fprintf(stderr, "Ring buffer allocation failed\n");
     return (0);
   }

   /* Allocate buffer memory */
   I810AllocHigh(&(info->BufferMem), &(info->SysMem),
		 I810_DMA_BUF_NR * I810_DMA_BUF_SZ);
   

   fprintf(stderr, "[dri] Buffer map : %lx\n",
	      info->BufferMem.Start);

   if (info->BufferMem.Start == 0 ||
       info->BufferMem.End - info->BufferMem.Start >
       I810_DMA_BUF_NR * I810_DMA_BUF_SZ) {
     fprintf(stderr,"[dri] Not enough memory for dma buffers.  Disabling DRI.\n");
     return 0;
   }

   if (drmAddMap(ctx->drmFD, (drm_handle_t) info->BufferMem.Start,
		 info->BufferMem.Size, DRM_AGP, 0, &info->buffer_map) < 0) {
     fprintf(stderr, "[drm] drmAddMap(buffer_map) failed.  Disabling DRI.\n");
     return 0;
   }

   if (drmAddMap(ctx->drmFD, (drm_handle_t) info->LpRing.mem.Start,
		 info->LpRing.mem.Size, DRM_AGP, 0, &info->ring_map) < 0) {
     fprintf(stderr, "[drm] drmAddMap(ring_map) failed.  Disabling DRI. \n");
     return 0;
   }

   /* Front, back and depth buffers - everything else texture??
    */
   info->textureSize = info->SysMem.Size;

   if (info->textureSize < 0) 
      return 0;

   
   l = I810MinBits((info->textureSize-1) / I810_NR_TEX_REGIONS);
   if (l < I810_LOG_MIN_TEX_REGION_SIZE) l = I810_LOG_MIN_TEX_REGION_SIZE;

   /* Round the texture size up to the nearest whole number of
    * texture regions.  Again, be greedy about this, don't
    * round down.
    */
   info->logTextureGranularity = l;
   info->textureSize = (info->textureSize >> l) << l;

   /* Set a minimum usable local texture heap size.  This will fit
    * two 256x256x32bpp textures.
    */
   if (info->textureSize < 512 * 1024) {
      info->textureOffset = 0;
      info->textureSize = 0;
   }

   I810AllocLow(&(info->TexMem), &(info->SysMem), info->textureSize);

   if (drmAddMap(ctx->drmFD, (drm_handle_t) info->TexMem.Start,
		 info->TexMem.Size, DRM_AGP, 0, &info->textures) < 0) {
     fprintf(stderr,
		 "[drm] drmAddMap(textures) failed.  Disabling DRI.\n");
      return 0;
   }

   /* Reserve space for textures */
   fprintf(stderr, 
	   "Will use back buffer at offset 0x%x\n",
	   info->BackOffset);
   fprintf(stderr, 
	   "Will use depth buffer at offset 0x%x\n",
	   info->DepthOffset);
   fprintf(stderr, 
	   "Will use %d kb for textures at offset 0x%x\n",
	   info->TexMem.Size/1024, info->TexMem.Start);

   return 1;
} 



/**
 * Called at the start of each server generation.
 *
 * \param ctx display handle.
 * \param info driver private data.
 *
 * \return non-zero on success, or zero on failure.
 *
 * Performs static frame buffer allocation. Opens the DRM device and add maps
 * to the SAREA, framebuffer and MMIO regions. Fills in \p info with more
 * information. Creates a \e server context to grab the lock for the
 * initialization ioctls and calls the other initilization functions in this
 * file. Starts the CP engine via the DRM_I810_CP_START command.
 *
 * Setups a I810DRIRec structure to be passed to i810_dri.so for its
 * initialization.
 */
static int I810ScreenInit( DRIDriverContext *ctx, I810Ptr info )
{
   I810DRIPtr   pI810DRI;
   int err;

   usleep(100);
   /*assert(!ctx->IsClient);*/

   /* from XFree86 driver */
   info->DepthOffset = 0x3000000;
   info->BackOffset = 0x3800000;
   {
      int  width_bytes = (ctx->shared.virtualWidth * ctx->cpp);
      int  maxy        = ctx->shared.fbSize / width_bytes;


      if (maxy <= ctx->shared.virtualHeight * 3) {
	 fprintf(stderr, 
		 "Static buffer allocation failed -- "
		 "need at least %d kB video memory (have %d kB)\n",
		 (ctx->shared.virtualWidth * ctx->shared.virtualHeight *
		  ctx->cpp * 3 + 1023) / 1024,
		 ctx->shared.fbSize / 1024);
	 return 0;
      } 
   }


   info->regsSize = ctx->MMIOSize;
   ctx->shared.SAREASize = 0x2000;

   /* Note that drmOpen will try to load the kernel module, if needed. */
   ctx->drmFD = drmOpen("i810", NULL );
   if (ctx->drmFD < 0) {
      fprintf(stderr, "[drm] drmOpen failed\n");
      return 0;
   }

   if ((err = drmSetBusid(ctx->drmFD, ctx->pciBusID)) < 0) {
      fprintf(stderr, "[drm] drmSetBusid failed (%d, %s), %s\n",
	      ctx->drmFD, ctx->pciBusID, strerror(-err));
      return 0;
   }

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
	   ctx->shared.SAREASize, ctx->shared.hSAREA);

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
	   ctx->shared.hSAREA, ctx->pSAREA, ctx->shared.SAREASize);

   if (drmAddMap(ctx->drmFD, 
		 ctx->MMIOStart,
		 ctx->MMIOSize,
		 DRM_REGISTERS, 
		 DRM_READ_ONLY, 
		 &info->regs) < 0) {
      fprintf(stderr, "[drm] drmAddMap mmio failed\n");	
      return 0;
   }
   fprintf(stderr,
	   "[drm] register handle = 0x%08x\n", info->regs);

   I810DRIAgpPreInit(ctx, info);
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
	   ctx->shared.hFrameBuffer);

   /* Check the i810 DRM version */
   if (!I810CheckDRMVersion(ctx, info)) {
      return 0;
   }

   /* Initialize AGP */
   if (!I810DRIAgpInit(ctx, info)) {
      return 0;
   }


   /* Memory manager setup */
   if (!I810MemoryInit(ctx, info)) {
      return 0;
   }

   /* Initialize the SAREA private data structure */
   {
      I810SAREAPtr pSAREAPriv;
      pSAREAPriv = (I810SAREAPtr)(((char*)ctx->pSAREA) + 
					sizeof(drm_sarea_t));
      memset(pSAREAPriv, 0, sizeof(*pSAREAPriv));
      //      pSAREAPriv->pf_enabled=1;
   }


   /* Create a 'server' context so we can grab the lock for
    * initialization ioctls.
    */
   if ((err = drmCreateContext(ctx->drmFD, &ctx->serverContext)) != 0) {
      fprintf(stderr, "%s: drmCreateContext failed %d\n", __FUNCTION__, err);
      return 0;
   }

   DRM_LOCK(ctx->drmFD, ctx->pSAREA, ctx->serverContext, 0); 

   /* Initialize the vertex buffers list */
   if (!I810DRIBufInit(ctx, info)) {
      fprintf(stderr, "I810DRIBufInit failed\n");
      DRM_UNLOCK(ctx->drmFD, ctx->pSAREA, ctx->serverContext);
      return 0;
   }

   /* Initialize the kernel data structures */
   if (!I810DRIKernelInit(ctx, info)) {
      fprintf(stderr, "I810DRIKernelInit failed\n");
      DRM_UNLOCK(ctx->drmFD, ctx->pSAREA, ctx->serverContext);
      return 0;
   }

   /* Initialize IRQ */
   I810DRIIrqInit(ctx, info);

   /* Quick hack to clear the front & back buffers.  Could also use
    * the clear ioctl to do this, but would need to setup hw state
    * first.
    */
#if 0
   memset((char *)ctx->FBAddress,
	  0,
	  info->auxPitch * ctx->cpp * ctx->shared.virtualHeight );

   memset((char *)info->backbuffer,
	  0,
	  info->auxPitch * ctx->cpp * ctx->shared.virtualHeight );
#endif

   /* This is the struct passed to i810_dri.so for its initialization */
   ctx->driverClientMsg = malloc(sizeof(I810DRIRec));
   ctx->driverClientMsgSize = sizeof(I810DRIRec);
   pI810DRI                    = (I810DRIPtr)ctx->driverClientMsg;

   pI810DRI->regs              = info->regs;
   pI810DRI->regsSize          = info->regsSize;
   // regsMap is unused

   pI810DRI->backbufferSize    = info->BackBuffer.Size;
   pI810DRI->backbuffer        = info->backbuffer;

   pI810DRI->depthbufferSize   = info->DepthBuffer.Size;
   pI810DRI->depthbuffer       = info->depthbuffer;

   pI810DRI->textures          = info->textures;
   pI810DRI->textureSize       = info->textureSize;

   pI810DRI->agp_buffers       = info->buffer_map;
   pI810DRI->agp_buf_size      = info->BufferMem.Size;

   pI810DRI->deviceID          = info->Chipset;
   pI810DRI->width             = ctx->shared.virtualWidth;
   pI810DRI->height            = ctx->shared.virtualHeight;
   pI810DRI->mem               = ctx->shared.fbSize;
   pI810DRI->cpp               = ctx->bpp / 8;
   pI810DRI->bitsPerPixel      = ctx->bpp;
   pI810DRI->fbOffset          = info->FrontBuffer.Start;
   pI810DRI->fbStride          = info->auxPitch;
   
   pI810DRI->backOffset        = info->BackBuffer.Start;
   pI810DRI->depthOffset       = info->DepthBuffer.Start;

   pI810DRI->auxPitch          = info->auxPitch;
   pI810DRI->auxPitchBits      = info->auxPitchBits;

   pI810DRI->logTextureGranularity = info->logTextureGranularity;
   pI810DRI->textureOffset     = info->TexMem.Start;
  
   pI810DRI->ringOffset        = info->LpRing.mem.Start;
   pI810DRI->ringSize          = info->LpRing.mem.Size;

   // drmBufs looks unused 
   pI810DRI->irq               = info->irq;
   pI810DRI->sarea_priv_offset = sizeof(drm_sarea_t);

   /* Don't release the lock now - let the VT switch handler do it. */
   return 1;
}


/**
 * \brief Validate the fbdev mode.
 * 
 * \param ctx display handle.
 *
 * \return one on success, or zero on failure.
 *
 * Saves some registers and returns 1.
 *
 * \sa i810ValidateMode().
 */
static int i810ValidateMode( const DRIDriverContext *ctx )
{
   unsigned char *I810MMIO = ctx->MMIOAddress;
   I810Ptr info = ctx->driverPrivate;

   return 1;
}


/**
 * \brief Examine mode returned by fbdev.
 * 
 * \param ctx display handle.
 *
 * \return one on success, or zero on failure.
 *
 * Restores registers that fbdev has clobbered and returns 1.
 *
 * \sa i810ValidateMode().
 */
static int i810PostValidateMode( const DRIDriverContext *ctx )
{
   unsigned char *I810MMIO = ctx->MMIOAddress;
   I810Ptr info = ctx->driverPrivate;

   return 1;
}


/**
 * \brief Initialize the framebuffer device mode
 *
 * \param ctx display handle.
 *
 * \return one on success, or zero on failure.
 *
 * Fills in \p info with some default values and some information from \p ctx
 * and then calls I810ScreenInit() for the screen initialization.
 * 
 * Before exiting clears the framebuffer memory accessing it directly.
 */
static int i810InitFBDev( DRIDriverContext *ctx )
{
  I810Ptr info = calloc(1, sizeof(*info));

   {
      int  dummy = ctx->shared.virtualWidth;

      switch (ctx->bpp / 8) {
      case 1: dummy = (ctx->shared.virtualWidth + 127) & ~127; break;
      case 2: dummy = (ctx->shared.virtualWidth +  31) &  ~31; break;
      case 3:
      case 4: dummy = (ctx->shared.virtualWidth +  15) &  ~15; break;
      }

      ctx->shared.virtualWidth = dummy;
   }

   ctx->driverPrivate = (void *)info;
   
   info->Chipset = ctx->chipset;

   if (!I810ScreenInit( ctx, info ))
      return 0;


   return 1;
}


/**
 * \brief The screen is being closed, so clean up any state and free any
 * resources used by the DRI.
 *
 * \param ctx display handle.
 *
 * Unmaps the SAREA, closes the DRM device file descriptor and frees the driver
 * private data.
 */
static void i810HaltFBDev( DRIDriverContext *ctx )
{
    drmUnmap( ctx->pSAREA, ctx->shared.SAREASize );
    drmClose(ctx->drmFD);

    if (ctx->driverPrivate) {
       free(ctx->driverPrivate);
       ctx->driverPrivate = 0;
    }
}


extern void i810NotifyFocus( int );

/**
 * \brief Exported driver interface for Mini GLX.
 *
 * \sa DRIDriverRec.
 */
const struct DRIDriverRec __driDriver = {
   i810ValidateMode,
   i810PostValidateMode,
   i810InitFBDev,
   i810HaltFBDev,
   I810EngineShutdown,
   I810EngineRestore,  
#ifndef _EMBEDDED
   0,
#else
   i810NotifyFocus, 
#endif
};
