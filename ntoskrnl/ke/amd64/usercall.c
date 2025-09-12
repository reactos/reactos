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
 *      with the ntdll.KiUserApcDispatcher entrypoint, copying a UAPC_FRAME
 *      structure with the context from the old trap frame to the threads user
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
    PUAPC_FRAME ApcFrame;
    PCONTEXT Context;
    EXCEPTION_RECORD ExceptionRecord;

    /* Sanity check, that the trap frame is from user mode */
    ASSERT((TrapFrame->SegCs & MODE_MASK) != KernelMode);

    /* Allocate a 16 byte aligned UAPC_FRAME structure on the user stack */
    ApcFrame = (PUAPC_FRAME)ALIGN_DOWN_POINTER_BY(TrapFrame->Rsp - sizeof(*ApcFrame), 16);
    Context = &ApcFrame->Context;

    /* Protect with SEH */
    _SEH2_TRY
    {
        /* Probe the user mode APC frame */
        ProbeForWrite(ApcFrame, sizeof(*ApcFrame), 16);

        /* Convert the current trap frame to a context */
        Context->ContextFlags = CONTEXT_FULL | CONTEXT_DEBUG_REGISTERS;
        KeTrapFrameToContext(TrapFrame, ExceptionFrame, Context);

        /* Set parameters for KiUserApcDispatcher */
        Context->P1Home = (ULONG64)NormalContext;
        Context->P2Home = (ULONG64)SystemArgument1;
        Context->P3Home = (ULONG64)SystemArgument2;
        Context->P4Home = (ULONG64)NormalRoutine;

        /* Set up the machine frame for unwinding */
        ApcFrame->MachineFrame.Rip = TrapFrame->Rip;
        ApcFrame->MachineFrame.Rsp = TrapFrame->Rsp;
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

/*
 * Stack layout for KiUserModeCallout:
 * ----------------------------------
 * KCALLOUT_FRAME.ResultLength    <= 2nd Parameter to KiCallUserMode
 * KCALLOUT_FRAME.Result          <= 1st Parameter to KiCallUserMode
 * KCALLOUT_FRAME.ReturnAddress   <= Return address of KiCallUserMode
 * KCALLOUT_FRAME.Ebp             \
 * KCALLOUT_FRAME.Ebx              | = non-volatile registers, pushed
 * KCALLOUT_FRAME.Esi              |   by KiCallUserMode
 * KCALLOUT_FRAME.Edi             /
 * KCALLOUT_FRAME.CallbackStack
 * KCALLOUT_FRAME.TrapFrame
 * KCALLOUT_FRAME.InitialStack    <= CalloutFrame points here
 * ----------------------------------
 * ~~ optional alignment ~~
 * ----------------------------------
 * FX_SAVE_AREA
 * ----------------------------------
 * KTRAP_FRAME
 * ----------------------------------
 * ~~ begin of stack frame for KiUserModeCallout ~~
 *
 */
NTSTATUS
FASTCALL
KiUserModeCallout(
    _Out_ PKCALLOUT_FRAME CalloutFrame)
{
    PKTHREAD CurrentThread;
    PKTRAP_FRAME TrapFrame;
    KTRAP_FRAME CallbackTrapFrame;
    PKIPCR Pcr;
    ULONG_PTR InitialStack;
    NTSTATUS Status;

    /* Get the current thread */
    CurrentThread = KeGetCurrentThread();

    /* Check if we are at pasive level */
    ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);

    /* Check if we are attached or APCs are disabled */
    ASSERT((CurrentThread->ApcStateIndex == OriginalApcEnvironment) &&
        (CurrentThread->CombinedApcDisable == 0));

    /* Align stack on a 16-byte boundary */
    InitialStack = (ULONG_PTR)ALIGN_DOWN_POINTER_BY(CalloutFrame, 16);

    /* Check if we have enough space on the stack */
    if ((InitialStack - KERNEL_STACK_SIZE) < CurrentThread->StackLimit)
    {
        /* We don't, we'll have to grow our stack */
        Status = MmGrowKernelStack((PVOID)InitialStack);

        /* Quit if we failed */
        if (!NT_SUCCESS(Status)) return Status;
    }

    /* Save the current callback stack and initial stack */
    CalloutFrame->CallbackStack = (ULONG_PTR)CurrentThread->CallbackStack;
    CalloutFrame->InitialStack = (ULONG_PTR)CurrentThread->InitialStack;

    /* Get and save the trap frame */
    TrapFrame = CurrentThread->TrapFrame;
    CalloutFrame->TrapFrame = (ULONG_PTR)TrapFrame;

    /* Set the new callback stack */
    CurrentThread->CallbackStack = CalloutFrame;

    /* Disable interrupts so we can fill the NPX State */
    _disable();

    /* Set the stack address */
    CurrentThread->InitialStack = (PVOID)InitialStack;

    /* Copy the trap frame to the new location */
    CallbackTrapFrame = *TrapFrame;

    /* Get PCR */
    Pcr = (PKIPCR)KeGetPcr();

    /* Set user-mode dispatcher address as EIP */
    Pcr->TssBase->Rsp0 = InitialStack;
    Pcr->Prcb.RspBase = InitialStack;
    CallbackTrapFrame.Rip = (ULONG_PTR)KeUserCallbackDispatcher;

    /* Bring interrupts back */
    _enable();

    /* Exit to user-mode */
    KiUserCallbackExit(&CallbackTrapFrame);
}

VOID
KiSetupUserCalloutFrame(
    _Out_ PUCALLOUT_FRAME UserCalloutFrame,
    _In_ PKTRAP_FRAME TrapFrame,
    _In_ ULONG ApiNumber,
    _In_ PVOID Buffer,
    _In_ ULONG BufferLength)
{
#ifdef _M_IX86
    CalloutFrame->Reserved = 0;
    CalloutFrame->ApiNumber = ApiNumber;
    CalloutFrame->Buffer = (ULONG_PTR)NewStack;
    CalloutFrame->Length = ArgumentLength;
#elif defined(_M_AMD64)
    UserCalloutFrame->Buffer = (PVOID)(UserCalloutFrame + 1);
    UserCalloutFrame->Length = BufferLength;
    UserCalloutFrame->ApiNumber = ApiNumber;
    UserCalloutFrame->MachineFrame.Rip = TrapFrame->Rip;
    UserCalloutFrame->MachineFrame.Rsp = TrapFrame->Rsp;
#else
#error "KiSetupUserCalloutFrame not implemented!"
#endif
}

NTSTATUS
NTAPI
KeUserModeCallback(
    IN ULONG RoutineIndex,
    IN PVOID Argument,
    IN ULONG ArgumentLength,
    OUT PVOID *Result,
    OUT PULONG ResultLength)
{
    ULONG_PTR OldStack;
    PUCHAR UserArguments;
    PUCALLOUT_FRAME CalloutFrame;
    PULONG_PTR UserStackPointer;
    NTSTATUS CallbackStatus;
#ifdef _M_IX86
    PEXCEPTION_REGISTRATION_RECORD ExceptionList;
#endif // _M_IX86
    PTEB Teb;
    ULONG GdiBatchCount = 0;
    ASSERT(KeGetCurrentThread()->ApcState.KernelApcInProgress == FALSE);
    ASSERT(KeGetPreviousMode() == UserMode);

    /* Get the current user-mode stack */
    UserStackPointer = KiGetUserModeStackAddress();
    OldStack = *UserStackPointer;

    /* Enter a SEH Block */
    _SEH2_TRY
    {
        /* Calculate and align the stack. This is unaligned by 8 bytes, since the following
           UCALLOUT_FRAME compensates for that and on entry we already have a full stack
           frame with home space for the next call, i.e. we are already inside the function
           body and the stack needs to be 16 byte aligned. */
        UserArguments = (PUCHAR)ALIGN_DOWN_POINTER_BY(OldStack - ArgumentLength, 16) - 8;

        /* The callout frame is below the arguments */
        CalloutFrame = ((PUCALLOUT_FRAME)UserArguments) - 1;

        /* Make sure it's all writable */
        ProbeForWrite(CalloutFrame,
                      sizeof(PUCALLOUT_FRAME) + ArgumentLength,
                      sizeof(PVOID));

        /* Copy the buffer into the stack */
        RtlCopyMemory(UserArguments, Argument, ArgumentLength);

        /* Write the arguments */
        KiSetupUserCalloutFrame(CalloutFrame,
                                KeGetCurrentThread()->TrapFrame,
                                RoutineIndex,
                                UserArguments,
                                ArgumentLength);

        /* Save the exception list */
        Teb = KeGetCurrentThread()->Teb;
#ifdef _M_IX86
        ExceptionList = Teb->NtTib.ExceptionList;
#endif // _M_IX86

        /* Jump to user mode */
        *UserStackPointer = (ULONG_PTR)CalloutFrame;
        CallbackStatus = KiCallUserMode(Result, ResultLength);
        if (CallbackStatus != STATUS_CALLBACK_POP_STACK)
        {
#ifdef _M_IX86
            /* Only restore the exception list if we didn't crash in ring 3 */
            Teb->NtTib.ExceptionList = ExceptionList;
#endif // _M_IX86
        }
        else
        {
            /* Otherwise, pop the stack */
            OldStack = *UserStackPointer;
        }

        /* Read the GDI Batch count */
        GdiBatchCount = Teb->GdiBatchCount;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        /* Get the SEH exception */
        _SEH2_YIELD(return _SEH2_GetExceptionCode());
    }
    _SEH2_END;

    /* Check if we have GDI Batch operations */
    if (GdiBatchCount)
    {
        *UserStackPointer -= 256;
        KeGdiFlushUserBatch();
    }

    /* Restore stack and return */
    *UserStackPointer = OldStack;
    return CallbackStatus;
}

NTSTATUS
NTAPI
NtCallbackReturn(
    _In_ PVOID Result,
    _In_ ULONG ResultLength,
    _In_ NTSTATUS CallbackStatus)
{
    PKTHREAD CurrentThread;
    PKCALLOUT_FRAME CalloutFrame;
    PKTRAP_FRAME CallbackTrapFrame, TrapFrame;
    PKIPCR Pcr;

    /* Get the current thread and make sure we have a callback stack */
    CurrentThread = KeGetCurrentThread();
    CalloutFrame = CurrentThread->CallbackStack;
    if (CalloutFrame == NULL)
    {
        return STATUS_NO_CALLBACK_ACTIVE;
    }

    /* Store the results in the callback stack */
    *((PVOID*)CalloutFrame->OutputBuffer) = Result;
    *((ULONG*)CalloutFrame->OutputLength) = ResultLength;

    /* Get the trap frame */
    CallbackTrapFrame = CurrentThread->TrapFrame;

    /* Disable interrupts for NPX save and stack switch */
    _disable();

    /* Restore the exception list */
    Pcr = (PKIPCR)KeGetPcr();

    /* Get the previous trap frame */
    TrapFrame = (PKTRAP_FRAME)CalloutFrame->TrapFrame;

    /* Check if we failed in user mode */
    if (CallbackStatus == STATUS_CALLBACK_POP_STACK)
    {
        *TrapFrame = *CallbackTrapFrame;
    }

    /* Clear DR7 */
    TrapFrame->Dr7 = 0;

    /* Check if debugging was active */
    if (CurrentThread->Header.DebugActive & 0xFF)
    {
        /* Copy debug registers data from it */
        TrapFrame->Dr0 = CallbackTrapFrame->Dr0;
        TrapFrame->Dr1 = CallbackTrapFrame->Dr1;
        TrapFrame->Dr2 = CallbackTrapFrame->Dr2;
        TrapFrame->Dr3 = CallbackTrapFrame->Dr3;
        TrapFrame->Dr6 = CallbackTrapFrame->Dr6;
        TrapFrame->Dr7 = CallbackTrapFrame->Dr7;
    }

    /* Switch the stack back to the previous value */
    Pcr->TssBase->Rsp0 = CalloutFrame->InitialStack;
    Pcr->Prcb.RspBase = CalloutFrame->InitialStack;

    /* Get the initial stack and restore it */
    CurrentThread->InitialStack = (PVOID)CalloutFrame->InitialStack;

    /* Restore the trap frame and the previous callback stack */
    CurrentThread->TrapFrame = TrapFrame;
    CurrentThread->CallbackStack = (PVOID)CalloutFrame->CallbackStack;

    /* Bring interrupts back */
    _enable();

    /* Now switch back to the old stack */
    KiCallbackReturn(CalloutFrame, CallbackStatus);
}

