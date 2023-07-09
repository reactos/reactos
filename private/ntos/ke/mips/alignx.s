//      TITLE("Unaligned Branch Tests")
//++
//
// Copyright (c) 1991  Microsoft Corporation
//
// Module Name:
//
//    alignx.s
//
// Abstract:
//
//    This module implements the unaligned branch tests.
//
// Author:
//
//    David N. Cutler (davec) 27-Feb-1991
//
// Environment:
//
//    User mode only.
//
// Revision History:
//
//--

#include "ksmips.h"

        SBTTL("Unaligned BEQ/BNE/BC1F/BC1T Branch Tests")
//++
//
// Routine Description:
//
//    The following routines implement beq/bne/bc1f/bc1t tests with an unaligned
//    load word instruction in the delay slot.
//
// Arguments:
//
//    a0 - Supplies first operand for branch test.
//    a1 - Supplies second operate for branch test.
//    a2 - Supplies a pointer to an unaligned word.
//    a3 - Supplies a pointer to an aligned word that receives the result
//       of the unaligned load.
//
// Return Value:
//
//    A value of true is returned in the brancd was taken. Otherwise,
//    FALSE is returned.
//
//--

        LEAF_ENTRY(Beq)

        .set    noreorder
        li      v0,1                    // set branched true
        beq     a0,a1,10f               // if eq, branch
        lw      v1,0(a2)                // load unaligned data
        move    v0,zero                 // set branched false
10:     j       ra                      // return
        sw      v1,0(a3)                // store unaligned value
        .set    reorder

        .end    Beq

        LEAF_ENTRY(Bne)

        .set    noreorder
        li      v0,1                    // set branched true
        bne     a0,a1,10f               // if eq, branch
        lw      v1,0(a2)                // load unaligned data
        move    v0,zero                 // set branched false
10:     j       ra                      // return
        sw      v1,0(a3)                // store unaligned value
        .set    reorder

        .end    Bne

        LEAF_ENTRY(Bc1f)

        .set    noreorder
        mtc1    a0,f0                   // set comparand 1
        mtc1    a1,f2                   // set comparand 2
        li      v0,1                    // set branched true
        c.eq.s  f0,f2                   // compare for equality
        bc1f    10f                     // if f, branch
        lw      v1,0(a2)                // load unaligned data
        move    v0,zero                 // set branched false
10:     j       ra                      // return
        sw      v1,0(a3)                // store unaligned value
        .set    reorder

        .end    Bc1f

        LEAF_ENTRY(Bc1t)

        .set    noreorder
        mtc1    a0,f0                   // set comparand 1
        mtc1    a1,f2                   // set comparand 2
        li      v0,1                    // set branched true
        c.eq.s  f0,f2                   // compare for equality
        bc1t    10f                     // if t, branch
        lw      v1,0(a2)                // load unaligned data
        move    v0,zero                 // set branched false
10:     j       ra                      // return
        sw      v1,0(a3)                // store unaligned value
        .set    reorder

        .end    Bc1t

        SBTTL("Unaligned BLEZ/BLTZ/BGEZ/BGTZ/BGEZAL/BLTZAL Branch Tests")
//++
//
// Routine Description:
//
//    The following routines implement blez/bltz/bgez/bgtz/bgezal/bltzal
//    tests with an unaligned load word instruction in the delay slot.
//
// Arguments:
//
//    a0 - Supplies the operand for branch test.
//    a1 - Supplies a pointer to an unaligned word.
//    a2 - Supplies a pointer to an aligned word that receives the result
//       of the unaligned load.
//
// Return Value:
//
//    A value of true is returned in the branch was taken. Otherwise,
//    FALSE is returned.
//
//--

        LEAF_ENTRY(Blez)

        .set    noreorder
        li      v0,1                    // set branched true
        blez    a0,10f                  // if lez, branch
        lw      v1,0(a1)                // load unaligned data
        move    v0,zero                 // set branched false
10:     j       ra                      // return
        sw      v1,0(a2)                // store unaligned value
        .set    reorder

        .end    Blez

        LEAF_ENTRY(Bltz)

        .set    noreorder
        li      v0,1                    // set branched true
        bltz    a0,10f                  // if ltz, branch
        lw      v1,0(a1)                // load unaligned data
        move    v0,zero                 // set branched false
10:     j       ra                      // return
        sw      v1,0(a2)                // store unaligned value
        .set    reorder

        .end    Bltz

        LEAF_ENTRY(Bgez)

        .set    noreorder
        li      v0,1                    // set branched true
        bgez    a0,10f                  // if gez, branch
        lw      v1,0(a1)                // load unaligned data
        move    v0,zero                 // set branched false
10:     j       ra                      // return
        sw      v1,0(a2)                // store unaligned value
        .set    reorder

        .end    Bgez

        LEAF_ENTRY(Bgtz)

        .set    noreorder
        li      v0,1                    // set branched true
        bgtz    a0,10f                  // if gtz, branch
        lw      v1,0(a1)                // load unaligned data
        move    v0,zero                 // set branched false
10:     j       ra                      // return
        sw      v1,0(a2)                // store unaligned value
        .set    reorder

        .end    Bgtz

        LEAF_ENTRY(Bgezal)

        .set    noreorder
        sw      ra,4 * 4(sp)            // save return address
        move    v0,zero                 // set branched false
        bgezal  a0,10f                  // if gez, branch and link
        lw      v1,0(a1)                // load unaligned data
        lw      ra,4 * 4(sp)            // restore return address
        sw      v1,0(a2)                // store unaligned value
        j       ra                      // return
        nop                             //

10:     j       ra                      // return
        li      v0,1                    // set branched true
        .set    reorder

        .end    Bgezal

        LEAF_ENTRY(Bltzal)

        .set    noreorder
        sw      ra,4 * 4(sp)            // save return address
        move    v0,zero                 // set branched false
        bltzal  a0,10f                  // if ltz, branch and link
        lw      v1,0(a1)                // load unaligned data
        lw      ra,4 * 4(sp)            // restore return address
        sw      v1,0(a2)                // store unaligned value
        j       ra                      // return
        nop                             //

10:     j       ra                      // return
        li      v0,1                    // set branched true
        .set    reorder

        .end    Bltzal

        SBTTL("Unaligned JAL/J Tests")
//++
//
// Routine Description:
//
//    The following routines implement jal/j tests with an unaligned
//    load word instruction in the delay slot.
//
// Arguments:
//
//    a0 - Supplies a pointer to an unaligned word.
//    a1 - Supplies a pointer to an aligned word that receives the result
//       of the unaligned load.
//
// Return Value:
//
//    A value of true is returned in the brancd was taken. Otherwise,
//    FALSE is returned.
//
//--

        LEAF_ENTRY(Jal)

        .set    noreorder
        sw      ra,4 * 4(sp)            // save return address
        move    v0,zero                 // set branched false
        jal     10f                     // jump and link
        lw      v1,0(a0)                // load unaligned data
        lw      ra,4 * 4(sp)            // restore return address
        sw      v1,0(a1)                // store unaligned value
        j       ra                      // return
        nop                             //

10:     j       ra                      // return
        li      v0,1                    // set branched true
        .set    reorder

        .end    Jal

        LEAF_ENTRY(Jalr)

        .set    noreorder
        sw      ra,4 * 4(sp)            // save return address
        move    v0,zero                 // set branched false
        la      t0,10f                  // get destination address
        jal     t0                      // jump
        lw      v1,0(a0)                // load unaligned data
        lw      ra,4 * 4(sp)            // restore return address
        sw      v1,0(a1)                // store unaligned value
        j       ra                      // return
        nop                             //

10:     j       ra                      // jump back
        li      v0,1                    // set branched true
        .set    reorder

        .end    Jalr

        LEAF_ENTRY(J)

        .set    noreorder
        sw      ra,4 * 4(sp)            // save return address
        move    v0,zero                 // set branched false
        j       10f                     // jump
        lw      v1,0(a0)                // load unaligned data
20:     lw      ra,4 * 4(sp)            // restore return address
        sw      v1,0(a1)                // store unaligned value
        j       ra                      // return
        nop                             //

10:     j       20b                     // jump back
        li      v0,1                    // set branched true
        .set    reorder

        .end    J

        LEAF_ENTRY(Jr)

        .set    noreorder
        sw      ra,4 * 4(sp)            // save return address
        move    v0,zero                 // set branched false
        la      t0,10f                  // get destination address
        j       t0                      // jump
        lw      v1,0(a0)                // load unaligned data
20:     lw      ra,4 * 4(sp)            // restore return address
        sw      v1,0(a1)                // store unaligned value
        j       ra                      // return
        nop                             //

10:     j       20b                     // return
        li      v0,1                    // set branched true
        .set    reorder

        .end    Jr
