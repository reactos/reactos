/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ke/usercall.c
 * PURPOSE:         User-Mode callbacks. Portable part.
 * PROGRAMMERS:     Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

NTSTATUS
STDCALL
KiCallUserMode(
    IN PVOID *OutputBuffer,
    IN PULONG OutputLength
);

PULONG
STDCALL
KiGetUserModeStackAddress(
    VOID
);

/* FUNCTIONS *****************************************************************/

/*
 * @implemented
 */
NTSTATUS
STDCALL
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
    DPRINT("KeUserModeCallback(RoutineIndex %d, Argument %X, ArgumentLength %d)\n",
            RoutineIndex, Argument, ArgumentLength);
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
        ExceptionList = KeGetCurrentThread()->Teb->Tib.ExceptionList;

        /* Jump to user mode */
        *UserEsp = NewStack;
        CallbackStatus = KiCallUserMode(Result, ResultLength);

        /* FIXME: Handle user-mode exception status */

        /* Restore exception list */
        KeGetCurrentThread()->Teb->Tib.ExceptionList = ExceptionList;
    }
    _SEH_HANDLE
    {
        CallbackStatus = _SEH_GetExceptionCode();
    }
    _SEH_END;

    /* FIXME: Flush GDI Batch */

    /* Restore stack and return */
    *UserEsp = OldStack;
    return CallbackStatus;
}

/* EOF */
