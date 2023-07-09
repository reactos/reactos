//       TITLE("Interlocked Support")
//++
//
// Copyright (c) 1998  Intel Corporation
// Copyright (c) 1998  Microsoft Corporation
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
//    William K. Cheung (v-wcheung) 10-Mar-1998
//
// Environment:
//
//    User mode.
//
// Revision History:
//
//--

#include "ksia64.h"

        SBTTL("Interlocked Pop Entry Sequenced List")

//++
//
// PSINGLE_LIST_ENTRY
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

        ld8         t0 = [a0]                   // load next entry & sequence
        add         t6 = 0xFFFF, r0
        dep         t1 = 0, a0, 0, 61           // capture region bits
        ;;

Epop10:
        mov         ar.ccv = t0
        shr         t2 = t0, 63 - 42 + 3
        ;;
        shladd      v0 = t2, 3, t1              // merge region & va bits
        ;;

        cmp.eq      pt2, pt1 = t1, v0           // if eq, list is empty
        add         t4 = t6, t0                 // adjust depth & sequence
        ;;
 (pt2)  add         v0 = 0, zero
        

//
// N.B. It is possible for the following instruction to fault in the rare
//      case where the first entry in the list is allocated on another
//      processor and free between the time the free pointer is read above
//      and the following instruction.
//

 (pt1)  ld8         t5 = [v0]                   // get addr of successor entry
        extr.u      t4 = t4, 0, 24              // extract depth & sequence
        ;;
 (pt1)  shl         t5 = t5, 63 - 42            // shift va into position
        ;;

 (pt1)  ld8         t3 = [a0]                   // reload next entry & sequence
 (pt1)  or          t5 = t4, t5                 // merge va, depth & sequence
 (pt2)  br.ret.spnt.clr brp                     // return if the list is null
        ;;

 (pt1)  cmp.eq.unc  pt4, pt1 = t3, t0
        ;;
 (pt4)  cmpxchg8.rel.nt1 t3 = [a0], t5, ar.ccv  // perform the pop
        nop.i       0
        ;;

 (pt4)  cmp.eq.unc  pt3 = t3, t0                // if eq, cmpxchg8 succeeded
        mov         t0 = t3
 (pt3)  br.ret.sptk brp

        br          Epop10                      // try again

        LEAF_EXIT(RtlpInterlockedPopEntrySList)

//++
//
// PSINGLE_LIST_ENTRY
// RtlpInterlockedPushEntrySList (
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

        LEAF_ENTRY(RtlpInterlockedPushEntrySList)

        ld8         t0 = [a0]                   // load next entry & sequence
        dep         t1 = 0, a0, 0, 61           // capture region bits
        mov         t6 = 0x10001
        shl         t5 = a1, 63 - 42            // shift va into position
        ;;

Epush10:
        mov         ar.ccv = t0                 // set the comparand
        add         t4 = t0, t6
        shr         v0 = t0, 63 - 42 + 3        // extract next entry address
        ;;

        cmp.ne      pt3 = zero, v0              // if ne, list not empty
        ;;
 (pt3)  shladd      v0 = v0, 3, t1              // merge region & va bits
        extr.u      t4 = t4, 0, 24              // extract depth & sequence
        ;;

        st8         [a1] = v0
        or          t5 = t4, t5                 // merge va, depth & sequence
        ;;

        cmpxchg8.rel t3 = [a0], t5, ar.ccv
        ;;
        cmp.eq      pt2, pt1 = t0, t3
        mov         t0 = t3

 (pt2)  br.ret.sptk brp                         // if equal, return
 (pt1)  br.spnt     Epush10                     // retry
 
        LEAF_EXIT(RtlpInterlockedPushEntrySList)
