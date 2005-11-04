/*
 * Mesa 3-D graphics library
 * Version:  5.1
 *
 * Copyright (C) 1999-2003  Brian Paul   All Rights Reserved.
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

/* Authors:
 *    Keith Whitwell
 *    Daniel Borca
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
 
#include "driver.h"
#include "drm.h"
#include "imports.h"

#include "dri_util.h"

#include "tdfx_context.h"
#include "tdfx_dri.h"
#include "xf86drm.h"


#define TILE_WIDTH 128
#define TILE_HEIGHT 32

#define CMDFIFO_PAGES 64


static int
calcBufferStride (int xres, int tiled, int cpp)
{
  int strideInTiles;

  if (tiled) {
    /* Calculate tile width stuff */
    strideInTiles = (xres+TILE_WIDTH-1)/TILE_WIDTH;

    return strideInTiles*cpp*TILE_WIDTH;
  } else {
    return xres*cpp;
  }
} /* calcBufferStride */


static int
calcBufferHeightInTiles (int yres)
{
  int heightInTiles;            /* Height of buffer in tiles */

  /* Calculate tile height stuff */
  heightInTiles = yres >> 5;

  if (yres & (TILE_HEIGHT - 1))
    heightInTiles++;

  return heightInTiles;

} /* calcBufferHeightInTiles */


static int
calcBufferSize (int xres, int yres, int tiled, int cpp)
{
  int stride, height, bufSize;

  if (tiled) {
    stride = calcBufferStride(xres, tiled, cpp);
    height = TILE_HEIGHT * calcBufferHeightInTiles(yres);
  } else {
    stride = xres*cpp;
    height = yres;
  }

  bufSize = stride * height;

  return bufSize;
} /* calcBufferSize */


static void allocateMemory (const DRIDriverContext *ctx, TDFXDRIPtr pTDFX)
{
  int memRemaining, fifoSize, screenSizeInTiles;
  int fbSize;
  char *str;
  int pixmapCacheLinesMin;
  int cursorOffset, cursorSize;

  pTDFX->stride = calcBufferStride(pTDFX->width, !0, pTDFX->cpp);

  /* enough to do DVD */
  pixmapCacheLinesMin = ((720*480*pTDFX->cpp) + 
					pTDFX->stride - 1)/pTDFX->stride;

  if (pTDFX->deviceID > PCI_CHIP_VOODOO3) {
  	if ((pixmapCacheLinesMin + pTDFX->height) > 4095)
		pixmapCacheLinesMin = 4095 - pTDFX->height;
  } else {
  	if ((pixmapCacheLinesMin + pTDFX->height) > 2047)
		pixmapCacheLinesMin = 2047 - pTDFX->height;
  }

  if (pTDFX->cpp!=3) {
    screenSizeInTiles=calcBufferSize(pTDFX->width, pTDFX->height,
				     !0, pTDFX->cpp);
  }
  else {
    /* cpp==3 needs to bump up to 4 */
    screenSizeInTiles=calcBufferSize(pTDFX->width, pTDFX->height,
				     !0, 4);
  }

  /*
   * Layout is:
   *    cursor, fifo, fb, tex, bb, db
   */

  fbSize = (pTDFX->height + pixmapCacheLinesMin) * pTDFX->stride;

  memRemaining=(pTDFX->mem - 1) &~ 0xFFF;
  /* Note that a page is 4096 bytes, and a  */
  /* tile is 32 x 128 = 4096 bytes.  So,    */
  /* page and tile boundaries are the same  */
  /* Place the depth offset first, forcing  */
  /* it to be on an *odd* page boundary.    */
  pTDFX->depthOffset = (memRemaining - screenSizeInTiles) &~ 0xFFF;
  if ((pTDFX->depthOffset & (0x1 << 12)) == 0) {
      pTDFX->depthOffset -= (0x1 << 12);
  }
  /* Now, place the back buffer, forcing it */
  /* to be on an *even* page boundary.      */
  pTDFX->backOffset = (pTDFX->depthOffset - screenSizeInTiles) &~ 0xFFF;
  if (pTDFX->backOffset & (0x1 << 12)) {
      pTDFX->backOffset -= (0x1 << 12);
  }
  /* Give the cmd fifo at least             */
  /* CMDFIFO_PAGES pages, but no more than  */
  /* 64. NOTE: Don't go higher than 64, as  */
  /* there is suspect code in Glide3 !      */
  fifoSize = ((64 <= CMDFIFO_PAGES) ? 64 : CMDFIFO_PAGES) << 12;

  /* We give 4096 bytes to the cursor  */
  cursorSize = 0/*4096*/;
  cursorOffset = 0;

  pTDFX->fifoOffset = cursorOffset + cursorSize;
  pTDFX->fifoSize = fifoSize;
  /* Now, place the front buffer, forcing   */
  /* it to be on a page boundary too, just  */
  /* for giggles.                           */
  pTDFX->fbOffset = pTDFX->fifoOffset + pTDFX->fifoSize;
  pTDFX->textureOffset = pTDFX->fbOffset + fbSize;
  if (pTDFX->depthOffset <= pTDFX->textureOffset ||
	pTDFX->backOffset <= pTDFX->textureOffset) {
    /*
     * pTDFX->textureSize < 0 means that the DRI is disabled.  pTDFX->backOffset
     * is used to calculate the maximum amount of memory available for
     * 2D offscreen use.  With DRI disabled, set this to the top of memory.
     */

    pTDFX->textureSize = -1;
    pTDFX->backOffset = pTDFX->mem;
    pTDFX->depthOffset = -1;
    fprintf(stderr, 
        "Not enough video memory available for textures and depth buffer\n"
	"\tand/or back buffer.  Disabling DRI.  To use DRI try lower\n"
	"\tresolution modes and/or a smaller virtual screen size\n");
  } else {
    pTDFX->textureSize = pTDFX->backOffset - pTDFX->textureOffset;
  }
}


static int createScreen (DRIDriverContext *ctx, TDFXDRIPtr pTDFX)
{
   int err;
   
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

   ctx->shared.SAREASize = SAREA_MAX;
   pTDFX->regsSize = ctx->MMIOSize;

   /* Note that drmOpen will try to load the kernel module, if needed. */
   ctx->drmFD = drmOpen("tdfx", NULL );
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


   if (drmAddMap(ctx->drmFD, 
		 ctx->MMIOStart,
		 ctx->MMIOSize,
		 DRM_REGISTERS, 
		 DRM_READ_ONLY, 
		 &pTDFX->regs) < 0) {
      fprintf(stderr, "[drm] drmAddMap mmio failed\n");	
      return 0;
   }
   fprintf(stderr,
	   "[drm] register handle = 0x%08lx\n", pTDFX->regs);


   /* Create a 'server' context so we can grab the lock for
    * initialization ioctls.
    */
   if ((err = drmCreateContext(ctx->drmFD, &ctx->serverContext)) != 0) {
      fprintf(stderr, "%s: drmCreateContext failed %d\n", __FUNCTION__, err);
      return 0;
   }

   DRM_LOCK(ctx->drmFD, ctx->pSAREA, ctx->serverContext, 0); 

   /* Initialize the kernel data structures */

   /* Initialize kernel gart memory manager */
   allocateMemory(ctx, pTDFX);

   /* Initialize the SAREA private data structure */


   /* Quick hack to clear the front & back buffers.  Could also use
    * the clear ioctl to do this, but would need to setup hw state
    * first.
    */


   /* This is the struct passed to tdfx_dri.so for its initialization */
   ctx->driverClientMsg = malloc(sizeof(TDFXDRIRec));
   ctx->driverClientMsgSize = sizeof(TDFXDRIRec);
   memcpy(ctx->driverClientMsg, pTDFX, ctx->driverClientMsgSize);
   pTDFX = (TDFXDRIPtr)ctx->driverClientMsg;

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
 * \sa tdfxValidateMode().
 */
static int tdfxValidateMode( const DRIDriverContext *ctx )
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
 * \sa tdfxValidateMode().
 */
static int tdfxPostValidateMode( const DRIDriverContext *ctx )
{
   return 1;
}


/**
 * \brief Initialize the framebuffer device mode
 *
 * \param ctx display handle.
 *
 * \return one on success, or zero on failure.
 *
 * Before exiting clears the framebuffer memory accessing it directly.
 */
static int tdfxInitFBDev( DRIDriverContext *ctx )
{
   TDFXDRIPtr pTDFX = calloc(1, sizeof(TDFXDRIRec));

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

   ctx->driverPrivate = (void *)pTDFX;

   pTDFX->deviceID = ctx->chipset;
   pTDFX->width    = ctx->shared.virtualWidth;
   pTDFX->height   = ctx->shared.virtualHeight;
   pTDFX->cpp      = ctx->cpp;
   pTDFX->mem      = ctx->FBSize; /* ->shared.fbSize? mem probe? */
   pTDFX->sarea_priv_offset = sizeof(drm_sarea_t);

   if (!createScreen(ctx, pTDFX))
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
static void tdfxHaltFBDev( DRIDriverContext *ctx )
{
    drmUnmap( ctx->pSAREA, ctx->shared.SAREASize );
    drmClose(ctx->drmFD);

    if (ctx->driverPrivate) {
       free(ctx->driverPrivate);
       ctx->driverPrivate = 0;
    }
}


/**
 * \brief Shutdown the drawing engine.
 *
 * \param ctx display handle
 *
 * Turns off the 3D engine & restores the graphics card
 * to a state that fbdev understands.
 */
static int tdfxEngineShutdown( const DRIDriverContext *ctx )
{
   fprintf(stderr, "%s: not implemented\n", __FUNCTION__);
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
 * Turns on 3dfx
 */
static int tdfxEngineRestore( const DRIDriverContext *ctx )
{
   fprintf(stderr, "%s: not implemented\n", __FUNCTION__);
   return 1;
}


/**
 * \brief Exported driver interface for Mini GLX.
 *
 * \sa DRIDriverRec.
 */
struct DRIDriverRec __driDriver = {
   tdfxValidateMode,
   tdfxPostValidateMode,
   tdfxInitFBDev,
   tdfxHaltFBDev,
   tdfxEngineShutdown,
   tdfxEngineRestore,
   0
};
