/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Source File for MPS specific functions
 * COPYRIGHT:   Copyright 2021 Justin Miller <justinmiller100@gmail.com>
 */

/* INCLUDES *******************************************************************/

#include <hal.h>
#include <smp.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

PROCESSOR_IDENTITY HalpStaticProcessorIdentity[MAXIMUM_PROCESSORS] = {{0}};
PPROCESSOR_IDENTITY HalpProcessorIdentity = NULL;
UINT32 PhysicalProcessorCount = 0;

/* FUNCTIONS ******************************************************************/

VOID
HalpParseApicTables(
    _In_ PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    UNIMPLEMENTED;
}

VOID
HalpPrintApicTables(VOID)
{
    UINT32 i;

    DPRINT1("HAL has detected a physical processor count of: %d\n", PhysicalProcessorCount);
    for (i = 0; i < PhysicalProcessorCount; i++)
    {
        DPRINT1("Information about the following processor is for processors number: %d\n"
                "   The BSPCheck is set to: %X\n"
                "   The LapicID is set to: %X\n",
                i, HalpProcessorIdentity[i].BSPCheck, HalpProcessorIdentity[i].LapicId);
    }
}
