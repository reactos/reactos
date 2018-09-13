//      TITLE("Runtime Stack Checking")
//++
//
// Copyright (c) 1991  Microsoft Corporation
//
// Module Name:
//
//    chkstk.s
//
// Abstract:
//
//    This module implements runtime stack checking.
//
// Author:
//
//    David N. Cutler (davec) 14-Mar-1991
//
// Environment:
//
//    User mode.
//
// Revision History:
//
//--

#include "ksmips.h"

        SBTTL("Check Stack")
//++
//
// ULONG
// _RtlCheckStack (
//    IN ULONG Allocation
//    )
//
// Routine Description:
//
//    This function provides runtime stack checking for local allocations
//    that are more than a page and for storage dynamically allocated with
//    the alloca function. Stack checking consists of probing downward in
//    the stack a page at a time. If the current stack commitment is exceeded,
//    then the system will automatically attempt to expand the stack. If the
//    attempt succeeds, then another page is committed. Otherwise, a stack
//    overflow exception is raised. It is the responsiblity of the caller to
//    handle this exception.
//
//    N.B. This routine is called using a calling sequence that assumes that
//       all registers are preserved.
//
// Arguments:
//
//    Allocation (t8) - Supplies the size of the allocation on the stack.
//
// Return Value:
//
//    None.
//
//--

        NESTED_ENTRY(_RtlCheckStack, 0, ra)

        sw      t7,0(sp)                // save temporary register
        sw      t8,4(sp)                // save allocation size
        sw      t9,8(sp)                // save temporary register

        PROLOGUE_END

        .set    noreorder
        .set    noat
        li      t9,UsPcr                // get address of user PCR
        lw      t9,PcTeb(t9)            // get address of environment block
        subu    t8,sp,t8                // compute new bottom of stack
        lw      t9,TeStackLimit(t9)     // get low stack address
        sltu    t7,t8,t9                // new stack address within limits?
        beq     zero,t7,20f             // if eq, stack within limits
        li      t7,~(PAGE_SIZE - 1)     // set address mask
        and     t8,t8,t7                // round down new stack address
10:     subu    t9,t9,PAGE_SIZE         // compute next address to check
        bne     t8,t9,10b               // if ne, more pages to probe
        sw      zero,0(t9)              // check stack address
20:     lw      t7,0(sp)                // restore temporary register
        lw      t8,4(sp)                // restore allocation size
        j       ra                      // return
        lw      t9,8(sp)                // restore temporary register
        .set    at
        .set    reorder

        .end    _RtlCheckStack
