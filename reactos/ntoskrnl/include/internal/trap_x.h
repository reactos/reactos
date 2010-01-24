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
// Unreachable code hint for GCC 4.5.x, 4.4.x, and MSVC
//
#if __GNUC__ * 100 + __GNUC_MINOR__ >= 405
#define UNREACHABLE __builtin_unreachable()
#elif __GNUC__ * 100 + __GNUC_MINOR__ >= 404
#define UNREACHABLE __builtin_trap()
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
// Assembly exit stubs
//
VOID
FORCEINLINE
//DECLSPEC_NORETURN
KiSystemCallReturn(IN PKTRAP_FRAME TrapFrame)
{
    /* Restore nonvolatiles, EAX, and do a "jump" back to the kernel caller */
    __asm__ __volatile__
    (
        "movl %0, %%esp\n"
        "movl %c[b](%%esp), %%ebx\n"
        "movl %c[s](%%esp), %%esi\n"
        "movl %c[i](%%esp), %%edi\n"
        "movl %c[p](%%esp), %%ebp\n"
        "movl %c[a](%%esp), %%eax\n"
        "movl %c[e](%%esp), %%edx\n"
        "addl $%c[v],%%esp\n" /* A WHOLE *KERNEL* frame since we're not IRET'ing */
        "jmp *%%edx\n"
        :
        : "r"(TrapFrame),
          [b] "i"(KTRAP_FRAME_EBX),
          [s] "i"(KTRAP_FRAME_ESI),
          [i] "i"(KTRAP_FRAME_EDI),
          [p] "i"(KTRAP_FRAME_EBP),
          [a] "i"(KTRAP_FRAME_EAX),
          [e] "i"(KTRAP_FRAME_EIP),
          [v] "i"(KTRAP_FRAME_ESP)
        : "%esp"
    );
    UNREACHABLE;
}

VOID
FORCEINLINE
//DECLSPEC_NORETURN
KiSystemCallTrapReturn(IN PKTRAP_FRAME TrapFrame)
{
    /* Regular interrupt exit, but we only restore EAX as a volatile */
    __asm__ __volatile__
    (
        "movl %0, %%esp\n"
        "movl %c[b](%%esp), %%ebx\n"
        "movl %c[s](%%esp), %%esi\n"
        "movl %c[i](%%esp), %%edi\n"
        "movl %c[p](%%esp), %%ebp\n"
        "movl %c[a](%%esp), %%eax\n"
        "addl $%c[e],%%esp\n"
        "iret\n"
        :
        : "r"(TrapFrame),
          [b] "i"(KTRAP_FRAME_EBX),
          [s] "i"(KTRAP_FRAME_ESI),
          [i] "i"(KTRAP_FRAME_EDI),
          [p] "i"(KTRAP_FRAME_EBP),
          [a] "i"(KTRAP_FRAME_EAX),         
          [e] "i"(KTRAP_FRAME_EIP)
        : "%esp"
    );
    UNREACHABLE;
}

VOID
FORCEINLINE
//DECLSPEC_NORETURN
KiSystemCallSysExitReturn(IN PKTRAP_FRAME TrapFrame)
{
    /* Restore nonvolatiles, EAX, and do a SYSEXIT back to the user caller */
    __asm__ __volatile__
    (
        "movl %0, %%esp\n"
        "movl %c[s](%%esp), %%esi\n"
        "movl %c[b](%%esp), %%ebx\n"
        "movl %c[i](%%esp), %%edi\n"
        "movl %c[p](%%esp), %%ebp\n"
        "movl %c[a](%%esp), %%eax\n"
        "movl %c[e](%%esp), %%edx\n" /* SYSEXIT says EIP in EDX */
        "movl %c[x](%%esp), %%ecx\n" /* SYSEXIT says ESP in ECX */
        "addl $%c[v],%%esp\n" /* A WHOLE *USER* frame since we're not IRET'ing */
        "sti\nsysexit\n"
        :
        : "r"(TrapFrame),
          [b] "i"(KTRAP_FRAME_EBX),
          [s] "i"(KTRAP_FRAME_ESI),
          [i] "i"(KTRAP_FRAME_EDI),
          [p] "i"(KTRAP_FRAME_EBP),
          [a] "i"(KTRAP_FRAME_EAX),
          [e] "i"(KTRAP_FRAME_EIP),
          [x] "i"(KTRAP_FRAME_ESP),
          [v] "i"(KTRAP_FRAME_V86_ES)
        : "%esp"
    );
    UNREACHABLE;
}

VOID
FORCEINLINE
//DECLSPEC_NORETURN
KiTrapReturn(IN PKTRAP_FRAME TrapFrame)
{
    /* Regular interrupt exit */
    __asm__ __volatile__
    (
        "movl %0, %%esp\n"
        "movl %c[a](%%esp), %%eax\n"
        "movl %c[b](%%esp), %%ebx\n"
        "movl %c[c](%%esp), %%ecx\n"
        "movl %c[d](%%esp), %%edx\n"
        "movl %c[s](%%esp), %%esi\n"
        "movl %c[i](%%esp), %%edi\n"
        "movl %c[p](%%esp), %%ebp\n"
        "addl $%c[e],%%esp\n"
        "iret\n"
        :
        : "r"(TrapFrame),
          [a] "i"(KTRAP_FRAME_EAX),
          [b] "i"(KTRAP_FRAME_EBX),
          [c] "i"(KTRAP_FRAME_ECX),
          [d] "i"(KTRAP_FRAME_EDX),
          [s] "i"(KTRAP_FRAME_ESI),
          [i] "i"(KTRAP_FRAME_EDI),
          [p] "i"(KTRAP_FRAME_EBP),
          [e] "i"(KTRAP_FRAME_EIP)
        : "%esp"
    );
    UNREACHABLE;
}

VOID
FORCEINLINE
//DECLSPEC_NORETURN
KiEditedTrapReturn(IN PKTRAP_FRAME TrapFrame)
{
    /* Regular interrupt exit */
    __asm__ __volatile__
    (
        "movl %0, %%esp\n"
        "movl %c[a](%%esp), %%eax\n"
        "movl %c[b](%%esp), %%ebx\n"
        "movl %c[c](%%esp), %%ecx\n"
        "movl %c[d](%%esp), %%edx\n"
        "movl %c[s](%%esp), %%esi\n"
        "movl %c[i](%%esp), %%edi\n"
        "movl %c[p](%%esp), %%ebp\n"
        "addl $%c[e],%%esp\n"
        "movl (%%esp), %%esp\n"
        "iret\n"
        :
        : "r"(TrapFrame),
          [a] "i"(KTRAP_FRAME_EAX),
          [b] "i"(KTRAP_FRAME_EBX),
          [c] "i"(KTRAP_FRAME_ECX),
          [d] "i"(KTRAP_FRAME_EDX),
          [s] "i"(KTRAP_FRAME_ESI),
          [i] "i"(KTRAP_FRAME_EDI),
          [p] "i"(KTRAP_FRAME_EBP),
          [e] "i"(KTRAP_FRAME_ERROR_CODE) /* We *WANT* the error code since ESP is there! */
        : "%esp"
    );
    UNREACHABLE;
}

//
// Generic Exit Routine
//
VOID
FORCEINLINE
//DECLSPEC_NORETURN
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
        /* Kernel call or user call? */
        if (__builtin_expect(KiUserTrap(TrapFrame), 1)) /* More Ring 3 than 0 */
        {
            /* Is SYSENTER supported and/or enabled, or are we stepping code? */
            if (__builtin_expect((KiFastSystemCallDisable) ||
                                 (TrapFrame->EFlags & EFLAGS_TF), 0))
            {
                /* Exit normally */
                KiSystemCallTrapReturn(TrapFrame);
            }
            else
            {
                /* Restore user FS */
                Ke386SetFs(KGDT_R3_TEB | RPL_MASK);
                
                /* Remove interrupt flag */
                TrapFrame->EFlags &= ~EFLAGS_INTERRUPT_MASK;
                __writeeflags(TrapFrame->EFlags);
                
                /* Exit through SYSEXIT */
                KiSystemCallSysExitReturn(TrapFrame);
            }
        }
        else
        {
            /* Restore EFLags */
            __writeeflags(TrapFrame->EFlags);
            
            /* Call is kernel, so do a jump back since this wasn't a real INT */
            KiSystemCallReturn(TrapFrame);
        }  
    }
    else
    {
        /* Return from interrupt */
        KiTrapReturn(TrapFrame);
    }
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
    /* Load correct registers */
    Ke386SetFs(KGDT_R0_PCR);
    Ke386SetDs(KGDT_R3_DATA | RPL_MASK);
    Ke386SetEs(KGDT_R3_DATA | RPL_MASK);

    /* Save exception list and bogus previous mode */
    TrapFrame->PreviousPreviousMode = -1;
    TrapFrame->ExceptionList = KeGetPcr()->Tib.ExceptionList;

    /* Clear direction flag */
    Ke386ClearDirectionFlag();
    
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
    /* Set bogus previous mode */
    TrapFrame->PreviousPreviousMode = -1;
    
    /* Check for V86 mode */
    if (__builtin_expect(TrapFrame->EFlags & EFLAGS_V86_MASK, 0))
    {
        DbgPrint("Need V8086 Interrupt Support!\n");
        while (TRUE);
    }
    
    /* Check if this wasn't kernel code */
    if (__builtin_expect(TrapFrame->SegCs != KGDT_R0_CODE, 1)) /* Ring 3 is more common */
    {
        /* Save segments and then switch to correct ones */
        TrapFrame->SegFs = Ke386GetFs();
        TrapFrame->SegGs = Ke386GetGs();
        TrapFrame->SegDs = Ke386GetDs();
        TrapFrame->SegEs = Ke386GetEs();
        Ke386SetFs(KGDT_R0_PCR);
        Ke386SetDs(KGDT_R3_DATA | RPL_MASK);
        Ke386SetEs(KGDT_R3_DATA | RPL_MASK);
    }
    
    /* Save exception list and terminate it */
    TrapFrame->ExceptionList = KeGetPcr()->Tib.ExceptionList;
    KeGetPcr()->Tib.ExceptionList = EXCEPTION_CHAIN_END;
    
    /* No error code */
    TrapFrame->ErrCode = 0;
    
    /* Clear direction flag */
    Ke386ClearDirectionFlag();
    
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
    ULONG Ds, Es;
    
    /*
     * We really have to get a good DS/ES first before touching any data.
     *
     * These two reads will either go in a register (with optimizations ON) or
     * a stack variable (which is on SS:ESP, guaranteed to be good/valid).
     *
     * Because the assembly is marked volatile, the order of instructions is
     * as-is, otherwise the optimizer could simply get rid of our DS/ES.
     *
     */
    Ds = Ke386GetDs();
    Es = Ke386GetEs();
    Ke386SetDs(KGDT_R3_DATA | RPL_MASK);
    Ke386SetEs(KGDT_R3_DATA | RPL_MASK);
    TrapFrame->SegDs = Ds;
    TrapFrame->SegEs = Es;
        
    /* Now we can save the other segments and then switch to the correct FS */
    TrapFrame->SegFs = Ke386GetFs();
    TrapFrame->SegGs = Ke386GetGs();
    Ke386SetFs(KGDT_R0_PCR);

    /* Save exception list and bogus previous mode */
    TrapFrame->PreviousPreviousMode = -1;
    TrapFrame->ExceptionList = KeGetPcr()->Tib.ExceptionList;
    
    /* Check for V86 mode */
    if (__builtin_expect(TrapFrame->EFlags & EFLAGS_V86_MASK, 0))
    {
        /* Restore V8086 segments into Protected Mode segments */
        TrapFrame->SegFs = TrapFrame->V86Fs;
        TrapFrame->SegGs = TrapFrame->V86Gs;
        TrapFrame->SegDs = TrapFrame->V86Ds;
        TrapFrame->SegEs = TrapFrame->V86Es;
    }

    /* Clear direction flag */
    Ke386ClearDirectionFlag();
    
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
