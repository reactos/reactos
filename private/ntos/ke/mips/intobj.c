/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    intobj.c

Abstract:

    This module implements the kernel interrupt object. Functions are provided
    to initialize, connect, and disconnect interrupt objects.

Author:

    David N. Cutler (davec) 3-Apr-1990

Environment:

    Kernel mode only.

Revision History:


--*/

#include "ki.h"

VOID
KeInitializeInterrupt (
    IN PKINTERRUPT Interrupt,
    IN PKSERVICE_ROUTINE ServiceRoutine,
    IN PVOID ServiceContext,
    IN PKSPIN_LOCK SpinLock OPTIONAL,
    IN ULONG Vector,
    IN KIRQL Irql,
    IN KIRQL SynchronizeIrql,
    IN KINTERRUPT_MODE InterruptMode,
    IN BOOLEAN ShareVector,
    IN CCHAR ProcessorNumber,
    IN BOOLEAN FloatingSave
    )

/*++

Routine Description:

    This function initializes a kernel interrupt object. The service routine,
    service context, spin lock, vector, IRQL, Synchronized IRQL, and floating
    context save flag are initialized.

Arguments:

    Interrupt - Supplies a pointer to a control object of type interrupt.

    ServiceRoutine - Supplies a pointer to a function that is to be
        executed when an interrupt occurs via the specified interrupt
        vector.

    ServiceContext - Supplies a pointer to an arbitrary data structure which is
        to be passed to the function specified by the ServiceRoutine parameter.

    SpinLock - Supplies an optional pointer to an executive spin lock.

    Vector - Supplies the index of the entry in the Interrupt Dispatch Table
        that is to be associated with the ServiceRoutine function.

    Irql - Supplies the request priority of the interrupting source.

    SynchronizeIrql - The request priority that the interrupt should be
        synchronized with.

    InterruptMode - Supplies the mode of the interrupt; LevelSensitive or
        Latched.

    ShareVector - Supplies a boolean value that specifies whether the
        vector can be shared with other interrupt objects or not.  If FALSE
        then the vector may not be shared, if TRUE it may be.
        Latched.

    ProcessorNumber - Supplies the number of the processor to which the
        interrupt will be connected.

    FloatingSave - Supplies a boolean value that determines whether the
        floating point registers and pipe line are to be saved before calling
        the ServiceRoutine function.

Return Value:

    None.

--*/

{

    LONG Index;

    //
    // Initialize standard control object header.
    //

    Interrupt->Type = InterruptObject;
    Interrupt->Size = sizeof(KINTERRUPT);

    //
    // Initialize the address of the service routine, the service context,
    // the address of the spin lock, the address of the actual spin lock
    // that will be used, the vector number, the IRQL of the interrupting
    // source, the Synchronized IRQL of the interrupt object, the interrupt
    // mode, the processor number, and the floating context save flag.
    //

    Interrupt->ServiceRoutine = ServiceRoutine;
    Interrupt->ServiceContext = ServiceContext;

    if (ARGUMENT_PRESENT(SpinLock)) {
        Interrupt->ActualLock = SpinLock;

    } else {
        Interrupt->SpinLock = 0;
        Interrupt->ActualLock = &Interrupt->SpinLock;
    }

    Interrupt->Vector = Vector;
    Interrupt->Irql = Irql;
    Interrupt->SynchronizeIrql = SynchronizeIrql;
    Interrupt->Mode = InterruptMode;
    Interrupt->ShareVector = ShareVector;
    Interrupt->Number = ProcessorNumber;
    Interrupt->FloatingSave = FloatingSave;

    //
    // Copy the interrupt dispatch code template into the interrupt object
    // and flush the dcache on all processors that the current thread can
    // run on to ensure that the code is actually in memory.
    //

    for (Index = 0; Index < DISPATCH_LENGTH; Index += 1) {
        Interrupt->DispatchCode[Index] = KiInterruptTemplate[Index];
    }

    KeSweepIcache(FALSE);

    //
    // Set the connected state of the interrupt object to FALSE.
    //

    Interrupt->Connected = FALSE;
    return;
}

BOOLEAN
KeConnectInterrupt (
    IN PKINTERRUPT Interrupt
    )

/*++

Routine Description:

    This function connects an interrupt object to the interrupt vector
    specified by the interrupt object. If the interrupt object is already
    connected, or an attempt is made to connect to an interrupt that cannot
    be connected, then a value of FALSE is returned. Else the specified
    interrupt object is connected to the interrupt vector, the connected
    state is set to TRUE, and TRUE is returned as the function value.

Arguments:

    Interrupt - Supplies a pointer to a control object of type interrupt.

Return Value:

    If the interrupt object is already connected or an attempt is made to
    connect to an interrupt vector that cannot be connected, then a value
    of FALSE is returned. Else a value of TRUE is returned.

--*/

{

    BOOLEAN Connected;
    PKINTERRUPT Interruptx;
    KIRQL Irql;
    CHAR Number;
    KIRQL OldIrql;
    KIRQL PreviousIrql;
    ULONG Vector;

    //
    // If the interrupt object is already connected, the interrupt vector
    // number is invalid, an attempt is being made to connect to a vector
    // that cannot be connected, the interrupt request level is invalid,
    // the processor number is invalid, of the interrupt vector is less
    // than or equal to the highest level and it not equal to the specified
    // IRQL, then do not connect the interrupt object. Else connect interrupt
    // object to the specified vector and establish the proper interrupt
    // dispatcher.
    //

    Connected = FALSE;
    Irql = Interrupt->Irql;
    Number = Interrupt->Number;
    Vector = Interrupt->Vector;
    if ((((Vector >= MAXIMUM_VECTOR) || (Irql > HIGH_LEVEL) ||
       ((Vector <= HIGH_LEVEL) &&
       ((((1 << Vector) & PCR->ReservedVectors) != 0) || (Vector != Irql))) ||
       (Number >= KeNumberProcessors))) == FALSE) {

        //
        // Set system affinity to the specified processor.
        //

        KeSetSystemAffinityThread((KAFFINITY)(1 << Number));

        //
        // Raise IRQL to dispatcher level and lock dispatcher database.
        //

        KiLockDispatcherDatabase(&OldIrql);

        //
        // If the specified interrupt vector is not connected, then
        // connect the interrupt vector to the interrupt object dispatch
        // code, establish the dispatcher address, and set the new
        // interrupt mode and enable masks. Else if the interrupt is
        // already chained, then add the new interrupt object at the end
        // of the chain. If the interrupt vector is not chained, then
        // start a chain with the previous interrupt object at the front
        // of the chain. The interrupt mode of all interrupt objects in
        // a chain must be the same.
        //

        if (Interrupt->Connected == FALSE) {
            if (PCR->InterruptRoutine[Vector] ==
                (PKINTERRUPT_ROUTINE)(&KxUnexpectedInterrupt.DispatchCode)) {
                Connected = TRUE;
                Interrupt->Connected = TRUE;
                if (Interrupt->FloatingSave != FALSE) {
                    Interrupt->DispatchAddress = KiFloatingDispatch;

                } else {
                    if (Interrupt->Irql == Interrupt->SynchronizeIrql) {
                        Interrupt->DispatchAddress =
                                    (PKINTERRUPT_ROUTINE)KiInterruptDispatchSame;

                    } else {
                        Interrupt->DispatchAddress =
                                    (PKINTERRUPT_ROUTINE)KiInterruptDispatchRaise;
                    }
                }

                PCR->InterruptRoutine[Vector] =
                            (PKINTERRUPT_ROUTINE)(&Interrupt->DispatchCode);

                HalEnableSystemInterrupt(Vector, Irql, Interrupt->Mode);

            } else {
                Interruptx = CONTAINING_RECORD(PCR->InterruptRoutine[Vector],
                                               KINTERRUPT,
                                               DispatchCode[0]);

                if (Interrupt->Mode == Interruptx->Mode) {
                    Connected = TRUE;
                    Interrupt->Connected = TRUE;
                    KeRaiseIrql(max(Irql, (KIRQL)KiSynchIrql), &PreviousIrql);
                    if (Interruptx->DispatchAddress != KiChainedDispatch) {
                        InitializeListHead(&Interruptx->InterruptListEntry);
                        Interruptx->DispatchAddress = KiChainedDispatch;
                    }

                    InsertTailList(&Interruptx->InterruptListEntry,
                                   &Interrupt->InterruptListEntry);

                    KeLowerIrql(PreviousIrql);
                }
            }
        }

        //
        // Unlock dispatcher database and lower IRQL to its previous value.
        //

        KiUnlockDispatcherDatabase(OldIrql);

        //
        // Set system affinity back to the original value.
        //

        KeRevertToUserAffinityThread();
    }

    //
    // Return whether interrupt was connected to the specified vector.
    //

    return Connected;
}

BOOLEAN
KeDisconnectInterrupt (
    IN PKINTERRUPT Interrupt
    )

/*++

Routine Description:

    This function disconnects an interrupt object from the interrupt vector
    specified by the interrupt object. If the interrupt object is not
    connected, then a value of FALSE is returned. Else the specified interrupt
    object is disconnected from the interrupt vector, the connected state is
    set to FALSE, and TRUE is returned as the function value.

Arguments:

    Interrupt - Supplies a pointer to a control object of type interrupt.

Return Value:

    If the interrupt object is not connected, then a value of FALSE is
    returned. Else a value of TRUE is returned.

--*/

{

    BOOLEAN Connected;
    PKINTERRUPT Interruptx;
    PKINTERRUPT Interrupty;
    KIRQL Irql;
    KIRQL OldIrql;
    KIRQL PreviousIrql;
    ULONG Vector;

    //
    // Set system affinity to the specified processor.
    //

    KeSetSystemAffinityThread((KAFFINITY)(1 << Interrupt->Number));

    //
    // Raise IRQL to dispatcher level and lock dispatcher database.
    //

    KiLockDispatcherDatabase(&OldIrql);

    //
    // If the interrupt object is connected, then disconnect it from the
    // specified vector.
    //

    Connected = Interrupt->Connected;
    if (Connected != FALSE) {
        Irql = Interrupt->Irql;
        Vector = Interrupt->Vector;

        //
        // If the specified interrupt vector is not connected to the chained
        // interrupt dispatcher, then disconnect it by setting its dispatch
        // address to the unexpected interrupt routine. Else remove the
        // interrupt object from the interrupt chain. If there is only
        // one entry remaining in the list, then reestablish the dispatch
        // address.
        //

        Interruptx = CONTAINING_RECORD(PCR->InterruptRoutine[Vector],
                                       KINTERRUPT,
                                       DispatchCode[0]);

        if (Interruptx->DispatchAddress == KiChainedDispatch) {
            KeRaiseIrql(max(Irql, (KIRQL)KiSynchIrql), &PreviousIrql);
            if (Interrupt == Interruptx) {
                Interruptx = CONTAINING_RECORD(Interruptx->InterruptListEntry.Flink,
                                               KINTERRUPT, InterruptListEntry);
                Interruptx->DispatchAddress = KiChainedDispatch;
                PCR->InterruptRoutine[Vector] =
                                (PKINTERRUPT_ROUTINE)(&Interruptx->DispatchCode);
            }

            RemoveEntryList(&Interrupt->InterruptListEntry);
            Interrupty = CONTAINING_RECORD(Interruptx->InterruptListEntry.Flink,
                                           KINTERRUPT,
                                           InterruptListEntry);

            if (Interruptx == Interrupty) {
                if (Interrupty->FloatingSave != FALSE) {
                    Interrupty->DispatchAddress = KiFloatingDispatch;

                } else {
                    if (Interrupty->Irql == Interrupty->SynchronizeIrql) {
                        Interrupty->DispatchAddress =
                                    (PKINTERRUPT_ROUTINE)KiInterruptDispatchSame;

                    } else {
                        Interrupty->DispatchAddress =
                                    (PKINTERRUPT_ROUTINE)KiInterruptDispatchRaise;
                    }
                }

                PCR->InterruptRoutine[Vector] =
                               (PKINTERRUPT_ROUTINE)(&Interrupty->DispatchCode);
                }

            KeLowerIrql(PreviousIrql);

        } else {
            HalDisableSystemInterrupt(Vector, Irql);
            PCR->InterruptRoutine[Vector] =
                    (PKINTERRUPT_ROUTINE)(&KxUnexpectedInterrupt.DispatchCode);
        }

        KeSweepIcache(TRUE);
        Interrupt->Connected = FALSE;
    }

    //
    // Unlock dispatcher database and lower IRQL to its previous value.
    //

    KiUnlockDispatcherDatabase(OldIrql);

    //
    // Set system affinity back to the original value.
    //

    KeRevertToUserAffinityThread();

    //
    // Return whether interrupt was disconnected from the specified vector.
    //

    return Connected;
}
