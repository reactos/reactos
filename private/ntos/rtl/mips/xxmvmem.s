//      TITLE("Compare, Move, Zero, and Fill Memory Support")
//++
//
// Copyright (c) 1990  Microsoft Corporation
//
// Module Name:
//
//    xxmvmem.s
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
//    David N. Cutler (davec) 11-Apr-1990
//
// Environment:
//
//    User or Kernel mode.
//
// Revision History:
//
//--

#include "ksmips.h"

        SBTTL("Compare Memory")
//++
//
// ULONG
// RtlCompareMemory (
//    IN PVOID Source1,
//    IN PVOID Source2,
//    IN ULONG Length
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

        daddu   a3,a0,a2                // compute ending address of source1
        move    v0,a2                   // save length of comparison
        and     t0,a2,32 - 1            // isolate residual bytes
        dsubu   t1,a2,t0                // subtract out residual bytes
        daddu   t4,a0,t1                // compute ending block address
        beq     zero,t1,100f            // if eq, no 32-byte block to compare
        or      t0,a0,a1                // merge and isolate alignment bits
        and     t0,t0,0x3               //
        bne     zero,t0,CompareUnaligned // if ne, unalignment comparison

//
// Compare memory aligned.
//

CompareAligned:                         //

        .set    noreorder
        .set    noat
10:     lw      t0,0(a0)                // compare 32-byte block
        lw      t1,0(a1)                //
        lw      t2,4(a0)                //
        bne     t0,t1,90f               // if ne, first word not equal
        lw      t3,4(a1)                //
        lw      t0,8(a0)                //
        bne     t2,t3,20f               // if ne, second word not equal
        lw      t1,8(a1)                //
        lw      t2,12(a0)               //
        bne     t0,t1,30f               // if ne, third word not equal
        lw      t3,12(a1)               //
        lw      t0,16(a0)               //
        bne     t2,t3,40f               // if ne, fourth word not equal
        lw      t1,16(a1)               //
        lw      t2,20(a0)               //
        bne     t0,t1,50f               // if ne, fifth word not equal
        lw      t3,20(a1)               //
        lw      t0,24(a0)               //
        bne     t2,t3,60f               // if ne, sixth word not equal
        lw      t1,24(a1)               //
        lw      t2,28(a0)               //
        bne     t0,t1,70f               // if ne, seventh word not equal
        lw      t3,28(a1)               //
        daddu   a0,a0,32                // advance source1 to next block
        bne     t2,t3,80f               // if ne, eighth word not equal
        nop                             //
        bne     a0,t4,10b               // if ne, more 32-byte blocks to compare
        daddu   a1,a1,32                // update source2 address
        .set    at
        .set    reorder

        dsubu   a2,a3,a0                // compute remaining bytes
        b       100f                    //

//
// Compare memory unaligned.
//

CompareUnaligned:                       //
        and     t0,a0,0x3               // isolate source1 alignment
        bne     zero,t0,CompareUnalignedS1 // if ne, source1 unaligned

//
// Source1 is aligned and Source2 is unaligned.
//

CompareUnalignedS2:                     //

        .set    noreorder
        .set    noat
10:     lw      t0,0(a0)                // compare 32-byte block
        lwr     t1,0(a1)                //
        lwl     t1,3(a1)                //
        lw      t2,4(a0)                //
        bne     t0,t1,90f               // if ne, first word not equal
        lwr     t3,4(a1)                //
        lwl     t3,7(a1)                //
        lw      t0,8(a0)                //
        bne     t2,t3,20f               // if ne, second word not equal
        lwr     t1,8(a1)                //
        lwl     t1,11(a1)               //
        lw      t2,12(a0)               //
        bne     t0,t1,30f               // if ne, third word not equal
        lwr     t3,12(a1)               //
        lwl     t3,15(a1)               //
        lw      t0,16(a0)               //
        bne     t2,t3,40f               // if ne, fourth word not equal
        lwr     t1,16(a1)               //
        lwl     t1,19(a1)               //
        lw      t2,20(a0)               //
        bne     t0,t1,50f               // if ne, fifth word not equal
        lwr     t3,20(a1)               //
        lwl     t3,23(a1)               //
        lw      t0,24(a0)               //
        bne     t2,t3,60f               // if ne, sixth word not equal
        lwr     t1,24(a1)               //
        lwl     t1,27(a1)               //
        lw      t2,28(a0)               //
        bne     t0,t1,70f               // if ne, seventh word not equal
        lwr     t3,28(a1)               //
        lwl     t3,31(a1)               //
        daddu   a0,a0,32                // advance source1 to next block
        bne     t2,t3,80f               // if ne, eighth word not equal
        nop                             //
        bne     a0,t4,10b               // if ne, more 32-byte blocks to compare
        daddu   a1,a1,32                // update source2 address
        .set    at
        .set    reorder

        dsubu   a2,a3,a0                // compute remaining bytes
        b       100f                    //

//
// Source1 is unaligned, check Source2 alignment.
//

CompareUnalignedS1:                     //
        and     t0,a1,0x3               // isolate Source2 alignment
        bne     zero,t0,CompareUnalignedS1AndS2 // if ne, Source2 unaligned

//
// Source1 is unaligned and Source2 is aligned.
//

        .set    noreorder
        .set    noat
10:     lwr     t0,0(a0)                // compare 32-byte block
        lwl     t0,3(a0)                //
        lw      t1,0(a1)                //
        lwr     t2,4(a0)                //
        lwl     t2,7(a0)                //
        bne     t0,t1,90f               // if ne, first word not equal
        lw      t3,4(a1)                //
        lwr     t0,8(a0)                //
        lwl     t0,11(a0)               //
        bne     t2,t3,20f               // if ne, second word not equal
        lw      t1,8(a1)                //
        lwr     t2,12(a0)               //
        lwl     t2,15(a0)               //
        bne     t0,t1,30f               // if ne, third word not equal
        lw      t3,12(a1)               //
        lwr     t0,16(a0)               //
        lwl     t0,19(a0)               //
        bne     t2,t3,40f               // if ne, fourth word not equal
        lw      t1,16(a1)               //
        lwr     t2,20(a0)               //
        lwl     t2,23(a0)               //
        bne     t0,t1,50f               // if ne, fifth word not equal
        lw      t3,20(a1)               //
        lwr     t0,24(a0)               //
        lwl     t0,27(a0)               //
        bne     t2,t3,60f               // if ne, sixth word not equal
        lw      t1,24(a1)               //
        lwr     t2,28(a0)               //
        lwl     t2,31(a0)               //
        bne     t0,t1,70f               // if ne, seventh word not equal
        lw      t3,28(a1)               //
        daddu   a0,a0,32                // advance source1 to next block
        bne     t2,t3,80f               // if ne, eighth word not equal
        nop                             //
        bne     a0,t4,10b               // if ne, more 32-byte blocks to compare
        daddu   a1,a1,32                // update source2 address
        .set    at
        .set    reorder

        dsubu   a2,a3,a0                // compute remaining bytes
        b       100f                    //

//
// Source1 and Source2 are unaligned.
//

CompareUnalignedS1AndS2:                //

        .set    noreorder
        .set    noat
10:     lwr     t0,0(a0)                // compare 32-byte block
        lwl     t0,3(a0)                //
        lwr     t1,0(a1)                //
        lwl     t1,3(a1)                //
        lwr     t2,4(a0)                //
        lwl     t2,7(a0)                //
        bne     t0,t1,90f               // if ne, first word not equal
        lwr     t3,4(a1)                //
        lwl     t3,7(a1)                //
        lwr     t0,8(a0)                //
        lwl     t0,11(a0)               //
        bne     t2,t3,20f               // if ne, second word not equal
        lwr     t1,8(a1)                //
        lwl     t1,11(a1)               //
        lwr     t2,12(a0)               //
        lwl     t2,15(a0)               //
        bne     t0,t1,30f               // if ne, third word not equal
        lwr     t3,12(a1)               //
        lwl     t3,15(a1)               //
        lwr     t0,16(a0)               //
        lwl     t0,19(a0)               //
        bne     t2,t3,40f               // if ne, fourth word not equal
        lwr     t1,16(a1)               //
        lwl     t1,19(a1)               //
        lwr     t2,20(a0)               //
        lwl     t2,23(a0)               //
        bne     t0,t1,50f               // if ne, fifth word not equal
        lwr     t3,20(a1)               //
        lwl     t3,23(a1)               //
        lwr     t0,24(a0)               //
        lwl     t0,27(a0)               //
        bne     t2,t3,60f               // if ne, sixth word not equal
        lwr     t1,24(a1)               //
        lwl     t1,27(a1)               //
        lwr     t2,28(a0)               //
        lwl     t2,31(a0)               //
        bne     t0,t1,70f               // if ne, seventh word not equal
        lwr     t3,28(a1)               //
        lwl     t3,31(a1)               //
        daddu   a0,a0,32                // advance source1 to next block
        bne     t2,t3,80f               // if ne, eighth word not equal
        nop                             //
        bne     a0,t4,10b               // if ne, more 32-byte blocks to compare
        daddu   a1,a1,32                // update source2 address
        .set    at
        .set    reorder

        dsubu   a2,a3,a0                // compute remaining bytes
        b       100f                    //

//
// Adjust source1 and source2 pointers dependent on position of miscompare in
// block.
//

20:    daddu    a0,a0,4                // mismatch on second word
       daddu    a1,a1,4                //
       b        90f                    //

30:    daddu    a0,a0,8                // mismatch on third word
       daddu    a1,a1,8                //
       b        90f                    //

40:    daddu    a0,a0,12               // mistmatch on fourth word
       daddu    a1,a1,12               //
       b        90f                    //

50:    daddu    a0,a0,16               // mismatch on fifth word
       daddu    a1,a1,16               //
       b        90f                    //

60:    daddu    a0,a0,20               // mismatch on sixth word
       daddu    a1,a1,20               //
       b        90f                    //

70:    daddu    a0,a0,24               // mismatch on seventh word
       daddu    a1,a1,24               //
       b        90f                    //

80:    dsubu    a0,a0,4                // mismatch on eighth word
       daddu    a1,a1,28               //
90:    dsubu    a2,a3,a0               // compute remaining bytes

//
// Compare 1-byte blocks.
//

100:    daddu   t2,a0,a2                // compute ending block address
        beq     zero,a2,120f            // if eq, no bytes to zero
110:    lb      t0,0(a0)                // compare 1-byte block
        lb      t1,0(a1)                //
        daddu   a1,a1,1                 // advance pointers to next block
        bne     t0,t1,120f              // if ne, byte not equal
        daddu   a0,a0,1                 //
        bne     a0,t2,110b              // if ne, more 1-byte block to zero

120:    dsubu   t0,a3,a0                // compute number of bytes not compared
        dsubu   v0,v0,t0                // compute number of byte that matched
        j       ra                      // return

        .end    RtlCompareMemory

        SBTTL("Equal Memory")
//++
//
// ULONG
// RtlEqualMemory (
//    IN PVOID Source1,
//    IN PVOID Source2,
//    IN ULONG Length
//    )
//
// Routine Description:
//
//    This function compares two blocks of memory for equality.
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
//    If all bytes in the source strings match, then a value of TRUE is
//    returned. Otherwise, FALSE is returned.
//
//--

        LEAF_ENTRY(RtlEqualMemory)

        li      v0,FALSE                // set return value FALSE
        daddu   a3,a0,a2                // compute ending address of source1
        and     t0,a2,16 - 1            // isolate residual bytes
        dsubu   t1,a2,t0                // subtract out residual bytes
        daddu   t4,a0,t1                // compute ending block address
        beq     zero,t1,20f             // if eq, no 16-byte block to compare
        or      t0,a0,a1                // merge and isolate alignment bits
        and     t0,t0,0x3               //
        bne     zero,t0,EqualUnaligned  // if ne, unalignment comparison

//
// Compare memory aligned.
//

EqualAligned:                           //

        .set    noreorder
        .set    noat
10:     lw      t0,0(a0)                // compare 16-byte block
        lw      t1,0(a1)                //
        lw      t2,4(a0)                //
        bne     t0,t1,50f               // if ne, first word not equal
        lw      t3,4(a1)                //
        lw      t0,8(a0)                //
        bne     t2,t3,50f               // if ne, second word not equal
        lw      t1,8(a1)                //
        lw      t2,12(a0)               //
        bne     t0,t1,50f               // if ne, third word not equal
        lw      t3,12(a1)               //
        bne     t2,t3,50f               // if ne, eighth word not equal
        daddu   a0,a0,16                // advance source1 to next block
        bne     a0,t4,10b               // if ne, more blocks to compare
        daddu   a1,a1,16                // advance source2 to next block
        .set    at
        .set    reorder

        dsubu   a2,a3,a0                // compute remaining bytes
        b       20f                     //

//
// Compare memory unaligned.
//

EqualUnaligned:                         //
        and     t0,a0,0x3               // isolate source1 alignment
        bne     zero,t0,EqualUnalignedS1 // if ne, source1 unaligned

//
// Source1 is aligned and Source2 is unaligned.
//

EqualUnalignedS2:                       //

        .set    noreorder
        .set    noat
10:     lw      t0,0(a0)                // compare 16-byte block
        lwr     t1,0(a1)                //
        lwl     t1,3(a1)                //
        lw      t2,4(a0)                //
        bne     t0,t1,50f               // if ne, first word not equal
        lwr     t3,4(a1)                //
        lwl     t3,7(a1)                //
        lw      t0,8(a0)                //
        bne     t2,t3,50f               // if ne, second word not equal
        lwr     t1,8(a1)                //
        lwl     t1,11(a1)               //
        lw      t2,12(a0)               //
        bne     t0,t1,50f               // if ne, third word not equal
        lwr     t3,12(a1)               //
        lwl     t3,15(a1)               //
        bne     t2,t3,50f               // if ne, fourth word not equal
        daddu   a0,a0,16                // advance source1 to next block
        bne     a0,t4,10b               // if ne, more blocks to compare
        daddu   a1,a1,16                // advance source2 to next block
        .set    at
        .set    reorder

        dsubu   a2,a3,a0                // compute remaining bytes
        b       20f                     //

//
// Source1 is unaligned, check Source2 alignment.
//

EqualUnalignedS1:                       //
        and     t0,a1,0x3               // isolate Source2 alignment
        bne     zero,t0,EqualUnalignedS1AndS2 // if ne, Source2 unaligned

//
// Source1 is unaligned and Source2 is aligned.
//

        .set    noreorder
        .set    noat
10:     lwr     t0,0(a0)                // compare 16-byte block
        lwl     t0,3(a0)                //
        lw      t1,0(a1)                //
        lwr     t2,4(a0)                //
        lwl     t2,7(a0)                //
        bne     t0,t1,50f               // if ne, first word not equal
        lw      t3,4(a1)                //
        lwr     t0,8(a0)                //
        lwl     t0,11(a0)               //
        bne     t2,t3,50f               // if ne, second word not equal
        lw      t1,8(a1)                //
        lwr     t2,12(a0)               //
        lwl     t2,15(a0)               //
        bne     t0,t1,50f               // if ne, third word not equal
        lw      t3,12(a1)               //
        bne     t2,t3,50f               // if ne, fourth word not equal
        daddu   a0,a0,16                // advance source1 to next block
        bne     a0,t4,10b               // if ne, more blocks to compare
        daddu   a1,a1,16                // advance source2 to next block
        .set    at
        .set    reorder

        dsubu   a2,a3,a0                // compute remaining bytes
        b       20f                     //

//
// Source1 and Source2 are unaligned.
//

EqualUnalignedS1AndS2:                  //

        .set    noreorder
        .set    noat
10:     lwr     t0,0(a0)                // compare 16-byte block
        lwl     t0,3(a0)                //
        lwr     t1,0(a1)                //
        lwl     t1,3(a1)                //
        lwr     t2,4(a0)                //
        lwl     t2,7(a0)                //
        bne     t0,t1,50f               // if ne, first word not equal
        lwr     t3,4(a1)                //
        lwl     t3,7(a1)                //
        lwr     t0,8(a0)                //
        lwl     t0,11(a0)               //
        bne     t2,t3,50f               // if ne, second word not equal
        lwr     t1,8(a1)                //
        lwl     t1,11(a1)               //
        lwr     t2,12(a0)               //
        lwl     t2,15(a0)               //
        bne     t0,t1,50f               // if ne, third word not equal
        lwr     t3,12(a1)               //
        lwl     t3,15(a1)               //
        bne     t2,t3,50f               // if ne, fourth word not equal
        daddu   a0,a0,16                // advance source1 to next block
        bne     a0,t4,10b               // if ne, more blocks to compare
        daddu   a1,a1,16                // advance source2 to next block
        .set    at
        .set    reorder

        dsubu   a2,a3,a0                // compute remaining bytes

//
// Compare 1-byte blocks.
//

20:     daddu   t2,a0,a2                // compute ending block address
        beq     zero,a2,40f             // if eq, no bytes to zero
30:     lb      t0,0(a0)                // compare 1-byte block
        lb      t1,0(a1)                //
        daddu   a1,a1,1                 // advance pointers to next block
        bne     t0,t1,50f               // if ne, byte not equal
        daddu   a0,a0,1                 //
        bne     a0,t2,30b               // if ne, more 1-byte block to zero
40:     li      v0,TRUE                 // set return value TRUE

50:     j       ra                      // return

        .end    RtlEqualMemory

        SBTTL("Move Memory")
//++
//
// PVOID
// RtlMoveMemory (
//    IN PVOID Destination,
//    IN PVOID Source,
//    IN ULONG Length
//    )
//
// PVOID64
// RtlMoveMemory64 (
//    IN PVOID64 Destination,
//    IN PVOID64 Source,
//    IN ULONG Length
//    )
//
// Routine Description:
//
//    This function moves memory either forward or backward, aligned or
//    unaligned, in 32-byte blocks, followed by 4-byte blocks, followed
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
//    The Destination address is returned as the function value.
//
//    N.B. The C runtime entry points memmove and memcpy are equivalent to
//         RtlMoveMemory thus alternate entry points are provided for these
//         routines.
//--

        LEAF_ENTRY(RtlMoveMemory)

        ALTERNATE_ENTRY(RtlMoveMemory64)
        ALTERNATE_ENTRY(memcpy)
        ALTERNATE_ENTRY(memmove)

        move    v0,a0                   // set return value

//
// If the source address is less than the destination address and source
// address plus the length of the move is greater than the destination
// address, then the source and destination overlap such that the move
// must be performed backwards.
//

10:     bgeu    a1,a0,MoveForward       // if geu, no overlap possible
        daddu   t0,a1,a2                // compute source ending address
        bgtu    t0,a0,MoveBackward      // if gtu, source and destination overlap

//
// Move memory forward aligned and unaligned.
//

MoveForward:                            //
        sltu    t0,a2,8                 // check if less than eight bytes
        bne     zero,t0,50f             // if ne, less than eight bytes to move
        xor     t0,a0,a1                // compare alignment bits
        and     t0,t0,0x7               // isolate alignment comparison
        bne     zero,t0,MoveForwardUnaligned // if ne, incompatible alignment

//
// Move memory forward aligned.
//

MoveForwardAligned:                     //
        dsubu   t0,zero,a0              // compute bytes until aligned
        and     t0,t0,0x7               // isolate residual byte count
        dsubu   a2,a2,t0                // reduce number of bytes to move
        beq     zero,t0,10f             // if eq, already aligned
        ldr     t1,0(a1)                // move unaligned bytes
        sdr     t1,0(a0)                //
        daddu   a0,a0,t0                // align destination address
        daddu   a1,a1,t0                // align source address

//
// Check for 32-byte blocks to move.
//

10:     and     t0,a2,32 - 1            // isolate residual bytes
        dsubu   t1,a2,t0                // subtract out residual bytes
        daddu   t8,a0,t1                // compute ending block address
        beq     zero,t1,30f             // if eq, no 32-byte block to zero
        move    a2,t0                   // set residual number of bytes

//
// Check for odd number of 32-byte blocks to move.
//

        and     t0,t1,1 << 5            // test if even number of 32-byte blocks
        beq     zero,t0,20f             // if eq, even number of 32-byte blocks

//
// Move one 32-byte block quadword aligned.
//

        .set    noreorder
        ld      t0,0(a1)                // move 32-byte block
        ld      t1,8(a1)                //
        ld      t2,16(a1)               //
        ld      t3,24(a1)               //
        sd      t0,0(a0)                //
        sd      t1,8(a0)                //
        sd      t2,16(a0)               //
        sd      t3,24(a0)               //
        daddu   a0,a0,32                // advance pointers to next block
        beq     a0,t8,30f               // if eq, end of block
        daddu   a1,a1,32                //
        .set    reorder

//
// Move 64-byte blocks quadword aligned.
//

        .set    noreorder
20:     ld      t0,0(a1)                // move 64-byte block
        ld      t1,8(a1)                //
        ld      t2,16(a1)               //
        ld      t3,24(a1)               //
        ld      t4,32(a1)               //
        ld      t5,40(a1)               //
        ld      t6,48(a1)               //
        ld      t7,56(a1)               //
        sd      t0,0(a0)                //
        sd      t1,8(a0)                //
        sd      t2,16(a0)               //
        sd      t3,24(a0)               //
        sd      t4,32(a0)               //
        sd      t5,40(a0)               //
        sd      t6,48(a0)               //
        sd      t7,56(a0)               //
        daddu   a0,a0,64                // advance pointers to next block
        bne     a0,t8,20b               // if ne, more 64-byte blocks to zero
        daddu   a1,a1,64                //
        .set    reorder

//
// Check for 4-byte blocks to move.
//

30:     and     t0,a2,4 - 1             // isolate residual bytes
        dsubu   t1,a2,t0                // subtract out residual bytes
        daddu   t2,a0,t1                // compute ending block address
        beq     zero,t1,50f             // if eq, no 4-byte block to zero
        move    a2,t0                   // set residual number of bytes

//
// Move 4-byte block.
//

        .set    noreorder
40:     lw      t0,0(a1)                // move 4-byte block
        daddu   a0,a0,4                 // advance pointers to next block
        sw      t0,-4(a0)               //
        bne     a0,t2,40b               // if ne, more 4-byte blocks to zero
        daddu   a1,a1,4                 //
        .set    reorder

//
// Move 1-byte blocks.
//

50:     daddu   t2,a0,a2                // compute ending block address
        beq     zero,a2,70f             // if eq, no bytes to zero

        .set    noreorder
60:     lb      t0,0(a1)                // move 1-byte block
        daddu   a0,a0,1                 // advance pointers to next block
        sb      t0,-1(a0)               //
        bne     a0,t2,60b               // if ne, more 1-byte block to zero
        daddu   a1,a1,1                 //
        .set    reorder

70:     j       ra                      // return

//
// Move memory forward unaligned.
//

MoveForwardUnaligned:                   //
        dsubu   t0,zero,a0              // compute bytes until aligned
        and     t0,t0,0x7               // isolate residual byte count
        dsubu   a2,a2,t0                // reduce number of bytes to move
        beq     zero,t0,10f             // if eq, already aligned
        ldr     t1,0(a1)                // move unaligned bytes
        ldl     t1,7(a1)                //
        sdr     t1,0(a0)                //
        daddu   a0,a0,t0                // align destination address
        daddu   a1,a1,t0                // update source address

//
// Check for 32-byte blocks to move.
//

10:     and     t0,a2,32 - 1            // isolate residual bytes
        dsubu   t1,a2,t0                // subtract out residual bytes
        daddu   t8,a0,t1                // compute ending block address
        beq     zero,t1,30f             // if eq, no 32-byte block to zero
        move    a2,t0                   // set residual number of bytes

//
// Check for odd number of 32-byte blocks to move.
//

        and     t0,t1,1 << 5            // test if even number of 32-byte blocks
        beq     zero,t0,20f             // if eq, even number of 32-byte blocks

//
// Move one 32-byte block quadword aligned.
//

        .set    noreorder
        ldr     t0,0(a1)                // move 32-byte block
        ldl     t0,7(a1)                //
        ldr     t1,8(a1)                //
        ldl     t1,15(a1)               //
        ldr     t2,16(a1)               //
        ldl     t2,23(a1)               //
        ldr     t3,24(a1)               //
        ldl     t3,31(a1)               //
        sd      t0,0(a0)                //
        sd      t1,8(a0)                //
        sd      t2,16(a0)               //
        sd      t3,24(a0)               //
        daddu   a0,a0,32                // advance pointers to next block
        beq     a0,t8,30f               // if eq, end of block
        daddu   a1,a1,32                //
        .set    reorder

//
// Move 64-byte block.
//

        .set    noreorder
20:     ldr     t0,0(a1)                // move 64-byte block
        ldl     t0,7(a1)                //
        ldr     t1,8(a1)                //
        ldl     t1,15(a1)               //
        ldr     t2,16(a1)               //
        ldl     t2,23(a1)               //
        ldr     t3,24(a1)               //
        ldl     t3,31(a1)               //
        ldr     t4,32(a1)               //
        ldl     t4,39(a1)               //
        ldr     t5,40(a1)               //
        ldl     t5,47(a1)               //
        ldr     t6,48(a1)               //
        ldl     t6,55(a1)               //
        ldr     t7,56(a1)               //
        ldl     t7,63(a1)               //
        sd      t0,0(a0)                //
        sd      t1,8(a0)                //
        sd      t2,16(a0)               //
        sd      t3,24(a0)               //
        sd      t4,32(a0)               //
        sd      t5,40(a0)               //
        sd      t6,48(a0)               //
        sd      t7,56(a0)               //
        daddu   a0,a0,64                // advance pointers to next block
        bne     a0,t8,20b               // if ne, more 32-byte blocks to zero
        daddu   a1,a1,64                //
        .set    reorder

//
// Check for 4-byte blocks to move.
//

30:     and     t0,a2,4 - 1             // isolate residual bytes
        dsubu   t1,a2,t0                // subtract out residual bytes
        daddu   t2,a0,t1                // compute ending block address
        beq     zero,t1,50f             // if eq, no 4-byte block to zero
        move    a2,t0                   // set residual number of bytes

//
// Move 4-byte block.
//

        .set    noreorder
40:     lwr     t0,0(a1)                // move 4-byte block
        lwl     t0,3(a1)                //
        daddu   a0,a0,4                 // advance pointers to next block
        sw      t0,-4(a0)               //
        bne     a0,t2,40b               // if ne, more 4-byte blocks to zero
        daddu   a1,a1,4                 //
        .set    reorder

//
// Move 1-byte blocks.
//

50:     daddu   t2,a0,a2                // compute ending block address
        beq     zero,a2,70f             // if eq, no bytes to zero

        .set    noreorder
60:     lb      t0,0(a1)                // move 1-byte block
        daddu   a0,a0,1                 // advance pointers to next block
        sb      t0,-1(a0)               //
        bne     a0,t2,60b               // if ne, more 1-byte block to zero
        daddu   a1,a1,1                 //
        .set    reorder

70:     j       ra                      // return

//
// Move memory backward.
//

MoveBackward:                           //
        daddu   a0,a0,a2                // compute ending destination address
        daddu   a1,a1,a2                // compute ending source address
        sltu    t0,a2,8                 // check if less than eight bytes
        bne     zero,t0,50f             // if ne, less than eight bytes to move
        xor     t0,a0,a1                // compare alignment bits
        and     t0,t0,0x7               // isolate alignment comparison
        bne     zero,t0,MoveBackwardUnaligned // if ne, incompatible alignment

//
// Move memory backward aligned.
//

MoveBackwardAligned:                    //
        and     t0,a0,0x7               // isolate residual byte count
        dsubu   a2,a2,t0                // reduce number of bytes to move
        beq     zero,t0,10f             // if eq, already aligned
        ldl     t1,-1(a1)               // move unaligned bytes
        sdl     t1,-1(a0)               //
        dsubu   a0,a0,t0                // align destination address
        dsubu   a1,a1,t0                // align source address

//
// Check for 32-byte blocks to move.
//

10:     and     t0,a2,32 - 1            // isolate residual bytes
        dsubu   t1,a2,t0                // subtract out residual bytes
        dsubu   t8,a0,t1                // compute ending block address
        beq     zero,t1,30f             // if eq, no 32-byte block to zero
        move    a2,t0                   // set residual number of bytes

//
// Check for odd number of 32-byte blocks to move.
//

        and     t0,t1,1 << 5            // test if even number of 32-byte blocks
        beq     zero,t0,20f             // if eq, even number of 32-byte blocks

//
// Move one 32-byte block quadword aligned.
//

        .set    noreorder
        ld      t0,-8(a1)               // move 32-byte block
        ld      t1,-16(a1)              //
        ld      t2,-24(a1)              //
        ld      t3,-32(a1)              //
        sd      t0,-8(a0)               //
        sd      t1,-16(a0)              //
        sd      t2,-24(a0)              //
        sd      t3,-32(a0)              //
        dsubu   a0,a0,32                // advance pointers to next block
        beq     a0,t8,30f               // if eq, end of block
        dsubu   a1,a1,32                //
        .set    reorder

//
// Move 64-byte blocks quadword aligned.
//

        .set    noreorder
20:     ld      t0,-8(a1)               // move 64-byte block
        ld      t1,-16(a1)              //
        ld      t2,-24(a1)              //
        ld      t3,-32(a1)              //
        ld      t4,-40(a1)              //
        ld      t5,-48(a1)              //
        ld      t6,-56(a1)              //
        ld      t7,-64(a1)              //
        sd      t0,-8(a0)               //
        sd      t1,-16(a0)              //
        sd      t2,-24(a0)              //
        sd      t3,-32(a0)              //
        sd      t4,-40(a0)              //
        sd      t5,-48(a0)              //
        sd      t6,-56(a0)              //
        sd      t7,-64(a0)              //
        dsubu   a0,a0,64                // advance pointers to next block
        bne     a0,t8,20b               // if ne, more 64-byte blocks to zero
        dsubu   a1,a1,64                //
        .set    reorder

//
// Check for 4-byte blocks to move.
//

30:     and     t0,a2,4 - 1             // isolate residual bytes
        dsubu   t1,a2,t0                // subtract out residual bytes
        dsubu   t2,a0,t1                // compute ending block address
        beq     zero,t1,50f             // if eq, no 4-byte block to zero
        move    a2,t0                   // set residual number of bytes

//
// Move 4-byte block.
//

        .set    noreorder
40:     lw      t0,-4(a1)               // move 4-byte block
        dsubu   a0,a0,4                 // advance pointers to next block
        sw      t0,0(a0)                //
        bne     a0,t2,40b               // if ne, more 4-byte blocks to zero
        dsubu   a1,a1,4                 //
        .set    reorder

//
// Move 1-byte blocks.
//

50:     dsubu   t2,a0,a2                // compute ending block address
        beq     zero,a2,70f             // if eq, no bytes to zero

        .set    noreorder
60:     lb      t0,-1(a1)               // move 1-byte block
        dsubu   a0,a0,1                 // advance pointers to next block
        sb      t0,0(a0)                //
        bne     a0,t2,60b               // if ne, more 1-byte block to zero
        dsubu   a1,a1,1                 //
        .set    reorder

70:     j       ra                      // return

//
// Move memory backward unaligned.
//

MoveBackwardUnaligned:                  //
        and     t0,a0,0x7               // isolate residual byte count
        dsubu   a2,a2,t0                // reduce number of bytes to move
        beq     zero,t0,10f             // if eq, already aligned
        ldl     t1,-1(a1)               // move unaligned bytes
        ldr     t1,-8(a1)               //
        sdl     t1,-1(a0)               //
        dsubu   a0,a0,t0                // align destination address
        dsubu   a1,a1,t0                // update source address

//
// Check for 32-byte blocks to move.
//

10:     and     t0,a2,32 - 1            // isolate residual bytes
        dsubu   t1,a2,t0                // subtract out residual bytes
        dsubu   t8,a0,t1                // compute ending block address
        beq     zero,t1,30f             // if eq, no 32-byte block to zero
        move    a2,t0                   // set residual number of bytes

//
// Check for odd number of 32-byte blocks to move.
//

        and     t0,t1,1 << 5            // test if even number of 32-byte blocks
        beq     zero,t0,20f             // if eq, even number of 32-byte blocks

//
// Move one 32-byte block.
//

        .set    noreorder
        ldr     t0,-8(a1)               // move 32-byte block
        ldl     t0,-1(a1)               //
        ldr     t1,-16(a1)              //
        ldl     t1,-9(a1)               //
        ldr     t2,-24(a1)              //
        ldl     t2,-17(a1)              //
        ldr     t3,-32(a1)              //
        ldl     t3,-25(a1)              //
        sd      t0,-8(a0)               //
        sd      t1,-16(a0)              //
        sd      t2,-24(a0)              //
        sd      t3,-32(a0)              //
        dsubu   a0,a0,32                // advance pointers to next block
        beq     a0,t8,30f               // if eq, end of block
        dsubu   a1,a1,32                //
        .set    reorder

//
// Move 32-byte block.
//

        .set    noreorder
20:     ldr     t0,-8(a1)               // move 32-byte block
        ldl     t0,-1(a1)               //
        ldr     t1,-16(a1)              //
        ldl     t1,-9(a1)               //
        ldr     t2,-24(a1)              //
        ldl     t2,-17(a1)              //
        ldr     t3,-32(a1)              //
        ldl     t3,-25(a1)              //
        ldr     t4,-40(a1)              //
        ldl     t4,-33(a1)              //
        ldr     t5,-48(a1)              //
        ldl     t5,-41(a1)              //
        ldr     t6,-56(a1)              //
        ldl     t6,-49(a1)              //
        ldr     t7,-64(a1)              //
        ldl     t7,-57(a1)              //
        sd      t0,-8(a0)               //
        sd      t1,-16(a0)              //
        sd      t2,-24(a0)              //
        sd      t3,-32(a0)              //
        sd      t4,-40(a0)              //
        sd      t5,-48(a0)              //
        sd      t6,-56(a0)              //
        sd      t7,-64(a0)              //
        dsubu   a0,a0,64                // advance pointers to next block
        bne     a0,t8,20b               // if ne, more 64-byte blocks to zero
        dsubu   a1,a1,64                //
        .set    reorder

//
// Check for 4-byte blocks to move.
//

30:     and     t0,a2,4 - 1             // isolate residual bytes
        dsubu   t1,a2,t0                // subtract out residual bytes
        dsubu   t2,a0,t1                // compute ending block address
        beq     zero,t1,50f             // if eq, no 4-byte block to zero
        move    a2,t0                   // set residual number of bytes

//
// Move 4-byte block.
//

        .set    noreorder
40:     lwr     t0,-4(a1)               // move 4-byte block
        lwl     t0,-1(a1)               //
        dsubu   a0,a0,4                 // advance pointers to next block
        sw      t0,0(a0)                //
        bne     a0,t2,40b               // if ne, more 4-byte blocks to zero
        dsubu   a1,a1,4                 //
        .set    reorder

//
// Move 1-byte blocks.
//

50:     dsubu   t2,a0,a2                // compute ending block address
        beq     zero,a2,70f             // if eq, no bytes to zero

        .set    noreorder
60:     lb      t0,-1(a1)               // move 1-byte block
        dsubu   a0,a0,1                 // advance pointers to next block
        sb      t0,0(a0)                //
        bne     a0,t2,60b               // if ne, more 1-byte block to zero
        dsubu   a1,a1,1                 //
        .set    reorder

70:     j       ra                      // return

        .end    RtlMoveMemory

        SBTTL("Zero Memory")
//++
//
// PVOID
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
//    a longword boundary, and then zeroing 32-byte blocks, followed by 4-byte
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
//    The destination address is returned as the function value.
//
//--

        LEAF_ENTRY(RtlZeroMemory)

        ALTERNATE_ENTRY(RtlZeroMemory64)

        move    a2,zero                 // set fill pattern
        b       RtlpFillMemory          //


        SBTTL("Fill Memory")
//++
//
// PVOID
// RtlFillMemory (
//    IN PVOID64 Destination,
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
//         to be reversed.
//
// Return Value:
//
//    The destination address is returned as the function value.
//
//--

        ALTERNATE_ENTRY(memset)

        move    a3,a1                   // swap length and fill arguments
        move    a1,a2                   //
        move    a2,a3                   //

        ALTERNATE_ENTRY(RtlFillMemory)
        ALTERNATE_ENTRY(RtlFillMemory64)

        and     a2,a2,0xff              // clear excess bits
        sll     t0,a2,8                 // duplicate fill byte
        or      a2,a2,t0                // generate fill word
        sll     t0,a2,16                // duplicate fill word
        or      a2,a2,t0                // generate fill longword

//
// Fill memory with the pattern specified in register a2.
//

RtlpFillMemory:                         //
        move    v0,a0                   // set return value
        dsll    a2,a2,32                // duplicate pattern to 64-bits
        dsrl    t0,a2,32                //
        or      a2,a2,t0                //
        dsubu   t0,zero,a0              // compute bytes until aligned
        and     t0,t0,0x7               // isolate residual byte count
        dsubu   t1,a1,t0                // reduce number of bytes to fill
        blez    t1,60f                  // if lez, less than 8 bytes to fill
        move    a1,t1                   // set number of bytes to fill
        beq     zero,t0,10f             // if eq, already aligned
        sdr     a2,0(a0)                // fill unaligned bytes
        daddu   a0,a0,t0                // align destination address

//
// Check for 32-byte blocks to fill.
//

10:     and     t0,a1,32 - 1            // isolate residual bytes
        dsubu   t1,a1,t0                // subtract out residual bytes
        daddu   t2,a0,t1                // compute ending block address
        beq     zero,t1,40f             // if eq, no 32-byte blocks to fill
        move    a1,t0                   // set residual number of bytes

//
// Fill 32-byte blocks.
//

        and     t0,a0,1 << 2            // check if destintion quadword aligned
        beq     zero,t0,20f             // if eq, yes
        sw      a2,0(a0)                // store destination longword
        daddu   a0,a0,4                 // align destination address
        daddu   a1,a1,t1                // recompute bytes to fill
        dsubu   a1,a1,4                 // reduce count by 4
        b       10b                     //

//
// The destination is quadword aligned.
//

20:     and     t0,t1,1 << 5            // test if even number of 32-byte blocks
        beq     zero,t0,30f             // if eq, even number of 32-byte blocks

//
// Fill one 32-byte block.
//

        .set    noreorder
        .set    noat
        sd      a2,0(a0)                // fill 32-byte block
        sd      a2,8(a0)                //
        sd      a2,16(a0)               //
        daddu   a0,a0,32                // advance pointer to next block
        beq     a0,t2,40f               // if ne, no 64-byte blocks to fill
        sd      a2,-8(a0)               //
        .set    at
        .set    reorder

//
// Fill 64-byte block.
//

        .set    noreorder
        .set    noat
30:     sd      a2,0(a0)                // fill 32-byte block
        sd      a2,8(a0)                //
        sd      a2,16(a0)               //
        sd      a2,24(a0)               //
        sd      a2,32(a0)               //
        sd      a2,40(a0)               //
        sd      a2,48(a0)               //
        daddu   a0,a0,64                // advance pointer to next block
        bne     a0,t2,30b               // if ne, more 32-byte blocks to fill
        sd      a2,-8(a0)               //
        .set    at
        .set    reorder

//
// Check for 4-byte blocks to fill.
//

40:     and     t0,a1,4 - 1             // isolate residual bytes
        dsubu   t1,a1,t0                // subtract out residual bytes
        daddu   t2,a0,t1                // compute ending block address
        beq     zero,t1,60f             // if eq, no 4-byte block to fill
        move    a1,t0                   // set residual number of bytes

//
// Fill 4-byte blocks.
//

        .set    noreorder
50:     daddu   a0,a0,4                 // advance pointer to next block
        bne     a0,t2,50b               // if ne, more 4-byte blocks to fill
        sw      a2,-4(a0)               // fill 4-byte block
        .set    reorder

//
// Check for 1-byte blocks to fill.
//

60:     daddu   t2,a0,a1                // compute ending block address
        beq     zero,a1,80f             // if eq, no bytes to fill

//
// Fill 1-byte blocks.
//

        .set    noreorder
70:     daddu   a0,a0,1                 // advance pointer to next block
        bne     a0,t2,70b               // if ne, more 1-byte block to fill
        sb      a2,-1(a0)               // fill 1-byte block
        .set    reorder

80:     j       ra                      // return

        .end    RtlZeroMemory

        SBTTL("Fill Memory Ulong")
//++
//
// PVOID
// RtlFillMemoryUlong (
//    IN PVOID Destination,
//    IN ULONG Length,
//    IN ULONG Pattern
//    )
//
// Routine Description:
//
//    This function fills memory with the specified longowrd pattern by
//    filling 32-byte blocks followed by 4-byte blocks.
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
//    The destination address is returned as the function value.
//
//--

        LEAF_ENTRY(RtlFillMemoryUlong)

        move    v0,a0                   // set function value
        srl     a1,a1,2                 // make sure length is an even number
        sll     a1,a1,2                 // of longwords
        dsll    a2,a2,32                // duplicate pattern to 64-bits
        dsrl    t0,a2,32                //
        or      a2,a2,t0                //

//
// Check for 32-byte blocks to fill.
//

10:     and     t0,a1,32 - 1            // isolate residual bytes
        dsubu   t1,a1,t0                // subtract out residual bytes
        daddu   t2,a0,t1                // compute ending block address
        beq     zero,t1,40f             // if eq, no 32-byte blocks to fill
        move    a1,t0                   // set residual number of bytes

//
// Fill 32-byte blocks.
//

        and     t0,a0,1 << 2            // check if destintion quadword aligned
        beq     zero,t0,20f             // if eq, yes
        sw      a2,0(a0)                // store destination longword
        daddu   a0,a0,4                 // align destination address
        daddu   a1,a1,t1                // recompute bytes to fill
        dsubu   a1,a1,4                 // reduce count by 4
        b       10b                     //

//
// The destination is quadword aligned.
//

20:     and     t0,t1,1 << 5            // test if even number of 32-byte blocks
        beq     zero,t0,30f             // if eq, even number of 32-byte blocks

//
// Fill one 32-byte block.
//

        .set    noreorder
        .set    noat
        sd      a2,0(a0)                // fill 32-byte block
        sd      a2,8(a0)                //
        sd      a2,16(a0)               //
        daddu   a0,a0,32                // advance pointer to next block
        beq     a0,t2,40f               // if ne, no 64-byte blocks to fill
        sd      a2,-8(a0)               //
        .set    at
        .set    reorder

//
// Fill 64-byte block.
//

        .set    noreorder
        .set    noat
30:     sd      a2,0(a0)                // fill 32-byte block
        sd      a2,8(a0)                //
        sd      a2,16(a0)               //
        sd      a2,24(a0)               //
        sd      a2,32(a0)               //
        sd      a2,40(a0)               //
        sd      a2,48(a0)               //
        daddu   a0,a0,64                // advance pointer to next block
        bne     a0,t2,30b               // if ne, more 32-byte blocks to fill
        sd      a2,-8(a0)               //
        .set    at
        .set    reorder

//
// Check for 4-byte blocks to fill.
//

40:     daddu   t2,a1,a0                // compute ending block address
        beq     zero,a1,60f             // if eq, no 4-byte block to fill

//
// Fill 4-byte blocks.
//

        .set    noreorder
50:     daddu   a0,a0,4                 // advance pointer to next block
        bne     a0,t2,50b               // if ne, more 4-byte blocks to fill
        sw      a2,-4(a0)               // fill 4-byte block
        .set    reorder

60:     j       ra                      // return

        .end    RtlFillMemoryUlong
