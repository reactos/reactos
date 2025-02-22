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
    PCHAR Offset;
    CHAR CommandLine[64];

    /* On versatile we load the RAMDISK with initrd */
    LlbEnvGetRamDiskInformation(&RootFs, &Size);

    /* The OS Loader is at 0x20000, always */
    Base = 0x20000;

    /* Read image offset */
    Offset = LlbEnvRead("rdoffset");

    /* Set parameters for the OS loader */
    sprintf(CommandLine, "rdbase=0x%x rdsize=0x%x rdoffset=%s", RootFs, Size, Offset);
    LlbSetCommandLine(CommandLine);

    /* Return the OS loader base address */
    return (POSLOADER_INIT)Base;
}


/* EOF */
