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

    3-19-96        Bernard Lint (blint)          Conversion to IA64 (from PPC and MIPS versions)

--*/

#include "ki.h"

VOID
KeContextToKframesSpecial (
    IN PKTHREAD Thread,
    IN OUT PKTRAP_FRAME TrapFrame,
    IN OUT PKEXCEPTION_FRAME ExceptionFrame,
    IN PCONTEXT ContextFrame,
    IN ULONG ContextFlags
    );

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

    N.B. Arguments to the new thread are passed in the Swap Frame preserved registers
         s0 - s3 which are restored by Swap Context when thread execution begins.

Arguments:

    Thread - Supplies a pointer to a dispatcher object of type thread.

    SystemRoutine - Supplies a pointer to the system function that is to be
        called when the thread is first scheduled for execution.

        N.B. This is the routine entry point, not a function pointer (plabel pointer).

    StartRoutine - Supplies an optional pointer to a function that is to be
        called after the system has finished initializing the thread. This
        parameter is specified if the thread is a system thread and will
        execute totally in kernel mode.

        N.B. This is the routine function pointer (plabel pointer).

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

    PKSWITCH_FRAME SwFrame;
    PKEXCEPTION_FRAME ExFrame;
    ULONG_PTR InitialStack;
    PKTRAP_FRAME TrFrame;

    //
    // Set up the thread backing store pointers from the initial stack pointers.
    //

    InitialStack = (ULONG_PTR)Thread->InitialStack;
    Thread->InitialBStore = (PVOID)InitialStack;
    Thread->BStoreLimit = (PVOID)(InitialStack + KERNEL_BSTORE_SIZE);

    //
    // If a context frame is specified, then initialize a trap frame and
    // and an exception frame with the specified user mode context. Also
    // allocate the switch frame.
    //

    if (ARGUMENT_PRESENT(ContextRecord)) {

        TrFrame = (PKTRAP_FRAME)((InitialStack) 
                      - KTHREAD_STATE_SAVEAREA_LENGTH
                      - KTRAP_FRAME_LENGTH);

        ExFrame = (PKEXCEPTION_FRAME)(((ULONG_PTR)TrFrame + 
                      STACK_SCRATCH_AREA - 
                      sizeof(KEXCEPTION_FRAME)) & ~((ULONG_PTR)15));

        SwFrame = (PKSWITCH_FRAME)(((ULONG_PTR)ExFrame -
                      sizeof(KSWITCH_FRAME)) & ~((ULONG_PTR)15));

        KeContextToKframesSpecial(Thread, TrFrame, ExFrame,
                           ContextRecord,
                           ContextRecord->ContextFlags | CONTEXT_CONTROL);

        //
        // Set the saved previous processor mode in the trap frame and the
        // previous processor mode in the thread object to user mode.
        //

        TrFrame->PreviousMode = UserMode;
        Thread->PreviousMode = UserMode;

        //
        // Initialize the FPSR for user mode
        //

        TrFrame->StFPSR = USER_FPSR_INITIAL;

        //
        // Initialize the user TEB pointer in the trap frame
        //

        TrFrame->IntTeb = (ULONGLONG)Thread->Teb;

    } else {

        SwFrame = (PKSWITCH_FRAME)((InitialStack) - sizeof(KSWITCH_FRAME));

        //
        // Set the previous mode in thread object to kernel.
        //

        Thread->PreviousMode = KernelMode;
    }

    //
    // Initialize context switch frame and set thread start up parameters.
    // The Swap return pointer and SystemRoutine are entry points, not function pointers.
    //

    RtlZeroMemory((PVOID)SwFrame, sizeof(KSWITCH_FRAME));   // init all to 0

    SwFrame->SwitchRp = ((PPLABEL_DESCRIPTOR)KiThreadStartup)->EntryPoint;
    SwFrame->SwitchExceptionFrame.IntS0 = (ULONGLONG)ContextRecord;
    SwFrame->SwitchExceptionFrame.IntS1 = (ULONGLONG)StartContext;
    SwFrame->SwitchExceptionFrame.IntS2 = (ULONGLONG)StartRoutine;
    SwFrame->SwitchExceptionFrame.IntS3 = 
        ((PPLABEL_DESCRIPTOR)SystemRoutine)->EntryPoint;
    SwFrame->SwitchFPSR = FPSR_FOR_KERNEL;
    SwFrame->SwitchBsp = (ULONGLONG)Thread->InitialBStore;

    Thread->KernelBStore = Thread->InitialBStore;
    Thread->KernelStack = (PVOID)((ULONG_PTR)SwFrame-STACK_SCRATCH_AREA);

    if (Thread->Teb) {
        PKAPPLICATION_REGISTERS AppRegs;

        AppRegs = GET_APPLICATION_REGISTER_SAVEAREA(Thread->StackBase);

        AppRegs->Ar21 = 0; // ContextRecord->StFCR;
        AppRegs->Ar24 = SANITIZE_FLAGS((ULONG) ContextRecord->Eflag, UserMode);
        AppRegs->Ar25 = USER_CODE_DESCRIPTOR;
        AppRegs->Ar26 = USER_DATA_DESCRIPTOR;
        AppRegs->Ar27 = (ULONGLONG)((CR4_VME << 32) | CR0_PE | CFLG_II);
        AppRegs->Ar28 = ContextRecord->StFSR;
        AppRegs->Ar29 = ContextRecord->StFIR;
        AppRegs->Ar30 = ContextRecord->StFDR;
        
    }

    return;
}

BOOLEAN
KeSetAutoAlignmentProcess (
    IN PRKPROCESS Process,
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
