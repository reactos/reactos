/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/ke/i386/usercall.c
 * PURPOSE:         User-mode Callout Mechanisms (APC and Win32K Callbacks)
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 *                  Timo Kreuzer (timo.kreuzer@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

extern PGDI_BATCHFLUSH_ROUTINE KeGdiFlushUserBatch;

/* PRIVATE FUNCTIONS *********************************************************/

/*++
 * @name KiInitializeUserApc
 *
 *     Prepares the Context for a User-Mode APC called through NTDLL.DLL
 *
 * @param Reserved
 *        Pointer to the Exception Frame on non-i386 builds.
 *
 * @param TrapFrame
 *        Pointer to the Trap Frame.
 *
 * @param NormalRoutine
 *        Pointer to the NormalRoutine to call.
 *
 * @param NormalContext
 *        Pointer to the context to send to the Normal Routine.
 *
 * @param SystemArgument[1-2]
 *        Pointer to a set of two parameters that contain untyped data.
 *
 * @return None.
 *
 * @remarks None.
 *
 *--*/
VOID
NTAPI
KiInitializeUserApc(IN PKEXCEPTION_FRAME ExceptionFrame,
                    IN PKTRAP_FRAME TrapFrame,
                    IN PKNORMAL_ROUTINE NormalRoutine,
                    IN PVOID NormalContext,
                    IN PVOID SystemArgument1,
                    IN PVOID SystemArgument2)
{
    CONTEXT Context;
    ULONG_PTR Stack, AlignedEsp;
    ULONG ContextLength;
    EXCEPTION_RECORD SehExceptRecord;

    /* Don't deliver APCs in V86 mode */
    if (TrapFrame->EFlags & EFLAGS_V86_MASK) return;

    /* Save the full context */
    Context.ContextFlags = CONTEXT_FULL | CONTEXT_DEBUG_REGISTERS;
    KeTrapFrameToContext(TrapFrame, ExceptionFrame, &Context);

    /* Protect with SEH */
    _SEH2_TRY
    {
        /* Sanity check */
        ASSERT((TrapFrame->SegCs & MODE_MASK) != KernelMode);

        /* Get the aligned size */
        AlignedEsp = Context.Esp & ~3;
        ContextLength = CONTEXT_ALIGNED_SIZE + (4 * sizeof(ULONG_PTR));
        Stack = ((AlignedEsp - 8) & ~3) - ContextLength;

        /* Probe the stack */
        ProbeForWrite((PVOID)Stack, AlignedEsp - Stack, 1);
        ASSERT(!(Stack & 3));

        /* Copy data into it */
        RtlCopyMemory((PVOID)(Stack + (4 * sizeof(ULONG_PTR))),
                      &Context,
                      sizeof(CONTEXT));

        /* Run at APC dispatcher */
        TrapFrame->Eip = (ULONG)KeUserApcDispatcher;
        TrapFrame->HardwareEsp = Stack;

        /* Setup Ring 3 state */
        TrapFrame->SegCs = Ke386SanitizeSeg(KGDT_R3_CODE, UserMode);
        TrapFrame->HardwareSegSs = Ke386SanitizeSeg(KGDT_R3_DATA, UserMode);
        TrapFrame->SegDs = Ke386SanitizeSeg(KGDT_R3_DATA, UserMode);
        TrapFrame->SegEs = Ke386SanitizeSeg(KGDT_R3_DATA, UserMode);
        TrapFrame->SegFs = Ke386SanitizeSeg(KGDT_R3_TEB, UserMode);
        TrapFrame->SegGs = 0;
        TrapFrame->ErrCode = 0;

        /* Sanitize EFLAGS */
        TrapFrame->EFlags = Ke386SanitizeFlags(Context.EFlags, UserMode);

        /* Check if thread has IOPL and force it enabled if so */
        if (KeGetCurrentThread()->Iopl) TrapFrame->EFlags |= EFLAGS_IOPL;

        /* Setup the stack */
        *(PULONG_PTR)(Stack + 0 * sizeof(ULONG_PTR)) = (ULONG_PTR)NormalRoutine;
        *(PULONG_PTR)(Stack + 1 * sizeof(ULONG_PTR)) = (ULONG_PTR)NormalContext;
        *(PULONG_PTR)(Stack + 2 * sizeof(ULONG_PTR)) = (ULONG_PTR)SystemArgument1;
        *(PULONG_PTR)(Stack + 3 * sizeof(ULONG_PTR)) = (ULONG_PTR)SystemArgument2;
    }
    _SEH2_EXCEPT((RtlCopyMemory(&SehExceptRecord, _SEH2_GetExceptionInformation()->ExceptionRecord, sizeof(EXCEPTION_RECORD)), EXCEPTION_EXECUTE_HANDLER))
    {
        /* Dispatch the exception */
        SehExceptRecord.ExceptionAddress = (PVOID)TrapFrame->Eip;
        KiDispatchException(&SehExceptRecord,
                            ExceptionFrame,
                            TrapFrame,
                            UserMode,
                            TRUE);
    }
    _SEH2_END;
}

/* PUBLIC FUNCTIONS **********************************************************/

/*
 * @implemented
 */
NTSTATUS
NTAPI
KeUserModeCallback(IN ULONG RoutineIndex,
                   IN PVOID Argument,
                   IN ULONG ArgumentLength,
                   OUT PVOID *Result,
                   OUT PULONG ResultLength)
{
    ULONG_PTR NewStack, OldStack;
    PULONG UserEsp;
    NTSTATUS CallbackStatus;
    PEXCEPTION_REGISTRATION_RECORD ExceptionList;
    PTEB Teb;
    ULONG GdiBatchCount = 0;
    ASSERT(KeGetCurrentThread()->ApcState.KernelApcInProgress == FALSE);
    ASSERT(KeGetPreviousMode() == UserMode);

    /* Get the current user-mode stack */
    UserEsp = KiGetUserModeStackAddress();
    OldStack = *UserEsp;

    /* Enter a SEH Block */
    _SEH2_TRY
    {
        /* Calculate and align the stack size */
        NewStack = (OldStack - ArgumentLength) & ~3;

        /* Make sure it's writable */
        ProbeForWrite((PVOID)(NewStack - 6 * sizeof(ULONG_PTR)),
                      ArgumentLength + 6 * sizeof(ULONG_PTR),
                      sizeof(CHAR));

        /* Copy the buffer into the stack */
        RtlCopyMemory((PVOID)NewStack, Argument, ArgumentLength);

        /* Write the arguments */
        NewStack -= 24;
        *(PULONG)NewStack = 0;
        *(PULONG)(NewStack + 4) = RoutineIndex;
        *(PULONG)(NewStack + 8) = (NewStack + 24);
        *(PULONG)(NewStack + 12) = ArgumentLength;

        /* Save the exception list */
        Teb = KeGetCurrentThread()->Teb;
        ExceptionList = Teb->NtTib.ExceptionList;

        /* Jump to user mode */
        *UserEsp = NewStack;
        CallbackStatus = KiCallUserMode(Result, ResultLength);
        if (CallbackStatus != STATUS_CALLBACK_POP_STACK)
        {
            /* Only restore the exception list if we didn't crash in ring 3 */
            Teb->NtTib.ExceptionList = ExceptionList;
            CallbackStatus = STATUS_SUCCESS;
        }
        else
        {
            /* Otherwise, pop the stack */
            OldStack = *UserEsp;
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
          *UserEsp -= 256;
          KeGdiFlushUserBatch();
    }

    /* Restore stack and return */
    *UserEsp = OldStack;
    return CallbackStatus;
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
KiUserModeCallout(PKCALLOUT_FRAME CalloutFrame)
{
    PKTHREAD CurrentThread;
    PKTRAP_FRAME TrapFrame, CallbackTrapFrame;
    PFX_SAVE_AREA FxSaveArea, OldFxSaveArea;
    PKPCR Pcr;
    PKTSS Tss;
    ULONG_PTR InitialStack;
    NTSTATUS Status;

    /* Get the current thread */
    CurrentThread = KeGetCurrentThread();

#if DBG
    /* Check if we are at pasive level */
    if (KeGetCurrentIrql() != PASSIVE_LEVEL)
    {
        /* We're not, bugcheck */
        KeBugCheckEx(IRQL_GT_ZERO_AT_SYSTEM_SERVICE,
                     0,
                     KeGetCurrentIrql(),
                     0,
                     0);
    }

    /* Check if we are attached or APCs are disabled */
    if ((CurrentThread->ApcStateIndex != OriginalApcEnvironment) ||
        (CurrentThread->CombinedApcDisable > 0))
    {
        KeBugCheckEx(APC_INDEX_MISMATCH,
                     0,
                     CurrentThread->ApcStateIndex,
                     CurrentThread->CombinedApcDisable,
                     0);
    }
#endif

    /* Align stack on a 16-byte boundary */
    InitialStack = ALIGN_DOWN_BY(CalloutFrame, 16);

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

    /* Set destination and origin NPX Areas */
    OldFxSaveArea = (PVOID)(CalloutFrame->InitialStack - sizeof(FX_SAVE_AREA));
    FxSaveArea = (PVOID)(InitialStack - sizeof(FX_SAVE_AREA));

    /* Disable interrupts so we can fill the NPX State */
    _disable();

    /* Now copy the NPX State */
    FxSaveArea->U.FnArea.ControlWord = OldFxSaveArea->U.FnArea.ControlWord;
    FxSaveArea->U.FnArea.StatusWord = OldFxSaveArea->U.FnArea.StatusWord;
    FxSaveArea->U.FnArea.TagWord = OldFxSaveArea->U.FnArea.TagWord;
    FxSaveArea->U.FnArea.DataSelector = OldFxSaveArea->U.FnArea.DataSelector;
    FxSaveArea->Cr0NpxState = OldFxSaveArea->Cr0NpxState;

    /* Set the stack address */
    CurrentThread->InitialStack = (PVOID)InitialStack;

    /* Locate the trap frame on the callback stack */
    CallbackTrapFrame = (PVOID)((ULONG_PTR)FxSaveArea - sizeof(KTRAP_FRAME));

    /* Copy the trap frame to the new location */
    *CallbackTrapFrame = *TrapFrame;

    /* Get PCR */
    Pcr = KeGetPcr();

    /* Update the exception list */
    CallbackTrapFrame->ExceptionList = Pcr->NtTib.ExceptionList;

    /* Get TSS */
    Tss = Pcr->TSS;

    /* Check for V86 mode */
    if (CallbackTrapFrame->EFlags & EFLAGS_V86_MASK)
    {
        /* Set new stack address in TSS (full trap frame) */
        Tss->Esp0 = (ULONG_PTR)(CallbackTrapFrame + 1);
    }
    else
    {
        /* Set new stack address in TSS (non-V86 trap frame) */
        Tss->Esp0 = (ULONG_PTR)&CallbackTrapFrame->V86Es;
    }

    /* Set user-mode dispatcher address as EIP */
    CallbackTrapFrame->Eip = (ULONG_PTR)KeUserCallbackDispatcher;

    /* Bring interrupts back */
    _enable();

    /* Exit to user-mode */
    KiServiceExit(CallbackTrapFrame, 0);
}


/* EOF */
