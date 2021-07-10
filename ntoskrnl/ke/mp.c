/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Architecture specific source file to hold multiprocessor functions
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
     /* Prep some globals for the AP */
    PVOID KernelStack;
    PVOID DPCStack;
    PVOID PAPInfo;
    KPROCESSOR_STATE ProcessorState;
    KDESCRIPTOR GdtDesc, IdtDesc;

    HighestPhysicalAddress.QuadPart = UINT64_MAX;
    __sgdt(&GdtDesc.Limit);
    __sidt(&IdtDesc.Limit);

    /* Attempt to allocate memory for the new setup */
    SIZE_T APInfoSize = sizeof(struct APInfo);

    do
    {
        PAPInfo = MmAllocateContiguousMemory(APInfoSize, HighestPhysicalAddress);
        ASSERT(PAPInfo);

        ProcessorCount++;

        RtlCopyMemory((PVOID)((ULONG_PTR)PAPInfo + sizeof(KPCR) + sizeof(KTSS)), (PVOID)GdtDesc.Base, GdtDesc.Limit + 1);
        RtlCopyMemory((PVOID)((ULONG_PTR)PAPInfo + sizeof(KPCR) + sizeof(KTSS) + GdtDesc.Limit + 1), (PVOID)IdtDesc.Base, IdtDesc.Limit + 1);

        /* Prep GDT and IDT based on BSPs */
        PKGDTENTRY pGdt = (PKGDTENTRY)((ULONG_PTR)PAPInfo + sizeof(KPCR) + sizeof(KTSS));
        PKIDTENTRY pIdt = (PKIDTENTRY)((ULONG_PTR)PAPInfo + sizeof(KPCR) + sizeof(KTSS) + GdtDesc.Limit + 1);

        /* Allocate PCR - TSS - InitalThread */
        PKPCR pKPcr = (PKPCR)PAPInfo;
        PKTSS pKTss = (PKTSS)((ULONG_PTR)PAPInfo + sizeof(KPCR));
        PKTHREAD InitialThread = (PKTHREAD)((ULONG_PTR) PAPInfo + sizeof(KPCR) + sizeof(KTSS));

        KernelStack = MmCreateKernelStack(FALSE, 0);
        ASSERT(KernelStack);

        DPCStack = MmCreateKernelStack(FALSE, 0);
        if (!DPCStack)
        {
            MmDeleteKernelStack(KernelStack, FALSE);
        }

#ifdef _M_AMD64 
        /* Initalize a new PCR for the specific AP */

#else
        KiInitializePcr(ProcessorCount,
                            (PKIPCR)pKPcr,
                            pIdt, 
                            pGdt, 
                            pKTss,
                            InitialThread,
                            (PVOID)DPCStack);
#endif

        /* Prep a new loaderblock for AP */
        KeLoaderBlock->KernelStack = (ULONG_PTR)KernelStack;
        KeLoaderBlock->Prcb = (ULONG_PTR)pKPcr;
        KeLoaderBlock->Thread = (ULONG_PTR)InitialThread;


#ifdef _M_IX86
        /* Fully initalize AP's TSS */
        Ki386InitializeTss(pKTss, pIdt, pGdt);
#endif

    } while (HalStartNextProcessor(KeLoaderBlock, &ProcessorState));

    ProcessorCount--;
    /* Started means the AP itself is online, this doesn't mean it's seen by the kernel */
    DPRINT1("HalStartNextProcessor: Sucessful AP startup count is %X\n", ProcessorCount);
    /* We have finished starting processors, Time to cleanup! */
    MmFreeContiguousMemory(PAPInfo);
    MmDeleteKernelStack(KernelStack, FALSE);
    MmDeleteKernelStack(DPCStack, FALSE);
} 

#ifdef _M_AMD64
VOID
NTAPI
KxInitAPProcessorState(
    _Out_ PKPROCESSOR_STATE ProcessorState)
    {

    }
#else
VOID
NTAPI
KxInitAPProcessorState(
    _Out_ PKPROCESSOR_STATE ProcessorState)
    {

    }
#endif