//      TITLE("Interlocked Increment and Decrement Support")
//++
//
// Copyright (c) 1991  Microsoft Corporation
// Copyright (c) 1992  Digital Equipment Corporation
//
// Module Name:
//
//    critsect.s
//
// Abstract:
//
//    This module implements functions to support user mode critical sections.
//    It contains some code from ntos\dll\alpha\critsect.s but without the Rtl
//    prefix.
//
// Author:
//
//    David N. Cutler 29-Apr-1992
//
// Environment:
//
//    Any mode.
//
// Revision History:
//
//    Thomas Van Baak (tvb) 22-Jul-1992
//
//        Adapted for Alpha AXP.
//
//--

#include "ksalpha.h"

        SBTTL("Interlocked Increment")
//++
//
// LONG
// InterlockedIncrement(
//    IN PLONG Addend
//    )
//
// Routine Description:
//
//    This function performs an interlocked increment on the addend variable.
//
//    This function and its companion are assuming that the count will never
//    be incremented past 2**31 - 1.
//
// Arguments:
//
//    Addend (a0) - Supplies a pointer to a variable whose value is to be
//       incremented.
//
// Return Value:
//
//    A negative value is returned if the updated value is less than zero,
//    a zero value is returned if the updated value is zero, and a nonzero
//    positive value is returned if the updated value is greater than zero.
//
//--

        LEAF_ENTRY(InterlockedIncrement)

10:     mb                              // synchronize memory access
        ldl_l   v0, 0(a0)               // get addend value - locked
        addl    v0, 1, v0               // increment addend value
        mov     v0, t0                  // copy updated value to t0 for store
        stl_c   t0, 0(a0)               // store conditionally
        beq     t0, 20f                 // if eq, conditional store failed
        mb                              // synchronize memory access
        ret     zero, (ra)              // return

//
// We expect the store conditional will usually succeed the first time so it
// is faster to branch forward (predicted not taken) to here and then branch
// backward (predicted taken) to where we wanted to go.
//

20:     br      zero, 10b               // go try load again

        .end    InterlockedIncrement

        SBTTL("InterlockedDecrement")
//++
//
// LONG
// InterlockedDecrement(
//    IN PLONG Addend
//    )
//
// Routine Description:
//
//    This function performs an interlocked decrement on the addend variable.
//
//    This function and its companion are assuming that the count will never
//    be decremented past 2**31 - 1.
//
// Arguments:
//
//    Addend (a0) - Supplies a pointer to a variable whose value is to be
//       decremented.
//
// Return Value:
//
//    A negative value is returned if the updated value is less than zero,
//    a zero value is returned if the updated value is zero, and a nonzero
//    positive value is returned if the updated value is greater than zero.
//
//--

        LEAF_ENTRY(InterlockedDecrement)

10:     mb                              // synchronize memory access
        ldl_l   v0, 0(a0)               // get addend value - locked
        subl    v0, 1, v0               // decrement addend value
        mov     v0, t0                  // copy updated value to t0 for store
        stl_c   t0, 0(a0)               // store conditionally
        beq     t0, 20f                 // if eq, conditional store failed
        mb                              // synchronize memory access
        ret     zero, (ra)              // return

//
// We expect the store conditional will usually succeed the first time so it
// is faster to branch forward (predicted not taken) to here and then branch
// backward (predicted taken) to where we wanted to go.
//

20:     br      zero, 10b               // go try load again

        .end    InterlockedDecrement

        SBTTL("Interlocked Exchange")
//++
//
// ULONG
// InterlockedExchange (
//    IN PULONG Source,
//    IN ULONG Value
//    )
//
// Routine Description:
//
//    This function performs an interlocked exchange of a longword value with
//    a longword in memory and returns the memory value.
//
// Arguments:
//
//    Source (a0) - Supplies a pointer to a variable whose value is to be
//       exchanged.
//
//    Value (a1) - Supplies the value to exchange with the source value.
//
// Return Value:
//
//    The source value is returned as the function value.
//
//--

        LEAF_ENTRY(InterlockedExchange)

10:     mb                              // synchronize memory access
        ldl_l   v0,0(a0)                // get current source value
        xor     a1,zero,t0              // set exchange value
        stl_c   t0,0(a0)                // replace source value
        beq     t0,20f                  // if eq, conditional store failed
        mb                              // synchronize memory access
        ret     zero,(ra)               // else/ return old value to caller

//
// We expect the store conditional will usually succeed the first time so it
// is faster to branch forward (predicted not taken) to here and then branch
// backward (predicted taken) to where we wanted to go.
//

20:     br      zero,10b                // go try spin lock again

        .end    InterlockedExchange

        SBTTL("Interlocked Compare Exchange")
//++
//
// LONG
// InterlockedCompareExchange (
//    IN OUT PLONG *Destination,
//    IN LONG Exchange,
//    IN LONG Comperand
//    )
//
// Routine Description:
//
//    This function performs an interlocked compare of the destination
//    value with the comperand value. If the destination value is equal
//    to the comperand value, then the exchange value is stored in the
//    destination. Otherwise, no opeation is performed.
//
// Arguments:
//
//    Destination (a0) - Supplies a pointer to the destination value.
//
//    Exchange (a1) - Supplies the exchange.
//
//    Comperand (a2) - Supplies the comperand value.
//
// Return Value:
//
//    The initial destination value is returned as the function value.
//
//--

        LEAF_ENTRY(InterlockedCompareExchange)

10:     mb                              // synchronize memory accesss
        ldl_l   v0, 0(a0)               // get current addend value
        bis     a1, zero, t0            // copy exchange value for store
        cmpeq   v0, a2, t1              // check if operands match
        beq     t1, 20f                 // if eq, operands mismatch
        stl_c   t0, 0(a0)               // store updated addend value
        beq     t0,25f                  // if eq, store conditional failed
        mb                              // synchronize memory access
20:     ret     zero, (ra)              // return

//
// We expect the store conditional will usually succeed the first time so it
// is faster to branch forward (predicted not taken) to here and then branch
// backward (predicted taken) to where we wanted to go.
//

25:     br      zero, 10b               // go try spin lock again

        .end    InterlockedCompareExchange

        SBTTL("Interlocked Exchange Add")
//++
//
// LONG
// InterlockedExchangeAdd (
//    IN PLONG Addend,
//    IN ULONG Increment
//    )
//
// Routine Description:
//
//    This function performs an interlocked add of an increment value to an
//    addend variable of type unsigned long. The initial value of the addend
//    variable is returned as the function value.
//
// Arguments:
//
//    Addend (a0) - Supplies a pointer to a variable whose value is to be
//       adjusted by the increment value.
//
//    Increment (a1) - Supplies the increment value to be added to the
//       addend variable.
//
// Return Value:
//
//    The initial value of the addend variable.
//
//--

        LEAF_ENTRY(InterlockedExchangeAdd)

10:     mb                              // synchronize memory access
        ldl_l   v0, 0(a0)               // get current addend value - locked
        addl    v0, a1, t0              // increment addend value
        stl_c   t0, 0(a0)               // store updated value - conditionally
        beq     t0, 20f                 // if eq, conditional store failed
        mb                              // synchronize memory access
        ret     zero, (ra)              // return

//
// We expect the store conditional will usually succeed the first time so it
// is faster to branch forward (predicted not taken) to here and then branch
// backward (predicted taken) to where we wanted to go.
//

20:     br      zero, 10b               // go try spin lock again

        .end    InterlockedExchangeAdd
