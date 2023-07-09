//       TITLE("Interlocked Support")
//++
//
// Copyright (c) 1990  Microsoft Corporation
// Copyright (c) 1992  Digital Equipment Corporation
//
// Module Name:
//
//    intrlock.s
//
// Abstract:
//
//    This module implements functions to support interlocked operations.
//    Interlocked operations can only operate on nonpaged data and the
//    specified spinlock cannot be used for any other purpose.
//
// Author:
//
//    David N. Cutler (davec) 26-Mar-1990
//
// Environment:
//
//    Kernel mode.
//
// Revision History:
//
//    Thomas Van Baak (tvb) 18-May-1992
//
//        Adapted for Alpha AXP.
//
//--

#include "ksalpha.h"

        SBTTL("Interlocked Add Large Integer")
//++
//
// LARGE_INTEGER
// ExInterlockedAddLargeInteger (
//    IN PLARGE_INTEGER Addend,
//    IN LARGE_INTEGER Increment,
//    IN PKSPIN_LOCK Lock
//    )
//
// Routine Description:
//
//    This function performs an interlocked add of an increment value to an
//    addend variable of type large integer. The initial value of the addend
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
//    Lock (a2) - Supplies a pointer to a spin lock to be used to synchronize
//       access to the addend variable.
//
// Return Value:
//
//    The result of the interlocked large integer add.
//
// Implementation Note:
//
//    The specification of this function requires that the given lock must be
//    used to synchronize the update even though on Alpha the operation can
//    actually done atomically without using the specified lock.
//
//--

        LEAF_ENTRY(ExInterlockedAddLargeInteger)

10:     DISABLE_INTERRUPTS              // disable interrupts

#if !defined(NT_UP)

        LDP_L   t0, 0(a2)               // get current lock value - locked
        bne     t0, 20f                 // if ne, spin lock owned
        mov     a2, t0                  // set ownership value (lock address)
        STP_C   t0, 0(a2)               // set spin lock owned - conditionally
        beq     t0, 20f                 // if eq, conditional store failed
        mb                              // synchronize memory access


#endif

        ldq     t0, 0(a0)               // get addend
        addq    t0, a1, v0              // do the add
        stq     v0, 0(a0)               // store result

#if !defined(NT_UP)

        mb                              // synchronize memory access
        STP     zero, 0(a2)             // set spin lock not owned

#endif

        ENABLE_INTERRUPTS               // enable interrupts

        ret     zero, (ra)              // return


//
// We expect the store conditional will usually succeed the first time so it
// is faster to branch forward (predicted not taken) to here and then branch
// backward (predicted taken) to where we wanted to go.
//

#if !defined(NT_UP)

20:     ENABLE_INTERRUPTS               // enable interrupts

22:     LDP     t0, 0(a2)               // read current lock value
        beq     t0, 10b                 // if eq, lock not owned
        br      zero, 22b               // spin in cache until available

#endif

        .end    ExInterlockedAddLargeInteger

        SBTTL("Interlocked Add Large Statistic")
//++
//
// VOID
// ExInterlockedAddLargeStatistic (
//    IN PLARGE_INTEGER Addend,
//    IN ULONG Increment
//    )
//
// Routine Description:
//
//    This function performs an interlocked add of an increment value to an
//    addend variable of type large integer.
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
//    None.
//
// Implementation Note:
//
//    The specification of this function requires that the given lock must be
//    used to synchronize the update even though on Alpha the operation can
//    actually done atomically without using the specified lock.
//
//--

        LEAF_ENTRY(ExInterlockedAddLargeStatistic)

        zap     a1, 0xf0, a1            // zero extend increment value
10:     ldq_l   t0, 0(a0)               // get addend
        addq    t0, a1, t0              // do the add
        stq_c   t0, 0(a0)               // store result
        beq     t0, 20f                 // if eq, store conditional failed
        ret     zero, (ra)              // return

//
// We expect the store conditional will usually succeed the first time so it
// is faster to branch forward (predicted not taken) to here and then branch
// backward (predicted taken) to where we wanted to go.
//

20:     br      zero, 10b               // try again

        .end    ExInterlockedAddLargeStatistic

        SBTTL("Interlocked Add Unsigned Long")
//++
//
// ULONG
// ExInterlockedAddUlong (
//    IN PULONG Addend,
//    IN ULONG Increment,
//    IN PKSPIN_LOCK Lock
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
//    Lock (a2) - Supplies a pointer to a spin lock to be used to synchronize
//       access to the addend variable.
//
// Return Value:
//
//    The initial value of the addend variable.
//
// Implementation Note:
//
//    The specification of this function requires that the given lock must be
//    used to synchronize the update even though on Alpha the operation can
//    actually done atomically without using the specified lock.
//
//--

        LEAF_ENTRY(ExInterlockedAddUlong)

10:     DISABLE_INTERRUPTS              // (PALcode) v0 is clobbered

#if !defined(NT_UP)

        LDP_L   t0, 0(a2)               // get current lock value - locked
        bne     t0, 20f                 // if ne, spin lock still owned
        mov     a2, t0                  // set ownership value (lock address)
        STP_C   t0, 0(a2)               // set spin lock owned - conditionally
        beq     t0, 20f                 // if eq, conditional store failed
        mb                              // synchronize memory access

#endif

//
// Set the return value in t0 for now since PALcode may use v0.
//

        ldl     t0, 0(a0)               // get addend value (return value also)
        addl    t0, a1, t1              // compute adjusted value
        stl     t1, 0(a0)               // store updated value

#if !defined(NT_UP)

        mb                              // synchronize memory access
        STP     zero, 0(a2)             // set spin lock not owned

#endif

        ENABLE_INTERRUPTS               // (PALcode) v0 is clobbered

        mov     t0, v0                  // set return value
        ret     zero, (ra)              // return

//
// We expect the store conditional will usually succeed the first time so it
// is faster to branch forward (predicted not taken) to here and then branch
// backward (predicted taken) to where we wanted to go.
//

#if !defined(NT_UP)

20:     ENABLE_INTERRUPTS               // (PALcode) v0 is clobbered

22:     LDP     t0, 0(a2)               // read current lock value
        beq     t0, 10b                 // try spinlock again if available
        br      zero, 22b               // spin in cache until available

#endif

        .end    ExInterlockedAddUlong

        SBTTL("Interlocked Exchange Unsigned Long")
//++
//
// ULONG
// ExInterlockedExchangeUlong (
//    IN PULONG Source,
//    IN ULONG Value,
//    IN PKSPIN_LOCK Lock
//    )
//
// Routine Description:
//
//    This function performs an interlocked exchange of a longword value with
//    a longword in memory and returns the memory value.
//
//    N.B. There is an alternate entry point provided for this routine which
//         is ALPHA target specific and whose prototype does not include the
//         spinlock parameter. Since the routine never refers to the spinlock
//         parameter, no additional code is required.
//
// Arguments:
//
//    Source (a0) - Supplies a pointer to a variable whose value is to be
//       exchanged.
//
//    Value (a1) - Supplies the value to exchange with the source value.
//
//    Lock (a2) - Supplies a pointer to a spin lock to be used to synchronize
//       access to the source variable.
//
// Return Value:
//
//    The source value is returned as the function value.
//
//--

        LEAF_ENTRY(ExInterlockedExchangeUlong)

        ALTERNATE_ENTRY(ExAlphaInterlockedExchangeUlong)

10:     ldl_l   v0, 0(a0)               // get current source value
        bis     a1, zero, t0            // set exchange value
        stl_c   t0, 0(a0)               // replace source value
        beq     t0, 20f                 // if eq, conditional store failed
        ret     zero, (ra)              // return old value to caller

//
// We expect the store conditional will usually succeed the first time so it
// is faster to branch forward (predicted not taken) to here and then branch
// backward (predicted taken) to where we wanted to go.
//

20:     br      zero,10b                // go try spin lock again

        .end    ExInterlockedExchangeUlong

        SBTTL("Interlocked Exchange Add Large Integer")
//++
//
// LARGE_INTEGER
// ExpInterlockedExchangeAddLargeInteger (
//    IN PLARGE_INTEGER Addend,
//    IN LARGE_INTEGER Increment
//    )
//
// Routine Description:
//
//    This function performs an interlocked add of an increment value to an
//    addend variable of type large integer. The initial value of the addend
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
//    The initial value of the addend variable
//
//--

        LEAF_ENTRY(ExpInterlockedExchangeAddLargeInteger)

10:     ldq_l   v0, 0(a0)               // get addend value
        addq    a1, v0, t1              // add increment
        stq_c   t1, 0(a0)               // store addend value
        beq     t1, 15f                 // if eq, store conditional failed
        ret     zero, (ra)              // return old value to caller

//
// Conditional store attempt failed.
//

15:     beq     zero, 10b               // retry

        .end    ExpInterlockedExchangeAddLargeInteger

        SBTTL("Interlocked Decrement Long")
//++
//
// INTERLOCKED_RESULT
// ExInterlockedDecrementLong (
//    IN PLONG Addend,
//    IN PKSPIN_LOCK Lock
//    )
//
// Routine Description:
//
//    This function performs an interlocked decrement on an addend variable
//    of type signed long. The sign and whether the result is zero is returned
//    as the function value.
//
//    N.B. There is an alternate entry point provided for this routine which
//         is ALPHA target specific and whose prototype does not include the
//         spinlock parameter. Since the routine never refers to the spinlock
//         parameter, no additional code is required.
//
// Arguments:
//
//    Addend (a0) - Supplies a pointer to a variable whose value is to be
//       decremented.
//
//    Lock (a1) - Supplies a pointer to a spin lock to be used to synchronize
//       access to the addend variable.
//
// Return Value:
//
//    RESULT_NEGATIVE is returned if the resultant addend value is negative.
//    RESULT_ZERO is returned if the resultant addend value is zero.
//    RESULT_POSITIVE is returned if the resultant addend value is positive.
//
// Implementation Note:
//
//    The specification of this function does not require that the given lock
//    be used to synchronize the update as long as the update is synchronized
//    somehow. On Alpha a single load locked-store conditional does the job.
//
//--

        LEAF_ENTRY(ExInterlockedDecrementLong)

        ALTERNATE_ENTRY(ExAlphaInterlockedDecrementLong)

10:     ldl_l   v0, 0(a0)               // get current addend value - locked
        subl    v0, 1, v0               // decrement addend value
        mov     v0, t0                  // copy updated value to t0 for store
        stl_c   t0, 0(a0)               // store updated value - conditionally
        beq     t0, 20f                 // if eq, conditional store failed

//
// Determine the INTERLOCKED_RESULT value based on the updated addend value.
// N.B. RESULT_ZERO = 0, RESULT_NEGATIVE = 1, RESULT_POSITIVE = 2.
//

        sra     v0, 63, t0              // replicate the sign bit to every bit
        addl    t0, 2, t0               // if t0 = 0 return 2, if -1 return 1
        cmovne  v0, t0, v0              // if v0 = 0 return 0
        ret     zero, (ra)              // return

//
// We expect the store conditional will usually succeed the first time so it
// is faster to branch forward (predicted not taken) to here and then branch
// backward (predicted taken) to where we wanted to go.
//

20:     br      zero, 10b               // go try spin lock again

        .end    ExInterlockedDecrementLong

        SBTTL("Interlocked Increment Long")
//++
//
// INTERLOCKED_RESULT
// ExInterlockedIncrementLong (
//    IN PLONG Addend,
//    IN PKSPIN_LOCK Lock
//    )
//
// Routine Description:
//
//    This function performs an interlocked increment on an addend variable
//    of type signed long. The sign and whether the result is zero is returned
//    as the function value.
//
//    N.B. There is an alternate entry point provided for this routine which
//         is ALPHA target specific and whose prototype does not include the
//         spinlock parameter. Since the routine never refers to the spinlock
//         parameter, no additional code is required.
//
// Arguments:
//
//    Addend (a0) - Supplies a pointer to a variable whose value is to be
//       incremented.
//
//    Lock (a1) - Supplies a pointer to a spin lock to be used to synchronize
//       access to the addend variable.
//
// Return Value:
//
//    RESULT_NEGATIVE is returned if the resultant addend value is negative.
//    RESULT_ZERO is returned if the resultant addend value is zero.
//    RESULT_POSITIVE is returned if the resultant addend value is positive.
//
// Implementation Note:
//
//    The specification of this function does not require that the given lock
//    be used to synchronize the update as long as the update is synchronized
//    somehow. On Alpha a single load locked-store conditional does the job.
//
//--

        LEAF_ENTRY(ExInterlockedIncrementLong)

        ALTERNATE_ENTRY(ExAlphaInterlockedIncrementLong)

10:     ldl_l   v0, 0(a0)               // get current addend value - locked
        addl    v0, 1, v0               // increment addend value
        mov     v0, t0                  // copy updated value to t0 for store
        stl_c   t0, 0(a0)               // store updated value - conditionally
        beq     t0, 20f                 // if eq, conditional store failed

//
// Determine the INTERLOCKED_RESULT value based on the updated addend value.
// N.B. RESULT_ZERO = 0, RESULT_NEGATIVE = 1, RESULT_POSITIVE = 2.
//

        sra     v0, 63, t0              // replicate the sign bit to every bit
        addl    t0, 2, t0               // if t0 = 0 return 2, if -1 return 1
        cmovne  v0, t0, v0              // if v0 = 0 return 0
        ret     zero, (ra)              // return

//
// We expect the store conditional will usually succeed the first time so it
// is faster to branch forward (predicted not taken) to here and then branch
// backward (predicted taken) to where we wanted to go.
//

20:     br      zero, 10b               // go try spin lock again

        .end    ExInterlockedIncrementLong

        SBTTL("Interlocked Insert Head List")
//++
//
// PLIST_ENTRY
// ExInterlockedInsertHeadList (
//    IN PLIST_ENTRY ListHead,
//    IN PLIST_ENTRY ListEntry,
//    IN PKSPIN_LOCK Lock
//    )
//
// Routine Description:
//
//    This function inserts an entry at the head of a doubly linked list
//    so that access to the list is synchronized in a multiprocessor system.
//
// Arguments:
//
//    ListHead (a0) - Supplies a pointer to the head of the doubly linked
//       list into which an entry is to be inserted.
//
//    ListEntry (a1) - Supplies a pointer to the entry to be inserted at the
//       head of the list.
//
//    Lock (a2) - Supplies a pointer to a spin lock to be used to synchronize
//       access to the list.
//
// Return Value:
//
//    Pointer to entry that was at the head of the list or NULL if the list
//    was empty.
//
//--

        LEAF_ENTRY(ExInterlockedInsertHeadList)

10:     DISABLE_INTERRUPTS              // (PALcode) v0 is clobbered

#if !defined(NT_UP)

        LDP_L   t0, 0(a2)               // get current lock value - locked
        bne     t0, 20f                 // if ne, spin lock still owned
        mov     a2, t0                  // set ownership value (lock address)
        STP_C   t0, 0(a2)               // set spin lock owned - conditionally
        beq     t0, 20f                 // if eq, conditional store failed
        mb                              // synchronize memory access

#endif

//
// Set the return value in t0 for now since PALcode may use v0.
//

        LDP     t0, LsFlink(a0)         // get address of next entry (return value also)
        STP     t0, LsFlink(a1)         // store next link in entry
        STP     a0, LsBlink(a1)         // store previous link in entry
        STP     a1, LsBlink(t0)         // store previous link in next
        STP     a1, LsFlink(a0)         // store next link in head

#if !defined(NT_UP)

        mb                              // sychronize memory access
        STP     zero, 0(a2)             // set spin lock not owned

#endif

        ENABLE_INTERRUPTS               // (PALcode) v0 is clobbered

        xor     t0, a0, v0              // if t0=a0, list empty, set v0 to NULL
        cmovne  v0, t0, v0              // else return previous entry at head
        ret     zero, (ra)              // return

//
// We expect the store conditional will usually succeed the first time so it
// is faster to branch forward (predicted not taken) to here and then branch
// backward (predicted taken) to where we wanted to go.
//

#if !defined(NT_UP)

20:     ENABLE_INTERRUPTS               // (PALcode) v0 is clobbered

22:     LDP     t0, 0(a2)               // read current lock value
        beq     t0, 10b                 // try spinlock again if available
        br      zero, 22b               // spin in cache until available

#endif

        .end    ExInterlockedInsertHeadList

        SBTTL("Interlocked Insert Tail List")
//++
//
// PLIST_ENTRY
// ExInterlockedInsertTailList (
//    IN PLIST_ENTRY ListHead,
//    IN PLIST_ENTRY ListEntry,
//    IN PKSPIN_LOCK Lock
//    )
//
// Routine Description:
//
//    This function inserts an entry at the tail of a doubly linked list
//    so that access to the list is synchronized in a multiprocessor system.
//
// Arguments:
//
//    ListHead (a0) - Supplies a pointer to the head of the doubly linked
//       list into which an entry is to be inserted.
//
//    ListEntry (a1) - Supplies a pointer to the entry to be inserted at the
//       tail of the list.
//
//    Lock (a2) - Supplies a pointer to a spin lock to be used to synchronize
//       access to the list.
//
// Return Value:
//
//    Pointer to entry that was at the tail of the list or NULL if the list
//    was empty.
//
//--

        LEAF_ENTRY(ExInterlockedInsertTailList)

10:     DISABLE_INTERRUPTS              // (PALcode) v0 is clobbered

#if !defined(NT_UP)

        LDP_L   t0, 0(a2)               // get current lock value - locked
        bne     t0, 20f                 // if ne, spin lock still owned
        mov     a2, t0                  // set ownership value (lock address)
        STP_C   t0, 0(a2)               // set spin lock owned - conditionally
        beq     t0, 20f                 // if eq, conditional store failed
        mb                              // sychronize memory access

#endif

//
// Set the return value in t0 for now since PALcode may use v0.
//

        LDP     t0, LsBlink(a0)         // get address of previous entry (return value also)
        STP     a0, LsFlink(a1)         // store next link in entry
        STP     t0, LsBlink(a1)         // store previous link in entry
        STP     a1, LsBlink(a0)         // store previous link in next
        STP     a1, LsFlink(t0)         // store next link in head

#if !defined(NT_UP)

        mb                              // sychronize memory access
        STP     zero, 0(a2)             // set spin lock not owned

#endif

        ENABLE_INTERRUPTS               // (PALcode) v0 is clobbered

        xor     t0, a0, v0              // if t0=a0, list empty, set v0 to NULL
        cmovne  v0, t0, v0              // else return previous entry at tail
        ret     zero, (ra)              // return

//
// We expect the store conditional will usually succeed the first time so it
// is faster to branch forward (predicted not taken) to here and then branch
// backward (predicted taken) to where we wanted to go.
//

#if !defined(NT_UP)

20:     ENABLE_INTERRUPTS               // (PALcode) v0 is clobbered

22:     LDP     t0, 0(a2)               // read current lock value
        beq     t0, 10b                 // try spinlock again if available
        br      zero, 22b               // spin in cache until available

#endif

        .end    ExInterlockedInsertTailList

        SBTTL("Interlocked Remove Head List")
//++
//
// PLIST_ENTRY
// ExInterlockedRemoveHeadList (
//    IN PLIST_ENTRY ListHead,
//    IN PKSPIN_LOCK Lock
//    )
//
// Routine Description:
//
//    This function removes an entry from the head of a doubly linked list
//    so that access to the list is synchronized in a multiprocessor system.
//    If there are no entries in the list, then a value of NULL is returned.
//    Otherwise, the address of the entry that is removed is returned as the
//    function value.
//
// Arguments:
//
//    ListHead (a0) - Supplies a pointer to the head of the doubly linked
//       list from which an entry is to be removed.
//
//    Lock (a1) - Supplies a pointer to a spin lock to be used to synchronize
//       access to the list.
//
// Return Value:
//
//    The address of the entry removed from the list, or NULL if the list is
//    empty.
//
//--

        LEAF_ENTRY(ExInterlockedRemoveHeadList)

10:     DISABLE_INTERRUPTS              // (PALcode) v0 is clobbered

#if !defined(NT_UP)

        LDP_L   t0, 0(a1)               // get current lock value - locked
        bne     t0, 30f                 // if ne, spin lock still owned
        mov     a1, t0                  // set ownership value (lock address)
        STP_C   t0, 0(a1)               // set spin lock owned - conditionally
        beq     t0, 30f                 // if eq, conditional store failed
        mb                              // synchronize memory access

#endif

//
// Set the return value in t0 for now since PALcode may use v0.
//

        LDP     t2, LsFlink(a0)         // get address of next entry
        xor     t2, a0, t0              // if t2=a0, list empty, set t0 to NULL
        beq     t0, 20f                 // if eq, list is empty
        LDP     t1, LsFlink(t2)         // get address of next entry
        STP     t1, LsFlink(a0)         // store address of next in head
        STP     a0, LsBlink(t1)         // store address of previous in next
        mov     t2, t0                  // return the address of entry removed
20:                                     //

#if !defined(NT_UP)

        mb                              // synchronize memory access
        STP     zero, 0(a1)             // set spin lock not owned

#endif

        ENABLE_INTERRUPTS               // (PALcode) v0 is clobbered

        mov     t0, v0                  // set return value
        ret     zero, (ra)              // return

//
// We expect the store conditional will usually succeed the first time so it
// is faster to branch forward (predicted not taken) to here and then branch
// backward (predicted taken) to where we wanted to go.
//

#if !defined(NT_UP)

30:     ENABLE_INTERRUPTS               // (PALcode) v0 is clobbered

32:     LDP     t0, 0(a1)               // read current lock value
        beq     t0, 10b                 // try spinlock again if available
        br      zero, 32b               // spin in cache until available

#endif

        .end    ExInterlockedRemoveHeadList

        SBTTL("Interlocked Pop Entry List")
//++
//
// PSINGLE_LIST_ENTRY
// ExInterlockedPopEntryList (
//    IN PSINGLE_LIST_ENTRY ListHead,
//    IN PKSPIN_LOCK Lock
//    )
//
// Routine Description:
//
//    This function removes an entry from the head of a singly linked list
//    so that access to the list is synchronized in a multiprocessor system.
//    If there are no entries in the list, then a value of NULL is returned.
//    Otherwise, the address of the entry that is removed is returned as the
//    function value.
//
// Arguments:
//
//    ListHead (a0) - Supplies a pointer to the head of the singly linked
//       list from which an entry is to be removed.
//
//    Lock (a1) - Supplies a pointer to a spin lock to be used to synchronize
//       access to the list.
//
// Return Value:
//
//    The address of the entry removed from the list, or NULL if the list is
//    empty.
//
//--

        LEAF_ENTRY(ExInterlockedPopEntryList)

10:     DISABLE_INTERRUPTS              // (PALcode) v0 is clobbered

#if !defined(NT_UP)

        LDP_L   t0, 0(a1)               // get current lock value - locked
        bne     t0, 30f                 // if ne, spin lock still owned
        mov     a1, t0                  // set ownership value (lock address)
        STP_C   t0, 0(a1)               // set spin lock owned - conditionally
        beq     t0, 30f                 // if eq, conditional store failed
        mb                              // synchronize memory access

#endif

//
// Set the return value in t0 for now since PALcode may use v0.
//

        LDP     t0, 0(a0)               // get address of next entry (return value also)
        beq     t0, 20f                 // if eq [NULL], list is empty
        LDP     t1, 0(t0)               // get address of next entry
        STP     t1, 0(a0)               // store address of next in head
20:                                     //

#if !defined(NT_UP)

        mb                              // synchronize memory access
        STP     zero, 0(a1)             // set spin lock not owned

#endif

        ENABLE_INTERRUPTS               // (PALcode) v0 is clobbered

        mov     t0, v0                  // set return value
        ret     zero, (ra)              // return

//
// We expect the store conditional will usually succeed the first time so it
// is faster to branch forward (predicted not taken) to here and then branch
// backward (predicted taken) to where we wanted to go.
//

#if !defined(NT_UP)

30:     ENABLE_INTERRUPTS               // (PALcode) v0 is clobbered

32:     LDP     t0, 0(a1)               // read current lock value
        beq     t0, 10b                 // try spinlock again if available
        br      zero, 32b               // spin in cache until available

#endif

        .end    ExInterlockedPopEntryList

        SBTTL("Interlocked Push Entry List")
//++
//
// PSINGLE_LIST_ENTRY
// ExInterlockedPushEntryList (
//    IN PSINGLE_LIST_ENTRY ListHead,
//    IN PSINGLE_LIST_ENTRY ListEntry,
//    IN PKSPIN_LOCK Lock
//    )
//
// Routine Description:
//
//    This function inserts an entry at the head of a singly linked list
//    so that access to the list is synchronized in a multiprocessor system.
//
// Arguments:
//
//    ListHead (a0) - Supplies a pointer to the head of the singly linked
//       list into which an entry is to be inserted.
//
//    ListEntry (a1) - Supplies a pointer to the entry to be inserted at the
//       head of the list.
//
//    Lock (a2) - Supplies a pointer to a spin lock to be used to synchronize
//       access to the list.
//
// Return Value:
//
//    Previous contents of ListHead.  NULL implies list went from empty
//       to not empty.
//
//--

        LEAF_ENTRY(ExInterlockedPushEntryList)

10:     DISABLE_INTERRUPTS              // (PALcode) v0 is clobbered

#if !defined(NT_UP)

        LDP_L   t0, 0(a2)               // get current lock value - locked
        bne     t0, 20f                 // if ne, spin lock still owned
        mov     a2, t0                  // set ownership value (lock address)
        STP_C   t0, 0(a2)               // set spin lock owned - conditionally
        beq     t0, 20f                 // if eq, conditional store failed
        mb                              // synchronize memory access

#endif

//
// Set the return value in t0 for now since PALcode may use v0.
//

        LDP     t0, 0(a0)               // get address of first entry (return value also)
        STP     t0, 0(a1)               // set address of next in new entry
        STP     a1, 0(a0)               // set address of first entry

#if !defined(NT_UP)

        mb                              // synchronize memory access
        STP     zero, 0(a2)             // set spin lock not owned

#endif

        ENABLE_INTERRUPTS               // (PALcode) v0 is clobbered

        mov     t0, v0                  // set return value
        ret     zero, (ra)              // return

//
// We expect the store conditional will usually succeed the first time so it
// is faster to branch forward (predicted not taken) to here and then branch
// backward (predicted taken) to where we wanted to go.
//

#if !defined(NT_UP)

20:     ENABLE_INTERRUPTS               // (PALcode) v0 is clobbered

22:     LDP     t0, 0(a2)               // read current lock value
        beq     t0, 10b                 // try spinlock again if available
        br      zero, 22b               // spin in cache until available

#endif
        .end    ExInterlockedPushEntryList

        SBTTL("Interlocked Compare Exchange")
//++
//
// PVOID
// InterlockedCompareExchange (
//    IN OUT PVOID *Destination,
//    IN PVOID Exchange,
//    IN PVOID Comperand
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

10:                                     //

#if !defined(NT_UP)

        mb                              // synchronize memory access

#endif

        ldl_l   v0, 0(a0)               // get current value
        bis     a1, zero, t0            // copy exchange value for store
        cmpeq   v0, a2, t1              // check if operands match
        beq     t1, 20f                 // if eq, operands mismatch
        stl_c   t0, 0(a0)               // store updated addend value
        beq     t0,25f                  // if eq, store conditional failed

#if !defined(NT_UP)

        mb                              // synchronize memory access

#endif

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
// ExInterlockedAdd (
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

10:                                     //

#if !defined(NT_UP)

        mb                              // synchronize memory access

#endif

        ldl_l   v0, 0(a0)               // get current addend value - locked
        addl    v0, a1, t0              // increment addend value
        stl_c   t0, 0(a0)               // store updated value - conditionally
        beq     t0, 20f                 // if eq, conditonal store failed

#if !defined(NT_UP)

        mb                              // synchronize memory access

#endif

        ret     zero, (ra)              // return

//
// We expect the store conditional will usually succeed the first time so it
// is faster to branch forward (predicted not taken) to here and then branch
// backward (predicted taken) to where we wanted to go.
//

20:     br      zero, 10b               // go try spin lock again

        .end    InterlockedExchangeAdd

        SBTTL("Interlocked Pop Entry Sequenced List")
//++
//
// PSINGLE_LIST_ENTRY
// ExpInterlockedPopEntrySList (
//    IN PSLIST_HEADER ListHead
//    )
//
// Routine Description:
//
//    This function removes an entry from the front of a sequenced singly
//    linked list so that access to the list is synchronized in a MP system.
//    If there are no entries in the list, then a value of NULL is returned.
//    Otherwise, the address of the entry that is removed is returned as the
//    function value.
//
// Arguments:
//
//    ListHead (a0) - Supplies a pointer to the sequenced listhead from which
//       an entry is to be removed.
//
// Return Value:
//
//    The address of the entry removed from the list, or NULL if the list is
//    empty.
//
//--

        LEAF_ENTRY(ExpInterlockedPopEntrySList)

//
// N.B. The following code is the continuation address should a fault
//      occur in the rare case described below.
//

        ALTERNATE_ENTRY(ExpInterlockedPopEntrySListResume)

10:     ldq     t0, 0(a0)               // get next entry address and sequence

#if defined(_AXP64_)

        sra     t0, 63 - 42, v0         // extract next entry address
        bic     v0, 7, v0               //
        beq     v0, 30f                 // if eq, list is empty
        bis     t0, zero, t1            // copy depth and sequence

#else

        addl    t0, zero, v0            // sign extend next entry address
        beq     v0, 30f                 // if eq, list is empty
        srl     t0, 32, t1              // shift sequence to low 32-bits

#endif

#if !defined(NT_UP)

        mb                              // synchronize memory access

#endif

//
// N.B. It is possible for the following instruction to fault in the rare
//      case where the first entry in the list is allocated on another
//      processor and freed between the time the free pointer is read above
//      and the following instruction. When this happens, the access fault
//      code continues execution above at the resumption address and the
//      entire operation is retried.
//

        ALTERNATE_ENTRY(ExpInterlockedPopEntrySListFault)

        LDP     t5, 0(v0)               // get address of successor entry

#if defined(_AXP64_)

        sll     t5, 63 - 42, t2         // shift address into position

#else

        zapnot  t5, 0xf ,t2             // clear high 32-bits for merge

#endif

        ldq_l   t3, 0(a0)               // reload next entry address and sequence
        ldil    t5, 0xffff              // decrement list depth and
        addl    t1, t5, t1              // increment sequence number

#if defined(_AXP64_)

        zapnot  t1, 0x7, t1             // clear upper five bytes

#else

        sll     t1, 32, t1              // shift depth and sequence into position

#endif

        cmpeq   t0, t3, t4              // check if listhead has changed
        beq     t4, 15f                 // if eq, listhead changed
        bis     t1, t2, t1              // merge address, depth, and sequence
        stq_c   t1, 0(a0)               // store next entry address and sequence
        beq     t1, 15f                 // if eq, store conditional failed

#if !defined(NT_UP)

        mb                              // synchronize memory access

#endif

30:     ret     zero, (ra)              //

//
// Conditional store attempt failed or listhead changed.
//

15:     br      zero, 10b               // retry

        .end    ExpInterlockedPopEntrySList

        SBTTL("Interlocked Push Entry Sequenced List")
//++
//
// PSINGLE_LIST_ENTRY
// ExpInterlockedPushEntrySList (
//    IN PSLIST_HEADER ListHead,
//    IN PSINGLE_LIST_ENTRY ListEntry
//    )
//
// Routine Description:
//
//    This function inserts an entry at the head of a sequenced singly linked
//    list so that access to the list is synchronized in an MP system.
//
// Arguments:
//
//    ListHead (a0) - Supplies a pointer to the sequenced listhead into which
//       an entry is to be inserted.
//
//    ListEntry (a1) - Supplies a pointer to the entry to be inserted at the
//       head of the list.
//
// Return Value:
//
//    Previous contents of ListHead.  NULL implies list went from empty
//       to not empty.
//
//--

        LEAF_ENTRY(ExpInterlockedPushEntrySList)

10:     ldq     t0, 0(a0)               // get next entry address and sequence

#if defined(_AXP64_)

        sra     t0, 63 - 42, v0         // extract next entry address
        bic     v0, 7, v0               //
        bis     t0, zero, t1            // copy depth and sequence number

#else

        addl    t0, zero, v0            // sign extend next entry address
        srl     t0, 32, t1              // shift sequence to low 32-bits

#endif

        STP     v0, 0(a1)               // set next link in new first entry

#if !defined(NT_UP)

        mb                              // synchronize memory access

#endif

#if defined(_AXP64_)

        sll     a1, 63 - 42, t2         // shift address into position

#else

        zapnot  a1, 0xf, t2             // zero extend new first entry

#endif

        ldq_l   t3, 0(a0)               // reload next entry address and sequence
        ldah    t5, 1(zero)             // get sequence adjustment value
        addl    t1, 1, t1               // increment list depth
        addl    t1, t5, t1              // increment sequence number

#if defined(_AXP64_)

        zapnot  t1, 0x7, t1             // clear upper five bytes

#else

        sll     t1, 32, t1              // merge new first entry address and sequence

#endif

        cmpeq   t0, t3, t4              // check if listhead changed
        beq     t4, 15f                 // if eq, listhead changed
        bis     t1, t2, t2              // merge address, depth, and sequence
        stq_c   t2, 0(a0)               // store next entry address and sequence
        beq     t2, 15f                 // if eq, store conditional failed
        ret     zero, (ra)              // return

//
// Conditional store attempt failed or listhead changed.
//

15:     br      zero, 10b               // retry

        .end    ExpInterlockedPushEntrySList

        SBTTL("Interlocked Flush Sequenced List")
//++
//
// PSINGLE_LIST_ENTRY
// ExpInterlockedFlushSList (
//    IN PSLIST_HEADER ListHead
//    )
//
// Routine Description:
//
//    This function flushes the entire list of entries on a sequenced singly
//    linked list so that access to the list is synchronized in a MP system.
//    If there are no entries in the list, then a value of NULL is returned.
//    Otherwise, the address of the 1st entry on the list is returned as the
//    function value.
//
// Arguments:
//
//    ListHead (a0) - Supplies a pointer to the sequenced listhead from which
//       an entry is to be removed.
//
// Return Value:
//
//    The address of the entry removed from the list, or NULL if the list is
//    empty.
//
//--

        LEAF_ENTRY(ExpInterlockedFlushSList)

        and     t1, zero, t1            // set new listhead value
10:     ldq_l   t0, 0(a0)               // get next entry address and sequence
        stq_c	t1, 0(a0)               // store new listhead value
        beq     t1, 15f                 // if eq, store conditional failed

#if defined(_AXP64_)

        sra     t0, 63 - 42, v0         // extract next entry address
        bic     v0, 7, v0               //

#else

        addl    t0, zero, v0            // sign extend next entry address

#endif

        ret     zero, (ra)              // return

//
// Conditional store attempt failed or listhead changed.
//

15:     br      zero, 10b               // retry, store conditional failed

        .end    ExpInterlockedFlushSList

        SBTTL("Interlocked Compare Exchange 64-bits")
//++
//
// LONGLONG
// ExpInterlockedCompareExchange64 (
//    IN PLONGLONG Destination,
//    IN PLONGLONG Exchange,
//    IN PLONGLONG Comperand
//    )
//
// Routine Description:
//
//    This function performs an interlocked compare and exchange of 64-bits.
//
// Arguments:
//
//    Destination (a0) - Supplies a pointer to the destination variable.
//
//    Exchange (a1) - Supplies a pointer to the exchange value.
//
//    Comperand (a2) - Supplies a pointer to the comperand value.
//
// Return Value:
//
//    The current destination value are returned as the function value.
//
//--

        LEAF_ENTRY(ExpInterlockedCompareExchange64)

        ldq     t0, 0(a1)               // get exchange value
        ldq     t1, 0(a2)               // get comperand value
10:     ldq_l   v0, 0(a0)               // get current destination value
        bis     t0, zero, t2            // set exchange value
        cmpeq   v0, t1, t3              // check if current and comperand match
        beq     t3, 20f                 // if eq, current and comperand mismatch
        stq_c   t2, 0(a0)               // store exchange value
        beq     t2, 30f                 // if eq, store conditional failed
20:     ret     zero, (ra)

//
// Conditional store attempt failed.
//

30:     br      zero, 10b               // retry

        .end    ExpInterlockedCompareExchange64
