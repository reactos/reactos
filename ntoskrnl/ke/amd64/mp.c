/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * FILE:        ntoskrnl/ke/amd64/mp.c
 * PURPOSE:     Source file to hold multiprocessor functions
 * PROGRAMMERS:  Copyright 2021 Justin Miller <justinmiller100@gmail.com>
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

#define PHYSICAL_ADDRESS  LARGE_INTEGER
PHYSICAL_ADDRESS HighestPhysicalAddress;
ULONG ProcessorCount = 0;

/* FUNCTIONS *****************************************************************/
VOID
NTAPI
KeStartAllProcessors()
{
    BOOLEAN ApplicationProcessorsEnabled = FALSE;
    HighestPhysicalAddress.QuadPart = UINT64_MAX;
    KDESCRIPTOR GdtDesc, IdtDesc;
    __sgdt(&GdtDesc.Limit);
    __sidt(&IdtDesc.Limit);
    do
    {
        ProcessorCount++;
        /* Attempt to allocate memory for its new setup */
        SIZE_T APInfo = sizeof(KPCR) + sizeof(KTSS) + sizeof(ETHREAD) + GdtDesc.Limit + 1 + IdtDesc.Limit + 1; 
        PVOID PAPInfo = MmAllocateContiguousMemory(APInfo, HighestPhysicalAddress);

        if(!PAPInfo)
        {
            ASSERT("KeStartAllProcessors: Memory Allocation has failed");
        }

        RtlCopyMemory((PVOID)((ULONG_PTR)PAPInfo + sizeof(KPCR) + sizeof(KTSS)), (PVOID)GdtDesc.Base, GdtDesc.Limit + 1);
        RtlCopyMemory((PVOID)((ULONG_PTR)PAPInfo + sizeof(KPCR) + sizeof(KTSS) + GdtDesc.Limit + 1), (PVOID)IdtDesc.Base, IdtDesc.Limit + 1);

        /* Prep processorstate */
        KPROCESSOR_STATE ProcessorState;

        /* Allocate PCR - InitalThread */
        PKPCR pKPcr = (PKPCR)PAPInfo;
        PKTHREAD InitialThread = (PKTHREAD)((ULONG_PTR) PAPInfo + sizeof(KPCR) + sizeof(KTSS));

        /* Prep some stacks for the AP */
        PVOID KernelStack;
        PVOID DPCStack;

        KernelStack = MmCreateKernelStack(FALSE, 0);
        if (!KernelStack)
        {
            ASSERT("KeStartAllProcessors: MmCreateKernelStack has failed for an AP");
        }

        DPCStack = MmCreateKernelStack(FALSE, 0);
        if (!DPCStack)
        {
            MmDeleteKernelStack(KernelStack, FALSE);
        }

        /* Prep a new loaderblock for AP */
        KeLoaderBlock->KernelStack = (ULONG_PTR)KernelStack;
        KeLoaderBlock->Prcb = (ULONG_PTR)pKPcr;
        KeLoaderBlock->Thread = (ULONG_PTR)InitialThread;

        ApplicationProcessorsEnabled = HalStartNextProcessor(KeLoaderBlock, &ProcessorState);

        if(ApplicationProcessorsEnabled == FALSE){
            /* We have finished starting processors, Time to cleanup! */
            MmFreeContiguousMemory(PAPInfo);
            MmDeleteKernelStack(KernelStack, FALSE);
            MmDeleteKernelStack(DPCStack, FALSE);
        }

    } while (ApplicationProcessorsEnabled);
}
