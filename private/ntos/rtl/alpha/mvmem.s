//      TITLE("Compare, Move, Zero, and Fill Memory Support")
//++
//
// Copyright (c) 1992  Digital Equipment Corporation
//
// Module Name:
//
//    mvmem.s
//
// Abstract:
//
//    This module implements functions to compare, move, zero, and fill
//    blocks of memory. If the memory is aligned, then these functions
//    are very efficient.
//
//    N.B. These routines MUST preserve all floating state since they are
//        frequently called from interrupt service routines that normally
//        do not save or restore floating state.
//
// Author:
//
//    Joe Notarangelo  21-May-1992
//
// Environment:
//
//    User or Kernel mode.
//
// Revision History:
//
//    Monty VanderBilt 14-Feb-1996 Avoid memory loads and branch takens between
//                                 load lock and store conditional instructions
//                                 to conform with all alpha architecture rules.
//    Monty VanderBilt 27-Feb-1996 Added RtlZeroBytes and RtlFillBytes to support
//                                 byte granularity access when necessary.
//--

#include "ksalpha.h"

        SBTTL("Compare Memory")
//++
//
// ULONG
// RtlCompareMemory (
//    IN const PVOID Source1,
//    IN const PVOID Source2,
//    IN SIZE_T Length
//    )
//
// Routine Description:
//
//    This function compares two blocks of memory and returns the number
//    of bytes that compared equal.
//
// Arguments:
//
//    Source1 (a0) - Supplies a pointer to the first block of memory to
//       compare.
//
//    Source2 (a1) - Supplies a pointer to the second block of memory to
//       compare.
//
//    Length (a2) - Supplies the length, in bytes, of the memory to be
//       compared.
//
// Return Value:
//
//    The number of bytes that compared equal is returned as the function
//    value. If all bytes compared equal, then the length of the orginal
//    block of memory is returned.
//
//--


        LEAF_ENTRY(RtlCompareMemory)

        bis     a2, zero, v0            // save length of comparison
        beq     a2, 90f                 // (JAE) quit if nothing to compare
        xor     a0, a1, t0              // check for compatible alignment
        and     t0, 0x7, t0             // low bits only
        bne     t0, CompareUnaligned    // if ne, incompatible alignment

//
// Compare memory aligned
//

CompareAligned:                         //

//
// compare memory until sources are aligned
//
        and     a0, 0x7, t0             // get low bits
        bne     t0, 10f                 // if ne, sources not aligned yet
        br      zero, 30f               // already aligned, predicted


10:
        ldq_u   t1, 0(a0)               // get unaligned quad at source 1
        ldq_u   t2, 0(a1)               // get unaligned quad at source 2

20:
        extbl   t1, t0, t4              // byte at t0 in source 1 quad
        extbl   t2, t0, t5              // byte at t0 in source 2 quad
        xor     t4, t5, t3              // t1 = t2 ?
        bne     t3, 110f                // not equal, miscompare
        subq    a2, 1, a2               // decrement bytes to compare
        beq     a2, 90f                 // if eq, compare success
        addq    t0, 1, t0               // increment pointer within quad
        cmpeq   t0, 8, t3               // t0 = 8?, if so first quadword done
        beq     t3, 20b                 // continue while t0 < 8


        addq    a0, 8, a0               // increment to next quadword
        addq    a1, 8, a1               // increment source 2 to next also
        bic     a0, 7, a0               // align source 1 quadword
        bic     a1, 7, a1               // align source 2 quadword


//
// aligned block compare, compare blocks of 64 bytes
//

30:
        srl     a2, 6, t0               // t0 = number of 64 byte blocks
        beq     t0, 50f                 // if eq, no 64 byte blocks

//
// N.B. loads from each of the sources were separated in case these
//      blocks are fighting for the cache
//
        .set    noat
40:
        ldq     t1, 0(a0)               // t1 = source 1, quad 0
        ldq     t2, 8(a0)               // t2 = source 1, quad 1
        ldq     t3, 16(a0)              // t3 = source 1, quad 2
        addq    a1, 64, a1              // increment source 2 pointer
        ldq     t4, 24(a0)              // t4 = source 1, quad 3

        ldq     t5, -64(a1)             // t5 = source 2, quad 0
        ldq     a4, -56(a1)             // a4 = source 2, quad 1
        ldq     a5, -48(a1)             // a5 = source 2, quad 2
        xor     t1, t5, $at             // quad 0 match?
        bne     $at, 200f               // if ne[false], miscompare
        ldq     t5, -40(a1)             // t5 = source 2, quad 3
        ldq     t1, 32(a0)              // t1 = source 1, quad 4
        xor     t2, a4, $at             // quad 1 match?
        bne     $at, 122f               // if ne[false], miscompare
        ldq     t2, 40(a0)              // t2 = source 1, quad 5
        xor     t3, a5, $at             // quad 2 match?
        bne     $at, 124f               // if ne[false], miscompare
        ldq     t3, 48(a0)              // t3 = source 1, quad 6
        xor     t4, t5, $at             // quad 3 match?
        bne     $at, 126f               // if ne[false], miscompare
        ldq     t4, 56(a0)              // t4 = source 1, quad 7

        ldq     t5, -32(a1)             // t5 = source 2, quad 4
        addq    a0, 64, a0              // increment source 1 pointer
        ldq     a4, -24(a1)             // a4 = source 2, quad 5
        subq    t0, 1, t0               // decrement blocks to compare
        ldq     a5, -16(a1)             // a5 = source 2, quad 6
        xor     t1, t5, $at             // quad 4 match?
        bne     $at, 130f               // if ne[false], miscompare
        ldq     t5, -8(a1)              // t5 = source 2, quad 7
        xor     t2, a4, $at             // quad 5 match?
        bne     $at, 132f               // if ne[false], miscompare
        xor     t3, a5, $at             // quad 6 match?
        bne     $at, 134f               // if ne[false], miscompare
        xor     t4, t5, $at             // quad 7 match?
        bne     $at, 136f               // if ne[false], miscompare
        subq    a2, 64, a2              // decrement bytes to compare
        bne     t0, 40b                 // if ne, more blocks to compare
        .set    at


//
// Compare quadwords
//

50:
        srl     a2, 3, t0               // t0 = number of quadwords to compare
        beq     t0, 70f                 // if eq, no quadwords to compare

        .set    noat
60:
        ldq     t1, 0(a0)               // t1 = quad from source 1
        lda     a0, 8(a0)               // increment source 1 pointer
        ldq     t2, 0(a1)               // t2 = quad from source 2
        lda     a1, 8(a1)               // increment source 2 pointer
        xor     t1, t2, $at             // are quadwords equal?
        bne     $at, 200f               // if ne, miscompare
        subq    t0, 1, t0               // decrement quads to compare
        subq    a2, 8, a2               // decrement bytes to compare
        bne     t0, 60b                 // if ne, more quads to compare

        .set    at

//
// Compare bytes in last quadword
//

//      a2 =  number of bytes to compare, less than 8, greater than zero
//      a0, a1, quad-aligned to last quadword


        .set    noat
70:
        beq     a2, 80f                 // if eq, all bytes compared
        ldq     t1, 0(a0)               // t1 = quad at source 1
        ldq     t2, 0(a1)               // t2 = quad at source 2
        bis     zero, 0xff, t0          // zap mask
        sll     t0, a2, t0              //
        zap     t1, t0, t1              // zero bytes not compared
        zap     t2, t0, t2              // same for source 2
        xor     t1, t2, $at             // compare quadwords
        bne     $at, 200f               // if ne, miscompare

        .set    at
//
// Successful compare
//      v0 already contains full length
//

80:
        ret     zero, (ra)              // return


//
// Sources have incompatible alignment
//
CompareUnaligned:


//
// Compare until source 1 (a0) is aligned
//

        and     a0, 0x7, t0             // get byte position of pointer
        beq     t0, 30f                 // if eq, already aligned

        ldq_u   t1, 0(a0)               // get unaligned quad at a0

10:
        ldq_u   t2, 0(a1)               // get unaligned quad at a1
        extbl   t1, t0, t4              // get byte to compare from source 1
        extbl   t2, a1, t2              // get byte to compare from source 2
        xor     t4, t2, t3              // do bytes match?
        bne     t3, 110f                // if ne, miscompare
        subq    a2, 1, a2               // decrement bytes to compare
        beq     a2, 90f                 // (JAE) quit if nothing left to compare
        addq    t0, 1, t0               // increment byte within source 1
        addq    a1, 1, a1               // increment source 2 pointer
        cmpeq   t0, 8, t3               // finished with source 1 quad?
        beq     t3, 10b                 // if eq[false], more to compare

        addq    a0, 7, a0               // point to next source 1 quad
        bic     a0, 7, a0               // align to quadword


//
// Compare 64-byte blocks
//

30:
        srl     a2, 6, t0               // t0 = number of blocks to compare
        beq     t0, 50f                 // if eq, no blocks to move

        ldq_u   t1, 0(a1)               // get source 2 unaligned quad 1

        .set    noat
40:
        ldq_u   t2, 7(a1)               // get source 2 unaligned quad 2
        addq    a0, 64, a0              // increment source 1 pointer
        ldq_u   t3, 15(a1)              // get source 2 unaligned quad 3
        extql   t1, a1, t1              // bytes from unaligned quad 1
        extqh   t2, a1, $at             // bytes from unaligned quad 2
        ldq_u   t4, 23(a1)              // get source 2 unaligned quad 4
        bis     t1, $at, t1             // t1 = quadword 1 (source 2)
        ldq_u   t5, 31(a1)              // get source 2 unaligned quad 5
        extql   t2, a1, t2              // bytes from unaligned quad 2
        extqh   t3, a1, $at             // bytes from unaligned quad 3
        ldq     a3, -64(a0)             // a3 = quadword 1 (source 1)
        bis     t2, $at, t2             // t2 = quadword 2 (source 2)
        ldq     a4, -56(a0)             // a4 = quadword 2 (source 1)
        extql   t3, a1, t3              // bytes from unaligned quad 3
        extqh   t4, a1, $at             // bytes from unaligned quad 4
        ldq     a5, -48(a0)             // a5 = quadword 3 (source 1)
        bis     t3, $at, t3             // t3 = quadword 3 (source 2)
        extql   t4, a1, t4              // bytes from unaligned quad 4
        extqh   t5, a1, $at             // bytes from unaligned quad 5
        subq    t0, 1, t0               // decrement blocks to compare
        bis     t4, $at, t4             // t4 = quadword 4 (source 2)

        xor     t1, a3, $at             // match on quadword 1?
        ldq     a3, -40(a0)             // a3 = quadword 4 (source 1)
        bne     $at, 200f               // if ne, miscompare quad 1
        xor     t2, a4, $at             // match on quadword 2?
        ldq_u   t2, 39(a1)              // get source 2 unaligned quad 6
        bne     $at, 122f               // if ne, miscompare quad 2
        xor     t3, a5, $at             // match on quadword 3?
        ldq_u   t3, 47(a1)              // get source 2 unaligned quad 7
        bne     $at, 124f               // if ne, miscompare quad 3
        xor     t4, a3, $at             // match on quadword 4?
        ldq_u   t4, 55(a1)              // get source 2 unaligned quad 8
        bne     $at, 126f               // if ne, miscompare quad 4
        ldq_u   t1, 63(a1)              // get source 2 unaligned quad 9

        ldq     a3, -32(a0)             // a3 = quadword 5 (source 1)
        extql   t5, a1, t5              // bytes from unaligned quad 5
        extqh   t2, a1, $at             // bytes from unaligned quad 6
        ldq     a4, -24(a0)             // a4 = quadword 6 (source 1)
        ldq     a5, -16(a0)             // a5 = quadword 7 (source 1)
        bis     t5, $at, t5             // t5 = quadword 5 (source 2)

        xor     t5, a3, $at             // match on quadword 5?
        ldq     a3, -8(a0)              // a3 = quadword 8 (source 1)
        bne     $at, 130f               // if ne, miscompare quad 5
        extql   t2, a1, t2              // bytes from unaligned quad 6
        extqh   t3, a1, $at             // bytes from unaligned quad 7
        extql   t3, a1, t3              // bytes from unaligned quad 7
        bis     t2, $at, t2             // t2 = quadword 6 (source 2)
        xor     t2, a4, $at             // match on quadword 6?
        bne     $at, 132f               // if ne, miscompare quad 6
        extqh   t4, a1, $at             // bytes from unaligned quad 8
        extql   t4, a1, t4              // bytes from unaligned quad 8
        bis     t3, $at, t3             // t3 = quadword 7 (source 2)
        xor     t3, a5, $at             // match on quadword 7?
        bne     $at, 134f               // if ne, miscompare quad 7
        extqh   t1, a1, $at             // bytes from unaligned quad 9
        addq    a1, 64, a1              // increment source 2 pointer
        bis     t4, $at, t4             // t4 = quadword 8 (source 2)
        xor     t4, a3, $at             // match on quadword 8?
        bne     $at, 136f               // if ne, miscompare quad 8
        subq    a2, 64, a2              // decrement number of bytes to compare
        bne     t0, 40b                 // if ne, more blocks to compare

        .set    at

//
// Compare quadwords
//


50:
        srl     a2, 3, t0               // t0 = number of quads to compare
        beq     t0, 70f                 // if eq, no quads to compare
        ldq_u   t1, 0(a1)               // get unaligned quad 1 (source 2)

        .set    noat
60:
        ldq_u   t2, 7(a1)               // get unaligned quad 2 (source 2)
        ldq     t3, 0(a0)               // t3 = quadword 1 (source 1)
        extql   t1, a1, t1              // get bytes from unaligned quad 1
        extqh   t2, a1, $at             // get bytes from unaligned quad 2
        addq    a1, 8, a1               // increment source 2 pointer
        bis     t1, $at, t1             // t1 = quadword 1 (source 2)
        xor     t1, t3, $at             // match on quadword?
        bne     $at, 200f               // if ne, miscompare
        subq    t0, 1, t0               // decrement quadwords to compare
        addq    a0, 8, a0               // increment source 1 pointer
        subq    a2, 8, a2               // decrement bytes to compare
        bis     t2, zero, t1            // save low quadword for next loop
        bne     t0, 60b                 // if ne, more quads to compare

        .set    at

//
// Compare bytes for final quadword
//

70:
        beq     a2, 90f                 // if eq, comparison complete

        ldq     t1, 0(a0)               // get quadword from source 1
        bis     zero, zero, t0          // t0 = byte position to compare

        .set    noat
80:
        ldq_u   t2, 0(a1)               // get unaligned quad from source 2
        extbl   t1, t0, t3              // t3 = byte from source 1
        extbl   t2, a1, t2              // t2 = byte from source 2
        xor     t3, t2, $at             // match on byte?
        bne     $at, 100f               // if ne, miscompare on byte
        addq    t0, 1, t0               // increment byte position
        addq    a1, 1, a1               // increment source 2 pointer
        subq    a2, 1, a2               // decrement bytes to compare
        bne     a2, 80b                 // if ne, more bytes to compare

        .set    at
//
// Successful full comparison
//

90:
        ret     zero, (ra)              // return, v0 already set


//
// Miscompare on last quadword
//

100:
        subq    v0, a2, v0              // subtract bytes not compared
        ret     zero, (ra)              // return

//
// Miscompare on first quadword, unaligned case
//
//  v0 = total bytes to compare
//  a2 = bytes remaining to compare
//

110:
        subq    v0, a2, v0              // bytes compared successfully
        ret     zero, (ra)              // return

//
// Miscompare on 64-byte block compare
//

122:
        subq    a2, 8, a2               // miscompare on quad 2
        br      zero, 200f              // finish in common code

124:
        subq    a2, 16, a2              // miscompare on quad 3
        br      zero, 200f              // finish in common code

126:
        subq    a2, 24, a2              // miscompare on quad 4
        br      zero, 200f              // finish in common code

130:
        subq    a2, 32, a2              // miscompare on quad 5
        br      zero, 200f              // finish in common code

132:
        subq    a2, 40, a2              // miscompare on quad 6
        br      zero, 200f              // finish in common code

134:
        subq    a2, 48, a2              // miscompare on quad 7
        br      zero, 200f              // finish in common code

136:
        subq    a2, 56, a2              // miscompare on quad 8
        br      zero, 200f              // finish in common code

//
// Miscompare, determine number of bytes that successfully compared
//      $at = xor of relevant quads from sources, must be non-zero
//      a2 = number of bytes left to compare
//
        .set    noat
200:
        cmpbge  zero, $at, $at          // $at = mask of non-zero bytes

        //
        // look for the first bit cleared in $at, this is the
        //      number of the first byte which differed
        //
        bis     zero, zero, t0          // bit position to look for clear

210:
        blbc    $at, 220f               // if low clear, found difference
        srl     $at, 1, $at             // check next bit
        addq    t0, 1, t0               // count bit position checked
        br      zero, 210b

220:
        subq    v0, a2, v0              // subtract bytes yet to compare
        addq    v0, t0, v0              // add bytes that matched on last quad

        ret     zero, (ra)

        .set    at

        .end    RtlCompareMemory


        SBTTL("Move Memory")
//++
//
// VOID
// RtlCopyMemory (
//    IN VOID UNALIGNED *Destination,
//    IN CONST VOID UNALIGNED *Source,
//    IN SIZE_T Length
//    )
//
// VOID
// RtlMoveMemory (
//    IN VOID UNALIGNED *Destination,
//    IN CONST VOID UNALIGNED *Source,
//    IN SIZE_T Length
//    )
//
// Routine Description:
//
//    This function moves memory either forward or backward, aligned or
//    unaligned, in 64-byte blocks, followed by 8-byte blocks, followed
//    by any remaining bytes.
//
//    N.B. These functions use the same code since the address format in
//         the argument registers is identical.
//
// Arguments:
//
//    Destination (a0) - Supplies a pointer to the destination address of
//       the move operation.
//
//    Source (a1) - Supplies a pointer to the source address of the move
//       operation.
//
//    Length (a2) - Supplies the length, in bytes, of the memory to be moved.
//
// Return Value:
//
//    None.
//
//--

        LEAF_ENTRY(RtlMoveMemory)

        ALTERNATE_ENTRY(RtlCopyMemory)

        beq     a2, 80f                 // if eq, no bytes to move

//
// If the source address is less than the destination address and source
// address plus the length of the move is greater than the destination
// address, then the source and destination overlap such that the move
// must be performed backwards.
//

        cmpult  a0, a1, t0              // is destination less than source
        bne     t0, MoveForward         // if eq [true] no overlap possible
        addq    a1, a2, t0              // compute source ending address
        cmpult  t0, a0, t1              // is source end less than dest.
        beq     t1, MoveBackward        // if eq [false], overlap

//
// Move memory forward aligned and unaligned.
//

MoveForward:                            //
        xor     a0, a1, t0              // compare alignment bits
        and     t0, 0x7, t0             // isloate alignment comparison
        bne     t0, MoveForwardUnaligned  // if ne, incompatible alignment

//
// Move memory forward aligned.
//

MoveForwardAligned:                     //

//
// Move bytes until source and destination are quadword aligned
//

        and     a0, 0x7, t0             // t0 = unaligned bits
        bne     t0, 5f                  // if ne, not quad aligned
        br      zero, 20f               // predicted taken

5:
        ldq_u   t2, 0(a0)               // get unaligned quad from dest.
        ldq_u   t1, 0(a1)               // get unaligned quadword from source
10:
        beq     a2, 15f                 // if eq, all bytes moved
        extbl   t1, t0, t3              // t3 = byte from source
        insbl   t3, t0, t3              // t3 = byte from source, in position
        mskbl   t2, t0, t2              // clear position in dest. quad
        bis     t2, t3, t2              // merge in byte from source
        subq    a2, 1, a2               // decrement bytes to move
        addq    t0, 1, t0               // increment byte within quad
        cmpeq   t0, 8, t3               // finished the quadword?
        beq     t3, 10b                 // if eq [false], do next byte
15:
        stq_u   t2, 0(a0)               // store merged destination bytes

        addq    a0, 7, a0               // move to next quadword
        bic     a0, 7, a0               // aligned quadword

        addq    a1, 7, a1               // move to next quadword
        bic     a1, 7, a1               // aligned quadword

//
// Check for 64-byte block moves
//

20:
        srl     a2, 6, t0               // t0 = number of 64 byte blocks
        beq     t0, 40f                 // if eq no blocks to move
        and     a2, 64-1, a2            // a2 = residual bytes

30:
        ldq     t1, 0(a1)               // load 64 bytes from source
        addq    a0, 64, a0              // increment destination pointer
        ldq     v0, 56(a1)              //
        ldq     a3, 32(a1)              //
        stq     t1, -64(a0)             // write to destination
        ldq     t2, 8(a1)               //   into volatile registers
        ldq     t3, 16(a1)              //
        ldq     t4, 24(a1)              //
        subq    t0, 1, t0               // decrement number of blocks
        stq     t2, -56(a0)             //
        ldq     a4, 40(a1)              //
        stq     t3, -48(a0)             //
        ldq     a5, 48(a1)              //
        stq     t4, -40(a0)             //
        addq    a1, 64, a1              // increment source pointer
        stq     a3, -32(a0)             //
        stq     a4, -24(a0)             //
        stq     a5, -16(a0)             //
        stq     v0, -8(a0)              //
        bne     t0, 30b                 // if ne, more blocks to copy

//
// Copy quadwords
//

40:
        srl     a2, 3, t0               // t0 = number of quadwords to move
        beq     t0, 60f                 // if eq no quadwords to move
        and     a2, 8-1, a2             // a2 = residual bytes

50:
        ldq     t1, 0(a1)               // load quadword from source
        addq    a1, 8, a1               // increment source pointer
        stq     t1, 0(a0)               // store quadword to destination
        addq    a0, 8, a0               // increment destination pointer
        subq    t0, 1, t0               // decrement number of quadwords
        bne     t0, 50b                 // if ne, more quadwords to move

//
// Move final residual bytes
//

60:
        beq     a2, 80f                 // if eq, no more bytes to move
        ldq     t1, 0(a1)               // get last source quadword
        ldq     t2, 0(a0)               // get last dest. quadword
        bis     zero, zero, t0          // t0 = next byte number to move

70:
        extbl   t1, t0, t3              // extract byte from source
        insbl   t3, t0, t3              // t3 = source byte, in position
        mskbl   t2, t0, t2              // clear byte position for dest.
        bis     t2, t3, t2              // merge in source byte
        addq    t0, 1, t0               // increment byte position
        subq    a2, 1, a2               // decrement bytes to move
        bne     a2, 70b                 // if ne => more bytes to move

        stq     t2, 0(a0)               // store merged data

//
// Finish aligned MoveForward
//

80:
        ret     zero, (ra)              // return



//
// Move memory forward unaligned.
//

MoveForwardUnaligned:                   //


//
// Move bytes until the destination is aligned
//

        and     a0, 0x7, t0             // t0 = unaligned bits
        beq     t0, 100f                // if eq, destination quad aligned

        ldq_u   t2, 0(a0)               // get unaligned quad from dest

90:
        beq     a2, 95f                 // if eq no more bytes to move
        ldq_u   t1, 0(a1)               // get unaligned quad from source
        extbl   t1, a1, t1              // extract source byte
        insbl   t1, t0, t1              // t1 = source byte, in position
        mskbl   t2, t0, t2              // clear byte position in dest.
        bis     t2, t1, t2              // merge in source byte
        addq    t0, 1, t0               // increment byte position
        addq    a1, 1, a1               // increment source pointer
        subq    a2, 1, a2               // decrement bytes to move
        cmpeq   t0, 8, t3               // t0 = 8? => quad finished
        beq     t3, 90b                 // if eq [false], more bytes to move
95:
        stq_u   t2, 0(a0)               // store merged quadword
        addq    a0, 7, a0               // increment to next quad
        bic     a0, 7, a0               // align next quadword

//
// Check for 64-byte blocks to move
//

100:
        srl     a2, 6, t0               // t0 = number of blocks to move
        beq     t0, 120f                // if eq no blocks to move
        and     a2, 64-1, a2            // a2 = residual bytes to move


        ldq_u   t1, 0(a1)               // t1 = first unaligned quad

110:
                                        // get source data and merge it
                                        //  as we go
        ldq_u   t2, 7(a1)               // t2 = second unaligned quad
        extql   t1, a1, t1              // extract applicable bytes from t1
        extqh   t2, a1, v0              // extract applicable bytes from t2
        bis     t1, v0, t1              // t1 = quad #1
        ldq_u   t3, 15(a1)              // t3 = third unaligned quad
        extql   t2, a1, t2              // extract applicable bytes from t2
        extqh   t3, a1, v0              // extract applicable bytes from t3
        stq     t1, 0(a0)               // store quad #1
        bis     t2, v0, t2              // t2 = quad #2
        ldq_u   t4, 23(a1)              // t4 = fourth unaligned quad
        extql   t3, a1, t3              // extract applicable bytes from t3
        extqh   t4, a1, v0              // extract applicable bytes from t4
        stq     t2, 8(a0)               // store quad #2
        bis     t3, v0, t3              // t3 = quad #3
        ldq_u   t5, 31(a1)              // t5 = fifth unaligned quad
        extql   t4, a1, t4              // extract applicable bytes from t4
        extqh   t5, a1, v0              // extract applicable bytes from t5
        stq     t3, 16(a0)              // store quad #3
        bis     t4, v0, t4              // t4 = quad #4
        ldq_u   a3, 39(a1)              // a3 = sixth unaligned quad
        extql   t5, a1, t5              // extract applicable bytes from t5
        extqh   a3, a1, v0              // extract applicable bytes from a3
        stq     t4, 24(a0)              // store quad #4
        bis     t5, v0, t5              // t5 = quad #5
        ldq_u   a4, 47(a1)              // a4 = seventh unaligned quad
        extql   a3, a1, a3              // extract applicable bytes from a3
        extqh   a4, a1, v0              // extract applicable bytes from a4
        stq     t5, 32(a0)              // store quad #5
        bis     a3, v0, a3              // a3 = quad #6
        ldq_u   a5, 55(a1)              // a5 = eighth unaligned quad
        extql   a4, a1, a4              // extract applicable bytes from a4
        extqh   a5, a1, v0              // extract applicable bytes from a5
        stq     a3, 40(a0)              // store quad #6
        bis     a4, v0, a4              // a4 = quad #7
        ldq_u   t1, 63(a1)              // t1 = ninth unaligned = 1st of next
        extql   a5, a1, a5              // extract applicable bytes from a5
        extqh   t1, a1, v0              // extract applicable bytes from t1
        stq     a4, 48(a0)              // store quad #7
        bis     a5, v0, a5              // a5 = quad #8
        addq    a1, 64, a1              // increment source pointer
        stq     a5, 56(a0)              // store quad #8
        addq    a0, 64, a0              // increment destination pointer
        subq    t0, 1, t0               // decrement number of blocks
        bne     t0, 110b                // if ne, more blocks to move

//
// Move unaligned source quads to aligned destination quads
//

120:
        srl     a2, 3, t0               // t0 = number of quads to move
        beq     t0, 140f                // if eq no quads to move
        and     a2, 8-1, a2             // a2 = residual bytes


        ldq_u   t1, 0(a1)               // t1 = first unaligned quad
130:
        ldq_u   t2, 7(a1)               // t2 = second unaligned quad
        addq    a0, 8, a0               // increment destination pointer
        extql   t1, a1, t1              // extract applicable bytes from t1
        extqh   t2, a1, v0              // extract applicable bytes from t2
        bis     t1, v0, t1              // t1 = quadword of data
        stq     t1, -8(a0)              // store data to destination
        addq    a1, 8, a1               // increment source pointer
        subq    t0, 1, t0               // decrement quads to move
        bis     t2, zero, t1            // t1 = first of next unaligned pair
        bne     t0, 130b                // if ne, more quads to move

//
// Move remaining bytes to final quadword
//


140:
        beq     a2, 160f                // if eq no more bytes to move
        ldq     t2, 0(a0)               // t2 = destination quadword
        bis     zero, zero, t3          // t3 = position for next insertion

150:
        ldq_u   t1, 0(a1)               // get unaligned source quad
        extbl   t1, a1, t1              // t1 = source byte
        insbl   t1, t3, t1              // t1 = source byte, in position
        mskbl   t2, t3, t2              // clear byte in destination
        bis     t2, t1, t2              // merge in source byte
        addq    a1, 1, a1               // increment source pointer
        subq    a2, 1, a2               // decrement bytes to move
        addq    t3, 1, t3               // increment destination position
        bne     a2, 150b                // more bytes to move

        stq     t2, 0(a0)               // store merged data

//
// Finish unaligned MoveForward
//

160:
        ret     zero, (ra)              // return


//
// Move memory backward.
//

MoveBackward:                           //

        addq    a0, a2, a0              // compute ending destination address
        addq    a1, a2, a1              // compute ending source address
        subq    a0, 1, a0               // point to last destination byte
        subq    a1, 1, a1               // point to last source byte
        xor     a0, a1, t0              // compare alignment bits
        and     t0, 0x7, t0             // isolate alignment comparison
        bne     t0, MoveBackwardUnaligned   // if ne, incompatible alignment

//
// Move memory backward aligned.
//

MoveBackwardAligned:                    //

//
// Move bytes until source and destination are quadword aligned
//

        and     a0, 0x7, t0             // t0 = unaligned bits
        cmpeq   t0, 7, t1               // last byte position 7?
        beq     t1, 5f                  // if eq [false], not quad aligned
        subq    a0, 7, a0               // point to beginning of last quad
        subq    a1, 7, a1               // point to beginning of last quad
        br      zero, 30f               // predicted taken

5:
        ldq_u   t1, 0(a0)               // get unaligned quad from dest.
        ldq_u   t2, 0(a1)               // get unaligned quad from source

10:
        beq     a2, 20f                 // if eq, all bytes moved
        extbl   t2, t0, t3              // t3 = byte from source
        insbl   t3, t0, t3              // t3 = byte from source, in position
        mskbl   t1, t0, t1              // clear position in destination
        bis     t1, t3, t1              // merge in byte from source
        subq    a2, 1, a2               // decrement bytes to move
        subq    t0, 1, t0               // decrement byte within quadword
        cmplt   t0, zero, t3            // finished the quadword?
        beq     t3, 10b                 // if eq [false], do next byte

20:
        stq_u   t1, 0(a0)               // store merged destination bytes

        subq    a0, 8, a0               // move to previous quadword
        bic     a0, 7, a0               // aligned quadword

        subq    a1, 8, a1               // move to previous quadword
        bic     a1, 7, a1               // aligned quadword

//
// Check for 64-byte block moves
//

30:

        srl     a2, 6, t0               // t0 = number of 64 byte blocks
        beq     t0, 50f                 // if eq, no blocks to move
        and     a2, 64-1, a2            // a2 = residual bytes

40:
        ldq     t1, 0(a1)               // load 64 bytes from source into
        subq    a0, 64, a0              // decrement destination pointer
        ldq     v0, -56(a1)             //
        ldq     a3, -32(a1)             //
        stq     t1, 64(a0)              // write to destination
        ldq     t2, -8(a1)              //   into volatile registers
        ldq     a5, -48(a1)             //
        ldq     a4, -40(a1)             //
        stq     t2, 56(a0)              //
        ldq     t3, -16(a1)             //
        ldq     t4, -24(a1)             //
        subq    a1, 64, a1              // decrement source pointer
        stq     t3, 48(a0)              //
        stq     t4, 40(a0)              //
        stq     a3, 32(a0)              //
        subq    t0, 1, t0               // decrement number of blocks
        stq     a4, 24(a0)              //
        stq     a5, 16(a0)              //
        stq     v0, 8(a0)               //
        bne     t0, 40b                 // if ne, more blocks to copy

//
// Copy quadwords
//

50:
        srl     a2, 3, t0               // t0 = number of quadwords to move
        beq     t0, 70f                 // if eq no quadwords to move
        and     a2, 8-1, a2             // a2 = residual bytes

60:
        ldq     t1, 0(a1)               // load quadword from source
        subq    a1, 8, a1               // decrement source pointer
        stq     t1, 0(a0)               // store quadword to destination
        subq    a0, 8, a0               // decrement destination pointer
        subq    t0, 1, t0               // decrement quadwords to move
        bne     t0, 60b                 // if ne, more quadwords to move

//
// Move final residual bytes
//

70:
        beq     a2, 90f                 // if eq, no more bytes to move
        ldq     t1, 0(a1)               // get last source quadword
        ldq     t2, 0(a0)               // get last destination quadword
        bis     zero, 7, t0             // t0 = next byte number to move

80:
        extbl   t1, t0, t3              // extract byte from source
        insbl   t3, t0, t3              // t3 = source byte, in position
        mskbl   t2, t0, t2              // clear byte position for dest.
        bis     t2, t3, t2              // merge in source byte
        subq    t0, 1, t0               // decrement byte position
        subq    a2, 1, a2               // decrement bytes to move
        bne     a2, 80b                 // if ne, more bytes to move

        stq     t2, 0(a0)               // write destination data
//
// Finish aligned MoveBackward
//

90:

        ret     zero, (ra)              // return


//
// Move memory backward unaligned.
//

MoveBackwardUnaligned:                  //


//
// Move bytes until the destination is aligned
//

        and     a0, 0x7, t0             // t0 = unaligned bits
        cmpeq   t0, 7, t1               // last byte of a quadword
        beq     t1, 95f                 // if eq[false], not aligned
        subq    a0, 7, a0               // align pointer to beginning of quad
        br      zero, 120f              //

95:
        ldq_u   t2, 0(a0)               // get unaligned quad from dest.

100:
        beq     a2, 110f                // if eq, no more bytes to move
        ldq_u   t1, 0(a1)               // get unaligned quad from source
        extbl   t1, a1, t1              // extract source byte
        insbl   t1, t0, t1              // t1 = source byte in position
        mskbl   t2, t0, t2              // clear byte position in dest.
        bis     t2, t1, t2              // merge source byte
        subq    t0, 1, t0               // decrement byte position
        subq    a1, 1, a1               // decrement source pointer
        subq    a2, 1, a2               // decrement number of bytes to move
        cmplt   t0, zero, t3            // t0 < 0? => quad finished
        beq     t3, 100b                // if eq [false], more bytes to move

110:
        stq_u   t2, 0(a0)               // store merged quadword

        subq    a0, 8, a0               // decrement dest. to previous quad
        bic     a0, 7, a0               // align previous quadword

//
// Check for 64-byte blocks to move
//

120:

        srl     a2, 6, t0               // t0 = number of blocks to move
        subq    a1, 7, a1               // point to beginning of last quad
        beq     t0, 140f                // if eq no blocks to move
        and     a2, 64-1, a2            // a2 = residual bytes to move

        ldq_u   t1, 7(a1)               // t1 = first unaligned quad

130:
                                        // get source data and merge it
                                        //  as we go
        ldq_u   t2, 0(a1)               // t2 = second unaligned quad
        extqh   t1, a1, t1              // extract applicable bytes from t1
        extql   t2, a1, v0              // extract applicable bytes from t2
        bis     t1, v0, t1              // t1 = quad #1
        ldq_u   t3, -8(a1)              // t3 = third unaligned quad
        extqh   t2, a1, t2              // extract applicable bytes from t2
        extql   t3, a1, v0              // extract applicable bytes from t3
        stq     t1, 0(a0)               // store quad #1
        bis     t2, v0, t2              // t2 = quad #2
        ldq_u   t4, -16(a1)             // t4 = fourth unaligned quad
        extqh   t3, a1, t3              // extract applicable bytes from t3
        extql   t4, a1, v0              // extract applicable bytes from t4
        stq     t2, -8(a0)              // store quad #2
        bis     t3, v0, t3              // t3 = quad #3
        ldq_u   t5, -24(a1)             // t5 = fifth unaligned quad
        extqh   t4, a1, t4              // extract applicable bytes from t4
        extql   t5, a1, v0              // extract applicable bytes from t5
        stq     t3, -16(a0)             // store quad #3
        bis     t4, v0, t4              // t4 = quad #4
        ldq_u   a3, -32(a1)             // a3 = sixth unaligned quad
        extqh   t5, a1, t5              // extract applicable bytes from t5
        extql   a3, a1, v0              // extract applicable bytes from a3
        stq     t4, -24(a0)             // store quad #4
        bis     t5, v0, t5              // t5 = quad #5
        ldq_u   a4, -40(a1)             // a4 = seventh unaligned quad
        extqh   a3, a1, a3              // extract applicable bytes from a3
        extql   a4, a1, v0              // extract applicable bytes from a4
        stq     t5, -32(a0)             // store quad #5
        bis     a3, v0, a3              // a3 = quad #6
        ldq_u   a5, -48(a1)             // a5 = eighth unaligned quad
        extqh   a4, a1, a4              // extract applicable bytes from a4
        extql   a5, a1, v0              // extract applicable bytes from a5
        stq     a3, -40(a0)             // store quad #6
        bis     a4, v0, a4              // a4 = quad #7
        ldq_u   t1, -56(a1)             // t1 = ninth unaligned = 1st of next
        extqh   a5, a1, a5              // extract applicable bytes from a5
        extql   t1, a1, v0              // extract applicable bytes from t1
        stq     a4, -48(a0)             // store quad #7
        bis     a5, v0, a5              // a5 = quad #8
        subq    a1, 64, a1              // increment source pointer
        stq     a5, -56(a0)             // store quad #8
        subq    a0, 64, a0              // increment destination pointer
        subq    t0, 1, t0               // decrement number of blocks
        bne     t0, 130b                // if ne, more blocks to move


//
// Move unaligned source quads to aligned destination quads
//

140:
        srl     a2, 3, t0               // t0 = number of quads to move
        beq     t0, 160f                // if eq no quads to move
        and     a2, 8-1, a2             // a2 = residual bytes

        ldq_u   t1, 7(a1)               // t1 = first unaligned quad

150:
        ldq_u   t2, 0(a1)               // t2 = second unaligned quad
        subq    a0, 8, a0               // decrement destination pointer
        extqh   t1, a1, t1              // extract applicable bytes from t1
        extql   t2, a1, v0              // extract applicable bytes from t2
        bis     t1, v0, t1              // t1 = quadword of data
        stq     t1, 8(a0)               // store data to destination
        subq    a1, 8, a1               // decrement source pointer
        subq    t0, 1, t0               // decrement quads to move
        bis     t2, zero, t1            // t1 = first of next unaligned pair
        bne     t0, 150b                // if ne, more quads to move

//
// Move remaining bytes to final quadword
//

160:
        beq     a2, 180f                // if eq, no more bytes to move
        ldq     t2, 0(a0)               // t2 = destination quadword
        bis     zero, 7, t0             // t0 = position for next insertion

170:
        subq    a1, 1, a1               // decrement source pointer
        ldq_u   t1, 8(a1)               // get unaligned source quad
        extbl   t1, a1, t1              // t1 = source byte
        insbl   t1, t0, t1              // t1 = source byte, in position
        mskbl   t2, t0, t2              // clear byte position
        bis     t2, t1, t2              // merge in source byte
        subq    t0, 1, t0               // decrement byte position for dest.
        subq    a2, 1, a2               // decrement bytes to move
        bne     a2, 170b                // if ne, more bytes to move

        stq     t2, 0(a0)               //

//
// Finish unaligned MoveBackward
//

180:
        ret     zero, (ra)              // return

        .end    RtlMoveMemory

        SBTTL("Zero Memory")
//++
//
// VOID
// RtlZeroMemory (
//    IN PVOID Destination,
//    IN ULONG Length
//    )
//
// PVOID64
// RtlZeroMemory64 (
//    IN PVOID64 Destination,
//    IN ULONG Length
//    )
//
// Routine Description:
//
//    This function zeros memory by first aligning the destination address to
//    a quadword boundary, and then zeroing 64-byte blocks, followed by 8-byte
//    blocks, followed by any remaining bytes.
//
//    N.B. These functions use the same code since the address format in
//         the argument registers is identical.
//
// Arguments:
//
//    Destination (a0) - Supplies a pointer to the memory to zero.
//
//    Length (a1) - Supplies the length, in bytes, of the memory to be zeroed.
//
// Return Value:
//
//    None.
//
//--

        LEAF_ENTRY(RtlZeroMemory)

        ALTERNATE_ENTRY(RtlZeroMemory64)

        bis     zero, zero, a2          // set fill pattern
        br      zero, RtlpFillMemory    //


        SBTTL("Fill Memory")
//++
//
// VOID
// RtlFillMemory (
//    IN PVOID Destination,
//    IN ULONG Length,
//    IN UCHAR Fill
//    )
//
// PVOID64
// RtlFillMemory64 (
//    IN PVOID64 Destination,
//    IN ULONG Length,
//    IN UCHAR Fill
//    )
//
// Routine Description:
//
//    This function fills memory by first aligning the destination address to
//    a longword boundary, and then filling 32-byte blocks, followed by 4-byte
//    blocks, followed by any remaining bytes.
//
//    N.B. These functions use the same code since the address format in
//         the argument registers is identical.
//
// Arguments:
//
//    Destination (a0) - Supplies a pointer to the memory to fill.
//
//    Length (a1) - Supplies the length, in bytes, of the memory to be filled.
//
//    Fill (a2) - Supplies the fill byte.
//
//    N.B. The alternate entry memset expects the length and fill arguments
//         to be reversed.  It also returns the Destination pointer
//
// Return Value:
//
//    None.
//
//--

        ALTERNATE_ENTRY(memset)

        bis     a0, zero, v0            // set return value
        bis     a1, zero, a3            // swap length and fill arguments
        bis     a2, zero, a1            //
        bis     a3, zero, a2            //

        ALTERNATE_ENTRY(RtlFillMemory)
        ALTERNATE_ENTRY(RtlFillMemory64)

        and     a2, 0xff, a2            // clear excess bits
        sll     a2, 8, t0               // duplicate fill byte
        bis     a2, t0, a2              // generate fill word
        sll     a2, 16, t0              // duplicate fill word
        bis     a2, t0, a2              // generate fill longword
        sll     a2, 32, t0              // duplicate fill longword
        bis     a2, t0, a2              // generate fill quadword

.align 3                                // ensure quadword aligned target
//
// Fill memory with the pattern specified in register a2.
//

RtlpFillMemory:                         //

//
// Align destination to quadword
//

        beq     a1, 80f                 // anything to fill? (paranoia)
        and     a0, 8-1, t0             // t0 = unaligned bits
        bne     t0, 5f                  // if ne, then not quad aligned
        br      zero, 20f               // if eq, then quad aligned

5:
        ldq_u   t1, 0(a0)               // get unaligned quadword
                                        //   for first group of bytes
10:
        beq     a1, 15f                 // if eq no more bytes to fill
        insbl   a2, t0, t2              // get fill byte into position
        mskbl   t1, t0, t1              // clear byte for fill
        bis     t1, t2, t1              // put in fill byte
        addq    t0, 1, t0               // increment to next byte position
        subq    a1, 1, a1               // decrement bytes to fill
        cmpeq   t0, 8, t2               // t0 = 8?
        beq     t2, 10b                 // if eq [false] more bytes to do

15:
        stq_u   t1, 0(a0)               // store modified bytes
        addq    a0, 7, a0               // move a0 to next quadword
        bic     a0, 7, a0               // align a0 to quadword

//
// Check for 64-byte blocks
//

20:
        srl     a1, 6, t0               // t0 = number of 64 byte blocks
        beq     t0, 40f                 // if eq then no 64 byte blocks
        and     a1, 64-1, a1            // a1 = residual bytes to fill

30:
        stq     a2, 0(a0)               // store 64 bytes
        stq     a2, 8(a0)               //
        stq     a2, 16(a0)              //
        stq     a2, 24(a0)              //
        stq     a2, 32(a0)              //
        stq     a2, 40(a0)              //
        stq     a2, 48(a0)              //
        stq     a2, 56(a0)              //

        subq    t0, 1, t0               // decrement blocks remaining
        addq    a0, 64, a0              // increment destination pointer
        bne     t0, 30b                 // more blocks to write



//
// Fill aligned quadwords
//

40:
        srl     a1, 3, t0               // t0 = number of quadwords
        bne     t0, 55f                 // if ne quadwords left to fill
        br      zero, 60f               // if eq no quadwords left

55:
        and     a1, 8-1, a1             // a1 = residual bytes to fill

50:
        stq     a2, 0(a0)               // store quadword
        subq    t0, 1, t0               // decrement quadwords remaining
        addq    a0, 8, a0               // next quadword
        bne     t0, 50b                 // more quadwords to write


//
// Fill bytes for last quadword
//

60:
        bne     a1, 65f                 // if ne bytes remain to be filled
        br      zero, 80f               // if eq no more bytes to fill

65:
        ldq     t1, 0(a0)               // get last quadword
        bis     zero, zero, t0          // t0 = byte position to start fill

70:
        beq     a1, 75f                 // if eq, no more bytes to fill
        insbl   a2, t0, t2              // get fill byte into position
        mskbl   t1, t0, t1              // clear fill byte position
        bis     t1, t2, t1              // insert fill byte
        addq    t0, 1, t0               // increment byte within quad
        subq    a1, 1, a1               // decrement bytes to fill
        cmpeq   t0, 8, t3               // t0 = 8? => finished quad
        beq     t3, 70b                 // if eq [false] more bytes to fill

75:
        stq     t1, 0(a0)               // write merged quadword

//
// Finish up
//

80:
        ret     zero, (ra)              // return


        .end    RtlZeroMemory

        SBTTL("Fill Memory Ulong")
//++
//
// VOID
// RtlFillMemoryUlong (
//    IN PVOID Destination,
//    IN ULONG Length,
//    IN ULONG Pattern
//    )
//
// Routine Description:
//
//    This function fills memory with the specified longowrd pattern by
//    filling 64-byte blocks followed by 8-byte blocks and finally
//    4-byte blocks.
//
//    N.B. This routine assumes that the destination address is aligned
//         on a longword boundary and that the length is an even multiple
//         of longwords.
//
// Arguments:
//
//    Destination (a0) - Supplies a pointer to the memory to fill.
//
//    Length (a1) - Supplies the length, in bytes, of the memory to be filled.
//
//    Pattern (a2) - Supplies the fill pattern.
//
// Return Value:
//
//    None.
//
//--

        LEAF_ENTRY(RtlFillMemoryUlong)

        bic     a1, 3, a1               // make sure length is an even number
                                        // of longwords
        sll     a2, 32, a3              // a3 = long pattern in upper 32 bits
        srl     a3, 32, t0              // clear upper bits, pattern in lower 32
        bis     a3, t0, a3              // a3 = quad version of fill pattern

//
// Make destination address quad-aligned
//

        and     a0, 4, t0               // is a0 quad aligned?
        beq     t0, 10f                 // if eq, then a0 quad aligned
        stl     a2, 0(a0)               // fill first longword
        addq    a0, 4, a0               // quad align a0
        subq    a1, 4, a1               // bytes remaining to store

//
// Check for 64-byte blocks to fill
//

10:
        srl     a1, 6, t0               // t0 = # 64-byte blocks to fill
        beq     t0, 30f                 // if eq no 64 byte blocks
        and     a1, 64-1, a1            // a1 = residual bytes

20:
        stq     a3, 0(a0)               // store 64 bytes
        stq     a3, 8(a0)               //
        stq     a3, 16(a0)              //
        stq     a3, 24(a0)              //
        stq     a3, 32(a0)              //
        stq     a3, 40(a0)              //
        stq     a3, 48(a0)              //
        stq     a3, 56(a0)              //
        subq    t0, 1, t0               // t0 = blocks remaining
        addq    a0, 64, a0              // increment address pointer
        bne     t0, 20b                 // if ne more blocks to fill

//
// Fill 8 bytes at a time while we can, a1 = bytes remaining
//

30:
        srl     a1, 3, t0               // t0 = # quadwords to fill
        beq     t0, 50f                 // if eq no quadwords left
        and     a1, 8-1, a1             // a1 = residual bytes
40:
        stq     a3, 0(a0)               // store quadword
        subq    t0, 1, t0               // t0 = quadwords remaining
        addq    a0, 8, a0               // increment address pointer
        bne     t0, 40b                 // if ne more quadwords to fill

//
// Fill last 4 bytes
//

50:
        beq     a1, 60f                 // if eq no longwords remain
        stl     a2, 0(a0)               // fill last longword

//
// Finish up
//

60:
        ret     zero, (ra)              // return to caller


        .end    RtlFillMemoryUlong

        SBTTL("Fill Memory Ulonglong")
//++
//
// VOID
// RtlFillMemoryUlonglong (
//    IN PVOID Destination,
//    IN ULONG Length,
//    IN ULONGLONG Pattern
//    )
//
// Routine Description:
//
//    This function fills memory with the specified quadword pattern by
//    filling 64-byte blocks followed by 8-byte blocks.
//
//    N.B. This routine assumes that the destination address is aligned
//         on a quadword boundary and that the length is an even multiple
//         of quadword.
//
// Arguments:
//
//    Destination (a0) - Supplies a pointer to the memory to fill.
//
//    Length (a1) - Supplies the length, in bytes, of the memory to be filled.
//
//    Pattern (a2) - Supplies the fill pattern.
//
// Return Value:
//
//    None.
//
//--

        LEAF_ENTRY(RtlFillMemoryUlonglong)

        bic     a1, 4, a1               // make sure length is an even number
                                        // of longwords
//
// Check for 64-byte blocks to fill
//

10:
        srl     a1, 6, t0               // t0 = # 64-byte blocks to fill
        beq     t0, 30f                 // if eq no 64 byte blocks
        and     a1, 64-1, a1            // a1 = residual bytes

20:
        stq     a2, 0(a0)               // store 64 bytes
        stq     a2, 8(a0)               //
        stq     a2, 16(a0)              //
        stq     a2, 24(a0)              //
        stq     a2, 32(a0)              //
        stq     a2, 40(a0)              //
        stq     a2, 48(a0)              //
        stq     a2, 56(a0)              //
        subq    t0, 1, t0               // t0 = blocks remaining
        addq    a0, 64, a0              // increment address pointer
        bne     t0, 20b                 // if ne more blocks to fill

//
// Fill 8 bytes at a time while we can, a1 = bytes remaining
//

30:
        beq     a1, 50f
40:
        stq     a2, 0(a0)               // store quadword
        subq    a1, 8, a1               // a1 = quadwords remaining
        addq    a0, 8, a0               // increment address pointer
        bne     a1, 40b                 // if ne more quadwords to fill

50:
        ret     zero, (ra)              // return to caller


        .end    RtlFillMemoryUlonglong

        SBTTL("Copy Memory With Byte Granularity")
//++
//
// VOID
// RtlCopyBytes (
//    IN PVOID Destination,
//    IN PVOID Source,
//    IN ULONG Length
//    )
//
// Routine Description:
//
//    This function copies non-overlapping memory, aligned or unaligned, in
//    64-byte blocks, followed by 8-byte blocks, followed by any remaining
//    bytes.  Unlike RtlCopyMemory or RtlMoveMemory the copy is done such
//    that byte granularity is assured for all platforms.
//
// Arguments:
//
//    Destination (a0) - Supplies a pointer to the destination address of
//       the move operation.
//
//    Source (a1) - Supplies a pointer to the source address of the move
//       operation.
//
//    Length (a2) - Supplies the length, in bytes, of the memory to be moved.
//
// Return Value:
//
//    None.
//
//--

        LEAF_ENTRY(RtlCopyBytes)

//
// Move memory forward aligned and unaligned.
//

        xor     a0, a1, t0              // compare alignment bits
        and     t0, 0x7, t0             // isolate alignment comparison
        bne     t0, CopyForwardUnaligned // if ne, incompatible alignment

//
// Source and Destination buffers have the same alignment. Move
// bytes until done or source and destination are quadword aligned
//

        and     a0, 0x7, t0             // t0 = unaligned bits
        bne     t0, 5f                  // if ne, not quad aligned
        br      zero, 20f               // predicted taken
5:
        bis     zero, zero, t1          // t4 = destination byte zap mask
        bis     zero, 1, t2
        sll     t2, t0, t2              // t2 = next bit to set in zap mask
10:
        beq     a2, 15f                 // if eq, all bits set
        bis     t1, t2, t1              // set bit in zap mask
        sll     t2, 1, t2               // set next higher bit for zap mask
        subq    a2, 1, a2               // decrement bytes to move
        addq    t0, 1, t0               // increment byte within quad
        cmpeq   t0, 8, t3               // finished the quadword?
        beq     t3, 10b                 // if eq [false], do next byte
15:
        ldq_u   t2, 0(a1)               // get unaligned quadword from source
        zapnot  t2, t1, t2              // clear source bytes
        bic     a0, 7, a3               // a3 = quadword base of destination
retry1:
        ldq_l   t0, 0(a3)               // load destination quadword
        zap     t0, t1, t0              // clear destination bytes
        or      t0, t2, t0              // merge in bytes from source
        stq_c   t0, 0(a3)               // store merged quadword conditional
        beq     t0, retry1f             // if eq, retry failed interlock

        addq    a0, 7, a0               // move to next quadword
        bic     a0, 7, a0               // aligned quadword

        addq    a1, 7, a1               // move to next quadword
        bic     a1, 7, a1               // aligned quadword

//
// Check for 64-byte block moves
//

20:
        srl     a2, 6, t0               // t0 = number of 64 byte blocks
        beq     t0, 40f                 // if eq no blocks to move
        and     a2, 64-1, a2            // a2 = residual bytes

30:
        ldq     t1, 0(a1)               // load 64 bytes from source
        addq    a0, 64, a0              // increment destination pointer
        ldq     v0, 56(a1)              //
        ldq     a3, 32(a1)              //
        stq     t1, -64(a0)             // write to destination
        ldq     t2, 8(a1)               //   into volatile registers
        ldq     t3, 16(a1)              //
        ldq     t4, 24(a1)              //
        subq    t0, 1, t0               // decrement number of blocks
        stq     t2, -56(a0)             //
        ldq     a4, 40(a1)              //
        stq     t3, -48(a0)             //
        ldq     a5, 48(a1)              //
        stq     t4, -40(a0)             //
        addq    a1, 64, a1              // increment source pointer
        stq     a3, -32(a0)             //
        stq     a4, -24(a0)             //
        stq     a5, -16(a0)             //
        stq     v0, -8(a0)              //
        bne     t0, 30b                 // if ne, more blocks to copy

//
// Copy quadwords
//

40:
        srl     a2, 3, t0               // t0 = number of quadwords to move
        beq     t0, 60f                 // if eq no quadwords to move
        and     a2, 8-1, a2             // a2 = residual bytes

50:
        ldq     t1, 0(a1)               // load quadword from source
        addq    a1, 8, a1               // increment source pointer
        stq     t1, 0(a0)               // store quadword to destination
        addq    a0, 8, a0               // increment destination pointer
        subq    t0, 1, t0               // decrement number of quadwords
        bne     t0, 50b                 // if ne, more quadwords to move

//
// Move final residual bytes
//

60:
        beq     a2, 80f                 // if eq, no more bytes to move
        mov     a2, t0                  // t0 = number of bytes to move
        mov     -1, t1                  // t1 = bit mask
        sll     t0, 3, t0               // # of bytes to # of bits
        srl     t1, t0, t1              // clear t0 bits
        sll     t1, t0, t0              // move it back
        ldq     t1, 0(a1)               // get last source quadword
        bic     t1, t0, t1              // clear bytes not copied
        not     t0, t0                  // complement to clear destination
retry2:
        ldq_l   t2, 0(a0)               // get last destination quadword locked
        bic     t2, t0, t2              // clear bytes to be copied
        bis     t2, t1, t2              // move bytes from source
        stq_c   t2, 0(a0)               // store merged quadword conditional
        beq     t2, retry2f             // if eq, retry failed interlock

//
// Finish aligned MoveForward
//

80:
        ret     zero, (ra)              // return

//
// Move memory forward unaligned.
//

CopyForwardUnaligned:                   //

//
// Move bytes until the destination is aligned
//

        and     a0, 0x7, t0             // t0 = unaligned bits
        beq     t0, 100f                // if eq, destination quad aligned
        bis     zero, zero, t1          // t4 = destination byte zap mask
        bis     zero, 1, t2
        sll     t2, t0, t2              // t2 = next bit to set in zap mask
        mov     zero, t4                // assemble destination bytes here
90:
        beq     a2, 95f                 // if eq no more bytes to move
        bis     t1, t2, t1              // set bit in zap mask
        sll     t2, 1, t2               // set next higher bit for zap mask
        ldq_u   t5, 0(a1)               // get unaligned quad from source
        extbl   t5, a1, t5              // extract source byte
        insbl   t5, t0, t5              // t5 = source byte, in position
        or      t4, t5, t4              // merge in source byte
        addq    t0, 1, t0               // increment byte position
        addq    a1, 1, a1               // increment source pointer
        subq    a2, 1, a2               // decrement bytes to move
        cmpeq   t0, 8, t3               // t0 = 8? => quad finished
        beq     t3, 90b                 // if eq [false], more bytes to move
95:
        bic     a0, 0x7, a3             // a3 = quadword base of destination
retry3:
        ldq_l   t0, 0(a3)               // load destination quadword
        zap     t0, t1, t0              // clear destination bytes
        or      t0, t4, t0              // merge in bytes from source
        stq_c   t0, 0(a3)               // store merged quadword conditional
        beq     t0, retry3f             // if eq, retry failed interlock

        addq    a0, 7, a0               // increment to next quad
        bic     a0, 7, a0               // align next quadword

//
// Check for 64-byte blocks to move
//

100:
        srl     a2, 6, t0               // t0 = number of blocks to move
        beq     t0, 120f                // if eq no blocks to move
        and     a2, 64-1, a2            // a2 = residual bytes to move

        ldq_u   t1, 0(a1)               // t1 = first unaligned quad
110:
                                        // get source data and merge it
                                        //  as we go
        ldq_u   t2, 7(a1)               // t2 = second unaligned quad
        extql   t1, a1, t1              // extract applicable bytes from t1
        extqh   t2, a1, v0              // extract applicable bytes from t2
        bis     t1, v0, t1              // t1 = quad #1
        ldq_u   t3, 15(a1)              // t3 = third unaligned quad
        extql   t2, a1, t2              // extract applicable bytes from t2
        extqh   t3, a1, v0              // extract applicable bytes from t3
        stq     t1, 0(a0)               // store quad #1
        bis     t2, v0, t2              // t2 = quad #2
        ldq_u   t4, 23(a1)              // t4 = fourth unaligned quad
        extql   t3, a1, t3              // extract applicable bytes from t3
        extqh   t4, a1, v0              // extract applicable bytes from t4
        stq     t2, 8(a0)               // store quad #2
        bis     t3, v0, t3              // t3 = quad #3
        ldq_u   t5, 31(a1)              // t5 = fifth unaligned quad
        extql   t4, a1, t4              // extract applicable bytes from t4
        extqh   t5, a1, v0              // extract applicable bytes from t5
        stq     t3, 16(a0)              // store quad #3
        bis     t4, v0, t4              // t4 = quad #4
        ldq_u   a3, 39(a1)              // a3 = sixth unaligned quad
        extql   t5, a1, t5              // extract applicable bytes from t5
        extqh   a3, a1, v0              // extract applicable bytes from a3
        stq     t4, 24(a0)              // store quad #4
        bis     t5, v0, t5              // t5 = quad #5
        ldq_u   a4, 47(a1)              // a4 = seventh unaligned quad
        extql   a3, a1, a3              // extract applicable bytes from a3
        extqh   a4, a1, v0              // extract applicable bytes from a4
        stq     t5, 32(a0)              // store quad #5
        bis     a3, v0, a3              // a3 = quad #6
        ldq_u   a5, 55(a1)              // a5 = eighth unaligned quad
        extql   a4, a1, a4              // extract applicable bytes from a4
        extqh   a5, a1, v0              // extract applicable bytes from a5
        stq     a3, 40(a0)              // store quad #6
        bis     a4, v0, a4              // a4 = quad #7
        ldq_u   t1, 63(a1)              // t1 = ninth unaligned = 1st of next
        extql   a5, a1, a5              // extract applicable bytes from a5
        extqh   t1, a1, v0              // extract applicable bytes from t1
        stq     a4, 48(a0)              // store quad #7
        bis     a5, v0, a5              // a5 = quad #8
        addq    a1, 64, a1              // increment source pointer
        stq     a5, 56(a0)              // store quad #8
        addq    a0, 64, a0              // increment destination pointer
        subq    t0, 1, t0               // decrement number of blocks
        bne     t0, 110b                // if ne, more blocks to move

//
// Move unaligned source quads to aligned destination quads
//

120:
        srl     a2, 3, t0               // t0 = number of quads to move
        beq     t0, 140f                // if eq no quads to move
        and     a2, 8-1, a2             // a2 = residual bytes


        ldq_u   t1, 0(a1)               // t1 = first unaligned quad
130:
        ldq_u   t2, 7(a1)               // t2 = second unaligned quad
        addq    a0, 8, a0               // increment destination pointer
        extql   t1, a1, t1              // extract applicable bytes from t1
        extqh   t2, a1, v0              // extract applicable bytes from t2
        bis     t1, v0, t1              // t1 = quadword of data
        stq     t1, -8(a0)              // store data to destination
        addq    a1, 8, a1               // increment source pointer
        subq    t0, 1, t0               // decrement quads to move
        bis     t2, zero, t1            // t1 = first of next unaligned pair
        bne     t0, 130b                // if ne, more quads to move

//
// Move remaining bytes to final quadword
//

140:
        beq     a2, 160f                // if eq no more bytes to move

        mov     zero, t3                // t3 = position for next insertion
        mov     zero, t4                // assemble destination bytes here
        mov     a2, t0                  // t0 = number of bytes to move
        mov     -1, t1                  // t1 = bit mask
        sll     t0, 3, t0               // # of bytes to # of bits
        srl     t1, t0, t1              // clear t0 bits
        sll     t1, t0, t0              // move it back
        not     t0, t0                  // complement for destination clear mask
150:
        ldq_u   t1, 0(a1)               // get unaligned source quad
        extbl   t1, a1, t1              // t1 = source byte
        insbl   t1, t3, t1              // t1 = source byte, in position
        bis     t4, t1, t4              // merge in source byte
        addq    a1, 1, a1               // increment source pointer
        subq    a2, 1, a2               // decrement bytes to move
        addq    t3, 1, t3               // increment destination position
        bne     a2, 150b                // more bytes to move
retry4:
        ldq_l   t2, 0(a0)               // get last destination quadword locked
        bic     t2, t0, t2              // clear bytes to be copied
        bis     t2, t4, t2              // move bytes from source
        stq_c   t2, 0(a0)               // store merged quadword conditional
        beq     t2, retry4f             // if eq, retry failed interlock

//
// Finish unaligned MoveForward
//

160:
        ret     zero, (ra)              // return

//
// Out of line branches for failed store conditional.
// Don't need to restore anything, just try again.
//

retry1f:
        br      retry1
retry2f:
        br      retry2
retry3f:
        br      retry3
retry4f:
        br      retry4

        .end    RtlCopyBytes

        SBTTL("Zero Bytes")
//++
//
// VOID
// RtlZeroBytes (
//    IN PVOID Destination,
//    IN ULONG Length
//    )
//
// Routine Description:
//
//    This function zeros memory by first aligning the destination address to
//    a quadword boundary, and then zeroing 64-byte blocks, followed by 8-byte
//    blocks, followed by any remaining bytes. Unlike RtlZeroMemory the copy is
//    done such that byte granularity is assured for all platforms.
//
// Arguments:
//
//    Destination (a0) - Supplies a pointer to the memory to zero.
//
//    Length (a1) - Supplies the length, in bytes, of the memory to be zeroed.
//
// Return Value:
//
//    None.
//
//--

        LEAF_ENTRY(RtlZeroBytes)

        bis     zero, zero, a2          // set fill pattern
        br      zero, RtlpFillBytes     //


        SBTTL("Fill Bytes")
//++
//
// VOID
// RtlFillBytes (
//    IN PVOID Destination,
//    IN ULONG Length,
//    IN UCHAR Fill
//    )
//
// Routine Description:
//
//    This function fills memory by first aligning the destination address to
//    a longword boundary, and then filling 32-byte blocks, followed by 4-byte
//    blocks, followed by any remaining bytes. Unlike RtlFillMemory the copy is
//    done such that byte granularity is assured for all platforms.
//
// Arguments:
//
//    Destination (a0) - Supplies a pointer to the memory to fill.
//
//    Length (a1) - Supplies the length, in bytes, of the memory to be filled.
//
//    Fill (a2) - Supplies the fill byte.
//
//    N.B. The alternate entry memset expects the length and fill arguments
//         to be reversed.  It also returns the Destination pointer
//
// Return Value:
//
//    None.
//
//--

        ALTERNATE_ENTRY(RtlFillBytes)

        and     a2, 0xff, a2            // clear excess bits
        sll     a2, 8, t0               // duplicate fill byte
        bis     a2, t0, a2              // generate fill word
        sll     a2, 16, t0              // duplicate fill word
        bis     a2, t0, a2              // generate fill longword
        sll     a2, 32, t0              // duplicate fill longword
        bis     a2, t0, a2              // generate fill quadword

.align 3                                // ensure quadword aligned target
//
// Fill memory with the pattern specified in register a2.
//

RtlpFillBytes:                          //

//
// Align destination to quadword
//

        beq     a1, 80f                 // anything to fill? (paranoia)
        and     a0, 8-1, t0             // t0 = unaligned bits
        bne     t0, 5f                  // if ne, then not quad aligned
        br      zero, 20f               // if eq, then quad aligned

5:
        bis     zero, zero, t1          // t4 = destination byte zap mask
        bis     zero, 1, t2
        sll     t2, t0, t2              // t2 = next bit to set in zap mask
10:
        beq     a1, 15f                 // if eq, all bits set
        bis     t1, t2, t1              // set bit in zap mask
        sll     t2, 1, t2               // set next higher bit for zap mask
        subq    a1, 1, a1               // decrement bytes to fill
        addq    t0, 1, t0               // increment byte within quad
        cmpeq   t0, 8, t3               // finished the quadword?
        beq     t3, 10b                 // if eq [false], do next byte
15:
        zapnot  a2, t1, t2              // clear fill bytes
        bic     a0, 7, a3               // a3 = quadword base of destination
retry5:
        ldq_l   t0, 0(a3)               // load destination quadword
        zap     t0, t1, t0              // clear destination bytes
        or      t0, t2, t0              // merge in fill bytes
        stq_c   t0, 0(a3)               // store merged quadword conditional
        beq     t0, retry5f            // if eq, retry failed interlock

        addq    a0, 7, a0               // move a0 to next quadword
        bic     a0, 7, a0               // align a0 to quadword

//
// Check for 64-byte blocks
//

20:
        srl     a1, 6, t0               // t0 = number of 64 byte blocks
        beq     t0, 40f                 // if eq then no 64 byte blocks
        and     a1, 64-1, a1            // a1 = residual bytes to fill

30:
        stq     a2, 0(a0)               // store 64 bytes
        stq     a2, 8(a0)               //
        stq     a2, 16(a0)              //
        stq     a2, 24(a0)              //
        stq     a2, 32(a0)              //
        stq     a2, 40(a0)              //
        stq     a2, 48(a0)              //
        stq     a2, 56(a0)              //

        subq    t0, 1, t0               // decrement blocks remaining
        addq    a0, 64, a0              // increment destination pointer
        bne     t0, 30b                 // more blocks to write



//
// Fill aligned quadwords
//

40:
        srl     a1, 3, t0               // t0 = number of quadwords
        bne     t0, 55f                 // if ne quadwords left to fill
        br      zero, 60f               // if eq no quadwords left

55:
        and     a1, 8-1, a1             // a1 = residual bytes to fill

50:
        stq     a2, 0(a0)               // store quadword
        subq    t0, 1, t0               // decrement quadwords remaining
        addq    a0, 8, a0               // next quadword
        bne     t0, 50b                 // more quadwords to write

//
// Fill bytes for last quadword
//

60:
        beq     a1, 80f                 // if eq no more bytes to fill

        mov     a1, t0                  // t0 = number of bytes to move
        mov     -1, t1                  // t1 = bit mask
        sll     t0, 3, t0               // # of bytes to # of bits
        srl     t1, t0, t1              // clear t0 bits
        sll     t1, t0, t0              // move it back
        bic     a2, t0, t1              // clear fill bytes not copied
        not     t0, t0                  // complement to clear destination
retry6:
        ldq_l   t2, 0(a0)               // get last destination quadword locked
        bic     t2, t0, t2              // clear bytes to be copied
        bis     t2, t1, t2              // move bytes from source
        stq_c   t2, 0(a0)               // store merged quadword conditional
        beq     t2, retry6f             // if eq, retry failed interlock

//
// Finish up
//

80:
        ret     zero, (ra)              // return

//
// Out of line branches for failed store conditional.
// Don't need to restore anything, just try again.
//

retry5f:
        br      retry5
retry6f:
        br      retry6

        .end    RtlZeroBytes
