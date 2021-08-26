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
        HalpInitializeAPStub(HalpLowStub, 
                             ProcessorState->SpecialRegisters.Gdtr,
                             ProcessorState->SpecialRegisters.Idtr);

        HalpWriteProcessorState(HalpLowStub, ProcessorState, (ULONG_PTR)LoaderBlock);
        #ifdef _M_AMD64
        HalpAdjustTempPageTable(HalpLowStub, (UINT32)ProcessorState->ContextFrame.Rcx, 
                               (PVOID)ProcessorState->ContextFrame.Rax, ProcessorState,
                               (UINT32)ProcessorState->ContextFrame.Rbx);
        #else
        HalpAdjustTempPageTable(HalpLowStub, (UINT32)ProcessorState->ContextFrame.Ecx, 
                               (PVOID)ProcessorState->ContextFrame.Eax, ProcessorState,
                               (UINT32)ProcessorState->ContextFrame.Ebx);
        #endif
        ApicStartApplicationProcessor(StartedProcessorCount, HalpLowStubPhysicalAddress);
        StartedProcessorCount++;

        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

