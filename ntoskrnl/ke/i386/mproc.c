/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Architecture specific source file to hold multiprocessor functions
 * COPYRIGHT:   Copyright 2023 Justin Miller <justin.miller@reactos.org>
 *              Copyright 2023 Victor Perevertkin <victor.perevertkin@reactos.org>
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

typedef struct _APINFO
{
    DECLSPEC_ALIGN(PAGE_SIZE) KIDTENTRY Idt[256];
    DECLSPEC_ALIGN(PAGE_SIZE) KGDTENTRY Gdt[128];
    DECLSPEC_ALIGN(16) UINT8 NMIStackData[DOUBLE_FAULT_STACK_SIZE];
    KIPCR Pcr;
    ETHREAD Thread;
    KTSS Tss;
    KTSS TssDoubleFault;
    KTSS TssNMI;
} APINFO, *PAPINFO;

typedef struct _AP_SETUP_STACK
{
    PVOID ReturnAddr;
    PVOID KxLoaderBlock;
} AP_SETUP_STACK, *PAP_SETUP_STACK; // Note: expected layout only for 32-bit x86

/* FUNCTIONS *****************************************************************/

CODE_SEG("INIT")
VOID
NTAPI
KeStartAllProcessors(VOID)
{
    PVOID KernelStack, DPCStack;
    ULONG ProcessorCount = 0;
    PAPINFO APInfo;

    while (TRUE)
    {
        ProcessorCount++;
        KernelStack = NULL;
        DPCStack = NULL;

        // Allocate structures for a new CPU.
        APInfo = ExAllocatePoolZero(NonPagedPool, sizeof(*APInfo), TAG_KERNEL);
        if (!APInfo)
            break;
        ASSERT(ALIGN_DOWN_POINTER_BY(APInfo, PAGE_SIZE) == APInfo);

        KernelStack = MmCreateKernelStack(FALSE, 0);
        if (!KernelStack)
            break;

        DPCStack = MmCreateKernelStack(FALSE, 0);
        if (!DPCStack)
            break;

        // Initalize a new PCR for the specific AP
        KiInitializePcr(ProcessorCount,
                        &APInfo->Pcr,
                        &APInfo->Idt[0],
                        &APInfo->Gdt[0],
                        &APInfo->Tss,
                        (PKTHREAD)&APInfo->Thread,
                        DPCStack);

        // Prepare descriptor tables
        KDESCRIPTOR bspGdt, bspIdt;
        __sgdt(&bspGdt.Limit);
        __sidt(&bspIdt.Limit);
        RtlCopyMemory(&APInfo->Gdt, (PVOID)bspGdt.Base, bspGdt.Limit + 1);
        RtlCopyMemory(&APInfo->Idt, (PVOID)bspIdt.Base, bspIdt.Limit + 1);

        KiSetGdtDescriptorBase(KiGetGdtEntry(&APInfo->Gdt, KGDT_R0_PCR), (ULONG_PTR)&APInfo->Pcr);
        KiSetGdtDescriptorBase(KiGetGdtEntry(&APInfo->Gdt, KGDT_DF_TSS), (ULONG_PTR)&APInfo->TssDoubleFault);
        KiSetGdtDescriptorBase(KiGetGdtEntry(&APInfo->Gdt, KGDT_NMI_TSS), (ULONG_PTR)&APInfo->TssNMI);

        KiSetGdtDescriptorBase(KiGetGdtEntry(&APInfo->Gdt, KGDT_TSS), (ULONG_PTR)&APInfo->Tss);
        // Clear TSS Busy flag (aka set the type to "TSS (Available)")
        KiGetGdtEntry(&APInfo->Gdt, KGDT_TSS)->HighWord.Bits.Type = I386_TSS;

        APInfo->TssDoubleFault.Esp0 = (ULONG_PTR)&APInfo->NMIStackData;
        APInfo->TssDoubleFault.Esp = (ULONG_PTR)&APInfo->NMIStackData;

        APInfo->TssNMI.Esp0 = (ULONG_PTR)&APInfo->NMIStackData;
        APInfo->TssNMI.Esp = (ULONG_PTR)&APInfo->NMIStackData;

        // Fill the processor state
        PKPROCESSOR_STATE ProcessorState = &APInfo->Pcr.Prcb->ProcessorState;
        RtlZeroMemory(ProcessorState, sizeof(*ProcessorState));

        ProcessorState->SpecialRegisters.Cr0 = __readcr0();
        ProcessorState->SpecialRegisters.Cr3 = __readcr3();
        ProcessorState->SpecialRegisters.Cr4 = __readcr4();

        ProcessorState->ContextFrame.SegCs = KGDT_R0_CODE;
        ProcessorState->ContextFrame.SegDs = KGDT_R3_DATA;
        ProcessorState->ContextFrame.SegEs = KGDT_R3_DATA;
        ProcessorState->ContextFrame.SegSs = KGDT_R0_DATA;
        ProcessorState->ContextFrame.SegFs = KGDT_R0_PCR;

        ProcessorState->SpecialRegisters.Gdtr.Base = (ULONG_PTR)APInfo->Gdt;
        ProcessorState->SpecialRegisters.Gdtr.Limit = sizeof(APInfo->Gdt) - 1;
        ProcessorState->SpecialRegisters.Idtr.Base = (ULONG_PTR)APInfo->Idt;
        ProcessorState->SpecialRegisters.Idtr.Limit = sizeof(APInfo->Idt) - 1;

        ProcessorState->SpecialRegisters.Tr = KGDT_TSS;

        ProcessorState->ContextFrame.Esp = (ULONG_PTR)KernelStack;
        ProcessorState->ContextFrame.Eip = (ULONG_PTR)KiSystemStartup;
        ProcessorState->ContextFrame.EFlags = __readeflags() & ~EFLAGS_INTERRUPT_MASK;

        ProcessorState->ContextFrame.Esp = (ULONG)((ULONG_PTR)ProcessorState->ContextFrame.Esp - sizeof(AP_SETUP_STACK));
        PAP_SETUP_STACK ApStack = (PAP_SETUP_STACK)ProcessorState->ContextFrame.Esp;
        ApStack->KxLoaderBlock = KeLoaderBlock;
        ApStack->ReturnAddr = NULL;

        // Update the LOADER_PARAMETER_BLOCK structure for the new processor
        KeLoaderBlock->KernelStack = (ULONG_PTR)KernelStack;
        KeLoaderBlock->Prcb = (ULONG_PTR)&APInfo->Pcr.Prcb;
        KeLoaderBlock->Thread = (ULONG_PTR)&APInfo->Pcr.Prcb->IdleThread;

        // Start the CPU
        DPRINT("Attempting to Start a CPU with number: %lu\n", ProcessorCount);
        if (!HalStartNextProcessor(KeLoaderBlock, ProcessorState))
        {
            break;
        }

        // And wait for it to start
        while (KeLoaderBlock->Prcb != 0)
        {
            //TODO: Add a time out so we don't wait forever
            KeMemoryBarrier();
            YieldProcessor();
        }
    }

    // The last CPU didn't start - clean the data
    ProcessorCount--;

    if (APInfo)
        ExFreePoolWithTag(APInfo, TAG_KERNEL);
    if (KernelStack)
        MmDeleteKernelStack(KernelStack, FALSE);
    if (DPCStack)
        MmDeleteKernelStack(DPCStack, FALSE);

    DPRINT1("KeStartAllProcessors: Successful AP startup count is %lu\n", ProcessorCount);
}
