//       TITLE("Interlocked Support")
//++
//
// Copyright (c) 1996  Microsoft Corporation
//
// Module Name:
//
//    slist.s
//
// Abstract:
//
//    This module implements functions to support interlocked S_List
//    operations.
//
// Author:
//
//    David N. Cutler (davec) 13-Mar-1996
//
// Environment:
//
//    Kernel mode.
//
// Revision History:
//
//--

#include "ksalpha.h"

        SBTTL("Interlocked Pop Entry Sequenced List")
//++
//
// PVOID
// RtlpInterlockedPopEntrySList (
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

        LEAF_ENTRY(RtlpInterlockedPopEntrySList)

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

        mb                              // synchornize memory access
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
        beq     t4, 15f                 // if eq, listhead has changed
        bis     t1, t2, t1              // merge address, depth, and sequence
        stq_c   t1, 0(a0)               // store next entry address and sequence
        beq     t1, 15f                 // if eq, store conditional failed
30:     mb                              // synchronize memory access
        ret     zero, (ra)              // return

//
// Conditional store attempt failed or listhead changed.
//

15:     br      zero, 10b               // retry

        .end    RtlpInterlockedPopEntrySList

        SBTTL("Interlocked Push Entry Sequenced List")
//++
//
// PVOID
// RtlpInterlockedPushEntrySList (
//    IN PSLIST_HEADER ListHead,
//    IN PVOID ListEntry
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

        LEAF_ENTRY(RtlpInterlockedPushEntrySList)

10:     ldq     t0, 0(a0)               // get next entry address and sequence

#if defined(_AXP64_)

        sra     t0, 63 - 42, v0         // extract next entry address
        bic     v0, 7, v0               //
        bis     t0, zero, t1            // copy depth and sequence

#else

        addl    t0, zero, v0            // sign extend next entry address
        srl     t0, 32, t1              // shift sequence to low 32-bits

#endif

        STP     v0, 0(a1)               // set next link in new first entry
        mb                              // synchronize memory access

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

        cmpeq   t0, t3, t4              // check if listhead has changed
        beq     t4, 15f                 // if eq, listhead changed
        bis     t1, t2, t2              // merge address, depth, and sequence
        stq_c   t2, 0(a0)               // store next entry address and sequence
        beq     t2, 15f                 // if eq, store conditional failed
        ret     zero, (ra)              // return

//
// Conditional store attempt failed or listhead changed.
//

15:     br      zero, 10b               // retry

        .end    RtlpInterlockedPushEntrySList
