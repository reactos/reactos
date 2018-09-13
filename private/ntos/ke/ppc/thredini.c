/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    thredini.c

Abstract:

    This module implements the machine dependent functions to set the initial
    context and data alignment handling mode of a thread object.

Author:

    David N. Cutler (davec) 1-Apr-1990

Environment:

    Kernel mode only.

Revision History:

    Sep 19, 1993    plj    Conversion to IBM PowerPC

--*/

#include "ki.h"

//
// The following assert macros are used to check that an input object is
// really the proper type.
//

#define ASSERT_PROCESS(E) {                    \
    ASSERT((E)->Header.Type == ProcessObject); \
}

#define ASSERT_THREAD(E) {                    \
    ASSERT((E)->Header.Type == ThreadObject); \
}


VOID
KiInitializeContextThread (
    IN PKTHREAD Thread,
    IN PKSYSTEM_ROUTINE SystemRoutine,
    IN PKSTART_ROUTINE StartRoutine OPTIONAL,
    IN PVOID StartContext OPTIONAL,
    IN PCONTEXT ContextRecord OPTIONAL
    )

/*++

Routine Description:

    This function initializes the machine dependent context of a thread object.

    Actually, what it does is to lay out the stack for the thread so that
    it contains a stack frame that will be picked up by SwapContext and
    returned thru, resulting in a transfer of control to KiThreadStartup.
    In otherwords, we lay out a stack with a stack frame that looks as if
    SwapContext had been called just before the first instruction in
    KiThreadStartup.

    N.B. This function does not check the accessibility of the context record.
         It is assumed the the caller of this routine is either prepared to
         handle access violations or has probed and copied the context record
         as appropriate.

    N.B. Arguments to the new thread are passed in the Swap Frame gprs
         16 thru 20 which will be loaded into the thread's gprs when
         execution begins.

    WARNING: If the thread has a user mode context the Top of stack MUST
             be laid out identically to the top of stack laid out when an
             interrupt or system call from user mode occurs.

Arguments:

    Thread - Supplies a pointer to a dispatcher object of type thread.

    SystemRoutine - Supplies a pointer to the system function that is to be
        called when the thread is first scheduled for execution.

    StartRoutine - Supplies an optional pointer to a function that is to be
        called after the system has finished initializing the thread. This
        parameter is specified if the thread is a system thread and will
        execute totally in kernel mode.

    StartContext - Supplies an optional pointer to an arbitrary data structure
        which will be passed to the StartRoutine as a parameter. This
        parameter is specified if the thread is a system thread and will
        execute totally in kernel mode.

    ContextRecord - Supplies an optional pointer a context frame which contains
        the initial user mode state of the thread. This parameter is specified
        if the thread is a user thread and will execute in user mode. If this
        parameter is not specified, then the Teb parameter is ignored.

Return Value:

    None.

--*/

{

    PKEXCEPTION_FRAME ExFrame;
    ULONG InitialStack;
    PKTRAP_FRAME TrFrame;
    PKSWAP_FRAME SwFrame;
    PSTACK_FRAME_HEADER StkFrame, ResumeFrame;

    //
    // If the initial stack address is in KSEG0, then the stack is not mapped
    // and the kernel stack PTEs are set to zero. Otherwise, capture the PTEs
    // that map the kernel stack.
    //

    InitialStack = (LONG)Thread->InitialStack;

    //
    // If a context frame is specified, then initialize a trap frame and
    // and an exception frame with the specified user mode context.
    //

    if (ARGUMENT_PRESENT(ContextRecord)) {
        ExFrame = (PKEXCEPTION_FRAME)(InitialStack - 8 -
                  sizeof(KEXCEPTION_FRAME));
        TrFrame = (PKTRAP_FRAME)((ULONG)ExFrame -  sizeof(KTRAP_FRAME));
        StkFrame = (PSTACK_FRAME_HEADER)((ULONG)TrFrame - (8 * sizeof(ULONG)) -
                  sizeof(STACK_FRAME_HEADER));

        //
        // Zero the exception and trap frames and copy information from the
        // specified context frame to the trap and exception frames.
        //

        RtlZeroMemory((PVOID)ExFrame, sizeof(KEXCEPTION_FRAME));
        RtlZeroMemory((PVOID)TrFrame, sizeof(KTRAP_FRAME));
        KeContextToKframes(TrFrame, ExFrame,
                           ContextRecord,
                           ContextRecord->ContextFlags | CONTEXT_CONTROL,
                           UserMode);

        //
        // Set the saved previous processor mode in the trap frame and the
        // previous processor mode in the thread object to user mode.
        //

        TrFrame->PreviousMode = UserMode;
        Thread->PreviousMode = UserMode;

    } else {

        StkFrame = (PSTACK_FRAME_HEADER)((InitialStack -
                  sizeof(STACK_FRAME_HEADER)) & 0xfffffff0);
        ExFrame = NULL;
        TrFrame = NULL;

        //
        // Set the previous mode in thread object to kernel.
        //

        Thread->PreviousMode = KernelMode;
    }

    StkFrame->BackChain = (ULONG)0;

    //
    // Initialize the Swap Frame that swap context will use to
    // initiate execution of this thread.
    //

    SwFrame = (PKSWAP_FRAME)(((ULONG)StkFrame -
              sizeof(KSWAP_FRAME)) & 0xfffffff0);

    //
    // Initialize stack frame and set thread start up parameters.
    //

    if (ExFrame == NULL) {
        SwFrame->ExceptionFrame.Gpr20 = (ULONG)ExFrame;
    } else {
        SwFrame->ExceptionFrame.Gpr20 = (ULONG)TrFrame;
    }

    SwFrame->ExceptionFrame.Gpr16 = (ULONG)ContextRecord;
    SwFrame->ExceptionFrame.Gpr17 = (ULONG)StartContext;
    SwFrame->ExceptionFrame.Gpr18 = (ULONG)StartRoutine;

    //
    // Pass the actual entry point addresses rather than the function
    // descriptor's address.
    //

    SwFrame->ExceptionFrame.Gpr19 = *(ULONG *)SystemRoutine;

    SwFrame->SwapReturn = *(ULONG *)KiThreadStartup;
    SwFrame->ConditionRegister = 0;

    //
    // Buy a stack frame so we have the stack frame that will be
    // current when SwapContext is running when we resume this
    // thread.


    ResumeFrame = ((PSTACK_FRAME_HEADER)SwFrame) - 1;

    ResumeFrame->BackChain = (ULONG)StkFrame;

    //
    // Set the initial kernel stack pointer.
    //

    Thread->KernelStack = (PVOID)ResumeFrame;
    ASSERT(!((ULONG)ResumeFrame & 0x7));        // ensure stack is align 8
    return;
}

BOOLEAN
KeSetAutoAlignmentProcess (
    IN PKPROCESS Process,
    IN BOOLEAN Enable
    )

/*++

Routine Description:

    This function sets the data alignment handling mode for the specified
    process and returns the previous data alignment handling mode.

Arguments:

    Process  - Supplies a pointer to a dispatcher object of type process.

    Enable - Supplies a boolean value that determines the handling of data
        alignment exceptions for the process. A value of TRUE causes all
        data alignment exceptions to be automatically handled by the kernel.
        A value of FALSE causes all data alignment exceptions to be actually
        raised as exceptions.

Return Value:

    A value of TRUE is returned if data alignment exceptions were
    previously automatically handled by the kernel. Otherwise, a value
    of FALSE is returned.

--*/

{

    KIRQL OldIrql;
    BOOLEAN Previous;

    ASSERT_PROCESS(Process);

    //
    // Raise IRQL to dispatcher level and lock dispatcher database.
    //

    KiLockDispatcherDatabase(&OldIrql);

    //
    // Capture the previous data alignment handling mode and set the
    // specified data alignment mode.
    //

    Previous = Process->AutoAlignment;
    Process->AutoAlignment = Enable;

    //
    // Unlock dispatcher database, lower IRQL to its previous value, and
    // return the previous data alignment mode.
    //

    KiUnlockDispatcherDatabase(OldIrql);
    return Previous;
}

BOOLEAN
KeSetAutoAlignmentThread (
    IN PKTHREAD Thread,
    IN BOOLEAN Enable
    )

/*++

Routine Description:

    This function sets the data alignment handling mode for the specified
    thread and returns the previous data alignment handling mode.

Arguments:

    Thread  - Supplies a pointer to a dispatcher object of type thread.

    Enable - Supplies a boolean value that determines the handling of data
        alignment exceptions for the thread. A value of TRUE causes all
        data alignment exceptions to be automatically handled by the kernel.
        A value of FALSE causes all data alignment exceptions to be actually
        raised as exceptions.

Return Value:

    A value of TRUE is returned if data alignment exceptions were
    previously automatically handled by the kernel. Otherwise, a value
    of FALSE is returned.

--*/

{

    KIRQL OldIrql;
    BOOLEAN Previous;

    ASSERT_THREAD( Thread );

    //
    // Raise IRQL to dispatcher level and lock dispatcher database.
    //

    KiLockDispatcherDatabase(&OldIrql);

    //
    // Capture the previous data alignment handling mode and set the
    // specified data alignment mode.
    //

    Previous = Thread->AutoAlignment;
    Thread->AutoAlignment = Enable;

    //
    // Unlock dispatcher database, lower IRQL to its previous value, and
    // return the previous data alignment mode.
    //

    KiUnlockDispatcherDatabase(OldIrql);
    return Previous;
}
