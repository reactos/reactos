/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Architecture source file to hold multiprocessor functions
 * COPYRIGHT:   Copyright 2021 Justin Miller <justinmiller100@gmail.com>
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

#define PHYSICAL_ADDRESS  LARGE_INTEGER
PHYSICAL_ADDRESS HighestPhysicalAddress;
ULONG ProcessorCount = 0;

struct APInfo
{
    KPCR pcr;
    KTSS tss;
    ETHREAD thread;
    KDESCRIPTOR GdtDesc;
    KDESCRIPTOR IdtDesc;
} APInfo;

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
        SIZE_T APInfo = sizeof(struct APInfo) + 2; 
        PVOID PAPInfo = MmAllocateContiguousMemory(APInfo, HighestPhysicalAddress);

        if (!PAPInfo)
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
            ASSERT("KeStartAllProcessors: Could not create Kernel Stack for an AP");
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

        if (ApplicationProcessorsEnabled == FALSE){
            ProcessorCount--;
            /* Started means the AP itself is online, this doesn't mean it's seen by the kernel */
            DPRINT1("HalStartNextProcessor: Sucessful AP startup count is %X\n", ProcessorCount);
            /* We have finished starting processors, Time to cleanup! */
            MmFreeContiguousMemory(PAPInfo);
            MmDeleteKernelStack(KernelStack, FALSE);
            MmDeleteKernelStack(DPCStack, FALSE);
        }

    } while (ApplicationProcessorsEnabled);
}
