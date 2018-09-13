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

--*/

#include "basedll.h"

extern PVOID BasepLockPrefixTable;

//
// Specify address of kernel32 lock prefixes
//
IMAGE_LOAD_CONFIG_DIRECTORY _load_config_used = {
    0,                              // Reserved
    0,                              // Reserved
    0,                              // Reserved
    0,                              // Reserved
    0,                              // GlobalFlagsClear
    0,                              // GlobalFlagsSet
    0,                              // CriticalSectionTimeout (milliseconds)
    0,                              // DeCommitFreeBlockThreshold
    0,                              // DeCommitTotalFreeThreshold
    (ULONG) &BasepLockPrefixTable,  // LockPrefixTable
    0, 0, 0, 0, 0, 0, 0             // Reserved
};

VOID
BaseInitializeContext(
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

    Raises STATUS_BAD_INITIAL_STACK if the value of InitialSp is not properly
           aligned.

    Raises STATUS_BAD_INITIAL_PC if the value of InitialPc is not properly
           aligned.

--*/

{

    Context->Eax = (ULONG)InitialPc;
    Context->Ebx = (ULONG)Parameter;

    Context->SegGs = 0;
    Context->SegFs = KGDT_R3_TEB;
    Context->SegEs = KGDT_R3_DATA;
    Context->SegDs = KGDT_R3_DATA;
    Context->SegSs = KGDT_R3_DATA;
    Context->SegCs = KGDT_R3_CODE;

    //
    // Start the thread at IOPL=3.
    //

    Context->EFlags = 0x3000;

    //
    // Always start the thread at the thread start thunk.
    //

    Context->Esp = (ULONG) InitialSp;

    if ( ContextType == BaseContextTypeThread ) {
        Context->Eip = (ULONG) BaseThreadStartThunk;
        }
    else if ( ContextType == BaseContextTypeFiber ) {
        Context->Eip = (ULONG) BaseFiberStart;
        }
    else {
        Context->Eip = (ULONG) BaseProcessStartThunk;
        }
    //
    // add code to check alignment and raise exception...
    //

    Context->ContextFlags = CONTEXT_FULL;
    Context->Esp -= sizeof(Parameter); // Reserve room for ret address
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
    BaseThreadStart( (LPTHREAD_START_ROUTINE)Fiber->FiberContext.Eax,
                     (LPVOID)Fiber->FiberContext.Ebx );
}
