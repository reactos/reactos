/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    intobj.c

Abstract:

    This module implements the kernel interrupt object. Functions are provided
    to initialize, connect, and disconnect interrupt objects.

Author:

    David N. Cutler (davec) 30-Jul-1989

Environment:

    Kernel mode only.

Revision History:

    23-Jan-1990    shielint

                   Modified for NT386 interrupt manager

--*/

#include "ki.h"

//
//  Externs from trap.asm used to compute and set handlers for unexpected
//  hardware interrupts.
//

extern  ULONG   KiStartUnexpectedRange(VOID);
extern  ULONG   KiEndUnexpectedRange(VOID);
extern  ULONG   KiUnexpectedEntrySize;


VOID
KiInterruptDispatch2ndLvl(
    VOID
    );


VOID
KiChainedDispatch2ndLvl(
    VOID
    );


typedef enum {
    NoConnect,
    NormalConnect,
    ChainConnect,
    UnkownConnect
} CONNECT_TYPE, *PCONNECT_TYPE;

typedef struct {
    CONNECT_TYPE            Type;
    PKINTERRUPT             Interrupt;
    PKINTERRUPT_ROUTINE     NoDispatch;
    PKINTERRUPT_ROUTINE     InterruptDispatch;
    PKINTERRUPT_ROUTINE     FloatingDispatch;
    PKINTERRUPT_ROUTINE     ChainedDispatch;
    PKINTERRUPT_ROUTINE    *FlatDispatch;
} DISPATCH_INFO, *PDISPATCH_INFO;


VOID
KiGetVectorInfo (
    IN  ULONG                Vector,
    OUT PDISPATCH_INFO       DispatchInfo
    );

VOID
KiConnectVectorAndInterruptObject (
    IN PKINTERRUPT Interrupt,
    IN CONNECT_TYPE Type
    );


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
    service context, spin lock, vector, IRQL, SynchronizeIrql, and floating
    context save flag are initialized.

Arguments:

    Interrupt - Supplies a pointer to a control object of type interrupt.

    ServiceRoutine - Supplies a pointer to a function that is to be
        executed when an interrupt occurs via the specified interrupt
        vector.

    ServiceContext - Supplies a pointer to an arbitrary data structure which is
        to be passed to the function specified by the ServiceRoutine parameter.

    SpinLock - Supplies a pointer to an executive spin lock.

    Vector - Supplies the index of the entry in the Interrupt Dispatch Table
        that is to be associated with the ServiceRoutine function.

    Irql - Supplies the request priority of the interrupting source.

    SynchronizeIrql - The request priority that the interrupt should be
        synchronized with.

    InterruptMode - Supplies the mode of the interrupt; LevelSensitive or

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
    PULONG pl;
    PULONG NormalDispatchCode;

    //
    // Initialize standard control object header.
    //

    Interrupt->Type = InterruptObject;
    Interrupt->Size = sizeof(KINTERRUPT);

    //
    // Initialize the address of the service routine,
    // the service context, the address of the spin lock, the vector
    // number, the IRQL of the interrupting source, the Irql used for
    // synchronize execution, the interrupt mode, the processor
    // number, and the floating context save flag.
    //

    Interrupt->ServiceRoutine = ServiceRoutine;
    Interrupt->ServiceContext = ServiceContext;

    if (ARGUMENT_PRESENT(SpinLock)) {
        Interrupt->ActualLock = SpinLock;
    } else {
        KeInitializeSpinLock (&Interrupt->SpinLock);
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
    // and edit the machine code stored in the structure (please see
    // _KiInterruptTemplate in intsup.asm.)  Finally, flush the dcache
    // on all processors that the current thread can
    // run on to ensure that the code is actually in memory.
    //

    NormalDispatchCode = &(Interrupt->DispatchCode[0]);

    pl = NormalDispatchCode;

    for (Index = 0; Index < NORMAL_DISPATCH_LENGTH; Index += 1) {
        *NormalDispatchCode++ = KiInterruptTemplate[Index];
    }

    //
    // The following two instructions set the address of current interrupt
    // object the the NORMAL dispatching code.
    //

    pl = (PULONG)((PUCHAR)pl + ((PUCHAR)&KiInterruptTemplateObject -
                                (PUCHAR)KiInterruptTemplate) -4); 
    *pl = (ULONG)Interrupt;

    KeSweepDcache(FALSE);

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
    DISPATCH_INFO DispatchInfo;
    BOOLEAN Connected;
    BOOLEAN ConnectError;
    BOOLEAN Enabled;
    KIRQL Irql;
    CCHAR Number;
    KIRQL OldIrql;
    ULONG Vector;

    //
    // If the interrupt object is already connected, the interrupt vector
    // number is invalid, an attempt is being made to connect to a vector
    // that cannot be connected, the interrupt request level is invalid, or
    // the processor number is invalid, then do not connect the interrupt
    // object. Else connect interrupt object to the specified vector and
    // establish the proper interrupt dispatcher.
    //

    Connected = FALSE;
    ConnectError = FALSE;
    Irql = Interrupt->Irql;
    Number = Interrupt->Number;
    Vector = Interrupt->Vector;
    if ( !((Irql > HIGH_LEVEL) ||
           (Number >= KeNumberProcessors) ||
           (Interrupt->SynchronizeIrql < Irql) ||
           (Interrupt->FloatingSave)    // R0 x87 usage not supported on x86
          )
       ) {

        //
        //
        // Set system affinity to the specified processor.
        //

        KeSetSystemAffinityThread((KAFFINITY)(1<<Number));

        //
        // Raise IRQL to dispatcher level and lock dispatcher database.
        //

        KiLockDispatcherDatabase(&OldIrql);

        //
        // Is interrupt object already connected?
        //

        if (!Interrupt->Connected) {

            //
            // Determine interrupt dispatch vector
            //

            KiGetVectorInfo (
                Vector,
                &DispatchInfo
                );

            //
            // If dispatch vector is not connected, then connect it
            //

            if (DispatchInfo.Type == NoConnect) {
                Connected = TRUE;
                Interrupt->Connected = TRUE;

                //
                // Connect interrupt dispatch to interrupt object dispatch code
                //

                InitializeListHead(&Interrupt->InterruptListEntry);
                KiConnectVectorAndInterruptObject (Interrupt, NormalConnect);

                //
                // Enabled system vector
                //

                Enabled = HalEnableSystemInterrupt(Vector, Irql, Interrupt->Mode);
                if (!Enabled) {
                    ConnectError = TRUE;
                }


            } else if (DispatchInfo.Type != UnkownConnect &&
                       Interrupt->ShareVector  &&
                       DispatchInfo.Interrupt->ShareVector  &&
                       DispatchInfo.Interrupt->Mode == Interrupt->Mode) {

                //
                // Vector is already connected as sharable.  New vector is sharable
                // and modes match.  Chain new vector.
                //

                Connected = TRUE;
                Interrupt->Connected = TRUE;

                ASSERT (Irql <= SYNCH_LEVEL);

                //
                // If not already using chained dispatch handler, set it up
                //

                if (DispatchInfo.Type != ChainConnect) {
                    KiConnectVectorAndInterruptObject (DispatchInfo.Interrupt, ChainConnect);
                }

                //
                // Add to tail of chained dispatch
                //

                InsertTailList(
                    &DispatchInfo.Interrupt->InterruptListEntry,
                    &Interrupt->InterruptListEntry
                    );

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

    if (Connected  &&  ConnectError) {
#if DBG
        DbgPrint ("HalEnableSystemInterrupt failed\n");
#endif
        KeDisconnectInterrupt (Interrupt);
        Connected = FALSE;
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

    DISPATCH_INFO DispatchInfo;
    BOOLEAN Connected;
    PKINTERRUPT Interrupty;
    KIRQL Irql;
    KIRQL OldIrql;
    ULONG Vector;

    //
    // Set system affinity to the specified processor.
    //

    KeSetSystemAffinityThread((KAFFINITY)(1<<Interrupt->Number));

    //
    // Raise IRQL to dispatcher level and lock dispatcher database.
    //

    KiLockDispatcherDatabase(&OldIrql);

    //
    // If the interrupt object is connected, then disconnect it from the
    // specified vector.
    //

    Connected = Interrupt->Connected;
    if (Connected) {
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

        //
        // Determine interrupt dispatch vector
        //

        KiGetVectorInfo (
            Vector,
            &DispatchInfo
            );


        //
        // Is dispatch a chained handler?
        //

        if (DispatchInfo.Type == ChainConnect) {

            ASSERT (Irql <= SYNCH_LEVEL);

            //
            // Is interrupt being removed from head?
            //

            if (Interrupt == DispatchInfo.Interrupt) {

                //
                // Update next interrupt object to be head
                //

                DispatchInfo.Interrupt = CONTAINING_RECORD(
                                               DispatchInfo.Interrupt->InterruptListEntry.Flink,
                                               KINTERRUPT,
                                               InterruptListEntry
                                               );

                KiConnectVectorAndInterruptObject (DispatchInfo.Interrupt, ChainConnect);
            }

            //
            // Remove interrupt object
            //

            RemoveEntryList(&Interrupt->InterruptListEntry);

            //
            // If there's only one interrupt object left on this vector,
            // determine proper interrupt dispatcher
            //

            Interrupty = CONTAINING_RECORD(
                                DispatchInfo.Interrupt->InterruptListEntry.Flink,
                                KINTERRUPT,
                                InterruptListEntry
                                );

            if (DispatchInfo.Interrupt == Interrupty) {
                KiConnectVectorAndInterruptObject (Interrupty, NormalConnect);
            }

        } else {

            //
            // Removing last interrupt object from the vector.  Disable the
            // vector, and set it to unconnected
            //

            HalDisableSystemInterrupt(Interrupt->Vector, Irql);
            KiConnectVectorAndInterruptObject (Interrupt, NoConnect);
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

VOID
KiGetVectorInfo (
    IN  ULONG                Vector,
    OUT PDISPATCH_INFO       DispatchInfo
    )
{
    PKINTERRUPT_ROUTINE Dispatch;
    ULONG CurrentDispatch;
    ULONG DispatchType;

    //
    // Get second level dispatch point
    //


    DispatchType = HalSystemVectorDispatchEntry (
                        Vector,
                        &DispatchInfo->FlatDispatch,
                        &DispatchInfo->NoDispatch
                        );

    //
    // Get vector info
    //

    switch (DispatchType) {
        case 0:
            //
            // Primary dispatch
            //

            DispatchInfo->NoDispatch = (PKINTERRUPT_ROUTINE) (((ULONG) &KiStartUnexpectedRange) +
                                     (Vector - PRIMARY_VECTOR_BASE) * KiUnexpectedEntrySize);

            DispatchInfo->InterruptDispatch = KiInterruptDispatch;
            DispatchInfo->FloatingDispatch = KiFloatingDispatch;
            DispatchInfo->ChainedDispatch = KiChainedDispatch;
            DispatchInfo->FlatDispatch = NULL;

            CurrentDispatch = (ULONG) KiReturnHandlerAddressFromIDT(Vector);
            DispatchInfo->Interrupt = CONTAINING_RECORD (
                                        CurrentDispatch,
                                        KINTERRUPT,
                                        DispatchCode
                                        );
            break;

        case 1:
            //
            // Secondardy dispatch.
            //

            DispatchInfo->InterruptDispatch = KiInterruptDispatch2ndLvl;
            DispatchInfo->FloatingDispatch = KiInterruptDispatch2ndLvl;
            DispatchInfo->ChainedDispatch = KiChainedDispatch2ndLvl;

            CurrentDispatch = (ULONG) *DispatchInfo->FlatDispatch;
            DispatchInfo->Interrupt = (PKINTERRUPT) ( (PUCHAR) CurrentDispatch -
                                            (PUCHAR) KiInterruptTemplate +
                                            (PUCHAR) &KiInterruptTemplate2ndDispatch
                                            );
            break;

        default:
            // Other values reserved
            KeBugCheck (MISMATCHED_HAL);
    }


    //
    // Determine dispatch type
    //

    if (((PKINTERRUPT_ROUTINE) CurrentDispatch) == DispatchInfo->NoDispatch) {

        //
        // Is connected to the NoDispatch function
        //

        DispatchInfo->Type = NoConnect;

    } else {
        Dispatch = DispatchInfo->Interrupt->DispatchAddress;

        if (Dispatch == DispatchInfo->ChainedDispatch) {
            //
            // Is connected to the chained handler
            //

            DispatchInfo->Type = ChainConnect;

        } else if (Dispatch == DispatchInfo->InterruptDispatch ||
                   Dispatch == DispatchInfo->FloatingDispatch) {
            //
            // If connection to the non-chained handler
            //

            DispatchInfo->Type = NormalConnect;

        } else {

            //
            // Unkown connection
            //

            DispatchInfo->Type = UnkownConnect;
#if DBG
            DbgPrint ("KiGetVectorInfo not understood\n");
#endif
        }
    }
}

VOID
KiConnectVectorAndInterruptObject (
    IN PKINTERRUPT Interrupt,
    IN CONNECT_TYPE Type
    )
{
    PKINTERRUPT_ROUTINE DispatchAddress;
    DISPATCH_INFO DispatchInfo;
    PULONG pl;

    //
    // Get current connect info
    //

    KiGetVectorInfo (
        Interrupt->Vector,
        &DispatchInfo
        );

    //
    // If disconnecting, set vector to NoDispatch
    //

    if (Type == NoConnect) {

        DispatchAddress = DispatchInfo.NoDispatch;

    } else {

        //
        // Set interrupt objects dispatch for new type
        //

        DispatchAddress = DispatchInfo.ChainedDispatch;

        if (Type == NormalConnect) {
            DispatchAddress = DispatchInfo.InterruptDispatch;
            if (Interrupt->FloatingSave) {
                DispatchAddress = DispatchInfo.FloatingDispatch;
            }
        }

        Interrupt->DispatchAddress = DispatchAddress;

        //
        // Set interrupt objects dispatch code to kernel dispatcher
        //

        pl = &(Interrupt->DispatchCode[0]);
        pl = (PULONG)((PUCHAR)pl +
                    ((PUCHAR)&KiInterruptTemplateDispatch -
                     (PUCHAR)KiInterruptTemplate) -4); 

        *pl = (ULONG)DispatchAddress-(ULONG)((PUCHAR)pl+4);

        //
        // Set dispatch vector to proper address dispatch code location
        //

        if (DispatchInfo.FlatDispatch) {

            //
            // Connect to flat dispatch
            //

            DispatchAddress = (PKINTERRUPT_ROUTINE)
                    ((PUCHAR) &(Interrupt->DispatchCode[0]) +
                     ((PUCHAR) &KiInterruptTemplate2ndDispatch -
                      (PUCHAR) KiInterruptTemplate));

        } else {

            //
            // Connect to enter_all dispatch
            //

            DispatchAddress = (PKINTERRUPT_ROUTINE) &Interrupt->DispatchCode;
        }
    }


    if (DispatchInfo.FlatDispatch) {

        //
        // Connect to flat dispatch
        //

        *DispatchInfo.FlatDispatch = DispatchAddress;

    } else {

        //
        // Connect to IDT
        //

        KiSetHandlerAddressToIDT (Interrupt->Vector, DispatchAddress);
    }
}
