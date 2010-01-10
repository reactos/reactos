/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            ntoskrnl/ke/i386/traphdlr.c
 * PURPOSE:         Kernel Trap Handlers
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>
#include "internal/trap_x.h"

/* TRAP EXIT CODE *************************************************************/

VOID
FASTCALL
KiExitTrap(IN PKTRAP_FRAME TrapFrame,
           IN UCHAR Skip)
{
    KTRAP_EXIT_SKIP_BITS SkipBits = { .Bits = Skip };
    KiExitTrapDebugChecks(TrapFrame, SkipBits);

    /* If you skip volatile reload, you must skip segment reload */
    ASSERT((SkipBits.SkipVolatiles == FALSE) || (SkipBits.SkipSegments == TRUE));

    /* Restore the SEH handler chain */
    KeGetPcr()->Tib.ExceptionList = TrapFrame->ExceptionList;
    
    /* Check if the previous mode must be restored */
    if (!SkipBits.SkipPreviousMode)
    {
        /* Not handled yet */
        UNIMPLEMENTED;
        while (TRUE);
    }

    /* Check if there are active debug registers */
    if (TrapFrame->Dr7 & ~DR7_RESERVED_MASK)
    {
        /* Not handled yet */
        UNIMPLEMENTED;
        while (TRUE);
    }
    
    /* Check if this was a V8086 trap */
    if (TrapFrame->EFlags & EFLAGS_V86_MASK) KiTrapReturn(TrapFrame);

    /* Check if the trap frame was edited */
    if (!(TrapFrame->SegCs & FRAME_EDITED))
    {
        /* Not handled yet */
        UNIMPLEMENTED;
        while (TRUE);
    }
    
    /* Check if segments should be restored */
    if (!SkipBits.SkipSegments)
    {
        /* Restore segments */
        Ke386SetGs(TrapFrame->SegGs);
        Ke386SetEs(TrapFrame->SegEs);
        Ke386SetDs(TrapFrame->SegDs);
        Ke386SetFs(TrapFrame->SegFs);
    }
    else if (KiUserTrap(TrapFrame))
    {
        /* Always restore FS since it goes from KPCR to TEB */
        Ke386SetFs(TrapFrame->SegFs);
    }
    
    /* Check if the caller wants to skip volatiles */
    if (SkipBits.SkipVolatiles)
    {
        /* 
         * When we do the system call handler through this path, we need
         * to have some sort to restore the kernel EAX instead of pushing
         * back the user EAX. We'll figure it out...
         */
        DPRINT1("Warning: caller doesn't want volatiles restored\n"); 
    }

    /* Check for ABIOS code segment */
    if (TrapFrame->SegCs == 0x80)
    {
        /* Not handled yet */
        UNIMPLEMENTED;
        while (TRUE);
    }
    
    /* Check for system call -- a system call skips volatiles! */
    if (SkipBits.SkipVolatiles)
    {
        /* Not handled yet */
        UNIMPLEMENTED;
        while (TRUE);
    }
    else
    {
        /* Return from interrupt */
        KiTrapReturn(TrapFrame); 
    }
}

VOID
FASTCALL
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
        if (!Thread->ApcState.UserApcPending) break;

        /* Raise to APC level and enable interrupts */
        OldIrql = KfRaiseIrql(APC_LEVEL);
        _enable();

        /* Deliver APCs */
        KiDeliverApc(UserMode, NULL, TrapFrame);

        /* Restore IRQL and disable interrupts once again */
        KfLowerIrql(OldIrql);
        _disable();
        
        /* Return if this isn't V86 mode anymore */
        if (TrapFrame->EFlags & EFLAGS_V86_MASK) return;
    }
     
    /* If we got here, we're still in a valid V8086 context, so quit it */
    if (TrapFrame->Dr7 & ~DR7_RESERVED_MASK)
    {
        /* Not handled yet */
        UNIMPLEMENTED;
        while (TRUE);
    }
     
    /* Return from interrupt */
    KiTrapReturn(TrapFrame);
}

VOID
FASTCALL
KiEoiHelper(IN PKTRAP_FRAME TrapFrame)
{
    /* Disable interrupts until we return */
    _disable();
    
    /* Check for APC delivery */
    KiCheckForApcDelivery(TrapFrame);
    
    /* Now exit the trap for real */
    KiExitTrap(TrapFrame, KTE_SKIP_PM_BIT);
}

/* TRAP ENTRY CODE ************************************************************/

VOID
FASTCALL
KiEnterV86Trap(IN PKTRAP_FRAME TrapFrame)
{
    /* Save registers */
    KiTrapFrameFromPushaStack(TrapFrame);
    
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
    if (TrapFrame->Dr7 & ~DR7_RESERVED_MASK)
    {
        UNIMPLEMENTED;
        while (TRUE);
    }
}

VOID
FASTCALL
KiEnterTrap(IN PKTRAP_FRAME TrapFrame)
{
    /* Save registers */
    KiTrapFrameFromPushaStack(TrapFrame);
    
    /* Save segments and then switch to correct ones */
    TrapFrame->SegFs = Ke386GetFs();
    TrapFrame->SegGs = Ke386GetGs();
    TrapFrame->SegDs = Ke386GetDs();
    TrapFrame->SegEs = Ke386GetEs();
    Ke386SetFs(KGDT_R0_PCR);
    Ke386SetDs(KGDT_R3_DATA | RPL_MASK);
    Ke386SetEs(KGDT_R3_DATA | RPL_MASK);
    
    /* Save exception list and bogus previous mode */
    TrapFrame->PreviousPreviousMode = -1;
    TrapFrame->ExceptionList = KeGetPcr()->Tib.ExceptionList;
    
    /* Check for 16-bit stack */
    if ((ULONG_PTR)TrapFrame < 0x10000)
    {
        UNIMPLEMENTED;
        while (TRUE);
    }
    
    /* Check for V86 mode */
    if (TrapFrame->EFlags & EFLAGS_V86_MASK)
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
    if (KeGetCurrentThread()->DispatcherHeader.DebugActive & 0xFF)
    {
        UNIMPLEMENTED;
        while (TRUE);
    }
    
    /* Set debug header */
    KiFillTrapFrameDebug(TrapFrame);
}

/* EXCEPTION CODE *************************************************************/

VOID
FASTCALL
KiSystemFatalException(IN ULONG ExceptionCode,
                       IN PKTRAP_FRAME TrapFrame)
{
    /* Bugcheck the system */
    KeBugCheckWithTf(UNEXPECTED_KERNEL_MODE_TRAP,
                     ExceptionCode,
                     0,
                     0,
                     0,
                     TrapFrame);
}

VOID
NTAPI
KiDispatchExceptionFromTrapFrame(IN NTSTATUS Code,
                                 IN ULONG_PTR Address,
                                 IN ULONG ParameterCount,
                                 IN ULONG_PTR Parameter1,
                                 IN ULONG_PTR Parameter2,
                                 IN ULONG_PTR Parameter3,
                                 IN PKTRAP_FRAME TrapFrame)
{
    EXCEPTION_RECORD ExceptionRecord;

    /* Build the exception record */
    ExceptionRecord.ExceptionCode = Code;
    ExceptionRecord.ExceptionFlags = 0;
    ExceptionRecord.ExceptionRecord = NULL;
    ExceptionRecord.ExceptionAddress = (PVOID)Address;
    ExceptionRecord.NumberParameters = ParameterCount;
    if (ParameterCount)
    {
        /* Copy extra parameters */
        ExceptionRecord.ExceptionInformation[0] = Parameter1;
        ExceptionRecord.ExceptionInformation[1] = Parameter2;
        ExceptionRecord.ExceptionInformation[2] = Parameter3;
    }
    
    /* Now go dispatch the exception */
    KiDispatchException(&ExceptionRecord,
                        NULL,
                        TrapFrame,
                        TrapFrame->EFlags & EFLAGS_V86_MASK ?
                        -1 : TrapFrame->SegCs & MODE_MASK,
                        TRUE);

    /* Return from this trap */
    KiEoiHelper(TrapFrame);
}

/* TRAP HANDLERS **************************************************************/

VOID
FASTCALL
KiDebugHandler(IN PKTRAP_FRAME TrapFrame,
               IN ULONG Parameter1,
               IN ULONG Parameter2,
               IN ULONG Parameter3)
{
    /* Check for VDM trap */
    ASSERT((KiVdmTrap(TrapFrame)) == FALSE);

    /* Enable interrupts if the trap came from user-mode */
    if (KiUserTrap(TrapFrame)) _enable();

    /* Dispatch the exception  */
    KiDispatchExceptionFromTrapFrame(STATUS_BREAKPOINT,
                                     TrapFrame->Eip - 1,
                                     3,
                                     Parameter1,
                                     Parameter2,
                                     Parameter3,
                                     TrapFrame); 
}

VOID
FASTCALL
KiNpxHandler(IN PKTRAP_FRAME TrapFrame,
             IN PKTHREAD Thread,
             IN PFX_SAVE_AREA SaveArea)
{
    ULONG Cr0, Mask, Error, ErrorOffset, DataOffset;
    
    /* Check for VDM trap */
    ASSERT((KiVdmTrap(TrapFrame)) == FALSE);

    /* Check for kernel trap */
    if (!KiUserTrap(TrapFrame))
    {
        /* Kernel might've tripped a delayed error */
        SaveArea->Cr0NpxState |= CR0_TS;
        
        /* Only valid if it happened during a restore */
        //if ((PVOID)TrapFrame->Eip == FrRestore)
        {
            /* It did, so just skip the instruction */
            //TrapFrame->Eip += 3; /* sizeof(FRSTOR) */
            //KiEoiHelper(TrapFrame);
        }
    }

    /* User or kernel trap -- get ready to issue an exception */
    if (Thread->NpxState == NPX_STATE_NOT_LOADED)
    {
        /* Update CR0 */
        Cr0 = __readcr0();
        Cr0 &= ~(CR0_MP | CR0_EM | CR0_TS);
        __writecr0(Cr0);

        /* Save FPU state */
        //Ke386SaveFpuState(SaveArea);

        /* Mark CR0 state dirty */
        Cr0 |= NPX_STATE_NOT_LOADED;
        Cr0 |= SaveArea->Cr0NpxState;
        __writecr0(Cr0);

        /* Update NPX state */
        Thread->NpxState = NPX_STATE_NOT_LOADED;
        KeGetCurrentPrcb()->NpxThread = NULL;
    }

    /* Clear the TS bit and re-enable interrupts */
    SaveArea->Cr0NpxState &= ~CR0_TS;
    _enable();
    
    /* Check if we should get the FN or FX error */
    if (KeI386FxsrPresent)
    {
        /* Get it from FX */
        Mask = SaveArea->U.FxArea.ControlWord;
        Error = SaveArea->U.FxArea.StatusWord;
        
        /* Get the FPU exception address too */
        ErrorOffset = SaveArea->U.FxArea.ErrorOffset;
        DataOffset = SaveArea->U.FxArea.DataOffset;
    }
    else
    {
        /* Get it from FN */
        Mask = SaveArea->U.FnArea.ControlWord;
        Error = SaveArea->U.FnArea.StatusWord;
        
        /* Get the FPU exception address too */
        ErrorOffset = SaveArea->U.FnArea.ErrorOffset;
        DataOffset = SaveArea->U.FnArea.DataOffset;
    }

    /* Get legal exceptions that software should handle */
    Error &= (FSW_INVALID_OPERATION |
              FSW_DENORMAL |
              FSW_ZERO_DIVIDE |
              FSW_OVERFLOW |
              FSW_UNDERFLOW |
              FSW_PRECISION);
    Error &= ~Mask;
    
    if (Error & FSW_STACK_FAULT)
    {
        /* Issue stack check fault */
        KiDispatchException2Args(STATUS_FLOAT_STACK_CHECK,
                                 ErrorOffset,
                                 0,
                                 DataOffset,
                                 TrapFrame);
    }
    
    /* Check for invalid operation */
    if (Error & FSW_INVALID_OPERATION)
    {
        /* Issue fault */
        KiDispatchException1Args(STATUS_FLOAT_INVALID_OPERATION,
                                 ErrorOffset,
                                 0,
                                 TrapFrame);
    }
    
    /* Check for divide by zero */
    if (Error & FSW_ZERO_DIVIDE)
    {
        /* Issue fault */
        KiDispatchException1Args(STATUS_FLOAT_DIVIDE_BY_ZERO,
                                 ErrorOffset,
                                 0,
                                 TrapFrame);
    }
    
    /* Check for denormal */
    if (Error & FSW_DENORMAL)
    {
        /* Issue fault */
        KiDispatchException1Args(STATUS_FLOAT_INVALID_OPERATION,
                                 ErrorOffset,
                                 0,
                                 TrapFrame);
    }
    
    /* Check for overflow */
    if (Error & FSW_OVERFLOW)
    {
        /* Issue fault */
        KiDispatchException1Args(STATUS_FLOAT_OVERFLOW,
                                 ErrorOffset,
                                 0,
                                 TrapFrame);
    }
    
    /* Check for underflow */
    if (Error & FSW_UNDERFLOW)
    {
        /* Issue fault */
        KiDispatchException1Args(STATUS_FLOAT_UNDERFLOW,
                                 ErrorOffset,
                                 0,
                                 TrapFrame);
    }

    /* Check for precision fault */
    if (Error & FSW_PRECISION)
    {
        /* Issue fault */
        KiDispatchException1Args(STATUS_FLOAT_INEXACT_RESULT,
                                 ErrorOffset,
                                 0,
                                 TrapFrame);
    }
    
    /* Unknown FPU fault */
    KeBugCheckWithTf(TRAP_CAUSE_UNKNOWN, 1, Error, 0, 0, TrapFrame);
}

VOID
FASTCALL
KiTrap0Handler(IN PKTRAP_FRAME TrapFrame)
{
    /* Save trap frame */
    KiEnterTrap(TrapFrame);
    
    /* Check for VDM trap */
    ASSERT((KiVdmTrap(TrapFrame)) == FALSE);

    /*  Enable interrupts */
    _enable();
    
    /* Dispatch the exception */
    KiDispatchException0Args(STATUS_INTEGER_DIVIDE_BY_ZERO,
                             TrapFrame->Eip,
                             TrapFrame);
}

VOID
FASTCALL
KiTrap1Handler(IN PKTRAP_FRAME TrapFrame)
{
    /* Save trap frame */
    KiEnterTrap(TrapFrame);
    
    /* Check for VDM trap */
    ASSERT((KiVdmTrap(TrapFrame)) == FALSE);

    /* Enable interrupts if the trap came from user-mode */
    if (KiUserTrap(TrapFrame)) _enable();
    
    /*  Mask out trap flag and dispatch the exception */
    TrapFrame->EFlags &= ~EFLAGS_TF;
    KiDispatchException0Args(STATUS_SINGLE_STEP,
                             TrapFrame->Eip,
                             TrapFrame);
}

VOID
FASTCALL
KiTrap3Handler(IN PKTRAP_FRAME TrapFrame)
{
    /* Save trap frame */
    KiEnterTrap(TrapFrame);
    
    /* Continue with the common handler */
    KiDebugHandler(TrapFrame, BREAKPOINT_BREAK, 0, 0);
}

VOID
FASTCALL
KiTrap4Handler(IN PKTRAP_FRAME TrapFrame)
{
    /* Save trap frame */
    KiEnterTrap(TrapFrame);
    
    /* Check for VDM trap */
    ASSERT((KiVdmTrap(TrapFrame)) == FALSE);

     /* Enable interrupts */
    _enable();
    
    /* Dispatch the exception */
    KiDispatchException0Args(STATUS_INTEGER_OVERFLOW,
                             TrapFrame->Eip - 1,
                             TrapFrame);
}

VOID
FASTCALL
KiTrap5Handler(IN PKTRAP_FRAME TrapFrame)
{
    /* Save trap frame */
    KiEnterTrap(TrapFrame);
    
    /* Check for VDM trap */
    ASSERT((KiVdmTrap(TrapFrame)) == FALSE);
    
    /* Check for kernel-mode fault */
    if (!KiUserTrap(TrapFrame)) KiSystemFatalException(EXCEPTION_BOUND_CHECK, TrapFrame);

    /* Enable interrupts */
    _enable();
    
    /* Dispatch the exception */
    KiDispatchException0Args(STATUS_ARRAY_BOUNDS_EXCEEDED,
                             TrapFrame->Eip,
                             TrapFrame);
}

VOID
FASTCALL
KiTrap6Handler(IN PKTRAP_FRAME TrapFrame)
{
    PUCHAR Instruction;
    ULONG i;
    
    /* Save trap frame */
    KiEnterTrap(TrapFrame);
    
    /* Check for VDM trap */
    ASSERT((KiVdmTrap(TrapFrame)) == FALSE);
    
    /* Enable interrupts */
    Instruction = (PUCHAR)TrapFrame->Eip;
    _enable();
        
    /* Check for user trap */
    if (KiUserTrap(TrapFrame))
    {
        /* FIXME: Use SEH */
        
        /* Scan next 4 opcodes */
        for (i = 0; i < 4; i++)
        {
            /* Check for LOCK instruction */
            if (Instruction[i] == 0xF0)
            {
                /* Send invalid lock sequence exception */
                KiDispatchException0Args(STATUS_INVALID_LOCK_SEQUENCE,
                                         TrapFrame->Eip,
                                         TrapFrame);
            }
        }
        
        /* FIXME: SEH ends here */
    }
    
    /* Kernel-mode or user-mode fault (but not LOCK) */
    KiDispatchException0Args(STATUS_ILLEGAL_INSTRUCTION,
                             TrapFrame->Eip,
                             TrapFrame);
    
}

VOID
FASTCALL
KiTrap7Handler(IN PKTRAP_FRAME TrapFrame)
{
    PKTHREAD Thread, NpxThread;
    PFX_SAVE_AREA SaveArea, NpxSaveArea;
    ULONG Cr0;
    
    /* Save trap frame */
    KiEnterTrap(TrapFrame);

    /* Try to handle NPX delay load */
    while (TRUE)
    {
        /* Get the current thread */
        Thread = KeGetCurrentThread();

        /* Get the NPX frame */
        SaveArea = KiGetThreadNpxArea(Thread);

        /* Check if emulation is enabled */
        if (SaveArea->Cr0NpxState & CR0_EM)
        {
            /* Not implemented */
            UNIMPLEMENTED;
            while (TRUE);
        }
    
        /* Save CR0 and check NPX state */
        Cr0 = __readcr0();
        if (Thread->NpxState != NPX_STATE_LOADED)
        {
            /* Update CR0 */
            Cr0 &= ~(CR0_MP | CR0_EM | CR0_TS);
            __writecr0(Cr0);
        
            /* Get the NPX thread */
            NpxThread = KeGetCurrentPrcb()->NpxThread;
            if (NpxThread)
            {
                /* Get the NPX frame */
                NpxSaveArea = KiGetThreadNpxArea(NpxThread);
                
                /* Save FPU state */
                //Ke386SaveFpuState(NpxSaveArea);

                /* Update NPX state */
                Thread->NpxState = NPX_STATE_NOT_LOADED;
           }
       
            /* Load FPU state */
            //Ke386LoadFpuState(SaveArea);
        
            /* Update NPX state */
            Thread->NpxState = NPX_STATE_LOADED;
            KeGetCurrentPrcb()->NpxThread = Thread;
        
            /* Enable interrupts */
            _enable();
        
            /* Check if CR0 needs to be reloaded due to context switch */
            if (!SaveArea->Cr0NpxState) KiEoiHelper(TrapFrame);
        
            /* Otherwise, we need to reload CR0, disable interrupts */
            _disable();
        
            /* Reload CR0 */
            Cr0 = __readcr0();
            Cr0 |= SaveArea->Cr0NpxState;
            __writecr0(Cr0);
        
            /* Now restore interrupts and check for TS */
            _enable();
            if (Cr0 & CR0_TS) KiEoiHelper(TrapFrame);
        
            /* We're still here -- clear TS and try again */
            __writecr0(__readcr0() &~ CR0_TS);
            _disable();
        }
        else
        {
            /* This is an actual fault, not a lack of FPU state */
            break;
        }
    }
    
    /* TS should not be set */
    if (Cr0 & CR0_TS)
    {
        /*
         * If it's incorrectly set, then maybe the state is actually still valid
         * but we could've lock track of that due to a BIOS call.
         * Make sure MP is still set, which should verify the theory.
         */
        if (Cr0 & CR0_MP)
        {
            /* Indeed, the state is actually still valid, so clear TS */
            __writecr0(__readcr0() &~ CR0_TS);
            KiEoiHelper(TrapFrame);
        }
        
        /* Otherwise, something strange is going on */
        KeBugCheckWithTf(TRAP_CAUSE_UNKNOWN, 2, Cr0, 0, 0, TrapFrame);
    }
    
    /* It's not a delayed load, so process this trap as an NPX fault */
    KiNpxHandler(TrapFrame, Thread, SaveArea);
}

VOID
FASTCALL
KiTrap8Handler(IN PKTRAP_FRAME TrapFrame)
{
    /* FIXME: Not handled */
    KiSystemFatalException(EXCEPTION_DOUBLE_FAULT, TrapFrame);
}

VOID
FASTCALL
KiTrap9Handler(IN PKTRAP_FRAME TrapFrame)
{
    /* Save trap frame */
    KiEnterTrap(TrapFrame);

    /* Enable interrupts and kill the system */
    _enable();
    KiSystemFatalException(EXCEPTION_NPX_OVERRUN, TrapFrame);
}

VOID
FASTCALL
KiTrap10Handler(IN PKTRAP_FRAME TrapFrame)
{
    /* Save trap frame */
    KiEnterTrap(TrapFrame);

    /* Check for VDM trap */
    ASSERT((KiVdmTrap(TrapFrame)) == FALSE);
    
    /* Kill the system */
    KiSystemFatalException(EXCEPTION_INVALID_TSS, TrapFrame);
}

VOID
FASTCALL
KiTrap11Handler(IN PKTRAP_FRAME TrapFrame)
{
    /* Save trap frame */
    KiEnterTrap(TrapFrame);

    /* FIXME: Kill the system */
    UNIMPLEMENTED;
    KiSystemFatalException(EXCEPTION_SEGMENT_NOT_PRESENT, TrapFrame);
}

VOID
FASTCALL
KiTrap12Handler(IN PKTRAP_FRAME TrapFrame)
{
    /* Save trap frame */
    KiEnterTrap(TrapFrame);

    /* FIXME: Kill the system */
    UNIMPLEMENTED;
    KiSystemFatalException(EXCEPTION_STACK_FAULT, TrapFrame);
}

VOID
FASTCALL
KiTrap14Handler(IN PKTRAP_FRAME TrapFrame)
{
    PKTHREAD Thread;
    ULONG_PTR Cr2;
    NTSTATUS Status;

    /* Save trap frame */
    KiEnterTrap(TrapFrame);

    /* Check for custom VDM trap handler */
    if (KeGetPcr()->VdmAlert)
    {
        /* Not implemented */
        UNIMPLEMENTED;
        while (TRUE);
    }

    /* Check if this is the base frame */
    Thread = KeGetCurrentThread();
    if (KeGetTrapFrame(Thread) != TrapFrame)
    {
        /* It isn't, check if this is a second nested frame */
        if (((ULONG_PTR)KeGetTrapFrame(Thread) - (ULONG_PTR)TrapFrame) <=
            FIELD_OFFSET(KTRAP_FRAME, EFlags))
        {
            /* The stack is somewhere in between frames, we need to fix it */
            UNIMPLEMENTED;
            while (TRUE);
        }
    }
    
    /* Save CR2 */
    Cr2 = __readcr2();
    
    /* Check for Pentium LOCK errata */
    if (KiI386PentiumLockErrataPresent)
    {
        /* Not yet implemented */
        UNIMPLEMENTED;
        while (TRUE);
    }
    
    /* HACK: Check if interrupts are disabled and enable them */
    if (!(TrapFrame->EFlags & EFLAGS_INTERRUPT_MASK))
    {
        /* Enable interupts */
        _enable();
#ifdef HACK_ABOVE_FIXED
        if (!(TrapFrame->EFlags & EFLAGS_INTERRUPT_MASK))
        {
            /* This is illegal */
            KeBugCheckWithTf(IRQL_NOT_LESS_OR_EQUAL,
                             Cr2,
                             -1,
                             TrapFrame->ErrCode & 1,
                             TrapFrame->Eip,
                             TrapFrame);
        }
#endif
    }

    /* Call the access fault handler */
    Status = MmAccessFault(TrapFrame->ErrCode & 1,
                           (PVOID)Cr2,
                           TrapFrame->SegCs & MODE_MASK,
                           TrapFrame);
    if (Status == STATUS_SUCCESS) KiEoiHelper(TrapFrame);
    
    /* Check for S-LIST fault */
    if (TrapFrame->Eip == (ULONG_PTR)ExpInterlockedPopEntrySListFault)
    {
        /* Not yet implemented */
        UNIMPLEMENTED;
        while (TRUE);   
    }
    
    /* Check for syscall fault */
    if ((TrapFrame->Eip == (ULONG_PTR)CopyParams) ||
        (TrapFrame->Eip == (ULONG_PTR)ReadBatch))
    {
        /* Not yet implemented */
        UNIMPLEMENTED;
        while (TRUE);
    }

    /* Check for VDM trap */
    ASSERT((KiVdmTrap(TrapFrame)) == FALSE);
    
    /* Either kernel or user trap (non VDM) so dispatch exception */
    if (Status == STATUS_ACCESS_VIOLATION)
    {
        /* This status code is repurposed so we can recognize it later */
        KiDispatchException2Args(KI_EXCEPTION_ACCESS_VIOLATION,
                                 TrapFrame->Eip,
                                 TrapFrame->ErrCode & 1,
                                 Cr2,
                                 TrapFrame);
    }
    else if ((Status == STATUS_GUARD_PAGE_VIOLATION) ||
             (Status == STATUS_STACK_OVERFLOW))
    {
        /* These faults only have two parameters */
        KiDispatchException2Args(Status,
                                 TrapFrame->Eip,
                                 TrapFrame->ErrCode & 1,
                                 Cr2,
                                 TrapFrame);
    }
    
    /* Only other choice is an in-page error, with 3 parameters */
    KiDispatchExceptionFromTrapFrame(STATUS_IN_PAGE_ERROR,
                                     TrapFrame->Eip,
                                     3,
                                     TrapFrame->ErrCode & 1,
                                     Cr2,
                                     Status,
                                     TrapFrame);
}

VOID
FASTCALL
KiTrap0FHandler(IN PKTRAP_FRAME TrapFrame)
{
    /* Save trap frame */
    KiEnterTrap(TrapFrame);

    /* FIXME: Kill the system */
    UNIMPLEMENTED;
    KiSystemFatalException(EXCEPTION_RESERVED_TRAP, TrapFrame);
}


VOID
FASTCALL
KiTrap16Handler(IN PKTRAP_FRAME TrapFrame)
{
    PKTHREAD Thread;
    PFX_SAVE_AREA SaveArea;
    
    /* Save trap frame */
    KiEnterTrap(TrapFrame);

    /* Check if this is the NPX thrad */
    Thread = KeGetCurrentThread();
    SaveArea = KiGetThreadNpxArea(Thread);
    if (Thread != KeGetCurrentPrcb()->NpxThread)
    {
        /* It isn't, enable interrupts and set delayed error */
        _enable();
        SaveArea->Cr0NpxState |= CR0_TS;
        
        /* End trap */
        KiEoiHelper(TrapFrame);
    }
    
    /* Otherwise, proceed with NPX fault handling */
    KiNpxHandler(TrapFrame, Thread, SaveArea);
}

VOID
FASTCALL
KiTrap17Handler(IN PKTRAP_FRAME TrapFrame)
{
    /* Save trap frame */
    KiEnterTrap(TrapFrame);

    /* Enable interrupts and kill the system */
    _enable();
    KiSystemFatalException(EXCEPTION_ALIGNMENT_CHECK, TrapFrame);
}

VOID
FASTCALL
KiTrap19Handler(IN PKTRAP_FRAME TrapFrame)
{
    PKTHREAD Thread;
    PFX_SAVE_AREA SaveArea;
    ULONG Cr0, MxCsrMask, Error;
    
    /* Save trap frame */
    KiEnterTrap(TrapFrame);

    /* Check if this is the NPX thrad */
    Thread = KeGetCurrentThread();
    if (Thread != KeGetCurrentPrcb()->NpxThread)
    {
        /* It isn't, kill the system */
        KeBugCheckWithTf(TRAP_CAUSE_UNKNOWN, 13, (ULONG_PTR)Thread, 0, 0, TrapFrame);
    }

    /* Get the NPX frame */
    SaveArea = KiGetThreadNpxArea(Thread);

    /* Check for VDM trap */
    ASSERT((KiVdmTrap(TrapFrame)) == FALSE);

    /* Check for user trap */
    if (!KiUserTrap(TrapFrame))
    {
        /* Kernel should not fault on XMMI */
        KeBugCheckWithTf(TRAP_CAUSE_UNKNOWN, 13, 0, 0, 2, TrapFrame);
    }
    
    /* Update CR0 */
    Cr0 = __readcr0();
    Cr0 &= ~(CR0_MP | CR0_EM | CR0_TS);
    __writecr0(Cr0);
    
    /* Save FPU state */
    //Ke386SaveFpuState(SaveArea);
    
    /* Mark CR0 state dirty */
    Cr0 |= NPX_STATE_NOT_LOADED;
    Cr0 |= SaveArea->Cr0NpxState;
     __writecr0(Cr0);
    
    /* Update NPX state */
    Thread->NpxState = NPX_STATE_NOT_LOADED;
    KeGetCurrentPrcb()->NpxThread = NULL;
    
    /* Clear the TS bit and re-enable interrupts */
    SaveArea->Cr0NpxState &= ~CR0_TS;
    _enable();

    /* Now look at MxCsr to get the mask of errors we should care about */
    MxCsrMask = ~((USHORT)SaveArea->U.FxArea.MXCsr >> 7);
    
    /* Get legal exceptions that software should handle */
    Error = (USHORT)SaveArea->U.FxArea.MXCsr & (FSW_INVALID_OPERATION |
                                                FSW_DENORMAL |
                                                FSW_ZERO_DIVIDE |
                                                FSW_OVERFLOW |
                                                FSW_UNDERFLOW |
                                                FSW_PRECISION);
    Error &= MxCsrMask;
    
    /* Now handle any of those legal errors */
    if (Error & (FSW_INVALID_OPERATION |
                 FSW_DENORMAL |
                 FSW_ZERO_DIVIDE |
                 FSW_OVERFLOW |
                 FSW_UNDERFLOW |
                 FSW_PRECISION))
    {
        /* By issuing an exception */
        KiDispatchException1Args(STATUS_FLOAT_MULTIPLE_TRAPS,
                                 TrapFrame->Eip,
                                 0,
                                 TrapFrame);
    }
    
    /* Unknown XMMI fault */
    KeBugCheckWithTf(TRAP_CAUSE_UNKNOWN, 13, 0, 0, 1, TrapFrame);
}

VOID
FASTCALL
KiRaiseAssertionHandler(IN PKTRAP_FRAME TrapFrame)
{
    /* Save trap frame */
    KiEnterTrap(TrapFrame);

    /* Decrement EIP to point to the INT2C instruction (2 bytes, not 1 like INT3) */
    TrapFrame->Eip -= 2;

    /* Dispatch the exception */
    KiDispatchException0Args(STATUS_ASSERTION_FAILURE,
                             TrapFrame->Eip,
                             TrapFrame);
}

VOID
FASTCALL
KiDebugServiceHandler(IN PKTRAP_FRAME TrapFrame)
{
    /* Save trap frame */
    KiEnterTrap(TrapFrame);
    
    /* Increment EIP to skip the INT3 instruction */
    TrapFrame->Eip++;
    
    /* Continue with the common handler */
    KiDebugHandler(TrapFrame, TrapFrame->Eax, TrapFrame->Ecx, TrapFrame->Edx);
}

/* EOF */
