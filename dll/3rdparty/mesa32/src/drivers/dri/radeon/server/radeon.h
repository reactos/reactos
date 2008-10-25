/**
 * \file server/radeon.h
 * \brief Radeon 2D driver data structures.
 */

/*
 * Copyright 2000 ATI Technologies Inc., Markham, Ontario, and
 *                VA Linux Systems Inc., Fremont, California.
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
 * NON-INFRINGEMENT.  IN NO EVENT SHALL ATI, VA LINUX SYSTEMS AND/OR
 * THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/ati/radeon.h,v 1.29 2002/10/12 01:38:07 martin Exp $ */

#ifndef _RADEON_H_
#define _RADEON_H_

#include "xf86drm.h"		/* drm_handle_t, etc */

#       define RADEON_AGP_1X_MODE           0x01
#       define RADEON_AGP_2X_MODE           0x02
#       define RADEON_AGP_4X_MODE           0x04
#       define RADEON_AGP_FW_MODE           0x10
#       define RADEON_AGP_MODE_MASK         0x17
#define RADEON_CP_CSQ_CNTL                  0x0740
#       define RADEON_CSQ_CNT_PRIMARY_MASK     (0xff << 0)
#       define RADEON_CSQ_PRIDIS_INDDIS        (0    << 28)
#       define RADEON_CSQ_PRIPIO_INDDIS        (1    << 28)
#       define RADEON_CSQ_PRIBM_INDDIS         (2    << 28)
#       define RADEON_CSQ_PRIPIO_INDBM         (3    << 28)
#       define RADEON_CSQ_PRIBM_INDBM          (4    << 28)
#       define RADEON_CSQ_PRIPIO_INDPIO        (15   << 28)

#define RADEON_PCIGART_TABLE_SIZE       32768

#define PCI_CHIP_R200_BB                0x4242
#define PCI_CHIP_RV250_Id               0x4964
#define PCI_CHIP_RV250_Ie               0x4965
#define PCI_CHIP_RV250_If               0x4966
#define PCI_CHIP_RV250_Ig               0x4967
#define PCI_CHIP_RADEON_LW		0x4C57
#define PCI_CHIP_RADEON_LX		0x4C58
#define PCI_CHIP_RADEON_LY		0x4C59
#define PCI_CHIP_RADEON_LZ		0x4C5A
#define PCI_CHIP_RV250_Ld		0x4C64
#define PCI_CHIP_RV250_Le		0x4C65
#define PCI_CHIP_RV250_Lf		0x4C66
#define PCI_CHIP_RV250_Lg		0x4C67
#define PCI_CHIP_R300_ND		0x4E44
#define PCI_CHIP_R300_NE		0x4E45
#define PCI_CHIP_R300_NF		0x4E46
#define PCI_CHIP_R300_NG		0x4E47
#define PCI_CHIP_RADEON_QD		0x5144
#define PCI_CHIP_RADEON_QE		0x5145
#define PCI_CHIP_RADEON_QF		0x5146
#define PCI_CHIP_RADEON_QG		0x5147
#define PCI_CHIP_R200_QL		0x514C
#define PCI_CHIP_R200_QN		0x514E
#define PCI_CHIP_R200_QO		0x514F
#define PCI_CHIP_RV200_QW		0x5157
#define PCI_CHIP_RV200_QX		0x5158
#define PCI_CHIP_RADEON_QY		0x5159
#define PCI_CHIP_RADEON_QZ		0x515A
#define PCI_CHIP_R200_Ql		0x516C
#define PCI_CHIP_RV370_5460             0x5460
#define PCI_CHIP_RV280_Y_		0x5960
#define PCI_CHIP_RV280_Ya		0x5961
#define PCI_CHIP_RV280_Yb		0x5962
#define PCI_CHIP_RV280_Yc		0x5963

/**
 * \brief Chip families.
 */
typedef enum {
    CHIP_FAMILY_UNKNOW,
    CHIP_FAMILY_LEGACY,
    CHIP_FAMILY_R128,
    CHIP_FAMILY_M3,
    CHIP_FAMILY_RADEON,
    CHIP_FAMILY_VE,
    CHIP_FAMILY_M6,
    CHIP_FAMILY_RV200,
    CHIP_FAMILY_M7,
    CHIP_FAMILY_R200,
    CHIP_FAMILY_RV250,
    CHIP_FAMILY_M9,
    CHIP_FAMILY_RV280,
    CHIP_FAMILY_R300,
    CHIP_FAMILY_R350,
    CHIP_FAMILY_RV350,
    CHIP_FAMILY_RV380,  /* RV370/RV380/M22/M24 */
    CHIP_FAMILY_R420,   /* R420/R423/M18 */
} RADEONChipFamily;


typedef unsigned long memType;


/**
 * \brief Radeon DDX driver private data.
 */
typedef struct {
   int               Chipset;          /**< \brief Chipset number */
   RADEONChipFamily  ChipFamily;       /**< \brief Chip family */

   unsigned long     LinearAddr;       /**< \brief Frame buffer physical address */


   drmSize           registerSize;     /**< \brief MMIO register map size */
   drm_handle_t         registerHandle;   /**< \brief MMIO register map handle */

   int               IsPCI;            /* Current card is a PCI card */
   
   /**
    * \name AGP
    */
   /*@{*/
   drmSize           gartSize;          /**< \brief AGP map size */
   drm_handle_t         gartMemHandle;     /**< \brief AGP map handle */
   unsigned long     gartOffset;        /**< \brief AGP offset */
   int               gartMode;          /**< \brief AGP mode */
   int               gartFastWrite;
   /*@}*/

   /**
    * \name CP ring buffer data
    */
   /*@{*/
   unsigned long     ringStart;        /**< \brief Offset into AGP space */
   drm_handle_t         ringHandle;       /**< \brief Handle from drmAddMap() */
   drmSize           ringMapSize;      /**< \brief Size of map */
   int               ringSize;         /**< \brief Size of ring (in MB) */

   unsigned long     ringReadOffset;   /**< \brief Read offset into AGP space */
   drm_handle_t         ringReadPtrHandle;/**< \brief Handle from drmAddMap() */
   drmSize           ringReadMapSize;  /**< \brief Size of map */
   /*@}*/

   /**
    * \name CP vertex/indirect buffer data
    */
   /*@{*/
   unsigned long     bufStart;         /**< \brief Offset into AGP space */
   drm_handle_t         bufHandle;        /**< \brief Handle from drmAddMap() */
   drmSize           bufMapSize;       /**< \brief Size of map */
   int               bufSize;          /**< \brief Size of buffers (in MB) */
   int               bufNumBufs;       /**< \brief Number of buffers */
   /*@}*/

   /**
    * \name CP AGP Texture data
    */
   /*@{*/
   unsigned long     gartTexStart;      /**< \brief Offset into AGP space */
   drm_handle_t         gartTexHandle;     /**< \brief Handle from drmAddMap() */
   drmSize           gartTexMapSize;    /**< \brief Size of map */
   int               gartTexSize;       /**< \brief Size of AGP tex space (in MB) */
   int               log2GARTTexGran;
   /*@}*/

   int               drmMinor;         /**< \brief DRM device minor number */

   int               frontOffset;      /**< \brief Front color buffer offset */
   int               frontPitch;       /**< \brief Front color buffer pitch */
   int               backOffset;       /**< \brief Back color buffer offset */
   int               backPitch;        /**< \brief Back color buffer pitch */
   int               depthOffset;      /**< \brief Depth buffer offset */
   int               depthPitch;       /**< \brief Depth buffer pitch */
   int               textureOffset;    /**< \brief Texture area offset */
   int               textureSize;      /**< \brief Texture area size */
   int               log2TexGran;      /**< \brief Texture granularity in base 2 log */

   unsigned int      frontPitchOffset;
   unsigned int      backPitchOffset;
   unsigned int      depthPitchOffset;
   
   int               colorTiling;      /**< \brief Enable color tiling */

   int               irq;              /**< \brief IRQ number */
   int               page_flip_enable; /**< \brief Page Flip enable */
   unsigned int      gen_int_cntl;
   unsigned int      crtc_offset_cntl;

   unsigned long     pcieGartTableOffset;
} RADEONInfoRec, *RADEONInfoPtr;


#endif /* _RADEON_H_ */
