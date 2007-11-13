/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/mga/mga_macros.h,v 1.22 2002/02/20 17:17:50 dawes Exp $ */

#ifndef _MGA_MACROS_H_
#define _MGA_MACROS_H_

#ifndef PSZ
#define PSZ 8
#endif

#if PSZ == 8
#define REPLICATE(r) r &= 0xFF; r |= r << 8; r |= r << 16
#elif PSZ == 16
#define REPLICATE(r) r &= 0xFFFF; r |= r << 16
#elif PSZ == 24
#define REPLICATE(r) r &= 0xFFFFFF; r |= r << 24
#else
#define REPLICATE(r) /* */
#endif

#define RGBEQUAL(c) (!((((c) >> 8) ^ (c)) & 0xffff))

#ifdef XF86DRI
#define MGA_SYNC_XTAG                 0x275f4200

#define MGABUSYWAIT() do { \
OUTREG(MGAREG_DWGSYNC, MGA_SYNC_XTAG); \
while(INREG(MGAREG_DWGSYNC) != MGA_SYNC_XTAG) ; \
}while(0);

#endif

#define MGAISBUSY() (INREG8(MGAREG_Status + 2) & 0x01)

#define WAITFIFO(cnt) \
   if(!pMga->UsePCIRetry) {\
	register int n = cnt; \
	if(n > pMga->FifoSize) n = pMga->FifoSize; \
	while(pMga->fifoCount < (n))\
	    pMga->fifoCount = INREG8(MGAREG_FIFOSTATUS);\
	pMga->fifoCount -= n;\
   }

#define XYADDRESS(x,y) \
    ((y) * pMga->CurrentLayout.displayWidth + (x) + pMga->YDstOrg)

#define MAKEDMAINDEX(index)  ((((index) >> 2) & 0x7f) | (((index) >> 6) & 0x80))

#define DMAINDICES(one,two,three,four)	\
	( MAKEDMAINDEX(one) | \
	 (MAKEDMAINDEX(two) << 8) | \
	 (MAKEDMAINDEX(three) << 16) | \
 	 (MAKEDMAINDEX(four) << 24) )

#if PSZ == 24
#define SET_PLANEMASK(p) /**/
#else
#define SET_PLANEMASK(p) \
	if(!(pMga->AccelFlags & MGA_NO_PLANEMASK) && ((p) != pMga->PlaneMask)) { \
	   pMga->PlaneMask = (p); \
	   REPLICATE((p)); \
	   OUTREG(MGAREG_PLNWT,(p)); \
	}
#endif

#define SET_FOREGROUND(c) \
	if((c) != pMga->FgColor) { \
	   pMga->FgColor = (c); \
	   REPLICATE((c)); \
	   OUTREG(MGAREG_FCOL,(c)); \
	}

#define SET_BACKGROUND(c) \
	if((c) != pMga->BgColor) { \
	   pMga->BgColor = (c); \
	   REPLICATE((c)); \
	   OUTREG(MGAREG_BCOL,(c)); \
	}

#define DISABLE_CLIP() { \
	pMga->AccelFlags &= ~CLIPPER_ON; \
	WAITFIFO(1); \
	OUTREG(MGAREG_CXBNDRY, 0xFFFF0000); }

#ifdef XF86DRI
#define CHECK_DMA_QUIESCENT(pMGA, pScrn) {	\
   if (!pMGA->haveQuiescense) {			\
      pMGA->GetQuiescence( pScrn );		\
   }						\
}
#else
#define CHECK_DMA_QUIESCENT(pMGA, pScrn)
#endif

#ifdef USEMGAHAL
#define HAL_CHIPSETS ((pMga->Chipset == PCI_CHIP_MGAG200_PCI) || \
		  (pMga->Chipset == PCI_CHIP_MGAG200) || \
		  (pMga->Chipset == PCI_CHIP_MGAG400) || \
		  (pMga->Chipset == PCI_CHIP_MGAG550))
    
#define MGA_HAL(x) { \
	MGAPtr pMga = MGAPTR(pScrn); \
	if (pMga->HALLoaded && HAL_CHIPSETS) { x; } \
}
#define MGA_NOT_HAL(x) { \
	MGAPtr pMga = MGAPTR(pScrn); \
	if (!pMga->HALLoaded || !HAL_CHIPSETS) { x; } \
}
#else
#define MGA_NOT_HAL(x) { x; }
#endif

#define MGAISGx50(x) ( (((x)->Chipset == PCI_CHIP_MGAG400) && ((x)->ChipRev >= 0x80)) || \
		       ((x)->Chipset == PCI_CHIP_MGAG550) )

#define MGA_DH_NEEDS_HAL(x) (((x)->Chipset == PCI_CHIP_MGAG400) && \
			     ((x)->ChipRev < 0x80))

#endif /* _MGA_MACROS_H_ */
