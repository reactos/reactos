//      TITLE("Compute Checksum")
//++
//
// Copyright (c) 1992  Microsoft Corporation
// Copyright (c) 1993  Digital Equipment Corporation
//
// Module Name:
//
//    chksum.s
//
// Abstract:
//
//    This module implements a function to compute the checksum of a buffer.
//
//    N.B. This code is also used in ntos\streams\tcpip\common\alpha\cksy.s.
//
// Author:
//
//    David N. Cutler (davec) 27-Jan-1992
//
// Environment:
//
//    User mode.
//
// Revision History:
//
//    Thomas Van Baak (tvb) 25-May-1993
//
//        Adapted for Alpha AXP
//
//--

#include "ksalpha.h"

//
// Define macro to add two quadwords with wrap-around carry. This is used
// to checksum eight 16-bit words in parallel.
//

#define ADDQC(Rx, Ry, Rz) \
        addq    Rx, Ry, Rx ;\
        cmpult  Rx, Ry, Ry ;\
        addq    Rx, Ry, Rz

        SBTTL("Compute Checksum")
//++
//
// USHORT
// ChkSum (
//    IN ULONG Checksum,
//    IN PUSHORT Source,
//    IN ULONG Length
//    )
//
// Routine Description:
//
//    This function computes the checksum of the specified buffer.
//
// Arguments:
//
//    Checksum (a0) - Supplies the initial checksum value.
//
//    Source (a1) - Supplies a pointer to the buffer to be checksummed.
//
//    Length (a2) - Supplies the length of the buffer in words.
//
// Return Value:
//
//    The computed checksum is returned as the function value.
//
//--

        LEAF_ENTRY(ChkSum)

        addq    a2, a2, a2              // convert length from words to bytes
        addq    a1, a2, a3              // compute ending buffer address

//
// Sum words before first quadword. A word aligned address is assumed.
//

10:     and     a1, 3*2, v0             // check word alignment
        beq     v0, 20f                 // if eq, no more leading words
        beq     a2, 80f                 // if byte count now zero, done

        ldq_u   t0, 0(a1)               // get quadword containing word
        extwl   t0, a1, t0              // extract word zero extended
        addq    a0, t0, a0              // add to sum
        addq    a1, 2, a1               // increment buffer address
        subq    a2, 2, a2               // decrement byte count
        br      10b                     // check for more words

//
// Sum quadwords before first 64-byte block. The address is now quadword
// aligned.
//

20:     and     a1, 7*8, v0             // check quadword alignment
        beq     v0, 30f                 // if eq, no more leading quadwords
        cmplt   a2, 8, t12              // less than one quadword left?
        bne     t12, 60f                // if ne[true], sum last words

        ldq     t0, 0(a1)               // get quadword
        ADDQC   (a0, t0, a0)            // add to sum
        addq    a1, 8, a1               // increment buffer address
        subq    a2, 8, a2               // decrement byte count
        br      20b                     // check for more quadwords

//
// Sum 64-byte size blocks. The address is now 64-byte aligned.
//
// N.B. The following code is written to be readable; the assembler will
//      schedule it optimally.
//

30:     bic     a3, 63, a4              // compute ending block address
        cmpeq   a1, a4, t12             // at end of last block?
        bne     t12, 50f                // if ne[true], no full blocks

40:     ldq     t0, 0*8(a1)             // load eight quadwords (64-bytes)
        ldq     t1, 1*8(a1)
        ldq     t2, 2*8(a1)
        ldq     t3, 3*8(a1)
        ldq     t4, 4*8(a1)
        ldq     t5, 5*8(a1)
        ldq     t6, 6*8(a1)
        ldq     t7, 7*8(a1)

        ADDQC   (t0, t1, t0)            // add 1st and 2nd quadwords into t0
        ADDQC   (t2, t3, t2)            // add 3rd and 4th quadwords into t2
        ADDQC   (t0, t2, t0)            // add first four quadwords into t0

        ADDQC   (t4, t5, t4)            // add 5th and 6th quadwords into t4
        ADDQC   (t6, t7, t6)            // add 7th and 8th quadwords into t6
        ADDQC   (t4, t6, t4)            // add second four quadwords into t4

        ADDQC   (t0, t4, t0)            // add all eight quadwords into t0
        ADDQC   (a0, t0, a0)            // add to sum in a0

        addq    a1, 64, a1              // increment buffer address
        cmpeq   a1, a4, t12             // at end of last block?
        beq     t12, 40b                // if eq[false], sum next block
        subq    a3, a1, a2              // recompute remaining byte count

//
// Sum quadwords following last 64-byte block.
//

50:     and     a2, 7*8, v0             // check for residual quadwords
        beq     v0, 60f                 // if eq, no more quadwords

        ldq     t0, 0(a1)               // get quadword
        ADDQC   (a0, t0, a0)            // add to sum
        addq    a1, 8, a1               // increment buffer address
        subq    a2, 8, a2               // decrement byte count
        br      50b                     // check for more quadwords

//
// Sum words following last quadword.
//

60:     and     a2, 3*2, v0             // check for residual words
        beq     v0, 70f                 // if eq, no more words

        ldq_u   t0, 0(a1)               // get quadword containing word
        extwl   t0, a1, t0              // extract word zero extended
        ADDQC   (a0, t0, a0)            // add to sum
        addq    a1, 2, a1               // increment buffer address
        subq    a2, 2, a2               // decrement byte count
        br      60b                     // check for more words

//
// Fold final sum in a0 from its four-word parallel quadword format into
// a single 16-bit word in v0.
//

70:     extwl   a0, 0, t0               // get word 0
        extwl   a0, 2, t1               // get word 1
        extwl   a0, 4, t2               // get word 2
        extwl   a0, 6, t3               // get word 3
        addq    t0, t1, t0              // add words 0 and 1
        addq    t2, t3, t2              // add words 2 and 3
        addq    t0, t2, a0              // four word sum, possible 2 bit carry

80:     srl     a0, 16, t0              // get carry bits
        zapnot  a0, 0x3, a0             // isolate sum bits
        addq    a0, t0, a0              // add words, possible one bit carry

        srl     a0, 16, t0              // get carry bit
        zapnot  a0, 0x3, a0             // isolate sum bits
        addq    a0, t0, v0              // add words, no carry possible

        ret     zero, (ra)              // return

        .end    ChkSum
