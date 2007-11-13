/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/via/via_driver.h,v 1.7 2003/11/06 18:38:11 tsi Exp $ */
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

#ifndef _VIA_DRIVER_H
#define _VIA_DRIVER_H

#if 0 /* DEBUG is use in VIA DRI code as a flag */
/* #define DEBUG_PRINT */
#ifdef DEBUG_PRINT
#define DEBUG(x) x
#else
#define DEBUG(x)
#endif
#endif

#if 0
#include "vgaHW.h"
#include "xf86.h"
#include "xf86Resources.h"
#include "xf86_ansic.h"
#include "xf86Pci.h"
#include "xf86PciInfo.h"
#include "xf86_OSproc.h"
#include "compiler.h"
#include "xf86Cursor.h"
#include "mipointer.h"
#include "micmap.h"

#define USE_FB
#ifdef USE_FB
#include "fb.h"
#else
#include "cfb.h"
#include "cfb16.h"
#include "cfb32.h"
#endif

#include "xf86cmap.h"
#include "vbe.h"
#include "xaa.h"

#include "via_regs.h"
#include "via_bios.h"
#include "via_gpioi2c.h"
#include "via_priv.h"
#include "ginfo.h"

#ifdef XF86DRI
#define _XF86DRI_SERVER_
#include "sarea.h"
#include "dri.h"
#include "GL/glxint.h"
#include "via_dri.h"
#endif
#else
#include "via_regs.h"

#include "GL/internal/dri_interface.h"
#include "via_dri.h"
#endif

/* _SOLO : copied from via_bios.h */
/* System Memory CLK */
#define		VIA_MEM_SDR66					0x00
#define		VIA_MEM_SDR100					0x01
#define		VIA_MEM_SDR133					0x02
#define		VIA_MEM_DDR200					0x03
#define		VIA_MEM_DDR266					0x04
#define		VIA_MEM_DDR333					0x05
#define		VIA_MEM_DDR400					0x06

#define DRIVER_NAME     "via"
#define DRIVER_VERSION  "4.1.0"
#define VERSION_MAJOR   4
#define VERSION_MINOR   1
#define PATCHLEVEL      41
#define VIA_VERSION     ((VERSION_MAJOR<<24) | (VERSION_MINOR<<16) | PATCHLEVEL)

#define VGAIN8(addr)        MMIO_IN8(pVia->MapBase+0x8000, addr)
#define VGAIN16(addr)       MMIO_IN16(pVia->MapBase+0x8000, addr)
#define VGAIN(addr)         MMIO_IN32(pVia->MapBase+0x8000, addr)

#define VGAOUT8(addr, val)  MMIO_OUT8(pVia->MapBase+0x8000, addr, val)
#define VGAOUT16(addr, val) MMIO_OUT16(pVia->MapBase+0x8000, addr, val)
#define VGAOUT(addr, val)   MMIO_OUT32(pVia->MapBase+0x8000, addr, val)

#define INREG(addr)         MMIO_IN32(pVia->MapBase, addr)
#define OUTREG(addr, val)   MMIO_OUT32(pVia->MapBase, addr, val)
#define INREG16(addr)       MMIO_IN16(pVia->MapBase, addr)
#define OUTREG16(addr, val) MMIO_OUT16(pVia->MapBase, addr, val)

#define VIA_PIXMAP_CACHE_SIZE   (256 * 1024)
#define VIA_CURSOR_SIZE         (4 * 1024)
#define VIA_VQ_SIZE             (256 * 1024)

typedef struct {
    unsigned int    mode, refresh, resMode;
    int             countWidthByQWord;
    int             offsetWidthByQWord;
    unsigned char   SR08, SR0A, SR0F;

    /*   extended Sequencer registers */
    unsigned char   SR10, SR11, SR12, SR13,SR14,SR15,SR16;
    unsigned char   SR17, SR18, SR19, SR1A,SR1B,SR1C,SR1D,SR1E;
    unsigned char   SR1F, SR20, SR21, SR22,SR23,SR24,SR25,SR26;
    unsigned char   SR27, SR28, SR29, SR2A,SR2B,SR2C,SR2D,SR2E;
    unsigned char   SR2F, SR30, SR31, SR32,SR33,SR34,SR40,SR41;
    unsigned char   SR42, SR43, SR44, SR45,SR46,SR47;

    unsigned char   Clock;

    /*   extended CRTC registers */
    unsigned char   CR13, CR30, CR31, CR32, CR33, CR34, CR35, CR36;
    unsigned char   CR37, CR38, CR39, CR3A, CR40, CR41, CR42, CR43;
    unsigned char   CR44, CR45, CR46, CR47, CR48, CR49, CR4A;
    unsigned char   CRTCRegs[83];
    unsigned char   TVRegs[0xCF];
    unsigned char   TVRegs2[0xCF];
/*    unsigned char   LCDRegs[0x40];*/

} VIARegRec, *VIARegPtr;


typedef struct _VIA {
    VIARegRec           SavedReg;
    VIARegRec           ModeReg;
#if 0
    xf86CursorInfoPtr   CursorInfoRec;
    int                 stateMode;
    VIAModeInfoPtr      VIAModeList;
#endif
    int                 ModeStructInit;
    int                 Bpp, Bpl, ScissB;
    unsigned            PlaneMask;

    unsigned long       videoRambytes;
    int                 videoRamKbytes;
    int                 FBFreeStart;
    int                 FBFreeEnd;
    int                 CursorStart;
    int                 VQStart;
    int                 VQEnd;

    /* These are physical addresses. */
    unsigned long       FrameBufferBase;
    unsigned long       MmioBase;

    /* These are linear addresses. */
    unsigned char*      MapBase;
    unsigned char*      VidMapBase;
    unsigned char*      BltBase;
    unsigned char*      MapBaseDense;
    unsigned char*      FBBase;
    unsigned char*      FBStart;

    int                 PrimaryVidMapped;
    int                 dacSpeedBpp;
    int                 minClock, maxClock;
    int                 MCLK, REFCLK, LCDclk;
    double              refclk_fact;

    /* Here are all the Options */
    int                 VQEnable;
    int                 pci_burst;
    int                 NoPCIRetry;
    int                 hwcursor;
    int                 NoAccel;
    int                 shadowFB;
    int                 NoDDCValue;
    int                 rotate;

#if 0
    CloseScreenProcPtr  CloseScreen;
    pciVideoPtr         PciInfo;
    PCITAG              PciTag;
#endif
    int                 Chipset;
    int                 ChipId;
    int                 ChipRev;
    /*vbeInfoPtr          pVbe;*/
    int                 EntityIndex;

    /* Support for shadowFB and rotation */
    unsigned char*      ShadowPtr;
    int                 ShadowPitch;
    void                (*PointerMoved)(int index, int x, int y);

    /* Support for XAA acceleration */
#if 0
    XAAInfoRecPtr       AccelInfoRec;
    xRectangle          Rect;
#endif
    u_int32_t            SavedCmd;
    u_int32_t            SavedFgColor;
    u_int32_t            SavedBgColor;
    u_int32_t            SavedPattern0;
    u_int32_t            SavedPattern1;
    u_int32_t            SavedPatternAddr;

#if 0
    /* Support for Int10 processing */
    xf86Int10InfoPtr    pInt10;

    /* BIOS Info Ptr */
    VIABIOSInfoPtr      pBIOSInfo;
    VGABIOSVERPtr       pBIOSVer;
#endif

    /* Support for DGA */
    int                 numDGAModes;
    /*DGAModePtr          DGAModes;*/
    int                 DGAactive;
    int                 DGAViewportStatus;

    /* The various wait handlers. */
    int                 (*myWaitIdle)(struct _VIA*);

#if 0
    /* I2C & DDC */
    I2CBusPtr           I2C_Port1;
    I2CBusPtr           I2C_Port2;
    xf86MonPtr          DDC1;
    xf86MonPtr          DDC2;
#endif

    /* MHS */
    int                 IsSecondary;
    int                 HasSecondary;

#if 0
    /* Capture information */
    VIACAPINFO     CapInfo[2];      /* 2 capture information */
#endif

/*
    u_int32_t            Cap0_Deinterlace;
    u_int32_t            Cap1_Deinterlace;

    int                 Cap0_FieldSwap;
    int                 NoCap0_HFilter;
    int                 Capture_OverScanOff;
    int                 NoMPEGHQV_VFilter;
*/
#ifdef XF86DRI
    int 		directRenderingEnabled;
    DRIInfoPtr		pDRIInfo;
    int 		drmFD;
    int 		numVisualConfigs;
    __GLXvisualConfig* 	pVisualConfigs;
    VIAConfigPrivPtr 	pVisualConfigsPriv;
    unsigned long 	agpHandle;
    unsigned long 	registerHandle;
    u_int32_t            agpAddr;
    unsigned char 	*agpBase;
    unsigned int 	agpSize;
    int  		IsPCI;
    int  		drixinerama;
#else
    int 		drmFD;
    unsigned long 	agpHandle;
    unsigned long 	registerHandle;
    unsigned long 	agpAddr;
    unsigned char 	*agpBase;
    unsigned int 	agpSize;
    int  		IsPCI;
#endif

    int     V4LEnabled;
    u_int16_t    ActiveDevice;	/* if SAMM, non-equal pBIOSInfo->ActiveDevice */
    unsigned char       *CursorImage;
    u_int32_t    CursorFG;
    u_int32_t    CursorBG;
    u_int32_t    CursorMC;

    unsigned char	MemClk;
    int 		EnableExtendedFIFO;
    VIADRIPtr		devPrivate;
} VIARec, *VIAPtr;


/* Shortcuts.  These depend on a local symbol "pVia". */

#define WaitIdle()      pVia->myWaitIdle(pVia)
#define VIAPTR(p)       ((VIAPtr)((p)->driverPrivate))

#endif /* _VIA_DRIVER_H */

