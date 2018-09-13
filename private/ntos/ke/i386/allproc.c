/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    allproc.c

Abstract:

    This module allocates and initializes kernel resources required
    to start a new processor, and passes a complete process_state
    structre to the hal to obtain a new processor.  This is done
    for every processor.

Author:

    Ken Reneris (kenr) 22-Jan-92

Environment:

    Kernel mode only.
    Phase 1 of bootup

Revision History:

--*/


#include "ki.h"

#ifdef NT_UP

VOID
KeStartAllProcessors (
    VOID
    )
{
        // UP Build - this function is a nop
}

#else

extern ULONG KeRegisteredProcessors;

static VOID
KiCloneDescriptor (
   IN PKDESCRIPTOR  pSrcDescriptorInfo,
   IN PKDESCRIPTOR  pDestDescriptorInfo
   );

static VOID
KiCloneSelector (
   IN ULONG    SrcSelector,
   IN PKGDTENTRY    pNGDT,
   IN PKDESCRIPTOR  pDestDescriptor
   );


#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,KeStartAllProcessors)
#pragma alloc_text(INIT,KiCloneDescriptor)
#pragma alloc_text(INIT,KiCloneSelector)
#endif

#if !defined(NT_UP)

ULONG KiBarrierWait = 0;

#endif



VOID
KeStartAllProcessors (
    VOID
    )
/*++

Routine Description:

    Called by p0 during phase 1 of bootup.  This function implements
    the x86 specific code to contact the hal for each system processor.

Arguments:

Return Value:

    All available processors are sent to KiSystemStartup.

--*/
{
    KPROCESSOR_STATE    ProcessorState;
    KDESCRIPTOR         Descriptor;
    KDESCRIPTOR         TSSDesc, DFTSSDesc, NMITSSDesc, PCRDesc;
    PKGDTENTRY          pGDT;
    PVOID               pStack;
    PVOID               pDpcStack;
    ULONG               DFStack;
    PUCHAR              pThreadObject;
    PULONG              pTopOfStack;
    ULONG               NewProcessorNumber;
    BOOLEAN             NewProcessor;
    PKPROCESS           Process;
    PKTHREAD            Thread;
    PKTSS               pTSS;
    PLIST_ENTRY         NextEntry;
    LONG                NumberProcessors;

    //
    // If the registered number of processors is greater than the maximum
    // number of processors supported, then only allow the maximum number
    // of supported processors.
    //

    if (KeRegisteredProcessors > MAXIMUM_PROCESSORS) {
        KeRegisteredProcessors = MAXIMUM_PROCESSORS;
    }

    //
    // Set barrier that will prevent any other processor from entering the
    // idle loop until all processors have been started.
    //

    KiBarrierWait = 1;


    while ((ULONG)KeNumberProcessors < KeRegisteredProcessors) {
        //
        //  Build up a processor state for new processor
        //

        RtlZeroMemory ((PVOID) &ProcessorState, sizeof ProcessorState);


        //
        //  Give the new processor its own GDT
        //

        _asm {
            sgdt    Descriptor.Limit
        }

        KiCloneDescriptor (&Descriptor,
                         &ProcessorState.SpecialRegisters.Gdtr);

        pGDT = (PKGDTENTRY) ProcessorState.SpecialRegisters.Gdtr.Base;


        //
        //  Give new processor its own IDT
        //

        _asm {
            sidt    Descriptor.Limit
        }
        KiCloneDescriptor (&Descriptor,
                         &ProcessorState.SpecialRegisters.Idtr);


        //
        //  Give new processor its own TSS and PCR
        //

        KiCloneSelector   (KGDT_TSS, pGDT, &TSSDesc);
        KiCloneSelector   (KGDT_R0_PCR, pGDT, &PCRDesc);

        //
        // Allocate double-fault TSS & stack, and NMI TSS
        //

        KiCloneSelector (KGDT_DF_TSS, pGDT, &DFTSSDesc);
        DFStack = (ULONG)ExAllocatePoolWithTag(NonPagedPool, DOUBLE_FAULT_STACK_SIZE, '  eK');
        pTSS = (PKTSS)DFTSSDesc.Base;
        pTSS->Esp0 = DFStack + DOUBLE_FAULT_STACK_SIZE;
        pTSS->NotUsed2[5] = DFStack + DOUBLE_FAULT_STACK_SIZE;

        KiCloneSelector (KGDT_NMI_TSS, pGDT, &NMITSSDesc);
        pTSS = (PKTSS)NMITSSDesc.Base;
        pTSS->Esp0 = DFStack + DOUBLE_FAULT_STACK_SIZE;
        pTSS->NotUsed2[5] = DFStack + DOUBLE_FAULT_STACK_SIZE;


        //
        //  Set other SpecialRegisters in processor state
        //

        _asm {
            mov     eax, cr0
            and     eax, NOT (CR0_AM or CR0_WP)
            mov     ProcessorState.SpecialRegisters.Cr0, eax
            mov     eax, cr3
            mov     ProcessorState.SpecialRegisters.Cr3, eax

            pushfd
            pop     ProcessorState.ContextFrame.EFlags
            and     ProcessorState.ContextFrame.EFlags, NOT EFLAGS_INTERRUPT_MASK
        }

        ProcessorState.SpecialRegisters.Tr  = KGDT_TSS;
        pGDT[KGDT_TSS>>3].HighWord.Bytes.Flags1 = 0x89;

#if defined(_X86PAE_)
        ProcessorState.SpecialRegisters.Cr4 = CR4_PAE;
#endif

        //
        // Allocate a DPC stack, idle thread stack and ThreadObject for
        // the new processor.
        //

        pStack = MmCreateKernelStack (FALSE);
        pDpcStack = MmCreateKernelStack (FALSE);
        pThreadObject = (PUCHAR)ExAllocatePoolWithTag (NonPagedPool, sizeof(ETHREAD), '  eK');

        //
        //  Zero initialize these...
        //

        RtlZeroMemory ((PVOID) PCRDesc.Base, sizeof (KPCR));
        RtlZeroMemory ((PVOID) pThreadObject, sizeof (KTHREAD));


        //
        //  Setup context
        //  Push variables onto new stack
        //

        pTopOfStack = (PULONG) pStack;
        pTopOfStack[-1] = (ULONG) KeLoaderBlock;
        ProcessorState.ContextFrame.Esp = (ULONG) (pTopOfStack-2);
        ProcessorState.ContextFrame.Eip = (ULONG) KiSystemStartup;

        ProcessorState.ContextFrame.SegCs = KGDT_R0_CODE;
        ProcessorState.ContextFrame.SegDs = KGDT_R3_DATA;
        ProcessorState.ContextFrame.SegEs = KGDT_R3_DATA;
        ProcessorState.ContextFrame.SegFs = KGDT_R0_PCR;
        ProcessorState.ContextFrame.SegSs = KGDT_R0_DATA;


        //
        //  Initialize new processor's PCR & Prcb
        //

        NewProcessorNumber = KeNumberProcessors;
        KiInitializePcr (
            (ULONG)       NewProcessorNumber,
            (PKPCR)       PCRDesc.Base,
            (PKIDTENTRY)  ProcessorState.SpecialRegisters.Idtr.Base,
            (PKGDTENTRY)  ProcessorState.SpecialRegisters.Gdtr.Base,
            (PKTSS)       TSSDesc.Base,
            (PKTHREAD)    pThreadObject,
            (PVOID)       pDpcStack
        );


        //
        //  Adjust LoaderBlock so it has the next processors state
        //

        KeLoaderBlock->KernelStack = (ULONG) pTopOfStack;
        KeLoaderBlock->Thread = (ULONG) pThreadObject;
        KeLoaderBlock->Prcb = (ULONG) ((PKPCR) PCRDesc.Base)->Prcb;


        //
        //  Contact hal to start new processor
        //

        NewProcessor = HalStartNextProcessor (KeLoaderBlock, &ProcessorState);


        if (!NewProcessor) {

            //
            //  There wasn't another processor, so free resources and
            //  break
            //

            KiProcessorBlock[NewProcessorNumber] = NULL;
            ExFreePool ((PVOID) ProcessorState.SpecialRegisters.Gdtr.Base);
            ExFreePool ((PVOID) ProcessorState.SpecialRegisters.Idtr.Base);
            ExFreePool ((PVOID) TSSDesc.Base);
            ExFreePool ((PVOID) DFTSSDesc.Base);
            ExFreePool ((PVOID) NMITSSDesc.Base);
            ExFreePool ((PVOID) PCRDesc.Base);
            ExFreePool ((PVOID) pThreadObject);
            ExFreePool ((PVOID) DFStack);
            MmDeleteKernelStack ( pStack, FALSE);
            MmDeleteKernelStack ( pDpcStack, FALSE);
            break;
        }


        //
        //  Wait for processor to initialize in kernel, then loop for another
        //

        while (*((volatile ULONG *) &KeLoaderBlock->Prcb) != 0) {
            KeYieldProcessor();
        }
    }

    //
    // Reset and synchronize the performance counters of all processors, by
    // applying a null adjustment to the interrupt time
    //

    KiAdjustInterruptTime (0);

    //
    // Allow all processors that were started to enter the idle loop and
    // begin execution.
    //

    KiBarrierWait = 0;
}



static VOID
KiCloneSelector (
   IN ULONG    SrcSelector,
   IN PKGDTENTRY    pNGDT,
   IN PKDESCRIPTOR  pDestDescriptor
/*++

Routine Description:

    Makes a copy of the current selector's data, and update the new
    gdt's linear address to point to the new copy.

Arguments:
    SrcSelector     -   Selector value to clone
    pNGDT           -   New gdt table which is being built
    DescDescriptor  -   descriptor structure to fill in with resulting memory

Return Value:

--*/
   )
{
    KDESCRIPTOR Descriptor;
    PKGDTENTRY  pGDT;
    ULONG       CurrentBase;
    ULONG       NewBase;

    _asm {
        sgdt    fword ptr [Descriptor.Limit]    ; Get GDT's addr
    }

    pGDT   = (PKGDTENTRY) Descriptor.Base;
    pGDT  += SrcSelector >> 3;
    pNGDT += SrcSelector >> 3;

    CurrentBase = pGDT->BaseLow | (pGDT->HighWord.Bits.BaseMid << 16) |
                 (pGDT->HighWord.Bits.BaseHi << 24);

    Descriptor.Base  = CurrentBase;
    Descriptor.Limit = pGDT->LimitLow;
    if (pGDT->HighWord.Bits.Granularity & GRAN_PAGE)
        Descriptor.Limit = (Descriptor.Limit << PAGE_SHIFT) -1;

    KiCloneDescriptor (&Descriptor, pDestDescriptor);
    NewBase = pDestDescriptor->Base;

    pNGDT->BaseLow = (USHORT) NewBase & 0xffff;
    pNGDT->HighWord.Bits.BaseMid = (UCHAR) (NewBase >> 16) & 0xff;
    pNGDT->HighWord.Bits.BaseHi  = (UCHAR) (NewBase >> 24) & 0xff;
}



static VOID
KiCloneDescriptor (
   IN PKDESCRIPTOR  pSrcDescriptor,
   IN PKDESCRIPTOR  pDestDescriptor
   )
/*++

Routine Description:

    Makes a copy of the specified descriptor, and supplies a return
    descriptor for the new copy

Arguments:
    pSrcDescriptor  - descriptor to clone
    pDescDescriptor - the cloned descriptor

Return Value:

--*/
{
    ULONG   Size;

    Size = pSrcDescriptor->Limit + 1;
    pDestDescriptor->Limit = (USHORT) Size -1;
    pDestDescriptor->Base  = (ULONG)  ExAllocatePoolWithTag (NonPagedPool, Size, '  eK');

    RtlMoveMemory ((PVOID) pDestDescriptor->Base,
                   (PVOID) pSrcDescriptor->Base, Size);
}


#endif      // !NT_UP
