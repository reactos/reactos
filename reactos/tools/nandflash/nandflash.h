/*
 * PROJECT:         OMAP3 NAND Flashing Utility
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            tools/nandflash/nandflash.h
 * PURPOSE:         Flashes OmapLDR, FreeLDR and a Root FS into a NAND image
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <host/typedefs.h>

/* NAND Image Sizes */
#define NAND_PAGE_SIZE  (2 * 1024)                              // 2 KB
#define NAND_OOB_SIZE   64                                      // 64 bytes
#define NAND_PAGES      ((256 * 1024 * 1024) / NAND_PAGE_SIZE)  // 256 MB

/* EOF */
