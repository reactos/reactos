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
           IN UCHAR State)
{
    KTRAP_STATE_BITS StateBits = { .Bits = State };
    KiExitTrapDebugChecks(TrapFrame, StateBits);

    /* Restore the SEH handler chain */
    KeGetPcr()->Tib.ExceptionList = TrapFrame->ExceptionList;
    
    /* Check if the previous mode must be restored */
    if (StateBits.PreviousMode)
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
    
    /* Check if all registers must be restored */
    if (StateBits.Full)
    {
        /* Only do the restore if we made a transition from user-mode */
        if (KiUserTrap(TrapFrame))
        {
            /* Restore segments */
            Ke386SetGs(TrapFrame->SegGs);
            Ke386SetEs(TrapFrame->SegEs);
            Ke386SetDs(TrapFrame->SegDs);
        }
    }
    
    /* Check if we came from user-mode */
    if (KiUserTrap(TrapFrame))
    {
        /* Check if the caller wants segments restored */
        if (StateBits.Segments)
        {
            /* Restore them */
            Ke386SetGs(TrapFrame->SegGs);
            Ke386SetEs(TrapFrame->SegEs);
            Ke386SetDs(TrapFrame->SegDs);
        }
        
        /* Always restore FS since it goes from KPCR to TEB */
        Ke386SetFs(TrapFrame->SegFs);
    }

    /* Check for ABIOS code segment */
    if (TrapFrame->SegCs == 0x80)
    {
        /* Not handled yet */
        UNIMPLEMENTED;
        while (TRUE);
    }
    
    /* Check for system call */
    if (StateBits.SystemCall)
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
KiEoiHelper(IN PKTRAP_FRAME TrapFrame)
{
    /* Disable interrupts until we return */
    _disable();
    
    /* Check for APC delivery */
    KiCheckForApcDelivery(TrapFrame);
    
    /* Now exit the trap for real */
    KiExitTrap(TrapFrame, KTS_SEG_BIT | KTS_VOL_BIT);
}

/* TRAP ENTRY CODE ************************************************************/

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
    extern VOID NTAPI ExpInterlockedPopEntrySListFault(VOID);
    extern VOID CopyParams(VOID);
    extern VOID ReadBatch(VOID);

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
