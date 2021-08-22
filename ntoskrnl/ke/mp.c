/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Architecture specific source file to hold multiprocessor functions
 * COPYRIGHT:   Copyright 2021 Justin Miller <justinmiller100@gmail.com>
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#include <reactos/mp.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

ULONG ProcessorCount;
PAPINFO APInfo;
KDESCRIPTOR BSPGdt, BSPIdt;
KPROCESSOR_STATE ProcessorState;
PVOID KernelStack, DPCStack, PageTableLoc;
extern LOADER_PARAMETER_BLOCK *KeLoaderBlock;
PHYSICAL_ADDRESS GdtPhysicalLoc, IdtPhysicalLoc, PageTablePhysicalLoc;

/* FUNCTIONS *****************************************************************/

CODE_SEG("INIT")
VOID
NTAPI
KeStartAllProcessors()
{
    ProcessorCount = 0;
    while (TRUE)
    {
        SIZE_T APInfoSize = sizeof(APINFO);
        PHARDWARE_PTE PDE;
        APInfo = ExAllocatePool(NonPagedPool, APInfoSize);
        PDE = ExAllocatePool(NonPagedPool, 100000); //TODO replace the silly size place holder
        PageTablePhysicalLoc = MmGetPhysicalAddress(PDE);
        /* Load the GDT */
        _sgdt(&BSPGdt.Limit);
        __sidt(&BSPIdt.Limit);
        GdtPhysicalLoc = MmGetPhysicalAddress((PVOID)&APInfo->Gdt);
        IdtPhysicalLoc = MmGetPhysicalAddress((PVOID)&APInfo->Idt);
        RtlCopyMemory(&APInfo->Gdt, (PVOID)BSPGdt.Base, BSPGdt.Limit + 1);
        RtlCopyMemory(&APInfo->Idt, (PVOID)BSPIdt.Base, BSPIdt.Limit + 1);

        /* Create the stacks */
        KernelStack = MmCreateKernelStack(FALSE, 0);
        DPCStack = MmCreateKernelStack(FALSE, 0);

        /* Check for sucess and increment processorcount */
        ASSERT(APInfo);
        ASSERT(KernelStack);
        ProcessorCount++;

        /* Initalize Architecture specific segments */
    #ifdef _M_AMD64
        /* Initalize a new PCR for the specific AP */
        KiInitializePcr(&APInfo->Pcr,
                        ProcessorCount,
                        &APInfo->Thread,
                        DPCStack);
        ProcessorState = APInfo->Pcr.Prcb.ProcessorState;

        /* Prep a new loaderblock for AP */
        KeLoaderBlock->KernelStack = (ULONG_PTR)KernelStack;
        KeLoaderBlock->Prcb = (ULONG_PTR)&APInfo->Pcr.Prcb;
        KeLoaderBlock->Thread = (ULONG_PTR)&APInfo->Pcr.Prcb.IdleThread;
    #elif _M_IX86
        /* Fully initalize AP's TSS */
        Ki386InitializeTss(&APInfo->Tss, &APInfo->Idt[0], &APInfo->Gdt[0]);

        /* Initalize a new PCR for the specific AP */
        KiInitializePcr(ProcessorCount,
                        &APInfo->Pcr,
                        &APInfo->Idt[0], 
                        &APInfo->Gdt[0], 
                        &APInfo->Tss,
                        &APInfo->Thread,
                        DPCStack);
        ProcessorState = APInfo->Pcr.Prcb->ProcessorState;

        /* Prep a new loaderblock for AP */
        KeLoaderBlock->KernelStack = (ULONG_PTR)KernelStack;
        KeLoaderBlock->Prcb = (ULONG_PTR)&APInfo->Pcr.Prcb;
        KeLoaderBlock->Thread = (ULONG_PTR)&APInfo->Pcr.Prcb->IdleThread;

        /* Modify GDT */
        KiSetGdtEntry(KiGetGdtEntry(&APInfo->Gdt, KGDT_TSS), (ULONG_PTR)&APInfo->Tss,
            0x78-1, 0x09, 0, 0);
        KiSetGdtEntry(KiGetGdtEntry(&APInfo->Gdt, KGDT_R0_PCR), (ULONG_PTR)&APInfo->Pcr,
            MM_PAGE_SIZE - 1, TYPE_DATA, 0, 2);
        KiSetGdtEntry(KiGetGdtEntry(&APInfo->Gdt, KGDT_DF_TSS), (ULONG_PTR)&APInfo->TssDoubleFault,
            0xFFFF, 0x09, 0, 2);
        KiSetGdtEntry(KiGetGdtEntry(&APInfo->Gdt, KGDT_NMI_TSS), (ULONG_PTR)&APInfo->TssNMI,
            0xFFFF, TYPE_CODE, 0, 2);
        ProcessorState.ContextFrame.Eax = (ULONG_PTR)PDE;
        ProcessorState.ContextFrame.Ecx = PageTablePhysicalLoc.QuadPart;
    #endif

        /* Prep ProcessorState then start the AP */
        KxInitAPProcessorState(&ProcessorState);
        KxInitAPTemporaryPageTables(PDE, &ProcessorState);

        if (!HalStartNextProcessor(KeLoaderBlock, &ProcessorState))
        {
            break;
        }
        while (KeLoaderBlock->Prcb)
        {
            KeMemoryBarrier();
            DPRINT("KeStartAllProcessors: Waiting for initalization confirmation from supposive CPU: #%X\n", ProcessorCount);
            YieldProcessor();
        }
    }

    ProcessorCount--;
    /* Started means the AP itself is online, doesn't quite mean it's seen by the kernel */
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
        UNIMPLEMENTED;
    }

VOID
NTAPI
KxInitAPTemporaryPageTables(PHARDWARE_PTE PageTableDirectory, 
                            PKPROCESSOR_STATE ProcessorState)
{
    UNIMPLEMENTED;
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
        ProcessorState->ContextFrame.SegSs = KGDT_R0_DATA;
        ProcessorState->ContextFrame.SegDs = KGDT_R0_DATA;
        ProcessorState->ContextFrame.SegEs = KGDT_R0_DATA; // This is vital for rep stosd.
        /* Clear GS */
        ProcessorState->ContextFrame.SegGs = 0;

        /* Set Special PCR and KernelStack */
        ProcessorState->ContextFrame.SegFs = (ULONG_PTR)&APInfo->Pcr;
        ProcessorState->ContextFrame.Esp = (ULONG_PTR)KernelStack;

        /* Setup GDT Ptrs for AP */
        ProcessorState->SpecialRegisters.Gdtr.Base = GdtPhysicalLoc.QuadPart;
        ProcessorState->SpecialRegisters.Gdtr.Limit = BSPGdt.Limit;
        ProcessorState->SpecialRegisters.Idtr.Base = IdtPhysicalLoc.QuadPart;
        ProcessorState->SpecialRegisters.Idtr.Limit = BSPIdt.Limit;

        /* Write other objects */
        ProcessorState->ContextFrame.Eip = (ULONG_PTR)KiSystemStartup;
    }

VOID
NTAPI
KxInitAPTemporaryPageTables(PHARDWARE_PTE PageTableDirectory, 
                            PKPROCESSOR_STATE ProcessorState)
{
    //PHARDWARE_PTE BootStubPTE, GDTPTE, IDTPTE;
    // Map the page directory at 0xC0000000 (maps itself)
    PageTableDirectory[3].PageFrameNumber = (ULONG)PageTableDirectory >> MM_PAGE_SHIFT;
    PageTableDirectory[3].Valid = 1;
    PageTableDirectory[3].Write = 1;
    //PDE [0] , pointing to page table for bootstub
    PageTableDirectory[0].Valid = 0;
    PageTableDirectory[0].Write = 0;
    //PDE [1] , pointing to page table for GDT
    PageTableDirectory[1].PageFrameNumber = (ULONG_PTR)ProcessorState->SpecialRegisters.Gdtr.Base >> MM_PAGE_SHIFT;
    PageTableDirectory[1].Valid = 1;
    PageTableDirectory[1].Write = 1;
    //PDE [2] , pointing to page table for IDT
    PageTableDirectory[2].PageFrameNumber = (ULONG_PTR)ProcessorState->SpecialRegisters.Idtr.Base >> MM_PAGE_SHIFT;
    PageTableDirectory[2].Valid = 1;
    PageTableDirectory[2].Write = 1;
}

/* GDT Functions, TODO: Find a way to share these between Freeldr and here */

FORCEINLINE
PKGDTENTRY
KiGetGdtEntry(
    IN PVOID pGdt,
    IN USHORT Selector)
{
    return (PKGDTENTRY)((ULONG_PTR)pGdt + (Selector & ~RPL_MASK));
}

FORCEINLINE
VOID
KiSetGdtDescriptorBase(
    IN OUT PKGDTENTRY Entry,
    IN ULONG32 Base)
{
    Entry->BaseLow = (USHORT)(Base & 0xffff);
    Entry->HighWord.Bytes.BaseMid = (UCHAR)((Base >> 16) & 0xff);
    Entry->HighWord.Bytes.BaseHi  = (UCHAR)((Base >> 24) & 0xff);
    // Entry->BaseUpper = (ULONG)(Base >> 32);
}

FORCEINLINE
VOID
KiSetGdtDescriptorLimit(
    IN OUT PKGDTENTRY Entry,
    IN ULONG Limit)
{
    if (Limit < 0x100000)
    {
        Entry->HighWord.Bits.Granularity = 0;
    }
    else
    {
        Limit >>= 12;
        Entry->HighWord.Bits.Granularity = 1;
    }
    Entry->LimitLow = (USHORT)(Limit & 0xffff);
    Entry->HighWord.Bits.LimitHi = ((Limit >> 16) & 0x0f);
}

VOID
KiSetGdtEntryEx(
    IN OUT PKGDTENTRY Entry,
    IN ULONG32 Base,
    IN ULONG Limit,
    IN UCHAR Type,
    IN UCHAR Dpl,
    IN BOOLEAN Granularity,
    IN UCHAR SegMode) // 0: 16-bit, 1: 32-bit, 2: 64-bit
{
    KiSetGdtDescriptorBase(Entry, Base);
    KiSetGdtDescriptorLimit(Entry, Limit);
    Entry->HighWord.Bits.Type = (Type & 0x1f);
    Entry->HighWord.Bits.Dpl  = (Dpl & 0x3);
    Entry->HighWord.Bits.Pres = (Type != 0); // Present, must be 1 when the GDT entry is valid.
    Entry->HighWord.Bits.Sys  = 0;           // System
    Entry->HighWord.Bits.Reserved_0  = 0;    // LongMode = !!(SegMode & 1);
    Entry->HighWord.Bits.Default_Big = !!(SegMode & 2);
    Entry->HighWord.Bits.Granularity |= !!Granularity; // The flag may have been already set by KiSetGdtDescriptorLimit().
    // Entry->MustBeZero = 0;
}

FORCEINLINE
VOID
KiSetGdtEntry(
    IN OUT PKGDTENTRY Entry,
    IN ULONG32 Base,
    IN ULONG Limit,
    IN UCHAR Type,
    IN UCHAR Dpl,
    IN UCHAR SegMode) // 0: 16-bit, 1: 32-bit, 2: 64-bit
{
    KiSetGdtEntryEx(Entry, Base, Limit, Type, Dpl, FALSE, SegMode);
}

#endif