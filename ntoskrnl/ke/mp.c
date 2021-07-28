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

/* FUNCTIONS *****************************************************************/

CODE_SEG("INIT")
VOID
NTAPI
KeStartAllProcessors()
{
     /* Prep some globals for the AP */
    PVOID KernelStack;
    PVOID DPCStack;
    PAPINFO APInfo;
    KDESCRIPTOR GdtDesc, IdtDesc;
    KPROCESSOR_STATE ProcessorState;
    __sgdt(&GdtDesc.Limit);
    __sidt(&IdtDesc.Limit);

    /* Attempt to allocate memory for the new setup */
    SIZE_T APInfoSize = sizeof(APInfo);

    do
    {
        (PVOID)APInfo = ExAllocatePool(NonPagedPool, APInfoSize);
        ASSERT(APInfo);
        ProcessorState.SpecialRegisters.Cr3 = __readcr3();
         ProcessorState.ContextFrame.Eip = (ULONG_PTR)KiSystemStartup;
        ProcessorCount++;

        //RtlCopyMemory(APInfo->Gdt, (PVOID)GdtDesc.Base, GdtDesc.Limit);
        //RtlCopyMemory(APInfo->Idt, (PVOID)IdtDesc.Base, IdtDesc.Limit);


        KernelStack = MmCreateKernelStack(FALSE, 0);
        ASSERT(KernelStack);

        DPCStack = MmCreateKernelStack(FALSE, 0);
        if (!DPCStack)
            goto APCleanup;


        /* Initalize a new PCR for the specific AP */
#ifdef _M_IX86
#if 0
        KiInitializePcr(ProcessorCount,
                        &APInfo->Pcr,
                        &APInfo->Idt, 
                        (PKGDTENTRY)&APInfo->Gdt, 
                        &APInfo->Tss,
                        &APInfo->Thread,
                        DPCStack);
#endif
#elif _M_AMD64
    /* Initalize a new PCR for the specific AP */
#endif

        /* Prep a new loaderblock for AP */
        KeLoaderBlock->KernelStack = (ULONG_PTR)KernelStack;
        //KeLoaderBlock->Prcb = (ULONG_PTR)APInfo->Pcr.Prcb;
        //KeLoaderBlock->Thread = (ULONG_PTR)&APInfo->Thread;


#ifdef _M_IX86
        /* Fully initalize AP's TSS */
        // Ki386InitializeTss(&APInfo->Tss, pIdt, pGdt);
#endif

    } while (HalStartNextProcessor(KeLoaderBlock, &ProcessorState));

APCleanup:
    ProcessorCount--;
    /* Started means the AP itself is online, this doesn't mean it's seen by the kernel */
    DPRINT1("HalStartNextProcessor: Sucessful AP startup count is %X\n", ProcessorCount);
    /* We have finished starting processors, Time to cleanup! */
    //MmFreeContiguousMemory(APInfo);
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
       /* Prep Cr Regsters */


    }
#endif
