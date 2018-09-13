//++
//
// Module Name:
//
//     critsect.s
//
// Abstract:
//
//    This module implements functions to support user mode interlocked operation
//
// Author:
//
//
// Revision History:
//
//--

#include "ksia64.h"

        .file    "critsect.s"
//++
//
// LONG
// InterlockedIncrement(
//    IN PLONG Addend
//    )
//
// Routine Description:
//
//    This function performs an interlocked add of one to the addend variable.
//
//    No checking is done for overflow.
//
// Arguments:
//
//    Addend - Supplies a pointer to a variable whose value is to be
//       incremented by one.
//
// Return Value:
//
//    A negative value is returned if the updated value is less than zero,
//    a zero value is returned if the updated value is zero, and a nonzero
//    positive value is returned if the updated value is greater than zero.
//
//--

        LEAF_ENTRY(InterlockedIncrement)

        fetchadd4.acq  v0 = [a0], 1
        ;;
        add         v0 = 1, v0
        br.ret.sptk brp

        LEAF_EXIT(InterlockedIncrement)

//++
//
// LONG
// InterlockedDecrement(
//    IN PLONG Addend
//    )
//
// Routine Description:
//
//    This function performs an interlocked add of -1 to the addend variable.
//
//    No checking is done for overflow
//
// Arguments:
//
//    Addend - Supplies a pointer to a variable whose value is to be
//       decremented by one.
//
// Return Value:
//
//    A negative value is returned if the updated value is less than zero,
//    a zero value is returned if the updated value is zero, and a nonzero
//    positive value is returned if the updated value is greater than zero.
//
//--
        LEAF_ENTRY(InterlockedDecrement)

        fetchadd4.acq  v0 = [a0], -1
        ;;
        sub         v0 = v0, zero, 1
        br.ret.sptk brp

        LEAF_EXIT(InterlockedDecrement)

//++
//
//   LONG
//   InterlockedExchange(
//       IN OUT LPLONG Target,
//       IN LONG Value
//       )
//
//   Routine Description:
//
//       This function atomically exchanges the Target and Value, returning
//       the prior contents of Target
//
//   Arguments:
//
//       Target - Address of LONG to exchange
//       Value  - New value of LONG
//
//   Return Value:
//
//       The prior value of Target
//
//--
        LEAF_ENTRY(InterlockedExchange)

        xchg4.nt1   v0 = [a0], a1
        br.ret.sptk brp

        LEAF_EXIT(InterlockedExchange)

//++
//
//   PVOID
//   InterlockedCompareExchange (
//       IN OUT PVOID *Destination,
//       IN PVOID Exchange,
//       IN PVOID Comperand
//       )
//
//   Routine Description:
//
//    This function performs an interlocked compare of the destination
//    value with the comperand value. If the destination value is equal
//    to the comperand value, then the exchange value is stored in the
//    destination. Otherwise, no operation is performed.
//
// Arguments:
//
//    Destination - Supplies a pointer to destination value.
//
//    Exchange - Supplies the exchange value.
//
//    Comperand - Supplies the comperand value.
//
// Return Value:
//
//    (v0) - The initial destination value.
//
//--

        LEAF_ENTRY(InterlockedCompareExchange)

        mov         ar.ccv = a2
        ;;
        cmpxchg4.acq v0 = [a0], a1, ar.ccv
        br.ret.sptk  brp

        LEAF_EXIT(InterlockedCompareExchange)

//++
//
//   LONG
//   InterlockedExchangeAdd (
//       IN OUT PLONG Addend,
//       IN LONG Increment
//       )
//
//   Routine Description:
//
//    This function performs an interlocked add of an increment value to an
//    addend variable of type unsinged long. The initial value of the addend
//    variable is returned as the function value.
//
//       It is NOT possible to mix ExInterlockedDecrementLong and
//       ExInterlockedIncrementong with ExInterlockedAddUlong.
//
//
// Arguments:
//
//    Addend - Supplies a pointer to a variable whose value is to be
//       adjusted by the increment value.
//
//    Increment - Supplies the increment value to be added to the
//       addend variable.
//
// Return Value:
//
//    (v0) - The initial value of the addend.
//
//--

        LEAF_ENTRY(InterlockedExchangeAdd)

Iea10:
        ld4          t1 = [a0]
        ;;
        mov          ar.ccv = t1
        add          t2 = a1, t1
        ;;

        cmpxchg4.acq v0 = [a0], t2, ar.ccv
        ;;

        cmp4.ne      pt7, pt8 = v0, t1
(pt7)   br.cond.spnt Iea10
(pt8)   br.ret.sptk  brp
        ;;

        LEAF_EXIT(InterlockedExchangeAdd)

