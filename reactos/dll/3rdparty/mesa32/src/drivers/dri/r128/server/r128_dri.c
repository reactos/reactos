/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/ati/r128_dri.c,v 1.28 2003/02/07 20:41:14 martin Exp $ */
/*
 * Copyright 1999, 2000 ATI Technologies Inc., Markham, Ontario,
 *                      Precision Insight, Inc., Cedar Park, Texas, and
 *                      VA Linux Systems Inc., Fremont, California.
 *
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation on the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial
 * portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NON-INFRINGEMENT.  IN NO EVENT SHALL ATI, PRECISION INSIGHT, VA LINUX
 * SYSTEMS AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

/*
 * Authors:
 *   Kevin E. Martin <martin@valinux.com>
 *   Rickard E. Faith <faith@valinux.com>
 *   Daryll Strauss <daryll@valinux.com>
 *   Gareth Hughes <gareth@valinux.com>
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
// Fix this to use kernel pci_ids.h when all of these IDs make it into the kernel 
#include "pci_ids.h"

#include "driver.h"
#include "drm.h"
#include "memops.h"

#include "r128.h"
#include "r128_dri.h"
#include "r128_macros.h"
#include "r128_reg.h"
#include "r128_version.h"
#include "r128_drm.h"

static size_t r128_drm_page_size;

/* Compute log base 2 of val. */
static int R128MinBits(int val)
{
    int bits;

    if (!val) return 1;
    for (bits = 0; val; val >>= 1, ++bits);
    return bits;
}

/* Initialize the AGP state.  Request memory for use in AGP space, and
   initialize the Rage 128 registers to point to that memory. */
static GLboolean R128DRIAgpInit(const DRIDriverContext *ctx)
{
    unsigned char *R128MMIO = ctx->MMIOAddress;
    R128InfoPtr info = ctx->driverPrivate;
    unsigned long mode;
    unsigned int  vendor, device;
    int           ret;
    unsigned long cntl, chunk;
    int           s, l;
    int           flags;
    unsigned long agpBase;

    if (drmAgpAcquire(ctx->drmFD) < 0) {
	fprintf(stderr, "[agp] AGP not available\n");
	return GL_FALSE;
    }

				/* Modify the mode if the default mode is
				   not appropriate for this particular
				   combination of graphics card and AGP
				   chipset. */

    mode   = drmAgpGetMode(ctx->drmFD);        /* Default mode */
    vendor = drmAgpVendorId(ctx->drmFD);
    device = drmAgpDeviceId(ctx->drmFD);

    mode &= ~R128_AGP_MODE_MASK;
    switch (info->agpMode) {
    case 4:          mode |= R128_AGP_4X_MODE;
    case 2:          mode |= R128_AGP_2X_MODE;
    case 1: default: mode |= R128_AGP_1X_MODE;
    }

    fprintf(stderr,
	       "[agp] Mode 0x%08lx [AGP 0x%04x/0x%04x; Card 0x%04x/0x%04x]\n",
	       mode, vendor, device,
	       0x1002,
	       info->Chipset);

    if (drmAgpEnable(ctx->drmFD, mode) < 0) {
	fprintf(stderr, "[agp] AGP not enabled\n");
	drmAgpRelease(ctx->drmFD);
	return GL_FALSE;
    }

    info->agpOffset = 0;

    if ((ret = drmAgpAlloc(ctx->drmFD, info->agpSize*1024*1024, 0, NULL,
			   &info->agpMemHandle)) < 0) {
	fprintf(stderr, "[agp] Out of memory (%d)\n", ret);
	drmAgpRelease(ctx->drmFD);
	return GL_FALSE;
    }
    fprintf(stderr,
	       "[agp] %d kB allocated with handle 0x%08x\n",
	       info->agpSize*1024, info->agpMemHandle);

    if (drmAgpBind(ctx->drmFD, info->agpMemHandle, info->agpOffset) < 0) {
	fprintf(stderr, "[agp] Could not bind\n");
	drmAgpFree(ctx->drmFD, info->agpMemHandle);
	drmAgpRelease(ctx->drmFD);
	return GL_FALSE;
    }

				/* Initialize the CCE ring buffer data */
    info->ringStart       = info->agpOffset;
    info->ringMapSize     = info->ringSize*1024*1024 + r128_drm_page_size;
    info->ringSizeLog2QW  = R128MinBits(info->ringSize*1024*1024/8) - 1;

    info->ringReadOffset  = info->ringStart + info->ringMapSize;
    info->ringReadMapSize = r128_drm_page_size;

				/* Reserve space for vertex/indirect buffers */
    info->bufStart        = info->ringReadOffset + info->ringReadMapSize;
    info->bufMapSize      = info->bufSize*1024*1024;

				/* Reserve the rest for AGP textures */
    info->agpTexStart     = info->bufStart + info->bufMapSize;
    s = (info->agpSize*1024*1024 - info->agpTexStart);
    l = R128MinBits((s-1) / R128_NR_TEX_REGIONS);
    if (l < R128_LOG_TEX_GRANULARITY) l = R128_LOG_TEX_GRANULARITY;
    info->agpTexMapSize   = (s >> l) << l;
    info->log2AGPTexGran  = l;

    if (info->CCESecure) flags = DRM_READ_ONLY;
    else                  flags = 0;

    if (drmAddMap(ctx->drmFD, info->ringStart, info->ringMapSize,
		  DRM_AGP, flags, &info->ringHandle) < 0) {
	fprintf(stderr,
		   "[agp] Could not add ring mapping\n");
	return GL_FALSE;
    }
    fprintf(stderr,
	       "[agp] ring handle = 0x%08x\n", info->ringHandle);

    if (drmMap(ctx->drmFD, info->ringHandle, info->ringMapSize,
	       (drmAddressPtr)&info->ring) < 0) {
	fprintf(stderr, "[agp] Could not map ring\n");
	return GL_FALSE;
    }
    fprintf(stderr,
	       "[agp] Ring mapped at 0x%08lx\n",
	       (unsigned long)info->ring);

    if (drmAddMap(ctx->drmFD, info->ringReadOffset, info->ringReadMapSize,
		  DRM_AGP, flags, &info->ringReadPtrHandle) < 0) {
	fprintf(stderr,
		   "[agp] Could not add ring read ptr mapping\n");
	return GL_FALSE;
    }
    fprintf(stderr,
 	       "[agp] ring read ptr handle = 0x%08x\n",
	       info->ringReadPtrHandle);

    if (drmMap(ctx->drmFD, info->ringReadPtrHandle, info->ringReadMapSize,
	       (drmAddressPtr)&info->ringReadPtr) < 0) {
	fprintf(stderr,
		   "[agp] Could not map ring read ptr\n");
	return GL_FALSE;
    }
    fprintf(stderr,
	       "[agp] Ring read ptr mapped at 0x%08lx\n",
	       (unsigned long)info->ringReadPtr);

    if (drmAddMap(ctx->drmFD, info->bufStart, info->bufMapSize,
		  DRM_AGP, 0, &info->bufHandle) < 0) {
	fprintf(stderr,
		   "[agp] Could not add vertex/indirect buffers mapping\n");
	return GL_FALSE;
    }
    fprintf(stderr,
	       "[agp] vertex/indirect buffers handle = 0x%08lx\n",
	       info->bufHandle);

    if (drmMap(ctx->drmFD, info->bufHandle, info->bufMapSize,
	       (drmAddressPtr)&info->buf) < 0) {
	fprintf(stderr,
		   "[agp] Could not map vertex/indirect buffers\n");
	return GL_FALSE;
    }
    fprintf(stderr,
	       "[agp] Vertex/indirect buffers mapped at 0x%08lx\n",
	       (unsigned long)info->buf);

    if (drmAddMap(ctx->drmFD, info->agpTexStart, info->agpTexMapSize,
		  DRM_AGP, 0, &info->agpTexHandle) < 0) {
	fprintf(stderr,
		   "[agp] Could not add AGP texture map mapping\n");
	return GL_FALSE;
    }
    fprintf(stderr,
	       "[agp] AGP texture map handle = 0x%08lx\n",
	       info->agpTexHandle);

    if (drmMap(ctx->drmFD, info->agpTexHandle, info->agpTexMapSize,
	       (drmAddressPtr)&info->agpTex) < 0) {
	fprintf(stderr,
		   "[agp] Could not map AGP texture map\n");
	return GL_FALSE;
    }
    fprintf(stderr,
	       "[agp] AGP Texture map mapped at 0x%08lx\n",
	       (unsigned long)info->agpTex);

				/* Initialize Rage 128's AGP registers */
    cntl  = INREG(R128_AGP_CNTL);
    cntl &= ~R128_AGP_APER_SIZE_MASK;
    switch (info->agpSize) {
    case 256: cntl |= R128_AGP_APER_SIZE_256MB; break;
    case 128: cntl |= R128_AGP_APER_SIZE_128MB; break;
    case  64: cntl |= R128_AGP_APER_SIZE_64MB;  break;
    case  32: cntl |= R128_AGP_APER_SIZE_32MB;  break;
    case  16: cntl |= R128_AGP_APER_SIZE_16MB;  break;
    case   8: cntl |= R128_AGP_APER_SIZE_8MB;   break;
    case   4: cntl |= R128_AGP_APER_SIZE_4MB;   break;
    default:
	fprintf(stderr,
		   "[agp] Illegal aperture size %d kB\n",
		   info->agpSize*1024);
	return GL_FALSE;
    }
    agpBase = drmAgpBase(ctx->drmFD);
    OUTREG(R128_AGP_BASE, agpBase); 
    OUTREG(R128_AGP_CNTL, cntl);

				/* Disable Rage 128's PCIGART registers */
    chunk = INREG(R128_BM_CHUNK_0_VAL);
    chunk &= ~(R128_BM_PTR_FORCE_TO_PCI |
	       R128_BM_PM4_RD_FORCE_TO_PCI |
	       R128_BM_GLOBAL_FORCE_TO_PCI);
    OUTREG(R128_BM_CHUNK_0_VAL, chunk);

    OUTREG(R128_PCI_GART_PAGE, 1); /* Ensure AGP GART is used (for now) */

    return GL_TRUE;
}

static GLboolean R128DRIPciInit(const DRIDriverContext *ctx)
{
    R128InfoPtr info = ctx->driverPrivate;
    unsigned char *R128MMIO = ctx->MMIOAddress;
    u_int32_t chunk;
    int ret;
    int flags;

    info->agpOffset = 0;

    ret = drmScatterGatherAlloc(ctx->drmFD, info->agpSize*1024*1024,
				&info->pciMemHandle);
    if (ret < 0) {
	fprintf(stderr, "[pci] Out of memory (%d)\n", ret);
	return GL_FALSE;
    }
    fprintf(stderr,
	       "[pci] %d kB allocated with handle 0x%08x\n",
	       info->agpSize*1024, info->pciMemHandle);

				/* Initialize the CCE ring buffer data */
    info->ringStart       = info->agpOffset;
    info->ringMapSize     = info->ringSize*1024*1024 + r128_drm_page_size;
    info->ringSizeLog2QW  = R128MinBits(info->ringSize*1024*1024/8) - 1;

    info->ringReadOffset  = info->ringStart + info->ringMapSize;
    info->ringReadMapSize = r128_drm_page_size;

				/* Reserve space for vertex/indirect buffers */
    info->bufStart        = info->ringReadOffset + info->ringReadMapSize;
    info->bufMapSize      = info->bufSize*1024*1024;

    flags = DRM_READ_ONLY | DRM_LOCKED | DRM_KERNEL;

    if (drmAddMap(ctx->drmFD, info->ringStart, info->ringMapSize,
		  DRM_SCATTER_GATHER, flags, &info->ringHandle) < 0) {
	fprintf(stderr,
		   "[pci] Could not add ring mapping\n");
	return GL_FALSE;
    }
    fprintf(stderr,
	       "[pci] ring handle = 0x%08lx\n", info->ringHandle);

    if (drmMap(ctx->drmFD, info->ringHandle, info->ringMapSize,
	       (drmAddressPtr)&info->ring) < 0) {
	fprintf(stderr, "[pci] Could not map ring\n");
	return GL_FALSE;
    }
    fprintf(stderr,
	       "[pci] Ring mapped at 0x%08lx\n",
	       (unsigned long)info->ring);
    fprintf(stderr,
	       "[pci] Ring contents 0x%08lx\n",
	       *(unsigned long *)info->ring);

    if (drmAddMap(ctx->drmFD, info->ringReadOffset, info->ringReadMapSize,
		  DRM_SCATTER_GATHER, flags, &info->ringReadPtrHandle) < 0) {
	fprintf(stderr,
		   "[pci] Could not add ring read ptr mapping\n");
	return GL_FALSE;
    }
    fprintf(stderr,
	       "[pci] ring read ptr handle = 0x%08lx\n",
	       info->ringReadPtrHandle);

    if (drmMap(ctx->drmFD, info->ringReadPtrHandle, info->ringReadMapSize,
	       (drmAddressPtr)&info->ringReadPtr) < 0) {
	fprintf(stderr,
		   "[pci] Could not map ring read ptr\n");
	return GL_FALSE;
    }
    fprintf(stderr,
	       "[pci] Ring read ptr mapped at 0x%08lx\n",
	       (unsigned long)info->ringReadPtr);
    fprintf(stderr,
	       "[pci] Ring read ptr contents 0x%08lx\n",
	       *(unsigned long *)info->ringReadPtr);

    if (drmAddMap(ctx->drmFD, info->bufStart, info->bufMapSize,
		  DRM_SCATTER_GATHER, 0, &info->bufHandle) < 0) {
	fprintf(stderr,
		   "[pci] Could not add vertex/indirect buffers mapping\n");
	return GL_FALSE;
    }
    fprintf(stderr,
	       "[pci] vertex/indirect buffers handle = 0x%08lx\n",
	       info->bufHandle);

    if (drmMap(ctx->drmFD, info->bufHandle, info->bufMapSize,
	       (drmAddressPtr)&info->buf) < 0) {
	fprintf(stderr,
		   "[pci] Could not map vertex/indirect buffers\n");
	return GL_FALSE;
    }
    fprintf(stderr,
	       "[pci] Vertex/indirect buffers mapped at 0x%08lx\n",
	       (unsigned long)info->buf);
    fprintf(stderr,
	       "[pci] Vertex/indirect buffers contents 0x%08lx\n",
	       *(unsigned long *)info->buf);

    if (!info->IsPCI) {
	/* This is really an AGP card, force PCI GART mode */
        chunk = INREG(R128_BM_CHUNK_0_VAL);
        chunk |= (R128_BM_PTR_FORCE_TO_PCI |
		  R128_BM_PM4_RD_FORCE_TO_PCI |
		  R128_BM_GLOBAL_FORCE_TO_PCI);
        OUTREG(R128_BM_CHUNK_0_VAL, chunk);
        OUTREG(R128_PCI_GART_PAGE, 0); /* Ensure PCI GART is used */
    }

    return GL_TRUE;
}

/* Add a map for the MMIO registers that will be accessed by any
   DRI-based clients. */
static GLboolean R128DRIMapInit(const DRIDriverContext *ctx)
{
    R128InfoPtr info = ctx->driverPrivate;
    int flags;

    if (info->CCESecure) flags = DRM_READ_ONLY;
    else                 flags = 0;

				/* Map registers */
    if (drmAddMap(ctx->drmFD, ctx->MMIOStart, ctx->MMIOSize,
		  DRM_REGISTERS, flags, &info->registerHandle) < 0) {
	return GL_FALSE;
    }
    fprintf(stderr,
	       "[drm] register handle = 0x%08x\n", info->registerHandle);

    return GL_TRUE;
}

/* Initialize the kernel data structures. */
static int R128DRIKernelInit(const DRIDriverContext *ctx)
{
    R128InfoPtr info = ctx->driverPrivate;
    drm_r128_init_t drmInfo;

    memset( &drmInfo, 0, sizeof(&drmInfo) );

    drmInfo.func                = R128_INIT_CCE;
    drmInfo.sarea_priv_offset   = sizeof(drm_sarea_t);
    drmInfo.is_pci              = info->IsPCI;
    drmInfo.cce_mode            = info->CCEMode;
    drmInfo.cce_secure          = info->CCESecure;
    drmInfo.ring_size           = info->ringSize*1024*1024;
    drmInfo.usec_timeout        = info->CCEusecTimeout;

    drmInfo.fb_bpp              = ctx->bpp;
    drmInfo.depth_bpp           = ctx->bpp;

    drmInfo.front_offset        = info->frontOffset;
    drmInfo.front_pitch         = info->frontPitch;

    drmInfo.back_offset         = info->backOffset;
    drmInfo.back_pitch          = info->backPitch;

    drmInfo.depth_offset        = info->depthOffset;
    drmInfo.depth_pitch         = info->depthPitch;
    drmInfo.span_offset         = info->spanOffset;

    drmInfo.fb_offset           = info->LinearAddr;
    drmInfo.mmio_offset         = info->registerHandle;
    drmInfo.ring_offset         = info->ringHandle;
    drmInfo.ring_rptr_offset    = info->ringReadPtrHandle;
    drmInfo.buffers_offset      = info->bufHandle;
    drmInfo.agp_textures_offset = info->agpTexHandle;

    if (drmCommandWrite(ctx->drmFD, DRM_R128_INIT,
                        &drmInfo, sizeof(drmInfo)) < 0)
        return GL_FALSE;

    return GL_TRUE;
}

/* Add a map for the vertex buffers that will be accessed by any
   DRI-based clients. */
static GLboolean R128DRIBufInit(const DRIDriverContext *ctx)
{
    R128InfoPtr info = ctx->driverPrivate;
				/* Initialize vertex buffers */
    if (info->IsPCI) {
	info->bufNumBufs = drmAddBufs(ctx->drmFD,
				      info->bufMapSize / R128_BUFFER_SIZE,
				      R128_BUFFER_SIZE,
				      DRM_SG_BUFFER,
				      info->bufStart);
    } else {
	info->bufNumBufs = drmAddBufs(ctx->drmFD,
				      info->bufMapSize / R128_BUFFER_SIZE,
				      R128_BUFFER_SIZE,
				      DRM_AGP_BUFFER,
				      info->bufStart);
    }
    if (info->bufNumBufs <= 0) {
	fprintf(stderr,
		   "[drm] Could not create vertex/indirect buffers list\n");
	return GL_FALSE;
    }
    fprintf(stderr,
	       "[drm] Added %d %d byte vertex/indirect buffers\n",
	       info->bufNumBufs, R128_BUFFER_SIZE);

    if (!(info->buffers = drmMapBufs(ctx->drmFD))) {
	fprintf(stderr,
		   "[drm] Failed to map vertex/indirect buffers list\n");
	return GL_FALSE;
    }
    fprintf(stderr,
	       "[drm] Mapped %d vertex/indirect buffers\n",
	       info->buffers->count);

    return GL_TRUE;
}

static void R128DRIIrqInit(const DRIDriverContext *ctx)
{
   R128InfoPtr info = ctx->driverPrivate;
   unsigned char *R128MMIO = ctx->MMIOAddress;
   
   if (!info->irq) {
       info->irq = drmGetInterruptFromBusID(
	   ctx->drmFD,
	   ctx->pciBus,
	   ctx->pciDevice,
	   ctx->pciFunc);

      if((drmCtlInstHandler(ctx->drmFD, info->irq)) != 0) {
	 fprintf(stderr,
		    "[drm] failure adding irq handler, "
		    "there is a device already using that irq\n"
		    "[drm] falling back to irq-free operation\n");
	 info->irq = 0;
      } else {
          info->gen_int_cntl = INREG( R128_GEN_INT_CNTL );
      }
   }

   if (info->irq)
      fprintf(stderr,
		 "[drm] dma control initialized, using IRQ %d\n",
		 info->irq);
}

static int R128CCEStop(const DRIDriverContext *ctx)
{
    R128InfoPtr info = ctx->driverPrivate;
    drm_r128_cce_stop_t stop;
    int            ret, i;

    stop.flush = 1;
    stop.idle  = 1;

    ret = drmCommandWrite( ctx->drmFD, DRM_R128_CCE_STOP,
                           &stop, sizeof(stop) );

    if ( ret == 0 ) {
        return 0;
    } else if ( errno != EBUSY ) {
        return -errno;
    }

    stop.flush = 0;

    i = 0;
    do {
        ret = drmCommandWrite( ctx->drmFD, DRM_R128_CCE_STOP,
                               &stop, sizeof(stop) );
    } while ( ret && errno == EBUSY && i++ < R128_IDLE_RETRY );

    if ( ret == 0 ) {
        return 0;
    } else if ( errno != EBUSY ) {
        return -errno;
    }

    stop.idle = 0;

    if ( drmCommandWrite( ctx->drmFD, DRM_R128_CCE_STOP,
                          &stop, sizeof(stop) )) {
        return -errno;
    } else {
        return 0;
    }
}

/* Initialize the CCE state, and start the CCE (if used by the X server) */
static void R128DRICCEInit(const DRIDriverContext *ctx)
{
   R128InfoPtr info = ctx->driverPrivate;

				/* Turn on bus mastering */
    info->BusCntl &= ~R128_BUS_MASTER_DIS;

				/* CCEMode is initialized in r128_driver.c */
    switch (info->CCEMode) {
    case R128_PM4_NONPM4:                 info->CCEFifoSize = 0;   break;
    case R128_PM4_192PIO:                 info->CCEFifoSize = 192; break;
    case R128_PM4_192BM:                  info->CCEFifoSize = 192; break;
    case R128_PM4_128PIO_64INDBM:         info->CCEFifoSize = 128; break;
    case R128_PM4_128BM_64INDBM:          info->CCEFifoSize = 128; break;
    case R128_PM4_64PIO_128INDBM:         info->CCEFifoSize = 64;  break;
    case R128_PM4_64BM_128INDBM:          info->CCEFifoSize = 64;  break;
    case R128_PM4_64PIO_64VCBM_64INDBM:   info->CCEFifoSize = 64;  break;
    case R128_PM4_64BM_64VCBM_64INDBM:    info->CCEFifoSize = 64;  break;
    case R128_PM4_64PIO_64VCPIO_64INDPIO: info->CCEFifoSize = 64;  break;
    }

    /* Make sure the CCE is on for the X server */
    R128CCE_START(ctx, info);
}


static int R128MemoryInit(const DRIDriverContext *ctx)
{
   R128InfoPtr info = ctx->driverPrivate;
   int        width_bytes = ctx->shared.virtualWidth * ctx->cpp;
   int        cpp         = ctx->cpp;
   int        bufferSize  = ((ctx->shared.virtualHeight * width_bytes
			      + R128_BUFFER_ALIGN)
			     & ~R128_BUFFER_ALIGN);
   int        depthSize   = ((((ctx->shared.virtualHeight+15) & ~15) * width_bytes
			      + R128_BUFFER_ALIGN)
			     & ~R128_BUFFER_ALIGN);
   int        l;

   info->frontOffset = 0;
   info->frontPitch = ctx->shared.virtualWidth;

   fprintf(stderr, 
	   "Using %d MB AGP aperture\n", info->agpSize);
   fprintf(stderr, 
	   "Using %d MB for the ring buffer\n", info->ringSize);
   fprintf(stderr, 
	   "Using %d MB for vertex/indirect buffers\n", info->bufSize);
   fprintf(stderr, 
	   "Using %d MB for AGP textures\n", info->agpTexSize);

   /* Front, back and depth buffers - everything else texture??
    */
   info->textureSize = ctx->shared.fbSize - 2 * bufferSize - depthSize;

   if (info->textureSize < 0) 
      return 0;

   l = R128MinBits((info->textureSize-1) / R128_NR_TEX_REGIONS);
   if (l < R128_LOG_TEX_GRANULARITY) l = R128_LOG_TEX_GRANULARITY;

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
   info->textureOffset = ((ctx->shared.fbSize - info->textureSize +
			   R128_BUFFER_ALIGN) &
			  ~R128_BUFFER_ALIGN);

   /* Reserve space for the shared depth
    * buffer.
    */
   info->depthOffset = ((info->textureOffset - depthSize +
			 R128_BUFFER_ALIGN) &
			~R128_BUFFER_ALIGN);
   info->depthPitch = ctx->shared.virtualWidth;

   info->backOffset = ((info->depthOffset - bufferSize +
			R128_BUFFER_ALIGN) &
		       ~R128_BUFFER_ALIGN);
   info->backPitch = ctx->shared.virtualWidth;


   fprintf(stderr, 
	   "Will use back buffer at offset 0x%x\n",
	   info->backOffset);
   fprintf(stderr, 
	   "Will use depth buffer at offset 0x%x\n",
	   info->depthOffset);
   fprintf(stderr, 
	   "Will use %d kb for textures at offset 0x%x\n",
	   info->textureSize/1024, info->textureOffset);

   return 1;
} 


/* Initialize the screen-specific data structures for the DRI and the
   Rage 128.  This is the main entry point to the device-specific
   initialization code.  It calls device-independent DRI functions to
   create the DRI data structures and initialize the DRI state. */
static GLboolean R128DRIScreenInit(DRIDriverContext *ctx)
{
    R128InfoPtr info = ctx->driverPrivate;
    R128DRIPtr    pR128DRI;
    int           err, major, minor, patch;
    drmVersionPtr version;
    drm_r128_sarea_t *pSAREAPriv;

    switch (ctx->bpp) {
    case 8:
	/* These modes are not supported (yet). */
    case 15:
    case 24:
	fprintf(stderr,
		   "[dri] R128DRIScreenInit failed (depth %d not supported).  "
		   "[dri] Disabling DRI.\n", ctx->bpp);
	return GL_FALSE;

	/* Only 16 and 32 color depths are supports currently. */
    case 16:
    case 32:
	break;
    }
    r128_drm_page_size = getpagesize();
    
    info->registerSize = ctx->MMIOSize;
    ctx->shared.SAREASize = SAREA_MAX;

    /* Note that drmOpen will try to load the kernel module, if needed. */
    ctx->drmFD = drmOpen("r128", NULL );
    if (ctx->drmFD < 0) {
	fprintf(stderr, "[drm] drmOpen failed\n");
	return 0;
    }
    
    /* Check the r128 DRM version */
    version = drmGetVersion(ctx->drmFD);
    if (version) {
	if (version->version_major != 2 ||
	    version->version_minor < 2) {
	    /* incompatible drm version */
	    fprintf(stderr,
		"[dri] R128DRIScreenInit failed because of a version mismatch.\n"
		"[dri] r128.o kernel module version is %d.%d.%d but version 2.2 or greater is needed.\n"
		"[dri] Disabling the DRI.\n",
		version->version_major,
		version->version_minor,
		version->version_patchlevel);
	    drmFreeVersion(version);
	    return GL_FALSE;
	}
	info->drmMinor = version->version_minor;
	drmFreeVersion(version);
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
   
   /* Need to AddMap the framebuffer and mmio regions here:
    */
   if (drmAddMap( ctx->drmFD,
		  (drm_handle_t)ctx->FBStart,
		  ctx->FBSize,
		  DRM_FRAME_BUFFER,
		  0,
		  &ctx->shared.hFrameBuffer) < 0)
   {
      fprintf(stderr, "[drm] drmAddMap framebuffer failed\n");
      return 0;
   }

   fprintf(stderr, "[drm] framebuffer handle = 0x%08lx\n",
	   ctx->shared.hFrameBuffer);

   if (!R128MemoryInit(ctx))
	return GL_FALSE;
   
				/* Initialize AGP */
    if (!info->IsPCI && !R128DRIAgpInit(ctx)) {
	info->IsPCI = GL_TRUE;
	fprintf(stderr,
		   "[agp] AGP failed to initialize -- falling back to PCI mode.\n");
	fprintf(stderr,
		   "[agp] Make sure you have the agpgart kernel module loaded.\n");
    }

				/* Initialize PCIGART */
    if (info->IsPCI && !R128DRIPciInit(ctx)) {
	return GL_FALSE;
    }

				/* DRIScreenInit doesn't add all the
				   common mappings.  Add additional
				   mappings here. */
    if (!R128DRIMapInit(ctx)) {
	return GL_FALSE;
    }

   /* Create a 'server' context so we can grab the lock for
    * initialization ioctls.
    */
   if ((err = drmCreateContext(ctx->drmFD, &ctx->serverContext)) != 0) {
      fprintf(stderr, "%s: drmCreateContext failed %d\n", __FUNCTION__, err);
      return 0;
   }

   DRM_LOCK(ctx->drmFD, ctx->pSAREA, ctx->serverContext, 0); 

    /* Initialize the kernel data structures */
    if (!R128DRIKernelInit(ctx)) {
	return GL_FALSE;
    }

    /* Initialize the vertex buffers list */
    if (!R128DRIBufInit(ctx)) {
	return GL_FALSE;
    }

    /* Initialize IRQ */
    R128DRIIrqInit(ctx);

    /* Initialize and start the CCE if required */
    R128DRICCEInit(ctx);

   /* Quick hack to clear the front & back buffers.  Could also use
    * the clear ioctl to do this, but would need to setup hw state
    * first.
    */
   drimemsetio((char *)ctx->FBAddress + info->frontOffset,
	  0,
	  info->frontPitch * ctx->cpp * ctx->shared.virtualHeight );

   drimemsetio((char *)ctx->FBAddress + info->backOffset,
	  0,
	  info->backPitch * ctx->cpp * ctx->shared.virtualHeight );
    
    pSAREAPriv = (drm_r128_sarea_t *)(((char*)ctx->pSAREA) + 
					sizeof(drm_sarea_t));
    memset(pSAREAPriv, 0, sizeof(*pSAREAPriv));

   /* This is the struct passed to radeon_dri.so for its initialization */
   ctx->driverClientMsg = malloc(sizeof(R128DRIRec));
   ctx->driverClientMsgSize = sizeof(R128DRIRec);
   
    pR128DRI                    = (R128DRIPtr)ctx->driverClientMsg;
    pR128DRI->deviceID          = info->Chipset;
    pR128DRI->width             = ctx->shared.virtualWidth;
    pR128DRI->height            = ctx->shared.virtualHeight;
    pR128DRI->depth             = ctx->bpp;
    pR128DRI->bpp               = ctx->bpp;

    pR128DRI->IsPCI             = info->IsPCI;
    pR128DRI->AGPMode           = info->agpMode;

    pR128DRI->frontOffset       = info->frontOffset;
    pR128DRI->frontPitch        = info->frontPitch;
    pR128DRI->backOffset        = info->backOffset;
    pR128DRI->backPitch         = info->backPitch;
    pR128DRI->depthOffset       = info->depthOffset;
    pR128DRI->depthPitch        = info->depthPitch;
    pR128DRI->spanOffset        = info->spanOffset;
    pR128DRI->textureOffset     = info->textureOffset;
    pR128DRI->textureSize       = info->textureSize;
    pR128DRI->log2TexGran       = info->log2TexGran;

    pR128DRI->registerHandle    = info->registerHandle;
    pR128DRI->registerSize      = info->registerSize;

    pR128DRI->agpTexHandle      = info->agpTexHandle;
    pR128DRI->agpTexMapSize     = info->agpTexMapSize;
    pR128DRI->log2AGPTexGran    = info->log2AGPTexGran;
    pR128DRI->agpTexOffset      = info->agpTexStart;
    pR128DRI->sarea_priv_offset = sizeof(drm_sarea_t);

    return GL_TRUE;
}

/* The screen is being closed, so clean up any state and free any
   resources used by the DRI. */
void R128DRICloseScreen(const DRIDriverContext *ctx)
{
    R128InfoPtr info = ctx->driverPrivate;
    drm_r128_init_t drmInfo;

    /* Stop the CCE if it is still in use */
    R128CCE_STOP(ctx, info);

    if (info->irq) {
	drmCtlUninstHandler(ctx->drmFD);
	info->irq = 0;
    }

    /* De-allocate vertex buffers */
    if (info->buffers) {
	drmUnmapBufs(info->buffers);
	info->buffers = NULL;
    }

    /* De-allocate all kernel resources */
    memset(&drmInfo, 0, sizeof(drmInfo));
    drmInfo.func = R128_CLEANUP_CCE;
    drmCommandWrite(ctx->drmFD, DRM_R128_INIT,
                    &drmInfo, sizeof(drmInfo));

    /* De-allocate all AGP resources */
    if (info->agpTex) {
	drmUnmap(info->agpTex, info->agpTexMapSize);
	info->agpTex = NULL;
    }
    if (info->buf) {
	drmUnmap(info->buf, info->bufMapSize);
	info->buf = NULL;
    }
    if (info->ringReadPtr) {
	drmUnmap(info->ringReadPtr, info->ringReadMapSize);
	info->ringReadPtr = NULL;
    }
    if (info->ring) {
	drmUnmap(info->ring, info->ringMapSize);
	info->ring = NULL;
    }
    if (info->agpMemHandle != DRM_AGP_NO_HANDLE) {
	drmAgpUnbind(ctx->drmFD, info->agpMemHandle);
	drmAgpFree(ctx->drmFD, info->agpMemHandle);
	info->agpMemHandle = 0;
	drmAgpRelease(ctx->drmFD);
    }
    if (info->pciMemHandle) {
	drmScatterGatherFree(ctx->drmFD, info->pciMemHandle);
	info->pciMemHandle = 0;
    }
}

static GLboolean R128PreInitDRI(const DRIDriverContext *ctx)
{
    R128InfoPtr info = ctx->driverPrivate;

    /*info->CCEMode = R128_DEFAULT_CCE_PIO_MODE;*/
    info->CCEMode = R128_DEFAULT_CCE_BM_MODE;
    info->CCESecure = GL_TRUE;

    info->agpMode        = R128_DEFAULT_AGP_MODE;
    info->agpSize        = R128_DEFAULT_AGP_SIZE;
    info->ringSize       = R128_DEFAULT_RING_SIZE;
    info->bufSize        = R128_DEFAULT_BUFFER_SIZE;
    info->agpTexSize     = R128_DEFAULT_AGP_TEX_SIZE;

    info->CCEusecTimeout = R128_DEFAULT_CCE_TIMEOUT;

    return GL_TRUE;
}

/**
 * \brief Initialize the framebuffer device mode
 *
 * \param ctx display handle.
 *
 * \return one on success, or zero on failure.
 *
 * Fills in \p info with some default values and some information from \p ctx
 * and then calls R128ScreenInit() for the screen initialization.
 * 
 * Before exiting clears the framebuffer memory accessing it directly.
 */
static int R128InitFBDev( DRIDriverContext *ctx )
{
   R128InfoPtr info = calloc(1, sizeof(*info));

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

   switch (info->Chipset) {
   case PCI_DEVICE_ID_ATI_RAGE128_LE:
   case PCI_DEVICE_ID_ATI_RAGE128_RE:
   case PCI_DEVICE_ID_ATI_RAGE128_RK:
   case PCI_DEVICE_ID_ATI_RAGE128_PD:
   case PCI_DEVICE_ID_ATI_RAGE128_PP:
   case PCI_DEVICE_ID_ATI_RAGE128_PR:
       /* This is a PCI card */
       info->IsPCI = GL_TRUE;
       break;
   default:
       /* This is an AGP card */
       info->IsPCI = GL_FALSE;
       break;
   }

   info->frontPitch = ctx->shared.virtualWidth;
   info->LinearAddr = ctx->FBStart & 0xfc000000;
   
   if (!R128PreInitDRI(ctx))
       return 0;

   if (!R128DRIScreenInit(ctx))
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
static void R128HaltFBDev( DRIDriverContext *ctx )
{
    drmUnmap( ctx->pSAREA, ctx->shared.SAREASize );
    drmClose(ctx->drmFD);

    if (ctx->driverPrivate) {
       free(ctx->driverPrivate);
       ctx->driverPrivate = 0;
    }
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
 * \sa R128PostValidateMode().
 */
static int R128ValidateMode( const DRIDriverContext *ctx )
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
 * \sa R128ValidateMode().
 */
static int R128PostValidateMode( const DRIDriverContext *ctx )
{
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
static int R128EngineShutdown( const DRIDriverContext *ctx )
{
    return 1;
}

/**
 * \brief Restore the drawing engine.
 *
 * \param ctx display handle
 *
 * Resets the graphics card and sets initial values for several registers of
 * the card's drawing engine.
 *
 * Turns on the R128 command processor engine (i.e., the ringbuffer).
 */
static int R128EngineRestore( const DRIDriverContext *ctx )
{
   return 1;
}


/**
 * \brief Exported driver interface for Mini GLX.
 *
 * \sa DRIDriverRec.
 */
const struct DRIDriverRec __driDriver = {
   R128ValidateMode,
   R128PostValidateMode,
   R128InitFBDev,
   R128HaltFBDev,
   R128EngineShutdown,
   R128EngineRestore,  
   0,
};
