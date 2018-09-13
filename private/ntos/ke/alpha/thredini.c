/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    thredini.c

Abstract:

    This module implements the machine dependent functions to set the initial
    context and data alignment handling mode for a process or thread object.

Author:

    David N. Cutler (davec) 1-Apr-1990

Environment:

    Kernel mode only.

Revision History:

    Joe Notarangelo  21-Apr-1992
        very minor changes for ALPHA
	     1. psr definition
	     2. pte and mask (from 3ffffc to 1ffffc), mips page size is 4k
	            our first alpha page size is 8k
		    mips code shifts right 10 (12-2) and then turns off the
		    upper 10 bits, alpha shifts right 11 (13-2) and so must
		    turn off upper 11 bits
	     3. Insert register values into context structure as quadwords
		    to insure that the written values are in canonical form

    Thomas Van Baak (tvb) 9-Oct-1992

        Adapted for Alpha AXP.

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

    N.B. This function does not check the accessibility of the context record.
         It is assumed the the caller of this routine is either prepared to
         handle access violations or has probed and copied the context record
         as appropriate.

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

    PKEXCEPTION_FRAME CxFrame;
    PKEXCEPTION_FRAME ExFrame;
    ULONG_PTR InitialStack;
    PKTRAP_FRAME TrFrame;

    //
    // If a context frame is specified, then initialize a trap frame and
    // and an exception frame with the specified user mode context.
    //

    InitialStack = (ULONG_PTR)Thread->InitialStack;
    if (ARGUMENT_PRESENT(ContextRecord)) {
        TrFrame = (PKTRAP_FRAME)(((InitialStack) -
                  sizeof(KTRAP_FRAME)) & ~((ULONG_PTR)15));
        ExFrame = (PKEXCEPTION_FRAME)(((ULONG_PTR)TrFrame -
                  sizeof(KEXCEPTION_FRAME)) & ~((ULONG_PTR)15));
        CxFrame = (PKEXCEPTION_FRAME)(((ULONG_PTR)ExFrame -
                  sizeof(KEXCEPTION_FRAME)) & ~((ULONG_PTR)15));

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
        // If the FPCR quadword in the specified context record is zero,
        // then assume it is a default value and force floating point round
        // to nearest mode (the hardware default mode is chopped rounding
        // which is not desirable for NT). It would be nice to initialize
        // the SoftFpcr here also but not all threads have a Teb.
        //

        if (TrFrame->Fpcr == 0) {
            ((PFPCR)(&TrFrame->Fpcr))->DynamicRoundingMode = ROUND_TO_NEAREST;
        }

        //
        // Set the saved previous processor mode in the trap frame and the
        // previous processor mode in the thread object to user mode.
        //

        TrFrame->PreviousMode = UserMode;
        Thread->PreviousMode = UserMode;

        //
        // Initialize the return address in the exception frame.
        //

        ExFrame->IntRa = 0;

    } else {
        ExFrame = NULL;
        CxFrame = (PKEXCEPTION_FRAME)(((InitialStack) -
                  sizeof(KEXCEPTION_FRAME)) & ~((ULONG_PTR)15));

        //
        // Set the previous mode in thread object to kernel.
        //

        Thread->PreviousMode = KernelMode;
    }

    //
    // Initialize context switch frame and set thread start up parameters.
    //
    // N.B. ULONG becomes canonical longword with (ULONGLONG)(LONG) cast.
    //

    CxFrame->SwapReturn = (ULONGLONG)(LONG_PTR)KiThreadStartup;
    if (ExFrame == NULL) {
        CxFrame->IntFp = (ULONGLONG)(LONG_PTR)ExFrame;

    } else {
        CxFrame->IntFp = (ULONGLONG)(LONG_PTR)TrFrame;
    }

    CxFrame->IntS0 = (ULONGLONG)(LONG_PTR)ContextRecord;
    CxFrame->IntS1 = (ULONGLONG)(LONG_PTR)StartContext;
    CxFrame->IntS2 = (ULONGLONG)(LONG_PTR)StartRoutine;
    CxFrame->IntS3 = (ULONGLONG)(LONG_PTR)SystemRoutine;

    CxFrame->Psr = 0;			// clear everything
    ((PSR *)(&CxFrame->Psr))->INTERRUPT_ENABLE = 1;
    ((PSR *)(&CxFrame->Psr))->IRQL = DISPATCH_LEVEL;
    ((PSR *)(&CxFrame->Psr))->MODE = 0;

    //
    // Set the initial kernel stack pointer.
    //

    Thread->KernelStack = (PVOID)(ULONGLONG)(LONG_PTR)CxFrame;
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

    ASSERT_THREAD(Thread);

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
