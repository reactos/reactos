/*
 * PROJECT:         ReactOS Boot Loader
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            boot/armllb/inc/machtype.h
 * PURPOSE:         Standard machine type definitions defined by Linux/U-boot
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

//
// Marvell Feroceon-based SoC:
// Buffalo Linkstation, KuroBox Pro, D-Link DS323 and others
//
#define MACH_TYPE_FEROCEON     526

//
// ARM Versatile PB:
// qemu-system-arm -M versatilepb, RealView Development Boards and others
//
#define MACH_TYPE_VERSATILE_PB 387

//
// TI Beagle Board, OMAP3530 SoC
// qemu-system-arm -M beagle, Beagle Board
//
#define MACH_TYPE_OMAP3_BEAGLE 1546

//
// LogicPD ZOOM-II MDK Board, OMAP3530 SoC
//
#define MACH_TYPE_OMAP_ZOOM2   1967

/* EOF */
