/*
 * PROJECT:     ReactOS Mini-HAL
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Initialize the x86 HAL
 * COPYRIGHT:   Copyright 1998 David Welch <welch@cwcom.net>
 */

/* INCLUDES *****************************************************************/

#include <hal.h>
#define NDEBUG
#include <debug.h>

/* FUNCTIONS ***************************************************************/

VOID
NTAPI
HalpInitProcessor(
    IN ULONG ProcessorNumber,
    IN PLOADER_PARAMETER_BLOCK LoaderBlock)
{
}

VOID
HalpInitPhase0(IN PLOADER_PARAMETER_BLOCK LoaderBlock)
{
}

VOID
HalpInitPhase1(VOID)
{
}

CODE_SEG("INIT")
NTSTATUS
NTAPI
HalpSetupAcpiPhase0(IN PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    return STATUS_SUCCESS;
}

VOID
NTAPI
HalpInitializePICs(IN BOOLEAN EnableInterrupts)
{
}

PDMA_ADAPTER
NTAPI
HalpGetDmaAdapter(
    IN PVOID Context,
    IN PDEVICE_DESCRIPTION DeviceDescription,
    OUT PULONG NumberOfMapRegisters)
{
    return NULL;
}

BOOLEAN
NTAPI
HalpBiosDisplayReset(VOID)
{
    return FALSE;
}

/* EOF */
