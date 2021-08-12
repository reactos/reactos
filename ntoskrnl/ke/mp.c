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
#define DPL_SYSTEM  0

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
    _Out_ PKPROCESSOR_STATE ProcessorState);

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
    KDESCRIPTOR Gdt, Idt;
   // PVOID NewGdt, NewIdt;
    extern LOADER_PARAMETER_BLOCK *KeLoaderBlock;
    do 
    {
        SIZE_T APInfoSize = sizeof(APINFO);
        APInfo = ExAllocatePool(NonPagedPool, APInfoSize);
        ASSERT(APInfo);
        ProcessorCount++;

        _sgdt(&Gdt.Limit);
        __sidt(&Idt.Limit);

        KernelStack = MmCreateKernelStack(FALSE, 0);
        ASSERT(KernelStack);
        DPCStack = MmCreateKernelStack(FALSE, 0);

        /* Initalize Architecture specific segments */
    #ifdef _M_AMD64
        /* Initalize a new PCR for the specific AP */
        KiInitializePcr(&APInfo->Pcr,
                        ProcessorCount,
                        &APInfo->Thread,
                        DPCStack);
        ProcessorState = APInfo->Pcr.Prcb.ProcessorState;

        /* Prep a new loaderblock for AP */
        KeLoaderBlock.KernelStack = (ULONG_PTR)KernelStack;
        KeLoaderBlock.Prcb = (ULONG_PTR)&APInfo->Pcr.Prcb;
        KeLoaderBlock.Thread = (ULONG_PTR)&APInfo->Pcr.Prcb.IdleThread;
    #elif _M_IX86
        /* Fully initalize AP's TSS */
        Ki386InitializeTss(&APInfo->Tss, &APInfo->Idt, &APInfo->Gdt[0]);

        /* Initalize a new PCR for the specific AP */
        KiInitializePcr(ProcessorCount,
                        &APInfo->Pcr,
                        &APInfo->Idt, 
                        (PKGDTENTRY)&APInfo->Gdt, 
                        &APInfo->Tss,
                        &APInfo->Thread,
                        DPCStack);
        ProcessorState = APInfo->Pcr.Prcb->ProcessorState;

        /* Prep a new loaderblock for AP */
        KeLoaderBlock->KernelStack = (ULONG_PTR)KernelStack;
        KeLoaderBlock->Prcb = (ULONG_PTR)&APInfo->Pcr.Prcb;
        KeLoaderBlock->Thread = (ULONG_PTR)&APInfo->Pcr.Prcb->IdleThread;
        ProcessorState.ContextFrame.Esp = (ULONG_PTR)KernelStack;
    #endif

        /* Prep ProcessorState then start the AP */
        KxInitAPProcessorState(&ProcessorState);

        /* Initalize GDT and IDT */
        //The following notes and code needs to be cleaned and change before commiting to PR

        
        /* Load TSR */
       // ProcessorState.SpecialRegisters.Tr = APTss;
    } while (HalStartNextProcessor(KeLoaderBlock, &ProcessorState));
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
    _Out_ PKPROCESSOR_STATE ProcessorState)
    {
        /* Prep Cr Regsters */
        ProcessorState->SpecialRegisters.Cr0 = __readcr0();
        ProcessorState->SpecialRegisters.Cr2 = __readcr2();
        ProcessorState->SpecialRegisters.Cr3 = __readcr3();
        ProcessorState->SpecialRegisters.Cr4 = __readcr4();
        ProcessorState->ContextFrame.Rip = (ULONG_PTR)KiSystemStartup;
    }
#else //_M_IX86
VOID
NTAPI
KxInitAPProcessorState(
    _Out_ PKPROCESSOR_STATE ProcessorState)
    {
        /* Prep Cr Regsters */
        ProcessorState->SpecialRegisters.Cr0 = __readcr0();
        ProcessorState->SpecialRegisters.Cr2 = __readcr2();
        ProcessorState->SpecialRegisters.Cr3 = __readcr3();
        ProcessorState->SpecialRegisters.Cr4 = __readcr4();

        /* Prepare Segment Registers */
        ProcessorState->ContextFrame.SegCs = KGDT_R0_CODE;
        ProcessorState->ContextFrame.SegSs = KGDT_R0_DATA;
        ProcessorState->ContextFrame.SegDs = KGDT_R0_DATA;
        ProcessorState->ContextFrame.SegEs = KGDT_R0_DATA; // This is vital for rep stosd.
        /* Clear GS */
        ProcessorState->ContextFrame.SegGs = 0;
        /* Set FS to PCR */
        ProcessorState->ContextFrame.SegFs = KGDT_R0_PCR;


        ProcessorState->ContextFrame.Eip = (ULONG_PTR)KiSystemStartup;
    }
#endif
