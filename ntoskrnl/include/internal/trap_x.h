/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            ntoskrnl/include/trap_x.h
 * PURPOSE:         Internal Inlined Functions for the Trap Handling Code
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

//
// Debug Macros
//
VOID
FORCEINLINE
KiDumpTrapFrame(IN PKTRAP_FRAME TrapFrame)
{
    /* Dump the whole thing */
    DPRINT1("DbgEbp: %x\n", TrapFrame->DbgEbp);
    DPRINT1("DbgEip: %x\n", TrapFrame->DbgEip);
    DPRINT1("DbgArgMark: %x\n", TrapFrame->DbgArgMark);
    DPRINT1("DbgArgPointer: %x\n", TrapFrame->DbgArgPointer);
    DPRINT1("TempSegCs: %x\n", TrapFrame->TempSegCs);
    DPRINT1("TempEsp: %x\n", TrapFrame->TempEsp);
    DPRINT1("Dr0: %x\n", TrapFrame->Dr0);
    DPRINT1("Dr1: %x\n", TrapFrame->Dr1);
    DPRINT1("Dr2: %x\n", TrapFrame->Dr2);
    DPRINT1("Dr3: %x\n", TrapFrame->Dr3);
    DPRINT1("Dr6: %x\n", TrapFrame->Dr6);
    DPRINT1("Dr7: %x\n", TrapFrame->Dr7);
    DPRINT1("SegGs: %x\n", TrapFrame->SegGs);
    DPRINT1("SegEs: %x\n", TrapFrame->SegEs);
    DPRINT1("SegDs: %x\n", TrapFrame->SegDs);
    DPRINT1("Edx: %x\n", TrapFrame->Edx);
    DPRINT1("Ecx: %x\n", TrapFrame->Ecx);
    DPRINT1("Eax: %x\n", TrapFrame->Eax);
    DPRINT1("PreviousPreviousMode: %x\n", TrapFrame->PreviousPreviousMode);
    DPRINT1("ExceptionList: %x\n", TrapFrame->ExceptionList);
    DPRINT1("SegFs: %x\n", TrapFrame->SegFs);
    DPRINT1("Edi: %x\n", TrapFrame->Edi);
    DPRINT1("Esi: %x\n", TrapFrame->Esi);
    DPRINT1("Ebx: %x\n", TrapFrame->Ebx);
    DPRINT1("Ebp: %x\n", TrapFrame->Ebp);
    DPRINT1("ErrCode: %x\n", TrapFrame->ErrCode);
    DPRINT1("Eip: %x\n", TrapFrame->Eip);
    DPRINT1("SegCs: %x\n", TrapFrame->SegCs);
    DPRINT1("EFlags: %x\n", TrapFrame->EFlags);
    DPRINT1("HardwareEsp: %x\n", TrapFrame->HardwareEsp);
    DPRINT1("HardwareSegSs: %x\n", TrapFrame->HardwareSegSs);
    DPRINT1("V86Es: %x\n", TrapFrame->V86Es);
    DPRINT1("V86Ds: %x\n", TrapFrame->V86Ds);
    DPRINT1("V86Fs: %x\n", TrapFrame->V86Fs);
    DPRINT1("V86Gs: %x\n", TrapFrame->V86Gs);
}

#if YDEBUG
FORCEINLINE
VOID
KiFillTrapFrameDebug(IN PKTRAP_FRAME TrapFrame)
{
    /* Set the debug information */
    TrapFrame->DbgArgPointer = TrapFrame->Edx;
    TrapFrame->DbgArgMark = 0xBADB0D00;
    TrapFrame->DbgEip = TrapFrame->Eip;
    TrapFrame->DbgEbp = TrapFrame->Ebp;   
}

FORCEINLINE
VOID
KiExitTrapDebugChecks(IN PKTRAP_FRAME TrapFrame,
                      IN KTRAP_STATE_BITS SkipBits)
{
    /* Make sure interrupts are disabled */
    if (__readeflags() & EFLAGS_INTERRUPT_MASK)
    {
        DPRINT1("Exiting with interrupts enabled: %lx\n", __readeflags());
        while (TRUE);
    }
    
    /* Make sure this is a real trap frame */
    if (TrapFrame->DbgArgMark != 0xBADB0D00)
    {
        DPRINT1("Exiting with an invalid trap frame? (No MAGIC in trap frame)\n");
        KiDumpTrapFrame(TrapFrame);
        while (TRUE);
    }
    
    /* Make sure we're not in user-mode or something */
    if (Ke386GetFs() != KGDT_R0_PCR)
    {
        DPRINT1("Exiting with an invalid FS: %lx\n", Ke386GetFs());
        while (TRUE);   
    }
    
    /* Make sure we have a valid SEH chain */
    if (KeGetPcr()->Tib.ExceptionList == 0)
    {
        DPRINT1("Exiting with NULL exception chain: %p\n", KeGetPcr()->Tib.ExceptionList);
        while (TRUE);
    }
    
    /* Make sure we're restoring a valid SEH chain */
    if (TrapFrame->ExceptionList == 0)
    {
        DPRINT1("Entered a trap with a NULL exception chain: %p\n", TrapFrame->ExceptionList);
        while (TRUE);
    }
    
    /* If we're ignoring previous mode, make sure caller doesn't actually want it */
    if ((SkipBits.SkipPreviousMode) && (TrapFrame->PreviousPreviousMode != -1))
    {
        DPRINT1("Exiting a trap witout restoring previous mode, yet previous mode seems valid: %lx", TrapFrame->PreviousPreviousMode);
        while (TRUE);
    }
}

FORCEINLINE
VOID
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

BOOLEAN
FORCEINLINE
KiVdmTrap(IN PKTRAP_FRAME TrapFrame)
{
    /* Either the V8086 flag is on, or this is user-mode with a VDM */
    return ((TrapFrame->EFlags & EFLAGS_V86_MASK) ||
            ((KiUserTrap(TrapFrame)) && (PsGetCurrentProcess()->VdmObjects)));
}

VOID
FORCEINLINE
KiCheckForApcDelivery(IN PKTRAP_FRAME TrapFrame)
{
    PKTHREAD Thread;
    KIRQL OldIrql;

    /* Check for V8086 or user-mode trap */
    if ((TrapFrame->EFlags & EFLAGS_V86_MASK) ||
        (KiUserTrap(TrapFrame)))
    {
        /* Get the thread */
        Thread = KeGetCurrentThread();
        while (TRUE)
        {
            /* Turn off the alerted state for kernel mode */
            Thread->Alerted[KernelMode] = FALSE;

            /* Are there pending user APCs? */
            if (!Thread->ApcState.UserApcPending) break;

            /* Raise to APC level and enable interrupts */
            OldIrql = KfRaiseIrql(APC_LEVEL);
            _enable();

            /* Deliver APCs */
            KiDeliverApc(UserMode, NULL, TrapFrame);

            /* Restore IRQL and disable interrupts once again */
            KfLowerIrql(OldIrql);
            _disable();
        }
    }
}

VOID
FORCEINLINE
KiDispatchException0Args(IN NTSTATUS Code,
                         IN ULONG_PTR Address,
                         IN PKTRAP_FRAME TrapFrame)
{
    /* Helper for exceptions with no arguments */
    KiDispatchExceptionFromTrapFrame(Code, Address, 0, 0, 0, 0, TrapFrame);
}

VOID
FORCEINLINE
KiDispatchException1Args(IN NTSTATUS Code,
                         IN ULONG_PTR Address,
                         IN ULONG P1,
                         IN PKTRAP_FRAME TrapFrame)
{
    /* Helper for exceptions with no arguments */
    KiDispatchExceptionFromTrapFrame(Code, Address, 1, P1, 0, 0, TrapFrame);
}

VOID
FORCEINLINE
KiDispatchException2Args(IN NTSTATUS Code,
                         IN ULONG_PTR Address,
                         IN ULONG P1,
                         IN ULONG P2,
                         IN PKTRAP_FRAME TrapFrame)
{
    /* Helper for exceptions with no arguments */
    KiDispatchExceptionFromTrapFrame(Code, Address, 2, P1, P2, 0, TrapFrame);
}

FORCEINLINE
VOID
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
}

FORCEINLINE
VOID
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
}

FORCEINLINE
VOID
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
}

FORCEINLINE
VOID
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
}

FORCEINLINE
VOID
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
}

NTSTATUS
FORCEINLINE
KiSystemCallTrampoline(IN PVOID Handler,
                       IN PVOID Arguments,
                       IN ULONG StackBytes)
{
    NTSTATUS Result;
    
    /*
     * This sequence does a RtlCopyMemory(Stack - StackBytes, Arguments, StackBytes)
     * and then calls the function associated with the system call.
     *
     * It's done in assembly for two reasons: we need to muck with the stack,
     * and the call itself restores the stack back for us. The only way to do
     * this in C is to do manual C handlers for every possible number of args on
     * the stack, and then have the handler issue a call by pointer. This is
     * wasteful since it'll basically push the values twice and require another
     * level of call indirection.
     *
     * The ARM kernel currently does this, but it should probably be changed
     * later to function like this as well.
     *
     */
    __asm__ __volatile__
    (
        "subl %1, %%esp\n"
        "movl %%esp, %%edi\n"
        "movl %2, %%esi\n"
        "shrl $2, %1\n"
        "rep movsd\n"
        "call *%3\n"
        "movl %%eax, %0\n"
        : "=r"(Result)
        : "c"(StackBytes),
          "d"(Arguments),
          "r"(Handler)
        : "%esp", "%esi", "%edi"
    );
    
    return Result;
}

NTSTATUS
FORCEINLINE
KiConvertToGuiThread(VOID)
{
    NTSTATUS Result;  
    PVOID StackFrame;

    /*
     * Converting to a GUI thread safely updates ESP in-place as well as the
     * current Thread->TrapFrame and EBP when KeSwitchKernelStack is called.
     *
     * However, PsConvertToGuiThread "helpfully" restores EBP to the original
     * caller's value, since it is considered a nonvolatile register. As such,
     * as soon as we're back after the conversion and we try to store the result
     * which will probably be in some stack variable (EBP-based), we'll crash as
     * we are touching the de-allocated non-expanded stack.
     *
     * Thus we need a way to update our EBP before EBP is touched, and the only
     * way to guarantee this is to do the call itself in assembly, use the EAX
     * register to store the result, fixup EBP, and then let the C code continue
     * on its merry way.
     *
     */
    __asm__ __volatile__
    (
        "movl %%ebp, %1\n"
        "subl %%esp, %1\n"
        "call _PsConvertToGuiThread@0\n"
        "addl %%esp, %1\n"
        "movl %1, %%ebp\n"
        "movl %%eax, %0\n"
        : "=r"(Result), "=r"(StackFrame)
        :
        : "%esp", "%ecx", "%edx"
    );
        
    return Result;
}

VOID
FORCEINLINE
KiSwitchToBootStack(IN ULONG_PTR InitialStack)
{
    /* We have to switch to a new stack before continuing kernel initialization */
    __asm__ __volatile__
    (
        "movl %0, %%esp\n"
        "subl %1, %%esp\n"
        "pushl %2\n"
        "jmp _KiSystemStartupBootStack@0\n"
        : 
        : "c"(InitialStack),
          "i"(NPX_FRAME_LENGTH + KTRAP_FRAME_ALIGN + KTRAP_FRAME_LENGTH),
          "i"(CR0_EM | CR0_TS | CR0_MP)
        : "%esp"
    );
}
