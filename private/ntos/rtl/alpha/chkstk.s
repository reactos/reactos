//      TITLE("Runtime Stack Checking")
//++
//
// Copyright (c) 1991  Microsoft Corporation
// Copyright (c) 1992  Digital Equipment Corporation
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
//    Any mode.
//
// Revision History:
//
//    Thomas Van Baak (tvb) 7-May-1992
//
//        Adapted for Alpha AXP.
//
//--

#include "ksalpha.h"

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
//    then the system will automatically attempts to expand the stack. If the
//    attempt succeeds, then another page is committed. Otherwise, a stack
//    overflow exception is raised. It is the responsibility of the caller to
//    handle this exception.
//
//    N.B. This routine is called using a non-standard calling sequence since
//       it is typically called from within the prologue. The allocation size
//       argument is in register t12 and it must be preserved. Register t11
//       may contain the callers saved ra and it must be preserved. The choice
//       of these registers is hard-coded into the acc C compiler. Register v0
//       may contain a static link pointer (exception handlers) and so it must
//       be preserved. Since this function is called from within the prolog,
//       the a' registers must be preserved, as well as all the s' registers.
//       Registers t8, t9, and t10 are used by this function and are not
//       preserved.
//
//       The typical calling sequence from the prologue is:
//
//           mov   ra, t11              // save return address
//           LDIP  t12, SIZE            // set requested stack frame size
//           bsr   ra, _RtlCheckStack   // check stack page allocation
//           subq  sp, t12, sp          // allocate stack frame
//           mov   t11, ra              // restore return address
//
// Arguments:
//
//    Allocation (t12) - Supplies the size of the allocation on the stack.
//
// Return Value:
//
//    None.
//
//--

        LEAF_ENTRY(_RtlCheckStack)

        subq    sp, t12, t8             // compute requested new stack address
        mov     v0, t10                 // save v0 since the PALcode uses it

#if defined(NTOS_KERNEL_RUNTIME)

//
// Running on kernel stack - get stack limit from thread object
//

        GET_CURRENT_THREAD              // current thread in v0

        LDP     t9, ThStackLimit(v0)    // get kernel stack limit

#else

//
// Running on user stack - get stack limit from thread environment block.
//

10:     GET_THREAD_ENVIRONMENT_BLOCK    // (PALcode) put TEB address in v0

        LDP     t9, TeStackLimit(v0)    // get user stack limit

#endif

//
// The requested bottom of the stack is in t8.
// The current low limit of the stack is in t9.
//
// If the new stack address is greater than the current stack limit, then the
// pages have already been allocated, and nothing further needs to be done.
//

        mov     t10, v0                 // restore v0, no further PAL calls
        cmpult  t8, t9, t10             // t8<t9? new stack base within limit?
        beq     t10, 40f                // if eq [false], then t8>=t9, so yes

        LDIP    t10, ~(PAGE_SIZE - 1)   // round down new stack address
        and     t8, t10, t8             //   to next page boundary

//
// Go down one page, touch one quadword in it, and repeat until we reach the
// new stack limit.
//

30:     lda     t9, -PAGE_SIZE(t9)      // compute next address to check
        stq     zero, 0(t9)             // probe stack address with a write
        cmpeq   t8, t9, t10             // t8=t9? at the low limit yet?
        beq     t10, 30b                // if eq [false], more pages to probe

40:     ret     zero, (ra)              // return

        .end    _RtlCheckStack
