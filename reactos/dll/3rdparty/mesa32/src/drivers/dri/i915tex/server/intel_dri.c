/**
 * \file server/intel_dri.c
 * \brief File to perform the device-specific initialization tasks typically
 * done in the X server.
 *
 * Here they are converted to run in the client (or perhaps a standalone
 * process), and to work with the frame buffer device rather than the X
 * server infrastructure.
 * 
 * Copyright (C) 2006 Dave Airlie (airlied@linux.ie)

 Permission is hereby granted, free of charge, to any person obtaining a
 copy of this software and associated documentation files (the
 "Software"), to deal in the Software without restriction, including
 without limitation the rights to use, copy, modify, merge, publish,
 distribute, sub license, and/or sell copies of the Software, and to
 permit persons to whom the Software is furnished to do so, subject to
 the following conditions:
 
 The above copyright notice and this permission notice (including the
 next paragraph) shall be included in all copies or substantial portions
 of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 IN NO EVENT SHALL THE COPYRIGHT HOLDERS AND/OR THEIR SUPPLIERS BE LIABLE FOR
 ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include "driver.h"
#include "drm.h"

#include "intel.h"
#include "i830_dri.h"

#include "memops.h"
#include "pciaccess.h"

static size_t drm_page_size;
static int nextTile = 0;
#define xf86DrvMsg(...) do {} while(0)

static const int pitches[] = {
  128 * 8,
  128 * 16,
  128 * 32,
  128 * 64,
  0
};

static Bool I830DRIDoMappings(DRIDriverContext *ctx, I830Rec *pI830, drmI830Sarea *sarea);

static unsigned long
GetBestTileAlignment(unsigned long size)
{
   unsigned long i;

   for (i = KB(512); i < size; i <<= 1)
      ;

   if (i > MB(64))
      i = MB(64);

   return i;
}

static void SetFenceRegs(const DRIDriverContext *ctx, I830Rec *pI830)
{
  int i;
  unsigned char *MMIO = ctx->MMIOAddress;

  for (i = 0; i < 8; i++) {
    OUTREG(FENCE + i * 4, pI830->Fence[i]);
    //    if (I810_DEBUG & DEBUG_VERBOSE_VGA)
    fprintf(stderr,"Fence Register : %x\n", pI830->Fence[i]);
  }
}

/* Tiled memory is good... really, really good...
 *
 * Need to make it less likely that we miss out on this - probably
 * need to move the frontbuffer away from the 'guarenteed' alignment
 * of the first memory segment, or perhaps allocate a discontigous
 * framebuffer to get more alignment 'sweet spots'.
 */
static void
SetFence(const DRIDriverContext *ctx, I830Rec *pI830,
	 int nr, unsigned int start, unsigned int pitch,
         unsigned int size)
{
   unsigned int val;
   unsigned int fence_mask = 0;
   unsigned int fence_pitch;

   if (nr < 0 || nr > 7) {
      fprintf(stderr,
		 "SetFence: fence %d out of range\n",nr);
      return;
   }

   pI830->Fence[nr] = 0;

   if (IS_I9XX(pI830))
   	fence_mask = ~I915G_FENCE_START_MASK;
   else
   	fence_mask = ~I830_FENCE_START_MASK;

   if (start & fence_mask) {
      fprintf(stderr,
		 "SetFence: %d: start (0x%08x) is not %s aligned\n",
		 nr, start, (IS_I9XX(pI830)) ? "1MB" : "512k");
      return;
   }

   if (start % size) {
      fprintf(stderr,
		 "SetFence: %d: start (0x%08x) is not size (%dk) aligned\n",
		 nr, start, size / 1024);
      return;
   }

   if (pitch & 127) {
      fprintf(stderr,
		 "SetFence: %d: pitch (%d) not a multiple of 128 bytes\n",
		 nr, pitch);
      return;
   }

   val = (start | FENCE_X_MAJOR | FENCE_VALID);

   if (IS_I9XX(pI830)) {
   	switch (size) {
	   case MB(1):
      		val |= I915G_FENCE_SIZE_1M;
      		break;
   	   case MB(2):
      		val |= I915G_FENCE_SIZE_2M;
      		break;
   	   case MB(4):
      		val |= I915G_FENCE_SIZE_4M;
      		break;
   	   case MB(8):
      		val |= I915G_FENCE_SIZE_8M;
      		break;
   	   case MB(16):
      		val |= I915G_FENCE_SIZE_16M;
      		break;
   	   case MB(32):
      		val |= I915G_FENCE_SIZE_32M;
      		break;
   	   case MB(64):
      		val |= I915G_FENCE_SIZE_64M;
      		break;
   	   default:
      		fprintf(stderr,
		 "SetFence: %d: illegal size (%d kByte)\n", nr, size / 1024);
      		return;
   	}
    } else {
   	switch (size) {
	   case KB(512):
      		val |= FENCE_SIZE_512K;
      		break;
	   case MB(1):
      		val |= FENCE_SIZE_1M;
      		break;
   	   case MB(2):
      		val |= FENCE_SIZE_2M;
      		break;
   	   case MB(4):
      		val |= FENCE_SIZE_4M;
      		break;
   	   case MB(8):
      		val |= FENCE_SIZE_8M;
      		break;
   	   case MB(16):
      		val |= FENCE_SIZE_16M;
      		break;
   	   case MB(32):
      		val |= FENCE_SIZE_32M;
      		break;
   	   case MB(64):
      		val |= FENCE_SIZE_64M;
      		break;
   	   default:
      		fprintf(stderr,
		 "SetFence: %d: illegal size (%d kByte)\n", nr, size / 1024);
      		return;
   	}
   }

   if (IS_I9XX(pI830))
	fence_pitch = pitch / 512;
   else
	fence_pitch = pitch / 128;

   switch (fence_pitch) {
   case 1:
      val |= FENCE_PITCH_1;
      break;
   case 2:
      val |= FENCE_PITCH_2;
      break;
   case 4:
      val |= FENCE_PITCH_4;
      break;
   case 8:
      val |= FENCE_PITCH_8;
      break;
   case 16:
      val |= FENCE_PITCH_16;
      break;
   case 32:
      val |= FENCE_PITCH_32;
      break;
   case 64:
      val |= FENCE_PITCH_64;
      break;
   default:
      fprintf(stderr,
		 "SetFence: %d: illegal pitch (%d)\n", nr, pitch);
      return;
   }

   pI830->Fence[nr] = val;
}

static Bool
MakeTiles(const DRIDriverContext *ctx, I830Rec *pI830, I830MemRange *pMem)
{
   int pitch, ntiles, i;

   pitch = pMem->Pitch * ctx->cpp;
   /*
    * Simply try to break the region up into at most four pieces of size
    * equal to the alignment.
    */
   ntiles = ROUND_TO(pMem->Size, pMem->Alignment) / pMem->Alignment;
   if (ntiles >= 4) {
      return FALSE;
   }

   for (i = 0; i < ntiles; i++, nextTile++) {
     SetFence(ctx, pI830, nextTile, pMem->Start + i * pMem->Alignment,
	       pitch, pMem->Alignment);
   }
   return TRUE;
}

static void I830SetupMemoryTiling(const DRIDriverContext *ctx, I830Rec *pI830)
{
  int i;

  /* Clear out */
  for (i = 0; i < 8; i++)
    pI830->Fence[i] = 0;
  
  nextTile = 0;

  if (pI830->BackBuffer.Alignment >= KB(512)) {
    if (MakeTiles(ctx, pI830, &(pI830->BackBuffer))) {
      fprintf(stderr,
		 "Activating tiled memory for the back buffer.\n");
    } else {
      fprintf(stderr,
		 "MakeTiles failed for the back buffer.\n");
      pI830->allowPageFlip = FALSE;
    }
  }
  
  if (pI830->DepthBuffer.Alignment >= KB(512)) {
    if (MakeTiles(ctx, pI830, &(pI830->DepthBuffer))) {
      fprintf(stderr,
		 "Activating tiled memory for the depth buffer.\n");
    } else {
      fprintf(stderr,
		 "MakeTiles failed for the depth buffer.\n");
    }
  }

  return;
}

static int I830DetectMemory(const DRIDriverContext *ctx, I830Rec *pI830)
{
  struct pci_device host_bridge, ig_dev;
  uint32_t gmch_ctrl;
  int memsize = 0;
  int range;
  uint32_t aper_size;
  uint32_t membase2 = 0;
      
  memset(&host_bridge, 0, sizeof(host_bridge));
  memset(&ig_dev, 0, sizeof(ig_dev));

  ig_dev.dev = 2;

  pci_device_cfg_read_u32(&host_bridge, &gmch_ctrl, I830_GMCH_CTRL);

  if (IS_I830(pI830) || IS_845G(pI830)) {
    if ((gmch_ctrl & I830_GMCH_MEM_MASK) == I830_GMCH_MEM_128M) {
      aper_size = 0x80000000;
    } else {
      aper_size = 0x40000000;
    }
  } else {
    if (IS_I9XX(pI830)) {
      int ret;
      ret = pci_device_cfg_read_u32(&ig_dev, &membase2, 0x18);
      if (membase2 & 0x08000000)
	aper_size = 0x8000000;
      else
	aper_size = 0x10000000;

      fprintf(stderr,"aper size is %08X %08x %d\n", aper_size, membase2, ret);
    } else
      aper_size = 0x8000000;
  }

  pI830->aper_size = aper_size;


  /* We need to reduce the stolen size, by the GTT and the popup.
   * The GTT varying according the the FbMapSize and the popup is 4KB */
  range = (ctx->shared.fbSize / (1024*1024)) + 4;

   if (IS_I85X(pI830) || IS_I865G(pI830) || IS_I9XX(pI830)) {
      switch (gmch_ctrl & I830_GMCH_GMS_MASK) {
      case I855_GMCH_GMS_STOLEN_1M:
	 memsize = MB(1) - KB(range);
	 break;
      case I855_GMCH_GMS_STOLEN_4M:
	 memsize = MB(4) - KB(range);
	 break;
      case I855_GMCH_GMS_STOLEN_8M:
	 memsize = MB(8) - KB(range);
	 break;
      case I855_GMCH_GMS_STOLEN_16M:
	 memsize = MB(16) - KB(range);
	 break;
      case I855_GMCH_GMS_STOLEN_32M:
	 memsize = MB(32) - KB(range);
	 break;
      case I915G_GMCH_GMS_STOLEN_48M:
	 if (IS_I9XX(pI830))
	    memsize = MB(48) - KB(range);
	 break;
      case I915G_GMCH_GMS_STOLEN_64M:
	 if (IS_I9XX(pI830))
	    memsize = MB(64) - KB(range);
	 break;
      }
   } else {
      switch (gmch_ctrl & I830_GMCH_GMS_MASK) {
      case I830_GMCH_GMS_STOLEN_512:
	 memsize = KB(512) - KB(range);
	 break;
      case I830_GMCH_GMS_STOLEN_1024:
	 memsize = MB(1) - KB(range);
	 break;
      case I830_GMCH_GMS_STOLEN_8192:
	 memsize = MB(8) - KB(range);
	 break;
      case I830_GMCH_GMS_LOCAL:
	 memsize = 0;
	 xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
		    "Local memory found, but won't be used.\n");
	 break;
      }
   }
   if (memsize > 0) {
     fprintf(stderr,
		 "detected %d kB stolen memory.\n", memsize / 1024);
   } else {
     fprintf(stderr,
		 "no video memory detected.\n");
   }
   return memsize;
}

static int AgpInit(const DRIDriverContext *ctx, I830Rec *info)
{
  unsigned long mode = 0x4;

  if (drmAgpAcquire(ctx->drmFD) < 0) {
    fprintf(stderr, "[gart] AGP not available\n");
    return 0;
  }
  
  if (drmAgpEnable(ctx->drmFD, mode) < 0) {
    fprintf(stderr, "[gart] AGP not enabled\n");
    drmAgpRelease(ctx->drmFD);
    return 0;
  }
  else
    fprintf(stderr, "[gart] AGP enabled at %dx\n", ctx->agpmode);

  return 1;
}

/*
 * Allocate memory from the given pool.  Grow the pool if needed and if
 * possible.
 */
static unsigned long
AllocFromPool(const DRIDriverContext *ctx, I830Rec *pI830, 
	      I830MemRange *result, I830MemPool *pool,
	      long size, unsigned long alignment, int flags)
{
   long needed, start, end;

   if (!result || !pool || !size)
      return 0;

   /* Calculate how much space is needed. */
   if (alignment <= GTT_PAGE_SIZE)
      needed = size;
   else {
	 start = ROUND_TO(pool->Free.Start, alignment);
	 end = ROUND_TO(start + size, alignment);
	 needed = end - pool->Free.Start;
   }
   if (needed > pool->Free.Size) {
     return 0;
   }

   result->Start = ROUND_TO(pool->Free.Start, alignment);
   pool->Free.Start += needed;
   result->End = pool->Free.Start;

   pool->Free.Size = pool->Free.End - pool->Free.Start;
   result->Size = result->End - result->Start;
   result->Pool = pool;
   result->Alignment = alignment;
   return needed;
}

static unsigned long AllocFromAGP(const DRIDriverContext *ctx, I830Rec *pI830, long size, unsigned long alignment, I830MemRange  *result)
{
   unsigned long start, end;
   unsigned long newApStart, newApEnd;
   int ret;
   if (!result || !size)
      return 0;
   
   if (!alignment)
     alignment = 4;

   start = ROUND_TO(pI830->MemoryAperture.Start, alignment);
   end = ROUND_TO(start + size, alignment);
   newApStart = end;
   newApEnd = pI830->MemoryAperture.End;

   ret=drmAgpAlloc(ctx->drmFD, size, 0, &(result->Physical), (drm_handle_t *)&(result->Key));
   
   if (ret)
   {
     fprintf(stderr,"drmAgpAlloc failed %d\n", ret);
     return 0;
   }
   pI830->allocatedMemory += size;
   pI830->MemoryAperture.Start = newApStart;
   pI830->MemoryAperture.End = newApEnd;
   pI830->MemoryAperture.Size = newApEnd - newApStart;
   //   pI830->FreeMemory -= size;
   result->Start = start;
   result->End = start + size;
   result->Size = size;
   result->Offset = start;
   result->Alignment = alignment;
   result->Pool = NULL;
  
   return size;
}

unsigned long
I830AllocVidMem(const DRIDriverContext *ctx, I830Rec *pI830,
                I830MemRange *result, I830MemPool *pool, long size,
                unsigned long alignment, int flags)
{
   unsigned long ret;

   if (!result)
      return 0;

   /* Make sure these are initialised. */
   result->Size = 0;
   result->Key = -1;

   if (!size) {
      return 0;
   }

   if (pool->Free.Size < size) {
      ret = AllocFromAGP(ctx, pI830, size, alignment, result);
   }
   else {
      ret = AllocFromPool(ctx, pI830, result, pool, size, alignment, flags);
      if (ret == 0)
         ret = AllocFromAGP(ctx, pI830, size, alignment, result);
   }
   return ret;
}

static Bool BindAgpRange(const DRIDriverContext *ctx, I830MemRange *mem)
{
  if (!mem)
    return FALSE;
  
  if (mem->Key == -1)
    return TRUE;

  return !drmAgpBind(ctx->drmFD, mem->Key, mem->Offset);
}

/* simple memory allocation routines needed */
/* put ring buffer in low memory */
/* need to allocate front, back, depth buffers aligned correctly,
   allocate ring buffer, 
*/

/* */
static Bool
I830AllocateMemory(const DRIDriverContext *ctx, I830Rec *pI830)
{
  unsigned long size, ret;
  unsigned long lines, lineSize, align;

  /* allocate ring buffer */
  memset(pI830->LpRing, 0, sizeof(I830RingBuffer));
  pI830->LpRing->mem.Key = -1;

  size = PRIMARY_RINGBUFFER_SIZE;
  
  ret = I830AllocVidMem(ctx, pI830, &pI830->LpRing->mem, &pI830->StolenPool, size, 0x1000, 0);
  
  if (ret != size)
  {
    fprintf(stderr,"unable to allocate ring buffer %ld\n", ret);
    return FALSE;
  }

  pI830->LpRing->tail_mask = pI830->LpRing->mem.Size - 1;

  
  /* allocate front buffer */
  memset(&(pI830->FrontBuffer), 0, sizeof(pI830->FrontBuffer));
  pI830->FrontBuffer.Key = -1;
  pI830->FrontBuffer.Pitch = ctx->shared.virtualWidth;

  align = KB(512);  

  lineSize = ctx->shared.virtualWidth * ctx->cpp;
  lines = (ctx->shared.virtualHeight + 15) / 16 * 16;
  size = lineSize * lines;
  size = ROUND_TO_PAGE(size);

  align = GetBestTileAlignment(size);

  ret = I830AllocVidMem(ctx, pI830, &pI830->FrontBuffer, &pI830->StolenPool, size, align, 0);
  if (ret < size)
  {
    fprintf(stderr,"unable to allocate front buffer %ld\n", ret);
    return FALSE;
  }

  memset(&(pI830->BackBuffer), 0, sizeof(pI830->BackBuffer));
  pI830->BackBuffer.Key = -1;
  pI830->BackBuffer.Pitch = ctx->shared.virtualWidth;

  ret = I830AllocVidMem(ctx, pI830, &pI830->BackBuffer, &pI830->StolenPool, size, align, 0);
  if (ret < size)
  {
    fprintf(stderr,"unable to allocate back buffer %ld\n", ret);
    return FALSE;
  }
  
  memset(&(pI830->DepthBuffer), 0, sizeof(pI830->DepthBuffer));
  pI830->DepthBuffer.Key = -1;
  pI830->DepthBuffer.Pitch = ctx->shared.virtualWidth;

  ret = I830AllocVidMem(ctx, pI830, &pI830->DepthBuffer, &pI830->StolenPool, size, align, 0);
  if (ret < size)
  {
    fprintf(stderr,"unable to allocate depth buffer %ld\n", ret);
    return FALSE;
  }

  memset(&(pI830->ContextMem), 0, sizeof(pI830->ContextMem));
  pI830->ContextMem.Key = -1;
  size = KB(32);

  ret = I830AllocVidMem(ctx, pI830, &pI830->ContextMem, &pI830->StolenPool, size, align, 0);
  if (ret < size)
  {
    fprintf(stderr,"unable to allocate context buffer %ld\n", ret);
    return FALSE;
  }

#if 0  
  memset(&(pI830->TexMem), 0, sizeof(pI830->TexMem));
  pI830->TexMem.Key = -1;

  size = 32768 * 1024;
  ret = AllocFromAGP(ctx, pI830, size, align, &pI830->TexMem);
  if (ret < size)
  {
    fprintf(stderr,"unable to allocate texture memory %ld\n", ret);
    return FALSE;
  }
#endif

  return TRUE;
}

static Bool
I830BindMemory(const DRIDriverContext *ctx, I830Rec *pI830)
{
  if (!BindAgpRange(ctx, &pI830->LpRing->mem))
    return FALSE;
  if (!BindAgpRange(ctx, &pI830->FrontBuffer))
    return FALSE;
  if (!BindAgpRange(ctx, &pI830->BackBuffer))
    return FALSE;
  if (!BindAgpRange(ctx, &pI830->DepthBuffer))
    return FALSE;
  if (!BindAgpRange(ctx, &pI830->ContextMem))
    return FALSE;
#if 0
  if (!BindAgpRange(ctx, &pI830->TexMem))
    return FALSE;
#endif
  return TRUE;
}

static void SetupDRIMM(const DRIDriverContext *ctx, I830Rec *pI830)
{
  unsigned long aperEnd = ROUND_DOWN_TO(pI830->aper_size, GTT_PAGE_SIZE) / GTT_PAGE_SIZE;
  unsigned long aperStart = ROUND_TO(pI830->aper_size - KB(32768), GTT_PAGE_SIZE) / GTT_PAGE_SIZE;

  fprintf(stderr, "aper size is %08X\n", ctx->shared.fbSize);
  if (drmMMInit(ctx->drmFD, aperStart, aperEnd - aperStart, DRM_BO_MEM_TT)) {
      fprintf(stderr,
	      "DRM MM Initialization Failed\n");
  } else {
    fprintf(stderr,
	    "DRM MM Initialized at offset 0x%lx length %d page\n", aperStart, aperEnd-aperStart);
  }

}

static Bool
I830CleanupDma(const DRIDriverContext *ctx)
{
   drmI830Init info;

   memset(&info, 0, sizeof(drmI830Init));
   info.func = I830_CLEANUP_DMA;

   if (drmCommandWrite(ctx->drmFD, DRM_I830_INIT,
		       &info, sizeof(drmI830Init))) {
     fprintf(stderr, "I830 Dma Cleanup Failed\n");
      return FALSE;
   }

   return TRUE;
}

static Bool
I830InitDma(const DRIDriverContext *ctx, I830Rec *pI830)
{
   I830RingBuffer *ring = pI830->LpRing;
   drmI830Init info;

   memset(&info, 0, sizeof(drmI830Init));
   info.func = I830_INIT_DMA;

   info.ring_start = ring->mem.Start + pI830->LinearAddr;
   info.ring_end = ring->mem.End + pI830->LinearAddr;
   info.ring_size = ring->mem.Size;

   info.mmio_offset = (unsigned int)ctx->MMIOStart;

   info.sarea_priv_offset = sizeof(drm_sarea_t);

   info.front_offset = pI830->FrontBuffer.Start;
   info.back_offset = pI830->BackBuffer.Start;
   info.depth_offset = pI830->DepthBuffer.Start;
   info.w = ctx->shared.virtualWidth;
   info.h = ctx->shared.virtualHeight;
   info.pitch = ctx->shared.virtualWidth;
   info.back_pitch = pI830->BackBuffer.Pitch;
   info.depth_pitch = pI830->DepthBuffer.Pitch;
   info.cpp = ctx->cpp;

   if (drmCommandWrite(ctx->drmFD, DRM_I830_INIT,
		       &info, sizeof(drmI830Init))) {
      fprintf(stderr,
		 "I830 Dma Initialization Failed\n");
      return FALSE;
   }

   return TRUE;
}

static int I830CheckDRMVersion( const DRIDriverContext *ctx,
				  I830Rec *pI830 )
{
   drmVersionPtr  version;

   version = drmGetVersion(ctx->drmFD);

   if (version) {
     int req_minor, req_patch;

     req_minor = 4;
     req_patch = 0;	

     if (version->version_major != 1 ||
	 version->version_minor < req_minor ||
	 (version->version_minor == req_minor && 
	  version->version_patchlevel < req_patch)) {
       /* Incompatible drm version */
       fprintf(stderr,
	       "[dri] I830DRIScreenInit failed because of a version "
	       "mismatch.\n"
	       "[dri] i915.o kernel module version is %d.%d.%d "
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
     
     pI830->drmMinor = version->version_minor;
     drmFreeVersion(version);
   }
   return 1;
}

static void
I830SetRingRegs(const DRIDriverContext *ctx, I830Rec *pI830)
{
  unsigned int itemp;
  unsigned char *MMIO = ctx->MMIOAddress;

   OUTREG(LP_RING + RING_LEN, 0);
   OUTREG(LP_RING + RING_TAIL, 0);
   OUTREG(LP_RING + RING_HEAD, 0);

   if ((long)(pI830->LpRing->mem.Start & I830_RING_START_MASK) !=
       pI830->LpRing->mem.Start) {
      fprintf(stderr,
		 "I830SetRingRegs: Ring buffer start (%lx) violates its "
		 "mask (%x)\n", pI830->LpRing->mem.Start, I830_RING_START_MASK);
   }
   /* Don't care about the old value.  Reserved bits must be zero anyway. */
   itemp = pI830->LpRing->mem.Start & I830_RING_START_MASK;
   OUTREG(LP_RING + RING_START, itemp);

   if (((pI830->LpRing->mem.Size - 4096) & I830_RING_NR_PAGES) !=
       pI830->LpRing->mem.Size - 4096) {
      fprintf(stderr,
		 "I830SetRingRegs: Ring buffer size - 4096 (%lx) violates its "
		 "mask (%x)\n", pI830->LpRing->mem.Size - 4096,
		 I830_RING_NR_PAGES);
   }
   /* Don't care about the old value.  Reserved bits must be zero anyway. */
   itemp = (pI830->LpRing->mem.Size - 4096) & I830_RING_NR_PAGES;
   itemp |= (RING_NO_REPORT | RING_VALID);
   OUTREG(LP_RING + RING_LEN, itemp);

   pI830->LpRing->head = INREG(LP_RING + RING_HEAD) & I830_HEAD_MASK;
   pI830->LpRing->tail = INREG(LP_RING + RING_TAIL);
   pI830->LpRing->space = pI830->LpRing->head - (pI830->LpRing->tail + 8);
   if (pI830->LpRing->space < 0)
      pI830->LpRing->space += pI830->LpRing->mem.Size;

   SetFenceRegs(ctx, pI830);
   
   /* RESET THE DISPLAY PIPE TO POINT TO THE FRONTBUFFER - hacky
      hacky hacky */
   OUTREG(DSPABASE, pI830->FrontBuffer.Start + pI830->LinearAddr);

}

static Bool
I830SetParam(const DRIDriverContext *ctx, int param, int value)
{
   drmI830SetParam sp;

   memset(&sp, 0, sizeof(sp));
   sp.param = param;
   sp.value = value;

   if (drmCommandWrite(ctx->drmFD, DRM_I830_SETPARAM, &sp, sizeof(sp))) {
      fprintf(stderr, "I830 SetParam Failed\n");
      return FALSE;
   }

   return TRUE;
}

static Bool
I830DRIMapScreenRegions(DRIDriverContext *ctx, I830Rec *pI830, drmI830Sarea *sarea)
{
   fprintf(stderr,
              "[drm] Mapping front buffer\n");

   if (drmAddMap(ctx->drmFD,
                 (drm_handle_t)(sarea->front_offset + pI830->LinearAddr),
                 sarea->front_size,
                 DRM_FRAME_BUFFER,  /*DRM_AGP,*/
                 0,
                 &sarea->front_handle) < 0) {
     fprintf(stderr,
	     "[drm] drmAddMap(front_handle) failed. Disabling DRI\n");
      return FALSE;
   }
   ctx->shared.hFrameBuffer = sarea->front_handle;
   ctx->shared.fbSize = sarea->front_size;
   fprintf(stderr, "[drm] Front Buffer = 0x%08x\n",
	   sarea->front_handle);

   if (drmAddMap(ctx->drmFD,
                 (drm_handle_t)(sarea->back_offset),
                 sarea->back_size, DRM_AGP, 0,
                 &sarea->back_handle) < 0) {
      fprintf(stderr,
                 "[drm] drmAddMap(back_handle) failed. Disabling DRI\n");
      return FALSE;
   }
   fprintf(stderr, "[drm] Back Buffer = 0x%08x\n",
              sarea->back_handle);

   if (drmAddMap(ctx->drmFD,
                 (drm_handle_t)sarea->depth_offset,
                 sarea->depth_size, DRM_AGP, 0,
                 &sarea->depth_handle) < 0) {
      fprintf(stderr,
                 "[drm] drmAddMap(depth_handle) failed. Disabling DRI\n");
      return FALSE;
   }
   fprintf(stderr, "[drm] Depth Buffer = 0x%08x\n",
              sarea->depth_handle);

#if 0
   if (drmAddMap(ctx->drmFD,
		 (drm_handle_t)sarea->tex_offset,
		 sarea->tex_size, DRM_AGP, 0,
		 &sarea->tex_handle) < 0) {
      fprintf(stderr,
		 "[drm] drmAddMap(tex_handle) failed. Disabling DRI\n");
      return FALSE;
   }
   fprintf(stderr, "[drm] textures = 0x%08x\n",
	      sarea->tex_handle);
#endif
   return TRUE;
}


static void
I830DRIUnmapScreenRegions(const DRIDriverContext *ctx, I830Rec *pI830, drmI830Sarea *sarea)
{
#if 1
   if (sarea->front_handle) {
      drmRmMap(ctx->drmFD, sarea->front_handle);
      sarea->front_handle = 0;
   }
#endif
   if (sarea->back_handle) {
      drmRmMap(ctx->drmFD, sarea->back_handle);
      sarea->back_handle = 0;
   }
   if (sarea->depth_handle) {
      drmRmMap(ctx->drmFD, sarea->depth_handle);
      sarea->depth_handle = 0;
   }
   if (sarea->tex_handle) {
      drmRmMap(ctx->drmFD, sarea->tex_handle);
      sarea->tex_handle = 0;
   }
}

static Bool
I830DRIDoMappings(DRIDriverContext *ctx, I830Rec *pI830, drmI830Sarea *sarea)
{
  if (drmAddMap(ctx->drmFD,
		(drm_handle_t)pI830->LpRing->mem.Start,
		pI830->LpRing->mem.Size, DRM_AGP, 0,
		&pI830->ring_map) < 0) {
    fprintf(stderr,
	    "[drm] drmAddMap(ring_map) failed. Disabling DRI\n");
    return FALSE;
  }
  fprintf(stderr, "[drm] ring buffer = 0x%08x\n",
	  pI830->ring_map);

  if (I830InitDma(ctx, pI830) == FALSE) {
    return FALSE;
  }
  
   /* init to zero to be safe */

  I830DRIMapScreenRegions(ctx, pI830, sarea);
  SetupDRIMM(ctx, pI830);

   if (ctx->pciDevice != PCI_CHIP_845_G &&
       ctx->pciDevice != PCI_CHIP_I830_M) {
      I830SetParam(ctx, I830_SETPARAM_USE_MI_BATCHBUFFER_START, 1 );
   }

   /* Okay now initialize the dma engine */
   {
      pI830->irq = drmGetInterruptFromBusID(ctx->drmFD,
					    ctx->pciBus,
					    ctx->pciDevice,
					    ctx->pciFunc);

      if (drmCtlInstHandler(ctx->drmFD, pI830->irq)) {
	 fprintf(stderr,
		    "[drm] failure adding irq handler\n");
	 pI830->irq = 0;
	 return FALSE;
      }
      else
	 fprintf(stderr,
		    "[drm] dma control initialized, using IRQ %d\n",
		    pI830->irq);
   }

   fprintf(stderr, "[dri] visual configs initialized\n");

   return TRUE;
}

static Bool
I830ClearScreen(DRIDriverContext *ctx, I830Rec *pI830, drmI830Sarea *sarea)
{
  /* need to drmMap front and back buffers and zero them */
  drmAddress map_addr;
  int ret;

  ret = drmMap(ctx->drmFD,
	       sarea->front_handle,
	       sarea->front_size,
	       &map_addr);

  if (ret)
  {
    fprintf(stderr, "Unable to map front buffer\n");
    return FALSE;
  }

  drimemsetio((char *)map_addr,
	      0,
	      sarea->front_size);
  drmUnmap(map_addr, sarea->front_size);


  ret = drmMap(ctx->drmFD,
	       sarea->back_handle,
	       sarea->back_size,
	       &map_addr);

  if (ret)
  {
    fprintf(stderr, "Unable to map back buffer\n");
    return FALSE;
  }

  drimemsetio((char *)map_addr,
	      0,
	      sarea->back_size);
  drmUnmap(map_addr, sarea->back_size);

  return TRUE;
}

static Bool
I830ScreenInit(DRIDriverContext *ctx, I830Rec *pI830)
		  
{
   I830DRIPtr pI830DRI;
   drmI830Sarea *pSAREAPriv;
   int err;
      
   drm_page_size = getpagesize();   

   pI830->registerSize = ctx->MMIOSize;
   /* This is a hack for now.  We have to have more than a 4k page here
    * because of the size of the state.  However, the state should be
    * in a per-context mapping.  This will be added in the Mesa 3.5 port
    * of the I830 driver.
    */
   ctx->shared.SAREASize = SAREA_MAX;

   /* Note that drmOpen will try to load the kernel module, if needed. */
   ctx->drmFD = drmOpen("i915", NULL );
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

   fprintf(stderr, "[drm] added %d byte SAREA at 0x%08x\n",
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
   fprintf(stderr, "[drm] mapped SAREA 0x%08x to %p, size %d\n",
	   ctx->shared.hSAREA, ctx->pSAREA, ctx->shared.SAREASize);
   

   if (drmAddMap(ctx->drmFD, 
		 ctx->MMIOStart,
		 ctx->MMIOSize,
		 DRM_REGISTERS, 
		 DRM_READ_ONLY, 
		 &pI830->registerHandle) < 0) {
      fprintf(stderr, "[drm] drmAddMap mmio failed\n");	
      return 0;
   }
   fprintf(stderr,
	   "[drm] register handle = 0x%08x\n", pI830->registerHandle);


   if (!I830CheckDRMVersion(ctx, pI830)) {
     return FALSE;
   }

   /* Create a 'server' context so we can grab the lock for
    * initialization ioctls.
    */
   if ((err = drmCreateContext(ctx->drmFD, &ctx->serverContext)) != 0) {
      fprintf(stderr, "%s: drmCreateContext failed %d\n", __FUNCTION__, err);
      return 0;
   }

   DRM_LOCK(ctx->drmFD, ctx->pSAREA, ctx->serverContext, 0); 

   /* Initialize the SAREA private data structure */
   pSAREAPriv = (drmI830Sarea *)(((char*)ctx->pSAREA) + 
				 sizeof(drm_sarea_t));
   memset(pSAREAPriv, 0, sizeof(*pSAREAPriv));

   pI830->StolenMemory.Size = I830DetectMemory(ctx, pI830);
   pI830->StolenMemory.Start = 0;
   pI830->StolenMemory.End = pI830->StolenMemory.Size;

   pI830->MemoryAperture.Start = pI830->StolenMemory.End;
   pI830->MemoryAperture.End = KB(40000);
   pI830->MemoryAperture.Size = pI830->MemoryAperture.End - pI830->MemoryAperture.Start;

   pI830->StolenPool.Fixed = pI830->StolenMemory;
   pI830->StolenPool.Total = pI830->StolenMemory;
   pI830->StolenPool.Free = pI830->StolenPool.Total;
   pI830->FreeMemory = pI830->StolenPool.Total.Size;

   if (!AgpInit(ctx, pI830))
     return FALSE;

   if (I830AllocateMemory(ctx, pI830) == FALSE)
   {
     return FALSE;
   }

   if (I830BindMemory(ctx, pI830) == FALSE)
   {
     return FALSE;
   }

   pSAREAPriv->rotated_offset = -1;
   pSAREAPriv->rotated_size = 0;
   pSAREAPriv->rotated_pitch = ctx->shared.virtualWidth;

   pSAREAPriv->front_offset = pI830->FrontBuffer.Start;
   pSAREAPriv->front_size = pI830->FrontBuffer.Size;
   pSAREAPriv->width = ctx->shared.virtualWidth;
   pSAREAPriv->height = ctx->shared.virtualHeight;
   pSAREAPriv->pitch = ctx->shared.virtualWidth;
   pSAREAPriv->virtualX = ctx->shared.virtualWidth;
   pSAREAPriv->virtualY = ctx->shared.virtualHeight;
   pSAREAPriv->back_offset = pI830->BackBuffer.Start;
   pSAREAPriv->back_size = pI830->BackBuffer.Size;
   pSAREAPriv->depth_offset = pI830->DepthBuffer.Start;
   pSAREAPriv->depth_size = pI830->DepthBuffer.Size;
#if 0
   pSAREAPriv->tex_offset = pI830->TexMem.Start;
   pSAREAPriv->tex_size = pI830->TexMem.Size;
#endif
   pSAREAPriv->log_tex_granularity = pI830->TexGranularity;

   ctx->driverClientMsg = malloc(sizeof(I830DRIRec));
   ctx->driverClientMsgSize = sizeof(I830DRIRec);
   pI830DRI = (I830DRIPtr)ctx->driverClientMsg;
   pI830DRI->deviceID = pI830->Chipset;
   pI830DRI->regsSize = I830_REG_SIZE;
   pI830DRI->width = ctx->shared.virtualWidth;
   pI830DRI->height = ctx->shared.virtualHeight;
   pI830DRI->mem = ctx->shared.fbSize;
   pI830DRI->cpp = ctx->cpp;

   pI830DRI->bitsPerPixel = ctx->bpp;
   pI830DRI->sarea_priv_offset = sizeof(drm_sarea_t);
   
   err = I830DRIDoMappings(ctx, pI830, pSAREAPriv);
   if (err == FALSE)
       return FALSE;

   I830SetupMemoryTiling(ctx, pI830);

   /* Quick hack to clear the front & back buffers.  Could also use
    * the clear ioctl to do this, but would need to setup hw state
    * first.
    */
   I830ClearScreen(ctx, pI830, pSAREAPriv);

   I830SetRingRegs(ctx, pI830);

   return TRUE;
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
 * \sa radeonValidateMode().
 */
static int i830ValidateMode( const DRIDriverContext *ctx )
{
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
static int i830PostValidateMode( const DRIDriverContext *ctx )
{
  I830Rec *pI830 = ctx->driverPrivate;

  I830SetRingRegs(ctx, pI830);
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
static int i830InitFBDev( DRIDriverContext *ctx )
{
  I830Rec *pI830 = calloc(1, sizeof(I830Rec));
  int i;

   {
      int  dummy = ctx->shared.virtualWidth;

      switch (ctx->bpp / 8) {
      case 1: dummy = (ctx->shared.virtualWidth + 127) & ~127; break;
      case 2: dummy = (ctx->shared.virtualWidth +  31) &  ~31; break;
      case 3:
      case 4: dummy = (ctx->shared.virtualWidth +  15) &  ~15; break;
      }

      ctx->shared.virtualWidth = dummy;
      ctx->shared.Width = ctx->shared.virtualWidth;
   }


   for (i = 0; pitches[i] != 0; i++) {
     if (pitches[i] >= ctx->shared.virtualWidth) {
       ctx->shared.virtualWidth = pitches[i];
       break;
     }
   }

   ctx->driverPrivate = (void *)pI830;
   
   pI830->LpRing = calloc(1, sizeof(I830RingBuffer));
   pI830->Chipset = ctx->chipset;
   pI830->LinearAddr = ctx->FBStart;

   if (!I830ScreenInit( ctx, pI830 ))
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
static void i830HaltFBDev( DRIDriverContext *ctx )
{
  drmI830Sarea *pSAREAPriv;
  I830Rec *pI830 = ctx->driverPrivate;

   if (pI830->irq) {
       drmCtlUninstHandler(ctx->drmFD);
       pI830->irq = 0;   }

   I830CleanupDma(ctx);

  pSAREAPriv = (drmI830Sarea *)(((char*)ctx->pSAREA) + 
				sizeof(drm_sarea_t));

  I830DRIUnmapScreenRegions(ctx, pI830, pSAREAPriv);
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
   i830ValidateMode,
   i830PostValidateMode,
   i830InitFBDev,
   i830HaltFBDev,
   NULL,//I830EngineShutdown,
   NULL, //I830EngineRestore,  
#ifndef _EMBEDDED
   0,
#else
   i810NotifyFocus, 
#endif
};
