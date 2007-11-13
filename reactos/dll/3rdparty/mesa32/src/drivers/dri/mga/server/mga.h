/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/mga/mga.h,v 1.85 2002/12/16 16:19:17 dawes Exp $ */
/*
 * MGA Millennium (MGA2064W) functions
 *
 * Copyright 1996 The XFree86 Project, Inc.
 *
 * Authors
 *		Dirk Hohndel
 *			hohndel@XFree86.Org
 *		David Dawes
 *			dawes@XFree86.Org
 */

#ifndef MGA_H
#define MGA_H


#include "xf86drm.h"
#include "linux/types.h"


#define PCI_CHIP_MGA2085		0x0518
#define PCI_CHIP_MGA2064		0x0519
#define PCI_CHIP_MGA1064		0x051A
#define PCI_CHIP_MGA2164		0x051B
#define PCI_CHIP_MGA2164_AGP		0x051F
#define PCI_CHIP_MGAG200_PCI		0x0520
#define PCI_CHIP_MGAG200		0x0521
#define PCI_CHIP_MGAG400		0x0525
#define PCI_CHIP_MGAG550		0x2527
#define PCI_CHIP_MGAG100_PCI		0x1000
#define PCI_CHIP_MGAG100		0x1001


#  define MMIO_IN8(base, offset) \
	*(volatile unsigned char *)(((unsigned char*)(base)) + (offset))
#  define MMIO_IN16(base, offset) \
	*(volatile unsigned short *)(void *)(((unsigned char*)(base)) + (offset))
#  define MMIO_IN32(base, offset) \
	*(volatile unsigned int *)(void *)(((unsigned char*)(base)) + (offset))
#  define MMIO_OUT8(base, offset, val) \
	*(volatile unsigned char *)(((unsigned char*)(base)) + (offset)) = (val)
#  define MMIO_OUT16(base, offset, val) \
	*(volatile unsigned short *)(void *)(((unsigned char*)(base)) + (offset)) = (val)
#  define MMIO_OUT32(base, offset, val) \
	*(volatile unsigned int *)(void *)(((unsigned char*)(base)) + (offset)) = (val)

#define INREG8(addr) MMIO_IN8(pMga->IOBase, addr)
#define INREG16(addr) MMIO_IN16(pMga->IOBase, addr)
#define INREG(addr) MMIO_IN32(pMga->IOBase, addr)
#define OUTREG8(addr, val) MMIO_OUT8(pMga->IOBase, addr, val)
#define OUTREG16(addr, val) MMIO_OUT16(pMga->IOBase, addr, val)
#define OUTREG(addr, val) MMIO_OUT32(pMga->IOBase, addr, val)

#define MGAIOMAPSIZE		0x00004000


typedef struct {
  int               Chipset;          /**< \brief Chipset number */

  int               irq;              /**< \brief IRQ number */


  int               frontOffset;      /**< \brief Front color buffer offset */
  int               frontPitch;       /**< \brief Front color buffer pitch */
  int               backOffset;       /**< \brief Back color buffer offset */
  int               backPitch;        /**< \brief Back color buffer pitch */
  int               depthOffset;      /**< \brief Depth buffer offset */
  int               depthPitch;       /**< \brief Depth buffer pitch */
  int               textureOffset;    /**< \brief Texture area offset */
  int               textureSize;      /**< \brief Texture area size */
  int               logTextureGranularity;

  /**
   * \name AGP
   */
  /*@{*/
  drmSize           agpSize;          /**< \brief AGP map size */
  int               agpMode;          /**< \brief AGP mode */
  /*@}*/

  drmRegion         agp;

  /* PCI mappings */
  drmRegion         registers;
  drmRegion         status;

  /* AGP mappings */
  drmRegion         warp;
  drmRegion         primary;
  drmRegion         buffers;
  drmRegion         agpTextures;

  drmBufMapPtr      drmBuffers;

  unsigned long     IOAddress;
  unsigned char    *IOBase;
  int		    HasSDRAM;

  __u32             reg_ien;
} MGARec, *MGAPtr;



#define MGA_FRONT	0x1
#define MGA_BACK	0x2
#define MGA_DEPTH	0x4

#define MGA_AGP_1X_MODE		0x01
#define MGA_AGP_2X_MODE		0x02
#define MGA_AGP_4X_MODE		0x04
#define MGA_AGP_MODE_MASK	0x07


#endif
