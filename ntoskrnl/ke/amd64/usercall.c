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

/*!
 *  \name KiInitializeUserApc
 *
 *  \brief
 *      Prepares the current trap frame (which must have come from user mode)
 *      with the ntdll.KiUserApcDispatcher entrypoint, copying a CONTEXT
 *      record with the context from the old trap frame to the threads user
 *      mode stack.
 *
 *  \param ExceptionFrame - Pointer to the Exception Frame
 *
 *  \param TrapFrame  Pointer to the Trap Frame.
 *
 *  \param NormalRoutine - Pointer to the NormalRoutine to call.
 *
 *  \param NormalContext - Pointer to the context to send to the Normal Routine.
 *
 *  \param SystemArgument[1-2]
 *        Pointer to a set of two parameters that contain untyped data.
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
    PCONTEXT Context;
    EXCEPTION_RECORD ExceptionRecord;

    /* Sanity check, that the trap frame is from user mode */
    ASSERT((TrapFrame->SegCs & MODE_MASK) != KernelMode);

    /* Align the user tack to 16 bytes and allocate space for a CONTEXT structure */
    Context = (PCONTEXT)ALIGN_DOWN_POINTER_BY(TrapFrame->Rsp, 16) - 1;

    /* Protect with SEH */
    _SEH2_TRY
    {
        /* Probe the context */
        ProbeForWrite(Context,  sizeof(CONTEXT), 16);

        /* Convert the current trap frame to a context */
        Context->ContextFlags = CONTEXT_FULL | CONTEXT_DEBUG_REGISTERS;
        KeTrapFrameToContext(TrapFrame, ExceptionFrame, Context);

        /* Set parameters for KiUserApcDispatcher */
        Context->P1Home = (ULONG64)NormalContext;
        Context->P2Home = (ULONG64)SystemArgument1;
        Context->P3Home = (ULONG64)SystemArgument2;
        Context->P4Home = (ULONG64)NormalRoutine;
    }
    _SEH2_EXCEPT(ExceptionRecord = *_SEH2_GetExceptionInformation()->ExceptionRecord, EXCEPTION_EXECUTE_HANDLER)
    {
        /* Dispatch the exception */
        ExceptionRecord.ExceptionAddress = (PVOID)TrapFrame->Rip;
        KiDispatchException(&ExceptionRecord,
                            ExceptionFrame,
                            TrapFrame,
                            UserMode,
                            TRUE);
    }
    _SEH2_END;

    /* Set the stack pointer to the context record */
    TrapFrame->Rsp = (ULONG64)Context;

    /* We jump to KiUserApcDispatcher in ntdll */
    TrapFrame->Rip = (ULONG64)KeUserApcDispatcher;

    /* Setup Ring 3 segments */
    TrapFrame->SegCs = KGDT64_R3_CODE | RPL_MASK;
    TrapFrame->SegFs = KGDT64_R3_CMTEB | RPL_MASK;
    TrapFrame->SegGs = KGDT64_R3_DATA | RPL_MASK;
    TrapFrame->SegSs = KGDT64_R3_DATA | RPL_MASK;

    /* Sanitize EFLAGS, enable interrupts */
    TrapFrame->EFlags &= EFLAGS_USER_SANITIZE;
    TrapFrame->EFlags |= EFLAGS_INTERRUPT_MASK;
}
