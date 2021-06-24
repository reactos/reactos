/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Source File for MADT Table parsing
 * COPYRIGHT:   Copyright 2021 Justin Miller <justinmiller100@gmail.com>
 */

/* INCLUDES *******************************************************************/

#include <hal.h>
#include <acpi.h>
/* ACPI_BIOS_ERROR defined in acoutput.h and bugcodes.h */
#undef ACPI_BIOS_ERROR
#include <smp.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

PROCESSOR_IDENTITY HalpStaticProcessorIdentity[MAXIMUM_PROCESSORS] = {{0}};
PPROCESSOR_IDENTITY HalpProcessorIdentity = NULL;

/* FUNCTIONS ******************************************************************/

VOID
HalpParseApicTables(
    _In_ PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    UNIMPLEMENTED;
}
