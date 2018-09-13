//       TITLE("Interlocked Support")
//++
//
// Copyright (c) 1993  IBM Corporation
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
//    Chuck Bauman 3-Sep-1993
//
// Environment:
//
//    Kernel mode.
//
// Revision History:
//
//--

#include "ksppc.h"

#if COLLECT_PAGING_DATA
        .extern KeNumberProcessors
        .extern KiProcessorBlock
        .extern ..RtlCopyMemory
#endif


//      SBTTL("Interlocked Add Large Integer")
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
//    Addend (r4) - Supplies a pointer to a variable whose value is to be
//       adjusted by the increment value.
//
//    Increment (r5, r6) - Supplies the increment value to be added to the
//       addend variable.
//
//    Lock (r7) - Supplies a pointer to a spin lock to be used to
//       synchronize access to the addend variable.
//
// Return Value:
//
//    The initial value of the addend variable is stored at the address
//    supplied by r3.
//
// Implementation Note:
//
//    The arithmetic for this function is performed as if this were an
//    unsigned large integer since this routine may not incur an overflow
//    exception.
//
//--

        LEAF_ENTRY(ExInterlockedAddLargeInteger)

//
// N.B.  ExInterlockedExchangeAddLargeInteger is the same as
//       ExInterlockedAddLargeInteger on 32-bit machines.
//       On 64-bit machines, an optimized version of the Exchange
//       version could be implemented using ldarx and stdcx.
//       The optimized version wouldn't need to take the spin lock.
//

        ALTERNATE_ENTRY(ExInterlockedExchangeAddLargeInteger)

        DISABLE_INTERRUPTS(r.8,r.12)    // disable interrupts
                                        // r.8  <- previous msr value
                                        // r.12 <- new (disabled) msr

#if !defined(NT_UP)

        ACQUIRE_SPIN_LOCK(r.7, r.7, r.9, addli, addliw)
#endif

        lwz     r.10,0(r.4)             // get low part of addend value
        lwz     r.11,4(r.4)             // get high part of addend value
        addc    r.5,r.10,r.5            // add low parts of large integer,(CA?)
        adde    r.6,r.11,r.6            // add high parts of large integer + CA
        stw     r.5,0(r.4)              // store low part of result
        stw     r.6,4(r.4)              // store high part of result

#if !defined(NT_UP)
        RELEASE_SPIN_LOCK(r.7, r.9)
#endif

        ENABLE_INTERRUPTS(r.8)          // enable interrupts

        stw     r.10,0(r.3)             // set low part of initial value
        stw     r.11,4(r.3)             // set high part of initial value

        blr                             // return

#if !defined(NT_UP)
        SPIN_ON_SPIN_LOCK_ENABLED(r.7, r.9, addli, addliw, addlix, r.8, r.12)
#endif

        DUMMY_EXIT(ExInterlockedAddLargeInteger)

//      SBTTL("Interlocked Add Large Statistic")
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
//    Addend (r.3) - Supplies a pointer to a variable whose value is to be
//       adjusted by the increment value.
//
//    Increment (r.4) - Supplies the increment value to be added to the
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

storelsfailed:
        lwarx   r.8, 0, r.3             // get low part of large statistic
        addc    r.9, r.8, r.4           // add increment to low part
        stwcx.  r.9, 0, r.3             // store result of low part add
        bne-    storelsfailed           // if ne, store conditional failed

        subfe.  r.0, r.0, r.0           // check carry bit (result is 0
                                        //  if CA is set)
        bnelr+                          // if ne, carry clear, so return

        li      r.10, 4                 // high part offset

storels2failed:
        lwarx   r.8, r.10, r.3          // get high part of large statistic
        addi    r.8, r.8,  1            // add carry to high part
        stwcx.  r.8, r.10, r.3          // store result of high part add
        bne-    storels2failed          // if ne, store conditional failed

        LEAF_EXIT(ExInterlockedAddLargeStatistic)

//      SBTTL("Interlocked Add Unsigned Long")
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
//    Addend (r.3) - Supplies a pointer to a variable whose value is to be
//       adjusted by the increment value.
//
//    Increment (r.4) - Supplies the increment value to be added to the
//       addend variable.
//
//    Lock (r.5) - Supplies a pointer to a spin lock to be used to synchronize
//       access to the addend variable.
//
// Return Value:
//
//    The initial value of the addend variable.
//
//--

        LEAF_ENTRY(ExInterlockedAddUlong)

        ori     r.6,r.3,0               // move addend address

        DISABLE_INTERRUPTS(r.8,r.12)    // disable interrupts
                                        // r.8  <- previous msr value
                                        // r.12 <- new (disabled) msr

#if !defined(NT_UP)

        ACQUIRE_SPIN_LOCK(r.5, r.5, r.9, addul, addulw)
#endif

        lwz     r.3,0(r.6)              // get initial addend value
        add     r.4,r.3,r.4             // compute adjusted value
        stw     r.4,0(r.6)              // set updated addend value

#if !defined(NT_UP)
        RELEASE_SPIN_LOCK(r.5, r.9)
#endif


        ENABLE_INTERRUPTS(r.8)          // enable interrupts

        blr                             // return

#if !defined(NT_UP)
        SPIN_ON_SPIN_LOCK_ENABLED(r.5, r.9, addul, addulw, addulx, r.8, r.12)
#endif

        DUMMY_EXIT(ExInterlockedAddUlong)

//      SBTTL("Interlocked Exchange Unsigned Long")
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
//         is PPC target specific and whose prototype does not include the
//         spinlock parameter. Since the routine never refers to the spinlock
//         parameter, no additional code is required.
//
//         There is an additional alternate entry point, InterlockedExchange,
//         which is not platform-specific and which also does not take the
//         spinlock parameter.
//
// Arguments:
//
//    Source (r.3) - Supplies a pointer to a variable whose value is to be
//       exchanged.
//
//    Value (r.4) - Supplies the value to exchange with the source value.
//
//    Lock (r.5) - Supplies a pointer to a spin lock to be used to synchronize
//       access to the source variable.
//
// Return Value:
//
//    The source value is returned as the function value.
//
//--

        LEAF_ENTRY(ExInterlockedExchangeUlong)

        ALTERNATE_ENTRY(ExPpcInterlockedExchangeUlong)
        ALTERNATE_ENTRY(InterlockedExchange)

exchgfailed:
        lwarx   r.5,0,r.3               // get current source value
        stwcx.  r.4,0,r.3               // set new source value
        bne-    exchgfailed             // if ne, store conditional failed

        ori     r.3,r.5,0               // return old value

        LEAF_EXIT(ExInterlockedExchangeUlong)  // return

//      SBTTL("Interlocked Decrement Long")
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
//         is PPC target specific and whose prototype does not include the
//         spinlock parameter. Since the routine never refers to the spinlock
//         parameter, no additional code is required.
//
// Arguments:
//
//    Addend (r.3) - Supplies a pointer to a variable whose value is to be
//       decremented.
//
//    Lock (r.4) - Supplies a pointer to a spin lock to be used to synchronize
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

        ALTERNATE_ENTRY(ExPpcInterlockedDecrementLong)

decfailed:
        lwarx   r.5,0,r.3               // get current addend value
        subi    r.4,r.5,1               // decrement addend value
        stwcx.  r.4,0,r.3               // set new addend value
        bne-    decfailed               // if ne, store conditional failed

        addic.  r.5,r.5,-1              // Reset CR0
        mfcr    r.3                     // Rotate bit 0 & 1 and right justify
        rlwinm  r.3,r.3,2,30,31         // 0 = 0, 1 = positive, 2 = negative
        neg     r.3,r.3                 // 0 = 0, -1 = positive, -2 = negative

        LEAF_EXIT(ExInterlockedDecrementLong)  // return

//      SBTTL("Interlocked Decrement")
//++
//
// LONG
// InterlockedDecrement (
//    IN PLONG Addend
//    )
//
// Routine Description:
//
//    This function performs an interlocked decrement on an addend variable
//    of type signed long. The result is returned as the function value.
//
// Arguments:
//
//    Addend (r.3) - Supplies a pointer to a variable whose value is to be
//       decremented.
//
// Return Value:
//
//    (r.3) The result of the decrement is returned.
//
//--

        LEAF_ENTRY(InterlockedDecrement)

dec2failed:
        lwarx   r.5,0,r.3               // get current addend value
        subi    r.4,r.5,1               // decrement addend value
        stwcx.  r.4,0,r.3               // set new addend value
        bne-    dec2failed              // if ne, store conditional failed

        ori     r.3,r.4,0               // return result

        LEAF_EXIT(InterlockedDecrement) // return

//      SBTTL("Interlocked Increment Long")
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
//         is PPC target specific and whose prototype does not include the
//         spinlock parameter. Since the routine never refers to the spinlock
//         parameter, no additional code is required.
//
// Arguments:
//
//    Addend (r.3) - Supplies a pointer to a variable whose value is to be
//       incremented.
//
//    Lock (r.4) - Supplies a pointer to a spin lock to be used to synchronize
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

        ALTERNATE_ENTRY(ExPpcInterlockedIncrementLong)

incfailed:
        lwarx   r.5,0,r.3               // get current addend value
        addi    r.4,r.5,1               // increment addend value
        stwcx.  r.4,0,r.3               // set new addend value
        bne-    incfailed               // if ne, store conditional failed

        addic.  r.5,r.5,1               // Reset CR0
        mfcr    r.3                     // Rotate bit 0 & 1 and right justify
        rlwinm  r.3,r.3,2,30,31         // 0 = 0, 1 = positive, 2 = negative
        neg     r.3,r.3                 // 0 = 0, -1 = positive, -2 = negative

        LEAF_EXIT(ExInterlockedIncrementLong)   // return

//      SBTTL("Interlocked Increment")
//++
//
// LONG
// InterlockedIncrement (
//    IN PLONG Addend
//    )
//
// Routine Description:
//
//    This function performs an interlocked increment on an addend variable
//    of type signed long. The result is returned as the function value.
//
// Arguments:
//
//    Addend (r.3) - Supplies a pointer to a variable whose value is to be
//       incremented.
//
// Return Value:
//
//    (r.3) The result of the increment is returned.
//
//--

        LEAF_ENTRY(InterlockedIncrement)

inc2failed:
        lwarx   r.5,0,r.3               // get current addend value
        addi    r.4,r.5,1               // increment addend value
        stwcx.  r.4,0,r.3               // set new addend value
        bne-    inc2failed              // if ne, store conditional failed

        ori     r.3,r.4,0               // return result

        LEAF_EXIT(InterlockedIncrement) // return

//      SBTTL("Interlocked Compare Exchange")
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
//    Destination (r.3) - Supplies a pointer to the destination value.
//
//    Exchange (r.4) - Supplies the exchange.
//
//    Comperand (r.5) - Supplies the comperand value.
//
// Return Value:
//
//    The initial destination value is returned as the function value.
//
//--

        LEAF_ENTRY(InterlockedCompareExchange)

Icmp10: lwarx   r.6,0,r.3               // get current operand value
        cmpw    r.6,r.5                 // compare with comperand
        bne-    Icmp20                  // if ne, compare failed
        stwcx.  r.4,0,r.3               // set new operand value
        bne-    Icmp10                  // if ne, store conditional failed

Icmp20: ori     r.3,r.6,0               // return result

        LEAF_EXIT(InterlockedCompareExchange) // return

//      SBTTL("Interlocked Exchange Add")
//++
//
// LONG
// InterlockedExchangeAdd (
//    IN PLONG Addend,
//    IN LONG Increment
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
//    Addend (r.3) - Supplies a pointer to a variable whose value is to be
//       adjusted by the increment value.
//
//    Increment (r.4) - Supplies the increment value to be added to the
//       addend variable.
//
// Return Value:
//
//    The initial value of the addend variable.
//
//--

        LEAF_ENTRY(InterlockedExchangeAdd)

add1failed:
        lwarx   r.5,0,r.3               // get current addend value
        add     r.6,r.5,r.4             // increment addend value
        stwcx.  r.6,0,r.3               // set new addend value
        bne-    add1failed              // if ne, store conditional failed

        ori     r.3,r.5,0               // return result

        LEAF_EXIT(InterlockedExchangeAdd) // return

//      SBTTL("Interlocked Insert Head List")
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
//    ListHead (r.3) - Supplies a pointer to the head of the doubly linked
//       list into which an entry is to be inserted.
//
//    ListEntry (r.4) - Supplies a pointer to the entry to be inserted at the
//       head of the list.
//
//    Lock (r.5) - Supplies a pointer to a spin lock to be used to synchronize
//       access to the list.
//
// Return Value:
//
//    Pointer to entry that was at the head of the list or NULL if the list
//    was empty.
//
//--

        LEAF_ENTRY(ExInterlockedInsertHeadList)

        ori     r.6,r.3,0               // move listhead address

        DISABLE_INTERRUPTS(r.8,r.12)    // disable interrupts
                                        // r.8  <- previous msr value
                                        // r.12 <- new (disabled) msr

#if !defined(NT_UP)

        ACQUIRE_SPIN_LOCK(r.5, r.5, r.9, inshl, inshlw)
#endif

        lwz     r.7,LsFlink(r.6)        // get address of next entry
        stw     r.6,LsBlink(r.4)        // store previous link in entry
        stw     r.7,LsFlink(r.4)        // store next link in entry
        xor.    r.3,r.7,r.6             // check if list was empty
        stw     r.4,LsBlink(r.7)        // store previous link in next
        stw     r.4,LsFlink(r.6)        // store next link in head
        beq     nullhlist               // if eq, list was null
        ori     r.3,r.7,0               // return previous entry at head
nullhlist:

#if !defined(NT_UP)
        RELEASE_SPIN_LOCK(r.5, r.9)
#endif

        ENABLE_INTERRUPTS(r.8)          // enable interrupts

        blr                             // return

#if !defined(NT_UP)
        SPIN_ON_SPIN_LOCK_ENABLED(r.5, r.9, inshl, inshlw, inshlx, r.8, r.12)
#endif

        DUMMY_EXIT(ExInterlockedInsertHeadList)


//      SBTTL("Interlocked Insert Tail List")
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
//    ListHead (r.3) - Supplies a pointer to the head of the doubly linked
//       list into which an entry is to be inserted.
//
//    ListEntry (r.4) - Supplies a pointer to the entry to be inserted at the
//       tail of the list.
//
//    Lock (r.5) - Supplies a pointer to a spin lock to be used to synchronize
//       access to the list.
//
// Return Value:
//
//    Pointer to entry that was at the tail of the list or NULL if the list
//    was empty.
//
//--

        LEAF_ENTRY(ExInterlockedInsertTailList)

        ori     r.6,r.3,0               // move listhead address

        DISABLE_INTERRUPTS(r.8,r.12)    // disable interrupts
                                        // r.8  <- previous msr value
                                        // r.12 <- new (disabled) msr

#if !defined(NT_UP)

        ACQUIRE_SPIN_LOCK(r.5, r.5, r.9, instl, instlw)
#endif

        lwz     r.7,LsBlink(r.6)        // get address of previous entry
        stw     r.6,LsFlink(r.4)        // store next link in entry
        stw     r.7,LsBlink(r.4)        // store previous link in entry
        xor.    r.3,r.7,r.6             // check if list was empty
        stw     r.4,LsBlink(r.6)        // store previous link in next
        stw     r.4,LsFlink(r.7)        // store next link in head
        beq     nulltlist               // if eq, list was empty
        ori     r.3,r.7,0               // return previous entry at tail
nulltlist:

#if !defined(NT_UP)
        RELEASE_SPIN_LOCK(r.5, r.9)
#endif

        ENABLE_INTERRUPTS(r.8)          // enable interrupts

        blr                             // return

#if !defined(NT_UP)
        SPIN_ON_SPIN_LOCK_ENABLED(r.5, r.9, instl, instlw, instlx, r.8, r.12)
#endif

        DUMMY_EXIT(ExInterlockedInsertTailList)

//      SBTTL("Interlocked Remove Head List")
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
//    ListHead (r.3) - Supplies a pointer to the head of the doubly linked
//       list from which an entry is to be removed.
//
//    Lock (r.4) - Supplies a pointer to a spin lock to be used to synchronize
//       access to the list.
//
// Return Value:
//
//    The address of the entry removed from the list, or NULL if the list is
//    empty.
//
//--

        LEAF_ENTRY(ExInterlockedRemoveHeadList)

        ori     r.6,r.3,0               // move listhead address

        DISABLE_INTERRUPTS(r.8,r.12)    // disable interrupts
                                        // r.8  <- previous msr value
                                        // r.12 <- new (disabled) msr

#if !defined(NT_UP)

        ACQUIRE_SPIN_LOCK(r.4, r.4, r.9, remhl, remhlw)
#endif

        lwz     r.7,LsFlink(r.6)        // get address of next entry
        xor.    r.3,r.7,r.6             // check if list is empty
        beq     nullrlist               // if eq, list is empty
        lwz     r.10,LsFlink(r.7)       // get address of next entry
        ori     r.3,r.7,0               // set address of entry removed
        stw     r.10,LsFlink(r.6)       // store address of next in head
        stw     r.6,LsBlink(r.10)       // store address of previous in next
nullrlist:

#if !defined(NT_UP)
        RELEASE_SPIN_LOCK(r.4, r.9)
#endif

        ENABLE_INTERRUPTS(r.8)          // enable interrupts

        blr                             // return

#if !defined(NT_UP)
        SPIN_ON_SPIN_LOCK_ENABLED(r.4, r.9, remhl, remhlw, remhlx, r.8, r.12)
#endif

        DUMMY_EXIT(ExInterlockedRemoveHeadList)

//      SBTTL("Interlocked Pop Entry List")
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
//    ListHead (r.3) - Supplies a pointer to the head of the singly linked
//       list from which an entry is to be removed.
//
//    Lock (r.4) - Supplies a pointer to a spin lock to be used to synchronize
//       access to the list.
//
// Return Value:
//
//    The address of the entry removed from the list, or NULL if the list is
//    empty.
//
//--

        LEAF_ENTRY(ExInterlockedPopEntryList)

        ori     r.6,r.3,0               // move listhead address

        DISABLE_INTERRUPTS(r.8,r.12)    // disable interrupts
                                        // r.8  <- previous msr value
                                        // r.12 <- new (disabled) msr

#if !defined(NT_UP)

        ACQUIRE_SPIN_LOCK(r.4, r.4, r.9, popel, popelw)
#endif

        lwz     r.3,0(r.6)              // get address of next entry
        cmpwi   r.3,0                   // check if list is empty
        beq     nullplist               // if eq, list is empty
        lwz     r.5,0(r.3)              // get address of next entry
        stw     r.5,0(r.6)              // store address of next in head
nullplist:

#if !defined(NT_UP)
        RELEASE_SPIN_LOCK(r.4, r.9)
#endif

        ENABLE_INTERRUPTS(r.8)          // enable interrupts

        blr                             // return

#if !defined(NT_UP)
        SPIN_ON_SPIN_LOCK_ENABLED(r.4, r.9, popel, popelw, popelx, r.8, r.12)
#endif

        DUMMY_EXIT(ExInterlockedPopEntryList)

//      SBTTL("Interlocked Push Entry List")
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
//    ListHead (r.3) - Supplies a pointer to the head of the singly linked
//       list into which an entry is to be inserted.
//
//    ListEntry (r.4) - Supplies a pointer to the entry to be inserted at the
//       head of the list.
//
//    Lock (r.5) - Supplies a pointer to a spin lock to be used to synchronize
//       access to the list.
//
// Return Value:
//
//    Previous contents of ListHead.  NULL implies list went from empty
//       to not empty.
//
//--

        LEAF_ENTRY(ExInterlockedPushEntryList)

        ori     r.6,r.3,0               // move listhead address

        DISABLE_INTERRUPTS(r.8,r.12)    // disable interrupts
                                        // r.8  <- previous msr value
                                        // r.12 <- new (disabled) msr

#if !defined(NT_UP)

        ACQUIRE_SPIN_LOCK(r.5, r.5, r.9, pushel, pushelw)
#endif

        lwz     r.3,0(r.6)              // get address of first entry (return value also)
        stw     r.4,0(r.6)              // set address of first entry
        stw     r.3,0(r.4)              // set address of next in new entry

#if !defined(NT_UP)
        RELEASE_SPIN_LOCK(r.5, r.9)
#endif

        ENABLE_INTERRUPTS(r.8)          // enable interrupts

        blr                             // return

#if !defined(NT_UP)
        SPIN_ON_SPIN_LOCK_ENABLED(r.5, r.9, pushel, pushelw, pushelx, r.8, r.12)
#endif

        DUMMY_EXIT(ExInterlockedPushEntryList)

//      SBTTL("Interlocked Pop Entry Sequenced List")
//++
//
// PSINGLE_LIST_ENTRY
// ExInterlockedPopEntrySList (
//    IN PSLIST_HEADER ListHead,
//    IN PKSPIN_LOCK Lock
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
//    ListHead (r.3) - Supplies a pointer to the sequenced listhead from which
//       an entry is to be removed.
//
//    Lock (r.4) - Supplies a pointer to a spin lock to be used to synchronize
//       access to the list.
//
// Return Value:
//
//    The address of the entry removed from the list, or NULL if the list is
//    empty.
//
//--

        LEAF_ENTRY(ExInterlockedPopEntrySList)

        ori     r.6,r.3,0               // move listhead address

        DISABLE_INTERRUPTS(r.8,r.12)    // disable interrupts
                                        // r.8  <- previous msr value
                                        // r.12 <- new (disabled) msr

        li      r.0,0                   // zero r.0

#if !defined(NT_UP)

        ACQUIRE_SPIN_LOCK(r.4, r.4, r.9, popesl, popeslw)
#endif

        lwz     r.3,0(r.6)              // get address of next entry
        ori     r.0,r.0,0xffff          // set r.0 to 0x0000ffff
        cmpwi   r.3,0                   // check if list is empty
        lwz     r.7,4(r.6)              // get depth and sequence number
        beq     nullpslist              // if eq, list is empty

        lwz     r.5,0(r.3)              // get address of next entry
        add     r.7,r.7,r.0             // decrement depth and increment
                                        //  sequence number
        stw     r.5,0(r.6)              // store address of next in head
        stw     r.7,4(r.6)              // store new depth and sequence number

nullpslist:

#if !defined(NT_UP)
        RELEASE_SPIN_LOCK(r.4, r.9)
#endif

        ENABLE_INTERRUPTS(r.8)          // enable interrupts

        blr                             // return

#if !defined(NT_UP)
        SPIN_ON_SPIN_LOCK_ENABLED(r.4, r.9, popesl, popeslw, popeslx, r.8, r.12)
#endif

        DUMMY_EXIT(ExInterlockedPopEntrySList)

//      SBTTL("Interlocked Push Entry Sequenced List")
//++
//
// PSINGLE_LIST_ENTRY
// ExInterlockedPushEntrySList (
//    IN PSLIST_HEADER ListHead,
//    IN PSINGLE_LIST_ENTRY ListEntry,
//    IN PKSPIN_LOCK Lock
//    )
//
// Routine Description:
//
//    This function inserts an entry at the head of a sequenced singly linked
//    list so that access to the list is synchronized in an MP system.
//
// Arguments:
//
//    ListHead (r.3) - Supplies a pointer to the sequenced listhead into which
//       an entry is to be inserted.
//
//    ListEntry (r.4) - Supplies a pointer to the entry to be inserted at the
//       head of the list.
//
//    Lock (r.5) - Supplies a pointer to a spin lock to be used to synchronize
//       access to the list.
//
// Return Value:
//
//    Previous contents of ListHead.  NULL implies list went from empty
//       to not empty.
//
//--

        LEAF_ENTRY(ExInterlockedPushEntrySList)

        ori     r.6,r.3,0               // move listhead address

        DISABLE_INTERRUPTS(r.8,r.12)    // disable interrupts
                                        // r.8  <- previous msr value
                                        // r.12 <- new (disabled) msr

#if !defined(NT_UP)

        ACQUIRE_SPIN_LOCK(r.5, r.5, r.9, pushesl, pusheslw)
#endif

        lwz     r.7,4(r.6)              // get depth and sequence number
        lwz     r.3,0(r.6)              // get address of first entry (return value also)
        addi    r.7,r.7,1               // increment depth
        stw     r.4,0(r.6)              // set address of first entry
        addis   r.7,r.7,1               // increment sequence number
        stw     r.3,0(r.4)              // set address of next in new entry
        stw     r.7,4(r.6)              // store new depth and sequence number

#if !defined(NT_UP)
        RELEASE_SPIN_LOCK(r.5, r.9)
#endif

        ENABLE_INTERRUPTS(r.8)          // enable interrupts

        blr                             // return

#if !defined(NT_UP)
        SPIN_ON_SPIN_LOCK_ENABLED(r.5, r.9, pushesl, pusheslw, pusheslx, r.8, r.12)
#endif

        DUMMY_EXIT(ExInterlockedPushEntrySList)

//      SBTTL("Interlocked Compare Exchange 64-bits")
//++
//
// ULONGLONG
// ExInterlockedCompareExchange64 (
//    IN PULONGLONG Destination,
//    IN PULONGLONG Exchange,
//    IN PULONGLONG Comparand,
//    IN PKSPIN_LOCK Lock
//    )
//
// Routine Description:
//
//    This function performs an interlocked compare and exchange of 64-bits.
//
// Arguments:
//
//    Destination (r3) - Supplies a pointer to the destination variable.
//
//    Exchange (r4) - Supplies a pointer to the exchange value.
//
//    Comparand (r5) - Supplies a pointer to the comparand value.
//
//    Lock (r6) - Supplies a pointer to a spin lock to use to synchronize
//        access to Destination.
//
// Return Value:
//
//    The current destination value is returned as the function value
//    (in r3 and r4).
//
//--

        LEAF_ENTRY(ExInterlockedCompareExchange64)

        lwz     r.0,0(r.5)              // get comparand (low)
        lwz     r.5,4(r.5)              // get comparand (high)

        DISABLE_INTERRUPTS(r.8,r.12)    // disable interrupts
                                        // r.8  <- previous msr value
                                        // r.12 <- new (disabled) msr

#if !defined(NT_UP)

        ACQUIRE_SPIN_LOCK(r.6, r.6, r.9, cmpex64, cmpex64w)
#endif

        lwz     r.10,0(r.3)             // get current value (low)
        lwz     r.11,4(r.3)             // get current value (high)
        cmpw    cr.0,r.0,r.10           // compare current with comparand (low)
        cmpw    cr.1,r.5,r.11           // compare current with comparand (high)
        bne     cr.0,cmpex64_no         // if ne, current and comparand mismatch (low)
        bne     cr.1,cmpex64_no         // if ne, current and comparand mismatch (high)

        lwz     r.0,0(r.4)              // get exchange value (low)
        lwz     r.5,4(r.4)              // get exchange value (high)
        stw     r.0,0(r.3)              // store exchange value (low)
        stw     r.5,4(r.3)              // store exchange value (high)

cmpex64_no:

#if !defined(NT_UP)
        RELEASE_SPIN_LOCK(r.6, r.9)
#endif

        ENABLE_INTERRUPTS(r.8)          // enable interrupts

        ori     r.3,r.10,0              // return current value (low)
        ori     r.4,r.11,0              // return current value (low)

        blr                             // return

#if !defined(NT_UP)
        SPIN_ON_SPIN_LOCK_ENABLED(r.6, r.9, cmpex64, cmpex64w, cmpex64x, r.8, r.12)
#endif

        DUMMY_EXIT(ExInterlockedCompareExchange64)

#if COLLECT_PAGING_DATA

        .struct 0
        .space  StackFrameHeaderLength
gpiLr:  .space  4
gpiR31: .space  4
gpiR30: .space  4
gpiR29: .space  4
gpiR28: .space  4
gpiR27: .space  4
gpiR26: .space  4
gpiR25: .space  4
gpiR24: .space  4
        .align  3
gpiFrameLength:

        SPECIAL_ENTRY(ExpGetPagingInformation)

        mflr    r0
        stwu    sp, -gpiFrameLength(sp)
        stw     r31, gpiR31(sp)
        stw     r30, gpiR30(sp)
        stw     r29, gpiR29(sp)
        stw     r28, gpiR28(sp)
        stw     r27, gpiR27(sp)
        stw     r26, gpiR26(sp)
        stw     r25, gpiR25(sp)
        stw     r24, gpiR24(sp)
        stw     r0, gpiLr(sp)

        PROLOGUE_END(ExpGetPagingInformation)

        ori     r30, r3, 0
        ori     r31, r4, 0

        lwz     r29, [toc]KeNumberProcessors(r2)
        lwz     r29, 0(r29)
        stw     r29, 0(r30)

        mfsdr1  r28
        addi    r27, r28, 1
        rlwinm  r27, r27, 16, 0x03ff0000
        stw     r27, 4(r30)
        rlwinm  r24, r27, 31, 0x7fffffff

        subi    r31, r31, 8
        addi    r30, r30, 8

        lwz     r26, [toc]KiProcessorBlock(r2)

gpi_procloop:

        lwz     r25, 0(r26)
        lwz     r25, PbPcrPage(r25)
        slwi    r25, r25, PAGE_SHIFT
        oris    r25, r25, 0x8000

        addi    r26, r26, 4
        subi    r29, r29, 1

        subic.  r31, r31, CTR_SIZE
        blt     skip_processor_data
        ori     r3, r30, 0
        la      r4, PcPagingData(r25)
        li      r5, CTR_SIZE
        bl      ..RtlCopyMemory

skip_processor_data:

        addi    r30, r30, CTR_SIZE

        cmpwi   r29, 0
        bne     gpi_procloop

        sub.    r31, r31, r27
        blt     skip_hpt
        ori     r3, r30, 0
        rlwinm  r28, r28, 0, 0xffff0000
        oris    r4, r28, 0x8000
        ori     r5, r27, 0
        bl      ..RtlCopyMemory

skip_hpt:

        ori     r3, r31, 0

        lwz     r0, gpiLr(sp)
        lwz     r31, gpiR31(sp)
        lwz     r30, gpiR30(sp)
        lwz     r29, gpiR29(sp)
        lwz     r28, gpiR28(sp)
        lwz     r27, gpiR27(sp)
        lwz     r26, gpiR26(sp)
        lwz     r25, gpiR25(sp)
        lwz     r24, gpiR24(sp)
        mtlr    r0
        addi    sp, sp, gpiFrameLength

        SPECIAL_EXIT(ExpGetPagingInformation)

#endif // COLLECT_PAGING_DATA
