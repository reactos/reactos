/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    thredini.c

Abstract:

    This module implements the machine dependent function to set the initial
    context and data alignment handling mode for a process or thread object.

Author:

    David N. Cutler (davec) 31-Mar-1990

Environment:

    Kernel mode only.

Revision History:

    3 April 90  bryan willman

        This version ported to 386.

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

//
// Our notion of alignment is different, so force use of ours
//
#undef  ALIGN_UP
#undef  ALIGN_DOWN
#define ALIGN_DOWN(address,amt) ((ULONG)(address) & ~(( amt ) - 1))
#define ALIGN_UP(address,amt) (ALIGN_DOWN( (address + (amt) - 1), (amt) ))

//
// The function prototype for the special APC we use to set the
// hardware alignment state for a thread
//

VOID
KepSetAlignmentSpecialApc(
    IN PKAPC Apc,
    IN PKNORMAL_ROUTINE *NormalRoutine,
    IN PVOID *NormalContext,
    IN PVOID *SystemArgument1,
    IN PVOID *SystemArgument2
    );

extern BOOLEAN KeI386XMMIPresent;


VOID
KiInitializeContextThread (
    IN PKTHREAD Thread,
    IN PKSYSTEM_ROUTINE SystemRoutine,
    IN PKSTART_ROUTINE StartRoutine OPTIONAL,
    IN PVOID StartContext OPTIONAL,
    IN PCONTEXT ContextFrame OPTIONAL
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

    ContextFrame - Supplies an optional pointer a context frame which contains
        the initial user mode state of the thread. This parameter is specified
        if the thread is a user thread and will execute in user mode. If this
        parameter is not specified, then the Teb parameter is ignored.

Return Value:

    None.

--*/

{
    PFX_SAVE_AREA NpxFrame;
    PKSWITCHFRAME SwitchFrame;
    PKTRAP_FRAME TrFrame;
    PULONG PSystemRoutine;
    PULONG PStartRoutine;
    PULONG PStartContext;
    PULONG PUserContextFlag;
    ULONG  ContextFlags;
    CONTEXT Context2;
    PCONTEXT ContextFrame2 = NULL;
    PFXSAVE_FORMAT   PFxSaveArea;

    //
    // If a context frame is specified, then initialize a trap frame and
    // and an exception frame with the specified user mode context.
    //

    if (ARGUMENT_PRESENT(ContextFrame)) {

        RtlMoveMemory(&Context2, ContextFrame, sizeof(CONTEXT));
        ContextFrame2 = &Context2;
        ContextFlags = CONTEXT_CONTROL;

        //
        // The 80387 save area is at the very base of the kernel stack.
        //

        NpxFrame = (PFX_SAVE_AREA)(((ULONG)(Thread->InitialStack) -
                    sizeof(FX_SAVE_AREA)));

        //
        // Load up an initial NPX state.
        //

        if (KeI386XMMIPresent == TRUE) {
            PFxSaveArea = (PFXSAVE_FORMAT)ContextFrame2->ExtendedRegisters;
    
            PFxSaveArea->ControlWord   = 0x27f;  // like fpinit but 64bit mode
            PFxSaveArea->StatusWord    = 0;
            PFxSaveArea->TagWord       = 0;
            PFxSaveArea->ErrorOffset   = 0;
            PFxSaveArea->ErrorSelector = 0;
            PFxSaveArea->DataOffset    = 0;
            PFxSaveArea->DataSelector  = 0;
            PFxSaveArea->MXCsr         = 0x1f80; // mask all the exceptions
        } else {
            ContextFrame2->FloatSave.ControlWord   = 0x27f;  // like fpinit but 64bit mode
            ContextFrame2->FloatSave.StatusWord    = 0;
            ContextFrame2->FloatSave.TagWord       = 0xffff;
            ContextFrame2->FloatSave.ErrorOffset   = 0;
            ContextFrame2->FloatSave.ErrorSelector = 0;
            ContextFrame2->FloatSave.DataOffset    = 0;
            ContextFrame2->FloatSave.DataSelector  = 0;
        }


        if (KeI386NpxPresent) {
            ContextFrame2->FloatSave.Cr0NpxState = 0;
            NpxFrame->Cr0NpxState = 0;
            NpxFrame->NpxSavedCpu = 0;
            if (KeI386XMMIPresent == TRUE) {
                ContextFlags |= CONTEXT_EXTENDED_REGISTERS;
            } else {
                ContextFlags |= CONTEXT_FLOATING_POINT;
            }

            //
            // Threads NPX state is not in the coprocessor.
            //

            Thread->NpxState = NPX_STATE_NOT_LOADED;
            Thread->NpxIrql = PASSIVE_LEVEL;

        } else {
            NpxFrame->Cr0NpxState = CR0_EM;

            //
            // Threads NPX state is not in the coprocessor.
            // In the emulator case, do not set the CR0_EM bit as their
            // emulators may not want exceptions on FWAIT instructions.
            //

            Thread->NpxState = NPX_STATE_NOT_LOADED & ~CR0_MP;
        }

        //
        // Force debug registers off.  They won't work anyway from an
        // initial frame, debuggers must set a hard breakpoint in the target
        //

        ContextFrame2->Dr0 = 0;
        ContextFrame2->Dr1 = 0;
        ContextFrame2->Dr2 = 0;
        ContextFrame2->Dr3 = 0;
        ContextFrame2->Dr6 = 0;
        ContextFrame2->Dr7 = 0;
        ContextFrame2->ContextFlags &= ~(CONTEXT_DEBUG_REGISTERS);
#if 0
        //
        // If AutoAlignment is FALSE, we want to set the Alignment Check bit
        // in Eflags, so we will get alignment faults.
        //

        if (Thread->AutoAlignment == FALSE) {
            ContextFrame2->EFlags |= EFLAGS_ALIGN_CHECK;
        }
#endif
        //
        // If the thread is set

        TrFrame = (PKTRAP_FRAME)(((ULONG)NpxFrame - KTRAP_FRAME_LENGTH));

        //  Space for arguments to KiThreadStartup.  Order is important,
        //  Since args are passed on stack through KiThreadStartup to
        //  PStartRoutine with PStartContext as an argument.

        PUserContextFlag = (PULONG)TrFrame - 1;
        PStartContext = PUserContextFlag - 1;
        PStartRoutine = PStartContext - 1;
        PSystemRoutine = PStartRoutine - 1;

        SwitchFrame = (PKSWITCHFRAME)((PUCHAR)PSystemRoutine -
                                    sizeof(KSWITCHFRAME));

        //
        // Copy information from the specified context frame to the trap and
        // exception frames.
        //

        KeContextToKframes(TrFrame, NULL, ContextFrame2,
                           ContextFrame2->ContextFlags | ContextFlags,
                           UserMode);

        TrFrame->HardwareSegSs |= RPL_MASK;
        TrFrame->SegDs |= RPL_MASK;
        TrFrame->SegEs |= RPL_MASK;

#if DBG
        TrFrame->DbgArgMark = 0xBADB0D00;
#endif

        //
        // Tell KiThreadStartup that a user context is present.
        //

        *PUserContextFlag = 1;


        //
        // Initialize the kernel mode ExceptionList pointer
        //

        TrFrame->ExceptionList = EXCEPTION_CHAIN_END;

        //
        // Initialize the saved previous processor mode.
        //

        TrFrame->PreviousPreviousMode = UserMode;

        //
        // Set the previous mode in thread object to user.
        //

        Thread->PreviousMode = UserMode;


    } else {

        //
        // Dummy floating save area.  Kernel threads don't have or use
        // the floating point - the dummy save area is make the stacks
        // consistent.
        //

        NpxFrame = (PFX_SAVE_AREA)(((ULONG)(Thread->InitialStack) -
                    sizeof(FX_SAVE_AREA)));

        //
        // Load up an initial NPX state.
        //
        RtlZeroMemory((PVOID)NpxFrame, sizeof(FX_SAVE_AREA));

        if (KeI386FxsrPresent == TRUE) {
            NpxFrame->U.FxArea.ControlWord = 0x27f;//like fpinit but 64bit mode
            NpxFrame->U.FxArea.MXCsr       = 0x1f80;// mask all the exceptions
        } else {
            NpxFrame->U.FnArea.ControlWord  = 0x27f;//like fpinit but 64bit mode
            NpxFrame->U.FnArea.TagWord      = 0xffff;
        }

        //
        // Threads NPX state is not in the coprocessor.
        //

        Thread->NpxState = NPX_STATE_NOT_LOADED;

        //
        //  Space for arguments to KiThreadStartup.
        //  Order of fields in the switchframe is important,
        //  Since args are passed on stack through KiThreadStartup to
        //  PStartRoutine with PStartContext as an argument.
        //

        PUserContextFlag = (PULONG)((ULONG)NpxFrame) - 1;

        PStartContext = PUserContextFlag - 1;
        PStartRoutine = PStartContext - 1;
        PSystemRoutine = PStartRoutine - 1;

        SwitchFrame = (PKSWITCHFRAME)((PUCHAR)PSystemRoutine -
                                        sizeof(KSWITCHFRAME));


        //
        // Tell KiThreadStartup that a user context is NOT present.
        //

        *PUserContextFlag = 0;


        //
        // Set the previous mode in thread object to kernel.
        //

        Thread->PreviousMode = KernelMode;
    }

    //
    //  Set up thread start parameters.
    //  (UserContextFlag set above)
    //

    *PStartContext = (ULONG)StartContext;
    *PStartRoutine = (ULONG)StartRoutine;
    *PSystemRoutine = (ULONG)SystemRoutine;


    //
    //  Set up switch frame.  Assume the thread doesn't use the 80387;
    //  if it ever does (and there is one), these flags will get reset.
    //  Each thread starts with these same flags set, regardless of
    //  whether the hardware exists or not.
    //

    SwitchFrame->RetAddr = (ULONG)KiThreadStartup;

    SwitchFrame->Eflags = EFLAGS_INTERRUPT_MASK;

#if 0
    //
    // If AutoAlignment is FALSE, we want to set the Alignment Check bit
    // in Eflags, so we will get alignment faults.
    //

    if (Thread->AutoAlignment == FALSE) {
        SwitchFrame->Eflags |= EFLAGS_ALIGN_CHECK;
    }
#endif

    SwitchFrame->ExceptionList = (ULONG)(EXCEPTION_CHAIN_END);

    //
    // Set the initial kernel stack pointer.
    //

//DbgPrint("KiInitializeContextThread Thread %08x  SwitchFrame %08x\n", Thread, SwitchFrame);
//DbgPrint("PSystemRoutine %08x  PStartRoutine %08x  PStartContext %08x\n", *PSystemRoutine, *PStartRoutine, *PStartContext);

    Thread->KernelStack = (PVOID)SwitchFrame;
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

    Thread - Supplies a pointer to a dispatcher object of type thread.

    Enable - Supplies a boolean value that determines the handling of data
        alignment exceptions for the specified thread. A value of TRUE causes
        all data alignment exceptions to be automatically handled by the kernel.
        A value of FALSE causes all data alignment exceptions to be actually
        raised as exceptions.

Return Value:

    A value of TRUE is returned if data alignment exceptions were
    previously automatically handled by the kernel. Otherwise, a value
    of FALSE is returned.

--*/

{

    BOOLEAN Previous;
    PKAPC Apc;
    PKEVENT Event;
    KIRQL OldIrql;

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
    // Unlock dispatcher database and lower IRQL to its previous value.
    //

    KiUnlockDispatcherDatabase(OldIrql);

#if 0
    Apc = ExAllocatePool(NonPagedPoolMustSucceed, sizeof(KAPC));
    Event = ExAllocatePool(NonPagedPoolMustSucceed, sizeof(KEVENT));

    KeInitializeEvent(Event, NotificationEvent, FALSE);

    if ( Thread == KeGetCurrentThread() ) {

        Apc->SystemArgument1 = Thread;
        Apc->SystemArgument2 = Event;

        KeRaiseIrql(APC_LEVEL, &Irql);
        KepSetAlignmentSpecialApc( Apc, NULL, NULL,
                                   &Apc->SystemArgument1,
                                   &Apc->SystemArgument2 );
        KeLowerIrql(Irql);
    } else {
        KeInitializeApc( Apc,
                         Thread,
                         CurrentApcEnvironment,
                         KepSetAlignmentSpecialApc,
                         NULL,
                         NULL,
                         KernelMode,
                         NULL );

        if (!KeInsertQueueApc( Apc,
                               Thread,
                               Event,
                               2 ) ) {
            //
            // We couldn't queue the APC, so we will not be able to change
            // the AutoAlignment.  Update the thread object so that it
            // stays in sync with the hardware state.
            //
#if DBG
            DbgPrint("KeSetAutoAlignmentThread: unable to change thread's context\n");
#endif
            Thread->AutoAlignment = Previous;
        }

        KeWaitForSingleObject( Event,
                               Executive,
                               KernelMode,
                               FALSE,
                               NULL );
    }

    ExFreePool(Apc);
    ExFreePool(Event);
#endif

    return(Previous);
}

#if 0

VOID
KepSetAlignmentSpecialApc(
    IN PKAPC Apc,
    IN PKNORMAL_ROUTINE *NormalRoutine,
    IN PVOID *NormalContext,
    IN PVOID *SystemArgument1,
    IN PVOID *SystemArgument2
    )

/*++

Routine Description:

    This function updates the alignment check bit of the current thread's
    EFLAGS to reflect the AutoAlignment setting of the thread object.

Arguments:

    Apc - Supplies a pointer to the APC control object that caused entry
          into this routine.

    NormalRoutine - Supplies a pointer to a pointer to the normal routine
        function that was specifed when the APC was initialized.

    NormalContext - Supplies a pointer to a pointer to an arbitrary data
        structure that was specified when the APC was initialized.

    SystemArgument1 - Supplies a pointer to a PKTHREAD

    SystemArgument2 - Supplies a pointer to a PKEVENT

Return Value:

    None.

--*/

{
    PKTHREAD Thread;
    PKEVENT Event;
    PKTRAP_FRAME TrapFrame;
    CONTEXT ContextFrame;

    Thread = *(PKTHREAD *)SystemArgument1;
    Event = *(PKEVENT *)SystemArgument2;

    ASSERT( Thread == KeGetCurrentThread() );

    //
    // Find the trap frame on the stack, so we can get the thread context
    //
    TrapFrame = (PKTRAP_FRAME)((PUCHAR)Thread->InitialStack -
                ALIGN_UP(sizeof(KTRAP_FRAME),KTRAP_FRAME_ALIGN) -
                sizeof(FX_SAVE_AREA));

    ContextFrame.ContextFlags = CONTEXT_CONTROL;

    KeContextFromKframes( TrapFrame,
                          NULL,
                          &ContextFrame );

    //
    // If AutoAlignment is TRUE, we want the processor to transparently fixup
    // all alignment faults, so we clear the Alignment Check bit.  If
    // AutoAlignment is FALSE, we set the bit, so 486 processors will
    // give us alignment faults.
    //

    if (Thread->AutoAlignment) {
        ContextFrame.EFlags &= (~EFLAGS_ALIGN_CHECK);
    } else {
        ContextFrame.EFlags |= EFLAGS_ALIGN_CHECK;
    }

    //
    // Replace the modified EFlags in the trap frame.  When the thread returns
    // to user mode, it will be running with the new alignment setting.
    //

    KeContextToKframes( TrapFrame,
                        NULL,
                        &ContextFrame,
                        CONTEXT_CONTROL,
                        KeGetPreviousMode() );

    KeSetEvent(Event,0,FALSE);
}
#endif
