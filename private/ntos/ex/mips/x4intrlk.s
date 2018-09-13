//       TITLE("Interlocked Support")
//++
//
// Copyright (c) 1990  Microsoft Corporation
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
//--

#include "ksmips.h"

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
//    Addend (a1) - Supplies a pointer to a variable whose value is to be
//       adjusted by the increment value.
//
//    Increment (a2, a3) - Supplies the increment value to be added to the
//       addend variable.
//
//    Lock (4 * 4(sp)) - Supplies a pointer to a spin lock to be used to
//       synchronize access to the addend variable.
//
// Return Value:
//
//    The initial value of the addend variable is stored at the address
//    supplied by a0.
//
// Implementation Note:
//
//    The arithmetic for this function is performed as if this were an
//    unsigned large integer since this routine may not incur an overflow
//    exception.
//
//--

        LEAF_ENTRY(ExInterlockedAddLargeInteger)

        lw      t0,4 * 4(sp)            // get address of spin lock

5:      DISABLE_INTERRUPTS(t1)          // disable interrupts

#if !defined(NT_UP)

10:     ll      t2,0(t0)                // get current lock value
        move    t3,t0                   // set ownership value
        bne     zero,t2,20f             // if ne, spin lock owned
        sc      t3,0(t0)                // set spin lock owned
        beq     zero,t3,10b             // if eq, store conditional failed

#endif

        lw      t2,0(a1)                // get low part of addend value
        lw      t3,4(a1)                // get high part of addend value
        addu    a2,t2,a2                // add low parts of large integer
        addu    a3,t3,a3                // add high parts of large integer
        sltu    t4,a2,t2                // generate carry from low part
        addu    a3,a3,t4                // add carry to high part
        sw      a2,0(a1)                // store low part of result
        sw      a3,4(a1)                // store high part of result

#if !defined(NT_UP)

        sw      zero,0(t0)              // set spin lock not owned

#endif

        ENABLE_INTERRUPTS(t1)           // enable interrupts

        sw      t2,0(a0)                // set low part of initial value
        sw      t3,4(a0)                // set high part of initial value
        move    v0,a0                   // set function return register
        j       ra                      // return

#if !defined(NT_UP)

20:     ENABLE_INTERRUPTS(t1)           // enable interrupts
        b       5b                      // try again

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
//    The arithmetic for this function is performed as if this were an
//    unsigned large integer since this routine may not incur an overflow
//    exception.
//
//--

        LEAF_ENTRY(ExInterlockedAddLargeStatistic)

10:     lld     t0,0(a0)                // get large statistic value
        daddu   t0,t0,a1                // add increment
        scd     t0,0(a0)                // store large statistic value
        beq     zero,t0,10b             // if eq, store conditional failed
        j       ra                      //

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
//--

        LEAF_ENTRY(ExInterlockedAddUlong)


5:      DISABLE_INTERRUPTS(t0)          // disable interrupts

#if !defined(NT_UP)

10:     ll      t1,0(a2)                // get current lock value
        move    t2,a2                   // set ownership value
        bne     zero,t1,20f             // if ne, spin lock owned
        sc      t2,0(a2)                // set spin lock owned
        beq     zero,t2,10b             // if eq, store conditional failed

#endif

        lw      v0,0(a0)                // get initial addend value
        addu    t1,v0,a1                // compute adjusted value
        sw      t1,0(a0)                // set updated addend value

#if !defined(NT_UP)

        sw      zero,0(a2)              // set spin lock not owned

#endif

        ENABLE_INTERRUPTS(t0)           // enable interrupts

        j       ra                      // return

#if !defined(NT_UP)

20:     ENABLE_INTERRUPTS(t0)           // enable interrupts

        b       5b                      // try again

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
//         is MIPS target specific and whose prototype does not include the
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

        ALTERNATE_ENTRY(ExMipsInterlockedExchangeUlong)

10:     ll      v0,0(a0)                // get current source value
        move    t1,a1                   // set exchange value
        sc      t1,0(a0)                // set new source value
        beq     zero,t1,10b             // if eq, store conditional failed

        j       ra                      // return

        .end    ExInterlockedExchangeUlong

        SBTTL("Interlocked Exchange Add Large Integer")
//++
//
// LARGE_INTEGER
// ExpInterlockedExchangeAddLargeInteger (
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
//    Addend (a1) - Supplies a pointer to a variable whose value is to be
//       adjusted by the increment value.
//
//    Increment (a2, a3) - Supplies the increment value to be added to the
//       addend variable.
//
// Return Value:
//
//    The initial value of the addend variable is stored at the address
//    supplied by a0.
//
// Implementation Note:
//
//    The arithmetic for this function is performed as if this were an
//    unsigned large integer since this routine may not incur an overflow
//    exception.
//
//--

        LEAF_ENTRY(ExpInterlockedExchangeAddLargeInteger)

        dsll    a2,a2,32                // merge low and high parts of
        dsrl    a2,a2,32                // increment value
        dsll    a3,a3,32                //
        or      a2,a2,a3                //
10:     lld     a3,0(a1)                // get addend value
        daddu   v0,a3,a2                // add increment
        scd     v0,0(a1)                // store addend value
        beq     zero,v0,10b             // if eq, store conditional failed
        sd      a3,0(a0)                //
        move    v0,a0                   // set function return register
        j       ra                      // return

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
//         is MIPS target specific and whose prototype does not include the
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
//--

        LEAF_ENTRY(ExInterlockedDecrementLong)

        ALTERNATE_ENTRY(ExMipsInterlockedDecrementLong)

10:     ll      t1,0(a0)                // get current addend value
        subu    t2,t1,1                 // decrement addend value
        sc      t2,0(a0)                // set new addend value
        beq     zero,t2,10b             // if eq, store conditional failed
        subu    v0,t1,1                 // decrement addend value
        sltu    t0,zero,v0              // check if result is nonzero
        sra     v0,v0,31                // sign extend result value
        subu    v0,v0,t0                // compute negative, zero, or positive

        j       ra                      // return

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
//         is MIPS target specific and whose prototype does not include the
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
//--

        LEAF_ENTRY(ExInterlockedIncrementLong)

        ALTERNATE_ENTRY(ExMipsInterlockedIncrementLong)

10:     ll      t1,0(a0)                // get current addend value
        addu    t2,t1,1                 // increment addend value
        sc      t2,0(a0)                // set new addend value
        beq     zero,t2,10b             // if eq, store conditional failed
        addu    v0,t1,1                 // increment addend value
        sltu    t0,zero,v0              // check if result is nonzero
        sra     v0,v0,31                // sign extend result value
        subu    v0,v0,t0                // compute negative, zero, or positive

        j       ra                      // return

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

5:      DISABLE_INTERRUPTS(t0)          // disable interrupts

#if !defined(NT_UP)

10:     ll      t2,0(a2)                // get current lock value
        move    t3,a2                   // set ownership value
        bne     zero,t2,20f             // if ne, spin lock owned
        sc      t3,0(a2)                // set spin lock owned
        beq     zero,t3,10b             // if eq, store conditional failed

#endif

        lw      t2,LsFlink(a0)          // get address of next entry
        sw      t2,LsFlink(a1)          // store next link in entry
        sw      a0,LsBlink(a1)          // store previous link in entry
        sw      a1,LsBlink(t2)          // store previous link in next
        sw      a1,LsFlink(a0)          // store next link in head
        xor     v0,t2,a0                // check if list was empty
        beq     v0,zero,15f             // if eq, list was null
        move    v0,t2                   // return previous entry at head
15:

#if !defined(NT_UP)

        sw      zero,0(a2)              // set spin lock not owned

#endif

        ENABLE_INTERRUPTS(t0)           // enable interrupts

        j       ra                      // return

#if !defined(NT_UP)

20:     ENABLE_INTERRUPTS(t0)           // enable interrupts

        b       5b                      // try again

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

5:      DISABLE_INTERRUPTS(t0)          // disable interrupts

#if !defined(NT_UP)

10:     ll      t2,0(a2)                // get current lock value
        move    t3,a2                   // set ownership value
        bne     zero,t2,20f             // if ne, spin lock owned
        sc      t3,0(a2)                // set spin lock owned
        beq     zero,t3,10b             // if eq, store conditional failed

#endif

        lw      t2,LsBlink(a0)          // get address of previous entry
        sw      a0,LsFlink(a1)          // store next link in entry
        sw      t2,LsBlink(a1)          // store previous link in entry
        sw      a1,LsBlink(a0)          // store previous link in next
        sw      a1,LsFlink(t2)          // store next link in head
        xor     v0,t2,a0                // check is list was emptyr
        beq     v0,zero,15f             // if eq, list was empty
        move    v0,t2                   // return previous entry at tail
15:

#if !defined(NT_UP)

        sw      zero,0(a2)              // set spin lock not owned

#endif

        ENABLE_INTERRUPTS(t0)           // enable interrupts

        j       ra                      // return


#if !defined(NT_UP)

20:     ENABLE_INTERRUPTS(t0)           // enable interrupts

        b       5b                      // try again

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

5:      DISABLE_INTERRUPTS(t0)          // disable interrupts

#if !defined(NT_UP)

10:     ll      t2,0(a1)                // get current lock value
        move    t3,a1                   // set ownership value
        bne     zero,t2,30f             // if ne, spin lock owned
        sc      t3,0(a1)                // set spin lock owned
        beq     zero,t3,10b             // if eq, store conditional failed

#endif

        lw      t2,LsFlink(a0)          // get address of next entry
        move    v0,zero                 // assume list is empty
        beq     t2,a0,20f               // if eq, list is empty
        lw      t3,LsFlink(t2)          // get address of next entry
        sw      t3,LsFlink(a0)          // store address of next in head
        sw      a0,LsBlink(t3)          // store address of previous in next
        move    v0,t2                   // set address of entry removed
20:                                     //

#if !defined(NT_UP)

        sw      zero,0(a1)              // set spin lock not owned

#endif

        ENABLE_INTERRUPTS(t0)           // enable interrupts

        j       ra                      // return


#if !defined(NT_UP)

30:     ENABLE_INTERRUPTS(t0)           // enable interrupts

        b       5b                      // try again

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
//    This function removes an entry from the front of a singly linked list
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

5:      DISABLE_INTERRUPTS(t0)          // disable interrupts

#if !defined(NT_UP)

10:     ll      t2,0(a1)                // get current lock value
        move    t3,a1                   // set ownership value
        bne     zero,t2,30f             // if ne, spin lock owned
        sc      t3,0(a1)                // set spin lock owned
        beq     zero,t3,10b             // if eq, store conditional failed

#endif

        lw      v0,0(a0)                // get address of next entry
        beq     zero,v0,20f             // if eq, list is empty
        lw      t2,0(v0)                // get address of next entry
        sw      t2,0(a0)                // store address of next in head
20:                                     //

#if !defined(NT_UP)

        sw      zero,0(a1)              // set spin lock not owned

#endif

        ENABLE_INTERRUPTS(t0)           // enable interrupts

        j       ra                      // return

#if !defined(NT_UP)

30:     ENABLE_INTERRUPTS(t0)           // enable interrupts

        b       5b                      // try again

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

5:      DISABLE_INTERRUPTS(t0)          // disable interrupts

#if !defined(NT_UP)

10:     ll      t2,0(a2)                // get current lock value
        move    t3,a2                   // set ownership value
        bne     zero,t2,20f             // if ne, spin lock owned
        sc      t3,0(a2)                // set spin lock owned
        beq     zero,t3,10b             // if eq, store conditional failed

#endif

        lw      v0,0(a0)                // get address of first entry (return value also)
        sw      v0,0(a1)                // set address of next in new entry
        sw      a1,0(a0)                // set address of first entry

#if !defined(NT_UP)

        sw      zero,0(a2)              // set spin lock not owned

#endif

        ENABLE_INTERRUPTS(t0)           // enable interrupts

        j       ra                      // return

#if !defined(NT_UP)

20:     ENABLE_INTERRUPTS(t0)           // enable interrupts

        b       5b                      // try again

#endif

        .end    ExInterlockedPushEntryList

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

        .set    noreorder
        .set    noat

//
// N.B. The following code is the continuation address should a fault
//      occur in the rare case described below.
//

        ALTERNATE_ENTRY(ExpInterlockedPopEntrySListResume)

10:     ld      t0,0(a0)                // get next entry address and sequence
20:     dsll    v0,t0,32                // sign extend next entry address
        dsra    v0,v0,32                //
        beq     zero,v0,30f             // if eq, list is empty
        dsrl    t1,t0,32                // shift sequence to low 32-bits

//
// N.B. It is possible for the following instruction to fault in the rare
//      case where the first entry in the list is allocated on another
//      processor and freed between the time the free pointer is read above
//      and the following instruction. When this happens, the access fault
//      code continues execution above at the resumption address and the
//      entire operation is retried.
//

        ALTERNATE_ENTRY(ExpInterlockedPopEntrySListFault)

        lwu     t2,0(v0)                // get address of successor entry
        lld     t3,0(a0)                // reload next entry address and sequence
        li      t4,0xffff               // decrement list depth and
        addu    t1,t1,t4                // increment sequence number
        dsll    t1,t1,32                // merge successor address and sequence
        bne     t0,t3,10b               // if ne, listhead has changed
        or      t1,t1,t2                //
        scd     t1,0(a0)                // store next emtry address and sequence
        beql    zero,t1,20b             // if eq, store conditional failed
        ld      t0,0(a0)                // get next entry address and sequence
        .set    at
        .set    reorder

30:     j       ra                      // return

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

        .set    noreorder
        .set    noat
10:     ld      t0,0(a0)                // get next entry address and sequence
20:     dsll    v0,t0,32                // sign extend next entry address
        dsra    v0,v0,32                //
        dsrl    t1,t0,32                // shift sequence to low 32-bits
        sw      v0,0(a1)                // set next link in new first entry
        dsll    t2,a1,32                // zero extend new first entry
        dsrl    t2,t2,32                //
        lld     t3,0(a0)                // reload next entry address and sequence
        lui     t4,1                    // get sequence adjustment value
        addu    t1,t1,1                 // increment list depth
        addu    t1,t1,t4                // increment sequence number
        dsll    t1,t1,32                // merge new first entry address and sequence
        bne     t0,t3,10b               // if ne, listhead has changed
        or      t1,t1,t2                //
        scd     t1,0(a0)                // store next emtry address and sequence
        beql    zero,t1,20b             // if eq, store conditional failed
        ld      t0,0(a0)                // get next entry address and sequence
        .set    at
        .set    reorder

        j       ra                      // return

        .end    ExpInterlockedPushEntrySList

        SBTTL("Interlocked Compare Exchange 64-bits")
//++
//
// ULONGLONG
// ExpInterlockedCompareExchange64 (
//    IN PULONGLONG Destination,
//    IN PULONGLONG Exchange,
//    IN PULONGLONG Comperand
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

        .set    noreorder
        .set    noat
        ld      t0,0(a1)                // get exchange value
        ld      t1,0(a2)                // get comperand value
        lld     v0,0(a0)                // get current destination value
10:     move    t2,t0                   // set exchange value
        bne     v0,t1,20f               // if ne, current and comperand mismatch
        dsra    v1,v0,32                // extract high part of result
        scd     t2,0(a0)                // store exchange value
        beql    zero,t2,10b             // if eq, store conditional failed
        lld     v0,0(a0)                // get next entry address and sequence
20:     dsll    v0,v0,32                // extract low part of result
        j       ra                      // return
        dsra    v0,v0,32                //
        .set    at
        .set    reorder

        .end    ExpInterlockedCompareExchange64
