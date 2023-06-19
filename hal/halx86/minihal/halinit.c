/*
 * PROJECT:     ReactOS Mini-HAL
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Initialize the x86 HAL
 * COPYRIGHT:   Copyright 1998 David Welch <welch@cwcom.net>
 */

/* INCLUDES *******************************************************************/

#include <hal.h>

/* FUNCTIONS ******************************************************************/

#if 0

VOID
NTAPI
HalpInitializePICs(
    IN BOOLEAN EnableInterrupts)
{
}

#endif

PDMA_ADAPTER
NTAPI
HalpGetDmaAdapter(
    IN PVOID Context,
    IN PDEVICE_DESCRIPTION DeviceDescription,
    OUT PULONG NumberOfMapRegisters)
{
    return NULL;
}

CODE_SEG("INIT")
VOID
NTAPI
HalpInitBusHandlers(VOID)
{
    /* Initialize the PCI bus */
    // HalpInitializePciBus();
    HalpInitializePciStubs();

    // /* Register root support */
    // HalpInitBusHandler();
}

/* EOF */
