/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL - 2.0 + (https ://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     AMD64 User-mode Callout Mechanisms (APC and Win32K Callbacks)
 * COPYRIGHT:   Timo Kreuzer(timo.kreuzer@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/*! \name KiInitializeUserApc
*
*  \brief
*      Prepares the current trap frame (which must have come from user mode)
*      with the ntdll.KiUserApcDispatcher entrypoint, copying a CONTEXT
*      record with the context from the old trap frame to the threads user
*      mode stack.
*
*  \param ExceptionFrame
*  \param TrapFrame
*  \param NormalRoutine
*  \param NormalContext
*  \param SystemArgument1
*  \param SystemArgument2
*
*  \remarks
*      This function is called from KiDeliverApc, when the trap frame came
*      from user mode. This happens before a systemcall or interrupt exits back
*      to usermode or when a thread is started from PspUserThreadstartup.
*      The trap exit code will then leave to KiUserApcDispatcher which in turn
*      calls the NormalRoutine, passing NormalContext, SystemArgument1 and
*      SystemArgument2 as parameters. When that function returns, it calls
*      NtContinue to return back to the kernel, where the old context that was
*      saved on the usermode stack is restored and execution is transferred
*      back to usermode, where the original trap originated from.
*
*--*/
VOID
NTAPI
KiInitializeUserApc(
    _In_ PKEXCEPTION_FRAME ExceptionFrame,
    _Inout_ PKTRAP_FRAME TrapFrame,
    _In_ PKNORMAL_ROUTINE NormalRoutine,
    _In_ PVOID NormalContext,
    _In_ PVOID SystemArgument1,
    _In_ PVOID SystemArgument2)
{
    CONTEXT Context;
    ULONG64 AlignedRsp, Stack;
    EXCEPTION_RECORD SehExceptRecord;

    /* Sanity check, that the trap frame is from user mode */
    ASSERT((TrapFrame->SegCs & MODE_MASK) != KernelMode);

    /* Convert the current trap frame to a context */
    Context.ContextFlags = CONTEXT_FULL | CONTEXT_DEBUG_REGISTERS;
    KeTrapFrameToContext(TrapFrame, ExceptionFrame, &Context);

    /* We jump to KiUserApcDispatcher in ntdll */
    TrapFrame->Rip = (ULONG64)KeUserApcDispatcher;

    /* Setup Ring 3 segments */
    TrapFrame->SegCs = KGDT64_R3_CODE | RPL_MASK;
    TrapFrame->SegDs = KGDT64_R3_DATA | RPL_MASK;
    TrapFrame->SegEs = KGDT64_R3_DATA | RPL_MASK;
    TrapFrame->SegFs = KGDT64_R3_CMTEB | RPL_MASK;
    TrapFrame->SegGs = KGDT64_R3_DATA | RPL_MASK;
    TrapFrame->SegSs = KGDT64_R3_DATA | RPL_MASK;

    /* Sanitize EFLAGS, enable interrupts */
    TrapFrame->EFlags = (Context.EFlags & EFLAGS_USER_SANITIZE);
    TrapFrame->EFlags |= EFLAGS_INTERRUPT_MASK;

    /* Set parameters for KiUserApcDispatcher */
    Context.P1Home = (ULONG64)NormalContext;
    Context.P2Home = (ULONG64)SystemArgument1;
    Context.P3Home = (ULONG64)SystemArgument2;
    Context.P4Home = (ULONG64)NormalRoutine;

    /* Check if thread has IOPL and force it enabled if so */
    //if (KeGetCurrentThread()->Iopl) TrapFrame->EFlags |= EFLAGS_IOPL;

    /* Align Stack to 16 bytes and allocate space */
    AlignedRsp = Context.Rsp & ~15;
    Stack = AlignedRsp - sizeof(CONTEXT);
    TrapFrame->Rsp = Stack;

    /* The stack must be 16 byte aligned for KiUserApcDispatcher */
    ASSERT((Stack & 15) == 0);

    /* Protect with SEH */
    _SEH2_TRY
    {
        /* Probe the stack */
        ProbeForWrite((PCONTEXT)Stack,  sizeof(CONTEXT), 8);

        /* Copy the context */
        RtlCopyMemory((PCONTEXT)Stack, &Context, sizeof(CONTEXT));
    }
    _SEH2_EXCEPT((RtlCopyMemory(&SehExceptRecord, _SEH2_GetExceptionInformation()->ExceptionRecord, sizeof(EXCEPTION_RECORD)), EXCEPTION_EXECUTE_HANDLER))
    {
        /* Dispatch the exception */
        SehExceptRecord.ExceptionAddress = (PVOID)TrapFrame->Rip;
        KiDispatchException(&SehExceptRecord,
                            ExceptionFrame,
                            TrapFrame,
                            UserMode,
                            TRUE);
    }
    _SEH2_END;
}
