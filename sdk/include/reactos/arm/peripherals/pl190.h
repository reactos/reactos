/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            include/reactos/arm/peripherals/pl190.h
 * PURPOSE:         PL190 Registers and Constants
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* GLOBALS ********************************************************************/

//
// VIC Registers
//
#define VIC_BASE                (ULONG_PTR)0x10140000

#define VIC_INT_STATUS          (PULONG)(VIC_BASE + 0x00)
#define VIC_INT_ENABLE          (PULONG)(VIC_BASE + 0x10)
#define VIC_INT_CLEAR           (PULONG)(VIC_BASE + 0x14)
#define VIC_SOFT_INT            (PULONG)(VIC_BASE + 0x18)
#define VIC_SOFT_INT_CLEAR      (PULONG)(VIC_BASE + 0x1C)
