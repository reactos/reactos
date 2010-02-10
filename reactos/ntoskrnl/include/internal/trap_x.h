/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            ntoskrnl/include/trap_x.h
 * PURPOSE:         Internal Inlined Functions for the Trap Handling Code
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */
#ifndef _TRAP_X_
#define _TRAP_X_

//
// Unreachable code hint for GCC 4.5.x, older GCC versions, and MSVC
//
#ifdef __GNUC__
#if __GNUC__ * 100 + __GNUC_MINOR__ >= 405
#define UNREACHABLE __builtin_unreachable()
#else
#define UNREACHABLE __builtin_trap()
#endif
#elif _MSC_VER
#define UNREACHABLE __assume(0)
#else
#define UNREACHABLE
#endif

//
// Debug Macros
//
VOID
FORCEINLINE
KiDumpTrapFrame(IN PKTRAP_FRAME TrapFrame)
{
    /* Dump the whole thing */
    DbgPrint("DbgEbp: %x\n", TrapFrame->DbgEbp);
    DbgPrint("DbgEip: %x\n", TrapFrame->DbgEip);
    DbgPrint("DbgArgMark: %x\n", TrapFrame->DbgArgMark);
    DbgPrint("DbgArgPointer: %x\n", TrapFrame->DbgArgPointer);
    DbgPrint("TempSegCs: %x\n", TrapFrame->TempSegCs);
    DbgPrint("TempEsp: %x\n", TrapFrame->TempEsp);
    DbgPrint("Dr0: %x\n", TrapFrame->Dr0);
    DbgPrint("Dr1: %x\n", TrapFrame->Dr1);
    DbgPrint("Dr2: %x\n", TrapFrame->Dr2);
    DbgPrint("Dr3: %x\n", TrapFrame->Dr3);
    DbgPrint("Dr6: %x\n", TrapFrame->Dr6);
    DbgPrint("Dr7: %x\n", TrapFrame->Dr7);
    DbgPrint("SegGs: %x\n", TrapFrame->SegGs);
    DbgPrint("SegEs: %x\n", TrapFrame->SegEs);
    DbgPrint("SegDs: %x\n", TrapFrame->SegDs);
    DbgPrint("Edx: %x\n", TrapFrame->Edx);
    DbgPrint("Ecx: %x\n", TrapFrame->Ecx);
    DbgPrint("Eax: %x\n", TrapFrame->Eax);
    DbgPrint("PreviousPreviousMode: %x\n", TrapFrame->PreviousPreviousMode);
    DbgPrint("ExceptionList: %x\n", TrapFrame->ExceptionList);
    DbgPrint("SegFs: %x\n", TrapFrame->SegFs);
    DbgPrint("Edi: %x\n", TrapFrame->Edi);
    DbgPrint("Esi: %x\n", TrapFrame->Esi);
    DbgPrint("Ebx: %x\n", TrapFrame->Ebx);
    DbgPrint("Ebp: %x\n", TrapFrame->Ebp);
    DbgPrint("ErrCode: %x\n", TrapFrame->ErrCode);
    DbgPrint("Eip: %x\n", TrapFrame->Eip);
    DbgPrint("SegCs: %x\n", TrapFrame->SegCs);
    DbgPrint("EFlags: %x\n", TrapFrame->EFlags);
    DbgPrint("HardwareEsp: %x\n", TrapFrame->HardwareEsp);
    DbgPrint("HardwareSegSs: %x\n", TrapFrame->HardwareSegSs);
    DbgPrint("V86Es: %x\n", TrapFrame->V86Es);
    DbgPrint("V86Ds: %x\n", TrapFrame->V86Ds);
    DbgPrint("V86Fs: %x\n", TrapFrame->V86Fs);
    DbgPrint("V86Gs: %x\n", TrapFrame->V86Gs);
}

#ifdef TRAP_DEBUG
VOID
FORCEINLINE
KiFillTrapFrameDebug(IN PKTRAP_FRAME TrapFrame)
{
    /* Set the debug information */
    TrapFrame->DbgArgPointer = TrapFrame->Edx;
    TrapFrame->DbgArgMark = 0xBADB0D00;
    TrapFrame->DbgEip = TrapFrame->Eip;
    TrapFrame->DbgEbp = TrapFrame->Ebp;   
}

VOID
FORCEINLINE
KiExitTrapDebugChecks(IN PKTRAP_FRAME TrapFrame,
                      IN KTRAP_STATE_BITS SkipBits)
{
    /* Make sure interrupts are disabled */
    if (__readeflags() & EFLAGS_INTERRUPT_MASK)
    {
        DbgPrint("Exiting with interrupts enabled: %lx\n", __readeflags());
        while (TRUE);
    }
    
    /* Make sure this is a real trap frame */
    if (TrapFrame->DbgArgMark != 0xBADB0D00)
    {
        DbgPrint("Exiting with an invalid trap frame? (No MAGIC in trap frame)\n");
        KiDumpTrapFrame(TrapFrame);
        while (TRUE);
    }
    
    /* Make sure we're not in user-mode or something */
    if (Ke386GetFs() != KGDT_R0_PCR)
    {
        DbgPrint("Exiting with an invalid FS: %lx\n", Ke386GetFs());
        while (TRUE);   
    }
    
    /* Make sure we have a valid SEH chain */
    if (KeGetPcr()->Tib.ExceptionList == 0)
    {
        DbgPrint("Exiting with NULL exception chain: %p\n", KeGetPcr()->Tib.ExceptionList);
        while (TRUE);
    }
    
    /* Make sure we're restoring a valid SEH chain */
    if (TrapFrame->ExceptionList == 0)
    {
        DbgPrint("Entered a trap with a NULL exception chain: %p\n", TrapFrame->ExceptionList);
        while (TRUE);
    }
    
    /* If we're ignoring previous mode, make sure caller doesn't actually want it */
    if ((SkipBits.SkipPreviousMode) && (TrapFrame->PreviousPreviousMode != -1))
    {
        DbgPrint("Exiting a trap witout restoring previous mode, yet previous mode seems valid: %lx", TrapFrame->PreviousPreviousMode);
        while (TRUE);
    }
}

VOID
FORCEINLINE
KiExitSystemCallDebugChecks(IN ULONG SystemCall,
                            IN PKTRAP_FRAME TrapFrame)
{
    KIRQL OldIrql;
    
    /* Check if this was a user call */
    if (KiUserMode(TrapFrame))
    {
        /* Make sure we are not returning with elevated IRQL */
        OldIrql = KeGetCurrentIrql();
        if (OldIrql != PASSIVE_LEVEL)
        {
            /* Forcibly put us in a sane state */
            KeGetPcr()->CurrentIrql = PASSIVE_LEVEL;
            _disable();
            
            /* Fail */
            KeBugCheckEx(IRQL_GT_ZERO_AT_SYSTEM_SERVICE,
                         SystemCall,
                         OldIrql,
                         0,
                         0);
        }
        
        /* Make sure we're not attached and that APCs are not disabled */
        if ((KeGetCurrentThread()->ApcStateIndex != CurrentApcEnvironment) ||
            (KeGetCurrentThread()->CombinedApcDisable != 0))
        {
            /* Fail */
            KeBugCheckEx(APC_INDEX_MISMATCH,
                         SystemCall,
                         KeGetCurrentThread()->ApcStateIndex,
                         KeGetCurrentThread()->CombinedApcDisable,
                         0);
        }
    }
}
#else
#define KiExitTrapDebugChecks(x, y)
#define KiFillTrapFrameDebug(x)
#define KiExitSystemCallDebugChecks(x, y)
#endif

//
// Helper Code
//
BOOLEAN
FORCEINLINE
KiUserTrap(IN PKTRAP_FRAME TrapFrame)
{
    /* Anything else but Ring 0 is Ring 3 */
    return (TrapFrame->SegCs & MODE_MASK);
}

//
// "BOP" code used by VDM and V8086 Mode
//
VOID
FORCEINLINE
KiIssueBop(VOID)
{
    /* Invalid instruction that an invalid opcode handler must trap and handle */
    asm volatile(".byte 0xC4\n.byte 0xC4\n");
}

VOID
FORCEINLINE
KiUserSystemCall(IN PKTRAP_FRAME TrapFrame)
{
    /*
     * Kernel call or user call?
     *
     * This decision is made in inlined assembly because we need to patch
     * the relative offset of the user-mode jump to point to the SYSEXIT
     * routine if the CPU supports it. The only way to guarantee that a
     * relative jnz/jz instruction is generated is to force it with the
     * inline assembler.
     */
    asm volatile
    (
        "test $1, %0\n" /* MODE_MASK */
        ".globl _KiSystemCallExitBranch\n_KiSystemCallExitBranch:\n"
        "jnz _KiSystemCallExit\n"
        :
        : "r"(TrapFrame->SegCs)
    );
}

//
// Generates an Exit Epilog Stub for the given name
//
#define KI_FUNCTION_CALL            0x1
#define KI_EDITED_FRAME             0x2
#define KI_DIRECT_EXIT              0x4
#define KI_FAST_SYSTEM_CALL_EXIT    0x8
#define KI_SYSTEM_CALL_EXIT         0x10
#define KI_SYSTEM_CALL_JUMP         0x20
#define KiTrapExitStub(x, y)        VOID FORCEINLINE DECLSPEC_NORETURN x(IN PKTRAP_FRAME TrapFrame) { KiTrapExit(TrapFrame, y); UNREACHABLE; }
#define KiTrapExitStub2(x, y)       VOID FORCEINLINE x(IN PKTRAP_FRAME TrapFrame) { KiTrapExit(TrapFrame, y); }

//
// How volatiles will be restored
//
#define KI_EAX_NO_VOLATILES         0x0
#define KI_EAX_ONLY                 0x1
#define KI_ALL_VOLATILES            0x2

//
// Exit mechanism to use
//
#define KI_EXIT_IRET                0x0
#define KI_EXIT_SYSEXIT             0x1
#define KI_EXIT_JMP                 0x2
#define KI_EXIT_RET                 0x3

//
// Master Trap Epilog
//
VOID
FORCEINLINE
KiTrapExit(IN PKTRAP_FRAME TrapFrame,
          IN ULONG Flags)
{
    ULONG FrameSize = FIELD_OFFSET(KTRAP_FRAME, Eip);
    ULONG ExitMechanism = KI_EXIT_IRET, Volatiles = KI_ALL_VOLATILES, NonVolatiles = TRUE;
    ULONG EcxField = FIELD_OFFSET(KTRAP_FRAME, Ecx), EdxField = FIELD_OFFSET(KTRAP_FRAME, Edx);
    
    /* System call exit needs a special label */
    if (Flags & KI_SYSTEM_CALL_EXIT) __asm__ __volatile__
    (
        ".globl _KiSystemCallExit\n_KiSystemCallExit:\n"
    );
            
    /* Start by making the trap frame equal to the stack */
    __asm__ __volatile__
    (
        "movl %0, %%esp\n"
        :
        : "r"(TrapFrame)
        : "%esp"
    );
        
    /* Check what kind of trap frame this trap requires */
    if (Flags & KI_FUNCTION_CALL)
    {
        /* These calls have an EIP on the stack they need */
        ExitMechanism = KI_EXIT_RET;
        Volatiles = FALSE;
    }
    else if (Flags & KI_EDITED_FRAME)
    {
        /* Edited frames store a new ESP in the error code field */
        FrameSize = FIELD_OFFSET(KTRAP_FRAME, ErrCode);
    }
    else if (Flags & KI_DIRECT_EXIT)
    {
        /* Exits directly without restoring anything, interrupt frame on stack */
        NonVolatiles = Volatiles = FALSE;
    }
    else if (Flags & KI_FAST_SYSTEM_CALL_EXIT)
    {
        /* We have a fake interrupt stack with a ring transition */
        FrameSize = FIELD_OFFSET(KTRAP_FRAME, V86Es);
        ExitMechanism = KI_EXIT_SYSEXIT;
        
        /* SYSEXIT wants EIP in EDX and ESP in ECX */
        EcxField = FIELD_OFFSET(KTRAP_FRAME, HardwareEsp);
        EdxField = FIELD_OFFSET(KTRAP_FRAME, Eip);
    }
    else if (Flags & KI_SYSTEM_CALL_EXIT)
    {
        /* Only restore EAX */
        NonVolatiles = KI_EAX_ONLY;
    }
    else if (Flags & KI_SYSTEM_CALL_JUMP)
    {
        /* We have a fake interrupt stack with no ring transition */
        FrameSize = FIELD_OFFSET(KTRAP_FRAME, HardwareEsp);
        NonVolatiles = KI_EAX_ONLY;
        ExitMechanism = KI_EXIT_JMP;
    }
    
    /* Restore the non volatiles */
    if (NonVolatiles) __asm__ __volatile__
    (
        "movl %c[b](%%esp), %%ebx\n"
        "movl %c[s](%%esp), %%esi\n"
        "movl %c[i](%%esp), %%edi\n"
        "movl %c[p](%%esp), %%ebp\n"
        :
        : [b] "i"(FIELD_OFFSET(KTRAP_FRAME, Ebx)),
          [s] "i"(FIELD_OFFSET(KTRAP_FRAME, Esi)),
          [i] "i"(FIELD_OFFSET(KTRAP_FRAME, Edi)),
          [p] "i"(FIELD_OFFSET(KTRAP_FRAME, Ebp))
        : "%esp"
    );
    
    /* Restore EAX if volatiles must be restored */
    if (Volatiles) __asm__ __volatile__
    (
        "movl %c[a](%%esp), %%eax\n":: [a] "i"(FIELD_OFFSET(KTRAP_FRAME, Eax)) : "%esp"
    );
    
    /* Restore the other volatiles if needed */
    if (Volatiles == KI_ALL_VOLATILES) __asm__ __volatile__
    (
        "movl %c[c](%%esp), %%ecx\n"
        "movl %c[d](%%esp), %%edx\n"
        :
        : [c] "i"(EcxField),
          [d] "i"(EdxField)
        : "%esp"
    );
    
    /* Ring 0 system calls jump back to EDX */
    if (Flags & KI_SYSTEM_CALL_JUMP) __asm__ __volatile__
    (
        "movl %c[d](%%esp), %%edx\n":: [d] "i"(FIELD_OFFSET(KTRAP_FRAME, Eip)) : "%esp"
    );

    /* Now destroy the trap frame on the stack */
    __asm__ __volatile__ ("addl $%c[e],%%esp\n":: [e] "i"(FrameSize) : "%esp");
    
    /* Edited traps need to change to a new ESP */
    if (Flags & KI_EDITED_FRAME) __asm__ __volatile__ ("movl (%%esp), %%esp\n":::"%esp");

    /* Check the exit mechanism and apply it */
    if (ExitMechanism == KI_EXIT_RET) __asm__ __volatile__("ret\n"::: "%esp");
    else if (ExitMechanism == KI_EXIT_IRET) __asm__ __volatile__("iret\n"::: "%esp");
    else if (ExitMechanism == KI_EXIT_JMP) __asm__ __volatile__("jmp *%%edx\n.globl _KiSystemCallExit2\n_KiSystemCallExit2:\n"::: "%esp");
    else if (ExitMechanism == KI_EXIT_SYSEXIT) __asm__ __volatile__("sti\nsysexit\n"::: "%esp");   
}

//
// All the specific trap epilog stubs
//
KiTrapExitStub (KiTrapReturn,              0);
KiTrapExitStub (KiDirectTrapReturn,        KI_DIRECT_EXIT);
KiTrapExitStub (KiCallReturn,              KI_FUNCTION_CALL);
KiTrapExitStub (KiEditedTrapReturn,        KI_EDITED_FRAME);
KiTrapExitStub2(KiSystemCallReturn,        KI_SYSTEM_CALL_JUMP);
KiTrapExitStub (KiSystemCallSysExitReturn, KI_FAST_SYSTEM_CALL_EXIT);
KiTrapExitStub (KiSystemCallTrapReturn,    KI_SYSTEM_CALL_EXIT);

//
// Generic Exit Routine
//
VOID
FORCEINLINE
DECLSPEC_NORETURN
KiExitTrap(IN PKTRAP_FRAME TrapFrame,
           IN UCHAR Skip)
{
    KTRAP_EXIT_SKIP_BITS SkipBits = { .Bits = Skip };
    PULONG ReturnStack;
    
    /* Debugging checks */
    KiExitTrapDebugChecks(TrapFrame, SkipBits);

    /* Restore the SEH handler chain */
    KeGetPcr()->Tib.ExceptionList = TrapFrame->ExceptionList;
    
    /* Check if the previous mode must be restored */
    if (__builtin_expect(!SkipBits.SkipPreviousMode, 0)) /* More INTS than SYSCALLs */
    {
        /* Restore it */
        KeGetCurrentThread()->PreviousMode = TrapFrame->PreviousPreviousMode;
    }

    /* Check if there are active debug registers */
    if (__builtin_expect(TrapFrame->Dr7 & ~DR7_RESERVED_MASK, 0))
    {
        /* Not handled yet */
        DbgPrint("Need Hardware Breakpoint Support!\n");
        DbgBreakPoint();
        while (TRUE);
    }
    
    /* Check if this was a V8086 trap */
    if (__builtin_expect(TrapFrame->EFlags & EFLAGS_V86_MASK, 0)) KiTrapReturn(TrapFrame);

    /* Check if the trap frame was edited */
    if (__builtin_expect(!(TrapFrame->SegCs & FRAME_EDITED), 0))
    {   
        /*
         * An edited trap frame happens when we need to modify CS and/or ESP but
         * don't actually have a ring transition. This happens when a kernelmode
         * caller wants to perform an NtContinue to another kernel address, such
         * as in the case of SEH (basically, a longjmp), or to a user address.
         *
         * Therefore, the CPU never saved CS/ESP on the stack because we did not
         * get a trap frame due to a ring transition (there was no interrupt).
         * Even if we didn't want to restore CS to a new value, a problem occurs
         * due to the fact a normal RET would not work if we restored ESP since
         * RET would then try to read the result off the stack.
         *
         * The NT kernel solves this by adding 12 bytes of stack to the exiting
         * trap frame, in which EFLAGS, CS, and EIP are stored, and then saving
         * the ESP that's being requested into the ErrorCode field. It will then
         * exit with an IRET. This fixes both issues, because it gives the stack
         * some space where to hold the return address and then end up with the
         * wanted stack, and it uses IRET which allows a new CS to be inputted.
         *
         */
         
        /* Set CS that is requested */
        TrapFrame->SegCs = TrapFrame->TempSegCs;
         
        /* First make space on requested stack */
        ReturnStack = (PULONG)(TrapFrame->TempEsp - 12);
        TrapFrame->ErrCode = (ULONG_PTR)ReturnStack;
         
        /* Now copy IRET frame */
        ReturnStack[0] = TrapFrame->Eip;
        ReturnStack[1] = TrapFrame->SegCs;
        ReturnStack[2] = TrapFrame->EFlags;
        
        /* Do special edited return */
        KiEditedTrapReturn(TrapFrame);
    }
    
    /* Check if this is a user trap */
    if (__builtin_expect(KiUserTrap(TrapFrame), 1)) /* Ring 3 is where we spend time */
    {
        /* Check if segments should be restored */
        if (!SkipBits.SkipSegments)
        {
            /* Restore segments */
            Ke386SetGs(TrapFrame->SegGs);
            Ke386SetEs(TrapFrame->SegEs);
            Ke386SetDs(TrapFrame->SegDs);
            Ke386SetFs(TrapFrame->SegFs);
        }
        
        /* Always restore FS since it goes from KPCR to TEB */
        Ke386SetFs(TrapFrame->SegFs);
    }
    
    /* Check for system call -- a system call skips volatiles! */
    if (__builtin_expect(SkipBits.SkipVolatiles, 0)) /* More INTs than SYSCALLs */
    {
        /* User or kernel call? */
        KiUserSystemCall(TrapFrame);
        
        /* Restore EFLags */
        __writeeflags(TrapFrame->EFlags);
            
        /* Call is kernel, so do a jump back since this wasn't a real INT */
        KiSystemCallReturn(TrapFrame);

        /* If we got here, this is SYSEXIT: are we stepping code? */
        if (!(TrapFrame->EFlags & EFLAGS_TF))
        {
            /* Restore user FS */
            Ke386SetFs(KGDT_R3_TEB | RPL_MASK);

            /* Remove interrupt flag */
            TrapFrame->EFlags &= ~EFLAGS_INTERRUPT_MASK;
            __writeeflags(TrapFrame->EFlags);

            /* Exit through SYSEXIT */
            KiSystemCallSysExitReturn(TrapFrame);
        }
        
        /* Exit through IRETD, either due to debugging or due to lack of SYSEXIT */
        KiSystemCallTrapReturn(TrapFrame);
    }
    
    /* Return from interrupt */
    KiTrapReturn(TrapFrame);
}

//
// Virtual 8086 Mode Optimized Trap Exit
//
VOID
FORCEINLINE
KiExitV86Trap(IN PKTRAP_FRAME TrapFrame)
{
    PKTHREAD Thread;
    KIRQL OldIrql;
    
    /* Get the thread */
    Thread = KeGetCurrentThread();
    while (TRUE)
    {
        /* Turn off the alerted state for kernel mode */
        Thread->Alerted[KernelMode] = FALSE;

        /* Are there pending user APCs? */
        if (__builtin_expect(!Thread->ApcState.UserApcPending, 1)) break;

        /* Raise to APC level and enable interrupts */
        OldIrql = KfRaiseIrql(APC_LEVEL);
        _enable();

        /* Deliver APCs */
        KiDeliverApc(UserMode, NULL, TrapFrame);

        /* Restore IRQL and disable interrupts once again */
        KfLowerIrql(OldIrql);
        _disable();
        
        /* Return if this isn't V86 mode anymore */
        if (__builtin_expect(TrapFrame->EFlags & EFLAGS_V86_MASK, 0)) return;
    }
     
    /* If we got here, we're still in a valid V8086 context, so quit it */
    if (__builtin_expect(TrapFrame->Dr7 & ~DR7_RESERVED_MASK, 0))
    {
        /* Not handled yet */
        DbgPrint("Need Hardware Breakpoint Support!\n");
        while (TRUE);
    }
     
    /* Return from interrupt */
    KiTrapReturn(TrapFrame);
}

//
// Virtual 8086 Mode Optimized Trap Entry
//
VOID
FORCEINLINE
KiEnterV86Trap(IN PKTRAP_FRAME TrapFrame)
{
    /* Save exception list */
    TrapFrame->ExceptionList = KeGetPcr()->Tib.ExceptionList;

    /* Save DR7 and check for debugging */
    TrapFrame->Dr7 = __readdr(7);
    if (__builtin_expect(TrapFrame->Dr7 & ~DR7_RESERVED_MASK, 0))
    {
        DbgPrint("Need Hardware Breakpoint Support!\n");
        while (TRUE);
    }
}

//
// Interrupt Trap Entry
//
VOID
FORCEINLINE
KiEnterInterruptTrap(IN PKTRAP_FRAME TrapFrame)
{
    /* Save exception list and terminate it */
    TrapFrame->ExceptionList = KeGetPcr()->Tib.ExceptionList;
    KeGetPcr()->Tib.ExceptionList = EXCEPTION_CHAIN_END;

    /* Flush DR7 and check for debugging */
    TrapFrame->Dr7 = 0;
    if (__builtin_expect(KeGetCurrentThread()->DispatcherHeader.DebugActive & 0xFF, 0))
    {
        DbgPrint("Need Hardware Breakpoint Support!\n");
        while (TRUE);
    }
    
    /* Set debug header */
    KiFillTrapFrameDebug(TrapFrame);
}

//
// Generic Trap Entry
//
VOID
FORCEINLINE
KiEnterTrap(IN PKTRAP_FRAME TrapFrame)
{
    /* Save exception list */
    TrapFrame->ExceptionList = KeGetPcr()->Tib.ExceptionList;
    
    /* Flush DR7 and check for debugging */
    TrapFrame->Dr7 = 0;
    if (__builtin_expect(KeGetCurrentThread()->DispatcherHeader.DebugActive & 0xFF, 0))
    {
        DbgPrint("Need Hardware Breakpoint Support!\n");
        while (TRUE);
    }
    
    /* Set debug header */
    KiFillTrapFrameDebug(TrapFrame);
}

#endif
