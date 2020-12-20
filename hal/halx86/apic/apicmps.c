/*
 * PROJECT:         ReactOS HAL
 * COPYRIGHT:       GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * FILE:            hal/halx86/apic/apicmps.c
 * PURPOSE:         MPS part APIC HALs code
 * PROGRAMMERS:     Copyright 2020 Vadim Galyant <vgal@rambler.ru>
 */

/* INCLUDES *******************************************************************/

#include <hal.h>
#define NDEBUG
#include <debug.h>

#include "apic.h"

/* GLOBALS ********************************************************************/

BOOLEAN HalpPciLockSettings;

/* FUNCTIONS ******************************************************************/

CODE_SEG("INIT")
VOID
NTAPI
HalpGetParameters(IN PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    PCHAR CommandLine;

    /* Make sure we have a loader block and command line */
    if ((LoaderBlock) && (LoaderBlock->LoadOptions))
    {
        /* Read the command line */
        CommandLine = LoaderBlock->LoadOptions;

        /* Check if PCI is locked */
        if (strstr(CommandLine, "PCILOCK")) HalpPciLockSettings = TRUE;

        /* Check for initial breakpoint */
        if (strstr(CommandLine, "BREAK")) DbgBreakPoint();
    }
}

BOOLEAN
NTAPI 
DetectMP(_In_ PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    // FIXME UNIMPLIMENTED;
    ASSERT(FALSE);
    return FALSE;
}

/* EOF */
