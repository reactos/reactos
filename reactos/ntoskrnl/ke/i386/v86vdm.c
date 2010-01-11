/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            ntoskrnl/ke/i386/v86vdm.c
 * PURPOSE:         V8086 and VDM Trap Emulation
 * PROGRAMMERS:     ReactOS Portable Systems Group
 *                  Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

ULONG KeI386EFlagsAndMaskV86 = EFLAGS_USER_SANITIZE;
ULONG KeI386EFlagsOrMaskV86 = EFLAGS_INTERRUPT_MASK;
PVOID Ki386IopmSaveArea;
BOOLEAN KeI386VirtualIntExtensions = FALSE;
const PULONG KiNtVdmState = (PULONG)FIXED_NTVDMSTATE_LINEAR_PC_AT;

/* UNHANDLED OPCODES **********************************************************/

KiVdmUnhandledOpcode(F);
KiVdmUnhandledOpcode(OUTSW);
KiVdmUnhandledOpcode(OUTSB);
KiVdmUnhandledOpcode(INSB);
KiVdmUnhandledOpcode(INSW);
KiVdmUnhandledOpcode(NPX);
KiVdmUnhandledOpcode(INBimm);
KiVdmUnhandledOpcode(INWimm);
KiVdmUnhandledOpcode(OUTBimm);
KiVdmUnhandledOpcode(OUTWimm);
KiVdmUnhandledOpcode(INB);
KiVdmUnhandledOpcode(INW);
KiVdmUnhandledOpcode(OUTB);
KiVdmUnhandledOpcode(OUTW);
KiVdmUnhandledOpcode(HLT);
KiVdmUnhandledOpcode(INTO);
KiVdmUnhandledOpcode(INV);

/* OPCODE HANDLERS ************************************************************/
    
BOOLEAN
FASTCALL
KiVdmOpcodePUSHF(IN PKTRAP_FRAME TrapFrame,
                 IN ULONG Flags)
{
    ULONG Esp, V86EFlags, TrapEFlags;
    
    /* Get current V8086 flags and mask out interrupt flag */
    V86EFlags = *KiNtVdmState;
    V86EFlags &= ~EFLAGS_INTERRUPT_MASK;
    
    /* Get trap frame EFLags and leave only align, nested task and interrupt */
    TrapEFlags = TrapFrame->EFlags;
    TrapEFlags &= (EFLAGS_ALIGN_CHECK | EFLAGS_NESTED_TASK | EFLAGS_INTERRUPT_MASK);
    
    /* Add in those flags if they exist, and add in the IOPL flag */
    V86EFlags |= TrapEFlags;
    V86EFlags |= EFLAGS_IOPL;
    
    /* Build flat ESP */
    Esp = (TrapFrame->HardwareSegSs << 4) + (USHORT)TrapFrame->HardwareEsp;
    Esp -= 2;
    
    /* Check for OPER32 */
    if (Flags & PFX_FLAG_OPER32)
    {
        /* Save EFlags */
        Esp -= 2;
        *(PULONG)(Esp - 2) = V86EFlags;
    }
    else
    {
        /* Save EFLags */
        *(PUSHORT)Esp = (USHORT)V86EFlags;
    }
    
    /* Set new ESP and EIP */
    TrapFrame->HardwareEsp = (USHORT)Esp;
    TrapFrame->Eip += (Flags & 0xFF);
    
    /* We're done */
    return TRUE;
}

BOOLEAN
FASTCALL
KiVdmOpcodePOPF(IN PKTRAP_FRAME TrapFrame,
                IN ULONG Flags)
{
    ULONG Esp, V86EFlags, EFlags, TrapEFlags;
    
    /* Build flat ESP */
    Esp = (TrapFrame->HardwareSegSs << 4) + (USHORT)TrapFrame->HardwareEsp;
    
    /* Read EFlags */
    EFlags = *(PULONG)Esp;
    Esp += 4;
    
    /* Check for OPER32 */
    if (!(Flags & PFX_FLAG_OPER32))
    {
        /* Read correct flags and use correct stack address */
        Esp -= 2;
        EFlags &= 0xFFFF;
    }
    
    /* Set new ESP */
    TrapFrame->HardwareEsp = Esp;
    
    /* Mask out IOPL from the flags */
    EFlags &= ~EFLAGS_IOPL;
    
    /* Save the V86 flags, but mask out the nested task flag */
    V86EFlags = EFlags & ~EFLAGS_NESTED_TASK;
    
    /* Now leave only alignment, nested task and interrupt flag */
    EFlags &= (EFLAGS_ALIGN_CHECK | EFLAGS_NESTED_TASK | EFLAGS_INTERRUPT_MASK);
    
    /* FIXME: Check for VME support */
    
    /* Add V86 and Interrupt flag */
    V86EFlags |= EFLAGS_V86_MASK | EFLAGS_INTERRUPT_MASK;
    
    /* Update EFlags in trap frame */
    TrapEFlags = TrapFrame->EFlags;
    TrapFrame->EFlags = (TrapFrame->EFlags & EFLAGS_VIP) | V86EFlags;
    
    /* Check if ESP0 needs to be fixed up */
    if (TrapEFlags & EFLAGS_V86_MASK) Ki386AdjustEsp0(TrapFrame);
    
    /* Update the V8086 EFlags state */
    KiVdmClearVdmEFlags(EFLAGS_ALIGN_CHECK | EFLAGS_NESTED_TASK | EFLAGS_INTERRUPT_MASK);
    KiVdmSetVdmEFlags(EFlags);
   
    /* FIXME: Check for VDM interrupts */
    
    /* Update EIP */
    TrapFrame->Eip += (Flags & 0xFF);
    
    /* We're done */
    return TRUE;
}

BOOLEAN
FASTCALL
KiVdmOpcodeINTnn(IN PKTRAP_FRAME TrapFrame,
                 IN ULONG Flags)
{
    ULONG Esp, V86EFlags, TrapEFlags, Eip, Interrupt;
    
    /* Read trap frame EFlags */
    TrapEFlags = TrapFrame->EFlags;
    
    /* Remove interrupt flag from V8086 EFlags */
    V86EFlags = *KiNtVdmState;
    KiVdmClearVdmEFlags(EFLAGS_INTERRUPT_MASK);
    
    /* Keep only alignment and interrupt flag from the V8086 state */
    V86EFlags &= (EFLAGS_ALIGN_CHECK | EFLAGS_INTERRUPT_MASK);
    
    /* FIXME: Support VME */
    
    /* Mask in the relevant V86 EFlags into the trap flags */
    V86EFlags |= (TrapEFlags & ~EFLAGS_INTERRUPT_MASK);
    
    /* And mask out the VIF, nested task and TF flag from the trap flags */
    TrapFrame->EFlags = TrapEFlags &~ (EFLAGS_VIF | EFLAGS_NESTED_TASK | EFLAGS_TF);
    
    /* Add the IOPL flag to the local trap flags */
    V86EFlags |= EFLAGS_IOPL;
    
    /* Build flat ESP */
    Esp = (TrapFrame->HardwareSegSs << 4) + TrapFrame->HardwareEsp;
    
    /* Push EFlags */
    Esp -= 2;
    *(PUSHORT)(Esp) = (USHORT)V86EFlags;
    
    /* Push CS */
    Esp -= 2;
    *(PUSHORT)(Esp) = (USHORT)TrapFrame->SegCs;
    
    /* Push IP */
    Esp -= 2;
    *(PUSHORT)(Esp) = (USHORT)TrapFrame->Eip + (Flags & 0xFF) + 1;
    
    /* Update ESP */
    TrapFrame->HardwareEsp = (USHORT)Esp;
    
    /* Get flat EIP */
    Eip = (TrapFrame->SegCs << 4) + TrapFrame->Eip;
    
    /* Now get the *next* EIP address (current is original + the count - 1) */
    Eip += (Flags & 0xFF);
    
    /* Now read the interrupt number */
    Interrupt = *(PUCHAR)Eip;
    
    /* Read the EIP from its IVT entry */
    Interrupt = *(PULONG)(Interrupt * 4);
    TrapFrame->Eip = (USHORT)Interrupt;
    
    /* Now get the CS segment */
    Interrupt = (USHORT)(Interrupt >> 16);
    
    /* Check if the trap was not V8086 trap */
    if (!(TrapFrame->EFlags & EFLAGS_V86_MASK))
    {
        /* Was it a kernel CS? */
        Interrupt |= RPL_MASK;
        if (TrapFrame->SegCs == KGDT_R0_CODE)
        {
            /* Add the RPL mask */
            TrapFrame->SegCs = Interrupt;
        }
        else
        {
            /* Set user CS */
            TrapFrame->SegCs = KGDT_R3_CODE | RPL_MASK;
        }
    }
    else
    {
        /* Set IVT CS */
        TrapFrame->SegCs = Interrupt;
    }
    
    /* We're done */
    return TRUE;
}

BOOLEAN
FASTCALL
KiVdmOpcodeIRET(IN PKTRAP_FRAME TrapFrame,
                IN ULONG Flags)
{
    ULONG Esp, V86EFlags, EFlags, TrapEFlags, Eip;
    
    /* Build flat ESP */
    Esp = (TrapFrame->HardwareSegSs << 4) + TrapFrame->HardwareEsp;
    
    /* Check for OPER32 */
    if (Flags & PFX_FLAG_OPER32)
    {
        /* Build segmented EIP */
        TrapFrame->Eip = *(PULONG)Esp;
        TrapFrame->SegCs = *(PUSHORT)(Esp + 4);
        
        /* Set new ESP */
        TrapFrame->HardwareEsp += 12;
        
        /* Get EFLAGS */
        EFlags = *(PULONG)(Esp + 8);
    }
    else
    {
        /* Build segmented EIP */
        TrapFrame->Eip = *(PUSHORT)Esp;
        TrapFrame->SegCs = *(PUSHORT)(Esp + 2);

        /* Set new ESP */
        TrapFrame->HardwareEsp += 6;

        /* Get EFLAGS */
        EFlags = *(PUSHORT)(Esp + 4);
    }
    
    /* Mask out EFlags */
    EFlags &= ~(EFLAGS_IOPL + EFLAGS_VIF + EFLAGS_NESTED_TASK + EFLAGS_VIP);
    V86EFlags = EFlags;
    
    /* FIXME: Check for VME support */
    
    /* Add V86 and Interrupt flag */
    EFlags |= EFLAGS_V86_MASK | EFLAGS_INTERRUPT_MASK;
    
    /* Update EFlags in trap frame */
    TrapEFlags = TrapFrame->EFlags;
    TrapFrame->EFlags = (TrapFrame->EFlags & EFLAGS_VIP) | EFlags;
    
    /* Check if ESP0 needs to be fixed up */
    if (!(TrapEFlags & EFLAGS_V86_MASK)) Ki386AdjustEsp0(TrapFrame);
    
    /* Update the V8086 EFlags state */
    KiVdmClearVdmEFlags(EFLAGS_INTERRUPT_MASK);
    KiVdmSetVdmEFlags(V86EFlags);
    
    /* Build flat EIP and check if this is the BOP instruction */
    Eip = (TrapFrame->SegCs << 4) + TrapFrame->Eip;
    if (*(PUSHORT)Eip == 0xC4C4)
    {
        /* Dispatch the BOP */
        VdmDispatchBop(TrapFrame);
    }
    else
    {
        /* FIXME: Check for VDM interrupts */
    }
    
    /* We're done */
    return TRUE;
}

BOOLEAN
FASTCALL
KiVdmOpcodeCLI(IN PKTRAP_FRAME TrapFrame,
               IN ULONG Flags)
{       
    /* FIXME: Support VME */

    /* disable interrupts */
    KiVdmClearVdmEFlags(EFLAGS_INTERRUPT_MASK);
    
    /* Skip instruction */
    TrapFrame->Eip += (Flags & 0xFF);
    
    /* Done */
    return TRUE;
}

BOOLEAN
FASTCALL
KiVdmOpcodeSTI(IN PKTRAP_FRAME TrapFrame,
               IN ULONG Flags)
{
    /* FIXME: Support VME */

    /* Enable interrupts */
    KiVdmSetVdmEFlags(EFLAGS_INTERRUPT_MASK);
    
    /* Skip instruction */
    TrapFrame->Eip += (Flags & 0xFF);
    
    /* Done */
    return TRUE;
}

/* MASTER OPCODE HANDLER ******************************************************/

BOOLEAN
FASTCALL
KiVdmHandleOpcode(IN PKTRAP_FRAME TrapFrame,
                  IN ULONG Flags)
{
    ULONG Eip;
    
    /* Get flat EIP of the *current* instruction (not the original EIP) */
    Eip = (TrapFrame->SegCs << 4) + TrapFrame->Eip;
    Eip += (Flags & 0xFF) - 1;
    
    /* Read the opcode entry */
    switch (*(PUCHAR)Eip)
    {
        case 0xF:               return KiCallVdmHandler(F);
        case 0x26:              return KiCallVdmPrefixHandler(PFX_FLAG_ES);
        case 0x2E:              return KiCallVdmPrefixHandler(PFX_FLAG_CS);
        case 0x36:              return KiCallVdmPrefixHandler(PFX_FLAG_SS);
        case 0x3E:              return KiCallVdmPrefixHandler(PFX_FLAG_DS);
        case 0x64:              return KiCallVdmPrefixHandler(PFX_FLAG_FS);
        case 0x65:              return KiCallVdmPrefixHandler(PFX_FLAG_GS);
        case 0x66:              return KiCallVdmPrefixHandler(PFX_FLAG_OPER32);
        case 0x67:              return KiCallVdmPrefixHandler(PFX_FLAG_ADDR32);
        case 0xF0:              return KiCallVdmPrefixHandler(PFX_FLAG_LOCK);
        case 0xF2:              return KiCallVdmPrefixHandler(PFX_FLAG_REPNE);
        case 0xF3:              return KiCallVdmPrefixHandler(PFX_FLAG_REP);
        case 0x6C:              return KiCallVdmHandler(INSB);
        case 0x6D:              return KiCallVdmHandler(INSW);
        case 0x6E:              return KiCallVdmHandler(OUTSB);
        case 0x6F:              return KiCallVdmHandler(OUTSW);
        case 0x98:              return KiCallVdmHandler(NPX);
        case 0xD8:              return KiCallVdmHandler(NPX);
        case 0xD9:              return KiCallVdmHandler(NPX);
        case 0xDA:              return KiCallVdmHandler(NPX);
        case 0xDB:              return KiCallVdmHandler(NPX);
        case 0xDC:              return KiCallVdmHandler(NPX);
        case 0xDD:              return KiCallVdmHandler(NPX);
        case 0xDE:              return KiCallVdmHandler(NPX);
        case 0xDF:              return KiCallVdmHandler(NPX);
        case 0x9C:              return KiCallVdmHandler(PUSHF);
        case 0x9D:              return KiCallVdmHandler(POPF);
        case 0xCD:              return KiCallVdmHandler(INTnn);
        case 0xCE:              return KiCallVdmHandler(INTO);
        case 0xCF:              return KiCallVdmHandler(IRET);   
        case 0xE4:              return KiCallVdmHandler(INBimm);   
        case 0xE5:              return KiCallVdmHandler(INWimm);
        case 0xE6:              return KiCallVdmHandler(OUTBimm);
        case 0xE7:              return KiCallVdmHandler(OUTWimm);        
        case 0xEC:              return KiCallVdmHandler(INB);
        case 0xED:              return KiCallVdmHandler(INW);
        case 0xEE:              return KiCallVdmHandler(OUTB);
        case 0xEF:              return KiCallVdmHandler(OUTW);
        case 0xF4:              return KiCallVdmHandler(HLT);
        case 0xFA:              return KiCallVdmHandler(CLI);
        case 0xFB:              return KiCallVdmHandler(STI);
        default:                return KiCallVdmHandler(INV);
    }    
}

/* PREFIX HANDLER *************************************************************/

BOOLEAN
FASTCALL
KiVdmOpcodePrefix(IN PKTRAP_FRAME TrapFrame,
                  IN ULONG Flags)
{
    /* Increase instruction size */
    Flags++;
    
    /* Handle the next opcode */
    return KiVdmHandleOpcode(TrapFrame, Flags);
}

/* TRAP HANDLER ***************************************************************/

BOOLEAN
FASTCALL
Ki386HandleOpcodeV86(IN PKTRAP_FRAME TrapFrame)
{
    /* Clean up */
    TrapFrame->Eip &= 0xFFFF;
    TrapFrame->HardwareEsp &= 0xFFFF;

    /* We start with only 1 byte per instruction */
    return KiVdmHandleOpcode(TrapFrame, 1);
}

ULONG_PTR
FASTCALL
KiExitV86Mode(IN PKTRAP_FRAME TrapFrame)
{
    PKV8086_STACK_FRAME StackFrame;
    PKGDTENTRY GdtEntry;
    PKTHREAD Thread;
    PKTRAP_FRAME PmTrapFrame;
    PKV86_FRAME V86Frame;
    PFX_SAVE_AREA NpxFrame;
    
    /* Get the stack frame back */
    StackFrame = CONTAINING_RECORD(TrapFrame->Esi, KV8086_STACK_FRAME, V86Frame);
    PmTrapFrame = &StackFrame->TrapFrame;
    V86Frame = &StackFrame->V86Frame;
    NpxFrame = &StackFrame->NpxArea;
    
    /* Copy the FPU frame back */
    Thread = KeGetCurrentThread();
    RtlCopyMemory(KiGetThreadNpxArea(Thread), NpxFrame, sizeof(FX_SAVE_AREA));

    /* Set initial stack back */
    Thread->InitialStack = (PVOID)((ULONG_PTR)V86Frame->ThreadStack + sizeof(FX_SAVE_AREA));
    
    /* Set ESP0 back in the KTSS */
    KeGetPcr()->TSS->Esp0 = (ULONG_PTR)&PmTrapFrame->V86Es;

    /* Restore TEB addresses */
    Thread->Teb = V86Frame->ThreadTeb;
    KeGetPcr()->Tib.Self = V86Frame->PcrTeb;
    
    /* Setup real TEB descriptor */
    GdtEntry = &((PKIPCR)KeGetPcr())->GDT[KGDT_R3_TEB / sizeof(KGDTENTRY)];
    GdtEntry->BaseLow = (USHORT)((ULONG_PTR)Thread->Teb & 0xFFFF);
    GdtEntry->HighWord.Bytes.BaseMid = (UCHAR)((ULONG_PTR)Thread->Teb >> 16);
    GdtEntry->HighWord.Bytes.BaseHi = (UCHAR)((ULONG_PTR)Thread->Teb >> 24);

    /* Enable interrupts and pop back non-volatiles */
    _enable();
    return TrapFrame->Edi;
}

VOID
FASTCALL
KiEnterV86Mode(VOID)
{
    PKTHREAD Thread;
    PKGDTENTRY GdtEntry;
    KV8086_STACK_FRAME StackFrameBuffer;
    PKV8086_STACK_FRAME StackFrame = &StackFrameBuffer;
    PKTRAP_FRAME TrapFrame = &StackFrame->TrapFrame;
    PKV86_FRAME V86Frame = &StackFrame->V86Frame;
    PFX_SAVE_AREA NpxFrame = &StackFrame->NpxArea;

    /* Build fake user-mode trap frame */
    TrapFrame->SegCs = KGDT_R0_CODE | RPL_MASK;
    TrapFrame->SegEs = TrapFrame->SegDs = TrapFrame->SegFs = TrapFrame->SegGs = 0;
    TrapFrame->ErrCode = 0;
    
    /* Get the current thread's initial stack */
    Thread = KeGetCurrentThread();
    V86Frame->ThreadStack = KiGetThreadNpxArea(Thread);
    
    /* Save TEB addresses */
    V86Frame->ThreadTeb = Thread->Teb;
    V86Frame->PcrTeb = KeGetPcr()->Tib.Self;
    
    /* Save return EIP */
    TrapFrame->Eip = (ULONG_PTR)Ki386BiosCallReturnAddress;
    
    /* Save our stack (after the frames) */
    TrapFrame->Esi = (ULONG_PTR)V86Frame;
    TrapFrame->Edi = (ULONG_PTR)_AddressOfReturnAddress() + 4;
    
    /* Sanitize EFlags and enable interrupts */
    TrapFrame->EFlags = __readeflags() & 0x60DD7;
    TrapFrame->EFlags |= EFLAGS_INTERRUPT_MASK;
    
    /* Fill out the rest of the frame */
    TrapFrame->HardwareSegSs = KGDT_R3_DATA | RPL_MASK;
    TrapFrame->HardwareEsp = 0x11FFE;
    TrapFrame->ExceptionList = EXCEPTION_CHAIN_END;
    TrapFrame->Dr7 = 0;
    //TrapFrame->DbgArgMark = 0xBADB0D00;
    TrapFrame->PreviousPreviousMode = -1;
    
    /* Disable interrupts */
    _disable();
    
    /* Copy the thread's NPX frame */
    RtlCopyMemory(NpxFrame, V86Frame->ThreadStack, sizeof(FX_SAVE_AREA));
    
    /* Clear exception list */
    KeGetPcr()->Tib.ExceptionList = EXCEPTION_CHAIN_END;
    
    /* Set new ESP0 */
    KeGetPcr()->TSS->Esp0 = (ULONG_PTR)&TrapFrame->V86Es;
                             
    /* Set new initial stack */
    Thread->InitialStack = V86Frame;
        
    /* Set VDM TEB */
    Thread->Teb = (PTEB)TRAMPOLINE_TEB;
    KeGetPcr()->Tib.Self = (PVOID)TRAMPOLINE_TEB;
    
    /* Setup VDM TEB descriptor */
    GdtEntry = &((PKIPCR)KeGetPcr())->GDT[KGDT_R3_TEB / sizeof(KGDTENTRY)];
    GdtEntry->BaseLow = (USHORT)((ULONG_PTR)TRAMPOLINE_TEB & 0xFFFF);
    GdtEntry->HighWord.Bytes.BaseMid = (UCHAR)((ULONG_PTR)TRAMPOLINE_TEB >> 16);
    GdtEntry->HighWord.Bytes.BaseHi = (UCHAR)((ULONG_PTR)TRAMPOLINE_TEB >> 24);
    
    /* Enable interrupts */
    _enable();
 
    /* Start VDM execution */
    NtVdmControl(VdmStartExecution, NULL);
    
    /* Exit to V86 mode */
    KiEoiHelper(TrapFrame);
}
 
/* PUBLIC FUNCTIONS ***********************************************************/

/*
 * @implemented
 */
NTSTATUS
NTAPI
Ke386CallBios(IN ULONG Int,
              OUT PCONTEXT Context)
{
    PUCHAR Trampoline = (PUCHAR)TRAMPOLINE_BASE;
    PTEB VdmTeb = (PTEB)TRAMPOLINE_TEB;
    PVDM_TIB VdmTib = (PVDM_TIB)TRAMPOLINE_TIB;
    ULONG ContextSize = FIELD_OFFSET(CONTEXT, ExtendedRegisters);
    PKTHREAD Thread = KeGetCurrentThread();
    PKTSS Tss = KeGetPcr()->TSS;
    PKPROCESS Process = Thread->ApcState.Process;
    PVDM_PROCESS_OBJECTS VdmProcessObjects;
    USHORT OldOffset, OldBase;

    /* Start with a clean TEB */
    RtlZeroMemory(VdmTeb, sizeof(TEB));

    /* Write the interrupt and bop */
    *Trampoline++ = 0xCD;
    *Trampoline++ = (UCHAR)Int;
    *(PULONG)Trampoline = TRAMPOLINE_BOP;

    /* Setup the VDM TEB and TIB */
    VdmTeb->Vdm = (PVOID)TRAMPOLINE_TIB;
    RtlZeroMemory(VdmTib, sizeof(VDM_TIB));
    VdmTib->Size = sizeof(VDM_TIB);

    /* Set a blank VDM state */
    *VdmState = 0;

    /* Copy the context */
    RtlCopyMemory(&VdmTib->VdmContext, Context, ContextSize);
    VdmTib->VdmContext.SegCs = (ULONG_PTR)Trampoline >> 4;
    VdmTib->VdmContext.SegSs = (ULONG_PTR)Trampoline >> 4;
    VdmTib->VdmContext.Eip = 0;
    VdmTib->VdmContext.Esp = 2 * PAGE_SIZE - sizeof(ULONG_PTR);
    VdmTib->VdmContext.EFlags |= EFLAGS_V86_MASK | EFLAGS_INTERRUPT_MASK;
    VdmTib->VdmContext.ContextFlags = CONTEXT_FULL;

    /* This can't be a real VDM process */
    ASSERT(PsGetCurrentProcess()->VdmObjects == NULL);

    /* Allocate VDM structure */
    VdmProcessObjects = ExAllocatePoolWithTag(NonPagedPool,
                                              sizeof(VDM_PROCESS_OBJECTS),
                                              '  eK');
    if (!VdmProcessObjects) return STATUS_NO_MEMORY;

    /* Set it up */
    RtlZeroMemory(VdmProcessObjects, sizeof(VDM_PROCESS_OBJECTS));
    VdmProcessObjects->VdmTib = VdmTib;
    PsGetCurrentProcess()->VdmObjects = VdmProcessObjects;

    /* Set the system affinity for the current thread */
    KeSetSystemAffinityThread(1);

    /* Make sure there's space for two IOPMs, then copy & clear the current */
    ASSERT(((PKIPCR)KeGetPcr())->GDT[KGDT_TSS / 8].LimitLow >=
            (0x2000 + IOPM_OFFSET - 1));
    RtlCopyMemory(Ki386IopmSaveArea, &Tss->IoMaps[0].IoMap, PAGE_SIZE * 2);
    RtlZeroMemory(&Tss->IoMaps[0].IoMap, PAGE_SIZE * 2);

    /* Save the old offset and base, and set the new ones */
    OldOffset = Process->IopmOffset;
    OldBase = Tss->IoMapBase;
    Process->IopmOffset = (USHORT)IOPM_OFFSET;
    Tss->IoMapBase = (USHORT)IOPM_OFFSET;

    /* Switch stacks and work the magic */
    Ki386SetupAndExitToV86Mode(VdmTeb);

    /* Restore IOPM */
    RtlCopyMemory(&Tss->IoMaps[0].IoMap, Ki386IopmSaveArea, PAGE_SIZE * 2);
    Process->IopmOffset = OldOffset;
    Tss->IoMapBase = OldBase;

    /* Restore affinity */
    KeRevertToUserAffinityThread();

    /* Restore context */
    RtlCopyMemory(Context, &VdmTib->VdmContext, ContextSize);
    Context->ContextFlags = CONTEXT_FULL;

    /* Free VDM objects */
    ExFreePool(PsGetCurrentProcess()->VdmObjects);
    PsGetCurrentProcess()->VdmObjects = NULL;

    /* Return status */
    return STATUS_SUCCESS;
}

/*
 * @unimplemented
 */
BOOLEAN
NTAPI
Ke386IoSetAccessProcess(IN PKPROCESS Process,
                        IN ULONG Flag)
{
    UNIMPLEMENTED;
    return FALSE;
}

/*
 * @unimplemented
 */
BOOLEAN
NTAPI
Ke386SetIoAccessMap(IN ULONG Flag,
                    IN PVOID IopmBuffer)
{
    UNIMPLEMENTED;
    return FALSE;
}

/*
 * @unimplemented
 */
BOOLEAN
NTAPI
Ke386QueryIoAccessMap(IN ULONG Flag,
                      IN PVOID IopmBuffer)
{
    UNIMPLEMENTED;
    return FALSE;
}
