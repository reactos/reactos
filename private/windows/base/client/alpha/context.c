/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    context.c

Abstract:

    This module contains the context management routines for
    Win32

Author:

    Mark Lucovsky (markl) 28-Sep-1990

Revision History:

    Thomas Van Baak (tvb) 22-Jul-1992

        Adapted for Alpha AXP.

--*/

#include "basedll.h"

VOID
BaseInitializeContext (
    OUT PCONTEXT Context,
    IN PVOID Parameter OPTIONAL,
    IN PVOID InitialPc OPTIONAL,
    IN PVOID InitialSp OPTIONAL,
    IN BASE_CONTEXT_TYPE ContextType
    )

/*++

Routine Description:

    This function initializes a context structure so that it can
    be used in a subsequent call to NtCreateThread.

Arguments:

    Context - Supplies a context buffer to be initialized by this routine.

    Parameter - Supplies the thread's parameter.

    InitialPc - Supplies an initial program counter value.

    InitialSp - Supplies an initial stack pointer value.

    NewThread - Supplies a flag that specifies that this is a new
        thread, or a new process.

Return Value:

    None.

--*/

{
    ULONG temp;
    PPEB Peb;

    Peb = NtCurrentPeb();

    //
    // Initialize the control registers.
    // So that the thread begins at BaseThreadStart
    //

    RtlZeroMemory((PVOID)Context, sizeof(CONTEXT));
    Context->IntGp = 1;
    Context->IntSp = (ULONGLONG)(LONG_PTR)InitialSp;
    Context->IntRa = 1;
    Context->ContextFlags = CONTEXT_FULL;
    if ( ContextType != BaseContextTypeProcess ) {
        if ( ContextType == BaseContextTypeThread ) {
            Context->Fir = (ULONGLONG)(LONG_PTR)BaseThreadStart;
        } else {
            Context->Fir = (ULONGLONG)(LONG_PTR)BaseFiberStart;
        }
        Context->IntA0 = (ULONGLONG)(LONG_PTR)InitialPc;
        Context->IntA1 = (ULONGLONG)(LONG_PTR)Parameter;
        Context->IntGp = (ULONGLONG)(LONG_PTR)RtlImageDirectoryEntryToData(
                           Peb->ImageBaseAddress,
                           TRUE,
                           IMAGE_DIRECTORY_ENTRY_GLOBALPTR,
                           &temp
                           );
        }
    else {
        Context->Fir = (ULONGLONG)(LONG_PTR)BaseProcessStart;
        Context->IntA0 = (ULONGLONG)(LONG_PTR)InitialPc;
        }
}

VOID
BaseFiberStart(
    VOID
    )

/*++

Routine Description:

    This function is called to start a Win32 fiber. Its purpose
    is to call BaseThreadStart, getting the necessary arguments
    from the fiber context record.

Arguments:

    None.

Return Value:

    None.

--*/

{
    PFIBER Fiber;

    Fiber = GetCurrentFiber();
    BaseThreadStart( (LPTHREAD_START_ROUTINE)Fiber->FiberContext.IntA0,
                     (LPVOID)Fiber->FiberContext.IntA1 );
}
