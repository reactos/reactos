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
#if YDEBUG
VOID
NTAPI
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
                      IN KTRAP_STATE_BITS StateBits)
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
    if (!(StateBits.PreviousMode) && (TrapFrame->PreviousPreviousMode != -1))
    {
        DPRINT1("Exiting a trap witout restoring previous mode, yet previous mode seems valid: %lx", TrapFrame->PreviousPreviousMode);
        while (TRUE);
    }
}
#else
#define KiExitTrapDebugChecks(x, y)
#define KiFillTrapFrameDebug(x)
#endif

//
// Helper Code
//

BOOLEAN
FORCEINLINE
KiUserTrap(IN PKTRAP_FRAME TrapFrame)
{
    /* Anything else but Ring 0 is Ring 3 */
    return (TrapFrame->SegCs != KGDT_R0_CODE);
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
KiTrapFrameFromPushaStack(IN PKTRAP_FRAME TrapFrame)
{
    /*
     * This sequence is Bavarian Alchemist Black Magic
     *
     * *** DO NOT MODIFY ***
     */
    TrapFrame->Edx = TrapFrame->Esi;
    TrapFrame->Esi = TrapFrame->PreviousPreviousMode;
    TrapFrame->Ecx = TrapFrame->Ebx;
    TrapFrame->Ebx = TrapFrame->Edi;
    TrapFrame->Edi = TrapFrame->Eax;
    TrapFrame->Eax = TrapFrame->Ebp;
    TrapFrame->Ebp = (ULONG)TrapFrame->ExceptionList;
    TrapFrame->TempEsp = TrapFrame->SegFs;
}

VOID
FORCEINLINE
KiPushaStackFromTrapFrame(IN PKTRAP_FRAME TrapFrame)
{
    /*
     * This sequence is Bavarian Alchemist Black Magic
     *
     * *** DO NOT MODIFY ***
     */
    TrapFrame->SegFs = TrapFrame->TempEsp;
    TrapFrame->ExceptionList = (PVOID)TrapFrame->Ebp;
    TrapFrame->Ebp = TrapFrame->Eax;
    TrapFrame->Eax = TrapFrame->Edi;
    TrapFrame->Edi = TrapFrame->Ebx;
    TrapFrame->Ebx = TrapFrame->Ecx;
    TrapFrame->PreviousPreviousMode = TrapFrame->Esi;
    TrapFrame->Esi = TrapFrame->Edx;
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

FORCEINLINE
VOID
KiTrapReturn(IN PKTRAP_FRAME TrapFrame)
{
    /* Restore registers */
    KiPushaStackFromTrapFrame(TrapFrame);

    /* Regular interrupt exit */
    __asm__ __volatile__
    (
        "movl %0, %%esp\n"
        "addl %1, %%esp\n"
        "popa\n"
        "addl $4, %%esp\n"
        "iret\n"
        :
        : "r"(TrapFrame), "i"(KTRAP_FRAME_LENGTH - KTRAP_FRAME_PREVIOUS_MODE)
        : "%esp"
    );
}
