/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/ke/i386/usercall.c
 * PURPOSE:         User-mode Callout Mechanisms (APC and Win32K Callbacks)
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/* PRIVATE FUNCTIONS *********************************************************/

_SEH_DEFINE_LOCALS(KiCopyInfo)
{
    volatile EXCEPTION_RECORD SehExceptRecord;
};

_SEH_FILTER(KiCopyInformation2)
{
    _SEH_ACCESS_LOCALS(KiCopyInfo);

    /* Copy the exception records and return to the handler */
    RtlCopyMemory((PVOID)&_SEH_VAR(SehExceptRecord),
                  _SEH_GetExceptionPointers()->ExceptionRecord,
                  sizeof(EXCEPTION_RECORD));
    return EXCEPTION_EXECUTE_HANDLER;
}

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
    _SEH_DECLARE_LOCALS(KiCopyInfo);

    /* Don't deliver APCs in V86 mode */
    if (TrapFrame->EFlags & EFLAGS_V86_MASK) return;

    /* Save the full context */
    Context.ContextFlags = CONTEXT_FULL | CONTEXT_DEBUG_REGISTERS;
    KeTrapFrameToContext(TrapFrame, ExceptionFrame, &Context);

    /* Protect with SEH */
    _SEH_TRY
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
        if (KeGetCurrentThread()->Iopl) TrapFrame->EFlags |= 0x3000;

        /* Setup the stack */
        *(PULONG_PTR)(Stack + 0 * sizeof(ULONG_PTR)) = (ULONG_PTR)NormalRoutine;
        *(PULONG_PTR)(Stack + 1 * sizeof(ULONG_PTR)) = (ULONG_PTR)NormalContext;
        *(PULONG_PTR)(Stack + 2 * sizeof(ULONG_PTR)) = (ULONG_PTR)SystemArgument1;
        *(PULONG_PTR)(Stack + 3 * sizeof(ULONG_PTR)) = (ULONG_PTR)SystemArgument2;
    }
    _SEH_EXCEPT(KiCopyInformation2)
    {
        /* Dispatch the exception */
        _SEH_VAR(SehExceptRecord).ExceptionAddress = (PVOID)TrapFrame->Eip;
        KiDispatchException(&SehExceptRecord,
                            ExceptionFrame,
                            TrapFrame,
                            UserMode,
                            TRUE);
    }
    _SEH_END;
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
    NTSTATUS CallbackStatus = STATUS_SUCCESS;
    PEXCEPTION_REGISTRATION_RECORD ExceptionList;
    PTEB Teb;
    ULONG GdiBatchCount = 0;
    ASSERT(KeGetCurrentThread()->ApcState.KernelApcInProgress == FALSE);
    ASSERT(KeGetPreviousMode() == UserMode);

    /* Get the current user-mode stack */
    UserEsp = KiGetUserModeStackAddress();
    OldStack = *UserEsp;

    /* Enter a SEH Block */
    _SEH_TRY
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
        ExceptionList = Teb->Tib.ExceptionList;

        /* Jump to user mode */
        *UserEsp = NewStack;
        CallbackStatus = KiCallUserMode(Result, ResultLength);
        if (CallbackStatus != STATUS_CALLBACK_POP_STACK)
        {
            /* Only restore the exception list if we didn't crash in ring 3 */
            Teb->Tib.ExceptionList = ExceptionList;
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
    _SEH_HANDLE
    {
        /* Get the SEH exception */
        CallbackStatus = _SEH_GetExceptionCode();
    }
    _SEH_END;
    if (!NT_SUCCESS(CallbackStatus)) return CallbackStatus;

    /* Check if we have GDI Batch operations */
    if (GdiBatchCount)
    {
        /* Shouldn't happen in ROS yet */
        ASSERT(FALSE);
    }

    /* Restore stack and return */
    *UserEsp = OldStack;
    return CallbackStatus;
}

/* EOF */
