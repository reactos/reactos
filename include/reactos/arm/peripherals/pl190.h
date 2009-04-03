/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            include/reactos/arm/targets/pl190.h
 * PURPOSE:         PL190 Registers and Constants
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* GLOBALS ********************************************************************/

//
// VIC Registers
//
#define VIC_BASE                (PVOID)0xE0040000 /* HACK: freeldr mapped it here */

#define VIC_INT_STATUS          (VIC_BASE + 0x00)
#define VIC_INT_ENABLE          (VIC_BASE + 0x10)
#define VIC_INT_CLEAR           (VIC_BASE + 0x14)
#define VIC_SOFT_INT            (VIC_BASE + 0x18)
#define VIC_SOFT_INT_CLEAR      (VIC_BASE + 0x1C)
