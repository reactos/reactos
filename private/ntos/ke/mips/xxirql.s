//      TITLE("Manipulate Interrupt Request Level")
//++
//
// Copyright (c) 1990  Microsoft Corporation
//
// Module Name:
//
//    manpirql.s
//
// Abstract:
//
//    This module implements the code necessary to lower and raise the current
//    Interrupt Request Level (IRQL).
//
//
// Author:
//
//    David N. Cutler (davec) 12-Aug-1990
//
// Environment:
//
//    Kernel mode only.
//
// Revision History:
//
//--

#include "ksmips.h"

//
// Define external variables that can be addressed using GP.
//

        .extern KiSynchIrql        4

        SBTTL("Lower Interrupt Request Level")
//++
//
// VOID
// KeLowerIrql (
//    KIRQL NewIrql
//    )
//
// Routine Description:
//
//    This function lowers the current IRQL to the specified value.
//
// Arguments:
//
//    NewIrql (a0) - Supplies the new IRQL value.
//
// Return Value:
//
//    None.
//
//--

        LEAF_ENTRY(KeLowerIrql)

        and     a0,a0,0xff              // isolate new IRQL
        lbu     t0,KiPcr + PcIrqlTable(a0) // get translation table entry value
        li      t1,~(0xff << PSR_INTMASK) // get interrupt enable mask
        sll     t0,t0,PSR_INTMASK       // shift table entry into position

        DISABLE_INTERRUPTS(t2)          // disable interrupts

        and     t2,t2,t1                // clear current interrupt enables
        or      t2,t2,t0                // set new interrupt enables
        sb      a0,KiPcr + PcCurrentIrql(zero) // set new IRQL

        ENABLE_INTERRUPTS(t2)           // enable interrupts

        j       ra                      // return

        .end    KeLowerIrql

        SBTTL("Raise Interrupt Request Level")
//++
//
// VOID
// KeRaiseIrql (
//    KIRQL NewIrql,
//    PKIRQL OldIrql
//    )
//
// Routine Description:
//
//    This function raises the current IRQL to the specified value and returns
//    the old IRQL value.
//
// Arguments:
//
//    NewIrql (a0) - Supplies the new IRQL value.
//
//    OldIrql (a1) - Supplies a pointer to a variable that recieves the old
//       IRQL value.
//
// Return Value:
//
//    None.
//
//--

        LEAF_ENTRY(KeRaiseIrql)

        and     a0,a0,0xff              // isolate new IRQL
        lbu     t0,KiPcr + PcIrqlTable(a0) // get translation table entry value
        li      t1,~(0xff << PSR_INTMASK) // get interrupt enable mask
        sll     t0,t0,PSR_INTMASK       // shift table entry into position
        lbu     t2,KiPcr + PcCurrentIrql(zero) // get current IRQL

        DISABLE_INTERRUPTS(t3)          // disable interrupts

        and     t3,t3,t1                // clear current interrupt enables
        or      t3,t3,t0                // set new interrupt enables
        sb      a0,KiPcr + PcCurrentIrql(zero) // set new IRQL

        ENABLE_INTERRUPTS(t3)           // enable interrupts

        sb      t2,0(a1)                // store old IRQL
        j       ra                      // return

        .end    KeRaiseIrql

        SBTTL("Raise Interrupt Request Level to DPC Level")
//++
//
// KIRQL
// KeRaiseIrqlToDpcLevel (
//    VOID
//    )
//
// Routine Description:
//
//    This function swaps the current IRQL with dispatch level.
//
// Arguments:
//
//    None.
//
// Return Value:
//
//    The previous IRQL is returned as the function value.
//
//--

        LEAF_ENTRY(KiRaiseIrqlToXxxLevel)

        ALTERNATE_ENTRY(KeRaiseIrqlToDpcLevel)

        li      a0,DISPATCH_LEVEL       // set new IRQL value
        b       KeSwapIrql              // finish in common code

        SBTTL("Swap Interrupt Request Level")
//++
//
// KIRQL
// KeRaiseIrqlToSynchLevel (
//    VOID
//    )
//
// Routine Description:
//
//    This function swaps the current IRQL with synchronization level.
//
// Arguments:
//
//    None.
//
// Return Value:
//
//    The previous IRQL is returned as the function value.
//
//--

        ALTERNATE_ENTRY(KeRaiseIrqlToSynchLevel)

        lbu     a0,KiSynchIrql          // set new IRQL level

//++
//
// KIRQL
// KeSwapIrql (
//    IN KIRQL NewIrql
//    )
//
// Routine Description:
//
//    This function swaps the current IRQL with the specified IRQL.
//
// Arguments:
//
//    NewIrql (a0) - supplies the new IRQL value.
//
// Return Value:
//
//    The previous IRQL is returned as the function value.
//
//--

        ALTERNATE_ENTRY(KeSwapIrql)

        lbu     t0,KiPcr + PcIrqlTable(a0) // get translation table entry
        li      t1,~(0xff << PSR_INTMASK) // get interrupt enable mask
        sll     t0,t0,PSR_INTMASK       // shift table entry into position
        lbu     v0,KiPcr + PcCurrentIrql(zero) // get current IRQL

        DISABLE_INTERRUPTS(t2)          // disable interrupts

        and     t2,t2,t1                // clear current interrupt enables
        or      t2,t2,t0                // set new interrupt enables
        sb      a0,KiPcr + PcCurrentIrql(zero) // set new IRQL

        ENABLE_INTERRUPTS(t2)           // enable interrupts

        j       ra                      // return

        .end    KiRaiseIrqlToXxxLevel
