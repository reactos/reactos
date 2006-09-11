/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/ke/i386/userapc.c
 * PURPOSE:         Implements User-Mode APC Initialization
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* PRIVATE FUNCTIONS *********************************************************/

_SEH_DEFINE_LOCALS(KiCopyInfo)
{
    volatile EXCEPTION_RECORD SehExceptRecord;
};

_SEH_FILTER(KiCopyInformation2)
{
    _SEH_ACCESS_LOCALS(KiCopyInfo);

    /* Copy the exception records and return to the handler */
    RtlMoveMemory((PVOID)&_SEH_VAR(SehExceptRecord),
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
STDCALL
KiInitializeUserApc(IN PKEXCEPTION_FRAME ExceptionFrame,
                    IN PKTRAP_FRAME TrapFrame,
                    IN PKNORMAL_ROUTINE NormalRoutine,
                    IN PVOID NormalContext,
                    IN PVOID SystemArgument1,
                    IN PVOID SystemArgument2)
{
    CONTEXT Context;
    ULONG_PTR Stack;
    ULONG Size;
    EXCEPTION_RECORD SehExceptRecord;
    _SEH_DECLARE_LOCALS(KiCopyInfo);

    /* Don't deliver APCs in V86 mode */
    if (TrapFrame->EFlags & X86_EFLAGS_VM) return;

    /* Save the full context */
    Context.ContextFlags = CONTEXT_FULL | CONTEXT_DEBUG_REGISTERS;
    KeTrapFrameToContext(TrapFrame, ExceptionFrame, &Context);

    /* Protect with SEH */
    _SEH_TRY
    {
        /* Sanity check */
        ASSERT((TrapFrame->SegCs & MODE_MASK) != KernelMode);

        /* Get the aligned size */
        Size = ((sizeof(CONTEXT) + 3) & ~3) + 4 * sizeof(ULONG_PTR);
        Stack = (Context.Esp & ~3) - Size;

        /* Probe and copy */
        ProbeForWrite((PVOID)Stack, Size, 4);
        RtlMoveMemory((PVOID)(Stack + (4 * sizeof(ULONG_PTR))),
                      &Context,
                      sizeof(CONTEXT));

        /* Run at APC dispatcher */
        TrapFrame->Eip = (ULONG)KeUserApcDispatcher;
        TrapFrame->HardwareEsp = Stack;

        /* Setup Ring 3 state */
        TrapFrame->SegCs = KGDT_R3_CODE | RPL_MASK;
        TrapFrame->HardwareSegSs = KGDT_R3_DATA | RPL_MASK;
        TrapFrame->SegDs = KGDT_R3_DATA | RPL_MASK;
        TrapFrame->SegEs = KGDT_R3_DATA | RPL_MASK;
        TrapFrame->SegGs = 0;

        /* Sanitize EFLAGS */
        TrapFrame->EFlags = Context.EFlags & EFLAGS_USER_SANITIZE;
        TrapFrame->EFlags |= EFLAGS_INTERRUPT_MASK;

        /* Check if user-mode has IO privileges */
        if (KeGetCurrentThread()->Iopl)
        {
            /* Enable them*/
            TrapFrame->EFlags |= (0x3000);
        }

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

