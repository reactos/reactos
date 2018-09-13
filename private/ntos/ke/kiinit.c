/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    kiinit.c

Abstract:

    This module implements architecture independent kernel initialization.

Author:

    David N. Cutler 11-May-1993

Environment:

    Kernel mode only.

Revision History:

--*/

#include "ki.h"

//
// Put all code for kernel initialization in the INIT section. It will be
// deallocated by memory management when phase 1 initialization is completed.
//

#if defined(ALLOC_PRAGMA)

#pragma alloc_text(INIT, KeInitSystem)
#pragma alloc_text(INIT, KiInitSystem)
#pragma alloc_text(INIT, KiComputeReciprocal)

#endif

BOOLEAN
KeInitSystem (
    VOID
    )

/*++

Routine Description:

    This function initializes executive structures implemented by the
    kernel.

    N.B. This function is only called during phase 1 initialization.

Arguments:

    None.

Return Value:

    A value of TRUE is returned if initialization is successful. Otherwise,
    a value of FALSE is returned.

--*/

{

    BOOLEAN Initialized = TRUE;

    //
    // Initialize the executive objects.
    //

#if 0

    if ((Initialized = KiChannelInitialization()) == FALSE) {
        KdPrint(("Kernel: Channel initialization failed\n"));
    }

#endif

#if defined(i386)

    //
    // Perform platform dependent initialization.
    //

    Initialized = KiInitMachineDependent();

#endif


    return Initialized;
}

VOID
KiInitSystem (
    VOID
    )

/*++

Routine Description:

    This function initializes architecture independent kernel structures.

Arguments:

    None.

Return Value:

    None.

--*/

{

    ULONG Index;

    //
    // Initialize dispatcher ready queue listheads.
    //

    for (Index = 0; Index < MAXIMUM_PRIORITY; Index += 1) {
        InitializeListHead(&KiDispatcherReadyListHead[Index]);
    }

    //
    // Initialize bug check callback listhead and spinlock.
    //

    InitializeListHead(&KeBugCheckCallbackListHead);
    KeInitializeSpinLock(&KeBugCheckCallbackLock);

    //
    // Initialize the timer expiration DPC object.
    //

    KeInitializeDpc(&KiTimerExpireDpc,
                    (PKDEFERRED_ROUTINE)KiTimerExpiration, NIL);

    //
    // Initialize the profile listhead and profile locks
    //

    KeInitializeSpinLock(&KiProfileLock);
    InitializeListHead(&KiProfileListHead);

    //
    // Initialize the active profile source listhead.
    //

    InitializeListHead(&KiProfileSourceListHead);

    //
    // Initialize the timer table, the timer completion listhead, and the
    // timer completion DPC.
    //

    for (Index = 0; Index < TIMER_TABLE_SIZE; Index += 1) {
        InitializeListHead(&KiTimerTableListHead[Index]);
    }

    //
    // Initialize the swap event, the process inswap listhead, the
    // process outswap listhead, the kernel stack inswap listhead,
    // the wait in listhead, and the wait out listhead.
    //

    KeInitializeEvent(&KiSwapEvent,
                      SynchronizationEvent,
                      FALSE);

    InitializeListHead(&KiProcessInSwapListHead);
    InitializeListHead(&KiProcessOutSwapListHead);
    InitializeListHead(&KiStackInSwapListHead);
    InitializeListHead(&KiWaitInListHead);
    InitializeListHead(&KiWaitOutListHead);

    //
    // Initialize the system service descriptor table.
    //

    KeServiceDescriptorTable[0].Base = &KiServiceTable[0];
    KeServiceDescriptorTable[0].Count = NULL;
    KeServiceDescriptorTable[0].Limit = KiServiceLimit;
#if defined(_IA64_)

    //
    // The global pointer associated with the table base is
    // placed just before the service table.
    //

    KeServiceDescriptorTable[0].TableBaseGpOffset =
        (LONG)(*(KiServiceTable-1) - (ULONG_PTR)KiServiceTable);
#endif
    KeServiceDescriptorTable[0].Number = &KiArgumentTable[0];
    for (Index = 1; Index < NUMBER_SERVICE_TABLES; Index += 1) {
        KeServiceDescriptorTable[Index].Limit = 0;
    }

    //
    // Copy the system service descriptor table to the shadow table
    // which is used to record the Win32 system services.
    //

    RtlCopyMemory(KeServiceDescriptorTableShadow,
                  KeServiceDescriptorTable,
                  sizeof(KeServiceDescriptorTable));

    //
    // Initialize call performance data structures.
    //

#if defined(_COLLECT_FLUSH_SINGLE_CALLDATA_)

    ExInitializeCallData(&KiFlushSingleCallData);

#endif

#if defined(_COLLECT_SET_EVENT_CALLDATA_)

    ExInitializeCallData(&KiSetEventCallData);

#endif

#if defined(_COLLECT_WAIT_SINGLE_CALLDATA_)

    ExInitializeCallData(&KiWaitSingleCallData);

#endif

    return;
}

LARGE_INTEGER
KiComputeReciprocal (
    IN LONG Divisor,
    OUT PCCHAR Shift
    )

/*++

Routine Description:

    This function computes the large integer reciprocal of the specified
    value.

Arguments:

    Divisor - Supplies the value for which the large integer reciprocal is
        computed.

    Shift - Supplies a pointer to a variable that receives the computed
        shift count.

Return Value:

    The large integer reciprocal is returned as the fucntion value.

--*/

{

    LARGE_INTEGER Fraction;
    LONG NumberBits;
    LONG Remainder;

    //
    // Compute the large integer reciprocal of the specified value.
    //

    NumberBits = 0;
    Remainder = 1;
    Fraction.LowPart = 0;
    Fraction.HighPart = 0;
    while (Fraction.HighPart >= 0) {
        NumberBits += 1;
        Fraction.HighPart = (Fraction.HighPart << 1) | (Fraction.LowPart >> 31);
        Fraction.LowPart <<= 1;
        Remainder <<= 1;
        if (Remainder >= Divisor) {
            Remainder -= Divisor;
            Fraction.LowPart |= 1;
        }
    }

    if (Remainder != 0) {
        if ((Fraction.LowPart == 0xffffffff) && (Fraction.HighPart == 0xffffffff)) {
            Fraction.LowPart = 0;
            Fraction.HighPart = 0x80000000;
            NumberBits -= 1;

        } else {
            if (Fraction.LowPart == 0xffffffff) {
                Fraction.LowPart = 0;
                Fraction.HighPart += 1;

            } else {
                Fraction.LowPart += 1;
            }
        }
    }

    //
    // Compute the shift count value and return the reciprocal fraction.
    //

    *Shift = (CCHAR)(NumberBits - 64);
    return Fraction;
}
