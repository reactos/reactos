/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Core source file for SMP management
 * COPYRIGHT:   Copyright 2021 Justin Miller <justinmiller100@gmail.com>
 */

/* INCLUDES ******************************************************************/

#include <hal.h>
#include <smp.h>
#include "smpp.h"
#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

extern PHYSICAL_ADDRESS HalpLowStubPhysicalAddress;
extern PVOID HalpLowStub;

/* TODO: MaxAPCount should be assigned by a Multi APIC table */
ULONG MaxAPCount = 2;
ULONG StartedProcessorCount = 1;

/* FUNCTIONS *****************************************************************/

BOOLEAN
NTAPI
HalStartNextProcessor(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock,
    IN PKPROCESSOR_STATE ProcessorState)
{
    if (MaxAPCount > StartedProcessorCount)
    {
        /* Start an AP */
        HalpInitializeAPStub(HalpLowStub);
        HalpInitalizeAPPageTable(HalpLowStub);
        ApicStartApplicationProcessor(StartedProcessorCount, HalpLowStubPhysicalAddress);
        StartedProcessorCount++;

        return TRUE;
    }
    else
    {
        return FALSE;
    }
}
