/*
 * PROJECT:         ReactOS Boot Loader
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            boot/armllb/hw/versatile/hwinit.c
 * PURPOSE:         LLB Hardware Initialization Routines for Versatile
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

#include "precomp.h"

VOID
NTAPI
LlbHwInitialize(VOID)
{
    /* Setup the CLCD (PL110) */
    LlbHwVersaClcdInitialize();
    
    /* Setup the UART (PL011) */
    LlbHwVersaUartInitialize();
    
    /* Setup the KMI (PL050) */
    LlbHwVersaKmiInitialize();
}

//
// Should go to hwdev.c
//
POSLOADER_INIT
NTAPI
LlbHwLoadOsLoaderFromRam(VOID)
{
    ULONG Base, RootFs, Size;
    CHAR CommandLine[64];
    
    /* On versatile, the NAND image is loaded as the RAMDISK */
    LlbEnvGetRamDiskInformation(&Base, &Size);
    
    /* The LLB is first, which we already have, so skip it */
    Base += 0x10000; // 64 KB (see nandflash)
    
    /* The OS loader is next, followed by the root file system */
    RootFs = Base + 0x80000; // 512 KB (see nandflash)
    
    /* Set parameters for the OS loader */
    sprintf(CommandLine, "rdbase=0x%x rdsize=0x%x", RootFs, Size);
    LlbSetCommandLine(CommandLine);
    
    /* Return the OS loader base address */
    return (POSLOADER_INIT)Base;
}


/* EOF */
