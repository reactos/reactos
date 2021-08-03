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

ULONG ProcessorCount = 0;

typedef struct _APINFO
{
    KIPCR Pcr;
    KTHREAD Thread;
    KGDTENTRY Gdt[128];
    KIDTENTRY Idt;
    KTSS Tss;
} APINFO, *PAPINFO;

VOID
NTAPI
KxInitAPProcessorState(
    _Out_ KPROCESSOR_STATE ProcessorState);

/* FUNCTIONS *****************************************************************/

CODE_SEG("INIT")
VOID
NTAPI
KeStartAllProcessors()
{
    PVOID KernelStack;
    PVOID DPCStack;
    PAPINFO APInfo;
    KPROCESSOR_STATE ProcessorState;
    do 
    {
        SIZE_T APInfoSize = sizeof(APINFO);
        (PVOID)APInfo = ExAllocatePool(NonPagedPool, APInfoSize);
        ASSERT(APInfo);
        ProcessorCount++;

        KernelStack = MmCreateKernelStack(FALSE, 0);
        ASSERT(KernelStack);
        DPCStack = MmCreateKernelStack(FALSE, 0);

        /* Initalize a new PCR for the specific AP */
        #ifdef _M_AMD64
        KiInitializePcr(&APInfo->Pcr,
                        ProcessorCount,
                        &APInfo->Thread,
                        DPCStack);
        ProcessorState = APInfo->Pcr.Prcb.ProcessorState;
        #elif _M_IX86
        KiInitializePcr(ProcessorCount,
                        &APInfo->Pcr,
                        &APInfo->Idt, 
                        (PKGDTENTRY)&APInfo->Gdt, 
                        &APInfo->Tss,
                        &APInfo->Thread,
                        DPCStack);
        ProcessorState = APInfo->Pcr.Prcb->ProcessorState;
        #endif    
    
        /* Prep ProcessorState then start the AP */
        KxInitAPProcessorState(ProcessorState);
    } while (HalStartNextProcessor(KeLoaderBlock, &ProcessorState));
APCleanup:
    ProcessorCount--;
    /* Started means the AP itself is online, this doesn't mean it's seen by the kernel */
    DPRINT1("HalStartNextProcessor: Sucessful AP startup count is %X\n", ProcessorCount);
    /* We have finished starting processors, Time to cleanup! */
    MmDeleteKernelStack(KernelStack, FALSE);
    MmDeleteKernelStack(DPCStack, FALSE);
}

#ifdef _M_AMD64
VOID
NTAPI
KxInitAPProcessorState(
    _Out_ KPROCESSOR_STATE ProcessorState)
    {

    }
#else //_M_IX86
VOID
NTAPI
KxInitAPProcessorState(
    _Out_ KPROCESSOR_STATE ProcessorState)
    {

    }
#endif