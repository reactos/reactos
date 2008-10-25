/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/via/via_dri.c,v 1.4 2003/09/24 02:43:30 dawes Exp $ */
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
 
#include "driver.h"
#include "drm.h"
#include "imports.h"

#include "dri_util.h"

#include "via_context.h"
#include "via_dri.h"
#include "via_driver.h"
#include "xf86drm.h"

static void VIAEnableMMIO(DRIDriverContext * ctx);
static void VIADisableMMIO(DRIDriverContext * ctx);
static void VIADisableExtendedFIFO(DRIDriverContext *ctx);
static void VIAEnableExtendedFIFO(DRIDriverContext *ctx);
static void VIAInitialize2DEngine(DRIDriverContext *ctx);
static void VIAInitialize3DEngine(DRIDriverContext *ctx);

static int VIADRIScreenInit(DRIDriverContext * ctx);
static void VIADRICloseScreen(DRIDriverContext * ctx);
static int VIADRIFinishScreenInit(DRIDriverContext * ctx);

/* _SOLO : missing macros normally defined by X code */
#define xf86DrvMsg(a, b, ...) fprintf(stderr, __VA_ARGS__)
#define MMIO_IN8(base, addr) ((*(((volatile u_int8_t*)base)+(addr)))+0)
#define MMIO_OUT8(base, addr, val) ((*(((volatile u_int8_t*)base)+(addr)))=((u_int8_t)val))
#define MMIO_OUT16(base, addr, val) ((*(volatile u_int16_t*)(((u_int8_t*)base)+(addr)))=((u_int16_t)val))

#define VIDEO	0 
#define AGP		1
#define AGP_PAGE_SIZE 4096
#define AGP_PAGES 8192
#define AGP_SIZE (AGP_PAGE_SIZE * AGP_PAGES)
#define AGP_CMDBUF_PAGES 512
#define AGP_CMDBUF_SIZE (AGP_PAGE_SIZE * AGP_CMDBUF_PAGES)

static char VIAKernelDriverName[] = "via";
static char VIAClientDriverName[] = "unichrome";

static int VIADRIAgpInit(const DRIDriverContext *ctx, VIAPtr pVia);
static int VIADRIPciInit(DRIDriverContext * ctx, VIAPtr pVia);
static int VIADRIFBInit(DRIDriverContext * ctx, VIAPtr pVia);
static int VIADRIKernelInit(DRIDriverContext * ctx, VIAPtr pVia);
static int VIADRIMapInit(DRIDriverContext * ctx, VIAPtr pVia);

static void VIADRIIrqInit( DRIDriverContext *ctx )
{
    VIAPtr pVia = VIAPTR(ctx);
    VIADRIPtr pVIADRI = pVia->devPrivate;

    pVIADRI->irqEnabled = drmGetInterruptFromBusID(pVia->drmFD,
					   ctx->pciBus,
					   ctx->pciDevice,
					   ctx->pciFunc);

    if ((drmCtlInstHandler(pVia->drmFD, pVIADRI->irqEnabled))) {
	xf86DrvMsg(pScreen->myNum, X_WARNING,
		   "[drm] Failure adding irq handler. "
		   "Falling back to irq-free operation.\n");
	pVIADRI->irqEnabled = 0;
    }

    if (pVIADRI->irqEnabled)
	xf86DrvMsg(pScreen->myNum, X_INFO,
		   "[drm] Irq handler installed, using IRQ %d.\n",
		   pVIADRI->irqEnabled);
}

static void VIADRIIrqExit( DRIDriverContext *ctx ) {
    VIAPtr pVia = VIAPTR(ctx);
    VIADRIPtr pVIADRI = pVia->devPrivate;

    if (pVIADRI->irqEnabled) {
	if (drmCtlUninstHandler(pVia->drmFD)) {
	    xf86DrvMsg(pScreen-myNum, X_INFO,"[drm] Irq handler uninstalled.\n");
	} else {
	    xf86DrvMsg(pScreen->myNum, X_ERROR,
		       "[drm] Could not uninstall irq handler.\n");
	}
    }
}
	    
static void VIADRIRingBufferCleanup(DRIDriverContext *ctx)
{
    VIAPtr pVia = VIAPTR(ctx);
    VIADRIPtr pVIADRI = pVia->devPrivate;
    drm_via_dma_init_t ringBufInit;

    if (pVIADRI->ringBufActive) {
	xf86DrvMsg(pScreen->myNum, X_INFO, 
		   "[drm] Cleaning up DMA ring-buffer.\n");
	ringBufInit.func = VIA_CLEANUP_DMA;
	if (drmCommandWrite(pVia->drmFD, DRM_VIA_DMA_INIT, &ringBufInit,
			    sizeof(ringBufInit))) {
	    xf86DrvMsg(pScreen->myNum, X_WARNING, 
		       "[drm] Failed to clean up DMA ring-buffer: %d\n", errno);
	}
	pVIADRI->ringBufActive = 0;
    }
}

static int VIADRIRingBufferInit(DRIDriverContext *ctx)
{
    VIAPtr pVia = VIAPTR(ctx);
    VIADRIPtr pVIADRI = pVia->devPrivate;
    drm_via_dma_init_t ringBufInit;
    drmVersionPtr drmVer;

    pVIADRI->ringBufActive = 0;

    if (NULL == (drmVer = drmGetVersion(pVia->drmFD))) {
	return GL_FALSE;
    }

    if (((drmVer->version_major <= 1) && (drmVer->version_minor <= 3))) {
	return GL_FALSE;
    } 

    /*
     * Info frome code-snippet on DRI-DEVEL list; Erdi Chen.
     */

    switch (pVia->ChipId) {
    case PCI_CHIP_VT3259:
    	ringBufInit.reg_pause_addr = 0x40c;
	break;
    default:
    	ringBufInit.reg_pause_addr = 0x418;
	break;
    }
   
    ringBufInit.offset = pVia->agpSize;
    ringBufInit.size = AGP_CMDBUF_SIZE;
    ringBufInit.func = VIA_INIT_DMA;
    if (drmCommandWrite(pVia->drmFD, DRM_VIA_DMA_INIT, &ringBufInit,
			sizeof(ringBufInit))) {
	xf86DrvMsg(pScreen->myNum, X_ERROR, 
		   "[drm] Failed to initialize DMA ring-buffer: %d\n", errno);
	return GL_FALSE;
    }
    xf86DrvMsg(pScreen->myNum, X_INFO, 
	       "[drm] Initialized AGP ring-buffer, size 0x%lx at AGP offset 0x%lx.\n",
	       ringBufInit.size, ringBufInit.offset);
   
    pVIADRI->ringBufActive = 1;
    return GL_TRUE;
}	    

static int VIADRIAgpInit(const DRIDriverContext *ctx, VIAPtr pVia)
{
    unsigned long  agp_phys;
    drmAddress agpaddr;
    VIADRIPtr pVIADRI;
    pVIADRI = pVia->devPrivate;
    pVia->agpSize = 0;

    if (drmAgpAcquire(pVia->drmFD) < 0) {
        xf86DrvMsg(pScreen->myNum, X_ERROR, "[drm] drmAgpAcquire failed %d\n", errno);
        return GL_FALSE;
    }

    if (drmAgpEnable(pVia->drmFD, drmAgpGetMode(pVia->drmFD)&~0x0) < 0) {
         xf86DrvMsg(pScreen->myNum, X_ERROR, "[drm] drmAgpEnable failed\n");
        return GL_FALSE;
    }
    
    xf86DrvMsg(pScreen->myNum, X_INFO, "[drm] drmAgpEnabled succeeded\n");

    if (drmAgpAlloc(pVia->drmFD, AGP_SIZE, 0, &agp_phys, &pVia->agpHandle) < 0) {
        xf86DrvMsg(pScreen->myNum, X_ERROR,
                 "[drm] drmAgpAlloc failed\n");
        drmAgpRelease(pVia->drmFD);
        return GL_FALSE;
    }
   
    if (drmAgpBind(pVia->drmFD, pVia->agpHandle, 0) < 0) {
        xf86DrvMsg(pScreen->myNum, X_ERROR,
                 "[drm] drmAgpBind failed\n");
        drmAgpFree(pVia->drmFD, pVia->agpHandle);
        drmAgpRelease(pVia->drmFD);

        return GL_FALSE;
    }

    /*
     * Place the ring-buffer last in the AGP region, and restrict the
     * public map not to include the buffer for security reasons.
     */

    pVia->agpSize = AGP_SIZE - AGP_CMDBUF_SIZE;
    pVia->agpAddr = drmAgpBase(pVia->drmFD);
    xf86DrvMsg(pScreen->myNum, X_INFO,
                 "[drm] agpAddr = 0x%08lx\n",pVia->agpAddr);
		 
    pVIADRI->agp.size = pVia->agpSize;
    if (drmAddMap(pVia->drmFD, (drm_handle_t)0,
                 pVIADRI->agp.size, DRM_AGP, 0, 
                 &pVIADRI->agp.handle) < 0) {
	xf86DrvMsg(pScreen->myNum, X_ERROR,
	    "[drm] Failed to map public agp area\n");
        pVIADRI->agp.size = 0;
        return GL_FALSE;
    }  
    /* Map AGP from kernel to Xserver - Not really needed */
    drmMap(pVia->drmFD, pVIADRI->agp.handle,pVIADRI->agp.size, &agpaddr);

    xf86DrvMsg(pScreen->myNum, X_INFO, 
                "[drm] agpAddr = 0x%08lx\n", pVia->agpAddr);
    xf86DrvMsg(pScreen->myNum, X_INFO, 
                "[drm] agpSize = 0x%08lx\n", pVia->agpSize);
    xf86DrvMsg(pScreen->myNum, X_INFO, 
                "[drm] agp physical addr = 0x%08lx\n", agp_phys);

    {
	drm_via_agp_t agp;
	agp.offset = 0;
	agp.size = AGP_SIZE-AGP_CMDBUF_SIZE;
	if (drmCommandWrite(pVia->drmFD, DRM_VIA_AGP_INIT, &agp,
			    sizeof(drm_via_agp_t)) < 0) {
	    drmUnmap(&agpaddr,pVia->agpSize);
	    drmRmMap(pVia->drmFD,pVIADRI->agp.handle);
	    drmAgpUnbind(pVia->drmFD, pVia->agpHandle);
	    drmAgpFree(pVia->drmFD, pVia->agpHandle);
	    drmAgpRelease(pVia->drmFD);
	    return GL_FALSE;
	}
    }

    return GL_TRUE;
}

static int VIADRIFBInit(DRIDriverContext * ctx, VIAPtr pVia)
{   
    int FBSize = pVia->FBFreeEnd-pVia->FBFreeStart;
    int FBOffset = pVia->FBFreeStart; 
    VIADRIPtr pVIADRI = pVia->devPrivate;
    pVIADRI->fbOffset = FBOffset;
    pVIADRI->fbSize = pVia->videoRambytes;

    {
	drm_via_fb_t fb;
	fb.offset = FBOffset;
	fb.size = FBSize;
	
	if (drmCommandWrite(pVia->drmFD, DRM_VIA_FB_INIT, &fb,
			    sizeof(drm_via_fb_t)) < 0) {
	    xf86DrvMsg(pScreen->myNum, X_ERROR,
		       "[drm] failed to init frame buffer area\n");
	    return GL_FALSE;
	} else {
	    xf86DrvMsg(pScreen->myNum, X_INFO,
		       "[drm] FBFreeStart= 0x%08x FBFreeEnd= 0x%08x "
		       "FBSize= 0x%08x\n",
		       pVia->FBFreeStart, pVia->FBFreeEnd, FBSize);
	    return GL_TRUE;	
	}   
    }
}

static int VIADRIPciInit(DRIDriverContext * ctx, VIAPtr pVia)
{
    return GL_TRUE;	
}

static int VIADRIScreenInit(DRIDriverContext * ctx)
{
    VIAPtr pVia = VIAPTR(ctx);
    VIADRIPtr pVIADRI;
    int err;

#if 0
    ctx->shared.SAREASize = ((sizeof(drm_sarea_t) + 0xfff) & 0x1000);
#else
    if (sizeof(drm_sarea_t)+sizeof(drm_via_sarea_t) > SAREA_MAX) {
	xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
			"Data does not fit in SAREA\n");
	return GL_FALSE;
    }
    ctx->shared.SAREASize = SAREA_MAX;
#endif

    ctx->drmFD = drmOpen(VIAKernelDriverName, NULL);
    if (ctx->drmFD < 0) {
        fprintf(stderr, "[drm] drmOpen failed\n");
        return 0;
    }
    pVia->drmFD = ctx->drmFD;

    err = drmSetBusid(ctx->drmFD, ctx->pciBusID);
    if (err < 0) {
        fprintf(stderr, "[drm] drmSetBusid failed (%d, %s), %s\n",
                ctx->drmFD, ctx->pciBusID, strerror(-err));
        return 0;
    }

    err = drmAddMap(ctx->drmFD, 0, ctx->shared.SAREASize, DRM_SHM,
                  DRM_CONTAINS_LOCK, &ctx->shared.hSAREA);
    if (err < 0) {
        fprintf(stderr, "[drm] drmAddMap failed\n");
        return 0;
    }
    fprintf(stderr, "[drm] added %d byte SAREA at 0x%08lx\n",
            ctx->shared.SAREASize, ctx->shared.hSAREA);

    if (drmMap(ctx->drmFD,
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
    if (drmAddMap(ctx->drmFD,
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

    pVIADRI = (VIADRIPtr) CALLOC(sizeof(VIADRIRec));
    if (!pVIADRI) {
        drmClose(ctx->drmFD);
        return GL_FALSE;
    }
    pVia->devPrivate = pVIADRI;
    ctx->driverClientMsg = pVIADRI;
    ctx->driverClientMsgSize = sizeof(*pVIADRI);

    /* DRIScreenInit doesn't add all the common mappings.  Add additional mappings here. */
    if (!VIADRIMapInit(ctx, pVia)) {
	VIADRICloseScreen(ctx);
	return GL_FALSE;
    }

    pVIADRI->regs.size = VIA_MMIO_REGSIZE;
    pVIADRI->regs.handle = pVia->registerHandle;
    xf86DrvMsg(pScreen->myNum, X_INFO, "[drm] mmio Registers = 0x%08lx\n",
	pVIADRI->regs.handle);

    if (drmMap(pVia->drmFD,
               pVIADRI->regs.handle,
               pVIADRI->regs.size,
               (drmAddress *)&pVia->MapBase) != 0)
    {
        VIADRICloseScreen(ctx);
        return GL_FALSE;
    }

    xf86DrvMsg(pScrn->scrnIndex, X_INFO, "[dri] mmio mapped.\n" );

    VIAEnableMMIO(ctx);

    /* Get video memory clock. */
    VGAOUT8(0x3D4, 0x3D);
    pVia->MemClk = (VGAIN8(0x3D5) & 0xF0) >> 4;
    xf86DrvMsg(0, X_INFO, "[dri] MemClk (0x%x)\n", pVia->MemClk);

    /* 3D rendering has noise if not enabled. */
    VIAEnableExtendedFIFO(ctx);

    VIAInitialize2DEngine(ctx);

    /* Must disable MMIO or 3D won't work. */
    VIADisableMMIO(ctx);

    VIAInitialize3DEngine(ctx);

    pVia->IsPCI = !VIADRIAgpInit(ctx, pVia);

    if (pVia->IsPCI) {
        VIADRIPciInit(ctx, pVia);
	xf86DrvMsg(pScrn->scrnIndex, X_INFO, "[dri] use pci.\n" );
    }
    else
        xf86DrvMsg(pScrn->scrnIndex, X_INFO, "[dri] use agp.\n" );

    if (!(VIADRIFBInit(ctx, pVia))) {
	VIADRICloseScreen(ctx);
        xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "[dri] frame buffer initialize fail .\n" );
        return GL_FALSE;
    }
    
    xf86DrvMsg(pScrn->scrnIndex, X_INFO, "[dri] frame buffer initialized.\n" );
 
    return VIADRIFinishScreenInit(ctx);
}

static void
VIADRICloseScreen(DRIDriverContext * ctx)
{
    VIAPtr pVia = VIAPTR(ctx);
    VIADRIPtr pVIADRI=(VIADRIPtr)pVia->devPrivate;

    VIADRIRingBufferCleanup(ctx);

    if (pVia->MapBase) {
	xf86DrvMsg(pScreen->myNum, X_INFO, "[drm] Unmapping MMIO registers\n");
        drmUnmap(pVia->MapBase, pVIADRI->regs.size);
    }

    if (pVia->agpSize) {
	xf86DrvMsg(pScreen->myNum, X_INFO, "[drm] Freeing agp memory\n");
        drmAgpFree(pVia->drmFD, pVia->agpHandle);
	xf86DrvMsg(pScreen->myNum, X_INFO, "[drm] Releasing agp module\n");
    	drmAgpRelease(pVia->drmFD);
    }

#if 0
    if (pVia->DRIIrqEnable) 
#endif
        VIADRIIrqExit(ctx);
}

static int
VIADRIFinishScreenInit(DRIDriverContext * ctx)
{
    VIAPtr pVia = VIAPTR(ctx);
    VIADRIPtr pVIADRI;
    int err;

    err = drmCreateContext(ctx->drmFD, &ctx->serverContext);
    if (err != 0) {
        fprintf(stderr, "%s: drmCreateContext failed %d\n", __FUNCTION__, err);
        return GL_FALSE;
    }

    DRM_LOCK(ctx->drmFD, ctx->pSAREA, ctx->serverContext, 0);


    if (!VIADRIKernelInit(ctx, pVia)) {
	VIADRICloseScreen(ctx);
	return GL_FALSE;
    }
    xf86DrvMsg(pScreen->myNum, X_INFO, "[dri] kernel data initialized.\n");

    /* set SAREA value */
    {
	drm_via_sarea_t *saPriv;

	saPriv=(drm_via_sarea_t*)(((char*)ctx->pSAREA) +
                               sizeof(drm_sarea_t));
	assert(saPriv);
	memset(saPriv, 0, sizeof(*saPriv));
	saPriv->ctxOwner = -1;
    }
    pVIADRI=(VIADRIPtr)pVia->devPrivate;
    pVIADRI->deviceID=pVia->Chipset;  
    pVIADRI->width=ctx->shared.virtualWidth;
    pVIADRI->height=ctx->shared.virtualHeight;
    pVIADRI->mem=ctx->shared.fbSize;
    pVIADRI->bytesPerPixel= (ctx->bpp+7) / 8; 
    pVIADRI->sarea_priv_offset = sizeof(drm_sarea_t);
    /* TODO */
    pVIADRI->scrnX=pVIADRI->width;
    pVIADRI->scrnY=pVIADRI->height;

    /* Initialize IRQ */
#if 0
    if (pVia->DRIIrqEnable) 
#endif
	VIADRIIrqInit(ctx);
    
    pVIADRI->ringBufActive = 0;
    VIADRIRingBufferInit(ctx);

    return GL_TRUE;
}

/* Initialize the kernel data structures. */
static int VIADRIKernelInit(DRIDriverContext * ctx, VIAPtr pVia)
{
    drm_via_init_t drmInfo;
    memset(&drmInfo, 0, sizeof(drm_via_init_t));
    drmInfo.sarea_priv_offset   = sizeof(drm_sarea_t);
    drmInfo.func = VIA_INIT_MAP;
    drmInfo.fb_offset           = pVia->FrameBufferBase;
    drmInfo.mmio_offset         = pVia->registerHandle;
    if (pVia->IsPCI)
	drmInfo.agpAddr = (u_int32_t)NULL;
    else
	drmInfo.agpAddr = (u_int32_t)pVia->agpAddr;

    if ((drmCommandWrite(pVia->drmFD, DRM_VIA_MAP_INIT,&drmInfo,
			     sizeof(drm_via_init_t))) < 0)
	    return GL_FALSE;

    return GL_TRUE;
}
/* Add a map for the MMIO registers */
static int VIADRIMapInit(DRIDriverContext * ctx, VIAPtr pVia)
{
    int flags = 0;

    if (drmAddMap(pVia->drmFD, pVia->MmioBase, VIA_MMIO_REGSIZE,
		  DRM_REGISTERS, flags, &pVia->registerHandle) < 0) {
	return GL_FALSE;
    }

    xf86DrvMsg(pScreen->myNum, X_INFO,
	"[drm] register handle = 0x%08lx\n", pVia->registerHandle);

    return GL_TRUE;
}

static int viaValidateMode(const DRIDriverContext *ctx)
{
    VIAPtr pVia = VIAPTR(ctx);

    return 1;
}

static int viaPostValidateMode(const DRIDriverContext *ctx)
{
    VIAPtr pVia = VIAPTR(ctx);

    return 1;
}

static void VIAEnableMMIO(DRIDriverContext * ctx)
{
    /*vgaHWPtr hwp = VGAHWPTR(ctx);*/
    VIAPtr pVia = VIAPTR(ctx);
    unsigned char val;

#if 0
    if (xf86IsPrimaryPci(pVia->PciInfo)) {
        /* If we are primary card, we still use std vga port. If we use
         * MMIO, system will hang in vgaHWSave when our card used in
         * PLE and KLE (integrated Trident MVP4)
         */
        vgaHWSetStdFuncs(hwp);
    }
    else {
        vgaHWSetMmioFuncs(hwp, pVia->MapBase, 0x8000);
    }
#endif

    val = VGAIN8(0x3c3);
    VGAOUT8(0x3c3, val | 0x01);
    val = VGAIN8(0x3cc);
    VGAOUT8(0x3c2, val | 0x01);

    /* Unlock Extended IO Space */
    VGAOUT8(0x3c4, 0x10);
    VGAOUT8(0x3c5, 0x01);

    /* Enable MMIO */
    if(!pVia->IsSecondary) {
	VGAOUT8(0x3c4, 0x1a);
	val = VGAIN8(0x3c5);
#ifdef DEBUG
	xf86DrvMsg(pScrn->scrnIndex, X_INFO, "primary val = %x\n", val);
#endif
	VGAOUT8(0x3c5, val | 0x68);
    }
    else {
	VGAOUT8(0x3c4, 0x1a);
	val = VGAIN8(0x3c5);
#ifdef DEBUG
	xf86DrvMsg(pScrn->scrnIndex, X_INFO, "secondary val = %x\n", val);
#endif
	VGAOUT8(0x3c5, val | 0x38);
    }

    /* Unlock CRTC registers */
    VGAOUT8(0x3d4, 0x47);
    VGAOUT8(0x3d5, 0x00);

    return;
}

static void VIADisableMMIO(DRIDriverContext * ctx)
{
    VIAPtr pVia = VIAPTR(ctx);
    unsigned char val;

    VGAOUT8(0x3c4, 0x1a);
    val = VGAIN8(0x3c5);
    VGAOUT8(0x3c5, val & 0x97);

    return;
}

static void VIADisableExtendedFIFO(DRIDriverContext *ctx)
{
    VIAPtr  pVia = VIAPTR(ctx);
    u_int32_t  dwGE230, dwGE298;

    /* Cause of exit XWindow will dump back register value, others chipset no
     * need to set extended fifo value */
    if (pVia->Chipset == VIA_CLE266 && pVia->ChipRev < 15 &&
        (ctx->shared.virtualWidth > 1024 || pVia->HasSecondary)) {
        /* Turn off Extend FIFO */
        /* 0x298[29] */
        dwGE298 = VIAGETREG(0x298);
        VIASETREG(0x298, dwGE298 | 0x20000000);
        /* 0x230[21] */
        dwGE230 = VIAGETREG(0x230);
        VIASETREG(0x230, dwGE230 & ~0x00200000);
        /* 0x298[29] */
        dwGE298 = VIAGETREG(0x298);
        VIASETREG(0x298, dwGE298 & ~0x20000000);
    }
}

static void VIAEnableExtendedFIFO(DRIDriverContext *ctx)
{
    VIAPtr  pVia = VIAPTR(ctx);
    u_int8_t   bRegTemp;
    u_int32_t  dwGE230, dwGE298;

    switch (pVia->Chipset) {
    case VIA_CLE266:
        if (pVia->ChipRev > 14) {  /* For 3123Cx */
            if (pVia->HasSecondary) {  /* SAMM or DuoView case */
                if (ctx->shared.virtualWidth >= 1024)
    	        {
    	            /* 3c5.16[0:5] */
        	        VGAOUT8(0x3C4, 0x16);
            	    bRegTemp = VGAIN8(0x3C5);
    	            bRegTemp &= ~0x3F;
        	        bRegTemp |= 0x1C;
            	    VGAOUT8(0x3C5, bRegTemp);
        	        /* 3c5.17[0:6] */
            	    VGAOUT8(0x3C4, 0x17);
                	bRegTemp = VGAIN8(0x3C5);
    	            bRegTemp &= ~0x7F;
        	        bRegTemp |= 0x3F;
            	    VGAOUT8(0x3C5, bRegTemp);
            	    pVia->EnableExtendedFIFO = GL_TRUE;
    	        }
            }
            else   /* Single view or Simultaneoue case */
            {
                if (ctx->shared.virtualWidth > 1024)
    	        {
    	            /* 3c5.16[0:5] */
        	        VGAOUT8(0x3C4, 0x16);
            	    bRegTemp = VGAIN8(0x3C5);
    	            bRegTemp &= ~0x3F;
        	        bRegTemp |= 0x17;
            	    VGAOUT8(0x3C5, bRegTemp);
        	        /* 3c5.17[0:6] */
            	    VGAOUT8(0x3C4, 0x17);
                	bRegTemp = VGAIN8(0x3C5);
    	            bRegTemp &= ~0x7F;
        	        bRegTemp |= 0x2F;
            	    VGAOUT8(0x3C5, bRegTemp);
            	    pVia->EnableExtendedFIFO = GL_TRUE;
    	        }
            }
            /* 3c5.18[0:5] */
            VGAOUT8(0x3C4, 0x18);
            bRegTemp = VGAIN8(0x3C5);
            bRegTemp &= ~0x3F;
            bRegTemp |= 0x17;
            bRegTemp |= 0x40;  /* force the preq always higher than treq */
            VGAOUT8(0x3C5, bRegTemp);
        }
        else {      /* for 3123Ax */
            if (ctx->shared.virtualWidth > 1024 || pVia->HasSecondary) {
                /* Turn on Extend FIFO */
                /* 0x298[29] */
                dwGE298 = VIAGETREG(0x298);
                VIASETREG(0x298, dwGE298 | 0x20000000);
                /* 0x230[21] */
                dwGE230 = VIAGETREG(0x230);
                VIASETREG(0x230, dwGE230 | 0x00200000);
                /* 0x298[29] */
                dwGE298 = VIAGETREG(0x298);
                VIASETREG(0x298, dwGE298 & ~0x20000000);

                /* 3c5.16[0:5] */
                VGAOUT8(0x3C4, 0x16);
                bRegTemp = VGAIN8(0x3C5);
                bRegTemp &= ~0x3F;
                bRegTemp |= 0x17;
                /* bRegTemp |= 0x10; */
                VGAOUT8(0x3C5, bRegTemp);
                /* 3c5.17[0:6] */
                VGAOUT8(0x3C4, 0x17);
                bRegTemp = VGAIN8(0x3C5);
                bRegTemp &= ~0x7F;
                bRegTemp |= 0x2F;
                /*bRegTemp |= 0x1F;*/
                VGAOUT8(0x3C5, bRegTemp);
                /* 3c5.18[0:5] */
                VGAOUT8(0x3C4, 0x18);
                bRegTemp = VGAIN8(0x3C5);
                bRegTemp &= ~0x3F;
                bRegTemp |= 0x17;
                bRegTemp |= 0x40;  /* force the preq always higher than treq */
                VGAOUT8(0x3C5, bRegTemp);
          	    pVia->EnableExtendedFIFO = GL_TRUE;
            }
        }
        break;
    case VIA_KM400:
        if (pVia->HasSecondary) {  /* SAMM or DuoView case */
            if ((ctx->shared.virtualWidth >= 1600) &&
                (pVia->MemClk <= VIA_MEM_DDR200)) {
        	    /* enable CRT extendded FIFO */
            	VGAOUT8(0x3C4, 0x17);
                VGAOUT8(0x3C5, 0x1C);
    	        /* revise second display queue depth and read threshold */
        	    VGAOUT8(0x3C4, 0x16);
            	bRegTemp = VGAIN8(0x3C5);
    	        bRegTemp &= ~0x3F;
    	        bRegTemp = (bRegTemp) | (0x09);
                VGAOUT8(0x3C5, bRegTemp);
            }
            else {
                /* enable CRT extendded FIFO */
                VGAOUT8(0x3C4, 0x17);
                VGAOUT8(0x3C5,0x3F);
                /* revise second display queue depth and read threshold */
                VGAOUT8(0x3C4, 0x16);
                bRegTemp = VGAIN8(0x3C5);
                bRegTemp &= ~0x3F;
                bRegTemp = (bRegTemp) | (0x1C);
                VGAOUT8(0x3C5, bRegTemp);
            }
            /* 3c5.18[0:5] */
            VGAOUT8(0x3C4, 0x18);
            bRegTemp = VGAIN8(0x3C5);
            bRegTemp &= ~0x3F;
            bRegTemp |= 0x17;
            bRegTemp |= 0x40;  /* force the preq always higher than treq */
            VGAOUT8(0x3C5, bRegTemp);
       	    pVia->EnableExtendedFIFO = GL_TRUE;
        }
        else {
            if ( (ctx->shared.virtualWidth > 1024) && (ctx->shared.virtualWidth <= 1280) )
            {
                /* enable CRT extendded FIFO */
                VGAOUT8(0x3C4, 0x17);
                VGAOUT8(0x3C5, 0x3F);
                /* revise second display queue depth and read threshold */
                VGAOUT8(0x3C4, 0x16);
                bRegTemp = VGAIN8(0x3C5);
                bRegTemp &= ~0x3F;
                bRegTemp = (bRegTemp) | (0x17);
                VGAOUT8(0x3C5, bRegTemp);
           	    pVia->EnableExtendedFIFO = GL_TRUE;
            }
            else if ((ctx->shared.virtualWidth > 1280))
            {
                /* enable CRT extendded FIFO */
                VGAOUT8(0x3C4, 0x17);
                VGAOUT8(0x3C5, 0x3F);
                /* revise second display queue depth and read threshold */
                VGAOUT8(0x3C4, 0x16);
                bRegTemp = VGAIN8(0x3C5);
                bRegTemp &= ~0x3F;
                bRegTemp = (bRegTemp) | (0x1C);
                VGAOUT8(0x3C5, bRegTemp);
           	    pVia->EnableExtendedFIFO = GL_TRUE;
            }
            else
            {
                /* enable CRT extendded FIFO */
                VGAOUT8(0x3C4, 0x17);
                VGAOUT8(0x3C5, 0x3F);
                /* revise second display queue depth and read threshold */
                VGAOUT8(0x3C4, 0x16);
                bRegTemp = VGAIN8(0x3C5);
                bRegTemp &= ~0x3F;
                bRegTemp = (bRegTemp) | (0x10);
                VGAOUT8(0x3C5, bRegTemp);
            }
            /* 3c5.18[0:5] */
            VGAOUT8(0x3C4, 0x18);
            bRegTemp = VGAIN8(0x3C5);
            bRegTemp &= ~0x3F;
            bRegTemp |= 0x17;
            bRegTemp |= 0x40;  /* force the preq always higher than treq */
            VGAOUT8(0x3C5, bRegTemp);
        }
        break;
    case VIA_K8M800:
        /*=* R1 Display FIFO depth (384 /8 -1 -> 0xbf) SR17[7:0] (8bits) *=*/
        VGAOUT8(0x3c4, 0x17);
        VGAOUT8(0x3c5, 0xbf);

        /*=* R2 Display fetch datum threshold value (328/4 -> 0x52)
             SR16[5:0], SR16[7] (7bits) *=*/
        VGAOUT8(0x3c4, 0x16);
        bRegTemp = VGAIN8(0x3c5) & ~0xBF;
        bRegTemp |= (0x52 & 0x3F);
        bRegTemp |= ((0x52 & 0x40) << 1);
        VGAOUT8(0x3c5, bRegTemp);

        /*=* R3 Switch to the highest agent threshold value (74 -> 0x4a)
             SR18[5:0], SR18[7] (7bits) *=*/
        VGAOUT8(0x3c4, 0x18);
        bRegTemp = VGAIN8(0x3c5) & ~0xBF;
        bRegTemp |= (0x4a & 0x3F);
        bRegTemp |= ((0x4a & 0x40) << 1);
        VGAOUT8(0x3c5, bRegTemp);
#if 0
        /*=* R4 Fetch Number for a scan line (unit: 8 bytes)
             SR1C[7:0], SR1D[1:0] (10bits) *=*/
        wRegTemp = (pBIOSInfo->offsetWidthByQWord >> 1) + 4;
        VGAOUT8(0x3c4, 0x1c);
        VGAOUT8(0x3c5, (u_int8_t)(wRegTemp & 0xFF));
        VGAOUT8(0x3c4, 0x1d);
        bRegTemp = VGAIN8(0x3c5) & ~0x03;
        VGAOUT8(0x3c5, bRegTemp | ((wRegTemp & 0x300) >> 8));
#endif
        if (ctx->shared.virtualWidth >= 1400 && ctx->bpp == 32)
        {
            /*=* Max. length for a request SR22[4:0] (64/4 -> 0x10) *=*/
            VGAOUT8(0x3c4, 0x22);
            bRegTemp = VGAIN8(0x3c5) & ~0x1F;
            VGAOUT8(0x3c5, bRegTemp | 0x10);
        }
        else
        {
            /*=* Max. length for a request SR22[4:0]
                 (128/4 -> over flow 0x0) *=*/
            VGAOUT8(0x3c4, 0x22);
            bRegTemp = VGAIN8(0x3c5) & ~0x1F;
            VGAOUT8(0x3c5, bRegTemp);
        }
        break;
    case VIA_PM800:
        /*=* R1 Display FIFO depth (96-1 -> 0x5f) SR17[7:0] (8bits) *=*/
        VGAOUT8(0x3c4, 0x17);
        VGAOUT8(0x3c5, 0x5f);

        /*=* R2 Display fetch datum threshold value (32 -> 0x20)
             SR16[5:0], SR16[7] (7bits) *=*/
        VGAOUT8(0x3c4, 0x16);
        bRegTemp = VGAIN8(0x3c5) & ~0xBF;
        bRegTemp |= (0x20 & 0x3F);
        bRegTemp |= ((0x20 & 0x40) << 1);
        VGAOUT8(0x3c5, bRegTemp);

        /*=* R3 Switch to the highest agent threshold value (16 -> 0x10)
             SR18[5:0], SR18[7] (7bits) *=*/
        VGAOUT8(0x3c4, 0x18);
        bRegTemp = VGAIN8(0x3c5) & ~0xBF;
        bRegTemp |= (0x10 & 0x3F);
        bRegTemp |= ((0x10 & 0x40) << 1);
        VGAOUT8(0x3c5, bRegTemp);
#if 0
        /*=* R4 Fetch Number for a scan line (unit: 8 bytes)
             SR1C[7:0], SR1D[1:0] (10bits) *=*/
        wRegTemp = (pBIOSInfo->offsetWidthByQWord >> 1) + 4;
        VGAOUT8(0x3c4, 0x1c);
        VGAOUT8(0x3c5, (u_int8_t)(wRegTemp & 0xFF));
        VGAOUT8(0x3c4, 0x1d);
        bRegTemp = VGAIN8(0x3c5) & ~0x03;
        VGAOUT8(0x3c5, bRegTemp | ((wRegTemp & 0x300) >> 8));
#endif
        if (ctx->shared.virtualWidth >= 1400 && ctx->bpp == 32)
        {
            /*=* Max. length for a request SR22[4:0] (64/4 -> 0x10) *=*/
            VGAOUT8(0x3c4, 0x22);
            bRegTemp = VGAIN8(0x3c5) & ~0x1F;
            VGAOUT8(0x3c5, bRegTemp | 0x10);
        }
        else
        {
            /*=* Max. length for a request SR22[4:0] (0x1F) *=*/
            VGAOUT8(0x3c4, 0x22);
            bRegTemp = VGAIN8(0x3c5) & ~0x1F;
            VGAOUT8(0x3c5, bRegTemp | 0x1F);
        }
        break;
    default:
        break;
    }
}

static void VIAInitialize2DEngine(DRIDriverContext *ctx)
{
    VIAPtr  pVia = VIAPTR(ctx);
    u_int32_t  dwVQStartAddr, dwVQEndAddr;
    u_int32_t  dwVQLen, dwVQStartL, dwVQEndL, dwVQStartEndH;
    u_int32_t  dwGEMode;

    /* init 2D engine regs to reset 2D engine */
    VIASETREG(0x04, 0x0);
    VIASETREG(0x08, 0x0);
    VIASETREG(0x0c, 0x0);
    VIASETREG(0x10, 0x0);
    VIASETREG(0x14, 0x0);
    VIASETREG(0x18, 0x0);
    VIASETREG(0x1c, 0x0);
    VIASETREG(0x20, 0x0);
    VIASETREG(0x24, 0x0);
    VIASETREG(0x28, 0x0);
    VIASETREG(0x2c, 0x0);
    VIASETREG(0x30, 0x0);
    VIASETREG(0x34, 0x0);
    VIASETREG(0x38, 0x0);
    VIASETREG(0x3c, 0x0);
    VIASETREG(0x40, 0x0);

    VIADisableMMIO(ctx);

    /* Init AGP and VQ regs */
    VIASETREG(0x43c, 0x00100000);
    VIASETREG(0x440, 0x00000000);
    VIASETREG(0x440, 0x00333004);
    VIASETREG(0x440, 0x60000000);
    VIASETREG(0x440, 0x61000000);
    VIASETREG(0x440, 0x62000000);
    VIASETREG(0x440, 0x63000000);
    VIASETREG(0x440, 0x64000000);
    VIASETREG(0x440, 0x7D000000);

    VIASETREG(0x43c, 0xfe020000);
    VIASETREG(0x440, 0x00000000);

    if (pVia->VQStart != 0) {
        /* Enable VQ */
        dwVQStartAddr = pVia->VQStart;
        dwVQEndAddr = pVia->VQEnd;
        dwVQStartL = 0x50000000 | (dwVQStartAddr & 0xFFFFFF);
        dwVQEndL = 0x51000000 | (dwVQEndAddr & 0xFFFFFF);
        dwVQStartEndH = 0x52000000 | ((dwVQStartAddr & 0xFF000000) >> 24) |
                        ((dwVQEndAddr & 0xFF000000) >> 16);
        dwVQLen = 0x53000000 | (VIA_VQ_SIZE >> 3);

        VIASETREG(0x43c, 0x00fe0000);
        VIASETREG(0x440, 0x080003fe);
        VIASETREG(0x440, 0x0a00027c);
        VIASETREG(0x440, 0x0b000260);
        VIASETREG(0x440, 0x0c000274);
        VIASETREG(0x440, 0x0d000264);
        VIASETREG(0x440, 0x0e000000);
        VIASETREG(0x440, 0x0f000020);
        VIASETREG(0x440, 0x1000027e);
        VIASETREG(0x440, 0x110002fe);
        VIASETREG(0x440, 0x200f0060);

        VIASETREG(0x440, 0x00000006);
        VIASETREG(0x440, 0x40008c0f);
        VIASETREG(0x440, 0x44000000);
        VIASETREG(0x440, 0x45080c04);
        VIASETREG(0x440, 0x46800408);

        VIASETREG(0x440, dwVQStartEndH);
        VIASETREG(0x440, dwVQStartL);
        VIASETREG(0x440, dwVQEndL);
        VIASETREG(0x440, dwVQLen);
    }
    else {
        /* Diable VQ */
        VIASETREG(0x43c, 0x00fe0000);
        VIASETREG(0x440, 0x00000004);
        VIASETREG(0x440, 0x40008c0f);
        VIASETREG(0x440, 0x44000000);
        VIASETREG(0x440, 0x45080c04);
        VIASETREG(0x440, 0x46800408);
    }

    dwGEMode = 0;

    switch (ctx->bpp) {
    case 16:
        dwGEMode |= VIA_GEM_16bpp;
        break;
    case 32:
        dwGEMode |= VIA_GEM_32bpp;
        break;
    default:
        dwGEMode |= VIA_GEM_8bpp;
        break;
    }

#if 0
    switch (ctx->shared.virtualWidth) {
    case 800:
        dwGEMode |= VIA_GEM_800;
        break;
    case 1024:
        dwGEMode |= VIA_GEM_1024;
        break;
    case 1280:
        dwGEMode |= VIA_GEM_1280;
        break;
    case 1600:
        dwGEMode |= VIA_GEM_1600;
        break;
    case 2048:
        dwGEMode |= VIA_GEM_2048;
        break;
    default:
        dwGEMode |= VIA_GEM_640;
        break;
    }
#endif
    
    VIAEnableMMIO(ctx);

    /* Set BPP and Pitch */
    VIASETREG(VIA_REG_GEMODE, dwGEMode);

    /* Set Src and Dst base address and pitch, pitch is qword */
    VIASETREG(VIA_REG_SRCBASE, 0x0);
    VIASETREG(VIA_REG_DSTBASE, 0x0);
    VIASETREG(VIA_REG_PITCH, VIA_PITCH_ENABLE |
              ((ctx->shared.virtualWidth * ctx->bpp >> 3) >> 3) |
              (((ctx->shared.virtualWidth * ctx->bpp >> 3) >> 3) << 16));
}

static int b3DRegsInitialized = 0;

static void VIAInitialize3DEngine(DRIDriverContext *ctx)
{
    VIAPtr  pVia = VIAPTR(ctx);
    int i;

    if (!b3DRegsInitialized)
    {

        VIASETREG(0x43C, 0x00010000);

        for (i = 0; i <= 0x7D; i++)
        {
            VIASETREG(0x440, (u_int32_t) i << 24);
        }

        VIASETREG(0x43C, 0x00020000);

        for (i = 0; i <= 0x94; i++)
        {
            VIASETREG(0x440, (u_int32_t) i << 24);
        }

        VIASETREG(0x440, 0x82400000);

        VIASETREG(0x43C, 0x01020000);


        for (i = 0; i <= 0x94; i++)
        {
            VIASETREG(0x440, (u_int32_t) i << 24);
        }

        VIASETREG(0x440, 0x82400000);
        VIASETREG(0x43C, 0xfe020000);

        for (i = 0; i <= 0x03; i++)
        {
            VIASETREG(0x440, (u_int32_t) i << 24);
        }

        VIASETREG(0x43C, 0x00030000);

        for (i = 0; i <= 0xff; i++)
        {
            VIASETREG(0x440, 0);
        }
        VIASETREG(0x43C, 0x00100000);
        VIASETREG(0x440, 0x00333004);
        VIASETREG(0x440, 0x10000002);
        VIASETREG(0x440, 0x60000000);
        VIASETREG(0x440, 0x61000000);
        VIASETREG(0x440, 0x62000000);
        VIASETREG(0x440, 0x63000000);
        VIASETREG(0x440, 0x64000000);

        VIASETREG(0x43C, 0x00fe0000);

        if (pVia->ChipRev >= 3 )
            VIASETREG(0x440,0x40008c0f);
        else
            VIASETREG(0x440,0x4000800f);

        VIASETREG(0x440,0x44000000);
        VIASETREG(0x440,0x45080C04);
        VIASETREG(0x440,0x46800408);
        VIASETREG(0x440,0x50000000);
        VIASETREG(0x440,0x51000000);
        VIASETREG(0x440,0x52000000);
        VIASETREG(0x440,0x53000000);

        b3DRegsInitialized = 1;
        xf86DrvMsg(pScrn->scrnIndex, X_INFO,
                   "3D Engine has been initialized.\n");
    }

    VIASETREG(0x43C,0x00fe0000);
    VIASETREG(0x440,0x08000001);
    VIASETREG(0x440,0x0A000183);
    VIASETREG(0x440,0x0B00019F);
    VIASETREG(0x440,0x0C00018B);
    VIASETREG(0x440,0x0D00019B);
    VIASETREG(0x440,0x0E000000);
    VIASETREG(0x440,0x0F000000);
    VIASETREG(0x440,0x10000000);
    VIASETREG(0x440,0x11000000);
    VIASETREG(0x440,0x20000000);
}

static int
WaitIdleCLE266(VIAPtr pVia)
{
    int loop = 0;

    /*mem_barrier();*/

    while (!(VIAGETREG(VIA_REG_STATUS) & VIA_VR_QUEUE_BUSY) && (loop++ < MAXLOOP))
        ;

    while ((VIAGETREG(VIA_REG_STATUS) &
          (VIA_CMD_RGTR_BUSY | VIA_2D_ENG_BUSY | VIA_3D_ENG_BUSY)) &&
          (loop++ < MAXLOOP))
        ;

    return loop >= MAXLOOP;
}

static int viaInitFBDev(DRIDriverContext *ctx)
{
    VIAPtr pVia = CALLOC(sizeof(*pVia));

    ctx->driverPrivate = (void *)pVia;

    switch (ctx->chipset) {
    case PCI_CHIP_CLE3122:
    case PCI_CHIP_CLE3022:
        pVia->Chipset = VIA_CLE266;
        break;
    case PCI_CHIP_VT7205:
    case PCI_CHIP_VT3205:
        pVia->Chipset = VIA_KM400;
        break;
    case PCI_CHIP_VT3204:
    case PCI_CHIP_VT3344:
        pVia->Chipset = VIA_K8M800;
        break;
    case PCI_CHIP_VT3259:
        pVia->Chipset = VIA_PM800;
        break;
    default:
        xf86DrvMsg(0, X_ERROR, "VIA: Unknown device ID (0x%x)\n", ctx->chipset);
    }

    /* _SOLO TODO XXX need to read ChipRev too */
    pVia->ChipRev = 0;

    pVia->videoRambytes = ctx->shared.fbSize;
    pVia->MmioBase = ctx->MMIOStart;
    pVia->FrameBufferBase = ctx->FBStart & 0xfc000000;

    pVia->FBFreeStart = ctx->shared.virtualWidth * ctx->cpp *
        ctx->shared.virtualHeight;

#if 1
    /* Alloc a second framebuffer for the second head */
    pVia->FBFreeStart += ctx->shared.virtualWidth * ctx->cpp *
	ctx->shared.virtualHeight;
#endif

    pVia->VQStart = pVia->FBFreeStart;
    pVia->VQEnd = pVia->FBFreeStart + VIA_VQ_SIZE - 1;

    pVia->FBFreeStart += VIA_VQ_SIZE;

    pVia->FBFreeEnd = pVia->videoRambytes;

    if (!VIADRIScreenInit(ctx))
        return 0;

    return 1;
}

static void viaHaltFBDev(DRIDriverContext *ctx)
{
    drmUnmap( ctx->pSAREA, ctx->shared.SAREASize );
    drmClose(ctx->drmFD);

    if (ctx->driverPrivate) {
        free(ctx->driverPrivate);
        ctx->driverPrivate = 0;
    }
}

static int viaEngineShutdown(const DRIDriverContext *ctx)
{
    return 1;
}

static int viaEngineRestore(const DRIDriverContext *ctx)
{
    return 1;
}

const struct DRIDriverRec __driDriver =
{
    viaValidateMode,
    viaPostValidateMode,
    viaInitFBDev,
    viaHaltFBDev,
    viaEngineShutdown,
    viaEngineRestore,  
    0,
};

